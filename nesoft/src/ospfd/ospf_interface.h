/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_INTERFACE_H
#define _PACOS_OSPF_INTERFACE_H

#define OSPF_AUTH_SIMPLE_SIZE           8
#define OSPF_AUTH_MD5_SIZE             16

#define RNI_OSPF_IF     0
#define RNI_CONNECTED   1

/* Macros. */
#define OSPF_IF_PARAM_CHECK(P, T)                                             \
    ((P) && CHECK_FLAG ((P)->config, OSPF_IF_PARAM_ ## T))
#define OSPF_IF_PARAM_SET(P, T)     ((P)->config |= OSPF_IF_PARAM_ ## T)
#define OSPF_IF_PARAM_UNSET(P, T)   ((P)->config &= ~(OSPF_IF_PARAM_ ## T))
#define OSPF_IF_PARAMS_EMPTY(P)     ((P)->config == 0)

#define OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT(P)                               \
    (((P) != NULL                                                             \
      && ((P)->type == OSPF_IFTYPE_NBMA                                       \
          || (P)->type == OSPF_IFTYPE_POINTOMULTIPOINT                        \
          || (P)->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA))                 \
     ? OSPF_HELLO_INTERVAL_NBMA_DEFAULT : OSPF_HELLO_INTERVAL_DEFAULT)

#define OSPF_IF_PARAM_HELLO_INTERVAL_UPDATE(P)                                \
    do {                                                                      \
      if (!OSPF_IF_PARAM_CHECK (P, HELLO_INTERVAL))                           \
        (P)->hello_interval = OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (P);       \
      else if ((P)->hello_interval                                            \
               == OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (P))                   \
        OSPF_IF_PARAM_UNSET (P, HELLO_INTERVAL);                              \
    } while (0)

#define OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT(P)                                \
    ((((P) != NULL && OSPF_IF_PARAM_CHECK ((P), HELLO_INTERVAL))              \
      ? (P)->hello_interval : OSPF_IF_PARAM_HELLO_INTERVAL_DEFAULT (P)) * 4)

#define OSPF_IF_PARAM_DEAD_INTERVAL_UPDATE(P)                                 \
    do {                                                                      \
      if (!OSPF_IF_PARAM_CHECK (P, DEAD_INTERVAL))                            \
        (P)->dead_interval = OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (P);         \
      else if ((P)->dead_interval == OSPF_IF_PARAM_DEAD_INTERVAL_DEFAULT (P)) \
        OSPF_IF_PARAM_UNSET (P, DEAD_INTERVAL);                               \
    } while (0)

struct ospf_if_params
{
  /* Pointer to the OSPF master.  */
  struct ospf_master *om;

  /* Pointer to ospf_if_param_table's desc. */
  char *desc;

  /* Configured flags. */
  u_int32_t config;
#define OSPF_IF_PARAM_NETWORK_TYPE              (1 << 0)
#define OSPF_IF_PARAM_AUTH_TYPE                 (1 << 1)
#define OSPF_IF_PARAM_PRIORITY                  (1 << 2)
#define OSPF_IF_PARAM_OUTPUT_COST               (1 << 3)
#define OSPF_IF_PARAM_TRANSMIT_DELAY            (1 << 4)
#define OSPF_IF_PARAM_RETRANSMIT_INTERVAL       (1 << 5)
#define OSPF_IF_PARAM_HELLO_INTERVAL            (1 << 6)
#define OSPF_IF_PARAM_DEAD_INTERVAL             (1 << 7)
#define OSPF_IF_PARAM_AUTH_SIMPLE               (1 << 8)
#define OSPF_IF_PARAM_AUTH_CRYPT                (1 << 9)
#define OSPF_IF_PARAM_DATABASE_FILTER           (1 << 10)
#ifdef HAVE_OSPF_TE
#define OSPF_IF_PARAM_TE_METRIC                 (1 << 11)
#endif /* HAVE_OSPF_TE */
#define OSPF_IF_PARAM_DISABLE_ALL               (1 << 12)
#define OSPF_IF_PARAM_MTU                       (1 << 13)
#define OSPF_IF_PARAM_MTU_IGNORE                (1 << 14)
#define OSPF_IF_PARAM_RESYNC_TIMEOUT            (1 << 15)
#ifdef HAVE_BFD
#define OSPF_IF_PARAM_BFD                       (1 << 16)
#define OSPF_IF_PARAM_BFD_DISABLE               (1 << 17)
#endif /* HAVE_BFD */

  /* Interface type. */
  u_char type;

  /* Authentication type. */
  u_char auth_type;

  /* Router priority. */
  u_char priority;

  /* MTU size. */
  u_int16_t mtu;

  /* Interface output cost. */
  u_int32_t output_cost;

#ifdef HAVE_OSPF_TE
  /* TE metric. */
  u_int32_t te_metric;
#endif /* HAVE_OSPF_TE */

  /* Interface transmit delay. */
  u_int32_t transmit_delay;

  /* Retransmit interval. */
  u_int32_t retransmit_interval;

  /* Hello interval. */
  u_int32_t hello_interval;

  /* Router dead interval. */
  u_int32_t dead_interval;

  /* Resync timeout. */
  u_int32_t resync_timeout;

  /* Password for simple. */
  char *auth_simple;

  /* Password list for crypt. */
  struct list *auth_crypt;
};

struct ospf_crypt_key
{
  u_char flags;
#define OSPF_AUTH_MD5_KEY_PASSIVE   (1 << 0)
  
  u_char key_id;
  char auth_key[OSPF_AUTH_MD5_SIZE + 1];

  struct thread *t_passive;                     /* key passive timer */

  /* Pointer to the OSPF master.  */
  struct ospf_master *om;
};

/* OSPF identity for DR Election. */
struct ospf_identity
{
  /* IPv4 prefix for the router. */
  struct prefix *address;

  /* Router ID. */
  struct pal_in4_addr router_id;

  /* Router ID for DR. */
  struct pal_in4_addr d_router;

  /* Router ID for Backup. */
  struct pal_in4_addr bd_router;

  /* Router Priority. */
  u_char priority;
};

/* OSPF interface structure */
struct ospf_interface
{
  /* Parent's OSPF instance. */
  struct ospf *top;

  /* Link information. */
  union
  {
    struct interface *ifp;              /* NSM. */
    struct ospf_vlink *vlink;           /* Virtual-Link. */
  } u;

  /* OSPF Area. */
  struct ospf_area *area;

  /* Destination prefix for PPP and virtual link. */
  struct prefix *destination;

  /* Config network. */
  struct ospf_network *network;

  /* Config host entry. */
  struct ospf_host_entry *host;

  /* Packet FIFO.  */
  struct ospf_packet *op_head;
  struct ospf_packet *op_tail;

  /* All of reference count, also lock to remove. */
  u_int16_t lock;

  /* Number of interfaces which have the same network addresses. */
  u_int16_t clone;

#ifdef HAVE_OSPF_MULTI_AREA
  /* Pointer to primary interface */
  struct ospf_interface *oi_primary;
#endif /* HAVE_OSPF_MULTI_AREA */
  /* Interface flags. */
  u_int16_t flags;
#define OSPF_IF_UP                      (1 << 0)
#define OSPF_IF_DESTROY                 (1 << 1)
#define OSPF_IF_WRITE_Q                 (1 << 2)
#define OSPF_IF_PASSIVE                 (1 << 3)
#define OSPF_IF_DR_ELECTION_READY       (1 << 4)
#define OSPF_IF_MC_ALLSPFROUTERS        (1 << 5)
#define OSPF_IF_MC_ALLDROUTERS          (1 << 6)
#ifdef HAVE_RESTART
#define OSPF_IF_GRACE_LSA_ACK_RECD      (1 << 7)
#define OSPF_IF_RS_BIT_SET              (1 << 8)
#endif /* HAVE_RESTART */
#ifdef HAVE_BFD
#define OSPF_IF_BFD                     (1 << 9)
#endif /* HAVE_BFD */
#ifdef HAVE_OSPF_MULTI_AREA
#define OSPF_MULTI_AREA_IF              (1 << 10)
#endif /* HAVE_OSPF_MULTI_AREA */
#ifdef HAVE_OSPF_MULTI_INST     
#define OSPF_IF_EXT_MULTI_INST          (1 << 11)
#endif /* HAVE_SPF_MULTI_INST */
#ifdef HAVE_GMPLS
#define OSPF_IF_GMPLS_TYPE_CONTROL      (1 << 12)
#define OSPF_IF_GMPLS_TYPE_DATA         (1 << 13)
#endif /* HAVE_GMPLS */

  /* OSPF Network Type. */
  u_char type;                          

  /* Interface Grace LSA ACKs count. */
#ifdef HAVE_RESTART
  u_int32_t grace_ack_recv_count;
#endif /* HAVE_RESTART */

  /* IFSM State. */
  u_char state;
  u_char ostate;

  /* Interface Opaque LSDB and Opaque data. */
#ifdef HAVE_OPAQUE_LSA
  struct ospf_lsdb *lsdb;
#endif /* HAVE_OPAQUE_LSA */
  
  /* Configuration parameters. */
  struct ospf_if_params *params;
  struct ospf_if_params *params_default;

  /* Cryptographic Sequence Number. */ 
  u_int32_t crypt_seqnum;               

  /* Interface Output Cost. */
  u_int32_t output_cost;

  /* TE metric. */
#ifdef HAVE_OSPF_TE
  u_int32_t te_metric;
#endif /* HAVE_OSPF_TE */

  /* Neighbor List. */
  struct ls_table *nbrs;

  /* OSPF identity for DR election. */
  struct ospf_identity ident;

  /* List of Link State Update. */
  struct ls_table *ls_upd_queue;

  /* List of Link State Acknowledgment. */
  vector ls_ack;
  struct
  {
    vector ls_ack;
    struct pal_in4_addr dst;
  } direct;

  /* Threads. */
  struct thread *t_hello;               /* Hello timer. */
  struct thread *t_wait;                /* Wait timer. */
  struct thread *t_ls_ack;              /* LS ack timer. */
  struct thread *t_ls_ack_direct;       /* LS ack direct event. */
  struct thread *t_ls_upd_event;        /* LS update event. */
#ifdef HAVE_RESTART 
  struct thread *t_restart_bit_check;   /* RS bit clear timer. */
#endif 
  /* Counters. */
  u_int32_t full_nbr_count;

  /* Statistics. */
  u_int32_t hello_in;           /* Hello packet input count. */
  u_int32_t hello_out;          /* Hello packet output count. */
  u_int32_t db_desc_in;         /* database desc. packet input count. */
  u_int32_t db_desc_out;        /* database desc. packet output count. */
  u_int32_t ls_req_in;          /* LS request packet input count. */
  u_int32_t ls_req_out;         /* LS request packet output count. */
  u_int32_t ls_upd_in;          /* LS update packet input count. */
  u_int32_t ls_upd_out;         /* LS update packet output count. */
  u_int32_t ls_ack_in;          /* LS Ack packet input count. */
  u_int32_t ls_ack_out;         /* LS Ack packet output count. */
  u_int32_t discarded;          /* Discarded input count by error. */
  u_int32_t state_change;       /* Number of IFSM state change. */
};

/* Prototypes. */
unsigned int ospf_address_less_ifindex (struct ospf_interface *);

struct ospf_interface *ospf_if_entry_lookup (struct ospf *,
                                             struct pal_in4_addr,
                                             u_int32_t);
struct ospf_interface *ospf_global_if_entry_lookup (struct ospf_master *,
                                                    struct pal_in4_addr,
                                                    u_int32_t);
int ospf_global_if_entry_add (struct ospf_master *, struct pal_in4_addr,
                              u_int32_t, struct ospf_interface *);
#ifdef HAVE_OSPF_MULTI_INST
struct ospf_interface *ospf_if_entry_match (struct ospf *,
                                            struct ospf_interface *);
#endif /* HAVE_OSPF_MULTI_INST */
struct ospf_interface *ospf_area_if_lookup_by_prefix (struct ospf_area *,
                                                      struct prefix *);

int ospf_global_if_entry_add (struct ospf_master *, struct pal_in4_addr ,
                              u_int32_t , struct ospf_interface *);
int ospf_if_entry_insert (struct ospf *, struct ospf_interface *,
                          struct connected *);
void ospf_global_if_host_entry_delete (struct ospf_master *,
                                       struct connected *);

int ospf_if_mtu_get (struct ospf_interface *);
int ospf_if_mtu_ignore_get (struct ospf_interface *);
u_char ospf_if_network_get (struct ospf_interface *);
u_int32_t ospf_if_transmit_delay_get (struct ospf_interface *);
u_int32_t ospf_if_retransmit_interval_get (struct ospf_interface *);
u_int32_t ospf_if_hello_interval_get (struct ospf_interface *);
u_int32_t ospf_if_dead_interval_get (struct ospf_interface *);
int ospf_if_authentication_type_get (struct ospf_interface *);
char *ospf_if_authentication_key_get (struct ospf_interface *);
struct ospf_crypt_key *ospf_if_message_digest_key_get (struct ospf_interface *,
                                                       u_char);
struct ospf_crypt_key *ospf_if_message_digest_key_get_last (struct ospf_interface *);
int ospf_if_database_filter_get (struct ospf_interface *);
int ospf_if_disable_all_get (struct interface *);
u_int32_t ospf_if_resync_timeout_get (struct ospf_interface *);

void ospf_if_priority_apply (struct ospf_master *, struct pal_in4_addr,
                             u_int32_t, int);
void ospf_if_output_cost_update (struct ospf_interface *);
void ospf_if_output_cost_apply (struct ospf_master *,
                                struct pal_in4_addr, u_int32_t);
void ospf_if_output_cost_apply_all (struct ospf *);
void ospf_if_database_filter_apply (struct ospf_master *, struct pal_in4_addr,
                                    u_int32_t);
void ospf_if_nbr_timers_update_by_addr (struct interface *,
                                        struct pal_in4_addr);
void ospf_if_nbr_timers_update (struct interface *);
void ospf_if_nbr_timers_update_by_vlink (struct ospf_vlink *);

void ospf_if_init (struct ospf_interface *, struct connected *,
                   struct ospf_area *);
struct ospf_interface *ospf_if_new (struct ospf *, struct connected *,
                                    struct ospf_area *);
struct ospf_interface *ospf_host_entry_if_new (struct ospf *,
                                               struct connected *,
                                               struct ospf_area *,
                                               struct ospf_host_entry *);

struct ospf_interface *ospf_if_lock (struct ospf_interface *);
void ospf_if_unlock (struct ospf_interface *);
int ospf_if_delete (struct ospf_interface *);

int ospf_auth_type (struct ospf_interface *);
void ospf_if_set_variables (struct ospf_interface *);
void ospf_if_reset_variables (struct ospf_interface *);
int ospf_if_is_up (struct ospf_interface *);
int ospf_if_up (struct ospf_interface *);
int ospf_if_down (struct ospf_interface *);
int ospf_if_update (struct ospf_interface *);
int ospf_if_update_by_params (struct ospf_if_params *);
struct ospf_interface *ospf_if_lookup_by_params (struct ospf_if_params *oip);

struct ls_table *ospf_if_params_table_lookup_by_name (struct ospf_master *,
                                                      char *);
int ospf_if_params_table_cmp (void *, void *);
struct ospf_if_params *ospf_if_params_new (struct ospf_master *);
struct ospf_if_params *ospf_if_params_lookup (struct ospf_master *, char *,
                                              struct ls_prefix *);
struct ospf_if_params *ospf_if_params_lookup_default (struct ospf_master *,
                                                      char *);
struct ospf_if_params *ospf_if_params_get (struct ospf_master *, char *,
                                           struct ls_prefix *);
struct ospf_if_params *ospf_if_params_get_default (struct ospf_master *,
                                                   char *);
void ospf_if_params_delete (struct ospf_master *, char *, struct ls_prefix *);
void ospf_if_params_delete_default (struct ospf_master *, char *);

int ospf_if_params_priority_set (struct ospf_if_params *, u_char);
int ospf_if_params_priority_unset (struct ospf_if_params *);
int ospf_if_params_mtu_set (struct ospf_if_params *, u_int16_t);
int ospf_if_params_mtu_unset (struct ospf_if_params *);
int ospf_if_params_mtu_ignore_set (struct ospf_if_params *);
int ospf_if_params_mtu_ignore_unset (struct ospf_if_params *);
int ospf_if_params_cost_set (struct ospf_if_params *, u_int32_t);
int ospf_if_params_cost_unset (struct ospf_if_params *);
int ospf_if_params_transmit_delay_set (struct ospf_if_params *, u_int32_t);
int ospf_if_params_transmit_delay_unset (struct ospf_if_params *);
int ospf_if_params_retransmit_interval_set (struct ospf_if_params *,
                                            u_int32_t);
int ospf_if_params_retransmit_interval_unset (struct ospf_if_params *);
int ospf_if_params_hello_interval_set (struct ospf_if_params *, u_int32_t);
int ospf_if_params_hello_interval_unset (struct ospf_if_params *);
int ospf_if_params_dead_interval_set (struct ospf_if_params *, u_int32_t);
int ospf_if_params_dead_interval_unset (struct ospf_if_params *);
int ospf_if_params_authentication_type_set (struct ospf_if_params *, u_char);
int ospf_if_params_authentication_type_unset (struct ospf_if_params *);
int ospf_if_params_authentication_key_set (struct ospf_if_params *, char *);
int ospf_if_params_authentication_key_unset (struct ospf_if_params *);
int ospf_if_message_digest_passive_timer (struct thread *);
int ospf_if_params_message_digest_passive_set_all (struct ospf_if_params *);
int ospf_if_params_message_digest_key_set (struct ospf_if_params *, u_char,
                                           char *, struct ospf_master *);
int ospf_if_params_message_digest_key_unset (struct ospf_if_params *,
                                             u_char);
int ospf_if_params_database_filter_set (struct ospf_if_params *);
int ospf_if_params_database_filter_unset (struct ospf_if_params *);
int ospf_if_params_disable_all_set (struct ospf_if_params *);
int ospf_if_params_disable_all_unset (struct ospf_if_params *);
int ospf_if_params_resync_timeout_set (struct ospf_if_params *, u_int32_t);
int ospf_if_params_resync_timeout_unset (struct ospf_if_params *);
#ifdef HAVE_OSPF_TE
int ospf_if_params_te_metric_set (struct ospf_if_params *, u_int32_t);
int ospf_if_params_te_metric_unset (struct ospf_if_params *);
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_BFD
int ospf_if_params_bfd_set (struct ospf_if_params *);
int ospf_if_params_bfd_unset (struct ospf_if_params *);
int ospf_if_params_bfd_disable_set (struct ospf_if_params *);
int ospf_if_params_bfd_disable_unset (struct ospf_if_params *);
#endif /* HAVE_BFD */

void ospf_if_master_init (struct ospf_master *);
void ospf_if_master_finish (struct ospf_master *);

#endif /* _PACOS_OSPF_INTERFACE_H */
