/* Copyright (C) 2003  All Righps Reserved. */
#include "pal.h"
#include "lib.h"
#include "table.h"
#include "ptree.h"
#include "avl_tree.h"

#include "garp_gid.h"
#include "garp_pdu.h"

#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_bridge_cli.h"
#include "nsm_vlan_cli.h"
#include "nsm_interface.h"
#ifdef HAVE_PROVIDER_BRIDGE
#include "nsm_pro_vlan.h"
#endif /* HAVE_PROVIDER_BRIDGE */
#include "gmrp.h"
#include "gmrp_api.h"
#include "gmrp_database.h"
#include "gmrp_cli.h"
#include "hal_garp.h"
#include "hal_gmrp.h"

#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */

s_int32_t
gmrp_check_timer_rules(struct nsm_bridge_master *master,
                       struct interface *ifp,
                       const u_int32_t timer_type,
                       const pal_time_t timer_value)
{
  pal_time_t timer_details[GARP_MAX_TIMERS];

  if (gmrp_get_timer_details (master, ifp, timer_details) != RESULT_OK)
    return RESULT_ERROR;

  switch (timer_type) {

    case GARP_JOIN_TIMER:
      if (timer_details[GARP_LEAVE_TIMER] >= timer_value * 3)
        return RESULT_OK;
      break;

    case GARP_LEAVE_TIMER:
      if ( (timer_value >= timer_details[GARP_JOIN_TIMER] * 3)  &&
                    (timer_details[GARP_LEAVE_ALL_TIMER] > timer_value) )
        return RESULT_OK;
      break;

    case GARP_LEAVE_ALL_TIMER:
      if (timer_value > timer_details[GARP_LEAVE_TIMER])
        return RESULT_OK;
      break;

    default:
      break;
  }

  return RESULT_ERROR;
}

static int
gmrp_vid_cmp (void *data1,void *data2)
{
  struct gmrp *gmrp1 = data1;
  struct gmrp *gmrp2 = data2;

  if (gmrp1->svlanid > gmrp2->svlanid)
    return 1;

  if (gmrp2->svlanid > gmrp1->svlanid)
    return -1;

  if (gmrp1->vlanid > gmrp2->vlanid)
    return 1;

  if (gmrp2->vlanid > gmrp1->vlanid)
    return -1;

  return 0;
}

static int
gmrp_port_vid_cmp (void *data1,void *data2)
{
  struct gmrp_port_instance *gmrp_port1 = data1;
  struct gmrp_port_instance *gmrp_port2 = data2;

  if (gmrp_port1->svlanid > gmrp_port2->svlanid)
    return 1;

  if (gmrp_port2->svlanid > gmrp_port1->svlanid)
    return -1;

  if (gmrp_port1->vlanid > gmrp_port2->vlanid)
    return 1;

  if (gmrp_port2->vlanid > gmrp_port1->vlanid)
    return -1;

  return 0;
}

s_int32_t
gmrp_enable (struct nsm_bridge_master *master,
             char *protocol,
             const char* const bridge_name)
{
  s_int32_t ret;
  struct gmrp *gmrp;
  struct nsm_bridge *bridge;
  struct avl_node *an = NULL;
  struct avl_tree *vlan_table;
  struct nsm_vlan *vlan = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_pro_edge_sw_ctx *ctx = NULL;
#endif /* HAVE_PROVIDER_BRIDGE */
  u_int16_t vlanid = NSM_VLAN_DEFAULT_VID;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* if igmp snoop enable or gmrp already enabled */  
  if ( (gmrp_bridge = bridge->gmrp_bridge) || (gmrp_list = bridge->gmrp_list)) 
    return 0;

  if (!gmrp_create_gmrp_bridge (bridge, PAL_TRUE, protocol, &gmrp_bridge))
    return NSM_BRIDGE_ERR_MEM;

  bridge->gmrp_bridge = gmrp_bridge;  

  if ( (ret = avl_create (&gmrp_list, 0, gmrp_vid_cmp)) < 0 )
     return NSM_BRIDGE_ERR_MEM;

  if ( bridge->vlan_table == NULL 
       && bridge->svlan_table == NULL )
    {
      vlanid = NSM_VLAN_DEFAULT_VID;

      if (!(gmrp_create_gmrp_instance (gmrp_list, gmrp_bridge, vlanid,
                                       vlanid, &gmrp)))
        return NSM_BRIDGE_ERR_MEM;

       bridge->gmrp_list = gmrp_list;
       return 0;

    }

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge)
    {
      for (an = avl_top (bridge->pro_edge_swctx_table); an; an = avl_next (an))
        {
          if ((ctx = an->info))
            {
              if (!(gmrp_create_gmrp_instance (gmrp_list, gmrp_bridge,
                                               ctx->cvid, ctx->svid, &gmrp)))
                return NSM_BRIDGE_ERR_MEM;
            }
        }
    }
  else
#endif /* HAVE_PROVIDER_BRIDGE */
    {
      for (an = avl_top (vlan_table); an; an = avl_next (an))
        {
          if ((vlan = an->info))
            {
               if (!(gmrp_create_gmrp_instance (gmrp_list, gmrp_bridge, vlan->vid,
                                                vlan->vid, &gmrp)))
                 return NSM_BRIDGE_ERR_MEM;
            }
        }
    }

  bridge->gmrp_list = gmrp_list;

  /* Inform forwarder that gmrp has been configured on this bridge*/
  hal_garp_set_bridge_type (bridge->name,
                            (gmrp_bridge->reg_type == REG_TYPE_GMRP)?
                             HAL_GMRP_CONFIGURED:HAL_MMRP_CONFIGURED,
                             PAL_TRUE);

  return 0;
}

s_int32_t
gmrp_extended_filtering (struct nsm_bridge_master *master,
                         const char* const bridge_name, bool_t enable)
{
  struct nsm_bridge *bridge;
  s_int32_t ret;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  if ( nsm_bridge_is_vlan_aware (master, bridge->name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;

  bridge->gmrp_ext_filter = enable ? 1 : 0; 
  ret = hal_gmrp_set_bridge_ext_filter (bridge->name, enable);
 
  if (ret < 0)
    return ret;

 return 0;
}

void *
gmrp_get_first_node ( struct avl_tree *tree )
{
  struct avl_node *node = NULL;

  if (!tree)
    return NULL;

  for (node = avl_top (tree); node; node = avl_next (node))
   {
     if (AVL_NODE_INFO (node))
       {
         return AVL_NODE_INFO (node);
       }
   }

  return NULL;
}

s_int32_t
gmrp_add_vlan_to_port (struct nsm_bridge *bridge, struct interface *ifp,
                       u_int16_t vlanid, u_int16_t svlanid)
{

  struct gmrp_gmd *gmd;
  struct ptree_node *rn;
  u_int32_t attribute_index;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp       *gmrp           = NULL;
  struct avl_tree   *gmrp_port_list = NULL;
  struct nsm_if     *zif            = NULL;
  struct gmrp_port  *gmrp_port      = NULL;
  struct avl_node   *node           = NULL;
  int    registration_type = GID_EVENT_MAX;
  struct avl_tree *gmrp_list  = bridge->gmrp_list;
  struct gmrp_port_instance  *gmrp_port_instance      = NULL;
  struct gmrp_port_instance  *old_gmrp_port_instance  = NULL;

  zif = (struct nsm_if *) ifp->info;

  if (!zif || !zif->switchport)
    return RESULT_ERROR;

  br_port = zif->switchport;
  gmrp_port_list = br_port->gmrp_port_list;
  gmrp_port      = br_port->gmrp_port;
 
  if ( (gmrp_list == NULL) || (gmrp_port_list == NULL) ||
       (gmrp_port == NULL) )
    return RESULT_ERROR;

  gmrp = gmrp_get_by_vid (gmrp_list, vlanid, svlanid, &node);

  if (gmrp == NULL)
    return RESULT_ERROR;

  old_gmrp_port_instance = gmrp_port_instance_get_by_vid (gmrp_port_list, vlanid,
                                                          svlanid, &node);

  if (old_gmrp_port_instance)
    {
      gmrp_destroy_port_instance (old_gmrp_port_instance);
    }

  if ( !gmrp_create_port_instance (gmrp_port_list, gmrp_port->gid_port, gmrp,
                                   &gmrp_port_instance))
    {
      return RESULT_ERROR;
    }

  /* If we are adding a gid context to a port in fixed/ forbidden
   * registration, put the new gmrp context also in the same
   * registration.
   */

  gmd = gmrp->gmd;

  if (!gmd)
    return RESULT_ERROR;

  if (gmrp_port->registration_cfg != GID_REG_MGMT_NORMAL)
    {
      if ( gmrp_port->registration_cfg == GID_REG_MGMT_FORBIDDEN )
        {
          registration_type = GID_EVENT_FORBID_REGISTRATION;
        } 
      else
        {
          registration_type = GID_EVENT_FIXED_REGISTRATION;
        } 

      for (rn = ptree_top (gmd->gmd_entry_table); rn;
           rn = ptree_next (rn))
        {
          attribute_index = gid_get_attr_index (rn);
          gid_set_attribute_states (gmrp_port_instance->gid, attribute_index, 
                                    registration_type);
        }
    }

  return RESULT_OK;

}

s_int32_t
gmrp_remove_vlan_from_port (struct nsm_bridge *bridge, struct nsm_if *port,
                            u_int16_t vlanid, u_int16_t svlanid)
{

  struct nsm_bridge_port *br_port = NULL;
  struct avl_tree   *gmrp_list    = NULL;
  struct gmrp       *gmrp         = NULL;
  struct avl_tree   *gmrp_port_list = NULL;
  struct gmrp_port  *gmrp_port      = NULL;
  struct avl_node   *node           = NULL;
  struct gmrp_port_instance  *gmrp_port_instance  = NULL;

  if (port == NULL
      || port->switchport == NULL)
    return RESULT_OK;

  br_port = port->switchport;
  gmrp_list = bridge->gmrp_list;
  gmrp_port = br_port->gmrp_port;
  gmrp_port_list = br_port->gmrp_port_list;

  if ( (gmrp_list == NULL) || (gmrp_port_list == NULL) ||
       (gmrp_port == NULL) )
    return RESULT_OK;

  gmrp = gmrp_get_by_vid (gmrp_list, vlanid, svlanid, &node);

  gmrp_port_instance = gmrp_port_instance_get_by_vid (gmrp_port_list, vlanid,
                                                      svlanid, &node);

  if ( (gmrp == NULL) || (gmrp_port_instance == NULL ) )
    return RESULT_OK;

  gmrp_destroy_port_instance (gmrp_port_instance);

  return RESULT_OK;
}

s_int32_t
gmrp_disable (struct nsm_bridge_master *master,
              char* bridge_name)
{
  struct gmrp *gmrp;
  struct nsm_bridge *bridge;
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port;
  struct avl_node    *node = NULL;
  struct avl_tree    *gmrp_list = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct avl_tree    *gmrp_port_list = NULL;

  /* Bridge does not exist, Global GMRP does not exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
       (!(gmrp_bridge = bridge->gmrp_bridge)) ||
       (!(gmrp_list = bridge->gmrp_list)) )
    return NSM_BRIDGE_ERR_NOTFOUND;

  for (node = avl_top (gmrp_list); node; node = avl_next (node))
    {
      gmrp = AVL_NODE_INFO (node);

      if ( gmrp )
        gmrp_free_gmrp_instance (gmrp);
    }

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      gmrp_port_list = br_port->gmrp_port_list;

      if (gmrp_port_list)
        {
           avl_tree_free (&gmrp_port_list, NULL);
           br_port->gmrp_port_list = NULL;
        }

      gmrp_destroy_port (br_port->gmrp_port);
      br_port->gmrp_port = NULL;
    }

  /* Inform forwarder that gmrp has been disabled on this bridge*/
  hal_garp_set_bridge_type (bridge->name,
                            (gmrp_bridge->reg_type == REG_TYPE_GMRP)?
                             HAL_GMRP_CONFIGURED:HAL_MMRP_CONFIGURED,
                             PAL_FALSE);

  gmrp_destroy_gmrp_bridge(bridge, gmrp_bridge);
  avl_tree_free (&gmrp_list, NULL);
  bridge->gmrp_list   = NULL;
  bridge->gmrp_bridge = NULL;

  return 0;
}

s_int32_t 
gmrp_add_port_cb ( struct nsm_bridge_master *master, 
                   char *bridge_name, struct interface *ifp)
{
  return gmrp_enable_port (master, ifp);
}

/* If there is a state change of the port, propagate join / leave events
 * according to Section 12.3.3.
 */
s_int32_t
gmrp_set_port_state (struct nsm_bridge_master *master,
                     struct interface *ifp)
{

  struct nsm_if *zif;
  struct gmrp *gmrp = 0;
  struct nsm_bridge *bridge = 0;
  struct avl_tree  *gmrp_list = NULL;
  struct gmrp_port *gmrp_port = 0;
  struct avl_tree  *gmrp_port_list = NULL;
  struct avl_node  *node = NULL;
  struct avl_node  *gmrp_node = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_port_instance *gmrp_port_instance = 0;

  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;

  /* Bridge does not exist, Global GMRP does not exist, Bridge Port
     does not exist, GMRP Port exist, GMRP Port creation failure */
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_list = bridge->gmrp_list)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      gmrp_port_instance = node->info;

      if (!gmrp_port_instance)
        continue;

      gmrp = gmrp_get_by_vid (gmrp_list, gmrp_port_instance->vlanid,
                              gmrp_port_instance->svlanid, &gmrp_node);

      if (!gmrp)
        continue;

      gid_set_propagate_port_state_event (gmrp_port_instance->gid, br_port->state); 
    }

  return 0;
} 

s_int32_t
gmrp_advertise_static_entry (struct nsm_bridge_master *master,
                             struct nsm_bridge *bridge, struct interface *ifp,
                             struct ptree *list, u_int16_t vid, u_int16_t svid)
{
  struct interface *ifnp = NULL;
  struct ptree_node *pn = NULL;
  struct nsm_bridge_static_fdb *static_fdb = NULL;
  struct nsm_bridge_fdb_ifinfo *pInfo = NULL;

  if ( !master || !ifp || !bridge || !list)
    return NSM_BRIDGE_ERR_GENERAL;

  for (pn = ptree_top (list); pn; pn = ptree_next (pn))
    {
      if (! (static_fdb = pn->info) )
        continue;

      pInfo = static_fdb->ifinfo_list;

      while (pInfo)
        {
          ifnp = if_lookup_by_index(&bridge->master->nm->vr->ifm,
                                   pInfo->ifindex);
          if (ifp == NULL)
            return NSM_BRIDGE_ERR_GENERAL;

          if (ifnp->ifindex == ifp->ifindex)
            gmrp_add_static_mac_addr (bridge->master, bridge->name, ifp,
                                      static_fdb->mac_addr, vid, svid,
                                      pInfo->is_forward);
          pInfo = pInfo->next_if;
        }
    }
  return 0;
};

s_int32_t 
gmrp_static_entry (struct nsm_bridge_master *master,
                   struct nsm_bridge *bridge, struct interface *ifp)
{
#ifdef HAVE_VLAN
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct avl_tree *vlan_table;
#endif /* HAVE_VLAN */

  int is_vlan_aware = PAL_FALSE;

  if ( !master || !ifp || !bridge)
    return NSM_BRIDGE_ERR_GENERAL;

  is_vlan_aware = nsm_bridge_is_vlan_aware (master, bridge->name);

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return NSM_GMRP_ERR_VLAN_NOT_FOUND;

  if ( !is_vlan_aware )
    {
      gmrp_advertise_static_entry (master, bridge, ifp,
                                   bridge->static_fdb_list,
                                   NSM_VLAN_NULL_VID,
                                   NSM_VLAN_NULL_VID);
    }
#ifdef HAVE_VLAN
  else
    {
      for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
        {
          if ((vlan = rn->info))
            gmrp_advertise_static_entry (master, bridge, ifp,
                                         vlan->static_fdb_list,
                                         vlan->vid, vlan->vid);

        }
    }
#endif /* HAVE_VLAN */

  return 0;
};

s_int32_t
gmrp_enable_port (struct nsm_bridge_master *master,
                  struct interface *ifp)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct gmrp *gmrp = 0;
  struct nsm_vlan_bmp nullmap;
  struct avl_node *node = NULL;
  struct nsm_bridge *bridge = 0;
  struct gmrp_port *gmrp_port = 0;
  struct avl_tree *gmrp_list = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp_port_config *gmrp_port_config = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  NSM_VLAN_BMP_INIT (nullmap);

  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;
  
  /* Bridge does not exist, Global GMRP does not exist, Bridge Port 
     does not exist, GMRP Port exist, GMRP Port creation failure */
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if (!zif->bridge)
    {
      if (!zif->gmrp_port_cfg)
        {
          gmrp_port_config = XCALLOC (MTYPE_GMRP_PORT_CONFIG,
                             sizeof (struct gmrp_port_config));
          if (gmrp_port_config)
            {
              gmrp_port_config->enable_port = PAL_TRUE;
              zif->gmrp_port_cfg = gmrp_port_config;
            }
        }
      else
        {
          zif->gmrp_port_cfg->enable_port = PAL_TRUE;
        }
    }

  if ( (!(bridge = zif->bridge)) ||
       (!(gmrp_list = bridge->gmrp_list)) ||
       (!(gmrp_bridge = bridge->gmrp_bridge)) ||
       (!(br_port = zif->switchport)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  vlan_port = & (br_port->vlan_port); 

  gmrp_port = br_port->gmrp_port;

  if (!gmrp_port)
    {
      if ( !gmrp_create_port (ifp, PAL_TRUE, &gmrp_port))
        return NSM_BRIDGE_ERR_GENERAL;
      br_port->gmrp_port = gmrp_port;
    }

  if (!gmrp_port->globally_enabled)
    return NSM_BRIDGE_ERR_GENERAL;

  if (!(gmrp_port_list = br_port->gmrp_port_list))
    {
      if ( (ret = avl_create (&gmrp_port_list, 0, gmrp_port_vid_cmp)) < 0 )
        return NSM_BRIDGE_ERR_GENERAL;

      br_port->gmrp_port_list = gmrp_port_list;
    }
  else
    {
      /* GMRP is already enabled on this port. So return SUCCESS */
      return 0;
    }

  if ((!NSM_VLAN_BMP_CMP(&nullmap, &(vlan_port->staticMemberBmp))) &&
       (!NSM_VLAN_BMP_CMP(&nullmap, &(vlan_port->dynamicMemberBmp))) )
    {
      if ( !(gmrp = gmrp_get_by_vid (gmrp_list, NSM_VLAN_DEFAULT_VID,
                                     NSM_VLAN_DEFAULT_VID, &node)) )
        {
          return NSM_BRIDGE_ERR_GENERAL;
        }

      if (!gmrp_create_port_instance (gmrp_port_list, gmrp_port->gid_port,
                                      gmrp, &gmrp_port_instance))
        {
          return NSM_BRIDGE_ERR_GENERAL;
        }

      return 0;
    }

  for (node = avl_top (gmrp_list); node; node = avl_next (node))
    {
      gmrp = AVL_NODE_INFO (node);

      if (gmrp)
        {
          if ( NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, gmrp->vlanid) ||
               NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, gmrp->vlanid) ||
               NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, gmrp->svlanid) ||
               NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, gmrp->svlanid))
            {
              if (!gmrp_create_port_instance (gmrp_port_list, gmrp_port->gid_port, gmrp, 
                                              &gmrp_port_instance))
                {
                  return NSM_BRIDGE_ERR_GENERAL;
                }
            }
        }
     }

  gmrp_static_entry (master, zif->bridge, ifp);

  if (zif->gmrp_port_cfg)
    {
      if (zif->gmrp_port_cfg->leave_all_timeout)
        {
          if (gmrp_check_timer_rules (master, ifp, GARP_LEAVE_ALL_TIMER,
                                      zif->gmrp_port_cfg->leave_all_timeout) ==
                                      RESULT_OK)
            {
              ret = gmrp_set_timer (master, ifp, GARP_LEAVE_ALL_TIMER,
                                    zif->gmrp_port_cfg->leave_all_timeout);
            }
        }

      if (zif->gmrp_port_cfg->leave_timeout)
        {
          if (gmrp_check_timer_rules (master, ifp, GARP_LEAVE_TIMER,
                                      zif->gmrp_port_cfg->leave_timeout) ==
                                      RESULT_OK)
            {
              ret = gmrp_set_timer (master, ifp, GARP_LEAVE_TIMER,
                                    zif->gmrp_port_cfg->leave_timeout);
            }
        }

      if (zif->gmrp_port_cfg->join_timeout)
        {
          if (gmrp_check_timer_rules (master, ifp, GARP_JOIN_TIMER,
                                      zif->gmrp_port_cfg->join_timeout) ==
                                      RESULT_OK)
            {
              ret = gmrp_set_timer (master, ifp, GARP_JOIN_TIMER,
                                    zif->gmrp_port_cfg->join_timeout);
            }
        }

      if (zif->gmrp_port_cfg->registration)
        {
          ret = gmrp_set_registration (master, ifp,
                                       zif->gmrp_port_cfg->registration);
        }

      if (zif->gmrp_port_cfg->fwd_all)
        {
          ret = gmrp_set_fwd_all (master ,ifp, GID_EVENT_JOIN);
        }

#ifdef HAVE_MMRP
      if (zif->gmrp_port_cfg->p2p == MMRP_POINT_TO_POINT_ENABLE)
        {
          ret = mmrp_set_pointtopoint (master, MMRP_POINT_TO_POINT_ENABLE, ifp);
        }
#endif /* HAVE_MMRP */
    }

  /* freeing the allocated memory */
  XFREE (MTYPE_GMRP_PORT_CONFIG, zif->gmrp_port_cfg);
  zif->gmrp_port_cfg = NULL;

#ifdef HAVE_ONMD
  nsm_oam_lldp_if_set_protocol_list (
                      ifp, (gmrp_bridge->reg_type == REG_TYPE_GMRP) ?
                      NSM_LLDP_PROTO_GMRP:NSM_LLDP_PROTO_MMRP,
                      PAL_TRUE);
#endif /* HAVE_ONMD */

  return 0;
}

s_int32_t
gmrp_enable_port_all (struct nsm_bridge_master *master)
{
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct avl_node *avl_port;
  struct interface *ifp;
  struct nsm_if *zif;
  s_int32_t ret = 0;

  if (! (bridge = nsm_get_first_bridge (master)))
    return NSM_BRIDGE_NOT_CFG;

  if (AVL_COUNT (bridge->port_tree) == 0)
    return NSM_BRIDGE_NO_PORT_CFG;

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
        
      ret = gmrp_enable_port (master, ifp);

      if (ret < 0)
        break;
    }

  return ret;
}

s_int32_t 
gmrp_delete_port_cb ( struct nsm_bridge_master *master, 
                      char *bridge_name, struct interface *ifp)
{
  return gmrp_disable_port (master, ifp);
}

/* When a port is shutdown only disable all the GMRP instances,
 * but retain the static GMRP configuration so that it can be 
 * enabled when the port is enabled again
 * 
 */

s_int32_t
gmrp_disable_port_cb (struct nsm_bridge_master *master,
                      struct interface *ifp)
{
  struct nsm_if *zif = NULL;
  struct avl_node  *node = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_node  *next_node = NULL;
  struct gmrp_port   *gmrp_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct avl_tree    *gmrp_port_list = NULL;
  struct gmrp_port_instance   *gmrp_port_instance = NULL;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist,
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  if ( (!(bridge = zif->bridge)) ||
       (!(gmrp_bridge = bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  for (node = avl_top (gmrp_port_list); node; node = next_node)
    {
      next_node = avl_next (node);

      gmrp_port_instance = AVL_NODE_INFO (node);

      if (!gmrp_port_instance)
        continue;

      gmrp_free_port_instance (gmrp_port_instance);
    }

  /* Remove all the nodes */
  avl_tree_cleanup (gmrp_port_list, NULL);

  return 0;

}

/* When a port is enabled, create the GMRP instances only when GMRP is enabled
 * on the port.
 */

s_int32_t
gmrp_enable_port_cb (struct nsm_bridge_master *master,
                     struct interface *ifp)
{
  struct gmrp *gmrp = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_bmp nullmap;
  struct avl_node *node = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct nsm_bridge *bridge = NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_tree  *gmrp_port_list = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  NSM_VLAN_BMP_INIT (nullmap);

  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;

  /* Bridge does not exist, Global GMRP does not exist, Bridge Port
     does not exist, GMRP Port does not exist, return failure */

  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_list = bridge->gmrp_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  vlan_port = & (br_port->vlan_port);

  if ((!NSM_VLAN_BMP_CMP(&nullmap, &(vlan_port->staticMemberBmp))) &&
       (!NSM_VLAN_BMP_CMP(&nullmap, &(vlan_port->dynamicMemberBmp))) )
    {
      if ( !(gmrp = gmrp_get_by_vid (gmrp_list, NSM_VLAN_DEFAULT_VID,
                                     NSM_VLAN_DEFAULT_VID, &node)) )
        {
          return NSM_BRIDGE_ERR_GENERAL;
        }

      if (!gmrp_create_port_instance (gmrp_port_list, gmrp_port->gid_port,
                                      gmrp, &gmrp_port_instance))
        {
          gmrp_disable_port (master, ifp);
          return NSM_BRIDGE_ERR_GENERAL;
        }

      return 0;
    }

  for (node = avl_top (gmrp_list); node; node = avl_next (node))
    {
      gmrp = AVL_NODE_INFO (node);

      if (gmrp)
        {
          if ( NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, gmrp->vlanid) ||
               NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, gmrp->vlanid) ||
               NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, gmrp->svlanid) ||
               NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, gmrp->svlanid))
            {
              if (!gmrp_create_port_instance (gmrp_port_list, gmrp_port->gid_port, gmrp,
                                              &gmrp_port_instance))
                {
                  gmrp_disable_port (master, ifp);
                  return NSM_BRIDGE_ERR_GENERAL;
                }
            }
        }
     }

  switch (gmrp_port->registration_cfg)
  {
    case GID_REG_MGMT_NORMAL:
      break;
    case GID_REG_MGMT_FIXED:
      gmrp_set_registration (master, ifp, GID_EVENT_FIXED_REGISTRATION);
      break;
    case GID_REG_MGMT_FORBIDDEN:
      gmrp_set_registration (master, ifp, GID_EVENT_FORBID_REGISTRATION);
      break;
    default:
      break;
  }

  if ( gmrp_port->forward_all_cfg & GMRP_MGMT_FORWARD_ALL_CONFIGURED)
    {
      gmrp_set_fwd_all (master, ifp, GID_EVENT_JOIN);
    }

  return 0;

}

s_int32_t
gmrp_disable_port (struct nsm_bridge_master *master,
                   struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct avl_node  *node = NULL;
  struct gmrp_port   *gmrp_port = 0;
  struct gmrp_bridge *gmrp_bridge = 0;
  struct nsm_bridge_port *br_port = NULL;
  struct avl_tree       *gmrp_port_list = NULL;
  struct gmrp_port_instance   *gmrp_port_instance = 0;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) ) 
    {
      return NSM_BRIDGE_ERR_GENERAL;    
    }
  
  if ( (!(bridge = zif->bridge)) ||
       (!(gmrp_bridge = bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
     
  for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      gmrp_port_instance = AVL_NODE_INFO (node);

      if (!gmrp_port_instance)
        continue;

      gmrp_free_port_instance (gmrp_port_instance);
    }

#ifdef HAVE_ONMD
  nsm_oam_lldp_if_set_protocol_list (
                      ifp, (gmrp_bridge->reg_type == REG_TYPE_GMRP) ?
                      NSM_LLDP_PROTO_GMRP:NSM_LLDP_PROTO_MMRP,
                      PAL_FALSE);
#endif /* HAVE_ONMD */

  avl_tree_free (&gmrp_port_list, NULL);
  br_port->gmrp_port_list = NULL;
  gmrp_destroy_port (gmrp_port);
  br_port->gmrp_port = NULL;

  return 0;
}

s_int32_t
gmrp_disable_port_all (struct nsm_bridge_master *master)
{
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct avl_node *avl_port;
  struct interface *ifp;
  struct nsm_if *zif;
  s_int32_t ret = 0;

  if (! (bridge = nsm_get_first_bridge (master)))
    return NSM_BRIDGE_NOT_CFG;

  if (AVL_COUNT (bridge->port_tree) == 0)
    return NSM_BRIDGE_NO_PORT_CFG;

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

      ret = gmrp_disable_port (master, ifp);

      if (ret < 0)
        break;
    }

  return ret;
}

s_int32_t
gmrp_enable_instance (struct nsm_bridge_master *master,
                      char *protocol, char*  bridge_name, u_int16_t vid)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct nsm_vlan nvlan;
  struct gmrp *gmrp = NULL;
  struct avl_node *an = NULL;
  s_int32_t    ret = RESULT_OK;
  struct avl_tree *vlan_table;
  struct avl_tree *gmrp_list = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *avl_port =  NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  char   gmrp_list_created = PAL_FALSE;
  char   gmrp_bridge_created = PAL_FALSE;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;

  if (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name)))
    return NSM_BRIDGE_ERR_NOTFOUND;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return NSM_GMRP_ERR_VLAN_NOT_FOUND;

  NSM_VLAN_SET(&nvlan, vid);

  an = avl_search (vlan_table, (void *)&nvlan);
  if (!an)
    return NSM_GMRP_ERR_VLAN_NOT_FOUND;

  if (!(gmrp_bridge = bridge->gmrp_bridge) )
    {
     if (!gmrp_create_gmrp_bridge (bridge, PAL_FALSE, protocol, &gmrp_bridge))
       return NSM_BRIDGE_ERR_MEM;
     bridge->gmrp_bridge = gmrp_bridge;
     gmrp_bridge_created = PAL_TRUE;
    }
  else if (gmrp_bridge->globally_enabled)
    {
      return NSM_GMRP_ERR_GMRP_GLOBAL_CFG_BR;
    }

  if (!(gmrp_list = bridge->gmrp_list) )
    {
      if ( (ret = avl_create (&gmrp_list, 0, gmrp_vid_cmp)) < 0 )
        {
          gmrp_destroy_gmrp_bridge (bridge, gmrp_bridge);
          bridge->gmrp_bridge = NULL;
          return NSM_BRIDGE_ERR_MEM;
        }
      bridge->gmrp_list = gmrp_list;
      gmrp_list_created = PAL_TRUE;
    }


  if (!(gmrp_create_gmrp_instance (gmrp_list, gmrp_bridge, vid,
                                   vid, &gmrp)))
    {
      if (gmrp_bridge_created)
        {
          gmrp_destroy_gmrp_bridge (bridge, gmrp_bridge);
          bridge->gmrp_bridge = NULL;
        }
      if (gmrp_list_created)
        {
          avl_tree_free (&gmrp_list, NULL);
          bridge->gmrp_list = NULL;
        }
      return NSM_BRIDGE_ERR_MEM;
    }

  /* Look for all the ports with GMRP globally enabled and
   * enable this instance if vlan is configured for this port.
   */
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ( ((zif = br_port->nsmif) == NULL)
           || ((ifp = zif->ifp) == NULL) )
        continue;

      if (!(vlan_port = &br_port->vlan_port))
        continue;

      gmrp_port_list = br_port->gmrp_port_list;
      gmrp_port      = br_port->gmrp_port;

      if (!gmrp_port || !gmrp_port_list || !gmrp_port->globally_enabled)
        continue;

      if ( NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, gmrp->vlanid) ||
           NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, gmrp->vlanid) )
        {

           ret = gmrp_add_vlan_to_port (bridge, ifp, vid, vid);

           if (ret < 0 )
             {
               gmrp_disable_instance (master, bridge_name, vid);
               return NSM_BRIDGE_ERR_MEM;
             }
        }
     }

  if (gmrp_list_created)
    {
      /* Inform forwarder that gmrp has been configured on this bridge*/
      hal_garp_set_bridge_type (bridge->name, HAL_GMRP_CONFIGURED, PAL_TRUE);
    }

  return RESULT_OK;

}

s_int32_t
gmrp_disable_instance (struct nsm_bridge_master *master,
                       char*  bridge_name, u_int16_t vid)
{
  struct nsm_vlan nvlan;
  struct avl_node *an = NULL;
  struct gmrp *old_gmrp = NULL;
  struct avl_node  *node = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct avl_node   *avl_port =  NULL;
  struct gmrp_port  *gmrp_port = NULL;
  struct avl_tree   *vlan_table = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct avl_tree   *gmrp_port_list = NULL;

  if (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name)))
    return NSM_BRIDGE_ERR_NOTFOUND;

  NSM_VLAN_SET(&nvlan, vid);

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return NSM_GMRP_ERR_VLAN_NOT_FOUND;

  an = avl_search (vlan_table, (void *)&nvlan);
  if (!an)
    return NSM_GMRP_ERR_VLAN_NOT_FOUND;

  if (!(gmrp_bridge = bridge->gmrp_bridge) ||
      !(gmrp_list = bridge->gmrp_list ) )
    {
       return NSM_BRIDGE_ERR_GMRP_NOCONFIG;
    }

  old_gmrp = gmrp_get_by_vid (gmrp_list, vid, vid, &node);

  if (old_gmrp)
    {
      gmrp_destroy_gmrp_instance (old_gmrp);
    }

  for (node = avl_top (gmrp_list); node ; node = avl_next (node))
    {
      if (AVL_NODE_INFO (node))
        return RESULT_OK;
    }

  /* If no context is active free all the gmrp_port_list and gmrp_port
   * strutures.
   */

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {

      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      gmrp_port_list = br_port->gmrp_port_list;
      gmrp_port      = br_port->gmrp_port;

      if (!gmrp_port || !gmrp_port_list || gmrp_port->globally_enabled)
        continue;

      gmrp_destroy_port (gmrp_port);
      avl_tree_free (&gmrp_port_list, NULL);
      br_port->gmrp_port_list = NULL;
      br_port->gmrp_port = NULL;

    }

  gmrp_destroy_gmrp_bridge (bridge, gmrp_bridge);

  avl_tree_free (&gmrp_list, NULL);
  bridge->gmrp_list =  NULL;
  bridge->gmrp_bridge = NULL;

  /* Inform forwarder that gmrp has been disabled on this bridge*/
  hal_garp_set_bridge_type (bridge->name, HAL_GMRP_CONFIGURED, PAL_FALSE);

  return RESULT_OK;

}

s_int32_t
gmrp_enable_port_instance (struct nsm_bridge_master *master,
                           struct interface *ifp,
                           u_int16_t vid)
{
  s_int32_t ret;
  struct gmrp *gmrp = NULL;
  struct nsm_if *zif = NULL;
  struct avl_node  *node = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct gmrp_port *gmrp_port = NULL;
  char   gmrp_port_created = PAL_FALSE;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct avl_tree  *gmrp_port_list = NULL;
  char   gmrp_port_list_created = PAL_FALSE;

  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;

  /* Bridge does not exist, Global GMRP does not exist, Bridge Port
     does not exist, GMRP Port exist, GMRP Port creation failure */
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_list = bridge->gmrp_list)) ||
       (!(gmrp_bridge = bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GMRP_NOCONFIG;
    }

  vlan_port = & (br_port->vlan_port);

  gmrp = gmrp_get_by_vid (gmrp_list, vid, vid, &node);

  if (!gmrp)
    {
      return NSM_GMRP_ERR_GMRP_NOT_CFG_ON_VLAN;
    }

  if (! (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid) ||
         NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vid)) )
    {
      return NSM_GMRP_ERR_VLAN_NOT_CFG_ON_PORT;
    }

  gmrp_port_list = br_port->gmrp_port_list;

  if (! gmrp_port_list)
   {
     if ((ret = avl_create (&gmrp_port_list, 0, gmrp_port_vid_cmp)) < 0)
       {
         return NSM_BRIDGE_ERR_MEM;
       }

     br_port->gmrp_port_list = gmrp_port_list;
     gmrp_port_list_created = PAL_TRUE;
   }


  gmrp_port = br_port->gmrp_port;

  if (!gmrp_port)
    {
      if ( !gmrp_create_port (ifp, PAL_FALSE, &gmrp_port))
        {
          if (gmrp_port_list_created)
            {
              avl_tree_free (&gmrp_port_list, NULL);
              br_port->gmrp_port_list = NULL;
            }
          return NSM_BRIDGE_ERR_MEM;
        }

      br_port->gmrp_port = gmrp_port;
      gmrp_port_created = PAL_TRUE;
    }
  else if (gmrp_port->globally_enabled)
    {
       return NSM_GMRP_ERR_GMRP_GLOBAL_CFG_PORT;
    }

  ret = gmrp_add_vlan_to_port (bridge, ifp, vid, vid);

  if (ret < 0)
    {
      if (gmrp_port_created)
        {
          gmrp_destroy_port (gmrp_port);
          br_port->gmrp_port = NULL;
        }
      if (gmrp_port_list_created)
        {
          avl_tree_free (&gmrp_port_list, NULL);
          br_port->gmrp_port_list = NULL; 
        }
      return NSM_BRIDGE_ERR_MEM;
    }

  return RESULT_OK;
}

s_int32_t
gmrp_disable_port_instance (struct nsm_bridge_master *master,
                            struct interface *ifp,
                            u_int16_t vid)
{
  s_int32_t ret;
  struct gmrp *gmrp = NULL;
  struct nsm_if *zif = NULL;
  struct avl_node    *node = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;

  /* Bridge does not exist, Global GMRP does not exist, Bridge Port
     does not exist, GMRP Port exist, GMRP Port creation failure */
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_list = bridge->gmrp_list)) ||
       (!(gmrp_bridge = bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GMRP_NOCONFIG;
    }

  vlan_port = & (br_port->vlan_port);

  gmrp = gmrp_get_by_vid (gmrp_list, vid, vid, &node);

  if (!gmrp)
    {
      return NSM_GMRP_ERR_GMRP_NOT_CFG_ON_VLAN;
    }

  if (! (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid) ||
         NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp, vid)) )
    {
      return NSM_GMRP_ERR_VLAN_NOT_CFG_ON_PORT;
    }

  gmrp_port = br_port->gmrp_port;
  gmrp_port_list = br_port->gmrp_port_list;

  if (!gmrp_port || !gmrp_port_list)
    {
      return NSM_ERR_GMRP_NOCONFIG_ONPORT;
    }

  gmrp_port_instance =  gmrp_port_instance_get_by_vid (gmrp_port_list,
                                                       vid, vid, &node);

  if (!gmrp_port_instance)
    {
       return NSM_GMRP_ERR_GMRP_NOT_CFG_ON_PORT_VLAN;
    }

  ret = gmrp_remove_vlan_from_port (bridge, zif, vid, vid);

  if (ret < 0)
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      if (AVL_NODE_INFO (node))
        return RESULT_OK;
    }

  gmrp_destroy_port (gmrp_port);
  avl_tree_free (&gmrp_port_list, NULL);

  br_port->gmrp_port = NULL;
  br_port->gmrp_port_list = NULL;

  return RESULT_OK;
}

extern s_int32_t
gmrp_get_port_details (struct nsm_bridge_master *master,
                       struct interface *ifp, u_char* flags)
{
  /*
  ---------------------------------
  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  ---------------------------------
  7   -> Is the port configured
  0-1 -> registration type of other attributes (multicasr address) 
         for the given port
  2   -> Is fwd_all set
  3-6 -> Unused
  */
  bool_t ret;
  struct nsm_if *zif;
  struct gid_states states;
  struct nsm_bridge *bridge = 0;
  struct gmrp_bridge *gmrp_bridge = 0;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_port   *gmrp_port   = NULL;
  struct avl_tree    *gmrp_port_list = NULL;
  struct gmrp_port_instance   *gmrp_port_instance   = NULL;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info)) ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port = br_port->gmrp_port)))
    {
      return NSM_BRIDGE_ERR_GENERAL;    
    }

  if ( (!(bridge = zif->bridge)) ||
       (!(gmrp_bridge = bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  *flags = 0x00;

  if (br_port->gmrp_port)
    {
      *flags |= GMRP_MGMT_PORT_CONFIGURED;
    }

  if ( (gmrp_port_list = br_port->gmrp_port_list) &&
       (gmrp_port_instance = (struct gmrp_port_instance *)
                             gmrp_get_first_node (gmrp_port_list) ) )
    {
      ret = gid_get_attribute_states (gmrp_port_instance->gid,
                                      GMRP_NUMBER_OF_LEGACY_CONTROL,
                                      &states);

      *flags |= (states.registrar_mgmt);

      ret = gid_get_attribute_states (gmrp_port_instance->gid,
                                      GMRP_LEGACY_FORWARD_ALL,
                                      &states);

      if (!ret)
        return PAL_FALSE;

      if (gmrp_port->flags)
        {
          *flags |= GMRP_MGMT_FORWARD_ALL_CONFIGURED;
        }

     if (gmrp_port->registration_cfg & GID_REG_MGMT_FORBIDDEN)
        {
          *flags |= GMRP_MGMT_FORWARD_ALL_FORBIDDEN;
        }
 
     
     if (gmrp_port->flags & GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED)
       {
        *flags |= GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED;
       }
     else if (gmrp_port->flags & GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN)
       {
        *flags |= GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN;
       }
    }
  
  return 0;
}
    
s_int32_t
gmrp_set_registration (struct nsm_bridge_master *master,
                       struct interface *ifp, const u_int32_t registration_type)
{ 
  struct nsm_if *zif;
  struct gmrp *gmrp;
  struct gmrp_gmd *gmd;
  struct ptree_node *rn;
  u_char registration_cfg;
  u_int32_t attribute_index;
  struct avl_tree  *gmrp_list;
  struct avl_node *node = NULL;
  struct nsm_bridge *bridge = 0;
  struct gmrp_port *gmrp_port = 0;
  struct avl_node *gmrp_node = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) ) 
    {
      return NSM_ERR_GMRP_NOCONFIG_ONPORT;    
    }

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
  gmrp_list = bridge->gmrp_list;

  switch (registration_type)
  {
    case GID_EVENT_NORMAL_REGISTRATION:
      registration_cfg = GID_REG_MGMT_NORMAL;
      break;
    case GID_EVENT_FIXED_REGISTRATION:
      registration_cfg = GID_REG_MGMT_FIXED;
      break;
    case GID_EVENT_FORBID_REGISTRATION:
      registration_cfg = GID_REG_MGMT_FORBIDDEN;
      break;
    case GID_EVENT_RESTRICTED_GROUP_REGISTRATION:
       registration_cfg = GID_REG_MGMT_RESTRICTED_GROUP;
       break;
    default:
      return NSM_BRIDGE_ERR_GENERAL;
      break;
  }

  gmrp_port->registration_cfg = registration_cfg;

  for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      gmrp_port_instance = AVL_NODE_INFO (node);
      if (gmrp_port_instance == NULL)
        continue;

      gmrp = gmrp_get_by_vid (gmrp_list, gmrp_port_instance->vlanid,
                              gmrp_port_instance->svlanid, &gmrp_node);

      if ((gmrp == NULL) || (gmrp->gmd == NULL))
        continue;

      gmd = gmrp->gmd;

      for (rn = ptree_top (gmd->gmd_entry_table); rn;
           rn = ptree_next (rn))
        {
          attribute_index = gid_get_attr_index (rn);
          gid_set_attribute_states (gmrp_port_instance->gid, attribute_index, 
                                    registration_type);
        }
    }

  return 0;
}

s_int32_t
gmrp_set_timer (struct nsm_bridge_master *master,
                struct interface *ifp,
                const u_int32_t timer_type,
                const pal_time_t timer_value)
{
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gmrp_port *gmrp_port = 0;
  struct nsm_bridge_port *br_port = NULL;
  struct avl_tree        *gmrp_port_list = NULL;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) ) 
    {
      return NSM_ERR_GMRP_NOCONFIG_ONPORT;    
    }
  
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  gid_set_timer (gmrp_port->gid_port, timer_type, timer_value);

  return 0;
}

/* Function returns the timer value associated with the timer_type 
   for the port 
*/
s_int32_t
gmrp_get_timer (struct nsm_bridge_master *master,
                struct interface *ifp,
                const u_int32_t timer_type,
                pal_time_t *timer_value)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct gmrp_port *gmrp_port = 0;
  struct nsm_bridge_port *br_port = 0;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port = br_port->gmrp_port)) ) 
    {
      return NSM_BRIDGE_ERR_GENERAL;    
    }
  
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
  gid_get_timer (gmrp_port->gid_port, timer_type, timer_value);

  return 0;
}

s_int32_t
gmrp_get_timer_details (struct nsm_bridge_master *master,
                        struct interface *ifp, pal_time_t* timer_details)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct gmrp_port *gmrp_port = 0;
  struct nsm_bridge_port *br_port = 0;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port = br_port->gmrp_port)) ) 
    {
      return NSM_BRIDGE_ERR_GENERAL;    
    }
  
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
  gid_get_timer_details (gmrp_port->gid_port, timer_details);

  return 0;
}

s_int32_t
gmrp_get_per_vlan_statistics_details (struct nsm_bridge_master *master,
                                      const char* const bridge_name,
                                      const u_int16_t vid,
                                      u_int32_t *receive_counters,
                                      u_int32_t *transmit_counters)
{
  struct nsm_bridge *bridge = 0;
  struct gmrp_bridge *gmrp_bridge = 0;
  struct avl_tree *gmrp_list = NULL;
  struct gmrp *gmrp = NULL;
  struct avl_node *node = NULL;

  /* Bridge does not exist, Global GMRP exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
      (!(gmrp_bridge = bridge->gmrp_bridge)) ||
      (!(gmrp_list = bridge->gmrp_list)) ) 
  {
    return NSM_BRIDGE_ERR_GENERAL;    
  }

  gmrp = gmrp_get_by_vid (gmrp_list, vid, vid, &node);

  if (!gmrp)
    return RESULT_ERROR;

#ifdef HAVE_MMRP
  pal_mem_cpy (receive_counters, gmrp->garp_instance.receive_counters,
               (MRP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));

  pal_mem_cpy (transmit_counters, gmrp->garp_instance.transmit_counters,
               (MRP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
#else
  pal_mem_cpy (receive_counters, gmrp->garp_instance.receive_counters,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));

  pal_mem_cpy (transmit_counters, gmrp->garp_instance.transmit_counters,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
#endif /* HAVE_MMRP */

  return 0;
}

s_int32_t
gmrp_clear_all_statistics (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm); 
  struct nsm_bridge *bridge;
  
  /* Bridge does not exist, Global GMRP exist */
  for (bridge=nsm_get_first_bridge(master);
      bridge; bridge = nsm_get_next_bridge (master, bridge))
     gmrp_clear_bridge_statistics(master, bridge->name);

  return 0;
}

s_int32_t
gmrp_clear_bridge_statistics (struct nsm_bridge_master *master,
                              const char * const bridge_name)
{
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp *gmrp = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct nsm_bridge *bridge = 0;
  struct avl_node *node = NULL;
   
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
      (!(gmrp_list = bridge->gmrp_list)) || 
      (!(gmrp_bridge = bridge->gmrp_bridge)) )
  {
    return NSM_BRIDGE_ERR_GENERAL;    
  }

  for (node = avl_top (gmrp_list); node; node = avl_next (node))
    {
      gmrp = AVL_NODE_INFO (node);
      if (gmrp)
        {
          pal_mem_set (gmrp->garp_instance.receive_counters, 0,
                      (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
          pal_mem_set (gmrp->garp_instance.transmit_counters, 0,
                      (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
        }
    }

  return 0;
}

s_int32_t
gmrp_clear_per_vlan_statistics (struct nsm_bridge_master *master,
                                const char* const bridge_name,
                                const u_int16_t vid)
{
  struct nsm_bridge *bridge = 0;
  struct gmrp_bridge *gmrp_bridge = NULL;
  struct gmrp *gmrp = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct avl_node *node = NULL;
  
  /* Bridge does not exist, Global GMRP exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
      (!(gmrp_bridge = bridge->gmrp_bridge)) ||
      (!(gmrp_list = bridge->gmrp_list)) ) 
  {
    return NSM_BRIDGE_ERR_GENERAL;    
  }

  gmrp = gmrp_get_by_vid (gmrp_list, vid, vid, &node);

  pal_mem_set( gmrp->garp_instance.receive_counters, 0,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
  pal_mem_set( gmrp->garp_instance.transmit_counters, 0,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));

  return 0;
}

int
gmrp_set_fwd_unregistered_forbid (struct nsm_bridge_master *master,
                                  struct interface *ifp,
                                  const u_int32_t event)
{
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gmrp_port *gmrp_port = 0;
  struct avl_tree *gmrp_port_list = NULL;
  struct avl_node *node = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;
  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist,
     GMRP Port does exist */
 if ((!(zif = (struct nsm_if *)ifp->info))  ||
      (!(gmrp_port_list = zif->switchport->gmrp_port_list)) ||
      (!(gmrp_port = zif->switchport->gmrp_port)))
    {
      return NSM_ERR_GMRP_NOCONFIG_ONPORT;
    }

  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  switch(event)
    {
      case GID_EVENT_FORBID_REGISTRATION:
        gmrp_port->forward_unregistered_cfg = 
          GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN; 
        gmrp_port->flags |= GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN;
        break;

      case GID_EVENT_LEAVE:
        gmrp_port->forward_unregistered_cfg = 
          ~GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN; 
        gmrp_port->flags &= 
          ~GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN;
        break;

      default:
        return NSM_BRIDGE_ERR_GENERAL;
    }

  for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      gmrp_port_instance = node->info;
      if (! gmrp_port_instance)
        continue;
      gid_set_attribute_states (gmrp_port_instance->gid, 
                               GMRP_LEGACY_FORWARD_UNREGISTERED, event);

    }
  return 0;
}

int
gmrp_set_fwd_unregistered (struct nsm_bridge_master *master,
                           struct interface *ifp, const u_int32_t event)
{
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gmrp_port *gmrp_port = 0;
  struct avl_tree *gmrp_port_list = NULL;
  struct avl_node *node = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;
  int ret;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist,
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(gmrp_port_list = zif->switchport->gmrp_port_list)) ||
       (!(gmrp_port = zif->switchport->gmrp_port)) )
    {
      return NSM_ERR_GMRP_NOCONFIG_ONPORT;
    }

  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  switch(event)
    {
      case GID_EVENT_JOIN:
        gmrp_port->forward_unregistered_cfg = GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED;
        gmrp_port->flags |= GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED;
        break;
      case GID_EVENT_LEAVE:
        gmrp_port->forward_unregistered_cfg = ~GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED;
        gmrp_port->flags &= ~GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED;
        break;
      default:
        return NSM_BRIDGE_ERR_GENERAL;
    }
 
   for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      gmrp_port_instance = node->info;
      if (! gmrp_port_instance)
        continue;
      gid_set_attribute_states (gmrp_port_instance->gid, GMRP_LEGACY_FORWARD_UNREGISTERED, event);

      if (event == GID_EVENT_JOIN)
         ret =  hal_l2_gmrp_set_service_requirement
                  (bridge->name, gmrp_port_instance->vlanid,ifp->ifindex,
                   HAL_GMRP_LEGACY_FORWARD_UNREGISTERED, PAL_TRUE);

      else if (event == GID_EVENT_LEAVE)
        ret = hal_l2_gmrp_set_service_requirement
                (bridge->name, gmrp_port_instance->vlanid, ifp->ifindex, 
                 HAL_GMRP_LEGACY_FORWARD_UNREGISTERED, PAL_FALSE);
    }
  return 0;
}

int
gmrp_set_fwd_all_forbid (struct nsm_bridge_master *master,
    struct interface *ifp, const u_int32_t event)
{
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gmrp_port *gmrp_port = 0;
  struct avl_tree *gmrp_port_list = NULL;
  struct avl_node *node = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist,
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(gmrp_port_list = zif->switchport->gmrp_port_list)) ||
       (!(gmrp_port = zif->switchport->gmrp_port)) )
    {
      return NSM_ERR_GMRP_NOCONFIG_ONPORT;
    }

  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  switch(event)
    {
      case GID_EVENT_FORBID_REGISTRATION:
        gmrp_port->flags |= GMRP_MGMT_FORWARD_ALL_FORBIDDEN;
        break;
      
      default:
        return NSM_BRIDGE_ERR_GENERAL;
    }


  for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      gmrp_port_instance = node->info;
      if (! gmrp_port_instance)
        continue;
      gid_set_attribute_states (gmrp_port_instance->gid, GMRP_LEGACY_FORWARD_ALL, event);
    }

  return 0;
}
s_int32_t
gmrp_set_fwd_all (struct nsm_bridge_master *master,
                  struct interface *ifp, const u_int32_t event)
{
  s_int32_t ret;
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gmrp_port *gmrp_port = 0;
  struct avl_node *node = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gmrp_port_instance *gmrp_port_instance = NULL;

  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) ) 
    {
      return NSM_ERR_GMRP_NOCONFIG_ONPORT;    
    }
  
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gmrp_bridge)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  switch(event)
    {
      case GID_EVENT_JOIN:
        gmrp_port->forward_all_cfg = GMRP_MGMT_FORWARD_ALL_CONFIGURED;
        gmrp_port->flags |= GMRP_MGMT_FORWARD_ALL_CONFIGURED;
        break;
      case GID_EVENT_LEAVE:
        gmrp_port->forward_all_cfg = ~GMRP_MGMT_FORWARD_ALL_CONFIGURED;
        gmrp_port->flags &= ~GMRP_MGMT_FORWARD_ALL_CONFIGURED;
        break;
      default:
        return NSM_BRIDGE_ERR_GENERAL;
    }


  for (node = avl_top (gmrp_port_list); node; node = avl_next (node))
    {
      gmrp_port_instance = AVL_NODE_INFO (node);

      if (! gmrp_port_instance)
        continue;

      gid_set_attribute_states (gmrp_port_instance->gid,
                                GMRP_LEGACY_FORWARD_ALL, event);
  
      if (event == GID_EVENT_JOIN)
        ret = hal_l2_gmrp_set_service_requirement
                     (bridge->name, gmrp_port_instance->vlanid,
                      ifp->ifindex, HAL_GMRP_LEGACY_FORWARD_ALL, PAL_TRUE);

      else if (event == GID_EVENT_LEAVE)
        ret = hal_l2_gmrp_set_service_requirement
                     (bridge->name, gmrp_port_instance->vlanid,
                      ifp->ifindex, HAL_GMRP_LEGACY_FORWARD_ALL, PAL_FALSE);
        
    }

  return 0;
}

s_int32_t
gmrp_is_configured (struct nsm_bridge_master *master,
                    const char* const bridge_name, u_char *is_gmrp_enabled)
{
  struct nsm_bridge *bridge;

  /* Bridge does not exist */
  if ( !(bridge =  nsm_lookup_bridge_by_name (master, bridge_name)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;    
    }

  *is_gmrp_enabled = ((bridge->gmrp_bridge) != 0);

  return 0;
}

struct gmrp *
gmrp_get_by_vid (struct avl_tree *gmrp_list, u_int16_t vlan,
                 u_int16_t svlan, struct avl_node **node)
{

  struct gmrp key;

  key.vlanid = vlan;
  key.svlanid = svlan;

  *node  = avl_search (gmrp_list, (void *) &key);

  if (! (*node))
     return NULL;

  return ( AVL_NODE_INFO ((*node)) ) ;

}

struct gmrp_port_instance *
gmrp_port_instance_get_by_vid (struct avl_tree *gmrp_port_list, u_int16_t vlan,
                               u_int16_t svlan, struct avl_node **node )
{
  struct gmrp_port_instance key;

  key.vlanid = vlan;
  key.svlanid = svlan;

  *node  = avl_search (gmrp_port_list, (void *) &key);

  if (! (*node) )
    return NULL;

  return ( AVL_NODE_INFO (*node) ) ;

}

void
gmrp_activate_port (struct nsm_bridge_master *master, struct interface *ifp)
{
  struct nsm_if *nif = NULL;
  s_int32_t ret = 0;

  if (!ifp)
    return ;

  nif = (struct nsm_if*)ifp->info;

  if (nif == NULL)
    return;

  if (nif && nif->ifp)
    ifp = nif->ifp;

  if (nif->bridge && nif->gmrp_port_cfg)
    {
      if (nif->gmrp_port_cfg->enable_port == PAL_TRUE)
        {
          ret = gmrp_enable_port (master, ifp);
        }
      else
        {
          ret = gmrp_disable_port (master, ifp);
        }
    }
}

#ifdef HAVE_MMRP

s_int32_t
mmrp_set_pointtopoint(struct nsm_bridge_master *master, u_char ctrl, 
                      struct interface *ifp)
{
  
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct gid_port *gid_port = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_bridge_port *br_port = NULL;

  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gmrp_list = bridge->gmrp_list)) ||
       (!(gmrp_port = br_port->gmrp_port)) ||
       (!(gid_port = gmrp_port->gid_port)) ||
       (!(gmrp_port_list = br_port->gmrp_port_list)))
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  gid_port->p2p = ctrl;
   
  return 0;
}

#endif /* HAVE_MMRP */
