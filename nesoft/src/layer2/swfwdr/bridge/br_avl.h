/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _BR_AVL_H
#define _BR_AVL_H

struct br_avl_node
{
  struct br_avl_node *left;      /* Left node. */
  struct br_avl_node *right;     /* Right node. */
  struct br_avl_node *parent;    /* Parent node. */
  int balance;                    /* Balance factor. */
  int rank;                       /* Relative position of node in own subtree i.e. number of
                                     nodes in left subtree + 1. */
  int flag;                       /* 0 -> left child of its parent or is root of the tree.
                                     1 -> right child of its parent. */
  void *info;                     /* Data. */
};

struct br_avl_tree
{
  int max_nodes ;             /* Maximum number of bytes for data. */
  int (*compare_function) (void *data1, void *data2); /* Compare function. */
  char *data;                 /* Array for preallocated entries. */
  struct br_avl_node *root;      /* Root. */
  struct br_avl_node *free_list; /* Free list. */
  int count;                  /* Number of entries in tree. */
};

typedef int (*br_avl_traversal_fn)(void *data, void *user_data); 
#define BR_AVL_NODE_INFO(n)       (n)->info
#define BR_AVL_COUNT(t)           (t)->count
#define BR_AVL_NODE_LEFT(n)       (n)->left
#define BR_AVL_NODE_RIGHT(n)      (n)->right
#define BR_AVL_NODE_PARENT(n)     (n)->parent

/* Function declarations. */

/* Delete node from AVL tree. */
void br_avl_delete_node (struct br_avl_tree *br_avl_tree, struct br_avl_node **node);

/* Create AVL tree. 
   If the max_nodes is 0, no preallocation is done. Every node is allocated
   first from the free_list and then from the system memory. */
int br_avl_create (struct br_avl_tree **br_avl_tree,
                    int max_nodes,
                    int (*compare_function) (void *data1, void *data2));

/* Traverse tree. */
int br_avl_tree_traverse (struct br_avl_tree *br_avl_tree, br_avl_traversal_fn fn, void *data);

/* Tree top. */
struct br_avl_node *br_avl_top (struct br_avl_tree *br_avl_tree);

/* Get next entry in AVL tree. */
struct br_avl_node* br_avl_getnext (struct br_avl_tree *br_avl_tree, struct br_avl_node *node);

/* Delete AVL node containing the data. */
int br_avl_delete (struct br_avl_tree *br_avl_tree, void *data);

/* Insert a node in AVL tree. */
int br_avl_insert (struct br_avl_tree *br_avl_tree, void *data);

/* Lookup AVL tree. */
struct br_avl_node *br_avl_lookup (struct br_avl_tree *br_avl_tree, void *data);

/* Get next node. */
struct br_avl_node* br_avl_next (struct br_avl_node *node);

/* Get  AVL tree node count. */
int br_avl_get_tree_size(struct br_avl_tree *p_br_avl_tree);

/* Free AVL tree . */
int br_avl_tree_free (struct br_avl_tree **pp_br_avl_tree,void (*br_avl_data_free)(void *ptr));

#endif /* _BR_AVL_H */
