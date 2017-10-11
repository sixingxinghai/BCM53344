/* Copyright 2003  All Rights Reserved. */

#include "thread.h"
#include "memory.h"
#include "pal.h"
#include "lib.h"
#include "table.h"
#include "ptree.h"
#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#include "garp_debug.h"
#include "garp_gip.h"
#include "hal_types.h"
#include "garp_gid_fsm.h"
#include "garp_gid_timer.h"
#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_bridge.h"
#include "nsm_interface.h"

/* Function definitions */

const char* const gid_event_string [] = 
{
  "GID_EVENT_NULL",
  "GID_EVENT_RCV_LEAVE_EMPTY",
  "GID_EVENT_RCV_LEAVE_IN",
  "GID_EVENT_RCV_EMPTY",
  "GID_EVENT_RCV_JOIN_EMPTY",
  "GID_EVENT_RCV_JOIN_IN",
  "GID_EVENT_JOIN",
  "GID_EVENT_LEAVE",
  "GID_EVENT_NEW",
  "GID_EVENT_NORMAL_OPERATION",
  "GID_EVENT_NO_PROTOCOL",
  "GID_EVENT_NORMAL_REGISTRATION",
  "GID_EVENT_FIXED_REGISTRATION",
  "GID_EVENT_FORBID_REGISTRATION",
  "GID_EVENT_RCV_LEAVE_ALL",
  "GID_EVENT_RCV_LEAVE_ALL_RANGE",
  "GID_EVENT_TX_LEAVE_EMPTY",
  "GID_EVENT_TX_LEAVE_IN",
  "GID_EVENT_TX_EMPTY",
  "GID_EVENT_TX_JOIN_EMPTY",
  "GID_EVENT_TX_JOIN_IN",
  "GID_EVENT_TX_LEAVE_ALL",
  "GID_EVENT_TX_LEAVE_ALL_RANGE",
  "GID_EVENT_RESTRICTED_VLAN_REGISTRATION",
  "GID_EVENT_RESTRICTED_GROUP_REGISTRATION",
  "GID_EVENT_MAX"
};

static inline bool_t
gid_new_gid (struct gid_port *gid_port, struct garp_instance *garp_instance, 
             struct gid **gid, u_int32_t init_machines)
{
  struct gid *new_gid = 0;
  struct garp *garp = garp_instance->garp;
  u_int32_t attr_index;
  struct ptree_node *rn;
  struct gid_machine *machine;

  if (!garp)
    return PAL_FALSE;

  new_gid = XCALLOC (MTYPE_GARP_GID, sizeof (struct gid));

  if (!new_gid)
    { 
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_new_gid: [port %d] Unable to create "
                      "new gid", gid_port->ifindex);
        }

      return PAL_FALSE;
    }
 
  /* Initialize the gid instance */
  /* set the back pointer to the garp structure for the gid_instance */
  new_gid->application = garp_instance;
  new_gid->gid_port = gid_port;

  /* timer flags are intially false as nothing is started */
  new_gid->cschedule_tx_now = PAL_FALSE;
  new_gid->cstart_join_timer = PAL_FALSE;
  new_gid->cstart_leave_timer = PAL_FALSE;
  new_gid->tx_now_scheduled = PAL_FALSE;
  new_gid->join_timer_running = PAL_FALSE;
  new_gid->leave_timer_running = PAL_FALSE;
  new_gid->hold_tx = PAL_FALSE;

  /* Comment Me */
  new_gid->leave_all_countdown = GID_LEAVE_ALL_COUNT;
  
  new_gid->gid_machine_table = ptree_init (IPV4_MAX_BITLEN);

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_new_gid: [port %d] Starting leave_all timer"
                  " timer_value %d", gid_port->ifindex, gid_port->leave_all_timeout_n);
    }
  
  /* Initialze the machines */
  for (attr_index = 0; attr_index < init_machines; attr_index++)
    {
      machine = XCALLOC (MTYPE_GARP_GID_MACHINE, (sizeof (struct gid_machine)));

      if (!machine)
        return PAL_FALSE;

      gid_fsm_init_machine (machine);

      rn = ptree_node_get (new_gid->gid_machine_table, (u_char *) &attr_index,
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        {
          /* Releasing Memory Allocated to machine. */
          XFREE (MTYPE_GARP_GID_MACHINE, machine);
          
          /* Releasing Memory Allocated to new_gid. */
          ptree_finish (new_gid->gid_machine_table);
          XFREE (MTYPE_GARP_GID, new_gid);
          return PAL_FALSE;
        }

      PREFIX_ATTRIBUTE_INFO_SET (rn, machine);

      ptree_unlock_node (rn);
    }

  /* set join and leave timer threads to NULL */
  new_gid->leave_timer_thread = NULL;
  new_gid->join_timer_thread = NULL;
  
  /* start the leave_all timer */
  new_gid->leave_all_timer_thread = garp_start_timer (
                                        gid_leave_all_timer_handler, 
                                        new_gid,
                                        gid_port->leave_all_timeout_n);
  
  /* tx_pending is false as intially there is hardly anything pending to tx */
  new_gid->tx_pending = PAL_FALSE;

  /* last_transmitted is set intially after the legacy control values (i.e
     intially to last_gid_index_used). No gid_machine has tx yet */
  new_gid->last_transmitted = 0; 

  /* last_transmitted is set intially after the legacy control values (i.e
     intially to last_gid_index_used). No gid_machine has tx yet */
  new_gid->next_to_transmit = 0; 

  /* Assign the created/intialized gid_instance */
  *gid = new_gid;

  return PAL_TRUE;
}

static inline void 
gid_free_gid (struct gid *old_gid)
{
  struct apn_vr *vr;
  struct nsm_if *zif;
  struct interface *ifp;
  struct ptree_node *rn;
  unsigned int gid_index;
  struct gid_machine *machine;
  struct garp *old_garp = NULL;
  struct nsm_bridge_port *br_port;
  struct gid_port *old_gid_port = old_gid->gid_port;
  struct garp_instance *old_garp_instance = old_gid->application;

  if (!old_garp_instance || !old_gid_port)
    return;

  old_garp = old_garp_instance->garp;

  if (!old_garp)
    return;

  vr = apn_vr_get_privileged (old_garp->garpm);

  if (!vr)
    return;

  ifp = if_lookup_by_index(&vr->ifm, old_gid_port->ifindex);

  if (!ifp || !(zif = ifp->info) || !(br_port = zif->switchport))
    return;

  /* Walk through the gid_index array and upcall the garp application's (GMRP,
     GVRP) leave_indication functions before freeing the gid_instance */
  for (rn = ptree_top (old_gid->gid_machine_table); rn;
       rn = ptree_next (rn))
    {
      gid_index = gid_get_attr_index (rn);
      if (gid_registered_here (old_gid, gid_index))
        {
                if (GARP_DEBUG(EVENT))
                  {
                    zlog_debug (old_garp->garpm, "gid_free_gid: [port %d] "
                          "Sending leave indication gid_index %u", 
                          old_gid_port->ifindex, gid_index);
                  }

          old_garp->leave_indication_func (old_gid->application->application,
                                           old_gid, 
                                            gid_index);

          if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
            {
              gip_propagate_leave (old_gid, gid_index);
              gip_do_actions (old_gid);
            }
        }

      if (!rn->info)
        continue;

      machine = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);
      XFREE (MTYPE_GARP_GID_MACHINE, machine);
    } 

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (old_garp->garpm, "gid_free_gid: [port %d] Stopping "
                  "all timers (join. leave, leaveall)", old_gid_port->ifindex);
    }

  /* cancel the timers that are running for this gid instance */
  garp_stop_timer (&old_gid->join_timer_thread);
  garp_stop_timer (&old_gid->hold_timer_thread);
  garp_stop_timer (&old_gid->leave_timer_thread);
  garp_stop_timer (&old_gid->leave_all_timer_thread);
  
  ptree_finish (old_gid->gid_machine_table);

  XFREE (MTYPE_GARP_GID, old_gid);
}


bool_t
gid_create_gid (struct gid_port *gid_port, struct garp_instance *garp_instance,
                struct gid **gid, u_int32_t init_machines)
{
  struct gid *new_gid;

  if (!gid_new_gid (gid_port, garp_instance, &new_gid, init_machines))
    {
      return PAL_FALSE;
    }

  /* Set the gid created to the port */ 
  *gid = new_gid;

  return PAL_TRUE;
}


void
gid_destroy_gid (struct gid *old_gid)
{

  if (!old_gid)
    return;
  
  gid_set_propagate_port_state_event (old_gid,
                                      NSM_BRIDGE_PORT_STATE_DISCARDING);

  gid_free_gid (old_gid);
}

bool_t
gid_create_gid_port (int ifindex, struct gid_port **gid_port)
{

  struct gid_port *new_gid_port = NULL;

  new_gid_port = XCALLOC (MTYPE_GARP_GID_PORT, sizeof (struct gid_port));

  if (!new_gid_port)
    {
      return PAL_FALSE;
    }

  new_gid_port->ifindex = ifindex;

    /* set the default join timeout value(200 millisecond) for the applicant's
     join timer  */
  new_gid_port->join_timeout = GID_DEFAULT_JOIN_TIME;

  /* set the default leave timeout value(600/4 milliseconds) for the
     registrar's leave timer  */
  new_gid_port->leave_timeout_4 = GID_DEFAULT_LEAVE_TIME/4;

  /* set the default configured leave timeout value(600 milliseconds)
     for the registrar's configured leave timeout */
  new_gid_port->leave_conf_timeout = GID_DEFAULT_LEAVE_TIME;

  /* set the default hold timeout value(100 milliseconds) for the
     applicant's hold timer  */
  new_gid_port->hold_timeout = GID_DEFAULT_HOLD_TIME;

  /* set the default leaveall timeout value(10000/4) for the registrar's
     leave all timer */
  new_gid_port->leave_all_timeout_n = GID_DEFAULT_LEAVE_ALL_TIME /
                                         GID_LEAVE_ALL_COUNT;

  /* set the default configured leave all timeout value(10000 milliseconds)
     for the registrar's configured leave all timeout */
  new_gid_port->leave_all_conf_timeout = GID_DEFAULT_LEAVE_ALL_TIME;

  *gid_port = new_gid_port;

  return PAL_TRUE;

}

bool_t
gid_get_attribute_states (struct gid *gid, unsigned int attribute_index,
                          struct gid_states *state)
{
  struct ptree_node *rn;
  struct gid_machine *machine;

  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    return PAL_FALSE;

  ptree_unlock_node (rn);

  machine = rn->info;
 
  if (!machine)
    return PAL_FALSE;

  gid_fsm_get_states (machine, state);

  return PAL_TRUE;
}
void
gid_propagate_join_leave_event (struct gid *gid,
                                unsigned char port_state)
 {
   struct ptree_node *node;
   struct garp *garp  = NULL;
   u_int32_t attribute_index;
   u_int32_t *joining_members;
   struct garp_instance *garp_instance = gid->application;

   garp = garp_instance->garp;

   if (!garp)
     return;

   PTREE_LOOP (garp_instance->gip_table, joining_members, node)
     {
       attribute_index = * ((u_int32_t *) PTREE_NODE_KEY (node));

       if ( (*joining_members) >= 1)
         {

           if ( port_state == NSM_BRIDGE_PORT_STATE_FORWARDING )
             {
                gid_join_event_request (gid, attribute_index, GID_EVENT_JOIN);
             }
           else
             {
                gid_leave_request (gid, attribute_index);
             }

           gid_do_actions (gid);
         }
     }
  return;

 }


 

/* This function is called when the state of the port changes. This functions propagates
 * leave or Join to the GIP context depending on the port state change. 
 * Section 12.3.3 
 */

void
gid_set_propagate_port_state_event (struct gid *gid, 
                                    unsigned char port_state)
{
  struct   gid_states state;
  u_int32_t attribute_index;
  struct ptree_node *rn;
  bool_t ret;

  if (!gid)
    return;
   
  for (rn = ptree_top (gid->gid_machine_table); rn;
       rn = ptree_next (rn))
    {
      attribute_index = gid_get_attr_index (rn);
      ret = gid_get_attribute_states (gid, attribute_index, &state);

      if (!ret)
        continue;

     /* Check whether a Join indication has occured more recently
      * than a leave indication.
      */
     if ( (state.registrar_state == GID_REG_STATE_IN) ||
          (state.registrar_state == GID_REG_STATE_LEAVE) )
       {
          if ( port_state == NSM_BRIDGE_PORT_STATE_FORWARDING )
            {
               gip_propagate_join_event (gid, attribute_index,
                                         GID_EVENT_JOIN);
               gip_do_actions (gid);
            }
          else if ( (port_state == NSM_BRIDGE_PORT_STATE_BLOCKING) ||
                    (port_state == NSM_BRIDGE_PORT_STATE_DISCARDING) ||
                    (port_state == NSM_BRIDGE_PORT_STATE_DISABLED) )
            {
               gip_propagate_leave (gid, attribute_index);
               gip_do_actions (gid);
            }

       }
   }
   gid_propagate_join_leave_event (gid, port_state);
}  

void
gid_set_attribute_states (struct gid *gid, unsigned int attribute_index,  
                          enum gid_event event)
{
  struct ptree_node *rn;
  enum gid_event new_event;
  struct garp *garp = NULL;
  struct apn_vr *vr = NULL;
  struct gid_machine *machine;
  struct interface *ifp = NULL;
  struct nsm_if    *zif = NULL;
  struct gid_port *gid_port = NULL;
  struct nsm_bridge_port *br_port;
  struct garp_instance *garp_instance = NULL;

  if (!gid)
    return;

  garp_instance = gid->application;
  gid_port = gid->gid_port; 

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp || !garp->garpm)
    return;

  vr = apn_vr_get_privileged (garp->garpm);

  if (!vr)
    return;

  ifp = if_lookup_by_index(&vr->ifm, gid_port->ifindex);

  if (!ifp || !(zif = ifp->info) || !(br_port = zif->switchport))
    return;

  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) & attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn || !rn->info)
    return;

  ptree_unlock_node (rn);

  machine = rn->info;
  new_event = gid_fsm_change_states (gid, machine, event);

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_set_attribute_states: [port %d] "
                  "Changing fsm state for gid_index %u rcv_event %s "
                  "new_event %s", gid_port->ifindex, attribute_index, 
                  gid_event_string[event], gid_event_string[new_event]);
    }

  if ((new_event == GID_EVENT_JOIN) || (new_event == GID_EVENT_NEW))
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_set_attribute_states: [port %d] "
                      "Sending join indication to the garp application for "
                      "gid_index %u and propagating the join through gip",  
                      gid_port->ifindex, attribute_index);
        }

      garp->join_indication_func (garp_instance->application,
                                  gid,
                                  attribute_index);

     /* Propagate the join through GIP if the port is forwarding.*/
     /* Section 12.3.4 */
      if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
          gip_propagate_join_event (gid, attribute_index, new_event);
        }
    }
  else if (new_event == GID_EVENT_LEAVE)
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_set_attribute_states: [port %d] "
                      "Sending leave indication to the garp application for "
                      "gid_index %u and propagating the leave through gip", 
                      gid_port->ifindex, attribute_index);
        }

      garp->leave_indication_func (garp_instance->application,
                                   gid,
                                   attribute_index);

     /* Propagate the leave through GIP if the port is forwarding.*/
     /* Section 12.3.4 */
      if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
          gip_propagate_leave (gid, attribute_index);
        }
    }
}

void
gid_leave_all (struct gid *gid,u_int32_t starting_gid_index,
               u_int32_t ending_gid_index)
{
  u_int32_t attribute_index;
  struct garp_instance *garp_instance = gid->application;
  struct gid_port *gid_port = gid->gid_port;
  struct garp     *garp = NULL;
  struct ptree_node *rn;
  struct gid_machine *machine;
  struct apn_vr *vr;
  struct interface *ifp;
  struct nsm_if *zif;
  enum gid_event new_event;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp)
    return;
  
  vr = apn_vr_get_privileged (garp->garpm);

  if (!vr)
    return;

  ifp = if_lookup_by_index(&vr->ifm, gid_port->ifindex);

  if (!ifp || !(zif = ifp->info))

    return;

  if (ending_gid_index != 0)
    {
      attribute_index = starting_gid_index;

      while (attribute_index < ending_gid_index)
        {
          rn = ptree_node_lookup (gid->gid_machine_table, 
                                  (u_char *) &attribute_index,
                                  ATTR_INDX_MAX_BITLEN);

          if (!rn)
            continue;

          ptree_unlock_node (rn);

          if (!rn->info)
            continue;

          machine = rn->info;

          if (garp->proto == REG_PROTO_GARP)
            gid_fsm_change_states (gid, machine, GID_EVENT_RCV_LEAVE_EMPTY);
          else
            gid_fsm_change_states (gid, machine, GID_EVENT_RCV_LEAVE_ALL);

          attribute_index++;
        }

      gid->tx_pending = PAL_TRUE;
      gid_do_actions (gid);
      return;
    }

  /* Walk through all the gid_index of the port that received the leave_all
   * message and change the applicant and registar states */
  for (rn = ptree_node_lookup (gid->gid_machine_table, 
                               (u_char *) &starting_gid_index, 
                                ATTR_INDX_MAX_BITLEN);
       rn; rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      machine = rn->info;

      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_leave_all: [port %d] "
                      "Changing fsm state for gid_index for rcv_event "
                      "GID_EVENT_RCV_LEAVE_EMPTY", gid_port->ifindex);
        }

       if (garp->proto == REG_PROTO_GARP)
         new_event = gid_fsm_change_states (gid, machine, GID_EVENT_RCV_LEAVE_EMPTY);
       else
         new_event = gid_fsm_change_states (gid, machine, GID_EVENT_RCV_LEAVE_ALL);
       
    }

  /* This is needed to send a empty message whenever we recv a leave_all
     message. The message will be sent after the join_timer expires */
  gid->tx_pending = PAL_TRUE;
  gid_do_actions (gid);
}

/* Attr_type should be zero for gvrp and service requirement for GMRP */
void
gid_rcv_leave_all (struct gid *gid, u_int32_t starting_gid_index,
                   u_int32_t ending_gid_index)
{
  struct garp_instance *garp_instance = gid->application;
  struct gid_port *gid_port = gid->gid_port;
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp)
    return;

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_rcv_leave_all: [port %d] ",
                  gid_port->ifindex);
    }

  /* reset the leave_all count */
  gid->leave_all_countdown = GID_LEAVE_ALL_COUNT;

  /* Change the applicant's and registrar's state based on the leave_empty */
  gid_leave_all (gid, starting_gid_index, ending_gid_index);
}

void
gid_manage_attribute (struct gid *gid, unsigned int gid_index, 
                      enum gid_event directive)
{
  struct ptree_node *rn;
  enum gid_event new_event;
  struct apn_vr *vr = NULL;
  struct garp *garp = NULL;
  struct gid_machine *machine;
  struct interface *ifp = NULL;
  struct nsm_if    *zif = NULL;
  struct nsm_bridge_port *br_port;
  struct gid_port *gid_port = gid->gid_port;
  struct garp_instance *garp_instance = gid->application;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp || !garp->garpm)
    return;

  vr = apn_vr_get_privileged (garp->garpm);

  if (!vr)
    return;

  ifp = if_lookup_by_index(&vr->ifm, gid_port->ifindex);

  if (!ifp || !(zif = ifp->info) || !(br_port = zif->switchport))
    return;

  if (!gid->gid_machine_table)
    return;


  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      machine = XCALLOC (MTYPE_GARP_GID_MACHINE, (sizeof (struct gid_machine)));

      if (!machine)
        return;

      gid_fsm_init_machine (machine);

      rn = ptree_node_get (gid->gid_machine_table, (u_char *) &gid_index,
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        return;

      PREFIX_ATTRIBUTE_INFO_SET (rn, machine);

      ptree_unlock_node (rn);
    }
  else
    {
      ptree_unlock_node (rn);

      machine = rn->info;
    }

  new_event = gid_fsm_change_states (gid, machine, directive);

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_manage_attribute: [port %d] "
                  "Received a managment directive for gid_index %u," 
                  " changing fsm states rcv_event %s, new_event %s", 
                  gid_port->ifindex, gid_index, 
                  gid_event_string[directive], gid_event_string[new_event]);
    }

  if ((new_event == GID_EVENT_JOIN) || (new_event == GID_EVENT_NEW))
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_manage_attribute: [port %d] "
                      "Sending join indication to the garp application for "
                      "gid_index %u and propagating the join through gip", 
                      gid_port->ifindex, gid_index);
        }
       
      /* inform the garp application about receiving a join message for the
         gid_index */
      garp->join_indication_func (garp_instance->application,
                                  gid, gid_index);
      

      /* Propagate the join through GIP if the port is forwarding.*/
      /* Section 12.3.4 */
      if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
           gip_propagate_join_event (gid, gid_index, new_event);
        }

    }
  else if (new_event == GID_EVENT_LEAVE)
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_rcv_attribute: [port %d] "
                      "Sending leave indication to the garp application for "
                      "gid_index %u and propagating the leave through gip", 
                      gid_port->ifindex, gid_index);
        }

      /* inform the garp application about receiving a leave message for the
         gid_index */
      garp->leave_indication_func (garp_instance->application,
                                   gid, gid_index);
      
     /* Propagate the leave through GIP if the port is forwarding.*/
     /* Section 12.3.4 */
      if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
          gip_propagate_leave (gid, gid_index);
        }

    }
}

void
gid_rcv_attribute (struct gid *gid, unsigned int gid_index, 
                   enum gid_event rcv_event)
{
  u_int32_t *gip;
  struct ptree_node *rn;
  enum gid_event new_event;
  struct apn_vr *vr = NULL;
  struct interface *ifp = NULL;
  struct nsm_if    *zif = NULL;
  struct gid_machine *machine = 0;
  struct gid_port *gid_port = NULL;
  struct garp *garp = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct garp_instance *garp_instance = NULL;

  if (!gid)
    return;

  garp_instance =  gid->application;
  gid_port = gid->gid_port;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp || !garp->garpm)
    return;

  vr = apn_vr_get_privileged (garp->garpm);

  if (!vr)
    return;

  ifp = if_lookup_by_index(&vr->ifm, gid_port->ifindex);

  if (!ifp || !(zif = ifp->info) || !(br_port = zif->switchport))
    return;

  if (!gid->gid_machine_table)
    return;

  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (rn)
    {
      ptree_unlock_node (rn);
      if (rn->info)
         machine = rn->info;
      else
        return;
    }
  else
   {
     machine = XCALLOC (MTYPE_GARP_GID_MACHINE, (sizeof (struct gid_machine)));

     if (!machine)
       return;

     gid_fsm_init_machine (machine);

     rn = ptree_node_get (gid->gid_machine_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

     if (!rn)
       return;

     PREFIX_ATTRIBUTE_INFO_SET (rn, machine);

     ptree_unlock_node (rn);
   }
  
  /* Initialze gip */
  if (!garp_instance->gip_table)
    {
      garp_instance->gip_table = ptree_init (ATTR_INDX_MAX_BITLEN);

      if (!garp_instance->gip_table)
        return;
    }

  rn = ptree_node_lookup (garp_instance->gip_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      gip = XCALLOC (MTYPE_GARP_GIP, sizeof (u_int32_t));

      if (!gip)
        return;

      rn = ptree_node_get (garp_instance->gip_table, (u_char *) &gid_index,
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        return;

      *gip = 0; 
      PREFIX_ATTRIBUTE_INFO_SET (rn, gip);

      ptree_unlock_node (rn);
    }
  else
    {
      ptree_unlock_node (rn);
    }

      
  new_event = gid_fsm_change_states (gid, machine, rcv_event);

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_rcv_attribute: [port %d] "
                  "Received a msg for gid_index %u, changing fsm states "
                  "rcv_event %s, new_event %s", gid_port->ifindex, gid_index, 
                  gid_event_string[rcv_event], gid_event_string[new_event]);
    }

  if ((new_event == GID_EVENT_JOIN) || (new_event == GID_EVENT_NEW))
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_rcv_attribute: [port %d] "
                      "Sending join indication to the garp application for "
                      "gid_index %u and propagating the join through gip", 
                      gid_port->ifindex,  gid_index);
        }
      
      /* inform the garp application about receiving a join message for the
         gid_index */
      garp->join_indication_func (garp_instance->application,
                                  gid, gid_index);
      
     /* Propagate the join through GIP if the port is forwarding.*/
     /* Section 12.3.4 */

      if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
          gip_propagate_join_event (gid, gid_index, new_event);
        }

      if (gid->cstart_join_timer)
        {
          gid->tx_pending = PAL_TRUE;
          gid_do_actions (gid);
        }
    }
  else if (new_event == GID_EVENT_LEAVE)
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_rcv_attribute: [port %d] "
                      "Sending leave indication to the garp application for "
                      "gid_index %u and propagating the leave through gip", 
                      gid_port->ifindex, gid_index);
        }

      /* inform the garp application about receiving a leave message for the
         gid_index */
      garp->leave_indication_func (garp_instance->application,
                                   gid, gid_index);
      
     /* Propagate the leave through GIP if the port is forwarding.*/
     /* Section 12.3.4 */

      if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
        {
          gip_propagate_leave (gid, gid_index);
        }

      if (gid->cstart_join_timer || gid->cstart_leave_timer)
        {
          gid->tx_pending = PAL_TRUE;
          gid_do_actions (gid);
        }
    }
  else if (new_event == GID_EVENT_NULL)
    {
      if ((rcv_event == GID_EVENT_RCV_LEAVE_IN) ||
          (rcv_event == GID_EVENT_RCV_LEAVE_EMPTY)  ||
          (gid->cstart_join_timer))
        {
          /* The reason this is done is make sure we are sending and empty 
           * to message on this gid that received the leanin or leave_empty */
          gid->tx_pending = PAL_TRUE;
          gid_do_actions (gid);
        }
    }
    
}

bool_t
gid_delete_attribute (struct gid *gid, u_int32_t attribute_index)
{
  struct ptree_node *rn;
  struct gid_machine *machine;

  if (!gid)
    return PAL_FALSE;


  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    return PAL_FALSE;
  else
    {
      ptree_unlock_node (rn);
      if (!rn->info)
        return PAL_FALSE;

      machine = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      XFREE (MTYPE_GARP_GID_MACHINE, machine);
    }

  return PAL_TRUE;
} 
      
void
gip_delete_attribute (struct gid *gid, u_int32_t attribute_index)
{
  struct ptree_node *rn;
  struct garp_instance *garp_instance;
  u_int32_t *gip;

  if (!gid)
    return;

  garp_instance =  gid->application;

  if (!garp_instance)
    return;


  rn = ptree_node_lookup (garp_instance->gip_table, (u_char *) &attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    return;
  else
    {
      ptree_unlock_node (rn);
      if (!rn->info)
        return;

      gip = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      XFREE (MTYPE_GARP_GIP, gip);
    }

  return;
}

void
gid_join_event_request (struct gid *gid, unsigned int attribute_index,
                        enum gid_event join_event)
{
  struct ptree_node *rn;
  struct gid_machine *machine;

  if (!gid)
    return;

  if (!gid->gid_machine_table)
    return;


  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      machine = XCALLOC (MTYPE_GARP_GID_MACHINE, (sizeof (struct gid_machine)));

      if (!machine)
        return;

      gid_fsm_init_machine (machine);

      rn = ptree_node_get (gid->gid_machine_table, (u_char *) &attribute_index,
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        {
          XFREE (MTYPE_GARP_GID_MACHINE, machine);
          return;
        }

      PREFIX_ATTRIBUTE_INFO_SET (rn, machine);
      ptree_unlock_node (rn);
    }
   else
    {
      ptree_unlock_node (rn);
      machine = rn->info;
    }

  /* Change applicant's and registar's states for event = GID_EVENT_JOIN */
  gid_fsm_change_states (gid, machine, join_event);
}

void
gid_register_gid (struct gid *gid, u_int32_t gid_index)
{
  struct ptree_node *rn;
  struct gid_machine *machine;
  struct garp_instance *garp_instance;
  u_int32_t *gip;

  if (!gid || !gid->gid_machine_table || !(garp_instance = gid->application))
    return;

  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      machine = XCALLOC (MTYPE_GARP_GID_MACHINE, (sizeof (struct gid_machine)));

      if (!machine)
        return;

      gid_fsm_init_machine (machine);

      rn = ptree_node_get (gid->gid_machine_table, (u_char *) &gid_index,
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        {
          XFREE (MTYPE_GARP_GID_MACHINE, machine);
          return;
        }

      PREFIX_ATTRIBUTE_INFO_SET (rn, machine);
      ptree_unlock_node (rn);
    }
  else
    {
      machine = rn->info;
      machine->registrar = REG_STATE_INN;
      ptree_unlock_node (rn);
    }

  /* Put the registran in INN state */
  machine->registrar = REG_STATE_INN;

  /* Increment the registrations for gip */
  rn = ptree_node_lookup (garp_instance->gip_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      gip = XCALLOC (MTYPE_GARP_GIP, sizeof (u_int32_t));

      if (!gip)
        return;

      rn = ptree_node_get (garp_instance->gip_table, (u_char *) &gid_index, 
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        {
          XFREE (MTYPE_GARP_GIP, gip);
          return;
        }

      *gip = 0;
      PREFIX_ATTRIBUTE_INFO_SET (rn, gip);

      ptree_unlock_node (rn);
    }
  else
    {
      ptree_unlock_node (rn);
      if (!rn->info)
        return;

      gip = rn->info;
      (*gip)++;
    }

}

void
gid_unregister_gid (struct gid *gid, u_int32_t gid_index)
{
  struct ptree_node *rn;
  struct gid_machine *machine;
  struct garp_instance *garp_instance;

  if (!gid || !gid->gid_machine_table || !(garp_instance = gid->application))
    return;


  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    return;

  ptree_unlock_node (rn);

  machine = rn->info;

  machine->registrar = REG_STATE_MT;

  /* Decrement the registrations for gip */
  /* Will be done in leave indication */
}

void
gid_leave_request (struct gid *gid, unsigned int gid_index)
{
  struct ptree_node *rn;
  struct gid_machine *machine;

  if (!gid)
    return;

  if (!gid->gid_machine_table)
    return;

  rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &gid_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    return;

  ptree_unlock_node (rn);

  machine = rn->info;

  /* Change applicant's and registar's states for event = GID_EVENT_LEAVE */
  gid_fsm_change_states (gid, machine, GID_EVENT_LEAVE);
}


bool_t
gid_registered_here (struct gid *gid, unsigned int gid_index)
{
  struct gid_states state;
  bool_t ret;
 
  ret = gid_get_attribute_states (gid, gid_index, &state);

  if (! ret)
    return PAL_FALSE;

  return ((state.registrar_state) == GID_REG_STATE_IN);
}

bool_t
gid_registrar_empty_here (struct gid *gid, unsigned int gid_index)
{
  struct gid_states state;
  bool_t ret;

  ret = gid_get_attribute_states (gid, gid_index, &state);

  if (!ret)
    return PAL_FALSE;

  return ((state.registrar_state) == GID_REG_STATE_EMPTY);

}

void
gid_do_actions (struct gid *gid)
{ 
  struct garp_instance *garp_instance = NULL;
  struct gid_port *gid_port = NULL;
  struct garp *garp = NULL;

  if (!gid)
    return;

  garp_instance = gid->application;
  gid_port = gid->gid_port;

  if (!garp_instance || !gid_port)
    return;

  garp = garp_instance->garp;

  if (!garp)
    return;


  if (gid->cstart_join_timer)
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_do_actions: [port %d] "
                      "Setting tx_pending to TRUE and cstart_join_timer to "
                      "FALSE", gid_port->ifindex);
        }

      gid->tx_pending = PAL_TRUE;
      gid->cstart_join_timer = PAL_FALSE;
    }

  if (!gid->hold_tx)
    {
       /* check if there is any pdu to be sent scheduled now for this port */
      if (gid->cschedule_tx_now)
        {
          /* schedule now for this port if none is scheduled for tx */
          if (!gid->tx_now_scheduled)
            {
              if (GARP_DEBUG(EVENT))
                {
                  zlog_debug (garp->garpm, "gid_do_actions: [port %d] "
                              "Starting join timer immediately", gid_port->ifindex);
                }

               garp_schedule_now (gid_join_timer_handler, gid);
            }

          gid->cschedule_tx_now = PAL_FALSE;
        }
      else if ((gid->tx_pending || (gid->leave_all_countdown == 0)) &&
               (!gid->join_timer_running))
        {
          if (GARP_DEBUG(EVENT))
            {
              zlog_debug (garp->garpm, "gid_do_actions: [port %d] "
                          "Starting join timer for timer_value %u", 
                          gid_port->ifindex, gid_port->join_timeout);
            }
  
           gid->join_timer_thread = garp_start_random_timer (
                                        gid_join_timer_handler,
                                        gid, 
                                        0,
                                        gid_port->join_timeout);


           gid->join_timer_running = PAL_TRUE;

        }
    }

  if ( gid->cstart_leave_timer && (!gid->leave_timer_running) &&
      (gid->leave_timer_thread == NULL) )
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_do_actions: [port %d] "
                      "Starting leave timer timer_value %u", gid_port->ifindex,
                      gid_port->leave_timeout_4);
        }

      gid->leave_timer_thread = garp_start_timer (
                                    gid_leave_timer_handler,
                                    gid,
                                    gid_port->leave_timeout_4); 


      gid->leave_timer_running = PAL_TRUE;
      gid->cstart_leave_timer = PAL_FALSE;
    }

    gid->cstart_leave_timer = PAL_FALSE;
}

enum gid_event
gid_tx_attribute (struct gid *gid, unsigned int start_index,
                  u_int32_t *gid_index)
{
  enum gid_event new_event;
  struct garp_instance *garp_instance = gid->application;
  struct gid_port *gid_port = gid->gid_port;
  struct garp *garp = NULL;
  struct ptree_node *rn, *rn_next;
  struct gid_machine *machine;
  struct gid_machine *tmp_machine;

  if (!garp_instance || !gid_port)
    return GID_EVENT_NULL;

  garp = garp_instance->garp;

  if (!garp)
    return GID_EVENT_NULL;

  /* If nothing then last_gid_index_used is returned as gid_index */
  *gid_index = 0;

  if (gid->hold_tx)
    {
      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_tx_attribute: [port %d] "
                      "hold_tx is TRUE, do not transmit event GID_EVENT_NULL",
                      gid_port->ifindex);
        }
      return GID_EVENT_NULL;
    }

  if (gid->leave_all_countdown == 0)
    {
      gid->leave_all_countdown = GID_LEAVE_ALL_COUNT;

      garp_stop_timer (&gid->leave_all_timer_thread);
      gid->leave_all_timer_thread =
            garp_start_timer ( gid_leave_all_timer_handler,
                               gid,
                               gid_port->leave_all_timeout_n);

      if (GARP_DEBUG(EVENT))
        {
           zlog_debug (garp->garpm, "gid_tx_attribute: [port %d] "
                       "leave_all_countdown is 0, start leave_all timer, "
                       "event GID_EVENT_TX_LEAVE_ALL", gid_port->ifindex);
        }
      *gid_index = 0;
      return GID_EVENT_TX_LEAVE_ALL;
    }

  /* nothing to transmit */
  if (!gid->tx_pending)
    return GID_EVENT_NULL;

  if (gid->next_to_transmit == 0)
    {
      /* Get the first attribute and tx */
      rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &start_index,
                              ATTR_INDX_MAX_BITLEN);

      if (rn == NULL
          || rn->info == NULL)
         {
           rn = ptree_top (gid->gid_machine_table);

           while (rn != NULL)
             {
                if (rn->info != NULL)
                  break;

                 rn = ptree_next (rn);
             }
         }
    }
  else
    {
      rn = ptree_node_lookup (gid->gid_machine_table, 
                              (u_char *) &gid->next_to_transmit,
                              ATTR_INDX_MAX_BITLEN);
    }

  if (rn && rn->info)
    {
      /* Tx this attr */
      machine = rn->info;

      *gid_index = gid_get_attr_index (rn);

      new_event = gid_fsm_change_tx_states (gid, machine);

      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_tx_attribute: [port %d] "
                      "new_event %s tx_pending %u",
                       gid_port->ifindex,  gid_event_string[new_event], gid->tx_pending);
        }

      /* Skip the attribute indexes with GID_EVENT_NULL */
      if (new_event == GID_EVENT_NULL)
        {
          rn = ptree_next (rn);

          while (rn)
            {
              if ((machine = rn->info)
                  && (new_event = gid_fsm_change_tx_states (gid, machine))
                      != GID_EVENT_NULL)
                {
                  *gid_index = gid_get_attr_index (rn);
                  break;
                }

               rn = ptree_next (rn);
            }
        }

      if (new_event != GID_EVENT_NULL)
        {
          *gid_index = (gid->last_transmitted = gid_get_attr_index (rn));
           rn_next = ptree_next (rn);

           while (rn_next)
             {
               if (rn_next->info)
                  break;
               else
                 rn_next = ptree_next (rn_next);
             }

           if (rn_next)
             ptree_unlock_node (rn_next);

           if (rn_next && rn_next->info)
             {
               gid->tx_pending = PAL_TRUE;
               tmp_machine = rn_next->info;
               gid->next_to_transmit = gid_get_attr_index (rn_next);
             }
           else
             {
               gid->next_to_transmit= 0;
               gid->tx_pending = PAL_FALSE;
             }

           if (GARP_DEBUG(EVENT))
             {
               zlog_debug (garp->garpm, "gid_tx_attribute: [port %d] "
                           "gid_index %u new_event %s tx_pending %u",
                           gid_port->ifindex, *gid_index,
                           gid_event_string[new_event], gid->tx_pending);
             }

           return new_event;
         }
       else if (rn)
         {
           ptree_unlock_node (rn);
         }
    }

  *gid_index = 0;
  return GID_EVENT_NULL;
}

#if defined HAVE_MVRP || defined HAVE_MMRP

void
mad_get_tx_leave_all_event (struct gid *gid, u_int8_t *tx_event,
                            u_int8_t *leave_all_event)
{
  struct gid_port *gid_port = gid->gid_port;
  struct garp_instance *garp_instance;
  struct garp *garp;

  if (gid_port == NULL
      || (garp_instance = gid->application) == NULL
      || (garp = garp_instance->garp) == NULL)
    return;

  *leave_all_event = MRP_LEAVE_ALL_EVENT_NULL;
  *tx_event = MRP_TX_EVENT_NULL;

  if (gid->leave_all_countdown == 0)
    {
      gid->leave_all_countdown = GID_LEAVE_ALL_COUNT;

      gid->leave_all_timer_thread =
            garp_start_timer ( gid_leave_all_timer_handler,
                               gid,
                               gid_port->leave_all_timeout_n);

      if (GARP_DEBUG(EVENT))
        {
           zlog_debug (garp->garpm, "mad_get_tx_event : [port %d] "
                       "leave_all_countdown is 0, start leave_all timer, ",
                       gid_port->ifindex);
        }

      if (gid_port->p2p)
        {
          *tx_event = MRP_TX_EVENT_LEAVE_MINE;
          *leave_all_event = MRP_LEAVE_ALL_EVENT_LEAVE_MINE;
          return;
        }
      else
        {
          *tx_event = MRP_TX_EVENT_LEAVE_ALL;
          *leave_all_event = MRP_LEAVE_ALL_EVENT_LEAVE_ALL;
          return;
        }
    }

  if (gid_port->p2p)
    {
      *tx_event = MRP_TX_EVENT_P2P;
      return;
    }
  else
    {
      *tx_event = MRP_TX_EVENT_NOT_P2P;
      return;
    }

  return;
}

enum gid_event
mad_tx_attribute (struct gid *gid, unsigned int start_index,
                  unsigned int *gid_index, mrp_tx_event_t tx_event)
{
  struct garp_instance *garp_instance = gid->application;
  struct gid_port *gid_port = gid->gid_port;
  struct ptree_node *rn, *rn_next;
  struct gid_machine *tmp_machine;
  enum gid_event new_event = 0;
  struct gid_machine *machine;
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return GID_EVENT_NULL;

  garp = garp_instance->garp;

  if (!garp)
    return GID_EVENT_NULL;
  
  /* If nothing then last_gid_index_used is returned as gid_index */
  *gid_index = 0;
  
  if (gid->hold_tx)
    {
      if (GARP_DEBUG(EVENT)) 
        {
          zlog_debug (garp->garpm, "gid_tx_attribute: [port %d] "
                      "hold_tx is TRUE, do not transmit event GID_EVENT_NULL",
                      gid_port->ifindex); 
        }
      return GID_EVENT_NULL;
    }

  /* nothing to transmit */
  if (!gid->tx_pending)
    return GID_EVENT_NULL;

  if (gid->next_to_transmit == 0)
    {
      /* Get the first attribute and tx */
      rn = ptree_node_lookup (gid->gid_machine_table, (u_char *) &start_index,
                              ATTR_INDX_MAX_BITLEN);
    }
  else
    {
      rn = ptree_node_lookup (gid->gid_machine_table,
                              (u_char *) &gid->next_to_transmit,
                              ATTR_INDX_MAX_BITLEN);
    }

  if (rn && rn->info)
    {
      /* Tx this attr */
      machine = rn->info;          

      *gid_index = gid_get_attr_index (rn);
      new_event = mad_fsm_change_tx_states (gid, machine,
                                            tx_event);

      if (GARP_DEBUG(EVENT))
        {
          zlog_debug (garp->garpm, "gid_tx_attribute: [port %d] "
                      "new_event %s tx_pending %u",
                       gid_port->ifindex,  gid_event_string[new_event], gid->tx_pending);
        }
      
      /* Skip the attribute indexes with GID_EVENT_NULL */
      if (new_event == GID_EVENT_NULL)
        {
          rn = ptree_next (rn);

          while (rn)
            {
              if ((machine = rn->info)
                  && (new_event = mad_fsm_change_tx_states (gid, machine,
                                                            tx_event))
                      != GID_EVENT_NULL)
                {
                  *gid_index = gid_get_attr_index (rn);
                  break;
                }

               rn = ptree_next (rn);
            }
        }

      if (new_event != GID_EVENT_NULL)
        {
          *gid_index = (gid->last_transmitted = gid_get_attr_index (rn));
           rn_next = ptree_next (rn);

           while (rn_next)
             {
               if (rn_next->info)
                  break;
               else
                 rn_next = ptree_next (rn_next);
             }

           if (rn_next)
             ptree_unlock_node (rn_next);

           if (rn_next && rn_next->info)
             {
               gid->tx_pending = PAL_TRUE;
               tmp_machine = rn_next->info;
               gid->next_to_transmit = gid_get_attr_index (rn_next);
             }
           else
             {
               gid->next_to_transmit = 0; 
               gid->tx_pending = PAL_FALSE;
             }

           if (GARP_DEBUG(EVENT))
             {
               zlog_debug (garp->garpm, "gid_tx_attribute: [port %d] "
                           "gid_index %u new_event %s tx_pending %u",
                           gid_port->ifindex, *gid_index,
                           gid_event_string[new_event], gid->tx_pending);
             } 

           return new_event;
         }
       else if (rn)
         {
           ptree_unlock_node (rn);
         }
    }

   gid->tx_pending = PAL_FALSE;
  *gid_index = 0;
  return GID_EVENT_NULL;
}

#endif /* HAVE_MVRP || HAVE_MMRP */

void
gid_set_timer (struct gid_port *gid_port, u_int32_t timer_type, 
               pal_time_t timer_value)
{
  switch (timer_type)
    {
      case GARP_JOIN_TIMER :
         gid_port->join_timeout = timer_value;
        break;
      case GARP_LEAVE_TIMER :
        {
           gid_port->leave_timeout_4 = timer_value / 4;
           gid_port->leave_conf_timeout = timer_value;
          break;
        }
      case GARP_LEAVE_ALL_TIMER :
        {   
           gid_port->leave_all_timeout_n = timer_value / GID_LEAVE_ALL_COUNT;
           gid_port->leave_all_conf_timeout = timer_value;
          break;
        }
      default :
        break;
    }
}

void
gid_get_timer (struct gid_port *gid_port, u_int32_t timer_type, 
               pal_time_t *timer_value)
{
  switch (timer_type)
    {
      case GARP_JOIN_TIMER :
         *timer_value = gid_port->join_timeout ;
        break;
      case GARP_LEAVE_TIMER :
         *timer_value = gid_port->leave_timeout_4 * 4;
        break;
      case GARP_LEAVE_ALL_TIMER :
         *timer_value = gid_port->leave_all_timeout_n * GID_LEAVE_ALL_COUNT;
        break;
      case  GARP_LEAVE_CONF_TIMER :
         *timer_value = gid_port->leave_conf_timeout;
        break;
      case GARP_LEAVEALL_CONF_TIMER :
         *timer_value = gid_port->leave_all_conf_timeout;
        break;
      default :
        break;
    }
}

void
gid_get_timer_details (struct gid_port *gid_port, pal_time_t *timer_details)
{
  timer_details[GARP_JOIN_TIMER] = gid_port->join_timeout;
  timer_details[GARP_LEAVE_TIMER] = gid_port->leave_timeout_4 * 4;
  timer_details[GARP_LEAVE_ALL_TIMER] = gid_port->leave_all_timeout_n * GID_LEAVE_ALL_COUNT;
  timer_details[GARP_LEAVE_CONF_TIMER] = gid_port->leave_conf_timeout;
  timer_details[GARP_LEAVEALL_CONF_TIMER] = gid_port->leave_all_conf_timeout;
}

s_int32_t
gid_get_attr_index (struct ptree_node *rn)
{
  if (!rn || !rn->info)
    return -1;

  return (* ((s_int32_t *) rn->key));
}

bool_t
gid_attr_entry_to_be_created (struct gid *gid, u_int32_t attribute_index)
{
  u_int32_t *gip;
  struct ptree_node *rn;
  struct garp_instance *garp_instance;

  if (!gid)
    return PAL_FALSE;

  garp_instance = gid->application;

  if (!garp_instance)
    return PAL_FALSE;
  
  rn = ptree_node_lookup (garp_instance->gip_table, (u_char *) &attribute_index,                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    return PAL_FALSE;
  else
    {
      ptree_unlock_node (rn);

      if (!rn->info)
        return PAL_FALSE;

      gip = rn->info;

      if ((*gip-1) == 0)
        return PAL_TRUE;
    }  

  return PAL_FALSE;
}

bool_t
gid_app_object_is_to_be_deleted (struct gid *gid, u_int32_t attribute_index)
{
  u_int32_t *gip;
  struct ptree_node *rn;
  struct garp_instance *garp_instance;

  if (!gid)
    return PAL_FALSE;

  garp_instance = gid->application;

  if (!garp_instance)
    return PAL_FALSE;

  rn = ptree_node_lookup (garp_instance->gip_table, (u_char *) &attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    return PAL_FALSE;
  else
    {
      ptree_unlock_node (rn);

      if (!rn->info)
        return PAL_FALSE;

      gip = rn->info;

      if ((*gip) == 0)
        return PAL_TRUE;
    }

  return PAL_FALSE;
}

 
