/* Copyright 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "memory.h"
#include "avl_tree.h"
#include "table.h"

#include "nsm_interface.h"
#include "nsm_vlan.h"

#include "garp_gid.h"
#include "garp_gip.h"
#include "garp_debug.h"
#include "gvrp.h"
#include "gvrp_cli.h"
#include "gvrp_database.h"
#include "gvrp_debug.h"
#include "gvrp_pdu.h"
#include "gvrp_static.h"

/* Static Functions Forward declaration */
static void
gvrp_report_vlan_added_to_port(struct interface *ifp,
                               u_int16_t vid, u_int32_t gid_index);

static void
gvrp_report_vlan_deleted_from_port(struct interface *ifp,
   u_int16_t vid, u_int32_t gid_index);

static void
gvrp_inject_event_all_trunk_ports(struct nsm_bridge *bridge, 
                                  u_int16_t vid,
                                  u_int32_t gid_index,
                                  gid_event_t event);
/* End Static function forward declaration */

void 
gvrp_check_static_vlans(struct nsm_bridge_master *master,
    struct nsm_bridge *bridge)
{
  int vid;
  struct gvrp *gvrp;
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;
  
  if (bridge == NULL)
    return;

  gvrp = bridge->gvrp;
  if (gvrp == NULL)
    return;

  /* Loop through the port list and check for need to add static vlans */
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

      if ( (vlan_port = &(br_port->vlan_port)) )
        {
          for (vid = NSM_VLAN_DEFAULT_VID; vid  <= NSM_VLAN_MAX; vid++) 
             {
               if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid)) 
                  gvrp_add_static_vlan(master, bridge->name, ifp, vid);
             }
        } 
    }
}

int
gvrp_port_check_static_vlans (struct nsm_bridge_master *master,
                              struct nsm_bridge *bridge, struct interface *ifp)
{
  int vid = 0;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  if (ifp == NULL)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return RESULT_ERROR;

  br_port = zif->switchport;

  if (br_port == NULL)
    return RESULT_ERROR;

  if ( (vlan_port = &(br_port->vlan_port)) )
    for (vid = NSM_VLAN_DEFAULT_VID; vid <= NSM_VLAN_MAX; vid++)
       {
         if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid))
           gvrp_add_static_vlan(master, bridge->name, ifp, vid);
       }

  return RESULT_OK;
}

void 
gvrp_delete_static_vlan_on_all_ports(struct nsm_bridge_master *master,
    char *bridge_name, int vid)
{
  struct gvrp *gvrp;
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;

  bridge = nsm_lookup_bridge_by_name (master, bridge_name);
  if (bridge == NULL)
    return;

  if ( !(gvrp = bridge->gvrp) )
    return;

  /* Loop through the port list and check for need to delete static vlans */

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

      if ( (vlan_port = &(br_port->vlan_port)) )
        {
          for (vid = NSM_VLAN_DEFAULT_VID; vid  <= NSM_VLAN_MAX; vid++) 
            {
              if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid)) 
                gvrp_delete_static_vlan(master, bridge->name, ifp, vid);
            } 
        }
    }
}

void 
gvrp_delete_static_vlans_on_port(struct nsm_bridge_master *master,
    struct nsm_bridge *bridge, struct interface *ifp)
{
  int vid;
  struct gvrp *gvrp;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_if *zif;

  if ( (bridge == NULL) || (ifp == NULL) )
    return;

  gvrp = bridge->gvrp;
  if (gvrp == NULL)
    return;

  if ( (zif = (struct nsm_if *)ifp->info) == NULL )
    return;
  
  if ( (br_port = zif->switchport) == NULL )
    return;

  /* Loop through the port list and check for need to delete static vlans */
  if ( (vlan_port = &(br_port->vlan_port)) )
  {
    for (vid = NSM_VLAN_DEFAULT_VID; vid  <= NSM_VLAN_MAX; vid++) 
    {
      if (NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid)) 
      {
        gvrp_delete_static_vlan(master, bridge->name, ifp, vid);
      }
    } 
  }
}

int
gvrp_change_port_mode(struct nsm_bridge_master *master, char *bridge_name,
                      struct interface *ifp)
{
  struct gid *gid;
  struct gvrp *gvrp;
  struct nsm_if *zif;
  u_int32_t gid_index;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_tree *vlan_table;
  struct nsm_bridge *bridge = NULL;
  struct gvrp_port *gvrp_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

   /* Bridge does not exist, Global GVRP does not exist, Bridge Port
     does not exist, GVRP Port does not exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
       (!(gvrp = bridge->gvrp)) || (!ifp) || (!(ifp->info)))
  {
    return RESULT_ERROR;
  }

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return RESULT_ERROR; 

  br_port = zif->switchport;

  if (br_port == NULL)
    return RESULT_ERROR;

  vlan_port = &br_port->vlan_port;

  gvrp_port = br_port->gvrp_port;

  if ( !gvrp_port || !vlan_port )
    return RESULT_ERROR;

  if ( (vlan_port->mode != NSM_VLAN_PORT_MODE_TRUNK)  &&
       (vlan_port->mode != NSM_VLAN_PORT_MODE_HYBRID) &&
       (vlan_port->mode != NSM_VLAN_PORT_MODE_PN) )
    return RESULT_OK;

  if(gvrp_port->registration_mode == GVRP_VLAN_REGISTRATION_FORBIDDEN) {
     zlog_debug(gvrpm,
                "gvrp_add_static_vlan: Cannot add, Registration forbidden");
     return RESULT_ERROR;
  }

  gid = gvrp_port->gid;

  /* When we got GVRP packet from non configured port. */
  if (gid == NULL)
    {
      zlog_debug(gvrpm,
                "gvrp_add_static_vlan: Cannot add, gid null");
      return RESULT_ERROR;
    }

  /* Intialize to a dummy value */
  gid_index = GVRP_NUMBER_OF_GID_MACHINES;

  for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
    {
      /* Skip if vlan has no member ports. */

      if ( (!(vlan = rn->info)) || (!(vlan->port_list)) || 
           (!(vlan->port_list->head)) )
        continue;

      /* Get the gid_index associated with the vid from the GVD */
      if (! gvd_find_entry (gvrp->gvd, vlan->vid, &gid_index))
        {
          if (!gvd_create_entry (gvrp->gvd, vlan->vid, &gid_index))
            {
              continue;
            }
          else
            return RESULT_ERROR;
        }

      gid_manage_attribute (gid, gid_index, GID_EVENT_JOIN);
      gid_do_actions (gid);
    }

  return RESULT_OK;
}

/* When a new Vlan is added to Bridge a gid entry is created and 
   the registrar state is set to EMPTY */
s_int32_t
gvrp_create_static_vlan (struct nsm_bridge_master *master, char *bridge_name,
                         u_int16_t vid)
{
  struct gid *gid;
  struct gvrp *gvrp;
  u_int32_t gid_index;
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge  *bridge = NULL;
  struct gvrp_port *gvrp_port = NULL;

  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
       (!(gvrp = bridge->gvrp)))
    return -1;

  /* Initialize to a dummy value */
  gid_index = GVRP_NUMBER_OF_GID_MACHINES;

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if (!(gvrp_port = br_port->gvrp_port) || !(gid = gvrp_port->gid))
        continue;

      /* Get the gid_index associated with the vid from the GVD */
      if (! gvd_find_entry (gvrp->gvd, vid, &gid_index))
        {
          if (!gvd_create_entry (gvrp->gvd, vid, &gid_index))
            {
              continue;
            }
        }
    }

  return 0;
}

/* When a port is enabled with GVRP a gid entry is created for all the vlans 
   configured on the bridge and the registrar state is set to EMPTY */
void
gvrp_port_create_static_vlans (struct nsm_bridge *bridge, struct gvrp *gvrp,
                               struct gvrp_port *gvrp_port)
{
  struct gid *gid;
  u_int32_t gid_index;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;
  struct avl_tree *vlan_table;

  if (!(gid = gvrp_port->gid))
    return;

  gid_index = GVRP_NUMBER_OF_GID_MACHINES;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return;

  for (rn = avl_top (vlan_table); rn; rn = avl_next (rn))
    {
      if ( !(vlan = rn->info))
        continue;

      /* Get the gid_index associated with the vid from the  GVD */
      if (! gvd_find_entry (gvrp->gvd, vlan->vid, &gid_index))
        {
          if (!gvd_create_entry (gvrp->gvd, vlan->vid, &gid_index))
            {
              continue;
            }
        } 
    }
}

int 
gvrp_add_static_vlan(struct nsm_bridge_master *master, char *bridge_name,
                     struct interface *ifp, u_int16_t vid)
{ 
  struct gid *gid;
  struct gvrp *gvrp;
  struct nsm_if *zif;
  u_int32_t gid_index;
  struct nsm_bridge *bridge = NULL;
  struct gvrp_port *gvrp_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  
   /* Bridge does not exist, Global GVRP does not exist, Bridge Port
     does not exist, GVRP Port does not exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
       (!(gvrp = bridge->gvrp)) || (!ifp) || (!(ifp->info)) )
  {
    return RESULT_ERROR;
  }
  
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return RESULT_ERROR;

  br_port = zif->switchport;

  gvrp_port = br_port->gvrp_port;
  
  if ( !gvrp_port )
    return RESULT_ERROR;
  
  if(gvrp_port->registration_mode == GVRP_VLAN_REGISTRATION_FORBIDDEN) {
     zlog_debug(gvrpm, 
                "gvrp_add_static_vlan: Cannot add, Registration forbidden");
     return RESULT_ERROR;
  }
   
  gid = gvrp_port->gid;
  
  /* When we got GVRP packet from non configured port. */
  if (gid == NULL)
    { 
      zlog_debug(gvrpm,
                "gvrp_add_static_vlan: Cannot add, gid null");
      return RESULT_ERROR;
    }
  
  /* Intialize to a dummy value */
  gid_index = GVRP_NUMBER_OF_GID_MACHINES;
  
  /* Get the gid_index associated with the vid from the GVD */
  if (! gvd_find_entry (gvrp->gvd, vid, &gid_index))
  {
    if (!gvd_create_entry (gvrp->gvd, vid, &gid_index)) 
    {
      zlog_debug(gvrpm,
                "gvrp_add_static_vlan: Cannot add, gid null");
      return RESULT_ERROR;

    }
  }
  
  gvrp_report_vlan_added_to_port(ifp, vid, gid_index);
  
  return RESULT_OK;
}

int 
gvrp_delete_static_vlan(struct nsm_bridge_master *master, 
    char *bridge_name, struct interface *ifp, u_int16_t vid)
{ 
  struct gid *gid;
  struct gvrp *gvrp;
  struct nsm_if *zif;
  u_int32_t gid_index;
  struct nsm_bridge *bridge = NULL;
  struct gvrp_port *gvrp_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
   
  /* Bridge does not exist, Global GVRP does not exist, Bridge Port
     does not exist, GVRP Port does not exist */
  if ( (!(bridge = nsm_lookup_bridge_by_name (master, bridge_name))) ||
       (!(gvrp = bridge->gvrp)) || (!ifp) || (!(ifp->info)) )
  {
    return RESULT_ERROR;
  }
  
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return RESULT_ERROR;

  br_port = zif->switchport;
  gvrp_port = br_port->gvrp_port;
  
  if ( !gvrp_port )
    return RESULT_ERROR;
  
  if(gvrp_port->registration_mode == GVRP_VLAN_REGISTRATION_FIXED) {
     zlog_debug(gvrpm,
                "gvrp_delete_static_vlan: Cannot Delete, Registration fixed");
     return RESULT_ERROR;
  }
  
  gid = gvrp_port->gid;
  if (gid == NULL)
      return RESULT_ERROR;

  /* Intialize to a dummy value */
  gid_index = GVRP_NUMBER_OF_GID_MACHINES;

  /* Get the gid_index associated with the mac address from the GMD */
  if (gvd_find_entry (gvrp->gvd, vid, &gid_index)) 
  {
    gvrp_report_vlan_deleted_from_port(ifp, vid, gid_index);
     
    return RESULT_OK;
  }
  else
    return RESULT_ERROR;
}

void
gvrp_report_vlan_added_to_port(struct interface *ifp,
                               u_int16_t vid, u_int32_t gid_index)
{
  struct gvrp *gvrp;
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_vlan_port *vlan_port;
  struct gid       *gid       = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gvrp_port *gvrp_port = NULL;
  
  if ( (!ifp) || (!(ifp->info)) )
    return;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL
      || (bridge = zif->bridge) == NULL)
    return;

  br_port = zif->switchport;

  if (br_port == NULL)
    return;

  vlan_port = &(br_port->vlan_port);

  if ( (!(gvrp_port = br_port->gvrp_port)) ||
       (!(gvrp = bridge->gvrp)) ||
       (!(gid = gvrp_port->gid)) )
    return;

  /* Put the port on which the vlan is configured in Fixed IN State */

  gid_register_gid (gid, gid_index);

  gid_manage_attribute (gid, gid_index, GID_EVENT_FIXED_REGISTRATION);

  /* This is probably the first access port to get associated
     with Vlan. Inject ReqJoin, i.e. GID_EVENT_JOIN to all
     active trunk ports */

  gvrp_inject_event_all_trunk_ports (zif->bridge, vid,
                                     gid_index, GID_EVENT_JOIN);
}

/* This is static function so we don't need to check every thing */
void
gvrp_report_vlan_deleted_from_port(struct interface *ifp,
   u_int16_t vid, u_int32_t gid_index)
{
  struct nsm_if *zif;
  struct nsm_vlan nvlan;
  struct avl_tree *vlan_table;
  struct nsm_vlan   *vlan = NULL;
  struct avl_node *rn   = NULL;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct gvrp_port *gvrp_port = NULL;
  struct gid       *gid       = NULL;

  if ( (!ifp) || (!(ifp->info)) )
    return;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return;

  br_port = zif->switchport;

  if (br_port == NULL)
    return;

  vlan_port = &(br_port->vlan_port);

  if ( (!(gvrp_port = br_port->gvrp_port)) ||
       (!(gid = gvrp_port->gid)) || 
       (!(bridge = zif->bridge)) )
    return;


  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    vlan_table = bridge->svlan_table;
  else
    vlan_table = bridge->vlan_table;

  if (vlan_table == NULL)
    return;

  NSM_VLAN_SET(&nvlan, vid);

  rn = avl_search (vlan_table, (void *) &nvlan);

  if (!rn)
    return;

  vlan = rn->info;

  /* Put the port on which the vlan is configured in Empty State */

  gid_unregister_gid (gid, gid_index);

  switch (gvrp_port->registration_mode)
    {
      case GVRP_VLAN_REGISTRATION_NORMAL:
        gid_manage_attribute (gid, gid_index, GID_EVENT_NORMAL_REGISTRATION);
        break;
      case GVRP_VLAN_REGISTRATION_FIXED:
        gid_manage_attribute (gid, gid_index, GID_EVENT_FIXED_REGISTRATION);
        break;
      case GVRP_VLAN_REGISTRATION_FORBIDDEN:
        gid_manage_attribute (gid, gid_index, GID_EVENT_FORBID_REGISTRATION);
        break;

      default:
        break;
    }

  /* If there are other ports in the vlan, do not propagate leave */
  if ( LISTCOUNT (vlan->port_list) > 1 )
    return;

  gvrp_inject_event_all_trunk_ports(zif->bridge, vid,
                                    gid_index, GID_EVENT_LEAVE);
}

void
gvrp_inject_event_all_trunk_ports(struct nsm_bridge *bridge, 
                                  u_int16_t vid,
                                  u_int32_t gid_index,
                                  gid_event_t event)
{
  struct gid *gid;
  struct gvrp *gvrp;
  struct avl_node *avl_port;
  struct gvrp_port *gvrp_port;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan_port *vlan_port;

  if ( !(bridge) || (!(gvrp = bridge->gvrp)) )
  {
    return;
  }
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ( (gvrp_port = br_port->gvrp_port) &&
            (vlan_port = &(br_port->vlan_port)) )
        {
          if ( ((vlan_port->mode == NSM_VLAN_PORT_MODE_TRUNK) ||
                (vlan_port->mode == NSM_VLAN_PORT_MODE_HYBRID) ||
                (vlan_port->mode == NSM_VLAN_PORT_MODE_PN)) && 
                (gid = gvrp_port->gid) )
            {
              gid_manage_attribute (gid, gid_index, event);
              gid_do_actions (gid);
            } /* end if gid and trunk or hybrid port */
        } /* end if gvrpport and vlan_port */
    } /* end for all bridge ports */

} /* END Function gvrp_inject_event_all_trunk_port */
