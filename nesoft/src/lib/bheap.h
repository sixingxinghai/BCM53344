/* Copyright (C) 2008  All Rights Reserved. */

#ifndef _PACOS_BHEAP_H
#define _PACOS_BHEAP_H

struct bin_heap
{
  unsigned int count;           /* number of entries in the array */
  void **array;                /* array to implement binary heap */
  void *opq;                  /* generic datatype for future use */
  unsigned int capacity;     /* Max number of entries in the array */
  int (*compare)(void *data1, void *data2);  /* compare the values of two data */
  void (*update)(void *data, int position);  /* Update the position 
                                               of array element */
};
/* Create a binary heap and initialize its structure */
struct bin_heap *bheap_initialize (int max_elements, int(*compare_function)(void *data1, void *data2),
            void(*update_function)(void *data, int pos), void *X);

/* Check if the binary heap is empty */
bool_t 
bheap_is_empty (struct bin_heap *);

/* Check if the binary heap is full */
bool_t 
bheap_is_full (struct bin_heap *);

/* Compare the children with the parent
   and move towards the root if the heap property is not satisfied */
void  
bheap_percolate_up (struct bin_heap *, int);

/* Insert a new entry to the heap array */
void  
bheap_insert (struct bin_heap *, void *);

/* Compare the parent with both its children
   and move towards the leaf nodes to satisfy the heap property */
void 
bheap_percolate_down (struct bin_heap *,int);

/* Remove the root from the heap */
void* 
bheap_delete_head (struct bin_heap *);

/* Find the root from the heap */
void* 
bheap_find_root (struct bin_heap *);

/* Make the contents of the heap empty */
void 
bheap_make_empty (struct bin_heap *);

/* Build any given random array to a binary heap */
struct bin_heap* 
bheap_build (struct bin_heap *, void **, int );

/* Free the allocated memory to binary heap and its array */
void 
bheap_free (struct bin_heap *);

/* Lookup any particular entry in the binary heap */
int 
bheap_lookup (struct bin_heap *, void *);

/* Lookup and remove any particular entry in the binary heap */
void* 
bheap_lookup_remove (struct bin_heap *, void *);

#endif /*_PACOS_BHEAP_H */
