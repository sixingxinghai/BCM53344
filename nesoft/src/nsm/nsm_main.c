/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "if.h"
#include "log.h"
#include "thread.h"
#include "filter.h"
#include "prefix.h"
#include "version.h"
#include "stream.h"
#include "hash.h"
#include "lib_mtrace.h"
#ifdef HAVE_MPLS
#include "mpls.h"
#endif /* HAVE_MPLS */
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_LICENSE_MGR
#include "licmgr.h"
#endif /* HAVE_LICENSE_MGR */

#include "nsm/nsmd.h"

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif /* HAVE_QOS */
#ifdef HAVE_L3
#include "nsm/rib/rib.h"
#endif /* HAVE_L3 */

#include "nsm/nsm_debug.h"
#include "nsm/nsm_server.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_router.h"
#ifdef HAVE_L3
#include "nsm/rib/nsm_nexthop.h"
#endif /* HAVE_L3 */
#include "nsm/nsm_fea.h"
#include "nsm/nsm_api.h"
#ifdef HAVE_NSM_IF_ARBITER
#include "nsm/nsm_if_arbiter.h"
#endif /* HAVE_NSM_IF_ARBITER */
#ifdef HAVE_VRF
#include "nsm/rib/nsm_table.h"
#endif /* HAVE_VRF */
#ifdef HAVE_VRRP
#include "nsm/vrrp/vrrp_init.h"
#endif /* HAVE_VRRP */
#ifdef HAVE_IPSEC
#include "nsm/ipsec/ipsec.h"
#endif /* HAVE_IPSEC */
#ifdef HAVE_FIREWALL
#include "nsm/firewall/nsm_firewall.h"
#include "nsm/firewall/nsm_firewall_api.h"
#endif /* HAVE_FIREWALL */
#ifdef HAVE_SNMP
#include "nsm/nsm_snmp.h"
#endif /* HAVE_SNMP */
#include "nsm/nsm_api.h"

#ifdef HAVE_L2
#include "nsm_bridge.h"
#include "nsm_pmirror.h"
#ifdef HAVE_WMI
#include "wmi_server.h"
#include "nsm_l2_wmi.h"
#endif /* HAVE_WMI */
#endif /* HAVE_L2 */

#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#endif /* HAVE_VLAN */

#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */

#ifdef HAVE_MPLS
#include "nsm_lp_serv.h"
#include "nsm_mpls.h"

#ifdef HAVE_SNMP
#include "nsm_mpls_rib.h"
#include "nsm_mpls_snmp.h"
#endif /* HAVE_SNMP */
#endif /* HAVE_MPLS */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
#include "igmp.h"
#ifdef HAVE_MCAST_IPV4
#include "nsm_mcast4.h"
#endif /* HAVE_MCAST_IPV4 */
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */
#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
#include "mld.h"
#ifdef HAVE_MCAST_IPV6
#include "nsm_mcast6.h"
#endif /* HAVE_MCAST_IPV6 */
#endif /* HAVE_MCAST_IPV6 || HAVE_MLD_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
#include "nsm_l2_mcast.h"
#endif /* HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP */

#ifdef HAVE_TE
#include "nsm_qos_serv.h"
#endif /* HAVE_TE */

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif /* HAVE_QOS */

#ifdef HAVE_ONMD
#include "nsm_l2_oam_cli.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_RMM
#include "nsm_rd.h"
#endif /* HAVE_RMM */

#ifdef HAVE_L2
#include "nsm_bridge.h"

#ifdef HAVE_GVRP
#include "gvrp.h"
#endif /* HAVE_GVRP */

#ifdef HAVE_GMRP
#include "gmrp.h"
#endif /* HAVE_GMRP */
#endif /* HAVE_L2 */

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif

#ifdef HAVE_HA
#include "cal_api.h"
#include "lib_cal.h"
#include "nsm_cal.h"
#ifdef HAVE_L3
#include "nsm_rib_cal.h"
#endif /* HAVE_L3 */
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "fm/lib_fm_api.h"
#include "smi_nsm_server.h"
#include "smi_server.h"
#include "smi_nsm_fm.h"
#endif /* HAVE_SMI */

#ifdef HAVE_IMI
#include "imi_client.h"
#endif /* HAVE_IMI */

#ifdef HAVE_BFD
#include "nsm/nsm_bfd.h"
#endif /* HAVE_BFD */

#ifdef HAVE_GMPLS
#include "nsm_gmpls_ifcli.h"
#endif /* HAVE_GMPLS */
#include "nsm_connected.h"

#ifdef HAVE_LACP
#include "nsm/lacp/nsm_lacp.h"
#endif /*HAVE_LACP*/

#ifdef ENABLE_HAL_PATH
struct hal_nsm_callbacks hal_nsm_callbacks;
#endif /*ENABLE_HAL_PATH*/
/* process id. */
pid_t nsm_old_pid;
pid_t nsm_pid;
pid_t nsm_parent_pid;

/* NSM Feature Capability variables. */
u_char nsm_cap_have_vrrp;
u_char nsm_cap_have_vrf;
u_char nsm_cap_have_mpls;
u_char nsm_cap_have_ipv6;

#ifdef HAVE_RMM
result_t nsm_any2init (void);
#endif /* HAVE_RMM */

/* Library Globals. */
struct lib_globals *nzg = NULL;

/* NSM Globals pointer. */
struct nsm_globals *ng = NULL;

#ifdef HAVE_L3
/* RIB Null. */
extern struct rib RibNull;
#endif /* HAVE_L3 */

s_int32_t
nsm_globals_init (struct lib_globals *zg)
{
#ifdef HAVE_L3
  u_int32_t idx;
#endif /* HAVE_L3 */

  zg->proto = XCALLOC (MTYPE_NSM_GLOBAL, sizeof (struct nsm_globals));

  if (! (ng = zg->proto))
    return NSM_ERR_MEM_ALLOC_FAILURE;

#ifdef HAVE_L3
  /* Restart initialization.  */
  for (idx = 0; idx < APN_PROTO_MAX; idx++)
    {
      ng->restart[idx].proto_id = idx;

#ifdef HAVE_HA
      nsm_cal_create_gr_restart (&ng->restart[idx]);
#endif /* HAVE_HA */
    }

  /* Initialize Null RIB. */
  pal_mem_set (&RibNull, 0, sizeof (struct rib));
#endif /* HAVE_L3 */

#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP                \
    || defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
  ng->iobuf = stream_new (
#if ! defined HAVE_MCAST_IPV4 && ! defined HAVE_MCAST_IPV6
                          NSM_L2_MCAST_BUFFER_SIZE
#elif ! defined HAVE_MCAST_IPV4
                          NSM_MCAST6_BUFFER_SIZE
#else
                          NSM_MCAST_BUFFER_SIZE
#endif /* ! (HAVE_MCAST_IPV4) */
                          );

  if (! ng->iobuf)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  ng->obuf = stream_new (
#if ! defined HAVE_MCAST_IPV4 && ! defined HAVE_MCAST_IPV6
                          NSM_L2_MCAST_BUFFER_SIZE
#elif ! defined HAVE_MCAST_IPV4
                          NSM_MCAST6_BUFFER_SIZE
#else
                          NSM_MCAST_BUFFER_SIZE
#endif /* ! (HAVE_MCAST_IPV4) */
                          );
  if (! ng->obuf)
    return NSM_ERR_MEM_ALLOC_FAILURE;

#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP
          || HAVE_MCAST_IPV6 || HAVE_MLD_SNOOP */

#ifdef HAVE_HA
  nsm_cal_create_nsm_globals (ng);
#endif /* HAVE_HA */

  return NSM_SUCCESS;
}

s_int32_t
nsm_globals_deinit (struct lib_globals *zg)
{
  ng = zg->proto;

  if (! ng)
    return NSM_SUCCESS;

  if (ng->iobuf)
    stream_free (ng->iobuf);

  if (ng->obuf)
    stream_free (ng->obuf);

  XFREE (MTYPE_NSM_GLOBAL, ng);

  zg->proto = NULL;

  /* There is no need for delete of the HA checkpoint */
  return NSM_SUCCESS;
}

void
nsm_terminate (struct lib_globals *zg)
{
  struct nsm_master *nm;
  struct apn_vr *vr;
  int i;

  SET_FLAG (ng->flags, NSM_SHUTDOWN);

#ifdef HAVE_RMM
  /* Gracefully shutdown the RMM connection. */
  nsm_rmm_shutdown ();

  /* Change state to initialize to gracefully bring down protocol tasks. */
  nsm_any2init ();
#endif /* HAVE_RMM */

#ifdef HAVE_VRRP
  vrrp_close_all (zg);
#endif

  /* Notify the clients with NSM Shutdown message
   * when NSM is terminated
   * */
  nsm_server_send_nsm_status (NSM_MSG_NSM_SERVER_SHUTDOWN);

  /* Shutdown all VR instances and interfaces. */
  for (i = vector_max (zg->vr_vec) - 1; i >= 0; i--)
    if ((vr = vector_slot (zg->vr_vec, i)))
      {
#ifdef HAVE_TE
        nsm_if_delete_interface_all (vr);
#endif /* HAVE_TE */

        nm = nsm_master_lookup_by_id (zg, vr->id);

#ifdef HAVE_VLAN
        if (nm)
          nsm_vlan_cleanup (nm);
#endif /* HAVE_VLAN  */

#ifdef HAVE_QOS
        if (nm)
          nsm_qos_deinit (nm);
#endif /* HAVE_QOS */

        /* Finish NSM master. */
        nsm_master_finish (vr);
      }

#ifdef HAVE_L2
  nsm_l2_deinit (zg);
#endif /* HAVE_L2 */

#ifdef HAVE_SNMP
  nsm_snmp_deinit (zg);
#endif /* HAVE_SNMP */

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    /* De-init top level MPLS data. */
    nsm_mpls_main_deinit ();
#endif /* HAVE_MPLS */

#ifdef HAVE_SMI
  /* Finish API server. */
  smi_server_finish (ng->nsm_smiserver);
#endif /* HAVE_SMI */

  /* Finish NSM server. */
  nsm_server_finish (ng->server);

  /* De-init NSM Global data */
  nsm_globals_deinit (zg);

  nsm_fea_deinit (zg);

  return;
}

#ifdef HAVE_VRRP
#ifdef HAVE_LICENSE_MGR
int
nsm_vrrp_lic_update (struct thread *t)
{
  struct lib_globals *zptr;
  zptr = THREAD_ARG(t);
  /***** Renew license key (license heartbeat) *****/
  if (LS_SUCCESS != LML_UPDATE(zptr->handle))
    {
      zptr->t_check_expiration = NULL;
      vrrp_close_all (nzg);
    }
  else
    zptr->t_check_expiration = thread_add_timer (zptr, nsm_vrrp_lic_update,
                                                 (void *) zptr, LIC_MGR_CHECK_INTERVAL);
  return 0;
}
#endif /* HAVE_LICENSE_MGR */
#endif /* HAVE_VRRP */

/* NSM Feature Capability check. */
static void
nsm_feature_capability_check ()
{
#ifdef HAVE_LICENSE_MGR
  int retval, retval1, retval2;
  char *buf[3];

  /* Check VRRP license. */
  buf[0] = LIC_MGR_FEATURE_VRRP;
  retval = lic_mgr_get (buf, 1, LIC_MGR_VERSION_IPV4, nzg);
  if (retval == LS_SUCCESS)
    nsm_cap_have_vrrp = 1;
  else
    nsm_cap_have_vrrp = 0;

  /* Check MPLS license. */
  buf[0] = LIC_MGR_FEATURE_MPLS;
  ng->mpls_lic.zg = nzg;
  retval = nsm_lic_mgr_get (buf, 1, LIC_MGR_VERSION_IPV4, &(ng->mpls_lic));
  buf[1] = LIC_MGR_FEATURE_LDP;
  ng->vpn_lic.zg = nzg;
  retval1 = nsm_lic_mgr_get (buf, 2, LIC_MGR_VERSION_IPV4, &(ng->vpn_lic));

  buf[1] = LIC_MGR_FEATURE_RSVP;
  ng->vpn_lic.zg = nzg;
  retval2 = nsm_lic_mgr_get (buf, 2, LIC_MGR_VERSION_IPV4, &(ng->vpn_lic));
  if (retval == LS_SUCCESS
      && (retval1 == LS_SUCCESS || retval2 == LS_SUCCESS))
    {
      nsm_cap_have_vrf = 1;
      nsm_cap_have_mpls = 1;
    }
  else
    {
      nsm_cap_have_vrf = 0;
      nsm_cap_have_mpls = 0;
    }

  /* Check PIM license. */
  buf[0] = LIC_MGR_FEATURE_PIMSM;
  ng->pim_lic.zg = nzg;
  retval = nsm_lic_mgr_get (buf, 1, LIC_MGR_VERSION_IPV4, &(ng->pim_lic));
  if (retval == LS_SUCCESS)
    {
      ;
    }
  else
    {
      ;
    }

  /* Check IPv6 license. */
  buf[0] = LIC_MGR_FEATURE_IPV6;
  ng->ipv6_lic.zg = nzg;
  retval = nsm_lic_mgr_get (buf, 1, LIC_MGR_VERSION_IPV4, &(ng->ipv6_lic));
  if (retval == LS_SUCCESS)
    nsm_cap_have_ipv6 = 1;
  else
    nsm_cap_have_ipv6 = 0;
#else
  nsm_cap_have_vrrp = 1;
  nsm_cap_have_vrf = 1;
  nsm_cap_have_mpls = 1;
  nsm_cap_have_ipv6 = 1;
#endif /* HAVE_LICENSE_MGR. */
}

/* Start up NSM.  */
int
nsm_start (u_int16_t batch_mode, u_int16_t daemon_mode,
           u_int16_t keep_kernel_mode, char *config_file,
           u_int16_t vty_port, u_int16_t fib_mode,
           char *progname, u_int32_t parent_pid,
           u_int16_t ha_cold)
{
  struct nsm_master *nm;
  struct thread thread;
  result_t ret;
#ifdef HAVE_HA
  HA_RC cal_rc;
  COMMSG_HAN cm_han;
  CAL_STARTUP_TYPE ha_startup_type;
#endif /* HAVE_HA */
  /* Initialize memory. */
  memory_init (APN_PROTO_NSM);

  /* Initialize zglobals. */
  nzg = lib_create(progname);
  if (nzg == NULL)
    return -1;

  /* First of all we need logging init. */
  nzg->protocol = APN_PROTO_NSM;
  nzg->vr_instance = 0;
  nzg->log = openzlog (nzg, nzg->vr_instance, APN_PROTO_NSM, LOGDEST_MAIN);

#ifdef TEST_FM
  pal_signal_set(SIGUSR1, siguser_handler);
#endif

#ifdef HAVE_SMI
  /* Initialize Fault management */
  nsm_smi_fm_init (nzg);

#endif  /* HAVE_SMI */

  /* Exit when NSM is working in batch mode. */
  if (batch_mode)
    return -1;

  /* Needed for routing socket. */
  nsm_old_pid = pal_get_process_id ();

  /* Daemonize. */
  if (daemon_mode)
    pal_daemonize (0, 0);

  /* Needed for routing socket. */
  nsm_pid = pal_get_process_id ();

#ifdef HAVE_PID
  PID_REGISTER (PATH_NSM_PID);
#endif /* HAVE_PID */

#ifdef HAVE_HA
  ha_startup_type = (ha_cold) ? CAL_STARTUP_TYPE_COLD : CAL_STARTUP_TYPE_AUTO;
  cal_rc = cal_init(nzg, ha_startup_type, NULL);
  if (cal_rc != HA_RC_OK)
    {
      return RESULT_ERROR;
    }
  cal_reg_new_active_cb (HA_CAL_PTR_GET(nzg), nsm_cal_switchover_cb);

  /* We need to register all CAL data types. */
  /* ... */
  nsm_cal_reg_all_types ();
#endif /* HAVE_HA */

  ret = lib_start (nzg);
  if (ret < 0)
    {
      lib_stop (nzg);
      return ret;
    }

  vport_master_init();
  service_master_init();//added by fxg

  /* NSM globals init. */
  nsm_globals_init (nzg);

  /* Check NSM feature capability. */
  nsm_feature_capability_check ();

#ifdef HAVE_VRF
  if (NSM_CAP_HAVE_VRF)
    SET_FLAG (ng->flags, NSM_VRF_NO_FIB);
#endif /* HAVE_VRF */

  /* Vty related initialize. */
  host_vty_init (nzg);

  nsm_cli_init (nzg);

#ifdef HAVE_WMI
  nsm_l2_wmi_init (nzg);
#endif /* HAVE_WMI */

#ifdef HAVE_SNMP
  nsm_snmp_init (nzg);
#endif /* HAVE_SNMP */

  /* PacOS related initialize. */
  nsm_server_init (nzg);

#ifdef HAVE_HA
  /* Hookup the COMMSG to NSM and vice versa. */
  cm_han =  commsg_reg_tp(nzg, ng->server,
                          nsm_server_commsg_getbuf,
                          nsm_server_commsg_send);
  ret = nsm_server_commsg_init(ng->server, commsg_recv, cm_han);
  if (ret) {
    return -1;
  }
  /* Once we have the message transport handle, let the CAL to know. */
  cal_audit_attach_commsg(HA_CAL_PTR_GET(nzg), cm_han);
  /* Register for audits */
  nsm_cal_audit_init ();
#endif /* HAVE_HA */

  /* NSM master init for PVR. */
  nm = nsm_master_init (apn_vr_get_privileged (nzg));
  if (! nm)
    return -1;

  nsm_if_init (nzg);

#ifdef HAVE_IPSEC
  ipsec_init (nzg);
#endif /* HAVE_IPSEC */
#ifdef HAVE_FIREWALL
  nsm_firewall_init (nzg);
  nsm_firewall_flush_rules ();
#endif /* HAVE_FIREWALL */
#ifdef HAVE_CUSTOM1
  swp_init ();
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_VRRP
  if (NSM_CAP_HAVE_VRRP)
    vrrp_cli_init (nzg);
#endif /* HAVE_VRRP */

#ifdef HAVE_L2
  nsm_l2_init (nzg);
#endif /* HAVE_L2 */

#ifdef HAVE_GMPLS
  nsm_gmpls_if_cli_init (nzg);
#endif /*HAVE_GMPLS */

#ifdef HAVE_RMM
  /* Initialize redundancy support. */
  nsm_rmm_init ();
#endif /* HAVE_RMM */

#ifdef HAVE_L2LERN
  mac_acl_list_master_init(nm);
#endif /* HAVE_L2LERN */

  ret = 0;         /* init before license check */
  access_list_init (nzg);

  /* Initialize route-map. */
  route_map_cli_init (nzg);

  /* Initialize SNMP community. */
  snmp_community_cli_init (nzg);


#ifdef HAVE_SNMP
  nsm_phy_entity_create (nm, CLASS_CHASSIS, NULL);
#endif /* HAVE_SNMP */

  /* Initialize Forwarding Element(FE) Layer */
  ret = nsm_fea_init (nzg);
  if (ret < 0)
    {
      zlog_warn (nzg, "Cannot initialize routing socket. Exiting!!!");
      return -1;
    }

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    {
      /* Initialize connection to the MPLS Forwarder. */
      ret = nsm_mpls_main_init ();

      if (ret < 0)
        return ret;
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_QOS
  nsm_qos_init (nm);
#endif /* HAVE_QOS */

#ifdef HAVE_ONMD
  nsm_efm_oam_cli_init (nzg);
#endif /* HAVE_ONMD */

#ifdef HAVE_CUSTOM1
#ifdef HAVE_VLAN
  nsm_vlan_init (0);
#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_NSM_IF_ARBITER
  /* Initialize Interface Arbiter.  */
  nsm_if_arbiter_init (NSM_ZG);
#endif /* HAVE_NSM_IF_ARBITER */

#ifdef HAVE_SMI
  nsm_smi_server_init (nzg);
#endif /* HAVE_SMI */

#ifdef HAVE_HA
  zlog_info (nzg, "Calling cal_start(CAL_STARTUP_DONE)");
  HA_RC rc = cal_start(HA_CAL_PTR_GET(nzg), CAL_STARTUP_DONE);
  if (rc == HA_RC_STARTUP_DONE)
    zlog_info (nzg, "CAL_STARTUP_DONE OK - ");
  else if (rc == HA_RC_CONFIG_DONE)
    zlog_info (nzg, "CAL_STARTUP_DONE: ACTIVE startup configuration");
  else
    {
      zlog_err (nzg, "nsm_start: cal_start failure");
      return (-1);
    }
#endif /* HAVE_HA */

  host_config_start (NSM_ZG, config_file, vty_port);
#ifdef HAVE_IMI
  imi_client_register_event_ntf_cb (NSM_ZG, IMI_CLT_NTF_CONFIG_DONE,
                                    nsm_imi_config_completed);
#endif /* HAVE_IMI */

#ifdef HAVE_APRESIA
  /* Start RIB kernel sync. */
  thread_execute (nm->zg, nsm_rib_kernel_sync_timer, nm, 0);
#endif /* HAVE_APRESIA */

  while (thread_fetch (nzg, &thread))
    {
      thread_call (&thread);

#ifdef HAVE_HA
      cal_txn_commit (HA_CAL_PTR_GET(nzg));
#endif /* HAVE_HA */
    }

  /* Not reached... */
  return 0;
}

#ifdef ENABLE_HAL_PATH
void
hal_nsm_cb_register()
{
#ifdef HAVE_L3
  hal_nsm_callbacks.nsm_connected_delete_ipv4_cb = nsm_connected_delete_ipv4;
  hal_nsm_callbacks.nsm_connected_add_ipv4_cb = nsm_connected_add_ipv4;
#endif /* HAVE_L3 */
#ifdef HAVE_IPV6
  hal_nsm_callbacks.nsm_connected_delete_ipv6_cb = nsm_connected_delete_ipv6;
  hal_nsm_callbacks.nsm_connected_add_ipv6_cb = nsm_connected_add_ipv6 ;
#endif /* HAVE_IPV6 */
  hal_nsm_callbacks.nsm_if_delete_update_cb = nsm_if_delete_update;
  hal_nsm_callbacks.nsm_if_ifindex_update_cb = nsm_if_ifindex_update;
  hal_nsm_callbacks.nsm_if_add_update_cb = nsm_if_add_update;
  hal_nsm_callbacks.nsm_if_up_cb = nsm_if_up;
  hal_nsm_callbacks.nsm_if_down_cb = nsm_if_down;
  hal_nsm_callbacks.nsm_server_if_up_update_cb = nsm_server_if_up_update;
  hal_nsm_callbacks.nsm_server_if_down_update_cb = nsm_server_if_down_update;
  hal_nsm_callbacks.nsm_bridge_if_send_state_sync_req_wrap_cb = nsm_bridge_if_send_state_sync_req_wrap;

  hal_nsm_lib_cb_register (&hal_nsm_callbacks);
}
#endif /* ENABLE_HAL_PATH */

/* Stop NSM process.  */
void
nsm_stop (void)
{
  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (nzg);

  /* Terminate NSM.  */
  nsm_terminate (nzg);

#ifdef HAVE_PID
  PID_REMOVE (PATH_NSM_PID);
#endif /* HAVE_PID */

  /* Stop the system. */
  lib_stop (nzg);
  nzg = NULL;
}
