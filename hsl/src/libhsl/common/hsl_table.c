/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_error.h"

#include "hsl_table.h"

/*void hsl_route_node_delete (struct hsl_route_node *);*/
void hsl_route_table_free (struct hsl_route_table *);

struct hsl_route_table *
hsl_route_table_init (void)
{
  struct hsl_route_table *rt;

  rt = oss_malloc (sizeof (struct hsl_route_table), OSS_MEM_HEAP);
  return rt;
}

void
hsl_route_table_finish (struct hsl_route_table *rt)
{
  hsl_route_table_free (rt);
}

/* Allocate new route node. */
struct hsl_route_node *
hsl_route_node_create ()
{
  return (struct hsl_route_node *) oss_malloc (sizeof (struct hsl_route_node), OSS_MEM_HEAP);
}

/* Allocate new route node with prefix set. */
struct hsl_route_node *
hsl_route_node_set (struct hsl_route_table *table, hsl_prefix_t *prefix)
{
  struct hsl_route_node *node;
  
  node = oss_malloc (sizeof (struct hsl_route_node), OSS_MEM_HEAP);
  if (! node)
    return NULL;

  hsl_prefix_copy (&node->p, prefix);
  node->table = table;

  return node;
}

/* Free route node. */
void
hsl_route_node_free (struct hsl_route_node *node)
{
  oss_free (node, OSS_MEM_HEAP);
}

/* Free route table. */
void
hsl_route_table_free (struct hsl_route_table *rt)
{
  struct hsl_route_node *tmp_node;
  struct hsl_route_node *node;
 
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

          hsl_route_node_free (tmp_node);
        }
      else
        {
          hsl_route_node_free (tmp_node);
          break;
        }
    }
 
  oss_free (rt, OSS_MEM_HEAP);
  return;
}

/* Utility mask array. */
static const u_char maskbit[] = 
{
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/* Common prefix route genaration. */
static void
route_common (hsl_prefix_t *n, hsl_prefix_t *p, hsl_prefix_t *new)
{
  int i;
  u_char diff;
  u_char mask;

  u_char *np = (u_char *)&n->u.prefix;
  u_char *pp = (u_char *)&p->u.prefix;
  u_char *newp = (u_char *)&new->u.prefix;

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

/* Macro version of check_bit (). */
#define CHECK_BIT(X,P) ((((u_char *)(X))[(P) / 8]) >> (7 - ((P) % 8)) & 1)

/* Check bit of the prefix. */
static int
check_bit (u_char *prefix, u_char prefixlen)
{
  int offset;
  int shift;
  u_char *p = (u_char *)prefix;

  HSL_ASSERT (prefixlen <= 128);

  offset = prefixlen / 8;
  shift = 7 - (prefixlen % 8);
  
  return (p[offset] >> shift & 1);
}

/* Macro version of set_link (). */
#define SET_LINK(X,Y) (X)->link[CHECK_BIT(&(Y)->prefix,(X)->prefixlen)] = (Y);\
                      (Y)->parent = (X)

static void
set_link (struct hsl_route_node *node, struct hsl_route_node *new)
{
  int bit;
    
  bit = check_bit (&new->p.u.prefix, node->p.prefixlen);

  HSL_ASSERT (bit == 0 || bit == 1);

  node->link[bit] = new;
  new->parent = node;
}

/* Lock node. */
struct hsl_route_node *
hsl_route_lock_node (struct hsl_route_node *node)
{
  node->lock++;
  return node;
}

/* Unlock node. */
void
hsl_route_unlock_node (struct hsl_route_node *node)
{
  node->lock--;

  if (node->lock == 0)
    hsl_route_node_delete (node);
}

/* Find matched prefix. */
struct hsl_route_node *
hsl_route_node_match (struct hsl_route_table *table, hsl_prefix_t *p)
{
  struct hsl_route_node *node;
  struct hsl_route_node *matched;

  matched = NULL;
  node = table->top;

  /* Walk down tree.  If there is matched route then store it to
     matched. */
  while (node && node->p.prefixlen <= p->prefixlen && 
         hsl_prefix_match (&node->p, p))
    {
      if (node->info)
        matched = node;
      node = node->link[check_bit(&p->u.prefix, node->p.prefixlen)];
    }

  /* If matched route found, return it. */
  if (matched)
    return hsl_route_lock_node (matched);

  return NULL;
}

/* Find matched prefix, excluding the node with the exclude prefix. */
struct hsl_route_node *
hsl_route_node_match_exclude (struct hsl_route_table *table,
                          hsl_prefix_t *p,
                          hsl_prefix_t *exclude)
{
  struct hsl_route_node *node;
  struct hsl_route_node *matched;

  if (! exclude)
    return hsl_route_node_match (table, p);

  matched = NULL;
  node = table->top;

  /* Walk down tree. If there is matched route and it is not the same as
     the exclude prefix, store it in matched. */
  while (node
         && node->p.prefixlen <= p->prefixlen
         && hsl_prefix_match (&node->p, p))
    {
      if (node->info && ! hsl_prefix_same (&node->p, exclude))
        matched = node;
      node = node->link[check_bit(&p->u.prefix, node->p.prefixlen)];
    }
  
  /* If matched route found, return it. */
  if (matched)
    return hsl_route_lock_node (matched);

  return NULL;
}


struct hsl_route_node *
hsl_route_node_match_ipv4 (struct hsl_route_table *table, hsl_ipv4Address_t *addr)
{
  hsl_ipv4Prefix_t p;

  memset (&p, 0, sizeof (hsl_ipv4Prefix_t));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.address = *addr;

  return hsl_route_node_match (table, (hsl_prefix_t *) &p);
}

#ifdef HAVE_IPV6
struct hsl_route_node *
hsl_route_node_match_ipv6 (struct hsl_route_table *table, hsl_ipv6Address_t *addr)
{
  hsl_ipv6Prefix_t p;

  memset (&p, 0, sizeof (hsl_ipv6Prefix_t));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.address = *addr;

  return hsl_route_node_match (table, (hsl_prefix_t *) &p);
}
#endif /* HAVE_IPV6 */

/* Lookup same prefix node.  Return NULL when we can't find route. */
struct hsl_route_node *
hsl_route_node_lookup (struct hsl_route_table *table, hsl_prefix_t *p)
{
  struct hsl_route_node *node;

  node = table->top;

  while (node && node->p.prefixlen <= p->prefixlen && 
         hsl_prefix_match (&node->p, p))
    {
      if (node->p.prefixlen == p->prefixlen && node->info)
        return hsl_route_lock_node (node);

      node = node->link[check_bit(&p->u.prefix, node->p.prefixlen)];
    }

  return NULL;
}

struct hsl_route_node *
hsl_route_node_lookup_ipv4 (struct hsl_route_table *table, hsl_ipv4Address_t *addr)
{
  hsl_prefix_t p;

  memset (&p, 0, sizeof (hsl_prefix_t));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = *addr;

  return hsl_route_node_lookup (table, &p);
}

#ifdef HAVE_IPV6
struct hsl_route_node *
hsl_route_node_lookup_ipv6 (struct hsl_route_table *table, hsl_ipv6Address_t *addr)
{
  hsl_prefix_t p;

  memset (&p, 0, sizeof (hsl_prefix_t));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = *addr;

  return hsl_route_node_lookup (table, &p);
}
#endif /* HAVE_IPV6 */

/* Add node to routing table. */
struct hsl_route_node *
hsl_route_node_get (struct hsl_route_table *table, hsl_prefix_t *p)
{
  struct hsl_route_node *new;
  struct hsl_route_node *node;
  struct hsl_route_node *match;

  match = NULL;
  node = table->top;
  while (node && node->p.prefixlen <= p->prefixlen && 
         hsl_prefix_match (&node->p, p))
    {
      if (node->p.prefixlen == p->prefixlen)
        {
          hsl_route_lock_node (node);
          return node;
        }
      match = node;
      node = node->link[check_bit(&p->u.prefix, node->p.prefixlen)];
    }

  if (node == NULL)
    {
      new = hsl_route_node_set (table, p);

      if (! new)
        return NULL;

      if (match)
        set_link (match, new);
      else
        table->top = new;
    }
  else
    {
      new = hsl_route_node_create ();

      if (! new)
        return NULL;

      route_common (&node->p, p, &new->p);
      new->p.family = p->family;
      new->table = table;
      new->is_ecmp = HSL_FALSE;
      set_link (new, node);

      if (match)
        set_link (match, new);
      else
        table->top = new;

      if (new->p.prefixlen != p->prefixlen)
        {
          match = new;
          new = hsl_route_node_set (table, p);
          if (! new)
            {
              /* Delete the match (above created "new") node. */
              hsl_route_node_delete (match);
              return NULL;
            }
          set_link (match, new);
        }
    }
  hsl_route_lock_node (new);
  
  return new;
}

struct hsl_route_node *
hsl_route_node_get_ipv4 (struct hsl_route_table *table, hsl_ipv4Address_t *addr)
{
  hsl_prefix_t p;

  memset (&p, 0, sizeof (hsl_prefix_t));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = *addr;

  return hsl_route_node_get (table, &p);
}

#ifdef HAVE_IPV6
struct hsl_route_node *
hsl_route_node_get_ipv6 (struct hsl_route_table *table, hsl_ipv6Address_t *addr)
{
  hsl_prefix_t p;

  memset (&p, 0, sizeof (hsl_prefix_t));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.u.prefix6 = *addr;

  return hsl_route_node_get (table, &p);
}
#endif /* HAVE_IPV6 */

/* Delete node from the routing table. */
void
hsl_route_node_delete (struct hsl_route_node *node)
{
  struct hsl_route_node *child;
  struct hsl_route_node *parent;

  HSL_ASSERT (node->lock == 0);
  HSL_ASSERT (node->info == NULL);

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

  hsl_route_node_free (node);

  /* If parent node is stub then delete it also. */
  if (parent && parent->lock == 0)
    hsl_route_node_delete (parent);
}

/* Get fist node and lock it.  This function is useful when one want
   to lookup all the node exist in the routing table. */
struct hsl_route_node *
hsl_route_top (struct hsl_route_table *table)
{
  /* If there is no node in the routing table return NULL. */
  if (table->top == NULL)
    return NULL;

  /* Lock the top node and return it. */
  hsl_route_lock_node (table->top);
  return table->top;
}

/* Unlock current node and lock next node then return it. */
struct hsl_route_node *
hsl_route_next (struct hsl_route_node *node)
{
  struct hsl_route_node *next;
  struct hsl_route_node *start;

  /* Node may be deleted from hsl_route_unlock_node so we have to preserve
     next node's pointer. */

  if (node->l_left)
    {
      next = node->l_left;
      hsl_route_lock_node (next);
      hsl_route_unlock_node (node);
      return next;
    }
  if (node->l_right)
    {
      next = node->l_right;
      hsl_route_lock_node (next);
      hsl_route_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent)
    {
      if (node->parent->l_left == node && node->parent->l_right)
        {
          next = node->parent->l_right;
          hsl_route_lock_node (next);
          hsl_route_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  hsl_route_unlock_node (start);
  return NULL;
}

/* Unlock current node and lock next node until limit. */
struct hsl_route_node *
hsl_route_next_until (struct hsl_route_node *node, struct hsl_route_node *limit)
{
  struct hsl_route_node *next;
  struct hsl_route_node *start;

  /* Node may be deleted from hsl_route_unlock_node so we have to preserve
     next node's pointer. */

  if (node->l_left)
    {
      next = node->l_left;
      hsl_route_lock_node (next);
      hsl_route_unlock_node (node);
      return next;
    }
  if (node->l_right)
    {
      next = node->l_right;
      hsl_route_lock_node (next);
      hsl_route_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent && node != limit)
    {
      if (node->parent->l_left == node && node->parent->l_right)
        {
          next = node->parent->l_right;
          hsl_route_lock_node (next);
          hsl_route_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  hsl_route_unlock_node (start);
  return NULL;
}


/* check if the table contains nodes with info set */
u_char 
hsl_route_table_has_info (struct hsl_route_table *table)
{
  struct hsl_route_node *node;

  if (table == NULL)
    return 0;
  
  node = table->top;

  while (node)
    {
      if (node->info)
        return 1;

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

      while (node->parent)
        {
          if (node->parent->l_left == node && node->parent->l_right)
            {
              node = node->parent->l_right;
              break;
            }
          node = node->parent;
        }
      
      if (node->parent == NULL)
        break;
    }

  return 0;
}

/* Specify an identifier for this route table. */
void
hsl_route_table_id_set (struct hsl_route_table *table, u_int32_t id)
{
  if (table)
    table->id = id;
}
