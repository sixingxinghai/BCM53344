/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_BFD_MESSAGE_H
#define _PACOS_BFD_MESSAGE_H

#if defined (HAVE_BFD) || defined (HAVE_MPLS_OAM)

#define BFD_ERR_PKT_TOO_SMALL                    -1
#define BFD_ERR_INVALID_SERVICE                  -2
#define BFD_ERR_INVALID_PKT                      -3
#define BFD_ERR_SYSTEM_FAILURE                   -4
#define BFD_ERR_INVALID_AFI                      -5
#define BFD_ERR_INVALID_TLV                      -100

/* BFD protocol version is 1.  */
#define BFD_PROTOCOL_VERSION_1                   1
#define BFD_PROTOCOL_VERSION_2                   2 

/* BFD message max length.  */
#define BFD_MESSAGE_MAX_LEN                      4096

/* BFD service types.  */
#define BFD_SERVICE_SESSION                      0
#define BFD_SERVICE_RESTART                      1
#define BFD_SERVICE_OAM                          2
#define BFD_SERVICE_MAX                          3

/* BFD messages.  */
#define BFD_MSG_BUNDLE

/* These messages has VR-ID = 0, VRF-ID = 0. */
#define BFD_MSG_SERVICE_REQUEST                  0
#define BFD_MSG_SERVICE_REPLY                    1

/* These messages has VR-ID = any, VRF-ID = 0. */
#define BFD_MSG_SESSION_PRESERVE                 2
#define BFD_MSG_SESSION_STALE_REMOVE             3

/* These messages has VR-ID = any, VRF-ID = any. */
#define BFD_MSG_SESSION_ADD                      4
#define BFD_MSG_SESSION_DELETE                   5
#define BFD_MSG_SESSION_UP                       6
#define BFD_MSG_SESSION_DOWN                     7
#define BFD_MSG_SESSION_ERROR                    8
#define BFD_MSG_SESSION_ATTR_UPDATE              9  /* Not supported yet.  */
 
#define OAM_MSG_MPLS_ECHO_REQUEST                10
#define OAM_MSG_MPLS_PING_REPLY                  11
#define OAM_MSG_MPLS_TRACERT_REPLY               12
#define OAM_MSG_MPLS_OAM_ERROR                   13
#define BFD_MSG_MAX                              14

#define OAM_MAX_CONSEQ_TIMEOUTS                   5
 
#ifdef HAVE_HA
/* Allowed messages to/from BFD in Standby */
#define HA_ALLOW_BFD_MSG_TYPE(TYPE)                                            \
      (((TYPE) == BFD_MSG_SERVICE_REQUEST)                                     \
        ||((TYPE) == BFD_MSG_SERVICE_REPLY))
#endif /* HAVE_HA */

#define BFD_CINDEX_SIZE                     MSG_CINDEX_SIZE

/* BFD Context Header
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            VR-ID                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            VRF-ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   BFD Message Header
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Message Type         |           Message Len         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Message Id                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct bfd_msg_header
{
  /* VR-ID. */
  u_int32_t vr_id;

  /* VRF-ID. */
  u_int32_t vrf_id;

  /* Message Type. */
  u_int16_t type;

  /* Message Len. */
  u_int16_t length;

  /* Message ID. */
  u_int32_t message_id;
};
#define BFD_MSG_HEADER_SIZE   16

/* BFD TLV Header
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            Type               |             Length            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct bfd_tlv_header
{
  u_int16_t type;
  u_int16_t length;
};
#define BFD_TLV_HEADER_SIZE   4

/* BFD Service message format

   This message is used by:

   BFD_MSG_SERVICE_REQUEST
   BFD_MSG_SERVICE_REPLY

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             Version           |             Reserved          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        PacOS Module Id                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Services                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Graceful Restart TLV

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        Restart State          |             Reserved          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     Grace Period Expires                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Grace Period Expires:  Restart time - (Current time - Disconnect time)

*/

/* BFD services structure.  */
struct bfd_msg_service
{
  /* TLV flags. */
  cindex_t cindex;
#define BFD_SERVICE_CTYPE_RESTART            0

  /* BFD Protocol Version. */
  u_int16_t version;

  /* Reserved. */
  u_int16_t reserved;

  /* Module ID. */
  u_int32_t module_id;

  /* Service Bits. */
  u_int32_t bits;

  /* Following 8 bytes are for restart TLV.  */

  /* Graceful Restart State */
  u_int16_t restart_state;

  /* Reserved. */
  u_int16_t restart_reserved;

  /* Grace Perioud Expires. */
  u_int32_t grace_period;
};
#define BFD_MSG_SERVICE_SIZE     12

/* BFD Session message.

   This message is used by:

   BFD_MSG_SESSION_ADD
   BFD_MSG_SESSION_DELETE
   BFD_MSG_SESSION_UP
   BFD_MSG_SESSION_DOWN
   BFD_MSG_SESSION_ERROR
   BFD_MSG_SESSION_ATTR_UPDATE

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Flags     |    LLType     |              AFI              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Interface Index                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Source address                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Destination address                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

#ifdef HAVE_MPLS_OAM

#define BFD_MPLS_MIN_LSP_PING_CONSTR 10

/**@brief ENUM of LSP Type values.
 */
enum
{
  BFD_MPLS_LSP_TYPE_LDP = 0,
  BFD_MPLS_LSP_TYPE_RSVP,
  BFD_MPLS_LSP_TYPE_STATIC,
  BFD_MPLS_LSP_TYPE_UNKNOWN
};

/**@brief BFD parameters for MPLS LSP FEC related sessions.
 */
struct bfd_mpls_params
{
  u_int32_t ftn_ix; /** FTN Index of the FEC. */
  u_int16_t lsp_type; /** LSP Type Value */
  struct prefix_ipv4 fec; /** FEC Prefix. */
  u_int32_t tun_length; /** RSVP Tunnel Length */
  u_char *tun_name; /** RSVP Tunnel Name */
  bool_t is_egress; /** Flag for Ingress/Egress LSR */

  /* User configured BFD attributes for BFD MPLS session. */
  u_int32_t lsp_ping_intvl; /** Desired LSP Ping Interval in seconds */
  u_int32_t min_tx; /** BFD min Tx value */
  u_int32_t min_rx; /** BFD min Rx value */
  u_int8_t mult; /** BFD Detection multiplier value */
  u_char fwd_flags; /** fwd flags */

  /* Default values for BFD attributes. */
#define BFD_MPLS_LSP_PING_INTERVAL_DEF         5  /* seconds */
#define BFD_MPLS_MIN_TX_INTERVAL_DEF           50 /* milli-sceonds */
#define BFD_MPLS_MIN_RX_INTERVAL_DEF           50 /* milli-sceonds */
#define BFD_MPLS_DETECT_MULT_DEF               5  /* number */

  /* To send packet over third party forwarder modules. */
  u_int32_t tunnel_label; /** FEC/RSVP Trunks' Tunnel Label */
};
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_VCCV
/**@brief BFD parameters for VCCV related sessions.
 */
struct bfd_vccv_params
{
  u_int32_t vc_id; /** VC Id*/
  u_int32_t in_vc_label; /** Incoming VC Label */
  u_int32_t out_vc_label; /** Outgoing VC Label */
  u_int8_t cc_type; /** CC Type */
  u_int8_t cv_type; /** CV Type */
  u_int32_t ac_ix; /** Access Interface Index */

  /* To send packet over third party forwarder modules. */
  u_int32_t tunnel_label; /** Tunnel Label */
};
#endif /* HAVE_VCCV */


struct bfd_msg_session
{
  /* Ctype index.  */
  cindex_t cindex;

  /* Interface index.  */
  u_int32_t ifindex;

  /* Flags of the session.  */
  u_char flags;
#define BFD_MSG_SESSION_FLAG_MH    (1 << 0)  /* Multi-Hop.  */
#define BFD_MSG_SESSION_FLAG_DC    (1 << 1)  /* Demand Circuit.  */
#define BFD_MSG_SESSION_FLAG_PS    (1 << 2)  /* Persistent Session.  */
#define BFD_MSG_SESSION_FLAG_AD    (1 << 3)  /* User Admin Down.  */

  /* Link layer type.  */
  u_char ll_type;
#define BFD_MSG_LL_TYPE_IPV4       1
#define BFD_MSG_LL_TYPE_IPV6       2
#define BFD_MSG_LL_TYPE_MPLS_LSP   3    
#define BFD_MSG_LL_TYPE_MPLS_VCCV  4    

  /* Address Family Identifier.  */
  afi_t afi;

  /* Source and destination addresses.  */
  union
  {
    struct
    {
      struct pal_in4_addr src;
      struct pal_in4_addr dst;
    } ipv4;
#ifdef HAVE_IPV6
    struct
    {
      struct pal_in6_addr src;
      struct pal_in6_addr dst;
    } ipv6;
#endif /* HAVE_IPV6 */
  } addr;

  /* Data required for BFD MPLS and BFD VCCV. */
#ifdef HAVE_MPLS_OAM
  union
  {
    struct bfd_mpls_params mpls_params;
#ifdef HAVE_VCCV
    struct bfd_vccv_params vccv_params;
#endif /* HAVE_VCCV */
  }addl_data;
#endif /* HAVE_MPLS_OAM */
};
#define BFD_MSG_SESSION_SIZE                   16
#define BFD_MPLS_MSG_SESSION_SIZE              52 

#ifdef HAVE_VCCV
#define BFD_VCCV_MSG_SESSION_SIZE              38  
#endif /* HAVE_VCCV */

/* BFD Session Management message.

   This message is used by:

   BFD_MSG_SESSION_PRESERVE
   BFD_MSG_SESSION_STALE_REMOVE

   Restart Time TLV
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Restart Time                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/

struct bfd_msg_session_manage
{
  cindex_t cindex;
#define BFD_SESSION_MANAGE_CTYPE_RESTART_TIME     0

  u_int32_t restart_time;
};
#define BFD_MSG_SESSION_MANAGE_SIZE               0

/* BFD message send queue.  */
struct bfd_message_queue
{
  struct bfd_message_queue *next;
  struct bfd_message_queue *prev;

  u_char *buf;
  u_int16_t length;
  u_int16_t written;
};

/* BFD callback function typedef.  */
typedef int (*BFD_CALLBACK) (struct bfd_msg_header *, void *, void *);
typedef int (*BFD_PARSER) (u_char **, u_int16_t *, struct bfd_msg_header *,
                           void *, BFD_CALLBACK);

void bfd_session_dump (struct lib_globals *, struct bfd_msg_session *);
void bfd_session_manage_dump (struct lib_globals *,
                              struct bfd_msg_session_manage *);

const char *bfd_service_to_str (int);
const char *bfd_msg_to_str (int);

int bfd_encode_session (u_char **, u_int16_t *, struct bfd_msg_session *);
int bfd_encode_header (u_char **, u_int16_t *, struct bfd_msg_header *);
int bfd_decode_session (u_char **, u_int16_t *, struct bfd_msg_session *);
void bfd_header_dump (struct lib_globals *, struct bfd_msg_header *);
void bfd_service_dump (struct lib_globals *, struct bfd_msg_service *);

int bfd_decode_header (u_char **, u_int16_t *, struct bfd_msg_header *);
int bfd_encode_service (u_char **, u_int16_t *, struct bfd_msg_service *);
int bfd_decode_service (u_char **, u_int16_t *, struct bfd_msg_service *);
int bfd_encode_session_manage (u_char **, u_int16_t *,
                               struct bfd_msg_session_manage *);
int bfd_decode_session_manage (u_char **, u_int16_t *,
                               struct bfd_msg_session_manage *);

int bfd_parse_session (u_char **, u_int16_t *, struct bfd_msg_header *,
                       void *, BFD_CALLBACK);
int bfd_parse_session_manage (u_char **, u_int16_t *, struct bfd_msg_header *,
                              void *, BFD_CALLBACK);

int bfd_parse_service (u_char **, u_int16_t *, struct bfd_msg_header *,
                       void *, BFD_CALLBACK);

#ifdef HAVE_MPLS_OAM

/* OAM Return Codes */
#define MPLS_OAM_DEFAULT_RET_CODE             0
#define MPLS_OAM_MALFORMED_REQUEST            1
#define MPLS_OAM_ERRORED_TLV                  2
#define MPLS_OAM_RTR_IS_EGRESS_AT_DEPTH       3
#define MPLS_OAM_RTR_HAS_NO_MAPPING_AT_DEPTH  4
#define MPLS_OAM_DSMAP_MISMATCH               5
#define MPLS_OAM_UPSTREAM_IF_UNKNOWN          6
#define MPLS_OAM_RSVD_VALUE                   7
#define MPLS_OAM_LBL_SWITCH_AT_DEPTH          8
#define MPLS_OAM_LBL_SWITCH_IP_FWD_AT_DEPTH   9
#define MPLS_OAM_MAPPING_ERR_AT_DEPTH        10
#define MPLS_OAM_NO_MAPPING_AT_DEPTH         11
#define MPLS_OAM_PROTOCOL_ERR_AT_DEPTH       12
#define MPLS_OAM_PING_TIMEOUT_ERR            13

/* OAM Error codes */
#define MPLS_OAM_VCCV_CCTYPE_MISMATCH        -1

/* OAM CTYPES */
typedef enum  {
   OAM_CTYPE_MSG_MPLSONM_OPTION_ONM_TYPE= 0,
   OAM_CTYPE_MSG_MPLSONM_OPTION_LSP_TYPE,
   OAM_CTYPE_MSG_MPLSONM_OPTION_LSP_ID,
   OAM_CTYPE_MSG_MPLSONM_OPTION_FEC,
   OAM_CTYPE_MSG_MPLSONM_OPTION_DESTINATION,
   OAM_CTYPE_MSG_MPLSONM_OPTION_SOURCE,
   OAM_CTYPE_MSG_MPLSONM_OPTION_TUNNELNAME,
   OAM_CTYPE_MSG_MPLSONM_OPTION_EGRESS,
   OAM_CTYPE_MSG_MPLSONM_OPTION_EXP,
   OAM_CTYPE_MSG_MPLSONM_OPTION_PEER,
   OAM_CTYPE_MSG_MPLSONM_OPTION_PREFIX,
   OAM_CTYPE_MSG_MPLSONM_OPTION_TTL,
   OAM_CTYPE_MSG_MPLSONM_OPTION_TIMEOUT,
   OAM_CTYPE_MSG_MPLSONM_OPTION_REPEAT,
   OAM_CTYPE_MSG_MPLSONM_OPTION_VERBOSE,
   OAM_CTYPE_MSG_MPLSONM_OPTION_REPLYMODE,
   OAM_CTYPE_MSG_MPLSONM_OPTION_INTERVAL,
   OAM_CTYPE_MSG_MPLSONM_OPTION_FLAGS,
   OAM_CTYPE_MSG_MPLSONM_OPTION_NOPHP
}oam_ctype_mpls_onm_option;

/* Should have matching string in mpls_onm_cmd_options structure */
typedef enum  {
   MPLSONM_OPTION_PING = 0,
   MPLSONM_OPTION_TRACE,
   MPLSONM_OPTION_LDP,
   MPLSONM_OPTION_RSVP,
   MPLSONM_OPTION_VPLS,
   MPLSONM_OPTION_L2CIRCUIT,
   MPLSONM_OPTION_VCCV,
   MPLSONM_OPTION_L3VPN,
   MPLSONM_OPTION_STATIC,
   MPLSONM_OPTION_DESTINATION,
   MPLSONM_OPTION_SOURCE,
   MPLSONM_OPTION_TUNNELNAME,
   MPLSONM_OPTION_EGRESS,
   MPLSONM_OPTION_EXP,
   MPLSONM_OPTION_PEER,
   MPLSONM_OPTION_PREFIX,
   MPLSONM_OPTION_TTL,
   MPLSONM_OPTION_TIMEOUT,
   MPLSONM_OPTION_REPEAT,
   MPLSONM_OPTION_VERBOSE,
   MPLSONM_OPTION_REPLYMODE,
   MPLSONM_OPTION_INTERVAL,
   MPLSONM_OPTION_FLAGS,
   MPLSONM_OPTION_NOPHP,
   MPLSONM_OPTION_SEQNUM,
   MPLSONM_MAX_OPTIONS
}mpls_onm_option_type;

#define  OAM_MSG_MPLS_ONM_SIZE 8
#define  OAM_MSG_MPLS_ONM_ERR_SIZE 4

#define  OAM_DETAIL_SET          0
#define  MPLS_OAM_DEFAULT_TTL   30
#define  MPLS_OAM_DEFAULT_COUNT  5

struct oam_msg_mpls_ping_req
{
  /* Echo Request Options */
  cindex_t cindex;

  union
  {
    struct rsvp_tunnel
    {
      struct pal_in4_addr addr;
      u_char *tunnel_name;
      int tunnel_len;
    }rsvp;
    struct fec_data
    {
      u_char *vrf_name;
      int vrf_name_len;
      u_int32_t prefixlen;
      struct pal_in4_addr ip_addr;
      struct pal_in4_addr vpn_prefix;
    }fec;
    struct vc_data
    {
      u_int32_t vc_id;
      struct pal_in4_addr vc_peer;
#ifdef HAVE_VCCV
      bool_t is_vccv;
#endif /* HAVE_VCCV */
    }l2_data;
  }u;

  struct pal_in4_addr dst;
  struct pal_in4_addr src;
  u_int32_t interval;
  u_int32_t timeout;
  u_int32_t ttl;
  u_int32_t repeat;
  u_int8_t exp_bits;
  u_char flags;
  u_int8_t reply_mode;
  u_int8_t verbose;
  u_int8_t nophp;
  bool_t is_periodic;

#ifdef HAVE_BFD
  /* Flag for BFD TLV*/
  bool_t is_bfd;
  
  /* BFD Discriminator to fill the BFD TLV. */
  u_int32_t bfd_disc;
#endif /* HAVE_BFD */

  /* Echo Type - trace or ping */
  bool_t req_type;
  /* FEC type */
  u_char type;
  /* Socket id of releavant Utility instance */
  pal_sock_handle_t sock;
};

/* Send reply for successful ping */
struct oam_msg_mpls_ping_reply
{
  cindex_t cindex;
  /* These values are used for detailed and standard output */
  u_int32_t sent_time_sec;
  u_int32_t sent_time_usec;
  u_int32_t recv_time_sec;
  u_int32_t recv_time_usec;
  struct pal_in4_addr reply;
  u_int32_t ping_count;
  u_int32_t recv_count;
  u_int32_t ret_code;
};

typedef enum {
  OAM_CTYPE_MSG_MPLSONM_PING_SENTTIME=0,
  OAM_CTYPE_MSG_MPLSONM_PING_RECVTIME,
  OAM_CTYPE_MSG_MPLSONM_PING_REPLYADDR,
  OAM_CTYPE_MSG_MPLSONM_PING_VPNID,
  OAM_CTYPE_MSG_MPLSONM_PING_COUNT,
  OAM_CTYPE_MSG_MPLSONM_PING_RECVCOUNT,
  OAM_CTYPE_MSG_MPLSONM_PING_RETCODE
} oam_ctype_mpls_onm_ping_reply;

/* Send reply for a trace route */
struct oam_msg_mpls_tracert_reply
{
  cindex_t cindex;
  struct pal_in4_addr reply;
  u_int32_t sent_time_sec;
  u_int32_t sent_time_usec;
  u_int32_t recv_time_sec;
  u_int32_t recv_time_usec;
  u_int32_t recv_count;
  u_int32_t ret_code;
  u_int16_t ttl;
  int ds_len;
  struct list *ds_label;
  bool_t last;
  u_int32_t out_label;
};

typedef enum {
  OAM_CTYPE_MSG_MPLSONM_TRACE_REPLYADDR=0,
  OAM_CTYPE_MSG_MPLSONM_TRACE_SENTTIME,
  OAM_CTYPE_MSG_MPLSONM_TRACE_RECVTIME,
  OAM_CTYPE_MSG_MPLSONM_TRACE_TTL,
  OAM_CTYPE_MSG_MPLSONM_TRACE_DSMAP,
  OAM_CTYPE_MSG_MPLSONM_TRACE_RETCODE,
  OAM_CTYPE_MSG_MPLSONM_TRACE_LAST_MSG,
  OAM_CTYPE_MSG_MPLSONM_TRACE_RECVCOUNT,
  OAM_CTYPE_MSG_MPLSONM_TRACE_OUTLABEL
} oam_ctype_mpls_onm_trace_reply;

#define OAM_MPLSONM_ECHO_TIMEOUT        0
#define OAM_MPLSONM_NO_FTN              1
#define OAM_MPLSONM_TRACE_LOOP          2
#define OAM_MPLSONM_PACKET_SEND_ERROR   3
#define OAM_MPLSONM_ERR_EXPLICIT_NULL   4
#define OAM_MPLSONM_ERR_NO_CONFIG       5
#define OAM_MPLSONM_ERR_UNKNOWN         6
#define OAM_MPLSONM_ERR_VCCV_NOT_IN_USE 7

/* Signal Error conditions, incl timeouts etc */
struct oam_msg_mpls_ping_error
{
  cindex_t cindex;
  u_int8_t msg_type;
  u_int8_t type;
  u_int32_t vpn_id;
  struct pal_in4_addr fec;
  struct pal_in4_addr vpn_peer;
  u_int32_t sent_time;

  u_int32_t ping_count;
  u_int32_t recv_count;
  u_int32_t ret_code;
  /* This is used to indicate internal errors and timeouts */
  int32_t err_code;
};

typedef enum {
  OAM_CTYPE_MSG_MPLSONM_ERROR_MSGTYPE=0,
  OAM_CTYPE_MSG_MPLSONM_ERROR_TYPE,
  OAM_CTYPE_MSG_MPLSONM_ERROR_VPNID,
  OAM_CTYPE_MSG_MPLSONM_ERROR_FEC,
  OAM_CTYPE_MSG_MPLSONM_ERROR_VPN_PEER,
  OAM_CTYPE_MSG_MPLSONM_ERROR_SENTTIME,
  OAM_CTYPE_MSG_MPLSONM_ERROR_RETCODE,
  OAM_CTYPE_MSG_MPLSONM_ERROR_PINGCOUNT,
  OAM_CTYPE_MSG_MPLSONM_ERROR_RECVCOUNT,
  OAM_CTYPE_MSG_MPLSONM_ERROR_ERRCODE
} oam_ctype_mpls_onm_error;

#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_MPLS_OAM
int
oam_parse_mpls_oam_request (u_char **pnt, u_int16_t *size,
                            struct bfd_msg_header *header, void *arg,
                            BFD_CALLBACK callback);

int
oam_parse_mpls_oam_req (u_char **pnt, u_int16_t *size,
                        struct bfd_msg_header *header, void *arg,
                        BFD_CALLBACK callback);
int
oam_encode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct oam_msg_mpls_ping_req *msg);
int
oam_decode_mpls_onm_req (u_char **pnt, u_int16_t *size,
                         struct oam_msg_mpls_ping_req *msg);
int
oam_encode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct oam_msg_mpls_ping_reply *msg);
int
oam_decode_mpls_onm_ping_reply (u_char **pnt, u_int16_t *size,
                                struct oam_msg_mpls_ping_reply *msg);
int
oam_encode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                struct oam_msg_mpls_tracert_reply *msg);
int
oam_decode_mpls_onm_trace_reply (u_char **pnt, u_int16_t *size,
                                struct oam_msg_mpls_tracert_reply *msg);
int
oam_encode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                           struct oam_msg_mpls_ping_error *msg);
int
oam_decode_mpls_onm_error (u_char **pnt, u_int16_t *size,
                          struct oam_msg_mpls_ping_error *msg);
int
oam_parse_ping_response (u_char **pnt, u_int16_t *size,
                         struct bfd_msg_header *header, void *arg,
                         BFD_CALLBACK callback);
int
oam_parse_tracert_response (u_char **pnt, u_int16_t *size,
                            struct bfd_msg_header *header, void *arg,
                            BFD_CALLBACK callback);
int
oam_parse_error_response (u_char **pnt, u_int16_t *size,
                          struct bfd_msg_header *header, void *arg,
                          BFD_CALLBACK callback);
#endif /* HAVE_MPLS_OAM */
#endif /* HAVE_BFD || HAVE_MPLS_OAM */
#endif /* _PACOS_BFD_MESSAGE_H */
