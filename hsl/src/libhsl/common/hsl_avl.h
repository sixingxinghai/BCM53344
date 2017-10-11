/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_AVL_H
#define _HSL_AVL_H

struct hsl_avl_node
{
  struct hsl_avl_node *left;      /* Left node. */
  struct hsl_avl_node *right;     /* Right node. */
  struct hsl_avl_node *parent;    /* Parent node. */
  int balance;                    /* Balance factor. */
  int rank;                       /* Relative position of node in own subtree i.e. number of
                                     nodes in left subtree + 1. */
  int flag;                       /* 0 -> left child of its parent or is root of the tree.
                                     1 -> right child of its parent. */
  void *info;                     /* Data. */
};

struct hsl_avl_tree
{
  int max_nodes ;             /* Maximum number of bytes for data. */
  int (*compare_function) (void *data1, void *data2); /* Compare function. */
  char *data;                 /* Array for preallocated entries. */
  struct hsl_avl_node *root;      /* Root. */
  struct hsl_avl_node *free_list; /* Free list. */
  int count;                  /* Number of entries in tree. */
};

typedef int (*hsl_avl_traversal_fn)(void *data, void *user_data); 
#define HSL_AVL_NODE_INFO(n)       (n)->info
#define HSL_AVL_COUNT(t)           (t)->count
#define HSL_AVL_NODE_LEFT(n)       (n)->left
#define HSL_AVL_NODE_RIGHT(n)      (n)->right
#define HSL_AVL_NODE_PARENT(n)     (n)->parent

/* Function declarations. */

/* Delete node from AVL tree. */
void hsl_avl_delete_node (struct hsl_avl_tree *hsl_avl_tree, struct hsl_avl_node **node);

/* Create AVL tree. 
   If the max_nodes is 0, no preallocation is done. Every node is allocated
   first from the free_list and then from the system memory. */
int hsl_avl_create (struct hsl_avl_tree **hsl_avl_tree,
                    int max_nodes,
                    int (*compare_function) (void *data1, void *data2));

/* Traverse tree. */
int hsl_avl_tree_traverse (struct hsl_avl_tree *hsl_avl_tree, hsl_avl_traversal_fn fn, void *data);

/* Tree top. */
struct hsl_avl_node *hsl_avl_top (struct hsl_avl_tree *hsl_avl_tree);

/* Get next entry in AVL tree. */
struct hsl_avl_node* hsl_avl_getnext (struct hsl_avl_tree *hsl_avl_tree, struct hsl_avl_node *node);

/* Delete AVL node containing the data. */
int hsl_avl_delete (struct hsl_avl_tree *hsl_avl_tree, void *data);

/* Insert a node in AVL tree. */
int hsl_avl_insert (struct hsl_avl_tree *hsl_avl_tree, void *data);

/* Lookup AVL tree. */
struct hsl_avl_node *hsl_avl_lookup (struct hsl_avl_tree *hsl_avl_tree, void *data);

/* Get next node. */
struct hsl_avl_node* hsl_avl_next (struct hsl_avl_node *node);

/* Get  AVL tree node count. */
int hsl_avl_get_tree_size(struct hsl_avl_tree *p_hsl_avl_tree);

/* Free AVL tree . */
int hsl_avl_tree_free (struct hsl_avl_tree **pp_hsl_avl_tree,void (*hsl_avl_data_free)(void *ptr));

#endif /* _HSL_AVL_H */
