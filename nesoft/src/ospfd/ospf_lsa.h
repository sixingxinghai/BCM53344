/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_LSA_H
#define _PACOS_OSPF_LSA_H

/* OSPF LSA Range definition. */
#define OSPF_MIN_LSA            1  /* begin range here */
#if defined (HAVE_OPAQUE_LSA)
#define OSPF_MAX_LSA            12
#elif defined (HAVE_NSSA)
#define OSPF_MAX_LSA            8
#else
#define OSPF_MAX_LSA            6
#endif /* HAVE_NSSA */

/* OSPF LSA Type definition. */
#define OSPF_UNKNOWN_LSA                        0
#define OSPF_ROUTER_LSA                         1
#define OSPF_NETWORK_LSA                        2
#define OSPF_SUMMARY_LSA                        3
#define OSPF_SUMMARY_LSA_ASBR                   4
#define OSPF_AS_EXTERNAL_LSA                    5
#define OSPF_GROUP_MEMBER_LSA                   6  /* Not supported. */
#define OSPF_AS_NSSA_LSA                        7
#define OSPF_EXTERNAL_ATTRIBUTES_LSA            8  /* Not supported. */
#define OSPF_LINK_OPAQUE_LSA                    9
#define OSPF_AREA_OPAQUE_LSA                   10
#define OSPF_AS_OPAQUE_LSA                     11

#define OSPF_LSA_OPAQUE_TYPE_TE                 1
#define OSPF_LSA_OPAQUE_TYPE_GRACE              3

#define LSA_FLOOD_SCOPE_UNKNOWN                 0
#define LSA_FLOOD_SCOPE_LINK                    1
#define LSA_FLOOD_SCOPE_AREA                    2
#define LSA_FLOOD_SCOPE_AS                      3

#define OSPF_LSA_HEADER_SIZE                    20
#define OSPF_LINK_DESC_SIZE                     12
#define OSPF_LINK_TOS_SIZE                      4

#define OSPF_MIN_LSA_BUFSIZ                  1500
#define OSPF_MAX_ROUTER_LSA_SIZE                                              \
    (OSPF_PACKET_MAX_SIZE - (sizeof (struct pal_in4_header)                   \
                             + OSPF_HEADER_SIZE                               \
                             + OSPF_LS_UPD_MIN_SIZE + OSPF_AUTH_MD5_SIZE + 12))

#define OSPF_LSA_LENGTH_OFFSET                 18
#define OSPF_ROUTER_LSA_BITS_OFFSET            20
#define OSPF_ROUTER_LSA_NUM_LINKS_OFFSET       22
#define OSPF_ROUTER_LSA_LINK_DESC_OFFSET       24
#define OSPF_NETWORK_LSA_NETMASK_OFFSET        20
#define OSPF_NETWORK_LSA_ROUTERS_OFFSET        24
#define OSPF_SUMMARY_LSA_NETMASK_OFFSET        20
#define OSPF_SUMMARY_LSA_METRIC_OFFSET         25
#define OSPF_EXTERNAL_LSA_DEST_OFFSET           4
#define OSPF_EXTERNAL_LSA_NETMASK_OFFSET       20
#define OSPF_EXTERNAL_LSA_E_BIT_OFFSET         24
#define OSPF_EXTERNAL_LSA_METRIC_OFFSET        25
#define OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET       28
#define OSPF_EXTERNAL_LSA_TAG_OFFSET           32

#define OSPF_EXTERNAL_LSA_E_BIT(L)                                            \
    OSPF_PNT_UCHAR_GET ((L), OSPF_EXTERNAL_LSA_E_BIT_OFFSET)

extern u_char ospf_flood_scope[];

#define LSA_FLOOD_SCOPE(T)  ospf_flood_scope[(T)] 

/* OSPF LSA header. */
struct lsa_header
{
  /* LS age. */
  u_int16_t ls_age;

  /* Options. */
  u_char options;

  /* LS type. */
  u_char type;

  /* Link State ID. */
  struct pal_in4_addr id;

  /* Advertising Router. */
  struct pal_in4_addr adv_router;

  /* LS Sequence Number. */
  s_int32_t ls_seqnum;

  /* Checksum. */
  u_int16_t checksum;

  /* Length. */
  u_int16_t length;
};

/* OSPF LSA. */
struct ospf_lsa
{
  struct ospf_lsa *next;
  struct ospf_lsa *prev;

  /* LSA flags.  */
  u_int16_t flags;
#define OSPF_LSA_SELF                   (1 << 0)
#define OSPF_LSA_RECEIVED               (1 << 1)
#define OSPF_LSA_DISCARD                (1 << 2)
#define OSPF_LSA_FLUSHED                (1 << 3)
#define OSPF_LSA_REFRESHED              (1 << 4) /* Periodical refresh. */
#define OSPF_LSA_REFRESH_QUEUED         (1 << 5)
#define OSPF_LSA_REFRESH_EVENT          (1 << 6)
#define OSPF_LSA_ORIGINATE_EVENT        (1 << 7)
#define OSPF_LSA_SEQNUM_WRAPPED         (1 << 8)
#define notEXPLORED                      -1
#define inSPFtree                        -2

  int status;

  /* Alloced bytes.  */
  u_int16_t alloced;

  /* Lock count. */
  u_int16_t lock;

  /* LSA data. */
  struct lsa_header *data;

  /* Time stamp updated. */
  struct pal_timeval tv_update;

  /* Time stamp for SPF to track 
     if the vertex of this LSA is 
     already in SPF tree */
  struct pal_timeval tv_spf;

  /* Parent LSDB. */
  struct ospf_lsdb *lsdb;

  /* Retransmit list pointer */
  struct ospf_lsdb *rxmt_list; 

  /* Parameter for LSA origination. */
  void *param;

  /* Number of reference on the neighbor retransmit list. */
  u_int16_t rxmt_count;

  /* Due age to refresh. */
  u_int16_t refresh_age;

  /* Index of Refresh queue. */
  u_int32_t refresh_index;
#define LSA_REFRESH_INDEX_ROW(L)        ((L)->refresh_index >> 24)
#define LSA_REFRESH_INDEX_COLUMN(L)     ((L)->refresh_index & 0x00FFFFFF)
#define LSA_REFRESH_INDEX_SET(L,R,C)                                          \
    do {                                                                      \
      (L)->refresh_index = ((R) << 24) | ((C) & 0x00FFFFFF);                  \
    } while (0)
};

/* OSPF LSA Link Type. */
#define LSA_LINK_TYPE_POINTOPOINT               1
#define LSA_LINK_TYPE_TRANSIT                   2
#define LSA_LINK_TYPE_STUB                      3
#define LSA_LINK_TYPE_VIRTUALLINK               4

/* OSPF Router LSA Flag. */
#define ROUTER_LSA_BIT_B                        (1 << 0)
#define ROUTER_LSA_BIT_E                        (1 << 1)
#define ROUTER_LSA_BIT_V                        (1 << 2)
#define ROUTER_LSA_BIT_W                        (1 << 3)
#define ROUTER_LSA_BIT_NT                       (1 << 4)
#define ROUTER_LSA_BIT_S                        (1 << 5) /* Shortcut-ABR. */

#define IS_ROUTER_LSA_SET(B,F)                                                \
    (CHECK_FLAG ((B), ROUTER_LSA_ ## F))

#define OSPF_LINK_DESC_ID_OFFSET                0
#define OSPF_LINK_DESC_DATA_OFFSET              4
#define OSPF_LINK_DESC_TYPE_OFFSET              8
#define OSPF_LINK_DESC_NUM_TOS_OFFSET           9
#define OSPF_LINK_DESC_METRIC_OFFSET            10
#define OSPF_LINK_DESC_GET(P,L)                                               \
    do {                                                                      \
      (L).id = OSPF_PNT_IN_ADDR_GET ((P), OSPF_LINK_DESC_ID_OFFSET);          \
      (L).data = OSPF_PNT_IN_ADDR_GET ((P), OSPF_LINK_DESC_DATA_OFFSET);      \
      (L).type = OSPF_PNT_UCHAR_GET ((P), OSPF_LINK_DESC_TYPE_OFFSET);        \
      (L).num_tos = OSPF_PNT_UCHAR_GET ((P), OSPF_LINK_DESC_NUM_TOS_OFFSET);  \
      (L).metric = OSPF_PNT_UINT16_GET ((P), OSPF_LINK_DESC_METRIC_OFFSET);   \
    } while (0)

struct ospf_link_desc
{
  /* Link ID. */
  struct pal_in4_addr *id;

  /* Link Data. */
  struct pal_in4_addr *data;

  /* Link Type. */
  u_char type;

  /* # TOS. */
  u_char num_tos;

  /* TOS 0 Metric. */
  u_int16_t metric;
};

/* Map for self-originated Router-LSA. */
struct ospf_link_map
{
  /* Link type. */
  u_char type;

  /* Source. */
  u_char source;
#define OSPF_LINK_MAP_IF                1
#define OSPF_LINK_MAP_NBR               2
#define OSPF_LINK_MAP_HOST              3

  /* Metric. */
  u_int16_t metric;

  /* Link ID. */
  struct pal_in4_addr id;

  /* Link data. */
  struct pal_in4_addr data;

  /* Nexthop. */
  union
  {
    /* OSPF interface for broadcast/NBMA/stub. */
    struct ospf_interface *oi;

    /* OSPF neighbor for P2MP/P2P/VLINK. */
    struct ospf_neighbor *nbr;

    /* OSPF host entry. */
    struct ospf_host_entry *host;

    /* Dummy. */
    void *ref;

  } nh;

};

struct ospf_ia_lsa_map
{
  /* Type. */
  u_char type;
#define OSPF_IA_MAP_PATH        1
#define OSPF_IA_MAP_RANGE       2
#define OSPF_IA_MAP_STUB        3

#ifdef HAVE_VRF_OSPF
  /* flags */
  u_char flags;
#define OSPF_SET_SUMMARY_DN_BIT 1

  /* metric for OSPF_VRF Summary routes*/
  u_int32_t cost;
#endif /* HAVE_VRF_OSPF */
  /* Prefix. */
  struct ls_prefix *lp;

  /* Area. */
  struct ospf_area *area;

  /* Origin. */
  union
  {
    /* Intra-Area path. */
    struct ospf_path *path;

    /* Area range. */
    struct ospf_area_range *range;
  } u;

  /* summary/ASBR-summary-LSA. */
  struct ospf_lsa *lsa;
};

/* Macros. */
#define IS_EXTERNAL_METRIC(x)   ((x) & 0x80)
#define IS_P_BIT_SET(x)         ((x) & 0x10)

#define LS_AGE(x)               ospf_lsa_age_get (x)
#define IS_LSA_SELF(L)          (CHECK_FLAG ((L)->flags, OSPF_LSA_SELF))
#define IS_LSA_MAXAGE(L)        (LS_AGE ((L)) == OSPF_LSA_MAXAGE)
#define IS_LSA_TYPE_KNOWN(T)                                                  \
  ((T) >= OSPF_MIN_LSA && (T) < OSPF_MAX_LSA                                  \
   && LSA_FLOOD_SCOPE (T) != LSA_FLOOD_SCOPE_UNKNOWN)

#define STREAM_PUT_METRIC(S,M)                                                \
    do {                                                                      \
      stream_putc ((S), ((M) >> 16) & 0xff);                                  \
      stream_putc ((S), ((M) >> 8) & 0xff);                                   \
      stream_putc ((S), (M) & 0xff);                                          \
    } while (0)

#define LSA_REFRESH_TIMER_ADD(O,T,L,A)                                        \
    do {                                                                      \
      struct ospf_lsa *_lsa;                                                  \
      _lsa = (L);                                                             \
      if (IS_PROC_ACTIVE (O))                                                 \
        {                                                                     \
          if (_lsa != NULL)                                                   \
            ospf_lsa_refresh_timer_add (_lsa);                                \
          else                                                                \
            ospf_lsa_originate ((O), (T), (A));                               \
        }                                                                     \
    } while (0)

#define LSA_MAP_FLUSH(L)                                                      \
    do {                                                                      \
      if ((L) != NULL)                                                        \
        {                                                                     \
          ospf_lsdb_lsa_discard ((L)->lsdb, (L), 1, 0, 0);                    \
          ospf_lsa_originate_event_unset ((L));                               \
          (L)->param = NULL;                                                  \
          ospf_lsa_unlock ((L));                                              \
          (L) = NULL;                                                         \
        }                                                                     \
    } while (0)

#ifdef HAVE_OPAQUE_LSA
struct opaque_lsa
{
  struct lsa_header header;
  u_char opaque_data[1];
};

#define IS_OPAQUE_LSA(L)                                                      \
    ((L)->type >= OSPF_LINK_OPAQUE_LSA                                        \
     && (L)->type <= OSPF_AS_OPAQUE_LSA)
#define OPAQUE_DATA_LENGTH(L)                                                 \
    (pal_ntoh16 ((L)->header.length) - sizeof (struct lsa_header))
#define OPAQUE_TYPE(A)          (pal_ntoh32 ((A).s_addr) >> 24)
#define OPAQUE_ID(A)            (pal_ntoh32 ((A).s_addr) & 0x00ffffff)
#define MAKE_OPAQUE_ID(T, I)    pal_hton32 (((I) & 0x00ffffff) | ((T) << 24));

#define OPAQUE_LSA_TYPE_OFFSET          8
#define OPAQUE_LSA_TYPE_BY_SCOPE(S)     ((S) + OPAQUE_LSA_TYPE_OFFSET)

#define OSPF_OPAQUE_DATA_SIZE_MIN       4
#define OSPF_OPAQUE_MAP_LEN(L)                                                \
   ((L) + sizeof (struct ospf_opaque_map) - OSPF_OPAQUE_DATA_SIZE_MIN)

struct ospf_opaque_map
{
  /* LSA. */
  struct ospf_lsa *lsa;

  /* LSDB. */
  struct ospf_lsdb *lsdb;

  /* Flags. */
  u_char flags;
#define OSPF_OPAQUE_MAP_UPDATED         (1 << 0)

  /* Length. */
  u_int16_t length;

  /* Opaque ID. */
  struct pal_in4_addr id;

  /* Opaque data. */
  char data[OSPF_OPAQUE_DATA_SIZE_MIN];
};
#endif /* HAVE_OPAQUE_LSA */

/* Prototypes. */
void ospf_lsa_free (struct ospf_lsa *);

void ospf_lsa_unuse_clear (struct ospf *);
void ospf_lsa_unuse_clear_half (struct ospf *);
void ospf_lsa_unuse_max_update (struct ospf *, u_int32_t);

struct ospf_lsa *ospf_lsa_lock (struct ospf_lsa *);
void ospf_lsa_unlock (struct ospf_lsa *);
void ospf_lsa_discard (struct ospf_lsa *);
struct ospf_lsa *ospf_lsa_get_without_time (struct ospf *, void *, size_t);

int ospf_lsa_age_get (struct ospf_lsa *);
int ospf_lsa_more_recent (struct ospf_lsa *, struct ospf_lsa *);
int ospf_lsa_header_more_recent (struct lsa_header *, struct ospf_lsa *);
int ospf_lsa_different (struct ospf_lsa *, struct ospf_lsa *);
u_int16_t ospf_lsa_checksum (struct lsa_header *);

void ospf_link_vec_unset_by_if (struct ospf_area *, struct ospf_interface *);
void ospf_link_vec_unset_by_nbr (struct ospf_area *, struct ospf_neighbor *);
void ospf_link_vec_unset_by_host (struct ospf *, struct ospf_host_entry *);
void ospf_link_map_free (struct ospf_link_map *);
struct ospf_lsa *ospf_router_lsa_self (struct ospf_area *);
void ospf_router_lsa_refresh_all (struct ospf *);
void ospf_router_lsa_refresh_by_area_id (struct ospf *, struct pal_in4_addr);
void ospf_router_lsa_refresh_by_interface (struct ospf_interface *);
struct ospf_lsa *ospf_network_lsa_self (struct ospf_interface *);
int ospf_translated_lsa_conditional_check (struct ospf *);
int ospf_external_lsa_equivalent_update_self (struct ls_table *,
                                              struct ospf_lsa *);

struct ospf_ia_lsa_map *ospf_ia_lsa_map_new (u_char);
void ospf_ia_lsa_map_free (struct ospf_ia_lsa_map *);
void ospf_ia_lsa_map_add (struct ls_table *, struct ls_prefix *,
                          struct ospf_ia_lsa_map *);
void ospf_ia_lsa_map_delete (struct ls_table *, struct ls_prefix *);
struct ospf_ia_lsa_map *ospf_ia_lsa_map_lookup (struct ls_table *,
                                                struct ls_prefix *);
void ospf_ia_lsa_map_clear (struct ls_table *);

struct ospf_lsa *ospf_lsa_install (struct ospf *, struct ospf_lsa *);
int ospf_lsa_maxage_walker (struct thread *);

struct ospf_lsa *ospf_lsa_lookup (struct ospf_interface *, u_int32_t,
                                  struct pal_in4_addr, struct pal_in4_addr);
struct ospf_lsa *ospf_network_lsa_lookup_by_addr (struct ospf_area *,
                                                  struct pal_in4_addr);

struct ospf_lsa *ospf_lsa_originate (struct ospf *, u_char, void *);
void ospf_lsa_originate_event_add (struct ospf *, struct ospf_lsa *);
void ospf_lsa_originate_event_unset (struct ospf_lsa *);

struct ospf_lsa *ospf_lsa_refresh (struct ospf *, struct ospf_lsa *);
void ospf_lsa_refresh_timer_add (struct ospf_lsa *);
void ospf_lsa_refresh_event_unset (struct ospf_lsa *);
int ospf_lsa_refresh_walker (struct thread *);
void ospf_lsa_refresh_init (struct ospf *);
void ospf_lsa_refresh_clear (struct ospf *);
void ospf_lsa_refresh_restart (struct ospf *);
void ospf_lsa_refresh_queue_unset (struct ospf_lsa *);
int ospf_ase_incr_counter_rst (struct thread *);
#ifdef HAVE_OPAQUE_LSA
struct cli;
int ospf_register_show_opaque_data (u_char, void (*) (u_char, struct cli *,
                                                      struct opaque_lsa *)); 
#endif /* HAVE_OPAQUE_LSA */

#endif /* _PACOS_OSPF_LSA_H */
