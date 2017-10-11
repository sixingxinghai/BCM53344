/* cOPYRIght (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "ptree.h"

/*
  This generic Patricia Tree structure may be used for variable length
  keys. Please note that key_len being passed to the routines is in bits,
  and not in bytes.

  Also note that this tree may not work if both IPv4 and IPv6 prefixes are
  stored in the table at the same time, since the prefix length values for
  the IPv4 and IPv6 addresses can be non-unique, and so can the bit
  representation of the address.
  If only host addresses are being stored, then this table may be used to
  store IPv4 and IPv6 addresses at the same time, since the prefix lengths
  will be 32 bits for the former, and 128 bits for the latter.
*/

/* Initialize tree. max_key_len is in bits. */
struct ptree *
ptree_init (u_int16_t max_key_len)
{
  struct ptree *tree;

  if (! max_key_len)
    return NULL;

  tree = XCALLOC (MTYPE_PTREE, sizeof (struct ptree));
  tree->max_key_len = max_key_len;

  return tree;
}

/* Free route tree. */
void
ptree_free (struct ptree *rt)
{
  struct ptree_node *tmp_node;
  struct ptree_node *node;

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

          ptree_node_free (tmp_node);
        }
      else
        {
          ptree_node_free (tmp_node);
          break;
        }
    }

  XFREE (MTYPE_PTREE, rt);
  return;
}

/* Remove route tree. */
void
ptree_finish (struct ptree *rt)
{
  ptree_free (rt);
}

int
ptree_bit_to_octets (u_int16_t key_len)
{
  return MAX ((key_len + 7) / 8, PTREE_KEY_MIN_LEN);
}

/* Set key in node. */
void
ptree_key_copy (struct ptree_node *node, u_char *key, u_int16_t key_len)
{
  int octets;

  if (key_len == 0)
    return;

  octets = ptree_bit_to_octets (key_len);
  pal_mem_cpy (PTREE_NODE_KEY (node), key, octets);
}

/* Allocate new route node. */
struct ptree_node *
ptree_node_create (u_int16_t key_len)
{
  struct ptree_node *pn;
  int octets;

  octets = ptree_bit_to_octets (key_len);

  pn = XCALLOC (MTYPE_PTREE_NODE, sizeof (struct ptree_node) + octets);
  if (! pn)
    return NULL;

  pn->key_len = key_len;

  return pn;
}

/* Utility mask array. */
static const u_char maskbit[] =
{
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
};

/* Match keys. If n includes p prefix then return TRUE else return FALSE. */
int
ptree_key_match (u_char *np, u_int16_t n_len, u_char *pp, u_int16_t p_len)
{
  int shift;
  int offset;

  if (n_len > p_len)
    return PAL_FALSE;

  offset = MIN (n_len, p_len) / 8;
  shift = MIN (n_len, p_len) % 8;

  if (shift)
    if (maskbit[shift] & (np[offset] ^ pp[offset]))
      return PAL_FALSE;

  while (offset--)
    if (np[offset] != pp[offset])
      return PAL_FALSE;

  return PAL_TRUE;
}

/* Match keys. If n includes p prefix then return TRUE else return FALSE.
 * This function supports search prefix to be smaller than the node prefix
 */
int
ptree_key_match1 (u_char *np, u_int16_t n_len, u_char *pp, u_int16_t p_len)
{
  int shift;
  int offset;

  offset = MIN (n_len, p_len) / 8;
  shift = MIN (n_len, p_len) % 8;

  if (shift)
    if (maskbit[shift] & (np[offset] ^ pp[offset]))
      return PAL_FALSE;

  while (offset--)
    if (np[offset] != pp[offset])
      return PAL_FALSE;

  return PAL_TRUE;
}

/* Allocate new route node with ptree_key set. */
struct ptree_node *
ptree_node_set (struct ptree *tree, u_char *key, u_int16_t key_len)
{
  struct ptree_node *node;

  node = ptree_node_create (key_len);
  if (! node)
    return NULL;

  /* Copy over key. */
  ptree_key_copy (node, key, key_len);
  node->tree = tree;

  return node;
}

/* Free route node. */
void
ptree_node_free (struct ptree_node *node)
{
  XFREE (MTYPE_PTREE_NODE, node);
}

/* Common ptree_key route genaration. */
static struct ptree_node *
ptree_node_common (struct ptree_node *n, u_char *pp, u_int16_t p_len)
{
  int i;
  int j;
  u_char diff;
  u_char mask;
  u_int16_t key_len;
  struct ptree_node *new;
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
  new = ptree_node_create (key_len);
  if (! new)
    return NULL;

  newp = PTREE_NODE_KEY (new);

  for (j = 0; j < i; j++)
    newp[j] = np[j];

  if (boundary)
    newp[j] = np[j] & maskbit[new->key_len % 8];

  return new;
}

/* Check bit of the ptree_key. */
int
ptree_check_bit (struct ptree *tree, u_char *p, u_int16_t key_len)
{
  int offset;
  int shift;

  pal_assert (tree->max_key_len >= key_len);

  offset = key_len / 8;
  shift = 7 - (key_len % 8);

  return (p[offset] >> shift & 1);
}

static void
ptree_set_link (struct ptree_node *node, struct ptree_node *new)
{
  int bit;

  bit = ptree_check_bit (new->tree, PTREE_NODE_KEY (new), node->key_len);

  pal_assert (bit == 0 || bit == 1);

  node->link[bit] = new;
  new->parent = node;
}

/* Lock node. */
struct ptree_node *
ptree_lock_node (struct ptree_node *node)
{
  node->lock++;
  return node;
}

/* Unlock node. */
void
ptree_unlock_node (struct ptree_node *node)
{
  node->lock--;

  if (node->lock == 0)
    ptree_node_delete (node);
}

/* Find matched ptree_key. This function supports search prefix len less
 * than the node prefix len
 */

struct ptree_node *
ptree_node_match1 (struct ptree *tree, u_char *key, u_int16_t key_len)
{
  struct ptree_node *node;
  struct ptree_node *matched;

  if (key_len > tree->max_key_len)
    return NULL;

  matched = NULL;
  node = tree->top;

  /* Walk down tree.  If there is matched route then store it to
     matched. */
  while (node && ptree_key_match1 (PTREE_NODE_KEY (node),
                                   node->key_len, key, key_len))
    {
      matched = node;
      node = node->link[ptree_check_bit (tree, key, node->key_len)];

      if (matched->key_len >= key_len)
        break;
    }

  /* If matched route found, return it. */
  if (matched)
    return ptree_lock_node (matched);

  return NULL;
}


/* Find matched ptree_key. */
struct ptree_node *
ptree_node_match (struct ptree *tree, u_char *key, u_int16_t key_len)
{
  struct ptree_node *node;
  struct ptree_node *matched;

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
           node = node->link[ptree_check_bit (tree, key, node->key_len)];
           continue;
         }

       if (!ptree_key_match (PTREE_NODE_KEY (node),
                            node->key_len, key, key_len))
         break;

      matched = node;
      node = node->link[ptree_check_bit (tree, key, node->key_len)];
    }

  /* If matched route found, return it. */
  if (matched)
    return ptree_lock_node (matched);

  return NULL;
}

struct ptree_node *
ptree_node_match_exclude (struct ptree *pt, u_char *key1, u_int16_t key_len1,
                          u_char *key2, u_int16_t key_len2)
{
  return NULL;
}

/* Lookup same ptree_key node.  Return NULL when we can't find the node. */
struct ptree_node *
ptree_node_lookup (struct ptree *tree, u_char *key, u_int16_t key_len)
{
  struct ptree_node *node;

  if (key_len > tree->max_key_len)
    return NULL;

  node = tree->top;
  while (node && node->key_len <= key_len
         && ptree_key_match (PTREE_NODE_KEY (node),
                             node->key_len, key, key_len))
    {
      if (node->key_len == key_len && node->info)
        return ptree_lock_node (node);

      node = node->link[ptree_check_bit(tree, key, node->key_len)];
    }

  return NULL;
}

/*
 * Lookup same ptree_key node in sub-tree rooted at 'start_node'.
 * Return NULL when we can't find the node.
 */
struct ptree_node *
ptree_node_sub_tree_lookup (struct ptree *tree,
                            struct ptree_node *start_node,
                            u_char *key,
                            u_int16_t key_len)
{
  struct ptree_node *node;

  if (! start_node || key_len > tree->max_key_len)
    return NULL;

  node = start_node;
  while (node && node->key_len <= key_len
         && ptree_key_match (PTREE_NODE_KEY (node),
                             node->key_len, key, key_len))
    {
      if (node->key_len == key_len && node->info)
        return ptree_lock_node (node);

      node = node->link[ptree_check_bit(tree, key, node->key_len)];
    }

  return NULL;
}

/* Add node to routing tree. */
struct ptree_node *
ptree_node_get (struct ptree *tree, u_char *key, u_int16_t key_len)
{
  struct ptree_node *new, *node, *match;

  if (key_len > tree->max_key_len)
    return NULL;

  match = NULL;
  node = tree->top;
  while (node && node->key_len <= key_len && 
         ptree_key_match (PTREE_NODE_KEY (node), node->key_len, key, key_len))
    {
      if (node->key_len == key_len)
        return ptree_lock_node (node);

      match = node;
      node = node->link[ptree_check_bit (tree, key, node->key_len)];
    }

  if (node == NULL)
    {
      new = ptree_node_set (tree, key, key_len);
      if (match)
        ptree_set_link (match, new);
      else
        tree->top = new;
    }
  else
    {
      new = ptree_node_common (node, key, key_len);
      if (! new)
        return NULL;
      new->tree = tree;
      ptree_set_link (new, node);

      if (match)
        ptree_set_link (match, new);
      else
        tree->top = new;

      if (new->key_len != key_len)
        {
          match = new;
          new = ptree_node_set (tree, key, key_len);
          ptree_set_link (match, new);
        }
    }

  ptree_lock_node (new);
  return new;
}

/* Delete node from the routing tree. */
void
ptree_node_delete (struct ptree_node *node)
{
  struct ptree_node *child, *parent;

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

  ptree_node_free (node);

  /* If parent node is stub then delete it also. */
  if (parent && parent->lock == 0)
    ptree_node_delete (parent);
}

/* Delete All Ptree Nodes */
void
ptree_node_delete_all (struct ptree *rt)
{
  struct ptree_node *tmp_node;
  struct ptree_node *node;

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

          ptree_node_free (tmp_node);
        }
      else
        {
          ptree_node_free (tmp_node);
          rt->top = NULL;
          break;
        }
    }

  return;
}

/* Get fist node and lock it.  This function is useful when one want
   to lookup all the node exist in the routing tree. */
struct ptree_node *
ptree_top (struct ptree *tree)
{
  /* If there is no node in the routing tree return NULL. */
  if (tree == NULL || tree->top == NULL)
    return NULL;

  /* Lock the top node and return it. */
  return ptree_lock_node (tree->top);
}

/* Unlock current node and lock next node then return it. */
struct ptree_node *
ptree_next (struct ptree_node *node)
{
  struct ptree_node *next, *start;

  /* Node may be deleted from ptree_unlock_node so we have to preserve
     next node's pointer. */

  if (node->p_left)
    {
      next = node->p_left;
      ptree_lock_node (next);
      ptree_unlock_node (node);
      return next;
    }
  if (node->p_right)
    {
      next = node->p_right;
      ptree_lock_node (next);
      ptree_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent)
    {
      if (node->parent->p_left == node && node->parent->p_right)
        {
          next = node->parent->p_right;
          ptree_lock_node (next);
          ptree_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  ptree_unlock_node (start);
  return NULL;
}

/* Unlock current node and lock next node until limit. */
struct ptree_node *
ptree_next_until (struct ptree_node *node, struct ptree_node *limit)
{
  struct ptree_node *next, *start;

  /* Node may be deleted from ptree_unlock_node so we have to preserve
     next node's pointer. */

  if (node->p_left)
    {
      next = node->p_left;
      ptree_lock_node (next);
      ptree_unlock_node (node);
      return next;
    }
  if (node->p_right)
    {
      next = node->p_right;
      ptree_lock_node (next);
      ptree_unlock_node (node);
      return next;
    }

  start = node;
  while (node->parent && node != limit)
    {
      if (node->parent->p_left == node && node->parent->p_right)
        {
          next = node->parent->p_right;
          ptree_lock_node (next);
          ptree_unlock_node (start);
          return next;
        }
      node = node->parent;
    }
  ptree_unlock_node (start);
  return NULL;
}

/* Check if the tree contains nodes with info set. */
int
ptree_has_info (struct ptree *tree)
{
  struct ptree_node *node;

  if (tree == NULL)
    return 0;

  node = tree->top;
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
