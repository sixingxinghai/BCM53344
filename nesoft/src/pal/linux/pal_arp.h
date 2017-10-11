/* Copyright (C) 2002  All Rights Reserved. */
#include "pal_arp.def"

#ifndef _PAL_ARP_H_
#define _PAL_ARP_H_

#define PAL_HW_ALEN             6
#define PAL_IPADDR_LEN          16
#define PAL_HWADDR_LEN          18
#define PAL_BUFF                200
#define PAL_IF_SIZ              20
#define _PAL_PATH_PROCNET_ARP   "/proc/net/arp"

#ifdef HAVE_RTADV
#define PAL_PATH_NEIGH          "/proc/sys/net/ipv6/neigh/"
#define PAL_PATH_REACHABLE_TIME_MS "base_reachable_time_ms"
#define PAL_PATH_REACHABLE_TIME  "base_reachable_time"
#define PAL_PATH_IPV6_CONF      "/proc/sys/net/ipv6/conf/"
#define PAL_PATH_HOPLIMIT       "hop_limit"
#endif /* HAVE_RTADV */
#

int  pal_set_arp ( struct pal_in4_addr *, char *);
int  pal_unset_arp (struct pal_in4_addr *);
int  pal_unset_arp_all ();
int  pal_display_arp (int (*func)(struct show_arp * ,void *), void *);
#endif /* _PAL_ARP_H_ */
