/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _PACOS_CLI_MODE_TYPES_H
#define _PACOS_CLI_MODE_TYPES_H

/* CLI modes.  */
#define LOGIN_MODE              0
#define AUTH_MODE               1
#define AUTH_ENABLE_MODE        2
#define SERVICE_MODE            3
#define EXEC_MODE               4
#define CONFIG_MODE             5
#define LINE_MODE               6
#define DNS_MODE                7
#define PPPOE_MODE              8
#define DHCP_MODE               9
#define DHCPS_MODE              10
#define DEBUG_MODE              11
    /* Keychain configuration.  */
#define KEYCHAIN_MODE           12
#define KEYCHAIN_KEY_MODE       13
    /* VR. */
#define VR_MODE                 14
    /* IP VRF.  */
#define VRF_MODE                15
    /* Interface.  */
#define INTERFACE_MODE          16
#define INTERFACE_L2_FAST_MODE  17
#define INTERFACE_L2_GIGE_MODE  18
#define INTERFACE_L2_RANGE_MODE 19
#define INTERFACE_L2_EXT_MODE   20
#define INTERFACE_MANAGE_MODE   21
    /* NSM MPLS node. */
#define NSM_MPLS_MODE           22
#define NSM_MPLS_STATIC_MODE    23
    /* NSM VPLS node. */
#define NSM_VPLS_MODE           24
    /* NSM MCAST node. */
#define NSM_MCAST_MODE          25
    /* NSM MCAST6 node. */
#define NSM_MCAST6_MODE         26

    /* Router ID/Hostname node. */
#define ROUTER_MODE             27
#define BGP_MODE                28
#define BGP_IPV4_MODE           29
#define BGP_IPV4M_MODE          30
#define BGP_IPV4_VRF_MODE       31
#define BGP_IPV6_MODE           32
#define BGP_VPNV4_MODE          33
#define RIP_MODE                34
#define RIP_VRF_MODE            35
#define RIPNG_MODE              36
#define OSPF_MODE               37
#define OSPF6_MODE              38
#define ISIS_MODE               39
#define ISIS_IPV6_MODE          40
#define PDM_MODE                41
#define PDM6_MODE               42
#define PIM_MODE                43
#define PIM6_MODE               44
#define DVMRP_MODE              45
#define VRRP_MODE               46
#define RMM_MODE                47
#define LDP_MODE                48
#define LDP_TRUNK_MODE          49
#define LDP_PATH_MODE           50
#define RSVP_MODE               51
#define RSVP_PATH_MODE          52
#define RSVP_TRUNK_MODE         53
    /* Static routes.  */
#define IP_MODE                 54
    /* Community list.  */
#define COMMUNITY_LIST_MODE     55
    /* Access list and prefix list.  */
#define PREFIX_MODE             56
#define ACCESS_MODE             57
#define L2_ACCESS_MODE          58
    /* IPv6 static routes.  */
#define IPV6_MODE               59
    /* IPv6 access list and prefix list.  */
#define ACCESS_IPV6_MODE        60
#define PREFIX_IPV6_MODE        61
    /* AS path access list.  */
#define AS_LIST_MODE            62
    /* Route-map.  */
#define RMAP_MODE               63
    /* User management. */
#define USER_MODE               64
    /* BGP dump mode. */
#define DUMP_MODE               65
    /* Layer 2 */
#define BRIDGE_MODE             66
#define VLAN_MODE               67
#define STP_MODE                68
#define RSTP_MODE               69
#define MSTP_MODE               70
#define MST_CONFIG_MODE         71
#define AUTH8021X_MODE          72
#define GVRP_MODE               73
#define GMRP_MODE               74
#define IGMP_MODE               75
    /* VTY */
#define VTY_MODE                76
    /* Fake modes used for config-write. */
#define SMUX_MODE               77
    /* Below is special modes.  */
#define EXEC_PRIV_MODE          78   /* Fake mode.  Same as EXEC_MODE.   */
#define IMISH_MODE              79  /* For shell and IMI communication.   */
#define MODIFIER_MODE           80  /* Output modifier node.  */
    /* QoS modes */
#define QOS_MODE                81
    /* Class Map mode */
#define CMAP_MODE               82
    /* Policy Map mode */
#define PMAP_MODE               83
    /* Policy Map Class mode */
#define PMAPC_MODE              84
#define IGMP_IF_MODE            85
#define ARP_MODE                86
/* Static multicast route mode */
#define IP_MROUTE_MODE          87
#define IPV6_MROUTE_MODE        88
#define VLAN_ACCESS_MODE        89
#define VLAN_FILTER_MODE        90
#define MAC_ACL_MODE            91
#define RMON_MODE               92
#define MLD_MODE                93
#define MLD_IF_MODE             94
/* IPSEC Transform Set represents a certain combination of
   security protocols and algorithms */
#define TRANSFORM_MODE          95
/* IPSEC CRYPTOMAP represents all parts needed to setup
   IPSec Security Associations and traffic to be protected
*/
#define CRYPTOMAP_MODE          96
#define ISAKMP_MODE             97
#define STACKING_MODE           98
#define RSVP_BYPASS_MODE        99
/* Firewall Mode */
#define FIREWALL_MODE           100
#define VRRP6_MODE              101

/*Modes for Metro Ethernet Enhancements */
#define ONMD_MODE               102
#define CVLAN_REGIST_MODE       103
#define ETH_CFM_MODE            104
#define ETH_CFM_PBB_MODE        105
/*Mode for 6PE*/
#define BGP_6PE_MODE            106
/* New mode added for IPv6 VRF new address family*/
#define BGP_VPNV6_MODE          107
/* New mode added for IPv6 VRF*/
#define BGP_IPV6_VRF_MODE       108
 /*Mode for L2-R3 */
#define TE_MSTI_CONFIG_MODE     109
#define RPVST_PLUS_CONFIG_MODE  110
#define RIPNG_VRF_MODE          111
#define OSPF6_VRF_MODE          112
#define BFD_MODE                113
#define VLOG_MODE               114
/* CFM enhancement to IEEE 802.1ag 2007 */
#define ETH_CFM_VLAN_MODE       115
#define ETH_CFM_PBB_TE_MODE     116
#define PBB_TE_VLAN_CONFIG_MODE 117
#define ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE 118
/*Mode for G8031 - EPS  */
#define G8031_CONFIG_SWITCHING_MODE 119
#define G8031_CONFIG_VLAN_MODE  120
/*Mode for G8032 - RAPS  */
#define G8032_CONFIG_MODE       121
#define G8032_CONFIG_VLAN_MODE  122
#define PBB_ISID_CONFIG_MODE    123
#define EVC_SERVICE_INSTANCE_MODE  124
#define ELMI_MODE               125

/* GMPLS Modes */
#define TELINK_MODE             126
#define CTRL_CHNL_MODE          127
#define CTRL_ADJ_MODE           128
#define NSM_GMPLS_STATIC_MODE   129
/* LMP Modes */
#define LMP_MODE                130
#define DCB_MODE		131

#define IP_BFD_MODE             132

/* PBB-TE ESP MODE */
#define PBB_TE_ESP_IF_CONFIG_MODE 133

/* Mode for LDP Router */
#define TARGETED_PEER_MODE      134

/* MAX Mode for the CLI's. All new modes should have less than this value*/

#define PW_MODE                 135
#define VPN_MODE                 136

#define AC_MODE                 138
#define TUN_MODE             139

#define MEG_MODE             140

#define OAM_1731_MODE             141
#define SEC_MODE             142
#define LSP_MODE                 143
#define RPORT_MODE               144

#define PTP_MODE                 145
#define PTP_PORT_MODE                 146

#define LSP_GRP_MODE                 147
#define LSP_OAM_MODE                 148


#define MAX_MODE                149

#endif /* _PACOS_CLI_MODE_TYPES_H */
