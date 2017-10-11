/* Copyright (C) 2003  All Rights Reserved. */
#include "pal.h"
#include "lib.h"
#include "table.h"

#include "nsm_interface.h"
#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#include "garp_gip.h"
#include "garp_gid_fsm.h"
#include "garp_gid_timer.h"
#include "garp_debug.h"

struct thread *
garp_start_random_timer (s_int32_t (*func) (struct thread *thread), 
                         struct gid *gid, pal_time_t min_timeout,
                         pal_time_t max_timeout)
{
  struct garp_instance *garp_instance = gid->application;
  struct gid_port *gid_port = gid->gid_port;
  pal_time_t rand_time;
  struct pal_timeval tv;
                        
  struct garp *garp = NULL;


  if (!garp_instance || !gid_port)
    return NULL;

  garp = garp_instance->garp;

  if (!garp)
    return NULL;

  rand_time = garp_rand (min_timeout, max_timeout);
  tv.tv_sec = CENTISECS_TO_SECS(rand_time); 
  tv.tv_usec = CENTISECS_TO_SUBSECS(rand_time);

  if (GARP_DEBUG(TIMER))
    {
      zlog_debug (garp->garpm, "garp_start_random_timer: [port %d] "
                  "Starting random timer, timer value %u", gid_port->ifindex, 
                  rand_time); 
    }

  return thread_add_timer_timeval (garp->garpm, func, gid, tv);
}


struct thread *
garp_start_timer (s_int32_t (*func) (struct thread *t), struct gid *gid,
                  pal_time_t time)
{
  struct garp_instance *garp_instance = gid->application;
  struct gid_port *gid_port = gid->gid_port;
  struct pal_timeval tv; 
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return NULL;

  garp = garp_instance->garp;

  if (!garp)
    return NULL;

  tv.tv_sec = CENTISECS_TO_SECS(time); 
  tv.tv_usec = CENTISECS_TO_SUBSECS(time);

  if (GARP_DEBUG(TIMER))
    {
      zlog_debug (garp->garpm, "garp_start_timer: [port %d] "
                  "Starting timer, timer value %u", gid_port->ifindex, time); 
    }

  return thread_add_timer_timeval (garp->garpm, func, gid, tv);
}


void
garp_stop_timer (struct thread **thread)
{
  if (thread && (*thread) && garp_is_timer_running (*thread))
    {
      thread_cancel (*thread);
      *thread = 0;
    }
}

struct thread *
garp_schedule_now (s_int32_t (*func) (struct thread *thread), struct gid *gid)
{
  pal_time_t time = 0;
  struct pal_timeval tv = { CENTISECS_TO_SECS(time), 
                            CENTISECS_TO_MICROSECS(time)};
  struct garp_instance *garp_instance = gid->application;
  struct gid_port *gid_port = gid->gid_port;
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return NULL;

  garp = garp_instance->garp;

  if (!garp)
    return NULL;


  if (GARP_DEBUG(TIMER))
    {
      zlog_debug (garp->garpm, "garp_schedule_now: [port %d] "
                  "Scheduling timer immediately", gid_port->ifindex); 
    }

  return thread_add_timer_timeval (garp->garpm, func, gid, tv);
}


s_int32_t
gid_leave_timer_handler (struct thread *thread)
{ 
  u_int32_t gid_index;
  struct gid *gid = thread ? (struct gid *)(thread->arg) : NULL;
  struct interface *ifp = NULL;
  struct nsm_if    *zif = NULL;
  struct apn_vr    *vr = NULL;
  struct garp_instance *garp_instance = NULL;
  struct gid_port *gid_port = NULL;
  struct garp *garp = NULL;
  struct ptree_node *rn;
  struct gid_machine *machine;
  struct nsm_bridge_port *br_port;

  if (gid == NULL)
    return 0;

  garp_instance = gid->application;
  gid_port = gid->gid_port;

  if (!garp_instance || !gid_port)
    return 0;

  garp = garp_instance->garp;

  if (!garp)
    return 0;

  vr = apn_vr_get_privileged (garp->garpm);

  if (!vr)
    return 0;

  ifp = if_lookup_by_index(&vr->ifm, gid_port->ifindex);

  if (!ifp || !(zif = ifp->info) || !(br_port = zif->switchport))
    return 0;

  gid->leave_timer_thread = NULL;
  
  /* Reset the timer flag as not running */
  gid->leave_timer_running = PAL_FALSE;

  for (rn = ptree_top (gid->gid_machine_table); rn;
       rn = ptree_next (rn))
    {
      gid_index = gid_get_attr_index (rn);
      if (GARP_DEBUG(TIMER))
        {
          zlog_debug (garp->garpm, "gid_leave_timer_handler: [port %d] "
                      "Changing states for gid_index %u", gid_port->ifindex, 
                      gid_index);
        }

      if (!rn->info)
        continue;

      machine = rn->info;
      if (gid_fsm_leave_timer_expiry (gid, machine)
          == GID_EVENT_LEAVE)
        {
          if (GARP_DEBUG(TIMER))
            {
              zlog_debug (garp->garpm, "gid_leave_timer_handler: [port %d]"
                          " Sending leave indication to the garp application "
                          "for gid_index %u and propagating leave through gip",
                          gid_port->ifindex, gid_index);
            }

          /* inform the application about the leave event */
          garp->leave_indication_func (garp_instance->application,
                                       gid, gid_index);

          /* Propagate the leave through GIP if the port is forwarding.*/
          /* Section 12.3.4 */
          if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
            { 
              /* Propagate through GIP about the leave event */
              gip_propagate_leave (gid, gid_index);
              gip_do_actions (gid);
            }

        }
    } /* end of for */
  
  gid_do_actions (gid);
  
  return 0;
}



s_int32_t
gid_join_timer_handler (struct thread *thread)
{
  struct gid *gid = thread ? (struct gid *)(thread->arg) : NULL;
  struct garp_instance *garp_instance = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct gid_port *gid_port = NULL;
  struct interface *ifp = NULL;
  struct nsm_if    *zif = NULL;
  struct apn_vr    *vr = NULL;
  struct garp *garp = NULL;

  if (gid == NULL)
    return 0;

  garp_instance = gid->application;

  gid_port = gid->gid_port;

  if (!garp_instance || !gid_port)
    return 0;

  garp = garp_instance->garp;

  if (!garp)
    return 0;

  vr = apn_vr_get_privileged (garp->garpm);

  if (!vr)
    return 0;

  ifp = if_lookup_by_index(&vr->ifm, gid_port->ifindex);

  if (!ifp || !(zif = ifp->info) || !(br_port = zif->switchport))
    return 0;

  gid->join_timer_thread = NULL;

  /* Reset the timer flag as not running */
  gid->join_timer_running = PAL_FALSE;
  
  if (GARP_DEBUG(TIMER))
    {
      zlog_debug (garp->garpm, "gid_join_timer_handler: [port %d] "
                  "Calling garp application transmit function ",
                  gid_port->ifindex);
    }

  /* inform the garp application about the join timer expired
     and to start transmit PDU  for this port/gid_instance */
  /* Setting next_to_transmit to 0, forces the transmit function
     to go over all machines to check for any pending transmission */

  /* Important assumption is that single call to tansmit_func of application
     will take care of sending PDUs for all attributes registered with gid,
     including when all attributes don't fit in a single PDU */
  /* Propagate the leave through GIP if the port is forwarding.*/
          /* Section 12.3.4 */
  if (br_port->state == NSM_BRIDGE_PORT_STATE_FORWARDING)
    {
      gid->next_to_transmit = 0;
      gid->tx_pending = PAL_TRUE;
      garp->transmit_func (garp_instance->application, gid);
    }
  gid_do_actions (gid);
  return 0;
}


s_int32_t
gid_leave_all_timer_handler (struct thread *thread)
{
  struct gid *gid = thread ? (struct gid *)(thread->arg) : NULL;
  struct garp_instance *garp_instance = NULL;
  struct gid_port *gid_port = NULL;
  struct garp *garp = NULL;

  if (gid == NULL)
    return 0;

  garp_instance = gid->application;
  gid_port = gid->gid_port;

  if (!garp_instance || !gid_port)
    return 0;

  garp = garp_instance->garp;

  if (!garp)
    return 0;

  gid->leave_all_timer_thread = NULL;

  /* Wait for atleast two countdown period before taking any action */
  if (gid->leave_all_countdown > 1)
    {
      gid->leave_all_countdown--;
      if (GARP_DEBUG(TIMER))
        {
          zlog_debug (garp->garpm, "gid_leave_all_timer_handler: [port %d]"
                      " Decrementing the leave_all_countdown to %u",
                      gid_port->ifindex, gid->leave_all_countdown);
        }
      /* start the leave_all timer */
      gid->leave_all_timer_thread = garp_start_random_timer (
                                    gid_leave_all_timer_handler, 
                                    gid,
                                    gid_port->leave_all_timeout_n,
                                    gid_port->leave_all_timeout_n*1.5);
    }
  else 
    {
      if (GARP_DEBUG(TIMER))
        {
          zlog_debug (garp->garpm, "gid_leave_all_timer_handler: [port %d]"
                      " Calling gid_leave_all", gid_port->ifindex);
        }
          
      /* Change the applicant and registrar state for leave_empty event */
      gid_leave_all (gid, 0, 0);
      gid->leave_all_countdown = 0;
      gid->cstart_join_timer = PAL_TRUE;

      if ((!gid->join_timer_running) && 
          (!gid->hold_tx))
        {
          if (GARP_DEBUG(TIMER))
            {
              zlog_debug (garp->garpm, "gid_leave_all_timer_handler: "
                          "[port %d] Starting the join_timer, timer value %u",
                          gid_port->ifindex, gid_port->join_timeout);
            }
          
          gid->join_timer_thread = garp_start_random_timer (
                                       gid_join_timer_handler,
                                       gid,
                                       ((gid_port->join_timeout) - GID_MAX_JOIN_TIME_DEVIATION),
                                       gid_port->join_timeout);

          gid->join_timer_running = PAL_TRUE;
        }
      
      gid->cstart_join_timer = PAL_FALSE;
    }

  return 0;
}

s_int32_t
gid_hold_timer_handler (struct thread *thread)
{
  struct gid *gid = thread ? (struct gid *)(thread->arg) : NULL;
  struct garp_instance *garp_instance = NULL;
  struct gid_port *gid_port = NULL;
  struct garp *garp = NULL;

  if (gid == NULL)
    return 0;

  garp_instance = gid->application;
  gid_port = gid->gid_port;

  if (!garp_instance || !gid_port)
    return 0;

  garp = garp_instance->garp;

  if (!garp)
    return 0;


  gid->hold_timer_thread = NULL;

  /* No longer hold for any pdus to be trasmitted */
  gid->hold_tx = PAL_FALSE;
  
  if (GARP_DEBUG(TIMER))
    {
      zlog_debug (garp->garpm, "gid_hold_timer_handler: [port %d] "
                  "Calling gid_do_actions", gid_port->ifindex);
    }
          
  gid_do_actions (gid);

  return 0;
}
