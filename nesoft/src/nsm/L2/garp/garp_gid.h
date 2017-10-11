/* Copyright 2003  All Rights Reserved. */

#ifndef _PACOS_GARP_GID_H
#define _PACOS_GARP_GID_H

#include "pal.h"
#include "table.h"
#include "ptree.h"
struct prefix_attribute
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  s_int32_t attribute_index;
};

#define PREFIX_ATTRIBUTE_SET(P,I)                                            \
   do {                                                                      \
       pal_mem_set ((P), 0, sizeof (struct prefix_attribute));               \
       (P)->family = AF_INET;                                                \
       (P)->prefixlen = 32;                                                  \
       (P)->attribute_index = pal_hton32 (I);                                \
   } while (0)

#define PREFIX_ATTRIBUTE_INFO_SET(R,V)                                       \
   do {                                                                      \
      (R)->info = (V);                                                       \
      ptree_lock_node (R);                                                   \
   } while (0)

#define PREFIX_ATTRIBUTE_INFO_UNSET(R)                                       \
   do {                                                                      \
     (R)->info = NULL;                                                       \
     ptree_unlock_node(R);                                                   \
   } while (0)

#define NUM_GID_RCV_EVENTS        6
#define NUM_GID_TX_EVENTS         7
#define NUM_GID_REQ_EVENTS        3
#define NUM_GID_APP_MGMT_EVENTS   2
#define NUM_GID_REG_MGMT_EVENTS   3

enum reg_proto_type
{
  REG_PROTO_GARP,
  REG_PROTO_MRP,
  REG_PROTO_MAX,
};

/* Timers */
enum garp_timers
{
  GARP_JOIN_TIMER          = 0,
  GARP_LEAVE_TIMER         = 1,
  GARP_LEAVE_ALL_TIMER     = 2,
  GARP_LEAVE_CONF_TIMER    = 3,
  GARP_LEAVEALL_CONF_TIMER = 4,
  GARP_MAX_TIMERS      = 5 
};

/* Note all times below are in centisecs */
#define GID_DEFAULT_JOIN_TIME       20
#define GID_DEFAULT_LEAVE_TIME      60
#define GID_DEFAULT_HOLD_TIME       10
#define GID_DEFAULT_LEAVE_ALL_TIME  1000

#define GID_MAX_JOIN_TIME_DEVIATION 5

#define GID_LEAVE_ALL_COUNT         4

#define GID_UNGET_GID_MACHINE       1
#define GID_INDEX_OFFSET            1

/* Enumeration for applicant states */
enum gid_applicant_state
{
  GID_APP_STATE_VERY_ANXIOUS,
  GID_APP_STATE_ANXIOUS,
  GID_APP_STATE_QUIET,
  GID_APP_STATE_LEAVING,
  GID_APP_STATE_CHANGE_MEMBER,
  GID_APP_STATE_MAX
};

/* Enumeration for application's management control */
enum gid_applicant_mgmt
{
  GID_APP_MGMT_NORMAL,
  GID_APP_MGMT_NO_PROTOCOL,
  GID_APP_MGMT_MAX
};

/* Enumeration for registrar states */
enum gid_registrar_state
{
  GID_REG_STATE_IN,
  GID_REG_STATE_LEAVE,
  GID_REG_STATE_EMPTY,
  GID_REG_STATE_MAX
};

/* Enumeration for registrar's management control */
enum gid_registrar_mgmt 
{
  GID_REG_MGMT_NORMAL,
  GID_REG_MGMT_FIXED,
  GID_REG_MGMT_FORBIDDEN,
  GID_REG_MGMT_RESTRICTED_GROUP,
  GID_REG_MGMT_MAX
};

/* Enumeration for the GID's events 
   Note: DO NOT CHANGE THE ORDER. Its got something tied upto the transition
   tables for registrar and applicant */
typedef enum gid_event
{
  GID_EVENT_NULL                           = 0,
  GID_EVENT_RCV_LEAVE_EMPTY                = 1,
  GID_EVENT_RCV_LEAVE_IN                   = 2,
  GID_EVENT_RCV_EMPTY                      = 3,
  GID_EVENT_RCV_JOIN_EMPTY                 = 4,
  GID_EVENT_RCV_JOIN_IN                    = 5,
  GID_EVENT_JOIN                           = 6,
  GID_EVENT_LEAVE                          = 7,
  GID_EVENT_NORMAL_REGISTRATION            = 8,
  GID_EVENT_FIXED_REGISTRATION             = 9,
  GID_EVENT_FORBID_REGISTRATION            = 10,
  GID_EVENT_NEW                            = 11,
  GID_EVENT_RCV_LEAVE_ALL                  = 12,
  GID_EVENT_RCV_LEAVE_MINE                 = 13,
  GID_EVENT_RCV_LEAVE                      = 14,
  GID_EVENT_RCV_JOIN                       = 15,
  GID_EVENT_RCV_NEW                        = 16,
  GID_EVENT_RCV_NEW_EMPTY                  = 17,
  GID_EVENT_RCV_NEW_IN                     = 18,
  GID_EVENT_TX_LEAVE_EMPTY                 = 19,
  GID_EVENT_TX_LEAVE_IN                    = 20,
  GID_EVENT_TX_EMPTY                       = 21,
  GID_EVENT_TX_JOIN_IN                     = 22,
  GID_EVENT_TX_JOIN_IN_OPT                 = 23,
  GID_EVENT_TX_JOIN_EMPTY                  = 24,
  GID_EVENT_TX_JOIN_EMPTY_OPT              = 25,
  GID_EVENT_TX_LEAVE_ALL                   = 26,
  GID_EVENT_TX_LEAVE_ALL_FULL              = 27,
  GID_EVENT_TX_LEAVE_MINE                  = 28,
  GID_EVENT_TX_LEAVE_MINE_FULL             = 29,
  GID_EVENT_TX_NEW_IN                      = 30,
  GID_EVENT_TX_NEW_EMPTY                   = 31,
  GID_EVENT_TX_LEAVE                       = 32,
  GID_EVENT_TX_LEAVE_OPT                   = 33,
  GID_EVENT_TX_NULL_OPT                    = 34,
  GID_EVENT_TX_FULL                        = 35,
  GID_EVENT_RESTRICTED_VLAN_REGISTRATION   = 36,
  GID_EVENT_RESTRICTED_GROUP_REGISTRATION  = 37,
  GID_EVENT_MAX                            = 38
}gid_event_t;

#if defined HAVE_MMRP || defined HAVE_MVRP

typedef enum mrp_app_event
{
  MRP_APP_EVENT_NULL               = 0,
  MRP_APP_EVENT_RCV_LEAVE          = 1,
  MRP_APP_EVENT_RCV_EMPTY          = 2,
  MRP_APP_EVENT_RCV_JOIN_NEW_MT    = 3,
  MRP_APP_EVENT_RCV_JOIN_NEW_IN    = 4,
  MRP_APP_EVENT_JOIN               = 5,
  MRP_APP_EVENT_LEAVE              = 6,
  MRP_APP_EVENT_NEW                = 7,
  MRP_APP_EVENT_RCV_LEAVE_ALL      = 8,
  MRP_APP_EVENT_MAX                = 9
}mrp_app_event_t;

typedef enum mrp_reg_event
{
  MRP_REG_EVENT_NULL               = 0,
  MRP_REG_EVENT_RCV_NEW            = 1,
  MRP_REG_EVENT_RCV_JOIN           = 2,
  MRP_REG_EVENT_RCV_LEAVE_P2P      = 3,
  MRP_REG_EVENT_RCV_LEAVE_NOT_P2P  = 4,
  MRP_REG_EVENT_LEAVE_ALL          = 5,
  MRP_REG_EVENT_MAX                = 6
}mrp_reg_event_t;

typedef enum mrp_tx_event
{
  MRP_TX_EVENT_NULL               = 0,
  MRP_TX_EVENT_P2P                = 1,
  MRP_TX_EVENT_NOT_P2P            = 2,
  MRP_TX_EVENT_LEAVE_MINE         = 3,
  MRP_TX_EVENT_LEAVE_ALL          = 4,
  MRP_TX_EVENT_MAX                = 5
}mrp_tx_event_t;

#endif /* HAVE_MMRP || HAVE_MVRP */

extern const char* const gid_event_string [];

/* struct for gid machine per port/per attribute(gid_index) */
struct gid_machine {

  /* Application states associated with the attribute */
  unsigned char applicant:5;

  /* Registrar states associated with the attribute */
  unsigned char registrar:5; 

};


/* struct for gid_states including the management state */
struct gid_states {
  /* bitmap to hold gid_applicant_state */
  unsigned char applicant_state:2;

  /* bitmap to hold gid_applicant_mgmt */
  unsigned char applicant_mgmt:2;

  /* bitmap to hold gid_registrar_state */
  unsigned char registrar_state:2;
  
  /* bitmap to hold gid_registrar_mgmt */
  unsigned char registrar_mgmt:2;
};


/* struct for the gid instance one per port/per application */
struct gid {
  /* pointer to the garp */
  struct garp_instance *application;

  struct gid_port *gid_port;
  
  /* Comment Me */
  unsigned char cschedule_tx_now:1;

  /* flag to specify to start the applicant's join timer */
  unsigned char cstart_join_timer:2;

  /* flag to specify to start registrar's leave timer */
  unsigned char cstart_leave_timer:2;

  /* Comment Me */
  unsigned char tx_now_scheduled:1;

  /* flag to specify the applicant's join timer is running */
  unsigned char join_timer_running:1;

  /* flag to specify the registrar's leave timer is running */
  unsigned char leave_timer_running:1;
  
  /* flag to specify that the hold timer is running and all scheduling and
     starting timers are held until the hold timer expires */
  unsigned char hold_tx:1;

  /* flag to specify if there are any messaged pending to be transmitted */
  unsigned char tx_pending:1;

  /* Comment Me */
  unsigned char leave_all_countdown;

  /* pointer to the gid_machines that keeps track of the fsm's associated with
     each attribute that are associated with garp application/port */
  struct ptree *gid_machine_table;

  /* attribute index last transmitted */
  u_int32_t last_transmitted;

  /* attribute index last to transmit */
  u_int32_t next_to_transmit;

  /* Comment Me */
  struct thread *join_timer_thread;

  /* Comment Me */
  struct thread *hold_timer_thread;

  /* Comment Me */
  struct thread *leave_timer_thread;

  /* Comment Me */
  struct thread *leave_all_timer_thread;
 
};

struct gid_port {

  /* port associated with gid instance */
  int ifindex;

  /* join timer value */
  pal_time_t join_timeout;

  /* leave timer value  */
  pal_time_t leave_timeout_4;

  /* Configured leave timer value */
  pal_time_t leave_conf_timeout;

  /* leaveall timer value */
  pal_time_t leave_all_timeout_n;

  /* configured leaveall timer value */
  pal_time_t leave_all_conf_timeout;

  /* Comment Me */
  int hold_timeout;

  u_int8_t p2p;

};

/* Function declarations */

/*
    FUNCTION : gid_create_gid

    DESCRIPTION :
    A new gid instance is created for the port specified. The instance is added 
    to the gid instances in next_in_port_ring. The garp application is 
    signalled about the new gid instance associated with the port being created.
    The gid instance is not yet added to next_in_connected_ring

    IN :
    struct garp *garp: garp pointer
    int ifindex: port on which the gid instance needs to be created
    struct gid *gid: gid pointer that should be given to tha application 
                     after cretion

    OUT :
    PAL_TRUE  : when the gid_instance is created and added to next_in_port_ring
    PAL_FALSE : when the allocation of the gid_instance fails

    CALLED BY:

 */
extern bool_t
gid_create_gid (struct gid_port *gid_port, struct garp_instance *garp_instance, 
                struct gid **gid, u_int32_t init_machines);


/*
    FUNCTION : gid_destroy_gid

    DESCRIPTION :
    The gid instance associated with the ifindex is searched in the 
    next_in_port_ring. The gid_instance if found is then disconnected from 
    the next_in_port_ring. Gip is informed to disconnect the gid_instance from 
    the next_in_connected_ring and to propagate a leave for all the gid_indices
    that were registered on this ifindex

    IN :
    int ifindex: port on which the gid instance needs to be destroyed 

    OUT :
    None

    CALLED BY : 
    gmr_destroy_gmr

 */
extern void
gid_destroy_gid (struct gid *old_gid);

/*
    FUNCTION : gid_get_attribute_states

    DESCRIPTION : 
    Reads the applicant and registrar's states for the given gid_instance, 
    gid_index

    IN :
    struct gid *gid : get the gid_states for given the gid_instance
    unsigned int attribute_index : get the gid_states for the given gid_index 
    struct gid_states *state : gid_states that are read for the given gid and 
                               gid_index

    OUT :
    None

    CALLED BY :
 */
extern bool_t
gid_get_attribute_states (struct gid *gid, unsigned int attribute_index,
                         struct gid_states *state);

/*
    FUNCTION : gid_set_attribute_states

    DESCRIPTION : 
    Changes the management state of the given gid_index associated with the
    gid_instance. The event can be GID_APP_MGMT_NORMAL, 
    GID_APP_MGMT_NO_PROTOCOL, GID_REG_MGMT_NORMAL, GID_REG_MGMT_FIXED, 
    GID_REG_MGMT_FORBID. If the event causes a transition that needs to be 
    propagated either join or leave, it gets propagated through GIP
     
    IN :
    struct gid *gid : set the gid_state for the given gid_instance 
    unsigned int attribute_index : set the gid_state for the given gid_index
    enum gid_event event : event that needs to be set for the given 
                           gid_instance and gid_index

    OUT :
    None

    CALLED BY :
 */
extern void
gid_set_attribute_states (struct gid *gid, unsigned int attribute_index,
                          enum gid_event event);


/*
    FUNCTION : gid_rcv_attribute 

    DESCRIPTION : 
    Changes the fsm state associated with the given gid_instance, received 
    gid_index and received event. Change in registrar state associated with the
    gid_index to either GID_EVENT_JOIN or GID_EVENT_LEAVE will cause an 
    upcall to the garp application (GVRP, GMRP) with the join or leave 
    indication and also propagating through GIP a join or leave. This routine 
    should strictly be called for events GID_EVENT_RCV_LEAVE, 
    GID_EVENT_RCV_EMPTY, GID_EVENT_RCV_JOIN_EMPTY, GID_EVENT_RCV_JOIN_IN only

    IN :
    struct gid *gid : gid_instance for which the message was received
    unsigned int gid_index : gid_index for which the message was received
    enum gid_event event : event received for the given gid_instance, gid_index

    OUT :
    None

    CALLED BY :
    gmr_rcv_msg
 */
extern void
gid_rcv_attribute (struct gid *gid, unsigned int gid_index, 
                   enum gid_event event);


/*
    FUNCTION : gid_join_event_request

    DESCRIPTION : 
    Changes the fsm state of the given gid_instance, gid_index to a new state
    for a GID_EVENT_JOIN event

    IN :
    struct gid *gid : gid_instance for which GID_EVENT_JOIN event must be 
                      processed
    unsigned int gid_index : gid_index for the fsm state transition should occur
    enum gid_event join_event: NEW or JOIN

    OUT :
    None

    CALLED BY :
    gip_connect_port, gip_propogate_join
 */
extern void
gid_join_event_request (struct gid *gid, unsigned int gid_index,
                        enum gid_event new_event);


/*
    FUNCTION : gid_leave_request 

    DESCRIPTION :
    Changes the fsm state of the given gid_instance, gid_index to a new state
    for a GID_EVENT_LEAVE

    IN :
    struct gid *gid : gid_instance for which GID_EVENT_LEAVE event must be 
                      processed
    unsigned int gid_index : gid_index for the fsm state transition should occur

    OUT :
    None

    CALLED BY :
 */
extern void
gid_leave_request (struct gid *gid, unsigned int gid_index);

/*
    FUNCTION : gid_leave_all

    DESCRIPTION :
    Changes the fsm state of the given gid_instance, gid_index to a new state
    for a GID_EVENT_LEAVE

    IN :
    struct gid *gid : gid_instance for which GID_EVENT_LEAVE event must be 
                      processed
    unsigned int starting_gid_index :
    unsigned int ending_gid_index :

    OUT :
    None

    CALLED BY :
    gid_rcv_leave_all, gid_leave_all_timer_handler 
 */
extern void
gid_leave_all (struct gid *gid,u_int32_t starting_gid_index,
              u_int32_t ending_gid_index);

/*
    FUNCTION : gid_rcv_leave_all 

    DESCRIPTION : 
    Resets the levaeall countdown associated with the gid_instance and calls 
    static gid_leave_all () to process this event

    IN :
    struct gid *gid : gid_instance for which the GID_EVENT_RCV_LEAVE_ALL event 
                      must be processed
                      
    unsigned int starting_gid_index :
    unsigned int ending_gid_index :

    OUT :
    None

    CALLED BY :
    gmr_rcv_msg 
 */
extern void
gid_rcv_leave_all (struct gid *gid, u_int32_t starting_gid_index,
                   u_int32_t ending_gid_index);

/*
    FUNCTION : gid_registered_here

    DESCRIPTION : 
    Checks if the Registrar state is not GID_REG_EMPTY or GID_REG_FIXED

    IN :
    struct gid *gid : gid_instance for which the registrar fsm is check
    unsigned int gid_index : gid_index for which the registrar fsm is checked

    OUT :
    PAL_TRUE : when the registrar state is not in GID_STATE_EMPTY or 
               GID_REG_FIXED
    PAL_FALSE : when the registrar state is GID_STATE_EMPTY and not 
                GID_REG_FIXED

    CALLED BY :
 */
extern bool_t
gid_registered_here (struct gid *gid, unsigned int gid_index);


/*
    FUNCTION : gid_do_actions

    DESCRIPTION : 
    Actions that will be done when the following happens
    * a msg is received on this gid_instance's port 
    * one of the ports in next_connected_in_ring received a msg
    * hold timer handler for the gid_instance
    When one of the above happens "scratchpad" actions that were accumulated, 
    any outstanding immediate transmissions and join timers that was delayed 
    due to hold timer will all be restarted

    IN :
    struct gid *gid : gid_instance for which some actions needs to be done

    OUT :
    None

    CALLED BY :
    gip_do_actions, gid_hold_timer_handler
 */
extern void
gid_do_actions (struct gid *gid);

/*
    FUNCTION : gid_tx_attribute

    DESCRIPTION :
    Scan the GID machines for messsages that require transmission.
    If there is no message to transmit return Gid_null; otherwise,
    return the message as a Gid_event.
    If message transmission is currently held [pending expiry of a hold
    timer and a call to gid_release_tx()], this function will return Gid_null
    so it may be safely called if that is convenient.
    Supports sets of GID machines that can generate more messages than
    can fit in a single application PDU. This allows this implementation
    to support, for example, GARP registration for all 4096 VLAN identifiers.
    To support the application's packing of messages into a single PDU
    without the detailed knowledge of frame format and message encodings
    required to tell whether a message will fit, and to avoid the application
    having to make two calls to GID for every message - one to get the
    message and another to say it has been taken - this routine supports the

    Check to see if a leaveall should be sent; if so, return Gid_tx_leaveall;
    otherwise, scan the GID machines for messsages that require transmission.
    Machines will be checked for potential transmission starting with the
    machine following last_transmitted and up to and including 
    next_to_transmit.
    If all machines have transmitted, last_transmitted equals next_to_transmit
    and tx_pending is False (in this case tx_pending distinguished the
    case of `all machines are yet to be checked for transmission' from 
    `all have been checked.')
    If tx_pending is True and all machines are yet to be checked, transmission
    will start from the machine with GID index 0, rather than from immediately
    following last_transmitted.

    IN :
    struct gid *gid : gid_instance for which the 
    unsigned int start_index : 
    unsigned int *gid_index : 

    OUT :
    GID_EVENT_NULL :
    GID_EVENT_TX_LEAVE_ALL :
    GID_EVENT_JOIN_IN_EMPTY :
    GID_EVENT_JOIN_IN :
    GID_EVENT_TX_EMPTY :
    GID_EVENT_LEAVE_IN :
    GID_EVENT_LEAVE_IN_EMPTY :

    CALLED BY :
    gmr_tx, 
 */
extern enum gid_event
gid_tx_attribute (struct gid *gid, unsigned int start_index, 
                  u_int32_t *gid_index);


/*
    FUNCTION : gid_untx_attribute

    DESCRIPTION :

    IN :
    struct gid *gid

    OUT :
    None

    CALLED BY :
 */
extern void
gid_untx_attribute (struct gid *gid);


/*
    FUNCTION : gid_set_timer

    DESCRIPTION :

    IN :
    struct gid *gid
    u_int32_t timer_type
    pal_time_t timer_value

    OUT :
    None

    CALLED BY :
 */
extern void
gid_set_timer (struct gid_port *gid_port, u_int32_t timer_type, 
               pal_time_t timer_value);


/*
    FUNCTION : gid_get_timer

    DESCRIPTION :

    IN :
    struct gid *gid
    u_int32_t timer_type
    pal_time_t *timer_value

    OUT :
    None

    CALLED BY :
*/
extern void
gid_get_timer (struct gid_port *gid_port, u_int32_t timer_type, 
               pal_time_t *timer_value);


/*
    FUNCTION : gid_get_timer_details

    DESCRIPTION :

    IN :
    struct gid *gid
    pal_time_t* timer_value

    OUT :
    None

    CALLED BY :
 */
extern void
gid_get_timer_details (struct gid_port *gid_port, pal_time_t *timer_details);


/*
   * Finds the GID instance for port number port_no.
   */
extern bool_t
gid_find_port(struct gid *first_port, int port_no, struct gid **gid);

/*
   * Finds the next port in the ring of ports for this application.
   */
extern struct gid 
*gid_next_port(struct gid *this_port);


extern void
gid_manage_attribute (struct gid *gid, unsigned int gid_index, 
                   enum gid_event directive);
extern void
gid_set_propagate_port_state_event (struct gid *gid,
                                    unsigned char port_state);
extern bool_t
gid_create_gid_port (int ifindex, struct gid_port **gid_port);

extern bool_t
gid_registrar_empty_here (struct gid *gid, unsigned int gid_index);

void
gid_register_gid (struct gid *gid, u_int32_t gid_index);

void
gid_unregister_gid (struct gid *gid, u_int32_t gid_index);

s_int32_t
gid_get_attr_index (struct ptree_node *rn);

bool_t
gid_delete_attribute (struct gid *gid, u_int32_t attribute_index);

void
gip_delete_attribute (struct gid *gid, u_int32_t  attribute_index);

bool_t
gid_attr_entry_to_be_created (struct gid *gid, u_int32_t attribute_index);

bool_t
gid_app_object_is_to_be_deleted (struct gid *gid, u_int32_t attribute_index);

#if defined HAVE_MVRP || defined HAVE_MMRP
extern enum gid_event
mad_tx_attribute (struct gid *gid, unsigned int start_index, 
                  unsigned int *gid_index, enum mrp_tx_event tx_event);

void
mad_get_tx_leave_all_event (struct gid *gid, u_int8_t *tx_event,
                            u_int8_t *leave_all_event);

#endif /* HAVE_MVRP || HAVE_MMRP */
#endif /* _PACOS_GARP_GID_H */
