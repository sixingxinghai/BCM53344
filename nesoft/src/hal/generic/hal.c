/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hsl/hal_netlink.h"
#include "hsl/hal_comm.h"

#ifdef HAVE_L2
int hal_l2_init (void);
int hal_l2_deinit (void);
#endif /* HAVE_L2 */
#ifdef HAVE_L3
int hal_l3_init (void);
int hal_l3_deinit (void);
#endif /* HAVE_L3 */

/* Libglobals. */
struct lib_globals *hal_zg;

struct halsock hallink_cmd  = { -1, 0, {0}, "hallink-cmd" };

/*
   Name: hal_init

   Description:
   Initialize the HAL component.

   Parameters:
   None

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_init (struct lib_globals *zg)
{
  return 0;
}

/*
   Name: hal_deinit

   Description:
   Deinitialize the HAL component.

   Parameters:
   None

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_deinit (struct lib_globals *zg)
{
  return 0;
}

int hal_talk (struct halsock *nl, struct hal_nlmsghdr *n, int (*filter) (struct hal_nlmsghdr *, void *), void *data)
{

  return 0;
}

