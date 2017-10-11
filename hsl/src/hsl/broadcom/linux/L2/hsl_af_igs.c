/* Copyright 2003  All Rights Reserved.  */

#include <linux/version.h>

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
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "hsl_bcm_l2.h"
#include "hsl_l2_sock.h"

#include "hal_netlink.h"
#include "hal_socket.h"
#include "hal_msg.h"
#include "hsl_vlan.h"

/* List of all HSL backend sockets. */
#ifdef LINUX_KERNEL_2_6
static struct hlist_head _igmp_snoop_socklist;
#else
static struct sock *_igmp_snoop_socklist = 0;
#endif
//qcl 20170808 static rwlock_t _igmp_snoop_socklist_lock = RW_LOCK_UNLOCKED;
static DEFINE_RWLOCK(_igmp_snoop_socklist_lock);
/* Private packet socket structures. */

/* Forward declarations. */
static int _igmp_snoop_sock_release (struct socket *sock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
static int _igmp_snoop_sock_create (struct net *net, struct socket *sock,
                                    int protocol);
#else
static int _igmp_snoop_sock_create (struct socket *sock, int protocol);
#endif
#ifdef LINUX_KERNEL_2_6
static int _igmp_snoop_sock_sendmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len);
static int _igmp_snoop_sock_recvmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len, int flags);
#else
static int _igmp_snoop_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len, struct scm_cookie *scm);
static int _igmp_snoop_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len, int flags, struct scm_cookie *scm);
#endif
static int _igmp_snoop_setsockopt (struct socket *sk, int level,
                                   int optname, char *optval,
                                   int optlen);

//qcl 20170808 static struct proto_ops SOCKOPS_WRAPPED (igmp_snoop_ops) = {
static struct proto_ops igmp_snoop_ops = {
  .family =       AF_IGMP_SNOOP,

  .release =       _igmp_snoop_sock_release,
  .bind =          sock_no_bind,
  .connect =       sock_no_connect,
  .socketpair =    sock_no_socketpair,
  .accept =        sock_no_accept,
  .getname =       sock_no_getname,
  .poll =          datagram_poll,
  .ioctl =         sock_no_ioctl,
  .listen =        sock_no_listen,
  .shutdown =      sock_no_shutdown,
  .setsockopt =    _igmp_snoop_setsockopt,
  .getsockopt =    sock_no_getsockopt,
  .sendmsg =       _igmp_snoop_sock_sendmsg,
  .recvmsg =       _igmp_snoop_sock_recvmsg,
  .mmap =          sock_no_mmap,
  .sendpage =      sock_no_sendpage,
};

static struct net_proto_family igmp_snoop_family_ops = {
  family:               AF_IGMP_SNOOP,
  create:               _igmp_snoop_sock_create,
};

static int
_igmp_snoop_setsockopt (struct socket *sk, int level,
                       int optname, char *optval, int optlen)
{
  int snoop_enable;

  if (level != AF_IGMP_SNOOP)
    return  -ENOPROTOOPT;

  if (optlen > 0)
    copy_from_user (&snoop_enable, optval, optlen);

#ifdef HAVE_IGMP_SNOOP
  if (snoop_enable)
    hsl_bcm_igmp_snooping_set (1);
  else
    hsl_bcm_igmp_snooping_set (0);
#endif /* HAVE_IGMP_SNOOP */

  return 0;
}

/* Destruct socket. */
static void 
_igmp_snoop_sock_destruct (struct sock *sk)
{
  if (!sk)
    return;

  HSL_SOCK_DESTRUCT(sk, _igmp_snoop);
}

/* Release socket. */
static int 
_igmp_snoop_sock_release (struct socket *sock)
{
  struct sock *sk = sock->sk;

  if (!sk)
    return 0;

  HSL_SOCK_DESTRUCT(sk, _igmp_snoop);
  sock->sk = NULL;
  return 0;
}

#ifdef LINUX_KERNEL_2_6
static struct proto _igmp_snoop_prot = {
        .name     = "HSL_IGMP_SNOOP",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};
#endif

/* Create socket. */
static int 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
_igmp_snoop_sock_create (struct net *net, struct socket *sock, int protocol)
#else
_igmp_snoop_sock_create (struct socket *sock, int protocol)
#endif
{
  if (!capable (CAP_NET_RAW))
    return -EPERM;
  if (sock->type  != SOCK_RAW)
    return -ESOCKTNOSUPPORT;

  sock->state = SS_UNCONNECTED;
  sock->ops = &igmp_snoop_ops;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
  HSL_SOCK_ALLOC(net, sock, AF_IGMP_SNOOP, protocol, _igmp_snoop);
#else
  HSL_SOCK_ALLOC(sock, AF_IGMP_SNOOP, protocol, _igmp_snoop);
#endif

  return 0;
}

/* Sendmsg. */
static int 
#ifdef LINUX_KERNEL_2_6
_igmp_snoop_sock_sendmsg (struct kiocb *iocb, struct socket *sock, 
                   struct msghdr *msg, size_t len)
#else
_igmp_snoop_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len,
                          struct scm_cookie *scm)
#endif
{
  bcm_pkt_t *pkt = NULL;
  int ret = 0;
  int tagged;
  u_char *hdr = NULL;
  u_char *buf = NULL;
  struct sockaddr_vlan *s;
  struct hsl_if *ifp;
  struct hsl_bcm_if *sifp;
  bcmx_lport_t dport;
  struct hsl_eth_header *eth;

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
  memcpy (eth->dmac, s->dest_mac, 6);
  memcpy (eth->smac, s->src_mac, 6);
  eth->d.vlan.tag_type = htons (0x8100);
  HSL_ETH_VLAN_SET_VID (eth->d.vlan.pri_cif_vid, 1);

    if (!s->vlanid)
    {
      tagged = 0; 
      HSL_ETH_VLAN_SET_VID (eth->d.vlan.pri_cif_vid, 1);
    }
  else
    {
      tagged = hsl_vlan_get_egress_type (ifp, s->vlanid);
      HSL_ETH_VLAN_SET_VID (eth->d.vlan.pri_cif_vid, s->vlanid);
    }

  eth->d.vlan.type = htons (HSL_ETHER_TYPE_IP);

  /* Send packet. */
  ret = hsl_bcm_pkt_send (pkt, dport, tagged);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending IGMP_SNOOP frame out on dport(%d)\n", dport);
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


/* Recvmsg. */
static int 
#ifdef LINUX_KERNEL_2_6
_igmp_snoop_sock_recvmsg (struct kiocb *iocb, struct socket *sock, 
                   struct msghdr *msg, size_t len, int flags)
#else
_igmp_snoop_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len,
                          int flags, struct scm_cookie *scm)
#endif
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

  /* Did user send lesser buffer? */
  copied = skb->len;
  if (copied > len)
    {
      copied = len;
      msg->msg_flags |= MSG_TRUNC;
    }

  /* Copy message. */
  ret = skb_copy_datagram_iovec (skb, size, msg->msg_iov, copied);
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

/* Post packet. */
int 
hsl_af_igs_post_packet (struct sk_buff *skb)
{
  struct sock * sk; 
  struct sk_buff *skbc;

  /* Read lock. */
  read_lock_bh (&_igmp_snoop_socklist_lock);

  /* Is there room to schedule the packet */
  HSL_SOCK_LIST_LOOP(sk, _igmp_snoop)
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
  read_unlock_bh (&_igmp_snoop_socklist_lock);

  return 0;
}

/* HSL socket initialization. */
int
hsl_af_igs_sock_init (void)
{
  sock_register (&igmp_snoop_family_ops);
  return 0;
}

/* HSL socket deinitialization. */
int
hsl_af_igs_sock_deinit (void)
{
  sock_unregister (AF_IGMP_SNOOP);
  return 0;
}

