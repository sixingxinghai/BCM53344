/* Copyright (C) 2004  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "hal_incl.h"

#ifdef HAVE_VLAN

#include "if.h"
#include "cli.h"
#include "table.h"
#include "hash.h"
#include "linklist.h"
#include "snprintf.h"
#include "nsm_message.h"
#include "nsm_client.h"
#include "nsmd.h"
#include "avl_tree.h"
#ifdef HAVE_L3
#include "rib.h"
#endif /* HAVE_L3 */
#include "nsm_api.h"
#include "nsm_server.h"
#ifdef HAVE_L2
#include "nsm_bridge.h"
#include "ptree.h"
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"
#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif /* HAVE_VLAN_CLASS */
#ifdef HAVE_VLAN_STACK
#include "nsm_vlan_stack.h"
#endif /* HAVE_VLAN_STACK */
#endif /* HAVE_L2 */

#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_list.h"
#endif /* HAVE_QOS */

#include "nsm_interface.h"
#include "nsm_router.h"
#include "nsm_debug.h"

#ifdef HAVE_CUSTOM1
#include "nsm_vlan.h"
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_LACPD
#include "nsm_lacp.h"
#endif /* HAVE_LACPD */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
#ifdef HAVE_IGMP_SNOOP
#include "igmp.h"
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
#include "mld.h"
#endif /* HAVE_MLD_SNOOP */

#include "nsm_l2_mcast.h"
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_PROVIDER_BRIDGE
#include "nsm_pro_vlan.h"
#endif

#ifdef HAVE_HA
#include "nsm_cal.h"
#ifdef HAVE_L2
#include "nsm_bridge_cal.h"
#endif /* HAVE_L2 */
#endif /* HAVE_HA */

#ifdef HAVE_SMI
#include "lib_fm.h"
#include "smi_nsm_fm.h"
#endif /* HAVE_SMI */

#ifdef HAVE_VRRP
#include "vrrp.h"
#endif /* HAVE_VRRP */

#if defined HAVE_I_BEB || defined HAVE_B_BEB
#include "nsm_bridge_pbb.h"
#endif/*HAVE_I_BEB || HAVE_B_BEB*/

#ifdef HAVE_PBB_TE
#include "nsm_pbb_te_cli.h"
#endif /* HAVE_PBB_TE */

#ifdef HAVE_MPLS_VC
extern void nsm_mpls_vc_vlan_bind (struct interface *ifp, u_int16_t vlan_id);
extern void nsm_mpls_vc_vlan_unbind (struct interface *ifp, u_int16_t vlan_id);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
extern void nsm_vpls_vlan_bind (struct interface *ifp, u_int16_t vlan_id);
extern void nsm_vpls_vlan_unbind (struct interface *ifp, u_int16_t vlan_id);
#endif /* HAVE_VPLS */

#ifdef HAVE_ELMID
#include "nsm_elmi.h"
#endif /* HAVE_ELMID */

#ifndef HAVE_CUSTOM1
static int
nsm_vlan_set_port_mode (struct interface *ifp, enum nsm_vlan_port_mode mode,
                        enum nsm_vlan_port_mode sub_mode,
                        bool_t notify_kernel);
#endif
static int nsm_vlan_set_default (struct nsm_bridge *bridge, 
                                 struct interface *ifp, int mode, 
                                 nsm_vid_t vid, bool_t notify_kernel);
#define INGRESS_FILTER_SET     1
#define INGRESS_FILTER_NOT_SET 0

#ifdef HAVE_PROVIDER_BRIDGE
int nsm_vlan_clear_provider_port (struct interface *ifp,
                                  enum nsm_vlan_port_mode mode,
                                  bool_t iterate_members,
                                  bool_t notify_kernel);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_L2_NPF
int
nsm_get_npf_vlan_state (enum nsm_vlan_state state)
{
  switch (state)
    {
    case NSM_VLAN_DISABLED:
      return NPF_VLAN_SUSPEND_STATE;
    case NSM_VLAN_ACTIVE:
      return NPF_VLAN_ACTIVE_STATE; 
    default:
      return -1;
    }

  return NPF_VLAN_ACTIVE_STATE;
}
#endif /* HAVE_L2_NPF */

/* Create VLAN interface. */
int
nsm_vlan_if_set (struct nsm_master *nm, nsm_vid_t vid,
                 nsm_vid_t svid, u_char type, char *brname)
{
  int ret;
  struct nsm_vlan p;
  struct nsm_if *zif;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct interface *ifp;
  struct nsm_bridge *bridge;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx key;
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */
  char ifname[INTERFACE_NAMSIZ + 1];

  bridge = nsm_lookup_bridge_by_name (nm->bridge, brname);

  if (!bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  NSM_VLAN_SET (&p, vid);
  vlan = NULL;

#ifdef HAVE_PROVIDER_BRIDGE
  ctx = NULL;
  key.svid = svid;
  key.cvid = vid;
  if (bridge->provider_edge)
    rn = avl_search (bridge->pro_edge_swctx_table, (void *) &key);
  else if (type & NSM_VLAN_SVLAN)
    rn = avl_search (bridge->svlan_table, (void *)&p);
  else
    rn = avl_search (bridge->vlan_table, (void *)&p);
#else
  rn = avl_search (bridge->vlan_table, (void *)&p);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_B_BEB
/* Overwrite the default rn entries if B_BEB is set */
  if (type & NSM_VLAN_BVLAN)
    rn = avl_search (bridge->bvlan_table, (void *)&p);
#endif

  if (! rn || !rn->info)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge)
    ctx = rn->info;
  else
    vlan = rn->info;
#else
  vlan = rn->info;
#endif /* HAVE_PROVIDER_BRIDGE */

  pal_mem_set (ifname, 0, (INTERFACE_NAMSIZ + 1));
  ifp = NULL;
  zif = NULL;
  ret = 0;

#ifdef HAVE_CUSTOM1
  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s %d", "vlan", vid);
#else /* HAVE_CUSTOM1 */
#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge)
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d.%d", "vlan",
               brname, vid, svid);
  else if (type & NSM_VLAN_SVLAN)
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "svlan", brname, vid);
  else 
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "vlan", brname, vid);
#else
  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "vlan", brname, vid);
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_B_BEB
  /* Edit the default interface name in case the type is BVLAN */
  /* This is to align with the port overloading in swfwdr for allowing PBB 
     control plane packets to be forwarded. The interface name should be 
     changed back to bvlan later.TBD */
  if (type & NSM_VLAN_BVLAN)
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "svlan", "b", vid);
#endif /* HAVE_B_BEB */

  ifp = ifg_lookup_by_name (&nzg->ifg, ifname);

  if (ifp != NULL)
    {
#ifdef HAVE_SWFWDR
      if (vid != NSM_VLAN_DEFAULT_VID)         
        {
#endif /* HAVE_SWFWDR */
          /* Previous instance of the interface has not been deleted 
           * cleanly yet */
          /* if (ifp->clean_pend_resp_list)
             return NSM_VLAN_ERR_IFP_NOT_DELETED;*/
#ifdef HAVE_SWFWDR
        }
#endif /* HAVE_SWFWDR */
    }

  if (! ifp)
    {
      ifp = ifg_get_by_name (&nzg->ifg, ifname);
      if (!ifp) 
        return -1;

      SET_FLAG (ifp->status, NSM_INTERFACE_MAPPED);
#ifdef HAVE_CUSTOM1
      zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%d", "vlan", vid);
#else /* HAVE_CUSTOM1 */
      if (type & NSM_VLAN_SVLAN)
        zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "svlan", brname, vid);
#ifdef HAVE_B_BEB      
      /* This is to align with the port overloading in swfwdr for allowing PBB 
         control plane packets to be forwarded. The interface name should be 
         changed back to bvlan later.TBD */
      if (type & NSM_VLAN_BVLAN)
        zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "svlan", "b", vid);
#endif /* HAVE_B_BEB */     
      else
        zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "vlan", brname, vid);
#endif /* HAVE_CUSTOM1 */
#ifdef HAVE_INTERFACE_NAME_MAPPING
      pal_strncpy (ifp->orig, ifname, INTERFACE_NAMSIZ);
#endif /* HAVE_INTERFACE_NAME_MAPPING. */
      SET_FLAG (ifp->flags, IFF_UP);
      SET_FLAG (ifp->flags, IFF_MULTICAST);
      ifp->hw_type = IF_TYPE_VLAN;
      ifp->ifindex = NSM_INVALID_INTERFACE_INDEX;
#ifdef HAVE_CUSTOM1
      ifp->pid = vid;
#endif /* HAVE_CUSTOM1 */
      ifp->type = NSM_IF_TYPE_L3;
      ifp->vr = nm->vr;
      nsm_if_add (ifp);

      listnode_add (ifp->vr->ifm.if_list, ifp);

      ifp->vrf = apn_vrf_lookup_default (nm->vr);

      zif = (struct nsm_if *)ifp->info;
      zif->type = NSM_IF_TYPE_L3;
      zif->vid = vid;

#if defined HAVE_L3 && defined HAVE_INTERVLAN_ROUTING && defined HAVE_HAL
#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
        {
          ret = hal_if_svi_create (ifp->name, NULL);
          if (ret < 0)
              goto ERR;
        }
#endif /* HAVE_L3 && HAVE_INTERVLAN_ROUTING && HAVE_HAL */
    }

  /* Associate the new vlanif to vlan */

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge)
    ctx->ifp = ifp;
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    vlan->ifp = ifp;

  return 0;

#if defined HAVE_L3 && defined HAVE_INTERVLAN_ROUTING && defined HAVE_HAL
 ERR:
  /* Adding SVI failed, rollback the addition and return error. */
  if (ifp)
    {
      /* Delete it from VR. */
      if (ifp->vr->ifm.if_list)
        listnode_delete (ifp->vr->ifm.if_list, ifp);

      /* Delete it from ifg. */
      if_delete (&nzg->ifg, ifp);
    }
#endif /* HAVE_L3 && HAVE_INTERVLAN_ROUTING && HAVE_HAL */

  return ret;
}

int
nsm_vlan_if_unset (struct nsm_master *nm, nsm_vid_t vid,
                   nsm_vid_t svid, u_char type, char *brname)
{
  int ret;
  struct nsm_vlan p;
  struct avl_node *rn;
  struct interface *ifp = NULL;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx key;
  struct nsm_pro_edge_sw_ctx *ctx;
#endif /* HAVE_PROVIDER_BRIDGE */
  char ifname[INTERFACE_NAMSIZ + 1];
  struct nsm_bridge_master *master;

  if (!nm || !nm->bridge)
    return -1;

  master = nsm_bridge_get_master (nm);

  if (!master)
    return -1;

  bridge = nsm_lookup_bridge_by_name (master, brname);

  if (!bridge)
    return -1;

  NSM_VLAN_SET (&p, vid);
  vlan = NULL;

#ifdef HAVE_PROVIDER_BRIDGE
  key.svid = svid;
  key.cvid = vid;
  ctx = NULL;
  if (bridge->provider_edge)
    rn = avl_search (bridge->pro_edge_swctx_table, (void *) &key);
  else if (type & NSM_VLAN_SVLAN)
    rn = avl_search (bridge->svlan_table, (void *)&p);
  else
    rn = avl_search (bridge->vlan_table, (void *)&p);
#else
  rn = avl_search (bridge->vlan_table, (void *)&p);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_B_BEB
/* editing the default value in case B_BEB is enabled and type is BVLAN*/
  if (type & NSM_VLAN_BVLAN)
    rn = avl_search (bridge->bvlan_table, (void *)&p);
#endif

  if (! rn || !rn->info)
    return -1;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge)
    ctx = rn->info;
  else
    vlan = rn->info;
#else
  vlan = rn->info;
#endif /* HAVE_PROVIDER_BRIDGE */

  pal_mem_set (ifname, 0, sizeof(ifname));
  ret = 0;

#ifdef HAVE_CUSTOM1
  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s %d", "vlan", vid);
#else /* HAVE_CUSTOM1 */
#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge)
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d%d", "vlan", brname, vid, svid);
  else if (type & NSM_VLAN_SVLAN)
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "svlan", brname, vid);
  else
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "vlan", brname, vid);
#else
  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "vlan", brname, vid);
#endif /* HAVE_PROVIDER_BRIDGE */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_B_BEB
/* editing the default value in case B_BEB is enabled and type is BVLAN*/
  if (type & NSM_VLAN_BVLAN)
    zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "bvlan", brname, vid);
#endif


  ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
  if (! ifp)
    return -1;

#if !defined HAVE_SWFWDR && (!defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING)
  if (if_is_up (ifp))
    {
      UNSET_FLAG (ifp->flags, IFF_UP);
      nsm_if_down (ifp);
    }

  /* Delete it from VR. */
  if (ifp->vr->ifm.if_list)
    listnode_delete (ifp->vr->ifm.if_list, ifp);

  ifp->vr = NULL;
  nsm_if_delete (ifp, ifp->vrf->proto);
#elif !defined HAVE_SWFWDR
  nsm_if_delete_update (ifp);
#endif /* !defined HAVE_L3 || !defined HAVE_INTERVLAN_ROUTING */

  if (vlan)
    vlan->ifp = NULL;

  return 0;
}

#ifdef HAVE_CUSTOM1
int
nsm_port_if_set (u_int32_t vr_id, char *str, int index, int pid)
{
  struct nsm_master *nm; 
  struct interface *ifp;
  char ifname[INTERFACE_NAMSIZ + 1];

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (! nm)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  zsnprintf (ifname, INTERFACE_NAMSIZ, "%s %d", str, index);

  ifp =  ifg_lookup_by_name (&nzg->ifg, ifname);
  if (! ifp)
    {
      ifp = ifg_get_by_name (&nzg->ifg, ifname);
      SET_FLAG (ifp->status, NSM_INTERFACE_MAPPED);
      ifp->hw_type = IF_TYPE_PORT;
      ifp->pid = (pid - 1);
      ifp->type = NSM_IF_TYPE_L3; 
      nsm_if_add (ifp);

      ifp->vr = nm->vr;
      listnode_add (ifp->vr->ifm.if_list, ifp);

      ifp->vrf = apn_vrf_lookup_default (nm->vr);
    }
  return NSM_API_SET_SUCCESS;
}

int
nsm_vlan_init (u_int32_t vr_id)
{
  int i;
  struct nsm_master *nm;

  /* Lookup master structure.  */
  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  /* Set default VLAN parameters.  */
  nm->l2.vlan_max = NSM_LAYER2_VLAN_MAX;
  nm->l2.vlan_num = 0;
  nm->l2.vlan_l3_max = NSM_LAYER2_VLAN_L3_MAX;
  nm->l2.vlan_l3_num = 0;

  /* Create interface structure.  */
  for (i = 1; i <= npf_fastcount(); i++)
    nsm_port_if_set (vr_id, "fastEthernet", i, i);

  for (i = npf_fastcount() + 1; i <= npf_fastcount() + npf_gigacount(); i++)
    nsm_port_if_set (vr_id, "gigabitEthernet", i - npf_fastcount(), i);

  /* Create a default VLAN 1.  */
  nsm_vlan_if_set (nm, 1, NULL, NSM_VLAN_CVLAN);
}
#else /* HAVE_CUSTOM1 */

/* Initialize VLAN. */
int
nsm_vlan_init (struct nsm_bridge *bridge)
{
  int ret;

  /* Initialize VLAN table. */
  ret = avl_create (&bridge->vlan_table, 0, nsm_vlan_id_cmp);
  if (ret < 0 )
    return ret;

  ret = avl_create (&bridge->svlan_table, 0,nsm_vlan_id_cmp);

  if (ret < 0)
   return ret;

#if defined HAVE_PBB_TE && defined HAVE_I_BEB && defined HAVE_B_BEB
  ret = avl_create (&bridge->stp_decoupled_bvlan_table, 0,nsm_vlan_id_cmp);
  
  if (ret < 0)
   return ret;
#endif /* HAVE_PBB_TE && HAVE_I_BEB && HAVE_B_BEB */

#if defined HAVE_PBB_TE && !defined HAVE_I_BEB && !defined HAVE_B_BEB
  ret = avl_create (&bridge->stp_decoupled_svlan_table, 0,nsm_vlan_id_cmp);

  if (ret < 0)
   return ret;
#endif /* HAVE_PBB_TE && !HAVE_I_BEB && !HAVE_B_BEB */


#ifdef HAVE_PROVIDER_BRIDGE
  ret = avl_create (&bridge->pro_edge_swctx_table, 0, nsm_pro_edge_sw_ctx_ent_cmp);

  if (ret < 0)
   return ret;

#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_B_BEB
  if(!pal_strncmp(bridge->name,"backbone",NSM_BRIDGE_NAMSIZ))
  {
    ret = avl_create (&bridge->bvlan_table, 0, nsm_vlan_id_cmp);
#if defined HAVE_PBB_TE && defined HAVE_I_BEB && defined HAVE_B_BEB 
    if (ret < 0)
      {
        avl_tree_free (&bridge->bvlan_table, NULL);
        bridge->bvlan_table = NULL;
      }
#endif /* HAVE_PBB_TE && HAVE_I_BEB && HAVE_B_BEB */
  }
  if (ret < 0)
    return ret;

#endif /* HAVE_B_BEB */
  /* Initialise listener list. */
  bridge->vlan_listener_list = list_new();

#ifdef HAVE_IGMP_SNOOP
  nsm_l2_mcast_vlan_init (bridge, bridge->master);
#endif /* HAVE_IGMP_SNOOP */

  /* Add default VLAN. */
#ifdef HAVE_PROVIDER_BRIDGE
  if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
      || bridge->provider_edge == PAL_TRUE)
    {
      ret = nsm_vlan_add (bridge->master, bridge->name, "default",
                          NSM_VLAN_DEFAULT_VID, NSM_VLAN_ACTIVE,
                          NSM_VLAN_CVLAN | NSM_VLAN_STATIC);

      if (ret < 0)
        {
          avl_tree_free (&bridge->vlan_table, NULL);
          avl_tree_free (&bridge->svlan_table, NULL);
          bridge->vlan_table = NULL;
          bridge->svlan_table = NULL;
          return ret;
        }
    }

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    {
      ret = nsm_vlan_add (bridge->master, bridge->name, "default",
                          NSM_VLAN_DEFAULT_VID, NSM_VLAN_ACTIVE,
                          NSM_VLAN_SVLAN | NSM_VLAN_STATIC);

      if (ret < 0)
        {
          avl_tree_free (&bridge->vlan_table, NULL);
          avl_tree_free (&bridge->svlan_table, NULL);
          bridge->vlan_table = NULL;
          bridge->svlan_table = NULL;
          return ret;
        }
    }

#else
  ret = nsm_vlan_add (bridge->master, bridge->name, "default",
                      NSM_VLAN_DEFAULT_VID, NSM_VLAN_ACTIVE,
                      NSM_VLAN_CVLAN | NSM_VLAN_STATIC);

  if (ret < 0)
    {
      avl_tree_free (&bridge->vlan_table, NULL);
      avl_tree_free (&bridge->svlan_table, NULL);
      bridge->vlan_table = NULL;
      bridge->svlan_table = NULL;
      return ret;
    }

#endif /* HAVE_PROVIDER_BRIDGE */

  return ret;
}

/* Deinitialize VLAN table. */
int
nsm_vlan_deinit_by_type (struct nsm_bridge *bridge, u_char vlan_type)
{
  struct avl_tree *vlan_table;
  struct avl_node *tmp_node;
  struct avl_node *node;
  struct nsm_vlan *vlan;
/*
  vlan_table = vlan_type == NSM_VLAN_SVLAN ? bridge->svlan_table :
#ifdef HAVE_B_BEB    
                            NSM_VLAN_BVLAN ? bridge->bvlan_table :
#endif                            
                                             bridge->vlan_table;*/

  if (CHECK_FLAG(vlan_type,NSM_VLAN_CVLAN))
     vlan_table = bridge->vlan_table;
  else if (CHECK_FLAG(vlan_type,NSM_VLAN_SVLAN))
     vlan_table = bridge->svlan_table;
#ifdef HAVE_B_BEB
  else if (CHECK_FLAG(vlan_type,NSM_VLAN_BVLAN))
     vlan_table = bridge->bvlan_table;
#endif /*HAVE_B_BEB*/
  else
     vlan_table = NULL;

  /* Free VLAN table. */
  if (vlan_table == NULL)
    return 0;

  node = vlan_table->root;

  while (node)
    {
      if (node->left)
        {
          node = node->left;
          continue;
        }

      if (node->right)
        {
          node = node->right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          vlan = tmp_node->info;

          if (vlan)
            {
#ifdef HAVE_HA
              list_free (vlan->port_cdr_ref_list);
#endif /* HAVE_HA */
              list_free (vlan->port_list);
#ifdef HAVE_PROVIDER_BRIDGE
              if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
                nsm_vlan_if_unset (bridge->master->nm, vlan->vid,
                                   vlan->vid, vlan_type, bridge->name);

              /* Delete VLAN from hardware. */
#ifdef HAVE_HAL
              hal_vlan_delete (bridge->name,
                               vlan_type == NSM_VLAN_CVLAN ?
                               HAL_VLAN_TYPE_CVLAN:
                               HAL_VLAN_TYPE_SVLAN, vlan->vid);
#endif /* HAVE_HAL */

              if (vlan->static_fdb_list)
                {
                  nsm_bridge_free_static_list (vlan->static_fdb_list);
                  ptree_finish (vlan->static_fdb_list);
                }
#ifdef HAVE_HA
              nsm_bridge_cal_delete_nsm_vlan (vlan);
#endif /* HAVE_HA */
              XFREE (MTYPE_VLAN, vlan);
            }

          if (node->left == tmp_node)
            node->left = NULL;
          else
            node->right = NULL;

          XFREE (MTYPE_AVL_TREE_NODE, tmp_node);
        }
      else
        {
          vlan = tmp_node->info;
          if (vlan)
            {
              list_free (vlan->port_list);
#ifdef HAVE_PROVIDER_BRIDGE
              if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
                nsm_vlan_if_unset (bridge->master->nm, vlan->vid,
                                   vlan->vid, vlan_type, bridge->name);

              if (vlan->static_fdb_list)
                {
                  nsm_bridge_free_static_list (vlan->static_fdb_list);
                  ptree_finish (vlan->static_fdb_list);
                }
#ifdef HAVE_HA
              nsm_bridge_cal_delete_nsm_vlan (vlan);
#endif /* HAVE_HA */
              XFREE (MTYPE_VLAN, vlan);
            }

          XFREE (MTYPE_AVL_TREE_NODE,tmp_node);
          break;
        }
    }

  XFREE (MTYPE_AVL_TREE, vlan_table);

#ifdef HAVE_IGMP_SNOOP
  nsm_l2_mcast_vlan_deinit (bridge->master, bridge->name);
#endif /* HAVE_IGMP_SNOOP */
  
  return 0;
}

int
nsm_vlan_deinit (struct nsm_bridge *bridge)
{
  nsm_vlan_deinit_by_type (bridge, NSM_VLAN_CVLAN);
  bridge->vlan_table = NULL;

  nsm_vlan_deinit_by_type (bridge, NSM_VLAN_SVLAN);
  bridge->svlan_table = NULL;
#ifdef HAVE_B_BEB
  nsm_vlan_deinit_by_type (bridge, NSM_VLAN_BVLAN);
  bridge->bvlan_table = NULL;
#endif
#ifdef HAVE_IGMP_SNOOP
  nsm_l2_mcast_vlan_deinit (bridge->master, bridge->name);
#endif /* HAVE_IGMP_SNOOP */

  if (bridge->vlan_listener_list)
  {
    list_delete(bridge->vlan_listener_list);
    bridge->vlan_listener_list = NULL;
  }
  return 0;
}


/* NSM server send message. */
int
nsm_vlan_send_vlan_msg (struct nsm_bridge *bridge,
                        struct nsm_msg_vlan *msg,
                        int msgid)
{
  int nbytes;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_vlan_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }

  return 0;
}

#ifdef HAVE_MSTPD
int
nsm_server_recv_vlan_add_to_instance(struct nsm_msg_header *header,
                                     void *arg, void *message)
{
  struct nsm_bridge_master *master;
  struct avl_tree *vlan_table;
  struct nsm_server_entry *nse;
  struct nsm_msg_vlan *msg;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_bridge *br;
  struct avl_node *rn;
  struct nsm_vlan p;
  struct nsm_vlan *vlan;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_vlan_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  master = nsm_bridge_get_master(nm);
  br = nsm_lookup_bridge_by_name(master, msg->bridge_name);
  if ( !br )
    return 0;

  if ( nsm_bridge_is_vlan_aware(master, br->name) != PAL_TRUE  )
    return 0;

  if (NSM_BRIDGE_TYPE_PROVIDER (br))
    vlan_table = br->svlan_table;
  else
    vlan_table = br->vlan_table;

  if (! vlan_table)
    return 0;

  NSM_VLAN_SET (&p, msg->vid);

  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {
      vlan = rn->info;
      /* Add VLAN. */
      if ( vlan )
        vlan->instance = msg->br_instance;

      hal_bridge_add_vlan_to_instance(br->name, msg->br_instance, msg->vid);
    }


  return 0;
}

/* VLAN delete. */
int
nsm_server_recv_vlan_delete_from_instance (struct nsm_msg_header *header,
                                           void *arg, void *message)
{
  struct nsm_bridge_master *master;
  struct avl_tree *vlan_table;
  struct nsm_server_entry *nse;
  struct nsm_msg_vlan *msg;
  struct nsm_master *nm;
  struct nsm_server *ns;
  struct nsm_bridge *br;
  struct avl_node *rn;
  struct nsm_vlan p;
  struct nsm_vlan *vlan;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = message;

  /* Debug.  */
  if (IS_NSM_DEBUG_RECV)
    nsm_vlan_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;
  
  master = nsm_bridge_get_master(nm);
  br = nsm_lookup_bridge_by_name(master, msg->bridge_name);
  if ( !br )
    return 0;

  if ( nsm_bridge_is_vlan_aware(master, br->name) != PAL_TRUE  )
    return 0;

  if (NSM_BRIDGE_TYPE_PROVIDER (br))
    vlan_table = br->svlan_table;
  else
    vlan_table = br->vlan_table;

  if (! vlan_table)
    return 0;

  NSM_VLAN_SET (&p, msg->vid);

  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {
      vlan = rn->info;
      if ( vlan )
        vlan->instance = 0;

      /* Add VLAN. */
      hal_bridge_delete_vlan_from_instance(br->name, msg->br_instance, msg->vid);
    }

  return 0;
}
#endif /* HAVE_MSTPD */

/* Set NSM server callbacks for VLANs. */
int
nsm_vlan_set_server_callback (struct nsm_server *ns)
{
#ifdef HAVE_MSTPD
  nsm_server_set_callback (ns, NSM_MSG_VLAN_ADD_TO_INST,
                           nsm_parse_vlan_msg,
                           nsm_server_recv_vlan_add_to_instance);
  nsm_server_set_callback (ns, NSM_MSG_VLAN_DELETE_FROM_INST,
                           nsm_parse_vlan_msg,
                           nsm_server_recv_vlan_delete_from_instance);

#endif /* HAVE_MSTPD */
  return 0;
}

/* NSM Server send vlan port type message. */
int
nsm_vlan_send_port_type_msg (struct nsm_bridge *bridge,
                             struct nsm_msg_vlan_port_type *msg)
{
  int nbytes;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_vlan_port_type_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_VLAN_PORT_TYPE, 0, nbytes);

      }
  return 0;
}

/* NSM server send interface add/delete vlan message. */
int
nsm_vlan_send_port_map (struct nsm_bridge *bridge, 
                        struct nsm_msg_vlan_port *msg, int msgid)
{
  int nbytes;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_vlan_port_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send VLAN port add/delete message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }

  return 0;
}

#if (defined HAVE_I_BEB || defined HAVE_B_BEB )
/* NSM server send isid2svid map */
int
nsm_pbb_send_isid2svid(struct nsm_bridge * bridge,
                       struct nsm_msg_pbb_isid_to_pip *msg,
                       int msgid )
{
  int nbytes;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_isid2svid_msg(&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send VLAN port add/delete message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }

  return 0;
}

/* NSM server send isid2bvid map */
int
nsm_pbb_send_isid2bvid(struct nsm_bridge * bridge,
    struct nsm_msg_pbb_isid_to_bvid *msg,
    int msgid )
{
    int nbytes;
    struct nsm_server_client *nsc;
    struct nsm_server_entry *nse;
    int i;

    NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
      if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
        {
          /* Set nse pointer and size. */
          nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
          nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

          /* Encode NSM bridge message. */
          nbytes = nsm_encode_isid2bvid_msg(&nse->send.pnt, &nse->send.size, msg);
          if (nbytes < 0)
            return nbytes;

          /* Send VLAN port add/delete message. */
          nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
        }

    return 0;
}

#endif /* HAVE_I_BEB || HAVE_B_BEB  */


/* Add ifp to a vlan port list. */
int
nsm_vlan_add_port (struct nsm_bridge *bridge, struct interface *ifp, nsm_vid_t vid,
                   enum nsm_vlan_egress_type egress_tagged,
                   bool_t notify_kernel)
{
  struct listnode *ln;
  struct nsm_vlan p;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct interface *vlanifp;
  struct nsm_msg_vlan_port msg;
  struct nsm_if *zif, *vlan_zif;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_listener *vlanlnr;
#ifdef HAVE_HA
  struct nsm_vlan_port_cdr_ref_node *vlan_port_cdr_ref_node;
#endif /* HAVE_HA */

#ifdef HAVE_ELMID
  cindex_t cindex = 0;
  struct nsm_svlan_reg_info *svlan_info = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL; 
#endif /* HAVE_ELMID */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

#if defined HAVE_I_BEB || defined HAVE_B_BEB
  if (vlan_port->mode == NSM_VLAN_PORT_MODE_CN ||
      vlan_port->mode == NSM_VLAN_PORT_MODE_CNP ||
      vlan_port->mode == NSM_VLAN_PORT_MODE_PIP ||
      (vlan_port->mode == NSM_VLAN_PORT_MODE_PN && 
       (bridge->type != NSM_BRIDGE_TYPE_BACKBONE_RSTP &&
        bridge->type != NSM_BRIDGE_TYPE_BACKBONE_MSTP)))
    vlan_table = bridge->svlan_table;
#ifdef HAVE_B_BEB
  else if (vlan_port->mode == NSM_VLAN_PORT_MODE_CBP ||
       (vlan_port->mode == NSM_VLAN_PORT_MODE_PNP && 
        (bridge->type == NSM_BRIDGE_TYPE_BACKBONE_RSTP ||
        bridge->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP)))
      vlan_table = bridge->bvlan_table;
#endif /* HAVE_B_BEB */
  else
    vlan_table = bridge->vlan_table;

#else
  
  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;
#endif /**/

  NSM_VLAN_SET(&p, vid);

  if (! vlan_table)
    return 0;

  rn = avl_search (vlan_table, (void *)&p);

  if (rn && rn->info)
    {

      vlan = rn->info;
      /* Add vid to port in Hardware */
#ifdef HAVE_HAL
      if (notify_kernel == PAL_TRUE)
        {
          switch(vlan_port->mode)
           {
             case NSM_VLAN_PORT_MODE_ACCESS:
               hal_vlan_add_vid_to_port (bridge->name, ifp->ifindex, vid,
                                         HAL_VLAN_EGRESS_UNTAGGED);
               break;

             case NSM_VLAN_PORT_MODE_TRUNK:
             case NSM_VLAN_PORT_MODE_PN:
               hal_vlan_add_vid_to_port (bridge->name, ifp->ifindex, vid, 
                                         HAL_VLAN_EGRESS_TAGGED);
               break;

             case NSM_VLAN_PORT_MODE_HYBRID:
             case NSM_VLAN_PORT_MODE_CN:
               if (egress_tagged == NSM_VLAN_EGRESS_TAGGED)
                 {
                   hal_vlan_add_vid_to_port (bridge->name, ifp->ifindex, vid, 
                                             HAL_VLAN_EGRESS_TAGGED);
                 }
               else
                 {
                   hal_vlan_add_vid_to_port (bridge->name, ifp->ifindex, vid, 
                                             HAL_VLAN_EGRESS_UNTAGGED);
                 }
               break;
             case NSM_VLAN_PORT_MODE_CE:
               /* For CUSTOMER-EDGE port C VLAN will be added when Mapping
                * is created for the port */
               break;
#if defined HAVE_I_BEB || defined HAVE_B_BEB
             case NSM_VLAN_PORT_MODE_CNP:
             case NSM_VLAN_PORT_MODE_PIP:
             case NSM_VLAN_PORT_MODE_CBP:
             /*case NSM_VLAN_PORT_MODE_PNP: same as PN */
               hal_vlan_add_vid_to_port (bridge->name, ifp->ifindex, vid, 
                                         HAL_VLAN_EGRESS_TAGGED);
               break;
#endif /* HAVE_I_BEB || HAVE_B_BEB */
             default:
               /* Invalid Mode */
               return NSM_VLAN_ERR_INVALID_MODE;
               break;
           }
        }
#endif /* HAVE_HAL */

#ifdef HAVE_HA
      vlan_port_cdr_ref_node = vlan_port_cdr_ref_lookup (vlan,
                                                         ifp->ifindex);

       if (vlan_port_cdr_ref_node)
         {
           nsm_bridge_cal_delete_nsm_vlan_port_association
                                  (vlan, br_port,
                                   vlan_port_cdr_ref_node);

           listnode_delete (vlan->port_cdr_ref_list, vlan_port_cdr_ref_node);
           vlan_port_cdr_ref_node = NULL;
         }
#endif /* HAVE_HA */

      /* Delete previous one incase it exists. */
      listnode_delete (vlan->port_list, ifp);

      /* Add this port to the vid. */
      listnode_add (vlan->port_list, ifp);

#ifdef HAVE_HA
      if (vlan_port_cdr_ref_node == NULL)
         vlan_port_cdr_ref_node = 
                     XCALLOC (MTYPE_TMP,
                              sizeof (struct nsm_vlan_port_cdr_ref_node));

      if (vlan_port_cdr_ref_node == NULL)
        return -1;

      vlan_port_cdr_ref_node->ifindex = ifp->ifindex;

      listnode_add (vlan->port_cdr_ref_list, vlan_port_cdr_ref_node);

      nsm_bridge_cal_create_nsm_vlan_port_association (vlan, br_port,
                                                       vlan_port_cdr_ref_node);
#endif /* HAVE_HA */ 

      /* Send message to PM for VLAN addition. */

      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
      msg.ifindex = ifp->ifindex;
      msg.num = 1;
      msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

      msg.vid_info[0] = vid;

      nsm_vlan_send_port_map(bridge, &msg, NSM_MSG_VLAN_ADD_PORT);

      XFREE (MTYPE_TMP, msg.vid_info);

#ifdef HAVE_PROVIDER_BRIDGE
      if (bridge->provider_edge == PAL_TRUE)
        {
          if (vlan_port->mode == NSM_VLAN_PORT_MODE_PN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_CN)
            nsm_pro_edge_add_port_to_swctx (bridge, ifp, vlan);
        }

#ifdef HAVE_ELMID
      if (CHECK_FLAG (vlan->type, NSM_VLAN_SVLAN))
        {
          cindex = 0;
          regtab = vlan_port->regtab;
          if (regtab)
            {
              if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, vid))
                {
                  if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                          vid)) != NULL)
                    {
                      NSM_SET_CTYPE (cindex, NSM_ELMI_CTYPE_EVC_TYPE);
                      nsm_server_evc_update (ifp, vid, svlan_info, cindex);
                    }
                }
            }
        }
#endif /* HAVE_ELMID */

#endif /* HAVE_RROVIDER_BRIDGE */
      /* Notify to all listeners */
      LIST_LOOP(bridge->vlan_listener_list, vlanlnr, ln)
        {
          if( (vlanlnr) &&
              (vlanlnr->add_vlan_to_port_func) )
            {   
              if(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vid))
                {
                  /* If port is dynamic member of the vlan report it only to
                     listeners interested in dynamic vlan registrations */
                  if ( (vlanlnr->flags & NSM_VLAN_DYNAMIC) )
                    (vlanlnr->add_vlan_to_port_func)(bridge->master, 
                                                     bridge->name, ifp, vid);
                }
              else
                {
                  /* Report to all listeners */
                  (vlanlnr->add_vlan_to_port_func)(bridge->master, 
                                                   bridge->name, ifp, vid);
                } /* end not member dynamic */
            } /* end vlan listener exists */
        } /* end LIST_LOOP */

      /* add ip-access-group if vlan is already configured */
      vlanifp = vlan->ifp;

      if (!vlanifp || !vlanifp->info)
        return 0;

      vlan_zif = vlanifp->info;
      if (vlan_zif && (vlan_zif->acl_name_str && vlan_zif->acl_dir_str))
        nsm_filter_set_interface_port_access_group (bridge->master->nm->vr,
                                                    vlan_zif->acl_name_str,
                                                    vlan_zif->acl_dir_str,
                                                    vlan->ifp->name, ifp,
                                                    PAL_TRUE);
    }

  return 0;
}

/* Delete ifp from a vlan port list. */
int
nsm_vlan_delete_port (struct nsm_bridge *bridge, struct interface *ifp,
                      nsm_vid_t vid, bool_t notify_kernel)
{
  struct listnode *ln;
  struct nsm_vlan p;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct interface *vlanifp;
  struct nsm_msg_vlan_port msg;
  struct nsm_if *zif, *vlan_zif;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_listener *vlanlnr;
#ifdef HAVE_HA
  struct nsm_vlan_port_cdr_ref_node *vlan_port_cdr_ref_node;
#endif /* HAVE_HA */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  vlanifp = NULL;
  vlan_zif = NULL;

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
#ifdef HAVE_B_BEB
               || ((vlan_port->mode == NSM_VLAN_PORT_MODE_PN)
                   && ((bridge->type != NSM_BRIDGE_TYPE_BACKBONE_MSTP)
                   &&(bridge->type != NSM_BRIDGE_TYPE_BACKBONE_RSTP )))
#else
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN
#endif /* HAVE_B_BEB */
               || vlan_port->mode == NSM_VLAN_PORT_MODE_CNP
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PIP ?
               bridge->svlan_table :
#ifdef HAVE_B_BEB
               vlan_port->mode == NSM_VLAN_PORT_MODE_CBP
               || ((vlan_port->mode == NSM_VLAN_PORT_MODE_PNP)
                   &&((bridge->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP )
                    ||(bridge->type == NSM_BRIDGE_TYPE_BACKBONE_RSTP))) ?
               bridge->bvlan_table :
#endif
               bridge->vlan_table;

  NSM_VLAN_SET(&p, vid);

  if (! vlan_table)
    return 0;

  rn = avl_search (vlan_table, (void *)&p);

  if (rn && rn->info)
    {
      vlan = rn->info;

      /* If not static or dynamic, delete this port from vlan port list. */
      if (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid))
          && !(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vid)))
        {
          /* delete ip-access-group if vlan is already configured*/
          vlanifp = vlan->ifp;

          if (vlanifp)
             vlan_zif = vlanifp->info;

          if (vlan_zif && (vlan_zif->acl_name_str && vlan_zif->acl_dir_str))
            nsm_filter_set_interface_port_access_group (bridge->master->nm->vr,
                                                        vlan_zif->acl_name_str,
                                                        vlan_zif->acl_dir_str,
                                                        vlan->ifp->name, ifp,
                                                        PAL_FALSE);

          /* Remove vid from the port in  hardware */
#ifdef HAVE_HAL
          if (notify_kernel == PAL_TRUE)
          hal_vlan_delete_vid_from_port (bridge->name, ifp->ifindex, vid);
#endif /* HAVE_VLAN */

          pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
          msg.ifindex = ifp->ifindex;
          msg.num = 1;
          msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);
          msg.vid_info[0] = vid;

          nsm_vlan_send_port_map(bridge, &msg, NSM_MSG_VLAN_DELETE_PORT);
          XFREE (MTYPE_TMP, msg.vid_info);

#ifdef HAVE_PROVIDER_BRIDGE
         if (bridge->provider_edge == PAL_TRUE)
           {
              if (vlan_port->mode == NSM_VLAN_PORT_MODE_PN
                  || vlan_port->mode == NSM_VLAN_PORT_MODE_CN)
                nsm_pro_edge_del_port_from_swctx (bridge, ifp, vlan);
           }
         else
#endif /* HAVE_RROVIDER_BRIDGE */
         /* Notify to all listeners */
         LIST_LOOP(bridge->vlan_listener_list, vlanlnr, ln)
           {
             if( (vlanlnr) &&
                 (vlanlnr->delete_vlan_from_port_func) )
               {   
                  if(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp,
                                             vid))
                    {
                      /* If port is dynamic member of the vlan report
                         it only to listeners interested in dynamic vlan
                         registrations */
                      if ( (vlanlnr->flags & NSM_VLAN_DYNAMIC) )
                           (vlanlnr->delete_vlan_from_port_func)
                                      (bridge->master,
                                       bridge->name, ifp,
                                       vid);
                   }
                 else
                   {
                     /* Report to all listeners */
                     (vlanlnr->delete_vlan_from_port_func)
                               (bridge->master, 
                                bridge->name, ifp, vid);
                   } /* end not member dynamic */
               } /* end vlan listener exists */
           } /* end LIST_LOOP */

#ifdef HAVE_PROVIDER_BRIDGE
          /* Delete all static fdb entries learnt for this vlan */
          if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
            nsm_bridge_delete_port_fdb_by_vid (bridge, zif, vid);

#ifdef HAVE_HA
          vlan_port_cdr_ref_node = vlan_port_cdr_ref_lookup (vlan,
                                                             ifp->ifindex);
          if (vlan_port_cdr_ref_node)
            {
              nsm_bridge_cal_delete_nsm_vlan_port_association
                                               (vlan, br_port,
                                                vlan_port_cdr_ref_node);
              listnode_delete (vlan->port_cdr_ref_list, vlan_port_cdr_ref_node);
              vlan_port_cdr_ref_node = NULL;
            }
#endif /* HAVE_HA */ 
          /* Delete this port from the vid. */
          listnode_delete (vlan->port_list, ifp);
        }

    }

  return 0;
}

#ifdef HAVE_SMI
/* Clear the vlan list from port. */
int
nsm_api_clear_vlan_port (struct nsm_bridge *bridge, struct interface *ifp,
                         struct smi_vlan_bmp vlan_bmp)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  int vid;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  if (! vlan_port)
    return NSM_VLAN_ERR_VLAN_NOT_IN_PORT;
    
  for (vid = NSM_VLAN_CLI_MIN; vid <= NSM_VLAN_MAX; vid++)
  {
    if (!SMI_BMP_IS_MEMBER (vlan_bmp, vid))
      continue;

    NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vid);
    NSM_VLAN_BMP_UNSET (vlan_port->dynamicMemberBmp, vid);
  }
#ifdef HAVE_HA
  nsm_bridge_cal_create_nsm_bridge_port (br_port);
#endif /*HAVE_HA*/
  return SMI_SUCEESS;
}
#endif /* HAVE_SMI */

/* Set a port in default VLAN. */
int
nsm_vlan_add_default (struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;

  zif = (struct nsm_if *)(ifp->info);

  if ( (!zif) || (!zif->bridge) )
    return 0;

  bridge = zif->bridge;

  if ( (bridge->type != NSM_BRIDGE_TYPE_STP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_RSTP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_RPVST_PLUS) &&
       (bridge->type != NSM_BRIDGE_TYPE_MSTP) )
    {
      return -1;
    }

  /* Currently the only place add default is called is in set
     switchport which the lacp aggregator inherits from the member
     hence we do not need to iterate over the members here */
  /* Set port to access. */

  NSM_VLAN_SET_PORT_DEFAULT_MODE (ifp, PAL_FALSE, PAL_TRUE);

  return 0;
}

/* Clear this port from all VLAN memberships. */
int
nsm_vlan_clear_port (struct interface *ifp)
{
  nsm_vid_t vid;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  if (! zif || ! zif->bridge || ! zif->switchport)
    return 0;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name)
                                != PAL_TRUE  )
    {
      return -1;
    }

#ifdef HAVE_PROVIDER_BRIDGE

  if (br_port->trans_tab)
    nsm_vlan_trans_tab_delete (ifp);

  if (vlan_port->regtab)
    nsm_cvlan_reg_tab_if_delete (ifp);

#endif /* HAVE_PROVIDER_BRIDGE */

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;

  /* Clear static vlans. */
  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
    {
      /* First unset this vid. */
      NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vid);

      nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);
    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);

  /* Clear dynamic vlans. */
  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->dynamicMemberBmp, vid)
    {
      /* First unset this vid. */
      NSM_VLAN_BMP_UNSET (vlan_port->dynamicMemberBmp, vid);

      nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);
    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->dynamicMemberBmp, vid);

  NSM_VLAN_BMP_CLEAR(vlan_port->egresstaggedBmp);
  pal_mem_set (vlan_port, 0, sizeof (struct nsm_vlan_port));

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  return 0;
}

/* Set default mode. */
static int
nsm_vlan_set_default (struct nsm_bridge *bridge, struct interface *ifp, 
                      int mode, nsm_vid_t vid,
                      bool_t notify_kernel)
{
  struct nsm_if *zif;
  struct nsm_msg_vlan_port msg;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  enum nsm_vlan_egress_type egress_tagged = NSM_VLAN_EGRESS_UNTAGGED;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  /* clear the static vlan membership for all vlans associated to the port
   * if port is access type
   */
  if (mode == NSM_VLAN_PORT_MODE_ACCESS)
    NSM_VLAN_BMP_CLEAR(vlan_port->staticMemberBmp);

  vlan_port->mode = mode;
  vlan_port->pvid = vid;

#ifdef HAVE_MAC_AUTH
  if (! NSM_VLAN_BMP_IS_MEMBER(vlan_port->authmacMemberBmp, vid))
    NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, vid);
#else
    NSM_VLAN_BMP_SET (vlan_port->staticMemberBmp, vid);
#endif /* HAVE_MAC_AUTH */

  /* Set all ports as egress untagged if the port is access. */
  if (mode == NSM_VLAN_PORT_MODE_ACCESS)
    NSM_VLAN_BMP_CLEAR(vlan_port->egresstaggedBmp);

  /* Set the corresponding hybrid port as egress untagged */
  if (mode == NSM_VLAN_PORT_MODE_HYBRID)
    NSM_VLAN_BMP_UNSET (vlan_port->egresstaggedBmp, vid);

  if (NSM_VLAN_BMP_IS_MEMBER(vlan_port->egresstaggedBmp, vid))
    egress_tagged = NSM_VLAN_EGRESS_TAGGED;
  else
    egress_tagged = NSM_VLAN_EGRESS_UNTAGGED;

  /* Add this port to new VID. */
  nsm_vlan_add_port (bridge, ifp, vid, egress_tagged, notify_kernel);

  /* Activate the static fdb configuration for the vlan. */

  nsm_bridge_static_fdb_config_activate (bridge->master, bridge->name, 
                                         vid, ifp);
#ifdef HAVE_HAL
  if (notify_kernel == PAL_TRUE)
    {
  switch(mode)
    {
    case NSM_VLAN_PORT_MODE_ACCESS:
      hal_vlan_set_default_pvid (bridge->name, ifp->ifindex, vid, 
                                 HAL_VLAN_EGRESS_UNTAGGED);
      break;
    case NSM_VLAN_PORT_MODE_TRUNK:
    case NSM_VLAN_PORT_MODE_PN:
      hal_vlan_set_default_pvid (bridge->name, ifp->ifindex, vid, 
                                 HAL_VLAN_EGRESS_TAGGED);
      break;
    case NSM_VLAN_PORT_MODE_HYBRID:
    case NSM_VLAN_PORT_MODE_CE:
    case NSM_VLAN_PORT_MODE_CN:
      if (egress_tagged == NSM_VLAN_EGRESS_TAGGED)
        {
          hal_vlan_set_default_pvid (bridge->name, ifp->ifindex, vid, 
              HAL_VLAN_EGRESS_TAGGED);
        }
      else
        {
          hal_vlan_set_default_pvid (bridge->name, ifp->ifindex, vid, 
              HAL_VLAN_EGRESS_UNTAGGED);
        }
      break;
#ifdef HAVE_I_BEB 
    case NSM_VLAN_PORT_MODE_CNP:
    case NSM_VLAN_PORT_MODE_PIP:
      hal_vlan_set_default_pvid (bridge->name, ifp->ifindex, vid, 
                                     HAL_VLAN_EGRESS_TAGGED);
      break;
    /* case NSM_VLAN_PORT_MODE_PNP: same as PN */
#endif /* HAVE_I_BEB */
#ifdef HAVE_B_BEB
    case NSM_VLAN_PORT_MODE_CBP:
      hal_vlan_set_default_pvid (bridge->name, ifp->ifindex, vid, 
                                     HAL_VLAN_EGRESS_TAGGED);
      break;

#endif /* HAVE_B_BEB */
     default:
      /* Invalid Mode */
      return NSM_VLAN_ERR_INVALID_MODE;
      break;
    }
    }
#endif /* HAVE_HAL */

  /* Send message to PM for PVID Update. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
  msg.ifindex = ifp->ifindex;
  msg.num = 1;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);
  msg.vid_info[0] = vid;

  nsm_vlan_send_port_map (bridge, &msg, NSM_MSG_VLAN_SET_PVID);
  XFREE (MTYPE_TMP, msg.vid_info);

  return 0;
}

/* Clear access. */
static int
nsm_vlan_clear_access_port (struct interface *ifp, bool_t notify_kernel)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_ACCESS)
    return -1;

  /* Delete this port from the oldvid. */
  NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vlan_port->pvid);
  NSM_VLAN_BMP_UNSET (vlan_port->egresstaggedBmp, vlan_port->pvid);
  nsm_vlan_delete_port (bridge, ifp, vlan_port->pvid, notify_kernel);

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  return 0;
}

/* Common function for clearing trunk or hybrid port. */
static int
nsm_vlan_clear_common_port (struct interface *ifp, enum nsm_vlan_port_mode mode,
                            bool_t iterate_members, bool_t notify_kernel)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
#ifdef HAVE_LACPD
  struct nsm_if *nif = ifp->info;
  struct interface *memifp;
  struct listnode *ln;
  int memret;
#endif /* HAVE_LACPD */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (vlan_port->mode != mode)
    return NSM_VLAN_ERR_INVALID_MODE;

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
    {
#ifdef  HAVE_SWFWDR
      nsm_clear_vlans (bridge, ifp, &vlan_port->staticMemberBmp, vlan_port,
                       notify_kernel);
      nsm_clear_vlans (bridge, ifp, &vlan_port->dynamicMemberBmp,vlan_port,
                       notify_kernel);
#else
      nsm_clear_vlans (bridge, ifp, &vlan_port->staticMemberBmp, vlan_port,
                       PAL_FALSE);
      nsm_clear_vlans (bridge, ifp, &vlan_port->dynamicMemberBmp,vlan_port,
                       PAL_FALSE);
#endif /* HAVE_SWFWDR */

      /* Set VLAN port mode to invalid. */
      vlan_port->mode = NSM_VLAN_PORT_MODE_INVALID;

      if (iterate_members == PAL_TRUE)
        {
          AGGREGATOR_MEMBER_LOOP (nif, memifp, ln)
          {
            NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY (memifp);
            NSM_BRIDGE_CHECK_AGG_MEM_BRIDGE_PROPERTY (memifp);

            memifp->agg_param_update = 1;
            memret = nsm_vlan_clear_common_port (memifp, mode, PAL_FALSE, PAL_TRUE);
            memifp->agg_param_update = 0;

            if (memret < 0)
              return memret;
          }
        }

      NSM_VLAN_SET_PORT_DEFAULT_MODE (ifp, PAL_FALSE, PAL_FALSE);
    }
  else
#endif /* HAVE_LACPD */
    {
      nsm_clear_vlans (bridge, ifp, &vlan_port->staticMemberBmp, vlan_port,
                       notify_kernel);
      nsm_clear_vlans (bridge, ifp, &vlan_port->dynamicMemberBmp,vlan_port,
                       notify_kernel);

#ifdef HAVE_ELMID
      /* Send UNI delete msg to ELMI */
      if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE)
        nsm_server_uni_delete (ifp); 
#endif /* HAVE_ELMID */

      /* Set VLAN port mode to invalid. */
      vlan_port->mode = NSM_VLAN_PORT_MODE_INVALID;

      /* Set port to default mode. */

      NSM_VLAN_SET_PORT_DEFAULT_MODE (ifp, PAL_FALSE, notify_kernel);

    } /* end else #ifdef HAVE_LACPD */

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  return 0;
}

void
nsm_clear_vlans (struct nsm_bridge *bridge, struct interface *ifp,
                 struct nsm_vlan_bmp *bmp, struct nsm_vlan_port *vlan_port,
                 bool_t notify_kernel)
{
  nsm_vid_t vid = 0;
  int count = 0;

  /* Get the count of the VIDs configured. */
  NSM_VLAN_SET_BMP_ITER_BEGIN(*bmp, vid)
    {
      count++;
    }
  NSM_VLAN_SET_BMP_ITER_END(*bmp, vid);

  /* Send message to PM for VLAN port addition. */
  if (count > 0)
    {
      /* Iterate through all the vlans and
       * delete the port from the vlan port list.
       */
      count = 0;
      NSM_VLAN_SET_BMP_ITER_BEGIN(*bmp, vid)
        {
          NSM_VLAN_BMP_UNSET (*bmp, vid);
          nsm_vlan_delete_port (bridge, ifp, vid, notify_kernel);
        }
      NSM_VLAN_SET_BMP_ITER_END(*bmp, vid);

      /* Clear bitmaps. */
      NSM_VLAN_BMP_CLEAR(*bmp);
      NSM_VLAN_BMP_CLEAR(vlan_port->egresstaggedBmp);

    }
  else
    {
      /* Clear bitmaps. */
      NSM_VLAN_BMP_CLEAR(*bmp);
      NSM_VLAN_BMP_CLEAR(vlan_port->egresstaggedBmp);
    }

}

/* Clear port and revert to access */
int
nsm_api_vlan_clear_port (struct interface *ifp, bool_t iterate_members,
                         bool_t notify_kernel)
{
  int ret = 0;
  struct nsm_if *zif;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_INVALID)
  {
    switch(vlan_port->mode)
    {
      case NSM_VLAN_PORT_MODE_ACCESS:
        ret = nsm_vlan_clear_access_port (ifp, notify_kernel);
        break;

      case NSM_VLAN_PORT_MODE_HYBRID:
        ret = nsm_vlan_clear_hybrid_port (ifp, iterate_members, notify_kernel);
        break;

      case NSM_VLAN_PORT_MODE_TRUNK:
        ret = nsm_vlan_clear_trunk_port (ifp, iterate_members, notify_kernel);
        break;

#ifdef HAVE_PROVIDER_BRIDGE
      case NSM_VLAN_PORT_MODE_CE:
      case NSM_VLAN_PORT_MODE_CN:
      case NSM_VLAN_PORT_MODE_PN:
        ret = nsm_vlan_clear_provider_port (ifp, vlan_port->mode,
                                            iterate_members, notify_kernel);
        if (br_port->trans_tab)
          nsm_vlan_trans_tab_delete (ifp);

        break;
#endif /* HAVE_PROVIDER_BRIDGE */

      case NSM_VLAN_PORT_MODE_INVALID:
        break;

      default:
        return NSM_VLAN_ERR_INVALID_MODE;
    }

    if (ret < 0)
    {
      return NSM_VLAN_ERR_MODE_CLEAR;
    }
  }
  return (ret);
}

/* Clear hybrid and revert a link to access. */
int
nsm_vlan_clear_hybrid_port (struct interface *ifp, bool_t iterate_members,
                            bool_t notify_kernel)
{
  return nsm_vlan_clear_common_port (ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                     iterate_members, notify_kernel);  
}

/* Clear trunk. */
int
nsm_vlan_clear_trunk_port (struct interface *ifp, bool_t iterate_members,
                            bool_t notify_kernel)
{
  return nsm_vlan_clear_common_port (ifp, NSM_VLAN_PORT_MODE_TRUNK,
                                     iterate_members, notify_kernel);  
}

#ifdef HAVE_PROVIDER_BRIDGE

int
nsm_vlan_clear_provider_port (struct interface *ifp,
                              enum nsm_vlan_port_mode mode,
                              bool_t iterate_members,
                              bool_t notify_kernel)
{
  return nsm_vlan_clear_common_port (ifp, mode, iterate_members,
                                     notify_kernel);
}

#endif /* HAVE_PROVIDER_BRIDGE */

/* Add a VLAN to a list of port of a bridge. */
static int
nsm_vlan_add_all (struct nsm_bridge *bridge, nsm_vid_t vid, u_char type)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      vlan_port = &br_port->vlan_port;

      if (vlan_port->config == NSM_VLAN_CONFIGURED_ALL
          && ((type & NSM_VLAN_STATIC) || (type & NSM_VLAN_AUTO)))
        {
          /* Add new port->vlan mapping. */
          NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, vid);

          /* Vlan doesnt exist previously. Newly added vlan.
           * Set egresstaggedBmp for vlan_port. 
           */
          NSM_VLAN_BMP_SET (vlan_port->egresstaggedBmp, vid);

#ifdef HAVE_HA
          nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

          /* Add new vlan->port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
            if (NSM_INTF_TYPE_AGGREGATOR(ifp))
              nsm_vlan_add_port (bridge, ifp, vid, NSM_VLAN_EGRESS_TAGGED,
                                 PAL_FALSE);
            else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
          nsm_vlan_add_port (bridge, ifp, vid, NSM_VLAN_EGRESS_TAGGED, 
                             PAL_TRUE);

          /* Activate the static fdb configuration for the vlan. */

          nsm_bridge_static_fdb_config_activate (bridge->master, bridge->name, 
                                                 vid, ifp);

#ifdef HAVE_MPLS_VC
          /* If some vc bind to this vlan, start it */
          nsm_mpls_vc_vlan_bind (ifp, vid);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
          /* If some vpls bind to this vlan, start it */
          nsm_vpls_vlan_bind (ifp, vid);
#endif /* HAVE_VPLS */
        }
    }
  return 0;
}

/* Delete a VLAN from the list of ports from a bridge. */
static int
nsm_vlan_delete_all (struct nsm_bridge *bridge, nsm_vid_t vid)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;


  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      vlan_port = &br_port->vlan_port;

      if ( !vlan_port )
        continue;

      if ( !(NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp, vid)) &&
           !(NSM_VLAN_BMP_IS_MEMBER(vlan_port->dynamicMemberBmp, vid)) )
        {
          /* Port is not member of this vlan */
          continue;
        }

      switch (vlan_port->mode)
        {
        case NSM_VLAN_PORT_MODE_ACCESS:
          {
            vlan_port->pvid = NSM_VLAN_DEFAULT_VID;

#ifdef HAVE_VLAN_STACK
            /* Disable vlan stacking if configured. */ 
            if (vlan_port->vlan_stacking == NSM_TRUE)
                nsm_vlan_stacking_disable (ifp);
#endif /* HAVE_VLAN_STACK */
            /* Delete old port->vlan mapping. */
            NSM_VLAN_BMP_UNSET(vlan_port->staticMemberBmp, vid);
            NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

            /* Delete old vlan->port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
            if (NSM_INTF_TYPE_AGGREGATOR(ifp))
              nsm_vlan_delete_port (bridge, ifp, vid, PAL_FALSE); 
            else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
              nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);
   
            /* Add default port->vlan mapping. */
            NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, NSM_VLAN_DEFAULT_VID);

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

            /* Add default vlan->port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
            if (NSM_INTF_TYPE_AGGREGATOR(ifp))
              nsm_vlan_set_default (bridge, ifp, NSM_VLAN_PORT_MODE_ACCESS,
                                     NSM_VLAN_DEFAULT_VID, PAL_FALSE);
            else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
              nsm_vlan_set_default (bridge, ifp, NSM_VLAN_PORT_MODE_ACCESS,
                                    NSM_VLAN_DEFAULT_VID, PAL_TRUE);

          }
          break;
        case NSM_VLAN_PORT_MODE_HYBRID:
          {
            if (vlan_port->pvid == vid)
              {
                vlan_port->pvid = NSM_VLAN_DEFAULT_VID;

#ifdef HAVE_MPLS_VC
                /* If some vc bind to this vlan, start it */
                nsm_mpls_vc_vlan_unbind (ifp, vid);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
                /* If some vpls bind to this vlan, start it */
                nsm_vpls_vlan_unbind (ifp, vid);
#endif /* HAVE_VPLS */

                /* Delete old port->vlan mapping. */
                NSM_VLAN_BMP_UNSET(vlan_port->staticMemberBmp, vid);
                NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);

#ifdef HAVE_HA
                nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

                /* Delete old vlan->port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
                if (NSM_INTF_TYPE_AGGREGATOR(ifp))
                  nsm_vlan_delete_port (bridge, ifp, vid, PAL_FALSE);
                else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
                  nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);

                /* Add default port->vlan mapping. */
                NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, NSM_VLAN_DEFAULT_VID);
#ifdef HAVE_HA
                nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

                /* Add default vlan->port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
                if (NSM_INTF_TYPE_AGGREGATOR(ifp))
  		   nsm_vlan_set_default (bridge, ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                          NSM_VLAN_DEFAULT_VID, PAL_FALSE);
                else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
		    nsm_vlan_set_default (bridge, ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                          NSM_VLAN_DEFAULT_VID, PAL_FALSE);
              }
            else
              {
#ifdef HAVE_MPLS_VC
                /* If some vc bind to this vlan, deactivate it */
                nsm_mpls_vc_vlan_unbind (ifp, vid);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
                /* If some vpls bind to this vlan, start it */
                nsm_vpls_vlan_unbind (ifp, vid);
#endif /* HAVE_VPLS */

                /* Delete old port->vlan mapping. */
                NSM_VLAN_BMP_UNSET(vlan_port->staticMemberBmp, vid);
                NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);

                /* Delete old vlan->port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
                if (NSM_INTF_TYPE_AGGREGATOR(ifp))
                  nsm_vlan_delete_port (bridge, ifp, vid, PAL_FALSE);
                else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
                  nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);

              }
          }
          break;
        case NSM_VLAN_PORT_MODE_TRUNK:
          {
            /* Trunk port. */
#ifdef HAVE_MPLS_VC
            /* If some vc bind to this vlan, start it */
            nsm_mpls_vc_vlan_unbind (ifp, vid);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
            /* If some vpls bind to this vlan, start it */
            nsm_vpls_vlan_unbind (ifp, vid);
#endif /* HAVE_VPLS */

            /* Delete old port->vlan mapping. */
            NSM_VLAN_BMP_UNSET(vlan_port->staticMemberBmp, vid);
            NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

            /* Delete old vlan->port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
            if (NSM_INTF_TYPE_AGGREGATOR(ifp))
              nsm_vlan_delete_port (bridge, ifp, vid, PAL_FALSE);
            else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
              nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);

          }
          break;
        default:
          continue;
        }
    }

  return 0;
}

/* Static function to send a vlan add message to NSM clients. */
static int
_nsm_vlan_send_vlan_add_msg (struct nsm_bridge *bridge, struct nsm_vlan *vlan, 
                             enum nsm_vlan_state state, u_int32_t flags)
{
  struct nsm_msg_vlan msg;
  int ret;

  ret = 0;

  /* Send VLAN Add message to PM. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan));
  msg.cindex = 0;
  if (state == NSM_VLAN_ACTIVE)
    msg.flags |= NSM_MSG_VLAN_STATE_ACTIVE;
  msg.flags |= flags;
  msg.vid = vlan->vid;
  pal_strncpy (msg.vlan_name, vlan->vlan_name, NSM_VLAN_NAMSIZ);
  NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_NAME);
  pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
  NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
  
  /* Send VLAN add message to PM. */
  ret = nsm_vlan_send_vlan_msg (bridge, &msg, NSM_MSG_VLAN_ADD);

  return ret;
}

/* Static function to send a vlan delete message to NSM clients. */
static int
_nsm_vlan_send_vlan_delete_msg (struct nsm_bridge *bridge, struct nsm_vlan *vlan,
                                u_int32_t flags)
{
  struct nsm_msg_vlan msg;
  int ret;

  ret = 0;

  /* Send vlan delete message to PMs. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan));
  msg.cindex = 0;
  msg.flags = flags;
  msg.vid = vlan->vid;
  pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
  NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);

  /* Send VLAN delete message to PM. */
  ret = nsm_vlan_send_vlan_msg (bridge, &msg, NSM_MSG_VLAN_DELETE);

  return ret;
}

/* Notify clients of vlan addition. */
static void
_nsm_vlan_notify_add_vlan (struct nsm_bridge *bridge, struct nsm_vlan *vlan,
                           u_char type)
{
  struct nsm_vlan_listener *vlanlnr;
  struct listnode *ln;

  LIST_LOOP(bridge->vlan_listener_list, vlanlnr, ln)
    {
      if( vlanlnr->add_vlan_to_bridge_func )
        {   
          if (type & NSM_VLAN_DYNAMIC)
            {
              /* If port is dynamic member of the vlan report it only to
                 listeners interested in dynamic vlan registrations */
              if ( (vlanlnr->flags & NSM_VLAN_DYNAMIC) )
                (vlanlnr->add_vlan_to_bridge_func)(bridge->master, 
                                                   bridge->name, vlan->vid);
            }
          else
            {
              /* Report to all listeners */
              (vlanlnr->add_vlan_to_bridge_func)(bridge->master, 
                                                 bridge->name, vlan->vid);
            } /* end not member dynamic */
        } /* end vlan listener exists */
    } /* end LIST_LOOP */
}

/* Notify clients of vlan deletion. */
static void
_nsm_vlan_notify_delete_vlan (struct nsm_bridge *bridge, struct nsm_vlan *vlan,
                              u_char type)
{
  struct nsm_vlan_listener *vlanlnr;
  struct listnode *ln;

  LIST_LOOP(bridge->vlan_listener_list, vlanlnr, ln)
    {
      if( (vlanlnr) &&
          (vlanlnr->delete_vlan_from_bridge_func) )
        {   
          if (type & NSM_VLAN_DYNAMIC)
            {
              /* If port is dynamic member of the vlan report it only to
                 listeners interested in dynamic vlan registrations */
              if ( (vlanlnr->flags & NSM_VLAN_DYNAMIC) )
                (vlanlnr->delete_vlan_from_bridge_func)(bridge->master, 
                                                        bridge->name, vlan->vid);
            }
          else
            {
              /* Report to all listeners */
              (vlanlnr->delete_vlan_from_bridge_func)(bridge->master, 
                                                      bridge->name, vlan->vid);
            } /* end not member dynamic */
        } /* end vlan listener exists */
    } /* end LIST_LOOP */
}

struct nsm_vlan *
nsm_vlan_find (struct nsm_bridge *bridge, u_int16_t vid)
{
  struct avl_tree *vlan_table;
  struct avl_node *rn;
  struct nsm_vlan p;

  if (! bridge)
    return NULL;

  if ( nsm_bridge_is_vlan_aware(bridge->master, bridge->name) != PAL_TRUE  )
    return NULL;

#if defined HAVE_I_BEB || defined HAVE_PROVIDER_BRIDGE
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
#endif
#ifdef HAVE_B_BEB
    if (NSM_BRIDGE_TYPE_BACKBONE(bridge))
      vlan_table = bridge->bvlan_table;
    else
#endif
      vlan_table = bridge->vlan_table;

  if (! vlan_table)
    return NULL;

  NSM_VLAN_SET (&p, vid);

  rn = avl_search (vlan_table, (void *)&p);

  return (rn != NULL) ? AVL_NODE_INFO (rn) : NULL;

}

/* Add a VLAN. */
int
nsm_vlan_add (struct nsm_bridge_master *master, char *brname, char *vlan_name,
              u_int16_t vid, enum nsm_vlan_state state, u_int16_t type)
{
  enum nsm_vlan_egress_type egress_tagged;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port;
  char vname[NSM_VLAN_NAME_MAX + 1];
  struct avl_tree *vlan_table = NULL;
  struct interface *ifp = NULL;
  struct listnode *ln = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge;
  u_int32_t msg_flags = 0;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_vlan p;
  int ret = 0;
#ifdef HAVE_HA
  struct nsm_vlan_port_cdr_ref_node *vlan_port_cdr_ref_node;
#endif /* HAVE_HA */

  /* Lookup bridge. */
  bridge = nsm_lookup_bridge_by_name(master, brname);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE; 

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_I_BEB)
  if (CHECK_FLAG (type, NSM_VLAN_CVLAN))
  {
    if (NSM_BRIDGE_TYPE_PROVIDER (bridge)
#ifdef HAVE_PROVIDER_BRIDGE
       && bridge->provider_edge == PAL_FALSE
#endif
        )
      return NSM_VLAN_ERR_NOT_EDGE_BRIDGE;

    vlan_table = bridge->vlan_table;
  }
  else 
  {
    if (CHECK_FLAG (type, NSM_VLAN_SVLAN))
    {
      /* Allow addition of svlan only to provider bridges */
      if (!(NSM_BRIDGE_TYPE_PROVIDER (bridge)) && 
          (CHECK_FLAG (type, NSM_VLAN_SVLAN)))
        return NSM_VLAN_ERR_NOT_PROVIDER_BRIDGE;
      else
        vlan_table = bridge->svlan_table;
    }
  }
#else
  vlan_table = bridge->vlan_table;
#endif /* HAVE_PROVIDER_BRIDGE || HAVE_I_BEB */

#if defined (HAVE_B_BEB)
    /* Allow addition of bvlan only to backbone bridges */
  if ((NSM_BRIDGE_TYPE_BACKBONE (bridge)) &&
      (CHECK_FLAG (type, NSM_VLAN_BVLAN)))
    vlan_table = bridge->bvlan_table;
#endif /* HAVE_B_BEB */


  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  /* Add vlan to hardware */
  NSM_VLAN_SET (&p, vid);

  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {
      vlan = rn->info;

      if (!vlan)
        return NSM_VLAN_ERR_GENERAL;

      if (vlan->vlan_state == state && vlan->type == type) 
        {
          if (vlan_name == NULL)
            return ret;
          else if (pal_strcmp (vlan->vlan_name, vlan_name) == 0)
            return ret;
        }
      vlan->vlan_state = state;
      vlan->type |= type;

      if (vlan_name)
        {
          pal_mem_set (vlan->vlan_name, 0, NSM_VLAN_NAME_MAX+1);
          pal_strncpy (vlan->vlan_name, vlan_name, NSM_VLAN_NAME_MAX);
        }
      else
        {
          if (! pal_strcmp (vlan->vlan_name, ""))
            {
              pal_mem_set (vname, 0, NSM_VLAN_NAME_MAX + 1);
              pal_snprintf (vname, NSM_VLAN_NAME_MAX, "VLAN%04d", vid);
              pal_strcpy (vlan->vlan_name, vname);
            }
        }

      if (vlan->vlan_state == NSM_VLAN_ACTIVE)  
        {
#ifdef HAVE_HAL
           /* Add VLAN to hardware. */
           ret = hal_vlan_add (bridge->name, CHECK_FLAG (type, NSM_VLAN_CVLAN) ?
                               HAL_VLAN_TYPE_CVLAN :
                               HAL_VLAN_TYPE_SVLAN, vid);

#endif /* HAVE_HAL */

           /* Create SVI. */
           if (ret >= 0)
             {
#ifdef HAVE_PROVIDER_BRIDGE
               if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
                 ret = nsm_vlan_if_set (bridge->master->nm, vid,
                                        vid, type, bridge->name);
             }

           if (ret < 0)
             {
#ifdef HAVE_HA
               nsm_bridge_cal_delete_nsm_vlan (vlan);
#endif /* HAVE_HA */
               avl_remove (vlan_table, vlan);
               XFREE (MTYPE_VLAN, vlan);
#ifdef HAVE_HAL
               if (ret == HAL_ERR_BRIDGE_VLAN_RESERVED_IN_HW)
                 return NSM_VLAN_ERR_RESERVED_IN_HW;
#endif /* HAVE_HAL */
               return ret;
             }

           LIST_LOOP (vlan->port_list, ifp, ln)
             {
               zif = (struct nsm_if *)ifp->info;

               if ( !zif || !zif->switchport)
                 continue;

               br_port = zif->switchport;
               vlan_port = &br_port->vlan_port;

               if ( !vlan_port )
                 continue;
#ifdef HAVE_HAL
              egress_tagged =
                          NSM_VLAN_BMP_IS_MEMBER (vlan_port->egresstaggedBmp,
                                            vid) ? HAL_VLAN_EGRESS_TAGGED :
                                            HAL_VLAN_EGRESS_UNTAGGED;

              hal_vlan_add_vid_to_port (bridge->name, ifp->ifindex, vid,
                                         egress_tagged);

#ifdef HAVE_HA
              vlan_port_cdr_ref_node = vlan_port_cdr_ref_lookup (vlan,
                                                                 ifp->ifindex);
              nsm_bridge_cal_modify_nsm_vlan_port_association
                                     (vlan, br_port,
                                      vlan_port_cdr_ref_node);
#endif /* HAVE_HA */

              if (CHECK_FLAG (vlan_port->flags,
                        NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED))
                 nsm_vlan_set_acceptable_frame_type (ifp, vlan_port->mode,
                                      NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);

              if (CHECK_FLAG (vlan_port->flags,
                        NSM_VLAN_ENABLE_INGRESS_FILTER))
                 nsm_vlan_set_ingress_filter (ifp, vlan_port->mode,
                                              vlan_port->sub_mode, PAL_TRUE);

#endif /* HAVE_HAL */
             }
        }
      else
        {
#ifdef HAVE_PROVIDER_BRIDGE
           if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
             nsm_vlan_if_unset (bridge->master->nm, vid, vid, type,
                                bridge->name);

           /* Delete VLAN from hardware. */
#ifdef HAVE_HAL
           hal_vlan_delete (bridge->name,
                            CHECK_FLAG (type, NSM_VLAN_CVLAN) ?
                            HAL_VLAN_TYPE_CVLAN :
                            HAL_VLAN_TYPE_SVLAN, vid);
#endif /* HAVE_HAL */
        }
      vlan->bridge = bridge;

#ifdef HAVE_HA
      nsm_bridge_cal_modify_nsm_vlan (vlan);
#endif /* HAVE_HA */
    }
  else
    {
      vlan = XCALLOC (MTYPE_VLAN, sizeof (struct nsm_vlan));

      if (! vlan)
        return NSM_VLAN_ERR_NOMEM;

#ifdef HAVE_HA
      nsm_bridge_cal_create_nsm_vlan (vlan);
#endif /* HAVE_HA */

      vlan->vid = p.vid;

      avl_insert (vlan_table, (void *)vlan);

      vlan->vlan_state = state;
      vlan->type |= type;

      if (vlan_name)
        pal_strcpy (vlan->vlan_name, vlan_name);
      else
        {
          if (! pal_strcmp (vlan->vlan_name, ""))
            {
               pal_mem_set (vname, 0, NSM_VLAN_NAME_MAX + 1);
               pal_snprintf (vname, NSM_VLAN_NAME_MAX, "VLAN%04d", vid);
               pal_strcpy (vlan->vlan_name, vname);
             }
        }

       vlan->port_list = list_new ();
    
       vlan->forbidden_port_list = list_new ();

#ifdef HAVE_HA
       vlan->port_cdr_ref_list = list_new ();
#endif /* HAVE_HA */

#ifdef HAVE_PROVIDER_BRIDGE
       if (type & NSM_VLAN_SVLAN &&
           bridge->provider_edge == PAL_TRUE)
         vlan->cvlanMemberBmp = XCALLOC (MTYPE_NSM_VLAN_BITMAP,
                                         sizeof (struct nsm_vlan_bmp));
#endif /* HAVE_PROVIDER_BRIDGE */

       vlan->static_fdb_list = ptree_init (ETHER_MAX_BIT_LEN);
       vlan->vid = vid;

       if (state == NSM_VLAN_ACTIVE) 

         {
#ifdef HAVE_HAL
          /* Add VLAN to hardware. */
          ret = hal_vlan_add (bridge->name,
                              CHECK_FLAG (type, NSM_VLAN_CVLAN) ?
                              HAL_VLAN_TYPE_CVLAN :
                              HAL_VLAN_TYPE_SVLAN, vid);
#endif /* HAVE_HAL */

          /* Create SVI. */
          if (ret >= 0)
            {
#ifdef HAVE_PROVIDER_BRIDGE
              if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
                ret = nsm_vlan_if_set (bridge->master->nm, vid, vid,
                                       type, bridge->name);
            }

          if (ret < 0)
            {
#ifdef HAVE_HA
              nsm_bridge_cal_delete_nsm_vlan (vlan);
#endif /* HAVE_HA */
              avl_remove (vlan_table, vlan);
              XFREE (MTYPE_VLAN, vlan);
#ifdef HAVE_HAL
              if (ret == HAL_ERR_BRIDGE_VLAN_RESERVED_IN_HW)
                return NSM_VLAN_ERR_RESERVED_IN_HW;
#endif /* HAVE_HAL */

              return ret;
            }
         }
        vlan->bridge = bridge;
#ifdef HAVE_G8032
        vlan->primary_vid = PAL_FALSE;
#endif /* HAVE_G8032 */

      }


  if (type & NSM_VLAN_DYNAMIC)
    msg_flags |= NSM_MSG_VLAN_DYNAMIC;
  if (type & NSM_VLAN_STATIC)
    msg_flags |= NSM_MSG_VLAN_STATIC;
  if (type & NSM_VLAN_AUTO)
    msg_flags |= NSM_MSG_VLAN_AUTO;

  if (CHECK_FLAG (type, NSM_VLAN_CVLAN))
    msg_flags |= NSM_MSG_VLAN_CVLAN;
  if (CHECK_FLAG (type, NSM_VLAN_SVLAN))
    msg_flags |= NSM_MSG_VLAN_SVLAN;

  _nsm_vlan_send_vlan_add_msg (bridge, vlan, state, msg_flags);      

  /* Notify to all listeners */
  _nsm_vlan_notify_add_vlan (bridge, vlan, type);

  if (state == NSM_VLAN_ACTIVE)
    {
      /* Go through the list of ports in the bridge 
       * and configure this new vlan. */

      nsm_vlan_add_all (bridge, vid, type);
    }

#ifdef HAVE_SNMP
  vlan->create_time = pal_time_current (NULL);
#endif /* HAVE_SNMP */

#ifdef HAVE_QOS
  nsm_mls_qos_vlan_priority_config_restore (master->nm, brname, vid);
#endif /* HAVE_QOS */

#ifdef HAVE_PBB_TE
  vlan->unicast_egress_ports = list_new();
  vlan->unicast_forbidden_ports = list_new();
  vlan->multicast_egress_ports = list_new();
  vlan->multicast_forbidden_ports = list_new();
#endif /* HAVE_PBB_TE */

  return ret;
}

/* Delete a VLAN. */
int
nsm_vlan_delete (struct nsm_bridge_master *master, char *brname,
                 u_int16_t vid, u_int16_t type)
{
  struct avl_tree *vlan_table;
  struct nsm_bridge *bridge;
  u_int32_t msg_flags = 0;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_vlan p;
  int ret = 0;
#ifdef HAVE_VRRP
  int sess_ix = 0;
  int apn_vrid = 0;
  VRRP_GLOBAL_DATA *vrrp = NULL;
  char ifname[INTERFACE_NAMSIZ + 1];
#endif /* HAVE_VRRP */
  bridge = nsm_lookup_bridge_by_name(master, brname);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

#ifdef HAVE_G8031
  if (! nsm_g8031_vlan_config_exists ( bridge , vid ))
    return NSM_VLAN_ERR_G8031_CONFIG_EXIST;                
#endif

#ifdef HAVE_G8032
    if (! nsm_g8032_vlan_config_exists ( bridge , vid ))
          return NSM_VLAN_ERR_G8032_CONFIG_EXIST;
#endif /* HAVE_G8032 */
    

  if (CHECK_FLAG (type, NSM_VLAN_CVLAN))
    vlan_table = bridge->vlan_table;
  else if ((!NSM_BRIDGE_TYPE_PROVIDER (bridge)) &&
          (CHECK_FLAG (type, NSM_VLAN_SVLAN)))
    return NSM_VLAN_ERR_NOT_PROVIDER_BRIDGE;
  else
    vlan_table = bridge->svlan_table;

#ifdef HAVE_B_BEB 
  if ((NSM_BRIDGE_TYPE_BACKBONE (bridge)) &&
          (CHECK_FLAG (type, NSM_VLAN_BVLAN)))
    vlan_table = bridge->bvlan_table;
#endif /*HAVE_B_BEB*/

  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  NSM_VLAN_SET (&p, vid);

  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {
      if (! rn->info)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      vlan = rn->info;
      if (vlan && vlan->vlan_state == NSM_VLAN_ACTIVE)
      {
#ifdef HAVE_VRRP
#ifdef HAVE_PROVIDER_BRIDGE
          if (PAL_FALSE == bridge->provider_edge)
            {
#endif /* HAVE_PROVIDER_BRIDGE */
              apn_vrid = vlan->ifp->vr->id;
              pal_mem_set (ifname, 0, (INTERFACE_NAMSIZ + 1));

#ifdef HAVE_CUSTOM1
              zsnprintf (ifname, INTERFACE_NAMSIZ, "%s %d", "vlan", vid);
#else /* HAVE_CUSTOM1 */
              zsnprintf (ifname, INTERFACE_NAMSIZ, "%s%s.%d", "vlan", brname, vid);
#endif /* HAVE_CUSTOM1 */

              if (apn_vrid)
                vrrp = VRRP_GET_GLOBAL(apn_vrid);

              if (vrrp)
                {
                  for (sess_ix = 0; sess_ix < vrrp->sess_tbl_cnt; sess_ix++)
                    {
                      if ((vrrp->sess_tbl[sess_ix]->ifp != NULL) && 
                          (!pal_strcmp( vrrp->sess_tbl[sess_ix]->ifp->name, ifname )))
                        return NSM_VLAN_ERR_VRRP_BIND_EXISTS;
                    }
                }
#ifdef HAVE_PROVIDER_BRIDGE
            }
#endif /* HAVE_PROVIDER_BRIDGE */

#endif /* HAVE_VRRP */
      }
#ifdef HAVE_PROVIDER_BRIDGE
      if (nsm_vlan_cvlan_regis_exist (bridge, vlan))
        return NSM_VLAN_ERR_CVLAN_REG_EXIST;
#endif /* HAVE_PROVIDER_BRIDGE */
#if defined HAVE_I_BEB || defined HAVE_B_BEB
      /* PBB bridge */
      if (bridge->backbone_edge == PAL_TRUE)
        if (nsm_pbb_if_vlan_config_exists (bridge, vid))
          return NSM_VLAN_ERR_PBB_CONFIG_EXIST;
#endif /* HAVE_I_BEB || HAVE_B_BEB */ 

      UNSET_FLAG (vlan->type, type);
      vlan->bridge = NULL;

#if defined HAVE_PROVIDER_BRIDGE || defined HAVE_I_BEB 
      if (CHECK_FLAG (type, NSM_VLAN_SVLAN))
        {
          UNSET_FLAG (vlan->type, NSM_SVLAN_P2P);
          UNSET_FLAG (vlan->type, NSM_SVLAN_M2M);
        }
#endif /* HAVE_PROVIDER_BRIDGE || defined HAVE_I_BEB */
#if defined HAVE_B_BEB
      if (CHECK_FLAG (type, NSM_VLAN_BVLAN))
        {
          UNSET_FLAG (vlan->type, NSM_BVLAN_P2P);
          UNSET_FLAG (vlan->type, NSM_BVLAN_M2M);
        }
#endif /* HAVE_B_BEB */

      /* If VLAN is static as well as dynamic, don't delete this vlan
         from the ports. */
      if (!vlan->type)
        {
          /* Delete this VLAN from all ports which are configured. */
          nsm_vlan_delete_all (bridge, vid);
        }

      if (CHECK_FLAG (type, NSM_VLAN_DYNAMIC))
        msg_flags |= NSM_MSG_VLAN_DYNAMIC;
      if (CHECK_FLAG (type, NSM_VLAN_STATIC))
        msg_flags |= NSM_MSG_VLAN_STATIC;

      if (CHECK_FLAG (type, NSM_VLAN_CVLAN))
        msg_flags |= NSM_MSG_VLAN_CVLAN;
      if (CHECK_FLAG (type, NSM_VLAN_SVLAN))
        msg_flags |= NSM_MSG_VLAN_SVLAN;

      _nsm_vlan_send_vlan_delete_msg (bridge, vlan, msg_flags);

      /* Notify to all listeners */
      _nsm_vlan_notify_delete_vlan (bridge, vlan, type);

      if (!vlan->type)
        {
#ifdef HAVE_PROVIDER_BRIDGE
          if (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
            nsm_vlan_if_unset (bridge->master->nm, vid, vid, type,
                               bridge->name);

#ifdef HAVE_HAL
          hal_vlan_delete (bridge->name,
                           CHECK_FLAG (type, NSM_VLAN_CVLAN) ?
                           HAL_VLAN_TYPE_CVLAN :
                           HAL_VLAN_TYPE_SVLAN, vid);
#endif /* HAVE_HAL */

          /* Unset rn info. */
          avl_remove (vlan_table, vlan);
          if (vlan->port_list)
            list_delete (vlan->port_list);
          if (vlan->forbidden_port_list)
            list_delete (vlan->forbidden_port_list);
          if (vlan->static_fdb_list)
            {
              nsm_bridge_free_static_list (vlan->static_fdb_list);
              ptree_finish (vlan->static_fdb_list);
            } 

#ifdef HAVE_PBB_TE
          list_delete (vlan->unicast_egress_ports);
          list_delete (vlan->unicast_forbidden_ports);
          list_delete (vlan->multicast_egress_ports);
          list_delete (vlan->multicast_forbidden_ports);
#endif /* HAVE_PBB_TE */

#ifdef HAVE_PROVIDER_BRIDGE
          if (vlan->cvlanMemberBmp)
            XFREE (MTYPE_NSM_VLAN_BITMAP, vlan->cvlanMemberBmp);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_HA
          nsm_bridge_cal_delete_nsm_vlan (vlan);
#endif /* HAVE_HA */

          XFREE (MTYPE_VLAN, vlan);
        }

      bridge->vlan_num_deletes++;
#ifdef HAVE_HA
      nsm_bridge_cal_modify_nsm_bridge (bridge);
#endif /* HAVE_HA */

    }
  return ret;
}

/* Delete All Vlan */
void 
nsm_vlan_cleanup (struct nsm_master *nm)
{  
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;

  master = nsm_bridge_get_master (nm);
  if (master)
    for (bridge = master->bridge_list; bridge; bridge = bridge->next)
       if (nsm_bridge_is_vlan_aware(master, bridge->name))
         {
           if (bridge->vlan_table != NULL)
             for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
               if ((vlan = rn->info) != NULL)
                 nsm_vlan_delete (master, bridge->name, vlan->vid, vlan->type);

           if (bridge->svlan_table != NULL)
             for (rn = avl_top (bridge->svlan_table); rn; rn = avl_next (rn))
               if ((vlan = rn->info) != NULL)
                 nsm_vlan_delete (master, bridge->name, vlan->vid, vlan->type);
         }
}

/* Set port to access. */

/* VLAN Mode change. */
static int
nsm_vlan_set_port_mode (struct interface *ifp, enum nsm_vlan_port_mode mode,
                        enum nsm_vlan_port_mode sub_mode, bool_t notify_kernel)
{
  int ret = 0;
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct listnode *ln = NULL;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_msg_vlan_port_type msg;
  struct nsm_vlan_listener *vlanlnr = NULL;
#ifdef HAVE_PROVIDER_BRIDGE
  bool_t enable_ingress_filter;
  enum nsm_vlan_egress_type egress_type;
  enum hal_vlan_port_type sub_port_type;
  enum hal_vlan_acceptable_frame_type frame_type;
#endif /* HAVE_PROVIDER_BRIDGE */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND; 

  if (vlan_port->mode == mode
      && vlan_port->sub_mode == sub_mode)
    {
      /* Mode already set, nothing to do */
      return 0;
    }

  nsm_bridge_delete_all_fdb_by_port (bridge, zif, PAL_FALSE);

  /* Clear previous mode. */
  if (vlan_port->mode != NSM_VLAN_PORT_MODE_INVALID)
    {
      switch(vlan_port->mode)
        {
        case NSM_VLAN_PORT_MODE_ACCESS:
          ret = nsm_vlan_clear_access_port (ifp, notify_kernel);
          break;

        case NSM_VLAN_PORT_MODE_HYBRID:
          ret = nsm_vlan_clear_hybrid_port (ifp, PAL_FALSE, notify_kernel);
          break;

        case NSM_VLAN_PORT_MODE_TRUNK:
          ret = nsm_vlan_clear_trunk_port (ifp, PAL_FALSE, notify_kernel);
          break;

#ifdef HAVE_PROVIDER_BRIDGE

        case NSM_VLAN_PORT_MODE_CE:
        case NSM_VLAN_PORT_MODE_CN:
        case NSM_VLAN_PORT_MODE_PN:
          ret = nsm_vlan_clear_provider_port (ifp, vlan_port->mode,
                                              PAL_FALSE, notify_kernel);

          if (br_port->trans_tab)
            nsm_vlan_trans_tab_delete (ifp);

          break;

#endif /* HAVE_PROVIDER_BRIDGE */
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
        case NSM_VLAN_PORT_MODE_CNP:
        case NSM_VLAN_PORT_MODE_CBP:
        case NSM_VLAN_PORT_MODE_PIP:
          /* Need to clear any entries? */
#endif /*(HAVE_I_BEB) || (HAVE_B_BEB)*/
        case NSM_VLAN_PORT_MODE_INVALID:
          break;

        default:
          return NSM_VLAN_ERR_INVALID_MODE;

        }

      if (ret < 0)
        {
          return NSM_VLAN_ERR_MODE_CLEAR;
        }
    }

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port_type));

  /* Set port mode. */
  vlan_port->mode = mode;
  vlan_port->sub_mode = sub_mode;

  /* Reset the port flags */
  vlan_port->flags = 0;

  msg.ifindex = ifp->ifindex;
  msg.port_type = mode;

  /* Set new mode. */
  switch (mode)
    {
    case NSM_VLAN_PORT_MODE_ACCESS:

      /* Send port mode message to PM. */

      msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
      msg.acceptable_frame_type = NSM_MSG_VLAN_ACCEPT_UNTAGGED_ONLY;

      vlan_port->flags |= NSM_VLAN_ENABLE_INGRESS_FILTER;

      if (notify_kernel == PAL_TRUE)
        {
          /* Set port type in hal */
          hal_vlan_set_port_type (bridge->name, ifp->ifindex, 
                                  HAL_VLAN_ACCESS_PORT,
                                  HAL_VLAN_ACCESS_PORT,
                                  HAL_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED,
                                  PAL_TRUE);
        }

      /* Set port to access. */
      nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID, PAL_FALSE,
                                notify_kernel);
      nsm_vlan_add_access_port (ifp, NSM_VLAN_DEFAULT_VID,
                                NSM_VLAN_EGRESS_UNTAGGED, PAL_FALSE,
                                notify_kernel);

      break;

    case NSM_VLAN_PORT_MODE_HYBRID:

      msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
      msg.acceptable_frame_type = NSM_MSG_VLAN_ACCEPT_FRAME_ALL;

      if (notify_kernel == PAL_TRUE)
        {
           /* Set port type in hal */
          hal_vlan_set_port_type(bridge->name, ifp->ifindex, 
                                 HAL_VLAN_HYBRID_PORT,
                                 HAL_VLAN_HYBRID_PORT,
                                 HAL_VLAN_ACCEPTABLE_FRAME_TYPE_ALL,
                                 PAL_TRUE);
        }

      /* Send port to hybrid. */
      nsm_vlan_set_hybrid_port (ifp, NSM_VLAN_DEFAULT_VID, PAL_FALSE,
                                notify_kernel);

      vlan_port->config = NSM_VLAN_CONFIGURED_NONE;
      vlan_port->flags |= NSM_VLAN_ENABLE_INGRESS_FILTER;

      nsm_vlan_add_hybrid_port (ifp, NSM_VLAN_DEFAULT_VID,
                                NSM_VLAN_EGRESS_UNTAGGED, PAL_FALSE,
                                notify_kernel);
      break;
    case NSM_VLAN_PORT_MODE_TRUNK:

      msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
      msg.acceptable_frame_type = NSM_MSG_VLAN_ACCEPT_TAGGED_ONLY;

      /* Trunk ports will accept only tagged packets by default. */
      vlan_port->flags |= (NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED |
                           NSM_VLAN_ENABLE_INGRESS_FILTER);

      if (notify_kernel == PAL_TRUE)
        {
          /* Set port type in hal */
          hal_vlan_set_port_type (bridge->name, ifp->ifindex, 
                                  HAL_VLAN_TRUNK_PORT,
                                  HAL_VLAN_TRUNK_PORT,
                                  HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED,
                                  PAL_TRUE);
        }

      /* Send port type message to PM. */
      nsm_vlan_send_port_type_msg (bridge, &msg);

      /* Send port to trunk. */
      nsm_vlan_set_trunk_port (ifp, NSM_VLAN_DEFAULT_VID, 
                               PAL_FALSE, notify_kernel);

      /* Add the trunk port to Default Vlan */
      vlan_port->native_vid = NSM_VLAN_DEFAULT_VID; 

      nsm_vlan_add_trunk_port(ifp, NSM_VLAN_DEFAULT_VID, PAL_FALSE,
                              notify_kernel);
      break;

#ifdef HAVE_PROVIDER_BRIDGE

    case NSM_VLAN_PORT_MODE_CE:

      avl_create (&br_port->trans_tab, 0, nsm_vlan_trans_tab_ent_cmp);
      avl_create (&br_port->rev_trans_tab, 0, nsm_vlan_rev_trans_tab_ent_cmp);
      br_port->uni_ser_attr = NSM_UNI_MUX_BNDL;


      switch (sub_mode)
        {
        case NSM_VLAN_PORT_MODE_ACCESS:
           msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
           msg.acceptable_frame_type = NSM_MSG_VLAN_ACCEPT_UNTAGGED_ONLY;
           enable_ingress_filter = PAL_TRUE;
           frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED;
           sub_port_type = HAL_VLAN_ACCESS_PORT;
           egress_type = NSM_VLAN_EGRESS_UNTAGGED;
           vlan_port->flags |= (NSM_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED | 
                                NSM_VLAN_ENABLE_INGRESS_FILTER);
           break;
        case NSM_VLAN_PORT_MODE_HYBRID:
           msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
           msg.acceptable_frame_type = NSM_MSG_VLAN_ACCEPT_FRAME_ALL;
           enable_ingress_filter = PAL_TRUE;
           frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_ALL;
           sub_port_type = HAL_VLAN_HYBRID_PORT;
           egress_type = NSM_VLAN_EGRESS_UNTAGGED;
           vlan_port->flags |= NSM_VLAN_ENABLE_INGRESS_FILTER;
           break;
        case NSM_VLAN_PORT_MODE_TRUNK:
           msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
           msg.acceptable_frame_type = NSM_MSG_VLAN_ACCEPT_TAGGED_ONLY;
           enable_ingress_filter = PAL_TRUE;
           frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED;
           sub_port_type = HAL_VLAN_TRUNK_PORT;
           egress_type = NSM_VLAN_EGRESS_TAGGED;
           /* Trunk ports will accept only tagged packets by default. */
           vlan_port->flags |= (NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED | 
                                NSM_VLAN_ENABLE_INGRESS_FILTER);
           break;
        default:
          return NSM_VLAN_ERR_INVALID_MODE;

        }

      if (notify_kernel == PAL_TRUE)
        {
          /* Set port type in hal */
          hal_vlan_set_port_type (bridge->name, ifp->ifindex,
                                  HAL_VLAN_PROVIDER_CE_PORT,
                                  sub_port_type,
                                  frame_type,
                                  enable_ingress_filter);
        }

#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD || defined (HAVE_RPSVT_PLUS)

      br_port->proto_process.stp_process = HAL_L2_PROTO_PEER;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_STP, HAL_L2_PROTO_PEER,
                                         NSM_VLAN_NULL_VID);

#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD || HAVE_RPVST_PLUS */

#ifdef HAVE_GMRP
      br_port->proto_process.gmrp_process = HAL_L2_PROTO_TUNNEL;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_GMRP, HAL_L2_PROTO_TUNNEL,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP
      br_port->proto_process.mmrp_process = HAL_L2_PROTO_TUNNEL;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_MMRP, HAL_L2_PROTO_TUNNEL,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP
      br_port->proto_process.gvrp_process = HAL_L2_PROTO_DISCARD;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_GVRP, HAL_L2_PROTO_DISCARD,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP
      br_port->proto_process.mvrp_process = HAL_L2_PROTO_DISCARD;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_MVRP, HAL_L2_PROTO_DISCARD,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD
      br_port->proto_process.lacp_process = HAL_L2_PROTO_PEER;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_LACP, HAL_L2_PROTO_PEER,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD
      br_port->proto_process.dot1x_process = HAL_L2_PROTO_PEER;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_LACP, HAL_L2_PROTO_PEER,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_AUTHD */

      nsm_vlan_set_provider_port (ifp, NSM_VLAN_DEFAULT_VID, mode, sub_mode,
                                  PAL_FALSE, notify_kernel);

      nsm_vlan_add_provider_port (ifp, NSM_VLAN_DEFAULT_VID, mode, sub_mode,
                                  egress_type,
                                  PAL_FALSE,
                                  notify_kernel);
      break;

    case NSM_VLAN_PORT_MODE_PN:
    case NSM_VLAN_PORT_MODE_CN:
      msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
      msg.acceptable_frame_type = NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED;

      avl_create (&br_port->trans_tab, 0, nsm_vlan_trans_tab_ent_cmp);
      avl_create (&br_port->rev_trans_tab, 0, nsm_vlan_rev_trans_tab_ent_cmp);

      /* Trunk ports will accept only tagged packets by default. */
      vlan_port->flags |= (NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED |
                           NSM_VLAN_ENABLE_INGRESS_FILTER );

      if (notify_kernel == PAL_TRUE)
        {
          /* Set port type in hal */
          hal_vlan_set_port_type (bridge->name, ifp->ifindex,
                                  mode == NSM_VLAN_PORT_MODE_PN ?
                                  HAL_VLAN_PROVIDER_PN_PORT:
                                  HAL_VLAN_PROVIDER_CN_PORT,
                                  mode == NSM_VLAN_PORT_MODE_PN ?
                                  HAL_VLAN_PROVIDER_PN_PORT:
                                  HAL_VLAN_PROVIDER_CN_PORT,
                                  HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED,
                                  PAL_TRUE);
        }

      /* Send port to trunk. */
      nsm_vlan_set_provider_port (ifp, NSM_VLAN_DEFAULT_VID,
                                  mode, mode,
                                  PAL_FALSE, notify_kernel);

      nsm_vlan_add_provider_port (ifp, NSM_VLAN_DEFAULT_VID,
                                  mode, mode,
                                  NSM_VLAN_EGRESS_TAGGED, PAL_FALSE,
                                  notify_kernel);

#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD

      br_port->proto_process.stp_process = HAL_L2_PROTO_TUNNEL;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_STP, HAL_L2_PROTO_TUNNEL,
                                         NSM_VLAN_NULL_VID);

#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GMRP
      br_port->proto_process.gmrp_process = HAL_L2_PROTO_TUNNEL;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_GMRP,
                                         HAL_L2_PROTO_TUNNEL,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP
      br_port->proto_process.mmrp_process = HAL_L2_PROTO_TUNNEL;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_MMRP,
                                         HAL_L2_PROTO_TUNNEL,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP
      br_port->proto_process.gvrp_process = HAL_L2_PROTO_TUNNEL;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_GVRP,
                                         HAL_L2_PROTO_TUNNEL,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP
      br_port->proto_process.mvrp_process = HAL_L2_PROTO_TUNNEL;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_MVRP,
                                         HAL_L2_PROTO_TUNNEL,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD
      br_port->proto_process.lacp_process = HAL_L2_PROTO_PEER;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_LACP, HAL_L2_PROTO_PEER,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_LACPD */

#ifdef HAVE_AUTHD
      br_port->proto_process.dot1x_process = HAL_L2_PROTO_PEER;
      hal_bridge_set_proto_process_port (bridge->name, ifp->ifindex,
                                         HAL_PROTO_LACP, HAL_L2_PROTO_PEER,
                                         NSM_VLAN_NULL_VID);
#endif /* HAVE_AUTHD */

      break;

#endif /* HAVE_PROVIDER_BRIDGE */
#ifdef HAVE_I_BEB     
     /* Overloading the CNP, PIP and CBP as the PN so that frame flow is possible. */ 
    case NSM_VLAN_PORT_MODE_CNP:
      msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
      vlan_port->flags |= NSM_VLAN_ENABLE_INGRESS_FILTER;
      
      /* For CNP, the sub-mode is Hybrid while for others it is Trunk */
      hal_vlan_set_port_type (bridge->name, ifp->ifindex,
                              HAL_VLAN_PROVIDER_PN_PORT,
                              NSM_VLAN_PORT_MODE_HYBRID,
                              HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED,
                              PAL_TRUE);
      break;
    case NSM_VLAN_PORT_MODE_PIP:
      msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
      vlan_port->flags |= (NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED | 
                           NSM_VLAN_ENABLE_INGRESS_FILTER );

      hal_vlan_set_port_type (bridge->name, ifp->ifindex,
                              HAL_VLAN_PROVIDER_PN_PORT,
                              NSM_VLAN_PORT_MODE_TRUNK,
                              HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED,
                              PAL_TRUE);
          break;
#endif
#ifdef HAVE_B_BEB          
    case NSM_VLAN_PORT_MODE_CBP:
      msg.ingress_filter = NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER;
      vlan_port->flags |= (NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED |
                           NSM_VLAN_ENABLE_INGRESS_FILTER );

      hal_vlan_set_port_type (bridge->name, ifp->ifindex,
                              HAL_VLAN_PROVIDER_PN_PORT,
                              NSM_VLAN_PORT_MODE_TRUNK,
                              HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED,
                              PAL_TRUE);
      break;
#endif          
    default:
      break;
    }

  /* Send port type message to PM. */
  nsm_vlan_send_port_type_msg (bridge, &msg);

#ifdef HAVE_PROVIDER_BRIDGE
  nsm_pro_vlan_send_proto_process (bridge->name,
                                   ifp, NSM_MSG_BRIDGE_PROTO_PEER);
#endif /* HAVE_PROVIDER_BRIDGE */

  if (bridge->vlan_listener_list)
    {
      LIST_LOOP(bridge->vlan_listener_list, vlanlnr, ln)
        {
          if ( (vlanlnr) &&
               (vlanlnr->change_port_mode_func) )
            {
              (vlanlnr->change_port_mode_func) (bridge->master,
                                                bridge->name, ifp);
            }
        }
    }

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  /* Set port type in hardware look at old l2_vlan_br_vlan_set_port_type. AVP. TBD */
  return 0;
}

/* VLAN get port mode. */
int
nsm_vlan_api_get_port_mode (struct interface *ifp,
                            enum nsm_vlan_port_mode *mode,
                            enum nsm_vlan_port_mode *sub_mode)
{
  struct nsm_if *zif = (struct nsm_if *)(ifp)->info;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  *mode = vlan_port->mode;
  *sub_mode = vlan_port->sub_mode;

  return NSM_API_GET_SUCCESS;
}

/*
** The flags are interpreted in the foll manner:
** iterate_members -- This flag is only for aggregator ports and doesn't
**                    have any meaning otherwise.
** notify_kernel   -- This is also applicable only for aggregators.
**                    In case of aggregators we may want to add properties to
**                    nsm but not notify the kernel (hal/hsl). This flag
**                    is used to control that.
*/
int
nsm_vlan_api_set_port_mode (struct interface *ifp, enum nsm_vlan_port_mode mode,
                            enum nsm_vlan_port_mode sub_mode,
                            bool_t iterate_members, bool_t notify_kernel)
{
  struct nsm_if *nif;
  struct nsm_bridge *bridge;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_LACPD
  struct interface *memifp;
  struct listnode *lsn;
#endif
  int ret = 0;
#ifdef HAVE_SMI
  int send_alarm = PAL_TRUE;
#endif

  nif = (struct nsm_if *)ifp->info;

  if (nif == NULL || nif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#if (defined HAVE_MPLS_VC || defined HAVE_VPLS)
  if (ifp->bind != 0)
    return NSM_VLAN_ERR_VC_BOUND;
#endif /* HAVE_MPLS_VC || HAVE_VPLS */

  br_port = nif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = nif->bridge;

  if (! bridge)
    {
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (nif->nsm_bridge_port_conf)
          {
            nif->nsm_bridge_port_conf->vlan_port.mode = mode;
            nif->nsm_bridge_port_conf->vlan_port.sub_mode = sub_mode;
          }

#endif /* HAVE_LACPD */

      return NSM_VLAN_ERR_BRIDGE_NOT_FOUND; 
    }


  if (vlan_port->mode == mode
      && vlan_port->sub_mode == sub_mode)
    {
      /* Mode already set, nothing to do */
      return 0;
    }

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge)
      && (mode == NSM_VLAN_PORT_MODE_ACCESS
          || mode == NSM_VLAN_PORT_MODE_HYBRID
          || mode == NSM_VLAN_PORT_MODE_TRUNK))
    return NSM_VLAN_ERR_INVALID_MODE;

#ifdef HAVE_B_BEB
  /* Check for PN and PNP on the bridge is presently not being done as 
   * PN-PNP clash arises for B-comp */
  if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
      && (mode == NSM_VLAN_PORT_MODE_CE
          || mode == NSM_VLAN_PORT_MODE_CN
          || mode == NSM_VLAN_PORT_MODE_CNP
          || mode == NSM_VLAN_PORT_MODE_PIP))
    {
      return NSM_VLAN_ERR_INVALID_MODE;
    }

  if (!NSM_BRIDGE_TYPE_BACKBONE (bridge)
      && (mode == NSM_VLAN_PORT_MODE_CBP))
    {
      return NSM_VLAN_ERR_INVALID_MODE;
    }
#else
   if (!NSM_BRIDGE_TYPE_PROVIDER (bridge)
      && (mode == NSM_VLAN_PORT_MODE_CE
          || mode == NSM_VLAN_PORT_MODE_CN
          || mode == NSM_VLAN_PORT_MODE_PN
          || mode == NSM_VLAN_PORT_MODE_CNP
          || mode == NSM_VLAN_PORT_MODE_PIP))
    {
      return NSM_VLAN_ERR_INVALID_MODE;
    }
 
#endif  

#ifdef HAVE_PROVIDER_BRIDGE

  if (mode == NSM_VLAN_PORT_MODE_CE
      && bridge->provider_edge == PAL_FALSE)
    return NSM_VLAN_ERR_NOT_EDGE_BRIDGE;
    
#endif /* HAVE_PROVIDER_BRIDGE */

  if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE
      && vlan_port->regtab)
    return NSM_VLAN_ERR_CVLAN_REG_EXIST;

#if defined HAVE_I_BEB || defined HAVE_B_BEB
   ret =  nsm_pbb_if_config_exists (ifp, vlan_port->mode);
   if (ret == NSM_VLAN_ERR_PBB_CONFIG_EXIST)
      return ret;
#endif /*HAVE_I_BEB || HAVE_B_BEB*/

#ifdef HAVE_SMI
  if (vlan_port->mode == NSM_VLAN_PORT_MODE_INVALID)
    send_alarm = PAL_FALSE;
#endif
#if defined HAVE_I_BEB || defined HAVE_B_BEB
  if ((mode == NSM_VLAN_PORT_MODE_CNP
      || mode == NSM_VLAN_PORT_MODE_CBP
      || mode == NSM_VLAN_PORT_MODE_PIP
      || ((mode == NSM_VLAN_PORT_MODE_PNP) &&
          (bridge->name[0]=='b')))
      && (!(vlan_port->mode == NSM_VLAN_PORT_MODE_CNP
      || vlan_port->mode == NSM_VLAN_PORT_MODE_CBP
      || vlan_port->mode == NSM_VLAN_PORT_MODE_PIP
      || ((vlan_port->mode == NSM_VLAN_PORT_MODE_PNP) &&
          (bridge->name[0]=='b')))))
    {
       bridge->master->beb->beb_tot_ext_ports++;
    }

  if ((vlan_port->mode == NSM_VLAN_PORT_MODE_CNP
      || vlan_port->mode == NSM_VLAN_PORT_MODE_CBP
      || vlan_port->mode == NSM_VLAN_PORT_MODE_PIP
      || ((vlan_port->mode == NSM_VLAN_PORT_MODE_PNP) &&
          (bridge->name[0]=='b')))
      && (!(mode == NSM_VLAN_PORT_MODE_CNP
      || mode == NSM_VLAN_PORT_MODE_CBP
      || mode == NSM_VLAN_PORT_MODE_PIP
      || ((mode == NSM_VLAN_PORT_MODE_PNP) &&
          (bridge->name[0]=='b')))))
    {
       bridge->master->beb->beb_tot_ext_ports--;
    }

#if !(defined HAVE_B_BEB)
  /* only I_BEB case */
  /* no need to check previous mode as control has already reached here */
  if (mode == NSM_VLAN_PORT_MODE_PIP)
    ret = nsm_vlan_port_add_pip(ifp);
  else if (vlan_port->mode == NSM_VLAN_PORT_MODE_PIP)
    ret = nsm_vlan_port_del_pip(ifp);

  if (ret < 0)
    return ret;
#endif /* HAVE_I_BEB && !HAVE_B_BEB */

#endif /*HAVE_I_BEB || HAVE_B_BEB*/
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
    ret = nsm_vlan_set_port_mode (ifp, mode, sub_mode, PAL_FALSE);
  else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
    ret = nsm_vlan_set_port_mode (ifp, mode, sub_mode, notify_kernel);
  if (ret < 0)
    return ret;

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (key != ifp->lacp_admin_key)
      nsm_server_if_lacp_admin_key_update (ifp);
  }

  if ((NSM_INTF_TYPE_AGGREGATOR(ifp)) && (iterate_members == PAL_TRUE))
    {
      AGGREGATOR_MEMBER_LOOP (nif, memifp, lsn)
      {
        NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);
        NSM_BRIDGE_CHECK_AGG_MEM_BRIDGE_PROPERTY(memifp);
        memifp->agg_param_update = 1;
        ret = nsm_vlan_api_set_port_mode (memifp, mode, sub_mode,
                                          PAL_FALSE, PAL_TRUE);
        memifp->agg_param_update = 0;
        if (ret < 0)
          return ret;
      }
    }
#endif /* HAVE_LACPD */

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (nif->switchport);
#endif /* HAVE_HA */

#ifdef HAVE_SMI
  if (send_alarm == PAL_TRUE)
    smi_record_vlan_port_mode_alarm (ifp->name, mode, sub_mode); 
#endif

#ifdef HAVE_ELMID
  /* Send UNI msg to ELMI if the mode is CE */
   if (NSM_BRIDGE_TYPE_PROVIDER (bridge)

       && (bridge->provider_edge == PAL_TRUE)
       && (vlan_port->mode == NSM_VLAN_PORT_MODE_CE))
     {
       nsm_elmi_send_uni_add (bridge, ifp); 
     }
#endif /* HAVE_ELMID */

  /* If the UNI Type is enabled on the bridge, the new port added to the 
   * brige group also should have the uni type enabled
   */
#ifdef HAVE_PROVIDER_BRIDGE
  /* If the bridge is Provider Edge bridge, the uni type mode
   * should be enabled on the Customer Edge Ports
   */
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge)
      && (bridge->provider_edge == PAL_TRUE)
      && (vlan_port->mode == NSM_VLAN_PORT_MODE_CE))
    {
      if (bridge->uni_type_mode == NSM_UNI_TYPE_ENABLE)
        {
          ret = nsm_uni_type_detect (ifp, bridge->uni_type_mode);

        }

      /* Setting the MAX EVC for CE-Port as 4094*/
      br_port->uni_max_evcs = NSM_VLAN_MAX;
    }
  else
   br_port->uni_max_evcs = NSM_VLAN_NONE;

  /* If the bridge is MSTP/RSTP bridge, the uni type mode
   * should be enabled on all the ports associated to
   * the bridge
   */
  if (!NSM_BRIDGE_TYPE_PROVIDER (bridge) &&
      (bridge->uni_type_mode == NSM_UNI_TYPE_ENABLE))
    {
      ret = nsm_uni_type_detect (ifp, bridge->uni_type_mode);
    }

#endif /* HAVE_PROVIDER_BRIDGE*/
  return 0;
}

/* Set port to access. */
int
nsm_vlan_set_common_port (struct interface *ifp, enum nsm_vlan_port_mode mode,
                          enum nsm_vlan_port_mode sub_mode,
                          nsm_vid_t pvid, bool_t iterate_members,
                          bool_t notify_kernel)
{
  int ret = 0;
  struct nsm_if *zif;
  struct nsm_vlan p;
  struct nsm_vlan *vlan;
  struct avl_node *rn;
  struct nsm_bridge *bridge;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_LACPD
  struct nsm_if *nif = ifp->info;
  struct interface *memifp;
  struct listnode *ln;
  int memret;
#endif /* HAVE_LACPD */

  zif = (struct nsm_if *)ifp->info;
  rn = NULL;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (! bridge)
    {
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (zif->nsm_bridge_port_conf)
          {
            vlan_port = &zif->nsm_bridge_port_conf->vlan_port;
            vlan_port->mode = mode;
            if (vlan_port->pvid)
              NSM_VLAN_BMP_UNSET( vlan_port->staticMemberBmp, vlan_port->pvid);
            vlan_port->pvid = pvid;
            NSM_VLAN_BMP_SET( vlan_port->staticMemberBmp, vlan_port->pvid);
          }
#endif /* HAVE_LACPD */

      return NSM_VLAN_ERR_IFP_NOT_BOUND;
    }

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
#ifdef HAVE_B_BEB
               || ((vlan_port->mode == NSM_VLAN_PORT_MODE_PN)
                   && ((bridge->type != NSM_BRIDGE_TYPE_BACKBONE_MSTP) 
                   &&(bridge->type != NSM_BRIDGE_TYPE_BACKBONE_RSTP )))
#else
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN
#endif /* HAVE_B_BEB */
               || vlan_port->mode == NSM_VLAN_PORT_MODE_CNP 
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PIP ?
               bridge->svlan_table : 
#ifdef HAVE_B_BEB
               vlan_port->mode == NSM_VLAN_PORT_MODE_CBP
               || ((vlan_port->mode == NSM_VLAN_PORT_MODE_PNP) 
                   &&((bridge->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP )
                    ||(bridge->type == NSM_BRIDGE_TYPE_BACKBONE_RSTP))) ?
               bridge->bvlan_table : 
#endif
               bridge->vlan_table;


  if (vlan_port->mode != mode
      || vlan_port->sub_mode != sub_mode)
    return NSM_VLAN_ERR_INVALID_MODE;

  /* Already set to the same pvid? */
  if ((vlan_port->mode == mode) && (vlan_port->pvid == pvid))
    return 0;

#ifdef HAVE_VLAN_STACK
  /* Disable vlan stacking if pvid has changed or port is no 
     longer access port. */
  if ((vlan_port->mode == NSM_VLAN_PORT_MODE_ACCESS) &&
      (vlan_port->vlan_stacking == NSM_TRUE))
     nsm_vlan_stacking_disable (ifp);
#endif /* HAVE_VLAN_STACK */

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  if (vlan_port->mode == mode || vlan_port->mode == NSM_VLAN_PORT_MODE_INVALID)
    {
      NSM_VLAN_SET (&p, pvid);

      if (vlan_table != NULL)
        rn = avl_search (vlan_table, (void *)&p);

      if (rn && rn->info)
        {
          vlan = rn->info;

          if ( (vlan_port->mode != NSM_VLAN_PORT_MODE_INVALID) &&
               (vlan_port->pvid) )
            {
              /* Delete vid from port. */
              NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vlan_port->pvid);
              NSM_VLAN_BMP_UNSET (vlan_port->egresstaggedBmp, vlan_port->pvid);

              /* Delete port from vid. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
              if (NSM_INTF_TYPE_AGGREGATOR(ifp))
                nsm_vlan_delete_port (bridge, ifp, vlan_port->pvid, PAL_FALSE);
              else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
                nsm_vlan_delete_port (bridge, ifp, vlan_port->pvid, 
                                      notify_kernel);

            }

          /* Set the default port type. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
          if (NSM_INTF_TYPE_AGGREGATOR(ifp))
            nsm_vlan_set_default (bridge, ifp, mode, pvid, PAL_FALSE);
          else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
            nsm_vlan_set_default (bridge, ifp, mode, pvid, notify_kernel);
        }
      else
        {
          return NSM_VLAN_ERR_VLAN_NOT_FOUND;
        }
    }
  else
    return NSM_VLAN_ERR_BASE;

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (key != ifp->lacp_admin_key)
      nsm_server_if_lacp_admin_key_update (ifp);
  }

  if ((NSM_INTF_TYPE_AGGREGATOR(ifp)) && (iterate_members == PAL_TRUE))
    {
  AGGREGATOR_MEMBER_LOOP (nif, memifp, ln)
    {
      NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);
      NSM_BRIDGE_CHECK_AGG_MEM_BRIDGE_PROPERTY(memifp);

          memifp->agg_param_update = 1;
          memret = nsm_vlan_set_common_port (memifp, mode, sub_mode, pvid, 
                                             PAL_FALSE, PAL_TRUE);
          memifp->agg_param_update = 0;
      if (memret < 0)
        return memret;
    }
    }
#endif /* HAVE_LACPD */

  return ret;
}
/* Set any port with default PVID. */
int
nsm_vlan_set_pvid (struct interface *ifp, enum nsm_vlan_port_mode mode,
                   enum nsm_vlan_port_mode sub_mode,
                   nsm_vid_t pvid, bool_t iterate_members,
                   bool_t notify_kernel)
{
  return nsm_vlan_set_common_port (ifp, mode,sub_mode,pvid, iterate_members,
                                   notify_kernel);
}

/* Get port Default VID. */
int
nsm_vlan_get_pvid (struct interface *ifp, nsm_vid_t *pvid)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
  {
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
  }

  vlan_port = &zif->switchport->vlan_port;

  if (vlan_port == NULL)
  {
    return NSM_API_GET_FAIL;
  }

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
  {
    if (zif->nsm_bridge_port_conf)
    {
      vlan_port = &zif->nsm_bridge_port_conf->vlan_port;
      if (vlan_port == NULL)
      {
        return NSM_API_GET_FAIL;
      }
    }
  }
#endif /* HAVE_LACPD */

  *pvid = vlan_port->pvid;

  return NSM_API_GET_SUCCESS;
}

/* Set access port with default PVID. */
int
nsm_vlan_set_access_port (struct interface *ifp, nsm_vid_t pvid, 
                          bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_set_common_port (ifp, NSM_VLAN_PORT_MODE_ACCESS,
                                   NSM_VLAN_PORT_MODE_ACCESS, pvid,
                                   iterate_members, notify_kernel);
}

/* Set hybrid port with default PVID. */
int
nsm_vlan_set_hybrid_port (struct interface *ifp, nsm_vid_t pvid,
                          bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_set_common_port (ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                   NSM_VLAN_PORT_MODE_HYBRID, pvid,
                                   iterate_members, notify_kernel);
}

/* Set trunk port with default PVID. */
int
nsm_vlan_set_trunk_port (struct interface *ifp, nsm_vid_t pvid,
                         bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_set_common_port (ifp, NSM_VLAN_PORT_MODE_TRUNK,
                                   NSM_VLAN_PORT_MODE_TRUNK, pvid,
                                   iterate_members, notify_kernel);
}

#ifdef HAVE_PROVIDER_BRIDGE

/* Set provider port with default PVID. */
int
nsm_vlan_set_provider_port (struct interface *ifp, nsm_vid_t pvid,
                            enum nsm_vlan_port_mode mode,
                            enum nsm_vlan_port_mode sub_mode,
                            bool_t iterate_members, bool_t notify_kernel)
{

  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  if  (!zif || !zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  if (nsm_vlan_port_cvlan_regis_exist (ifp, vlan_port->pvid))
    return NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST;

  return nsm_vlan_set_common_port (ifp, mode, sub_mode, pvid, iterate_members,
                                   notify_kernel);
}

#endif /* HAVE_PROVIDER_BRIDGE */

/* Set acceptable frame type. */
int 
nsm_vlan_set_acceptable_frame_type (struct interface *ifp,
                                    enum nsm_vlan_port_mode mode,
                                    int acceptable_frame_type)
{
  int ret = -1;
  struct nsm_if *zif;
  char  enable_ingress_filter;
  char  l_acceptable_frame_type;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (!bridge)
    {
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (zif->nsm_bridge_port_conf)
          SET_FLAG (zif->nsm_bridge_port_conf->vlan_port.flags,
                    NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);
#endif /* HAVE_LACPD */

      return -1; 
    }

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  if (vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID)
    {
      if (vlan_port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER)
        {
          enable_ingress_filter = PAL_TRUE;
        }
      else
        {
          enable_ingress_filter = PAL_FALSE;
        }

      if (acceptable_frame_type == NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
        l_acceptable_frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED;
      else
        l_acceptable_frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_ALL;

      ret = hal_vlan_set_port_type (bridge->name, ifp->ifindex,
                                    mode == NSM_VLAN_PORT_MODE_CE ?
                                    HAL_VLAN_PROVIDER_CE_PORT:
                                    HAL_VLAN_HYBRID_PORT,
                                    HAL_VLAN_HYBRID_PORT,
                                    l_acceptable_frame_type, enable_ingress_filter);
      if ( ret >= 0)
        {
          if (acceptable_frame_type == NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
            vlan_port->flags |= acceptable_frame_type;
          else 
            vlan_port->flags &= acceptable_frame_type;
        }
      return ret; 
    }
  else
    return -1;
}

int
nsm_update_portbased_vlan (struct nsm_if *zif, u_int32_t portmap)
{
  int ret = 0;
  struct hal_port_map hal_bmap;

  if (zif == NULL)
    return -1;

  pal_mem_set (hal_bmap.bitmap, 0, sizeof(hal_bmap.bitmap));

  zif->port_vlan = portmap;
  hal_bmap.bitmap[0] = portmap;

#ifdef HAVE_HAL
  ret = hal_if_set_portbased_vlan (zif->ifp->ifindex, hal_bmap);
#endif

  if (ret < 0)
    return -1;

  SET_FLAG (zif->l2_flags, NSM_VLAN_PORT_BASED_VLAN_ENABLE);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_if (zif);
#endif /* HAVE_HA */

  return ret;
}

int
nsm_vlan_port_set_dot1q_state (struct interface *ifp,
                               int enable)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
#ifdef HAVE_HAL
  u_int8_t enable_ingress_filter;
#endif /* HAVE_HAL */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#ifdef HAVE_HAL

  if (zif->switchport != NULL)
   {
      vlan_port = &zif->switchport->vlan_port;

      if (vlan_port == NULL)
        return NSM_VLAN_ERR_IFP_NOT_BOUND;

      if (CHECK_FLAG (vlan_port->flags,
                      NSM_VLAN_ENABLE_INGRESS_FILTER))
        enable_ingress_filter = PAL_TRUE;
      else
        enable_ingress_filter = PAL_FALSE;

      ret = hal_vlan_port_set_dot1q_state (ifp->ifindex, enable,
                                           enable_ingress_filter);

      if (ret != HAL_SUCCESS)
        return NSM_VLAN_ERR_HAL_ERROR;
    }
  else
    {
      ret = hal_vlan_port_set_dot1q_state (ifp->ifindex, enable,
                                           PAL_FALSE);

      if (ret != HAL_SUCCESS)
        return NSM_VLAN_ERR_HAL_ERROR;
    }
#endif

  if (enable == PAL_FALSE)
    {
      SET_FLAG (zif->l2_flags, NSM_VLAN_DOT1Q_DISABLE);
      UNSET_FLAG (zif->l2_flags, NSM_VLAN_DOT1Q_ENABLE);
    }
  else
    {
      SET_FLAG (zif->l2_flags, NSM_VLAN_DOT1Q_ENABLE);
      UNSET_FLAG (zif->l2_flags, NSM_VLAN_DOT1Q_DISABLE);
    }

#ifdef HAVE_HA
  nsm_cal_modify_nsm_if (zif);
#endif /* HAVE_HA */

  return 0;
}

#ifdef HAVE_SMI

/* Get acceptable frame type. */
int
nsm_vlan_get_acceptable_frame_type (struct interface *ifp, 
                        enum smi_acceptable_frame_type *acceptable_frame_type)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
  {
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
  }

  vlan_port = &zif->switchport->vlan_port;

  if (vlan_port == NULL)
  {
    return NSM_API_GET_FAIL;
  }

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
  {
    if (zif->nsm_bridge_port_conf)
    {
      vlan_port = &zif->nsm_bridge_port_conf->vlan_port;
      if (vlan_port == NULL)
      {
        return NSM_API_GET_FAIL;
      }
    }
  }
#endif /* HAVE_LACPD */

  if ((vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED) &&
      (vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED))
  {
    *acceptable_frame_type = SMI_FRAME_TYPE_ALL;
  }
  else if (vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
  {
    *acceptable_frame_type = SMI_FRAME_TYPE_TAGGED;
  }
  else if (vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED)
  {
    *acceptable_frame_type = SMI_FRAME_TYPE_UNTAGGED;
  }
  else
  {
    return NSM_VLAN_ERR_INVALID_FRAME_TYPE;
  }

  return NSM_API_GET_SUCCESS;
}

int
nsm_set_egress_port_mode (struct interface *ifp, int egress_mode)
{
  int ret = 0;

  if (ifp == NULL)
    return SMI_ERROR;

#ifdef HAVE_HAL
  ret = hal_if_set_port_egress (ifp->ifindex, egress_mode);
#endif

  if (ret < 0)
    return SMI_ERROR;

  return ret;
}

int
nsm_set_portbased_vlan (struct interface *ifp, struct smi_port_bmp smibmap)
{
  int ret = 0;
  struct nsm_if *zif;
  struct hal_port_map hal_bmap;

  if (ifp == NULL
      || ifp->info == NULL)
    return SMI_ERROR;

  zif = ifp->info;

  pal_mem_cpy (hal_bmap.bitmap, smibmap.bitmap, sizeof(smibmap.bitmap));
  
  zif->port_vlan = smibmap.bitmap [0];

#ifdef HAVE_HAL
  ret = hal_if_set_portbased_vlan (ifp->ifindex, hal_bmap);
#endif
  if (ret < 0)
    return SMI_ERROR;

  SET_FLAG (zif->l2_flags, NSM_VLAN_PORT_BASED_VLAN_ENABLE);

#ifdef HAVE_HA
  nsm_cal_modify_nsm_if (zif);
#endif /* HAVE_HA */

  return ret;
}

int
nsm_set_cpuport_default_vlan (struct interface *ifp, int vid)
{
  int ret = 0;

  if (ifp == NULL)
    return SMI_ERROR;

#ifdef HAVE_HAL
  ret = hal_if_set_cpu_default_vid (ifp->ifindex, vid);  
#endif

  if (ret < 0)
    return SMI_ERROR;

  return ret;
}

int
nsm_set_waysideport_default_vlan (struct interface *ifp, int vid)
{
 int ret = 0;

  if (ifp == NULL)
    return SMI_ERROR;

#ifdef HAVE_HAL
  ret = hal_if_set_wayside_default_vid (ifp->ifindex, vid);
#endif

  if (ret < 0)
    return SMI_ERROR;

  return ret;
}

int
nsm_force_default_vlan (struct interface *ifp, int vid)
{
  int ret = 0;

  if (ifp == NULL)
    return SMI_ERROR;

#ifdef HAVE_HAL
  ret = hal_if_set_force_vlan (ifp->ifindex, vid);
#endif
   return ret;
}


int 
nsm_svlan_set_port_ether_type (struct interface *ifp, u_int16_t etype)
{
  int ret = 0;

  if (ifp == NULL)
    return SMI_ERROR;

#ifdef HAVE_HAL
   ret = hal_if_set_ether_type (ifp->ifindex, etype);
#endif
  return ret;
}


int 
nsm_set_learn_disable (struct interface *ifp, int enable)
{
  int ret = 0; 
  
  if (ifp == NULL)  
     return SMI_ERROR;  
 
#ifdef HAVE_HAL
  ret = hal_if_set_learn_disable (ifp->ifindex, enable);  
#endif

  if (ret >= 0)    /* success */
  {
    if (enable == PAL_TRUE)
      SET_FLAG (ifp->status, IF_NON_LEARNING_FLAG);
    else
      UNSET_FLAG (ifp->status, IF_NON_LEARNING_FLAG);
  }

  return ret; 
}

int 
nsm_get_learn_disable (struct interface *ifp, enum smi_port_learn_state *enable)
{
  int ret = 0, val;
  

  if (ifp == NULL)
     return SMI_ERROR;

#ifdef HAVE_HAL
  ret = hal_if_get_learn_disable (ifp->ifindex, &val);
#endif

  *enable = val;

  return ret;
}

int
nsm_vlan_port_get_dot1q_state (struct interface *ifp, u_int8_t *enable)
{
  struct nsm_if *zif;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  if (CHECK_FLAG (zif->l2_flags, NSM_VLAN_DOT1Q_DISABLE))
    *enable = PAL_FALSE;
  else
    *enable = PAL_TRUE;

  return 0;

}

#endif /* HAVE_SMI */

/* Set ingress filtering. */
int
nsm_vlan_set_ingress_filter (struct interface *ifp,
                             enum nsm_vlan_port_mode mode,
                             enum nsm_vlan_port_mode sub_mode,
                             int enable)
{
#ifdef HAVE_LACPD
  int ret;
#endif /* HAVE_LACPD */

  struct nsm_if *zif;
  bool_t configure_hal = 0;
  char acceptable_frame_type = 0;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  enum hal_vlan_port_type sub_port_type;
#ifdef HAVE_LACPD
  struct nsm_if *nif = ifp->info;
  struct interface *memifp;
  struct listnode *ln;
#endif

  zif = (struct nsm_if *)ifp->info;
  if ( !zif || !(zif->bridge) || !(zif->switchport))
    {
#ifdef HAVE_LACPD
      if (enable && NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (zif->nsm_bridge_port_conf)
          SET_FLAG (zif->nsm_bridge_port_conf->vlan_port.flags,
                    NSM_VLAN_ENABLE_INGRESS_FILTER);
#endif /* HAVE_LACPD */
      return -1;
    }

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  if (enable)
    {
      configure_hal = 1;
      vlan_port->flags |= NSM_VLAN_ENABLE_INGRESS_FILTER;
    }
  else
    {
      if (vlan_port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER)
        configure_hal = 1;

      vlan_port->flags &= ~NSM_VLAN_ENABLE_INGRESS_FILTER;
    }

  if (vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED)
    {
      acceptable_frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED;
    }
  else if (vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
    {
      acceptable_frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED;
    }
  else
    {
      acceptable_frame_type = HAL_VLAN_ACCEPTABLE_FRAME_TYPE_ALL;
    }

  if (configure_hal)
    {
      switch(vlan_port->mode)
        {
        case NSM_VLAN_PORT_MODE_ACCESS:
          /* Set port type in hal */
          hal_vlan_set_port_type(zif->bridge->name, ifp->ifindex,
                                 HAL_VLAN_ACCESS_PORT,
                                 HAL_VLAN_ACCESS_PORT,
                                 acceptable_frame_type,
                                 enable);
          break;
        case NSM_VLAN_PORT_MODE_HYBRID:
          /* Set port type in hal */
          hal_vlan_set_port_type(zif->bridge->name, ifp->ifindex,
                                 HAL_VLAN_HYBRID_PORT,
                                 HAL_VLAN_HYBRID_PORT,
                                 acceptable_frame_type,
                                 enable);
          break;
        case NSM_VLAN_PORT_MODE_TRUNK:
          /* Set port type in hal */
          hal_vlan_set_port_type(zif->bridge->name, ifp->ifindex,
                                 HAL_VLAN_TRUNK_PORT,
                                 HAL_VLAN_TRUNK_PORT,
                                 acceptable_frame_type,
                                 enable);
          break;
       case NSM_VLAN_PORT_MODE_CE:
         switch (sub_mode)
           {
           case NSM_VLAN_PORT_MODE_ACCESS:
             sub_port_type = HAL_VLAN_ACCESS_PORT;
             break;
           case NSM_VLAN_PORT_MODE_HYBRID:
             sub_port_type = HAL_VLAN_HYBRID_PORT;
             break;
           case NSM_VLAN_PORT_MODE_TRUNK:
             sub_port_type = HAL_VLAN_TRUNK_PORT;
             break;
           default:
             return NSM_VLAN_ERR_INVALID_MODE;
           }

          /* Set port type in hal */
          hal_vlan_set_port_type(zif->bridge->name, ifp->ifindex,
                                 HAL_VLAN_PROVIDER_CE_PORT,
                                 sub_port_type,
                                 acceptable_frame_type,
                                 enable);
         break;
       case NSM_VLAN_PORT_MODE_PN:
       case NSM_VLAN_PORT_MODE_CN:
          /* Set port type in hal */
          hal_vlan_set_port_type(zif->bridge->name, ifp->ifindex,
                                 mode == NSM_VLAN_PORT_MODE_PN ?
                                 HAL_VLAN_PROVIDER_PN_PORT:
                                 HAL_VLAN_PROVIDER_CN_PORT,
                                 mode == NSM_VLAN_PORT_MODE_PN ?
                                 HAL_VLAN_PROVIDER_PN_PORT:
                                 HAL_VLAN_PROVIDER_CN_PORT,
                                 acceptable_frame_type,
                                 enable);
          break;
        default:
          break;
        }
    }

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
    {
      AGGREGATOR_MEMBER_LOOP (nif, memifp, ln)
        {
           NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);
           NSM_BRIDGE_CHECK_AGG_MEM_BRIDGE_PROPERTY(memifp);
           memifp->agg_param_update = 1;

           ret = nsm_vlan_set_ingress_filter (memifp, mode, sub_mode, enable);

           memifp->agg_param_update = 0;

           if (ret < 0)
             return ret;
        }
    }
#endif /* HAVE_LACPD */  

  return 0;
}

/* Get ingress filtering. */
int
nsm_vlan_get_ingress_filter (struct interface *ifp, 
                             enum nsm_vlan_port_mode *mode,
                             enum nsm_vlan_port_mode *sub_mode,
                             int *ingress_filter)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

#ifdef HAVE_LACPD
  int enable = 0;
#endif

  zif = (struct nsm_if *)ifp->info;
  if( !zif || !(zif->bridge) || !(zif->switchport))
  {
    return NSM_API_GET_FAIL;
  }
#ifdef HAVE_LACPD
  if(NSM_INTF_TYPE_AGGREGATOR(ifp))
  {
    if(zif->nsm_bridge_port_conf)
    {
      FLAG_ISSET (zif->nsm_bridge_port_conf->vlan_port.flags, 
                  NSM_VLAN_ENABLE_INGRESS_FILTER) ? \
                  (enable = INGRESS_FILTER_SET): \
                  (enable = INGRESS_FILTER_NOT_SET);

      *ingress_filter = enable;
      *mode = zif->nsm_bridge_port_conf->vlan_port.mode;
      *sub_mode = zif->nsm_bridge_port_conf->vlan_port.sub_mode;

      return NSM_API_GET_SUCCESS;
    }
    else
      return NSM_API_GET_FAIL;
  }
#endif /* HAVE_LACPD */

  br_port = zif->switchport;
  if (br_port == NULL)
    return NSM_API_GET_FAIL;
  
  vlan_port = &br_port->vlan_port;
  if (vlan_port == NULL)
    return NSM_API_GET_FAIL;

  if (vlan_port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER)
    *ingress_filter = INGRESS_FILTER_SET;
  else
    *ingress_filter = INGRESS_FILTER_NOT_SET;
  
  *mode = vlan_port->mode;
  *sub_mode = vlan_port->sub_mode;

  return NSM_API_GET_SUCCESS;
}

#ifdef HAVE_VLAN_CLASS
/* Set access port classifier group. */
int
nsm_vlan_set_access_port_classifier (struct interface *ifp, 
                                     u_int8_t class_grp)
{
  int ret = 0;
#if 0
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_vlan_port *vlan_port;
  struct nsm_msg_vlan_port_classifier msg;
   
  zif = (struct nsm_if *)ifp->info;
  vlan_port = &zif->vlan_port;
  bridge = zif->bridge;
  if (! bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  if (vlan_port->mode != mode)
    return NSM_VLAN_ERR_INVALID_MODE;

  vlan_port->mode = mode;

  if (vid == NSM_VLAN_ALL)
    {
      vlan_port->config = NSM_VLAN_CONFIGURED_ALL;

      /* Get count of VLANs to allocate the PM message vlan info. */
      count = 0;
      for (rn = route_top (bridge->vlan_table); rn; rn = route_next (rn))
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          count++;

      /* Create PM message. */
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
      msg.ifindex = ifp->ifindex;
      msg.num = count;
      msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

      /* For all VLANs configured on a bridge add this port */
      count = 0;
      for (rn = route_top (bridge->vlan_table);
           rn; 
           rn = route_next (rn))
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          {
            nsm_vlan_add_port (bridge, ifp, vlan->vid);

            /* Set port to vlan mapping. */            
            NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, vlan->vid);

            /* Set egress tagged bitmap. */
            if (egress_tagged)
              {
                msg.vid_info[count++] = (1 << NSM_VLAN_EGRESS_SHIFT_BITS) | vlan->vid;
                NSM_VLAN_BMP_SET(vlan_port->egresstaggedBmp, vlan->vid);
              }
            else
              {
                msg.vid_info[count++] = vlan->vid;
                NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vlan->vid);
              }
          }

      /* Send message. */
      nsm_vlan_send_port_map (bridge, &msg, NSM_MSG_VLAN_ADD_PORT);

      XFREE (MTYPE_TMP, msg.vid_info);
    }
  else if (vid == NSM_VLAN_NONE)
    {
      vlan_port->config = NSM_VLAN_CONFIGURED_NONE;

      /* Get count of VLANs to allocate the PM message vlan info. */
      count = 0;
      for (rn = route_top (bridge->vlan_table); rn; rn = route_next (rn))
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          count++;

      /* Create PM message. */
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
      msg.ifindex = ifp->ifindex;
      msg.num = count;
      msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

      /* For all VLANs configured on a bridge delete this port */
      count = 0;
      for (rn = route_top (bridge->vlan_table);
           rn; 
           rn = route_next (rn))
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          {
            /* Set port to vlan mapping. */            
            NSM_VLAN_BMP_UNSET(vlan_port->staticMemberBmp, vlan->vid);
            NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vlan->vid);

            nsm_vlan_delete_port (bridge, ifp, vlan->vid);

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

            /* Set egress tagged bitmap. */
            if (egress_tagged)
              {
                msg.vid_info[count++] = (1 << NSM_VLAN_EGRESS_SHIFT_BITS) | vlan->vid;
                NSM_VLAN_BMP_SET(vlan_port->egresstaggedBmp, vlan->vid);
              }
            else
              {
                msg.vid_info[count++] = vlan->vid;
                NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vlan->vid);
              }
          }

      /* Send message. */
      nsm_vlan_send_port_map(bridge, &msg, NSM_MSG_VLAN_DELETE_PORT);

      XFREE (MTYPE_TMP, msg.vid_info);
    }
  else
    {
      vlan_port->config = NSM_VLAN_CONFIGURED_SPECIFIC;

      /* Add VLAN to port mapping. */
      nsm_vlan_add_port (bridge, ifp, vid);

      /* Add port to VLAN mapping. */
      NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, vid);

      /* Create PM message. */
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
      msg.ifindex = ifp->ifindex;
      msg.num = 1;
      msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

      /* Set egress tagged bitmap. */
      if (egress_tagged)
        {
          msg.vid_info[0] = (1 << NSM_VLAN_EGRESS_SHIFT_BITS) | vid;
          NSM_VLAN_BMP_SET(vlan_port->egresstaggedBmp, vid);
        }
      else
        {
          msg.vid_info[0] = vid;
          NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);
        }

      /* Send message. */
      nsm_vlan_send_port_map (bridge, &msg, NSM_MSG_VLAN_ADD_PORT);

      XFREE (MTYPE_TMP, msg.vid_info);
    }

#endif
  return ret;
}
#endif /* HAVE_VLAN_CLASS */

/* Add VLANs to a trunk/hybrid port. */
int
nsm_vlan_add_common_port (struct interface *ifp, enum nsm_vlan_port_mode mode,
                          enum nsm_vlan_port_mode sub_mode,
                          nsm_vid_t vid, enum nsm_vlan_egress_type egress_tagged,
                          bool_t iterate_members, bool_t notify_kernel)
{
  int count;
  int ret = 0;
  struct nsm_if *zif;
  struct avl_node *rn;
  struct nsm_vlan *vlan = NULL;
  struct nsm_bridge *bridge;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_LACPD
  struct nsm_if *nif = ifp->info;
  struct interface *memifp;
  struct listnode *ln;
  int memret;
#endif /* HAVE_LACPD */
  struct nsm_vlan p;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (! bridge)
    {
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (zif->nsm_bridge_port_conf)
          {
            zif->nsm_bridge_port_conf->vlan_port.pvid = vid;
            zif->nsm_bridge_port_conf->vlan_port.mode = mode;
            NSM_VLAN_BMP_SET(
                 zif->nsm_bridge_port_conf->vlan_port.staticMemberBmp,
                 vid);
            if (egress_tagged)
              {
                NSM_VLAN_BMP_SET(
                    zif->nsm_bridge_port_conf->vlan_port.egresstaggedBmp,
                    vid);
              }  
          }
#endif /* HAVE_LACPD */

      return NSM_VLAN_ERR_IFP_NOT_BOUND;
    }

  if (vlan_port->mode != mode
      || vlan_port->sub_mode != sub_mode)
    return NSM_VLAN_ERR_INVALID_MODE;

  vlan_port->mode = mode;
  vlan_port->sub_mode = sub_mode;

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN
#ifdef HAVE_I_BEB
               ||  vlan_port->mode == NSM_VLAN_PORT_MODE_CNP
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PIP
#endif 
               ?
               bridge->svlan_table : bridge->vlan_table;
#ifdef HAVE_B_BEB
  if ((vlan_port->mode == NSM_VLAN_PORT_MODE_CBP
      ||vlan_port->mode == NSM_VLAN_PORT_MODE_PNP)
      && bridge->backbone_edge)
    vlan_table =  bridge->bvlan_table;
#endif

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  if (vid == NSM_VLAN_ALL)
    {
      vlan_port->config = NSM_VLAN_CONFIGURED_ALL;

      /* Get count of VLANs to allocate the PM message vlan info. */
      count = 0;
      for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          count++;

      if (count == 0)
        return 0;

      /* For all VLANs configured on a bridge add this port */
      count = 0;
      for (rn = avl_top (vlan_table);
           rn;
           rn = avl_next (rn))
      {
        if ((vlan = rn->info) && 
           (((vlan->vid == NSM_VLAN_DEFAULT_VID) &&
             (vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID) &&
             (vlan_port->pvid != NSM_VLAN_DEFAULT_VID))||
             (vlan->vid != NSM_VLAN_DEFAULT_VID)))
          {
            if ((vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID) &&
                (vlan->vid == vlan_port->pvid))
               continue;

            /* If egress tagging has changed we need to delete the port from
               vlan and add it back */
            if ((egress_tagged == NSM_VLAN_EGRESS_TAGGED) && 
                (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vlan->vid)) &&
                !(NSM_VLAN_BMP_IS_MEMBER (vlan_port->egresstaggedBmp, vlan->vid)))
              {
                NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vlan->vid);
#ifdef HAVE_HA
                nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
                if (NSM_INTF_TYPE_AGGREGATOR(ifp))
                  nsm_vlan_delete_port (bridge, ifp, vlan->vid, PAL_FALSE);
                else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
                  nsm_vlan_delete_port (bridge, ifp, vlan->vid, notify_kernel);
              }
            else if ((egress_tagged == NSM_VLAN_EGRESS_UNTAGGED) && 
                     (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vlan->vid)) &&
                     (NSM_VLAN_BMP_IS_MEMBER (vlan_port->egresstaggedBmp, vlan->vid)))
              {
                NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vlan->vid);
                NSM_VLAN_BMP_UNSET (vlan_port->egresstaggedBmp, vlan->vid);
#ifdef HAVE_HA
                nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
                if (NSM_INTF_TYPE_AGGREGATOR(ifp))
                  nsm_vlan_delete_port (bridge, ifp, vlan->vid, PAL_FALSE);
                else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
                  nsm_vlan_delete_port (bridge, ifp, vlan->vid, notify_kernel);
              }

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
            if (NSM_INTF_TYPE_AGGREGATOR(ifp))
              nsm_vlan_add_port (bridge, ifp, vlan->vid, egress_tagged,
                                 PAL_FALSE);
            else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
              nsm_vlan_add_port (bridge, ifp, vlan->vid, egress_tagged,
                                 notify_kernel);

            /* Set port to vlan mapping. */
            NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, vlan->vid);
     
            /* Activate the static fdb configuration for the vlan. */

            nsm_bridge_static_fdb_config_activate (bridge->master, bridge->name, 
                                                   vlan->vid, ifp);

            /* Set egress tagged bitmap. */
            if (egress_tagged == NSM_VLAN_EGRESS_TAGGED)
              NSM_VLAN_BMP_SET(vlan_port->egresstaggedBmp, vlan->vid);
            else
              NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vlan->vid);
          }
      }
    }
  else if (vid == NSM_VLAN_NONE)
    {
      vlan_port->config = NSM_VLAN_CONFIGURED_NONE;

      /* Get count of VLANs to allocate the PM message vlan info. */
      count = 0;
      for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          count++;

      if (count == 0)
        return 0;

      /* For all VLANs configured on a bridge delete this port */
      count = 0;
      for (rn = avl_top (vlan_table);
           rn;
           rn = avl_next (rn))
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          {
            /* Check for the MemberShip */
            if (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vlan->vid))
                && !(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vlan->vid)))
              continue;

            /* Set port to vlan mapping. */
            NSM_VLAN_BMP_UNSET(vlan_port->staticMemberBmp, vlan->vid);
            NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vlan->vid);
#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */


#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
            if (NSM_INTF_TYPE_AGGREGATOR(ifp))
              nsm_vlan_delete_port (bridge, ifp, vlan->vid, PAL_FALSE);
            else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
              nsm_vlan_delete_port (bridge, ifp, vlan->vid, notify_kernel);

          }

    }
  else
    {
      /* To ensure that Non Existing Vlans are not added */
      if (vid != NSM_VLAN_DEFAULT_VID)
       {
         NSM_VLAN_SET (&p, vid);
 
         rn = avl_search (vlan_table, (void *)&p);
         if (rn == NULL  || rn->info == NULL)
            return NSM_VLAN_ERR_VLAN_NOT_FOUND;
       }
          
      vlan_port->config = NSM_VLAN_CONFIGURED_SPECIFIC;
     
      /* If egress tagging has changed we need to delete the port from
         vlan and add it back */
      if ((egress_tagged == NSM_VLAN_EGRESS_TAGGED) && 
          (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid)) &&
         !(NSM_VLAN_BMP_IS_MEMBER (vlan_port->egresstaggedBmp, vid)))
        {
          if ((mode == NSM_VLAN_PORT_MODE_HYBRID) &&
              (vid == vlan_port->pvid))
            return NSM_VLAN_ERR_CONFIG_PVID_TAG;

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

          NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vid);
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
          if (NSM_INTF_TYPE_AGGREGATOR(ifp))
            nsm_vlan_delete_port (bridge, ifp, vid, PAL_FALSE);
          else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
            nsm_vlan_delete_port (bridge, ifp, vid, notify_kernel);
        }
      else if ((egress_tagged == NSM_VLAN_EGRESS_UNTAGGED) && 
               (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid)) &&
               (NSM_VLAN_BMP_IS_MEMBER (vlan_port->egresstaggedBmp, vid)))
        {
          NSM_VLAN_BMP_UNSET (vlan_port->staticMemberBmp, vid);
          NSM_VLAN_BMP_UNSET (vlan_port->egresstaggedBmp, vid);

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
          if (NSM_INTF_TYPE_AGGREGATOR(ifp))
            nsm_vlan_delete_port (bridge, ifp, vid, PAL_FALSE);
          else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
            nsm_vlan_delete_port (bridge, ifp, vid, notify_kernel);
        }

#ifdef HAVE_HA
            nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

      /* Add VLAN to port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        nsm_vlan_add_port (bridge, ifp, vid, egress_tagged, PAL_FALSE);
      else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
        nsm_vlan_add_port (bridge, ifp, vid, egress_tagged, notify_kernel);

      /* Add port to VLAN mapping. */
      NSM_VLAN_BMP_SET(vlan_port->staticMemberBmp, vid);

      /* Activate the static fdb configuration for the vlan. */

      nsm_bridge_static_fdb_config_activate (bridge->master, bridge->name, 
                                             vid, ifp);

      /* Set egress tagged bitmap. */
      if (egress_tagged == NSM_VLAN_EGRESS_TAGGED)
        NSM_VLAN_BMP_SET(vlan_port->egresstaggedBmp, vid);
      else
        NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);
    }

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (key != ifp->lacp_admin_key)
      nsm_server_if_lacp_admin_key_update (ifp);
  }

  if (NSM_INTF_TYPE_AGGREGATOR(ifp) && (iterate_members == PAL_TRUE))
    {
      AGGREGATOR_MEMBER_LOOP (nif, memifp, ln)
      {
        NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);
        NSM_BRIDGE_CHECK_AGG_MEM_BRIDGE_PROPERTY(memifp);

        memifp->agg_param_update = 1;
        memret = nsm_vlan_add_common_port (memifp, mode, sub_mode,
                                           vid, egress_tagged,
                                           iterate_members, notify_kernel);
        memifp->agg_param_update = 0;

        if (memret < 0)
          return memret;
      }
    }
#endif /* HAVE_LACPD */

#ifdef HAVE_MPLS_VC
  /* If some vc bind to this vlan, start it */
  if (vid != NSM_VLAN_NONE)
    nsm_mpls_vc_vlan_bind (ifp, vid);
  else
    nsm_mpls_vc_vlan_unbind (ifp, NSM_VLAN_ALL);
#endif /* HAVE_MPLS_VC */
                                                                                
#ifdef HAVE_VPLS
  /* If some vpls bind to this vlan, start it */
  if (vid != NSM_VLAN_NONE)
    nsm_vpls_vlan_bind (ifp, vid);
  else
    nsm_vpls_vlan_unbind (ifp, NSM_VLAN_ALL);
#endif /* HAVE_VPLS */

  if ((ret == RESULT_OK) && ((vlan_port->mode == NSM_VLAN_PORT_MODE_TRUNK) ||
      (vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID)))
    {
      for (rn = avl_top (vlan_table);
          rn;
          rn = avl_next (rn))
        {
          if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
            {
              if ((vid == NSM_VLAN_ALL) || (vid == NSM_VLAN_NONE))
                listnode_delete (vlan->forbidden_port_list, ifp);
              else
                {
                  if (vid == vlan->vid)
                    {
                      listnode_delete (vlan->forbidden_port_list, ifp);
                      break;
                    }
                }
            }
        }
    }


  return ret;
}

/* Add VLANs to an access port. */
int
nsm_vlan_add_access_port (struct interface *ifp, nsm_vid_t vid,
                          enum nsm_vlan_egress_type egresstagged,
                          bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_add_common_port (ifp, NSM_VLAN_PORT_MODE_ACCESS,
                                   NSM_VLAN_PORT_MODE_ACCESS, vid,
                                   egresstagged, iterate_members,
                                   notify_kernel);
}

/* Add VLANs to a trunk port. */
int
nsm_vlan_add_trunk_port (struct interface *ifp, nsm_vid_t vid,
                         bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_add_common_port (ifp, NSM_VLAN_PORT_MODE_TRUNK,
                                   NSM_VLAN_PORT_MODE_TRUNK, vid,
                                   NSM_VLAN_EGRESS_TAGGED, iterate_members,
                                   notify_kernel);
}

/* Add VLANs to a hybrid port. */
int
nsm_vlan_add_hybrid_port (struct interface *ifp, nsm_vid_t vid, 
                          enum nsm_vlan_egress_type egresstagged,
                          bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_add_common_port (ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                   NSM_VLAN_PORT_MODE_HYBRID, vid,
                                   egresstagged, iterate_members,
                                   notify_kernel);
}

#ifdef HAVE_PROVIDER_BRIDGE

/* Add VLANs to a Provider port. */

int
nsm_vlan_add_provider_port (struct interface *ifp, nsm_vid_t vid,
                            enum nsm_vlan_port_mode mode,
                            enum nsm_vlan_port_mode sub_mode,
                            enum nsm_vlan_egress_type egresstagged,
                            bool_t iterate_members, bool_t notify_kernel)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  if  (!zif || !zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  /* When NSM_VLAN_NONE is given we need to check if any of the vlans
   * has a cvlan registration table entry.
   */

  if (vid == NSM_VLAN_NONE
      && nsm_vlan_port_cvlan_regis_exist (ifp, NSM_VLAN_ALL))
    return NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST;

  return nsm_vlan_add_common_port (ifp, mode, sub_mode, vid,
                                   egresstagged, iterate_members,
                                   notify_kernel);
}

#endif /* HAVE_PROVIDER_BRIDGE */

/* Common delete VLAN function for hybrid and trunk ports. */
int
nsm_vlan_delete_common_port (struct interface *ifp, 
                             enum nsm_vlan_port_mode mode,
                             enum nsm_vlan_port_mode sub_mode, nsm_vid_t vid,
                             bool_t iterate_members, bool_t notify_kernel)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
#ifdef HAVE_LACPD
  struct nsm_if *nif = ifp->info;
  struct interface *memifp;
  struct listnode *ln;
  int memret;
#endif /* HAVE_LACPD */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (vlan_port->mode != mode
      && vlan_port->sub_mode != sub_mode)
    return NSM_VLAN_ERR_INVALID_MODE;

#ifdef HAVE_MPLS_VC
  /* If some vc bind to this vlan, unbind it */
  nsm_mpls_vc_vlan_unbind (ifp, vid);
#endif /* HAVE_MPLS_VC */

  /* Delete port to VLAN mapping. */
  NSM_VLAN_BMP_UNSET(vlan_port->staticMemberBmp, vid);
  NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);

  vlan_port->config = NSM_VLAN_CONFIGURED_SPECIFIC;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  /* 
     If deleted vlan equals pvid on the port 
     set pvid to default vlan id. 
   */ 
  if(vlan_port->pvid == vid
     && mode != NSM_VLAN_PORT_MODE_TRUNK)
  {
    /* Set the port in default mode. */
     nsm_vlan_set_default (bridge, ifp, mode,
                           NSM_VLAN_DEFAULT_VID, notify_kernel);
  }
  /* Delete VLAN from port mapping. */
#if !defined (HAVE_SWFWDR) && defined (HAVE_LACPD)
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
    nsm_vlan_delete_port (bridge, ifp, vid, PAL_FALSE);
  else
#endif /* !HAVE_SWFWDR && HAVE_LACPD */
    nsm_vlan_delete_port (bridge, ifp, vid, notify_kernel);

#ifdef HAVE_LACPD
  {
    u_int16_t key = ifp->lacp_admin_key;
    nsm_lacp_if_admin_key_set (ifp);
    if (key != ifp->lacp_admin_key)
      nsm_server_if_lacp_admin_key_update (ifp);
  }

  if ((NSM_INTF_TYPE_AGGREGATOR(ifp)) && (iterate_members == PAL_TRUE))
    {
  AGGREGATOR_MEMBER_LOOP (nif, memifp, ln)
    {
      NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(memifp);
      NSM_BRIDGE_CHECK_AGG_MEM_BRIDGE_PROPERTY(memifp);

          memifp->agg_param_update = 1;    
          memret = nsm_vlan_delete_common_port (memifp, mode, sub_mode, vid, 
                                               iterate_members, notify_kernel);
          memifp->agg_param_update = 0;    
      if (memret < 0)
        return memret;
    }
    }
#endif /* HAVE_LACPD */

                                                                                
#ifdef HAVE_VPLS
  /* If some vpls bind to this vlan, unbind it */
  nsm_vpls_vlan_unbind (ifp, vid);
#endif /* HAVE_VPLS */

  return 0;
}

/* Delete VLANs from a trunk port. */
int
nsm_vlan_delete_trunk_port (struct interface *ifp, nsm_vid_t vid,
                            bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_delete_common_port (ifp, NSM_VLAN_PORT_MODE_TRUNK,
                                      NSM_VLAN_PORT_MODE_TRUNK, vid,
                                      iterate_members, notify_kernel);
}

/* Delete VLANs from a hybrid port. */
int
nsm_vlan_delete_hybrid_port (struct interface *ifp, nsm_vid_t vid,
                             bool_t iterate_members, bool_t notify_kernel)
{
  return nsm_vlan_delete_common_port (ifp, NSM_VLAN_PORT_MODE_HYBRID,
                                      NSM_VLAN_PORT_MODE_HYBRID, vid,
                                      iterate_members, notify_kernel);
}

#ifdef HAVE_PROVIDER_BRIDGE

/* Delete VLANs from a provider port. */
int
nsm_vlan_delete_provider_port (struct interface *ifp, nsm_vid_t vid,
                               enum nsm_vlan_port_mode mode,
                               enum nsm_vlan_port_mode sub_mode,
                               bool_t iterate_members, bool_t notify_kernel)
{

  if (nsm_vlan_port_cvlan_regis_exist (ifp, vid))
    return NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST;

  return nsm_vlan_delete_common_port (ifp, mode, sub_mode, vid,
                                      iterate_members, notify_kernel);
}

#endif /* HAVE_PROVIDER_BRIDGE */

/* Add all VLANs except vid to a trunk/hybrid port. */
int
nsm_vlan_add_all_except_vid (struct interface *ifp, enum nsm_vlan_port_mode mode,
                             enum nsm_vlan_port_mode sub_mode,
                             struct nsm_vlan_bmp *excludeBmp, 
                             enum nsm_vlan_egress_type egress_tagged,
                             bool_t iterate_members, bool_t notify_kernel)
{
  int ret = 0;
  nsm_vid_t vid = 0;
  struct nsm_if *zif = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_tree *vlan_table;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct interface *ifp_f = NULL;
  struct listnode *ln = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (! bridge)
    {
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (zif->nsm_bridge_port_conf)
          zif->nsm_bridge_port_conf->vlan_port.mode = mode;
#endif /* HAVE_LACPD */

      return NSM_VLAN_ERR_IFP_NOT_BOUND;
    }

  if (vlan_port->mode != mode
      && vlan_port->sub_mode != sub_mode)
    return NSM_VLAN_ERR_INVALID_MODE;

  if (!excludeBmp)
    return NSM_VLAN_ERR_GENERAL;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;

  /* For all VLANs configured on a bridge add this port */
  if (vlan_port->mode == NSM_VLAN_PORT_MODE_TRUNK)
    {
      for (rn = avl_top (vlan_table);
           rn;
           rn = avl_next (rn))
        {
          if ((vlan = rn->info)
              && (!(NSM_VLAN_BMP_IS_MEMBER (*excludeBmp, vlan->vid)))
              && (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp,
                                            vlan->vid))))
            {
              ret = nsm_vlan_add_common_port (ifp, mode, sub_mode, vlan->vid,
                                              egress_tagged, iterate_members,
                                              notify_kernel);
 
              if ( ret < 0 )
                {
                  return ret;
                }
            }
        }
    }
  else
    {
      for (rn = avl_top (vlan_table);
           rn;
           rn = avl_next (rn))
        if ((vlan = rn->info) && 
           (((vlan->vid == NSM_VLAN_DEFAULT_VID) &&
            (vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID) &&
            (vlan_port->pvid != NSM_VLAN_DEFAULT_VID))
            || (vlan->vid != NSM_VLAN_DEFAULT_VID))
            && (!(NSM_VLAN_BMP_IS_MEMBER (*excludeBmp, vlan->vid)))
            && (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vlan->vid))))
          {
            ret = nsm_vlan_add_common_port (ifp, mode, sub_mode, vlan->vid,
                                            egress_tagged, iterate_members,
                                            notify_kernel);
              if ( ret < 0 )
              {
                return ret;
              }
          }
    }
 

  NSM_VLAN_SET_BMP_ITER_BEGIN (*excludeBmp, vid)
    {
      ret = nsm_vlan_delete_common_port (ifp, mode, sub_mode, vid,
                                         iterate_members, notify_kernel);
      if ( ret < 0 )
        return ret;
    }

  NSM_VLAN_SET_BMP_ITER_END (*excludeBmp, vid);

  NSM_VLAN_SET_BMP_ITER_BEGIN (*excludeBmp, vid)
    {
      for (rn = avl_top (vlan_table);
           rn;
           rn = avl_next (rn))
        {
          if ((vlan = rn->info)
              && (vlan->vid == vid)
              && ((vlan_port->mode == NSM_VLAN_PORT_MODE_TRUNK) ||
              (vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID )))
            {
              LIST_LOOP (vlan->forbidden_port_list, ifp_f, ln)
                {
                  if (ifp_f->ifindex == ifp->ifindex)
                    return 0;
                }
              listnode_add (vlan->forbidden_port_list, ifp);
            }
        }
     }

  NSM_VLAN_SET_BMP_ITER_END (*excludeBmp, vid);

  return 0;

}

/* Set Native VLAN */
int
nsm_vlan_set_native_vlan (struct interface *ifp, nsm_vid_t native_vid)
{
  int ret;
  struct nsm_vlan p;
  struct nsm_if *zif = NULL;
  struct avl_node *rn = NULL;
  struct avl_tree *vlan_table;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (! bridge)
    {
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (zif->nsm_bridge_port_conf)
          zif->nsm_bridge_port_conf->vlan_port.native_vid = native_vid;
#endif /* HAVE_LACPD */

      return NSM_VLAN_ERR_IFP_NOT_BOUND;
    }

  if (vlan_port->mode != NSM_VLAN_PORT_MODE_TRUNK)
    return NSM_VLAN_ERR_INVALID_MODE;

  /* Already set to the same native_vid? */
  if (vlan_port->native_vid == native_vid )
    return 0;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_bridge_port (zif->switchport);
#endif /* HAVE_HA */

#ifdef HAVE_PROVIDER_BRIDGE
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
  {
    vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
                 || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
                 bridge->svlan_table : bridge->vlan_table;
  }
  else
    vlan_table = bridge->vlan_table;
#else /* HAVE_PROVIDER_BRIDGE */
   vlan_table = bridge->vlan_table;
#endif /* HAVE_PROVIDER_BRIDGE */

  NSM_VLAN_SET (&p, native_vid);

  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  /* Check whether vlan is part of bridge */
  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {

      if ( !NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, native_vid) )
        {
          return NSM_VLAN_ERR_VLAN_NOT_IN_PORT;
        }

#ifdef HAVE_HAL
      ret = hal_vlan_set_native_vid ( bridge->name, ifp->ifindex, native_vid);
      if (ret < 0)
        {
          return ret;
        }
#endif /* HAVE_HAL */

      vlan_port->native_vid = native_vid;

    }
  else
    {
      return NSM_VLAN_ERR_VLAN_NOT_FOUND;

    }

  return 0;
}

/* Get port Native VID. */
int
nsm_vlan_get_native_vlan (struct interface *ifp, nsm_vid_t *native_vid)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
  {
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
  }

  vlan_port = &zif->switchport->vlan_port;

  if (vlan_port == NULL)
  {
    return NSM_API_GET_FAIL;
  }

#ifdef HAVE_LACPD
  if (NSM_INTF_TYPE_AGGREGATOR(ifp))
  {
    if (zif->nsm_bridge_port_conf)
    {
      vlan_port = &zif->nsm_bridge_port_conf->vlan_port;
      if (vlan_port == NULL)
      {
        return NSM_API_GET_FAIL;
      }
    }
  }
#endif /* HAVE_LACPD */

  *native_vid = vlan_port->native_vid;

  return NSM_API_GET_SUCCESS;
}

/* Set MTU for a VLAN. */
int
nsm_vlan_set_mtu (struct nsm_bridge_master *master, char *brname,
                  u_int16_t vid, u_char vlan_type, u_int32_t mtu_val)
{
  struct nsm_vlan p;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *vlan_table = NULL;

  bridge = nsm_lookup_bridge_by_name(master, brname);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware (master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  vlan_table = (vlan_type & NSM_VLAN_SVLAN) ? bridge->svlan_table :
                                             bridge->vlan_table;

  if (! vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  NSM_VLAN_SET (&p, vid);

  rn = avl_search (vlan_table, (void *)&p);

  if (!rn || !rn->info)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

#if 0
  ret = npf_vlan_set_mtu (brname, vid, mtu_val);

  if ( ret < 0 )
    return NSM_VLAN_ERR_GENERAL;
#endif

  vlan = rn->info;

  vlan->mtu_val = mtu_val;

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_vlan (vlan);
#endif /* HAVE_HA */

  return 0;

}

/* Sync VLAN list. */
static int
nsm_bridge_vlan_list_sync (struct nsm_master *nm, struct nsm_server_entry *nse,
                           struct nsm_bridge *bridge, struct nsm_vlan *vlan)
{
  struct nsm_msg_vlan msg;

  if (!bridge || !vlan)
    return -1;

  if ((vlan->type & NSM_VLAN_STATIC) ||
      (vlan->type & NSM_VLAN_AUTO))
    {
      /* Send VLAN Add message to PM. */
      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan));
      msg.cindex = 0;
      if (vlan->vlan_state == NSM_VLAN_ACTIVE)
        msg.flags |= NSM_MSG_VLAN_STATE_ACTIVE;
      msg.flags |= NSM_MSG_VLAN_STATIC;
      msg.vid = vlan->vid;
      pal_strncpy (msg.vlan_name, vlan->vlan_name, NSM_VLAN_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_NAME);
      pal_strncpy (msg.bridge_name, bridge->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);

      /* Send VLAN add message to PM. */
      nsm_vlan_send_vlan_msg (bridge, &msg, NSM_MSG_VLAN_ADD);
    }

  return 0;
}

static int
nsm_bridge_vlan_port_type_to_msg (enum nsm_vlan_port_mode type)
{
  switch (type)
    {
      case NSM_VLAN_PORT_MODE_ACCESS:
        return NSM_MSG_VLAN_PORT_MODE_ACCESS;
      case NSM_VLAN_PORT_MODE_HYBRID:
        return NSM_MSG_VLAN_PORT_MODE_HYBRID;
      case NSM_VLAN_PORT_MODE_TRUNK:
        return NSM_MSG_VLAN_PORT_MODE_TRUNK;
      case NSM_VLAN_PORT_MODE_CE:
        return NSM_MSG_VLAN_PORT_MODE_CE;
      case NSM_VLAN_PORT_MODE_CN:
        return NSM_MSG_VLAN_PORT_MODE_CN;
      case NSM_VLAN_PORT_MODE_PN:
        return NSM_MSG_VLAN_PORT_MODE_PN;
#if defined HAVE_I_BEB || defined HAVE_B_BEB
      case NSM_VLAN_PORT_MODE_CNP:
        return NSM_MSG_VLAN_PORT_MODE_CNP;
      case NSM_VLAN_PORT_MODE_PIP:
        return NSM_MSG_VLAN_PORT_MODE_PIP;
      case NSM_VLAN_PORT_MODE_CBP:
        return NSM_MSG_VLAN_PORT_MODE_CBP;
#endif /* HAVE_I_BEB || HAVE_B_BEB */
      default:
        return NSM_MSG_VLAN_PORT_MODE_INVALID;
    }

  return NSM_MSG_VLAN_PORT_MODE_INVALID;
}

/* Sync port type information. */
int
nsm_bridge_vlan_port_type_sync (struct nsm_master *nm, 
                                struct nsm_bridge *bridge,
                                struct interface *ifp)
{
  struct nsm_if *zif;
  struct avl_node *avl_port;
  struct interface *mem_ifp;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_msg_vlan_port_type msg;

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((mem_ifp = zif->ifp) == NULL)
        continue;
        
      if (ifp != NULL
          && mem_ifp != ifp)
        continue;

#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATED (mem_ifp))
        continue;
#endif /* HAVE_LACPD */

      vlan_port = &br_port->vlan_port;

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_INVALID)
        {
          pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port_type));
          msg.ifindex = mem_ifp->ifindex;

          msg.port_type = nsm_bridge_vlan_port_type_to_msg (vlan_port->mode);

          msg.ingress_filter = (vlan_port->flags & NSM_VLAN_ENABLE_INGRESS_FILTER) ?
                                NSM_MSG_VLAN_PORT_ENABLE_INGRESS_FILTER :
                                NSM_MSG_VLAN_PORT_DISABLE_INGRESS_FILTER;

          msg.acceptable_frame_type =
                               (vlan_port->flags & NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED) ?
                                NSM_MSG_VLAN_ACCEPT_TAGGED_ONLY : NSM_MSG_VLAN_ACCEPT_FRAME_ALL;

          /* Send port type message to PM. */
          nsm_vlan_send_port_type_msg (bridge, &msg);
        }
    }

  return 0;
}

/* Sync port vlan membership information. */
int
nsm_bridge_vlan_port_membership_sync (struct nsm_master *nm, 
                                      struct nsm_bridge *bridge,
                                      struct interface *ifp)
{
  int count;
  nsm_vid_t vid;
#ifdef HAVE_PROVIDER_BRIDGE
  u_int16_t svid;
#endif /* HAVE_PROVIDER_BRIDGE */
  struct nsm_if *zif;
  struct avl_node *avl_port;
  struct interface *mem_ifp;
  struct nsm_msg_vlan_port msg;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_svlan_reg_info *svlan_info = NULL;
  struct nsm_pro_edge_info *pro_edge_port;
  struct listnode *ls_node = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((mem_ifp = zif->ifp) == NULL)
        continue;

      if (ifp != NULL
          && mem_ifp != ifp)
        continue;

#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATED (mem_ifp))
        continue;
#endif /* HAVE_LACPD */

      vlan_port = &br_port->vlan_port;

      if (vlan_port->mode == NSM_VLAN_PORT_MODE_ACCESS 
          || vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID)
        {
          pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
          msg.ifindex = mem_ifp->ifindex;
          msg.num = 1;
          msg.vid_info = XCALLOC (MTYPE_TMP, sizeof (u_int16_t) * msg.num);

          /* Set egress. */
          if (NSM_VLAN_BMP_IS_MEMBER(vlan_port->egresstaggedBmp, vlan_port->pvid))
            msg.vid_info[0] = (1 << NSM_VLAN_EGRESS_SHIFT_BITS) | vlan_port->pvid;
          else
            msg.vid_info[0] = vlan_port->pvid;

          /* Set default. */
          msg.vid_info[0] |= (1 << NSM_VLAN_DEFAULT_SHIFT_BITS);

          /* Send message. */
          nsm_vlan_send_port_map (bridge, &msg, NSM_MSG_VLAN_ADD_PORT);

          XFREE (MTYPE_TMP, msg.vid_info);
        }

      /* Get count of the vids. */
      count = 0;
      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
        {
          count++;
        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port-staticMemberBmp, vid);

      if (count > 0)
        {
          pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
          msg.ifindex = mem_ifp->ifindex;
          msg.num = count;
          msg.vid_info = XCALLOC (MTYPE_TMP, sizeof (u_int16_t) * msg.num);

          count = 0;
          NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
            {
              /* Set egress. */
              if (NSM_VLAN_BMP_IS_MEMBER(vlan_port->egresstaggedBmp, vlan_port->pvid))
                msg.vid_info[count] = (1 << NSM_VLAN_EGRESS_SHIFT_BITS) | vid;
              else
                msg.vid_info[count] = vid;

              if (vid == NSM_VLAN_DEFAULT_VID)
                msg.vid_info[count] |= (1 << NSM_VLAN_DEFAULT_SHIFT_BITS);

              count++;
            }
          NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);

          /* Send message. */
          nsm_vlan_send_port_map (bridge, &msg, NSM_MSG_VLAN_ADD_PORT);
          XFREE (MTYPE_TMP, msg.vid_info);
        }

#ifdef HAVE_PROVIDER_BRIDGE
        if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE
            && vlan_port->regtab != NULL)
          {
            /* Sending svlans on this CE port to PMs */
            msg.ifindex = mem_ifp->ifindex;
            msg.num = NSM_PEP_SVID_MSG_NUM;
            msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

            NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->svlanMemberBmp, svid)
              {
                msg.vid_info[0] = svid;
                msg.vid_info[1] = 0;
                nsm_vlan_send_port_map (zif->bridge, &msg, 
                                        NSM_MSG_SVLAN_ADD_CE_PORT);

                if ((svlan_info = nsm_reg_tab_svlan_info_lookup (vlan_port->regtab,
                         svid)) == NULL)
                   continue;
                 
                LIST_LOOP (svlan_info->port_list, pro_edge_port, ls_node)
                  {
                    /* Untagged vid for this PE port */
                    msg.vid_info[0] = svid;
                    msg.vid_info[1] = pro_edge_port->untagged_vid;
                    nsm_vlan_send_port_map (zif->bridge, &msg, 
                        NSM_MSG_UNTAGGED_VID_PE_PORT);

                    /* Default vid for this PE port */
                    msg.vid_info[0] = svid;
                    msg.vid_info[1] = pro_edge_port->pvid;
                    nsm_vlan_send_port_map (zif->bridge, &msg, 
                        NSM_MSG_DEFAULT_VID_PE_PORT);
                  }/* LIST_LOOP (svlan_info->port_list, pro_edge_port, 
                                 ls_node) */
              }
            NSM_VLAN_SET_BMP_ITER_END (vlan_port->svlanMemberBmp, svid);

            XFREE (MTYPE_TMP, msg.vid_info);
          }
#endif /* HAVE_PROVIDER_BRIDGE */

    } /* bridge->port_tree loop */
  return 0;
}

/* Sync VLAN information from NSM to PM. */
int 
nsm_bridge_vlan_sync (struct nsm_master *nm, struct nsm_server_entry *nse)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *rn;
  struct nsm_vlan *vlan;

  if (! nm || ! nse || ! nm->bridge)
    return -1;

  master = nsm_bridge_get_master (nm);
  if (! master)
    return -1;

  bridge = master->bridge_list;
  while (bridge)
    {
      if ( nsm_bridge_is_vlan_aware(master, bridge->name) )
        {
          /* Send VLAN list. */
          if (bridge->vlan_table != NULL)
            for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
               if ((vlan = rn->info))
                 {
                   nsm_bridge_vlan_list_sync (nm, nse, bridge, vlan);
                 }

#if defined HAVE_PROVIDER_BRIDGE || defined HAVE_I_BEB
          if (bridge->svlan_table != NULL)
            for (rn = avl_top (bridge->svlan_table); rn; rn = avl_next (rn))
               if ((vlan = rn->info))
                 {
                   nsm_bridge_vlan_list_sync (nm, nse, bridge, vlan);
                 }
#endif /* HAVE_PROVIDER_BRIDGE */

          /* Send port type. */
          nsm_bridge_vlan_port_type_sync (nm, bridge, NULL);

          /* Send port vlan membership. */
          nsm_bridge_vlan_port_membership_sync (nm, bridge, NULL);
        }

      bridge = bridge->next;
    }
#ifdef HAVE_B_BEB
  bridge = master->b_bridge;
  if (bridge)
    {
      if (bridge->bvlan_table != NULL)
        for (rn = avl_top (bridge->bvlan_table); rn; rn = avl_next (rn))
          if ((vlan = rn->info))
            {
              nsm_bridge_vlan_list_sync (nm, nse, bridge, vlan);
            }
      /* Send port type. */
      nsm_bridge_vlan_port_type_sync (nm, bridge, NULL);

      /* Send port vlan membership. */
      nsm_bridge_vlan_port_membership_sync (nm, bridge, NULL);
    }
#endif /* HAVE_B_BEB */

  return 0;
}
#endif /* HAVE_CUSTOM1 */

/* Vlan listener interface */
int nsm_add_listener_to_list(struct list *listener_list,
                             struct nsm_vlan_listener *appln)
{
  if ( !(appln) || !(listener_list) ) 
    {
      return RESULT_ERROR;
    }

  if ( ((appln->listener_id) < NSM_LISTENER_ID_MIN) ||
       ((appln->listener_id) >= NSM_LISTENER_ID_MAX) ) 
    {
      return RESULT_ERROR;
    }

  if ( !(listnode_lookup(listener_list, appln)) ) 
    {
      listnode_add(listener_list, appln);
    }
  return RESULT_OK;
}

void nsm_remove_listener_from_list(struct list *listener_list,
                                   nsm_listener_id_t appln_id)
{
  struct listnode *ln = NULL;
  struct listnode *lnext = NULL;
  struct nsm_vlan_listener *appln = NULL;

  if ( !(listener_list) ||
       (appln_id < NSM_LISTENER_ID_MIN) ||
       (appln_id >= NSM_LISTENER_ID_MAX) )
    {
      return;
    }

  for (ln = LISTHEAD (listener_list); ln; ln = lnext)
    {
      lnext = ln->next;

      appln = (struct nsm_vlan_listener *)GETDATA (ln);

      if(appln->listener_id == appln_id)
        {
          listnode_delete(listener_list, appln);
          break;
        }
    }
}

int nsm_create_vlan_listener(struct nsm_vlan_listener **appln)
{
  if (((*appln) =
       XCALLOC (MTYPE_VLAN_DATABASE, sizeof (struct nsm_vlan_listener))) ==
      NULL)
    return RESULT_ERROR;

  (*appln)->listener_id = NSM_LISTENER_ID_MAX;
  (*appln)->add_vlan_to_bridge_func = NULL;
  (*appln)->delete_vlan_from_bridge_func = NULL;
  (*appln)->add_vlan_to_port_func = NULL;
  (*appln)->delete_vlan_from_port_func = NULL;
  return RESULT_OK;
}

void nsm_destroy_vlan_listener(struct nsm_vlan_listener *appln)
{
  if ( appln != NULL ) 
    {
      appln->add_vlan_to_bridge_func = NULL;
      appln->delete_vlan_from_bridge_func = NULL;
      appln->add_vlan_to_port_func = NULL;
      appln->delete_vlan_from_port_func = NULL;

      XFREE (MTYPE_VLAN_DATABASE, appln);
    }
}

/* Config store and activation */
struct nsm_vlan_config_entry *
nsm_vlan_config_entry_new (int vid)
{
  struct nsm_vlan_config_entry *vlan_conf_new = NULL;

  vlan_conf_new = XCALLOC (MTYPE_VLAN_CONFIG_ENTRY,
                           sizeof (struct nsm_vlan_config_entry));

  if (vlan_conf_new)
    vlan_conf_new->vid = vid;

  return vlan_conf_new;
}

void
nsm_vlan_config_entry_insert (struct nsm_bridge_master *master,
                              char *br_name, struct nsm_vlan_config_entry *vlan_conf_new)
{
  struct nsm_bridge_config *br_conf;
  struct nsm_vlan_config_entry *tmp;
  struct listnode *ln;

  br_conf = nsm_bridge_config_find(master, br_name);
  if ( !br_conf )
    return;

  if (br_conf->vlan_config_list != NULL)
    {
      LIST_LOOP(br_conf->vlan_config_list, tmp, ln)
        {
          if (tmp->vid == vlan_conf_new->vid)
            {
              /* remove the old config */
              listnode_delete(br_conf->vlan_config_list, tmp);
              XFREE (MTYPE_VLAN_CONFIG_ENTRY, tmp);
            }
        }
    }

  listnode_add(br_conf->vlan_config_list,vlan_conf_new);
  return;
}

void
nsm_vlan_config_entry_free (struct nsm_vlan_config_entry *vlan_conf_entry)
{
  XFREE (MTYPE_VLAN_CONFIG_ENTRY, vlan_conf_entry);
  return;
}

struct nsm_vlan_config_entry *
nsm_vlan_config_entry_find (struct nsm_bridge_master *master,
                            char* br_name, int vid)
{
  struct nsm_bridge_config *br_conf = NULL;
  struct nsm_vlan_config_entry * vlan_config_entry = NULL;
  struct nsm_vlan_config_entry * tmp= NULL;
  struct listnode *ln;

  br_conf = nsm_bridge_config_find (master, br_name);
  if (!br_conf)
    {
      br_conf = nsm_bridge_config_new (master, br_name);
      if (!br_conf)
        return NULL;
    }

  if (br_conf->vlan_config_list != NULL)
    {
      LIST_LOOP(br_conf->vlan_config_list, tmp, ln)
        {
          if (tmp->vid == vid)
            {
              vlan_config_entry = tmp;
            }
        }
    }

  if (!vlan_config_entry)
    {
      vlan_config_entry = nsm_vlan_config_entry_new (vid);
      if (!vlan_config_entry)
        return NULL;
      nsm_vlan_config_entry_insert (master, br_name, vlan_config_entry);
    }
  return vlan_config_entry;
}

int
nsm_vlan_validate_vid (char *str, struct cli *cli)
{
  nsm_vid_t vid;
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, str, 2, 4094);
  return CLI_SUCCESS;
}

int
nsm_vlan_validate_vid1 (char *str, struct cli *cli)
{
  nsm_vid_t vid;
  CLI_GET_INTEGER_RANGE ("VLAN id", vid, str, NSM_VLAN_DEFAULT_VID,
                         NSM_VLAN_CLI_MAX);
  return CLI_SUCCESS;
}

/* This check is for mpls-vc and mpls-vpls bind to interface vlan.
   Check if this interface has bind to a bridge. 
   If vlan_port mode is ACCESS, only vlan-id 0 is allowed. */
int
nsm_vlan_id_validate_by_interface (struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  if (ifp == NULL)
    return NSM_ERR_IF_BIND_VLAN_ERR;

  zif = (struct nsm_if *)ifp->info;
  if (zif && zif->switchport)
    {
      bridge = zif->bridge; 
      if (! bridge)
        return NSM_ERR_IF_BIND_VLAN_ERR;

      br_port = zif->switchport;
      vlan_port = &br_port->vlan_port;
      if (!vlan_port)
        return NSM_ERR_IF_BIND_VLAN_ERR;

      if ((vlan_port->mode == NSM_VLAN_PORT_MODE_ACCESS) && (vlan_id != 0))
        return NSM_ERR_IF_BIND_VLAN_ERR;
    }
  return NSM_SUCCESS;
}

/* Check for given interface and vlan-id, is the vlan exist. */ 
int
nsm_is_vlan_exist (struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  if (ifp == NULL)
    return NSM_FAILURE;

  zif = (struct nsm_if *)ifp->info;
  if (zif && zif->switchport)
    {
      bridge = zif->bridge; 

      if (! bridge)
        return NSM_FAILURE;

      br_port = zif->switchport;
      vlan_port = &br_port->vlan_port;

      if (!vlan_port)
        return NSM_FAILURE;

      if ((vlan_port->mode == NSM_VLAN_PORT_MODE_ACCESS) && (vlan_id == 0))
        return NSM_SUCCESS;

      if ((vlan_port->mode == NSM_VLAN_PORT_MODE_ACCESS) && (vlan_id != 0))
        return NSM_FAILURE;

#ifdef HAVE_PROVIDER_BRIDGE
      if ((vlan_port->mode == NSM_VLAN_PORT_MODE_CE) && (vlan_id == 0))
	return NSM_SUCCESS;

      if ((vlan_port->mode == NSM_VLAN_PORT_MODE_CE) && (vlan_id != 0))
	return NSM_FAILURE;
#endif /* HAVE_PROVIDER_BRIDGE */

      if ((NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vlan_id)) ||
#ifdef HAVE_PROVIDER_BRIDGE
	  (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, vlan_id)) ||
#endif /* HAVE_PROVIDER_BRIDGE */
          (NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vlan_id))) 
	return NSM_SUCCESS;
    }
  return NSM_FAILURE;
}

#ifdef HAVE_LACPD
int
nsm_vlan_agg_mode_vlan_sync(struct nsm_bridge_master *master,
                            struct interface *ifp,
                            bool_t use_conf)
{
  struct nsm_if *agg_zif;
  struct nsm_bridge_port_conf *nsm_bridge_port_conf;
  struct nsm_if *mem_zif;
  struct interface *memifp;
  struct listnode *ln;
  nsm_vid_t vid;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  enum nsm_vlan_egress_type egress_tagged;

  if (!NSM_INTF_TYPE_AGGREGATOR(ifp))
    return -1;

  agg_zif = (struct nsm_if *)ifp->info;
  if (agg_zif && (listcount(agg_zif->agg.info.memberlist) > 0))
    {
      if (!use_conf)
        {
          ln=(agg_zif->agg.info.memberlist)->head;
          memifp=ln->data;
          if (!memifp)
            return -1;

          mem_zif = (struct nsm_if *)memifp->info;

          if (!mem_zif || !mem_zif->switchport)
            return -1;

          br_port = mem_zif->switchport;

          if (!br_port)
            return -1;

          vlan_port = &br_port->vlan_port;
        }
      else
        {
          nsm_bridge_port_conf = agg_zif->nsm_bridge_port_conf;
          if (! nsm_bridge_port_conf)
            return -1;
          vlan_port = &nsm_bridge_port_conf->vlan_port;
        }

      nsm_vlan_api_set_port_mode (ifp, vlan_port->mode, vlan_port->sub_mode,
                                  PAL_FALSE, PAL_TRUE);

      switch (vlan_port->mode)
        {
          case NSM_VLAN_PORT_MODE_ACCESS:
            if (vlan_port->pvid != NSM_VLAN_DEFAULT_VID)
              nsm_vlan_set_access_port(ifp, vlan_port->pvid,
                                       PAL_FALSE, PAL_TRUE);
           break;
          case NSM_VLAN_PORT_MODE_TRUNK:
            {
              if (vlan_port->native_vid != NSM_VLAN_DEFAULT_VID)
                nsm_vlan_set_native_vlan (ifp, vlan_port->native_vid);

              if (vlan_port->config == NSM_VLAN_CONFIGURED_ALL)
                nsm_vlan_add_trunk_port (ifp, NSM_VLAN_ALL, PAL_FALSE, 
                                         PAL_TRUE);
              else if (vlan_port->config == NSM_VLAN_CONFIGURED_NONE)
                nsm_vlan_add_trunk_port (ifp, NSM_VLAN_NONE, PAL_FALSE, 
                                         PAL_TRUE);
              else
                {
                  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
                    {
                      nsm_vlan_add_trunk_port (ifp, vid, PAL_FALSE, PAL_TRUE);
                    }
                  NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);
                }
            }
           break;
          case NSM_VLAN_PORT_MODE_HYBRID:
            {
              if (vlan_port->pvid != NSM_VLAN_DEFAULT_VID)
                nsm_vlan_set_hybrid_port(ifp, vlan_port->pvid, 
                                         PAL_FALSE, PAL_TRUE);

              if (vlan_port->config == NSM_VLAN_CONFIGURED_ALL)
                nsm_vlan_add_hybrid_port (ifp, NSM_VLAN_ALL, 
                                          NSM_VLAN_EGRESS_TAGGED, 
                                          PAL_FALSE, PAL_TRUE);
              else if (vlan_port->config == NSM_VLAN_CONFIGURED_NONE)
                nsm_vlan_add_hybrid_port (ifp, NSM_VLAN_NONE,
                                          NSM_VLAN_EGRESS_TAGGED,
                                          PAL_FALSE, PAL_TRUE);
              else
                {
                  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
                    {
                      egress_tagged = 
                            NSM_VLAN_BMP_IS_MEMBER (vlan_port->egresstaggedBmp,
                                     vid) ? NSM_VLAN_EGRESS_TAGGED : 
                                            NSM_VLAN_EGRESS_UNTAGGED;  
                      nsm_vlan_add_hybrid_port (ifp, vid, egress_tagged,
                                                PAL_FALSE, PAL_TRUE);
                    }
                  NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);
                }
            }
           break;
#ifdef HAVE_PROVIDER_BRIDGE
          case NSM_VLAN_PORT_MODE_CE:
          case NSM_VLAN_PORT_MODE_CN:
          case NSM_VLAN_PORT_MODE_PN:
            if (vlan_port->pvid != NSM_VLAN_DEFAULT_VID)
              nsm_vlan_set_provider_port (ifp, vlan_port->pvid,
                                          vlan_port->mode,
                                          vlan_port->sub_mode,
                                          PAL_FALSE, PAL_TRUE);

            if (vlan_port->config == NSM_VLAN_CONFIGURED_ALL)
              nsm_vlan_add_provider_port (ifp, NSM_VLAN_ALL,
                                          vlan_port->mode,
                                          vlan_port->sub_mode,
                                          NSM_VLAN_EGRESS_TAGGED,
                                          PAL_FALSE, PAL_TRUE);
            else if (vlan_port->config == NSM_VLAN_CONFIGURED_NONE)
              nsm_vlan_add_provider_port (ifp, NSM_VLAN_NONE,
                                          vlan_port->mode,
                                          vlan_port->sub_mode,
                                          NSM_VLAN_EGRESS_TAGGED,
                                          PAL_FALSE, PAL_TRUE);
            else
              {
                NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
                  {
                    egress_tagged =
                          NSM_VLAN_BMP_IS_MEMBER (vlan_port->egresstaggedBmp,
                                   vid) ? NSM_VLAN_EGRESS_TAGGED :
                                          NSM_VLAN_EGRESS_UNTAGGED;
                    nsm_vlan_add_provider_port (ifp, vid,
                                                vlan_port->mode,
                                                vlan_port->sub_mode,
                                                egress_tagged,
                                                PAL_FALSE, PAL_TRUE);
                  }
                NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);
              }
           break;
#endif /* HAVE_PROVIDER_BRIDGE */
          default:
           break;
        } /* End of Switch */

        if (CHECK_FLAG (vlan_port->flags,
                        NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED))
          nsm_vlan_set_acceptable_frame_type (ifp, vlan_port->mode,
                                         NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED);

        if (CHECK_FLAG (vlan_port->flags,
                        NSM_VLAN_ENABLE_INGRESS_FILTER))
          nsm_vlan_set_ingress_filter (ifp, vlan_port->mode,
                                       vlan_port->sub_mode, PAL_TRUE);

#ifdef HAVE_PROVIDER_BRIDGE
        if (vlan_port->mode == NSM_VLAN_PORT_MODE_CE
            && vlan_port->regtab != NULL)
          nsm_cvlan_reg_tab_if_apply (ifp, vlan_port->regtab->name);
#endif /* HAVE_PROVIDER_BRIDGE */
    }

  return RESULT_OK;
}
#endif /* HAVE_LACPD */

static int
_nsm_vlan_id_cmp_int (nsm_vid_t first, nsm_vid_t second)
{
  if (first > second)
    return 1;

  if (second > first)
    return -1;

  return 0;
}

int 
nsm_vlan_id_cmp (void * data1, void * data2)
{
  struct nsm_vlan *vlan1 = (struct nsm_vlan *) data1;
  struct nsm_vlan *vlan2 = (struct nsm_vlan *) data2;

  if (!data1 || !data2)
    return -1;

  return _nsm_vlan_id_cmp_int(vlan1->vid, vlan2->vid);
}

#ifdef HAVE_SMI
/* Get all the vlan ids configured on a bridge */
int
nsm_get_all_vlan_config (char *br_name, struct smi_vlan_bmp *vlan_bitmap)
{
  struct nsm_bridge *bridge;
  struct nsm_vlan *vlan;
  struct avl_node *an;
  struct avl_tree *vlan_table;
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, SMI_DFLT_VRID);
  if (! nm)
    return NSM_API_GET_ERROR;

  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  if (! master)
    return NSM_API_GET_ERROR;
 
  bridge = nsm_lookup_bridge_by_name (master, br_name);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return NSM_API_GET_ERROR;
  
  SMI_BMP_INIT (*vlan_bitmap);

  for (an = avl_top(vlan_table); an; an = avl_next(an))
  {
    if ((vlan = an->info) == NULL)
      continue;

    SMI_BMP_SET (*vlan_bitmap, vlan->vid);
  }  
  return NSM_API_GET_SUCCESS;
}

/* Get the configuration of a vlan from vid */
int
nsm_get_vlan_by_id (char *br_name, int vlan_id, 
                    struct smi_vlan_info *vlan_info)
{
  struct nsm_bridge *bridge;
  struct nsm_vlan p, *vlan;
  struct avl_node *an;
  struct avl_tree *vlan_table;
  struct interface *ifp = NULL;
  struct listnode *ln = NULL;

  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, SMI_DFLT_VRID);
  if (! nm)
    return NSM_API_GET_ERROR;

  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  if (! master)
    return NSM_API_GET_ERROR;

  bridge = nsm_lookup_bridge_by_name (master, br_name);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return NSM_API_GET_ERROR;

  NSM_VLAN_SET (&p, vlan_id);
  an = avl_search (vlan_table, (void *)&p);

  if ((an == NULL) || (an->info == NULL))
    return NSM_API_GET_ERROR;

  vlan = an->info;
  
  if (vlan->vid != vlan_id)
    return NSM_API_GET_ERROR;

  snprintf(vlan_info->vlan_name, SMI_VLAN_NAMSIZ, vlan->vlan_name);
  /* bridge id NOT writen */
  vlan_info->vid = vlan->vid;
  vlan_info->type = vlan->type;
  vlan_info->vlan_state = vlan->vlan_state;
  vlan_info->mtu_val = vlan->mtu_val;

  /* Port list */
  SMI_BMP_INIT(vlan_info->port_list);
  LIST_LOOP(vlan->port_list, ifp, ln)
  {
    if ((ifp->ifindex - (SMI_L2_IFINDEX_START + 1)) < 5)
      { 
        /* For  fe1 (Port 0) to fe5 (Port 4) */
        SMI_BMP_SET (vlan_info->port_list, 
                    (ifp->ifindex - (SMI_L2_IFINDEX_START + 1)));
      }
    else
      {
        /* For Port fe6(Port 6), fe7 (Port 7) and ge1 (port 8), ge2 Port 9 */
        SMI_BMP_SET (vlan_info->port_list,
                    (ifp->ifindex - SMI_L2_IFINDEX_START));
      }
  }
  vlan_info->instance = vlan->instance;
  return NSM_API_GET_SUCCESS;
}  

/* Get Vlan configuration on an interface */
int
nsm_vlan_if_get (struct interface *ifp, struct smi_if_vlan_info *if_vlan_info)
{
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;

  if (ifp == NULL)
    return NSM_ERR_IF_BIND_VLAN_ERR;

  zif = (struct nsm_if *)ifp->info;
  if (zif && zif->switchport)
  {
    bridge = zif->bridge;
    if (! bridge)
      return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

    vlan_port = &zif->switchport->vlan_port;
    if (!vlan_port)
      return NSM_ERR_IF_BIND_VLAN_ERR;
 
    snprintf(if_vlan_info->name, INTERFACE_NAMSIZ, ifp->name);
    if_vlan_info->mode = vlan_port->mode;
    if_vlan_info->sub_mode = vlan_port->sub_mode;
    if_vlan_info->pvid = vlan_port->pvid;
    if_vlan_info->native_vid = vlan_port->native_vid;
#ifdef HAVE_VLAN_STACK
    if_vlan_info->tag_ethtype = vlan_port->tag_ethtype;
#endif /* HAVE_VLAN_STACK */
    if_vlan_info->flags = vlan_port->flags;
  
    if_vlan_info->config = vlan_port->config;
 
     pal_mem_cpy (&if_vlan_info->staticMemberBmp, 
                  &vlan_port->staticMemberBmp,
                  sizeof(struct smi_vlan_bmp));

     pal_mem_cpy (&if_vlan_info->dynamicMemberBmp, 
                  &vlan_port->dynamicMemberBmp,
                  sizeof(struct smi_vlan_bmp));
  }
  return NSM_API_GET_SUCCESS;
}

/* Get the configuration of a bridge */
int
nsm_get_bridge_config(char *br_name, struct smi_bridge *smi_bridge)
{
  struct nsm_bridge *bridge;
  struct nsm_vlan *vlan;
  struct avl_node *an;
  struct nsm_bridge_port *br_port;

  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  bridge = nsm_lookup_bridge_by_name (master, br_name);
  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  snprintf(smi_bridge->name, SMI_BRIDGE_NAMSIZ, br_name);
  /* TODO: bridge_id  ? */
  smi_bridge->type = bridge->type;
  smi_bridge->is_default = bridge->is_default;
  smi_bridge->ageing_time = bridge->ageing_time;
  smi_bridge->learning = bridge->learning;

  /* Port bitmap */
  SMI_BMP_INIT(smi_bridge->port_list);
  for (an = avl_top(bridge->port_tree); an; an = avl_next(an))
  {
    if ((br_port = AVL_NODE_INFO(an)) == NULL)
      continue;

    SMI_BMP_SET(smi_bridge->port_list, br_port->ifindex);
  }

  /* Vlan bitmap */
  SMI_BMP_INIT(smi_bridge->vlanbmp);
  for (an = avl_top(bridge->vlan_table); an; an = avl_next(an))
  {
    if ((vlan = an->info) == NULL)
      continue;

    SMI_BMP_SET (smi_bridge->vlanbmp, vlan->vid);
  }
    
#ifdef HAVE_GVRP
  if (bridge->gvrp != NULL)
    smi_bridge->gvrp_enabled = SMI_GVRP_ENABLED;
  else 
    smi_bridge->gvrp_enabled = SMI_GVRP_DISABLED;
#endif /* HAVE_GVRP */

#ifdef HAVE_GMRP
  if ((bridge->gmrp_bridge != NULL) || (bridge->gmrp_list != NULL))
    smi_bridge->gmrp_enabled = SMI_GMRP_ENABLED;
  else
    smi_bridge->gmrp_enabled = SMI_GMRP_DISABLED;
#endif /* HAVE_GMRP */

  smi_bridge->traffic_class_enabled = bridge->traffic_class_enabled;
  smi_bridge->topology_type = bridge->topology_type;

  return NSM_API_GET_SUCCESS;
}    

/* Function is used to update vlan list for alarms */ 
int
nsm_vlan_update_vid_bmp (struct interface *ifp, enum nsm_vlan_port_mode mode,
                         struct smi_vlan_bmp *vlan_bmp, 
                         struct smi_vlan_bmp *egr_vlan_bmp)

{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
 
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
 
  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  pal_mem_cpy (vlan_bmp, &vlan_port->staticMemberBmp,
                sizeof (struct smi_vlan_bmp));     

  pal_mem_cpy (egr_vlan_bmp, &vlan_port->egresstaggedBmp,
               sizeof (struct smi_vlan_bmp));    
 
  return 0;
}

int
nsm_vlan_remove_all_except_vid (struct interface *ifp, enum nsm_vlan_port_mode mode, 
                                enum nsm_vlan_port_mode sub_mode,
                                struct nsm_vlan_bmp *excludeBmp,
                                enum nsm_vlan_egress_type egress_tagged,
                                enum smi_vlan_add_opt vlan_add_opt)
{
  struct nsm_if *zif = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_tree *vlan_table;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  int ret=0;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;
  bridge = zif->bridge;

  if (! bridge)
    {
#ifdef HAVE_LACPD
      if (NSM_INTF_TYPE_AGGREGATOR(ifp))
        if (zif->nsm_bridge_port_conf)
          zif->nsm_bridge_port_conf->vlan_port.mode = mode;
#endif /* HAVE_LACPD */

      return NSM_VLAN_ERR_IFP_NOT_BOUND;
    }
 
  if (egress_tagged != NSM_VLAN_EGRESS_TAGGED)
   return -1;

  vlan_table = vlan_port->mode == NSM_VLAN_PORT_MODE_CN
               || vlan_port->mode == NSM_VLAN_PORT_MODE_PN ?
               bridge->svlan_table : bridge->vlan_table;

  if ((mode != vlan_port->mode) ||
      (vlan_port->mode == SMI_VLAN_PORT_MODE_ACCESS))
    return -1;

  switch (mode)
   {
     case NSM_VLAN_PORT_MODE_HYBRID:
       ret =  nsm_vlan_add_hybrid_port (ifp, NSM_VLAN_NONE, NSM_VLAN_EGRESS_TAGGED,
                                        PAL_TRUE, PAL_TRUE);
      break;
     case NSM_VLAN_PORT_MODE_TRUNK:
       ret = nsm_vlan_add_trunk_port (ifp, NSM_VLAN_NONE, PAL_TRUE, PAL_TRUE);
      break;
     case NSM_VLAN_PORT_MODE_PN:
      ret = nsm_vlan_add_provider_port (ifp, NSM_VLAN_NONE,
                                        NSM_VLAN_PORT_MODE_PN,
                                        NSM_VLAN_PORT_MODE_PN,
                                        NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                        PAL_TRUE);
      break;
     case NSM_VLAN_PORT_MODE_CN:
      ret = nsm_vlan_add_provider_port (ifp, NSM_VLAN_NONE,
                                        NSM_VLAN_PORT_MODE_CN,
                                        NSM_VLAN_PORT_MODE_CN,
                                        NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                        PAL_TRUE);
      break;
      default:
       ret = -1;
      break;
    }
   
    for (rn = avl_top (vlan_table);
         rn;
         rn = avl_next (rn))
     {
       vlan = rn->info;
       if (!vlan)
        continue;
      
       if (vlan->vid == NSM_VLAN_DEFAULT_VID)
        continue;

       if ((NSM_VLAN_BMP_IS_MEMBER (*excludeBmp, vlan->vid)))
         {
           ret = nsm_vlan_add_common_port (ifp, mode, sub_mode, vlan->vid,
                                           NSM_VLAN_EGRESS_TAGGED, PAL_TRUE,
                                           PAL_TRUE);

          if ( ret < 0 )
           {
             return ret;
           }
         }
     }

   return ret;         
}

int 
nsm_vlan_api_add_all_vid (struct interface *ifp, enum nsm_vlan_port_mode mode,
                          enum nsm_vlan_port_mode sub_mode,
                          struct nsm_vlan_bmp *excludeBmp,
                          enum nsm_vlan_egress_type egress_tagged,
                          bool_t iterate_members, bool_t notify_kernel)

{
  int ret = 0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;  

  ret = nsm_vlan_add_all_except_vid (ifp, mode, sub_mode, excludeBmp, 
                                     egress_tagged, iterate_members, 
                                     notify_kernel);
  if (ret >= 0)
    vlan_port->config = NSM_VLAN_CONFIGURED_ALL;

  return ret;
}
       
#endif /* HAVE_SMI */

#ifdef HAVE_PBB_TE
/* Add the interface to the vlan_list*/ 
int 
nsm_vlan_pbb_te_add_interface (struct nsm_bridge_master *master, char* brname, 
                               nsm_vid_t vid,
                               struct interface *ifp, bool_t is_allowed,
                               bool_t multicast)
{
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan;
  struct nsm_vlan p;
  struct list *port_list;
  struct avl_tree *vlan_table = NULL;
  struct nsm_bridge *bridge = NULL;
  struct listnode *lnode = NULL;
  struct interface *ifp_temp = NULL;
  int found = 0;

  /* Lookup bridge. */
  bridge = nsm_lookup_bridge_by_name(master, brname);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;
#if defined HAVE_I_BEB || defined HAVE_PROVIDER_BRIDGE
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
#endif /* HAVE_I_BEB */
#ifdef HAVE_B_BEB
  if (NSM_BRIDGE_TYPE_BACKBONE (bridge))
    vlan_table = bridge->bvlan_table;
#endif /* HAVE_B_BEB */
  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  NSM_VLAN_SET (&p, vid);

  rn = avl_search (vlan_table, (void *)&p);

 /* list needs is created when vlan is added */
  if (rn)
    {
      vlan = rn->info;

      if (vlan)
        {
          LIST_LOOP (vlan->multicast_forbidden_ports, ifp_temp, lnode)
            if (ifp_temp == ifp)
              {
                found = 1;
                break;
              }
          if (found)
            return -1;

          LIST_LOOP (vlan->multicast_egress_ports, ifp_temp, lnode)
            if (ifp_temp == ifp)
              {
                found = 1;
                break;
              }
          if (found)
            return -1;

          LIST_LOOP (vlan->unicast_forbidden_ports, ifp_temp, lnode)
            if (ifp_temp == ifp)
              {
                found = 1;
                break;
              }
          if (found)
            return -1;

          LIST_LOOP (vlan->unicast_egress_ports, ifp_temp, lnode)
            if (ifp_temp == ifp)
              {
                found = 1;
                break;
              }
          if (found)
            return -1;

          if (is_allowed)
            {
              if (multicast)
                port_list = vlan->multicast_egress_ports;
              else 
                port_list = vlan->unicast_egress_ports;
            }
          else
            {
              if (multicast)
                port_list = vlan->multicast_forbidden_ports;
              else
                port_list = vlan->unicast_forbidden_ports;
            }
          if (!port_list)
            return NSM_PBB_TE_VLAN_PORT_LIST_NOT_INITIALIZED;      

          listnode_add (port_list, ifp);
          /* Add a fdb entry with wildcard MAC*/
          return 0;
        }
    }
  return -1;
}

/* Remove the interface from the vlan_list*/
int
nsm_vlan_pbb_te_delete_interface (struct nsm_bridge_master *master, char* brname, 
                                  nsm_vid_t vid,
                                  struct interface *ifp, bool_t is_allowed,
                                  bool_t multicast)
{
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan p;
  struct list *port_list = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_bridge *bridge = NULL;
  struct listnode *lnode = NULL;
  struct interface *ifp_temp = NULL;
  int found = 0;


  /* Lookup bridge. */
  bridge = nsm_lookup_bridge_by_name(master, brname);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, brname) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

#if defined HAVE_I_BEB || defined HAVE_PROVIDER_BRIDGE
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
#endif /* HAVE_I_BEB */
#ifdef HAVE_B_BEB
  if (NSM_BRIDGE_TYPE_BACKBONE (bridge))
    vlan_table = bridge->bvlan_table;
#endif /* HAVE_B_BEB */

  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  NSM_VLAN_SET (&p, vid);

  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {
      vlan = rn->info;

      if (vlan)
        {
          if (is_allowed)
            {
              if (multicast)
                port_list = vlan->multicast_egress_ports;
              else
                port_list = vlan->unicast_egress_ports;
            }
          else
            {
              if (multicast)
                port_list = vlan->multicast_forbidden_ports;
              else
                port_list = vlan->unicast_forbidden_ports;
            }
          if (!port_list)
            return NSM_PBB_TE_VLAN_PORT_LIST_NOT_INITIALIZED;
          /* check if the interface is the member of this list */
          LIST_LOOP (port_list, ifp_temp, lnode)
            if (ifp_temp == ifp)
              {
                found = 1;
                break;
              }
          if (!found)
            return -1;
          listnode_delete (port_list, ifp);
          /* Remove an existing fdb entry with wildcard MAC*/
          return 0;
        }
    }
  return -1;
}
#endif /* HAVE_PBB_TE */

#endif /* HAVE_VLAN */
