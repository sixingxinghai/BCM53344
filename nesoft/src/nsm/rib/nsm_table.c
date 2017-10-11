/* Copyright (C) 2001, 2002  All Rights Reserved. */

#include "pal.h"
#include "rib/nsm_table.h"

#include "ptree.h"
/*void nsm_ptree_node_delete (struct nsm_ptree_node *);*/
void nsm_ptree_table_free (struct nsm_ptree_table *);

struct nsm_ptree_table *
nsm_ptree_table_init (u_int16_t max_key_len)
{
  struct nsm_ptree_table *rt;

 if (!max_key_len)
   return NULL;

  rt = XCALLOC (MTYPE_NSM_PTREE_TABLE, sizeof (struct nsm_ptree_table));
  rt->max_key_len = max_key_len;
  return rt;
}

void
nsm_ptree_table_finish (struct nsm_ptree_table *rt)
{
  nsm_ptree_table_free (rt);
}

/* Allocate new ptree node. */
struct nsm_ptree_node *
nsm_ptree_node_create (u_int16_t key_len)
{
  struct nsm_ptree_node *pn;
  int octets;

  octets = ptree_bit_to_octets (key_len);

  pn = XCALLOC (MTYPE_NSM_PTREE_NODE, sizeof (struct nsm_ptree_node) + octets);
  if (! pn)
    return NULL;

  pn->key_len = key_len;

  return pn;

}

/* Allocate new ptree node with prefix set. */
struct nsm_ptree_node *
nsm_ptree_node_set (struct nsm_ptree_table *table, u_char *key, u_int16_t key_len)
{
  struct nsm_ptree_node *node;
  
  node = nsm_ptree_node_create (key_len);

  /* Copy over key. */
  nsm_ptree_key_copy (node, key, key_len);
  node->tree = table;

  return node;
}

/* Free ptree node. */
void
nsm_ptree_node_free (struct nsm_ptree_node *node)
{
  XFREE(MTYPE_NSM_PTREE_NODE,node);
}

void
nsm_ptree_key_copy (struct nsm_ptree_node *node, u_char *key, u_int16_t key_len)
{
  int octets;

  if (key_len == 0)
    return;

  octets = ptree_bit_to_octets (key_len);
  pal_mem_cpy (PTREE_NODE_KEY (node), key, octets);
}


/* Free ptree table. */
void
nsm_ptree_table_free (struct nsm_ptree_table *rt)
{
  struct nsm_ptree_node *tmp_node;
  struct nsm_ptree_node *node;
 
  if (rt == NULL)
    return;

  node = rt->top;

  while (node)
    {
      if (node->p_left)
        {
          node = node->p_left;
          continue;
        }

      if (node->p_right)
        {
          node = node->p_right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          if (node->p_left == tmp_node)
            node->p_left = NULL;
          else
            node->p_right = NULL;

          nsm_ptree_node_free (tmp_node);
        }
      else
        {
          nsm_ptree_node_free (tmp_node);
          break;
        }
    }
 
  XFREE(MTYPE_NSM_PTREE_TABLE,rt);
  return;
}

/* Utility mask array. */
static const u_char maskbit[] = 
{
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/* Macro version of check_bit (). */
#define CHECK_BIT(X,P) ((((u_char *)(X))[(P) / 8]) >> (7 - ((P) % 8)) & 1)


/* Macro version of set_link (). */
#define SET_LINK(X,Y) (X)->link[CHECK_BIT(&(Y)->prefix,(X)->prefixlen)] = (Y);\
                      (Y)->parent = (X)

/* Check bit of the ptree_key. */
static int
nsm_ptree_check_bit (struct nsm_ptree_table *tree, u_char *p, u_int16_t key_len)
{
  int offset;
  int shift;

  pal_assert (tree->max_key_len >= key_len);

  offset = key_len / 8;
  shift = 7 - (key_len % 8);

  return (p[offset] >> shift & 1);
}

static void
nsm_ptree_set_link (struct nsm_ptree_node *node, struct nsm_ptree_node *new)
{
   int bit;

  bit = nsm_ptree_check_bit (new->tree, PTREE_NODE_KEY(new), node->key_len);

  pal_assert (bit == 0 || bit == 1);

  node->link[bit] = new;
  new->parent = node;

}


/* Lock node. */
struct nsm_ptree_node *
nsm_ptree_lock_node (struct nsm_ptree_node *node)
{
  node->lock++;
  return node;
}

/* Unlock node. */
void
nsm_ptree_unlock_node (struct nsm_ptree_node *node)
{
  node->lock--;

  if (node->lock == 0)
    nsm_ptree_node_delete (node);
}

/* Find matched prefix. */
struct nsm_ptree_node *
nsm_ptree_node_match (struct nsm_ptree_table *tree, u_char *key, u_int16_t key_len)
{
  struct nsm_ptree_node *node;
  struct nsm_ptree_node *matched;

  if (key_len > tree->max_key_len)
    return NULL;

  matched = NULL;
  node = tree->top;

  /* Walk down tree.  If there is matched route then store it to
     matched. */
  while (node && (node->key_len <= key_len)) 
    { 
       if (node->info == NULL)
         {
           node = node->link[nsm_ptree_check_bit (tree, key, node->key_len)];
           continue;
         }

       if (!ptree_key_match (PTREE_NODE_KEY (node),
                            node->key_len, key, key_len))
         break;

      matched = node;
      node = node->link[nsm_ptree_check_bit (tree, key, node->key_len)];
    }


  /* If matched route found, return it. */
  if (matched)
    return nsm_ptree_lock_node (matched);

  return NULL;

}


struct nsm_ptree_node *
nsm_ptree_node_match_ipv4 (struct nsm_ptree_table *table, struct pal_in4_addr *addr)
{

  return nsm_ptree_node_match (table, (u_char *) addr, IPV4_MAX_PREFIXLEN);
}

#ifdef HAVE_IPV6
struct nsm_ptree_node *
nsm_ptree_node_match_ipv6 (struct nsm_ptree_table *table, struct pal_in6_addr *addr)
{
  return nsm_ptree_node_match (table, (u_char *) addr, IPV6_MAX_PREFIXLEN);
}
#endif /* HAVE_IPV6 */

/* Lookup same prefix node.  Return NULL when we can't find ptree. */
struct nsm_ptree_node *
nsm_ptree_node_lookup (struct nsm_ptree_table *table, u_char *key, u_int16_t key_len)
{
  struct nsm_ptree_node *node;

  if (key_len > table->max_key_len)
    return NULL;

  node = table->top;
  while (node && node->key_len <= key_len
         && ptree_key_match (PTREE_NODE_KEY (node),
                             node->key_len, key, key_len))
    {
      if (node->key_len == key_len && node->info)
        return nsm_ptree_lock_node (node);

      node = node->link[nsm_ptree_check_bit(table, key, node->key_len)];
    }

  return NULL;
}

struct nsm_ptree_node *
nsm_ptree_node_lookup_ipv4 (struct nsm_ptree_table *table,
                            struct pal_in4_addr *addr)
{
  return nsm_ptree_node_lookup (table, (u_char *)addr, IPV4_MAX_PREFIXLEN);
}
/* Common ptree_key route genaration. */
static struct nsm_ptree_node *
nsm_ptree_node_common (struct nsm_ptree_node *n, u_char *pp, u_int16_t p_len)
{
  int i;
  int j;
  u_char diff;
  u_char mask;
  u_int16_t key_len;
  struct nsm_ptree_node *new;
  u_char *np;
  u_char *newp;
  u_char boundary = 0;

  np = PTREE_NODE_KEY (n);

  for (i = 0; i < p_len / 8; i++)
    if (np[i] != pp[i])
      break;

  key_len = i * 8;

  if (key_len != p_len)
    {
      diff = np[i] ^ pp[i];
      mask = 0x80;
      while (key_len < p_len && !(mask & diff))
        {
          if (boundary == 0)
            boundary = 1;
          mask >>= 1;
          key_len++;
        }
    }

  /* Fill new key. */
  new = nsm_ptree_node_create (key_len);
  if (! new)
    return NULL;

  newp = PTREE_NODE_KEY (new);

  for (j = 0; j < i; j++)
    newp[j] = np[j];

  if (boundary)
    newp[j] = np[j] & maskbit[new->key_len % 8];

  return new;
}

/* Add node to routing table. */
struct nsm_ptree_node *
nsm_ptree_node_get (struct nsm_ptree_table *table, u_char *key, u_int16_t key_len)
{
  struct nsm_ptree_node *new, *node, *match;

  if (key_len > table->max_key_len)
    return NULL;

  match = NULL;
  node = table->top;
  while (node && node->key_len <= key_len && 
         ptree_key_match (PTREE_NODE_KEY (node), node->key_len, key, key_len))
    {
      if (node->key_len == key_len)
        return nsm_ptree_lock_node (node);

      match = node;
      node = node->link[nsm_ptree_check_bit (table, key, node->key_len)];
    }

  if (node == NULL)
    {
      new = nsm_ptree_node_set (table, key, key_len);
      if (match)
        nsm_ptree_set_link (match, new);
      else
        table->top = new;
    }
  else
    {
      new = nsm_ptree_node_common (node, key, key_len);
      if (! new)
        return NULL;
      new->tree = table;
      nsm_ptree_set_link (new, node);

      if (match)
        nsm_ptree_set_link (match, new);
      else
        table->top = new;

      if (new->key_len != key_len)
        {
          match = new;
          new = nsm_ptree_node_set (table, key, key_len);
          nsm_ptree_set_link (match, new);
        }
    }

  nsm_ptree_lock_node (new);
  return new;
}

struct nsm_ptree_node *
nsm_ptree_node_get_ipv4 (struct nsm_ptree_table *table,
                         struct pal_in4_addr *addr, int *rn_new)
{
  return nsm_ptree_node_get (table, (u_char *)addr, IPV4_MAX_PREFIXLEN);
}

/* Delete node from the routing table. */
void
nsm_ptree_node_delete (struct nsm_ptree_node *node)
{
  struct nsm_ptree_node *child, *parent;

  pal_assert (node->lock == 0);
  pal_assert (node->info == NULL);

  if (node->p_left && node->p_right)
    return;

  if (node->p_left)
    child = node->p_left;
  else
    child = node->p_right;

  parent = node->parent;

  if (child)
    child->parent = parent;

  if (parent)
    {
      if (parent->p_left == node)
        parent->p_left = child;
      else
        parent->p_right = child;
    }
  else
    node->tree->top = child;

  nsm_ptree_node_free (node);

  /* If parent node is stub then delete it also. */
  if (parent && parent->lock == 0)
    nsm_ptree_node_delete (parent);

}

/* Get fist node and lock it.  This function is useful when one want
   to lookup all the node exist in the routing table. */
struct nsm_ptree_node *
nsm_ptree_top (struct nsm_ptree_table *table)
{
  /* If there is no node in the routing table return NULL. */
  if (table == NULL || table->top == NULL)
    return NULL;

  /* Lock the top node and return it. */
  return nsm_ptree_lock_node (table->top);
}

/* Unlock current node and lock next node then return it. */
struct nsm_ptree_node *
nsm_ptree_next (struct nsm_ptree_node *node)
{
  struct nsm_ptree_node *next;
  struct nsm_ptree_node *start;

  /* Node may be deleted from nsm_ptree_unlock_node so we have to preserve
     next node's pointer. */

  if (node->p_left)
    {
      next = node->p_left;
      nsm_ptree_lock_node (next);
      nsm_ptree_unlock_node (node);
      return next;
    }
  if (node->p_right)
    {
      next = node->p_right;
      nsm_ptree_lock_node (next);
      nsm_ptree_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent)
    {
      if (node->parent->p_left == node && node->parent->p_right)
        {
          next = node->parent->p_right;
          nsm_ptree_lock_node (next);
          nsm_ptree_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  nsm_ptree_unlock_node (start);
  return NULL;
}

/* Unlock current node and lock next node until limit. */
struct nsm_ptree_node *
nsm_ptree_next_until (struct nsm_ptree_node *node,
                      struct nsm_ptree_node *limit)
{
  struct nsm_ptree_node *next;
  struct nsm_ptree_node *start;

  /* Node may be deleted from nsm_ptree_unlock_node so we have to preserve
     next node's pointer. */

  if (node->p_left)
    {
      next = node->p_left;
      nsm_ptree_lock_node (next);
      nsm_ptree_unlock_node (node);
      return next;
    }
  if (node->p_right)
    {
      next = node->p_right;
      nsm_ptree_lock_node (next);
      nsm_ptree_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent && node != limit)
    {
      if (node->parent->p_left == node && node->parent->p_right)
        {
          next = node->parent->p_right;
          nsm_ptree_lock_node (next);
          nsm_ptree_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  nsm_ptree_unlock_node (start);
  return NULL;
}


/* check if the table contains nodes with info set */
u_char 
nsm_ptree_table_has_info (struct nsm_ptree_table *table)
{
  struct nsm_ptree_node *node;

  if (table == NULL)
    return 0;
  
  node = table->top;

  while (node)
    {
      if (node->info)
        return 1;

      if (node->p_left)
        {
          node = node->p_left;
          continue;
        }

      if (node->p_right)
        {
          node = node->p_right;
          continue;
        }

      while (node->parent)
        {
          if (node->parent->p_left == node && node->parent->p_right)
            {
              node = node->parent->p_right;
              break;
            }
          node = node->parent;
        }
      
      if (node->parent == NULL)
        break;
    }

  return 0;
}


