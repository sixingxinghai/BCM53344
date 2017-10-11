/**@file elmi_types.h
 * @brief  This file contains the data structures, constants 
 * and macros used for elmid daemon
 */
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_TYPES_H
#define _PACOS_ELMI_TYPES_H

#define ELMI_IFNAMSIZ                 16
#define ELMI_MAC_ADDR_LEN             6

#define ELMI_SUCCESS                  0
#define ELMI_FAILURE                 -1
#define ELMI_OK                       1
#define ELMI_ACTIVE                   1
#define ELMI_INACTIVE                 0

#define ELMI_DEFAULT_VAL              0

#define ELMI_FALSE                    0
#define ELMI_TRUE                     1

#define ELMI_TIME_LENGTH                 48
#define DEFAULT_BRIDGE                   "0"

#define ELMI_ETH_TYPE                   0x88ee

/* Constants used in encoding/decoding part */
#define ELMI_RESERVED_BYTE               0
#define ELMI_UNUSED_BYTE                 1
#define ELMI_TLV_LEN                     1

#define ELMI_ETH_TYPE                   0x88ee

#define ELMI_TLV_HEADER_SIZE            2 

#define ELMI_MIN_PACKET_SIZE            46
#define ELMI_MAX_PACKET_SIZE            1500

/* MEF16 TLV Types */
#define ELMI_PROTO_VERSION              1

#define ELMI_MSG_TYPE_STATUS            0x7D
#define ELMI_MSG_TYPE_STATUS_ENQUIRY    0x75

/* Report Types */
#define ELMI_REPORT_TYPE_FULL_STATUS    0x00
#define ELMI_REPORT_TYPE_ECHECK         0x01
#define ELMI_REPORT_TYPE_SINGLE_EVC     0x02
#define ELMI_REPORT_TYPE_FULL_STATUS_CONTD  0x03

#define ELMI_REPORT_TYPE_IE_TLV           0x01
#define ELMI_REPORT_TYPE_IE_LENGTH        0x01

#define ELMI_SEQUENCE_NUM_IE_TLV          0x02
#define ELMI_SEQUENCE_NUM_IE_LENGTH       0x02

#define ELMI_DATA_INSTANCE_IE_TLV         0x03
#define ELMI_DATA_INSTANCE_IE_LENGTH      0x05

#define ELMI_UNI_STATUS_IE_TLV            0x11
#define ELMI_EVC_STATUS_IE_TLV            0x21

#define ELMI_CEVLAN_EVC_MAP_IE_TLV        0x22
#define ELMI_CEVLAN_EVC_MAP_FIXED_LEN     8
#define ELMI_CEVLAN_EVC_MAP_MAX_VLAN_GRP  121

/* Subinformation Elements */
#define ELMI_UNI_ID_SUB_IE_TLV            0x51

#define ELMI_EVC_PARAMS_SUB_IE_TLV        0x61
#define ELMI_EVC_PARAMS_SUB_IE_TLV_LEN    0x01

#define ELMI_EVC_ID_SUB_IE_TLV            0x62
#define ELMI_EVC_MAP_ENTRY_SUB_IE_TLV     0x63
#define ELMI_BW_PROFILE_SUB_IE_TLV        0x71
#define ELMI_BW_PROFILE_SUB_IE_LEN        0x0C

#define ELMI_MAX_TLV                      255

#define ELMI_BW_COUPLING_FLAG_MASK        0x02
#define ELMI_BW_COLOR_MODE_MASK           0x04

/* Others */
#define ELMI_BW_PROFILE_MIN_LEN         1
#define ELMI_TLV_MIN_SIZE               2

#define ELMI_TLV_TYPE_FIELD_SIZE        0x01
#define ELMI_TLV_LENGTH_FIELD_SIZE      0x01

#define ELMI_INVALID_REPORT_TYPE        10
#define ELMI_UNIC_BUF_SIZE              16
#define ELMI_UNIN_ASYNC_BUF_SIZE        21

#define ELMI_BW_PROFILE_FIELD_LENGTH    12
#define ELMI_UNI_NAME_LEN               64
#define ELMI_EVC_NAME_LEN               100

#define ELMI_MAX_COS_ID                 8
#define ELMI_VLAN_NAMSIZ                20

#define ELMI_HEADER_FIXED_FRAME_LEN     16

#define ELMI_UNI_FIXED_FRAME_LEN        19
/* 1 (Tlv type) + 1 (Tlv Len) + 1 (Cevlan/evc map type) + 
 * 1 (UNI id tlv) + * 1 (UNI id tlv len) + 
 * 1 (BW tlv type) + 1 (BW tlv len) + 12 (BW len val)
 */

#define ELMI_EVC_FIXED_FRAME_LEN        10
/* 1 (Tlv type) + 1 (Tlv Len) + 2 (EVC Ref id) + 
 * 1 (EVC status type) + 1 (EVC parameters tlv type) + 
 * 1 (EVC parameters tlv len) + 1 (EVC parameters tlv val) + 
 * 1 (EVC id tlv type) + 1 (EVC id tlv len)
 */

#define ELMI_EVC_ASYNC_EVC_LEN          3
#define ELMI_MAX_CVLANS_IN_CMAP_TLV     121

#define ELMI_EVC_REFERENCE_ID_LEN       2
#define ELMI_EVC_STATUS_TYPE_LEN        1
#define ELMI_EVC_PARAMETERS_LEN         1
#define ELMI_UNI_MAP_TYPE_LEN           1

#define ELMI_SEQ_NUM_MAX                255
#define ELMI_CEVLAN_EVC_SEQ_NUM_MAX     64
#define ELMI_SEQ_NUM_MIN                1
#define ELMI_COUNTER_MIN                0
#define ELMI_CVLAN_TLV_LEN              2

#define ELMI_EVC_TYPE_MASK              0x07 

/* System Parameters */
/* N391 Polling Counter */
/* Used by UNI_C only */
#define ELMI_N391_POLLING_COUNTER_DEF   360
#define ELMI_N391_POLLING_COUNTER_MIN   1
#define ELMI_N391_POLLING_COUNTER_MAX   65000

/* N393 Status Counter */
/* Used by UNI-C and UNI-N */
#define ELMI_N393_STATUS_COUNTER_DEF    4
#define ELMI_N393_STATUS_COUNTER_MIN    2
#define ELMI_N393_STATUS_COUNTER_MAX    10

/* T391 Polling Timer - PT */
/* Used by UNI-C */
#define ELMI_T391_POLLING_TIME_DEF      10
#define ELMI_T391_POLLING_TIME_MIN      5
#define ELMI_T391_POLLING_TIME_MAX      30

/* T392 Polling Verification Timer - PVT */
/* Used by UNI-N */
#define ELMI_T392_DEFAULT              15
#define ELMI_T392_PVT_MIN              5
#define ELMI_T392_PVT_MAX              30
#define ELMI_T392_PVT_DISABLE          0

#define ELMI_ASYNC_TIME_INTERVAL_MIN   1
#define ELMI_ASYNC_TIME_INTERVAL_MAX   3
#define ELMI_ASYNC_TIME_INTERVAL_DEF   1

/* ELMI Session state */
#define ELMI_OPERATIONAL_STATE_DOWN    0
#define ELMI_OPERATIONAL_STATE_UP      1 

#define ELMI_UNIN_DATA_INSTANCE_MIN    1
#define ELMI_INVALID_VAL               0

/* Bit masks */
#define ELMI_SEQ_NUM_BIT_MASK          0x3f
#define ELMI_SEQ_NUM_BITS              6
#define ELMI_LAST_IE_BIT_MASK          0x40

/* VLAN bitmap manipulation macros. */
#define ELMI_VLAN_MIN                 1       /* Min EVCs.   */
#define ELMI_VLAN_MAX                 4094    /* Max VLANs.   */
#define ELMI_VLAN_BMP_WORD_WIDTH      32
#define ELMI_VLAN_BMP_WORD_MAX                  \
        ((ELMI_VLAN_MAX + ELMI_VLAN_BMP_WORD_WIDTH) / ELMI_VLAN_BMP_WORD_WIDTH)


#define ELMI_VLAN_SET_BMP_ITER_BEGIN(bmp, vid)                             \
    do {                                                                   \
        int _w, _i;                                                        \
        (vid) = 0;                                                         \
        for (_w = 0; _w < ELMI_VLAN_BMP_WORD_MAX; _w++)                    \
          for (_i = 0; _i < ELMI_VLAN_BMP_WORD_WIDTH; _i++, (vid)++)       \
            if ((bmp).bitmap[_w] & (1U << _i))

#define ELMI_VLAN_SET_BMP_ITER_END(bmp, vid)                               \
    } while (0)

#define ELMI_VLAN_UNSET_BMP_ITER_BEGIN(bmp, vid)                           \
    do {                                                                   \
        int _w, _i;                                                        \
        (vid) = 0;                                                         \
        for (_w = 0; _w < ELMI_VLAN_BMP_WORD_MAX; _w++)                    \
          for (_i = 0; _i < ELMI_VLAN_BMP_WORD_WIDTH; _i++, (vid)++)       \
            if ((bmp).bitmap[_w] & (1U << _i))

#define ELMI_VLAN_UNSET_BMP_ITER_END(bmp, vid)                             \
    } while (0)

#define ELMI_VLAN_BMP_INIT(bmp)                                            \
   do {                                                                    \
      pal_mem_set ((bmp).bitmap, 0, sizeof ((bmp).bitmap));                \
   } while (0)

#define ELMI_VLAN_BMP_SET(bmp, vid)                                        \
   do {                                                                    \
      u_int32_t _word = (vid) / ELMI_VLAN_BMP_WORD_WIDTH;                  \
      (bmp).bitmap[_word] |= (1U << ((vid) % ELMI_VLAN_BMP_WORD_WIDTH));   \
  } while (0)

#define ELMI_VLAN_BMP_UNSET(bmp, vid)                                      \
   do {                                                                    \
      u_int32_t _word = (vid) / ELMI_VLAN_BMP_WORD_WIDTH;                  \
      (bmp).bitmap[_word] &= ~(1U <<((vid) % ELMI_VLAN_BMP_WORD_WIDTH));   \
   } while (0)

#define ELMI_VLAN_BMP_IS_MEMBER(bmp, vid)               \
    ((bmp)->bitmap[(vid) / ELMI_VLAN_BMP_WORD_WIDTH] &  \
    (1U << ((vid) % ELMI_VLAN_BMP_WORD_WIDTH)))

#define ELMI_BRIDGE_TYPE_STP             NSM_BRIDGE_TYPE_STP
#define ELMI_BRIDGE_TYPE_STP_VLANAWARE   NSM_BRIDGE_TYPE_STP_VLANAWARE
#define ELMI_BRIDGE_TYPE_RSTP            NSM_BRIDGE_TYPE_RSTP
#define ELMI_BRIDGE_TYPE_RSTP_VLANAWARE  NSM_BRIDGE_TYPE_RSTP_VLANAWARE
#define ELMI_BRIDGE_TYPE_MSTP            NSM_BRIDGE_TYPE_MSTP
#define ELMI_BRIDGE_TYPE_PROVIDER_RSTP   NSM_BRIDGE_TYPE_PROVIDER_RSTP
#define ELMI_BRIDGE_TYPE_PROVIDER_MSTP   NSM_BRIDGE_TYPE_PROVIDER_MSTP
#define ELMI_BRIDGE_TYPE_CE              NSM_BRIDGE_TYPE_CE
#define ELMI_BRIDGE_TYPE_RPVST_PLUS      NSM_BRIDGE_TYPE_RPVST_PLUS
#define ELMI_BRIDGE_TYPE_BACKBONE_RSTP   NSM_BRIDGE_TYPE_BACKBONE_RSTP
#define ELMI_BRIDGE_TYPE_BACKBONE_MSTP   NSM_BRIDGE_TYPE_BACKBONE_MSTP

#define ELMI_VLAN_PORT_MODE_INVALID   NSM_MSG_VLAN_PORT_MODE_INVALID
#define ELMI_VLAN_PORT_MODE_ACCESS    NSM_MSG_VLAN_PORT_MODE_ACCESS
#define ELMI_VLAN_PORT_MODE_HYBRID    NSM_MSG_VLAN_PORT_MODE_HYBRID
#define ELMI_VLAN_PORT_MODE_TRUNK     NSM_MSG_VLAN_PORT_MODE_TRUNK
#define ELMI_VLAN_PORT_MODE_CE        NSM_MSG_VLAN_PORT_MODE_CE
#define ELMI_VLAN_PORT_MODE_CN        NSM_MSG_VLAN_PORT_MODE_CN
#define ELMI_VLAN_PORT_MODE_PN        NSM_MSG_VLAN_PORT_MODE_PN
#define ELMI_VLAN_PORT_MODE_PE        NSM_MSG_VLAN_PORT_MODE_PE

/* EVC Types */
#define  ELMI_NSM_SVLAN_P2P   (1 << 4) 
#define  ELMI_NSM_SVLAN_M2M   (1 << 5) 

/* Val started from 100 not to overlap with number defined in NSM */
enum elmi_mef_bridge_type
{
  ELMI_BRIDGE_TYPE_MEF_CE = 100,
  ELMI_BRIDGE_TYPE_MEF_PE,
  ELMI_BRIDGE_TYPE_MEF_INVALID,
};

enum elmi_bandwidth_profile_lvl
{
  ELMI_BW_PROFILE_INVALID = 0,
  ELMI_BW_PROFILE_PER_UNI,
  ELMI_BW_PROFILE_PER_EVC,
  ELMI_BW_PROFILE_PER_EVC_COS,
};

/* For vlan table key. */
struct elmi_prefix_vlan
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  s_int32_t vid;
};

/* CVLAN/EVC MAP Types */
enum cvlan_evc_map_type 
{
  ELMI_BUNDLING_INVALID = 0,
  ELMI_ALL_TO_ONE_BUNDLING,
  ELMI_SERVICE_MULTIPLEXING, 
  ELMI_BUNDLING,
};

enum elmi_nsm_uni_service_attr
{
  ELMI_NSM_UNI_MUX_BNDL,
  ELMI_NSM_UNI_MUX,
  ELMI_NSM_UNI_BNDL,
  ELMI_NSM_UNI_AIO_BNDL,
  ELMI_NSM_UNI_SERVICE_MAX
};


/* CVLAN/EVC MAP Types */
enum elmi_uni_mode 
{
  ELMI_UNI_MODE_UNIC = 100,
  ELMI_UNI_MODE_UNIN,
  ELMI_UNI_MODE_INVALID,
};

extern struct lib_globals *elmi_zg;
#define ZG  (elmi_zg)

struct debug_elmi
{
  u_int8_t event;
  u_int8_t packet;
  u_int8_t protocol;
  u_int8_t timer;
  u_int8_t tx;
  u_int8_t rx;
  u_int8_t nsm;
  u_int8_t detail;
};

/* Debug flags.  */
struct
{
  /* Debug flags for configuration. */
  struct debug_elmi conf;

  /* Debug flags for terminal. */
  struct debug_elmi term;

} debug;

/* ELMI protocol counters */
struct elmi_counters
{
  /* Realibility Errors */
  u_int32_t status_enq_timeouts;
  u_int32_t inval_seq_num;
  u_int32_t inval_status_resp;
  u_int32_t unsolicited_status_rcvd;

  /* Protocol errors */
  u_int32_t inval_protocol_ver;
  u_int32_t inval_msg_type;
  u_int32_t duplicate_ie;
  u_int32_t inval_mandatory_ie;
  u_int32_t unrecognized_ie;
  u_int32_t short_msg;
  u_int32_t inval_evc_ref_id;
  u_int32_t outof_seq_ie;
  u_int32_t mandatory_ie_missing;
  u_int32_t inval_non_mandatory_ie;
  u_int32_t unexpected_ie;
  pal_time_t last_full_status_enq_sent;
  pal_time_t last_full_status_rcvd;
  pal_time_t last_status_check_sent;
  pal_time_t last_status_check_rcvd;
  pal_time_t last_counters_cleared;
  pal_time_t last_full_status_enq_rcvd;
};


/* elmi master data structure */

struct elmi_master
{
  /* Pointer to VR.  */
  struct apn_vr *vr;

  /* Pointer to globals.  */
  struct lib_globals *zg;

  struct thread *elmi_rcv_thread;

  /* Pointer to Bridge list */
  struct list *bridge_list;

    /* Tree of all interfaces in the system */
  struct list *if_list;

  /* Debugging flags one per term and one per conf */
  struct
    {
      /* Debug flags for configuration. */
      struct debug_elmi conf;

      /* Debug flags for terminal. */
      struct debug_elmi term;

    } debug;

  
  pal_sock_handle_t sockfd;

  u_int8_t ether_type;

};

/* ELMI Bridge */
struct elmi_bridge
{
  struct elmi_master *elmi_master;

  /* VLAN tree indexed on vid. */
  struct route_table *vlan_table;

  /* Port list */
  struct avl_tree *port_tree;

  /* Bridge Name */
  u_int8_t name[L2_BRIDGE_NAME_LEN + 1];

  u_int8_t bridge_type; ///< Flag to indicate bridge type CE/PE/Other

  u_int8_t  elmi_enabled; ///< Flag to indicate elmi enable/disable state

};

struct elmi_vlan_bmp
{
  u_int32_t bitmap[ELMI_VLAN_BMP_WORD_MAX];
};


struct elmi_vlan
{
  u_int16_t vlan_id;
  u_int8_t vlan_name [ELMI_VLAN_NAMSIZ];
  u_int8_t bridge_name[L2_BRIDGE_NAME_LEN+1];
  u_int8_t flags;
};

struct elmi_ifp 
{
  struct interface *ifp;
  struct elmi_bridge *br;
  struct elmi_master *elmim;
  struct elmi_port *port;
  struct elmi_vlan_bmp *vlan_bmp;
  struct list *evc_status_list; ///<List of elmi_evc_status nodes
  struct elmi_uni_info *uni_info;
  struct list *cevlan_evc_map_list; ///<List of CE-vlan to EVC mapping node
  u_int32_t ifindex;
  u_int16_t pvid;

  bool_t active;
  u_int8_t dev_addr [ELMI_MAC_ADDR_LEN];
  u_int8_t elmi_enabled; ///< Flag to indicate elmi enable/disable state
  u_int8_t uni_mode;   ///< Flag to indicate UNI-C/UNI-N mode
  u_int8_t port_type; 
  u_int8_t bw_profile_level; ///< Flag to indicate BW profile type 
                             ///(Per UNI/EVC/EVCCOS) 
                             
  /* Field to maintain configured polling time interval */
  u_int8_t  polling_time;

  /* Field to maintain configured polling verification time interval */
  u_int8_t  polling_verification_time;

  /* Field to maintain configured status counter threshold value */
  s_int8_t  status_counter_limit;

  /* flag to disable PVT */
  u_int8_t  enable_pvt;

  /* Field to maintain configured polling counter threshold value */
  u_int16_t polling_counter_limit;

  /* Field to maintain Single EVC Asysnchronous Timer */
  u_int8_t  asynchronous_time;

};

/* Interface information */
struct elmi_port
{
  struct elmi_ifp  *elmi_if; 

  /* Polling Timer */
  struct thread  *polling_timer;

  /* Polling Verifcation Timer */
  struct thread  *polling_verification_timer;

  /* Timer to maintain minimum interval b/w async evc notifications */
  struct thread  *async_timer;

  /* Structure variable to maintain elmi counters */
  struct elmi_counters stats;

  /* list of evcs to send evc notifications */
  struct list *async_evc_list;

  /* Field to maintain data instance value */
  s_int32_t data_instance;

  /* Field to maintain running polling counter value */
  u_int16_t current_polling_counter;

  /* To store reference id of last evc sent/received */
  u_int16_t last_evc_id;  

  /* Field to maintain length of all bytes to be sent */
  u_int16_t total_length;

  /* Field to maintain running error status counter value */
  s_int8_t  current_status_counter;

  /* Field to maintain recent elmi success operations */ 
  u_int8_t  rcnt_success_oper_cnt;

  /* Field to maintain  report type */
  u_int8_t report_type;

  /* Field to store Seq num sent in last frame */
  u_int8_t sent_seq_num;
  
  /* Field to maintain last received send seq number */
  u_int8_t recvd_send_seq_num;

  /* Field to store response status from other end */
  u_int8_t rcvd_response;

  /* Field to store ELMI operational state */
  u_int8_t  elmi_operational_state;     

  /* Field to store last request type sent to UNI */
  u_int8_t  last_request_type;

  /* flag used for full status continued msg */
  u_int8_t  continued_msg;

  /* flag used for last ie */
  u_int8_t  last_ie_type;
#define ELMI_LAST_IE_EVC_TLV   1
#define ELMI_LAST_CEVLAN_EVC_MAP_TLV   2

  /* Request sent is true or false */
  u_int8_t  request_sent;
};

struct elmi_bw_profile
{
  /* Committed Information Rate (CIR) */
  u_int32_t cir;

  /* Committed Burst Size (CBS) */
  u_int32_t cbs;

  /* Excess Information Rate (EIR) */
  u_int32_t eir ;

  /* Excess Burst Size  (EBS) */
  u_int32_t ebs ;

  /* Color mode */
  u_int8_t cm;

  /* Coupling flag */
  u_int8_t cf;

  /* Per COS priority bits */
  u_int8_t per_cos;

  /* cos id used to store bw profile of each cos value set */
  u_int8_t cos_id;

  /* Magnitude and multiplier fields used to send cir, cbs, eir, ebs fields */
  u_int8_t cir_magnitude;
  u_int8_t cbs_magnitude; 
  u_int8_t cbs_multiplier; 
  u_int16_t eir_multiplier; 
  u_int16_t cir_multiplier;
  u_int8_t eir_magnitude;  
  u_int8_t ebs_magnitude;
  u_int8_t ebs_multiplier;  

};

/* UNI Status Information Element */
struct elmi_uni_info
{
  /* UNI Bandwidth Profile */
  struct elmi_bw_profile *uni_bw;

  u_int32_t ifindex;

  /* CE-VLAN ID/EVC Map Type */
  u_int8_t cevlan_evc_map_type;

  /* UNI id */
  u_int8_t uni_id[ELMI_UNI_NAME_LEN];
};

#define ELMI_PT_PT_EVC     0x0  
#define ELMI_MLPT_MLPT_EVC 0x1  

enum elmi_evc_status_type
{
  EVC_STATUS_NOT_ACTIVE = 0x0,
  EVC_STATUS_NEW_AND_NOTACTIVE,
  EVC_STATUS_ACTIVE,
  EVC_STATUS_NEW_AND_ACTIVE,
  EVC_STATUS_PARTIALLY_ACTIVE,
  EVC_STATUS_NEW_PARTIALLY_ACTIVE,
  EVC_STATUS_INVALID,
};

enum elmi_cfm_evc_status_type
{
  EVC_CFM_STATUS_NOT_UNKNOWN = 0x0,
  EVC_CFM_STATUS_ACTIVE,
  EVC_CFM_STATUS_PARTIALLY_ACTIVE,
  EVC_CFM_STATUS_NOTACTIVE,
};

#define ELMI_EVC_BW_PROFILE_PER_EVC  1
#define ELMI_EVC_BW_PROFILE_PER_COS  2

/* EVC Status Information Element structure */
/* elmi_evc_status is a node in the evc_status_list */
struct elmi_evc_status
{
    struct elmi_evc_status *next;
    struct elmi_evc_status **pprev;  

  /* EVC Reference ID, svid */
  u_int16_t evc_ref_id;

  /* EVC status type, (active/not-active/new/partially active etc) */
  u_int8_t evc_status_type;

  /* EVC parameters, PT_PT or MPT-MPT */
  u_int8_t evc_type;

  /* EVC ID */
  u_int8_t evc_id[ELMI_EVC_NAME_LEN+1];

  u_int8_t active;
  
  /* Per EVC or Per EVC COS Bandwidth Profile */
  u_int8_t bw_profile_level;

  /* Pointer to store EVC bandwidth profile details */
  struct elmi_bw_profile *evc_bw;

  /* Pointer to store EVC COS bandwidth profile details */
  struct elmi_bw_profile *evc_cos_bw[ELMI_MAX_COS_ID];
  u_int8_t cos_val_set[ELMI_MAX_COS_ID];

};

/* Information element CVLAN-EVC MAP */
struct elmi_cvlan_evc_map
{
  /* EVC Reference ID */
  u_int16_t evc_ref_id; 

  /* 1 bit - LAST IE */
  u_int8_t last_ie;  

  /* 0 bit - Reserve, 6bits- cvaln_evc_map_seq */
  u_int8_t cvlan_evc_map_seq; 

  /* 6 bits - reserve, 7 bit- untagged/priority tag */
  u_int8_t untag_or_pri_frame;

  /* Flag to maintain default evc id */
  u_int8_t default_evc;                     

  /* EVC MAP Entry */
  struct elmi_vlan_bmp cvlanbitmap;

};

/* Structure for elmi common fields */
struct elmi_pdu
{
  u_int8_t    proto_ver;
  u_int8_t    msg_type;
  u_int8_t    report_type;
  u_int8_t    rcvd_seq_num;
  u_int32_t   data_instance;
};


#define ELMI_TIMER_ON(T,F,S,V)                         \
  do {                                                 \
    if (! (T))                                         \
      (T) = thread_add_timer (ELMI_ZG, (F), (S), (V)); \
  } while (0)


#define ELMI_TIMER_OFF(T)               \
  do {                                  \
      if ((T) != NULL)                  \
        {                               \
          thread_cancel (T);            \
          (T) = NULL;                   \
        }                               \
  } while (0)

#endif /* _PACOS_ELMI_TYPES_H */
