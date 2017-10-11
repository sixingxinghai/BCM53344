/* Copyright (C) 2008  All Rights Reserved. */

#include "pal.h"
#include "bheap.h"

/* Create a binary heap and initialize its structure */ 
struct bin_heap * 
bheap_initialize (int bheap_max, 
        int (*compare_function)(void *data1, void *data2),
        void (*update_function)(void *data, int pos),void *X)
{
  struct bin_heap *bh;

  /* Allocate memory for binary heap */
  bh = (struct bin_heap*) XMALLOC (MTYPE_BINARY_HEAP, sizeof (struct bin_heap));

  if (! bh)
    {
      return NULL;
    }

  /* Allocate memory for the array */
  bh->array = (void**) XCALLOC (MTYPE_BINARY_HEAP_ARRAY,
                            (bheap_max * sizeof (int)));

  if (! bh->array)
    { 
      XFREE(MTYPE_BINARY_HEAP,bh);
      return NULL;
    }

  /* Initialize the binary heap */ 
  bh->count = 0;
  bh->compare = compare_function;
  bh->update = update_function;
  bh->capacity = bheap_max;
  bh->opq = X;
      
  return bh;
}

/* Check if the binary heap is empty */
bool_t 
bheap_is_empty (struct bin_heap *bh)
{
  if (bh->count == 0)
    {
      return PAL_TRUE;
    }
  else
    {
     return PAL_FALSE;
    }
} 

/* Check if the binary heap is full */
bool_t
bheap_is_full (struct bin_heap *bh)
{
  if (bh->count == bh->capacity)
    {
      return PAL_TRUE;
    }
  else
    {
      return PAL_FALSE;
    }
} 

/* Compare the children with the parent
   and move towards the root if the heap property is not satisfied */
void  
bheap_percolate_up (struct bin_heap *bh, int index)
{
  void *val = NULL;
  int i;

  val = bh->array[index];
  
  /* Check to make if the parent exist */
  for (i = index; i > 0; i = ((i-1)/2))
    {       
      /* Compare the inserted and its parent */
      if ((*bh->compare)(val, bh->array[((i-1) / 2)]) < 0)
        {
          bh->array[i]= bh->array[((i-1) / 2)];
    
          if (bh->update != NULL)
            {
              (*bh->update)(bh->array[i], i);
            }
        }
      else
        break;
    }

  bh->array[i] = val;

  /* To update position in the binary heap*/
  if (bh->update != NULL)
    {
      (*bh->update)(val, i);
    }

  return;
}

/* Insert a new entry to the heap array */
void  
bheap_insert (struct bin_heap *bh, void *val)
{
  if (bheap_is_full(bh))
    {
      /* Reallocate array mem if heap is full */
      bh->array = (void **) XREALLOC (MTYPE_BINARY_HEAP, bh->array,
                           ((bh->capacity + (bh->capacity / 2)) * sizeof(int)));
      if (! bh->array)
        {  
          return;
        }
    }        
               
  bh->array[bh->count] = val;

  if (bh->update != NULL)
    {
      (*bh->update)(val, bh->count);    
    }
   
  bheap_percolate_up (bh, bh->count);
  bh->count++;

  return;
}

/* Compare the parent with both its children 
   and move towards the leaf nodes to satisfy the heap property */
void 
bheap_percolate_down (struct bin_heap *bh, int index)
{
  void *val = NULL;

  val = bh->array[index];
  
  /* If parent has atleast one child */
  while (((2*index)+1) < bh->count)
    {
      /* If right child is present */ 
      if(((2*index)+ 2) < bh->count)
        {
          /* Choose smallest among both the children */
          if ((*bh->compare)(bh->array[((2 * index) + 1)], 
                             bh->array[((2 * index) + 2)]) < 0)
            {
              /* If left child is small */
              if ((*bh->compare)(val, bh->array[((2 * index) + 1)]) > 0)
                {
                  bh->array[index] = bh->array[((2 * index) + 1)];

                  if (bh->update != NULL)
                    { 
                      (*bh->update)(bh->array[index], index);
                    }

                  index = ((2 * index) + 1);
                }
              else
                break;
            }
          else
            {
              /* If right child is small */ 
              if (( *bh->compare)(val, bh->array[((2 * index) + 2)]) > 0)
                {
                  bh->array[index] = bh->array[((2 * index) + 2)];

                  if (bh->update != NULL)
                    {
                      (*bh->update)(bh->array[index], index);
                    }

                  index = ((2 * index) + 2);     
                }
              else
                break;
            }
        }
      else 
        {
          /* If only left child is present */
          if ((*bh->compare)(val, bh->array[((2 * index) + 1)]) > 0)
            {
              bh->array[index] = bh->array[((2 * index) + 1)];

              if (bh->update != NULL)
                {
                  (*bh->update)(bh->array[index], index);
                }

              index = ((2 * index) + 1);
            }
          else
            break;
        } 
    }
  bh->array[index] = val;

  if (bh->update != NULL)
    {
      (*bh->update)(val, index);    
    }
  
  return;
}

/* Remove the root from the heap */
void * 
bheap_delete_head (struct bin_heap *bh)
{
  void *data = NULL;
   
  if (bheap_is_empty (bh))
    {
      return NULL;
    }
   
  data = bh->array[0];
  bh->array[0] = bh->array[--bh->count];

  bheap_percolate_down (bh, 0);

  return data;
}

/* Find the root from the heap */
void * 
bheap_find_root (struct bin_heap *bh)
{
  if (bheap_is_empty (bh))
    { 
      return NULL;
    }
  else
    {
      return  bh->array[0];
    }
}

/* Make the contents of the heap empty */
void 
bheap_make_empty (struct bin_heap *bh)
{ 
  int i;
  
  if (bheap_is_empty (bh))
    {
      return;
    }

  for (i = 0; i < bh->capacity; i++)
    {
      bh->array[i] = NULL;
    }

  bh->count = 0;
}

/* Build any given random array to a binary heap */ 
struct bin_heap* 
bheap_build (struct bin_heap *bh, void **A, int size)
{
  int i;   

  if (bh == NULL)
    {
      return NULL;
    }
  bheap_make_empty (bh); 
  
  for (i = 0; i < size; i++)
    {
      bh->array[i] = A[i];
      bh->count++;
      bheap_percolate_up (bh, i);
    }
  
  return bh;
}

/* Free the allocated memory to binary heap and its array */
void 
bheap_free (struct bin_heap *bh)
{
  XFREE (MTYPE_BINARY_HEAP_ARRAY, bh->array);
  XFREE (MTYPE_BINARY_HEAP, bh);
}

/* Lookup any particular entry in the binary heap */
int 
bheap_lookup (struct bin_heap *bh, void *data)
{
  int i;
  
  if (bheap_is_empty (bh))
    {
      return -1;
    }
  
  for (i = 0; i < bh->count; i++)
    {
      if (bh->array[i] == data)  
        { 
          return i;
        }
    }  

  return -1;
}


/* Lookup and remove any particular entry in the binary heap */
void*
bheap_lookup_remove (struct bin_heap *bh, void *data)
{
  int i;
  void *temp = NULL;

  if (bheap_is_empty (bh))
    {
      return NULL;
    }

  for (i = 0; i < bh->count; i++)
    {
      if (bh->array[i] == data)
        { 
          temp = bh->array[i];
          bh->array[i] = bh->array[--bh->count];
          bheap_percolate_down (bh, i);

          return temp;
        }
    }

  return NULL;
}


