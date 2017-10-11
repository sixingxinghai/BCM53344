/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "ls_prefix.h"
#include "ls_table.h"

void ls_table_free (struct ls_table *);
int ls_node_info_exist (struct ls_node *);


/* Utility mask array. */
static u_char maskbit[] = 
{
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};


struct ls_table *
ls_table_init (u_char prefixsize, u_char slotsize)
{
  struct ls_table *rt;

  rt = XMALLOC (MTYPE_LS_TABLE, LS_TABLE_SIZE (slotsize));
  pal_mem_set (rt, 0, LS_TABLE_SIZE (slotsize));
  rt->prefixsize = prefixsize;
  rt->slotsize = slotsize;
  
  return rt;
}

void
ls_table_finish (struct ls_table *rt)
{
  ls_table_free (rt);
}

/* Allocate new route node. */
struct ls_node *
ls_node_create (struct ls_table *table)
{
  return (struct ls_node *) XCALLOC (MTYPE_LS_NODE, LS_NODE_SIZE (table));
}

/* Allocate new route node with allocating new ls_prefix. */
struct ls_node *
ls_node_set (struct ls_table *table, struct ls_prefix *prefix)
{
  struct ls_node *node;
  
  pal_assert (table->prefixsize == prefix->prefixsize);

  node = XCALLOC (MTYPE_LS_NODE, LS_NODE_SIZE (table));

  node->p = ls_prefix_new (prefix->prefixsize);
  ls_prefix_copy (node->p, prefix);
  node->table = table;

  return node;
}

/* Free route node. */
void
ls_node_free (struct ls_node *node)
{
  ls_prefix_free (node->p);
  XFREE (MTYPE_LS_NODE, node);
}

/* Free route table. */
void
ls_table_free (struct ls_table *rt)
{
  struct ls_node *tmp_node;
  struct ls_node *node;
 
  if (rt == NULL)
    return;

  node = rt->top;

  while (node)
    {
      if (node->l_left)
        {
          node = node->l_left;
          continue;
        }

      if (node->l_right)
        {
          node = node->l_right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          if (node->l_left == tmp_node)
            node->l_left = NULL;
          else
            node->l_right = NULL;

          ls_node_free (tmp_node);
        }
      else
        {
          ls_node_free (tmp_node);
          break;
        }
    }
 
  XFREE (MTYPE_LS_TABLE, rt);
  return;
}

/* Common prefix route generation. */
struct ls_prefix *
ls_route_common (struct ls_prefix *n, struct ls_prefix *p)
{
  int i;
  u_char diff;
  u_char mask;

  struct ls_prefix *new;

  u_char *np = n->prefix;
  u_char *pp = p->prefix;
  u_char *newp;

  pal_assert (n->prefixsize == p->prefixsize);

  new = ls_prefix_new (n->prefixsize);
  newp = new->prefix;

  for (i = 0; i < p->prefixlen / 8; i++)
    {
      if (np[i] == pp[i])
        newp[i] = np[i];
      else
        break;
    }

  new->prefixlen = i * 8;

  if (new->prefixlen != p->prefixlen)
    {
      diff = np[i] ^ pp[i];
      mask = 0x80;
      while (new->prefixlen < p->prefixlen && !(mask & diff))
        {
          mask >>= 1;
          new->prefixlen++;
        }
      newp[i] = np[i] & maskbit[new->prefixlen % 8];
    }

  return new;
}

/* Macro version of check_bit (). */
#define CHECK_BIT(X,P) ((((u_char *)(X))[(P) / 8]) >> (7 - ((P) % 8)) & 1)

/* Check bit of the prefix. */
static int
check_bit (u_char *prefix, u_char prefixlen)
{
  int offset;
  int shift;
  u_char *p = (u_char *)prefix;

  /*  assert (prefixlen <= 128); */

  offset = prefixlen / 8;
  shift = 7 - (prefixlen % 8);
  
  return (p[offset] >> shift & 1);
}

/* Macro version of set_link (). */
#define SET_LINK(X,Y) (X)->link[CHECK_BIT(&(Y)->prefix,(X)->prefixlen)] = (Y);\
                      (Y)->parent = (X)

static void
set_link (struct ls_node *node, struct ls_node *new)
{
  int bit;
    
  bit = check_bit (new->p->prefix, node->p->prefixlen);

  pal_assert (bit == 0 || bit == 1);

  node->link[bit] = new;
  new->parent = node;
}

/* Lock node. */
struct ls_node *
ls_lock_node (struct ls_node *node)
{
  node->lock++;
  return node;
}

/* Unlock node. */
void
ls_unlock_node (struct ls_node *node)
{
  node->lock--;

  if (node->lock == 0)
    ls_node_delete (node);
}

/* Find matched prefix. */
struct ls_node *
ls_node_match (struct ls_table *table, struct ls_prefix *p)
{
  struct ls_node *node;
  struct ls_node *matched;

  matched = NULL;
  node = table->top;

  /* Walk down tree.  If there is matched route then store it to
     matched. */
  while (node && node->p->prefixlen <= p->prefixlen && 
         ls_prefix_match (node->p, p))
    {
      if (node->vinfo0)
        matched = node;
      node = node->link[check_bit (p->prefix, node->p->prefixlen)];
    }

  /* If matched route found, return it. */
  if (matched)
    return ls_lock_node (matched);

  return NULL;
}

/* Lookup same prefix node.  Return NULL when we can't find route. */
struct ls_node *
ls_node_lookup (struct ls_table *table, struct ls_prefix *p)
{
  struct ls_node *node;

  node = table->top;

  while (node && node->p->prefixlen <= p->prefixlen && 
         ls_prefix_match (node->p, p))
    {
      if (node->p->prefixlen == p->prefixlen && ls_node_info_exist (node))
        return ls_lock_node (node);

      node = node->link[check_bit (p->prefix, node->p->prefixlen)];
    }

  return NULL;
}

struct ls_node *
ls_node_lookup_first (struct ls_table *table)
{
  struct ls_node *node;

  for (node = ls_table_top (table); node; node = ls_route_next (node))
    if (node->vinfo0)
      return node;

  return NULL;
}

/* Add node to routing table. */
struct ls_node *
ls_node_get (struct ls_table *table, struct ls_prefix *p)
{
  struct ls_node *new;
  struct ls_node *node;
  struct ls_node *match;

  match = NULL;
  node = table->top;
  while (node && node->p->prefixlen <= p->prefixlen && 
         ls_prefix_match (node->p, p))
    {
      if (node->p->prefixlen == p->prefixlen)
        {
          ls_lock_node (node);
          return node;
        }
      match = node;
      node = node->link[check_bit (p->prefix, node->p->prefixlen)];
    }

  if (node == NULL)
    {
      new = ls_node_set (table, p);
      if (match)
        set_link (match, new);
      else
        table->top = new;
    }
  else
    {
      new = ls_node_create (table);
      new->p = ls_route_common (node->p, p);
      new->p->family = p->family;
      new->table = table;
      set_link (new, node);

      if (match)
        set_link (match, new);
      else
        table->top = new;

      if (new->p->prefixlen != p->prefixlen)
        {
          match = new;
          new = ls_node_set (table, p);
          set_link (match, new);
        }
    }
  ls_lock_node (new);
  
  return new;
}

/* Delete node from the routing table. */
void
ls_node_delete (struct ls_node *node)
{
  struct ls_node *child;
  struct ls_node *parent;

  pal_assert (node->lock == 0);
  pal_assert (node->vinfo0 == NULL);
  /*  assert (node->vinfo1 == NULL); */

  if (node->l_left && node->l_right)
    return;

  if (node->l_left)
    child = node->l_left;
  else
    child = node->l_right;

  parent = node->parent;

  if (child)
    child->parent = parent;

  if (parent)
    {
      if (parent->l_left == node)
        parent->l_left = child;
      else
        parent->l_right = child;
    }
  else
    node->table->top = child;

  ls_node_free (node);

  /* If parent node is stub then delete it also. */
  if (parent && parent->lock == 0)
    ls_node_delete (parent);
}

/* Get first node and lock it.  This function is useful when one want
   to lookup all the node exist in the routing table. */
struct ls_node *
ls_table_top (struct ls_table *table)
{
  /* If there is no node in the routing table return NULL. */
  if (table == NULL || table->top == NULL)
    return NULL;

  /* Lock the top node and return it. */
  ls_lock_node (table->top);
  return table->top;
}

/* Unlock current node and lock next node then return it. */
struct ls_node *
ls_route_next (struct ls_node *node)
{
  struct ls_node *next;
  struct ls_node *start;

  /* Node may be deleted from ls_unlock_node so we have to preserve
     next node's pointer. */

  if (node->l_left)
    {
      next = node->l_left;
      ls_lock_node (next);
      ls_unlock_node (node);
      return next;
    }
  if (node->l_right)
    {
      next = node->l_right;
      ls_lock_node (next);
      ls_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent)
    {
      if (node->parent->l_left == node && node->parent->l_right)
        {
          next = node->parent->l_right;
          ls_lock_node (next);
          ls_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  ls_unlock_node (start);
  return NULL;
}

/* Unlock current node and lock next node until limit. */
struct ls_node *
ls_route_next_until (struct ls_node *node, struct ls_node *limit)
{
  struct ls_node *next;
  struct ls_node *start;

  /* Node may be deleted from ls_unlock_node so we have to preserve
     next node's pointer. */

  if (node->l_left)
    {
      next = node->l_left;
      ls_lock_node (next);
      ls_unlock_node (node);
      return next;
    }
  if (node->l_right)
    {
      next = node->l_right;
      ls_lock_node (next);
      ls_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent && node != limit)
    {
      if (node->parent->l_left == node && node->parent->l_right)
        {
          next = node->parent->l_right;
          ls_lock_node (next);
          ls_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  ls_unlock_node (start);
  return NULL;
}

int
ls_node_info_exist (struct ls_node *node)
{
  int index;

  for (index = 0; index < node->table->slotsize; index++)
    if (node->vinfo[index])
      return 1;

  return 0;
}

void *
ls_node_info_set (struct ls_node *node, int index, void *info)
{
  void *old = node->vinfo[index];

  if (old == NULL)
    {
      ls_lock_node (node);
      node->table->count[index]++;
    }

  node->vinfo[index] = info;

  if (info == NULL)
    {
      node->table->count[index]--;
      ls_unlock_node (node);
    }

  return old;
}

void *
ls_node_info_unset (struct ls_node *node, int index)
{
  void *old = node->vinfo[index];

  if (node->vinfo[index] != NULL)
    {
      node->vinfo[index] = NULL;
      node->table->count[index]--;
      ls_unlock_node (node);
    }

  return old;
}

/* Return ls_node only if specified info exists. */
struct ls_node *
ls_node_info_lookup (struct ls_table *table, struct ls_prefix *p, int index)
{
  struct ls_node *node;
  pal_assert (table->slotsize > index);

  node = ls_node_lookup (table, p);
  if (node)
    {
      if (node->vinfo[index])
        return node;

      ls_unlock_node (node);
    }
  return NULL;
}
