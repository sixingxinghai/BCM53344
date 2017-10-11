/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_FIB_OS_H_
#define _HSL_FIB_OS_H_
#include "hsl_fib.h"

/* 
   FIB OS callbacks.
*/
struct hsl_fib_os_callbacks
{
  /* Interface manager OS initialization. */
  int (*os_fib_init) (hsl_fib_id_t fib_id);

  /* Interface manager OS deinitialization. */
  int (*os_fib_deinit) (hsl_fib_id_t fib_id);

  /* Dump. */
  void (*os_fib_dump) (struct hsl_if *ifp);

  /* Add prefix. */
  int (*os_prefix_add) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh);

  /* Add local routes. */
  int (*os_prefix_add_if) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp);

  /* Delete prefix. */
  int (*os_prefix_delete) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh);

  /* Delete local routes. */
  int (*os_prefix_delete_if) (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp);

  /* Nexthop add. */
  int (*os_nh_add) (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh);

  /* Nexthop delete due to hardware ageing. */
  int (*os_nh_delete) (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh);
};

#endif /* _HSL_FIB_OS_H_ */
