/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "pal_memory.h"

/* Hack for GNU libc version 2. */
#ifndef MSG_TRUNC
#define MSG_TRUNC      0x20
#endif /* MSG_TRUNC */

#include "lib.h"
#include "thread.h"
#include "linklist.h"
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "table.h"
#include "nexthop.h"
#include "nsm_message.h"

#include "nsm/nsmd.h"
#ifdef HAVE_L3
#include "rib/rib.h"
#include "nsm/nsm_connected.h"
#include "nsm/rib/nsm_redistribute.h"
#endif /* HAVE_L3 */
#include "nsm/nsm_interface.h"
#include "nsm/nsm_debug.h"
#include "nsm/nsm_server.h"
#include "nsm/nsm_api.h"

#ifdef HAVE_CRX
#include "crx_vip.h"
#endif /* HAVE_CRX. */

#ifdef HAVE_HA
#include "lib_cal.h"
#endif /* HAVE_HA */
#ifdef HAVE_GMPLS
#include "nsm/gmpls/nsm_gmpls.h"
#endif /* HAVE_GMPLS */

#ifdef HAVE_MULTIPLE_FIB
void ifreq_set_name (struct ifreq *, struct interface *);
int if_ioctl (int, caddr_t);
#endif /* HAVE_MULTIPLE_FIB */
int kernel_read (struct thread *thread);

#ifndef RTA_VR
#define RTA_VR 16
#endif

#ifndef SO_VR
#define SO_VR 0x1020
#endif

#ifndef RTA_TABLE_NAME
#define RTA_TABLE_NAME 18
#endif

#ifndef NDA_RTA
#define NDA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif
#ifndef IFA_RTA
#define IFA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#endif

#ifndef NDA_RTA
#define NDA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif

/* Socket interface to kernel. */
struct nlsock
{
  int sock;
  int seq;
  struct sockaddr_nl snl;
  char *name;
  struct thread *t_read;
};

/* Kernel messages. */
struct nlsock *netlink_listen[FIB_ID_MAX];

/* Command channel. */
struct nlsock netlink_cmd  = { -1, 0, {0}, "netlink-cmd", NULL };

/* if-arbiter command channel. */
struct nlsock netlink_poll = { -1, 0, {0}, "netlink-poll", NULL };

struct message nlmsg_str[] =
  {
    {RTM_NEWROUTE, "RTM_NEWROUTE"},
    {RTM_DELROUTE, "RTM_DELROUTE"},
    {RTM_GETROUTE, "RTM_GETROUTE"},
    {RTM_NEWLINK,  "RTM_NEWLINK"},
    {RTM_DELLINK,  "RTM_DELLINK"},
    {RTM_GETLINK,  "RTM_GETLINK"},
    {RTM_NEWADDR,  "RTM_NEWADDR"},
    {RTM_DELADDR,  "RTM_DELADDR"},
    {RTM_GETADDR,  "RTM_GETADDR"},
    {RTM_NEWRULE,  "RTM_NEWRULE"},
    {RTM_DELRULE,  "RTM_DELRULE"},
    {0,            NULL}
  };

/* Prototype. */
int netlink_talk (struct nlmsghdr *, struct nlsock *);

unsigned long netlink_poll_flags = 0;
#define NETLINK_POLL_IPV4_ADDRESS       (1 << 0)
#define NETLINK_POLL_IPV6_ADDRESS       (1 << 1)

extern result_t
pal_kernel_if_flags_get_fib (struct interface *ifp, fib_id_t fib_id);
extern result_t
pal_kernel_if_get_bw_fib (struct interface *ifp, fib_id_t fib_id);

fib_id_t
netlink_rtmsg_fib_get (struct rtmsg *rtm)
{
#ifdef HAVE_MULTIPLE_FIB
  return (fib_id_t) rtm->rtm_vrf;
#endif /* HAVE_MULTIPLE_FIB */

  if (rtm->rtm_table != RT_TABLE_MAIN &&
      rtm->rtm_table != RT_TABLE_LOCAL)
    return (fib_id_t) rtm->rtm_table;

  return 0;
}

int
netlink_rtmsg_fib_set (struct rtmsg *rtm, fib_id_t fib)
{
#ifdef HAVE_MULTIPLE_FIB
  rtm->rtm_vrf = fib;
  rtm->rtm_table = RT_TABLE_MAIN;
#endif /* HAVE_MULTIPLE_FIB */

  return 0;
}



/* Make socket for Linux netlink interface. */
static int
netlink_socket (struct nlsock *nl, unsigned long groups, u_char non_block)
{
  int ret;
  struct sockaddr_nl snl;
  int sock;
  int namelen;

  sock = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "Can't open %s socket: %s", nl->name,
                  strerror (errno));
      return -1;
    }

  if (non_block)
    {
      ret = fcntl (sock, F_SETFL, O_NONBLOCK);
      if (ret < 0)
        {
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "Can't set %s socket flags: %s", nl->name,
                      strerror (errno));
          close (sock);
          return -1;
        }
    }

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;
  snl.nl_groups = groups;

  /* Bind the socket to the netlink structure for anything. */
  ret = bind (sock, (struct sockaddr *) &snl, sizeof snl);
  if (ret < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "Can't bind %s socket to group 0x%x: %s", nl->name,
                  snl.nl_groups, strerror (errno));
      close (sock);
      return -1;
    }

  /* multiple netlink sockets will have different nl_pid */
  namelen = sizeof snl;
  ret = getsockname (sock, (struct sockaddr *) &snl, &namelen);
  if (ret < 0 || namelen != sizeof snl)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "Can't get %s socket name: %s", nl->name,
                  strerror (errno));
      close (sock);
      return -1;
    }

  nl->snl = snl;
  nl->sock = sock;
  return ret;
}

/* Initialize and bind netlink listen socket to a FIB */
int
netlink_listen_sock_init (fib_id_t fib)
{
  struct nlsock *nl;
  char *sock_name = "netlink-listen";
  u_int8_t name_sz;
  unsigned long groups;
  int ret;

  groups = RTMGRP_LINK|RTMGRP_IPV4_ROUTE|RTMGRP_IPV4_IFADDR;
#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      groups |= RTMGRP_IPV6_ROUTE|RTMGRP_IPV6_IFADDR;
    }
#endif /* HAVE_IPV6 */

  if (fib >= FIB_ID_MAX)
    return -1;

  nl = XCALLOC (MTYPE_TMP, sizeof (struct nlsock));
  if (nl == NULL)
    return -1;

  name_sz = pal_strlen (sock_name) + 5;
  nl->name = XCALLOC (MTYPE_TMP, name_sz);
  if (nl->name == NULL)
    {
      XFREE (MTYPE_TMP, nl);
      return -1;
    }

  nl->sock = -1;
  pal_snprintf (nl->name, name_sz, "%s-%d", sock_name, fib);

  /* Create netlink socket */
  ret = netlink_socket (nl, groups, 1);
  if (ret < 0)
    {
      XFREE (MTYPE_TMP, nl->name);
      XFREE (MTYPE_TMP, nl);
      return -1;
    }

  /* Bind socket to FIB */
  ret = pal_sock_set_bindtofib (nl->sock, fib);
  if (ret < 0)
    {
      close (nl->sock);
      XFREE (MTYPE_TMP, nl->name);
      XFREE (MTYPE_TMP, nl);
      return -1;
    }

  /* Register kernel socket. */
  nl->t_read = thread_add_read (NSM_ZG, kernel_read, nl, nl->sock);


  netlink_listen[fib] = nl;

  return 0;
}

/* Uninitialize and unbind netlink listen socket from a FIB */
int
netlink_listen_sock_uninit (fib_id_t fib)
{
  struct nlsock *nl;

  if (fib >= FIB_ID_MAX)
    return -1;

  if ((nl = netlink_listen[fib]) == NULL)
    return -1;

  THREAD_READ_OFF (nl->t_read);

  close (nl->sock);

  XFREE (MTYPE_TMP, nl->name);
  XFREE (MTYPE_TMP, nl);

  netlink_listen[fib] = NULL;

  return 0;
}

/* Get type specified information from netlink. */
static int
netlink_request (int family, int type, struct nlsock *nl)
{
  int ret;
  struct sockaddr_nl snl;

  struct
  {
    struct nlmsghdr nlh;
    struct rtgenmsg g;
  } req;

  /* Check netlink socket. */
  if (nl->sock < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "%s socket isn't active.", nl->name);
      return -1;
    }

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  req.nlh.nlmsg_len = sizeof req;
  req.nlh.nlmsg_type = type;
  req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
  req.nlh.nlmsg_pid = 0;
  req.nlh.nlmsg_seq = ++nl->seq;
  req.g.rtgen_family = family;

  ret = sendto (nl->sock, (void*) &req, sizeof req, 0,
                (struct sockaddr*) &snl, sizeof snl);
  if (ret < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "%s sendto failed: %s", nl->name, strerror (errno));
      return -1;
    }
  return 0;
}

/* Receive message from netlink interface and pass those information
   to the given function. */
static int
netlink_parse_info (int (*filter) (struct sockaddr_nl *, struct nlmsghdr *),
                    struct nlsock *nl)
{
  int status;
  int ret = 0;
  int error;

  while (1)
    {
      char buf[4096];
      struct iovec iov = { buf, sizeof buf };
      struct sockaddr_nl snl;
      struct msghdr msg = { (void*)&snl, sizeof snl, &iov, 1, NULL, 0, 0};
      struct nlmsghdr *h;

      status = recvmsg (nl->sock, &msg, 0);

      if (status < 0)
        {
          if (errno == EINTR)
            continue;
          if (errno == EWOULDBLOCK)
            break;
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "%s recvmsg overrun: %s", nl->name,
                      strerror (errno));
          continue;
        }

      if (snl.nl_pid != 0)
        {
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "Ignoring non kernel message from pid %u",
                      snl.nl_pid);
          continue;
        }

      if (status == 0)
        {
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "%s EOF", nl->name);
          return -1;
        }

      if (msg.msg_namelen != sizeof snl)
        {
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "%s sender address length error: length %d",
                      nl->name, msg.msg_namelen);
          return -1;
        }

      for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, status);
           h = NLMSG_NEXT (h, status))
        {
          /* Finish of reading. */
          if (h->nlmsg_type == NLMSG_DONE)
            return ret;

          /* Error handling. */
          if (h->nlmsg_type == NLMSG_ERROR)
            {
              struct nlmsgerr *err = (struct nlmsgerr *) NLMSG_DATA (h);

              /* Sometimes the nlmsg_type is NLMSG_ERROR but the err->error
               *                  is 0. This is a success. */
              if (err->error == 0)
                {
                  if (IS_NSM_DEBUG_KERNEL)
                    zlog_info (NSM_ZG,
                               "%s: %s ACK: type=%s(%u), seq=%u, pid=%d",
                               __FUNCTION__, nl->name,
                               lookup (nlmsg_str, err->msg.nlmsg_type),
                               err->msg.nlmsg_type, err->msg.nlmsg_seq,
                               err->msg.nlmsg_pid);

                  /* return if not a multipart message, otherwise continue */
                  if (!(h->nlmsg_flags & NLM_F_MULTI))
                    {
                      return 0;
                    }
                  continue;
                }

              if (h->nlmsg_len < NLMSG_LENGTH (sizeof (struct nlmsgerr)))
                {
                  if (IS_NSM_DEBUG_KERNEL)
                    zlog_err (NSM_ZG, "%s error: message truncated", nl->name);
                  return -1;
                }

              if (IS_NSM_DEBUG_KERNEL)
                zlog_err (NSM_ZG, "%s error: %s(%d), type=%s(%u), seq=%u, pid=%d",
                          nl->name, strerror (-err->error), err->error,
                          lookup (nlmsg_str, err->msg.nlmsg_type),
                          err->msg.nlmsg_type, err->msg.nlmsg_seq,
                          err->msg.nlmsg_pid);
              return err->error;
            }

          /* OK we got netlink message. */
          if (IS_NSM_DEBUG_KERNEL)
            zlog_info (NSM_ZG,
                       "netlink_parse_info: %s type %s(%u), seq=%u, pid=%d",
                       nl->name, lookup (nlmsg_str, h->nlmsg_type),
                       h->nlmsg_type, h->nlmsg_seq, h->nlmsg_pid);

          /* Skip unsolicited messages originating from command socket. */
          if (nl != &netlink_cmd && h->nlmsg_pid == netlink_cmd.snl.nl_pid)
            {
              if (IS_NSM_DEBUG_KERNEL)
                zlog_info (NSM_ZG,
                           "netlink_parse_info: %s packet comes from %s",
                           nl->name, netlink_cmd.name);
              continue;
            }

          error = (*filter) (&snl, h);
          if (error < 0)
            {
              if (IS_NSM_DEBUG_KERNEL)
                zlog_err (NSM_ZG, "%s filter function error", nl->name);
              ret = error;
            }
        }

      /* After error care. */
      if (msg.msg_flags & MSG_TRUNC)
        {
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "%s error: message truncated", nl->name);
          continue;
        }
      if (status)
        {
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "%s error: data remnant size %d",
                      nl->name, status);
          return -1;
        }
    }
  return ret;
}

/* Utility function for parse rtattr. */
static void
netlink_parse_rtattr (struct rtattr **tb, int max, struct rtattr *rta, int len)
{
  while (RTA_OK(rta, len))
    {
      if (rta->rta_type <= max)
        tb[rta->rta_type] = rta;
      rta = RTA_NEXT(rta,len);
    }
}

void
netlink_hw_addr_update (struct interface *ifp, struct rtattr **tb)
{
  int i;
  int hw_addr_len;

  hw_addr_len = RTA_PAYLOAD(tb[IFLA_ADDRESS]);

  if (hw_addr_len > INTERFACE_HWADDR_MAX)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_warn (NSM_ZG, "Hardware address is too large: %d", hw_addr_len);
    }
  else
    {
      ifp->hw_addr_len = hw_addr_len;
      memcpy (ifp->hw_addr, RTA_DATA(tb[IFLA_ADDRESS]), hw_addr_len);

      for (i = 0; i < hw_addr_len; i++)
        if (ifp->hw_addr[i] != 0)
          break;

      if (i == hw_addr_len)
        ifp->hw_addr_len = 0;
      else
        ifp->hw_addr_len = hw_addr_len;
    }
}

#ifdef HAVE_MULTIPLE_FIB
int
netlink_if_bind_update (struct interface *ifp, struct rtattr *tb)
{
  fib_id_t fib_id;

  /* Read FIB ID.  */
  fib_id = *(int *) RTA_DATA (tb);

  /* Set the NSM FIB ID to the kernel.  */
  if (fib_id != ifp->vrf->fib_id)
    pal_kernel_if_bind_vrf (ifp, ifp->vrf->fib_id);

  return 0;
}
#endif /* HAVE_MULTIPLE_FIB */

int netlink_new_link (struct sockaddr_nl *, struct nlmsghdr *);

/* Called from interface_update_netlink(). */
int
netlink_link_update (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  struct ifinfomsg *ifi;
  struct interface *ifp;

  ifi = NLMSG_DATA (h);

  if (h->nlmsg_type != RTM_NEWLINK)
    return 0;

  if (netlink_new_link (snl, h) < 0)
    return 0;

  ifp = ifg_lookup_by_index (&NSM_ZG->ifg, ifi->ifi_index);
  if (ifp != NULL)
    SET_FLAG (ifp->status, NSM_INTERFACE_ARBITER);

  return 0;
}

#ifdef HAVE_L3
/* Update interface IPv4/IPv6 address. */
int
netlink_addr_update (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len;
  struct ifaddrmsg *ifa;
  struct rtattr *tb [IFA_MAX + 1];
  struct interface *ifp;
  struct connected *ifc;
  void *addr = NULL;
  void *broad = NULL;
  u_char flags = 0;

  ifa = NLMSG_DATA (h);

  if (ifa->ifa_family != AF_INET
#ifdef HAVE_IPV6
      && (!NSM_CAP_HAVE_IPV6 || ifa->ifa_family != AF_INET6)
#endif /* HAVE_IPV6 */
      )
    return 0;

  if ((h->nlmsg_type != RTM_NEWADDR) && (h->nlmsg_type != RTM_DELADDR))
    return 0;

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof (struct ifaddrmsg));
  if (len < 0)
    return -1;

  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, IFA_MAX, IFA_RTA (ifa), len);

  ifp = ifg_lookup_by_index (&NSM_ZG->ifg, ifa->ifa_index);
  if (ifp == NULL)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG,
                  "netlink_interface_addr can't find interface by index %d",
                  ifa->ifa_index);
      return -1;
    }

  if (tb[IFA_ADDRESS] == NULL)
    tb[IFA_ADDRESS] = tb[IFA_LOCAL];

  if (ifp->flags & IFF_POINTOPOINT)
    {
      if (tb[IFA_LOCAL])
        {
          addr = RTA_DATA (tb[IFA_LOCAL]);
          if (tb[IFA_ADDRESS])
            broad = RTA_DATA (tb[IFA_ADDRESS]);
          else
            broad = NULL;
        }
      else
        {
          if (tb[IFA_ADDRESS])
            addr = RTA_DATA (tb[IFA_ADDRESS]);
          else
            addr = NULL;
        }
    }
  else
    {
      if (tb[IFA_ADDRESS])
        addr = RTA_DATA (tb[IFA_ADDRESS]);
      else
        addr = NULL;

      if (tb[IFA_BROADCAST])
        broad = RTA_DATA(tb[IFA_BROADCAST]);
      else
        broad = NULL;
    }

  /* Register interface address to the interface. */
  if (ifa->ifa_family == AF_INET)
    {
      struct prefix_ipv4 p;

      p.family = AF_INET;
      p.prefixlen = ifa->ifa_prefixlen;
      IPV4_ADDR_COPY (&p.prefix, addr);

      if (IN_MULTICAST (pal_ntoh32 (p.prefix.s_addr)))
        return 0;

      ifc = nsm_connected_real_ipv4 (ifp, (struct prefix *)&p);
#ifdef HAVE_NSM_IF_ARBITER
      if (ng->t_if_arbiter)
        {
          if (ifc == NULL)
            if (h->nlmsg_type == RTM_NEWADDR)
              ifc = nsm_connected_add_ipv4 (ifp, flags, (struct pal_in4_addr *) addr,
                                            ifa->ifa_prefixlen,
                                            (struct pal_in4_addr *) broad);
          if (ifc != NULL)
            {
              SET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
              lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */
            }
        }
      else
#endif /* HAVE_NSM_IF_ARBITER */
        {
          if (h->nlmsg_type == RTM_NEWADDR)
            ifc = nsm_connected_add_ipv4 (ifp, flags, (struct pal_in4_addr *) addr,                                            ifa->ifa_prefixlen,
                                          (struct pal_in4_addr *) broad);
          else
            nsm_connected_delete_ipv4  (ifp, flags, (struct pal_in4_addr *) addr,
                                        ifa->ifa_prefixlen,
                                        (struct pal_in4_addr *) broad);
        }
    }
#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      if (ifa->ifa_family == AF_INET6)
        {
          struct prefix_ipv6 p;

          p.family = AF_INET6;
          p.prefixlen = ifa->ifa_prefixlen;
          IPV6_ADDR_COPY (&p.prefix, addr);

          /* Netlink may pass IPv6 multicast address. We must skip it */
          if (IN6_IS_ADDR_MULTICAST (&p.prefix))
            return 0;

          ifc = nsm_connected_real_ipv6 (ifp, (struct prefix *)&p);
#ifdef HAVE_NSM_IF_ARBITER
          if (ng->t_if_arbiter)
            {
              if (ifc == NULL)
                if (h->nlmsg_type == RTM_NEWADDR)
                  ifc = nsm_connected_add_ipv6 (ifp, (struct pal_in6_addr *) addr,
                                                ifa->ifa_prefixlen,
                                                (struct pal_in6_addr *) broad);
              if (ifc != NULL)
                {
                  SET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
                  lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */
                }
            }
          else
#endif /* HAVE_NSM_IF_ARBITER */
            {
              if (h->nlmsg_type == RTM_NEWADDR)
                {
                  ifc = nsm_connected_add_ipv6 (ifp, 
                                                (struct pal_in6_addr *) addr,
                                                 ifa->ifa_prefixlen,
                                                (struct pal_in6_addr *) broad);
                  if (ifc == NULL)
                    return -1;
                }
              else
                nsm_connected_delete_ipv6 (ifp, (struct pal_in6_addr *) addr,
                                           ifa->ifa_prefixlen,
                                           (struct pal_in6_addr *) broad);
            }
        }
    }
#endif /* HAVE_IPV6*/

  return 0;
}

/* Looking up routing table by netlink interface. */
int
netlink_routing_table (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len;
  struct rtmsg *rtm;
  struct rtattr *tb [RTA_MAX + 1];
  u_char flags = 0;
  u_int32_t metric = 0;
  char anyaddr[16] = {0};
  fib_id_t fib;
  struct nsm_vrf *vrf;
  struct nlsock *nl;
  int ret;

  int index;
  int table;
  void *dest;
  void *gate;

  rtm = NLMSG_DATA (h);

  if (h->nlmsg_type != RTM_NEWROUTE)
    return 0;

  if (rtm->rtm_type != RTN_UNICAST
      && rtm->rtm_type != RTN_BLACKHOLE)
    return 0;

  table = rtm->rtm_table;

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof (struct rtmsg));
  if (len < 0)
    return -1;

  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, RTA_MAX, RTM_RTA (rtm), len);

  if (rtm->rtm_flags & RTM_F_CLONED)
    return 0;
  if (rtm->rtm_protocol == RTPROT_REDIRECT
      || rtm->rtm_protocol == RTPROT_KERNEL)
    return 0;
  if (rtm->rtm_src_len != 0)
    return 0;

  /* Route which inserted by Zebra. */
  if (rtm->rtm_protocol == RTPROT_ZEBRA)
    flags |= RIB_FLAG_SELFROUTE;

  index = 0;
  dest = NULL;
  gate = NULL;

  if (tb[RTA_OIF])
    index = *(int *) RTA_DATA (tb[RTA_OIF]);

  if (tb[RTA_DST])
    dest = RTA_DATA (tb[RTA_DST]);
  else
    dest = anyaddr;

  /* Metric.  */
  if (tb[RTA_PRIORITY])
    metric = *(u_int32_t *)RTA_DATA (tb[RTA_PRIORITY]);

  /* Multipath treatment is needed. */
  if (tb[RTA_GATEWAY])
    gate = RTA_DATA (tb[RTA_GATEWAY]);

  /* Bug in Linux kernel.
     For kernel ECMP routes, the kernel doesn't send the ifindex or the
     gateway. Ignore these routes to avoid crash.
  */
  if (!index && !gate
      && rtm->rtm_type != RTN_BLACKHOLE)
    return 0;

  /* Resolve FIB ID. */
  fib = netlink_rtmsg_fib_get (rtm);

  /* Lookup corresponding vrf. */
  vrf = nsm_vrf_lookup_by_fib_id (NSM_ZG, fib);
  if (! vrf)
    {
      /* Clean-up 'zebra' routes for VRFs not defined in NSM. */
      if (rtm->rtm_protocol == RTPROT_ZEBRA)
        {
          if (rtm->rtm_family == AF_INET)
            nl = &netlink_cmd;
          else
            {
              if ((fib >= FIB_ID_MAX) ||
                  ((nl = netlink_listen[fib]) == NULL))
                return -1;
            }

          h->nlmsg_type = RTM_DELROUTE;
          h->nlmsg_flags = NLM_F_REQUEST;
          ret = netlink_talk (h, nl);
          if (ret < 0)
            return ret;
        }

      return 0;
    }

  if (rtm->rtm_family == AF_INET)
    {
      struct prefix_ipv4 p;

      p.family = AF_INET;
      memcpy (&p.prefix, dest, 4);
      p.prefixlen = rtm->rtm_dst_len;

      /* Add the route. */
      nsm_rib_add_ipv4 (APN_ROUTE_KERNEL, &p, index, gate,
                        flags, metric, fib);
    }

#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      if (rtm->rtm_family == AF_INET6)
        {
          struct prefix_ipv6 p;
          p.family = AF_INET6;
          memcpy (&p.prefix, dest, 16);
          p.prefixlen = rtm->rtm_dst_len;

          nsm_rib_add_ipv6 (APN_ROUTE_KERNEL, &p, index, gate,
                            flags, metric, fib);
        }
    }
#endif /* HAVE_IPV6 */

  return 0;
}
#endif /* HAVE_L3 */

struct message rtproto_str [] =
  {
    {RTPROT_REDIRECT, "redirect"},
    {RTPROT_KERNEL,   "kernel"},
    {RTPROT_BOOT,     "boot"},
    {RTPROT_STATIC,   "static"},
    {RTPROT_GATED,    "GateD"},
    {RTPROT_RA,       "router advertisement"},
    {RTPROT_MRT,      "MRT"},
    {RTPROT_ZEBRA,    "Zebra"},
#ifdef RTPROT_BIRD
    {RTPROT_BIRD,     "BIRD"},
#endif /* RTPROT_BIRD */
    {0,               NULL}
  };

#ifdef HAVE_L3
/* New route from the kernel. */
int
netlink_new_route (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  struct nsm_vrf *vrf;
  struct rtmsg *rtm;
  struct rtattr *tb [RTA_MAX + 1];
  u_int32_t metric = 0;
  int len;
  char anyaddr[16] = {0};
  int index;
  void *dest;
  void *gate;
  fib_id_t fib;

  rtm = NLMSG_DATA (h);

  /* Connected route. */
  if (IS_NSM_DEBUG_KERNEL)
    zlog_info (NSM_ZG, "RTM_NEWROUTE %s %s proto %s",
               rtm->rtm_family == AF_INET ? "ipv4" : "ipv6",
               rtm->rtm_type == RTN_UNICAST ? "unicast" : "multicast",
               lookup (rtproto_str, rtm->rtm_protocol));

  if (rtm->rtm_type != RTN_UNICAST)
    return 0;

  /* Resolve FIB ID. */
  fib = netlink_rtmsg_fib_get (rtm);

  /* Lookup corresponding vrf. */
  vrf = nsm_vrf_lookup_by_fib_id (NSM_ZG, fib);
  if (! vrf)
    return 0;

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof (struct rtmsg));
  if (len < 0)
    return -1;

  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, RTA_MAX, RTM_RTA (rtm), len);

  /* Flag check. */
  if (rtm->rtm_flags & RTM_F_CLONED)
    return 0;

  /* Protocol check. */
  if (rtm->rtm_protocol == RTPROT_REDIRECT
      || rtm->rtm_protocol == RTPROT_KERNEL
      || rtm->rtm_protocol == RTPROT_ZEBRA)
    return 0;

  if (rtm->rtm_src_len != 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_warn (NSM_ZG, "RTM_NEWROUTE: no src len");
      return 0;
    }

  index = 0;
  dest = NULL;
  gate = NULL;

  if (tb[RTA_OIF])
    index = *(int *) RTA_DATA (tb[RTA_OIF]);

  if (tb[RTA_DST])
    dest = RTA_DATA (tb[RTA_DST]);
  else
    dest = anyaddr;

  /* Metric.  */
  if (tb[RTA_PRIORITY])
    metric = *(u_int32_t *)RTA_DATA (tb[RTA_PRIORITY]);

  if (tb[RTA_GATEWAY])
    gate = RTA_DATA (tb[RTA_GATEWAY]);

  /* Bug in Linux kernel.
     For kernel ECMP routes, the kernel doesn't send the ifindex or the
     gateway. Ignore these routes to avoid crash. */
  if (!index && !gate)
    return 0;

  if (rtm->rtm_family == AF_INET)
    {
      struct prefix_ipv4 p;
      p.family = AF_INET;
      memcpy (&p.prefix, dest, 4);
      p.prefixlen = rtm->rtm_dst_len;

      if (IS_NSM_DEBUG_KERNEL)
        zlog_info (NSM_ZG, "RTM_NEWROUTE %r/%d vrf=%d fib=%d",
                   &p.prefix, p.prefixlen, vrf->vrf->id, fib);

      nsm_rib_add_ipv4 (APN_ROUTE_KERNEL, &p, index,
                        gate, 0, metric, fib);
    }

#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      if (rtm->rtm_family == AF_INET6)
        {
          struct prefix_ipv6 p;
          char buf[BUFSIZ];

          p.family = AF_INET6;
          memcpy (&p.prefix, dest, 16);
          p.prefixlen = rtm->rtm_dst_len;

          if (IS_NSM_DEBUG_KERNEL)
            zlog_info (NSM_ZG, "RTM_NEWROUTE %s/%d vrf=%d fib=%d",
                       inet_ntop (AF_INET6, &p.prefix, buf, BUFSIZ),
                       p.prefixlen, vrf->vrf->id, fib);

          nsm_rib_add_ipv6 (APN_ROUTE_KERNEL, &p, index,
                            gate, 0, metric, fib);
        }
    }
#endif /* HAVE_IPV6 */

  return 0;
}

/* Routing information change from the kernel. */
int
netlink_del_route (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  struct nsm_vrf *vrf;
  struct rtmsg *rtm;
  struct rtattr *tb [RTA_MAX + 1];
  u_int32_t metric = 0;
  int len;
  char anyaddr[16] = {0};
  int index;
  void *dest;
  void *gate;
  fib_id_t fib;

  rtm = NLMSG_DATA (h);

  /* Connected route. */
  if (IS_NSM_DEBUG_KERNEL)
    zlog_info (NSM_ZG, "RTM_DELROUTE %s %s proto %s",
               rtm->rtm_family == AF_INET ? "ipv4" : "ipv6",
               rtm->rtm_type == RTN_UNICAST ? "unicast" : "multicast",
               lookup (rtproto_str, rtm->rtm_protocol));

  if (rtm->rtm_type != RTN_UNICAST)
    return 0;

  /* Resolve FIB ID. */
  fib = netlink_rtmsg_fib_get (rtm);

  /* Lookup corresponding vrf. */
  vrf = nsm_vrf_lookup_by_fib_id (NSM_ZG, fib);
  if (! vrf)
    return 0;

  len = h->nlmsg_len - NLMSG_LENGTH(sizeof (struct rtmsg));
  if (len < 0)
    return -1;

  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, RTA_MAX, RTM_RTA (rtm), len);

  /* Flag check. */
  if (rtm->rtm_flags & RTM_F_CLONED)
    return 0;

  /* Protocol check. */
  if (rtm->rtm_protocol == RTPROT_REDIRECT
      || rtm->rtm_protocol == RTPROT_KERNEL)
    return 0;

  if (rtm->rtm_src_len != 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_warn (NSM_ZG, "netlink_del_route(): no src len");
      return 0;
    }

  index = 0;
  dest = NULL;
  gate = NULL;

  if (tb[RTA_OIF])
    index = *(int *) RTA_DATA (tb[RTA_OIF]);

  if (tb[RTA_DST])
    dest = RTA_DATA (tb[RTA_DST]);
  else
    dest = anyaddr;

  /* Metric.  */
  if (tb[RTA_PRIORITY])
    metric = *(u_int32_t *)RTA_DATA (tb[RTA_PRIORITY]);

  if (tb[RTA_GATEWAY])
    gate = RTA_DATA (tb[RTA_GATEWAY]);

  /* Bug in Linux kernel.
     For kernel ECMP routes, the kernel doesn't send the ifindex or the
     gateway. Ignore these routes to avoid crash.
  */
  if (!index && !gate)
    return 0;

  if (rtm->rtm_family == AF_INET)
    {
      struct prefix_ipv4 p;
      p.family = AF_INET;
      memcpy (&p.prefix, dest, 4);
      p.prefixlen = rtm->rtm_dst_len;

      if (IS_NSM_DEBUG_KERNEL)
        {
          char tb[50];

          pal_inet_ntoa (p.prefix, tb);
          zlog_info (NSM_ZG, "RTM_DELROUTE %s/%d vrf=%d fib=%d",
                     tb, p.prefixlen, vrf->vrf->id, fib);
        }

      nsm_rib_delete_ipv4 (APN_ROUTE_KERNEL, &p, index, gate, fib);
    }

#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      if (rtm->rtm_family == AF_INET6)
        {
          struct prefix_ipv6 p;

          p.family = AF_INET6;
          memcpy (&p.prefix, dest, 16);
          p.prefixlen = rtm->rtm_dst_len;

          if (IS_NSM_DEBUG_KERNEL)
            zlog_info (NSM_ZG, "RTM_DELROUTE %R/%d", &p.prefix, p.prefixlen);

          nsm_rib_delete_ipv6 (APN_ROUTE_KERNEL, &p, index, gate, fib);
        }
    }
#endif /* HAVE_IPV6 */

  return 0;
}
#endif /* HAVE_L3 */

int
netlink_new_link (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len;
  struct ifinfomsg *ifi;
  struct rtattr *tb [IFLA_MAX + 1];
  struct interface *ifp;
  struct nsm_vrf *nvrf;
  char *name;
  u_int16_t fib_id = FIB_ID_MAIN;
  u_int16_t hw_type;

  ifi = NLMSG_DATA (h);

  len = h->nlmsg_len - NLMSG_LENGTH (sizeof (struct ifinfomsg));
  if (len < 0)
    return -1;

  /* Looking up interface name. */
  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, IFLA_MAX, IFLA_RTA (ifi), len);
  if (tb[IFLA_IFNAME] == NULL)
    return -1;
  name = (char *)RTA_DATA(tb[IFLA_IFNAME]);

#ifdef HAVE_IPNET
  if (tb[IFLA_VR])
    fib_id = *(u_int16_t *)RTA_DATA (tb[IFLA_VR]);
#endif /* HAVE_IPNET */

  nvrf = nsm_vrf_lookup_by_fib_id (NSM_ZG, fib_id);
  if (nvrf == NULL)
    return -1;

  /* Ignore loopback interfaces on non-default VRFs */
  if (((hw_type = pal_if_type (ifi->ifi_type)) == IF_TYPE_LOOPBACK) &&
      (nvrf->vrf->name != NULL))
    return 0;

  ifp = ifg_lookup_by_name (&NSM_ZG->ifg, name);
  if (ifp == NULL || ! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    {
      if (ifp == NULL)
        ifp = ifg_get_by_name (&NSM_ZG->ifg, name);
      else if (ifp->ifindex != ifi->ifi_index)
        {
          /* Update the interface index.  */
          nsm_if_ifindex_update (ifp, ifi->ifi_index);
        }

      ifp->ifindex = ifi->ifi_index;
      ifp->mtu = *(int *)RTA_DATA (tb[IFLA_MTU]);
      ifp->metric = 1;
      ifp->hw_type = hw_type;

      if (tb[IFLA_ADDRESS])
        netlink_hw_addr_update (ifp, tb);

#ifdef HAVE_GMPLS
      if (ifp->gifindex == 0)
        {
          ifp->gifindex = NSM_GMPLS_GIFINDEX_GET ();
        }
#endif /* HAVE_GMPLS */

      /* Add to database. */
      ifg_table_add (&NSM_ZG->ifg, ifp->ifindex, ifp);

      pal_kernel_if_get_bw_fib (ifp, fib_id);
      pal_kernel_if_flags_get_fib (ifp, fib_id);

      /* Notice NSM a new interface.  */
      nsm_if_add_update (ifp, (fib_id_t)fib_id);

#ifdef HAVE_MULTIPLE_FIB
      /* Update interface binding.  */
      if (tb[IFLA_VRF])
        netlink_if_bind_update (ifp, tb[IFLA_VRF]);
#endif /* HAVE_MULTIPLE_FIB */
    }
  else
    {
      int ifindex;
      int param_changed = 0;
      cindex_t cindex = 0;

      /* Interface status change. */
      ifindex = ifi->ifi_index;
      ifp->metric = 1;

      if (ifp->ifindex != ifindex)
        {
          /* Update the interface index.  */
          nsm_if_ifindex_update (ifp, ifindex);

          param_changed = 1;
        }
      if (tb[IFLA_MTU])
        {
          int mtu;
          mtu = *(int *)RTA_DATA (tb[IFLA_MTU]);
          if (ifp->mtu != mtu)
            {
              ifp->mtu = mtu;
              param_changed = 1;
              NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_MTU);
            }
        }

      if (tb[IFLA_ADDRESS])
        netlink_hw_addr_update (ifp, tb);

      if (if_is_up (ifp) && if_is_running (ifp))
        {
          pal_kernel_if_flags_get (ifp);

          if (! (if_is_up (ifp) && if_is_running (ifp)))
            {
              if (param_changed)
                nsm_server_if_down_update (ifp, cindex);
              else
                nsm_if_down (ifp);
            }
          else if (param_changed)
            nsm_server_if_up_update (ifp, cindex);
        }
      else
        {
          pal_kernel_if_flags_get (ifp);

          if (if_is_up (ifp) && if_is_running (ifp))
            {
              if (param_changed)
                nsm_server_if_up_update (ifp, cindex);
              else
                nsm_if_up (ifp);

#ifdef HAVE_L3
              /* Linux deletes all IPv6 addresses from a interface when
                 the interface is brought down. For addresses
                 configured from NSM, we reinstall the addresses
                 in the kernel. */
              nsm_if_addr_wakeup (ifp);
#endif /* HAVE_L3 */
            }
          else if (param_changed)
            nsm_server_if_down_update (ifp, cindex);
        }

#ifdef HAVE_MULTIPLE_FIB
      /* Update interface binding.  */
      if (tb[IFLA_VRF])
        netlink_if_bind_update (ifp, tb[IFLA_VRF]);
#endif /* HAVE_MULTIPLE_FIB */
    }
#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return 0;
}

int
netlink_del_link (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  int len;
  struct ifinfomsg *ifi;
  struct rtattr *tb [IFLA_MAX + 1];
  struct interface *ifp;
  char *name;

  ifi = NLMSG_DATA (h);

  len = h->nlmsg_len - NLMSG_LENGTH (sizeof (struct ifinfomsg));
  if (len < 0)
    return -1;

  /* Looking up interface name. */
  memset (tb, 0, sizeof tb);
  netlink_parse_rtattr (tb, IFLA_MAX, IFLA_RTA (ifi), len);
  if (tb[IFLA_IFNAME] == NULL)
    return -1;
  name = (char *)RTA_DATA(tb[IFLA_IFNAME]);

  /* RTM_DELLINK. */
  ifp = ifg_lookup_by_name (&NSM_ZG->ifg, name);
  if (ifp == NULL)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_warn (NSM_ZG, "interface %s is deleted but can't find", name);
      return 0;
    }

  nsm_if_delete_update (ifp);

  return 0;
}

#ifdef HAVE_L3
void
netlink_update_ipv4_address_set (void)
{
  SET_FLAG (netlink_poll_flags, NETLINK_POLL_IPV4_ADDRESS);
}

#ifdef HAVE_IPV6
void
netlink_update_ipv6_address_set (void)
{
  SET_FLAG (netlink_poll_flags, NETLINK_POLL_IPV6_ADDRESS);
}
#endif /* HAVE_IPV6 */

int
netlink_newdel_addr (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  struct ifaddrmsg *ifa;

  ifa = NLMSG_DATA (h);

  if (ifa->ifa_family == AF_INET)
    netlink_update_ipv4_address_set ();
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6 && ifa->ifa_family == AF_INET6)
    netlink_update_ipv6_address_set ();
#endif /* HAVE_IPV6 */

  /* Handle message directly only if if-arbiter is not enabled */
#ifdef HAVE_NSM_IF_ARBITER
  if (! ng->t_if_arbiter)
#endif /* HAVE_NSM_IF_ARBITER */
    netlink_addr_update (snl, h);
  return 0;
}
#endif /* HAVE_L3 */

int
netlink_information_fetch (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  switch (h->nlmsg_type)
    {
#ifdef HAVE_L3
    case RTM_NEWROUTE:
      return netlink_new_route (snl, h);
      break;
    case RTM_DELROUTE:
      return netlink_del_route (snl, h);
      break;
#endif /* HAVE_L3 */
    case RTM_NEWLINK:
      return netlink_new_link (snl, h);
      break;
    case RTM_DELLINK:
      return netlink_del_link (snl, h);
      break;
#ifdef HAVE_L3
    case RTM_NEWADDR:
    case RTM_DELADDR:
      return netlink_newdel_addr (snl, h);
      break;
#endif /* HAVE_L3 */
    default:
      if (IS_NSM_DEBUG_KERNEL)
        zlog_warn (NSM_ZG, "Unknown netlink nlmsg_type %d\n", h->nlmsg_type);
      break;
    }
  return 0;
}

/* Interface update by netlink socket. */
int
netlink_update_interface_all ()
{
  struct interface *ifp;
  struct connected *ifc;
  struct route_node *rn;
  int ret;

  /* Update interface information. */
  ret = netlink_request (AF_PACKET, RTM_GETLINK, &netlink_poll);
  if (ret < 0)
    return ret;
  ret = netlink_parse_info (netlink_link_update, &netlink_poll);
  if (ret < 0)
    return ret;

#ifdef HAVE_L3
  if (CHECK_FLAG (netlink_poll_flags, NETLINK_POLL_IPV4_ADDRESS))
    {
      /* Update IPv4 address of the interfaces. */
      ret = netlink_request (AF_INET, RTM_GETADDR, &netlink_poll);
      if (ret < 0)
        return ret;

      ret = netlink_parse_info (netlink_addr_update, &netlink_poll);
      if (ret < 0)
        return ret;

      UNSET_FLAG (netlink_poll_flags, NETLINK_POLL_IPV4_ADDRESS);
    }
  else
#endif /* HAVE_L3 */
    {
      for (rn = route_top (NSM_ZG->ifg.if_table); rn; rn = route_next (rn))
        if ((ifp = rn->info))
          for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
            {
              SET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
              lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */
            }
    }

#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      if (CHECK_FLAG (netlink_poll_flags, NETLINK_POLL_IPV6_ADDRESS))
        {
          /* Update IPv6 address of the interfaces. */
          ret = netlink_request (AF_INET6, RTM_GETADDR, &netlink_poll);
          if (ret < 0)
            return ret;
          ret = netlink_parse_info (netlink_addr_update, &netlink_poll);
          if (ret < 0)
            return ret;

          UNSET_FLAG (netlink_poll_flags, NETLINK_POLL_IPV6_ADDRESS);
        }
      else
        {
          for (rn = route_top (NSM_ZG->ifg.if_table); rn; rn = route_next (rn))
            if ((ifp = rn->info))
              for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
                {
                  SET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
                  lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */
                }
        }
    }
#endif /* HAVE_IPV6 */

  return 0;
}

#ifdef HAVE_L3
/* Routing table read function using netlink interface.  Only called
   bootstrap time. */
int
netlink_route_read ()
{
  int ret;

  /* Get IPv4 routing table. */
  ret = netlink_request (AF_INET, RTM_GETROUTE, &netlink_cmd);
  if (ret < 0)
    return ret;
  ret = netlink_parse_info (netlink_routing_table, &netlink_cmd);
  if (ret < 0)
    return ret;

#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      /* Get IPv6 routing table. */
      ret = netlink_request (AF_INET6, RTM_GETROUTE, &netlink_cmd);
      if (ret < 0)
        return ret;
      ret = netlink_parse_info (netlink_routing_table, &netlink_cmd);
      if (ret < 0)
        return ret;
    }
#endif /* HAVE_IPV6 */

  return 0;
}
#endif /* HAVE_L3 */

int
addattr_l (struct nlmsghdr *n, int maxlen, int type, void *data, int alen)
{
  int len;
  struct rtattr *rta;

  len = RTA_LENGTH(alen);

  if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr*) (((char*)n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA(rta), data, alen);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  return 0;
}

int
rta_addattr_l (struct rtattr *rta, int maxlen, int type, void *data, int alen)
{
  int len;
  struct rtattr *subrta;

  len = RTA_LENGTH(alen);

  if (RTA_ALIGN(rta->rta_len) + len > maxlen)
    return -1;

  subrta = (struct rtattr*) (((char*)rta) + RTA_ALIGN (rta->rta_len));
  subrta->rta_type = type;
  subrta->rta_len = len;
  memcpy (RTA_DATA(subrta), data, alen);
  rta->rta_len = NLMSG_ALIGN (rta->rta_len) + len;

  return 0;
}

int
addattr32 (struct nlmsghdr *n, int maxlen, int type, int data)
{
  int len;
  struct rtattr *rta;

  len = RTA_LENGTH(4);

  if (NLMSG_ALIGN (n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr*) (((char*)n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA(rta), &data, 4);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  return 0;
}

static int
netlink_talk_filter (struct sockaddr_nl *snl, struct nlmsghdr *h)
{
  if (IS_NSM_DEBUG_KERNEL)
    zlog_warn (NSM_ZG, "netlink_talk: ignoring message type 0x%04x",
               h->nlmsg_type);
  return 0;
}

/* sendmsg() to netlink socket then recvmsg(). */
int
netlink_talk (struct nlmsghdr *n, struct nlsock *nl)
{
  int status;
  struct sockaddr_nl snl;
  struct pal_iovec iov = { (void*) n, n->nlmsg_len };
  struct pal_msghdr msg = {(void*) &snl, sizeof snl, &iov, 1, NULL, 0, 0};
  int flags = 0;

  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  n->nlmsg_seq = ++netlink_cmd.seq;

  /* Request an acknowledgement by setting NLM_F_ACK */
  n->nlmsg_flags |= NLM_F_ACK;

  if (IS_NSM_DEBUG_KERNEL)
    zlog_info (NSM_ZG, "netlink_talk: %s type %s(%u), seq=%u",
               netlink_cmd.name, lookup (nlmsg_str, n->nlmsg_type),
               n->nlmsg_type, n->nlmsg_seq);

  /* Send message to netlink interface. */
  status = pal_sock_sendmsg (nl->sock, &msg, 0);
  if (status < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "netlink_talk sendmsg() error: %s",
                  strerror (errno));
      return -1;
    }

  /*
   *    * Change socket flags for blocking I/O.
   *       * This ensures we wait for a reply in netlink_parse_info().
   *          */
  if ((flags = fcntl (nl->sock, F_GETFL, 0)) < 0)
    if (IS_NSM_DEBUG_KERNEL)
      zlog_err (NSM_ZG, "%s:%i F_GETFL error: %s",
                __FUNCTION__, __LINE__, strerror (errno));

  flags &= ~O_NONBLOCK;
  if (fcntl (nl->sock, F_SETFL, flags) < 0)
    if (IS_NSM_DEBUG_KERNEL)
      zlog_err (NSM_ZG, "%s:%i F_SETFL error: %s",
                __FUNCTION__, __LINE__, strerror (errno));

  status = netlink_parse_info (netlink_talk_filter, nl);

  /* Restore socket flags for nonblocking I/O */
  flags |= O_NONBLOCK;
  if (fcntl (nl->sock, F_SETFL, flags) < 0)
    if (IS_NSM_DEBUG_KERNEL)
      zlog_err (NSM_ZG, "%s:%i F_SETFL error: %s",
                __FUNCTION__, __LINE__, strerror (errno));

  return status;
}

#ifdef HAVE_L3
/* Routing table change via netlink interface. */
int
netlink_route (int cmd, int family, void *dest, int length, void *gate,
               int index, int zebra_flags, int zebra_ext_flags, int table,
               int metric)
{
  int ret;
  int bytelen;
  struct sockaddr_nl snl;
  struct nlsock *nl;
  int discard;
#ifdef HAVE_IPNET
  unsigned long vrid;
#endif /* HAVE_IPNET */

  struct
  {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[1024];
  } req;

  if (table >= FIB_ID_MAX)
    return -1;

  memset (&req, 0, sizeof req);

  bytelen = (family == AF_INET ? 4 : 16);

  req.n.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg));
  req.n.nlmsg_flags = NLM_F_CREATE | NLM_F_REQUEST;
  req.n.nlmsg_type = cmd;
  req.r.rtm_family = family;
  req.r.rtm_table = table;
  req.r.rtm_dst_len = length;

  if (zebra_flags & RIB_FLAG_BLACKHOLE ||
      zebra_ext_flags & RIB_EXT_FLAG_BLACKHOLE_RECURSIVE)
    discard = 1;
  else
    discard = 0;

  if (cmd == RTM_NEWROUTE)
    {
      req.r.rtm_protocol = RTPROT_ZEBRA;
      req.r.rtm_scope = RT_SCOPE_UNIVERSE;

      if (discard)
        req.r.rtm_type = RTN_BLACKHOLE;
      else
        req.r.rtm_type = RTN_UNICAST;
    }

  if (dest)
    addattr_l (&req.n, sizeof req, RTA_DST, dest, bytelen);

  if (! discard)
    {
      if (index > 0)
        addattr32 (&req.n, sizeof req, RTA_OIF, index);
      if (gate)
        addattr_l (&req.n, sizeof req, RTA_GATEWAY, gate, bytelen);
    }

  if (metric > 0)
    addattr32 (&req.n, sizeof req, RTA_PRIORITY, metric);

  /* Destination netlink address. */
  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  nl = netlink_listen[table];
  if (nl == NULL)
    return -1;

#ifdef HAVE_IPNET
  vrid = table;
  if (setsockopt (nl->sock, IP_SOL_SOCKET, IP_SO_X_VR, &vrid,
                  sizeof(vrid)) < 0)
    {
      perror("setsockopt(SO_VR) failed");
      return -1;
    }
#endif

  /* Talk to netlink socket. */
  ret = netlink_talk (&req.n, nl);

#ifdef HAVE_IPNET
  vrid = 0;
  if (setsockopt (nl->sock, IP_SOL_SOCKET, IP_SO_X_VR, &vrid,
                  sizeof(vrid)) < 0)
    {
      perror("setsockopt(SO_VR) failed");
      return -1;
    }
#endif

  if (ret < 0)
    return -1;

  return 0;
}
#endif /* HAVE_L3 */

#ifdef HAVE_VRX
/* If nexthop is WarpJ interface, a source IP address
 * is needed because no IP exists.  Without it, host
 * generated packets (ping, etc) can't be sent. */
int
addattr_vrx (struct nlmsghdr *n, size_t size, struct rib *rib)
{
  struct interface *ifp;
  struct prefix_ipv4 *p;

  ifp = ifg_lookup_by_index (&NSM_ZG->ifg,
                             nexthop->ifindex);
  if (ifp != NULL)
    if (CHECK_FLAG (ifp->vrx_flag, IF_VRX_FLAG_WRPJ))
      {
        /* Lookup local-src prefix. */
        p = vrx_localsrc_ipv4_lookup (&nzg->ifg, rib->vrf->table_id);
        if (p != NULL)
          addattr32 (n, size, RTA_PREFSRC, p->prefix.s_addr);
      }

  return 0;
}
#endif /* HAVE_VRX */

#ifdef HAVE_L3

static int
netlink_rhop_exist_in_list (struct nexthop *rhop, int count, struct pal_in4_addr *nlist)
{
  int i;

  for (i = 0; i < count; i++)
     {
	if (!pal_mem_cmp(&nlist[i], &(rhop->gate.ipv4), sizeof (struct pal_in4_addr)))
	  return 1;
     }
  return 0;
}



/* Routing table change via netlink interface. */
int
netlink_route_multipath (int cmd, u_int16_t flags,
                         struct prefix *p, struct rib *rib, int family)
{
  struct nsm_master *nm = rib->vrf->vrf->vr->proto;
  struct nsm_vrf *nsm_vrf = rib->vrf;
  int bytelen;
#ifdef HAVE_IPNET
  unsigned long vrid;
#endif /* HAVE_IPNET */
  struct sockaddr_nl snl;
  struct nexthop *nexthop = NULL;
  int nexthop_num = 0;
  struct nlsock *nl;
  int discard;
  int ret;
  struct rib *rrib = NULL;
  struct nexthop *rhop = NULL;
#define RTA_BUF_SIZE  2048

  struct
  {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[RTA_BUF_SIZE];
  } req;

  if (nsm_vrf->vrf->fib_id >= FIB_ID_MAX)
    return -1;

  memset (&req, 0, sizeof req);

  bytelen = (family == AF_INET ? 4 : 16);

  req.n.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST | flags;
  req.n.nlmsg_type = cmd;
  req.r.rtm_family = family;

  if ((rib->flags & RIB_FLAG_BLACKHOLE) ||
      (rib->ext_flags & RIB_EXT_FLAG_BLACKHOLE_RECURSIVE))
    discard = 1;
  else
    discard = 0;

  /* Set rtm FIB and Table ID. */
  netlink_rtmsg_fib_set (&req.r, nsm_vrf->vrf->fib_id);

  req.r.rtm_dst_len = p->prefixlen;

  if (cmd == RTM_NEWROUTE)
    {
      req.r.rtm_protocol = RTPROT_ZEBRA;
      req.r.rtm_scope = RT_SCOPE_UNIVERSE;

#ifdef HAVE_CRX
      /* If this is a dummy virtual IP route, set scope to RT_SCOPE_LINK. */
      if (crx_vip_exists (p))
        req.r.rtm_scope = RT_SCOPE_LINK;
#endif /* HAVE_CRX. */

      if (discard)
        req.r.rtm_type = RTN_BLACKHOLE;
      else
        req.r.rtm_type = RTN_UNICAST;

      /* Reset the FIB flag first.  */
      for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
        UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
    }
  else
    {
#ifdef HAVE_CRX
      /* If this is a dummy virtual IP route, set scope to RT_SCOPE_LINK. */
      if (crx_vip_exists (p))
        req.r.rtm_scope = RT_SCOPE_LINK;
#endif /* HAVE_CRX. */
    }

  /* If the route type is connected, then set link scope. */
  if (rib->type == APN_ROUTE_CONNECT)
    {
      req.r.rtm_protocol = RTPROT_KERNEL;
      req.r.rtm_scope = RT_SCOPE_LINK;
    }

  addattr_l (&req.n, sizeof req, RTA_DST, &p->u.prefix, bytelen);

#ifdef HAVE_UPDATE_FIB_METRIC
  /* Metric. */
  addattr32 (&req.n, sizeof req, RTA_PRIORITY, rib->metric);
#endif /* HAVE_UPDATE_FIB_METRIC */

  if (discard)
    {
      if (cmd == RTM_NEWROUTE)
        for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
          SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
      goto skip;
    }
  if ( CHECK_FLAG (rib->nexthop->flags, NEXTHOP_FLAG_RECURSIVE) )
    {
      if (!rib->nexthop->rrn || !rib->nexthop->rrn->info)
        goto skip;
      for (rrib = rib->nexthop->rrn->info; rrib; rrib = rrib->next)
        if (CHECK_FLAG (rrib->flags, RIB_FLAG_SELECTED))
          break;
      if (!rrib || !rrib->nexthop)
        goto skip;
    }


  /* Multipath case. */
  if ( rib->nexthop_active_num == 1  || (rib->nexthop_active_num == 1
       && rrib && (rrib->nexthop_active_num == 1)) ||
       (nm->multipath_num == 1))
    {
      for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
        if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
          {
            if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
              {
                if (nexthop->rtype == NEXTHOP_TYPE_IPV4
                    || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFNAME
                    || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX)
                  addattr_l (&req.n, sizeof req, RTA_GATEWAY,
                             &nexthop->rgate.ipv4, bytelen);
#ifdef HAVE_IPV6
                if (NSM_CAP_HAVE_IPV6)
                  {
                    if (nexthop->rtype == NEXTHOP_TYPE_IPV6
                        || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX
                        || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME)
                      addattr_l (&req.n, sizeof req, RTA_GATEWAY,
                                 &nexthop->rgate.ipv6, bytelen);
                  }
#endif /* HAVE_IPV6 */
                if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
                    || nexthop->rtype == NEXTHOP_TYPE_IFNAME
                    || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFNAME
                    || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX
                    || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX
                    || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME)
                  {
                    addattr32 (&req.n, sizeof req, RTA_OIF,
                               nexthop->rifindex);
#ifdef HAVE_VRX
                    addattr_vrx (&req.n, sizeof req, rib);
#endif /* HAVE_VRX */
                  }
              }
            else
              {
                if (nexthop->type == NEXTHOP_TYPE_IPV4
                    || nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME
                    || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX)
                  addattr_l (&req.n, sizeof req, RTA_GATEWAY,
                             &nexthop->gate.ipv4, bytelen);
#ifdef HAVE_IPV6
                if (NSM_CAP_HAVE_IPV6)
                  {
                    if (nexthop->type == NEXTHOP_TYPE_IPV6
                        || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME
                        || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX)
                      addattr_l (&req.n, sizeof req, RTA_GATEWAY,
                                 &nexthop->gate.ipv6, bytelen);
                  }
#endif /* HAVE_IPV6 */
                if (nexthop->type == NEXTHOP_TYPE_IFINDEX
                    || nexthop->type == NEXTHOP_TYPE_IFNAME
                    || nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME
                    || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                    || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                    || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                  {
                    addattr32 (&req.n, sizeof req,
                               RTA_OIF, nexthop->ifindex);
#ifdef HAVE_VRX
                    addattr_vrx (&req.n, sizeof req, rib);
#endif /* HAVE_VRX */
                  }
              }

            if (cmd == RTM_NEWROUTE)
              SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
            else
              UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);

            nexthop_num++;
            break;
          }
    }
  else
    {
      /*
       * The rib is either BGP  multipath with multiple BGP nexthops
       * OR
       * BGP multipath with one or more recursive nexthops
       * OR
       * IGP ECMP nexthops with multiple active routes
       */
#define MULTIPATH_MAX  64

      char buf[RTA_BUF_SIZE];
      struct rtattr *rta = (void *) buf;
      struct rtnexthop *rtnh;
      struct rib *irib = NULL;
      struct pal_in4_addr nh_list[MULTIPATH_MAX];
      int list_count = 0;

      rta->rta_type = RTA_MULTIPATH;
      rta->rta_len = RTA_LENGTH(0);
      rtnh = RTA_DATA(rta);
      
      pal_mem_set (nh_list, 0, MULTIPATH_MAX * sizeof (struct pal_in4_addr));

      nexthop_num = 0;
      for (nexthop = rib->nexthop; nexthop;
           nexthop = nexthop->next)
        {
          if ((cmd == RTM_NEWROUTE
               && CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
              || (cmd == RTM_DELROUTE
                  && CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)))
            {
              nexthop_num++;

              rtnh->rtnh_len = sizeof (*rtnh);
              rtnh->rtnh_flags = 0;
              rtnh->rtnh_hops = 0;
              rta->rta_len += rtnh->rtnh_len;

              if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                {
                   if (nexthop->rrn)
                     {
		       /* Recursive IGP nexthop is ECMP as well */

      		       for (irib = nexthop->rrn->info; irib; irib = irib->next)
        		 if (CHECK_FLAG (irib->flags, RIB_FLAG_SELECTED))
         		    break;
		        if (!irib)
			  continue; 

                        for (rhop = irib->nexthop; rhop; rhop = rhop->next)
                    	  {
                            if (rhop->type == NEXTHOP_TYPE_IPV4
                                || rhop->type == NEXTHOP_TYPE_IPV4_IFINDEX)
                              {
				if (list_count ==  MULTIPATH_MAX)
				  break;
				if ((list_count != 0) && 
			             netlink_rhop_exist_in_list(rhop, list_count, nh_list))
				  continue;
                                rta_addattr_l (rta, 4096, RTA_GATEWAY,
                                 &rhop->gate.ipv4, bytelen);
                                rtnh->rtnh_len += sizeof (struct rtattr) + 4;
				pal_mem_cpy(&nh_list[list_count], &rhop->gate.ipv4,
				            sizeof (struct pal_in4_addr));
				list_count++;
                              }
#ifdef HAVE_IPV6
                            if (NSM_CAP_HAVE_IPV6)
                              {
                                if (rhop->type == NEXTHOP_TYPE_IPV6
                                    || rhop->type == NEXTHOP_TYPE_IPV6_IFNAME
                                    || rhop->type == NEXTHOP_TYPE_IPV6_IFINDEX)
                                  rta_addattr_l (rta, 4096, RTA_GATEWAY,
                                  &rhop->gate.ipv6, bytelen);
                               }
#endif /* HAVE_IPV6 */
                           /* ifindex */
                           if (rhop->type == NEXTHOP_TYPE_IFINDEX
                               || rhop->type == NEXTHOP_TYPE_IFNAME
                               || rhop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                               || rhop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                               || rhop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                             rtnh->rtnh_ifindex = rhop->ifindex;
                           else
                             rtnh->rtnh_ifindex = 0;
                          if ( !rhop->next )
                            {
                              continue;
                            }
                          rtnh = RTNH_NEXT(rtnh);
                          if (cmd == RTM_NEWROUTE)
                            {
                              /*Check multipath-num allowed when do new route install.*/
                              if (nexthop_num == nm->multipath_num)
                                {
                                  break;
                                }
                            }
                          nexthop_num++;
                          rtnh->rtnh_len = sizeof (*rtnh);
                          rtnh->rtnh_flags = 0;
                          rtnh->rtnh_hops = 0;
                          rta->rta_len += rtnh->rtnh_len;
		        }
                    }
                  else              
                    {
		      /* Recursive nexthop but it has a single path */
                      if (nexthop->rtype == NEXTHOP_TYPE_IPV4
                          || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX)
                        {
                           rta_addattr_l (rta, 4096, RTA_GATEWAY,
                                     &nexthop->rgate.ipv4, bytelen);
                           rtnh->rtnh_len += sizeof (struct rtattr) + 4;
                        }
#ifdef HAVE_IPV6
                      if (NSM_CAP_HAVE_IPV6)
                        {
                          if (nexthop->rtype == NEXTHOP_TYPE_IPV6
                              || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME
                              || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX)
                            rta_addattr_l (rta, 4096, RTA_GATEWAY,
                                  &nexthop->rgate.ipv6, bytelen);
                        }
#endif /* HAVE_IPV6 */
                      /* ifindex */
                      if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
                          || nexthop->rtype == NEXTHOP_TYPE_IFNAME
                          || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX
                          || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX
                          || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME)
                        rtnh->rtnh_ifindex = nexthop->rifindex;
                      else
                        rtnh->rtnh_ifindex = 0;
                    }
                  }
                else
                  {
		    /* Non-recursive multipath nexthop */
                    if (nexthop->type == NEXTHOP_TYPE_IPV4
                        || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX)
                      {
                        rta_addattr_l (rta, 4096, RTA_GATEWAY,
                                     &nexthop->gate.ipv4, bytelen);
                        rtnh->rtnh_len += sizeof (struct rtattr) + 4;
                      }
#ifdef HAVE_IPV6
                    if (NSM_CAP_HAVE_IPV6)
                      {
                        if (nexthop->type == NEXTHOP_TYPE_IPV6
                            || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME
                            || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX)
                          rta_addattr_l (rta, 4096, RTA_GATEWAY,
                                       &nexthop->gate.ipv6, bytelen);
                      }
#endif /* HAVE_IPV6 */
                    /* ifindex */
                    if (nexthop->type == NEXTHOP_TYPE_IFINDEX
                        || nexthop->type == NEXTHOP_TYPE_IFNAME
                        || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                        || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME
                        || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX)
                      rtnh->rtnh_ifindex = nexthop->ifindex;
                    else
                      rtnh->rtnh_ifindex = 0;
                  }
                 rtnh = RTNH_NEXT(rtnh);

                if (cmd == RTM_NEWROUTE)
                  {
                    SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);

                    /* Check multipath-num allowed when do new route install. */
                    if (nexthop_num == nm->multipath_num)
                      break;
                  }
                else
                  UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
              }
          }

        if (rta->rta_len > RTA_LENGTH (0))
          addattr_l (&req.n, RTA_BUF_SIZE, RTA_MULTIPATH, RTA_DATA(rta),
                   RTA_PAYLOAD(rta));
    }

 skip:

  /* Destination netlink address. */
  memset (&snl, 0, sizeof snl);
  snl.nl_family = AF_NETLINK;

  if (family == AF_INET)
    nl = &netlink_cmd;
  else
    {
      nl = netlink_listen[nsm_vrf->vrf->fib_id];
      if (nl == NULL)
        return -1;
    }

#ifdef HAVE_IPNET
  vrid = nsm_vrf->vrf->fib_id;
  if (setsockopt (nl->sock, IP_SOL_SOCKET, IP_SO_X_VR, &vrid,
                  sizeof(vrid)) < 0)
    {
      perror("setsockopt(SO_VR) failed");
      return -1;
    }
#endif

  /* Talk to netlink socket.  */
  ret = netlink_talk (&req.n, nl);
  if (ret < 0)
    {
      if ((ret == -ENOENT) && (cmd == RTM_NEWROUTE))
        {
          /* No entry found for replace. In cases of admin shutdown,
           * kernel cleans up routes egressing out of the interface.
           * In these cases, create a new route with CREATE */
          req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;

          ret = netlink_talk (&req.n, nl);
        }
    }

#ifdef HAVE_IPNET
  vrid = 0;
  if (setsockopt (nl->sock, IP_SOL_SOCKET, IP_SO_X_VR, &vrid,
                  sizeof(vrid)) < 0)
    {
      perror("setsockopt(SO_VR) failed");
      return -1;
    }
#endif

#ifndef HAVE_IPNET
#ifndef HAVE_MULTIPLE_FIB
  /* Install the route to the main FIB as well if the kernel doesn't
     support the multiple FIB to support packet outgoing.  */
  if (req.r.rtm_table != RT_TABLE_MAIN)
    {
      req.r.rtm_table = RT_TABLE_MAIN;
      netlink_talk (&req.n, nl);
    }
#endif /* ! HAVE_MULTIPLE_FIB */
#endif /* ! HAVE_IPNET */

/*If the route already exists return 0*/
  if (ret == -EEXIST)
    return 0;

  return ret;
}

/* VRF route should not affect kernel routing table. */
result_t
pal_kernel_ipv4_add (struct prefix *p, struct rib *rib)
{
  return netlink_route_multipath (RTM_NEWROUTE, NLM_F_CREATE|NLM_F_EXCL,
                                  p, rib, AF_INET);
}

/* VRF route should not affect kernel routing table. */
result_t
pal_kernel_ipv4_del (struct prefix *p, struct rib *rib)
{
  return netlink_route_multipath (RTM_DELROUTE, 0, p, rib, AF_INET);
}

/* VRF route should not affect kernel routing table. */
result_t
pal_kernel_ipv4_update (struct prefix *p, struct rib *fib, struct rib *select)
{
  int ret;

#ifndef HAVE_IPNET

  ret = netlink_route_multipath (RTM_NEWROUTE, NLM_F_REPLACE,
                                 p, select, AF_INET);
  if (ret < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "Kernel route update process (updation failed. Trying adding new route) %r: %s",
                  p, strerror (errno));

      return -1;
    }
#else /* HAVE_IPNET */

  /* Delete the old route first. */
  ret = pal_kernel_ipv4_del (p, fib);
  if (ret < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "Kernel route update process (Deletion failed) %r: %s",
                  p, strerror (errno));
    }

  /* Add the new route. */
  ret = pal_kernel_ipv4_add (p, select);
  if (ret < 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (NSM_ZG, "Kernel route update process (Addition failed) %r: %s",
                  p, strerror (errno));

      /* Addition of new route failed, revert the old route. */
      ret = pal_kernel_ipv4_add (p, fib);
      if (ret < 0)
        {
          if (IS_NSM_DEBUG_KERNEL)
            zlog_err (NSM_ZG, "Kernel route update process (Reverting old route failed) %r: %s",
                      p, strerror (errno));
        }

      return -1;
    }
#endif /* HAVE_IPNET */


  return ret;
}

#ifdef HAVE_IPV6
result_t
netlink_route_multipath_ipv6 (int cmd, struct prefix *p, struct rib *rib)
{
  struct nsm_master *nm = rib->vrf->nm;
  struct nexthop *nh;
  int count = 0;
  int ret = 0;
  int metric = 0;

  for (nh = rib->nexthop; nh; nh = nh->next)
    if ((cmd == RTM_NEWROUTE && CHECK_FLAG (nh->flags, NEXTHOP_FLAG_ACTIVE))
        || (cmd == RTM_DELROUTE && CHECK_FLAG (nh->flags, NEXTHOP_FLAG_FIB)))
      if (count < nm->multipath_num)
        {

          /* No need to add IPv6 connected route on a Loopback interface */
          if ((rib->type == APN_ROUTE_CONNECT) && (nh->ifindex == 1))
            return 0;

          if (rib->type == APN_ROUTE_CONNECT && cmd == RTM_NEWROUTE)
            metric = IPV6_CONNECTED_ROUTE_DEFAULT_METRIC;


          if (CHECK_FLAG (nh->flags, NEXTHOP_FLAG_RECURSIVE))
            {
#ifdef HAVE_6PE
              if (nh->type == NEXTHOP_TYPE_IPV4)
                ret = netlink_route (cmd, AF_INET6, &p->u.prefix6, p->prefixlen,
                    NULL, nh->rifindex, rib->flags,
                    rib->ext_flags, rib->vrf->vrf->fib_id, metric);
              else
#endif
                ret = netlink_route (cmd, AF_INET6, &p->u.prefix6, p->prefixlen,
                    IN6_IS_ADDR_UNSPECIFIED (&nh->rgate.ipv6)
                    ? NULL : &nh->rgate.ipv6, nh->rifindex,
                    rib->flags, rib->ext_flags, rib->vrf->vrf->fib_id, metric);
            }
          else
            {
              ret = netlink_route (cmd, AF_INET6, &p->u.prefix6, 
	              p->prefixlen,
                  IN6_IS_ADDR_UNSPECIFIED (&nh->gate.ipv6)
                  ? NULL : &nh->gate.ipv6, nh->ifindex,
                  rib->flags, rib->ext_flags, 
                  rib->vrf->vrf->fib_id, metric);
            }

          if (ret == 0)
            {
              if (cmd == RTM_NEWROUTE)
                {
                  SET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);
                  count++;
                }
              else
                UNSET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);
            }
          else
            {
              if (IS_NSM_DEBUG_KERNEL)
                zlog_err (NSM_ZG, "netlink_route_multipath_ipv6(%s) %O: %s",
                    lookup (nlmsg_str, cmd), p, strerror (errno));
            }
        }

  return ret;
}

result_t
pal_kernel_ipv6_add (struct prefix *p, struct rib *rib)
{
  /* Linux IPv6 kernel doesn't support the full multipath yet.  */
  return netlink_route_multipath_ipv6 (RTM_NEWROUTE, p, rib);
}

result_t
pal_kernel_ipv6_del (struct prefix *p, struct rib *rib)
{
  /* Linux IPv6 kernel doesn't support the full multipath yet.  */
  return netlink_route_multipath_ipv6 (RTM_DELROUTE, p, rib);
}

result_t
pal_kernel_ipv6_update (struct prefix *p, struct rib *fib, struct rib *select)
{
  int ret;

  /* Linux IPv6 doesn't support NLM_F_REPLACE.  */
  pal_kernel_ipv6_del (p, fib);
  ret = pal_kernel_ipv6_add (p, select);

  return ret;
}

/* Delete IPv6 route from the kernel. */
result_t
pal_kernel_ipv6_old_del (struct prefix_ipv6 *dest, struct pal_in6_addr *gate,
                         u_int32_t index, u_int32_t flags, u_int32_t table)
{
  return netlink_route (RTM_DELROUTE, AF_INET6, &dest->prefix, dest->prefixlen,
                        gate, index, flags, 0, table, 0);
}
#endif /* HAVE_IPV6 */


/* Interface address modification. */
int
netlink_address (int cmd, int family, struct interface *ifp,
                 struct connected *ifc)
{
  int bytelen;
  struct prefix *p;
#ifdef HAVE_IPNET
  unsigned long vrid;
#endif /* HAVE_IPNET */
  int ret;

  struct
  {
    struct nlmsghdr n;
    struct ifaddrmsg ifa;
    char buf[1024];
  } req;

  p = ifc->address;
  memset (&req, 0, sizeof req);

  bytelen = (family == AF_INET ? 4 : 16);

  req.n.nlmsg_len = NLMSG_LENGTH (sizeof(struct ifaddrmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.n.nlmsg_type = cmd;
  req.ifa.ifa_family = family;

  req.ifa.ifa_index = ifp->ifindex;
  if (if_is_pointopoint (ifp))
    {
      if (if_is_ipv4_unnumbered (ifp))
        req.ifa.ifa_prefixlen = IPV4_MAX_PREFIXLEN;
#ifdef HAVE_IPV6
      else if (if_is_ipv6_unnumbered (ifp))
        req.ifa.ifa_prefixlen = IPV6_MAX_PREFIXLEN;
#endif /* HAVE_IPV6 */
      else
        req.ifa.ifa_prefixlen = p->prefixlen;
    }
  else
    req.ifa.ifa_prefixlen = p->prefixlen;


  addattr_l (&req.n, sizeof req, IFA_LOCAL, &p->u.prefix, bytelen);

  if (family == AF_INET && cmd == RTM_NEWADDR)
    {
      if (ifc->destination)
        {
          if (if_is_broadcast (ifp))
            {
              p = ifc->destination;
              addattr_l(&req.n, sizeof req, IFA_BROADCAST, &p->u.prefix,
                        bytelen);
            }
          else if (if_is_pointopoint (ifp))
            {
              p = ifc->destination;
              addattr_l(&req.n, sizeof req, IFA_ADDRESS, &p->u.prefix, bytelen);
            }
        }
    }


  if (cmd != RTM_DELADDR)
    {
      if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
        SET_FLAG (req.ifa.ifa_flags, IFA_F_SECONDARY);

      if (CHECK_FLAG (ifc->flags, NSM_IFA_ANYCAST))
        SET_FLAG (req.ifa.ifa_flags, NSM_IFA_ANYCAST);

      if (ifc->label)
        addattr_l (&req.n, sizeof req, IFA_LABEL, ifc->label,
                   strlen (ifc->label) + 1);
    }


#ifdef HAVE_IPNET
  vrid = ifp->vrf ? ifp->vrf->fib_id : 0;
  if (setsockopt (netlink_cmd.sock, IP_SOL_SOCKET, IP_SO_X_VR, &vrid,
                  sizeof(vrid)) < 0)
    {
      perror("setsockopt(SO_VR) failed");
      return -1;
    }
#endif
  ret = netlink_talk (&req.n, &netlink_cmd);

#ifdef HAVE_IPNET
  vrid = 0;
  if (setsockopt (netlink_cmd.sock, IP_SOL_SOCKET, IP_SO_X_VR, &vrid,
                  sizeof(vrid)) < 0)
    {
      perror("setsockopt(SO_VR) failed");
      return -1;
    }
#endif

  return ret;
}

int
kernel_address_add_ipv4 (struct interface *ifp, struct connected *ifc)
{
  return netlink_address (RTM_NEWADDR, AF_INET, ifp, ifc);
}

int
kernel_address_delete_ipv4 (struct interface *ifp, struct connected *ifc)
{
  return netlink_address (RTM_DELADDR, AF_INET, ifp, ifc);
}

#ifdef HAVE_IPV6
int
kernel_address_add_ipv6 (struct interface *ifp, struct connected *ifc)
{
  return netlink_address (RTM_NEWADDR, AF_INET6, ifp, ifc);
}

int
kernel_address_delete_ipv6 (struct interface *ifp, struct connected *ifc)
{
  int ret;
  struct connected *ifc_cmp;

  if ((ret = netlink_address (RTM_DELADDR, AF_INET6, ifp, ifc)) == 0)
    {
      /* XXX Linux kernel 2.4 doesn't delete IPv6 connected route by itself. */
      for (ifc_cmp = ifp->ifc_ipv6; ifc_cmp; ifc_cmp = ifc_cmp->next)
        if (ifc_cmp != ifc)
          if (prefix_cmp (ifc_cmp->address, ifc->address) == 0)
            return ret;

      netlink_route (RTM_DELROUTE, AF_INET6,
                     &ifc->address->u.prefix6, ifc->address->prefixlen,
                     NULL, ifp->ifindex, 0, 0, ifp->vrf ? ifp->vrf->fib_id : 0
	                 , 0);
    }
  return ret;
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

/* FIB modification. */
int
netlink_rtm_fib (int cmd, int family, fib_id_t fib, int attrtype,
                 struct interface *ifp, char *fib_name)
{

  unsigned long tfib;
  int ret;
  struct
  {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[1024];
  } req;

  memset (&req, 0, sizeof (req));
  req.n.nlmsg_type = cmd;
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST;

  req.r.rtm_family   = family;
  req.r.rtm_protocol = RTPROT_BOOT;
  req.r.rtm_scope    = RT_SCOPE_UNIVERSE;
  req.r.rtm_table    = 0;
  req.r.rtm_type     = RTN_UNSPEC;
  req.r.rtm_flags    = 0;
  req.r.rtm_src_len  = 0;
  req.r.rtm_tos          = 0;

  if (cmd == RTM_NEWRULE)
    {
      req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_EXCL;
      req.r.rtm_type = RTN_UNICAST;
    }

  tfib = fib;
  addattr_l (&req.n, sizeof(req), RTA_VR, &tfib, sizeof(tfib));

  if (fib_name)
    addattr_l (&req.n, sizeof(req), RTA_TABLE_NAME,
        fib_name, pal_strlen(fib_name) + 1); /* len + 1 for '\0' */

  ret = netlink_talk ((struct nlmsghdr*)&req.n, &netlink_cmd);
  return ret;
}

int
netlink_rtm_if_bind (int cmd, int family, fib_id_t fib, int attrtype,
                     struct interface *ifp)
{
  unsigned long tfib;
  int ret;
  struct
  {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[1024];
  } req;

  memset (&req, 0, sizeof (req));
  req.n.nlmsg_type = cmd;
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
  req.n.nlmsg_flags = NLM_F_REQUEST;

  req.r.rtm_family   = family;
  req.r.rtm_table    = 0;
  req.r.rtm_protocol = RTPROT_BOOT;
  req.r.rtm_scope    = RT_SCOPE_UNIVERSE;
  req.r.rtm_flags    = 0;
  req.r.rtm_src_len  = 0;
  req.r.rtm_tos          = 0;

  if (cmd == RTM_NEWRULE)
    {
      req.n.nlmsg_flags |= NLM_F_CREATE|NLM_F_EXCL;
      req.r.rtm_type = RTN_UNICAST;
    }

  tfib = fib;
  addattr_l (&req.n, sizeof(req), RTA_VR, &tfib, sizeof(tfib));

  if (ifp)
    addattr_l (&req.n, sizeof(req), RTA_IIF, if_kernel_name (ifp),
               strlen (if_kernel_name(ifp))+1);

  ret = netlink_talk ((struct nlmsghdr*)&req.n, &netlink_cmd);

  return ret;
}

int
pal_kernel_scan_vr_if (fib_id_t fib)
{
  int ret;

  /* Bind poll socket to FIB */
  ret = pal_sock_set_bindtofib (netlink_poll.sock, fib);
  if (ret < 0)
    return ret;

  /* Scan if and addresses */
  ret = pal_kernel_if_scan ();
  if (ret < 0)
    return ret;

  /* Bind poll socket to default FIB */
  ret = pal_sock_set_bindtofib (netlink_poll.sock, (fib_id_t)FIB_ID_MAIN);

  return ret;
}

int
kernel_fib_create (fib_id_t fib, bool_t new_vr, char *fib_name)
{
  int ret;

  /* We supply FIB name only for new VR creation (and not for new VRF
   * creation) because the fib_name will be used to name the per-VR
   * loopback interface
   */
  if (new_vr)
    ret = netlink_rtm_fib (RTM_NEWRULE, AF_INET, fib, 0, NULL, fib_name);
  else
    ret = netlink_rtm_fib (RTM_NEWRULE, AF_INET, fib, 0, NULL, NULL);

  if (ret < 0 && ret != -EEXIST)
    return ret;

  ret = pal_kernel_scan_vr_if (fib);
  if (ret < 0)
    return ret;

  return netlink_listen_sock_init (fib);
}

int
kernel_fib_delete (fib_id_t fib)
{
  int ret;

  ret =  netlink_listen_sock_uninit (fib);
  if (ret < 0)
    return ret;

  return netlink_rtm_fib (RTM_DELRULE, AF_INET, fib, 0, NULL, NULL);
}

int kernel_fib_if_bind (fib_id_t fib, struct interface *ifp)
{
  return  netlink_rtm_if_bind (RTM_NEWRULE, AF_INET, fib,
                               RTA_IIF, ifp);
}

int kernel_fib_if_unbind (fib_id_t fib, struct interface *ifp)
{
  return  netlink_rtm_if_bind (RTM_DELRULE, AF_INET, fib,
                               RTA_IIF, ifp);
}

/*
** This function is called to bind an interface to a FIB in the fowrarding
** plan.
**
** Parameters:
**    IO struct interface *ifp          : Interface to bind.
**    IO fib_id_t table                 : FIB to bind interface to
**
** Results:
**    RESULT_OK for success, else error
*/
/* Interface binding via ioctl. */
result_t
pal_kernel_if_bind_vrf (struct interface *ifp, fib_id_t fib_id)
{
#ifdef HAVE_MULTIPLE_FIB
  struct ifreq ifreq;

  memset (&ifreq, 0, sizeof (struct ifreq));

  ifreq_set_name (&ifreq, ifp);
  ifreq.ifr_flags = fib_id;

  return if_ioctl (SIOCSIFVRF, (caddr_t) &ifreq);
#endif /* HAVE_MULTIPLE_FIB */

  return kernel_fib_if_bind (fib_id, ifp);
}

/*
** This function is called to bind an interface to a FIB in the fowrarding
** plan.
**
** Parameters:
**    IO struct interface *ifp          : Interface to bind.
**    IO fib_id_t table                 : FIB to bind interface to
**
** Results:
**    RESULT_OK for success, else error
*/
result_t
pal_kernel_if_unbind_vrf (struct interface *ifp, fib_id_t fib_id)
{
#ifdef HAVE_MULTIPLE_FIB
  struct ifreq ifreq;

  memset (&ifreq, 0, sizeof (struct ifreq));

  ifreq_set_name (&ifreq, ifp);
  ifreq.ifr_flags = 0;

  return if_ioctl (SIOCSIFVRF, (caddr_t) &ifreq);
#endif /* HAVE_MULTIPLE_FIB */

  return kernel_fib_if_unbind (fib_id, ifp);
}

/* Kernel route reflection. */
int
kernel_read (struct thread *thread)
{
  int ret;
  struct nlsock *nl;

  nl = THREAD_ARG (thread);
  nl->t_read = NULL;

  ret = netlink_parse_info (netlink_information_fetch, nl);

  nl->t_read = thread_add_read (NSM_ZG, kernel_read, nl, nl->sock);

  return 0;
}

/* Exported interface function.  This function simply calls
   netlink_socket (). */
result_t
kernel_init ()
{
  int ret;

  /* Initialize listen sock for default fib */
  ret = netlink_listen_sock_init (0);
  if (ret < 0)
    return ret;

  netlink_socket (&netlink_cmd, 0, 1);
  netlink_socket (&netlink_poll, 0, 0);

  return RESULT_OK;
}

void
kernel_uninit ()
{
  fib_id_t fib_id;

  for (fib_id = 0; fib_id < FIB_ID_MAX; fib_id++)
    {
      if (netlink_listen[fib_id] == NULL)
        continue;
      netlink_listen_sock_uninit (fib_id);
    }

  return;
}
