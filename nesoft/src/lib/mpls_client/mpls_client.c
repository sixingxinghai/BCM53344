/* Copyright (C) 2001-2005  All Rights Reserved.  */

#include "pal.h"
#include "pal_types.h"

#include "mpls_common.h"
#include "pal_mpls_client.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifdef HAVE_TE
#include "qos_common.h"
#endif /* HAVE_TE */

/*
 * These APIs will be executed by the APN protocols in order to
 * interact with the MPLS forwarder.
 */

/* 
 * Initialize the layer-2 connection.
 */
int
apn_mpls_init_all_handles (struct lib_globals *zg, u_char protocol)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_init_all_handles (zg, protocol);
#elif defined (HAVE_HAL)
  return hal_mpls_init (protocol);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Close the layer-2 connection.
 */
int
apn_mpls_close_all_handles (u_char protocol)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_close_all_handles (protocol);
#elif defined (HAVE_HAL)
  return hal_mpls_deinit (protocol);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Create a VRF table in the MPLS Forwarder.
 */
int
apn_mpls_vrf_init (int vrf_ident)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_vrf_init (vrf_ident);
#elif defined (HAVE_HAL)
  return hal_mpls_vrf_create (vrf_ident);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}
  
/*
 * Delete a VRF table from the MPLS Forwarder.
 */
int
apn_mpls_vrf_end (int vrf_ident)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_vrf_end (vrf_ident);
#elif defined (HAVE_HAL)
  return hal_mpls_vrf_destroy (vrf_ident);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Enable an interface for MPLS.
 */
int
apn_mpls_if_init (struct if_ident *if_info,
                  unsigned short label_space)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_if_init (if_info, label_space);
#elif defined (HAVE_HAL)
  return hal_mpls_enable_interface (if_info, label_space);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Update the VRF that an MPLS interface points to.
 */
int
apn_mpls_if_update_vrf (struct if_ident *if_info,
                        int vrf_ident)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_if_update_vrf (if_info, vrf_ident);
#elif defined (HAVE_HAL)
  return hal_mpls_if_update_vrf (if_info, vrf_ident);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Disable MPLS on an interface.
 */
int
apn_mpls_if_end (struct if_ident *if_info)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_if_end (if_info);
#elif defined (HAVE_HAL)
  return hal_mpls_disable_interface (if_info);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Add an ILM entry to the ILM table.
 */
#ifdef HAVE_IPV6
int apn_mpls_ilm6_entry_add (u_char protocol,
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
                             u_int32_t *qos_resource_id,
                             u_int32_t nhlfe_ix,
                             struct mpls_owner_fwd *owner,
                             u_int32_t vpn_id,
                             struct pal_in6_addr *vc_peer)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_ilm6_entry_add (protocol, 
                                      label_id_in,
                                      label_id_out,
                                      if_in,
                                      if_out,
                                      fec_addr,
                                      fec_prefixlen,
                                      nexthop_addr,
                                      is_egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                      ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                      opcode,
                                      nhlfe_ix,
                                      owner);
#elif defined (HAVE_HAL)
  return hal_mpls_ilm6_entry_add (label_id_in,
                                  if_in,
                                  opcode,
                                  (struct hal_in6_addr *)nexthop_addr,
                                  if_out,
                                  label_id_out,
                                  nhlfe_ix,
                                  is_egress,
                                  tunnel_label,
                                  qos_resource_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                  (struct hal_mpls_diffserv *)ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

                                  (struct hal_in6_addr *)fec_addr,
                                  fec_prefixlen,
                                  vpn_id, 
                                  (struct hal_in6_addr *)vc_peer);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}
#endif

int apn_mpls_ilm4_entry_add (u_char protocol,
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
                             u_int32_t *qos_resource_id,
                             u_int32_t nhlfe_ix,
                             struct mpls_owner_fwd *owner,
                             u_int32_t vpn_id,
                             struct pal_in4_addr *vc_peer)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_ilm_entry_add (protocol, 
                                      label_id_in,
                                      label_id_out,
                                      if_in,
                                      if_out,
                                      fec_addr,
                                      fec_prefixlen,
                                      nexthop_addr,
                                      is_egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                      ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                      opcode,
                                      nhlfe_ix,
                                      owner);
#elif defined (HAVE_HAL)
  return hal_mpls_ilm_entry_add (label_id_in,
                                 if_in,
                                 opcode,
                                 (struct hal_in4_addr *)nexthop_addr,
                                 if_out,
                                 label_id_out,
                                 nhlfe_ix,
                                 is_egress,
                                 tunnel_label,
                                 qos_resource_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                 (struct hal_mpls_diffserv *)ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                 (struct hal_in4_addr *)fec_addr,
                                 fec_prefixlen,
                                 vpn_id, 
                                 (struct hal_in4_addr *)vc_peer);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Add an FTN entry to the FTN table.
 */
#ifdef HAVE_IPV6
int
apn_mpls_ftn6_entry_add (int vrf,
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
			int active_head)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_ftn6_entry_add (vrf,
                                     protocol,
                                     vpn_label ? vpn_label : tunnel_label,
                                     (struct pal_in6_addr *) fec_addr,
                                     prefixlen,
                                     dscp_in,
                                     tunnel_id,
                                     qos_resrc_id,
                                     (struct mpls_nh_fwd *)( vpn_nhop ? vpn_nhop : tunnel_nhop),
                                     vpn_if_info ? vpn_if_info : tunnel_if_info,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                     ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

                                     opcode,
                                     nhlfe_ix,
                                     ftn_ix,
                                     ftn_type,
                                     owner,
                                     bypass_ftn_ix,
                                     lsp_type,
				     active_head);
#elif defined (HAVE_HAL)
  return hal_mpls_ftn_entry_add     (vrf,
                                     protocol,
                                     (struct hal_in4_addr *)fec_addr,
                                     prefixlen,
                                     dscp_in,
                                     tunnel_label,
                                     (struct hal_in4_addr *)tunnel_nhop,
                                     tunnel_if_info,
                                     vpn_label,
                                     (struct hal_in4_addr *)vpn_nhop,
                                     vpn_if_info,
                                     tunnel_id,
                                     qos_resrc_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                     (struct hal_mpls_diffserv *)ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                     opcode,
                                     nhlfe_ix,
                                     ftn_ix,
                                     ftn_type,
                                     owner,
                                     bypass_ftn_ix,
                                     lsp_type);
#endif /* HAVE_MPLS_FWD */
  return 0;
}
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
			int active_head)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_ftn_entry_add (vrf,
                                     protocol,
                                     vpn_label ? vpn_label : tunnel_label,
                                     fec_addr,
                                     prefixlen,
                                     dscp_in,
                                     tunnel_id,
                                     qos_resrc_id,
                                     vpn_nhop ? vpn_nhop : tunnel_nhop,
                                     vpn_if_info ? vpn_if_info : tunnel_if_info,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                     ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                     opcode,
                                     nhlfe_ix,
                                     ftn_ix,
                                     ftn_type,
                                     owner,
                                     bypass_ftn_ix,
                                     lsp_type,
				     active_head);
#elif defined (HAVE_HAL)
  return hal_mpls_ftn_entry_add     (vrf,
                                     protocol,
                                     (struct hal_in4_addr *)fec_addr,
                                     prefixlen,
                                     dscp_in,
                                     tunnel_label,
                                     (struct hal_in4_addr *)tunnel_nhop,
                                     tunnel_if_info,
                                     vpn_label,
                                     (struct hal_in4_addr *)vpn_nhop,
                                     vpn_if_info,
                                     tunnel_id,
                                     qos_resrc_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                     (struct hal_mpls_diffserv *)ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                     opcode,
                                     nhlfe_ix,
                                     ftn_ix,
                                     ftn_type,
                                     owner,
                                     bypass_ftn_ix,
                                     lsp_type);
#endif /* HAVE_MPLS_FWD */
  return 0;
}

/*
 * Delete an ILM entry to the ILM table.
 *
 * Once the incoming label goes away, the entry becomes an
 * FTN entry.
 */
int
apn_mpls_ilm_entry_del (u_char protocol,
                        u_int32_t *label_id_in,
                        struct if_ident *if_info)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_ilm_entry_del (protocol,
                                     label_id_in,
                                     if_info);
#elif defined (HAVE_HAL)
  return hal_mpls_ilm_entry_delete  (protocol,
                                     label_id_in,
                                     if_info);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Delete an FTN entry to the FTN table.
 */
#ifdef HAVE_IPV6
int
apn_mpls_ftn6_entry_del (int vrf,
                        u_char protocol,
                        u_int32_t *tunnel_id,
                        struct pal_in4_addr *fec_addr,
                        u_char *prefixlen,
                        u_char *dscp_in,
                        u_int32_t nhlfe_ix,
                        struct pal_in4_addr *tunnel_nhop,
                        u_int32_t ftn_ix)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_ftn_entry_del (vrf,
                                     protocol,
                                     tunnel_id,
                                     fec_addr,
                                     prefixlen,
                                     ftn_ix);
#elif defined (HAVE_HAL)
  return hal_mpls_ftn_entry_delete  (vrf,
                                     protocol,
                                     (struct hal_in4_addr *)fec_addr,
                                     prefixlen,
                                     dscp_in,
                                     (struct hal_in4_addr *)tunnel_nhop,
                                     nhlfe_ix,
                                     tunnel_id,
                                     ftn_ix);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}
#endif

int
apn_mpls_ftn4_entry_del (int vrf,
                        u_char protocol,
                        u_int32_t *tunnel_id,
                        struct pal_in4_addr *fec_addr,
                        u_char *prefixlen,
                        u_char *dscp_in,
                        u_int32_t nhlfe_ix,
                        struct pal_in4_addr *tunnel_nhop,
                        u_int32_t ftn_ix)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_ftn_entry_del (vrf,
                                     protocol,
                                     tunnel_id,
                                     fec_addr,
                                     prefixlen,
                                     ftn_ix);
#elif defined (HAVE_HAL)
  return hal_mpls_ftn_entry_delete  (vrf,
                                     protocol,
                                     (struct hal_in4_addr *)fec_addr,
                                     prefixlen,
                                     dscp_in,
                                     (struct hal_in4_addr *)tunnel_nhop,
                                     nhlfe_ix,
                                     tunnel_id,
                                     ftn_ix);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Clean the MPLS FIB of entries populated by the specified
 * protocol.
 */
int
apn_mpls_clean_fib_for (u_char protocol)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_clean_fib_for (protocol);
#elif defined (HAVE_HAL)
  return hal_mpls_clear_fib_table (protocol);
#endif /* HAVE_MPLS_FWD */
  return 0;
}

/*
 * Clean the MPLS FIB of entries populated by the specified
 * protocol.
 */
int
apn_mpls_clean_vrf_for (u_char protocol)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_clean_vrf_for (protocol);
#elif defined (HAVE_HAL)
  return hal_mpls_clear_vrf_table (protocol);
#endif /* HAVE_MPLS_FWD */
  return 0;
}

/*
 * Set the TTL value for ingress/egress for all LSPs.
 *
 * TTL value of -1 means that we wish to use the default handling of TTL.
 */
int
apn_mpls_send_ttl (u_char protocol, u_char type, int ingress, int new_ttl)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_send_ttl (protocol, type, ingress, new_ttl);
#elif defined (HAVE_HAL)
  return hal_mpls_send_ttl (protocol, type, ingress, new_ttl);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Enable/Disable the mapping of locally generated TCP packets.
 */
int
apn_mpls_local_pkt_handle  (u_char protocol, int enable)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_local_pkt_handle  (protocol, enable);
#elif defined (HAVE_HAL)
  return hal_mpls_local_pkt_handle  (protocol, enable);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Enable/Disable debugging.
 */
int
apn_mpls_debugging_handle  (u_char protocol, u_char msg_type,
                            int enable)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_debugging_handle  (protocol, msg_type, enable);
#endif /* HAVE_MPLS_FWD */
  return 0;
}

#ifdef HAVE_MPLS_VC

/*Check if the Hardware supports the use of Control word */
int
apn_mpls_cw_capability ()
{
#ifdef HAVE_HAL
  return hal_mpls_cw_capability ();
#endif /* HAVE_HAL */
  return 0;
}

/*
 * Bind an interface to a Virtual Circuit.
 */
int
apn_mpls_vc_init (u_int32_t vc_id, struct if_ident *if_info, 
                  u_int16_t vlan_id)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_vc_init (if_info, vc_id);
#elif defined (HAVE_HAL)
  return hal_mpls_vc_init (vc_id, if_info, vlan_id);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Unbind an interface form a Virtual Circuit.
 */
int
apn_mpls_vc_end (u_int32_t vc_id, struct if_ident *if_info, 
                 u_int16_t vlan_id)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_vc_end (if_info);
#elif defined (HAVE_HAL)
  return hal_mpls_vc_deinit (vc_id, if_info, vlan_id);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

/*
 * Send a Generic VC FIB add. VPLS is not currently supported on the MPLS Software
 * Forwarder.
 */
#ifdef HAVE_IPV6
int
apn_mpls_vc6_fib_add (u_int32_t vc_id,
                      u_int32_t vc_style,
                      u_int32_t *vpls_id,
                      u_int32_t *in_label,
                      u_int32_t *out_label,
                      u_int32_t *ac_if_ix,
                      u_int32_t *nw_if_ix,                
                      struct if_ident *if_in,
                      struct if_ident *if_out,
                      u_char opcode,
                      struct pal_in4_addr *peer_addr,
                      struct pal_in4_addr *peer_nhop_addr,
                      struct pal_in4_addr *fec_addr,
                      u_char *fec_prefixlen,
                      u_int32_t *tunnel_label,
                      struct pal_in4_addr *tunnel_nhop,
                      u_int32_t *tunnel_oix,
                      u_int32_t *tunnel_nhlfe_ix,
                      u_int32_t *tunnel_ftnix)
{
  int ret = 0;
#ifdef HAVE_MPLS_FWD
  struct mpls_owner_fwd fwd;
  struct pal_in6_addr ilm_nhop;

  /* We will make two PAL calls - One for ILM and one for FTN 
   * If a VPLS Id exists return a Success since VPLS is not supported on 
   * Linux
   */
  if (vpls_id)
    return 0;
 
  pal_mem_set (&ilm_nhop, 0, sizeof (struct pal_in6_addr));

  pal_mem_set (&fwd, 0, sizeof (struct mpls_owner_fwd));
  ret = pal_apn_mpls_vc_ftn6_entry_add  (APN_PROTO_NSM,
                                         &vc_id,
                                         out_label,
                                         ( struct mpls_nh_fwd *) peer_addr,
                                         if_in,
                                         if_out,
                                         opcode,
                                         tunnel_ftnix != NULL
                                         ? *tunnel_ftnix : 0);
  if (ret < 0)
    return ret;

  ret = pal_apn_mpls_ilm6_entry_add (APN_PROTO_NSM,
                                    in_label,
                                    0,
                                    if_out,
                                    if_in,
                                    (struct pal_in6_addr *)fec_addr,
                                    fec_prefixlen,
                                    (struct mpls_nh_fwd *) &ilm_nhop,
                                    PAL_TRUE,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                    NULL,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                    POP_FOR_VC,
                                    0,
                                    &fwd);

  if (ret < 0)
    return ret;

#elif defined (HAVE_HAL)
  return hal_mpls_vc_fib_add (vc_id,
                              vc_style,
                              vpls_id ? *vpls_id : vc_id,
                              *in_label,
                              *out_label,
                              *ac_if_ix,
                              *nw_if_ix,
                              opcode,
                              peer_addr, 
                              peer_nhop_addr,
                              tunnel_label != NULL
                              ? *tunnel_label : LABEL_VALUE_INVALID,
                              tunnel_nhop,
                              tunnel_oix != NULL
                              ? *tunnel_oix : 0,
                              tunnel_nhlfe_ix != NULL
                              ? *tunnel_nhlfe_ix : 0);
#endif /* HAVE_HAL */
  return ret;
}
#endif

int
apn_mpls_vc4_fib_add (u_int32_t vc_id,
                      u_int32_t vc_style,
                      u_int32_t *vpls_id,
                      u_int32_t *in_label,
                      u_int32_t *out_label,
                      u_int32_t *ac_if_ix,
                      u_int32_t *nw_if_ix,                
                      struct if_ident *if_in,
                      struct if_ident *if_out,
                      u_char opcode,
                      struct pal_in4_addr *peer_addr,
                      struct pal_in4_addr *peer_nhop_addr,
                      struct pal_in4_addr *fec_addr,
                      u_char *fec_prefixlen,
                      u_int32_t *tunnel_label,
                      struct pal_in4_addr *tunnel_nhop,
                      u_int32_t *tunnel_oix,
                      u_int32_t *tunnel_nhlfe_ix,
                      u_int32_t *tunnel_ftnix,
                      u_int8_t is_ms_pw)
{
  int ret = 0;
#ifdef HAVE_MPLS_FWD
  struct mpls_owner_fwd fwd;
  struct pal_in4_addr ilm_nhop;

  /* We will make two PAL calls - One for ILM and one for FTN 
   * If a VPLS Id exists return a Success since VPLS is not supported on 
   * Linux
   */
  if (vpls_id)
    return 0;
 
  pal_mem_set (&ilm_nhop, 0, sizeof (struct pal_in4_addr));

  pal_mem_set (&fwd, 0, sizeof (struct mpls_owner_fwd));

  /* A FTN entry needs to be added only when the VC is not a part of a
   * Multi-segment PW.
   */
  if (!is_ms_pw)
    {
      ret = pal_apn_mpls_vc_ftn_entry_add  (APN_PROTO_NSM,
                                            &vc_id,
                                            out_label,
                                            *tunnel_label == LABEL_IMPLICIT_NULL
                                            ? peer_nhop_addr : peer_addr,
                                            if_in,
                                            if_out,
                                            opcode,
                                            tunnel_ftnix != NULL
                                            ? *tunnel_ftnix : 0);
      if (ret < 0)
        return ret;
    }

 if (is_ms_pw)
    {      
      ret = pal_apn_mpls_ilm_entry_add (APN_PROTO_NSM, 
                                        in_label,
                                        out_label,
                                        if_in,
                                        if_out,
                                        fec_addr,
                                        fec_prefixlen, 
                                        &ilm_nhop,
                                        PAL_TRUE,
#ifdef HAVE_DIFFSERV                                        
                                        NULL, 
#endif /* HAVE_DIFFSERV */
                                        SWAP_AND_LOOKUP,
                                        0,
                                        &fwd);

      if (ret < 0)
        return ret;
    }
  else
    {
      ret = pal_apn_mpls_ilm_entry_add (APN_PROTO_NSM,
                                        in_label,
                                        0,
                                        if_out,
                                        if_in,
                                        fec_addr,
                                        fec_prefixlen,
                                        &ilm_nhop,
                                        PAL_TRUE,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                        NULL,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                        POP_FOR_VC,
                                        0,
                                        &fwd);

      if (ret < 0)
        return ret;
    }
#elif defined (HAVE_HAL)
  return hal_mpls_vc_fib_add (vc_id,
                              vc_style,
                              vpls_id ? *vpls_id : vc_id,
                              *in_label,
                              *out_label,
                              *ac_if_ix,
                              *nw_if_ix,
                              opcode,
                              peer_addr, 
                              peer_nhop_addr,
                              tunnel_label != NULL
                              ? *tunnel_label : LABEL_VALUE_INVALID,
                              tunnel_nhop,
                              tunnel_oix != NULL
                              ? *tunnel_oix : 0,
                              tunnel_nhlfe_ix != NULL
                              ? *tunnel_nhlfe_ix : 0);
#endif /* HAVE_HAL */
  return ret;
}

/*
 * Send a Generic VC FIB delete.
 */
int
apn_mpls_vc_fib_delete (u_int32_t vc_id,
                        u_int32_t vc_style,
                        u_int32_t *vpls_id,
                        u_int32_t *in_label,
                        u_int32_t *out_label,
                        u_int32_t *ac_if_ix,
                        u_int32_t *nw_if_ix,
                        struct if_ident *if_in,
                        struct if_ident *if_out,
                        struct pal_in4_addr *peer_addr,
                        u_int8_t is_ms_pw)
{
  int ret=0;
  /* Check for the VPLS id and if yes proceed with standard VPLS operations
   * Martini Circuits should not have the VPLS ID field
   */
#ifdef HAVE_MPLS_FWD

  if (vpls_id)
    return 0;

  if (!is_ms_pw)
    ret = pal_apn_mpls_vc_ftn_entry_del (APN_PROTO_NSM,
                                         if_in,
                                         &vc_id);
  if (ret < 0)
    return ret;
 
  ret = pal_apn_mpls_ilm_entry_del (APN_PROTO_NSM,
                                    in_label,
                                    if_out);

  if (ret < 0)
    return ret;
#elif defined (HAVE_HAL)
  return hal_mpls_vc_fib_delete (vc_id,
                                 vc_style,
                                 vpls_id ? *vpls_id : vc_id,
                                 *in_label,
                                 *nw_if_ix,
                                 (struct hal_in4_addr *)peer_addr);
#endif /* HAVE_HAL */
   return ret;
}

#ifdef HAVE_VPLS
/*
 * Send a vpls fwd add.
 */
int
apn_mpls_vpls_fwd_add (u_int32_t *vpls_id)
{
#ifdef HAVE_HAL
  return hal_mpls_vpls_add (*vpls_id);
#endif /* HAVE_HAL */
  
  return 0;
}

/*
 * Send a vpls fwd delete.
 */
int
apn_mpls_vpls_fwd_delete (u_int32_t *vpls_id)
{
#ifdef HAVE_HAL 
  return hal_mpls_vpls_del (*vpls_id);
#endif /* HAVE_HAL */

  return 0;
}

/*
 * Send a vpls fwd if bind.
 */
int
apn_mpls_vpls_fwd_if_bind (u_int32_t *vpls_id,
                           u_int32_t *if_ix,
                           u_int16_t vlan_id)
{
#ifdef HAVE_HAL
  return hal_mpls_vpls_if_bind (*vpls_id, *if_ix, vlan_id);
#endif /* HAVE_HAL */
  return 0;
}

/*
 * Send a vpls fwd if unbind.
 */
int
apn_mpls_vpls_fwd_if_unbind (u_int32_t *vpls_id,
                             u_int32_t *if_ix, 
                             u_int16_t vlan_id)
{
#ifdef HAVE_HAL
  return hal_mpls_vpls_if_unbind (*vpls_id, *if_ix, 0);
#endif /* HAVE_HAL */
  return 0;
}

/*
 * Send a vpls mac withdraw.
 */
int
apn_mpls_vpls_mac_withdraw (u_int32_t vpls_id,
                            u_int16_t num,
                            u_char *mac_addrs)
{
    /* if num == 0, withdraw all MAC Addresses for this VPLS */
#ifdef HAVE_HAL
    return hal_mpls_vpls_mac_withdraw (vpls_id, num, mac_addrs);
#else
    return 0;
#endif /* HAVE_HAL */
}

#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */

/* Clear out all MPLS specific forwarding stats. */
int
apn_mpls_clear_fwd_stats (u_char protocol, u_char type)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_clear_fwd_stats (protocol, type);
#endif /* HAVE_MPLS_FWD */
  return 0;
}

/* For ftn index the value of ftn_idx in software is ent*/
int
apn_mpls_fwd_over_tunnel (u_int32_t entry_index, u_int32_t cpkt_id, 
                          u_int32_t flags, u_int32_t total_len,
                          u_int32_t seq_num, u_int32_t pkt_size, u_char *pkt)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_fwd_over_tunnel  (entry_index, cpkt_id, flags, total_len,
                                        seq_num, pkt_size, pkt);
#elif defined (HAVE_HAL)
  /* HAL call goes in here */
  return 0;
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

#ifdef HAVE_TE
/*
 * Send qos reserve to forwarder 
 */
int
apn_mpls_qos_reserve (struct qos_resource *resource)
{
#ifdef HAVE_HAL
  struct hal_mpls_qos qos;

  pal_mem_set (&qos, 0, sizeof (struct hal_mpls_qos));

  qos.resource_id = resource->resource_id;
  qos.ct_num = resource->ct_num;
  qos.setup_priority = resource->setup_priority;
  qos.hold_priority = resource->hold_priority;
  qos.service_type = resource->t_spec.service_type;
  qos.min_policed_unit = resource->t_spec.mpu;
  qos.max_packet_size = resource->t_spec.max_packet_size;
  qos.peak_rate = resource->t_spec.peak_rate;
  qos.rate = resource->t_spec.committed_bucket.rate;
  qos.size = resource->t_spec.committed_bucket.size;
  qos.excess_burst = resource->t_spec.excess_burst;
  qos.weight = resource->t_spec.weight;
  qos.out_ifindex = resource->if_spec.out_ifindex;

  return hal_mpls_qos_reserve (&qos);
#endif /* HAVE_HAL */

  return 0;
}

/*
 * Send qos release to forwarder 
 */
int
apn_mpls_qos_release (struct qos_resource *resource)
{
#ifdef HAVE_HAL
  struct hal_mpls_qos qos;

  pal_mem_set (&qos, 0, sizeof (struct hal_mpls_qos));
  qos.resource_id = resource->resource_id;
  qos.ct_num = resource->ct_num;
  qos.setup_priority = resource->setup_priority;
  qos.hold_priority = resource->hold_priority;
  qos.service_type = resource->t_spec.service_type;
  qos.min_policed_unit = resource->t_spec.mpu;
  qos.max_packet_size = resource->t_spec.max_packet_size;
  qos.peak_rate = resource->t_spec.peak_rate;
  qos.rate = resource->t_spec.committed_bucket.rate;
  qos.size = resource->t_spec.committed_bucket.size;
  qos.excess_burst = resource->t_spec.excess_burst;
  qos.weight = resource->t_spec.weight;
  qos.out_ifindex = resource->if_spec.out_ifindex;

  return hal_mpls_qos_release (&qos);
#endif /* HAVE_HAL */

  return 0;
}
#endif /* HAVE_TE */

#if defined HAVE_MPLS_OAM || defined HAVE_MPLS_OAM_ITUT || defined HAVE_NSM_MPLS_OAM
/* MPLS OAM Packet forwarding API */ 
int
apn_mpls_oam_send_echo_request (u_int32_t ftn_ix, u_int32_t vrf_id,
                                u_int32_t flags, u_int32_t ttl,
                                u_int32_t pkt_size, u_char *pkt)
{
#ifdef HAVE_MPLS_FWD
 
  return pal_apn_mpls_fwd_oam_pkt (ftn_ix, vrf_id, flags,
                                   ttl , pkt_size, pkt);
#elif defined (HAVE_HAL)
  /* HAL call goes here */
  return 0;
#else 
  return 0;
#endif /* HAVE_MPLS_FWD */
}

#ifdef HAVE_MPLS_VC
/* MPLS OAM VC Packet forwarding API */ 
int
apn_mpls_oam_vc_send_echo_request (u_int32_t if_index, u_int32_t vc_id, 
                                   u_int32_t flags, u_int32_t ttl,
                                   u_int8_t cc_type, u_int8_t cc_input,
                                   u_int32_t pkt_size, u_char *pkt)
{
#ifdef HAVE_MPLS_FWD
  return pal_apn_mpls_fwd_oam_vc_pkt (if_index, vc_id, flags, ttl, 
                                      cc_type, cc_input,
                                      pkt_size, pkt); 
#elif defined (HAVE_HAL)
  /* HAL call goes here */
  return 0;
#else 
  return 0;
#endif /* HAVE_MPLS_FWD */
}
#endif /* HAVE_MPLS_VC */

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
/**@brief - API function to forward the BFD MPLS packet to the Forwarder.

 *  @param ftn_ix - FTN Index. 
 *  @param vrf_id - VRF Id. 
 *  @param flags - Flags specifying Nophp or explicit Null label. 
 *  @param ttl - TTL value. 
 *  @param pkt_size - Length of the packet. 
 *  @param *pkt - Packet buffer. 

 *  @return Success (0) or Failuire (-1) 
 */
int
apn_bfd_send_mpls_pkt (u_int32_t ftn_ix, u_int32_t vrf_id,
                       u_int32_t flags, u_int32_t ttl,
                       u_int32_t pkt_size, u_char *pkt)
{
#ifdef HAVE_MPLS_FWD
 
  return pal_apn_mpls_fwd_oam_pkt (ftn_ix, vrf_id, flags,
                                   ttl , pkt_size, pkt);
#elif defined (HAVE_HAL)
  /* HAL call goes here */
  return 0;
#else 
  return 0;
#endif /* HAVE_MPLS_FWD */
}

#ifdef HAVE_VCCV
/* BFD VC Packet forwarding API */ 
int
apn_bfd_send_vc_pkt (u_int32_t if_index, u_int32_t vc_id, 
                     u_int32_t flags, u_int32_t ttl,
                     u_int8_t cc_type, u_int8_t cc_input,
                     u_int32_t pkt_size, u_char *pkt)
{
#ifdef HAVE_MPLS_FWD
 
  return pal_apn_mpls_fwd_oam_vc_pkt (if_index, vc_id, flags,
                                      ttl , cc_type, cc_input, pkt_size, pkt); 
#elif defined (HAVE_HAL)
  /* HAL call goes here */
  return 0;
#else 
  return 0;
#endif /* HAVE_MPLS_FWD */
}
#endif /* HAVE_VCCV */

#endif /* HAVE_BFD && HAVE_MPLS_OAM */

int 
apn_mpls_register_oam_callback (OAM_NETLINK_CALLBACK callback)
{
#ifdef HAVE_MPLS_FWD
  return pal_register_mpls_oam_netlink_callback (callback);
#else
  return 0;
#endif /* HAVE_MPLS_FWD */
}

#endif  /* HAVE_MPLS_OAM || HAVE_MPLS_OAM_ITUT || HAVE_NSM_MPLS_OAM */
