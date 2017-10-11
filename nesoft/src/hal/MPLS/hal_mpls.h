/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_MPLS_H_
#define _HAL_MPLS_H_

#include "mpls_client/mpls_common.h"
#include "hal_incl.h"
#include "hal_mpls_types.h"

struct nsm_mpls_pw_snmp_perf_curr;

/* 
   Name: hal_mpls_init

   Description:
   This API initializes MPLS forwarding plane.

   Parameters:
   IN -> protocol

   Returns:
   HAL_MPLS_INIT_ERR
   HAL_SUCCESS
*/
int
hal_mpls_init (u_char protocol);

/* 
   Name: hal_mpls_deinit 

   Description:
   This API deinitializes MPLS forwarding plane.

   Parameters:
   IN -> protocol

   Returns:
   HAL_MPLS_DEINIT_ERR
   HAL_SUCCESS
*/
int
hal_mpls_deinit (u_char protocol);

/*
  Name: hal_mpls_vrf_create
  
  Description:
  This API creates a VRF table.

  Parameters:
  IN -> vrf : VRF table id.

  Returns:
  HAL_MPLS_VRF_EXISTS
  HAL_SUCCESS
*/
int
hal_mpls_vrf_create (int vrf);

/*
  Name: hal_mpls_vrf_destroy

  Description:
  This API deletes a VRF table.

  Parameters:
  IN -> vrf : VRF table id.

  Returns:
  HAL_MPLS_VRF_NOT_EXISTS
  HAL_SUCCESS
*/
int
hal_mpls_vrf_destroy (int vrf);

/*
  Name: hal_mpls_enable_interface

  Description:
  This API enables a IP interface for MPLS forwarding. If a interface is
  already MPLS enabled, then this API can be used to change the association
  of the label space. The new label space is bound to this interface.
  A new ILM table is created for a new label space identifier. 

  Parameters:
  IN -> if_ident : Interface identifier
  IN -> label_space: Label space identifier

  Returns:
  HAL_MPLS_INTERFACE_ERR
  HAL_SUCCESS
*/
int
hal_mpls_enable_interface (struct if_ident *if_ident,
                           unsigned short label_space);


/*
  Name: hal_mpls_disable_interface

  Description:
  This API disables interface for MPLS forwarding. If the reference count
  of the ILM table that this interface is bound to, becomes 0, then the
  ILM table is deleted.

  Parameters:
  IN -> if_ident : Interface identifier

  Returns:
  HAL_MPLS_INTERFACE_ERR
  HAL_SUCCESS
*/
int
hal_mpls_disable_interface (struct if_ident *if_ident);

/*
  Name: hal_mpls_if_update_vrf

  Description:
  This API updates the VRF that the MPLS interface points to.

  Parameters:
  IN -> if_ident : Interface identifier
  IN -> vrf : VRF table identifier

  Returns:
  HAL_MPLS_VRF_ERR
  HAL_SUCCESS
*/
int
hal_mpls_if_update_vrf (struct if_ident *if_ident, int vrf);

/*
  Name: hal_mpls_clear_fib_table

  Description:
  This API clears all the MPLS FIB entries matching the specified identifier.
  The identifier is application specific. For eg: the applications can
  use protocol identifier as a identifier for clearing out entries.

  Parameters:
  IN -> protocol

  Returns:
  HAL_MPLS_CLEAR_FIB_ERR
  HAL_SUCCESS
*/
int
hal_mpls_clear_fib_table (u_char protocol);

/*
  Name: hal_mpls_clear_vrf_table

  Description:
  This API clears all the VRF entries matching the specified identifier.
  The identifier is application specific. For eg: the applications can
  use protocol identifier as a identifier for clearing out entries.

  Parameters:
  IN -> protocol

  Returns:
  HAL_MPLS_CLEAR_VRF_ERR
  HAL_SUCCESS
*/
int
hal_mpls_clear_vrf_table (u_char protocol);

/*
  Name: hal_mpls_ftn_entry_add

  Description:
  This API adds the specified FTN entry to the FTN table. If the entry
  exists, this request is ignored. This API can also be used to modify
  an existing entry.

  Parameters:
  IN -> vrf : VRF to add the entry to. A value of 
              HAL_MPLS_GLOBAL_FTN_TABLE table adds the entry to the global
              FTN table.
  IN -> protocol: identifier for this entry
  IN -> fec_addr : IP address of FEC corresponding to this FTN entry
  IN -> fec_prefix_len : Length of the prefix for this FEC
  IN -> dscp_in : DSCP Code Point
  IN -> tunnel_label : Tunnel LSP label. Only the lower order 20 bits are used
                       This is the lsp label or tunnel label used to 
                       carry l2/l3 vpn labels.
  IN -> tunnel_nexthop_addr : IP address of the tunnel lsp next-hop to be used 
                              for this FEC
  IN -> tunnel_nexthop_if : Nexthop interface for the tunnel lsp
  IN -> vpn_label : Inner label (vc/vrf label). Only the lower order 20 bits are used
  IN -> vpn_nexthop_addr : IP address of vpn (vc/vrf) peer (only required for vc/vrf LSPs)
  IN -> vpn_nexthop_if : Outgoing interface used for (only required for vc/vrf LSPs)
                         vpn (vc/vrf) peer (Optional Parameter, may be set to NULL)
  IN -> tunnel_id : Tunnel ID
  IN -> qos_resource_id : QoS resource ID
  IN -> tunnel_ds_info : Diffserv information for this FTN entry
  IN -> opcode : Operation code to be applied to this FTN entry
                 (HAL_MPLS_PUSH, HAL_MPLS_PUSH_AND_LOOKUP,
                 HAL_MPLS_DLVR_TO_IP)
  IN -> nhlfe_ix : Next hop label forwarding entry index 
  IN -> owner :  MPLS owner type
  IN -> bypass_ftn_ix : bypass FTN index
  IN -> lsp_type : LSP type

  Returns:
  HAL_MPLS_FTN_ADD_ERR
  HAL_SUCCESS
*/
int
hal_mpls_ftn_entry_add (int vrf,
                        u_char protocol,
                        struct hal_in4_addr *fec_addr,
                        u_char *fec_prefix_len,
                        u_char *dscp_in,
                        u_int32_t *tunnel_label,
                        struct hal_in4_addr *tunnel_nexthop_addr,
                        struct if_ident *tunnel_nexthop_if,
                        u_int32_t *vpn_label,
                        struct hal_in4_addr *vpn_nexthop_addr,
                        struct if_ident *vpn_outgoing_if,
                        u_int32_t *tunnel_id,
                        u_int32_t *qos_resource_id,
#ifdef HAVE_DIFFSERV
                        struct hal_mpls_diffserv *tunnel_ds_info,
#endif /* HAVE_DIFFSERV */
                        char opcode,
                        u_int32_t nhlfe_ix,
                        u_int32_t ftn_ix,
                        u_char ftn_type,
                        struct mpls_owner_fwd *owner,
                        u_int32_t bypass_ftn_ix,
                        u_char lsp_type);



/*
  Name: hal_mpls_ftn_entry_delete

  Description:
  This API removes the specified entry from the FTN table. If the identifier
  doesnot match the one stored in the FTN entry, the delete operation will 
  fail.

  Parameters:
  IN -> vrf : VRF table identifier
  IN -> protocol : Entry identifier
  IN -> fec_addr : IP address of FEC corresponding to this FTN entry
  IN -> fec_prefix_len : Length of the prefix for this FEC
  IN -> dscp_in : DSCP Code Point
  IN -> tunnel_nhop : IP address of the tunnel lsp next-hop to be used 
                      for this FEC
  IN -> nhlfe_ix : 
  IN -> tunnel_id: LSP tunnel identifier.

  Returns:
  HAL_MPLS_FTN_DELETE_ERR
  HAL_SUCCESS
*/
int
hal_mpls_ftn_entry_delete (int vrf,
                           u_char protocol,
                           struct hal_in4_addr *fec_addr,
                           u_char *fec_prefix_len,
                           u_char *dscp_in,
                           struct hal_in4_addr *tunnel_nhop,
                           u_int32_t nhlfe_ix,
                           u_int32_t *tunnel_id,
                           u_int32_t ftn_ix);




/*
  Name : hal_mpls_ilm_entry_add
  
  Description:
  This API adds the specified ILM entry to the ILM table. If this entry
  already exists in the ILM table, the request is ignored. This API can
  also be used to modify an existing entry.

  Parameters:
  IN -> in_label : Incoming label ID. Only the low order 20 bits are used.
  IN -> in_if : Identifying object for the incoming interface
  IN -> opcode : Operation code to be applied for this FTN entry
                 (HAL_MPLS_POP, HAL_MPLS_SWAP, HAL_MPLS_POP_FOR_VPN)
  IN -> nexthop : IP address of the next-hop to be used for this FEC
  IN -> out_if : Identfying object for the outgoing interface
  IN -> swap_label: ID of the swap label. Only the low order 20 bits are used.
  IN -> nhlfe_ix :`
  IN -> is_egress : Flag to identify whether the LSR is a egress for this FEC.
  IN -> tunnel_label: ID of the tunnel label (if any).
  IN -> qos_resource_id : QOS resource ID.
  IN -> ds_info : Diffserv information for ILM entry
  IN -> fec_addr : IP address of FEC corresponding to this ILM entry
  IN -> fec_prefixlen : Length of the prefix for this FEC
  IN -> vpn_id - VPI identifier
  IN -> vc_peer - VC peer address (for VC ilm entries only)

  Returns:
  HAL_MPLS_ILM_ADD_ERR
  HAL_SUCCESS
*/
int
hal_mpls_ilm_entry_add (u_int32_t *in_label,
                        struct if_ident *in_if,
                        u_char opcode,
                        struct hal_in4_addr *nexthop,
                        struct if_ident *out_if,
                        u_int32_t *swap_label,
                        u_int32_t nhlfe_ix,
                        u_char is_egress,
                        u_int32_t *tunnel_label,
                        u_int32_t *qos_resource_id,
#ifdef HAVE_DIFFSERV
                        struct hal_mpls_diffserv *ds_info,
#endif /* HAVE_DIFFSERV */
                        struct hal_in4_addr *fec_addr,
                        unsigned char *fec_prefixlen,
                        u_int32_t vpn_id,
                        struct hal_in4_addr *vc_peer);

#ifdef HAVE_IPV6
int
hal_mpls_ilm6_entry_add (u_int32_t *in_label,
                         struct if_ident *in_if,
                         u_char opcode,
                         struct hal_in6_addr *nexthop,
                         struct if_ident *out_if,
                         u_int32_t *swap_label,
                         u_int32_t nhlfe_ix,
                         u_char is_egress,
                         u_int32_t *tunnel_label,
                         u_int32_t *qos_resource_id,
#ifdef HAVE_DIFFSERV
                         struct hal_mpls_diffserv *ds_info,
#endif /* HAVE_DIFFSERV */
                         struct hal_in6_addr *fec_addr,
                         unsigned char *fec_prefixlen,
                         u_int32_t vpn_id,
                         struct hal_in6_addr *vc_peer);
#endif /* HAVE_IPV6 */


/*
  Name : hal_mpls_ilm_entry_delete

  Description:
  This API deletes the specified entry from the ILM table. If this
  entry is not present in the ILM table, this request is ignored. If
  the identifier doesnot match to the one stored in the ILM entry,
  the delete operation fails.

  Parameters:
  IN -> protocol : Identifier for this ILM entry
  IN -> label_id_in : Incoming label ID. Only the low order 20 bits are used.
  IN -> if_info : Indentifying object for the incoming interface
  
  Returns:
  HAL_MPLS_ILM_DELETE_ERR
  HAL_SUCCESS
*/
int
hal_mpls_ilm_entry_delete (u_char protocol,
                           u_int32_t *label_id_in,
                           struct if_ident *if_info);


/*
  Name : hal_mpls_send_ttl

  Description:
  This API sets the new TTL value for all packets that are switched through 
  the LSPs that use the current LSR for either ingress or egress.
  A value of -1 for the new TTL will use the default mechanism (the copying
  of TTL from IP packet to labeled packet and vice-versa)

  Parameters:
  IN -> protocol : Identifier for this entry
  IN -> ingress : Is ingress?
  IN -> new_ttl : New TTL value

  Returns:
  HAL_MPLS_TTL_ERR
  HAL_SUCCESS
*/
int
hal_mpls_send_ttl (u_char protocol,
                   u_char type,
                   int ingress,
                   int new_ttl);


/*
  Name : hal_mpls_local_pkt_handle 

  Description:
  This API is used to enable/disable the mapping of locally generated 
  packets.

  Parameters:
  IN -> protocol : Identifier
  IN -> enable : 1 = enable, 0 = disable

  Returns:
  HAL_MPLS_ERR
  HAL_SUCCESS
*/
int
hal_mpls_local_pkt_handle (u_char protocol,
                           int enable);


#ifdef HAVE_MPLS_VC
/*
  Name : hal_mpls_vc_init

  Description:
  This API binds a interface to a Virtual Circuit.

  Parameters:
  IN -> vc_id : Virtual Circuit identifier
  IN -> if_info : interface identifier
  IN -> vlan_id : Vlan Identifier

  Returns:
  HAL_MPLS_VC_BIND_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vc_init (u_int32_t vc_id,
                  struct if_ident *if_info,
                  u_int16_t vlan_id);


/*
  Name : hal_mpls_vc_deinit

  Description:
  This API unbinds a interface from a Virtual Circuit

  Parameters:
  IN -> vc_id : Virtual Circuit identifier
  IN -> if_info : interface identifier
  IN -> vlan_id : Vlan Identifier

  Returns:
  HAL_MPLS_VC_UNBIND_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vc_deinit (u_int32_t vc_id,
                  struct if_ident *if_info,
                  u_int16_t vlan_id);
                    

/*
  Name : hal_mpls_cw_capability

  Description:
  This API checks whether the control word support is present or not.
  At this point this is just a dummy API always returning SUCCESS indicating
  that the control word is supported

  Parameters:
  None

  Returns:
  HAL_SUCCESS
  < 0 on error
*/
int
hal_mpls_cw_capability ();


/*
  Name : hal_mpls_vc_fib_add

  Description:
  This API adds a VC FIB(FTN/ILM) entry for a VC peer.  This API can also be used to modify an existing FTN entry.

  Parameters:
  IN -> vc_id : VC identifier
  IN -> vc_style : Type of vc (Mesh, Spoke, Martini)
  IN -> vpls_id : VPLS Identifier
  IN -> in_label :  Incoming VC label
  IN -> out_label : VC label to be pushed on the outgoing packet
  IN -> ac_ifindex : Incoming interface index for incoming label
  IN -> nw_ifindex : Outgoing interface to reach vc neighbor
  IN -> ftn_opcode : MPLS opcode for VC FTN entry
  IN -> ftn_vc_peer : Address of VC neighbor
  IN -> ftn_vc_nhop : Address of nexthop to reach vc neighbor
  IN -> ftn_tunnel_label : Tunnel label for carrying vc traffic
  IN -> ftn_tunnel_nhop : Address of nexthop node for tunnel lsp
  IN -> ftn_tunnel_ifindex : Outgoing interface index for tunnel lsp
  IN -> ftn_tunnel_nhlfe_ix : NHLFE index for tunnel ftn

  Returns:
  HAL_MPLS_VPLS_FIB_ADD_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vc_fib_add (u_int32_t vc_id,
                       u_int32_t vc_style,
                       u_int32_t vpls_id,
                       u_int32_t in_label,
                       u_int32_t out_label,
                       u_int32_t ac_ifindex,
                       u_int32_t nw_ifindex,
                       u_char ftn_opcode,
                       struct pal_in4_addr *ftn_vc_peer,
                       struct pal_in4_addr *ftn_vc_nhop,
                       u_int32_t ftn_tunnel_label,
                       struct pal_in4_addr *ftn_tunnel_nhop,
                       u_int32_t ftn_tunnel_ifindex,
                       u_int32_t ftn_tunnel_nhlfe_ix);

                       

/*
  Name : hal_mpls_vc_fib_delete

  Description:
  This API deletes a VC FIB entry(FTN/ILM). If the identifier doesnot match, then the delete operation fails.

  Parameters:
  IN -> vc_id : VC identifier
  IN -> vc_style : Type of vc (Mesh, Spoke)
  IN -> vpls_id : VPLS Identifier
  IN -> in_label :  Incoming VC label
  IN -> nw_ifindex : Incoming interface index for incoming label
  IN -> ftn_vc_peer : Address of VC neighbor

  Returns:
  HAL_MPLS_VPLS_FIB_DELETE_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vc_fib_delete (u_int32_t vc_id,
                        u_int32_t vc_style,
                        u_int32_t vpls_id,
                        u_int32_t in_label,
                        u_int32_t nw_ifindex,
                        struct hal_in4_addr *ftn_vc_peer);


/** @brief Function to add the HAL to HSL
 *   messaging part - in future
 *
 *  @return HAL_SUCCESS 
 **/
int
hal_mpls_pw_get_perf_cntr (struct nsm_mpls_pw_snmp_perf_curr *curr);

#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
/*
  Name : hal_mpls_vpls_add

  Description:
  This API adds a VPLS entry.

  Parameters:
  IN -> vpls_id : VPLS Identifier

  Returns:
  HAL_MPLS_VPLS_FWD_ADD_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vpls_add (u_int32_t vpls_id);

/*
  Name : hal_mpls_vpls_del

  Description:
  This API deletes a VPLS entry.

  Parameters:
  IN -> vpls_id : VPLS Identifier

  Returns:
  HAL_MPLS_VPLS_FWD_ADD_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vpls_del (u_int32_t vpls_id);





/*
  Name : hal_mpls_vpls_if_bind

  Description:
  This API binds a VPLS entry to an interface.

  Parameters:
  IN -> vpls_id : VPLS Identifier
  IN -> ifindex : Interface index being bound to a VPLS

  Returns:
  HAL_MPLS_VPLS_IF_BIND_ERR
  HAL_SUCCESS
*/

int
hal_mpls_vpls_if_bind (u_int32_t vpls_id,
                       u_int32_t ifindex,
                       u_int16_t vlan_id);


/*
  Name : hal_mpls_vpls_if_unbind

  Description:
  This API unbinds a VPLS entry to an interface.

  Parameters:
  IN -> vpls_id : VPLS Identifier
  IN -> ifindex : Interface index being bound to a VPLS

  Returns:
  HAL_MPLS_VPLS_IF_BIND_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vpls_if_unbind (u_int32_t vpls_id,
                         u_int32_t ifindex,
                         u_int16_t vlan_id);

/*
  Name : hal_mpls_vpls_mac_withdraw

  Description:
  This API send VPLS MAC Withdraw message to HW.

  Parameters:
  IN -> vpls_id : VPLS Identifier
  IN -> num : number of withdrawed mac address 
  IN -> mac_addrs : withdrawed mac address 

  Returns:
  HAL_MPLS_VPLS_MAC_WITHDRAW_ERR
  HAL_SUCCESS
*/
int
hal_mpls_vpls_mac_withdraw (u_int32_t vpls_id,
                            u_int16_t num,
                            u_char *mac_addrs);

#endif /* HAVE_VPLS */

#ifdef HAVE_TE 
/*
   Name: hal_mpls_qos_reserve

   Description:
   This API reserve MPLS QoS.

   Parameters:
   IN-> qos : qos related parameter of struct hal_mpls_qos.

   Return:
   HAL_MPLS_QOS_RESERVE_ERR
   HAL_SUCCESS
*/
int
hal_mpls_qos_reserve (struct hal_mpls_qos *qos);

/*
   Name: hal_mpls_qos_release

   Description:
   This API release reserved MPLS QoS.

   Parameters:
   IN-> qos : qos related parameter of struct hal_mpls_qos.

   Return:
   HAL_MPLS_QOS_RELEASE_ERR
   HAL_SUCCESS
*/
int
hal_mpls_qos_release (struct hal_mpls_qos *qos);

#endif /* HAVE_TE */
#endif /* _HAL_MPLS_H_ */
