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
#include "hsl_l2_sock.h"

#include "hal_netlink.h"
#include "hal_socket.h"
#include "hal_msg.h"
#include "hsl_vlan.h"

#define HSL_L2_ETH_P_LLDP               0x88cc

/* List of all HSL backend sockets. */
#ifdef LINUX_KERNEL_2_6
static struct hlist_head _lldp_socklist;
#else
static struct sock *_lldp_socklist = 0;
#endif /* LINUX_KERNEL_2_6 */

//qcl 20170808 static rwlock_t _lldp_socklist_lock = RW_LOCK_UNLOCKED;
static DEFINE_RWLOCK(_lldp_socklist_lock);
/* Private packet socket structures. */

/* Forward declarations. */
static int _lldp_sock_release (struct socket *sock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
static int _lldp_sock_create (struct net *net, struct socket *sock,
                              int protocol);
#else
static int _lldp_sock_create (struct socket *sock, int protocol);
#endif

#ifdef LINUX_KERNEL_2_6
static int _lldp_sock_sendmsg (struct kiocb *iocb, struct socket *sock,
                              struct msghdr *msg, size_t len);
static int _lldp_sock_recvmsg (struct kiocb *iocb, struct socket *sock,
                              struct msghdr *msg, size_t len, int flags);
#else
static int _lldp_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len, struct scm_cookie *scm);
static int _lldp_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len, int flags, struct scm_cookie *scm);
#endif /* LINUX_KERNEL_2_6 */

//qcl 20170808 static struct proto_ops SOCKOPS_WRAPPED (lldp_ops) = {
static struct proto_ops lldp_ops = {
  .family =       AF_LLDP,

  .release =       _lldp_sock_release,
  .bind =          sock_no_bind,
  .connect =       sock_no_connect,
  .socketpair =    sock_no_socketpair,
  .accept =        sock_no_accept,
  .getname =       sock_no_getname,
  .poll =          datagram_poll,
  .ioctl =         sock_no_ioctl,
  .listen =        sock_no_listen,
  .shutdown =      sock_no_shutdown,
  .setsockopt =    sock_no_setsockopt,
  .getsockopt =    sock_no_getsockopt,
  .sendmsg =       _lldp_sock_sendmsg,
  .recvmsg =       _lldp_sock_recvmsg,
  .mmap =          sock_no_mmap,
  .sendpage =      sock_no_sendpage,
};

static struct net_proto_family lldp_family_ops = {
  family:               AF_LLDP,
  create:               _lldp_sock_create,
};


/* Destruct socket. */
static void 
_lldp_sock_destruct (struct sock *sk)
{
  if (!sk)
    return;

  HSL_SOCK_DESTRUCT(sk, _lldp);

  return;

}

/* Release socket. */
static int 
_lldp_sock_release (struct socket *sock)
{
  struct sock *sk = sock->sk;

  HSL_SOCK_DESTRUCT(sk, _lldp);
  sock->sk = NULL;

  return 0;
}

#ifdef LINUX_KERNEL_2_6
static struct proto _lldp_prot = {
        .name     = "HSL_LLDP",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};
#endif /* LINUX_KERNEL_2_6 */

/* Create socket. */
static int 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
_lldp_sock_create (struct net *net, struct socket *sock, int protocol)
#else
_lldp_sock_create (struct socket *sock, int protocol)
#endif
{
  if (!capable (CAP_NET_RAW))
    return -EPERM;
  if (sock->type  != SOCK_RAW)
    return -ESOCKTNOSUPPORT;

  sock->state = SS_UNCONNECTED;
  sock->ops = &lldp_ops;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
  HSL_SOCK_ALLOC(net, sock, AF_LLDP, protocol, _lldp);
#else
  HSL_SOCK_ALLOC(sock, AF_LLDP, protocol, _lldp);
#endif

  return 0;
}

/* Sendmsg. */
#ifdef LINUX_KERNEL_2_6
static int _lldp_sock_sendmsg (struct kiocb *iocb, struct socket *sock,
                                struct msghdr *msg, size_t len)
#else
static int 
_lldp_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len,
                    struct scm_cookie *scm)
#endif /* LINUX_KERNEL_2_6 */
{
  bcm_pkt_t *pkt = NULL;
  int ret = 0;
  u_char *hdr = NULL;
  u_char *buf = NULL;
  struct sockaddr_vlan *s;
  struct hsl_if *ifp;
  struct hsl_if *ifp2;
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
  else if (ifp->type == HSL_IF_TYPE_IP)
    {
      ifp2 = hsl_ifmgr_get_first_L2_port (ifp);

      if (! ifp2)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          ret = -EINVAL;
          goto RET;
        }

      sifp = ifp2->system_info;

      if (! sifp)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_IFMGR_IF_REF_DEC (ifp2);
          ret = -EINVAL;
          goto RET;
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
  HSL_ETH_VLAN_SET_VID (eth->d.vlan.pri_cif_vid, 1);
  eth->d.vlan.type = htons (HSL_L2_ETH_P_LLDP);
  
  /* Send packet. */
  ret = hsl_bcm_pkt_send (pkt, dport, 0);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending LLDP frame out on dport(%d)\n", dport);
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
#ifdef LINUX_KERNEL_2_6
static int _lldp_sock_recvmsg (struct kiocb *iocb, struct socket *sock,
                              struct msghdr *msg, size_t len, int flags)
#else
static int 
_lldp_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len,
                    int flags, struct scm_cookie *scm)
#endif /* LINUX_KERNEL_2_6 */
{
  int size = sizeof (struct sockaddr_vlan);
  struct hsl_if_list *node = NULL;
  struct sockaddr_l2 *s = NULL;
  struct hsl_if *ifpl3 = NULL;
  struct sock *sk = sock->sk;
  struct hsl_if *ifp = NULL;
  struct sk_buff *skb;
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
    {
      memcpy (msg->msg_name, (struct sockaddr_vlan *) skb->data, size);

      s = (struct sockaddr_l2 *)(msg->msg_name);

      ifp = hsl_ifmgr_lookup_by_index (s->port);

      if ( !ifp )
        goto ERR;

      if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        {
           if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
             {
               if ((node = ifp->parent_list) && (ifpl3 = node->ifp))
                 {
                   s->port = ifpl3->ifindex;
                 }
               else
                 {
                   HSL_IFMGR_IF_REF_DEC (ifp);
                   goto ERR;
                 }
             }
         }

       HSL_IFMGR_IF_REF_DEC (ifp);

    }
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

ERR:
  if (sk && skb)
    skb_free_datagram(sk, skb);
  return -1; 

}

#ifdef HAVE_ONMD
/* Post packet. */
int 
hsl_af_lldp_post_packet (struct sk_buff *skb)
{
  struct sock * sk; 
  struct sk_buff *skbc;

  /* Read lock. */
  read_lock_bh (&_lldp_socklist_lock);

  /* Is there room to schedule the packet */
  HSL_SOCK_LIST_LOOP(sk, _lldp)
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
  read_unlock_bh (&_lldp_socklist_lock);

  return 0;
}
#endif /* HAVE_ONMD */

/* HSL socket initialization. */
int
hsl_af_lldp_sock_init (void)
{
  sock_register (&lldp_family_ops);
  return 0;
}

/* HSL socket deinitialization. */
int
hsl_af_lldp_sock_deinit (void)
{
  sock_unregister (AF_LLDP);
  return 0;
}
