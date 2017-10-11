/* Copyright (C) 2003  All Rights Reserved.

Provider Vlan CLI

This module defines the APIs for the 802.1 AD functionality

*/

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "cli.h"


#include "nsmd.h"
#include "ptree.h"
#include "table.h"
#include "bitmap.h"
#include "linklist.h"
#include "avl_tree.h"
#include "hal_incl.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_pro_vlan.h"
#include "nsm_vlan_cli.h"
#include "nsm_interface.h"
#ifdef HAVE_ELMID
#include "nsm_elmi.h"
#endif /* HAVE_ELMID */
#include "lib/if.h"
#include "nsm/L2/nsm_bridge_cli.h"

#ifdef HAVE_PROVIDER_BRIDGE
enum nsm_uni_service_attr
nsm_map_str_to_service_attr (u_int8_t *proto_str)
{

  if (! pal_strncmp (proto_str, "multiplex", 9))
    return NSM_UNI_MUX;
  else if (! pal_strncmp (proto_str, "bundle", 6))
    return NSM_UNI_BNDL;
  else if (! pal_strncmp (proto_str, "all-to-one", 10))
    return NSM_UNI_AIO_BNDL;

  return NSM_UNI_MUX_BNDL;
}

static int
nsm_cvlan_reg_tab_ent_cmp (void *data1, void *data2)
{

  struct nsm_cvlan_reg_key *key1 = data1;
  struct nsm_cvlan_reg_key *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->cvid > key2->cvid)
    return 1;

  if (key2->cvid > key1->cvid)
    return -1;

  return 0;
}

int
nsm_vlan_trans_tab_ent_cmp (void *data1, void *data2)
{

  struct nsm_vlan_trans_key *key1 = data1;
  struct nsm_vlan_trans_key *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->vid > key2->vid)
    return 1;

  if (key2->vid > key1->vid)
    return -1;

  return 0;
}

int
nsm_vlan_rev_trans_tab_ent_cmp (void *data1, void *data2)
{

  struct nsm_vlan_trans_key *key1 = data1;
  struct nsm_vlan_trans_key *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->trans_vid > key2->trans_vid)
    return 1;

  if (key2->trans_vid > key1->trans_vid)
    return -1;

  return 0;
}

int
nsm_svlan_reg_info_ent_cmp (void *data1, void *data2)
{

  struct nsm_svlan_reg_info *key1 = data1;
  struct nsm_svlan_reg_info *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->svid > key2->svid)
    return 1;

  if (key2->svid > key1->svid)
    return -1;

  return 0;
}

int
nsm_pro_edge_sw_ctx_ent_cmp (void *data1, void *data2)
{
  struct nsm_pro_edge_sw_ctx *key1 = data1;
  struct nsm_pro_edge_sw_ctx *key2 = data2;

  if ((!data1) || (!data2))
    return -1;


  if (pal_mem_cmp (key1, key2, 2 * sizeof (u_int16_t)) > 0)
    return 1;

  if (pal_mem_cmp (key2, key1, 2 * sizeof (u_int16_t)) > 0)
    return -1;

  return 0;
}

static s_int32_t
nsm_bridge_regtag_to_uni_check (struct nsm_cvlan_reg_tab *regtab,
                                u_int16_t cvid, u_int16_t svid,
                                u_int8_t add)
{
  struct nsm_if *zif;
  struct listnode *nn;
  struct interface *ifp;
  struct nsm_bridge_port *br_port;
  struct nsm_svlan_reg_info *svlan_info;

  svlan_info = nsm_reg_tab_svlan_info_lookup (regtab, svid);

  LIST_LOOP (regtab->port_list, ifp, nn)
    {
      zif = ifp->info;

      if (!zif || !zif->switchport)
        continue;

      br_port = zif->switchport;

      switch (br_port->uni_ser_attr)
        {
          case NSM_UNI_MUX_BNDL:
            break;
          case NSM_UNI_MUX:
            if (svlan_info == NULL)
              break;

            if (add == PAL_TRUE)
              {
                if (svlan_info->num_of_cvlans > 1)
                  return PAL_TRUE;

                if (! NSM_VLAN_BMP_IS_MEMBER(svlan_info->cvlanMemberBmp,
                                             cvid))
                  return PAL_TRUE;
              }
            break;
          case NSM_UNI_BNDL:
            if (add == PAL_TRUE
                && svlan_info == NULL
                && (AVL_COUNT (regtab->svlan_tree) >= 1))
              return PAL_TRUE;

            break;
          case NSM_UNI_AIO_BNDL:
            return PAL_TRUE;

            break;
          default:
            return PAL_TRUE;
            break;
        }
   }

  return PAL_FALSE;
}

static s_int32_t
nsm_bridge_uni_to_regtag_check (struct nsm_cvlan_reg_tab *regtab,
                                enum nsm_uni_service_attr service_attr)
{
  struct nsm_svlan_reg_info *svlan_info;
  struct avl_node *node;

  if (regtab == NULL)
    return PAL_FALSE;

  switch (service_attr)
    {
      case NSM_UNI_MUX_BNDL:
        return PAL_FALSE;
        break;

      case NSM_UNI_MUX:
        for (node = avl_top (regtab->svlan_tree); node; node = avl_next (node))
          {
            if (! (svlan_info = AVL_NODE_INFO (node)))
              continue;

            if (svlan_info->num_of_cvlans > 1)
              return PAL_TRUE;
          }

        return PAL_FALSE;
        break;

      case NSM_UNI_BNDL:
        if (AVL_COUNT (regtab->svlan_tree) != 1)
          return PAL_TRUE;
        else
          return PAL_FALSE;
        break;

      case NSM_UNI_AIO_BNDL:
        if (AVL_COUNT (regtab->svlan_tree) != 1)
          return PAL_TRUE;
        else
          {
            for (node = avl_top (regtab->svlan_tree); node; node = avl_next (node))
              {
                if (! (svlan_info = AVL_NODE_INFO (node)))
                  continue;

                if (svlan_info->num_of_cvlans == NSM_VLAN_MAX)
                   return PAL_FALSE;
                else
                   return PAL_TRUE;
              }

            return PAL_TRUE;
          }
        break;

      default:
        return PAL_TRUE;
        break;
    }

  return PAL_TRUE;
}


static s_int32_t
nsm_bridge_max_evc_check (struct nsm_bridge_port *br_port, 
    struct nsm_cvlan_reg_tab *reg_tab)
{
  /* If UNI MAX EVCs are configured, then
   * the max svlan in the registration table applied 
   * to the UNI (CEP) should not exceed 
   * the configired uni max evcs
   */
  if (br_port->uni_max_evcs != PAL_FALSE)
    {
      if (AVL_COUNT (reg_tab->svlan_tree) > br_port->uni_max_evcs)
        return PAL_TRUE;
      else
        return PAL_FALSE;
    }

  return PAL_FALSE;
    

}

struct nsm_svlan_reg_info *
nsm_reg_tab_svlan_info_get (struct nsm_cvlan_reg_tab *reg_tab,
                            u_int16_t svid, u_int16_t cvid)
{
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_svlan_reg_info key;
  struct avl_node *node;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (Registration Table SVLAN Info Get);

  pal_mem_set (&key, 0, sizeof (struct nsm_svlan_reg_info));
  svlan_info = NULL;

  key.svid = svid;

  if (! reg_tab)
    goto EXIT;

  node = avl_search (reg_tab->svlan_tree, &key);

  if (node && AVL_NODE_INFO (node))
    {
      svlan_info = AVL_NODE_INFO (node);

      if (!NSM_VLAN_BMP_IS_MEMBER (svlan_info->cvlanMemberBmp, cvid))
        {
          NSM_VLAN_BMP_SET (svlan_info->cvlanMemberBmp, cvid);
          svlan_info->num_of_cvlans += 1;
        }

      goto EXIT;
    }

  svlan_info = XCALLOC (MTYPE_NSM_VLAN_SVLAN_INFO,
                        sizeof (struct nsm_svlan_reg_info));

  if (svlan_info == NULL)
    goto EXIT;

  svlan_info->port_list = list_new ();

  if (svlan_info->port_list == NULL)
    goto CLEANUP;

  svlan_info->svid = svid;
  NSM_VLAN_BMP_SET (svlan_info->cvlanMemberBmp, cvid);
  svlan_info->num_of_cvlans += 1;

  ret = avl_insert (reg_tab->svlan_tree, svlan_info);

  if (ret < 0)
    goto CLEANUP;

  goto EXIT;

CLEANUP:

  if (svlan_info)
    {
      if (svlan_info->port_list)
        list_delete (svlan_info->port_list);
      XFREE (MTYPE_NSM_VLAN_PRO_EDGE_SWCTX, svlan_info);
    }

  svlan_info = NULL;

EXIT:
  NSM_VLAN_FN_EXIT (svlan_info);

}

struct nsm_svlan_reg_info *
nsm_reg_tab_svlan_info_lookup (struct nsm_cvlan_reg_tab *reg_tab,
                               u_int16_t svid)
{
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_svlan_reg_info key;
  struct avl_node *node;

  NSM_VLAN_FN_ENTER (Registration Table SVLAN Info Lookup);

  pal_mem_set (&key, 0, sizeof (struct nsm_svlan_reg_info));
  svlan_info = NULL;

  key.svid = svid;

  if (! reg_tab)
    goto EXIT;

  node = avl_search (reg_tab->svlan_tree, &key);

  if (node && AVL_NODE_INFO (node))
    {
      svlan_info = AVL_NODE_INFO (node);
      goto EXIT;
    }

EXIT:
  NSM_VLAN_FN_EXIT (svlan_info);
}

void
nsm_reg_tab_svlan_info_release (struct nsm_cvlan_reg_tab *reg_tab,
                                u_int16_t svid, u_int16_t cvid)
{
  struct nsm_pro_edge_info *pro_edge_port;
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_svlan_reg_info key;
  struct avl_node *node;
  struct listnode *nn;

  NSM_VLAN_FN_ENTER (Registration Table SVLAN Info Release);

  pal_mem_set (&key, 0, sizeof (struct nsm_svlan_reg_info));
  svlan_info = NULL;
  pro_edge_port = NULL;

  key.svid = svid;

  if (! reg_tab)
    goto EXIT;

  node = avl_search (reg_tab->svlan_tree, &key);

  if (node && AVL_NODE_INFO (node))
    {
      svlan_info = AVL_NODE_INFO (node);
      NSM_VLAN_BMP_UNSET (svlan_info->cvlanMemberBmp, cvid);
      svlan_info->num_of_cvlans -= 1;

      if (svlan_info->num_of_cvlans == 0)
        {
          avl_remove (reg_tab->svlan_tree, svlan_info);

          LIST_LOOP (svlan_info->port_list, pro_edge_port, nn)
            {
              XFREE (MTYPE_NSM_VLAN_SVLAN_PORT_INFO, pro_edge_port);       
            }

          list_delete (svlan_info->port_list);
          XFREE (MTYPE_NSM_VLAN_SVLAN_INFO, svlan_info);
        }

      goto EXIT;
    }

EXIT:
  NSM_VLAN_FN_EXIT ();

}

struct nsm_pro_edge_info *
nsm_reg_tab_svlan_info_port_add (struct nsm_svlan_reg_info *svlan_info,
                                 struct interface *ifp)
{
  struct nsm_pro_edge_info *pro_edge_port;
  struct listnode *node;

  NSM_VLAN_FN_ENTER (SVLAN Info Port Add);

  pro_edge_port = NULL;
  node = NULL;

  if (! svlan_info || ! ifp)
    goto EXIT;

  LIST_LOOP (svlan_info->port_list, pro_edge_port, node)
    {
      if (pro_edge_port->ifp == ifp)
        break;
    }

  if (node != NULL)
    goto EXIT;

  pro_edge_port = XCALLOC (MTYPE_NSM_VLAN_SVLAN_PORT_INFO,
                           sizeof (struct nsm_pro_edge_info));

  pro_edge_port->ifp = ifp;
  pro_edge_port->untagged_vid = NSM_VLAN_DEFAULT_VID;
  pro_edge_port->pvid = NSM_VLAN_DEFAULT_VID;

  listnode_add (svlan_info->port_list, pro_edge_port);

EXIT:
  NSM_VLAN_FN_EXIT (pro_edge_port);

}

struct nsm_pro_edge_info *
nsm_reg_tab_svlan_info_port_lookup (struct nsm_svlan_reg_info *svlan_info,
                                    struct interface *ifp)
{
  struct nsm_pro_edge_info *pro_edge_port;
  struct listnode *node;

  NSM_VLAN_FN_ENTER (SVLAN Info Port Lookup);

  pro_edge_port = NULL;
  node = NULL;

  if (! svlan_info || ! ifp)
    goto EXIT;

  LIST_LOOP (svlan_info->port_list, pro_edge_port, node)
    {
      if (pro_edge_port->ifp == ifp)
        break;
    }

  if (node == NULL)
    {
      node = NULL;
      pro_edge_port = NULL;
      goto EXIT;
    }

EXIT:
  NSM_VLAN_FN_EXIT (pro_edge_port);

}

void
nsm_reg_tab_svlan_info_port_delete  (struct nsm_svlan_reg_info *svlan_info,
                                     struct interface *ifp)
{
  struct nsm_pro_edge_info *pro_edge_port;
  struct listnode *node;

  NSM_VLAN_FN_ENTER (SVLAN Info Port Delete);

  pro_edge_port = NULL;
  node = NULL;

  if (! svlan_info || ! ifp)
    goto EXIT;

  LIST_LOOP (svlan_info->port_list, pro_edge_port, node)
    {
      if (pro_edge_port->ifp == ifp)
        break;
    }

  if (node == NULL)
    goto EXIT;

  listnode_delete (svlan_info->port_list, pro_edge_port);

  XFREE (MTYPE_NSM_VLAN_SVLAN_PORT_INFO, pro_edge_port);

EXIT:
  NSM_VLAN_FN_EXIT ();

}


int
nsm_svlan_set_cos_preservation (struct nsm_bridge_master *master,
                                char *bridge_name, u_int16_t vid,
                                u_int8_t preserve_ce_cos)
{
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_cvlan_reg_tab *regtab;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct avl_tree *vlan_table;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct interface *ifp;
  struct nsm_vlan *vlan;
  struct avl_node *rn;
  struct listnode *nn;
  struct nsm_if *zif;
  struct nsm_vlan p;
  u_int16_t cvid;

  bridge = nsm_lookup_bridge_by_name(master, bridge_name);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, bridge_name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  vlan_table = bridge->svlan_table;

  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  NSM_VLAN_SET (&p, vid);

  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {
      if (! rn->info)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      vlan = rn->info;
      vlan->preserve_ce_cos = preserve_ce_cos;
    }

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

      if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE
          || ((regtab = vlan_port->regtab) == NULL))
        continue;

      if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                                                       vid)) == NULL)
        continue;

      svlan_info->preserve_ce_cos = preserve_ce_cos;

      NSM_VLAN_SET_BMP_ITER_BEGIN (svlan_info->cvlanMemberBmp, cvid)
        {
          hal_l2_qos_set_cos_preserve (ifp->ifindex, cvid, preserve_ce_cos);
        }
      NSM_VLAN_SET_BMP_ITER_END (svlan_info->cvlanMemberBmp, cvid);
    }

  LIST_LOOP (bridge->cvlan_reg_tab_list, regtab, nn)
    {
      if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                                                       vid)) == NULL)
        continue;

      svlan_info->preserve_ce_cos = preserve_ce_cos;
    }

  return NSM_L2_ERR_NONE;
}

struct nsm_cvlan_reg_tab *
nsm_cvlan_reg_tab_lookup (struct nsm_bridge *bridge,
                          char *cvlan_reg_tab_name)
{
  struct nsm_cvlan_reg_tab *regtab;
  struct listnode *nn;

  NSM_VLAN_FN_ENTER (CVLAN Reg Tab Lookup);

  nn = NULL;
  regtab = NULL;

  if (! bridge
      || ! cvlan_reg_tab_name)
    goto EXIT;

  LIST_LOOP (bridge->cvlan_reg_tab_list, regtab, nn)
    {
      if (pal_strcmp (regtab->name, cvlan_reg_tab_name) == 0)
        break;
    }

  if (! nn)
    regtab = NULL;

EXIT:

  NSM_VLAN_FN_EXIT (regtab);
}

/* Notify clients of Switching Context Addition. */
static inline void
_nsm_pro_edge_notify_add_swctx (struct nsm_bridge *bridge,
                                struct nsm_pro_edge_sw_ctx *ctx)
{
  struct nsm_vlan_listener *vlanlnr;
  struct listnode *ln;

  LIST_LOOP (bridge->vlan_listener_list, vlanlnr, ln)
    if (vlanlnr->add_pro_edge_swctx_to_bridge)
      (vlanlnr->add_pro_edge_swctx_to_bridge)(bridge->master,
                                              bridge->name, ctx->cvid,
                                              ctx->svid);
}

/* Notify clients of Switching Context Deletion. */
static inline void
_nsm_pro_edge_notify_delete_swctx (struct nsm_bridge *bridge,
                                   struct nsm_pro_edge_sw_ctx *ctx)
{
  struct nsm_vlan_listener *vlanlnr;
  struct listnode *ln;

  LIST_LOOP (bridge->vlan_listener_list, vlanlnr, ln)
    if (vlanlnr->delete_pro_edge_swctx_from_bridge)
      (vlanlnr->delete_pro_edge_swctx_from_bridge)
                                (bridge->master,
                                 bridge->name, ctx->cvid,
                                 ctx->svid);

}

/* Notify clients of Switching Context Port Addition. */
static inline void
_nsm_pro_edge_notify_add_port_to_swctx (struct nsm_bridge *bridge,
                                        struct interface *ifp,
                                        u_int16_t cvid, u_int16_t svid)
{
  struct nsm_vlan_listener *vlanlnr;
  struct nsm_pro_edge_sw_ctx *ctx;
  struct listnode *ln;

  ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

  if (ctx)
    listnode_add (ctx->port_list, ifp);

  LIST_LOOP (bridge->vlan_listener_list, vlanlnr, ln)
    if (vlanlnr->add_port_to_swctx_func)
      (vlanlnr->add_port_to_swctx_func) (bridge->master,
                                         bridge->name, ifp,
                                         cvid, svid);
}

/* Notify clients of Switching Context Port Deletion. */
static inline void
_nsm_pro_edge_notify_del_port_from_swctx (struct nsm_bridge *bridge,
                                          struct interface *ifp,
                                          u_int16_t cvid, u_int16_t svid)
{
  struct nsm_vlan_listener *vlanlnr;
  struct nsm_pro_edge_sw_ctx *ctx;
  struct listnode *ln;

  ctx = nsm_pro_edge_swctx_lookup (bridge, cvid, svid);

  nsm_bridge_delete_port_fdb_by_vid_svid (bridge, ifp->info, cvid, svid);

  LIST_LOOP (bridge->vlan_listener_list, vlanlnr, ln)
    if (vlanlnr->del_port_from_swctx_func)
      (vlanlnr->del_port_from_swctx_func) (bridge->master,
                                           bridge->name, ifp,
                                           cvid, svid);
  if (ctx)
    listnode_delete (ctx->port_list, ifp);

}


void
nsm_pro_edge_add_port_to_swctx (struct nsm_bridge *bridge,
                                struct interface *ifp,
                                struct nsm_vlan *svlan)
{
  u_int16_t cvid;

  NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
   {
     _nsm_pro_edge_notify_add_port_to_swctx (bridge,
                                             ifp, cvid,
                                             svlan->vid);
   }
  NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

}

void
nsm_pro_edge_del_port_from_swctx (struct nsm_bridge *bridge,
                                  struct interface *ifp,
                                  struct nsm_vlan *svlan)
{
  u_int16_t cvid;


  NSM_VLAN_SET_BMP_ITER_BEGIN ((*svlan->cvlanMemberBmp), cvid)
   {
     _nsm_pro_edge_notify_del_port_from_swctx (bridge,
                                               ifp, cvid,
                                               svlan->vid);
   }
  NSM_VLAN_SET_BMP_ITER_END ((*svlan->cvlanMemberBmp), cvid);

}

s_int32_t
nsm_cvlan_reg_tab_if_apply (struct interface *ifp,
                            char *cvlan_reg_tab_name)
{
  enum hal_vlan_egress_type egress_tagged;
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_cvlan_reg_tab *regtab;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_cvlan_reg_key *key;
  struct nsm_msg_vlan_port msg;
  struct avl_node *node;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (Apply CVLAN Registration Table);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
  svlan_info = NULL;

  if (! zif->bridge
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (br_port->vlan_port.regtab)
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_EXISTS;
      goto EXIT;
    }

  if (br_port->vlan_port.mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  regtab = nsm_cvlan_reg_tab_lookup (zif->bridge, cvlan_reg_tab_name);

  if (! regtab)
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND;
      goto EXIT;
    }
  
  /*Check if the SVLANs in the Registration Table
   * exceeds the configured MAX EVCs
   */
  if (nsm_bridge_uni_to_regtag_check (regtab, br_port->uni_ser_attr)
                                      == PAL_TRUE)
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_MAP_ERR;
      goto EXIT;
    }

  if (nsm_bridge_max_evc_check (br_port, regtab))
    {
      return NSM_ERR_CROSSED_MAX_EVC;
      
    }

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
     {
       if (! (key = AVL_NODE_INFO (node)))
         continue;

       if (!NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, key->cvid))
         {
           ret = NSM_VLAN_ERR_VLAN_NOT_IN_PORT;
           goto EXIT;
         }
    }

  msg.ifindex = ifp->ifindex;
  msg.num = 1;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
    {
      if (! (key = AVL_NODE_INFO (node)))
        continue;

      if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                                                       key->svid)) == NULL)
        continue;

      hal_l2_qos_set_cos_preserve (ifp->ifindex, key->cvid,
                                   svlan_info->preserve_ce_cos);

      if (!NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, key->svid))
        {
          nsm_reg_tab_svlan_info_port_add (svlan_info, ifp);

          hal_vlan_add_pro_edge_port (zif->bridge->name, ifp->ifindex,
                                      key->svid);

          NSM_VLAN_BMP_SET(vlan_port->svlanMemberBmp, key->svid);
          msg.vid_info[0] = key->svid;
          nsm_vlan_send_port_map (zif->bridge, &msg, NSM_MSG_SVLAN_ADD_CE_PORT);
        }

      ret = hal_vlan_create_cvlan_registration_entry (zif->bridge->name,
                                                      zif->ifp->ifindex,
                                                      key->cvid,
                                                      key->svid);

      if (ret != HAL_SUCCESS)
        {
          ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_HAL_ERR;
          goto CLEANUP;
        }

      egress_tagged = NSM_VLAN_BMP_IS_MEMBER (br_port->vlan_port.egresstaggedBmp,
                                              key->cvid) ?
                      HAL_VLAN_EGRESS_TAGGED : HAL_VLAN_EGRESS_UNTAGGED;

      ret = hal_vlan_add_cvid_to_port (zif->bridge->name, zif->ifp->ifindex,
                                       key->cvid, key->svid, egress_tagged);

      if (ret != HAL_SUCCESS)
        {
          ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_HAL_ERR;
          goto CLEANUP;
        }

      _nsm_pro_edge_notify_add_port_to_swctx (zif->bridge, zif->ifp,
                                              key->cvid, key->svid);
#ifdef HAVE_ELMID         
         nsm_elmi_send_evc_add (zif->bridge, ifp, key->svid, svlan_info); 
#endif /* HAVEE_ELMID */
    }

  br_port->vlan_port.regtab = regtab;

  listnode_add (regtab->port_list, ifp);

  goto EXIT;

CLEANUP:

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
    {
      if (! (key = AVL_NODE_INFO (node)))
        continue;

      if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, key->svid))
        {
          if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                                                           key->svid)))
            {
              hal_vlan_del_pro_edge_port (zif->bridge->name, ifp->ifindex,
                                          key->svid);

              nsm_reg_tab_svlan_info_port_delete (svlan_info, ifp);
            }

          NSM_VLAN_BMP_UNSET(vlan_port->svlanMemberBmp, key->svid);
          msg.vid_info[0] = key->svid;
          nsm_vlan_send_port_map (zif->bridge, &msg, NSM_MSG_SVLAN_DELETE_CE_PORT);
        }

      hal_vlan_delete_cvlan_registration_entry (zif->bridge->name,
                                                zif->ifp->ifindex,
                                                key->cvid,
                                                key->svid);

      hal_vlan_delete_cvid_from_port (zif->bridge->name, zif->ifp->ifindex,
                                      key->cvid, key->svid);
    }

EXIT:

  if (msg.vid_info)
    XFREE (MTYPE_TMP, msg.vid_info);

  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_cvlan_reg_tab_if_delete (struct interface *ifp)
{
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_bridge_port *br_port;
  struct nsm_cvlan_reg_tab *regtab;
  struct nsm_vlan_port *vlan_port;
  struct nsm_cvlan_reg_key *key = NULL;
  struct nsm_msg_vlan_port msg;
  struct avl_node *node;
  struct nsm_if *zif;
  u_int16_t svid;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (Delete CVLAN Registration Table);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
  svlan_info = NULL;

  if (! zif->bridge
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! (regtab = br_port->vlan_port.regtab))
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND;
      goto EXIT;
    }

  if (br_port->vlan_port.mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
    {
      if (! (key = AVL_NODE_INFO (node)))
        continue;

      hal_vlan_delete_cvlan_registration_entry (zif->bridge->name,
                                                zif->ifp->ifindex,
                                                key->cvid,
                                                key->svid);

      _nsm_pro_edge_notify_del_port_from_swctx (zif->bridge, zif->ifp,
                                                key->cvid, key->svid);
    }

  for (node = avl_top (regtab->svlan_tree); node; node = avl_next (node))
    {
      if (! (svlan_info = AVL_NODE_INFO (node)))
        continue;

      hal_vlan_del_pro_edge_port (zif->bridge->name, ifp->ifindex,
                                  svlan_info->svid);

      nsm_reg_tab_svlan_info_port_delete (svlan_info, ifp);
    }

  msg.ifindex = ifp->ifindex;
  msg.num = 1;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->svlanMemberBmp, svid)
    {
      NSM_VLAN_BMP_UNSET (vlan_port->svlanMemberBmp, svid);
      msg.vid_info[0] = svid;
      nsm_vlan_send_port_map (zif->bridge, &msg,
                              NSM_MSG_SVLAN_DELETE_CE_PORT);

#ifdef HAVE_ELMID
      nsm_server_evc_delete (ifp, svid);
#endif /* HAVE_ELMID */

    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->svlanMemberBmp, svid);

  NSM_VLAN_BMP_CLEAR (vlan_port->svlanMemberBmp);

  br_port->vlan_port.regtab = NULL;

  listnode_delete (regtab->port_list, ifp);

EXIT:

  if (msg.vid_info)
    XFREE (MTYPE_TMP, msg.vid_info);

  NSM_VLAN_FN_EXIT (ret);
}

struct nsm_pro_edge_sw_ctx *
nsm_pro_edge_swctx_lookup (struct nsm_bridge *bridge, u_int16_t cvid,
                           u_int16_t svid)
{
  struct avl_node *node;
  struct nsm_pro_edge_sw_ctx key;
  struct nsm_pro_edge_sw_ctx *ctx;

  NSM_VLAN_FN_ENTER (Provider Edge Switching Context Lookup);

  key.svid = svid;
  key.cvid = cvid;
  ctx = NULL;

  if (! bridge || ! bridge->pro_edge_swctx_table)
    goto EXIT;

  node = avl_search (bridge->pro_edge_swctx_table, &key);

  if (node && AVL_NODE_INFO (node))
    {
      ctx = AVL_NODE_INFO (node);
      goto EXIT;
    }

EXIT:
  NSM_VLAN_FN_EXIT (ctx);

}

struct nsm_pro_edge_sw_ctx *
nsm_pro_edge_swctx_get (struct nsm_bridge *bridge, struct nsm_vlan *cvlan,
                        struct nsm_vlan *svlan)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *node;
  struct avl_node *avl_port;
  struct nsm_pro_edge_sw_ctx key;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_pro_edge_sw_ctx *ctx;

  NSM_VLAN_FN_ENTER (Provider Edge Switching Context Get);

  key.svid = svlan->vid;
  key.cvid = cvlan->vid;
  ctx = NULL;

  if (! bridge || ! bridge->pro_edge_swctx_table)
    goto EXIT;

  node = avl_search (bridge->pro_edge_swctx_table, &key);

  if (node && AVL_NODE_INFO (node))
    {
      ctx = AVL_NODE_INFO (node);
      ctx->refcnt += 1;
      goto EXIT;
    }

  ctx = XCALLOC (MTYPE_NSM_VLAN_PRO_EDGE_SWCTX,
                 sizeof (struct nsm_pro_edge_sw_ctx));
  
  if (!ctx)
    goto EXIT;
  
  ctx->svid = svlan->vid;
  ctx->cvid = cvlan->vid;
  ctx->refcnt += 1;
  ctx->port_list = list_new ();
  ctx->static_fdb_list = ptree_init (ETHER_MAX_BIT_LEN);

  ret = avl_insert (bridge->pro_edge_swctx_table, ctx);

  if (ret < 0)
    goto CLEANUP;

  if ((ret = nsm_vlan_if_set (bridge->master->nm, key.cvid,
                              key.svid, 0, bridge->name) < 0))
    goto CLEANUP;

  _nsm_pro_edge_notify_add_swctx (bridge, ctx);

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

      if ((vlan_port->mode == NSM_VLAN_PORT_MODE_PN
           || vlan_port->mode == NSM_VLAN_PORT_MODE_CN)
          && NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp, svlan->vid)
          && NSM_VLAN_BMP_IS_MEMBER(vlan_port->dynamicMemberBmp, svlan->vid))
        _nsm_pro_edge_notify_add_port_to_swctx (bridge, ifp,
                                                key.cvid, key.svid);

    }

  if (svlan->cvlanMemberBmp)
    NSM_VLAN_BMP_SET ((*svlan->cvlanMemberBmp), cvlan->vid);

  goto EXIT;

CLEANUP:

  if (ctx)
    XFREE (MTYPE_NSM_VLAN_PRO_EDGE_SWCTX, ctx);

  ctx = NULL;

EXIT:
  NSM_VLAN_FN_EXIT (ctx);
}

void
nsm_pro_edge_swctx_release (struct nsm_bridge *bridge,  struct nsm_vlan *cvlan,
                            struct nsm_vlan *svlan)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *node;
  struct avl_node *avl_port;
  struct nsm_pro_edge_sw_ctx key;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_pro_edge_sw_ctx *ctx;

  NSM_VLAN_FN_ENTER (Provider Edge Switching Context Release);

  key.svid = svlan->vid;
  key.cvid = cvlan->vid;

  if (! bridge || ! bridge->pro_edge_swctx_table)
    goto EXIT;

  node = avl_search (bridge->pro_edge_swctx_table, &key);

  if (! node
      || ! AVL_NODE_INFO (node))
    goto EXIT;

  ctx = AVL_NODE_INFO (node);
 
  ctx->refcnt -= 1; 

  if (ctx->refcnt > 0)
    goto EXIT;

  _nsm_pro_edge_notify_delete_swctx (bridge, ctx);

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

      if ((vlan_port->mode == NSM_VLAN_PORT_MODE_PN
           || vlan_port->mode == NSM_VLAN_PORT_MODE_CN)
          && NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp, key.svid)
          && NSM_VLAN_BMP_IS_MEMBER(vlan_port->dynamicMemberBmp, key.svid))
        _nsm_pro_edge_notify_del_port_from_swctx (bridge, ifp,
                                                  key.cvid, key.svid);

    }

  avl_remove (bridge->pro_edge_swctx_table, ctx);
  nsm_bridge_free_static_list (ctx->static_fdb_list);
  ptree_finish(ctx->static_fdb_list);

  if (svlan->cvlanMemberBmp)
    NSM_VLAN_BMP_UNSET ((*svlan->cvlanMemberBmp), cvlan->vid);

  nsm_vlan_if_unset (bridge->master->nm, key.cvid, key.svid,
                     0, bridge->name);

  list_delete (ctx->port_list);
  XFREE (MTYPE_NSM_VLAN_PRO_EDGE_SWCTX, ctx);

EXIT:
  NSM_VLAN_FN_EXIT ();
}

struct nsm_cvlan_reg_tab *
nsm_cvlan_reg_tab_create (char *cvlan_reg_tab_name)
{
  struct nsm_cvlan_reg_tab *regtab;

  NSM_VLAN_FN_ENTER (CVLAN Registration Table Create);

  regtab = XCALLOC (MTYPE_NSM_CVLAN_REG_TAB, sizeof (struct nsm_cvlan_reg_tab));

  if (! regtab)
    goto EXIT;

  pal_strncpy (regtab->name, cvlan_reg_tab_name, NSM_CVLAN_REG_TAB_NAME_MAX + 1);

  if (avl_create (&regtab->reg_tab, 0, nsm_cvlan_reg_tab_ent_cmp) < 0)
    goto CLEANUP;

  if (avl_create (&regtab->svlan_tree, 0, nsm_svlan_reg_info_ent_cmp) < 0)
    goto CLEANUP;

  regtab->port_list = list_new ();

  if (! regtab->port_list)
    goto CLEANUP;

  regtab->bridge = NULL;
  goto EXIT;

CLEANUP:

  if (regtab)
    {
       if (regtab->reg_tab)
         avl_tree_free (&regtab->reg_tab, NULL);

       if (regtab->svlan_tree)
         avl_tree_free (&regtab->svlan_tree, NULL);

       if (regtab->port_list)
         list_delete (regtab->port_list);

       XFREE (MTYPE_NSM_CVLAN_REG_TAB, regtab);
       regtab = NULL;
    }

EXIT:

  NSM_VLAN_FN_EXIT (regtab);

}

struct nsm_cvlan_reg_tab *
nsm_cvlan_reg_tab_get (struct nsm_bridge *bridge,
                       char *cvlan_reg_tab_name)
{
  struct nsm_cvlan_reg_tab *regtab;

  NSM_VLAN_FN_ENTER (CVLAN Registration Table Get);

  regtab = NULL;

  if (! bridge
      || ! cvlan_reg_tab_name)
    goto EXIT;

  regtab = nsm_cvlan_reg_tab_lookup (bridge, cvlan_reg_tab_name);

  if (regtab)
    goto EXIT;

  regtab = nsm_cvlan_reg_tab_create (cvlan_reg_tab_name);

  if (! regtab)
     goto EXIT;

  regtab->bridge = bridge;
  listnode_add (bridge->cvlan_reg_tab_list, regtab);

EXIT:

  NSM_VLAN_FN_EXIT (regtab);
}

void
nsm_cvlan_reg_tab_delete (struct nsm_bridge *bridge,
                          char *cvlan_reg_tab_name)
{
  struct nsm_pro_edge_info *pro_edge_port;
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_cvlan_reg_tab *regtab;
  struct nsm_cvlan_reg_key *tmpkey;
  struct listnode *next_node;
  struct avl_node *node;
  struct interface *ifp;
  struct listnode *nn;

  NSM_VLAN_FN_ENTER (CVLAN Registration Table Delete);

  regtab = nsm_cvlan_reg_tab_lookup (bridge, cvlan_reg_tab_name);

  if (! regtab)
    goto EXIT;

  LIST_LOOP_DEL (regtab->port_list, ifp, nn, next_node)
    nsm_cvlan_reg_tab_if_delete (ifp);

  listnode_delete (bridge->cvlan_reg_tab_list, regtab);

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
     {
       if (! (tmpkey = AVL_NODE_INFO (node)))
         continue;

       XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);
     }

  for (node = avl_top (regtab->svlan_tree) ; node; node = avl_next (node))
    {
      if (! (svlan_info = AVL_NODE_INFO (node)))
         continue;

      LIST_LOOP (svlan_info->port_list, pro_edge_port, nn)
        XFREE (MTYPE_NSM_VLAN_SVLAN_PORT_INFO, pro_edge_port);

      list_delete (svlan_info->port_list);
      XFREE (MTYPE_NSM_VLAN_SVLAN_INFO, svlan_info);
    }
    

  avl_tree_free (&regtab->reg_tab, NULL);
  avl_tree_free (&regtab->svlan_tree, NULL);
  list_free (regtab->port_list);

  XFREE (MTYPE_NSM_CVLAN_REG_TAB, regtab);

EXIT:

  NSM_VLAN_FN_EXIT ();

}

static s_int32_t
nsm_cvlan_reg_tab_handle_entry_delete (u_int16_t svid,
                                       struct interface *ifp)
{
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_cvlan_reg_tab *regtab;
  struct nsm_cvlan_reg_key *tmpkey;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_msg_vlan_port msg;
  struct nsm_bridge *bridge;
  struct nsm_vlan *svlan;
  struct avl_node *node;
  struct avl_node *rn;
  struct nsm_vlan nvlan;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (CVLAN Registration Table Handle Delete);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
  svlan_info = NULL;

  if (! zif
      || ! (bridge = zif->bridge)
      || ! (br_port = zif->switchport))
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  NSM_VLAN_SET(&nvlan, svid);

  rn = avl_search (bridge->vlan_table, (void *)&nvlan);

  if (! rn || !rn->info )
    {
      ret = NSM_VLAN_ERR_SVLAN_NOT_FOUND;
      goto EXIT;
    }

  svlan = rn->info;

  vlan_port = &br_port->vlan_port;

  if (! (regtab = vlan_port->regtab))
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND;
      goto EXIT;
    }

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
     {
       if (! (tmpkey = AVL_NODE_INFO (node)))
         continue;

       if (tmpkey->svid == svid)
         goto EXIT;
     }

  msg.ifindex = ifp->ifindex;
  msg.num = 1;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

  /* There is no C VLAN corresponding to the S VLAN. So delete it */
  if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, svid))
    {
      if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                                                       svid)))
        {
          hal_vlan_del_pro_edge_port (zif->bridge->name, ifp->ifindex,
                                      svid);

          nsm_reg_tab_svlan_info_port_delete (svlan_info, ifp);
        }

      vlan_port->uni_default_evc = NSM_DEFAULT_VAL;

      NSM_VLAN_BMP_UNSET(vlan_port->svlanMemberBmp, svid);
      msg.vid_info[0] = svid;

      nsm_vlan_send_port_map (zif->bridge, &msg,
                              NSM_MSG_SVLAN_DELETE_CE_PORT);
#ifdef HAVE_ELMID
      nsm_server_evc_delete (ifp, svid);
#endif /* HAVE_ELMID */
    }

EXIT:

  if (msg.vid_info)
    XFREE (MTYPE_TMP, msg.vid_info);
  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_cvlan_reg_tab_check_membership (struct nsm_cvlan_reg_tab *regtab,
                                    u_int16_t cvid)
{
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct interface *ifp;
  struct listnode *nn;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (Check CVLAN Membership);

  ret = PAL_TRUE;

  LIST_LOOP (regtab->port_list, ifp, nn)
    {
      zif = ifp->info;

      if (!zif || !zif->switchport)
        continue;

      br_port = zif->switchport;

      vlan_port = &br_port->vlan_port;

      if (! NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, cvid))
        {
          ret = PAL_FALSE;
          goto EXIT;
        }
    }

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_vlan_cvlan_regis_exist (struct nsm_bridge *bridge, struct nsm_vlan *vlan)
{
  struct nsm_cvlan_reg_tab *regtab;
  struct nsm_cvlan_reg_key *tmpkey;
  struct nsm_cvlan_reg_key key;
  struct avl_node *node;
  struct listnode *nn;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (Registration Entry Check);

  tmpkey = NULL;
  ret = PAL_FALSE;

  if (! bridge
      || ! vlan)
    goto EXIT;

  LIST_LOOP (bridge->cvlan_reg_tab_list, regtab, nn)
    {
      if (vlan->type & NSM_VLAN_CVLAN)
        {
          key.cvid = vlan->vid;
          node = avl_search (regtab->reg_tab, (u_char *) &key);

          if (node && (tmpkey = AVL_NODE_INFO (node)))
            {
              ret = PAL_TRUE;
              goto EXIT;
            }
        }
      else
        {
          for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
             {
               if (! (tmpkey = AVL_NODE_INFO (node)))
                 continue;

               if (tmpkey->svid != vlan->vid)
                 {
                   ret = PAL_TRUE;
                   goto EXIT;
                 }
             }
        }
    }

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

struct nsm_cvlan_reg_key *
nsm_vlan_port_cvlan_regis_exist (struct interface *ifp,
                                 u_int16_t cvid)
{
  struct nsm_cvlan_reg_key *tmpkey;
  struct nsm_cvlan_reg_key *retkey;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_cvlan_reg_key key;
  struct avl_node *node;
  struct nsm_if *zif;
  u_int16_t vid;

  NSM_VLAN_FN_ENTER (Registration Entry Check);

  retkey = NULL;
  zif = (struct nsm_if *)ifp->info;

  if  (! zif || ! zif->switchport)
    goto EXIT;

  br_port = zif->switchport;

  vlan_port = &br_port->vlan_port;

  if (! vlan_port->regtab)
    goto EXIT;

  if (cvid == NSM_VLAN_ALL)
    {
      NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->staticMemberBmp, vid)
        {
          key.cvid = vid;

          node = avl_search (vlan_port->regtab->reg_tab, (u_char *) &key);

          if (node && (tmpkey = AVL_NODE_INFO (node)))
            {
              retkey = tmpkey;
              goto EXIT;
            }
        }
      NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, vid);
    }
  else
    {
      key.cvid = cvid;

      node = avl_search (vlan_port->regtab->reg_tab, (u_char *) &key);

      if (node && (tmpkey = AVL_NODE_INFO (node)))
        {
          retkey = tmpkey;
          goto EXIT;
        }
    }

EXIT:

  NSM_VLAN_FN_EXIT (retkey);
}

s_int32_t
nsm_cvlan_reg_tab_entry_add (struct nsm_cvlan_reg_tab *regtab, u_int16_t cvid,
                             u_int16_t svid)
{
  enum hal_vlan_egress_type egress_tagged;
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_cvlan_reg_key *tmpkey;
  struct nsm_pro_edge_sw_ctx *ctx;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_msg_vlan_port msg;
  struct nsm_cvlan_reg_key key;
  struct nsm_bridge *bridge;
  struct nsm_vlan *cvlan;
  struct nsm_vlan *svlan;
  struct avl_node *node;
  struct nsm_vlan nvlan;
  struct interface *ifp;
  struct avl_node *rn;
  struct listnode *nn;
  struct nsm_if *zif;
  s_int32_t ret;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
#endif /* HAVE_ELMID */

  NSM_VLAN_FN_ENTER (CVLAN Registration Table Entry Add);

  key.cvid = cvid;
  key.svid = svid;
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
  ctx = NULL;
  svlan_info = NULL;
  ret = NSM_L2_ERR_NONE;

  if (!nsm_cvlan_reg_tab_check_membership (regtab, cvid))
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_PORT_NOT_FOUND;
      goto EXIT;
    }

  if (nsm_bridge_regtag_to_uni_check (regtab, cvid, svid, PAL_TRUE)
                       == PAL_TRUE)
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_MAP_ERR;
      goto EXIT;
    }

  node = avl_search (regtab->reg_tab, (u_char *) &key);

  if (node && (tmpkey = AVL_NODE_INFO (node)))
   {
     if (tmpkey->svid == svid)
       goto EXIT;
     else
       {
         ret = NSM_PRO_VLAN_ERR_CVLAN_MAP_EXISTS;
         goto EXIT;
       }
   }

  bridge = regtab->bridge;

  if (! bridge)
    {
      ret = NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
      goto EXIT;
    }

  if (bridge->provider_edge == PAL_FALSE)
    {
      ret = NSM_VLAN_ERR_NOT_EDGE_BRIDGE;
      goto EXIT;
    }

  NSM_VLAN_SET(&nvlan, cvid);

  rn = avl_search (bridge->vlan_table, (void *)&nvlan);

  if (! rn || !(cvlan = rn->info))
    {
      ret = NSM_VLAN_ERR_VLAN_NOT_FOUND;
      goto EXIT;
    }

  NSM_VLAN_SET(&nvlan, svid);

  rn = avl_search (bridge->svlan_table, (void *)&nvlan);

  if (! rn || !(svlan = rn->info))
    {
      ret = NSM_VLAN_ERR_SVLAN_NOT_FOUND;
      goto EXIT;
    }

  tmpkey = XCALLOC (MTYPE_NSM_VLAN_DBL_VID_KEY,
                    sizeof (struct nsm_cvlan_reg_key));

  if (! tmpkey)
    {
      ret = NSM_L2_ERR_MEM;
      goto EXIT;
    }

  tmpkey->cvid = cvid;
  tmpkey->svid = svid;

  ret = avl_insert (regtab->reg_tab, (void *) tmpkey);

  if (ret < 0)
    {
      ret = NSM_L2_ERR_MEM;
      goto CLEANUP;
    }

  msg.num = 1;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);
  msg.vid_info[0] = key.svid;

  ret = hal_vlan_create_cvlan (regtab->bridge->name, cvid, svid);

  if (ret < 0)
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_HAL_ERR;
      goto CLEANUP;
    }

  ctx = nsm_pro_edge_swctx_get (bridge, cvlan, svlan);

  if (ctx == NULL)
    {
      ret = NSM_L2_ERR_MEM;
      goto CLEANUP;
    }

  svlan_info = nsm_reg_tab_svlan_info_get (regtab, svid, cvid);

  if (svlan_info == NULL)
    {
      ret = NSM_L2_ERR_MEM;
      goto CLEANUP;
    }

  svlan_info->preserve_ce_cos = svlan->preserve_ce_cos;

  LIST_LOOP (regtab->port_list, ifp, nn)
    {
      zif = ifp->info;

      if (!zif || !zif->switchport)
        continue;

      br_port = zif->switchport;

      vlan_port = &br_port->vlan_port;

      ret = hal_vlan_create_cvlan_registration_entry (zif->bridge->name,
                                                      zif->ifp->ifindex,
                                                      key.cvid,
                                                      key.svid);

      if (ret != HAL_SUCCESS)
        {
          ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_HAL_ERR;
          goto CLEANUP;
        }

      egress_tagged = NSM_VLAN_BMP_IS_MEMBER (br_port->vlan_port.egresstaggedBmp,
                                              key.cvid) ?
                      HAL_VLAN_EGRESS_TAGGED : HAL_VLAN_EGRESS_UNTAGGED;

      ret = hal_vlan_add_cvid_to_port (zif->bridge->name, zif->ifp->ifindex,
                                       key.cvid, key.svid, egress_tagged);

      if (ret != HAL_SUCCESS)
        {
          ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_HAL_ERR;
          goto CLEANUP;
        }

      msg.ifindex = ifp->ifindex;

      if (!NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, key.svid))
        {
          nsm_reg_tab_svlan_info_port_add (svlan_info, ifp);

          hal_vlan_add_pro_edge_port (zif->bridge->name, ifp->ifindex,
                                      key.svid);

          NSM_VLAN_BMP_SET(vlan_port->svlanMemberBmp, key.svid);
          nsm_vlan_send_port_map (zif->bridge, &msg,
                                  NSM_MSG_SVLAN_ADD_CE_PORT);
        }

      _nsm_pro_edge_notify_add_port_to_swctx (zif->bridge, zif->ifp,
                                              key.cvid, key.svid);

      hal_l2_qos_set_cos_preserve (ifp->ifindex, cvid, svlan->preserve_ce_cos);

#ifdef HAVE_ELMID         
      cindex = 0;
      NSM_SET_CTYPE(cindex, NSM_ELMI_CTYPE_EVC_CVLAN_BITMAPS);
      nsm_elmi_send_evc_add (zif->bridge, zif->ifp, key.svid, svlan_info);
#endif /* HAVEE_ELMID */
    }

  goto EXIT;

CLEANUP:
   if (ctx)
     nsm_pro_edge_swctx_release (regtab->bridge, cvlan, svlan);

   if (tmpkey)
     {
       hal_vlan_delete_cvlan (regtab->bridge->name, cvid, svid);

       LIST_LOOP (regtab->port_list, ifp, nn)
         {
           zif = ifp->info;

           if (!zif)
             continue;

           hal_vlan_delete_cvlan_registration_entry (zif->bridge->name,
                                                     zif->ifp->ifindex,
                                                     key.cvid,
                                                     key.svid);

           hal_vlan_delete_cvid_from_port (zif->bridge->name,
                                           zif->ifp->ifindex,
                                           key.cvid, key.svid);

           nsm_cvlan_reg_tab_handle_entry_delete (svid, ifp);
         }

       avl_remove (regtab->reg_tab, (void *) tmpkey);
       XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);
     }

EXIT:

  if (msg.vid_info)
    XFREE (MTYPE_TMP, msg.vid_info);
  NSM_VLAN_FN_EXIT (ret);

}

s_int32_t
nsm_cvlan_reg_tab_entry_delete (struct nsm_cvlan_reg_tab *regtab,
                                u_int16_t cvid)
{
  struct nsm_cvlan_reg_key *tmpkey;
  struct nsm_cvlan_reg_key key;
  struct nsm_bridge *bridge;
  struct nsm_vlan *cvlan;
  struct nsm_vlan *svlan;
  struct nsm_vlan nvlan;
  struct avl_node *node;
  struct interface *ifp;
  struct listnode *nn;
  struct avl_node *rn;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (CVLAN Registration Table Entry Delete);

  pal_mem_set (&key, 0, sizeof (struct nsm_cvlan_reg_key));

  key.cvid = cvid;
  ret = NSM_L2_ERR_NONE;

  bridge = regtab->bridge;

  if (! bridge)
    {
      ret = NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
      goto EXIT;
    }

  NSM_VLAN_SET(&nvlan, cvid);

  rn = avl_search (bridge->vlan_table, (void *)&nvlan);

  if (! rn || !(cvlan = rn->info))
    {
      ret = NSM_VLAN_ERR_VLAN_NOT_FOUND;
      goto EXIT;
    }

  node = avl_search (regtab->reg_tab, (u_char *) &key);

  if (node && (tmpkey = AVL_NODE_INFO (node)))
   {
     if (nsm_bridge_regtag_to_uni_check (regtab, cvid,
                                         tmpkey->svid, PAL_TRUE)
                                           == PAL_TRUE)
       {
         ret = NSM_PRO_VLAN_ERR_CVLAN_MAP_ERR;
         goto EXIT;
       }

     NSM_VLAN_SET(&nvlan, tmpkey->svid);

     rn = avl_search (bridge->svlan_table, (void *)&nvlan);

     if (! rn || !(svlan = rn->info))
       {
         ret = NSM_VLAN_ERR_SVLAN_NOT_FOUND;
         goto EXIT;
       }

     /* Delete this node */
     avl_remove (regtab->reg_tab, tmpkey);

     /* Loop through all the ports and delete the mapping*/

     LIST_LOOP (regtab->port_list, ifp, nn)
       {
         zif = ifp->info;

         hal_vlan_delete_cvlan_registration_entry (zif->bridge->name,
                                                   zif->ifp->ifindex,
                                                   cvid,
                                                   tmpkey->svid);

         hal_vlan_delete_cvid_from_port (zif->bridge->name,
                                         zif->ifp->ifindex,
                                         cvid, tmpkey->svid);

         _nsm_pro_edge_notify_del_port_from_swctx (zif->bridge, zif->ifp,
                                                   tmpkey->cvid,
                                                   tmpkey->svid);

         nsm_cvlan_reg_tab_handle_entry_delete (tmpkey->svid, ifp);

#ifdef HAVE_ELMID
         /* Send msg to ELMI to delete evcs */
         nsm_server_evc_delete (zif->ifp, tmpkey->svid);
#endif /* HAVE_ELMID */
       }

     hal_vlan_delete_cvlan (regtab->bridge->name, cvid, tmpkey->svid);

     nsm_pro_edge_swctx_release (regtab->bridge, cvlan, svlan);

     nsm_reg_tab_svlan_info_release (regtab, tmpkey->svid, tmpkey->cvid);

     XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);

    }

EXIT:

  NSM_VLAN_FN_EXIT (ret);

}

s_int32_t
nsm_cvlan_reg_tab_entry_delete_by_svid (struct nsm_cvlan_reg_tab *regtab,
                                        u_int16_t svid)
{
  struct nsm_cvlan_reg_key *tmpkey;
  struct avl_node *next_node;
  struct nsm_bridge *bridge;
  struct nsm_vlan *cvlan;
  struct nsm_vlan *svlan;
  struct nsm_vlan nvlan;
  struct avl_node *node;
  struct interface *ifp;
  struct listnode *nn;
  struct avl_node *rn;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (CVLAN Registration Table Entry Delete by SVLAN);

  bridge = regtab->bridge;
  svlan = NULL;
  ret = NSM_L2_ERR_NONE;

  if (! bridge)
    {
      ret = NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
      goto EXIT;
    }

  for (node = avl_getnext (regtab->reg_tab, NULL); node; node = next_node)
     {
       next_node = avl_getnext (regtab->reg_tab, node);

       if (! (tmpkey = AVL_NODE_INFO (node)))
         continue;

       if (tmpkey->svid != svid)
         continue;

       NSM_VLAN_SET(&nvlan, tmpkey->cvid);

       rn = avl_search (bridge->vlan_table, (void *)&nvlan);

       if (! rn || !(cvlan = rn->info))
         continue;

       NSM_VLAN_SET(&nvlan, tmpkey->svid);

       rn = avl_search (bridge->svlan_table, (void *)&nvlan);

       if (! rn || !(svlan = rn->info))
         continue;

       /* Delete this node */
       avl_remove (regtab->reg_tab, tmpkey);

       /* Loop through all the ports and delete the mapping*/
       LIST_LOOP (regtab->port_list, ifp, nn)
         {
           zif = ifp->info;

           hal_vlan_delete_cvlan_registration_entry (zif->bridge->name,
                                                     zif->ifp->ifindex,
                                                     tmpkey->cvid,
                                                     svid);

           hal_vlan_delete_cvid_from_port (zif->bridge->name,
                                           zif->ifp->ifindex,
                                           tmpkey->cvid, svid);

           _nsm_pro_edge_notify_del_port_from_swctx (zif->bridge, zif->ifp,
                                                     tmpkey->cvid,
                                                     tmpkey->svid);
 
           nsm_cvlan_reg_tab_handle_entry_delete (svid, ifp);

#ifdef HAVE_ELMID
      nsm_server_evc_delete (ifp, svid);
#endif /* HAVE_ELMID */

         }

       hal_vlan_delete_cvlan (regtab->bridge->name, tmpkey->cvid,
                              tmpkey->svid);

       nsm_pro_edge_swctx_release (regtab->bridge, cvlan, svlan);

       nsm_reg_tab_svlan_info_release (regtab, tmpkey->svid, tmpkey->cvid);

       XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);
     }

EXIT:
  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_vlan_trans_tab_entry_add (struct interface *ifp, u_int16_t vid,
                              u_int16_t trans_vid)
{
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_trans_key *tmpkey;
  struct nsm_vlan_port *vlan_port;
  struct nsm_vlan_trans_key key;
  struct avl_node *node;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (Add Translation Table Entry);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port
      || ! br_port->trans_tab)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if ((!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, trans_vid)))
      && (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, trans_vid))) )
    {
      ret = NSM_VLAN_ERR_VLAN_NOT_IN_PORT;
      goto EXIT;
    }

  key.vid = vid;
  key.trans_vid = trans_vid;

  node = avl_search (br_port->rev_trans_tab, (u_char *) &key);

  if (node && (tmpkey = AVL_NODE_INFO (node)))
    {
      if (tmpkey->vid == vid)
        goto EXIT;
    
      ret = NSM_PRO_VLAN_ERR_VLAN_TRANS_EXISTS;
      goto EXIT; 
    }

  node = avl_search (br_port->trans_tab, (u_char *) &key);

  if (node && (tmpkey = AVL_NODE_INFO (node)))
    {
      if (tmpkey->trans_vid == trans_vid)
        goto EXIT;

      hal_vlan_delete_vlan_trans_entry (zif->bridge->name,
                                        zif->ifp->ifindex,
                                        tmpkey->vid,
                                        tmpkey->trans_vid);

      avl_remove (br_port->trans_tab, tmpkey);
      avl_remove (br_port->rev_trans_tab, tmpkey);

      XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);

    }

  tmpkey = XCALLOC (MTYPE_NSM_VLAN_DBL_VID_KEY,
                    sizeof (struct nsm_vlan_trans_key));

  if (! tmpkey)
    {
      ret = NSM_L2_ERR_MEM;
      goto EXIT;
    }

  tmpkey->vid = vid;
  tmpkey->trans_vid = trans_vid;

  ret = avl_insert (br_port->trans_tab, (void *) tmpkey);

  if (ret < 0)
    {
      ret = NSM_L2_ERR_MEM;
      goto CLEANUP;
    }

  ret = avl_insert (br_port->rev_trans_tab, (void *) tmpkey);

  if (ret < 0)
    {
      ret = NSM_L2_ERR_MEM;
      goto CLEANUP;
    }

  ret = hal_vlan_create_vlan_trans_entry (zif->bridge->name,
                                          zif->ifp->ifindex,
                                          vid,
                                          trans_vid);

  if (ret != HAL_SUCCESS)
    {
      ret = NSM_PRO_VLAN_ERR_VLAN_TRANS_HAL_ERR;
      goto CLEANUP;
    }

  goto EXIT;

CLEANUP:

  if (tmpkey)
    {
      avl_remove (br_port->trans_tab, (void *) tmpkey);
      XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);
    }

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}


s_int32_t
nsm_vlan_trans_tab_entry_delete (struct interface *ifp, u_int16_t vid)
{
  struct nsm_vlan_trans_key *tmpkey;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_trans_key key;
  struct avl_node *node;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (Delete Translation Table Entry);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port
      || ! br_port->trans_tab)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  key.vid = vid;

  node = avl_search (br_port->trans_tab, (u_char *) &key);

  if (node && (tmpkey = AVL_NODE_INFO (node)))
    {
      hal_vlan_delete_vlan_trans_entry (zif->bridge->name,
                                        zif->ifp->ifindex,
                                        vid,
                                        tmpkey->trans_vid);

      avl_remove (br_port->trans_tab, tmpkey);
      avl_remove (br_port->rev_trans_tab, tmpkey);

      XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);

    }

EXIT:

  NSM_VLAN_FN_EXIT (ret);

}

void
nsm_vlan_trans_tab_delete (struct interface *ifp)
{
  struct nsm_vlan_trans_key *tmpkey;
  struct nsm_bridge_port *br_port;
  struct avl_node *node;
  struct nsm_if *zif;

  NSM_VLAN_FN_ENTER (Translation Table Delete);

  zif = ifp->info;

  if (! zif
      || ! zif->switchport)
    goto EXIT;

  br_port = zif->switchport;

  if (! br_port
      || ! br_port->trans_tab)
    goto EXIT;

  for (node = avl_top (br_port->trans_tab); node; node = avl_next (node))
     {
       if (! (tmpkey = AVL_NODE_INFO (node)))
         continue;

       hal_vlan_delete_vlan_trans_entry (zif->bridge->name,
                                         zif->ifp->ifindex,
                                         tmpkey->vid,
                                         tmpkey->trans_vid);

       XFREE (MTYPE_NSM_VLAN_DBL_VID_KEY, tmpkey);
     }

  avl_tree_free (&br_port->trans_tab, NULL);
  avl_tree_free (&br_port->rev_trans_tab, NULL);
  br_port->trans_tab = NULL;
  br_port->rev_trans_tab = NULL;

EXIT:

  NSM_VLAN_FN_EXIT ();
}

s_int32_t
nsm_bridge_uni_set_service_attr (struct interface *ifp,
                                 enum nsm_uni_service_attr service_attr)
{
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_if *zif;
  s_int32_t ret;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
#endif /* HAVE_ELMID */

  NSM_VLAN_FN_ENTER (UNI Service Attribute Set);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  if (vlan_port->regtab
      && nsm_bridge_uni_to_regtag_check (vlan_port->regtab, service_attr)
                                         == PAL_TRUE)
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_MAP_ERR;
      goto EXIT;
    }

  br_port->uni_ser_attr = service_attr;

  /* unset previously set default evc */
  if (service_attr != NSM_UNI_BNDL) 
    vlan_port->uni_default_evc = NSM_DEFAULT_VAL;

#ifdef HAVE_ELMID
  /* Send UNI attribute update message to ELMI */
  NSM_SET_CTYPE (cindex, NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE);
  nsm_server_uni_update (br_port, ifp, cindex);
#endif /* HAVE_ELMID */

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_bridge_uni_unset_service_attr (struct interface *ifp,
                                 enum nsm_uni_service_attr service_attr)
{
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_if *zif;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
#endif /* HAVE_ELMID */
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (UNI Service Attribute Unset);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  if (br_port->uni_ser_attr != service_attr)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_SERVICE_ATTR;
      goto EXIT;
    }

  if (br_port->uni_ser_attr == NSM_UNI_BNDL)
    vlan_port->uni_default_evc = NSM_DEFAULT_VAL;

  br_port->uni_ser_attr = NSM_UNI_MUX_BNDL;

#ifdef HAVE_ELMID
  /* Send UNI attribute update message to ELMI */
  NSM_SET_CTYPE (cindex, NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE);
  nsm_server_uni_update (br_port, ifp, cindex);
#endif /* HAVE_ELMID */


EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_bridge_uni_set_name (struct interface *ifp,
                         char *uni_name)
{
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_if *zif;
  s_int32_t ret = RESULT_OK;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
#endif /* HAVE_ELMID */

  NSM_VLAN_FN_ENTER (UNI Name Set);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  pal_strncpy (br_port->uni_name, uni_name, NSM_UNI_NAME_MAX + 1);

#ifdef HAVE_ELMID
  NSM_SET_CTYPE (cindex, NSM_ELMI_CTYPE_UNI_ID);
  nsm_server_uni_update (br_port, ifp, cindex);
#endif /* HAVE_ELMID */ 

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_bridge_uni_unset_name (struct interface *ifp)
{
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_if *zif;
  s_int32_t ret = RESULT_OK;

  NSM_VLAN_FN_ENTER (UNI Name Unset);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  pal_mem_set (br_port->uni_name, 0, NSM_UNI_NAME_MAX + 1);

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

/**@brief - API To set the MAX EVC for the UNI.
 * @param ifp - Interface for which max evc need to be set.
 * @param max_evc - Configured max_evc value for the uni.
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t 
nsm_uni_set_max_evc (struct interface *ifp, u_int16_t max_evc)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_if *zif = NULL;
  s_int32_t ret;
  u_int16_t evc_count = PAL_FALSE;
  u_int16_t i = PAL_FALSE;

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;

  /* IF the port is not a UNI-N
   * or the Customer Edge Port,
   * then return an error
   */
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }
  
  if ((br_port->uni_max_evcs != PAL_FALSE)
      && (br_port->uni_max_evcs != NSM_VLAN_MAX))
    {
      return NSM_ERR_MAX_EVC_CONFIGURED; 
    }
    
  /* Set the MAX EVC */
  if (vlan_port->regtab == NULL)
    {
      br_port->uni_max_evcs = max_evc;
    }
  else
    {
      /* If the configured svlans on the CEP,
       * are more than the user given EVCs, 
       * then return an error
       */
      for (i = 1; i <= NSM_VLAN_MAX; i++)
        {
          if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->svlanMemberBmp, i))
            {
              evc_count++;
            }
        }
      
     if (max_evc < evc_count)
       return NSM_ERR_MAX_EVC_LESS;
     else
       {
         br_port->uni_max_evcs = max_evc;
       }
    }
  return RESULT_OK;
}

/**@brief - API To unset the MAX EVC for the UNI.
 * @param ifp - Interface for which max evc need to be set.
 * @return RESULT_OK - For Success .
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t 
nsm_uni_unset_max_evc (struct interface *ifp)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_if *zif = NULL;
  s_int32_t ret = PAL_FALSE;

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }

  /* This will set the max EVC to MAX number of VLANs(4094) */
  br_port->uni_max_evcs = NSM_VLAN_MAX;

  return RESULT_OK;
}



/**@brief  - API to configure BW profile Parameters per UNI.
 * @param ifp - UNI for which the bw profile parameters need to be set.
 * @param cir - Committed information rate per uni.
 * @param cbs - Committed burst size per uni.
 * @param eir - Excess information rate per uni.
 * @param ebs - Excess burst size per uni.
 * @coupling_flag - enable/disable coupling flag per uni.
 * @ color_mode - Variable to set color-aware/color-blind mode per uni.
 * @ bw_profile_type - Specified ingress/egress bw profiling type.
 * @ bw_profile_parameter - Indicates the bw profile parameter set.
 * @return RESULT_OK - For Success.
 * @return Error - Error reflecting the type of failure.
 * */
s_int32_t
nsm_uni_bw_profiling (struct interface *ifp, u_int32_t cir, u_int32_t cbs, 
    u_int32_t eir, u_int32_t ebs, u_int8_t coupling_flag, u_int8_t color_mode,
    u_int8_t bw_profile_type, u_int8_t bw_profile_parameter)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *bw_profile = NULL;
  s_int32_t ret;
  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;
  
  /* BW Profile Parameters can be configured only on the Customer Edge Port
   */ 
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }
  
  
  if (! (zif->nsm_bw_profile))
    {
      /* Allocationg memory for zif->nsm_bw_profile*/
      zif->nsm_bw_profile = nsm_bridge_if_bw_profile_init (zif);
      
        bw_profile = zif->nsm_bw_profile;
    }
  else
    bw_profile = zif->nsm_bw_profile;

  if (! bw_profile)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  /* When user configured ingress-policing */
  if (bw_profile_type == NSM_INGRESS_POLICING)
    {
      /* IF the Ingress Bandwidth Profiling per UNI is active,
       * the user cannot modify the bw profile parameters.
       */
      if ((bw_profile->ingress_uni_bw_profile) &&
          (bw_profile->ingress_uni_bw_profile->active == PAL_TRUE))
        return NSM_ERR_BW_PROFILE_ACTIVE;
      
      /* If the bw profiling per EVC is configured, then return and error
       * section 6.2.2 MEF 10.1
       */
      if ((bw_profile->ingress_bw_profile_status == BW_PROFILE_CONFIGURED)
          && (bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_EVC))
        {
          return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
        }
      else
        {
          /* After the above checks, update the bw profile on the interface
           * as per UNI
           */
          bw_profile->ingress_bw_profile_status = BW_PROFILE_CONFIGURED;
          bw_profile->ingress_bw_profile_type = NSM_BW_PROFILE_PER_UNI;
        }
       
      /* Allocate memory for Ingress UNI BW Profile
       */
      if (!bw_profile->ingress_uni_bw_profile)
        {
          bw_profile->ingress_uni_bw_profile = XCALLOC(
              MTYPE_NSM_INGRESS_UNI_BW_PROFILE,
              sizeof (struct nsm_bw_profile_parameters));
        }

    }
  else
    {
      /* IF the Egress Bandwidth Profiling per UNI is active,
       * the user cannot modify the bw profile parameters.
       */
      if ((bw_profile->egress_uni_bw_profile)
          && (bw_profile->egress_uni_bw_profile->active == PAL_TRUE))
        return NSM_ERR_BW_PROFILE_ACTIVE;

      /* If the bw profiling per EVC is configured, then return and error
       * section 6.2.2 MEF 10.1
       */
      if ((bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED)
          && (bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_EVC))
        {
          return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
        }
      else
        {
          /* After the above checks, update the bw profile on the interface
           * as per UNI
           */
          bw_profile->egress_bw_profile_status = BW_PROFILE_CONFIGURED;
          bw_profile->egress_bw_profile_type = NSM_BW_PROFILE_PER_UNI;
        }
      /* Allocate memory for egress UNI BW Profile
       */
      if (!bw_profile->egress_uni_bw_profile)
        {
          bw_profile->egress_uni_bw_profile = XCALLOC(
              MTYPE_NSM_EGRESS_UNI_BW_PROFILE,
              sizeof (struct nsm_bw_profile_parameters));
        }

    }
  
  /* Update the bw profile parameter configured into the structures
   */
  if (bw_profile_type == NSM_INGRESS_POLICING)
    {
      switch (bw_profile_parameter)
        {
        case NSM_BW_PARAMETER_CIR:
          bw_profile->ingress_uni_bw_profile->comm_info_rate = cir;
          break;
        case NSM_BW_PARAMETER_CBS:
          bw_profile->ingress_uni_bw_profile->comm_burst_size = cbs;
          break;
        case NSM_BW_PARAMETER_EIR:
          bw_profile->ingress_uni_bw_profile->excess_info_rate = eir;
          break;
        case NSM_BW_PARAMETER_EBS:
          bw_profile->ingress_uni_bw_profile->excess_burst_size = ebs;
          break;
        case NSM_BW_PARAMETER_COUPLING_FLAG:
          bw_profile->ingress_uni_bw_profile->coupling_flag = coupling_flag;
          break;
        case NSM_BW_PARAMETER_COLOR_MODE:
          bw_profile->ingress_uni_bw_profile->color_mode = color_mode;
          break;
        default:
          return NSM_L2_ERR_INVALID_ARG;
        }
    }
  else
    {
      switch (bw_profile_parameter)
        {
        case NSM_BW_PARAMETER_CIR:
          bw_profile->egress_uni_bw_profile->comm_info_rate = cir;
          break;
        case NSM_BW_PARAMETER_CBS:
          bw_profile->egress_uni_bw_profile->comm_burst_size = cbs;
          break;
        case NSM_BW_PARAMETER_EIR:
          bw_profile->egress_uni_bw_profile->excess_info_rate = eir;
          break;
        case NSM_BW_PARAMETER_EBS:
          bw_profile->egress_uni_bw_profile->excess_burst_size = ebs;
          break;
        case NSM_BW_PARAMETER_COUPLING_FLAG:
          bw_profile->egress_uni_bw_profile->coupling_flag = coupling_flag;
          break;
        case NSM_BW_PARAMETER_COLOR_MODE:
          bw_profile->egress_uni_bw_profile->color_mode    = color_mode;
          break;
        default:
          return NSM_L2_ERR_INVALID_ARG;
        }
    }

  return RESULT_OK;
}

/**@brief  - API to delete configured BW profile Parameters per UNI.
 * @ ifp   - Interface on which the bw profiling neet be set per uni.
 * @ bw_profile_type - Specified ingress/egress bw profiling type per uni.
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t
nsm_delete_uni_bw_profiling (struct interface *ifp, u_int8_t bw_profile_type)
{
  struct nsm_if *zif = NULL;
  struct nsm_band_width_profile *bw_profile = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;

  if (ifp == NULL)
    return NSM_ERR_NOT_FOUND;

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;
  
  /* If the port is not a CEP, then return an error
   */
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }
  
  if (! (zif->nsm_bw_profile))
    {
      return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
    }

  bw_profile = zif->nsm_bw_profile;

  if (bw_profile_type == NSM_INGRESS_POLICING)
    {
      /* If there is no ingress uni bw profile
       * return an error
       */
      if (!bw_profile->ingress_uni_bw_profile)
        {
          return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
        }
      else
        {
          /* If the bw profile is active, user cannot delete the
           * existing bw profile
           */
          if (bw_profile->ingress_uni_bw_profile->active == PAL_TRUE)
            return NSM_ERR_BW_PROFILE_ACTIVE;

          /* Unconfigure the bw profile parameters
           * and free the memory
           */
          bw_profile->ingress_bw_profile_status = 
            BW_PROFILE_NOT_CONFIGURED;
          bw_profile->ingress_bw_profile_type = PAL_FALSE;

          XFREE (MTYPE_NSM_INGRESS_UNI_BW_PROFILE,
              bw_profile->ingress_uni_bw_profile);
          bw_profile->ingress_uni_bw_profile = NULL;

          /* If there are no other bw profile parameters,
           * then delete the nsm_bw_profile pointer in zif
           */
          if ((!bw_profile->egress_uni_bw_profile)
              && (!bw_profile->uni_evc_bw_profile_list))
            {
              XFREE (MTYPE_NSM_UNI_BW_PROFILE, 
                  zif->nsm_bw_profile);
              zif->nsm_bw_profile  = NULL;
            }
        }
    }
  else
    {
      /* If there is no ingress uni bw profile
       * return an error
       */
      if (!bw_profile->egress_uni_bw_profile)
        {
          return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
        }
      else
        {
          /* If the bw profile is active, user cannot delete the
           * existing bw profile
           */
          if (bw_profile->egress_uni_bw_profile->active == PAL_TRUE)
            return NSM_ERR_BW_PROFILE_ACTIVE;

          /* Unconfigure the bw profile parameters
           * and free the memory
           */
          bw_profile->egress_bw_profile_status = 
            BW_PROFILE_NOT_CONFIGURED;
          bw_profile->egress_bw_profile_type = PAL_FALSE;

          XFREE (MTYPE_NSM_EGRESS_UNI_BW_PROFILE,
              bw_profile->egress_uni_bw_profile);

          bw_profile->egress_uni_bw_profile = NULL;

          /* If there are no other bw profile parameters,
           * then delete the nsm_bw_profile pointer in zif
           */
          if ((!bw_profile->ingress_uni_bw_profile)
              && (!bw_profile->uni_evc_bw_profile_list))
            {
              XFREE (MTYPE_NSM_UNI_BW_PROFILE, 
                  zif->nsm_bw_profile);

              zif->nsm_bw_profile= NULL;
            }
        }
    }

  return RESULT_OK;
}

/**@brief  - API to activate/inactivate BW profile Parameters per UNI.
 * @ ifp   - Interface on which the bw profiling neet be set per uni.
 * @ bw_profile_type - Specified ingress/egress bw profiling type per uni.
 * @ bw_profile_status - Activate/inactivate the configured bw profile per uni,
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t
nsm_set_uni_bw_profiling (struct interface *ifp, u_int8_t bw_profile_type,
    u_int8_t bw_profile_status)
{
  struct nsm_if *zif = NULL;
  struct nsm_band_width_profile *bw_profile = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
#ifdef HAVE_ELMID
  struct nsm_msg_bw_profile uni_bw_msg;
#endif /* HAVE_ELMID */

  if (ifp == NULL)
    return NSM_ERR_NOT_FOUND;

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;

  /* If the port is not a CEP,
   * then return an error
   */
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }
  
  if (! (zif->nsm_bw_profile))
    {
      return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
    }

  bw_profile = zif->nsm_bw_profile;

  if (bw_profile_type == NSM_INGRESS_POLICING)
    {
      /* If the bw profile per EVC is configured, then return an error
       */
      if ((bw_profile->ingress_bw_profile_status == BW_PROFILE_CONFIGURED)
          && (bw_profile->ingress_bw_profile_type ==
            NSM_BW_PROFILE_PER_EVC))
        {
          return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
        }
      /* IF ingress bw profile per uni is not configured, 
       * then return an error
       */
      if (!bw_profile->ingress_uni_bw_profile)
        return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
      
      /* If user configured bw profile active */
      if (bw_profile_status == PAL_TRUE)
        {
          /* section 7.11.1 MEF 10.1, If CIR is greater than zero,
           *  CBS MUST be greater than or equal to the largest
           *  Maximum Transmission Unit size among all of the EVCs
           *   that the Bandwidth Profile applies to.
           *   In PacOS, the max MTU size on Linux can be 1500, so the CBS
           *    should be greater than or equal to 1500 */
          if (bw_profile->ingress_uni_bw_profile->comm_info_rate >
              PAL_FALSE)
            {
              if (bw_profile->ingress_uni_bw_profile->comm_burst_size <
                  1500)
                return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
            }

          if (bw_profile->ingress_uni_bw_profile->excess_info_rate > PAL_FALSE)
            {
              if (bw_profile->ingress_uni_bw_profile->excess_burst_size <
                  1500)
                return NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED;
            }

          /* Committed Information Rate should not be greater than
           *  Excess information Rate
           */
          if (bw_profile->ingress_uni_bw_profile->excess_info_rate != 
              NSM_DEFAULT_VAL)
            { 
              if (bw_profile->ingress_uni_bw_profile->comm_info_rate >
                  bw_profile->ingress_uni_bw_profile->excess_info_rate)
                return NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED;
            }

          /* Committed Burst Size should not be greater than
           * Excess Burst Size
           */
          if (bw_profile->ingress_uni_bw_profile->excess_burst_size != 
              NSM_DEFAULT_VAL)
            {
              if (bw_profile->ingress_uni_bw_profile->comm_burst_size >
                  bw_profile->ingress_uni_bw_profile->excess_burst_size)
                return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
            }

          /* After all the required checks, the Ingress Bandwidth Profile
           * per UNI will be activated
           */
          bw_profile->ingress_uni_bw_profile->active = PAL_TRUE;

#ifdef HAVE_ELMID
         pal_mem_set (&uni_bw_msg, 0, sizeof (struct nsm_msg_bw_profile));

         /* send msg to elmi regarding evc bw profile */
         NSM_SET_CTYPE(uni_bw_msg.cindex, NSM_ELMI_CTYPE_UNI_BW);
         
         uni_bw_msg.ifindex = br_port->ifindex;
         uni_bw_msg.cir = bw_profile->ingress_uni_bw_profile->comm_info_rate;
         uni_bw_msg.cbs = bw_profile->ingress_uni_bw_profile->comm_burst_size;
         uni_bw_msg.eir = bw_profile->ingress_uni_bw_profile->excess_info_rate;
         uni_bw_msg.ebs = bw_profile->ingress_uni_bw_profile->excess_burst_size;
         uni_bw_msg.cf = bw_profile->ingress_uni_bw_profile->coupling_flag;
         uni_bw_msg.cm = bw_profile->ingress_uni_bw_profile->color_mode;

         nsm_server_elmi_send_bw (br_port, &uni_bw_msg, NSM_MSG_ELMI_UNI_BW_ADD); 

#endif /* HAVE_ELMID */
        }
      else
        {
          /* IF user inactivated the BW Profile */
          if (bw_profile->ingress_uni_bw_profile->active == PAL_TRUE)
            {
              bw_profile->ingress_uni_bw_profile->active = PAL_FALSE;

#ifdef HAVE_ELMID
              pal_mem_set (&uni_bw_msg, 0, sizeof (struct nsm_msg_bw_profile));

              /* send msg to elmi regarding evc bw profile */
              NSM_SET_CTYPE(uni_bw_msg.cindex, NSM_ELMI_CTYPE_UNI_BW);

              uni_bw_msg.ifindex = br_port->ifindex;

              /* send msg to elmi to delete uni bw profile */
              nsm_server_elmi_send_bw (br_port, &uni_bw_msg, 
                                       NSM_MSG_ELMI_UNI_BW_DEL);
#endif /* HAVE_ELMID */
            }
          else
            return NSM_ERR_BW_PROFILE_NOT_ACTIVE;
        }
    }
  else
    {
      /* If the egress bw profile per EVC is configured, then return an error
      */
      if ((bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED)
          && (bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_EVC))
        {
          return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
        }

      /* IF egress bw profile per uni is not configured, 
       * then return an error
       */
      if (!bw_profile->egress_uni_bw_profile)
        return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
       
      /* IF egress bw profile is activated */
      if (bw_profile_status == PAL_TRUE)
        {
          /* section 7.11.1 MEF 10.1, If CIR is greater than zero,
           *  CBS MUST be greater than or equal to the largest
           *  Maximum Transmission Unit size among all of the EVCs
           *   that the Bandwidth Profile applies to.
           *   In PacOS, the max MTU size on Linux can be 1500, so the CBS
           *    should be greater than or equal to 1500 */
          if (bw_profile->egress_uni_bw_profile->comm_info_rate > PAL_FALSE)
            {
              if (bw_profile->egress_uni_bw_profile->comm_burst_size < 1500)
                return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
            }

          if (bw_profile->egress_uni_bw_profile->excess_info_rate >
              PAL_FALSE)
            {
              if (bw_profile->egress_uni_bw_profile->excess_burst_size <
                  1500)
                return NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED;
            }

          /* Committed Information Rate should not be greater than
           *  Excess information Rate
           */
          if (bw_profile->egress_uni_bw_profile->excess_info_rate != 
              NSM_DEFAULT_VAL)
            { 
              if (bw_profile->egress_uni_bw_profile->comm_info_rate >
                  bw_profile->egress_uni_bw_profile->excess_info_rate)
                return NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED;
            }

          /* Committed Burst Size should not be greater than
           *  Excess Burst Size
           */
          if (bw_profile->egress_uni_bw_profile->excess_burst_size != 
              NSM_DEFAULT_VAL)
            { 
              if (bw_profile->egress_uni_bw_profile->comm_burst_size >
                  bw_profile->egress_uni_bw_profile->excess_burst_size)
                return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
            }

          /* After all the required checks, the Ingress Bandwidth Profile
           * per UNI will be activated
           */
          bw_profile->egress_uni_bw_profile->active = PAL_TRUE;

        }
      else
        {
          /* IF user inactivated the egress uni bw profile
           */
          if (bw_profile->egress_uni_bw_profile->active == PAL_TRUE)
            {
              bw_profile->egress_uni_bw_profile->active = PAL_FALSE;
            }
          else
            return NSM_ERR_BW_PROFILE_NOT_ACTIVE;
        }

    }

  return RESULT_OK;

}

/**@brief  - API to configure the bw profile parameters per EVC.
 * @param ifp - UNI for which the bw profile parameters need to be set.
 * @param bw_profile - Specifies the evc for which the bw profile 
 *                     parameters need to be set.
 * @param cir - Committed information rate per evc.
 * @param cbs - Committed burst size per evc.
 * @param eir - Excess information rate per evc.
 * @param ebs - Excess burst size per evc.
 * @coupling_flag - Enable/disable coupling flag per evc.
 * @ color_mode - Variable to set color-aware/color-blind mode per evc.
 * @ bw_profile_type - Specified ingress/egress bw profiling type.
 * @ bw_profile_parameter - Indicates the bw profile parameter set.
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t
nsm_evc_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int32_t cir, u_int32_t cbs,
    u_int32_t eir, u_int32_t ebs, u_int8_t coupling_flag, u_int8_t color_mode,
    u_int8_t bw_profile_type, u_int8_t bw_profile_parameter)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *nsm_bw_profile = NULL;
  s_int32_t ret;
  ret = NSM_L2_ERR_INVALID_ARG;

  if (ifp == NULL)
    {
      return NSM_VLAN_ERR_IFP_NOT_BOUND; 
    }

 if (bw_profile == NULL)
   {
     return ret;
   }
  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;

  /* IF the ports is not a CEP, then return an error */
 
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }

 if (zif->nsm_bw_profile)
   {
     nsm_bw_profile = zif->nsm_bw_profile;
   }
  else
    return NSM_L2_ERR_INVALID_ARG;

 if (bw_profile_type == NSM_INGRESS_POLICING)
   {
     /* IF EVC BW Profile is activated, then user cannot change the 
      * bw profile parameters applied
      */
     if ((bw_profile->ingress_evc_bw_profile) &&
         (bw_profile->ingress_evc_bw_profile->active == PAL_TRUE))
       return NSM_ERR_BW_PROFILE_ACTIVE;

     if (nsm_bw_profile && (nsm_bw_profile->ingress_bw_profile_status
                                    == BW_PROFILE_CONFIGURED))
       {
          /* IF the UNI BW profile is configured, then return an error */
         if (nsm_bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }
         /* When user configured bw profile per evc,
          * If the bw profile configured for the evc is per cos, then return
          *  an error
          */
         if (bw_profile->ingress_bw_profile_type == NSM_EVC_COS_BW_PROFILE)
           {
             return NSM_ERR_BW_PROFILE_PER_COS_CONFIGURED;
           }
       }
     /* after the above checks, update the actual bw profile being
      *  configured
      */
     if (nsm_bw_profile)
       {
         nsm_bw_profile->ingress_bw_profile_status = BW_PROFILE_CONFIGURED;
         nsm_bw_profile->ingress_bw_profile_type = NSM_BW_PROFILE_PER_EVC;
       }
     bw_profile->ingress_bw_profile_type = NSM_EVC_BW_PROFILE_PER_EVC; 

     /* Allocate memory for egress evc bw profile */
     if (bw_profile->ingress_evc_bw_profile == NULL)
       {
         bw_profile->ingress_evc_bw_profile= XCALLOC (
             MTYPE_NSM_INGRESS_EVC_BW_PROFILE, 
             sizeof (struct nsm_bw_profile_parameters));
       }
   }/* if (bw_profile_type == NSM_INGRESS_POLICING)*/
 else
   {

     /* IF EVC BW Profile is activated, then user cannot change the 
      * bw profile parameters applied
      */
     if ((bw_profile->egress_evc_bw_profile) &&
         (bw_profile->egress_evc_bw_profile->active == PAL_TRUE))
       return NSM_ERR_BW_PROFILE_ACTIVE;

     if (nsm_bw_profile && (nsm_bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED))
       {
          /* IF the UNI BW profile is configured, then return an error */
         if (nsm_bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }

         /* When user configured bw profile per evc,
          * If the bw profile configured for the evc is per cos, then return
          *  an error
          */
         if (bw_profile->egress_bw_profile_type == NSM_EVC_COS_BW_PROFILE)
           {
             return NSM_ERR_BW_PROFILE_PER_COS_CONFIGURED;
           }
       }
     /* after the above checks, update the actual bw profile being
      *  configured
      */
     if (nsm_bw_profile)
       {
         nsm_bw_profile->egress_bw_profile_status = BW_PROFILE_CONFIGURED;
         nsm_bw_profile->egress_bw_profile_type = NSM_BW_PROFILE_PER_EVC;
       }
     bw_profile->egress_bw_profile_type = NSM_EVC_BW_PROFILE_PER_EVC; 

      /* Allocate memory for egress evc bw profile
       */
     if (bw_profile->egress_evc_bw_profile == NULL)
       {
         bw_profile->egress_evc_bw_profile = XCALLOC (
             MTYPE_NSM_EGRESS_EVC_BW_PROFILE, 
             sizeof (struct nsm_bw_profile_parameters));
       }

     if (bw_profile->egress_evc_bw_profile == NULL)
       {
         return NSM_L2_ERR_INVALID_ARG;
       }

   }/*else */

 /* Updating the bw profile parameter configured to the evc bw profile
  * structures
  */
 if (bw_profile_type == NSM_INGRESS_POLICING)
   {
     switch (bw_profile_parameter)
       {
       case NSM_BW_PARAMETER_CIR:
         bw_profile->ingress_evc_bw_profile->comm_info_rate = cir;
         break;
       case NSM_BW_PARAMETER_CBS:
         bw_profile->ingress_evc_bw_profile->comm_burst_size = cbs;
         break;
       case NSM_BW_PARAMETER_EIR:
         bw_profile->ingress_evc_bw_profile->excess_info_rate = eir;
         break;
       case NSM_BW_PARAMETER_EBS:
         bw_profile->ingress_evc_bw_profile->excess_burst_size = ebs;
         break;
       case NSM_BW_PARAMETER_COUPLING_FLAG:
         bw_profile->ingress_evc_bw_profile->coupling_flag = coupling_flag;
         break;
       case NSM_BW_PARAMETER_COLOR_MODE:
         bw_profile->ingress_evc_bw_profile->color_mode = color_mode;
         break;
       default:
         return NSM_L2_ERR_INVALID_ARG;
       }
   }
 else
   {
     switch (bw_profile_parameter)
       {
       case NSM_BW_PARAMETER_CIR:
         bw_profile->egress_evc_bw_profile->comm_info_rate = cir;
         break;
       case NSM_BW_PARAMETER_CBS:
         bw_profile->egress_evc_bw_profile->comm_burst_size = cbs;
         break;
       case NSM_BW_PARAMETER_EIR:
         bw_profile->egress_evc_bw_profile->excess_info_rate = eir;
         break;
       case NSM_BW_PARAMETER_EBS:
         bw_profile->egress_evc_bw_profile->excess_burst_size = ebs;
         break;
       case NSM_BW_PARAMETER_COUPLING_FLAG:
         bw_profile->egress_evc_bw_profile->coupling_flag = coupling_flag;
         break;
       case NSM_BW_PARAMETER_COLOR_MODE:
         bw_profile->egress_evc_bw_profile->color_mode    = color_mode;
         break;
       default:
         return NSM_L2_ERR_INVALID_ARG;
       }

   }

  return RESULT_OK;
}

/**@brief  - API to delete the bw profile per EVC.
 * @param ifp - UNI for which the bw profile parameters need to be set.
 * @param bw_profile - Specifies the evc for which the bw profile 
 *                     parameters need to be deleted.
 * @ bw_profile_type - specified ingress/egress bw profiling type.
 * @return RESULT_OK - For Success.
 * @return Error - Error reflecting the type of failure.
 * */
s_int32_t 
nsm_delete_evc_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *evc_bw_profile,  u_int8_t bw_profile_type)
{
  struct nsm_if *zif = NULL;
  struct nsm_band_width_profile *bw_profile = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_uni_evc_bw_profile *evc_profile = NULL;
  struct listnode *evc_node = NULL;
  u_int8_t evc_bw_profile_count = PAL_FALSE;

  if (ifp == NULL)
    return NSM_ERR_NOT_FOUND;

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;

  /* IF the port is not a CEP, then return an error
   */
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }
  
  if (! (zif->nsm_bw_profile))
    {
      return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
    }

  bw_profile = zif->nsm_bw_profile;

  if (bw_profile_type == NSM_INGRESS_POLICING)
    {
      /* IF there is ingress evc bw profile, then return an error */
      if (!evc_bw_profile->ingress_evc_bw_profile)
        {
          return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
        }
      else
        {
          /* IF the bw profile is activated, deletion will not be allowed */
          if (evc_bw_profile->ingress_evc_bw_profile->active == PAL_TRUE)
            return NSM_ERR_BW_PROFILE_ACTIVE;

          /* If there are any other EVC Bandwidth profiling configured,
           * then the status and type will not be set to default values
           */
          if (bw_profile->uni_evc_bw_profile_list)
            if (bw_profile->uni_evc_bw_profile_list->count == PAL_FALSE)
              {
                bw_profile->ingress_bw_profile_status = 
                  BW_PROFILE_NOT_CONFIGURED;
                bw_profile->ingress_bw_profile_type = PAL_FALSE;
              }

          /* Setting the ingress bandwidth profiling per EVC
           * to default values*/
          evc_bw_profile->ingress_bw_profile_type = PAL_FALSE;

          /* Free the memory of the ingress evc bandwidth profile
           */
          XFREE (MTYPE_NSM_INGRESS_EVC_BW_PROFILE,
              evc_bw_profile->ingress_evc_bw_profile);
          evc_bw_profile->ingress_evc_bw_profile = NULL;

          /* Check if there are any other bw profile per evc 
           * on this interface to unset the status flag
           */
         if (bw_profile->ingress_bw_profile_type ==
             NSM_BW_PROFILE_PER_EVC) 
           {
             if (bw_profile->uni_evc_bw_profile_list)
               LIST_LOOP (bw_profile->uni_evc_bw_profile_list,
                   evc_profile, evc_node)
                 {
                   if ((evc_profile->ingress_evc_bw_profile) ||
                       (evc_profile->ingress_bw_profile_type ==
                        NSM_EVC_COS_BW_PROFILE))
                     {
                       evc_bw_profile_count++;
                       break;
                     }    
                 } 
             
             if (evc_bw_profile_count == PAL_FALSE)
               {
                 bw_profile->ingress_bw_profile_status = BW_PROFILE_NOT_CONFIGURED;
                 bw_profile->ingress_bw_profile_type = BW_PROFILE_NOT_CONFIGURED;
               }
           }
        }

    }
  else
    {
      /* IF there is ingress evc bw profile, then return an error */
      if (!evc_bw_profile->egress_evc_bw_profile)
        {
          return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
        }
      else
        {
          /* IF the bw profile is activated, deletion will not be allowed */
          if (evc_bw_profile->egress_evc_bw_profile->active == PAL_TRUE)
            return NSM_ERR_BW_PROFILE_ACTIVE;
          
          /* If there are any other EVC Bandwidth profiling done,
           * then the status and type will not be set to default values
           */
          if (bw_profile->uni_evc_bw_profile_list)
            if (bw_profile->uni_evc_bw_profile_list->count == PAL_FALSE)
              {
                bw_profile->egress_bw_profile_status = 
                  BW_PROFILE_NOT_CONFIGURED;
                bw_profile->egress_bw_profile_type = PAL_FALSE;
              }

          /* Setting the ingress bandwidth profiling per EVC
           * to default values*/
          evc_bw_profile->egress_bw_profile_type = PAL_FALSE;

          /* Free the memory of the ingress evc bandwidth profile
           */
          XFREE (MTYPE_NSM_EGRESS_EVC_BW_PROFILE,
              evc_bw_profile->egress_evc_bw_profile);
          evc_bw_profile->egress_evc_bw_profile = NULL;

          /* Check if there are any other bw profile per evc 
           * on this interface to unset the status flag
           */
         if (bw_profile->egress_bw_profile_type ==
             NSM_BW_PROFILE_PER_EVC) 
           {
             if (bw_profile->uni_evc_bw_profile_list)
               LIST_LOOP (bw_profile->uni_evc_bw_profile_list,
                   evc_profile, evc_node)
                 {
                   if ((evc_profile->egress_evc_bw_profile)||
                       (evc_profile->egress_bw_profile_type ==
                        NSM_EVC_COS_BW_PROFILE))
                     {
                       evc_bw_profile_count++;
                       break;
                     }    
                 } 
             
             if (evc_bw_profile_count == PAL_FALSE)
               {
                 bw_profile->egress_bw_profile_status = BW_PROFILE_NOT_CONFIGURED;
                 bw_profile->egress_bw_profile_type = BW_PROFILE_NOT_CONFIGURED;
               }
               
           }

        }
    }

  return RESULT_OK;
}

/**@brief  - API to activate/inactivate the bw profile per EVC.
 * @param ifp - UNI for which the bw profile parameters need to be set.
 * @param bw_profile - Specifies the evc for which the bw profile 
 *                     parameters need to be activated/inactivated.
 * @ bw_profile_type - Specified ingress/egress bw profiling type.
 * @ bw_profile_status - Specifies activate/inactivate option configured.
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */
s_int32_t
nsm_set_evc_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int8_t bw_profile_type,
    u_int8_t bw_profile_status)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *nsm_bw_profile = NULL;
#ifdef HAVE_ELMID
  struct nsm_msg_bw_profile evc_bw_msg;
#endif /* HAVE_ELMID */
  s_int32_t ret;
  ret = NSM_L2_ERR_INVALID_ARG;

  if (ifp == NULL)
    {
      return NSM_VLAN_ERR_IFP_NOT_BOUND; 
    }

  if (bw_profile == NULL)
    {
      return ret;
    }
  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;

  /* IF the ports is not a CEP, then return an error */
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }

  if (zif->nsm_bw_profile)
    {
      nsm_bw_profile = zif->nsm_bw_profile;
    }
  else
    return NSM_L2_ERR_INVALID_ARG;

 if (bw_profile_type == NSM_INGRESS_POLICING)
   {
     if (nsm_bw_profile && (nsm_bw_profile->ingress_bw_profile_status 
                                    == BW_PROFILE_CONFIGURED))
       {
         /* IF the BW Profile per UNI is configured, then return an error */
         if (nsm_bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }
         /* IF the bw profile per COS is configured, the  return an error */
         if (bw_profile->ingress_bw_profile_type == NSM_EVC_COS_BW_PROFILE)
           {
             return NSM_ERR_BW_PROFILE_PER_COS_CONFIGURED;
           }
       }
     /* IF there is no bw profile per EVC configured, then return an error */
     if (!bw_profile->ingress_evc_bw_profile)
       {
         return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
       }
     
     if (bw_profile_status == PAL_TRUE) 
       {
         /* section 7.11.1 MEF 10.1, If CIR is greater than zero,
          *  CBS MUST be greater than or equal to the largest
          *  Maximum Transmission Unit size among all of the EVCs
          *   that the Bandwidth Profile applies to.
          *   In PacOS, the max MTU size on Linux can be 1500, so the CBS
          *    should be greater than or equal to 1500 */
         if (bw_profile->ingress_evc_bw_profile->comm_info_rate >
             PAL_FALSE)
           {
             if (bw_profile->ingress_evc_bw_profile->comm_burst_size <
                 1500)
               return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
           }

         if (bw_profile->ingress_evc_bw_profile->excess_info_rate > PAL_FALSE)
           {
             if (bw_profile->ingress_evc_bw_profile->excess_burst_size <
                 1500)
               return NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED;
           }

         /* Committed Information Rate should not be greater than
          *  Excess information Rate
          */
         if (bw_profile->ingress_evc_bw_profile->excess_info_rate !=
             NSM_DEFAULT_VAL)
           {
             if (bw_profile->ingress_evc_bw_profile->comm_info_rate >
                 bw_profile->ingress_evc_bw_profile->excess_info_rate)
               return NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED;
           }

         /* Committed Burst Size should not be greater than
          * Excess Burst Size
          */
         if (bw_profile->ingress_evc_bw_profile->excess_burst_size != 
             NSM_DEFAULT_VAL)
           {
             if (bw_profile->ingress_evc_bw_profile->comm_burst_size >
                 bw_profile->ingress_evc_bw_profile->excess_burst_size)
               return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
           }

         /* After all the required checks, the Ingress Bandwidth Profile
          * per UNI will be activated
          */
         bw_profile->ingress_evc_bw_profile->active = PAL_TRUE;

#ifdef HAVE_ELMID
         pal_mem_set (&evc_bw_msg, 0, sizeof (struct nsm_msg_bw_profile));

         /* send msg to elmi regarding evc bw profile */
         NSM_SET_CTYPE(evc_bw_msg.cindex, NSM_ELMI_CTYPE_EVC_BW);
         
         evc_bw_msg.ifindex = br_port->ifindex;
         evc_bw_msg.evc_ref_id = bw_profile->svlan;
         evc_bw_msg.cir = bw_profile->ingress_evc_bw_profile->comm_info_rate;
         evc_bw_msg.cbs = bw_profile->ingress_evc_bw_profile->comm_burst_size;
         evc_bw_msg.eir = bw_profile->ingress_evc_bw_profile->excess_info_rate;
         evc_bw_msg.ebs = bw_profile->ingress_evc_bw_profile->excess_burst_size;
         evc_bw_msg.cf = bw_profile->ingress_evc_bw_profile->coupling_flag;
         evc_bw_msg.cm = bw_profile->ingress_evc_bw_profile->color_mode;

         nsm_server_elmi_send_bw (br_port, &evc_bw_msg, 
                                  NSM_MSG_ELMI_EVC_BW_ADD);

#endif /* HAVE_ELMID */

       }
     else
       {
         /* IF the bw profile pe EVC is inactivated */
         if (bw_profile->ingress_evc_bw_profile->active == PAL_TRUE)
           {
             bw_profile->ingress_evc_bw_profile->active = PAL_FALSE;

#ifdef HAVE_ELMID
             pal_mem_set (&evc_bw_msg, 0, sizeof (struct nsm_msg_bw_profile));

             NSM_SET_CTYPE(evc_bw_msg.cindex, NSM_ELMI_CTYPE_EVC_BW);
             evc_bw_msg.ifindex = br_port->ifindex;
             evc_bw_msg.evc_ref_id = bw_profile->svlan;

             /* send msg to elmi to delete evc bw profile */
             nsm_server_elmi_send_bw (br_port, &evc_bw_msg, 
                                      NSM_MSG_ELMI_EVC_BW_DEL);
#endif /* HAVE_ELMID */
           }
         else
           return NSM_ERR_BW_PROFILE_NOT_ACTIVE;
       }
   }
 else
   {

     if (nsm_bw_profile && (nsm_bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED))
       {
         /* IF the BW Profile per UNI is configured, then return an error */
         if (nsm_bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }

         /* IF the bw profile per COS is configured, the  return an error */
         if (bw_profile->egress_bw_profile_type == NSM_EVC_COS_BW_PROFILE)
           {
             return NSM_ERR_BW_PROFILE_PER_COS_CONFIGURED;
           }
       }

     /* IF there is no bw profile per EVC configured, then return an error */
     if (bw_profile->egress_evc_bw_profile == NULL)
       {
         return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
       }


     if (bw_profile_status == PAL_TRUE) 
       {
         /* section 7.11.1 MEF 10.1, If CIR is greater than zero,
          *  CBS MUST be greater than or equal to the largest
          *  Maximum Transmission Unit size among all of the EVCs
          *   that the Bandwidth Profile applies to.
          *   In PacOS, the max MTU size on Linux can be 1500, so the CBS
          *    should be greater than or equal to 1500 */
         if (bw_profile->egress_evc_bw_profile->comm_info_rate >
             PAL_FALSE)
           {
             if (bw_profile->egress_evc_bw_profile->comm_burst_size <
                 1500)
               return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
           }

         if (bw_profile->egress_evc_bw_profile->excess_info_rate > PAL_FALSE)
           {
             if (bw_profile->egress_evc_bw_profile->excess_burst_size <
                 1500)
               return NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED;
           }

         /* Committed Information Rate should not be greater than
          *  Excess information Rate
          */
         if (bw_profile->egress_evc_bw_profile->excess_info_rate 
             != NSM_DEFAULT_VAL)
           {
             if (bw_profile->egress_evc_bw_profile->comm_info_rate >
                 bw_profile->egress_evc_bw_profile->excess_info_rate)
               return NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED;
           }

         /* Committed Burst Size should not be greater than
          * Excess Burst Size
          */
         if (bw_profile->egress_evc_bw_profile->excess_burst_size
             != NSM_DEFAULT_VAL)
           {
             if (bw_profile->egress_evc_bw_profile->comm_burst_size >
                 bw_profile->egress_evc_bw_profile->excess_burst_size)
               return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
           }

         /* After all the required checks, the Ingress Bandwidth Profile
          * per UNI will be activated
          */
         bw_profile->egress_evc_bw_profile->active = PAL_TRUE;


       }
     else
       {
         /* IF the bw profile pe EVC is inactivated */
         if (bw_profile->egress_evc_bw_profile->active == PAL_TRUE)
           bw_profile->egress_evc_bw_profile->active = PAL_FALSE;
         else
           return NSM_ERR_BW_PROFILE_NOT_ACTIVE;
       }
   }

  return RESULT_OK;
}

/**@brief  - API to configure bw profile parameters per COS.
 * @param ifp - UNI for which the bw profile parameters need to be set.
 * @param bw_profile - Specifies the evc for which the bw profile 
 *                     parameters need to be set.
 * @param cos - Cos values for which bw profile parameters need to be set.
 * @param cir - Committed information rate per cos.
 * @param cbs - Committed burst size per cos.
 * @param eir - Excess information rate per cos.
 * @param ebs - Excess burst size per cos.
 * @coupling_flag - Enable/disable coupling flag per cos.
 * @ color_mode - Variable to set color-aware/color-blind mode per cos.
 * @ bw_profile_type - Specified ingress/egress bw profiling type.
 * @ bw_profile_parameter - Indicates the bw profile parameter set.
 * @return RESULT_OK - For Success.
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t
nsm_evc_cos_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int8_t *cos, u_int32_t cir,
    u_int32_t cbs, u_int32_t eir, u_int32_t ebs, u_int8_t coupling_flag,
    u_int8_t color_mode, u_int8_t bw_profile_type,
    u_int8_t bw_profile_parameter)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *nsm_bw_profile = NULL;
  s_int32_t ret;
  u_int8_t i = PAL_FALSE;
  u_int8_t j = PAL_FALSE;
  u_int8_t cos_check = PAL_FALSE;
  u_int8_t temp_cosid = PAL_FALSE;
  u_int8_t cosid_configured = PAL_FALSE;
  ret = NSM_L2_ERR_INVALID_ARG;

  if (ifp == NULL)
    {
      return NSM_VLAN_ERR_IFP_NOT_BOUND; 
    }

 if (bw_profile == NULL)
   {
     return NSM_L2_ERR_INVALID_ARG;
   }

 if (cos == NULL)
   {
     return NSM_L2_ERR_INVALID_ARG;
   }

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
     return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;
  
  /* IF the port is not a CEP, then return an error */ 
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }

 if (zif->nsm_bw_profile)
   {
     nsm_bw_profile = zif->nsm_bw_profile;
   }

 if (bw_profile_type == NSM_INGRESS_POLICING)
   { 
     if (nsm_bw_profile && (nsm_bw_profile->ingress_bw_profile_status
                              == BW_PROFILE_CONFIGURED))
       {
         /* IF bw profile per UNI is configured, then return an error */
         if (nsm_bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }
         /* IF the bw profile per EVC is configured, then return an error */
         if (bw_profile->ingress_bw_profile_type == NSM_EVC_BW_PROFILE_PER_EVC)
           {
             return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
           }
       }
   }
 else
   {
     if (nsm_bw_profile && (nsm_bw_profile->egress_bw_profile_status
                               == BW_PROFILE_CONFIGURED))
       {
         /* IF bw profile per UNI is configured, then return an error */
         if (nsm_bw_profile->egress_bw_profile_type != NSM_BW_PROFILE_PER_EVC)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }

         /* IF the bw profile per EVC is configured, then return an error */
         if (bw_profile->egress_bw_profile_type == NSM_EVC_BW_PROFILE_PER_EVC)
           {
             return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
           }
       }
   }



 if (bw_profile_type == NSM_INGRESS_POLICING)
   {
     /* Check if the cos values configured are already configured as
      * part of another cos id */
     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             /* If the configured cos, is already part of bw profile per cos,
              * then check if all the other cos values given from the user
              * have the same bw profile per COS configured. 
              * Else return an error
              */
             if (bw_profile->ingress_CoS_id [i] != PAL_FALSE)
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->ingress_CoS_id [i];
                     cos_check = PAL_TRUE;
                     cosid_configured = PAL_TRUE;
                   }
                 else
                   {
                     if (temp_cosid != bw_profile->ingress_CoS_id [i])
                       return NSM_ERR_COS_ALREADY_CONFIGURED;
                     cosid_configured = PAL_TRUE;
                   }

               }
             else
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->ingress_CoS_id [i];
                     cos_check = PAL_TRUE;
                   }
                 if (temp_cosid != bw_profile->ingress_CoS_id [i])
                   return NSM_ERR_COS_ALREADY_CONFIGURED;
                 if (temp_cosid != NSM_DEFAULT_VAL)
                   cosid_configured = PAL_TRUE;
               }
           }
       }
    
    /* A cos id is being assigned to the cos values configured for bw profile
     * per COS
     */
    if (cosid_configured == PAL_FALSE)
      for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
        {
          if (bw_profile->num_of_ingress_cos_id [i] == PAL_FALSE)
            {
              bw_profile->num_of_ingress_cos_id [i] = PAL_TRUE;

              for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)
                {
                  if (cos [j] == PAL_TRUE)
                    {
                      bw_profile->ingress_CoS_id [j] =
                        i + 1;

                    }
                }
              break;
            }
        }

    for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
      {
        if (cos [i] == PAL_TRUE)
          {
            /* Allocate memory for cos bw profile */
            if (bw_profile->ingress_evc_cos_bw_profile[i] == NULL)
              {
                bw_profile->ingress_evc_cos_bw_profile[i] = XCALLOC (
                    MTYPE_NSM_INGRESS_EVC_COS_BW_PROFILE, 
                    sizeof (struct nsm_bw_profile_parameters));

                if (bw_profile->ingress_evc_cos_bw_profile[i] == NULL)
                  {
                    return NSM_L2_ERR_MEM;
                  }
              }/*if (bw_profile->ingress_evc_cos_bw_profile[i] == NULL)*/
            else
              {
                /* If cos bw profile configured is activated, then user 
                 * cannot modify the bw profile parameters configured */
                if (bw_profile->ingress_evc_cos_bw_profile[i]->active ==
                    PAL_TRUE)
                  return NSM_ERR_BW_PROFILE_ACTIVE;
              }
            
            /* Update the bw profile parameters per cos */
            switch (bw_profile_parameter)
              {
              case NSM_BW_PARAMETER_CIR:
                bw_profile->ingress_evc_cos_bw_profile[i]->comm_info_rate =
                  cir;
                break;
              case NSM_BW_PARAMETER_CBS:
                bw_profile->ingress_evc_cos_bw_profile[i]->comm_burst_size =
                  cbs;
                break;
              case NSM_BW_PARAMETER_EIR:
                bw_profile->ingress_evc_cos_bw_profile[i]->excess_info_rate =
                  eir;
                break;
              case NSM_BW_PARAMETER_EBS:
                bw_profile->ingress_evc_cos_bw_profile[i]->excess_burst_size =
                  ebs;
                break;
              case NSM_BW_PARAMETER_COUPLING_FLAG:
                bw_profile->ingress_evc_cos_bw_profile[i]->coupling_flag =
                  coupling_flag;
                break;
              case NSM_BW_PARAMETER_COLOR_MODE:
                bw_profile->ingress_evc_cos_bw_profile[i]->color_mode =
                  color_mode;
                break;
              default:
                return NSM_L2_ERR_INVALID_ARG;
              }
          }
      }
    /* After updating the parameters, update the profile types
     * and status
     */
    nsm_bw_profile->ingress_bw_profile_status = BW_PROFILE_CONFIGURED;
    nsm_bw_profile->ingress_bw_profile_type = NSM_BW_PROFILE_PER_EVC;
    bw_profile->ingress_bw_profile_type = NSM_EVC_COS_BW_PROFILE; 

      

   }
 else
   {
     /* Check if the cos values configured are already configured as
      * part of another cos id */
     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             /* If the configured cos, is already part of bw profile per cos,
              * then check if all the other cos values given from the user
              * have the same bw profile per COS configured. 
              * Else return an error
              */
             if (bw_profile->egress_CoS_id [i] != PAL_FALSE)
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->egress_CoS_id [i];
                     cos_check = PAL_TRUE;
                     cosid_configured = PAL_TRUE;
                   }
                 else
                   {
                     if (temp_cosid != bw_profile->egress_CoS_id [i])
                       return NSM_ERR_COS_ALREADY_CONFIGURED;
                     cosid_configured = PAL_TRUE;
                   }
               }
             else
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->egress_CoS_id [i];
                     cos_check = PAL_TRUE;
                   }

                 if (temp_cosid != bw_profile->egress_CoS_id [i])
                   return NSM_ERR_COS_ALREADY_CONFIGURED;
                 if (temp_cosid != NSM_DEFAULT_VAL)
                   cosid_configured = PAL_TRUE;
               }

           }
       }

    /* A cos id is being assigned to the cos values configured for bw profile
     * per COS
     */
     if (cosid_configured == PAL_FALSE)
       for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
         {
           if (bw_profile->num_of_egress_cos_id [i] == PAL_FALSE)
             {
               bw_profile->num_of_egress_cos_id [i] = PAL_TRUE;

               for (j = 0; j < NSM_MAX_COS_PER_EVC; j++)
                 {
                   if (cos [j] == PAL_TRUE)
                     {
                       bw_profile->egress_CoS_id [j] =
                         i + 1;
                     }
                 }
               break;
             }
         }

     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
            /* Allocate memory for cos bw profile */
             if (bw_profile->egress_evc_cos_bw_profile[i] == NULL)
               {
                 bw_profile->egress_evc_cos_bw_profile[i] = XCALLOC (
                     MTYPE_NSM_EGRESS_EVC_COS_BW_PROFILE, 
                     sizeof (struct nsm_bw_profile_parameters));

                 if (bw_profile->egress_evc_cos_bw_profile[i] == NULL)
                   {
                     return NSM_L2_ERR_MEM;
                   }
               }/*if (bw_profile->egress_evc_cos_bw_profile[i] == NULL)*/
             else
               {
                 /* If cos bw profile configured is activated, then user 
                 * cannot modify the bw profile parameters configured */
                if (bw_profile->egress_evc_cos_bw_profile[i]->active ==
                     PAL_TRUE)
                   return NSM_ERR_BW_PROFILE_ACTIVE;
               }

            /* Update the bw profile parameters per cos */
             switch (bw_profile_parameter)
               {
               case NSM_BW_PARAMETER_CIR:
                 bw_profile->egress_evc_cos_bw_profile[i]->comm_info_rate =
                   cir;
                 break;
               case NSM_BW_PARAMETER_CBS:
                 bw_profile->egress_evc_cos_bw_profile[i]->comm_burst_size =
                   cbs;
                 break;
               case NSM_BW_PARAMETER_EIR:
                 bw_profile->egress_evc_cos_bw_profile[i]->excess_info_rate =
                   eir;
                 break;
               case NSM_BW_PARAMETER_EBS:
                 bw_profile->egress_evc_cos_bw_profile[i]->excess_burst_size =
                   ebs;
                 break;
               case NSM_BW_PARAMETER_COUPLING_FLAG:
                 bw_profile->egress_evc_cos_bw_profile[i]->coupling_flag =
                   coupling_flag;
                 break;
               case NSM_BW_PARAMETER_COLOR_MODE:
                 bw_profile->egress_evc_cos_bw_profile[i]->color_mode =
                   color_mode;
                 break;
               default:
                 return NSM_L2_ERR_INVALID_ARG;
               }/* switch (bw_profile_parameter)*/
           }/* if (cos [i] == PAL_TRUE)*/
         
       }/* for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)*/

     /* After updating the parameters, update the profile types
     * and status
     */
     nsm_bw_profile->egress_bw_profile_status = BW_PROFILE_CONFIGURED;
     nsm_bw_profile->egress_bw_profile_type = NSM_BW_PROFILE_PER_EVC;
     bw_profile->egress_bw_profile_type = NSM_EVC_COS_BW_PROFILE; 

   }/*else */

  return RESULT_OK;
}

/**@brief  - API to delete bw profile parameters per COS.
 * @param ifp - UNI for which the bw profile parameters need to be set.
 * @param bw_profile - Specifies the evc for which the bw profile 
 *                     parameters need to be set.
 * @param cos - Cos values for which bw profile parameters need to be set.
 * @ bw_profile_type - Specified ingress/egress bw profiling type.
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t 
nsm_delete_evc_cos_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile,  u_int8_t *cos,
    u_int8_t bw_profile_type)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *nsm_bw_profile = NULL;
  struct nsm_uni_evc_bw_profile *evc_profile = NULL;
  struct listnode *evc_node = NULL;
  s_int32_t ret;
  u_int8_t i = PAL_FALSE;
  u_int8_t cos_check = PAL_FALSE;
  u_int8_t temp_cosid = PAL_FALSE;
  u_int8_t cos_bw_profile_count = PAL_FALSE;
  ret = NSM_L2_ERR_INVALID_ARG;

  if (ifp == NULL)
    {
      return NSM_VLAN_ERR_IFP_NOT_BOUND; 
    }

  if (bw_profile == NULL)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  if (cos == NULL)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;

  /* IF the port is not a CEP, then return an error */ 
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }

  if (zif->nsm_bw_profile)
    {
      nsm_bw_profile = zif->nsm_bw_profile;
    }
  else
    return NSM_L2_ERR_INVALID_ARG;

  if (bw_profile_type == NSM_INGRESS_POLICING)
    { 
      if (nsm_bw_profile && (nsm_bw_profile->ingress_bw_profile_status
                                       == BW_PROFILE_CONFIGURED))
        {
          /* IF bw profile per UNI is configured, then return an error */
          if (nsm_bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
            {
              return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
            }

         /* IF the bw profile per EVC is configured, then return an error */
          if (bw_profile->ingress_bw_profile_type == NSM_EVC_BW_PROFILE_PER_EVC)
            {
              return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
            }
        }
      else
        {
         /* Set the bw profile parameters to default values */
          if (nsm_bw_profile->uni_evc_bw_profile_list->count == PAL_TRUE)
            {
              nsm_bw_profile->ingress_bw_profile_status =
                BW_PROFILE_NOT_CONFIGURED;
              nsm_bw_profile->ingress_bw_profile_type = PAL_FALSE;
            }
          bw_profile->ingress_bw_profile_type = PAL_FALSE; 
        }
    }
  else
    {
      if (nsm_bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED)
        {
          /* IF bw profile per UNI is configured, then return an error */
          if (nsm_bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
            {
              return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
            }

         /* IF the bw profile per EVC is configured, then return an error */
          if (bw_profile->egress_bw_profile_type == NSM_EVC_BW_PROFILE_PER_EVC)
            {
              return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
            }
        }
      else
        {
          /* set the bw profile types and status to default values */
          if (nsm_bw_profile->uni_evc_bw_profile_list->count == PAL_TRUE)
            {
              nsm_bw_profile->egress_bw_profile_status =
                BW_PROFILE_NOT_CONFIGURED;
              nsm_bw_profile->egress_bw_profile_type = PAL_FALSE;
            }
          bw_profile->egress_bw_profile_type = PAL_FALSE; 
        }
    }


  if (bw_profile_type == NSM_INGRESS_POLICING)
    {
     /* Check if the cos values configured are already configured as
      * part of another cos id */
      for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
        {
          if (cos [i] == PAL_TRUE)
            {
             /* If the configured cos, is already part of bw profile per cos,
              * then check if all the other cos values given from the user
              * have the same bw profile per COS configured. 
              * Else return an error
              */
              if (bw_profile->ingress_CoS_id [i] != PAL_FALSE)
                {
                  if (cos_check == PAL_FALSE)
                    {
                      temp_cosid = bw_profile->ingress_CoS_id [i];
                      cos_check = PAL_TRUE;
                    }
                  else
                    {
                      if (temp_cosid != bw_profile->ingress_CoS_id [i])
                        return NSM_ERR_COS_ALREADY_CONFIGURED;
                    }
                }
              else
                {
                  return NSM_ERR_COS_ALREADY_CONFIGURED;
                }
            }
        }
      /* After the above check free the bw profile per cos */
      for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
        {
          if (cos [i] == PAL_TRUE)
            {
              /* IF the bandwidth profile is active, no permission to delete*/
              if (bw_profile->ingress_evc_cos_bw_profile[i]->active ==
                  PAL_TRUE)
                return NSM_ERR_BW_PROFILE_ACTIVE;
              
              if (bw_profile->ingress_evc_cos_bw_profile[i] != NULL)
                {
                  XFREE (MTYPE_NSM_INGRESS_EVC_COS_BW_PROFILE, 
                      bw_profile->ingress_evc_cos_bw_profile[i]);

                  bw_profile->ingress_evc_cos_bw_profile[i] = NULL;
                  bw_profile->ingress_CoS_id[i] = NSM_DEFAULT_VAL;

                }/*if (bw_profile->ingress_evc_cos_bw_profile[i] == NULL)*/

            }
        }
     
      /* Set the cos id to false, so that new cos values configured
       * can use this cos id
       */
      bw_profile->num_of_ingress_cos_id [temp_cosid - 1] = PAL_FALSE;
      
      for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
        {
          if (bw_profile->num_of_ingress_cos_id [i] == PAL_TRUE)
            {
              cos_bw_profile_count++;
              break;
            }
        } 
      if (cos_bw_profile_count == PAL_FALSE)
        bw_profile->ingress_bw_profile_type = 0;

      cos_bw_profile_count = PAL_FALSE; 
            
      /* Check if there are any other bw profile per evc cos
       * on this interface to unset the status flag
       */
      if (nsm_bw_profile->ingress_bw_profile_type ==
           NSM_BW_PROFILE_PER_EVC)
        {
          if (nsm_bw_profile->uni_evc_bw_profile_list)
            LIST_LOOP (nsm_bw_profile->uni_evc_bw_profile_list, evc_profile,
                       evc_node)
              {
                for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                  {
                    if ((evc_profile->num_of_ingress_cos_id [i] == PAL_TRUE)
                        || (evc_profile->ingress_bw_profile_type == 
                            NSM_EVC_BW_PROFILE_PER_EVC))
                      {
                        cos_bw_profile_count++;
                        break;
                      }
                  }
                if (cos_bw_profile_count > 0)
                  break;
              }
        }

      if (cos_bw_profile_count == PAL_FALSE)
        {
          nsm_bw_profile->ingress_bw_profile_status = BW_PROFILE_NOT_CONFIGURED;
          nsm_bw_profile->ingress_bw_profile_type = BW_PROFILE_NOT_CONFIGURED;
        }
    }
  else
    {
      /* Check if the cos values configured are already configured as
       * part of another cos id */
      for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
        {
          if (cos [i] == PAL_TRUE)
            {
              /* If the configured cos, is already part of bw profile per cos,
               * then check if all the other cos values given from the user
               * have the same bw profile per COS configured. 
               * Else return an error
               */
              if (bw_profile->egress_CoS_id [i] != PAL_FALSE)
                {
                  if (cos_check == PAL_FALSE)
                    {
                      temp_cosid = bw_profile->egress_CoS_id [i];
                      cos_check = PAL_TRUE;
                    }
                  else
                    {
                      if (temp_cosid != bw_profile->egress_CoS_id [i])
                        return NSM_ERR_COS_ALREADY_CONFIGURED;
                    }
                }
              else
                {
                  return NSM_ERR_COS_ALREADY_CONFIGURED;
                }
            }
        }
      /* After the above check, free the cos bw profile memory */
      for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
        {
          if (cos [i] == PAL_TRUE)
            {
              /* IF the bandwidth profile is active, no permission to delete*/
              if (bw_profile->egress_evc_cos_bw_profile[i]->active ==
                  PAL_TRUE)
                return NSM_ERR_BW_PROFILE_ACTIVE;

              if (bw_profile->egress_evc_cos_bw_profile[i] != NULL)
                {
                  XFREE (MTYPE_NSM_INGRESS_EVC_COS_BW_PROFILE, 
                      bw_profile->egress_evc_cos_bw_profile[i]);

                  bw_profile->egress_evc_cos_bw_profile[i] = NULL;
                  bw_profile->egress_CoS_id[i] = NSM_DEFAULT_VAL;

                }/*if (bw_profile->egress_evc_cos_bw_profile[i] == NULL)*/

            }
        }

      bw_profile->num_of_egress_cos_id [temp_cosid - 1] = PAL_FALSE;
      
      for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
        {
          if (bw_profile->num_of_egress_cos_id [i] == PAL_TRUE)
            {
              cos_bw_profile_count++;
              break;
            }
        }
      if (cos_bw_profile_count == PAL_FALSE)
        bw_profile->egress_bw_profile_type = 0;
      
      cos_bw_profile_count = PAL_FALSE;
      
      /* Check if there are any other bw profile per evc cos
       * on this interface to unset the status flag
       */
      if (nsm_bw_profile->egress_bw_profile_type ==
           NSM_BW_PROFILE_PER_EVC)
        {
          if (nsm_bw_profile->uni_evc_bw_profile_list)
            LIST_LOOP (nsm_bw_profile->uni_evc_bw_profile_list, evc_profile,
                       evc_node)
              {
                for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
                  {
                    if ((evc_profile->num_of_egress_cos_id [i] == PAL_TRUE)
                         || (evc_profile->egress_bw_profile_type ==
                            NSM_EVC_BW_PROFILE_PER_EVC))
                      {
                        cos_bw_profile_count++;
                        break;
                      }
                  }
                if (cos_bw_profile_count > 0)
                  break;
              }
        }
        
      if (cos_bw_profile_count == PAL_FALSE)
        {
          nsm_bw_profile->egress_bw_profile_status = BW_PROFILE_NOT_CONFIGURED;
          nsm_bw_profile->egress_bw_profile_type = BW_PROFILE_NOT_CONFIGURED;
        }


    }/*else */

  return RESULT_OK;

}

/**@brief  - API to activate/inactivate cos bw profile.
 * @param ifp - UNI for which the bw profile parameters need to be set.
 * @param bw_profile - Specifies the evc for which the bw profile 
 *                     parameters need to be set.
 * @param cos - Cos values for which bw profile parameters need to be set.
 * @param bw_profile_type - specified ingress/egress bw profiling type.
 * @param bw_profile_status - Variable specifies activate/inactivate
 *                            configured.
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t
nsm_set_evc_cos_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int8_t *cos, 
    u_int8_t bw_profile_type, u_int8_t bw_profile_status)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *nsm_bw_profile = NULL;
  s_int32_t ret;
  u_int8_t i = PAL_FALSE;
  u_int8_t cos_check = PAL_FALSE;
  u_int8_t temp_cosid = PAL_FALSE;
  ret = NSM_L2_ERR_INVALID_ARG;

#ifdef HAVE_ELMID
  struct nsm_msg_bw_profile msg;
  u_int8_t first = PAL_FALSE;
  u_int8_t msg_cos_value = 0;
  u_int8_t msg_cos_id = 0;
  u_int8_t msg_cos_pri = 0;
#endif /* HAVE_ELMID */

  if (ifp == NULL)
    {
      return NSM_VLAN_ERR_IFP_NOT_BOUND; 
    }

 if (bw_profile == NULL)
   {
     return NSM_L2_ERR_INVALID_ARG;
   }

 if (cos == NULL)
   {
     return NSM_L2_ERR_INVALID_ARG;
   }

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;
 
  /* IF the port is not a CEP, then return an error */ 
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }

 if (zif->nsm_bw_profile)
   {
     nsm_bw_profile = zif->nsm_bw_profile;
   }
  else
    return NSM_L2_ERR_INVALID_ARG;

 if (bw_profile_type == NSM_INGRESS_POLICING)
   { 
     if (nsm_bw_profile && (nsm_bw_profile->ingress_bw_profile_status 
                              == BW_PROFILE_CONFIGURED))
       {
         /* IF bw profile per UNI is configured, then return an error */
         if (nsm_bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }

         /* IF the bw profile per EVC is configured, then return an error */
         if (bw_profile->ingress_bw_profile_type == NSM_EVC_BW_PROFILE_PER_EVC)
           {
             return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
           }
       }
   }
 else
   {
     if (nsm_bw_profile && (nsm_bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED))
       {
         /* IF bw profile per UNI is configured, then return an error */
         if (nsm_bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
           {
             return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED;
           }

         /* IF the bw profile per EVC is configured, then return an error */
         if (bw_profile->egress_bw_profile_type == NSM_EVC_BW_PROFILE_PER_EVC)
           {
             return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED;
           }
       }
   }

 if (bw_profile_type == NSM_INGRESS_POLICING)
   {
     /* Check if the cos values configured are already configured as
      * part of another cos id */
     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
            /* If the configured cos, is already part of bw profile per cos,
              * then check if all the other cos values given from the user
              * have the same bw profile per COS configured. 
              * Else return an error
              */
             if (bw_profile->ingress_CoS_id [i] != PAL_FALSE)
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->ingress_CoS_id [i];
                     cos_check = PAL_TRUE;
                   }
                 else
                   {
                     if (temp_cosid != bw_profile->ingress_CoS_id [i])
                       return NSM_ERR_COS_ALREADY_CONFIGURED;
                   }

               }
             else
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->ingress_CoS_id [i];
                     cos_check = PAL_TRUE;
                   }

                 if (temp_cosid != bw_profile->ingress_CoS_id [i])
                   return NSM_ERR_COS_ALREADY_CONFIGURED;
               }
           }
       }
     /* NULL for the cos values entered */
     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             if (bw_profile->ingress_evc_cos_bw_profile[i] == NULL)
               {
                 return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
               }
           }
       }

     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             /* Activate cos bw profile */
             if (bw_profile_status == PAL_TRUE) 
               {
                 /* section 7.11.1 MEF 10.1, If CIR is greater than zero,
                  *  CBS MUST be greater than or equal to the largest
                  *  Maximum Transmission Unit size among all of the EVCs
                  *   that the Bandwidth Profile applies to.
                  *   In PacOS, the max MTU size on Linux can be 1500, so the CBS
                  *    should be greater than or equal to 1500 */
                 if (bw_profile->ingress_evc_cos_bw_profile[i]->comm_info_rate >
                     PAL_FALSE)
                   {
                     if (bw_profile->ingress_evc_cos_bw_profile[i]->
                         comm_burst_size < 1500)
                       return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
                   }

                 if (bw_profile->ingress_evc_cos_bw_profile[i]->excess_info_rate
                     > PAL_FALSE)
                   {
                     if (bw_profile->ingress_evc_cos_bw_profile[i]->
                         excess_burst_size < 1500)
                       return NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED;
                   }

                 /* Committed Information Rate should not be greater than
                  *  Excess information Rate
                  */
                 if (bw_profile->ingress_evc_cos_bw_profile[i]->excess_info_rate
                     != NSM_DEFAULT_VAL)
                   {
                     if (bw_profile->ingress_evc_cos_bw_profile[i]->
                         comm_info_rate >
                         bw_profile->ingress_evc_cos_bw_profile[i]->
                         excess_info_rate)
                       return NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED;
                   }

                 /* Committed Burst Size should not be greater than
                  * Excess Burst Size
                  */
                 if (bw_profile->ingress_evc_cos_bw_profile[i]->excess_burst_size
                     != NSM_DEFAULT_VAL)
                   {
                     if (bw_profile->ingress_evc_cos_bw_profile[i]->
                         comm_burst_size
                         > bw_profile->ingress_evc_cos_bw_profile[i]->
                         excess_burst_size)
                       return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
                   }

                 /* After all the required checks, the Ingress Bandwidth Profile
                  * per UNI will be activated
                  */
                 bw_profile->ingress_evc_cos_bw_profile[i]->active = PAL_TRUE;

               }
             else
               {
                 if (bw_profile->ingress_evc_cos_bw_profile[i]->active == PAL_TRUE)
                   {
                     bw_profile->ingress_evc_cos_bw_profile[i]->active = PAL_FALSE;
#ifdef HAVE_ELMID
                     pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bw_profile));

                     NSM_SET_CTYPE(msg.cindex, NSM_ELMI_CTYPE_EVC_COS_BW);
                   
                     msg.cos_id = bw_profile->ingress_CoS_id [i]; 

                     msg.ifindex = br_port->ifindex;

                     /* Send msg to elmi to delete evc bw profile */
                     nsm_server_elmi_send_bw (br_port, &msg, 
                                              NSM_MSG_ELMI_EVC_COS_BW_DEL);
#endif /* HAVE_ELMID */
                   }
                 else
                   return NSM_ERR_BW_PROFILE_NOT_ACTIVE;
               }

           }
       }
   }
 else
   {
     /* Check if the cos values configured are already configured as
      * part of another cos id */
     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             /* If the configured cos, is already part of bw profile per cos,
              * then check if all the other cos values given from the user
              * have the same bw profile per COS configured. 
              * Else return an error
              */

             if (bw_profile->egress_CoS_id [i] != PAL_FALSE)
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->egress_CoS_id [i];
                     cos_check = PAL_TRUE;
                   }
                 else
                   {
                     if (temp_cosid != bw_profile->egress_CoS_id [i])
                       return NSM_ERR_COS_ALREADY_CONFIGURED;
                   }
               }
             else
               {
                 if (cos_check == PAL_FALSE)
                   {
                     temp_cosid = bw_profile->ingress_CoS_id [i];
                     cos_check = PAL_TRUE;
                   }

                 if (temp_cosid != bw_profile->egress_CoS_id [i])
                   return NSM_ERR_COS_ALREADY_CONFIGURED;
               }

           }
       }
     /* NULL check for the cos values configured */
     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             if (bw_profile->egress_evc_cos_bw_profile[i] == NULL)
               {
                 return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
               }
           }
       }

     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             /* Activate the cos bw profile */
             if (bw_profile_status == PAL_TRUE) 
               {
                 /* section 7.11.1 MEF 10.1, If CIR is greater than zero,
                  *  CBS MUST be greater than or equal to the largest
                  *  Maximum Transmission Unit size among all of the EVCs
                  *   that the Bandwidth Profile applies to.
                  *   In PacOS, the max MTU size on Linux can be 1500, so the CBS
                  *    should be greater than or equal to 1500 */
                 if (bw_profile->egress_evc_cos_bw_profile[i]->comm_info_rate >
                     PAL_FALSE)
                   {
                     if (bw_profile->egress_evc_cos_bw_profile[i]->
                         comm_burst_size < 1500)
                       return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
                   }

                 if (bw_profile->egress_evc_cos_bw_profile[i]->excess_info_rate
                     > PAL_FALSE)
                   {
                     if (bw_profile->egress_evc_cos_bw_profile[i]->
                         excess_burst_size < 1500)
                       return NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED;
                   }

                 /* Committed Information Rate should not be greater than
                  *  Excess information Rate
                  */
                 if (bw_profile->egress_evc_cos_bw_profile[i]->excess_info_rate
                     != NSM_DEFAULT_VAL)
                   {
                     if (bw_profile->egress_evc_cos_bw_profile[i]->
                         comm_info_rate >
                         bw_profile->egress_evc_cos_bw_profile[i]->
                         excess_info_rate)
                       return NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED;
                   }

                 /* Committed Burst Size should not be greater than
                  * Excess Burst Size
                  */
                 if (bw_profile->egress_evc_cos_bw_profile[i]->
                     excess_burst_size != NSM_DEFAULT_VAL)
                   {
                     if (bw_profile->egress_evc_cos_bw_profile[i]->
                         comm_burst_size
                         > bw_profile->egress_evc_cos_bw_profile[i]->
                         excess_burst_size)
                       return NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED;
                   }

                 /* After all the required checks, the Ingress Bandwidth Profile
                  * per UNI will be activated
                  */
                 bw_profile->egress_evc_cos_bw_profile[i]->active = PAL_TRUE;

               }
             else
               {
                 if (bw_profile->egress_evc_cos_bw_profile[i]->active == PAL_TRUE)
                   bw_profile->egress_evc_cos_bw_profile[i]->active = PAL_FALSE;
                 else
                   return NSM_ERR_BW_PROFILE_NOT_ACTIVE;
               }

           }/*if (cos [i] == PAL_TRUE)*/
       } /* for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)*/

   }/*else */

#ifdef HAVE_ELMID
 if (bw_profile_type == NSM_INGRESS_POLICING && bw_profile_status == PAL_TRUE)
   {
     pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bw_profile));

     /* send msg to elmi regarding evc bw profile */
     NSM_SET_CTYPE(msg.cindex, NSM_ELMI_CTYPE_EVC_COS_BW);

     msg.ifindex = br_port->ifindex;
     msg.evc_ref_id = bw_profile->svlan;

     for (i = 0; i < NSM_MAX_COS_PER_EVC; i++)
       {
         if (cos [i] == PAL_TRUE)
           {
             if (first == PAL_FALSE)
               {
                 msg_cos_value = i;
                 msg_cos_id = bw_profile->ingress_CoS_id [i];
               }
             SET_FLAG(msg_cos_pri, i);
           }
       }

     if (bw_profile->ingress_evc_cos_bw_profile[msg_cos_value] != NULL)
       {
         msg.cir = 
           bw_profile->ingress_evc_cos_bw_profile[msg_cos_value]->comm_info_rate;
         msg.cbs = 
           bw_profile->ingress_evc_cos_bw_profile[msg_cos_value]->comm_burst_size;
         msg.eir = 
           bw_profile->ingress_evc_cos_bw_profile[msg_cos_value]->excess_info_rate;
         msg.ebs = 
           bw_profile->ingress_evc_cos_bw_profile[msg_cos_value]->excess_burst_size;
         msg.cf = 
           bw_profile->ingress_evc_cos_bw_profile[msg_cos_value]->coupling_flag;
         msg.cm = 
           bw_profile->ingress_evc_cos_bw_profile[msg_cos_value]->color_mode;
       }

     msg.cos_id = msg_cos_id;
     msg.per_cos = msg_cos_pri;

     nsm_server_elmi_send_bw (br_port, &msg, NSM_MSG_ELMI_EVC_COS_BW_ADD);
   }
#endif /* HAVE_ELMID */

  return RESULT_OK;
}

/**@brief  - Function to allocate memory for nsm_bw_profile in nsm_if structure.
 * @param zif - nsm_interface on which the memory needs to be allocated.
 * @return bw_profile - Successfully allocated memory for zif->nsm_bw_profile
 *                      and returning the nsm_nw_profile pointet.
 * @return NULL - Error Memory not allocated .
 * */

struct nsm_band_width_profile *
nsm_bridge_if_bw_profile_init (struct nsm_if *zif)
{
  struct nsm_band_width_profile *bw_profile = NULL;

  if (!zif)
   return NULL;

  if (zif->nsm_bw_profile != NULL)
    return NULL;

  bw_profile = XCALLOC (MTYPE_NSM_UNI_BW_PROFILE,
                        sizeof (struct nsm_band_width_profile));

  if (!bw_profile)
    {
      zlog_info(NSM_ZG, "NSM out of memory");
      return NULL;
    }

  bw_profile->zif = zif;
  bw_profile->bridge = zif->bridge;

  return bw_profile; 
}

/**@brief  -Function to find svlan by evc -id. 
 * @param bridge - Bridge on which the svlan need to be searched.
 * @param evc_id - Evc id by which the svlan need to be searched.
 * @return vlan  - Successfully searched svlan and returning the vlan.
 * @return NULL - Error SVLAN not found by EVC-ID.
 * */
struct nsm_vlan*
nsm_find_svlan_by_evc_id (struct nsm_bridge *bridge, u_int8_t *evc_id)
{
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;

  if (!bridge)
    return NULL;
  
  if (bridge->svlan_table != NULL)
    for (rn = avl_getnext (bridge->svlan_table, NULL); rn;
        rn = avl_getnext (bridge->svlan_table, rn))
      {
        if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
          {
            if (vlan->evc_id != NULL)
              {
                if (pal_strncmp (vlan->evc_id, evc_id, 
                      (pal_strlen (evc_id) + 1)) == 0)
                  
                  return vlan;
              }
          }
      }
  return NULL;
}

/**@brief  - Function to find the uni_evc_bw_profile node for 
 *           SVLAN/EVC on an interface.
 * @param ifp - Interface on which the svlan's evc_bw_profile node
 *              need to be searched.
 * @param vid - Svlan id by which the evc_bw_profile need to be searched.
 * @return evc_bw_profile  - Successfully searched evc_bw_profile node by svlan 
 *                           and returning the svlan's evc_bw_profile.
 * @return NULL - evc_bw_profile for SVLAN not found.
 * */

static struct nsm_uni_evc_bw_profile *
nsm_find_uni_evc_bw (struct interface *ifp, u_int16_t vid)
{
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_band_width_profile *uni_bw_profile = NULL;
  struct nsm_uni_evc_bw_profile *evc_bw_profile = NULL;
  struct listnode *evc_node = NULL;
  
  zif = ifp->info;
  
  if (!zif)
    return NULL;

  bridge = zif->bridge;

  if (!bridge)
    return NULL;

  if (!zif->nsm_bw_profile)
    return NULL;
  
  uni_bw_profile = zif->nsm_bw_profile;

  if (!uni_bw_profile->uni_evc_bw_profile_list)
    return NULL;
  
  LIST_LOOP (uni_bw_profile->uni_evc_bw_profile_list, evc_bw_profile, evc_node)
    {
      if (evc_bw_profile->svlan == vid)
        return evc_bw_profile;
    }

 return NULL;

}

/**@brief  - API to set the service instance id for svlan/evvc-id.
 * @param ifp - Interface on which the svlan's evc_bw_profile node
 *              need to be searched.
 * @param instance_id - Instance id or svlan id of the evc.
 * @param evc_id - The evc-id for which the service instance mode need
 *                  to be entered.
 * @param **ret_nsm_uni_evc_bw_profile - Pointer which is updated with the
 *                                       evc_bw_profile node.
 * @return RESULT_OK  - Successfully found evc_bw_profile.
 * @return Error - Error reflecting the type of the error.
 * */

s_int32_t
nsm_uni_evc_set_service_instance (struct interface *ifp, u_int16_t instance_id, 
    u_int8_t *evc_id, struct nsm_uni_evc_bw_profile 
    **ret_nsm_uni_evc_bw_profile)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge                *bridge = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan                  *vlan = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *bw_profile = NULL;
  struct nsm_uni_evc_bw_profile    *evc_bw_profile = NULL;
  s_int32_t ret = NSM_L2_ERR_INVALID_ARG;
  
  if (ifp == NULL)
    return NSM_L2_ERR_INVALID_ARG;

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      return ret;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      return ret;
    }

  if (zif->bridge)
    {
      bridge = zif->bridge;
    }

  if (bridge == NULL)
    return NSM_ERR_IF_NOT_FOUND;

  vlan_port = &br_port->vlan_port;
 
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      return ret;
    }

   /* Find svlan by EVC-ID */
  vlan = nsm_find_svlan_by_evc_id (bridge, evc_id);

  if (vlan == NULL)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }
  
  if (vlan->vid != instance_id) 
    {
      return NSM_ERR_INSTANCE_ID_DO_NOT_MATCH;
    }
  /* Allocate memory of nsm_bw_profile is NULL */
  if (! (zif->nsm_bw_profile))
    {
      zif->nsm_bw_profile = nsm_bridge_if_bw_profile_init (zif);

      if (zif->nsm_bw_profile)
        {
          bw_profile = zif->nsm_bw_profile;  
        }

    }
  else
    bw_profile = zif->nsm_bw_profile;

  if (bw_profile == NULL)
    {
      return ret;
    }
  
  /* IF the ingress and egress bw profile are per uni, then donot allow
   *  service instance
   */
  if ((bw_profile->ingress_bw_profile_status == BW_PROFILE_CONFIGURED)
      && (bw_profile->ingress_bw_profile_type == NSM_BW_PROFILE_PER_UNI)
      && (bw_profile->egress_bw_profile_status == BW_PROFILE_CONFIGURED)
      && (bw_profile->egress_bw_profile_type == NSM_BW_PROFILE_PER_UNI))
    {
      return NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED; 
    }
  
  /* Creating the list for evc bw profile */
  if ( bw_profile->uni_evc_bw_profile_list == NULL)
    {
      bw_profile->uni_evc_bw_profile_list = list_new ();
    }
  
  /* Find the evc bw profile, if it exists already */ 
  evc_bw_profile = nsm_find_uni_evc_bw (ifp, vlan->vid);
  
  /* If evc_bw_profile donot exists, then allocate memory */
  if (evc_bw_profile == NULL)
    {
      evc_bw_profile = XCALLOC (MTYPE_NSM_UNI_EVC_BW,
          sizeof (struct nsm_uni_evc_bw_profile));
      
      if (evc_bw_profile == NULL)
        return RESULT_ERROR;

      evc_bw_profile->svlan = vlan->vid;

      if (vlan->evc_id)
        {
          evc_bw_profile->evc_id = XSTRDUP (MTYPE_VLAN_EVC_ID, vlan->evc_id);
        }
     
      evc_bw_profile->instance_id = instance_id;
      evc_bw_profile->ifp = ifp;

      listnode_add (bw_profile->uni_evc_bw_profile_list, 
          evc_bw_profile);
    }

  zif->nsm_bw_profile = bw_profile;
  
  *ret_nsm_uni_evc_bw_profile = evc_bw_profile;

  return RESULT_OK;

}

/**@brief  - API to delete service instance.
 * @param ifp - Interface on which the svlan's evc_bw_profile node
 *              need to be searched.
 * @param instance_id - Instance id or svlan id of the evc.
 * @param evc_id - The evc-id for which the service instance mode need
 *                  to be entered.
 * @return RESULT_OK  - Successfully deleted evc_bw_profile.
 * @return Error - Error reflecting the type of the error.
 * */

s_int32_t
nsm_unset_uni_evc_set_service_instance (struct interface *ifp,
    u_int16_t instance_id, u_int8_t *evc_id)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan                  *vlan = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  struct nsm_band_width_profile    *bw_profile = NULL;
  struct nsm_uni_evc_bw_profile    *evc_bw_profile = NULL;
  s_int32_t ret = NSM_L2_ERR_INVALID_ARG;
  
  if (ifp == NULL)
    return NSM_L2_ERR_INVALID_ARG;

  zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      return ret;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;
 
  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }

  if (zif->nsm_bw_profile)
    {
      bw_profile = zif->nsm_bw_profile;  
    }
  else
    {
      return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;
    }
  
  /* Find svlan by EVC ID */
  vlan = nsm_find_svlan_by_evc_id (zif->bridge, evc_id);

  if (vlan == NULL)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }
  /* Find evc bw profile per vlan */ 
  evc_bw_profile = nsm_find_uni_evc_bw (ifp, vlan->vid);
  
  /* IF it is null, then return an error */
  if (evc_bw_profile == NULL)
    return NSM_ERR_BW_PROFILE_NOT_CONFIGURED;

  /* Free the memory allocated */
  if ((evc_bw_profile->ingress_evc_bw_profile == NULL) &&
      (evc_bw_profile->egress_evc_bw_profile == NULL)) 
    {
      listnode_delete (bw_profile->uni_evc_bw_profile_list, 
          evc_bw_profile);
    }
  else
    {
      return NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED; 
    }

  /* If there are no evc bw profiles configured, then
   * delete the list*/
  if ((bw_profile->uni_evc_bw_profile_list != NULL) &&
      (bw_profile->uni_evc_bw_profile_list->count == PAL_FALSE))
    {
      list_free (bw_profile->uni_evc_bw_profile_list);
      bw_profile->uni_evc_bw_profile_list = NULL;
    }
  
  /* If ingress and egress bw profile are not configured, 
   * then free zif->nsm_bw_profile 
   */ 
  if ((bw_profile->ingress_bw_profile_status == BW_PROFILE_NOT_CONFIGURED)
      && (bw_profile->egress_bw_profile_status == BW_PROFILE_NOT_CONFIGURED))
    {
      XFREE (MTYPE_NSM_UNI_BW_PROFILE, bw_profile);
      bw_profile = NULL;
      zif->nsm_bw_profile = NULL;
    }
  
  return RESULT_OK;
}

/**@brief  - API to set evc-id for SVLAN.
 * @param master - Bridge master in which the entered the bridge need to be
 *                 found.
 * @param bridge_name - Bridge name.
 * @param svid        - Svlan id for which the evc-id need to be configured.
 * @param evc_id      - Evc-id entered for the svlan id.
 * @return RESULT_OK  - Successfully set evc_id for the svlan.
 * @return Error      - Error reflecting the type of the error.
 * */
s_int32_t 
nsm_svlan_set_evc_id (struct nsm_bridge_master *master, 
                      u_int8_t *bridge_name, u_int16_t svid, u_int8_t *evc_id)
{
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan p;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
  struct avl_node *node = NULL;
  struct nsm_svlan_reg_info *svlan_info = NULL;
  struct nsm_cvlan_reg_tab *regtab = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL; 
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
#endif /* HAVE_ELMID */

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, bridge_name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  vlan_table = bridge->svlan_table;

  if (! vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  /* Find svlan by the given evc -id */
  vlan = nsm_find_svlan_by_evc_id (bridge, evc_id);
  
  /* If the evc-id is already set, then return an error */
  if (vlan != NULL)
    return NSM_VLAN_ERR_EVC_ID_SET;

  NSM_VLAN_SET (&p, svid);

  rn = avl_search (vlan_table, (void *)&p);

  if (rn)
    {
      if (! rn->info)
        return NSM_VLAN_ERR_VLAN_NOT_FOUND;

      vlan = rn->info;
      
      if (vlan->evc_id)
        {
          vlan->evc_id = NULL;
          XFREE (MTYPE_VLAN_EVC_ID, vlan->evc_id);
        }
      
      /* Allocate memory for vlan->evc_id and copy the evc_id
       * to vlan->evc_id
       */
      vlan->evc_id = XSTRDUP (MTYPE_VLAN_EVC_ID, evc_id);

      /* Send msg to ELMI about evc id update */
#ifdef HAVE_ELMID
      /* check if this svid is applied to any if */ 
      AVL_TREE_LOOP (bridge->port_tree, br_port, node)
        {
          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
            continue;

          regtab = vlan_port->regtab;
          if (regtab == NULL)
             continue;

          if ((svlan_info = nsm_reg_tab_svlan_info_lookup (regtab,
                                                           svid)) == NULL)
            continue;
              
      cindex = 0;
      NSM_SET_CTYPE(cindex, NSM_ELMI_CTYPE_EVC_ID_STR);
      nsm_server_evc_update (ifp, svid, svlan_info, cindex);

        }
#endif /* HAVE_ELMID */
       
    }
  else
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  return RESULT_OK;

}

/**@brief  - API to unset evc-id for SVLAN.
 * @param master      - Bridge master in which the entered the bridge need to be
 *                      found.
 * @param bridge_name - Bridge name. 
 * @param svid        - Svlan id for which the evc-id need to be unconfigured.
 * @param evc_id      - evc-id entered for the svlan id.
 * @return RESULT_OK  - Successfully set evc_id for the svlan.
 * @return Error - Error reflecting the type of the error.
 * */
s_int32_t 
nsm_svlan_unset_evc_id (struct nsm_bridge_master *master, 
                      u_int8_t *bridge_name, u_int16_t svid, u_int8_t *evc_id)
{
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *vlan_table;
  struct nsm_vlan *vlan;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if ( nsm_bridge_is_vlan_aware(master, bridge_name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  vlan_table = bridge->svlan_table;

  if (! vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  /* Find svlan by the evc-id */
  vlan = nsm_find_svlan_by_evc_id (bridge, evc_id);

  if (vlan == NULL)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;
  
  /* Free the evc id memory */
  if (vlan->evc_id)
    {
      XFREE (MTYPE_VLAN_EVC_ID, vlan->evc_id);
      vlan->evc_id = NULL;
    }
  else
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  return RESULT_OK;

}

/**@brief  - API to set max uni for the EVC.
 * @param master      - Bridge master in which the entered the bridge need to be
 *                      found.
 * @param bridge_name - Bridge name. 
 * @param svid        - Svlan id for which the max-uni need to be set.
 * @param evc_id      - Evc-id for which the max-uni need to be set.
 * @param max_uni     - max_uni configured for svlan/evc.
 * @return RESULT_OK  - Successfully set max_uni for the svlan/evc.
 * @return Error - Error reflecting the type of the error.
 * */
s_int32_t
nsm_svlan_set_max_uni (struct nsm_bridge_master *master,
                    u_int8_t *bridge_name, u_int16_t svid, u_int8_t *evc_id,
                    u_int16_t max_uni)
{
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan p;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (!NSM_BRIDGE_TYPE_PROVIDER (bridge) || 
      (bridge->provider_edge == PAL_FALSE))
    {
      return NSM_VLAN_ERR_NOT_EDGE_BRIDGE;
    }

  if ( nsm_bridge_is_vlan_aware(master, bridge_name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  vlan_table = bridge->svlan_table;

  if (! vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;
 
  /* Find svlan by evc-id */ 
  if (evc_id != NULL)
    {
      vlan = nsm_find_svlan_by_evc_id (bridge, evc_id);
    }
  else
    {
      /* Find svlan by vlan id */
      NSM_VLAN_SET (&p, svid);

      rn = avl_search (vlan_table, (void *)&p);

      if (rn)
        {
          if (! rn->info)
            return NSM_VLAN_ERR_VLAN_NOT_FOUND;

          vlan = rn->info;
        }
    }

  if (vlan == NULL)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;
  
  /* IF vlan is Point-to-Point , the max uni must be two */
  if (vlan->type & NSM_SVLAN_P2P)
    {
      if (max_uni == 2)
        {
          vlan->max_uni = max_uni;
        } 
      else
        return NSM_VLAN_ERR_MAX_UNI_PT_TO_PT;
    }
  /* IF SVLAN is Multipoint-to-Multipoint, then mac uni must be >= 2 */
  else if (vlan->type & NSM_SVLAN_M2M)
    {
      if (max_uni < 2)
        {
          return NSM_VLAN_ERR_MAX_UNI_M2M;
        }
      vlan->max_uni = max_uni; 
      
    }

  return RESULT_OK;

}

/**@brief  - API to unset max uni for the EVC.
 * @param master      - Bridge master in which the entered the bridge need to be
 *                      found.
 * @param bridge_name - Bridge name.
 * @param svid        - Svlan id for which the max-uni need to be unset.
 * @param evc_id      - Evc-id for which the max-uni need to be unset.
 * @return RESULT_OK  - Successfully deleted max_uni for the svlan/evc.
 * @return Error - Error reflecting the type of the error.
 * */
s_int32_t
nsm_svlan_unset_max_uni (struct nsm_bridge_master *master,
                    u_int8_t *bridge_name, u_int16_t svid, u_int8_t *evc_id)
{
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan p;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (!NSM_BRIDGE_TYPE_PROVIDER (bridge) || 
      (bridge->provider_edge == PAL_FALSE))
    {
      return NSM_VLAN_ERR_NOT_EDGE_BRIDGE;
    }


  if ( nsm_bridge_is_vlan_aware(master, bridge_name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  vlan_table = bridge->svlan_table;

  if (! vlan_table)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;
  
  if (evc_id != NULL)
    {
      /* Find svlan by evc-id */
      vlan = nsm_find_svlan_by_evc_id (bridge, evc_id);
    }
  else
    {
      /* Find svlan by vlan-id */
      NSM_VLAN_SET (&p, svid);

      rn = avl_search (vlan_table, (void *)&p);

      if (rn)
        {
          if (! rn->info)
            return NSM_VLAN_ERR_VLAN_NOT_FOUND;

          vlan = rn->info;
        }
    }

  if (vlan == NULL)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;
  
  /* Unsetting the max uni */
  vlan->max_uni = PAL_FALSE;

  return RESULT_OK;
}

/**@brief  - Function to find interface by uni-id.
 * @ifm      - if_vr_master in which the interface need to be searched.
 * @param uni_name - uni_name.
 * @return interface *ifp - Successfully found interface by the uni_name.
 * @return NULL - Error, ifp not found.
 * */
struct interface *
nsm_if_lookup_by_uni_id (struct if_vr_master *ifm, char *uni_name)
{
  struct interface *ifp;
  struct listnode *node;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
#ifdef HAVE_INTERFACE_NAME_MAPPING
  struct if_master *ifg = &ifm->vr->zg->ifg;
  struct if_map find;
  struct if_map *key;

  pal_strncpy (find.from, name, INTERFACE_NAMSIZ);
  key = hash_lookup (ifg->if_map_hash, &find);
  if (key)
    name = key->to;
#endif /* HAVE_INTERFACE_NAME_MAPPING */

  LIST_LOOP (ifm->if_list, ifp, node)
    {
      zif = ifp->info;
      if (!zif)
        continue;
      
      br_port = zif->switchport;

      if (!br_port)
        continue;

      if (pal_strcmp (br_port->uni_name, uni_name) == 0)
        return ifp;
    }

  return NULL;
}

s_int32_t
nsm_pro_edge_port_set_pvid (struct interface *ifp,
                            u_int16_t svid,
                            u_int16_t pvid)
{
  struct nsm_pro_edge_info *pro_edge_port;
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_msg_vlan_port msg;
  struct nsm_if *zif;
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (UNI Name Set);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  if (! vlan_port->regtab)
    {
      ret = NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND;
      goto EXIT;
    }

  if (! (svlan_info = nsm_reg_tab_svlan_info_lookup
                                  (vlan_port->regtab, svid)))
    {
      ret = NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND;
      goto EXIT;
    }

  if (! NSM_VLAN_BMP_IS_MEMBER (svlan_info->cvlanMemberBmp, pvid))
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND;
      goto EXIT;
    }

  if (! (pro_edge_port = nsm_reg_tab_svlan_info_port_lookup
                                  (svlan_info, ifp)))
    {
      ret = NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND;
      goto EXIT;
    }

  pro_edge_port->pvid = pvid;

  ret = hal_vlan_set_pro_edge_pvid (zif->bridge->name, ifp->ifindex,
                                    svid, pvid);

  if (ret < 0)
    ret = NSM_PRO_VLAN_ERR_HAL_ERR;

  /* The message would contain CE port ifindex, svid to identify PE port 
   * and the untagged_vid for this PE port */
  msg.ifindex = ifp->ifindex;
  msg.num = NSM_PEP_SVID_MSG_NUM;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

  if (msg.vid_info == NULL)
    goto EXIT;

  msg.vid_info[0] = svlan_info->svid;
  msg.vid_info[1] = pro_edge_port->pvid;
        
  nsm_vlan_send_port_map (zif->bridge, &msg, NSM_MSG_DEFAULT_VID_PE_PORT);

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_pro_edge_port_set_untagged_vid (struct interface *ifp,
                                    u_int16_t svid,
                                    u_int16_t untagged_vid)
{
  struct nsm_pro_edge_info *pro_edge_port;
  struct nsm_svlan_reg_info *svlan_info;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_msg_vlan_port msg;
  struct nsm_if *zif;
  s_int32_t ret;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
#endif /* HAVE_ELMID */

  NSM_VLAN_FN_ENTER (UNI Name Set);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  if (! vlan_port->regtab)
    {
      ret = NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND;
      goto EXIT;
    }

  if (! (svlan_info = nsm_reg_tab_svlan_info_lookup
                                  (vlan_port->regtab, svid)))
    {
      ret = NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND;
      goto EXIT;
    }

  if (! NSM_VLAN_BMP_IS_MEMBER (svlan_info->cvlanMemberBmp, untagged_vid))
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND;
      goto EXIT;
    }

  if (! (pro_edge_port = nsm_reg_tab_svlan_info_port_lookup
                                  (svlan_info, ifp)))
    {
      ret = NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND;
      goto EXIT;
    }

  pro_edge_port->untagged_vid = untagged_vid;

  ret = hal_vlan_set_pro_edge_untagged_vid (zif->bridge->name, ifp->ifindex,
                                            svid, untagged_vid);

  if (ret < 0)
    ret = NSM_PRO_VLAN_ERR_HAL_ERR;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));

  /* The message would contain CE port ifindex, svid to identify PE port 
   * and the untagged_vid for this PE port */
  msg.ifindex = ifp->ifindex;
  msg.num = NSM_PEP_SVID_MSG_NUM;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

  if (msg.vid_info == NULL)
    goto EXIT;

  msg.vid_info[0] = svlan_info->svid;
  msg.vid_info[1] = pro_edge_port->untagged_vid;
        
  nsm_vlan_send_port_map (zif->bridge, &msg, NSM_MSG_UNTAGGED_VID_PE_PORT);

#ifdef HAVE_ELMID
  cindex = 0;
  NSM_SET_CTYPE(cindex, NSM_ELMI_CTYPE_UNTAGGED_PRI);
  nsm_server_evc_update (ifp, svid, svlan_info, cindex);
#endif /* HAVE_ELMID */

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

/* Sync Customer Edge port information. */
int
nsm_pro_vlan_ce_port_sync (struct nsm_master *nm,
                           struct interface *ifp)
{
  u_int16_t svid;
  struct nsm_if *zif;
  struct nsm_msg_vlan_port msg;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  if (ifp == NULL
      || ((zif = ifp->info) == NULL)
      || ((br_port = zif->switchport) == NULL)
      || ((vlan_port = &br_port->vlan_port) == NULL)
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    return 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
  msg.ifindex = ifp->ifindex;
  msg.num = 1;
  msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

  NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->svlanMemberBmp, svid)
    {
      msg.vid_info[0] = svid;
      nsm_vlan_send_port_map (zif->bridge, &msg, NSM_MSG_SVLAN_ADD_CE_PORT);
    }
  NSM_VLAN_SET_BMP_ITER_END (vlan_port->staticMemberBmp, svid);

  if (msg.vid_info)
    XFREE (MTYPE_TMP, msg.vid_info);

  return 0;
}

void
nsm_pro_vlan_send_proto_process (u_int8_t *bridge_name,
                                 struct interface *ifp,
                                 u_int8_t bpdu_process)
{
  s_int32_t i;
  s_int32_t nbytes;
  struct nsm_server_client *nsc;
  struct nsm_msg_bridge_if msg;
  struct nsm_server_entry *nse;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if));

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_strncpy (msg.bridge_name, bridge_name, NSM_BRIDGE_NAMSIZ);
        msg.num = 1;

        /* Allocate ifindex array. */
        msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
        msg.ifindex[0] = ifp->ifindex;
        msg.cust_bpdu_process = bpdu_process;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt,
                                           &nse->send.size, &msg);
        if (nbytes < 0)
          return;

        /* Send bridge interface message. */
        nsm_server_send_message (nse, 0, 0, NSM_MSG_SET_BPDU_PROCESS,
                                 0, nbytes);

        /* Free ifindex array. */
        XFREE (MTYPE_TMP, msg.ifindex);
      }
}

#ifdef HAVE_SMI

int
nsm_pro_vlan_set_dtag_mode (struct interface *ifp,
                            enum smi_dtag_mode dtag_mode)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;
#ifdef HAVE_HAL
  u_int8_t hal_dtag_mode = HAL_VLAN_STACK_MODE_NONE;
#endif /* HAVE_HAL */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  vlan_port = &zif->switchport->vlan_port;

  if (vlan_port == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#ifdef HAVE_HAL
  switch (dtag_mode)
   {
     case SMI_DTAG_MODE_INTERNAL:
       hal_dtag_mode = HAL_VLAN_STACK_MODE_INTERNAL;
       break;
     case SMI_DTAG_MODE_EXTERNAL:
       hal_dtag_mode = HAL_VLAN_STACK_MODE_EXTERNAL;
       break;
     default:
       hal_dtag_mode = HAL_VLAN_STACK_MODE_NONE;
       break;
   }

  ret = hal_pro_vlan_set_dtag_mode (ifp->ifindex, hal_dtag_mode);

  if (ret != HAL_SUCCESS)
    return NSM_VLAN_ERR_HAL_ERROR;

  vlan_port->stack_mode = hal_dtag_mode;

#endif /* HAVE_HAL */

  return 0;

}

int
nsm_pro_vlan_get_dtag_mode (struct interface *ifp,
                            enum smi_dtag_mode *dtag_mode)
{
  struct nsm_if *zif;
  struct nsm_vlan_port *vlan_port;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  vlan_port = &zif->switchport->vlan_port;

  if (vlan_port == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  switch (vlan_port->stack_mode)
    {
     case HAL_VLAN_STACK_MODE_INTERNAL:
       *dtag_mode = SMI_DTAG_MODE_INTERNAL;
       break;
     case HAL_VLAN_STACK_MODE_EXTERNAL:
       *dtag_mode = SMI_DTAG_MODE_EXTERNAL;
       break;
     default:
       *dtag_mode = SMI_DTAG_MODE_INVALID;
       break;
    }

  return 0;

}

#endif /* HAVE_SMI */

#endif /* HAVE_PROVIDER_BRIDGE */

/**@brief  - API to enable/disabe UNI Type mode on the bridge.
 * @param br_name - Name of the bridge for which UNI Type mode need to be set.
 * @param uni_type_mode - Variable which mentions enable/disable the uni type
 *                         mode.
 * @ param master - bridge master from which the given configured bridge 
 *                  need to be searched.
 * @return RESULT_OK - For Success. 
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t
nsm_bridge_uni_type_detect (u_int8_t *br_name, u_int8_t uni_type_mode,
                              struct nsm_bridge_master *master)
{
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct interface *ifp = NULL;
  struct avl_node *avl_port = NULL;
  s_int32_t ret = RESULT_OK;

  bridge = nsm_lookup_bridge_by_name (master,br_name);

  if (bridge == NULL)
    {
      return NSM_BRIDGE_ERR_NOTFOUND;
    }
  /* If the bridge is Provider- MSTP or
   * Provider RSTP bridge i.e. PB Bridge,
   * then return an error
   */
  if (NSM_BRIDGE_TYPE_PROVIDER (bridge)
#ifdef HAVE_PROVIDER_BRIDGE
      && (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
      )
    {
      return NSM_BRIDGE_ERR_INVALID_ARG; 
    }

  /* Enable UNI Type on the bridge */
  bridge->uni_type_mode = uni_type_mode;

  /* Enable UNI Type on all the ports associated to the bridge
   */  
  AVL_TREE_LOOP (bridge->port_tree, br_port, avl_port)
    {
      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;
      
      vlan_port = &br_port->vlan_port;

      if (! vlan_port)
        continue;

      if (NSM_BRIDGE_TYPE_PROVIDER (bridge)
#ifdef HAVE_PROVIDER_BRIDGE
      && (bridge->provider_edge == PAL_FALSE)
#endif /* HAVE_PROVIDER_BRIDGE */
         )

        {
          continue;
        }
      
      ret = nsm_uni_type_detect (ifp, uni_type_mode);
    }

  return RESULT_OK;
}

/**@brief  - API to enable/disabe UNI Type mode on the Interface.

 * @param ifp - Interface for which UNI Type mode need to be set.
 * @param uni_type_mode - variable which mentions enable/disable the uni type
 *                         mode.
 * @return RESULT_OK - For Success.
 * @return Error - Error reflecting the type of failure.
 * */

s_int32_t 
nsm_uni_type_detect (struct interface *ifp, u_int8_t uni_type_mode)
{
  struct nsm_if                    *zif = NULL;
  struct nsm_bridge_port           *br_port = NULL;
  struct nsm_vlan_port             *vlan_port = NULL;
  s_int32_t ret;
  
  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return NSM_L2_ERR_INVALID_ARG;
    }

  vlan_port = &br_port->vlan_port;
 
  if (! vlan_port)
    return NSM_L2_ERR_INVALID_ARG;
  
  if (!zif->bridge)
    return NSM_L2_ERR_INVALID_ARG;
  
  if (NSM_BRIDGE_TYPE_PROVIDER (zif->bridge) &&
#ifdef HAVE_PROVIDER_BRIDGE
      (zif->bridge->provider_edge == PAL_TRUE) &&
#endif /* HAVE_PROVIDER_BRIDGE */
      (vlan_port->mode != NSM_VLAN_PORT_MODE_CE))
    {
      return NSM_PRO_VLAN_ERR_INVALID_MODE;
    }
  
  /* Returning when the the user tries to set the uni type mode 
   * which is already set 
   */ 
  /*if (br_port->uni_type_status.uni_type_mode == uni_type_mode)
    {
      return NSM_ERR_UNI_MODE_CONFIGURED;
    }*/
      
  if (uni_type_mode == NSM_UNI_TYPE_ENABLE)
    {
      /* If ELMI status is Operational,
       * then the type is UNI Type 2 and UNI Type 2.2
       */
      if (br_port->uni_type_status.elmi_status == NSM_ELMI_OPERATIONAL)
        {
          br_port->uni_type_status.uni_type = NSM_UNI_TYPE_2;
          br_port->uni_type_status.uni_type_2_status = 
                                         NSM_UNI_TYPE_TWO_TWO; 
        }
      else 
        {
          /* IF E-LMI is not operational and if CFM is Operational,
           * then the uni type is uni type 2 with UNI Type 2.1
           */
           if (br_port->uni_type_status.cfm_status == NSM_CFM_OPERATIONAL)
             {
               br_port->uni_type_status.uni_type =  NSM_UNI_TYPE_2;
               br_port->uni_type_status.uni_type_2_status = 
                                               NSM_UNI_TYPE_TWO_ONE;
             }
           else
             {
               /* When both CFM and E-LMI are not operational,
                * UNI Type is UNI Type 1
                */
               br_port->uni_type_status.uni_type = NSM_UNI_TYPE_1;
             }
        }
      
      /* Enabling uni type mode on the interface */
      br_port->uni_type_status.uni_type_mode = NSM_UNI_TYPE_ENABLE;
    }
  else
    {
      br_port->uni_type_status.uni_type = NSM_UNI_TYPE_1;
      br_port->uni_type_status.uni_type_2_status = NSM_UNI_TYPE_DISABLE;
      
      /* Disabling uni type mode on interface */
      br_port->uni_type_status.uni_type_mode = NSM_UNI_TYPE_DISABLE;
    }
    
  return RESULT_OK;
}


#ifdef HAVE_PROVIDER_BRIDGE
s_int32_t
nsm_customer_edge_port_set_def_svid (struct interface *ifp,
                                     u_int16_t svid, 
                                     enum nsm_vlan_port_mode mode,
                                     enum nsm_vlan_port_mode sub_mode)
{
  struct nsm_svlan_reg_info *svlan_info = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_if *zif = NULL;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
#endif /* HAVE_ELMID */
  s_int32_t ret;

  NSM_VLAN_FN_ENTER (UNI Name Set);

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  if (! vlan_port->regtab)
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND;
      goto EXIT;
    }

  if (! (svlan_info = nsm_reg_tab_svlan_info_lookup
        (vlan_port->regtab, svid)))
    {
      ret = NSM_VLAN_ERR_SVLAN_NOT_FOUND;
      goto EXIT;
    }

  if (br_port->uni_ser_attr != NSM_UNI_BNDL)
    {
      ret = NSM_PRO_VLAN_ERR_SVLAN_SERVICE_ATTR;
      goto EXIT;
    }

  vlan_port->uni_default_evc = svid;

  /* Send msg to ELMI about evc details update */
#ifdef HAVE_ELMID
  cindex = 0;
  NSM_SET_CTYPE(cindex, NSM_ELMI_CTYPE_DEFAULT_EVC);
  nsm_server_evc_update (ifp, svid, svlan_info, cindex);
#endif /* HAVE_ELMID */


EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

s_int32_t
nsm_customer_edge_port_unset_def_svid (struct interface *ifp, 
                                       enum nsm_vlan_port_mode mode,
                                       enum nsm_vlan_port_mode sub_mode)

{
  struct nsm_cvlan_reg_tab *regtab = NULL;
  struct nsm_svlan_reg_info *svlan_info = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_cvlan_reg_key *key = NULL;
  struct avl_node *node = NULL;
  struct nsm_if *zif = NULL;
  u_int8_t send_msg = PAL_FALSE;
#ifdef HAVE_ELMID
  cindex_t cindex = 0;
#endif /* HAVE_ELMID */
  s_int32_t ret;

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (! zif
      || ! zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      goto EXIT;
    }

  vlan_port = &br_port->vlan_port;

  if (! vlan_port
      || vlan_port->mode != NSM_VLAN_PORT_MODE_CE)
    {
      ret = NSM_PRO_VLAN_ERR_INVALID_MODE;
      goto EXIT;
    }

  if (vlan_port->uni_default_evc == NSM_DEFAULT_VAL)
    {
      ret = NSM_PRO_VLAN_ERR_DEFAULT_EVC_NOT_SET;
      goto EXIT;
    }

  if (! (regtab = br_port->vlan_port.regtab))
    {
      ret = NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND;
      goto EXIT;
    }


  if (br_port->uni_ser_attr != NSM_UNI_BNDL)
    {
      ret = NSM_PRO_VLAN_ERR_SVLAN_SERVICE_ATTR;
      goto EXIT;

    }

  for (node = avl_top (regtab->reg_tab); node; node = avl_next (node))
    {
      if (! (key = AVL_NODE_INFO (node)))
        continue;

      if (key->svid == vlan_port->uni_default_evc)
        {
          vlan_port->uni_default_evc = NSM_DEFAULT_VAL;
          send_msg = PAL_TRUE;
          break;
        }
    }

  if (send_msg)
    {
      svlan_info = nsm_reg_tab_svlan_info_lookup (regtab, key->svid);
      if (!svlan_info)
        goto EXIT;

      /* Send msg to ELMI about evc details update */
#ifdef HAVE_ELMID
      cindex = 0;
      NSM_SET_CTYPE(cindex, NSM_ELMI_CTYPE_DEFAULT_EVC);
      nsm_server_evc_update (ifp, key->svid, svlan_info, cindex);
#endif /* HAVE_ELMID */
    }

EXIT:

  NSM_VLAN_FN_EXIT (ret);
}

#endif /* HAVE_PRVIDER_BRIDGE */
