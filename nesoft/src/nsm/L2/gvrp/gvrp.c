/* Copyright 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "memory.h"
#include "table.h"
#include "avl_tree.h"

#include "nsmd.h"
#include "nsm_interface.h"
#include "nsm_vlan.h"

#include "garp_gid.h"
#include "garp_gip.h"
#include "garp_debug.h"
#include "garp_sock.h"
#include "gvrp_api.h"
#include "gvrp.h"
#include "gvrp_cli.h"
#include "gvrp_static.h"
#include "gvrp_database.h"
#include "gvrp_debug.h"
#include "gvrp_pdu.h"

struct lib_globals *gvrpm;
/* Forward declarations of static functions */
static bool_t
gvrp_reg_vlan_listener(struct gvrp *new_gvrp);

static void
gvrp_dereg_vlan_listener(struct gvrp *new_gvrp);

static bool_t
gvrp_reg_bridge_listener(struct gvrp *new_gvrp);

static void
gvrp_dereg_bridge_listener(struct gvrp *new_gvrp);

static void
gvrp_destroy_ports (struct gvrp *gvrp);
/* End Forward declaration */

extern int nsm_vlan_add_port (struct nsm_bridge *bridge, 
                              struct interface *ifp, nsm_vid_t vid,
                              enum nsm_vlan_egress_type egresstagged,
                              bool_t notify_kernel);
extern int nsm_vlan_delete_port (struct nsm_bridge *bridge,
                                 struct interface *ifp, nsm_vid_t vid,
                                 bool_t notify_kernel);

void
gvrp_init (struct lib_globals *zg)
{
  gvrpm = zg;

  /* CLI initialization. */
  gvrp_cli_init (zg);

  /* Debugging initialization. */
  gvrp_debug_init ();
}

bool_t
gvrp_create_port (struct gvrp *gvrp, struct interface *ifp, u_int16_t vid,
                  struct gvrp_port **gvrp_port)
{
  struct gvrp_port *new_port;

  new_port =  XCALLOC (MTYPE_GVRP_PORT, sizeof (struct gvrp_port));

  if (!new_port)
    {
      return PAL_FALSE;
    }

  pal_mem_set (new_port, 0, sizeof (struct gvrp_port));
  new_port->vid = vid;
  new_port->port = ifp;
  new_port->registration_mode = GVRP_VLAN_REGISTRATION_NORMAL;
  new_port->applicant_state = GVRP_APPLICANT_NORMAL;

  if (GVRP_DEBUG(EVENT))
    {
      zlog_debug (gvrpm, "gvrp_create_port: [port %d] Creating gvrp port", 
                  ifp->ifindex);
    }

  if (!gid_create_gid_port (ifp->ifindex, &new_port->gid_port))
    {
      XFREE (MTYPE_GVRP_PORT, new_port);
      return PAL_FALSE;
    }

  if (!gid_create_gid (new_port->gid_port, &gvrp->garp_instance, &new_port->gid, 1))
    {
      XFREE (MTYPE_GARP_GID_PORT, new_port->gid_port);
      XFREE (MTYPE_GVRP_PORT, new_port);
      return PAL_FALSE;
    }

  *gvrp_port = new_port; 

  return PAL_TRUE;
}


static inline bool_t
gvrp_create_ports (struct nsm_bridge *bridge,
                   struct gvrp *gvrp)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct gvrp_port *new_gvrp_port;
  struct nsm_bridge_port *br_port;

  /* Loop through the port list and create a gid per port */
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

      if (!gvrp_create_port (gvrp, ifp, gvrp->vid, &new_gvrp_port))
        {
           /* memory allocation failures */
          return PAL_FALSE;
        }

      /* Set the gvrp pointer in the port struct */
      br_port->gvrp_port = new_gvrp_port;
    }

  return PAL_TRUE;
}

void
gvrp_destroy_port (struct gvrp_port *old_gvrp_port)
{
  gid_destroy_gid (old_gvrp_port->gid);
  XFREE (MTYPE_GARP_GID_PORT, old_gvrp_port->gid_port);
  XFREE (MTYPE_GVRP_PORT, old_gvrp_port);
}

bool_t
gvrp_create_gvrp (struct nsm_bridge *bridge, u_int16_t vid, struct gvrp **gvrp)
{
  struct gvrp *new_gvrp = NULL;

  if (!bridge)
    return PAL_FALSE;

  /* Assign the global variable (lib_globals for gvrp) to be sanme as that of
     the bridge (rstp, stp, mstp) */
  new_gvrp = XCALLOC (MTYPE_GVRP, sizeof (struct gvrp));

  if (!new_gvrp)
    {
      if (GVRP_DEBUG(EVENT))
        {
          zlog_debug (gvrpm, "gvrp_create_gvrp:Unable to create "
                      "new gvrp for vlan %d", vid);
        }

      return PAL_FALSE;
    }

  new_gvrp->bridge = bridge;
  new_gvrp->garp_instance.application = (struct garp *)new_gvrp; 

  if (!gip_create_gip (&new_gvrp->garp_instance))
    {
      if (GVRP_DEBUG(EVENT))
        {
          zlog_debug (gvrpm, "gvrp_create_gvrp: Unable to create new gvrp "
                      "for vlan %d", vid);
        }

      XFREE (MTYPE_GVRP, new_gvrp);

      return PAL_FALSE;
    }

  if (!gvd_create_gvd (&new_gvrp->gvd))
    {
      if (GVRP_DEBUG(EVENT))
        {
          zlog_debug (gvrpm, "gvrp_create_gvrp: Unable to create new gmd "
                      "for vlan %d", vid);
        }

      gip_destroy_gip (&new_gvrp->garp_instance);
      XFREE (MTYPE_GVRP, new_gvrp);

      return PAL_FALSE;
    }

  pal_mem_set (new_gvrp->garp_instance.receive_counters, 0,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));

  pal_mem_set (&new_gvrp->garp_instance.transmit_counters, 0,
               (GARP_ATTR_EVENT_MAX + 1) * sizeof (u_int32_t));
  new_gvrp->garp.garpm = gvrpm;
  new_gvrp->garp.join_indication_func = gvrp_join_indication;
  new_gvrp->garp.leave_indication_func = gvrp_leave_indication;
  new_gvrp->garp.join_propagated_func = gvrp_join_propagated;
  new_gvrp->garp.leave_propagated_func = gvrp_leave_propagated;
  new_gvrp->garp.get_bridge_instance_func  = gvrp_get_bridge_instance;
  new_gvrp->garp.get_vid_func = gvrp_get_vid;
  new_gvrp->garp.get_svid_func = gvrp_get_svid;
  new_gvrp->garp.get_gid_func = (struct gid* (*)(const void *, uint16_t, uint16_t))
                                                gvrp_get_gid;
  new_gvrp->garp.transmit_func = gvrp_transmit_pdu;
  new_gvrp->vid = vid;
  new_gvrp->dynamic_vlan_enabled = PAL_FALSE;
  new_gvrp->garp_instance.garp = &new_gvrp->garp;

  /* Create and Initialise the gvrp listener for nsm vlan info
     with proper values */
  if ( !gvrp_reg_vlan_listener(new_gvrp) ) {
    if (GVRP_DEBUG(EVENT)) {
      zlog_debug (gvrpm, "gvrp_create_gvrp: Unable to create new gvrp vlan "
                  "listener for vlan %d", vid);
    }
    XFREE (MTYPE_GVRP, new_gvrp->gvd);
    XFREE (MTYPE_GVRP, new_gvrp);

    return PAL_FALSE;
  }

  /* Create and Initialise the gvrp listener for nsm bridge info
     with proper values */
  if ( !gvrp_reg_bridge_listener(new_gvrp) ) {
    if (GVRP_DEBUG(EVENT)) {
      zlog_debug (gvrpm, "gvrp_create_gvrp: Unable to create new gvrp "
                  "bridge listener");
    }
    gvrp_dereg_vlan_listener(new_gvrp);
    XFREE (MTYPE_GVRP, new_gvrp->gvd);
    XFREE (MTYPE_GVRP, new_gvrp);

    return PAL_FALSE;
  }

  *gvrp = new_gvrp;

  return PAL_TRUE;
}

static void
gvrp_destroy_ports (struct gvrp *gvrp)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct gvrp_port *old_gvrp_port;
  struct nsm_bridge_port *br_port;

  /* Loop through the port list and destroy gid per port */
  for (avl_port = avl_top (gvrp->bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      old_gvrp_port = br_port->gvrp_port;

      if (! old_gvrp_port)
        continue;

      gvrp_destroy_port (old_gvrp_port);

      /* Reset the gvrp pointer in the port struct */
      br_port->gvrp_port = NULL;
    }
}

int 
gvrp_destroy_gvrp (struct nsm_bridge_master *master, char *bridge_name)
{
  struct nsm_bridge *br;
  
  br = nsm_lookup_bridge_by_name(master, bridge_name);
  if ( !br )
    return 0;
  
  if (br->gvrp == NULL)
    return 0;

  gvrp_dereg_bridge_listener(br->gvrp);
  gvrp_destroy_ports (br->gvrp);
  gvrp_dereg_vlan_listener(br->gvrp);
  gvd_destroy_gvd (br->gvrp->gvd);
  br->gvrp->gvd = NULL;
  gip_destroy_gip (&br->gvrp->garp_instance);

  br->gvrp->bridge = NULL;
  XFREE(MTYPE_GVRP, br->gvrp);
  br->gvrp = NULL;
  
  return 0;
}

void
gvrp_join_indication (void *application, struct gid *gid,
                      u_int32_t joining_gid_index)
{
  u_int8_t vlan_type;
  struct nsm_vlan nvlan;
  struct apn_vr    *vr;
  struct interface *ifp;
  u_int16_t         vid;
  struct nsm_if    *zif;
  struct avl_node *rn = NULL;
  struct nsm_vlan  *vlan = NULL;
  struct nsm_bridge *bridge = 0;
  struct avl_tree *vlan_table;
  struct nsm_vlan_port *port = 0;
  struct nsm_bridge_port *br_port = NULL;
  struct gvrp_port *gvrp_port = 0;
  struct gid_port *gid_port = NULL;
  char vlan_name[NSM_VLAN_NAME_MAX + 1];
  struct gvrp_attr_entry_tbd *attr_entry_tbd;
  struct gvrp      *gvrp = (struct gvrp *)application;
  s_int32_t ret = 0;

  /* Check  if Bridge exists, Bridge Port exists, and GMRP Port exists */
  if ( (!(bridge = nsm_lookup_bridge_by_name(gvrp->bridge->master, gvrp->bridge->name))) )
    return;

  if ( (!gid) || (!(gid_port = gid->gid_port)) )
    return;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    {
      vlan_type = NSM_VLAN_DYNAMIC | NSM_VLAN_SVLAN;
      vlan_table = bridge->svlan_table;
    }
  else
    {
      vlan_type = NSM_VLAN_DYNAMIC | NSM_VLAN_CVLAN;
      vlan_table = bridge->vlan_table;
    }

  if (vlan_table == NULL)
    return;

  vr = apn_vr_get_privileged (gvrpm);
  if (vr == NULL)
    return;

  ifp = if_lookup_by_index (&vr->ifm, gid_port->ifindex);

  if (ifp == NULL)
    return;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return;

  br_port = zif->switchport;

  if (br_port == NULL)
    return;

  if (br_port->state != NSM_BRIDGE_PORT_STATE_FORWARDING)
    {
      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_join_indication: [port %d] GVRP is in"
                    "discarding state\\n", ifp->ifindex);
      return;
    }

  gvrp_port = br_port->gvrp_port;
  port = &(br_port->vlan_port);

  if ( (ifp != nsm_lookup_port_by_index(bridge, gid_port->ifindex)) ||
       (!gvrp_port) || (!port) )
    {
      zlog_debug (gvrpm, "gvrp_join_indication:Can't change the default VID "
                  "of the port as it is not attached to any bridge-group\n");
      return;
    }
  /* if dynamic vlan creation is enabled and registration mode is normal 
     then remove the dynamic vlan id */
   if (gvrp->dynamic_vlan_enabled && 
      (gvrp_port->registration_mode != GVRP_VLAN_REGISTRATION_FORBIDDEN)) 
    {
      /* Extract vid from gid index */
      vid = gvd_get_vid (gvrp->gvd, joining_gid_index);

      /* Ignore invalid VLANs */
      if ((vid < NSM_VLAN_DEFAULT_VID) || (vid > NSM_VLAN_MAX))
        return;

      if (GVRP_DEBUG(EVENT))
        {
          zlog_debug (gvrpm, "gvrp_join_indication: [port %d] Inserting vid "
                      "entry %u into FDB", gid_port->ifindex, vid);
        }

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);


      /* First check for the existance of the VLAN. */
      if ( rn && (vlan = (struct nsm_vlan *)rn->info)) 
        {
          /* Change VLAN information in database. */
          if (vlan->vlan_state != NSM_VLAN_ACTIVE) 
            {
              ret = nsm_vlan_add (bridge->master, bridge->name,
                                  vlan->vlan_name, vid, 
                                  NSM_VLAN_ACTIVE, vlan_type);
              if (ret < 0)
                {
                  gvrp_port->gvrp_failed_registrations++;
                  return;
                }
            }
          else
            {
              vlan->type |= NSM_VLAN_DYNAMIC;
            } 
        }
      else 
        {
          pal_snprintf(vlan_name, NSM_VLAN_NAME_MAX, "VLAN%04d", vid);

          /* Add the vlan to the bridge's vlan database */
          ret = nsm_vlan_add (bridge->master, bridge->name, vlan_name, vid,
                              NSM_VLAN_ACTIVE, vlan_type);
          if (ret < 0)
            {
              gvrp_port->gvrp_failed_registrations++;
              return;
            }
        } 

      if (vlan == NULL)
        {
          rn = avl_search (vlan_table, (void *)&nvlan);
          if (rn == NULL)
            return;

          vlan = (struct nsm_vlan *)rn->info;
        }

      /* install vid in fdb according to port type */
      switch (port->mode) {
      case NSM_VLAN_PORT_MODE_CN:
      case NSM_VLAN_PORT_MODE_ACCESS:
        {
          if ( (port->pvid != NSM_VLAN_DEFAULT_VID) &&
               (port->pvid != vid) )
            {
              struct nsm_vlan *port_vlan = NULL;

              NSM_VLAN_SET (&nvlan, port->pvid);

              rn = avl_search (vlan_table, (void *)&nvlan);
         
              if (rn && (port_vlan = (struct nsm_vlan *)rn->info))
               {
                  /* If the vlan currently associated with the port is a
                     static vlan we ignore this join_indication, else we
                     change the current vlan associated with the port */
                  if ( port_vlan->type & NSM_VLAN_STATIC)
                    {
                      zlog_debug (gvrpm, "gvrp_join_indication:Port [%d]"
                                  " already associated with vlan [%d]\n",
                                  ifp->ifindex, port->pvid);
                      return;
                    }/* end id vlan type static */
                }/*end if port vlan not null */
              /* Delete dynamic port->vlan mapping. */
              NSM_VLAN_BMP_UNSET(port->dynamicMemberBmp, port->pvid);

              /* Delete dynamic new vlan->port mapping. */
              nsm_vlan_delete_port (zif->bridge, ifp, port->pvid, PAL_TRUE);
            }/* end if port vid ! default vlan id */

          else if ( port->pvid == NSM_VLAN_DEFAULT_VID )
            {
              NSM_VLAN_BMP_UNSET (port->dynamicMemberBmp, port->pvid);
              /* Add new port->vlan mapping. */
              NSM_VLAN_BMP_SET(port->dynamicMemberBmp, vid);
              nsm_vlan_set_access_port (ifp, vid, PAL_FALSE, PAL_TRUE);
            }

          /* Add new port->vlan mapping. */
          NSM_VLAN_BMP_SET(port->dynamicMemberBmp, vid);

          /* Add new vlan->port mapping. */
          nsm_vlan_add_port (zif->bridge, ifp, vid, NSM_VLAN_EGRESS_UNTAGGED,
                             PAL_TRUE);
          break;
        }
      case NSM_VLAN_PORT_MODE_PN:
      case NSM_VLAN_PORT_MODE_TRUNK:
      case NSM_VLAN_PORT_MODE_HYBRID:
        {        
          /* Doesn't matter what the vlan type is, on this port it has been
             learnt dynamically so add it as dynamic to the port */
          if ( vid == NSM_VLAN_DEFAULT_VID )
            {
              /* Vlan is already added to port */
              if (GVRP_DEBUG(EVENT))
                zlog_debug (gvrpm, "No need to add VLAN %u\n", vid);
              return;
            }

          NSM_VLAN_BMP_SET(port->egresstaggedBmp, vid);

          /* Add new port->vlan mapping. */
          NSM_VLAN_BMP_SET(port->dynamicMemberBmp, vid);

          /* Add new vlan->port mapping. */
          nsm_vlan_add_port (zif->bridge, ifp, vid, NSM_VLAN_EGRESS_TAGGED,
                             PAL_TRUE);
          break;
        }
      default:
        break;
      }  
    }
  attr_entry_tbd = gvd_entry_tbd_lookup (gvrp->gvd, joining_gid_index);

  if (attr_entry_tbd)
    gvrp_gvd_delete_tbd_entry (gvrp->gvd, joining_gid_index);

  return;

}

void
gvrp_join_propagated (void *gvrp, struct gid *gid,
                      u_int32_t joining_gid_index)
{
  /* Nothing to be propagated */
}

void
gvrp_leave_indication (void *application, struct gid *gid, 
                       u_int32_t leave_gid_index)
{
  u_int8_t     vlan_type;
  u_int16_t         vid;
  struct apn_vr    *vr;
  struct nsm_vlan nvlan;
  struct nsm_if    *zif;
  struct avl_node *rn;
  struct interface *ifp;
  struct listnode *ln = NULL;
  struct nsm_if *zif2 = NULL;
  struct nsm_vlan  *vlan = 0;
  struct nsm_bridge *bridge = 0;
  struct avl_tree *vlan_table;
  struct interface *ifp2 = NULL;
  struct gvrp_port *gvrp_port = 0;
  struct gid_port *gid_port = NULL;
  struct nsm_vlan_port *port = NULL;
  bool_t reset_vlan_type = PAL_TRUE;
  struct nsm_vlan_port *vlan_port = 0;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge_port *br_port2 = NULL;
  struct gvrp_attr_entry_tbd *attr_entry_tbd;
  struct gvrp      *gvrp = (struct gvrp *)application;

  /* Check  if Bridge exists, Bridge Port exists, and GMRP Port exists */
  bridge = gvrp->bridge;
  if (! bridge)
    return;

  if ( (!gid) || (!(gid_port = gid->gid_port)) )
    return;

  if (NSM_BRIDGE_TYPE_PROVIDER (bridge))
    {
      vlan_type = NSM_VLAN_DYNAMIC | NSM_VLAN_SVLAN;
      vlan_table = bridge->svlan_table;
    }
  else
    {
      vlan_type = NSM_VLAN_DYNAMIC | NSM_VLAN_CVLAN;
      vlan_table = bridge->vlan_table;
    }

  if (vlan_table == NULL)
    return;

  vr = apn_vr_get_privileged (gvrpm);
  if (vr == NULL)
    return;

  ifp = if_lookup_by_index (&vr->ifm, gid_port->ifindex);
  if (ifp == NULL)
    return;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return;

  br_port = zif->switchport;

  if (br_port == NULL)
    return;

  gvrp_port = br_port->gvrp_port;
  vlan_port = &(br_port->vlan_port);

  if ( (ifp != nsm_lookup_port_by_index(bridge, gid_port->ifindex)) ||
       (!gvrp_port) || (!vlan_port) )
    {
      zlog_debug (gvrpm, "gvrp_leave_indication: Can't change the default VID"
                  " of the port as it is not attached to any bridge-group\n");
      return;
    }

  /* if dynamic vlan creation is enabled and registration mode is not
     fixed then remove the dynamic vlan id */
  if ( gvrp->dynamic_vlan_enabled && 
       (gvrp_port->registration_mode != GVRP_VLAN_REGISTRATION_FIXED))
    {
      /* Extract vid from gid index */
      vid = gvd_get_vid (gvrp->gvd, leave_gid_index);

      /* Ignore invalid VLANs */
      if ((vid < NSM_VLAN_DEFAULT_VID) || (vid > NSM_VLAN_MAX))
        return;

      if (GVRP_DEBUG(EVENT))
        {
          zlog_debug (gvrpm, "gvrp_leave_indication: [port %d] Deleting vid entry %u from FDB", 
                      gid_port->ifindex, vid);
        }

      NSM_VLAN_SET (&nvlan, vid);

      rn = avl_search (vlan_table, (void *)&nvlan);

      /* First check for the existance of the VLAN. */
      if ( rn && (vlan = (struct nsm_vlan *)rn->info)) 
        {
          /* Remove vlan id from port */
          switch (vlan_port->mode)
            {
            case NSM_VLAN_PORT_MODE_CN:         
            case NSM_VLAN_PORT_MODE_ACCESS:
              {
                if ( (vlan_port->pvid != NSM_VLAN_DEFAULT_VID) &&
                     (vlan_port->pvid != vid) )
                  {
                    struct nsm_vlan *port_vlan = NULL;

                    NSM_VLAN_SET (&nvlan, vlan_port->pvid);

                    rn = avl_search (vlan_table, (void *)&nvlan);

                    if ( rn && (port_vlan = (struct nsm_vlan *) rn->info))
                      {
                        /* If the vlan currently associated with the port is a
                           static vlan we ignore this leave_indication, else we
                           change the current vlan associated with the port */
                        if ( port_vlan->type & NSM_VLAN_STATIC)
                          {
                            zlog_debug (gvrpm, "gvrp_leave_indication:Port [%d]"
                                        " already associated with vlan [%d]\n",
                                        ifp->ifindex, vlan_port->pvid);
                            return;
                          }/* end id vlan type static */
                      }/*end if port vlan not null */
                    /* Delete dynamic port->vlan mapping. */
                    NSM_VLAN_BMP_UNSET(vlan_port->dynamicMemberBmp, vlan_port->pvid);

                    /* Delete dynamic new vlan->port mapping. */
                    nsm_vlan_delete_port (zif->bridge, ifp, vlan_port->pvid, PAL_TRUE);
                  }/* end if port vid ! default vlan id */

                if (vlan->type & NSM_VLAN_DYNAMIC) 
                  {
                    /* Inform NSM that old vlan got deleted from port and 
                       default vlan got added */
                    /* Delete dynamic port->vlan mapping. */
                    NSM_VLAN_BMP_UNSET(vlan_port->dynamicMemberBmp, vid);

                    /* Delete dynamic new vlan->port mapping. */
                    nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);

                    /* Add new port->vlan mapping. */
                    NSM_VLAN_BMP_SET(vlan_port->dynamicMemberBmp, NSM_VLAN_DEFAULT_VID);
                    /* Add new vlan->port mapping. */
                    nsm_vlan_add_port (bridge, ifp, NSM_VLAN_DEFAULT_VID, 
                                       NSM_VLAN_EGRESS_UNTAGGED, PAL_TRUE);
                    nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID,
                                              PAL_FALSE, PAL_TRUE);
                  }
                break;
              }
            case NSM_VLAN_PORT_MODE_PN:       
            case NSM_VLAN_PORT_MODE_TRUNK:
            case NSM_VLAN_PORT_MODE_HYBRID:
              {
                /* Delete vlan id from port in vlan database */
                if ((vlan->type & NSM_VLAN_DYNAMIC) ||
                    (NSM_VLAN_BMP_IS_MEMBER(vlan_port->dynamicMemberBmp, 
                                            vid)) )
                  {
                    /* Delete dynamic port->vlan mapping. */
                    NSM_VLAN_BMP_UNSET(vlan_port->dynamicMemberBmp, vid);
                    if(!(NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp, vid)))
                      NSM_VLAN_BMP_UNSET(vlan_port->egresstaggedBmp, vid);

                    /* Delete dynamic new vlan->port mapping. */
                    nsm_vlan_delete_port (bridge, ifp, vid, PAL_TRUE);
                  }
                break;
              }

            default:
              break;
            } 

          if ( vlan->port_list->head )
            {
              LIST_LOOP(vlan->port_list, ifp2, ln)
                {
                  if ( (zif2 = (struct nsm_if *)(ifp2->info)) 
                       && (br_port2 = zif->switchport) )
                    {
                      if ( (port = &br_port2->vlan_port) )
                        {
                          if ( NSM_VLAN_BMP_IS_MEMBER(port->dynamicMemberBmp, vid) )
                            {
                              reset_vlan_type = PAL_FALSE;
                            } 
                        }
                    }
                }

              if (reset_vlan_type)  
                {
                  vlan->type &= ~(NSM_VLAN_DYNAMIC);
                } 
            }
 
          if (vlan->vlan_state == NSM_VLAN_ACTIVE) 
            {
              if ( (vlan->type & NSM_VLAN_DYNAMIC) &&
                   !(vlan->type & NSM_VLAN_STATIC) ) 
                {
                  if ( !(vlan->port_list->head) )
                    nsm_vlan_delete (bridge->master, bridge->name, vid, 
                                     vlan_type);
                }
            }
        }

      if (gid_attr_entry_to_be_created (gid, leave_gid_index))
        {
          attr_entry_tbd = gvd_entry_tbd_create (gvrp->gvd, leave_gid_index);

          if (!attr_entry_tbd)
            return;

          /* Delete the attribute from this gid as leave indication sent */
          gid_delete_attribute (gid, leave_gid_index);
        }
    }
}

void
gvrp_leave_propagated (void *application, struct gid *gid, 
                       u_int32_t leave_gid_index)
{
  struct gvrp_attr_entry_tbd *attr_entry_tbd;
  struct gvrp *gvrp;

  gvrp = (struct gvrp*)application;

  if (!gvrp || !gid)
    return;

   /* Dont delete it now after sending the leave delete it */
  attr_entry_tbd = gvd_entry_tbd_lookup (gvrp->gvd, leave_gid_index);

  if (!attr_entry_tbd)
    return;

  attr_entry_tbd->pending_gid++;
}

void 
gvrp_do_actions (struct gid *gid)
{
  gip_do_actions (gid);
}

struct gid*
gvrp_get_gid (struct nsm_bridge_port *br_port, u_int16_t vid)
{
  if ( br_port == NULL)
    return NULL;
  
  return (br_port->gvrp_port ? br_port->gvrp_port->gid : NULL);
}

void*
gvrp_get_bridge_instance (void *application)
{
  struct gvrp *gvrp = (struct gvrp *)application;
  return gvrp->bridge;
}

u_int16_t
gvrp_get_vid (void *application)
{
  return NSM_VLAN_DEFAULT_VID;
}

u_int16_t
gvrp_get_svid (void *application)
{
  return NSM_VLAN_DEFAULT_VID;
}

static bool_t
gvrp_reg_vlan_listener(struct gvrp *new_gvrp)
{
  if ( (nsm_create_vlan_listener(&new_gvrp->gvrp_appln) != RESULT_OK) ) 
    {
      return PAL_FALSE;
    }
  (new_gvrp->gvrp_appln)->listener_id = NSM_LISTENER_ID_GVRP_PROTO;
  (new_gvrp->gvrp_appln)->flags = NSM_VLAN_STATIC;
  (new_gvrp->gvrp_appln)->add_vlan_to_port_func = gvrp_add_static_vlan;
  (new_gvrp->gvrp_appln)-> add_vlan_to_bridge_func = gvrp_create_static_vlan;
  (new_gvrp->gvrp_appln)->delete_vlan_from_port_func= gvrp_delete_static_vlan;
  (new_gvrp->gvrp_appln)->change_port_mode_func = gvrp_change_port_mode;

  if( (new_gvrp->bridge)->vlan_listener_list ) 
    {
      if( nsm_add_listener_to_list((new_gvrp->bridge)->vlan_listener_list,
                                   new_gvrp->gvrp_appln) == RESULT_OK ) 
        {
          return PAL_TRUE;
        }
    }

  nsm_destroy_vlan_listener(new_gvrp->gvrp_appln);
  return PAL_FALSE;
}

static void
gvrp_dereg_vlan_listener(struct gvrp *new_gvrp)
{
  if( (new_gvrp->bridge)->vlan_listener_list ) 
    {
      nsm_remove_listener_from_list((new_gvrp->bridge)->vlan_listener_list,
                                    (new_gvrp->gvrp_appln)->listener_id);
    }

  nsm_destroy_vlan_listener(new_gvrp->gvrp_appln);
}

static bool_t
gvrp_reg_bridge_listener(struct gvrp *new_gvrp)
{
  if ( (nsm_create_bridge_listener(&new_gvrp->gvrp_br_appln) != RESULT_OK) ) 
    {
      return PAL_FALSE;
    }
  (new_gvrp->gvrp_br_appln)->listener_id = NSM_LISTENER_ID_GVRP_PROTO;
  (new_gvrp->gvrp_br_appln)->delete_bridge_func= gvrp_destroy_gvrp;
  (new_gvrp->gvrp_br_appln)->disable_port_func= gvrp_disable_port_cb;
  (new_gvrp->gvrp_br_appln)->enable_port_func= gvrp_enable_port_cb;
  (new_gvrp->gvrp_br_appln)->change_port_state_func = gvrp_set_port_state;
  (new_gvrp->gvrp_br_appln)->delete_port_from_bridge_func = gvrp_delete_port_cb;
  (new_gvrp->gvrp_br_appln)->activate_port_config_func = gvrp_port_activate;

  if( (new_gvrp->bridge)->bridge_listener_list ) 
    {
      if( nsm_add_listener_to_bridgelist((new_gvrp->bridge)->bridge_listener_list,
                                         new_gvrp->gvrp_br_appln) == RESULT_OK ) 
        {
          return PAL_TRUE;
        }
    }

  nsm_destroy_bridge_listener(new_gvrp->gvrp_br_appln);
  new_gvrp->gvrp_br_appln = NULL;
  return PAL_FALSE;
}

static void
gvrp_dereg_bridge_listener(struct gvrp *new_gvrp)
{
  if( (new_gvrp->bridge)->bridge_listener_list ) 
    {
      nsm_remove_listener_from_bridgelist((new_gvrp->bridge)->bridge_listener_list,
                                          (new_gvrp->gvrp_br_appln)->listener_id);
    }

  nsm_destroy_bridge_listener(new_gvrp->gvrp_br_appln);
  new_gvrp->gvrp_br_appln = NULL;
}
