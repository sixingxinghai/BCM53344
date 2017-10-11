/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NSM_MPLS_OAM_H
#define _PACOS_NSM_MPLS_OAM_H

extern bool_t oam_reply_tlv;
extern bool_t oam_req_dsmap_tlv;

#define MPLS_OAM_DEFAULT_PORT_UDP 3503
#define IPOPT_RA_LEN 4
#define MPLS_OAM_PDU_SIZE 200
#define UDP_PSEUDO_HEADER_SIZE 12
#define MPLS_ECHO     0
#define MPLS_REPLY    1
#define TRACERT_REPLY 2

#ifndef INADDR_ALLRTRS_GROUP
#define INADDR_ALLRTRS_GROUP           0xe0000002U /* 224.0.0.2 */
#endif /* #ifndef INADDR_ALLRTRS_GROUP */

#define MPLS_OAM_VERSION 1
#define VPN_RD_SAME(RD1, RD2)                                         \
  (! pal_mem_cmp ((RD1), (RD2), VPN_RD_SIZE))

/* OAM TLV Types */
#define MPLS_OAM_FEC_TLV        1
#define MPLS_OAM_DSMAP_TLV      2
#define MPLS_OAM_PAD_TLV        3
#define MPLS_OAM_VENDOR_NUMBER  5
#define MPLS_OAM_IF_LBL_STACK   7
#define MPLS_OAM_ERROR_TLVS     9
#define MPLS_OAM_REPLY_TOS_TLV 10
#define MPLS_OAM_MAX_TLV_VALUE 32768

/* VENDOR PRIVATE TLV definitions */
#define MPLS_OAM_VENDOR_NUMBER1 31744
#define MPLS_OAM_VENDOR_NUMBER2 32767
#define MPLS_OAM_VENDOR_NUMBER3 64512
#define MPLS_OAM_VENDOR_NUMBER4 65535

/* OAM FEC Sub TLV Types - Only Supported values defined */
#define MPLS_OAM_LDP_IPv4_PREFIX_TLV     1
#define MPLS_OAM_RSVP_IPv4_PREFIX_TLV    3
#define MPLS_OAM_IPv4_VPN_TLV            6
#define MPLS_OAM_FEC128_DEP_TLV          9
#define MPLS_OAM_FEC128_CURR_TLV        10
#define MPLS_OAM_FEC129_PW_TLV          11
#define MPLS_OAM_GENERIC_FEC_TLV        15
#define MPLS_OAM_NIL_FEC_TLV            16
 
/* Length Values */
#define MPLS_OAM_LDP_IPv4_FIX_LEN        5
#define MPLS_OAM_RSVP_IPv4_FIX_LEN      20 
#define MPLS_OAM_IPv4_VPN_FIX_LEN       13
#define MPLS_OAM_FEC128_DEP_FIX_LEN     10
#define MPLS_OAM_FEC128_CURR_FIX_LEN    14
#define MPLS_OAM_FEC129_PW_MIN_LEN      16
#define MPLS_OAM_GENERIC_IPv4_FIX_LEN    5
#define MPLS_OAM_NIL_FEC_FIX_LEN         4
#define MPLS_OAM_PAD_LEN_BY(S,V) (S =(S + V))
#define MPLS_OAM_REM_PAD(S,V) (S = (S - V))


/* Internal Length Values */
#define MPLS_OAM_HDR_LEN                  32
#define MPLS_OAM_TLV_HDR_FIX_LEN           4
#define MPLS_OAM_FEC_TLV_MIN_LEN          12  
#define MPLS_OAM_DSMAP_TLV_IPv4_FIX_LEN   16
#define MPLS_OAM_DSMAP_TLV_IPv6_FIX_LEN   40
#define MPLS_OAM_DSMAP_TLV_IPv6z_FIX_LEN  28
#define MPLS_OAM_ECHO_REQUEST_MIN_SIZE    48
#define MPLS_OAM_ECHO_REQ_SIZE_WITH_DSMAP 68
#define MPLS_OAM_REPLY_TOS_TLV_FIX_LEN     4
#define MPLS_OAM_IFLABEL_TLV_IPv4_FIX_LEN 12
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

#define DSMAP_IPV4_NUMBERED_PAD     8
#define DSMAP_IPV6_NUMBERED_PAD     32
#define DSMAP_IPV6_UNNUMBERED_PAD   20

#define IFLABEL_IPV4_NUMBERED_PAD     8
#define IFLABEL_IPV6_NUMBERED_PAD     32
#define IFLABEL_IPV6_UNNUMBERED_PAD   20
#define DSMAP_LABEL_DATA_LEN   4
#define GET_ENCODE_PAD_TLV_4(l)                     \
         (((l) % 4) ?                              \
          ((((l) + 4) - ((l) %4))- (l)) : 0)

#define MPLS_OAM_PAD_TLV_2      2
#define MPLS_OAM_PAD_TLV_3      3
#define FEC_FLAGS_SIZE          16
#define OAM_DATA_SIZE           200

#define OAM_IPOPT_RA      148
#define OAM_IPOPT_RA_LEN  4

/* Main Struct */
struct mpls_oam_hdr
{
  u_int16_t ver_no;
  /* Global Flags Field */
  union
  {
    struct global_flags
    {
      u_int16_t resvd :15;
      u_int16_t fec :1;
    }flags;
    u_int16_t l_packet;
  }u;

#define MPLS_ECHO_REQUEST_MESSAGE 1
#define MPLS_ECHO_REPLY_MESSAGE   2
  u_int8_t msg_type;

#define MPLS_OAM_DEFAULT_REPLY_MODE  1
#define MPLS_OAM_IP_UDP_REPLY        2
#define MPLS_OAM_IP_UDP_RA_REPLY     3
#define MPLS_OAM_APP_CHANNEL_REPLY   4
  u_int8_t reply_mode;

  u_int8_t return_code;

  u_int8_t ret_subcode;
  /* Filled by Sender and Copied into Reply message */
  u_int32_t sender_handle;
  u_int32_t seq_number;
  u_int32_t ts_tx_sec; 
  u_int32_t ts_tx_usec;
  /* Time when Recevier gets the packet */
  u_int32_t ts_rx_sec;
  u_int32_t ts_rx_usec;
};

struct mpls_oam_tlv_hdr
{
  u_int16_t type;
  u_int16_t length;
};

struct mpls_label_stack
{
  u_int32_t seq_num;
  u_int32_t ifindex;
  
  struct list *label;
};

struct prefix_length
{
  u_int32_t len:8;
  u_int32_t mbz:24;
}val;

struct ldp_ipv4_prefix 
{
  struct mpls_oam_tlv_hdr tlv_h;

  struct pal_in4_addr ip_addr;
  union 
  {
    struct prefix_length val;
    u_int32_t l_packet;
  }u; 
};

struct rsvp_ipv4_prefix
{
  struct mpls_oam_tlv_hdr tlv_h;

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
  struct mpls_oam_tlv_hdr tlv_h;

  /* Route Distingusher value. Not really used in Processing */
  struct vpn_rd rd; 
  struct pal_in4_addr vpn_pfx;
  union
  {
    struct prefix_length val;
    u_int32_t l_packet;
  }u;
};
  
struct l2_circuit_tlv_dep
{
  struct mpls_oam_tlv_hdr tlv_h;
 
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

struct l2_circuit_tlv_curr
{
  struct mpls_oam_tlv_hdr tlv_h;

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

struct mpls_pw_tlv 
{
  struct mpls_oam_tlv_hdr tlv_h;

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

struct generic_ipv4_prefix
{
  struct mpls_oam_tlv_hdr tlv_h;
  
  struct pal_in4_addr ipv4_addr;
  union
  {
    struct prefix_length val;
    u_int32_t l_packet;
  }u;
};
  
struct nil_fec_prefix
{
  struct mpls_oam_tlv_hdr tlv_h;
  union
  {
    struct label_val
    {
      u_int32_t label :20;
      u_int32_t mbz :12;
    }bits;
    u_int32_t l_packet;
  }u;
};

struct mpls_oam_target_fec_stack
{
  struct mpls_oam_tlv_hdr tlv_h;

/* flags is set to indicate which FEC's are to be encoded */
  u_int16_t flags;
#define MPLS_OAM_LDP_IPv4_PREFIX        (1 << 0)
#define MPLS_OAM_RSVP_IPv4_PREFIX       (1 << 1)
#define MPLS_OAM_L2_CIRCUIT_PREFIX_DEP  (1 << 2)
#define MPLS_OAM_L2_CIRCUIT_PREFIX_CURR (1 << 3)
#define MPLS_OAM_GEN_PW_ID_PREFIX       (1 << 4)
#define MPLS_OAM_MPLS_VPN_PREFIX        (1 << 5)
#define MPLS_OAM_GENERIC_IPv4_PREFIX    (1 << 6)
#define MPLS_OAM_NIL_FEC_PREFIX         (1 << 7) 

  struct ldp_ipv4_prefix ldp;
  struct rsvp_ipv4_prefix rsvp;
  struct l3vpn_ipv4_prefix l3vpn;
/* Please note that this Deprecated TLV can only be Received 
 * It SHOULD NOT be sent. Use the Current TLV for REPLIES 
 */
  struct l2_circuit_tlv_dep l2_dep;
  struct l2_circuit_tlv_curr l2_curr;
  struct mpls_pw_tlv pw_tlv;
  struct generic_ipv4_prefix ipv4;
  struct nil_fec_prefix nil_fec;
};

struct ds_flags
 {
  u_int8_t resvd :6;
  u_int8_t i_bit :1;
  u_int8_t n_bit :1;
  u_int8_t c_packet;
 };


struct mpls_oam_downstream_map
{
  struct mpls_oam_tlv_hdr tlv_h;
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

struct oam_ds_label {
  union  {
     struct ds_label {
     u_int32_t label:20;
     u_int32_t exp:3;
     u_int32_t s:1;
     u_int32_t protocol:8;
     }lb;
    u_int32_t l_packet;
  }u;
};
struct mpls_oam_reply_tos
{
  struct mpls_oam_tlv_hdr tlv_h;
  union 
  {
    struct reply_byte
    {
      u_int32_t tos_byte :8;
      u_int32_t mbz :24;
    }tos;
   u_int32_t l_packet;
  }u;
};

struct mpls_oam_if_lbl_stk
{
  struct mpls_oam_tlv_hdr tlv_h;
  
  union 
  {
    struct family
    {
      u_int32_t family:8;
      u_int32_t mbz :24;
    }fam;
    u_int32_t l_packet;
  }u;

  struct pal_in4_addr addr;
  u_int32_t ifindex;

  u_int32_t depth;  
  u_int32_t out_stack[MAX_LABEL_STACK_DEPTH];;
};

struct mpls_oam_errored_tlv
{
  struct mpls_oam_tlv_hdr tlv_h;

  u_char *buf;
};

struct mpls_oam_pad_tlv
{
  struct mpls_oam_tlv_hdr tlv_h;

  u_char val;
#define MPLS_OAM_PAD_VAL_DROP     1
#define MPLS_OAM_PAD_VAL_COPY     2
#define MPLS_OAM_PAD_VAL_RESV     3
};

/* Echo Request Structure */
struct mpls_oam_echo_request
{ 
  struct mpls_oam_hdr omh;    
  struct mpls_oam_target_fec_stack fec_tlv;
  
  u_char flags;
#define MPLS_OAM_FLAGS_DSMAP_TLV              (1 << 0)
#define MPLS_OAM_FLAGS_REPLY_TOS_TLV          (1 << 1)
#define MPLS_OAM_FLAGS_PAD_TLV                (1 << 2)

  struct mpls_oam_downstream_map dsmap;
  struct mpls_oam_reply_tos tos_reply;
  struct mpls_oam_pad_tlv pad_tlv;
};


struct nsm_mpls_oam 
{
  int max_ttl;
  /* Backpointer to required structures */
  struct nsm_master *nm;
  struct thread *oam_echo_interval;
  struct nsm_msg_mpls_ping_req msg;

  /* IP/ UDP header details */ 
  struct pal_in4_header iph;
  struct pal_udp_header udph;
 
  struct mpls_oam_echo_request req; 
  /* FTN Entry pointer for Label Information */
  struct ftn_entry *ftn;

#ifdef HAVE_VRF
  /* NSM VRF Pointer */
  struct nsm_vrf *n_vrf;
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS_VC
  /* Union of VC/VPLS Data */
  union
  {
    struct nsm_mpls_circuit *vc;
#ifdef HAVE_VPLS
    struct nsm_vpls *vpls;
#endif /* HAVE_VPLS */
  }u;
#endif /* HAVE_MPLS_VC */

  /* Store the Count of pings sent. */ 
  u_int32_t count;
  struct thread *pkt_interval;
 
  /* Socket id to identify OAM Utility instance */
  pal_sock_handle_t sock; 
  
  /* Source Address of last replying Router */
  struct pal_in4_addr r_src;
  bool_t trace_timeout;
 
  /* NSM Client id*/
  struct nsm_server_entry *nse;
  struct list *oam_req_list;
  u_int8_t fwd_flags;
};

/* Operating Structure for MPLS OAM */
struct nsm_mpls_oam_data
{
  /* Back pointer to OAM Main List */
  struct nsm_mpls_oam *oam; 
  u_int32_t pkt_count;

  struct thread *pkt_timeout;
  bool_t state;
  
  struct pal_timeval sent_time;
  struct pal_timeval recv_time; 
};

/* Echo Reply Structure */
struct mpls_oam_echo_reply
{
  bool_t state;

  struct mpls_oam_hdr omh;
  struct mpls_oam_target_fec_stack fec_tlv;
  
  u_char flags;
#define MPLS_OAM_FLAGS_IF_LBL_STK_TLV          (1 << 3)
#define MPLS_OAM_FLAGS_ERR_TLV                 (1 << 4)

  struct mpls_oam_downstream_map dsmap;
  struct mpls_oam_if_lbl_stk if_lbl_stk;
  struct mpls_oam_errored_tlv err_tlv;
  struct mpls_oam_pad_tlv pad_tlv;
};


#ifdef HAVE_MPLS_OAM_ITUT

#define MPLS_OAM_LABEL_LEN              5 
#define MPLS_OAM_ITUT_REQUEST_MIN_LEN   44
#define MPLS_OAM_ITUT_ALERT_LABEL       14
#define MPLS_OAM_ITUT_TIME_OUT          3

#define MPLS_OAM_CV_PROCESS_MESSAGE     1
#define MPLS_OAM_FDI_PROCESS_MESSAGE    2
#define MPLS_OAM_BDI_PROCESS_MESSAGE    3
#define MPLS_OAM_FFD_PROCESS_MESSAGE    7



#define MPLS_DEFECT_TYPE_DSERVER        0x0101
#define MPLS_DEFECT_TYPE_DPEERME        0x0102
#define MPLS_DEFECT_TYPE_DLOCV          0x0201
#define MPLS_DEFECT_TYPE_DTTSI_MISMATCH 0x0202
#define MPLS_DEFECT_TYPE_DTTSI_MISMERGE 0x0203
#define MPLS_DEFECT_TYPE_DEXCESS        0x0204
#define MPLS_DEFECT_TYPE_DUNKNOWN       0x02FF

#define MPLS_ETHER_TYPE                 0x8847


struct mpls_oam_eth_header
{
  u_char dmac[6];
  u_char smac[6];
  u_int16_t type;
};


struct oam_label 
{
 union 
  {
  #if 0 
   struct oam_alert_label 
   {
     BITFIELDS3(u_int32_t label:20,
                u_int32_t exp:3,
                u_int32_t s:1/*,
                u_int32_t ttl:8*/);
   }bits;
  #endif
   u_int32_t l_packet;
  }u;
  u_int8_t  pkt_type;
};

struct nsm_mpls_oam_itut
{
  struct nsm_master *nm;
  struct nsm_msg_mpls_oam_itut_req msg;

  u_int8_t function_type;
  u_int16_t defect_type;
 
  struct ttsi
  {
     u_int16_t lsrpad;
     struct pal_in4_addr lsr_addr;
     u_int32_t lsp_id;
  } lsp_ttsi;
 
  u_int16_t  frequency; /* in case of FFD*/
  u_int32_t defect_location;/* in case of FDI/BDI packets*/

  u_int16_t bip16;

  /* FTN entry pointer for Label Information */
  struct ftn_entry *ftn;
  struct thread *oam_pkt_interval;

  /* Store the count of packets sent */
  u_int16_t count;

  /* Socket id to identify OAM utility instance*/
  pal_sock_handle_t sock;
 
  /*NSM client id*/
  struct nsm_server_entry *nse;
  struct list *oam_req_list;
  u_int8_t fwd_flags;
};


struct nsm_mpls_oam_itut_data
{
  /* Back pointer to OAM Main List */
  struct nsm_mpls_oam_itut *oam; 
  u_int32_t pkt_count;

  struct thread *pkt_timeout;
  bool_t state;
  
  struct pal_timeval sent_time;
  struct pal_timeval recv_time; 
};

#endif /* HAVE_MPLS_OAM_ITUT */

int
nsm_mpls_oam_recv_echo_req (struct nsm_master *, struct mpls_oam_hdr *,
                            struct mpls_oam_echo_request *, struct pal_timeval*,
                            struct mpls_oam_ctrl_data *);

int
nsm_mpls_oam_echo_request_send (struct nsm_mpls_oam *, u_char *, u_int32_t);

int
nsm_mpls_oam_prep_req_for_network (struct nsm_mpls_oam *, u_char *, u_int16_t,
                                   u_char *);

int
nsm_mpls_oam_echo_request_create_send (struct thread *);

int
nsm_mpls_oam_process_echo_request (struct nsm_master *, 
                                   struct nsm_server_entry *,
                                   struct nsm_msg_mpls_ping_req *);

int
nsm_mpls_oam_packet_read (struct nsm_master *, u_char *, int,
                          struct mpls_oam_ctrl_data *,
                          struct pal_udp_ctrl_info *);

int
nsm_mpls_oam_udp_read (struct thread *);

int
nsm_mpls_oam_recv_echo_reply (struct nsm_master *,
                              struct mpls_oam_echo_reply *,
                              u_char trace_reply, struct pal_timeval *,
                              struct pal_udp_ctrl_info *);

int
nsm_mpls_oam_echo_reply_send (struct nsm_master *, struct mpls_oam_hdr *,
                              struct mpls_oam_ctrl_data *, u_char *, u_int16_t);

int
nsm_mpls_oam_echo_reply_process (struct nsm_master *, struct mpls_oam_hdr *,
                                 struct mpls_oam_echo_request *,
                                 struct pal_timeval *,
                                 struct interface *, u_char, u_char, 
                                 struct mpls_label_stack *, 
                                 struct mpls_oam_ctrl_data *);

int
nsm_mpls_oam_set_ip_udp_hdr (struct nsm_mpls_oam *,
                             struct nsm_msg_mpls_ping_req *,
                             struct nsm_master *);

void
nsm_mpls_oam_cleanup (struct nsm_mpls_oam *);

int
nsm_mpls_oam_route_ipv4_lookup (struct nsm_master *, vrf_id_t,
                                struct pal_in4_addr *, u_char);

int
nsm_mpls_oam_transit_stack_lookup (struct nsm_master *,
                                   struct mpls_oam_ctrl_data *,
                                   struct mpls_label_stack *,
                                   struct prefix *, u_char *, u_char);

int
nsm_mpls_oam_process_l3vpn_tlv (struct nsm_master *, struct mpls_oam_hdr *,
                                struct mpls_oam_echo_request *,
                                struct pal_timeval *, 
                                struct mpls_oam_ctrl_data *);

int
nsm_mpls_oam_process_ldp_ipv4_tlv (struct nsm_master *, struct mpls_oam_hdr *,
                                   struct mpls_oam_echo_request *,
                                   struct pal_timeval *,
                                   struct mpls_oam_ctrl_data *, u_char);

int
nsm_mpls_oam_process_rsvp_ipv4_tlv (struct nsm_master *,
                                    struct mpls_oam_hdr *,
                                    struct mpls_oam_echo_request *,
                                    struct pal_timeval *,
                                    struct mpls_oam_ctrl_data *,
                                    u_char);

#ifdef HAVE_MPLS_VC
int
nsm_mpls_oam_process_l2data_tlv (struct nsm_master *,
                                 struct mpls_oam_hdr *,
                                 struct mpls_oam_echo_request *,
                                 struct pal_timeval *,
                                 struct mpls_oam_ctrl_data *);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS 
int
nsm_mpls_oam_send_vpls_echo_request (struct nsm_vpls *,
                                        struct pal_in4_addr *,
                                        u_int32_t, u_int32_t,
                                        u_int32_t, u_char *);
#endif /* HAVE_VPLS */

int
nsm_mpls_oam_process_gen_ipv4_tlv (struct nsm_master *,
                                   struct mpls_oam_hdr *,
                                   struct mpls_oam_echo_request *,
                                   struct pal_timeval *,
                                   struct mpls_oam_ctrl_data *,
                                   u_char);

int
nsm_mpls_oam_echo_timeout (struct thread *);

int
nsm_mpls_oam_set_defaults (struct nsm_mpls_oam *,
                           struct nsm_msg_mpls_ping_req *,
                           struct nsm_master *);

int
nsm_mpls_oam_get_tunnel_info (struct nsm_mpls_oam *,
                              struct nsm_msg_mpls_ping_req *);

int
nsm_mpls_oam_set_ping_reply (struct nsm_msg_mpls_ping_reply *,
                             struct mpls_oam_echo_reply *,
                             struct nsm_mpls_oam_data *,
                             struct nsm_mpls_oam *,
                             struct pal_timeval *);

void
nsm_mpls_oam_client_disconnect (struct nsm_master *, struct nsm_server_entry *);

int
nsm_mpls_oam_set_trace_reply (struct nsm_msg_mpls_tracert_reply *,
                              struct mpls_oam_echo_reply *,
                              struct nsm_mpls_oam_data *,
                              struct nsm_mpls_oam *,
                              struct pal_timeval *,
                              bool_t);

void
send_nsm_mpls_oam_ping_error (struct nsm_mpls_oam *, int);

#ifdef HAVE_MPLS_OAM_ITUT
int
nsm_mpls_oam_itut_process_request(struct nsm_master *,
                                  struct nsm_server_entry *,
                                  struct nsm_msg_mpls_oam_itut_req *);
int 
nsm_mpls_oam_stats_display( struct nsm_master *, 
                            struct nsm_mpls_oam_stats *);

int
nsm_mpls_oam_itut_request_send (struct nsm_mpls_oam_itut  *,
                                u_char *sendbuf,
                                u_int32_t length);
int
nsm_mpls_oam_set_itut_req( struct nsm_mpls_oam_itut *, 
                           struct nsm_msg_mpls_oam_itut_req *,
                           struct nsm_master *);
int
nsm_mpls_oam_prep_req_for_ethernet (struct nsm_mpls_oam_itut *,
                                    u_char *, 
                                    u_int16_t ,
                                    u_char *);
int
nsm_mpls_oam_itut_get_tunnel_info (struct nsm_mpls_oam_itut *,
                                   struct nsm_msg_mpls_oam_itut_req *);
int
nsm_mpls_oam_itut_timeout (struct thread *);

void
send_nsm_mpls_oam_itut_error (struct nsm_mpls_oam_itut *,
                              int );
void
nsm_mpls_oam_itut_cleanup (struct nsm_mpls_oam_itut *);

int
nsm_mpls_oam_itut_request_create_send (struct thread *t);

int
nsm_mpls_oam_itut_check_tool (struct thread *t);

int
nsm_mpls_oam_itut_packet_read (struct nsm_master *, u_char *, int,
                               struct mpls_oam_ctrl_data *);


int 
nsm_mpls_oam_itut_process_fdi_request(struct nsm_server_entry *, 
                                      struct interface *);

#endif /* HAVE_MPLS_OAM_ITUT */

#endif /* _PACOS_NSM_MPLS_OAM_H */




















