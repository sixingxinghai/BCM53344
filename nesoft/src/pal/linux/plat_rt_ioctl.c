/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "prefix.h"
#include "log.h"
#include "if.h"
#include "nexthop.h"

#include "nsm/nsmd.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_debug.h"

/* Initialize of kernel interface.  There is no kernel communication
   support under ioctl().  So this is dummy stub function. */
void
kernel_init ()
{
  return;
}

/* Dummy function of routing socket. */
void
kernel_read (int sock)
{
  return;
}

/* Solaris has ortentry. */
#ifdef HAVE_OLD_RTENTRY
#define rtentry ortentry
#endif /* HAVE_OLD_RTENTRY */

/* Interface to ioctl route message. */
int
kernel_add_route (struct prefix_ipv4 *dest, struct in_addr *gate,
                  int index, int flags)
{
  int ret;
  int sock;
  struct rtentry rtentry;
  struct sockaddr_in sin_dest, sin_mask, sin_gate;

  memset (&rtentry, 0, sizeof (struct rtentry));

  /* Make destination. */
  memset (&sin_dest, 0, sizeof (struct sockaddr_in));
  sin_dest.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
  sin_dest.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
  sin_dest.sin_addr = dest->prefix;

  /* Make gateway. */
  if (gate)
    {
      memset (&sin_gate, 0, sizeof (struct sockaddr_in));
      sin_gate.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
      sin_gate.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
      sin_gate.sin_addr = *gate;
    }

  memset (&sin_mask, 0, sizeof (struct sockaddr_in));
  sin_mask.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
      sin_gate.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
  masklen2ip (dest->prefixlen, &sin_mask.sin_addr);

  /* Set destination address, mask and gateway.*/
  memcpy (&rtentry.rt_dst, &sin_dest, sizeof (struct sockaddr_in));
  if (gate)
    memcpy (&rtentry.rt_gateway, &sin_gate, sizeof (struct sockaddr_in));
#ifndef SUNOS_5
  memcpy (&rtentry.rt_genmask, &sin_mask, sizeof (struct sockaddr_in));
#endif /* SUNOS_5 */

  /* Routing entry flag set. */
  if (dest->prefixlen == 32)
    rtentry.rt_flags |= RTF_HOST;

  if (gate && gate->s_addr != INADDR_ANY)
    rtentry.rt_flags |= RTF_GATEWAY;

  rtentry.rt_flags |= RTF_UP;

  /* Additional flags */
  rtentry.rt_flags |= flags;


  /* For tagging route. */
  /* rtentry.rt_flags |= RTF_DYNAMIC; */

  /* Open socket for ioctl. */
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      /*zlog_warn ("can't make socket\n");*/
      return -1;
    }

  /* Send message by ioctl(). */
  ret = ioctl (sock, SIOCADDRT, &rtentry);
  if (ret < 0)
    {
      switch (errno) 
        {
        case EEXIST:
          close (sock);
          return FIB_ERR_RTEXIST;
          break;
        case ENETUNREACH:
          close (sock);
          return FIB_ERR_RTUNREACH;
          break;
        case EPERM:
          close (sock);
          return FIB_ERR_EPERM;
          break;
        }

      close (sock);
      /*zlog_warn ("write : %s (%d)", strerror (errno), errno);*/
      return 1;
    }
  close (sock);

  return ret;
}

/* Interface to ioctl route message. */
int
kernel_ioctl_ipv4 (int cmd, struct prefix *p, struct rib *rib, int family)
{
  int ret;
  int sock;
  struct rtentry rtentry;
  struct sockaddr_in sin_dest, sin_mask, sin_gate;
  struct nexthop *nexthop;
  int nexthop_num = 0;
  struct interface *ifp;

  memset (&rtentry, 0, sizeof (struct rtentry));

  /* Make destination. */
  memset (&sin_dest, 0, sizeof (struct sockaddr_in));
  sin_dest.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
  sin_dest.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
  sin_dest.sin_addr = p->u.prefix4;

  memset (&sin_gate, 0, sizeof (struct sockaddr_in));

  /* Make gateway. */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      if ((cmd == SIOCADDRT 
           && CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
          || (cmd == SIOCDELRT
              && CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)))
        {
          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            {
              if (nexthop->rtype == NEXTHOP_TYPE_IPV4 ||
                  nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX)
                {
                  sin_gate.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
                  sin_gate.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
                  sin_gate.sin_addr = nexthop->rgate.ipv4;
                  rtentry.rt_flags |= RTF_GATEWAY;
                }
              if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IFNAME)
                {
                  ifp = ifg_lookup_by_index (&NSM_ZG->ifg, nexthop->rifindex);
                  if (ifp)
                    rtentry.rt_dev = if_kernel_name (ifp);
                  else
                    return -1;
                }
            }
          else
            {
              if (nexthop->type == NEXTHOP_TYPE_IPV4 ||
                  nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX)
                {
                  sin_gate.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
                  sin_gate.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
                  sin_gate.sin_addr = nexthop->gate.ipv4;
                  rtentry.rt_flags |= RTF_GATEWAY;
                }
              if (nexthop->type == NEXTHOP_TYPE_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IFNAME)
                {
                  ifp = ifg_lookup_by_index (&NSM_ZG->ifg, nexthop->ifindex);
                  if (ifp)
                    rtentry.rt_dev = if_kernel_name (ifp);
                  else
                    return -1;
                }
            }

          if (cmd == SIOCADDRT)
            SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);

          nexthop_num++;
          break;
        }
    }

  /* If there is no useful nexthop then return. */
  if (nexthop_num == 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        /*zlog_info ("netlink_route_multipath(): No useful nexthop.");*/
      return 0;
    }

  memset (&sin_mask, 0, sizeof (struct sockaddr_in));
  sin_mask.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
  sin_mask.sin_len = sizeof (struct sockaddr_in);
#endif /* HAVE_SIN_LEN */
  masklen2ip (p->prefixlen, &sin_mask.sin_addr);

  /* Set destination address, mask and gateway.*/
  memcpy (&rtentry.rt_dst, &sin_dest, sizeof (struct sockaddr_in));

  if (rtentry.rt_flags & RTF_GATEWAY)
    memcpy (&rtentry.rt_gateway, &sin_gate, sizeof (struct sockaddr_in));

#ifndef SUNOS_5
  memcpy (&rtentry.rt_genmask, &sin_mask, sizeof (struct sockaddr_in));
#endif /* SUNOS_5 */

  /* Metric.  It seems metric minus one value is installed... */
  rtentry.rt_metric = rib->metric;

  /* Routing entry flag set. */
  if (p->prefixlen == 32)
    rtentry.rt_flags |= RTF_HOST;

  rtentry.rt_flags |= RTF_UP;

  /* Additional flags */
  /* rtentry.rt_flags |= flags; */

  /* For tagging route. */
  /* rtentry.rt_flags |= RTF_DYNAMIC; */

  /* Open socket for ioctl. */
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      /*zlog_warn ("can't make socket\n");*/
      return -1;
    }

  /* Send message by ioctl(). */
  ret = ioctl (sock, cmd, &rtentry);
  if (ret < 0)
    {
      switch (errno) 
        {
        case EEXIST:
          close (sock);
          return FIB_ERR_RTEXIST;
          break;
        case ENETUNREACH:
          close (sock);
          return FIB_ERR_RTUNREACH;
          break;
        case EPERM:
          close (sock);
          return FIB_ERR_EPERM;
          break;
        }

      close (sock);
      /*zlog_warn ("write : %s (%d)", strerror (errno), errno);*/
      return ret;
    }
  close (sock);

  return ret;
}

result_t 
pal_kernel_ipv4_add(struct prefix *p,
                    struct rib *rib)
{
  return kernel_ioctl_ipv4 (SIOCADDRT, p, rib, AF_INET);
}

result_t 
pal_kernel_ipv4_del(struct prefix *p,
                    struct rib *rib)
{
  return kernel_ioctl_ipv4 (SIOCDELRT, p, rib, AF_INET);
}

#ifdef HAVE_IPV6

/* Below is hack for GNU libc definition and Linux 2.1.X header. */
#undef RTF_DEFAULT
#undef RTF_ADDRCONF

#include <asm/types.h>

#if defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
/* struct in6_rtmsg will be declared in net/route.h. */
#else
#include <linux/ipv6_route.h>
#endif

int
kernel_ioctl_ipv6 (int type, struct prefix_ipv6 *dest, struct in6_addr *gate,
                   int index, int flags)
{
  int ret;
  int sock;
  struct in6_rtmsg rtm;
    
  memset (&rtm, 0, sizeof (struct in6_rtmsg));

  rtm.rtmsg_flags |= RTF_UP;
  rtm.rtmsg_metric = 1;
  memcpy (&rtm.rtmsg_dst, &dest->prefix, sizeof (struct in6_addr));
  rtm.rtmsg_dst_len = dest->prefixlen;

  /* We need link local index. But this should be done caller...
  if (IN6_IS_ADDR_LINKLOCAL(&rtm.rtmsg_gateway))
    {
      index = if_index_address (&rtm.rtmsg_gateway);
      rtm.rtmsg_ifindex = index;
    }
  else
    rtm.rtmsg_ifindex = 0;
  */

  rtm.rtmsg_flags |= RTF_GATEWAY;

  /* For tagging route. */
  /* rtm.rtmsg_flags |= RTF_DYNAMIC; */

  memcpy (&rtm.rtmsg_gateway, gate, sizeof (struct in6_addr));

  if (index)
    rtm.rtmsg_ifindex = index;
  else
    rtm.rtmsg_ifindex = 0;

  rtm.rtmsg_metric = 1;
  
  sock = socket (AF_INET6, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      /*zlog_warn ("can't make socket\n");*/
      return -1;
    }

  /* Send message via ioctl. */
  ret = ioctl (sock, type, &rtm);
  if (ret < 0)
    {
      /*zlog_warn ("can't %s ipv6 route: %s\n", type == SIOCADDRT ? "add" : "delete", 
           strerror(errno));*/
      ret = errno;
      close (sock);
      return ret;
    }
  close (sock);

  return ret;
}

int
kernel_ioctl_ipv6_multipath (int cmd, struct prefix *p, struct rib *rib,
                             int family)
{
  int ret;
  int sock;
  struct in6_rtmsg rtm;
  struct nexthop *nexthop;
  int nexthop_num = 0;
    
  memset (&rtm, 0, sizeof (struct in6_rtmsg));

  rtm.rtmsg_flags |= RTF_UP;
  rtm.rtmsg_metric = rib->metric;
  memcpy (&rtm.rtmsg_dst, &p->u.prefix, sizeof (struct in6_addr));
  rtm.rtmsg_dst_len = p->prefixlen;

  /* We need link local index. But this should be done caller...
  if (IN6_IS_ADDR_LINKLOCAL(&rtm.rtmsg_gateway))
    {
      index = if_index_address (&rtm.rtmsg_gateway);
      rtm.rtmsg_ifindex = index;
    }
  else
    rtm.rtmsg_ifindex = 0;
  */

  rtm.rtmsg_flags |= RTF_GATEWAY;

  /* For tagging route. */
  /* rtm.rtmsg_flags |= RTF_DYNAMIC; */

  /* Make gateway. */
  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      if ((cmd == SIOCADDRT 
           && CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
          || (cmd == SIOCDELRT
              && CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)))
        {
          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
            {
              if (nexthop->rtype == NEXTHOP_TYPE_IPV6
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX)
                {
                  memcpy (&rtm.rtmsg_gateway, &nexthop->rgate.ipv6,
                          sizeof (struct in6_addr));
                }
              if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
                  || nexthop->rtype == NEXTHOP_TYPE_IFNAME
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFNAME
                  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX)
                rtm.rtmsg_ifindex = nexthop->rifindex;
              else
                rtm.rtmsg_ifindex = 0;
              
            }
          else
            {
              if (nexthop->type == NEXTHOP_TYPE_IPV6
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX)
                {
                  memcpy (&rtm.rtmsg_gateway, &nexthop->gate.ipv6,
                          sizeof (struct in6_addr));
                }
              if (nexthop->type == NEXTHOP_TYPE_IFINDEX
                  || nexthop->type == NEXTHOP_TYPE_IFNAME
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME
                  || nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX)
                rtm.rtmsg_ifindex = nexthop->ifindex;
              else
                rtm.rtmsg_ifindex = 0;
            }

          if (cmd == SIOCADDRT)
            SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);

          nexthop_num++;
          break;
        }
    }

  /* If there is no useful nexthop then return. */
  if (nexthop_num == 0)
    {
      if (IS_NSM_DEBUG_KERNEL)
        /*zlog_info ("netlink_route_multipath(): No useful nexthop.");*/
      return 0;
    }

  sock = socket (AF_INET6, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      /*zlog_warn ("can't make socket\n");*/
      return -1;
    }

  /* Send message via ioctl. */
  ret = ioctl (sock, cmd, &rtm);
  if (ret < 0)
    {
      /*zlog_warn ("can't %s ipv6 route: %s\n",
                 cmd == SIOCADDRT ? "add" : "delete", 
           strerror(errno));*/
      ret = errno;
      close (sock);
      return ret;
    }
  close (sock);

  return ret;
}


result_t 
pal_kernel_ipv6_add (struct prefix *p, struct rib *r)
{
  return kernel_ioctl_ipv6_multipath (SIOCADDRT, p, r, AF_INET6);
}

result_t 
pal_kernel_ipv6_del(struct prefix *p, struct rib *r)
{
  return kernel_ioctl_ipv6_multipath (SIOCDELRT, p, r, AF_INET6);
}

/* Delete IPv6 route from the kernel. */
result_t 
pal_kernel_ipv6_old_del(struct prefix_ipv6 *dest,
                        struct pal_in6_addr *gate,
                        u_int32_t index,
                        u_int32_t flags,
                        u_int32_t table)
{
  return kernel_ioctl_ipv6 (SIOCDELRT, dest, gate, index, flags);
}
#endif /* HAVE_IPV6 */
