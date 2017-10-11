/* Copyright (C) 2004   All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_debug.h"
#include "hal_msg.h"

#ifdef HAVE_L2
int hal_l2_init (void);
int hal_l2_deinit (void);
#endif /* HAVE_L2 */

extern struct lib_globals *hal_zg;

/*
  Initialize HAL L2 components.
*/
int
hal_l2_init (void)
{
  int ret;

#if defined(HAVE_STPD) || defined(HAVE_RSTPD) || defined (HAVE_MSTPD) || defined (HAVE_RPVST_PLUS)
  ret = hal_bridge_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL Bridging");
      goto CLEANUP;
    }

  ret = hal_l2_fdb_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL L2 FDB");
      goto CLEANUP;
    }
#endif /* defined(HAVE_STPD) || defined(HAVE_RSTPD) || defined (HAVE_MSTPD) || defined(HAVE_RPVST_PLUS)*/

#ifdef HAVE_AUTHD
  ret = hal_auth_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL 802.1x");
      goto CLEANUP;
    }
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
  ret = hal_lacp_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL Link Aggregation\n");
      goto CLEANUP;
    }
#endif /* HAVE_LACPD */

#ifdef HAVE_IGMP_SNOOP
  ret = hal_igmp_snooping_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL IGMP Snooping\n");
      goto CLEANUP;
    }
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  ret = hal_mld_snooping_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL MLD Snooping\n");
      goto CLEANUP;
    }
#endif /* HAVE_MLD_SNOOP */



   /* Flow control. */
  ret = hal_flow_control_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL Flow Control\n");
      goto CLEANUP;
    }

  /* Rate limit control. */
  ret = hal_ratelimit_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL Rate Limiting\n");
      goto CLEANUP;
    }

#ifdef HAVE_VLAN
  ret = hal_vlan_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL VLAN\n");
      goto CLEANUP;
    }
#endif /* HAVE_VLAN */

  return 0;

 CLEANUP:
  /* Deinitialize. */
  hal_l2_deinit ();

  return -1;
}

/*
  Deinitialize HAL L2 components.
*/
int
hal_l2_deinit (void)
{
  int ret;

#if defined(HAVE_STPD) || defined(HAVE_RSTPD) || defined (HAVE_MSTPD) || defined(HAVE_RPVST_PLUS)
  ret = hal_bridge_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL Bridging\n");
    }

  ret = hal_l2_fdb_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL L2 FDB\n");
    }
#endif /* defined(HAVE_STPD) || defined(HAVE_RSTPD) || defined (HAVE_MSTPD) || defined(HAVE_RPVST_PLUS)*/

#ifdef HAVE_AUTHD
  ret = hal_auth_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL 802.1x\n");
    }
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
  ret = hal_lacp_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL Link Aggregation\n");
    }
#endif /* HAVE_LACPD */

#ifdef HAVE_IGMP_SNOOP
  ret = hal_igmp_snooping_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL IGMP Snooping\n");
    }
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  ret = hal_mld_snooping_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL MLD Snooping\n");
    }
#endif /* HAVE_MLD_SNOOP */


   /* Flow control. */
  ret = hal_flow_control_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL Flow Control\n");
    }

  /* Rate limit control. */
  ret = hal_ratelimit_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL Broadcast Storm Control\n");
    }

#ifdef HAVE_VLAN
  ret = hal_vlan_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL VLAN\n");
    }
#endif /* HAVE_VLAN */

  return 0;
}
