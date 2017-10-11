#ifndef _PACOS_NSM_MPLS_VC_API_H
#define _PACOS_NSM_MPLS_VC_API_H

#ifdef HAVE_MPLS_VC
#ifdef HAVE_SNMP
#define NSM_API_GET_SUCCESS                   1
#define NSM_API_GET_ERROR                     0
#define NSM_API_SET_SUCCESS                   1
#define NSM_API_SET_ERROR                     0

#define FCS_RETENTN_DISABLE                   1 /*As per RFC5601*/ 
#define FCS_RETENTN_STATUS_DIS                3 /*As per RFC 5601*/
#define NSM_MPLS_PW_DEF_SETUP_PRIO            0 
#define NSM_MPLS_PW_DEF_HOLD_PRIO             0
#define NSM_MPLS_PW_IF_INDEX                  0     
#define NSM_PW_VPLS_LOC_ATTCH_ID              0
#define NSM_PW_VPLS_PEER_ATTCH_ID             0
#define NSM_MPLS_PW_PEER_ATTCH_IX             0
#define NSM_MPLS_PW_TTL_VALUE                 2/*default value*/
#define NSM_MPLS_PW_OUTER_TUNNEL              1 /*default value*/
#define NSM_MPLS_VC_ADMINSTATE_UP             1
#define NSM_MPLS_VC_ADMINSTATE_DOWN           2

#define MPLSTE                                0
#define MPLSNONTE                             1
#define PWONLY                                2
/*For vpls not up, returning Row status as 3(Not Ready) */
#define NSM_VPLS_STATE_NOT_READY              3

#define NSM_MPLS_PW_PAL_TRUE                  1
#define NSM_MPLS_PW_PAL_FALSE                 2
#define NSM_MPLS_PW_NOT_SUPPORTED             0

#define TNL_TYPE_NOT_KNOWN                    1
#define TNL_TYPE_MPLSTE                       2
#define TNL_TYPE_MPLSNONTE                    3
#define TNL_TYPE_PWONLY                       4

#define SNMP_REDIRECT_LSP               1
#define SNMP_REDIRECT_TUNNEL            2

#define TYPE_NONE                       0
#define TYPE_MAIN                       1
#define TYPE_TEMP                       2

#define IDX_LOOKUP                      1
#define NON_IDX_LOOKUP                  2

#define PW_CW_SET                       5
#define PW_CW_NOT_SET                   6

#define PW_L2_STATUS_UP                 1
#define PW_L2_STATUS_DOWN               2
#define NSM_PW_VALID_INTERFACE(I)                                            \
    ((I) && (if_is_up (I)) && (!if_is_loopback (I)) &&                         \
     ((I)->ls_data.status == LABEL_SPACE_VALID))

#define SET_PW_VAL(A,M,V) \
    (A = ((A & ~(M)) | V))

struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_lookup (u_int32_t, u_int32_t);

struct nsm_mpls_circuit_temp *
nsm_mpls_l2_circuit_temp_lookup_by_name (char *, u_int32_t);
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_by_addr (struct nsm_master *nm,
                                         u_int32_t *pw_type,
                                         u_int32_t *peer_addr_type,
                                         struct pal_in4_addr *peer_addr,
                                         u_int32_t *pw_ix );

struct nsm_mpls_circuit *
nsm_mpls_vc_snmp_lookup_next_by_intvl (struct nsm_master *nm,
                                       u_int32_t *pw_ix,
                                       u_int32_t *pw_perf_intrvl_ix);
struct nsm_mpls_circuit *
nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (struct nsm_master *nm,
                                           u_int32_t *pw_ix,
                                           u_int32_t *pw_perf_intrvl_ix);

struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_for_te_map (struct nsm_master *nm,
                                            u_int32_t *tunnel_id,
                                            u_int32_t *tunnel_inst,
                                            struct pal_in4_addr *peer_lsr,
                                            struct pal_in4_addr *lcl_lsr,
                                            u_int32_t *pw_ix);
int
nsm_mpls_getnext_index_te_map_tunnel_inst (struct nsm_master *nm,
                                  struct nsm_mpls_circuit **vc,
                                  u_int32_t *tunnel_id,
                                  u_int32_t *tunnel_inst,
                                  struct pal_in4_addr *peer_lsr,
                                  struct pal_in4_addr *lcl_lsr,
                                  u_int32_t *pw_ix);
int
nsm_mpls_getnext_index_te_map_peer_lsr (struct nsm_master *nm,
                               struct nsm_mpls_circuit **vc,
                               u_int32_t *tunnel_id,
                               u_int32_t *tunnel_inst,
                               struct pal_in4_addr *peer_lsr,
                               struct pal_in4_addr *lcl_lsr,
                               u_int32_t *pw_ix);
int
nsm_mpls_getnext_index_te_map_lcl_lsr (struct nsm_master *nm,
                              struct nsm_mpls_circuit **vc,
                              u_int32_t *tunnel_id,
                              u_int32_t *tunnel_inst,
                              struct pal_in4_addr *peer_lsr,
                              struct pal_in4_addr *lcl_lsr,
                              u_int32_t *pw_ix);
int
nsm_mpls_getnext_index_te_map_pw_ix (struct nsm_master *nm,
                            struct nsm_mpls_circuit **vc,
                            u_int32_t *tunnel_id,
                            u_int32_t *tunnel_inst,
                            struct pal_in4_addr *peer_lsr,
                            struct pal_in4_addr *lcl_lsr,
                            u_int32_t *pw_ix);
u_int32_t
nsm_mpls_getnext_lcl_lsr (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                 u_int32_t *tunnel_id, u_int32_t *tunnel_inst,
                 struct pal_in4_addr *peer_lsr, struct pal_in4_addr *lcl_lsr);
u_int32_t
nsm_mpls_getnext_peer_lsr (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                  u_int32_t *tunnel_id, u_int32_t *tunnel_inst,
                  struct pal_in4_addr *peer_lsr);
u_int32_t
nsm_mpls_getnext_tunnel_inst (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                     u_int32_t *tunnel_id, u_int32_t *tunnel_inst);
u_int32_t
nsm_mpls_getnext_tunnel_id (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                   u_int32_t *tunnel_id);
u_int32_t
nsm_mpls_getnext_xc_ix (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
               u_int32_t *dn, u_int32_t *xc_ix);
u_int32_t
nsm_mpls_getnext_dn (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
            u_int32_t *dn);
struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_for_non_te_map (struct nsm_master *nm,
                                                u_int32_t *dn,
                                                u_int32_t *xc_ix,
                                                u_int32_t *pw_ix);
int
nsm_mpls_getnext_index_non_te_map_xc_ix (struct nsm_master *nm,
                                struct nsm_mpls_circuit **vc,
                                u_int32_t *dn,
                                u_int32_t *xc_ix,
                                u_int32_t *pw_ix);
int
nsm_mpls_getnext_index_non_te_map_pw_ix (struct nsm_master *nm,
                                struct nsm_mpls_circuit **vc,
                                u_int32_t *dn,
                                u_int32_t *xc_ix,
                                u_int32_t *pw_ix);

/* Forward Declaration */
struct ldp_id;
int nsm_vlan_id_validate_by_interface (struct interface *, u_int16_t);


/* Row Status support functions' prototypes */
u_int32_t
nsm_mpls_create_new_nsm_mpls_circuit (struct nsm_master *,
                             struct nsm_mpls_circuit_temp **,
                             struct nsm_mpls_circuit **);
u_int32_t
nsm_mpls_create_new_vc_temp (struct nsm_master *,
                             struct nsm_mpls_circuit *, u_int32_t);

u_int32_t nsm_mpls_get_pw_index_next (struct nsm_master *,
                                      u_int32_t *);
u_int32_t nsm_mpls_get_pw_up_dn_notify (struct nsm_master*, u_int32_t *);
u_int32_t nsm_mpls_get_pw_del_notify (struct nsm_master *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_notify_rate (struct nsm_master *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_tot_err_pckts (struct nsm_master *,
                                         u_int32_t *);
u_int32_t nsm_mpls_get_pw_type (struct nsm_master *,
                                u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_type (struct nsm_master *,
                                     u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_owner (struct nsm_master *,
                                 u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_owner (struct nsm_master *,
                                      u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_psn_type (struct nsm_master *,
                                    u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_psn_type (struct nsm_master *,
                                         u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_setup_prty (struct nsm_master *,
                                      u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_setup_prty (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_hold_prty (struct nsm_master *,
                                     u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_hold_prty (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_peer_addr_type (struct nsm_master *,
                                          u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_peer_addr_type (struct nsm_master *,
                                               u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_peer_addr (struct nsm_master *,
                                     u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_peer_addr (struct nsm_master *,
                                          u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_attchd_pw_ix (struct nsm_master *,
                                        u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_attchd_pw_ix (struct nsm_master *,
                                             u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_if_ix (struct nsm_master *,
                                 u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_if_ix (struct nsm_master *,
                                      u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_id (struct nsm_master *,
                              u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_id (struct nsm_master *,
                                   u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_local_grp_id (struct nsm_master *,
                                        u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_local_grp_id (struct nsm_master *,
                                             u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_grp_attchmt_id (struct nsm_master *,
                                          u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_grp_attchmt_id (struct nsm_master *,
                                               u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_local_attchmt_id (struct nsm_master *,
                                            u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_local_attchmt_id (struct nsm_master *,
                                                 u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_peer_attchmt_id (struct nsm_master *,
                                           u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_peer_attchmt_id (struct nsm_master *,
                                                u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_cw_prfrnce (struct nsm_master *,
                                      u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_cw_prfrnce (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_local_if_mtu (struct nsm_master *,
                                        u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_local_if_mtu (struct nsm_master *,
                                             u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_local_if_string (struct nsm_master *,
                                           u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_local_if_string (struct nsm_master *,
                                                u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_local_capab_advrt (struct nsm_master *,
                                             u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_local_capab_advrt (struct nsm_master *,
                                                  u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_remote_grp_id (struct nsm_master *,
                                         u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_remote_grp_id (struct nsm_master *,
                                              u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_cw_status (struct nsm_master *,
                                     u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_cw_status (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_remote_if_mtu (struct nsm_master *,
                                         u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_remote_if_mtu (struct nsm_master *,
                                              u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_remote_if_string (struct nsm_master *,
                                            u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_remote_if_string (struct nsm_master *,
                                                 u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_remote_capab (struct nsm_master *,
                                        u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_remote_capab (struct nsm_master *,
                                             u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_frgmt_cfg_size (struct nsm_master *,
                                          u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_frgmt_cfg_size (struct nsm_master *,
                                               u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_rmt_frgmt_capab (struct nsm_master *,
                                           u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_rmt_frgmt_capab (struct nsm_master *,
                                                u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_fcs_retentn_cfg (struct nsm_master *,
                                           u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_fcs_retentn_cfg (struct nsm_master *,
                                                u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_fcs_retentn_status (struct nsm_master *,
                                              u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_fcs_retentn_status (struct nsm_master *,
                                                   u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_outbd_label (struct nsm_master *,
                                       u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_outbd_label (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_inbd_label (struct nsm_master *nm,
                                      u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_inbd_label (struct nsm_master *nm,
                                           u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_name (struct nsm_master *,
                                u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_name (struct nsm_master *,
                                     u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_descr (struct nsm_master *,
                                 u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_descr (struct nsm_master *,
                                      u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_create_time (struct nsm_master *,
                                       u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_create_time (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_up_time (struct nsm_master *,
                                   u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_up_time (struct nsm_master *,
                                        u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_last_change (struct nsm_master *,
                                       u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_last_change (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_admin_status (struct nsm_master *,
                                        u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_admin_status (struct nsm_master *,
                                             u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_oper_status (struct nsm_master *,
                                       u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_oper_status (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_local_status (struct nsm_master *,
                                        u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_local_status (struct nsm_master *,
                                             u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_rmt_status_capab (struct nsm_master *,
                                            u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_rmt_status_capab (struct nsm_master *,
                                                 u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_rmt_status (struct nsm_master *,
                                      u_int32_t , char **);
u_int32_t nsm_mpls_get_next_pw_rmt_status (struct nsm_master *,
                                           u_int32_t *, char **);
u_int32_t nsm_mpls_get_pw_time_elapsed (struct nsm_master *,
                                        u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_time_elapsed (struct nsm_master *,
                                             u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_valid_intrvl (struct nsm_master *,
                                        u_int32_t , u_int64_t *);
u_int32_t nsm_mpls_get_next_pw_valid_intrvl (struct nsm_master *,
                                             u_int32_t *, u_int64_t *);
u_int32_t nsm_mpls_get_pw_st_type (struct nsm_master *,
                                   u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_st_type (struct nsm_master *,
                                        u_int32_t *, u_int32_t *);
u_int32_t nsm_mpls_get_pw_row_status (struct nsm_master *,
                                      u_int32_t , u_int32_t *);
u_int32_t nsm_mpls_get_next_pw_row_status (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int64_t nsm_mpls_get_pw_crnt_in_hc_pckts (struct nsm_master *,
                                            u_int32_t , u_int64_t *);
u_int64_t 
nsm_mpls_get_next_pw_crnt_in_hc_pckts (struct nsm_master *,
                                       u_int32_t *, u_int64_t *);
u_int64_t
nsm_mpls_get_pw_crnt_in_hc_bytes (struct nsm_master *,
                                  u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_crnt_in_hc_bytes (struct nsm_master *,
                                       u_int32_t *, u_int64_t *);
u_int64_t
nsm_mpls_get_pw_crnt_out_hc_pckts (struct nsm_master *,
                                   u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_crnt_out_hc_pckts (struct nsm_master *,
                                        u_int32_t *, u_int64_t *);
u_int64_t
nsm_mpls_get_pw_crnt_out_hc_bytes (struct nsm_master *,
                                   u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_crnt_out_hc_bytes (struct nsm_master *,
                                        u_int32_t *, u_int64_t *);
u_int32_t
nsm_mpls_get_pw_crnt_in_pckts (struct nsm_master *,
                               u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_crnt_in_pckts (struct nsm_master *,
                                    u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_crnt_in_bytes (struct nsm_master *,
                               u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_crnt_in_bytes (struct nsm_master *,
                                    u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_crnt_out_pckts (struct nsm_master *,
                                u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_crnt_out_pckts (struct nsm_master *,
                                     u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_crnt_out_bytes (struct nsm_master *,
                                u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_crnt_out_bytes (struct nsm_master *,
                                     u_int32_t *, u_int32_t *);

u_int32_t
nsm_mpls_get_pw_intrvl_valid_dt (struct nsm_master *,
                                 u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_intrvl_valid_dt (struct nsm_master *,
                                      u_int32_t * , u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_intrvl_time_elpsd (struct nsm_master *,
                                   u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_intrvl_time_elpsd (struct nsm_master *,
                                        u_int32_t * , u_int32_t *, u_int32_t *);
u_int64_t
nsm_mpls_get_pw_intrvl_in_hc_pckts (struct nsm_master *,
                                    u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_intrvl_in_hc_pckts (struct nsm_master *,
                                         u_int32_t  *, u_int32_t *,
                                         u_int64_t *);
u_int64_t
nsm_mpls_get_pw_intrvl_in_hc_bytes (struct nsm_master *,
                                    u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_intrvl_in_hc_bytes (struct nsm_master *,
                                         u_int32_t  *, u_int32_t *,
                                         u_int64_t *);
u_int64_t
nsm_mpls_get_pw_intrvl_out_hc_pckts (struct nsm_master *,
                                     u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_intrvl_out_hc_pckts (struct nsm_master *,
                                          u_int32_t * , u_int32_t *,
                                          u_int64_t *);
u_int64_t
nsm_mpls_get_pw_intrvl_out_hc_bytes (struct nsm_master *,
                                     u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_intrvl_out_hc_bytes (struct nsm_master *,
                                          u_int32_t  *, u_int32_t *,
                                          u_int64_t *);
u_int32_t
nsm_mpls_get_pw_intrvl_in_pckts (struct nsm_master *,
                                 u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_intrvl_in_pckts (struct nsm_master *,
                                      u_int32_t  *, u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_intrvl_in_bytes (struct nsm_master *,
                                 u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_intrvl_in_bytes (struct nsm_master *,
                                      u_int32_t * , u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_intrvl_out_pckts (struct nsm_master *,
                                  u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_intrvl_out_pckts (struct nsm_master *,
                                       u_int32_t  *, u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_intrvl_out_bytes (struct nsm_master *,
                                  u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_intrvl_out_bytes (struct nsm_master *,
                                       u_int32_t  *, u_int32_t *, u_int32_t *);

u_int32_t
nsm_mpls_get_pw_1dy_valid_dt (struct nsm_master *,
                              u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_1dy_valid_dt (struct nsm_master *,
                                   u_int32_t  *, u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_get_pw_1dy_moni_secs (struct nsm_master *,
                               u_int32_t , u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_1dy_moni_secs (struct nsm_master *,
                                    u_int32_t  *, u_int32_t *, u_int32_t *);
u_int64_t
nsm_mpls_get_pw_1dy_in_hc_pckts (struct nsm_master *,
                                 u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_1dy_in_hc_pckts (struct nsm_master *,
                                      u_int32_t  *, u_int32_t *, u_int64_t *);
u_int64_t
nsm_mpls_get_pw_1dy_in_hc_bytes (struct nsm_master *,
                                 u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_1dy_in_hc_bytes (struct nsm_master *,
                                      u_int32_t  *, u_int32_t *, u_int64_t *);
u_int64_t
nsm_mpls_get_pw_1dy_out_hc_pckts (struct nsm_master *,
                                  u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_1dy_out_hc_pckts (struct nsm_master *,
                                       u_int32_t  *, u_int32_t *,
                                       u_int64_t *);
u_int64_t
nsm_mpls_get_pw_1dy_out_hc_bytes (struct nsm_master *,
                                  u_int32_t , u_int32_t , u_int64_t *);
u_int64_t
nsm_mpls_get_next_pw_1dy_out_hc_bytes (struct nsm_master *,
                                       u_int32_t  *, u_int32_t *,
                                       u_int64_t *);
u_int32_t
nsm_mpls_get_pw_id_map_pw_ix (struct nsm_master *,
                              u_int32_t , u_int32_t ,
                              u_int32_t ,
                              struct pal_in4_addr , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_id_map_pw_ix (struct nsm_master *,
                                   u_int32_t *, u_int32_t *,
                                   u_int32_t *,
                                   struct pal_in4_addr *,
                                   u_int32_t *);
u_int32_t
nsm_mpls_get_pw_peer_map_pw_ix (struct nsm_master *,
                                u_int32_t ,
                                struct pal_in4_addr ,
                                u_int32_t ,
                                u_int32_t , u_int32_t *);
u_int32_t
nsm_mpls_get_next_pw_peer_map_pw_ix (struct nsm_master *,
                                     u_int32_t *,
                                     struct pal_in4_addr *,
                                     u_int32_t *,
                                     u_int32_t *, u_int32_t *);
u_int32_t
nsm_mpls_set_pw_up_dn_notify (struct nsm_mpls *, u_int32_t);
u_int32_t
nsm_mpls_set_pw_del_notify (struct nsm_mpls *nmpls, u_int32_t val);
u_int32_t
nsm_mpls_set_pw_notify_rate (struct nsm_mpls *nmpls, u_int32_t val);
u_int32_t nsm_mpls_get_pw_enet_pw_instance (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_pw_vlan (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_vlan_mode (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_port_vlan (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_port_if_index (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_pw_if_index (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_row_status (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_storage_type (struct nsm_master *,
                                        u_int32_t , u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_pw_instance (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_pw_vlan (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_vlan_mode (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_port_vlan (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_port_if_index (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_pw_if_index (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_row_status (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_storage_type (struct nsm_master *,
                                        u_int32_t *, u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_stats_illegal_vlan (struct nsm_master *,
                                                   u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_pw_enet_stats_illegal_length (struct nsm_master *,
                                                     u_int32_t, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_stats_illegal_vlan (struct nsm_master *,
                                                        u_int32_t *, u_int32_t *);

u_int32_t nsm_mpls_get_next_pw_enet_stats_illegal_length (struct nsm_master *,
                                                          u_int32_t *, u_int32_t *);

/* prototypes for PW MPLS MIB */
u_int32_t
nsm_mpls_get_pw_mpls_mpls_type (struct nsm_master *,
                                u_int32_t, char **);
u_int32_t
nsm_mpls_get_next_pw_mpls_mpls_type (struct nsm_master *,
                                     u_int32_t *, char **);
u_int32_t
nsm_mpls_get_pw_mpls_exp_bits_mode (struct nsm_master *nm,
                                    u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_exp_bits_mode (struct nsm_master *nm,
                                         u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_exp_bits (struct nsm_master *nm,
                               u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_exp_bits (struct nsm_master *nm,
                                    u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_ttl (struct nsm_master *nm,
                          u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_ttl (struct nsm_master *nm,
                               u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_lcl_ldp_id (struct nsm_master *nm,
                                 u_int32_t pw_ix, char **ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_lcl_ldp_id (struct nsm_master *nm,
                                      u_int32_t *pw_ix, char **ret);
u_int32_t
nsm_mpls_get_pw_mpls_lcl_ldp_entty_ix (struct nsm_master *nm,
                                       u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_lcl_ldp_entty_ix (struct nsm_master *nm,
                                            u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_peer_ldp_id (struct nsm_master *nm,
                                  u_int32_t pw_ix, char **ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_peer_ldp_id (struct nsm_master *nm,
                                       u_int32_t *pw_ix, char **ret);
u_int32_t
nsm_mpls_get_pw_mpls_st_type (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_st_type (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_outbd_lsr_xc_ix (struct nsm_master *nm,
                                      u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_lsr_xc_ix (struct nsm_master *nm,
                                           u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_ix (struct nsm_master *nm,
                                   u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_ix (struct nsm_master *nm,
                                        u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_instnce (struct nsm_master *nm,
                                        u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_instnce (struct nsm_master *nm,
                                             u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_lcl_lsr (struct nsm_master *nm,
                                        u_int32_t pw_ix, char **ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_lcl_lsr (struct nsm_master *nm,
                                             u_int32_t *pw_ix, char **ret);
u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_peer_lsr (struct nsm_master *nm,
                                         u_int32_t pw_ix, char **ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_peer_lsr (struct nsm_master *nm,
                                              u_int32_t *pw_ix, char **ret);
u_int32_t
nsm_mpls_get_pw_mpls_outbd_if_ix (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_if_ix (struct nsm_master *nm,
                                       u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_typ_inuse (struct nsm_master *nm,
                                          u_int32_t pw_ix, u_int32_t *ret);

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_typ_inuse (struct nsm_master *nm,
                                               u_int32_t *pw_ix,
                                               u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_inbd_xc_ix (struct nsm_master *nm,
                                 u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_inbd_xc_ix (struct nsm_master *nm,
                                      u_int32_t *pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_non_te_map_pw_ix (struct nsm_master *nm,
                                       u_int32_t dn, u_int32_t xc_ix,
                                       u_int32_t if_ix,
                                       u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_non_te_map_pw_ix (struct nsm_master *nm,
                                   u_int32_t *dn, u_int32_t *xc_ix,
                                   u_int32_t *if_ix, u_int32_t *pw_ix,
                                   u_int32_t *ret);
u_int32_t
nsm_mpls_get_pw_mpls_te_map_pw_ix (struct nsm_master *nm,
                                   u_int32_t tunl_ix, u_int32_t tnl_inst,
                                   struct pal_in4_addr peer_lsr,
                                   struct pal_in4_addr lcl_lsr,
                                   u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_mpls_te_map_pw_ix (struct nsm_master *nm,
                                        u_int32_t *tunl_ix,
                                        u_int32_t *tnl_inst,
                                        struct pal_in4_addr *peer_lsr,
                                        struct pal_in4_addr *lcl_lsr,
                                        u_int32_t *pw_ix, u_int32_t *ret);

u_int32_t
nsm_mpls_get_pw_oam_en (struct nsm_master *nm,
                        u_int32_t pw_ix, u_int32_t *ret);

u_int32_t
nsm_mpls_get_next_pw_oam_en (struct nsm_master *nm,
                             u_int32_t *pw_ix, u_int32_t *ret);

u_int32_t
nsm_mpls_get_pw_gen_agi_type (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret);

u_int32_t
nsm_mpls_get_next_pw_gen_agi_type (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret);

u_int32_t
nsm_mpls_get_pw_gen_loc_aii_type (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret);

u_int32_t
nsm_mpls_get_next_pw_gen_loc_aii_type (struct nsm_master *nm,
                                       u_int32_t *pw_ix,
                                       u_int32_t *ret);

u_int32_t
nsm_mpls_get_pw_gen_rem_aii_type (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret);
u_int32_t
nsm_mpls_get_next_pw_gen_rem_aii_type (struct nsm_master *nm,
                                       u_int32_t *pw_ix,
                                       u_int32_t *ret);

u_int32_t
nsm_mpls_get_pw_gen_fec_vc_id (struct nsm_master *nm,
                               u_int32_t,
                               u_int32_t,
                               u_int32_t,
                               u_int32_t,
                               u_int32_t,
                               u_int32_t,
                               u_int32_t *ret);

u_int32_t
nsm_mpls_get_next_pw_gen_fec_vc_id (struct nsm_master *nm,
                                     u_int32_t *,
                                     u_int32_t *,
                                     u_int32_t *,
                                     u_int32_t *,
                                     u_int32_t *,
                                     u_int32_t *,
                                     u_int32_t *ret);

struct nsm_mpls_circuit *
nsm_pw_snmp_get_vc_next_index (struct nsm_master *nm,
     struct route_table *vc_table, u_int32_t vc_index);

struct nsm_vpls_peer *
nsm_pw_snmp_get_vpls_next_index (struct nsm_master *nm,
                                struct route_table *vpls_table, 
                                u_int32_t vc_index);

struct nsm_vpls_peer*
nsm_pw_snmp_get_all_peers_vpls_next_index (struct nsm_master *nm,
                                         u_int32_t vc_index);


int
nsm_mpls_get_vc_addr_fam (u_int32_t peer_addr_type);

#endif /* HAVE_SNMP */
#endif /* HAVE_MPLS_VC */
#endif /*_PACOS_NSM_MPLS_VC_API_H*/
