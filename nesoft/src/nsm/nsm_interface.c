/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "cli.h"
#include "sockunion.h"
#include "prefix.h"
#include "log.h"
#include "table.h"
#include "thread.h"
#include "nsm_message.h"
#include "ptree.h"
#include "bitmap.h"
#include "hash.h"
#include "lib_mtrace.h"
#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
#include "igmp.h"
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */
#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
#include "mld.h"
#endif /* HAVE_MCAST_IPV6 || HAVE_MLD_SNOOP */

#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */

#ifdef HAVE_MPLS
#include "mpls.h"
#include "label_pool.h"
#include "admin_grp.h"
#include "qos_common.h"
#include "mpls_common.h"
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#endif /* HAVE_MPLS */


#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "nsmd.h"
#ifdef HAVE_L2
#include "nsm_bridge.h"
#include "nsm_bridge_cli.h"
#include "nsm_pmirror.h"
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#endif /* HAVE_VLAN */
#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_L2 */
#include "nsm_interface.h"
#include "nsm_connected.h"
#include "nsm_router.h"
#ifdef HAVE_L3
#include "rib/nsm_table.h"
#include "rib/rib.h"
#include "rib/nsm_redistribute.h"
#include "rib/nsm_static_mroute.h"
#endif /* HAVE_L3 */
#include "nsm_server.h"
#include "nsm_debug.h"
#include "nsm_api.h"
#include "nsm/nsm_cli.h"
#include "nsm_fea.h"
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
#include "nsm_l2_mcast.h"
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_NSM_IF_UNNUMBERED
#include "nsm/nsm_if_unnumbered.h"
#endif /* HAVE_NSM_IF_UNNUMBERED */

#ifdef HAVE_NSM_IF_ARBITER
#include "nsm/nsm_if_arbiter.h"
#endif /* HAVE_NSM_IF_ARBITER */

#ifdef HAVE_TUNNEL
#include "nsm/tunnel/nsm_tunnel.h"
#endif /* HAVE_TUNNEL */

#ifdef HAVE_VRRP
#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_api.h"
#include "nsm/vrrp/vrrp_cli.h"
#endif /* HAVE_VRRP */

#ifdef HAVE_RTADV
#include "nsm/nsm_rtadv.h"
#endif /* HAVE_RTADV */

#ifdef HAVE_TE
#include "nsm_mpls_qos.h"
#include "nsm_qos_serv.h"
#endif /* HAVE_TE */

#ifdef HAVE_MPLS
#include "nsm_lp_serv.h"
#include "nsm_mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */

#ifdef HAVE_GMPLS
#include "nsm/gmpls/nsm_gmpls.h"
#include "nsm_gmpls_ifcli.h"
#include "nsm_gmpls_if.h"
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS
#include "nsm/mpls/nsm_mpls.h"
#include "nsm/mpls/nsm_mpls_api.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_MCAST_IPV4
#include "nsm_mcast4.h"
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#include "nsm_mcast6.h"
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_LACPD
#include "nsm_lacp.h"
#include "nsm_static_aggregator_cli.h"
#endif /* HAVE_LACPD */

#ifdef HAVE_HAL
#include "hal_if.h"
#endif /* HAVE_HAL */

#ifdef HAVE_IPSEC
#include "nsm/ipsec/ipsec_api.h"
#include "nsm/ipsec/ipsec_cli.h"
#endif /* HAVE_IPSEC */
#ifdef HAVE_FIREWALL
#include "nsm/firewall/nsm_firewall_api.h"
#endif /* HAVE_FIREWALL */

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif /* HAVE_QOS */

#ifdef HAVE_HA
#include "lib_cal.h"
#include "nsm_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#endif /* HAVE_QOS */

#ifdef HAVE_L2
#include "nsm_stats_api.h"
#endif /* HAVE_L2 */

#ifdef HAVE_PBR
#include "nsm_pbr_filter.h"
#endif /* HAVE_PBR */

#include "nsm_ifma.h"

#ifdef HAVE_BFD
#include "nsm_bfd.h"
#endif /* HAVE_BFD */

/* Prototypes. */
int nsm_if_bind_clean (struct interface *);
#ifdef HAVE_VPLS
int nsm_vpls_if_add_send (struct interface *, u_int16_t);
int nsm_vpls_if_delete_send (struct interface *);
#endif /* HAVE_VPLS */
#ifdef HAVE_L3
struct connected *nsm_if_connected_primary_lookup (struct interface *);
#endif /* HAVE_L3 */

#ifdef HAVE_NSM_IF_PARAMS
void nsm_if_params_pending_proc (struct nsm_master *, char *);
#endif /* HAVE_NSM_IF_PARAMS */

#ifdef HAVE_CUSTOM1
void nsm_if_custom1_init (struct cli_tree *);
#endif /* HAVE_CUSTOM1 */

const char *mpls_if_commands = "MPLS";
const char *ipv4_if_commands = "IPv4";
const char *ipv6_if_commands = "IPv6";
const char *l3_if_commands = "L3";


/* Add new NSM interface.  */
int
nsm_if_add (struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_master *nm;
#if !defined (HAVE_L3) && defined (HAVE_L2)
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
#endif /* !defined (HAVE_L3) && defined (HAVE_L2) */

#ifdef HAVE_VLAN_CLASS
  int ret = 0;
#endif /* HAVE_VLANC_LASS */

  nm = ifp->vr->proto;
  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    {
      zif = XMALLOC (MTYPE_NSM_IF, sizeof (struct nsm_if));
      pal_mem_set (zif, 0, sizeof (struct nsm_if));

      ifp->info = zif;
      zif->ifp = ifp;
      /* Initialize the NSM_IFMA_VEC container of MAC addresses. */
#ifdef HAVE_HA
      nsm_cal_create_nsm_if (zif);
#endif /* HAVE_HA */
      nsm_ifma_init(zif);

#ifdef HAVE_GMPLS
      ifp->gifindex = NSM_GMPLS_GIFINDEX_GET ();
      avl_create2 (&zif->cctree, 0, gmpls_control_channel_index_cmp);
#endif /* HAVE_GMPLS */
    }
  else
    {
#ifdef HAVE_HA
      nsm_cal_modify_nsm_if (zif);
#endif /* HAVE_HA */
      nsm_ifma_update(zif);
    }

  /* Set default interface type as L3. */
  zif->type = ifp->type;

  ifp->arp_ageing_timeout = NSM_ARP_AGE_TIMEOUT_DEFAULT;

#ifdef HAVE_SNMP
  /* Enable Traps */
  ifp->if_linktrap = NSM_IF_TRAP_ENABLE;
#endif /* HAVE_SNMP */

#ifdef HAVE_L3
  if(ifp->type == NSM_IF_TYPE_L3)
    {
      NSM_INTERFACE_SET_L3_MEMBERSHIP (ifp);
    }
#endif /* HAVE_L3 */

#ifdef HAVE_GMPLS
  ifp->phy_prop.encoding_type = GMPLS_ENCODING_TYPE_PACKET;
  ifp->phy_prop.switching_cap = GMPLS_SWITCH_CAPABILITY_FLAG_PSC1;
#endif /* HAVE_GMPLS */

#ifdef HAVE_L2
  if(ifp->type == NSM_IF_TYPE_L2)
    {
      NSM_INTERFACE_SET_L2_MEMBERSHIP (ifp);
#if !defined (HAVE_L3) && defined (HAVE_L2)
      nm = ifp->vr->proto;
      if ((master = nsm_bridge_get_master (nm)) == NULL)
        return -1;

      bridge = nsm_get_default_bridge (master);

      nsm_bridge_if_init_switchport (bridge, ifp, NSM_VLAN_DEFAULT_VID);
#endif /* !defined (HAVE_L3) && defined (HAVE_L2) */
#if defined (HAVE_STPD) || defined(HAVE_RSTPD) || defined(HAVE_MSTPD) || defined(HAVE_RPSVT_PLUS)
#ifdef HAVE_DEFAULT_BRIDGE
      nsm_add_if_to_def_bridge (ifp);
#endif /* HAVE_DEFAULT_BRIDGE */
#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD || HAVE_RPSVT_PLUS */
    }
#ifdef HAVE_VLAN_CLASS
  ret = avl_create(&zif->group_tree, 0, nsm_vlan_class_if_group_id_cmp);
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_L2 */

#ifdef HAVE_TE
  zif->qos_if = nsm_qos_serv_if_init (ifp);
  if (! zif->qos_if)
    {
      XFREE (MTYPE_NSM_IF, zif);
      ifp->info = NULL;
      return -1;
    }
#endif /* HAVE_TE */

#ifdef HAVE_VRRP
  /* Initialize VRRP.  */
  IF_NSM_CAP_HAVE_VRRP
    vrrp_if_init (ifp);
#endif /* HAVE_VRRP */

#ifdef HAVE_MCAST_IPV4
  zif->mcast_ttl = MCAST_VIF_TTL_INVALID;
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Initialize the unnumbered interface.  */
  nsm_if_unnumbered_init (ifp);
#endif /* HAVE_NSM_IF_UNNUMBERED */

#ifdef HAVE_L2
  zif->bridge_static_mac_config = list_new();
#endif /* HAVE_L2 */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  nsm_l2_mcast_if_add(&zif, ifp);
#endif /* HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP */

#ifdef HAVE_ONMD
  nsm_oam_lldp_if_add (ifp);
#endif /* HAVE_ONMD */

  return 0;
}

/* Delete NSM interface.  */
int
nsm_if_delete (struct interface *ifp, struct nsm_vrf *nv)
{
   struct nsm_if *zif;
#ifdef HAVE_VPLS
   struct nsm_mpls_if *mif;
   struct nsm_mpls_vc_info *vc_info;
   struct listnode *ln, *next;
#endif /* HAVE_VPLS */

  if (! ifp)
    return 0;

  zif = (struct nsm_if *)ifp->info;

#ifdef HAVE_LACPD
  if ((zif) && (NSM_INTF_TYPE_AGGREGATOR(ifp)) &&
      (listcount(zif->agg.info.memberlist) > 0))
    return 0;
#endif /* HAVE_LACPD */

  if (zif)
    {
#ifdef HAVE_L2
      nsm_bridge_static_fdb_config_free (ifp);
#endif
#ifdef HAVE_RTADV
      if (NSM_CAP_HAVE_IPV6)
        nsm_rtadv_if_free (ifp);
#endif /* HAVE_RTADV */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
      nsm_l2_mcast_if_delete(&zif);
#endif /* HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP */

#ifdef HAVE_HA
      nsm_cal_delete_nsm_if (zif);
#endif /* HAVE_HA */

#ifdef HAVE_ONMD
      nsm_oam_lldp_if_delete (ifp);
#endif /* HAVE_ONMD */

      nsm_ifma_close(zif);

#ifdef HAVE_LACPD
      if ((zif) && (!(NSM_INTF_TYPE_AGGREGATOR(ifp)) ||
          !(listcount(zif->agg.info.memberlist) > 0)))
        {
          XFREE (MTYPE_NSM_IF, ifp->info);
          ifp->info = NULL;
          zif = NULL;
        }
#else
      XFREE (MTYPE_NSM_IF, ifp->info);
      ifp->info = NULL;
      zif = NULL;
#endif /* HAVE_LACPD */
    }

#ifdef HAVE_L3
  ROUTER_ID_UPDATE_TIMER_ADD (nv);
#endif /* HAVE_L3 */

#ifdef HAVE_VPLS
  if ((mif = nsm_mpls_if_lookup_in (nv->nm, ifp)))
    {
      for (ln = LISTHEAD (mif->vpls_info_list); ln; ln = next)
        {
          /* Save next pointer since vc_info might be deleted */
          next = ln->next;
          vc_info = ln->data;
          if (vc_info == NULL)
            continue;

          if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
               CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
              vc_info->u.vpls)
            {
              nsm_vpls_interface_delete (vc_info->u.vpls, ifp, vc_info);
              vc_info->u.vpls = NULL;
              UNSET_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS);
              UNSET_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN);
              nsm_mpls_vc_info_del (mif, mif->vpls_info_list, vc_info);
              nsm_mpls_vc_info_free (vc_info);
            }
        }
      if (LISTCOUNT (mif->vpls_info_list) == 0)
        {
          UNSET_FLAG (ifp->bind, NSM_IF_BIND_VPLS);
          UNSET_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN);
        }
    }
#endif /* HAVE_VPLS */

#ifdef HAVE_VLAN_CLASS
    nsm_del_all_classifier_groups_from_if(ifp);
    if (zif)
      if (zif->group_tree)
        avl_tree_free (&zif->group_tree,
                       nsm_vlan_classifier_free_if_group);

#endif /* HAVE_VLAN_CLASS */

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Finish the unnumbered interface.  */
  nsm_if_unnumbered_finish (ifp, nv->vrf->vr->id);
#endif /* HAVE_NSM_IF_UNNUMBERED */

  /* XXX-VR if-down is implicitly sent by library. */
#ifdef HAVE_SNMP
  /* Disable Traps */
  ifp->if_linktrap = NSM_IF_TRAP_DISABLE;

#endif /* HAVE_SNMP */

#ifdef HAVE_FIREWALL
  nsm_firewall_delete_interface (nv->nm, ifp);
#endif /* HAVE_FIREWALL */

  /* Delete interface. */
  if_delete (&NSM_ZG->ifg, ifp);

  return 0;
}

#ifdef HAVE_L3
/* Wake up configured address if it is not in current kernel address. */
void
nsm_if_addr_wakeup_ipv4 (struct interface *ifp)
{
  struct connected *ifc;
  struct prefix *p;
  int ret;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    if ((p = ifc->address))
      if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
        if (CHECK_FLAG (ifc->conf, NSM_IFC_CONFIGURED)
#ifdef HAVE_UPDATE_ADDRESS_ON_UNNUMBERED
            || if_is_ipv4_unnumbered (ifp)
#endif /* HAVE_UPDATE_ADDRESS_ON_UNNUMBERED */
            )
          {
            ret = nsm_fea_if_ipv4_address_add (ifp, ifc);
            if (ret < 0)
              {
                if (IS_NSM_DEBUG_KERNEL)
                  zlog_warn (NSM_ZG, "Can't set interface's address: %s",
                             pal_strerror (errno));
                continue;
              }

            SET_FLAG (ifc->conf, NSM_IFC_REAL);
            nsm_server_if_address_add_update (ifp, ifc);

            nsm_connected_up_ipv4 (ifp, ifc);

#ifdef HAVE_HA
            lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */
          }
}

#ifdef HAVE_IPV6
void
nsm_if_addr_wakeup_ipv6 (struct interface *ifp)
{
  struct connected *ifc;
  struct prefix *p;
  int ret;

  for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
    if ((p = ifc->address))
      if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
        if (CHECK_FLAG (ifc->conf, NSM_IFC_CONFIGURED)
#ifdef HAVE_UPDATE_ADDRESS_ON_UNNUMBERED
            || if_is_ipv6_unnumbered (ifp)
#endif /* HAVE_UPDATE_ADDRESS_ON_UNNUMBERED */
            )
          {
            if (!if_is_up (ifp))
              nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

            ret = nsm_fea_if_ipv6_address_add (ifp, ifc);
            if (ret < 0)
              {
                if (IS_NSM_DEBUG_KERNEL)
                  zlog_warn (NSM_ZG, "Can't set interface's address: %s",
                             pal_strerror (errno));
                continue;
              }

            /* Notify to the PMs.  */
            nsm_connected_valid_ipv6 (ifp, ifc);

#ifdef HAVE_VRRP
  /* Update VRRP of IP address addtion. */
  if (NSM_CAP_HAVE_VRRP)
    vrrp_if_add_ipv6_addr (ifp, ifc);
#endif /* HAVE_VRRP */

          }
}
#endif /* HAVE_IPV6 */

void
nsm_if_addr_wakeup (struct interface *ifp)
{
  /* Check if interface is up. */
  if (!if_is_up (ifp))
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_warn (NSM_ZG, "wakeup called for disabled interface %s",
                   ifp->name);
      return;
    }

  /* Wakeup Addresses. */
  nsm_if_addr_wakeup_ipv4 (ifp);

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    nsm_if_addr_wakeup_ipv6 (ifp);
#endif /* HAVE_IPV6 */
}
#endif /* HAVE_L3 */

/* XXX: Note that the current logic in this function
 * is used to hack the binding of a newly discovered
 * interface to a non-default VR
 */
void
nsm_if_add_update_vr (struct interface *ifp)
{
  struct apn_vr *vr = ifp->vr;

  /* Bind the interface to PVR for the first time.
     Otherwise, rebind the interface to the previous VR.  */
  if (vr == NULL)
    vr = apn_vr_get_privileged (nzg);
  else
    {
      /* Remove the pseudo interface from the VR temporarily.  */
      listnode_delete (vr->ifm.if_list, ifp);
      if_vr_table_delete(&vr->ifm, ifp->ifindex);
      if_vrf_table_delete(&ifp->vrf->ifv, ifp->ifindex);
      ifp->vrf = NULL;
      ifp->vr = NULL;
    }

  if_vr_bind (&vr->ifm, ifp->ifindex);
}

/* Handle interface addition */
void
nsm_if_add_update (struct interface *ifp, fib_id_t fib_id)
{
  struct nsm_if *zif;
  struct nsm_vrf *nvrf;
#ifdef HAVE_LACPD
  s_int32_t ret;
#endif /* HAVE_LACPD */

#ifdef HAVE_LACPD
  char name[NSM_BRIDGE_NAMSIZ + 1];
#endif /* HAVE_LACPD */

#ifdef HAVE_VLAN
  int vid;
  int br;
#endif /* HAVE_VLAN */

  zif = (struct nsm_if *)ifp->info;

  /* Hack to bind a new interface to a non-default VR */
  /* Set the ifp->vr from the fib_id parameter so that
   * nsm_if_add_update_vr() will bind the new interface to
   * the required VR
   * XXX: Note that this hack depends upon current logic
   * in nsm_if_add_update_vr(). If the bind function is
   * changed, then this hack also needs to be changed.
   */
  nvrf = nsm_vrf_lookup_by_fib_id (NSM_ZG, fib_id);
  if (nvrf == NULL)
    return;
  ifp->vr = nvrf->vrf->vr;
  ifp->vrf = nvrf->vrf;

  /* VR binding.  */
  nsm_if_add_update_vr (ifp);

  /* Set NSM interface.  */
  if (!zif)
    {
      nsm_if_add (ifp);
      zif = (struct nsm_if *)ifp->info;
    }
  else
    nsm_ifma_update(zif);

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
#ifdef HAVE_TE
      nsm_qos_init_interface (ifp);
#endif /* HAVE_TE */

      nsm_mpls_if_add (ifp);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_TUNNEL
  nsm_tunnel_if_update (ifp);
#endif /* HAVE_TUNNEL */

#ifdef HAVE_LACPD
  /* update bandwidth for aggregate interfaces */
  if (zif && zif->agg.type == NSM_IF_AGGREGATOR)
    {
      nsm_lacp_aggregator_bw_update (ifp);

      ret = hal_lacp_psc_set (ifp->ifindex,
                              zif->agg.psc);

      if (ret < 0)
        zlog_info (NSM_ZG, "PSC set for Aggregator %s failed \n",
                   ifp->name);
    }

  nsm_lacp_if_admin_key_set (ifp);
#endif /* HAVE_LACPD */

  nsm_server_if_add (ifp);

#if defined HAVE_LACPD && defined HAVE_VLAN
   if (zif && zif->agg.type == NSM_IF_AGGREGATOR)
     {
       struct nsm_bridge_master *master
            = nsm_bridge_get_master (ifp->vr->proto);

       pal_mem_set (name, 0, NSM_BRIDGE_NAMSIZ + 1);

       if (master && (ifp->bridge_name[0] != '\0'))
         {
           /* Copy ifp->bridge_name since nsm_bridge_port_delete
            * resets ifp->bridge_name when aggregator is
            * removed from default bridge
            */

           pal_strncpy (name, ifp->bridge_name,
                        NSM_BRIDGE_NAMSIZ + 1);

           nsm_bridge_api_port_add (master, name, ifp,
                                    PAL_FALSE, PAL_TRUE, PAL_FALSE,
                                    PAL_FALSE);

#ifdef HAVE_VLAN
           nsm_vlan_agg_mode_vlan_sync (master, ifp, PAL_FALSE);
#endif /* HAVE_VLAN */

         }
       else if (master && zif->nsm_bridge_port_conf &&
                zif->nsm_bridge_port_conf->bridge)
         {
            nsm_bridge_api_port_add (master,
                                     zif->nsm_bridge_port_conf->bridge->name,
                                     ifp, PAL_TRUE, PAL_TRUE, PAL_FALSE,
                                     PAL_FALSE);
#ifdef HAVE_VLAN
           nsm_vlan_agg_mode_vlan_sync (master, ifp, PAL_TRUE);
#endif /* HAVE_VLAN */
           XFREE (MTYPE_TMP, zif->nsm_bridge_port_conf);
           zif->nsm_bridge_port_conf = NULL;
         }
     }
#endif /* HAVE_LACPD && HAVE_VLAN */

#if defined HAVE_ONMD && defined HAVE_LACPD

  if (zif == NULL)
    return;

  if (zif && zif->agg.type == NSM_IF_AGGREGATOR)
    nsm_lacp_update_lldp_agg_port_id (ifp);

#endif /* HAVE_ONMD && HAVE_LACPD */

  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    {
      SET_FLAG (ifp->status, NSM_INTERFACE_ACTIVE);
#ifdef HAVE_L3
      if (if_is_up (ifp))
        nsm_if_addr_wakeup (ifp);
#endif /* HAVE_L3 */

      if (IS_NSM_DEBUG_KERNEL)
        zlog_info (NSM_ZG, "interface %s index %d becomes active.",
                   ifp->name, ifp->ifindex);
    }
  else
    {
      if (IS_NSM_DEBUG_KERNEL)
        zlog_info (NSM_ZG, "interface %s index %d is added.",
                   ifp->name, ifp->ifindex);
    }

#ifdef HAVE_VLAN

  if (! pal_mem_cmp (ifp->name, "vlan", 4))
    {
      pal_sscanf (ifp->name, "vlan%d.%d", &br, &vid);
      ifp->hw_type = IF_TYPE_VLAN;
      if (zif)
        zif->vid = (u_int16_t)vid;
    }
  else if (! pal_mem_cmp (ifp->name, "svlan", 5))
    {
      pal_sscanf (ifp->name, "svlan%d.%d", &br, &vid);
      ifp->hw_type = IF_TYPE_VLAN;
      if (zif)
        zif->vid = (u_int16_t)vid;
    }
#endif /* HAVE_VLAN */

#ifdef HAVE_LACPD
  /* For aggregator interfaces, set the aggregator type */
  if ((! pal_mem_cmp (ifp->name, "po", 2))
      || (! pal_mem_cmp (ifp->name, "sa", 2)))
    ifp->hw_type = IF_TYPE_AGGREGATE;
#endif /* HAVE_LACPD */
#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */


#ifdef HAVE_NSM_IF_PARAMS
  /* Process pending IP addresses. */
   nsm_if_params_pending_proc (ifp->vr->proto, ifp->name);
#endif /* HAVE_NSM_IF_PARAMS */

#ifdef HAVE_SNMP
   nsm_phy_entity_create (ifp->vr->proto, CLASS_PORT, ifp);
#endif /* HAVE_SNMP */
}

/* Handle an interface delete event */
void
nsm_if_delete_update (struct interface *ifp)
{
  struct nsm_vrf *nv;
  struct nsm_if *zif = NULL;
#ifdef HAVE_L3
  struct connected *ifc, *next;
  struct prefix *p;
#endif /* HAVE_L3 */
#ifdef HAVE_LACPD
  struct interface *ifp_agg = NULL;
  struct nsm_if *zif_agg = NULL;
#endif /* HAVE_LACPD */
#if defined HAVE_VLAN
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  int vid;
  int brno;
#endif /* defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP */
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  u_int32_t su_id;
  u_int32_t svid;
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
#endif /* HAVE_VLAN */
#ifdef HAVE_MLD_SNOOP
  s_int32_t  ret = 0;
#endif /* HAVE_MLD_SNOOP */

  /* Mark interface as inactive */
  UNSET_FLAG (ifp->status, NSM_INTERFACE_ACTIVE);
  SET_FLAG (ifp->status, NSM_INTERFACE_DELETE);

  zif = (struct nsm_if *)ifp->info;

  if (IS_NSM_DEBUG_KERNEL)
    zlog_info (NSM_ZG, "interface %s index %d is now inactive.",
               ifp->name, ifp->ifindex);

  nv = ifp->vrf->proto;

#ifdef HAVE_L3
  /* Delete connected routes from the kernel. */
  for (ifc = ifp->ifc_ipv4; ifc; ifc = next)
    {
      next = ifc->next;
      if ((p = ifc->address))
        {
          nsm_delete_static_arp (ifp->vr->proto, ifc, ifp);
          nsm_connected_down_ipv4 (ifp, ifc);
          nsm_server_if_address_delete_update (ifp, ifc);

          NSM_IF_DELETE_IFC_IPV4 (ifp, ifc);
          ifc_free (ifp->vr->zg, ifc);
        }
    }
  ifp->ifc_ipv4 = NULL;

#ifdef HAVE_IPV6
  for (ifc = ifp->ifc_ipv6; ifc; ifc = next)
    {
      next = ifc->next;
      if ((p = ifc->address))
        {
          NSM_IF_DELETE_IFC_IPV6 (ifp, ifc);
          nsm_connected_invalid_ipv6 (ifp, ifc);
          ifc_free (ifp->vr->zg, ifc);
        }
    }
  ifp->ifc_ipv6 = NULL;
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    {
      nsm_mpls_rib_if_down_process (ifp);

#ifdef HAVE_TE
      nsm_qos_deinit_interface (ifp, NSM_TRUE, NSM_FALSE, NSM_TRUE);
#endif /* HAVE_TE */

      nsm_mpls_if_del (ifp, NSM_FALSE);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VRRP
  /* Check VRRP on interface delete. */
  if (NSM_CAP_HAVE_VRRP)
    vrrp_if_delete (ifp);
#endif /* HAVE_VRRP */

#if defined HAVE_VLAN
  if (pal_mem_cmp (ifp->name, "vlan", 4) == 0
      || pal_mem_cmp (ifp->name, "svlan", 5) == 0)
    {
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
      struct nsm_bridge_master *master =
                  nsm_bridge_get_master (ifp->vr->proto);
#endif /* defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
#ifdef HAVE_PROVIDER_BRIDGE
      if (ifp->name [0] == 's')
        {
          if (pal_sscanf (ifp->name, "svlan%d.%u.%u", &brno, &vid, &svid) == 2)
            svid = vid;
        }
      else
        {
          if (pal_sscanf (ifp->name, "vlan%d.%u.%u", &brno, &vid, &svid) == 2)
            svid = vid;
        }
#else
      pal_sscanf (ifp->name, "vlan%d.%u", &brno, &vid);
      svid = vid;
#endif /* HAVE_PROVIDER_BRIDGE */
      su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, svid);
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
#ifdef HAVE_IGMP_SNOOP
     (void) nsm_l2_mcast_igmp_if_delete (master->l2mcast, ifp,
                                   su_id);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      ret = nsm_l2_mcast_mld_if_delete (master->l2mcast, ifp,
                                  su_id);
      if (ret < 0)
        zlog_warn (NSM_ZG, "MLD Interface deletion for %s vid %d not successful",
                             ifp->name, vid);

#endif /* HAVE_MLD_SNOOP */
    }
#endif

#ifdef HAVE_MCAST_IPV4
  nsm_mcast_vif_delete (nv->mcast, ifp);
  nsm_mcast_igmp_if_delete (nv->mcast, ifp);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  nsm_mcast6_mif_delete (nv->mcast6, ifp);
  nsm_mcast6_mld_if_delete (nv->mcast6, ifp);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_SNMP
   nsm_phy_entity_delete (ifp->vr->proto, ifp);
#endif /* HAVE_SNMP */

#ifdef HAVE_LACPD
  {
    if (zif && zif->agg.type == NSM_IF_AGGREGATED
        && zif->agg.info.parent)
      ifp_agg = zif->agg.info.parent;

    if (ifp_agg)
      zif_agg = ifp_agg->info;

    if (zif_agg && zif_agg->agg.type == NSM_IF_AGGREGATOR)
      {
        nsm_lacp_api_delete_aggregator_member (nv->nm, ifp, PAL_FALSE);
        nsm_lacp_process_delete_aggregator_member (nv->nm, ifp_agg->name,
                                                   ifp->ifindex);
      }
    nsm_lacp_if_admin_key_unset (ifp);
  }
#endif /* HAVE_LACPD */

#ifdef HAVE_L2
  {
    if (zif && zif->switchport && zif->switchport->bridge)
      nsm_bridge_if_delete (zif->switchport->bridge, ifp, PAL_TRUE);
  }
#endif /* HAVE_L2 */

#ifdef HAVE_L3
#ifdef HAVE_IPV6
  nsm_api_ipv6_nbr_del_by_ifp (ifp->vr->proto, ifp);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

  /* Send delete to clients. */
  nsm_server_if_delete (ifp);
}

void
nsm_if_delete_update_done (struct interface *ifp)
{
  struct nsm_vrf *nv;

  nv = ifp->vrf->proto;

#ifdef HAVE_HAL

#if defined HAVE_L3 && defined HAVE_INTERVLAN_ROUTING
  if (if_is_vlan (ifp))
    hal_if_svi_delete (ifp->name, ifp->ifindex);
#endif /* HAVE_L3 && HAVE_INTERVLAN_ROUTING && HAVE_HAL */

  hal_if_delete_done(ifp->name, ifp->ifindex);

#endif /* HAVE_HAL */

  /* XXX-VR Library should notify to PM. */
  nsm_if_bind_clean (ifp);

  /* Delete NSM interface.  */
  nsm_if_delete (ifp, nv);
}

void
nsm_if_ifindex_update (struct interface *ifp, int ifindex)
{
  cindex_t cindex = 0;
  int ret;

  /* Update the interface index.  */
  ret = if_ifindex_update (&NSM_ZG->ifg, ifp, ifindex);

  /* Notify it to the PMs.  */
  if (ret)
    nsm_server_if_update (ifp, cindex);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */
}

/* Check if L3 commands are allowed on the interface */
bool_t
nsm_check_l3_commands_allowed (struct cli *cli, struct interface *ifp,
                               const char *err_str)
{
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  if (NSM_INTF_TYPE_L2 (ifp))
    {
      cli_out (cli, "%% %s commands not allowed on L2 interface\n", err_str);
      return PAL_FALSE;
    }
  return PAL_TRUE;
}

bool_t
nsm_check_l3_gmpls_commands_allowed (struct cli *cli, struct interface *ifp,
                                     const char *err_str)
{
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  if (NSM_INTF_TYPE_L2 (ifp)
#ifdef HAVE_GMPLS
      /* For GMPLS case allow setting L3 commands if data  interface */
       &&  (!(ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA))
#endif /* HAVE_GMPLS */
     )
    {
      cli_out (cli, "%% %s commands not allowed on L2 interface\n", err_str);
      return PAL_FALSE;
    }
  return PAL_TRUE;
}

/* Check if L3 commands are allowed on the interface */
bool_t
nsm_check_l2_commands_allowed (struct cli *cli, struct interface *ifp)
{
#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
#ifdef HAVE_VLAN
  /* No L2 commands on VLAN interfaces */
  if (if_is_vlan (ifp))
    {
      cli_out (cli, "%% L2 commands not allowed on vlan interfaces\n");
      return PAL_FALSE;
    }
#endif /* HAVE_VLAN */
#if defined HAVE_TUNNEL || defined HAVE_MCAST_IPV4
  /* No L2 commands on tunnel interfaces */
  if (if_is_tunnel (ifp))
    {
      cli_out (cli, "%% L2 commands not allowed on tunnel interfaces\n");
      return PAL_FALSE;
    }
#endif /* HAVE_TUNNEL || HAVE_MCAST_IPV4 */
#ifdef HAVE_LACPD
  if (ifp->info &&
      NSM_INTF_TYPE_AGGREGATOR (ifp))
    {
      cli_out (cli, "%% L2/L3 mode cannot be explicitly configured on aggregator interfaces\n");
      return PAL_FALSE;
    }
#endif /* HAVE_LACPD */

  return PAL_TRUE;
}

/* Handle interface type change. Called when interface becomes L2. */
int
nsm_interface_set_mode_l2 (struct interface *ifp)
{
  struct nsm_if *zif;

  zif = (struct nsm_if *)ifp->info;
  if (! zif)
    return -1;

  /* Set type. */
  zif->type = NSM_IF_TYPE_L2;
  ifp->type = IF_TYPE_L2;

  NSM_INTERFACE_SET_L2_MEMBERSHIP (ifp);

  /* Bind message to PMs.  */
  nsm_server_if_bind_all (ifp);

#ifdef HAVE_LACPD
  /* We just send bind message for
     interface so always send update */
  nsm_lacp_if_admin_key_set (ifp);
  nsm_server_if_lacp_admin_key_update (ifp);
#endif /* HAVE_LACPD */

#ifdef HAVE_HA
  nsm_cal_modify_nsm_if (zif);
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return 0;
}

int
nsm_interface_unset_mode_l2 (struct interface *ifp)
{
  struct nsm_if *zif;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return -1;

#ifdef HAVE_LACPD
  {
    struct nsm_master *nm = ifp->vr->proto;
    struct interface *agg_ifp;
    struct nsm_if *zif_agg;

    if (zif->agg.type == NSM_IF_AGGREGATED)
      {
        agg_ifp = zif->agg.info.parent;
        zif_agg = agg_ifp->info;
        if ((zif_agg) && (LISTCOUNT (zif_agg->agg.info.memberlist) == 1))
          {
            nsm_lacp_api_delete_aggregator_member (nm, ifp, PAL_FALSE);
            nsm_lacp_cleanup_aggregator (nm, agg_ifp);
            nsm_lacp_process_delete_aggregator (nm, agg_ifp);
          }
        else
         {
           nsm_lacp_api_delete_aggregator_member (nm, ifp, PAL_FALSE);
           nsm_lacp_process_delete_aggregator_member (nm, agg_ifp->name,
                                                      ifp->ifindex);
         }
      }
  }
#endif /* HAVE_LACPD */

  /* Unbind message to PMs.  */
  nsm_server_if_unbind_all (ifp);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_if (zif);
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return 0;
}

#ifdef HAVE_L3
/* Handle interface type change. Called when interface becomes L3. */
int
nsm_interface_set_mode_l3 (struct interface *ifp)
{
  struct nsm_if *zif;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return -1;

  /* Set type. */
  zif->type = NSM_IF_TYPE_L3;
  ifp->type = IF_TYPE_L3;

  NSM_INTERFACE_SET_L3_MEMBERSHIP (ifp);

  /* Bind message to PMs.  */
  nsm_server_if_bind_all (ifp);

#ifdef HAVE_VRRP
  /* Initialize VRRP.  */
  IF_NSM_CAP_HAVE_VRRP
    vrrp_if_init (ifp);
#endif /* HAVE_VRRP */

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;

    nsm_lacp_if_admin_key_set (ifp);
    if (ifp->lacp_admin_key != key)
      nsm_server_if_lacp_admin_key_update (ifp);
  }
#endif /* HAVE_LACPD */

#ifdef HAVE_HA
  nsm_cal_modify_nsm_if (zif);
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return 0;
}

/* Handle interface type change. Called when interface becomes L2. */
int
nsm_interface_unset_mode_l3 (struct interface *ifp)
{
  struct nsm_vrf *nv;
  struct apn_vrf *vrf;

  if (IS_NSM_DEBUG_KERNEL)
    zlog_info (NSM_ZG, "interface %s index %d is now inactive.",
               ifp->name, ifp->ifindex);

  /* Delete connected routes from the kernel. */
  nsm_ip_address_uninstall_all (ifp->vr->id, ifp->name);

#ifdef HAVE_IPV6
  nsm_ipv6_address_uninstall_all (ifp->vr->id, ifp->name);
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      nsm_mpls_rib_if_down_process (ifp);

#ifdef HAVE_TE
      nsm_qos_deinit_interface (ifp, NSM_TRUE, NSM_FALSE, NSM_TRUE);
#endif /* HAVE_TE */
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VRRP
  /* Check VRRP on interface delete. */
  if (NSM_CAP_HAVE_VRRP)
    vrrp_if_delete (ifp);
#endif /* HAVE_VRRP */

  nv = ifp->vrf->proto;
  vrf = nv->vrf;

  /* Delete static  routes. */
  nsm_static_delete_by_if (vrf->proto, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_static_delete_by_if (vrf->proto, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */

  /* Unbind the interface from IGMP/MLD instances */
#ifdef HAVE_MCAST_IPV4
  nsm_mcast_vif_delete (nv->mcast, ifp);
  nsm_mcast_igmp_if_delete (nv->mcast, ifp);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  nsm_mcast6_mif_delete (nv->mcast6, ifp);
  nsm_mcast6_mld_if_delete (nv->mcast6, ifp);
#endif /* HAVE_MCAST_IPV6 */

  /* Delete static multicast routes. */
  nsm_static_mroute_delete_by_if (vrf->proto, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_static_mroute_delete_by_if (vrf->proto, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */

#if defined (HAVE_VRF) && defined (HAVE_MPLS)
  /* Unset binding in MPLS Forwarder. */
  if (NSM_CAP_HAVE_VRF && NSM_CAP_HAVE_MPLS)
    if (vrf->id != 0)
      nsm_mpls_if_vrf_unbind_by_ifp (ifp);
#endif /* HAVE_VRF && HAVE_MPLS */

#ifdef HAVE_LACPD
  {
    struct nsm_if *zif = ifp->info;
    struct nsm_if *zif_agg;
    struct nsm_master *nm = ifp->vr->proto;
    struct interface *agg_ifp;

    if (zif->agg.type == NSM_IF_AGGREGATED)
      {
        agg_ifp = zif->agg.info.parent;
        zif_agg = agg_ifp->info;
        if ((zif_agg) && (LISTCOUNT (zif_agg->agg.info.memberlist) == 1))
          {
            nsm_lacp_cleanup_aggregator (nm, agg_ifp);
            nsm_lacp_process_delete_aggregator (nm, agg_ifp);
          }
        else
         {
           nsm_lacp_api_delete_aggregator_member (nm, ifp, PAL_FALSE);
           nsm_lacp_process_delete_aggregator_member (nm, agg_ifp->name,
                                                      ifp->ifindex);
         }
      }
  }
#endif /* HAVE_LACPD */

  /* Unbind message to PMs.  */
  nsm_server_if_unbind_all (ifp);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return 0;
}

/* Handle L3 specific interface UP handling. */
void
_nsm_l3_if_up (struct interface *ifp)
{
  struct connected *ifc;
  struct prefix *p;
#ifdef HAVE_MPLS
  struct nsm_mpls_if *mif = NULL;
#endif /* HAVE_MPLS */
  cindex_t cindex = 0;
  struct apn_vrf *iv = ifp->vrf;
  struct nsm_vrf *nv = iv->proto;
#ifdef HAVE_VPLS
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
#ifdef HAVE_TE
      nsm_qos_init_interface (ifp);
#endif /* HAVE_TE */

      /* Lookup mpls interface  */
      mif = nsm_mpls_if_lookup (ifp);
      if (mif)
        {
          /* Copy over label_space data to ifp. */
          nsm_mpls_ifp_set (mif, ifp);
          NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);
        }
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VRRP
  IF_NSM_CAP_HAVE_VRRP
    {
      /* Update VRRP for if_up. */
      vrrp_if_up (ifp);
    }
#endif /* HAVE_VRRP */

#ifdef HAVE_RTADV
  if (NSM_CAP_HAVE_IPV6)
    nsm_rtadv_if_up (ifp);
#endif /* HAVE_RTADV */

  /* Notify the protocol daemons. */
  nsm_server_if_up_update (ifp, cindex);

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      if (mif)
        nsm_mpls_if_up (mif);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_MCAST_IPV4
  /* Update the IGMP interface */
  nsm_mcast_igmp_if_update (nv->mcast, ifp);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  /* Update the MLD interface */
  nsm_mcast6_mld_if_update (nv->mcast6, ifp);
#endif /* HAVE_MCAST_IPV6 */

  /* TBD old code need remove
#ifdef HAVE_GMPLS
  if (ifp->gmpls_if)
    nsm_gmpls_if_update (ifp);
#endif  HAVE_GMPLS */

#ifdef HAVE_VPLS
  if ((mif = nsm_mpls_if_lookup (ifp)))
    LIST_LOOP (mif->vpls_info_list, vc_info, ln)
      if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
          CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
          vc_info->u.vpls)
        nsm_vpls_interface_up (vc_info->u.vpls, ifp);
#endif /* HAVE_VPLS */

  /* Install connected routes to the kernel. */
  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    if ((p = ifc->address))
      {
#ifdef HAVE_CRX
        /* Skip Virtual addresses. */
        if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
          continue;
#endif /* HAVE_CRX. */

        nsm_connected_up_ipv4 (ifp, ifc);
        /* Updating the static ip routes related to this interface
         * in FIB database. Handle case where interface is down and
         * up before NSM_RIB_UPDATE_DELAY timer expires. Interface
         * state can be changed either from CLI or kernel.
         */
        nsm_rib_ipaddr_chng_ipv4_fib_update (
                                  ((struct nsm_vrf *)(ifp->vrf->proto))->nm,
                                  ifp->vrf->proto, ifc);
      }

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
      if ((p = ifc->address))
        {
#ifdef HAVE_CRX
        /* Skip Virtual addresses. */
          if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
            continue;
#endif /* HAVE_CRX. */

          nsm_connected_up_ipv6 (ifp, ifc);
        }
#endif /* HAVE_IPV6 */

#ifdef HAVE_VLAN
  if (if_is_vlan (ifp))
    nsm_if_addr_wakeup (ifp);
#endif /* HAVE_VLAN */

  /* Update router-ID. */
  ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);

  /* Examine all static routes. */
  nsm_rib_update_timer_add (nv, AFI_IP);
#ifdef HAVE_IPV6
  nsm_rib_update_timer_add (nv, AFI_IP6);
#endif /* HAVE_IPV6 */

  /* Update the multicast routes */
  nsm_mrib_handle_if_state_change (nv, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_mrib_handle_if_state_change (nv, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */

#ifdef HAVE_GMPLS
  nsm_gmpls_control_channel_interface_update (ifp);
  nsm_gmpls_data_link_interface_update (ifp);
#endif /* HAVE_GMPLS */
}
#endif /* HAVE_L3 */

#ifdef HAVE_L2
/* Handle L2 specific interface UP handling. */
void
_nsm_l2_if_up (struct interface *ifp)
{
  cindex_t cindex = 0;
#ifdef HAVE_MPLS
  struct nsm_mpls_if *mif = NULL;
#ifdef HAVE_VPLS
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      /* Lookup mpls interface  */
      mif = nsm_mpls_if_lookup (ifp);
      if (mif)
        {
          /* Copy over label_space data to ifp. */
          nsm_mpls_ifp_set (mif, ifp);

          NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);
        }
    }
#endif /* HAVE_MPLS */

  if (ifp->bandwidth > 0)
    NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);

  /* Notify to the protocol daemons. */
  nsm_server_if_up_update (ifp, cindex);

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      if (mif)
        nsm_mpls_if_up (mif);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VPLS
  if ((mif = nsm_mpls_if_lookup (ifp)))
    LIST_LOOP (mif->vpls_info_list, vc_info, ln)
      if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
          CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
          vc_info->u.vpls)
        nsm_vpls_interface_up (vc_info->u.vpls, ifp);
#endif /* HAVE_VPLS */

#ifdef HAVE_GMPLS
  nsm_gmpls_data_link_interface_update (ifp);
#endif /* HAVE_GMPLS */
}
#endif /* HAVE_L2 */

/* Interface is up. */
void
nsm_if_up (struct interface *ifp)
{
  struct nsm_if *zif;
#ifdef HAVE_L3
  struct nsm_master *nm = ifp->vr->proto;
#endif /* HAVE_L3 */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  struct nsm_bridge_master *master;
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_LACPD
  int one_member_up = PAL_FALSE;
  struct nsm_if *parent_if = NULL;
#endif /* end of LACPD */

#if defined HAVE_LACPD || defined HAVE_PBR
  struct apn_vr *vr = ifp->vr;

  if (vr == NULL)
    vr = apn_vr_get_privileged (nzg);
#endif /* HAVE_LACPD || HAVE_PBR */

  zif = (struct nsm_if *) ifp->info;
  if (! zif)
    return;

#ifdef HAVE_L3
  if (zif->type == NSM_IF_TYPE_L3)
    _nsm_l3_if_up (ifp);
#endif /* HAVE_L3 */

#ifdef HAVE_L2
  if (zif->type == NSM_IF_TYPE_L2)
    _nsm_l2_if_up (ifp);
#endif /* HAVE_L2 */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  master = nsm_bridge_get_master (ifp->vr->proto);
  if (if_is_vlan (ifp) && master)
    {
#ifdef HAVE_IGMP_SNOOP
      nsm_l2_mcast_igmp_if_update (master->l2mcast, ifp, zif->vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      nsm_l2_mcast_mld_if_update (master->l2mcast, ifp, zif->vid);
#endif /* HAVE_MLD_SNOOP */
    }
#endif  /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP*/

#ifdef HAVE_LACPD
  /* Update aggegrated member state */
  nsm_update_aggregator_member_state (zif);

  NSM_AGG_IS_ONE_MEMBER_UP (zif);
  /* Only for static agg it is made up. For LACP, it is by protocol operation*/
  if (one_member_up && (zif->agg_config_type == AGG_CONFIG_STATIC)
      && parent_if )
    nsm_if_flag_up_set (vr->id, parent_if->ifp->name, PAL_FALSE);
#endif /* end of LACPD */

#ifdef HAVE_SNMP
  if (ifp->if_linktrap == NSM_IF_TRAP_ENABLE)
    nsm_snmp_trap (ifp->ifindex, NSM_SNMP_INTERFACE_UP,
                   NSM_SNMP_INTERFACE_UP);
#endif /* HAVE_SNMP */

#ifdef HAVE_L3
  nsm_if_arp_set (nm, ifp);
#ifdef HAVE_PBR
  nsm_pbr_if_up (vr, ifp->name);
#endif /* HAVE_PBR */
#endif /* HAVE_L3 */
  return;
}

#ifdef HAVE_L3
/* Handle L3 specific interface DOWN event. */
void
_nsm_l3_if_down (struct interface *ifp)
{
  struct connected *ifc;
  struct prefix *p;
  struct apn_vrf *iv = ifp->vrf;
  struct nsm_vrf *nv = iv->proto;
#ifdef HAVE_VPLS
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      nsm_mpls_if_down (ifp);
      nsm_mpls_rib_if_down_process (ifp);

#ifdef HAVE_TE
      nsm_qos_deinit_interface (ifp, NSM_FALSE, NSM_FALSE, NSM_TRUE);
#endif /* HAVE_TE */
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_MCAST_IPV4
  /* Update the IGMP interface */
  nsm_mcast_igmp_if_update (nv->mcast, ifp);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  /* Update the MLD interface */
  nsm_mcast6_mld_if_update (nv->mcast6, ifp);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_RTADV
  if (NSM_CAP_HAVE_IPV6)
    nsm_rtadv_if_down (ifp);
#endif /* HAVE_RTADV */

#ifdef HAVE_VRRP
  IF_NSM_CAP_HAVE_VRRP
    {
      vrrp_if_down (ifp);
    }
#endif /* HAVE_VRRP */

  /* Update the multicast routes */
  nsm_mrib_handle_if_state_change (nv, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_mrib_handle_if_state_change (nv, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */

  /* Notify to the protocol daemons. */
  nsm_server_if_down_update (ifp, 0);

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      /* Unset label_space data from ifp. */
      nsm_mpls_ifp_unset (ifp);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VPLS
  if ((mif = nsm_mpls_if_lookup (ifp)))
    LIST_LOOP (mif->vpls_info_list, vc_info, ln)
      if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
          CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
          vc_info->u.vpls)
        nsm_vpls_interface_down (vc_info->u.vpls, ifp);
#endif /* HAVE_VPLS */

  /* Delete connected routes from the kernel. */
  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    if ((p = ifc->address))
      {
#ifdef HAVE_CRX
        /* Skip Virtual addresses. */
        if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
          continue;
#endif /* HAVE_CRX. */

        nsm_connected_down_ipv4 (ifp, ifc);
      }

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
      if ((p = ifc->address))
        {
#ifdef HAVE_CRX
        /* Skip Virtual addresses. */
          if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
            continue;
#endif /* HAVE_CRX. */

          nsm_connected_down_ipv6 (ifp, ifc);
        }
#endif /* HAVE_IPV6 */

  /* Update router-ID. */
  ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);

  /* Examine all static routes which direct to the interface. */
  nsm_rib_update_timer_add (nv, AFI_IP);
#ifdef HAVE_IPV6
  nsm_rib_update_timer_add (nv, AFI_IP6);
#endif /* HAVE_IPV6 */

#ifdef HAVE_GMPLS
  nsm_gmpls_control_channel_interface_update (ifp);
  nsm_gmpls_data_link_interface_update (ifp);
#endif /* HAVE_GMPLS */
}
#endif /* HAVE_L3 */

#ifdef HAVE_L2
/* Handle L2 specific interface DOWN event. */
void
_nsm_l2_if_down (struct interface *ifp)
{
#ifdef HAVE_VPLS
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;
#endif /* HAVE_VPLS */
  cindex_t cindex = 0;

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      nsm_mpls_if_down (ifp);
    }
#endif /* HAVE_MPLS */

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);

  /* Notify to the protocol daemons. */
  nsm_server_if_down_update (ifp, cindex);

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      /* Unset label_space data from ifp. */
      nsm_mpls_ifp_unset (ifp);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VPLS
  if ((mif = nsm_mpls_if_lookup (ifp)))
    LIST_LOOP (mif->vpls_info_list, vc_info, ln)
      if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
          CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
          vc_info->u.vpls)
        nsm_vpls_interface_down (vc_info->u.vpls, ifp);
#endif /* HAVE_VPLS */

#ifdef HAVE_GMPLS
  nsm_gmpls_data_link_interface_update (ifp);
#endif /* HAVE_GMPLS */
}
#endif /* HAVE_L2 */

/* Interface goes down.  We have to manage different behavior of based
   OS. */
void
nsm_if_down (struct interface *ifp)
{
  struct nsm_if *zif;
#ifdef HAVE_L3
  struct nsm_master *nm = ifp->vr->proto;
#endif

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  struct nsm_bridge_master *master;
#endif  /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
#ifdef HAVE_LACPD
  int one_member_up = PAL_FALSE;
  struct nsm_if *parent_if = NULL;
#endif /* end of LACPD */

  zif = (struct nsm_if *) ifp->info;
  if (! zif)
    return;

#ifdef HAVE_L3
  if (zif->type == NSM_IF_TYPE_L3)
    _nsm_l3_if_down (ifp);
#endif /* HAVE_L3 */

#ifdef HAVE_L2
  if (zif->type == NSM_IF_TYPE_L2)
    _nsm_l2_if_down (ifp);
#endif /* HAVE_L2 */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  master = nsm_bridge_get_master (ifp->vr->proto);
  if (if_is_vlan (ifp) && master)
    {
#ifdef HAVE_IGMP_SNOOP
      nsm_l2_mcast_igmp_if_update (master->l2mcast, ifp, zif->vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      nsm_l2_mcast_mld_if_update (master->l2mcast, ifp, zif->vid);
#endif /* HAVE_MLD_SNOOP */
    }
#endif  /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_LACPD
  /* Update aggegrated member state */
  nsm_update_aggregator_member_state (zif);

  NSM_AGG_IS_ONE_MEMBER_UP (zif);
  if (!one_member_up && parent_if)
     nsm_if_flag_up_unset (ifp->vr->id, parent_if->ifp->name, PAL_FALSE);
#endif /* end of LACPD */

#ifdef HAVE_SNMP
  if (ifp->if_linktrap == NSM_IF_TRAP_DISABLE)
    nsm_snmp_trap (ifp->ifindex, NSM_SNMP_INTERFACE_DOWN,
                   NSM_SNMP_INTERFACE_DOWN);
#endif /* HAVE_SNMP */

#ifdef HAVE_L3
  nsm_if_arp_unset (nm, ifp);
#endif /* HAVE_L3 */

  return;
}

void
nsm_if_refresh (struct interface *ifp)
{
  int up;
  int running;

  /* Preserve old status. */
  up = if_is_up (ifp);
  running = if_is_running (ifp);

  /* Get current status. */
  nsm_fea_if_flags_get (ifp);

  /* Check if status has changed. */
  if (up && running)
    {
      if (!if_is_running (ifp))
        nsm_if_down (ifp);
    }
  else
    {
      if (if_is_running (ifp))
        nsm_if_up (ifp);
      else if (! running &&
              ((CHECK_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC)) ||
              (CHECK_FLAG (ifp->bind,NSM_IF_BIND_MPLS_VC_VLAN))))
        nsm_if_down (ifp);
    }
}

/* If statistics update threshold timer expiry */
int
nsm_if_stat_update_threshold_timer (struct thread *t)
{
   struct apn_vr *vr;

   vr = THREAD_ARG (t);
   if (vr == NULL)
     return -1;

   vr->t_if_stat_threshold = NULL;

  return 0;
}

/* Printout flag information into vty */
void
nsm_if_flag_dump_vty (struct cli *cli, u_int32_t flag)
{
  s_int32_t separator = 0;

#define IFF_OUT_VTY(X, Y) \
  if ((X) && (flag & (X))) \
    { \
      if (separator) \
        cli_out (cli, ","); \
      else \
        separator = 1; \
      cli_out (cli, Y); \
    }

  cli_out (cli, "  <");
  IFF_OUT_VTY (IFF_UP, "UP");
  IFF_OUT_VTY (IFF_BROADCAST, "BROADCAST");
  IFF_OUT_VTY (IFF_DEBUG, "DEBUG");
  IFF_OUT_VTY (IFF_LOOPBACK, "LOOPBACK");
  IFF_OUT_VTY (IFF_POINTOPOINT, "POINTOPOINT");
  IFF_OUT_VTY (IFF_NOTRAILERS, "NOTRAILERS");
  IFF_OUT_VTY (IFF_RUNNING, "RUNNING");
  IFF_OUT_VTY (IFF_NOARP, "NOARP");
  IFF_OUT_VTY (IFF_PROMISC, "PROMISC");
  IFF_OUT_VTY (IFF_ALLMULTI, "ALLMULTI");
  IFF_OUT_VTY (IFF_OACTIVE, "OACTIVE");
  IFF_OUT_VTY (IFF_SIMPLEX, "SIMPLEX");
  IFF_OUT_VTY (IFF_LINK0, "LINK0");
  IFF_OUT_VTY (IFF_LINK1, "LINK1");
  IFF_OUT_VTY (IFF_LINK2, "LINK2");
  IFF_OUT_VTY (IFF_MULTICAST, "MULTICAST");
  cli_out (cli, ">");
}

/* Output prefix string to vty. */
result_t
nsm_prefix_cli_out (struct cli *cli, struct prefix *p)
{
  char str[INET6_ADDRSTRLEN];

  pal_inet_ntop (p->family, &p->u.prefix, str, sizeof (str));
  cli_out (cli, "%s", str);
  return pal_strlen (str);
}

/* Dump if address information to vty. */
void
nsm_connected_dump_vty (struct cli *cli, struct connected *connected)
{
  struct prefix *p;
  struct interface *ifp;

  /* Set interface pointer. */
  ifp = connected->ifp;

  /* Print interface address. */
  p = connected->address;
  cli_out (cli, "  %s ", prefix_family_str (p));
  nsm_prefix_cli_out (cli, p);
  cli_out (cli, "/%d", p->prefixlen);

  /* If there is destination address, print it. */
  p = connected->destination;
  if (p)
    {
      if (p->family == AF_INET)
        if (ifp->flags & IFF_BROADCAST)
          {
            cli_out (cli, " broadcast ");
            nsm_prefix_cli_out (cli, p);
          }

      if (ifp->flags & IFF_POINTOPOINT)
        {
          cli_out (cli, " pointopoint ");
          nsm_prefix_cli_out (cli, p);
        }
    }

  if (CHECK_FLAG (connected->flags, NSM_IFA_SECONDARY))
    cli_out (cli, " secondary");

  if (CHECK_FLAG (connected->flags, NSM_IFA_ANYCAST))
    cli_out (cli, " anycast");

  if (CHECK_FLAG (connected->flags, NSM_IFA_VIRTUAL))
    cli_out (cli, " virtual");

  if (connected->label)
    cli_out (cli, " %s", connected->label);

  cli_out (cli, "\n");
}

/* Interface's information print out to vty interface. */
char *
nsm_if_type_str (u_int16_t type)
{
  switch (type)
    {
    case IF_TYPE_ETHERNET:
      return "Ethernet";
    case IF_TYPE_LOOPBACK:
      return "Loopback";
    case IF_TYPE_HDLC:
      return "HDLC";
    case IF_TYPE_PPP:
      return "PPP";
    case IF_TYPE_ATM:
      return "ATM";
    case IF_TYPE_VLAN:
      return "VLAN";
    case IF_TYPE_AGGREGATE:
      return "AGGREGATE";
    case IF_TYPE_FRELAY:
      return "Frame relay";
    case IF_TYPE_APNP:
    case IF_TYPE_GREIP:
    case IF_TYPE_IPV6IP:
      return "Tunnel";
    case IF_TYPE_PBB_LOGICAL:
      return "PBB logical port";
    default:
      return "Unknown";
    }
}

char *
nsm_if_media_type_str (u_int16_t mtype)
{
  switch (mtype)
    {
      case NSM_IF_HALF_DUPLEX:
        return "half";
      case NSM_IF_FULL_DUPLEX:
        return "full";
      case NSM_IF_AUTO_NEGO:
        return "auto";
      default:
        return "unknown";
    }
}

void
nsm_if_dump_vty (struct cli *cli, struct interface *ifp)
{

#ifdef HAVE_MPLS
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_mpls_if *mif;
#endif /* HAVE_MPLS */
  char buf[BUFSIZ];
  struct connected *ifc;
  struct nsm_if *nif = ifp->info;
  u_int8_t *mac_addr;
  u_int8_t id = 0;
  char* serv_str = NULL;

  /* Interface name. */
  cli_out (cli, "Interface %s\n", ifp->name);

  /* Hardware information. */
  cli_out (cli, "  Hardware is %s", nsm_if_type_str (ifp->hw_type));
  if (ifp->hw_type == IF_TYPE_ETHERNET || ifp->hw_type == IF_TYPE_VLAN
      ||ifp->hw_type == IF_TYPE_PBB_LOGICAL)
    {
      cli_out(cli, "  Current HW addr: ");
      if (ifp->hw_addr_len)
        cli_out(cli, "%s\n", nsm_ifma_to_str(ifp->hw_addr, buf, sizeof(buf)));
      else
        cli_out(cli, "(not set)\n");

      cli_out(cli, "  Physical:");
      mac_addr = nsm_ifma_get_physical(nif, NULL, 0);
      if (mac_addr)
        cli_out(cli, "%s", nsm_ifma_to_str(mac_addr, buf, sizeof(buf)));
      else
        cli_out(cli, "(not set)");

#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)
      cli_out(cli, "  Logical:");
      mac_addr = nsm_ifma_get_logical(nif, NULL, 0);
      if (mac_addr)
        cli_out(cli, "%s", nsm_ifma_to_str(mac_addr, buf, sizeof(buf)));
      else
        cli_out(cli, "(not set)");
#endif
      cli_out (cli, "\n");
    }

  /* Description. */
  if (ifp->desc)
    cli_out (cli, "  Description: %s\n", ifp->desc);

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Unnumbered interface.  */
  nsm_if_unnumbered_show (cli, ifp);
#endif /* HAVE_NSM_IF_UNNUMBERED */

  if (ifp->ifindex <= 0)
    {
      cli_out (cli, "  index %d pseudo interface\n", ifp->ifindex);
      return;
    }
  else if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "  index %d inactive interface\n", ifp->ifindex);
      return;
    }

  nsm_fea_if_get_duplex (ifp);

  cli_out (cli, "  index %d metric %d mtu %d duplex-%s"
           " arp ageing timeout %d\n", ifp->ifindex, ifp->metric, ifp->mtu,
           nsm_if_media_type_str(ifp->duplex), ifp->arp_ageing_timeout);

  nsm_if_flag_dump_vty (cli, ifp->flags);
  cli_out (cli, "\n");

  if (NSM_CAP_HAVE_VRF)
    {
      if (ifp->vrf && ifp->vrf->name)
        cli_out (cli, "  VRF Binding: Associated with %s\n",
                 ifp->vrf->name);
      else
        cli_out (cli, "  VRF Binding: Not bound\n");
    }

#ifdef HAVE_VRX
  if (CHECK_FLAG (ifp->vrx_flag, IF_VRX_FLAG_WRPJ))
    cli_out (cli, "  CheckPoint VSX Gateway EVR wrpj interface\n");
  else if (CHECK_FLAG (ifp->vrx_flag, IF_VRX_FLAG_WRP))
    cli_out (cli, "  CheckPoint VSX Gateway VS wrp interface\n");
  else
    ;
  if (ifp->local_flag == IF_VRX_FLAG_LOCALSRC)
    cli_out (cli, "  This is LocalSrc IP of this VS \n");
#endif /* HAVE_VRX */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      mif = nsm_mpls_if_lookup (ifp);

      if (mif && mif->nls)
        {
          /* Label space data. */
          cli_out (cli, "  Label switching is enabled");
          cli_out (cli, " with label-space %d\n", mif->nls->label_space);
          cli_out (cli, "   minimum label value configured is %d\n",
                   ((mif->nls->min_label_val != 0) ?
                    mif->nls->min_label_val :
                    nsm_mpls_min_label_val_get (nm)));
          cli_out (cli, "   maximum label value configured is %d\n",
                   ((mif->nls->max_label_val != 0) ?
                    mif->nls->max_label_val :
                    nsm_mpls_max_label_val_get (nm)));

          if ((CHECK_FLAG (mif->nls->config,
                                  NSM_LABEL_SPACE_CONFIG_MIN_LABEL_RANGE)) ||
              (CHECK_FLAG (mif->nls->config,
                                  NSM_LABEL_SPACE_CONFIG_MAX_LABEL_RANGE)))
            {
              for (id = LABEL_POOL_RANGE_INITIAL;
                   id < LABEL_POOL_RANGE_MAX; id++)
                {
                  /* Skip un-supported service at this time */
                  if ((id == 2) || (id == 3))
                    continue;

#if (defined HAVE_PACKET || defined HAVE_GMPLS)
                  if ((mif->nls->service_ranges[id].from_label !=
                                                   LABEL_UNINITIALIZED_VALUE) ||
                      (mif->nls->service_ranges[id].to_label !=
                                                   LABEL_UNINITIALIZED_VALUE))
                    {
                      serv_str = nsm_gmpls_get_lbl_range_service_name(id);
                      cli_out (cli,
                        "     minimum %s label range value configured is %d\n",
                        serv_str,
                        ((mif->nls->service_ranges[id].from_label != 0) ?
                              mif->nls->service_ranges[id].from_label : 0));
                      cli_out (cli,
                        "     maximum %s label range value configured is %d\n",
                        serv_str,
                        ((mif->nls->service_ranges[id].to_label != 0) ?
                               mif->nls->service_ranges[id].to_label : 0));
                    }
#endif /* HAVE_PACKET || HAVE_GMPLS*/
                }
            }
        }
      else
        {
          /* Label space data. */
          cli_out (cli, "  Label switching is disabled");
          cli_out (cli, "\n");
        }
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_VPLS
  {
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;
    /* Lookup mif. */
    mif = nsm_mpls_if_lookup (ifp);
    if (mif)
      {
        LIST_LOOP (mif->vpls_info_list, vc_info, ln)
        if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
             CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
            vc_info->u.vpls)
          cli_out (cli, "  Virtual Private LAN Service configured is %s\n",
                   vc_info->u.vpls->vpls_name);
      }
  }
#endif /* HAVE_VPLS */
#ifdef HAVE_MPLS_VC
    {
      struct nsm_mpls_if *mif;
      struct nsm_mpls_vc_group *group = NULL;
      struct nsm_mpls_vc_info *vc_info;
      struct listnode *ln;
      char *vc_name = NULL;
      int vc_config = 0;
      u_int16_t vc_type = VC_TYPE_UNKNOWN;

      /* Lookup mif. */
      mif = nsm_mpls_if_lookup (ifp);
      if (mif)
        {
          LIST_LOOP (mif->vc_info_list, vc_info, ln)
            {
              if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
                  (! CHECK_FLAG (vc_info->if_bind_type,
                                 NSM_IF_BIND_MPLS_VC_VLAN)))
                continue;

              if (vc_info->vc_name)
                {
                  vc_name = vc_info->vc_name;
                  vc_type = vc_info->vc_type;
                  if (vc_info->u.vc)
                    group = vc_info->u.vc->group;
                }

              if ((vc_name) && (vc_type != VC_TYPE_UNKNOWN))
                {
                  vc_config = 1;
                  cli_out (cli, "  Virtual Circuit configured is \'%s\'", vc_name);
                  if (group)
                    cli_out (cli, " belonging to group \'%s\'", group->name);
                  cli_out (cli, "\n");
                  cli_out (cli, "   type is \'%s\'\n",
                           mpls_vc_type_to_str (vc_type));
                }
            }
          if (vc_config == 0)
            cli_out (cli, "  No Virtual Circuit configured\n");
        }
    }
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_TE
  {
    int i;
    int heading_done = 0;
    int count = 0;
    int found_one = 0;
    char *name;

    for (i = 0; i < ADMIN_GROUP_MAX; i++)
      {
        if (heading_done == 0)
          {
            cli_out (cli, "  Administrative Group(s): ");
            heading_done = 1;
          }

        if (CHECK_FLAG (ifp->admin_group, (1 << i)))
          {
            name = admin_group_get_name (NSM_MPLS->admin_group_array, i);
            if (name == NULL)
              continue;

            cli_out (cli, "%s", name);

            if (found_one == 0)
              found_one = 1;

            /* Increment count */
            count++;

            /* If this was a multiple of four ... */
            if ((count % 4) == 0)
              cli_out (cli, "\n                           ");
            else
              cli_out (cli, " ");
          }
      }

    if (found_one == 0)
      cli_out (cli, "None\n");
    else
      cli_out (cli, "\n");
  }
#endif /* HAVE_TE */

  /* Bandwidth */
  if (ifp->bandwidth != 0)
    {
      char bw_buf[BW_BUFSIZ];
      cli_out (cli, "  Bandwidth %s",
               bandwidth_float_to_string (bw_buf, ifp->bandwidth));
      cli_out (cli, "\n");
    }


#ifdef HAVE_TE
  {
    char bw_buf[BW_BUFSIZ];

    if (ifp->max_resv_bw != 0)
      cli_out (cli, "  Maximum reservable bandwidth %s\n",
               bandwidth_float_to_string (bw_buf, ifp->max_resv_bw));

#ifdef HAVE_DSTE
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
    int i;
    cli_out (cli, "  DSTE Bandwidth Constraint Mode is %s\n",
             ifp->bc_mode == BC_MODE_MAM ? "MAM" :
             ifp->bc_mode == BC_MODE_RSDL ? "RSDL" :
             "MAR");

    for (i = 0; i < MAX_CLASS_TYPE; i++)
      if (NSM_MPLS->ct_name[i][0] != '\0')
        cli_out (cli, "  Bandwidth Constraint for Class Type %s is %s\n",
                 NSM_MPLS->ct_name[i],
                 bandwidth_float_to_string (bw_buf, ifp->bw_constraint[i]));

    for (i = 0; i < MAX_TE_CLASS; i++)
      if (NSM_MPLS->te_class[i].ct_num != CT_NUM_INVALID)
        cli_out (cli, "  Available b/w for TE-CLASS %d {%s, %d} is %s\n",
                 i,
                 NSM_MPLS->ct_name[NSM_MPLS->te_class[i].ct_num],
                 NSM_MPLS->te_class[i].priority,
                 bandwidth_float_to_string (bw_buf, ifp->tecl_priority_bw[i]));
#endif /* (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))*/
#else
    int i;
    if (ifp->max_resv_bw != 0)
      {
        for (i = 0; i < MAX_PRIORITIES; i++)
          cli_out (cli, "    Available b/w at priority %d is %s\n", i,
                   bandwidth_float_to_string (bw_buf,
                                              ifp->tecl_priority_bw[i]));
      }
#endif /* HAVE_DSTE */
  }
#endif /* HAVE_TE */

  if (!if_is_ipv4_unnumbered (ifp))
    for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
      if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
        nsm_connected_dump_vty (cli, ifc);

#ifdef HAVE_VRRP
  vrrp_if_master_dump (cli, ifp);
#endif /* HAVE_VRRP */

#ifdef HAVE_IPV6
  if (!if_is_ipv6_unnumbered (ifp))
    for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
      if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
        nsm_connected_dump_vty (cli, ifc);
#endif /* HAVE_IPV6 */

#ifdef HAVE_TUNNEL
  nsm_tunnel_if_show (cli, ifp);
#endif /* HAVE_TUNNEL */

#ifdef HAVE_GMPLS
  nsm_gmpls_if_show (cli, ifp);
#endif /* HAVE_GMPLS */

#ifdef HAVE_RTADV
  IF_NSM_CAP_HAVE_IPV6
    {
      nsm_rtadv_if_show (cli, ifp);
    }
#endif /* HAVE_RTADV */

#if !(defined HAVE_HAL) || defined HAVE_SWFWDR
  /* Print statictics if available */
  pal_if_stats_get_input_stats (&ifp->stats, buf, BUFSIZ);
  if (pal_strcmp(buf, ""))
    cli_out (cli, "%s\n", buf);

  pal_if_stats_get_input_errors (&ifp->stats, buf, BUFSIZ);
  if (pal_strcmp(buf, ""))
    cli_out (cli, "%s\n", buf);

  pal_if_stats_get_output (&ifp->stats, buf, BUFSIZ);
  if (pal_strcmp(buf, ""))
    cli_out (cli, "%s\n", buf);

  pal_if_stats_get_output_errors (&ifp->stats, buf, BUFSIZ);
  if (pal_strcmp(buf, ""))
    cli_out (cli, "%s\n", buf);

  pal_if_stats_get_collisions (&ifp->stats, buf, BUFSIZ);
  if (pal_strcmp(buf, ""))
    cli_out (cli, "%s\n", buf);
#else  /* HAVE_HAL */
  {
     struct hal_if_counters if_stats;
     hal_if_get_counters(ifp->ifindex, &if_stats);

     cli_out (cli, "    input packets %u%u, bytes %u%u, dropped %u%u, multicast packets %u%u\n",
              if_stats.good_pkts_rcv.l[1],if_stats.good_pkts_rcv.l[0],
              if_stats.good_octets_rcv.l[1],if_stats.good_octets_rcv.l[0],
              if_stats.drop_events.l[1],if_stats.drop_events.l[0],
              if_stats.mc_pkts_rcv.l[1],if_stats.mc_pkts_rcv.l[0]);
     cli_out (cli, "    output packets %u%u, bytes %u%u, multicast packets %u%u broadcast packets %u%u\n",
              if_stats.good_pkts_sent.l[1],if_stats.good_pkts_sent.l[0],
              if_stats.good_octets_sent.l[1],if_stats.good_octets_sent.l[0],
              if_stats.mc_pkts_sent.l[1],if_stats.mc_pkts_sent.l[0],
              if_stats.brdc_pkts_sent.l[1],if_stats.brdc_pkts_sent.l[0]);
  }
#endif /* HAVE_HAL */
}

/* Check supported address family. */
result_t
nsm_if_supported_family (s_int32_t family)
{
  if (family == AF_INET)
    return 1;
#ifdef HAVE_IPV6
  IF_NSM_CAP_HAVE_IPV6
    {
      if (family == AF_INET6)
        return 1;
    }
#endif /* HAVE_IPV6 */
  return 0;
}

#ifdef HAVE_TE
void
nsm_if_delete_interface_all (struct apn_vr *vr)
{
  struct interface *ifp;
  struct listnode *node;
  struct nsm_if *nif;

  LIST_LOOP (vr->ifm.if_list, ifp, node)
    if ((nif = ifp->info))
      if (nif->qos_if)
        nsm_qos_deinit_interface (ifp, NSM_TRUE, NSM_FALSE, NSM_TRUE);
}
#endif /* HAVE_TE */

#ifdef HAVE_SNMP
/********************************************************************
 * Function : nsm_if_alias_set                                      *
 *                                                                  *
 * Input Parameters:                                                *
 *  ifp    : interface on which alias has to be set                 *
 *  alias  : Alias name                                             *
 *                                                                  *
 * Output :                                                         *
 *   NSM_IF_ALIAS_NO_INTF : No such interface                       *
 *   NSM_IF_ALIAS_INVALID : Invalid argument alias                  *
 *   NSM_IF_ALIAS_OK      : successfully in setting the alias       *
 *                                                                  *
 * Description : This fucntion will set the alias name for the      *
 *               given interface.                                   *
 ********************************************************************/
  
NSM_IF_ALIAS_T
nsm_if_alias_set (struct interface *ifp, char * alias)
{
  
  /* SANITY Check */
  if (NULL == ifp)
    return -(NSM_IF_ALIAS_NO_INTF);

  /* SANITY Check */
  if (NULL == alias)
    return -(NSM_IF_ALIAS_INVALID);
  
  /* Validate the length */
  if (pal_strlen (alias) > INTERFACE_NAMSIZ)
    return -(NSM_IF_ALIAS_INVLAID_LENGTH);
 
  /* Reset the inteface alias */
  pal_mem_set (ifp->if_alias, 0x0, INTERFACE_NAMSIZ);

  /* update the interface alias */
  pal_strncpy (ifp->if_alias, alias, pal_strlen (alias));

 return NSM_IF_ALIAS_OK;
}

/********************************************************************
 * Function : nsm_if_alias_unset                                    *
 *                                                                  *
 * Input Parameters:                                                *
 *  ifp    : interface  on the alias has to be un-set               *
 *                                                                  *
 * Output :                                                         *
 *   NSM_IF_ALIAS_NO_INTF    : No such interface                    *
 *   NSM_IF_ALIAS_NOT_CONFIG : Alias has not configured previously  *
 *   NSM_IF_ALIAS_OK      : successfully in unseting the alias      *
 *                                                                  *
 * Description : This fucntion will unset the alias name for the    *
 *               given interface.                                   *
 ********************************************************************/

NSM_IF_ALIAS_T 
nsm_if_alias_unset (struct interface *ifp)
{

  /* SANITY Check */
  if (NULL == ifp)
     return -(NSM_IF_ALIAS_NO_INTF);
   
  /* Validate the length */
  if (pal_strlen (ifp->if_alias) == 0)
    return -(NSM_IF_ALIAS_NOT_CONFIG);

  /* Reset the alias */
  pal_mem_set (ifp->if_alias, 0x0, INTERFACE_NAMSIZ);

 return NSM_IF_ALIAS_OK;
}
#endif /* HAVE_SNMP */
 
CLI (nsm_interface,
     nsm_interface_cmd,
     "interface IFNAME",
     "Select an interface to configure",
     "Interface's name")
{
  struct apn_vr *vr = cli->vr;
  struct interface * ifp;

  if (cli->vr->id == 0)
    ifp = ifg_lookup_by_name (&nzg->ifg, argv[0]);
  else
    ifp = if_lookup_by_name (&vr->ifm, argv[0]);

  /* We only allow creating a tunnel interface.  */
  if (ifp == NULL
      || CHECK_FLAG (ifp->status, IF_HIDDEN_FLAG))
#ifdef HAVE_TUNNEL
    if ((ifp = nsm_tunnel_if_name_set (vr->id, argv[0])) == NULL)
#endif /* HAVE_TUNNEL */
      {
        cli_out (cli, "%% No such interface\n");
        return CLI_ERROR;
      }

  if (CHECK_FLAG (ifp->status, IF_NON_CONFIGURABLE_FLAG))
     {
       cli_out (cli, "%% Non Configurable Interface\n");
       return CLI_ERROR;
     }

  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}
#ifdef HAVE_SNMP
CLI (nsm_interface_alias,
     nsm_interface_alias_cmd,
     "alias WORD ",
     "Alias name for the interface",
     "Name ")
{
 struct interface * ifp = NULL;
 int ret;

 ifp = (struct interface *) cli->index;
 ret = 0;
   
 ret = nsm_if_alias_set (ifp, argv[0]);
 
 if (ret == -(NSM_IF_ALIAS_NO_INTF))
   {
       cli_out (cli, "%% Error no such interface \n");
       return CLI_ERROR;
    }
 
  if (ret == -(NSM_IF_ALIAS_INVALID))
    {
      cli_out (cli, " %% Invalid argument alias : %s \n", argv[0]);
      return CLI_ERROR;
    }
 
  if (ret == -(NSM_IF_ALIAS_INVLAID_LENGTH))
    {
      cli_out (cli, " %% Invalid alias length, Max allowed length is : %d \n",
                      INTERFACE_NAMSIZ);
      return CLI_ERROR;
    }
  
 return CLI_SUCCESS;
 
}
  /* reset the alias name */
CLI (no_nsm_interface_alias,
     no_nsm_interface_alias_cmd,
     "no alias",
     CLI_NO_STR,
     "Alias name for the interface")
{
 struct interface * ifp = NULL;
 int ret;
   
 /* Intialization */
 ret = 0;
 
 ifp = (struct interface *) cli->index;
  
 ret = nsm_if_alias_unset (ifp);
  
 if (ret == -(NSM_IF_ALIAS_NO_INTF))
   {
      cli_out (cli, " %% Error no such interface \n");
      return CLI_ERROR;
   }
 
 if (ret == -(NSM_IF_ALIAS_NOT_CONFIG))
   {
     cli_out (cli, "%% Warring : Alias not configured before \n");
     return CLI_ERROR;
   }

   
 return CLI_SUCCESS;
}
#endif /* HAVE_SNMP */

CLI (no_nsm_interface,
     no_nsm_interface_cmd,
     "no interface IFNAME",
     CLI_NO_STR,
     "Delete a pseudo interface's configuration",
     "Interface's name")
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp;

  ifp = if_lookup_by_name (&vr->ifm, argv[0]);
  if (ifp == NULL)
    {
      cli_out (cli, "%% Interface %s does not exist\n", argv[0]);
      return CLI_ERROR;
    }

  if ( CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    {
      cli_out (cli, "%% Removal of interface not allowed\n");
      return CLI_ERROR;
    }

  /* Delete NSM interface.  */
  nsm_if_delete (ifp, ifp->vrf->proto);

  return CLI_SUCCESS;
}

/* Show all or specified interface to vty. */
CLI (show_interface,
     show_interface_cmd,
     "show interface (IFNAME|)",
     CLI_SHOW_STR,
     "Interface status and configuration",
     "Interface name")
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp;
  struct listnode *node;

  /* Get interface statistics if available */
#if !(defined HAVE_HAL) || defined HAVE_SWFWDR
  NSM_IF_STAT_UPDATE (vr);
#endif  /* !HAVE_HAL || HAVE_SWFWDR */

  /* Specified interface print. */
  if (argc != 0)
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[0]);
      if (ifp == NULL
#ifdef HAVE_SUPPRESS_UNMAPPED_IF
          || ! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED)
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */
          )
        {
          cli_out (cli, "%% Can't find interface %s\n", argv[0]);
          return CLI_ERROR;
        }
      nsm_if_dump_vty (cli, ifp);
      return CLI_SUCCESS;
    }

  /* All interface print. */
  LIST_LOOP (vr->ifm.if_list, ifp, node)
   {
      if (ifp->ifindex < 0)
        continue;

#ifdef HAVE_SUPPRESS_UNMAPPED_IF
      if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */
        nsm_if_dump_vty (cli, ifp);
   }

  return CLI_SUCCESS;
}

void
nsm_show_ip_if_brief (struct cli *cli, struct interface *ifp)
{
#ifdef HAVE_L3
  struct connected *ifc;
#endif /* HAVE_L3 */

  /* Interface name. */
  cli_out (cli, "%-20s  ", ifp->name);

#ifdef HAVE_L3
  /* IP addresss, show only primary. */
  ifc = nsm_if_connected_primary_lookup (ifp);

  if (ifc != NULL)
    cli_out (cli, "%-15r ", &ifc->address->u.prefix4);
  else
    cli_out (cli, "unassigned      ");
#endif /* HAVE_L3 */

  /* Status. */
#ifdef HAVE_CUSTOM1
  if (CHECK_FLAG (ifp->flags, IFF_UP))
#else /* HAVE_CUSTOM1 */
  if (if_is_up (ifp))
#endif /* HAVE_CUSTOM1 */
    cli_out (cli, "up                    ");
  else
    cli_out (cli, "administratively down ");

  /* Protocol. */
  if (if_is_running (ifp))
    cli_out (cli, "up                   ");
  else
    cli_out (cli, "down                 ");

#ifdef HAVE_GMPLS
  char *str;

  str = nsm_gmpls_if_type_str (ifp->gmpls_type);
  cli_out (cli, "%s", str);
#endif /*HAVE_GMPLS */

#ifdef HAVE_VRX
  if (ifp->local_flag == IF_VRX_FLAG_LOCALSRC)
    cli_out (cli, "LocalSrc IP of VS[id=%d]", ifp->vrf_id);
#endif /* HAVE_VRX */

  cli_out (cli, "\n");
}

CLI (show_ip_interface_brief,
     show_ip_interface_brief_cmd,
     "show ip interface brief",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP interface status and configuration",
     "Brief summary of IP status and configuration")
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp;
  struct listnode *node;

  if (argc > 0)
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[0]);
      if (ifp == NULL
#ifdef HAVE_SUPPRESS_UNMAPPED_IF
          || ! CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED)
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */
          )
        {
          cli_out (cli, "%% There is no such interface\n");
          return CLI_ERROR;
        }
      cli_out (cli, "Interface             IP-Address      "
               "Status                Protocol");
#ifdef HAVE_GMPLS
      cli_out (cli, "             GMPLS Type");
#endif /* HAVE_GMPLS */
      cli_out (cli, "\n");
      nsm_show_ip_if_brief (cli, ifp);
    }
  else
    {
      cli_out (cli, "Interface             IP-Address      "
               "Status                Protocol");
#ifdef HAVE_GMPLS
      cli_out (cli, "             GMPLS Type");
#endif /* HAVE_GMPLS */
      cli_out (cli, "\n");
      LIST_LOOP (vr->ifm.if_list, ifp, node)
       {
#ifdef HAVE_SUPPRESS_UNMAPPED_IF
          if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */
            nsm_show_ip_if_brief (cli, ifp);
       }
    }

  return CLI_SUCCESS;
}

ALI (show_ip_interface_brief,
     show_ip_interface_if_brief_cmd,
     "show ip interface IFNAME brief",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP interface status and configuration",
     "Interface name",
     "Brief summary of IP status and configuration");

#ifdef HAVE_IPV6
void
nsm_show_ipv6_if_brief (struct cli *cli, struct interface *ifp)
{
  struct connected *ifc = NULL;

  /* Interface name. */
  cli_out (cli, "%-25s  ", ifp->name);

  cli_out (cli, "[");

  /* Status. */
  if (if_is_up (ifp))
    cli_out (cli, "up");
  else
    cli_out (cli, "administratively down");

  cli_out (cli, "/");

  /* Protocol. */
  if (if_is_running (ifp))
    cli_out (cli, "up");
  else
    cli_out (cli, "down");

  cli_out (cli, "]\n");

  if (ifp->ifc_ipv6 == NULL)
    cli_out (cli, "    unassigned\n");
  else
    for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
      if (!IN6_IS_ADDR_UNSPECIFIED (&ifc->address->u.prefix6))
        cli_out (cli, "    %R\n", &ifc->address->u.prefix6);
}

CLI (show_ipv6_interface_brief,
     show_ipv6_interface_brief_cmd,
     "show ipv6 interface brief",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "IPv6 interface status and configuration",
     "Brief summary of IPv6 status and configuration")
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp;
  struct listnode *node;

  if (argc > 0)
    {
      ifp = if_lookup_by_name (&vr->ifm, argv[0]);
      if (ifp == NULL)
        {
          cli_out (cli, "%% There is no such interface\n");
          return CLI_ERROR;
        }
      nsm_show_ipv6_if_brief (cli, ifp);
    }
  else
    {
      LIST_LOOP (vr->ifm.if_list, ifp, node)
        {
          nsm_show_ipv6_if_brief (cli, ifp);
        }
    }

  return CLI_SUCCESS;
}

ALI (show_ipv6_interface_brief,
     show_ipv6_interface_if_brief_cmd,
     "show ipv6 interface IFNAME brief",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "IPv6 interface status and configuration",
     "Interface name",
     "Brief summary of IPv6 status and configuration");
#endif /* HAVE_IPV6 */

#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)
CLI (logical_hw_addr_set,
     logical_hw_addr_set_cmd,
     "mac-address MAC",
     "Set svi interface logical hw address",
     "mac-address in HHHH.HHHH.HHHH format")
{
  struct interface *ifp;
  unsigned char mac_addr [ETHER_ADDR_LEN];
  int ret;

  ifp = (struct interface *) cli->index;
  ret = 0;

  if (!ifp)
    return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);

  if(pal_strncmp (ifp->name, "vlan", 4) != 0)
    {
       cli_out (cli, "%% Feature applicable to SVI interfaces only\n");
       return CLI_ERROR;
    }

  if (pal_sscanf (argv[0], "%4hx.%4hx.%4hx",
                  (unsigned short *)&mac_addr[0],
                  (unsigned short *)&mac_addr[2],
                  (unsigned short *)&mac_addr[4]) != 3)
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }

  /* Convert to network order */
  *(unsigned short *)&mac_addr[0] = pal_hton16(*(unsigned short *)&mac_addr[0]);
  *(unsigned short *)&mac_addr[2] = pal_hton16(*(unsigned short *)&mac_addr[2]);
  *(unsigned short *)&mac_addr[4] = pal_hton16(*(unsigned short *)&mac_addr[4]);

  if (! nsm_api_mac_address_is_valid (argv[0]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[0]);
      return CLI_ERROR;
    }

  /* Add the MAC address to the container of known MAC addresses to this
     interface and install it only if this is currently the most preferable
     one.
   */
  if (nsm_ifma_set_logical(ifp->name,
                           mac_addr,
                           ETHER_ADDR_LEN) != ZRES_OK)
    {
       cli_out (cli, "%%Failed to update mac address in hw.\n");
       return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

CLI (no_logical_hw_addr_set,
     no_logical_hw_addr_set_cmd,
     "no mac-address",
     CLI_NO_STR,
     "Set svi interface logical hw address")
{
  struct interface *ifp;
  int ret;

  ifp = (struct interface *) cli->index;
  ret = 0;

  if (!ifp)
    return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);

  if(pal_strncmp (ifp->name, "vlan", 4) != 0)
    {
       cli_out (cli, "%% Feature applicable to SVI interfaces only.\n");
       return CLI_ERROR;
    }

  /* Delete the MAC address from the container of known MAC addresses for this
     interface and reinstall the current most preferable one.
   */
  if (nsm_ifma_del_logical(ifp->name) != ZRES_OK)
    {
       cli_out (cli, "%%Failed to update mac address in hw.\n");
       return CLI_ERROR;
    }
  return CLI_SUCCESS;
}

ALI(no_logical_hw_addr_set,
    no_logical_hw_addr_mac_set_cmd,
    "no mac-address MAC",
    CLI_NO_STR,
    "Set svi interface logical hw address",
    "mac-address in HHHH.HHHH.HHHH format");

#endif /* HAVE_L3 && HAVE_INTERVLAN_ROUTING */

#ifndef IF_BANDWIDTH_INFO
#ifdef HAVE_TE
CLI (bandwidth_if,
     bandwidth_if_cmd,
     "bandwidth BANDWIDTH",
     "Set maximum bandwidth parameter",
     BANDWIDTH_RANGE)
{
  struct interface *ifp;
  int ret;

  ifp = (struct interface *) cli->index;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  ret = nsm_api_if_max_bandwidth_set (cli->vr->id, ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}

CLI (no_bandwidth_if,
     no_bandwidth_if_cmd,
     "no bandwidth",
     CLI_NO_STR,
     "Unset maximum bandwidth parameter")
{
  struct interface *ifp = cli->index;
  int ret;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_api_if_max_bandwidth_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}
#else /* ! HAVE_TE */
CLI (bandwidth_if,
     bandwidth_if_cmd,
     "bandwidth BANDWIDTH",
     "Set maximum bandwidth parameter",
     BANDWIDTH_RANGE)
{
  struct nsm_master *nm;
  struct interface *ifp = cli->index;
  struct nsm_if *zif = NULL;
  float bandwidth;
  int ret;
  cindex_t cindex = 0;

#ifdef HAVE_LACPD
  float mem_bandwidth = 0.0;
  u_int16_t mem_count = 0;
  struct interface *memifp = NULL;
  struct listnode *lsn = NULL;
#endif /* HAVE LACP */

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ret = bandwidth_string_to_float (argv[0], &bandwidth);
  if (ret < 0)
    {
      cli_out (cli, "%% Bandwidth is invalid\n");
      return CLI_ERROR;
    }

   if (ifp->duplex == NSM_IF_AUTO_NEGO)
    {
      cli_out (cli, "%% AutoNegotiation is Enabled\n");
      return CLI_ERROR;
    }

#ifdef HAVE_LACPD
  zif = (struct nsm_if *)ifp->info;
  if (zif && (zif->agg.type == NSM_IF_AGGREGATOR))
    {
      ret = nsm_fea_if_set_bandwidth (ifp, bandwidth);
      if (ret >= 0)
        {
          mem_count  = LISTCOUNT (zif->agg.info.memberlist);
          mem_bandwidth = bandwidth / (mem_count * 1.0);

          AGGREGATOR_MEMBER_LOOP (zif, memifp, lsn)
           {
             memifp->bandwidth = mem_bandwidth;
             nsm_fea_if_set_bandwidth (memifp, mem_bandwidth);
             SET_FLAG (memifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);
           }
        }
    }
  else
#endif /* HAVE_LACPD */

  ret = nsm_fea_if_set_bandwidth (ifp, bandwidth);
  if (ret < 0)
    {
      cli_out (cli, "%% Bandwidth set failed\n");
      return CLI_ERROR;
    }
  else
    {
      ifp->bandwidth = bandwidth;
      SET_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);
    }

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);

  /* Force protocols to recalculate routes due to cost change. */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

  return NSM_API_SET_SUCCESS;
}

CLI (no_bandwidth_if,
     no_bandwidth_if_cmd,
     "no bandwidth",
     CLI_NO_STR,
     "Unset bandwidth informational parameter")
{
  struct nsm_master *nm;
  struct interface *ifp = cli->index;
  cindex_t cindex = 0;
  int ret;

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  /* Set auto-negotiation. */

  if (ifp->duplex == NSM_IF_AUTO_NEGO)
    {
      ret = nsm_fea_if_set_autonego (ifp, NSM_IF_AUTONEGO_ENABLE);

      if (ret < 0)
        {
          cli_out (cli, "%% Bandwidth reset failed\n");
          return CLI_ERROR;
        }
      else
        UNSET_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);
    }
   else
     {
        cli_out (cli, "Enable Auto negotiation for resetting bandwidth \n");
        return CLI_ERROR;
     }

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);

  /* Force protocols to recalculate routes due to cost change */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

  return NSM_API_SET_SUCCESS;
}
#endif /* HAVE_TE */
#endif /* !IF_BANDWIDTH_INFO */

#ifdef HAVE_TE
#ifdef HAVE_DSTE
CLI (bw_constraint_if,
     bw_constraint_if_cmd,
     "bandwidth-constraint CT-NAME BANDWIDTH",
     "Set bandwidth constraint parameter",
     "DSTE Class Type name which bandwidth associated with.",
     "bandwidth constraint <1-10000000000 bits> (usable units : k, m, g)")
{
  struct interface *ifp;
  int ret;

  ifp = (struct interface *) cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, mpls_if_commands);

  ret = nsm_api_if_bandwidth_constraint_set (cli->vr->id, ifp->name, argv[0], argv[1]);

  return nsm_cli_return (cli, ret);
}

CLI (no_bw_constraint_if,
     no_bw_constraint_if_cmd,
     "no bandwidth-constraint CT-NAME",
     CLI_NO_STR,
     "Unset bandwidth constraint parameter",
     "DSTE Class Type name which bandwidth associated with.")
{
  struct interface *ifp;
  int ret;

  ifp = (struct interface *) cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, mpls_if_commands);

  ret = nsm_api_if_bandwidth_constraint_unset (cli->vr->id, ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}

ALI (no_bw_constraint_if,
     no_bw_constraint_if_val_cmd,
     "no bandwidth-constraint CT-NAME <1-10000000000>",
     CLI_NO_STR,
     "Unset bandwidth constraint parameter",
     "DSTE Class Type name which bandwidth associated with.",
     "Bandwidth");
#endif /* HAVE_DSTE */


CLI (max_resv_bw_if,
     max_resv_bw_if_cmd,
     "reservable-bandwidth BANDWIDTH",
     "Set reservable bandwidth parameter",
     "Reservable bandwidth <1-10000000000 bits> (usable units : k, m, g)")
{
  struct interface *ifp;
  int ret;

  ifp = (struct interface *) cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, mpls_if_commands);

  ret = nsm_api_if_reservable_bandwidth_set (cli->vr->id, ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}

CLI (no_max_resv_bw_if,
     no_max_resv_bw_if_cmd,
     "no reservable-bandwidth",
     CLI_NO_STR,
     "Unset reservable bandwidth parameter")
{
  struct interface *ifp;
  int ret;

  ifp = (struct interface *) cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, mpls_if_commands);

  ret = nsm_api_if_reservable_bandwidth_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (if_admin_group,
     if_admin_group_cmd,
     "admin-group NAME",
     "Administrative group to which this interface belongs",
     "Name of administrative group to be used")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  s_int32_t val;
  cindex_t cindex = 0;

  /* Get interface */
  ifp = cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, mpls_if_commands);

  /* Get the value for the specified admin group */
  val = admin_group_get_val (NSM_MPLS->admin_group_array, argv [0]);
  if (val < 0)
    {
      cli_out (cli, "%% Invalid administrative group specified\n");
      return CLI_ERROR;
    }

  /* Check if this is already set */
  if (CHECK_FLAG (ifp->admin_group, (1 << val)))
    {
      cli_out (cli, "%% Interface is already part of the specified "
               "administrative group\n");
      return CLI_ERROR;
    }

  /* Set this group in the interface */
  SET_FLAG (ifp->admin_group, (1 << val));

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_ADMIN_GROUP);

  /* Send update to protocols */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

  /* Return success */
  return CLI_SUCCESS;
}

CLI (if_no_admin_group,
     if_no_admin_group_cmd,
     "no admin-group NAME",
     CLI_NO_STR,
     "Administrative group to which this interface belongs",
     "Name of administrative group to be removed")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  s_int32_t val;
  cindex_t cindex = 0;

  /* Get interface */
  ifp = cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, mpls_if_commands);

  /* Get the value for the specified admin group */
  val = admin_group_get_val (NSM_MPLS->admin_group_array, argv [0]);
  if (val < 0)
    {
      cli_out (cli, "%% Invalid administrative group specified\n");
      return CLI_ERROR;
    }

  /* Compare values */
  if (! CHECK_FLAG (ifp->admin_group, (1 << val)))
    {
      cli_out (cli, "%% Interface does not belong to the specified "
               "administrative group\n");
      return CLI_ERROR;
    }

  /* Unset the interface admin group */
  UNSET_FLAG (ifp->admin_group, (1 << val));

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_ADMIN_GROUP);

  /* Send update to protocols */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

  /* Return success */
  return CLI_SUCCESS;
}
#endif /* HAVE_TE */

#ifdef HAVE_DSTE
CLI (if_bc_mode,
     if_bc_mode_cmd,
     "bc-mode MODE",
     "Configure bandwidth constraint mode",
     "Bandwidth constraint mode: MAM, RSDL or MAR")
{
  struct interface *ifp;

  /* Get interface */
  ifp = cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, mpls_if_commands);

  /* Check bandwidth constraint mode. */
  if (pal_strcmp (argv[0], "MAM") != 0
      && pal_strcmp (argv[0], "RSDL") != 0
      && pal_strcmp (argv[0], "MAR") != 0)
    {
      cli_out (cli, "%% Add failed: Invalid input parameter. \n");
      return CLI_ERROR;
    }
  else
    {
      nsm_dste_bc_mode_update (argv[0], ifp);
      return CLI_SUCCESS;
    }
}
#endif /* HAVE_DSTE */

#ifdef HAVE_L3
int
nsm_ip_address_del (struct interface *ifp, struct connected *ifc)
{
#if defined (HAVE_MCAST_IPV4) || defined (HAVE_CUSTOM1) || defined (HAVE_BFD)
  struct nsm_vrf *nv = ifp->vrf->proto;
#endif /* defined (HAVE_MCAST_IPV4) || defined (HAVE_CUSTOM1) */

#ifdef HAVE_VRRP
  /* Update VRRP. */
  if (NSM_CAP_HAVE_VRRP)
      vrrp_if_del_ipv4_addr (ifp, ifc);
#endif /* HAVE_VRRP */

#ifdef HAVE_BFD
      /* Delete static BFD session when IP address is unconfigured */
      if (!nsm_bfd_update_interface (nv, AFI_IP, ifp, ifc, session_del))
          zlog_info (NSM_ZG, "Unable to delete static BFD session");
#endif /* HAVE_BFD */

  NSM_IF_DELETE_IFC_IPV4 (ifp, ifc);

#ifdef HAVE_MCAST_IPV4
  /* Delete Primary address from IGMP interface */
  nsm_mcast_igmp_if_addr_del (nv->mcast, ifp, ifc);
#endif /* HAVE_MCAST_IPV4 */

  /* Remove static arp entries */
  nsm_delete_static_arp (ifp->vr->proto, ifc, ifp);

  if (nsm_fea_if_ipv4_address_delete (ifp, ifc) < 0)
    return NSM_API_SET_ERR_CANT_UNSET_ADDRESS;

  /* Redistribute this information. */
  nsm_server_if_address_delete_update (ifp, ifc);

  /* Remove connected route. */
  nsm_connected_down_ipv4 (ifp, ifc);

  /* Free address information. */
  ifc_free (ifp->vr->zg, ifc);

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Update the unnumbered address.  */
  nsm_if_ipv4_unnumbered_update (ifp, NULL);
#endif /* HAVE_NSM_IF_UNNUMBERED */

  /* Router ID process function. */
  ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);

#ifdef HAVE_CUSTOM1
  if (ifp->ifc_ipv4 == NULL && pal_strncmp (ifp->name, "vlan", 4) == 0)
    nm->l2.vlan_l3_num--;
#endif /* HAVE_CUSTOM1 */

  return NSM_API_SET_SUCCESS;
}

struct connected *
nsm_if_connected_primary_lookup (struct interface *ifp)
{
  struct connected *ifc;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    if (!CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
      return ifc;

  return NULL;
}

#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
struct connected *
nsm_if_connected_secondary_lookup (struct interface *ifp,
                                   struct pal_in4_addr *addr)
{
  struct connected *ifc = NULL;
  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY) &&
          IPV4_ADDR_SAME (&ifc->address->u.prefix4, addr))
        return ifc;
    }
  return NULL;
}
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

int
nsm_if_connected_count_ipv4 (struct interface *ifp)
{
  struct connected *ifc;
  int count = 0;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    count++;

  return count;
}

struct connected *
nsm_ipv4_connected_table_lookup (struct nsm_vrf *nv, struct prefix *p)
{
  struct apn_vrf *vrf = nv->vrf;
  struct route_node *rn;

  rn = route_node_get (vrf->ifv.ipv4_table, p);
  if (rn->info)
    {
      route_unlock_node (rn);
      return (struct connected *)rn->info;
    }

  return NULL;
}

struct connected *
nsm_ipv4_check_address_overlap (struct interface *ifp,
                                struct prefix *p, int secondary)
{
  struct interface *ifp_node;
  struct connected *ifc;
  struct route_node *rn;
  int ret;

  for (rn = route_top (ifp->vrf->ifv.if_table); rn; rn = route_next (rn))
    if ((ifp_node = (struct interface *) rn->info))
      if (!if_is_ipv4_unnumbered (ifp_node))
        for (ifc = ifp_node->ifc_ipv4; ifc; ifc = ifc->next)
#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
          {
            if (ifc->ifp != ifp
                || ifc->address->prefixlen != p->prefixlen)
              {
                /* Check for identical addresses with different masks */
                ret = prefix_addr_same(p, ifc->address);

                if (ret && (ifc->ifp == ifp))
                  {
                    /* If primary address exists and trying to add it as
                       secondary address, overlap error is thrown*/
                    if (secondary
                        && !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
                      {
                         /* Return the overlapped address.  */
                         route_unlock_node (rn);
                         return ifc;
                      }
                    /* If secondary address exists and trying to add it as
                       primary address, overlap error is thrown */
                    else if (!secondary
                             && CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
                      {
                        /* Return the overlapped address.  */
                         route_unlock_node (rn);
                         return ifc;
                      }
                    else
                      continue;
                  }
              }
            else
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */
              {
                if (ifc->ifp == ifp)
                      continue;

                if (p->prefixlen < ifc->address->prefixlen)
                  ret = prefix_match (p, ifc->address);
                else
                  ret = prefix_match (ifc->address, p);
#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
              }
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

            if (!ret)
              continue;

            /* Return the overlapped address.  */
            route_unlock_node (rn);
            return ifc;
          }

  return NULL;
}

int
nsm_ipv4_connected_table_set (struct apn_vrf *vrf, struct connected *new)
{
  struct route_node *rn;
  struct prefix p;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_BITLEN;
  p.u.prefix4 = new->address->u.prefix4;

  /* Assuming there is no info on the node. */
  rn = route_node_get (vrf->ifv.ipv4_table, &p);
  if (rn->info == NULL)
    {
      rn->info = new;
      return 1;
    }
  route_unlock_node (rn);

  return 0;
}

int
nsm_ipv4_connected_table_unset (struct apn_vrf *vrf, struct connected *del)
{
  struct route_node *rn;
  struct prefix p;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_BITLEN;
  p.u.prefix4 = del->address->u.prefix4;

  rn = route_node_lookup (vrf->ifv.ipv4_table, &p);
  if (rn)
    {
      if (rn->info != NULL)
        if ((struct connected *) rn->info == del)
          {
            rn->info = NULL;
            route_unlock_node (rn);
          }

      route_unlock_node (rn);
      return 1;
    }

  return 0;
}

#ifdef HAVE_IPV6
struct connected *
nsm_ipv6_check_address_overlap (struct interface *ifp, struct prefix *p)
{
  struct interface *ifp_node;
  struct connected *ifc;
  struct route_node *rn;
  int ret;

  for (rn = route_top (ifp->vrf->ifv.if_table); rn; rn = route_next (rn))
    if ((ifp_node = rn->info))
      if (!if_is_ipv6_unnumbered (ifp_node))
        for (ifc = ifp_node->ifc_ipv6; ifc; ifc = ifc->next)
          if (!IN6_IS_ADDR_LINKLOCAL (&ifc->address->u.prefix6))
#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
          {
            if (ifc->ifp != ifp
                || ifc->address->prefixlen != p->prefixlen)
              {
                /* Check for identical addresses with different masks */
                ret = prefix_addr_same(p, ifc->address);
              }
            else
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */
              {
                /* Allow IPv6 overlapping on the same interface */
                if (ifc->ifp == ifp)
                  continue;

                if (p->prefixlen < ifc->address->prefixlen)
                  ret = prefix_match (p, ifc->address);
                else
                  ret = prefix_match (ifc->address, p);
#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
              }
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

            if (!ret)
              continue;

            /* Return the overlapped address.  */
            route_unlock_node (rn);
            return ifc;
          }

  return NULL;
}

int
nsm_ipv6_connected_table_set (struct apn_vrf *vrf, struct connected *new)
{
  struct route_node *rn;
  struct prefix p;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_BITLEN;
  p.u.prefix6 = new->address->u.prefix6;

  /* Assuming there is no info on the node. */
  rn = route_node_get (vrf->ifv.ipv6_table, &p);
  if (rn->info == NULL)
    {
      rn->info = new;
      return 1;
    }
  route_unlock_node (rn);

  return 0;
}

int
nsm_ipv6_connected_table_unset (struct apn_vrf *vrf, struct connected *del)
{
  struct route_node *rn;
  struct prefix p;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_BITLEN;
  p.u.prefix6 = del->address->u.prefix6;

  rn = route_node_lookup (vrf->ifv.ipv6_table, &p);
  if (rn)
    {
      if (rn->info == del)
        {
          rn->info = NULL;
          route_unlock_node (rn);
        }

      route_unlock_node (rn);
      return 1;
    }

  return 0;
}
#endif /* HAVE_IPV6 */

int
nsm_ip_address_install (u_int32_t vr_id, char *ifname,
                        struct pal_in4_addr *addr, u_char prefixlen,
                        char *peer_str, int secondary)
{
  struct nsm_master *nm;
  struct interface *ifp;
  struct connected *ifc;
  struct connected *ifc_old = NULL;
  struct prefix_ipv4 cp;
  struct prefix_ipv4 *p;
  struct pal_in4_addr mask;
  struct nsm_if *nif;
  struct nsm_vrf *nv;
  int ret;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  /* Check IP address */
  if (IN_EXPERIMENTAL (pal_ntoh32 (addr->s_addr))
      || IN_CLASSD (pal_ntoh32 (addr->s_addr)))
    return NSM_API_SET_ERR_INVALID_IPV4_ADDRESS;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  nif = (struct nsm_if *)ifp->info;
  nv = ifp->vrf->proto;

  /* Do NOT allow IP Address change for a PPP interface. */
  if (if_is_pointopoint (ifp) && ! if_is_tunnel (ifp))
    return NSM_API_SET_ERR_CANT_SET_ADDRESS_ON_P2P;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Check for IPv4 unnumbered interface.  */
  if (if_is_ipv4_unnumbered (ifp))
    nsm_if_ipv4_unnumbered_unset (vr_id, ifname);
#endif /* HAVE_NSM_IF_UNNUMBERED */

  cp.family = AF_INET;
  cp.prefixlen = prefixlen;
  cp.prefix = *addr;

  /* If there is nothing, only allow to add primary. */
  if (ifp->ifc_ipv4 == NULL)
    if (secondary)
      {
#ifdef HAVE_NSM_IF_PARAMS
        if (pal_strncmp (ifp->name, "vlan", 4) == 0)
          if (ifp->ifindex == 0)
            return NSM_API_SET_ERR_CANT_SET_ADDRESS_WITH_ZERO_IFINDEX;
#endif /* HAVE_NSM_IF_PARAMS */

        return NSM_API_SET_ERR_CANT_SET_SECONDARY_FIRST;
      }

#ifdef NSM_INTERFACE_ADDRESS_MAX
  {
    int i = 0;

    for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
      i++;

    if (i >= NSM_INTERFACE_ADDRESS_MAX)
      return NSM_API_SET_ERR_MAX_ADDRESS_LIMIT;
  }
#endif /* NSM_INTERFACE_ADDRESS_MAX */

#ifdef HAVE_CUSTOM1
  if (pal_strncmp (ifp->name, "vlan", 4) == 0)
    if (ifp->ifc_ipv4 == NULL)
      if (nm->l2.vlan_l3_num >= nm->l2.vlan_l3_max)
        return NSM_API_SET_ERR_MAX_ADDRESS_LIMIT;

  if (ifp->hw_type == IF_TYPE_VLAN && ifp->ifc_ipv4 == NULL
      && ifp->ifindex == 0)
    {
      swif_create (IF_TYPE_VLAN, ifp->pid, NULL, NULL);
      SET_FLAG (ifp->flags, IFF_UP);
      SET_FLAG (ifp->flags, IFF_RUNNING);
      SET_FLAG (ifp->flags, IFF_LINK1);
#ifdef HAVE_HA
      lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */
    }

  /* Pending IP address assignment till ifindex becomes non-zero. */
  if (ifp->ifindex == 0)
    return NSM_API_SET_ERR_CANT_SET_ADDRESS_WITH_ZERO_IFINDEX;

  if (pal_strncmp (ifp->name, "vlan", 4) == 0)
    if (ifp->ifc_ipv4 == NULL)
      nm->l2.vlan_l3_num++;

#endif /* HAVE_CUSTOM1 */


  /* Exact the same configuration, should be silently ignored. */
  ifc = if_lookup_ifc_prefix (ifp, (struct prefix *) &cp);
  if (ifc != NULL)
    {
      /* Changing from Primary to Secondary is not allowed. */
      if (secondary && !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
        return NSM_API_SET_ERR_CANT_CHANGE_PRIMARY;

      /* Changing from Secondary to Primary is not allowed. */
      if (!secondary && CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
        return NSM_API_SET_ERR_CANT_CHANGE_SECONDARY;

      return NSM_API_SET_ERR_SAME_ADDRESS_EXIST;
    }

  /* Check overlapping address. */
  if (nsm_ipv4_check_address_overlap (ifp, (struct prefix *)&cp, secondary))
    return NSM_API_SET_ERR_ADDRESS_OVERLAPPED;

  /* Update primary. */
  /* XXX: Important to delete the "old" primary from cal so
   * that delete is checkpointed before the create of new primary
   */
  if (!secondary && ifp->ifc_ipv4)
    {
      ifc_old = nsm_if_connected_primary_lookup (ifp);
      /* delete the ip address only if it exists */
      if (ifc_old)
        {
          if (! HOST_CONFIG_READ_IS_DONE (nm->vr->host))
            {
              SET_FLAG (ifc_old->flags, NSM_IFA_SECONDARY);
#ifdef HAVE_HA
              lib_cal_modify_connected (ifp->vr->zg, ifc_old);
#endif /* HAVE_HA */
              ifc_old = NULL;
            }
          else
            {
#ifdef HAVE_PBR
              nsm_pbr_rule_add_delete_by_ip_address (RMAP_POLICY_DELETE,
                                                 ifp->vr, ifc_old->address);
#endif /* HAVE_PBR */
#ifdef HAVE_BFD
             /* Delete static BFD session when primary IP address is modified */
             if (!(nsm_bfd_update_interface (nv, AFI_IP, ifp, ifc, session_del)))
               zlog_info (NSM_ZG, "Unable to delete static BFD session");
               
#endif /* HAVE_BFD */
              NSM_IF_DELETE_IFC_IPV4 (ifp, ifc_old);
#ifdef HAVE_HA
              lib_cal_delete_connected (nzg, ifc_old);
#endif /* HAVE_HA */
            }
        }
    }
#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
  else if(secondary)
    {
      ifc_old = nsm_if_connected_secondary_lookup (ifp, addr);

      if (ifc_old)
        {
#ifdef HAVE_PBR
          nsm_pbr_rule_add_delete_by_ip_address (RMAP_POLICY_DELETE,
                                             ifp->vr, ifc_old->address);
#endif /* HAVE_PBR */
          NSM_IF_DELETE_IFC_IPV4 (ifp, ifc_old);
#ifdef HAVE_HA
          lib_cal_delete_connected (nzg, ifc_old);
#endif /* HAVE_HA */
        }
    }
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

  /* Get IPv4 connected. */
  ifc = ifc_get_ipv4 (addr, prefixlen, ifp);
  if (secondary)
    SET_FLAG (ifc->flags, NSM_IFA_SECONDARY);

  /* This address is configured from nsm. */
  SET_FLAG (ifc->conf, NSM_IFC_CONFIGURED);

  /* Broadcast. */
  if (prefixlen <= 30)
    {
      p = prefix_ipv4_new ();
      *p = cp;
      masklen2ip (p->prefixlen, &mask);
      p->prefix.s_addr |= ~mask.s_addr;
      ifc->destination = (struct prefix *) p;
    }

#ifdef HAVE_VRRP_LINK_ADDR
  if (NSM_CAP_HAVE_VRRP)
    if (vrrp_if_is_vipv4_addr (ifp, &ifc->address->u.prefix4))
      SET_FLAG (ifc->flags, NSM_IFA_VIRTUAL);
#endif /* HAVE_VRRP_LINK_ADDR */

  /* Update address to the kernel.  */
  if (ifc_old)
    {
      if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)
          && !CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
        {
#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
          if (secondary)
            ret = nsm_fea_if_ipv4_secondary_address_update (ifp,
                                                            ifc_old, ifc);
          else
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */
            ret = nsm_fea_if_ipv4_address_update (ifp, ifc_old, ifc);
          if (ret < 0)
            return NSM_API_SET_ERR_CANT_SET_ADDRESS;

#ifdef HAVE_VRRP
          /* Update VRRP. */
          if (NSM_CAP_HAVE_VRRP)
            {
              vrrp_if_del_ipv4_addr (ifp, ifc_old);
            }
#endif /* HAVE_VRRP */
          /* IP address properly set. */
          SET_FLAG (ifc->conf, NSM_IFC_REAL);

          /* Update interface address information to protocol daemon. */
          nsm_server_if_address_delete_update (ifp, ifc_old);
          nsm_server_if_address_add_update (ifp, ifc);

#ifdef HAVE_MCAST_IPV4
          /* Delete old address to IGMP interface */
          nsm_mcast_igmp_if_addr_del (nv->mcast, ifp, ifc_old);
#endif /* HAVE_MCAST_IPV4 */

          /* Register the connected route.  */
          if (if_is_running (ifp))
            nsm_connected_up_ipv4 (ifp, ifc);
        }
    }
  else
  /* Add address to the kernel.  */
#ifndef HAVE_CUSTOM1
  if (if_is_up (ifp)
      && CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)
#else /* HAVE_CUSTOM1 */
  if ((ifp->flags & IFF_UP)
#endif /* HAVE_CUSTOM1 */
      && !CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    {
      ret = nsm_fea_if_ipv4_address_add (ifp, ifc);
      if (ret < 0)
        return NSM_API_SET_ERR_CANT_SET_ADDRESS;

      /* IP address properly set. */
      SET_FLAG (ifc->conf, NSM_IFC_REAL);

      /* Update interface address information to protocol daemon. */
      nsm_server_if_address_add_update (ifp, ifc);

      /* Register the connected route.  */
      if (if_is_running (ifp))
        nsm_connected_up_ipv4 (ifp, ifc);
    }

  NSM_IF_ADD_IFC_IPV4 (ifp, ifc);

#ifdef HAVE_PBR
  nsm_pbr_rule_add_delete_by_ip_address (RMAP_POLICY_ADD,
                                    ifp->vr, ifc->address);
#endif /* HAVE_PBR */

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_NW_ADDRESS);
  nsm_server_if_update (ifp, cindex);

#ifdef HAVE_MCAST_IPV4
  /* Add Primary address to IGMP interface */
  nsm_mcast_igmp_if_addr_add (nv->mcast, ifp, ifc);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_VRRP
  if (NSM_CAP_HAVE_VRRP && CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    {
      if ((ifc_old
          && CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
          ||
#ifndef HAVE_CUSTOM1
          (if_is_up (ifp) && CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)))
#else /* HAVE_CUSTOM1 */
          (ifp->flags & IFF_UP))
#endif /* HAVE_CUSTOM1 */
        {
          /* Update VRRP. */
          vrrp_if_add_ipv4_addr (ifp, ifc);
        }
    }
#endif /* HAVE_VRRP */

  if (ifc_old)
    {
      /* Updating the static ip routes related to this interface
         in FIB database */
      nsm_rib_ipaddr_chng_ipv4_fib_update (nm, nv, ifc);

      nsm_connected_down_ipv4 (ifp, ifc_old);
      ifc_free (ifp->vr->zg, ifc_old);
    }

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Update the unnumbered address.  */
  nsm_if_ipv4_unnumbered_update (ifp, NULL);
#endif /* HAVE_NSM_IF_UNNUMBERED */

#ifdef HAVE_KERNEL_ROUTE_SYNC
  THREAD_TIMER_OFF (nm->t_rib_kernel_sync);
  thread_execute (nm->zg, nsm_rib_kernel_sync_timer, nm, 0);
#endif /* HAVE_KERNEL_ROUTE_SYNC */

#ifdef HAVE_IPSEC
  /* PAL_TRUE to denote addition of ip address */
  if ( !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    ipsec_ip_address_update (vr_id, ifp, PAL_TRUE);
#endif /* HAVE_IPSEC */
  /* Router ID process function. */
  ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);

  return CLI_SUCCESS;
}

int
nsm_ip_address_uninstall (u_int32_t vr_id, char *ifname,
                          struct pal_in4_addr *addr, u_char prefixlen,
                          char *peer_str, int secondary)
{
  struct nsm_master *nm;
  struct interface *ifp;
  struct connected *ifc;
  struct prefix_ipv4 cp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Check for IPv4 unnumbered interface.  */
  if (if_is_ipv4_unnumbered (ifp))
    return NSM_API_SET_ERR_ADDRESS_NOT_EXIST;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  /* Convert to prefix structure. */
  cp.family = AF_INET;
  cp.prefixlen = prefixlen;
  cp.prefix = *addr;

  /* Check current interface address. */
  ifc = if_lookup_ifc_prefix (ifp, (struct prefix *) &cp);
  if (ifc == NULL)
    return NSM_API_SET_ERR_ADDRESS_NOT_EXIST;

  if (secondary && !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    return NSM_API_SET_ERR_CANT_CHANGE_PRIMARY;

  if ((nsm_if_connected_count_ipv4 (ifp) > 1 )
      && !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    return NSM_API_SET_ERR_MUST_DELETE_SECONDARY_FIRST;

  /* This is not real address or interface is not active. */
  if (! CHECK_FLAG (ifc->conf, NSM_IFC_REAL)
      || ! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    {
#ifdef HAVE_VRRP_LINK_ADDR
      IF_NSM_CAP_HAVE_VRRP
        {
          if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
            vrrp_if_del_ipv4_addr (ifp, ifc);
        }
#endif /* HAVE_VRRP_LINK_ADDR */
#ifdef HAVE_PBR
      nsm_pbr_rule_add_delete_by_ip_address (RMAP_POLICY_ADD, ifp->vr,
                                         ifc->address);
#endif /* HAVE_PBR */
      NSM_IF_DELETE_IFC_IPV4 (ifp, ifc);
      ifc_free (ifp->vr->zg, ifc);

      return CLI_ERROR;
    }

#ifdef HAVE_IPSEC
  /* Send flag as PAL_FALSE to denote deletion. We will delete SA only for
     primary address */
  if ( !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    ipsec_ip_address_update (vr_id, ifp, PAL_FALSE);
#endif /* HAVE_IPSEC */
  return nsm_ip_address_del (ifp, ifc);

}

int
nsm_ip_address_uninstall_all (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct interface *ifp;
  struct connected *ifc, *next;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Check for IPv4 unnumbered interface.  */
  if (if_is_ipv4_unnumbered (ifp))
    nsm_if_ipv4_unnumbered_unset (vr_id, ifname);
#endif /* HAVE_NSM_IF_UNNUMBERED */

  if (CHECK_FLAG (ng->flags, NSM_SHUTDOWN))
    return NSM_API_SET_SUCCESS;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = next)
    {
      next = ifc->next;

      /* Skip the loopback address.  */
      if (if_is_loopback (ifp))
        if (IN_LOOPBACK (pal_ntoh32 (ifc->address->u.prefix4.s_addr)))
          continue;

#ifdef HAVE_IPSEC
  if ( !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    ipsec_ip_address_update (vr_id, ifp, PAL_FALSE);
#endif /* HAVE_IPSEC */
      nsm_ip_address_del (ifp, ifc);
    }

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_NW_ADDRESS);
  nsm_server_if_update (ifp, cindex);

  return NSM_API_SET_SUCCESS;
}
#endif /* HAVE_L3 */

#ifdef HAVE_NSM_IF_PARAMS
struct nsm_if_params *
nsm_if_params_new (char *name)
{
  struct nsm_if_params *new;

  new = XCALLOC (MTYPE_NSM_IF_PARAMS, sizeof (struct nsm_if_params));
  new->pending = list_new ();
  new->name = XSTRDUP (MTYPE_TMP, name);

  return new;
}

void
nsm_if_params_free (struct nsm_if_params *nip)
{
  struct listnode *node, *next;

  if (nip->pending)
    {
      for (node = LISTHEAD (nip->pending); node; node = next)
        {
          next = node->next;
          XFREE (MTYPE_TMP, node->data);
          list_delete_node (nip->pending, node);
        }

      list_free (nip->pending);
    }

  XFREE (MTYPE_TMP, nip->name);
  XFREE (MTYPE_NSM_IF_PARAMS, nip);
}

struct nsm_if_params *
nsm_if_params_lookup (struct nsm_master *nm, char *name)
{
  struct nsm_if_params *nip;
  struct listnode *node;

  LIST_LOOP (nm->if_params, nip, node)
    if (pal_strcmp (nip->name, name) == 0)
      return nip;

  return NULL;
}

struct nsm_if_params *
nsm_if_params_get (struct nsm_master *nm, char *name)
{
  struct nsm_if_params *nip;

  nip = nsm_if_params_lookup (nm, name);
  if (nip != NULL)
    return nip;

  nip = nsm_if_params_new (name);
  listnode_add (nm->if_params, nip);

  return nip;
}

void
nsm_if_params_delete (struct nsm_master *nm, char *name)
{
  struct nsm_if_params *nip;

  nip = nsm_if_params_lookup (nm, name);
  if (nip != NULL)
    {
      listnode_delete (nm->if_params, nip);
      nsm_if_params_free (nip);
    }
}

void
nsm_if_params_clean (struct nsm_master *nm)
{
  struct listnode *node, *next;

  for (node = LISTHEAD (nm->if_params); node; node = next)
    {
      next = node->next;
      nsm_if_params_free (node->data);
      list_delete_node (nm->if_params, node);
    }
  list_free (nm->if_params);
}

void
nsm_ip_address_pending_set (struct nsm_master *nm, char *name,
                            struct prefix_ipv4 *p, int secondary)
{
  struct nsm_if_params *nip;
  struct nsm_ip_pending *pend;

  pend = XCALLOC (MTYPE_TMP, sizeof (struct nsm_ip_pending));
  pend->p = *p;
  pend->secondary = secondary;

  nip = nsm_if_params_get (nm, name);
  listnode_add (nip->pending, pend);
}

void
nsm_if_shutdown_pending_set (struct nsm_master *nm, char *name)
{
  struct nsm_if_params *nip;

  nip = nsm_if_params_get (nm, name);
  SET_FLAG (nip->flags, NSM_IF_PARAM_SHUTDOWN);
}

void
nsm_if_params_pending_proc (struct nsm_master *nm, char *name)
{
  struct nsm_if_params *nip;
#ifdef HAVE_L3
  struct nsm_ip_pending *pend;
  struct listnode *node;
#endif /* HAVE_L3 */

  nip = nsm_if_params_lookup (nm, name);
  if (nip == NULL)
    return;

  if (!CHECK_FLAG (nip->flags, NSM_IF_PARAM_SHUTDOWN))
    nsm_if_flag_up_set (nm->vr->id, name, PAL_TRUE);

#ifdef HAVE_L3
  /* Process all pending IP addresses. */
  LIST_LOOP (nip->pending, pend, node)
    nsm_ip_address_install (nm->vr->id, name,
                            &pend->p.prefix, pend->p.prefixlen,
                            NULL, pend->secondary);
#endif /* HAVE_L3 */

  /* Set UP flag according to configuraiton. */
  if (CHECK_FLAG (nip->flags, NSM_IF_PARAM_SHUTDOWN))
    nsm_if_flag_up_unset (nm->vr->id, name, PAL_TRUE);

  nsm_if_params_delete (nm, name);
}

#endif /* HAVE_NSM_IF_PARAMS */

int
nsm_ip_address_label_set (u_int32_t vr_id, char *ifname,
                          struct pal_in4_addr *addr,
                          u_char prefixlen, char *label)
{
  struct nsm_master *nm;
  struct interface *ifp;
  struct connected *ifc;
  struct prefix p;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Convert to prefix structure. */
  p.family = AF_INET;
  p.prefixlen = prefixlen;
  p.u.prefix4 = *addr;

  /* Check current interface address. */
  ifc = if_lookup_ifc_prefix (ifp, &p);
  if (ifc == NULL)
    return NSM_API_SET_ERR_ADDRESS_NOT_EXIST;

  /* Set label. */
  if (ifc->label != NULL)
    XFREE (MTYPE_NSM_DESC, ifc->label);
  ifc->label = XSTRDUP (MTYPE_NSM_DESC, label);

  return NSM_API_SET_SUCCESS;
}

int
nsm_ip_address_label_unset (u_int32_t vr_id, char *ifname,
                            struct pal_in4_addr *addr,
                            u_char prefixlen, char *label)
{
  struct nsm_master *nm;
  struct interface *ifp;
  struct connected *ifc;
  struct prefix p;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Convert to prefix structure. */
  p.family = AF_INET;
  p.prefixlen = prefixlen;
  p.u.prefix4 = *addr;

  /* Check current interface address. */
  ifc = if_lookup_ifc_prefix (ifp, &p);
  if (ifc == NULL)
    return NSM_API_SET_ERR_ADDRESS_NOT_EXIST;

  /* Unset label. */
  XFREE (MTYPE_NSM_DESC, ifc->label);

  return NSM_API_SET_SUCCESS;
}

#ifdef HAVE_L3
CLI (ip_address,
     ip_address_cmd,
     "ip address A.B.C.D/M (secondary|)",
     "Interface Internet Protocol config commands",
     "Set the IP address of an interface",
     "IP address (e.g. 10.0.0.1/8)",
     "Secondary IP address")
{
  struct interface *ifp = cli->index;
#ifdef HAVE_NSM_IF_PARAMS
  struct nsm_master *nm = ifp->vr->proto;
#endif /* HAVE_NSM_IF_PARAMS */
#ifdef HAVE_CUSTOM1
  int shutdown_flag = (!CHECK_FLAG (ifp->flags, IFF_UP));
#endif /* HAVE_CUSTOM1 */
  struct prefix_ipv4 p;
  char *label = NULL;
  int secondary = 0;
  int ret;
  int i;

#ifdef HAVE_LACPD
  /* This check is being added here for an aggregated interface, the
   * error message was not being thrown. */
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv4_if_commands);

  CLI_GET_IPV4_PREFIX ("IPv4 address", p, argv[0]);

  if (!nsm_cli_ipv4_addr_check (&p, ifp))
    {
      cli_out (cli, "%% Bad mask /%d for address %r\n",
               p.prefixlen, &p.prefix);
      return CLI_ERROR;
    }

  for (i = 1; i < argc; i++)
    {
      if (pal_strncmp (argv[i], "s", 1) == 0)
        secondary = 1;
      else if (pal_strncmp (argv[i], "l", 1) == 0)
        label = argv[++i];
    }

  /* Special handling for loopback.  */
  if (if_is_loopback (ifp))
    {
      if (!IPV4_NET127 (pal_ntoh32 (p.prefix.s_addr)))
        secondary = 1;
      else
        {
          if (!(p.prefix.s_addr == pal_ntoh32 (INADDR_LOOPBACK) &&
              (p.prefixlen == IN_CLASSA_PREFIXLEN)))
            {
              cli_out (cli, "%% %P can't be loopback's primary address\n",&p);
              return CLI_ERROR;
            }
        }
    }

  ret = nsm_ip_address_install (cli->vr->id, ifp->name,
                                &p.prefix, p.prefixlen, NULL, secondary);
  /* Address is overlapped. */
  if (ret == NSM_API_SET_ERR_ADDRESS_OVERLAPPED)
    {
      struct connected *ifc =
        nsm_ipv4_check_address_overlap (ifp, (struct prefix *)&p, secondary);
      if(ifc && ifc->ifp)
        cli_out (cli, "%% %P overlaps with %s\n", &p, ifc->ifp->name);
      return CLI_ERROR;
    }

#ifdef HAVE_LABEL
  if (label != NULL)
    nsm_ip_address_label_set (cli->vr->id, ifp->name,
                              &p.prefix, p.prefixlen, label);
#endif /* HAVE_LABEL */

#ifdef HAVE_CUSTOM1
  if (ret == NSM_API_SET_SUCCESS && shutdown_flag)
    ret = nsm_if_flag_up_unset (cli->vr->id, ifp->name, PAL_TRUE);
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_NSM_IF_PARAMS
  if (ret == NSM_API_SET_ERR_CANT_SET_ADDRESS_WITH_ZERO_IFINDEX)
    nsm_ip_address_pending_set (nm, ifp->name, &p, secondary);
#endif /* HAVE_NSM_IF_PARAMS */

  return nsm_cli_return (cli, ret);
}

CLI (no_ip_address,
     no_ip_address_cmd,
     "no ip address (A.B.C.D/M (secondary|)|)",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "Set the IP address of an interface",
     "IP Address (e.g. 10.0.0.1/8)",
     "Secondary IP address")
{
  struct interface *ifp = cli->index;
  struct prefix_ipv4 p;
  int secondary = 0;
  int ret;
  int i;

#ifdef HAVE_LACPD
  /* This check is being added here for an aggregated interface, the
   * error message was not being thrown. */
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv4_if_commands);

  if (argc == 0)
    ret = nsm_ip_address_uninstall_all (cli->vr->id, ifp->name);
  else
    {
      CLI_GET_IPV4_PREFIX ("IPv4 address", p, argv[0]);

      for (i = 1; i < argc; i++)
        if (pal_strncmp (argv[i], "s", 1) == 0)
          secondary = 1;

      /* Special handling for loopback.  */
      if (if_is_loopback (ifp))
        {
          if (!IPV4_NET127 (pal_ntoh32 (p.prefix.s_addr)))
            secondary = 1;
          else
            {
              if (p.prefix.s_addr == pal_ntoh32 (INADDR_LOOPBACK))
                if (p.prefixlen == IN_CLASSA_PREFIXLEN)
                  {
                    cli_out (cli, "%% Can't delete the primary address %P\n",&p);
                    return CLI_ERROR;
                  }
            }
        }

      ret = nsm_ip_address_uninstall (cli->vr->id, ifp->name,
                                      &p.prefix, p.prefixlen, NULL, secondary);
    }

  return nsm_cli_return (cli, ret);
}

#ifdef HAVE_LABEL
ALI (ip_address,
     ip_address_label_cmd,
     "ip address A.B.C.D/M (label) LINE",
     "Interface Internet Protocol config commands",
     "Set the IP address of an interface",
     "IP address (e.g. 10.0.0.1/8)",
     "Label of this address",
     "Label");

ALI (ip_address,
     ip_address_secondary_label_cmd,
     "ip address A.B.C.D/M (secondary) (label) LINE",
     "Interface Internet Protocol config commands",
     "Set the IP address of an interface",
     "IP address (e.g. 10.0.0.1/8)",
     "Secondary IP address",
     "Label of this address",
     "Label");

CLI (no_ip_address_label,
     no_ip_address_label_cmd,
     "no ip address A.B.C.D/M label LINE",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "Set the IP address of an interface",
     "IP address (e.g. 10.0.0.1/8)",
     "Label of this address",
     "Label")
{
  struct interface *ifp = cli->index;
  struct prefix_ipv4 p;

#ifdef HAVE_LACPD
  /* This check is being added here for an aggregated interface, the
   * error message was not being thrown. */
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv4_if_commands);

  CLI_GET_IPV4_PREFIX ("IPv4 address", p, argv[0]);

  nsm_ip_address_label_unset (cli->vr->id, ifp->name,
                              &p.prefix, p.prefixlen, argv[1]);

  return CLI_SUCCESS;
}

ALI (no_ip_address_label,
     no_ip_address_secondary_label_cmd,
     "no ip address A.B.C.D/M secondary label LINE",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "Set the IP address of an interface",
     "IP address (e.g. 10.0.0.1/8)",
     "Secondary IP address",
     "Label of this address",
     "Label");
#endif /* HAVE_LABEL */

#ifdef HAVE_IPV6
int
nsm_ipv6_address_install (u_int32_t vr_id, char *ifname,
                          struct pal_in6_addr *addr, u_char prefixlen,
                          char *peer_str, char *label, int anycast)
{
  struct nsm_master *nm;
  struct prefix_ipv6 cp;
  struct interface *ifp;
  struct connected *ifc;
  struct nsm_if *nif;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  cp.family = AF_INET6;
  cp.prefixlen = prefixlen;
  cp.prefix = *addr;

  nif = (struct nsm_if *)ifp->info;
#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Check for IPv6 unnumbered interface.  */
  if (if_is_ipv6_unnumbered (ifp))
    nsm_if_ipv6_unnumbered_unset (vr_id, ifname);
#endif /* HAVE_NSM_IF_UNNUMBERED */

#ifdef HAVE_VRRP
  /* Prohibit configuration of virtual IP address from the CLI,
     if any VRRP session configured with this address as not address owner.
   */
  IF_NSM_CAP_HAVE_VRRP
    if (vrrp_if_is_vipv6_addr (ifp, addr))
      {
        if (! CHECK_FLAG (nif->vrrp_if.vrrp_flags, NSM_VRRP_IF_SET_VIP6))
          return NSM_API_SET_ERR_INVALID_IPV4_ADDRESS_VRRP;
      }
#endif /* HAVE_VRRP */
  ifc = nsm_connected_check_ipv6 (ifp, (struct prefix *) &cp);
  if (ifc == NULL)
    {
      if (nsm_ipv6_check_address_overlap (ifp, (struct prefix *)&cp))
        return NSM_API_SET_ERR_ADDRESS_OVERLAPPED;

      /* Address. */
      ifc = ifc_get_ipv6 (addr, prefixlen, ifp);

      /* Anycast. */
      if (anycast)
        SET_FLAG (ifc->flags, NSM_IFA_ANYCAST);

      /* Label. */
      if (label)
        ifc->label = XSTRDUP (MTYPE_NSM_DESC, label);

      /* Add to linked list. */
      NSM_IF_ADD_IFC_IPV6 (ifp, ifc);
    }

  /* This address is configured from nsm. */
  SET_FLAG (ifc->conf, NSM_IFC_CONFIGURED);

#ifdef HAVE_VRRP_LINK_ADDR
  if (NSM_CAP_HAVE_VRRP)
    if (vrrp_if_is_vipv6_addr (ifp, &ifc->address->u.prefix6)
        && CHECK_FLAG (ifc->conf, NSM_IFC_CONFIGURED))
      SET_FLAG (ifc->flags, NSM_IFA_VIRTUAL);
#endif /* HAVE_VRRP_LINK_ADDR */

#ifdef HAVE_HA
  lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */

  /* Assign to the kernel.  */
#ifndef HAVE_CUSTOM1
  if (if_is_up (ifp)
      && CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE)
#else /* HAVE_CUSTOM1 */
  if ((ifp->flags & IFF_UP)
#endif /* HAVE_CUSTOM1 */
      && !CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    {
      ret = nsm_fea_if_ipv6_address_add (ifp, ifc);
      if (ret < 0)
        {
          NSM_IF_DELETE_IFC_IPV6 (ifp, ifc);
          ifc_free (ifp->vr->zg, ifc);

          return NSM_API_SET_ERR_CANT_SET_ADDRESS;
        }
      /* Notify to the PMs.  */
      nsm_connected_valid_ipv6 (ifp, ifc);

#ifdef HAVE_VRRP
      /* Update VRRP. */
      if (NSM_CAP_HAVE_VRRP)
        vrrp_if_add_ipv6_addr (ifp, ifc);
#endif /* HAVE_VRRP */

#ifdef HAVE_NSM_IF_UNNUMBERED
      /* Update the unnumbered address.  */
      nsm_if_ipv6_unnumbered_update (ifp);
#endif /* HAVE_NSM_IF_UNNUMBERED */
    }

#ifdef HAVE_IPSEC
  /* PAL_TRUE to denote addition of ip address */
  if ( !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    ipsec_ip_address_update (vr_id, ifp, PAL_TRUE);
#endif /* HAVE_IPSEC */

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv6_address_del (struct interface *ifp, struct connected *ifc)
{

#ifdef HAVE_VRRP
  /* Update VRRP. */
  if (NSM_CAP_HAVE_VRRP)
    {
      if (vrrp_if_is_vipv6_addr (ifp, &ifc->address->u.prefix6))
        return NSM_API_SET_ERR_CANT_UNSET_ADDRESS_VRRP;

      vrrp_if_del_ipv6_addr (ifp, ifc);
    }
#endif /* HAVE_VRRP */

  /* This is real route. */
  if (nsm_fea_if_ipv6_address_delete (ifp, ifc) < 0)
    return NSM_API_SET_ERR_CANT_UNSET_ADDRESS;

  NSM_IF_DELETE_IFC_IPV6 (ifp, ifc);

  /* Notify to the PMs.  */
  nsm_connected_invalid_ipv6 (ifp, ifc);

  /* Free address information. */
  ifc_free (ifp->vr->zg, ifc);

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Update the unnumbered address.  */
  nsm_if_ipv6_unnumbered_update (ifp);
#endif /* HAVE_NSM_IF_UNNUMBERED */

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv6_address_uninstall (u_int32_t vr_id, char *ifname,
                            struct pal_in6_addr *addr, u_char prefixlen)
{
  struct nsm_master *nm;
  struct prefix_ipv6 cp;
  struct interface *ifp;
  struct connected *ifc;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Check for IPv6 unnumbered interface.  */
  if (if_is_ipv6_unnumbered (ifp))
    return NSM_API_SET_ERR_ADDRESS_NOT_EXIST;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  cp.family = AF_INET6;
  cp.prefixlen = prefixlen;
  cp.prefix = *addr;

  /* Check current interface address. */
  ifc = nsm_connected_check_ipv6 (ifp, (struct prefix *) &cp);
  if (! ifc)
    return NSM_API_SET_ERR_ADDRESS_NOT_EXIST;

  /* This is not real address or interface is not active. */
  if (! CHECK_FLAG (ifc->conf, NSM_IFC_REAL)
      || ! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    {
#ifdef HAVE_VRRP_LINK_ADDR
      IF_NSM_CAP_HAVE_VRRP
        {
          if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
            vrrp_if_del_ipv6_addr (ifp, ifc);
        }
#endif /* HAVE_VRRP_LINK_ADDR */

      NSM_IF_DELETE_IFC_IPV6 (ifp, ifc);
      ifc_free (ifp->vr->zg, ifc);

      return NSM_API_SET_SUCCESS;
    }

#ifdef HAVE_IPSEC
  /* PAL_TRUE to denote addition of ip address */
  if ( !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    ipsec_ip_address_update (vr_id, ifp, PAL_FALSE);
#endif /* HAVE_IPSEC */

  return nsm_ipv6_address_del (ifp, ifc);
}

int
nsm_ipv6_address_uninstall_all (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct interface *ifp;
  struct connected *ifc, *next;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Check for IPv6 unnumbered interface.  */
  if (if_is_ipv6_unnumbered (ifp))
    nsm_if_ipv6_unnumbered_unset (vr_id, ifname);
#endif /* HAVE_NSM_IF_UNNUMBERED */

  if (CHECK_FLAG (ng->flags, NSM_SHUTDOWN))
    return NSM_API_SET_SUCCESS;

  for (ifc = ifp->ifc_ipv6; ifc; ifc = next)
    {
      next = ifc->next;

      /* Skip the loopback address.  */
      if (if_is_loopback (ifp))
        if (IN6_IS_ADDR_LOOPBACK (&ifc->address->u.prefix6))
          continue;

#ifdef HAVE_IPSEC
  /* PAL_TRUE to denote addition of ip address */
  if ( !CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    ipsec_ip_address_update (vr_id, ifp, PAL_FALSE);
#endif /* HAVE_IPSEC */

      nsm_ipv6_address_del (ifp, ifc);
    }

  return NSM_API_SET_SUCCESS;
}

CLI (ipv6_address,
     ipv6_address_cmd,
     "ipv6 address X:X::X:X/M",
     "Interface Internet Protocol v6 config commands",
     "Set the IPv6 address of an interface",
     "IPv6 address (e.g. 3ffe:506::1/48)")
{
  int ret;
  struct prefix_ipv6 p;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  /* This check is being added here for an aggregated interface, the
   * error message was not being thrown. */
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv6_if_commands);

  CLI_GET_IPV6_PREFIX ("IPv6 prefix", p, argv[0]);

  if (!nsm_cli_ipv6_addr_check (&p, ifp))
    {
      if (p.prefixlen == IPV6_MAX_PREFIXLEN)
        cli_out (cli, "%% Bad mask /%d for address %Q\n",
                 p.prefixlen, &p);
      else
        cli_out (cli, "%% Invalid address\n");
      return CLI_ERROR;
    }

  ret = nsm_ipv6_address_install (cli->vr->id, ifp->name, &p.prefix,
                                  p.prefixlen, NULL, NULL, 0);

  /* Address is overlapped. */
  if (ret == NSM_API_SET_ERR_ADDRESS_OVERLAPPED)
    {
      struct connected *ifc =
        nsm_ipv6_check_address_overlap (ifp, (struct prefix *)&p);
      cli_out (cli, "%% %Q overlaps with %s\n", &p, ifc->ifp->name);
      return CLI_ERROR;
    }

  return nsm_cli_return (cli, ret);
}

CLI (no_ipv6_address,
     no_ipv6_address_cmd,
     "no ipv6 address X:X::X:X/M",
     CLI_NO_STR,
     "Interface Internet Protocol v6 config commands",
     "Set the IPv6 address of an interface",
     "IPv6 address (e.g. 3ffe:506::1/48)")
{
  int ret;
  struct prefix_ipv6 p;
  struct interface *ifp = cli->index;

#ifdef HAVE_LACPD
  /* This check is being added here for an aggregated interface, the
   * error message was not being thrown. */
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

#ifdef HAVE_LACPD
  /* This check is being added here for an aggregated interface, the
   * error message was not being thrown. */
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv6_if_commands);

  CLI_GET_IPV6_PREFIX ("IPv6 prefix", p, argv[0]);

  ret = nsm_ipv6_address_uninstall (cli->vr->id, ifp->name,
                                    &p.prefix, p.prefixlen);

  return nsm_cli_return (cli, ret);
}

CLI (ipv6_address_anycast,
     ipv6_address_anycast_cmd,
     "ipv6 address X:X::X:X/M anycast",
     "Interface Internet Protocol v6 config commands",
     "Set the IPv6 address of an interface",
     "IPv6 address (e.g. 3ffe:506::1/48)",
     "Anycast flag")
{
  int ret;
  struct prefix_ipv6 p;
  struct interface *ifp = cli->index;

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv6_if_commands);

  CLI_GET_IPV6_PREFIX ("IPv6 prefix", p, argv[0]);

  if (!nsm_cli_ipv6_addr_check (&p, ifp))
    {
      cli_out (cli, "%% Invalid address\n");
      return CLI_ERROR;
    }

  ret = nsm_ipv6_address_install (cli->vr->id, ifp->name, &p.prefix,
                                  p.prefixlen, NULL, NULL, 1);

  /* Address is overlapped. */
  if (ret == NSM_API_SET_ERR_ADDRESS_OVERLAPPED)
    {
      struct connected *ifc =
        nsm_ipv6_check_address_overlap (ifp, (struct prefix *)&p);
      cli_out (cli, "%% %Q overlaps with %s\n", &p, ifc->ifp->name);
      return CLI_ERROR;
    }

  return nsm_cli_return (cli, ret);
}
#endif /* HAVE_IPV6 */
#ifdef HAVE_PBR
CLI (ip_policy,
     ip_policy_cmd,
     "ip policy route-map WORD",
     CLI_IP_STR,
     CLI_POLICY_STR,
     CLI_ROUTEMAP_STR,
     "Route map tag")
{
  int ret;
  struct interface *ifp = cli->index;

  ret = nsm_pbr_ip_policy_apply (cli->vr->id, argv[0], ifp->name,
                                ifp->ifindex);

  return nsm_cli_return (cli, ret);
}
CLI (no_ip_policy,
     no_ip_policy_cmd,
     "no ip policy route-map WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_POLICY_STR,
     CLI_ROUTEMAP_STR,
     "Route map tag")
{
  int ret;
  struct interface *ifp = cli->index;

  ret = nsm_pbr_ip_policy_delete (cli->vr->id, argv[0], ifp->name,
                                  ifp->ifindex);

  return nsm_cli_return (cli, ret);
}
CLI (show_ip_policy,
     show_ip_policy_cmd,
     "show ip policy",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_POLICY_STR)
{
  nsm_pbr_ip_policy_display (cli);

  return CLI_SUCCESS;
}
#endif /* HAVE_PBR */
#endif /* HAVE_L3 */

#ifdef HAVE_SMI
#ifdef HAVE_L2
CLI (nsm_interface_no_learning,
     nsm_interface_no_learning_cmd,
     "no-learning",
     "select an interface",
     "Interface's name",
     "configure no-learning")
{
  struct interface *ifp;
  int ret = 0;

  ifp = (struct interface *) cli->index;

  ret = nsm_set_learn_disable (ifp, PAL_TRUE);
  if ( ret < 0)
    return CLI_ERROR;

  return CLI_SUCCESS;
}

CLI (no_nsm_interface_no_learning,
     no_nsm_interface_no_learning_cmd,
     "no no-learning",
      CLI_NO_STR,
     "select an interface",
     "Interface's name",
     "configure no-learning")
{
  struct interface *ifp;
  int ret = 0;

  ifp = (struct interface *) cli->index;

  ret = nsm_set_learn_disable (ifp, PAL_FALSE);
  if (ret < 0)
    return CLI_ERROR;

  return CLI_SUCCESS;
}
#endif /* HAVE_L2 */
#endif /* HAVE_SMI */

#ifdef HAVE_MPLS
static int
label_switching_enable (struct cli *cli, struct interface *ifp,
                        u_int16_t label_space)
{
  int ret;

  ret = nsm_mpls_if_enable_by_ifp (ifp, label_space);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_FAILURE:
          cli_out (cli, "%% Failed to enable label switching on interface "
                   "%s\n", ifp->name);
          break;
        case NSM_ERR_LS_ON_LOOPBACK:
          cli_out (cli, "%% Cannot enable label switching on loopback "
                   "interface\n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Enable interface for label-switching. */
CLI (if_label_switching,
     if_label_switching_cmd,
     "label-switching",
     "Enable label-switching on interface")
{
  struct interface *ifp;

  ifp = cli->index;

#ifdef HAVE_GMPLS
  NSM_CHECK_LABEL_SWITCH_ON_GMPLS_IFP (cli, ifp);
#endif /* HAVE_GMPLS */
  NSM_CHECK_L3_GMPLS_COMMANDS (cli, ifp, mpls_if_commands);

  return label_switching_enable (cli, ifp, PLATFORM_WIDE_LABEL_SPACE);
}

/* Enable interface for label-switching. */
CLI (if_label_switching_val,
     if_label_switching_val_cmd,
     "label-switching <0-60000>",
     "Enable label-switching on interface",
     "Label space value")
{
  struct interface *ifp = cli->index;
  int label_space;

#ifdef HAVE_GMPLS
  NSM_CHECK_LABEL_SWITCH_ON_GMPLS_IFP (cli, ifp);
#endif /* HAVE_GMPLS */
  NSM_CHECK_L3_GMPLS_COMMANDS (cli, ifp, mpls_if_commands);

  /* Get labelspace and confirm value. */
  CLI_GET_INTEGER_RANGE ("label-space", label_space, argv[0], 0,
                         MAXIMUM_LABEL_SPACE);

  return label_switching_enable (cli, ifp, label_space);
}

/* Disable interface for label-switching. */
CLI (if_no_label_switching,
     if_no_label_switching_cmd,
     "no label-switching",
     CLI_NO_STR,
     "Disable label-switching on interface")
{
  struct interface *ifp;
  int ret;

  ifp = cli->index;

#ifdef HAVE_GMPLS
  NSM_CHECK_LABEL_SWITCH_ON_GMPLS_IFP (cli, ifp);
#endif /* HAVE_GMPLS */

  NSM_CHECK_L3_GMPLS_COMMANDS (cli, ifp, mpls_if_commands);

  ret = nsm_mpls_if_disable_by_ifp (ifp);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_FAILURE:
          cli_out (cli, "%% Failed to disable label switching on interface "
                   "%s\n", ifp->name);
          break;
        case NSM_ERR_LS_ON_LOOPBACK:
          cli_out (cli, "%% Cannot disable label switching on loopback "
                   "interface\n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
ALI (if_no_label_switching,
     if_no_label_switching_val_cmd,
     "no label-switching <0-65535>",
     CLI_NO_STR,
     "Disable label-switching on interface",
     "Label space value");
#endif /* HAVE_MPLS */

#ifdef HAVE_VRX
int
nsm_if_localsrc_func (struct interface *ifp, int set)
{
  if (set)
    ifp->local_flag = IF_VRX_FLAG_LOCALSRC;
  else
    ifp->local_flag = 0;

  nsm_server_if_add (ifp);

  return 0;
}
#endif /* HAVE_VRX */

#ifdef HAVE_VR
int
nsm_if_bind_vr (struct interface *ifp, struct nsm_master *nm)
{
  struct apn_vr *vr = nm->vr;
  struct nsm_master *nm_old = ifp->vr->proto;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int flag;
  int i;

  if (ifp->vr == vr)
    return NSM_API_SET_SUCCESS;

  /* Remove connected route from global-RIB. */
  nsm_fea_if_flags_get (ifp);
  if ((flag = (if_is_running (ifp) || if_is_up (ifp))))
    nsm_if_flag_up_unset (ifp->vr->id, ifp->name, PAL_TRUE);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    nsm_server_send_vr_unbind (nse, ifp);

#ifdef HAVE_L3
  nsm_ip_address_uninstall_all (ifp->vr->id, ifp->name);
#ifdef HAVE_IPV6
  nsm_ipv6_address_uninstall_all (ifp->vr->id, ifp->name);
#endif /* HAVE_IPV6 */

  /* Update router-ID for bound VRF. */
  if (ifp->vrf->id != 0)
    nsm_router_id_update (ifp->vrf->proto);
#endif /*HAVE_L3 */

  /* Unbind the interface from the FIB.  */
  nsm_fea_if_unbind_vrf (ifp, ifp->vrf->fib_id);

  if_vr_unbind (&nm_old->vr->ifm, ifp->ifindex);
  if_vr_bind (&nm->vr->ifm, ifp->ifindex);

  /* Bind the interface to the FIB.  */
  nsm_fea_if_bind_vrf (ifp, ifp->vrf->fib_id);

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    /* Move ifp to correct master. */
    nsm_mpls_if_binding_update (ifp, nm_old, nm);
#endif /* HAVE_MPLS */

  UNSET_FLAG (ifp->bind, NSM_IF_BIND_VRF);

  /* Send VR bind. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      nsm_server_send_vr_bind (nse, ifp);

  /* Install connected route to the VR table. */
  if (flag)
    nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

#ifdef HAVE_L3
  /* Update router-ID for default VRF. */
  nsm_router_id_update (ifp->vrf->proto);
#endif /*HAVE_L3 */

  return 0;
}

int
nsm_if_unbind_vr (struct interface *ifp, struct nsm_master *nm)
{
  struct nsm_master *nm_new;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int flag;
  int i;

  /* Interface unbind callback.  This callback should be called
     before the interface shutdown to make sure the VTY client
     receives the message.  */
  if (nm->vr->name != NULL)
    if (nm->vr->zg->vr_callback[VR_CALLBACK_UNBIND])
      (*nm->vr->zg->vr_callback[VR_CALLBACK_UNBIND]) (nm->vr);

  /* Remove connected route from global-RIB. */
  nsm_fea_if_flags_get (ifp);
  if ((flag = (if_is_running (ifp) || if_is_up (ifp))))
    nsm_if_flag_up_unset (ifp->vr->id, ifp->name, PAL_TRUE);

  nm_new = nsm_master_lookup_by_id (nzg, 0);

#ifdef HAVE_L3
  nsm_ip_address_uninstall_all (ifp->vr->id, ifp->name);
#ifdef HAVE_IPV6
  nsm_ipv6_address_uninstall_all (ifp->vr->id, ifp->name);
#endif /* HAVE_IPV6 */
#endif /*HAVE_L3 */

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      nsm_server_send_vr_unbind (nse, ifp);

#ifdef HAVE_L3
  /* Update router-ID for bound VRF. */
  if (ifp->vrf->id != 0)
    nsm_router_id_update (ifp->vrf->proto);
#endif /*HAVE_L3 */

  /* Unbind the interface from the FIB.  */
  nsm_fea_if_unbind_vrf (ifp, ifp->vrf->fib_id);

  if_vr_unbind (&nm->vr->ifm, ifp->ifindex);
  if (nm_new && nm_new->vr)
    if_vr_bind (&nm_new->vr->ifm, ifp->ifindex);

  /* Bind the interface to the FIB.  */
  nsm_fea_if_bind_vrf (ifp, ifp->vrf->fib_id);

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    /* Move ifp to correct master. */
    nsm_mpls_if_binding_update (ifp, nm, nm_new);
#endif /* HAVE_MPLS */

  UNSET_FLAG (ifp->bind, NSM_IF_BIND_VRF);

  /* Send VR bind. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      nsm_server_send_vr_bind (nse, ifp);

  /* Install connected route to the VR table. */
  if (flag)
    nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

#ifdef HAVE_L3
  /* Update router-ID for default VRF. */
  nsm_router_id_update (ifp->vrf->proto);
#endif /*HAVE_L3 */

  return 0;
}
#endif /* HAVE_VR */

int
nsm_if_bind_vrf (struct interface *ifp, struct nsm_vrf *nv)
{
  struct apn_vrf *vrf = nv->vrf;
  struct nsm_vrf *nv_old = ifp->vrf->proto;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int flag;
  int i;

  /* Already same VRF is bound. */
  if (ifp->vrf == vrf)
    return NSM_API_SET_SUCCESS;

  /* Remove connected route from global-RIB. */
  nsm_fea_if_flags_get (ifp);
  if ((flag = (if_is_running (ifp) || if_is_up (ifp))))
    nsm_if_flag_up_unset (ifp->vr->id, ifp->name, PAL_TRUE);

#ifdef HAVE_L3
  nsm_ip_address_uninstall_all (ifp->vr->id, ifp->name);
#ifdef HAVE_IPV6
  nsm_ipv6_address_uninstall_all (ifp->vr->id, ifp->name);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

  /* Send VRF Unbind to all clients. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      nsm_server_send_vrf_unbind (nse, ifp);

  /* Unbind the interface from IGMP/MLD instances */
#ifdef HAVE_MCAST_IPV4
  nsm_mcast_vif_delete (nv_old->mcast, ifp);
  nsm_mcast_igmp_if_delete (nv_old->mcast, ifp);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  nsm_mcast6_mif_delete (nv_old->mcast6, ifp);
  nsm_mcast6_mld_if_delete (nv_old->mcast6, ifp);
#endif /* HAVE_MCAST_IPV6 */

  /* Unbind the interface from the FIB.  */
  nsm_fea_if_unbind_vrf (ifp, ifp->vrf->fib_id);

  /* Remove VRF binding from default VRF. */
  if_vrf_unbind (&nv_old->vrf->ifv, ifp->ifindex);

#ifdef HAVE_L3
  /* Update router-ID for default VRF. */
  nsm_router_id_update (nv_old);
#endif /* HAVE_L3 */

  /* Bind VRF. */
  SET_FLAG (ifp->bind, NSM_IF_BIND_VRF);
  if_vrf_bind (&nv->vrf->ifv, ifp->ifindex);

  /* Bind the interface to the FIB.  */
  nsm_fea_if_bind_vrf (ifp, ifp->vrf->fib_id);

#if defined (HAVE_VRF) && defined (HAVE_MPLS)
  /* Set binding in MPLS Forwarder. */
  if (NSM_CAP_HAVE_VRF && NSM_CAP_HAVE_MPLS)
    nsm_mpls_if_vrf_bind_by_ifp (ifp, vrf->id);
#endif /* HAVE_VRF && HAVE_MPLS */

  /* Send VRF bind to VRF capable client. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_VRF))
        nsm_server_send_vrf_bind (nse, ifp);

  /* Install connected route to the VRF table. */
  if (flag)
    nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

#ifdef HAVE_L3
  /* Update router-ID for bound VRF. */
  nsm_router_id_update (nv);
#endif /* HAVE_L3 */

  return 0;
}

int
nsm_if_unbind_vrf (struct interface *ifp, struct nsm_vrf *nv)
{
  struct apn_vrf *vrf = nv->vrf;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int flag;
  int i;

#ifdef HAVE_L3
  nsm_static_delete_by_if (nv, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_static_delete_by_if (nv, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */

  /* Delete static multicast routes. */
  nsm_static_mroute_delete_by_if (vrf->proto, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_static_mroute_delete_by_if (vrf->proto, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */

#endif /* HAVE_L3 */

  /* If interface is up, shut it down temporarily to remove routes. */
  nsm_fea_if_flags_get (ifp);
  if ((flag = (if_is_running (ifp) || if_is_up (ifp))))
    nsm_if_flag_up_unset (ifp->vr->id, ifp->name, PAL_TRUE);

#ifdef HAVE_L3
  nsm_ip_address_uninstall_all (ifp->vr->id, ifp->name);
#ifdef HAVE_IPV6
  nsm_ipv6_address_uninstall_all (ifp->vr->id, ifp->name);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

  /* Send VRF Unbind to all clients. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_VRF))
        nsm_server_send_vrf_unbind (nse, ifp);

  /* Unbind the interface from IGMP/MLD instances */
#ifdef HAVE_MCAST_IPV4
  nsm_mcast_vif_delete (nv->mcast, ifp);
  nsm_mcast_igmp_if_delete (nv->mcast, ifp);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  nsm_mcast6_mif_delete (nv->mcast6, ifp);
  nsm_mcast6_mld_if_delete (nv->mcast6, ifp);
#endif /* HAVE_MCAST_IPV6 */

  /* Unbind the interface from the FIB.  */
  nsm_fea_if_unbind_vrf (ifp, ifp->vrf->fib_id);

  /* Change VRF binding. */
  if_vrf_unbind (&vrf->ifv, ifp->ifindex);

#ifdef HAVE_L3
  /* Update router-ID for bound VRF. */
  nsm_router_id_update (nv);
#endif /* HAVE_L3 */

#if defined (HAVE_VRF) && defined (HAVE_MPLS)
  /* Unset binding in MPLS Forwarder. */
  if (NSM_CAP_HAVE_VRF && NSM_CAP_HAVE_MPLS)
    nsm_mpls_if_vrf_unbind_by_ifp (ifp);
#endif /* HAVE_VRF && HAVE_MPLS */

  /* Unset VRF bind flag. */
  UNSET_FLAG (ifp->bind, NSM_IF_BIND_VRF);

  /* Bind back to the default VRF.  */
  if (!IS_APN_VRF_DEFAULT (nv->vrf))
    {
      struct nsm_vrf *nv_new;

      /* Lookup the default VRF.  */
      nv_new = nsm_vrf_lookup_by_id (nv->nm, 0);
      if (nv_new == NULL)
        return 0;

      if_vrf_bind (&nv_new->vrf->ifv, ifp->ifindex);

      /* Bind the interface to the FIB.  */
      nsm_fea_if_bind_vrf (ifp, ifp->vrf->fib_id);

      /* Send VRF bind to all clients. */
      NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
        if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
          nsm_server_send_vrf_bind (nse, ifp);

      if (flag)
        nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

#ifdef HAVE_L3
      /* Update router-ID for default VRF. */
      nsm_router_id_update (ifp->vrf->proto);
#endif /* HAVE_L3 */
    }

  return 0;
}

/* Clean VR/VRF bind when interface is deleted. */
int
nsm_if_bind_clean (struct interface *ifp)
{
#ifdef HAVE_LACPD
  struct nsm_if *zif = ifp->info;
#endif /* HAVE_LACPD */
  struct apn_vrf *vrf = ifp->vrf;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nsm_vrf *nv;
  int i;

  nv = vrf->proto;

#ifdef HAVE_L3
  nsm_static_delete_by_if (nv, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_static_delete_by_if (nv, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */

  /* Delete static multicast routes. */
  nsm_static_mroute_delete_by_if (nv, AFI_IP, ifp);
#ifdef HAVE_IPV6
  nsm_static_mroute_delete_by_if (nv, AFI_IP6, ifp);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

  /* Send VRF Unbind to all clients. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_VRF))
        nsm_server_send_vrf_unbind (nse, ifp);

  /* Unbind the interface from IGMP/MLD instances */
#ifdef HAVE_MCAST_IPV4
  nsm_mcast_vif_delete (nv->mcast, ifp);
  nsm_mcast_igmp_if_delete (nv->mcast, ifp);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  nsm_mcast6_mif_delete (nv->mcast6, ifp);
  nsm_mcast6_mld_if_delete (nv->mcast6, ifp);
#endif /* HAVE_MCAST_IPV6 */

#if defined (HAVE_VRF) && defined (HAVE_MPLS)
  /* Unset binding in MPLS Forwarder. */
  if (NSM_CAP_HAVE_VRF && NSM_CAP_HAVE_MPLS)
    if (vrf->id != 0)
      nsm_mpls_if_vrf_unbind_by_ifp (ifp);
#endif /* HAVE_VRF && HAVE_MPLS */

#ifdef HAVE_LACPD
  if ((zif) && (NSM_INTF_TYPE_AGGREGATOR(ifp)) &&
      (listcount(zif->agg.info.memberlist) > 0))
    return 0;
#endif /* HAVE_LACPD */

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE))
      nsm_server_send_vr_unbind (nse, ifp);

  /* if_vr_unbind is handled in if_delete of nsm_if_delete */

  return 0;
}

#ifdef HAVE_VRF
int
nsm_ip_vrf_forwarding_set (struct apn_vr *vr, char *ifname, char *vrf_name)
{
  struct nsm_master *nm = vr->proto;
  struct nsm_vrf *nv;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_if *mif;
#endif /* HAVE_MPLS_VC */
  struct interface *ifp;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    return NSM_API_SET_ERR_IF_NOT_ACTIVE;

#ifdef HAVE_VPLS
  if (NSM_IF_BIND_CHECK (ifp, VPLS))
    return NSM_API_SET_ERR_VPLS_BOUND;
#endif /* HAVE_VPLS */

  nv = nsm_vrf_lookup_by_name (nm, vrf_name);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  if (ifp->vrf->id != 0)
    return NSM_API_SET_ERR_VRF_BOUND;

#ifdef HAVE_MPLS_VC
  /* If already bound to a Virtual Circuit, return error. */
  mif = nsm_mpls_if_lookup (ifp);
  if (mif != NULL)
    if (LISTCOUNT (mif->vc_info_list) > 0)
      return NSM_ERR_VPN_EXISTS_ON_IF;
  /* If a VC has been configured for this interface, but the
     VC hasn't been created yet, binding this interface to a VRF
     should still be illegal. */
#endif /* HAVE_MPLS_VC */

  /* Bind VRF to interface. */
  return nsm_if_bind_vrf (ifp, nv);
}

int
nsm_ip_vrf_forwarding_unset (struct apn_vr *vr, char *ifname, char *vrf_name)
{
  struct nsm_master *nm = vr->proto;
  struct nsm_vrf *nv;
  struct interface *ifp;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    return NSM_API_SET_ERR_IF_NOT_ACTIVE;

  if (ifp->vrf->id == 0)
    return NSM_API_SET_ERR_VRF_NOT_BOUND;

  if (pal_strcmp (ifp->vrf->name, vrf_name) != 0)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  nv = nsm_vrf_lookup_by_name (nm, vrf_name);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  if (ifp->vrf != nv->vrf)
    return NSM_API_SET_ERR_DIFFERENT_VRF_BOUND;

  /* Unbind VRF from interface. */
  return nsm_if_unbind_vrf (ifp, nv);
}

CLI (ip_vrf_forwarding,
     ip_vrf_forwarding_cmd,
     "ip vrf forwarding WORD",
     CLI_IP_STR,
     CLI_VRF_STR,
     "Associate interface with specific VRF",
     CLI_VRF_NAME_STR)
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, l3_if_commands);

  if (if_is_loopback (ifp))
    {
      cli_out (cli, "%% Loopback interface can not be bound to VRF\n");
      return CLI_ERROR;
    }

  ret = nsm_ip_vrf_forwarding_set (vr, ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}

CLI (no_ip_vrf_forwarding,
     no_ip_vrf_forwarding_cmd,
     "no ip vrf forwarding WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_VRF_STR,
     "Clear association of interface with specific VRF",
     CLI_VRF_NAME_STR)
{
  struct apn_vr *vr = cli->vr;
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, l3_if_commands);

  if (if_is_loopback (ifp))
    {
      cli_out (cli, "%% Loopback interface can not be unbound from default VRF\n");
      return CLI_ERROR;
    }

  ret = nsm_ip_vrf_forwarding_unset (vr, ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS_VC
int
nsm_if_mpls_l2_circuit_bind (struct cli *cli, char *vc_name, char *vc_type,
                             u_int16_t vlan_id, u_char vc_mode)
{
  struct interface *ifp;
  u_int16_t type, hw_type;
  int ret = NSM_SUCCESS;

  /* Get interface. */
  ifp = cli->index;

  /* Figure out VC type. */
#ifdef HAVE_L2
  if (ifp->type != IF_TYPE_L2)
    {
      cli_out (cli, "%% Virtual Circuit need bind to Switchport\n");
      return CLI_ERROR;
    }
#ifdef HAVE_GMPLS
  if (ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)
    {
      cli_out (cli, "%% Virtual Circuit need bind to Non-GMPLS port\n");
      return CLI_ERROR;
    }
#endif /* HAVE_GMPLS */
#endif /* HAVE_L2 */

#ifdef HAVE_VPLS
  if (CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS))
    {
      cli_out (cli, "%% Interface bound to VPLS\n");
      return CLI_ERROR;
    }
#endif /* HAVE_VPLS */

  type = VC_TYPE_UNKNOWN;
  if (vc_type)
    {
      if (pal_strncasecmp (vc_type, "ethernet", pal_strlen (vc_type)) == 0)
        type = VC_TYPE_ETHERNET;
      else if (pal_strncasecmp (vc_type, "vlan", pal_strlen (vc_type)) == 0)
        type = VC_TYPE_ETH_VLAN;
      else if (pal_strncasecmp (vc_type, "ppp", pal_strlen (vc_type)) == 0)
        type = VC_TYPE_PPP;
      else if (pal_strncasecmp (vc_type, "hdlc", pal_strlen (vc_type)) == 0)
        type = VC_TYPE_HDLC;
      else
        {
          cli_out (cli, "%% Virtual Circuit type is invalid\n");
          return CLI_ERROR;
        }
    }
  else if (vlan_id == 0)
    {
      /* Guess interface type.  */
      hw_type = if_get_hw_type (ifp);
      if (hw_type == IF_TYPE_ETHERNET)
        type = VC_TYPE_ETHERNET;
      else if (if_is_pointopoint (ifp))
        type = VC_TYPE_PPP;
      else if (hw_type == IF_TYPE_HDLC)
        type = VC_TYPE_HDLC;
      else
        type = VC_TYPE_UNKNOWN;
    }
  else
    type = VC_TYPE_ETH_VLAN;

#ifdef HAVE_VLAN
  /* This interface needs binding to a bridge.
     If vlan_port mode is ACCESS, only vlan-id 0 is allowed. */
  if (vlan_id != 0)
    ret = nsm_vlan_id_validate_by_interface (ifp, vlan_id);
#endif /* HAVE_VLAN */

  if (ret == NSM_SUCCESS)
    ret = nsm_mpls_l2_circuit_bind_by_ifp (ifp, vc_name, type, vlan_id, vc_mode);

  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_VC_TYPE_UNKNOWN:
          cli_out (cli, "%% Virtual Circuit type for interface %s is "
                   "unknown\n", ifp->name);
          break;
        case NSM_ERR_NOT_FOUND:
          cli_out (cli, "%% Interface %s is not valid\n", ifp->name);
          break;
        case NSM_ERR_VC_IN_USE:
          cli_out (cli, "%% Operation not allowed. MPLS Layer-2 Virtual "
                   "Circuit %s is already in use\n", vc_name);
          break;
        case NSM_ERR_VPN_EXISTS_ON_IF:
          cli_out (cli, "%% Interface %s is already bound to a Virtual "
                   "Circuit or a Layer-3 VPN\n", ifp->name);
          break;
        case NSM_ERR_VPN_EXISTS_ON_IF_VLAN:
          cli_out (cli, "%% Interface %s vlan-id %d is already bound to "
                   "a Virtual Circuit or a Layer-3 VPN\n", ifp->name, vlan_id);
          break;
        case NSM_ERR_VPN_BIND_ON_IF_VLAN:
          cli_out (cli, "%% Virtual Circuit %s already bind to interface %s\n",
                   vc_name, ifp->name);
          break;
        case NSM_ERR_MEM_ALLOC_FAILURE:
          cli_out (cli, "%% Operation failed. No memory available\n");
          break;
        case NSM_ERR_VC_TYPE_MISMATCH:
          cli_out (cli, "%% Operation not allowed. Please unbind Virtual "
                   "Circuit from interface first\n");
          break;
        case NSM_ERR_IF_BIND_VLAN_ERR:
          cli_out (cli, "%% Operation not allowed. Interface need bind to "
                   "a bridge and if switchport mode is ACCESS, only no "
                   "vlan type can be accepted. \n");
          break;
        case NSM_ERR_VC_MODE_MISMATCH:
          cli_out (cli, "%% Operation not allowed. Can not bind Secondary VC "
                   "when sibling already configured Standby mode. \n ");
          break;
        case NSM_ERR_VC_MODE_MISMATCH_REV:
          cli_out (cli, "%% Operation not allowed. Can not bind primary VC "
                   "when sibling is primary and already configured Revertive "
                   "mode. \n ");
          break;
        default:
          cli_out (cli, "%% Failed to bind interface to Virtual Circuit\n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* The following to be used for testing only. */
#ifdef HAVE_VLAN
CLI (if_mpls_l2_circuit,
     if_mpls_l2_circuit_cmd,
     "mpls-l2-circuit NAME ((ethernet|ppp|hdlc|vlan <2-4094>|) (primary|secondary|)|)",
     "Bind interface to an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Ethernet",
     "Point-to-Point",
     "Hdlc",
     "Ethernet VLAN",
     "Vlan Identifier",
     "Primary Link",
     "Secondary Link - Will not be activated unless primary fails")

{
  u_int16_t vlan_id = 0;
  char *vc_type = NULL;
  u_char vc_mode = 0;

  switch (argc)
    {
    case 2:
      if (pal_strncasecmp (argv[1], "secondary", pal_strlen (argv[1])) == 0)
        vc_mode = NSM_MPLS_VC_CONF_SECONDARY;
      else
        vc_type = argv[1];
      break;
    case 3:
      if (argv[1][0] == 'v')
        {
          vc_type = argv[1];
          CLI_GET_INTEGER ("VLAN id", vlan_id, argv[2]);
        }
      else
        {
          if (pal_strncasecmp (argv[2], "secondary", pal_strlen (argv[2])) == 0)
            vc_mode = NSM_MPLS_VC_CONF_SECONDARY;
          vc_type = argv[1];
        }
      break;
    case 4:
       vc_type = argv[1];
       CLI_GET_INTEGER ("VLAN id", vlan_id, argv[2]);
       if (pal_strncasecmp (argv[3], "secondary", pal_strlen (argv[3])) == 0)
         vc_mode = NSM_MPLS_VC_CONF_SECONDARY;
       break;
    default:
      break;
    }

  return nsm_if_mpls_l2_circuit_bind (cli, argv[0], vc_type, vlan_id, vc_mode);
}
#else /* HAVE_VLAN */
CLI (if_mpls_l2_circuit,
     if_mpls_l2_circuit_cmd,
     "mpls-l2-circuit NAME ((ethernet|ppp|hdlc|) (primary|secondary|)|)",
     "Bind interface to an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Ethernet",
     "Point-to-Point",
     "Hdlc",
     "Primary Link",
     "Secondary Link - Will not be activated unless primary fails")
{
  u_int16_t vlan_id = 0;
  char *vc_type = NULL;
  u_char vc_mode = 0;

  switch (argc)
    {
    case 2:
      if (pal_strncasecmp (argv[1], "secondary", pal_strlen (argv[1])) == 0)
        vc_mode = NSM_MPLS_VC_CONF_SECONDARY;
      else
        vc_type = argv[1];
      break;
    case 3:
      if (pal_strncasecmp (argv[2], "secondary", pal_strlen (argv[2])) == 0)
        vc_mode = NSM_MPLS_VC_CONF_SECONDARY;
        vc_type = argv[1];
      break;
    default:
      break;
    }

  return nsm_if_mpls_l2_circuit_bind (cli, argv[0], vc_type, vlan_id, vc_mode);
}
#endif /* HAVE_VLAN */

CLI (if_no_mpls_l2_circuit,
     if_no_mpls_l2_circuit_cmd,
     "no mpls-l2-circuit NAME (ethernet|ppp|hdlc|)",
     CLI_NO_STR,
     "Unbind interface from an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Ethernet",
     "Point-to-Point",
     "Hdlc")
{
  struct interface *ifp;
  u_int16_t vlan_id = 0;
  int ret;

  /* Get the vid and check if it is within range (2-4094). */
  if (argc > 2)
    CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[2], 2,
                            4094);

  /* Get interface. */
  ifp = cli->index;

  ret = nsm_mpls_l2_circuit_unbind_by_ifp (ifp, argv[0], vlan_id);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_VC_NOT_CONFIGURED:
          cli_out (cli, "%% No MPLS Layer-2 Circuit bound to interface "
                   "%s\n", ifp->name);
          break;
        case NSM_ERR_MIF_VC_MISMATCH:
          cli_out (cli, "%% MPLS Layer-2 Circuit %s not bound to "
                   "interface %s\n", argv[0], ifp->name);
          break;
        case NSM_ERR_NOT_FOUND:
          cli_out (cli, "%% Interface %s is not valid\n", ifp->name);
          break;
        case NSM_ERR_VC_ID_NOT_FOUND:
          cli_out (cli, "%% MPLS Layer-2 Circuit %s not bound to vlan-id %d\n",
                   argv[0], vlan_id);
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_VLAN
ALI (if_no_mpls_l2_circuit,
     if_no_mpls_l2_circuit_type_cmd,
     "no mpls-l2-circuit NAME (vlan <2-4094>|)",
     CLI_NO_STR,
     "Unbind interface from an MPLS Layer-2 Virtual Circuit",
     "Identifying string for MPLS Layer-2 Virtual Circuit",
     "Ethernet VLAN",
     "Vlan Identifier");
#endif /* HAVE_VLAN */

int
nsm_if_mpls_l2_circuit_runtime_mode_conf (struct cli *cli, u_char vc_mode,
                                          u_int16_t vlan_id)
{
  struct interface *ifp;
  int ret = 0;

  /* Get interface */
  ifp = cli->index;

  if (vlan_id == 0 && ! CHECK_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC))
    {
      cli_out (cli, "%% Virtual Circuit not bind to port \n");
      return CLI_ERROR;
    }

  if (vlan_id != 0 && ! CHECK_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN))
    {
      cli_out (cli, "%% Virtual Circuit not bind to port Vlan \n");
      return CLI_ERROR;
    }

  ret = nsm_mpls_l2_circuit_runtime_mode_conf (ifp, vc_mode, vlan_id);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_NOT_FOUND:
          cli_out (cli, "%% Interface %s is not valid\n", ifp->name);
          break;
        case NSM_ERR_VPN_EXISTS_ON_IF:
          cli_out (cli, "%% Interface %s is bound to a Virtual "
                   "Circuit or a Layer-3 VPN\n", ifp->name);
          break;
        case NSM_ERR_VC_MODE_MISMATCH:
          cli_out (cli, "%% All bound VC need to be Primary \n");
          break;
        case NSM_ERR_VC_MODE_CONFLICT:
          cli_out (cli, "%% Already configured REVERTIIVE mode \n");
          break;
        case NSM_ERR_VC_NOT_CONFIGURED:
          cli_out (cli, "%% Could not find bound VC \n");
          break;
        default:
          cli_out (cli, "%% Failed to sets runtime mode to binded VC \n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

int
nsm_if_mpls_l2_circuit_revertive_mode_conf (struct cli *cli, u_char vc_mode,
                                            u_int16_t vlan_id)
{
  struct interface *ifp;
  int ret = 0;

  /* Get interface */
  ifp = cli->index;

  if (vlan_id == 0 && ! CHECK_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC))
    {
      cli_out (cli, "%% Virtual Circuit not bind to port \n");
      return CLI_ERROR;
    }

  if (vlan_id != 0 && ! CHECK_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN))
    {
      cli_out (cli, "%% Virtual Circuit not bind to port Vlan \n");
      return CLI_ERROR;
    }

  ret = nsm_mpls_l2_circuit_revertive_mode_conf (ifp, vc_mode, vlan_id);
  if (ret < 0)
    {
      switch (ret)
        {
        case NSM_ERR_NOT_FOUND:
          cli_out (cli, "%% Interface %s is not valid\n", ifp->name);
          break;
        case NSM_ERR_VPN_EXISTS_ON_IF:
          cli_out (cli, "%% Interface %s is bound to a Virtual "
                   "Circuit or a Layer-3 VPN\n", ifp->name);
          break;
        case NSM_ERR_VC_NOT_CONFIGURED:
          cli_out (cli, "%% Could not find bound VC \n");
          break;
        case NSM_ERR_VC_MODE_CONFLICT:
          cli_out (cli, "%% Already configured STANDBY mode \n");
          break;
        case NSM_ERR_VC_MODE_MISMATCH_REV:
          cli_out (cli, "%% Could not configure Revertive mode for primary VC "
                   "with sibling is also primary. Revertive is for "
                   "primary/secondary pair. \n");
          break;
        default:
          cli_out (cli, "%% Failed to sets runtime mode to binded VC \n");
          break;
        }
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_VLAN
CLI (if_mpls_l2_circuit_runtime_mode,
     if_mpls_l2_circuit_runtime_mode_cmd,
     "vc-mode (standby) (vlan <2-4094>|)",
     "Virtual Circuit mode",
     "VC runtime Standby mode",
     "Ethernet VLAN",
     "Vlan Identifier")
{
  u_char vc_mode = NSM_MPLS_VC_CONF_STANDBY;
  u_int16_t vlan_id = 0;

  if (argc > 1)
    CLI_GET_INTEGER ("VLAN id", vlan_id, argv[2]);

  return nsm_if_mpls_l2_circuit_runtime_mode_conf (cli, vc_mode, vlan_id);
}

CLI (if_no_mpls_l2_circuit_runtime_mode,
     if_no_mpls_l2_circuit_runtime_mode_cmd,
     "no vc-mode (standby) (vlan <2-4094>|)",
     CLI_NO_STR,
     "Virtual Circuit runntime mode",
     "VC runtime Standby mode",
     "Ethernet VLAN",
     "Vlan Identifier")
{
  u_char vc_mode = 0;
  u_int16_t vlan_id = 0;

  if (argc > 1)
    CLI_GET_INTEGER ("VLAN id", vlan_id, argv[2]);

  return nsm_if_mpls_l2_circuit_runtime_mode_conf (cli, vc_mode, vlan_id);
}

CLI (if_mpls_l2_circuit_revertive_mode,
     if_mpls_l2_circuit_revertive_mode_cmd,
     "vc-mode (revertive) (vlan <2-4094>|)",
     "Virtual Circuit redundancy mode",
     "VC redundancy revertive mode",
     "Ethernet VLAN",
     "Vlan Identifier")
{
  u_char vc_mode = NSM_MPLS_VC_CONF_REVERTIVE;
  u_int16_t vlan_id = 0;

  if (argc > 1)
    CLI_GET_INTEGER ("VLAN id", vlan_id, argv[2]);

  return nsm_if_mpls_l2_circuit_revertive_mode_conf (cli, vc_mode, vlan_id);
}

CLI (if_no_mpls_l2_circuit_revertive_mode,
     if_no_mpls_l2_circuit_revertive_mode_cmd,
     "no vc-mode (revertive) (vlan <2-4094>|)",
     CLI_NO_STR,
     "Virtual Circuit redundancy mode",
     "VC redundancy revertive mode",
     "Ethernet VLAN",
     "Vlan Identifier")
{
  u_char vc_mode = 0;
  u_int16_t vlan_id = 0;

  if (argc > 1)
    CLI_GET_INTEGER ("VLAN id", vlan_id, argv[2]);

  return nsm_if_mpls_l2_circuit_revertive_mode_conf (cli, vc_mode, vlan_id);
}

#else
CLI (if_mpls_l2_circuit_runtime_mode,
     if_mpls_l2_circuit_runtime_mode_cmd,
     "vc-mode (standby)",
     "Virtual Circuit mode",
     "VC runtime Standby mode")
{
  u_char vc_mode = NSM_MPLS_VC_CONF_STANDBY;
  u_int16_t vlan_id = 0;

  return nsm_if_mpls_l2_circuit_runtime_mode_conf (cli, vc_mode, vlan_id);
}

CLI (if_no_mpls_l2_circuit_runtime_mode,
     if_no_mpls_l2_circuit_runtime_mode_cmd,
     "no vc-mode (standby)",
      CLI_NO_STR,
     "Virtual Circuit runntime mode",
     "VC runtime Standby mode")
{
  u_char vc_mode = 0;
  u_int16_t vlan_id = 0;

  return nsm_if_mpls_l2_circuit_runtime_mode_conf (cli, vc_mode, vlan_id);
}

CLI (if_mpls_l2_circuit_revertive_mode,
     if_mpls_l2_circuit_revertive_mode_cmd,
     "vc-mode (revertive)",
     "Virtual Circuit redundancy mode",
     "VC redundancy revertive mode")
{
  u_char vc_mode = NSM_MPLS_VC_CONF_REVERTIVE;
  u_int16_t vlan_id = 0;

  return nsm_if_mpls_l2_circuit_revertive_mode_conf (cli, vc_mode, vlan_id);
}

CLI (if_no_mpls_l2_circuit_revertive_mode,
     if_no_mpls_l2_circuit_revertive_mode_cmd,
     "no vc-mode (revertive)",
     CLI_NO_STR,
     "Virtual Circuit redundancy mode",
     "VC redundancy revertive mode")
{
  u_char vc_mode = 0;
  u_int16_t vlan_id = 0;

  return nsm_if_mpls_l2_circuit_revertive_mode_conf (cli, vc_mode, vlan_id);
}
#endif /* HAVE_VLAN */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
int
nsm_if_vpls_bind (char *sz_name, struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_vpls *vpls;
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *vc_info, *vc_info_temp = NULL;
  struct listnode *ln;
  u_int16_t vc_type = vlan_id ? VC_TYPE_ETH_VLAN : IF_TYPE_ETHERNET;
  u_char bind_type = vlan_id ? NSM_IF_BIND_VPLS_VLAN : NSM_IF_BIND_VPLS;
  int ret = NSM_SUCCESS;
  int ret1;

  if (if_is_loopback (ifp))
    return NSM_ERR_INVALID_INTERFACE;

  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return NSM_ERR_INTERNAL;

  vpls = nsm_vpls_lookup_by_name (nm, sz_name);
  if (vpls)
    {
      if (CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU))
        {
          if (ifp->mtu < vpls->ifmtu)
           return NSM_VPLS_MTU_ERR;
        }
      else
        vpls->ifmtu = ifp->mtu;
    }

  if (FLAG_ISSET (ifp->bind, NSM_IF_BIND_VRF) ||
      FLAG_ISSET (ifp->bind, NSM_IF_BIND_MPLS_VC) ||
      FLAG_ISSET (ifp->bind, NSM_IF_BIND_VPLS))
    return NSM_ERR_IF_BINDING_EXISTS;

  if (vlan_id == 0 &&
      (FLAG_ISSET (ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN) ||
       FLAG_ISSET (ifp->bind, NSM_IF_BIND_VPLS_VLAN)))
    return NSM_ERR_IF_BINDING_EXISTS;

  ret1 = nsm_mpls_l2_circuit_bind_check (mif, vc_type, sz_name, vlan_id,
                                         0 , bind_type,
                                         &vc_info_temp);
  if (ret1 != NSM_SUCCESS)
    return ret1;

  vc_info = nsm_mpls_vc_info_create (sz_name, mif, VC_TYPE_UNKNOWN, vlan_id, 0,
                                     vlan_id ?
                                     NSM_IF_BIND_VPLS_VLAN :
                                     NSM_IF_BIND_VPLS);
  if (!vc_info)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  if (vlan_id == 0)
    SET_FLAG (ifp->bind, NSM_IF_BIND_VPLS);
  else
    SET_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN);

#ifdef HAVE_VLAN
  if (vlan_id != 0)
    ret = nsm_is_vlan_exist (ifp, vlan_id);
#endif /* HAVE_VLAN */

  /* send vpls interface binding msg to ldp */
  if (ret == NSM_SUCCESS)
    nsm_vpls_if_add_send (ifp, vlan_id);

  if (vpls)
    {
      vc_info->u.vpls = vpls;
      nsm_vpls_interface_add (vpls, ifp, vc_info);
      LIST_LOOP (vpls->vpls_info_list, vc_info, ln)
        {
          if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
                CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
              vc_info->u.vpls)
            if (! CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU))
              {
                if (vc_info->mif->ifp->mtu < vpls->ifmtu)
                  vpls->ifmtu = vc_info->mif->ifp->mtu;
              }
        }
    }

  return NSM_SUCCESS;
}

int
nsm_if_vpls_unbind (char *sz_name, struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct listnode *ln, *next;
  struct nsm_vpls *vpls = NULL;
  struct nsm_mpls_if *mif;
  int found = 0;
  int ret = NSM_SUCCESS;
  int flag, temp = 0;

  if (if_is_loopback (ifp))
    return NSM_ERR_INVALID_INTERFACE;

  if ((! CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS)) &&
      (! CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN)))
    return NSM_ERR_IF_NOT_BOUND;

  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return NSM_ERR_INTERNAL;

  if ((!sz_name) &&
      (CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN)))
    return NSM_ERR_INVALID_VPLS;

  if (sz_name && vlan_id == 0 &&
      (CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN)))
    return NSM_ERR_INVALID_VPLS;

  if (sz_name)
    {
      vpls = nsm_vpls_lookup_by_name (nm, sz_name);

      for (ln = LISTHEAD (mif->vpls_info_list); ln; ln = next)
        {
          next = ln->next;
          vc_info = ln->data;
          if (vc_info == NULL)
            continue;

          if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
              (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS)))
            continue;

          if (pal_strcmp (vc_info->vc_name, sz_name) == 0)
            {
             if (vpls && vpls->ifmtu == vc_info->mif->ifp->mtu)
               flag = 1;

              /* vlan case */
              if (vlan_id != 0)
                {
                  /* Found exact vpls + vlan_id entry */
                  if (vc_info->vlan_id == vlan_id)
                    {
                      if (vpls)
                        nsm_vpls_interface_delete (vpls, ifp, vc_info);

                      nsm_mpls_vc_info_del (mif, mif->vpls_info_list, vc_info);
                      nsm_mpls_vc_info_free (vc_info);
                      found = 1;
                      break;
                    }
                }
              /* non-vlan case */
              else
                {
                 if (vpls)
                    nsm_vpls_interface_delete (vpls, ifp, vc_info);

                  nsm_mpls_vc_info_del (mif, mif->vpls_info_list, vc_info);
                  nsm_mpls_vc_info_free (vc_info);
                  found = 1;
                  break;
                }
            }
        }

      if (found == 0)
        return NSM_ERR_INVALID_VPLS;
    }
  /* Non-vlan case, only one vc_info should be in mif->vpls_info_list. */
  else
    {
      if ((ln = LISTHEAD (mif->vpls_info_list)))
        vc_info = ln->data;

      if (vc_info)
        {
          if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
              (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS)))
            return NSM_ERR_INVALID_VPLS;

          vpls = vc_info->u.vpls;
         if (vpls && vpls->ifmtu == vc_info->mif->ifp->mtu)
           flag = 1;
         if (vpls)
           nsm_vpls_interface_delete (vpls, ifp, vc_info);

          nsm_mpls_vc_info_del (mif, mif->vpls_info_list, vc_info);
          nsm_mpls_vc_info_free (vc_info);
        }
    }

  if (vpls && flag == 1)
    {
      LIST_LOOP (vpls->vpls_info_list, vc_info, ln)
        {
          if (! CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU))
            {
              if (temp == 0)
                temp = vc_info->mif->ifp->mtu;
              else if (vc_info->mif->ifp->mtu < temp)
                temp = vc_info->mif->ifp->mtu;
            }
          vpls->ifmtu = temp;
        }
      /*if only one interface is used and unbinded, 
       * it is already deleted form the vpls->vpls_info_list
       * then default value is assigned 
       */
      if (temp == 0)
        vpls->ifmtu = IF_ETHER_DEFAULT_MTU;
    }


  if (LISTCOUNT (mif->vpls_info_list) == 0)
    {
      if (vlan_id == 0)
        UNSET_FLAG (ifp->bind, NSM_IF_BIND_VPLS);
      else
        UNSET_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN);
    }

#ifdef HAVE_VLAN
  if (vlan_id != 0)
    ret = nsm_is_vlan_exist (ifp, vlan_id);
#endif /* HAVE_VLAN */

  if (ret == NSM_SUCCESS)
    nsm_vpls_if_delete_send (ifp);

  return NSM_SUCCESS;
}

int
nsm_if_vpls_bind_cli (struct cli *cli, char *sz_name, struct interface *ifp,
                      u_int16_t vlan_id)
{
  int ret = NSM_SUCCESS;

#ifdef HAVE_L2
  if (ifp->type != IF_TYPE_L2)
    {
      cli_out (cli, "%% Vpls need bind to Switchport\n");
      return CLI_ERROR;
    }
#endif /* HAVE_L2 */

#ifdef HAVE_VLAN
  /* This interface need binded to a bridge.
     If vlan_port mode is ACCESS, only vlan-id 0 is allowed. */
  if (vlan_id != 0)
    ret = nsm_vlan_id_validate_by_interface (ifp, vlan_id);
#endif /* HAVE_VLAN */

  if (ret == NSM_SUCCESS)
    ret = nsm_if_vpls_bind (sz_name, ifp, vlan_id);

  if (ret != NSM_SUCCESS)
    {
      switch (ret)
        {
        case NSM_ERR_INVALID_INTERFACE:
          cli_out (cli, "%% Loopback interface %s cannot be bound to VPLS "
                   "instance\n", ifp->name);
          break;
        case NSM_ERR_IF_BINDING_EXISTS:
          cli_out (cli, "%% Interface %s already bound to Virtual Circuit, "
                   "VRF or VPLS instance\n", ifp->name);
          break;
        case NSM_ERR_IF_BIND_VLAN_ERR:
          cli_out (cli, "%% Operation not allowed. Interface need bind to "
                        "a bridage and if switchport mode is ACCESS, only none "
                        "vlan type can be accept. \n");
          break;
        case NSM_VPLS_MTU_ERR:
          cli_out (cli, "%% Interface MTU should be greater or equal to "
                        " VPLS configure MTU\n");
          break;  
        case NSM_ERR_INTERNAL:
        default:
          cli_out (cli, "%% Failed to bind interface to vpls\n");
          break;
        }

      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


int
nsm_if_vpls_unbind_cli (struct cli *cli, char *sz_name, struct interface *ifp,
                        u_int16_t vlan_id)
{
  int ret;

  ret = nsm_if_vpls_unbind (sz_name, ifp, vlan_id);
  if (ret != NSM_SUCCESS)
    {
      cli_out (cli, "%% Failed to unbind interface from vpls\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

#ifdef HAVE_VLAN
CLI (if_vpls_bind,
     if_vpls_bind_cmd,
     "mpls-vpls NAME (vlan <2-4094>|)",
     "Bind interface to VPLS",
     "Identifying string for VPLS instance",
     "Ethernet VLAN",
     "Vlan Identifier")

{
  struct interface *ifp;
  u_int16_t vlan_id = 0;

  /* Get the vid and check if it is within range (2-4094). */
  if (argc > 2)
    CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[2], 2,
                           4094);

  ifp = cli->index;

  return nsm_if_vpls_bind_cli (cli, argv[0], ifp, vlan_id);
}
#else /* HAVE_VLAN */
CLI (if_vpls_bind,
     if_vpls_bind_cmd,
     "mpls-vpls NAME",
     "Bind interface to VPLS",
     "Identifying string for VPLS instance")

{
  struct interface *ifp;
  u_int16_t vlan_id = 0;

  ifp = cli->index;

  return nsm_if_vpls_bind_cli (cli, argv[0], ifp, vlan_id);
}
#endif /* HAVE_VLAN */

#ifdef HAVE_VLAN
CLI (if_vpls_unbind,
     if_vpls_name_unbind_cmd,
     "no mpls-vpls NAME (vlan <2-4094>|)",
     CLI_NO_STR,
     "Unbind interface from VPLS",
     "Identifying string for VPLS instance",
     "Ethernet VLAN",
     "Vlan Identifier")
{
  struct interface *ifp = cli->index;
  u_int16_t vlan_id = 0;

  if (argc == 0)
    if ((CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS_VLAN)) ||
        (CHECK_FLAG (ifp->bind, NSM_IF_BIND_MPLS_VC_VLAN)))
      {
        cli_out (cli, "%% vpls-name and vlan-id needed.\n");
        return CLI_ERROR;
      }

  if (argc == 3)
    {
      /* Get the vid and check if it is within range (2-4094). */
      CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[2], 2,
                             4094);
    }

  return nsm_if_vpls_unbind_cli (cli, argv[0], ifp, vlan_id);
}
#else /* HAVE_VLAN */

CLI (if_vpls_unbind,
     if_vpls_name_unbind_cmd,
     "no mpls-vpls NAME",
     CLI_NO_STR,
     "Unbind interface from VPLS",
     "Identifying string for VPLS instance")
{
  struct interface *ifp = cli->index;
  u_int16_t vlan_id = 0;
  char *sz_name = NULL;

  if (argc == 1)
    sz_name = argv[0];

  return nsm_if_vpls_unbind_cli (cli, argv[0], ifp, vlan_id);
}
#endif /* HAVE_VLAN */

ALI (if_vpls_unbind,
     if_vpls_unbind_cmd,
     "no mpls-vpls",
     CLI_NO_STR,
     "Unbind interface from VPLS");
#endif /* HAVE_VPLS */

int
nsm_if_config_write_ifc (struct cli *cli, struct connected *ifc)
{
  struct prefix *p;
  int write = 0;

  if (!CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
    if ((p = ifc->address))
      {
        cli_out (cli, " ip%s address %O", p->family == AF_INET ? "" : "v6", p);

        if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
          cli_out (cli, " secondary");

        if (CHECK_FLAG (ifc->flags, NSM_IFA_ANYCAST))
          cli_out (cli, " anycast");

        cli_out (cli, "\n");
        write++;
      }

  return write;
}
#ifdef HAVE_PBR
int
nsm_ip_policy_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct route_map_if *rmap_if = NULL;
  struct listnode *rn, *rn1 = NULL;
  struct nsm_master *nm = NULL;
  char *name = NULL;

  nm = cli->vr->proto;
  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    LIST_LOOP (rmap_if->ifnames, name, rn1)
      {
        if (pal_strcmp (name, ifp->name) == 0)
          cli_out (cli, " ip policy route-map %s\n", rmap_if->route_map_name);
      }

   return 0;
}
#endif
int
nsm_if_config_write_mpls (struct cli *cli, struct interface *ifp)
{
  char buf[BUFSIZ]; /* XXX */
#if defined (HAVE_TE) || defined (HAVE_DSTE)
  struct nsm_master *nm = cli->vr->proto;
#endif /* HAVE_TE || HAVE_DSTE */

#ifdef HAVE_MPLS
  IF_NSM_CAP_HAVE_MPLS
    {
      struct nsm_mpls_if *mif;

#ifdef HAVE_GMPLS
      if (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
        {
#endif
          mif = nsm_mpls_if_lookup (ifp);

          /* Set label space value */
          if (mif && mif->nls)
            {
              if (mif->nls->label_space == PLATFORM_WIDE_LABEL_SPACE)
                cli_out (cli, " label-switching\n");
              else
                cli_out (cli, " label-switching %d\n",
                         mif->nls->label_space);
            }
#ifdef HAVE_GMPLS
        }
#endif /* HAVE_GMPLS */
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_MPLS_VC
  {
    struct nsm_mpls_if *mif;
    struct nsm_mpls_vc_info *vc_info, *s_info = NULL;
    struct listnode *ln;

    /* Get corresponding MPLS Interface. */
    mif = nsm_mpls_if_lookup (ifp);
    if (mif)
      {
        LIST_LOOP (mif->vc_info_list, vc_info, ln)
          {
            if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN))
                && (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)))
              continue;

#ifdef HAVE_MS_PW
            if ((vc_info->u.vc) && (vc_info->u.vc->ms_pw))
              continue;
#endif /* HAVE_MS_PW */

            if ((s_info = vc_info->sibling_info))
              {
                if (CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_WROTE))
                  {
                    UNSET_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_WROTE);
                    continue;
                  }
              }

            cli_out (cli, " mpls-l2-circuit %s %s", vc_info->vc_name,
                     mpls_vc_type_to_str2 (vc_info->vc_type));
            if (vc_info->vc_type == VC_TYPE_ETH_VLAN)
              cli_out (cli, " %d", vc_info->vlan_id);
            if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))
              cli_out (cli, " %s", "secondary");

            cli_out (cli, "\n");

            if (s_info)
              {
                SET_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_WROTE);
                cli_out (cli, " mpls-l2-circuit %s %s", s_info->vc_name,
                         mpls_vc_type_to_str2 (s_info->vc_type));
                if (s_info->vc_type == VC_TYPE_ETH_VLAN)
                  cli_out (cli, " %d", s_info->vlan_id);
                if (CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))
                  cli_out (cli, " %s", "secondary");

                cli_out (cli, "\n");
              }

            if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY))
              {
                cli_out (cli, " vc-mode standby");
                if (vc_info->vlan_id != 0)
                  cli_out (cli, " vlan %d ", vc_info->vlan_id);

                cli_out (cli, "\n");
              }

            if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
              {
                cli_out (cli, " vc-mode revertive");
                if (vc_info->vlan_id != 0)
                  cli_out (cli, " vlan %d ", vc_info->vlan_id);

                cli_out (cli, "\n");
              }
          }
      }
  }
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
  {
    struct nsm_mpls_if *mif;
    struct nsm_mpls_vc_info *vc_info;
    struct listnode *ln;

    /* Get corresponding MPLS Interface. */
    mif = nsm_mpls_if_lookup (ifp);
    if (mif)
      {
        LIST_LOOP (mif->vpls_info_list, vc_info, ln)
          {
            if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
                (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS)))
              continue;

      if (CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS))
        cli_out (cli, " mpls-vpls %s\n", vc_info->vc_name);
      else if (CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN))
        cli_out (cli, " mpls-vpls %s vlan %d\n",
           vc_info->vc_name, vc_info->vlan_id);
        }
    }
  }
#endif /* HAVE_VPLS */

  /* Assign bandwidth here to avoid unnecessary interface flap
     while processing config script */
  if (CHECK_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED))
    cli_out (cli, " bandwidth %s\n",
             bandwidth_float_to_string (buf, ifp->bandwidth));

#ifdef HAVE_TE
  /* Maximum reservable bandwidth. */
  {
    struct qos_interface *qif = nsm_qos_serv_if_get (nm, ifp->ifindex);
    if (qif && CHECK_FLAG (ifp->conf_flags, NSM_MAX_RESV_BW_CONFIGURED))
      cli_out (cli, " reservable-bandwidth %s\n",
               bandwidth_float_to_string (buf, ifp->max_resv_bw));
  }
#ifdef HAVE_DSTE
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  /* Bandwidth Constraint. */
  {
    u_char i;
    struct qos_interface *qif = nsm_qos_serv_if_get (nm, ifp->ifindex);

    if (qif)
      for (i = 0; i < MAX_CLASS_TYPE; i++)
        if (NSM_MPLS->ct_name[i][0] != '\0'
            && ifp->bw_constraint[i] != 0)
          cli_out (cli, " bandwidth-constraint %s %s\n",
                   NSM_MPLS->ct_name[i],
                   bandwidth_float_to_string (buf, ifp->bw_constraint[i]));
  }
#endif /*(!defined (HAVE_GMPLS) || defined (HAVE_PACKET))*/
#endif /* HAVE_DSTE */
#endif /* HAVE_TE */

#ifdef HAVE_DSTE
  if (ifp->bc_mode == BC_MODE_RSDL)
    cli_out (cli, " bc-mode RSDL\n");
  else if (ifp->bc_mode == BC_MODE_MAR)
    cli_out (cli, " bc-mode MAR\n");
#endif /* HAVE_DSTE */

  return 1;
}

/* Interface config-write. */
int
nsm_if_config_write (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  struct connected *ifc;
  struct nsm_if *zif;
  struct listnode *node;

#ifndef HAVE_MPLS
  char buf[BUFSIZ];
#endif /* HAVE_MPLS */

#ifdef HAVE_NSM_IF_ARBITER
  nsm_if_arbiter_config_write (cli);
#endif /* HAVE_NSM_IF_ARBITER */

  LIST_LOOP (nzg->ifg.if_list, ifp, node)
#ifdef HAVE_SUPPRESS_UNMAPPED_IF
    if (CHECK_FLAG (ifp->status, NSM_INTERFACE_MAPPED))
#endif /* HAVE_SUPPRESS_UNMAPPED_IF */
      if (nm->vr->id == 0 || nm->vr == ifp->vr)
        {
          if (CHECK_FLAG (ifp->status, IF_HIDDEN_FLAG))
            continue;

          zif = (struct nsm_if *)ifp->info;
          cli_out (cli, "interface %s\n", ifp->name);

          if (ifp->desc)
            cli_out (cli, " description %s\n", ifp->desc);

          if (CHECK_FLAG (ifp->status, IF_NON_LEARNING_FLAG))
            cli_out (cli, " no-learning\n");
         
#ifdef HAVE_SNMP
           if (pal_strlen (ifp->if_alias))
             cli_out (cli, " alias  %s \n", ifp->if_alias);
#endif /* HAVE_SNMP */

#ifdef HAVE_CUSTOM1
          if (ifp->hw_type == IF_TYPE_PORT)
            {
              swp_port_information (cli, ifp, ifp->pid);
              swp_vlan_information (cli, ifp, ifp->pid);
              swp_lag_information (cli, ifp, ifp->pid);
              swp_stpport_information (cli, ifp, ifp->pid);
              swp_egrsh_information (cli, ifp, ifp->pid);
              swp_rdd_information (cli, ifp, ifp->pid);
              continue;
            }
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_VR
          if (nm->vr != ifp->vr)
            {
              cli_out (cli, " virtual-router forwarding %s\n", ifp->vr->name);
              cli_out (cli, "!\n");
              continue;
            }
#endif /* HAVE_VR */

#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)
          if (nsm_ifma_is_logical_set(ifp->info))
            {
              char      buf[INTERFACE_HWADDR_MAX*4];
              u_int8_t *lma = nsm_ifma_get_logical(ifp->info, NULL, 0);
              if (lma)
                cli_out (cli, " mac-address %s\n",
                         nsm_ifma_to_str(lma, buf, sizeof(buf)));
            }
#endif /* HAVE_L3 && HAVE_INTERVLAN_ROUTING */

          if (ifp->hw_type == IF_TYPE_ETHERNET)
            {
              if (ifp->config_duplex != NSM_IF_DUPLEX_UNKNOWN)
                cli_out (cli, " duplex %s\n",
                         nsm_if_media_type_str(ifp->config_duplex));
            }

          /* Print MTU. */
          switch (ifp->hw_type)
            {
            case IF_TYPE_ETHERNET:
              if (ifp->mtu != IF_ETHER_DEFAULT_MTU
                  && ifp->mtu != 0
                  && NSM_INTF_TYPE_L3(ifp))
                cli_out (cli, " mtu %d\n", ifp->mtu);
              break;
            case IF_TYPE_PPP:
              if (ifp->mtu != IF_PPP_DEFAULT_MTU)
                cli_out (cli, " mtu %d\n", ifp->mtu);
              break;
            case IF_TYPE_LOOPBACK:
              break;
            case IF_TYPE_HDLC:
              if (ifp->mtu != IF_HDLC_DEFAULT_MTU)
                cli_out (cli, " mtu %d\n", ifp->mtu);
              break;
            case IF_TYPE_ATM:
              if (ifp->mtu != IF_ATM_DEFAULT_MTU)
                cli_out (cli, " mtu %d\n", ifp->mtu);
              break;
            }
#if 0
#ifdef HAVE_L2
          port_mirroring_write(cli,ifp->ifindex);
#endif /* HAVE_L2 */
#endif

#ifdef HAVE_QOS
          nsm_qos_if_config_write (cli, ifp);
#endif /* HAVE_QOS */

#ifdef HAVE_L3
          if (NSM_INTF_TYPE_L3(ifp))
            {
              if (ifp->arp_ageing_timeout != NSM_ARP_AGE_TIMEOUT_DEFAULT)
                cli_out (cli, " arp-ageing-timeout %d\n",
                         ifp->arp_ageing_timeout);
            }
          if ( (zif) && CHECK_FLAG( zif->flags, NSM_IF_SET_PROXY_ARP))
            cli_out (cli, " ip proxy-arp\n");
#ifdef HAVE_PBR
          nsm_ip_policy_if_config_write (cli, ifp);
#endif /* HAVE_PBR */
#endif /* HAVE_L3 */

#ifdef HAVE_VRF
          if (NSM_CAP_HAVE_VRF)
            if (ifp->vrf->id != 0 && ifp->vrf->name)
              cli_out (cli, " ip vrf forwarding %s\n", ifp->vrf->name);
#endif /* HAVE_VRF */

#ifdef HAVE_TUNNEL
          nsm_tunnel_if_config_write (cli, ifp);
#endif /* HAVE_TUNNEL */

#ifdef HAVE_GMPLS
          nsm_gmpls_if_config_write (cli, ifp);
#endif /* HAVE_GMPLS */

#ifdef HAVE_MCAST_IPV4
          nsm_mcast_if_config_write (cli, ifp);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
          nsm_mcast6_if_config_write (cli, ifp);
#endif /* HAVE_MCAST_IPV6 */

          /* IPv4 addresses.  */
#ifdef HAVE_NSM_IF_UNNUMBERED
          if (if_is_ipv4_unnumbered (ifp))
            nsm_if_ipv4_unnumbered_config_write (cli, ifp);
          else
#endif /* HAVE_NSM_IF_UNNUMBERD */
            if (ifp->ifc_ipv4)
              {
                /* Show primary first.  */
                for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
                  if (!CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
                    nsm_if_config_write_ifc (cli, ifc);

                for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
                  if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
                    nsm_if_config_write_ifc (cli, ifc);
              }

#ifdef HAVE_IPV6
          /* IPv6 addresses.  */
#ifdef HAVE_NSM_IF_UNNUMBERED
          if (if_is_ipv6_unnumbered (ifp))
            nsm_if_ipv6_unnumbered_config_write (cli, ifp);
          else
#endif /* HAVE_NSM_IF_UNNUMBERED */
            if (ifp->ifc_ipv6)
              {
                /* Show link-local first. */
                for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
                  if (IN6_IS_ADDR_LINKLOCAL (&ifc->address->u.prefix6))
                    nsm_if_config_write_ifc (cli, ifc);

                for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
                  if (!IN6_IS_ADDR_LINKLOCAL (&ifc->address->u.prefix6))
                    nsm_if_config_write_ifc (cli, ifc);
              }
#endif /* HAVE_IPV6 */

          if (if_is_up(ifp))
            cli_out (cli, " no shutdown\n");
          else
            cli_out (cli, " shutdown\n");
 
#ifdef HAVE_L2
          nsm_bridge_if_config_write (cli, ifp);
          port_mirroring_write(cli,ifp->ifindex);
#endif /* HAVE_L2 */

#ifdef HAVE_LACPD
          nsm_static_aggregator_if_config_write (cli, ifp);
          nsm_lacp_if_config_write (cli, ifp);
#endif /*HAVE_LACPD*/

#ifdef HAVE_MPLS
          /* Print MPLS related configs. */
          if (nm->nmpls != NULL)
            nsm_if_config_write_mpls (cli, ifp);
#else
          if (CHECK_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED))
            cli_out (cli, " bandwidth %s\n",
                     bandwidth_float_to_string (buf, ifp->bandwidth));
#endif /* HAVE_MPLS */

#ifdef HAVE_TUNNEL
          /* For tunnel, def mcast status is disabled unlike other 
           * interfaces which have def mcast enabled. So for a tunnel  
           * iintf, we will write enabled status alone to conf file */
          if (if_is_tunnel (ifp))
            {
              if (if_is_multicast (ifp))
                cli_out (cli, " multicast\n");
            }
          else
#endif
          if (! if_is_multicast (ifp)
              && ! if_is_loopback (ifp))
            cli_out (cli, " no multicast\n");
            
#ifdef HAVE_RTADV
          if (NSM_CAP_HAVE_IPV6)
            nsm_rtadv_if_config_write (cli, ifp);
#endif /* HAVE_RTADV */

#if defined (HAVE_MCAST_IPV4) || defined (HAVE_IGMP_SNOOP)
#ifndef HAVE_IMI
          igmp_config_write_interface (cli, ifp);
#endif /* !HAVE_IMI */
#endif /* (HAVE_MCAST_IPV4) || defined (HAVE_IGMP_SNOOP) */

#ifdef HAVE_IPV6
#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
#ifndef HAVE_IMI
          mld_config_write_interface (cli, ifp);
#endif /* !HAVE_IMI */
#endif /* (HAVE_MCAST_IPV6) || defined (HAVE_MLD_SNOOP) */
#endif /* HAVE_IPV6 */

#ifdef HAVE_TE
          if (NSM_MPLS && ! ADMIN_GROUP_INVALID (ifp->admin_group))
            {
              s_int32_t i;
              char *name;

              for (i = 0; i < ADMIN_GROUP_MAX; i++)
                {
                  if (CHECK_FLAG (ifp->admin_group, (1 << i)))
                    {
                      name =
                        admin_group_get_name (NSM_MPLS->admin_group_array, i);
                      if (name != NULL)
                        cli_out (cli, " admin-group %s\n", name);
                    }
                }
            }
#endif /* HAVE_TE */
#ifdef HAVE_IPSEC
          nsm_ipsec_crypto_if_config_write (cli, ifp);
#endif /* HAVE_IPSEC */

#ifdef HAVE_FIREWALL
          nsm_firewall_if_config_write (cli, ifp);
#endif /* HAVE_FIREWALL */

#ifdef HAVE_BFD
          if ( (zif) && CHECK_FLAG( zif->flags, NSM_IF_BFD))
            cli_out (cli, " ip static bfd\n");
          if ( (zif) && CHECK_FLAG( zif->flags, NSM_IF_BFD_DISABLE))
            cli_out (cli, " ip static bfd disable\n");
#endif

#ifdef HAVE_L2LERN
          if (zif->mac_acl)
            cli_out (cli, " mac access-group %s in\n", zif->mac_acl->name);
#endif
          if (zif)
            {
              if (zif->acl_name_str && zif->acl_dir_str)
                cli_out (cli, " ip-access-group %s %s\n", zif->acl_name_str,
                         zif->acl_dir_str);
            }
          cli_out (cli, "!\n");
        }

  return 0;
}

/* Allocate and initialize interface vector. */
void
nsm_if_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* Non-VR installs nsm_if_config_write for INTERFACE_NODE write
     function.  VR installs a different function from
     nsm_vrcli_init. */
  cli_install_config (ctree, INTERFACE_MODE, nsm_if_config_write);

  /* Only Global administrators are allowed to delete interfaces for VR. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_interface_cmd);

  /* IPv4 show ip interface brief commands. */

#ifdef HAVE_L3
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_interface_brief_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_interface_if_brief_cmd);
#ifdef HAVE_PBR
  /* "ip policy" CLI */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_policy_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_policy_cmd);

  /* show ip policy */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_policy_cmd);
#endif /* HAVE_PBR */
#endif /* HAVE_L3 */

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      /* IPv6 show ipv6 interface brief commands. */
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_interface_brief_cmd);
      cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                       &show_ipv6_interface_if_brief_cmd);
    }
#endif /* HAVE_IPV6 */

#ifndef HAVE_CUSTOM1
  /* Install interface command. */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_interface_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_interface_cmd);
#endif /* HAVE_CUSTOM1 */

  cli_install_default (ctree, INTERFACE_MODE);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &interface_desc_cli);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_interface_desc_cli);

#ifdef HAVE_SNMP
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_interface_alias_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_interface_alias_cmd);
#endif /* HAVE_SNMP */
 
#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &logical_hw_addr_set_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_logical_hw_addr_set_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_logical_hw_addr_mac_set_cmd);
#endif /* HAVE_L3 && HAVE_INTERVLAN_ROUTING */


#ifndef IF_BANDWIDTH_INFO
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &bandwidth_if_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_bandwidth_if_cmd);
#endif /* !IF_BANDWIDTH_INFO */
#ifdef HAVE_TE
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &max_resv_bw_if_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_max_resv_bw_if_cmd);
#endif /* HAVE_TE */
#ifdef HAVE_L3
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_address_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_address_cmd);

#ifdef HAVE_LABEL
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_address_label_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_address_secondary_label_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_address_label_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_address_secondary_label_cmd);
#endif /* HAVE_LABEL */

#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                       &ipv6_address_cmd);
      cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ipv6_address_cmd);
      cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                       &ipv6_address_anycast_cmd);
    }
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

#ifdef HAVE_VRF
  if (NSM_CAP_HAVE_VRF)
    {
      cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                       &ip_vrf_forwarding_cmd);
      cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                       &no_ip_vrf_forwarding_cmd);
    }
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS
  if (NSM_CAP_HAVE_MPLS)
    {
      /* Label pool specific commands */
      cli_install (ctree, INTERFACE_MODE, &if_label_switching_cmd);
      cli_install (ctree, INTERFACE_MODE, &if_label_switching_val_cmd);
      cli_install (ctree, INTERFACE_MODE, &if_no_label_switching_cmd);
      cli_install (ctree, INTERFACE_MODE, &if_no_label_switching_val_cmd);
    }
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
  cli_install (ctree, INTERFACE_MODE, &if_admin_group_cmd);
  cli_install (ctree, INTERFACE_MODE, &if_no_admin_group_cmd);
#endif /* HAVE_TE */

#ifdef HAVE_MPLS_VC
  cli_install (ctree, INTERFACE_MODE, &if_mpls_l2_circuit_cmd);
  cli_install (ctree, INTERFACE_MODE, &if_no_mpls_l2_circuit_cmd);
  cli_install (ctree, INTERFACE_MODE, &if_mpls_l2_circuit_runtime_mode_cmd);
  cli_install (ctree, INTERFACE_MODE, &if_no_mpls_l2_circuit_runtime_mode_cmd);
  cli_install (ctree, INTERFACE_MODE, &if_mpls_l2_circuit_revertive_mode_cmd);
  cli_install (ctree, INTERFACE_MODE,
               &if_no_mpls_l2_circuit_revertive_mode_cmd);
#ifdef HAVE_VLAN
  cli_install (ctree, INTERFACE_MODE, &if_no_mpls_l2_circuit_type_cmd);
#endif /* HAVE_VLAN */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
  cli_install (ctree, INTERFACE_MODE, &if_vpls_bind_cmd);
  cli_install (ctree, INTERFACE_MODE, &if_vpls_name_unbind_cmd);
  cli_install (ctree, INTERFACE_MODE, &if_vpls_unbind_cmd);
#endif /* HAVE_VPLS */

#ifdef HAVE_DSTE
  cli_install (ctree, INTERFACE_MODE, &if_bc_mode_cmd);
  cli_install (ctree, INTERFACE_MODE, &bw_constraint_if_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_bw_constraint_if_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_bw_constraint_if_val_cmd);
#endif /* HAVE_DSTE */

#ifdef HAVE_CUSTOM1
  nsm_if_custom1_init (ctree);
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_SMI
#ifdef HAVE_L2
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_interface_no_learning_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_interface_no_learning_cmd);
#endif  /* HAVE_L2 */
#endif /* HAVE_SMI */
}

int
nsm_if_stat_update (struct apn_vr *vr)
{
   struct apn_vrf *vrf;
   int fib_id;

   if (vr == NULL)
     return -1;

   /* Loop for all VRF's to get the FIB-ID */
   for (vrf = vr->vrf_list; vrf; vrf = vrf->next)
     {
       fib_id = vrf->fib_id;
       pal_if_stat_update (&fib_id);
     }

   return 0;
}
#ifdef HAVE_PBR
void
nsm_pbr_ip_policy_display (struct cli *cli)
{
  struct route_map_if *rmap_if = NULL;
  char *name = NULL;
  struct listnode *rn = NULL, *rn1 = NULL;
  struct nsm_master *nm = cli->vr->proto;

  cli_out (cli, "        Interface        Route map            \n");
  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    LIST_LOOP (rmap_if->ifnames, name, rn1)
    {
      cli_out (cli, "        %s               %s                   \n",
               name, rmap_if->route_map_name);
    }
}
#endif /* HAVE_PBR */
