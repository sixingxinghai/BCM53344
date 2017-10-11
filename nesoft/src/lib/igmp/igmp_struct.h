/* Copyright (C) 2001-2005  All Rights Reserved. */

#ifndef _PACOS_IGMP_STRUCT_H
#define _PACOS_IGMP_STRUCT_H

/* IGMP Interface Update Modes */
enum igmp_if_update_mode
{
  IGMP_IF_UPDATE_MCAST_ENABLE = 1,
  IGMP_IF_UPDATE_MCAST_DISABLE,
  IGMP_IF_UPDATE_MCAST_LMEM_MANUAL,
  IGMP_IF_UPDATE_L2_MCAST_ENABLE,
  IGMP_IF_UPDATE_L2_MCAST_DISABLE,
  IGMP_IF_UPDATE_CONFIG_ENABLE,
  IGMP_IF_UPDATE_CONFIG_DISABLE,
  IGMP_IF_UPDATE_CONFIG_RESTART,
  IGMP_IF_UPDATE_CONFIG_MROUTE_RESTART,
  IGMP_IF_UPDATE_CONFIG_PROXY_RESTART,
  IGMP_IF_UPDATE_IFF_STATE,
  IGMP_IF_UPDATE_IFF_ADDR_ADD,
  IGMP_IF_UPDATE_IFF_ADDR_DEL
};

/* IGMP Service Types */
enum igmp_svc_type
{
  IGMP_SVC_TYPE_INVALID = 0,
  IGMP_SVC_TYPE_L2,
  IGMP_SVC_TYPE_L3,
  IGMP_SVC_TYPE_MAX
};

/* IGMP Router-Filter-Mode State Enumeration */
enum igmp_filter_mode_state
{
  IGMP_FMS_INVALID = 0,
  IGMP_FMS_INCLUDE,
  IGMP_FMS_EXCLUDE,
  IGMP_FMS_MAX
};

/* IGMP Group Router Filter Mode FSM Events Enumeration */
enum igmp_filter_mode_event
{
  IGMP_FME_INVALID = 0,
  IGMP_FME_MODE_IS_INCL,
  IGMP_FME_MODE_IS_EXCL,
  IGMP_FME_CHG_TO_INCL,
  IGMP_FME_CHG_TO_EXCL,
  IGMP_FME_ALLOW_NEW_SRCS,
  IGMP_FME_BLOCK_OLD_SRCS,
  IGMP_FME_GROUP_TIMER_EXPIRY,
  IGMP_FME_SOURCE_TIMER_EXPIRY,
  IGMP_FME_IMMEDIATE_LEAVE,
  IGMP_FME_MANUAL_CLEAR,
  IGMP_FME_MANUAL_LMEM_UPDATE
};

/* IGMP Host-side Filter-Mode State Enumeration */
enum igmp_hst_filter_mode_state
{
  IGMP_HFMS_INVALID = IGMP_FMS_INVALID,
  IGMP_HFMS_INCLUDE = IGMP_FMS_INCLUDE,
  IGMP_HFMS_EXCLUDE = IGMP_FMS_EXCLUDE,
  IGMP_HFMS_MAX = IGMP_FMS_MAX
};

/* IGMP Host-side Filter-Mode FSM Events Enumeration */
enum igmp_hst_filter_mode_event
{
  IGMP_HFME_INVALID = 0,
  IGMP_HFME_INCL,
  IGMP_HFME_EXCL,
  IGMP_HFME_MFC_MSG,
  IGMP_HFME_MANUAL_CLEAR
};

/* IGMP Host-side Record Types Enumeration */
enum igmp_hst_rec_type
{
  IGMP_HRT_INVALID = IGMP_FME_INVALID,
  IGMP_HRT_MODE_IS_INCL = IGMP_FME_MODE_IS_INCL,
  IGMP_HRT_MODE_IS_EXCL = IGMP_FME_MODE_IS_EXCL,
  IGMP_HRT_CHG_TO_INCL = IGMP_FME_CHG_TO_INCL,
  IGMP_HRT_CHG_TO_EXCL = IGMP_FME_CHG_TO_EXCL,
  IGMP_HRT_ALLOW_NEW_SRCS = IGMP_FME_ALLOW_NEW_SRCS,
  IGMP_HRT_BLOCK_OLD_SRCS = IGMP_FME_BLOCK_OLD_SRCS
};

/* IGMP Group Source Query-Types Enumeration */
enum igmp_gs_query_type
{
  IGMP_GS_QUERY_INVALID = 0,
  IGMP_GS_QUERY_SFLAG_SET,
  IGMP_GS_QUERY_SFLAG_UNSET,
  IGMP_GS_QUERY_DONE
};

/* IGMP Svc-User IF-ID Callback Function Type */
typedef s_int32_t
(* igmp_cback_if_su_id_t) (u_int32_t ,
                           struct interface *,
                           u_int16_t ,
                           u_int32_t *);

/* IGMP VIF Control Callback Function Type */
typedef s_int32_t
(* igmp_cback_vif_ctl_t) (u_int32_t ,
                          struct interface *,
                          u_int32_t if_su_id,
                          struct prefix *,
                          bool_t );

/* IGMP Local-Membership Update Callback Function Type */
typedef s_int32_t
(* igmp_cback_lmem_update_t) (u_int32_t ,
                              struct interface *,
                              u_int32_t if_su_id,
                              enum igmp_filter_mode_state ,
                              struct pal_in4_addr *,
                              u_int16_t ,
                              struct pal_in4_addr []);

/* IGMP Mcast-Fwdr-Cache Programming Callback Function Type */
typedef s_int32_t
(* igmp_cback_mfc_prog_t) (u_int32_t ,
                           bool_t ,
                           bool_t ,
                           struct pal_in4_addr *,
                           struct pal_in4_addr *,
                           struct interface *,
                           u_int32_t if_su_id,
                           struct list *);

/* IGMP MRouter IF Update Callback Function Type*/
typedef s_int32_t
(* igmp_cback_mrt_if_update_t) (u_int32_t,
                                struct interface *,
                                u_int32_t if_su_id,
                                bool_t);


/* IGMP Tunnel Interface Get Callback Function Type */
typedef struct interface *
(* igmp_cback_tunnel_get_t) (u_int32_t ,
                             u_int8_t *);

/* IGMP Instance Structure */
struct igmp_instance
{
  /* Owning module's Library globals */
  struct lib_globals *igi_lg;

  /* Owning Library VRF structure */
  struct apn_vrf *igi_owning_ivrf;

  /* IGMP Svc Registrations List */
  struct list igi_svc_reg_lst;

  /* IGMP SSM-Map Static List */
  struct list igi_ssm_map_static_lst;

  /* IGMP Interfaces AVL Tree */
  struct avl_tree *igi_if_tree;

  /* IGMP Input buffer */
  struct stream *igi_i_buf;

  /* IGMP Output buffer */
  struct stream *igi_o_buf;

  /* IGMP Tunnel Interface Get */
  igmp_cback_tunnel_get_t igi_cback_tunnel_get;

  /* IGMP Instance wide limit */
  u_int32_t igi_limit;

  /* IGMP Instance wide limit exception ACL */
  u_int8_t *igi_limit_except_alist;

  /* IGMP Instance wide G-Recs Count */
  u_int32_t igi_num_grecs;

  /* IGMP MIB Variables  */
  struct variable *igi_mib_vars;

  /* IGMP well-known muticast address in Network-order */
  struct pal_in4_addr igi_allhosts;
  struct pal_in4_addr igi_allrouters;
  struct pal_in4_addr igi_igmp_v3routers;
  struct pal_in4_addr igi_in4any_addr;

  /* IGMP Instance Configuration Flags */
  u_int16_t igi_cflags;
#define IGMP_INST_CFLAG_LIMIT_GREC          (1 << 0)
#define IGMP_INST_CFLAG_SNOOP_DISABLED      (1 << 1)
#define IGMP_INST_CFLAG_SSM_MAP_DISABLED    (1 << 2)
#define IGMP_INST_CFLAG_SSM_MAP_STATIC      (1 << 3)

  /* IGMP Instance Status Flags */
  u_int16_t igi_sflags;
#define IGMP_INST_SFLAG_SNOOP_ENABLED       (1 << 0)
#define IGMP_INST_SFLAG_L3_ENABLED          (1 << 1)
  
  /* IGMP Instance Debug Flags */
  u_int32_t igi_conf_dbg_flags;
  u_int32_t igi_term_dbg_flags;
#define IGMP_INST_DBG_DECODE                (1 << 0)
#define IGMP_INST_DBG_ENCODE                (1 << 1)
#define IGMP_INST_DBG_EVENTS                (1 << 2)
#define IGMP_INST_DBG_FSM                   (1 << 3)
#define IGMP_INST_DBG_TIB                   (1 << 4)
};

/* IGMP Instance Intialization Parameteres */
struct igmp_init_params
{
  /* IGMP Instance Lib globals */
  struct lib_globals *igin_lg;

  /* Owning Library VRF structure */
  struct apn_vrf *igin_owning_ivrf;

  /* IGMP Input Buffer */
  struct stream *igin_i_buf;

  /* IGMP Output Buffer */
  struct stream *igin_o_buf;

  /* IGMP Tunnel Interface Get */
  igmp_cback_tunnel_get_t igin_cback_tunnel_get;
};

/* IGMP Service-Registration Structure */
struct igmp_svc_reg
{
  /* Owning IGMP instance */
  struct igmp_instance *igsr_owning_igi;

  /* IGMP Svc User's Reg-id */
  u_int32_t igsr_su_id;

  /* IGMP Svc Provider's Reg-id (Pointer to Self-struct) */
  u_int32_t igsr_sp_id;

  /* IGMP Registration Service Type */
  enum igmp_svc_type igsr_svc_type;

  /* IGMP Svc Registration's Socket */
  pal_sock_handle_t igsr_sock;

  /* IGMP Svc Registration's Read Thread */
  struct thread *t_igsr_read;

  /* IGMP Svc User's IF-ID Callback Function */
  igmp_cback_if_su_id_t igsr_cback_if_su_id;

  /* IGMP Svc User's VIF Control Callback Function */
  igmp_cback_vif_ctl_t igsr_cback_vif_ctl;

  /* IGMP Svc User's L-Mem Update Callback Function */
  igmp_cback_lmem_update_t igsr_cback_lmem_update;

  /* IGMP Svc User's MFC Programming Callback Function */
  igmp_cback_mfc_prog_t igsr_cback_mfc_prog;

  /* IGMP Svc User's MRouter IF Update Callback Function */
  igmp_cback_mrt_if_update_t igsr_cback_mrt_if_update;

  /* IGMP Svc Registration Status Flags */
  u_int16_t igsr_sflags;
#define IGMP_SVC_REG_SFLAG_SOCK_OWNER   (1 << 0)
};

/* IGMP Service-Registration Parameters */
struct igmp_svc_reg_params
{
  /* IGMP Svc User's Reg-id */
  u_int32_t igsrp_su_id;

  /* IGMP Svc Provider's Reg-id (Pointer to Self-struct) */
  u_int32_t igsrp_sp_id;

  /* IGMP Registration Service Type */
  enum igmp_svc_type igsrp_svc_type;

  /* IGMP Svc Registration's Socket */
  pal_sock_handle_t igsrp_sock;

  /* IGMP Svc User's IF-ID Callback Function */
  igmp_cback_if_su_id_t igsrp_cback_if_su_id;

  /* IGMP Svc User's VIF Control Callback Function */
  igmp_cback_vif_ctl_t igsrp_cback_vif_ctl;

  /* IGMP Svc User's L-Mem Update Callback Function */
  igmp_cback_lmem_update_t igsrp_cback_lmem_update;

  /* IGMP Svc User's MFC Programming Callback Function */
  igmp_cback_mfc_prog_t igsrp_cback_mfc_prog;

  /* IGMP Svc User's MRouter IF Update Callback Function */
  igmp_cback_mrt_if_update_t igsrp_cback_mrt_if_update;
};

/* IGMP Static SSM-Map */
struct igmp_ssm_map_static
{
  /* IGMP Static SSM-Map ACL Group Address */
  u_int8_t *igssms_grp_alist;

  /* IGMP Static SSM-Map Source Address */
  struct pal_in4_addr igssms_msrc;
};

/* IGMP Interface Configurable Variables */
struct igmp_if_conf
{
  /* IGMP Interface Access-List*/
  u_int8_t *igifc_access_list;

  /* IGMP Interface Immediate-Leave Group-list*/
  u_int8_t *igifc_immediate_leave;

  /*
   * IGMP Interface Mrouter Interface
   * (variable overloaded for both Proxy-Service & Snooping)
   */
  struct list igifc_mrouter_if_lst;

  /* IGMP Interface Last-Member Query Count */
  u_int16_t igifc_lmqc;

  /* IGMP Interface Last-Member Query Interval (sec) */
  u_int16_t igifc_lmqi;

  /* IGMP Interface Limit G-Records */
  u_int32_t igifc_limit;
  
  /* IGMP Interface Startup Query Interval */
  u_int16_t igifc_sqi;

  /* IGMP Interface Startup Query Count */
  u_int8_t igifc_sqc;

  /* IGMP Interface limit exception ACL */
  u_int8_t *igifc_limit_except_alist;

  /* IGMP Interface Other-Querier Interval */
  u_int16_t igifc_oqi;

  /* IGMP Interface Query Interval */
  u_int16_t igifc_qi;

  /* IGMP Interface Query Response Time */
  u_int16_t igifc_qri;

  /* IGMP Interface Version */
  u_int16_t igifc_version;

  /* IGMP Interface Robustness Variable */
  u_int8_t igifc_rv;
  u_int8_t pad [3];

  /* IGMP Interface Configured Values Flags */
  u_int32_t igifc_cflags;
#define IGMP_IF_CFLAG_CONFIG_ENABLED                (1 << 0)
#define IGMP_IF_CFLAG_ACCESS_LIST                   (1 << 1)
#define IGMP_IF_CFLAG_IMMEDIATE_LEAVE               (1 << 2)
#define IGMP_IF_CFLAG_LAST_MEMBER_QUERY_COUNT       (1 << 3)
#define IGMP_IF_CFLAG_LAST_MEMBER_QUERY_INTERVAL    (1 << 4)
#define IGMP_IF_CFLAG_LIMIT_GREC                    (1 << 5)
#define IGMP_IF_CFLAG_MROUTE_PROXY                  (1 << 6)
#define IGMP_IF_CFLAG_PROXY_SERVICE                 (1 << 7)
#define IGMP_IF_CFLAG_QUERIER_TIMEOUT               (1 << 8)
#define IGMP_IF_CFLAG_QUERY_INTERVAL                (1 << 9)
#define IGMP_IF_CFLAG_QUERY_RESPONSE_INTERVAL       (1 << 10)
#define IGMP_IF_CFLAG_ROBUSTNESS_VAR                (1 << 11)
#define IGMP_IF_CFLAG_SNOOP_ENABLED                 (1 << 12)
#define IGMP_IF_CFLAG_SNOOP_DISABLED                (1 << 13)
#define IGMP_IF_CFLAG_SNOOP_FAST_LEAVE              (1 << 14)
#define IGMP_IF_CFLAG_SNOOP_MROUTER_IF              (1 << 15)
#define IGMP_IF_CFLAG_SNOOP_QUERIER                 (1 << 16)
#define IGMP_IF_CFLAG_SNOOP_NO_REPORT_SUPPRESS      (1 << 17)
#define IGMP_IF_CFLAG_VERSION                       (1 << 18)
#define IGMP_IF_CFLAG_RA_OPT                        (1 << 19)
#define IGMP_IF_CFLAG_STARTUP_QUERY_COUNT           (1 << 20)
#define IGMP_IF_CFLAG_STARTUP_QUERY_INTERVAL        (1 << 21)
#define IGMP_OFFLINK_IF_CFLAG_CONFIG_ENABLED        (1 << 22)
};

/* IGMP Interface Indexing Structure */
struct igmp_if_idx
{
  /* Primary Index - Pointer to Lib Interface */
  struct interface *igifidx_ifp;

  /* Secondary Index - Svc User's IF Sec-ID */
  u_int32_t igifidx_sid;
};

/* IGMP Interface Strucutre */
struct igmp_if
{
  /* Parent Interface Information */
  struct igmp_if *igmp_parent_igif;

  /* IGMP Interface Configuration */
  struct igmp_if_conf igif_conf;

  /* Owning IGMP Instance */
  struct igmp_instance *igif_owning_igi;

  /* IGMP Interface Index */
  struct igmp_if_idx igif_idx;
  /* Owning Library Interface */
#define igif_owning_ifp igif_idx.igifidx_ifp
  /* IGMP Svc User's Interface Index */
#define igif_su_id      igif_idx.igifidx_sid

  /* IGMP Interface Address Prefix */
  struct prefix *igif_paddr;

  /* IGMP Group Membership Records Tree-Info-Base */
  struct ptree *igif_gmr_tib;

  /* IGMP Host-side Group Membership Records Tree-Info-Base */
  struct ptree *igif_hst_gmr_tib;

  /*
   * IGMP Host-side consolidator Interface
   * (variable overloaded as IGMP Mroute-proxy Interface in IGMP
   * Proxying and consolidator of Bridge/VLAN ports in IGMP Snooping)
   */
  struct igmp_if *igif_hst_igif;

  /* IGMP Downstream IFs List for Host-side IF */
  struct list igif_hst_dsif_lst;

  /* DS IFs which will be added once HST IF gets life */
  struct list igif_hst_chld_lst;

  /* IGMP Other Querier Address (valid only if non-Querier) */
  struct pal_in4_addr igif_other_querier_addr;

  /* IGMP Querier Timer Thread */
  struct thread *t_igif_querier;

  /* IGMP Other-Querier Timer Thread */
  struct thread *t_igif_other_querier;

  /* IGMP Interface Warnings Rate-Limit Timer */
  struct thread *t_igif_warn_rlimit;

  /* IGMP Host-side Ver1 Querier Present Timer Thread */
  struct thread *t_igif_hst_v1_querier_present;

  /* IGMP Host-side Ver2 Querier Present Timer Thread */
  struct thread *t_igif_hst_v2_querier_present;

  /* IGMP Proxy/Snoop Service General-Report Timer Thread */
  struct thread *t_igif_hst_send_general_report;

  /* IGMP Join Group General-Report Timer Thread */
  struct thread *t_igif_jg_send_general_report;

  /* IGMP Interface Uptime */
  pal_time_t igif_uptime;

  /* IGMP Interface Startup Query Count */
  u_int16_t igif_sqc;

  /*
   * IGMP Interface Group-Membership-Interval
   * (variable overloaded as Older Version Querier Present
   *  Timeout for Proxy/Snoop Service)
   */
  u_int16_t igif_gmi;

  /* IGMP Interface Query Interval */
  u_int16_t igif_qi;

  /* IGMP Interface Other Querier Interval */
  u_int16_t igif_oqi;

  /* IGMP Interface G-Recs Count */
  u_int32_t igif_num_grecs;

  /* IGMP Interface Grp-Joins Count */
  u_int32_t igif_num_joins;

  /* IGMP Interface Grp-Leaves Count */
  u_int32_t igif_num_leaves;

  /* IGMP Interface V1-Querier Warnings Count */
  u_int16_t igif_v1_querier_wcount;

  /* IGMP Interface V2-Querier Warnings Count */
  u_int16_t igif_v2_querier_wcount;

  /* IGMP Interface V3-Querier Warnings Count */
  u_int16_t igif_v3_querier_wcount;

  /* IGMP Interface Robustness Variable */
  u_int8_t igif_rv;
  u_int8_t pad [1];

#ifdef HAVE_SNMP
  /* SNMP Generic Return Variable */
  u_int32_t igif_snmp_ret_var;
#endif /* HAVE_SNMP */
  /* IGMP Interface Status Flags */
  /*
   * NOTE: Either OR both of '_SVC_TYPE_L2' and '_SVC_TYPE_L3'
   * Flags SHOULD be Set. Both being Unset is Invalid.
   */
  u_int32_t igif_sflags;
#define IGMP_IF_SFLAG_ACTIVE                    (1 << 0)
#define IGMP_IF_SFLAG_HOST_COMPAT_V1            (1 << 1)
#define IGMP_IF_SFLAG_HOST_COMPAT_V2            (1 << 2)
#define IGMP_IF_SFLAG_HOST_COMPAT_V3            (1 << 3)
#define IGMP_IF_SFLAG_IF_CONF_INHERITED         (1 << 4)
#define IGMP_IF_SFLAG_L2_MCAST_ENABLED          (1 << 5)
#define IGMP_IF_SFLAG_MCAST_ENABLED             (1 << 6)
#define IGMP_IF_SFLAG_MFC_MSG_IN_IF             (1 << 7)
#define IGMP_IF_SFLAG_QUERIER                   (1 << 8)
#define IGMP_IF_SFLAG_SNOOPING                  (1 << 9)
#define IGMP_IF_SFLAG_SNOOP_MROUTER_IF          (1 << 10)
#define IGMP_IF_SFLAG_SNOOP_MROUTER_IF_CONFIG   (1 << 11)
#define IGMP_IF_SFLAG_SVC_TYPE_L2               (1 << 12)
#define IGMP_IF_SFLAG_SVC_TYPE_L3               (1 << 13)
};

/* IGMP_SNMP Structures */
#ifdef HAVE_SNMP
struct igmp_snmp_rtr_if_index
{
  u_int32_t len;
  u_int32_t ifindex;
  u_int8_t qtype;
  u_int8_t pad [3];
};

struct igmp_snmp_rtr_cache_index
{
  u_int32_t len;
  u_int8_t qtype;
  u_int8_t pad [3];
  struct pal_in4_addr cache_addr;
  u_int32_t ifindex;
};

struct igmp_snmp_inv_rtr_index
{
  u_int32_t len;
  u_int32_t ifindex;
  u_int8_t qtype;
  u_int8_t pad [3];
  struct pal_in4_addr cache_addr;
};

struct igmp_snmp_rtr_src_list_index
{
  u_int32_t len;
  u_int8_t qtype;
  u_int8_t pad [3];
  struct pal_in4_addr group_addr;
  u_int32_t ifindex;
  struct pal_in4_addr host_addr;
};
#endif /* HAVE_SNMP */

/* IGMP Interface Group Record */
struct igmp_group_rec
{
  /* IGMP Group Record Owning P-Trie Node */
  struct ptree_node *igr_owning_pn;

  /* IGMP Group Record Owning IGMP IF */
  struct igmp_if *igr_owning_igif;

  /* IGMP Group Record Last Reporting Host */
  struct pal_in4_addr igr_last_reporter;

  /* IGMP Group Record Uptime  */
  pal_time_t igr_uptime;

  /* IGMP Group Record Liveness Timer Value */
  u_int32_t v_igr_liveness;

  /* IGMP Group Record Liveness Timer Thread */
  struct thread *t_igr_liveness;

  /*
   * IGMP Group Record Filter-Mode State
   * (variable overloaded for both Router & Host FSMs)
   */
  enum igmp_filter_mode_state igr_filt_mode_state;

  /* IGMP Group Record Source-List-A P-Trie */
  struct ptree *igr_src_a_tib;

  /* IGMP Group Record Source-List-B P-Trie */
  struct ptree *igr_src_b_tib;

  /* IGMP Group Record Source-List-A Count */
  u_int16_t igr_src_a_tib_count;

  /* IGMP Group Record Source-List-B Count */
  u_int16_t igr_src_b_tib_count;

  /* IGMP Group Ver1 Host Present Timer Thread */
  struct thread *t_igr_v1_host_present;

  /* IGMP Group Ver2 Host Present Timer Thread */
  struct thread *t_igr_v2_host_present;

  /* IGMP Group Query Re-transmit  Count */
  u_int16_t igr_rexmit_group_lmqc;

  /* IGMP Group-Source Query Re-transmit Count */
  u_int16_t igr_rexmit_group_source_lmqc;

  /* IGMP Grp/Grp-Src Report Re-transmit Host Rec Type */
  enum igmp_hst_rec_type igr_rexmit_hrt;

  /*
   * IGMP Group Query Re-transmission Timer Thread
   * (variable overloaded for sending Grp-Report (Host-side))
   */
  struct thread *t_igr_rexmit_group;

  /*
   * IGMP Group-Source Query Re-transmission Timer Thread
   * (variable overloaded for sending Grp-Src-Report (Host-side))
   */
  struct thread *t_igr_rexmit_group_source;

  /*
   *IGMP Group Query Re-transmission Timer Thread
   *(variable overloaded for sending Grp-Report (Join-group))
   */
  struct thread *t_igr_join_group;

  /*
   * IGMP Group-Source Query Re-transmission TIB
   * (variable overloaded for Rexmit Grp-Src-Report (Host-side))
   */
  struct ptree *igr_rexmit_srcs_tib;

  /* IGMP Group Source ALLOW-NEW_SRCS Rexmit TIB */
  struct ptree *igr_rexmit_allow_tib;

  /* IGMP Group Source BLOCK-OLD_SRCS Rexmit TIB */
  struct ptree *igr_rexmit_block_tib;

  /*
   * IGMP Group-Source Query Re-transmission TIB Count
   * (variable overloaded for Rexmit Grp-Src-Report (Host-side))
   */
  u_int16_t igr_rexmit_srcs_tib_count;

  /* IGMP Group Source ALLOW-NEW_SRCS TIB Count */
  u_int16_t igr_rexmit_srcs_allow_tib_count;

  /* IGMP Group Source BLOCK-OLD_SRCS TIB Count */
  u_int16_t igr_rexmit_srcs_block_tib_count;

  /* IGMP Group Record Status-Flags */
  u_int16_t igr_sflags;
#define IGMP_IGR_SFLAG_COMPAT_V1                (1 << 0)
#define IGMP_IGR_SFLAG_COMPAT_V2                (1 << 1)
#define IGMP_IGR_SFLAG_COMPAT_V3                (1 << 2)
#define IGMP_IGR_SFLAG_REPORT_PENDING           (1 << 3)
#define IGMP_IGR_SFLAG_MFC_PROGMED              (1 << 4)
#define IGMP_IGR_SFLAG_STATE_REFRESH            (1 << 5)
#define IGMP_IGR_SFLAG_STATIC                   (1 << 6)
#define IGMP_IGR_SFLAG_DYNAMIC                  (1 << 7)
#define IGMP_IGR_SFLAG_JOIN                     (1 << 8) 
  u_int32_t igr_cflags;
#define IGMP_IGR_CFLAG_STATIC_GROUP             (1 << 0)
#define IGMP_IGR_CFLAG_STATIC_GROUP_SOURCE      (1 << 1)
#define IGMP_IGR_CFLAG_STATIC_SOURCE_SSM_MAP    (1 << 2)
#define IGMP_IGR_CFLAG_STATIC_GROUP_IF_NAME     (1 << 3)
};

/* IGMP Interface Source Record */
struct igmp_source_rec
{
  /* IGMP Source Record Owning P-Trie Node */
  struct ptree_node *isr_owning_pn;

  /* IGMP Source Record Owning Group Record */
  struct igmp_group_rec *isr_owning_igr;

  /* IGMP Source Record Uptime  */
  pal_time_t isr_uptime;

  /* IGMP Source Record Liveness Timer Thread */
  struct thread *t_isr_liveness;

  /* IGMP Source Record Liveness Timer Value */
  u_int16_t v_isr_liveness;

  /* IGMP Source Record Status-Flags */
  u_int16_t isr_sflags;
#define IGMP_ISR_SFLAG_REPORT_PENDING           (1 << 0)
#define IGMP_ISR_SFLAG_STATIC                   (1 << 1)
#define IGMP_ISR_SFLAG_DYNAMIC                  (1 << 2)
};

/* IGMP Sources List */
struct igmp_source_list
{
  /* IGMP Number of Sources */
  u_int16_t isl_num;
  /* IGMP Source Record   */
  u_int16_t isl_static;
  /* IGMP Source Addresses List */
  struct pal_in4_addr isl_saddr [1];
};

/* IGMP IPv4 Pkt Header + RA Opt Structure */
struct igmp_in4_header
{
  struct pal_in4_header iph;
  u_int8_t ra_opt [IGMP_RA_SIZE];
};

/* IGMP IPv4 Socket Control Message Union */
union igmp_in4_cmsg
{
  struct pal_sockaddr sa;
  struct pal_cdata_msg cdm;
  struct pal_sockaddr_in4 sin4;
  struct sockaddr_vlan vaddr;
};

/*
 * IGMP Router-side Group Record Finite State Machine
 * Action Function Type
 */
typedef s_int32_t (* igmp_igr_fsm_act_func_t)
                  (struct igmp_group_rec *,
                   enum igmp_filter_mode_event,
                   struct igmp_source_list *);

/*
 * IGMP Host-side Group Record Finite State Machine
 * Action Function Type
 */
typedef s_int32_t (* igmp_hst_igr_fsm_act_func_t)
                  (struct igmp_group_rec *,
                   enum igmp_hst_filter_mode_event,
                   struct igmp_source_list *);

#endif /* _PACOS_IGMP_STRUCT_H */
