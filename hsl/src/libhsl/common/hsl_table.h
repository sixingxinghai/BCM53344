/* Copyright (C) 2001-2004  All Rights Reserved. */

#ifndef _PACOS_TABLE_H
#define _PACOS_TABLE_H

/* Routing table top structure. */
struct hsl_route_table
{
  struct hsl_route_node *top;

  /* Table identifier. */
  u_int32_t id;
};

/* Each routing entry. */
struct hsl_route_node
{
  /* DO NOT MOVE the first 2 pointers. They are used for memory
     manager as well */
  struct hsl_route_node *link[2];
#define l_left   link[0]
#define l_right  link[1]

  /* Actual prefix of this radix. */
  hsl_prefix_t p;

  /* Flag for ECMP */
  HSL_BOOL is_ecmp;

  /* Tree link. */
  struct hsl_route_table *table;
  struct hsl_route_node *parent;

  /* Lock of this radix */
  u_int32_t lock;

  /* Each node of route. */
  void *info;
};

/* Prototypes. */
struct hsl_route_table *hsl_route_table_init (void);
void hsl_route_table_finish (struct hsl_route_table *);
void hsl_route_unlock_node (struct hsl_route_node *node);
void hsl_route_node_delete (struct hsl_route_node *node);
struct hsl_route_node *hsl_route_top (struct hsl_route_table *);
struct hsl_route_node *hsl_route_next (struct hsl_route_node *);
struct hsl_route_node *hsl_route_next_until (struct hsl_route_node *, struct hsl_route_node *);
struct hsl_route_node *hsl_route_node_get (struct hsl_route_table *, hsl_prefix_t *);
struct hsl_route_node *hsl_route_node_get_ipv4 (struct hsl_route_table *, 
                                        hsl_ipv4Address_t *);
#ifdef HAVE_IPV6
struct hsl_route_node *hsl_route_node_get_ipv6 (struct hsl_route_table *,
                                        hsl_ipv6Address_t *);
#endif /* HAVE_IPV6 */
struct hsl_route_node *hsl_route_node_lookup (struct hsl_route_table *, hsl_prefix_t *);
struct hsl_route_node *hsl_route_node_lookup_ipv4 (struct hsl_route_table *,
                                           hsl_ipv4Address_t *);
#ifdef HAVE_IPV6
struct hsl_route_node *hsl_route_node_lookup_ipv6 (struct hsl_route_table *,
                                           hsl_ipv6Address_t *);
#endif /* HAVE_IPV6 */
struct hsl_route_node *hsl_route_lock_node (struct hsl_route_node *node);
struct hsl_route_node *hsl_route_node_match (struct hsl_route_table *, hsl_prefix_t *);
struct hsl_route_node *hsl_route_node_match_exclude (struct hsl_route_table *,
                                             hsl_prefix_t *,
                                             hsl_prefix_t *);
struct hsl_route_node *hsl_route_node_match_ipv4 (struct hsl_route_table *,
                                          hsl_ipv4Address_t *);
void hsl_route_node_free (struct hsl_route_node *node);
u_char hsl_route_table_has_info (struct hsl_route_table *);
void hsl_route_table_id_set (struct hsl_route_table *, u_int32_t);
#ifdef HAVE_IPV6
struct hsl_route_node *hsl_route_node_match_ipv6 (struct hsl_route_table *,
                                          hsl_ipv6Address_t *);
#endif /* HAVE_IPV6 */

#endif /* _PACOS_TABLE_H */
