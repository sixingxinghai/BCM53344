/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_MSG_H_
#define _HAL_MSG_H_

#include "hal_types.h"
#include "hal_if.h"
#ifdef HAVE_L2LERN
#include "hal_acl.h"
#endif /* HAVE_L2LERN */
#ifdef HAVE_L2
#include "hal_l2.h"
#include "hal_bridge.h"
#endif /* HAVE_L2 */
#ifdef HAVE_L3
#include "hal_l3.h"
#endif /* HAVE_L3 */

#ifdef HAVE_QOS
#include "hal_qos.h"
#endif /* HAVE_QOS */

#ifdef HAVE_DCB
#include "hal_dcb.h"
#endif /* HAVE_DCB */

#if defined HAVE_GVRP || defined HAVE_GMRP || defined HAVE_MVRP \
    || defined HAVE_MMMRP

#include "L2/hal_garp.h"

#endif /* HAVE_GVRP || HAVE_GMRP || HAVE_MVRP || HAVE_MMMRP */

#ifdef HAVE_ONMD
#include "L2/hal_oam.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_MPLS
#include "hal_mpls_types.h"
#endif /* HAVE_MPLS */

/* 
   The following are the message types for the HAL services layer. The messages
   are categorized based on the HAL APIs. 
*/

#define HAL_MSG_BASE                                100
#define HAL_MSG_BASE_MSG(n)                         (HAL_MSG_BASE + (n))

/*
   HAL initialization/deinitialization.
*/
#define HAL_MSG_INIT                                HAL_MSG_BASE_MSG(1)
#define HAL_MSG_DEINIT                              HAL_MSG_BASE_MSG(2)

/* 
   Bridge messages. 
*/
#define HAL_MSG_BRIDGE_BASE                         HAL_MSG_BASE_MSG(10)
#define HAL_MSG_BRIDGE_MSG(n)                       (HAL_MSG_BRIDGE_BASE + (n))
#define HAL_MSG_BRIDGE_INIT                         HAL_MSG_BRIDGE_MSG(1)
#define HAL_MSG_BRIDGE_DEINIT                       HAL_MSG_BRIDGE_MSG(2)
#define HAL_MSG_BRIDGE_ADD                          HAL_MSG_BRIDGE_MSG(3)
#define HAL_MSG_BRIDGE_DELETE                       HAL_MSG_BRIDGE_MSG(4)
#define HAL_MSG_BRIDGE_SET_AGEING_TIME              HAL_MSG_BRIDGE_MSG(5)
#define HAL_MSG_BRIDGE_SET_LEARNING                 HAL_MSG_BRIDGE_MSG(6)
#define HAL_MSG_BRIDGE_ADD_PORT                     HAL_MSG_BRIDGE_MSG(7)
#define HAL_MSG_BRIDGE_DELETE_PORT                  HAL_MSG_BRIDGE_MSG(8)
#define HAL_MSG_BRIDGE_ADD_INSTANCE                 HAL_MSG_BRIDGE_MSG(9)
#define HAL_MSG_BRIDGE_DELETE_INSTANCE              HAL_MSG_BRIDGE_MSG(10)
#define HAL_MSG_BRIDGE_ADD_VLAN_TO_INSTANCE         HAL_MSG_BRIDGE_MSG(11)
#define HAL_MSG_BRIDGE_DELETE_VLAN_FROM_INSTANCE    HAL_MSG_BRIDGE_MSG(12)
#define HAL_MSG_BRIDGE_SET_PORT_STATE               HAL_MSG_BRIDGE_MSG(13)
#define HAL_MSG_BRIDGE_CHANGE_VLAN_TYPE             HAL_MSG_BRIDGE_MSG(14)
#define HAL_MSG_BRIDGE_PROTO_PROCESS                HAL_MSG_BRIDGE_MSG(15)
#define HAL_MSG_BRIDGE_SET_STATE                    HAL_MSG_BRIDGE_MSG(16)
#define HAL_MSG_PBB_DISPATCH_SERVICE                HAL_MSG_BRIDGE_MSG(17)

/* 
   VLAN messages. 
*/
#define HAL_MSG_VLAN_BASE                           HAL_MSG_BASE_MSG(30)
#define HAL_MSG_VLAN_MSG(n)                         (HAL_MSG_VLAN_BASE + (n))
#define HAL_MSG_VLAN_INIT                           HAL_MSG_VLAN_MSG(1)
#define HAL_MSG_VLAN_DEINIT                         HAL_MSG_VLAN_MSG(2)
#define HAL_MSG_VLAN_ADD                            HAL_MSG_VLAN_MSG(3)
#define HAL_MSG_VLAN_DELETE                         HAL_MSG_VLAN_MSG(4)
#define HAL_MSG_VLAN_SET_PORT_TYPE                  HAL_MSG_VLAN_MSG(5)
#define HAL_MSG_VLAN_SET_DEFAULT_PVID               HAL_MSG_VLAN_MSG(6)
#define HAL_MSG_VLAN_ADD_VID_TO_PORT                HAL_MSG_VLAN_MSG(7)
#define HAL_MSG_VLAN_DELETE_VID_FROM_PORT           HAL_MSG_VLAN_MSG(8)
#define HAL_MSG_VLAN_CLASSIFIER_ADD                 HAL_MSG_VLAN_MSG(9)
#define HAL_MSG_VLAN_CLASSIFIER_DELETE              HAL_MSG_VLAN_MSG(10)
#define HAL_MSG_VLAN_STACKING_ENABLE                HAL_MSG_VLAN_MSG(11)
#define HAL_MSG_VLAN_STACKING_DISABLE               HAL_MSG_VLAN_MSG(12)
#define HAL_MSG_VLAN_ADD_VLAN_REG_TAB_ENT           HAL_MSG_VLAN_MSG(13)
#define HAL_MSG_VLAN_DEL_VLAN_REG_TAB_ENT           HAL_MSG_VLAN_MSG(14)
#define HAL_MSG_GXRP_ENABLE                         HAL_MSG_VLAN_MSG(15)
#define HAL_MSG_DOT1Q_SET_PORT_STATE                HAL_MSG_VLAN_MSG(16)
#define HAL_MSG_SET_DTAG_MODE                       HAL_MSG_VLAN_MSG(17)

/* 
   Flow control messages. 
*/
#define HAL_MSG_FLOW_CONTROL_BASE                   HAL_MSG_BASE_MSG(50)
#define HAL_MSG_FLOW_CONTROL_MSG(n)                 (HAL_MSG_FLOW_CONTROL_BASE + (n))
#define HAL_MSG_FLOW_CONTROL_INIT                   HAL_MSG_FLOW_CONTROL_MSG(1)
#define HAL_MSG_FLOW_CONTROL_DEINIT                 HAL_MSG_FLOW_CONTROL_MSG(2)
#define HAL_MSG_FLOW_CONTROL_SET                    HAL_MSG_FLOW_CONTROL_MSG(3)
#define HAL_MSG_FLOW_CONTROL_STATISTICS             HAL_MSG_FLOW_CONTROL_MSG(4)

/* 
   L2 QoS messages. 
*/
#define HAL_MSG_L2_QOS_BASE                         HAL_MSG_BASE_MSG(60)
#define HAL_MSG_L2_QOS_MSG(n)                       (HAL_MSG_L2_QOS_BASE + (n))
#define HAL_MSG_L2_QOS_INIT                         HAL_MSG_L2_QOS_MSG(1)
#define HAL_MSG_L2_QOS_DEINIT                       HAL_MSG_L2_QOS_MSG(2)
#define HAL_MSG_L2_QOS_DEFAULT_USER_PRIORITY_SET    HAL_MSG_L2_QOS_MSG(3)
#define HAL_MSG_L2_QOS_DEFAULT_USER_PRIORITY_GET    HAL_MSG_L2_QOS_MSG(4)
#define HAL_MSG_L2_QOS_REGEN_USER_PRIORITY_SET      HAL_MSG_L2_QOS_MSG(5)
#define HAL_MSG_L2_QOS_REGEN_USER_PRIORITY_GET      HAL_MSG_L2_QOS_MSG(6)
#define HAL_MSG_L2_QOS_TRAFFIC_CLASS_SET            HAL_MSG_L2_QOS_MSG(7)
#define HAL_MSG_L2_QOS_NUM_COSQ_GET                 HAL_MSG_L2_QOS_MSG(8)
#define HAL_MSG_L2_QOS_NUM_COSQ_SET                 HAL_MSG_L2_QOS_MSG(9)

/*
  Rate limiting control messages. 
*/
#define HAL_MSG_RATELIMIT_BASE                    HAL_MSG_BASE_MSG(70)
#define HAL_MSG_RATELIMIT_MSG(n)                  (HAL_MSG_RATELIMIT_BASE + (n))
#define HAL_MSG_RATELIMIT_INIT                    HAL_MSG_RATELIMIT_MSG(1)
#define HAL_MSG_RATELIMIT_DEINIT                  HAL_MSG_RATELIMIT_MSG(2)
#define HAL_MSG_RATELIMIT_BCAST                   HAL_MSG_RATELIMIT_MSG(3)
#define HAL_MSG_RATELIMIT_BCAST_DISCARDS_GET      HAL_MSG_RATELIMIT_MSG(4)
#define HAL_MSG_RATELIMIT_MCAST                   HAL_MSG_RATELIMIT_MSG(5)
#define HAL_MSG_RATELIMIT_MCAST_DISCARDS_GET      HAL_MSG_RATELIMIT_MSG(6)
#define HAL_MSG_RATELIMIT_DLF_BCAST               HAL_MSG_RATELIMIT_MSG(7)
#define HAL_MSG_RATELIMIT_DLF_BCAST_DISCARDS_GET  HAL_MSG_RATELIMIT_MSG(8)
#define HAL_MSG_RATELIMIT_BCAST_MCAST             HAL_MSG_RATELIMIT_MSG(9)
#define HAL_MSG_RATELIMIT_ONLY_BROADCAST          HAL_MSG_RATELIMIT_MSG(10)

/* 
   IGMP Snooping messages. 
*/
#define HAL_MSG_IGMP_SNOOPING_BASE                  HAL_MSG_BASE_MSG(80)
#define HAL_MSG_IGMP_SNOOPING_MSG(n)                (HAL_MSG_IGMP_SNOOPING_BASE + (n))
#define HAL_MSG_IGMP_SNOOPING_INIT                  HAL_MSG_IGMP_SNOOPING_MSG(1)
#define HAL_MSG_IGMP_SNOOPING_DEINIT                HAL_MSG_IGMP_SNOOPING_MSG(2)
#define HAL_MSG_IGMP_SNOOPING_ENABLE                HAL_MSG_IGMP_SNOOPING_MSG(3)
#define HAL_MSG_IGMP_SNOOPING_DISABLE               HAL_MSG_IGMP_SNOOPING_MSG(4)
#define HAL_MSG_IGMP_SNOOPING_ENTRY_ADD             HAL_MSG_IGMP_SNOOPING_MSG(5)
#define HAL_MSG_IGMP_SNOOPING_ENTRY_DELETE          HAL_MSG_IGMP_SNOOPING_MSG(6)
#define HAL_MSG_IGMP_SNOOPING_ENABLE_PORT           HAL_MSG_IGMP_SNOOPING_MSG(7)
#define HAL_MSG_IGMP_SNOOPING_DISABLE_PORT          HAL_MSG_IGMP_SNOOPING_MSG(8)
#define HAL_MSG_L2_UNKNOWN_MCAST_MODE_DISCARD       HAL_MSG_IGMP_SNOOPING_MSG(9)
#define HAL_MSG_L2_UNKNOWN_MCAST_MODE_FLOOD         HAL_MSG_IGMP_SNOOPING_MSG(10)
/* 
   L2 FDB messages. 
*/
#define HAL_MSG_L2_FDB_BASE                         HAL_MSG_BASE_MSG(90)
#define HAL_MSG_L2_FDB_MSG(n)                       (HAL_MSG_L2_FDB_BASE + (n))
#define HAL_MSG_L2_FDB_INIT                         HAL_MSG_L2_FDB_MSG(1)
#define HAL_MSG_L2_FDB_DEINIT                       HAL_MSG_L2_FDB_MSG(2)
#define HAL_MSG_L2_FDB_ADD                          HAL_MSG_L2_FDB_MSG(3)
#define HAL_MSG_L2_FDB_DELETE                       HAL_MSG_L2_FDB_MSG(4)
#define HAL_MSG_L2_FDB_UNICAST_GET                  HAL_MSG_L2_FDB_MSG(5)
#define HAL_MSG_L2_FDB_MULTICAST_GET                HAL_MSG_L2_FDB_MSG(6)
#define HAL_MSG_L2_FDB_FLUSH_PORT                   HAL_MSG_L2_FDB_MSG(7)
#define HAL_MSG_L2_FLUSH_FDB_BY_MAC                 HAL_MSG_L2_FDB_MSG(8)
#define HAL_MSG_L2_FDB_MAC_PRIO_ADD                 HAL_MSG_L2_FDB_MSG(9)
#define HAL_MSG_L2_FDB_GET_MAC_VID                  HAL_MSG_L2_FDB_MSG(10)

/* 
   L2 FDB MSG Flags 
*/
#define HAL_L2_DELETE_STATIC                        1
#define HAL_L2_DELETE_DYNAMIC                       2

/*
  Port mirroring. 
*/
#define HAL_MSG_L2_PMIRROR_BASE                     HAL_MSG_BASE_MSG(100)
#define HAL_MSG_L2_PMIRROR_MSG(n)                   (HAL_MSG_L2_PMIRROR_BASE + (n))
#define HAL_MSG_PMIRROR_INIT                        HAL_MSG_L2_PMIRROR_MSG(1)
#define HAL_MSG_PMIRROR_DEINIT                      HAL_MSG_L2_PMIRROR_MSG(2)
#define HAL_MSG_PMIRROR_SET                         HAL_MSG_L2_PMIRROR_MSG(3)
#define HAL_MSG_PMIRROR_UNSET                       HAL_MSG_L2_PMIRROR_MSG(4)

/* 
   Link aggregation. 
*/
#define HAL_MSG_LACP_BASE                           HAL_MSG_BASE_MSG(110)
#define HAL_MSG_LACP_MSG(n)                         (HAL_MSG_LACP_BASE + (n))
#define HAL_MSG_LACP_INIT                           HAL_MSG_LACP_MSG(1)
#define HAL_MSG_LACP_DEINIT                         HAL_MSG_LACP_MSG(2)
#define HAL_MSG_LACP_ADD_AGGREGATOR                 HAL_MSG_LACP_MSG(3)
#define HAL_MSG_LACP_DELETE_AGGREGATOR              HAL_MSG_LACP_MSG(4)
#define HAL_MSG_LACP_ATTACH_MUX_TO_AGGREGATOR       HAL_MSG_LACP_MSG(5)
#define HAL_MSG_LACP_DETACH_MUX_FROM_AGGREGATOR     HAL_MSG_LACP_MSG(6)
#define HAL_MSG_LACP_PSC_SET                        HAL_MSG_LACP_MSG(7)
#define HAL_MSG_LACP_COLLECTING                     HAL_MSG_LACP_MSG(8)
#define HAL_MSG_LACP_DISTRIBUTING                   HAL_MSG_LACP_MSG(9)
#define HAL_MSG_LACP_COLLECTING_DISTRIBUTING        HAL_MSG_LACP_MSG(10)

/*
  Port authentication. 
*/
#define HAL_MSG_8021x_BASE                          HAL_MSG_BASE_MSG(140)
#define HAL_MSG_8021x_MSG(n)                        (HAL_MSG_8021x_BASE + (n))
#define HAL_MSG_8021x_INIT                          HAL_MSG_8021x_MSG(1)
#define HAL_MSG_8021x_DEINIT                        HAL_MSG_8021x_MSG(2)
#define HAL_MSG_8021x_PORT_STATE                    HAL_MSG_8021x_MSG(3)

#ifdef HAVE_MAC_AUTH
#define HAL_MSG_AUTH_MAC_PORT_STATE                 HAL_MSG_8021x_MSG(4)
#define HAL_MSG_AUTH_MAC_UNSET_IRQ                  HAL_MSG_8021x_MSG(5)
#endif

#ifdef HAVE_QOS
/*
   QoS messages.
*/
#define HAL_MSG_QOS_BASE                                 (HAL_MSG_BASE + 150)
#define HAL_MSG_QOS_INIT                                 (HAL_MSG_QOS_BASE + 1)
#define HAL_MSG_QOS_DEINIT                               (HAL_MSG_QOS_BASE + 2)
#define HAL_MSG_QOS_ENABLE                               (HAL_MSG_QOS_BASE + 3)
#define HAL_MSG_QOS_DISABLE                              (HAL_MSG_QOS_BASE + 4)
#define HAL_MSG_QOS_WRR_QUEUE_LIMIT                      (HAL_MSG_QOS_BASE + 5)
#define HAL_MSG_QOS_WRR_TAIL_DROP_THRESHOLD              (HAL_MSG_QOS_BASE + 6)
#define HAL_MSG_QOS_WRR_THRESHOLD_DSCP_MAP               (HAL_MSG_QOS_BASE + 7)
#define HAL_MSG_QOS_WRR_WRED_DROP_THRESHOLD              (HAL_MSG_QOS_BASE + 8)
#define HAL_MSG_QOS_WRR_SET_BANDWIDTH                    (HAL_MSG_QOS_BASE + 9)
#define HAL_MSG_QOS_WRR_QUEUE_COS_MAP_SET                (HAL_MSG_QOS_BASE + 10)
#define HAL_MSG_QOS_WRR_QUEUE_COS_MAP_UNSET              (HAL_MSG_QOS_BASE + 11)
#define HAL_MSG_QOS_WRR_QUEUE_MIN_RESERVE                (HAL_MSG_QOS_BASE + 12)
#define HAL_MSG_QOS_SET_TRUST_STATE                      (HAL_MSG_QOS_BASE + 13)
#define HAL_MSG_QOS_SET_DEFAULT_COS                      (HAL_MSG_QOS_BASE + 14)
#define HAL_MSG_QOS_SET_DSCP_MAP_TBL                     (HAL_MSG_QOS_BASE + 15)
#define HAL_MSG_QOS_SET_CLASS_MAP                        (HAL_MSG_QOS_BASE + 16)
#define HAL_MSG_QOS_SET_POLICY_MAP                       (HAL_MSG_QOS_BASE + 17)
#define HAL_MSG_QOS_SET_CMAP_COS_INNER                   (HAL_MSG_QOS_BASE + 18)
#define HAL_MSG_QOS_PORT_ENABLE                          (HAL_MSG_QOS_BASE + 19)
#define HAL_MSG_QOS_PORT_DISABLE                         (HAL_MSG_QOS_BASE + 20)
#define HAL_MSG_QOS_SET_COS_TO_QUEUE                     (HAL_MSG_QOS_BASE + 21)
#define HAL_MSG_QOS_SET_DSCP_TO_QUEUE                    (HAL_MSG_QOS_BASE + 22)
#define HAL_MSG_QOS_SET_FORCE_TRUST_COS                  (HAL_MSG_QOS_BASE + 23)
#define HAL_MSG_QOS_SET_FRAME_TYPE_PRIORITY_OVERRIDE     (HAL_MSG_QOS_BASE + 24)
#define HAL_MSG_QOS_SET_VLAN_PRIORITY                    (HAL_MSG_QOS_BASE + 25)
#define HAL_MSG_QOS_UNSET_VLAN_PRIORITY                  (HAL_MSG_QOS_BASE + 26)
#define HAL_MSG_QOS_SET_PORT_VLAN_PRIORITY_OVERRIDE      (HAL_MSG_QOS_BASE + 27)
#define HAL_MSG_QOS_SET_QUEUE_WEIGHT                     (HAL_MSG_QOS_BASE + 28)
#define HAL_MSG_QOS_UNSET_FRAME_TYPE_PRIORITY_OVERRIDE   (HAL_MSG_QOS_BASE + 29)
#define HAL_MSG_QOS_SET_EGRESS_RATE_SHAPE                (HAL_MSG_QOS_BASE + 30)
#define HAL_MSG_QOS_SET_STRICT_QUEUE                     (HAL_MSG_QOS_BASE + 31)
#define HAL_MSG_QOS_SET_PORT_DA_PRIORITY_OVERRIDE        (HAL_MSG_QOS_BASE + 32)
#endif /* HAVE_QOS */

/*
   MLD Snooping messages.
*/
#define HAL_MSG_MLD_SNOOPING_BASE                  HAL_MSG_BASE_MSG(200)
#define HAL_MSG_MLD_SNOOPING_MSG(n)                (HAL_MSG_MLD_SNOOPING_BASE + (n))
#define HAL_MSG_MLD_SNOOPING_INIT                  HAL_MSG_MLD_SNOOPING_MSG(1)
#define HAL_MSG_MLD_SNOOPING_DEINIT                HAL_MSG_MLD_SNOOPING_MSG(2)
#define HAL_MSG_MLD_SNOOPING_ENABLE                HAL_MSG_MLD_SNOOPING_MSG(3)
#define HAL_MSG_MLD_SNOOPING_DISABLE               HAL_MSG_MLD_SNOOPING_MSG(4)
#define HAL_MSG_MLD_SNOOPING_ENTRY_ADD             HAL_MSG_MLD_SNOOPING_MSG(5)
#define HAL_MSG_MLD_SNOOPING_ENTRY_DELETE          HAL_MSG_MLD_SNOOPING_MSG(6)
#define HAL_MSG_MLD_SNOOPING_ENABLE_PORT           HAL_MSG_MLD_SNOOPING_MSG(7)
#define HAL_MSG_MLD_SNOOPING_DISABLE_PORT          HAL_MSG_MLD_SNOOPING_MSG(8)

/* 
   Private Vlan messages
*/
#define HAL_MSG_PVLAN_BASE                          HAL_MSG_BASE_MSG(250)
#define HAL_MSG_PVLAN_MSG(n)                        (HAL_MSG_PVLAN_BASE + (n))
#define HAL_MSG_PVLAN_SET_VLAN_TYPE                 HAL_MSG_PVLAN_MSG(1)
#define HAL_MSG_PVLAN_VLAN_ASSOCIATE                HAL_MSG_PVLAN_MSG(2)
#define HAL_MSG_PVLAN_VLAN_DISSOCIATE               HAL_MSG_PVLAN_MSG(3)
#define HAL_MSG_PVLAN_PORT_ADD                      HAL_MSG_PVLAN_MSG(4)
#define HAL_MSG_PVLAN_PORT_DELETE                   HAL_MSG_PVLAN_MSG(5)
#define HAL_MSG_PVLAN_SET_PORT_MODE                 HAL_MSG_PVLAN_MSG(6)

/* 
   Interface manager messages. 
*/
#define HAL_MSG_IF_BASE                             HAL_MSG_BASE_MSG(500)
#define HAL_MSG_IF_MSG(n)                           (HAL_MSG_IF_BASE + (n))
#define HAL_MSG_IF_NEWLINK                          HAL_MSG_IF_MSG(1)
#define HAL_MSG_IF_DELLINK                          HAL_MSG_IF_MSG(2)
#define HAL_MSG_IF_GETLINK                          HAL_MSG_IF_MSG(3)
#define HAL_MSG_IF_GET_METRIC                       HAL_MSG_IF_MSG(4)
#define HAL_MSG_IF_GET_MTU                          HAL_MSG_IF_MSG(5)
#define HAL_MSG_IF_SET_MTU                          HAL_MSG_IF_MSG(6)
#define HAL_MSG_IF_GET_HWADDR                       HAL_MSG_IF_MSG(7)
#define HAL_MSG_IF_SET_HWADDR                       HAL_MSG_IF_MSG(8)
#define HAL_MSG_IF_SET_SEC_HWADDRS                  HAL_MSG_IF_MSG(9)
#define HAL_MSG_IF_ADD_SEC_HWADDRS                  HAL_MSG_IF_MSG(10)
#define HAL_MSG_IF_DELETE_SEC_HWADDRS               HAL_MSG_IF_MSG(11)
#define HAL_MSG_IF_GET_FLAGS                        HAL_MSG_IF_MSG(12)
#define HAL_MSG_IF_SET_FLAGS                        HAL_MSG_IF_MSG(13)
#define HAL_MSG_IF_UNSET_FLAGS                      HAL_MSG_IF_MSG(14)
#define HAL_MSG_IF_GET_BW                           HAL_MSG_IF_MSG(15)
#define HAL_MSG_IF_SET_BW                           HAL_MSG_IF_MSG(16)
#define HAL_MSG_IF_IPV4_NEWADDR                     HAL_MSG_IF_MSG(17)
#define HAL_MSG_IF_IPV4_DELADDR                     HAL_MSG_IF_MSG(18)
#define HAL_MSG_IF_SET_PORT_TYPE                    HAL_MSG_IF_MSG(19)
#define HAL_MSG_IF_CREATE_SVI                       HAL_MSG_IF_MSG(20)
#define HAL_MSG_IF_DELETE_SVI                       HAL_MSG_IF_MSG(21)
#define HAL_MSG_IF_COUNTERS_GET                     HAL_MSG_IF_MSG(22)
#define HAL_MSG_IF_L3_INIT                          HAL_MSG_IF_MSG(23) 
#define HAL_MSG_IF_L2_INIT                          HAL_MSG_IF_MSG(24) 
#define HAL_MSG_IF_GET_DUPLEX                       HAL_MSG_IF_MSG(25)
#define HAL_MSG_IF_SET_DUPLEX                       HAL_MSG_IF_MSG(26)
#define HAL_MSG_IF_SET_AUTONEGO                     HAL_MSG_IF_MSG(27)
#define HAL_MSG_IF_GET_ARP_AGEING_TIMEOUT           HAL_MSG_IF_MSG(28)
#define HAL_MSG_IF_SET_ARP_AGEING_TIMEOUT           HAL_MSG_IF_MSG(29)
#ifdef HAVE_IPV6
#define HAL_MSG_IF_IPV6_NEWADDR                     HAL_MSG_IF_MSG(30)
#define HAL_MSG_IF_IPV6_DELADDR                     HAL_MSG_IF_MSG(31)
#endif /* HAVE_IPV6 */
#define HAL_MSG_IF_STP_REFRESH                      HAL_MSG_IF_MSG(32)
#define HAL_MSG_IF_UPDATE                           HAL_MSG_IF_MSG(33)
#define HAL_MSG_IF_CLEANUP_DONE                     HAL_MSG_IF_MSG(33)
#define HAL_MSG_IF_FIB_BIND                         HAL_MSG_IF_MSG(34)
#define HAL_MSG_IF_FIB_UNBIND                       HAL_MSG_IF_MSG(35)
#define HAL_MSG_IF_STATS_FLUSH                      HAL_MSG_IF_MSG(36)
#define HAL_MSG_IF_SET_MDIX                         HAL_MSG_IF_MSG(37)
#define HAL_MSG_IF_PORTBASED_VLAN                   HAL_MSG_IF_MSG(38) 
#define HAL_MSG_IF_PORT_EGRESS                      HAL_MSG_IF_MSG(39)
#define HAL_MSG_IF_CPU_DEFAULT_VLAN                 HAL_MSG_IF_MSG(40)
#define HAL_MSG_IF_SET_FORCE_VLAN                   HAL_MSG_IF_MSG(41)
#define HAL_MSG_IF_SET_ETHER_TYPE                   HAL_MSG_IF_MSG(42)
#define HAL_MSG_IF_SET_LEARN_DISABLE                HAL_MSG_IF_MSG(43)
#define HAL_MSG_IF_GET_LEARN_DISABLE                HAL_MSG_IF_MSG(44)         
#define HAL_MSG_IF_SET_SW_RESET                     HAL_MSG_IF_MSG(45)
#define HAL_MSG_IF_WAYSIDE_DEFAULT_VLAN             HAL_MSG_IF_MSG(46)
#define HAL_MSG_IF_PRESERVE_CE_COS                  HAL_MSG_IF_MSG(47) 
#define HAL_MSG_IF_CLEAR_COUNTERS                   HAL_MSG_IF_MSG(48)
#define HAL_MSG_IF_L3_DEINIT                        HAL_MSG_IF_MSG(49) 
#define HAL_MSG_IF_L3_STATUS                        HAL_MSG_IF_MSG(50)
#define HAL_MSG_VRRP_UPDATE                         HAL_MSG_IF_MSG(51)
#ifdef HAVE_IPV6
#define HAL_MSG_IF_IPV6_L3_STATUS                   HAL_MSG_IF_MSG(52)
#endif /* HAVE_IPV6 */

/* 
   FIB management messages. 
*/
#define HAL_MSG_FIB_BASE                            HAL_MSG_BASE_MSG(600)
#define HAL_MSG_FIB_MSG(n)                          (HAL_MSG_FIB_BASE + (n))
#define HAL_MSG_FIB_CREATE                          HAL_MSG_FIB_MSG(1)
#define HAL_MSG_FIB_DELETE                          HAL_MSG_FIB_MSG(2)
#define HAL_MSG_IPV4_INIT                           HAL_MSG_FIB_MSG(3)
#define HAL_MSG_IPV4_DEINIT                         HAL_MSG_FIB_MSG(4)
#define HAL_MSG_IPV6_INIT                           HAL_MSG_FIB_MSG(5)
#define HAL_MSG_IPV6_DEINIT                         HAL_MSG_FIB_MSG(6)
#define HAL_MSG_GET_MAX_MULTIPATH                   HAL_MSG_FIB_MSG(7)

/* 
   IPv4 address management messages. 
*/
#define HAL_MSG_IF_IPV4_MSG_BASE                    HAL_MSG_BASE_MSG(650)
#define HAL_MSG_IF_IPV4_MSG(n)                      (HAL_MSG_IF_IPV4_MSG_BASE + (n))
#define HAL_MSG_IF_IPV4_ADDRESS_ADD                 HAL_MSG_IF_IPV4_MSG(1)
#define HAL_MSG_IF_IPV4_ADDRESS_DELETE              HAL_MSG_IF_IPV4_MSG(2)

/* IPv4 messages. */
#define HAL_MSG_IPV4_MSG_BASE                       HAL_MSG_BASE_MSG(700)
#define HAL_MSG_IPV4_MSG(n)                         (HAL_MSG_IPV4_MSG_BASE + (n))
#define HAL_MSG_IPV4_UC_INIT                        HAL_MSG_IPV4_MSG(1)
#define HAL_MSG_IPV4_UC_DEINIT                      HAL_MSG_IPV4_MSG(2)
#define HAL_MSG_IPV4_UC_ADD                         HAL_MSG_IPV4_MSG(3)
#define HAL_MSG_IPV4_UC_DELETE                      HAL_MSG_IPV4_MSG(4)
#define HAL_MSG_IPV4_UC_UPDATE                      HAL_MSG_IPV4_MSG(5)

#ifdef HAVE_PBR
#define HAL_MSG_PBR_IPV4_UC_ADD                     HAL_MSG_IPV4_MSG(6)
#define HAL_MSG_PBR_IPV4_UC_DELETE                  HAL_MSG_IPV4_MSG(7)
#endif /* HAVE_PBR */

/* IPv4 Multicast messages. */
#define HAL_MSG_IPV4_MC_BASE                        HAL_MSG_BASE_MSG(750)
#define HAL_MSG_IPV4_MC_MSG(n)                      (HAL_MSG_IPV4_MC_BASE + (n))
#define HAL_MSG_IPV4_MC_INIT                        HAL_MSG_IPV4_MC_MSG(1)
#define HAL_MSG_IPV4_MC_DEINIT                      HAL_MSG_IPV4_MC_MSG(2)
#define HAL_MSG_IPV4_MC_PIM_INIT                    HAL_MSG_IPV4_MC_MSG(3)
#define HAL_MSG_IPV4_MC_PIM_DEINIT                  HAL_MSG_IPV4_MC_MSG(4)
#define HAL_MSG_IPV4_MC_VIF_ADD                     HAL_MSG_IPV4_MC_MSG(5)
#define HAL_MSG_IPV4_MC_VIF_DEL                     HAL_MSG_IPV4_MC_MSG(6)
#define HAL_MSG_IPV4_MC_MRT_ADD                     HAL_MSG_IPV4_MC_MSG(7)
#define HAL_MSG_IPV4_MC_MRT_DEL                     HAL_MSG_IPV4_MC_MSG(8)
#define HAL_MSG_IPV4_MC_SG_STAT                     HAL_MSG_IPV4_MC_MSG(9)

/*
   IPv6 address management messages.
*/
#define HAL_MSG_IF_IPV6_MSG_BASE                    HAL_MSG_BASE_MSG(800)
#define HAL_MSG_IF_IPV6_MSG(n)                      (HAL_MSG_IF_IPV6_MSG_BASE + (n))
#define HAL_MSG_IF_IPV6_ADDRESS_ADD                 HAL_MSG_IF_IPV6_MSG(1)
#define HAL_MSG_IF_IPV6_ADDRESS_DELETE              HAL_MSG_IF_IPV6_MSG(2)

/* IPv6 messages. */
#define HAL_MSG_IPV6_MSG_BASE                       HAL_MSG_BASE_MSG(850)
#define HAL_MSG_IPV6_MSG(n)                         (HAL_MSG_IPV6_MSG_BASE + (n))
#define HAL_MSG_IPV6_UC_INIT                        HAL_MSG_IPV6_MSG(1)
#define HAL_MSG_IPV6_UC_DEINIT                      HAL_MSG_IPV6_MSG(2)
#define HAL_MSG_IPV6_UC_ADD                         HAL_MSG_IPV6_MSG(3)
#define HAL_MSG_IPV6_UC_DELETE                      HAL_MSG_IPV6_MSG(4)
#define HAL_MSG_IPV6_UC_UPDATE                      HAL_MSG_IPV6_MSG(5)

/* IPv6 Multicast messages. */
#define HAL_MSG_IPV6_MC_BASE                        HAL_MSG_BASE_MSG(900)
#define HAL_MSG_IPV6_MC_MSG(n)                      (HAL_MSG_IPV6_MC_BASE + (n))
#define HAL_MSG_IPV6_MC_INIT                        HAL_MSG_IPV6_MC_MSG(1)
#define HAL_MSG_IPV6_MC_DEINIT                      HAL_MSG_IPV6_MC_MSG(2)
#define HAL_MSG_IPV6_MC_PIM_INIT                    HAL_MSG_IPV6_MC_MSG(3)
#define HAL_MSG_IPV6_MC_PIM_DEINIT                  HAL_MSG_IPV6_MC_MSG(4)
#define HAL_MSG_IPV6_MC_VIF_ADD                     HAL_MSG_IPV6_MC_MSG(5)
#define HAL_MSG_IPV6_MC_VIF_DEL                     HAL_MSG_IPV6_MC_MSG(6)
#define HAL_MSG_IPV6_MC_MRT_ADD                     HAL_MSG_IPV6_MC_MSG(7)
#define HAL_MSG_IPV6_MC_MRT_DEL                     HAL_MSG_IPV6_MC_MSG(8)
#define HAL_MSG_IPV6_MC_SG_STAT                     HAL_MSG_IPV6_MC_MSG(9)

/* MPLS MESSAGES */
#define HAL_MSG_MPLS_BASE                           HAL_MSG_BASE_MSG(950)
#define HAL_MSG_MPLS_MSG(n)                         (HAL_MSG_MPLS_BASE + (n))
#define HAL_MSG_MPLS_INIT                           HAL_MSG_MPLS_MSG(1)
#define HAL_MSG_MPLS_IF_INIT                        HAL_MSG_MPLS_MSG(2)
#define HAL_MSG_MPLS_VRF_INIT                       HAL_MSG_MPLS_MSG(3)
#define HAL_MSG_MPLS_NEWILM                         HAL_MSG_MPLS_MSG(4)
#define HAL_MSG_MPLS_DELILM                         HAL_MSG_MPLS_MSG(5)
#define HAL_MSG_MPLS_NEWFTN                         HAL_MSG_MPLS_MSG(6)
#define HAL_MSG_MPLS_DELFTN                         HAL_MSG_MPLS_MSG(7)
#define HAL_MSG_MPLS_FIB_CLEAN                      HAL_MSG_MPLS_MSG(8)
#define HAL_MSG_MPLS_VRF_CLEAN                      HAL_MSG_MPLS_MSG(9)
#define HAL_MSG_MPLS_IF_UPDATE                      HAL_MSG_MPLS_MSG(10)
#define HAL_MSG_MPLS_VRF_END                        HAL_MSG_MPLS_MSG(11)
#define HAL_MSG_MPLS_IF_END                         HAL_MSG_MPLS_MSG(12)
#define HAL_MSG_MPLS_END                            HAL_MSG_MPLS_MSG(13)
#define HAL_MSG_MPLS_TTL_HANDLING                   HAL_MSG_MPLS_MSG(14)
#define HAL_MSG_MPLS_LOCAL_PKT                      HAL_MSG_MPLS_MSG(15)
#define HAL_MSG_MPLS_DEBUGGING                      HAL_MSG_MPLS_MSG(16)
#define HAL_MSG_MPLS_VC_INIT                        HAL_MSG_MPLS_MSG(17)
#define HAL_MSG_MPLS_VC_END                         HAL_MSG_MPLS_MSG(18)
#define HAL_MSG_MPLS_VC_FIB_ADD                     HAL_MSG_MPLS_MSG(19)
#define HAL_MSG_MPLS_VC_FIB_DEL                     HAL_MSG_MPLS_MSG(20)
#define HAL_MSG_MPLS_VPLS_ADD                       HAL_MSG_MPLS_MSG(21)
#define HAL_MSG_MPLS_VPLS_DEL                       HAL_MSG_MPLS_MSG(22)
#define HAL_MSG_MPLS_VPLS_IF_BIND                   HAL_MSG_MPLS_MSG(23)
#define HAL_MSG_MPLS_VPLS_IF_UNBIND                 HAL_MSG_MPLS_MSG(24)
#define HAL_MSG_MPLS_STATS_CLEAR                    HAL_MSG_MPLS_MSG(25)


/* ARP Messages */
#define HAL_MSG_ARP_BASE                            HAL_MSG_BASE_MSG(1000)
#define HAL_MSG_ARP_MSG(n)                          (HAL_MSG_ARP_BASE + (n))
#define HAL_MSG_ARP_ADD                             HAL_MSG_ARP_MSG(1)
#define HAL_MSG_ARP_DEL                             HAL_MSG_ARP_MSG(2)
#define HAL_MSG_ARP_DEL_ALL                         HAL_MSG_ARP_MSG(3)
#define HAL_MSG_ARP_CACHE_GET                       HAL_MSG_ARP_MSG(4)


/* IPV6 neighbor Messages */
#define HAL_MSG_IPV6_NBR_BASE                       HAL_MSG_BASE_MSG(1050)
#define HAL_MSG_IPV6_NBR_MSG(n)                     (HAL_MSG_IPV6_NBR_BASE + (n))
#define HAL_MSG_IPV6_NBR_ADD                        HAL_MSG_IPV6_NBR_MSG(1)
#define HAL_MSG_IPV6_NBR_DEL                        HAL_MSG_IPV6_NBR_MSG(2)
#define HAL_MSG_IPV6_NBR_DEL_ALL                    HAL_MSG_IPV6_NBR_MSG(3)
#define HAL_MSG_IPV6_NBR_CACHE_GET                  HAL_MSG_IPV6_NBR_MSG(4)

#define HAL_MSG_ACL_BASE                            HAL_MSG_BASE_MSG(1100)
#define HAL_MSG_ACL_MSG(n)                          (HAL_MSG_ACL_BASE + (n))
#define HAL_MSG_MAC_SET_ACCESS_GROUP                HAL_MSG_ACL_MSG(1)
#define HAL_MSG_VLAN_SET_ACCESS_MAP                 HAL_MSG_ACL_MSG(2)
#define HAL_MSG_IP_SET_ACCESS_GROUP                 HAL_MSG_ACL_MSG(3)
#define HAL_MSG_IP_UNSET_ACCESS_GROUP               HAL_MSG_ACL_MSG(4)
#define HAL_MSG_IP_SET_ACCESS_GROUP_INTERFACE       HAL_MSG_ACL_MSG(5)
#define HAL_MSG_IP_UNSET_ACCESS_GROUP_INTERFACE     HAL_MSG_ACL_MSG(6)


/* CPU information and stacking related messages */
#define HAL_MSG_CPU_BASE                            HAL_MSG_BASE_MSG(1150)
#define HAL_MSG_CPU_MSG(n)                          (HAL_MSG_CPU_BASE + (n))
#define HAL_MSG_CPU_GET_NUM                         HAL_MSG_CPU_MSG(1)
#define HAL_MSG_CPU_GETDB_INFO                      HAL_MSG_CPU_MSG(2)
#define HAL_MSG_CPU_GET_MASTER                      HAL_MSG_CPU_MSG(3)
#define HAL_MSG_CPU_SET_MASTER                      HAL_MSG_CPU_MSG(4)
#define HAL_MSG_CPU_GETDB_INDEX                     HAL_MSG_CPU_MSG(5)
#define HAL_MSG_CPU_GET_LOCAL                       HAL_MSG_CPU_MSG(6)
#define HAL_MSG_CPU_GETDUMP_INDEX                   HAL_MSG_CPU_MSG(7)

/* Registers related messages */
#define HAL_MSG_REG_GET                        HAL_MSG_BASE_MSG(1200)
#define HAL_MSG_REG_SET                        HAL_MSG_BASE_MSG(1201)

/* Statistics related messages */
#define HAL_MSG_STATS_VLAN_GET                 HAL_MSG_BASE_MSG(1202)
#define HAL_MSG_STATS_PORT_GET                 HAL_MSG_BASE_MSG(1203)
#define HAL_MSG_STATS_HOST_GET                 HAL_MSG_BASE_MSG(1204)
#define HAL_MSG_STATS_CLEAR                    HAL_MSG_BASE_MSG(1205)

/* Flow Control messages*/
#define HAL_MSG_FLOW_CONTROL_WM_SET            HAL_MSG_BASE_MSG(1206)

/* EFM */
#define HAL_MSG_EFM_BASE                        HAL_MSG_BASE_MSG(1250)
#define HAL_MSG_EFM_MSG(n)                      (HAL_MSG_EFM_BASE + (n))
#define HAL_MSG_EFM_SET_PORT_STATE              HAL_MSG_EFM_MSG(1)
#define HAL_MSG_EFM_DYING_GASP                  HAL_MSG_EFM_MSG(2)
#define HAL_MSG_EFM_SET_FRAME_PERIOD_WINDOW     HAL_MSG_EFM_MSG(3)
#define HAL_MSG_EFM_SET_SYMBOL_PERIOD_WINDOW    HAL_MSG_EFM_MSG(4)
#define HAL_MSG_EFM_SYMBOL_PERIOD_WINDOW_EXPIRY HAL_MSG_EFM_MSG(5)
#define HAL_MSG_EFM_FRAME_PERIOD_WINDOW_EXPIRY  HAL_MSG_EFM_MSG(6) 
#define HAL_MSG_EFM_FRAME_ERROR_COUNT           HAL_MSG_EFM_MSG(7)
#define HAL_MSG_EFM_FRAME_ERROR_SECONDS_COUNT   HAL_MSG_EFM_MSG(8)

#define HAL_MSG_L2_OAM_BASE                    HAL_MSG_BASE_MSG(1300)
#define HAL_MSG_L2_OAM_MSG(n)                  (HAL_MSG_L2_OAM_BASE + (n))
#define HAL_MSG_L2_OAM_SET_HW_ADDR             HAL_MSG_L2_OAM_MSG(1)
#define HAL_MSG_L2_OAM_SET_ETHER_TYPE          HAL_MSG_L2_OAM_MSG(2)

/* CFM */
#define HAL_MSG_CFM_BASE                       HAL_MSG_BASE_MSG(1350)
#define HAL_MSG_CFM_MSG(n)                     (HAL_MSG_CFM_BASE + (n))
#define HAL_MSG_CFM_TRAP_PDU                   HAL_MSG_CFM_MSG(1)
#define HAL_MSG_CFM_UNTRAP_PDU                 HAL_MSG_CFM_MSG(2)


/* DCB Messages */
#define HAL_MSG_DCB_BASE                          HAL_MSG_BASE_MSG(1400)        
#define HAL_MSG_DCB_MSG(n)                        (HAL_MSG_DCB_BASE + (n))
#define HAL_MSG_DCB_INIT                          HAL_MSG_DCB_MSG(1)
#define HAL_MSG_DCB_DEINIT                        HAL_MSG_DCB_MSG(2)
#define HAL_MSG_DCB_ENABLE                        HAL_MSG_DCB_MSG(3)
#define HAL_MSG_DCB_DISABLE                       HAL_MSG_DCB_MSG(4)
#define HAL_MSG_DCB_ETS_ENABLE                    HAL_MSG_DCB_MSG(5)
#define HAL_MSG_DCB_ETS_DISABLE                   HAL_MSG_DCB_MSG(6)
#define HAL_MSG_DCB_IF_ENABLE                     HAL_MSG_DCB_MSG(7)
#define HAL_MSG_DCB_IF_DISABLE                    HAL_MSG_DCB_MSG(8)
#define HAL_MSG_DCB_ETS_IF_ENABLE                 HAL_MSG_DCB_MSG(9)
#define HAL_MSG_DCB_ETS_IF_DISABLE                HAL_MSG_DCB_MSG(10)
#define HAL_MSG_DCB_ETS_SELECT_MODE               HAL_MSG_DCB_MSG(11)
#define HAL_MSG_DCB_ETS_ADD_PRI_TO_TCG            HAL_MSG_DCB_MSG(12)
#define HAL_MSG_DCB_ETS_REMOVE_PRI_FROM_TCG       HAL_MSG_DCB_MSG(13)
#define HAL_MSG_DCB_ETS_TCG_BW_SET                HAL_MSG_DCB_MSG(14)
#define HAL_MSG_DCB_ETS_APP_PRI_SET               HAL_MSG_DCB_MSG(15)
#define HAL_MSG_DCB_ETS_APP_PRI_UNSET             HAL_MSG_DCB_MSG(16)
#define HAL_MSG_DCB_PFC_ENABLE                    HAL_MSG_DCB_MSG(17)
#define HAL_MSG_DCB_PFC_DISABLE                   HAL_MSG_DCB_MSG(18)
#define HAL_MSG_DCB_PFC_IF_ENABLE                 HAL_MSG_DCB_MSG(19)
#define HAL_MSG_DCB_PFC_SELECT_MODE               HAL_MSG_DCB_MSG(20)
#define HAL_MSG_DCB_PFC_SET_CAP                   HAL_MSG_DCB_MSG(21)
#define HAL_MSG_DCB_PFC_SET_LDA                   HAL_MSG_DCB_MSG(22)

/* Tunnel */
#define HAL_MSG_TUNNEL_BASE                    HAL_MSG_BASE_MSG(1450)
#define HAL_MSG_TUNNEL_MSG(n)                  (HAL_MSG_TUNNEL_BASE + (n))
#define HAL_MSG_TUNNEL_ADD                     HAL_MSG_TUNNEL_MSG(1)
#define HAL_MSG_TUNNEL_DELETE                  HAL_MSG_TUNNEL_MSG(2)
#define HAL_MSG_TUNNEL_INIT_SET                HAL_MSG_TUNNEL_MSG(3)
#define HAL_MSG_TUNNEL_INIT_CLEAR              HAL_MSG_TUNNEL_MSG(4)
#define HAL_MSG_TUNNEL_TERM_SET                HAL_MSG_TUNNEL_MSG(5)
#define HAL_MSG_TUNNEL_TERM_CLEAR              HAL_MSG_TUNNEL_MSG(6)

#if 0
/*vpws*/  /*cai 2011-10 vpws module same with vis msg define temp def*/ 
#define HAL_MSG_VPWS_BASE                    HAL_MSG_BASE_MSG(1500)
#define HAL_MSG_VPWS_MSG(n)                  (HAL_MSG_VPWS_BASE + (n))
#define HAL_MSG_VPWS_ADD		       HAL_MSG_VPWS_MSG(1)	
#define HAL_MSG_VPWS_DELETE                    HAL_MSG_VPWS_MSG(2)	

/*djg20111018 vsi Messages*/
#define HAL_MSG_VSI_BASE                           HAL_MSG_BASE_MSG(1550)
#define HAL_MSG_VSI_MSG(n)                         (HAL_MSG_VSI_BASE  + (n))
#define HAL_MSG_VLL_INGRESS	                    HAL_MSG_VSI_MSG(1)
#define HAL_MSG_VLL_EGRESS	                    HAL_MSG_VSI_MSG(2)
#define HAL_MSG_VLL_INTERMEDIATE                    HAL_MSG_VSI_MSG(3)
#define HAL_MSG_VLL_DELETE                          HAL_MSG_VSI_MSG(4)
#define HAL_MSG_TUN_DELETE                          HAL_MSG_VSI_MSG(5)
#define HAL_MSG_VSI_ADD	                            HAL_MSG_VSI_MSG(10)
#define HAL_MSG_VSI_DELETE                          HAL_MSG_VSI_MSG(11)
#endif

/*add cyh 2012 06 21*/
#define HAL_MSG_VPN_BASE                              HAL_MSG_BASE_MSG(1500)
#define HAL_MSG_VPN_MSG(n)                         (HAL_MSG_VPN_BASE  + (n))
#define HAL_MSG_VPN_ADD	                        HAL_MSG_VPN_MSG(1)
#define HAL_MSG_VPN_DEL	                                HAL_MSG_VPN_MSG(2)
#define HAL_MSG_VPN_PORT_ADD                    HAL_MSG_VPN_MSG(3)
#define HAL_MSG_VPN_PORT_DEL                     HAL_MSG_VPN_MSG(4)
#define HAL_MSG_VPN_STATIC_MAC_ADD        HAL_MSG_VPN_MSG(5)
#define HAL_MSG_VPN_STATIC_MAC_DEL         HAL_MSG_VPN_MSG(6)


/*add zlh 2014 09 10*/
#define HAL_MSG_LSP_BASE                               HAL_MSG_BASE_MSG(1550)
#define HAL_MSG_LSP_MSG(n)                            (HAL_MSG_LSP_BASE  + (n))
#define HAL_MSG_LSP_ADD                                 HAL_MSG_LSP_MSG(1)
#define HAL_MSG_LSP_DEL                                  HAL_MSG_LSP_MSG(2)
#define HAL_MSG_LSP_GET_LINK_STATUS                     HAL_MSG_LSP_MSG(3)
#define HAL_MSG_LSP_PROTECT_GRP_ADD                   HAL_MSG_LSP_MSG(4)
#define HAL_MSG_LSP_PROTECT_GRP_DEL                    HAL_MSG_LSP_MSG(5)
#define HAL_MSG_LSP_PROTECT_GRP_MOD_PARAM     HAL_MSG_LSP_MSG(6)
#define HAL_MSG_LSP_PROTECT_GRP_SWITCH             HAL_MSG_LSP_MSG(7)
#define HAL_MSG_LSP_GET_PROTECT_GRP_INFO          HAL_MSG_LSP_MSG(8)     

/*add zlh 2014 09 10*/
#define HAL_MSG_OAM_BASE                                     HAL_MSG_BASE_MSG(1650)
#define HAL_MSG_OAM_MSG(n)                                 (HAL_MSG_OAM_BASE  + (n))
#define HAL_MSG_OAM_ADD                                      HAL_MSG_OAM_MSG(1)
#define HAL_MSG_OAM_DEL                                       HAL_MSG_OAM_MSG(2)
#define HAL_MSG_OAM_GET_STATUS                        HAL_MSG_OAM_MSG(3)



/*add cyh 2012 06 21*/
#define HAL_MSG_MULTICAST_BASE                      HAL_MSG_BASE_MSG(1600)
#define HAL_MSG_MULTICAST_MSG(n)                   (HAL_MSG_MULTICAST_BASE  + (n))
#define HAL_MSG_MULTICAST_GROUP_ADD          HAL_MSG_MULTICAST_MSG(1)
#define HAL_MSG_MULTICAST_GROUP_DEL           HAL_MSG_MULTICAST_MSG(2)
#define HAL_MSG_MULTICAST_GRP_PORT_ADD    HAL_MSG_MULTICAST_MSG(3)
#define HAL_MSG_MULTICAST_GRP_PORT_DEL     HAL_MSG_MULTICAST_MSG(4)


#define HAL_MSG_DEV_SRC_MAC_BASE                  HAL_MSG_BASE_MSG(1650)
#define HAL_MSG_DEV_SRC_MAC_MSG(n)              (HAL_MSG_DEV_SRC_MAC_BASE  + (n))
#define HAL_MSG_SET_DEV_SRC_MAC                   HAL_MSG_DEV_SRC_MAC_MSG(1)





struct hal_msg_vll_ingress
{
    unsigned int vllid;

    unsigned int inport;//ac port
	unsigned int vlan;//ac vlan

	unsigned int outport;//tunnel port
	unsigned int outlabel;//tunnnel label
	unsigned int tunexp;
	unsigned int pwlabel;
	unsigned int pwexp;

	unsigned int remote_pw;
	unsigned int remote_tunnel;

};

struct hal_msg_vll_egress
{
    unsigned int vllid;

    unsigned int inport;//tunnel port
	unsigned int inlabel;//tunnel label

	unsigned int outport;//ac port
	unsigned int pwlabel;
	unsigned int vlan;//ac vlan

};

struct hal_msg_vll_intermediate
{
   unsigned int tunid;

    unsigned int inport;//tunnel port
	unsigned int inlabel;//tunnel label

	unsigned int outlabel;//tunnel port
	unsigned int outport;//tunnel label
};

/*   Message structures. 
*/

struct hal_cpu_info_entry
{
   char mac_addr [6];
};

struct hal_stk_port_info
{
  int unit;
  int port;
  int flags;
  int weight;
};

struct hal_cpu_dump_entry
{
  char mac_addr [6];
  char pack_1[2]; /* Packing bytes */
  int  num_units;
  int  master_prio;
  int  slot_id;

  /* Stack port info */
  int hal_stk_ports;
  struct hal_stk_port_info stk[5]; /* Allow a maximum of 5 stack ports */
};


/*
  Index for TLVs.
*/
typedef unsigned int hal_cindex_t;
#define CINDEX_SIZE                    32
#define CHECK_CINDEX(F,C)              (CHECK_FLAG (F, (1 << C)))
#define SET_CINDEX(F,C)                (SET_FLAG (F, (1 << C)))

/*
  TLV header.
*/
struct hal_msg_tlv_header
{
  unsigned short type;
  unsigned short length;
};
#define HAL_MSG_TLV_HEADER_SIZE        4

#define HAL_DECODE_TLV_HEADER(TH)                                       \
    do {                                                                \
      TLV_DECODE_GETW ((TH).type);                                      \
      TLV_DECODE_GETW ((TH).length);                                    \
      (TH).length -= HAL_MSG_TLV_HEADER_SIZE;                           \
    } while (0)


/* 
   Interface message. 
*/
struct hal_msg_if
{
  hal_cindex_t cindex;

  /* Mandatory parameters start. */

  /* Name. */
  char name[HAL_IFNAME_LEN + 1];

  /* Ifindex. */
  unsigned int ifindex;

  /* Mandatory parameters end. */

  /* Flags TLV. */
  unsigned long flags;

  /* Metric TLV. */
  unsigned int metric;

  /* MTU TLV. */
  unsigned int mtu;

  /* DUPLEX TLV. */
  unsigned int duplex;

  /* AUTONEGO TLV, */
  unsigned int autonego;

  /* ARP AGEING TIMEOUT TLV. */
  unsigned int arp_ageing_timeout;

  /* HW Type, Address TLV. */
  unsigned short hw_type;
  unsigned short hw_addr_len;
  unsigned char hw_addr[HAL_HW_LENGTH];

  /* Number of secondary MAC addresses.*/
  unsigned char nAddrs;
  unsigned short sec_hw_addr_len;
  unsigned char **addresses;

  /* Bandwidth TLV. */
  unsigned int bandwidth;

  /* Interface type L3/L2 */
   unsigned char type;

  /* MDIX TLV */
   unsigned int mdix;

  /* Interface Properties */
#define HAL_IF_INTERNAL          (1 << 0)
  unsigned int if_flags;

};

#define HAL_MSG_IF_SIZE                                 (HAL_IFNAME_LEN + 1 + 4)

#define HAL_MSG_CINDEX_IF_FLAGS                          0
#define HAL_MSG_CINDEX_IF_METRIC                         1
#define HAL_MSG_CINDEX_IF_MTU                            2
#define HAL_MSG_CINDEX_IF_HW                             3
#define HAL_MSG_CINDEX_IF_BANDWIDTH                      4
#define HAL_MSG_CINDEX_IF_DUPLEX                         5
#define HAL_MSG_CINDEX_IF_AUTONEGO                       6
#define HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT             7
#define HAL_MSG_CINDEX_IF_SEC_HW_ADDRESSES               8
#define HAL_MSG_CINDEX_IF_IF_FLAGS                       9

#define HAL_MSG_LACP_PSC_SIZE  (8)

#ifdef HAVE_L2

struct hal_msg_bridge
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  char is_vlan_aware;
  enum hal_bridge_type type;
  char edge;
};

struct hal_msg_bridge_ageing
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  int ageing_time;
};

struct hal_msg_bridge_state
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  int enable;
};

struct hal_msg_bridge_port
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned int ifindex;
};

#ifdef HAVE_PROVIDER_BRIDGE
struct hal_msg_proto_process
{
  unsigned int ifindex;
  hal_l2_proto_process_t proto_process;
  hal_l2_proto_t proto;
  unsigned short vid;
};
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_DCB
struct hal_msg_dcb_bridge
{
   char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
};

struct hal_msg_dcb_if
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
};

struct hal_msg_dcb_pfc_pri
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  u_int8_t pfc_enable_vector[HAL_DCB_NUM_USER_PRIORITY];
};

struct hal_msg_dcb_pfc_cap
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  u_int8_t pfc_cap;
};

struct hal_msg_dcb_pfc_lda
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  u_int32_t pfc_lda;
};

struct hal_msg_dcb_pfc_mode
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  enum hal_dcb_mode mode;
};

struct hal_msg_dcb_mode
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  enum hal_dcb_mode mode;
};

struct hal_msg_dcb_ets_tcg_pri
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  u_int8_t tcgid;
  u_int8_t priority_list;
};

struct hal_msg_dcb_ets_tcg_bw
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  u_int16_t bandwidth[HAL_DCB_NUM_DEFAULT_TCGS]; 
};

struct hal_msg_dcb_ets_appl_pri
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  u_int32_t ifindex;
  u_int8_t selector;
  u_int16_t protocol_value;
  u_int8_t priority;
};

#endif /* HAVE_DCB*/

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)

struct hal_msg_bridge_pbb_service
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned int ifindex;
  unsigned int isid;  
  unsigned short svid_low ;
  unsigned short svid_high;
  unsigned short bvid; 
  unsigned char default_dst_bmac[ETHER_ADDR_LEN]; 
  unsigned char service_type; 
};

#endif
struct hal_msg_bridge_instance
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned short instance;
};

struct hal_msg_bridge_learn
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned char learn;
};

struct hal_msg_bridge_vid_instance
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned short vid;
  int instance;
};

struct hal_msg_vlan
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  enum hal_vlan_type type;
  unsigned short vid;
};


struct hal_msg_vlan_port_type
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned int ifindex;
  enum hal_vlan_port_type port_type;
  enum hal_vlan_port_type sub_port_type;
  enum hal_vlan_acceptable_frame_type acceptable_frame_type;
  unsigned short enable_ingress_filter;
};

struct hal_msg_vlan_port
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned int ifindex;
  unsigned short vid;
  enum hal_vlan_egress_type egress;
};

struct hal_msg_gxrp
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned long garp_type;
  int enable;
};
  
struct hsl_msg_enable_dot1q
{ 
  unsigned int ifindex;
  unsigned short enable;
  unsigned short enable_ingress_filter;
};

#ifdef HAVE_VLAN_CLASS
#define HAL_MSG_VLAN_CLASS_RULE_SIZE 19
struct hal_msg_vlan_classifier_rule
{
  int type;
  unsigned short vlan_id;
  unsigned int   ifindex;                /* interface index.      */
  unsigned int   refcount;               /* Rule reference count. */

  union
  {
    unsigned char hw_addr[ETHER_ADDR_LEN];

    struct
    {
      unsigned short ether_type;         /* protocol value        */
      unsigned int   encaps;             /* packet encapsulation. */
    } protocol;

    struct 
    {
      unsigned int addr;
      unsigned char masklen;
    }ipv4;
  } u;
};

int hal_msg_encode_vlan_classifier_rule(u_char **pnt, u_int32_t *size, struct hal_msg_vlan_classifier_rule *msg);
#endif /* HAVE_VLAN_CLASS */

#if defined HAVE_VLAN_STACK || defined HAVE_PROVIDER_BRIDGE

struct hal_msg_vlan_stack
{
  u_int32_t ifindex;
  u_int16_t ethtype;
  u_int16_t stackmode;
};

#endif /* HAVE_VLAN_STACK  || HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_PROVIDER_BRIDGE

struct hal_msg_cvlan_reg_tab
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned int ifindex;
  unsigned short cvid;
  unsigned short svid;
};

#endif /* HAVE_PROVIDER_BRIDGE */

struct hal_msg_flow_control
{
  unsigned int ifindex;
  unsigned char direction;
};

struct hal_msg_flow_control_stats
{
  unsigned int ifindex;
  unsigned char direction;
  int rxpause;
  int txpause;
};

struct hal_msg_ratelimit
{
  unsigned int ifindex;
  unsigned char level;
  unsigned char fraction;
};


struct hal_msg_stp_port_state
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned short instance;
  unsigned int port_ifindex;
  unsigned int port_state;
};

struct hal_msg_auth_port_state
{
  unsigned int port_ifindex;
  unsigned int port_state;
  unsigned int port_mode;
};

#ifdef HAVE_MAC_AUTH
struct hal_msg_auth_mac_port_state
{
  unsigned int port_ifindex;
  unsigned int port_state;
  unsigned int port_mode;
};

struct hal_msg_auth_mac_unset_irq
{
  unsigned int port_ifindex;
};

#endif

struct hal_msg_l2_fdb_entry
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned char mac[HAL_HW_LENGTH];
  unsigned int ifindex;
  unsigned char flags;
  unsigned short vid;
  unsigned char is_forward;
};

enum hal_bridge_pri_ovr_mac_type
{ 
  HAL_BRIDGE_MAC_PRI_OVR_NONE,
  HAL_BRIDGE_MAC_STATIC,
  HAL_BRIDGE_MAC_STATIC_PRI_OVR,
  HAL_BRIDGE_MAC_STATIC_MGMT,
  HAL_BRIDGE_MAC_STATIC_MGMT_PRI_OVR,
  HAL_BRIDGE_MAC_PRI_OVR_MAX,
};

struct hal_msg_l2_fdb_prio_override
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned char mac[HAL_HW_LENGTH];
  unsigned int ifindex;
  unsigned short vid;
  enum hal_bridge_pri_ovr_mac_type ovr_mac_type;
  unsigned char priority;
};

struct hal_msg_l2_fdb_flush
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned char mac[HAL_HW_LENGTH];
  unsigned short vid;
  unsigned int ifindex;
  unsigned int instance;  
};

#define HAL_MSG_L2_FDB_ENTRY_REQ_SIZE                 27  /* 32 if not packed */

enum hal_msg_l2_fdb_req_type
{
  HAL_MSG_L2_FDB_REQ_GET = 1,
  HAL_MSG_L2_FDB_REQ_WALK,
  HAL_MSG_L2_FDB_REQ_MAC,
};

struct hal_msg_l2_fdb_entry_req
{
  u_int32_t count;
  enum hal_msg_l2_fdb_req_type req_type;
  struct hal_fdb_entry start_fdb_entry;
};

#define HAL_MSG_L2_FDB_ENTRY_RESP_SIZE                 4 
struct hal_msg_l2_fdb_entry_resp
{
  u_int32_t count;
  struct hal_fdb_entry *fdb_entry;
};

#define HAL_MSG_EFM_ERR_FRAME_REQ_SIZE 4
#define HAL_MSG_EFM_ERR_FRAME_RESP_SIZE 4

#ifdef HAVE_PVLAN
struct hal_msg_pvlan
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned short vid;
  enum hal_pvlan_type vlan_type;
};

struct hal_msg_pvlan_association
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned short pvid;
  unsigned short svid;
};

struct hal_msg_pvlan_port_mode
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned int ifindex;
  enum hal_pvlan_port_mode port_mode;
};

struct hal_msg_pvlan_port_set
{
  char name[HAL_BRIDGE_NAME_LEN + 1];
  unsigned int ifindex;
  unsigned short pvid;
  unsigned short svid;
};

#endif /* HAVE_PVLAN */

int hal_msg_encode_l2_fdb_resp (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_resp *msg);
int hal_msg_decode_l2_fdb_resp (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_resp *msg);
int hal_msg_encode_l2_fdb_req (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_req *msg);
int hal_msg_decode_l2_fdb_req (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_req *msg);


struct hal_msg_igs_bridge
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
};
#define HAL_MSG_BRIDGE_IGS_SIZE  (HAL_BRIDGE_NAME_LEN)
int hal_msg_encode_igs_bridge(u_char **pnt, u_int32_t *size, struct hal_msg_igs_bridge *msg);
int hal_msg_decode_igs_bridge(u_char **pnt, u_int32_t *size, struct hal_msg_igs_bridge *msg);

struct hal_msg_igmp_snoop_entry
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  struct hal_in4_addr src;
  struct hal_in4_addr group;
  char is_exclude;
  unsigned short vid;
  int count;
  u_int32_t *ifindexes;
};

#define HAL_MSG_IGMP_SNOOP_ENTRY_SIZE         14 
#define HAL_MSG_CINDEX_IGMP_SNOOP_IFINDEXES   0
int hal_msg_encode_igs_entry (u_char **pnt, u_int32_t *size, struct hal_msg_igmp_snoop_entry *msg);
int hal_msg_decode_igs_entry (u_char **pnt, u_int32_t *size, struct hal_msg_igmp_snoop_entry *msg, void *(*mem_alloc)(int size,int mtype));

#ifdef HAVE_MLD_SNOOP
struct hal_msg_mlds_bridge
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
};
#define HAL_MSG_BRIDGE_MLDS_SIZE  (HAL_BRIDGE_NAME_LEN)
int hal_msg_encode_mlds_bridge(u_char **pnt, u_int32_t *size,
                               struct hal_msg_mlds_bridge *msg);
int hal_msg_decode_mlds_bridge(u_char **pnt, u_int32_t *size,
                               struct hal_msg_mlds_bridge *msg);

struct hal_msg_mld_snoop_entry
{
  char bridge_name[HAL_BRIDGE_NAME_LEN + 1];
  struct hal_in6_addr src;
  struct hal_in6_addr group;
  char is_exclude;
  unsigned short vid;
  int count;
  u_int32_t *ifindexes;
};

#define HAL_MSG_MLD_SNOOP_ENTRY_SIZE          38
#define HAL_MSG_CINDEX_MLD_SNOOP_IFINDEXES     0
int hal_msg_encode_mlds_entry (u_char **pnt, u_int32_t *size, 
                               struct hal_msg_mld_snoop_entry *msg);
int hal_msg_decode_mlds_entry (u_char **pnt, u_int32_t *size, 
                               struct hal_msg_mld_snoop_entry *msg,
                               void *(*mem_alloc)(int size,int mtype));

#endif /* HAVE_MLD_SNOOP */

#endif /* HAVE_L2 */

struct hal_msg_port_mirror
{
  unsigned int to_ifindex;
  unsigned int from_ifindex;
  enum hal_port_mirror_direction direction;
};

struct hal_msg_if_port_type
{
  char name[HAL_IFNAME_LEN + 1];
  unsigned int ifindex;
  unsigned char type;
#define HAL_MSG_SET_SWITCHPORT          1
#define HAL_MSG_SET_ROUTERPORT          2
};

struct hal_msg_svi
{
  char name[HAL_IFNAME_LEN + 1];
  unsigned int ifindex;
};

#ifdef HAVE_TUNNEL
struct hal_msg_tunnel_if
{
  u_char config;
#define TUNNEL_CONFIG_CHECKSUM                  (1 << 0)
#define TUNNEL_CONFIG_PMTUD                     (1 << 1)

  u_char mode;
#define TUNNEL_MODE_NONE                        0
#define TUNNEL_MODE_IPIP                        1
#define TUNNEL_MODE_GREIP                       2
#define TUNNEL_MODE_IPV6IP                      3
#define TUNNEL_MODE_IPV6IP_6TO4                 4
#define TUNNEL_MODE_IPV6IP_ISATAP               5
#define TUNNEL_MODE_MAX                         6

  /* IPv4 tunnel information. */
  u_char ttl;
  u_char tos;
  struct hal_in4_addr saddr4;
  struct hal_in4_addr daddr4;
  /* Note: dmac address should always be at the last of tunnel_if structure.*/
  unsigned char dmac_addr [ETHER_ADDR_LEN];
};

struct hal_msg_tunnel_add_if
{
  char name[HAL_IFNAME_LEN +1];
  unsigned int ifindex;
  int mtu;
  int speed;
  u_int32_t flags;
  unsigned char hwaddr[ETHER_ADDR_LEN];
  struct hal_msg_tunnel_if tif;
  u_int32_t duplex;
};

#endif

#ifdef HAVE_L3
struct hal_msg_l3_add_if
{
  char name[HAL_IFNAME_LEN + 1];
  unsigned char hwaddr[ETHER_ADDR_LEN];
  int hwaddrlen;
  int num;
  unsigned int *ifindex;
};

struct hal_msg_l3_delete_if
{
  char name[HAL_IFNAME_LEN + 1];
  unsigned int ifindex;
};

#define HAL_MSG_IPV4_ADDR_SIZE                         30
struct hal_msg_if_ipv4_addr
{
  char name[HAL_IFNAME_LEN + 1];
  unsigned int ifindex;
  struct hal_in4_addr addr;
  unsigned char ipmask;
};

struct hal_msg_if_fib_bind_unbind
{
  unsigned int ifindex;
  unsigned int fib_id;
};
#define HAL_MSG_IF_FIB_BIND_UNBIND_SIZE     8

/* Maximum nexthops. */
#define HAL_IPV4UC_MAX_NEXTHOPS        8

struct hal_msg_ipv4uc_route
{
  unsigned short fib_id;
  struct hal_in4_addr addr;
  u_char masklen;
  int num;
  struct hal_ipv4uc_nexthop nh[HAL_IPV4UC_MAX_NEXTHOPS];
};

#define HAL_MSG_CINDEX_NEXTHOP                          0

#define HAL_MSG_IPV4_ROUTE_SIZE                         7
#define HAL_MSG_IPV4_NEXTHOP_SIZE                       13

struct hal_msg_ipv4uc_route_resp
{
  struct hal_in4_addr addr;
  u_char masklen;
  int num;
  struct 
  {
    unsigned int id;
    int status;
  } resp[HAL_IPV4UC_MAX_NEXTHOPS];
};

#define HAL_MSG_CINDEX_NEXTHOP_RESP                     0

#define HAL_MSG_IPV4_ROUTE_RESP_SIZE                    5
#define HAL_MSG_IPV4_NEXTHOP_RESP_SIZE                  8

#ifdef HAVE_IPV6
#define HAL_MSG_IPV6_ADDR_SIZE                          43
struct hal_msg_if_ipv6_addr
{
  char name[HAL_IFNAME_LEN + 1];
  unsigned int ifindex;
  struct hal_in6_addr addr;
  unsigned char ipmask;
  unsigned char flags;
};


/* Maximum IPV6 nexthops. */
#define HAL_IPV6UC_MAX_NEXTHOPS        8
                                                                                                                             
struct hal_msg_ipv6uc_route
{
  unsigned short fib_id;
  struct hal_in6_addr addr;
  u_char masklen;
  int num;
  struct hal_ipv6uc_nexthop nh[HAL_IPV6UC_MAX_NEXTHOPS];
};
                                                                                                                             
#define HAL_MSG_CINDEX_NEXTHOP                          0
                                                                                                                             
#define HAL_MSG_IPV6_ROUTE_SIZE                         19
#define HAL_MSG_IPV6_NEXTHOP_SIZE                       25
#endif /* HAVE_IPV6 */

#ifdef HAVE_MCAST_IPV4

#define HAL_MAX_VIFS                                    32

struct hal_msg_ipv4mc_vif_add
{
  u_int32_t vif_index;
  u_int32_t ifindex;
  struct hal_in4_addr loc_addr;
  struct hal_in4_addr rmt_addr;
  u_int16_t flags;
#define HAL_VIFF_TUNNEL     0x1
#define HAL_VIFF_REGISTER   0x4
};
#define HAL_MSG_IPV4_VIF_ADD_SIZE   18

struct hal_msg_ipv4mc_mrt_add
{
  struct hal_in4_addr src;
  struct hal_in4_addr group;
  u_int32_t iif_vif;
  u_int32_t num_olist; 
  u_char *olist_ttls;
};
#define HAL_MSG_IPV4_MRT_ADD_SIZE   16

struct hal_msg_ipv4mc_mrt_del
{
  struct hal_in4_addr src;
  struct hal_in4_addr group;
};
#define HAL_MSG_IPV4_MRT_DEL_SIZE   8

struct hal_msg_ipv4mc_sg_stat
{
  struct hal_in4_addr src;
  struct hal_in4_addr group;
  u_int32_t iif_vif;
  u_int32_t pktcnt;
  u_int32_t bytecnt;
  u_int32_t wrong_if;
};

#define HAL_MSG_IPV4_SG_STAT_SIZE   24

#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6

#define HAL_MAX_IPV6_VIFS                                    64

struct hal_msg_ipv6mc_vif_add
{
  u_int32_t vif_index;
  u_int32_t phy_ifindex;
  u_int16_t flags;
#define HAL_IPV6_VIFF_REGISTER   0x1
};
#define HAL_MSG_IPV6_VIF_ADD_SIZE   10

struct hal_msg_ipv6mc_mrt_add
{
  struct hal_in6_addr src;
  struct hal_in6_addr group;
  u_int32_t iif_vif;
  u_int32_t num_olist;
  u_int16_t *olist;
};
#define HAL_MSG_IPV6_MRT_ADD_SIZE   40

struct hal_msg_ipv6mc_mrt_del
{
  struct hal_in6_addr src;
  struct hal_in6_addr group;
};
#define HAL_MSG_IPV6_MRT_DEL_SIZE   32

struct hal_msg_ipv6mc_sg_stat
{
  struct hal_in6_addr src;
  struct hal_in6_addr group;
  u_int32_t iif_vif;
  u_int32_t pktcnt;
  u_int32_t bytecnt;
  u_int32_t wrong_if;
};
#define HAL_MSG_IPV6_SG_STAT_SIZE   48

#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_PBR
struct hal_msg_pbr_ipv4_route
{
  char rmap_name[HAL_RMAP_NAME_LEN + 1];   
  s_int32_t seq_num;
 
  struct hal_prefix src_prefix; /* source prefix */
  struct hal_prefix dst_prefix; /* destination prefix */
  s_int16_t protocol;       /* protocol type */

  /* source layer4 port */
  s_int16_t sport_op;
  u_int32_t sport_low;
  u_int32_t sport;

  /* destination layer4 port */
  s_int16_t dport_op;
  u_int32_t dport_low;
  u_int32_t dport;

  /* TOS */
  s_int16_t tos_op;
  u_int8_t tos_low;
  u_int8_t tos;

  int rule_type ;      /* permit or deny */
  u_int32_t if_id; /* interface id on which policy being installed */
  s_int32_t pcp; /* precedence */
  struct hal_in4_addr nexthop; /* nexthop address */
  s_int16_t nh_type; /* neighbor type */
  char ifname[HAL_IFNAME_LEN + 1];
  u_int32_t cindex; /* Ctype index */
};

#define HAL_MSG_CINDEX_PBR_PROTOCOL      0
#define HAL_MSG_CINDEX_PBR_SRC_PORT      1
#define HAL_MSG_CINDEX_PBR_DST_PORT      2
#define HAL_MSG_CINDEX_PBR_TOS           3
#define HAL_MSG_CINDEX_PBR_PCP           6
#endif /* HAVE_PBR */

#endif /* HAVE_L3 */

#ifdef HAVE_LACPD
struct hal_msg_lacp_psc_set
{
  unsigned int ifindex;
  int psc;
};
#define HAL_MSG_LACP_PSC_SIZE  (8)
int hal_msg_encode_lacp_psc_set (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_psc_set *msg);
int hal_msg_decode_lacp_psc_set (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_psc_set *msg);

struct hal_msg_lacp_agg_identifier
{
  char agg_name[HAL_IFNAME_LEN + 1];
  unsigned int agg_ifindex;
};
#define HAL_MSG_LACP_AGG_ID_SIZE  (25)
int hal_msg_encode_lacp_id (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_identifier *msg);
int hal_msg_decode_lacp_id (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_identifier *msg);

struct hal_msg_lacp_agg_add
{
  char agg_name[HAL_IFNAME_LEN + 1];
  unsigned char agg_mac[ETHER_ADDR_LEN];
  int agg_type;
};
#define HAL_MSG_LACP_AGG_ADD_SIZE  (31)
int hal_msg_encode_lacp_agg_add (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_add *msg);
int hal_msg_decode_lacp_agg_add (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_add *msg);

struct hal_msg_lacp_mux
{
  char agg_name[HAL_IFNAME_LEN + 1];
  unsigned int agg_ifindex;
  char port_name[HAL_IFNAME_LEN + 1];
  unsigned int port_ifindex;
};
#define HAL_MSG_LACP_MUX_ADD_SIZE  (50)
int hal_msg_encode_lacp_mux(u_char **pnt, u_int32_t *size, struct hal_msg_lacp_mux *msg);
int hal_msg_decode_lacp_mux(u_char **pnt, u_int32_t *size, struct hal_msg_lacp_mux *msg);
#endif /* HAVE_LACPD */

struct hal_msg_l2_qos_regen_usr_pri
{
  int ifindex;
  unsigned char recvd_user_priority;
  unsigned char regen_user_priority;
};

struct hal_msg_l2_qos_traffic_class
{
  int ifindex;
  unsigned char user_priority;
  unsigned char traffic_class_value;
};

#ifdef HAVE_QOS
struct hal_msg_qos_enable
{
  char q0[2];
  char q1[2];
  char q2[2];
  char q3[2];
  char q4[2];
  char q5[2];
  char q6[2];
  char q7[2];
};

struct hal_msg_qos_disable
{
  int num_queue;
};

struct hal_msg_qos_wrr_queue_limit
{
  int ifindex;
  int ratio[8];
};

struct hal_msg_qos_wrr_tail_drop_threshold
{
  int ifindex;
  int qid;
  int thres[2];
};

struct hal_msg_qos_wrr_threshold_dscp_map
{
  u_int32_t ifindex;
  int thid;     /* threshold id */
  int num;      /* num of dscp */
  u_char dscp[8];  /* max. dscp */
};

struct hal_msg_qos_wrr_wred_drop_threshold
{
  int ifindex;
  int qid;
  int thres[2];
};

struct hal_msg_qos_wrr_set_bandwidth
{
  int ifindex;
  int bw[8];
};

struct hal_msg_qos_wrr_queue_cos_map_set
{
  u_int32_t ifindex;
  int qid;
  int cos_count;
  int cos[8];
};

/* zlh@150119*/
typedef struct hal_wrr_car_info_s
{
	int  sev_index;  //acid / pwid/ tunId
	int  enable;     //endable /disable
	unsigned long int  portId;
	int qid;//0-7
	int cir;
	int pir;
} hal_wrr_car_info_t;

/* zlh@150119*/
typedef struct hal_wrr_wred_info_s
{
	int sev_index;  //acid / pwid/ tunId
	int	enable;     //endable /disable

	unsigned long int  portId;
	int qid;//0-7
	int startpoint; //0-100
	int slope;  //0-90
	int color;  //yellow /red /green /nontcp
	int time;   //0-255
} hal_wrr_wred_info_t;

/* zlh@150119*/
typedef struct hal_wrr_weight_info_s
{
	int sev_index; //acid / pwid/ tunId
	int	enable;    //endable /disable
	unsigned long int  portId;
	int weight[8];
} hal_wrr_weight_info_t;


struct hal_msg_qos_wrr_queue_cos_map_unset
{
  int ifindex;
  int cos_count;
  int cos[8];
};

struct hal_msg_qos_wrr_queue_min_reserve
{
  int ifindex;
  int qid;
  int packets;
};

struct hal_msg_qos_set_trust_state
{
  int ifindex;
  enum hal_qos_trust_state  trust_state;
};

struct hal_msg_qos_set_default_cos
{
  int ifindex;
  int cos_value;
};


struct hal_msg_dscp_map_table 
{
  int in_dscp;
  int out_dscp;
  int out_pri;
};


struct hal_msg_qos_set_dscp_map_tbl
{
  int ifindex;
  int flag;
  struct hal_msg_dscp_map_table map_table[HAL_DSCP_TBL_SIZE];
  int map_count;
};

struct hal_msg_qos_set_class_map
{
  int ifindex;
  int action;           /* Attach or detach */
  int flag;             /* Indicate the starting or ending point of policy-map when attach */
  int dir;              /* Incoming or outgoing */
  struct hal_class_map cmap;
};

struct hal_msg_qos_set_policy_map
{
  int ifindex;
  int action;           /* Attach or detach */
  int start_end;        /* Indicate the start(1)/end(2) of policy-map when attach */
  int dir;              /* Incoming or outgoing */
  struct hal_policy_map pmap;
};
#endif /* HAVE_QOS */

#ifdef HAVE_L2LERN
struct hal_msg_mac_set_access_grp
{
  int ifindex;
  int action;           /* Attach or detach */
  int dir;              /* Incoming or outgoing */
  struct hal_mac_access_grp hal_macc_grp;
};

#ifdef HAVE_VLAN
struct hal_msg_vlan_set_access_map
{
  int vid;
  int action;           /* Attach or detach */
  struct hal_mac_access_grp hal_macc_grp;
};
#endif /* HAVE_VLAN */

#endif /* HAVE_L2LERN */
struct hal_msg_ip_set_access_grp
{
  char *ifname;
  int action;           /* Attach or detach */
  int dir;              /* Incoming or outgoing */
  struct hal_ip_access_grp hal_ip_grp;
};

struct hal_msg_ip_set_access_grp_interface
{
  char *vifname;
  char *ifname;
  int action;           /* Attach or detach */
  int dir;              /* Incoming or outgoing */
  struct hal_ip_access_grp hal_ip_grp;
};

#ifdef HAVE_QOS

struct hal_msg_qos_set_cmap_match_traffic
{
  char *cmap_name;
  u_int16_t traffic_type;
  enum hal_qos_traffic_match match_mode;
};

struct hal_msg_qos_unset_cmap_match_traffic
{
  char *cmap_name;
  u_int16_t traffic_type;
};


struct hal_msg_qos_set_pmapc_police_delete
{
  char *cmap_name;
  char *pmap_name;
};


struct hal_msg_qos_set_port_service_policy
{
  char *ifname;
  u_int32_t ifindex;
  char *pmap_name;
  enum hal_qos_service_policy_direction dir;
};

struct hal_msg_qos_set_cos_to_queue
{
  u_int8_t cos;
  u_int8_t queue_id;
};

struct hal_msg_qos_set_dscp_to_queue
{
  u_int8_t dscp;
  u_int8_t queue_id;
};


struct hal_msg_qos_set_force_trust_cos
{
  u_int32_t ifindex;
  u_int8_t force_trust_cos;
};
  
struct hal_msg_qos_set_egress_rate
{
  u_int32_t ifindex;
  u_int32_t egress_rate;
  enum hal_qos_rate_unit rate_unit;
};
  
struct hal_msg_qos_set_strict_queue
{
  u_int32_t ifindex;
  u_int8_t strict_queue;
};
  
struct hal_msg_qos_set_frame_type_priority_override
{
  enum hal_qos_frame_type frame_type;
  u_int8_t queue_id;
};

struct hal_msg_qos_set_vlan_priority
{
  u_int16_t vid;
  u_int8_t priority;
};

struct hal_msg_qos_unset_vlan_priority
{
  u_int16_t vid;
};

struct hal_msg_qos_set_port_vlan_priority_override
{
  int  ifindex;
  enum hal_qos_vlan_pri_override port_mode;
};

struct hal_msg_qos_set_port_da_priority_override
{ 
  int  ifindex;
  enum hal_qos_da_pri_override port_mode;
};

struct hal_msg_qos_set_queue_weight
{ 
  int weight [HAL_QOS_MAX_QUEUE_SIZE];
};

#endif /* HAVE_QOS */

/*
   Function prototypes.
*/
int hal_msg_encode_tlv (u_char **pnt, u_int32_t *size, u_int16_t type, u_int16_t length);
int hal_msg_encode_if (u_char **pnt, u_int32_t *size, struct hal_msg_if *msg);
int hal_msg_decode_if (u_char **pnt, u_int32_t *size, struct hal_msg_if *msg);

#ifdef HAVE_MCAST_IPV4
int hal_msg_encode_ipv4_vif_add (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_vif_add *);
int hal_msg_decode_ipv4_vif_add (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_vif_add *);

int hal_msg_encode_ipv4_mrt_add (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_mrt_add *);
int hal_msg_decode_ipv4_mrt_add (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_mrt_add *);

int hal_msg_encode_ipv4_mrt_del (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_mrt_del *);
int hal_msg_decode_ipv4_mrt_del (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_mrt_del *);

int hal_msg_encode_ipv4_sg_stat (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_sg_stat *);
int hal_msg_decode_ipv4_sg_stat (u_char **, u_int32_t *,
    struct hal_msg_ipv4mc_sg_stat *);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
int hal_msg_encode_ipv6_vif_add (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_vif_add *);
int hal_msg_decode_ipv6_vif_add (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_vif_add *);

int hal_msg_encode_ipv6_mrt_add (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_mrt_add *);
int hal_msg_decode_ipv6_mrt_add (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_mrt_add *);

int hal_msg_encode_ipv6_mrt_del (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_mrt_del *);
int hal_msg_decode_ipv6_mrt_del (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_mrt_del *);

int hal_msg_encode_ipv6_sg_stat (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_sg_stat *);
int hal_msg_decode_ipv6_sg_stat (u_char **, u_int32_t *,
    struct hal_msg_ipv6mc_sg_stat *);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_MPLS

struct hal_msg_mpls_control
{
  /* The protocol (LDP or BGP etc.) that sent this request */
  u_char protocol;
};

struct hal_msg_mpls_if
{
  /* Interface index */
  u_int32_t ifindex;

  /* Interface name */
  char ifname[HAL_IFNAME_LEN + 1];
  
  /* Label space for this interface */
  u_int32_t label_space;
  
  /* VRF table identifier */
  int vrf_ident;

  /* VC identifier. */
  u_int32_t vc_id;
};

/* Prefix structure for IPv4 FEC. */
struct hal_msg_mpls_prefix
{
  u_char family;
  u_char prefixlen;
  union
  {
    u_char prefix;
    struct hal_in4_addr prefix4;
  } u;
};

/* Key used by RSVP-TE protocol for IPV4 */
struct hal_msg_rsvp_key_ipv4_fwd
{
  u_int16_t trunk_id;
  u_int16_t lsp_id;
  struct hal_in4_addr ingr;
  struct hal_in4_addr egr;
};

/* Generic RSVP_TE Key for MPLS forwarder */
struct hal_msg_rsvp_key_fwd
{
  u_char afi;
  u_int16_t len;
  union
  {
    u_char key;
    struct hal_msg_rsvp_key_ipv4_fwd ipv4;
  } u;
};

struct hal_msg_mpls_owner_fwd
{
  /* APN_PROTO_xxx */
  u_char protocol;
  union
  {
    struct hal_msg_rsvp_key_fwd r_key;
  } u;
};


#define HAL_MSG_MPLS_FTN_ADD_LEN  (60 + 2 * HAL_IFNAME_LEN)

struct hal_msg_mpls_ftn_add
{
  /* address family, AF_INET. */
  u_char family;

  /* VRF table identifier */
  u_int32_t vrf;

  /* FEC for which this FTN was created */
  u_int32_t fec_addr;
  u_char fec_prefixlen;

  /* DSCP codepoint of incoming IP packet */
  u_char dscp_in;

  /* Tunnel Label */
  u_int32_t tunnel_label;

  /* IP address of tunnel lsp nexthop */
  u_int32_t tunnel_nhop;
      
  /* Outgoing interface for tunnel lsp */
  u_int32_t tunnel_oif_ix;
  char tunnel_oif_name[HAL_IFNAME_LEN + 1];
  
  /* OPCODE to be used */
  char opcode;

  /* NHLFE index */
  u_int32_t nhlfe_ix;

  /* FTN index */
  u_int32_t ftn_ix;

  /* FTN type */
  u_char ftn_type;

  /* Tunnel Identifier (trunk id) */
  u_int32_t tunnel_id;

  /* QoS Identifier */
  u_int32_t qos_resource_id;

  /*Bypass FTN index */
  u_int32_t bypass_ftn_ix;

  /* LSP type */
  u_char lsp_type;

  /* Inner label (vpn label) */
  u_int32_t vpn_label;

  /* IP address of remote vpn peer (bgp peer for L3 vpns) */
  u_int32_t vpn_nhop;

  /* Outgoing interface to reach vpn peer */
  u_int32_t vpn_oif_ix;
  char vpn_oif_name[HAL_IFNAME_LEN + 1];

#ifdef HAVE_DIFFSERV
  /* Diffserv related data */
  struct hal_mpls_diffserv tunnel_ds_info;
#endif /* HAVE_DIFFSERV */
};

struct hal_msg_mpls_ftn_del
{
  u_char family;

  /* VRF table identifier */
  int vrf;

  u_char dscp_in;

  /* FEC for which this FTN was created */
  u_int32_t fec_addr;
  u_char fec_prefixlen;

  /* IP address of tunnel lsp nexthop */
  u_int32_t tunnel_nhop;

  /* NHLFE index */
  u_int32_t nhlfe_ix;
  
  u_int32_t tunnel_id;

  u_int32_t ftn_ix;
};

struct hal_msg_mpls_ilm_add
{
  /* address family, AF_INET. */
  u_char family;
  
  /* Incoming label */
  int in_label;
  
  /* Incoming interface index */
  u_int32_t in_ifindex;
  
  /* Name of the incoming interface, eg "eth0" */
  char in_ifname[HAL_IFNAME_LEN + 1];   

  /* OPCODE to be used */
  char opcode;

  /* Nexthop for this ILM */
  u_int32_t nexthop;

  /* Outgoing interface id. Use outintf/outlabel as per convenience. */
  u_int32_t out_ifindex;
  
  /* Name of the outgoing interface, eg "eth0" */
  char out_ifname[HAL_IFNAME_LEN + 1];

  /* Swap Label */
  u_int32_t swap_label;

  /* NHLFE index */
  u_int32_t nhlfe_ix;

  /* Tunnel label */
  u_int32_t tunnel_label;

  /* VPLS/VC/VRF ID */
  u_int32_t vpn_id;
  
  /* VC Peer address */
  u_int32_t vc_peer;

  /* FEC for which this ILM was created */
  u_int32_t fec_addr;
  u_char fec_prefixlen;

#ifdef HAVE_DIFFSERV
  /* Diffserv related data */
  struct hal_mpls_diffserv ds_info;
#endif /* HAVE_DIFFSERV */
};

struct hal_msg_mpls_ilm_del
{
  u_char family;

  /* Incoming label */
  int in_label;
  
  /* Incoming interface index */
  u_int32_t in_ifindex;
};

struct hal_msg_mpls_ttl
{
  /* The protocol (LDP or BGP etc.) that sent this request */
  u_char protocol;

  u_char type;

  /* Ingress or not */
  u_char is_ingress;

  /* New ttl value */
  int new_ttl;
};

struct hal_msg_mpls_conf_handle
{
  /* The protocol (LDP or BGP etc.) that sent this request */
  u_char protocol;

  /* Enable attribute */
  int enable;
};

#ifdef HAVE_MPLS_VC
struct hal_msg_mpls_vpn_if 
{
  u_int32_t vpn_id;
  u_int32_t ifindex;
  u_int16_t vlan_id;
};


struct hal_msg_mpls_vc_fib_add
{
  u_int32_t vc_id;

  u_int32_t vc_style;

  u_int32_t vpls_id;

  u_int32_t in_label;

  /* Outgoing label. */
  u_int32_t out_label;

  u_int32_t ac_ifindex;

  u_int32_t nw_ifindex;

  /* OPCODE to be used */
  char opcode;

  u_int32_t vc_peer;

  /* Next hop. */
  u_int32_t vc_nhop;

    /* Tunnel Label */
  u_int32_t tunnel_label;

  /* IP address of tunnel lsp nexthop */
  u_int32_t tunnel_nhop;

  /* Outgoing interface for tunnel lsp */
  u_int32_t tunnel_oif_ix;

  /* NHLFE index */
  u_int32_t tunnel_nhlfe_ix;
};


struct hal_msg_mpls_vc_fib_del
{
  u_int32_t vc_id;
  u_char vc_style;
  u_int32_t vpls_id;
  u_int32_t in_label;
  u_int32_t nw_ifindex;
  u_int32_t vc_peer;
};
#endif /* HAVE_MPLS_VC */

#endif /* HAVE_MPLS */

#ifdef HAVE_L3
struct hal_msg_arp_update 
{
  struct hal_in4_addr ip_addr;
  unsigned char mac_addr[ETHER_ADDR_LEN];
  unsigned int  ifindex;
  u_int8_t is_proxy_arp;
};

struct hal_msg_arp_del_all 
{
  unsigned short fib_id;
  u_int8_t clr_flag;
};

#define HAL_MSG_ARP_CACHE_REQ_SIZE   10 
struct hal_msg_arp_cache_req
{
  unsigned short fib_id;
  struct hal_in4_addr ip_addr;
  u_int32_t count;
};

#define HAL_MSG_ARP_CACHE_RESP_SIZE  4
struct hal_msg_arp_cache_resp
{
  u_int32_t count;
  struct hal_arp_cache_entry *cache;
};

struct hal_msg_if_l3_status
{
  int status;
};

#ifdef HAVE_IPV6
struct hal_msg_ipv6_nbr_update 
{
  struct hal_in6_addr addr;
  unsigned char mac_addr[ETHER_ADDR_LEN];
  unsigned int  ifindex;
};

struct hal_msg_ipv6_nbr_del_all 
{
  unsigned short fib_id;
  u_int8_t clr_flag;
};

#define HAL_MSG_IPV6_NBR_CACHE_REQ_SIZE   22 
struct hal_msg_ipv6_nbr_cache_req
{
  unsigned short fib_id;
  struct hal_in6_addr addr;
  unsigned int count;
};


#define HAL_MSG_IPV6_NBR_CACHE_RESP_SIZE  4
struct hal_msg_ipv6_nbr_cache_resp
{
  unsigned int count;
  struct hal_ipv6_nbr_cache_entry *cache;
};
#endif /* HAVE_IPV6 */

int hal_msg_decode_if_fib_bind_unbind (u_char **pnt, u_int32_t *size, struct hal_msg_if_fib_bind_unbind *msg);
int hal_msg_encode_if_fib_bind_unbind (u_char **pnt, u_int32_t *size, struct hal_msg_if_fib_bind_unbind *msg);
int hal_msg_encode_arp_cache_resp (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_resp *msg);
int hal_msg_decode_arp_cache_resp (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_resp *msg);
int hal_msg_encode_arp_cache_req (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_req *msg);
int hal_msg_decode_arp_cache_req (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_req *msg);

int hal_msg_encode_ipv4_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv4_addr *msg);
int hal_msg_decode_ipv4_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv4_addr *msg);
int hal_msg_encode_ipv4_route (u_char **pnt, u_int32_t *size, struct hal_msg_ipv4uc_route *msg);
#ifdef HAVE_IPV6
int hal_msg_encode_ipv6_nbr_cache_resp (u_char **pnt, u_int32_t *size, struct hal_msg_ipv6_nbr_cache_resp *msg);
int hal_msg_decode_ipv6_nbr_cache_resp (u_char **pnt, u_int32_t *size, struct hal_msg_ipv6_nbr_cache_resp *msg);
int hal_msg_encode_ipv6_nbr_cache_req (u_char **pnt, u_int32_t *size, struct hal_msg_ipv6_nbr_cache_req *msg);
int hal_msg_decode_ipv6_nbr_cache_req (u_char **pnt, u_int32_t *size, struct hal_msg_ipv6_nbr_cache_req *msg);

int hal_msg_encode_ipv6_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv6_addr *msg);
int hal_msg_decode_ipv6_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv6_addr *msg);
int hal_msg_encode_ipv6_route (u_char **pnt, u_int32_t *size, struct hal_msg_ipv6uc_route *msg);
#endif /* HAVE_IPV6 */

#ifdef HAVE_VRRP
int hal_vrrp_virtual_addr_update (u_int8_t, u_int8_t,
                                  void *, s_int32_t, char);
#endif /* HAVE_VRRP */

#endif /* HAVE_L3. */

#ifdef HAVE_ONMD
#define HAL_MSG_EFM_PORT_STATE_SIZE 12
struct hal_msg_efm_port_state
{
  unsigned int index;
  enum hal_efm_par_action local_par_action;
  enum hal_efm_mux_action local_mux_action;
};

struct hal_msg_oam_hw_addr
{
  u_int8_t dest_addr [HAL_HW_LENGTH];
  enum hal_l2_proto proto;
};

struct hal_msg_oam_ether_type
{
  u_int16_t ether_type;
  enum hal_l2_proto proto;
};

struct hal_msg_cfm_trap_level_pdu
{
  u_int8_t level;
  enum hal_cfm_pdu_type pdu;
};

struct hal_msg_efm_set_symbol_period_window
{
  unsigned int index;
  ut_int64_t symbol_period_window;  
};

struct hal_msg_efm_set_frame_period_window
{
  unsigned int index;
  u_int32_t frame_period_window;
};

struct hal_msg_efm_err_frames_resp
{
  u_int32_t index;
  ut_int64_t* err_frames;
};

struct hal_msg_efm_err_frame_secs_resp
{
  u_int32_t index;
  u_int32_t* err_frame_secs;
};

struct hal_msg_efm_err_frames
{
  u_int32_t index;
  ut_int64_t err_frames;
};

struct hal_msg_efm_err_frame_stat
{
  u_int32_t index;
  u_int32_t err_frames;
};

int
hal_msg_encode_err_frames_req (u_char **pnt, u_int32_t *size, u_int32_t *msg);

int
hal_msg_decode_err_frames_req (u_char **pnt, u_int32_t *size, u_int32_t *msg);

int
hal_msg_encode_err_frames_resp (u_char **pnt, u_int32_t *size,
                               struct hal_msg_efm_err_frames *msg);

int
hal_msg_decode_err_frames_resp(u_char **pnt, u_int32_t *size,
                               struct hal_msg_efm_err_frames_resp *msg);

int
hal_msg_encode_err_frame_secs_req (u_char **pnt, u_int32_t *size, u_int32_t *msg);

int
hal_msg_decode_err_frame_secs_req (u_char **pnt, u_int32_t *size, u_int32_t *msg);


int
hal_msg_encode_err_frame_secs_resp (u_char **pnt, u_int32_t *size,
                                struct hal_msg_efm_err_frame_stat *msg);
int
hal_msg_decode_err_frame_secs_resp (u_char **pnt, u_int32_t *size,
                                struct hal_msg_efm_err_frame_secs_resp *msg);

#endif /* HAVE_ONMD */
#ifdef HAVE_PBR
int hal_msg_encode_pbr_ipv4_route (u_char **pnt, u_int32_t *size,
                                   struct hal_msg_pbr_ipv4_route *msg);
#endif /* HAVE_PBR */

#ifdef HAVE_TUNNEL
/* HAL tunnel APIs */
int hal_msg_tunnel_add (struct hal_msg_tunnel_add_if *tif);
int hal_msg_tunnel_delete (struct hal_msg_tunnel_if *tif);
int hal_msg_tunnel_initiator_set (struct hal_msg_tunnel_if *tif);
int hal_msg_tunnel_initiator_clear (struct hal_msg_tunnel_if *tif);
int hal_msg_tunnel_terminator_set (struct hal_msg_tunnel_if *tif);
int hal_msg_tunnel_terminator_clear (struct hal_msg_tunnel_if *tif);
#endif

/*
  Error codes.
*/
#define HAL_MSG_ERR_BASE                            -100
#define HAL_MSG_PKT_TOO_SMALL                       (HAL_MSG_ERR_BASE + 1)
#define HAL_MSG_MEM_ALLOC_ERROR                     (HAL_MSG_ERR_BASE + 2)

#endif /* _HAL_MSG_H_ */
