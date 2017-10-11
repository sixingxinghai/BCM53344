/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "mpls_fwd.h"
#include "mpls_common.h"
#include "mpls_fib.h"
#include "mpls_hash.h"

struct mpls_hash if_hash;

/* Allocate new mpls_hash.  */
void
mpls_hash_init (uint32 (*mpls_hash_key) (struct mpls_interface *),
                int (*mpls_hash_cmp) (struct mpls_interface *, struct mpls_interface *))
{
  memset (&if_hash, 0, sizeof (struct mpls_hash));
  
  if_hash.size = MPLSHASHTABSIZE;
  if_hash.mpls_hash_key = mpls_hash_key;
  if_hash.mpls_hash_cmp = mpls_hash_cmp;
}

/* Utility function for mpls_hash_get().  When this function is specified
   as alloc_func, return arugment as it is.  This function is used for
   intern already allocated value.  */
void *
mpls_hash_alloc_intern (void *arg)
{
  return arg;
}

/* Lookup and return mpls_hash bucket in mpls_hash */
void * 
mpls_hash_get_using_key (int key)
{
  uint32 index;
  struct mpls_hash_bucket *bucket;

  index = key % if_hash.size;

  for (bucket = if_hash.index[index]; bucket != NULL;
       bucket = bucket->next) 
    if (bucket->key == key)
      return bucket->data;

  return NULL;
}

/* Lookup and return mpls_hash bucket in mpls_hash.  If there is no
   corresponding mpls_hash bucket and alloc_func is specified, create new
   mpls_hash bucket.  */
void *
mpls_hash_get (void *data, int create)
{
  uint32 key;
  uint32 index;
  struct mpls_hash_bucket *bucket;

  key = (*if_hash.mpls_hash_key) (data);
  index = key % if_hash.size;

  for (bucket = if_hash.index[index]; bucket != NULL;
       bucket = bucket->next) 
    if (bucket->key == key &&
        (*if_hash.mpls_hash_cmp) (bucket->data, data) == 1)
      return bucket->data;

  if (create == 1)
    {
      bucket = (struct mpls_hash_bucket *)
        kmalloc (sizeof (struct mpls_hash_bucket), GFP_ATOMIC);
      memset (bucket, 0, sizeof (struct mpls_hash_bucket));

      bucket->data = data;
      bucket->key = key;
      bucket->next = if_hash.index[index];
      if_hash.index[index] = bucket;
      if_hash.count++;

      return bucket->data;
    }
  return NULL;
}

/* Hash lookup.  */
void *
mpls_hash_lookup (void *data)
{
  return mpls_hash_get (data, 0 /* dont create */);
}

/* This function release registered value from specified mpls_hash.  When
   release is successfully finished, return the data pointer in the
   mpls_hash bucket.  */
void *
mpls_hash_release (void *data)
{
  void *ret;
  uint32 key;
  uint32 index;
  struct mpls_hash_bucket *bucket;
  struct mpls_hash_bucket *pp;

  key = (*if_hash.mpls_hash_key) (data);
  index = key % if_hash.size;

  for (bucket = pp = if_hash.index[index]; bucket;
       bucket = bucket->next)
    {
      if (bucket->key == key &&
          (*if_hash.mpls_hash_cmp) (bucket->data, data) == 1) 
        {
          if (bucket == pp)
            if_hash.index[index] = bucket->next;
          else
            pp->next = bucket->next;

          ret = bucket->data;
          kfree (bucket);
          if_hash.count--;
          return ret;
        }
      pp = bucket;
    }
  return NULL;
}

/* Iterator function for mpls_hash.  */
void
mpls_hash_iterate (void (*func) (struct mpls_hash_bucket *, void *),
                   void *arg)
{
  int i;
  struct mpls_hash_bucket *hb;
  
  for (i = 0; i < if_hash.size; i++)
    for (hb = if_hash.index[i]; hb; hb = hb->next)
      (*func) (hb, arg);
}

/* Clean up mpls_hash.  */
void
mpls_hash_free_all (void (* func) (void *))
{
  int i;
  struct mpls_hash_bucket *hb;
  struct mpls_hash_bucket *next;

  PRINTK_DEBUG ("Freeing all in interface hash table\n");

  if (if_hash.size <= 0)
    return;

  for (i = 0; i < if_hash.size; i++)
    {
      for (hb = if_hash.index[i]; hb; hb = next)
        {
          next = hb->next;
          
          if (func)
            (*func) (hb->data);
          
          kfree (hb);
          if_hash.count--;
        }

      if_hash.index[i] = NULL;
    }
}
