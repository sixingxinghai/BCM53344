/* Copyright (C) 2009-10  All Rights Reserved. */
#ifndef _PACOS_NSM_MPLS_OAM_H
#define _PACOS_NSM_MPLS_OAM_H

#ifdef HAVE_MPLS_OAM

int
nsm_mpls_oam_process_lsp_ping_req (struct nsm_master *nm,
                                   struct nsm_server_entry *nse,
                                   struct nsm_msg_oam_lsp_ping_req_process *msg);

int
nsm_mpls_oam_send_lsp_ping_req_resp_ldp (struct nsm_master *nm,
                                         struct nsm_server_entry *nse,
                                         struct nsm_msg_oam_lsp_ping_req_process *msg,
                                         struct ftn_entry *ftn);

int
nsm_mpls_oam_send_lsp_ping_req_resp_rsvp (struct nsm_master *nm,
                                          struct nsm_server_entry *nse,
                                          struct nsm_msg_oam_lsp_ping_req_process *msg,
                                          struct ftn_entry *ftn);

int
nsm_mpls_oam_send_lsp_ping_req_resp_gen (struct nsm_master *nm,
                                         struct nsm_server_entry *nse,
                                         struct nsm_msg_oam_lsp_ping_req_process *msg,
                                         struct ftn_entry *ftn);

#ifdef HAVE_MPLS_VC
int
nsm_mpls_oam_send_lsp_ping_req_resp_l2vc (struct nsm_master *nm,
                                          struct nsm_server_entry *nse,
                                          struct nsm_msg_oam_lsp_ping_req_process *msg,
                                          struct nsm_mpls_circuit *vc,
#ifdef HAVE_VPLS
                                          struct nsm_vpls *vpls,
#endif /* HAVE_VPLS */
                                          struct ftn_entry *ftn);
#endif /*  HAVE_MPLS_VC */

#ifdef HAVE_VRF
int
nsm_mpls_oam_send_lsp_ping_req_resp_l3vpn (struct nsm_master *nm,
                                           struct nsm_server_entry *nse,
                                           struct nsm_msg_oam_lsp_ping_req_process *msg,
                                           struct nsm_vrf *n_vrf,
                                           struct ftn_entry *ftn);
#endif /* HAVE_VRF */

void
nsm_mpls_oam_send_lsp_ping_req_resp_err (struct nsm_server_entry *nse,
                                         struct nsm_msg_oam_lsp_ping_req_process *msg,
                                         int err_code);

void
nsm_mpls_oam_process_lsp_ping_rep_ldp (struct nsm_master *nm,
                                       struct nsm_server_entry *nse,
                                       struct nsm_msg_oam_lsp_ping_rep_process_ldp *msg);

void
nsm_mpls_oam_process_lsp_ping_rep_rsvp (struct nsm_master *nm,
                                        struct nsm_server_entry *nse,
                                        struct nsm_msg_oam_lsp_ping_rep_process_rsvp *msg);

void
nsm_mpls_oam_process_lsp_ping_rep_gen (struct nsm_master *nm,
                                       struct nsm_server_entry *nse,
                                       struct nsm_msg_oam_lsp_ping_rep_process_gen *msg);

#ifdef HAVE_MPLS_VC
void
nsm_mpls_oam_process_lsp_ping_rep_l2vc (struct nsm_master *nm,
                                        struct nsm_server_entry *nse,
                                        struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg);
#endif /* HAVE_MPLS_VC */

void
nsm_mpls_oam_process_lsp_ping_rep_l3vpn (struct nsm_master *nm,
                                         struct nsm_server_entry *nse,
                                         struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg);

void
nsm_mpls_oam_send_lsp_ping_rep_resp (struct nsm_server_entry *nse,
                                     struct nsm_msg_oam_lsp_ping_rep_resp *msg);

void
nsm_mpls_oam_send_ftn_update (struct nsm_master *nm, struct ftn_entry *ftn);

#ifdef HAVE_MPLS_VC
void
nsm_mpls_oam_send_vc_update (struct nsm_master *nm, struct nsm_mpls_circuit *vc);
#endif /* HAVE_MPLS_VC */

int
nsm_mpls_oam_route_ipv4_lookup (struct nsm_master *nm, vrf_id_t vrf_id,
                                struct pal_in4_addr *ipv4_addr,
                                u_char prefixlen);

int
nsm_mpls_oam_transit_stack_lookup (struct nsm_master *,
                                   struct mpls_oam_ctrl_data *,
                                   struct mpls_label_stack *,
                                   struct prefix *, u_char );

void
nsm_mpls_oam_resp_ldp_ipv4_data_set (struct nsm_msg_oam_lsp_ping_req_process *, 
                                     struct prefix *, struct ldp_ipv4_prefix *);

void
nsm_mpls_oam_resp_rsvp_ipv4_data_set (struct ftn_entry *,
                                      struct rsvp_ipv4_prefix *);

#ifdef HAVE_MPLS_VC
int
nsm_mpls_oam_resp_l2_data_set (struct nsm_msg_oam_lsp_ping_req_process *, 
                               struct nsm_mpls_circuit *,
                               struct ftn_entry *,
                               struct nsm_msg_oam_lsp_ping_req_resp_l2vc *);

#ifdef HAVE_VPLS
int
nsm_mpls_oam_resp_vpls_data_set (struct nsm_msg_oam_lsp_ping_req_process *,
                                 struct nsm_vpls *,
                                 struct ftn_entry *,
                                 struct nsm_msg_oam_lsp_ping_req_resp_l2vc *);
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
int
nsm_mpls_oam_resp_l3vpn_data_set (struct nsm_master *,
                                  struct nsm_msg_oam_lsp_ping_req_process *,
                                  struct nsm_vrf *,
                                  struct ftn_entry *,
                                  struct nsm_msg_oam_lsp_ping_req_resp_l3vpn *);
#endif /* HAVE_VRF */

void
nsm_mpls_oam_resp_gen_ipv4_data_set (struct nsm_msg_oam_lsp_ping_req_process *,
                                     struct prefix *,
                                     struct generic_ipv4_prefix *);
int
nsm_mpls_oam_resp_dsmap_data_set (struct nsm_master *, struct ftn_entry *,
                                  struct mpls_oam_downstream_map *);

void
nsm_mpls_oam_process_ldp_ipv4_data (struct nsm_master *nm,
                                    struct ldp_ipv4_prefix *ldp_data,
                                    struct mpls_oam_ctrl_data *ctrl_data,
                                    struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                    bool_t is_dsmap,
                                    u_char lookup);

void
nsm_mpls_oam_process_rsvp_ipv4_data (struct nsm_master *nm,
                                     struct rsvp_ipv4_prefix *rsvp_data,
                                     struct mpls_oam_ctrl_data *ctrl_data,
                                     struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                     bool_t is_dsmap,
                                     u_char lookup);

void
nsm_mpls_oam_process_gen_ipv4_data (struct nsm_master *nm,
                                    struct generic_ipv4_prefix *gen_data,
                                    struct mpls_oam_ctrl_data *ctrl_data,
                                    struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                    bool_t is_dsmap,
                                    u_char lookup);

#ifdef HAVE_MPLS_VC
void
nsm_mpls_oam_process_l2vc_data (struct nsm_master *nm,
                                struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg,
                                struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                bool_t is_dsmap);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VRF
void
nsm_mpls_oam_process_l3vpn_data (struct nsm_master *nm,
                                 struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg,
                                 struct nsm_msg_oam_lsp_ping_rep_resp *oam_resp,
                                 bool_t is_dsmap);
#endif /* HAVE_VRF */

int
nsm_mpls_oam_reply_dsmap_data_set (struct mpls_oam_downstream_map *dsmap,
                                   struct mpls_label_stack *out_label,
                                   struct interface *ifp);
#endif /* HAVE_MPLS_OAM */
#endif /* _PACOS_NSM_MPLS_OAM_H */
