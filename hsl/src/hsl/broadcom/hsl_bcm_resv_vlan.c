/* Copyright (C) 2005  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

/* 
   Broadcom includes. 
*/
#include "bcm_incl.h"

/* 
   HAL includes.
*/
#include "hal_types.h"
#ifdef HAVE_L2
#include "hal_l2.h"
#endif /* HAVE_L2 */

#include "hal_msg.h"

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_avl.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_ifmap.h"
#include "hsl_bcm_resv_vlan.h"
#include "hsl_table.h"
#include "hsl_ether.h"


/* Pointer to reserved vlan database. */
static struct hsl_bcm_resv_vlan_db *p_resv_vlan_db;

/* Create reserved VLAN in hardware. */
static int 
_hsl_bcm_create_vlan (hsl_vid_t vid)
{
  int ret;

  if (HSL_DEFAULT_VID == vid)
    return 0;

  /* Create VLAN. */ 
  ret = bcmx_vlan_create (vid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't add VLAN %d\n", vid);
      return -1;
    }

  /* Add CPU port as a member of the VLAN. */
  hsl_bcm_add_port_to_vlan (vid, BCMX_LPORT_LOCAL_CPU, 1);

  return 0;
}

/* Create reserved VLAN in hardware. */
static int 
_hsl_bcm_delete_vlan (hsl_vid_t vid)
{
  int ret;

  if (HSL_DEFAULT_VID == vid)
    return 0;

  /* Delete VLAN. */
  ret = bcmx_vlan_destroy (vid);
  if (ret < 0)
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't delete VLAN %d\n", vid);

  return 0;
}

/* Initialize reserved VLAN database. */
/* Initialize reserved VLAN database. */
int hsl_bcm_resv_vlan_db_init(
                              hsl_vid_t start_vid,  /* start of reserved vlans. */
                              int range             /* Range. */
                              )
{
   int ret;
   int index;
   int size;
   struct hsl_bcm_resv_vlan *entry;
   
   HSL_FN_ENTER();

   /* Sanity checks. */
   if(4096 < (start_vid + range))
     HSL_FN_EXIT(STATUS_ERROR);
   
   /* Alloc db control structure. */
   p_resv_vlan_db = oss_malloc (sizeof(struct hsl_bcm_resv_vlan_db), OSS_MEM_HEAP); 
   if(! p_resv_vlan_db)
     {
       HSL_FN_EXIT(STATUS_ERROR);
     }

   /* Store start_vid and range. */
   p_resv_vlan_db->start_vid = start_vid;
   p_resv_vlan_db->range = range;

   /* Create protection semaphore. */
   ret = oss_sem_new ("RES_VLAN_PORT_MUTEX", OSS_SEM_MUTEX, 0, NULL, 
                      &p_resv_vlan_db->mutex);
   if(ret < 0)
     {
       hsl_bcm_resv_vlan_db_deinit ();  
       HSL_FN_EXIT(STATUS_ERROR); 
     }

   /* Alloc db entries for all reserved vlans. */
   size  = range * sizeof(struct hsl_bcm_resv_vlan);
   entry = oss_malloc (size,OSS_MEM_HEAP); 
   if(! entry) 
     {
       HSL_ERR_MALLOC (size);  
       hsl_bcm_resv_vlan_db_deinit ();  
       HSL_FN_EXIT (STATUS_ERROR); 
     }

   /* Save db pointer in control structure. */ 
   p_resv_vlan_db->res_vlan_db = entry;

   /* Mark all  db entries as invalid ones. */
   for(index = 0; index < range; index ++)
   {
     /* Set vid. */
     entry[index].vid  = (start_vid + index);

     /* Update prev/next ptrs */
     if(p_resv_vlan_db->free_list)
       p_resv_vlan_db->free_list->prev = &entry[index];
     entry[index].next = p_resv_vlan_db->free_list;

     /* Update free list ptr */
     p_resv_vlan_db->free_list = &entry[index];
   }

   HSL_FN_EXIT(STATUS_OK);
}

/* Deinitialize reserved VLAN database. */
int 
hsl_bcm_resv_vlan_db_deinit(void) 
{
  HSL_FN_ENTER (); 

  if(!p_resv_vlan_db)
    HSL_FN_EXIT (STATUS_OK); 
  
  /* Delete semaphore. */
  if(p_resv_vlan_db->mutex)
    oss_sem_delete (OSS_SEM_MUTEX, p_resv_vlan_db->mutex);


  /* Free interface <-> vlan database. */  
  if(p_resv_vlan_db->res_vlan_db)
    oss_free (p_resv_vlan_db->res_vlan_db,OSS_MEM_HEAP);

  /* Free db header. */   
  oss_free (p_resv_vlan_db, OSS_MEM_HEAP); 

  HSL_FN_EXIT (STATUS_OK);
}

/* Allocate a reserved vlan. 
   Returns a void * which should be used for freeing the entry.
   Also returns the VID. */
int
hsl_bcm_resv_vlan_allocate (struct hsl_bcm_resv_vlan **retdata)
{
  struct hsl_bcm_resv_vlan *entry;
  int ret;

  HSL_FN_ENTER(); 

  if(!p_resv_vlan_db)
    HSL_FN_EXIT(STATUS_OK); 

  /* Lock the database. */
  HSL_BCM_RESV_VLAN_LOCK;

  /* Return error if free list is empty. */  
  if(!p_resv_vlan_db->free_list)
    {
      HSL_BCM_RESV_VLAN_UNLOCK;

      if (retdata)
        *retdata = NULL;

      HSL_FN_EXIT(STATUS_ERROR);
    }

  /* Alloc first entry from the free list. */  
  entry = p_resv_vlan_db->free_list;
  if (entry->next)
    entry->next->prev = NULL;
  p_resv_vlan_db->free_list = entry->next; 

  /* Add entry to allocated list */
  if(p_resv_vlan_db->alloc_list)
     p_resv_vlan_db->alloc_list->prev = entry;

  entry->next = p_resv_vlan_db->alloc_list;

  /* Update free list ptr */
  p_resv_vlan_db->alloc_list = entry;

  /* Unlock the database. */
  HSL_BCM_RESV_VLAN_UNLOCK;

  /* Create VLAN in H/W. */
  ret = _hsl_bcm_create_vlan (entry->vid);
  if (ret < 0)
    {
      /* Free entry. */
      hsl_bcm_resv_vlan_deallocate (entry);
      HSL_FN_EXIT (STATUS_ERROR);
    }

  /* Set return data. */
  if (retdata)
    *retdata = entry;

  HSL_FN_EXIT(STATUS_OK);   
}

/* Free a allocated reserved vlan. */
int
hsl_bcm_resv_vlan_deallocate (struct hsl_bcm_resv_vlan *entry)
{
  HSL_FN_ENTER(); 

  if (! entry)
    {
      HSL_ERR_WRONG_PARAMS;
      HSL_FN_EXIT(STATUS_ERROR); 
    }

  /* Lock the database. */
  HSL_BCM_RESV_VLAN_LOCK;

  /* Update alloc list */
  if (p_resv_vlan_db->alloc_list == entry)
    p_resv_vlan_db->alloc_list = entry->next; 
  
  if (entry->prev)
    entry->prev->next = entry->next;

  if (entry->next)
    entry->next->prev = entry->prev;

  /* Add entry to the head of free list. */
  p_resv_vlan_db->free_list->prev = entry;
  entry->next = p_resv_vlan_db->free_list;
  entry->prev = NULL;
  p_resv_vlan_db->free_list = entry;

  /* Unlock the database. */
  HSL_BCM_RESV_VLAN_UNLOCK;

  /* Delete VLAN from hardware. */
  _hsl_bcm_delete_vlan (entry->vid);
 
  HSL_FN_EXIT(STATUS_OK); 
}

/* Check if a VLAN is reserved. 
   Returns 1 if the VID is reserved.
   Returns 0 if the VID is unreserved. */
int
hsl_bcm_resv_vlan_is_allocated (hsl_vid_t vid)
{
  int ret = 0;

  HSL_FN_ENTER ();

  /* Lock the database. */
  HSL_BCM_RESV_VLAN_LOCK;

  if ((vid >= p_resv_vlan_db->start_vid) && 
      (vid <= (p_resv_vlan_db->start_vid + p_resv_vlan_db->range)))
    ret = 1;
  
  /* Unlock the database. */
  HSL_BCM_RESV_VLAN_UNLOCK;

  HSL_FN_EXIT (ret);
}
