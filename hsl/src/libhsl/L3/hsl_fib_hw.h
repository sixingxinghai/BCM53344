/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_FIB_HW_H_
#define _HSL_FIB_HW_H_
#include "hsl_fib.h"

/* 
   FIB HW callbacks.
*/
struct hsl_fib_hw_callbacks
{
  /* Initialization. */
  int (*hw_fib_init) (hsl_fib_id_t fib_id);

  /* Deinitialization. */
  int (*hw_fib_deinit) (hsl_fib_id_t fib_id);

  /* Dump. */
  void (*hw_fib_dump) (hsl_fib_id_t fib_id);

  /* Add prefix. */
  int (*hw_prefix_add) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh);

  /* Add prefix as exception. */
  int (*hw_prefix_add_exception) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp);

  /* Delete prefix. */
  int (*hw_prefix_delete) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh);

  /* Add nexthop. */
  int (*hw_nh_add) (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh);

  /* Nexthop delete. */
  int (*hw_nh_delete) (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh);

  /* Nexthop hit. 
     Returns 1 for hit.
     Returns 0 for not hit. */
  int (*hw_nh_hit) (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh);

  /* IPv4 connected route exception addition. */
  int (*hw_add_connected_route) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp);

  /* IPv4 connected route exception deletion. */
  int (*hw_delete_connected_route) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp);

  /* Get maximum number of multipaths.. */
  int (*hw_get_max_multipath) (u_int32_t *ecmp);
};

#endif /* _HSL_FIB_HW_H_ */
