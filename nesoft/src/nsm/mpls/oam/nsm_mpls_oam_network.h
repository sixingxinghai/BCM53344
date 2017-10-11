/* Copyright (C) 2002  All Rights Reserved. */

#ifndef _PACOS_NSM_MPLS_OAM_NETWORK_H
#define _PACOS_NSM_MPLS_OAM_NETWORK_H

int
nsm_mpls_oam_set_tlv_hdr (struct mpls_oam_tlv_hdr *, u_int16_t , u_int16_t);

int
nsm_mpls_set_echo_request_hdr (struct nsm_mpls_oam *);

int
nsm_mpls_set_echo_reply_hdr (struct mpls_oam_hdr *, struct mpls_oam_hdr *,
                             struct pal_timeval *, u_char , u_char);

int
nsm_mpls_oam_ldp_ivp4_tlv_set (struct nsm_mpls_oam *, struct prefix *,
                               struct ldp_ipv4_prefix*);

int
nsm_mpls_oam_rsvp_ipv4_tlv_set (struct nsm_mpls_oam *, struct ftn_entry *,
                                struct rsvp_ipv4_prefix *);

int
nsm_mpls_oam_l3vpn_tlv_set (struct nsm_mpls_oam *, struct ftn_entry *,
                            struct mpls_oam_target_fec_stack *);

int
nsm_mpls_oam_l2_data_tlv_set (struct nsm_mpls_oam *, struct ftn_entry *,
                              struct mpls_oam_target_fec_stack *);

int
nsm_mpls_oam_vpls_tlv_set (struct nsm_mpls_oam *, struct ftn_entry *,
                           struct mpls_oam_target_fec_stack *);

int
nsm_mpls_oam_gen_ipv4_tlv_set (struct nsm_mpls_oam *,
                               struct generic_ipv4_prefix *);

int
nsm_mpls_oam_nil_fec_tlv_set (struct nsm_mpls_oam *, struct nil_fec_prefix *,
                              u_int32_t);

int
nsm_mpls_oam_fec_tlv_set (struct nsm_mpls_oam *, struct ftn_entry *,
                          struct mpls_oam_target_fec_stack *, u_char);

int
nsm_mpls_oam_reply_fec_tlv_set (struct mpls_oam_echo_request *,
                                struct mpls_oam_target_fec_stack *);

int
nsm_mpls_oam_dsmap_tlv_set (struct nsm_mpls_oam *, struct ftn_entry *,
                            struct mpls_oam_downstream_map *, u_char);

int
nsm_mpls_oam_pad_tlv_set (struct mpls_oam_echo_request *, u_char *, u_int32_t);

int
nsm_mpls_oam_reply_dsmap_tlv_set (struct mpls_oam_echo_request *,
                                  struct mpls_oam_downstream_map *,
                                  struct mpls_label_stack *, struct interface *,
                                  struct mpls_oam_ctrl_data *);

int
nsm_mpls_oam_reply_if_tlv_set (struct nsm_master *, struct mpls_oam_if_lbl_stk*,
                               struct mpls_oam_ctrl_data *);

int
nsm_mpls_oam_reply_pad_tlv_set (struct mpls_oam_pad_tlv *,
                                struct mpls_oam_pad_tlv *);

int
nsm_mpls_set_echo_request_data (struct nsm_mpls_oam *, 
                                struct nsm_mpls_oam_data *);

#endif /* _PACOS_NSM_MPLS_OAM_NETWORK_H */
