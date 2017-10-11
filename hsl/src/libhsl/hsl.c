/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hal_if.h"
#include "hsl_oss.h"
#include "hsl_logs.h"
#include "hsl.h"
#include "hsl_table.h"
#include "hsl_ether.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_if_os.h"
#ifdef HAVE_L2
#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "hsl_mac_tbl.h"
#endif /* HAVE_L2 */
#ifdef HAVE_L3
#include "hsl_fib.h"
#endif /* HAVE_L3 */
#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 || defined HAVE_IGMP_SNOOP
#include "hsl_mcast_fib.h"
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 || HAVE_IGMP_SNOOP */

#include "hal_types.h"

static int hsl_initialized = 0;

/* Extern interfaces. */
extern void hsl_sock_ifmgr_notify_chain_register (void);
extern void hsl_sock_ifmgr_notify_chain_unregister (void);

/*
  Initialize HSL.
*/
int
hsl_init (void)
{
  HSL_FN_ENTER ();

  if (hsl_initialized)
    return 0;

  /* Initialize OS layer, backends, network buffers etc. */
  hsl_os_init ();

  /* Initialize interface manager. */
  hsl_ifmgr_init ();

#ifdef HAVE_L2
  /* Initialize master bridge structure. */
  hsl_bridge_master_init ();

  /* Initialize FDB table */
  if (0 != hsl_init_fdb_table ())
    hsl_deinit_fdb_table();
#endif /* HAVE_L2 */

#ifdef HAVE_L3
  /* Initialize FIB manager. */
  hsl_fib_init ();
#endif /* HAVE_L3 */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  hsl_ipv4_mc_db_init();
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */

#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
  hsl_ipv6_mc_db_init();
#endif /* HAVE_MCAST_IPV6 */

  /* Initialize hardware layer. */
  hsl_hw_init ();
  hsl_soft_init();

  hsl_acl_init();
  hsl_car_init();

  
  /* Register interface manager notifiers. */
  hsl_sock_ifmgr_notify_chain_register (); 
  
    hsl_ptp_init();/*djg*/ 

  hsl_initialized = 1;



  HSL_FN_EXIT (0);
}

/**************************************************************************************/
#if 0
int
hsl_init_ (void)
{
  HSL_FN_ENTER ();

  if (hsl_initialized)
    return 0;

  /* Initialize OS layer, backends, network buffers etc. */
  hsl_os_init ();

  /* Initialize interface manager. */
  hsl_ifmgr_init ();

#ifdef HAVE_L2
  /* Initialize master bridge structure. */
  hsl_bridge_master_init ();

  /* Initialize FDB table */
  if (0 != hsl_init_fdb_table ())
    hsl_deinit_fdb_table();
#endif /* HAVE_L2 */

#ifdef HAVE_L3
  /* Initialize FIB manager. */
  hsl_fib_init ();
#endif /* HAVE_L3 */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
  hsl_ipv4_mc_db_init();
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */

#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
  hsl_ipv6_mc_db_init();
#endif /* HAVE_MCAST_IPV6 */

  /* Initialize hardware layer. */
  hsl_hw_init_test ();

  /* Register interface manager notifiers. */
  hsl_sock_ifmgr_notify_chain_register ();  

  hsl_initialized = 1;

  HSL_FN_EXIT (0);
}

#endif

/* 
   Deinitialize HSL.
*/
int
hsl_deinit (void)
{
  HSL_FN_ENTER ();

  if (! hsl_initialized)
    HSL_FN_EXIT (-1);

  /* Unregister interface manager notifiers. */
  hsl_sock_ifmgr_notify_chain_unregister ();

  /* Deinitialize OS layer, backends, network buffers etc. */
  hsl_os_deinit ();

  /* Deinitialize interface manager. */
  hsl_ifmgr_deinit ();

#ifdef HAVE_L2
  /* Deinitialize master bridge structure. */
  hsl_bridge_master_deinit ();

  /* Deinitialize FDB table */
  hsl_deinit_fdb_table();
#endif /* HAVE_L2 */

#ifdef HAVE_L3
  /* Deinitialize FIB manager. */
  hsl_fib_deinit ();

#ifdef HAVE_MCAST_IPV4
  hsl_ipv4_mc_db_deinit();
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  hsl_ipv6_mc_db_deinit();
#endif /* HAVE_MCAST_IPV6 */

#endif /* HAVE_L3 */

  /* Deinitialize hardware layer. */
  hsl_hw_deinit ();

  hsl_initialized = 0;

  HSL_FN_EXIT (0);
}
