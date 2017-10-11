/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "bitmap.h"
#include "prefix.h"
#include "table.h"
#include "zffs.h"

/*
  APIs are:

  1. bitmap_new ();
  2. bitmap_passive_new ();  [Currently in use by lp_client]
  3. bitmap_free ();
  4. bitmap_request_index ();
  5. bimap_release_index ();
  6. bitmap_passive_release_index (); [Currently in use by lp_client]

  The min/max range to be used is specified via the bitmap_new API.
  The max range is from 0 to ULONG_MAX.

  For optimum memory usage, please use large enough bucket sizes for
  bitmaps. The reason for this is:

  1. When a bucket (struct bitmap_block) is completely used, we will free
  the internal bit map, but will keep the bucket itself in scope.

  2. When a bucket (struct bitmap_block) is completely released, we will
  free the internal bit map, but will keep the bucket itself in scope.

  Each bitmap node has a size of 28 or 25 bytes, depending on the arch.
  Therefore, a relatively large bucket size would prove to be more
  memory efficient.
*/


/* Return corresponding bitmap block for ID. */
struct bitmap_block *
bitmap_block_get_by_id (struct bitmap *bitmap, u_int32_t id)
{
  struct bitmap_block *block;

  if (! bitmap)
    return NULL;

  for (block = bitmap->hash[BITMAP_HASH_KEY (id)]; block; block = block->next)
    if (block->id == id)
      return block;

  return NULL;
}

/* Allocate new array in bitmap block.  */
u_int32_t *
bitmap_block_create_map (struct bitmap *bitmap, u_char unused)
{
  u_int32_t *array;

  if (! bitmap)
    return NULL;

  array = XCALLOC (MTYPE_BITMAP_BLOCK_ARRAY,
                   bitmap->array_size * sizeof (u_int32_t));
  if (! array)
    return NULL;

  pal_mem_set (array, unused ? 0xff : 0,
               bitmap->array_size * sizeof (u_int32_t));

  return array;
}

/* When (re)creating a block, some bits may have to be marked as used
   beforehand, based on the index range requested. This routine will
   take care of marking all such indices as marked. */
static void
bitmap_block_preprocess (struct bitmap *bitmap, struct bitmap_block *block,
                         u_int32_t min_index, u_int32_t max_index)
{
  u_int32_t id;
  u_int32_t dirty_map_node1, dirty_map_node2;
  int dirty_int1, dirty_int2;
  int min_bit, max_bit;
  int i, j;

  /* Effected map nodes. */
  dirty_map_node1 = min_index / bitmap->block_size;
  dirty_map_node2 = max_index / bitmap->block_size;

  /* Set first free index. */
  id = block->id;
  if (id == dirty_map_node1)
    block->first_free_index = min_index;
  else
    block->first_free_index = id * bitmap->block_size;

  /* Set the total free indices for a default bitmap_block. */
  block->total_free_indices = bitmap->block_size;

  if (id != dirty_map_node1 && id != dirty_map_node2)
    {
      /* Update holder for maximum usable indices in this block. */
      block->max_usable = block->total_free_indices;

      return;
    }

  dirty_int1 = (min_index % bitmap->block_size) / INTEGER_BITS;
  dirty_int2 = (max_index % bitmap->block_size) / INTEGER_BITS;

  min_bit = min_index % INTEGER_BITS;
  max_bit = max_index % INTEGER_BITS;

  if (id == dirty_map_node1)
    {
      /* The block just created contains some indices that it cannot use.
         Clean from the start to one prior to min_index. */

      /* Mark some bits as used */
      for (i = 0; i < dirty_int1; i++)
        {
          block->map[i] = 0;
          block->total_free_indices -= INTEGER_BITS;
        }

      /* Work on dirty int seperately */
      for (j = 0; j < min_bit; j++)
        {
          UNSET_FLAG (block->map[dirty_int1], (1 << j));
          --block->total_free_indices;
        }
    }

  if (id == dirty_map_node2)
    {
      /* The block just created contains some indices that it cannot use.
         Clean from the max_index plus one to the end of this block. */

      /* For the ditry_block, clean seperately */
      for (j = max_bit+1; j < INTEGER_BITS; j++)
        {
          UNSET_FLAG (block->map[dirty_int2], (1 << j));
          --block->total_free_indices;
        }

      /* Mark the rest as used as well */
      for (i = dirty_int2 + 1; i < bitmap->array_size; i++)
        {
          block->map[i] = 0;
          block->total_free_indices -= INTEGER_BITS;
        }
    }

  /* Update holder for maximum usable indices in this block. */
  block->max_usable = block->total_free_indices;
}

/* Create a new bitmap_block. If dummy is TRUE, the new node being created
   will be marked as completely used. */
struct bitmap_block *
bitmap_block_new (struct bitmap *bitmap, u_int32_t id, u_int32_t min_index,
                  u_int32_t max_index)
{
  struct bitmap_block *block, *head;
  int arr_idx;

  /* Figure out where min_index and max_index lie. If the new id doesnt
     match, return NULL. */
  if (id > (max_index / bitmap->block_size)
      || id < (min_index / bitmap->block_size))
    return NULL;

  block = XCALLOC (MTYPE_BITMAP_BLOCK, sizeof (struct bitmap_block));
  if (! block)
    return NULL;

  block->map = bitmap_block_create_map (bitmap, PAL_TRUE);
  if (! block->map)
    {
      XFREE (MTYPE_BITMAP_BLOCK, block);
      return NULL;
    }

  /* Add this map_node to the front of the list. */
  arr_idx = BITMAP_HASH_KEY (id);
  head = bitmap->hash[arr_idx];
  if (! head)
    bitmap->hash[arr_idx] = block;
  else
    {
      while (head->next)
        head = head->next;
      head->next = block;
    }

  /* Set the id and parent. */
  block->id = id;
  block->parent = bitmap;

  /* Some indices may need to be marked as used. */
  bitmap_block_preprocess (bitmap, block, min_index, max_index);

  return block;
}

/* Update current node pointer. */
void
bitmap_update_curr_node_ptr (struct bitmap *bitmap)
{
  int i;
  struct bitmap_block *block;

  for (i = 0; i < BITMAP_HASH_SIZE; i++)
    for (block = bitmap->hash[i]; block; block = block->next)
      if (block->map && block->total_free_indices > 0)
        {
          bitmap->curr_node = block;
          return;
        }

  bitmap->curr_node = NULL;
}

/* Clean up array and update curr node. */
void
bitmap_block_mark_released (struct bitmap *bitmap, struct bitmap_block *block)
{
  if (bitmap->curr_node == block)
    bitmap_update_curr_node_ptr (bitmap);

  if (block->map)
    {
      XFREE (MTYPE_BITMAP_BLOCK_ARRAY, block->map);
      block->map = NULL;
    }
}

/* Free a given map_node and the list node where it is stored. */
void
bitmap_block_free (struct bitmap_block *block)
{
  struct bitmap *bitmap;
  struct bitmap_block *list;
  int arr_idx;

  if (! block)
    return;

  /* Find correct list. */
  bitmap = block->parent;
  arr_idx = BITMAP_HASH_KEY (block->id);
  list = bitmap->hash[arr_idx];

  if (block == list)
    bitmap->hash[arr_idx] = block->next;
  else
    {
      struct bitmap_block *tmp;
      for (tmp = list; tmp; tmp = tmp->next)
        if (tmp->next == block)
          {
            tmp->next = block->next;
            break;
          }
    }

  /* Clean up array and update curr node. */
  bitmap_block_mark_released (bitmap, block);

  XFREE (MTYPE_BITMAP_BLOCK, block);
}

/* Get bitmap node corresponding to the passed in index. */
struct bitmap_block *
bitmap_block_get (struct bitmap *bitmap, u_int32_t index)
{
  struct bitmap_block *block;
  u_int32_t node_id;

  if (! bitmap)
    return NULL;

  /* Confirm that index is legal. */
  if (index > bitmap->max_index || index < bitmap->min_index)
    return NULL;

  /* Figure out the map node number in which this index should reside. */
  node_id = index / bitmap->block_size;

  block = bitmap_block_get_by_id (bitmap, node_id);
  if (block)
    {
      if (! block->map)
        {
          block->map =
            bitmap_block_create_map (bitmap,
                                     CHECK_FLAG (block->flags,
                                                 BITMAP_BLOCK_FLAG_USED) ?
                                     PAL_FALSE : PAL_TRUE);
          if (! block->map)
            return NULL;

          /* If re-creating unused, we may need to mark some bits as used. */
          if (! CHECK_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED))
            bitmap_block_preprocess (bitmap,
                                     block,
                                     bitmap->min_index,
                                     bitmap->max_index);
        }
      return block;
    }

  return NULL;
}

/* Get the total number of indices in the list, excluding the block
   corresponding to the block id passed in. */
u_int32_t
bitmap_get_available_index_count (struct bitmap *bitmap, u_int32_t node_id)
{
  int i;
  struct bitmap_block *block;
  u_int32_t indices = 0;

  for (i = 0; i < BITMAP_HASH_SIZE; i++)
    for (block = bitmap->hash[i]; block; block = block->next)
      if (block->id != node_id)
        indices += block->total_free_indices;

  return indices;
}

/* Check if at least one index is available, excluding the node with
   id specified. */
int
bitmap_is_usable (struct bitmap *bitmap, u_int32_t id)
{
  struct bitmap_block *block;
  int i;

  for (i = 0; i < BITMAP_HASH_SIZE; i++)
    for (block = bitmap->hash[i]; block; block = block->next)
      if (block->id != id && block->map && block->total_free_indices)
        return PAL_TRUE;

  return PAL_FALSE;
}

/* Check if at least one index is available for the block specified. */
int
bitmap_is_block_usable(struct bitmap *bitmap, struct bitmap_block *block)
{
  if (block && block->map && block->total_free_indices)
    {
      return block->total_free_indices;
    }

  return 0;
}

/* Will return NULL if no nodes were found that were "marked for
   release." */
struct bitmap_block *
bitmap_use_released (struct bitmap *bitmap)
{
  int i;
  struct bitmap_block *block;

  if (! bitmap)
    return NULL;

  for (i = 0; i < BITMAP_HASH_SIZE; i++)
    for (block = bitmap->hash[i]; block; block = block->next)
      if (! block->map && ! CHECK_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED))
        if ((block->map = bitmap_block_create_map (bitmap, PAL_TRUE)) != NULL)
          {
            /* Mark any bits that should be set as used. */
            bitmap_block_preprocess (bitmap,
                                     block,
                                     bitmap->min_index,
                                     bitmap->max_index);
            return block;
          }

  return NULL;
}

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART || defined HAVE_SNMP
int
bitmap_reserve_index (struct bitmap *bitmap, u_int32_t index)
{
  int bits;
  int offset;
  int i;
  int first_bit_set;
  struct bitmap_block *block;

  if (! bitmap)
    return BITMAP_FAILURE;

  block = bitmap_block_get (bitmap, index);
  if (! block)
    return BITMAP_FAILURE;

  if (! block->map)
    return BITMAP_FAILURE;

  bits = index % INDEX_VALUE_MAX;
  offset = (index % bitmap->block_size) / INTEGER_BITS;

  /* already use this label for other LSP */
  if (! CHECK_FLAG (block->map[offset], (1 << bits)))
    return BITMAP_FAILURE;

  UNSET_FLAG (block->map[offset], (1 << bits));

  if (block->total_free_indices > 0)
    block->total_free_indices--;

  if (block->total_free_indices == 0)
    {
      SET_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED);
      
      if (block->map)
        {
          XFREE (MTYPE_BITMAP_BLOCK_ARRAY, block->map);
          block->map = NULL;
        }
      return BITMAP_SUCCESS;
    }

  for (i = offset; i < bitmap->array_size; i++)
    {
      /* Find first bit call.  */
      first_bit_set = zffs (block->map[i]);

      /* Found a bit. Figure out the index number it corresponds to. */
      if (first_bit_set)
        {
          block->first_free_index = ((bitmap->block_size * block->id)
                                     + (i * INTEGER_BITS)
                                     + first_bit_set - 1);
          return BITMAP_SUCCESS;
        }
    }

  return BITMAP_FAILURE;
}
#endif /* HAVE_RSVP_GRST || HAVE_RESTART || HAVE_SNMP*/

/* Get the first free index from a block, and update internal
   values.  */

int
bitmap_request_index (struct bitmap *bitmap, u_int32_t *index)
{
  struct bitmap_block *block;
  int bit;
  int offset;
  int first_bit_set;
  int i;

  /* Get bitmap.  */
  if (! bitmap)
    return BITMAP_FAILURE;

  /* Get bitmap block.  */
  block = bitmap->curr_node;
  if (! block)
    return BITMAP_FAILURE;

  /* This is what will be returned. */
  *index = block->first_free_index;
  block->first_free_index = 0;

  /* Index value check.  */
  if (*index > INDEX_VALUE_MAX)
    return BITMAP_FAILURE;

  /* Find the location of this index in the bitmap block. */
  bit = *index % INTEGER_BITS;
  offset = (*index % bitmap->block_size) / INTEGER_BITS;

  /* When corresponding bit is already 0 (used), something is
     wrong.  */
  if (! CHECK_FLAG (block->map[offset], (1 << bit)))
    return BITMAP_FAILURE;

  /* Set this index's corresponding bit to 0 (used).  */
  UNSET_FLAG (block->map[offset], (1 << bit));

  /* Update the total free indices counter. */
  if (block->total_free_indices > 0)
    block->total_free_indices--;

  /* Index in this block runs out. */
  if (block->total_free_indices == 0)
    {
      SET_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED);
      bitmap_block_mark_released (bitmap, block);

      /* Check if current node is set. */
      if (bitmap->curr_node)
        return BITMAP_SUCCESS;

      /* Allocate a new node and make it current. */
      if (bitmap->auto_node_create)
        {
          /* Check if any released nodes are available. */
          bitmap->curr_node = bitmap_use_released (bitmap);
          if (! bitmap->curr_node)
            bitmap->curr_node = bitmap_block_new (bitmap, ++bitmap->last_node,
                                                  bitmap->min_index,
                                                  bitmap->max_index);
          return BITMAP_SUCCESS;
        }
      else
        return BITMAP_REQUEST_NEW_ID;
    }

  /* Go through the array of integers, and find the first set bit, which
     will indicate that this bit's corresponding index is empty.  */
  for (i = offset; i < bitmap->array_size; i++)
    {
      /* Find fist bit call.  */
      first_bit_set = zffs (block->map[i]);

      /* Found a bit. Figure out the index number it corresponds to. */
      if (first_bit_set)
        {
          block->first_free_index = ((bitmap->block_size * block->id)
                                     + (i * INTEGER_BITS)
                                     + first_bit_set - 1);
          return BITMAP_SUCCESS;
        }
    }

  return BITMAP_FAILURE;
}

int
bitmap_get_next_free_index (struct bitmap *bitmap, struct bitmap_block *block,
                        u_int32_t curr_idx, u_int32_t *next_idx)
{
  int bit;
  int offset;
  int first_bit_set;
  int i;

  /* Find the location of this index in the bitmap block. */
  bit = curr_idx % INTEGER_BITS;
  offset = (curr_idx % bitmap->block_size) / INTEGER_BITS;

  if (!(block->total_free_indices > 0))
    {
      return BITMAP_GET_NEXT_INDEX_FAILED;
    }

  /* Go through the array of integers, and find the first set bit, which
     will indicate that this bit's corresponding index is empty.  */
  for (i = offset; i < bitmap->array_size; i++)
    {
      /* Find fist bit call.  */
      first_bit_set = zffs (block->map[i]);

      /* Found a bit. Figure out the index number it corresponds to. */
      if (first_bit_set)
        {
          *next_idx = ((bitmap->block_size * block->id)
                               + (i * INTEGER_BITS)
                               + first_bit_set - 1);
          return BITMAP_SUCCESS;
        }
    }
  return BITMAP_FAILURE;
}

/* Release an index. If id != 0xffffffff, free the indicated node. */
int
bitmap_passive_release_index (struct bitmap *bitmap, u_int32_t index,
                              u_int32_t *id)
{
  int bit;
  int offset;
  struct bitmap_block *block;

  /* Preset id. */
  *id = 0xffffffff;

  /* Make sure this index is not outside the allowed range. */
  if (index > INDEX_VALUE_MAX
      || index < bitmap->min_index
      || index > bitmap->max_index)
    return BITMAP_FAILURE;

  /* Get the map_node in which this index belongs. */
  block = bitmap_block_get (bitmap, index);
  if (! block)
    return BITMAP_NOT_FOUND;

  /* Get the position of this index in the block. */
  bit = index % INTEGER_BITS;
  offset = (index % bitmap->block_size) / INTEGER_BITS;

  /* Check for internal error. */
  if (CHECK_FLAG (block->map[offset], (1 << bit)))
    return BITMAP_INTERNAL_ERROR;

  /* Set this index's corresponding bit to 1. */
  SET_FLAG (block->map[offset], (1 << bit));

  if (CHECK_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED))
    UNSET_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED);

  /* Update the total_free_indices counter. */
  block->total_free_indices++;
  if (! bitmap->curr_node
#ifdef HAVE_RESTART
      && !CHECK_FLAG(block->flags, BITMAP_BLOCK_FLAG_STALE)
#endif /* HAVE_RESTART */
     )
    bitmap->curr_node = block;

  /* Now check if the first_free_index value needs to be changed. */
  if (block->first_free_index > index || block->first_free_index == 0)
    block->first_free_index = index;
  else if (block->first_free_index == index)
    return BITMAP_RE_RELEASE_RCVD;

#ifdef HAVE_RESTART
  if (CHECK_FLAG(block->flags, BITMAP_BLOCK_FLAG_STALE))
    return BITMAP_SUCCESS;
#endif /* HAVE_RESTART */

  /* Check if this map_node is completely unused. */
  if (block->total_free_indices == block->max_usable)
    {
      /* Now check if excluding this map_node, all the other map nodes have
         indices more than the min available index figure. */
      if (bitmap_is_usable (bitmap, block->id))
        {
          if (! bitmap->auto_node_create)
            {
              *id = block->id;
              bitmap_block_free (block);
            }
          else
            {
              bitmap_block_mark_released (bitmap, block);
              bitmap_update_curr_node_ptr (bitmap);
            }
        }
    }

  return BITMAP_SUCCESS;
}

/* Release an index. */
int
bitmap_release_index (struct bitmap *bitmap, u_int32_t index)
{
  u_int32_t id;

  return bitmap_passive_release_index (bitmap, index, &id);
}

/* Create new bitmap client. Supported range is 1 -- ULONG_MAX. Block size
   MUST be multiple of 32. */
struct bitmap *
bitmap_passive_new (int block_size, u_int32_t min_index, u_int32_t max_index)
{
  struct bitmap *bitmap;

  if (min_index > max_index)
    return NULL;

  /* Confirm min/max. */
  if (min_index >= INDEX_VALUE_MAX || max_index > INDEX_VALUE_MAX)
    return NULL;

  /* Check block size.  It must be multiple size of 32. */
  if (block_size == 0 || block_size % INTEGER_BITS)
    return NULL;

  /* Alloc mem. */
  bitmap = XCALLOC (MTYPE_BITMAP, sizeof (struct bitmap));
  if (! bitmap)
    return NULL;

  /* Set bucket, min and max size. */
  bitmap->block_size = block_size;
  bitmap->array_size = block_size / INTEGER_BITS;
  bitmap->min_index = min_index;
  bitmap->max_index = max_index;
  bitmap->last_node = min_index / block_size;

  return bitmap;
}

/* Create new bitmap for active use. Block size MUST be multiple of 32. */
struct bitmap *
bitmap_new (int block_size, u_int32_t min_index, u_int32_t max_index)
{
  struct bitmap *bitmap;

  bitmap = bitmap_passive_new (block_size, min_index, max_index);

  if (bitmap)
    {
      bitmap->auto_node_create = PAL_TRUE;
      bitmap->curr_node = bitmap_block_new (bitmap, bitmap->last_node,
                                            min_index, max_index);
    }
  return bitmap;
}

/* Helper routine to create a bitmap with block size 1280, min_index 1
   and max_index ULONG_MAX. */
struct bitmap *
bitmap_default_new ()
{
  return bitmap_new (1280, 1, ULONG_MAX);
}

/* Remove bitmap client. */
void
bitmap_free (struct bitmap *bitmap)
{
  int i;
  struct bitmap_block *block1, *block2;

  if (! bitmap)
    return;

  for (i = 0; i < BITMAP_HASH_SIZE; i++)
    {
      if (! bitmap->hash[i])
        continue;

      /* Free bitmap nodes. */
      for (block1 = bitmap->hash[i]; block1; block1 = block2)
        {
          block2 = block1->next;
          bitmap_block_free (block1);
        }
    }

  /* Free bitmap. */
  XFREE (MTYPE_BITMAP, bitmap);
}

/* Add id from wrapper code. */
void
bitmap_add_new_node (struct bitmap *bitmap, u_int32_t id, u_int8_t add_flags)
{
  /* If automatic node add mode return.  */
  if (bitmap->auto_node_create)
    return;

  /* If this is a on demand block add by the client, the curr_node is not
   * updated */
  if (CHECK_FLAG (add_flags, BITMAP_NODE_ADD_ON_DEMAND))
    {
      bitmap_block_new (bitmap, id, bitmap->min_index, bitmap->max_index);
      return;
    }

  /* Create new map node for the passed-in id and update curr_node. */
  bitmap->curr_node
    = bitmap_block_new (bitmap, id, bitmap->min_index, bitmap->max_index);
}

/* Indicate to bitmap that the index is being used  */
int
bitmap_add_index (struct bitmap *bitmap, u_int32_t index)
{
  struct bitmap_block *block;
  int bit;
  int offset;
  int first_bit_set;
  int i;

  /* Get bitmap.  */
  if (! bitmap)
    return BITMAP_FAILURE;

  /* Get bitmap block.  */
  block = bitmap->curr_node;
  if (! block)
    return BITMAP_FAILURE;

  /* This is what will be returned. */
  block->first_free_index = 0;

  /* Index value check.  */
  if (index > INDEX_VALUE_MAX)
    return BITMAP_FAILURE;

  /* Find the location of this index in the bitmap block. */
  bit = index % INTEGER_BITS;
  offset = (index % bitmap->block_size) / INTEGER_BITS;

  /* When corresponding bit is already 0 (used), something is
     wrong.  */
  if (! CHECK_FLAG (block->map[offset], (1 << bit)))
    return BITMAP_FAILURE;

  /* Set this index's corresponding bit to 0 (used).  */
  UNSET_FLAG (block->map[offset], (1 << bit));

  /* Update the total free indices counter. */
  if (block->total_free_indices > 0)
    block->total_free_indices--;

  /* Index in this block runs out. */
  if (block->total_free_indices == 0)
    {
      SET_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED);
      bitmap_block_mark_released (bitmap, block);

      /* Check if current node is set. */
      if (bitmap->curr_node)
        return BITMAP_SUCCESS;

      /* Allocate a new node and make it current. */
      if (bitmap->auto_node_create)
        {
          /* Check if any released nodes are available. */
          bitmap->curr_node = bitmap_use_released (bitmap);

          if (! bitmap->curr_node)
            bitmap->curr_node = bitmap_block_new (bitmap, ++bitmap->last_node,
                                                  bitmap->min_index,
                                                  bitmap->max_index);
          return BITMAP_SUCCESS;
        }
      else
        return BITMAP_REQUEST_NEW_ID;
    }

  /* Go through the array of integers, and find the first set bit, which
     will indicate that this bit's corresponding index is empty.  */
  for (i = offset; i < bitmap->array_size; i++)
    {
      /* Find fist bit call.  */
      first_bit_set = zffs (block->map[i]);

      /* Found a bit. Figure out the index number it corresponds to. */
      if (first_bit_set)
        {
          block->first_free_index = ((bitmap->block_size * block->id)
                                     + (i * INTEGER_BITS)
                                     + first_bit_set - 1);
          return BITMAP_SUCCESS;
        }
    }

  return BITMAP_FAILURE;
}

/* This function checks whether the given index within a block is available
 * for use. If it is then the corresponding bit is marked as in-use */
int
bitmap_request_index_by_block (struct bitmap *bitmap, struct bitmap_block *block,
                           u_int32_t index)
{
  int bit;
  int offset;
  int first_bit_set;
  int i;

  /* Get bitmap.  */
  if ((! bitmap) || (! block))
    {
      return BITMAP_FAILURE;
    }

  /* Index value check.  */
  if (index > INDEX_VALUE_MAX)
    {
      return BITMAP_FAILURE;
    }

  /* Find the location of this index in the bitmap block. */
  bit = index % INTEGER_BITS;
  offset = (index % bitmap->block_size) / INTEGER_BITS;

  /* When corresponding bit is already in use (0), failure  */
  if (! CHECK_FLAG (block->map[offset], (1 << bit)))
    {
      return BITMAP_FAILURE;
    }

  /* Set this index's corresponding bit to 0 (used).  */
  UNSET_FLAG (block->map[offset], (1 << bit));

  /* Update the total free indices counter. */
  if (block->total_free_indices > 0)
    {
      block->total_free_indices--;
    }

  /* Go through the array of integers, and find the first set bit, which
     will indicate that this bit's corresponding index is empty.  */
  for (i = offset; i < bitmap->array_size; i++)
    {
      /* Find fist bit call.  */
      first_bit_set = zffs (block->map[i]);

      /* Found a bit. Figure out the index number it corresponds to. */
      if (first_bit_set)
        {
          block->first_free_index = ((bitmap->block_size * block->id)
                                     + (i * INTEGER_BITS)
                                     + first_bit_set - 1);
          return BITMAP_SUCCESS;
        }
    }

  return BITMAP_FAILURE;
}

/** @brief Function to find the next free available bitmap index for the block
    @param struct bitmap *bitmap - Bitmap structure
    @param struct bitmap_block *block - block id
    @param u_int32_t index - current free
    @return First Free Index Integer value
*/
int
bitmap_request_next_free_index_by_block (struct bitmap *bitmap,
                                   struct bitmap_block *block, u_int32_t index)
{
  int bit;
  int offset;
  int first_bit_set;
  int next_free_bit;
  int i;

  /* Get bitmap.  */
  if ((! bitmap) || (! block))
    {
      return BITMAP_FAILURE;
    }

  /* Index value check.  */
  if (index > INDEX_VALUE_MAX)
    {
      return BITMAP_FAILURE;
    }

  /* Find the location of this index in the bitmap block. */
  bit = index % INTEGER_BITS;
  offset = (index % bitmap->block_size) / INTEGER_BITS;

  /* Go through the array of integers, and find the first set bit, which
     will indicate that this bit's corresponding index is empty.  */
  for (i = offset; i < bitmap->array_size; i++)
    {
      /* Find first bit call.  */
      first_bit_set = zffs (block->map[i]);

      /* Found a bit. Figure out the index number it corresponds to. */
      if (first_bit_set)
        {
          next_free_bit = ((bitmap->block_size * block->id)
                                     + (i * INTEGER_BITS)
                                     + first_bit_set - 1);
          return next_free_bit;
        }
    }

  return BITMAP_FAILURE;
}

/** @brief Function to find the first free available bitmap index for the block
    @param struct bitmap *bitmap - Bitmap structure
    @param struct bitmap_block *block - block id
    @return First Free Index Integer value
*/
int
bitmap_get_first_free_index_by_block (struct bitmap *bitmap,
                                struct bitmap_block *block)
{
  if ((! bitmap) || (! block))
      return BITMAP_FAILURE;

  /*This has the first free index value*/
  return block->first_free_index;
}

/** @brief Function to find the next free available bitmap index.
    @param struct bitmap *bitmap - Bitmap structure
    @return Free Index Integer value
*/
int
bitmap_get_first_free_index (struct bitmap *bitmap)
{
  struct bitmap_block *block;

  /* Get bitmap.  */
  if (! bitmap)
      return BITMAP_FAILURE;

  /* Get bitmap block.  */
  block = bitmap->curr_node;
  if (! block)
      return BITMAP_FAILURE;

  /*This has the first free index value*/
  return block->first_free_index;
}


/******************************************************************************
    This function is used to find whether the required number of indices are
    avaialable consecutively or not

PARAMETERS:
  IN  -->  block:            Bitmap block being looked for consecutive indices.
  IN  -->  num_idx_req:      Number of consecutive indices required
  OUT -->  On Error, a value -1
           On Success, a value of starting index for the consecutive indices
*******************************************************************************/

int
bitmap_check_consecutive_indices_bitmap_block (struct bitmap_block *block,
                                               u_int32_t num_idx_req)
{
 int starting_index;
 int free_index_count;
 int i;
 int bit;
 int offset;
 struct bitmap *bitmap; 

 bitmap = block->parent; 
 starting_index = block->first_free_index;

 if (num_idx_req == 1)
   return(starting_index);

 free_index_count = 1;

 for (i = block->first_free_index+1; 
      i < ((block->id * bitmap->block_size) + bitmap->block_size); i++)
   {
     bit = i % INTEGER_BITS;

     offset = (i % bitmap->block_size) / INTEGER_BITS;

     if (! CHECK_FLAG (block->map[offset], (1 << bit)))
       free_index_count = 0;
     else
       {
         if (!free_index_count)
          starting_index = i;
         ++free_index_count;
       }

     if (num_idx_req == free_index_count)
       break;
   }

 if (num_idx_req == free_index_count)
   return (starting_index);
 else
   return (-1);

}


/*****************************************************************************
 This function is used to find indices consecutively from current block,
 partially used block, completlu free block, or a completly new block in
 sequence.

PARAMETERS:
  IN  -->  bitmap:           Bitmap whose bitmap block indices are to be used.
  IN  -->  idx_arr           Consecutive indices returned in the array
  IN -->   num_req           Number of consecutive indices required.
  OUT -->  On Error, a value < 0
           On Success, a value of 0.
******************************************************************************/

int
bitmap_request_consecutive (struct bitmap *bitmap, u_int32_t *idx_arr,
                            u_int32_t num_req)
{
  int i;
  int ret;
  struct bitmap_block *curr_block;
  struct bitmap_block *last_block;
  struct bitmap_block *new_block;
  struct bitmap_block *block;
  u_int32_t num_bl1;
  u_int32_t num_bl2;
  u_int32_t bl1_idx;
  u_int32_t bl2_idx;
  
  if (bitmap == NULL)
    return BITMAP_FAILURE;
    
  curr_block = bitmap->curr_node;

  if (curr_block == NULL)
    return BITMAP_FAILURE;

  /* Check current block for consecutive and also adjoining next and 
     previous blocks for consecutive */

  ret = bitmap_check_block_for_consecutive (curr_block, idx_arr, num_req);

  if (ret  ==  BITMAP_SUCCESS)
    return BITMAP_SUCCESS;
 
  /* Check from the beginning of blocks till a partially used block is found*/
  for (block = last_block = bitmap_first_block (bitmap);
       block != NULL;
       block = bitmap_next_block (bitmap, block) )
    {
      last_block = block;

      if (curr_block->id == block->id)
        continue;

      if ((block->map) && (block->total_free_indices > 0))
        {
          ret = bitmap_check_block_for_consecutive (block, idx_arr, num_req);

          if (ret  ==  BITMAP_SUCCESS)
            return BITMAP_SUCCESS;
        }
    }

    /* Check from the beginning of blocks till a completly free block is found*/
   for ( block = last_block = bitmap_first_block (bitmap);
         block != NULL;
         block = bitmap_next_block (bitmap, block))
     { 
       last_block = block; 

       if (curr_block->id == block->id)
         continue;
     
      if (! block->map && ! CHECK_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED) )
        {
          if ((block->map = bitmap_block_create_map (bitmap, PAL_TRUE)) != NULL )
            {
              bitmap_block_preprocess (bitmap, block, bitmap->min_index,
                                       bitmap->max_index );

              ret = bitmap_check_block_for_consecutive (block, idx_arr,
                                                        num_req);

              if (ret  ==  BITMAP_SUCCESS)
                return BITMAP_SUCCESS;
            }
          else
            {
              XFREE (MTYPE_BITMAP_BLOCK_ARRAY, block->map);
              block->map = NULL;
            }
        }
     }

  /* No free consecutive indices in current blocks. Try to allocate a new 
  block */
  new_block = bitmap_block_new (bitmap, ++bitmap->last_node,
                                bitmap->min_index, bitmap->max_index);
  if ( new_block == NULL )
    {
      bitmap->curr_node = NULL;
      return BITMAP_FAILURE;
    }

  /* Check if end of last block and beginning of new block can be
   * consecutively combined
   */

  ret =  bitmap_two_blocks_have_consecutive (last_block, new_block, num_req,
                                             &num_bl1, &num_bl2, &bl1_idx,
                                             &bl2_idx ); 
  if (ret != BITMAP_FAILURE)
    {
      bitmap->curr_node = last_block;

      _bitmap_reserve_consecutive (last_block, bl1_idx, num_bl1);
      _bitmap_reserve_consecutive (new_block, bl2_idx, num_bl2);

      for (i = 0; i < num_bl1 + num_bl2; i++)
        idx_arr[i] = bl1_idx++;

      return BITMAP_SUCCESS;
    }

  /* Last check if the newly created block can be used */
  ret = bitmap_check_block_for_consecutive (new_block, idx_arr, num_req);
  
  if (ret  ==  BITMAP_SUCCESS)
    return BITMAP_SUCCESS;

  return BITMAP_FAILURE;

}


/******************************************************************************

 This function is used to find indices consecutively from the current bolck,
 current & next block, current & prev. block in sequence.

PARAMETERS:
  IN  -->  block             Bitmap block whose indices are to be used.
  IN  -->  idx_arr           Consecutive indices returned in the array
  IN -->   num_req           Number of consecutive indices required.
  OUT -->  On Error, a value < 0
           On Success, a value of 0.
*******************************************************************************/


int
bitmap_check_block_for_consecutive (struct bitmap_block *block,
                                    u_int32_t *idx_arr,
                                    u_int32_t num_req )
{
  int ret; 
  struct bitmap *bitmap; 
  struct bitmap_block *next_block;
  struct bitmap_block *prev_block;
  u_int32_t num_bl1;
  u_int32_t num_bl2;
  u_int32_t bl1_idx;
  u_int32_t bl2_idx;
  int i;

  bitmap = block->parent;
 
  /* A. Current block */

  ret = bitmap_current_block_has_consecutive (block, num_req, &bl1_idx);

  if (ret != BITMAP_FAILURE)
    {
      bitmap->curr_node = block;
      _bitmap_reserve_consecutive (block, bl1_idx, num_req);

      for ( i = 0; i < num_req; i++)
        idx_arr[i] = bl1_idx++;
 
      return BITMAP_SUCCESS;
    }

  /* B. Current and Next block has consectuive */

  next_block = bitmap_next_block (block->parent, block);

  if ((next_block != NULL))
    {
      if ( !next_block->map && 
           !CHECK_FLAG (next_block->flags, BITMAP_BLOCK_FLAG_USED))
        {
          if ((next_block->map = bitmap_block_create_map (bitmap, PAL_TRUE))
                                       != NULL )
            {
              bitmap_block_preprocess (bitmap, next_block, bitmap->min_index,
                                       bitmap->max_index);

              if ((ret = bitmap_two_blocks_have_consecutive (block, next_block,
                                                             num_req, &num_bl1,
                                                             &num_bl2, &bl1_idx,
                                                             &bl2_idx ))
                                                          != BITMAP_FAILURE )
                {
                  bitmap->curr_node = block;
                  _bitmap_reserve_consecutive (block, bl1_idx, num_bl1);
                  _bitmap_reserve_consecutive (next_block, bl2_idx, num_bl2);

                  for ( i = 0; i < num_bl1 + num_bl2; i ++)
                    idx_arr[i] = bl1_idx ++;

                  return BITMAP_SUCCESS;
                }
              else
                {
                  XFREE (MTYPE_BITMAP_BLOCK_ARRAY, next_block->map);
                  next_block->map = NULL;
                }

            }
        }
      else if( next_block->map ) 
        {
          if ((ret = bitmap_two_blocks_have_consecutive (block, next_block,
                                                         num_req, &num_bl1,
                                                         &num_bl2, &bl1_idx,
                                                         &bl2_idx ))
                       != BITMAP_FAILURE )
            {
              bitmap->curr_node = block;
              _bitmap_reserve_consecutive (block, bl1_idx, num_bl1);
              _bitmap_reserve_consecutive (next_block, bl2_idx, num_bl2);

              for ( i = 0; i < num_bl1 + num_bl2; i ++)
                idx_arr[i] = bl1_idx ++;

              return BITMAP_SUCCESS;
           }
        }
    }

  /* C. Current and Prev block has consectuive */
  prev_block = bitmap_prev_block ( block->parent, block );

  if ((prev_block != NULL ))
    {
      if (!prev_block->map
          && !CHECK_FLAG (prev_block->flags, BITMAP_BLOCK_FLAG_USED))
        {
          if ((prev_block->map = bitmap_block_create_map (bitmap , PAL_TRUE))
                                                                        != NULL)
            {
              bitmap_block_preprocess (bitmap, prev_block, bitmap->min_index,
                                       bitmap->max_index);

              if ((ret = bitmap_two_blocks_have_consecutive_prev (block,
                                                                  prev_block,
                                                                  num_req,
                                                                  &num_bl1,
                                                                  &num_bl2,
                                                                  &bl1_idx,
                                                                  &bl2_idx ))
                                     != BITMAP_FAILURE)
                {
                  bitmap->curr_node = block;
                  _bitmap_reserve_consecutive (block, bl1_idx, num_bl1);
                  _bitmap_reserve_consecutive (prev_block, bl2_idx, num_bl2);

                  for ( i = 0; i < num_bl1 + num_bl2; i ++)
                    idx_arr[i] = bl1_idx ++;

                  return BITMAP_SUCCESS;
                }
              else
                {
                  XFREE (MTYPE_BITMAP_BLOCK_ARRAY, prev_block->map);
                  prev_block->map = NULL;
                }

            }
        }
      else if (prev_block->map)
        {
          if ((ret = bitmap_two_blocks_have_consecutive_prev (block, prev_block,
                                                              num_req, &num_bl1,
                                                              &num_bl2, &bl1_idx,
                                                              &bl2_idx))
                                                     != BITMAP_FAILURE )
            {
              bitmap->curr_node = block;
              _bitmap_reserve_consecutive ( block, bl1_idx, num_bl1 );
              _bitmap_reserve_consecutive ( prev_block, bl2_idx, num_bl2 );

              for (i = 0; i < num_bl1 + num_bl2; i++)
                idx_arr[i] = bl1_idx ++;

              return BITMAP_SUCCESS;
            }
        }
    }

  return BITMAP_FAILURE;

}


/******************************************************************************
 This function is used to find indices consecutively from the block.

PARAMETERS:
  IN  -->  block             Bitmap block whose indices are to be used.
  IN  -->  idx               Consecutive indices returned in the array
  IN -->   num_req           Number of consecutive indices required.
  OUT -->  On Error, a value < 0
           On Success, a value of 0.
*******************************************************************************/

int
bitmap_current_block_has_consecutive (struct bitmap_block *block,
                                      u_int32_t num_req,
                                      u_int32_t *idx)
{

  int starting_index;

  if (block->total_free_indices < num_req)
    return BITMAP_FAILURE;

  starting_index = bitmap_check_consecutive_indices_bitmap_block (block,
                                                                  num_req);
  
  if ( starting_index != -1 )
    {
      *idx = starting_index;
       return BITMAP_SUCCESS;
    }
  else
    return BITMAP_FAILURE;

}


/*****************************************************************************

 This function is used to find indices consecutively from the two blocks.
 First looks in current block & then in the next block.

PARAMETERS:
  IN  -->  block1            Bitmap block whose indices are to be used.
  IN  -->  block2            Bitmap block whose indices are to be used together
                             with block1.
  IN  -->  num_bl1           Number of consecutive indices in block1   
  IN  -->  num_bl2           Number of consecutive indices in block2
  IN  -->  bl1_idx           Consecutive indices returned in the array for
                             block1
  IN  -->  bl2_idx           Consecutive indices returned in the array for
                             block2
  IN -->   num_req           Number of consecutive indices required.
  OUT -->  On Error, a value < 0
           On Success, a value of 0.
******************************************************************************/

int
bitmap_two_blocks_have_consecutive (struct bitmap_block *block1,
                                    struct bitmap_block *block2,
                                    u_int32_t num_req,
                                    u_int32_t *num_bl1,
                                    u_int32_t *num_bl2,
                                    u_int32_t *bl1_idx,
                                    u_int32_t *bl2_idx)
{
  int ret;

  /* If next block has no free indices, we cannot combine current & next */

  if ((block1->total_free_indices == 0) || (block2->total_free_indices == 0))
    return BITMAP_FAILURE;

  ret = bitmap_find_consecutive_at_block_end (block1, num_req, num_bl1,
                                              bl1_idx);

  if (ret == BITMAP_FAILURE)
    return BITMAP_FAILURE;

  ret = bitmap_find_consecutive_at_block_beginning (block2,
                                                    (num_req - *num_bl1),
                                                    num_bl2, bl2_idx);

  if (ret == BITMAP_FAILURE)
    return BITMAP_FAILURE;

  return BITMAP_SUCCESS;
}


/******************************************************************************
 This function is used to find indices consecutively from the two blocks. First
 looks in prev. block & then in the current block.

PARAMETERS:
  IN  -->  block1            Bitmap block whose indices are to be used.
  IN  -->  block2            Bitmap block whose indices are to be used together
                             with block1.
  IN  -->  num_bl1           Number of consecutive indices in block1
  IN  -->  num_bl2           Number of consecutive indices in block2
  IN  -->  bl1_idx           Consecutive indices returned in the array for
                             block1
  IN  -->  bl2_idx           Consecutive indices returned in the array for
                             block2
  IN -->   num_req           Number of consecutive indices required.
  OUT -->  On Error, a value < 0
           On Success, a value of 0.
******************************************************************************/

int
bitmap_two_blocks_have_consecutive_prev (struct bitmap_block *block1,
                                         struct bitmap_block *block2,
                                         u_int32_t num_req,
                                         u_int32_t *num_bl1,
                                         u_int32_t *num_bl2,
                                         u_int32_t *bl1_idx,
                                         u_int32_t *bl2_idx)
{
  int ret;
  
  if ((block1->total_free_indices == 0) || ( block2->total_free_indices == 0))
    return BITMAP_FAILURE;

  ret = bitmap_find_consecutive_at_block_beginning (block1, num_req,
                                                    num_bl1, bl1_idx );

  if (ret == BITMAP_FAILURE)
    return BITMAP_FAILURE;

  ret = bitmap_find_consecutive_at_block_end (block2, num_req - *num_bl1,
                                              num_bl2, bl2_idx);

  if (ret == BITMAP_FAILURE)
    return BITMAP_FAILURE;

  return BITMAP_SUCCESS;
}

/* This function finds the consecutive indices at the end of current block */

int
bitmap_find_consecutive_at_block_end (struct bitmap_block *block,
                                      u_int32_t num_req, u_int32_t *num,
                                      u_int32_t *idx )
{
  u_int32_t last_index;
  struct bitmap * bitmap;
  int bit;
  int offset;
  int i;

  bitmap = block->parent;

  last_index = (block->id * bitmap->block_size) + bitmap->block_size - 1;
 
  bit = last_index % INTEGER_BITS;
  offset = (last_index % bitmap->block_size) / INTEGER_BITS;

  if (!CHECK_FLAG (block->map[offset], (1 << bit)))
    return BITMAP_FAILURE;

  *num = 1;
  *idx = last_index;

  /* At least one bit available at the end */
  for (i = 1 ; i < num_req; i++)
    {
      last_index -- ;
      bit = last_index % INTEGER_BITS;
      offset = (last_index % bitmap->block_size) / INTEGER_BITS;

      if (!CHECK_FLAG (block->map[offset], (1 << bit)))
        break;
    
      (*num)++;
      (*idx)--;
    }

  return BITMAP_SUCCESS;
}


/* This function finds the consecutive indices at the beginning of current
   block */

int
bitmap_find_consecutive_at_block_beginning (struct bitmap_block *block,
                                            u_int32_t num_req, u_int32_t *num,
                                            u_int32_t *idx)
{
  u_int32_t first_index;
  struct bitmap * bitmap;
  int bit;
  int offset;
  int i;

  bitmap = block->parent;
  first_index =  block->id * bitmap->block_size ; 
 
  bit = first_index % INTEGER_BITS;
  offset = (first_index % bitmap->block_size) / INTEGER_BITS;

  if (!CHECK_FLAG (block->map[offset], (1 << bit)))
    return BITMAP_FAILURE;

  *num = 1;
  *idx = first_index;

  /* At least one bit available at the beginning */
  for (i = 1; i < num_req; i++)
    {
      first_index++ ;
      bit = first_index % INTEGER_BITS;
      offset = (first_index % bitmap->block_size) / INTEGER_BITS;

      if (!CHECK_FLAG (block->map[offset], (1 << bit)))
        return BITMAP_FAILURE;

      (*num)++;
    
    }

  return BITMAP_SUCCESS;
}


/* This function reserves the indices of current block as used */
void
_bitmap_reserve_consecutive (struct bitmap_block *block, u_int32_t idx,
                             u_int32_t num_req)
{
  struct bitmap * bitmap;
  int i,j;
  int bit;
  int offset;
  int first_bit_set;

  bitmap = block->parent;

  for (i = 0; i < num_req; i++)
    {
      bit = idx % INTEGER_BITS;

      offset = (idx % bitmap->block_size) / INTEGER_BITS;

      UNSET_FLAG (block->map[offset], (1 << bit));
     
      if (block->total_free_indices > 0)
        block->total_free_indices--;

      if (block->first_free_index == idx)
        {
          block->first_free_index = 0;

          for (j = offset; j < bitmap->array_size; j++)
            {
              first_bit_set = zffs (block->map[j]);

              if (first_bit_set)
                {
                  block->first_free_index = ((bitmap->block_size * block->id)
                                              + (j * INTEGER_BITS)
                                              + first_bit_set - 1);
                  break;
                }

            }
        }

      idx++;
    }

  if (block->total_free_indices == 0)
    {
      SET_FLAG (block->flags, BITMAP_BLOCK_FLAG_USED);

      bitmap_block_mark_released (bitmap, block);

      if (bitmap->curr_node)
        return ;

      if (bitmap->auto_node_create)
        {
          bitmap->curr_node = bitmap_use_released (bitmap);

          if (! bitmap->curr_node)
             bitmap->curr_node  =  bitmap_block_new (bitmap, ++bitmap->last_node,
                                                     bitmap->min_index,
                                                     bitmap->max_index );
          return;
        }
    }
 
  return;

}

/* This function is used to find the first block of the bitmap */
struct bitmap_block *
bitmap_first_block (struct bitmap *bitmap)
{
  struct bitmap_block *block;
  u_int32_t id;

  id = BITMAP_FIRST_BLOCK_ID (bitmap);

  block =  bitmap_block_get_by_id (bitmap, id);

  return block;
}

/* This function is used to find a block next to the current block */
struct bitmap_block *
bitmap_next_block (struct bitmap *bitmap, struct bitmap_block *block)
{
  struct bitmap_block *block1;

  if (BITMAP_LAST_BLOCK (bitmap, block))
    return NULL;

  block1 =  bitmap_block_get_by_id (bitmap, (block->id + 1));

  return block1;
}

/* This function is used to find a block previous to the current block*/
struct bitmap_block *
bitmap_prev_block (struct bitmap *bitmap, struct bitmap_block *block)
{
  struct bitmap_block *block1;

  if (BITMAP_FIRST_BLOCK(bitmap,block) )
    return NULL;

  block1 = bitmap_block_get_by_id (bitmap, (block->id - 1));

  return block1;
}
