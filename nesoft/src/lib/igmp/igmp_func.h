/* Copyright (C) 2001-2005  All Rights Reserved. */

#ifndef _PACOS_IGMP_FUNC_H
#define _PACOS_IGMP_FUNC_H

/* Function Prototype Declarations from igmp.c */
s_int32_t
igmp_initialize (struct igmp_init_params *igin,
                 struct igmp_instance **pp_igi);
s_int32_t
igmp_uninitialize (struct igmp_instance *igi,
                   bool_t forced);
s_int32_t
igmp_svc_register (struct igmp_instance *igi,
                   struct igmp_svc_reg_params *igsrp);
s_int32_t
igmp_svc_deregister (struct igmp_instance *igi,
                     struct igmp_svc_reg_params *igsrp);
s_int32_t
igmp_svc_l3_sock_setopt (struct igmp_svc_reg *igsr);
s_int32_t
igmp_svc_l2_sock_setopt (struct igmp_svc_reg *igsr);
bool_t
igmp_if_svc_reg_match (struct igmp_if *igif,
                       struct igmp_svc_reg *igsr);
u_int32_t
igmp_if_svc_reg_su_id (struct igmp_instance *igi,
                       struct interface *ifp);

u_int32_t
igmp_if_snp_svc_reg_su_id (struct igmp_instance *igi,
                           struct interface *ifp,
                           u_int16_t sec_key);

u_int32_t
igmp_if_hkey_make (void_t *arg);
bool_t
igmp_if_hkey_cmp (void_t *arg1,
                  void_t *arg2);
s_int32_t
igmp_if_get_vif (struct igmp_if *igif,
                 struct igmp_svc_reg *igsr);
s_int32_t
igmp_if_release_vif (struct igmp_if *igif,
                     struct igmp_svc_reg *igsr);
struct igmp_if *
igmp_if_get (struct igmp_instance *igi,
             struct igmp_if_idx *igifidx);
struct igmp_if *
igmp_if_lookup (struct igmp_instance *igi,
                struct igmp_if_idx *igifidx);
struct igmp_if *
igmp_if_lookup_next (struct igmp_instance *igi,
                     struct igmp_if_idx *igifidx);
s_int32_t
igmp_if_delete (struct igmp_if *igif);
struct igmp_if *
igmp_if_create (struct igmp_if_idx *igifidx);
s_int32_t
igmp_if_free (struct igmp_if *igif);
s_int32_t
igmp_if_start (struct igmp_if *igif, struct igmp_if *p_igif);
s_int32_t
igmp_if_stop (struct igmp_if *igif);
s_int32_t
igmp_if_clear (struct igmp_if *igif,
               struct pal_in4_addr *pgrp,
               struct pal_in4_addr *msrc,
               bool_t delstatgrp);
s_int32_t
igmp_if_update (struct igmp_if *igif,
                struct igmp_if *p_igif,
                enum igmp_if_update_mode umode);
s_int32_t
igmp_if_addr_add (struct igmp_if *igif,
                  struct connected *ifc);
s_int32_t
igmp_if_addr_del (struct igmp_if *igif,
                  struct connected *ifc);
s_int32_t
igmp_if_group_join (struct igmp_if *igif,
                    struct igmp_svc_reg *igsr,
                    struct pal_in4_addr *group);
s_int32_t
igmp_if_group_leave (struct igmp_if *igif,
                     struct igmp_svc_reg *igsr,
                     struct pal_in4_addr *group);
s_int32_t
igmp_if_grec_create (struct igmp_if *igif,
                     struct igmp_group_rec **pp_igr);
s_int32_t
igmp_if_grec_rexmit_tib_delete (struct igmp_group_rec *igr);
s_int32_t
igmp_if_grec_delete (struct igmp_group_rec *igr);
struct igmp_group_rec *
igmp_if_grec_get (struct igmp_if *igif,
                  struct pal_in4_addr *pgrp);
struct igmp_group_rec *
igmp_if_hst_grec_get (struct igmp_if *igif,
                      struct pal_in4_addr *pgrp);
s_int32_t
igmp_if_hst_grec_update (struct igmp_if *igif,
                         struct pal_in4_addr *pgrp,
                         struct igmp_group_rec *igr);
struct igmp_group_rec *
igmp_if_grec_get_tib (struct igmp_if *igif,
                      struct ptree *gmr_tib,
                      struct pal_in4_addr *pgrp);
struct igmp_group_rec *
igmp_if_grec_lookup (struct igmp_if *igif,
                     struct pal_in4_addr *pgrp);
struct igmp_group_rec *
igmp_if_grec_lookup_next (struct igmp_if *igif,
                          struct pal_in4_addr *pgrp);
struct igmp_group_rec *
igmp_if_hst_grec_lookup (struct igmp_if *igif,
                         struct pal_in4_addr *pgrp);
struct igmp_group_rec *
igmp_if_grec_lookup_tib (struct igmp_if *igif,
                         struct ptree *gmr_tib,
                         struct pal_in4_addr *pgrp);
s_int32_t
igmp_if_grec_update (struct igmp_group_rec *igr,
                     u_int32_t max_rsp_time);
s_int32_t
igmp_if_grec_update_state_refresh (struct igmp_if *,
                                   struct pal_in4_addr *,
                                   bool_t);
s_int32_t
igmp_if_grec_update_cmode (struct igmp_group_rec *igr);

s_int32_t
igmp_if_grec_update_static (struct igmp_group_rec *igr,
                            struct igmp_source_list *isl,
                            bool_t is_igr_add);
s_int32_t
igmp_if_srec_create (struct igmp_group_rec *igr,
                     struct igmp_source_rec **pp_isr);
s_int32_t
igmp_if_srec_delete (struct igmp_source_rec *isr);
struct igmp_source_rec *
igmp_if_srec_get (struct igmp_group_rec *igr,
                  bool_t use_tib_a,
                  struct pal_in4_addr *msrc);
struct igmp_source_rec *
igmp_if_srec_lookup (struct igmp_group_rec *igr,
                     bool_t use_tib_a,
                     struct pal_in4_addr *msrc);
struct igmp_source_rec *
igmp_if_srec_lookup_next (struct igmp_group_rec *igr,
                          struct pal_in4_addr *msrc);
u_int16_t
igmp_igr_srec_count (struct igmp_group_rec *igr,
                     bool_t use_tib_a,
                     u_int16_t sflag);
s_int32_t
igmp_if_srec_update (struct igmp_source_rec *isr,
                     u_int32_t max_rsp_time);
s_int32_t
igmp_if_grec_timer_v1_host_present (struct thread *t_timer);
s_int32_t
igmp_if_grec_timer_v2_host_present (struct thread *t_timer);
s_int32_t
igmp_if_grec_timer_rexmit_group (struct thread *t_timer);
s_int32_t
igmp_if_grec_timer_rexmit_group_source (struct thread *t_timer);
s_int32_t
igmp_if_grec_timer_liveness (struct thread *t_timer);
s_int32_t
igmp_if_srec_timer_liveness (struct thread *t_timer);
u_int32_t
igmp_if_timer_generate_jitter (u_int32_t time);
s_int32_t
igmp_if_timer_warn_rlimit (struct thread *t_timer);
s_int32_t
igmp_if_timer_querier (struct thread *t_timer);
s_int32_t
igmp_if_timer_other_querier (struct thread *t_timer);
s_int32_t
igmp_if_update_rtp_vars (struct igmp_if *igif);
s_int32_t
igmp_if_access_list_update (struct igmp_if *igif);
s_int32_t
igmp_if_send_lmem_update (struct igmp_if *igif);
s_int32_t
igmp_if_avl_traverse_clear (void_t *avl_node_info,
                            void_t *arg1,
                            void_t *arg2);
s_int32_t
igmp_if_hst_avl_traverse_associate (void_t *avl_node_info,
                                    void_t *arg1);
s_int32_t
igmp_if_hst_avl_traverse_dissociate (void_t *avl_node_info,
                                     void_t *arg1);
s_int32_t
igmp_if_snp_avl_traverse_update (void_t *avl_node_info,
                                 void_t *arg1);
s_int32_t
igmp_if_snp_avl_traverse_match_status (void_t *avl_node_info,
                                       void_t *arg1,
                                       void_t *arg2);
s_int32_t
igmp_if_pxy_service_association (struct igmp_if *hst_igif,
                                 struct igmp_if *ds_igif,
                                 bool_t associate);
s_int32_t
igmp_if_snp_service_association (struct igmp_if *hst_igif,
                                 struct igmp_if *ds_igif,
                                 bool_t associate);
bool_t
igmp_if_snp_start (struct igmp_if *igif);
bool_t
igmp_if_snp_stop (struct igmp_if *igif);
s_int32_t
igmp_if_snp_inherit_config (struct igmp_if *hst_igif);
bool_t
igmp_if_igmp_enabled (struct igmp_if *igif);
bool_t
igmp_if_snp_enabled (struct igmp_if *igif);
s_int32_t
igmp_if_hst_timer_v1_querier_present (struct thread *t_timer);
s_int32_t
igmp_if_hst_timer_v2_querier_present (struct thread *t_timer);
s_int32_t
igmp_if_hst_timer_general_report (struct thread *t_timer);
s_int32_t
igmp_if_hst_timer_group_report (struct thread *t_timer);
s_int32_t
igmp_if_hst_timer_group_source_report (struct thread *t_timer);
s_int32_t
igmp_if_hst_grec_timer_rexmit_group (struct thread *t_timer);
s_int32_t
igmp_if_hst_grec_timer_rexmit_group_source (struct thread *t_timer);
s_int32_t
igmp_if_hst_update_host_cmode (struct igmp_if *hst_igif);
s_int32_t
igmp_if_hst_consolidate_mship (struct igmp_group_rec *igr,
                               struct igmp_source_list *isl);
s_int32_t
igmp_if_hst_sched_report (struct igmp_if *hst_igif,
                          struct pal_in4_addr *pgrp,
                          struct pal_in4_addr *msrc,
                          u_int32_t rsp_time);
s_int32_t
igmp_if_hst_process_mfc_query (struct igmp_if *iigif,
                               u_int32_t mfc_msg_type,
                               struct pal_in4_addr *pgrp,
                               struct pal_in4_addr *msrc);
s_int32_t
igmp_if_igr_timer_general_report (struct thread *t_timer);
s_int32_t
igmp_if_jg_timer_group_report (struct thread *t_timer);
s_int32_t
igmp_if_igr_group_report (struct igmp_if *igif,
                          struct igmp_group_rec *igr,
                          u_int32_t rsp_time);
s_int32_t
igmp_if_ssm_map_process (struct igmp_if *igif,
                         struct pal_in4_addr *pgrp,
                         struct igmp_source_list **isl);


/* Function Prototype Declarations from igmp_cli.c */
s_int32_t
igmp_cli_return (struct cli *cli,
                 s_int32_t iret);
s_int32_t
igmp_debug_config_write (struct cli *cli);
s_int32_t
igmp_config_write (struct cli *cli);
s_int32_t
igmp_if_avl_traverse_config_write (void_t *avl_node_info,
                                   void_t *arg1,
                                   void_t *arg2);

s_int32_t
igmp_if_config_write_all (struct igmp_if *igif, 
                          s_int32_t *write,
                          struct cli *cli,
                          struct interface *ifp);
s_int32_t
igmp_if_config_write (struct cli *cli);
#ifndef HAVE_IMI
s_int32_t
igmp_config_write_interface (struct cli *cli, struct interface *ifp);
#endif /* !HAVE_IMI */
s_int32_t
igmp_cli_init (struct lib_globals *zlg);
s_int32_t
igmp_debug_all_off(struct cli *cli, int argc, char **argv);

/* Function Prototype Declarations from igmp_decode.c */
s_int32_t
igmp_dec_msg_query (struct igmp_if *igif,
                    u_int8_t max_rsp_code,
                    struct pal_in4_addr *psrc,
                    struct pal_in4_addr *pdst);
s_int32_t
igmp_dec_msg_v1_report (struct igmp_if *igif,
                        struct pal_in4_addr *psrc,
                        struct pal_in4_addr *pdst);
s_int32_t
igmp_dec_msg_v2_report (struct igmp_if *igif,
                        struct pal_in4_addr *psrc,
                        struct pal_in4_addr *pdst);
s_int32_t
igmp_dec_msg_v3_group_rec (struct igmp_if *igif,
                           struct pal_in4_addr *psrc,
                           struct pal_in4_addr *pdst);
s_int32_t
igmp_dec_msg_v3_report (struct igmp_if *igif,
                        struct pal_in4_addr *psrc,
                        struct pal_in4_addr *pdst);
s_int32_t
igmp_dec_msg_v2_leave_group (struct igmp_if *igif,
                             struct pal_in4_addr *psrc,
                             struct pal_in4_addr *pdst);
s_int32_t
igmp_dec_msg (struct igmp_if *igif,
              struct pal_in4_addr *psrc,
              struct pal_in4_addr *pdst,
              u_int32_t msg_len);
s_int32_t
igmp_sock_read (struct thread *t_read);

/* Function Prototype Declarations from igmp_encode.c */
s_int32_t
igmp_if_send_general_query (struct igmp_if *igif);
s_int32_t
igmp_if_send_group_query (struct igmp_group_rec *igr);
s_int32_t
igmp_if_send_group_source_query (struct igmp_group_rec *igr,
                                 struct ptree *psrcs_tib,
                                 u_int16_t num_srcs);
s_int32_t
igmp_if_hst_forward_report (struct igmp_if *iigif,
                            struct igmp_if *oigif);
s_int32_t
igmp_if_hst_forward_query (struct igmp_if *iigif,
                           struct igmp_if *oigif);
s_int32_t
igmp_if_hst_send_general_report (struct igmp_if *igif);
s_int32_t
igmp_if_hst_send_group_report (struct igmp_group_rec *igr);
s_int32_t
igmp_if_hst_send_group_source_report (struct igmp_group_rec *hst_igr,
                                      struct ptree *psrcs_tib);
s_int32_t
igmp_if_jg_send_group_report (struct igmp_group_rec *jg_igr);
s_int32_t
igmp_if_jg_send_general_report (struct igmp_if *igif);

s_int32_t
igmp_enc_msg_query (struct igmp_if *igif,
                    struct igmp_group_rec *igr,
                    u_int16_t num_srcs,
                    struct ptree *psrcs_tib,
                    enum igmp_gs_query_type *query_type,
                    u_int16_t *num_enc_srcs,
                    u_int16_t *msg_len);
s_int32_t
igmp_enc_msg_query_hdr (struct igmp_if *igif,
                        u_int32_t max_rsp_time,
                        u_int16_t *msg_len);
s_int32_t
igmp_enc_msg_v3_report (struct igmp_if *hst_igif,
                        struct igmp_group_rec *hst_igr,
                        struct ptree *psrcs_tib,
                        u_int16_t *msg_len,
                        struct ptree_node **last_grp_enc,
                        struct ptree_node **last_src_enc);
s_int32_t
igmp_enc_msg_v3_report_grec (struct igmp_group_rec *hst_igr,
                             enum igmp_hst_rec_type igr_hrt,
                             struct ptree *psrcs_tib,
                             u_int16_t *msg_len,
                             struct ptree_node **last_src_enc);
s_int32_t
igmp_enc_msg_v1_v2_report_leave (struct igmp_group_rec *hst_igr,
                                 u_int8_t msg_type,
                                 u_int16_t *msg_len);
s_int32_t
igmp_sock_write (struct igmp_if *igif,
                 struct pal_in4_addr *psrc,
                 struct pal_in4_addr *pdst,
                 u_int16_t msg_len);

/* Function Prototype Declarations from igmp_fsm.c */
s_int32_t
igmp_igr_fsm_process_event (struct igmp_group_rec *igr,
                            enum igmp_filter_mode_event igr_fme,
                            struct igmp_source_list *isl);
s_int32_t
igmp_igr_fsm_change_state (struct igmp_group_rec *igr,
                           enum igmp_filter_mode_state igr_fms);
s_int32_t
igmp_igr_fsm_action_invalid (struct igmp_group_rec *igr,
                             enum igmp_filter_mode_event igr_fme,
                             struct igmp_source_list *isl);
s_int32_t
igmp_igr_fsm_action_include (struct igmp_group_rec *igr,
                             enum igmp_filter_mode_event igr_fme,
                             struct igmp_source_list *isl);
s_int32_t
igmp_igr_fsm_action_exclude (struct igmp_group_rec *igr,
                             enum igmp_filter_mode_event igr_fme,
                             struct igmp_source_list *isl);
s_int32_t
igmp_igr_fsm_send_lmem_update (struct igmp_group_rec *igr);
s_int32_t
igmp_hst_igr_fsm_process_event (struct igmp_group_rec *hst_igr,
                                enum igmp_hst_filter_mode_event igr_hfme,
                                struct igmp_source_list *isl);
s_int32_t
igmp_hst_igr_fsm_change_state (struct igmp_group_rec *hst_igr,
                               enum igmp_hst_filter_mode_state igr_hfms);
s_int32_t
igmp_hst_igr_fsm_action_invalid (struct igmp_group_rec *hst_igr,
                                 enum igmp_hst_filter_mode_event igr_hfme,
                                 struct igmp_source_list *isl);
s_int32_t
igmp_hst_igr_fsm_action_include (struct igmp_group_rec *hst_igr,
                                 enum igmp_hst_filter_mode_event igr_hfme,
                                 struct igmp_source_list *isl);
s_int32_t
igmp_hst_igr_fsm_action_exclude (struct igmp_group_rec *hst_igr,
                                 enum igmp_hst_filter_mode_event igr_hfme,
                                 struct igmp_source_list *isl);
s_int32_t
igmp_hst_igr_fsm_preprocess_incl_event (struct igmp_group_rec *hst_igr,
                                        bool_t *grec_del,
                                        bool_t *chg_to_incl,
                                        bool_t agg_incl_srcs);

s_int32_t
igmp_hst_igr_fsm_get_block_tib (struct igmp_group_rec *hst_igr);

s_int32_t
igmp_hst_igr_fsm_mfc_prog (struct igmp_group_rec *hst_igr,
                           struct pal_in4_addr *msrc,
                           bool_t mfe_del,
                           bool_t null_olst);
s_int32_t
igmp_hst_igr_fsm_mfc_reprog (struct igmp_group_rec *hst_igr,
                             struct ptree *psrcs_tib);

/* Function Prototype Declarations from igmp_show.c */
s_int32_t
igmp_cli_show_init (struct lib_globals *zlg);

#ifdef HAVE_SNMP
/* Function Prototype Declarations from igmp_snmp.c */

/* Hook functions */
u_int8_t *
igmp_snmp_rtr_if_tab ();
u_int8_t *
igmp_snmp_rtr_cache_tab ();
u_int8_t *
igmp_snmp_inv_rtr_cache_tab ();
u_int8_t *
igmp_snmp_rtr_src_list_tab ();

/* IGMP_SNMP_INIT for Registrating MIB */
s_int32_t
igmp_snmp_init (struct igmp_instance *);

/*IGMP_SNMP_DEINIT for Unregistering MIB */
s_int32_t
igmp_snmp_deinit (struct igmp_instance *);

/*  Utility function to get igmp_snmp_rtr_if_tab index.  */
u_int32_t
igmp_snmp_rtr_if_index_get (struct variable *, oid *, size_t *,
                            struct igmp_snmp_rtr_if_index *,
                            bool_t);

/*  Utility function to get igmp_snmp_rtr_cache_tab Index.  */
u_int32_t
igmp_snmp_rtr_cache_index_get (struct variable *, oid *, size_t *,
                               struct igmp_snmp_rtr_cache_index *,
                               bool_t);
/*  Utility function to get igmp_snmp_inv_rtr_cache_tab index.  */
u_int32_t
igmp_snmp_rtr_inv_cache_index_get (struct variable *, oid *, size_t *,
                                   struct igmp_snmp_inv_rtr_index *,
                                   bool_t);

/*  Utility function to get igmp_snmp_rtr_src_list_tab index.  */
u_int32_t
igmp_snmp_rtr_srclist_index_get (struct variable *, oid *, size_t *,
                                 struct igmp_snmp_rtr_src_list_index *,
                                 bool_t);

/* Utility function to set igmp_snmp_rtr_if_tab index.  */
s_int32_t
igmp_snmp_rtr_if_index_set (struct variable *, oid *, size_t *,
                            struct interface *);

/* Utility function to set igmp_snmp_rtr_cache_tab index.  */
s_int32_t
igmp_snmp_rtr_cache_index_set (struct variable *, oid *, size_t *,
                               struct igmp_snmp_rtr_cache_index *,
                               struct interface *);

/* Utility function to set igmp_snmp_inv_rtr_cache_tab index.  */
s_int32_t
igmp_snmp_rtr_inv_cache_index_set (struct variable *, oid *, size_t *,
                                   struct igmp_snmp_inv_rtr_index *,
                                   struct interface *);

/* Utility function to set igmp_snmp_rtr_src_list_tab index.  */
s_int32_t
igmp_snmp_rtr_srclist_index_set (struct variable *, oid *, size_t *,
                                 struct igmp_snmp_rtr_src_list_index *,
                                 struct interface *);

#endif /* HAVE_SNMP */
#endif /* _PACOS_IGMP_FUNC_H */
