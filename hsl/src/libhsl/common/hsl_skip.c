/* Copyright (C) 2002-2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_skip.h"

static void
hsl_search_skip_list_ext(hsl_skip_list *sk_list, hsl_skiplist_node **skip_node, void *info);

 
/* **********************************************************
 * hsl_add_skip_node Add  skip list node routine            *  
 * Parameters:                                              *
 *   sk_list pointer to list to insert  a node              *              
 *   info    pointer to allocated user data                 *  
 * Returns:                                                 *
 *   OK - on successful insertion                           *
 *   DUPLICATE_KEY - in case entry already present          *  
 *   MEMORY ERROR  - in case memory free/allocation fails   *
 ************************************************************/
int 
hsl_add_skip_node(hsl_skip_list *sk_list, void *info) 
{
  int new_level,index;
  hsl_skiplist_node *skip_node;

  /*
   * Input parameters verification. 
   */
  if((NULL == sk_list)||(NULL == info))
  {
    return STATUS_WRONG_PARAMS;
  }

  /*
   *  Allocate a node for data and insert in list  
   */

  /*
   * Search skip list for location where record belongs 
   */
    
  hsl_search_skip_list_ext(sk_list,&skip_node,info);
  
  if (skip_node != NIL)
  {  
    if(HSL_COMP_EQUAL == sk_list->comp_func(skip_node->info, info)) 
    {
      return STATUS_DUPLICATE_KEY;
    }
  }

  /* Decide on node level */
  for (new_level = 0; oss_rand() < OSS_RANDOM_MAX/2 && new_level < sk_list->max_level; 
       new_level++);
  
  if (new_level > sk_list->list_level) 
  {
    for (index = sk_list->list_level + 1; index <= new_level; index++)
    {
      sk_list->update_ptr[index] = NIL;
    }
    sk_list->list_level = new_level;
  }

  /* Allocate a new node */
  skip_node = oss_malloc(sizeof(hsl_skiplist_node) + new_level*sizeof(hsl_skiplist_node *),OSS_MEM_HEAP);
  if(NULL == skip_node)
  {
    return STATUS_MEM_EXHAUSTED;
  }

  /* Set node data */ 
  skip_node->info = info;

  /* Update node forward links */
  for (index = 0; index <= new_level; index++) 
  {
    skip_node->forward_ptr[index] = sk_list->update_ptr[index]->forward_ptr[index];
    sk_list->update_ptr[index]->forward_ptr[index] = skip_node;
  }
  /* Update previous pointer for inserted node */
  skip_node->prev_ptr = skip_node->forward_ptr[0]->prev_ptr;

  /* Set new  node to be a previous for the next node */
  skip_node->forward_ptr[0]->prev_ptr  = skip_node;

  return STATUS_OK;
}

/************************************************************
 * hsl_remove_skip_node  Delete skip list node routine      *  
 * Parameters:                                              *
 *   sk_list pointer to list to remove a node               *              
 *   info    pointer to data  with key to delete            *  
 *   data_free  boolean to free/preserve data               *
 *   info    pointer to data  with key to delete            *  
 * Returns:                                                 *
 *   OK - on successful removal                             *
 *   KEY_NOT_FOUND - in case deleted entry was not found    * 
 *   MEMORY ERROR  - in case memory free/allocation fails   *
 ************************************************************/
int 
hsl_remove_skip_node(hsl_skip_list *sk_list, void *info, short data_free) 
{
  int index;   
  hsl_skiplist_node *skip_node;
      
  /* Find deleted data node  */
  hsl_search_skip_list_ext(sk_list,&skip_node,info);
  
  if (skip_node == NIL)
  {
    /* Deleted entry was not found - search returned head of the list */
    return STATUS_KEY_NOT_FOUND;
  }

  if(HSL_COMP_EQUAL != sk_list->comp_func(skip_node->info, info)) 
  {
    /* Deleted entry was not found */
    return STATUS_KEY_NOT_FOUND;
  }
   
  /* Adjust other nodes forward pointers */
  for (index = 0; index <= sk_list->list_level; index++)
  {
    if ((hsl_skiplist_node *)(sk_list->update_ptr[index]->forward_ptr[index]) != skip_node)
      break;
    sk_list->update_ptr[index]->forward_ptr[index] = skip_node->forward_ptr[index];
  }
  /* Free node data if required */
  if (data_free) 
    if(NULL != skip_node->info) oss_free (skip_node->info,OSS_MEM_HEAP);   

  /* Free node memory */ 
  oss_free(skip_node,OSS_MEM_HEAP);

  /* Adjust skip list header level */
  while (sk_list->list_level > 0)
  {
    if(sk_list->hdr->forward_ptr[sk_list->list_level] == NIL)
    {
      sk_list->list_level--;
    }  
    else /* Level remains the same */
    {
      break;
    }
  }
  return STATUS_OK;
}

/* **************************************************************
 * hsl_search_skip_list_ext Internal routine to search skip list*  
 * Parameters:                                                  *
 *   sk_list pointer to list to search                          *              
 *   info    pointer to data with key to search                 *  
 *   skip_node pointer to fill search result                      *
 * Returns:                                                     *
 *   void                                                       *
 ****************************************************************/
static void
hsl_search_skip_list_ext(hsl_skip_list *sk_list,hsl_skiplist_node **skip_node, void *info)
{
  int index;
#ifdef SKIP_PROFILE
  extern int debug; 
  long profile_counter = 0;  
  double  average_val = 0.0;  
  long total_counter = 0;  
#endif
  hsl_skiplist_node *iterator;
  /* Start from list header node */
  iterator = sk_list->hdr;

  /*
   *  Search skip list from header node 
   *  starting highest link and going down based on
   *  the key comparison
   */

  for (index = sk_list->list_level; index >= 0; index--) 
  {
    while (iterator->forward_ptr[index] != NIL) 
    {
#ifdef SKIP_PROFILE
      profile_counter++;
#endif 
      if (HSL_COMP_LESS_THAN == sk_list->comp_func(iterator->forward_ptr[index]->info, info))
      { /* Move forward_ptr on the same level. */
        iterator = iterator->forward_ptr[index];
      }
      else /* Time to go one level down. */
      {
        break;
      }
    }
    /* Update "update" ptr for removal/insertion */ 
    sk_list->update_ptr[index] = iterator;
  }
  /* Update search result pointer */   
  *skip_node = iterator->forward_ptr[0];
#ifdef SKIP_PROFILE
  total_counter++;
  average_val =  ((total_counter -1) * average_val + profile_counter)/total_counter;
  if(debug) 
     printf("Skip iterations average %f\n",average_val);
#endif
  return;
}

/************************************************************
 * search_hsl_skip_list Search skip list node routine           *  
 * Parameters:                                              *
 *   sk_list pointer to list to search                      *              
 *   key     pointer to data  with key to search            *  
 *   lkup_type  type of search                              *
 *   info    pointer to fill search result                  *
 * Returns:                                                 *
 *   OK -node is found                                      *  
 *   KEY_NOT_FOUND - in case entry was not found            * 
 ************************************************************/
int 
hsl_search_skip_list(hsl_skip_list *sk_list, 
                 void *key, hsl_skip_search_type lkup_type, void **info) 
{
  hsl_skiplist_node *skip_node;
  
  /* Input parameters check */
  if((NULL == sk_list)||(NULL == info))
  {
    return STATUS_WRONG_PARAMS;
  }

  /* Take care of first and last entry without look up */ 
  if(NULL == key) 
  {
    switch (lkup_type) {
    case SEARCH_TYPE_NEXT:
      if(NIL != sk_list->hdr->forward_ptr[0])
      { 
        *info = sk_list->hdr->forward_ptr[0]->info;
        return STATUS_OK;
      }
      break;
    case SEARCH_TYPE_PREV: 
      if(NIL != sk_list->hdr->prev_ptr)
      { 
        *info = sk_list->hdr->prev_ptr->info;
        return STATUS_OK;
      }
      break;
     case SEARCH_TYPE_EXACT:
      break;
    } 
    *info = NULL;
    return STATUS_KEY_NOT_FOUND;
  }
   
  /* Search the list for the key */  
  hsl_search_skip_list_ext(sk_list,&skip_node,key);
  if (skip_node == NIL)
  { 
    *info = NULL;
    return STATUS_KEY_NOT_FOUND;
  }      
 
  switch (lkup_type) {
  case   SEARCH_TYPE_EXACT:
    if(HSL_COMP_EQUAL == sk_list->comp_func(skip_node->info, key)) 
    {
      *info = skip_node->info;
      return STATUS_OK;
    }
    break;
  case SEARCH_TYPE_NEXT:
    if(HSL_COMP_EQUAL == sk_list->comp_func(skip_node->info, key)) 
    {
      if(NIL != skip_node->forward_ptr[0])
      { 
        *info = skip_node->forward_ptr[0]->info;
        return STATUS_OK;
      }
    }
    else   
    {
      *info = skip_node->info;
      return STATUS_OK;
    } 
    break;
  case SEARCH_TYPE_PREV:
    if(NIL != skip_node->prev_ptr)
    { 
      *info = skip_node->prev_ptr->info;
      return STATUS_OK;
    }
  }
  /* No entry found */
  *info = NULL;
  return STATUS_KEY_NOT_FOUND;
}

/************************************************************
 * hsl_init_skip_list skip list initialization routine          *  
 * Parameters:                                              *
 *   new_list pointer to list to initialize                 *              
 *   comp_func comparison function to insert data in sorted *
 *             order                                        * 
 *   max_elements maximum number of elements in the list    *
 * Returns:                                                 *
 *   OK -initialization success
 *   MEMORY ERROR  - in case memory free/allocation fails   *
 ************************************************************/
int
hsl_init_skip_list(hsl_skip_list **new_list, skip_comp_func comp_func, long max_elements) 
{
  int index,max_level;
  hsl_skip_list *sk_list;  
  /*
   * Input parameters check.  
   */
  if((NULL == comp_func) || (max_elements <= 0) || (NULL == new_list))
  {
    return STATUS_WRONG_PARAMS;
  }
  
  /*
   * Skip list initialization part.
   */

  /*
   * Calculate skip list level log2(max_elements) - 1  
   */
  for (max_level = 0; max_elements > 2 ; max_level++)
  {
    max_elements >>= 1; 
  }


  /* Allocate skip list data structure. */
  sk_list = oss_malloc (sizeof(hsl_skip_list) + max_level*sizeof(hsl_skiplist_node *),OSS_MEM_HEAP); 

  if (NULL == sk_list) 
  {
    return STATUS_MEM_EXHAUSTED;
  }

  /* Allocate skip list header node. */
  sk_list->hdr = oss_malloc (sizeof(hsl_skiplist_node) + max_level*sizeof(hsl_skiplist_node *),OSS_MEM_HEAP); 

  if (NULL == sk_list->hdr) 
  {
    oss_free(sk_list,OSS_MEM_HEAP);
    return STATUS_MEM_EXHAUSTED;
  }
  
  /* Init header forward_ptr pointers. */
  for (index = 0; index <= max_level; index++)
  {
    sk_list->hdr->forward_ptr[index] = NIL;
  }
  /* Set skip list properties. */
  sk_list->hdr->prev_ptr = NIL;  
  sk_list->comp_func  = comp_func;
  sk_list->list_level = 0;
  sk_list->max_level  = max_level;
  *new_list = sk_list;

  return STATUS_OK;
}



/************************************************************
 * hsl_flush_skip_list skip list flush routine                  *  
 * Parameters:                                              *
 *   new_list pointer to list to flush                      * 
 *   data_free - boolean to free/preserve data              *
 * Returns:                                                 *
 *   OK - success                                           *  
 ************************************************************/
int
hsl_flush_skip_list(hsl_skip_list *sk_list, short data_free) 
{
  hsl_skiplist_node *skip_node,*tmp_node;
  /*
   * Input parameters check.  
   */
  if((NULL == sk_list) || (NULL == sk_list->hdr))
  {
    return STATUS_WRONG_PARAMS;
  }
 
  skip_node = sk_list->hdr->forward_ptr[0];
   
  while (skip_node != NIL)
  {
    if (data_free)
    {
      if(NULL != skip_node->info)
          oss_free (skip_node->info,OSS_MEM_HEAP);
    }

    tmp_node = skip_node->forward_ptr[0];
    oss_free(skip_node,OSS_MEM_HEAP);
    skip_node = tmp_node;
   }

  /* Free header node */
  oss_free(sk_list->hdr,OSS_MEM_HEAP);

  /* Free skip list */
  oss_free(sk_list,OSS_MEM_HEAP);
  return STATUS_OK;
}
