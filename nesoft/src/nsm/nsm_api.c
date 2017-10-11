/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "if.h"
#include "prefix.h"
#include "nexthop.h"
#include "thread.h"
#include "version.h"
#include "nsm_message.h"

#include "nsmd.h"
#include "nsm_vrf.h"
#ifdef HAVE_L3
#include "nsm/rib/rib.h"
#include "nsm/rib/nsm_table.h"
#endif /* HAVE_L3 */
#include "nsm_router.h"
#include "nsm_interface.h"
#include "nsm_server.h"
#include "nsm_api.h"
#include "nsm_fea.h"
#include "nsm_snmp.h"
#ifdef HAVE_TE
#include "nsm_qos_serv.h"
#endif /* HAVE_TE */
#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /*HAVE_DSTE */
#ifdef HAVE_VRF
#include "nsm_rib_vrf.h"
#endif /* HAVE_VRF */
#ifdef HAVE_GMPLS
#include "nsm_gmpls_if.h"
#include "nsm_gmpls_ifapi.h"
#include "nsm_mpls_rib.h"
#endif /*HAVE_GMPLS*/
#ifdef HAVE_LACPD
#include "nsm_lacp.h"
#endif /* HAVE_LACPD */

#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifdef HAVE_L2
#include "nsm_bridge.h"
#endif /* HAVE_L2 */

#include "ptree.h"

#ifdef HAVE_HA
#include "lib_cal.h"
#include "nsm_cal.h"
#endif /* HAVE_HA */

extern int hal_get_master_cpu_entry (unsigned char *cpu_entry);
extern int hal_get_local_cpu_entry (unsigned char *cpu_entry);
extern int hal_stacking_set_entry_master (unsigned char *mac);
extern int hal_get_num_cpu_entries(unsigned int *num_cpu);
extern int hal_get_index_cpu_entry (unsigned int entry_num, unsigned char *cpu_entry);
extern int hal_get_index_dump_cpu_entry (unsigned int entry_num, 
    unsigned char *cpu_entry);

#ifdef HAVE_HAL
int
nsm_set_acl_for_access_group (struct access_list *access,
                              struct hal_ip_access_grp *access_grp);
#endif /* HAVE_HAL */

/* CLI-APIs.  */
int
nsm_if_flag_up_set (u_int32_t vr_id, char *ifname, bool_t iterate_members)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;
  struct nsm_if *nif;
#ifdef HAVE_LACPD
  struct interface *memifp;
  struct listnode *lsn;
#endif /* HAVE LACP */

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Get NSM interface.  */
  nif = ifp->info;

#ifdef HAVE_LACPD
  if (nif && (nif->agg.type == NSM_IF_AGGREGATOR))
    {
      if (listcount(nif->agg.info.memberlist) == 0)
        return NSM_API_SET_ERR_EMPTY_AGGREGATOR_DOWN;
      else
        {
          if (iterate_members)
            {
              AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
                {
                  nsm_if_flag_up_set (vr_id, memifp->name, PAL_FALSE);
                }
            }
        }
    }
#endif /* HAVE_LACPD */

  ret = nsm_fea_if_flags_set (ifp, IFF_UP);
  if (ret < 0)
    return NSM_API_SET_ERR_FLAG_UP_CANT_SET;

  nsm_if_refresh (ifp);

#ifdef HAVE_CUSTOM1
  if (!CHECK_FLAG (ifp->flags, IFF_UP))
#else /* HAVE_CUSTOM1 */
  if (!if_is_up (ifp))
#endif /* HAVE_CUSTOM1 */
    return NSM_API_SET_ERR_FLAG_UP_CANT_SET;

#ifdef HAVE_L3
  /* Wakeup configured addresses. */
  if (if_is_up (ifp))
    nsm_if_addr_wakeup (ifp);

  /* Update Router ID. */
  ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);
#endif /* HAVE_L3 */

  /* update the time for last status change */
  ifp->ifLastChange = pal_time_current (NULL);

#ifdef HAVE_LACPD
  /* Send aggregator member state to static aggregator to denote no shutdown */
  nsm_update_aggregator_member_state (nif);
#endif /* HAVE_LACPD */

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_flag_up_unset (u_int32_t vr_id, char *ifname, bool_t iterate_members)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;
  struct nsm_if *nif;
#ifdef HAVE_LACPD
  struct interface *memifp;
  struct listnode *lsn;
#endif /* HAVE LACP */

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Get NSM interface.  */
  nif = ifp->info;

#ifdef HAVE_LACPD
 if (NSM_INTF_TYPE_AGGREGATOR(ifp)) 
   {
     if (iterate_members)
       {
         AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
           {
             nsm_if_flag_up_unset (vr_id, memifp->name, PAL_FALSE);
           }
       }
   }
#endif /* HAVE LACP */
#ifdef HAVE_NSM_IF_PARAMS
   if (ifp->ifindex == 0)
     {
 #ifdef HAVE_LACPD
       if (NSM_INTF_TYPE_AGGREGATOR(ifp))
         {
           if (iterate_members)
             nsm_if_shutdown_pending_set (nm, ifname);
         }
       else
 #endif /* HAVE LACP */
         nsm_if_shutdown_pending_set (nm, ifname);
     }
#endif /* HAVE_NSM_IF_PARAMS */

  ret = nsm_fea_if_flags_unset (ifp, IFF_UP);
  if (ret < 0)
    return NSM_API_SET_ERR_FLAG_UP_CANT_UNSET;

  nsm_if_refresh (ifp);

  if (if_is_up (ifp))
    return NSM_API_SET_ERR_FLAG_UP_CANT_UNSET;

#ifdef HAVE_L3
  /* Update Router ID. */
  ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);
#endif /* HAVE_L3 */

  /* update the time for last status change */
  ifp->ifLastChange = pal_time_current (NULL);

#ifdef HAVE_LACPD
  /* Send aggregator member state to static aggregator to denote the shutdown*/
  nsm_update_aggregator_member_state (nif);
#endif

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_flag_multicast_set (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (if_is_multicast (ifp))
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_if_flags_set (ifp, IFF_MULTICAST);
  if (ret < 0)
    return NSM_API_SET_ERR_FLAG_MULTICAST_CANT_SET;

  /* Get current status.  */
  nsm_fea_if_flags_get (ifp);

  if (!if_is_multicast (ifp))
    return NSM_API_SET_ERR_FLAG_MULTICAST_CANT_SET;

  /* Set the interface flag ctype in the cindex.  */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_FLAGS);

  /* Announce to the PMs.  */
  nsm_server_if_update (ifp, cindex);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_flag_multicast_unset (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (!if_is_multicast (ifp))
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_if_flags_unset (ifp, IFF_MULTICAST);
  if (ret < 0)
    return NSM_API_SET_ERR_FLAG_MULTICAST_CANT_UNSET;

  /* Get current status.  */
  nsm_fea_if_flags_get (ifp);

  if (if_is_multicast (ifp))
    return NSM_API_SET_ERR_FLAG_MULTICAST_CANT_UNSET;

  /* Set the interface flag ctype in the cindex.  */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_FLAGS);

  /* Announce to the PMs.  */
  nsm_server_if_update (ifp, cindex);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

/* Get multicast setting on an interface */
int
nsm_if_flag_multicast_get (u_int32_t vr_id, char *ifname, int *mcast)
{
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  nsm_fea_if_flags_get (ifp);

  if (if_is_multicast (ifp))
    *mcast = NSM_IF_MCAST_IS_SET;
  else
    *mcast = NSM_IF_MCAST_IS_CLR;

  return NSM_API_GET_SUCCESS;
}

int
nsm_if_mtu_set (u_int32_t vr_id, char *ifname, int mtu)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;
#ifdef HAVE_LACPD
  struct nsm_if *nif;
  struct interface *memifp;
  struct listnode *ln;
#endif /* HAVE_LACPD */

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

#ifdef HAVE_LACPD
  nif = ifp->info;
#endif /* HAVE_LACPD */

  if (ifp->mtu == mtu)
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_if_set_mtu (ifp, mtu);
  if (ret < 0)
    return NSM_API_SET_ERR_INVALID_MTU_SIZE;
  else
    ifp->mtu = mtu;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_MTU);

  /* Send update to PMs.  */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

#ifdef HAVE_LACPD
  /* Set mtu for members too */
  if (nif &&
      nif->agg.type == NSM_IF_AGGREGATOR &&
      (listcount(nif->agg.info.memberlist) > 0))
    LIST_LOOP (nif->agg.info.memberlist, memifp, ln)
     {
       if (memifp->mtu != mtu)
         {
           memifp->agg_param_update = 1;
           nsm_if_mtu_set (memifp->vr->id, memifp->name, mtu);
           memifp->agg_param_update = 0;
         }
     }
#endif /* HAVE_LACPD */

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_mtu_unset (u_int32_t vr_id, char *ifname)
{
  int ret;
  int mtu;
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;
#ifdef HAVE_LACPD
  struct nsm_if *nif;
  struct interface *memifp;
  struct listnode *ln;
#endif /* HAVE_LACPD */

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

#ifdef HAVE_LACPD
  nif = ifp->info;
#endif /* HAVE_LACPD */

  if (ifp->hw_type == IF_TYPE_ETHERNET)
    mtu = IF_ETHER_DEFAULT_MTU;
  else if (ifp->hw_type == IF_TYPE_PPP)
    mtu = IF_PPP_DEFAULT_MTU;
  else if (ifp->hw_type == IF_TYPE_LOOPBACK)
    mtu = IF_LO_DEFAULT_MTU;
  else if (ifp->hw_type == IF_TYPE_HDLC)
    mtu = IF_HDLC_DEFAULT_MTU;
  else if (ifp->hw_type == IF_TYPE_ATM)
    mtu = IF_ATM_DEFAULT_MTU;
  else
    return NSM_API_SET_ERR_INVALID_IF_TYPE;

  if (ifp->mtu == mtu)
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_if_set_mtu (ifp, mtu);
  if (ret < 0)
    return NSM_API_SET_ERR_INVALID_MTU_SIZE;
  else
    ifp->mtu = mtu;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_MTU);

  /* Send update to PMs.  */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

#ifdef HAVE_LACPD
  /* Set mtu for members too */
  if (nif &&
      nif->agg.type == NSM_IF_AGGREGATOR &&
      (listcount(nif->agg.info.memberlist) > 0))
    LIST_LOOP (nif->agg.info.memberlist, memifp, ln)
     {
       if (memifp->mtu != mtu)
         {
           memifp->agg_param_update = 1;
           nsm_if_mtu_set (memifp->vr->id, memifp->name, mtu);
           memifp->agg_param_update = 0;
         }
     }
#endif /* HAVE_LACPD */

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

/* Validate the MAC address */
int
nsm_api_mac_address_is_valid (char *mac_addr)
{
  char *p = mac_addr;
  int i = 0;

  while (*p)
    {
      if ((*p) == '.')
        {
          i++;
          if (i > 2)
            {
              return NSM_FALSE;
            }
          p++;
          continue;
        }

      if (((*p) >= '0' && (*p) <= '9') ||
          ((*p) >= 'a' && (*p) <= 'f') ||
          ((*p) >= 'A' && (*p) <= 'F'))
        {
          p++;
          continue;
        }
      else
        {
          return NSM_FALSE;
        }
    }

  return NSM_TRUE;
}

#ifdef HAVE_L3
int
nsm_delete_static_arp (struct nsm_master *nm, struct connected *ifc, struct interface *ifp)
{
  struct nsm_static_arp_entry *arp_entry = NULL;
  struct nsm_arp_master *arp_master      = NULL;
  struct ptree_node *pn                  = NULL;
  arp_master = nm->arp;

  for (pn = ptree_top (arp_master->nsm_static_arp_list); pn; pn = ptree_next (pn))
    {
      if ((arp_entry = pn->info))
        if (ifc == arp_entry->ifc)
          {
            nsm_api_arp_entry_del (nm,&arp_entry->addr,
                                   arp_entry->mac_addr, ifp);
          }
    }

    return 0;
}
int
nsm_if_arp_ageing_timeout_set (u_int32_t vr_id, char *ifname, int arp_ageing_timeout)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (ifp->arp_ageing_timeout == arp_ageing_timeout)
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_if_set_arp_ageing_timeout (ifp, arp_ageing_timeout);
  if (ret < 0)
    return NSM_API_SET_ERR_ARP_AGEING_TIMEOUT;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_ARP_AGEING_TIMEOUT);

  ifp->arp_ageing_timeout = arp_ageing_timeout;

  /* Send update to PMs.  */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_arp_ageing_timeout_unset (u_int32_t vr_id, char *ifname)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ret = nsm_fea_if_set_arp_ageing_timeout (ifp, 0);
  if (ret < 0)
    return NSM_API_SET_ERR_ARP_AGEING_TIMEOUT;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_ARP_AGEING_TIMEOUT);

  ifp->arp_ageing_timeout = NSM_ARP_AGE_TIMEOUT_DEFAULT;

  /* Send update to PMs.  */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}
#endif /* HAVE_L3 */

int
nsm_if_duplex_set (u_int32_t vr_id, char *ifname, int duplex)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;

#ifdef HAVE_LACPD
  struct interface *memifp = NULL;
  struct listnode *lsn = NULL;
  struct nsm_if *zif   = NULL;
#endif /* HAVE LACP */

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if ((ifp->duplex == duplex) && (ifp->config_duplex == duplex))
    {
      return 0;
    }

#ifdef HAVE_LACPD
   zif = (struct nsm_if *)ifp->info;
  /* Bandwidth Setting is allowed on Aggregator. */
  if (zif && (zif->agg.type == NSM_IF_AGGREGATOR))
    {
      ret = nsm_fea_if_set_duplex (ifp, duplex);

      if (ret >= 0)
        {
          AGGREGATOR_MEMBER_LOOP (zif, memifp, lsn)
           {
             memifp->duplex= duplex;
             nsm_fea_if_set_duplex (memifp, duplex);
           }
        }
    }
  else
#endif /* HAVE_LACPD */

  ret = nsm_fea_if_set_duplex (ifp, duplex);
  if (ret < 0)
    return NSM_API_SET_ERR_DUPLEX;
  else
    ifp->duplex = ifp->config_duplex = duplex;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_DUPLEX);

  /* Send update to PMs.  */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_duplex_unset (u_int32_t vr_id, char *ifname)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* If duplex was not previously configured do nothing */
  if (ifp->config_duplex == NSM_IF_DUPLEX_UNKNOWN) 
    return 0;

  /* Set auto-negotiation */
  ret = nsm_fea_if_set_duplex (ifp, NSM_IF_AUTO_NEGO);
  if (ret < 0)
    return NSM_API_SET_ERR_DUPLEX;
  /* Set auto negotiate enable 
   * and set the duplex to auto nego */
  if (ifp->duplex != NSM_IF_AUTO_NEGO)
    ifp->autonego = NSM_IF_AUTONEGO_ENABLE; 
  ifp->duplex = NSM_IF_AUTO_NEGO;

  ifp->config_duplex = NSM_IF_DUPLEX_UNKNOWN;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_DUPLEX);

  /* Send update to PMs.  */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

/* Get the Duplex status on an interface */
int
nsm_if_duplex_get (u_int32_t vr_id, char *ifname, int *duplex)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ret = nsm_fea_if_get_duplex (ifp);
  if (ret < 0)
    return NSM_API_GET_FAIL;
  else
    *duplex = ifp->duplex;

  return NSM_API_GET_SUCCESS;
}

#ifdef HAVE_TE
s_int32_t
nsm_api_if_max_bandwidth_set (u_int32_t vr_id, char *ifname, char *str)
{
  float bandwidth;
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ret = bandwidth_string_to_float (str, &bandwidth);
  if (ret < 0)
    return NSM_API_SET_ERR_INVALID_BW;

  /* Update max bandwidth */
  ret = nsm_qos_serv_update_max_bw (ifp, bandwidth);
  if (ret < 0)
    return NSM_API_SET_ERR_BW_IN_USE;

  return NSM_API_SET_SUCCESS;
}

s_int32_t
nsm_api_if_max_bandwidth_unset (u_int32_t vr_id, char *ifname)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  UNSET_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);

  /* Update max bandwidth */
  ret = nsm_qos_serv_update_max_bw (ifp, 0);
  if (ret < 0)
    return NSM_API_SET_ERR_BW_IN_USE;

  return NSM_API_SET_SUCCESS;
}

s_int32_t
nsm_api_if_reservable_bandwidth_set (u_int32_t vr_id, char *ifname, char *str)
{
  float bandwidth;
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ret = bandwidth_string_to_float (str, &bandwidth);
  if (ret < 0)
    return NSM_API_SET_ERR_INVALID_BW;

  /* Update max resv bw */
  ret = nsm_qos_serv_update_max_resv_bw (ifp, bandwidth, NSM_TRUE);
  if (ret < 0)
    {
      switch (ret)
        {
#ifdef HAVE_DSTE
          case NSM_ERR_INVALID_BW_CONSTRAINT :
            return NSM_API_SET_ERR_INVALID_BW_CONSTRAINT;
#endif /* HAVE_DSTE */
          default :
            return NSM_API_SET_ERR_BW_IN_USE;
        }
    }

  return NSM_API_SET_SUCCESS;
}

s_int32_t
nsm_api_if_reservable_bandwidth_unset (u_int32_t vr_id, char *ifname)
{
  int ret;
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Update max resv bw */
  ret = nsm_qos_serv_update_max_resv_bw (ifp, 0, NSM_TRUE);
  if (ret < 0)
    {
      switch (ret)
        {
#ifdef HAVE_DSTE
          case NSM_ERR_INVALID_BW_CONSTRAINT :
            return NSM_API_SET_ERR_INVALID_BW_CONSTRAINT;
#endif /* HAVE_DSTE */
          default :
            return NSM_API_SET_ERR_BW_IN_USE;
        }
    }

  return NSM_API_SET_SUCCESS;
}

#ifdef HAVE_DSTE
s_int32_t
nsm_api_if_bandwidth_constraint_set (u_int32_t vr_id, char *ifname, 
                                     char *ct_str, char *bw_str)
{
  float bandwidth;
  int ret, ct_num;
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ct_num = nsm_dste_get_class_type_num (nm, ct_str);

  if (ct_num == CT_NUM_INVALID)
    return NSM_API_SET_ERR_INVALID_CLASS_TYPE;

  ret = bandwidth_string_to_float (bw_str, &bandwidth);
  if (ret < 0)
    return NSM_API_SET_ERR_INVALID_BW;

  /* Update BW Constraint */
  ret = nsm_qos_serv_update_bw_constraint (ifp, bandwidth, ct_num);
  if (ret < 0)
    {
      switch (ret)
        {
          case NSM_ERR_INTERNAL:
            return NSM_API_SET_ERROR;
          case NSM_ERR_INVALID_BW_CONSTRAINT:
            return NSM_API_SET_ERR_INVALID_BW_CONSTRAINT;
          default:
            return NSM_API_SET_ERROR;
        }
    }

  return NSM_API_SET_SUCCESS;
}

s_int32_t
nsm_api_if_bandwidth_constraint_unset (u_int32_t vr_id, char *ifname, 
                                       char *ct_str)
{
  int ret, ct_num;
  struct nsm_master *nm;
  struct interface *ifp;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ct_num = nsm_dste_get_class_type_num (nm, ct_str);

  if (ct_num == CT_NUM_INVALID)
    return NSM_API_SET_ERR_INVALID_CLASS_TYPE;

  /* Update max resv bw */
  ret = nsm_qos_serv_update_bw_constraint (ifp, 0, ct_num);
  if (ret < 0)
    {
      return NSM_API_SET_ERROR;
    }

  return NSM_API_SET_SUCCESS;
}
#endif /*HAVE_DSTE*/
#endif /*HAVE_TE*/

#ifdef HAVE_L3
int
nsm_fib_retain_set (u_int32_t vr_id, int retain_time)
{
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (retain_time < 0 || retain_time > NSM_FIB_RETAIN_TIME_MAX)
    return NSM_API_SET_ERR_INVALID_VALUE;

  SET_FLAG (nm->flags, NSM_FIB_RETAIN_RESTART);
  nm->fib_retain_time = retain_time;

  /* Update RIB sweep timer.  */
  nsm_rib_sweep_timer_update (nm);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_fib_retain_unset (u_int32_t vr_id)
{
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  UNSET_FLAG (nm->flags, NSM_FIB_RETAIN_RESTART);
  nm->fib_retain_time = NSM_FIB_RETAIN_TIME_DEFAULT;

  /* Update RIB sweep timer.  */
  nsm_rib_sweep_timer_update (nm);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv4_forwarding_set (u_int32_t vr_id)
{
  struct nsm_master *nm;
  int ipforward = 1;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (CHECK_FLAG (nm->flags, NSM_IPV4_FORWARDING))
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_ipv4_forwarding_set (ipforward);
  if (ret != RESULT_OK)
    return NSM_API_SET_ERR_IPV4_FORWARDING_CANT_CHANGE;

  SET_FLAG (nm->flags, NSM_IPV4_FORWARDING);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */

#ifdef HAVE_ONMD
  nsm_oam_lldp_set_sys_cap (vr_id, NSM_LLDP_IPV4_ROUTING, PAL_TRUE);
#endif

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv4_forwarding_unset (u_int32_t vr_id)
{
  struct nsm_master *nm;
  int ipforward = 0;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (!CHECK_FLAG (nm->flags, NSM_IPV4_FORWARDING))
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_ipv4_forwarding_set (ipforward);
  if (ret != RESULT_OK)
    return NSM_API_SET_ERR_IPV4_FORWARDING_CANT_CHANGE;

  UNSET_FLAG (nm->flags, NSM_IPV4_FORWARDING);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */

#ifdef HAVE_ONMD
  nsm_oam_lldp_set_sys_cap (vr_id, NSM_LLDP_IPV4_ROUTING, PAL_FALSE);
#endif

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv4_route_set (struct nsm_master *nm,
                    vrf_id_t vrf_id, struct prefix_ipv4 *p,
                    union nsm_nexthop *gate, char *ifname, int distance,
                    int metric, int snmp_route_type,
                    u_int32_t tag, char *desc)
{
  struct nsm_vrf *vrf;
  int ret = 0;

  apply_mask_ipv4 (p);

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (vrf == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  ret = nsm_static_add (vrf, AFI_IP, (struct prefix *)p,
                        gate, ifname, distance, metric, snmp_route_type,
                        tag, desc);

#ifdef HAVE_VRX
  /* On success, export route add/delete to other Virtual Routers. */
  if (ret == 1)
    nsm_vr_export_static_route_ipv4 (1, p, &gate.ipv4,
                                     ifname, distance, vrf_id,
                                     tag, desc);
#endif /* HAVE_VRX */

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv4_route_unset (struct nsm_master *nm,
                      vrf_id_t vrf_id, struct prefix_ipv4 *p,
                      union nsm_nexthop *gate, char *ifname, int distance,
                      u_int32_t tag, char *desc)
{
  struct nsm_vrf *vrf;
  int ret = 0;

  apply_mask_ipv4 (p);

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (vrf == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  ret = nsm_static_delete (vrf, AFI_IP, (struct prefix *)p,
                           gate, ifname, distance);

#ifdef HAVE_VRX
  /* On success, export route add/delete to other Virtual Routers. */
  if (ret == 1)
    nsm_vr_export_static_route_ipv4 (0, p, &gate.ipv4,
                                     ifname, distance, vrf_id,
                                     tag, desc);
#endif /* HAVE_VRX */

  if (ret == 0)
    return NSM_API_SET_ERR_NO_MATCHING_ROUTE;

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv4_route_unset_all (struct nsm_master *nm,
                          vrf_id_t vrf_id, struct prefix_ipv4 *p)
{
  struct nsm_vrf *vrf;
  int ret = 0;

  apply_mask_ipv4 (p);

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (vrf != NULL)
    ret = nsm_static_delete_all (vrf, AFI_IP, (struct prefix *)p);

  return ret;
}

int
nsm_ipv4_route_stale_clear (u_int32_t vr_id)
{
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nsm_rib_sweep_stale (nm, AFI_IP);

  return NSM_API_SET_SUCCESS;
}

#ifdef HAVE_IPV6
int
nsm_ipv6_forwarding_set (u_int32_t vr_id)
{
  struct nsm_master *nm;
  int ipforward = 1;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (CHECK_FLAG (nm->flags, NSM_IPV6_FORWARDING))
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_ipv6_forwarding_set (ipforward);
  if (ret != RESULT_OK)
    return NSM_API_SET_ERR_IPV6_FORWARDING_CANT_CHANGE;

  SET_FLAG (nm->flags, NSM_IPV6_FORWARDING);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */

#ifdef HAVE_ONMD
  nsm_oam_lldp_set_sys_cap (vr_id, NSM_LLDP_IPV6_ROUTING, PAL_TRUE);
#endif

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv6_forwarding_unset (u_int32_t vr_id)
{
  struct nsm_master *nm;
  int ipforward = 0;
  int ret;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (!CHECK_FLAG (nm->flags, NSM_IPV6_FORWARDING))
    return NSM_API_SET_SUCCESS;

  ret = nsm_fea_ipv6_forwarding_set (ipforward);
  if (ret != RESULT_OK)
    return NSM_API_SET_ERR_IPV6_FORWARDING_CANT_CHANGE;

  UNSET_FLAG (nm->flags, NSM_IPV6_FORWARDING);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_master (nm);
#endif /* HAVE_HA */

#ifdef HAVE_ONMD
  nsm_oam_lldp_set_sys_cap (vr_id, NSM_LLDP_IPV6_ROUTING, PAL_FALSE);
#endif

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv6_route_set (struct nsm_master *nm,
                    vrf_id_t vrf_id, struct prefix_ipv6 *p,
                    union nsm_nexthop *gate, char *ifname, int distance)

{
  struct nsm_vrf *vrf;
  int ret = 0;

  apply_mask_ipv6 (p);

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (vrf == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  if (gate != NULL && IN6_IS_ADDR_LINKLOCAL (&gate->ipv6) && ifname == NULL)
    return NSM_API_SET_ERR_INVALID_IPV6_NEXTHOP_LINKLOCAL;

  ret = nsm_static_add (vrf, AFI_IP6, (struct prefix *)p,
                        gate, ifname, distance, 0, ROUTE_TYPE_OTHER,
                        APN_TAG_DEFAULT, NULL);

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv6_route_unset (struct nsm_master *nm,
                      vrf_id_t vrf_id, struct prefix_ipv6 *p,
                      union nsm_nexthop *gate, char *ifname, int distance)
{
  struct nsm_vrf *vrf;
  int ret = 0;

  apply_mask_ipv6 (p);

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (vrf == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  ret = nsm_static_delete (vrf, AFI_IP6, (struct prefix *)p,
                           gate, ifname, distance);

  if (ret == 0)
    return NSM_API_SET_ERR_NO_MATCHING_ROUTE;

  return NSM_API_SET_SUCCESS;
}

int
nsm_ipv6_route_unset_all (struct nsm_master *nm,
                          vrf_id_t vrf_id, struct prefix_ipv6 *p)
{
  struct nsm_vrf *vrf;
  int ret = 0;

  apply_mask_ipv6 (p);

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (vrf != NULL)
    ret = nsm_static_delete_all (vrf, AFI_IP6, (struct prefix *)p);

  return ret;
}

int
nsm_ipv6_route_stale_clear (u_int32_t vr_id)
{
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nsm_rib_sweep_stale (nm, AFI_IP6);

  return NSM_API_SET_SUCCESS;
}
#endif /* HAVE_IPV6 */

int
nsm_router_id_set (u_int32_t vr_id, vrf_id_t vrf_id,
                   struct pal_in4_addr *router_id)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  SET_FLAG (nv->config, NSM_VRF_CONFIG_ROUTER_ID);
  nv->router_id_config = *router_id;

#ifdef HAVE_HA
  nsm_cal_modify_nsm_vrf (nv);
#endif /* HAVE_HA */

  nsm_router_id_update (nv);

  return NSM_API_SET_SUCCESS;
}

int
nsm_router_id_unset (u_int32_t vr_id, vrf_id_t vrf_id)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  UNSET_FLAG (nv->config, NSM_VRF_CONFIG_ROUTER_ID);
  nv->router_id_config.s_addr = 0;

#ifdef HAVE_HA
  nsm_cal_modify_nsm_vrf (nv);
#endif /* HAVE_HA */

  nsm_router_id_update (nv);

  return NSM_API_SET_SUCCESS;
}
#endif /* HAVE_L3 */

int nsm_get_service_type (int protocol_id)
{
  switch (protocol_id)
  {
    case APN_PROTO_RIP:
    case APN_PROTO_RIPNG:
    case APN_PROTO_OSPF:
    case APN_PROTO_OSPF6:
    case APN_PROTO_BGP:
    case APN_PROTO_ISIS:
    case APN_PROTO_PIMSM:
    case APN_PROTO_PIMSM6:
    case APN_PROTO_DVMRP:
    case APN_PROTO_RSVP:
    case APN_PROTO_LDP:
      return NSM_SYS_SERVICE_L3;
      break;

    case APN_PROTO_STP:
    case APN_PROTO_MSTP:
    case APN_PROTO_RSTP:
    case APN_PROTO_8021X:
    case APN_PROTO_LACP:
    case APN_PROTO_ELMI:
      return NSM_SYS_SERVICE_L2;
      break;

    default:
      return 0;
  }
}

#ifdef HAVE_L3
int nsm_get_forward_number_ipv4 ()
{
  struct nsm_master *nm = NULL;
  struct nsm_vrf *vrf = NULL;
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib = NULL;
  struct nexthop *nexthop = NULL;
  int count = 0;

  nm = nsm_master_lookup_by_id (nzg, 0);

  if(nm)
    vrf = nsm_vrf_lookup_by_id (nm, 0);

  if (! vrf)
    return 0;

  NSM_CIDR_ROUTE_LOOP (rn, vrf, rib, nexthop)
    {
      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) &&
         CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
        count++;
    }

  return count;
}

/* Utility to check if the ifindex added by user for the specified gateway
 * is resolved or not.
 */
bool_t nsm_gateway_ifindex_match (struct pal_in4_addr route_dest,
                                  struct pal_in4_addr route_mask,
                                  struct pal_in4_addr next_hop,
                                  int ifindex)
{
  struct nsm_master *nm = NULL;
  struct nsm_vrf *vrf = NULL;
  struct prefix_ipv4 p;
  struct connected *ifc = NULL;
  struct nsm_ptree_node *rn = NULL;
  struct rib *match = NULL;
  struct nexthop *newhop = NULL;
  int result = PAL_FALSE;

  /* We consider only the default virtual router while doing lookup */
  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return PAL_FALSE;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (vrf == NULL)
    return PAL_FALSE;

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.prefix.s_addr = next_hop.s_addr;

  /* Check if nexthop address is ourself. */
  ifc = nsm_ipv4_connected_table_lookup (vrf, (struct prefix *)&p);
  if (ifc != NULL) /* nexthop address is ourself */
    {
      if (ifc->ifp->ifindex == ifindex)
        result = PAL_TRUE;
    }

  rn = nsm_ptree_node_match (vrf->RIB_IPV4_UNICAST, 
                             (u_char *)&p.prefix.s_addr, 
                             p.prefixlen);

  while (rn)
    {
      nsm_ptree_unlock_node (rn);

      /* Pick up selected route. */
      for (match = rn->info; match; match = match->next)
        { 
          if (CHECK_FLAG (match->flags, RIB_FLAG_SELECTED))
            {
              break;
            }
        } 

      /* If there is no selected route or matched route is EGP, go up
         tree. */
      if (! match || match->type == APN_ROUTE_BGP)
        {
          do 
            {
              rn = rn->parent;
            } while (rn && rn->info == NULL);
          if (rn)
            nsm_ptree_lock_node (rn);
        }
      else
        {
          for (newhop = match->nexthop; newhop; newhop = newhop->next)
            {
              if ( ! CHECK_FLAG (newhop->flags, NEXTHOP_FLAG_RECURSIVE))
                {
                  if (ifindex == newhop->ifindex)
                    {
                      result = PAL_TRUE;
                    }
                 }
            }
          do
            {
              rn = rn->parent;
            } while (rn && rn->info == NULL);
          if (rn)
            nsm_ptree_lock_node (rn);
        }
    }

  return result; 
}
#endif /* HAVE_L3 */

void  nsm_get_sys_desc (char *sysDesc)
{
  struct pal_utsname names;
  char nwsoftware[50];

  pal_mem_set (&names, 0, sizeof (struct pal_utsname));

  pal_uname (&names);
  pacos_version (nwsoftware, 50);

  pal_snprintf (sysDesc, SYS_DESC_LEN - 1, 
      "%s %s %s %s %s PacOS %s", names.sysname, names.nodename,
           names.release, names.version, names.machine, nwsoftware);

  return;
}

void nsm_get_sys_name (char *sysname)
{
  char domain[65];
  char* domainname = NULL;

  domainname = (char *) pal_get_sys_name ();
  if (domainname != NULL)
    {
      pal_strcpy (domain, domainname);
      pal_snprintf (sysname, SYS_NAME_LEN - 1, 
          "system domain name is: %s", domain);
    }
  else
    pal_snprintf (sysname, SYS_NAME_LEN - 1,"system domain name is: Unknown");

  return;
}

/* This is a stub filled in with the virtual router uptime. Should be written
 * for sysUpTime.
 */
pal_time_t nsm_get_sys_up_time ()
{
  pal_time_t sysUpTime = 0;
  struct nsm_master *nm = NULL;

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm == NULL)
    return 0;

  sysUpTime = pal_time_current (NULL) - nm->start_time;
  return sysUpTime;

}

/* This is a stub. Should be provided later. */
void nsm_get_sys_location (char *location)
{
  char Syslocation[] = "PacBridge, Shen Zhen";
  pal_strcpy (location, Syslocation);

  return;
}

int nsm_get_sys_services (void)
{
  struct nsm_server_client *nsc ;
  struct nsm_server_entry *nse ;
  bool_t service_l2 = PAL_FALSE;
  bool_t service_gw = PAL_FALSE;
  int service_type = 0;
  int i ;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      service_type = nsm_get_service_type (nse->service.protocol_id);
        if ((service_type == NSM_SYS_SERVICE_L2) && (service_l2 != PAL_TRUE))
          service_l2 = PAL_TRUE;
        else if ((service_type == NSM_SYS_SERVICE_L3)
                 && (service_gw != PAL_TRUE))
          service_gw = PAL_TRUE;
        if (service_l2 && service_gw)
          break;
    }

    if (service_l2 && service_gw )
      return ((1 << (NSM_SNMP_SYSSERVICE_DATALINK - 1)) +
             (1 << (NSM_SNMP_SYSSERVICE_GATEWAY - 1)) +
             (1 << (NSM_SNMP_SYSSERVICE_IPHOSTS - 1)));
    else if (service_l2 )
      return ((1 << (NSM_SNMP_SYSSERVICE_DATALINK - 1)) +
              (1  << (NSM_SNMP_SYSSERVICE_IPHOSTS - 1)));
    else if (service_gw == PAL_TRUE)
      return ((1 << (NSM_SNMP_SYSSERVICE_GATEWAY - 1)) +
              (1 << (NSM_SNMP_SYSSERVICE_IPHOSTS - 1)));
    else
      return (1 << (NSM_SNMP_SYSSERVICE_IPHOSTS - 1));
}

#ifdef HAVE_L3
int
nsm_arp_entry(struct show_arp *s, void *temp)
{
  struct cli *cli;
#ifdef HAVE_INTERFACE_NAME_MAPPING
  struct if_master ifg;
  char *name;
#endif /* HAVE_INTERFACE_NAME_MAPPING */

  cli = (struct cli *) temp;
#ifdef HAVE_INTERFACE_NAME_MAPPING
  ifg = cli->zg->ifg;
  name = if_map_lookup (ifg.if_map_hash, s->dev);
  if (name)
    pal_strcpy(s->dev, name);
#endif /* HAVE_INTERFACE_NAME_MAPPING */

  return  cli_out(cli,"%-23.23r %-23.23s %-13.13s %-8s \n",
                  &s->ipaddr,s->hwaddr,s->dev,
                  (s->flags == PAL_ARP_FLAG_STATIC) ? "Static" : "Dynamic");
}

int
nsm_arp_show (struct cli *cli)
{
#ifdef  ENABLE_HAL_PATH
  struct pal_in4_addr addr;
  int i, count = 0, j;
  bool_t first = NSM_TRUE;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *iv;
  struct hal_arp_cache_entry cache[HAL_ARP_CACHE_GET_COUNT];

  cli_out (cli, " IP Address      MAC Address   Interface  Type\n");
  for (j = 0; j < vector_max (vr->vrf_vec); j++)
    {
      if ((iv = vector_slot (vr->vrf_vec, j)) == NULL)
        continue;

      count = 0;
      first = NSM_TRUE;
      do
        {
          count = hal_arp_cache_get ((u_int16_t)iv->fib_id, first ? NULL : &addr, HAL_ARP_CACHE_GET_COUNT, cache);
          for (i = 0; i < count; i++)
            {
              cli_out (cli, "%-15r %.04hx.%.04hx.%.04hx %-10s %-9s\n",
                  &cache[i].ip_addr,
                  pal_ntoh16(((unsigned short *)cache[i].mac_addr)[0]),
                  pal_ntoh16(((unsigned short *)cache[i].mac_addr)[1]),
                  pal_ntoh16(((unsigned short *)cache[i].mac_addr)[2]),
                  if_index2name (&vr->ifm, cache[i].ifindex),
                  (cache[i].static_flag) ? "static" : "dynamic");
            }

          if (count > 0)
            IPV4_ADDR_COPY (&addr, &cache[i-1].ip_addr);

          first = NSM_FALSE;
        } while (count > 0);
    }
#endif

#ifdef  ENABLE_PAL_PATH
  cli_out(cli,"Address                 HWaddress              Interface      Type \n");
  if (pal_display_arp (nsm_arp_entry, (void *)cli) < 0)
    return NSM_API_SET_ERROR;
#endif
  return NSM_API_SET_SUCCESS;
}

int
nsm_if_arp_set (struct nsm_master *nm, struct interface *ifp)
{
  struct  nsm_arp_master *arp_master = NULL;
  struct  nsm_static_arp_entry *arp_entry = NULL;
  struct  ptree_node *pg = NULL;
  struct connected *ifc;
  u_int32_t ifindex;
  int ret;

  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifindex = ifp->ifindex;
  arp_master = nm->arp;

  if (!arp_master || !arp_master->nsm_static_arp_list)
    return NSM_API_SET_ARP_GENERAL_ERR;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      for (pg = ptree_top (arp_master->nsm_static_arp_list); pg;
                           pg = ptree_next (pg))
        {
          if ((arp_entry = pg->info))
            if (ifc == arp_entry->ifc)
              {
#ifdef  ENABLE_HAL_PATH
                u_int8_t is_proxy_arp = 0;
                ret = hal_arp_entry_add (&arp_entry->addr, arp_entry->mac_addr,ifindex, is_proxy_arp);

                if (ret != HAL_SUCCESS)
                  {
                    zlog_warn (NSM_ZG, " Error setting arp entry for %r \n",&arp_entry->addr);
                    XFREE(MTYPE_NSM_ARP_STATIC_ENTRY, arp_entry);
                    pg->info = NULL;
                    ptree_unlock_node (pg);
                  }
#endif

#ifdef  ENABLE_PAL_PATH

                if (if_is_up (ifp))
                  {
                    ret = pal_set_arp (&arp_entry->addr, arp_entry->mac_addr);
                    if (ret != 0)
                      {
                        zlog_warn (NSM_ZG, " Error setting arp entry for %r",&arp_entry->addr);
                        XFREE(MTYPE_NSM_ARP_STATIC_ENTRY, arp_entry);
                        pg->info = NULL;
                        ptree_unlock_node (pg);
                      }
                  }
#endif
              }
        }
    }
  return NSM_API_SET_SUCCESS;
}

int
nsm_if_arp_unset (struct nsm_master *nm, struct interface *ifp)
{
  struct  nsm_arp_master *arp_master = NULL;
  struct  nsm_static_arp_entry *arp_entry = NULL;
  struct  ptree_node *pg = NULL;
  struct connected *ifc;
  u_int32_t ifindex;
  int ret = 0;

  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifindex = ifp->ifindex;
  arp_master = nm->arp;

  if (!arp_master || !arp_master->nsm_static_arp_list)
    return NSM_API_SET_ARP_GENERAL_ERR;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      for (pg = ptree_top (arp_master->nsm_static_arp_list); pg;
                           pg = ptree_next (pg))
        {
          if ((arp_entry = pg->info))
            if (ifc == arp_entry->ifc)
              {
#ifdef  ENABLE_HAL_PATH
                hal_arp_entry_del (&arp_entry->addr, arp_entry->mac_addr, ifindex);
#endif

#ifdef  ENABLE_PAL_PATH
                ret = pal_unset_arp (&arp_entry->addr);
#endif
              }
        }
    }
  return ret;
}

int
nsm_api_arp_entry_add (struct nsm_master *nm, struct pal_in4_addr *addr,
                       unsigned char *mac_addr, struct connected *ifc,
                        struct interface *ifp,u_int8_t is_proxy_arp)
{
  int ret;
  struct  nsm_arp_master *arp_master = NULL;
  struct  nsm_static_arp_entry *arp_entry = NULL;
  struct  ptree_node *pg = NULL;
  u_char key [IPV4_MAX_BYTELEN];
  u_int32_t ifindex;

  ifindex = ifp->ifindex;
  if (! nm || ! addr)
    return NSM_API_SET_ERR_INVALID_VALUE;

  arp_master = nm->arp;
  if (!arp_master || !arp_master->nsm_static_arp_list)
    return NSM_API_SET_ARP_GENERAL_ERR;
   pal_mem_cpy (key, (u_char *)&addr->s_addr, IPV4_MAX_BYTELEN);

  pg = ptree_node_get (arp_master->nsm_static_arp_list, key,
                       IPV4_MAX_BITLEN);

  if ( pg == NULL )
    {
      return NSM_API_SET_ARP_GENERAL_ERR;
    }

  if (pg->info)
    {
      arp_entry = pg->info;

#ifdef  ENABLE_HAL_PATH
      hal_arp_entry_del (&arp_entry->addr,arp_entry->mac_addr, ifindex);
      ret = hal_arp_entry_add (addr, mac_addr, ifindex,is_proxy_arp);
      if (ret != HAL_SUCCESS)
        {
          XFREE(MTYPE_NSM_ARP_STATIC_ENTRY, arp_entry);
          pg->info = NULL;
          ptree_unlock_node (pg); 
          return NSM_API_SET_ERR_HAL_FAILURE;
        }
#endif

#ifdef  ENABLE_PAL_PATH
      if (if_is_up (ifp))
        {
          ret = pal_set_arp(addr,mac_addr);
          if (ret != 0)
            {
              XFREE(MTYPE_NSM_ARP_STATIC_ENTRY, arp_entry);
              pg->info = NULL;
              ptree_unlock_node (pg);
              return NSM_API_SET_ERROR;
            }
        }
#endif
       /* Config for this entry is already saved hence quit*/
       if (pal_mem_cmp (arp_entry->mac_addr, mac_addr, ETHER_ADDR_LEN) == 0)
         {
           ptree_unlock_node (pg);
           return 0;
         }

      pal_mem_cpy (arp_entry->mac_addr, mac_addr, ETHER_ADDR_LEN);
      ptree_unlock_node (pg);
    }
  else
    {
#ifdef  ENABLE_HAL_PATH
      ret = hal_arp_entry_add (addr, mac_addr, ifindex, is_proxy_arp);
      if (ret != HAL_SUCCESS)
        return NSM_API_SET_ERR_HAL_FAILURE;
#endif

#ifdef  ENABLE_PAL_PATH
      if (if_is_up (ifp))
        {
          ret = pal_set_arp(addr,mac_addr);
          if (ret != 0)
            return NSM_API_SET_ERROR;
        }
#endif
      arp_entry = XCALLOC (MTYPE_NSM_ARP_STATIC_ENTRY, sizeof (struct nsm_static_arp_entry));
      if (!arp_entry)
        return NSM_API_SET_ARP_GENERAL_ERR;

      pal_mem_cpy (&arp_entry->addr, addr, sizeof (struct pal_in4_addr));
      pal_mem_cpy (arp_entry->mac_addr, mac_addr, ETHER_ADDR_LEN);
      arp_entry->ifc = ifc;
      arp_entry->pn = pg;
      arp_entry->is_arp_proxy = is_proxy_arp;
      pg->info = arp_entry;
    }

  return 0;
}


int
nsm_api_arp_entry_del (struct nsm_master *nm, struct pal_in4_addr *addr,
                       unsigned char *mac_addr, struct interface *ifp)
{
  struct nsm_arp_master *arp_master = NULL;
  struct nsm_static_arp_entry *arp_entry = NULL;
  struct ptree_node *pg = NULL;
  u_char key [IPV4_MAX_BYTELEN];
  int ret = 0;

  if (! nm || ! addr)
    return NSM_API_SET_ERR_INVALID_VALUE;

  arp_master = nm->arp;


  if (!arp_master || !arp_master->nsm_static_arp_list)
    return NSM_API_SET_ARP_GENERAL_ERR;

  pal_mem_cpy (key, (u_char *)&addr->s_addr, IPV4_MAX_BYTELEN);

  pg = ptree_node_lookup (arp_master->nsm_static_arp_list, key,
                          IPV4_MAX_BITLEN);

  if ( !pg || !pg->info)
    {
      return 0;
    }

  arp_entry = pg->info;

#ifdef  ENABLE_HAL_PATH
  hal_arp_entry_del (addr, mac_addr, arp_entry->ifc->ifp->ifindex);
#endif

#ifdef  ENABLE_PAL_PATH
  ret =  pal_unset_arp (addr);
  if (ret != 0 && if_is_up (ifp))
    return NSM_API_SET_ERROR;
#endif

  XFREE(MTYPE_NSM_ARP_STATIC_ENTRY, arp_entry);
  pg->info = NULL;
  ptree_unlock_node (pg);

  return ret;
}


void
nsm_api_arp_del_all (struct nsm_master *nm, u_char clr_flag)
{
  struct nsm_static_arp_entry *arp_entry = NULL;
  struct ptree_node *node                = NULL;

#ifdef  ENABLE_HAL_PATH
  struct apn_vrf *iv;
  int i;

  /* Delete ARP entries from all tables. */
  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)))
      hal_arp_del_all ((u_int16_t)iv->fib_id, clr_flag);

  if (FLAG_ISSET (clr_flag, NSM_API_ARP_FLAG_STATIC))
    {
      for (node = ptree_top (nm->arp->nsm_static_arp_list); 
           node; node = ptree_next (node))
       {
         if ((arp_entry = node->info))
           {
             node->info = NULL;
             ptree_unlock_node (node);
             XFREE(MTYPE_NSM_ARP_STATIC_ENTRY, arp_entry);
           }
       }
    }
#endif

#ifdef  ENABLE_PAL_PATH
  int ret;

  /* Delete all the Dynamic ARP entries */
  pal_unset_arp_all();

  /* Delete the Static entries only if requested for */
  if (FLAG_ISSET (clr_flag, NSM_API_ARP_FLAG_STATIC))
    {
      for (node = ptree_top (nm->arp->nsm_static_arp_list);
          node; node = ptree_next (node))
        {
          if ((arp_entry = node->info))
            {
              ret = pal_unset_arp(&arp_entry->addr);
              node->info = NULL;
              ptree_unlock_node (node);
              XFREE(MTYPE_NSM_ARP_STATIC_ENTRY, arp_entry);
            }
        }
    }
#endif
}

struct nsm_arp_master *
nsm_arp_create_master ()
{

  struct nsm_arp_master *arp_master = NULL;

  arp_master = XCALLOC (MTYPE_NSM_ARP_MASTER, sizeof (struct nsm_arp_master));

  if (!arp_master)
    return NULL;

  arp_master->nsm_static_arp_list = ptree_init (IPV4_MAX_BITLEN);

  if (!arp_master->nsm_static_arp_list)
    {
      XFREE (MTYPE_NSM_ARP_MASTER, arp_master);
      return NULL;
    }

#ifdef HAVE_IPV6
  arp_master->ipv6_static_nbr_table = ptree_init (IPV6_MAX_BITLEN);
  if (! arp_master->ipv6_static_nbr_table)
    {
      XFREE (MTYPE_NSM_ARP_MASTER, arp_master);
      ptree_finish (arp_master->nsm_static_arp_list);
      return NULL;
    }
#endif /* HAVE_IPV6 */

  return arp_master;
}

void
nsm_arp_destroy_master (struct nsm_master *master)
{
  struct nsm_arp_master *arp_master      = NULL;
  struct ptree *arp_list                 = NULL;

  if (!master || !master->arp )
    return;

  arp_master = master->arp;

  arp_list = arp_master->nsm_static_arp_list;

  if (!arp_list)
    return;

  nsm_api_arp_del_all (master, NSM_API_ARP_FLAG_STATIC);
  ptree_finish (arp_list);

#ifdef HAVE_IPV6
  nsm_api_ipv6_nbr_del_all (master, NSM_API_ARP_FLAG_STATIC);
  ptree_finish (arp_master->ipv6_static_nbr_table);
#endif /* HAVE_IPV6 */

  XFREE(MTYPE_NSM_ARP_MASTER, arp_master);
  master->arp = NULL;
}


#ifdef HAVE_IPV6
int 
nsm_api_ipv6_nbr_add (struct nsm_master *nm, struct prefix_ipv6 *p,
                      struct interface *ifp, u_char *mac_addr )
{
#ifdef HAVE_HAL
  int ret;

  struct nsm_arp_master *arp_master = NULL;
  struct nsm_ipv6_static_nbr_entry *nbr_entry = NULL;
  struct ptree_node *pn = NULL;
  u_char key[IPV6_MAX_BYTELEN];

  if (! nm || ! p || ! mac_addr || ! ifp)
    return NSM_API_SET_ERR_INVALID_VALUE;

  arp_master = nm->arp;
  if (! arp_master || ! arp_master->ipv6_static_nbr_table)
    return NSM_API_SET_ARP_GENERAL_ERR;
  
  pal_mem_cpy (key, (u_char *)&p->prefix.s6_addr, IPV6_MAX_BYTELEN);
  pn = ptree_node_get (arp_master->ipv6_static_nbr_table,
                       key, IPV6_MAX_BITLEN);
  if (! pn)
    {
      return NSM_API_SET_ARP_GENERAL_ERR;
    }

  if (pn->info)
    {
      nbr_entry = pn->info;
      if (pal_mem_cmp (nbr_entry->mac_addr, mac_addr, ETHER_ADDR_LEN) != 0)
        {
          hal_ipv6_nbr_del (&nbr_entry->addr, nbr_entry->mac_addr,
              ifp->ifindex);
        }
      else
        return NSM_API_SET_ERR_ENTRY_EXISTS;
      
      ptree_unlock_node (pn);
    }
  else
    {
      nbr_entry = XCALLOC (MTYPE_NSM_IPV6_STATIC_NBR_ENTRY, 
                           sizeof (struct nsm_ipv6_static_nbr_entry));
      if (! nbr_entry)
        {
          ptree_unlock_node (pn);
          return NSM_API_SET_ARP_GENERAL_ERR;
        }
      pn->info = nbr_entry;
    }

  pal_mem_cpy (nbr_entry->mac_addr, mac_addr, ETHER_ADDR_LEN);
  pal_mem_cpy (&nbr_entry->addr, &p->prefix, sizeof (struct pal_in6_addr));
  nbr_entry->ifp = ifp;
  
  ret = hal_ipv6_nbr_add (&p->prefix, mac_addr, ifp->ifindex);
  if (ret != HAL_SUCCESS)
    {
      pn->info = NULL;
      ptree_unlock_node (pn);
      XFREE (MTYPE_NSM_IPV6_STATIC_NBR_ENTRY, nbr_entry);
      return NSM_API_SET_ERR_HAL_FAILURE;
    }
#endif /* HAVE_HAL */

  return NSM_SUCCESS;
}


int
nsm_api_ipv6_nbr_del (struct nsm_master *nm, struct prefix_ipv6 *p,
                      struct interface *ifp)
{
#ifdef HAVE_HAL
  struct nsm_arp_master *arp_master = NULL;
  struct nsm_ipv6_static_nbr_entry *nbr_entry = NULL;
  struct ptree_node *pn = NULL;
  u_char key[IPV6_MAX_BYTELEN];

  if (! nm || ! p || ! ifp)
    return NSM_API_SET_ERR_INVALID_VALUE;
  
  arp_master = nm->arp;
  if (! arp_master || ! arp_master->ipv6_static_nbr_table)
    return NSM_API_SET_ARP_GENERAL_ERR;
  
  pal_mem_cpy (key, (u_char *)&p->prefix.s6_addr, IPV6_MAX_BYTELEN);
  pn = ptree_node_lookup (arp_master->ipv6_static_nbr_table,
        key, IPV6_MAX_BITLEN);
  if (! pn)
    {
      return NSM_API_SET_ARP_GENERAL_ERR;
    }

  nbr_entry = pn->info;

  hal_ipv6_nbr_del (&nbr_entry->addr, nbr_entry->mac_addr, ifp->ifindex);

  pn->info = NULL;
  ptree_unlock_node (pn);
  ptree_unlock_node (pn);
  XFREE (MTYPE_NSM_IPV6_STATIC_NBR_ENTRY, nbr_entry);
#endif /* HAVE_HAL */

  return NSM_SUCCESS;
}


void
nsm_api_ipv6_nbr_del_by_ifp (struct nsm_master *nm, struct interface *ifp)
{
#ifdef HAVE_HAL
  struct ptree_node *node;
  struct nsm_ipv6_static_nbr_entry *nbr_entry = NULL;

  if (! nm->arp || ! nm->arp->ipv6_static_nbr_table)
    return;

  for (node = ptree_top (nm->arp->ipv6_static_nbr_table); node;
       node = ptree_next (node))
    {
      if (! node->info)
        continue;
      
      nbr_entry = node->info;
      if (nbr_entry->ifp == ifp)
        {
          node->info = NULL;
          ptree_unlock_node (node);
          hal_ipv6_nbr_del (&nbr_entry->addr, nbr_entry->mac_addr, 
              ifp->ifindex);
          XFREE (MTYPE_NSM_IPV6_STATIC_NBR_ENTRY, nbr_entry);
        }
    }
#endif /* HAVE_HAL */
}


void
nsm_api_ipv6_nbr_del_all (struct nsm_master *nm, u_char clr_flag)
{
#ifdef HAVE_HAL
  struct ptree_node *node;
  struct nsm_ipv6_static_nbr_entry *nbr_entry = NULL;
  struct apn_vrf *iv;
  int i;

  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)))
      hal_ipv6_nbr_del_all ((u_int16_t)iv->fib_id, clr_flag);

  if (FLAG_ISSET (clr_flag, NSM_API_ARP_FLAG_STATIC))
    {
      for (node = ptree_top (nm->arp->ipv6_static_nbr_table); node;
           node = ptree_next (node))
        {
          if (! node->info)
            continue;

          nbr_entry = node->info;
                node->info = NULL;
                ptree_unlock_node (node);
                XFREE (MTYPE_NSM_IPV6_STATIC_NBR_ENTRY, nbr_entry);
        }
    }
#endif /* HAVE_HAL */
}

void
nsm_api_ipv6_nbr_all_show (struct cli *cli)
{
#ifdef HAVE_HAL
  struct pal_in6_addr addr;
  int i, count = 0, j;
  bool_t first = NSM_TRUE;
  struct apn_vr *vr = cli->vr;
  struct apn_vrf *iv;
  struct hal_ipv6_nbr_cache_entry cache[HAL_IPV6_NBR_CACHE_GET_COUNT];

  cli_out (cli, " %-40s %-15s %-10s %-9s\n", "IPv6 Address", "MAC Address",
                  "Interface", "Type");
  for (j = 0; j < vector_max (vr->vrf_vec); j++)
    {
      if ((iv = vector_slot (vr->vrf_vec, j)) == NULL)
        continue;

      count = 0;
      first = NSM_TRUE;
      do
        {
          count = hal_ipv6_nbr_cache_get ((u_int16_t)iv->fib_id,
              first ? NULL : &addr, HAL_IPV6_NBR_CACHE_GET_COUNT, cache);
          for (i = 0; i < count; i++)
            {
              cli_out (cli, "%-40R %.04hx.%.04hx.%.04hx   %-10s %-9s\n",
                  &cache[i].addr,
                  pal_ntoh16(((unsigned short *)cache[i].mac_addr)[0]),
                  pal_ntoh16(((unsigned short *)cache[i].mac_addr)[1]),
                  pal_ntoh16(((unsigned short *)cache[i].mac_addr)[2]),
                  if_index2name (&vr->ifm, cache[i].ifindex),
                  (cache[i].static_flag) ? "static" : "dynamic");
            }
          if (count > 0)
            IPV6_ADDR_COPY (&addr, &cache[i-1].addr);

          first = NSM_FALSE;
        } while (count > 0);
    }
#endif /* HAVE_HAL */
}

#endif /* HAVE_IPV6 */

#endif /* HAVE_L3 */


/* This is a stub. Should be provided later */
#ifdef HAVE_SNMP
oid *
nsm_get_sys_oid (void)
{
  oid *sys_oid = NULL;
  return sys_oid; 
}

void
nsm_pal_ent_vars_get (struct entPhysicalEntry *entPhy)
{
  /* Hal call would be implemented */
  entPhy->entPhysicalVendorType[0] = '\0';
  entPhy->entPhysicalHardwareRev[0] = '\0';
  entPhy->entPhysicalFirmwareRev[0] = '\0';
  entPhy->entPhysicalSerialNum[0] = '\0';
  entPhy->entPhysicalMfgName[0] = '\0';
  entPhy->entPhysicalModelName[0] = '\0';
  entPhy->entPhysicalAlias[0] = '\0';
  entPhy->entPhysicalAssetID[0] = '\0';
  entPhy->entPhysicalIsFRU = NSM_SNMP_FRU_FALSE;
}

int
nsm_get_next_phy_index (struct nsm_master *nm)
{
  struct entPhysicalEntry *entPhy = NULL;
  struct listnode *node = NULL;
  int index = 1;

  LIST_LOOP (nm->phyEntityList, entPhy, node)
    index++;
  return index;
}

void
nsm_phy_entity_create (struct nsm_master *nm, u_int32_t class, 
                       struct interface *ifp)
{
  struct entPhysicalEntry *entPhy = NULL;
  struct entPhysicalEntry *entPhytab = NULL;
  u_int32_t ContainedInindex = 0;
  struct listnode *node = NULL;
  char buf [50];
  int index = 0;

  entPhy = XMALLOC (MTYPE_TMP, sizeof (struct entPhysicalEntry));
  if (! entPhy)
    return;

  index = nsm_get_next_phy_index (nm);

  pacos_version (buf, 50);
  pal_strcpy (entPhy->entPhysicalSoftwareRev, buf);
  nsm_pal_ent_vars_get (entPhy);

  switch (class)
    {
      case CLASS_CHASSIS:
        entPhy->entPhysicalIndex = index;
        entPhy->entPhysicalDescr = "CHASSIS";
        entPhy->entPhysicalContainedIn = 0; /* For Chassis */
        entPhy->entPhysicalClass = class;
        entPhy->entPhysicalParentRelPos = -1; /* No parent for Chassis */
        entPhy->entPhysicalName = "CHASSIS";
      break;

      case CLASS_PORT:
        LIST_LOOP (nm->phyEntityList, entPhytab, node)
          {
            if (entPhytab->entPhysicalClass == CLASS_CHASSIS)
              ContainedInindex = entPhytab->entPhysicalIndex;
            break;
          }
        entPhy->entPhysicalContainedIn = ContainedInindex; /* Chassis index */
        entPhy->entPhysicalIndex = index;
        entPhy->entPhysicalDescr = "PORT";
        entPhy->entPhysicalClass = class;
        entPhy->entPhysicalParentRelPos = ifp->ifindex;
        entPhy->entPhysicalName = ifp->name;
      break;

      default:
      break;
    }

  listnode_add (nm->phyEntityList, entPhy);
  nm->last_change_time = pal_time_current (NULL);
  nsm_ent_config_chg_trap ();
}

void
nsm_phy_entity_delete (struct nsm_master *nm, struct interface *ifp)
{
  struct entPhysicalEntry *entPhy = NULL;
  struct listnode *node = NULL;
  int index = 0;

  /* Loop through the phyEntityList to find the node & delete */
  LIST_LOOP (nm->phyEntityList, entPhy, node)
    if (! pal_strcmp (entPhy->entPhysicalName, ifp->name))
      {
        index = entPhy->entPhysicalIndex;
        listnode_delete (nm->phyEntityList, entPhy);
        XFREE (MTYPE_TMP, entPhy);
        break;
      }

  /* Decrement entPhysicalIndex by 1 for further nodes */
  LIST_LOOP (nm->phyEntityList, entPhy, node)
    if (entPhy->entPhysicalIndex > index)
      entPhy->entPhysicalIndex = entPhy->entPhysicalIndex - 1;

  nm->last_change_time = pal_time_current (NULL);
  nsm_ent_config_chg_trap ();
}

#if defined HAVE_HAL && defined HAVE_L2
int
nsm_get_rcvaddress_status (struct rcvaddr_index *rcvaddr, int *status)
{
  *status = NSM_SNMP_ROW_STATUS_ACTIVE;
  return NSM_API_GET_SUCCESS;
}

int
nsm_get_next_rcvaddress_status (struct rcvaddr_index *rcvaddr, int *status)
{
  *status = NSM_SNMP_ROW_STATUS_ACTIVE;
  return NSM_API_GET_SUCCESS;
}

int
nsm_set_rcvaddress_status (struct nsm_master *nm, 
                           struct rcvaddr_index *rcvaddr, 
                           int status)
{
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge = nsm_get_first_bridge (master);
  u_int16_t vid = NSM_VLAN_NULL_VID;
  u_int8_t mac[6];
  int ret = 0;

  if (bridge == NULL)
    return NSM_API_SET_ERROR;

  /* Get the MAC address */
  pal_mem_cpy (&mac[0], rcvaddr->mac_addr, 6);

  switch (status)
    {
      case NSM_SNMP_ROW_STATUS_CREATEANDGO:
        ret = hal_l2_add_fdb (bridge->name, rcvaddr->ifindex, mac, 
                              HAL_HW_LENGTH, vid, vid,
                              HAL_L2_FDB_STATIC, PAL_TRUE);
        if (ret != HAL_SUCCESS)
          return NSM_API_SET_ERROR;

        if (nsm_add_mac_to_static_list (bridge, bridge->static_fdb_list, 
                                        rcvaddr->ifindex, mac,
                                        PAL_TRUE, CLI_MAC, vid) != RESULT_OK )
          return NSM_API_SET_ERROR;
      break;

      default:
        return NSM_API_SET_ERROR;
      break;
    }
  return NSM_API_SET_SUCCESS;
}

int
nsm_get_rcvaddress_type (struct rcvaddr_index *rcvaddr, int *type)
{
  *type = NONVOLATILE;
  return NSM_API_GET_SUCCESS;
}

int
nsm_get_next_rcvaddress_type (struct rcvaddr_index *rcvaddr, int *type)
{
  *type = NONVOLATILE;
  return NSM_API_GET_SUCCESS;
}

int
nsm_set_rcvaddress_type (struct rcvaddr_index *rcvaddr, int *type)
{
  if (*type == VOLATILE)
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}
  
#endif /* HAVE_L2 && HAVE_HAL*/
#endif /* HAVE_SNMP */

int
nsm_filter_set_group (struct apn_vr *vr, char *name_str, char *dir_str,
                      char *ifname, int enabled)
{
#ifdef HAVE_HAL
  struct hal_ip_access_grp access_grp;
  int dir_type;
  int action;
#endif /* HAVE_HAL */
  int ret;
  struct interface *ifp;
  struct access_list *access;
  struct nsm_if *zif;

  ret = NSM_SUCCESS;

#ifdef HAVE_HAL
  if (pal_strncmp (dir_str, "in", 2) != 0)
    dir_type = HAL_ACL_DIRECTION_INGRESS;
  else if (pal_strncmp (dir_str, "out", 3) != 0)
    dir_type =  HAL_ACL_DIRECTION_EGRESS;
  else
    return NSM_API_SET_ERR_INVALID_VALUE;
#endif /* HAVE_HAL */

   /* Check if access list is found */
  if ((access = access_list_lookup (vr, AFI_IP, name_str)) == NULL)
    return NSM_API_SET_ERR_ACL_INVALID_VALUE;

  ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  zif = (struct nsm_if *)ifp->info;
  if (zif != NULL)
    {
      if (enabled)
        {
#ifdef HAVE_HAL
          /* Code the ACL List */
          ret = nsm_set_acl_for_access_group (access, &access_grp);
          if (ret != HAL_SUCCESS)
            return NSM_API_SET_ERR_HAL_FAILURE;
          action = HAL_ACL_ACTION_ATTACH;
          ret = hal_ip_set_access_group (access_grp, ifname,
                                         action, dir_type);
          if (ret != HAL_SUCCESS)
            return NSM_API_SET_ERR_HAL_FAILURE;
#endif /*HAVE_HAL */

          zif->acl_name_str = XCALLOC (MTYPE_TMP, (pal_strlen (name_str) + 1));
          zif->acl_dir_str = XCALLOC (MTYPE_TMP, (pal_strlen (dir_str) + 1));
          pal_strcpy (zif->acl_name_str, name_str);
          pal_strcpy (zif->acl_dir_str, dir_str);
        }
      else
        {
          if (zif->acl_name_str && zif->acl_dir_str)
            {
              if (!pal_strcmp (zif->acl_name_str, name_str) &&
                  !pal_strcmp (zif->acl_dir_str, dir_str))
                {
#ifdef HAVE_HAL
                  /* Code the ACL List */
                  ret = nsm_set_acl_for_access_group (access, &access_grp);
                  if (ret != HAL_SUCCESS)
                    return NSM_API_SET_ERR_HAL_FAILURE;
                  action = HAL_ACL_ACTION_DETACH;
                  ret = hal_ip_set_access_group (access_grp, ifname,
                                                 action, dir_type);

                  if (ret != HAL_SUCCESS)
                    return NSM_API_SET_ERR_HAL_FAILURE;
#endif /* HAVE_HAL */
                  if (zif->acl_name_str)
                    {
                      XFREE (MTYPE_TMP, zif->acl_name_str);
                      zif->acl_name_str = NULL;
                    }

                  if (zif->acl_dir_str)
                    {
                      XFREE (MTYPE_TMP, zif->acl_dir_str);
                      zif->acl_dir_str = NULL;
                    }
                }
              else
                return NSM_API_ACL_ERR_NOT_SET;
            }
          else
            return NSM_API_ACL_ERR_NOT_SET;
        }
    }
  else
    return NSM_API_SET_ERR_HAL_FAILURE;

  return NSM_SUCCESS;
}

#ifdef HAVE_HAL
int
nsm_set_acl_for_access_group (struct access_list *access,
                              struct hal_ip_access_grp *access_grp)
{
  struct filter_list *mfilter;
  int i = 0;

  pal_mem_set(access_grp, 0, sizeof(struct hal_ip_access_grp));

  if (access->head == NULL)
    return -1;
  if (access->head->common == FILTER_COMMON)
    {
      for (mfilter = access->head; mfilter; mfilter = mfilter->next)
        {
          access_grp->hfilter[i].type
                            = mfilter->type;
          access_grp->hfilter[i].ace.ipfilter.addr
                            = mfilter->u.cfilter.addr.s_addr;
          access_grp->hfilter[i].ace.ipfilter.addr_mask
                            = mfilter->u.cfilter.addr_mask.s_addr;
          access_grp->hfilter[i].ace.ipfilter.mask
                            = mfilter->u.cfilter.mask.s_addr;
          access_grp->hfilter[i].ace.ipfilter.mask_mask
                            = mfilter->u.cfilter.mask_mask.s_addr;
          i++;
          if (i >= HAL_IP_MAX_ACL_FILTER)
            break;
        }
    }
  else if (access->head->common == FILTER_PACOS)
    {
      for (mfilter = access->head; mfilter; mfilter = mfilter->next)
        {
          access_grp->hfilter[i].type
                            = mfilter->type;
          access_grp->hfilter[i].ace.ipfilter.addr
                            = mfilter->u.zfilter.prefix.u.prefix4.s_addr;
          access_grp->hfilter[i].ace.ipfilter.addr_mask
                            = nsm_masklen_to_mask_ipv4
                              (mfilter->u.zfilter.prefix.prefixlen);
          access_grp->hfilter[i].ace.ipfilter.addr_mask
                            = ~(access_grp->hfilter[i].ace.ipfilter.addr_mask);
          i++;
          if (i >= HAL_IP_MAX_ACL_FILTER)
            break;
        }
    }
  else if (access->head->common == FILTER_PACOS_EXT)
    {
      for (mfilter = access->head; mfilter; mfilter = mfilter->next)
        {
          access_grp->hfilter[i].type
                            = mfilter->type;
          access_grp->hfilter[i].ace.ipfilter.addr
                            = mfilter->u.zextfilter.sprefix.u.prefix4.s_addr;
          access_grp->hfilter[i].ace.ipfilter.addr_mask
                            = nsm_masklen_to_mask_ipv4
                              (mfilter->u.zextfilter.sprefix.prefixlen);
          access_grp->hfilter[i].ace.ipfilter.addr_mask
                            = ~(access_grp->hfilter[i].ace.ipfilter.addr_mask);
          access_grp->hfilter[i].ace.ipfilter.mask
                            = mfilter->u.zextfilter.dprefix.u.prefix4.s_addr;
          access_grp->hfilter[i].ace.ipfilter.mask_mask
                            = nsm_masklen_to_mask_ipv4
                              (mfilter->u.zextfilter.dprefix.prefixlen);
          access_grp->hfilter[i].ace.ipfilter.mask_mask
                            = ~(access_grp->hfilter[i].ace.ipfilter.mask_mask);
          i++;
          if (i >= HAL_IP_MAX_ACL_FILTER)
            break;
        }
    }
  else
    return -1;

  access_grp->ace_number = i;
  return 0;
}
#endif /* HAVE_HAL */

/* Function to convert IPv4 mask length to mask.  */
u_int32_t
nsm_masklen_to_mask_ipv4 (u_int8_t masklen)
{
  if (masklen == 0)
    return 0;
  else
    return ((u_int32_t)((s_int32_t)(0x80000000) >> (masklen - 1)));
}

int
nsm_filter_set_interface_port_access_group (struct apn_vr *vr, char *name,
                                            char *direct, char *vifname,
                                            struct interface *ifp, int enabled)
{
#ifdef HAVE_HAL
  struct hal_ip_access_grp access_grp;
  int dir_type;
  int action;
#endif /* HAVE_HAL */
  int ret;
  struct access_list *access;
  struct nsm_if *zif;
  struct interface *vifp;

  ret = NSM_SUCCESS;

#ifdef HAVE_HAL
  if (pal_strncmp (direct, "in", 2) != 0)
    dir_type = HAL_ACL_DIRECTION_INGRESS;
  else if (pal_strncmp (direct, "out", 3) != 0)
    dir_type =  HAL_ACL_DIRECTION_EGRESS;
  else
    return NSM_API_SET_ERR_INVALID_VALUE;
#endif /* HAVE_HAL */

   /* Check if access list is found */
  if ((access = access_list_lookup (vr, AFI_IP, name)) == NULL)
    return NSM_API_SET_ERR_ACL_INVALID_VALUE;

  vifp = if_lookup_by_name (&vr->ifm, vifname);

  if (vifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  zif = (struct nsm_if *)vifp->info;
  if (zif != NULL)
    {
      if (enabled)
        {
#ifdef HAVE_HAL
          /* Code the ACL List */
          ret = nsm_set_acl_for_access_group (access, &access_grp);
          if (ret != HAL_SUCCESS)
            return NSM_API_SET_ERR_HAL_FAILURE;
          action = HAL_ACL_ACTION_ATTACH;
          ret = hal_ip_set_access_group_interface (access_grp, vifname,
                                                   ifp->name, action,
                                                   dir_type);
          if (ret != HAL_SUCCESS)
            return NSM_API_SET_ERR_HAL_FAILURE;
#endif /*HAVE_HAL */
        }
      else
        {
#ifdef HAVE_HAL
          /* Code the ACL List */
          ret = nsm_set_acl_for_access_group (access, &access_grp);
          if (ret != HAL_SUCCESS)
            return NSM_API_SET_ERR_HAL_FAILURE;
          action = HAL_ACL_ACTION_DETACH;
          ret = hal_ip_set_access_group_interface (access_grp, vifname,
                                                   ifp->name, action,
                                                   dir_type);

          if (ret != HAL_SUCCESS)
            return NSM_API_SET_ERR_HAL_FAILURE;
#endif /* HAVE_HAL */
        }
    }
  else
    return NSM_API_SET_ERR_HAL_FAILURE;

  return NSM_SUCCESS;
}


/* Create/Remove the access-group. */
int
nsm_access_group_set (struct apn_vr *vr, char *name,
                      char *direct, char *ifname, int enabled)
{
  return nsm_filter_set_group (vr, name, direct, ifname, enabled);
}

#ifdef HAVE_HWSTACK
int nsm_get_master_cpu_entry (struct nsm_cpu_info_entry *cpu_entry)
{
#ifdef HAVE_HAL
  /* Write an API in BCM and then call that API from HAL.
     Call HAL information from here */
  hal_get_master_cpu_entry ((unsigned char *)cpu_entry); 
#endif
  return 1;
}

int nsm_get_local_cpu_entry (struct nsm_cpu_info_entry *cpu_entry)
{
#ifdef HAVE_HAL
  /* Write an API in BCM and then call that API from HAL.
     Call HAL information from here */
  hal_get_local_cpu_entry ((unsigned char *)cpu_entry); 
#endif
  return 1;
}

int nsm_get_index_dump_cpu_entry (u_int32_t entry_num,
                           struct nsm_cpu_dump_entry *cpu_entry)
{
#ifdef HAVE_HAL
  /* Write an API in BCM and then call that API from HAL.
     Call HAL information from here */
  hal_get_index_dump_cpu_entry (entry_num, (unsigned char *)cpu_entry); 
#endif
  return 1;
}

int nsm_get_index_cpu_entry (u_int32_t entry_num,
                           struct nsm_cpu_info_entry *cpu_entry)
{
#ifdef HAVE_HAL
  /* Write an API in BCM and then call that API from HAL.
     Call HAL information from here */
  hal_get_index_cpu_entry (entry_num, (unsigned char *) cpu_entry); 
#endif
  return 1;
}

int nsm_get_num_cpu_entries (u_int32_t *num_entries)
{
#ifdef HAVE_HAL
  hal_get_num_cpu_entries (num_entries);
#endif
  return 1;  
}

int nsm_stacking_set_entry_master (unsigned char *macAddr)
{
#ifdef HAVE_HAL
  hal_stacking_set_entry_master (macAddr);
#endif
  return 1;  
}
#endif /* HAVE_HWSTACK */

#ifdef HAVE_SMI
/* Get Interface statistics */
int
nsm_interface_get_counters(struct interface *ifp, 
                           struct smi_if_stats *if_stats)
{
  int ret = 0;
  struct hal_if_counters hal_if_stats;

  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  pal_mem_set(&hal_if_stats, 0, sizeof(struct hal_if_counters));
  pal_mem_set(if_stats, 0, sizeof(struct smi_if_stats));

  ret = hal_if_get_counters(ifp->ifindex, &hal_if_stats);
  if (ret < 0)
    return SMI_ERROR;

  pal_mem_cpy(if_stats, &hal_if_stats, sizeof(struct smi_if_stats));

  return SMI_SUCEESS;
}

/* Clear Interface statistics */
int
nsm_interface_clear_counters (struct interface *ifp)
{
  int ret = 0;

  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ret = hal_if_stats_flush(ifp->ifindex);
  if (ret < 0)
    return SMI_ERROR;

  return SMI_SUCEESS;
}


/* Set MDI/MDIX crossover setting for the port */
int
nsm_if_mdix_crossover_set(struct interface *ifp, u_int32_t mdix)
{
  int ret = 0;

  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  ret = hal_if_set_mdix (ifp->ifindex, mdix);
  if (ret < 0)
    return SMI_ERROR;
 
  ifp->mdix = mdix;

  return SMI_SUCEESS;
}

/* Get MDI/MDIX crossover setting for the port */
int
nsm_if_mdix_crossover_get(struct interface *ifp, u_int32_t *mdix)
{
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  *mdix = ifp->mdix;
  return SMI_SUCEESS;
}

/* Delete dynamic entry from FDB with a given mac address. */
int
nsm_bridge_flush_dynamic_fdb_by_mac(char *name, char *mac_addr)
{
#ifdef HAVE_HAL
  if (hal_bridge_flush_dynamic_fdb_by_mac (name, mac_addr,
                                           HAL_HW_LENGTH) != HAL_SUCCESS)
    return SMI_ERROR;
#endif /* HAVE_HAL */
  return SMI_SUCEESS;
}

/* Get the data in traffic class table */
int
nsm_apn_get_traffic_class_table(struct interface *ifp, 
            u_char traffic_class_table[][SMI_BRIDGE_MAX_TRAFFIC_CLASS + 1])
{
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;

  if(ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  zif = (struct nsm_if *)ifp->info;

  if (zif)
  {
    if (zif->bridge)
    {
      bridge = zif->bridge;

      if ((br_port = zif->switchport) == NULL)
        return NSM_VLAN_ERR_IFP_NOT_BOUND;

      vlan_port = &br_port->vlan_port;
      if(!vlan_port)
        return NSM_VLAN_ERR_IFP_NOT_BOUND;
      if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE )
            return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;
    }
    else
      return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
  }
  else
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  pal_mem_cpy (traffic_class_table, br_port->vlan_port.traffic_class_table, 
               SMI_BRIDGE_MAX_TRAFFIC_CLASS * SMI_BRIDGE_MAX_USER_PRIO * 
               sizeof (u_char));

  return SMI_SUCEESS;
}

int
nsm_if_set_bandwidth (struct interface *ifp, float bw)
{
  int ret = 0;
  cindex_t cindex = 0;

#ifdef HAVE_LACPD
  float mem_bandwidth = 0.0;
  u_int16_t mem_count = 0;
  struct interface *memifp = NULL;
  struct listnode *lsn = NULL;
#endif /* HAVE LACP */

 struct nsm_if *zif = (struct nsm_if *)ifp->info;

#ifdef HAVE_LACPD
 /* Bandwidth Setting is allowed on Aggregator. */
  if (zif && (zif->agg.type == NSM_IF_AGGREGATOR))
    {
      ret = nsm_fea_if_set_bandwidth (ifp, bw);

      if (ret >= 0)
        {
          mem_count  = LISTCOUNT (zif->agg.info.memberlist);

          mem_bandwidth = bw / (mem_count * 1.0);

          AGGREGATOR_MEMBER_LOOP (zif, memifp, lsn)
           {
             memifp->bandwidth = mem_bandwidth;
             nsm_if_set_bandwidth (memifp,  memifp->bandwidth);
             SET_FLAG (memifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);
           }
        }
    }
  else
#endif /* HAVE_LACPD */

  ret = nsm_fea_if_set_bandwidth (ifp, bw);

  if (ret < 0)
    return SMI_ERROR;

  ifp->bandwidth = bw;

#ifdef HAVE_GMPLS
  if ((ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA) ||
     (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA_CONTROL))
    {
      if (ifp->num_dl == 1)
        {
          nsm_data_link_property_copy (ifp, PAL_TRUE);
        }
      else
        {
          return NSM_FAILURE;
        }
    }
#endif /* */
  SET_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);

  /* Force protocols to recalculate routes due to cost change. */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);

  return SMI_SUCEESS;
}

int
nsm_if_non_conf_set (struct interface *ifp, enum smi_port_conf_state state)
{
  if (state == SMI_PORT_ENABLE)
    SET_FLAG (ifp->status, IF_NON_CONFIGURABLE_FLAG);
  else
    UNSET_FLAG (ifp->status, IF_NON_CONFIGURABLE_FLAG);

  return SMI_SUCEESS;
}

int
nsm_if_non_conf_get (struct interface *ifp, enum smi_port_conf_state *state)
{
  if (CHECK_FLAG (ifp->status, IF_NON_CONFIGURABLE_FLAG))
    *state = SMI_PORT_ENABLE;
  else
    *state = SMI_PORT_DISABLE;

  return SMI_SUCEESS;
}

int
nsm_api_get_port_non_switch (struct interface *ifp, 
                             enum smi_port_conf_state *state)
{
  struct nsm_bridge_port *br_port;
  struct nsm_if *zif = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (!zif)
    return SMI_ERROR;

  if ((br_port = zif->switchport) == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  *state = br_port->spanning_tree_disable;

  return SMI_SUCEESS;
}

#endif /* HAVE_SMI */
