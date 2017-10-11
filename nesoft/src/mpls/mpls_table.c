/* Copyright (C) 2001-2003  All Rights Reserved.  */

/*********************************************************************
 * The following routines are being stolen from lib/table.c,
 * since we dont want to bind the whole lib to the mpls_module,
 * cos there'll be too many issues with free and assert and all that.
 *********************************************************************/

#include "mpls_fwd.h"

#include "mpls_common.h"

#include "mpls_table.h"

void mpls_table_node_delete (struct mpls_table_node *);
void mpls_table_free (struct mpls_table *);

static u_char maskbit[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0,
                           0xf8, 0xfc, 0xfe, 0xff};

struct mpls_table *
mpls_table_init (int ident)
{
  struct mpls_table *rt;

  rt = (struct mpls_table *)
    kmalloc (sizeof (struct mpls_table), GFP_ATOMIC);
  memset (rt, 0, sizeof (struct mpls_table));

  /* Set the identifier */
  rt->ident = ident;

  /* Ref count is already zero */

  /* Return the new table */
  return rt;
}

void
mpls_table_finish (struct mpls_table *rt)
{
  mpls_table_free (rt);
}

/* Allocate new route node. */
struct mpls_table_node *
mpls_table_node_new (void)
{
  struct mpls_table_node *node;

  node = (struct mpls_table_node *)
    kmalloc (sizeof (struct mpls_table_node), GFP_ATOMIC);
  memset (node, 0, sizeof (struct mpls_table_node));

  return node;
}

/* Allocate new route node with mpls_prefix set. */
struct mpls_table_node *
mpls_table_node_set (struct mpls_table *table, struct mpls_prefix *prefix)
{
  struct mpls_table_node *node;
  
  node = (struct mpls_table_node *)
    kmalloc (sizeof (struct mpls_table_node), GFP_ATOMIC);
  memset (node, 0, sizeof (struct mpls_table_node));

  mpls_prefix_copy (&node->p, prefix);
  node->table = table;

  return node;
}

/* Free route node. */
void
mpls_table_node_free (struct mpls_table_node *node)
{
  kfree (node);
}

/* Free route table. */
void
mpls_table_free (struct mpls_table *rt)
{
  struct mpls_table_node *tmp_node;
  struct mpls_table_node *node;
 
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

          mpls_table_node_free (tmp_node);
        }
      else
        {
          mpls_table_node_free (tmp_node);
          break;
        }
    }
 
  kfree (rt);
  return;
}

/* Common mpls_prefix route genaration. */
static void
mpls_table_common (struct mpls_prefix *n, struct mpls_prefix *p,
                   struct mpls_prefix *new)
{
  int i;
  unsigned char diff;
  unsigned char mask;

  unsigned char *np = (unsigned char *)&n->u.prefix4;
  unsigned char *pp = (unsigned char *)&p->u.prefix4;
  unsigned char *newp = (unsigned char *)&new->u.prefix4;

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
}

/* Check bit of the prefix. */
static int
mpls_check_bit (unsigned char *prefix, unsigned char prefixlen)
{
  int offset;
  int shift;
  unsigned char *p = (unsigned char *)prefix;

  MPLS_ASSERT (prefixlen <= 128)

  offset = prefixlen / 8;
  shift = 7 - (prefixlen % 8);
  
  return (p[offset] >> shift & 1);
}

static void
mpls_set_link (struct mpls_table_node *node, struct mpls_table_node *new)
{
  int bit;
    
  bit = mpls_check_bit (&new->p.u.prefix, node->p.prefixlen);

  MPLS_ASSERT ((bit == 0) || (bit == 1));

  node->link[bit] = new;
  new->parent = node;
}

/* Lock node. */
struct mpls_table_node *
mpls_table_lock_node (struct mpls_table_node *node)
{
  node->lock++;
  return node;
}

/* Unlock node. */
void
mpls_table_unlock_node (struct mpls_table_node *node)
{
  node->lock--;

  if (node->lock == 0)
    mpls_table_node_delete (node);
}

/* Lookup best prefix node.  Return NULL when we can't find route. */
struct mpls_table_node *
mpls_table_node_match_ipv4 (struct mpls_table *table, uint32 daddr)
{
  struct mpls_table_node *node;
  struct mpls_table_node *matched;
  struct mpls_prefix p;

  /* Create prefix */
  p.family = AF_INET;
  p.prefixlen = 32;
  p.u.prefix4.s_addr = daddr;

  /* Get the top node of the table */
  node = table->top;

  /* Set matched to NULL */
  matched = NULL;
  
  while (node && node->p.prefixlen <= p.prefixlen &&
         mpls_prefix_match (&node->p, &p))
    {
      if (node->info)
        matched = node;
      
      node = node->link[mpls_check_bit(&p.u.prefix, node->p.prefixlen)];
    }
  
  if (matched)
    return mpls_table_lock_node (matched);

  return NULL;
}

/* Lookup same prefix node.  Return NULL when we can't find route. */
struct mpls_table_node *
mpls_table_node_lookup (struct mpls_table *table, struct mpls_prefix *p)
{
  struct mpls_table_node *node;

  /* Get the top node of the table */
  node = table->top;

  while (node && node->p.prefixlen <= p->prefixlen &&
         mpls_prefix_match (&node->p, p))
    {
      if (node->p.prefixlen == p->prefixlen && node->info)
        return mpls_table_lock_node (node);
          
      node = node->link[mpls_check_bit(&p->u.prefix, node->p.prefixlen)];
    }
  
  return NULL;
}

/* Add node to routing table. */
struct mpls_table_node *
mpls_table_node_get (struct mpls_table *table, struct mpls_prefix *p)
{
  struct mpls_table_node *new;
  struct mpls_table_node *node;
  struct mpls_table_node *match;

  match = NULL;
  node = table->top;
  while (node && node->p.prefixlen <= p->prefixlen &&
         mpls_prefix_match (&node->p, p))
    {
      if (node->p.prefixlen == p->prefixlen)
        {
          mpls_table_lock_node (node);
          return node;
        }
      match = node;
      
      node = node->link[mpls_check_bit(&p->u.prefix, node->p.prefixlen)];
    }
  
  if (node == NULL)
    {
      new = mpls_table_node_set (table, p);
      if (match)
        mpls_set_link (match, new);
      else
        table->top = new;
    }
  else
    {
      new = mpls_table_node_new ();
      mpls_table_common (&node->p, p, &new->p);
      new->p.family = p->family;
      new->table = table;
      mpls_set_link (new, node);

      if (match)
        mpls_set_link (match, new);
      else
        table->top = new;

      if (new->p.prefixlen != p->prefixlen)
        {
          match = new;
          new = mpls_table_node_set (table, p);
          mpls_set_link (match, new);
        }
    }
  mpls_table_lock_node (new);
  
  return new;
}

/* Delete node from the routing table. */
void
mpls_table_node_delete (struct mpls_table_node *node)
{
  struct mpls_table_node *child;
  struct mpls_table_node *parent;

  if (node->lock != 0)
    return;

  if (node->info != NULL)
    return;

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

  mpls_table_node_free (node);

  /* If parent node is stub then delete it also. */
  if (parent && parent->lock == 0)
    mpls_table_node_delete (parent);
}

/* Get fist node and lock it.  This function is useful when one want
   to lookup all the node exist in the routing table. */
struct mpls_table_node *
mpls_table_top (struct mpls_table *table)
{
  /* If there is no node in the routing table return NULL. */
  if (table->top == NULL)
    return NULL;

  /* Lock the top node and return it. */
  mpls_table_lock_node (table->top);
  return table->top;
}

/* Unlock current node and lock next node then return it. */
struct mpls_table_node *
mpls_table_next (struct mpls_table_node *node)
{
  struct mpls_table_node *next;
  struct mpls_table_node *start;

  /* Node may be deleted from mpls_table_unlock_node so we have to preserve
     next node's pointer. */

  if (node->l_left)
    {
      next = node->l_left;
      mpls_table_lock_node (next);
      mpls_table_unlock_node (node);
      return next;
    }
  if (node->l_right)
    {
      next = node->l_right;
      mpls_table_lock_node (next);
      mpls_table_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent)
    {
      if (node->parent->l_left == node && node->parent->l_right)
        {
          next = node->parent->l_right;
          mpls_table_lock_node (next);
          mpls_table_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  mpls_table_unlock_node (start);
  return NULL;
}

/* Helper function */
const int
mpls_prefix_match(struct mpls_prefix *n, struct mpls_prefix *p)
{
  int offset;
  int shift;

  uint8 *np = (u_char *)&n->u.prefix;
  uint8 *pp = (u_char *)&p->u.prefix;

  if (n->prefixlen > p->prefixlen)
    return 0;

  offset = n->prefixlen / PNBBY;
  shift =  n->prefixlen % PNBBY;

  if (shift)
    if (maskbit[shift] & (np[offset] ^ pp[offset]))
      return 0;

  while (offset--)
    if (np[offset] != pp[offset])
      return 0;

  return 1;
}
