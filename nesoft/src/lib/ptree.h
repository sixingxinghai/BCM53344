/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_PTREE_H
#define _PACOS_PTREE_H

/* Patricia tree top structure. */
struct ptree
{
  /* Top node. */
  struct ptree_node *top;

  /* Maximum key size allowed (in bits). */
  u_int32_t max_key_len;

  /* Identifier of the FIB the tree is for - default is 0 */
  u_int32_t id;
};

/* Patricia tree node structure. */
struct ptree_node
{
  struct ptree_node *link[2];
#define  p_left      link[0]
#define  p_right     link[1]

  /* Tree link. */
  struct ptree *tree;
  struct ptree_node *parent;

  /* Lock of this radix. */
  u_int32_t lock;

  /* Each node of route. */
  void *info;
 
  /* Key len (in bits). */
  u_int32_t key_len;

  /* Key begins here. */
  u_int8_t key [1];
};

#define PTREE_KEY_MIN_LEN       1
#define PTREE_NODE_KEY(n)       (& (n)->key [0])

#define PTREE_LOOP(T,V,N)                                 \
  if (T)                                                  \
    for ((N) = ptree_top ((T)); (N); (N) = ptree_next ((N)))  \
      if (((V) = (N)->info) != NULL)

/* Prototypes. */
struct ptree *ptree_init (u_int16_t max_key_len);
struct ptree_node *ptree_top (struct ptree *tree);
struct ptree_node *ptree_next (struct ptree_node *node);
struct ptree_node *ptree_next_until (struct ptree_node *node1,
                                     struct ptree_node *node2);
struct ptree_node *ptree_node_get (struct ptree *tree, u_char *key,
                                   u_int16_t key_len);
struct ptree_node *ptree_node_lookup (struct ptree *tree, u_char *key,
                                      u_int16_t key_len);
struct ptree_node *ptree_lock_node (struct ptree_node *node);
struct ptree_node *ptree_node_match1 (struct ptree *tree, u_char *key,
                                      u_int16_t key_len);
struct ptree_node *ptree_node_match (struct ptree *tree, u_char *key,
                                     u_int16_t key_len);
void ptree_node_free (struct ptree_node *node);
void ptree_finish (struct ptree *tree);
void ptree_unlock_node (struct ptree_node *node);
void ptree_node_delete (struct ptree_node *node);
void ptree_node_delete_all (struct ptree *tree);
int ptree_has_info (struct ptree *tree);
void
ptree_key_copy (struct ptree_node *node, u_char *key, u_int16_t key_len);
int
ptree_bit_to_octets (u_int16_t key_len);
int
ptree_key_match (u_char *np, u_int16_t n_len, u_char *pp, u_int16_t p_len);
int
ptree_check_bit (struct ptree *tree, u_char *p, u_int16_t key_len);
struct ptree_node * ptree_node_match_exclude (struct ptree *pt, u_char *key1, 
                                              u_int16_t key_len1, u_char *key2,
                                              u_int16_t key_len2);


/* End Prototypes. */

#endif /* _PACOS_PTREE_H */
