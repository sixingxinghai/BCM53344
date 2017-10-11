/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "prefix.h"
#include "log.h"
#include "if.h"

#include "rib/rib.h"
#include "nsmd.h"

/* Proc file system to read IPv4 routing table. */
#ifndef _PATH_PROCNET_ROUTE
#define _PATH_PROCNET_ROUTE      "/proc/net/route"
#endif /* _PATH_PROCNET_ROUTE */

/* Proc file system to read IPv6 routing table. */
#ifndef _PATH_PROCNET_ROUTE6
#define _PATH_PROCNET_ROUTE6     "/proc/net/ipv6_route"
#endif /* _PATH_PROCNET_ROUTE6 */

/* Reading buffer for one routing entry. */
#define RT_BUFSIZ 1024

/* Kernel routing table read up by /proc filesystem. */
int
proc_route_read ()
{
  FILE *fp;
  char buf[RT_BUFSIZ];
  char iface[INTERFACE_NAMSIZ], dest[9], gate[9], mask[9];
  int flags, refcnt, use, metric, mtu, window, rtt;

  /* Open /proc filesystem */
  fp = fopen (_PATH_PROCNET_ROUTE, "r");
  if (fp == NULL)
    {
      zlog_warn (NSM_ZG, "Can't open %s : %s\n", _PATH_PROCNET_ROUTE, strerror (errno));
      return -1;
    }
  
  /* Drop first label line. */
  fgets (buf, RT_BUFSIZ, fp);

  while (fgets (buf, RT_BUFSIZ, fp) != NULL)
    {
      int n;
      struct prefix_ipv4 p;
      struct in_addr tmpmask;
      struct in_addr gateway;
      u_char zebra_flags = 0;

      n = sscanf (buf, "%s %s %s %x %d %d %d %s %d %d %d",
                  iface, dest, gate, &flags, &refcnt, &use, &metric, 
                  mask, &mtu, &window, &rtt);
      if (n != 11)
        {       
          zlog_warn (NSM_ZG, "can't read all of routing information\n");
          continue;
        }
      if (! (flags & RTF_UP))
        continue;
      if (! (flags & RTF_GATEWAY))
        continue;

      if (flags & RTF_DYNAMIC)
        zebra_flags |= RIB_FLAG_SELFROUTE;

      p.family = AF_INET;
      sscanf (dest, "%lX", (unsigned long *)&p.prefix);
      sscanf (mask, "%lX", (unsigned long *)&tmpmask);
      p.prefixlen = ip_masklen (tmpmask);
      sscanf (gate, "%lX", (unsigned long *)&gateway);

      nsm_rib_add_ipv4 (APN_ROUTE_KERNEL, &p, 0, &gateway, zebra_flags, 0, 0);
    }

  fclose(fp);
  return 0;
}

#ifdef HAVE_IPV6
int
proc_ipv6_route_read ()
{
  FILE *fp;
  char buf [RT_BUFSIZ];

  /* Open /proc filesystem */
  fp = fopen (_PATH_PROCNET_ROUTE6, "r");
  if (fp == NULL)
    {
      zlog_warn (NSM_ZG, "Can't open %s : %s", _PATH_PROCNET_ROUTE6, 
        strerror (errno));
      return -1;
    }
  
  /* There is no title line, so we don't drop first line.  */
  while (fgets (buf, RT_BUFSIZ, fp) != NULL)
    {
      int n;
      char dest[33], src[33], gate[33];
      char iface[INTERFACE_NAMSIZ];
      int dest_plen, src_plen;
      int metric, use, refcnt, flags;
      struct prefix_ipv6 p;
      struct in6_addr gateway;
      u_char zebra_flags = 0;

      /* Linux 2.1.x write this information at net/ipv6/route.c
         rt6_info_node () */
      n = sscanf (buf, "%32s %02x %32s %02x %32s %08x %08x %08x %08x %s",
                  dest, &dest_plen, src, &src_plen, gate,
                  &metric, &use, &refcnt, &flags, iface);

      if (n != 10)
        {       
          zlog_warn (NSM_ZG, "can't read all of routing information %d\n%s\n", n, buf);
          continue;
        }

      if (! (flags & RTF_UP))
        continue;
      if (! (flags & RTF_GATEWAY))
        continue;

      if (flags & RTF_DYNAMIC)
        zebra_flags |= RIB_FLAG_SELFROUTE;

      p.family = AF_INET6;
      str2in6_addr (dest, &p.prefix);
      str2in6_addr (gate, &gateway);
      p.prefixlen = dest_plen;

      nsm_rib_add_ipv6 (APN_ROUTE_KERNEL, &p, 0, &gateway, zebra_flags, 0, 0);
    }

  fclose (fp);

  return 0;
}
#endif /* HAVE_IPV6 */

result_t
pal_kernel_route_scan()
{
  proc_route_read ();
#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      proc_ipv6_route_read ();
    }
#endif /* HAVE_IPV6 */
  return RESULT_OK;
}
