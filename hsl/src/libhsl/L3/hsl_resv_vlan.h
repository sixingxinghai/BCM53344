/* Copyright (C) 2009  All Rights Reserved. */

#ifndef _HSL_RESV_VLAN_H_
#define _HSL_RESV_VLAN_H_

struct hsl_if_resv_vlan_db;

struct hsl_if_resv_vlan_cb
{
   int (*resv_vlan_init) (hsl_vid_t vid);

   int (*add_vid_to_l3_port) (struct hsl_if *ifp, hsl_vid_t vid);

   int (*del_vid_from_l3_port) (struct hsl_if *ifp, hsl_vid_t vid);

   int (*mod_vid_for_l3_port)  (struct hsl_if *ifp, hsl_vid_t old_vid,
                                hsl_vid_t new_vid);
};

/* Vlan association. */
typedef struct hsl_if_resv_vlan
{
  hsl_vid_t                   vid;   /* Reserved vlan id. */

  /* Entry list management */
  struct hsl_if_resv_vlan    *next;   /* Next entry in list   */
  struct hsl_if_resv_vlan    *prev;   /* Next entry in list   */

  /* Sanity check freed entry should point to alloc db.  */
  struct hsl_if_resv_vlan_db *p_db;   /* Pointer to database. */

  struct hsl_if              *ifp;  /* Pointer to the L3 interface */

}hsl_ifresv_vlan_t;

/* Database for vlan. */
struct hsl_if_resv_vlan_db
{
   struct hsl_if_resv_vlan    *res_vlan_db;    /* reserved_vlan_db.  */
   struct hsl_if_resv_vlan    *free_list;      /* free entries list. */
   struct hsl_if_resv_vlan    *alloc_list;     /* allocated entries. */
   apn_sem_id                 res_vlan_mutex;  /* Protection mutex.  */
   struct hsl_if_resv_vlan_cb *resv_vlan_cb;
};

extern struct hsl_if_resv_vlan_db *p_hsl_if_resv_vlan_db;

#define HSL_IF_RESV_VLAN_CB(FN)  (p_hsl_if_resv_vlan_db->resv_vlan_cb->FN)

#define HSL_RESV_VLAN_IF_LOCK     oss_sem_lock (OSS_SEM_MUTEX, \
                                              p_db->res_vlan_mutex,\
                                              OSS_WAIT_FOREVER)

#define HSL_RESV_VLAN_IF_UNLOCK   oss_sem_unlock (OSS_SEM_MUTEX, \
                                                 p_db->res_vlan_mutex)

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

int hsl_reserved_vlan_db_init (hsl_vid_t first_vid, u_int16_t num,
                               struct hsl_if_resv_vlan_cb *cb);

/****************************************************************************
 * Function: hsl_reserved_vlan_db_deinit-Deinitialize reserved vlan db. *
 * Parameters:                                                              *
 *   void                                                                   *
 * Return value:                                                            *
 *    OK - initialization success.                                          *
 *    Error - otherwise                                                     *
 ****************************************************************************/

int hsl_reserved_vlan_db_deinit();

/****************************************************************************
 * Function: hsl_reserved_vlan_aloc -Allocate vid for L3 port. Routine      *
 * will allocated reserved vid and set l2 port pvid to allocated value      *
 * Parameters:                                                              *
 * IN  ifp - layer 3 interface structure                                    *
 * Return value:                                                            *
 *    OK - initialization success.                                          *  
 *    Error - otherwise                                                     *
 ****************************************************************************/
int hsl_reserved_vlan_alloc (struct hsl_if *ifp);

/****************************************************************************
 * Function: hsl_reserved_vlan_free - Free vid for L3 port. Routine     *
 * will free reserved vid and reset l2 port pvid to default vlan value      *
 * Parameters:                                                              *
 * IN  ifp - layer 3 interface structure                                    *
 * Return value:                                                            *
 *    OK - initialization success.                                          *
 *    Error - otherwise                                                     *
 ****************************************************************************/

int hsl_reserved_vlan_free (struct hsl_if *ifp);

/****************************************************************************
 * Function: hsl_dump_reserved_vlan_db -Dump reserved vlan db.          *
 * Parameters:  void                                                        *
 * Return value:void                                                        *
 ****************************************************************************/
void hsl_dump_reserved_vlan_db (void);

#endif /* _HSL_RESV_VLAN_H_ */
