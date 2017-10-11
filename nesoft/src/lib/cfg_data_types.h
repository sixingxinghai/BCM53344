/* Copyright (C) 2007  All Rights Reserved.  */

#ifndef _CFG_DATA_TYPES_H
#define _CFG_DATA_TYPES_H

/*-------------------------------------------------------------
 * cfgDataTypes_e - enumeration type consists identifiers for
 *                  all aggregate data types.
 * The list consists of two parts:
 * 1. Contains data types as the result of direct
 *    mapping to CLI modes.
 * 2. Contains data types that are configured
 *    in the CONFIG_MODE and/or are local to IMI.
 *    These are not mapped to CLI modes.
 * The data types are used to identify configuration data coming
 * to the IMI at the time of parsing the CLI command before it is
 * executed by the CLI command handler.
 *--------------------------------------------------------------
 */
typedef enum cfgDataType
{
  /* Data types mapping directly into cli_mode_types.h. */
  CFG_DTYP_LOGIN              = 0,
  CFG_DTYP_AUTH               = 1,
  CFG_DTYP_AUTH_ENABLE        = 2,
  CFG_DTYP_SERVICE            = 3,
  CFG_DTYP_EXEC               = 4,
  CFG_DTYP_CONFIG             = 5,
  CFG_DTYP_LINE               = 6,
  CFG_DTYP_DNS                = 7,
  CFG_DTYP_PPPOE              = 8,
  CFG_DTYP_DHCP               = 9,
  CFG_DTYP_DHCPS              = 10,
  CFG_DTYP_DEBUG              = 11,
  CFG_DTYP_KEYCHAIN           = 12,
  CFG_DTYP_KEYCHAIN_KEY       = 13,
  CFG_DTYP_VR                 = 14,
  CFG_DTYP_VRF                = 15,
  CFG_DTYP_INTERFACE          = 16,
  CFG_DTYP_INTERFACE_L2_FAST  = 17,
  CFG_DTYP_INTERFACE_L2_GIGE  = 18,
  CFG_DTYP_INTERFACE_L2_RANGE = 19,
  CFG_DTYP_INTERFACE_L2_EXT   = 20,
  CFG_DTYP_INTERFACE_MANAGE   = 21,
  CFG_DTYP_NSM_MPLS           = 22,
  CFG_DTYP_NSM_MPLS_STATIC    = 23,
  CFG_DTYP_NSM_VPLS           = 24,
  CFG_DTYP_NSM_MCAST          = 25,
  CFG_DTYP_NSM_MCAST6         = 26,
  CFG_DTYP_ROUTER             = 27,
  CFG_DTYP_BGP                = 28,
  CFG_DTYP_BGP_IPV4           = 29,
  CFG_DTYP_BGP_IPV4M          = 30,
  CFG_DTYP_BGP_IPV4_VRF       = 31,
  CFG_DTYP_BGP_IPV6           = 32,
  CFG_DTYP_BGP_VPNV4          = 33,
  CFG_DTYP_RIP                = 34,
  CFG_DTYP_RIP_VRF            = 35,
  CFG_DTYP_RIPNG              = 36,
  CFG_DTYP_OSPF               = 37,
  CFG_DTYP_OSPF6              = 38,
  CFG_DTYP_ISIS               = 39,
  CFG_DTYP_ISIS_IPV6          = 40,
  CFG_DTYP_PDM                = 41,
  CFG_DTYP_PDM6               = 42,
  CFG_DTYP_PIM                = 43,
  CFG_DTYP_PIM6               = 44,
  CFG_DTYP_DVMRP              = 45,
  CFG_DTYP_VRRP               = 46,
  CFG_DTYP_RMM                = 47,
  CFG_DTYP_LDP                = 48,
  CFG_DTYP_LDP_TRUNK          = 49,
  CFG_DTYP_LDP_PATH           = 50,
  CFG_DTYP_RSVP               = 51,
  CFG_DTYP_RSVP_PATH          = 52,
  CFG_DTYP_RSVP_TRUNK         = 53,
  CFG_DTYP_IP                 = 54,
  CFG_DTYP_COMMUNITY_LIST     = 55,
  CFG_DTYP_PREFIX_IPV4        = 56,
  CFG_DTYP_ACCESS_IPV4        = 57,
  CFG_DTYP_L2_ACCESS          = 58,
  CFG_DTYP_IPV6               = 59,
  CFG_DTYP_ACCESS_IPV6        = 60,
  CFG_DTYP_PREFIX_IPV6        = 61,
  CFG_DTYP_AS_LIST            = 62,
  CFG_DTYP_RMAP               = 63,
  CFG_DTYP_USER               = 64,
  CFG_DTYP_DUMP               = 65,
  CFG_DTYP_BRIDGE             = 66,
  CFG_DTYP_VLAN               = 67,
  CFG_DTYP_STP                = 68,
  CFG_DTYP_RSTP               = 69,
  CFG_DTYP_MSTP               = 70,
  CFG_DTYP_MST_CONFIG         = 71,
  CFG_DTYP_AUTH8021X          = 72,
  CFG_DTYP_GVRP               = 73,
  CFG_DTYP_GMRP               = 74,
  CFG_DTYP_IGMP               = 75,
  CFG_DTYP_VTY                = 76,
  CFG_DTYP_SMUX               = 77,
  CFG_DTYP_EXEC_PRIV          = 78,   /* Fake mode.  Same as EXEC.   */
  CFG_DTYP_IMISH              = 79,   /* For shell and IMI communication.   */
  CFG_DTYP_MODIFIER           = 80,   /* Output modifier node.  */
  CFG_DTYP_QOS                = 81,
  CFG_DTYP_CMAP               = 82,
  CFG_DTYP_PMAP               = 83,
  CFG_DTYP_PMAPC              = 84,
  CFG_DTYP_IGMP_IF            = 85,
  CFG_DTYP_ARP                = 86,
  CFG_DTYP_IP_MROUTE          = 87,
  CFG_DTYP_IPV6_MROUTE        = 88,
  CFG_DTYP_VLAN_ACCESS        = 89,
  CFG_DTYP_VLAN_FILTER        = 90,
  CFG_DTYP_MAC_ACL            = 91,
  CFG_DTYP_RMON               = 92,
  CFG_DTYP_MLD                = 93,
  CFG_DTYP_MLD_IF             = 94,
  CFG_DTYP_TRANSFORM          = 95,
  CFG_DTYP_CRYPTOMAP          = 96,
  CFG_DTYP_ISAKMP             = 97,
  CFG_DTYP_STACKING           = 98,
  CFG_DTYP_RSVP_BYPASS        = 99,
  CFG_DTYP_FIREWALL           = 100,
  CFG_DTYP_VRRP6              = 101,
  CFG_DTYP_ONMD               = 102,
  CFG_DTYP_CVLAN_REGIST       = 103,
  CFG_DTYP_ETH_CFM            = 104,
  CFG_DTYP_ETH_CFM_PBB        = 105,
  CFG_DTYP_BGP_6PE            = 106,
  CFG_DTYP_BGP_VPNV6          = 107,
  CFG_DTYP_BGP_IPV6_VRF       = 108,
  CFG_DTYP_TE_MSTI            = 109,
  CFG_DTYP_RPVST_PLUS         = 110,
  CFG_DTYP_RIPNG_VRF          = 111,
  CFG_DTYP_OSPF6_VRF          = 112,
  CFG_DTYP_BFD                = 113,
  CFG_DTYP_VLOG               = 114,

  CFG_DTYP_ETH_CFM_VLAN       = 115,
  CFG_DTYP_ETH_CFM_PBB_TE     = 116,
  CFG_DTYP_PBB_TE_VLAN_CONFIG = 117,
  CFG_DTYP_ETH_PBB_TE_EPS     = 118,
  CFG_DTYP_G8031_SWITCHING    = 119,
  CFG_DTYP_G8031_VLAN         = 120,
  CFG_DTYP_G8032_CONFIG       = 121,
  CFG_DTYP_G8032_VLAN         = 122,
  CFG_DTYP_PBB_ISID_CONFIG    = 123,
  CFG_DTYP_NSM_EVC_SERVICE_INSTANCE = 124,

  CFG_DTYP_TELINK             = 125,
  CFG_DTYP_CTRL_CHNL          = 126,
  CFG_DTYP_CTRL_ADJ           = 127,
  CFG_DTYP_NSM_GMPLS_STATIC   = 128,
  CFG_DTYP_LMP                = 129,

  CFG_DTYP_PBB_TE_ESP_IF_CONFIG = 133,
  CFG_DTYP_LDP_ROUTER         = 134,
  /* End of direct mapping to the cli_mode_types.h */

  /* Local IMI defined data types. */
  CFG_DTYP_IMI_FIRST          = 200,
  CFG_DTYP_IMI_HOST_SERVICE   = 201,
  CFG_DTYP_IMI_DHCPS_SERVICE  = 202,
  CFG_DTYP_IMI_HOST           = 203,
  CFG_DTYP_IMI_CRX_DEBUG      = 204,
  CFG_DTYP_IMI_PPPOE          = 205,
  CFG_DTYP_IMI_DNS            = 206,
  CFG_DTYP_IMI_DHCPS          = 207,
  CFG_DTYP_IMI_SNMP           = 208,
  CFG_DTYP_IMI_VR             = 209,
  CFG_DTYP_IMI_NAT            = 210,
  CFG_DTYP_IMI_VIRTUAL_SERVER = 211,
  CFG_DTYP_IMI_NTP            = 212,
  CFG_DTYP_IMI_EXTERN         = 213,
  CFG_DTYP_IMI_CRX            = 214,
  CFG_DTYP_IMI_ARP            = 215,
  CFG_DTYP_IMI_LINE           = 216,
  CFG_DTYP_IP_BFD             = 217,

  CFG_DTYP_TUN                = 218,
  CFG_DTYP_AC                 = 219,//fxg
  CFG_DTYP_PW                 = 220,//fxg
  CFG_DTYP_VPN                = 221,
  CFG_DTYP_MEG                = 223,
  CFG_DTYP_OAM_1731            = 224,
  CFG_DTYP_SEC                 = 225,

  CFG_DTYP_LSP                = 226,
  CFG_DTYP_RPORT                =227,
  
  CFG_DTYP_PTP                = 228,
  CFG_DTYP_PTP_PORT           = 229,

  CFG_DTYP_LSP_GRP                = 230,
  CFG_DTYP_LSP_OAM                = 231,
  
  CFG_DTYP_MAX
} cfgDataType_e;

#endif /* _CFG_DATA_TYPES_H */
