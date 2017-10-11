/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_MPLS_COMMON_H
#define _PACOS_MPLS_COMMON_H

#ifndef INTERFACE_NAMSIZ
#define INTERFACE_NAMSIZ 20
#endif /* INTERFACE_NAMSIZ */

/* Invalid label. */
#ifndef    LABEL_VALUE_INVALID
#define    LABEL_VALUE_INVALID    1048576
#endif     /* LABEL_VALUE_INVALID */

/* Invalid Virtual Circuit. */
#define    VC_INVALID             0

/* Sequence number. */
#define    MPLS_SEQ_NUM           0xdeadbeef

/* Netlink message types. */
#define    USERSOCK_MPLS_INIT     0
#define    USERSOCK_MPLS_IF_INIT  1
#define    USERSOCK_MPLS_VRF_INIT 2
#define    USERSOCK_NEWILM        3
#define    USERSOCK_DELILM        4
#define    USERSOCK_NEWFTN        5
#define    USERSOCK_DELFTN        6
#define    USERSOCK_FIB_CLEAN     7
#define    USERSOCK_VRF_CLEAN     8
#define    USERSOCK_IF_UPDATE     9
#define    USERSOCK_MPLS_VRF_END  10
#define    USERSOCK_MPLS_IF_END   11
#define    USERSOCK_MPLS_END      12
#define    USERSOCK_TTL_HANDLING  13
#define    USERSOCK_LOCAL_PKT     14
#define    USERSOCK_DEBUGGING     15
#define    USERSOCK_MPLS_VC_INIT  16
#define    USERSOCK_MPLS_VC_END   17
#define    USERSOCK_NEW_VC_FTN    18
#define    USERSOCK_DEL_VC_FTN    19
#define    USERSOCK_STATS_CLEAR   20
#define    USERSOCK_BYPASS_SEND   21
#define    USERSOCK_OAM_SEND      22
#define    USERSOCK_OAM_VC_SEND   23

/* MPLS Control Types */
#define    MPLS_END               0
#define    MPLS_INIT              1

#define    GLOBAL_TABLE           -1

/* Kernel message types */
#define    KERN_MSG_ERROR         (1 << 0)
#define    KERN_MSG_WARNING       (1 << 1)
#define    KERN_MSG_DEBUG         (1 << 2)
#define    KERN_MSG_NOTICE        (1 << 3)
#define    KERN_MSG_MAX           (1 << 4)

/* LSP Models */
#define    LSP_MODEL_UNIFORM      0
#define    LSP_MODEL_PIPE         1

/* TTL Propagation MACROs */
#define    TTL_PROPAGATE_DISABLED   0
#define    TTL_PROPAGATE_ENABLED    1

/* MAximum TTL Value */
#define    MAX_TTL                 255

/*prototypes*/

struct ilm_entry;
struct if_dent;

#define MAX_LABEL_STACK_DEPTH     4

/* Prefix structure for IPv4 FEC. */
struct mpls_prefix
{
  u_char family;
  u_char prefixlen;
  union
  {
    u_char prefix;
    struct pal_in4_addr prefix4;
  } u;
};

/* Interface identifier structure. */
struct if_ident
{
  u_int32_t if_index;
  char if_name[INTERFACE_NAMSIZ + 1];
};

#ifdef HAVE_DIFFSERV

struct ds_info_fwd
{
  u_char lsp_type;

  /* DSCP-to-EXP mapping for ELSP. */
  u_char dscp_exp_map[8];

  /* DSCP value for LLSP. */
  u_char dscp;

  /* AF set for LLSP. */
  u_char af_set;
};

#endif /* HAVE_DIFFSERV */

/* Key used by RSVP-TE protocol for IPV4 */
struct rsvp_key_ipv4_fwd
{
  u_int16_t trunk_id;
  u_int16_t lsp_id;
  struct pal_in4_addr ingr;
  struct pal_in4_addr egr;
};

#ifdef HAVE_IPV6
struct rsvp_key_ipv6_fwd
{
  u_int16_t trunk_id;
  u_int16_t lsp_id;
  struct pal_in6_addr ingr;
  struct pal_in6_addr egr;
};
#endif

/* Generic RSVP_TE Key for MPLS forwarder */
struct rsvp_key_fwd
{
  u_char afi;
  u_int16_t len;
  union
  {
    u_char key;
    struct rsvp_key_ipv4_fwd ipv4;
#ifdef HAVE_IPV6
    struct rsvp_key_ipv6_fwd ipv6;
#endif
  } u;
};

struct mpls_owner_fwd
{
  /* APN_PROTO_xxx */
  u_char protocol;
  union
  {
    struct rsvp_key_fwd r_key;
  } u;
};

struct mpls_nh_fwd
{
  u_char afi;
  union
  {
    u_char key;
    struct pal_in4_addr ipv4;
#ifdef HAVE_IPV6
    struct pal_in6_addr ipv6;
#endif
  } u;
};

/*
 * defines for function return types and/or status of various flag bits
 */
#define FAILURE               -1
#define SUCCESS                0
#define ARP_WAIT               2
#define MPLS_TRUE              1
#define MPLS_FALSE             0
#define INVALID               -1
#define VALID                  1
#define ENABLED                MPLS_TRUE
#define DISABLED               MPLS_FALSE
#define DONT_CARE             -1
#define NOT_FOUND              0
#define ENABLE                 ENABLED
#define DISABLE                DISABLED
#define SHIM_HLEN              4
#define PW_ACH_LEN             4
#define ETH_HLEN2              16
#define IPV4                   4
#define IPV6                   6
#define UNKNOWN_TYPE          -2
#define MSG_PROCESS_ERR       -3
#define VALID                  1
#define INVALID               -1
#define ETHFRAME               1514
#define ETHMTU                 1500
#define DO_SECOND_FTN_LOOKUP   1
#define DO_IP_RCV              2
#define MPLS_RET_TTL_EXPIRED   3
#define MPLS_RET_DST_LOOPBACK  4
#define INVALID_FTN_IX         0
#define MPLS_PKT_SIZE_MAX      4000 

/*
 * Possible label opcodes
 */
#define PUSH                   1
#define POP                    2
#define SWAP                   3
#define POP_FOR_VPN            4
#define DLVR_TO_IP             5
#define PUSH_AND_LOOKUP        6
#define PUSH_FOR_VC            7
#define PUSH_AND_LOOKUP_FOR_VC 8
#define POP_FOR_VC             9
#define SWAP_AND_LOOKUP        10
#define MPLS_NO_OP             11
#define FTN_LOOKUP             12


struct mpls_control
{
  /* The protocol (LDP or BGP etc.) that sent this request */
  u_char protocol;
};

struct mpls_ttl_handling
{
  /* The protocol (LDP or BGP etc.) that sent this request */
  u_char protocol;

  u_char type;
#define MPLS_TTL_VALUE_SET         1
#define MPLS_TTL_PROPAGATE_SET     2
#define MPLS_TTL_PROPAGATE_UNSET   3
#define MPLS_LSP_MODEL_PIPE_SET    4
#define MPLS_LSP_MODEL_PIPE_UNSET  5

  /* Ingress or not */
  u_char is_ingress;

  /* New ttl value */
  int new_ttl;
};

struct mpls_conf_handle
{
  /* The protocol (LDP or BGP etc.) that sent this request */
  u_char protocol;

  /* Message type */
  u_char msg_type;

  /* Enable attribute */
  int enable;
};

struct mpls_stat_clear_handle
{
  /* The protocol (LDP or BGP etc.) that sent this request */
  u_char protocol;

  /* Clear type. */
#define MPLS_STAT_CLEAR_TYPE_ALL        0
#define MPLS_STAT_CLEAR_TYPE_TOP        1
#define MPLS_STAT_CLEAR_TYPE_FTN        2
#define MPLS_STAT_CLEAR_TYPE_ILM        3
  u_char type;
};

struct mpls_if
{
  /* Interface index */
  u_int32_t ifindex;

  /* Interface name */
  char ifname[INTERFACE_NAMSIZ + 1];
  
  /* Label space for this interface */
  u_int32_t label_space;
  
  /* VRF table identifier */
  int vrf_ident;

  /* VC identifier. */
  u_int32_t vc_id;
};

struct ilm_add_struct
{
  /* First byte should denote the address family, AF_INET.  */
  u_char rtnl_family;
  
  /* Protocol type */
  u_char protocol;
  
  /* Incoming label */
  int in_label;
  
  /* Incoming interface index */
  u_int32_t inl2id;
  
  /* Name of the incoming interface, eg "eth0" */
  char inl2name[INTERFACE_NAMSIZ + 1];   

  /* FEC for which this ILM entry was created */
  struct mpls_prefix fec;

  /* Nexthop for this ILM */
  u_int32_t nexthop;

  /* Name of the outgoing interface, eg "eth0" */
  char outintf[INTERFACE_NAMSIZ + 1];

  /* Outgoing interface id. Use outintf/outlabel as per convenience. */
  u_int32_t outl2id;            

  /* Outgoing label */                                           
  int out_label;

  /* OPCODE to be used */
  char opcode;

  /* NHLFE index */
  u_int32_t nhlfe_ix;

  /* TE Tunnel key value */
  struct mpls_owner_fwd owner;

};

struct ilm_del_struct
{
  /* First byte should denote the address family, AF_INET.  */
  u_char rtnl_family;

  /* Protocol type */
  u_char protocol;

  /* Incoming label */
  int in_label;

  /* Incoming interface index */
  u_int32_t inl2id;

  /* Name of the incoming interface, eg "eth0" */
  char inl2name[INTERFACE_NAMSIZ + 1];  
};

struct ftn_add_struct
{
  /* First byte should denote the address family, AF_INET. */
  u_char rtnl_family;

  /* VRF table identifier */
  int vrf;

  /* Protocol type */
  u_char protocol;

  /* FEC for which this FTN was created */
  struct mpls_prefix fec;

  /* Next hop */
  u_int32_t nexthop;

  /* Name of the outgoing interface, eg "eth0" */
  char outintf[INTERFACE_NAMSIZ + 1];

  /* Outgoing interface id. Use outintf/outlabel as per convenience. */
  u_int32_t outl2id;
                
  /* Outgoing label */
  int out_label;

  /* OPCODE to be used */
  char opcode;

  /* NHLFE index */
  u_int32_t nhlfe_ix;

  /* NHLFE index */
  u_int32_t ftn_ix;

  /* TE Tunnel key value */
  struct mpls_owner_fwd owner;

  /*Bypass FTN index */
  u_int32_t bypass_ftn_ix;

  /*LSP type*/
  u_char lsp_type;

  int active_head;
};

struct ftn_del_struct
{
  /* First byte should denote the address family, AF_INET.  */
  u_char rtnl_family;

  /* VRF table identifier */
  int vrf;

  /* Protocol type */
  u_char protocol;

  /* FEC for which this FTN was created */
  struct mpls_prefix fec;

  /* FTN index */
  u_int32_t ftn_ix;

};

struct vc_ftn_add_struct
{
  /* First byte should denote the address family, AF_INET. */
  u_char rtnl_family;

  /* Protocol type. */
  u_char protocol;

  /* Virtual Circuit id. */
  unsigned int vc_id;

  /* Outgoing label. */
  int out_label;

  /* Next hop. */
  u_int32_t nexthop;

  /* Name of the incoming interface, eg "eth0". */
  char inintf[INTERFACE_NAMSIZ + 1];

  /* Incoming interface id. Use outintf/outlabel as per convenience. */
  u_int32_t inl2id;

  /* Name of the outgoing interface, eg "eth0". */
  char outintf[INTERFACE_NAMSIZ + 1];

  /* Outgoing interface id. Use outintf/outlabel as per convenience. */
  u_int32_t outl2id;
                
  /* OPCODE to be used */
  char opcode;

  u_int32_t tunnel_ftnix;
};

struct vc_ftn_del_struct
{
  /* First byte should denote the address family, AF_INET.  */
  u_char rtnl_family;

  /* Protocol type. */
  u_char protocol;

  /* Virtual Circuit id. */
  unsigned int vc_id;

  /* Name of the incoming interface, eg "eth0". */
  char inintf[INTERFACE_NAMSIZ + 1];

  /* Incoming interface id. Use outintf/outlabel as per convenience. */
  u_int32_t inl2id;
};

struct mpls_oam_ctrl_data 
{
  struct pal_in4_addr src_addr;
  u_int32_t in_ifindex;
  u_int32_t label_stack_depth;
  u_int32_t label_stack[MAX_LABEL_STACK_DEPTH];
  u_int32_t input1;
  u_int32_t input2;
};

/* Macros for PW ACH Channel Types */
#define MPLS_PW_ACH_IPV4                 0x21
#define MPLS_PW_ACH_IPV6                 0x57
#define MPLS_PW_ACH_BFD                  0x07

 /* BFD VCCV CV Encapsulation Types */
enum bfd_encap
{
   BFD_CV_RAW_ENCAP = 1,
   BFD_CV_IP_UDP_ENCAP,
};

#define MPLS_OAM                          (1)
#define BFD_OAM                           (2)

typedef int (*OAM_NETLINK_CALLBACK) (u_char *buf, int len, struct mpls_oam_ctrl_data *, u_int8_t pkt_type);  

#define MPLSONM_FWD_OPTION_NOPHP         (1 << 0)
#define MPLSONM_FWD_OPTION_LABEL         (1 << 1)

enum
{
    CC_TYPE_NONE = 0,
    CC_TYPE_1,
    CC_TYPE_2,
    CC_TYPE_3
};

enum
{
    CV_TYPE_NONE =0,
    CV_TYPE_ICMP_PING,
    CV_TYPE_LSP_PING,
    CV_TYPE_BFD_IPUDP_DET,
    CV_TYPE_BFD_IPUDP_DET_SIG,
    CV_TYPE_BFD_ACH_DET,
    CV_TYPE_BFD_ACH_DET_SIG
};
  
/* Flag manipulation macros. */
#define CHECK_FLAG(V,F)      ((V) & (F))
#define SET_FLAG(V,F)        (V) = (V) | (F)
#define UNSET_FLAG(V,F)      (V) = (V) & ~(F)
#define FLAG_ISSET(V,F)      (((V) & (F)) == (F))

#define IS_PW_ACH(V) (((V) & 0xF0) == 0x10)

/* PWE3 Control Word with 0001b as first nibble */
#define CC_TYPE_1_BIT             (0x1 << 0) 
/* MPLS Router Alert Label */
#define CC_TYPE_2_BIT             (0x1 << 1) 
/* MPLS PW Label with TTL == 1 */
#define CC_TYPE_3_BIT             (0x1 << 2) 

/* CV types */
#define CV_TYPE_ICMP_PING_BIT          (0x1 << 0) /* ICMP Ping */
#define CV_TYPE_LSP_PING_BIT           (0x1 << 1) /* LSP Ping */

/* BFD IP/UDP-encapsulated, for PW Fault Detection only  */
#define CV_TYPE_BFD_IPUDP_DET_BIT      (0x1 << 2) 
/* BFD IP/UDP-encapsulated, for PW Fault Detection and AC/PW Fault 
 * Status Signaling */
#define CV_TYPE_BFD_IPUDP_DET_SIG_BIT  (0x1 << 3) 
/* BFD PW-ACH-encapsulated, for PW Fault Detection only */
#define CV_TYPE_BFD_ACH_DET_BIT        (0x1 << 4) 
/* BFD PW-ACH-encapsulated, for PW Fault Detection and AC/PW Fault
 * Status Signaling */
#define CV_TYPE_BFD_ACH_DET_SIG_BIT    (0x1 << 5) 

#endif /* _PACOS_MPLS_COMMON_H */
