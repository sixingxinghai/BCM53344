/* Copyright 2003  All Rights Reserved. 

Multiple Spanning Tree Protocol

This modules declares the interface to the Port related functions.

*/

#ifndef __PACOS_MSTP_PORT_H__
#define __PACOS_MSTP_PORT_H__

/*
  extern int mstp_rr_timer_handler (struct thread * t);
  extern int mstp_rb_timer_handler (struct thread * t);
  extern int mstp_tc_timer_handler (struct thread * t);
  extern int mstp_forward_delay_timer_handler (struct thread * t);
*/

#define MSTP_INTERFACE_SHUTDOWN  1
#define MSTP_INTERFACE_UP        2
#define STP_HOLD_TIME            1

/* This function returns non-zero if the specified port is the designated port.
   Otherwise, it returns 0. */
static inline int 
mstp_cist_is_designated_port (struct mstp_port *port)
{
  return (pal_mem_cmp (&port->cist_designated_bridge, 
                       &port->br->cist_bridge_id, MSTP_BRIDGE_ID_LEN) == 0)
    && (port->cist_designated_port_id == port->cist_port_id);
}

extern void 
mstp_cist_init_port (struct mstp_port *port);

extern void 
mstp_cist_enable_port (void *port);

extern void 
mstp_cist_disable_port (void *port);

extern void 
mstp_cist_set_port_priority (struct mstp_port *port,
                             int newprio);

extern void 
mstp_cist_set_port_path_cost (void *port, u_int32_t path_cost);

extern void
mstp_set_port_pathcost_configured (struct interface *ifp, int flag);

extern void 
mstp_cist_set_change_detection_enabled (struct mstp_port *port,
                                        int change_detection_flag);

u_int32_t
mstp_map_port2msg_state(int state);

u_int16_t
mstp_map_port2msg_next_state(int state);
  
int
mstp_bridge_port_state_send (char * name, const u_int32_t ifindex);

/* This function should be called only from disable_bridge when called
   with bridge_forward set to true */
extern int
mstp_cist_set_port_state_forward (struct mstp_port *port);

extern int
mstp_cist_set_port_state (struct mstp_port *port, enum port_state state);

extern void
mstp_cist_set_port_role (struct mstp_port *port, enum port_role role);

static inline int 
mstp_cist_is_root_port (struct mstp_port *port)
{
  return (pal_mem_cmp (&port->cist_root, &port->br->cist_bridge_id, 
                       MSTP_BRIDGE_ID_LEN) == 0)
    && (port->cist_designated_port_id == port->cist_port_id);
}

extern inline u_int16_t
mstp_cist_make_port_id (struct mstp_port * port);
/*{
  return (u_int16_t) ((port->ifindex & 0x0fff) | 
  ((port->cist_priority << 8) & 0xf000));
  }*/

extern void 
mstp_msti_init_port (struct mstp_instance_port *inst_port);

extern void 
mstp_msti_enable_port (void *port);

extern void 
mstp_msti_disable_port (void *port);

extern void 
mstp_msti_set_port_priority (struct mstp_instance_port *inst_port,
                             int newprio);

extern void 
mstp_msti_set_port_path_cost (void *port, u_int32_t path_cost);

extern void 
mstp_msti_set_change_detection_enabled (struct mstp_instance_port *inst_port,
                                        int change_detection_flag);


extern int
mstp_msti_set_port_state (struct mstp_instance_port *inst_port, enum port_state state);

extern void
mstp_msti_set_port_role (struct mstp_instance_port *inst_port, enum port_role role);


static inline int 
mstp_msti_is_root_port (struct mstp_instance_port *inst_port)
{
  return (pal_mem_cmp (&inst_port->msti_root, &inst_port->br->msti_bridge_id, 
                       MSTP_BRIDGE_ID_LEN) == 0)
    && (inst_port->designated_port_id == inst_port->msti_port_id);
}

/* This function returns non-zero if the specified port is the designated port.
   Otherwise, it returns 0. */
static inline int 
mstp_msti_is_designated_port (struct mstp_instance_port *inst_port)
{
  return (pal_mem_cmp (&inst_port->designated_bridge, 
                       &inst_port->br->msti_bridge_id, MSTP_BRIDGE_ID_LEN) == 0)
    && (inst_port->designated_port_id == inst_port->msti_port_id);
}

/* Make a port id from the priority and the port id. 
 * 802.1t Section 9.2.7 */
static inline short 
mstp_msti_make_port_id (struct mstp_instance_port * inst_port)
{
  return (u_int16_t) ((inst_port->ifindex & 0x0fff) | 
                      ((inst_port->msti_priority << 8) & 0xf000));
}

int mstp_is_port_up (const u_int32_t ifindex);

void mstp_set_cist_port_path_cost (void *the_port);

struct mstp_interface *
mstp_interface_new (struct interface *ifp);

int
mstp_interface_add (struct interface *ifp);

void
mstp_activate_interface (struct mstp_interface *mstpif);

void
mstp_set_port_pathcost_configured (struct interface *ifp, int flag);

extern void
mstpif_free_instance_list (struct mstp_interface *mstpif);

int
mstp_cist_set_ring_port_state (struct mstp_port *port);

int
mstp_cist_ring_handle_topology_change (struct mstp_port *port);

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
int 
mstp_msti_set_port_state_forward (struct mstp_instance_port *port);
#endif  /*(HAVE_PROVIDER_BRIDGE) ||(HAVE_B_BEB)*/
#endif /* ifndef PACOS_MSTP_PORT_H */
