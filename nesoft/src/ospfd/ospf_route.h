/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_ROUTE_H
#define _PACOS_OSPF_ROUTE_H

#define OSPF_NEXTHOP_TABLE_DEPTH        8

/* OSPF nexthop. */
struct ospf_nexthop
{
  /* Backpointer to the top.  */
  struct ospf *top;

  /* Flags. */
  u_int16_t flags;
#define OSPF_NEXTHOP_INVALID            (1 << 0)
#define OSPF_NEXTHOP_CONNECTED          (1 << 1)
#define OSPF_NEXTHOP_UNNUMBERED         (1 << 2)
#define OSPF_NEXTHOP_VIA_TRANSIT        (1 << 3)

  /* Reference count. */
  u_int32_t lock;

  /* Nexthop interface. */
  struct ospf_interface *oi;

  /* Nexthop interface ID.  */
  struct pal_in4_addr if_id;

  /* Neighbor IP interface address. */
  struct pal_in4_addr nbr_id;
};

/* OSPF path. */
struct ospf_path
{
  /* Path type. */
  u_char type;

  /* Flags. */
  u_char flags;

  /* Lock. */
  u_int32_t lock;

  /* Area ID.  */
  struct pal_in4_addr area_id;

  /* Metric. */
  u_int32_t cost;

  /* Tag */
  u_int32_t tag;

  /* Type-2 cost for external routes. */
  u_int32_t type2_cost;

  /* Nexthop vector. */
  vector nexthops;

#ifdef HAVE_MPLS
  /* Tunnel Nexthop vector. */
  struct ospf_igp_shortcut_lsp *t_lsp;
  u_int16_t t_cost;
#endif /* HAVE_MPLS */

  /* Origin LSAs vector for incremental update. */
  vector lsa_vec;

  /* Path base for external routes.  */
  struct ospf_path *path_base;

  /* Transit path.  */
  struct ospf_path *path_transit;
};

/* OSPF route. */
struct ospf_route
{
  /* Node to IP prefix. */
  struct ls_node *rn;

  /* Path selected. */
  struct ospf_path *selected;

  /* Distance. */
  u_char distance;

  /* Path list. */
  vector paths;

  /* Candidate path list.  */
  vector news;
};

/* OSPF route calculation.  */
struct ospf_route_calc
{
  /* Next route calculation candidate.  */
  struct ls_node *rn_next[OSPF_PATH_MAX];
  struct ls_node *rn_next_asbr[OSPF_PATH_MAX];

  /* Threads.  */
  struct thread *t_rt_calc[OSPF_PATH_MAX]; /* Route calculation thread.  */
  struct thread *t_rt_calc_asbr[OSPF_PATH_MAX]; /* ASBR route calc thread.  */
};

/* Macros.  */
#define OSPF_PATH_SELECTED(P)                                                 \
    ((P) != NULL && ospf_path_valid ((P)->path_transit)                       \
     && (P)->path_transit->cost <= (P)->cost ? (P)->path_transit : (P))

#define OSPF_PATH_COST(P)                                                     \
    (OSPF_PATH_SELECTED (P)->cost)

#define OSPF_PATH_NEXTHOPS(P)                                                 \
    (OSPF_PATH_SELECTED (P)->nexthops)

#define OSPF_NEXTHOP_IF_ID(A, I)                                              \
    ((I) == NULL ? (A)->area_id.s_addr                                        \
     : (I)->type == OSPF_IFTYPE_VIRTUALLINK                                   \
     ? ((struct ospf_vlink *)(I)->u.vlink)->area_id.s_addr                    \
     : pal_hton32 ((I)->u.ifp->ifindex))

#define OSPF_NEXTHOP_NBR_ID(N)                                                \
    ((N)->oi->type == OSPF_IFTYPE_VIRTUALLINK                                 \
     ? (N)->ident.router_id : (N)->ident.address->u.prefix4)

#define IS_OSPF_NEXTHOP_IN_AREA(N, A)                                         \
    ((N)->oi == NULL || (N)->oi->type == OSPF_IFTYPE_VIRTUALLINK              \
     ? (N)->if_id.s_addr == (A)->area_id.s_addr : (N)->oi->area == (A))

#define OSPF_EVENT_SUSPEND_ON_TOP(T,F,A,V)                                    \
      do {                                                                      \
        if (!(T))                                                               \
          (A)->t_route_cleanup = thread_add_event_low (ZG, (F), (A), (V));\
      } while (0)


/* Prototypes. */
struct ospf_nexthop *ospf_nexthop_get (struct ospf_area *,
                                       struct ospf_interface *,
                                       struct pal_in4_addr);
struct ospf_nexthop *ospf_nexthop_lock (struct ospf_nexthop *);
void ospf_nexthop_unlock (struct ospf_nexthop *);
void ospf_nexthop_if_down (struct ospf_interface *);
void ospf_nexthop_nbr_down (struct ospf_neighbor *);
void ospf_nexthop_abr_down (struct ospf_area *, struct pal_in4_addr);
void ospf_nexthop_transit_down (struct ospf_area *);

struct ospf_path *ospf_path_new (int);
struct ospf_path *ospf_path_lock (struct ospf_path *);
void ospf_path_unlock (struct ospf_path *);
void ospf_path_nexthop_add (struct ospf_path *, struct ospf_nexthop *);
void ospf_path_nexthop_copy_all (struct ospf *, struct ospf_path *,
                                 struct ospf_vertex *);
void ospf_path_nexthop_delete (struct ospf_path *,
                               struct pal_in4_addr, struct pal_in4_addr);
void ospf_path_nexthop_delete_other (struct ospf_path *,
                                     struct ospf_nexthop *);
vector ospf_path_nexthop_vector (struct ospf_path *);
struct ospf_path *ospf_path_copy (struct ospf_path *);
void ospf_path_swap (struct ospf_path *, struct ospf_path *);
int ospf_path_valid (struct ospf_path *);
int ospf_path_code (struct ospf_path *, struct ospf_nexthop *);
int ospf_path_cmp (struct ospf *, struct ospf_path *, struct ospf_path *);

struct ospf_route *ospf_route_new ();
void ospf_route_free (struct ospf_route *);
struct ospf_path *ospf_route_path_lookup (struct ospf_route *,
                                          int, struct ospf_area *);
struct ospf_path *ospf_route_path_lookup_from_default (struct ospf_route *,
                                                       struct ospf *);
void ospf_route_path_set_new (struct ospf_route *, struct ospf_path *);
struct ospf_path *ospf_route_path_lookup_new (struct ospf_route *,
                                              int, struct ospf_area *);
struct ospf_path *ospf_route_path_lookup_by_area (struct ospf_route *,
                                                  struct ospf_area *);
struct ospf_route *ospf_route_lookup (struct ospf *,
                                      struct pal_in4_addr *, u_char);
struct ospf_route *ospf_route_match (struct ospf *,
                                     struct pal_in4_addr *, u_char);
struct ospf_route *ospf_route_match_internal (struct ospf *,
                                              struct pal_in4_addr *);
void ospf_route_add (struct ospf *, struct pal_in4_addr *, u_char,
                     struct ospf_route *);
int ospf_route_install (struct ospf *, struct ospf_area *, int,
                        struct ls_node *);
void ospf_route_install_all_event_add (struct ospf_area *, int);

struct ospf_route *ospf_route_asbr_lookup (struct ospf *, struct pal_in4_addr);
struct ospf_path *ospf_route_path_asbr_lookup_by_area (struct ospf_area *,
                                                       struct pal_in4_addr);
void ospf_route_asbr_add (struct ospf *,
                          struct pal_in4_addr, struct ospf_route *);
int ospf_route_asbr_install (struct ospf *, struct ospf_area *,
                             int, struct ls_node *);
void ospf_route_asbr_install_all_event_add (struct ospf_area *, int);

void ospf_route_add_connected (struct ospf_interface *, struct prefix *);
void ospf_route_delete_connected (struct ospf_interface *, struct prefix *);
void ospf_route_add_discard (struct ospf *,
                             struct ospf_area *, struct ls_prefix *);
void ospf_route_delete_discard (struct ospf *,
                                struct ospf_area *, struct ls_prefix *);
struct ospf_route *ospf_route_add_stub (struct ospf_area *,
                                        struct ospf_vertex *,
                                        struct pal_in4_addr,
                                        u_char, u_int16_t);
void ospf_route_add_spf (struct ospf_area *, struct ospf_vertex *);
void ospf_route_clean_invalid_event_add (struct ospf *);

void ospf_route_path_summary_lsa_set (struct ospf *, struct ospf_lsa *);
void ospf_route_path_summary_lsa_unset (struct ospf *, struct ospf_lsa *);
void ospf_route_path_asbr_summary_lsa_set (struct ospf *, struct ospf_lsa *);
void ospf_route_path_asbr_summary_lsa_unset (struct ospf *, struct ospf_lsa *);
void ospf_route_path_external_lsa_set (struct ospf *, struct ospf_lsa *);
void ospf_route_path_external_lsa_unset (struct ospf *, struct ospf_lsa *);

int ospf_route_clean_invalid (struct thread *);
void ospf_route_table_withdraw (struct ospf *);
void ospf_route_table_finish (struct ls_table *);

void ospf_route_calc_route_clean (struct ospf_area *);
struct ospf_route_calc *ospf_route_calc_init (void);
void ospf_route_calc_finish (struct ospf_area *);

#endif /* _PACOS_OSPF_ROUTE_H */
