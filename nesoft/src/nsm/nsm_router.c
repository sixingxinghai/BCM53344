/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "table.h"
#include "bitmap.h"
#include "lib_mtrace.h"
#include "nsm_message.h"

#ifdef HAVE_L2
#include "nsm_bridge.h"
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
#include "nsm_l2_mcast.h"
#endif
#endif

#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#include "nsmd.h"

#ifdef HAVE_MPLS
#include "mpls.h"
#include "bitmap.h"
#include "mpls_common.h"
#include "nsm_mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
#include "nsm_qos_serv.h"
#endif /* HAVE_TE */

#ifdef HAVE_L3
#include "rib.h"
#endif /* HAVE_L3 */

#include "nsm_interface.h"
#include "nsm_server.h"
#include "nsm_router.h"
#ifdef HAVE_L3
#include "nsm_nexthop.h"
#endif /* HAVE_L3 */
#include "nsm/nsm_fea.h"
#include "nsm/nsm_api.h"

#include "nsm_fea.h"
#include "nsm_api.h"

#ifdef HAVE_RTADV
#include "nsm_rtadv.h"
#endif /* HAVE_RTADV */
#ifdef HAVE_VRRP
#include "vrrp_init.h"
#endif /* HAVE_VRRP */
#ifdef HAVE_IPSEC
#include "nsm/ipsec/ipsec.h"
#endif
#ifdef HAVE_MCAST_IPV4
#include "nsm_mcast4.h"
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
#include "nsm_mcast6.h"
#endif /* HAVE_MCAST_IPV6 */
#ifdef HAVE_FIREWALL
#include "nsm/firewall/nsm_firewall.h"
#endif /* HAVE_FIREWALL */
#ifdef HAVE_LACPD
#include "nsm_lacp.h"
#endif /* HAVE_LACPD */
#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_HA
#include "lib_cal.h"
#include "nsm_cal.h"
#ifdef HAVE_L3
#include "nsm_rib_cal.h"
#endif /* HAVE_L3 */
#endif /* HAVE_HA */

#ifdef HAVE_PBR
#include "nsm_pbr_filter.h"
#endif /* HAVE_PBR */

#ifdef HAVE_L3
static void
nsm_router_get_highest_addr (struct interface *ifp,
                             struct pal_in4_addr *loopback,
                             struct pal_in4_addr *normal)
{
  struct connected *ifc;
  struct prefix *p;
  struct pal_in4_addr *router_id;
  struct pal_in4_addr *addr;

  if (if_is_loopback (ifp))
    router_id = loopback;
  else
    router_id = normal;

  /* Check all IP address.  */
  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      p = ifc->address;
      addr = &p->u.prefix4;

      /* Skip configuration information.  */
      if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
        /* Ignore net 127.  */
        if (!IPV4_NET127 (pal_ntoh32 (addr->s_addr)))
          /* Compare address.  */
          if (pal_ntoh32 (router_id->s_addr) < pal_ntoh32 (addr->s_addr))
            *router_id = *addr;
    }
}

/* Set router ID for specific VRF.  */
void
nsm_router_id_update (struct nsm_vrf *nv)
{
  struct apn_vrf *iv = nv->vrf;
  struct interface *ifp;
  struct pal_in4_addr old_router_id;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
  int i;

  /* Store current ID.  */
  old_router_id = iv->router_id;

  /* Is router id manually set?  */
  if (CHECK_FLAG (nv->config, NSM_VRF_CONFIG_ROUTER_ID))
    {
      iv->router_id = nv->router_id_config;
      nv->router_id_type = NSM_ROUTER_ID_CONFIG;
    }
  else
    {
      struct route_node *rn;
      struct pal_in4_addr loopback;
      struct pal_in4_addr normal;

      /* Clear router-id value.  */
      loopback.s_addr = 0;
      normal.s_addr = 0;

      for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
        if ((ifp = rn->info))
          if (if_is_running (ifp))
            nsm_router_get_highest_addr (ifp, &loopback, &normal);

      /* Check loopback address is found or not.  */
      if (loopback.s_addr)
        {
          iv->router_id = loopback;
          nv->router_id_type = NSM_ROUTER_ID_AUTOMATIC_LOOPBACK;
        }
      else if (normal.s_addr)
        {
          iv->router_id = normal;
          nv->router_id_type = NSM_ROUTER_ID_AUTOMATIC;
        }
      else
        {
          iv->router_id.s_addr = 0;
          nv->router_id_type = NSM_ROUTER_ID_NONE;
        }
    }

  iv->vr->router_id = iv->router_id;

  /* Notify all clients the change of the router ID.  */
  if (old_router_id.s_addr != iv->router_id.s_addr)
    NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
      if (nsm_service_check (nse, NSM_SERVICE_ROUTER_ID))
        nsm_server_send_router_id_update (iv->vr->id, iv->id,
                                          nse, iv->router_id);

#ifdef HAVE_HA
  lib_cal_modify_vrf (iv->vr->zg, iv);

  lib_cal_modify_vr (iv->vr->zg, iv->vr);

  nsm_cal_modify_nsm_vrf (nv);
#endif /* HAVE_HA */
}

int
nsm_router_id_update_timer (struct thread *thread)
{
  struct nsm_vrf *nv = THREAD_ARG (thread);

  nv->t_router_id = NULL;

#ifdef HAVE_HA
  nsm_cal_delete_rtr_id_timer (nv);
#endif /* HAVE_HA */

  nsm_router_id_update (nv);

  return 0;
}
#endif /* HAVE_L3 */


/* NSM master initialize. */
struct nsm_master *
nsm_master_init (struct apn_vr *vr)
{
  struct nsm_master *nm;

  nm = XCALLOC (MTYPE_NSM_MASTER, sizeof (struct nsm_master));
  nm->zg = nzg;
  nm->vr = vr;
  vr->proto = nm;

#ifdef HAVE_HA
  nsm_cal_create_nsm_master (nm);
#endif /* HAVE_HA */

  /* Initialize default VRF. */
  nsm_vrf_get_by_name (nm, NULL);

  /* Set protocol ID for VR.
     Commented out: Will update IMI when it starts.
  */
/*  MODBMAP_SET (nm->module_bits, APN_PROTO_IMI); */

  /* Set start time.  */
  nm->start_time = pal_time_current (NULL);

#ifdef HAVE_ONMD
  nsm_l2_oam_master_init (nm);
#endif /* HAVE_ONMD */

#ifdef HAVE_L3
  /* Set the IPv4 forwarding by default.  */
  nsm_ipv4_forwarding_set (vr->id);

#ifdef HAVE_IPV6
  /* Set the IPv6 forwarding by default.  */
  nsm_ipv6_forwarding_set (vr->id);
#endif /* HAVE_IPV6*/
#endif /* HAVE_L3 */

  /* Set Default multipath num. */
  nm->multipath_num = DEFAULT_MULTIPATH_NUM;

#ifdef HAVE_NSM_IF_PARAMS
  /* Initialize the interface parameter list.  */
  nm->if_params = list_new ();
#endif /* HAVE_NSM_IF_PARAMS */

#ifdef HAVE_L3
  /* Set default maximum static routes */
  nm->max_static_routes = MAX_STATIC_ROUTE_DEFAULT;
  nm->max_fib_routes = MAX_FIB_ROUTE_DEFAULT;

  /* Register the stale FIB sweep timer.  */
  THREAD_TIMER_ON (nm->zg, nm->t_sweep, nsm_rib_sweep_timer,
                   nm, NSM_FIB_RETAIN_TIME_DEFAULT);
#ifdef HAVE_HA
  nsm_cal_create_rib_sweep_timer (nm);
#endif /* HAVE_HA */
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
  /* Initialize MPLS. */
  if (NSM_CAP_HAVE_MPLS)
    nsm_mpls_init (nm);
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
  /* QOS INIT */
  nsm_qos_serv_init (nm);
#endif /* HAVE_TE */

#ifdef HAVE_LACPD
  nm->lacp_admin_key_mgr = bitmap_new (1280, 1, 65535);
#endif /* HAVE_LACPD */

#ifdef HAVE_RTADV
  /* Initialize the router advertisement service.  */
  if (NSM_CAP_HAVE_IPV6)
    nsm_rtadv_init (nm);
#endif /* HAVE_RTADV */

#ifdef HAVE_VRRP
  vrrp_init (nm);
#endif /* HAVE_VRRP */

#ifdef HAVE_STAGGER_KERNEL_MSGS
  nm->kernel_msg_stagger_list = list_new ();
#endif /* HAVE_STAGGER_KERNEL_MSGS */

  nm->phyEntityList = list_new ();

#ifdef HAVE_L3
  nm->arp = nsm_arp_create_master ();
#endif /* HAVE_L3 */

#ifdef HAVE_IPSEC
  ipsec_initialize (nm);
#endif /* HAVE_IPSEC */
#ifdef HAVE_FIREWALL
  nsm_firewall_initialize (nm);
#endif /* HAVE_FIREWALL */

#ifdef HAVE_L2
  nsm_bridge_init (nm);
#endif /* HAVE_L2 */

#ifdef HAVE_L3
#ifdef HAVE_PBR
  nsm_pbr_route_map_if_init (nm);  
  nsm_pbr_event_init (nm);
 /* Initialize Access list.  */
  access_list_add_hook (vr, nsm_pbr_access_list_add);
  access_list_delete_hook (vr, nsm_pbr_access_list_delete);
#endif /* HAVE_PBR */
#endif /* HAVE_L3 */

  return nm;
}

/* Finish NSM Master. */
void
nsm_master_finish (struct apn_vr *vr)
{
#ifdef HAVE_LACPD
  struct nsm_lacp_admin_key_element *ake, *ake_next;
#endif /* HAVE_LACPD */
  struct nsm_master *nm;
  struct apn_vrf *iv;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
#ifdef HAVE_VR
  struct listnode *node = NULL, *node_next = NULL;
  struct interface *ifp = NULL;
#endif /* HAVE_VR */
#ifdef HAVE_L2
  struct nsm_bridge_master *master;
#endif /* HAVE_L2 */
  int i;

  if (! vr)
    return;

  nm = vr->proto;
  if (! nm)
    return;

#ifdef HAVE_VRRP
  vrrp_close (nm);
#endif /* HAVE_VRRP */

#ifdef HAVE_VR
  /* Bind all interfaces to default VR */
  if (! IS_APN_VR_PRIVILEGED (vr))
    {
      for (node = LISTHEAD (vr->ifm.if_list); node; node = node_next)
        {
          node_next = node->next;
          ifp = GETDATA (node);

          /* Per-VR loopback interface (created by IP stack at the
           * time of VR creation), should be deleted instead of being
           * moved to the PVR
           * This should only be done for non-PVR.
           */
          if (if_is_loopback (ifp))
            {
              nsm_if_delete_update (ifp);

              if (ifp->clean_pend_resp_list)
                list_free (ifp->clean_pend_resp_list);

              nsm_if_delete_update_done (ifp);
            }
          else
            {
              nsm_if_unbind_vr (ifp, nm);
            }
        }
    }
#endif /* HAVE_VR */


#ifdef HAVE_STAGGER_KERNEL_MSGS
  nsm_kernel_msg_stagger_clean_all (nm);
  list_delete (nm->kernel_msg_stagger_list);
#endif /* HAVE_STAGGER_KERNEL_MSGS */

#ifdef HAVE_L3
  /* Cancel the stale FIB sweep timer.  */
#ifdef HAVE_HA
  if (nm->t_sweep)
    nsm_cal_delete_rib_sweep_timer (nm);
#endif /* HAVE_HA */
  THREAD_TIMER_OFF (nm->t_sweep);

  if (! CHECK_FLAG (nm->flags, NSM_FIB_RETAIN_RESTART))
    nsm_rib_close (nm);
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
  /* Deinit MPLS */
  if (NSM_CAP_HAVE_MPLS)
    nsm_mpls_deinit (nm);
#endif /* HAVE_MPLS */

#ifdef HAVE_FIREWALL
  nsm_firewall_deinitialize (nm);
#endif /* HAVE_FIREWALL */

#ifdef HAVE_LACPD
  for (ake = nm->ake_list; ake; ake = ake_next)
    {
      ake_next = ake->next;
      nsm_lacp_admin_key_element_free (nm, ake);
    }
  nm->ake_list = NULL;

  if (nm->lacp_admin_key_mgr)
    {
      bitmap_free (nm->lacp_admin_key_mgr);
      nm->lacp_admin_key_mgr = NULL;
    }

  nsm_lacp_delete_all_aggregators (nm);
#endif /* HAVE_LACPD */

#ifdef HAVE_L3
  if (nm->arp)
    nsm_arp_destroy_master (nm);
#endif /* HAVE_L3 */

  /* Delete VRF tables. */
  for (i = vector_max (nm->vr->vrf_vec) - 1; i >= 0; i--)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)))
      nsm_vrf_destroy (nm->vr, iv->proto);

  if (! IS_APN_VR_PRIVILEGED (vr))
    NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
      nsm_server_send_vr_delete (nse, nm->vr);

#ifdef HAVE_RTADV
  /* Finish the router advertisement service.  */
  if (NSM_CAP_HAVE_IPV6)
    nsm_rtadv_finish (nm);
#endif /* HAVE_RTADV */

  /* Free description. */
  if (nm->desc)
    XFREE (MTYPE_NSM_DESC, nm->desc);

#ifdef HAVE_NSM_IF_PARAMS
  /* Free the interface parameter list.  */
  nsm_if_params_clean (nm);
#endif /* HAVE_NSM_IF_PARAMS */

#ifdef HAVE_L2
  master = nsm_bridge_get_master(nm);
  if (master)
    {
      if (master->l2mcast)
        {
          /* igmp and mld instances are already deleted in nsm_vrf_destroy. Don't need to delete in nsm_bridge_deinit */
#ifdef HAVE_IGMP_SNOOP
          master->l2mcast->igmp_inst = NULL;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
          master->l2mcast->mld_inst = NULL;
#endif /* HAVE_MLD_SNOOP */
        }
    }
  nsm_bridge_deinit (nm);
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD
  nsm_l2_oam_master_deinit (nm);
#endif /* HAVE_ONMD */

  vr->proto = NULL;

#ifdef HAVE_HA
  nsm_cal_delete_nsm_master (nm);
#endif /* HAVE_HA */

#ifdef HAVE_L3
#ifdef HAVE_PBR
  nsm_pbr_route_map_if_finish (nm);
#endif /* HAVE_PBR */
#endif /* HAVE_L3 */

  /* Free phyEntityList */
  list_delete (nm->phyEntityList);
  /* Free nsm master structure */
  XFREE (MTYPE_NSM_MASTER, nm);

  /* Set VR FLAG, when Shutdown is not in Progress */
  if (!CHECK_FLAG (ng->flags, NSM_SHUTDOWN))
    SET_FLAG (vr->flags, LIB_FLAG_DELETE_VR_CONFIG_FILE);

  /* Free VR. */
  apn_vr_delete (vr);

  return;
}

struct nsm_master *
nsm_master_lookup_by_id (struct lib_globals *zg, u_int32_t id)
{
  struct apn_vr *vr;

  vr = apn_vr_lookup_by_id (zg, id);
  if (vr != NULL)
    return (struct nsm_master *)vr->proto;

  return NULL;
}

struct nsm_master *
nsm_master_lookup_by_name (struct lib_globals *zg, char *name)
{
  struct apn_vr *vr;

  vr = apn_vr_lookup_by_name (zg, name);
  if (vr != NULL)
    return (struct nsm_master *)vr->proto;

  return NULL;
}

