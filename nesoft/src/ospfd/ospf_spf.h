/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_SPF_H
#define _PACOS_OSPF_SPF_H

#define OSPF_VERTEX_ROUTER      OSPF_ROUTER_LSA         /* 1 */
#define OSPF_VERTEX_NETWORK     OSPF_NETWORK_LSA        /* 2 */

/* Table indices. */
#define RNI_VERTEX_INDEX(T)     ((T) - 1)

struct ospf_vertex
{
  /* Flags. */
  u_int32_t flags:8;
#define OSPF_VERTEX_NETWORK_CONNECTED   (1 << 0)
#define OSPF_VERTEX_PROCESSED           (1 << 1)

  int *status;

  /* Distance from root to this vertex. */
  u_int32_t distance:24;

  /* Distance from root to this vertex from tunnel.
     Should always less than or equal to distance. */
  u_int16_t t_distance;

  /* Origin LSA. */
  struct ospf_lsa *lsa;

  /* Vector of children. */
  vector child;

  /* Vector of nexthops. */
  vector nexthop;

#ifdef HAVE_MPLS
  /* Vector of nexthops from tunnel. */
  struct ospf_igp_shortcut_lsp *t_lsp;
#endif /* HAVE_MPLS */

  /* Vector of parents. */
  vector parent;
};

/* Prototypes. */
void ospf_vertex_free (struct ospf_vertex *);
int ospf_vertex_cmp (void *, void *);
struct ospf_vertex *ospf_spf_vertex_lookup (struct ospf_area *,
                                            struct pal_in4_addr);
void ospf_spf_vertex_delete_all (struct ospf_area *);
int ospf_spf_calculate_timer (struct thread *);
void ospf_spf_calculate_timer_add (struct ospf_area *);
void ospf_spf_calculate_timer_add_all (struct ospf *);
void ospf_spf_calculate_timer_reset_all (struct ospf *);

#endif /* _PACOS_OSPF_SPF_H */
