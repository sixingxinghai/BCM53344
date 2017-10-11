/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NSM_MPLS_API_H
#define _PACOS_NSM_MPLS_API_H

#ifndef NSM_API_GET_SUCCESS
#define NSM_API_GET_SUCCESS                   1
#endif /* NSM_API_GET_SUCCESS */

#ifndef NSM_API_GET_ERROR
#define NSM_API_GET_ERROR                     0
#endif /* NSM_API_GET_ERROR */

#ifndef NSM_API_SET_SUCCESS
#define NSM_API_SET_SUCCESS                   1
#endif /* NSM_API_SET_SUCCESS */

#ifndef NSM_API_SET_ERROR
#define NSM_API_SET_ERROR                     0
#endif /* NSM_API_SET_ERROR */

#define SNMP_REDIRECT_LSP               1
#define SNMP_REDIRECT_TUNNEL            2

#define TYPE_NONE                       0
#define TYPE_MAIN                       1
#define TYPE_TEMP                       2

#define IDX_LOOKUP                      1
#define NON_IDX_LOOKUP                  2

/* Flags used from CLI for calling adds */
#define NSM_MPLS_CLI_FTN_PRIMARY           (1 << 0)
#define NSM_MPLS_CLI_FTN_BIDIR_CONFIG      (1 << 1)
#define NSM_MPLS_CLI_FTN_BIDIR_ENABLED     (1 << 2)
#define NSM_MPLS_CLI_FTN_MPLS_CONFIG       (1 << 3)

#define NSM_MPLS_CLI_ILM_BIDIR_CONFIG      (1 << 1)
#define NSM_MPLS_CLI_ILM_BIDIR_ENABLED     (1 << 2)
#define NSM_MPLS_CLI_ILM_BIDIR_FWD         (1 << 3)
#define NSM_MPLS_CLI_ILM_BIDIR_REV_FTN     (1 << 4)
#define NSM_MPLS_CLI_ILM_MPLS_CONFIG       (1 << 5)

#define NSM_MPLS_VALID_INTERFACE(I)                                            \
    ((I) && (if_is_up (I)) && (!if_is_loopback (I)) &&                         \
     ((I)->ls_data.status == LABEL_SPACE_VALID))

#define NSM_MPLS_PHP_ENTRY(I, OPCODE)                                           \
    ((if_is_loopback (I)) && ((OPCODE) == POP))

void
nsm_mpls_ftn_entry_dump (struct cli *cli, struct ftn_entry *ftn,
                         struct prefix *fec, bool_t is_primary);
void
nsm_mpls_ilm_entry_show (struct cli *cli, struct ilm_entry *ilm);
int
nsm_mpls_ftn_entry (struct cli *cli, int vrf, u_int32_t t_id, char *fec,
                    char *fec_mask, char *out_label_str, char *nhop_str,
                    char *ifname,  u_char opcode, bool_t is_add,
                    char *ftn_ix_str, u_int32_t flags, u_int32_t rev_idx);
int
nsm_mpls_ilm_entry_add (struct cli *cli, char *vc_id_str, char *in_lbl_str,
                        char *if_in, char *out_lbl_str, char *if_out,
                        char *nhop_str, u_char opcode, char *fec_str,
                        char *fmask_str, char *ilm_ix_str, u_int32_t flags,
                         u_int32_t rev_idx);
int
nsm_mpls_ilm_entry_del (struct cli *cli, char *in_label_str, char *if_in,
                        char *ilm_ix_str);
void nsm_mpls_api_enable_all_interfaces (struct nsm_master *, u_int16_t);
void nsm_mpls_api_disable_all_interfaces (struct nsm_master *);
int nsm_mpls_api_max_label_val_set (struct nsm_master *, u_int32_t);
int nsm_mpls_api_min_label_val_set (struct nsm_master *, u_int32_t);
int nsm_mpls_api_max_label_val_unset (struct nsm_master *);
int nsm_mpls_api_min_label_val_unset (struct nsm_master *);
int nsm_mpls_api_ls_min_label_val_set (struct nsm_master *,
                                       u_int16_t, u_int32_t);
int nsm_mpls_api_ls_max_label_val_set (struct nsm_master *,
                                       u_int16_t, u_int32_t);
int nsm_mpls_api_ls_min_label_val_unset (struct nsm_master *, u_int16_t);
int nsm_mpls_api_ls_max_label_val_unset (struct nsm_master *, u_int16_t);
void nsm_mpls_api_ingress_ttl_set (struct nsm_master *, int);
void nsm_mpls_api_egress_ttl_set (struct nsm_master *, int);

#ifdef HAVE_DIFFSERV
void nsm_mpls_api_pipe_model_update (struct nsm_master *, bool_t);
#endif /* HAVE_DIFFSERV */

void nsm_mpls_api_ttl_propagate_cap_update (struct nsm_master *, bool_t);
void nsm_mpls_api_local_pkt_handling (struct nsm_master *, bool_t);

int nsm_gmpls_api_add_mapped_route (struct nsm_master *, struct fec_gen_entry *,
                                   struct prefix *);
int nsm_gmpls_api_del_mapped_route (struct nsm_master *, struct fec_gen_entry *,
                                   struct prefix *);
int nsm_gmpls_api_add_lsp_tunnel (struct nsm_master *, struct interface *,
           u_int32_t, u_int32_t, struct prefix *);
int nsm_mpls_api_del_lsp_tunnel (struct nsm_master *, struct interface *,
                             u_int32_t);
#ifdef HAVE_TE
int  nsm_mpls_api_admin_group_add (struct nsm_master *, char *, int);
int  nsm_mpls_api_admin_group_del (struct nsm_master *, char *, int);
#endif /* HAVE_TE */

struct prefix *nsm_mpls_api_cal_max_addr (struct prefix *, struct prefix *);

void nsm_mpls_ftn_delete (struct nsm_master *, struct ftn_entry *,
                          u_int32_t, struct prefix *,
                          struct ptree_ix_table *);

u_int32_t nsm_gmpls_set_ftn_row_status (struct nsm_master *,
                                       u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_descr (struct nsm_master *, u_int32_t, char *);

u_int32_t nsm_mpls_set_ftn_mask (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_addr_type (struct nsm_master *,
                                      u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_src_addr_min_max (struct nsm_master *,
                                             u_int32_t, struct pal_in4_addr *);

u_int32_t nsm_mpls_set_ftn_dst_addr_min_max (struct nsm_master *,
                                             u_int32_t, struct pal_in4_addr *);

u_int32_t nsm_mpls_set_ftn_src_dst_port_min (struct nsm_master *,
                                             u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_src_dst_port_max (struct nsm_master *,
                                             u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_protocol (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_dscp (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_act_type (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_act_pointer (struct nsm_master *, u_int32_t,
                                        u_int32_t, u_int32_t, u_int32_t);

u_int32_t nsm_mpls_set_ftn_st_type (struct nsm_master *, u_int32_t, u_int32_t);

u_int32_t nsm_gmpls_get_if_lb_min_in (struct nsm_master *,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_lb_min_in (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_lb_max_in (struct nsm_master *,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_lb_max_in (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_lb_min_out (struct nsm_master *,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_lb_min_out (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_lb_max_out (struct nsm_master *,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_lb_max_out (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_total_bw (struct nsm_master *,
                                    u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_total_bw (struct nsm_master *,
                                         u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_ava_bw (struct nsm_master *,
                                  u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_ava_bw (struct nsm_master *,
                                       u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_lb_pt_type (struct nsm_master *,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_lb_pt_type (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_in_lb_used (struct nsm_master *,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_in_lb_used (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_failed_lb_lookup (struct nsm_master *,
                                            u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_failed_lb_lookup (struct nsm_master *,
                                                 u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_out_lb_used (struct nsm_master *,
                                       u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_out_lb_used (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_if_out_fragments (struct nsm_master *,
                                         u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_if_out_fragments (struct nsm_master *,
                                              u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_map_ix (struct nsm_master *, u_int32_t,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_map_ix (struct nsm_master *, u_int32_t *,
                                          u_int32_t *, u_int32_t, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_if (struct nsm_master *, u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_if (struct nsm_master *, u_int32_t *,
                                      u_int32_t *);
u_int32_t nsm_gmpls_get_inseg_lb (struct nsm_master *, u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_lb (struct nsm_master *, u_int32_t *,
                                      u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_lb_ptr (struct nsm_master *, u_int32_t, char **);

u_int32_t nsm_gmpls_get_next_inseg_lb_ptr (struct nsm_master *,
                                          u_int32_t *, char **);

u_int32_t nsm_gmpls_get_inseg_npop (struct nsm_master *,
                                   u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_npop (struct nsm_master *,
                                        u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_addr_family (struct nsm_master *,
                                          u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_addr_family (struct nsm_master *,
                                               u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_xc_ix (struct nsm_master *,
                                    u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_xc_ix (struct nsm_master *,
                                         u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_owner (struct nsm_master *, u_int32_t,
                                    u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_owner (struct nsm_master *,
                                         u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_tf_prm (struct nsm_master *, u_int32_t,
                                     char **);
u_int32_t nsm_gmpls_get_next_inseg_tf_prm (struct nsm_master *,
                                          u_int32_t *, char **);

u_int32_t nsm_gmpls_get_inseg_row_status (struct nsm_master *,
                                         u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_row_status (struct nsm_master *,
                                              u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_st_type (struct nsm_master *,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_st_type (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_octs (struct nsm_master *, u_int32_t,
                                   u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_octs (struct nsm_master *,
                                        u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_inseg_pkts (struct nsm_master *, u_int32_t,
                                   u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_pkts (struct nsm_master *,
                                        u_int32_t *, u_int32_t *);
u_int32_t nsm_gmpls_get_inseg_errors (struct nsm_master *, u_int32_t,
                                     u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_errors (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);
u_int32_t nsm_gmpls_get_inseg_discards (struct nsm_master *, u_int32_t,
                                       u_int32_t *);
u_int32_t nsm_gmpls_get_next_inseg_discards (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);
u_int32_t nsm_gmpls_get_inseg_hc_octs (struct nsm_master *, u_int32_t,
                                      ut_int64_t *);
u_int32_t nsm_gmpls_get_next_inseg_hc_octs (struct nsm_master *, u_int32_t *,
                                           ut_int64_t *);
u_int32_t nsm_gmpls_get_inseg_perf_dis_time (struct nsm_master *, u_int32_t,
                                            pal_time_t *);
u_int32_t nsm_gmpls_get_next_inseg_perf_dis_time (struct nsm_master *,
                                                 u_int32_t *, pal_time_t *);


u_int32_t nsm_gmpls_get_outseg_if_ix (struct nsm_master *, u_int32_t,
                                     u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_if_ix (struct nsm_master *, u_int32_t *,
                                          u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_push_top_lb (struct nsm_master *, u_int32_t,
                                           u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_push_top_lb (struct nsm_master *,
                                                u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_top_lb (struct nsm_master *, u_int32_t,
                                      u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_top_lb (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_top_lb_ptr (struct nsm_master *, u_int32_t,
                                          char **);
u_int32_t nsm_gmpls_get_next_outseg_top_lb_ptr (struct nsm_master *,
                                               u_int32_t *, char **);

u_int32_t nsm_gmpls_get_outseg_nxt_hop_ipa_type (struct nsm_master *,
                                                u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_nxt_hop_ipa_type (struct nsm_master *,
                                                     u_int32_t *,
                                                     u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_nxt_hop_ipa (struct nsm_master *,
                                           u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_outseg_nxt_hop_ipa (struct nsm_master *,
                                                u_int32_t *, char **);

u_int32_t nsm_gmpls_get_outseg_xc_ix (struct nsm_master *,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_xc_ix (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_owner (struct nsm_master *,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_owner (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_tf_prm (struct nsm_master *, u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_outseg_tf_prm (struct nsm_master *,
                                           u_int32_t *, char **);

u_int32_t nsm_gmpls_get_outseg_row_status (struct nsm_master *,
                                          u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_row_status (struct nsm_master *,
                                               u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_st_type (struct nsm_master *,
                                       u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_st_type (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_octs (struct nsm_master *,
                                    u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_octs (struct nsm_master *,
                                         u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_pkts (struct nsm_master *,
                                    u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_pkts (struct nsm_master *,
                                         u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_errors (struct nsm_master *,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_errors (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_discards (struct nsm_master *,
                                        u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_outseg_discards (struct nsm_master *,
                                             u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_outseg_hc_octs (struct nsm_master *,
                                       u_int32_t, ut_int64_t *);
u_int32_t nsm_gmpls_get_next_outseg_hc_octs (struct nsm_master *,
                                            u_int32_t *, ut_int64_t *);

u_int32_t nsm_gmpls_get_outseg_perf_dis_time (struct nsm_master *,
                                             u_int32_t, pal_time_t *);
u_int32_t nsm_gmpls_get_next_outseg_perf_dis_time (struct nsm_master *,
                                                  u_int32_t *, pal_time_t *);



u_int32_t nsm_gmpls_get_xc_lspid (struct nsm_master *,
                                 u_int32_t, u_int32_t,
                                 u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_xc_lspid (struct nsm_master *,
                                      u_int32_t *, u_int32_t *,
                                      u_int32_t *, u_int32_t, char **);

u_int32_t nsm_gmpls_get_xc_lb_stk_ix (struct nsm_master *,
                                     u_int32_t, u_int32_t,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_xc_lb_stk_ix (struct nsm_master *,
                                          u_int32_t *, u_int32_t *,
                                          u_int32_t *, u_int32_t,
                                          u_int32_t *);

u_int32_t nsm_gmpls_get_xc_owner (struct nsm_master *,
                                 u_int32_t, u_int32_t,
                                 u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_xc_owner (struct nsm_master *,
                                      u_int32_t *, u_int32_t *,
                                      u_int32_t *, u_int32_t,
                                      u_int32_t *);

u_int32_t nsm_gmpls_get_xc_row_status (struct nsm_master *,
                                      u_int32_t, u_int32_t,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_xc_row_status (struct nsm_master *,
                                           u_int32_t *, u_int32_t *,
                                           u_int32_t *, u_int32_t,
                                           u_int32_t *);

u_int32_t nsm_gmpls_get_xc_st_type (struct nsm_master *,
                                   u_int32_t, u_int32_t,
                                   u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_xc_st_type (struct nsm_master *,
                                        u_int32_t *, u_int32_t *,
                                        u_int32_t *, u_int32_t,
                                        u_int32_t *);

u_int32_t nsm_gmpls_get_xc_adm_status (struct nsm_master *,
                                      u_int32_t, u_int32_t,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_xc_adm_status (struct nsm_master *,
                                           u_int32_t *, u_int32_t *,
                                           u_int32_t *, u_int32_t,
                                           u_int32_t *);

u_int32_t nsm_gmpls_get_xc_oper_status (struct nsm_master *,
                                       u_int32_t, u_int32_t,
                                       u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_xc_oper_status (struct nsm_master *,
                                            u_int32_t *, u_int32_t *,
                                            u_int32_t *, u_int32_t,
                                            u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_row_status (struct nsm_master *,
                                       u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_row_status (struct nsm_master *,
                                            u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_descr (struct nsm_master *,
                                  u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_ftn_descr (struct nsm_master *,
                                       u_int32_t *, char **);

u_int32_t nsm_gmpls_get_ftn_mask (struct nsm_master *,
                                 u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_mask (struct nsm_master *,
                                      u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_addr_type (struct nsm_master *,
                                      u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_addr_type (struct nsm_master *,
                                           u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_src_addr_min (struct nsm_master *,
                                         u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_ftn_src_addr_min (struct nsm_master *,
                                              u_int32_t *, char **);

u_int32_t nsm_gmpls_get_ftn_src_addr_max (struct nsm_master *,
                                         u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_ftn_src_addr_max (struct nsm_master *,
                                              u_int32_t *, char **);

u_int32_t nsm_gmpls_get_ftn_dst_addr_min (struct nsm_master *,
                                         u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_ftn_dst_addr_min (struct nsm_master *,
                                              u_int32_t *, char **);

u_int32_t nsm_gmpls_get_ftn_dst_addr_max (struct nsm_master *,
                                         u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_ftn_dst_addr_max (struct nsm_master *,
                                              u_int32_t *, char **);

u_int32_t nsm_gmpls_get_ftn_src_port_min (struct nsm_master *,
                                         u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_src_port_min (struct nsm_master *,
                                              u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_src_port_max (struct nsm_master *,
                                         u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_src_port_max (struct nsm_master *,
                                              u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_dst_port_min (struct nsm_master *,
                                         u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_dst_port_min (struct nsm_master *,
                                              u_int32_t *,u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_dst_port_max (struct nsm_master *,
                                         u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_dst_port_max (struct nsm_master *,
                                              u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_protocol (struct nsm_master *,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_protocol (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_dscp (struct nsm_master *,
                                 u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_dscp (struct nsm_master *,
                                      u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_act_type (struct nsm_master *,
                                     u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_act_type (struct nsm_master *,
                                          u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_act_pointer (struct nsm_master *,
                                        u_int32_t, char **);
u_int32_t nsm_gmpls_get_next_ftn_act_pointer (struct nsm_master *,
                                             u_int32_t *, char **);


u_int32_t nsm_gmpls_get_ftn_st_type (struct nsm_master *,
                                    u_int32_t, u_int32_t *);
u_int32_t nsm_gmpls_get_next_ftn_st_type (struct nsm_master *,
                                         u_int32_t *, u_int32_t *);

u_int32_t nsm_gmpls_get_ftn_mtch_pkts (struct nsm_master *,
                                      u_int32_t, ut_int64_t *);
u_int32_t nsm_gmpls_get_next_ftn_mtch_pkts (struct nsm_master *,
                                           u_int32_t *, ut_int64_t *);

u_int32_t nsm_gmpls_get_ftn_mtch_octs (struct nsm_master *,
                                      u_int32_t, ut_int64_t *);
u_int32_t nsm_gmpls_get_next_ftn_mtch_octs (struct nsm_master *,
                                           u_int32_t *, ut_int64_t *);

u_int32_t nsm_gmpls_get_ftn_disc_time (struct nsm_master *,
                                      u_int32_t, pal_time_t *);
u_int32_t nsm_gmpls_get_next_ftn_disc_time (struct nsm_master *,
                                           u_int32_t *, pal_time_t *);

int nsm_mpls_check_valid_interface (struct interface *, u_char);

int nsm_mpls_api_max_lbl_range_set (struct cli*, u_int32_t, char*, char*,
                                 u_int16_t);

int nsm_mpls_api_max_lbl_range_unset (struct cli*, u_int32_t, char*, u_int16_t);

void nsm_mpls_dump_label_ranges (struct cli*, u_int16_t);

char* nsm_gmpls_get_lbl_range_service_name (u_int8_t);

int nsm_mpls_api_min_lbl_range_set (struct cli*, u_int32_t, char*, char*,
                                 u_int16_t);

int nsm_mpls_api_min_lbl_range_unset (struct cli*, u_int32_t, char*, u_int16_t);

s_int32_t nsm_gmpls_get_notify (struct nsm_mpls *, s_int32_t *);
s_int32_t nsm_gmpls_ftn_select (struct nsm_master *, struct ftn_entry *,
                                struct fec_gen_entry *);
void nsm_gmpls_ftn_deselect (struct nsm_master *, struct ftn_entry *,
                             struct fec_gen_entry *);
s_int32_t gmpls_ftn_unselected_entry_process (struct nsm_master *,
                                              struct ftn_entry *);

#endif /*_PACOS_NSM_MPLS_API_H*/
