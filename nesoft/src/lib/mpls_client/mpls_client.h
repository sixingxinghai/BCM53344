/* Copyright (C) 2001-2003  All Rights Reserved.  */

#ifndef _PACOS_MPLS_LIBLINK_H
#define _PACOS_MPLS_LIBLINK_H

/*
 * The following are the APIs that will be used by the APN protocols.
 */
int apn_mpls_close_all_handles (u_char protocol);
int apn_mpls_if_init           (struct if_ident *if_info,
                                unsigned short label_space);
int apn_mpls_if_end            (struct if_ident *if_info);
int apn_mpls_vrf_init          (int vrf);
int apn_mpls_vrf_end           (int vrf);
#ifdef HAVE_IPV6
int apn_mpls_ilm6_entry_add    (u_char protocol,
                                u_int32_t *label_id_in,
                                u_int32_t *label_id_out,
                                struct if_ident *if_in,
                                struct if_ident *if_out,
                                struct pal_in6_addr *fec_addr,
                                u_char *fec_prefixlen,
                                struct mpls_nh_fwd *nexthop_addr,
                                u_char is_egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                char opcode,
                                u_int32_t *tunnel_label,
                                u_int32_t *qos_resrc_id,
                                u_int32_t nhlfe_ix,
                                struct mpls_owner_fwd *owner,
                                u_int32_t vpn_id,
                                struct pal_in4_addr *vc_peer);
#endif

int apn_mpls_ilm4_entry_add     (u_char protocol,
                                u_int32_t *label_id_in,
                                u_int32_t *label_id_out,
                                struct if_ident *if_in,
                                struct if_ident *if_out,
                                struct pal_in4_addr *fec_addr,
                                u_char *fec_prefixlen,
                                struct pal_in4_addr *nexthop_addr,
                                u_char is_egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                char opcode,
                                u_int32_t *tunnel_label,
                                u_int32_t *qos_resrc_id,
                                u_int32_t nhlfe_ix,
                                struct mpls_owner_fwd *owner,
                                u_int32_t vpn_id,
                                struct pal_in4_addr *vc_peer);
#ifdef HAVE_IPV6
int
apn_mpls_ftn6_entry_add (int vrf,
                        u_char protocol,
                        struct pal_in6_addr *fec_addr,
                        u_char *prefixlen,
                        u_char *dscp_in,
                        u_int32_t *tunnel_label,
                        struct mpls_nh_fwd *tunnel_nhop,
                        struct if_ident *tunnel_if_info,
                        u_int32_t *vpn_label,
                        struct mpls_nh_fwd *vpn_nhop,
                        struct if_ident *vpn_if_info,
                        u_int32_t *tunnel_id,
                        u_int32_t *qos_resrc_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                        struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                        char opcode,
                        u_int32_t nhlfe_ix,
                        u_int32_t ftn_ix,
                        u_char ftn_type,
                        struct mpls_owner_fwd *owner,
                        u_int32_t bypass_ftn_ix,
                        u_char lsp_type,
			int active_head);
#endif
int
apn_mpls_ftn4_entry_add (int vrf,
                        u_char protocol,
                        struct pal_in4_addr *fec_addr,
                        u_char *prefixlen,
                        u_char *dscp_in,
                        u_int32_t *tunnel_label,
                        struct pal_in4_addr *tunnel_nhop,
                        struct if_ident *tunnel_if_info,
                        u_int32_t *vpn_label,
                        struct pal_in4_addr *vpn_nhop,
                        struct if_ident *vpn_if_info,
                        u_int32_t *tunnel_id,
                        u_int32_t *qos_resrc_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                        struct ds_info_fwd *ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                        char opcode,
                        u_int32_t nhlfe_ix,
                        u_int32_t ftn_ix,
                        u_char ftn_type,
                        struct mpls_owner_fwd *owner,
                        u_int32_t bypass_ftn_ix,
                        u_char lsp_type,
			int active_head);

int apn_mpls_ilm_entry_del     (u_char protocol,
                                u_int32_t *label_id_in,
                                struct if_ident *if_info);


#ifdef HAVE_IPV6
int apn_mpls_ftn6_entry_del (int vrf,
                            u_char protocol,
                            u_int32_t *tunnel_id,
                            struct pal_in6_addr *fec_addr,
                            u_char *prefixlen,
                            u_char *dscp_in,
                            u_int32_t nhlfe_ix,
                            struct mpls_nh_fwd *tunnel_nhop,
                            u_int32_t ftn_ix);
#endif

int apn_mpls_ftn4_entry_del (int vrf,
                            u_char protocol,
                            u_int32_t *tunnel_id,
                            struct pal_in4_addr *fec_addr,
                            u_char *prefixlen,
                            u_char *dscp_in,
                            u_int32_t nhlfe_ix,
                            struct pal_in4_addr *tunnel_nhop,
                            u_int32_t ftn_ix);

int apn_mpls_clean_fib_for     (u_char protocol);
int apn_mpls_clean_vrf_for     (u_char protocol);
int apn_mpls_if_update_vrf     (struct if_ident *if_info,
                                int vrf);
int apn_mpls_send_ttl          (u_char protocol,
                                u_char type,
                                int ingress,
                                int new_ttl);
int apn_mpls_local_pkt_handle  (u_char protocol,
                                int enable);
int apn_mpls_debugging_handle  (u_char protocol,
                                u_char msg_type,
                                int enable);

#ifdef HAVE_MPLS_VC
int apn_mpls_vc_init           (u_int32_t vc_id, struct if_ident *if_info,
                                u_int16_t vlan_id);
                                
int apn_mpls_vc_end            (u_int32_t vc_id, struct if_ident *if_info,
                                u_int16_t vlan_id);

int apn_mpls_cw_capability     ();

#ifdef HAVE_IPV6
int apn_mpls_vc6_fib_add (u_int32_t, u_int32_t, u_int32_t *, u_int32_t *,
                          u_int32_t *, u_int32_t *, u_int32_t *,
                          struct if_ident *,  struct if_ident *, u_char,
                          struct pal_in4_addr *, struct mpls_nh_fwd *,
                          struct pal_in6_addr *, u_char *, u_int32_t *,
                          struct mpls_nh_fwd *, u_int32_t *, u_int32_t *, 
                          uint32_t *);
#endif
int apn_mpls_vc4_fib_add (u_int32_t, u_int32_t, u_int32_t *, u_int32_t *,
                         u_int32_t *, u_int32_t *, u_int32_t *,
                         struct if_ident *,  struct if_ident *, u_char,
                         struct pal_in4_addr *, struct pal_in4_addr *,
                         struct pal_in4_addr *, u_char *, u_int32_t *,
                         struct pal_in4_addr *, u_int32_t *, u_int32_t *,
                         u_int32_t *, u_int8_t);
int 
apn_mpls_vc_fib_delete (u_int32_t, u_int32_t, u_int32_t *, u_int32_t *,
                            u_int32_t *, u_int32_t *, u_int32_t *,
                            struct if_ident *, struct if_ident *,
                            struct pal_in4_addr *, u_int8_t);
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
int apn_mpls_vpls_fwd_add (u_int32_t *);

int apn_mpls_vpls_fwd_delete (u_int32_t *);

int apn_mpls_vpls_fwd_if_bind (u_int32_t *, u_int32_t *, u_int16_t);

int apn_mpls_vpls_fwd_if_unbind (u_int32_t *, u_int32_t *, u_int16_t);

int apn_mpls_vpls_mac_withdraw (u_int32_t, u_int16_t, u_char *);
#endif /* HAVE_VPLS */

int apn_mpls_clear_fwd_stats (u_char, u_char);

int apn_mpls_fwd_over_tunnel (u_int32_t, u_int32_t, u_int32_t, u_int32_t,
                              u_int32_t, u_int32_t, u_char *);
int
apn_mpls_oam_send_echo_request (u_int32_t ftn_ix, u_int32_t vrf_id,
                                u_int32_t flags, u_int32_t ttl,
                                u_int32_t pkt_size, u_char *pkt);
int
apn_mpls_oam_vc_send_echo_request (u_int32_t ftn_ix, u_int32_t vrf_id,
                                   u_int32_t flags, u_int32_t ttl,
                                   u_int8_t cc_type, u_int8_t cc_input,
                                   u_int32_t pkt_size, u_char *pkt);

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
int
apn_bfd_send_mpls_pkt (u_int32_t ftn_ix, u_int32_t vrf_id,
                       u_int32_t flags, u_int32_t ttl,
                       u_int32_t pkt_size, u_char *pkt);
#ifdef HAVE_VCCV
int
apn_bfd_send_vc_pkt (u_int32_t if_index, u_int32_t vc_id,
                     u_int32_t flags, u_int32_t ttl,
                     u_int8_t cc_type, u_int8_t cc_input,
                     u_int32_t pkt_size, u_char *pkt);
#endif /* HAVE_VCCV */
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

int apn_mpls_register_oam_callback (OAM_NETLINK_CALLBACK callback);
#ifdef HAVE_TE
int apn_mpls_qos_reserve (struct qos_resource *);
int apn_mpls_qos_release (struct qos_resource *);
#endif /* HAVE_TE */
#endif /* _PACOS_MPLS_LIBLINK_H */
