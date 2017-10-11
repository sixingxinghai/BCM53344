/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "tlv.h"

#ifdef HAVE_MPLS

#include "mpls_client/mpls_common.h"
#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"
#include "hal_mpls_types.h"

/* Libglobals. */
extern struct lib_globals *hal_zg;

int
_hal_send_mpls_control (int msgtype, u_char protocol)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_control mc;
  } req;

  req.mc.protocol = protocol;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.mc));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_mpls_init

   Description:
   This API initializes MPLS forwarding plane.

   Parameters:
   IN -> protocol

   Returns:
   < 0 error
   HAL_SUCCESS
*/
int
hal_mpls_init (u_char protocol)
{
  return _hal_send_mpls_control (HAL_MSG_MPLS_INIT, protocol);
}

/* 
   Name: hal_mpls_deinit 

   Description:
   This API deinitializes MPLS forwarding plane.

   Parameters:
   IN -> protocol

   Returns:
   < 0 error
   HAL_SUCCESS
*/
int
hal_mpls_deinit (u_char protocol)
{
  return _hal_send_mpls_control (HAL_MSG_MPLS_END, protocol);
}

int
_hal_send_mpls_if (int msgtype, struct hal_msg_mpls_if *msg)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_if mif;
  } req;

  pal_mem_cpy (&req.mif, msg, sizeof (req.mif));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.mif));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


int
_hal_mpls_vpn_add_del (int msgtype, u_int32_t vpn_id)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_int32_t vpn_id;
  } req;

  req.vpn_id = vpn_id;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (u_int32_t));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  return ret;
}



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
hal_mpls_vrf_create (int vrf)
{
  if (vrf < 0)
    return -1;

  return _hal_mpls_vpn_add_del (HAL_MSG_MPLS_VRF_INIT, (u_int32_t)vrf);
}

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
hal_mpls_vrf_destroy (int vrf)
{
  return _hal_mpls_vpn_add_del (HAL_MSG_MPLS_VRF_END, (u_int32_t)vrf);
}

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
                           unsigned short label_space)
{
  struct hal_msg_mpls_if m_if;

  /* Make sure that the loopback interface is not being passed here */
  if (if_ident->if_index <= 1)
    return -1;

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (m_if));

  /* Set ifname */
  pal_strncpy (m_if.ifname, if_ident->if_name, HAL_IFNAME_LEN);

  /* Set the ifindex */
  m_if.ifindex = if_ident->if_index;

  /* Set the label space */
  m_if.label_space = label_space;

  return _hal_send_mpls_if (HAL_MSG_MPLS_IF_INIT, &m_if);
}

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
hal_mpls_disable_interface (struct if_ident *if_ident)
{
  struct hal_msg_mpls_if m_if;

  /* Make sure that the loopback interface is not being passed here */
  if (if_ident->if_index <= 1)
    return -1;

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (m_if));

  /* Set ifname */
  pal_strncpy (m_if.ifname, if_ident->if_name, HAL_IFNAME_LEN);

  /* Set the ifindex */
  m_if.ifindex = if_ident->if_index;

  return _hal_send_mpls_if (HAL_MSG_MPLS_IF_END, &m_if);
}

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
hal_mpls_if_update_vrf (struct if_ident *if_ident, int vrf)
{
#if 0
  struct hal_msg_mpls_if m_if;

  /* Make sure that the vrf ident is not zero */
  pal_assert (vrf != 0);

  /* Make sure that the loopback interface is not being passed here */
  if (if_ident->if_index <= 1)
    return -1;

  /* Start with clean mif. */
  pal_mem_set (&m_if, 0, sizeof (m_if));

  /* Set ifname */
  pal_strncpy (m_if.ifname, if_ident->if_name, HAL_IFNAME_LEN);

  /* Set label space to zero */
  m_if.label_space = 0;

  /* Check for vrf identifier */
  m_if.vrf_ident = vrf;

  /* Set the ifindex */
  m_if.ifindex = if_ident->if_index;

  return _hal_send_mpls_if (HAL_MSG_MPLS_IF_UPDATE, &m_if);
#endif
  return 0;
}

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
hal_mpls_clear_fib_table (u_char protocol)
{
  return _hal_send_mpls_control (HAL_MSG_MPLS_FIB_CLEAN, protocol);
}

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
hal_mpls_clear_vrf_table (u_char protocol)
{
  return _hal_send_mpls_control (HAL_MSG_MPLS_VRF_CLEAN, protocol);
}

int
_hal_mpls_send_ftn_add (u_char *fa_req, int len)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[512];
  } req;

  
  pal_mem_cpy (req.buf, fa_req, len);

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (len);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_NEWFTN;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

void
_mpls_ftn_msg_encode (u_char **pnt, u_int16_t *size, struct hal_msg_mpls_ftn_add *fa)
{
  TLV_ENCODE_PUTC (fa->family);
  TLV_ENCODE_PUTL (fa->vrf);
  TLV_ENCODE_PUTL (fa->fec_addr);
  TLV_ENCODE_PUTC (fa->fec_prefixlen);
  
  if (fa->dscp_in != 0xff)
    TLV_ENCODE_PUTC (fa->dscp_in);
  else 
    TLV_ENCODE_PUT_EMPTY (1);
  TLV_ENCODE_PUTL (fa->tunnel_label);
  TLV_ENCODE_PUTL (fa->tunnel_nhop);
  TLV_ENCODE_PUTL (fa->tunnel_oif_ix);
  TLV_ENCODE_PUT (fa->tunnel_oif_name, HAL_IFNAME_LEN + 1);
  TLV_ENCODE_PUTC (fa->opcode);
  TLV_ENCODE_PUTL (fa->nhlfe_ix);
  TLV_ENCODE_PUTL (fa->ftn_ix);
  TLV_ENCODE_PUTC (fa->ftn_type);

  if (fa->tunnel_id != 0)
    TLV_ENCODE_PUTL (fa->tunnel_id);
  else
    TLV_ENCODE_PUT_EMPTY (4);

  if (fa->qos_resource_id != 0)
    TLV_ENCODE_PUTL (fa->qos_resource_id);
  else
    TLV_ENCODE_PUT_EMPTY (4);

  TLV_ENCODE_PUTL (fa->bypass_ftn_ix);
  TLV_ENCODE_PUTC (fa->lsp_type);

  if (fa->vpn_label != LABEL_VALUE_INVALID)
    {
      TLV_ENCODE_PUTL (fa->vpn_label);
      TLV_ENCODE_PUTL (fa->vpn_nhop);

      if (fa->vpn_oif_ix > 0)
        {
          TLV_ENCODE_PUTL (fa->vpn_oif_ix);
          TLV_ENCODE_PUT (fa->vpn_oif_name, HAL_IFNAME_LEN + 1);
        }
      else
        {
          TLV_ENCODE_PUT_EMPTY (HAL_IFNAME_LEN + 5);
        }
    }
  else
    {
      TLV_ENCODE_PUT_EMPTY (HAL_IFNAME_LEN + 13);
    }
}


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
  IN -> diffserv_info : Diffserv information for this FTN entry
  IN -> opcode : Operation code to be applied to this FTN entry
                 (HAL_MPLS_PUSH, HAL_MPLS_PUSH_AND_LOOKUP,
                 HAL_MPLS_DLVR_TO_IP)
  IN -> nhlfe_ix : 
  IN -> owner : 

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
                        struct hal_in4_addr *tunnel_nhop,
                        struct if_ident *tunnel_if_ident,
                        u_int32_t *vpn_label,
                        struct hal_in4_addr *vpn_nhop,
                        struct if_ident *vpn_if_ident,
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
                        u_char lsp_type)
{
  u_char ptr[512];
  u_char *pnt = ptr;
  u_int16_t size = 512;
  struct hal_msg_mpls_ftn_add fa;

  if (!fec_prefix_len || !fec_addr || !tunnel_nhop || !tunnel_if_ident)
    return -1;

  /* Make sure that the opcode is correct */
  pal_assert ((opcode == PUSH) || (opcode == PUSH_AND_LOOKUP) ||
          (opcode == DLVR_TO_IP) || (opcode == FTN_LOOKUP));

  memset (ptr, 0, 512);
  memset (&fa, 0, sizeof (struct hal_msg_mpls_ftn_add));
  fa.family = AF_INET;
  fa.vrf = vrf;
  fa.fec_addr = fec_addr->s_addr;
  fa.fec_prefixlen = *fec_prefix_len;
  if (dscp_in)
    fa.dscp_in = *dscp_in;
  else
    fa.dscp_in = 0xff;
  fa.tunnel_label = *tunnel_label;
  fa.tunnel_nhop = tunnel_nhop->s_addr;
  fa.tunnel_oif_ix = tunnel_if_ident->if_index;
  pal_strcpy (fa.tunnel_oif_name, tunnel_if_ident->if_name);
  if (tunnel_id)
    fa.tunnel_id = *tunnel_id;

  fa.opcode = opcode;
  fa.nhlfe_ix = nhlfe_ix;
  fa.ftn_ix = ftn_ix;
  fa.ftn_type = ftn_type;

  if (qos_resource_id)
    fa.qos_resource_id = *qos_resource_id;
   
  if (vpn_label)
    {
  fa.vpn_label = *vpn_label;
  fa.vpn_nhop = vpn_nhop->s_addr;
    if (vpn_if_ident)
      {
  fa.vpn_oif_ix = vpn_if_ident->if_index;
  pal_strcpy (fa.vpn_oif_name, vpn_if_ident->if_name);
      }
    }

#if 0
#ifdef HAVE_DIFFSERV
  if (tunnel_ds_info)
    {
      fa.tunnel_ds_info.lsp_type = tunnel_ds_info->lsp_type;
      if (fa.tunnel_ds_info.lsp_type == HAL_MPLS_LLSP)
        {
          fa.tunnel_ds_info.dscp = tunnel_ds_info->dscp;
        }
      else if (fa.tunnel_ds_info.lsp_type != HAL_MPLS_LSP_DEFAULT)
        {
          pal_mem_cpy (fa.tunnel_ds_info.dscp_exp_map, 
                       tunnel_ds_info->dscp_exp_map, 8);
        }
    }
  else
    {
      fa.tunnel_ds_info.lsp_type = HAL_MPLS_LSP_DEFAULT;
    }
#endif /* HAVE_DIFFSERV */
#endif /* 0 */

  /* Fill Bypass FTN index. */
  fa.bypass_ftn_ix = bypass_ftn_ix;

  /* Fill LSP type. */
  fa.lsp_type = lsp_type;

  _mpls_ftn_msg_encode (&pnt, &size, &fa);

  if (IS_HAL_DEBUG_EVENT)
    zlog_info (hal_zg, "hal_mpls_ftn_entry_add: FTN entry add"
               " with QOS resource-id %u",
               (qos_resource_id == NULL) ? 0 : *qos_resource_id);

  return _hal_mpls_send_ftn_add (ptr, HAL_MSG_MPLS_FTN_ADD_LEN);
}

int
_hal_mpls_send_ftn_del (struct hal_msg_mpls_ftn_del *fd_req)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_ftn_del msg;
  } req;

  
  pal_mem_cpy (&req.msg, fd_req, sizeof (req.msg));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.msg));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_DELFTN;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

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
                           u_int32_t ftn_ix)
{
  struct hal_msg_mpls_ftn_del fd;

  if (!fec_addr || !fec_prefix_len)
    return -1;

  fd.family = AF_INET;

  /* Set the VRF where we want to add to */
  fd.vrf = vrf;

  /* Set the fec prefix */
  fd.fec_addr = fec_addr->s_addr;

  /* Set the fec prefixlen */
  fd.fec_prefixlen = *fec_prefix_len;

  if (dscp_in)
    fd.dscp_in = *dscp_in;
  else
    fd.dscp_in = 0xff;

  fd.tunnel_nhop = tunnel_nhop->s_addr;
  fd.nhlfe_ix = nhlfe_ix;
  if (tunnel_id)
    fd.tunnel_id = *tunnel_id;
  fd.ftn_ix = ftn_ix;

  return _hal_mpls_send_ftn_del (&fd);
}

int
_hal_mpls_send_ilm_add (struct hal_msg_mpls_ilm_add *ia_req)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_ilm_add msg;
  } req;

  
  pal_mem_cpy (&req.msg, ia_req, sizeof (req.msg));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.msg));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_NEWILM;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

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
  IN -> qos_resource_id : QoS resource ID
  IN -> ds_info : Diffserv information for ILM entry
  IN -> fec_addr : IP address of FEC corresponding to this ILM entry
  IN -> fec_prefixlen : Length of the prefix for this FEC
  IN -> vpn_id : VPN identifier
  IN -> vc_peer : VC peer address (for vc ilm entries only)

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
                        struct hal_in4_addr *vc_peer)
{
  struct hal_msg_mpls_ilm_add ia;

  if (! in_label || ! in_if || ! out_if || !fec_addr ||
      !fec_prefixlen || !nexthop)
    return -1;

  pal_mem_set (&ia, 0, sizeof (struct hal_msg_mpls_ilm_add));
  ia.family = AF_INET;

  /* Set the incoming label */
  ia.in_label = *in_label;

  /* No interface specific label space */
  ia.in_ifindex = in_if->if_index;
  pal_strncpy (ia.in_ifname, in_if->if_name, HAL_IFNAME_LEN);

  /* Set the fec info */
  ia.fec_addr = fec_addr->s_addr;
  ia.fec_prefixlen = *fec_prefixlen;
  
  /* Set the nexthop addr */
  ia.nexthop = nexthop->s_addr;

  /* Outgoing ifindex */
  ia.out_ifindex = out_if->if_index;
  pal_strncpy (ia.out_ifname, out_if->if_name, HAL_IFNAME_LEN);

  /* Set the swap label */
  if (swap_label)
    {
      ia.swap_label = *swap_label;
      if (ia.swap_label == HAL_MPLS_IMPLICIT_NULL)
        opcode = HAL_MPLS_PHP;
    }
  else if ((! swap_label) || is_egress)
    {
      ia.swap_label = -1;
    }

  ia.tunnel_label = LABEL_VALUE_INVALID;

  /* Pass in the opcode */
  ia.opcode = opcode;
  
  /* Fill NHLFE index. */
  ia.nhlfe_ix = nhlfe_ix;

  ia.vpn_id = vpn_id;

#ifdef HAVE_MPLS_VC
  if (opcode == HAL_MPLS_POP_FOR_VC)
    ia.vc_peer = vc_peer->s_addr;
#endif /* HAVE_MPLS_VC */


#ifdef HAVE_DIFFSERV 
  if (ds_info)
    {
      ia.ds_info.lsp_type = ds_info->lsp_type;
      if (ia.ds_info.lsp_type == HAL_MPLS_LLSP)
        {
          ia.ds_info.dscp = ds_info->dscp;
        }
      else if (ia.ds_info.lsp_type != HAL_MPLS_LSP_DEFAULT)
        {
          pal_mem_cpy (ia.ds_info.dscp_exp_map, ds_info->dscp_exp_map, 8);
        }
    }
  else
    {
      ia.ds_info.lsp_type = HAL_MPLS_LSP_DEFAULT;
    }
#endif /* HAVE_DIFFSERV */

  if (IS_HAL_DEBUG_EVENT)
    zlog_info (hal_zg, "hal_mpls_ilm_entry_add: ILM entry add"
               " with QOS resource-id %u", 
               (qos_resource_id == NULL) ? 0 : *qos_resource_id);

  return _hal_mpls_send_ilm_add (&ia);
}

#ifdef HAVE_IPV6
/* Defined here to fix the compilation issue */
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
                         struct hal_in6_addr *vc_peer)
{
  if (IS_HAL_DEBUG_EVENT)
    zlog_info (hal_zg, "hal_mpls_ilm6_entry_add: ILM entry add"
               " with QOS resource-id %u",
               (qos_resource_id == NULL) ? 0 : *qos_resource_id);
 
  return 0;
}
#endif /* HAVE_IPV6 */


int
_hal_mpls_send_ilm_del (struct hal_msg_mpls_ilm_del *id_req)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_ilm_del msg;
  } req;

  
  pal_mem_cpy (&req.msg, id_req, sizeof (req.msg));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.msg));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_DELILM;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

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
                           struct if_ident *if_info)
{
  struct hal_msg_mpls_ilm_del id;

  if (label_id_in == NULL || if_info == NULL)
    return -1;

  id.family = AF_INET;

  id.in_label = *label_id_in;
  
  /* No interface specific label space */
  id.in_ifindex = if_info->if_index;

  /* Send the ILM del entry to the kernel */
  return _hal_mpls_send_ilm_del (&id);
}

int
_hal_mpls_send_ttl (struct hal_msg_mpls_ttl *ttl_req)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_ttl msg;
  } req;

  
  pal_mem_cpy (&req.msg, ttl_req, sizeof (req.msg));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.msg));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_TTL_HANDLING;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

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
                   int new_ttl)
{
  struct hal_msg_mpls_ttl m_ttl;

  /* Currently only supporting this via the configuration utility */
  pal_assert (protocol == APN_PROTO_NSM);

  /* Set the protocol */
  m_ttl.protocol = protocol;

  /* Set type */
  m_ttl.type = type;

  /* Set the ingress value */
  m_ttl.is_ingress = ingress;

  /* Set the new_ttl */
  m_ttl.new_ttl = new_ttl;

  return _hal_mpls_send_ttl (&m_ttl);
}

int
_hal_mpls_send_local_pkt_handle (struct hal_msg_mpls_conf_handle *conf)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_conf_handle msg;
  } req;

  pal_mem_cpy (&req.msg, conf, sizeof (req.msg));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.msg));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_LOCAL_PKT;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

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
                           int enable)
{
  struct hal_msg_mpls_conf_handle m_local;

  /* Currently only supporting this via the configuration utility */
  pal_assert (protocol == APN_PROTO_NSM);
  
  /* Set the protocol */
  m_local.protocol = protocol;

  /* Set the enable/disable flag */
  m_local.enable = enable;

  return _hal_mpls_send_local_pkt_handle (&m_local);
}


#ifdef HAVE_MPLS_VC
int
_hal_mpls_vpn_if (int msgtype, u_int32_t vpls_id,
                  u_int32_t ifindex, u_int16_t vlan_id)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_vpn_if msg;
  } req;

  req.msg.vpn_id = vpls_id;
  req.msg.ifindex = ifindex;
  req.msg.vlan_id = vlan_id;


  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_mpls_vpn_if));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  return ret;
}


/*
  Name : hal_mpls_vc_init

  Description:
  This API creates a virtual circuit and binds an interface to it

  Parameters:
  IN -> vc_id : Virtual Circuit identifier
  IN -> if_info : interface identifier
  IN -> vlan_id : Vlan identifier

  Returns:
  HAL_SUCCESS
  < 0 on error
*/
int
hal_mpls_vc_init (u_int32_t vc_id,
                  struct if_ident *if_info,
                  u_int16_t vlan_id)
{
  return _hal_mpls_vpn_if (HAL_MSG_MPLS_VC_INIT, vc_id,
                           if_info->if_index, vlan_id);
}

/*
  Name : hal_mpls_vc_deinit

  Description:
  This API unbinds an interface from a Virtual Circuit and deletes the vc

  Parameters:
  IN -> vc_id : Virtual Circuit identifier
  IN -> if_info : interface identifier
  IN -> vlan_id : Vlan identifier

  Returns:
  HAL_SUCCESS
  < 0 on error
*/
int
hal_mpls_vc_deinit (u_int32_t vc_id, 
                    struct if_ident *if_info, u_int16_t vlan_id)
{
  return _hal_mpls_vpn_if (HAL_MSG_MPLS_VC_END, vc_id,
                           if_info->if_index, vlan_id);
}

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
hal_mpls_cw_capability ()
{
  return 0;
}

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
                       u_int32_t ftn_tunnel_nhlfe_ix)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_vc_fib_add msg;
  } req;


  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_mpls_vc_fib_add));

  req.msg.vc_id = vc_id;
  req.msg.vc_style = vc_style;
  req.msg.vpls_id = vpls_id;
  req.msg.in_label = in_label;
  req.msg.out_label = out_label;
  req.msg.ac_ifindex = ac_ifindex;
  req.msg.nw_ifindex = nw_ifindex;
  req.msg.opcode = ftn_opcode;
  req.msg.vc_peer = ftn_vc_peer->s_addr;
  req.msg.vc_nhop = ftn_vc_nhop->s_addr;
  req.msg.tunnel_label = ftn_tunnel_label;
  if (ftn_tunnel_nhop)
    req.msg.tunnel_nhop = ftn_tunnel_nhop->s_addr;
  req.msg.tunnel_oif_ix = ftn_tunnel_ifindex;
  req.msg.tunnel_nhlfe_ix = ftn_tunnel_nhlfe_ix;
    
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_mpls_vc_fib_add));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_VC_FIB_ADD;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  return ret;
}



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
                          struct hal_in4_addr *ftn_vc_peer)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mpls_vc_fib_del msg;
  } req;

  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_mpls_vc_fib_del));

  req.msg.vc_id = vc_id;
  req.msg.vc_style = vc_style;
  req.msg.vpls_id = vpls_id;
  req.msg.in_label = in_label;
  req.msg.nw_ifindex = nw_ifindex;
  req.msg.vc_peer = ftn_vc_peer->s_addr;
    
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_mpls_vc_fib_del));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MPLS_VC_FIB_DEL;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  return ret;
}

/** @brief Function to add the HAL to HSL
 *  messaging part - in future
 *
 *  @return 0
 **/
int
hal_mpls_pw_get_perf_cntr (struct nsm_mpls_pw_snmp_perf_curr *curr)
{
 /* for future use - hal to hsl messaging to be done here for
  * a specific hardware*/
  return HAL_SUCCESS;
}
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
hal_mpls_vpls_add (u_int32_t vpls_id)
{
  return _hal_mpls_vpn_add_del (HAL_MSG_MPLS_VPLS_ADD, vpls_id);
}



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
hal_mpls_vpls_del (u_int32_t vpls_id)
{
  return _hal_mpls_vpn_add_del (HAL_MSG_MPLS_VPLS_ADD, vpls_id);
}



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
                       u_int16_t vlan_id)
{
  return _hal_mpls_vpn_if (HAL_MSG_MPLS_VPLS_IF_BIND, vpls_id,
                            ifindex, vlan_id);
}


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
                         u_int16_t vlan_id)
{
  return _hal_mpls_vpn_if (HAL_MSG_MPLS_VPLS_IF_UNBIND, vpls_id,
                           ifindex, vlan_id);
}

/*
 *   Name : hal_mpls_vpls_mac_withdraw
 * 
 *   Description:
 *   This API send VPLS MAC Withdraw message to HW.
 * 
 *   Parameters:
 *   IN -> vpls_id : VPLS Identifier
 *   IN -> num : number of withdrawed mac address 
 *   IN -> mac_addrs : withdrawed mac address 
 *
 *   Returns:
 *   HAL_MPLS_VPLS_MAC_WITHDRAW_ERR
 *   HAL_SUCCESS
 */
int
hal_mpls_vpls_mac_withdraw (u_int32_t vpls_id,
                            u_int16_t num,
                            u_char *mac_addrs)
{
      return HAL_SUCCESS;
}

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
hal_mpls_qos_reserve (struct hal_mpls_qos *qos)
{
  if (IS_HAL_DEBUG_EVENT)   
    zlog_info (hal_zg, "hal_mpls_qos_reserve: Reserve QOS resource");
  return 0;
}

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
hal_mpls_qos_release (struct hal_mpls_qos *qos)
{
  if (IS_HAL_DEBUG_EVENT)
    zlog_info (hal_zg, "hal_mpls_qos_release: Release QOS resource");
  return 0;
}

#endif /* HAVE_TE */
#endif /* HAVE_MPLS */

