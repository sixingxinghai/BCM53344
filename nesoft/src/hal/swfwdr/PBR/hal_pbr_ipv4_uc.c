/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_PBR
#include "hal_incl.h"
#include "hal_pbr_ipv4_uc.h"


/*
   Name: hal_pbr_ipv4_uc_route

   Description:
   Common function for PBR route.

   Parameters:

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_pbr_ipv4_uc_route (int op, char *rmap_name, s_int32_t seq_num,
                       struct hal_prefix *src_prefix,
                       struct hal_prefix *dst_prefix, s_int16_t protocol,
                       int sport_op, u_int32_t sport_low, u_int32_t sport,
                       int dport_op, u_int32_t dport_low, u_int32_t dport,
                       int tos_op, u_int8_t tos_low, u_int8_t tos,
                       enum filter_type filter_type,
                       s_int32_t precedence,
                       struct hal_in4_addr *nexthop,
                       s_int16_t nh_type,
                       char *ifname,
                       s_int32_t if_index)
{
  return 1;
}
#endif /* HAVE_PBR */
