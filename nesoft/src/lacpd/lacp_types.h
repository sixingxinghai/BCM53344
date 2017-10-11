/* Copyright 2003-2004  All Rights Reserved. */

#ifndef __LACP_TYPES_H__
#define __LACP_TYPES_H__

#include <thread.h>
#ifdef HAVE_HA
#include "lib_ha.h"
#endif /* HAVE_HA */

#include "lacp_config.h"

/* Generic constants */

#define LACP_IFNAMSIZ       16
#define LACP_GRP_ADDR_LEN   6

#define LACP_SUCCESS        0
#define LACP_FAILURE        -1

/* Table 43B-2 */
#define LACP_TYPE           0x8809

#define LACP_PORT_NUM_MIN 1


/* PDU encoding */

/* 43.4.2.2.d */
#define LACP_SUBTYPE                        0x01
/* 43.4.2.2.e */
#define LACP_VERSION                        0x01
/* 43.4.2.2.f */
#define LACP_ACTOR_INFORMATION_TLV          0x01
/* 43.4.2.2.g */
#define LACP_ACTOR_INFORMATION_LENGTH       0x14
/* 43.4.2.2.o */
#define LACP_PARTNER_INFORMATION_TLV        0x02
/* 43.4.2.2.p */
#define LACP_PARTNER_INFORMATION_LENGTH     0x14
/* 43.4.2.2.o */
#define LACP_COLLECTOR_INFORMATION_TLV      0x03
/* 43.4.2.2.p */
#define LACP_COLLECTOR_INFORMATION_LENGTH   0x10
/* 43.4.2.2.z */
#define LACP_COLLECTOR_MAX_DELAY            0x05
/* 43.4.2.2.ab */
#define LACP_TERMINATOR_TLV                 0x00
/* 43.4.2.2.ac */
#define LACP_TERMINATOR_LENGTH              0x00

/* 43.5.3.2.d */
#define MARKER_SUBTYPE                      0x02
/* 43.5.3.2.e */
#define MARKER_VERSION                      0x01
/* 43.5.3.2.f */
#define MARKER_INFORMATION_TLV              0x01
/* 43.5.3.2.f */
#define MARKER_RESPONSE_TLV                 0x02
/* 43.5.3.2.g */
#define MARKER_INFORMATION_LENGTH           0x10
/* 43.5.3.2.g */
#define MARKER_RESPONSE_LENGTH              0x10
/* 43.5.3.2.g */
#define MARKER_RESPONSE_LENGTH              0x10
/* 43.5.3.2.l */
#define MARKER_TERMINATOR_TLV               0x00
/* 43.5.3.2.m */
#define MARKER_TERMINATOR_LENGTH            0x00

/* State machine states */

enum port_selected_state { UNSELECTED, SELECTED, STANDBY };

enum lacp_rcv_state { RCV_INVALID, RCV_INITIALIZE, RCV_PORT_DISABLED, 
                      RCV_LACP_DISABLED, RCV_EXPIRED, 
                      RCV_DEFAULTED, RCV_CURRENT };

enum lacp_mux_state { MUX_DETACHED, MUX_WAITING, MUX_ATTACHED,
                      MUX_COLLECTING, MUX_DISTRIBUTING, 
                      MUX_COLLECTING_DISTRIBUTING };

enum lacp_periodic_tx_state { PERIODIC_TX_INVALID, 
                              PERIODIC_TX_NO_PERIODIC,
                              PERIODIC_TX_FAST_PERIODIC,
                              PERIODIC_TX_SLOW_PERIODIC,
                              PERIODIC_TX };

enum lacp_actor_churn_state { ACTOR_CHURN_INVALID,
                              ACTOR_CHURN_MONITOR,
                              NO_ACTOR_CHURN,
                              ACTOR_CHURN };

enum lacp_partner_churn_state { PARTNER_CHURN_INVALID,
                                PARTNER_CHURN_MONITOR,
                                NO_PARTNER_CHURN,
                                PARTNER_CHURN };
          
      

/* FLAG manipulation */

#define GET_LACP_ACTIVITY(p)          ((p>>0) & 1)
#define GET_LACP_TIMEOUT(p)           ((p>>1) & 1)
#define GET_AGGREGATION(p)            ((p>>2) & 1)
#define GET_SYNCHRONIZATION(p)        ((p>>3) & 1)
#define GET_COLLECTING(p)             ((p>>4) & 1)
#define GET_DISTRIBUTING(p)           ((p>>5) & 1)
#define GET_DEFAULTED(p)              ((p>>6) & 1)
#define GET_EXPIRED(p)                ((p>>7) & 1)

/* FLAG values - 43.4.2.2.m */

#define LACP_ACTIVITY_PASSIVE             0
#define LACP_ACTIVITY_ACTIVE              1
#define LACP_TIMEOUT_LONG                 0
#define LACP_TIMEOUT_SHORT                1
#define LACP_AGGREGATION_INDIVIDUAL       0
#define LACP_AGGREGATION_AGGREGATABLE     1
#define LACP_SYNCHRONIZATION_OUT_OF_SYNC  0
#define LACP_SYNCHRONIZATION_IN_SYNC      1
#define LACP_COLLECTING_DISABLED          0
#define LACP_COLLECTING_ENABLED           1
#define LACP_DISTRIBUTING_DISABLED        0
#define LACP_DISTRIBUTING_ENABLED         1
#define LACP_NOT_DEFAULTED                0
#define LACP_DEFAULTED                    1
#define LACP_NOT_EXPIRED                  0
#define LACP_EXPIRED                      1

/* Flag manipulation */

#define SET_LACP_ACTIVITY(p)          (p |= (1<<0))
#define SET_LACP_TIMEOUT(p)           (p |= (1<<1))
#define SET_AGGREGATION(p)            (p |= (1<<2))
#define SET_SYNCHRONIZATION(p)        (p |= (1<<3))
#define SET_COLLECTING(p)             (p |= (1<<4))
#define SET_DISTRIBUTING(p)           (p |= (1<<5))
#define SET_DEFAULTED(p)              (p |= (1<<6))
#define SET_EXPIRED(p)                (p |= (1<<7))


#define CLR_LACP_ACTIVITY(p)          (p &= ~(1<<0))
/* Set timeout to "long" */
#define CLR_LACP_TIMEOUT(p)           (p &= ~(1<<1))
#define CLR_AGGREGATION(p)            (p &= ~(1<<2))
#define CLR_SYNCHRONIZATION(p)        (p &= ~(1<<3))
#define CLR_COLLECTING(p)             (p &= ~(1<<4))
#define CLR_DISTRIBUTING(p)           (p &= ~(1<<5))
#define CLR_DEFAULTED(p)              (p &= ~(1<<6))
#define CLR_EXPIRED(p)                (p &= ~(1<<7))

/* System Constants - 43.4.4 */
#define  FAST_PERIODIC_TIME   1
#define  SLOW_PERIODIC_TIME   30
#define  SHORT_TIMEOUT_TIME   3
#define  LONG_TIMEOUT_TIME    90
#define  ACTOR_CHURN_TIME     60
#define  PARTNER_CHURN_TIME   60
#define  AGGREGATE_WAIT_TIME  2


/* Minimum aggregator index */
#define LACP_AGG_IX_MIN_LIMIT         1000000

/* Maximum aggregator index */
#define LACP_AGG_IX_MAX_LIMIT         1000000000

/* Maximum number of aggregators in the system */
#define LACP_AGG_COUNT_MAX            LACP_MAX_LINKS/2

/* Error codes */
#define LACP_ERR_MEM_ALLOC_FAILURE    -100

#define LACP_PORT_PRIORITY_SHIFTS     16

struct lacp_system
{
  u_int16_t system_priority;
  u_char system_id[LACP_GRP_ADDR_LEN];
};

/* This structure contains the decoded information in a received lacp pdu */

struct lacp_pdu
{
  u_char subtype;
  u_char version_number;

  /* Actor information */
  u_int16_t  actor_system_priority;
  u_char   actor_system[LACP_GRP_ADDR_LEN];
  u_char   actor_state;
  u_int16_t  actor_key;
  u_int16_t  actor_port_priority;
  u_int16_t  actor_port_number;
  /* Partner information */
  u_int16_t  partner_system_priority;
  u_char   partner_system[LACP_GRP_ADDR_LEN];
  u_char   partner_state;
  u_int16_t  partner_key;
  u_int16_t  partner_port_priority;
  u_int16_t  partner_port;
  /* Collector information */
  u_int16_t  collector_max_delay;

};

/* This struct contains the decoded information in a received Marker PDU. */
/* See Figure 43-18. */

struct marker_pdu
{
  int response; /* PAL_TRUE if this is a response PDU */
  u_int16_t  port;
  u_char   system[LACP_GRP_ADDR_LEN];
  unsigned int    transaction_id;
};

struct lacp_aggregator
{
  /* List of links attached on this aggregator */
  struct lacp_link *link[LACP_MAX_AGGREGATOR_LINKS];  /* 43.4.6 - LAG_Ports */
  u_char       linkCnt;
  u_char       refCnt;

  /* Administrative name */
  u_char     name[LACP_IFNAMSIZ];
  /* Aggregator variables - 43.4.6 */
  u_int32_t agg_ix;
  u_char     aggregator_mac_address[LACP_GRP_ADDR_LEN];
  u_char     partner_system[LACP_GRP_ADDR_LEN];
  u_int16_t    partner_system_priority;
  u_int16_t    partner_oper_aggregator_key;
  u_int16_t    actor_admin_aggregator_key;
  u_int16_t    actor_oper_aggregator_key;
  u_char     receive_state;  /* Count of mux's in collecting state */
  u_char     transmit_state; /* Count of mux's in distributing state */
  unsigned int      individual_aggregator:1;
  unsigned int      ready:1;    /* 43.4.8 */

  /* Collector information */
  u_int16_t  collector_max_delay;

  struct interface *ifp;

  #define LACP_AGG_FLAG_INSTALLED  (1 << 0)
  #define LACP_HA_AGG_STALE_FLAG   (1 << 1)
  u_char flag;

  /* Index of agg in lacp_instance */
  s_int32_t instance_agg_index;

#ifdef HAVE_HA
  HA_CDR_REF lacp_agg_cdr_ref;
#endif /* HAVE_HA */
};

/* Link variables 43.4.7 */

struct lacp_link
{
  /* Housekeeping */
  struct lacp_link  *       next; /* List of all links. */
  struct lacp_aggregator  * aggregator; /* Who's your daddy? */

  /* Local mac address of the link */
  u_char             mac_addr[LACP_GRP_ADDR_LEN]; 
  u_char             name[LACP_IFNAMSIZ];  /* Administrative name */

  struct lacp_pdu *         pdu;      /* Decoded PDU - Used in place 
                                         of CtrlMuxN:MA_DATA if not null. */
  /* State tracking */
  u_char             rcv_state;
  u_char             periodic_tx_state;
  u_char             mux_machine_state;
  u_char             actor_churn_state;
  u_char             partner_churn_state;

   
  /* Number of times Actor churn state machine
   * has entered ACTOR_CHURN state */
  u_int32_t         actor_churn_count;
  
  /* Number of times Partner churn state machine
   * has entered PARTNER_CHURN state */
  u_int32_t         partner_churn_count;
   
  /* Number of times Actor Mux state 
   * has entered IN_SYNC state */
  u_int32_t         actor_sync_transition_count;
    
  /* Number of times Partner Mux state 
   * has entered IN_SYNC state */
  u_int32_t         partner_sync_transition_count;
      
  /* Timers */
  struct thread *current_while_timer;
  struct thread *actor_churn_timer;
  struct thread *periodic_timer;
  struct thread *partner_churn_timer;
  struct thread *wait_while_timer;
  
  /* Timer expiry flags */
  unsigned int  current_while_timer_expired:1;
  unsigned int  actor_churn_timer_expired:1;
  unsigned int  periodic_timer_expired:1;
  unsigned int  partner_churn_timer_expired:1;

  /* Link variables */
  unsigned int  ntt:1;
  unsigned int  port_enabled:1;
  unsigned int  lacp_enabled:1;     /* 43.4.8 */
  unsigned int  actor_churn:1;      /* 43.4.8 */
  unsigned int  partner_churn:1;    /* 43.4.8 */
  unsigned int  ready_n:1;          /* 43.4.8 */
  unsigned int  selected:2;         /* 43.4.8 */
  unsigned int  port_moved:1;       /* 43.4.8 */

  u_int16_t  actor_port_number;      /* System-wide unique index of port */
  u_int16_t  actor_port_priority;
  unsigned int    actor_port_aggregator_identifier;
  u_int16_t  actor_admin_port_key;
  u_int16_t  actor_oper_port_key;
  u_char   actor_admin_port_state;
  u_char   actor_oper_port_state;
  u_char   partner_admin_system[LACP_GRP_ADDR_LEN];
  u_char   partner_oper_system[LACP_GRP_ADDR_LEN];
  u_int16_t  partner_admin_system_priority;
  u_int16_t  partner_oper_system_priority;
  u_int16_t  partner_admin_key;
  u_int16_t  partner_oper_key;
  u_int16_t  partner_admin_port_number;
  u_int16_t  partner_oper_port_number;
  u_int16_t  partner_admin_port_priority;
  u_int16_t  partner_oper_port_priority;
  u_char   partner_admin_port_state;
  u_char   partner_oper_port_state;
  /* number of times the Partner's perception of
   * the LAG ID (see 43.3.6.1) for this Aggregation Port has changed*/
  u_int16_t partner_change_count;
      
    
  /* Used to track LACPDU transmissions */
  u_int16_t  tx_count;
  /*Used to count no of LACPDUs sent or recieved*/
  u_int32_t lacpdu_sent_count;
  u_int32_t lacpdu_recv_count; 
  u_int32_t mpdu_recv_count;
  u_int32_t mpdu_sent_count;
  u_int32_t pckt_sent_err_count;
  u_int32_t pckt_recv_err_count;
  u_int32_t mpdu_response_recv_count;
  u_int32_t mpdu_response_sent_count;
  u_int32_t pckt_unknown_rx_count;
   
  
  /* Time when the last pdu has been received */
  pal_time_t last_pdu_rx_time;
  struct marker_pdu received_marker_info;

  struct interface *ifp;

  /* Index of link in the aggregator */
  s_int32_t agg_link_index;

#define LINK_FLAG_AGG_MATCH      (1 << 0)
#define LACP_HA_LINK_STALE_FLAG  (1 << 1)
  u_char flags;

#ifdef HAVE_HA
  HA_CDR_REF lacp_link_cdr_ref; /* CDR ref for lacp_link */
  HA_CDR_REF current_while_timer_cdr_ref; /* CDR ref for current_while timer */
  HA_CDR_REF actor_churn_timer_cdr_ref; /* CDR ref for actor_churn_timer */
  HA_CDR_REF periodic_timer_cdr_ref; /* CDR ref for periodic timer */
  HA_CDR_REF partner_churn_timer_cdr_ref; /* CDR ref for partner churn timer */
  HA_CDR_REF wait_while_timer_cdr_ref; /* CDR ref for wait while timer */
#endif /* HAVE_HA */
  
};

/* Global lacp structure */
struct lacp
{
  /* LACP System Identifier */
  struct lacp_system  system_identifier;
  
  /* Local socket connection to forwarder */
  pal_sock_handle_t fwd_socket;

  /* NSM client */
  struct nsm_client *lacp_nc;

  /* Global variable container. */
  struct lib_globals *lacpm;

  /* LACP aggregator base index */
  u_int32_t agg_ix; 
  
  /* Our list of LACP Aggregators  */
  u_int32_t agg_cnt;
  struct lacp_aggregator *lacp_agg_list[LACP_AGG_COUNT_MAX];

  /* List of links used by the selection logic.  */
  struct lacp_link *links[LACP_MAX_LINKS];
  int  link_count;

  /* Transmit timer */
  struct thread *tx_timer;

  /* Lacp read thread */
  struct thread *t_read;
 
  /* number of times the Actor's perception of
   * the LAG ID for this Aggregation Port has changed.*/  
  u_int16_t actor_change_count;
 
  /* Flags */
#define LACP_SYSTEM_ID_ACTIVE       (1 << 0)
  u_char flags;

  /* Configuration Flags */
#define LACP_CONFIG_SYSTEM_PRIORITY   (1 << 0)
  u_char config;
  
  bool_t lacp_begin;

#ifdef HAVE_HA
  HA_CDR_REF lacp_instance_cdr_ref;
  HA_CDR_REF tx_timer_cdr_ref;
#endif /* HAVE_HA */
};

#define LACPM                  (lacp_instance->lacpm)
#define LACP_SYSTEM_PRIORITY   (lacp_instance->system_identifier.system_priority)
#define LACP_SYSTEM_ID         (lacp_instance->system_identifier.system_id)
#define LACP_NSM_CLIENT        (lacp_instance->lacp_nc)
#define LACP_AGG_LIST          (lacp_instance->lacp_agg_list)
#define LACP_LINKS             (lacp_instance->links)

#define IS_LACP_LINK_ENABLED(l) \
((l)->ifp && (l)->port_enabled && ((l)->actor_admin_port_key > 0))

#define LACP_ASSERT pal_assert

#endif /* __LACP_TYPES_H__ */
