/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _PACOS_AVL_H
#define _PACOS_AVL_H

struct avl_node
{
  struct avl_node *left;          /* Left node. */
  struct avl_node *right;         /* Right node. */
  struct avl_node *parent;        /* Parent node. */

  struct avl_node *prev;          /* List for partial cleanup */
  struct avl_node *next;

  int balance;                    /* Balance factor. */
  int rank;                       /* Relative position of node in own subtree i.e. number of
                                     nodes in left subtree + 1. */
  int flag;                       /* 0 -> left child of its parent or is root of the tree.
                                     1 -> right child of its parent. */
  u_int32_t lock;
  void *info;                     /* Data. */
};

struct avl_tree
{
  int max_nodes ;             /* Maximum number of bytes for data. */
  int (*compare_function) (void *data1, void *data2); /* Compare function. */
  char *data;                 /* Array for preallocated entries. */
  struct avl_node *root;      /* Root. */
  struct avl_node *free_list; /* Free list. */
  struct avl_node *head;      /* List head */
  int count;                  /* Number of entries in tree. */
};

#define avl_list_head(tree)  (tree)->head
#define avl_list_next(node)  (node)->next

#define AVL_LIST_LOOP_DEL(T,V,N,NN)  \
  if (T) \
    for ((N) = avl_list_head (T), NN = ((N)!=NULL) ? avl_list_next(N) : NULL; \
         (N);                                                                 \
         (N) = (NN), NN = ((N)!=NULL) ? avl_list_next(N) : NULL)              \
      if (((V) = (N)->info) != NULL)
   
#define AVL_LIST_LOOP(T,V,N      )                                   \
  if (T)                                                             \
    for ((N) = avl_list_head ((T)); (N); (N) = avl_list_next ((N)))  \
      if (((V) = (N)->info) != NULL)

typedef int (*avl_traversal_fn)(void *data, void *user_data); 
typedef int (*avl_traversal2_fn)(void *data, void *user_data1,
                                 void *user_data2); 
typedef int (*avl_traversal3_fn)(void *data, void *user_data1,
                                 void *user_data2, void *user_data3); 
#define avl_lock_node(n)       (n)->lock ++
#define avl_unlock_node(n)     (n)->lock --
#define avl_is_last_node(n)    ((n)->lock <= 1)

#define AVL_NODE_INFO(n)       (n)->info
#define AVL_COUNT(t)           (t)->count
#define AVL_NODE_LEFT(n)       (n)->left
#define AVL_NODE_RIGHT(n)      (n)->right

#define AVL_TREE_LOOP(T,V,N)                              \
  if (T)                                                  \
    for ((N) = avl_top ((T)); (N); (N) = avl_next ((N)))  \
      if (((V) = (N)->info) != NULL)

#define AVL_LOOP_DEL(T,V,N,NN) \
  if (T) \
    for ((N) = avl_top (T), NN = ((N)!=NULL) ? avl_next(N) : NULL;  \
         (N);                                                       \
         (N) = (NN), NN = ((N)!=NULL) ? avl_next(N) : NULL)         \
      if (((V) = (N)->info) != NULL)

/* Function declarations. */

/* Delete node from AVL tree. */
int avl_delete_node (struct avl_tree *avl_tree, struct avl_node *node);

/* Create AVL tree. 
   If the max_nodes is 0, no preallocation is done. Every node is allocated
   first from the free_list and then from the system memory. */
int avl_create (struct avl_tree **avl_tree,
                    int max_nodes,
                    int (*compare_function) (void *data1, void *data2));

/* Create where the avl_tree is allocated in the caller routine */
int
avl_create2 (struct avl_tree *avl,
            int max_nodes,
            int (*compare_function) (void *data1, void *data2));

/* Traverse tree. */
int avl_traverse (struct avl_tree *avl_tree, avl_traversal_fn fn, void *data);

int avl_traverse2 (struct avl_tree *avl_tree, avl_traversal2_fn fn,
                   void *data1, void *data2);

int avl_traverse3 (struct avl_tree *avl_tree, avl_traversal3_fn fn,
                   void *data1, void *data2, void *data3);

/* Tree top. */
struct avl_node *avl_top (struct avl_tree *avl_tree);

/* Get next entry in AVL tree. */
struct avl_node* avl_getnext (struct avl_tree *avl_tree, struct avl_node *node);

/* Delete AVL node containing the data. */
int avl_remove (struct avl_tree *avl_tree, void *data);

/* Insert a node in the AVL tree and return the node. */
struct avl_node *
avl_insert_node (struct avl_tree *avl_tree, void *data);

/* Insert a node in AVL tree. */
int avl_insert (struct avl_tree *avl_tree, void *data);

/* Lookup AVL tree. */
struct avl_node *avl_search (struct avl_tree *avl_tree, void *data);

/* Get next node. */
struct avl_node* avl_next (struct avl_node *node);

/* Get  AVL tree node count. */
int avl_get_tree_size(struct avl_tree *p_avl_tree);

/*
  Tree cleanup. Remove all nodes from tree but do not free the tree.
*/
int avl_tree_cleanup (struct avl_tree *avl_tree, void (*avl_data_free)(void *ptr));

/* Free AVL tree . */
int avl_tree_free (struct avl_tree **pp_avl_tree,void (*avl_data_free)(void *ptr));

int avl_finish (struct avl_tree *avl_tree);

int avl_finish2 (struct avl_tree *avl_tree);
#endif /* _PACOS_AVL_H */
