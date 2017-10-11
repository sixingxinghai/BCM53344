/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PAL_SOCK_RAW_H
#define _PAL_SOCK_RAW_H

/*
  pal_sock_raw.h -- PacOS PAL interface statistics
*/
#include "pal.h"

/* PAL types for IPv6 Extension Headers */
#undef  pal_ip6_hbh
#define pal_ip6_hbh             ip6_hbh  

#undef  pal_ip6_dest
#define pal_ip6_dest            ip6_dest  

/* Option types and related macros */
#ifndef IP6OPT_PAD1
#define IP6OPT_PAD1             0x00    /* 00 0 00000 */
#endif /* IP6OPT_PAD1 */
#ifndef IP6OPT_PADN
#define IP6OPT_PADN             0x01    /* 00 0 00001 */
#endif /* IP6OPT_PADN */
#ifndef IP6OPT_JUMBO
#define IP6OPT_JUMBO            0xC2    /* 11 0 00010 = 194 */
#endif /* IP6OPT_JUMBO */
#ifndef IP6OPT_JUMBO_LEN
#define IP6OPT_JUMBO_LEN        6
#endif /* IP6OPT_JUMBO_LEN */
#ifndef IP6OPT_RTALERT
#define IP6OPT_RTALERT          0x05    /* 00 0 00101 */
#endif /* IP6OPT_RTALERT */
#ifndef IP6OPT_RTALERT_LEN
#define IP6OPT_RTALERT_LEN      4
#endif /* IP6OPT_RTALERT_LEN */
#ifndef IP6OPT_RTALERT_MLD
#define IP6OPT_RTALERT_MLD      0       /* Datagram contains an MLD message */
#endif /* IP6OPT_RTALERT_MLD */
#ifndef IP6OPT_RTALERT_RSVP
#define IP6OPT_RTALERT_RSVP     1       /* Datagram contains an RSVP message */
#endif /* IP6OPT_RTALERT_RSVP */
#ifndef IP6OPT_RTALERT_ACTNET
#define IP6OPT_RTALERT_ACTNET   2       /* contains an Active Networks msg */
#endif /* IP6OPT_RTALERT_ACTNET */
#ifndef IP6OPT_MINLEN
#define IP6OPT_MINLEN           2
#endif /* IP6OPT_MINLEN */

#ifndef HAVE_IPNET

#undef  PAL_IP6OPT_PAD1
#define PAL_IP6OPT_PAD1         IP6OPT_PAD1  

#undef  PAL_IP6OPT_PADN
#define PAL_IP6OPT_PADN         IP6OPT_PADN  
/* PAL IPv6 Router Alert options */
#undef  PAL_IP6OPT_RTALERT
#define PAL_IP6OPT_RTALERT      IP6OPT_RTALERT  

#undef  PAL_IP6OPT_RTALERT_LEN
#define PAL_IP6OPT_RTALERT_LEN  IP6OPT_RTALERT_LEN 

#undef  PAL_IP6OPT_RTALERT_MLD
#define PAL_IP6OPT_RTALERT_MLD  IP6OPT_RTALERT_MLD 

#undef  PAL_IP6OPT_RTALERT_RSVP
#define PAL_IP6OPT_RTALERT_RSVP  IP6OPT_RTALERT_RSVP

#define PAL_CMSG_SPACE          CMSG_SPACE
#define PAL_CMSG_FIRSTHDR       CMSG_FIRSTHDR
#define PAL_CMSG_LEN            CMSG_LEN
#define PAL_CMSG_DATA           CMSG_DATA
#define PAL_CMSG_NXTHDR         CMSG_NXTHDR

/*
  pktinfo
*/
#define pal_in_pktinfo    in_pktinfo

#else /* HAVE_IPNET */

#undef  PAL_IP6OPT_PAD1
#define PAL_IP6OPT_PAD1         IP_IP6OPT_PAD1 

#undef  PAL_IP6OPT_PADN
#define PAL_IP6OPT_PADN         IP_IP6OPT_PADN 
/* PAL IPv6 Router Alert options */
#undef  PAL_IP6OPT_RTALERT
#define PAL_IP6OPT_RTALERT      IP_IP6OPT_ROUTER_ALERT 

#undef  PAL_IP6OPT_RTALERT_LEN
#define PAL_IP6OPT_RTALERT_LEN  (4) 

#undef  PAL_IP6OPT_RTALERT_MLD
#define PAL_IP6OPT_RTALERT_MLD  IP_IP6_ALERT_MLD 

#undef  PAL_IP6OPT_RTALERT_RSVP
#define PAL_IP6OPT_RTALERT_RSVP  IP_IP6_ALERT_RSVP

#define PAL_CMSG_SPACE          CMSG_SPACE
#define PAL_CMSG_FIRSTHDR       CMSG_FIRSTHDR
#define PAL_CMSG_LEN            CMSG_LEN
#define PAL_CMSG_DATA           CMSG_DATA
#define PAL_CMSG_NXTHDR         CMSG_NXTHDR
 
/*
  pktinfo
*/
#define pal_in_pktinfo    Ip_in_pktinfo

#endif /* HAVE_IPNET */

/* IPv6 options */
struct pal_ip6_opt
{
  u_char ip6o_type;
  u_char ip6o_len;
};

/* PAL defines for IPv6 Options header */
int pal_inet6_opt_init (void *extbuf, size_t extlen);
int pal_inet6_opt_append (void *extbuf, socklen_t extlen, int offset, 
    u_char type, socklen_t len, u_char align, void **databufp);
int pal_inet6_opt_set_val (void *databuf, int offset, void *val, 
    socklen_t vallen);
int pal_inet6_opt_finish (void *extbuf, socklen_t extlen, int offset);

#include "pal_sock_raw.def"

#endif /* PAL_SOCK_RAW_H */
