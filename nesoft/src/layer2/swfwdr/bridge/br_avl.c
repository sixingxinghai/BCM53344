/* Copyright (C) 2004  All Rights Reserved. */

#include <linux/autoconf.h>
#include "br_types.h"
#include "br_avl.h"

/* Get a free AVL tree node. */
static struct br_avl_node *
_br_avl_node_new (struct br_avl_tree *br_avl_tree)
{
  struct br_avl_node *node;
  int node_sz;

  node = br_avl_tree->free_list;
  if (node)
    {
      br_avl_tree->free_list = node->right;
      br_avl_tree->count++;
    }
  else if (br_avl_tree->max_nodes == 0)
    {
      node_sz = sizeof (struct br_avl_node);

      node = (struct br_avl_node *) kmalloc (node_sz, GFP_ATOMIC);
      if (! node)
        return NULL;

      br_avl_tree->count++;
    }

  return node;
}

/* Free a AVL tree node. */
static void
_br_avl_node_free (struct br_avl_tree *br_avl_tree, struct br_avl_node *node)
{
  node->right = br_avl_tree->free_list;
  node->left = NULL;
  br_avl_tree->free_list = node;
  br_avl_tree->count--;

  return;
}

/* Free AVL free node list. */
static void
_br_avl_freelist_free (struct br_avl_tree *br_avl_tree)
{
  struct br_avl_node *node;
  struct br_avl_node *next;

  for (node = br_avl_tree->free_list; node; node = next)
    {
      next = node->right;
      if (node)
        kfree (node);
      br_avl_tree->count--;
    }
}

/* Create AVL tree. 
   If the max_nodes is 0, no preallocation is done. Every node is allocated
   first from the free_list and then from the system memory. */
int
br_avl_create (struct br_avl_tree **br_avl_tree,
                int max_nodes,
                int (*compare_function) (void *data1, void *data2))
{
  int i;
  struct br_avl_tree *avl;
  struct br_avl_node *node;
  int node_sz;

  avl = (struct br_avl_tree *) kmalloc (sizeof (struct br_avl_tree), GFP_ATOMIC);
  if (! avl)
    return -1;

  avl->max_nodes = max_nodes;
  avl->root = NULL;
  avl->compare_function = compare_function;

  /* Initialize free list. */
  avl->free_list = NULL;
  
  /* Initialize max count. */
  avl->count = max_nodes;

  /* Check if nodes are to be preallocated. */
  if (max_nodes != 0)
    {
      /* Preallocate maximum number of entries and put on free list. */
      node_sz = sizeof (struct br_avl_node);

      avl->data = (char *) kmalloc (node_sz * max_nodes, GFP_ATOMIC);
      if (! avl->data)
        {
          kfree (avl);
          return -1;
        }

      /* Add all entries in the free list. */
      for (i = 0; i < max_nodes; i++)
        {
          node = (struct br_avl_node *) (avl->data + i * node_sz);
          _br_avl_node_free (avl, node);
        }
    }

  *br_avl_tree = avl;

  return 0;
}

/* Delete AVL tree. */
static int
br_avl_finish (struct br_avl_tree *br_avl_tree)
{
  /* Check tree nodes allocation type. */
  if(br_avl_tree->max_nodes == 0)
    {
       /* Free free list. */
       _br_avl_freelist_free (br_avl_tree);
    }
  else if(br_avl_tree->data)
    {
       /* Free all preallocated nodes. */
       kfree (br_avl_tree->data);
    }
  /* Free tree. */
  kfree (br_avl_tree);

  return 0;
}

/* Internal function for traversal. */
static int
_br_avl_traverse (struct br_avl_tree *br_avl_tree,
                  struct br_avl_node *node,
                  br_avl_traversal_fn fn,
                  void *data)
{
  int  ret;
                                                                               
  if (node != NULL) 
    {
      if ((ret = _br_avl_traverse(br_avl_tree, node->left, fn, data)) < 0) 
        return ret;

      if ((ret = (*fn)(node->info, data)) < 0) 
        return ret;
                                                                               
      if ((ret = _br_avl_traverse(br_avl_tree, node->right, fn, data)) < 0) 
        return ret;
    }

  return 0;
}

/* Traverse tree. */
int
br_avl_traverse (struct br_avl_tree *br_avl_tree, br_avl_traversal_fn fn, void *data)
{
  return _br_avl_traverse (br_avl_tree, br_avl_tree->root, fn, data);
}

/* Tree top. */
struct br_avl_node *
br_avl_top (struct br_avl_tree *br_avl_tree)
{
  return br_avl_tree->root;
}

/* Internal function to lookup node. */
static struct br_avl_node*
_br_avl_lookup (struct br_avl_tree *tree, struct br_avl_node *node, void *data)
{    
  int cmp;

  if (node == NULL)
    return NULL;

  cmp = (*(tree->compare_function)) (data, node->info);
  if (cmp < 0)
    return _br_avl_lookup (tree, node->left, data);
  if (cmp > 0)
    return _br_avl_lookup (tree, node->right, data);

  return node;
}

/* Lookup AVL tree. */
struct br_avl_node *
br_avl_lookup (struct br_avl_tree *tree, void *data)
{
  if (tree->root == NULL)
    return NULL;

  return _br_avl_lookup (tree, tree->root, data);
}

/* Next node. */
struct br_avl_node* 
br_avl_next (struct br_avl_node *node)
{
  struct br_avl_node *start;
  int count = 0;

  if (node->left)
    return node->left;

  if (node->right)
    return node->right;

  start = node;
  while (node->parent)
    {
      if (node->parent->left == node && node->parent->right)
        return node->parent->right;

      count++;

      node = node->parent;
    }

  return NULL;
}

/* Get next node. */
struct br_avl_node* br_avl_getnext (struct br_avl_tree *tree, struct br_avl_node *node)
{     
  struct br_avl_node *nodeA, *nodeB;

  if (tree->root == NULL) return NULL;
  nodeA = node;
  nodeB = (nodeA == NULL ? tree->root : nodeA->right);
  if (nodeB == NULL)
    {  
      /* Go up parent from the left subtree */
      for (;;)
        {  
          nodeB = nodeA->parent;
          if (nodeB == NULL)
            break;
          if (nodeA->flag == 0)
            break;
          nodeA = nodeB;
        }
    }
  else
    {  /* go downstairs into the right subtree */
      for (;;)
        {  
          nodeA = nodeB->left;
          if (nodeA == NULL)
            break;
          nodeB = nodeA;
        }
    }

  return nodeB;
}

/* Rotate tree for balancing. */
static struct br_avl_node *
_br_avl_rotate (struct br_avl_tree *tree, struct br_avl_node *node)
{
  struct br_avl_node *nodeA, *nodeB, *nodeC, *nodeD, *nodeE, *nodeF;

  nodeB = node;

  if (nodeB->balance < 0)
    {  
      /* Negative (left) rotation */
      nodeA = nodeB->parent; 
      nodeC = nodeB->left; 
      nodeD = nodeC->right;
      if (nodeC->balance <= 0)
        {  
          /* Single negative rotation */
          if (nodeA == NULL)
            tree->root = nodeC;
          else if (nodeB->flag == 0) 
            nodeA->left = nodeC; 
          else 
            nodeA->right = nodeC;
          nodeB->rank -= nodeC->rank;
          nodeC->parent = nodeA; 
          nodeC->flag = nodeB->flag; 
          nodeC->balance++; 
          nodeC->right = nodeB;
          nodeB->parent = nodeC; 
          nodeB->flag = 1;
          nodeB->balance = (-nodeC->balance); 
          nodeB->left = nodeD;
          if (nodeD != NULL) 
            {
              nodeD->parent = nodeB;
              nodeD->flag = 0;
            }
          node = nodeC;
        }
      else
        {  
          /* Double negative rotation */
          nodeE = nodeD->left; nodeF = nodeD->right;
          if (nodeA == NULL)
            tree->root = nodeD;
          else  if (nodeB->flag == 0) 
            nodeA->left = nodeD; else nodeA->right = nodeD;
          nodeB->rank -= (nodeC->rank + nodeD->rank);
          nodeD->rank += nodeC->rank;
          nodeB->balance = (nodeD->balance >= 0 ? 0 : +1);
          nodeC->balance = (nodeD->balance <= 0 ? 0 : -1);
          nodeD->parent = nodeA; 
          nodeD->flag = nodeB->flag; 
          nodeD->balance = 0;
          nodeD->left = nodeC; 
          nodeD->right = nodeB;
          nodeB->parent = nodeD; 
          nodeB->flag = 1; 
          nodeB->left = nodeF;
          nodeC->parent = nodeD; 
          nodeC->flag = 0; 
          nodeC->right = nodeE;
          if (nodeE != NULL) 
            {
              nodeE->parent = nodeC;
              nodeE->flag = 1;
            }
          if (nodeF != NULL)
            {
              nodeF->parent = nodeB;
              nodeF->flag = 0;
            }
          node = nodeD;
        }
    }
  else
    {  
      /* Positive (right) rotation */
      nodeA = nodeB->parent; 
      nodeC = nodeB->right; 
      nodeD = nodeC->left;
      if (nodeC->balance >= 0)
        {  
          /* Single positive rotation */
          if (nodeA == NULL)
            tree->root = nodeC;
          else if (nodeB->flag == 0) 
            nodeA->left = nodeC; 
          else 
            nodeA->right = nodeC;
          nodeC->rank += nodeB->rank;
          nodeC->parent = nodeA; 
          nodeC->flag = nodeB->flag; 
          nodeC->balance--; 
          nodeC->left = nodeB;
          nodeB->parent = nodeC; 
          nodeB->flag = 0;
          nodeB->balance = (-nodeC->balance); 
          nodeB->right = nodeD;
          if (nodeD != NULL)
            {
              nodeD->parent = nodeB;
              nodeD->flag = 1;
            }
          node = nodeC;
        }
      else
        {  
          /* Double positive rotation */
          nodeE = nodeD->left; 
          nodeF = nodeD->right;
          if (nodeA == NULL)
            tree->root = nodeD;
          else if (nodeB->flag == 0) 
            nodeA->left = nodeD; 
          else 
            nodeA->right = nodeD;
          nodeC->rank -= nodeD->rank;
          nodeD->rank += nodeB->rank;
          nodeB->balance = (nodeD->balance <= 0 ? 0 : -1);
          nodeC->balance = (nodeD->balance >= 0 ? 0 : +1);
          nodeD->parent = nodeA; 
          nodeD->flag = nodeB->flag; 
          nodeD->balance = 0;
          nodeD->left = nodeB; 
          nodeD->right = nodeC;
          nodeB->parent = nodeD; 
          nodeB->flag = 0; 
          nodeB->right = nodeE;
          nodeC->parent = nodeD; 
          nodeC->flag = 1; 
          nodeC->left = nodeF;
          if (nodeE != NULL)
            {
              nodeE->parent = nodeB;
              nodeE->flag = 1;
            }
          if (nodeF != NULL)
            {
              nodeF->parent = nodeC;
              nodeF->flag = 0;
            }
          node = nodeD;
        }
    }
  return node;
}

/* Insert a node in the AVL tree. */
int
br_avl_insert (struct br_avl_tree *tree, void *data)
{
  struct br_avl_node *nodeA, *nodeB, *nodeC;
  int flag = 0;

  /* Find location for insertion. */
  nodeA = NULL;
  nodeB = tree->root;

  while (nodeB != NULL)
    {
      nodeA = nodeB;
      if (tree->compare_function (data, nodeA->info) <= 0)
        {
          flag = 0;
          nodeB = nodeA->left;
          nodeA->rank++;
        }
      else
        {
          flag = 1;
          nodeB = nodeA->right;
        }
    }

  /* Create a new node. */
  nodeC = _br_avl_node_new (tree);
  nodeC->info = data;
  nodeC->rank = 1;
  nodeC->parent = nodeA;
  nodeC->flag = (nodeA == NULL ? 0 : flag);
  nodeC->balance = 0;
  nodeC->left = nodeC->right = NULL;
  if (nodeA == NULL)
    tree->root = nodeC;
  else if (flag == 0)
    nodeA->left = nodeC;
  else
    nodeA->right = nodeC;

  /* Go parent root and correct all subtrees affected by insertion. */
  while (nodeA != NULL)
    {
      if (flag == 0)
        {
          /* Height of p's left subtree increased. */
          if (nodeA->balance > 0)
            {
              nodeA->balance = 0;
              break;
            }
          if (nodeA->balance < 0)
            {
              _br_avl_rotate (tree, nodeA);
              break;
            }
          nodeA->balance = -1;
          flag = nodeA->flag;
          nodeA = nodeA->parent;
        }
      else
        {
          /* Height of p's right subtree increased. */
          if (nodeA->balance < 0)
            {
              nodeA->balance = 0;
              break;
            }
          if (nodeA->balance > 0)
            {
              _br_avl_rotate (tree, nodeA);
              break;
            }
          nodeA->balance = +1;
          flag = nodeA->flag;
          nodeA = nodeA->parent;
        }
    }

  return 0;
}

/* Remove a node entry. */
static int
_br_avl_delete_node (struct br_avl_tree *tree, struct br_avl_node *node)
{
  struct br_avl_node *nodeA, *nodeB, *nodeC, *nodeD, *nodeE, *nodeF, *nodeG;
  int flag;

  if (node == NULL)
    return -1;

  nodeB = node;

  /* If both subtrees of the specified node are non-empty, the node
     should be interchanged with the next one, at least one subtree
     of which is always empty. */
  if (nodeB->left == NULL || nodeB->right == NULL) 
    goto SKIP;

  nodeA = nodeB->parent; 
  nodeC = nodeB->left;

  nodeD = br_avl_getnext (tree, nodeB); 
  nodeE = nodeD->right;
  if (nodeB->right == nodeD)
    {  
      if (nodeA == NULL)
        tree->root = nodeD;
      else if (nodeB->flag == 0) 
        nodeA->left = nodeD; 
      else nodeA->right = nodeD;
      nodeD->rank = nodeB->rank; 
      nodeD->parent = nodeA;
      nodeD->flag = nodeB->flag; 
      nodeD->balance = nodeB->balance;
      nodeD->left = nodeC; 
      nodeD->right = nodeB;
      nodeC->parent = nodeD;
      nodeB->rank = 1; 
      nodeB->parent = nodeD; 
      nodeB->flag = 1;
      nodeB->balance = (nodeE == NULL ? 0 : +1);
      nodeB->left = NULL; 
      nodeB->right = nodeE;
    
      if (nodeE != NULL) 
        nodeE->parent = nodeB;
    }
  else
    {  
      nodeF = nodeB->right; 
      nodeG = nodeD->parent;

      if (nodeA == NULL)
        tree->root = nodeD;
      else if (nodeB->flag == 0) 
        nodeA->left = nodeD; 
      else nodeA->right = nodeD;

      nodeD->rank = nodeB->rank; 
      nodeD->parent = nodeA;
      nodeD->flag = nodeB->flag; 
      nodeD->balance = nodeB->balance;
      nodeD->left = nodeC; 
      nodeD->right = nodeF;
      nodeC->parent = nodeD; 
      nodeF->parent = nodeD; 
      nodeG->left = nodeB;
      nodeB->rank = 1; 
      nodeB->parent = nodeG; 
      nodeB->flag = 0;
      nodeB->balance = (nodeE == NULL ? 0 : +1);
      nodeB->left = NULL; 
      nodeB->right = nodeE;
      
      if (nodeE != NULL) 
        nodeE->parent = nodeB;
    }

 SKIP:
  /* Now the specified node [p] has at least one empty subtree;
     go up to the root and adjust the rank field of all nodes
     affected by deletion */
  nodeC = nodeB; 
  nodeA = nodeC->parent;
  while (nodeA != NULL)
    {  
      if (nodeC->flag == 0) 
        nodeA->rank--;
      nodeC = nodeA; 
      nodeA = nodeC->parent;
    }

  /* delete the specified node from the tree */
  nodeA = nodeB->parent; 
  flag = nodeB->flag;
  nodeC = nodeB->left != NULL ? nodeB->left : nodeB->right;
  
  if (nodeA == NULL)
    tree->root = nodeC;
  else if (flag == 0) 
    nodeA->left = nodeC; 
  else 
    nodeA->right = nodeC;
  if (nodeC != NULL) 
    {
      nodeC->parent = nodeA;
      nodeC->flag = flag;
    }
  
  /* Go up to the root and correct all subtrees affected by
     deletion */
  while (nodeA != NULL)
    {  
      if (flag == 0)
        {  /* the height of the left subtree of [f] is decreased */
          if (nodeA->balance == 0)
            {  
              nodeA->balance = +1;
              break;
            }
          if (nodeA->balance < 0)
            nodeA->balance = 0;
          else
            {  
              nodeA = _br_avl_rotate (tree, nodeA);
              if (nodeA->balance < 0) 
                break;
            }
          flag = nodeA->flag; 
          nodeA = nodeA->parent;
        }
      else
        {  /* the height of the right subtree of [f] is decreased */
          if (nodeA->balance == 0)
            {  
              nodeA->balance = -1;
              break;
            }
          if (nodeA->balance > 0)
            nodeA->balance = 0;
          else
            {  
              nodeA = _br_avl_rotate (tree, nodeA);
              if (nodeA->balance > 0)
                break;
            }
          flag = nodeA->flag; 
          nodeA = nodeA->parent;
        }
    }

  /* Free node. */
  _br_avl_node_free (tree, nodeB);

  return 0;
}

/* Delete node containing data. */
int
br_avl_delete (struct br_avl_tree *tree, void *data)
{
  struct br_avl_node *node;
  int ret = 0;

  node =  br_avl_lookup (tree, data);
  if (node)
    {
      /* Delete node. */
      ret = _br_avl_delete_node (tree, node);      
    }
  
  return 0;
}

/* Get  AVL tree node count. */
int 
br_avl_get_tree_size(struct br_avl_tree *p_br_avl_tree)
{
  return p_br_avl_tree->count;
}


/*
  tree free function.
*/
int
br_avl_tree_free (struct br_avl_tree **pp_br_avl_tree,void (*br_avl_data_free)(void *ptr))
{
  struct br_avl_node *tmp_node;
  struct br_avl_node *node;

  if (*pp_br_avl_tree == NULL)
    return (0);

  node = br_avl_top (*pp_br_avl_tree);

  while (node)
    {
      if (node->left)
        {
          node = node->left;
          continue;
        }

      if (node->right)
        {
          node = node->right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          if (node->left == tmp_node)
            node->left = NULL;
          else
            node->right = NULL;

          /* Free node data */
          if(br_avl_data_free)
             (*br_avl_data_free)(tmp_node->info);

           /* Move node to a free list.*/  
          _br_avl_node_free (*pp_br_avl_tree, tmp_node);
        }
      else
        {
          /* Free node data */
          if(br_avl_data_free)
             (*br_avl_data_free)(tmp_node->info);

          /* Move node to a free list.*/  
          _br_avl_node_free (*pp_br_avl_tree, tmp_node);
          break;
        }
    }

  /* Delete tree. */
  br_avl_finish (*pp_br_avl_tree);
  *pp_br_avl_tree = NULL;
  return 0;
}

#ifdef DEBUG
/* Verify parent. */
void
br_avl_verify_parent (struct br_avl_node *node, struct br_avl_node *parent)
{
  if (! node)
    return;
  struct bcmif *ifp;
                                                                                
  if (node->parent != parent) {
    ifp = (struct bcmif *) node->info;
  }
  if (node->left) {
    br_avl_verify_parent (node->left, node);
  }
  if (node->right) {
    br_avl_verify_parent (node->right, node);
  }
}
#endif /* DEBUG */
