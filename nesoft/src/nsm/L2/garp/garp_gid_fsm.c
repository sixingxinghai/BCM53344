/* Copyright 2003  All Rights Reserved. */

#include "pal.h"

#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#include "garp_gid_fsm.h"
#include "garp_debug.h"
#include "garp_gid_timer.h"

extern const char* const gid_event_string [];


/* Enumeration for Timers */
enum timers 
{
  NO_TIMER    = 0, /* No timer action */
  JOIN_TIMER  = 1, /* cstart_join_timer */
  LEAVE_TIMER = 2, /* cstart_leave_timer */
  LEAVE_ALL_TIMER = 3, /* leave all timer */ 
  TIMER_MAX_  = 4
};


/* Enumerations for GID's Applicant Messages */
enum applicant_msg 
{
  APP_MSG_NULL       = 0, /* No message to transmit */
  APP_MSG_JOIN       = 1, /* Transmit a join message */
  APP_MSG_LEAVE      = 2, /* Transmit a leave message */
  APP_MSG_EMPTY      = 3, /* Transmit an empty message */
  APP_MSG_NEW        = 4, /* Transmit a New or NewIn message */
  APP_MSG_JOIN_OPT   = 5, /* Transmit a join message for Optimisation */
  APP_MSG_LEAVE_OPT  = 6, /* Transmit a leave message for Optimisation */
  APP_MSG_NULL_OPT   = 7, /* Transmit NULL for Optimisation */
  APP_MSG_MAX        = 8
};

enum leave_all_msg
{
  LEAVEALL_MSG_NONE       = 0,
  LEAVEALL_MSG_LEAVE_ALL  = 1,
  LEAVEALL_MSG_LEAVE_MINE = 2,
  LEAVEALL_MSG_MAX        = 3,
};


/* Enumerations for GID's Registrar's indications */
enum registrar_indications 
{
  REG_IND_NI  = 0, /* No indication */
  REG_IND_LI  = 1, /* Leave Indication */
  REG_IND_JI  = 2, /* Join Indication */
  REG_IND_NWI = 3, /* New Indication */
  REG_IND_MAX = 4 
};

/* Structure definition for GID's Applicant transition table entry */
struct applicant_tt_entry  
{
  unsigned char new_app_state     : 5; /* applicant states */
  unsigned char cstart_join_timer : 2; /* flag to start join timer */
};


/* Structure definition for GID's Registrar transition table entry */
struct registrar_tt_entry  
{
  unsigned char new_reg_state      : 5; /* registrar states */
  unsigned char indications        : 2; /* registar indications */
  unsigned char cstart_leave_timer : 2; /* flag to start leave timer */
};


/* Structure definition for GID's Applicant transmit transition table entry */
struct applicant_txtt_entry
{
  unsigned char new_app_state     : 5; /* applicant states */
  unsigned char msg_to_transmit   : 4; /* applicant msg to transmit */
  unsigned char cstart_join_timer : 2; /* flag to start join timer */
};

/* Structure definition for GID's Registrar leave timer entry */
struct registrar_leave_timer_entry 
{
  unsigned char new_reg_state      : 5; /* registrar states */
  unsigned char leave_indication   : 2; /* flag to specify leave indication */
  unsigned char cstart_leave_timer : 2; /* flag to start leave timer */
};

static struct applicant_tt_entry
applicant_garp_tt [NUM_GID_RCV_EVENTS + NUM_GID_REQ_EVENTS +
                   NUM_GID_APP_MGMT_EVENTS + NUM_GID_REG_MGMT_EVENTS]
                  [NUM_GID_APP_STATES] =
{ /* array[6+3+2+2][14] */
  { /* GID_EVENT_NULL  0 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_AA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA, NO_TIMER },      /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QP, NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_LO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_RCV_LEAVE_EMPTY 1 */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_VP, NO_TIMER },      /* Anxious Active */
    { APP_STATE_VP, JOIN_TIMER },    /* Quiet Active */
    { APP_STATE_VO, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_VP, JOIN_TIMER },    /* Quiet Passive */
    { APP_STATE_LO, JOIN_TIMER },    /* Very Anxious Observer */
    { APP_STATE_LO, JOIN_TIMER },    /* Anxious Observer */
    { APP_STATE_LO, JOIN_TIMER },    /* Quiet Observer */
    { APP_STATE_VO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_RCV_LEAVE_IN  2 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_VA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_VP, JOIN_TIMER },    /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_VP, JOIN_TIMER },    /* Quiet Passive */
    { APP_STATE_LO, JOIN_TIMER },    /* Very Anxious Observer */
    { APP_STATE_LO, JOIN_TIMER },    /* Anxious Observer */
    { APP_STATE_LO, JOIN_TIMER },    /* Quiet Observer */
    { APP_STATE_VO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_RCV_EMPTY  3 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_VA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_VA, JOIN_TIMER },    /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_VP, JOIN_TIMER },    /* Quiet Passive */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_VO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_VO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_VO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_RCV_JOIN_EMPTY 4 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_VA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_VA, JOIN_TIMER },    /* Quiet Active */
    { APP_STATE_VO, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_VP, JOIN_TIMER },    /* Quiet Passive */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_VO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_VO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_VO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_RCV_JOIN_IN 5 */
    { APP_STATE_AA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_QA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA, NO_TIMER },      /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_AP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_QP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QP, NO_TIMER },      /* Quiet Passive */
    { APP_STATE_AO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_QO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_AO, NO_TIMER },      /* Leave Observer */
  } ,
  { /* GID_EVENT_JOIN 6 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_AA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA, NO_TIMER },      /* Quiet Active */
    { APP_STATE_VA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QP, NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VP, JOIN_TIMER },    /* Very Anxious Observer */
    { APP_STATE_AP, JOIN_TIMER },    /* Anxious Observer */
    { APP_STATE_QP, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_VP, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_LEAVE 7 */
    { APP_STATE_LA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_LA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_LA, JOIN_TIMER },    /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AO, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QO, NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_LO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_NORMAL_REGISTRATION 8 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_AA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA, NO_TIMER },      /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QP, NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_LO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_FIXED_REGISTRATION 9 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_AA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA, NO_TIMER },      /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QP, NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_LO, NO_TIMER },      /* Leave Observer */
  },
  { /* GID_EVENT_FORBID_REGISTRATION 10 */
    { APP_STATE_VA, NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_AA, NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA, NO_TIMER },      /* Quiet Active */
    { APP_STATE_LA, NO_TIMER },      /* Leave Active */
    { APP_STATE_VP, NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AP, NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QP, NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VO, NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AO, NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO, NO_TIMER },      /* Quiet Observer */
    { APP_STATE_LO, NO_TIMER },      /* Leave Observer */
  }
};

#if defined HAVE_MMRP || defined HAVE_MVRP

static struct applicant_tt_entry 
applicant_mrp_tt  [MRP_APP_EVENT_MAX]
                  [NUM_GID_APP_STATES] = 
{
  { 
    /* MRP_APP_EVENT_NULL 0 */

    { APP_STATE_VA,  NO_TIMER },      /* Very Anxious Active */
    { APP_STATE_AA,  NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA,  NO_TIMER },      /* Quiet Active */
    { APP_STATE_LA,  JOIN_TIMER },      /* Leave Active */
    { APP_STATE_VP,  NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AP,  JOIN_TIMER },      /* Anxious Passive */
    { APP_STATE_QP,  NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VO,  NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AO,  NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO,  NO_TIMER },      /* Quiet Observer */
    { APP_STATE_LO,  NO_TIMER },      /* Leave Observer */
    { APP_STATE_VC,  NO_TIMER },      /* Very Anxious Changing */
    { APP_STATE_AC,  NO_TIMER }       /* Anxiuous Changing */
  }, 
  {
    /* MRP_APP_EVENT_RCV_LEAVE 1 */

    /* For MRP This will be generated for both
     * Leave and LeaveMine message 
     */

    { APP_STATE_VA,  JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_VA,  JOIN_TIMER },      /* Anxious Active */
    { APP_STATE_VA,  JOIN_TIMER },      /* Quiet Active */
    { APP_STATE_LA,  JOIN_TIMER },      /* Leave Active */
    { APP_STATE_VP,  JOIN_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP,  JOIN_TIMER },      /* Anxious Passive */
    { APP_STATE_VP,  JOIN_TIMER },      /* Quiet Passive */
    { APP_STATE_LO,  JOIN_TIMER },      /* Very Anxious Observer */
    { APP_STATE_LO,  JOIN_TIMER },      /* Anxious Observer */
    { APP_STATE_LO,  JOIN_TIMER },      /* Quiet Observer */
    { APP_STATE_VO,  NO_TIMER },      /* Leave Observer */
    { APP_STATE_VC,  JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_VC,  JOIN_TIMER }       /* Anxiuous Changing */
  },
  { 
    /* MRP_APP_EVENT_RCV_EMPTY 2 */

    /* This maps to receiving a empty message */

    { APP_STATE_VA,  JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_VA,  JOIN_TIMER },      /* Anxious Active */
    { APP_STATE_VA,  JOIN_TIMER },      /* Quiet Active */
    { APP_STATE_LA,  JOIN_TIMER },      /* Leave Active */
    { APP_STATE_VP,  JOIN_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP,  JOIN_TIMER },      /* Anxious Passive */
    { APP_STATE_VP,  JOIN_TIMER },      /* Quiet Passive */
    { APP_STATE_VO,  NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_VO,  NO_TIMER },      /* Anxious Observer */
    { APP_STATE_VO,  NO_TIMER },      /* Quiet Observer */
    { APP_STATE_VO,  NO_TIMER },      /* Leave Observer */
    { APP_STATE_VC,  JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_VC,  JOIN_TIMER }       /* Anxiuous Changing */
  },
  { 
    /* MRP_APP_EVENT_RCV_JOIN_NEW_MT 3 */

    /* For MRP This will be generated for both
     * Join Empty and New Empty 
     */

    { APP_STATE_VA,  JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_VA,  JOIN_TIMER },      /* Anxious Active */
    { APP_STATE_VA,  JOIN_TIMER },      /* Quiet Active */
    { APP_STATE_VO,  NO_TIMER },      /* Leave Active */
    { APP_STATE_VP,  JOIN_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP,  JOIN_TIMER },      /* Anxious Passive */
    { APP_STATE_VP,  JOIN_TIMER },      /* Quiet Passive */
    { APP_STATE_VO,  NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_VO,  NO_TIMER },      /* Anxious Observer */
    { APP_STATE_VO,  NO_TIMER },      /* Quiet Observer */
    { APP_STATE_VO,  NO_TIMER },      /* Leave Observer */
    { APP_STATE_VC,  JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_VC,  JOIN_TIMER }       /* Anxiuous Changing */
  },
  { 
    /* MRP_APP_EVENT_RCV_JOIN_NEW_IN 4 */

    /* For MRP This will be generated for both
     * Join In and New In.
     */

    { APP_STATE_AA,  JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_QA,  NO_TIMER },      /* Anxious Active */
    { APP_STATE_QA,  NO_TIMER },      /* Quiet Active */
    { APP_STATE_AO,  NO_TIMER },      /* Leave Active */
    { APP_STATE_AP,  JOIN_TIMER },      /* Very Anxious Passive */
    { APP_STATE_QP,  NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QP,  NO_TIMER },      /* Quiet Passive */
    { APP_STATE_AO,  NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_QO,  NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO,  NO_TIMER },      /* Quiet Observer */
    { APP_STATE_AO,  NO_TIMER },      /* Leave Observer */
    { APP_STATE_VC,  JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_AC,  JOIN_TIMER }       /* Anxiuous Changing */
  } ,
  { 
    /* MRP_APP_EVENT_JOIN 5 */

    /* This maps to Join Indication from the Propagation */

    { APP_STATE_VA,  JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_AA,  JOIN_TIMER },      /* Anxious Active */
    { APP_STATE_QA,  NO_TIMER },      /* Quiet Active */
    { APP_STATE_VA,  JOIN_TIMER },      /* Leave Active */
    { APP_STATE_VP,  JOIN_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AP,  JOIN_TIMER },      /* Anxious Passive */
    { APP_STATE_QP,  NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VP,  JOIN_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AP,  JOIN_TIMER },      /* Anxious Observer */
    { APP_STATE_QP,  NO_TIMER },      /* Quiet Observer */
    { APP_STATE_VP,  JOIN_TIMER },      /* Leave Observer */
    { APP_STATE_VC,  JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_AC,  JOIN_TIMER }       /* Anxiuous Changing */
  },

  { 
    /* MRP_APP_EVENT_LEAVE 6 */

    /* This maps to Join Indication from the Propagation */

    { APP_STATE_LA,  JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_LA,  JOIN_TIMER },      /* Anxious Active */
    { APP_STATE_LA,  JOIN_TIMER },      /* Quiet Active */
    { APP_STATE_LA,  JOIN_TIMER },      /* Leave Active */
    { APP_STATE_VO,  NO_TIMER },      /* Very Anxious Passive */
    { APP_STATE_AO,  NO_TIMER },      /* Anxious Passive */
    { APP_STATE_QO,  NO_TIMER },      /* Quiet Passive */
    { APP_STATE_VO,  NO_TIMER },      /* Very Anxious Observer */
    { APP_STATE_AO,  NO_TIMER },      /* Anxious Observer */
    { APP_STATE_QO,  NO_TIMER },      /* Quiet Observer */
    { APP_STATE_LO,  JOIN_TIMER },      /* Leave Observer */
    { APP_STATE_LA,  JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_LA,  JOIN_TIMER }       /* Anxiuous Changing */
  },
  { 
    /* MRP_APP_EVENT_NEW 7 */

    { APP_STATE_VC, JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_VC, JOIN_TIMER },      /* Anxious Active */
    { APP_STATE_VC, JOIN_TIMER },      /* Quiet Active */
    { APP_STATE_VC, JOIN_TIMER },      /* Leave Active */
    { APP_STATE_VC, JOIN_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VC, JOIN_TIMER },      /* Anxious Passive */
    { APP_STATE_VC, JOIN_TIMER },      /* Quiet Passive */
    { APP_STATE_VC, JOIN_TIMER },      /* Very Anxious Observer */
    { APP_STATE_VC, JOIN_TIMER },      /* Anxious Observer */
    { APP_STATE_VC, JOIN_TIMER },      /* Quiet Observer */
    { APP_STATE_VC, JOIN_TIMER },      /* Leave Observer */
    { APP_STATE_VC, JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_AC, JOIN_TIMER }       /* Anxiuous Changing */
  },
  {
    /* MRP_APP_EVENT_RCV_LEAVE_ALL 8 */

    { APP_STATE_VP, JOIN_TIMER },      /* Very Anxious Active */
    { APP_STATE_VP, JOIN_TIMER },      /* Anxious Active */
    { APP_STATE_VP, JOIN_TIMER },      /* Quiet Active */
    { APP_STATE_LO, JOIN_TIMER },      /* Leave Active */
    { APP_STATE_VP, JOIN_TIMER },      /* Very Anxious Passive */
    { APP_STATE_VP, JOIN_TIMER },      /* Anxious Passive */
    { APP_STATE_VP, JOIN_TIMER },      /* Quiet Passive */
    { APP_STATE_LO, JOIN_TIMER },      /* Very Anxious Observer */
    { APP_STATE_LO, JOIN_TIMER },      /* Anxious Observer */
    { APP_STATE_LO, JOIN_TIMER },      /* Quiet Observer */
    { APP_STATE_VO, NO_TIMER },      /* Leave Observer */
    { APP_STATE_VC, JOIN_TIMER },      /* Very Anxious Changing */
    { APP_STATE_VC, JOIN_TIMER }       /* Anxiuous Changing */
  },
};

#endif /* HAVE_MMRP || HAVE_MVRP */

static struct registrar_tt_entry
registrar_garp_tt[NUM_GID_RCV_EVENTS + NUM_GID_REQ_EVENTS +
                  NUM_GID_APP_MGMT_EVENTS + NUM_GID_REG_MGMT_EVENTS]
                 [NUM_GID_REG_STATES] =
{ /* array[6+2+2+3][18] =  a[13][18]*/
  { /* GID_EVENT_NULL  0 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER }, /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },  /* Empty Normal */

    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },  /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden  */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden  */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden  */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* GID_EVENT_RCV_LEAVE_EMPTY 1 */
    { REG_STATE_LV, REG_IND_NI, LEAVE_TIMER },  /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },     /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },     /* Leave countdown3 Normal*/
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },     /* Leave countdown2 Normal*/
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },     /* Leave countdown1 Normal*/
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },     /* Empty Normal*/

    { REG_STATE_INR, REG_IND_NI, NO_TIMER },    /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },    /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },    /* Empty Fixed */

    { REG_STATE_LVF, REG_IND_NI, LEAVE_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },    /* Leave Forbidden */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }     /* Empty Forbidden */
  },
  { /* GID_EVENT_RCV_LEAVE_IN 2 */
    { REG_STATE_LV, REG_IND_NI, LEAVE_TIMER },  /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },     /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },     /* Leave countdown3 Normal*/
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },     /* Leave countdown2 Normal*/
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },     /* Leave countdown1 Normal*/
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },     /* Empty Normal*/

    { REG_STATE_INR, REG_IND_NI, NO_TIMER },    /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },    /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },    /* Empty Fixed */

    { REG_STATE_LVF, REG_IND_NI, LEAVE_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },    /* Leave Forbidden */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }     /* Empty Forbidden */
  },
  { /* GID_EVENT_RCV_EMPTY 3 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER }, /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },  /* Empty Normal */

    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },  /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* GID_EVENT_RCV_JOIN_EMPTY 4 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* In Normal Registration */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_INN, REG_IND_JI, NO_TIMER },  /* Empty Normal */

    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_INR, REG_IND_JI, NO_TIMER },  /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* GID_EVENT_RCV_JOIN_IN 5 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* In Normal Registration */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_INN, REG_IND_JI, NO_TIMER },  /* Empty Normal */

    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_INR, REG_IND_JI, NO_TIMER },  /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* GID_EVENT_JOIN 6 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER }, /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },  /* Empty Normal */

    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },  /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* GID_EVENT_LEAVE  7 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },     /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },     /* Leave countdown3 Normal*/
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },     /* Leave countdown2 Normal*/
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },     /* Leave countdown1 Normal*/
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },     /* Empty Normal*/

    { REG_STATE_INR, REG_IND_NI, NO_TIMER }, /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },    /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },    /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },    /* Leave Forbidden */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }     /* Empty Forbidden */
  },
  { /* GID_EVENT_NORMAL_REGISTRATION 8 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER }, /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },  /* Empty Normal */

    { REG_STATE_INN, REG_IND_NI, NO_TIMER }, /* In Fixed Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_MT, REG_IND_LI, NO_TIMER },  /* Empty Fixed */

    { REG_STATE_INN, REG_IND_JI, NO_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LV, REG_IND_JI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_L3, REG_IND_JI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_L2, REG_IND_JI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_L1, REG_IND_JI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_MT, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* GID_EVENT_FIXED_REGISTRATION 9 */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER }, /* In Normal Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER }, /* Leave Normal */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER }, /* Leave countdown3 Normal */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER }, /* Leave countdown2 Normal */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER }, /* Leave countdown1 Normal */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER }, /* Empty Normal */

    { REG_STATE_INR, REG_IND_NI, NO_TIMER }, /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER }, /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER }, /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER }, /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER }, /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER }, /* Empty Fixed */

    { REG_STATE_INR, REG_IND_JI, NO_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LVR, REG_IND_JI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_L3R, REG_IND_JI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_L2R, REG_IND_JI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_L1R, REG_IND_JI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* GID_EVENT_FORBID_REGISTRATION 10 */
    { REG_STATE_INF, REG_IND_LI, NO_TIMER }, /* In Normal Registration */
    { REG_STATE_LVF, REG_IND_LI, NO_TIMER }, /* Leave Normal */
    { REG_STATE_L3F, REG_IND_LI, NO_TIMER }, /* Leave countdown3 Normal */
    { REG_STATE_L2F, REG_IND_LI, NO_TIMER }, /* Leave countdown2 Normal */
    { REG_STATE_L1F, REG_IND_LI, NO_TIMER }, /* Leave countdown1 Normal */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }, /* Empty Normal */

    { REG_STATE_INF, REG_IND_LI, NO_TIMER }, /* In Fixed Registration */
    { REG_STATE_LVF, REG_IND_LI, NO_TIMER }, /* Leave Fixed */
    { REG_STATE_L3F, REG_IND_LI, NO_TIMER }, /* Leave countdown3 Fixed */
    { REG_STATE_L2F, REG_IND_LI, NO_TIMER }, /* Leave countdown2 Fixed */
    { REG_STATE_L1F, REG_IND_LI, NO_TIMER }, /* Leave countdown1 Fixed */
    { REG_STATE_MTF, REG_IND_LI, NO_TIMER }, /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER }, /* Leave Forbidden  */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER }, /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER }, /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER }, /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }  /* Empty Forbidden */
  }
};

#if defined HAVE_MMRP || defined HAVE_MVRP

static struct registrar_tt_entry
registrar_mrp_tt [MRP_REG_EVENT_MAX]
                 [NUM_GID_REG_STATES] = 
{
  { /* MRP_REG_EVENT_NULL 0 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER }, /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },  /* Empty Normal */
    
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },  /* Empty Fixed */
    
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden  */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden  */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden  */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* MRP_REG_EVENT_RCV_NEW 1 */
    { REG_STATE_INN, REG_IND_NWI, NO_TIMER },  /* In Normal Registration */
    { REG_STATE_INN, REG_IND_NWI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_INN, REG_IND_NWI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_INN, REG_IND_NWI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_INN, REG_IND_NWI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_INN, REG_IND_NWI, NO_TIMER },  /* Empty Normal */

    { REG_STATE_INR, REG_IND_NWI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_INR, REG_IND_NWI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_INR, REG_IND_NWI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_INR, REG_IND_NWI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_INR, REG_IND_NWI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_INR, REG_IND_NWI, NO_TIMER },  /* Empty Fixed */

    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* MRP_REG_EVENT_RCV_JOIN 2 */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* In Normal Registration */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Normal */
    { REG_STATE_INN, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Normal */
    { REG_STATE_INN, REG_IND_JI, NO_TIMER },  /* Empty Normal */
    
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* In Fixed Registration */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Fixed */
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Fixed */
    { REG_STATE_INR, REG_IND_JI, NO_TIMER },  /* Empty Fixed */
    
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* In Forbidden Registration */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave Forbidden  */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown3 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown2 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER },  /* Leave countdown1 Forbidden */
    { REG_STATE_INF, REG_IND_NI, NO_TIMER }   /* Empty Forbidden */
  },
  { /* MRP_REG_EVENT_RCV_LEAVE_P2P 3 */
    { REG_STATE_MT, REG_IND_LI, NO_TIMER  },    /* In Normal Registration */
    { REG_STATE_MT, REG_IND_LI, NO_TIMER },     /* Leave Normal */
    { REG_STATE_MT, REG_IND_LI, NO_TIMER },     /* Leave countdown3 Normal*/
    { REG_STATE_MT, REG_IND_LI, NO_TIMER },     /* Leave countdown2 Normal*/
    { REG_STATE_MT, REG_IND_LI, NO_TIMER },     /* Leave countdown1 Normal*/
    { REG_STATE_MT, REG_IND_LI, NO_TIMER },     /* Empty Normal*/
    
    { REG_STATE_MTR, REG_IND_LI, NO_TIMER },    /* In Fixed Registration */
    { REG_STATE_MTR, REG_IND_LI, NO_TIMER },    /* Leave Fixed */
    { REG_STATE_MTR, REG_IND_LI, NO_TIMER },    /* Leave countdown3 Fixed */
    { REG_STATE_MTR, REG_IND_LI, NO_TIMER },    /* Leave countdown2 Fixed */
    { REG_STATE_MTR, REG_IND_LI, NO_TIMER },    /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_LI, NO_TIMER },    /* Empty Fixed */
    
    { REG_STATE_MTF, REG_IND_LI, LEAVE_TIMER }, /* In Forbidden Registration */
    { REG_STATE_MTF, REG_IND_LI, NO_TIMER },    /* Leave Forbidden */
    { REG_STATE_MTF, REG_IND_LI, NO_TIMER },    /* Leave countdown3 Forbidden */
    { REG_STATE_MTF, REG_IND_LI, NO_TIMER },    /* Leave countdown2 Forbidden */
    { REG_STATE_MTF, REG_IND_LI, NO_TIMER },    /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_LI, NO_TIMER }     /* Empty Forbidden */
  },
  { /* MRP_REG_EVENT_RCV_LEAVE_NOT_P2P 4 */
    { REG_STATE_LV, REG_IND_NI, LEAVE_TIMER },  /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },     /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },     /* Leave countdown3 Normal*/
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },     /* Leave countdown2 Normal*/
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },     /* Leave countdown1 Normal*/
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },     /* Empty Normal*/
    
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },    /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },    /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },    /* Empty Fixed */
    
    { REG_STATE_LVF, REG_IND_NI, LEAVE_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },    /* Leave Forbidden */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }     /* Empty Forbidden */
  },
  { /* MRP_REG_EVENT_LEAVE_ALL 5 */
    { REG_STATE_LV, REG_IND_NI, LEAVE_TIMER },  /* In Normal Registration */
    { REG_STATE_LV, REG_IND_NI, NO_TIMER },     /* Leave Normal */
    { REG_STATE_L3, REG_IND_NI, NO_TIMER },     /* Leave countdown3 Normal*/
    { REG_STATE_L2, REG_IND_NI, NO_TIMER },     /* Leave countdown2 Normal*/
    { REG_STATE_L1, REG_IND_NI, NO_TIMER },     /* Leave countdown1 Normal*/
    { REG_STATE_MT, REG_IND_NI, NO_TIMER },     /* Empty Normal*/
    
    { REG_STATE_INR, REG_IND_NI, NO_TIMER },    /* In Fixed Registration */
    { REG_STATE_LVR, REG_IND_NI, NO_TIMER },    /* Leave Fixed */
    { REG_STATE_L3R, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Fixed */
    { REG_STATE_L2R, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Fixed */
    { REG_STATE_L1R, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Fixed */
    { REG_STATE_MTR, REG_IND_NI, NO_TIMER },    /* Empty Fixed */
    
    { REG_STATE_LVF, REG_IND_NI, LEAVE_TIMER }, /* In Forbidden Registration */
    { REG_STATE_LVF, REG_IND_NI, NO_TIMER },    /* Leave Forbidden */
    { REG_STATE_L3F, REG_IND_NI, NO_TIMER },    /* Leave countdown3 Forbidden */
    { REG_STATE_L2F, REG_IND_NI, NO_TIMER },    /* Leave countdown2 Forbidden */
    { REG_STATE_L1F, REG_IND_NI, NO_TIMER },    /* Leave countdown1 Forbidden */
    { REG_STATE_MTF, REG_IND_NI, NO_TIMER }     /* Empty Forbidden */
  },
};

#endif /* HAVE_MVRP || HAVE_MMRP */


static struct applicant_txtt_entry
applicant_garp_txtt[NUM_GID_APP_STATES] =
{
  /* Very Anxious Active */
  { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER },
  /* Anxious Active */
  { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
  /* Quiet Active */
  { APP_STATE_QA, APP_MSG_NULL, NO_TIMER },
  /* Leave Active */
  { APP_STATE_VO, APP_MSG_LEAVE, NO_TIMER },
  /* Very Anxious Passive */
  { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER },
  /* Anxious Passive */
  { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
  /* Quiet Passive */
  { APP_STATE_QP, APP_MSG_NULL, NO_TIMER },
  /* Very Anxious Observer */
  { APP_STATE_VO, APP_MSG_NULL, NO_TIMER },
  /* Anxious Observer */
  { APP_STATE_AO, APP_MSG_NULL, NO_TIMER },
  /* Quiet Observer */
  { APP_STATE_QO, APP_MSG_NULL, NO_TIMER },
  /* Leave Observer */
  { APP_STATE_VO, APP_MSG_EMPTY, NO_TIMER },
};

#if defined HAVE_MMRP || defined HAVE_MVRP

static struct applicant_txtt_entry
applicant_mrp_txtt [MRP_TX_EVENT_MAX] [NUM_GID_APP_STATES] =
{
  { /* MRP_TX_EVENT_NULL 0 */

    /* Very Anxious Active */
    { APP_STATE_VA, APP_MSG_NULL, NO_TIMER }, 
    /* Anxious Active */
    { APP_STATE_AA, APP_MSG_NULL, NO_TIMER }, 
    /* Quiet Active */
    { APP_STATE_QA, APP_MSG_NULL, NO_TIMER },
    /* Leave Active */
    { APP_STATE_LA, APP_MSG_NULL, NO_TIMER },
    /* Very Anxious Passive */
    { APP_STATE_VP, APP_MSG_NULL, NO_TIMER },
    /* Anxious Passive */
    { APP_STATE_AP, APP_MSG_NULL, NO_TIMER },
    /* Quiet Passive */
    { APP_STATE_QP, APP_MSG_NULL, NO_TIMER },
    /* Very Anxious Observer */
    { APP_STATE_VO, APP_MSG_NULL, NO_TIMER },
    /* Anxious Observer */
    { APP_STATE_AO, APP_MSG_NULL, NO_TIMER },
    /* Quiet Observer */
    { APP_STATE_QO, APP_MSG_NULL, NO_TIMER },
    /* Leave Observer */
    { APP_STATE_LO, APP_MSG_NULL, NO_TIMER },
    /* Very Anxious Change member */
    { APP_STATE_VC, APP_MSG_NULL, NO_TIMER },
    /* Anxious Change member */
    { APP_STATE_AC, APP_MSG_NULL, NO_TIMER }
  },

  { /* MRP_TX_EVENT_P2P 1 */

    /* Very Anxious Active */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER }, 
    /* Anxious Active */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER }, 
    /* Quiet Active */
    { APP_STATE_QA, APP_MSG_JOIN_OPT, NO_TIMER },
    /* Leave Active */
    { APP_STATE_VO, APP_MSG_LEAVE, NO_TIMER },
    /* Very Anxious Passive */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER },
    /* Anxious Passive */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
    /* Quiet Passive */
    { APP_STATE_QA, APP_MSG_NULL_OPT, NO_TIMER },
    /* Very Anxious Observer */
    { APP_STATE_VO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Anxious Observer */
    { APP_STATE_AO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Quiet Observer */
    { APP_STATE_QO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Leave Observer */
    { APP_STATE_VO, APP_MSG_LEAVE, NO_TIMER },
    /* Very Anxious Change member */
    { APP_STATE_AC, APP_MSG_NEW, JOIN_TIMER },
    /* Anxious Change member */
    { APP_STATE_QA, APP_MSG_NEW, NO_TIMER }
  },

  { /* MRP_TX_EVENT_NOT_P2P 2 */

    /* Very Anxious Active */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER }, 
    /* Anxious Active */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER }, 
    /* Quiet Active */
    { APP_STATE_QA, APP_MSG_JOIN_OPT, NO_TIMER },
    /* Leave Active */
    { APP_STATE_VO, APP_MSG_LEAVE, NO_TIMER },
    /* Very Anxious Passive */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER },
    /* Anxious Passive */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
    /* Quiet Passive */
    { APP_STATE_QA, APP_MSG_NULL_OPT, NO_TIMER },
    /* Very Anxious Observer */
    { APP_STATE_VO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Anxious Observer */
    { APP_STATE_AO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Quiet Observer */
    { APP_STATE_QO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Leave Observer */
    { APP_STATE_VO, APP_MSG_EMPTY, NO_TIMER },
    /* Very Anxious Change member */
    { APP_STATE_AC, APP_MSG_NEW, JOIN_TIMER },
    /* Anxious Change member */
    { APP_STATE_QA, APP_MSG_NEW, NO_TIMER }
  },

  { /* MRP_TX_EVENT_LEAVE_MINE 2 */

    /* Very Anxious Active */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER }, 
    /* Anxious Active */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER }, 
    /* Quiet Active */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
    /* Leave Active */
    { APP_STATE_VO, APP_MSG_LEAVE, NO_TIMER },
    /* Very Anxious Passive */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER },
    /* Anxious Passive */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
    /* Quiet Passive */
    { APP_STATE_QA, APP_MSG_NULL_OPT, NO_TIMER },
    /* Very Anxious Observer */
    { APP_STATE_VO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Anxious Observer */
    { APP_STATE_AO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Quiet Observer */
    { APP_STATE_QO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Leave Observer */
    { APP_STATE_VO, APP_MSG_LEAVE_OPT, NO_TIMER },
    /* Very Anxious Change member */
    { APP_STATE_AC, APP_MSG_NEW, JOIN_TIMER },
    /* Anxious Change member */
    { APP_STATE_QA, APP_MSG_NEW, NO_TIMER }
  },

  { /* MRP_TX_EVENT_LEAVE_ALL 4 */

    /* Very Anxious Active */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER }, 
    /* Anxious Active */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER }, 
    /* Quiet Active */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
    /* Leave Active */
    { APP_STATE_VO, APP_MSG_LEAVE, NO_TIMER },
    /* Very Anxious Passive */
    { APP_STATE_AA, APP_MSG_JOIN, JOIN_TIMER },
    /* Anxious Passive */
    { APP_STATE_QA, APP_MSG_JOIN, NO_TIMER },
    /* Quiet Passive */
    { APP_STATE_QA, APP_MSG_NULL_OPT, NO_TIMER },
    /* Very Anxious Observer */
    { APP_STATE_LO, APP_MSG_NULL_OPT, JOIN_TIMER },
    /* Anxious Observer */
    { APP_STATE_LO, APP_MSG_NULL_OPT, JOIN_TIMER },
    /* Quiet Observer */
    { APP_STATE_LO, APP_MSG_NULL_OPT, JOIN_TIMER },
    /* Leave Observer */
    { APP_STATE_VO, APP_MSG_NULL_OPT, NO_TIMER },
    /* Very Anxious Change member */
    { APP_STATE_AC, APP_MSG_NEW, JOIN_TIMER },
    /* Anxious Change member */
    { APP_STATE_QA, APP_MSG_NEW, NO_TIMER }
  },

};

#endif /* HAVE_MVRP || HAVE_MMRP */

static struct registrar_leave_timer_entry
registrar_leave_timer_table[NUM_GID_REG_STATES] = 
{
  { REG_STATE_INN, REG_IND_NI, NO_TIMER },     /* In Normal Registration */
  { REG_STATE_L3, REG_IND_NI, LEAVE_TIMER },   /* Leave Normal */
  { REG_STATE_L2, REG_IND_NI, LEAVE_TIMER },   /* Leave countdown3 Normal */
  { REG_STATE_L1, REG_IND_NI, LEAVE_TIMER },   /* Leave countdown2 Normal */
  { REG_STATE_MT, REG_IND_LI, NO_TIMER },      /* Leave countdown1 Normal */
  { REG_STATE_MT, REG_IND_NI, NO_TIMER },      /* Empty Normal */
  
  { REG_STATE_INR, REG_IND_NI, NO_TIMER },     /* In Fixed Registration */
  { REG_STATE_L3R, REG_IND_NI, LEAVE_TIMER },  /* Leave Fixed */
  { REG_STATE_L2R, REG_IND_NI, LEAVE_TIMER },  /* Leave countdown3 Fixed */
  { REG_STATE_L1R, REG_IND_NI, LEAVE_TIMER },  /* Leave countdown2 Fixed */
  { REG_STATE_MTR, REG_IND_LI, NO_TIMER },     /* Leave countdown1 Fixed */
  { REG_STATE_MTR, REG_IND_NI, NO_TIMER },     /* Empty Fixed */

  { REG_STATE_INF, REG_IND_NI, NO_TIMER },     /* In Forbidden Registration */
  { REG_STATE_L3F, REG_IND_NI, LEAVE_TIMER },  /* Leave Forbidden */
  { REG_STATE_L2F, REG_IND_NI, LEAVE_TIMER },  /* Leave countdown3 Forbidden */
  { REG_STATE_L1F, REG_IND_NI, LEAVE_TIMER },  /* Leave countdown2 Forbidden */
  { REG_STATE_MTF, REG_IND_NI, NO_TIMER },     /* Leave countdown1 Forbidden */
  { REG_STATE_MTF, REG_IND_NI, NO_TIMER }      /* Empty Forbidden */
};

static enum gid_applicant_state 
applicant_state_table[NUM_GID_APP_STATES] = 
{
    GID_APP_STATE_VERY_ANXIOUS,   /* Very Anxious Active */
    GID_APP_STATE_ANXIOUS,        /* Anxious Active */
    GID_APP_STATE_QUIET  ,        /* Quiet Active */
    GID_APP_STATE_LEAVING  ,      /* Leaving Avtive */
    GID_APP_STATE_VERY_ANXIOUS  , /* Very Anxious Passive */
    GID_APP_STATE_ANXIOUS  ,      /* Anxious Passive*/
    GID_APP_STATE_QUIET  ,        /* Quiet Passive  */
    GID_APP_STATE_VERY_ANXIOUS  , /* Very Anxious Observer */
    GID_APP_STATE_ANXIOUS  ,      /* Anxious Observer */
    GID_APP_STATE_QUIET  ,        /* Quiet Observer */
    GID_APP_STATE_LEAVING  ,      /* Leaving Observer */
    GID_APP_STATE_CHANGE_MEMBER , /* Very Anxious Change Member */
    GID_APP_STATE_CHANGE_MEMBER   /* Anxious Change Member */
};


static enum gid_applicant_mgmt
applicant_mgmt_table[NUM_GID_APP_STATES] = 
{
    GID_APP_MGMT_NORMAL  ,         /* Very Anxious Active */
    GID_APP_MGMT_NORMAL  ,         /* Anxious Active */
    GID_APP_MGMT_NORMAL  ,         /* Quiet Active */
    GID_APP_MGMT_NORMAL  ,         /* Leaving Active */

    GID_APP_MGMT_NORMAL  ,         /* Very Anxious Passive */
    GID_APP_MGMT_NORMAL  ,         /* Anxious Passive*/
    GID_APP_MGMT_NORMAL  ,         /* Quiet Passive  */

    GID_APP_MGMT_NORMAL  ,         /* Very Anxious Observer */
    GID_APP_MGMT_NORMAL  ,         /* Anxious Observer */
    GID_APP_MGMT_NORMAL  ,         /* Quiet Observer */
    GID_APP_MGMT_NORMAL  ,         /* Leaving Observer */

    GID_APP_MGMT_NORMAL  ,         /* Very Anxious Changing*/
    GID_APP_MGMT_NORMAL  ,         /* Anxious Changing*/
};


static enum gid_registrar_state
registrar_state_table[NUM_GID_REG_STATES] = 
{
    GID_REG_STATE_IN,      /* In Normal Registration */
    GID_REG_STATE_LEAVE,   /* Leave Normal */
    GID_REG_STATE_LEAVE,   /* Leave countdown3 Normal */
    GID_REG_STATE_LEAVE,   /* Leave countdown2 Normal */
    GID_REG_STATE_LEAVE,   /* Leave countdown1 Normal */
    GID_REG_STATE_EMPTY,   /* Empty Normal */

    GID_REG_STATE_IN,      /* In Fixed Registration */
    GID_REG_STATE_LEAVE,   /* Leave Fixed */
    GID_REG_STATE_LEAVE,   /* Leave countdown3 Fixed */
    GID_REG_STATE_LEAVE,   /* Leave countdown2 Fixed */
    GID_REG_STATE_LEAVE,   /* Leave countdown1 Fixed */
    GID_REG_STATE_EMPTY,   /* Empty Fixed */

    GID_REG_STATE_IN,      /* In Forbidden Registration */
    GID_REG_STATE_LEAVE,   /* Leave Forbidden */
    GID_REG_STATE_LEAVE,   /* Leave countdown3 Forbidden */
    GID_REG_STATE_LEAVE,   /* Leave countdown2 Forbidden */
    GID_REG_STATE_LEAVE,   /* Leave countdown1 Forbidden */
    GID_REG_STATE_EMPTY    /* Empty Forbidden */
}; 

static enum gid_registrar_mgmt
registrar_mgmt_table[NUM_GID_REG_STATES] = 
{
    GID_REG_MGMT_NORMAL  ,   /* In Normal Registration */
    GID_REG_MGMT_NORMAL  ,   /* Leave Normal */
    GID_REG_MGMT_NORMAL  ,   /* Leave countdown3 Normal */
    GID_REG_MGMT_NORMAL  ,   /* Leave countdown2 Normal */
    GID_REG_MGMT_NORMAL  ,   /* Leave countdown1 Normal */
    GID_REG_MGMT_NORMAL  ,   /* Empty Normal */

    GID_REG_MGMT_FIXED  ,   /* In Fixed Registration */
    GID_REG_MGMT_FIXED  ,   /* Leave Fixed */
    GID_REG_MGMT_FIXED  ,   /* Leave countdown3 Fixed */
    GID_REG_MGMT_FIXED  ,   /* Leave countdown2 Fixed */
    GID_REG_MGMT_FIXED  ,   /* Leave countdown1 Fixed */
    GID_REG_MGMT_FIXED  ,   /* Empty Fixed */

    GID_REG_MGMT_FORBIDDEN  ,   /* In Forbidden Registration */
    GID_REG_MGMT_FORBIDDEN  ,   /* Leave Forbidden */
    GID_REG_MGMT_FORBIDDEN  ,   /* Leave countdown3 Forbidden */
    GID_REG_MGMT_FORBIDDEN  ,   /* Leave countdown2 Forbidden */
    GID_REG_MGMT_FORBIDDEN  ,   /* Leave countdown1 Forbidden */
    GID_REG_MGMT_FORBIDDEN      /* Empty Forbidden */
};

static bool_t
registrar_in_table[NUM_GID_REG_STATES] =
{
    PAL_TRUE  ,   /* In Normal Registration */
    PAL_TRUE  ,   /* Leave Normal */
    PAL_TRUE  ,   /* Leave countdown3 Normal */
    PAL_TRUE  ,   /* Leave countdown2 Normal */
    PAL_TRUE  ,   /* Leave countdown1 Normal */
    PAL_FALSE  ,  /* Empty Normal */

    PAL_TRUE  ,   /* In Fixed Registration */
    PAL_TRUE  ,   /* Leave Fixed */
    PAL_TRUE  ,   /* Leave countdown3 Fixed */
    PAL_TRUE  ,   /* Leave countdown2 Fixed */
    PAL_TRUE  ,   /* Leave countdown1 Fixed */
    PAL_TRUE  ,   /* Empty Fixed */

    PAL_FALSE  ,   /* In Forbidden Registration */
    PAL_FALSE  ,   /* Leave Forbidden */
    PAL_FALSE  ,   /* Leave countdown3 Forbidden */
    PAL_FALSE  ,   /* Leave countdown2 Forbidden */
    PAL_FALSE  ,   /* Leave countdown1 Forbidden */
    PAL_FALSE      /* Empty Forbidden */
};

char* applicant_states_string[NUM_GID_APP_STATES+1] = 
{
  "VA",  /* Very Anxious Active */
  "AA",  /* Anxious Active */
  "QA",  /* Quiet Active */
  "LA",  /* Leaving Active */
  "VP",  /* Very Anxious Passive */
  "AP",  /* Anxious Passive */
  "QP",  /* Quiet Passive */
  "VO",  /* Very Anxious Observer */
  "AO",  /* Anxious Observer */
  "QO",  /* Quiet Observer */
  "LO",  /* Leaving Observer */
  "VC",  /* Very Anxious Changing */
  "AC",  /* Anxious Changing */
  "MAX"
};


/* Enumeration for GID's Registrar State Machine */
char* registrar_states_string[NUM_GID_REG_STATES+1] =
{
  "INN",  /* In Normal Registration */
  "LV",  /* Leave Normal Registration */
  "L3",  /* Leave countdown3 Normal Registration */
  "L2",  /* Leave countdown2 Normal Registration */
  "L1",  /* Leave countdown1 Normal Registration */
  "MT",  /* Empty Normal Registration */
  "INR",  /* In Fixed Registration */
  "LVR",  /* Leave Fixed Registration */
  "L3R",  /* Leave countdown3 Fixed Registration */
  "L2R",  /* Leave countdown2 Fixed Registration */
  "L1R", /* Leave countdown1 Normal Registration */
  "MTR", /* Empty Fixed Registration */
  "INF", /* In Forbidden Registration */
  "LVF", /* Leave Forbidden Registration */
  "L3F", /* Leave countdown3 Forbidden Registarion */
  "L2F", /* Leave countdown2 Forbidden Registarion */
  "L1F", /* Leave countdown1 Forbidden Registarion */
  "MTF", /* Empty Forbidden Registration */
  "MAX"
};

const char* const timers_string[] = 
{
  "NO_TIMER",    /* No timer action */
  "JOIN_TIMER",  /* cstart_join_timer */
  "LEAVE_TIMER", /* cstart_leave_timer */
  "LEAVE_ALL_TIMER",
  "TIMER_MAX_"
};

const char* const registrar_indications_string[] = 
{
  "REG_IND_NI", /* No indication */
  "REG_IND_LI", /* Leave Indication */
  "REG_IND_JI", /* Join Indication */
  "REG_IND_MAX "
};

#if defined HAVE_MVRP || defined HAVE_MMRP
static mrp_app_event_t
mrp_map_gid_event_to_applicant_event (enum gid_event event)
{
  mrp_app_event_t app_event;

  switch (event)
    {
      case GID_EVENT_RCV_LEAVE_MINE:
      case GID_EVENT_RCV_LEAVE:
        app_event = MRP_APP_EVENT_RCV_LEAVE;
         break;
      case GID_EVENT_RCV_EMPTY:
        app_event = MRP_APP_EVENT_RCV_EMPTY;
        break;
      case GID_EVENT_RCV_NEW_EMPTY:
      case GID_EVENT_RCV_JOIN_EMPTY:
        app_event = MRP_APP_EVENT_RCV_JOIN_NEW_MT;
        break;
      case GID_EVENT_RCV_NEW_IN:
      case GID_EVENT_RCV_JOIN_IN:
        app_event = MRP_APP_EVENT_RCV_JOIN_NEW_IN;
        break;
      case GID_EVENT_JOIN:
        app_event = MRP_APP_EVENT_JOIN;
        break;
      case GID_EVENT_NEW:
        app_event = MRP_APP_EVENT_NEW;
        break;
      case GID_EVENT_LEAVE:
        app_event = MRP_APP_EVENT_LEAVE;
        break;
      case GID_EVENT_RCV_LEAVE_ALL:
        app_event = MRP_APP_EVENT_RCV_LEAVE_ALL;
        break;
      default:
        app_event = MRP_APP_EVENT_NULL;
        break;
    }
  return app_event;
}

static mrp_reg_event_t
mrp_map_gid_event_to_registrar_event (enum gid_event event,
                                      u_int8_t p2p)
{
  mrp_reg_event_t reg_event;

  switch (event)
    {
      case GID_EVENT_RCV_NEW:
      case GID_EVENT_RCV_NEW_IN:
      case GID_EVENT_RCV_NEW_EMPTY:
        reg_event = MRP_REG_EVENT_RCV_NEW;
        break;
      case GID_EVENT_RCV_JOIN:
      case GID_EVENT_RCV_JOIN_IN:
      case GID_EVENT_RCV_JOIN_EMPTY:
        reg_event = MRP_REG_EVENT_RCV_JOIN;
        break;
      case GID_EVENT_RCV_LEAVE:
        if (p2p)
          reg_event = MRP_REG_EVENT_RCV_LEAVE_P2P;
        else
          reg_event = MRP_REG_EVENT_RCV_LEAVE_NOT_P2P;
        break;
      case GID_EVENT_RCV_LEAVE_ALL:
        reg_event = MRP_REG_EVENT_LEAVE_ALL;
        break;
      default:
        reg_event = MRP_REG_EVENT_NULL;
        break;
    }

  return reg_event;
}

#endif /* HAVE_MVRP || HAVE_MMRP */


enum gid_event
gid_fsm_change_states (struct gid *port, struct gid_machine *machine, 
                       enum gid_event event)
{
  struct applicant_tt_entry *atransition;
  struct registrar_tt_entry *rtransition;
  enum gid_event new_event;
#if defined HAVE_MVRP || defined HAVE_MMRP
  mrp_app_event_t app_event;
  mrp_reg_event_t reg_event;
#endif /* HAVE_MVRP || define HAVE_MMRP */
  struct garp_instance *garp_instance = port->application;
  struct gid_port *gid_port = port->gid_port;
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return GID_EVENT_NULL;

  garp = garp_instance->garp;

  if (!garp)
    return GID_EVENT_NULL;


  /* print_event (event); */

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_change_states: [port %d] " 
                  "old reg_state %s old app_state %s "
                  "new reg_leave_timer %s new app_join_timer %s ",
                  gid_port->ifindex, 
                  registrar_states_string[machine->registrar],
                  applicant_states_string[machine->applicant],
                  timers_string[port->cstart_leave_timer],
                  timers_string[port->cstart_join_timer]);
    }

#if defined HAVE_MVRP || defined HAVE_MMRP
  if (garp->proto == REG_PROTO_MRP)
    {
      app_event = mrp_map_gid_event_to_applicant_event (event);
      reg_event = mrp_map_gid_event_to_registrar_event (event, gid_port->p2p);
      atransition = &applicant_mrp_tt[app_event][machine->applicant];
      rtransition = &registrar_mrp_tt[reg_event][machine->registrar];
    }
  else
#endif /* HAVE_MVRP || define HAVE_MMRP */
    {
      atransition = &applicant_garp_tt[event][machine->applicant];
      rtransition = &registrar_garp_tt[event][machine->registrar];
    }
    

  /* Change the states of applicant and registrar to new state */
  machine->applicant = atransition->new_app_state;
  machine->registrar = rtransition->new_reg_state;

  if ((event == GID_EVENT_JOIN) && (atransition->cstart_join_timer) &&
      (!port->join_timer_running) )
    {
      port->join_timer_thread = garp_start_random_timer ( 
                                                        gid_join_timer_handler,
                                                        port, 0,
                                                        gid_port->join_timeout);
      port->join_timer_running = PAL_TRUE;

    }

  port->cstart_join_timer = port->cstart_join_timer ||
                            atransition->cstart_join_timer;
  port->cstart_leave_timer = port->cstart_leave_timer ||
                             rtransition->cstart_leave_timer;

  switch (rtransition->indications)
    {
      case REG_IND_JI :
        new_event = GID_EVENT_JOIN;
        break;
      case REG_IND_LI :
        new_event = GID_EVENT_LEAVE;
        break;
      case REG_IND_NWI :
#if defined HAVE_MVRP || defined HAVE_MMRP
        new_event = GID_EVENT_NEW;
        break;
#endif /* HAVE_MVRP || HAVE_MMRP */
      case REG_IND_NI:
      default:
        new_event = GID_EVENT_NULL;
        break;
    }

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_change_states: [port %d] "
                  "new reg_state %s new app_state %s new reg_indications %s "
                  "new reg_leave_timer %s new app_join_timer %s new_event %s",
                  gid_port->ifindex, registrar_states_string[machine->registrar],
                  applicant_states_string[machine->applicant], 
                  registrar_indications_string[rtransition->indications],
                  timers_string[rtransition->cstart_leave_timer],
                  timers_string[atransition->cstart_join_timer],
                  gid_event_string[new_event]);
    }

  return new_event;
}

enum gid_event
gid_fsm_change_tx_states (struct gid *port, struct gid_machine *machine)
{
  enum gid_event new_event;
  unsigned int regin = GID_REG_STATE_EMPTY;
  u_int32_t msg;
  struct garp_instance *garp_instance = port->application;
  struct gid_port *gid_port = port->gid_port;
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return GID_EVENT_NULL;

  garp = garp_instance->garp;

  if (!garp)
    return GID_EVENT_NULL;


  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_change_tx_states: [port %d] "
                  "old_reg_state %s old app_state %s ports cstart_join_timer "
                  "%s\n", gid_port->ifindex,
                  registrar_states_string[machine->registrar],
                  applicant_states_string[machine->applicant],
                  timers_string[port->cstart_join_timer]);
    }

  msg = applicant_garp_txtt[machine->applicant].msg_to_transmit;

  if(msg != APP_MSG_NULL)
    {
      regin = registrar_state_table[machine->registrar];
    }

  port->cstart_join_timer = port->cstart_join_timer ||
               applicant_garp_txtt[machine->applicant].cstart_join_timer;

  machine->applicant = applicant_garp_txtt[machine->applicant].new_app_state;

  switch (msg)
    {
      case APP_MSG_JOIN : /* Transmit a Join Message */
        new_event = (regin != GID_REG_STATE_EMPTY ? GID_EVENT_TX_JOIN_IN :
                                                    GID_EVENT_TX_JOIN_EMPTY);
        break;
      case APP_MSG_LEAVE : /* Transmit a Leave Message */
        new_event = (regin != GID_REG_STATE_EMPTY ? GID_EVENT_TX_LEAVE_IN :
                                                    GID_EVENT_TX_LEAVE_EMPTY);
        break;
      case APP_MSG_EMPTY : /* Transmit an Empty Message */
        new_event = GID_EVENT_TX_EMPTY;
        break;
      case APP_MSG_NULL : /* Dont Transmit any Message */
      default:
        new_event = GID_EVENT_NULL;
        break;
    }

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_change_tx_states: [port %d] "
                  "new app_state %s "
                  "new reg_state %s "
                  "ports cstart_join_timer %s new_event %s",
                  gid_port->ifindex,
                  applicant_states_string[machine->applicant],
                  registrar_states_string[machine->registrar],
                  timers_string[port->cstart_join_timer],
                  gid_event_string[new_event]);
    }

  return new_event;
}

#if defined HAVE_MVRP || defined HAVE_MMRP

enum gid_event
mad_fsm_change_tx_states (struct gid *port, struct gid_machine *machine,
                          mrp_tx_event_t tx_event)
{
  enum gid_event new_event;
  unsigned int regin = GID_REG_STATE_EMPTY;
  u_int32_t msg;
  struct garp_instance *garp_instance = port->application;
  struct gid_port *gid_port = port->gid_port;
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return GID_EVENT_NULL;

  garp = garp_instance->garp;

  if (!garp)
    return GID_EVENT_NULL;


  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_change_tx_states: [port %d] "
                  "old_reg_state %s old app_state %s ports cstart_join_timer "
                  "%s\n", gid_port->ifindex,
                  registrar_states_string[machine->registrar],
                  applicant_states_string[machine->applicant],
                  timers_string[port->cstart_join_timer]);
    }

  msg = applicant_mrp_txtt [tx_event] [machine->applicant].msg_to_transmit;

  if(msg != APP_MSG_NULL)
    {
      regin = registrar_state_table[machine->registrar];
    }

  port->cstart_join_timer = port->cstart_join_timer ||
        applicant_mrp_txtt [tx_event][machine->applicant].cstart_join_timer;

  machine->applicant = 
           applicant_mrp_txtt [tx_event][machine->applicant].new_app_state;

  switch (msg)
    {
      case APP_MSG_JOIN : /* Transmit a Join Message */
        new_event = (regin != GID_REG_STATE_EMPTY ? GID_EVENT_TX_JOIN_IN :
                                                    GID_EVENT_TX_JOIN_EMPTY);
        break;
      case APP_MSG_JOIN_OPT : /* Transmit a Join Message */
        new_event = (regin != GID_REG_STATE_EMPTY ? GID_EVENT_TX_JOIN_IN_OPT :
                                                    GID_EVENT_TX_JOIN_EMPTY_OPT);
        break;
      case APP_MSG_NEW:
        new_event = (regin != GID_REG_STATE_EMPTY ? GID_EVENT_TX_NEW_IN :
                                                    GID_EVENT_TX_NEW_EMPTY);
        break;
      case APP_MSG_LEAVE: /* Transmit a Leave Message */
        new_event = GID_EVENT_TX_LEAVE;
        break;
      case APP_MSG_LEAVE_OPT: /* Transmit a Leave Message */
        new_event = GID_EVENT_TX_LEAVE_OPT;
        break;
      case APP_MSG_EMPTY: /* Transmit an Empty Message */
        new_event = GID_EVENT_TX_EMPTY;
        break;
      case APP_MSG_NULL_OPT:
        new_event = GID_EVENT_TX_NULL_OPT;
      case APP_MSG_NULL: /* Dont Transmit any Message */
      default:
        new_event = GID_EVENT_NULL;
        break;
    }

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_change_tx_states: [port %d] "
                  "new app_state %s "
                  "new reg_state %s "
                  "ports cstart_join_timer %s new_event %s",
                  gid_port->ifindex, 
                  applicant_states_string[machine->applicant],
                  registrar_states_string[machine->registrar],
                  timers_string[port->cstart_join_timer],
                  gid_event_string[new_event]);
    }
  
  return new_event;
}

#endif /* HAVE_MVRP || HAVE_MRP */

bool_t
gid_fsm_get_registrar_in (struct gid_machine *machine)
{
  return registrar_in_table[machine->registrar];
}

enum gid_event
gid_fsm_leave_timer_expiry (struct gid *port, struct gid_machine *machine)
{
  struct registrar_leave_timer_entry *rtransition;
  u_int32_t new_event;
  struct garp_instance *garp_instance = port->application;
  struct gid_port *gid_port = port->gid_port;
  struct garp *garp = NULL;

  if (!garp_instance || !gid_port)
    return GID_EVENT_NULL;

  garp = garp_instance->garp;

  if (!garp)
    return GID_EVENT_NULL;

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_leave_timer_expiry: [port %d] "
                  "old reg_state %s port cstart_leave_timer %s",
                  gid_port->ifindex, registrar_states_string[machine->registrar],
                  timers_string[port->cstart_leave_timer]);
    }
  
  rtransition = &registrar_leave_timer_table[machine->registrar];
  machine->registrar = rtransition->new_reg_state;

  port->cstart_leave_timer = port->cstart_leave_timer ||
                             rtransition->cstart_leave_timer;

  new_event = ((rtransition->leave_indication == REG_IND_LI) ? GID_EVENT_LEAVE :
                                                               GID_EVENT_NULL);

  if (GARP_DEBUG(EVENT))
    {
      zlog_debug (garp->garpm, "gid_fsm_leave_timer_expiry: [port %d] "
                  "new registrar state %s leave_indication %s "
                  "port cstart_leave_timer %s new_event %s",
                  gid_port->ifindex, registrar_states_string[machine->registrar],
                  registrar_indications_string[rtransition->leave_indication],
                  timers_string[rtransition->cstart_leave_timer],
                  gid_event_string[new_event]);
    }


  return new_event; 
}


bool_t
gid_fsm_is_machine_active (struct gid_machine *machine)
{
  return ((machine->applicant != APP_STATE_VO) && 
          (machine->registrar != REG_STATE_MT));
}

bool_t
gid_fsm_is_empty_attribute (struct gid_machine *machine)
{
  return ((machine->applicant == APP_STATE_VO) && 
          (machine->registrar == REG_STATE_MT));
}


void
gid_fsm_get_states (struct gid_machine *machine, struct gid_states *state)
{
  state->applicant_state = applicant_state_table[machine->applicant];
  state->applicant_mgmt = applicant_mgmt_table[machine->applicant];
  state->registrar_state = registrar_state_table[machine->registrar];
  state->registrar_mgmt = registrar_mgmt_table[machine->registrar];
}

void
gid_fsm_init_machine (struct gid_machine *machine) 
{
  /* initial state of these machines */
  machine->applicant = APP_STATE_VO;
  machine->registrar = REG_STATE_MT;
}
