/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_LSDB_H
#define _PACOS_OSPF_LSDB_H

/* OSPF link list element */
struct ospf_link_list
{
  /* List head and tail */
  struct ospf_list_elt *head;
  struct ospf_list_elt *tail;

  /* count of the number of element */
  u_int16_t            count;
};

struct ospf_list_elt
{
  struct ospf_list_elt *next;
  struct ospf_list_elt *prev;
  struct ospf_lsa      *lsa;
};

/* OSPF Link State Database structure.  */
struct ospf_lsdb_slot
{
  /* List head and tail.  */
  struct ospf_lsa *head;
  struct ospf_lsa *tail;

  /* LSA Table. */
  struct ls_table *db;

  /* Sum of checksum. */
  u_int32_t checksum;

  /* Next candidate for the suspended processing.  */
  struct ospf_lsa *next;
  struct ls_node *next_node;
};

struct ospf_lsdb
{
  /* Flooding scope. */
  u_char scope;

  /* LSDB slots. */
  struct ospf_lsdb_slot type[OSPF_MAX_LSA];

  /* Next Slot for the suspended processing.  */
  int next_slot_type;

#ifdef HAVE_OPAQUE_LSA  
  struct ls_table *opaque_table;
#endif /* HAVE_OPAQUE_LSA */

  /* TE database. */
#ifdef HAVE_OSPF_CSPF 
  struct ptree *te_lsdb;
#endif /* HAVE_OSPF_CSPF */

  /* Parent by flooding scope. */
  union
  {
    /* Link-local scope. */
    struct ospf_interface *oi;

    /* Area scope. */
    struct ospf_area *area;

    /* AS scope. */
    struct ospf *top;

    /* Generic pointer. */
    void *parent;
  } u;

 // Vriable to store the current value of Maximum 
 //age for all the LSAs that are not max-aged 
  u_int16_t  max_lsa_age;      

/* Time stamp. */
  struct pal_timeval tv_maxage;            /* MaxAge Time Stamp. */

  /* Threads. */
  struct thread *t_lsa_walker;                  /* MaxAge Walker. */
#ifdef HAVE_OPAQUE_LSA
  struct thread *t_opaque_lsa;                  /* Opaque LSA timer. */
#endif /* HAVE_OPAQUE_LSA */

  /* LSDB suspend handling.  */
  struct thread *t_suspend;             /* Suspended thread.  */
  struct thread_list event_pend;        /* Pending events.  */
};

#define RNI_LSDB_LSA    0
#define RNI_LSDB_MAXAGE 1

struct ls_prefix_ls
{
  u_char family;
  u_char prefixlen;
  u_char prefixsize;
  u_char pad2;
  struct pal_in4_addr id;
  struct pal_in4_addr adv_router;
};

#ifdef HAVE_OSPF_CSPF
struct te_lsa_node
{
  struct list *link_lsa_list;
  struct ospf_lsa  *rtaddr_lsa;
};
#endif /* HAVE_OSPF_CSPF */

/* Macros. */
/* Macros. */
#define LSDB_LOOP(S, N, L)                                                    \
  if ((S)->db != NULL)                                                        \
    for ((N) = ls_table_top ((S)->db); (N); (N) = ls_route_next (N))          \
      if (((L) = (N)->vinfo[0]))

#define LSDB_LOOP_ALL(D, I, N, L)                                             \
  for ((I) = OSPF_MIN_LSA; (I) < OSPF_MAX_LSA; (I)++)                         \
    LSDB_LOOP (&(D)->type[(I)], (N), (L))

#define LSDB_LOOP_LSA(S, N, L)                                                    \
  if ((S)->db != NULL)                                                        \
    for ((N) = (S)->next_node ? (S)->next_node : ls_table_top ((S)->db); (N); (N) = ls_route_next (N))          \
      if (((L) = (N)->vinfo[0]))

#define LSDB_LOOP_ALL_LSA(D, I, N, L)                                             \
  for ((I) = (D)->next_slot_type; (I) < OSPF_MAX_LSA; (I)++)                         \
    LSDB_LOOP_LSA (&(D)->type[(I)], (N), (L))

#define LSDB_SIZE(S)                                                          \
    (sizeof (struct ospf_lsdb) + ((S) - 1) * sizeof (struct ospf_lsdb_slot))

#define ROUTER_LSDB(A)             (&(A)->lsdb->type[OSPF_ROUTER_LSA])
#define NETWORK_LSDB(A)            (&(A)->lsdb->type[OSPF_NETWORK_LSA])
#define SUMMARY_LSDB(A)            (&(A)->lsdb->type[OSPF_SUMMARY_LSA])
#define ASBR_SUMMARY_LSDB(A)       (&(A)->lsdb->type[OSPF_SUMMARY_LSA_ASBR])
#define EXTERNAL_LSDB(O)           (&(O)->lsdb->type[OSPF_AS_EXTERNAL_LSA])
#ifdef HAVE_NSSA
#define NSSA_LSDB(A)               (&(A)->lsdb->type[OSPF_AS_NSSA_LSA])
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
#define LINK_OPAQUE_LSDB(I)        (&(I)->lsdb->type[OSPF_LINK_OPAQUE_LSA])
#define AREA_OPAQUE_LSDB(A)        (&(A)->lsdb->type[OSPF_AREA_OPAQUE_LSA])
#define AS_OPAQUE_LSDB(O)          (&(O)->lsdb->type[OSPF_AS_OPAQUE_LSA])
#endif /* HAVE_OPAQUE_LSA */

#define ROUTER_LSDB_HEAD(A)        ((ROUTER_LSDB (A))->head)
#define NETWORK_LSDB_HEAD(A)       ((NETWORK_LSDB (A))->head)
#define SUMMARY_LSDB_HEAD(A)       ((SUMMARY_LSDB (A))->head)
#define ASBR_SUMMARY_LSDB_HEAD(A)  ((ASBR_SUMMARY_LSDB (A))->head)
#define EXTERNAL_LSDB_HEAD(O)      ((EXTERNAL_LSDB (O))->head)
#ifdef HAVE_NSSA
#define NSSA_LSDB_HEAD(A)          ((NSSA_LSDB (A))->head)
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
#define LINK_OPAQUE_LSDB_HEAD(I)   ((LINK_OPAQUE_LSDB (I))->head)
#define AREA_OPAQUE_LSDB_HEAD(A)   ((AREA_OPAQUE_LSDB (A))->head)
#define AS_OPAQUE_LSDB_HEAD(O)     ((AS_OPAQUE_LSDB (O))->head)
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_OPAQUE_LSA
#define ospf_opaque_lsa_redistribute_link(S,I,D,L)                            \
    ospf_opaque_lsa_redistribute ((S)->lsdb, (I), (D), (L));
#define ospf_opaque_lsa_redistribute_area(S,I,D,L)                            \
    ospf_opaque_lsa_redistribute ((S)->lsdb, (I), (D), (L));
#define ospf_opaque_lsa_redistribute_as(S,I,D,L)                              \
    ospf_opaque_lsa_redistribute ((S)->lsdb, (I), (D), (L));
#endif /* HAVE_OPAQUE_LSA */

/* OSPF LSDB related functions.  */
unsigned long ospf_lsdb_count (struct ospf_lsdb *, int);
unsigned long ospf_lsdb_count_all (struct ospf_lsdb *);
unsigned long ospf_lsdb_count_external_non_default (struct ospf *);
u_int32_t ospf_proc_lsdb_count (struct ospf *);
unsigned long ospf_lsdb_isempty (struct ospf_lsdb *);
u_int32_t ospf_lsdb_checksum (struct ospf_lsdb *, int);
u_int32_t ospf_lsdb_checksum_all (struct ospf_lsdb *);

void lsdb_prefix_set (struct ls_prefix_ls *, struct lsa_header *);
void ospf_lsdb_add (struct ospf_lsdb *, struct ospf_lsa *);
void ospf_lsdb_delete (struct ospf_lsdb *, struct ospf_lsa *);
struct ospf_lsa *ospf_lsdb_lookup (struct ospf_lsdb *, struct ospf_lsa *);
struct ospf_lsa *ospf_lsdb_lookup_by_id (struct ospf_lsdb *, u_char,
                                         struct pal_in4_addr,
                                         struct pal_in4_addr);
void ospf_lsdb_lsa_discard (struct ospf_lsdb *,
                            struct ospf_lsa *, int, int, int);
void ospf_lsdb_lsa_discard_by_type (struct ospf_lsdb *, int, int, int, int);
void ospf_lsdb_lsa_discard_all (struct ospf_lsdb *, int, int, int);

struct ospf_lsdb *ospf_lsdb_get_by_type (struct ospf_interface *, u_char);
#ifdef HAVE_OPAQUE_LSA
void ospf_opaque_lsa_redistribute (struct ospf_lsdb *, struct pal_in4_addr,
                                   void *, u_int16_t);
void ospf_opaque_lsa_timer_add (struct ospf_lsdb *);
struct ospf_opaque_map *ospf_opaque_map_lookup (struct ospf_lsdb *,
                                                struct pal_in4_addr);
void ospf_opaque_map_delete_all (struct ls_table *);
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_OSPF_CSPF
struct te_lsa_node *ospf_te_lsa_node_new ();
void ospf_te_lsa_node_free (struct te_lsa_node *);
void ospf_te_lsdb_add (struct ptree *, struct ospf_lsa *);
struct ospf_lsa *
ospf_te_lsdb_lookup (struct ptree *, struct pal_in4_addr,
                     u_char, struct pal_in4_addr *, u_char, u_char);
struct ptree_node *ospf_te_lsa_node_lookup (struct ptree *,
                                            struct pal_in4_addr);
struct listnode *ospf_te_link_lsa_lookup (struct te_lsa_node *,
                                          struct pal_in4_addr *, u_char,
                                          struct pal_in4_addr *,
                                          struct pal_in4_addr *);
    
#endif /* HAVE_OSPF_CSPF */

struct ospf_lsdb *ospf_lsdb_init (void);
struct ospf_lsdb *ospf_link_lsdb_init (struct ospf_interface *);
struct ospf_lsdb *ospf_area_lsdb_init (struct ospf_area *);
struct ospf_lsdb *ospf_as_lsdb_init (struct ospf *);
void ospf_lsdb_finish (struct ospf_lsdb *);
void ospf_link_lsdb_finish (struct ospf_lsdb *);
void ospf_area_lsdb_finish (struct ospf_lsdb *);
void ospf_as_lsdb_finish (struct ospf_lsdb *);

void ospf_lsdb_linklist_init (struct ospf_link_list *list);
unsigned long ospf_lsdb_linklist_count_all (struct ospf_link_list *lsdb);
int ospf_lsdb_linklist_isempty (struct ospf_link_list *list);
void ospf_lsdb_linklist_add (struct ospf_link_list *lsdb, 
                             struct ospf_lsa *lsa);
void ospf_lsdb_linklist_delete (struct ospf_link_list *lsdb, 
                                struct ospf_lsa *lsa);
void ospf_lsdb_linklist_finish (struct ospf_link_list *lsdb);

#define OSPF_LIST_ISEMPTY(list) (!list->count && (list->head == list->tail))

#endif /* _PACOS_OSPF_LSDB_H */

