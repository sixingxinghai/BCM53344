/* Copyright (C) 2002-2003  All Rights Reserved. */
#ifndef  PAL_MCAST6_H
#define  PAL_MCAST6_H

#define _LINUX_IN_H

#ifndef HAVE_IPNET /* ! HAVE_IPNET */
#ifdef HAVE_LINUX_MROUTE6_H
#include <linux/mroute6.h>
#endif /* HAVE_MROUTE6 */

typedef unsigned short mifi_t;

#undef  pal_mifi_t
#define pal_mifi_t            mifi_t

#undef  PAL_MAXMIFS
#define PAL_MAXMIFS           MAXMIFS

#undef  PAL_IF_SETSIZE
#define PAL_IF_SETSIZE        IF_SET_SIZE

#undef  pal_if_set
#define pal_if_set            if_set

#undef  pal_mif6ctl
#define pal_mif6ctl           mif6ctl

#undef  PAL_MIFF_REGISTER
#define PAL_MIFF_REGISTER     MIFF_REGISTER

#undef  pal_mf6cctl
#define pal_mf6cctl           mf6cctl

#undef  pal_sioc_sg_req6
#define pal_sioc_sg_req6      sioc_sg_req6

#undef  pal_sioc_mif_req6
#define pal_sioc_mif_req6      sioc_mif_req6

#undef  pal_mrt6msg
#define pal_mrt6msg           mrt6msg

#undef  PAL_MRT6_NOCACHE
#define PAL_MRT6_NOCACHE      MRT6MSG_NOCACHE

#undef  PAL_MRT6_WRONGMIF
#define PAL_MRT6_WRONGMIF     MRT6MSG_WRONGMIF

#undef  PAL_MRT6_WHOLEPKT
#define PAL_MRT6_WHOLEPKT     MRT6MSG_WHOLEPKT

#undef PAL_SIOCGETSGCNT_IN6
#define PAL_SIOCGETSGCNT_IN6          SIOCGETSGCNT_IN6

#undef PAL_SIOCGETMIFCNT_IN6
#define PAL_SIOCGETMIFCNT_IN6         SIOCGETMIFCNT_IN6

#else /* HAVE_IPNET */

#define IP_MAXMIFS   64  /* Maximum number of multicast interfaces that can be installed */


#ifndef IP_IF_SETSIZE
#define IP_IF_SETSIZE 256
#endif

#define IP_MRT6_NIFBITS (sizeof(Ip_if_mask) * 8) /* Bits per mask */

#define IP_MRT6_MIFF_REGISTER 0x1 /* Mif represents a register end-point */

/* Macros to operate on the mf6cc_ifset bit array */
#define IP_MRT6_IF_SET(n, p)    \
((p)->ifs_bits[(n)/IP_MRT6_NIFBITS] |= (1 << ((n) % IP_MRT6_NIFBITS)))
#define IP_MRT6_IF_CLR(n, p)    \
((p)->ifs_bits[(n)/IP_MRT6_NIFBITS] &= ~(1 << ((n) % IP_MRT6_NIFBITS)))
#define IP_MRT6_IF_ISSET(n, p) \
((p)->ifs_bits[(n)/IP_MRT6_NIFBITS] & (1 << ((n) % IP_MRT6_NIFBITS)))

/* Message types for Ip_mrt6msg */
#define IP_MRT6MSG_NOCACHE  1  /* Received packet that did not match any route entry */
#define IP_MRT6MSG_WRONGMIF 2  /* Packet arrived on the wrong interface */
#define IP_MRT6MSG_WHOLEPKT 3  /* PIM register processing */

/*
 *===========================================================================
 *                         ioctl values
 *===========================================================================
 * IOCTL value: <3-bit type> <13-bit size> <4-bit flag> <4-bit group> <8-bit id>
 */

/* IOCTL types: */
#define IP_IOC_VOID  0x20000000
#define IP_IOC_OUT   0x40000000
#define IP_IOC_IN    0x80000000
#define IP_IOC_INOUT (IP_IOC_IN | IP_IOC_OUT)

/* IOCTL size: */
#define IP_IOCPARM_MASK    0x1fff      /* Parameter length 13 bits */
#define IP_IOCPARM_LEN(r)  (IP_IOCPARM_MASK & ((r) >> 16))

/* IOCTL flags. */
#define IP_IOCF_R       0x00000000   /* read. */
#define IP_IOCF_W       0x00001000   /* write bit. */

/* IOCTL groups: */
#define IP_IOCG_BASE        1
#define IP_IOCG_SOCK        2
#define IP_IOCG_NETIF       3
#define IP_IOCG_ARP         4
#define IP_IOCG_INET        5
#define IP_IOCG_INET6       6
#define IP_IOCG_ETH         7
#define IP_IOCG_PPP         8
#define IP_IOCG_WLAN        9
#define IP_IOCG_MCAST       10
#define IP_IOCG_MCAST_IN6   11
#define IP_IOCG_MPLS        12
#define IP_IOCG_RTAB        13
#define IP_IOCG_DS          14
#define IP_IOCG_POLICY_RT   15

/* PAL definitions */
#undef  pal_mifi_t
#define pal_mifi_t            Ip_mifi_t

#undef  PAL_MAXMIFS
#define PAL_MAXMIFS           IP_MAXMIFS

#undef  PAL_IF_SETSIZE
#define PAL_IF_SETSIZE        IP_IF_SET_SIZE

#undef  pal_if_set
#define pal_if_set            Ip_if_set

#undef  IF_SET
#define IF_SET                IP_MRT6_IF_SET

#undef  IF_CLR
#define IF_CLR                IP_MRT6_IF_CLR

#undef  IF_ISSET
#define IF_ISSET              IP_MRT6_IF_ISSET

#undef  IF_COPY
#define        IF_COPY(f, t)         bcopy(f, t, sizeof(*(f)))

#undef  IF_ZERO
#define        IF_ZERO(p)            bzero(p, sizeof(*(p)))

#undef  pal_mif6ctl
#define pal_mif6ctl           Ip_mif6ctl

#undef  PAL_MIFF_REGISTER
#define PAL_MIFF_REGISTER     IP_MRT6_MIFF_REGISTER

#undef  pal_mf6cctl
#define pal_mf6cctl           Ip_mf6cctl

#undef  pal_sioc_sg_req6
#define pal_sioc_sg_req6      Ip_sioc_sg_req6

#undef  pal_sioc_mif_req6
#define pal_sioc_mif_req6      Ip_sioc_mif_req6

#undef  pal_mrt6msg
#define pal_mrt6msg           Ip_mrt6msg

#undef  PAL_MRT6_NOCACHE
#define PAL_MRT6_NOCACHE      IP_MRT6MSG_NOCACHE

#undef  PAL_MRT6_WRONGMIF
#define PAL_MRT6_WRONGMIF     IP_MRT6MSG_WRONGMIF

#undef  PAL_MRT6_WHOLEPKT
#define PAL_MRT6_WHOLEPKT     IP_MRT6MSG_WHOLEPKT

#undef PAL_SIOCGETSGCNT_IN6
#define PAL_SIOCGETSGCNT_IN6          IP_SIOCGETSGCNT_IN6

#undef PAL_SIOCGETMIFCNT_IN6
#define PAL_SIOCGETMIFCNT_IN6         IP_SIOCGETMIFCNT_IN6

#ifndef IPPROTO_NONE
#define IPPROTO_NONE          59
#endif

#endif /* HAVE_IPNET */

#include "pal_mcast6.def"

#endif /* PAL_MCAST6_H */
