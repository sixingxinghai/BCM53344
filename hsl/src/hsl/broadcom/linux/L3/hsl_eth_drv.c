/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

/* 
   Broadcom includes. 
*/
#include "bcm_incl.h"

/* 
   HAL includes.
*/
#include "hal_types.h"

#ifdef HAVE_L2
#include "hal_l2.h"
#endif /* HAVE_L2 */

#include "hal_msg.h"

/* 
   HSL includes. 
*/
#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_ifmgr.h"
#include "hsl_if_os.h"
#include "hsl_ifmgr.h"
#include "hsl_logger.h"
#include "hsl_ether.h"

/* 
   Broadcom L3 interface driver. 
*/
#include "hsl_eth_drv.h"
#include "hsl_bcm_pkt.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_ifmap.h"

#ifdef HAVE_L2
#include "hsl_vlan.h"
#endif /* HAVE_L2 */

#define ALLOC_FLAGS (in_interrupt () ? GFP_ATOMIC : GFP_KERNEL)

#define HSL_ETH_DRV_TX_THREAD

#ifdef HSL_ETH_DRV_TX_THREAD
/* Tx thread. */
static struct sal_thread_s *hsl_eth_drv_tx_thread = NULL;
static struct sk_buff_head hsl_eth_tx_queue;
static apn_sem_id hsl_eth_tx_sem = NULL;
static volatile int hsl_eth_tx_thread_running;

/* Forward declarations. */
static void _hsl_eth_tx_handler (void *notused);
#endif /* HSL_ETH_DRV_TX_THREAD */
static struct net_device_stats net_stats;

static struct net_device_stats  net_stats;

/* Initiailization. */
int
hsl_eth_drv_init (void)
{
  int ret = 0;

  HSL_FN_ENTER ();

#ifdef HSL_ETH_DRV_TX_THREAD
  ret = oss_sem_new ("Eth Tx sem",
                     OSS_SEM_BINARY,
                     0,
                     NULL,
                     &hsl_eth_tx_sem);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_DEVDRV, HSL_LEVEL_ERROR, "Cannot create ethernet driver Tx synchronization semaphore\n");
      goto ERR;
    }

  hsl_eth_tx_thread_running = 1;
  skb_queue_head_init (&hsl_eth_tx_queue);
 
  /* Create thread for processing Tx. */
  hsl_eth_drv_tx_thread = sal_thread_create ("zEthTx",
                                             SAL_THREAD_STKSZ,
                                             200,
                                             _hsl_eth_tx_handler,
                                             0);
  if (hsl_eth_drv_tx_thread == SAL_THREAD_ERROR)
    {
      HSL_LOG (HSL_LOG_DEVDRV, HSL_LEVEL_ERROR, "Cannot create Tx thread\n");
      goto ERR;
    }
#else
  if (ret < 0) /* to avoid compile warnings. */
    goto ERR;

#endif /* HSL_ETH_DRV_TX_THREAD */

  HSL_FN_EXIT (0);

 ERR:
  hsl_eth_drv_deinit ();
  HSL_FN_EXIT (-1);
}

/* Deinitialization. */
int
hsl_eth_drv_deinit (void)
{
#ifdef HSL_ETH_DRV_TX_THREAD
  struct sk_buff *skb;

  /* Cancel Tx thread. */
  if (hsl_eth_drv_tx_thread)
    {
      hsl_eth_tx_thread_running = 0;
      oss_sem_unlock (OSS_SEM_BINARY, hsl_eth_tx_sem);
    }

  hsl_eth_tx_thread_running = 0;
  while ((skb = skb_dequeue (&hsl_eth_tx_queue)) != NULL)
    kfree_skb (skb);
#endif /* HSL_ETH_DRV_TX_THREAD */

  return 0;
}

/* Untag skb. */
static void
_hsl_eth_untag_skb (struct sk_buff *skb)
{
  enet_hdr_t *th, *uh;
  const u_int16_t dec_length = sizeof(th->en_tag_ctrl)+sizeof(th->en_tag_len);

  th = (enet_hdr_t *) skb->data;
  if (ENET_TAGGED(th)) {                /* If tagged - strip */
    uh = (enet_hdr_t *)((char *)th + dec_length);
    /* Use memmove as it takes care of overlap */
    memmove(uh->en_shost, th->en_shost, /* ORDER --- FIRST */
               sizeof(th->en_shost));
    memmove(uh->en_dhost, th->en_dhost, /* ORDER --- SECOND */
               sizeof(th->en_dhost));
    if (ENET_LEN(uh)) {
      uh->en_untagged_len = th->en_tag_len;
    }
    skb_pull (skb, dec_length);
  }
}

/* Callback for deferred xmit. */
static void
_hsl_eth_xmit_done (int ignored, bcm_pkt_t *pkt, void *cookie)
{
  
  if (pkt)
    {
      /* Free buffer. */
      kfree ((u_char *)pkt->cookie2);

      /* Free pkt. */
      hsl_bcm_tx_pkt_free (pkt);
    }
}

/* Tx routine for pure L3 ports. */
static int
_hsl_eth_drv_l3_tx (struct hsl_if *ifp, struct sk_buff *skb)
{
#if 0
#define ETH_MINLEN                  60
#endif
  int len;
  u_char *buf = NULL;
  bcm_pkt_t *pkt = NULL;
  struct hsl_if *ifpc;
  struct hsl_bcm_if *sifp;
  int ret = 0, offset, length;

  HSL_FN_ENTER ();
#if 0
  length = ETH_MINLEN < skb->len ? skb->len : ETH_MINLEN;
#else
  length = skb->len;
#endif 

  /* Allocate pkt info structure. */
  pkt = hsl_bcm_tx_pkt_alloc (1);
  if (! pkt)
    {
      ret = ENOMEM;
      goto ERR;
    }
 
  /* Set packet flags. */
  pkt->flags |= BCM_TX_CRC_APPEND;
  
  /* Set COS to highest. */
  pkt->cos = 1;

  /* Allocate buffer. */
  buf = kmalloc (HSL_ETH_PKT_SIZE, GFP_KERNEL);
  if (! buf)
    {
      ret = ENOMEM;
      goto ERR;
    }

  /* Copy entire packet. */      
  memcpy (buf + ENET_TAG_SIZE, skb->data, length);

  /* Correct length. */
  len = (skb->len - ENET_UNTAGGED_HDR_LEN) + ENET_TAGGED_HDR_LEN;
  
  /* Set data in pkt. */
  pkt->pkt_data[0].data = buf;
  pkt->pkt_data[0].len = pkt->pkt_len = len;

  pkt->cookie   = pkt;
  pkt->cookie2  = buf;
  /* Fill header. */
  offset = ENET_TAG_SIZE;
  BCM_PKT_HDR_DMAC_SET (pkt, buf + offset);
  offset += 6;
  BCM_PKT_HDR_SMAC_SET (pkt, buf + offset);
  BCM_PKT_HDR_TPID_SET (pkt, 0x8100);
  BCM_PKT_HDR_VTAG_CONTROL_SET (pkt, VLAN_CTRL (0, 0, ifp->u.ip.vid)); 

  /* Set callbacks for deferred xmit. */
  pkt->call_back = _hsl_eth_xmit_done;
      
  /* Access ifmgr and send pkt. */
  ifpc = hsl_ifmgr_get_first_L2_port (ifp);
  if (! ifpc)
    {
      ret = EINVAL;
      goto ERR;
    }

  sifp = ifpc->system_info;
  if (! sifp)
    {
      ret = EINVAL;
      goto ERR;
    }
  /* Send packet. */
  ret = hsl_bcm_pkt_send (pkt, sifp->u.l2.lport, 0);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_DEVDRV, HSL_LEVEL_ERROR, "Failure sending packet\n");
      goto ERR;
    }

  HSL_IFMGR_IF_REF_DEC (ifpc);
  HSL_FN_EXIT (0);

 ERR:
  if (buf)
    kfree (buf);
  if (pkt)
    hsl_bcm_tx_pkt_free (pkt);
  return 0;  
}

/* Tx routine for SVI ports. */
static int
_hsl_eth_drv_svi_tx (struct hsl_if *ifp, struct sk_buff *skb)
{
  int len;
  u_char *buf = NULL;
  bcm_pkt_t *pkt = NULL;
  struct hsl_if *ifpc = NULL;
  struct hsl_if_list *nodel2;
  struct hsl_bcm_if *bcmifp = NULL;
  int ret = 0;
  int brid, vid;
  int offset;
  bcmx_l2_addr_t l2addr;
  int tagged = -1;
  
  HSL_FN_ENTER ();

  /* Allocate pkt info structure. */
  pkt = hsl_bcm_tx_pkt_alloc (1);
  if (! pkt)
    {
      ret = ENOMEM;
      goto ERR;
    }

  /* Set packet flags. */
  pkt->flags |= BCM_TX_CRC_APPEND;

  /* Set COS to highest. */
  pkt->cos = 1;

  /* Allocate buffer. */
  buf = kmalloc (HSL_ETH_PKT_SIZE, GFP_ATOMIC);
  if (! buf)
    {
      ret = ENOMEM;
      goto ERR;
    }

  /* Copy entire packet. */      
  memcpy (buf + ENET_TAG_SIZE, skb->data, skb->len);

  /* Correct length. */
  len = (skb->len - ENET_UNTAGGED_HDR_LEN) + ENET_TAGGED_HDR_LEN;

  /* Set data in pkt. */
  pkt->pkt_data[0].data = buf;
  pkt->pkt_data[0].len = pkt->pkt_len = len;

  /* Set callbacks for deferred xmit. */
  pkt->call_back = NULL;
  pkt->cookie   = pkt;
  pkt->cookie2  = buf;

  /* Get vid. */
  sscanf (ifp->name, "vlan%d.%d", &brid, &vid);
      
  /* Fill header. */
  offset = ENET_TAG_SIZE;
  BCM_PKT_HDR_DMAC_SET (pkt, buf + offset);
  offset += 6;
  BCM_PKT_HDR_SMAC_SET (pkt, buf + offset);
  BCM_PKT_HDR_TPID_SET (pkt, 0x8100);
  BCM_PKT_HDR_VTAG_CONTROL_SET (pkt, VLAN_CTRL (0, 0, vid));
  
  if ((pkt->pkt_data[0].data[0] & 0x1) == 0)
    {
      /* Unicast. */
      if (bcmx_l2_addr_get((void *) BCM_PKT_DMAC (pkt), vid, &l2addr, NULL) == 0)
        {
           if(BCMX_LPORT_INVALID == l2addr.lport)
             ifpc = hsl_bcm_ifmap_if_get (HSL_BCM_TRUNK_2_LPORT(l2addr.tgid));
          else
             ifpc = hsl_bcm_ifmap_if_get (l2addr.lport);

          if (ifpc)
            {
              bcmifp = (struct hsl_bcm_if *)ifpc->system_info;
              if (!bcmifp)
                goto ERR;
              
              if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK)
                {
                  nodel2 = ifpc->children_list;
                  if (!nodel2)
                    goto ERR;
                   
                  ifpc = nodel2->ifp;
                  if (!ifpc)
                    goto ERR;
                }
                           
              bcmifp = (struct hsl_bcm_if *)ifpc->system_info;
              if (!bcmifp)
                goto ERR;
             
#ifdef HAVE_L2 
              tagged = hsl_vlan_get_egress_type (ifpc, vid);
              if (tagged < 0)
                goto ERR;
#else /* HAVE_L2 */
              tagged = 0;
#endif /* HAVE_L2 */

              ret = hsl_bcm_pkt_send (pkt, bcmifp->u.l2.lport, tagged);

            }
        }
      else
        {
           HSL_LOG (HSL_LOG_DEVDRV, HSL_LEVEL_ERROR, "Cannot find destination MAC while sending packet\n");
           goto ERR;
        }
    }
  else
    {
      /* Broadcast, multicast flooded. */
       ret = hsl_bcm_pkt_vlan_flood (pkt, vid, ifp);
     }
   if (ret < 0)
     {
       HSL_LOG (HSL_LOG_DEVDRV, HSL_LEVEL_ERROR, "Failure sending packet\n");
       goto ERR;
     }

 ERR:
  if (buf)
    kfree (buf);
  if (pkt)
    hsl_bcm_tx_pkt_free (pkt);

  HSL_FN_EXIT (0); 
} 

/* Tx a skb out on a interface. */
static int
_hsl_eth_drv_tx (struct sk_buff *skb)
{
  struct net_device * dev;
  struct hsl_if *ifp;

  HSL_FN_ENTER();

  dev = skb->dev;
  if (! dev)
    HSL_FN_EXIT (-1);

  ifp = dev->ml_priv;

  if (! ifp)
    {
      /* Free skb. */
      kfree_skb (skb);
      HSL_FN_EXIT (0);
    }

  HSL_IFMGR_IF_REF_INC (ifp);

  /* Check for a SVI or pure L3 port transmit. */
  if (memcmp (ifp->name, "vlan", 4) == 0)
    _hsl_eth_drv_svi_tx (ifp, skb);
  else
    _hsl_eth_drv_l3_tx (ifp, skb);

  /* Free skb. */
  kfree_skb (skb);

  HSL_IFMGR_IF_REF_DEC (ifp);

  return 0;
}

#ifdef HSL_ETH_DRV_TX_THREAD
/* Tx thread. */
static void
_hsl_eth_tx_handler (void *notused)
{
  struct net_device *dev = NULL;
  struct hsl_if *ifp = NULL;
  struct sk_buff *skb;

  while (hsl_eth_tx_thread_running)
    {
      while ((skb = skb_dequeue (&hsl_eth_tx_queue)) != NULL)
        {

          dev = skb->dev;

          if (dev != NULL)
            ifp = dev->ml_priv;
          
          _hsl_eth_drv_tx (skb);

          if (ifp != NULL)
            HSL_IFMGR_IF_REF_DEC (ifp);

        }
      oss_sem_lock (OSS_SEM_BINARY, hsl_eth_tx_sem, OSS_WAIT_FOREVER);
    }

  hsl_eth_drv_tx_thread = NULL;

  /* Cancel Tx semaphore. */
  if (hsl_eth_tx_sem)
    {
      oss_sem_delete (OSS_SEM_BINARY, hsl_eth_tx_sem);
      hsl_eth_tx_sem = NULL;
    }

  sal_thread_exit (0);

}
#endif /* HSL_ETH_DRV_TX_THREAD */

/* Function to post L3 message. */
int
hsl_eth_drv_post_l3_pkt (struct hsl_if *ifpl3, bcm_pkt_t *pkt)
{
  int len;
  struct hsl_eth_header *eth;
  u_char *p;
  struct net_device *dev;
  struct sk_buff *skb = NULL;

  HSL_FN_ENTER ();

  dev = ifpl3->os_info;
  if (! dev)
    HSL_FN_EXIT (-1);

  len = pkt->pkt_len;

  p = BCM_PKT_DMAC (pkt);
  eth = (struct hsl_eth_header *) p;

  if (!(pkt->flags & BCM_RX_CRC_STRIP))
    {
      /* Substract CRC from length. */
      len -= 4;
    }

  /* Allocate skb. */
  skb = dev_alloc_skb (len + 2);
  if (! skb)
    goto DROP;

  skb->dev = dev;
  skb_reserve (skb, 2); /* 16 byte align the IP fields. */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27))
  skb_copy_to_linear_data (skb, pkt->_pkt_data.data, len); /* Copy buffer. */
#else
  eth_copy_and_sum (skb, pkt->_pkt_data.data, len, 0); /* Copy buffer. */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
  skb_put (skb, len);

  /* Untag MBLK. */
  _hsl_eth_untag_skb (skb);

  skb->protocol = eth_type_trans (skb, dev);

  /* Pass it up. */
  netif_rx (skb);
  dev->last_rx = jiffies;

  HSL_FN_EXIT (0);

 DROP:
  if (skb)
    kfree_skb (skb);

  HSL_FN_EXIT (-1);
}

/* Transmit a packet. */
int
hsl_eth_dev_xmit (struct sk_buff * skb, struct net_device * dev)
{

  struct hsl_if *ifp;

  HSL_FN_ENTER();

  skb->dev = dev;

  ifp = dev->ml_priv;

  if (ifp == NULL
     || (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_L3_DELETE)))
    {
      kfree_skb (skb);
      HSL_FN_EXIT (0); 
    }

  HSL_IFMGR_IF_REF_INC (ifp);

#ifdef HSL_ETH_DRV_TX_THREAD

  /* Post to tail. */
  skb_queue_tail (&hsl_eth_tx_queue, skb);

  /* Release semaphore. */
  oss_sem_unlock (OSS_SEM_BINARY, hsl_eth_tx_sem);
#else
  /* Stop queue. */
  netif_stop_queue (dev);

  /* Xmit pkt. */
  _hsl_eth_drv_tx (skb);

  /* Wakeup queue. */
  netif_wake_queue (dev);

  HSL_IFMGR_IF_REF_DEC (ifp);
#endif /* ! HSL_ETH_DRV_TX_THREAD */

  HSL_FN_EXIT (0);
}

/* Open. */
int
hsl_eth_dev_open (struct net_device * dev)
{
  /* Start queue. */
  netif_start_queue (dev);

  return 0;
}

/* Close. */
int
hsl_eth_dev_close (struct net_device * dev)
{
  /* Stop queue. */
  netif_stop_queue (dev);

  return 0;
}

/* Get interface statistics. */
struct net_device_stats *
hsl_eth_dev_get_stats(struct net_device *dev)
{
  struct hsl_if *ifp;

  /* Initialization */
  ifp = NULL;

  if (dev)
    ifp = dev->ml_priv;
  

  /* reset the counters before updating it*/
  memset (&net_stats, 0x0, sizeof (struct net_device_stats ));

  /* update the stats from HW */
  if (ifp) 
    {
      
      net_stats.rx_packets =  ifp->mac_cntrs.good_pkts_rcv.l[0];
      net_stats.tx_packets =  ifp->mac_cntrs.good_pkts_sent.l[0];

      net_stats.rx_bytes   =  ifp->mac_cntrs.good_octets_rcv.l[0] ;
      net_stats.tx_bytes   =  ifp->mac_cntrs.good_octets_sent.l[0];

      net_stats.rx_dropped  =  ifp->mac_cntrs.in_discards.l[0];
      net_stats.tx_dropped  =  ifp->mac_cntrs.out_discards.l[0];

      net_stats.rx_errors  =  ifp->mac_cntrs.bad_pkts_rcv.l[0];
      net_stats.tx_errors  =  ifp->mac_cntrs.out_errors.l[0];

      net_stats.multicast  =  ifp->mac_cntrs.mc_pkts_rcv.l[0];
      net_stats.collisions =  ifp->mac_cntrs.collisions.l[0];

      net_stats.tx_aborted_errors =  ifp->mac_cntrs.out_discards.l[0];

      net_stats.rx_crc_errors  =  ifp->mac_cntrs.bad_crc.l[0];
      net_stats.rx_frame_errors = ifp->mac_cntrs.bad_crc.l[0];
   }

 return &net_stats;
}

/* Set MC list. */
void
hsl_eth_dev_set_mc_list (struct net_device *dev)
{
  return;
}

/* Set multicast address. */
int
hsl_eth_dev_set_mac_addr (struct net_device *dev, void *p)
{
  struct sockaddr *addr = (struct sockaddr *) p;

  if (netif_running (dev))
    return -EBUSY;

  /* Set address. */
  memcpy (dev->dev_addr, addr->sa_data, dev->addr_len);

  return 0;
}

static const struct net_device_ops hsl_eth_dev_ops = {
	.ndo_open		= hsl_eth_dev_open,
	.ndo_stop		= hsl_eth_dev_close,
	.ndo_tx_timeout		= NULL,
	.ndo_get_stats		= hsl_eth_dev_get_stats,
	.ndo_start_xmit		= hsl_eth_dev_xmit,
	.ndo_set_multicast_list	= hsl_eth_dev_set_mc_list,
	.ndo_validate_addr	= NULL,
	.ndo_set_mac_address	= hsl_eth_dev_set_mac_addr,
	.ndo_change_mtu		= NULL,
	.ndo_vlan_rx_register	= NULL,
};

/* Setup netdevice functions/parameters. */
static void
hsl_eth_dev_setup (struct net_device *dev)
{
  dev->netdev_ops = &hsl_eth_dev_ops;

#ifndef LINUX_KERNEL_2_6
  dev->features           |= NETIF_F_DYNALLOC;
#endif

  memset (dev->dev_addr, 0, ETH_ALEN);

  dev->tx_queue_len = 0;

  return;
}

/* Function to create a L3 netdevice. */
struct net_device *
hsl_eth_drv_create_netdevice (struct hsl_if *ifp,  u_char *hwaddr, int hwaddrlen)
{
  struct net_device *dev;
  struct sockaddr addr;
  int ret, _dev_put = 0;
  char ifname[IFNAMSIZ+1];

  memset(ifname, 0, IFNAMSIZ);
  snprintf (ifname, IFNAMSIZ, "%s", ifp->name); 

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  dev = dev_get_by_name (current->nsproxy->net_ns, ifname);
#else
  dev = dev_get_by_name (ifname);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

  if (!dev)
    {
#ifdef LINUX_KERNEL_2_6
      dev = alloc_netdev(sizeof(ifp), ifname, ether_setup);
#else
      dev = dev_alloc(ifname, &ret);
#endif
      if (dev == NULL)
        {
          return NULL;
        }
    }
  else
    _dev_put = 1;

  /* Ethernet type setup */
#ifndef LINUX_KERNEL_2_6
  ether_setup (dev);
#endif
  
  hsl_eth_dev_setup (dev);
  
  /* Set ifp. */
  dev->ml_priv = ifp;

  /* Set flags. */
  ifp->flags = IFF_UP | IFF_BROADCAST | IFF_MULTICAST;
  
  /* Set mac. */
  if (hwaddrlen <= 14)
    {
      memset (&addr, 0, sizeof (struct sockaddr));
      memcpy (addr.sa_data, hwaddr, hwaddrlen);
      
      hsl_eth_dev_set_mac_addr (dev, (void *) &addr);
    }
  
  if (_dev_put)
    dev_put (dev);
  else
    {
      /* Register netdevice. */
      ret = register_netdev (dev);
      if (ret)
        {
          return NULL;
        }
    }

  return dev;
}

/* Destroy netdevice. */
int
hsl_eth_drv_destroy_netdevice (struct net_device *dev)
{
  /* Unregister netdevice. */
  unregister_netdev (dev);

  return 0;
}
