/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_NEIGHBOR_H
#define _PACOS_OSPF_NEIGHBOR_H

#include "ospfd/ospf_packet.h"

struct ospf_link_list;

/* Neighbor Data Structure. */
struct ospf_neighbor
{
  struct ospf_neighbor *next;
  struct ospf_neighbor *prev;

  /* This neighbor's parent ospf interface. */
  struct ospf_interface *oi;

  /* Flags. */
  u_int16_t flags;
#define OSPF_NEIGHBOR_STATIC            (1 << 0)
#define OSPF_NEIGHBOR_INIT              (1 << 1)
#define OSPF_NEIGHBOR_DD_INIT           (1 << 2)
#define OSPF_NEIGHBOR_DD_PENDING        (1 << 3)
#define OSPF_NEIGHBOR_MD5_KEY_ROLLOVER  (1 << 4)
#ifdef HAVE_RESTART
#define OSPF_NEIGHBOR_RESTART           (1 << 5)        /* For helper. */
#define OSPF_NEIGHBOR_RESYNC_CAPABLE    (1 << 6)
#define OSPF_NEIGHBOR_RESYNC_PROCESS    (1 << 7)
#define OSPF_NEIGHBOR_GRACE_ACK_RECD    (1 << 8)
#endif /* HAVE_RESTART */
#ifdef HAVE_BFD
#define OSPF_NEIGHBOR_BFD               (1 << 9)
#endif /* HAVE_BFD */
#define OSPF_NEIGHBOR_DB_SUMMARY_OPT    (1 << 10)

  /* Counters for acks and no of LSAs */
  u_int32_t ack_count;   
  u_int32_t rxmt_count;  

  /* Field for congestion indication */
  u_int32_t is_congested; 

  /* NFSM state. */
  u_char state;
  u_char ostate;
  
  /* Received options and OSPF identity. */
  u_char options;
  struct ospf_identity ident;

#ifdef HAVE_RESTART
  /* Restart signaling. */
  u_int32_t eo_options;
  pal_time_t last_oob;
#endif /* HAVE_RESTART */

  /* Timestamp. */
  pal_time_t ctime;

  /* DD packet recv/sent information. */
  struct
  {
    /* DD bit flags. */
    u_char flags;

    /* DD Sequence Number. */
    u_int32_t seqnum;

    /* Sent information. */
    struct
    {
      /* Last sent DD packet. */
      struct ospf_packet *packet;

      /* Timestamp when last DD packet was sent. */
      struct pal_timeval ts;

    } sent;

    /* Received information. */
    struct ospf_db_desc recv;

  } dd;

  /* Database Summary List. */ 
  struct ospf_lsdb *db_sum; 

  /* Link State Request List. */
  struct ls_table *ls_req;
  struct ospf_ls_request *ls_req_last;

  /* Link State Retransmission List. */
  struct ospf_lsdb *ls_rxmt_list[OSPF_RETRANSMIT_LISTS];

  /* Cryptographic Sequence Number. */
  u_int32_t crypt_seqnum;

  /* Timer values. */
  u_int32_t v_inactivity;
  u_int32_t v_dd_inactivity;
  u_int32_t v_retransmit;
  u_int32_t v_retransmit_max;

#define v_db_desc  v_retransmit
#define v_ls_req   v_retransmit
#define v_ls_upd   v_retransmit

  /* Threads. */
  struct thread *t_inactivity;
  struct thread *t_dd_inactivity;
  struct thread *t_db_desc;
  struct thread *t_db_desc_free;
  struct thread *t_ls_req;
  struct thread *t_ls_upd;
  struct thread *t_ls_ack_cnt;

#ifdef HAVE_BFD
  struct thread *t_bfd_down;
#endif /* HAVE_BFD */

#ifdef HAVE_RESTART
  struct thread *t_restart;
#define t_helper        t_restart       /* For Graceful Restart. */
#define t_resync        t_restart       /* For Restart Signaling. */
#endif /* HAVE_RESTART */
  struct thread *t_events[OSPF_NFSM_EVENT_MAX];
  
  /* Statistics Field */
  u_int32_t state_change;
};

/* OSPF static neighbor structure. */
struct ospf_nbr_static
{
  /* Neighbor IP address. */
  struct pal_in4_addr addr;

  /* OSPF neighbor structure. */
  struct ospf_neighbor *nbr;

  /* Flags. */
  u_char flags;
#define OSPF_NBR_STATIC_PRIORITY        (1 << 0)
#define OSPF_NBR_STATIC_POLL_INTERVAL   (1 << 1)
#define OSPF_NBR_STATIC_COST            (1 << 2)

  /* Row Status. */
  u_char status;

  /* Neighbor priority. */
  u_char priority;

  /* Poll timer value. */
  u_int16_t poll_interval;

  /* Neighbor cost. */
  u_int16_t cost;

  /* Poll timer thread. */
  struct thread *t_poll;
};

/* Macros. */
#define NBR_ADDR(N)     ((N)->ident.address->u.prefix4)
#define NBR_ID(N)       ((N)->ident.router_id)
#define IS_NBR_DECLARED_DR(N)                                                 \
    IPV4_ADDR_SAME (&((N)->ident.address->u.prefix4), &((N)->ident.d_router))
#define IS_NBR_DECLARED_BACKUP(N)                                             \
    IPV4_ADDR_SAME (&((N)->ident.address->u.prefix4), &((N)->ident.bd_router))
#ifdef HAVE_RESTART
#define IS_NBR_FULL(N)                                                        \
    ((N)->state == NFSM_Full                                                  \
     || (CHECK_FLAG ((N)->flags, OSPF_NEIGHBOR_RESYNC_PROCESS)                \
         && ((N)->state == NFSM_ExStart                                       \
             || (N)->state == NFSM_Exchange                                   \
             || (N)->state == NFSM_Loading)))
#else /* HAVE_RESTART */
#define IS_NBR_FULL(N)                                                        \
    ((N)->state == NFSM_Full)
#endif /* HAVE_RESTART */
#define IS_NBR_OPAQUE_CAPABLE(N)                                              \
    (CHECK_FLAG ((N)->options, OSPF_OPTION_O))
#define OSPF_NBR_DD_INCREMENT(N)                                              \
    (CHECK_FLAG ((N)->flags, OSPF_NEIGHBOR_DD_INIT)                           \
     ? (N)->oi->top->dd_count_in++ : (N)->oi->top->dd_count_out++)
#define OSPF_NBR_DD_DECREMENT(N)                                              \
    do {                                                                      \
      (CHECK_FLAG ((N)->flags, OSPF_NEIGHBOR_DD_INIT)                         \
       ? (N)->oi->top->dd_count_in-- : (N)->oi->top->dd_count_out--);         \
      UNSET_FLAG ((N)->flags, OSPF_NEIGHBOR_DD_INIT);                         \
    } while (0)
#define IS_OSPF_NBR_DD_LIMIT(N)                                               \
    (CHECK_FLAG ((N)->flags, OSPF_NEIGHBOR_DD_INIT)                           \
     ? (N)->oi->top->dd_count_in >= (N)->oi->top->max_dd                      \
     : (N)->oi->top->dd_count_out >= (N)->oi->top->max_dd)

/* Prototypes. */
int ospf_nbr_entry_insert (struct ospf *, struct ospf_neighbor *);
int ospf_nbr_entry_delete (struct ospf *, struct ospf_neighbor *);
int ospf_nbr_count_by_state (struct ospf_interface *, int);
int ospf_nbr_count_all (struct ospf_interface *);
int ospf_nbr_count_full (struct ospf_interface *);
#ifdef HAVE_RESTART
int ospf_nbr_count_restart (struct ospf_interface *);
#endif /* HAVE_RESTART */
struct ospf_neighbor *ospf_nbr_lookup_by_addr (struct ls_table *,
                                               struct pal_in4_addr *);
struct ospf_neighbor *ospf_nbr_lookup_by_router_id (struct ls_table *,
                                                    struct pal_in4_addr *);
struct ospf_neighbor *ospf_nbr_lookup_ptop (struct ospf_interface *);
struct ospf_neighbor *ospf_nbr_get (struct ospf_interface *,
                                    struct pal_in4_addr, u_char);
void ospf_nbr_free (struct ospf_neighbor *);
void ospf_nbr_delete (struct ospf_neighbor *);
int ospf_nbr_message_digest_rollover_state_check (struct ospf_interface *);

struct ospf_neighbor *ospf_nbr_add_dd_pending (struct ospf_neighbor *);
struct ospf_neighbor *ospf_nbr_delete_dd_pending (struct ospf_neighbor *);
void ospf_nbr_dd_pending_check (struct ospf *);

void ospf_nbr_static_if_update (struct ospf_interface *);
struct ospf_nbr_static *ospf_nbr_static_lookup (struct ospf *,
                                                struct pal_in4_addr);
struct ospf_nbr_static *ospf_nbr_static_new ();
void ospf_nbr_static_free (struct ospf_nbr_static *);
void ospf_nbr_static_add (struct ospf *, struct ospf_nbr_static *);
void ospf_nbr_static_reset (struct ospf_interface *, struct pal_in4_addr);
void ospf_nbr_static_up (struct ospf_nbr_static *, struct ospf_interface *);
void ospf_nbr_static_delete (struct ospf *, struct ospf_nbr_static *);
void ospf_nbr_static_poll_timer_cancel (struct ospf_neighbor *);
void ospf_nbr_static_cost_apply (struct ospf *, struct ospf_nbr_static *);
void ospf_nbr_static_priority_apply (struct ospf_nbr_static *, u_char);

#endif /* _PACOS_OSPF_NEIGHBOR_H */
