/* Copyright (C) 2002-2004  All Rights Reserved. */

#ifndef _HSL_SKIP_H
#define _HSL_SKIP_H

#include <hsl_oss.h>

#define HSL_SKIP_RAND_MAX                 2147483647

/*
 * Skip search type 
 */  
typedef enum {
  SEARCH_TYPE_EXACT,
  SEARCH_TYPE_NEXT,
  SEARCH_TYPE_PREV,
} hsl_skip_search_type;

/* 
 * Skip list data comparison function. 
 */
typedef comp_result_t (*skip_comp_func)(void *data1, void *data2);

typedef struct _hsl_skiplist_node {
  void *info;                                 /* User data. */
  struct _hsl_skiplist_node *prev_ptr;        /* Skip list previous node pointer. */
  struct _hsl_skiplist_node *forward_ptr[1];  /* Skip list next node pointer. */
} hsl_skiplist_node;

/* implementation independent declarations */
typedef struct _hsl_hsl_skip_list {
  hsl_skiplist_node *hdr;                  /* List Header. */
  skip_comp_func comp_func;                /* Comparison function for the list. */  
  int list_level;                          /* Current level of list. */
  int max_level;                           /* Maximum level of list. */
  hsl_skiplist_node *update_ptr[1];        /* List update pointer. */
} hsl_skip_list;

#define NIL (hsl_skiplist_node *)sk_list->hdr
/* **********************************************************
 * add_skip_node Add  skip list node routine                *  
 * Parameters:                                              *
 *   sk_list pointer to list to insert  a node              *              
 *   info    pointer to allocated user data                 *  
 * Returns:                                                 *
 *   OK - on successful insertion                           *
 *   DUPLICATE_KEY - in case entry already present          *  
 *   MEMORY ERROR  - in case memory free/allocation fails   *
 ************************************************************/
int 
hsl_add_skip_node(hsl_skip_list *sk_list, void *info); 

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
hsl_remove_skip_node(hsl_skip_list *sk_list, void *info, short data_free);

/************************************************************
 * hsl_search_skip_list Search skip list node routine       *  
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
                 void *key, hsl_skip_search_type lkup_type, void **info);

/************************************************************
 * hsl_init_skip_list skip list initialization routine      *  
 * Parameters:                                              *
 *   new_list pointer to list to initialize                 *              
 *   comp_func comparison function to insert data in sorted *
 *             order                                        * 
 *   max_elements maximum number of elements in the list    *
 * Returns:                                                 *
 *   OK -initialization success.                            *
 *   MEMORY ERROR  - in case memory free/allocation fails   *
 ************************************************************/
int
init_hsl_skip_list(hsl_skip_list **new_list, skip_comp_func comp_func, long max_elements); 

/************************************************************
 * hsl_flush_skip_list skip list flush routine              *  
 * Parameters:                                              *
 *   new_list pointer to list to flush                      * 
 *   data_free - boolean to free/preserve data              *
 * Returns:                                                 *
 *   OK - success                                           *  
 ************************************************************/
int
hsl_flush_skip_list(hsl_skip_list *sk_list, short data_free); 

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
hsl_init_skip_list(hsl_skip_list **new_list, skip_comp_func comp_func, long max_elements);

#endif /* _HSL_SKIP_H */
