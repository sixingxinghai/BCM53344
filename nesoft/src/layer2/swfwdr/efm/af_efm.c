/* Copyright (C) 2003  All Rights Reserved. 
 
   EFM - An implementation of the EFM sublayer for Linux (See 802.3 ah)

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
#include "if_efm.h"
#include "l2_forwarder.h"
#include "bdebug.h"
#include "efm.h"

/* List of all EFM sockets. */
static struct hlist_head efm_sklist;
static rwlock_t efm_sklist_lock = RW_LOCK_UNLOCKED;

static struct proto_ops efm_ops;

static struct proto _proto = {
        .name     = "8023ah",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};


/* The number of open sockets */
static atomic_t efm_socks_nr;

int efm_create (struct net *net, struct socket *sock, int protocol);

/* Put a DIX header into a frame - only if the device doesn't have a
   way to do it. */

int 
efm_header (struct sk_buff *skb, struct net_device *dev,
           void * daddr, void * saddr, unsigned len)
{
        struct ethhdr *eth = (struct ethhdr *) skb_push (skb, ETH_HLEN);

        eth->h_proto = ETH_P_EFM;

        /*
         *      Set the source hardware address. 
         */
         
        if (saddr)
                memcpy (eth->h_source, saddr, dev->addr_len);
        else
                memcpy (eth->h_source, dev->dev_addr, dev->addr_len);

        /*
         *      Anyway, the loopback-device should never use this function... 
         */

        if (dev->flags & (IFF_LOOPBACK|IFF_NOARP)) 
        {
                memset (eth->h_dest, '\0', dev->addr_len);
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
 printk (KERN_INFO "skb_dump: head(%p) tail(%p) data(%p) h.raw(%p) mac(%p)\n",
                 skb->head, skb->tail, skb->data, skb->transport_header, skb->mac_header);

#ifdef CONFIG_NET_SCHED
 printk ("mark %lu tcindex %d priority %d  \n", skb->mark,
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
efm_sock_destruct (struct sock *sk)
{
  BDEBUG ("EFM socket %p free - %d are left alive\n", sk, 
           atomic_read (&efm_socks_nr));
}

/*
  This the "bottom half" of the efm packet handling code.
  We do understand shared skbs. We do register a packet handler
  that is called here for the ETH_P_EFM protocol type.  */

int 
efm_rcv (struct sk_buff * skb, struct net_device * dev, 
         struct packet_type * pt, struct net_device *orig_dev)
{
  struct sock * sk;
  u8 * skb_head = skb->data;
  int skb_len = skb->len;
  struct sockaddr_l2 *efm;

  BDEBUG ("port %s len %d\n", dev->name, skb->len);

  /* EFM packets are never tagged. */
  if (htons(pt->type) != ETH_P_EFM)
    {
      BDEBUG ("packet failed EFM type test 0x%x - drop\n", pt->type);
      goto drop;
    }

  /* We try to queue the data on an open socket with room for the data. */
  sk = hlist_empty (&efm_sklist) ? NULL : __sk_head (&efm_sklist);
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

      /* Fill out the efm structure here */
      efm = (struct sockaddr_l2*)&skb->cb[0];
      efm->port = skb->dev->ifindex;
      memcpy (&efm->dest_mac[0], skb->mac_header, ETH_ALEN);
      memcpy (&efm->src_mac[0], skb->mac_header + ETH_ALEN, ETH_ALEN);

      /* Queue it on the socket */
      BDEBUG("after pull - skb_len %d \n", skb_len);
  
      /* skb_dump (skb); */
      skb_set_owner_r (skb, sk);
      skb->dev = NULL;
      spin_lock (&sk->sk_receive_queue.lock);
      __skb_queue_tail (&sk->sk_receive_queue, skb);
      spin_unlock (&sk->sk_receive_queue.lock);
      sk->sk_data_ready (sk, skb->len);
    }

 return 0;

 drop_n_acct:

  BDEBUG ("drop\n");
  if (skb_head != skb->data && skb_shared (skb))
    {
      skb->data = skb_head;
      skb->len = skb_len;
    }
 drop:
  kfree_skb (skb);
  return 0;
}

/* Packet handler registration entry. */

static struct packet_type efm_packet_type = {
  type: __constant_htons (ETH_P_EFM),
  dev: NULL,
  func: efm_rcv,
};

/* Send a message on an EFM socket */

static int 
efm_sendmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, 
               size_t len)
{
  struct sock *sk = sock->sk;
  struct sockaddr_l2 * efm= (struct sockaddr_l2 *)msg->msg_name;
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
  if (efm == NULL) 
    {
      BWARN ("efm address is null\n");
      goto out;
    }
  else 
    {
      if (msg->msg_namelen < sizeof (struct sockaddr_l2))
        goto out;

      ifindex   = efm->port;
      addr      = efm->dest_mac;
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

  skb_reserve (skb, (dev->hard_header_len + 15) & ~15);
  skb->mac_header = skb->network_header = skb->data;

  if ((dev->header_ops != NULL) && (dev->header_ops->create)) 
    {
      int res;
      err = -EINVAL;
            BDEBUG ("dev->hard_header\n");
      res = dev->header_ops->create (skb, dev, ETH_P_EFM, addr, NULL, len);
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
      BDEBUG ("efm_header\n");
      efm_header (skb, dev, efm->dest_mac, 
                  efm->src_mac, len);
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
  Close an EFM socket. This is fairly simple. We immediately go
        to 'closed' state and remove our protocol entry in the device list.  */

static int 
efm_release (struct socket *sock)
{
  struct sock *sk = sock->sk;
  struct sock **skp;
  BDEBUG ("sock %p\n", sk);

  if (!sk)
    return 0;

  BR_WRITE_LOCK_BH (&efm_sklist_lock);
  sk_del_node_init (sk);
  BR_WRITE_UNLOCK_BH (&efm_sklist_lock);

  if (atomic_dec_and_test (&efm_socks_nr))
  {
    efm_remove_all_ports ();
    /* Unhook packet receive handler.  */
    dev_remove_pack (&efm_packet_type);
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
efm_recvmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len, int flags)
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
efm_notifier (struct notifier_block *this, unsigned long msg, void *data)
{
  struct net_device * dev = (struct net_device *)data;
  struct efm_port * port = 0;
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

DECLARE_MUTEX(efm_ioctl_mutex);

/* This function handles all ioctl commands. */

#define MAX_ARGS  4

int 
efm_ioctl (struct socket *sock, unsigned int cmd, unsigned long arg)
{
  int err = 0;
  char port_name[IFNAMSIZ];
  unsigned long arglist[MAX_ARGS];
  BDEBUG ("sock %p cmd %d arg 0x%lx\n", sock, cmd, arg);

  if (cmd != SIOCPROTOPRIVATE)
    /* return dev_ioctl (current->nsproxy->net_ns, cmd,(void *)arg); */
    return -EINVAL;

  if (!capable (CAP_NET_ADMIN))
    return -EPERM;

  /* Command is arg 0 */
  if (copy_from_user (arglist, (void *)arg, MAX_ARGS * sizeof (unsigned long)))
    return -EFAULT;

  down (&efm_ioctl_mutex);
  BDEBUG("arg0(%ld) arg1(%ld) arg2(%ld)\n", arglist[0], arglist[1], arglist[2]);
  switch (arglist[0]) 
    {
    case APNEFM_GET_VERSION:
      BDEBUG("Get version\n");
      err = APN_EFM_VERSION;
      break;

    case APNEFM_ADD_PORT:
      BDEBUG("Add port %ld\n", arglist[1]);
      err = efm_add_port (arglist[1]);
      break;

    case APNEFM_DEL_PORT:
      BDEBUG("Delete port %ld\n", arglist[1]);
      err = efm_remove_port (arglist[1]);
      break;

    case APNEFM_SET_PORT_STATE:
      BDEBUG("Set port state %ld - %ld - %ld\n", arglist[1], arglist[2], arglist[3]);
      err = efm_set_port_state (arglist[1], arglist[2], arglist[3]);
      break;

    default:
      BDEBUG("Unknown command - cmd %ld arg1(%ld) arg2(%ld)\n",
         arglist[0], arglist[1], arglist[2]);
      break;
    }
  up (&efm_ioctl_mutex);
  return err;
}

/*
 *      Create a packet of type SOCK_EFM. 
 */

int 
efm_create (struct net *net, struct socket *sock, int protocol)
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
  sk = sk_alloc (net, PF_EFM, GFP_KERNEL, &_proto);
  if (sk == NULL)
    return err;

  BDEBUG ("sock %p\n",sk);
  sock->ops = &efm_ops;
  sock_init_data (sock,sk);

  sk->sk_family = PF_EFM;
  sk->sk_protocol = protocol;

  sk->sk_destruct = efm_sock_destruct;

  /*
   *    Attach a protocol block
   */

  if (atomic_read (&efm_socks_nr) == 0)
  {
    l2_mod_inc_use_count ();
    dev_add_pack (&efm_packet_type);
  }
  atomic_inc (&efm_socks_nr);

  BR_WRITE_LOCK_BH (&efm_sklist_lock);
  sk_add_node (sk, &efm_sklist);
  BR_WRITE_UNLOCK_BH (&efm_sklist_lock);

  return 0;
}

/* We don't do much, but register for the things that we do. */

static struct proto_ops SOCKOPS_WRAPPED (efm_ops) = {
  family:       PF_EFM,
  release:      efm_release,
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
  sendmsg:      efm_sendmsg,
  recvmsg:      efm_recvmsg,
  mmap:         sock_no_mmap,
  sendpage:     sock_no_sendpage,
};

static struct net_proto_family efm_family_ops = 
{
  family:       PF_EFM,
  create:       efm_create,
  owner:        THIS_MODULE,
};

static struct notifier_block efm_netdev_notifier = 
{
  notifier_call:        efm_notifier,
};

/* Handle module exit */
void 
efm_exit (void)
{
  BWARN ("NET4: Linux 802.3ah EFM 7/19/2004 for Net4.0 removed\n");
  unregister_netdevice_notifier (&efm_netdev_notifier);
  sock_unregister (PF_EFM);
  return;
}

/* Handle module init */
int  
efm_init (void)
{
  BWARN ("NET4: Linux 802.3ah EFM 7/19/2004 for Net4.0\n");
  sock_register (&efm_family_ops);
  register_netdevice_notifier (&efm_netdev_notifier);
  return 0;
}
