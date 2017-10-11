/* pal_mld.h,v 1.10 2008/01/30 20:29:30 bob.macgill Exp */
/* Copyright (C) 2002-2005  All Rights Reserved. */

#ifndef _PACOS_PAL_MLD_H
#define _PACOS_PAL_MLD_H

#ifndef HAVE_IPNET
/* MLD header for Linux */
/* *MLD header defined here till Linux supports MLDv1* */

/*
 * Multicast Listener Discovery
 */
struct mld6_hdr 
{
  struct icmp6_hdr              mld6_hdr;
  struct in6_addr               mld6_addr; /* multicast address */
};

#undef pal_mld
#define pal_mld mld6_hdr

#define mld6_type       mld6_hdr.icmp6_type
#define mld6_code       mld6_hdr.icmp6_code
#define mld6_cksum      mld6_hdr.icmp6_cksum
#define mld6_maxdelay   mld6_hdr.icmp6_maxdelay
#define mld6_reserved   mld6_hdr.icmp6_data16[1]

/* MLD message type mappings for Linux */
#undef  PAL_MLD_LISTENER_QUERY
#ifdef MLD_LISTENER_QUERY
#define PAL_MLD_LISTENER_QUERY                  MLD_LISTENER_QUERY
#else
#define PAL_MLD_LISTENER_QUERY                  ICMP6_MEMBERSHIP_QUERY
#endif /* MLD_LISTENER_QUERY */

#undef  PAL_MLD_LISTENER_REPORT
#ifdef MLD_LISTENER_REPORT
#define PAL_MLD_LISTENER_REPORT                 MLD_LISTENER_REPORT
#else
#define PAL_MLD_LISTENER_REPORT                 ICMP6_MEMBERSHIP_REPORT
#endif /* MLD_LISTENER_REPORT */

#undef  PAL_MLDV2_LISTENER_REPORT
#define PAL_MLDV2_LISTENER_REPORT               (143)

#undef  PAL_MLD_LISTENER_DONE
#ifdef MLD_LISTENER_REDUCTION
#define PAL_MLD_LISTENER_DONE                   MLD_LISTENER_REDUCTION
#else
#define PAL_MLD_LISTENER_DONE                   ICMP6_MEMBERSHIP_REDUCTION
#endif /* MLD_LISTENER_REDUCTION */

#else  /* HAVE_IPNET */

/*
 *===========================================================================
 *                      ICMPv6 header & types
 *===========================================================================
 */

#define ipnet_icmp6_data32      icmp6_dataun.icmp6_un_data32
#define ipnet_icmp6_data16      icmp6_dataun.icmp6_un_data16
#define ipnet_icmp6_data8       icmp6_dataun.icmp6_un_data8
#define ipnet_icmp6_pptr        ipnet_icmp6_data32[0]  /* parameter prob */
#define ipnet_icmp6_mtu         ipnet_icmp6_data32[0]  /* packet too big */
#define ipnet_icmp6_id          ipnet_icmp6_data16[0]  /* echo request/reply */
#define ipnet_icmp6_seq         ipnet_icmp6_data16[1]  /* echo request/reply */
#define ipnet_icmp6_maxdelay    ipnet_icmp6_data16[0]  /* mcast group membership */

/*
 *===========================================================================
 *                ICMPv6 Multicast Listener Discovery types
 *===========================================================================
 */

/* Multicast Listener Discovery Definitions */
#define IPNET_MLD_LISTENER_QUERY        130
#define IPNET_MLD_LISTENER_REPORT       131
#define IPNET_MLD_LISTENER_REDUCTION    132


#define ipnet_mld_type          mld_icmp6_hdr.icmp6_type
#define ipnet_mld_code          mld_icmp6_hdr.icmp6_code
#define ipnet_mld_cksum         mld_icmp6_hdr.icmp6_cksum
#define ipnet_mld_maxdelay      mld_icmp6_hdr.ipnet_icmp6_data16[0]
#define ipnet_mld_reserved      mld_icmp6_hdr.ipnet_icmp6_data16[1]

/*
 *===========================================================================
 *           ICMPv6 Multicast Listener Discovery Version 2 types
 *===========================================================================
 */

#define IPNET_MLDV2_LISTENER_REPORT     143

/* PAL definitions */
#undef pal_mld
#define pal_mld Ipnet_mld_hdr

#undef  mld6_type
#define mld6_type               ipnet_mld_type

#undef  mld6_code
#define mld6_code               ipnet_mld_code

#undef  mld6_cksum
#define mld6_cksum              ipnet_mld_cksum

#undef  mld6_maxdelay
#define mld6_maxdelay           ipnet_mld_maxdelay

#undef  mld6_reserved
#define mld6_reserved           ipnet_mld_reserved

#undef  mld6_reserved
#define mld6_reserved           ipnet_mld_reserved

#undef  mld6_addr
#define mld6_addr               mld_addr


/* MLD message type mappings for IPNET2 */
#undef  PAL_MLD_LISTENER_QUERY             
#define PAL_MLD_LISTENER_QUERY                IPNET_MLD_LISTENER_QUERY

#undef  PAL_MLD_LISTENER_REPORT             
#define PAL_MLD_LISTENER_REPORT               IPNET_MLD_LISTENER_REPORT

#undef  PAL_MLD_LISTENER_DONE             
#define PAL_MLD_LISTENER_DONE                 IPNET_MLD_LISTENER_REDUCTION

#undef  PAL_MLDV2_LISTENER_REPORT
#ifdef IPNET_MLDV2_LISTENER_REPORT
#define PAL_MLDV2_LISTENER_REPORT             IPNET_MLDV2_LISTENER_REPORT  
#else
#define PAL_MLDV2_LISTENER_REPORT             (143)  
#endif /* IPNET_MLDV2_LISTENER_REPORT */

#endif /* ! HAVE_IPNET */

#endif /* PAL_MLD_H */
