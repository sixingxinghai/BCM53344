/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GID_FSM_H
#define _PACOS_GID_FSM_H

#include "pal.h"

/*
    FUNCTION : gid_fsm_change_states

    DESCRIPTION :

    IN :
    struct gid *gid :
    struct gid_machine *machine :
    enum gid_event event :
    
    OUT :
    PAL_TRUE :
    PAL_FALSE :

    CALLED BY :
*/
extern enum gid_event
gid_fsm_change_states (struct gid *gid, struct gid_machine *machine, 
                       enum gid_event event);


/* Enumeration for GID's Applicant State Machine */
enum applicant_states
{
  APP_STATE_VA  = 0,  /* Very Anxious Active */
  APP_STATE_AA  = 1,  /* Anxious Active */
  APP_STATE_QA  = 2,  /* Quiet Active */
  APP_STATE_LA  = 3,  /* Leaving Active */
  APP_STATE_VP  = 4,  /* Very Anxious Passive */
  APP_STATE_AP  = 5,  /* Anxious Passive */
  APP_STATE_QP  = 6,  /* Quiet Passive */
  APP_STATE_VO  = 7,  /* Very Anxious Observer */
  APP_STATE_AO  = 8,  /* Anxious Observer */
  APP_STATE_QO  = 9,  /* Quiet Observer */
  APP_STATE_LO  = 10, /* Leaving Observer */
  APP_STATE_VC  = 11, /* Very Anxious Change Member */
  APP_STATE_AC  = 12, /* Anxious Change Member */
  APP_STATE_MAX = 13
};

/* Enumeration for GID's Registrar State Machine */
enum registrar_states
{
  REG_STATE_INN  = 0,  /* In Normal Registration */
  REG_STATE_LV   = 1,  /* Leave Normal Registration */
  REG_STATE_L3   = 2,  /* Leave countdown3 Normal Registration */
  REG_STATE_L2   = 3,  /* Leave countdown2 Normal Registration */
  REG_STATE_L1   = 4,  /* Leave countdown1 Normal Registration */
  REG_STATE_MT   = 5,  /* Empty Normal Registration */
  REG_STATE_INR  = 6,  /* In Fixed Registration */
  REG_STATE_LVR  = 7,  /* Leave Fixed Registration */
  REG_STATE_L3R  = 8,  /* Leave countdown3 Fixed Registration */
  REG_STATE_L2R  = 9,  /* Leave countdown2 Fixed Registration */
  REG_STATE_L1R  = 10, /* Leave countdown1 Normal Registration */
  REG_STATE_MTR  = 11, /* Empty Fixed Registration */
  REG_STATE_INF  = 12, /* In Forbidden Registration */
  REG_STATE_LVF  = 13, /* Leave Forbidden Registration */
  REG_STATE_L3F  = 14, /* Leave countdown3 Forbidden Registarion */
  REG_STATE_L2F  = 15, /* Leave countdown2 Forbidden Registarion */
  REG_STATE_L1F  = 16, /* Leave countdown1 Forbidden Registarion */
  REG_STATE_MTF  = 17, /* Empty Forbidden Registration */
  REG_STATE_MAX  = 18 
};

#if defined HAVE_MVRP || defined HAVE_MMRP

enum leave_all_states
{
  LEAVEALL_STATE_ACTIVE    = 0,
  LEAVEALL_STATE_PASSIVE   = 1,
  LEAVEALL_STATE_MAX       = 2 
};

enum leave_all_events
{
  LEAVEALL_EVENT_TX_P2P           = 0,
  LEAVEALL_EVENT_TX_NOT_P2P       = 1,
  LEAVEALL_EVENT_RECV_LEAVE_ALL   = 2,
  LEAVEALL_EVENT_RECV_LEAVE_MINE  = 3,
  LEAVEALL_EVENT_TIMER_EXPIRY     = 4,
  LEAVEALL_EVENT_MAX              = 5
};

#endif /* HAVE_MVRP || HAVE_MMRP */

#define NUM_GID_REG_STATES REG_STATE_MAX
#define NUM_GID_APP_STATES APP_STATE_MAX

extern char *applicant_states_string[NUM_GID_APP_STATES+1];
extern char *registrar_states_string[NUM_GID_REG_STATES+1];

/*
    FUNCTION : gid_fsm_change_tx_states 
    DESCRIPTION :

    IN :
    struct gid *gid :
    struct gid_machine *machine :
    
    OUT :
    PAL_TRUE :
    PAL_FALSE :

    CALLED BY :
*/
extern enum gid_event
gid_fsm_change_tx_states (struct gid *gid, struct gid_machine *machine);


/*
    FUNCTION : gid_fsm_leave_timer_expiry 

    DESCRIPTION :

    IN :
    struct gid *gid
    struct gid_machine *machine

    OUT :

    CALLED BY :
*/
extern enum gid_event
gid_fsm_leave_timer_expiry (struct gid *gid, struct gid_machine *machine);


/*
    FUNCTION : gid_fsm_is_machine_active 

    DESCRIPTION :

    IN :
    struct gid_machine *machine :
    
    OUT :
    PAL_TRUE :
    PAL_FALSE :

    CALLED BY :
*/
extern bool_t
gid_fsm_is_machine_active (struct gid_machine *machine);

/*
    FUNCTION : gid_fsm_is_empty_attribute

    DESCRIPTION : Determines whether a gid machine is inactive.

    IN :
    struct gid_machine *machine :

    OUT :
    PAL_TRUE :
    PAL_FALSE :

    CALLED BY : gid_find_unused_attribute_index
*/


extern bool_t
gid_fsm_is_empty_attribute (struct gid_machine *machine);


/*
    FUNCTION : gid_fsm_is_machine_registered 

    DESCRIPTION :

    IN :
    struct gid_machine *machine : 
    
    OUT :
    PAL_TRUE :
    PAL_FALSE :

    CALLED BY :
*/
extern bool_t
gid_fsm_is_machine_registered (struct gid_machine *machine);


/*
    FUNCTION : gid_fsm_get_states 

    DESCRIPTION :

    IN :
    struct gid_machine *machine : 
    struct gid_states *state : 
    
    OUT :
    PAL_TRUE :
    PAL_FALSE :

    CALLED BY :
*/
extern void
gid_fsm_get_states (struct gid_machine *machine, struct gid_states *state);


/*
    FUNCTION : gid_fsm_init_machine

    DESCRIPTION :

    IN :
    struct gid_machine *machine : 
    struct gid_states *state : 
    
    OUT :
    none

    CALLED BY : gid_new_gid 
*/
extern void
gid_fsm_init_machine (struct gid_machine *machine); 

#if defined HAVE_MMRP || defined HAVE_MVRP

/*
    FUNCTION : mad_fsm_change_tx_states 
    DESCRIPTION :

    IN :
    struct gid *gid :
    struct gid_machine *machine :
    mrp_tx_event_t tx_event

    OUT :
    PAL_TRUE :
    PAL_FALSE :

    CALLED BY :
*/

enum gid_event
mad_fsm_change_tx_states (struct gid *port, struct gid_machine *machine,
                          enum mrp_tx_event tx_event);

#endif /* HAVE_MMRP || HAVE_MVRP */
#endif /* _PACOS_GID_FSM_H */
