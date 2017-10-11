/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_L3
/* Extern from rt_netlink.c */
void netlink_route_read ();

result_t
pal_kernel_route_scan()
{
  netlink_route_read ();
  return RESULT_OK;
}
#endif /* HAVE_L3 */
