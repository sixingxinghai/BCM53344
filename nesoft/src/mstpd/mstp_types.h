/* Copyright (C) 2003  All Rights Reserved.

   Multiple Spanning Tree Protocol types
   This file defines fundamental structures used by the MSTP deamon
   Section 8.5

    
*/

#ifndef __PACOS_MSTP_TYPES_H
#define __PACOS_MSTP_TYPES_H

#define MSG_TYPE_REPEATED_DESIGNATED  0
#define MSG_TYPE_CONFIRMED_ROOT       1
#define MSG_TYPE_SUPERIOR_DESIGNATED  2
#define MSG_TYPE_INFERIOR_DESIGNATED  3
#define MSG_TYPE_INFERIOR_ALTERNATE   4
#define MSG_TYPE_OTHER_MSG            5

#define MSTP_VLAN_NULL_VID     0
#define MSTP_VLAN_DEFAULT_VID  1
#define MSTP_VLAN_MAX_VID      4096

#define MSTP_PDU_SIZE          1500

#define MSTP_VLAN_PORT_MODE_INVALID   NSM_MSG_VLAN_PORT_MODE_INVALID
#define MSTP_VLAN_PORT_MODE_ACCESS    NSM_MSG_VLAN_PORT_MODE_ACCESS
#define MSTP_VLAN_PORT_MODE_HYBRID    NSM_MSG_VLAN_PORT_MODE_HYBRID
#define MSTP_VLAN_PORT_MODE_TRUNK     NSM_MSG_VLAN_PORT_MODE_TRUNK
#define MSTP_VLAN_PORT_MODE_CE        NSM_MSG_VLAN_PORT_MODE_CE
#define MSTP_VLAN_PORT_MODE_CN        NSM_MSG_VLAN_PORT_MODE_CN
#define MSTP_VLAN_PORT_MODE_PN        NSM_MSG_VLAN_PORT_MODE_PN
#define MSTP_VLAN_PORT_MODE_PE        NSM_MSG_VLAN_PORT_MODE_PE
#if ((defined HAVE_I_BEB) || defined (HAVE_B_BEB))
#define MSTP_VLAN_PORT_MODE_CNP       NSM_MSG_VLAN_PORT_MODE_CNP
#define MSTP_VLAN_PORT_MODE_VIP       NSM_MSG_VLAN_PORT_MODE_VIP
#define MSTP_VLAN_PORT_MODE_PIP       NSM_MSG_VLAN_PORT_MODE_PIP
#define MSTP_VLAN_PORT_MODE_CBP       NSM_MSG_VLAN_PORT_MODE_CBP
#define MSTP_VLAN_PORT_MODE_PNP       NSM_MSG_VLAN_PORT_MODE_PNP
#endif


#define MSTP_BRIDGE_TYPE_STP             NSM_BRIDGE_TYPE_STP
#define MSTP_BRIDGE_TYPE_STP_VLANAWARE   NSM_BRIDGE_TYPE_STP_VLANAWARE
#define MSTP_BRIDGE_TYPE_RSTP            NSM_BRIDGE_TYPE_RSTP
#define MSTP_BRIDGE_TYPE_RSTP_VLANAWARE  NSM_BRIDGE_TYPE_RSTP_VLANAWARE
#define MSTP_BRIDGE_TYPE_MSTP            NSM_BRIDGE_TYPE_MSTP
#define MSTP_BRIDGE_TYPE_PROVIDER_RSTP   NSM_BRIDGE_TYPE_PROVIDER_RSTP
#define MSTP_BRIDGE_TYPE_PROVIDER_MSTP   NSM_BRIDGE_TYPE_PROVIDER_MSTP
#define MSTP_BRIDGE_TYPE_CE              NSM_BRIDGE_TYPE_CE
#define MSTP_BRIDGE_TYPE_RPVST_PLUS      NSM_BRIDGE_TYPE_RPVST_PLUS
#define MSTP_BRIDGE_TYPE_BACKBONE_RSTP   NSM_BRIDGE_TYPE_BACKBONE_RSTP
#define MSTP_BRIDGE_TYPE_BACKBONE_MSTP   NSM_BRIDGE_TYPE_BACKBONE_MSTP

#if (defined HAVE_I_BEB )
#define MSTP_BRIDGE_TYPE_CNP             NSM_BRIDGE_TYPE_CNP
#endif

/* INSTANCE bitmap manipulation macros. */
#define MSTP_INSTANCE_MAX                           4094        /* Max INSTANCEs. */
#define MSTP_INSTANCE_MIN                           1        /* MIN INSTANCEs for CLI*/
#define MSTP_INSTANCE_BMP_WORD_WIDTH                32
#define MSTP_INSTANCE_BMP_WORD_MAX                  \
((MSTP_INSTANCE_MAX + MSTP_INSTANCE_BMP_WORD_WIDTH) / MSTP_INSTANCE_BMP_WORD_WIDTH)

#define MSTP_INSTANCE_BMP_INIT(bmp)                                             \
  do {                                                                     \
      pal_mem_set ((bmp).bitmap, 0, sizeof ((bmp).bitmap));                \
  } while (0)

#define MSTP_INSTANCE_BMP_SET(bmp, instance)                                         \
do {                                                                     \
    int _word = (instance) / MSTP_INSTANCE_BMP_WORD_WIDTH;                       \
      (bmp).bitmap[_word] |= (1U << ((instance) % MSTP_INSTANCE_BMP_WORD_WIDTH));  \
} while (0)

#define MSTP_INSTANCE_BMP_UNSET(bmp, instance)                                       \
do {                                                                     \
    int _word = (instance) / MSTP_INSTANCE_BMP_WORD_WIDTH;                       \
      (bmp).bitmap[_word] &= ~(1U <<((instance) % MSTP_INSTANCE_BMP_WORD_WIDTH));  \
} while (0)

#define MSTP_INSTANCE_BMP_IS_MEMBER(bmp, instance)                                   \
((bmp).bitmap[(instance) / MSTP_INSTANCE_BMP_WORD_WIDTH] & (1U << ((instance) % MSTP_INSTANCE_BMP_WORD_WIDTH)))

struct mstp_protection_bmp 
{
    u_int32_t bitmap[MSTP_INSTANCE_BMP_WORD_MAX];
};

/* VLAN bitmap manipulation macros. */
#define MSTP_VLAN_MAX                           4094        /* Max VLANs. */
#define MSTP_VLAN_MIN                           2        /* MIN VLANs for CLI*/
#define MSTP_VLAN_BMP_WORD_WIDTH                32
#define MSTP_VLAN_BMP_WORD_MAX                  \
        ((MSTP_VLAN_MAX + MSTP_VLAN_BMP_WORD_WIDTH) / MSTP_VLAN_BMP_WORD_WIDTH)

#define MSTP_VLAN_BMP_INIT(bmp)                                             \
   do {                                                                     \
       pal_mem_set ((bmp).bitmap, 0, sizeof ((bmp).bitmap));                \
   } while (0)

#define MSTP_VLAN_BMP_SET(bmp, vid)                                         \
   do {                                                                     \
        int _word = (vid) / MSTP_VLAN_BMP_WORD_WIDTH;                       \
        (bmp).bitmap[_word] |= (1U << ((vid) % MSTP_VLAN_BMP_WORD_WIDTH));  \
   } while (0)

#define MSTP_VLAN_BMP_UNSET(bmp, vid)                                       \
   do {                                                                     \
        int _word = (vid) / MSTP_VLAN_BMP_WORD_WIDTH;                       \
        (bmp).bitmap[_word] &= ~(1U <<((vid) % MSTP_VLAN_BMP_WORD_WIDTH));  \
   } while (0)

#define MSTP_VLAN_BMP_IS_MEMBER(bmp, vid)                                   \
  ((bmp).bitmap[(vid) / MSTP_VLAN_BMP_WORD_WIDTH] & (1U << ((vid) % MSTP_VLAN_BMP_WORD_WIDTH)))

struct mstp_vlan_bmp
{
  u_int32_t bitmap[MSTP_VLAN_BMP_WORD_MAX];
};

#ifdef HAVE_I_BEB
#define PBB_ETHER_TYPE 0x88a8
#define PBB_TPID 0x88e7
#define CUSTOMER_SRC_MAC_OFFSET  16 
#define CUSTOMER_DST_MAC_OFFSET  10 
#define MSTP_SVID_OFFSET         36 
#define MSTP_PB_PACKET_OFFSET    22 
#define MSTP_PBB_ISID_OFFSET     6
#endif

/* For legacy systems 802.1s uses the master role in its place*/
#define ROLE_UNKNOWN 0

enum port_role {
  ROLE_MASTERPORT=0,
  ROLE_ALTERNATE,
  ROLE_ROOTPORT,
  ROLE_DESIGNATED,
  ROLE_DISABLED,
  ROLE_BACKUP
};

enum port_state {
  STATE_DISCARDING=0,
  STATE_LISTENING,
  STATE_LEARNING,
  STATE_FORWARDING,
  STATE_BLOCKING
};

enum info_type {
  INFO_TYPE_DISABLED=0,
  INFO_TYPE_AGED,
  INFO_TYPE_MINE,
  INFO_TYPE_RECEIVED
};

enum port_bpduguard
{
  MSTP_PORT_PORTFAST_BPDUGUARD_ENABLED = 0,
  MSTP_PORT_PORTFAST_BPDUGUARD_DISABLED,
  MSTP_PORT_PORTFAST_BPDUGUARD_DEFAULT,
};

enum port_bpdufilter
{
  MSTP_PORT_PORTFAST_BPDUFILTER_ENABLED = 0,
  MSTP_PORT_PORTFAST_BPDUFILTER_DISABLED,
  MSTP_PORT_PORTFAST_BPDUFILTER_DEFAULT,
};

/* We do not use TYPE_MST to transmit but to validate on recv.*/
enum bpdu_type
{
  BPDU_TYPE_CONFIG =  0x00, 
  BPDU_TYPE_RST =     0x02,
  BPDU_TYPE_MST =     0x03, 
  BPDU_TYPE_TCN =     0x80
};


enum add_type
{
  TYPE_EXPLICIT,
  TYPE_IMPLICIT
};

enum tc_state
{
  TC_INACTIVE,
  TC_ACTIVE
};

enum mstp_topology
{
  MSTP_TOPOLOGY_NONE,
  MSTP_TOPOLOGY_RING
};

enum rpvst_bpdu_event
{
  RPVST_PLUS_TIMER_EVENT = 1,
  RPVST_PLUS_RCVD_EVENT
};

enum rpvst_bpdu_type
{
  RPVST_PLUS_BPDU_TYPE_CIST = 1,
  RPVST_PLUS_BPDU_TYPE_UNTAGGED,
  RPVST_PLUS_BPDU_TYPE_TAGGED,
  RPVST_PLUS_BPDU_TYPE_SSTP
};

struct rpvst_vlan_tlv
{
  u_int16_t tlv_type;
  u_int16_t tlv_length;
  u_int16_t vlan_id;
};


struct mstp_port
{
  /* Housekeeping */
  struct mstp_port *          next;
  struct mstp_port **         pprev;

  struct mstp_instance_port *      instance_list;
  struct mstp_bridge *             br;

  struct mstp_bridge *             ce_br;

  u_char                      dev_addr[ETHER_ADDR_LEN];
  u_int32_t                   ifindex;
  char                        name[L2_IF_NAME_LEN];
 
  u_char 	              tx_count;
  u_int32_t 		      total_tx_count;
  u_int32_t		      rx_count;

  u_char                      force_version;

  u_char                      info_internal:1;
  u_char                      rcvd_internal:1;

  u_char                      admin_p2p_mac:2;
  u_char                      oper_p2p_mac:1;
  u_char                      admin_edge:1;
  u_char                      oper_edge:1;
  u_char                      auto_edge:1;

  u_char                      portfast_conf:1;
  u_char                      port_enabled:1;
  u_char                      rcvd_mstp:1;
  u_char                      rcvd_stp:1;
  u_char                      rcvd_rstp:1;
  u_char                      send_mstp:1;
  u_char                      tc_ack:1;
  

  u_char                      selected:1;
  u_char                      send_proposal:1;
  u_char                      rcv_proposal:1;
  u_char                      reselect:1; /* Section 17.18.29 */
  u_char                      updtInfo:1;
  u_char                      pathcost_configured:1;
  u_char                      disputed:1;
  u_char                      agree:1;
  u_char                      agreed:1;
  u_char                      config_bpdu_pending:1;
#ifdef HAVE_HA
  u_char                      ha_stale:1;
#endif /* HAVE_HA */

#ifdef HAVE_L2GP  
  u_char                      isL2gp; /* 802.1ah-d4-1 13.25.21 */
  u_char                      enableBPDUrx; /* 802.1ah-d4-1 13.25.18 */
  u_int8_t                    enableBPDUtx; /* 802.1ah-d4-1 13.25.19 */
  struct bridge_id            psuedoRootId; /* 802.1ah-d4-1 13.25.20 */
#endif /* HAVE_L2GP */

  /* L2 security related information */
  s_int32_t                   admin_bpduguard;
  u_char                      oper_bpduguard;
  struct thread *             errdisable_timer;
  u_char                      admin_bpdufilter;
  u_char                      oper_bpdufilter;
  u_char                      admin_rootguard;
  u_char                      oper_rootguard;
  u_char                      cisco_cfg_format_id;

  bool_t                      restricted_role;
  bool_t                      restricted_tcn;

  s_int32_t                   hello_time;

  char                        ref_count;
  enum add_type               type;
  char                        port_type;
  char                        any_msti_rootport;
  char                        any_msti_desigport;

  u_int32_t                   cist_path_cost; /*  */
  s_int16_t                   cist_priority;
  u_int16_t                   cist_port_id;  /* Combo of ifindex and priority */

  /* CSTI  Port Priority Vector */
  struct bridge_id            cist_root;
  struct bridge_id            cist_reg_root;
  struct bridge_id            cist_rcvd_reg_root;
  struct bridge_id            cist_designated_bridge;
  u_int32_t                   cist_external_rpc;
  u_int32_t                   cist_internal_rpc;
  s_int32_t                   cist_designated_port_id;

  /* CSTI  Port times variable */
  s_int32_t                    cist_message_age; 
  s_int32_t                    cist_max_age;
  s_int32_t                    cist_fwd_delay;
  s_int32_t                    cist_hello_time;
  s_int32_t                    hop_count;

  bool_t                       newInfoCist;
  bool_t                       newInfoMsti;
  bool_t                       critical_bpdu;

  enum port_state             cist_state;
  enum port_state             cist_next_state;
  enum info_type              cist_info_type; /* not needed probably */
  enum port_role              cist_role;
  enum port_role              cist_selected_role;
  enum tc_state               cist_tc_state; 
  

  /* Flags */
  /* State Machine Variables */
 
  u_char                      helloWhen; 

  /* port timer  */
  struct thread *             port_timer;

  /* edgeDelayWhile timer Section 17.17.1 */
  struct thread *             edge_delay_timer; 

  /* fdWhile timer Section 17.17.2 */
  struct thread *             forward_delay_timer;

  /* helloWhen timer Section 17.17.3 */
  struct thread *             hello_timer;

  /* mdelayWhile Timer Section 17.17.4 */
  struct thread *             migrate_timer;
 
  /* rbWhile timer Section 17.17.5 */
  struct thread *             recent_backup_timer;

  /* rcvdInfoWhile Section 17.17.6 */
  struct thread *             message_age_timer;

  /* rrWhile timer Section 17.17.7 */
  struct thread *             recent_root_timer;

  /* tcWhile timer Section 17.17.8 */
  struct thread *             tc_timer; 

  struct thread *             hold_timer;

  /* Statistics */
  u_int32_t                   cist_forward_transitions;

  /* VLAN Membership of the port. Is CVLAN is case of.
   * CE port and is SVLAN in the case of CN and PN port
   */
  struct mstp_vlan_bmp        vlanMemberBmp;

  /* SVLAN Membership of the CE port. This is used to
   * keep track of the logical PE port
   */
  struct mstp_vlan_bmp        svlanMemberBmp;

  /* SVLAN ID of the Provider Edge Port */

  u_int16_t                   svid;

  u_int8_t                    spanning_tree_disable;

#if (defined HAVE_I_BEB )
  struct mstp_bridge         *cn_br;

  uint32_t                    isid;

  uint32_t                    pip_port; /* if_index */

  uint32_t                    bvid;

  struct list                 * svid_list;

#endif


  /* only used for rpvst_plus */
#ifdef HAVE_RPVST_PLUS
  /* Hello Timer fired or Rcvd superior bpdu */
  enum rpvst_bpdu_event       rpvst_event;

  /* BPDU type is 8021.D(CIST)  or SSTP(for each vlan) */
  enum rpvst_bpdu_type        rpvst_bpdu_type;

  /* Set this flag when new bpdu for a vlan to be tx */
  bool_t                    newInfoSstp;

  /* Vid to be used in 802.1Q tag for SSTP Bpdus */
  u_int16_t                   vid_tag;
  int                      default_vlan;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
  HA_CDR_REF mstp_port_cdr_ref; /* CDR ref for mstp_port */
  HA_CDR_REF errdisable_timer_cdr_ref; /* CDR ref for errdisable timer */
  HA_CDR_REF port_timer_cdr_ref; /* CDR ref for port_timer */
  HA_CDR_REF edge_delay_timer_cdr_ref; /* CDR ref for edge_delay timer */
  HA_CDR_REF forward_delay_timer_cdr_ref; /* CDR ref for forward_delay timer */
  HA_CDR_REF hello_timer_cdr_ref; /* CDR ref for hello timer */
  HA_CDR_REF migrate_timer_cdr_ref; /* CDR ref for migrate timer */
  HA_CDR_REF recent_backup_timer_cdr_ref; /* CDR ref for recent_backup timer */
  HA_CDR_REF message_age_timer_cdr_ref; /* CDR ref for message_age timer */
  HA_CDR_REF recent_root_timer_cdr_ref; /* CDR ref for recent_root timer */
  HA_CDR_REF tc_timer_cdr_ref; /* CDR ref for tc timer */
  HA_CDR_REF hold_timer_cdr_ref; /* CDR ref for hold timer */
#endif /* HAVE_HA */
 /* statistics */
 u_int32_t conf_bpdu_sent;
 u_int32_t conf_bpdu_rcvd;
 u_int32_t tcn_bpdu_sent;
 u_int32_t tcn_bpdu_rcvd;
 u_int32_t src_mac_count; 
 u_int32_t similar_bpdu_cnt;
 u_int32_t msg_age_timer_cnt;
 u_int32_t total_src_mac_count;
};

  /* per instance */
struct mstp_instance_port
{
  /* Housekeeping */
  /* own instance list of all ports */
  struct mstp_instance_port *          next;
  struct mstp_instance_port **         pprev;
  struct mstp_port *cst_port;

  struct mstp_bridge_instance *       br;

  /* All instances list of the same port  */
  struct mstp_instance_port *          next_inst;
  struct mstp_instance_port **         pprev_inst;
  

  /* following 2 vars are used by the instance zero to 
   * keep track of the order of insertion */
  char                                ref_count;
  char                                instance_id;
  enum add_type                       type;

  u_int32_t                   ifindex;
  s_int16_t                   msti_priority;
  u_int16_t                   msti_port_id; /* Combo of ifindex and priority */
  u_int32_t                   msti_path_cost; /*  */

  /* MSTI Port Priority Vector */
  u_int32_t                   internal_root_path_cost;
  struct bridge_id            msti_root;
  struct bridge_id            designated_bridge;
  s_int32_t                   designated_port_id;

  /* MSTI Port times variable */
  s_int32_t                    message_age; 
  s_int32_t                    max_age;
  s_int32_t                    fwd_delay;
  s_int32_t                    hello_time;
  s_int32_t                    hop_count;

  bool_t                      restricted_role;
  bool_t                      restricted_tcn;

  /* Section 8.5.5 */
  enum port_state             state;
  enum port_state             next_state;
  enum info_type              info_type;
  enum port_role              role;
  enum port_role              selected_role;
  enum tc_state               msti_tc_state; 
  

  /* Flags */
  u_char                      topology_change:1;
  u_char                      send_proposal:1;
  u_char                      reselect:1; /* Section 17.18.29 */
  u_char                      updtInfo:1;
  u_char                      pathcost_configured:1; 
  
  /* State Machine Variables */
  u_char                      rcv_proposal:1;
  u_char                      selected:1;
  u_char                      changed_master:1;
  u_char                      msti_mastered:1;
  u_char                      agree:1;
  u_char                      agreed:1;
  u_char                      disputed:1;
  

  /* fdWhile timer Section 17.17.2 */
  struct thread *             forward_delay_timer;

  /* rbWhile timer Section 17.17.5 */
  struct thread *             recent_backup_timer;

  /* rcvdInfoWhile Section 17.17.6 */
  struct thread *             message_age_timer;

  /* rrWhile timer Section 17.17.7 */
  struct thread *             recent_root_timer;

  /* tcWhile timer Section 17.17.8 */
  struct thread *             tc_timer; 

#ifdef HAVE_RPVST_PLUS
  u_char                      tc_ack:1; 
  bool_t                      newInfoSstp;
  char                        any_msti_rootport;
  char                        any_msti_desigport;
  u_int16_t                   vlan_id;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
  HA_CDR_REF mstp_instance_port_cdr_ref; /* CDR ref for mstp_instance_port */
  HA_CDR_REF forward_delay_timer_cdr_ref; /* CDR ref for forward_delay timer */
  HA_CDR_REF recent_backup_timer_cdr_ref; /* CDR ref for recent_backup timer */
  HA_CDR_REF message_age_timer_cdr_ref; /* CDR ref for message_age timer */
  HA_CDR_REF recent_root_timer_cdr_ref; /* CDR ref for recent_root timer */
  HA_CDR_REF tc_timer_cdr_ref; /* CDR ref for tc timer */
#endif /* HAVE_HA */

 /* statistics variables */
 u_int32_t conf_bpdu_sent;
 u_int32_t conf_bpdu_rcvd;
 u_int32_t tcn_bpdu_sent;
 u_int32_t tcn_bpdu_rcvd;
 u_int32_t similar_bpdu_cnt;
};

/*This is the element of the list
  since we are going to store vid 
  we are using vid_t for ease of understanding
*/
typedef u_int16_t      mstp_vid_t;

/* This is a range list.A customized data structure to maintain 
    mostly contiguous elements.
   e.g. for elements 2 3 4 5 the list would contain 2-5 
        for 2 4 5 6 7 10 11 12 the list would contain 2, 4-7,10-12 
*/
struct rlist_info
{
  struct rlist_info *next;
  mstp_vid_t hi;
  mstp_vid_t lo;
  mstp_vid_t vlan_range_indx;
};

struct mstp_bridge_instance
{
  /* Housekeeping variables */
  struct mstp_bridge_instance *               next;
  struct mstp_bridge_instance * *             pprev;
  struct mstp_bridge *                        bridge;

  /* Each instance has its own port list
   * the IST has all ports added to it */
  struct mstp_instance_port *            port_list;

  struct rlist_info *            vlan_list;
  struct rlist_info *            fid_list;

  char                           instance_id;

  /* Bitmap for Vlan Range Index, used by snmp */
  struct bitmap *vlan_range_index_bmp;
    
  /* Index of the lowest numbered port (by ifindex) */
  s_int32_t                      low_port;         
 
  /* indicates if the bridge has selected one of its ports 
   * as master ports */
  u_char                        master:1 ;
  u_char                        learning_enabled:1;
  u_char                        msti_mastered:1;
  u_char                        reselect:1;
  /*L2-R3 */
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
  u_char                        mstp_enabled:1; 
#endif /*(HAVE_PROVIDER_BRIDGE) ||(HAVE_B_BEB)*/

  struct bridge_id              msti_bridge_id;
  u_int32_t                     msti_bridge_priority;

  /* Index ofthe port requesting other recent roots to revert to 
     discarding state */
  u_int32_t                     recent_root; 

  /* Recent Root Timer Count */
  u_int32_t                     br_inst_all_rr_timer_cnt;

  /* MSTP -MST root priority vector sec 17.4.2.2 and sec 17.17.6*/
  struct bridge_id              msti_designated_root;
  struct bridge_id              msti_designated_bridge;
  u_int32_t                     internal_root_path_cost;
  u_int16_t                     msti_root_port_id;
  struct mstp_instance_port *   root_inst_port;
  u_int32_t                     msti_root_port_ifindex;

  s_int16_t                     port_index;
  
  s_int32_t                     hop_count;


   /* statistics for instance/vlan level */
  bool_t                        tc_flag;
  bool_t                        topology_change_detected;
  pal_time_t                    time_last_topo_change;
  u_int32_t                     num_topo_changes;
  u_int32_t                     total_num_topo_changes;
  u_int16_t                     tc_initiator;
  u_char                        tc_last_rcvd_from[ETHER_ADDR_LEN];
 

#ifdef HAVE_RPVST_PLUS
  u_int16_t                     vlan_id;
  /* MSTI Port times variable */
  s_int32_t                    message_age; 
  s_int32_t                    max_age;
  s_int32_t                    fwd_delay;
  s_int32_t                    hello_time;
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_HA
  HA_CDR_REF mstp_bridge_instance_cdr_ref; /* CDR ref for mstp_bridge_instance */
#endif /* HAVE_HA */

  s_int32_t is_te_instance;
  
};


struct mstp_config_info
{
  u_char        cfg_format_id;      /* format selector */
  char cfg_name[MSTP_CONFIG_NAME_LEN + 1]; /* SNMP admin string */
  u_int16_t      cfg_revision_lvl;
  char cfg_digest[MSTP_CONFIG_DIGEST_LEN]; 
};

/*struct vlan_info
{
  u_char        enabled:1;
  u_char        mst_id:6;
};
  
*/

/* For vlan table key. */
struct mstp_prefix_vlan
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  s_int32_t vid;
};

#define MSTP_PREFIX_VLAN_SET(P,I)                                            \
   do {                                                                      \
       pal_mem_set ((P), 0, sizeof (struct mstp_prefix_vlan));               \
       (P)->family = AF_INET;                                                \
       (P)->prefixlen = 32;                                                  \
       (P)->vid = pal_hton32 (I);                                            \
   } while (0)

#define MSTP_VLAN_INFO_SET(R,V)                                              \
   do {                                                                      \
      (R)->info = (V);                                                       \
      route_lock_node (R);                                                   \
   } while (0)

#define MSTP_VLAN_INFO_UNSET(R)                                              \
   do {                                                                      \
     (R)->info = NULL;                                                       \
     route_unlock_node(R);                                                   \
   } while (0)

/* VLAN structure. */
struct mstp_vlan
{
  u_int16_t vid;
  char      name[NSM_VLAN_NAMSIZ];
  char      bridge_name[L2_BRIDGE_NAME_LEN+1];
  u_char    flags;
#define MSTP_VLAN_SUSPEND                 0
#define MSTP_VLAN_ACTIVE                  (1 << 0)
#define MSTP_VLAN_INSTANCE_CONFIGURED     (1 << 1)

#ifdef HAVE_HA
#define MSTP_VLAN_HA_STALE                (1 << 2)
#endif /* HAVE_HA */

#if defined HAVE_G8031 || defined HAVE_G8032
#define MSTP_VLAN_PG_CONFIGURED           (1 << 3)
#endif /* HAVE_G8031 || HAVE_G8032*/

  s_int32_t instance; 
#ifdef HAVE_PVLAN
  u_int16_t pvlan_type;

#define MSTP_PVLAN_NONE                 0
#define MSTP_PVLAN_PRIMARY              1
#define MSTP_PVLAN_SECONDARY            2

  struct mstp_vlan_bmp secondary_vlan_bmp;
#endif /* HAVE_PVLAN */

#ifdef HAVE_HA
  HA_CDR_REF mstp_vlan_cdr_ref; /* CDR ref for mstp_vlan */
#endif /* HAVE_HA */
};

/* Section 8.5.3 */
struct mstp_bridge
{
  /* Housekeeping variables */
  struct mstp_bridge *          next;
  struct mstp_bridge * *        pprev;
  struct mstp_port *            port_list;

  /* VLAN tree indexed on vid. */
  struct route_table            *vlan_table;

  struct mstp_config_info       config;

  char                          name[L2_BRIDGE_NAME_LEN+1];

  u_int8_t                      type;

 /* List of vlans added o the common instance */ 
  struct rlist_info *           vlan_list;
  struct rlist_info *           fid_list;

  /* Index of the lowest numbered port (by ifindex) */
  s_int32_t                     low_port;

  /* Since the priority is diff for each instance we just store the mac addr
   * i.e. the addr of the lowest indexed port from which bridge id 
   * is calculated by each instance */
  struct mac_addr               bridge_addr;
  struct bridge_id              cist_bridge_id;

  /* MSTP -CIST root priority vector sec 13.23.3 IEEE 802.1s */
  struct bridge_id              cist_designated_root;
  struct bridge_id              cist_reg_root;
  struct bridge_id              cist_rcvd_reg_root;
  struct bridge_id              cist_designated_bridge;
  u_int32_t                     external_root_path_cost;
  u_int32_t                     internal_root_path_cost;
  u_int16_t                     cist_root_port_id;
  u_int32_t                     cist_root_port_ifindex;
  u_int32_t                     bpdu_recv_port_ifindex;
  struct mstp_port*             root_port;
  struct mstp_port*             alternate_port;

  /* Working values - CIST root times sec 17.17.7 */
  s_int32_t                     cist_max_age;
  s_int32_t                     cist_message_age;
  s_int32_t                     cist_hello_time;
  s_int32_t                     cist_forward_delay;
  s_int32_t                     hop_count;

  u_char                        force_version;

  /* Configuration values- bridge times sec 17.17.4  */
  s_int32_t                     bridge_max_age;
  s_int32_t                     bridge_hello_time;
  s_int32_t                     bridge_forward_delay;
  s_int32_t                     bridge_max_hops;


  u_int32_t                     cist_bridge_priority;

  u_int32_t                     recent_root;

  /* Recent Root Timer Count */
  u_int32_t                     br_all_rr_timer_cnt;
  
  /* How long dynamic entries are kept. Default 300s */
  s_int32_t                     ageing_time;   

  u_char                        learning_enabled;
  /* BPDU flags - see 802.1d */
  u_char                        mstp_enabled:1;
  u_char                        mstp_brforward:1;
  u_char                        topology_change:1;
  u_char                        topology_change_detected:1;
  u_char                        bridge_enabled:1;
  u_char                        is_vlan_aware:1;
  u_char                        reselect:1;
  u_char                        ageing_time_is_fwd_delay:1;
  u_char                        ce_bridge:1;
#if (defined HAVE_I_BEB)
  u_char                        cn_bridge:1;
#endif

  u_char                        is_default:1;
#ifdef HAVE_HA
  u_char                        ha_stale:1;
#endif /* HAVE_HA */

  /* Used for Path cost method Short/Long */
  u_int8_t                      path_cost_method;

  s_int16_t                     port_index;

#ifdef HAVE_RPVST_PLUS
  struct mstp_bridge_instance   *instance_list[RPVST_MAX_INSTANCES];
#else
  struct mstp_bridge_instance   *instance_list[MST_MAX_INSTANCES];
#endif /* HAVE_RPVST_PLUS */

  u_int16_t max_instances;

  s_int16_t                     num_ports;

  /* Statistics */
  pal_time_t   time_last_topo_change;
  u_int32_t    num_topo_changes;
  u_int32_t    total_num_topo_changes;
  u_int32_t    max_age_count; 
  
 /* L2 security related parameters */
  unsigned char                 bpduguard;
  s_int32_t                     errdisable_timeout_interval;
  unsigned char                 errdisable_timeout_enable;
  unsigned char                 bpdu_filter;
  unsigned char                 oper_cisco;
  unsigned char                 admin_cisco;
  unsigned char                 transmit_hold_count;

  /* Topology type - none/ring - currently used to support RRSTP */
  enum mstp_topology topology_type;

  struct thread *               topology_change_timer;
  struct thread *               tcn_timer;

  /* statistics for CIST level */
  bool_t                        tc_flag;
  u_int16_t                     tc_initiator;
  u_char                        tc_last_rcvd_from[ETHER_ADDR_LEN];

#ifdef HAVE_RPVST_PLUS
  u_int16_t vlan_instance_map[RPVST_MAX_INSTANCES];
#endif /* HAVE_RPVST_PLUS */

#if defined(HAVE_PROVIDER_BRIDGE) || defined(HAVE_I_BEB)

#define MSTP_BRIDGE_PROTO_TUNNEL    NSM_MSG_BRIDGE_PROTO_TUNNEL
#define MSTP_BRIDGE_PROTO_DISCARD   NSM_MSG_BRIDGE_PROTO_DISCARD
#define MSTP_BRIDGE_PROTO_PEER      NSM_MSG_BRIDGE_PROTO_PEER

  /* Processing of Customer BPDUs on customer edge port */
  u_int8_t                    cust_bpdu_process;
#endif /* HAVE_PROVIDER_BRIDGE || HAVE_I_BEB */

#ifdef HAVE_HA
  HA_CDR_REF mstp_bridge_cdr_ref; /* CDR ref for mstp_bridge */
  HA_CDR_REF topology_change_timer_cdr_ref; /* CDR ref for topology_change_timer */
  HA_CDR_REF tcn_timer_cdr_ref; /* CDR ref for tcn_timer */
#endif /* HAVE_HA */

#if defined HAVE_G8031 || defined HAVE_G8032
  struct mstp_protection_bmp mstpProtectionBmp;
#endif /* HAVE_G8031  || defined HAVE_G8032 */
  
};

#endif
