/* Copyright 2003  All Rights Reserved. 

   CFM AF - An implementation of the CFM address family for Linux
   (See 802.1ag)

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
#include <linux/etherdevice.h>
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
#include "bridge/if_ipifwd.h"
#include "bridge/br_input.h"
#include "bridge/br_types.h"
#include "bridge/br_ioctl.h"
#include "bridge/br_api.h"
#include "bridge/br_vlan.h"
#include "bridge/br_pro_vlan.h"
#include "af_cfm.h"
#include "bdebug.h"

/* List of all cfm sockets. */
static struct hlist_head  cfm_sklist;
static rwlock_t cfm_sklist_lock = RW_LOCK_UNLOCKED;
static atomic_t cfm_socks_nr;

static struct proto _proto = {
        .name     = "CFM",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};


int cfm_create (struct net *net, struct socket *sock, int protocol);
/* Private packet socket structures. */

static void
skb_dump (struct sk_buff *skb)
{
 unsigned int i, snap = 64;

 printk ("skb_dump (%p): from %s with len %d (%d) headroom=%d tailroom = %d\n ",
      skb, skb->dev ? skb->dev->name : "ip stack", skb->len,
      skb->truesize, skb_headroom (skb), skb_tailroom(skb));
#ifdef CONFIG_NET_SCHED
 printk ("mark %lu tcindex %d priority %d  \n", skb->mark,
    skb->tc_index, skb->priority);
#endif
 printk ("\n======= Data dump ===========\n");
 for (i = (unsigned int) skb->head; i <= (unsigned int) skb->tail; i++) {
   if (i == (unsigned int) skb->data)
     printk ("D");
   if (i == (unsigned int) skb->transport_header)
     printk ("R");
   if (i == (unsigned int) skb->network_header)
     printk ("N");
   if (i == (unsigned int) skb->mac_header)
     printk ("M");
   printk ("%02x", *((unsigned char *) i));
   if (0 == i % 8)
     printk (" ");
   if (0 == i % 32)
     printk ("\n");
   if (i == (unsigned int) skb->tail)
     printk ("T");
   if (i == snap)
     break;
 }
 printk ("\n======= Data dump end ============\n");
}


void 
cfm_sock_destruct (struct sock *sk)
{

  BDEBUG ("CFM socket %p free - %d are alive\n", sk, 
                        atomic_read (&cfm_socks_nr));
}

static struct proto_ops cfm_ops;

/* Push a 802.1q VLAN tag onto a frame */
static struct sk_buff *
cfm_vlan_push_tag_header (struct sk_buff *skb, vid_t vid, 
                          unsigned short proto, unsigned int priority,
                          bool_t drop_enable)
{
  struct ethhdr eth_hdr;
  struct vlan_header *vlan_hdr = 0;
  unsigned short vlan_tci = 0;

  BDEBUG("pushing tag header %x \n", vid);

  /* Copy the ethernet header from the skb before the skb_push for VLAN */
  memcpy (&eth_hdr, skb->mac_header, sizeof (struct ethhdr));

  /* Make sure that there is enough headroom for the VLAN header */
  if (skb_headroom (skb) < VLAN_ETH_HEADER_LEN)
    {
      struct sk_buff *sk_tmp = skb;
      skb = skb_realloc_headroom (sk_tmp, VLAN_ETH_HEADER_LEN);
      kfree_skb (sk_tmp);
      if (skb == NULL)
        return NULL;
    }

  /* Add VLAN header. */
  vlan_hdr = (struct vlan_header *)skb_push (skb, VLAN_HEADER_LEN);

  /* TODO */
  if (proto == ETH_P_8021Q_CTAG)
    {
      vlan_tci |= (priority << 13 );
      vlan_tci |= (vid & 0x0fff);
    }
  else
   {
    vlan_tci = (vid & 0x0fff);
    vlan_tci |= (priority << 13);
    vlan_tci |= (1 << 12);
   }

  vlan_tci = htons (vlan_tci);
  vlan_hdr->vlan_tci = vlan_tci;
  vlan_hdr->encapsulated_proto = eth_hdr.h_proto;

  /* Create new ethernet header. */
  if ((skb->dev->header_ops != NULL) && (skb->dev->header_ops->create))
    {
      skb->dev->header_ops->create (skb, skb->dev, proto, eth_hdr.h_dest,
          eth_hdr.h_source, skb->len + VLAN_HEADER_LEN + ETH_HLEN);
    }
  else
    {
      eth_header (skb, skb->dev, proto, eth_hdr.h_dest,
                  eth_hdr.h_source, skb->len + VLAN_HEADER_LEN + ETH_HLEN);
    }

  return skb;
}  
/*
  This the bottom half of the bridge packet handling code.
  We do understand shared skbs. We do not register a packet handler
  because the bridge has grabbed all of the frames. We are really
  skipping the ip stack altogether.
*/

int 
cfm_rcv (struct sk_buff * skb, struct apn_bridge * bridge,  
         struct net_device * port)
{
  struct sock * sk; 
  unsigned short proto;
  u8 * skb_head = skb->data;
  int skb_len = skb->len;
  struct sockaddr_igs * l2_skaddr;
  br_frame_t frame_type;
  u_int16_t cvid;
  u_int16_t svid;
  u_int8_t level;
  struct apn_bridge_port *bridge_port;
  int pkt_type = *(skb->mac_header + 2 * ETH_ALEN) << 8 | 
                      *(skb->mac_header + 2 * ETH_ALEN + 1);

  struct br_vlan_pro_edge_port *pro_edge_port = NULL;
  struct vlan_header *vlan_hdr = NULL;
  u_int8_t priority;

  BR_READ_LOCK_BH (&cfm_sklist_lock);

  /* Is there room to schedule the packet */
  sk = hlist_empty (&cfm_sklist) ? NULL : __sk_head (&cfm_sklist);

  for (; sk; sk = sk_next (sk))
    {
      BDEBUG ("sock %p bridge %s port %s len %d\n", sk, 
          bridge->name, port->name, skb->len);

      if (atomic_read (&sk->sk_rmem_alloc) + skb->truesize 
          >= (unsigned)sk->sk_rcvbuf)
        {
          continue;
        }

      if (sk == 0)
        {
          BDEBUG ("No room to schedule received frame on any socket - drop\n");
          goto drop;
        }

      /* Create a copy of the packet as we make change to it 
         for untagged frames. */
        {
          struct sk_buff *nskb = skb_copy (skb, GFP_ATOMIC);
          BDEBUG ("skb shared \n");

          if (nskb == NULL)
            goto drop_n_acct;

          skb = nskb;
        }

      /* Fill out the l2_skaddr structure here */
      l2_skaddr = (struct sockaddr_igs*)&skb->cb[0];
      /* Indices first */
      l2_skaddr->port = skb->dev->ifindex;

      memcpy (&l2_skaddr->dest_mac[0], skb->mac_header, ETH_ALEN);

      if ((skb->dev->header_ops != NULL) && (skb->dev->header_ops->parse))
        {
          skb->dev->header_ops->parse (skb, l2_skaddr->src_mac);
        }
      else
        {
          memcpy (l2_skaddr->src_mac, skb->mac_header + ETH_ALEN, ETH_ALEN);
        }

      /* Queue it on the socket */
      if (pkt_type < 1536)
        skb_trim (skb, pkt_type);

      /*  Get frame type. If untagged, set vlan header. */
      bridge_port = br_get_port (bridge, skb->dev->ifindex);

      if (! bridge_port)
        goto drop;

      /* Get frame type. If untagged, set vlan header. */
      if (bridge_port->port_type == PRO_NET_PORT
          || bridge_port->port_type == CUST_NET_PORT)
        {
          proto = ETH_P_8021Q_STAG;
          frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_STAG);
        }
      else
        {
          proto = ETH_P_8021Q_CTAG;
          frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);
        }

      if (frame_type == UNTAGGED || frame_type == PRIORITY_TAGGED)
        {
          l2_skaddr->vlanid = bridge_port->default_pvid;

          if (frame_type == PRIORITY_TAGGED)
            skb_pull (skb, VLAN_HEADER_LEN);

          level = l2_skaddr->dest_mac[5] & CFM_GET_LEVEL_DEST_ADDR_MASK;

          if (level == CFM_DEFAULT_LINK_LEVEL)
            {
              /* All untagged frames at level 0 will be passed to onmd */
              goto lift_to_onmd;
            }

          /* For untagged frames at any other level, bridge_port->default_pvid
           * will be used as cvid */
        }
      else if (frame_type == TAGGED)
        {
          /* all this should happen for untagged also based on the default vid
           * of the CE port */
          l2_skaddr->vlanid = br_vlan_get_vid_from_frame(proto, skb);
              
          vlan_hdr = (struct vlan_header *)skb->data;

          priority = ((ntohs(vlan_hdr->vlan_tci) >> 13) 
                           & VLAN_USER_PRIORITY_MASK );

        }
      else
        {
          /* Unknown frame type. Drop */
          goto drop;
        }
      
      if (bridge_port->port_type == CUST_EDGE_PORT)
        {
          /* In PB, from the edge bridge tagged as well as untagged frames 
           * will be forwarded into the core. 
           * Untag vids will use the default-vid of the port as the cvid */

          /* On a Provider Edge Bridge, the cvlan-svlan mapping is given
           * by the swfwdr to ONMD */
          cvid = l2_skaddr->vlanid;

          svid = br_vlan_svid_get (bridge_port, cvid);

          l2_skaddr->svlanid = svid;

          pro_edge_port = br_vlan_pro_edge_port_lookup (bridge_port, svid);

          if (pro_edge_port == NULL)
            {
              /* This svid is not configued so dont lift the frame. 
               * Just drop it here */
              goto drop;
            }

          if (pro_edge_port->untagged_vid != cvid)
            {
              /* Add ctag, add stag relay the frame out of PN ports */
              if (frame_type == UNTAGGED)
                {
                  /* C-tag would have to be added to the frame */
                  skb = br_handle_provider_edge_config(skb, bridge_port, cvid, svid);
                }

              /* Relay the frame */
              br_provider_vlan_flood (bridge, skb, cvid, svid, 0); 

              /* Do no lift the frame */
              goto drop;

            }
          else
            {
              /* Frame is on maintenace cvlan, lift it to onmd, as flow
               * points might be present for this */

              /* Just fall through with existing code flow. */

            }
        }
      else if (bridge_port->port_type == PRO_NET_PORT)
        {
          /* Check if the incoming frame is double tagged 
           * If so, do not lift the frame to onmd, just relay it onto the CE ports */
          if (proto == ETH_P_8021Q_STAG)
            {
              /* Outer tag is s-tag */

              svid = l2_skaddr->vlanid;

              /* Checking if ctag is present in the frame */
              vlan_hdr = (struct vlan_header *)skb->data;
              if (ntohs (vlan_hdr->encapsulated_proto) == ETH_P_8021Q_CTAG)
                {
                  /* obtain cvid from the frame */
                  cvid = br_vlan_get_vid_from_frame (ETH_P_8021Q_CTAG, skb);

                  BDEBUG ("Double tag frame, cvid %d, svid %d\n", cvid, svid);

                  /* Relay the frame */
                  br_provider_vlan_flood (bridge, skb, cvid, svid, 0); 

                  /* Dont pass up to onmd */
                  goto drop; 

                }
            }/* if (proto == ETH_P_8021Q_STAG) */

        }/* else if (bridge_port->port_type == PRO_NET_PORT) */

     if (frame_type == TAGGED) 
       {
         /* Encoding the priority received in the frame */
         l2_skaddr->vlanid = (l2_skaddr->vlanid | (priority << 13)) & 0xFFFF;

         /* set the CIF (bit12) in vlanid in the received frame
          * to indicate that the frame received was tagged. */
         l2_skaddr->vlanid |= 0x1000;
         skb_pull (skb, VLAN_HEADER_LEN);
       }

 lift_to_onmd:    
      skb_set_owner_r (skb, sk);
      skb->dev = NULL;
      spin_lock (&sk->sk_receive_queue.lock);
      __skb_queue_tail (&sk->sk_receive_queue, skb);
      spin_unlock (&sk->sk_receive_queue.lock);
      sk->sk_data_ready (sk, skb->len); 
    }
  
 BR_READ_UNLOCK_BH (&cfm_sklist_lock);

 return 0;

 drop_n_acct:

  BDEBUG ("drop\n");
  if (skb_head != skb->data && skb_shared (skb)) 
    {
      skb->data = skb_head;
      skb->len = skb_len;
    }
 drop:

  BR_READ_UNLOCK_BH (&cfm_sklist_lock);
  kfree_skb (skb);
  return 0;
}

  

static int
cfm_sendmsg (struct kiocb *iocb, struct socket *sock,
             struct msghdr *msg, size_t len)
{
  char *pnt;
  int hdrsz;
  char *start = NULL;
  struct sk_buff *skb;
  enum vlan_type type;
  unsigned char *addr;
  unsigned short proto;
  struct net_device *dev;
  char *config_ports = NULL;
  struct sock *sk = sock->sk;
  struct apn_bridge *br = NULL;
  int ifindex, err, reserve = 0;
  bool_t is_egress_tagged = BR_FALSE;
  struct apn_bridge_port *port = NULL;
  struct sockaddr_igs * l2_skaddr = (struct sockaddr_igs *)msg->msg_name;
  u_int32_t priority = 0;
  bool_t drop_enable = BR_FALSE;
    
  /*
   *    Get and verify the address. 
   */
         
  BDEBUG ("sock %p msg %p len %d\n", sk, msg, len);

  if (l2_skaddr == NULL) 
    {
      BWARN ("l2_skaddr is null\n");
      return -EINVAL;
    }
  else 
    {
      err = -EINVAL;
      if (msg->msg_namelen < sizeof (struct sockaddr_igs))
        goto out;

      ifindex   = l2_skaddr->port;
      addr      = l2_skaddr->dest_mac;
    }

  proto = sk->sk_protocol;

  dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);

  /* Extract the priority & drop elegibility from the vlan id
   * Assumption is vlan id consists of 16 bits.
   * only 12 bits are used for representing the same.
   * 3 MSB bits (last 3 bits) can be used for storing the priority.
   * 4 MSB bit can be used for storing the drop elegibility.
   * These parameters are supplied from CFM Mep.
   */
  priority = (l2_skaddr->vlanid >> 13 ) & 0xF;

  drop_enable = (l2_skaddr->vlanid >> 12) & 0x1;

  l2_skaddr->vlanid &=  0x0FFF;
  
  BDEBUG ("ifindex %d dev %p sock %p type %d proto %d vid %d\n", 
          ifindex, dev, sk, sock->type, proto, l2_skaddr->vlanid);

  err = -ENXIO;
  if (dev == NULL)
    goto out_unlock;

  if (sock->type == SOCK_RAW)
    reserve = dev->hard_header_len;

  err = -EMSGSIZE;

  if (len > (dev->mtu + reserve))
    goto out_unlock;

  /* Allocate work memory. */
  start = (unsigned char *) kmalloc (len, GFP_KERNEL);

  if (! start)
    goto out_unlock;

  /* Returns -EFAULT on error */
  err = memcpy_fromiovec (start, msg->msg_iov, len);
  if (err)
    goto out_unlock;

  hdrsz = ETH_HLEN + 3; /* DA MAC + SA MAC */
  
  hdrsz += 4; /* Vlan Info */

  skb = sock_alloc_send_skb (sk, len+reserve+ hdrsz, 
                             msg->msg_flags & MSG_DONTWAIT, &err);
  if (skb==NULL)
    goto out_unlock;

  skb_reserve (skb, (reserve + hdrsz) & ~hdrsz);
  skb->network_header = skb->data;

  skb->protocol = proto;
  skb->dev = dev;

  port = (struct apn_bridge_port *)skb->dev->apn_fwd_port;

  if ( (!port) )
    goto out_free;

  br = port->br;

  if ( (!br) )
    goto out_free;

  if (br->is_vlan_aware)
    {
      if (l2_skaddr->vlanid != 0)
        {
          if (!br_vlan_is_port_in_vlans_member_set (br,l2_skaddr->vlanid, 
                port->port_no))
            {
              BDEBUG ("port not in vlan member set\n");
              goto out_free;
            }

          if (port->port_type == PRO_NET_PORT
              || port->port_type == CUST_NET_PORT)
            type = SERVICE_VLAN;
          else
            type = CUSTOMER_VLAN;

          config_ports = br_vlan_get_vlan_config_ports ( br, type, l2_skaddr->vlanid);

          if (!config_ports)
            {
              BDEBUG ("config ports null\n");
              goto out_free;
            }

          is_egress_tagged = IS_VLAN_EGRESS_TAGGED ( config_ports[port->port_id]);
        }

       eth_header (skb, dev, ETH_P_CFM,
                   l2_skaddr->dest_mac, l2_skaddr->src_mac, len); 

       skb->mac_header=skb->data;

       if (is_egress_tagged)
         {
            skb_pull (skb, ETH_HLEN);
            cfm_vlan_push_tag_header (skb, l2_skaddr->vlanid,
              type == CUSTOMER_VLAN ? ETH_P_8021Q_CTAG:
              ETH_P_8021Q_STAG, priority, drop_enable);
         }
    }
  else 
    {
      if ((dev->header_ops != NULL) && (dev->header_ops->create))
        {
          int res;
          err = -EINVAL;
          res = dev->header_ops->create (skb, dev, ETH_P_CFM, addr,
					 l2_skaddr->src_mac, len);
	  
          if (sock->type != SOCK_RAW) /* Changed from DGRAM */
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
	  eth_header (skb, dev, ETH_P_CFM,
		      l2_skaddr->dest_mac, l2_skaddr->src_mac, len);
	}
    }
  /* Create space in buffer. */
  pnt = skb_put (skb, len);
  
  /* Just copy the contents removing the VLAN information passed. */ 
  memcpy (pnt, start, len);
  
  /* Free work memory. */
  kfree (start);
  start = NULL;
  
  err = -ENETDOWN;
  if (!(dev->flags & IFF_UP))
    goto out_free;
  
  /*
   *    Now send it
   */
  
  BDEBUG ("dev_queue_xmit called\n");
  err = dev_queue_xmit (skb);
  if (err > 0 && (err = net_xmit_errno (err)) != 0)
    goto out_unlock;
  
  dev_put (dev);
  
  return (len);
  
 out_free:
  if (start)
    {
      kfree (start);
      start = NULL;
    }
  kfree_skb (skb);
 out_unlock:
  if (start)
    {
      kfree (start);
      start = NULL;
    }
  if (dev)
    dev_put (dev);
 out:
  if (start)
    {
      kfree (start);
      start = NULL;
    }
  return err;
}

/*
 *      Close a CFM socket. This is fairly simple. We immediately go
 *      to 'closed' state and remove our protocol entry in the device list.
 */

static int 
cfm_release (struct socket *sock)
{
  struct sock *sk = sock->sk;
  struct sock **skp;
  BDEBUG ("sock %p\n", sk);

  if (!sk)
    return 0;

  BR_WRITE_LOCK_BH (&cfm_sklist_lock);
  sk_del_node_init (sk);
  BR_WRITE_UNLOCK_BH (&cfm_sklist_lock);
  /*
   *    Now the socket is dead. No more input will appear.
   */

  sock_orphan (sk);
  sock->sk = NULL;

  /* Purge queues */
  skb_queue_purge (&sk->sk_receive_queue);

  if (atomic_dec_and_test (&cfm_socks_nr))
    {
      l2_mod_dec_use_count ();
    }

  sock_put (sk);
  return 0;
}


/*
 *      Create a packet of type SOCK_CFM. 
 */

int 
cfm_create (struct net *net, struct socket *sock, int protocol)
{
  struct sock *sk;

  BDEBUG ("protocol %d socket addr %p\n",protocol,sock);
  if (!capable (CAP_NET_RAW))
      return -EPERM;

  if (sock->type  != SOCK_RAW)
      return -ESOCKTNOSUPPORT;
 
  sock->state = SS_UNCONNECTED;

  sk = sk_alloc (net, PF_CFM, GFP_KERNEL, &_proto);
  
  if (sk == NULL)
    return -ENOBUFS;

  BDEBUG ("sock %p\n",sk);
  sock->ops = &cfm_ops;
  sock_init_data (sock,sk);

  sk->sk_family = PF_CFM;
  sk->sk_protocol = protocol;
  sk->sk_destruct = cfm_sock_destruct;

  if (atomic_read (&cfm_socks_nr) == 0)
  {
    l2_mod_inc_use_count ();
  }
  atomic_inc (&cfm_socks_nr);

  BR_WRITE_LOCK_BH (&cfm_sklist_lock);
  sk_add_node (sk, &cfm_sklist);
  BR_WRITE_UNLOCK_BH (&cfm_sklist_lock);
  return 0;
}

/*
 *      Pull a packet from our receive queue and hand it to the user.
 *      If necessary we block.
 */

static int
cfm_recvmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg,
             size_t len, int flags)
{
  struct sock *sk;
  struct sk_buff *skb;
  int copied, err;

  err = -EINVAL;
  if (flags & ~(MSG_PEEK|MSG_DONTWAIT|MSG_TRUNC))
    return err;

  if ((sock == 0) || (msg == 0) || (iocb == 0) ||(sock->sk == 0 ))
    {
      BDEBUG("Invalid datagram received sock->sk (%p) scm (%p) \n",sock->sk,iocb);
      return err;
    }
  
  sk = sock->sk;
  BDEBUG ("RECVD MSD sock %p msg %p len %d flags 0x%x\n", sk, msg, len, flags);
  /*
   *    If the address length field is there to be filled in, we fill
   *    it in now.
   */

  msg->msg_namelen = sizeof (struct sockaddr_igs);

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
      copied=len;
      msg->msg_flags |= MSG_TRUNC;
    }

  err = skb_copy_datagram_iovec (skb, 0, msg->msg_iov, copied);
  if (err)
    {
      BDEBUG("Copy datagram_iovec failed %d\n", err);
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

DECLARE_MUTEX(cfm_ioctl_mutex);

static struct proto_ops SOCKOPS_WRAPPED (cfm_ops) = {
  family:       PF_CFM,

  release:      cfm_release,
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
  sendmsg:      cfm_sendmsg,
  recvmsg:      cfm_recvmsg,
  mmap:         sock_no_mmap,
  sendpage:     sock_no_sendpage,
};

static struct net_proto_family cfm_family_ops = {
  family:               PF_CFM,
  create:               cfm_create,
};

void 
cfm_exit (void)
{
  BWARN ("NET4: 7/19/2004 Linux 802.1ag CFM 1.0 for Net4.0 removed\n");
  sock_unregister (PF_CFM);
  return;
}

int  
cfm_init (void)
{
  BWARN ("NET4: 7/19/2004 Linux 802.1ag CFM 1.0 for Net4.0\n");
  sock_register (&cfm_family_ops);
  return 0;
}
