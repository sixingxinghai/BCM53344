/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "table.h"
#include "avl_tree.h"

#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"

#include "nsm_message.h"
#include "nsm_bridge_cli.h"
#include "nsm_vlan_cli.h"
#include "nsm_interface.h"
#include "gvrp.h"

#include "gvrp_api.h"
#include "gvrp_database.h"
#include "gvrp_static.h"
#include "gvrp_cli.h"
#include "hal_garp.h"

#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */


void
gvrp_zlog_process_result (int result,
                          struct interface  *ifp,
                          char *bridge_name);

int
gvrp_check_timer_rules(struct nsm_bridge_master *master,
                       struct interface *ifp,
                       const u_int32_t timer_type,
                       const pal_time_t timer_value)
{
  pal_time_t timer_details[GARP_MAX_TIMERS];

  if (gvrp_get_timer_details (master, ifp, timer_details) != RESULT_OK)
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

int
gvrp_enable (struct nsm_bridge_master *master,
             char* reg_type,
             const char* const bridge_name)
{
  struct gvrp *gvrp;
  struct nsm_bridge *bridge;
  u_int16_t vid;
  
  vid = 1;
  bridge = NULL;
  gvrp = NULL;
  
  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if ( !bridge )
    return NSM_BRIDGE_ERR_NOTFOUND;

  if ( nsm_bridge_is_vlan_aware (master, bridge->name) != PAL_TRUE  )
    return NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE;
  
  gvrp = bridge->gvrp;
  if (gvrp != NULL)
    {
      /* GVRP instance already exists for the bridge  */
      return 0;
    }
  
  if (!gvrp_create_gvrp (bridge, vid, &gvrp))
    return NSM_BRIDGE_ERR_MEM;

  XVRP_UPDATE_PROTOCOL(gvrp, reg_type);

  bridge->gvrp = gvrp;
  
  gvrp_check_static_vlans(master, bridge);

  /* Inform forwarder that gvrp has been configured on this bridge*/
  hal_garp_set_bridge_type (bridge->name, (gvrp->reg_type == REG_TYPE_GVRP)?
                            HAL_GVRP_CONFIGURED:HAL_MVRP_CONFIGURED, PAL_TRUE);
 
  return 0;
}

int
gvrp_disable (struct nsm_bridge_master *master,
              char* bridge_name)
{
  struct nsm_bridge *bridge;
  struct gvrp *gvrp;
  
  /* Bridge does not exist, Global GMRP does not exist */
  if ((!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
      (!(gvrp = bridge->gvrp)))
    {
       return NSM_BRIDGE_ERR_NOTFOUND;
    }
  
  gvrp_destroy_gvrp (master, bridge_name);
  bridge->gvrp = NULL;
  /* Inform forwarder that gvrp has been disabled on this bridge*/
  hal_garp_set_bridge_type (bridge->name, HAL_GVRP_CONFIGURED, PAL_FALSE);
  
  return 0;
}

/* If there is a state change of the port, propagate join / leave events
 * according to Section 12.3.3. 
 */ 

int
gvrp_set_port_state (struct nsm_bridge_master *master,
                     struct interface *ifp)
{
  struct nsm_if *zif;
  struct gvrp *gvrp = 0;
  struct nsm_bridge *bridge = 0;
  struct nsm_bridge_port *br_port;
  struct gvrp_port *gvrp_port = 0;

  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;

  /* Bridge does not exist, Global GMRP does not exist, Bridge Port
     does not exist, GMRP Port exist, GMRP Port creation failure */
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp = bridge->gvrp)) ||
       (!(gvrp_port= br_port->gvrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  gid_set_propagate_port_state_event (gvrp_port->gid, br_port->state);

  return 0;
}

int
gvrp_enable_port (struct nsm_bridge_master *master,
                  struct interface *ifp)
{
  struct nsm_bridge *bridge = 0;
  struct gvrp *gvrp = 0;
  struct nsm_if *zif;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_bridge_port *br_port = 0;
  struct gvrp_port_config *gvrp_port_record = NULL;
  int ret;
  
  if ( !master || !ifp )
    return NSM_BRIDGE_ERR_GENERAL;
  
  /* Bridge does not exist, Global GVRP does not exist, Bridge Port 
     does not exist, GVRP Port exist, GVRP Port creation failure */
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if (zif->bridge == NULL)
    {
      if (zif->gvrp_port_config == NULL)
        {
          gvrp_port_record = XCALLOC (MTYPE_GVRP_PORT_CONFIG,
                                      sizeof (struct gvrp_port_config));
          if (gvrp_port_record)
            {
              gvrp_port_record->enable_port = PAL_TRUE;
              zif->gvrp_port_config = gvrp_port_record;
            }
        }
      else
        {
          zif->gvrp_port_config->enable_port = PAL_TRUE;
        }
    }
  
  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp = bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

#ifdef HAVE_PVLAN
  if (br_port->vlan_port.pvlan_configured)
    return NSM_PVLAN_ERR_CONFIGURED;
#endif /* HAVE_PVLAN */

#ifdef HAVE_PROVIDER_BRIDGE
  if (bridge->provider_edge == PAL_TRUE
      && br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_CE)
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
#endif /* HAVE_PROVIDER_BRIDGE */

  if(  (!(gvrp_port = br_port->gvrp_port) && 
       (PAL_FALSE == gvrp_create_port (gvrp, ifp, 
                                       NSM_VLAN_DEFAULT_VID, &gvrp_port))) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  /* Set the newly created gvrp port to the port structure */ 
  br_port->gvrp_port = gvrp_port;

  if ( (gvrp_port_check_static_vlans (master, bridge, ifp)) < 0 )
    return NSM_BRIDGE_ERR_GENERAL;

  gvrp_port_create_static_vlans (bridge, gvrp, gvrp_port);

  /* If it a trunk ot hybrid port add vlans in other ports
   * into the applicant state.
   */

  gvrp_change_port_mode (master, bridge->name, ifp);

  if (zif->gvrp_port_config)
    {
      if ( zif->gvrp_port_config->timer_details[GARP_LEAVE_ALL_TIMER] )
        {
          if (gvrp_check_timer_rules( master, ifp, GARP_LEAVE_ALL_TIMER,
                                      zif->gvrp_port_config->timer_details[
                                      GARP_LEAVE_ALL_TIMER] ) == RESULT_OK)
            {
              ret = gvrp_set_timer ( master, ifp, GARP_LEAVE_ALL_TIMER,
                                     zif->gvrp_port_config->timer_details[
                                     GARP_LEAVE_ALL_TIMER] );
            }
        }

      if ( zif->gvrp_port_config->timer_details[GARP_LEAVE_TIMER] )
        {
          if (gvrp_check_timer_rules( master, ifp, GARP_LEAVE_TIMER,
                                      zif->gvrp_port_config->timer_details[
                                      GARP_LEAVE_TIMER] ) == RESULT_OK)
            {
              ret = gvrp_set_timer ( master, ifp, GARP_LEAVE_TIMER,
                                     zif->gvrp_port_config->timer_details[
                                     GARP_LEAVE_TIMER] );
            }
        }

      if ( zif->gvrp_port_config->timer_details[GARP_JOIN_TIMER] )
        {
          if (gvrp_check_timer_rules( master, ifp, GARP_JOIN_TIMER,
                                      zif->gvrp_port_config->timer_details[
                                      GARP_JOIN_TIMER] ) == RESULT_OK)
            {
              ret = gvrp_set_timer ( master, ifp, GARP_JOIN_TIMER,
                                     zif->gvrp_port_config->timer_details[
                                     GARP_JOIN_TIMER] );
            }
        }

      if (zif->gvrp_port_config->registration_mode)
        {
          ret = gvrp_set_registration (master, ifp,
                zif->gvrp_port_config->registration_mode);
          gvrp_zlog_process_result (ret, ifp, 0);
        }

      if (zif->gvrp_port_config->applicant_state)
        {
          ret = gvrp_set_applicant_state (master, ifp,
                zif->gvrp_port_config->applicant_state);
          gvrp_zlog_process_result (ret, ifp, 0);
        }

#ifdef HAVE_MVRP
      if (zif->gvrp_port_config->p2p == MVRP_POINT_TO_POINT_ENABLE)
        {
          ret = mvrp_set_pointtopoint (master, MVRP_POINT_TO_POINT_ENABLE,
                                       ifp);
          gvrp_zlog_process_result (ret, ifp, 0);
        }
#endif /* HAVE_MVRP */
    }

  XFREE (MTYPE_GVRP_PORT_CONFIG, zif->gvrp_port_config);
  zif->gvrp_port_config = NULL;

#ifdef HAVE_ONMD
  nsm_oam_lldp_if_set_protocol_list (
                      ifp, (gvrp->reg_type == REG_TYPE_GVRP) ?
                      NSM_LLDP_PROTO_GVRP:NSM_LLDP_PROTO_MVRP,
                      PAL_TRUE);
#endif /* HAVE_ONMD */

  return 0;
}

int
gvrp_enable_port_all (struct nsm_bridge_master *master,
                      struct interface *ifp)
{
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct avl_node *avl_port;
  struct nsm_if *zif;
  int ret = 0;

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

      ret = gvrp_enable_port (master, ifp);

      if (ret < 0)
        break;
    }

  return ret;
}

int
gvrp_delete_port_cb ( struct nsm_bridge_master *master,
                      char *bridge_name, struct interface *ifp)
{
  return gvrp_disable_port (master, ifp);
}

int
gvrp_enable_port_cb (struct nsm_bridge_master *master,
                     struct interface *ifp)
{
  struct gvrp *gvrp = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge = NULL;
  struct gvrp_port *gvrp_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist,
     GVRP Port does not exist or GID Port does not exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ||
       (!gvrp_port->gid_port) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(gvrp = bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  /* if the GARP Instance is already enabled on the port, just return */

  if (gvrp_port->gid)
    return 0;

  if (!gid_create_gid (gvrp_port->gid_port, &gvrp->garp_instance, 
                       &gvrp_port->gid, NSM_VLAN_DEFAULT_VID) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
  if ( (gvrp_port_check_static_vlans (master, bridge, ifp)) < 0 )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  if (gvrp_port->registration_mode != GVRP_VLAN_REGISTRATION_NORMAL)
    {
      gvrp_set_registration (master, ifp, gvrp_port->registration_mode);
    }

  if (gvrp_port->applicant_state != GVRP_APPLICANT_NORMAL)
    {
      gvrp_set_applicant_state (master, ifp, gvrp_port->applicant_state);
    }

  return 0;
}

int
gvrp_disable_port_cb (struct nsm_bridge_master *master,
                      struct interface *ifp)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_if *zif = NULL;
  struct gvrp_port *gvrp_port = NULL;

  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist,
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  gvrp_delete_static_vlans_on_port(master, bridge, ifp);
  gid_destroy_gid (gvrp_port->gid);
  gvrp_port->gid = NULL;

  return 0;
}


int
gvrp_disable_port (struct nsm_bridge_master *master,
                  struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_bridge_port *br_port = 0;

  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ) 
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
  gvrp_delete_static_vlans_on_port(master, bridge, ifp);    
  gvrp_destroy_port (gvrp_port);
  br_port->gvrp_port = NULL;

#ifdef HAVE_ONMD
  nsm_oam_lldp_if_set_protocol_list (
                      ifp, (bridge->gvrp->reg_type == REG_TYPE_GVRP) ?
                      NSM_LLDP_PROTO_GVRP:NSM_LLDP_PROTO_MVRP,
                      PAL_FALSE);
#endif /* HAVE_ONMD */

  return 0;
}

int
gvrp_disable_port_all (struct nsm_bridge_master *master,
                      struct interface *ifp)
{
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct avl_node *avl_port;
  struct nsm_if *zif;
  int ret = 0;

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

      ret = gvrp_disable_port (master, ifp);

      if (ret < 0)
        break;
    }

  return ret;
}

extern int
gvrp_get_port_details (struct nsm_bridge_master *master,
                       struct interface *ifp,
                       u_char* flags)
{
  struct nsm_bridge_port *br_port = 0;
  struct nsm_bridge *bridge = 0;
  struct gid_states states;
  struct nsm_if *zif;
  
  /* Bridge does not exist, Global GMRP exist, Bridge Port does not exist, 
     GMRP Port does exist */
  if ( !(zif = (struct nsm_if *)ifp->info) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  *flags = 0x00;

  if ( br_port->gvrp_port ) 
    {
      *flags |= GVRP_MGMT_PORT_CONFIGURED;
      if (br_port->gvrp_port->gid) 
        gid_get_attribute_states (br_port->gvrp_port->gid, 0, &states);
      /* TODO set the state of the gid_index in the flags field */
    }
  
  return 0;
}

int
gvrp_set_applicant_state(struct nsm_bridge_master *master,
                         struct interface *ifp,
                         const u_int32_t state)
{
  struct nsm_bridge *bridge = 0;
  struct nsm_bridge_port *br_port = 0;
  struct nsm_if *zif;
  struct gvrp_port *gvrp_port = 0;
   
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ) 
    {
      return NSM_ERR_GVRP_NOCONFIG_ONPORT;
    }
   if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

  gvrp_port->applicant_state = state;
   
  return 0;
}

int
gvrp_get_applicant_state(struct nsm_bridge_master *master,
                         struct interface *ifp,
                         u_int32_t *state)
{
  struct nsm_bridge_port *br_port = 0;
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gvrp_port *gvrp_port = 0;
    
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ) 
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
  *state = gvrp_port->applicant_state;  
   
  return 0;
}

/* Called by gvrp_set_registration to change registration
   on the port for all vlans */
int
gvrp_port_inject_gid_event_all_vlans(struct nsm_vlan_port *port,
                                     struct nsm_bridge *bridge,
                                     struct gvrp_port *gvrp_port,
                                     gid_event_t event)
{
  u_int16_t vid;
  struct gid *gid;
  struct gvrp *gvrp;
  u_int32_t gid_index;
  struct ptree_node *rn;
  u_int32_t gid_default_vlan_index;

  if ( (!bridge) || (!gvrp_port) )
    return NSM_BRIDGE_ERR_GENERAL;
  
  gvrp = bridge->gvrp;
  gid = gvrp_port->gid;
  if ( (!gvrp) || (!gid))
    return NSM_BRIDGE_ERR_GENERAL;
  
  if( !(gvd_find_entry (gvrp->gvd, NSM_VLAN_DEFAULT_VID, &gid_default_vlan_index)) )
      return RESULT_ERROR;
  
  for (rn = ptree_top (gid->gid_machine_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      gid_index = gid_get_attr_index (rn);
      if (gid_index != gid_default_vlan_index)
        {
          if (event == GID_EVENT_NORMAL_REGISTRATION)
            {
              vid = gvd_get_vid (gvrp->gvd, gid_index);

              /* Static vlans should be in fixed registration irrespective of
               * port registration.
               */
  
              if ( port && NSM_VLAN_BMP_IS_MEMBER (port->staticMemberBmp, vid) )
               continue;
            }

          /* Inject the appropriate event into GARP state machine for all vlans
             on the port except the default vlan (because currently we're using
             it for setting registration mainly. This might change if the usage
             changes later. */ 
          gid_manage_attribute (gid, gid_index, event);

          /* Inform gvrp to propagate the information received to all the other
          ports that are part of the bridge and port state is forwarding */
          gvrp_do_actions (gid);
        }
    }
  
  return 0;
}

int
gvrp_deregister_vlan_on_port(struct nsm_bridge *bridge, 
                             struct interface *ifp, 
                             u_int16_t vid)
{
  struct gid *gid;
  struct nsm_if *zif;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_vlan_port *port = 0;
  struct nsm_bridge_port *br_port = NULL;
  int ret = 0;
   
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(bridge->gvrp)) || 
       (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ||
       (!(port = &(br_port->vlan_port))) ||
       (!(gid = gvrp_port->gid)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
   
  /* Currently called only from gvrp_deregister_all_vlans_port which is 
     called from gvrp_set_registration where all checks are are already 
     done no checks are need for the parameters */
  /* Remove vlan id from port */
  switch (port->mode) 
  {
    case NSM_VLAN_PORT_MODE_ACCESS:
      nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID, PAL_TRUE, PAL_TRUE);
      {
        zlog_debug (gvrpm, "gvrp_deregister:Can't set the default VID\n");
        return NSM_BRIDGE_ERR_GENERAL;
      }
     break;

    case NSM_VLAN_PORT_MODE_TRUNK:
      nsm_vlan_delete_trunk_port (ifp, vid, PAL_TRUE, PAL_TRUE);
     break;
      
    case NSM_VLAN_PORT_MODE_HYBRID:
      nsm_vlan_delete_hybrid_port (ifp, vid, PAL_TRUE, PAL_TRUE);
     break;

#ifdef HAVE_PROVIDER_BRIDGE
    case NSM_VLAN_PORT_MODE_CN :
       ret = nsm_vlan_set_provider_port(ifp, NSM_VLAN_DEFAULT_VID, port->mode,
                                        port->sub_mode, PAL_TRUE, PAL_TRUE);
       if (ret < 0)
         zlog_debug (gvrpm, "gvrp_deregister:Can't set provider port\n");
     break;

    case NSM_VLAN_PORT_MODE_PN : 
      ret = nsm_vlan_delete_provider_port(ifp, vid, port->mode, port->sub_mode, 
                                          PAL_TRUE, PAL_TRUE);
      if (ret < 0)
        zlog_debug (gvrpm, "gvrp_deregister:Can't delete from provider port\n");
     break;
#endif /* HAVE_PROVIDER_BRIDGE */

    default:
     break;
  }
   
  /* Delete VLAN information in database. */
  nsm_vlan_delete(bridge->master, bridge->name, vid, NSM_VLAN_DYNAMIC);
   
  return 0;
}

void
gvrp_deregister_all_vlans_on_port(struct nsm_bridge *bridge, 
                                  struct interface *ifp)
{
  u_int16_t               vid;
   
  /* Since currently bieng called only from gvrp_set_registration where all
     checks are are already done no checks are need for the parameters */
  for ( vid = NSM_VLAN_DEFAULT_VID + 1; vid <= NSM_VLAN_MAX; vid++ ) 
    {
      gvrp_deregister_vlan_on_port(bridge, ifp, vid);
    } /* end for vid loop */  
}

int
gvrp_set_registration (struct nsm_bridge_master *master,
                       struct interface *ifp,
                       const u_int32_t registration_type)
{
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_vlan_port *port = 0;
  struct nsm_bridge_port *br_port = NULL;
  
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ||
       (!(port = &(br_port->vlan_port))) )
    {
      return NSM_ERR_GVRP_NOCONFIG_ONPORT;
    }
   if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
    /*gid_set_attributes_states (gvrp_port->gid, 0, 99, registration_type);*/
    gvrp_port->registration_mode = registration_type;
    switch (registration_type) 
      {
        case GVRP_VLAN_REGISTRATION_NORMAL:
          gvrp_port_inject_gid_event_all_vlans(port, bridge, gvrp_port,
                                               GID_EVENT_NORMAL_REGISTRATION);
          break;
        case GVRP_VLAN_REGISTRATION_FIXED:
          gvrp_port_inject_gid_event_all_vlans(port, bridge, gvrp_port,
                                               GID_EVENT_FIXED_REGISTRATION);
          break;
        case GVRP_VLAN_REGISTRATION_FORBIDDEN:
          gvrp_deregister_all_vlans_on_port(bridge, ifp);
          gvrp_port_inject_gid_event_all_vlans(port, bridge, gvrp_port, 
                                               GID_EVENT_FORBID_REGISTRATION);
          break;
       default:
          break;
      }
  return 0;
}

int
gvrp_get_registration (struct nsm_bridge_master *master,
                       struct interface *ifp,
                       u_int32_t *registration_type)
{
  struct nsm_bridge *bridge = 0;
  struct nsm_if *zif;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_vlan_port *port = 0;
  struct nsm_bridge_port *br_port = NULL;
   
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ||
       (!(port = &(br_port->vlan_port))) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
 
  *registration_type = gvrp_port->registration_mode;
   
  return 0;
}

int
gvrp_set_timer (struct nsm_bridge_master *master,
                struct interface *ifp,
                const u_int32_t timer_type,
                const pal_time_t timer_value)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_bridge_port *br_port = 0;
    
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
   
  gid_set_timer (gvrp_port->gid_port, timer_type, timer_value);
   
  return 0;
}

int
gvrp_get_timer (struct nsm_bridge_master *master,
                struct interface *ifp,
                const u_int32_t timer_type,
                const pal_time_t *timer_value)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_bridge_port *br_port = 0;
    
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  
  gid_get_timer (gvrp_port->gid_port, timer_type, (pal_time_t *)timer_value);
   
  return 0;
}

int
gvrp_get_timer_details (struct nsm_bridge_master *master,
                        struct interface *ifp, 
                        pal_time_t *timer_details)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge = 0;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_bridge_port *br_port = 0;
    
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
  if ( (!(bridge = zif->bridge)) ||
       (!(bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
   
  gid_get_timer_details (gvrp_port->gid_port, timer_details);
   
  return 0;
}

int
gvrp_get_per_vlan_statistics_details (struct nsm_bridge_master *master,
                                      const char* const bridge_name,
                                      const u_int16_t vid,
                                      u_int32_t *receive_counters,
                                      u_int32_t *transmit_counters)
{
  struct nsm_bridge *bridge = 0;
  struct gvrp *gvrp = 0;
   
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
       (!(gvrp = bridge->gvrp)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
   
  pal_mem_cpy (receive_counters, &gvrp->garp_instance.receive_counters, 
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
   
  pal_mem_cpy (transmit_counters, &gvrp->garp_instance.transmit_counters,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
   
  return 0;
}

int
gvrp_clear_per_bridge_statistics (struct nsm_bridge_master *master,
                                  const char *name)
{
  struct nsm_bridge_port *br_port;
  struct nsm_bridge *bridge;
  struct avl_node *avl_port;
  struct interface *ifp;
  struct nsm_if *zif;

  if ( !(bridge = nsm_lookup_bridge_by_name (master, name)) )
    {
      return NSM_BRIDGE_ERR_NOTFOUND;
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

      gvrp_clear_per_port_statistics (master, ifp);

    }
   
  return 0;
}

int
gvrp_clear_per_port_statistics (struct nsm_bridge_master *master,
                                struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_bridge_port *br_port;
  struct gvrp_port *gvrp_port = 0;
    
  if ( !ifp )
    return RESULT_ERROR;
   
  /* Bridge does not exist, Global GVRP exist, Bridge Port does not exist, 
     GVRP Port does exist */
  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }
   
  pal_mem_set (gvrp_port->receive_counters, 0,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
  pal_mem_set (gvrp_port->transmit_counters, 0,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
   
  return 0;
}

int
gvrp_clear_all_statistics (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  struct nsm_bridge *bridge;
   
  for (bridge=nsm_get_first_bridge(master);
       bridge; bridge = nsm_get_next_bridge (master, bridge))
    gvrp_clear_per_bridge_statistics (master, bridge->name);
   
  return 0;
}

int
gvrp_dynamic_vlan_learning_set (struct nsm_bridge_master *master, 
    char *br_name, bool_t vlan_learning_enable)
{
  struct nsm_bridge *bridge;

  if ( (!(bridge = nsm_lookup_bridge_by_name (master, br_name))) ||
       (!(bridge->gvrp)) )
  {
    return NSM_BRIDGE_ERR_NOTFOUND;
  }
  bridge->gvrp->dynamic_vlan_enabled = vlan_learning_enable;
  return 0;
}

void
gvrp_zlog_process_result ( int result,
                         struct interface  *ifp,
                         char *bridge_name)
{
  switch (result)
    {
      case NSM_BRIDGE_ERR_NOT_BOUND:
        ifp ? (zlog_debug (gvrpm, "%% Interface %d not bound to bridge\n",
              ifp->ifindex)):
              (zlog_debug (gvrpm, "%% Interface not bound to bridge\n"));
        break;
      case NSM_BRIDGE_ERR_MEM:
        bridge_name ? (zlog_debug (gvrpm, "%% Bridge %s Out of Memory\n",
                       bridge_name)):
                       (zlog_debug (gvrpm, "%% Bridge Out of memory\n"));
        break;
      case NSM_BRIDGE_ERR_NOTFOUND:
        bridge_name ? zlog_debug (gvrpm, "%% Bridge %s Not Found\n",
                      bridge_name):
                      zlog_debug (gvrpm, "%% Bridge Not Found\n");
        break;
      case NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE:
        bridge_name ? (zlog_debug (gvrpm, "%% Bridge %s is not vlan aware\n",
                       bridge_name)) :
                      (zlog_debug (gvrpm, "%% Bridge is not vlan aware\n"));
        break;
      case NSM_BRIDGE_ERR_GVRP_NOCONFIG:
        bridge_name ? (zlog_debug (gvrpm,
                      "%% No GVRP configured on bridge %s\n", bridge_name)):
                      (zlog_debug (gvrpm, "%% No GVRP configured on bridge\n"));
        break;
      case NSM_ERR_GVRP_NOCONFIG_ONPORT:
        ifp ? (zlog_debug (gvrpm, "%% No GVRP configured on the port %s\n",
               ifp->name)):
             (zlog_debug (gvrpm, "%% No GVRP configured on the port"));
        break;
      case NSM_BRIDGE_ERR_GENERAL:
        zlog_debug (gvrpm, "%% Bridge error\n");
        break;
      default:
        break;
    }
}

int
gvrp_api_clear_all_statistics (struct nsm_bridge_master *master)
{
  struct nsm_bridge *bridge;
   
  for (bridge=nsm_get_first_bridge(master);
       bridge; bridge = nsm_get_next_bridge (master, bridge))
    gvrp_clear_per_bridge_statistics (master, bridge->name);
   
  return RESULT_OK;
}

#if defined HAVE_MVRP
s_int32_t
mvrp_set_pointtopoint(struct nsm_bridge_master *master, u_char ctrl,
                      struct interface *ifp)
{
  struct nsm_bridge_port *br_port;
  struct gvrp_port *gvrp_port = 0;
  struct nsm_if *zif;

  if ( (!(zif = (struct nsm_if *)ifp->info))  ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp_port = br_port->gvrp_port)) ||
       (!gvrp_port->gid_port) )
    {
      return NSM_BRIDGE_ERR_GENERAL;
    }

 gvrp_port->gid_port->p2p = ctrl;

 return 0; 

}
#endif /* HAVE_MVRP */

#ifdef HAVE_SMI

int
gvrp_get_configuration_bridge(struct nsm_bridge_master *master, 
                              char *bridge_name,  char* reg_type,
                              struct gvrp_bridge_configuration *gvrp_br_config)
{
  u_char flags;
  struct nsm_if *zif;
  struct avl_node *avl_port;
  struct nsm_bridge  *bridge;
  struct gvrp *gvrp;
  register struct interface *ifp;
  struct nsm_bridge_port *br_port;

  if (! master)
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* Check for existence of bridge */
  if ( !(bridge = nsm_lookup_bridge_by_name(master, bridge_name)) )
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* If gvrp is not enabled on bridge return */
  if ( !(gvrp = (struct gvrp*)bridge->gvrp))  
     return NSM_BRIDGE_ERR_GVRP_NOCONFIG;

  API_CHECK_XVRP_IS_RUNNING(bridge, reg_type);

  pal_snprintf(gvrp_br_config->bridge_name, SMI_BRIDGE_NAMSIZ, bridge_name);
  gvrp_br_config->dynamic_vlan_enabled = gvrp->dynamic_vlan_enabled;

  SMI_BMP_INIT(gvrp_br_config->port_list);

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

    if (gvrp_get_port_details (master, ifp, &flags) != RESULT_OK)
        continue;

    if (flags & GVRP_MGMT_PORT_CONFIGURED)
    {
       SMI_BMP_SET(gvrp_br_config->port_list, ifp->ifindex);
    }
  }
  return RESULT_OK;
}

int
gvrp_get_port_statistics(struct nsm_bridge_master *master,
                         struct interface *ifp,
                         struct smi_gvrp_statistics *gvrp_stats)
{
  struct nsm_bridge  *bridge = NULL;
  struct nsm_if *zif = NULL;
  struct gvrp *gvrp = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gvrp_port *gvrp_port = NULL;

  if (ifp == NULL)
    return NSM_ERR_IF_BIND_VLAN_ERR;

  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp = bridge->gvrp)) ||
       (!(gvrp_port= br_port->gvrp_port)) )
  {
    return NSM_BRIDGE_ERR_GENERAL;
  }

  pal_snprintf(gvrp_stats->if_name, INTERFACE_NAMSIZ, ifp->name);

  pal_mem_cpy(gvrp_stats->receive_counters, gvrp_port->receive_counters,
              SMI_GARP_ATTR_EVENT_MAX);

  pal_mem_cpy(gvrp_stats->transmit_counters, gvrp_port->transmit_counters,
              SMI_GARP_ATTR_EVENT_MAX);


  return RESULT_OK;
}

/* Get VIDs learnt and GID machine table info */
int
gvrp_get_vid_details (struct nsm_bridge_master *master,
                      struct interface *ifp,
                      u_int32_t first_call,
                      u_int32_t gid_idx,
                      struct smi_gvrp_vid_detail *gvrp_vid_detail)
{
  struct gid *gid;
  struct gvrp *gvrp;
  struct garp *garp;
  struct nsm_if *zif;
  struct ptree_node *rn;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
  struct gid_machine *machine = 0;
  u_int32_t gid_index;
  int i;
 
  if (ifp == NULL)
    return NSM_ERR_IF_BIND_VLAN_ERR;

  if ( !(zif = (struct nsm_if *)ifp->info) )
    return NSM_BRIDGE_ERR_GENERAL;

  if ( (!(bridge = zif->bridge)) ||
       (!(br_port = zif->switchport)) ||
       (!(gvrp = bridge->gvrp)))
  {
    return NSM_BRIDGE_ERR_GENERAL;
  }

  API_CHECK_XVRP_IS_RUNNING(bridge, "gvrp");

  garp = &gvrp->garp;

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

      gid = (struct gid *)garp->get_gid_func (br_port, NSM_VLAN_DEFAULT_VID,
                                              NSM_VLAN_DEFAULT_VID);

      if (gid)
      {
        if(first_call)
        {
          for (i = 0, rn = ptree_top (gid->gid_machine_table); 
               rn && (i <= SMI_NUM_REC); 
               rn = ptree_next (rn), i++)
          {
            if (!rn->info)
              continue;
  
            machine = rn->info;
  
            gid_index = gvrp_vid_detail[i].gid_index = 
                      gid_get_attr_index (rn);
            gvrp_vid_detail[i].vid = gvd_get_vid (gvrp->gvd, gid_index);
            gvrp_vid_detail[i].applicant = machine->applicant;
            gvrp_vid_detail[i].registrar = machine->registrar;
  
            rn->info = NULL;
            ptree_unlock_node (rn);
          }
        }
        else
        {
          rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &gid_idx,
                                  PTREE_MAX_KEY_LEN);
          for (i = 0, rn = ptree_next(rn);
               rn && (i <= SMI_NUM_REC); 
               rn = ptree_next (rn))
          {
            if (!rn->info)
              continue;

            machine = rn->info;

            gid_index = gvrp_vid_detail[i].gid_index = 
                        gid_get_attr_index (rn);
            gvrp_vid_detail[i].vid = gvd_get_vid (gvrp->gvd, gid_index);
            gvrp_vid_detail[i].applicant = machine->applicant;
            gvrp_vid_detail[i].registrar = machine->registrar;

            rn->info = NULL;
            ptree_unlock_node (rn);
            i++; /* increment only if the node is added to the array */
          }
        }
      }
    }
  return RESULT_OK;
}

#endif /* HAVE_SMI */
