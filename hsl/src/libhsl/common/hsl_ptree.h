/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _HSL_PTREE_H
#define _HSL_PTREE_H

/* Patricia tree top structure. */
struct hsl_ptree
{
  /* Top node. */
  struct hsl_ptree_node *top;

  /* Maximum key size allowed (in bits). */
  u_int32_t max_key_len;
};

/* Patricia tree node structure. */
struct hsl_ptree_node
{
  struct hsl_ptree_node *link[2];
#define  p_left      link[0]
#define  p_right     link[1]

  /* Tree link. */
  struct hsl_ptree *tree;
  struct hsl_ptree_node *parent;

  /* Lock of this radix. */
  u_int32_t lock;

  /* Each node of route. */
  void *info;

  /* Key len (in bits). */
  u_int32_t key_len;

  /* Key begins here. */
  u_int8_t key [1];
};

#define HSL_PTREE_KEY_MIN_LEN       1
#define HSL_PTREE_NODE_KEY(n)       (& (n)->key [0])

/* Prototypes. */
struct hsl_ptree *hsl_ptree_init (u_int16_t max_key_len);
struct hsl_ptree_node *hsl_ptree_top (struct hsl_ptree *tree);
struct hsl_ptree_node *hsl_ptree_next (struct hsl_ptree_node *node);
struct hsl_ptree_node *hsl_ptree_next_until (struct hsl_ptree_node *node1,
                                     struct hsl_ptree_node *node2);
struct hsl_ptree_node *hsl_ptree_node_get (struct hsl_ptree *tree, u_char *key,
                                   u_int16_t key_len);
struct hsl_ptree_node *hsl_ptree_node_lookup (struct hsl_ptree *tree, u_char *key,
                                      u_int16_t key_len);
struct hsl_ptree_node *hsl_ptree_lock_node (struct hsl_ptree_node *node);
struct hsl_ptree_node *hsl_ptree_node_match (struct hsl_ptree *tree, u_char *key,
                                     u_int16_t key_len);
void hsl_ptree_node_free (struct hsl_ptree_node *node);
void hsl_ptree_finish (struct hsl_ptree *tree);
void hsl_ptree_unlock_node (struct hsl_ptree_node *node);
void hsl_ptree_node_delete (struct hsl_ptree_node *node);
void hsl_ptree_node_delete_all (struct hsl_ptree *tree);
int hsl_ptree_has_info (struct hsl_ptree *tree);
/* End Prototypes. */

#endif /* _HSL_PTREE_H */
