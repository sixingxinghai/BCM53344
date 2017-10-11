/* Copyright (C) 2001-2004  All Rights Reserved.  */

#include "pal.h"

#ifdef HAVE_NSM_IF_ARBITER

#include "lib.h"
#include "cli.h"
#include "if.h"
#include "linklist.h"
#include "prefix.h"
#include "table.h"
#include "ptree.h"
#include "thread.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_fea.h"
#ifdef HAVE_L3
#include "nsm/rib/nsm_table.h"
#include "nsm/rib/rib.h"
#endif /* HAVE_L3 */
#include "nsm/nsm_interface.h"
#include "nsm/nsm_connected.h"
#include "nsm/nsm_if_arbiter.h"
#include "nsm/nsm_debug.h"
#ifdef HAVE_HA
#include "lib_cal.h"
#include "nsm_cal.h"
#endif /* HAVE_HA */


/* Interface arbiter for some kernel does not send interface update
   message to routing daemon.  */
void
nsm_if_arbiter_delete_ifp (struct interface *ifp)
{
  /* Only delete interraces in the default VRf of Priviledged VR. 
   * This change makes if arbiter compatible with
   * VR/VRF.
   */
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ARBITER))
    UNSET_FLAG (ifp->status, NSM_INTERFACE_ARBITER);
  else if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE) && 
           ifp->vrf && IS_APN_VRF_PRIV_DEFAULT (ifp->vrf))
    nsm_if_delete_update (ifp);
}

void
nsm_if_arbiter_delete_interface (void)
{
  struct interface *ifp;
  struct listnode *node, *next;

  for (node = LISTHEAD (NSM_ZG->ifg.if_list); node; node = next)
    {
      ifp = GETDATA (node);
      next = node->next;
      nsm_if_arbiter_delete_ifp (ifp);
    }
}

#ifdef HAVE_L3
void
nsm_if_arbiter_delete_interface_addr (void)
{
  struct interface *ifp;
  struct connected *ifc, *next;
  struct route_node *rn;
  struct prefix *p;

  for (rn = route_top (NSM_ZG->ifg.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      {
        if (!if_is_ipv4_unnumbered (ifp))
          for (ifc = ifp->ifc_ipv4; ifc; ifc = next)
            {
              next = ifc->next;
              p = ifc->address;

              if (CHECK_FLAG (ifc->conf, NSM_IFC_ARBITER))
                {
                  UNSET_FLAG (ifc->conf, NSM_IFC_ARBITER);

#ifdef HAVE_HA
                  lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */

                }
              else if (if_is_running (ifp))
                nsm_connected_delete_ipv4 (ifp, 0, &p->u.prefix4,
                                           p->prefixlen, NULL);
            }

#ifdef HAVE_IPV6
        if (!if_is_ipv6_unnumbered (ifp))
          for (ifc = ifp->ifc_ipv6; ifc; ifc = next)
            {
              next = ifc->next;
              p = ifc->address;
              if (CHECK_FLAG (ifc->conf, NSM_IFC_ARBITER))
                {
                  UNSET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
                  lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */

                }
              else if (if_is_running (ifp))
                nsm_connected_delete_ipv6 (ifp, &p->u.prefix6,
                                           p->prefixlen, NULL);
            }
#endif /* HAVE_IPV6 */
      }

  return;
}
#endif /* HAVE_L3 */

/* Refresh all of interface's flag.  */
s_int32_t
nsm_if_arbiter (struct thread *t)
{
  struct interface *ifp;
  struct route_node *rn;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "If-arbiter timer expire");

#ifdef HAVE_HA
  nsm_cal_delete_if_arbiter_timer (ng);
#endif /* HAVE_HA */

  /* Update interface. */
  nsm_fea_if_update ();

  /* Trim unnecessary interfaces. */
  nsm_if_arbiter_delete_interface ();

#ifdef HAVE_L3
  /* Trim unnecessary interface addresses. */
  nsm_if_arbiter_delete_interface_addr ();
#endif /* HAVE_L3 */

  /* Refresh interface. */
  for (rn = route_top (NSM_ZG->ifg.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      nsm_if_refresh (ifp);

#ifdef HAVE_CUSTOM1
  if (ng->if_arbiter_interval != 0 && ng->t_if_arbiter == NULL)
#else
  if (ng->t_if_arbiter != NULL)
#endif /* HAVE_CUSTOM1 */
    ng->t_if_arbiter = thread_add_timer (NSM_ZG, nsm_if_arbiter, NULL,
                                       ng->if_arbiter_interval);

#ifdef HAVE_HA
  nsm_cal_create_if_arbiter_timer (ng);
#endif /* HAVE_HA */

  return 0;
}

/* Start interface arbiter thread.  */
void
nsm_if_arbiter_start (int interval)
{
  ng->if_arbiter_interval = interval;

  if  (! ng->t_if_arbiter)
    {
      ng->t_if_arbiter = thread_add_timer (nzg, nsm_if_arbiter, NULL, interval);
#ifdef HAVE_HA
      nsm_cal_create_if_arbiter_timer (ng);
#endif /* HAVE_HA */
    }

#ifdef HAVE_HA
  nsm_cal_modify_nsm_globals (ng);
#endif /* HAVE_HA */
}

/* Stop interface arbiter thread.  */
void
nsm_if_arbiter_stop (void)
{
  ng->if_arbiter_interval = NSM_IF_ARBITER_INTERVAL_DEFAULT;

#ifdef HAVE_HA
  if  (ng->t_if_arbiter)
    nsm_cal_delete_if_arbiter_timer (ng);
#endif /* HAVE_HA */

  THREAD_OFF (ng->t_if_arbiter);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_globals (ng);
#endif /* HAVE_HA */
}

CLI (interface_arbiter,
     interface_arbiter_cmd,
     "if-arbiter (interval <1-65535>|)",
     "Start arbiter to check interface information periodically",
     "Poll interval",
     "Seconds")
{
  int interval = NSM_IF_ARBITER_INTERVAL_DEFAULT;

  if (argc > 1)
    CLI_GET_INTEGER_RANGE ("interval", interval, argv[1], 1, 65535);

  nsm_if_arbiter_start (interval);

  return CLI_SUCCESS;
}

CLI (no_interface_arbiter,
     no_interface_arbiter_cmd,
     "no if-arbiter",
     CLI_NO_STR,
     "Stop arbiter to check interface information periodically")
{
  nsm_if_arbiter_stop ();
  return CLI_SUCCESS;
}

void
nsm_if_arbiter_config_write (struct cli *cli)
{
  if (ng->t_if_arbiter)
    {
      cli_out (cli, "if-arbiter interval %d\n", ng->if_arbiter_interval);
      cli_out (cli, "!\n");
    }
}

void
nsm_if_arbiter_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* Interface Arbiter.  */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &interface_arbiter_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_interface_arbiter_cmd);

}

#endif /* HAVE_NSM_IF_ARBITER */
