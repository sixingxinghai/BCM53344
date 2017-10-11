/* Copyright (C) 2002-2003   All Rights Reserved.  */

#include "pal.h"
#include "checksum.h"
#include "pal_mcast6.h"

/* PAL Multicast  API.  */
s_int32_t
pal_mcast6_mfc_add (pal_sock_handle_t sock, struct pal_in6_addr *src, 
                    struct pal_in6_addr *group, pal_mifi_t iif, int num_oifs, u_int16_t *oifs)
{
  struct pal_mf6cctl mc;
  int i;

#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
  if (num_oifs > PAL_MAXMIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

  pal_mem_set (&mc, 0, sizeof (mc));

  for (i = 0; i < num_oifs; i++)
    {
#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
      if (oifs[i] >= PAL_MAXMIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
        return 0;
#else
        return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

      IF_SET (oifs[i], &mc.mf6cc_ifset);
    }

  mc.mf6cc_origin.sin6_family = mc.mf6cc_mcastgrp.sin6_family = AF_INET6;
#ifndef HAVE_IPNET
  mc.mf6cc_origin.sin6_addr = *src;
  mc.mf6cc_mcastgrp.sin6_addr = *group;
#else
  mc.mf6cc_origin.sin6_addr = *(struct Ip_in6_addr *) src;
  mc.mf6cc_mcastgrp.sin6_addr = *(struct Ip_in6_addr *) group;
#endif /* ! HAVE_IPNET */
  mc.mf6cc_parent = iif;

  return pal_sock_set_ipv6_mrt6_add_mfc (sock, (void *) &mc, sizeof (mc));
}

s_int32_t
pal_mcast6_mfc_delete (pal_sock_handle_t sock, struct pal_in6_addr *src, 
                       struct pal_in6_addr *group)
{
  struct pal_mf6cctl mc;

  pal_mem_set (&mc, 0, sizeof (mc));

  mc.mf6cc_origin.sin6_family = mc.mf6cc_mcastgrp.sin6_family = AF_INET6;
#ifndef HAVE_IPNET
  mc.mf6cc_origin.sin6_addr = *src;
  mc.mf6cc_mcastgrp.sin6_addr = *group;
#else
  mc.mf6cc_origin.sin6_addr = *(struct Ip_in6_addr *) src;
  mc.mf6cc_mcastgrp.sin6_addr = *(struct Ip_in6_addr *) group;
#endif /* ! HAVE_IPNET */

  return pal_sock_set_ipv6_mrt6_del_mfc (sock, (void *) &mc, sizeof (mc));
}

s_int32_t 
pal_mcast6_vif_attach (pal_sock_handle_t sock, pal_mifi_t vif_index, 
                       u_int8_t flags, u_int16_t pifi)
{
  struct pal_mif6ctl vifc;

#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
  if (vif_index >= PAL_MAXMIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

  pal_mem_set (&vifc, 0, sizeof (vifc));
  vifc.mif6c_mifi = vif_index;
  vifc.mif6c_flags = flags;
  vifc.mif6c_pifi = pifi;
#ifndef HAVE_IPNET
  vifc.vifc_threshold = 1;
  vifc.vifc_rate_limit = 0;
#endif /* ! HAVE_APNNET */

  return pal_sock_set_ipv6_mrt6_add_vif (sock, (void *) &vifc, 
                                         sizeof (vifc));
}

s_int32_t 
pal_mcast6_vif_detach (pal_sock_handle_t sock, pal_mifi_t vif_index) 
{
  struct pal_mif6ctl vifc;

#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
  if (vif_index >= PAL_MAXMIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

  pal_mem_set (&vifc, 0, sizeof (vifc));
  vifc.mif6c_mifi = vif_index;
#ifndef HAVE_IPNET
  vifc.vifc_threshold = 1;
  vifc.vifc_rate_limit = 1;
#endif /* ! HAVE_IPNET */

  return pal_sock_set_ipv6_mrt6_del_vif (sock, (void *) &vifc, 
                                         sizeof (vifc));
}

s_int32_t 
pal_mcast6_get_sg_count (pal_sock_handle_t sock, struct pal_in6_addr *src,
                         struct pal_in6_addr *group, u_int32_t *pktcnt, u_int32_t *bytecnt,
                         u_int32_t *wrong_if)
{
  struct pal_sioc_sg_req6 req;
  int ret;

  pal_mem_set (&req, 0, sizeof (req));
  req.src.sin6_family = req.grp.sin6_family = AF_INET6;
#ifndef HAVE_IPNET
  req.src.sin6_addr = *src;
  req.grp.sin6_addr = *group;
#else
  req.src.sin6_addr = *(struct Ip_in6_addr *)src;
  req.grp.sin6_addr = *(struct Ip_in6_addr *)group;
#endif /* ! HAVE_IPNET */

  if ((ret =  ioctl (sock, PAL_SIOCGETSGCNT_IN6, &req)) < 0)
    return ret;

  *pktcnt = (u_int32_t) req.pktcnt;
  *bytecnt = (u_int32_t) req.bytecnt;
  *wrong_if = (u_int32_t) req.wrong_if;

  return RESULT_OK;
}

s_int32_t 
pal_mcast6_get_vif_count (pal_sock_handle_t sock, int vif_index,
                          u_int32_t *ipktcnt, u_int32_t *opktcnt, u_int32_t *ibytecnt,
                          u_int32_t *obytecnt)
{
  struct pal_sioc_mif_req6 req;
  int ret;

  if (vif_index >= PAL_MAXMIFS)
    return -1;

  pal_mem_set (&req, 0, sizeof (req));

  req.mifi = vif_index;

  if ((ret =  ioctl (sock, PAL_SIOCGETMIFCNT_IN6, &req)) < 0)
    return ret;

  *ipktcnt = (u_int32_t) req.icount;
  *opktcnt = (u_int32_t) req.ocount;
  *ibytecnt = (u_int32_t) req.ibytes;
  *obytecnt = (u_int32_t) req.obytes;

  return RESULT_OK;
}

/* Set the multicast routing socket options and start multicast routing
**
** Parameters:
**    IN     pal_sock_handle_t   sock
** Results:
**    0 for success, -1 for error
*/
s_int32_t 
pal_mrt6_start (pal_sock_handle_t sock)
{
  s_int32_t on = 1;

  return pal_sock_set_ipv6_mrt6_init (sock, on);
}

/* Unset the multicast routing socket options and stop 
** multicast routing
**
** Parameters:
**    IN     pal_sock_handle_t   sock
** Results:
**    0 for success, -1 for error
*/
s_int32_t 
pal_mrt6_stop (pal_sock_handle_t sock)
{
  s_int32_t off = 0;

  return pal_sock_set_ipv6_mrt6_done (sock, off);
}

/* Set the multicast routing socket options for PIM and start multicast routing
**
** Parameters:
**    IN     pal_sock_handle_t   sock
** Results:
**    0 for success, -1 for error
*/
s_int32_t 
pal_pim6_mrt_start (pal_sock_handle_t sock)
{
  s_int32_t on = 1;

  return pal_sock_set_ipv6_mrt6_pim (sock, on);
}

/* Unset the multicast routing socket options for PIM and stop multicast routing
**
** Parameters:
**    IN     pal_sock_handle_t   sock
** Results:
**    0 for success, -1 for error
*/
s_int32_t 
pal_pim6_mrt_stop (pal_sock_handle_t sock)
{
  s_int32_t off = 0;
         
  return pal_sock_set_ipv6_mrt6_pim (sock, off);
}

/* Set the TTL of PIM Register Innher multicast data packet IP header
**
** Parameters:
**    IN     struct pal_in6_header*   iph
** Results:
**    None
*/
void 
pal_pim6_reg_data_set_ttl (struct pal_in6_header *iph)
{
  --iph->ip6_hlim;
  return;
}

/* Get the packet len of inner multicast data packet for Registers
**
** Parameters:
**    IN     struct pal_in6_header*   iph
**    OUT    u_int32_t*   len
** Results:
**    None
*/
void 
pal_pim6_reg_data_pkt_len_get (struct pal_in6_header *iph, u_int32_t *len)
{
  *len = pal_ntoh16 (iph->ip6_plen);
  return;
#if 0
  *len = 0;
#endif
}

/* Calculate the checksum of PIM Register Inner multicast data packet IP header
**
** Parameters:
**    IN     struct pal_in6_header*   iph
** Results:
**    None
*/
void 
pal_pim6_reg_data_set_cksum (struct pal_in6_header *iph)
{
  return;
}

/* Adjust the mrt6msg in the buffer received from kernel so that im6_src and
** im6_dst are properly set to ip6_src and ip6_dst for WHOLEPKT message.
**
** Parameters:
**    IN     u_char*   buf
** Results:
**    None
*/
void 
pal_pim6_correct_wholepkt_msg (u_char *buf)
{
  return;
}

