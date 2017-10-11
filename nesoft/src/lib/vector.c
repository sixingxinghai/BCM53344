/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "vector.h"

/* Initialize vector : allocate memory and return vector. */
vector
vector_init (u_int32_t size)
{
  vector v = XMALLOC (MTYPE_VECTOR, sizeof (struct _vector));

  /* allocate at least one slot */
  if (size == 0)
    size = 1;

  v->alloced = size;
  v->max = 0;
  v->index = XMALLOC (MTYPE_VECTOR_INDEX, sizeof (void *) * size);
  pal_mem_set (v->index, 0, sizeof (void *) * size);
  return v;
}

void
vector_only_wrapper_free (vector v)
{
  XFREE (MTYPE_VECTOR, v);
}

void
vector_free (vector v)
{
  XFREE (MTYPE_VECTOR_INDEX, v->index);
  XFREE (MTYPE_VECTOR, v);
}

vector
vector_copy (vector v)
{
  u_int32_t size;
  vector new = XMALLOC (MTYPE_VECTOR, sizeof (struct _vector));

  new->max = v->max;
  new->alloced = v->alloced;

  size = sizeof (void *) * (v->alloced);
  new->index = XMALLOC (MTYPE_VECTOR_INDEX, size);
  pal_mem_cpy (new->index, v->index, size);

  return new;
}

/* Check assigned index, and if it runs short double index pointer */
void
vector_ensure (vector v, u_int32_t num)
{
  if (v->alloced > num)
    return;

  v->index = XREALLOC (MTYPE_VECTOR_INDEX, v->index,
                       sizeof (void*) * (v->alloced * 2));
  pal_mem_set (&v->index[v->alloced], 0, sizeof (void *) * v->alloced);
  v->alloced *= 2;

  if (v->alloced <= num)
    vector_ensure (v, num);
}

/* This function only returns next empty slot index.  It dose not mean
   the slot's index memory is assigned, please call vector_ensure()
   after calling this function. */
u_int32_t
vector_empty_slot (vector v)
{
  u_int32_t i;

  if (v->max == 0)
    return 0;

  for (i = 0; i < v->max; i++)
    if (v->index[i] == 0)
      return i;

  return i;
}

/* Set value to the smallest empty slot. */
u_int32_t
vector_set (vector v, void *val)
{
  u_int32_t i;

  i = vector_empty_slot (v);
  vector_ensure (v, i);

  v->index[i] = val;

  if (v->max <= i)
    v->max = i + 1;

  return i;
}

/* Set value to specified index slot. */
u_int32_t
vector_set_index (vector v, u_int32_t i, void *val)
{
  vector_ensure (v, i);

  v->index[i] = val;

  if (v->max <= i)
    v->max = i + 1;

  return i;
}

/* Lookup vector, ensure it. */
void *
vector_lookup_index (vector v, u_int32_t i)
{
  vector_ensure (v, i);
  return v->index[i];
}

/* Unset value at specified index slot. */
void
vector_unset (vector v, u_int32_t i)
{
  if (i >= v->alloced)
    return;

  v->index[i] = NULL;

  if (i + 1 == v->max)
    {
      v->max--;
      while (i && v->index[--i] == NULL && v->max--)
        ;
    }
}

/* Count the number of not emplty slot. */
u_int32_t
vector_count (vector v)
{
  u_int32_t i;
  unsigned count = 0;

  for (i = 0; i < v->max; i++)
    if (v->index[i] != NULL)
      count++;

  return count;
}

/* Add vector src items to vector dest.  */
void
vector_add (vector dest, vector src)
{
  int i;
  void *val;

  for (i = 0; i < vector_max (src); i++)
    if ((val = vector_slot (src, i)))
      vector_set (dest, val);
}

/* Reset dest before vector add.  */
void
vector_dup (vector dest, vector src)
{
  vector_reset (dest);
  vector_add (dest, src);
}

ZRESULT
vector_walk(vector v, VECTOR_WALK_CB user_cb, intptr_t user_ref)
{
  int i;
  void *val;

  for (i = 0; i < vector_max (v); i++)
    if ((val = vector_slot (v, i))!= NULL)
      if (user_cb)
        user_cb(val, user_ref);

  return ZRES_OK;
}

