/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_BITMAP_H
#define _PACOS_BITMAP_H

/* Check if all bits are in use or not. */
#define U_INT_IN_BLOCK_FULL(l) ((l) ^ 0x00000000)
#define U_INT_IN_BLOCK_NOT_EMPTY(l) ((l) | 0x00000000)

/* Size defines. */
#define INDEX_VALUE_MAX        ULONG_MAX
#define INTEGER_BITS           32
#define BITMAP_HASH_SIZE       25

/* Bitmap block.  */
struct bitmap_block
{
  /* Id. */
  u_int32_t id;

  /*  Index map. */
  u_int32_t *map;

  /* Keep track of the first free index. */
  u_int32_t first_free_index;

  /* Keep track of the number of indices available in this bitmap. */
  u_int32_t total_free_indices;

  /* Keep track of the total indices that are USABLE in this bitmap. */
  u_int32_t max_usable;

  /* Bitmap holding this node. */
  struct bitmap *parent;

  /* Next pointer. */
  struct bitmap_block *next;

  /* Flags. */
#define BITMAP_BLOCK_FLAG_USED   (1 << 0)
#ifdef HAVE_RESTART
#define BITMAP_BLOCK_FLAG_STALE  (1 << 1)
#endif /* HAVE_RESTART */
  u_char flags;
};

/* Bitmap structure. */
struct bitmap
{
  /* Each bitmap block's index size.  */
  int block_size;

  /* Each bucket's array size. This is block_size / MAX_INDICES_PER_INT. */
  int array_size;

  /* Maximum and minimum indices. */
  u_int32_t min_index;
  u_int32_t max_index;

  /* To be used for clients using LOCAL ID GENERATION. */
  u_int32_t last_node;

  /* Automatically create a node when it is needed.  */
  u_char auto_node_create;

  /* Bitmap node hash table. */
  struct bitmap_block *hash[BITMAP_HASH_SIZE];

  /* Map being worked on. */
  struct bitmap_block *curr_node;
};

#define BITMAP_HASH_KEY(i)         ((i) % BITMAP_HASH_SIZE)

/* Error/Success codes. */
#define BITMAP_SUCCESS                 0
#define BITMAP_SUCCESS_INDEX_BY_VAL    1

#define BITMAP_FAILURE                -1
#define BITMAP_REQUEST_NEW_ID         -2
#define BITMAP_NOT_FOUND              -3
#define BITMAP_INTERNAL_ERROR         -4
#define BITMAP_RE_RELEASE_RCVD        -5
#define BITMAP_GET_NEXT_INDEX_FAILED  -6

/* Bit flags for bitmap_add_new_node function */
#define BITMAP_NODE_ADD_NONE           0
#define BITMAP_NODE_ADD_ON_DEMAND     (1 << 0)

#define BITMAP_LAST_BLOCK_ID(bitmap) \
  ( bitmap->max_index / bitmap->block_size )

#define BITMAP_FIRST_BLOCK_ID(bitmap) \
  ( bitmap->min_index / bitmap->block_size )

#define BITMAP_LAST_BLOCK(bitmap,block ) \
  ( ( block->id == BITMAP_LAST_BLOCK_ID ( bitmap ) ) ? PAL_TRUE : PAL_FALSE)

#define BITMAP_FIRST_BLOCK(bitmap,block ) \
  ( ( block->id ==  BITMAP_FIRST_BLOCK_ID ( bitmap ) ) ? PAL_TRUE : PAL_FALSE)

/* Prototypes. */
struct bitmap *bitmap_new (int, u_int32_t, u_int32_t);
struct bitmap *bitmap_default_new (void);
struct bitmap *bitmap_passive_new (int, u_int32_t, u_int32_t);
struct bitmap_block *bitmap_block_get (struct bitmap *, u_int32_t);
struct bitmap_block *bitmap_block_get_by_id (struct bitmap *, u_int32_t);
int bitmap_request_index (struct bitmap *, u_int32_t *);
int bitmap_release_index (struct bitmap *, u_int32_t);
int bitmap_passive_release_index (struct bitmap *, u_int32_t, u_int32_t *);
int bitmap_request_index_by_block (struct bitmap *, struct bitmap_block *,
                               u_int32_t);
int bitmap_request_next_free_index_by_block (struct bitmap *, struct bitmap_block *,
                                       u_int32_t);
int bitmap_get_first_free_index_by_block (struct bitmap *, struct bitmap_block *);

int bitmap_add_index (struct bitmap *, u_int32_t);
void bitmap_free (struct bitmap *);
void bitmap_block_free (struct bitmap_block *);
void bitmap_add_new_node (struct bitmap *, u_int32_t, u_int8_t);
u_int32_t bitmap_get_available_index_count (struct bitmap *, u_int32_t);
int bitmap_is_usable (struct bitmap *, u_int32_t);
int bitmap_is_block_usable(struct bitmap *, struct bitmap_block *);
int bitmap_get_first_free_index (struct bitmap *bitmap);

#if defined HAVE_RSVP_GRST || defined HAVE_RESTART || defined HAVE_SNMP || defined (HAVE_MSTPD)
int bitmap_reserve_index (struct bitmap *, u_int32_t);
#endif /* HAVE_RSVP_GRST || HAVE_RESTART || HAVE_SNMP || HAVE_MSTPD */

int bitmap_check_consecutive_indices_bitmap_block (struct bitmap_block *,
                                                   u_int32_t);

int bitmap_request_consecutive (struct bitmap *, u_int32_t *, u_int32_t);

int bitmap_check_block_for_consecutive (struct bitmap_block *, u_int32_t *,
                                        u_int32_t);

int bitmap_current_block_has_consecutive (struct bitmap_block *, u_int32_t,
                                          u_int32_t * );

int bitmap_two_blocks_have_consecutive (struct bitmap_block *,
                                        struct bitmap_block *,
                                        u_int32_t, u_int32_t *, u_int32_t *,
                                        u_int32_t *, u_int32_t *);

int bitmap_find_consecutive_at_block_end (struct bitmap_block *, u_int32_t,
                                         u_int32_t *, u_int32_t *);

int bitmap_find_consecutive_at_block_beginning (struct bitmap_block *,
                                                u_int32_t, u_int32_t *,
                                                u_int32_t * );

void _bitmap_reserve_consecutive (struct bitmap_block *, u_int32_t , u_int32_t);

struct bitmap_block *bitmap_first_block (struct bitmap * );

struct bitmap_block *bitmap_next_block (struct bitmap *,
                                        struct bitmap_block *);

struct bitmap_block *bitmap_prev_block (struct bitmap *,
                                        struct bitmap_block *);

int bitmap_two_blocks_have_consecutive_prev (struct bitmap_block *,
                                         struct bitmap_block *, u_int32_t ,
                                         u_int32_t *, u_int32_t *, u_int32_t *,
                                         u_int32_t *);


#endif /* _PACOS_BITMAP_H */
