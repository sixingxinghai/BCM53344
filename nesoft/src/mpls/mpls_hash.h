/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_MPLS_HASH_H
#define _PACOS_MPLS_HASH_H

/* Default hash table size.  */ 
#define MPLSHASHTABSIZE     256

struct mpls_hash_bucket
{
  /* Linked list.  */
  struct mpls_hash_bucket *next;

  /* Hash key. */
  uint32 key;

  /* Data.  */
  void *data;
};

struct mpls_hash
{
  /* Hash bucket. */
  /*struct mpls_hash_bucket **index;*/
  struct mpls_hash_bucket *index [MPLSHASHTABSIZE];

  /* Hash table size. */
  uint32 size;

  /* Key make function. */
  uint32 (*mpls_hash_key) (struct mpls_interface *);

  /* Data compare function. */
  int (*mpls_hash_cmp) (struct mpls_interface *, struct mpls_interface *);

  /* bucket alloc. */
  uint32 count;
};

void mpls_hash_init (uint32 (*) (struct mpls_interface *), 
                     int (*) (struct mpls_interface *, struct mpls_interface *));
void *mpls_hash_get (void *, int);
void *mpls_hash_alloc_intern (void *);
void *mpls_hash_lookup (void *);
void *mpls_hash_release (void *);
void *mpls_hash_get_using_key (int);
void mpls_hash_iterate (void (*) (struct mpls_hash_bucket *, void *), void *);

void mpls_hash_free_all (void (*) (void *));

#endif /* _PACOS_MPLS_HASH_H */
