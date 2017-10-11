/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_debug.h"

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

/* Forward declarations. */
#ifdef HAVE_L3
int hal_num_multipath_resp(struct hal_nlmsghdr *h, void *data);
#endif /* HAVE_L3 */


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
  int ret;
  int init_mode = HAL_MSG_IF_L2_INIT; /* Default init mode is L2 */
  /* Set ZG. */
  hal_zg = zg;

  /* Initialize DEBUG clis. */
  hal_debug_cli_init (zg);

  /* Initialize HAL-HSL transport. */
  hal_comm_init (zg);

  /* Send HAL initialization message to HSL. */
  ret = hal_msg_generic_request (HAL_MSG_INIT, NULL, NULL);
  if (ret < 0)
    goto CLEANUP;

#ifdef HAVE_L2
  /* L2 initialization. */
  ret = hal_l2_init ();
  if (ret < 0)
    goto CLEANUP;
#endif /* HAVE_L2 */

  /* Port mirroring. */
  ret = hal_port_mirror_init ();
  if (ret < 0)
      goto CLEANUP;

#ifdef HAVE_L3
  /* L3 initialization. */
  ret = hal_l3_init ();
  if (ret < 0)
    goto CLEANUP;

  ret = hal_msg_generic_request (HAL_MSG_GET_MAX_MULTIPATH, hal_num_multipath_resp, NULL);
  if (ret < 0)
    goto CLEANUP;

  init_mode = HAL_MSG_IF_L3_INIT;
#endif /* HAVE_L3 */

  /* Send interface L2/L3 initialization mode. */
  ret = hal_msg_generic_request (init_mode, NULL, NULL);
  if (ret < 0)
    goto CLEANUP;

#ifdef HAVE_MCAST_IPV4
   {
     /* Send enable multicast router. */
     int def_fib = 0;
     ret = hal_ipv4_mc_init (def_fib);
     if(ret < 0 )
       goto CLEANUP;
   }
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
   {
     /* Send enable multicast router. */
     int def_fib = 0;
     ret = hal_ipv6_mc_init (def_fib);
     if(ret < 0 )
       goto CLEANUP;
   }
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_QOS
   ret = hal_qos_init ();
   if (ret < 0)
     goto CLEANUP;
#endif /* HAVE_QOS */

  /* Populate the interface manager. */
  hal_if_get_list ();

  return 0;

 CLEANUP:
  hal_deinit (zg);

  return -1;
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
#ifdef HAVE_MCAST_IPV4
  {
    /* Send enable multicast router. */
    int def_fib = 0;
    hal_ipv4_mc_deinit (def_fib);
  }
#endif /* HAVE_MCAST_IPV4 */

  /* Send HAL deinitialization message to HSL. */
  hal_msg_generic_request (HAL_MSG_DEINIT, NULL, NULL);

#ifdef HAVE_L2
  /* L2 initialization. */
  hal_l2_deinit ();
#endif /* HAVE_L2 */

  /* Port mirroring. */
  hal_port_mirror_deinit ();

#ifdef HAVE_L3
  /* L3 initialization. */
  hal_l3_deinit ();
#endif /* HAVE_L3 */

#ifdef HAVE_QOS
  hal_qos_deinit ();
#endif /* HAVE_QOS */

  /* Deinitialize HAL-HSL transport. */
  hal_comm_deinit (zg);

  return 0;
}

