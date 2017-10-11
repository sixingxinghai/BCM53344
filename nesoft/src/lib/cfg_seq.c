/* Copyright (C) 2007-2009  All Rights Reserved.  */

/*-------------------------------------------------------------
 * cfgDataSeq - Defines sequence of configuration data in the
 *              PacOS config file.
 *--------------------------------------------------------------
 */
#include "pal.h"
#include "lib.h"
#include "cfg_data_types.h"
#include "cfg_seq.h"

#define __IMI_LOCAL (-1)


cfgDataSeq_t cfgDataSeq[] =
{
  { CFG_DTYP_IMI_HOST_SERVICE,    CFG_STORE_IMI, __IMI_LOCAL }, /* host_service_write */
  { CFG_DTYP_IMI_DHCPS_SERVICE,   CFG_STORE_IMI, __IMI_LOCAL }, /* imi_dhcps_service_write */
  { CFG_DTYP_IMI_HOST,            CFG_STORE_IMI, __IMI_LOCAL }, /* host_config_write */
  { CFG_DTYP_IMI_CRX_DEBUG,       CFG_STORE_IMI, __IMI_LOCAL }, /* crx_debug_config_write */
  { CFG_DTYP_DEBUG,               CFG_STORE_PM , DEBUG_MODE },
  { CFG_DTYP_STACKING,            CFG_STORE_PM , STACKING_MODE },
  { CFG_DTYP_IMI_PPPOE,           CFG_STORE_IMI, __IMI_LOCAL }, /* imi_pppoe_config_write */
  { CFG_DTYP_IMI_DNS,             CFG_STORE_IMI, __IMI_LOCAL }, /* imi_dns_config_write */
  { CFG_DTYP_IMI_DHCPS,           CFG_STORE_IMI, __IMI_LOCAL }, /* imi_dhcps_config_write */
  { CFG_DTYP_IMI_SNMP,            CFG_STORE_IMI, __IMI_LOCAL }, /* imi_snmp_config_write */
  { CFG_DTYP_IMI_VR,              CFG_STORE_IMI, __IMI_LOCAL }, /* imi_vr_config_write */
  { CFG_DTYP_VRF,                 CFG_STORE_PM , VRF_MODE },
  { CFG_DTYP_KEYCHAIN,            CFG_STORE_IMI, __IMI_LOCAL }, /* keychain_config_write */
  { CFG_DTYP_NSM_MPLS,            CFG_STORE_PM , NSM_MPLS_MODE },
  { CFG_DTYP_NSM_VPLS,            CFG_STORE_PM , NSM_VPLS_MODE },
  { CFG_DTYP_TUN,                  CFG_STORE_PM , TUN_MODE},
  { CFG_DTYP_AC,                   CFG_STORE_PM , AC_MODE},
  { CFG_DTYP_RPORT,                CFG_STORE_PM , RPORT_MODE},
  { CFG_DTYP_PW,                  CFG_STORE_PM , PW_MODE},
  { CFG_DTYP_VPN,                  CFG_STORE_PM , VPN_MODE},
  { CFG_DTYP_LSP,                  CFG_STORE_PM , LSP_MODE},
  { CFG_DTYP_LSP_GRP,              CFG_STORE_PM , LSP_GRP_MODE},
  { CFG_DTYP_LSP_OAM,              CFG_STORE_PM , LSP_OAM_MODE},
  { CFG_DTYP_MEG,                  CFG_STORE_PM , MEG_MODE},
  { CFG_DTYP_PTP,                  CFG_STORE_PM , PTP_MODE},
  { CFG_DTYP_PTP_PORT,             CFG_STORE_PM , PTP_PORT_MODE},
#ifdef HAVE_MCAST_IPV4
  { CFG_DTYP_NSM_MCAST,           CFG_STORE_PM , NSM_MCAST_MODE },
#endif
#ifdef HAVE_MCAST_IPV6
  { CFG_DTYP_NSM_MCAST6,          CFG_STORE_PM , NSM_MCAST6_MODE },
#endif
  { CFG_DTYP_EXEC,                CFG_STORE_PM , EXEC_MODE },
  { CFG_DTYP_CONFIG,              CFG_STORE_PM , CONFIG_MODE },
  { CFG_DTYP_GVRP,                CFG_STORE_PM , GVRP_MODE },
  { CFG_DTYP_GMRP,                CFG_STORE_PM , GMRP_MODE },
  { CFG_DTYP_PDM,                 CFG_STORE_PM , PDM_MODE },
  { CFG_DTYP_PDM6,                CFG_STORE_PM , PDM6_MODE },
#ifdef HAVE_QOS
  { CFG_DTYP_CMAP,                CFG_STORE_PM , CMAP_MODE },
  { CFG_DTYP_PMAP,                CFG_STORE_PM , PMAP_MODE },
  { CFG_DTYP_PMAPC,               CFG_STORE_PM , PMAPC_MODE },
#endif
  { CFG_DTYP_AUTH8021X,           CFG_STORE_PM , AUTH8021X_MODE },
  { CFG_DTYP_VLAN,                CFG_STORE_PM , VLAN_MODE },
  { CFG_DTYP_MST_CONFIG,          CFG_STORE_PM , MST_CONFIG_MODE },
#ifdef HAVE_RPVST_PLUS
  { CFG_DTYP_RPVST_PLUS,          CFG_STORE_PM , RPVST_PLUS_CONFIG_MODE },
#endif
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HVE_B_BEB)
  { CFG_DTYP_TE_MSTI,             CFG_STORE_PM , TE_MSTI_CONFIG_MODE },
#endif
  { CFG_DTYP_ETH_CFM,             CFG_STORE_PM , ETH_CFM_MODE },
  { CFG_DTYP_ETH_CFM_PBB,         CFG_STORE_PM , ETH_CFM_PBB_MODE },
  { CFG_DTYP_ETH_CFM_VLAN,        CFG_STORE_PM , ETH_CFM_VLAN_MODE },
  { CFG_DTYP_CVLAN_REGIST,        CFG_STORE_PM , CVLAN_REGIST_MODE },
  { CFG_DTYP_VLAN_ACCESS,         CFG_STORE_PM , VLAN_ACCESS_MODE },
  { CFG_DTYP_PBB_ISID_CONFIG,     CFG_STORE_PM , PBB_ISID_CONFIG_MODE },
  { CFG_DTYP_PBB_TE_VLAN_CONFIG,  CFG_STORE_PM , PBB_TE_VLAN_CONFIG_MODE },
  { CFG_DTYP_ETH_CFM_PBB_TE,      CFG_STORE_PM , ETH_CFM_PBB_TE_MODE },
#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMPSNOOP
  { CFG_DTYP_IGMP,                CFG_STORE_PM , IGMP_MODE },
  { CFG_DTYP_IGMP_IF,             CFG_STORE_PM , IGMP_IF_MODE },
#endif
  { CFG_DTYP_PIM,                 CFG_STORE_PM , PIM_MODE },
  { CFG_DTYP_PIM6,                CFG_STORE_PM , PIM6_MODE },
  { CFG_DTYP_LDP,                 CFG_STORE_PM , LDP_MODE },
  { CFG_DTYP_LDP_ROUTER,          CFG_STORE_PM , TARGETED_PEER_MODE },
  { CFG_DTYP_RSVP,                CFG_STORE_PM , RSVP_MODE },
  { CFG_DTYP_INTERFACE,           CFG_STORE_PM , INTERFACE_MODE },
  { CFG_DTYP_PBB_TE_ESP_IF_CONFIG, CFG_STORE_PM , PBB_TE_ESP_IF_CONFIG_MODE },
#ifdef HAVE_GMPLS
  { CFG_DTYP_TELINK,              CFG_STORE_PM , TELINK_MODE },
  { CFG_DTYP_CTRL_CHNL,           CFG_STORE_PM , CTRL_CHNL_MODE },
  { CFG_DTYP_CTRL_ADJ,            CFG_STORE_PM , CTRL_ADJ_MODE },
  { CFG_DTYP_NSM_GMPLS_STATIC,    CFG_STORE_PM , NSM_GMPLS_STATIC_MODE },
#endif /*HAVE_GMPLS*/
#ifdef HAVE_LMP
  { CFG_DTYP_LMP,                 CFG_STORE_PM , LMP_MODE },
#endif /* HAVE_LMP */
  { CFG_DTYP_NSM_MPLS_STATIC,     CFG_STORE_PM , NSM_MPLS_STATIC_MODE },
  { CFG_DTYP_ETH_PBB_TE_EPS,      CFG_STORE_PM , ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE },
  { CFG_DTYP_G8031_VLAN,          CFG_STORE_PM , G8031_CONFIG_VLAN_MODE },
  { CFG_DTYP_G8031_SWITCHING,     CFG_STORE_PM , G8031_CONFIG_SWITCHING_MODE },
  { CFG_DTYP_G8032_VLAN,          CFG_STORE_PM , G8032_CONFIG_VLAN_MODE },
  { CFG_DTYP_G8032_CONFIG,        CFG_STORE_PM , G8032_CONFIG_MODE },
  { CFG_DTYP_NSM_EVC_SERVICE_INSTANCE, CFG_STORE_PM,  EVC_SERVICE_INSTANCE_MODE},
  { CFG_DTYP_OSPF,                CFG_STORE_PM , OSPF_MODE },
  { CFG_DTYP_OSPF6,               CFG_STORE_PM , OSPF6_MODE },
  { CFG_DTYP_ISIS,                CFG_STORE_PM , ISIS_MODE },
  { CFG_DTYP_RIP,                 CFG_STORE_PM , RIP_MODE },
  { CFG_DTYP_RIPNG,               CFG_STORE_PM , RIPNG_MODE },
  { CFG_DTYP_VR,                  CFG_STORE_PM , VR_MODE },
  { CFG_DTYP_BGP,                 CFG_STORE_PM , BGP_MODE },
  { CFG_DTYP_BFD,                 CFG_STORE_PM , BFD_MODE },
#ifdef HAVE_VRRP
  { CFG_DTYP_VRRP,                CFG_STORE_PM , VRRP_MODE },
#ifdef HAVE_IPV6
  { CFG_DTYP_VRRP6,               CFG_STORE_PM , VRRP6_MODE },
#endif
#endif
  { CFG_DTYP_LDP_PATH,            CFG_STORE_PM , LDP_PATH_MODE },
  { CFG_DTYP_LDP_TRUNK,           CFG_STORE_PM , LDP_TRUNK_MODE },
  { CFG_DTYP_RSVP_PATH,           CFG_STORE_PM , RSVP_PATH_MODE },
  { CFG_DTYP_RSVP_TRUNK,          CFG_STORE_PM , RSVP_TRUNK_MODE },
#ifdef HAVE_MPLS_FRR
  { CFG_DTYP_RSVP_BYPASS,         CFG_STORE_PM , RSVP_BYPASS_MODE },
#endif
  { CFG_DTYP_IP,                  CFG_STORE_PM , IP_MODE },
  { CFG_DTYP_IP_MROUTE,           CFG_STORE_PM , IP_MROUTE_MODE },
#ifdef HAVE_BFD
  { CFG_DTYP_IP_BFD,              CFG_STORE_PM , IP_BFD_MODE },
#endif /* HAVE_BFD */
  { CFG_DTYP_COMMUNITY_LIST,      CFG_STORE_PM , COMMUNITY_LIST_MODE },
  { CFG_DTYP_AS_LIST,             CFG_STORE_PM , AS_LIST_MODE },
  { CFG_DTYP_ACCESS_IPV4,         CFG_STORE_IMI, __IMI_LOCAL }, /* config_write_access_ipv4 */
  { CFG_DTYP_PREFIX_IPV4,         CFG_STORE_IMI, __IMI_LOCAL }, /* config_write_prefix_ipv4 */
#ifdef HAVE_IPV6
  { CFG_DTYP_IPV6,                CFG_STORE_PM , IPV6_MODE },
  { CFG_DTYP_IPV6_MROUTE,         CFG_STORE_PM , IPV6_MROUTE_MODE },
  { CFG_DTYP_ACCESS_IPV6,         CFG_STORE_IMI, __IMI_LOCAL }, /* config_write_access_ipv6 */
  { CFG_DTYP_PREFIX_IPV6,         CFG_STORE_IMI, __IMI_LOCAL }, /* config_write_prefix_ipv6 */
#endif
  { CFG_DTYP_RMAP,                CFG_STORE_IMI, __IMI_LOCAL }, /* route_map_config_write */
  { CFG_DTYP_IMI_NAT,             CFG_STORE_IMI, __IMI_LOCAL }, /* imi_nat_config_write */
  { CFG_DTYP_IMI_VIRTUAL_SERVER,  CFG_STORE_IMI, __IMI_LOCAL }, /* imi_virtual_server_config_write */
  { CFG_DTYP_IMI_NTP,             CFG_STORE_IMI, __IMI_LOCAL }, /* imi_ntp_config_write */
  { CFG_DTYP_IMI_EXTERN,          CFG_STORE_IMI, __IMI_LOCAL }, /* imi_extern_config_write */
  { CFG_DTYP_IMI_CRX,             CFG_STORE_IMI, __IMI_LOCAL }, /* crx_config_write */
  { CFG_DTYP_ARP,                 CFG_STORE_PM , ARP_MODE },
  { CFG_DTYP_IMI_LINE,            CFG_STORE_IMI, __IMI_LOCAL } /* IMI_LINE_CONFIG_WRITE */
};

void
cfg_seq_walk(intptr_t ref1, intptr_t ref2, cfgSeqWalkCb_t walk_cb)
{
  int seq_ix;
  cfgDataSeq_t *seq_entry;

  if (walk_cb == NULL)
    return;

  for (seq_ix=0; seq_ix<sizeof(cfgDataSeq)/sizeof(cfgDataSeq_t); seq_ix++)
    {
      seq_entry = &cfgDataSeq[seq_ix];
      walk_cb(ref1, ref2,
              seq_entry->cfgStoreType,
              seq_entry->cfgDataType,
              seq_entry->cfgConfigMode);
    }
}


bool_t
cfg_seq_is_imi_dtype (int seq_ix)
{
  if (seq_ix >= sizeof(cfgDataSeq)/sizeof(cfgDataSeq_t)) {
    return PAL_FALSE;
  }
  return (cfgDataSeq[seq_ix].cfgStoreType == CFG_STORE_IMI);
}

cfgDataType_e
cfg_seq_get_dtype (int seq_ix)
{
  if (seq_ix >= sizeof(cfgDataSeq)/sizeof(cfgDataSeq_t)) {
    return CFG_DTYP_MAX;
  }
  return (cfgDataSeq[seq_ix].cfgDataType);
}

int
cfg_seq_get_index (cfgDataType_e dtype)
{
  int seq_ix;

  for (seq_ix=0; seq_ix<sizeof(cfgDataSeq)/sizeof(cfgDataSeq_t); seq_ix++) {
    if (cfgDataSeq[seq_ix].cfgDataType == dtype) {
      return seq_ix;
    }
  }
  return (-1);
}

