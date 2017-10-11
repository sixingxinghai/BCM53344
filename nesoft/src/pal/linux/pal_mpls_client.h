/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PAL_MPLS_CLIENT_H
#define _PAL_MPLS_CLIENT_H

#ifdef HAVE_MPLS
#include "mpls_client/mpls_common.h"
#include "pal_mpls_client.def"
#endif /* HAVE_MPLS */
int
pal_apn_mpls_fwd_oam_pkt     (u_int32_t ftn_index, u_int32_t vrf_id, 
                              u_int32_t flags, u_int32_t ttl,
                              u_int32_t pkt_size, u_char *pkt);
int
pal_apn_mpls_fwd_oam_vc_pkt  (u_int32_t if_index, u_int32_t vc_id, 
                              u_int32_t flags, u_int32_t ttl,
                              u_int8_t cc_type, u_int8_t cc_input,
                              u_int32_t pkt_size, u_char *pkt);
int 
send_oam_pkt_over_mpls     (int netlink, u_int32_t ftn_index, u_int32_t vrf_id,
                            u_int32_t flags, u_int32_t ttl, 
                            u_int32_t pkt_size, u_char *pkt);

int 
send_oam_vc_pkt_over_mpls  (int netlink, u_int32_t if_index, u_int32_t vc_id,
                            u_int32_t flags, u_int32_t ttl, u_int8_t cc_type, 
                            u_int8_t cc_input,
                            u_int32_t pkt_size, u_char *pkt);

#ifdef HAVE_IPV6
int
pal_apn_mpls_ilm6_entry_add (u_char protocol,
                             unsigned int *label_id_in,
                             unsigned int *label_id_out,
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
                             u_int32_t nhlfe_ix,
                             struct mpls_owner_fwd *owner);

int
pal_apn_mpls_ftn6_entry_add (int vrf,
                             u_char protocol,
                             unsigned int *label_id_out,
                             struct pal_in6_addr *fec_addr,
                             u_char *prefixlen,
                             u_char *in_dscp,
                             u_int32_t *tunnel_id,
                             u_int32_t *qos_resrc_id,
                             struct mpls_nh_fwd *nexthop_addr,
                             struct if_ident *if_info,
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

int
pal_apn_mpls_vc_ftn6_entry_add  (u_char protocol,
                                 unsigned int *vc_id,
                                 unsigned int *label_id_out,
                                 struct mpls_nh_fwd *nexthop_addr,
                                 struct if_ident *if_in,
                                 struct if_ident *if_out,
                                 char opcode,
                                 u_int32_t tunnel_ftnix);

#endif /* HAVE_IPV6 */
#endif /* _PAL_MPLS_CLIENT_H */
