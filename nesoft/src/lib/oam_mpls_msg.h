/* Copyright (C) 2009-2010  All Rights Reserved. */
#ifndef _PACOS_OAM_MPLS_MSG_H
#define _PACOS_OAM_MPLS_MSG_H

#ifdef HAVE_MPLS_OAM
#include "nsm_message.h"
#include "mpls_client/mpls_common.h"

struct prefix_length
{
  u_int32_t len:8;
  u_int32_t mbz:24;
}val;

struct mpls_label_stack
{
  u_int32_t seq_num;
  u_int32_t ifindex;
  
  struct list *label;
};

struct shimhdr 
{
  u_int32_t shim;

  /*-----------------------------------------------------------------------
    Some hash defines to get the different fields of the shim header.
    -----------------------------------------------------------------------*/
#define get_label_net(shim)      (((shim) >> 12) & 0xFFFFF)
#define get_exp_net(shim)        (((shim) >> 9)  & 0x7)
#define get_bos_net(shim)        (((shim) >> 8)  & 0x1)
#define get_ttl_net(shim)        ((shim)  & 0xFF)
#define set_label_net(shim, val) {(shim) &= 0x00000FFF;(shim) |= (val)<<12;}
#define set_exp_net(shim, val)   \
                                 {(shim) &= 0xFFFFF1FF;(shim) |= (val)<<9;}
#define set_bos_net(shim, val)   \
                                 {(shim) &= 0xFFFFFEFF;(shim) |= (val)<<8;}
#define set_ttl_net(shim, val)   \
                                 {(shim) &= 0xFFFFFF00;(shim) |= (val);}
#define get_label_host(shim)     get_label_net(shim)
#define get_exp_host(shim)       get_exp_net(shim)
#define get_bos_host(shim)       get_bos_net(shim)
#define get_ttl_host(shim)       get_ttl_net(shim)
#define set_label_host(shim,val) set_label_net(shim,val)
#define set_exp_host(shim,val)   set_exp_net(shim,val)
#define set_bos_host(shim,val)   set_bos_net(shim,val)
#define set_ttl_host(shim,val)   set_ttl_net(shim,val)
};

#define VPN_RD_SAME(RD1, RD2)                                         \
  (! pal_mem_cmp ((RD1), (RD2), VPN_RD_SIZE))

#define MPLS_OAM_LDP_IPv4_PREFIX        (1 << 0)
#define MPLS_OAM_RSVP_IPv4_PREFIX       (1 << 1)
#define MPLS_OAM_L2_CIRCUIT_PREFIX_DEP  (1 << 2)
#define MPLS_OAM_L2_CIRCUIT_PREFIX_CURR (1 << 3)
#define MPLS_OAM_GEN_PW_ID_PREFIX       (1 << 4)
#define MPLS_OAM_MPLS_VPN_PREFIX        (1 << 5)
#define MPLS_OAM_GENERIC_IPv4_PREFIX    (1 << 6)
#define MPLS_OAM_NIL_FEC_PREFIX         (1 << 7) 

struct generic_ipv4_prefix
{
  struct pal_in4_addr ipv4_addr;
  union
  {
    struct prefix_length val;
    u_int32_t l_packet;
  }u;
};
struct ldp_ipv4_prefix 
{
  struct pal_in4_addr ip_addr;
  union 
  {
    struct prefix_length val;
    u_int32_t l_packet;
  }u; 
};

struct rsvp_ipv4_prefix
{
  struct pal_in4_addr ip_addr;
  /* 2 bytes mbz here */
  u_int16_t tunnel_id;
  struct pal_in4_addr ext_tunnel_id;
  struct pal_in4_addr send_addr;
  /* 2 bytes mbz here */
  u_int16_t lsp_id;
};

struct l3vpn_ipv4_prefix
{
  /* Route Distingusher value. Not really used in Processing */
  struct vpn_rd rd; 
  struct pal_in4_addr vpn_pfx;
  union
  {
    struct prefix_length val;
    u_int32_t l_packet;
  }u;
};

struct l2_circuit_dep
{
  struct pal_in4_addr remote;
  u_int32_t vc_id;
  u_int16_t pw_type;
 /* PW Types Supported are - 
  * Ethernet
  * Ethernet VLAN
  * HDLC 
  * PPP
  */
  /* Next two bytes must be Zero for Padding */
};

struct l2_circuit_curr
{
  struct pal_in4_addr source;
  struct pal_in4_addr remote;
  u_int32_t vc_id;
  u_int16_t pw_type;

 /* PW Types Supported are -
  * Ethernet
  * Ethernet VLAN
  * HDLC
  * PPP
  */
  /* Next two bytes must be Zero for Padding */
};

struct mpls_pw
{
  struct pal_in4_addr source;
  struct pal_in4_addr remote;
  u_int16_t pw_type;
  
  u_int8_t agi_type;
  u_int8_t agi_length;
  /* Allocate agi_length for agi_value */
  u_char *agi_value;

  u_int8_t saii_type;
  u_int8_t saii_length;
  /* Allocate saii_length for saii_value */
  u_char *saii_value;
  
  u_int8_t taii_type;
  u_int8_t taii_length;
  /* Allocate taii_length for taii_value */
  u_char *taii_value;

  /* Align to 4 octet boundary based on lengths */
  /* PW Types Supported are -
  * Ethernet
  * Ethernet VLAN
  * HDLC
  * PPP
  */
};

struct mpls_oam_downstream_map
{
  union 
  {
    struct dsmap_data
    {
      MPLS_BITFIELDS3 (u_int32_t mtu:16,
      u_int32_t family:8,
      u_int32_t flags:8);
    }data;

    u_int32_t l_packet;
  }u;

#define DS_FLAGS_N_BIT (1 << 0)
#define DS_FLAGS_I_BIT (1 << 1)

  struct pal_in4_addr ds_ip;
  struct pal_in4_addr ds_if_ip;
  
  /* Followng 4 values are not supported. Defined for Compatibility */
  u_int8_t multipath_type;
  u_int8_t depth;
  u_int16_t multipath_len;
  u_char *multi_info;

  struct list *ds_label;
};

/* OAM CTYPES */
typedef enum  {
   NSM_CTYPE_MSG_OAM_OPTION_LSP_TYPE = 0,
   NSM_CTYPE_MSG_OAM_OPTION_DSMAP,
   NSM_CTYPE_MSG_OAM_OPTION_LSP_ID,
   NSM_CTYPE_MSG_OAM_OPTION_FEC,
   NSM_CTYPE_MSG_OAM_OPTION_TUNNELNAME,
   NSM_CTYPE_MSG_OAM_OPTION_EGRESS,
   NSM_CTYPE_MSG_OAM_OPTION_PEER,
   NSM_CTYPE_MSG_OAM_OPTION_PREFIX,
   NSM_CTYPE_MSG_OAM_OPTION_SEQ_NUM
}nsm_ctype_oam_option;

#define NSM_OAM_NO_FTN              0
#define NSM_OAM_PACKET_SEND_ERROR   1
#define NSM_OAM_ERR_EXPLICIT_NULL   2
#define NSM_OAM_ERR_NO_CONFIG       3
#define NSM_OAM_ERR_UNKNOWN         4

/* OAM L2 DATA TYPES */
typedef enum {
   NSM_OAM_L2_CIRCUIT_PREFIX_DEP = 0,
   NSM_OAM_L2_CIRCUIT_PREFIX_CURR,
   NSM_OAM_GEN_PW_ID_PREFIX
}nsm_oam_l2_data_type;

/* OAM TUNNEL TYPES */
typedef enum {
   NSM_OAM_LDP_IPv4_PREFIX = 0,
   NSM_OAM_RSVP_IPv4_PREFIX,
   NSM_OAM_GEN_IPv4_PREFIX
}nsm_oam_tunnel_type;

/* Structures for the OAM Process/Response messages to be exchanged between 
 * NSM MPLS OAM module and OAMD module.
 */

/* NSM_MSG_OAM_LSP_PING_REQ_PROCESS */
struct nsm_msg_oam_lsp_ping_req_process
{
  /* Echo Request Options */
  cindex_t cindex;

  /* FEC type */
  u_char type;

  /* Is dsmap requested. */
  bool_t is_dsmap;

  union
  {
    struct
    {
      struct pal_in4_addr addr;
      u_char *tunnel_name;
      int tunnel_len;
    }rsvp;
    struct
    {
      u_char *vrf_name;
      int vrf_name_len;
      u_int32_t prefixlen;
      struct pal_in4_addr ip_addr;
      struct pal_in4_addr vpn_prefix;
    }fec;
    struct
    {
      u_int32_t vc_id;
      struct pal_in4_addr vc_peer;
      bool_t is_vccv;
    }l2_data;
  }u;

  /* To match request and reply between NSM and OAMD */
  u_int32_t seq_no;
};

/* NSM_MSG_OAM_LSP_PING_REQ_RESP_LDP */
struct nsm_msg_oam_lsp_ping_req_resp_ldp
{
  struct ldp_ipv4_prefix data;
  bool_t is_dsmap;
  struct mpls_oam_downstream_map dsmap;
  u_int32_t seq_no;
  struct shimhdr label;
  u_int32_t ftn_ix;
  u_int32_t oif_ix;
};

/* NSM_MSG_OAM_LSP_PING_REQ_RESP_GEN */
struct nsm_msg_oam_lsp_ping_req_resp_gen
{
  struct generic_ipv4_prefix data;
  bool_t is_dsmap;
  struct mpls_oam_downstream_map dsmap;
  u_int32_t seq_no;
  struct shimhdr label;
  u_int32_t ftn_ix;
  u_int32_t oif_ix;
};

/* NSM_MSG_OAM_LSP_PING_REQ_RESP_RSVP */
struct nsm_msg_oam_lsp_ping_req_resp_rsvp
{
  struct rsvp_ipv4_prefix data;
  bool_t is_dsmap;
  struct mpls_oam_downstream_map dsmap;
  u_int32_t seq_no;
  struct shimhdr label;
  u_int32_t ftn_ix;
  u_int32_t oif_ix;
};

/* NSM_MSG_OAM_LSP_PING_REQ_RESP_L2VC */
struct nsm_msg_oam_lsp_ping_req_resp_l2vc
{
  u_int16_t l2_data_type;
  u_int16_t tunnel_type;
  bool_t is_dsmap;
  struct mpls_oam_downstream_map dsmap;
  u_int32_t seq_no;

  union 
  {
    struct l2_circuit_dep pw_128_dep;
    struct l2_circuit_curr pw_128_cur;
    struct mpls_pw pw_129;
  }l2_data;

  union
  {
    struct ldp_ipv4_prefix ldp;
    struct rsvp_ipv4_prefix rsvp;
    struct generic_ipv4_prefix gen;
  }tunnel_data;

  u_int8_t cc_type;

  struct shimhdr tunnel_label;
  struct shimhdr vc_label;

  u_int32_t ftn_ix;
  u_int32_t acc_if_index;

  u_int32_t oif_ix;
};

/* NSM_MSG_OAM_LSP_PING_REQ_RESP_L3VPN */
struct nsm_msg_oam_lsp_ping_req_resp_l3vpn
{
  u_int16_t tunnel_type;
  bool_t is_dsmap;
  struct mpls_oam_downstream_map dsmap;
  u_int32_t seq_no;
  struct l3vpn_ipv4_prefix data;
  vrf_id_t vrf_id;

  union
  {
    struct ldp_ipv4_prefix ldp;
    struct rsvp_ipv4_prefix rsvp;
    struct generic_ipv4_prefix gen;
  }tunnel_data;

  struct shimhdr tunnel_label;
  struct shimhdr vpn_label;

  u_int32_t ftn_ix;

  u_int32_t oif_ix;
};

/* NSM_MSG_OAM_LSP_PING_REQ_RESP_ERR */
struct nsm_msg_oam_lsp_ping_req_resp_err
{
  u_int32_t seq_no;
  u_int32_t err_code; 
};

/* NSM_MSG_OAM_LSP_PING_REP_PROCESS_LDP */
struct nsm_msg_oam_lsp_ping_rep_process_ldp
{
  struct ldp_ipv4_prefix data;
  bool_t is_dsmap;
  u_int32_t seq_no;
  struct mpls_oam_ctrl_data ctrl_data;
};

/* NSM_MSG_OAM_LSP_PING_REP_PROCESS_RSVP */
struct nsm_msg_oam_lsp_ping_rep_process_rsvp
{
  struct rsvp_ipv4_prefix data;
  bool_t is_dsmap;
  u_int32_t seq_no;
  struct mpls_oam_ctrl_data ctrl_data;
};

/* NSM_MSG_OAM_LSP_PING_REP_PROCESS_GEN */
struct nsm_msg_oam_lsp_ping_rep_process_gen
{
  struct generic_ipv4_prefix data;
  bool_t is_dsmap;
  u_int32_t seq_no;
  struct mpls_oam_ctrl_data ctrl_data;
};

#ifdef HAVE_MPLS_VC
/* NSM_MSG_OAM_LSP_PING_REP_PROCESS_L2VC */
struct nsm_msg_oam_lsp_ping_rep_process_l2vc
{
  u_int16_t l2_data_type;
  u_int16_t tunnel_type;
  bool_t is_dsmap;
  u_int32_t seq_no;

  union 
  {
    struct l2_circuit_dep pw_128_dep;
    struct l2_circuit_curr pw_128_cur;
    struct mpls_pw pw_129;
  }l2_data;

  union
  {
    struct ldp_ipv4_prefix ldp;
    struct rsvp_ipv4_prefix rsvp;
    struct generic_ipv4_prefix gen;
  }tunnel_data;

  struct mpls_oam_ctrl_data ctrl_data;
};
#endif /* HAVE_MPLS_VC */

/* NSM_MSG_OAM_LSP_PING_REP_PROCESS_L3VPN */
struct nsm_msg_oam_lsp_ping_rep_process_l3vpn
{
  u_int16_t tunnel_type;
  bool_t is_dsmap;
  u_int32_t seq_no;
  struct l3vpn_ipv4_prefix data;

  union
  {
    struct ldp_ipv4_prefix ldp;
    struct rsvp_ipv4_prefix rsvp;
    struct generic_ipv4_prefix gen;
  }tunnel_data;

  struct mpls_oam_ctrl_data ctrl_data;
};

/* NSM_MSG_OAM_LSP_PING_REP_RESPONSE */
struct nsm_msg_oam_lsp_ping_rep_resp
{
  u_int32_t seq_no;
  u_int8_t  err_code;
  u_int8_t  return_code;
  u_int8_t  ret_subcode;
  bool_t is_dsmap; /* Set to TRUE if dsmap is not requested or failed to fill. */
  struct mpls_oam_downstream_map dsmap;
};

enum {
  NSM_MSG_OAM_FTN_UPDATE = 0,
  NSM_MSG_OAM_VC_UPDATE
};

#define NSM_MSG_OAM_UPDATE_TYPE_DELETE (1 << 0)

/* NSM_MSG_OAM_UPDATE */
struct nsm_msg_oam_update
{
  u_char param_type; 
  union
  {
    /* VC Update message to OAM from NSM VC module. */
    struct nsm_mpls_oam_vc_update
    {
      u_int32_t vc_id;
      struct pal_in4_addr peer;
      u_int8_t update_type;
    }vc_update;

    /* FTN update message to OAM from NSM MPLS module. */
    struct nsm_mpls_oam_ftn_update
    {
      u_int32_t ftn_ix;
      u_int8_t update_type; 
    }ftn_update;
  }param;
};

/* DSMAP TLV Defines */
/* Address Family */
#define MPLS_OAM_IPv4_NUMBERED    1
#define MPLS_OAM_IPv4_UNNUMBERED  2
#define MPLS_OAM_IPv6_NUMBERED    3
#define MPLS_OAM_IPv6_UNNUMBERED  4
/* Multipath Types */
#define MPLS_OAM_NO_MULTIPATH      0
#define MPLS_OAM_IP_ADDRESS        2
#define MPLS_OAM_IP_RANGE          4
#define MPLS_OAM_BIT_MAP_ADDR_SET  8
#define MPLS_OAM_BIT_MAP_LBL_SET   9
/* Protocol Types */
#define MPLS_PROTO_UNKNOWN     0
#define MPLS_PROTO_STATIC      1
#define MPLS_PROTO_BGP         2
#define MPLS_PROTO_LDP         3
#define MPLS_PROTO_RSVP_TE     4

/* Error Codes */
#define MPLS_OAM_ERR_PACKET_TOO_SMALL        -100
#define MPLS_OAM_ERR_NO_FTN_FOUND            -101
#define MPLS_OAM_ERR_NO_VPN_FOUND            -102
#define MPLS_OAM_ERR_DECODE_LENGTH_ERROR     -103
#define MPLS_OAM_ERR_UNKNOWN_TLV             -104
#define MPLS_OAM_ERR_EXPLICIT_NULL           -105
#define MPLS_OAM_ERR_ECHO_TIMEOUT            -106
#define MPLS_OAM_ERR_TRACE_LOOP              -107
#define MPLS_OAM_ERR_PACKET_SEND             -108
#define MPLS_OAM_ERR_NO_VC_FOUND             -109
#define MPLS_OAM_ERR_NO_VPLS_FOUND           -110
#define MPLS_OAM_ERR_VCCV_NOT_IN_USE         -111

int
nsm_encode_oam_ctrl_data (u_char **pnt, u_int16_t *size,
                          struct mpls_oam_ctrl_data *msg);

int
nsm_encode_oam_req_process (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_oam_lsp_ping_req_process *msg);

int
nsm_encode_oam_rep_process_ldp (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_rep_process_ldp *msg);

int
nsm_encode_oam_rep_process_rsvp (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_process_rsvp *msg);

int
nsm_encode_oam_rep_process_gen (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_rep_process_gen *msg);

#ifdef HAVE_MPLS_VC
int
nsm_encode_oam_rep_process_l2vc (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg);
#endif /* HAVE_MPLS_VC */

int
nsm_encode_oam_rep_process_l3vpn (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg);

int
nsm_decode_oam_ctrl_data (u_char **pnt, u_int16_t *size,
                          struct mpls_oam_ctrl_data *msg);

int
nsm_decode_oam_req_process (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_oam_lsp_ping_req_process *msg);

int
nsm_decode_oam_rep_process_ldp (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_oam_lsp_ping_rep_process_ldp *msg);

int
nsm_decode_oam_rep_process_rsvp (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_oam_lsp_ping_rep_process_rsvp *msg);

int
nsm_decode_oam_rep_process_gen (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_oam_lsp_ping_rep_process_gen *msg);

#ifdef HAVE_MPLS_VC
int
nsm_decode_oam_rep_process_l2vc (u_char **pnt, u_int16_t *size,
                            struct nsm_msg_oam_lsp_ping_rep_process_l2vc *msg);
#endif /* HAVE_MPLS_VC */

int
nsm_decode_oam_rep_process_l3vpn (u_char **pnt, u_int16_t *size,
                           struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *msg);

int
nsm_parse_mpls_oam_req_process (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_header *header, void *arg,
                                NSM_CALLBACK callback);

int
nsm_parse_mpls_oam_rep_process_ldp (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_header *header, void *arg,
                                    NSM_CALLBACK callback);

int
nsm_parse_mpls_oam_rep_process_rsvp (u_char **pnt, u_int16_t *size,
                                     struct nsm_msg_header *header, void *arg,
                                     NSM_CALLBACK callback);

int
nsm_parse_mpls_oam_rep_process_gen (u_char **pnt, u_int16_t *size,
                                    struct nsm_msg_header *header, void *arg,
                                    NSM_CALLBACK callback);

#ifdef HAVE_MPLS_VC
int
nsm_parse_mpls_oam_rep_process_l2vc (u_char **pnt, u_int16_t *size,
                                     struct nsm_msg_header *header, void *arg,
                                     NSM_CALLBACK callback);
#endif /* HAVE_MPLS_VC */

int
nsm_parse_mpls_oam_rep_process_l3vpn (u_char **pnt, u_int16_t *size,
                                      struct nsm_msg_header *header, void *arg,
                                      NSM_CALLBACK callback);

int
nsm_encode_oam_dsmap_data (u_char **pnt, u_int16_t *size,
                               struct mpls_oam_downstream_map *msg);

int
nsm_encode_oam_ldp_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct ldp_ipv4_prefix *msg);

int
nsm_encode_oam_ping_req_resp_ldp (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_ldp *msg);

int
nsm_encode_oam_rsvp_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct rsvp_ipv4_prefix *msg);

int
nsm_encode_oam_ping_req_resp_rsvp (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_oam_lsp_ping_req_resp_rsvp *msg);

int
nsm_encode_oam_gen_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct generic_ipv4_prefix *msg);

int
nsm_encode_oam_ping_req_resp_gen (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_gen *msg);

#ifdef HAVE_MPLS_VC
int
nsm_encode_oam_l2vc_prefix_dep (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_dep *msg);

int
nsm_encode_oam_l2vc_prefix_cur (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_curr *msg);
#endif /* HAVE_MPLS_VC */

int
nsm_encode_oam_gen_pw_id_prefix (u_char **pnt, u_int16_t *size,
                                 struct mpls_pw *msg);

#ifdef HAVE_MPLS_VC
int
nsm_encode_oam_ping_req_resp_l2vc (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_oam_lsp_ping_req_resp_l2vc *msg);
#endif /* HAVE_MPLS_VC */

int
nsm_encode_oam_l3vpn_data (u_char **pnt, u_int16_t *size,
                               struct l3vpn_ipv4_prefix *msg);

int
nsm_encode_oam_ping_req_resp_l3vpn (u_char **pnt, u_int16_t *size,
                               struct nsm_msg_oam_lsp_ping_req_resp_l3vpn *msg);

int
nsm_encode_oam_ping_req_resp_err (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_err *msg);

int
nsm_encode_oam_ping_rep_resp (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_resp *msg);

int
nsm_encode_oam_update (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_oam_update *msg);

int
nsm_decode_oam_dsmap_data (u_char **pnt, u_int16_t *size,
                               struct mpls_oam_downstream_map *msg);

int
nsm_decode_oam_ldp_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct ldp_ipv4_prefix *msg);

int
nsm_decode_oam_ping_req_resp_ldp (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_ldp *msg);

int
nsm_decode_oam_rsvp_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct rsvp_ipv4_prefix *msg);

int
nsm_decode_oam_ping_req_resp_rsvp (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_oam_lsp_ping_req_resp_rsvp *msg);

int
nsm_decode_oam_gen_ipv4_data (u_char **pnt, u_int16_t *size,
                             struct generic_ipv4_prefix *msg);

int
nsm_decode_oam_ping_req_resp_gen (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_gen *msg);

#ifdef HAVE_MPLS_VC
int
nsm_decode_oam_l2vc_prefix_dep (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_dep *msg);

int
nsm_decode_oam_l2vc_prefix_cur (u_char **pnt, u_int16_t *size,
                                struct l2_circuit_curr *msg);
#endif /* HAVE_MPLS_VC */

int
nsm_decode_oam_gen_pw_id_prefix (u_char **pnt, u_int16_t *size,
                                 struct mpls_pw *msg);

#ifdef HAVE_MPLS_VC
int
nsm_decode_oam_ping_req_resp_l2vc (u_char **pnt, u_int16_t *size,
                                struct nsm_msg_oam_lsp_ping_req_resp_l2vc *msg);
#endif /* HAVE_MPLS_VC */

int
nsm_decode_oam_l3vpn_data (u_char **pnt, u_int16_t *size,
                               struct l3vpn_ipv4_prefix *msg);

int
nsm_decode_oam_ping_req_resp_l3vpn (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_req_resp_l3vpn *msg);

int
nsm_decode_oam_ping_req_resp_err (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_oam_lsp_ping_req_resp_err *msg);

int
nsm_decode_oam_ping_rep_resp (u_char **pnt, u_int16_t *size,
                              struct nsm_msg_oam_lsp_ping_rep_resp *msg);

int
nsm_decode_oam_update (u_char **pnt, u_int16_t *size,
                       struct nsm_msg_oam_update *msg);

int
nsm_parse_oam_ping_req_resp_ldp (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback);

int
nsm_parse_oam_ping_req_resp_rsvp (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_header *header, void *arg,
                                  NSM_CALLBACK callback);

int
nsm_parse_oam_ping_req_resp_gen (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback);

#ifdef HAVE_MPLS_VC
int
nsm_parse_oam_ping_req_resp_l2vc (u_char **pnt, u_int16_t *size,
                                  struct nsm_msg_header *header, void *arg,
                                  NSM_CALLBACK callback);
#endif /* HAVE_MPLS_VC */

int
nsm_parse_oam_ping_req_resp_l3vpn (u_char **pnt, u_int16_t *size,
                                   struct nsm_msg_header *header, void *arg,
                                   NSM_CALLBACK callback);

int
nsm_parse_oam_ping_req_resp_err (u_char **pnt, u_int16_t *size,
                                 struct nsm_msg_header *header, void *arg,
                                 NSM_CALLBACK callback);

int
nsm_parse_oam_ping_rep_resp (u_char **pnt, u_int16_t *size,
                             struct nsm_msg_header *header, void *arg,
                             NSM_CALLBACK callback);

int
nsm_parse_oam_update (u_char **pnt, u_int16_t *size,
                      struct nsm_msg_header *header, void *arg,
                      NSM_CALLBACK callback);
#endif /* HAVE_MPLS_OAM */
#endif /* _PACOS_OAM_MPLS_MSG_H */

