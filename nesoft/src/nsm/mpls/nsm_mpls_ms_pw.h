/* Copyright (C) 2009-2010  All Rights Reserved. */
/**@file nsm_mpls_ms_pw.h
   @brief This is the header file that contains the new data structures and
          function prototypes required for supporting the MSPW feature.
*/

#ifndef _NSM_MPLS_MS_PW_H
#define _NSM_MPLS_MS_PW_H

#include "hash.h"

#define NSM_MPLS_MS_PW_ID_LEN 4            
#define NSM_MPLS_MS_PW_DESCR_LEN 80            
#define NSM_MPLS_MS_PW_DET_CLI_NUM_COLS    4
#define NSM_MPLS_MS_PW_CLI_NUM_COLS        5
#define NSM_MPLS_MS_PW_VC_TAB_CLI_NUM_COLS 7

#define NSM_MPLS_VLAN_CLI_MIN              2
#define NSM_MPLS_VLAN_CLI_MAX              4094
#define NSM_MPLS_MTU_MIN                   0
#define NSM_MPLS_MTU_MAX                   1500

/**@brief nsm ms pw instance parameters.
*/
struct nsm_mpls_ms_pw
{
  struct nsm_mpls_circuit *vc1; /**< SS-PW 1 */

  struct nsm_mpls_circuit *vc2;/**< SS-PW 2 */

  char *ms_pw_name; /**< name of the MS-PW */

  u_int8_t ms_pw_spe_descr_set; /**< S-PE descr set */

  char ms_pw_spe_descr[NSM_MPLS_MS_PW_DESCR_LEN + 1]; /**< S-PE Description */

  bool_t user_config; /**< user configured param */

  u_int32_t mtu; /**< mtu */

  u_int32_t vlan_id; /**< vlan-id */

#define NSM_MPLS_MSPW_FIB_COMPLETE         (1 << 0)
#define NSM_MPLS_MSPW_MANUAL_VC_FIB_READY  (1 << 1)
  u_int8_t ms_pw_flag; /**< MSPW flag */
};

/* prototype declaration */
void nsm_ms_pw_show_iterator (struct hash_backet *, struct cli *);
void nsm_ms_pw_sync (struct hash_backet *, struct nsm_master *);
u_int32_t nsm_ms_pw_hash_key_make (char *);
bool_t nsm_ms_pw_hash_cmp (struct nsm_mpls_ms_pw *, char *);
int nsm_ms_pw_hash_init (struct nsm_master *);
void * nsm_ms_pw_hash_alloc (char *);
void nsm_ms_pw_remove_from_hash (struct nsm_master *, 
                                 struct nsm_mpls_ms_pw *);
struct nsm_mpls_ms_pw * nsm_ms_pw_lookup_by_name (struct nsm_master * , 
                                                  char *);
struct nsm_mpls_ms_pw * nsm_ms_pw_get_by_name (struct nsm_master * , 
                                               char *, int *);
int nsm_mpls_ms_pw_stitch_vc_instance (struct nsm_master *, char* ,
                                       char *, char* ,
                                       bool_t , u_int32_t ,
                                       u_int32_t );
int nsm_mpls_ms_pw_set_spe_descr (struct nsm_master *, char *,
                                  char *, bool_t );
int nsm_mpls_ms_pw_vc_info_create (struct nsm_master *, 
                                    struct nsm_mpls_circuit *);
void nsm_mpls_ms_pw_show_det (struct nsm_master *, char *, struct vrep_table *);
void nsm_mpls_ms_pw_show (struct hash_backet *, struct vrep_table *);
void nsm_mpls_ms_pw_show_vc_tab (struct nsm_master *, struct cli *, char *);
s_int16_t nsm_mpls_ms_pw_vrep_show (intptr_t , char *);
int nsm_mpls_ms_pw_stitch_send (struct nsm_master *, struct nsm_mpls_ms_pw *,
                                bool_t );
int nsm_mpls_ms_pw_deactivate (struct nsm_master *, struct nsm_mpls_circuit *);
int nsm_mpls_ms_pw_vc_info_cleanup (struct nsm_master *, 
                                    struct nsm_mpls_circuit *);
int nsm_mpls_ms_pw_cleanup (struct nsm_master *, 
                            struct nsm_mpls_circuit *, 
                            s_int32_t);
#endif /* _NSM_MPLS_MS_PW_H */
