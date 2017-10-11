/* Copyright 2003  All Rights Reserved.  */

#include<linux/version.h>

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
#include "hal_l2.h"
#include "hal_msg.h"

/* HSL includes.*/
#include "hsl_types.h"
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_if_os.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_pkt.h"
#include "hsl_l2_sock.h"

#include "hal_netlink.h"
#include "hal_socket.h"
#include "hal_msg.h"
#include "hsl_vlan.h"

/* List of all HSL backend sockets. */
#ifdef LINUX_KERNEL_2_6
static struct hlist_head _cfm_socklist;
#else
static struct sock *_cfm_socklist = 0;
#endif /* LINUX_KERNEL_2_6 */

// qcl 20170808 static rwlock_t _cfm_socklist_lock = RW_LOCK_UNLOCKED;
static DEFINE_RWLOCK(_cfm_socklist_lock); 
/* Private packet socket structures. */

/* Forward declarations. */
static int _cfm_sock_release (struct socket *sock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
static int _cfm_sock_create (struct net *net, struct socket *sock,
                             int protocol);
#else
static int _cfm_sock_create (struct socket *sock, int protocol);
#endif

#ifdef LINUX_KERNEL_2_6
static int _cfm_sock_sendmsg (struct kiocb *iocb, struct socket *sock,
                              struct msghdr *msg, size_t len);
static int _cfm_sock_recvmsg (struct kiocb *iocb, struct socket *sock,
                              struct msghdr *msg, size_t len, int flags);
#else
static int _cfm_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len, 
                              struct scm_cookie *scm);
static int _cfm_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len, 
                              int flags, struct scm_cookie *scm);
#endif /* LINUX_KERNEL_2_6 */

//qcl 20170808 static struct proto_ops SOCKOPS_WRAPPED (cfm_ops) = {
static struct proto_ops cfm_ops = {
  .family =       AF_CFM,

  .release =      _cfm_sock_release,
  .bind =         sock_no_bind,
  .connect =      sock_no_connect,
  .socketpair =   sock_no_socketpair,
  .accept =       sock_no_accept,
  .getname =      sock_no_getname,
  .poll =         datagram_poll,
  .ioctl =        sock_no_ioctl,
  .listen =       sock_no_listen,
  .shutdown =     sock_no_shutdown,
  .setsockopt =   sock_no_setsockopt,
  .getsockopt =   sock_no_getsockopt,
  .sendmsg =      _cfm_sock_sendmsg,
  .recvmsg =      _cfm_sock_recvmsg,
  .mmap =         sock_no_mmap,
  .sendpage =     sock_no_sendpage,
};

static struct net_proto_family cfm_family_ops = {
  family:               AF_CFM,
  create:               _cfm_sock_create,
};


/* Destruct socket. */
static void 
_cfm_sock_destruct (struct sock *sk)
{
  if (!sk)
    return;

  HSL_SOCK_DESTRUCT(sk, _cfm);

}

/* Release socket. */
static int 
_cfm_sock_release (struct socket *sock)
{
  struct sock *sk = sock->sk;

  HSL_SOCK_DESTRUCT(sk, _cfm);
  sock->sk = NULL;

  return 0;
}

#ifdef LINUX_KERNEL_2_6
static struct proto _cfm_prot = {
        .name     = "HSL_CFM",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};
#endif /* LINUX_KERNEL_2_6 */

/* Create socket. */
static int 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
_cfm_sock_create (struct net *net, struct socket *sock, int protocol)
#else
_cfm_sock_create (struct socket *sock, int protocol)
#endif
{
  struct sock *sk;

  if (!capable (CAP_NET_RAW))
    return -EPERM;
  if (sock->type  != SOCK_RAW)
    return -ESOCKTNOSUPPORT;

  sock->state = SS_UNCONNECTED;
  sock->ops = &cfm_ops;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
  HSL_SOCK_ALLOC(net, sock, AF_CFM, protocol, _cfm);
#else
  HSL_SOCK_ALLOC(sock, AF_CFM, protocol, _cfm);
#endif
  return 0;
}

#ifdef LINUX_KERNEL_2_6
static int _cfm_sock_sendmsg (struct kiocb *iocb, struct socket *sock,
                              struct msghdr *msg, size_t len)
#else
/* Sendmsg. */
static int 
_cfm_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len,
                    struct scm_cookie *scm)
#endif /* LINUX_KERNEL_2_6 */
{
  bcm_pkt_t *pkt = NULL;
  int ret = 0;
  u_char *hdr = NULL;
  u_char *buf = NULL;
  struct sockaddr_vlan *s;
  struct hsl_if *ifp;
  struct hsl_bcm_if *sifp;
  bcmx_lport_t dport;
  struct hsl_eth_header *eth;
  int tagged = 0;
  u_int32_t priority = 0;
  u_int8_t drop_enable = 0;

  s = (struct sockaddr_vlan *)msg->msg_name;

  ifp = hsl_ifmgr_lookup_by_index (s->port);
  if (! ifp)
    {
      ret = -EINVAL;
      goto RET;
    }

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      sifp = ifp->system_info;
      if (! sifp)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          ret = -EINVAL;
          goto RET;
        }

      if (sifp->trunk_id >= 0)
        {
          /* Get first member port. */
          if (ifp->children_list
              && ifp->children_list->ifp)
            {
              sifp = ifp->children_list->ifp->system_info;
            }
          else
            {
              HSL_IFMGR_IF_REF_DEC (ifp);
              ret = -EINVAL;
              goto RET;
            }
        }

      dport = sifp->u.l2.lport;
    }
  else
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      ret = -EINVAL;
      goto RET; 
    }

  HSL_IFMGR_IF_REF_DEC (ifp);
  
  /* Allocate pkt info structure. One for the header and one for the body. */
  pkt = hsl_bcm_tx_pkt_alloc (2);
  if (! pkt)
    {
      ret = -ENOMEM;
      goto RET;
    }

  /* Set packet flags. */
  SET_FLAG (pkt->flags, BCM_TX_CRC_APPEND);

  /* Set COS to highest. */
  pkt->cos = 1;

  /* Allocate buffer for header. */
  hdr = kmalloc (ENET_TAGGED_HDR_LEN, GFP_KERNEL);
  if (! hdr)
    {
      ret = -ENOMEM;
      goto RET;
    }

  /* Set header. */
  pkt->pkt_data[0].data = hdr;
  pkt->pkt_data[0].len = ENET_TAGGED_HDR_LEN;

  /* Allocate buffer for body. */
  buf = kmalloc (len, GFP_KERNEL);
  if (! buf)
    {
      ret = -ENOMEM;
      goto RET;
    }

  /* Copy from iov's */
  ret = memcpy_fromiovec (buf, msg->msg_iov, len);
  if (ret < 0)
    {
      ret = -ENOMEM;
      goto RET;
    }

  /* Set body. */
  pkt->pkt_data[1].data = buf;
  pkt->pkt_data[1].len = len;

  /* Set ETH header. */
  eth = (struct hsl_eth_header *) hdr;
  memset (eth, 0, sizeof (struct hsl_eth_header));
  memcpy (eth->dmac, s->dest_mac, 6);
  memcpy (eth->smac, s->src_mac, 6);
  eth->d.vlan.tag_type = htons (0x8100);

  /* Extract the priority & drop elegibility from the vlan id
   * Assumption is vlan id consists of 16 bits.
   * only 12 bits are used for representing the same.
   * 3 MSB bits (last 3 bits) can be used for storing the priority.
   * 4 MSB bit can be used for storing the drop elegibility.
   * These parameters are supplied from CFM Mep.
   */
  priority = (s->vlanid >> 13 ) & 0xF;

  drop_enable = (s->vlanid >> 12) & 0x1;

  s->vlanid &=  0x0FFF;

  if (!s->vlanid)
    {
      tagged = 0;
      HSL_ETH_VLAN_SET_VID (eth->d.vlan.pri_cif_vid, 1);
    }
  else
    {
      tagged = hsl_vlan_get_egress_type (ifp, s->vlanid);
      HSL_ETH_VLAN_SET_VID (eth->d.vlan.pri_cif_vid, s->vlanid);
      eth->d.vlan.pri_cif_vid |= htons(priority << 13);
    }

  eth->d.vlan.type = htons (HSL_ETHER_TYPE_CFM);
  
  /* Send packet. */
  if (tagged)
    ret = hsl_bcm_pkt_send (pkt, dport, 1);
  else
  ret = hsl_bcm_pkt_send (pkt, dport, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending cfm frame out on dport(%d)\n", dport);
      ret = -EINVAL;
      goto RET;
    }

  ret = 0;

 RET:
  if (pkt)
    hsl_bcm_tx_pkt_free (pkt);
  if (hdr)
    kfree (hdr);
  if (buf)
    kfree (buf);

  return ret;
}

/* for event packet that need to be timestamped and for multicast and for sync*/
int 
ptp_sock_sendmsg_event ( char *buf, int len, int outport_bitmap)
{
  int ret = 0;
  struct 
  {
      struct hsl_eth_header eth; 
	  unsigned char msgObuf[50];//44
  }msg;
 
  /* Set ETH header. */
  //memset (&msg.eth, 0, sizeof (struct hsl_eth_header));
  u_char dest_mac[6]={0x01,0x1b,0x19,0x0,0x0,0x00};
  u_char src_mac[6]={0x00,0x11,0x80,0x00,0x52,0xac};
  memcpy (msg.eth.dmac, dest_mac, 6);
  memcpy (msg.eth.smac, src_mac, 6);
  msg.eth.d.vlan.tag_type = htons (0x8100);
  HSL_ETH_VLAN_SET_VID (msg.eth.d.vlan.pri_cif_vid, 1);
  msg.eth.d.vlan.type = htons (0x88f7);

  unsigned int flags = 0;
  int i;
  bcm_pbmp_t pbmp;
  BCM_PBMP_CLEAR(pbmp);
  
  for(i = 0; i <28; i ++)
  {
	  if ( (outport_bitmap & (0x01 <<i)) != 0 )
		BCM_PBMP_PORT_ADD(pbmp, i+2);
  }
  
  SET_FLAG (flags, BCM_TX_TIME_STAMP_REPORT);
  SET_FLAG (flags, BCM_PKT_F_TIMESYNC);
  
  for( i = 0; i <len; i ++ )
	 *(unsigned char *)(msg.msgObuf + i) = *(unsigned char *)( buf + i );
  
	ret = SendPkt(&msg, ENET_TAGGED_HDR_LEN + len, flags, pbmp,pbmp);
  if (ret < 0)
    {
      //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending EFM frame out on dport\n");
      ret = -EINVAL;
      goto RET;
    }

  ret = 0;

 RET:
  return ret;
}

/* djg Sendmsg. for event packet that need to be timestamped and for unicast*/
int 
ptp_sock_peer_sendmsg_event_pdelay ( char *buf, int len, int port)
{
  int ret = 0;
  struct 
  {
      struct hsl_eth_header eth; 
	  unsigned char msgObuf[60];//54
  }msg;
 
  /* Set ETH header. */
  //memset (&msg.eth, 0, sizeof (struct hsl_eth_header));
  u_char dest_mac[6]={0x01,0x80,0xc2,0x0,0x0,0x0e}; 
  u_char src_mac[6]={0xBC,0xAE,0xC5,0xE4,0x75,0x78};
  //u_char src_mac[6]={0x00,0x11,0x80,0x00,0x52,0xac};
  memcpy (msg.eth.dmac, dest_mac, 6);
  memcpy (msg.eth.smac, src_mac, 6);
  msg.eth.d.vlan.tag_type = htons (0x8100);
  HSL_ETH_VLAN_SET_VID (msg.eth.d.vlan.pri_cif_vid, 1);
  msg.eth.d.vlan.type = htons (0x88f7);

  unsigned int flags = 0;
  int i;
  bcm_pbmp_t pbmp;
  BCM_PBMP_CLEAR(pbmp);  
  BCM_PBMP_PORT_ADD(pbmp, port+2);
  
  SET_FLAG (flags, BCM_TX_TIME_STAMP_REPORT);
  SET_FLAG (flags, BCM_PKT_F_TIMESYNC);
  
  for( i = 0; i <len; i ++ )
	 *(unsigned char *)(msg.msgObuf + i) = *(unsigned char *)( buf + i );
  
  ret = SendPkt(&msg, ENET_TAGGED_HDR_LEN + len, flags, pbmp,pbmp);
  if (ret < 0)
    {
      //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending EFM frame out on dport\n");
      ret = -EINVAL;
      goto RET;
    }

  ret = 0;

 RET:
  return ret;
}

/* djg Sendmsg. for general packet that don't need to be timestamped and for multicast*/
int 
ptp_sock_sendmsg_general ( char *buf, int len, int outport_bitmap)
{
  int ret = 0;
  struct 
  {
      struct hsl_eth_header eth; 
	  unsigned char msgObuf[64];
  }msg;

  /* Set ETH header. */
  //memset (&msg.eth, 0, sizeof (struct hsl_eth_header));
  u_char dest_mac[6]={0x01,0x1b,0x19,0x0,0x0,0x00};
 // u_char src_mac[6]={0xBC,0xAE,0xC5,0xE4,0x75,0x78};
  u_char src_mac[6]={0x00,0x11,0x80,0x00,0x52,0xac};
  memcpy (msg.eth.dmac, dest_mac, 6);
  memcpy (msg.eth.smac, src_mac, 6);
  msg.eth.d.vlan.tag_type = htons (0x8100);
  HSL_ETH_VLAN_SET_VID (msg.eth.d.vlan.pri_cif_vid, 1);
  msg.eth.d.vlan.type = htons (0x88f7);
  
  unsigned int flags = 0;
  int i;
  bcm_pbmp_t pbmp;
  BCM_PBMP_CLEAR(pbmp);
  for(i = 0; i <28; i ++)
  {
	if ( (outport_bitmap & (0x01 <<i)) != 0 )
	  BCM_PBMP_PORT_ADD(pbmp, i+2);
  }
        
  for( i = 0; i <len; i ++ )
	*(msg.msgObuf + i) = *(unsigned char *)( buf + i );
  
  ret = SendPkt(&msg, len + ENET_TAGGED_HDR_LEN, flags, pbmp,pbmp);
  if (ret < 0)
    {
      //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending EFM frame out on dport\n");
      ret = -EINVAL;
      goto RET;
    }

  ret = 0;

 RET:
  return ret;
}

/* djg Sendmsg. for general packet that don't need to be timestamped and for unicast*/
int 
ptp_sock_peer_sendmsg_general_pdelay ( char *buf, int len, int port)
{
  int ret = 0;
  struct 
  {
      struct hsl_eth_header eth; 
	  unsigned char msgObuf[60];//54
  }msg;

  /* Set ETH header. */
  //memset (&msg.eth, 0, sizeof (struct hsl_eth_header));
  u_char dest_mac[6]={0x01,0x80,0xc2,0x0,0x0,0x0e};
  u_char src_mac[6]={0xBC,0xAE,0xC5,0xE4,0x75,0x78};
  //u_char src_mac[6]={0x00,0x11,0x80,0x00,0x52,0xac};
  memcpy (msg.eth.dmac, dest_mac, 6);
  memcpy (msg.eth.smac, src_mac, 6);
  msg.eth.d.vlan.tag_type = htons (0x8100);
  HSL_ETH_VLAN_SET_VID (msg.eth.d.vlan.pri_cif_vid, 1);
  msg.eth.d.vlan.type = htons (0x88f7);
  
  unsigned int flags = 0;
  int i;
  bcm_pbmp_t pbmp;
  BCM_PBMP_CLEAR(pbmp);
  BCM_PBMP_PORT_ADD(pbmp, port+2);
        
  for( i = 0; i <len; i ++ )
	*(msg.msgObuf + i) = *(unsigned char *)( buf + i );
  
  ret = SendPkt(&msg, len + ENET_TAGGED_HDR_LEN, flags, pbmp,pbmp);
  if (ret < 0)
    {
      //HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending EFM frame out on dport\n");
      ret = -EINVAL;
      goto RET;
    }

  ret = 0;

 RET:
  return ret;
}

/* Recvmsg. */

#ifdef LINUX_KERNEL_2_6
static int _cfm_sock_recvmsg (struct kiocb *iocb, struct socket *sock,
                              struct msghdr *msg, size_t len, int flags)
#else
static int 
_cfm_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len,
                    int flags, struct scm_cookie *scm)
#endif /* LINUX_KERNEL_2_6 */
{
  int size = sizeof (struct sockaddr_vlan);
  struct sk_buff *skb;
  struct sock *sk = sock->sk;
  int copied;
  int ret;

  if (! sk)
    return -EINVAL;

  /* Receive one msg from the queue. */
  skb = skb_recv_datagram (sk, flags, flags & MSG_DONTWAIT, &ret);
  if (! skb)
    return -EINVAL;

  /* Copy sockaddr_vlan. */
  if (msg->msg_name)
    memcpy (msg->msg_name, (struct sockaddr_vlan *) skb->data, size);
  msg->msg_namelen = size;

  /* Pull data of sockaddr_vlan from skb. */
  skb_pull (skb, size);

  /* Did user send lesser buffer? */
  copied = skb->len;
  if (copied > len)
    {
      copied = len;
      msg->msg_flags |= MSG_TRUNC;
    }

  /* Copy message. */
  ret = skb_copy_datagram_iovec (skb, 0, msg->msg_iov, copied);
  if (ret < 0)
    {
      skb_free_datagram (sk, skb);
      return ret;
    }

  sock_recv_timestamp (msg, sk, skb);
  
  /* Free. */
  skb_free_datagram (sk, skb);

  return copied;
}

#ifdef HAVE_ONMD
/* Post packet. */
int 
hsl_af_cfm_post_packet (struct sk_buff *skb)
{
  struct sock * sk; 
  struct sk_buff *skbc;

  /* Read lock. */
  read_lock_bh (&_cfm_socklist_lock);

  /* Is there room to schedule the packet */
  HSL_SOCK_LIST_LOOP(sk, _cfm)
    {
      /* Skip dead sockets. */
      if(HSL_CHECK_SOCK_DEAD(sk))
        {
          continue;
        }

      /* Check if there is space. */
      if(HSL_CHECK_SOCK_RECV_BUFF(sk, skb->truesize))
        {
          continue;
        }

      /* Clone the skb. */
      skbc = skb_clone (skb, GFP_ATOMIC);
      if (! skbc)
        {
          /* Continue with next. */
          continue;
        }

      /* Post this skb. */
      hsl_sock_post_skb (sk, skbc);
    }

  /* Read unlock. */
  read_unlock_bh (&_cfm_socklist_lock);

  return 0;
}
#endif /* HAVE_ONMD */

/* HSL socket initialization. */
int
hsl_af_cfm_sock_init (void)
{
  sock_register (&cfm_family_ops);
  return 0;
}

/* HSL socket deinitialization. */
int
hsl_af_cfm_sock_deinit (void)
{
  sock_unregister (AF_CFM);
  return 0;
}
