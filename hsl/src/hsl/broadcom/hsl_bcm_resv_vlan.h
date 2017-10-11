/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _HSL_BCM_PORT_VLAN_H_
#define _HSL_BCM_PORT_VLAN_H_

/* Reserved VLAN information. */
struct hsl_bcm_resv_vlan
{
  struct hsl_bcm_resv_vlan     *next;          /* Next entry in list. */
  struct hsl_bcm_resv_vlan     *prev;          /* Previous entry in list. */
  hsl_vid_t                    vid;            /* Reserved vlan id.    */

  struct hsl_bcm_if            *bcmifp;        /* Pointer to the interface using
                                                  this reserved vlan. */
};

/* Database for reserved vlan. This database is used to Layer 3 ports. 
 For L2/L3 switch, these sets of VLANs can't be configured for L2 switching. */
struct hsl_bcm_resv_vlan_db
{
  struct hsl_bcm_resv_vlan    *res_vlan_db;    /* free entries list. */
  struct hsl_bcm_resv_vlan    *free_list;      /* free entries list. */
  struct hsl_bcm_resv_vlan    *alloc_list;     /* allocated entries. */
  apn_sem_id                  mutex;           /* Protection mutex.  */
  hsl_vid_t                   start_vid;       /* Start VID. */
  u_int16_t                   range;           /* Range. */
};

/* Macro for locking reserved vlan database. */
#define HSL_BCM_RESV_VLAN_LOCK                                                    \
  oss_sem_lock  (OSS_SEM_MUTEX, p_resv_vlan_db->mutex, OSS_WAIT_FOREVER)

/* Macro for unlocking reserved vlan database. */
#define HSL_BCM_RESV_VLAN_UNLOCK                                                  \
  oss_sem_unlock(OSS_SEM_MUTEX, p_resv_vlan_db->mutex)

/* Start of the reserved VLAN range. */
#ifdef HAVE_VPLS
#define HSL_MVL_RESV_VLAN_FIRST_DEFAULT  (3774)

/* Range of the reserved VLAN range. */
#define HSL_MVL_RESV_VLAN_NUM_DEFAULT     (320)
#else
#define HSL_MVL_RESV_VLAN_FIRST_DEFAULT  (4034)

/* Range of the reserved VLAN range. */
#define HSL_MVL_RESV_VLAN_NUM_DEFAULT     (60)
#endif /* HAVE_VPLS */


/* Function prototypes. */

/* Initialize reserved VLAN database. */
int hsl_bcm_resv_vlan_db_init(
                              hsl_vid_t start_vid,  /* start of reserved vlans. */
                              int range             /* Range. */
                              );

/* Deinitialize reserved VLAN database. */
int hsl_bcm_resv_vlan_db_deinit(void);

/* Allocate a reserved vlan. 
   Returns a void * which should be used for freeing the entry.
   Also returns the VID. */
int hsl_bcm_resv_vlan_allocate (struct hsl_bcm_resv_vlan **retdata);

/* Free a allocated reserved vlan. */
int hsl_bcm_resv_vlan_deallocate (struct hsl_bcm_resv_vlan *resv_vlan);

/* Check if a VLAN is reserved. 
   Returns 1 if the VID is reserved.
   Returns 0 if the VID is unreserved. */
int hsl_bcm_resv_vlan_is_allocated (hsl_vid_t vid);

#endif /* _HSL_BCM_PORT_VLAN_H_ */
