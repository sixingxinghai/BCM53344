/* Copyright (C) 2003  All Rights Reserved. 
 
   EAPOL - An implementation of the EAPOL sublayer for Linux (See 802.1x)

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

#include "l2_forwarder.h"
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_input.h"
#include "br_vlan.h"
/*MAC-based authentication Enhancement*/
#include "br_api.h"

#include "if_eapol.h"
#include "bdebug.h"
#include "auth.h"

/* List of all eapol sockets. */
static struct hlist_head eapol_sklist;
static rwlock_t eapol_sklist_lock = RW_LOCK_UNLOCKED;

static struct proto_ops eapol_ops;

static struct proto _proto = {
        .name     = "8021x",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};


/* The number of open sockets */
static atomic_t eapol_socks_nr;

int eapol_create (struct net *net, struct socket *sock, int protocol);

/* Put a DIX header into a frame - only if the device doesn't have a
   way to do it. */

int 
eapol_header (struct sk_buff *skb, struct net_device *dev,
           void * daddr, void * saddr, unsigned len)
{
        struct ethhdr *eth = (struct ethhdr *) skb_push (skb, ETH_HLEN);

        eth->h_proto = ETH_P_PAE;

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
eapol_sock_destruct (struct sock *sk)
{
  BDEBUG ("EAPOL socket %p free - %d are left alive\n", sk, 
                        atomic_read (&eapol_socks_nr));
}

/*
  This the "bottom half" of the eapol packet handling code.
  We do understand shared skbs. We do register a packet handler
  that is called here for the ETH_P_PAE protocol type.  */

int 
eapol_rcv (struct sk_buff * skb, struct net_device * dev, 
    struct packet_type * pt, struct net_device *orig_dev)
{
  u8 * skb_type;
  u16   pkt_type;
  struct sock * sk;
  br_frame_t frame_type;
  struct sk_buff *skb2;
  int skb_len;
  struct eapol_addr * eapol;
  u8 * skb_head;
  struct apn_bridge_port *port;
#ifdef HAVE_MAC_AUTH
  struct auth_port *auth_port = auth_find_port (dev->ifindex);
  struct apn_bridge_port *bridge_port;
  struct apn_bridge *bridge;
  unsigned int proto;
#endif

  skb2 = NULL;

  if (skb == NULL)
    return 0;

  skb_len = skb->len;
  skb_head = skb->data;

#if defined(CONFIG_APNFWD) || defined(CONFIG_APNFWD_MODULE)
  port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if (port && port->port_type == CUST_EDGE_PORT)
    {
      if (port->proto_process.dot1x_process == APNBR_L2_PROTO_DISCARD)
        goto drop;
      else if (port->proto_process.dot1x_process == APNBR_L2_PROTO_TUNNEL)
        {
          br_flood_forward (port->br, skb, VLAN_NULL_VID,
                            port->proto_process.dot1x_tun_vid, 0);
          return 0;
        }
    }
#endif /* CONFIG_APNFWD || CONFIG_APNFWD_MODULE */

  BDEBUG ("port %s len %d\n", dev->name, skb->len);

  skb2 = skb_copy (skb, GFP_ATOMIC);

  if (skb2 == NULL)
    goto drop;

  if (htons(skb2->protocol) != ETH_P_PAE)
    {
      if (htons(skb2->protocol) == 0x8100)      /* We hardwire this so as to not require
                                   that vlan be installed. */
        {
          skb_type = skb2->data + 2; /* Point to the real length/type */
          pkt_type = ((*skb_type) << 8) | (*(skb_type + 1) & 0xff);
          BDEBUG (" Received 8021Q packet \n");
          BDEBUG ("pkt_type = %x\n", pkt_type);
          if (pkt_type == ETH_P_PAE)
            {
              skb_head += 4;
              skb_len -= 4;
              BDEBUG (" pkt_type is ETH_P_PAE \n");

              frame_type = br_cvlan_get_frame_type (skb2);
              switch (frame_type)
                {
                  case UNTAGGED:
                       break;
                  case PRIORITY_TAGGED:
                       BDEBUG("Pop vlan tag header for priority tagged frame\n");
                       skb2 = br_vlan_pop_tag_header (skb2);

                      /* Since br_vlan_pop_tag_header() has popped the VLAN tag header
                       * and pushed a regular Ethernet header, we need to pull it so
                       * that the eth_type_trans() logic below works
                       */
                       skb_pull (skb2, skb->dev->hard_header_len);
                       break;
                  case TAGGED:
                  default:
                       kfree_skb (skb);
                       kfree_skb (skb2);
                       return 0;
                       break;
                }

            }
          else
            {
              BDEBUG ("priority tagged packet failed eapol type test 0x%x - drop\n", pt->type);
              goto drop;
            }
        }
      else
        {
          BDEBUG ("packet failed eapol type test 0x%x - drop\n", pt->type);
          goto drop;
        }
    }

  /* We try to queue the data on an open socket with room for the data. */
  sk = hlist_empty (&eapol_sklist) ? NULL : __sk_head (&eapol_sklist);
  for (; sk; sk = sk_next (sk))
    {
      if (atomic_read (&sk->sk_rmem_alloc) + skb2->truesize >= (unsigned)sk->sk_rcvbuf)
          continue;

      /* sk now points to a socket with room for the buffer */

      if (skb_shared (skb2)) 
        {
          struct sk_buff *nskb = skb_clone (skb2, GFP_ATOMIC);
          BDEBUG ("skb shared \n");
      
          if (nskb == NULL)
            goto drop_n_acct;

          if (skb_head != skb2->data) 
            {
              skb2->data = skb_head;
              skb2->len = skb_len;
            }
          kfree_skb (skb2);
          skb2 = nskb;
         }

      /* Fill out the eapol structure here */
      eapol = (struct eapol_addr*)&skb2->cb[0];
      eapol->port = skb2->dev->ifindex;
      memcpy (&eapol->destination_address[0], skb2->mac_header, ETH_ALEN);
      memcpy (&eapol->source_address[0], skb2->mac_header, ETH_ALEN);
#ifdef HAVE_MAC_AUTH
      if ((auth_port) && ( auth_port->auth_state == MACAUTH_ENABLED))
        { 
          bridge_port = (struct apn_bridge_port *)(skb2->dev->apn_fwd_port);
          if (bridge_port == NULL)
            {
              kfree_skb (skb);
              kfree_skb (skb2);
              return -1;
            }
          bridge = bridge_port->br;
          if (bridge == NULL)
            {
              kfree_skb (skb);
              return -1;
            }
          if (bridge_port->port_type == PRO_NET_PORT 
              || bridge_port->port_type == CUST_NET_PORT)
            proto = ETH_P_8021Q_STAG;
          else 
            proto = ETH_P_8021Q_CTAG;

          frame_type = br_vlan_get_frame_type (skb, proto);
          if (frame_type == UNTAGGED)
            {
              if (bridge_port->br->is_vlan_aware)
                eapol->vlanid = bridge_port->default_pvid;
              else
                eapol->vlanid = 0;
            }
          else if ((frame_type == TAGGED) || (frame_type == PRIORITY_TAGGED))
            eapol->vlanid = br_vlan_get_vid_from_frame(proto, skb2);
        } 
#endif
      /* We munge out the packet length here to trim the buffer down */
      skb_len = ((skb2->data[2] << 8) | skb2->data[3]) + 4;
      skb_trim (skb2, skb_len);

      if ((skb->dev->header_ops != NULL) && (skb->dev->header_ops->parse))
        {
          skb2->dev->header_ops->parse (skb2, eapol->source_address);
        }
      else
        {
          memcpy (eapol->source_address, skb2->mac_header + ETH_ALEN, ETH_ALEN);
        }

      /* Queue it on the socket */
      BDEBUG("after pull - skb_len %d \n", skb_len);
  
      /* skb_dump (skb); */
      skb_set_owner_r (skb2, sk);
      skb2->dev = NULL;
      spin_lock (&sk->sk_receive_queue.lock);
      __skb_queue_tail (&sk->sk_receive_queue, skb2);
      spin_unlock (&sk->sk_receive_queue.lock);
      sk->sk_data_ready (sk, skb2->len);
    }

  if (skb != NULL)
    kfree_skb (skb);

  return 0;

 drop_n_acct:

  BDEBUG ("drop\n");
  if (skb_head != skb2->data && skb_shared (skb2)) 
    {
      skb2->data = skb_head;
      skb2->len = skb_len;
    }
 drop:

  kfree_skb (skb);

  if (skb2)
    kfree_skb (skb2);
  return 0;
}

/* Packet handler registration entry. */

static struct packet_type eapol_packet_type = {
  type: __constant_htons (ETH_P_PAE),
  dev: NULL,
  func: eapol_rcv,
};

/* Send a message on an EAPOL socket */

static int 
eapol_sendmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, 
               size_t len)
{
  struct sock *sk = sock->sk;
  struct eapol_addr * eapol = (struct eapol_addr *)msg->msg_name;
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
  if (eapol == NULL) 
    {
      BWARN ("eapol address is null\n");
      goto out;
    }
  else 
    {
      if (msg->msg_namelen < sizeof (struct eapol_addr))
        goto out;

      ifindex   = eapol->port;
      addr      = eapol->destination_address;
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

  if (dev->header_ops) 
    {
      int res;
      err = -EINVAL;
            BDEBUG ("dev->hard_header\n");
      res = dev->header_ops->create (skb, dev, ETH_P_PAE, addr, NULL, len);
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
      BDEBUG ("eapol_header\n");
      eapol_header (skb, dev, eapol->destination_address, 
                      eapol->source_address, len);
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
  Close an EAPOL socket. This is fairly simple. We immediately go
        to 'closed' state and remove our protocol entry in the device list.  */

static int 
eapol_release (struct socket *sock)
{
  struct sock *sk = sock->sk;
  struct sock **skp;
  BDEBUG ("sock %p\n", sk);

  if (!sk)
    return 0;

  BR_WRITE_LOCK_BH (&eapol_sklist_lock);
  sk_del_node_init (sk);
  BR_WRITE_UNLOCK_BH (&eapol_sklist_lock);

  if (atomic_dec_and_test (&eapol_socks_nr))
  {
    auth_remove_all_ports ();
    /* Unhook packet receive handler.  */
    dev_remove_pack (&eapol_packet_type);
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
eapol_recvmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len, int flags)
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

  msg->msg_namelen = sizeof (struct eapol_addr);

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
eapol_notifier (struct notifier_block *this, unsigned long msg, void *data)
{
  struct net_device * dev = (struct net_device *)data;
  struct auth_port * port = 0;
  if (dev == 0)
    return NOTIFY_DONE;

  BDEBUG ("msg %ld dev %s\n", msg, dev->name);
  switch (msg) 
    {
    case NETDEV_DOWN:
    case NETDEV_UNREGISTER:
      BR_WRITE_LOCK (&auth_port_lock);
      port = auth_find_port (dev->ifindex);
      BR_WRITE_UNLOCK (&auth_port_lock);
      if (port !=NULL)
         auth_set_port_state (dev->ifindex, APN_AUTH_PORT_STATE_UNBLOCK);
      break;
    case NETDEV_UP:
      BR_WRITE_LOCK (&auth_port_lock);
      port = auth_find_port (dev->ifindex);
      BR_WRITE_UNLOCK (&auth_port_lock);
      if (port !=NULL)
         auth_set_port_state (dev->ifindex, APN_AUTH_PORT_STATE_BLOCK_INOUT);
      break;
    }
  return NOTIFY_DONE;
}

DECLARE_MUTEX(eapol_ioctl_mutex);

/* This function handles all ioctl commands. */

#define MAX_ARGS  3

int 
eapol_ioctl (struct socket *sock, unsigned int cmd, unsigned long arg)
{
  int err = 0;
  char port_name[IFNAMSIZ +1];
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

  down (&eapol_ioctl_mutex);
  BDEBUG("arg0(%ld) arg1(%ld) arg2(%ld)\n", arglist[0], arglist[1], arglist[2]);
  switch (arglist[0]) 
    {
    case APNEAPOL_GET_VERSION:
      BDEBUG("Get version\n");
      err = APN_EAPOL_VERSION;
      break;

    case APNEAPOL_ADD_PORT:
      if (copy_from_user (port_name, (void *)arglist[1], IFNAMSIZ))
        return -EFAULT;
      port_name[IFNAMSIZ] = '\0';
      BDEBUG("Add port %s\n", port_name);
      err = auth_add_port (port_name);
      break;

    case APNEAPOL_DEL_PORT:
      if (copy_from_user (port_name, (void *)arglist[1], IFNAMSIZ))
        return -EFAULT;
      port_name[IFNAMSIZ] = '\0';
      BDEBUG("Delete port %s\n", port_name);
      err = auth_remove_port (port_name);
      break;

    case APNEAPOL_SET_PORT_STATE:
      BDEBUG("Set port state %ld - %ld\n", arglist[1], arglist[2]);
      err = auth_set_port_state (arglist[1], arglist[2]);
      break;
    /* AUTH Mac Feature */ 
    case APNEAPOL_SET_PORT_MACAUTH_STATE:
      BDEBUG("Set AUTH MAC port state %ld - %ld\n", arglist[1], arglist[2]);
      err = auth_mac_set_port_state (arglist[1], arglist[2]);
      break;

    default:
      BDEBUG("Unknown command - cmd %ld arg1(%ld) arg2(%ld)\n",
         arglist[0], arglist[1], arglist[2]);
      err = -EINVAL;
      break;
    }
  up (&eapol_ioctl_mutex);
  return err;
}

/*
 *      Create a packet of type SOCK_EAPOL. 
 */

int 
eapol_create (struct net *net, struct socket *sock, int protocol)
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
  sk = sk_alloc (net, PF_EAPOL, GFP_KERNEL, &_proto);
  if (sk == NULL)
    return err;

  BDEBUG ("sock %p\n",sk);
  sock->ops = &eapol_ops;
  sock_init_data (sock,sk);

  sk->sk_family = PF_EAPOL;
  sk->sk_protocol = protocol;

  sk->sk_destruct = eapol_sock_destruct;

  /*
   *    Attach a protocol block
   */

  if (atomic_read (&eapol_socks_nr) == 0)
  {
    l2_mod_inc_use_count ();
    dev_add_pack (&eapol_packet_type);
  }
  atomic_inc (&eapol_socks_nr);

  BR_WRITE_LOCK_BH (&eapol_sklist_lock);
  sk_add_node (sk, &eapol_sklist);
  BR_WRITE_UNLOCK_BH (&eapol_sklist_lock);

  return 0;
}

/* We don't do much, but register for the things that we do. */

static struct proto_ops SOCKOPS_WRAPPED (eapol_ops) = {
  family:               PF_EAPOL,

  release:      eapol_release,
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
  sendmsg:      eapol_sendmsg,
  recvmsg:      eapol_recvmsg,
  mmap:         sock_no_mmap,
  sendpage:     sock_no_sendpage,
};

static struct net_proto_family eapol_family_ops = 
{
  family:       PF_EAPOL,
  create:       eapol_create,
  owner:        THIS_MODULE,
};

static struct notifier_block eapol_netdev_notifier = 
{
  notifier_call:        eapol_notifier,
};

/* Handle module exit */
void 
eapol_exit (void)
{
  BWARN ("NET4: Linux 802.1x EAPOL 7/19/2004 for Net4.0 removed\n");
  unregister_netdevice_notifier (&eapol_netdev_notifier);
  sock_unregister (PF_EAPOL);
  return;
}

/* Handle module init */
int  
eapol_init (void)
{
  BWARN ("NET4: Linux 802.1x EAPOL 7/19/2004 for Net4.0\n");
  sock_register (&eapol_family_ops);
  register_netdevice_notifier (&eapol_netdev_notifier);
  return 0;
}
