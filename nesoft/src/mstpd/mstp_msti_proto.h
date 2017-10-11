/* Copyright 2003  All Rights Reserved. 

    Multiple Spanning Tree Protocol (802.1s)

    This modules declares the interface to the STP state machines
    and associated functions specific to each instance.
  
*/

#ifndef __PACOS_MSTP_MSTI_PROTO_H__
#define __PACOS_MSTP_MSTI_PROTO_H__

#include "l2_timer.h"
#include "mstp_types.h"
#include "mstp_bridge.h"

extern void
mstp_msti_process_message (struct mstp_instance_port *inst_port, 
                        struct mstp_instance_bpdu *inst_bpdu);

extern int
mstp_msti_handle_rcvd_bpdu (struct mstp_instance_port *inst_port,
                                struct mstp_instance_bpdu *inst_bpdu);
extern int
mstp_msti_port_config_update (struct mstp_instance_port *inst_port);
extern int
mstp_msti_rcv_proposal (struct mstp_instance_port *curr_inst_port);
extern void
mstp_msti_send_proposal (struct mstp_instance_port *curr_inst_port);
extern int
mstp_msti_port_role_selection (struct mstp_bridge_instance *br_inst);
extern void
mstp_msti_propogate_topology_change (struct mstp_instance_port *this_port_inst);
extern void 
mstp_record_msti_priority_vector (struct mstp_instance_port *inst_port, 
                                    struct mstp_instance_bpdu *inst_bpdu);
extern void 
mstp_msti_handle_designatedport_transition 
            (struct mstp_instance_port * inst_port);
extern void 
mstp_msti_handle_rootport_transition (struct mstp_instance_port *inst_port);

extern void
mstp_msti_set_port_role_all (struct mstp_port *port, enum port_role role);

static inline s_int32_t
mstp_msti_get_tc_time (struct mstp_instance_port *inst_port)
{
  struct mstp_port *port = inst_port->cst_port;
 
  if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS(port->br))
    return (port->send_mstp ? (port->cist_hello_time + L2_TIMER_SCALE_FACT):
                              (port->br->cist_max_age +
                             port->br->cist_forward_delay));
  else
    return  (port->send_mstp ? (port->br->cist_hello_time + L2_TIMER_SCALE_FACT):
                               (port->br->cist_max_age +
                                port->br->cist_forward_delay));
}

static inline s_int32_t
mstp_msti_get_fdwhile_time (struct mstp_instance_port *inst_port)
{
  struct mstp_port *port = inst_port->cst_port;

  if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS(port->br))
    return port->send_mstp ? port->cist_hello_time:
                             port->br->cist_forward_delay;
  else
    return port->send_mstp ? port->br->cist_hello_time :
                             port->br->cist_forward_delay;

}

/* Timer functions for msti  */
extern int mstp_msti_forward_delay_timer_handler (struct thread *);
extern int mstp_msti_tc_timer_handler (struct thread *);
extern int mstp_msti_message_age_timer_handler (struct thread *);
extern int mstp_msti_rr_timer_handler (struct thread *);
extern int mstp_msti_rb_timer_handler (struct thread *);

#endif
