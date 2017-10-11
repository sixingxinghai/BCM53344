/* 
   Copyright (C) 2003  All Rights Reserved. 
   
   LAYER 2 BRIDGE
   
   This module maintains the "RANGE list" used to store vlan id's 
   associated with instances. 
  
*/

#include "pal.h"
#include "lib.h"
#include "l2lib.h"
#include "mstp_config.h"
#include "mstp_types.h"
#include "mstp_bridge.h"
#include "mstpd.h"

/* Combines the two adjacent element to get a new range 
   e.g 8-9, and 10-13 to get 8-13
*/
static inline void
mstp_join_rlist (struct rlist_info *prev_info , struct rlist_info *curr_info)
{
  if ((!prev_info) || (!curr_info))
    return;
  
  if (prev_info->hi == (curr_info->lo - 1))
    {
      prev_info->hi = curr_info->hi;
      prev_info->next = curr_info->next;
      XFREE(MTYPE_MSTP_MSTI_INFO, curr_info);
    }
}

/* Adds the new range element in the list. 
   internal function of the rlist api 
*/
static void
mstp_link_rlist (struct rlist_info **range_list, 
                 struct rlist_info *new_ele,
                 int *inst_info_count)
{

  struct rlist_info *curr;
  struct rlist_info *prev = NULL;

  pal_assert (range_list);

  curr = *range_list;
  
  if (!curr)
    {
      *range_list = new_ele;

      *inst_info_count = *inst_info_count + 1;

      return;
    }

  while ((curr) && (new_ele->lo > curr->lo))
    { 
      prev = curr;  
      curr = curr->next;
    }
 
  /* Check if the entry is a right/left overlap.
   * if so modify the hi/lo values accordingly and return. */
  if ((prev) && 
      (new_ele->lo <= prev->hi ))
    {
      if (new_ele->hi > prev->hi)   
        prev->hi = new_ele->hi;  
    
      XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
      return;
    }
    
  
  if ((curr) && (new_ele->hi >= curr->lo))
    { 
      curr->lo = new_ele->lo;
    
      XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
      return;
    }

  /* There is no overlap but the entries are contiguous */
  if ((prev) && 
      (new_ele->lo == (prev->hi +1)))
    {
      /* merge with left side */
      prev->hi = new_ele->hi ;
      mstp_join_rlist (prev,curr);
    
      XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
    
    }
  
  else if ((curr) && 
           (new_ele->hi == (curr->lo - 1)))
    {
      /* Merge with right hand side */
      curr->lo = new_ele->lo; 
      mstp_join_rlist (prev,curr);

      XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
   
    }
  else
    {
      /*else insert element into the list */

      if (prev) 
        prev->next = new_ele;
      else
        *range_list = new_ele;
      new_ele->next = curr;

      *inst_info_count = *inst_info_count + 1; 
    }

}

/* Remove the range list element from the list 
   internal function which does all the work for the rlist api 
*/
static int
mstp_unlink_rlist (struct rlist_info **range_list, 
                   struct rlist_info *new_ele,
                   u_int16_t *inst_info_count)
{
  struct rlist_info *curr;
  struct rlist_info *prev = NULL;
  int temp;
  int edge = PAL_FALSE;
  
  curr = *range_list;
   
  pal_assert (range_list);
 
  /* Search the element */ 
  while ((curr) && (new_ele->lo > curr->hi))
    { 
      prev = curr;  
      curr = curr->next;
    }

  if (curr)
    {
      if (curr->lo > new_ele->hi)
        {
          /* The element has not been added to the list */
          XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
          return RESULT_ERROR;
        }    
      temp = curr->hi;
      if (curr->lo == new_ele->lo)
        { 
          /* The element is the first in the range */
          if (curr->hi == new_ele->hi)
            {
              /* We want to delete the entire range */
              if (prev)
                {
                  prev->next = curr->next; 
                }
              else 
                {
                  /* The range is the first in the list */
                  *range_list = curr->next;
                }
              *inst_info_count = curr->vlan_range_indx;

              XFREE (MTYPE_MSTP_MSTI_INFO, curr);
              XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
              return RESULT_OK;
            }
          curr->lo = new_ele->hi+1;
          edge = PAL_TRUE;
        }
      
      else if (curr->hi == new_ele->hi)
        {
          curr->hi = new_ele->lo -1 ;
          edge = PAL_TRUE;

        }

      if (!edge )
        {
          curr->hi = new_ele->lo - 1;
    
          new_ele->lo = new_ele->hi + 1;

          new_ele->hi = temp;
          new_ele->next = curr->next ;
          curr->next = new_ele;
        }
      else
        {
          XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
        }
      
      return RESULT_OK;
    
    }
  else
    {
      /* Free the memory allocated br the api function */ 
      XFREE (MTYPE_MSTP_MSTI_INFO, new_ele);
      return RESULT_ERROR;
    }

}

/* This is the API provided by the rlist .
   This function builds the new element of the rlist_info type 
   and calls the internal link_rlist function
   Arguments : The address of the list and the data element to be added
*/
int
mstp_rlist_add (struct rlist_info **range_list , mstp_vid_t element,
                u_int32_t rlist_index)
{
  struct rlist_info *new_ele;
  int rlist_count = -1;
 
  new_ele  = XCALLOC(MTYPE_MSTP_MSTI_INFO,sizeof(struct rlist_info));
  if (!new_ele)
    {
      zlog_err (mstpm, "Cannot allocate memory for MSTI info \n");
      return 0;
    }
  new_ele->lo = element;
  new_ele->hi = element;

  mstp_link_rlist (range_list, new_ele, &rlist_count);

  if (!rlist_count)
    {
      new_ele->vlan_range_indx = rlist_index;

      return PAL_TRUE;
    }
  return PAL_FALSE;
}

/* This is the API provided by the rlist .
   This function builds the new element of the rlist_info type 
   and calls the internal remove link_rlist function
   Arguments : The address of the list and the data element to be added
*/

int
mstp_rlist_delete (struct rlist_info **range_list , mstp_vid_t element,
                   u_int16_t *range_index)
{
  struct rlist_info *new_ele;
  u_int16_t rlist_idx = 0 ;
  s_int32_t ret = 0;
 
  /* Might seem strange to allocate memory on delete 
     but the internal function will determine which 
     memory is to be freed
  */
  new_ele  = XCALLOC(MTYPE_MSTP_MSTI_INFO, sizeof(struct rlist_info));
  if (!new_ele)
    {
      zlog_err (mstpm, "Cannot allocate memory for MSTI info \n");
      return RESULT_ERROR;
    }

  new_ele->lo = element;
  new_ele->hi = element;

  ret = mstp_unlink_rlist (range_list, new_ele, &rlist_idx);

  *range_index = rlist_idx;
    
  return ret; 
}

/* Free the list and release all allocated resources */
void
mstp_rlist_free (struct rlist_info **range_list)
{
  struct rlist_info *list_ele = range_list ? *range_list : NULL ;
  struct rlist_info *temp;
  
  while (list_ele)
    {
      temp =  list_ele->next;
      list_ele->next = NULL;  
      XFREE(MTYPE_MSTP_MSTI_INFO, list_ele);
      list_ele = temp;
    }


} 

/* Insert the source range list into dest list and free up the src list  */
void
mstp_rlist_move (struct rlist_info **dest_list, struct rlist_info **src_list)
{
  struct rlist_info *list_ele = src_list ? *src_list : NULL ;
  struct rlist_info *temp;
  int count = -1;
  
  while (list_ele)
    {
      temp =  list_ele->next  ;
      list_ele->next = NULL;  
      mstp_link_rlist (dest_list, list_ele, &count);
      list_ele = temp;
    }
}
