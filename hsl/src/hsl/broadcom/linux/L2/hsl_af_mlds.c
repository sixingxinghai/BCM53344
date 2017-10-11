/* Copyright 2003  All Rights Reserved.  */

#include <linux/version.h>

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#ifdef HAVE_MLD_SNOOP

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
#include "hsl_bridge.h"
#include "hsl_bcm_l2.h"

/* List of all HSL backend sockets. */
#ifdef LINUX_KERNEL_2_6
static struct hlist_head _mld_snoop_socklist;
#else
static struct sock *_mld_snoop_socklist = 0;
#endif
static rwlock_t _mld_snoop_socklist_lock = RW_LOCK_UNLOCKED;

/* Private packet socket structures. */

/* Forward declarations. */
static int _mld_snoop_sock_release (struct socket *sock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
static int _mld_snoop_sock_create (struct net *net, struct socket *sock,
                                   int protocol);
#else
static int _mld_snoop_sock_create (struct socket *sock, int protocol);
#endif
#ifdef LINUX_KERNEL_2_6
static int _mld_snoop_sock_sendmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len);
static int _mld_snoop_sock_recvmsg (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t len, int flags);
#else
static int _mld_snoop_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len, struct scm_cookie *scm);
static int _mld_snoop_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len, int flags, struct scm_cookie *scm);
#endif

static struct proto_ops SOCKOPS_WRAPPED (mld_snoop_ops) = {
  family:        AF_MLD_SNOOP,

  release:        _mld_snoop_sock_release,
  bind:                sock_no_bind,
  connect:        sock_no_connect,
  socketpair:        sock_no_socketpair,
  accept:        sock_no_accept,
  getname:        sock_no_getname,
  poll:                datagram_poll,
  ioctl:        sock_no_ioctl,
  listen:        sock_no_listen,
  shutdown:        sock_no_shutdown,
  setsockopt:        sock_no_setsockopt,
  getsockopt:        sock_no_getsockopt,
  sendmsg:        _mld_snoop_sock_sendmsg,
  recvmsg:        _mld_snoop_sock_recvmsg,
  mmap:                sock_no_mmap,
  sendpage:        sock_no_sendpage,
};

static struct net_proto_family mld_snoop_family_ops = {
  family:                AF_MLD_SNOOP,
  create:                _mld_snoop_sock_create,
};

#ifdef HAVE_IPNET
#undef s6_addr
#undef ifr_name

#include "ipcom.h"
#include "ipcom_lkm_h.h"
#include "ipcom_sock.h"
#include "ipcom_sock6.h"
#include "ipcom_sock2.h"
#include "ipcom_inet.h"
#include "ipnet.h"
#include "ipnet_netif.h"
#include "ipnet_routesock.h"
#include "ipcom_sysctl.h"
#include "ipnet_route.h"
#include "ipnet_h.h"

static int
hsl_ipnet_put_cmsg (struct Ip_msghdr *msg, char *buf, int ifindex)
{

  int cmsg_len = 0;
  int len = 0;
  int *hoplimit_ptr;
  u_int8_t msg_type;
  struct hal_in6_header *ipv6hdr;
  struct Ip_in6_pktinfo *pktinfo;
  struct Ip_cmsghdr*cmsg = IP_CMSG_FIRSTHDR(msg);

  if (cmsg == NULL)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "cmsg NULL \n");
      return 0;
    }

  ipv6hdr = (struct hal_in6_header *) buf;

  if (ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_nxt == IP_IPPROTO_HOPOPTS)
    {
      msg_type = (u_int8_t) *(buf + sizeof (struct hal_in6_header)
                                  + MLD_MESSAGE_TYPE_OFFSET);

      if ((msg_type != MLD_LISTENER_QUERY)
           && (msg_type != MLD_LISTENER_REPORT)
           && (msg_type != MLD_LISTENER_DONE)
           && (msg_type != MLDV2_LISTENER_REPORT))
        return -1;
    }
  else
    return -1;

  len = sizeof(struct Ip_in6_pktinfo);

  cmsg_len += IP_CMSG_SPACE(len);

  if (cmsg_len > msg->msg_controllen)
    goto truncated;

  cmsg->cmsg_len   = IP_CMSG_LEN(len);
  cmsg->cmsg_level = IP_IPPROTO_IPV6;
#ifdef IP_PORT_LKM
  cmsg->cmsg_type = IP_IPV6_RECVPKTINFO;
#else
  cmsg->cmsg_type = IP_IPV6_PKTINFO;
#endif

  pktinfo = (struct Ip_in6_pktinfo *) IP_CMSG_DATA(cmsg);

  memcpy (&pktinfo->ipi6_addr, &ipv6hdr->ip6_dst, sizeof (struct hal_in6_addr));

  pktinfo->ipi6_ifindex = ifindex;
  cmsg = IP_CMSG_NXTHDR(msg, cmsg);

  len = sizeof(int);
  cmsg_len += IP_CMSG_SPACE(len);

  if (cmsg_len > msg->msg_controllen)
    goto truncated;

  cmsg->cmsg_len   = IP_CMSG_LEN(len);
  cmsg->cmsg_level = IP_IPPROTO_IPV6;
  cmsg->cmsg_type  = IP_IPV6_HOPLIMIT;
  hoplimit_ptr = (int *) IP_CMSG_DATA(cmsg);
  *hoplimit_ptr = ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_hlim;
  cmsg = IP_CMSG_NXTHDR(msg, cmsg);

  buf = buf + sizeof (struct hal_in6_header);

  len = (buf [1]+ 1) << 3;
  cmsg_len += IP_CMSG_SPACE(len);

  if (cmsg_len > msg->msg_controllen)
    goto truncated;

  cmsg->cmsg_len   = IP_CMSG_LEN(len);
  cmsg->cmsg_level = IP_IPPROTO_IPV6;
  cmsg->cmsg_type  = IP_IPV6_HOPOPTS;

  memcpy(IP_CMSG_DATA(cmsg), buf, len);
  cmsg = IP_CMSG_NXTHDR(msg, cmsg);

  msg->msg_controllen -= cmsg_len;
  msg->msg_control += cmsg_len;

goto exit;

truncated:

  if (msg->msg_controllen < cmsg_len)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Message truncated \n");
      msg->msg_flags |= MSG_CTRUNC;
      cmsg_len -= CMSG_SPACE(len);
    }

exit:
   return 0;
}

static u_char *
hsl_ipnet_add_ipv6_hdr (struct Ip_msghdr *msg, int *hdrlen,
                        hsl_ipv6Address_t ip6_dst)
{
  int i;
  int msg_len = 0;
  u_char *buf = NULL;
  struct Ip_cmsghdr *cmsg;
  struct hal_in6_header *ipv6hdr;
  struct in6_pktinfo *pktinfo;
  u_char ra[8] = { IP_IPPROTO_ICMPV6, 0,
                   IP_IP_ROUTER_ALERT, 2, 0, 0,
                   IP_IP6OPT_PADN, 0 };

  buf = (u_char *) kmalloc (sizeof (struct hal_in6_header) + sizeof (ra),
                            GFP_KERNEL);
  if (! buf)
    {
      if (hdrlen)
        *hdrlen = 0;
      return NULL;
    }

  for (i = 0; i < msg->msg_iovlen; i++)
     msg_len += msg->msg_iov[i].iov_len;

  ipv6hdr = (struct hal_in6_header *) buf;

  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_plen = msg_len + sizeof (ra);
  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_flow = 0;
  ipv6hdr->hal_ip6_ctlun.ip6_un2_vfc =  0x60;
  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_hlim = HSL_MLD_HOP_LIMIT_DEF;
  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_nxt = IPPROTO_HOPOPTS;
  memcpy (&ipv6hdr->ip6_dst, &ip6_dst, sizeof (struct hal_in6_addr));
  memcpy (buf + sizeof (struct hal_in6_header), ra, sizeof (ra));

  for (cmsg = IP_CMSG_FIRSTHDR(msg);
       cmsg != IP_NULL;
       cmsg = IP_CMSG_NXTHDR(msg, cmsg))
     {
       if (cmsg->cmsg_level != IP_IPPROTO_IPV6)
         continue;
       switch (cmsg->cmsg_type)
        {
#ifdef IP_PORT_LKM
          case IP_IPV6_RECVPKTINFO:
#endif
          case IP_IPV6_PKTINFO:
            pktinfo = IP_CMSG_DATA (cmsg);
            memcpy (&ipv6hdr->ip6_src, &pktinfo->ipi6_addr, sizeof (struct hal_in6_addr));
            break;
          default:
          break;
        }
     }

   if (hdrlen)
     *hdrlen = sizeof (struct hal_in6_header) + sizeof (ra);

   return buf;
}

#else
static int
hsl_put_cmsg (struct msghdr *msg, char *buf, int ifindex)
{

  int cmsg_len = 0;
  int len = 0;
  int *hoplimit_ptr;
  u_int8_t msg_type;
  struct hal_in6_header *ipv6hdr;
  struct in6_pktinfo *pktinfo;
  struct cmsghdr *cmsg = CMSG_FIRSTHDR(msg);

  if (cmsg == NULL)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "cmsg NULL \n");
      return 0;
    }

  ipv6hdr = (struct hal_in6_header *) buf;

  if (ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_nxt == IPPROTO_HOPOPTS)
    {
      msg_type = (u_int8_t) *(buf + sizeof (struct hal_in6_header)
                                  + MLD_MESSAGE_TYPE_OFFSET);

      if ((msg_type != MLD_LISTENER_QUERY)
           && (msg_type != MLD_LISTENER_REPORT)
           && (msg_type != MLD_LISTENER_DONE)
           && (msg_type != MLDV2_LISTENER_REPORT))
        return -1;
    }
  else
    return -1;

  len = sizeof(struct in6_pktinfo);

  cmsg_len += CMSG_SPACE(len);

  if (cmsg_len > msg->msg_controllen)
    goto truncated;

  cmsg->cmsg_len   = CMSG_LEN(len);
  cmsg->cmsg_level = IPPROTO_IPV6;
  cmsg->cmsg_type = IPV6_PKTINFO;

  pktinfo = (struct in6_pktinfo *) CMSG_DATA(cmsg);

  memcpy (&pktinfo->ipi6_addr, &ipv6hdr->ip6_dst, sizeof (struct hal_in6_addr));

  pktinfo->ipi6_ifindex = ifindex;
  cmsg = CMSG_NXTHDR(msg, cmsg);

  len = sizeof(int);
  cmsg_len += CMSG_SPACE(len);

  if (cmsg_len > msg->msg_controllen)
    goto truncated;

  cmsg->cmsg_len   = CMSG_LEN(len);
  cmsg->cmsg_level = IPPROTO_IPV6;
  cmsg->cmsg_type  = IPV6_HOPLIMIT;
  hoplimit_ptr = (int *) CMSG_DATA(cmsg);
  *hoplimit_ptr = ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_hlim;
  cmsg = CMSG_NXTHDR(msg, cmsg);

  buf = buf + sizeof (struct hal_in6_header);

  len = (buf [1]+ 1) << 3;
  cmsg_len += CMSG_SPACE(len);

  if (cmsg_len > msg->msg_controllen)
    goto truncated;

  cmsg->cmsg_len   = CMSG_LEN(len);
  cmsg->cmsg_level = IPPROTO_IPV6;
  cmsg->cmsg_type  = IPV6_HOPOPTS;

  memcpy(CMSG_DATA(cmsg), buf, len);

  cmsg = CMSG_NXTHDR(msg, cmsg);

  msg->msg_controllen -= cmsg_len;
  msg->msg_control += cmsg_len;

goto exit;

truncated:

  if (msg->msg_controllen < cmsg_len)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Message truncated \n");
      msg->msg_flags |= MSG_CTRUNC;
      cmsg_len -= CMSG_SPACE(len);
    }

exit:
   return 0;
}

static u_char *
hsl_add_ipv6_hdr (struct msghdr *msg, int *hdrlen,
                  hsl_ipv6Address_t ip6_dst)
{
  int i;
  int msg_len = 0;
  u_char *buf = NULL;
  struct cmsghdr *cmsg;
  struct hal_in6_header *ipv6hdr;
  struct in6_pktinfo *pktinfo;
  u_char ra[8] = { IPPROTO_ICMPV6, 0,
                   IP_ROUTER_ALERT, 2, 0, 0,
                   IPV6_TLV_PADN, 0 };

  buf = (u_char *) kmalloc (sizeof (struct hal_in6_header) + sizeof (ra),
                            GFP_KERNEL);
  if (! buf)
    {
      if (hdrlen)
        *hdrlen = 0;
      return NULL;
    }

  for (i = 0; i < msg->msg_iovlen; i++)
     msg_len += msg->msg_iov[i].iov_len;

  ipv6hdr = (struct hal_in6_header *) buf;

  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_plen = msg_len + sizeof (ra);
  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_flow = 0;
  ipv6hdr->hal_ip6_ctlun.ip6_un2_vfc =  0x60;
  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_hlim = HSL_MLD_HOP_LIMIT_DEF;
  ipv6hdr->hal_ip6_ctlun.hal_ip6_un1.ip6_un1_nxt = IPPROTO_HOPOPTS;
  memcpy (&ipv6hdr->ip6_dst, &ip6_dst, sizeof (struct hal_in6_addr));
  memcpy (buf + sizeof (struct hal_in6_header), ra, sizeof (ra));

  for (cmsg = CMSG_FIRSTHDR(msg);
       cmsg != NULL;
       cmsg = CMSG_NXTHDR(msg, cmsg))
     {
       if (cmsg->cmsg_level != IPPROTO_IPV6)
         continue;
       switch (cmsg->cmsg_type)
        {
          case IPV6_PKTINFO:
            pktinfo = (struct in6_pktinfo *) CMSG_DATA (cmsg);
            memcpy (&ipv6hdr->ip6_src, &pktinfo->ipi6_addr, sizeof (struct hal_in6_addr));
            break;
          default:
          break;
        }
     }

   if (hdrlen)
     *hdrlen = sizeof (struct hal_in6_header) + sizeof (ra);

   return buf;
}
#endif

/* Destruct socket. */
static void 
_mld_snoop_sock_destruct (struct sock *sk)
{
  if (!sk)
    return;

  HSL_SOCK_DESTRUCT(sk, _mld_snoop);
}

/* Release socket. */
static int 
_mld_snoop_sock_release (struct socket *sock)
{
  struct sock *sk = sock->sk;

  if (!sk)
    return 0;

  HSL_SOCK_DESTRUCT(sk, _mld_snoop);
  sock->sk = NULL;
  return 0;
}

#ifdef LINUX_KERNEL_2_6
static struct proto _mld_snoop_prot = {
        .name     = "HSL_MLD",
        .owner    = THIS_MODULE,
        .obj_size = sizeof(struct sock),
};
#endif

/* Create socket. */
static int 
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
_mld_snoop_sock_create (struct net *net, struct socket *sock, int protocol)
#else
_mld_snoop_sock_create (struct socket *sock, int protocol)
#endif
{
  if (!capable (CAP_NET_RAW))
    return -EPERM;
  if (sock->type  != SOCK_RAW)
    return -ESOCKTNOSUPPORT;

  sock->state = SS_UNCONNECTED;
  sock->ops = &mld_snoop_ops;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
  HSL_SOCK_ALLOC(net, sock, AF_MLD_SNOOP, protocol, _mld_snoop);
#else
  HSL_SOCK_ALLOC(sock, AF_MLD_SNOOP, protocol, _mld_snoop);
#endif
  return 0;
}

/* Sendmsg. */
static int 
#ifdef LINUX_KERNEL_2_6
_mld_snoop_sock_sendmsg (struct kiocb *iocb, struct socket *sock, 
                   struct msghdr *msg, size_t len)
#else
_mld_snoop_sock_sendmsg (struct socket *sock, struct msghdr *msg, int len,
                          struct scm_cookie *scm)
#endif
{
  bcm_pkt_t *pkt = NULL;
  int ret = 0;
  int ip6len = 0;
  int tagged;
  u_char *hdr = NULL;
  u_char *buf = NULL;
  struct sockaddr_mld *s;
  struct hsl_if *ifp;
  struct hsl_bcm_if *sifp;
  bcmx_lport_t dport;
  struct hsl_eth_header *eth;

  s = (struct sockaddr_mld *)msg->msg_name;

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
  pkt = hsl_bcm_tx_pkt_alloc (3);
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

#ifdef HAVE_IPNET
  pkt->pkt_data[1].data = hsl_ipnet_add_ipv6_hdr ((struct Ip_msghdr *)msg,
                                                   &ip6len, s->ip6_dst);
  pkt->pkt_data[1].len =  ip6len;
#else
  pkt->pkt_data[1].data = hsl_add_ipv6_hdr (msg, &ip6len, s->ip6_dst);
  pkt->pkt_data[1].len =  ip6len;
#endif

  /* Copy from iov's */
  ret = memcpy_fromiovec (buf, msg->msg_iov, len);
  if (ret < 0)
    {
      ret = -ENOMEM;
      goto RET;
    }

  /* Set body. */
  pkt->pkt_data[2].data = buf;
  pkt->pkt_data[2].len = len;

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

  eth->d.vlan.type = htons (HSL_ETHER_TYPE_IPV6);

  /* Send packet. */
  ret = hsl_bcm_pkt_send (pkt, dport, tagged);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error sending MLD_SNOOP frame out on dport(%d)\n", dport);
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
_mld_snoop_sock_recvmsg (struct kiocb *iocb, struct socket *sock, 
                   struct msghdr *msg, size_t len, int flags)
#else
_mld_snoop_sock_recvmsg (struct socket *sock, struct msghdr *msg, int len,
                          int flags, struct scm_cookie *scm)
#endif
{
  int size = sizeof (struct sockaddr_vlan);
  struct hal_in6_header *ipv6hdr;
  struct sock *sk = sock->sk;
  struct sockaddr_mld skml;
  struct sk_buff *skb;
  int offset;
  int copied;
  int ret, ret1;

  if (! sk)
    return -EINVAL;

  /* Receive one msg from the queue. */
  skb = skb_recv_datagram (sk, flags, flags & MSG_DONTWAIT, &ret);
  if (! skb)
    return -EINVAL;

  /* Copy sockaddr_vlan. */
  if (msg->msg_name)
    memcpy (msg->msg_name, (struct sockaddr_vlan *) skb->data, size);

  msg->msg_namelen = sizeof (struct sockaddr_mld);
  offset = size;

  memcpy (&skml, skb->data, size);

  ipv6hdr = (struct hal_in6_header *) (skb->data + size);

  if (msg->msg_name)
    {
      memcpy (&skml.ip6_src, &ipv6hdr->ip6_src, sizeof (struct hal_in6_addr));
      memcpy (msg->msg_name, &skml, sizeof (struct sockaddr_mld));
    }


#ifdef HAVE_IPNET
  ret1 = hsl_ipnet_put_cmsg ((struct Ip_msghdr *) msg,
                            skb->data + size, skml.port);
#else
  ret1 = hsl_put_cmsg (msg, (char*)skb->data + size, skml.port);
#endif

  offset += sizeof (struct hal_in6_header);
  offset += (skb->data [offset + 1] + 1) << 3;

  /* Did user send lesser buffer? */
  copied = skb->len - offset;
  if (copied > len)
    {
      copied = len;
      msg->msg_flags |= MSG_TRUNC;
    }

  /* Copy message. */
  ret = skb_copy_datagram_iovec (skb, offset, msg->msg_iov, copied);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR, "Error %d\n", ret);
      skb_free_datagram (sk, skb);
      return ret;
    }

  sock_recv_timestamp (msg, sk, skb);

  /* Free. */
  skb_free_datagram (sk, skb);

  if (ret1 < 0)
    {
      flags &= ~MSG_PEEK;
      /* Receive one msg from the queue. */
      skb = skb_recv_datagram (sk, flags, flags & MSG_DONTWAIT, &ret);
      if (! skb)
        return -EINVAL;

      /* Free. */
      skb_free_datagram (sk, skb);
    }

  return copied;
}

/* Post packet. */
int 
hsl_af_mlds_post_packet (struct sk_buff *skb)
{
  struct sock * sk; 
  struct sk_buff *skbc;

  /* Read lock. */
  read_lock_bh (&_mld_snoop_socklist_lock);

  /* Is there room to schedule the packet */
  HSL_SOCK_LIST_LOOP(sk, _mld_snoop)
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
  read_unlock_bh (&_mld_snoop_socklist_lock);

  return 0;
}

/* HSL socket initialization. */
int
hsl_af_mlds_sock_init (void)
{
  sock_register (&mld_snoop_family_ops);
  return 0;
}

/* HSL socket deinitialization. */
int
hsl_af_mlds_sock_deinit (void)
{
  sock_unregister (AF_MLD_SNOOP);
  return 0;
}

#endif

