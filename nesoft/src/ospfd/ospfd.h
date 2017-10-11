/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPFD_H
#define _PACOS_OSPFD_H

#include "timeutil.h"
#include "ls_prefix.h"
#include "ls_table.h"
#include "thread.h"
#include "ospf_common/ospf_common.h"
#include "bheap.h"
#ifdef HAVE_OSPF_TE
#include "ospf_common/ospf_te_common.h" 
#ifdef HAVE_OSPF_CSPF
#include "cspf/cspf_common.h"
#endif /* HAVE_OSPF_CSPF */
#endif /* HAVE_OSPF_TE */

/* OSPF version number. */
#define OSPF_VERSION                             2

/* IP TTL for OSPF protocol. */
#define OSPF_IP_TTL                              1
#define OSPF_VLINK_IP_TTL                      255

#ifdef HAVE_VRF_OSPF
/* OSPF Domain-ID type */
#define TYPE_AS_DID                 0x0005      /* AS type domain-id */
#define TYPE_IP_DID                 0x0105      /* IP type domain-id */
#define TYPE_AS4_DID                0x0205      /* AS4 type domain-id */
#define TYPE_BACK_COMP_DID          0x8005      /* backward compatibility domain-id */

#define OSPF_DOMAIN_ID_FLENGTH        8         /* Full length of 8 bytes domain-id value */
#define OSPF_DOMAIN_ID_VLENGTH        6         /* Domian-id length of last 6 bytes */
#define OSPF_IP_DOMAIN_ID_VLENGTH     4         /* IP address format domain-ID value
                                                   length of 4bytes */
#endif /* HAVE_VRF_OSPF */

/* Architectual constants for OSPFv2. */
#define OSPF_LS_INFINITY                  0xffffff

#define OSPFV2_ALLSPFROUTERS            0xE0000005      /* 224.0.0.5. */
#define OSPFV2_ALLDROUTERS              0xE0000006      /* 224.0.0.6. */
#define IPV4_ADDRESS_UNSPEC             0x00000000      /* 0.0.0.0. */
#define IPV4_ADDRESS_LOOPBACK           0x7F000001      /* 127.0.0.1. */

#define OSPF_LSA_MAXAGE_INTERVAL_DEFAULT        10
#define OSPF_LSA_REFRESH_INTERVAL_DEFAULT       10
#define OSPF_EXTERNAL_LSA_ORIGINATE_DELAY        1
#define OSPF_LSA_REFRESH_EVENT_INTERVAL     500000
#ifdef HAVE_OPAQUE_LSA
#define OSPF_OPAQUE_LSA_GENERATE_DELAY           2
#endif /* HAVE_OPAQUE_LSA */

/* OSPF Authentication Type. */
#define OSPF_AUTH_NULL                           0
#define OSPF_AUTH_SIMPLE                         1
#define OSPF_AUTH_CRYPTOGRAPHIC                  2

#ifdef HAVE_OSPF_MULTI_INST
/* OSPF Interface instance ID */
#define OSPF_IF_INSTANCE_ID_DEFAULT              0
#endif /* HAVE_OSPF_MULTI_INST */
#define OSPF_IF_INSTANCE_ID_UNUSE              256

/* Table definition. */
#define OSPF_GLOBAL_IF_TABLE_DEPTH               8

/* OSPF options. */
#define OSPF_OPTION_T                   (1 << 0)        /* TOS. */
#define OSPF_OPTION_E                   (1 << 1)
#define OSPF_OPTION_MC                  (1 << 2)
#define OSPF_OPTION_NP                  (1 << 3)
#define OSPF_OPTION_EA                  (1 << 4)
#define OSPF_OPTION_DC                  (1 << 5)
#define OSPF_OPTION_O                   (1 << 6)
#define OSPF_OPTION_L                   OSPF_OPTION_EA  /* LLS. */
#define OSPF_OPTION_DN                  (1 << 7)        /* OSPF Down-bit */

/* OSPF Database Description flags. */
#define OSPF_DD_FLAG_MS                 (1 << 0)
#define OSPF_DD_FLAG_M                  (1 << 1)
#define OSPF_DD_FLAG_I                  (1 << 2)
#define OSPF_DD_FLAG_R                  (1 << 3)
#define OSPF_DD_FLAG_ALL                                                      \
    (OSPF_DD_FLAG_MS|OSPF_DD_FLAG_M|OSPF_DD_FLAG_I)

/* OSPF Process ID constant. */
#define OSPF_PROCESS_ID_ANY                     -1
#define OSPF_PROCESS_ID_SINGLE                   0
#define OSPF_PROCESS_ID_MIN                      0
#define OSPF_PROCESS_ID_MAX                  65535

#define OSPF_LS_REFRESH_SHIFT            (60 * 15)
#define OSPF_LS_REFRESH_JITTER                  60
#define OSPF_LS_REFRESH_INITIAL_JITTER         700/* Should be less than 740.*/

#define OSPF_GC_TIMER_INTERVAL                1800

#ifdef HAVE_VRF
#define OSPF_VRF_ID_MAX           VRF_ID_MAX
#else /* HAVE_VRF */
#define OSPF_VRF_ID_MAX                    1
#endif /* HAVE_VRF */

#ifdef HAVE_SNMP
#define OSPF_TRAP_ALL                      0
#define OSPF_TRAP_ID_MIN                   1
#define OSPF_TRAP_ID_MAX                   16
#define OSPF_TRAP_VEC_MIN_SIZE             1
#define OSPF_MAX_RETX_TRAPS                5
#endif /* HAVE_SNMP */

#define OSPF_AREA_CONF_CHECK(A, T)                                            \
    (CHECK_FLAG ((A)->conf.config, OSPF_AREA_CONF_ ## T))

#define OSPF_AREA_CONF_SET(A, T)                                              \
    do {                                                                      \
      if (!OSPF_AREA_CONF_CHECK (A, T))                                       \
        {                                                                     \
          ((A)->conf.config |= OSPF_AREA_CONF_ ## T);                         \
          ospf_area_lock ((A));                                               \
        }                                                                     \
    } while (0)

#define OSPF_AREA_CONF_UNSET(A, T)                                            \
    do {                                                                      \
      if ((A) && OSPF_AREA_CONF_CHECK (A, T))                                 \
        {                                                                     \
          ((A)->conf.config &= ~(OSPF_AREA_CONF_ ## T));                      \
          (A) = ospf_area_unlock ((A));                                       \
        }                                                                     \
    } while (0)

#define OSPF_AREA_CONF_UNSET2(A, T)                                           \
    do {                                                                      \
      if ((A) && OSPF_AREA_CONF_CHECK (A, T))                                 \
        {                                                                     \
          ((A)->conf.config &= ~(OSPF_AREA_CONF_ ## T));                      \
          (void) ospf_area_unlock ((A));                                      \
        }                                                                     \
    } while (0)

/* OSPF area parameter. */
struct ospf_area_conf
{
  /* Configuration flags.  */
  u_int16_t config;
#define OSPF_AREA_CONF_EXTERNAL_ROUTING   (1 << 0) /* external routing. */
#define OSPF_AREA_CONF_NO_SUMMARY         (1 << 1) /* stub no import summary.*/
#define OSPF_AREA_CONF_DEFAULT_COST       (1 << 2) /* stub default cost. */
#define OSPF_AREA_CONF_AUTH_TYPE          (1 << 3) /* Area auth type. */
#define OSPF_AREA_CONF_NSSA_TRANSLATOR    (1 << 4) /* NSSA translate role. */
#define OSPF_AREA_CONF_STABILITY_INTERVAL (1 << 5) /* NSSA StabilityInterval.*/
#define OSPF_AREA_CONF_NO_REDISTRIBUTION  (1 << 6) /* NSSA redistribution. */
#define OSPF_AREA_CONF_DEFAULT_ORIGINATE  (1 << 7) /* NSSA default originate.*/
#define OSPF_AREA_CONF_METRIC             (1 << 8) /* NSSA default metric. */
#define OSPF_AREA_CONF_METRIC_TYPE        (1 << 9) /* NSSA default mtype. */
#define OSPF_AREA_CONF_SHORTCUT_ABR       (1 << 10)/* Shortcut ABR. */
#define OSPF_AREA_CONF_ACCESS_LIST_IN     (1 << 11)/* Access-List in. */
#define OSPF_AREA_CONF_ACCESS_LIST_OUT    (1 << 12)/* Access-List out. */
#define OSPF_AREA_CONF_PREFIX_LIST_IN     (1 << 13)/* Prefix-List in. */
#define OSPF_AREA_CONF_PREFIX_LIST_OUT    (1 << 14)/* Prefix-List out. */
#define OSPF_AREA_CONF_SNMP_CREATE        (1 << 15)/* ospfAreaStatus.  */

  /* Area ID format -- This parameter is not flaged. */
  u_char format;

  /* External routing capability. */
  u_char external_routing;

  /* Authentication type. */
  u_char auth_type;
#define OSPF_AREA_AUTH_NULL             OSPF_AUTH_NULL
#define OSPF_AREA_AUTH_SIMPLE           OSPF_AUTH_SIMPLE
#define OSPF_AREA_AUTH_CRYPTOGRAPHIC    OSPF_AUTH_CRYPTOGRAPHIC

  /* StubDefaultCost. */
  u_int32_t default_cost;

  /* Area configured as shortcut. */
  u_char shortcut;
#define OSPF_SHORTCUT_DEFAULT           0
#define OSPF_SHORTCUT_ENABLE            1
#define OSPF_SHORTCUT_DISABLE           2

#ifdef HAVE_NSSA
  /* NSSA Role during configuration. */
  u_char nssa_translator_role;
#define OSPF_NSSA_TRANSLATE_NEVER       0
#define OSPF_NSSA_TRANSLATE_ALWAYS      1
#define OSPF_NSSA_TRANSLATE_CANDIDATE   2

  /* NSSA default-information originate metric-type. */
  u_char metric_type;
#define OSPF_NSSA_METRIC_TYPE_DEFAULT   EXTERNAL_METRIC_TYPE_2

  /* NSSA default-information originate metric. */
  u_int32_t metric;
#define OSPF_NSSA_METRIC_DEFAULT        1

  /* NSSA TranslatorStabilityInterval */
  u_int16_t nssa_stability_interval;
#define OSPF_TIMER_NSSA_STABILITY_INTERVAL      40

  /* Default information originate config */
  u_int32_t default_config;
#endif /* HAVE_NSSA */
};

/* OSPF Area filter structure. */
struct ospf_area_filter
{
  /* Access-list. */
  struct 
  {
    /* Name of access-list. */
    char *name;

    /* Pointer to access-list. */
    struct access_list *list;

  } alist[FILTER_MAX];
#define ALIST_NAME(A,T)         (A)->filter.alist[(T)].name
#define ALIST_LIST(A,T)         (A)->filter.alist[(T)].list

  /* Prefix-list. */
  struct
  {
    /* Name of prefix-list. */
    char *name;

    /* Pointer to prefix-list. */
    struct prefix_list *list;

  } plist[FILTER_MAX];
#define PLIST_NAME(A,T)         (A)->filter.plist[(T)].name
#define PLIST_LIST(A,T)         (A)->filter.plist[(T)].list
};

/* OSPF area structure. */
struct ospf_area
{
  /* Area ID. */
  struct pal_in4_addr area_id;

  /* Administrative flags. */
  u_char flags;
  u_char oflags;
#define OSPF_AREA_UP                 (1 << 0)
#define OSPF_AREA_TRANSIT            (1 << 1)   /* TransitCapability. */
#define OSPF_AREA_SHORTCUT           (1 << 2)   /* Other ABR agree on S-bit. */

  /* Row Status. */
  u_char status;

  /* For better packing the 4 char are togather */
#ifdef HAVE_NSSA
  /* NSSA TranslatorState. */
  u_char translator_state;
#define OSPF_NSSA_TRANSLATOR_DISABLED   0
#define OSPF_NSSA_TRANSLATOR_ENABLED    1
#define OSPF_NSSA_TRANSLATOR_ELECTED    2
 
 /* Uncondiftional LSA translation */
 u_char translator_flag;
#define OSPF_NSSA_TRANSLATOR_UNCONDL   (1 << 0)
#endif /* HAVE_NSSA */

  /* OSPF instance. */
  struct ospf *top;

  /* OSPF interface list belonging to the area. */
  struct list *iflist;

  /* Link vector. */
  vector link_vec;

  /* Lock count. */
  int lock; 

  /* Area incremental defer */
  bool_t incr_defer;   
  int incr_count;

  /* Area config. */
  struct ospf_area_conf conf;

  /* Stub default. */
  struct ospf_ia_lsa_map *stub_default;

  /* Area filter structure. */
  struct ospf_area_filter filter;

  /* Redistribute timer argument. */
  int redist_update;

  /* Tables. */
  struct ls_table *ranges;              /* Area ranges. */
  struct ls_table *aggregate_table;     /* API table. */
  struct ls_table *ia_prefix;           /* Summary Prefix. */
  struct ls_table *ia_router;           /* Summary router. */
#ifdef HAVE_NSSA
  struct ls_table *redist_table;        /* Redist table. */
#endif /* HAVE_NSSA */

  /* Area related LSDBs. */
  struct ospf_lsdb *lsdb;

#define MAXELEMENTS 1000
  /* Shortest Path Tree. */
  struct ospf_vertex *spf;
  struct ls_table *vertices;            /* SPF vertices. */
  struct bin_heap *candidate;

  /* Route calculation.  */
  struct ospf_route_calc *rt_calc;

  /* LSA vectors.  */
  vector lsa_incremental;

  /* Threads. */
  struct thread *t_spf_calc;            /* SPF calculation timer. */
  struct thread *t_ia_calc_prefix;      /* IA summary LSA thread.  */
  struct thread *t_ia_calc_router;      /* IA ASBR summary LSA thread.  */
  struct thread *t_lsa_incremental;     /* LSA incremental thread.  */
  struct thread *t_ia_inc_calc;
#ifdef HAVE_NSSA
  struct thread *t_nssa;                /* NSSA translate/stability timer. */
  struct thread *t_nssa_calc;           /* NSSA external calculation event.  */
  struct thread *t_redist;              /* NSSA redistribute timer. */
#endif /* HAVE_NSSA */

  /* Area suspend handling.  */
  struct thread *t_suspend;             /* Suspended thread.  */
  struct thread_list event_pend;        /* Pending events.  */
  u_char spf_re_run;                    /* Re-schedule SPF calculation */

  /* Time stamp. */
  struct pal_timeval tv_spf;            /* SPF calculation. */
  struct pal_timeval tv_spf_curr;       /* Current SPF calculation. */
  struct pal_timeval tv_redist;         /* Redistribution. */

  /* Counters. */
  u_char    abr_count;                  /* ABR router in this area. */
  u_char    asbr_count;                 /* ASBR router in this area. */
  u_int16_t active_if_count;            /* Active interfaces. */
  u_int16_t vlink_count;                /* VLINK interfaces. */
  u_int16_t full_virt_nbr_count;        /* Fully adjacent virtual neighbors. */
  u_int32_t full_nbr_count;             /* Fully adjacent neighbors. */

  /* Statistics field. */
  u_int32_t spf_calc_count;             /* SPF Calculation Count. */
};

/* OSPF config network structure. */
struct ospf_network
{
  /* Area ID. */
  struct pal_in4_addr area_id;

  /* Network prefix. */
  struct ls_prefix *lp;

  /* Area ID format. */
  u_char format;

#ifdef HAVE_OSPF_MULTI_INST
  /* Instance ID */
  u_int8_t inst_id;
#endif /* HAVE_OSPF_MULTI_INST */

  /* Wildcard mask flag. */
  u_char wildcard;
#define OSPF_NETWORK_MASK_DEFAULT    0
#define OSPF_NETWORK_MASK_DECIMAL    1
#define OSPF_NETWORK_MASK_WILD       2
};

/* Passive interface. */
struct ospf_passive_if
{
  char *name;
  struct pal_in4_addr addr;
};

/* OSPF host entry. */
struct ospf_host_entry
{
  /* Host address. */
  struct pal_in4_addr addr;

  /* Host metric. */
  u_int16_t metric;

  /* TOS. */
  u_char tos;

  /* Area ID format. */
  u_char format;

  /* Row status. */
  u_char status;

  /* OSPF Area ID. */
  struct pal_in4_addr area_id;
};

/* Configuration redistribute. */
struct ospf_redist_conf
{
  /* Flags. */
  u_int32_t flags:8;
#define OSPF_REDIST_ENABLE              (1 << 0)
#define OSPF_REDIST_METRIC_TYPE         (1 << 1)
#define OSPF_REDIST_METRIC              (1 << 2)
#define OSPF_REDIST_ROUTE_MAP           (1 << 3)
#define OSPF_REDIST_TAG                 (1 << 4)
#define OSPF_REDIST_FILTER_LIST         (1 << 5)
#define OSPF_DIST_LIST_IN               (1 << 6)

  /* Metric value (24-bit). */
  u_int32_t metric:24;

  /* Route tag. */
  u_int32_t tag;

  /* Route-map. */
  struct
  {
    char *name;
    struct route_map *map;

  } route_map;

  /* Distribute-list. */
  struct
  {
    char *name;
    struct access_list *list;

  } distribute_list;

  /* Instance ID (20 bit)*/
  u_int32_t pid:20;
 
  struct ospf_redist_conf *next;
  struct ospf_redist_conf *prev;
 
};

#define METRIC_TYPE(C)          (CHECK_FLAG ((C)->flags,                      \
                                             OSPF_REDIST_METRIC_TYPE) ?       \
                                             EXTERNAL_METRIC_TYPE_1 :         \
                                             EXTERNAL_METRIC_TYPE_2)
#define METRIC_TYPE_SET(C,T)                                                  \
    do {                                                                      \
      if ((T) == EXTERNAL_METRIC_TYPE_1)                                      \
        SET_FLAG ((C)->flags, OSPF_REDIST_METRIC_TYPE);                       \
      else if ((T) == EXTERNAL_METRIC_TYPE_2)                                 \
        UNSET_FLAG ((C)->flags, OSPF_REDIST_METRIC_TYPE);                     \
    } while (0)
#define METRIC_VALUE(C)         ((C)->metric)
#define REDIST_TAG(C)           ((C)->tag)
#define DIST_NAME(C)            ((C)->distribute_list.name)
#define DIST_LIST(C)            ((C)->distribute_list.list)
#define RMAP_NAME(C)            ((C)->route_map.name)
#define RMAP_MAP(C)             ((C)->route_map.map)
#define INST_VALUE(C)           ((C)->pid)
#define REDIST_PROTO_SET(C)                                                   \
    (SET_FLAG ((C)->flags, OSPF_REDIST_ENABLE))
#define REDIST_PROTO_UNSET(C)                                                 \
    (UNSET_FLAG ((C)->flags, OSPF_REDIST_ENABLE))
#define REDIST_PROTO_CHECK(C)                                                 \
    (CHECK_FLAG ((C)->flags, OSPF_REDIST_ENABLE))
#define REDIST_PROTO_NSSA_CHECK(A, T)                                         \
    ((T) == APN_ROUTE_DEFAULT ? OSPF_AREA_CONF_CHECK ((A), DEFAULT_ORIGINATE) \
     : (REDIST_PROTO_CHECK (&(A)->top->redist[(T)])                           \
        && !OSPF_AREA_CONF_CHECK ((A), NO_REDISTRIBUTION)))

/* AS extenral route calculation arguments. */
struct ospf_ase_calc_arg
{
  u_char type;
  u_int16_t max_count;
  struct pal_in4_addr area_id;
  struct ospf_lsa *lsa;
};

/* OSPF instance structure. */
struct ospf
{
  /* OSPF process ID. */
  u_int16_t ospf_id;
  
  /* OSPF start time. */
  pal_time_t start_time;

  /* Pointer to OSPF master. */
  struct ospf_master *om;

  /* VRF vinding. */
  struct ospf_vrf *ov;

  /* OSPF Router ID. */
  struct pal_in4_addr router_id;                /* OSPF Router-ID. */
  struct pal_in4_addr router_id_config;         /* Router-ID configured. */

#ifdef HAVE_VRF_OSPF
  /* OSPF Domain ID */
  struct list *domainid_list;
  
  /* primary domain ID */
  struct ospf_vrf_domain_id *pdomain_id;       /* Primary domain-id configured. */
#endif /* HAVE_VRF_OSPF */   

  bool_t incr_defer;
  int tot_incr;

  /* Administrative flags. */
  u_int16_t flags;
#define OSPF_PROC_UP                    (1 << 0)
#define OSPF_PROC_DESTROY               (1 << 1)
#define OSPF_ROUTER_ABR                 (1 << 2)
#define OSPF_ROUTER_ASBR                (1 << 3)
#define OSPF_ROUTER_DB_OVERFLOW         (1 << 4)
#define OSPF_ROUTER_RESTART             (1 << 5)
#define OSPF_LSDB_EXCEED_OVERFLOW_LIMIT (1 << 6)
#define OSPF_ASE_CALC_SUSPENDED         (1 << 7)
#define OSPF_GRACE_LSA_ACK_RECD         (1 << 8)

  /* Configuration variables. */
  u_int16_t config;
#define OSPF_CONFIG_ROUTER_ID           (1 << 0)
#define OSPF_CONFIG_DEFAULT_METRIC      (1 << 1)
#define OSPF_CONFIG_MAX_CONCURRENT_DD   (1 << 2)
#define OSPF_CONFIG_RFC1583_COMPATIBLE  (1 << 3)
#define OSPF_CONFIG_OPAQUE_LSA          (1 << 4)
#define OSPF_CONFIG_TRAFFIC_ENGINEERING (1 << 5)
#define OSPF_CONFIG_RESTART_METHOD      (1 << 6)
#define OSPF_CONFIG_OVERFLOW_LSDB_LIMIT (1 << 7)
#define OSPF_CONFIG_ROUTER_ID_USE       (1 << 8)
#ifdef HAVE_BFD
#define OSPF_CONFIG_BFD                 (1 << 9)
#endif /* HAVE_BFD */
#ifdef HAVE_VRF_OSPF
#define OSPF_CONFIG_DOMAIN_ID_SEC       (1 << 10)
#define OSPF_CONFIG_DOMAIN_ID_PRIMARY   (1 << 11)
#define OSPF_CONFIG_NULL_DOMAIN_ID      (1 << 12)
#endif /*HAVVE_VRF_OSPF */
#define OSPF_CONFIG_DB_SUMMARY_OPT      (1 << 13)

  /* ABR type. */
  u_char abr_type;

  /* Default information originate. */
  u_char default_origin;
#define OSPF_DEFAULT_ORIGINATE_UNSPEC   0
#define OSPF_DEFAULT_ORIGINATE_NSM      1
#define OSPF_DEFAULT_ORIGINATE_ALWAYS   2

  /* SPF timer config. */
  struct pal_timeval spf_min_delay;         /* SPF minimum delay time. */
  struct pal_timeval spf_max_delay;         /* SPF maximum delay time. */
#define OSPF_SPF_INCREMENT_VALUE        5

  /* Reference bandwidth (Kbps).  */
  u_int32_t ref_bandwidth;

  /* Max concurrent DD. */
  u_int16_t max_dd;                     /* Maximum concurrent DD.  */

#ifdef HAVE_RESTART
  u_char restart_method;
#define OSPF_RESTART_METHOD_NEVER       0
#define OSPF_RESTART_METHOD_GRACEFUL    1
#define OSPF_RESTART_METHOD_SIGNALING   2
#define OSPF_RESTART_METHOD_DEFAULT     OSPF_RESTART_METHOD_GRACEFUL
#define OSPF_RESTART_GRACE_ACK_TIME     2
#endif /* HAVE_RESTART */

#define OSPF_MAX_CONCURRENT_DD_DEFAULT  5

  /* Configuration tables. */
  struct ls_table *networks;            /* Network area tables. */
  struct ls_table *summary;             /* Address range for external-LSAs. */
  struct ls_table *nbr_static;          /* Static neighbor for NBMA. */
  struct list *passive_if;              /* Pasive interfaces. */
  struct list *no_passive_if;           /* No passive interfaces */

  /* Variable to track default option for  passive interface is set or not. */
  bool_t passive_if_default;

  /* Redistriribute configuration. */
  struct ospf_redist_conf redist[APN_ROUTE_MAX];
  struct ospf_redist_conf dist_in;

  /* Redistribute default metric. */
  u_int32_t default_metric;

  /* Redistribute timer argument. */
  int redist_update;

  /* OSPF specific object tables. */
  struct ls_table *area_table;          /* Area table. */
  struct ls_table *if_table;            /* Interface table. */
  struct ls_table *nexthop_table;       /* Nexthop table. */
  struct ls_table *redist_table;        /* Redistribute map table. */
  struct ls_table *vlink_table;         /* Virtual interface table. */
#ifdef HAVE_OSPF_MULTI_AREA
  struct ls_table *multi_area_link_table; /* Multi area interface table. */
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Pointer to the Backbone Area. */
  struct ospf_area *backbone;

  /* LSDB of AS-external-LSAs. */
  struct ospf_lsdb *lsdb;
  
#ifdef HAVE_OSPF_CSPF 
  /* CSPF instance.  */
  struct cspf *cspf;
#endif /* HAVE_OSPF_CSPF */

#ifdef HAVE_OSPF_DB_OVERFLOW
  /* Database overflow stuff */
  int ext_lsdb_limit;
#define OSPF_DEFAULT_LSDB_LIMIT                 -1

  u_int16_t exit_overflow_interval;
#define OSPF_DEFAULT_EXIT_OVERFLOW_INTERVAL     0

#define IS_DB_OVERFLOW(O)       CHECK_FLAG((O)->flags, OSPF_ROUTER_DB_OVERFLOW)
#endif /* HAVE_OSPF_DB_OVERFLOW */

  /* Limit of number of LSAs */
  u_int32_t lsdb_overflow_limit;        
  int lsdb_overflow_limit_type;         /* Soft or hard limit. */
#define OSPF_LSDB_OVERFLOW_LIMIT_OSCILATE_RANGE         0.95
#define OSPF_LSDB_OVERFLOW_SOFT_LIMIT   1  
#define OSPF_LSDB_OVERFLOW_HARD_LIMIT   2
  
#define OSPF_AREA_LIMIT_DEFAULT                 (~0 - 1)
  u_int32_t max_area_limit;

  /* Routing tables. */
  struct ls_table *rt_asbr;             /* ASBR routing table. */
  struct ls_table *rt_network;          /* IP routing table. */
  struct ls_node *rt_next;              /* candidate for Nextroute calculation*/



  /* Threads. */
  struct thread *t_ase_inc_calc;         /* ASE incremental defer timer */
  struct thread *t_ase_calc;            /* ASE calculation timer. */
  struct thread *t_redist;              /* Redistribute timer. */
  struct thread *t_lsa_event;           /* LS refresh event timer. */
  struct thread *t_lsa_walker;          /* LS refresh pereodic timer. */
  struct thread *t_lsa_originate;       /* LSA originate event.  */
#ifdef HAVE_NSSA
  struct thread *t_nssa_inc_calc;       /* NSSA incremental defer timer */
#endif /* HAVE_NSSA */
#ifdef HAVE_RESTART
  struct thread *t_restart_state;       /* Restart State check timer. */
  struct thread *t_grace_ack_event;     /* Grace LSA Ack timer. */
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_DB_OVERFLOW
  struct thread *t_overflow_exit;       /* DB overflow exit timer. */
#endif /* HAVE_OSPF_DB_OVERFLOW */
  struct thread *t_lsdb_overflow_event; /* LSDB overflow shut down event. */
  struct thread *t_gc;                  /* Gargabe collector. */
  struct thread *t_read;
  struct thread *t_write;

  /* Buffers and unuse list. */
  struct stream *ibuf;                  /* Buffer for receiving packets. */
  struct stream *obuf;                  /* Buffer for sending packets. */
  struct stream *lbuf;                  /* Buffer for building LSAs. */

  /* Pakcet unused list.  */
  struct ospf_packet *op_unuse_head;    /* Packet unused list head.  */
  struct ospf_packet *op_unuse_tail;    /* Packet unused list tail.  */
  u_int16_t op_unuse_count;             /* Pakcet unused count.  */
  u_int16_t op_unuse_max;               /* Packet unused maximum number.  */
#define OSPF_PACKET_UNUSE_MAX_DEFAULT   200

  /* LSA unused list.  */
  struct ospf_lsa *lsa_unuse_head;      /* LSA unused list head.  */
  struct ospf_lsa *lsa_unuse_tail;      /* LSA unused list tail.  */
  u_int16_t lsa_unuse_count;            /* LSA unused count.  */
  u_int16_t lsa_unuse_max;              /* LSA unused maximum number.  */
#define OSPF_LSA_UNUSE_MAX_DEFAULT      200

  /* OSPF socket for read/write. */
  int fd;

  /* Interface list of queue. */
  struct list *oi_write_q;
  
  /* Distance parameter. */
  struct ls_table *distance_table;
  u_char distance_all;
  u_char distance_intra;
  u_char distance_inter;
  u_char distance_external;

  /* MaxAge timer value. */
  u_int16_t lsa_maxage_interval;

  /* LSA refresh walker. */
  struct
  {
    /* Current index. */
    u_char index;

    /* Number of rows. */
    u_char rows;

    /* Refresh interval. */
    u_int16_t interval;

    /* Refresh queue vector. */
    vector vec;

  } lsa_refresh;

  /* LSA event vectors.  */
  vector lsa_event;
  vector lsa_originate;

  /* Time stamp. */
  struct pal_timeval tv_redist;

  /* OSPF hello timer jitter seed.  */
  u_int32_t jitter_seed;

  /* Concurrent DD handling.  */
  struct ospf_neighbor *dd_nbr_head;
  struct ospf_neighbor *dd_nbr_tail;
  u_int16_t dd_count_in;                /* Incomming DD neighbors.  */
  u_int16_t dd_count_out;               /* Outgoing DD neighbors.  */

  /* Counters.  */
#ifdef HAVE_NSSA
  u_int32_t nssa_count;                 /* NSSA attached. */
  bool_t incr_n_defer;                  /* NSSA incremental defer */
  int tot_n_incr;                       /* NSSA incremental defer count */ 
#endif /* HAVE_NSSA */

  /* Statistics. */
  u_int32_t lsa_originate_count;        /* LSA origination. */
  u_int32_t rx_lsa_count;               /* LSA used for new instantiation. */

  /* API related tables. */
  struct ls_table *stub_table;          /* OSPF stub area table. */
  struct ls_table *lsdb_table;          /* OSPF LSDB upper table. */
  struct ls_table *host_table;          /* OSPF Host table. */
  struct ls_table *area_range_table;    /* OSPF Area Range table. */
  struct ls_table *nbr_table;           /* OSPF Neighbor table. */

  /* Current time for avoiding system call many times.  */
  struct pal_timeval tv_current;
 
  struct thread *t_route_cleanup;
};

struct debug_ospf
{
  unsigned long packet[6];
  unsigned long event;
  unsigned long ifsm;
  unsigned long nfsm;
  unsigned long lsa;
  unsigned long nsm;
  unsigned long bfd;
  unsigned long rt_calc;
};

/* OSPF master for system wide configurations and variables. */
struct ospf_master
{
  /* Pointer to VR. */
  struct apn_vr *vr;

  /* Pointer to globals.  */
  struct lib_globals *zg;

  /* OSPF instance list. */
  struct list *ospf;

#ifdef HAVE_OSPF_CSPF
  /* CSPF instance. */
  struct cspf *cspf;
  struct cspf_debug_flags debug_cspf;
#endif /* HAVE_OSPF_CSPF */

  /* OSPF debug flags. */
  struct
  {
    /* Debug flags for configuration. */
    struct debug_ospf conf;

    /* Debug flags for terminal. */
    struct debug_ospf term;

  } debug;

  /* Virtual interface index. */
  int vlink_index;

  /* OSPF global flags. */
  u_char flags;
#define OSPF_GLOBAL_FLAG_RESTART                        (1 << 0)
#ifdef HAVE_OSPF_MULTI_INST
#define OSPF_EXT_MULTI_INST                             (1 << 1)
#endif /* HAVE_OSPF_MULTI_INST */

#ifdef HAVE_RESTART
  /* OSPF global config. */
  u_char config;
#define OSPF_GLOBAL_CONFIG_RESTART_GRACE_PERIOD         (1 << 0)
#define OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY        (1 << 1)
#define OSPF_GLOBAL_CONFIG_RESTART_HELPER_GRACE_PERIOD  (1 << 2)

  /* Restart reason for Graceful Restart. */
  u_char restart_reason;
#define OSPF_RESTART_REASON_UNKNOWN                      0
#define OSPF_RESTART_REASON_RESTART                      1
#define OSPF_RESTART_REASON_UPGRADE                      2
#define OSPF_RESTART_REASON_SWITCH_REDUNDANT             3

  /* Helper policy for Graceful Restart. */
  u_char helper_policy;
#define OSPF_RESTART_HELPER_UNSPEC                       0
#define OSPF_RESTART_HELPER_NEVER                        1
#define OSPF_RESTART_HELPER_ONLY_RELOAD                  2
#define OSPF_RESTART_HELPER_ONLY_UPGRADE                 3

  /* Helper limit grace period for Graceful Restart. */
  u_int16_t max_grace_period;

  /* Grace period for Graceful Restart. */
  u_int16_t grace_period;

  /* Buffer for restart option. */
  struct stream *restart_option;
  vector restart_tlvs;

  /* Graceful Restart expire timer/enter event. */
  struct thread *t_restart;

  /* OSPF Helper Never Router-ID list */
  struct list *never_restart_helper_list;
#endif /* HAVE_RESTART */

  /* OSPF global interface table. */
  struct ls_table *if_table;

  /* OSPF interface parameter pool. */
  struct list *if_params;

  /* OSPF notifiers. */
  struct list *notifiers[OSPF_NOTIFY_MAX];

  /* OSPF SNMP trap callback function */
#ifdef HAVE_SNMP
  vector traps[OSPF_TRAP_ID_MAX];
#endif /* HAVE_SNMP */
};

/* Macro. */
#define IS_RFC1583_COMPATIBLE(O)                                              \
    CHECK_FLAG ((O)->config, OSPF_CONFIG_RFC1583_COMPATIBLE)
#define IS_OPAQUE_CAPABLE(O)                                                  \
    CHECK_FLAG ((O)->config, OSPF_CONFIG_OPAQUE_LSA)
#define IS_TE_CAPABLE(O)                                                      \
    CHECK_FLAG ((O)->config, OSPF_CONFIG_TRAFFIC_ENGINEERING)

#define AREA_COUNT_NON_BACKBONE(O)                                            \
    (ospf_area_count ((O)) - (top->backbone != NULL ? 1 : 0))

#define OSPF_TLV_CHAR_PUT(S,T,L,V)                                            \
    do {                                                                      \
      stream_putw ((S), (T));                                                 \
      stream_putw ((S), (L));                                                 \
      stream_putc ((S), (V));                                                 \
      stream_putc ((S), 0);                                                   \
      stream_putc ((S), 0);                                                   \
      stream_putc ((S), 0);                                                   \
    } while (0)

#define OSPF_TLV_INT32_PUT(S,T,L,V)                                           \
    do {                                                                      \
      stream_putw ((S), (T));                                                 \
      stream_putw ((S), (L));                                                 \
      stream_putl ((S), (V));                                                 \
    } while (0)

#define OSPF_TLV_IN_ADDR_PUT(S,T,L,V)                                         \
    do {                                                                      \
      stream_putw ((S), (T));                                                 \
      stream_putw ((S), (L));                                                 \
      stream_put_in_addr ((S), (V));                                          \
    } while (0)

#define OSPF_TLV_SPACE(L)               (((((L) + 3) / 4) * 4) + 4)

/* OSPF Feature Capability variables. */
extern u_char ospf_cap_have_vrf_ospf;
extern u_char ospf_cap_have_vr;

/* Macro to check OSPF License manager variables. */
#ifdef HAVE_LICENSE_MGR
#define IF_OSPF_CAP(var)                 if (ospf_cap_ ## var)
#define OSPF_CAP(var)                    (ospf_cap_ ## var)
#else
#define IF_OSPF_CAP(var)                 if (ospf_cap_ ## var)          
#define OSPF_CAP(var)                    1
#endif /* HAVE_LICENSE_MGR. */

#define OSPF_CAP_HAVE_VRF_OSPF           OSPF_CAP (have_vrf_ospf)
#define IF_OSPF_CAP_HAVE_VRF_OSPF        IF_OSPF_CAP (have_vrf_ospf)
#define OSPF_CAP_HAVE_VR                 OSPF_CAP (have_vr)
#define IF_OSPF_CAP_HAVE_VR              IF_OSPF_CAP (have_vr)

/* Messages. */
extern struct message ospf_lsa_type_msg[];
extern struct message ospf_link_state_id_type_msg[];
extern struct message ospf_link_type_msg[];
extern int ospf_lsa_type_msg_max;
extern int ospf_link_state_id_type_msg_max;
extern int ospf_link_type_msg_max;

/* Global variables. */
extern struct lib_globals *og;
extern struct pal_in4_addr IPv4AllSPFRouters;
extern struct pal_in4_addr IPv4AllDRouters;
extern struct pal_in4_addr IPv4AddressUnspec;
extern struct pal_in4_addr IPv4LoopbackAddress;
extern struct pal_in4_addr IPv4HostAddressMask;
extern struct ls_prefix LsPrefixIPv4Default;
#ifdef HAVE_MD5
extern u_char OSPFMessageDigestKeyUnspec[];
#endif /* HAVE_MD5 */
extern struct pal_timeval OSPFLSARefreshEventInterval;
extern struct ls_prefix DefaultIPv4;

#define ZG      (og)

/* Prototypes. */
struct ospf_interface;

void *ospf_register_notifier (struct ospf_master *, u_char,
                              int (*) (u_char, void *, void *,
                                       void *), void *, int);
void ospf_unregister_notifier (struct ospf_master *, void *);
void ospf_call_notifiers (struct ospf_master *, u_char, void *, void *);

u_int32_t ospf_area_count (struct ospf *);
u_int32_t ospf_area_default_count (struct ospf *);
u_int32_t ospf_area_if_count (struct ospf_area *);
u_int32_t ospf_area_vlink_count (struct ospf_area *);
struct ospf_area *ospf_area_entry_lookup (struct ospf *, struct pal_in4_addr);
struct ospf_area *ospf_stub_area_entry_lookup (struct ospf *,
                                               struct pal_in4_addr, int);

void ospf_update_status (void *data, int pos);
struct ospf_area *ospf_area_lock (struct ospf_area *);
struct ospf_area *ospf_area_unlock (struct ospf_area *);
void ospf_area_up (struct ospf_area *);
void ospf_area_down (struct ospf_area *);
struct ospf_area *ospf_area_get (struct ospf *, struct pal_in4_addr);
struct ospf_area *ospf_area_get_wait (struct ospf *, struct pal_in4_addr);
int ospf_area_conf_external_routing_set (struct ospf_area *, u_char);
int ospf_area_conf_default_cost_set (struct ospf_area *, u_int32_t);
u_char ospf_area_auth_type_get (struct ospf_area *);
#ifdef HAVE_NSSA
u_char ospf_nssa_translator_role_get (struct ospf_area *);
u_char ospf_nssa_default_originate_get (struct ospf_area *);
#endif /* HAVE_NSSA */
u_char ospf_area_shortcut_get (struct ospf_area *);

void ospf_network_free (struct ospf_network *);
struct ospf_network *ospf_network_lookup (struct ospf *, struct ls_prefix *);
struct ospf_network *ospf_network_match (struct ospf *, struct ls_prefix *);
bool_t ospf_network_same (struct ospf *, struct ls_prefix *);
int ospf_network_area_match (struct ospf_area *);
void ospf_network_delete (struct ospf *, struct ospf_network *);
int ospf_network_get (struct ospf *, struct pal_in4_addr,
                                       struct ls_prefix *, u_int16_t id);
int ospf_network_apply (struct ospf *,
                        struct ospf_network *, struct connected *);
void ospf_host_entry_match_apply (struct ospf *, struct connected *);
int ospf_host_entry_apply (struct ospf *,
                           struct ospf_host_entry *, struct connected *);

int ospf_passive_if_apply (struct ospf_interface *);
int ospf_passive_if_add (struct ospf *, char *, struct pal_in4_addr);
int ospf_passive_if_delete (struct ospf *, char *, struct pal_in4_addr);
void ospf_passive_if_delete_by_interface (struct ospf_master *,
                                          struct interface *);
void ospf_passive_if_add_by_interface (struct ospf_master *,
                                          struct interface *);
void ospf_passive_if_list_delete (struct ospf *top);
void ospf_no_passive_if_list_delete (struct ospf *top);

struct ospf_host_entry *ospf_host_entry_new ();
void ospf_host_entry_free (struct ospf_host_entry *);
int ospf_host_entry_insert (struct ospf *, struct ospf_host_entry *);
int ospf_host_entry_delete (struct ospf *, struct ospf_host_entry *);
struct ospf_host_entry *ospf_host_entry_lookup (struct ospf *,
                                                struct pal_in4_addr, int);
void ospf_host_entry_run (struct ospf *, struct ospf_host_entry *);

struct ospf_redist_conf *ospf_redist_conf_lookup_by_inst (struct ospf *, int , int);
struct ospf *ospf_proc_lookup (struct ospf_master *, int);
struct ospf *ospf_proc_lookup_any (int, u_int32_t);
struct ospf *ospf_proc_lookup_by_lsdb (struct ospf_lsdb *);
pal_time_t ospf_proc_uptime (struct ospf *);
int ospf_proc_up (struct ospf *);
int ospf_proc_down (struct ospf *);
int ospf_proc_update (struct ospf *);
void ospf_router_id_update (struct ospf *);
struct ospf *ospf_proc_get (struct ospf_master *, int, char *);
void ospf_proc_destroy (struct ospf *);

#ifdef HAVE_OSPF_DB_OVERFLOW
void ospf_db_overflow_exit (struct ospf *);
void ospf_db_overflow_enter (struct ospf *);
#endif /* HAVE_OSPF_DB_OVERFLOW */

void ospf_buffer_stream_ensure (struct stream *, size_t);
int ospf_thread_wrapper (struct thread *, int (*func)(struct thread *),
                         char *, int);
void ospf_global_init (void);
int ospf_master_init (struct apn_vr *);
struct ospf_master *ospf_master_lookup_default (struct lib_globals *);
struct ospf_master *ospf_master_lookup_by_id (struct lib_globals *, u_int32_t);

struct ls_node *ospf_api_lookup (struct ls_table *, int, ...);

void ospf_init (struct lib_globals *);
void ospf_disconnect_clean ();
void ospf_terminate (struct lib_globals *);

#endif /* _PACOS_OSPFD_H */
