/* Copyright (C) 2002-2003   All Rights Reserved.  */

#include "pal.h"
#include "checksum.h"
#include "pal_memory.h"
#include "pal_socket.h"
#include "pal_mcast.h"

s_int32_t
pal_mcast_mfc_add (pal_sock_handle_t sock, struct pal_in4_addr *src, 
    struct pal_in4_addr *group, vifi_t iif, int num_oifs, 
    u_int16_t *olist_vifs, u_char ttls[])
{
  struct pal_mfcctl mc;
  vifi_t vifi;
  int i;

#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
  if (num_oifs > PAL_MAXVIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */

  pal_mem_set (&mc, 0, sizeof (mc));

  mc.mfcc_origin = *src;
  mc.mfcc_mcastgrp = *group;
  mc.mfcc_parent = iif;

  for (i = 0; i < num_oifs; i++)
    {
#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
      if ((vifi = olist_vifs[i]) >= PAL_MAXVIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
        return 0;
#else
        return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */

      mc.mfcc_ttls[vifi] = ttls[i];
    }

  return pal_sock_set_ipv4_mrt_add_mfc (sock, (void *) &mc, sizeof (mc));
}

s_int32_t
pal_mcast_mfc_delete (pal_sock_handle_t sock, struct pal_in4_addr *src, 
                      struct pal_in4_addr *group, vifi_t iif)
{
  struct pal_mfcctl mc;

  mc.mfcc_origin = *src;
  mc.mfcc_mcastgrp = *group;
  mc.mfcc_parent = iif;

  return pal_sock_set_ipv4_mrt_del_mfc (sock, (void *) &mc, sizeof (mc));
}

s_int32_t 
pal_mcast_vif_attach (pal_sock_handle_t sock, s_int32_t vif_index, 
                      struct pal_in4_addr *addr, struct pal_in4_addr *rmt_addr, u_int16_t flags)
{
  struct pal_vifctl vifc;

#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
  if (vif_index >= PAL_MAXVIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */


  pal_mem_set (&vifc, 0, sizeof (struct pal_vifctl));
  vifc.vifc_vifi = vif_index;
  vifc.vifc_flags = flags;
  vifc.vifc_threshold = 1;
  vifc.vifc_rate_limit = 0;
  vifc.vifc_lcl_addr = *addr;
  vifc.vifc_rmt_addr = *rmt_addr;

  return pal_sock_set_ipv4_mrt_add_vif (sock, (void *) &vifc, 
                                        sizeof (struct pal_vifctl));
}

s_int32_t 
pal_mcast_vif_detach (pal_sock_handle_t sock, s_int32_t vif_index, 
                      struct pal_in4_addr *addr, struct pal_in4_addr *rmt_addr, u_int16_t flags)
{
  struct pal_vifctl vifc;

#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
  if (vif_index >= PAL_MAXVIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */

  pal_mem_set (&vifc, 0, sizeof (struct pal_vifctl));
  vifc.vifc_vifi = vif_index;
  vifc.vifc_flags = flags;
  vifc.vifc_threshold = 1;
  vifc.vifc_rate_limit = 1;
  vifc.vifc_lcl_addr = *addr;
  vifc.vifc_rmt_addr = *rmt_addr;

  return pal_sock_set_ipv4_mrt_del_vif (sock, (void *) &vifc, 
                                        sizeof (struct pal_vifctl));
}

s_int32_t 
pal_mcast_get_sg_count (pal_sock_handle_t sock, struct pal_in4_addr *src,
                        struct pal_in4_addr *group, u_int32_t *pktcnt, u_int32_t *bytecnt,
                        u_int32_t *wrong_if)
{
  struct pal_sioc_sg_req req;
  int ret;

  req.src = *src;
  req.grp = *group;

  if ((ret = ioctl (sock, PAL_SIOCGETSGCNT, &req)) < 0)
    return ret;

  *pktcnt = req.pktcnt;
  *bytecnt = req.bytecnt;
  *wrong_if = req.wrong_if;

  return RESULT_OK;
}

s_int32_t 
pal_mcast_get_vif_count (pal_sock_handle_t sock, int vif_index,
                         u_int32_t *ipktcnt, u_int32_t *opktcnt, u_int32_t *ibytecnt,
                         u_int32_t *obytecnt)
{
  struct pal_sioc_vif_req req;
  int ret;

  if (vif_index >= PAL_MAXVIFS)
    return -1;

  req.vifi = vif_index;

  if ((ret = ioctl (sock, PAL_SIOCGETVIFCNT, &req)) < 0)
    return ret;

  *ipktcnt = req.icount;
  *opktcnt = req.ocount;
  *ibytecnt = req.ibytes;
  *obytecnt = req.obytes;

  return RESULT_OK;
}

/* Set the multicast routing socket options for PIM and start multicast routing
**
** Parameters:
**    IN     pal_sock_handle_t   sock
** Results:
**    0 for success, -1 for error
*/
s_int32_t 
pal_pim_mrt_start (pal_sock_handle_t sock)
{
  s_int32_t on = 1;

  return pal_sock_set_ipv4_pim (sock, on);
}

/* Unset the multicast routing socket options for PIM and stop multicast routing
**
** Parameters:
**    IN     pal_sock_handle_t   sock
** Results:
**    0 for success, -1 for error
*/
s_int32_t 
pal_pim_mrt_stop (pal_sock_handle_t sock)
{
  s_int32_t off = 0;
         
  return pal_sock_set_ipv4_pim (sock, off);
}

/* Set the multicast routing socket options and start 
** multicast routing
**
** Parameters:
**    IN     pal_sock_handle_t   sock
** Results:
**    0 for success, -1 for error
*/
s_int32_t 
pal_mrt_start (pal_sock_handle_t sock)
{
  s_int32_t on = 1;

  return pal_sock_set_ipv4_mrt_init (sock, on);
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
pal_mrt_stop (pal_sock_handle_t sock)
{
  s_int32_t off = 0;
         
  return pal_sock_set_ipv4_mrt_done (sock, off);
}

/* Set the TTL of PIM Register Innher multicast data packet IP header
**
** Parameters:
**    IN     struct pal_in4_header*   iph
** Results:
**    None
*/
void 
pal_pim_reg_data_set_ttl (struct pal_in4_header *iph)
{
  --iph->ip_ttl;
  return;
}

/* Get the packet len of inner multicast data packet for Registers
**
** Parameters:
**    IN     struct pal_in4_header*   iph
**    OUT    u_int32_t*   len
** Results:
**    None
*/
void 
pal_pim_reg_data_pkt_len_get (struct pal_in4_header *iph, u_int32_t *len)
{
  pal_in4_ip_packet_len_get (iph, len);
  return;
}

/* Calculate the checksum of PIM Register Inner multicast data packet IP header
**
** Parameters:
**    IN     struct pal_in4_header*   iph
** Results:
**    None
*/
void 
pal_pim_reg_data_set_cksum (struct pal_in4_header *iph)
{
  u_int16_t ip_hdr_len;

  pal_in4_ip_hdr_len_get (iph, &ip_hdr_len);
  iph->ip_sum = 0;
  iph->ip_sum = in_checksum ((u_int16_t *)iph, ip_hdr_len << 2);
  return;
}

