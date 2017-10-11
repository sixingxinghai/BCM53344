/* Copyright 2003  All Rights Reserved. 

Multiple Spanning Tree Protocol (802.1Q)

This modules declares the interface to the STP state machines
and associated functions.
  
*/

#ifndef __PACOS_MSTP_CIST_PROTO_H__
#define __PACOS_MSTP_CIST_PROTO_H__

#include "l2_timer.h"
#include "mstp_bridge.h"

extern int
mstp_handle_rcvd_bpdu (struct mstp_port *port,
                       struct mstp_bpdu *bpdu);
extern int
mstp_cist_port_config_update (struct mstp_port *port);
extern int
mstp_cist_rcv_proposal (struct mstp_port *curr_port);
extern void
mstp_cist_send_proposal (struct mstp_port *curr_port);
extern int
mstp_cist_port_role_selection (struct mstp_bridge *br);
extern void
mstp_cist_propogate_topology_change (struct mstp_port *this_port);
extern void 
mstp_cist_record_priority_vector (struct mstp_port *port, 
                                  struct mstp_bpdu *bpdu);
extern void 
mstp_cist_handle_designatedport_transition (struct mstp_port *port);
extern void 
mstp_cist_handle_rootport_transition (struct mstp_port *port);

extern void
mstp_cist_process_message (struct mstp_port *port, 
                           struct mstp_bpdu *bpdu);

extern void
stp_topology_change_detection (struct mstp_bridge * br);

static inline s_int32_t
mstp_cist_get_tc_time (struct mstp_port *port)
{

  if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS (port->br))
    return (port->send_mstp ? (port->cist_hello_time + L2_TIMER_SCALE_FACT):
                             (port->br->cist_max_age + 
                              port->br->cist_forward_delay));
  else
    return (port->send_mstp ? (port->br->cist_hello_time + L2_TIMER_SCALE_FACT):
                             (port->br->cist_max_age + 
                              port->br->cist_forward_delay));
}

static inline s_int32_t
mstp_cist_get_edge_delay_time (struct mstp_port *port)
{
  return port->oper_p2p_mac ? MSTP_MIGRATE_TIME : port->br->cist_max_age;
}

static inline s_int32_t
mstp_cist_get_fdwhile_time (struct mstp_port *port)
{

  if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS(port->br))
    return port->send_mstp ? port->cist_hello_time:
                             port->br->cist_forward_delay;
  else
    return port->send_mstp ? port->br->cist_hello_time:
                             port->br->cist_forward_delay;

}

/* Timer functions for cist */
extern int mstp_forward_delay_timer_handler (struct thread *);
extern int mstp_tc_timer_handler (struct thread *);
extern int mstp_message_age_timer_handler (struct thread *);
extern int mstp_rr_timer_handler (struct thread *);
extern int mstp_rb_timer_handler (struct thread *);

extern int mstp_port_timer_handler (struct thread *);
extern int mstp_hello_timer_handler (struct thread *);
extern int mstp_migrate_timer_handler (struct thread *);
extern int mstp_errdisable_timer_handler (struct thread *t);
extern int mstp_edge_delay_timer_handler (struct thread *t);

/* STP Timers */

extern int stp_tcn_timer_handler (struct thread *t);
extern int stp_topology_change_timer_handler (struct thread *t);
extern int stp_hold_timer_handler (struct thread * t);
  
#endif
