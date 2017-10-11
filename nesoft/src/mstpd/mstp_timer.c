/* Copyright (C) 2003-2004  All Rights Reserved.

MSTP Timer handlers.

*/

#include "thread.h"
#include "log.h"
#include "l2lib.h"
#include "mstpd.h"
#include "mstp_config.h"
#include "mstp_types.h"
#include "l2_timer.h" 
#include "mstp_transmit.h" 
#include "mstp_bpdu.h"
#include "mstp_cist_proto.h"
#include "mstp_msti_proto.h"
#include "mstp_port.h"
#include "mstp_bridge.h"

#include "l2_debug.h"

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */
#ifdef HAVE_SNMP
#include "mstp_snmp.h"
#endif /*HAVE_SNMP */

#define DEC(x) { if(x !=0) x--; }

int 
mstp_port_timer_handler (struct thread * t)
{
  struct mstp_port * port =  t ? (struct mstp_port *)t->arg : NULL ;
  
  if (!port)
    return RESULT_ERROR;
 
#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_delete_mstp_port_timer (port);
#endif /* HAVE_HA */

  DEC(port->tx_count);

  port->port_timer = l2_start_timer (mstp_port_timer_handler, (void *)port, 
                                     ONE_SEC,mstpm);

#ifdef HAVE_HA
   mstp_cal_create_mstp_port_timer (port);
#endif /* HAVE_HA */
 
  return RESULT_OK;

}
#ifdef HAVE_RPVST_PLUS
static void
mstp_msti_set_rpvst_sstp_flag (struct mstp_port *port)
{
    struct mstp_instance_port *inst_port = port->instance_list;

    while (inst_port)
      {
        inst_port->newInfoSstp = inst_port->newInfoSstp || 
                                 (inst_port->any_msti_desigport ||
                                 (inst_port->any_msti_rootport && 
                                 (l2_is_timer_running(inst_port->tc_timer))));
             
        inst_port = inst_port->next_inst;
      }
}
#endif /* HAVE_RPVST_PLUS */

int mstp_msti_tc_l2_is_timer_running (struct mstp_port *port)
{
  struct mstp_instance_port *temp_port = port->instance_list;

  while (temp_port)
    {
      if (l2_is_timer_running(temp_port->tc_timer))
        {
         return PAL_TRUE;
        }
      temp_port = temp_port->next_inst;
    }

    return PAL_FALSE;
}

/* This function handles the expiry of the hello timer. Section 17.27
   transmit_periodic state of the port transmit state machine  */

int 
mstp_hello_timer_handler (struct thread * t)
{
  struct mstp_port * port =  t ? (struct mstp_port *)t->arg : NULL ;

#ifdef HAVE_L2GP
  struct interface * ifp = NULL;
#endif /* HAVE_L2GP */
  
  if (!port)
    return RESULT_ERROR;

  if (LAYER2_DEBUG (proto, TIMER_DETAIL))
    zlog_debug (mstpm, "mstp_hello_timer_handler: "
                "expired for bridge %s index %u", port->br->name, port->ifindex);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_delete_mstp_hello_timer (port);
#endif /* HAVE_HA */

  /* For STP bridge do not generate BPDU if it is not root bridge */
  if ( IS_BRIDGE_STP (port->br)
       && ! mstp_bridge_is_cist_root (port->br))
    {
      port->hello_timer = l2_start_timer (mstp_hello_timer_handler,
                                          (void *)port,
                                          port->br->cist_hello_time, mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_hello_timer (port);
#endif /* HAVE_HA */

      return RESULT_OK;
    }

#ifdef HAVE_L2GP
  /* Look up interface so that check current status.  */
  ifp = ifg_lookup_by_index (&mstpm->ifg, port->ifindex);

  if (!ifp)
    return RESULT_ERROR;

  if (port->isL2gp == PAL_TRUE && if_is_up(ifp))
    {
      mstp_l2gp_prep_and_send_pseudoinfo(port);
      zlog_debug (mstpm, "sending psuedo BPDU to state machine from "
                "%s port", ifp->name);
    }
  else 
    {
#endif /* HAVE_L2GP */
     DEC(port->tx_count);
     DEC(port->helloWhen);
     port->newInfoCist = port->newInfoCist ||
       ( (port->cist_role == ROLE_DESIGNATED) ||
       ( (port->cist_role == ROLE_ROOTPORT) &&
        l2_is_timer_running(port->tc_timer) ) );

     port->newInfoMsti = port->newInfoMsti || 
       ( port->any_msti_desigport ||
       ( port->any_msti_rootport &&
        mstp_msti_tc_l2_is_timer_running( port ) ) );

#ifdef HAVE_RPVST_PLUS
     if (port->br->type == NSM_BRIDGE_TYPE_RPVST_PLUS)
       {
         port->rpvst_event = RPVST_PLUS_TIMER_EVENT;
         mstp_msti_set_rpvst_sstp_flag(port);
       }
#endif
     mstp_tx(port);
#ifdef HAVE_L2GP
    }
#endif /* HAVE_L2GP */
 
  if (IS_BRIDGE_MSTP (port->br)|| IS_BRIDGE_RPVST_PLUS(port->br)) 
    {
      port->hello_timer = l2_start_timer (mstp_hello_timer_handler,
                                          (void *)port, 
                                          port->cist_hello_time, mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_hello_timer (port);
#endif /* HAVE_HA */
    }
  else
    {
      port->hello_timer = l2_start_timer (mstp_hello_timer_handler,
                                          (void *)port, 
                                          port->br->cist_hello_time, mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_hello_timer (port);
#endif /* HAVE_HA */
    }

  return RESULT_OK;
}


/* This function handles the expiry of the Protocol Migraton
 * timer . IEEE 802.1w-2001 Section 17.26 */
   
int 
mstp_migrate_timer_handler (struct thread * t)
{
  struct mstp_port * port =  t ? (struct mstp_port *)t->arg : NULL ;
 
  if (!port)
    return RESULT_ERROR;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_delete_mstp_migrate_timer (port);
#endif /* HAVE_HA */

  port->migrate_timer = NULL;

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_migrate_timer_handler:expired for bridge "
                "%s port %u", port->br->name, port->ifindex);

  if (!port->rcvd_mstp && !port->rcvd_stp)
    {
      /* We have not recieved anyting. No need to do anything. */
      return RESULT_OK;
    }
  /* Does the value of recvd mstp  match that of the send mstp ? */
  if (port->send_mstp != port->rcvd_mstp)
    {
      port->send_mstp ? mstp_sendstp(port) : mstp_sendmstp(port);
    }


  return RESULT_OK;
}


/* This function is called when the message age timer expires.
   Implements the AGED state in Port Information State Machine.
   IEEE 802.1w-2001 Section 17.21*/
int
mstp_message_age_timer_handler (struct thread * t)
{
  struct mstp_port * port; 
  struct mstp_instance_port *inst_port  = NULL;
  bool_t is_root;

  port = t ? (struct mstp_port *)t->arg : NULL;
   /* port or thread has gone dead */
    if (!port)
      return RESULT_ERROR;

   is_root = mstp_bridge_is_cist_root (port->br);

  is_root = mstp_bridge_is_cist_root (port->br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_delete_mstp_message_age_timer (port);
#endif /* HAVE_HA */

  port->message_age_timer = NULL;

  if (port->updtInfo)
    return RESULT_ERROR;

  /* Ignore latent timers if disabled. */
  if (port->cist_info_type == INFO_TYPE_DISABLED)
    return RESULT_OK;

  zlog_warn (mstpm, "BPDU Skew detected on port %u, Bridge %s becoming root",
             port->ifindex, port->br->name); 

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_message_age_timer_handler: expired for bridge"
                " %s index %u", port->br->name, port->ifindex);

  port->cist_info_type = INFO_TYPE_AGED;
  port->reselect = PAL_TRUE;
  port->selected = PAL_FALSE;

#ifdef HAVE_SNMP
  /* stp becoming root */
  mstp_snmp_new_root (port->br);
#endif /* HAVE_SNMP */

  mstp_cist_port_role_selection (port->br);

  if (!port->info_internal)
    {
      inst_port = port->instance_list;
      while (inst_port)
        {
           mstp_msti_port_role_selection (inst_port->br);
           inst_port = inst_port->next_inst;
        }
    }

  if (IS_BRIDGE_STP (port->br) && ! is_root 
      && mstp_bridge_is_cist_root (port->br))
    {
      stp_topology_change_detection (port->br);
#ifdef HAVE_HA
      mstp_cal_delete_mstp_tcn_timer (port->br);
#endif /* HAVE_HA */

      l2_stop_timer (&port->br->tcn_timer);
      stp_generate_bpdu_on_bridge (port->br);
    }
  else
    mstp_tx_bridge (port->br);

  return RESULT_OK;
}



/* This function handles the expiry of the topology change timer. */
    

int
mstp_tc_timer_handler (struct thread * t)
{
  struct mstp_port * port =  t ? (struct mstp_port *)t->arg : NULL ;
  struct mstp_bridge * br = port ? port->br : NULL; 
  if ((!port) || (!br))
    return RESULT_ERROR;

#ifdef HAVE_HA
  if (l2_is_timer_running (port->tc_timer))
    mstp_cal_delete_mstp_tc_timer (port);
#endif /* HAVE_HA */

  port->tc_timer = NULL;

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_tc_timer_handler: expired for bridge %s port %u",
                port->br->name, port->ifindex);
 
  /* As the mstp_tx function simply looks at wheather tc timer is running or not
     We don't need to do anything. Just clear ourselves and sit tight */ 
  return RESULT_OK;
}



/* mstp_forward_delay_time_handler handles the exiry of the forward delay timer.
   It updates the state of the port (LISTENING -> LEARNING 
   or LEARNING -> FORWARDING). */

int 
mstp_forward_delay_timer_handler (struct thread * t)
{
  struct mstp_port * port =  t ? t->arg : NULL ;
  struct mstp_bridge * br = port ? port->br : NULL; 
  if ((!port) || (!br))
    return RESULT_ERROR;
  
#ifdef HAVE_HA
  if (l2_is_timer_running (port->forward_delay_timer))
    {
      mstp_cal_delete_mstp_forward_delay_timer (port);
      mstp_cal_modify_mstp_port (port);
    }
#endif /* HAVE_HA */

  port->forward_delay_timer = NULL;

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_forward_delay_timer_handler: expired "
                "for bridge %s port %u state (%d)", 
                br->name, port->ifindex, port->cist_state);

  if ((port->cist_state == STATE_DISCARDING)  
      && ((port->cist_role == ROLE_ROOTPORT)
          || (port->cist_role == ROLE_DESIGNATED)))
    {
      mstp_cist_set_port_state (port, STATE_LEARNING);
      
      port->forward_delay_timer = 
        l2_start_timer (mstp_forward_delay_timer_handler, 
                        port, mstp_cist_get_fdwhile_time (port), mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
    }
  else if (port->cist_state == STATE_LEARNING)
    {
      mstp_cist_set_port_state (port,STATE_FORWARDING);
      port->send_proposal = PAL_FALSE;

      if (mstp_cist_has_designated_port (br)
          && IS_BRIDGE_STP (br))
        stp_topology_change_detection (br);
    }

  mstp_tx_bridge (br);

  return RESULT_OK;
}

int
mstp_rr_timer_handler (struct thread *t)
{
  
  struct mstp_port * port;
  struct mstp_port *root_port ;
  struct mstp_bridge *br;
   
  port = t ? (struct mstp_port *)t->arg : NULL;
  
  if (!port)
    return RESULT_ERROR;

   br = port->br;
  
  br = port->br;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_modify_mstp_bridge (port->br);
  mstp_cal_delete_mstp_rr_timer (port);
#endif /* HAVE_HA */

  port->recent_root_timer = NULL;
  port->br->br_all_rr_timer_cnt--;
  
  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_rr_timer_handler:expired for bridge %s port %u",
                port->br->name, port->ifindex);

  /* clear the corresponding bit from recent root port map  */
  if (br->recent_root)
    {
      /* check if any other recent roots are running 
         if not transition  the port indicated by reroot 
         parameter of the bridge to forwarding state
         and clear the recent root parameter of the bridge
      */
      /* Check if all recent root timers  have expired */
      if (br->br_all_rr_timer_cnt == 0)
        {
          root_port = mstp_find_cist_port(br,br->recent_root);
          if (root_port == NULL)
            {
              /* Log Error */
              return RESULT_ERROR;
            }
          /* If so transition this port directly to forwarding state */ 
          mstp_cist_handle_rootport_transition (port);
          br->recent_root = MSTP_BR_NO_PORT;
        }
    }

  return RESULT_OK;
  
}

int
mstp_rb_timer_handler (struct thread *t)
{
  struct mstp_port * port = t ? (struct mstp_port *)t->arg : NULL ;

  if (!port)
    return RESULT_ERROR;

#ifdef HAVE_HA
  mstp_cal_delete_mstp_rb_timer (port);
#endif /* HAVE_HA */

  port->recent_backup_timer = NULL;

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_rb_timer_handler:expired for bridge %s port %u",
                port->br->name, port->ifindex);

  if (port->cist_role == ROLE_ROOTPORT)
    mstp_cist_handle_rootport_transition (port);
  
  return RESULT_OK;

}

/*
  The following fnctions handle the timer expiry for each 
  individual instance.
*/

/* This function is called when the message age timer expires.
   Implements the AGED state in Port Information State Machine.
   IEEE 802.1w-2001 Section 17.21*/
int
mstp_msti_message_age_timer_handler (struct thread * t)
{
  struct mstp_instance_port * inst_port =  t ? 
    (struct mstp_instance_port *)t->arg : NULL ;

  if (!inst_port)
    return RESULT_ERROR;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port); 
  mstp_cal_delete_mstp_inst_message_age_timer (inst_port);
#endif /* HAVE_HA */

  inst_port->message_age_timer = NULL;

  if (inst_port->updtInfo)
    return RESULT_ERROR;

  /* Ignore latent timers if disabled. */
  if (inst_port->info_type == INFO_TYPE_DISABLED)
    return RESULT_OK;

  zlog_warn (mstpm, "BPDU Skew detected on port %u instance %d ,"
             " Bridge %s becoming root", inst_port->ifindex,inst_port->instance_id,
             inst_port->br->bridge->name); 

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_msti_message_age_timer_handler: expired for "
                "bridge %s index %u", inst_port->br->bridge->name, inst_port->ifindex);

  inst_port->info_type = INFO_TYPE_AGED;
  inst_port->reselect = PAL_TRUE;
  inst_port->selected = PAL_FALSE;

  mstp_msti_port_role_selection (inst_port->br);

  mstp_tx_bridge (inst_port->br->bridge);

  return RESULT_OK;
}


/* This function handles the expiry of the topology change timer. */
    

int
mstp_msti_tc_timer_handler (struct thread * t)
{
  struct mstp_instance_port * inst_port = t ? 
    (struct mstp_instance_port *)t->arg : NULL ;
  
  if (!inst_port)
    return RESULT_ERROR;

#ifdef HAVE_HA
  mstp_cal_delete_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */

  inst_port->tc_timer = NULL;

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_msti_tc_timer_handler: expired for "
                "instance %d port %u", inst_port->instance_id, inst_port->ifindex);
 
  /* As the mstp_tx function simply looks at wheather tc timer is running or not
     We don't need to do anything. Just clear ourselves and sit tight */ 
  return RESULT_OK;
}



/* mstp_forward_delay_time_handler handles the exiry of the forward delay timer.
   It updates the state of the port (LISTENING -> LEARNING 
   or LEARNING -> FORWARDING). */

int 
mstp_msti_forward_delay_timer_handler (struct thread * t)
{
  struct mstp_instance_port * inst_port =  t ? t->arg : NULL;
  struct mstp_port *port ;
  
  if ((inst_port == NULL) ||(!(port = inst_port->cst_port)))
    return RESULT_ERROR;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port); 
  mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */

  inst_port->forward_delay_timer = NULL;

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm,"mstp_msti_forward_delay_timer_handler: "
                "expired for bridge %s port %u state (%d)", 
                inst_port->br->bridge->name, inst_port->ifindex, inst_port->state);

  if ((inst_port->state == STATE_DISCARDING)  
      && ((inst_port->role == ROLE_ROOTPORT)
          || (inst_port->role == ROLE_DESIGNATED)))
    {
    
      mstp_msti_set_port_state (inst_port, STATE_LEARNING);
      
      inst_port->forward_delay_timer = 
        l2_start_timer (mstp_msti_forward_delay_timer_handler, 
                        inst_port, 
                        mstp_msti_get_fdwhile_time (inst_port), mstpm);
#ifdef HAVE_HA
      mstp_cal_create_mstp_inst_forward_delay_timer (inst_port);
#endif /* HAVE_HA */
    }
  else if (inst_port->state == STATE_LEARNING)
    {
      
      mstp_msti_set_port_state (inst_port,STATE_FORWARDING);
      inst_port->send_proposal = PAL_FALSE;
    }

  mstp_tx_bridge (port->br);

  return RESULT_OK;
}




int
mstp_msti_rr_timer_handler (struct thread *t)
{
  
  struct mstp_instance_port * inst_port = t ? 
    (struct mstp_instance_port *)t->arg: NULL ;
  struct mstp_instance_port *root_port ;
  struct mstp_bridge_instance *br_inst ;
 
  if ((!inst_port) ||(!(br_inst = inst_port->br)))
    return RESULT_ERROR;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port); 
  mstp_cal_modify_mstp_bridge_instance (inst_port->br);
  mstp_cal_delete_mstp_inst_rr_timer (inst_port);
#endif /* HAVE_HA */

  inst_port->recent_root_timer = NULL;
  inst_port->br->br_inst_all_rr_timer_cnt--;
  
  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_msti_rr_timer_handler:expired for port %u", 
                inst_port->ifindex);

  /* clear the corresponding bit from recent root port map  */
  if (br_inst->recent_root)
    {
      /* check if any other recent roots are running 
         if not transition  the port indicated by reroot 
         parameter of the bridge to forwarding state
         and clear the recent root parameter of the bridge
      */
      /* Check if all recent root timers  have expired */
      if (br_inst->br_inst_all_rr_timer_cnt == 0)
        {
          root_port = mstp_find_msti_port(br_inst,br_inst->recent_root);
          if (root_port == NULL)
            {
              /* Log Error */
              return RESULT_ERROR;
            }
          /* If so transition this port directly to forwarding state */ 
          mstp_msti_handle_rootport_transition (inst_port);
          br_inst->recent_root = MSTP_BR_NO_PORT;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_bridge_instance (br_inst);
#endif /* HAVE_HA */
        }
    }

  return RESULT_OK;
  
}

int
mstp_msti_rb_timer_handler (struct thread *t)
{
  struct mstp_instance_port * inst_port =  t ? 
    (struct mstp_instance_port *)t->arg : NULL ;

  if (!inst_port)
    return RESULT_ERROR;
  
#ifdef HAVE_HA
  mstp_cal_delete_mstp_inst_rb_timer (inst_port);
#endif /* HAVE_HA */

  inst_port->recent_backup_timer = NULL;

  if (LAYER2_DEBUG (proto, TIMER))
    zlog_debug (mstpm, "mstp_msti_rb_timer_handler: expired for "
                "bridge %s port %u",
                inst_port->br->bridge->name, inst_port->ifindex);

  if (inst_port->role == ROLE_ROOTPORT)
    mstp_msti_handle_rootport_transition (inst_port);
  
  return RESULT_OK;

}

/* timer handler for errdisable timeout */
int
mstp_errdisable_timer_handler (struct thread *t)
{
  struct mstp_port * port =  (struct mstp_port *)(t ? t->arg : NULL);

  if (port == NULL)
    {
      zlog_warn (mstpm, "mstp_errdisable_timer_handler: "
                 "received null port argument");
      return RESULT_ERROR;
    }

#ifdef HAVE_HA
  mstp_cal_delete_mstp_errdisable_timer (port);
#endif /* HAVE_HA */

  port->errdisable_timer = NULL;

  if (port->br == NULL)
    {
      zlog_warn (mstpm, "rstp_errdisable_timer_handler: " "received null "
                 "bridge argument for port %u", port->ifindex);
      return RESULT_ERROR;
    }

  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, " erridisable timer expired for port %u", port->ifindex);

  mstp_nsm_send_port_state (port->ifindex, MSTP_INTERFACE_UP);

  return RESULT_OK;
}

int
mstp_edge_delay_timer_handler (struct thread *t)
{
  struct mstp_port * port =  (struct mstp_port *)(t ? t->arg : NULL);

  if (port == NULL)
    {
      zlog_warn (mstpm, "mstp_errdisable_timer_handler: "
                 "received null port argument");
      return RESULT_ERROR;
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_delete_mstp_edge_delay_timer (port);
#endif /* HAVE_HA */

  port->edge_delay_timer = NULL;

  if (port->auto_edge
      && port->send_proposal
      && port->cist_role == ROLE_DESIGNATED)
    {
      port->oper_edge = PAL_TRUE;
      mstp_cist_set_port_state (port, STATE_FORWARDING);
      if (l2_is_timer_running (port->forward_delay_timer))
        {
#ifdef HAVE_HA
          mstp_cal_delete_mstp_forward_delay_timer (port);
#endif /* HAVE_HA */
          l2_stop_timer (&port->forward_delay_timer);
        }
      port->send_proposal = PAL_FALSE;
    }

  if (port->newInfoCist)
     mstp_tx (port);

  return RESULT_OK;

}

/* STP Timer Functions */

/* This function handles the expiry of the topology change timer.
   The topology change flags are cleared. Section 8.7.7 */

int
stp_topology_change_timer_handler (struct thread * t)
{
  int ret;

  struct mstp_bridge * br = (struct mstp_bridge *) (t ? t->arg : NULL);

  if (br == NULL)
    {
      zlog_warn (mstpm, "stp_topology_change_timer_handler: "
                 "received nil bridge argument");
      return RESULT_ERROR;
    }

  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, "Topology change timer expired for bridge %s",
                br->name);

  /* Section 8.7.7.1 */
  br->topology_change_detected = PAL_FALSE;
  /* Section 8.7.7.2 */
  br->topology_change = PAL_FALSE;

  /* statistics */
  br->tc_flag = PAL_FALSE;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
  mstp_cal_delete_mstp_br_tc_timer (br);
#endif /* HAVE_HA */

  br->topology_change_timer = NULL;

  /* Section 8.5.1.10 */
  ret = mstp_nsm_send_ageing_time( br->name, br->ageing_time);

  if (ret != RESULT_OK)
    zlog_warn (mstpm, "stp_topology_change_detection: "
               "Bridge %s could not set ageing time to forward delay "
               "on topology change", br->name);
  br->ageing_time_is_fwd_delay = PAL_FALSE;
  return RESULT_OK;
}


/* This function handles the expiry of the topology change notification timer.
   A topology change notification message is broadcast
   and the timer is restarted. Section 8.7.6 */

int
stp_tcn_timer_handler (struct thread * t)
{
  struct mstp_bridge * br = (struct mstp_bridge *) (t ? t->arg : NULL);

  if (br == NULL)
    {
      zlog_warn (mstpm, "stp_tcn_timer_handler: received nil bridge argument");
      return RESULT_ERROR;
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
  mstp_cal_delete_mstp_tcn_timer (br);
#endif /* HAVE_HA */

  br->tcn_timer = NULL;

  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, "TCN timer expired for bridge %s",
                br->name);

  /* Section 8.7.6.1 */
  stp_transmit_tcn_bpdu (br);

  /* Section 8.7.6.2 */
  br->tcn_timer = l2_start_timer (stp_tcn_timer_handler,
                                  br, br->bridge_hello_time, mstpm);

#ifdef HAVE_HA
  mstp_cal_create_mstp_tcn_timer (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* This function handles the expiry of the hold timer. Section 8.7.8 */

int
stp_hold_timer_handler (struct thread * t)
{
  struct mstp_port * port = (struct mstp_port *) (t ? t->arg : NULL);

  if (port == NULL)
    {
      zlog_warn (mstpm, "stp_hold_timer_handler: received nil port argument");
      return RESULT_ERROR;
    }

  if (LAYER2_DEBUG (proto, PROTO))
    zlog_debug (mstpm, "Hold timer expired for port %u",
                port->ifindex);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_delete_mstp_hold_timer (port);
#endif /* HAVE_HA */

  port->hold_timer = NULL;

  /* Section 8.7.8 */
  if (port->config_bpdu_pending)
    {
      port->newInfoCist = PAL_TRUE;
      mstp_tx (port);
    }

  return RESULT_OK;
}
