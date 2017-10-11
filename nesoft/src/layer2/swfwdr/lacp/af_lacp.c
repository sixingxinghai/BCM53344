/* Copyright (C) 2003  All Rights Reserved. 
 
LACP - An implementation of the LACP sublayer for Linux (See 802.1x) 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version
2 of the License, or (at your option) any later version.

*/
 
#include <linux/autoconf.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/kmod.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/ioctls.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/inetdevice.h>
#include <asm/uaccess.h>
#include <net/net_namespace.h>
#include <linux/nsproxy.h>

#include "bdebug.h"
#include "l2_forwarder.h"
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_input.h"
#include "br_api.h"
#include "br_vlan.h"
#include "if_lacp.h"

/* System max ports 32, max no of aggregators is MAX_PORTS/2
 * Max Links per aggregator is 6 
 */
#define LACP_MAX_PORTS        32
#define LACP_MAX_AGG_LINKS    6
#define LACP_MAX_AGGREGATORS  LACP_MAX_PORTS/2

struct aggregator
{
  spinlock_t    lock;
  unsigned int  num_links;
  struct net_device * dev;
  struct net_device * link[LACP_MAX_AGG_LINKS];
};

struct agg_private
{
  struct net_device_stats enet_stats;
  struct aggregator       *agg;
};

static struct aggregator agg[LACP_MAX_AGGREGATORS];

static struct proto _proto = {
        .name     = "LACP",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};

/* Aggregator device functions */

static struct net_device_stats *agg_get_stats(struct net_device *dev)
{
  struct agg_private *ap = (struct agg_private *)dev->priv;
  struct net_device_stats *r = &ap->enet_stats;

  return r;
}

static int
mac_hash(unsigned char * mac, int k)
{
  unsigned long hash;
  struct ethhdr * eh;

  eh = (struct ethhdr *)mac;
  hash = *(unsigned long *)(&eh->h_dest[ETH_ALEN-4]) ^
    *(unsigned long *)(&eh->h_source[ETH_ALEN-4]);
  hash ^= hash >> 11;
  return hash % k;
}

static __inline__ void
link_link (int agg_ndx, int link_ndx, struct net_device * dev)
{
  agg[agg_ndx].link[link_ndx] = dev;
  dev->agg_dev = agg[agg_ndx].dev;
  /* All packets are processed on a bridge */
  dev_set_promiscuity (dev, 1);
  agg[agg_ndx].num_links++;
}

static __inline__ void
unlink_link (int agg_ndx, int link_ndx)
{
  register unsigned int k;

  //agg[agg_ndx].link[link_ndx]->priv = 0;
  dev_set_promiscuity (agg[agg_ndx].link[link_ndx], -1);

  agg[agg_ndx].link[link_ndx]->agg_dev = NULL;
  agg[agg_ndx].link[link_ndx] = NULL;
  agg[agg_ndx].num_links--;
  /* Alway pack the links down so we can hash appropriately */
  for (k = link_ndx; k < (LACP_MAX_AGG_LINKS - 1); k++)
    {
      agg[agg_ndx].link[k] = agg[agg_ndx].link[k+1];
    }
  agg[agg_ndx].link[LACP_MAX_AGG_LINKS - 1] = NULL;
}

void lacp_handle_statistics (struct sk_buff * skb)
{
  struct agg_private *ap = NULL;
  struct net_device *agg_dev = NULL;

  if ( ((agg_dev = skb->dev)) && ((ap = agg_dev->priv)) )
    ap->enet_stats.rx_packets++;
}

static int
frame_distribution (struct sk_buff * skb, struct net_device * dev)
{
  struct net_device *tgt_dev = NULL;
  unsigned char * mac;
  struct aggregator * agg = NULL;
  struct sk_buff *new_skb;
  struct agg_private *ap;

  ap = dev->priv;
  if (ap != NULL)
    agg = ap->agg;

  if (agg == NULL)
    return 0;

  new_skb = skb_clone(skb, GFP_ATOMIC);
  kfree_skb(skb);

  if (!new_skb)
    {
      return 0;
    }


  switch (agg->num_links)
    {
    case 0:
      /* There are no ports for distributing this skb. */
      goto freeandout;
    case 1:
      tgt_dev = agg->link[0];
      break;
    default:
      mac = new_skb->data;
      tgt_dev = agg->link[mac_hash(mac, agg->num_links)];
      break;
    }


  if (!tgt_dev || !(tgt_dev->flags & IFF_UP))
    goto freeandout;

  new_skb->dev = tgt_dev;
  ap->enet_stats.tx_packets++;
  dev_queue_xmit(new_skb);
  return 0;

 freeandout:
  kfree_skb(new_skb);
  return 0;
}

static int
agg_open (struct net_device * dev)
{
  netif_start_queue (dev);
  return 0;
}

static int
agg_close (struct net_device * dev)
{
  netif_stop_queue (dev);
  return 0;
}

static void
agg_dev_destructor (struct net_device *dev)
{
  struct aggregator *agg;
  
  if (! dev->priv)
    return;

  agg = ((struct agg_private *)dev->priv)->agg;
  if (agg)
    agg->dev = NULL;

  kfree (dev->priv);
  dev->priv = NULL;

  return;
}


/* Aggregator management */

static int
add_aggregator (char *aname, unsigned char *agg_mac)
{
  int i = -1;
  struct agg_private *ap;

  for (i = 0; i < LACP_MAX_AGGREGATORS; i++)
    {
      if (agg[i].dev == NULL)
        break;
    }

  if (i >= LACP_MAX_AGGREGATORS || i < 0)
    {
      BWARN ("add_aggregator: Maximum aggregators already allocated"); 
      return -ENFILE;
    }

  agg[i].dev = alloc_netdev (0, aname, ether_setup);

  if (!agg[i].dev)
    {
      BWARN ("add_aggregator: unable to allocate aggregator"); 
      return -ENOMEM;
    }
  
  agg[i].dev->priv = 
    (struct agg_private *)kmalloc(sizeof(struct agg_private), GFP_KERNEL);
  ap = agg[i].dev->priv;
  memset (ap, 0, sizeof (struct agg_private));
  ap->agg = &agg[i];

  agg[i].dev->hard_start_xmit   = frame_distribution;
  agg[i].dev->open              = agg_open;
  agg[i].dev->stop              = agg_close;
  agg[i].dev->get_stats         = agg_get_stats; 
  agg[i].dev->destructor        = agg_dev_destructor;
  memcpy (agg[i].dev->dev_addr, agg_mac, ETH_ALEN);
  agg[i].dev->tx_queue_len = 0;
  agg[i].lock = SPIN_LOCK_UNLOCKED;
  agg[i].dev->validate_addr = NULL;

  register_netdev (agg[i].dev); 

  return 0;
}

static int
del_aggregator (char * aname)
{
  int i;
  int k;
  for (i = 0; i < LACP_MAX_AGGREGATORS; i++)
    if (agg[i].dev && strcmp(agg[i].dev->name, aname) == 0)
      break;

  if (i >= LACP_MAX_AGGREGATORS)
    {
      BWARN ("del_aggregator: Unknown aggregator %s", aname);
      return -ENXIO;
    }

  agg_close (agg[i].dev);
  
  for (k = 0; k < LACP_MAX_AGG_LINKS; k++)
    {
      if (agg[i].link[k]) {
        printk(KERN_INFO "unlinking agg[%d].link[%d]\n",i,k);
        unlink_link (i, k);
      }
    }

  printk("agg[%d].dev = %p\n", i, agg[i].dev);
  if (agg[i].dev) {     
    unregister_netdev (agg[i].dev);
    agg[i].dev = NULL;
  }

  return 0;
}
  
static int
set_mac_address (char *aname, unsigned char *agg_mac)
{
  int i;
  int slot = -1;

  for (i = 0; i < LACP_MAX_AGGREGATORS; i++) {
    if (agg[i].dev == NULL) {
      if (slot == -1)
        slot = i;

      continue;
    }
      
    if (strcmp (agg[i].dev->name, aname) == 0) {
      /* check for change in mac address */
      if (memcmp (agg[i].dev->dev_addr, agg_mac, ETH_ALEN) != 0) {
        rtnl_lock();
        if (agg[i].dev->set_mac_address) {
          int ret;
          ret = agg[i].dev->set_mac_address(agg[i].dev, agg_mac);
          if (ret == 0)
            netdev_state_change (agg[i].dev);
        }
        rtnl_unlock();
      }

      return 0;
    }
  }
  
  return 0;
}


/* Link management */
static int
add_link (char * aname, char * lname)
{
  int i;
  int k;

  struct net_device * dev = dev_get_by_name (current->nsproxy->net_ns, lname);

  if (dev == NULL)
    {
      BWARN ("add_link: No such link %s", lname);
      return -ENODEV;
    }

  for (i = 0; i < LACP_MAX_AGGREGATORS; i++)
    if (agg[i].dev && strcmp(agg[i].dev->name, aname) == 0)
      break;

  if (i >= LACP_MAX_AGGREGATORS)
    {
      BWARN ("add_link: No such aggregator %s", aname);
      dev_put (dev);
      return -ENXIO;
    }

  for (k = 0; k < LACP_MAX_AGG_LINKS; k++)
    {
      if (agg[i].link[k] == 0)
        {
          link_link (i, k, dev);
          dev_put (dev);
          return 0;
        }
    }
  BWARN ("add_link: Aggregator link limit exceeded %s", aname);
  dev_put (dev);
  return -EMFILE;
}

static int
del_link (char * aname, char * lname)
{
  int i;
  int k;

  struct net_device * dev = dev_get_by_name (current->nsproxy->net_ns, lname);

  if (dev == NULL)
    {
      BWARN ("del_link: No such link %s", lname);
      return -ENOENT;
    }

  for (i = 0; i < LACP_MAX_AGGREGATORS; i++)
    if (agg[i].dev && strcmp(agg[i].dev->name, aname) == 0)
      break;

  if (i >= LACP_MAX_AGGREGATORS)
    {
      BWARN ("del_link: No such aggregator %s", aname);
      dev_put (dev);
      return -ENXIO;
    }
  
  for (k = 0; k < LACP_MAX_AGG_LINKS; k++)
    {
      printk("agg[%d].link[%d] = %p, dev = %p\n", i,k,agg[i].link[k], dev);
      if (agg[i].link[k] == dev)
        {
          unlink_link (i, k);
          //dev_put (dev);
          return 0;
        }
    }
  BWARN ("del_link: Link not found %s", aname);
  dev_put (dev);
  return -ENOENT;
}

/* List of all LACP sockets. */
static struct hlist_head lacp_sklist;
static rwlock_t lacp_sklist_lock = RW_LOCK_UNLOCKED;

static struct proto_ops lacp_ops;

/* The number of open sockets */
static atomic_t lacp_socks_nr;

int lacp_create (struct net *net, struct socket *sock, int protocol);

/* Put a DIX header into a frame - only if the device doesn't have a
   way to do it. */

int 
lacp_header (struct sk_buff *skb, struct net_device *dev,
             void * daddr, void * saddr, unsigned len)
{
  struct ethhdr *eth = (struct ethhdr *) skb_push (skb, ETH_HLEN);

  eth->h_proto = ETH_P_LACP;

  /*
   *    Set the source hardware address. 
   */
         
  if (saddr)
    memcpy (eth->h_source, saddr, dev->addr_len);
  else
    memcpy (eth->h_source, dev->dev_addr, dev->addr_len);

  /*
   *    Anyway, the loopback-device should never use this function... 
   */

  if (dev->flags & (IFF_LOOPBACK|IFF_NOARP)) 
    {
      memset (eth->h_dest, 0, dev->addr_len);
      return dev->hard_header_len;
    }
        
  if (daddr)
    {
      memcpy (eth->h_dest, daddr, dev->addr_len);
      return dev->hard_header_len;
    }
        
  return -dev->hard_header_len;
}

static void
skb_dump (struct sk_buff *skb)
{
  unsigned char * i;

  printk (KERN_INFO "skb_dump (%p): from %s with len(%d) truesize(%d) headroom(%d) tailroom(%d)\n",
          skb, skb->dev ? skb->dev->name : "ip stack", skb->len,
          skb->truesize, skb_headroom (skb), skb_tailroom (skb));

#ifdef CONFIG_NET_SCHED
  printk ("nfmark %lu tcindex %d priority %d  \n", skb->mark,
          skb->tc_index, skb->priority);
#endif
  for (i = skb->head; i <= skb->tail; i++) 
    {
      if (((i - skb->head) & 0x7f) == 0)
        printk ("\n");
      printk ("%02x", *i);
    }
  printk ("\n\n");
}

void 
lacp_sock_destruct (struct sock *sk)
{
  BDEBUG ("LACP socket %p free - %d are left alive\n", sk, 
          atomic_read (&lacp_socks_nr));
}

/*
  This the "bottom half" of the lacp packet handling code.
  We do understand shared skbs. We do register a packet handler
  that is called here for the ETH_P_LACP protocol type.  */

int 
lacp_rcv (struct sk_buff * skb, struct net_device * dev, 
          struct packet_type * pt, struct net_device *orig_dev)
{
  struct sock * sk;
  int skb_len = skb->len;
  struct sockaddr_l2 * lacp;
  u8 * skb_head = skb->data;
  struct apn_bridge_port *port;

  BDEBUG ("port %s len %d\n", dev->name, skb->len);

  /* LACP packets are never tagged. */
  if (htons(pt->type) != ETH_P_LACP)
    {
      BDEBUG ("packet failed lacp type test 0x%x - drop\n", pt->type);
      goto drop;
    }

  BR_READ_LOCK_BH (&lacp_sklist_lock);

#if defined(CONFIG_APNFWD) || defined(CONFIG_APNFWD_MODULE)
  port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if (port && port->port_type == CUST_EDGE_PORT)
    {
      if (port->proto_process.lacp_process == APNBR_L2_PROTO_DISCARD)
        goto drop_n_acct;
      else if (port->proto_process.lacp_process == APNBR_L2_PROTO_TUNNEL)
        {
          br_flood_forward (port->br, skb, VLAN_NULL_VID,
                            port->proto_process.lacp_tun_vid, 0);
          BR_READ_UNLOCK_BH (&lacp_sklist_lock);
          return 0;
        }
    }
#endif /* CONFIG_APNFWD || CONFIG_APNFWD_MODULE */


  /* We try to queue the data on an open socket with room for the data. */
  sk = hlist_empty (&lacp_sklist) ? NULL : __sk_head (&lacp_sklist);
  for (; sk; sk = sk_next (sk))
    {
      if (atomic_read (&sk->sk_rmem_alloc) + skb->truesize >= (unsigned)sk->sk_rcvbuf)
        continue;


      /* sk now points to a socket with room for the buffer */
      if (skb_shared (skb)) 
        {
          struct sk_buff *nskb = skb_clone (skb, GFP_ATOMIC);
          BDEBUG ("skb shared \n");
      
          if (nskb == NULL)
            goto drop_n_acct;

          if (skb_head != skb->data) 
            {
              skb->data = skb_head;
              skb->len = skb_len;
            }
          kfree_skb (skb);
          skb = nskb;
        }

      /* Fill out the lacp structure here */
      lacp = (struct sockaddr_l2*)&skb->cb[0];
      lacp->port = skb->dev->ifindex;
      memcpy (&lacp->dest_mac[0], skb->mac_header, ETH_ALEN);
      memcpy (&lacp->src_mac[0], skb->mac_header + ETH_ALEN, ETH_ALEN);
      skb_pull (skb, 2);

      /* Queue it on the socket */
      BDEBUG("after pull - skb_len %d \n", skb_len);

      skb_set_owner_r (skb, sk);
      skb->dev = NULL;
      spin_lock (&sk->sk_receive_queue.lock);
      __skb_queue_tail (&sk->sk_receive_queue, skb);
      spin_unlock (&sk->sk_receive_queue.lock);
      sk->sk_data_ready (sk, skb->len);

    }

  BR_READ_UNLOCK_BH (&lacp_sklist_lock);

  return 0;

 drop_n_acct:

  BDEBUG ("drop\n");
  if (skb_head != skb->data && skb_shared (skb)) 
    {
      skb->data = skb_head;
      skb->len = skb_len;
    }
 drop:

  BR_READ_UNLOCK_BH (&lacp_sklist_lock);

  kfree_skb (skb);

  return 0;
}

/* Packet handler registration entry. */

static struct packet_type lacp_packet_type = {
  type: __constant_htons (ETH_P_LACP),
  dev: NULL,
  func: lacp_rcv,
};

/* Send a message on an LACP socket */

static int 
lacp_sendmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, 
              size_t len)
{
  struct sock *sk = sock->sk;
  struct sockaddr_l2 *lacp = (struct sockaddr_l2 *)msg->msg_name;
  struct sk_buff *skb;
  struct net_device *dev;
  unsigned short proto;
  unsigned char *addr;
  int ifindex, err;

  /*
   *    Get and verify the address. 
   */
         
  BDEBUG ("sock %p msg %p len %d\n", sk, msg, len);
  err = -EINVAL;
  if (lacp == NULL) 
    {
      BWARN ("lacp address is null\n");
      goto out;
    }
  else 
    {
      if (msg->msg_namelen < sizeof (struct sockaddr_l2))
        goto out;

      ifindex   = lacp->port;
      addr      = lacp->dest_mac;
    }

  proto = sk->sk_protocol;

  dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);
  BDEBUG ("ifindex %d dev %p sock %p type %d proto %d\n", 
          ifindex, dev, sk, sock->type, proto);
  err = -ENXIO;
  if (dev == NULL)
    goto out_unlock;

  err = -EMSGSIZE;
  if (len > (dev->mtu + dev->hard_header_len))
    goto out_unlock;

  skb = sock_alloc_send_skb (sk, len + dev->hard_header_len + 15, 
                             msg->msg_flags & MSG_DONTWAIT, &err);
  if (skb == NULL)
    goto out_unlock;

  /* Reserve at least 16 octets */
  skb_reserve (skb, (dev->hard_header_len + 15) & ~15);
  skb->network_header = skb->data;

  if (dev->header_ops) 
    {
      int res;
      err = -EINVAL;
      BDEBUG ("dev->hard_header\n");
      res = dev->header_ops->create (skb, dev, ETH_P_LACP, addr, NULL, len);
      if (sock->type != SOCK_RAW)
        {
          skb->tail = skb->data;
          skb->len = 0;
        } 
      else if (res < 0)
        {
          goto out_free;
        }
    }
  else
    {
      /* Only ethernet encapsulation for now. */
      BDEBUG ("lacp_header\n");
      lacp_header (skb, dev, lacp->dest_mac, 
                   lacp->src_mac, len);
    }

  /* Returns -EFAULT on error */
  BDEBUG ("memcpy_fromiovec\n");
  err = memcpy_fromiovec (skb_put (skb, len), msg->msg_iov, len);
  if (err)
    goto out_free;

  skb->protocol = proto;
  skb->dev = dev;
#ifdef CONFIG_NET_SCHED
  skb->priority = sk->sk_priority;
#endif

  err = -ENETDOWN;
  BDEBUG ("dev->flags\n");
  if (!(dev->flags & IFF_UP))
    goto out_free;

  /* Now send it */

  BDEBUG ("dev_queue_xmit called\n");
  err = dev_queue_xmit (skb);
  if (err > 0 && (err = net_xmit_errno (err)) != 0)
    goto out_unlock;

  dev_put (dev);

  return  len;

 out_free:
  BDEBUG ("out_free: send failed\n");
  kfree_skb (skb);
 out_unlock:
  BDEBUG ("out_unlock: send failed\n");
  if (dev)
    dev_put (dev);
 out:
  return err;
}

/*
  Close an LACP socket. This is fairly simple. We immediately go
  to 'closed' state and remove our protocol entry in the device list.  */

static int 
lacp_release (struct socket *sock)
{
  struct sock *sk = sock->sk;

  if (!sk)
    return 0;

  BR_WRITE_LOCK_BH (&lacp_sklist_lock);
  sk_del_node_init (sk);
  BR_WRITE_UNLOCK_BH (&lacp_sklist_lock);

  if (atomic_dec_and_test (&lacp_socks_nr))
    {
      l2_mod_dec_use_count ();

      /* Unhook packet receive handler.  */
      dev_remove_pack (&lacp_packet_type);
    }

  /*
   *    Now the socket is dead. No more input will appear.
   */

  sock_orphan (sk);
  sock->sk = NULL;

  /* Purge queues */
  skb_queue_purge (&sk->sk_receive_queue);

  sock_put (sk);

  return 0;
}


/*
  Pull a packet from the socket's receive queue and hand it to the user.
  If necessary we block.  */

static int 
lacp_recvmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, 
              size_t len, int flags)
{
  struct sock *sk = sock->sk;
  struct sk_buff *skb;
  int copied, err;

  BDEBUG (" RECVD MSD sock %p msg %p len %d flags 0x%x\n", sk, msg, len, flags);
  err = -EINVAL;
  if (flags & ~(MSG_PEEK|MSG_DONTWAIT|MSG_TRUNC))
    return err;

  /*
   *    If the address length field is there to be filled in, we fill
   *    it in now.
   */

  msg->msg_namelen = sizeof (struct sockaddr_l2);

  /*
   *    Call the generic datagram receiver. This handles all sorts
   *    of horrible races and re-entrancy so we can forget about it
   *    in the protocol layers.
   *
   *    Now it will return ENETDOWN, if device have just gone down,
   *    but then it will block.
   */

  skb = skb_recv_datagram (sk, flags, flags & MSG_DONTWAIT, &err);

  /*
   *    An error occurred so return it. Because skb_recv_datagram () 
   *    handles the blocking we don't see and worry about blocking
   *    retries.
   */

  if (skb == NULL)
    return err;

  /* Push the ethertype onto the head of the data buffer */
  skb_push (skb, 2);

  /*
   *    You lose any data beyond the buffer you gave. If it worries a
   *    user program they can ask the device for its MTU anyway.
   */

  copied = skb->len;
  if (copied > len)
    {
      copied = len;
      msg->msg_flags |= MSG_TRUNC;
    }

  err = skb_copy_datagram_iovec (skb, 0, msg->msg_iov, copied);
  if (err)
    {
      skb_free_datagram (sk, skb);
      return err;
    }

  sock_recv_timestamp (msg, sk, skb);

  if (msg->msg_name)
    memcpy (msg->msg_name, skb->cb, msg->msg_namelen);

  /*
   *    Free or return the buffer as appropriate. Again this
   *    hides all the races and re-entrancy issues from us.
   */
  err = (flags & MSG_TRUNC) ? skb->len : copied;

  skb_free_datagram (sk, skb);
  return err;
}

/* Handle underlying device up/down */

static int 
lacp_notifier (struct notifier_block *this, unsigned long msg, void *data)
{
  struct net_device * dev = (struct net_device *)data;
  if (dev == 0)
    return NOTIFY_DONE;

  BDEBUG ("msg %ld dev %s\n", msg, dev->name);
  switch (msg) 
    {
    case NETDEV_DOWN:
    case NETDEV_UNREGISTER:
      break;
    case NETDEV_UP:
      break;
    }
  return NOTIFY_DONE;
}

DECLARE_MUTEX(lacp_ioctl_mutex);

/* This function handles all ioctl commands. */

#define MAX_ARGS  3

int 
lacp_ioctl (struct socket *sock, unsigned int cmd, unsigned long arg)
{
  int err = 0;
  unsigned long arglist[MAX_ARGS];
  
  BDEBUG ("sock %p cmd %d arg 0x%lx\n", sock, cmd, arg);

  if (cmd != SIOCPROTOPRIVATE)
    /* return dev_ioctl (cmd,(void *)arg); */
    return -EINVAL;

  if (!capable (CAP_NET_ADMIN))
    return -EPERM;

  /* Command is arg 0 */
  if (copy_from_user (arglist, (void *)arg, MAX_ARGS * sizeof (unsigned long)))
    return -EFAULT;

  down (&lacp_ioctl_mutex);

  switch (arglist[0]) 
    {
    case APNLACP_GET_VERSION:
      BDEBUG("Get version\n");
      err = APN_LACP_VERSION;
      break;

    case APNLACP_ADD_AGG:
      BDEBUG("Add aggregator %s\n", (char *)arglist[1]);
      err = add_aggregator ((char *)arglist[1], (unsigned char *)arglist[2]);
      break;

    case APNLACP_DEL_AGG:
      BDEBUG("Delete aggregator %s\n", (char *)arglist[1]);
      err = del_aggregator ((char *)arglist[1]);
      break;

    case APNLACP_AGG_LINK:
      BDEBUG("Aggregate link %s - %s\n", (char *)arglist[1], 
             (char *)arglist[2]);
      err = add_link ((char *)arglist[1], (char *)arglist[2]);
      break;

    case APNLACP_DEAGG_LINK:
      BDEBUG("Disaggregate link %s - %s\n", (char *)arglist[1], 
             (char *)arglist[2]);
      err = del_link ((char *)arglist[1], (char *)arglist[2]);
      break;

    case APNLACP_SET_MACADDR:
      BDEBUG ("Set MAC Address %s - %s\n", (char *) arglist[1],
              (char *) arglist[2]);
      err = set_mac_address ((char *) arglist[1], (unsigned char *) arglist[2]);
      break;

    default:
      {
        BDEBUG("Unknown command - cmd %ld arg1(%ld) arg2(%ld)\n",
               arglist[0], arglist[1], arglist[2]);
        err = -EINVAL;
      }
      break;
    }
  up (&lacp_ioctl_mutex);
  return err;
}

/*
 *      Create a packet of type SOCK_LACP. 
 */

int 
lacp_create (struct net *net, struct socket *sock, int protocol)
{
  struct sock *sk;
  int err;

  BDEBUG ("protocol %d socket addr %p\n", protocol, sock);
  if (!capable (CAP_NET_RAW))
      return -EPERM;
  
  if (sock->type  != SOCK_RAW)
      return -ESOCKTNOSUPPORT;

  sock->state = SS_UNCONNECTED;

  err = -ENOBUFS;
  sk = sk_alloc (net, PF_LACP, GFP_KERNEL, &_proto);
  if (sk == NULL)
      return err;

  BDEBUG ("sock %p\n",sk);
  sock->ops = &lacp_ops;
  sock_init_data (sock,sk);

  sk->sk_family = PF_LACP;
  sk->sk_protocol = protocol;

  sk->sk_destruct = lacp_sock_destruct;

  /*
   *    Attach a protocol block
   */

  if (atomic_read (&lacp_socks_nr) == 0)
    {
      l2_mod_inc_use_count ();
      dev_add_pack (&lacp_packet_type);
    }
  atomic_inc (&lacp_socks_nr);

  BR_WRITE_LOCK_BH (&lacp_sklist_lock);
  sk_add_node (sk, &lacp_sklist);
  BR_WRITE_UNLOCK_BH (&lacp_sklist_lock);

  return 0;
}

/* We don't do much, but register for the things that we do. */

static struct proto_ops SOCKOPS_WRAPPED (lacp_ops) = {
  family:       PF_LACP,

  release:      lacp_release,
  bind:         sock_no_bind,
  connect:      sock_no_connect,
  socketpair:   sock_no_socketpair,
  accept:       sock_no_accept,
  getname:      sock_no_getname,
  poll:         datagram_poll,
  ioctl:        sock_no_ioctl,
  listen:       sock_no_listen,
  shutdown:     sock_no_shutdown,
  setsockopt:   sock_no_setsockopt,
  getsockopt:   sock_no_getsockopt,
  sendmsg:      lacp_sendmsg,
  recvmsg:      lacp_recvmsg,
  mmap:          sock_no_mmap,
  sendpage:     sock_no_sendpage,
};

static struct net_proto_family lacp_family_ops = 
  {
    family:     PF_LACP,
    create:     lacp_create,
    owner:      THIS_MODULE,
  };

static struct notifier_block lacp_netdev_notifier = 
  {
    notifier_call:      lacp_notifier,
  };

/* Handle module exit (called from auth_module_exit) */

void 
lacp_exit (void)
{
  BWARN ("NET4: Linux 802.1x LACP 7/19/2004 for Net4.0 removed\n");
  unregister_lacp_handle_frame_hook ();
  unregister_netdevice_notifier (&lacp_netdev_notifier);
  sock_unregister (PF_LACP);
  return;
}

/* Handle module init (called from auth_module_init) */

int  
lacp_init (void)
{
  BWARN ("NET4: Linux 802.1x LACP 7/19/2004 for Net4.0\n");
  sock_register (&lacp_family_ops);
  register_lacp_handle_frame_hook(lacp_handle_statistics);
  register_netdevice_notifier (&lacp_netdev_notifier);
  memset (agg, 0, sizeof(agg));
  return 0;
}
