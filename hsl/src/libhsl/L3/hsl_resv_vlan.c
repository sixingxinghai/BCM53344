/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_avl.h"
#include "hsl_logger.h"
#include "hsl_logs.h"
#include "hsl_error.h"
#include "hsl_table.h"
#include "hsl_ether.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_resv_vlan.h"

/* TODO Change the DB init to take the start of reserved vid and number
 * of reserved vids
 */

#ifdef HAVE_L3

struct hsl_if_resv_vlan_db *p_hsl_if_resv_vlan_db = NULL;

/****************************************************************************
 * Function: _hsl_reserved_vlan_db_init -Initialize reserved vlan db.       *
 * Parameters:                                                              *
 * IN  first_vid - First vlan id.                                           *
 * IN  num - Number of reserved vlans                                       *
 *           Usually total number of pure L3 ports.                         *
 * Return value:                                                            *
 *    OK - initialization success.                                          *  
 *    Error - otherwise                                                     *
 ****************************************************************************/

static int 
_hsl_reserved_vlan_db_init (hsl_vid_t first_vid, u_int16_t num,
                            struct hsl_if_resv_vlan_db **p_db,
                            struct hsl_if_resv_vlan_cb *cb)
{
   int ret;
   int size;
   int index;
   int status;
   struct hsl_if_resv_vlan *entry;
   struct hsl_if_resv_vlan_db *p_db_ptr;
   
   HSL_FN_ENTER ();

   status = STATUS_OK;

   /* Sanity checks. */
   if (4096 < (first_vid  + num))
     HSL_FN_EXIT (STATUS_ERROR);
   
   /* Alloc db control structure. */
   size = sizeof (struct hsl_if_resv_vlan_db);

   p_db_ptr = oss_malloc (size, OSS_MEM_HEAP);

   if (!p_db_ptr)
     {
       HSL_ERR_MALLOC (size);
       HSL_FN_EXIT (STATUS_ERROR);
     }

   p_db_ptr->resv_vlan_cb = cb;

   /* Create protection semaphore. */
   ret = oss_sem_new ("RESV_VLAN_PORT_MUTEX", OSS_SEM_MUTEX, 0, NULL, 
                      &p_db_ptr->res_vlan_mutex);
   if (ret < 0)
     {
       hsl_reserved_vlan_db_deinit (p_db_ptr);
       HSL_FN_EXIT (STATUS_ERROR);
     }

   /* Alloc db entries for all reserved vlans. */
   size  = num * sizeof(struct hsl_if_resv_vlan);

   entry = oss_malloc(size,OSS_MEM_HEAP); 
   if (!entry)
     {
       HSL_ERR_MALLOC (size);  
       hsl_reserved_vlan_db_deinit (p_db_ptr);  
       HSL_FN_EXIT (STATUS_ERROR); 
     }

   /* Save db pointer in control structure. */ 
   p_db_ptr->res_vlan_db = entry;

   /* Mark all  db entries as invalid ones. */
   for (index = 0; index < num; index ++)
     {

       /* Set vid. */
       entry[index].vid  = (first_vid + index);
       entry[index].p_db = p_db_ptr;

       /* Init vlan in HW */
       if (cb->resv_vlan_init)
         status = cb->resv_vlan_init (entry[index].vid);

       if (STATUS_OK != status)
         {
            HSL_ERR_VID_CREATION_FAILED (status, entry[index].vid, 0);
            hsl_reserved_vlan_db_deinit (p_db_ptr);
            HSL_FN_EXIT (STATUS_ERROR);
         }

       /* Update prev/next ptrs */
       if(p_db_ptr->free_list)
         p_db_ptr->free_list->prev = &entry[index];

       entry[index].next = p_db_ptr->free_list;

       /* Update free list ptr */
       p_db_ptr->free_list = &entry[index];
    }

   *p_db = p_db_ptr;
   HSL_FN_EXIT (STATUS_OK);
}

/****************************************************************************
 * Function: _hsl_reserved_vlan_db_deinit-Deinitialize reserved vlan db.*
 * Parameters:                                                              *
 * IN  p_db database to deinit                                              *
 * Return value:                                                            *
 *    OK - initialization success.                                          *
 *    Error - otherwise                                                     *
 ****************************************************************************/

static int 
_hsl_reserved_vlan_db_deinit (struct hsl_if_resv_vlan_db *p_db)
{
  HSL_FN_ENTER (); 

  if (!p_db)
    HSL_FN_EXIT (STATUS_OK); 
  
  /* Delete semaphore. */
  if (p_db->res_vlan_mutex)
    oss_sem_delete (OSS_SEM_MUTEX, p_db->res_vlan_mutex);

  /* Free interface <-> vlan database. */  
  if (p_db->res_vlan_db)
    oss_free (p_db->res_vlan_db, OSS_MEM_HEAP);

  /* Free db header. */   
  oss_free (p_db,OSS_MEM_HEAP); 

  HSL_FN_EXIT (STATUS_OK);
}

/****************************************************************************
 * Function: hsl_reserved_vlan_db_init -Initialize reserved vlan db.        *
 * Parameters:                                                              *
 * IN  first_vid - First vlan id.                                           *
 * IN  num - Number of reserved vlans                                       *
 *           Usually total number of pure L3 ports.                         *
 * IN  cb  - Reserved vlans callback                                        *
 * Return value:                                                            *
 *    OK - initialization success.                                          *  
 *    Error - otherwise                                                     *
 ****************************************************************************/

int 
hsl_reserved_vlan_db_init (hsl_vid_t first_vid, u_int16_t num,
                           struct hsl_if_resv_vlan_cb *cb)
{
  HSL_FN_ENTER ();

  if (!cb)
    {
      HSL_ERR_WRONG_PARAMS;
      HSL_FN_EXIT (STATUS_ERROR); 
    }

  HSL_FN_EXIT (_hsl_reserved_vlan_db_init (first_vid, num,
                                           &p_hsl_if_resv_vlan_db, cb));
}

/****************************************************************************
 * Function: hsl_reserved_vlan_db_deinit-Deinitialize reserved vlan db. *
 * Parameters:                                                              *
 *   void                                                                   *
 * Return value:                                                            *
 *    OK - initialization success.                                          *
 *    Error - otherwise                                                     *
 ****************************************************************************/

int 
hsl_reserved_vlan_db_deinit ()
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (_hsl_reserved_vlan_db_deinit (p_hsl_if_resv_vlan_db));
}

/****************************************************************************
 * Function: _hsl_reserved_vlan_alloc -Allocated reserved vlan entry.   *
 * Parameters:                                                              *
 * IN  ifp - layer 3 interface structure                                    *
 * IN  p_db    - pointer to database.                                       *
 * IN  p_entry - pointer to allocated entry.                                *
 * Return value:                                                            *
 *    OK - success.                                                         *
 *    Error - otherwise                                                     *
 ****************************************************************************/

static int
_hsl_reserved_vlan_alloc (struct hsl_if *ifp,
                          struct hsl_if_resv_vlan_db *p_db,
                          struct hsl_if_resv_vlan **p_entry)
{
  struct hsl_if_resv_vlan *entry;

  HSL_FN_ENTER (); 

  if (!p_db)
    HSL_FN_EXIT (STATUS_ERROR); 

  /* Lock the database. */
  HSL_RESV_VLAN_IF_LOCK;

  /* Return error if free list is empty. */  
  if (!p_db->free_list)
    {
       HSL_RESV_VLAN_IF_UNLOCK;
       *p_entry = NULL;
       HSL_FN_EXIT (STATUS_ERROR);
    }

  /* Alloc first entry from the free list. */  
  entry = p_db->free_list;
  entry->next->prev = NULL;
  p_db->free_list = entry->next; 

  /* Save ifp in the entry */ 
  entry->ifp = ifp;

  /* Add entry to allocated list */
  if (p_db->alloc_list)
     p_db->alloc_list->prev = entry;

  entry->next = p_db->alloc_list;

  /* Update free list ptr */
  p_db->alloc_list = entry;

  *p_entry = entry;

  /* Unlock the database. */
  HSL_RESV_VLAN_IF_UNLOCK;

  HSL_FN_EXIT (STATUS_OK); 
}

/****************************************************************************
 * Function: _hsl_reserved_vlan_free -Free reserved vlan entry.         *
 * Parameters:                                                              *
 * IN  p_db    - pointer to database.                                       *
 * IN  p_entry - pointer to allocated entry.                                *
 * Return value:                                                            *
 *    OK - success.                                                         *
 *    Error - otherwise                                                     *
 ****************************************************************************/
static int
_hsl_reserved_vlan_free (struct hsl_if_resv_vlan_db *p_db,
                         struct hsl_if_resv_vlan *p_entry)
{
  HSL_FN_ENTER (); 

  if ((!p_db) || (!p_entry))
    {
      HSL_ERR_WRONG_PARAMS;
      HSL_FN_EXIT (STATUS_ERROR); 
    }

  if (p_entry->p_db != p_db)
    {
      HSL_ERR_WRONG_PARAMS;
      HSL_FN_EXIT (STATUS_ERROR); 
    }

  /* Lock the database. */
  HSL_RESV_VLAN_IF_LOCK;

  /* Update alloc list */
  if (p_db->alloc_list == p_entry)
    p_db->alloc_list = p_entry->next; 
  
  if (p_entry->prev)
    p_entry->prev->next = p_entry->next;

  if (p_entry->next)
    p_entry->next->prev = p_entry->prev;

  /* Reset entry data */
  p_entry->ifp = NULL;

  /* Add entry to the head of free list. */
  p_db->free_list->prev = p_entry;
  p_entry->next = p_db->free_list;
  p_entry->prev = NULL;
  p_db->free_list = p_entry;

  /* Unlock the database. */
  HSL_RESV_VLAN_IF_UNLOCK;

  HSL_FN_EXIT (STATUS_OK); 
}

/****************************************************************************
 * Function: hsl_reserved_vlan_aloc -Allocate vid for L3 port. Routine  *
 * will allocated reserved vid and set l2 port pvid to allocated value      *
 * Parameters:                                                              *
 * IN  ifp - layer 3 interface structure                                    *
 * Return value:                                                            *
 *    OK - initialization success.                                          *  
 *    Error - otherwise                                                     *
 ****************************************************************************/

int 
hsl_reserved_vlan_alloc (struct hsl_if *ifp)
{
  struct hsl_if_resv_vlan *entry;
  int ret;

  HSL_FN_ENTER (); 

  ret = _hsl_reserved_vlan_alloc (ifp, p_hsl_if_resv_vlan_db, &entry);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,"%s:%d Failed to allocaed vid "
               "for L3 interface (%d)\n",__FILE__,__LINE__, ifp->ifindex);
      HSL_FN_EXIT (STATUS_ERROR);
    }

  /* Update l3 info with reserved vlan id */
  ifp->u.ip.resv_vlan = entry; 

  ret = HSL_IF_RESV_VLAN_CB(add_vid_to_l3_port) (ifp, entry->vid); 

  if (ret < 0)
    {
      _hsl_reserved_vlan_free (p_hsl_if_resv_vlan_db, entry);
      ifp->u.ip.resv_vlan = NULL;
      HSL_FN_EXIT (STATUS_ERROR);
    }

  HSL_FN_EXIT (STATUS_OK);
}

/****************************************************************************
 * Function: hsl_reserved_vlan_free - Free vid for L3 port. Routine         *
 * will free reserved vid and reset l2 port pvid to default vlan value      *
 * Parameters:                                                              *
 * IN  ifp - layer 3 interface structure                                    *
 * Return value:                                                            *
 *    OK - initialization success.                                          *
 *    Error - otherwise                                                     *
 ****************************************************************************/

int 
hsl_reserved_vlan_free (struct hsl_if *ifp)
{
  hsl_vid_t vid;
  int ret; 

  HSL_FN_ENTER ();

  vid   = ifp->u.ip.resv_vlan->vid; 

  /* Free reserved vlan. */
  ret = _hsl_reserved_vlan_free (p_hsl_if_resv_vlan_db,
                                 ifp->u.ip.resv_vlan);

  if (ret < 0)
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "%s:%d Failed to free vid "
             "ifindex (%d)\n", __FILE__, __LINE__, ifp->ifindex);

  /* Zero vlan info. */ 
  ifp->u.ip.resv_vlan = NULL; 

  /* Remove port from reserved vlan id */
  HSL_IF_RESV_VLAN_CB(del_vid_from_l3_port) (ifp, vid);

  HSL_FN_EXIT(ret);
}

/****************************************************************************
 * Function: hsl_mvl_dump_reserved_vlan_db -Dump reserved vlan db.          *
 * Parameters:  void                                                        *
 * Return value:void                                                        *
 ****************************************************************************/

void 
hsl_dump_reserved_vlan_db (void)
{
   struct hsl_if_resv_vlan *entry;
   struct hsl_if_resv_vlan_db *p_db= p_hsl_if_resv_vlan_db;
   int count;
   
   HSL_FN_ENTER ();
  
   if (!p_db)
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Reserved db is not "
                                                 "initialized.\n");

   /* Lock the database. */
   HSL_RESV_VLAN_IF_LOCK;

   /* Dump free list. */ 
   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Reserved db free list.\n");

   for (count = 0,entry = p_db->free_list; entry; entry = entry->next)
     {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "%4d)Entry (%x) Vlan (%d)\n",
                count, entry, entry->vid);
       count++;
     }

   /* Dump Allocated list. */ 
   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Reserved db alloc list.\n");

   for (count = 0,entry = p_db->alloc_list; entry; entry = entry->next)
     {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "%4d)Entry (%x) Vlan (%d) "
                "ifindex (%d)\n", count, entry, entry->vid,
                entry->ifp->ifindex);
       count++;
     }

   HSL_RESV_VLAN_IF_UNLOCK;

   HSL_FN_EXIT (STATUS_OK);

}

#endif /* HAVE_L3 */
