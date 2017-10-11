/* Copyright (C) 2002-2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl_skip.h"
#include "hsl_ifmgr.h"
#include "hsl_mac_tbl.h"
#include "hsl_logs.h"

/* Mac address table */
static fdb_table_t *p_hsl_fdb_table = NULL;

/************************************************************
 * mac_comp_func - Service routine for ethernet address     *
 *                 comparison                               *
 * Parameters:                                              *
 *       data1 - first entry to compare                     *
 *       data2 - second entry to compare                    *
 * Returns:                                                 *
 * HSL_COMP_EQUAL - if data is identical                        *
 * HSL_COMP_LESS_THAN - if data 1 is less than data 2           *
 * HSL_COMP_GREATER_THAN - if data 1 is greater than data 2     *
 ************************************************************/
comp_result_t 
hsl_mac_comp_func(void *data1, void *data2)
{
  int res;
  fdb_entry_t *addr1,*addr2; 
  addr1 = (fdb_entry_t *)data1;
  addr2 = (fdb_entry_t *)data2;
  res = memcmp(addr1->mac_addr,addr2->mac_addr,ETH_ADDR_LEN);
  if(res < 0 ) 
    return HSL_COMP_LESS_THAN; 
  if (res > 0)
    return HSL_COMP_GREATER_THAN; 
  return HSL_COMP_EQUAL;
}

/************************************************************
 * vlan_mac_comp_func - Service routine to compare two      *
 *              entries based on vlan and ethernet address. *
 * Parameters:                                              *
 *       data1 - first entry to compare                     *
 *       data2 - second entry to compare                    *
 * Returns:                                                 *
 * HSL_COMP_EQUAL - if data is identical                        *
 * HSL_COMP_LESS_THAN - if data 1 is less than data 2           *
 * HSL_COMP_GREATER_THAN - if data 1 is greater than data 2     *
 ************************************************************/
comp_result_t 
hsl_vlan_mac_comp_func(void *data1, void *data2)
{
  fdb_entry_t *addr1,*addr2; 
  addr1 = (fdb_entry_t *)data1;
  addr2 = (fdb_entry_t *)data2;
  
  if ( addr1->vid < addr2->vid )
    return HSL_COMP_LESS_THAN; 
  else if ( addr1->vid > addr2->vid ) 
    return HSL_COMP_GREATER_THAN; 
  return hsl_mac_comp_func(data1,data2);
}

/************************************************************
 * init_fdb_table Init mac table - creates skip lists      *
 *                 for every index                          *  
 * Parameters:                                              *
 *        NONE                                              *  
 * Returns:                                                 *
 *   OK - on successful initiazation                        *
 *   MEMORY ERROR  - in case memory free/allocation fails   *
 ************************************************************/
int 
hsl_init_fdb_table ( void ) 
{
  HSL_FN_ENTER ();

  /* If already initialized, just return. */
  if (p_hsl_fdb_table)
    HSL_FN_EXIT (STATUS_OK);

  /* Allocate memory to hold table descriptor */
  p_hsl_fdb_table = oss_malloc(sizeof(fdb_table_t),OSS_MEM_HEAP);
  if(NULL == p_hsl_fdb_table)
    {  
      HSL_FN_EXIT (STATUS_MEM_EXHAUSTED);
    }
  memset(p_hsl_fdb_table,0,sizeof(fdb_table_t));  

  do {
    /* Init semaphore */
    if (0 != oss_sem_new( "FDB_TABLE_SEM", OSS_SEM_MUTEX, 0, NULL, &p_hsl_fdb_table->fdb_table_mutex))
      break; 
    /* Init skip lists */
    if (0 != hsl_init_skip_list(&p_hsl_fdb_table->vlan_mac_list,hsl_vlan_mac_comp_func, MAX_MAC_LIST_LEN))
      break;
    HSL_FN_EXIT (STATUS_OK);
  } while (0); 

  hsl_deinit_fdb_table();
  HSL_FN_EXIT (STATUS_MEM_EXHAUSTED);
 
}
/************************************************************
 * deinit_fdb_table  Deinit mac table - free skip lists     *
 *                   destroy semaphore                      *
 * Parameters:                                              *
 *        NONE                                              *  
 * Returns:                                                 *
 *        VOID                                              *
 ************************************************************/
void
hsl_deinit_fdb_table ( void ) 
{
  HSL_FN_ENTER ();

  if(NULL == p_hsl_fdb_table)
    HSL_FN_EXIT (-1);

  /* Lock the table. */  
  if( NULL != p_hsl_fdb_table->fdb_table_mutex) 
    {
      oss_sem_lock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex, OSS_WAIT_FOREVER);
    }

  /* Destroy the lists. */ 
  hsl_flush_skip_list(p_hsl_fdb_table->vlan_mac_list, OSS_TRUE);

  /* Unlock the table. */ 
  if( NULL != p_hsl_fdb_table->fdb_table_mutex) 
    {
      oss_sem_unlock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex);
    }

  /* Delete semaphore */
  oss_sem_delete(OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex);

  /* Free table descriptor */ 
  oss_free (p_hsl_fdb_table,OSS_MEM_HEAP);
  p_hsl_fdb_table = NULL;

  HSL_FN_EXIT (STATUS_OK);
}

/************************************************************
 * add_fdb_entry  Add an entry to fdb table                 *
 * Parameters:                                              *
 *   entry pointer to fdb data information                  *
 * Returns:                                                 *
 *   OK - on successful insertion                           *
 *   DUPLICATE_KEY - in case entry already present          *  
 *   MEMORY ERROR  - in case memory free/allocation fails   *
 ************************************************************/
int 
hsl_add_fdb_entry(fdb_entry_t *entry) 
{
  fdb_entry_t *new_entry;

  HSL_FN_ENTER ();
   
  if((NULL == entry) || (NULL == p_hsl_fdb_table))
    HSL_FN_EXIT (STATUS_WRONG_PARAMS);

  new_entry = (fdb_entry_t *)oss_malloc (sizeof (fdb_entry_t),OSS_MEM_HEAP); 
  if(NULL == new_entry)
    HSL_FN_EXIT (STATUS_MEM_EXHAUSTED);

  memcpy(new_entry,entry,sizeof(fdb_entry_t));
  /* Lock the table. */
  if(0 != oss_sem_lock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex, OSS_WAIT_FOREVER))
    {
      oss_free(new_entry,OSS_MEM_HEAP);
      HSL_FN_EXIT (STATUS_SEMAPHORE_LOCK_ERROR);
    }
  /* Add node to the skip lists. */
  do {
    if(STATUS_OK != hsl_add_skip_node(p_hsl_fdb_table->vlan_mac_list, new_entry))
      break;
    /* Unlock the table. */
    oss_sem_unlock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex);
    HSL_FN_EXIT (STATUS_OK);
  } while (0);

  /* Sem UnLock */
  oss_sem_unlock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex);

  /* Free allocated entry */ 
  oss_free (new_entry,OSS_MEM_HEAP);

  HSL_FN_EXIT (STATUS_DUPLICATE_KEY);
}
/************************************************************
 * delete_fdb_entry  Remove an entry from fdb table         *
 * Parameters:                                              *
 *   entry pointer to fdb_entry_t data -removal key         *
 * Returns:                                                 *
 *   OK - on successful removal                             *
 *   KEY_NOT_FOUND - in case deleted entry was not found    * 
 ************************************************************/
int 
hsl_delete_fdb_entry(fdb_entry_t *entry) 
{
  int status,tmp_status; 

  HSL_FN_ENTER ();

  if((NULL == entry) || (NULL == p_hsl_fdb_table))
    HSL_FN_EXIT (STATUS_WRONG_PARAMS);
   
  status = STATUS_OK;
  /* Lock the table. */
  if(0 != oss_sem_lock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex, OSS_WAIT_FOREVER))
    {
      HSL_FN_EXIT (STATUS_SEMAPHORE_LOCK_ERROR);
    }
  tmp_status = hsl_remove_skip_node(p_hsl_fdb_table->vlan_mac_list, entry,OSS_TRUE);
  if(STATUS_OK != tmp_status)
    status = tmp_status;
  /* Unlock the table.*/
  oss_sem_unlock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex);
  HSL_FN_EXIT (status);
}

/************************************************************
 * get_fdb_entry  Find an entry from fdb table              *
 * Parameters:                                              *
 *   entry pointer to fill                                  * 
 *   lkup_type search type (mac based,vlan_mac, or port_mac *
 *                          or type_vlan_mac based search)  *
 *   key_entry look up key                                  *
 * Returns:                                                 *
 *   OK - on successful find                                *
 *   KEY_NOT_FOUND - in case entry was not found            * 
 * NOTE:                                                    * 
 ************************************************************/
int 
hsl_get_fdb_entry(fdb_entry_t *entry,
                  fdb_search_type lkup_type,
                  fdb_entry_t *key_entry)
{
  fdb_entry_t *found_entry;
  int status = 0;
  int index;

  HSL_FN_ENTER ();
  
  /* Input parameters verification */
  if((NULL == entry) || (NULL == key_entry))
    {
      HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }

  /* Lock the table. */
  if(0 != oss_sem_lock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex, OSS_WAIT_FOREVER))
    {
      HSL_FN_EXIT (STATUS_SEMAPHORE_LOCK_ERROR);
    }
  /* Search for entry */
  switch (lkup_type)
    {
    case  SEARCH_BY_MAC:
      for (index = 0; index < MAX_VID_VALUE; index ++) 
        {
          key_entry->vid = index;
          status = hsl_search_skip_list(p_hsl_fdb_table->vlan_mac_list, key_entry,SEARCH_TYPE_EXACT,(void **)&found_entry);
          if (STATUS_OK == status)
            break; 
        }
      break;
    case  SEARCH_BY_VLAN_MAC:
      status = hsl_search_skip_list(p_hsl_fdb_table->vlan_mac_list, key_entry,SEARCH_TYPE_EXACT,(void **)&found_entry);
      break; 
    }

  if(STATUS_OK == status)
    *entry = *found_entry;
   
  /* Unlock the table.*/
  oss_sem_unlock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex);
  HSL_FN_EXIT (status);
}

/************************************************************
 * getnext_fdb_entry  Find next entry from fdb table        *
 * Parameters:                                              *
 *   entry pointer to fill                                  * 
 *   lkup_type search type (mac based,vlan_mac, or port_mac *
 *                          or type_vlan_mac based search)  *
 *   key_entry - lkup key                                   *  
 * Returns:                                                 *
 *   OK - on successful find                                *
 *   KEY_NOT_FOUND - in case entry was not found            * 
 * NOTE:                                                    * 
 ************************************************************/
int 
hsl_getnext_fdb_entry(fdb_entry_t *entry,
                      fdb_search_type lkup_type,
                      fdb_entry_t *key_entry) 
{
  fdb_entry_t *found_entry;
  int status;

  HSL_FN_ENTER ();
  
  /* Input parameters verification */
  if((NULL == entry))
    {
      HSL_FN_EXIT (STATUS_WRONG_PARAMS);
    }

  /* Lock the table. */
  if(0 != oss_sem_lock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex, OSS_WAIT_FOREVER))
    {
      HSL_FN_EXIT (STATUS_SEMAPHORE_LOCK_ERROR);
    }

  switch (lkup_type)
    {
    case  SEARCH_BY_VLAN_MAC:
      status = hsl_search_skip_list(p_hsl_fdb_table->vlan_mac_list, key_entry,SEARCH_TYPE_NEXT,(void **)&found_entry);
      break;
    default:
      status = STATUS_KEY_NOT_FOUND;
    }
  if(STATUS_OK == status)
    *entry = *found_entry;

  /* Unlock the table.*/
  oss_sem_unlock( OSS_SEM_MUTEX, p_hsl_fdb_table->fdb_table_mutex);

  HSL_FN_EXIT (status);
}


/*
   Get unicast FDB
*/
int
hsl_fdb_unicast_get_fdb (struct hal_msg_l2_fdb_entry_req *req,
                         struct hal_msg_l2_fdb_entry_resp *resp)
{
  struct hal_fdb_entry *entry;
  fdb_entry_t result;
  struct hsl_if *ifp;
  u_int32_t count;
  int ret;

  HSL_FN_ENTER ();

  entry = &req->start_fdb_entry;
  memcpy (&result.mac_addr[0], &entry->mac_addr[0], ETH_ADDR_LEN);
  result.vid = entry->vid;
  count = 0;
  ret = 0;

  entry = &resp->fdb_entry[0];

  resp->count = 0;

  /* Get entry */

  if (req->req_type == HAL_MSG_L2_FDB_REQ_GET)
    {

      if (STATUS_OK == hsl_get_fdb_entry (&result, SEARCH_BY_VLAN_MAC,
                                          &result))
        {
          /* Get interface structure. */
          ifp = hsl_ifmgr_lookup_by_index (result.port_no);

          if (!ifp)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Unknown ifindex %d "
                                    "in l2 fdb database\n",result.port_no);
              HSL_FN_EXIT (ret);
            }

          /* We don't need to show mac addresses learned on L3 interfaces. */ 
          if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
            {
              HSL_IFMGR_IF_REF_DEC (ifp);
              HSL_FN_EXIT (ret);
            }

          HSL_IFMGR_IF_REF_DEC (ifp);

          /* Interface index. */
          entry->port = result.port_no;

          /* Mac address. */
          memcpy (entry->mac_addr, result.mac_addr, ETH_ADDR_LEN);

          /* vid */
          entry->vid = result.vid;

          /* is_fwd */
          entry->is_forward = result.is_fwd;

          /* ageing_timer_value */
          entry->ageing_timer_value = result.ageing_timer_value;

          entry += 1;

          count += 1;
        }

      resp->count = count;

      HSL_FN_EXIT (ret);
    }

  while (count < req->count &&
         STATUS_OK == hsl_getnext_fdb_entry (&result, SEARCH_BY_VLAN_MAC,
                                             &result))
    {
      if (result.is_static == 1)
         continue; 

      /* Get interface structure. */
      ifp = hsl_ifmgr_lookup_by_index (result.port_no);

      if (!ifp)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Unknown ifindex %d "
                                "in l2 fdb database\n",result.port_no);
          continue;
        }

      /* We don't need to show mac addresses learned on L3 interfaces. */ 
      if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          continue;
        }

      HSL_IFMGR_IF_REF_DEC (ifp);

      /* Interface index. */
      entry->port = result.port_no;

      /* Mac address. */
      memcpy (entry->mac_addr, result.mac_addr, ETH_ADDR_LEN);

      /* vid */
      entry->vid = result.vid;

      /* is_fwd */
      entry->is_forward = result.is_fwd;

      /* ageing_timer_value */
      entry->ageing_timer_value = result.ageing_timer_value;

      entry += 1;
      count += 1;
    }

  resp->count = count;

  HSL_FN_EXIT (ret);
}
