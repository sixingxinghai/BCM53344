/* Copyright (C) 2001, 2002  All Rights Reserved. */

#ifndef _NSM_TABLE_H
#define _NSM_TABLE_H

#include "pal.h"
#include "prefix.h"

#define EXISTING_TREE_NODE   0
#define NEW_TREE_NODE        1

/* Patricia tree top structure. */
struct nsm_ptree_table
{
  /* Top node. */
  struct nsm_ptree_node *top;

  /* Maximum key size allowed (in bits). */
  u_int32_t max_key_len;

  /* Specify the type of ptree */
  u_char ptype;
};
/* Each routing entry. */
struct nsm_ptree_node
{
  /* DO NOT MOVE the first 2 pointers. They are used for memory
     manager as well */
  struct nsm_ptree_node *link[2];
#define p_left   link[0]
#define p_right  link[1]

  /* Tree link. */
  struct nsm_ptree_table *tree;
  struct nsm_ptree_node *parent;

  /* Lock of this radix */
  u_int32_t lock;

  /* Each node of tree. */
  void *info;

  /* Nexthop registration list. */
  struct list *nh_list;

  /* Key len (in bits). */
  u_int32_t key_len;

  /* Key begins here. */
  u_int8_t key [1];

};

#define NSM_PTREE_KEY_MIN_LEN       1
#define NSM_PTREE_NODE_KEY(n)       (& (n)->key [0])
#define NSM_PTREE_KEY_COPY(P, RN)    \
             ((RN)->key_len ? pal_mem_cpy ((P), (RN)->key, ptree_bit_to_octets((RN)->key_len)) : 0)
/* Prototypes. */
struct nsm_ptree_table *nsm_ptree_table_init (u_int16_t);
void nsm_ptree_table_finish (struct nsm_ptree_table *);
void nsm_ptree_unlock_node (struct nsm_ptree_node *);
void nsm_ptree_node_delete (struct nsm_ptree_node *);
struct nsm_ptree_node *nsm_ptree_top (struct nsm_ptree_table *);
struct nsm_ptree_node *nsm_ptree_next (struct nsm_ptree_node *);
struct nsm_ptree_node *nsm_ptree_next_until (struct nsm_ptree_node *,
                                             struct nsm_ptree_node *);
struct nsm_ptree_node *nsm_ptree_node_get (struct nsm_ptree_table *,
                                           u_char *, u_int16_t);
struct nsm_ptree_node *nsm_ptree_node_get_ipv4 (struct nsm_ptree_table *, 
                                        struct pal_in4_addr *, int *);
struct nsm_ptree_node *nsm_ptree_node_lookup (struct nsm_ptree_table *,
                                               u_char *, u_int16_t);
struct nsm_ptree_node *nsm_ptree_node_lookup_ipv4 (struct nsm_ptree_table *,
                                                 struct pal_in4_addr *);
struct nsm_ptree_node *nsm_ptree_lock_node (struct nsm_ptree_node *);
struct nsm_ptree_node *nsm_ptree_node_match (struct nsm_ptree_table *,
                                             u_char *, u_int16_t );
struct nsm_ptree_node *nsm_ptree_node_match_ipv4 (struct nsm_ptree_table *,
                                          struct pal_in4_addr *);
void nsm_ptree_node_free (struct nsm_ptree_node *);
u_char nsm_ptree_table_has_info (struct nsm_ptree_table *);
#ifdef HAVE_IPV6
struct nsm_ptree_node *nsm_ptree_node_match_ipv6 (struct nsm_ptree_table *,
                                                  struct pal_in6_addr *);
#endif /* HAVE_IPV6 */
void
nsm_ptree_key_copy (struct nsm_ptree_node *node, u_char *key,
                    u_int16_t key_len);
#endif /* _NSM_TABLE_H */
