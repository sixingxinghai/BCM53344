/* Copyright (C) 2002  All Rights Reserved. */

#ifndef _PACOS_NSM_MPLS_OAM_PACKET_H
#define _PACOS_NSM_MPLS_OAM_PACKET_H

int
nsm_mpls_oam_decode_echo_reply (u_char **, u_int16_t *, struct mpls_oam_hdr *,
                                struct mpls_oam_echo_reply *, u_char *);

int
nsm_mpls_oam_decode_tlv_header (u_char **, u_int16_t *, 
                                struct mpls_oam_tlv_hdr* );

int
nsm_mpls_oam_ldp_ipv4_decode (u_char **, u_int16_t *, struct ldp_ipv4_prefix *,
                              struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_rsvp_ipv4_decode (u_char **, u_int16_t *,
                               struct rsvp_ipv4_prefix *,
                               struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_l3vpn_ipv4_decode (u_char **, u_int16_t *,
                                struct l3vpn_ipv4_prefix *,
                                struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_l2dep_ipv4_decode (u_char **, u_int16_t *,
                                struct l2_circuit_tlv_dep *,
                                struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_l2curr_ipv4_decode (u_char **, u_int16_t *,
                                 struct l2_circuit_tlv_curr *,
                                 struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_pw_ipv4_decode (u_char **, u_int16_t *, struct mpls_pw_tlv *,
                             struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_gen_ipv4_decode (u_char **, u_int16_t *,
                              struct generic_ipv4_prefix *,
                              struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_decode_unknown_tlv (u_char **, u_int16_t *,
                                 struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_decode_fec_tlv (u_char **, u_int16_t *,
                             struct mpls_oam_target_fec_stack *,
                             struct mpls_oam_tlv_hdr *, int *);

int
nsm_mpls_oam_decode_dsmap_tlv (u_char **, u_int16_t *,
                               struct mpls_oam_downstream_map *,
                               struct mpls_oam_tlv_hdr *, u_char);

int
nsm_mpls_oam_decode_if_lbl_stack_tlv (u_char **, u_int16_t *,
                                      struct mpls_oam_if_lbl_stk *,
                                      struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_decode_errored_tlv (u_char **, u_int16_t *,
                                 struct mpls_oam_errored_tlv *,
                                 struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_decode_pad_tlv (u_char **, u_int16_t *,
                             struct mpls_oam_pad_tlv *,
                             struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_decode_rtos_tlv (u_char **, u_int16_t *,
                              struct mpls_oam_reply_tos *,
                              struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_header_decode (u_char **, u_int16_t *, struct mpls_oam_hdr *);

int
nsm_mpls_oam_decode_echo_request (u_char **, u_int16_t *,
                                  struct mpls_oam_hdr *,
                                  struct mpls_oam_echo_request *, int *);

int
mpls_oam_tlv_header_encode (u_char **, u_int16_t *,
                            struct mpls_oam_tlv_hdr *);

int
nsm_mpls_oam_encode_oam_header (u_char **, u_int16_t *,
                                struct mpls_oam_hdr *, u_char);

int
nsm_mpls_oam_encode_ldp_ipv4_tlv (u_char **, u_int16_t *, 
                                  struct ldp_ipv4_prefix *);

int
nsm_mpls_oam_encode_rsvp_ipv4_tlv (u_char **, u_int16_t *,
                                   struct rsvp_ipv4_prefix *);

int
nsm_mpls_oam_encode_l3vpn_ipv4_tlv (u_char **, u_int16_t *,
                                    struct l3vpn_ipv4_prefix *);

int
nsm_mpls_oam_encode_l2_dep_tlv (u_char **, u_int16_t *,
                                struct l2_circuit_tlv_dep *);

int
nsm_mpls_oam_encode_l2_curr_tlv (u_char **, u_int16_t *,
                                 struct l2_circuit_tlv_curr *);

int
nsm_mpls_oam_encode_pw_tlv (u_char **, u_int16_t *, struct mpls_pw_tlv *pw_tlv);

int
nsm_mpls_oam_encode_gen_ipv4_tlv (u_char **, u_int16_t *,
                                  struct generic_ipv4_prefix *);

int
nsm_mpls_oam_encode_fec_tlv (u_char **, u_int16_t *,
                             struct mpls_oam_target_fec_stack *,
                             u_char);

int
nsm_mpls_oam_encode_pad_tlv (u_char **, u_int16_t *,
                             struct mpls_oam_pad_tlv *,
                             u_char);

int
nsm_mpls_oam_encode_dsmap_tlv (u_char **, u_int16_t *,
                               struct mpls_oam_downstream_map *,
                               u_char);

int
nsm_mpls_oam_encode_if_lbl_stack_tlv (u_char **, u_int16_t *,
                                      struct mpls_oam_if_lbl_stk *,
                                      u_char);

int
nsm_mpls_oam_encode_errored_tlv (u_char **, u_int16_t *,
                                 struct mpls_oam_errored_tlv *,
                                 u_char);

int
nsm_mpls_oam_encode_echo_request (u_char **, u_int16_t *,
                                  struct mpls_oam_echo_request *, bool_t);

int
nsm_mpls_oam_encode_echo_reply (u_char **, u_int16_t *,
                                struct mpls_oam_echo_reply *);

#ifdef HAVE_MPLS_OAM_ITUT
int
nsm_mpls_oam_encode_itut_request (u_char **, u_int16_t *, 
                                    struct nsm_mpls_oam_itut *);
int
nsm_mpls_oam_encode_itut_payload (u_char **pnt, u_int16_t *, 
                                  struct nsm_mpls_oam_itut *);

int
nsm_mpls_encode_oam_alert_label (u_char **, u_int16_t *, 
                                  struct oam_label *);
int
nsm_mpls_oam_decode_itut_request (u_char **, u_int16_t *,
                                  struct nsm_mpls_oam_itut *, int *);
int
nsm_mpls_decode_oam_alert_label (u_char **, u_int16_t *, 
                                  struct oam_label *);
int
nsm_mpls_oam_decode_itut_payload(u_char **, u_int16_t *,
                                 struct nsm_mpls_oam_itut * );	


#endif /*HAVE_MPLS_OAM_ITUT*/

#endif /* _PACOS_NSM_MPLS_OAM_PACKET_H */
