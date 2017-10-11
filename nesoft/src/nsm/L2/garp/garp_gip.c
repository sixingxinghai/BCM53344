/* Copyright 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "memory.h"
#include "avl_tree.h"
#include "table.h"

#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#include "garp_gip.h"
#include "garp_debug.h"
#include "nsm_interface.h"
#include "nsm_bridge.h"

bool_t
gip_create_gip (struct garp_instance *garp_instance)
{
  if (!garp_instance)
    return PAL_FALSE;

  garp_instance->gip_table = ptree_init (ATTR_INDX_MAX_BITLEN);

  if (!garp_instance->gip_table)
    return PAL_FALSE;

  return PAL_TRUE;
}

void
gip_destroy_gip (struct garp_instance *garp_instance)
{
  struct ptree_node *rn;
  u_int32_t *gip;

  if (!garp_instance)
    return;

  for (rn = ptree_top (garp_instance->gip_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      gip = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      XFREE (MTYPE_GARP_GIP, gip);
   }
  
  ptree_finish (garp_instance->gip_table);
}

void
gip_propagate_join_event (struct gid *gid, u_int32_t gid_index,
                          enum gid_event event)
{ 
  u_int16_t     vid;
  u_int16_t     svid;
  u_int32_t *curr_gip;
  struct gid *curr_gid;
  struct ptree_node *rn;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct garp *garp  = NULL;
  unsigned int joining_members;
  struct gid_port *curr_gid_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gid_port *gid_port = gid->gid_port;
  struct garp_instance *garp_instance = gid->application;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp)
    return;

  rn = ptree_node_lookup (garp_instance->gip_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn || !rn->info)
    return;

  curr_gip = rn->info;

  joining_members = ((*curr_gip) += 1);

  vid    = garp->get_vid_func (garp_instance->application);
  svid    = garp->get_svid_func (garp_instance->application);
  
  if (joining_members <= 2)
    {
      bridge = garp->get_bridge_instance_func (garp_instance->application);

      for (avl_port = avl_top (bridge->port_tree); avl_port; 
           avl_port = avl_next (avl_port))
        {
          br_port = (struct nsm_bridge_port *) AVL_NODE_INFO (avl_port);

          if ( (!br_port) || (br_port->state != NSM_BRIDGE_PORT_STATE_FORWARDING) )
            continue;

          curr_gid = (struct gid *)garp->get_gid_func (br_port, vid, svid);

          if ((curr_gid) && (curr_gid_port = curr_gid->gid_port) && 
              (curr_gid != gid) &&
              ((joining_members == 1) || 
               (!gid_registrar_empty_here (curr_gid, gid_index))))
            {
              if (GARP_DEBUG(EVENT))
                {
                  zlog_debug (garp->garpm, "gip_propagate_join: [port %d] "
                              "Sending gid_join_request and calling garp "
                              "application join propagated function for " 
                              "gid_index %u joining members %u", 
                              curr_gid_port->ifindex, gid_index, joining_members);
                }

              gid_join_event_request (curr_gid, gid_index, event);

              /* Inform garp application about the propagated join */
              garp->join_propagated_func (
                                          curr_gid->application->application,
                                          curr_gid,
                                          gid_index);
            }
        }
    }
}


void
gip_propagate_leave (struct gid *gid, u_int32_t gid_index)
{
  u_int16_t vid;
  u_int16_t svid;
  u_int32_t *curr_gip;
  struct gid *curr_gid;
  struct ptree_node *rn;
  struct garp *garp = NULL;
  struct nsm_bridge *bridge;
  struct avl_node *avl_port;
  unsigned int remaining_members;
  struct gid_port *curr_gid_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gid_port *gid_port = gid->gid_port;
  struct garp_instance *garp_instance = gid->application;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp)
    return;

  /* Decrement the number of members registered for the gid index
   * if the count is greater than zero.
   */

  rn = ptree_node_lookup (garp_instance->gip_table, (u_char *) &gid_index, 
                          ATTR_INDX_MAX_BITLEN);

  if (!rn || !rn->info)
    return;

  ptree_unlock_node (rn);
  curr_gip = rn->info;

  if ( *curr_gip > 0 )
    {
      remaining_members = ((*curr_gip) -= 1); 
    }
  else
    {
      remaining_members = *curr_gip;
    }

  vid = garp->get_vid_func (garp_instance->application);
  svid = garp->get_svid_func (garp_instance->application);
  
  if (remaining_members <= 1)
    { 
      bridge = garp->get_bridge_instance_func (garp_instance->application);

      for (avl_port = avl_top (bridge->port_tree); avl_port; 
           avl_port = avl_next (avl_port))
        {
          br_port = (struct nsm_bridge_port *) AVL_NODE_INFO (avl_port);

          if ( (!br_port) || (br_port->state != NSM_BRIDGE_PORT_STATE_FORWARDING) )
            continue;

          curr_gid = (struct gid *)garp->get_gid_func (br_port, vid, svid);

          if ((curr_gid) && (curr_gid_port = curr_gid->gid_port) &&
              (curr_gid != gid) &&
              ((remaining_members == 0) || (!gid_registrar_empty_here (curr_gid,
                                                                   gid_index))))
            {

              if (GARP_DEBUG(EVENT))
                {
                  zlog_debug (garp->garpm, "gip_propagate_leave: [port %d]"
                              " Sending gid_leave_request and calling garp "
                              "application leave propagated function for "
                              "gid_index %u", curr_gid_port->ifindex, gid_index);
                }

              gid_leave_request (curr_gid, gid_index); 
              
              /* Inform garp application about the propagated leave */
              garp->leave_propagated_func (
                                           curr_gid->application->application,
                                           curr_gid,
                                           gid_index);
            }
        }
    }
}

void
gip_do_actions (struct gid *gid)
{
  u_int16_t vid;
  u_int16_t svid;
  struct gid *curr_gid;
  struct garp *garp = NULL;
  struct avl_node *avl_port;
  struct nsm_bridge *bridge;
  struct gid_port *curr_gid_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gid_port *gid_port = gid->gid_port;
  struct garp_instance *garp_instance = gid->application;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp)
    return;

  curr_gid = gid;
 
  bridge = garp->get_bridge_instance_func (gid->application->application);
  vid    = garp->get_vid_func (garp_instance->application);
  svid    = garp->get_svid_func (garp_instance->application);

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "[port %d] gip_do_actions: ", gid_port->ifindex);
    }
    
  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      br_port = (struct nsm_bridge_port *) AVL_NODE_INFO (avl_port);

      if (br_port == NULL)
        continue;

      curr_gid = (struct gid *)garp->get_gid_func (br_port, vid, svid);

      if ((curr_gid) && ((curr_gid_port = curr_gid->gid_port)) && 
          (curr_gid != gid))
        {
          if (GARP_DEBUG(EVENT))
            {
              zlog_debug (garp->garpm, "gip_do_actions: [port %d] "
                          "Calling gid_do_actions", curr_gid_port->ifindex);
            }

          gid_do_actions (curr_gid);
        }
    }
}
