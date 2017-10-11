/* Copyright (C) 2005  All Rights Reserved. */

#include "pal.h"
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
#include "lib.h"
#include "ptree.h"
#include "avl_tree.h"
#include "thread.h"
#include "table.h"
#include "stream.h"
#include "hash.h"
#ifdef HAVE_IGMP_SNOOP
#include "igmp.h"
#endif /* HAVE_IGMP_SNOOP */
#ifdef HAVE_MLD_SNOOP
#include "mld.h"
#endif /* HAVE_MLD_SNOOP */

#include "nsmd.h"
#include "nsm_interface.h"
#include "nsm_server.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#ifdef HAVE_PROVIDER_BRIDGE
#include "nsm_pro_vlan.h"
#endif /* HAVE_PROVIDER_BRIDGE */
#include "nsm_l2_mcast.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifdef HAVE_TUNNEL
#include "nsm/tunnel/nsm_tunnel.h"
#endif

/*

IGMP/MLD SNOOPING

This module contains the initialization routines

*/

nsm_l2_mcast_action_func_t nsm_l2_mcast_action_func [NSM_L2_MCAST_FMS_MAX] =
{
  nsm_l2_mcast_action_invalid,
  nsm_l2_mcast_action_include,
  nsm_l2_mcast_action_exclude
};

/* IGMP IF Source Record Create */
s_int32_t
nsm_l2_mcast_srec_create (struct nsm_l2_mcast_grec *ngr,
                          struct nsm_l2_mcast_source_rec **pp_nsr)
{
  s_int32_t ret;

  ret = 0;

  if (ngr == NULL
      || pp_nsr == NULL)
    return NSM_FAILURE;

  (*pp_nsr) = XCALLOC (MTYPE_NSM_L2_MCAST_SRC_REC,
                       sizeof (struct nsm_l2_mcast_source_rec));
  if ((*pp_nsr) == NULL)
    return NSM_FAILURE;

  (*pp_nsr)->owning_ngr = ngr;
  SET_FLAG ((*pp_nsr)->flags, NSM_ISR_SFLAG_VALID);

  return 0;

}

s_int32_t
nsm_l2_mcast_srec_delete (struct nsm_l2_mcast_source_rec *nsr)
{
  /* Release the owning P-Tree Node */
  if (nsr->owning_pn != NULL)
    {
      nsr->owning_pn->info = NULL;
      ptree_unlock_node (nsr->owning_pn);
      nsr->owning_pn = NULL;
    }

  nsr->owning_ngr->num_srcs -= 1;

  /* Release the Source Rec */
  XFREE (MTYPE_NSM_L2_MCAST_SRC_REC, nsr);

  return 0;

}

struct nsm_l2_mcast_source_rec *
nsm_l2_mcast_srec_get (struct nsm_l2_mcast_grec *ngr,
                       void_t *msrc)
{
  struct nsm_l2_mcast_source_rec *nsr;
  struct ptree_node *pn;
  s_int32_t ret;

  nsr = NULL;

  if (ngr == NULL
      || msrc == NULL)
    return NULL;

  if (ngr->family == AF_INET)
    pn = ptree_node_get (ngr->nsm_l2_mcast_src_tib,
                        (u_int8_t *) msrc, IPV4_MAX_PREFIXLEN);
#ifdef HAVE_MLD_SNOOP
  else if (ngr->family == AF_INET6)
    pn = ptree_node_get (ngr->nsm_l2_mcast_src_tib,
                        (u_int8_t *) msrc, IPV6_MAX_PREFIXLEN);
#endif /* HAVE_MLD_SNOOP */
  else
    return NULL;

  if (pn == NULL)
    return NULL;

  if (! (nsr = pn->info))
    {
      ret = nsm_l2_mcast_srec_create (ngr, &nsr);

      if (ret < 0)
        return NULL;

      pn->info = nsr;
      nsr->owning_pn = pn;

      ngr->num_srcs += 1;
    }
  else
    ptree_unlock_node (pn);

  return nsr;

}

struct nsm_l2_mcast_source_rec *
nsm_l2_mcast_srec_lookup (struct nsm_l2_mcast_grec *ngr,
                          void_t *msrc)
{
  struct nsm_l2_mcast_source_rec *nsr;
  struct ptree_node *pn;

  nsr = NULL;

  if (ngr == NULL
      || msrc == NULL)
    return NULL;

  if (ngr->family == AF_INET)
    pn = ptree_node_lookup (ngr->nsm_l2_mcast_src_tib,
                           (u_int8_t *) msrc,
                           IPV4_MAX_PREFIXLEN);
#ifdef HAVE_MLD_SNOOP
  else if (ngr->family == AF_INET6)
    pn = ptree_node_lookup (ngr->nsm_l2_mcast_src_tib,
                           (u_int8_t *) msrc,
                           IPV6_MAX_PREFIXLEN);
#endif /*HAVE_MLD_SNOOP*/
  else
   return NULL;

  if (pn)
    {
      nsr = pn->info;
      ptree_unlock_node (pn);
    }

  return nsr;
}

#ifdef HAVE_IGMP_SNOOP

/*
 * Add/Delete mrouter interface to all group entries in forwarder 
 */
s_int32_t
nsm_l2_mcast_igmp_add_del_mrt_entry (struct nsm_bridge_master *master,
                                     char *bridge_name,
                                     struct interface *ifp,
                                     u_int32_t su_id,
                                     bool_t is_exclude)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  struct igmp_if *ds_igif;
  struct listnode *nn;
  struct nsm_bridge *bridge;
  struct nsm_vlan *vlan;
  struct nsm_l2_mcast_grec *ngr;
  struct nsm_l2_mcast_grec_key grec_key;
  struct ptree_node *pn;
  struct ptree_node *pn_next;
  struct nsm_if *zif;
  struct pal_in4_addr *grp_addr;
  struct pal_in4_addr src_addr;
  u_int16_t vlanid;
  u_int16_t svlanid;

  if (! master || ! master->l2mcast || ! master->l2mcast->igmp_inst || !ifp)
    return NSM_ERR_INTERNAL;

  ngr = NULL;
  pn = NULL;
  pn_next = NULL;
  zif = NULL;
  grp_addr = NULL;

  pal_mem_set (&src_addr, 0, sizeof (struct pal_in4_addr));

  vlanid = NSM_L2_MCAST_GET_CVID (su_id);
  svlanid = NSM_L2_MCAST_GET_SVID (su_id);

  if (! bridge_name)
    bridge = nsm_get_default_bridge (master);
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_ERR_INTERNAL;

  vlan = nsm_vlan_find (bridge, vlanid);

  if (! vlan || ! vlan->ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = vlan->ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_lookup (master->l2mcast->igmp_inst, &igifidx);

  if (! igif)
    return NSM_ERR_INTERNAL;

  LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
    {
      if ( ! ds_igif || ! ds_igif->igif_owning_ifp || ! ds_igif->igif_owning_ifp->info)
        continue;

      zif = ds_igif->igif_owning_ifp->info;
       
      for (pn = ptree_top (zif->l2mcastif.igmpsnp_gmr_tib); pn; pn = pn_next)
        {
          pn_next = ptree_next (pn);

          if (! (ngr = pn->info))
            continue;

          pal_mem_cpy (&grec_key, PTREE_NODE_KEY (ngr->owning_pn),
                       NSM_L2_MCAST_IPV4_PREFIXLEN / 8);

          grp_addr = &grec_key.addr;

          if ( PAL_TRUE  == is_exclude )
             hal_igmp_snooping_delete_entry (bridge_name,
                                             (struct hal_in4_addr *) &src_addr,
                                             (struct hal_in4_addr *) grp_addr,
                                             PAL_FALSE,
                                             vlanid,
                                             svlanid,
                                             1,
                                             &ifp->ifindex);
          else
             hal_igmp_snooping_add_entry (bridge_name,
                                          (struct hal_in4_addr *) &src_addr,
                                          (struct hal_in4_addr *) grp_addr,
                                          PAL_FALSE,
                                          vlanid,
                                          svlanid,
                                          1,
                                          &ifp->ifindex);
       }
    }
  return NSM_SUCCESS;
}

/* Add all mrouter interfaces to group entry in forwarder */
s_int32_t
nsm_l2_mcast_igmp_grp_add_mrt_if (struct nsm_bridge_master *master,
                                  char *bridge_name,
                                  void_t *pgrp,
                                  u_int32_t su_id)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  struct igmp_if *ds_igif;
  struct listnode *nn;
  struct nsm_bridge *bridge;
  struct nsm_vlan *vlan;
  struct pal_in4_addr src_addr;
  u_int16_t vlanid;
  u_int16_t svlanid;

  if (! master || ! master->l2mcast || ! master->l2mcast->igmp_inst)
   return NSM_ERR_INTERNAL;

  ds_igif = NULL;
  igif = NULL;
  nn = NULL;

  pal_mem_set (&src_addr, 0, sizeof (struct pal_in4_addr));

  vlanid = NSM_L2_MCAST_GET_CVID (su_id);
  svlanid = NSM_L2_MCAST_GET_SVID (su_id);

  if (! bridge_name)
    bridge = nsm_get_default_bridge (master);
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_ERR_INTERNAL;

  vlan = nsm_vlan_find (bridge, vlanid);

  if (! vlan || ! vlan->ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = vlan->ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_lookup (master->l2mcast->igmp_inst, &igifidx);
 
  if (! igif)
     return NSM_ERR_INTERNAL;

  LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
    if (CHECK_FLAG (ds_igif->igif_sflags,
                    IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG)
        || CHECK_FLAG (ds_igif->igif_sflags,
                       IGMP_IF_SFLAG_SNOOP_MROUTER_IF))
      hal_igmp_snooping_add_entry (bridge_name,
                                   (struct hal_in4_addr *) &src_addr,
                                   (struct hal_in4_addr *) pgrp,
                                   PAL_FALSE,
                                   vlanid,
                                   svlanid,
                                   1,
                                   &ds_igif->igif_owning_ifp->ifindex);

  return NSM_SUCCESS;
}

/* Delete all mrouter interfaces from group entry in forwarder */
s_int32_t
nsm_l2_mcast_igmp_grp_del_mrt_if (struct nsm_bridge_master *master,
                                  char *bridge_name,
                                  void_t *pgrp,
                                  u_int32_t su_id)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;
  struct igmp_if *ds_igif;
  struct listnode *nn;
  struct nsm_bridge *bridge;
  struct nsm_vlan *vlan;
  struct pal_in4_addr src_addr;
  u_int16_t vlanid;
  u_int16_t svlanid;

  if (! master || ! master->l2mcast || ! master->l2mcast->igmp_inst)
    return NSM_ERR_INTERNAL;

  ds_igif = NULL;
  igif = NULL;
  nn = NULL;

  pal_mem_set (&src_addr, 0, sizeof (struct pal_in4_addr));

  vlanid = NSM_L2_MCAST_GET_CVID (su_id);
  svlanid = NSM_L2_MCAST_GET_SVID (su_id);

  if (! bridge_name)
    bridge = nsm_get_default_bridge (master);
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
     return NSM_ERR_INTERNAL;

  vlan = nsm_vlan_find (bridge, vlanid);

  if (! vlan || ! vlan->ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = vlan->ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_lookup (master->l2mcast->igmp_inst, &igifidx);

  if (! igif)
    return NSM_ERR_INTERNAL;

  LIST_LOOP (&igif->igif_hst_dsif_lst, ds_igif, nn)
    if (CHECK_FLAG (ds_igif->igif_sflags,
                    IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG)
        || CHECK_FLAG (ds_igif->igif_sflags,
                         IGMP_IF_SFLAG_SNOOP_MROUTER_IF))
       hal_igmp_snooping_delete_entry (bridge_name,
                                       (struct hal_in4_addr *) &src_addr,
                                       (struct hal_in4_addr *) pgrp,
                                       PAL_FALSE,
                                       vlanid,
                                       svlanid,
                                       1,
                                       &ds_igif->igif_owning_ifp->ifindex);

  return NSM_SUCCESS;
}
#endif /* HAVE_IGMP_SNOOP */


s_int32_t
nsm_l2_mcast_grec_create (struct nsm_if *zif,
                          struct nsm_l2_mcast_grec **pp_ngr,
                          u_int32_t su_id,
                          void_t *pgrp,
                          u_int8_t family)
{
  s_int32_t ret;
  u_int16_t vid;
  u_int16_t svid;
  struct ptree_node *pn;
  u_int8_t mac [ETHER_ADDR_LEN];
  struct nsm_l2_mcast_grec_key grec_key;

  ret = NSM_SUCCESS;
  grec_key.su_id = su_id;

  vid = NSM_L2_MCAST_GET_VID (su_id);
  svid = NSM_L2_MCAST_GET_SVID (su_id);

  if (zif == NULL
      || pp_ngr == NULL 
      || zif->bridge == NULL
      || zif->bridge->master == NULL)
    {
      return NSM_FAILURE;
    }

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    IGMP_CONVERT_IPV4MCADDR_TO_MAC (pgrp, mac);
#ifdef HAVE_MLD_SNOOP
  else if (family == AF_INET6)
    MLD_CONVERT_IPV6MCADDR_TO_MAC (pgrp, mac);
#endif /* HAVE_MLD_SNOOP */
#else
#ifdef HAVE_MLD_SNOOP
  if (family == AF_INET6)
    MLD_CONVERT_IPV6MCADDR_TO_MAC (pgrp, mac);
#endif /* HAVE_MLD_SNOOP */
#endif /* HAVE_IGMP_SNOOP */

  (*pp_ngr) = XCALLOC (MTYPE_NSM_L2_MCAST_GRP_REC,
                       sizeof (struct nsm_l2_mcast_grec));
  if (! (*pp_ngr))
    {
      return NSM_FAILURE;
    }

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    {
      pal_mem_cpy (&grec_key.addr, pgrp, sizeof (struct pal_in4_addr));
      pn = ptree_node_get (zif->l2mcastif.igmpsnp_gmr_tib,
                           (u_int8_t *) &grec_key,
                           NSM_L2_MCAST_IPV4_PREFIXLEN);
      if (pn == NULL)
        return NSM_ERR_INTERNAL;
    }
#ifdef HAVE_MLD_SNOOP
  else if (family == AF_INET6)
    {
      pal_mem_cpy (&grec_key.addr, pgrp, sizeof (struct pal_in6_addr));
      pn = ptree_node_get (zif->l2mcastif.mldsnp_gmr_tib,
                           (u_int8_t *) &grec_key,
                           NSM_L2_MCAST_IPV6_PREFIXLEN);
      if (pn == NULL)
        return NSM_ERR_INTERNAL;
   }
#endif /* HAVE_MLD_SNOOP */
  else
    return NSM_ERR_INTERNAL;
#else
#ifdef HAVE_MLD_SNOOP
  if (family == AF_INET6)
    {
      pal_mem_cpy (&grec_key.addr, pgrp, sizeof (struct pal_in6_addr));
      pn = ptree_node_get (zif->l2mcastif.mldsnp_gmr_tib,
                           (u_int8_t *) &grec_key,
                           NSM_L2_MCAST_IPV6_PREFIXLEN);
      if (pn == NULL)
        return NSM_ERR_INTERNAL;
   }
  else
    return NSM_ERR_INTERNAL;
#endif /* HAVE_MLD_SNOOP */
#endif /* HAVE_IGMP_SNOOP */


  (*pp_ngr)->owning_if = zif;
  (*pp_ngr)->filt_mode = NSM_L2_MCAST_FMS_INCLUDE;
  (*pp_ngr)->num_srcs = 0;
  if (family == AF_INET)
    (*pp_ngr)->nsm_l2_mcast_src_tib = ptree_init (IPV4_MAX_PREFIXLEN);
#ifdef HAVE_MLD_SNOOP
  else if (family == AF_INET6)
    (*pp_ngr)->nsm_l2_mcast_src_tib = ptree_init (IPV6_MAX_PREFIXLEN);
#endif /* HAVE_MLD_SNOOP */
  else
    return NSM_ERR_INTERNAL;

  (*pp_ngr)->family = family;
  (*pp_ngr)->owning_if = zif;
  (*pp_ngr)->owning_pn = pn;
  pn->info = *pp_ngr;

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    {
      if (nsm_bridge_is_vlan_aware (zif->bridge->master, zif->bridge->name)
                                   == PAL_TRUE)
        nsm_bridge_add_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                            mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                            IGMP_SNOOPING_MAC);
      else
        nsm_bridge_add_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                            mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                            IGMP_SNOOPING_MAC);
    }
#ifdef HAVE_MLD_SNOOP
  else if (family == AF_INET6)
    {
      if (nsm_bridge_is_vlan_aware (zif->bridge->master, zif->bridge->name)
                                  == PAL_TRUE)
        nsm_bridge_add_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                            mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                            MLD_SNOOPING_MAC);
      else
        nsm_bridge_add_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                            mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                            MLD_SNOOPING_MAC);
    }
#endif /* HAVE_MLD_SNOOP */
  else
    return NSM_ERR_INTERNAL;
#else
#ifdef HAVE_MLD_SNOOP
  if (family == AF_INET6)
    {
      if (nsm_bridge_is_vlan_aware (zif->bridge->master, zif->bridge->name)
                                  == PAL_TRUE)
        nsm_bridge_add_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                            mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                            MLD_SNOOPING_MAC);
      else
        nsm_bridge_add_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                            mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                            MLD_SNOOPING_MAC);          
    }
  else
    return NSM_ERR_INTERNAL;
#endif /* HAVE_MLD_SNOOP */
#endif /* HAVE_IGMP_SNOOP */

  return ret;

}

void
nsm_l2_mcast_grec_delete (struct nsm_l2_mcast_grec *ngr)
{
  struct nsm_l2_mcast_grec_key grec_key;
  struct nsm_l2_mcast_source_rec *nsr;
  u_int8_t mac [ETHER_ADDR_LEN];
  struct ptree_node *pn_next;
  struct ptree_node *pn;
  struct nsm_if *zif;
  u_int16_t svid;
  u_int16_t vid;
  u_int8_t family;

  pal_mem_set (&grec_key, 0, sizeof (struct nsm_l2_mcast_grec_key));
  family = ngr->family;

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    {
      pal_mem_cpy (&grec_key, PTREE_NODE_KEY (ngr->owning_pn),
                   NSM_L2_MCAST_IPV4_PREFIXLEN / 8);
      IGMP_CONVERT_IPV4MCADDR_TO_MAC (&grec_key.addr, mac);
    }
#ifdef HAVE_MLD_SNOOP
  else if (family == AF_INET6)
    {
      pal_mem_cpy (&grec_key, PTREE_NODE_KEY (ngr->owning_pn),
                   NSM_L2_MCAST_IPV6_PREFIXLEN / 8);
      MLD_CONVERT_IPV6MCADDR_TO_MAC (&grec_key.addr, mac);
    }
#endif /* HAVE_MLD_SNOOP */
#else
#ifdef HAVE_MLD_SNOOP
  if (family == AF_INET6)
    {
      pal_mem_cpy (&grec_key, PTREE_NODE_KEY (ngr->owning_pn),
                   NSM_L2_MCAST_IPV6_PREFIXLEN / 8);
      MLD_CONVERT_IPV6MCADDR_TO_MAC (&grec_key.addr, mac);
    }
#endif  /* HAVE_MLD_SNOOP */
#endif  /* HAVE_IGMP_SNOOP */

  vid = NSM_L2_MCAST_GET_VID (grec_key.su_id);
  svid = NSM_L2_MCAST_GET_SVID (grec_key.su_id);

  zif = ngr->owning_if;

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    if (zif && zif->bridge && zif->bridge->master && zif->bridge->name)
      nsm_l2_mcast_igmp_grp_del_mrt_if (zif->bridge->master,
                                        zif->bridge->name,
                                        (void_t *) &grec_key.addr,
                                        grec_key.su_id);
#endif /* HAVE_IGMP_SNOOP */


  for (pn = ptree_top (ngr->nsm_l2_mcast_src_tib); pn; pn = pn_next)
    {
      pn_next = ptree_next (pn);

      if (! (nsr = pn->info))
        continue;

      nsm_l2_mcast_srec_delete (nsr);

      ngr->num_srcs -= 1;
    }
  ptree_finish (ngr->nsm_l2_mcast_src_tib);

  ngr->nsm_l2_mcast_src_tib = NULL;
  ngr->owning_if = NULL;
  ngr->owning_pn->info = NULL;
  ngr->owning_pn = NULL;

  XFREE (MTYPE_NSM_L2_MCAST_GRP_REC, ngr);

  if (zif == NULL
      || zif->bridge == NULL
      || zif->bridge->master == NULL)
    return;

  if (family == AF_INET)
    {
      if (nsm_bridge_is_vlan_aware (zif->bridge->master, zif->bridge->name)
                                  == PAL_TRUE)
        nsm_bridge_delete_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                               mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                               IGMP_SNOOPING_MAC);
      else
        nsm_bridge_delete_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                               mac, NSM_VLAN_NULL_VID, NSM_VLAN_NULL_VID,
                               HAL_L2_FDB_STATIC, PAL_TRUE, IGMP_SNOOPING_MAC);
    }
  else
    {
      if (nsm_bridge_is_vlan_aware (zif->bridge->master, zif->bridge->name)
                                  == PAL_TRUE)
        nsm_bridge_delete_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                               mac, vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE,
                               MLD_SNOOPING_MAC);
      else
        nsm_bridge_delete_mac (zif->bridge->master, zif->bridge->name, zif->ifp,
                               mac, NSM_VLAN_NULL_VID, NSM_VLAN_NULL_VID,
                               HAL_L2_FDB_STATIC, PAL_TRUE, MLD_SNOOPING_MAC);
    }

  return;

}

struct nsm_l2_mcast_grec *
nsm_l2_mcast_grec_lookup (struct nsm_if *zif,
                          u_int32_t su_id,
                          void_t *pgrp,
                          u_int8_t family)
{
  struct nsm_l2_mcast_grec_key grec_key;
  struct nsm_l2_mcast_grec *ngr;
  struct ptree_node *pn;

  if (! zif || ! pgrp)
    return NULL;

  pn = NULL;
  ngr = NULL;
  grec_key.su_id = su_id;

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    {
      pal_mem_cpy (&grec_key.addr, pgrp, sizeof (struct pal_in4_addr));
      pn = ptree_node_lookup (zif->l2mcastif.igmpsnp_gmr_tib,
                              (u_int8_t *) &grec_key,
                              NSM_L2_MCAST_IPV4_PREFIXLEN);
    }
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  if (family == AF_INET6)
    {
      pal_mem_cpy (&grec_key.addr, pgrp, sizeof (struct pal_in6_addr));
      pn = ptree_node_lookup (zif->l2mcastif.mldsnp_gmr_tib,
                              (u_int8_t *) &grec_key,
                              NSM_L2_MCAST_IPV6_PREFIXLEN);
    }
#endif /* HAVE_MLD_SNOOP */

  if (pn)
    {
      ngr = pn->info;
      ptree_unlock_node (pn);
    }

  return ngr;
}

struct nsm_l2_mcast_grec *
nsm_l2_mcast_grec_get (struct nsm_if *zif,
                       char *bridgename,
                       u_int32_t su_id,
                       void_t *pgrp,
                       u_int8_t family)
{
  struct nsm_l2_mcast_grec_key grec_key;
  struct nsm_l2_mcast_grec *ngr;
  struct ptree_node *pn;
  s_int32_t ret;

  ret = 0;
  ngr = NULL;
  pn = NULL;

  grec_key.su_id = su_id;

  if (! zif || ! pgrp)
    return NULL;

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    {
      pal_mem_cpy (&grec_key.addr, pgrp, sizeof (struct pal_in4_addr));
      pn = ptree_node_get (zif->l2mcastif.igmpsnp_gmr_tib,
                           (u_int8_t *) &grec_key,
                           NSM_L2_MCAST_IPV4_PREFIXLEN);
    }
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  if (family == AF_INET6)
    {
      pal_mem_cpy (&grec_key.addr, pgrp, sizeof (struct pal_in6_addr));
      pn = ptree_node_get (zif->l2mcastif.mldsnp_gmr_tib,
                           (u_int8_t *) &grec_key,
                           NSM_L2_MCAST_IPV6_PREFIXLEN);
    }
#endif /* HAVE_MLD_SNOOP */

  if (! pn)
    return NULL;

  if (! (ngr = pn->info))
    {
      ret = nsm_l2_mcast_grec_create (zif, &ngr, su_id, pgrp, family);

      if (ret < 0)
        {
          if (ngr)
            XFREE(MTYPE_NSM_L2_MCAST_GRP_REC, ngr);
          ptree_unlock_node(pn);
          return NULL;
        }
      ngr->owning_pn = pn;
      pn->info = ngr;

#ifdef HAVE_IGMP_SNOOP
      if (family == AF_INET)
        if ( zif && zif->bridge && zif->bridge->master)
          nsm_l2_mcast_igmp_grp_add_mrt_if (zif->bridge->master,
                                            bridgename,
                                            pgrp,
                                            su_id);
#endif /* HAVE_IGMP_SNOOP */
    }
  else
    ptree_unlock_node (pn);

  return ngr;
}

s_int32_t
nsm_l2_mcast_process_event (struct nsm_bridge_master *master,
                            char *bridgename,
                            u_int8_t family,
                            void_t *pgrp,
                            u_int32_t su_id,
                            struct interface *ifp,
                            u_int16_t num_srcs,
                            void_t *src_addr_list,
                            enum nsm_l2mcast_filter_mode_state filt_mode)
{
  struct nsm_l2_mcast_grec *ngr;
  u_int8_t mac [ETHER_ADDR_LEN];
  s_int32_t ret;

  ret = NSM_SUCCESS;

#ifdef HAVE_IGMP_SNOOP
  if (family == AF_INET)
    IGMP_CONVERT_IPV4MCADDR_TO_MAC (pgrp, mac);
#ifdef HAVE_MLD_SNOOP
  else if (family == AF_INET6)
    MLD_CONVERT_IPV6MCADDR_TO_MAC (pgrp, mac);
#endif /* HAVE_MLD_SNOOP */
  else
    return NSM_ERR_INTERNAL;
#else
#ifdef HAVE_MLD_SNOOP
  if (family == AF_INET6)
    MLD_CONVERT_IPV6MCADDR_TO_MAC (pgrp, mac);
  else
    return NSM_ERR_INTERNAL;
#endif /* HAVE_MLD_SNOOP */
#endif /* HAVE_IGMP_SNOOP */


  ngr = nsm_l2_mcast_grec_get (ifp->info, bridgename, su_id, pgrp, family);

  nsm_l2_mcast_action_func [ngr->filt_mode]
                                 (bridgename,
                                  family,
                                  pgrp,
                                  su_id,
                                  ifp,
                                  num_srcs,
                                  src_addr_list,
                                  filt_mode,
                                  ngr);


  return ret;
}

static s_int32_t
nsm_l2_mcast_check_and_disable_snpif (struct nsm_bridge_master *master,
                                      char *bridge_name, u_int16_t vid,
                                      u_int16_t svid)
{
  s_int32_t ret;
  u_int32_t brno;
  u_int32_t su_id;
  struct nsm_if *zif;
  struct listnode *ln;
  struct avl_node *rn;
  struct nsm_vlan nvlan;
  struct nsm_master *nm;
  struct nsm_vlan *vlan;
  struct interface *ifp;
  struct list *port_list;
  bool_t l2mcast_disable;
  struct interface *p_ifp;
  struct interface *tmpifp;
  struct nsm_bridge *bridge;
  struct nsm_l2_mcast *l2mcast;
  struct avl_tree *vlan_table;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */

  if (! master || !master->l2mcast)
    return -1;

  ret = 0;
  zif = NULL;
  nm = master->nm;
  ifp = NULL;
  vlan = NULL;
#ifdef HAVE_PROVIDER_BRIDGE
  ctx = NULL;
#endif /* HAVE_PROVIDER_BRiDE */

  vlan_table = NULL;
  port_list = NULL;
  l2mcast = master->l2mcast;
  l2mcast_disable = PAL_TRUE;
  pal_sscanf (bridge_name, "%u", &brno);

  su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, svid);

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (!bridge)
    return -1;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);
  else if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    vlan_table = bridge->vlan_table;

#ifdef HAVE_PROVIDER_BRIDGE
   if (bridge->provider_edge == PAL_TRUE)
     {
       if (ctx)
         port_list = ctx->port_list;
     }
   else
#endif /* HAVE_PROVIDER_BRIDGE */
     {
       if (vlan_table == NULL)
         return -1;

       NSM_VLAN_SET (&nvlan, vid);

       rn = avl_search (vlan_table, (void *)&nvlan);

       if (! rn || !rn->info )
         return -1;

       vlan = rn->info;

       if (vlan)
         port_list = vlan->port_list;
     }

  LIST_LOOP (port_list, tmpifp, ln)
    {
      if (! tmpifp || ! tmpifp->info)
        continue;

      zif = tmpifp->info;

      if (! zif->switchport)
        continue;

      br_port = zif->switchport; 

      if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
          l2mcast_disable = PAL_FALSE;
          break;
        }
    }

  if (l2mcast_disable)
    {
#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->provider_edge == PAL_TRUE)
        {
          if (ctx)
            ifp = ctx->ifp;
        }
      else
#endif /* HAVE_PROVIDER_BRIDGE */
        ifp = vlan->ifp;

      p_ifp = ifp;

      if (! ifp)
        return -1;

#ifdef HAVE_IGMP_SNOOP
      ret = nsm_l2_mcast_igmp_if_disable (master->l2mcast, ifp,
                                          p_ifp, su_id);
      if (ret < 0)
        return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      ret = nsm_l2_mcast_mld_if_disable (master->l2mcast, ifp,
                                         p_ifp, su_id);
      if (ret < 0)
        return ret;
#endif /* HAVE_MLD_SNOOP */
    }

  return 0;
}

s_int32_t
nsm_l2_mcast_add_port_cb (struct nsm_bridge_master *master,
                          char *bridge_name,
                          struct interface *ifp)
{
  /*
   * No Operation. Add IF only when vlan is added to port
   */

  return NSM_SUCCESS;
}

s_int32_t
nsm_l2_mcast_delete_port (struct nsm_bridge_master *master,
                          char *bridge_name,
                          struct interface *ifp)
{
  u_int16_t vid;
  s_int32_t ret;
  u_int32_t brno;
  u_int32_t su_id;
  struct nsm_if *zif;
#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
  struct avl_node *rn;
  struct nsm_vlan nvlan;
  struct nsm_vlan *vlan;
  struct interface *vlanifp;
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */
  struct nsm_master *nm;
  struct nsm_bridge *bridge;
  struct nsm_l2_mcast *l2mcast;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;

  if (! master || ! master->l2mcast || ! master->nm || ! ifp || ! ifp->info)
    return -1;

  ret = 0;
  nm = master->nm;
  zif = ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return -1;

  pal_sscanf (bridge_name, "%u", &brno);
  br_port = zif->switchport;
  l2mcast = master->l2mcast;
  vlan_port = &br_port->vlan_port;
  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return 0;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return 0;

  /* Delete all the igif corresponding to the ports for
   * all vlans (dynamic as well as static).
   */
  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
    {
      su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, vid);

#ifdef HAVE_IGMP_SNOOP
      ret = nsm_l2_mcast_igmp_if_delete (l2mcast, ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface deletion for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      ret = nsm_l2_mcast_mld_if_delete (l2mcast, ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface deletion for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_MLD_SNOOP */

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || rn->info )
        continue;

      vlan = rn->info;

      if (listcount (vlan->port_list) == 1)
        {
          vlanifp = vlan->ifp;

          if (! vlanifp)
            return -1;

          /* Vlan interface administratively/operationally down*/
          vlanifp->flags &= ~ (IFF_UP | IFF_RUNNING);

#ifdef HAVE_IGMP_SNOOP
          ret = nsm_l2_mcast_igmp_if_update (master->l2mcast, vlanifp,
                                             su_id);
          if (ret < 0)
            return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
          ret = nsm_l2_mcast_mld_if_update (master->l2mcast, vlanifp,
                                            su_id);
          if (ret < 0)
            return ret;
#endif /* HAVE_MLD_SNOOP */
        }
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */
    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);

  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->dynamicMemberBmp, vid)
    {
      su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, vid);

#ifdef HAVE_IGMP_SNOOP
      ret = nsm_l2_mcast_igmp_if_delete (l2mcast, ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface deletion for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
       ret = nsm_l2_mcast_mld_if_delete (l2mcast, ifp, su_id);

       if (ret < 0)
         zlog_warn (nm->zg, "MLD Interface deletion for %s vid %d not successful",
                             ifp->name, vid);
#endif /* HAVE_MLD_SNOOP */

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || !rn->info )
        continue;

      vlan = rn->info;

      if (listcount (vlan->port_list) == 1)
        {
          vlanifp = vlan->ifp; 

          if (! vlanifp)
            return -1;

          /* Vlan interface administratively/ operationally down*/
          vlanifp->flags &= ~ (IFF_UP | IFF_RUNNING);

#ifdef HAVE_IGMP_SNOOP
          ret = nsm_l2_mcast_igmp_if_update (master->l2mcast, vlanifp,
                                             su_id);
          if (ret < 0)
            return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
          ret = nsm_l2_mcast_mld_if_update (master->l2mcast, vlanifp,
                                            su_id);
          if (ret < 0)
            return ret;
#endif /* HAVE_MLD_SNOOP */
        }
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */
    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->dynamicMemberBmp, vid);

  return 0;
}

s_int32_t
nsm_l2_mcast_enable_port (struct nsm_bridge_master *master,
                          struct interface *ifp)
{
  u_int16_t vid;
  s_int32_t ret;
  u_int32_t brno;
  u_int32_t su_id;
  struct nsm_if *zif;
  struct avl_node *rn;
  struct nsm_vlan nvlan;
  struct nsm_vlan *vlan;
  struct nsm_master *nm;
  struct interface *p_ifp;
  struct interface *vlanifp;
  struct nsm_bridge *bridge;
  struct nsm_l2_mcast *l2mcast;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;

  if (! master || ! master->l2mcast || ! master->nm || ! ifp || ! ifp->info)
    return -1;

  ret = 0;
  vlanifp = NULL;
  p_ifp = NULL;
  nm = master->nm;
  zif = ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return -1;

  br_port = zif->switchport;
  l2mcast = master->l2mcast;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (! bridge)
    return 0;


  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return 0;

  pal_sscanf (bridge->name, "%u", &brno);

  /* Call igmp_if_start for all the igif corresponding to
   * all the vlans of the port.
   */

  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
    {
      su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, vid);

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || !rn->info )
        continue;

      vlan = rn->info;

      vlanifp = vlan->ifp;

      if (! vlanifp)
        continue;

      p_ifp = vlanifp;

#ifdef HAVE_IGMP_SNOOP
      /* Enable L2MCAST for VLAN IF*/
      ret = nsm_l2_mcast_igmp_if_enable (l2mcast, vlanifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d not successful",
                            vlanifp->name, vid);

      /* Enable L2MCAST for constituent IF*/
      ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      /* Enable L2MCAST for VLAN IF*/
      ret = nsm_l2_mcast_mld_if_enable (l2mcast, vlanifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface enable for %s vid %d not successful",
                            vlanifp->name, vid);

      /* Enable L2MCAST for constituent IF*/
      ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface enable for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_MLD_SNOOP */

#ifdef HAVE_IGMP_SNOOP

      ret = hal_igmp_snooping_if_enable (bridge->name, ifp->ifindex);

#endif /* HAVE_IGMP_SNOOP */
#ifdef HAVE_MLD_SNOOP

      ret = hal_mld_snooping_if_enable (bridge->name, ifp->ifindex);

#endif /* HAVE_MLD_SNOOP */

      if (ret < 0)
        return ret;

    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);

  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->dynamicMemberBmp, vid)
    {
      su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, vid);

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || !rn->info)
        continue;

      vlan = rn->info;

      vlanifp = vlan->ifp;

      if (! vlanifp)
        continue;

#ifdef HAVE_IGMP_SNOOP
      /* Enable L2MCAST for VLAN IF*/
      ret = nsm_l2_mcast_igmp_if_enable (l2mcast, vlanifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d not successful",
                            vlanifp->name, vid);

      /* Enable L2MCAST for constituent IF*/
      ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      /* Enable L2MCAST for VLAN IF*/
      ret = nsm_l2_mcast_mld_if_enable (l2mcast, vlanifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface enable for %s vid %d not successful",
                            vlanifp->name, vid);

      /* Enable L2MCAST for constituent IF*/
      ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface enable for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_MLD_SNOOP */

    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->dynamicMemberBmp, vid);

  return 0;
}

s_int32_t
nsm_l2_mcast_disable_port (struct nsm_bridge_master *master,
                           struct interface *ifp)
{
  u_int16_t vid;
  s_int32_t ret;
  u_int32_t brno;
  u_int32_t su_id;
  struct nsm_if *zif;
  struct avl_node *rn;
  struct nsm_master *nm;
  struct nsm_vlan *vlan;
  struct nsm_vlan nvlan;
  struct interface *p_ifp;
  struct interface *vlanifp;
  struct nsm_bridge *bridge;
  struct avl_tree *vlan_table;
  struct nsm_l2_mcast *l2mcast;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;

  if (! master || ! master->l2mcast || ! master->nm || ! ifp || ! ifp->info)
    return -1;

  ret = 0;
  l2mcast = master->l2mcast;
  zif = ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return -1;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  nm = master->nm;
  vlanifp = NULL;
  bridge = zif->bridge; 

  if (! bridge)
    return 0;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return 0;

  pal_sscanf (bridge->name, "%u", &brno);

  /* Call igmp_if_stop for all the igif corresponding to
   * all the vlans of the port.
   */

  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
    {
      su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, vid);

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (bridge->vlan_table, (void *)&nvlan);

      if (! rn || ! rn->info)
        continue;

      vlan = rn->info;

      vlanifp = vlan->ifp;

      if (! vlanifp)
        continue;
      p_ifp = vlanifp;

#ifdef HAVE_IGMP_SNOOP
      ret = nsm_l2_mcast_igmp_if_disable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface deletion for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      ret = nsm_l2_mcast_mld_if_disable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface deletion for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */

      nsm_l2_mcast_check_and_disable_snpif (master, bridge->name, vid, vid); 
    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);

  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->dynamicMemberBmp, vid)
    {
      su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, vid);

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (bridge->vlan_table, (void *)&nvlan);

      if (! rn || ! rn->info)
        continue;

      vlan = rn->info;

      vlanifp = vlan->ifp;

      if (! vlanifp)
        continue;

      p_ifp = vlanifp;

#ifdef HAVE_IGMP_SNOOP
      ret = nsm_l2_mcast_igmp_if_disable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface disable for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      ret = nsm_l2_mcast_mld_if_disable (l2mcast, ifp, p_ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface disable for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_MLD_SNOOP */

      nsm_l2_mcast_check_and_disable_snpif (master, bridge->name, vid, vid);

    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->dynamicMemberBmp, vid);

  return 0;
}

#ifdef HAVE_PROVIDER_BRIDGE

s_int32_t
nsm_l2_mcast_delete_pro_edge_port (struct nsm_bridge_master *master,
                                   char *bridge_name,
                                   struct interface *ifp)
{
  s_int32_t ret;
  u_int32_t brno;
  u_int16_t cvid;
  u_int16_t svid;
  u_int32_t su_id;
  struct nsm_if *zif;
  struct avl_node *rn;
  struct avl_node *node;
  struct nsm_master *nm;
  struct nsm_vlan nvlan;
  struct nsm_vlan *svlan;
#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
  struct interface *ctxifp;
#endif /* HAVE_L3 || HAVE_INTERVLAN_ROUTING  */
  struct nsm_bridge *bridge;
  struct nsm_l2_mcast *l2mcast;
  struct nsm_vlan_port *vlan_port;
#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_L3 || HAVE_INTERVLAN_ROUTING  */
  struct nsm_cvlan_reg_key *key = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL;

  if (! master || ! master->l2mcast || ! master->nm || ! ifp || ! ifp->info)
    return -1;

  ret = 0;
  nm = master->nm;
  zif = ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return -1;

  br_port = zif->switchport;
  l2mcast = master->l2mcast;
  vlan_port = &br_port->vlan_port;
  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return 0;

  pal_sscanf (bridge->name, "%u", &brno);

  /* Delete all the igif corresponding to the ports for
   * all Switching contexts.
   */

  if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
    {
      regtab = vlan_port->regtab;

      if (regtab == NULL)
        return 0;

      for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
         {
           if (! (key = AVL_NODE_INFO (node)))
             continue;

           su_id = NSM_L2_MCAST_MAKE_KEY (brno, key->cvid, key->svid);

#ifdef HAVE_IGMP_SNOOP
           ret = nsm_l2_mcast_igmp_if_delete (l2mcast, ifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "IGMP Interface deletion for %s vid %d svid %d "
                        "not successful", ifp->name, key->cvid, key->svid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
           ret = nsm_l2_mcast_mld_if_delete (l2mcast, ifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "MLD Interface deletion for %s vid %d svid %d "
                        "not successful", ifp->name, key->cvid, key->svid);
#endif /* HAVE_MLD_SNOOP */

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING

           ctx = nsm_pro_edge_swctx_lookup (bridge, key->cvid, key->svid);

           if (listcount (ctx->port_list) == 1)
             {
               ctxifp = ctx->ifp;

               if (! ctxifp)
                 return -1;

               /* Switching Context interface administratively/
                * operationally down
                */

               ctxifp->flags &= ~ (IFF_UP | IFF_RUNNING);

#ifdef HAVE_IGMP_SNOOP
               ret = nsm_l2_mcast_igmp_if_update (master->l2mcast, ctxifp,
                                                 su_id);
               if (ret < 0)
                 return ret;
#endif /* HAVE_IGMP_SNOOP */


#ifdef HAVE_MLD_SNOOP
               ret = nsm_l2_mcast_mld_if_update (master->l2mcast, ctxifp,
                                                 su_id);
               if (ret < 0)
                 return ret;
#endif /* HAVE_MLD_SNOOP */
            }
#endif /* HAVE_L3 || HAVE_INTERVLAN_ROUTING  */
         }
    }
  else
    {
      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, svid)
        {
          NSM_VLAN_SET (&nvlan, svid);

          rn = avl_search (bridge->svlan_table, (void *)&nvlan);

          if (! rn || ! rn->info )
            continue;

          svlan = rn->info;

          NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
            {
              su_id = NSM_L2_MCAST_MAKE_KEY (brno, cvid, svid);

#ifdef HAVE_IGMP_SNOOP
              ret = nsm_l2_mcast_igmp_if_delete (l2mcast, ifp, su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface deletion for %s vid %d "
                           "svid %d not successful", ifp->name, cvid,
                           svid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
              ret = nsm_l2_mcast_mld_if_delete (l2mcast, ifp, su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface deletion for %s vid %d "
                           "svid %d not successful", ifp->name, cvid,
                           svid);
#endif /* HAVE_MLD_SNOOP */

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING

              ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

              if (listcount (ctx->port_list) == 1)
                {
                  ctxifp = ctx->ifp;

                  if (! ctxifp)
                    return -1;

                  /* Switching Context interface administratively/
                     operationally down*/
                  ctxifp->flags &= ~ (IFF_UP | IFF_RUNNING);

#ifdef HAVE_IGMP_SNOOP
                  ret = nsm_l2_mcast_igmp_if_update (master->l2mcast, ctxifp,
                                                     su_id);
                  if (ret < 0)
                    return ret;
#endif /* HAVE_IGMP_SNOOP */


#ifdef HAVE_MLD_SNOOP
                  ret = nsm_l2_mcast_mld_if_update (master->l2mcast, ctxifp,
                                                    su_id);
                  if (ret < 0)
                    return ret;
#endif /* HAVE_MLD_SNOOP */

               }
#endif /* HAVE_L3 || HAVE_INTERVLAN_ROUTING  */
            }
          NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, svid);

      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->dynamicMemberBmp, svid)
        {
          NSM_VLAN_SET (&nvlan, svid);

          rn = avl_search (bridge->svlan_table, (void *)&nvlan);

          if (! rn || rn->info )
            continue;

          svlan = rn->info;

          NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
            {
              su_id = NSM_L2_MCAST_MAKE_KEY (brno, cvid, svid);

#ifdef HAVE_IGMP_SNOOP
              ret = nsm_l2_mcast_igmp_if_delete (l2mcast, ifp, su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface deletion for %s vid %d "
                           "svid %d not successful", ifp->name, cvid,
                           svid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
              ret = nsm_l2_mcast_mld_if_delete (l2mcast, ifp, su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface deletion for %s vid %d "
                           "svid %d not successful", ifp->name, cvid,
                           svid);
#endif /* HAVE_MLD_SNOOP */

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING

              ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

              if (listcount (ctx->port_list) == 1)
                {
                  ctxifp = ctx->ifp;

                  if (! ctxifp)
                    return -1;

                  /* Switching Context interface administratively/
                   * operationally down
                   A
*/
                  ctxifp->flags &= ~ (IFF_UP | IFF_RUNNING);

#ifdef HAVE_IGMP_SNOOP
                  ret = nsm_l2_mcast_igmp_if_update (master->l2mcast, ctxifp,
                                                     su_id);
                  if (ret < 0)
                    return ret;
#endif /* HAVE_IGMP_SNOOP */


#ifdef HAVE_MLD_SNOOP
                  ret = nsm_l2_mcast_mld_if_update (master->l2mcast, ctxifp,
                                                    su_id);
                  if (ret < 0)
                    return ret;
#endif /* HAVE_MLD_SNOOP */
                }
#endif /* HAVE_L3 || HAVE_INTERVLAN_ROUTING  */
            }
          NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port->dynamicMemberBmp, svid);
    }

  return 0;
}

s_int32_t
nsm_l2_mcast_enable_pro_edge_port (struct nsm_bridge_master *master,
                                   struct interface *ifp)
{
  u_int32_t brno;
  s_int32_t ret;
  u_int16_t cvid;
  u_int16_t svid;
  u_int32_t su_id;
  struct nsm_if *zif;
  struct avl_node *rn;
  struct avl_node *node;
  struct nsm_master *nm;
  struct nsm_vlan nvlan;
  struct nsm_vlan *svlan;
  struct interface *p_ifp;
  struct interface *ctxifp;
  struct nsm_bridge *bridge;
  struct nsm_l2_mcast *l2mcast;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_pro_edge_sw_ctx *ctx;
  struct nsm_cvlan_reg_key *key = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL;

  if (! master || ! master->l2mcast || ! master->nm || ! ifp || ! ifp->info)
    return -1;

  ret = 0;
  ctxifp = NULL;
  p_ifp = NULL;
  nm = master->nm;
  zif = ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return -1;

  br_port = zif->switchport;
  l2mcast = master->l2mcast;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (! bridge)
    return 0;

  regtab = vlan_port->regtab;

  pal_sscanf (bridge->name, "%u", &brno);

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return 0;

  /* Call igmp_if_start for all the igif corresponding to
   * all the swtching contexts of the port.
   */

  if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
    {
      if (regtab == NULL)
        return 0;

      for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
         {
           if (! (key = AVL_NODE_INFO (node)))
             continue;

           ctx = NULL;
           ctxifp = NULL;

           ctx = nsm_pro_edge_swctx_lookup (bridge, key->cvid, key->svid);

           if (ctx == NULL || ctx->ifp == NULL)
             continue;

           ctxifp = ctx->ifp;

           su_id = NSM_L2_MCAST_MAKE_KEY (brno, key->cvid, key->svid);

#ifdef HAVE_IGMP_SNOOP
           ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ctxifp, ctxifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d svid %d "
                        "not successful", ctxifp->name, key->cvid, key->svid);

           ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, ctxifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d svid %d "
                        "not successful", ifp->name, key->cvid, key->svid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
           ret = nsm_l2_mcast_mld_if_enable (l2mcast, ctxifp, ctxifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "MLD Interface enable for %s vid %d svid %d "
                        "not successful", ctxifp->name, key->cvid, key->svid);

           ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, ctxifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "MLD Interface enable for %s vid %d svid %d "
                        "not successful", ifp->name, key->cvid, key->svid);
#endif /* HAVE_MLD_SNOOP */
         }
    }
  else
    {
      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, svid)
        {
          NSM_VLAN_SET (&nvlan, svid);

          rn = avl_search (bridge->svlan_table, (void *)&nvlan);

          if (! rn || ! rn->info )
            continue;

          svlan = rn->info;

          NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
            {
              su_id = NSM_L2_MCAST_MAKE_KEY (brno, cvid, svid);

              ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

              ctxifp = ctx->ifp;

              if (! ctxifp)
                continue;

#ifdef HAVE_IGMP_SNOOP
              ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ctxifp, ctxifp,
                                                 su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

              ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, ctxifp,
                                                 su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
              ret = nsm_l2_mcast_mld_if_enable (l2mcast, ctxifp, ctxifp,
                                                su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

              ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, ctxifp,
                                                su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);
#endif /* HAVE_MLD_SNOOP */

            }
          NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, svid);

      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->dynamicMemberBmp, svid)
        {
          NSM_VLAN_SET (&nvlan, svid);

          rn = avl_search (bridge->svlan_table, (void *)&nvlan);

          if (! rn || rn->info )
            continue;

          svlan = rn->info;

          NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
            {
              su_id = NSM_L2_MCAST_MAKE_KEY (brno, cvid, svid);

              ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

              ctxifp = ctx->ifp;

              if (! ctxifp)
                continue;

#ifdef HAVE_IGMP_SNOOP
              ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ctxifp, ctxifp,
                                                 su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

              ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, ctxifp,
                                                 su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
              ret = nsm_l2_mcast_mld_if_enable (l2mcast, ctxifp, ctxifp,
                                                su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

              ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, ctxifp,
                                                su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);
#endif /* HAVE_MLD_SNOOP */

            }
          NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port->dynamicMemberBmp, svid);
    }

  return 0;
}

s_int32_t
nsm_l2_mcast_disable_pro_edge_port (struct nsm_bridge_master *master,
                                    struct interface *ifp)
{
  s_int32_t ret;
  u_int32_t brno;
  u_int16_t svid;
  u_int16_t cvid;
  u_int32_t su_id;
  struct nsm_if *zif;
  struct avl_node *rn;
  struct avl_node *node;
  struct nsm_master *nm;
  struct nsm_vlan nvlan;
  struct nsm_vlan *svlan;
  struct interface *ctxifp;
  struct nsm_bridge *bridge;
  struct nsm_l2_mcast *l2mcast;
  struct nsm_vlan_port *vlan_port;
  struct nsm_pro_edge_sw_ctx *ctx;
  struct nsm_cvlan_reg_key *key = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL;

  if (! master || ! master->l2mcast || ! master->nm || ! ifp || ! ifp->info)
    return -1;

  ret = 0;
  l2mcast = master->l2mcast;
  zif = ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return -1;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  nm = master->nm;
  ctxifp = NULL;
  bridge = zif->bridge; 

  if (! bridge)
    return 0;

  pal_sscanf (bridge->name, "%u", &brno);

  regtab = vlan_port->regtab;

  /* Call igmp_if_stop for all the igif corresponding to
   * all the vlans of the port.
   */

  if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
    {

      if (regtab == NULL)
        return 0;

      for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
         {
           if (! (key = AVL_NODE_INFO (node)))
             continue;

           ctx = NULL;
           ctxifp = NULL;

           ctx = nsm_pro_edge_swctx_lookup (bridge, key->cvid, key->svid);

           if (ctx == NULL || ctx->ifp == NULL)
             continue;

           ctxifp = ctx->ifp;

           su_id = NSM_L2_MCAST_MAKE_KEY (brno, key->cvid, key->svid);

#ifdef HAVE_IGMP_SNOOP

           ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, ctxifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d svid %d "
                        "not successful", ifp->name, key->cvid, key->svid);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP

           ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, ctxifp, su_id);

           if (ret < 0)
             zlog_warn (nm->zg, "MLD Interface enable for %s vid %d svid %d "
                        "not successful", ifp->name, key->cvid, key->svid);
#endif /* HAVE_MLD_SNOOP */

           nsm_l2_mcast_check_and_disable_snpif (master, bridge->name, key->cvid,
                                                 key->svid);
         }
    }
  else
    {
      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, svid)
        {
          NSM_VLAN_SET (&nvlan, svid);

          rn = avl_search (bridge->svlan_table, (void *)&nvlan);

          if (! rn || ! rn->info )
            continue;

          svlan = rn->info;

          NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
            {
              su_id = NSM_L2_MCAST_MAKE_KEY (brno, cvid, svid);

              ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

              ctxifp = ctx->ifp;

              if (! ctxifp)
                continue;

#ifdef HAVE_IGMP_SNOOP

              ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, ctxifp, su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP

              ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, ctxifp, su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);
#endif /* HAVE_MLD_SNOOP */

             nsm_l2_mcast_check_and_disable_snpif (master, bridge->name, cvid,
                                                   svid);
            }
          NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, svid);

      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->dynamicMemberBmp, svid)
        {
          NSM_VLAN_SET (&nvlan, svid);

          rn = avl_search (bridge->svlan_table, (void *)&nvlan);

          if (! rn || ! rn->info )
            continue;

          svlan = rn->info;

          NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
            {
              su_id = NSM_L2_MCAST_MAKE_KEY (brno, cvid, svid);

              ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

              ctxifp = ctx->ifp;

              if (! ctxifp)
                continue;

#ifdef HAVE_IGMP_SNOOP

              ret = nsm_l2_mcast_igmp_if_enable (l2mcast, ifp, ctxifp,
                                                 su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "IGMP Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);

#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP

              ret = nsm_l2_mcast_mld_if_enable (l2mcast, ifp, ctxifp,
                                                su_id);

              if (ret < 0)
                zlog_warn (nm->zg, "MLD Interface enable for %s vid %d "
                           "svid %d not successful", ctxifp->name, cvid,
                           svid);
#endif /* HAVE_MLD_SNOOP */

             nsm_l2_mcast_check_and_disable_snpif (master, bridge->name, cvid,
                                                   svid);
            }
          NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port->dynamicMemberBmp, svid);
    }

  return 0;
}

#endif /* HAVE_PROVIDER_BRIDGE */

s_int32_t
nsm_l2_mcast_delete_port_cb (struct nsm_bridge_master *master,
                             char *bridge_name,
                             struct interface *ifp)
{
  struct nsm_bridge *bridge;
  struct nsm_if *zif;

  zif = ifp->info;

  if (zif == NULL || zif->bridge == NULL)
    return -1;

  bridge = zif->bridge;

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return nsm_l2_mcast_delete_pro_edge_port (master, bridge_name, ifp);
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    return nsm_l2_mcast_delete_port (master, bridge_name, ifp);
}

s_int32_t
nsm_l2_mcast_enable_port_cb (struct nsm_bridge_master *master,
                             struct interface *ifp)
{
  struct nsm_bridge *bridge;
  struct nsm_if *zif;

  zif = ifp->info;

  if (zif == NULL || zif->bridge == NULL)
    return -1;

  bridge = zif->bridge;

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return nsm_l2_mcast_enable_pro_edge_port (master, ifp);
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    return nsm_l2_mcast_enable_port (master, ifp);

}

s_int32_t
nsm_l2_mcast_disable_port_cb (struct nsm_bridge_master *master,
                              struct interface *ifp)
{
  struct nsm_bridge *bridge;
  struct nsm_if *zif;

  zif = ifp->info;

  if (zif == NULL || zif->bridge == NULL)
    return -1;

  bridge = zif->bridge;

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return nsm_l2_mcast_disable_pro_edge_port (master, ifp);
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    return nsm_l2_mcast_disable_port (master, ifp);
}


s_int32_t
nsm_l2_mcast_send_query_for_bridge (struct nsm_bridge_master *master,
                                    char *bridge_name)
{
  /* Send general queries for all the Layer 2 ports. This is needed
   * when the topology changes and a port goes to forwarding from
   * Discarding.
   */

  return 0;
}

s_int32_t
nsm_l2_mcast_add_vlan (struct nsm_bridge_master *master,
                       char *bridgename,
                       u_int16_t vid,
                       u_int16_t svid)
{
  s_int32_t ret;
  u_int32_t brno;
  u_int32_t su_id;
  struct avl_node *rn;
  struct nsm_vlan nvlan;
  struct nsm_vlan *vlan;
  struct interface *ifp;
  struct nsm_bridge *bridge;
  struct avl_tree *vlan_table;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */

  if (!master)
    return -1;

  ret = 0;

  bridge = nsm_lookup_bridge_by_name (master, bridgename);

  if (!bridge)
    return -1;

  vlan_table = NULL;

  pal_sscanf (bridge->name, "%u", &brno);

#ifdef HAVE_PROVIDER_BRIDGE
  ctx = NULL;

  if (bridge->provider_edge == PAL_TRUE)
    ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    vlan_table = bridge->vlan_table;

#ifdef HAVE_PROVIDER_BRIDGE
   if (bridge->provider_edge == PAL_TRUE)
     {
       if (ctx == NULL)
         return -1;
       ifp = ctx->ifp;
     }
   else
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      if (vlan_table == NULL)
        return -1;

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || !rn->info)
        return -1;

      vlan = rn->info;

      ifp = vlan->ifp;
    }

  if (! ifp)
    return -1;

  su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, svid);

#ifdef HAVE_IGMP_SNOOP
  ret = nsm_l2_mcast_igmp_if_create (master->l2mcast, ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  ret = nsm_l2_mcast_mld_if_create (master->l2mcast, ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_MLD_SNOOP */

  return 0;
}

s_int32_t
nsm_l2_mcast_del_vlan (struct nsm_bridge_master *master,
                       char *bridgename,
                       u_int16_t vid,
                       u_int16_t svid)
{
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct avl_tree *vlan_table;
#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
  struct interface *p_ifp;
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */
  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge *bridge;
  struct list *port_list;
  struct nsm_master *nm;
  struct nsm_vlan *vlan;
  struct interface *ifp;
  struct nsm_vlan nvlan;
  struct listnode *ln;
  struct avl_node *rn;
  u_int32_t su_id;
  u_int32_t brno;
  s_int32_t ret;

  if (! master || ! master->l2mcast || ! master->nm)
    return -1;

  ret = 0;
  l2mcast = master->l2mcast;
  nm = master->nm;
  bridge = nsm_lookup_bridge_by_name (master, bridgename); 
  vlan_table = NULL;

  if (! bridge)
    return 0;

  pal_sscanf (bridge->name, "%u", &brno);

#ifdef HAVE_PROVIDER_BRIDGE
  ctx = NULL;

  if (bridge->provider_edge == PAL_TRUE)
    ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    vlan_table = bridge->vlan_table;

#ifdef HAVE_PROVIDER_BRIDGE
   if (bridge->provider_edge == PAL_TRUE)
     {
       if (ctx == NULL)
         return -1;

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
       p_ifp = ctx->ifp;
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */
       port_list = ctx->port_list;
     }
   else
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      if (vlan_table == NULL)
        return -1;

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || !rn->info)
        return -1;

      vlan = rn->info;

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
      p_ifp = vlan->ifp;
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */
      port_list = vlan->port_list;
    }

  su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, svid);

  /* Delete all the igif corresponding to the vlan */
  LIST_LOOP (port_list, ifp, ln)
    {
#ifdef HAVE_IGMP_SNOOP
      ret = nsm_l2_mcast_igmp_if_delete (l2mcast, ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "IGMP Interface deletion for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_IGMP_SNOOP */
#ifdef HAVE_MLD_SNOOP
      ret = nsm_l2_mcast_mld_if_delete (l2mcast, ifp, su_id);

      if (ret < 0)
        zlog_warn (nm->zg, "MLD Interface deletion for %s vid %d not successful",
                            ifp->name, vid);
#endif /* HAVE_MLD_SNOOP */
    }

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
  if (! p_ifp)
    return -1;

#ifdef HAVE_IGMP_SNOOP
  ret = nsm_l2_mcast_igmp_if_delete (master->l2mcast, p_ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  nsm_l2_mcast_mld_if_delete (master->l2mcast, p_ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_MLD_SNOOP */
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */

  return 0;
}

s_int32_t
nsm_l2_mcast_add_vlan_port (struct nsm_bridge_master *master,
                            char *bridgename,
                            struct interface *ifp,
                            u_int16_t vid,
                            u_int16_t svid)
{
  s_int32_t ret;
  u_int32_t brno;
  u_int32_t su_id;
  struct nsm_if *zif;
  struct nsm_vlan nvlan;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct list *port_list;
  struct interface *p_ifp;
  struct nsm_bridge *bridge;
  struct avl_tree *vlan_table;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */

  if (! master->l2mcast || ! ifp || ! ifp->info)
    return -1;

  vlan_table = NULL;
  zif = ifp->info;

  br_port = zif->switchport;

  if (! br_port)
    return -1;

  bridge = nsm_lookup_bridge_by_name (master, bridgename); 

  if (!bridge)
    return -1;

#ifdef HAVE_PROVIDER_BRIDGE

  ctx = NULL;

  if (bridge->provider_edge == PAL_TRUE)
    ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    vlan_table = bridge->vlan_table;

#ifdef HAVE_PROVIDER_BRIDGE
   if (bridge->provider_edge == PAL_TRUE)
     {
       if (ctx == NULL)
         return -1;

       p_ifp = ctx->ifp;
       port_list = ctx->port_list;
     }
   else
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      if (vlan_table == NULL)
        return -1;

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || !rn->info)
        return -1;

      vlan = rn->info;

      p_ifp = vlan->ifp;
      port_list = vlan->port_list;
    }

  pal_sscanf (bridge->name, "%u", &brno);

  su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, svid);

#ifdef HAVE_IGMP_SNOOP
  /* Add a igif wiih this ifindex and vid */
  ret = nsm_l2_mcast_igmp_if_create (master->l2mcast, ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  /* Add a mldif wiih this ifindex and vid */
  ret = nsm_l2_mcast_mld_if_create (master->l2mcast, ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_MLD_SNOOP */

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
  /* Vlan interface administratively/ operationally up*/
  p_ifp->flags |= (IFF_UP | IFF_RUNNING);

#ifdef HAVE_IGMP_SNOOP
  ret = nsm_l2_mcast_igmp_if_update (master->l2mcast, p_ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  ret = nsm_l2_mcast_mld_if_update (master->l2mcast, p_ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_MLD_SNOOP */
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */

  if (br_port->state != NSM_BRIDGE_PORT_STATE_FORWARDING)
    return 0;

#ifdef HAVE_IGMP_SNOOP
  /* Enable l2 mcast on vlan ifp */
  ret = nsm_l2_mcast_igmp_if_enable (master->l2mcast, p_ifp, p_ifp, su_id);

  if (ret < 0)
    return ret;

  ret = nsm_l2_mcast_igmp_if_enable (master->l2mcast, ifp, p_ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  /* Enable l2 mcast on vlan ifp */
  ret = nsm_l2_mcast_mld_if_enable (master->l2mcast, p_ifp, p_ifp, su_id);

  if (ret < 0)
    return ret;

  ret = nsm_l2_mcast_mld_if_enable (master->l2mcast, ifp, p_ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_MLD_SNOOP */

  return 0;
}

s_int32_t
nsm_l2_mcast_del_vlan_port (struct nsm_bridge_master *master,
                            char *bridgename,
                            struct interface *ifp,
                            u_int16_t vid,
                            u_int16_t svid)
{
#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING
  struct interface *p_ifp;
  struct list *port_list;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct avl_tree *vlan_table;
  struct nsm_bridge *bridge;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_vlan nvlan;
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */
  u_int32_t su_id;
  u_int32_t brno;
  s_int32_t ret;

  if (! master->l2mcast)
    return -1;

  pal_sscanf (bridgename, "%u", &brno);

  su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, svid);

#ifdef HAVE_IGMP_SNOOP
  /* Delete the igif wiith this ifindex and vid */
  ret = nsm_l2_mcast_igmp_if_delete (master->l2mcast, ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  /* Delete the mlif wiith this ifindex and vid */
  ret = nsm_l2_mcast_mld_if_delete (master->l2mcast, ifp, su_id);

  if (ret < 0)
    return ret;
#endif /* HAVE_MLD_SNOOP */

#if !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING

  bridge = nsm_lookup_bridge_by_name (master, bridgename);

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    ctx = nsm_pro_edge_swctx_lookup (bridge, vid, svid);
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    vlan_table = bridge->vlan_table;

#ifdef HAVE_PROVIDER_BRIDGE
   if (bridge->provider_edge == PAL_TRUE)
     {
       if (ctx == NULL)
         return -1;

       p_ifp = ctx->ifp;
       port_list = ctx->port_list;
     }
   else
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      if (vlan_table == NULL)
        return -1;

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      if (! rn || !rn->info)
        return -1;

      vlan = rn->info;

      p_ifp = vlan->ifp;

      port_list = vlan->port_list;
    }

  if (listcount (port_list) == 1)
    {
      if (! p_ifp)
        return -1;

      /* Parent interface administratively/ operationally down*/
      p_ifp->flags &= ~ (IFF_UP | IFF_RUNNING) ;

#ifdef HAVE_IGMP_SNOOP
      ret = nsm_l2_mcast_igmp_if_update (master->l2mcast, p_ifp,
                                         su_id);
      if (ret < 0)
        return ret;
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
      ret = nsm_l2_mcast_mld_if_update (master->l2mcast, p_ifp,
                                        su_id);
      if (ret < 0)
        return ret;

#endif /* HAVE_MLD_SNOOP */

#if defined (HAVE_BROADCOM) || defined (HAVE_MARVELL) || defined (HAVE_AXELX) \
            || defined (HAVE_MARVELL_LS)
#ifdef HAVE_IGMP_SNOOP 

      ret = hal_igmp_snooping_if_disable (bridge->name, ifp->ifindex);

#endif /* HAVE_IGMP_SNOOP */
#endif /* HAVE_BROADCOM || HAVE_MARVELL || HAVE_AXELX || HAVE_MARVELL_LS*/
#ifdef HAVE_MLD_SNOOP

      ret = hal_mld_snooping_if_disable (bridge->name, ifp->ifindex);

#endif /* HAVE_MLD_SNOOP */

      if (ret < 0)
        return ret;
    }
#endif /*!defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */

  return 0;
}

s_int32_t
nsm_l2_mcast_add_vlan_cb (struct nsm_bridge_master *master,
                          char *bridgename,
                          u_int16_t vid)
{
  struct nsm_bridge *bridge;

  bridge = nsm_lookup_bridge_by_name (master, bridgename);

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return 0;
#endif /* HAVE_PROVIDER_BRIDGE */

  return nsm_l2_mcast_add_vlan (master, bridgename, vid, vid);

}

s_int32_t
nsm_l2_mcast_del_vlan_cb (struct nsm_bridge_master *master,
                          char *bridgename,
                          u_int16_t vid)
{

  struct nsm_bridge *bridge;

  bridge = nsm_lookup_bridge_by_name (master, bridgename);

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return 0;
#endif /* HAVE_PROVIDER_BRIDGE */

  return nsm_l2_mcast_del_vlan (master, bridgename, vid, vid);

}


s_int32_t
nsm_l2_mcast_add_vlan_port_cb (struct nsm_bridge_master *master,
                               char *bridgename,
                               struct interface *ifp,
                               u_int16_t vid)
{
  struct nsm_bridge *bridge;

  bridge = nsm_lookup_bridge_by_name (master, bridgename);

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return 0;
#endif /* HAVE_PROVIDER_BRIDGE */

  return nsm_l2_mcast_add_vlan_port (master, bridgename, ifp, vid, vid);
}

s_int32_t
nsm_l2_mcast_del_vlan_port_cb (struct nsm_bridge_master *master,
                            char *bridgename,
                            struct interface *ifp,
                            u_int16_t vid)
{
  struct nsm_bridge *bridge;

  bridge = nsm_lookup_bridge_by_name (master, bridgename);

  if (! bridge)
    return 0;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE)
    return 0;
#endif /* HAVE_PROVIDER_BRIDGE */

  return nsm_l2_mcast_del_vlan_port (master, bridgename, ifp, vid, vid);
}

#ifdef HAVE_PROVIDER_BRIDGE

s_int32_t
nsm_l2_mcast_add_swctx_cb (struct nsm_bridge_master *master,
                           char *bridgename,
                           u_int16_t cvid,
                           u_int16_t svid)
{
  return nsm_l2_mcast_add_vlan (master, bridgename, cvid, svid);
}

s_int32_t
nsm_l2_mcast_del_swctx_cb (struct nsm_bridge_master *master,
                           char *bridgename,
                           u_int16_t cvid,
                           u_int16_t svid)
{
  return nsm_l2_mcast_del_vlan (master, bridgename, cvid, svid);
}


s_int32_t
nsm_l2_mcast_add_swctx_port_cb (struct nsm_bridge_master *master,
                                char *bridgename,
                                struct interface *ifp,
                                u_int16_t cvid,
                                u_int16_t svid)
{
  return nsm_l2_mcast_add_vlan_port (master, bridgename, ifp, cvid, svid);
}

s_int32_t
nsm_l2_mcast_del_swctx_port_cb (struct nsm_bridge_master *master,
                                char *bridgename,
                                struct interface *ifp,
                                u_int16_t cvid,
                                u_int16_t svid)
{
  return nsm_l2_mcast_del_vlan_port (master, bridgename, ifp, cvid, svid);
}
#endif /* HAVE_PROVIDER_BRIDGE */

static void_t
nsm_l2_mcast_free_bridge_listener (struct nsm_l2_mcast *l2mcast)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge_listener *l2mcast_br_appln;

  if (! l2mcast || ! l2mcast->nsm_l2mcast_br_appln || ! l2mcast->master)
    return;

  master = l2mcast->master;
  l2mcast_br_appln = l2mcast->nsm_l2mcast_br_appln;

  nsm_destroy_bridge_listener(l2mcast_br_appln);
  l2mcast->nsm_l2mcast_br_appln = NULL;

}

#ifdef HAVE_VLAN

static void_t
nsm_l2_mcast_free_vlan_listener (struct nsm_l2_mcast *l2mcast)
{
  struct nsm_bridge_master *master;
  struct nsm_vlan_listener *l2mcast_vlan_appln;

  if (! l2mcast || ! l2mcast->nsm_l2_mcast_vlan_appln || ! l2mcast->master)
    return;

  master = l2mcast->master;
  l2mcast_vlan_appln = l2mcast->nsm_l2_mcast_vlan_appln;


  nsm_destroy_vlan_listener (l2mcast->nsm_l2_mcast_vlan_appln);
  l2mcast->nsm_l2_mcast_vlan_appln = NULL;

}

#endif /* HAVE_VLaN */

static void_t
nsm_l2_mcast_dereg_bridge_listener (struct nsm_bridge *bridge,
                                    struct nsm_l2_mcast *l2mcast)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge_listener *l2mcast_br_appln;

  if (! l2mcast || ! l2mcast->nsm_l2mcast_br_appln || ! l2mcast->master)
    return;

  master = l2mcast->master;
  l2mcast_br_appln = l2mcast->nsm_l2mcast_br_appln;

  if( bridge && bridge->bridge_listener_list )
    {
      nsm_remove_listener_from_bridgelist (bridge->bridge_listener_list,
                                           l2mcast_br_appln->listener_id);
    }

  return;
}

static void_t
nsm_l2_mcast_dereg_vlan_listener (struct nsm_bridge *bridge,
                                  struct nsm_l2_mcast *l2mcast)
{
  struct nsm_bridge_master *master;
  struct nsm_vlan_listener *l2mcast_vlan_appln;

  if (! l2mcast || ! l2mcast->nsm_l2_mcast_vlan_appln || ! l2mcast->master)
    return;

  master = l2mcast->master;
  l2mcast_vlan_appln = l2mcast->nsm_l2_mcast_vlan_appln;

  if( bridge && bridge->vlan_listener_list )
    {
      nsm_remove_listener_from_list (bridge->vlan_listener_list,
                                     l2mcast_vlan_appln->listener_id);
    }

  return;
}

s_int32_t
nsm_l2_mcast_delete_bridge (struct nsm_bridge_master *master,
                            char *bridge_name)
{
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct avl_tree *vlan_table;
  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge *br;
  struct nsm_vlan *vlan;
  struct avl_node *rn;
  s_int32_t ret;

  ret = 0;

  if (! master || ! master->l2mcast)
    {
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  l2mcast = master->l2mcast;

  br = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! br)
    {
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  if (NSM_BRIDGE_TYPE_PROVIDER (br))
    vlan_table = br->svlan_table;
  else
    vlan_table = br->vlan_table;

  if (vlan_table == NULL)
    return NSM_ERR_L2_MCAST_INVALID_ARG;

  /* Delete all the snpif corresponding to all Layer 2 ports for
   * all vlans.
   */
#ifdef HAVE_PROVIDER_BRIDGE
  if (br->provider_edge == PAL_TRUE)
    {
      for (rn = avl_top (br->pro_edge_swctx_table); rn; rn = avl_next (rn))
        {
          if (! rn || !rn->info )
            continue;

          ctx = rn->info;

          ret = nsm_l2_mcast_del_vlan (master, bridge_name, ctx->cvid,
                                       ctx->svid);

          if (ret < 0)
            return ret;
        }
    }
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
        {
          if (! rn || !rn->info )
            continue;

          vlan = rn->info;

          ret = nsm_l2_mcast_del_vlan (master, bridge_name, vlan->vid, vlan->vid);

           if (ret < 0)
             return ret;
        }
    }

#ifdef HAVE_VLAN
  nsm_l2_mcast_dereg_vlan_listener (br, l2mcast);
#endif

  nsm_l2_mcast_dereg_bridge_listener (br, l2mcast);

#ifdef HAVE_HAL
#ifdef HAVE_IGMP_SNOOP
  hal_igmp_snooping_disable (br->name);
#endif
#ifdef HAVE_MLD_SNOOP
  hal_mld_snooping_disable (br->name);
#endif
#endif

  return 0;
}

static bool_t
nsm_l2_mcast_reg_bridge_listener (struct nsm_bridge *bridge,
                                  struct nsm_l2_mcast *l2mcast)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_bridge_listener *l2mcast_br_appln;

  if (! l2mcast || ! l2mcast->master || ! l2mcast->master->nm )
    {
      return PAL_FALSE;
    }

  master = l2mcast->master;
  nm = master->nm;

  if (! bridge)
    {
      zlog_warn (nm->zg, "NSM Bridge not configured");
      return PAL_FALSE;
    }

  if (l2mcast->nsm_l2mcast_br_appln == NULL)
    {
      if ((nsm_create_bridge_listener(&l2mcast->nsm_l2mcast_br_appln)
                                                != RESULT_OK))
        {
          zlog_warn (nm->zg, "Not able to create bridge listener");
          return PAL_FALSE;
        }
    }

  l2mcast_br_appln = l2mcast->nsm_l2mcast_br_appln;

  l2mcast_br_appln->listener_id = NSM_LISTENER_ID_L2MCAST_PROTO;
  l2mcast_br_appln->delete_bridge_func = nsm_l2_mcast_delete_bridge;
  l2mcast_br_appln->enable_port_func   = nsm_l2_mcast_enable_port_cb;
  l2mcast_br_appln->disable_port_func  = nsm_l2_mcast_disable_port_cb;
  l2mcast_br_appln->add_port_to_bridge_func = nsm_l2_mcast_add_port_cb;
  l2mcast_br_appln->delete_port_from_bridge_func = nsm_l2_mcast_delete_port_cb;
  l2mcast_br_appln->topology_change_notify_func =
                                      nsm_l2_mcast_send_query_for_bridge;

  if( bridge->bridge_listener_list )
    {
      if( nsm_add_listener_to_bridgelist (bridge->bridge_listener_list,
                                          l2mcast->nsm_l2mcast_br_appln)
                                                     == RESULT_OK )
        {
          return PAL_TRUE;
        }
    }

  nsm_destroy_bridge_listener (l2mcast->nsm_l2mcast_br_appln);
  l2mcast->nsm_l2mcast_br_appln = NULL;

  return PAL_FALSE;
}

static bool_t
nsm_l2_mcast_reg_vlan_listener (struct nsm_bridge *bridge,
                                struct nsm_l2_mcast *l2mcast)
{
  struct nsm_master *nm;
  struct nsm_bridge_master *master;
  struct nsm_vlan_listener *l2mcast_vlan_appln;

  if (! l2mcast || ! l2mcast->master || ! l2mcast->master->nm )
    {
      return PAL_FALSE;
    }

  master = l2mcast->master;
  nm = master->nm;

  if (! bridge)
    {
      zlog_warn (nm->zg, "NSM Bridge not configured");
      return PAL_FALSE;
    }

  if (l2mcast->nsm_l2_mcast_vlan_appln == NULL)
    {
      if ( (nsm_create_vlan_listener(&l2mcast->nsm_l2_mcast_vlan_appln) != RESULT_OK) )
        {
          zlog_warn (nm->zg, "Not able to create bridge listener");
          return PAL_FALSE;
        }
    }

  l2mcast_vlan_appln = l2mcast->nsm_l2_mcast_vlan_appln;

  l2mcast_vlan_appln->listener_id = NSM_LISTENER_ID_L2MCAST_PROTO;
  l2mcast_vlan_appln->flags = NSM_VLAN_STATIC | NSM_VLAN_DYNAMIC;
  l2mcast_vlan_appln->add_vlan_to_bridge_func = nsm_l2_mcast_add_vlan_cb;
  l2mcast_vlan_appln->delete_vlan_from_bridge_func = nsm_l2_mcast_del_vlan_cb;
  l2mcast_vlan_appln->add_vlan_to_port_func = nsm_l2_mcast_add_vlan_port_cb;
  l2mcast_vlan_appln->delete_vlan_from_port_func = nsm_l2_mcast_del_vlan_port_cb;

#ifdef HAVE_PROVIDER_BRIDGE
  l2mcast_vlan_appln->add_pro_edge_swctx_to_bridge = nsm_l2_mcast_add_swctx_cb;
  l2mcast_vlan_appln->delete_pro_edge_swctx_from_bridge =
                                        nsm_l2_mcast_del_swctx_cb;
  l2mcast_vlan_appln->add_port_to_swctx_func = nsm_l2_mcast_add_swctx_port_cb;
  l2mcast_vlan_appln->del_port_from_swctx_func = nsm_l2_mcast_del_swctx_port_cb;
#endif /* HAVE_PROVIDER_BRIDGE */

  if( bridge && bridge->vlan_listener_list )
    {
      if( nsm_add_listener_to_list (bridge->vlan_listener_list,
                                    l2mcast->nsm_l2_mcast_vlan_appln)
                                            == RESULT_OK )
        {
          return PAL_TRUE;
        }
    }

  nsm_destroy_vlan_listener(l2mcast->nsm_l2_mcast_vlan_appln);
  l2mcast->nsm_l2_mcast_vlan_appln = NULL;

  return PAL_FALSE;
}

/********************************************************************
 * Initialize Layer 2 Multicast
 ********************************************************************/
s_int32_t
nsm_l2_mcast_init (struct nsm_master *nm)
{
  s_int32_t ret;
  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge_master *master;

  ret = 0;

  master = nsm_bridge_get_master (nm);

  if (! master)
    {
      zlog_warn (nm->zg, "NSM Bridge Master is not initialised");
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  if (master->l2mcast)
    {
      return 0;
    }

  l2mcast = XCALLOC (MTYPE_NSM_L2_MCAST, sizeof(struct nsm_l2_mcast));

  if (! l2mcast)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  master->l2mcast = l2mcast;
  l2mcast->master = master;

  /* Associate input-output buffers */
  l2mcast->iobuf = ng->iobuf;

  if (! l2mcast->iobuf)
    return NSM_ERR_INTERNAL;

  /* Associate output buffers */
  l2mcast->obuf = ng->obuf;

  if (! l2mcast->obuf)
    return NSM_ERR_INTERNAL;

#ifdef HAVE_IGMP_SNOOP
  ret = nsm_l2_mcast_igmp_init (l2mcast);

  if (ret < 0)
    {
      XFREE (MTYPE_NSM_L2_MCAST, l2mcast);
      master->l2mcast = NULL;
      return NSM_ERR_INTERNAL;
    }
#endif /* HAVE_MLD_SNOOP */

#ifdef HAVE_MLD_SNOOP
  ret = nsm_l2_mcast_mld_init (l2mcast);

  if (ret < 0)
    {

#ifdef HAVE_IGMP_SNOOP
      nsm_l2_mcast_igmp_uninit (l2mcast);
#endif /* HAVE_IGMP_SNOOP */

      XFREE (MTYPE_NSM_L2_MCAST, l2mcast);
      master->l2mcast = NULL;
      return NSM_ERR_INTERNAL;
    }
#endif /* HAVE_MLD_SNOOP */

#ifdef HAVE_IGMP_SNOOP
  /* Register IGMP L2 Service */
  ret = nsm_l2_mcast_igmp_svc_reg (l2mcast);
#endif /* HAVE_MLD_SNOOP */

#ifdef HAVE_MLD_SNOOP
  /* Register MLD L2 Service */
  ret = nsm_l2_mcast_mld_svc_reg (l2mcast);
#endif /* HAVE_MLD_SNOOP */

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  /* The Listeners will be populated when a bridge is added */
  l2mcast->nsm_l2mcast_br_appln = NULL;
  l2mcast->nsm_l2_mcast_vlan_appln = NULL;

  return 0;

}

s_int32_t
nsm_l2_mcast_add_bridge (struct nsm_bridge *bridge,
                         struct nsm_bridge_master *master)
{
  struct nsm_l2_mcast *l2mcast;
  struct nsm_master *nm;

  if (! master || ! master->l2mcast || ! master->nm)
    {
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  if (! bridge)
    {
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  l2mcast = master->l2mcast;
  nm = master->nm;

  if (! nsm_l2_mcast_reg_bridge_listener (bridge, l2mcast))
    {
      XFREE (MTYPE_NSM_L2_MCAST, l2mcast);
      master->l2mcast = NULL;
      return NSM_ERR_MEM_ALLOC_FAILURE;
    }

#ifdef HAVE_HAL
#ifdef HAVE_IGMP_SNOOP
  hal_igmp_snooping_enable (bridge->name);
#endif
#ifdef HAVE_MLD_SNOOP
  hal_mld_snooping_enable (bridge->name);
#endif
#endif /* HAVE_HAL */

  return 0;

}

#ifdef HAVE_VLAN
s_int32_t
nsm_l2_mcast_vlan_init (struct nsm_bridge *bridge,
                        struct nsm_bridge_master *master)
{
  struct nsm_l2_mcast *l2mcast;
  struct nsm_master *nm;

  if (! master || ! master->l2mcast || ! master->nm)
    {
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  l2mcast = master->l2mcast;
  nm = master->nm;

  if (! nsm_l2_mcast_reg_vlan_listener (bridge, l2mcast))
    {
      nsm_l2_mcast_dereg_bridge_listener (bridge, l2mcast);
      XFREE (MTYPE_NSM_L2_MCAST, l2mcast);
      master->l2mcast = NULL;
      return NSM_ERR_MEM_ALLOC_FAILURE;
    }

  return 0;
}

s_int32_t
nsm_l2_mcast_vlan_deinit (struct nsm_bridge_master *master,
                          char *bridge_name)
{
  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge *br;


  if (! master || ! master->l2mcast)
    {
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  br = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! br)
    {
      return NSM_ERR_L2_MCAST_INVALID_ARG;
    }

  if (pal_mem_cmp (bridge_name, br->name, NSM_BRIDGE_NAMSIZ) != 0)
    {
      return 0;
    }

  l2mcast = master->l2mcast;

  nsm_l2_mcast_dereg_vlan_listener (br, l2mcast);

  return 0;
}
#endif /* HAVE_VLAN */

void_t
nsm_l2_mcast_deinit (struct nsm_master *nm)
{
  struct nsm_bridge_master *master;
  struct nsm_l2_mcast *l2mcast;

  if (! nm )
    return;

  master = nsm_bridge_get_master (nm);

  if (! master || ! master->l2mcast)
    return;

  l2mcast = master->l2mcast;

#ifdef HAVE_IGMP_SNOOP
  /* De-register IGMP L2 Service */
  nsm_l2_mcast_igmp_svc_dereg (l2mcast);

  /* Uninitialize IGMP */
  nsm_l2_mcast_igmp_uninit (l2mcast);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  /* De-register MLD L2 Service */
  nsm_l2_mcast_mld_svc_dereg (l2mcast);

  /* Uninitialize MLD */
  nsm_l2_mcast_mld_uninit (l2mcast);
#endif /* HAVE_MLD_SNOOP */

  /* Dissociate input-output buffer */
  l2mcast->iobuf = NULL;

  /* Dissociate output buffer */
  l2mcast->obuf = NULL;

  nsm_l2_mcast_free_bridge_listener (l2mcast);
#ifdef HAVE_VLAN
  nsm_l2_mcast_free_vlan_listener (l2mcast);
#endif /* HAVE_VLAN */

  XFREE (MTYPE_NSM_L2_MCAST, l2mcast);
  master->l2mcast = NULL;

  return;
}

/* IGMP/MLD Snooping Interface SuID Callback */
s_int32_t
nsm_l2_mcast_if_su_id_get (u_int32_t su_id,
                           struct interface *ifp,
                           u_int16_t sec_key,
                           u_int32_t *if_su_id)
{
  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  s_int32_t svid;
  u_int32_t brno;
  s_int32_t vid;

  l2mcast = (struct nsm_l2_mcast *) su_id;
  bridge = NULL;

  if (! ifp || ! if_su_id)
    return NSM_ERR_INTERNAL;

  zif = ifp->info;

  if (! zif)
    return NSM_ERR_INTERNAL;

  if (! INTF_TYPE_L2 (ifp)
      && ! if_is_vlan (ifp))
    return NSM_ERR_INTERNAL;

  if (INTF_TYPE_L2 (ifp)
      && (bridge = zif->bridge) == NULL)
    return NSM_ERR_INTERNAL;

#ifdef HAVE_PROVIDER_BRIDGE
  if (if_is_vlan (ifp))
    {
      if (ifp->name [0] == 's')
        {
          if (pal_sscanf (ifp->name, "svlan%u.%u.%u", &brno, &vid, &svid) == 2)
            svid = vid;
        }
      else
        {
          if (pal_sscanf (ifp->name, "vlan%u.%u.%u", &brno, &vid, &svid) == 2)
            svid = vid;
        }
    }
#else
  if (if_is_vlan (ifp))
    {
      pal_sscanf (ifp->name, "vlan%u.%u", &brno, &vid);
      svid = vid;
    }
#endif /* HAVE_PROVIDER_BRIDGE */
  else
    {
      pal_sscanf (bridge->name, "%u", &brno);
      svid = vid = sec_key;
    }
   
  *if_su_id = NSM_L2_MCAST_MAKE_KEY (brno, vid, svid);

  return NSM_SUCCESS;
}

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP

/* Function to enable or disable default mode
     flooding of unlearned multicast in igmp_snooping */
s_int32_t
nsm_l2_unknown_mcast_mode (int umode)
{
      return hal_l2_unknown_mcast_mode (umode);
}

CLI (l2_unknown_mcast,
     l2_unknown_mcast_cmd,
     "l2 unknown mcast (flood|discard)",
     "Layer 2",
     "Unknown",
     "Mcast Traffic",
     "flood mode",
     "Discard mode"
     )
{
  s_int32_t ret;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_l2_mcast *l2mcast = master->l2mcast;
  int mode = 0;
  
  if (pal_strncmp ("f", argv[0], 1) == 0)
    mode = 0;
  else if (pal_strncmp ("d", argv[0], 1) == 0)
    mode = 1;
  
  ret = nsm_l2_unknown_mcast_mode(mode);
  
  if (ret < 0)
    {
      cli_out (cli, "%% Setting of l2 unknown mcast mode failed \n");
      return CLI_ERROR;
    }
  else
    {
     l2mcast->nsm_l2_unknown_mcast = mode;
     return CLI_SUCCESS;
    }
}

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_IGMP_SNOOP
/* IGMP Snooping L-Mem Message Indication Callback */
s_int32_t
nsm_l2_mcast_igmp_lmem (u_int32_t su_id,
                        struct interface *ifp,
                        u_int32_t key,
                        enum igmp_filter_mode_state filt_mode,
                        struct pal_in4_addr *grp_addr,
                        u_int16_t num_srcs,
                        struct pal_in4_addr src_addr_list [])
{
  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge_master *master;
  char bridge_name [NSM_BRIDGE_NAMSIZ + 1];

  l2mcast = (struct nsm_l2_mcast *) su_id;

  pal_mem_set (bridge_name, 0, NSM_BRIDGE_NAMSIZ + 1);

  if (! l2mcast || ! l2mcast->master || ! ifp || ! grp_addr)
    return NSM_SUCCESS;

  master = l2mcast->master;

  pal_snprintf (bridge_name, NSM_BRIDGE_NAMSIZ, "%u",
                NSM_L2_MCAST_GET_BRID (key));

  nsm_l2_mcast_process_event (master, bridge_name, AF_INET,
                              grp_addr, key, ifp,
                              num_srcs, src_addr_list, filt_mode);

  return NSM_SUCCESS;
}

/* IGMP Snooping callback to add/delete Mrouter interfaces in forwarder */
s_int32_t
nsm_l2_mcast_igmp_mrt_if_update (u_int32_t su_id,
                                 struct interface *ifp,
                                 u_int32_t key,
                                 bool_t is_exclude)
{

  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge_master *master;
  char bridge_name [NSM_BRIDGE_NAMSIZ + 1];

  l2mcast = (struct nsm_l2_mcast *) su_id;

  pal_mem_set (bridge_name, 0, NSM_BRIDGE_NAMSIZ + 1);

  if (! l2mcast || ! l2mcast->master || ! ifp)
    return NSM_SUCCESS;

  master = l2mcast->master;

  pal_snprintf (bridge_name, NSM_BRIDGE_NAMSIZ, "%u",
                NSM_L2_MCAST_GET_BRID (key));

  return nsm_l2_mcast_igmp_add_del_mrt_entry (master, bridge_name, ifp,
                                              key, is_exclude);
}



/* Library VRF's Callback to get IGMP Instance */
void_t *
nsm_l2_mcast_igmp_get_instance (struct apn_vrf *ivrf)
{
  struct apn_vr *vr;
  struct nsm_master *nm;

  if (! ivrf
      || ! (vr = ivrf->vr)
      || ! (nm = (struct nsm_master *) vr->proto)
      || ! nm->bridge
      || ! nm->bridge->l2mcast)
    return NULL;

  return ((void_t *) nm->bridge->l2mcast->igmp_inst);
}

/* Initialize IGMP Instance */
s_int32_t
nsm_l2_mcast_igmp_init (struct nsm_l2_mcast *l2mcast)
{
  struct igmp_init_params iginp;
  struct igmp_instance *igi;
  struct apn_vrf *vrf;
#ifdef HAVE_IGMP_SNOOP
  struct cli_tree *ctree;
#endif
  s_int32_t ret;

  if (l2mcast->igmp_inst)
    return NSM_ERR_INTERNAL;

  if (! l2mcast->master || ! l2mcast->master->nm || ! l2mcast->master->nm->vr)
    return NSM_ERR_INTERNAL;

  vrf = apn_vrf_lookup_by_id (l2mcast->master->nm->vr, 0);

  if (! vrf)
    return NSM_ERR_INTERNAL;

  iginp.igin_lg = L2MZG (l2mcast);
  iginp.igin_owning_ivrf = vrf;
  iginp.igin_i_buf = l2mcast->iobuf;
  iginp.igin_o_buf = l2mcast->obuf;

#ifdef HAVE_TUNNEL
  iginp.igin_cback_tunnel_get = nsm_tunnel_if_name_set;
#else
  iginp.igin_cback_tunnel_get = NULL;
#endif /* HAVE_TUNNEL */


  ret = igmp_initialize (&iginp, &igi);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->igmp_inst = igi;

  LIB_VRF_SET_IGMP_INSTANCE_GET_FUNC (vrf,
                                      nsm_l2_mcast_igmp_get_instance);

  ctree = igi->igi_lg->ctree;
#ifdef HAVE_IGMP_SNOOP
  /* Install the CLIs for enable/disable default unlearned 
   multicast flooding mode */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &l2_unknown_mcast_cmd);
    
#endif 
  return NSM_SUCCESS;
}

s_int32_t
nsm_l2_mcast_igmp_uninit (struct nsm_l2_mcast *l2mcast)
{
  struct apn_vrf *vrf;
  s_int32_t ret;

  if (! l2mcast->igmp_inst
      || ! l2mcast->master
      || ! l2mcast->master->nm->vr
      || ! l2mcast->master->nm)
    return NSM_ERR_INTERNAL;

  vrf = apn_vrf_lookup_by_id (l2mcast->master->nm->vr, 0);

  if (! vrf)
    return NSM_ERR_INTERNAL;

  ret = igmp_uninitialize (l2mcast->igmp_inst, PAL_FALSE);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->igmp_inst = NULL;

  LIB_VRF_SET_IGMP_INSTANCE_GET_FUNC (vrf, NULL);

  return NSM_SUCCESS;
}

/* Register for Layer-2 IGMP Service */
s_int32_t
nsm_l2_mcast_igmp_svc_reg (struct nsm_l2_mcast *l2mcast)
{
  struct igmp_svc_reg_params igsrp;
  s_int32_t ret;

  if (! l2mcast->igmp_inst)
    return NSM_ERR_INTERNAL;

  igsrp.igsrp_su_id = (u_int32_t) l2mcast;
  igsrp.igsrp_svc_type = IGMP_SVC_TYPE_L2;
  igsrp.igsrp_sock = IGMP_SOCK_FD_INVALID;
  igsrp.igsrp_cback_if_su_id = nsm_l2_mcast_if_su_id_get;
  igsrp.igsrp_cback_vif_ctl = NULL;
  igsrp.igsrp_cback_lmem_update = nsm_l2_mcast_igmp_lmem;
  igsrp.igsrp_cback_mfc_prog = NULL;
  igsrp.igsrp_cback_mrt_if_update = nsm_l2_mcast_igmp_mrt_if_update;

  ret = igmp_svc_register (l2mcast->igmp_inst, &igsrp);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->igmp_svc_reg_id = igsrp.igsrp_sp_id;

  return NSM_SUCCESS;
}

/* De-Register Layer-2 IGMP Service */
s_int32_t
nsm_l2_mcast_igmp_svc_dereg (struct nsm_l2_mcast *l2mcast)
{
  struct igmp_svc_reg_params igsrp;
  s_int32_t ret;

  if (! l2mcast->igmp_inst)
    return NSM_ERR_INTERNAL;

  igsrp.igsrp_sp_id = (u_int32_t) l2mcast->igmp_svc_reg_id;
  igsrp.igsrp_svc_type = IGMP_SVC_TYPE_L2;

  ret = igmp_svc_deregister (l2mcast->igmp_inst, &igsrp);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->igmp_svc_reg_id = igsrp.igsrp_sp_id;

  return NSM_SUCCESS;
}

/* Add IGMP Interface */
s_int32_t
nsm_l2_mcast_igmp_if_create (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             u_int32_t su_id)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;

  if (! l2mcast || ! l2mcast->igmp_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_get (l2mcast->igmp_inst, &igifidx);

  if (! igif)
    return NSM_ERR_INTERNAL;

  return NSM_SUCCESS;
}

/* Delete IGMP Interface */
s_int32_t
nsm_l2_mcast_igmp_if_delete (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             u_int32_t su_id)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;

  if (! l2mcast || ! l2mcast->igmp_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_lookup (l2mcast->igmp_inst, &igifidx);

  if (! igif)
    return NSM_ERR_INTERNAL;

  return igmp_if_delete (igif);
}

/* Enable IGMP Interface */
s_int32_t
nsm_l2_mcast_igmp_if_enable (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             struct interface *p_ifp,
                             u_int32_t su_id)
{
  struct igmp_if_idx p_igifidx;
  struct igmp_if_idx igifidx;
  struct igmp_if *p_igif;
  struct igmp_if *igif;

  if (! l2mcast || ! l2mcast->igmp_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_lookup (l2mcast->igmp_inst, &igifidx);

  if (! igif)
    return NSM_ERR_INTERNAL;

  /* For a better time complexity during VLAN IF association*/  
  p_igifidx.igifidx_ifp = p_ifp;
  p_igifidx.igifidx_sid = su_id;

  p_igif = igmp_if_lookup (l2mcast->igmp_inst, &p_igifidx);
  
  return igmp_if_update (igif, p_igif, IGMP_IF_UPDATE_L2_MCAST_ENABLE);
}

/* Disable IGMP Interface */
s_int32_t
nsm_l2_mcast_igmp_if_disable (struct nsm_l2_mcast *l2mcast,
                              struct interface *ifp,
                              struct interface *p_ifp,
                              u_int32_t su_id)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *p_igif;
  struct igmp_if *igif;

  if (! l2mcast || ! l2mcast->igmp_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_lookup (l2mcast->igmp_inst, &igifidx);

  if (! igif)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = p_ifp;
  igifidx.igifidx_sid = su_id;

  p_igif = igmp_if_lookup (l2mcast->igmp_inst, &igifidx);

  return igmp_if_update (igif, p_igif, IGMP_IF_UPDATE_L2_MCAST_DISABLE);
}

/* Update IGMP Interface of vlanifp */
s_int32_t
nsm_l2_mcast_igmp_if_update (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             u_int32_t su_id)
{
  struct igmp_if_idx igifidx;
  struct igmp_if *igif;

  if (! l2mcast || ! l2mcast->igmp_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  igifidx.igifidx_ifp = ifp;
  igifidx.igifidx_sid = su_id;

  igif = igmp_if_lookup (l2mcast->igmp_inst, &igifidx);

  if (! igif)
    return NSM_SUCCESS;

  return igmp_if_update (igif, NULL, IGMP_IF_UPDATE_IFF_STATE);
}
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
/* MLD Snooping L-Mem Message Indication Callback */
s_int32_t
nsm_l2_mcast_mld_lmem (u_int32_t su_id,
                       struct interface *ifp,
                       u_int32_t key,
                       enum mld_filter_mode_state filt_mode,
                       struct pal_in6_addr *grp_addr,
                       u_int16_t num_srcs,
                       struct pal_in6_addr src_addr_list [])
{
  struct nsm_bridge_master *master;
  struct nsm_l2_mcast *l2mcast;
  struct nsm_bridge *bridge;

  l2mcast = (struct nsm_l2_mcast *) su_id;

  if (! l2mcast || ! l2mcast->master || ! ifp || ! grp_addr)
    return NSM_SUCCESS;

  master = l2mcast->master;

  bridge = nsm_get_first_bridge (master);

  nsm_l2_mcast_process_event (master, bridge->name, AF_INET6,
                              grp_addr, key, ifp,
                              num_srcs, src_addr_list, filt_mode);
  return NSM_SUCCESS;
}

/* Library VRF's Callback to get MLD Instance */
void_t *
nsm_l2_mcast_mld_get_instance (struct apn_vrf *ivrf)
{
  struct apn_vr *vr;
  struct nsm_master *nm;

  if (! ivrf
      || ! (vr = ivrf->vr)
      || ! (nm = (struct nsm_master *) vr->proto)
      || ! nm->bridge
      || ! nm->bridge->l2mcast)
    return NULL;

  return ((void_t *) nm->bridge->l2mcast->mld_inst);
}

/* Initialize MLD Instance */
s_int32_t
nsm_l2_mcast_mld_init (struct nsm_l2_mcast *l2mcast)
{
  struct mld_init_params mlinp;
  struct mld_instance *mli;
  struct apn_vrf *vrf;
#ifdef HAVE_MLD_SNOOP
  struct cli_tree *ctree;
#endif
  s_int32_t ret;

  if (l2mcast->mld_inst)
    return NSM_ERR_INTERNAL;

  if (! l2mcast->master || ! l2mcast->master->nm)
    return NSM_ERR_INTERNAL;

  vrf = apn_vrf_lookup_by_id (l2mcast->master->nm->vr, 0);

  if (! vrf)
    return NSM_ERR_INTERNAL;

  mlinp.mlin_lg = L2MZG (l2mcast);
  mlinp.mlin_owning_ivrf = vrf;
  mlinp.mlin_i_buf = l2mcast->iobuf;
  mlinp.mlin_o_buf = l2mcast->obuf;
#ifdef HAVE_TUNNEL
  mlinp.mlin_cback_tunnel_get = nsm_tunnel_if_name_set;
#else
  mlinp.mlin_cback_tunnel_get = NULL;
#endif /* HAVE_TUNNEL */


  ret = mld_initialize (&mlinp, &mli);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->mld_inst = mli;

  LIB_VRF_SET_MLD_INSTANCE_GET_FUNC (vrf,
                                     nsm_l2_mcast_mld_get_instance);

#ifdef HAVE_MLD_SNOOP
  ctree = mli->mli_lg->ctree;
  /* Install the CLIs for enable/disable default unlearned
     multicast flooding mode */
     cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                      &l2_unknown_mcast_cmd);
#endif /* HAVE_MLD_SNOOP */

  return NSM_SUCCESS;
}

s_int32_t
nsm_l2_mcast_mld_uninit (struct nsm_l2_mcast *l2mcast)
{
  struct apn_vrf *vrf;
  s_int32_t ret;

  if (! l2mcast->mld_inst
      || ! l2mcast->master
      || ! l2mcast->master->nm)
    return NSM_ERR_INTERNAL;

  vrf = apn_vrf_lookup_by_id (l2mcast->master->nm->vr, 0);

  if (! vrf)
    return NSM_ERR_INTERNAL;

  ret = mld_uninitialize (l2mcast->mld_inst, PAL_FALSE);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->mld_inst = NULL;

  LIB_VRF_SET_MLD_INSTANCE_GET_FUNC (vrf, NULL);

  return NSM_SUCCESS;
}

/* Register for Layer-2 MLD Service */
s_int32_t
nsm_l2_mcast_mld_svc_reg (struct nsm_l2_mcast *l2mcast)
{
  struct mld_svc_reg_params mlsrp;
  s_int32_t ret;

  if (! l2mcast->mld_inst)
    return NSM_ERR_INTERNAL;

  mlsrp.mlsrp_su_id = (u_int32_t) l2mcast;
  mlsrp.mlsrp_svc_type = MLD_SVC_TYPE_L2;
  mlsrp.mlsrp_sock = MLD_SOCK_FD_INVALID;
  mlsrp.mlsrp_cback_if_su_id = nsm_l2_mcast_if_su_id_get;
  mlsrp.mlsrp_cback_vif_ctl = NULL;
  mlsrp.mlsrp_cback_lmem_update = nsm_l2_mcast_mld_lmem;
  mlsrp.mlsrp_cback_mfc_prog = NULL;

  ret = mld_svc_register (l2mcast->mld_inst, &mlsrp);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->mld_svc_reg_id = mlsrp.mlsrp_sp_id;

  return NSM_SUCCESS;
}

/* De-Register Layer-2 MLD Service */
s_int32_t
nsm_l2_mcast_mld_svc_dereg (struct nsm_l2_mcast *l2mcast)
{
  struct mld_svc_reg_params mlsrp;
  s_int32_t ret;

  if (! l2mcast->mld_inst)
    return NSM_ERR_INTERNAL;

  mlsrp.mlsrp_sp_id = (u_int32_t) l2mcast->mld_svc_reg_id;
  mlsrp.mlsrp_svc_type = MLD_SVC_TYPE_L2;

  ret = mld_svc_deregister (l2mcast->mld_inst, &mlsrp);

  if (ret < 0)
    return NSM_ERR_INTERNAL;

  l2mcast->mld_svc_reg_id = mlsrp.mlsrp_sp_id;

  return NSM_SUCCESS;
}

/* Add MLD Interface */
s_int32_t
nsm_l2_mcast_mld_if_create (struct nsm_l2_mcast *l2mcast,
                            struct interface *ifp,
                            u_int32_t su_id)
{
  struct mld_if_idx mlifidx;
  struct mld_if *mlif;

  if (! l2mcast || ! l2mcast->mld_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  mlifidx.mlifidx_ifp = ifp;
  mlifidx.mlifidx_sid = su_id;

  mlif = mld_if_get (l2mcast->mld_inst, &mlifidx);

  if (! mlif)
    return NSM_ERR_INTERNAL;

  return NSM_SUCCESS;
}

/* Delete MLD Interface */
s_int32_t
nsm_l2_mcast_mld_if_delete (struct nsm_l2_mcast *l2mcast,
                            struct interface *ifp,
                            u_int32_t su_id)
{
  struct mld_if_idx mlifidx;
  struct mld_if *mlif;

  if (! l2mcast || ! l2mcast->mld_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  mlifidx.mlifidx_ifp = ifp;
  mlifidx.mlifidx_sid = su_id;

  mlif = mld_if_lookup (l2mcast->mld_inst, &mlifidx);

  if (! mlif)
    return NSM_ERR_INTERNAL;

  return mld_if_delete (mlif);
}

/* Enable MLD Interface */
s_int32_t
nsm_l2_mcast_mld_if_enable (struct nsm_l2_mcast *l2mcast,
                            struct interface *ifp,
                            struct interface *p_ifp,
                            u_int32_t su_id)
{
  struct mld_if_idx p_mlifidx;
  struct mld_if_idx mlifidx;
  struct mld_if *p_mlif;
  struct mld_if *mlif;

  if (! l2mcast || ! l2mcast->mld_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  mlifidx.mlifidx_ifp = ifp;
  mlifidx.mlifidx_sid = su_id;

  mlif = mld_if_lookup (l2mcast->mld_inst, &mlifidx);

  if (! mlif)
    return NSM_ERR_INTERNAL;

  /* For a better time complexity during VLAN IF association*/
  p_mlifidx.mlifidx_ifp = p_ifp;
  p_mlifidx.mlifidx_sid = su_id;

  p_mlif = mld_if_lookup (l2mcast->mld_inst, &p_mlifidx);

  return mld_if_update (mlif, p_mlif, MLD_IF_UPDATE_L2_MCAST_ENABLE);
}

/* Enable MLD Interface */
s_int32_t
nsm_l2_mcast_mld_if_disable (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             struct interface *p_ifp,
                             u_int32_t su_id)
{
  struct mld_if_idx mlifidx;
  struct mld_if *p_mlif;
  struct mld_if *mlif;

  if (! l2mcast || ! l2mcast->mld_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  mlifidx.mlifidx_ifp = ifp;
  mlifidx.mlifidx_sid = su_id;

  mlif = mld_if_lookup (l2mcast->mld_inst, &mlifidx);

  if (! mlif)
    return NSM_ERR_INTERNAL;

  mlifidx.mlifidx_ifp = p_ifp;
  mlifidx.mlifidx_sid = su_id;

  p_mlif = mld_if_lookup (l2mcast->mld_inst, &mlifidx);

  return mld_if_update (mlif, p_mlif, MLD_IF_UPDATE_L2_MCAST_DISABLE);
}

/* Update MLD Interface of vlanifp */
s_int32_t
nsm_l2_mcast_mld_if_update (struct nsm_l2_mcast *l2mcast,
                            struct interface *ifp,
                            u_int32_t su_id)
{
  struct mld_if_idx mlifidx;
  struct mld_if *mlif;

  if (! l2mcast || ! l2mcast->mld_inst || ! ifp)
    return NSM_ERR_INTERNAL;

  mlifidx.mlifidx_ifp = ifp;
  mlifidx.mlifidx_sid = su_id;

  mlif = mld_if_lookup (l2mcast->mld_inst, &mlifidx);

  if (! mlif)
    return NSM_SUCCESS;

  return mld_if_update (mlif, NULL, MLD_IF_UPDATE_IFF_STATE);
}

#endif /* HAVE_MLD_SNOOP */

void
nsm_l2_mcast_if_add (struct nsm_if **zif, struct interface *ifp)
{
  (*zif)->l2mcastif.ifp = ifp;
#ifdef HAVE_IGMP_SNOOP
  (*zif)->l2mcastif.igmpsnp_gmr_tib = ptree_init (NSM_L2_MCAST_IPV4_PREFIXLEN);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  (*zif)->l2mcastif.mldsnp_gmr_tib = ptree_init (NSM_L2_MCAST_IPV6_PREFIXLEN);
#endif /* HAVE_MLD_SNOOP */
}

void
nsm_l2_mcast_if_delete (struct nsm_if **zif)
{
  (*zif)->ifp = NULL;

#ifdef HAVE_IGMP_SNOOP
  ptree_finish((*zif)->l2mcastif.igmpsnp_gmr_tib);
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  ptree_finish((*zif)->l2mcastif.mldsnp_gmr_tib);
#endif /* HAVE_MLD_SNOOP */
}

void_t
nsm_l2_mcast_srec_clear_valid (struct nsm_l2_mcast_grec *ngr)
{
  struct nsm_l2_mcast_source_rec *nsr;
  struct ptree_node *pn;

  for (pn = ptree_top (ngr->nsm_l2_mcast_src_tib); pn; pn = ptree_next (pn))
    {
      if (! (nsr = pn->info))
        continue;

      UNSET_FLAG (nsr->flags, NSM_ISR_SFLAG_VALID);
    }
}

s_int32_t
nsm_l2_mcast_action_include (char *bridgename,
                             u_int8_t family,
                             void_t *pgrp,
                             u_int32_t su_id,
                             struct interface *ifp,
                             u_int16_t num_srcs,
                             void_t *src_addr_list,
                             enum nsm_l2mcast_filter_mode_state filt_mode,
                             struct nsm_l2_mcast_grec *ngr)
{
  u_int32_t idx;
  u_int16_t vlanid;
  u_int16_t svlanid;
  struct ptree_node *pn;
  struct hal_in4_addr src;
#ifdef HAVE_MLD_SNOOP
  struct hal_in6_addr src6;
#endif /* HAVE_MLD_SNOOP */
  struct ptree_node *pn_next;
  struct pal_in4_addr *src_addr;
  struct nsm_l2_mcast_source_rec *nsr;

  src_addr = src_addr_list;

  vlanid = NSM_L2_MCAST_GET_CVID (su_id);
  svlanid = NSM_L2_MCAST_GET_SVID (su_id);

  switch (filt_mode)
    {
    case NSM_L2_MCAST_FMS_INCLUDE:

      nsm_l2_mcast_srec_clear_valid (ngr);

      for (idx=0; idx < num_srcs; idx++)
        {
          nsr = nsm_l2_mcast_srec_lookup (ngr, &src_addr [idx]);

          if (nsr == NULL)
            {
               nsr = nsm_l2_mcast_srec_get (ngr,
                                            & src_addr[idx]);

               if (family == AF_INET)
                 hal_igmp_snooping_add_entry (bridgename,
                                              (struct hal_in4_addr *)
                                              &src_addr[idx],
                                              (struct hal_in4_addr *) pgrp,
                                              PAL_FALSE,
                                              vlanid,
                                              svlanid,
                                              1,
                                              &ifp->ifindex);
#ifdef HAVE_MLD_SNOOP
               else if (family == AF_INET6)
                  hal_mld_snooping_add_entry (bridgename,
                                              (struct hal_in6_addr *)
                                               &src_addr[idx],
                                               (struct hal_in6_addr *) pgrp,
                                               PAL_FALSE,
                                               vlanid,
                                               svlanid,
                                               1,
                                               &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */
                 else
                   return NSM_ERR_INTERNAL;

            }
          else
            SET_FLAG (nsr->flags, NSM_ISR_SFLAG_VALID);
        }

      for (pn = ptree_top (ngr->nsm_l2_mcast_src_tib); pn;
           pn = pn_next)
        {

          pn_next = ptree_next (pn);

          if (! (nsr = pn->info))
            continue;

          if (! CHECK_FLAG (nsr->flags, NSM_ISR_SFLAG_VALID))
            {
              if (family == AF_INET)
                {
                 hal_igmp_snooping_delete_entry (bridgename,
                                                 (struct hal_in4_addr *)
                                                 PTREE_NODE_KEY (pn),
                                                 (struct hal_in4_addr *) pgrp,
                                                 PAL_FALSE,
                                                 vlanid,
                                                 svlanid,
                                                 1,
                                                 &ifp->ifindex);
                }
#ifdef HAVE_MLD_SNOOP
               else if (family == AF_INET6)
                  hal_mld_snooping_delete_entry  (bridgename,
                                                 (struct hal_in6_addr *)
                                                  PTREE_NODE_KEY (pn),
                                                  (struct hal_in6_addr *) pgrp,
                                                  PAL_FALSE,
                                                  vlanid,
                                                  svlanid,
                                                  1,
                                                  &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */
                else
                  return NSM_ERR_INTERNAL;

              nsm_l2_mcast_srec_delete (nsr);
            }
        }

      ngr->filt_mode = NSM_L2_MCAST_FMS_INCLUDE;

      if (ngr->num_srcs == 0)
        nsm_l2_mcast_grec_delete (ngr);

    break ;

    case NSM_L2_MCAST_FMS_EXCLUDE:
      for (pn = ptree_top (ngr->nsm_l2_mcast_src_tib); pn;
           pn = pn_next)
        {

          pn_next = ptree_next (pn);

          if (! (nsr = pn->info))
            continue;

          if (family == AF_INET)
            {
             hal_igmp_snooping_delete_entry (bridgename,
                                             (struct hal_in4_addr *)
                                             PTREE_NODE_KEY (pn),
                                             (struct hal_in4_addr *) pgrp,
                                             PAL_TRUE,
                                             vlanid,
                                             svlanid,
                                             1,
                                             &ifp->ifindex);
            }
#ifdef HAVE_MLD_SNOOP
          else if (family == AF_INET6)
                  hal_mld_snooping_delete_entry  (bridgename,
                                                  (struct hal_in6_addr *)
                                                  PTREE_NODE_KEY (pn),
                                                  (struct hal_in6_addr *) pgrp,
                                                  PAL_TRUE,
                                                  vlanid,
                                                  svlanid,
                                                  1,
                                                  &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */

            else
              return NSM_ERR_INTERNAL;

          nsm_l2_mcast_srec_delete (nsr);
        }

      for (idx=0; idx < num_srcs; idx++)
        {
           nsm_l2_mcast_srec_get (ngr,
                                  &src_addr[idx]);

          if (family == AF_INET)
            hal_igmp_snooping_add_entry (bridgename,
                                         (struct hal_in4_addr *)
                                          &src_addr[idx],
                                         (struct hal_in4_addr *)
                                          pgrp,
                                         PAL_TRUE,
                                         vlanid,
                                         svlanid,
                                         1,
                                         &ifp->ifindex);
#ifdef HAVE_MLD_SNOOP
           else if (family == AF_INET6)
              hal_mld_snooping_add_entry  (bridgename,
                                           (struct hal_in6_addr *)
                                           &src_addr[idx],
                                           (struct hal_in6_addr *) pgrp,
                                           PAL_TRUE,
                                           vlanid,
                                           svlanid,
                                           1,
                                           &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */

            else
              return NSM_ERR_INTERNAL;

         }

      src.s_addr = 0;

#ifdef HAVE_MLD_SNOOP
      pal_mem_set (&src6, 0, sizeof (struct hal_in6_addr));
#endif /* HAVE_MLD_SNOOP*/


      if (family == AF_INET)
        hal_igmp_snooping_add_entry (bridgename,
                                     (struct hal_in4_addr *) &src,
                                     (struct hal_in4_addr *) pgrp,
                                     PAL_FALSE,
                                     vlanid,
                                     svlanid,
                                     1,
                                     &ifp->ifindex);
#ifdef HAVE_MLD_SNOOP
      else if (family == AF_INET6)
        hal_mld_snooping_add_entry  (bridgename,
                                     (struct hal_in6_addr *) &src6,
                                     (struct hal_in6_addr *) pgrp,
                                     PAL_FALSE,
                                     vlanid,
                                     svlanid,
                                     1,
                                     &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */
      else
         return NSM_ERR_INTERNAL;

      ngr->filt_mode = NSM_L2_MCAST_FMS_EXCLUDE;

    break;
    default:
    break;
    }
  return 0;
}

s_int32_t
nsm_l2_mcast_action_exclude (char *bridgename,
                             u_int8_t family,
                             void_t *pgrp,
                             u_int32_t su_id,
                             struct interface *ifp,
                             u_int16_t num_srcs,
                             void_t *src_addr_list,
                             enum nsm_l2mcast_filter_mode_state filt_mode,
                             struct nsm_l2_mcast_grec *ngr)
{
  u_int32_t idx;
  u_int16_t vlanid;
  u_int16_t svlanid;
  struct ptree_node *pn;
  struct hal_in4_addr src;
#ifdef HAVE_MLD_SNOOP
  struct hal_in6_addr src6;
#endif /* HAVE_MLD_SNOOP */
  struct ptree_node *pn_next;
  struct pal_in4_addr *src_addr;
  struct nsm_l2_mcast_source_rec *nsr;

  src_addr = src_addr_list;
  vlanid = NSM_L2_MCAST_GET_CVID (su_id);
  svlanid = NSM_L2_MCAST_GET_SVID (su_id);

  switch (filt_mode)
    {
    case NSM_L2_MCAST_FMS_INCLUDE:

      src.s_addr = 0;

#ifdef HAVE_MLD_SNOOP
      pal_mem_set (&src6, 0, sizeof (struct hal_in6_addr));
#endif /* HAVE_MLD_SNOOP*/

      if (family == AF_INET)
        {
         hal_igmp_snooping_delete_entry (bridgename,
                                         (struct hal_in4_addr *) &src,
                                         (struct hal_in4_addr *) pgrp,
                                         PAL_FALSE,
                                         vlanid,
                                         svlanid,
                                         1,
                                         &ifp->ifindex);
        }
#ifdef HAVE_MLD_SNOOP
      else if (family == AF_INET6)
          hal_mld_snooping_delete_entry  (bridgename,
                                          (struct hal_in6_addr *) &src6,
                                          (struct hal_in6_addr *) pgrp,
                                          PAL_FALSE,
                                          vlanid,
                                          svlanid,
                                          1,
                                          &ifp->ifindex);

#endif /* HAVE_MLD_SNOOP */
        else
          return NSM_ERR_INTERNAL;

      for (pn = ptree_top (ngr->nsm_l2_mcast_src_tib); pn;
           pn = pn_next)
        {
          pn_next = ptree_next (pn);

          if (! (nsr = pn->info))
            continue;

          if (family == AF_INET)
            {
             hal_igmp_snooping_delete_entry (bridgename,
                                             (struct hal_in4_addr *)
                                             PTREE_NODE_KEY (pn),
                                             (struct hal_in4_addr *) pgrp,
                                             PAL_FALSE,
                                             vlanid,
                                             svlanid,
                                             1,
                                             &ifp->ifindex);
            }
#ifdef HAVE_MLD_SNOOP
          else if (family == AF_INET6)
              hal_mld_snooping_add_entry  (bridgename,
                                           (struct hal_in6_addr *)
                                           PTREE_NODE_KEY (pn),
                                           (struct hal_in6_addr *) pgrp,
                                           PAL_FALSE,
                                           vlanid,
                                           svlanid,
                                           1,
                                           &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */ 
            else
              return NSM_ERR_INTERNAL;

          nsm_l2_mcast_srec_delete (nsr);

        }

      for (idx=0; idx < num_srcs; idx++)
        {
          nsm_l2_mcast_srec_get (ngr,
                                 &src_addr[idx]);

          if (family == AF_INET)
            hal_igmp_snooping_add_entry (bridgename,
                                         (struct hal_in4_addr *)
                                         &src_addr[idx],
                                         (struct hal_in4_addr *) pgrp,
                                         PAL_FALSE,
                                         vlanid,
                                         svlanid,
                                         1,
                                         &ifp->ifindex);
#ifdef HAVE_MLD_SNOOP
          else if (family == AF_INET6)
              hal_mld_snooping_add_entry  (bridgename,
                                           (struct hal_in6_addr *)
                                           &src_addr[idx],
                                           (struct hal_in6_addr *) pgrp,
                                           PAL_FALSE,
                                           vlanid,
                                           svlanid,
                                           1,
                                           &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */
          else
            return NSM_ERR_INTERNAL;

        }

      ngr->filt_mode = NSM_L2_MCAST_FMS_INCLUDE ;

      if (ngr->num_srcs == 0)
        nsm_l2_mcast_grec_delete (ngr);

      break;

      case NSM_L2_MCAST_FMS_EXCLUDE:

      nsm_l2_mcast_srec_clear_valid (ngr);

      for (idx=0; idx < num_srcs; idx++)
        {
          nsr = nsm_l2_mcast_srec_lookup (ngr, &src_addr[idx]);

          if (nsr == NULL)
            {
               nsr = nsm_l2_mcast_srec_get (ngr,
                                            & src_addr[idx]);

               if (family == AF_INET)
                 hal_igmp_snooping_add_entry (bridgename,
                                              (struct hal_in4_addr *)
                                              &src_addr[idx],
                                              (struct hal_in4_addr *) pgrp,
                                              PAL_TRUE,
                                              vlanid,
                                              svlanid,
                                              1,
                                              &ifp->ifindex);
#ifdef HAVE_MLD_SNOOP
               else if (family == AF_INET6)
                   hal_mld_snooping_add_entry  (bridgename,
                                                (struct hal_in6_addr *)
                                                &src_addr[idx],
                                                (struct hal_in6_addr *) pgrp,
                                                PAL_TRUE,
                                                vlanid,
                                                svlanid,
                                                1,
                                                &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */  
                 else
                   return NSM_ERR_INTERNAL;

            }
          else
            SET_FLAG (nsr->flags, NSM_ISR_SFLAG_VALID);
        }

      for (pn = ptree_top (ngr->nsm_l2_mcast_src_tib); pn;
           pn = pn_next)
        {

          pn_next = ptree_next (pn);

          if (! (nsr = pn->info))
            continue;

          if (! CHECK_FLAG (nsr->flags, NSM_ISR_SFLAG_VALID))
            {
              if (family == AF_INET)
                hal_igmp_snooping_delete_entry (bridgename,
                                                (struct hal_in4_addr *)
                                                PTREE_NODE_KEY (pn),
                                                (struct hal_in4_addr *) pgrp,
                                                PAL_TRUE,
                                                vlanid,
                                                svlanid,
                                                1,
                                                &ifp->ifindex);
#ifdef HAVE_MLD_SNOOP
               else if (family == AF_INET6)
                  hal_mld_snooping_delete_entry (bridgename,
                                                  (struct hal_in6_addr *)
                                                  PTREE_NODE_KEY (pn),
                                                  (struct hal_in6_addr *) pgrp,
                                                  PAL_TRUE,
                                                  vlanid,
                                                  svlanid,
                                                  1,
                                                  &ifp->ifindex);
#endif /* HAVE_MLD_SNOOP */
                else
                  return NSM_ERR_INTERNAL ;

              nsm_l2_mcast_srec_delete (nsr);
            }
        }

      ngr->filt_mode = NSM_L2_MCAST_FMS_EXCLUDE;
    break;
    default:
    break;
     }

  return 0;
}



s_int32_t
nsm_l2_mcast_action_invalid (char *bridgename,
                             u_int8_t family,
                             void_t *pgrp,
                             u_int32_t su_id,
                             struct interface *ifp,
                             u_int16_t num_srcs,
                             void_t *src_addr_list,
                             enum nsm_l2mcast_filter_mode_state filt_mode,
                             struct nsm_l2_mcast_grec *ngr)

{
 return NSM_FAILURE;
}

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */


