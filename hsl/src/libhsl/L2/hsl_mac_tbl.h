/* Copyright (C) 2004  All Rights Reserved. */

#include <hsl_skip.h>

#ifndef __HSL_MAC_TBL_H
#define __HSL_MAC_TBL_H 

#define ETH_ADDR_LEN        (6)
#define MAX_MAC_LIST_LEN (16384) /* 16K entries */
#define MAX_VID_VALUE    (4096) 


typedef enum {
   SEARCH_BY_MAC,
   SEARCH_BY_VLAN_MAC,
} fdb_search_type;

typedef struct fdb_table
{
  /* Indexes (skiplists) for fdb table */
  hsl_skip_list *vlan_mac_list;
  /* Protection semaphore */
  apn_sem_id fdb_table_mutex;
}fdb_table_t;

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
hsl_init_fdb_table ( void );

/************************************************************
 * deinit_fdb_table  Deinit mac table - free skip lists     *
 *                   destroy semaphore                      *
 * Parameters:                                              *
 *        NONE                                              *  
 * Returns:                                                 *
 *        VOID                                              *
 ************************************************************/
void
hsl_deinit_fdb_table ( void );
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
hsl_add_fdb_entry(fdb_entry_t *entry);
/************************************************************
 * delete_fdb_entry  Remove an entry from fdb table         *
 * Parameters:                                              *
 *   entry pointer to fdb_entry_t data -removal key         *
 * Returns:                                                 *
 *   OK - on successful removal                             *
 *   KEY_NOT_FOUND - in case deleted entry was not found    * 
 ************************************************************/
int
hsl_delete_fdb_entry(fdb_entry_t *entry);

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
              fdb_entry_t *key_entry);

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
                  fdb_entry_t *key_entry);

/*
   Get unicast FDB
*/
int
hsl_fdb_unicast_get_fdb (struct hal_msg_l2_fdb_entry_req *req,
                         struct hal_msg_l2_fdb_entry_resp *resp);

#endif /* __HSL_MAC_TBL_H  */
