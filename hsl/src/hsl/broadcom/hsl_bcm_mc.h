/* Copyright (C) 2004   All Rights Reserved. */

#ifndef __HSL_BCM_MC_FIB_H
#define __HSL_BCM_MC_FIB_H 

#ifdef HAVE_L3
struct hsl_bcm_mfib
{
  struct hsl_route_table *grp_tree;
};

struct hsl_bcm_mfib_entry1
{
  struct hsl_route_node *node;

  u_int32_t  src_count;
  struct hsl_route_table *src_tree;
};

struct hsl_bcm_mfib_entry
{
  struct hsl_route_node *node;

  struct hsl_bcm_mfib_entry1 *grp;

  u_int32_t iif_ifindex;
};
#endif /* HAVE_L3 */
#endif /* __HSL_BCM_MC_FIB_H */
