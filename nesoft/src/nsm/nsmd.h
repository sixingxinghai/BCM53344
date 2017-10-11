/* Copyright (C)  2002-2003  All Rights Reserved. */

#ifndef _PACOS_NSMD_H
#define _PACOS_NSMD_H

#ifdef HAVE_TE
#include "admin_grp.h"
#endif /* HAVE_TE */

#if defined(HAVE_VLAN) && defined (HAVE_CUSTOM1)
#include "L2/nsm_vlan.h"
#endif /* HAVE_VLAN */

#include "nsm_vrf.h"

#define NSM_FALSE   0
#define NSM_TRUE    1

#ifdef HAVE_RESTART
#define NSM_MARK_STALE_ENTRIES         1 
#define NSM_UNMARK_STALE_ENTRIES       2
#endif /* HAVE_RESTART */

/* NSM interface parameter.  */
#define HAVE_NSM_IF_PARAMS

/* Number of static routes. */
#define MAX_STATIC_ROUTE_DEFAULT        4294967294UL

/* Number of FIB routes */
#define MAX_FIB_ROUTE_DEFAULT    4294967294UL

/* Routing information preserve timer.
   Each protocol can set different restart time.  */
struct nsm_restart
{
  /* Restarting protocol ID. */
  int proto_id;

  /* State of restart */
  u_char state;
#define NSM_RESTART_INACTIVE    0
#define NSM_RESTART_ACTIVE      1

  /* Registered restart time. */
  u_int32_t restart_time;

  /* Registered restart time. */
  u_int32_t preserve_time;

  /* Preserve thread. */
  struct thread *t_preserve;

  /* Disconnect time. */
  pal_time_t disconnect_time;

  /* Option value. */
  u_int16_t restart_length;
  u_char *restart_val;

#ifdef HAVE_HA
  /* NSM GR CDR reference */
  HA_CDR_REF gr_restart_cdr_ref;

  /* NSM GR Val CDR reference */
  HA_CDR_REF gr_restart_val_cdr_ref;
#endif /* HAVE_HA */
};

/* Global data outside of VR context. */
struct nsm_globals
{
  /* NSM Server. */
  struct nsm_server *server;

  /* Flags. */
  u_int32_t flags;
#define NSM_VRF_NO_FIB                  (1 << 0)
#define NSM_SHUTDOWN                    (1 << 1)

#ifdef HAVE_NSM_IF_ARBITER
  /* If-Arbiter interval.  */
  int if_arbiter_interval;

  /* If-Arbiter thread.  */
  struct thread *t_if_arbiter;
#endif /* HAVE_NSM_IF_ARBITER */

  /* If stats update threshold timer */
  struct thread *t_if_stat_threshold;
#define NSM_IF_STAT_UPDATE_THRESHOLD        5  /* seconds */

  struct nsm_restart restart[APN_PROTO_MAX];

  /* Default rtm_table for all clients */
  s_int32_t rtm_table_default;

#ifdef HAVE_RMM
  struct nsm_rmm_top *rmm;
#endif /* HAVE_RMM */

#ifdef HAVE_LICENSE_MGR
  struct nsm_client_lic
  {
    struct lib_globals *zg;
    lic_mgr_handle_t handle;
    struct thread *t_check_expiration;
  };

  struct nsm_client_lic vpn_lic;
  struct nsm_client_lic ldp_lic;
  struct nsm_client_lic ipv6_lic;
  struct nsm_client_lic mpls_lic;
  struct nsm_client_lic pim_lic;
#endif /* HAVE_LICENSE_MGR */

#ifdef HAVE_LACPD
  /* Total aggregator count of LACP aggregators and Static aggregators*/
  u_int32_t agg_cnt;
#endif /* HAVE_LACPD */

  struct stream *iobuf;

  struct stream *obuf;

  /* HA Record Reference */
#ifdef HAVE_HA
  /* NSM Globals CDR reference */
  HA_CDR_REF nsm_globals_cdr_ref;

#ifdef HAVE_NSM_IF_ARBITER
  /* NSM IF Arbiter timer CDR reference */
  HA_CDR_REF nsm_if_arbiter_cdr_ref;
#endif /* HAVE_NSM_IF_ARBITER */

#endif /* HAVE_HA */
#ifdef HAVE_SMI
  /* API Server. */
  struct smi_server *nsm_smiserver;
#endif /* HAVE_SMI */
};

#ifdef HAVE_QOS
struct nsm_qos_acl_master;
struct nsm_qos_cmap_master;
struct nsm_qos_pmap_master;
#endif /* HAVE_QOS */

struct nsm_debug_flags
{
#ifdef HAVE_BFD
  u_int32_t ndf_bfd;
#endif /* HAVE_BFD */
  u_int32_t ndf_event;
  u_int32_t ndf_packet;
  u_int32_t ndf_kernel;
  u_int32_t ndf_ha;
};

/* NSM Master for system wide configurations and variables */
struct nsm_master
{
  /* Pointer to VR. */
  struct apn_vr *vr;

  /* NSM pointer to lib_globals */
  struct lib_globals *zg;
#define NSM_ZG   (nzg)

  /* Description. */
  char *desc;

  /* Control VR support per PM. */
  modbmap_t module_bits;

  /* NSM master start time.  */
  pal_time_t start_time;

  u_char flags;
#define NSM_MULTIPATH_REFRESH           (1 << 0)
#define NSM_FIB_RETAIN_RESTART          (1 << 1)
#define NSM_IPV4_FORWARDING             (1 << 2)
#define NSM_IPV6_FORWARDING             (1 << 3)

  /* Maximum path config. */
  u_char multipath_num;

  /* Maximum static routes */
  u_int32_t max_static_routes;

  /* Maximum FIB routes excluding Kernel, Connect and Static*/
  u_int32_t max_fib_routes;

  /* The time of retaining stale FIB routes when NSM start. */
  u_int16_t fib_retain_time;
#define NSM_FIB_RETAIN_TIME_MIN         1
#define NSM_FIB_RETAIN_TIME_MAX         65535
#define NSM_FIB_RETAIN_TIME_DEFAULT     60
#define NSM_FIB_RETAIN_TIME_FOREVER     0

  /* Threads. */
  struct thread *t_sweep;       /* Sweep stale FIB routes. */
#ifdef HAVE_KERNEL_ROUTE_SYNC
  struct thread *t_rib_kernel_sync;     /* RIB kernel sync. */
#endif /* HAVE_KERNEL_ROUTE_SYNC */

  /* The following is the table of label pool managers that are
     handled by PacOS */
#if defined HAVE_GMPLS
  struct route_table *label_pool_table[GMPLS_LABEL_TYPE_MAX];
#elif defined HAVE_MPLS
  struct route_table *label_pool_table[1];
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS
  /* NSM MPLS top structure. */
  struct nsm_mpls *nmpls;

#define NSM_MPLS   (nm->nmpls)
#endif /* HAVE_MPLS */


#ifdef HAVE_TE
  /* QOS resource id counter */
  u_int32_t resource_counter;

  struct admin_group admin_group_array [ADMIN_GROUP_MAX];
#endif /* HAVE_TE */

#ifdef HAVE_RTADV
  struct rtadv *rtadv;
#endif /* HAVE_RTADV */

#ifdef HAVE_VRRP
  struct vrrp_global *vrrp;
#endif /* HAVE_VRRP */

#ifdef HAVE_STAGGER_KERNEL_MSGS
  /* Hold timer to stagger writes to the kernel. */
  struct thread *t_kernel_msg_stagger;

  /* List for storing messages that need to be sent to the kernel. */
  struct list *kernel_msg_stagger_list;
#endif /* HAVE_STAGGER_KERNEL_MSGS */

#ifdef HAVE_L2
  struct nsm_bridge_master *bridge;
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD
  struct nsm_l2_oam_master *l2_oam_master;
#endif

#ifdef HAVE_CUSTOM1
#ifdef HAVE_VLAN
  /* Layer 2 related information.  */
  struct nsm_layer2_master l2;
#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_VLAN
  struct nsm_vlan_access_master *vmap;
#endif /* HAVE_VLAN */

#ifdef HAVE_NSM_IF_PARAMS
  /* Interface parameter list.  */
  struct list *if_params;
#endif /* HAVE_NSM_IF_PARAMS */

#ifdef HAVE_LACPD
  struct bitmap *lacp_admin_key_mgr;
  struct nsm_lacp_admin_key_element *ake_list;

  /* Port Selection Criteria */
  int psc;
#endif /* HAVE_LACPD */

  struct list *phyEntityList;

  /* Disconnect time. */
  pal_time_t last_change_time;

#ifdef HAVE_QOS
  /* QoS parameter list */
  /* MAC and IP acl list */
  struct nsm_qos_acl_master *acl;

  /* CMAP list */
  struct nsm_qos_cmap_master *cmap;

  /* PMAP list */
  struct nsm_qos_pmap_master *pmap;
#endif /* HAVE_QOS */

#ifdef HAVE_L2LERN
  struct nsm_mac_acl_master *mac_acl;
#endif /* HAVE_L2LERN */

#ifdef HAVE_L3
  struct nsm_arp_master *arp;
#endif /* HAVE_L3 */

#ifdef HAVE_IPSEC
  /* IPSEC NSM Master */
  struct nsm_ipsec_master *ipsec_master;
#endif /* HAVE_IPSEC */

#ifdef HAVE_FIREWALL
  struct nsm_firewall_master *firewall_master;
#endif /* HAVE_FIREWALL */

#ifdef HAVE_HA
  HA_CDR_REF  nsm_master_cdr_ref;

  HA_CDR_REF  nsm_master_rib_sweep_tmr_cdr_ref;
#endif /* HAVE_HA */

  /* NSM configured and terminal debug flags.  */
  struct
  {
    struct nsm_debug_flags nmd_conf;
    struct nsm_debug_flags nmd_term;
  } nm_debug;

#ifdef HAVE_L3
#ifdef HAVE_PBR
  /* list of route map interface structure which keeps track of route maps
     configured on interfaces */
  struct list * route_map_if;
#endif /* HAVE_PBR */
#endif /* HAVE_L3 */
};

#ifdef HAVE_STAGGER_KERNEL_MSGS
#define KERNEL_MSG_WRITE_MAX                   100
#define KERNEL_MSG_WRITE_INTERVAL_MAX          3      /* sec */
#define KERNEL_MSG_STAGGER_TIMER_START_S       0      /* sec */
#define KERNEL_MSG_STAGGER_TIMER_START_US      100000 /* usec */
#define KERNEL_MSG_STAGGER_TIMER_RESTART_S     0      /* sec */
#define KERNEL_MSG_STAGGER_TIMER_RESTART_US    500000 /* usec */

struct nsm_kernel_msg_stagger_node
{
  struct rib *rib;
  struct nsm_tree_node *rn;
};
#endif /* HAVE_STAGGER_KERNEL_MSGS */

/* Default multipath number supported. */
#ifdef HAVE_MULTIPATH
#define DEFAULT_MULTIPATH_NUM           16
#else
#define DEFAULT_MULTIPATH_NUM           1
#endif /* HAVE_MULTIPATH */

/* Count prefix size from mask length */
#define PSIZE(a) (((a) + 7) / (8))

/* PAL, HAL path definitions. */
#undef ENABLE_PAL_PATH
#undef ENABLE_PAL_INIT
#undef ENABLE_PAL_DEINIT
#undef ENABLE_HAL_INIT
#undef ENABLE_HAL_DEINIT
#ifdef HAVE_HAL
#undef ENABLE_PAL_PATH
#define ENABLE_HAL_INIT
#define ENABLE_HAL_DEINIT
#if defined (HAVE_SWFWDR) || defined (HAVE_INTEL)
#define ENABLE_PAL_INIT
#define ENABLE_PAL_DEINIT
#define ENABLE_PAL_PATH
#endif /* HAVE_SWFWDR || HAVE_INTEL */
#if !defined (HAVE_SWFWDR)
#define ENABLE_HAL_PATH
#endif /* ! HAVE_SWFWDR */
#else
#define ENABLE_PAL_INIT
#define ENABLE_PAL_DEINIT
#define ENABLE_PAL_PATH
#endif /* HAVE_HAL */

#ifdef HAVE_INTEL
#define ENABLE_HAL_PATH
#define HAVE_HAL
#endif /* HAVE_INTEL */

/* Error definitions. */
#define NSM_SUCCESS                     0
#define NSM_FAILURE                    -1
#define NSM_ERR_VPN_EXISTS_ON_IF       -2
#define NSM_ERR_VPN_EXISTS_ON_IF_VLAN  -3
#define NSM_ERR_VPN_BIND_ON_IF_VLAN    -4
#define NSM_ERR_VC_ID_NOT_FOUND        -5
#define NSM_ERR_VC_NAME_NOT_FOUND      -6
#define NSM_ERR_MEM_ALLOC_FAILURE      -7
#define NSM_ERR_NAME_IN_USE            -8
#define NSM_ERR_VAL_IN_USE             -9
#define NSM_ERR_VC_NAME_ID_MISMATCH    -10
#define NSM_ERR_NAME_TOO_LONG          -11
#define NSM_ERR_MIF_VC_MISMATCH        -12
#define NSM_ERR_VC_NOT_CONFIGURED      -13
#define NSM_ERR_VC_IN_USE              -14
#define NSM_ERR_NOT_FOUND              -15
#define NSM_ERR_VC_TYPE_UNKNOWN        -16
#define NSM_ERR_VC_TYPE_MISMATCH       -17
#define NSM_ERR_LS_IN_USE              -18
#define NSM_ERR_INVALID_ARGS           -19
#define NSM_ERR_NO_TABLE_INDEX         -20
#define NSM_ERR_NO_FTN_ENTRY           -21
#define NSM_ERR_NO_NHLFE_ENTRY         -22
#define NSM_ERR_INVALID_NHLFE          -23
#define NSM_ERR_INTERNAL               -24
#define NSM_ERR_INVALID_ADDR_FAMILY    -25
#define NSM_ERR_NO_ILM_ENTRY           -26
#define NSM_ERR_INVALID_INTERFACE      -27
#define NSM_ERR_INVALID_FTN_INDEX      -28
#define NSM_ERR_INVALID_LABEL          -29
#define NSM_ERR_INVALID_NEXTHOP        -30
#define NSM_ERR_INVALID_OPCODE         -31
#define NSM_ERR_OWNER_MISMATCH         -32
#define NSM_ERR_ILM_INSTALL_FAILURE    -33
#define NSM_ERR_FTN_INSTALL_FAILURE    -34
#define NSM_ERR_INVALID_VRF_ID         -35
#define NSM_ERR_INVALID_FTN_TYPE       -36
#define NSM_ERR_LS_ON_LOOPBACK         -37
#define NSM_ERR_DSCP_INVALID           -38
#define NSM_ERR_DSCP_NOT_SUPPORTED     -39
#define NSM_ERR_DSCP_EXP_MISMATCH      -40
#define NSM_ERR_VRF_ENTRY_EXISTS       -41
#define NSM_ERR_IF_BINDING_EXISTS      -42
#define NSM_ERR_IF_NOT_BOUND           -43
#define NSM_ERR_VC_BINDING_EXISTS      -44
#define NSM_ERR_INVALID_VC_ID          -45
#define NSM_ERR_INVALID_BW_CONSTRAINT  -46
#define NSM_ERR_INVALID_XC             -47
#define NSM_ERR_EXISTS                 -48
#define NSM_ERR_IF_NOT_FOUND           -49
#define NSM_ERR_IF_BIND_VLAN_ERR       -50
#define NSM_ERR_NO_CW_CAPABILITY       -51
#define NSM_ERR_LS_INVALID_MIN_LABEL   -52
#define NSM_ERR_LS_INVALID_MAX_LABEL   -53
#define NSM_ERR_VC_MODE_MISMATCH       -54
#define NSM_ERR_VC_MODE_CONFLICT       -55
#define NSM_ERR_VC_MODE_MISMATCH_REV   -56
#define NSM_ERR_ILM_DUP_ENTRY          -57
#define NSM_ERR_LABEL_SPACE_CONFIGURED        -58
#define NSM_ERR_MODULE_INVALID                -59
#define NSM_ERR_MODULE_LBL_RANGE_EXISTS       -60
#define NSM_ERR_MODULE_LBL_OUT_OF_RANGE       -61
#define NSM_ERR_MODULE_LBL_OVERLAPPING_RANGE  -62
#define NSM_ERR_INVALID_DUMMY_FLAG            -63
#define NSM_ERR_VR_DOES_NOT_EXIST             -64
#define NSM_ERR_IF_NOT_CONTROL_TYPE           -65
#define NSM_ERR_MODULE_LBL_INVALID_RANGE      -66
#define NSM_ERR_MODULE_LBL_RANGE_COLLISION    -67
#define NSM_ERR_GROUP_ID_EXISTS               -68
#define NSM_ERR_GROUP_EXISTS                  -69
#define NSM_ERR_IF_GMPLS_ENABLED              -70
#define NSM_ERR_IF_MPLS_ENABLED               -71


#ifdef HAVE_VPLS
#define NSM_ERR_VPLS_BASE                            -700
#define NSM_ERR_VPLS_NOT_FOUND                       (NSM_ERR_VPLS_BASE) - 1
#define NSM_ERR_VPLS_PEER_NOT_FOUND                  (NSM_ERR_VPLS_BASE) - 2
#define NSM_ERR_VPLS_PEER_EXISTS                     (NSM_ERR_VPLS_BASE) - 3
#define NSM_ERR_INVALID_VPLS                         (NSM_ERR_VPLS_BASE) - 4
#define NSM_ERR_SPOKE_VC_NOT_FOUND                   (NSM_ERR_VPLS_BASE) - 5
#define NSM_ERR_INVALID_VPLS_ID                      (NSM_ERR_VPLS_BASE) - 6
#define NSM_ERR_INVALID_VPLS_NAME                    (NSM_ERR_VPLS_BASE) - 7
#define NSM_ERR_VPLS_PEER_BOUND_TO_PRIM              (NSM_ERR_VPLS_BASE) - 8
#define NSM_ERR_VPLS_SEC_PEER_FAILURE                (NSM_ERR_VPLS_BASE) - 9
#define NSM_ERR_VPLS_PEER_PW_MANUAL_NOT_CONFIGURED   (NSM_ERR_VPLS_BASE) - 10
#define NSM_ERR_VPLS_INVALID_INCOMING_LABEL          (NSM_ERR_VPLS_BASE) - 11
#define NSM_ERR_VPLS_INVALID_OUTGOING_LABEL          (NSM_ERR_VPLS_BASE) - 12
#define NSM_ERR_VPLS_INVALID_INCOMING_INTERFACE      (NSM_ERR_VPLS_BASE) - 13
#define NSM_ERR_VPLS_INVALID_OUTGOING_INTERFACE      (NSM_ERR_VPLS_BASE) - 14
#define NSM_ERR_VPLS_INCOMING_LABEL_MISMATCH         (NSM_ERR_VPLS_BASE) - 15
#define NSM_ERR_VPLS_OUTGOING_LABEL_MISMATCH         (NSM_ERR_VPLS_BASE) - 16
#define NSM_ERR_VPLS_INCOMING_INTERFACE_MISMATCH     (NSM_ERR_VPLS_BASE) - 17
#define NSM_ERR_VPLS_OUTGOING_INTERFACE_MISMATCH     (NSM_ERR_VPLS_BASE) - 18
#define NSM_ERR_VPLS_FIB_ADD_FAILED                  (NSM_ERR_VPLS_BASE) - 19
#define NSM_ERR_VPLS_FIB_ENTRY_NOT_FOUND             (NSM_ERR_VPLS_BASE) - 20
#define NSM_ERR_VPLS_TUNNEL_ID_MISMATCH              (NSM_ERR_VPLS_BASE) - 21
#define NSM_ERR_VPLS_TUNNEL_NAME_MISMATCH            (NSM_ERR_VPLS_BASE) - 22
#define NSM_ERR_VPLS_TUNNEL_DIR_MISMATCH            (NSM_ERR_VPLS_BASE) -  23
#define NSM_ERR_VPLS_VC_TYPE_MISMATCH                (NSM_ERR_VPLS_BASE) - 24
#define NSM_ERR_VPLS_SPOKE_CONF_PW_TYPE_MISMATCH     (NSM_ERR_VPLS_BASE) - 25 
#define NSM_ERR_VPLS_SPOKE_CONF_MAP_TYPE_MISMATCH    (NSM_ERR_VPLS_BASE) - 26
#define NSM_ERR_VPLS_SPOKE_VC_IN_USE                 (NSM_ERR_VPLS_BASE) - 27 
#define NSM_ERR_SPOKE_VC_VIRTUAL_CIRCUIT_NOT_FOUND   (NSM_ERR_VPLS_BASE) - 28
#define NSM_ERR_VPLS_SPOKE_VC_MANUAL_NOT_CONFIGURED  (NSM_ERR_VPLS_BASE) - 29
#define NSM_ERR_VPLS_MESH_OWNER_MISMATCH             (NSM_ERR_VPLS_BASE) - 30
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS_VC
#define NSM_ERR_VC_BASE                              -800
#define NSM_ERR_INVALID_VC_NAME                      (NSM_ERR_VC_BASE) - 1
#define NSM_ERR_PRIMARY_VC_INACTIVE                  (NSM_ERR_VC_BASE) - 2
#define NSM_ERR_SECONDARY_VC_INACTIVE                (NSM_ERR_VC_BASE) - 3
#define NSM_VC_ERR_NO_SIBLING                        (NSM_ERR_VC_BASE) - 4
#define NSM_VC_ERR_SIBLING_MISMATCH                  (NSM_ERR_VC_BASE) - 5
#define NSM_ERR_VC_SWITCHOVER_MODE_CONFLICT          (NSM_ERR_VC_BASE) - 6
#define NSM_ERR_SEC_VC_STATUS_DOWN                   (NSM_ERR_VC_BASE) - 7
#define NSM_ERR_SEC_VC_ALREADY_INSTALLED             (NSM_ERR_VC_BASE) - 8
#define NSM_ERR_VC_MODE_NO_SECONDARY                 (NSM_ERR_VC_BASE) - 9
#define NSM_ERR_SAME_VC_NAME                         (NSM_ERR_VC_BASE) - 10
#define NSM_ERR_MS_PW_ALREADY_STITCHED               (NSM_ERR_VC_BASE) - 11
#define NSM_ERR_MS_PW_EXISTS                         (NSM_ERR_VC_BASE) - 12
#define NSM_ERR_MS_PW_NOT_FOUND                      (NSM_ERR_VC_BASE) - 13
#define NSM_ERR_MS_PW_SPE_DESCR_NOT_SET              (NSM_ERR_VC_BASE) - 14
#define NSM_ERR_VC_ALREADY_BOUND                     (NSM_ERR_VC_BASE) - 15
#define NSM_ERR_MS_PW_ROLE_MISMATCH                  (NSM_ERR_VC_BASE) - 16
#endif /* HAVE_MPLS_VC */

#if defined (HAVE_MPLS_OAM) && defined (HAVE_BFD)
#define NSM_ERR_MPLS_BFD_BASE                       -900
#define NSM_MPLS_BFD_ERR_INVALID_INTVL              (NSM_ERR_MPLS_BFD_BASE) - 1
#define NSM_MPLS_BFD_ERR_LSP_UNKNOWN                (NSM_ERR_MPLS_BFD_BASE) - 2
#define NSM_MPLS_BFD_ERR_ENTRY_EXISTS               (NSM_ERR_MPLS_BFD_BASE) - 3
#define NSM_MPLS_BFD_ERR_ENTRY_NOT_FOUND            (NSM_ERR_MPLS_BFD_BASE) - 4
#define NSM_MPLS_BFD_ERR_INVALID_PREFIX             (NSM_ERR_MPLS_BFD_BASE) - 5
#ifdef HAVE_VCCV
#define NSM_MPLS_BFD_VCCV_ERR_SESS_EXISTS           (NSM_ERR_MPLS_BFD_BASE) - 6
#define NSM_MPLS_BFD_VCCV_ERR_SESS_NOT_EXISTS       (NSM_ERR_MPLS_BFD_BASE) - 7
#define NSM_MPLS_BFD_VCCV_ERR_NOT_CONFIGURED        (NSM_ERR_MPLS_BFD_BASE) - 8
#endif /* HAVE_VCCV */

#endif /* HAVE_MPLS_OAM && HAVE_BFD */

#ifdef HAVE_MCAST_IPV4
#define NSM_ERR_MCAST_BASE                     -400
#define NSM_ERR_MCAST_FIB_SOCK_FAILURE         (NSM_ERR_MCAST_BASE) - 1
#define NSM_ERR_MCAST_PIM_SOCK_FAILURE         (NSM_ERR_MCAST_BASE) - 2
#define NSM_ERR_MCAST_MFWD_FAILURE             (NSM_ERR_MCAST_BASE) - 3
#define NSM_ERR_MCAST_NO_INTERFACE             (NSM_ERR_MCAST_BASE) - 4
#define NSM_ERR_MCAST_VIF_EXISTS               (NSM_ERR_MCAST_BASE) - 5
#define NSM_ERR_MCAST_NO_VIF_INDEX             (NSM_ERR_MCAST_BASE) - 6
#define NSM_ERR_MCAST_VIF_FWD_ADD_FAILURE      (NSM_ERR_MCAST_BASE) - 7
#define NSM_ERR_MCAST_NO_VIF                   (NSM_ERR_MCAST_BASE) - 8
#define NSM_ERR_MCAST_VIF_NOT_BELONG           (NSM_ERR_MCAST_BASE) - 9
#define NSM_ERR_MCAST_VIF_FWD_DEL_FAILURE      (NSM_ERR_MCAST_BASE) - 10
#define NSM_ERR_MCAST_MRT_EXISTS               (NSM_ERR_MCAST_BASE) - 11
#define NSM_ERR_MCAST_MRT_LIMIT_EXCEED         (NSM_ERR_MCAST_BASE) - 12
#define NSM_ERR_MCAST_MRT_FWD_ADD_FAILURE      (NSM_ERR_MCAST_BASE) - 13
#define NSM_ERR_MCAST_NO_MRT                   (NSM_ERR_MCAST_BASE) - 14
#define NSM_ERR_MCAST_MRT_NOT_BELONG           (NSM_ERR_MCAST_BASE) - 15
#define NSM_ERR_MCAST_MRT_FWD_DEL_FAILURE      (NSM_ERR_MCAST_BASE) - 16
#define NSM_ERR_MCAST_NO_CLIENT                (NSM_ERR_MCAST_BASE) - 17
#define NSM_ERR_MCAST_NO_REG                   (NSM_ERR_MCAST_BASE) - 18
#define NSM_ERR_MCAST_NO_REG_PKT               (NSM_ERR_MCAST_BASE) - 19
#define NSM_ERR_MCAST_NO_MCAST                 (NSM_ERR_MCAST_BASE) - 20
#define NSM_ERR_MCAST_RT_LIMIT_EXCEED_RTS      (NSM_ERR_MCAST_BASE) - 21
#define NSM_ERR_MCAST_RT_THRESH_EXCEED_RT_LIMIT  (NSM_ERR_MCAST_BASE) - 22
#define NSM_ERR_MCAST_VIF_GMP_FAILURE            (NSM_ERR_MCAST_BASE) - 23
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#define NSM_ERR_MCAST6_BASE                     -500
#define NSM_ERR_MCAST6_FIB_SOCK_FAILURE         (NSM_ERR_MCAST6_BASE) - 1
#define NSM_ERR_MCAST6_PIM_SOCK_FAILURE         (NSM_ERR_MCAST6_BASE) - 2
#define NSM_ERR_MCAST6_MFWD_FAILURE             (NSM_ERR_MCAST6_BASE) - 3
#define NSM_ERR_MCAST6_NO_INTERFACE             (NSM_ERR_MCAST6_BASE) - 4
#define NSM_ERR_MCAST6_MIF_EXISTS               (NSM_ERR_MCAST6_BASE) - 5
#define NSM_ERR_MCAST6_NO_MIF_INDEX             (NSM_ERR_MCAST6_BASE) - 6
#define NSM_ERR_MCAST6_MIF_FWD_ADD_FAILURE      (NSM_ERR_MCAST6_BASE) - 7
#define NSM_ERR_MCAST6_NO_MIF                   (NSM_ERR_MCAST6_BASE) - 8
#define NSM_ERR_MCAST6_MIF_NOT_BELONG           (NSM_ERR_MCAST6_BASE) - 9
#define NSM_ERR_MCAST6_MIF_FWD_DEL_FAILURE      (NSM_ERR_MCAST6_BASE) - 10
#define NSM_ERR_MCAST6_MRT_EXISTS               (NSM_ERR_MCAST6_BASE) - 11
#define NSM_ERR_MCAST6_MRT_LIMIT_EXCEED         (NSM_ERR_MCAST6_BASE) - 12
#define NSM_ERR_MCAST6_MRT_FWD_ADD_FAILURE      (NSM_ERR_MCAST6_BASE) - 13
#define NSM_ERR_MCAST6_NO_MRT                   (NSM_ERR_MCAST6_BASE) - 14
#define NSM_ERR_MCAST6_MRT_NOT_BELONG           (NSM_ERR_MCAST6_BASE) - 15
#define NSM_ERR_MCAST6_MRT_FWD_DEL_FAILURE      (NSM_ERR_MCAST6_BASE) - 16
#define NSM_ERR_MCAST6_NO_CLIENT                (NSM_ERR_MCAST6_BASE) - 17
#define NSM_ERR_MCAST6_NO_REG                   (NSM_ERR_MCAST6_BASE) - 18
#define NSM_ERR_MCAST6_NO_REG_PKT               (NSM_ERR_MCAST6_BASE) - 19
#define NSM_ERR_MCAST6_NO_MCAST                 (NSM_ERR_MCAST6_BASE) - 20
#define NSM_ERR_MCAST6_RT_LIMIT_EXCEED_RTS      (NSM_ERR_MCAST6_BASE) - 21
#define NSM_ERR_MCAST6_RT_THRESH_EXCEED_RT_LIMIT  (NSM_ERR_MCAST6_BASE) - 22

#define NSM_ERR_MCAST6_MIF_GMP_FAILURE            (NSM_ERR_MCAST6_BASE) - 23
#endif /* HAVE_MCAST_IPV6 */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
#define NSM_ERR_L2_MCAST_BASE                   -600
#define NSM_ERR_L2_MCAST_INVALID_ARG            (NSM_ERR_L2_MCAST_BASE) -1
#define NSM_ERR_L2_MCAST_SOCK_FAILURE           (NSM_ERR_L2_MCAST_BASE) -2
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP*/
/* End error definitions. */

#define NSM_INVALID_INTERFACE_INDEX             -1
#define NSM_LOGICAL_INTERFACE_INDEX              500

#ifdef HAVE_DEV_TEST
#define NSM_ASSERT(expr)  pal_assert (expr)
#else
#define NSM_ASSERT(expr)
#endif /* HAVE_DEV_TEST */

/* NSM Feature Capability variables. */
extern u_char nsm_cap_have_vrrp;
extern u_char nsm_cap_have_vrf;
extern u_char nsm_cap_have_mpls;
extern u_char nsm_cap_have_ipv6;

/* Macro to check NSM Capability variables. */
#ifdef HAVE_LICENSE_MGR
#define IF_NSM_CAP(var)              if (nsm_cap_ ## var)
#define NSM_CAP(var)                 (nsm_cap_ ## var)
#else /* HAVE_LICENSE_MGR */
#define IF_NSM_CAP(var)
#define NSM_CAP(var)                 1
#endif /* HAVE_LICENSE_MGR. */

#define IF_NSM_CAP_HAVE_VRRP         IF_NSM_CAP (have_vrrp)
#define NSM_CAP_HAVE_VRRP            NSM_CAP (have_vrrp)
#define IF_NSM_CAP_HAVE_VRF          IF_NSM_CAP (have_vrf)
#define NSM_CAP_HAVE_VRF             NSM_CAP (have_vrf)
#define IF_NSM_CAP_HAVE_MPLS         IF_NSM_CAP (have_mpls)
#define NSM_CAP_HAVE_MPLS            NSM_CAP (have_mpls)
#define IF_NSM_CAP_HAVE_IPV6         IF_NSM_CAP (have_ipv6)
#define NSM_CAP_HAVE_IPV6            NSM_CAP (have_ipv6)

#define NULL_INTERFACE               "Null"
#define IS_NULL_INTERFACE_STR(S)                              \
    (S ? (pal_strncasecmp (S, "Null", 4) == 0) : 0)

/* Macro for un-referenced function parameter */
#define NSM_UNREFERENCED_PARAMETER(PARAM) ((PARAM) = (PARAM))

/* Macro for threshold based if stat update */
#define NSM_IF_STAT_UPDATE(VR) \
  do \
   { \
     if ((VR)->t_if_stat_threshold == NULL) \
      { \
        nsm_if_stat_update (VR); \
        THREAD_TIMER_ON (NSM_ZG, (VR)->t_if_stat_threshold, \
          nsm_if_stat_update_threshold_timer, VR, \
          NSM_IF_STAT_UPDATE_THRESHOLD); \
      } \
   } while (0)

/* Prototypes. */
void interface_list (void);
void if_update (void);
void nsm_cli_init (struct lib_globals *);
void nsm_show_init (struct lib_globals *);
int nsm_if_stat_update_threshold_timer (struct thread *);
int nsm_if_stat_update (struct apn_vr *);
extern struct lib_globals *nzg;
extern struct nsm_globals *ng;

#endif /* _PACOS_NSMD_H */
