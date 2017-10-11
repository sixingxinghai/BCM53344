/* Copyright (C) 2003  All Rights Reserved.  */

/* Common mode functions.  This library is only for IMI and IMI shell
   not needed in protocol module.  */

#include <pal.h>

#include "cli.h"
#include "modbmap.h"

/* Static utility function to change mode.  */
static void
cli_mode_change (struct cli *cli, int mode)
{
  cli->mode = mode;
}
#ifdef HAVE_B_BEB
CLI (g8032_configure_beb_ring_create,
     g8032_configure_beb_ring_create_cmd,
     "g8032 configure switching ring-id RINGID bridge (<1-32> | backbone)",
     "g8032 realted parameters",
     "configure Ring  protection parameters",
     "RAPS switching parameters",
     "Ring ID to identify a ring  protection",
     "ring ID undergoing switching configuration",
     "Bridge group commands",
     "Bridge group for bridging",
     "backbone bridge")
{
  cli_mode_change (cli, G8032_CONFIG_MODE);
  return CLI_SUCCESS;
}

CLI (g8032_beb_vlan_create,
     g8032_beb_vlan_create_cmd,
     "g8032 configure vlan ring-id RINGID bridge (<1-32> | backbone)",
     "parameters are G.8032 related",
     "Ring  configuration",
     "configure vlan members",
     "unique id to identify a Ring protection link",
     "Ring protection group ID",
     "Bridge group",
     "Bridge group name",
     "backbone bridge")
{
  cli_mode_change (cli, G8032_CONFIG_VLAN_MODE);
  return CLI_SUCCESS;
}

#else
CLI (g8032_configure_ring_create,
     g8032_configure_ring_create_cmd,
     "g8032 configure switching ring-id RINGID bridge <1-32>",
     "g8032 realted parameters",
     "configure Ring  protection parameters",
     "RAPS switching parameters",
     "unique id to identify a ring  protection ",
     "ring ID undergoing switching configuration",
     "Bridge group commands",
     "Bridge group for bridging")
{
  cli_mode_change (cli, G8032_CONFIG_MODE);
  return CLI_SUCCESS;
}
CLI (g8032_vlan_create,
     g8032_vlan_create_cmd,
     "g8032 configure vlan ring-id RINGID bridge <1-32>",
     "parameters are G.8032 related",
     "Ring configuration",
     "configure vlan members",
     "unique id to identify a Ring protection link",
     "Ring protection group ID",
     "Bridge group",
     "Bridge group name")
{
  cli_mode_change (cli, G8032_CONFIG_VLAN_MODE);
  return CLI_SUCCESS;
}

#endif

/* VRF mode.  */
#ifdef HAVE_VRF
CLI (mode_ip_vrf,
     mode_ip_vrf_cli,
     "ip vrf WORD",
     CLI_IP_STR,
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  cli_mode_change (cli, VRF_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_VRF */

#ifdef HAVE_VR
CLI (mode_virtual_router,
     mode_virtual_router_cli,
     "virtual-router WORD",
     CLI_VR_STR,
     CLI_VR_NAME_STR)
{
  cli_mode_change (cli, VR_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_VR */

/* RIP mode.  */
#ifdef HAVE_RIPD
CLI (mode_router_rip,
     mode_router_rip_cli,
     "router rip",
     CLI_ROUTER_STR,
     "Routing Information Protocol (RIP)")
{
  cli_mode_change (cli, RIP_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_VRF_RIP
CLI (mode_rip_af_ipv4_vrf,
     mode_rip_af_ipv4_vrf_cli,
     "address-family ipv4 vrf NAME",
     "Enter Address Family command mode",
     "Address-family",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  cli_mode_change (cli, RIP_VRF_MODE);
  return CLI_SUCCESS;
}

CLI (mode_rip_exit_af,
     mode_rip_exit_af_cli,
     "exit-address-family",
     "Exit from Address Family configuration mode")
{
  cli_mode_change (cli, RIP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_VRF_RIP */
#endif /* HAVE_RIPD */


CLI(pw_mode_by_id,
    pw_mode_by_id_cli,
    "pw <1-2000>",
    CLI_PW_MODE_STR,
    "pseudo-wire id, range 1-2000")
{
    cli_mode_change (cli, PW_MODE);
    return CLI_SUCCESS;
}


CLI(vpn_mode_by_name,
    vpn_mode_by_name_cli,
    "vpn <1-2000>",
    CLI_VPN_MODE_STR,
   "vpn id, range 1-2000")
{

    cli_mode_change (cli, VPN_MODE);
    return CLI_SUCCESS;
}

CLI(lsp_mode_by_id,
    lsp_mode_by_id_cli,
    "lsp <1-2000>",
    CLI_LSP_MODE_STR,
   "lsp id, range 1-2000")
{

 cli_mode_change (cli, LSP_MODE);
 return CLI_SUCCESS;
}

CLI(lsp_grp_mode_by_id,
    lsp_grp_mode_by_id_cli,
    "lsp-protect-group <0-63>",
    CLI_LSP_GRP_MODE_STR,
   "lsp protect group id, range 0-63")
{

 cli_mode_change (cli, LSP_GRP_MODE);
 return CLI_SUCCESS;
}

CLI(lsp_oam_mode_by_id,
    lsp_oam_mode_by_id_cli,
    "lsp-oam <0-63>",
    CLI_LSP_OAM_MODE_STR,
   "lsp oam id, range 0-63")
{

 cli_mode_change (cli, LSP_OAM_MODE);
 return CLI_SUCCESS;
}


CLI(ac_mode_by_id,
    ac_mode_by_id_cli,
    "ac <1-2000>",
    CLI_AC_MODE_STR,
    "attachment-circuit id, range 1-2000")
{
    cli_mode_change (cli, AC_MODE);
    return CLI_SUCCESS;
}

CLI(rport_mode_by_fe,
	rport_mode_by_fe_cli,
	"fe <1-24>",
	CLI_RPORT_MODE_STR,
	"fe interface id")
{
    cli_mode_change (cli, RPORT_MODE);
    return CLI_SUCCESS;
}

CLI(rport_mode_by_ge,
	rport_mode_by_ge_cli,
	"ge <1-4>",
    CLI_RPORT_MODE_STR,
    "ge interface id")
{
    cli_mode_change (cli, RPORT_MODE);
    return CLI_SUCCESS;
}

CLI(tun_mode_by_id,
    tun_mode_by_id_cli,
    "tun <1-2000>",
    CLI_TUN_MODE_STR,
    "tunnel id, range 1-2000")
{
    cli_mode_change (cli, TUN_MODE);
    return CLI_SUCCESS;
}


CLI(meg_mode_by_index,
    meg_mode_by_index_cli,
    "meg INDEX",
    CLI_MEG_MODE_STR,
   "Indentifying MEG index")
{

 cli_mode_change (cli, MEG_MODE);
 return CLI_SUCCESS;
}

CLI(sec_mode_by_id,
    sec_mode_by_id_cli,
    "section ID",
    CLI_SEC_MODE_STR,
   "Indentifying SEC ID")
{

 cli_mode_change (cli, SEC_MODE);
 return CLI_SUCCESS;
}

CLI(oam_1731_mode,
    oam_1731_mode_cli,
    "mpls-tp oam {y1731-mode |bfd-mode}",
    CLI_OAM_1731_MODE_STR,
   "Indentifying OAM_1731 mode")
{

 cli_mode_change (cli, OAM_1731_MODE);
 return CLI_SUCCESS;
}

CLI(ptp_mode,
    ptp_mode_cli,
    "ptp",
    CLI_PTP_MODE_STR,
   "PTP mode add")
{

 cli_mode_change (cli, PTP_MODE);
 return CLI_SUCCESS;
}

CLI(ptp_port_id,
    ptp_port_id_cli,
    "port <0-19>",
    CLI_PTP_PORT_MODE_STR,
   "PTP mode configure")
{

 cli_mode_change (cli, PTP_PORT_MODE);
 return CLI_SUCCESS;
}


/* RIPng mode.  */
#if defined (HAVE_RIPNGD) && defined(HAVE_IPV6)
CLI (mode_router_ipv6_rip,
     mode_router_ipv6_rip_cli,
     "router ipv6 rip",
     "Enable a routing process",
     "IPv6 information",
     "Routing Information Protocol (RIP)")
{
  cli_mode_change (cli, RIPNG_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_VRF_RIPNG
CLI (mode_rip_af_ipv6_vrf,
     mode_rip_af_ipv6_vrf_cli,
     "address-family ipv6 vrf NAME",
     "Enter Address Family command mode",
     "Address-family",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  cli_mode_change (cli, RIPNG_VRF_MODE);
  return CLI_SUCCESS;
}

CLI (mode_ripng_exit_af,
     mode_ripng_exit_af_cli,
     "exit-address-family",
     "Exit from Address Family configuration mode")
{
  cli_mode_change (cli, RIPNG_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_VRF_RIPNG */
#endif /* HAVE_RIPNGD && HAVE_IPV6 */

/* OSPF mode.  */
#ifdef HAVE_OSPFD
CLI (mode_router_ospf,
     mode_router_ospf_cli,
     "router ospf",
     "Enable a routing process",
     "Open Shortest Path First (OSPF)")
{
  cli_mode_change (cli, OSPF_MODE);

  return CLI_SUCCESS;
}

#ifdef HAVE_VRF_OSPF
CLI (mode_router_ospf_id_vrf,
     mode_router_ospf_id_vrf_cli,
     "router ospf <1-65535> WORD",
     "Enable a routing process",
     "Start OSPF configuration",
     "OSPF process ID",
     "VRF Name to associate with this instance")
{
  cli_mode_change (cli, OSPF_MODE);

  return CLI_SUCCESS;
}
#endif /* HAVE_VRF_OSPF */
CLI (mode_router_ospf_id,
     mode_router_ospf_id_cli,
     "router ospf <1-65535>",
     "Enable a routing process",
     "Open Shortest Path First (OSPF)",
     "OSPF process ID")
{
  cli_mode_change (cli, OSPF_MODE);

  return CLI_SUCCESS;
}

#endif /* HAVE_OSPFD */

/* OSPFv3.  */
#if defined (HAVE_OSPF6D) && defined(HAVE_IPV6)
CLI (mode_router_ipv6_ospf,
     mode_router_ipv6_ospf_cli,
     "router ipv6 ospf",
     "Enable a routing process",
     "Enable an IPv6 routing process",
     "Open Shortest Path First (OSPF)")
{
  cli_mode_change (cli, OSPF6_MODE);
  return CLI_SUCCESS;
}

CLI (mode_router_ipv6_vrf_ospf,
     mode_router_ipv6_vrf_ospf_cli,
     "router ipv6 vrf ospf WORD",
     "Enable a routing process",
     "Enable an IPv6 routing process",
     "Enable an IPv6 VRF routing process",
     CLI_OSPF6_STR,
     "VRF Name to associate with this instance")
{
  cli_mode_change (cli, OSPF6_MODE);
  return CLI_SUCCESS;
}

CLI (mode_router_ipv6_ospf_tag,
     mode_router_ipv6_ospf_tag_cli,
     "router ipv6 ospf WORD",
     "Enable a routing process",
     "Enable an IPv6 routing process",
     "Open Shortest Path First (OSPF)",
     "OSPFv3 process tag")
{
  cli_mode_change (cli, OSPF6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF6D && HAVE_IPV6 */

/* IS-IS.  */
#ifdef HAVE_ISISD
CLI (mode_router_isis,
     mode_router_isis_cli,
     "router isis (WORD|)",
     "Enable a routing process",
     "Intermediate System - Intermediate System (IS-IS)",
     "ISO routing area tag")
{
  cli_mode_change (cli, ISIS_MODE);
  return CLI_SUCCESS;
}

/* IS-IS IPv6.  */
#ifdef HAVE_IPV6
CLI (mode_isis_af_ipv6,
     mode_isis_af_ipv6_cli,
     "address-family ipv6 (unicast|)",
     "Enter Address Family command mode",
     "Address family",
     "Address Family modifier")
{
  cli_mode_change (cli, ISIS_IPV6_MODE);
  return CLI_SUCCESS;
}

CLI (mode_isis_exit_af,
     mode_isis_exit_af_cli,
     "exit-address-family",
     "Exit from Address Family configuration mode")
{
  cli_mode_change (cli, ISIS_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_ISISD */

#ifdef HAVE_BGPD
#ifndef HAVE_EXT_CAP_ASN
CLI (mode_router_bgp,
     mode_router_bgp_cli,
     "router bgp <1-65535>",
     CLI_ROUTER_STR,
     "Border Gateway Protocol (BGP)",
     CLI_AS_STR)
#else
CLI (mode_router_bgp,
     mode_router_bgp_cli,
     "router bgp <1-4294967295>",
     CLI_ROUTER_STR,
     "Border Gateway Protocol (BGP)",
     CLI_AS_STR)
#endif
{
  cli_mode_change (cli, BGP_MODE);
  return CLI_SUCCESS;
}
#ifndef HAVE_EXT_CAP_ASN
ALI (mode_router_bgp,
     mode_router_bgp_view_cli,
     "router bgp <1-65535> view WORD",
     CLI_ROUTER_STR,
     "Border Gateway Protocol (BGP)",
     CLI_AS_STR,
     "BGP view",
     "view name");
#else
ALI (mode_router_bgp,
     mode_router_bgp_view_cli,
     "router bgp <1-4294967295> view WORD",
     CLI_ROUTER_STR,
     "Border Gateway Protocol (BGP)",
     CLI_AS_STR,
     "BGP view",
     "view name");
#endif /* HAVE_EXT_CAP_ASN */
CLI (mode_bgp_af_ipv4,
     mode_bgp_af_ipv4_cli,
     "address-family ipv4",
     "Enter Address Family command mode",
     "Address family")
{
  cli_mode_change (cli, BGP_IPV4_MODE);
  return CLI_SUCCESS;
}

CLI (mode_bgp_af_ipv4_unicast,
     mode_bgp_af_ipv4_unicast_cli,
     "address-family ipv4 unicast",
     "Enter Address Family command mode",
     "Address family",
     "Address Family modifier")
{
  cli_mode_change (cli, BGP_IPV4_MODE);
  return CLI_SUCCESS;
}

CLI (mode_bgp_af_ipv4_multicast,
     mode_bgp_af_ipv4_multicast_cli,
     "address-family ipv4 multicast",
     "Enter Address Family command mode",
     "Address family",
     "Address Family modifier")
{
  cli_mode_change (cli, BGP_IPV4M_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (mode_bgp_af_ipv6,
     mode_bgp_af_ipv6_cli,
     "address-family ipv6 (unicast|)",
     "Enter Address Family command mode",
     "Address family",
     "Address Family modifier")
{
  cli_mode_change (cli, BGP_IPV6_MODE);
  return CLI_SUCCESS;
}
#ifdef HAVE_6PE
CLI (mode_bgp_af_ipv6_labeled_unicast,
     mode_bgp_af_ipv6_labeled_unicast_cli,
     "address-family ipv6 labeled-unicast",
     "Enter Address Family command mode",
     "Address family",
     "Address Family modifier")
{
  cli_mode_change (cli, BGP_6PE_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_6PE */
#endif /* HAVE_IPV6 */

#ifdef HAVE_VRF
CLI (mode_bgp_af_vpnv4,
     mode_bgp_af_vpnv4_cli,
     "address-family vpnv4",
     "Enter Address Family command mode",
     "Address family")
{
  cli_mode_change (cli, BGP_VPNV4_MODE);
  return CLI_SUCCESS;
}

CLI (mode_bgp_af_vpnv4_unicast,
     mode_bgp_af_vpnv4_unicast_cli,
     "address-family vpnv4 unicast",
     "Enter Address Family command mode",
     "Address family",
     "Address Family Modifier")
{
  cli_mode_change (cli, BGP_VPNV4_MODE);
  return CLI_SUCCESS;
}
#ifdef HAVE_IPV6 /*6VPE*/
CLI (mode_bgp_af_vpnv6,
     mode_bgp_af_vpnv6_cli,
     "address-family vpnv6",
     "Enter Address Family command mode",
     "Address family")
{
  cli_mode_change (cli, BGP_VPNV6_MODE);
  return CLI_SUCCESS;
}

CLI (mode_bgp_af_vpnv6_unicast,
     mode_bgp_af_vpnv6_unicast_cli,
     "address-family vpnv6 unicast",
     "Enter Address Family command mode",
     "Address family",
     "Address Family Modifier")
{
  cli_mode_change (cli, BGP_VPNV6_MODE);
  return CLI_SUCCESS;
}
CLI (mode_bgp_af_ipv6_vrf,
     mode_bgp_af_ipv6_vrf_cli,
     "address-family ipv6 vrf NAME",
     "Enter Address Family command mode",
     "Address family",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  cli_mode_change (cli, BGP_IPV6_VRF_MODE);
  return CLI_SUCCESS;
}
#endif /*HAVE_IPV6*/
#endif /* HAVE_VRF */

CLI (mode_bgp_af_ipv4_vrf,
     mode_bgp_af_ipv4_vrf_cli,
     "address-family ipv4 vrf NAME",
     "Enter Address Family command mode",
     "Address family",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
{
  cli_mode_change (cli, BGP_IPV4_VRF_MODE);
  return CLI_SUCCESS;
}

CLI (mode_bgp_exit_af,
     mode_bgp_exit_af_cli,
     "exit-address-family",
     "Exit from Address Family configuration mode")
{
  cli_mode_change (cli, BGP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_BGPD */

/* LDP.  */
#ifdef HAVE_LDPD
CLI (mode_router_ldp,
     mode_router_ldp_cli,
     "router ldp",
     CLI_ROUTER_STR,
     "Lable Distribution Protocol (LDP)")
{
  cli_mode_change (cli, LDP_MODE);
  return CLI_SUCCESS;
}

CLI (mode_ldp_add_targeted_peer,
     mode_ldp_add_targeted_peer_cli,
     "targeted-peer ipv4 A.B.C.D",
     "Specify a targeted LDP peer",
     CLI_IP_STR,
     "IPv4 address of the targeted-peer")
{
  cli_mode_change (cli, TARGETED_PEER_MODE);
  return CLI_SUCCESS;
}

CLI (mode_ldp_exit_tp,
     mode_ldp_exit_tp_cli,
     "exit-targeted-peer-mode",
     "Exit from Targeted Peer configuration mode")
{
  cli_mode_change (cli, LDP_MODE);
  return CLI_SUCCESS;
}


#ifdef HAVE_CR_LDP
CLI (mode_ldp_path,
     mode_ldp_path_cli,
     "ldp-path PATHNAME",
     "Path for Lable Distribution Protocol (LDP)",
     "Name to be used for path")
{
  cli_mode_change (cli, LDP_PATH_MODE);
  return CLI_SUCCESS;
}

CLI (mode_ldp_trunk,
     mode_ldp_trunk_cli,
     "ldp-trunk TRUNKNAME",
     "Trunk for Label Distribution Protocol (LDP)",
     "Name to be used for trunk")
{
  cli_mode_change (cli, LDP_TRUNK_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_CR_LDP */
#endif /* HAVE_LDPD */

/* RSVP. */
#ifdef HAVE_RSVPD
CLI (mode_router_rsvp,
     mode_router_rsvp_cli,
     "router rsvp",
     CLI_ROUTER_STR,
     "Resource Reservation Protocol (RSVP)")
{
  cli_mode_change (cli, RSVP_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_GMPLS
CLI (mode_rsvp_trunk,
     mode_rsvp_trunk_cli,
     "rsvp-trunk TRUNKNAME (ipv4|ipv6|gmpls|)",
     "Trunk for Resource Reservation Protocol (RSVP)",
     "Name to be used for trunk",
     "IPv4 address family trunk",
     "IPv6 address family trunk",
     "GMPLS enabled trunk")
#else
CLI (mode_rsvp_trunk,
     mode_rsvp_trunk_cli,
     "rsvp-trunk TRUNKNAME (ipv4|ipv6)",
     "Trunk for Resource Reservation Protocol (RSVP)",
     "Name to be used for trunk",
     "IPv4 address family trunk",
     "IPv6 address family trunk")
#endif /* HAVE_GMPLS */
{
  cli_mode_change (cli, RSVP_TRUNK_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_GMPLS
CLI (mode_rsvp_path,
     mode_rsvp_path_cli,
     "rsvp-path PATHNAME (mpls|gmpls|)",
     "Path for Resource Reservation Protocol (RSVP)",
     "Name to be used for path",
     "Path type is mpls",
     "Path type is gmpls")
#else
CLI (mode_rsvp_path,
     mode_rsvp_path_cli,
     "rsvp-path PATHNAME",
     "Path for Resource Reservation Protocol (RSVP)",
     "Name to be used for path")
#endif /* HAVE_GMPLS */
{
  cli_mode_change (cli, RSVP_PATH_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_MPLS_FRR
CLI (mode_rsvp_frr_bypass,
     mode_rsvp_frr_bypass_cli,
     "rsvp-bypass BYPASSNAME",
     "Bypass Tunnel For the RSVP",
     "Name of the Bypass Tunnel")
{
  cli_mode_change (cli, RSVP_BYPASS_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MPLS_FRR */
#endif /* HAVE_RSVPD */

#ifdef HAVE_VPLS
CLI (mode_mpls_vpls,
     mode_mpls_vpls_cli,
     "mpls vpls NAME <1-4294967295>",
     CONFIG_MPLS_STR,
     "Create an instance of MPLS based Virtual Private Lan Service (VPLS)",
     "Identifying string for VPLS",
     "Identifying value for VPLS")
{
  cli_mode_change (cli, NSM_VPLS_MODE);
  return CLI_SUCCESS;
}

CLI (mode_mpls_vpls_by_name,
     mode_mpls_vpls_by_name_cli,
     "mpls vpls NAME",
     CONFIG_MPLS_STR,
     "Create an instance of MPLS based Virtual Private Lan Service (VPLS)",
     "Identifying string for VPLS")
{
  cli_mode_change (cli, NSM_VPLS_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_VPLS */

/* VRRP.  */
#ifdef HAVE_VRRP
CLI (mode_router_vrrp,
     mode_router_vrrp_cli,
     "router vrrp <1-255> IFNAME",
     "Enable routing process",
     "Start VRRP configuration",
     "Virtual router identifier",
     "Interface name")
{
  cli_mode_change (cli, VRRP_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_CUSTOM1
CLI (mode_router_vrrp_vlan,
     mode_router_vrrp_vlan_cli,
     "router vrrp <1-255> vlan <1-4094>",
     "Enable routing process",
     "Start VRRP configuration",
     "Virtual router identifier",
     "VLAN interface",
     "VLAN id")
{
  cli_mode_change (cli, VRRP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_IPV6
CLI (mode_router_ipv6_vrrp,
     mode_router_ipv6_vrrp_cli,
     "router ipv6 vrrp <1-255> IFNAME",
     "Enable routing process",
     "Assume IPv6 address family",
     "Start VRRP configuration",
     "Virtual router identifier",
     "Interface name")
{
  cli_mode_change (cli, VRRP6_MODE);
  return CLI_SUCCESS;
}
#ifdef HAVE_CUSTOM1
CLI (mode_router_ipv6_vrrp_vlan,
     mode_router_ipv6_vrrp_vlan_cli,
     "router ipv6 vrrp <1-255> vlan <1-4094>",
     "Enable routing process",
     "Assume IPv6 address family",
     "Start VRRP configuration",
     "Virtual router identifier",
     "VLAN interface",
     "VLAN id")
{
  cli_mode_change (cli, VRRP6_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_CUSTOM1 */
#endif /*HAVE_IPV6 */


/* Perl script ignores commands with this syntax. */
IMI_ALI (NULL,
   imish_vrrp_enable_cli,
   "enable",
   "Enable VRRP session");

IMI_ALI (NULL,
   imish_vrrp_disable_cli,
   "disable",
   "Disable VRRP session");
#endif /* HAVE_VRRP */

#ifdef HAVE_LMP
CLI (mode_lmp_configuration,
     mode_lmp_configuration_cli,
     "lmp-configuration",
     "LMP Configuration")
{
  cli_mode_change (cli, LMP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_LMP */

#ifdef HAVE_QOS
CLI (mode_qos_class_map,
     mode_qos_class_map_cli,
     "class-map NAME",
     "Class map command",
     "Specify a class-map name")
{
  cli_mode_change (cli, CMAP_MODE);
  return CLI_SUCCESS;
}

CLI (mode_qos_policy_map,
     mode_qos_policy_map_cli,
     "policy-map NAME",
     "Policy map command",
     "Specify a policy-map name")
{
  cli_mode_change (cli, PMAP_MODE);
  return CLI_SUCCESS;
}

CLI (mode_qos_pmap_class,
     mode_qos_pmap_class_cli,
     "class NAME",
     "Specify class-map",
     "Specify class map name")
{
  cli_mode_change (cli, PMAPC_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_QOS */

#ifdef HAVE_VLAN
CLI (vlan_database,
     vlan_database_cli,
     "vlan database",
     "Configure VLAN parameters",
     "Configure VLAN database")
{
  cli_mode_change (cli, VLAN_MODE);
  return CLI_SUCCESS;
}


CLI (cvlan_reg_tab,
     cvlan_reg_tab_cli,
     "cvlan registration table WORD",
     "Configure C-VLAN parameters",
     "Configure C-VLAN Registration parameters",
     "Configure C-VLAN Registration table",
     "Configure C-VLAN Registration table name")
{
  cli_mode_change (cli, CVLAN_REGIST_MODE);
  return CLI_SUCCESS;
}

CLI (cvlan_reg_tab_br,
     cvlan_reg_tab_br_cli,
     "cvlan registration table WORD bridge <1-32>",
     "Configure C-VLAN parameters",
     "Configure C-VLAN Registration parameters",
     "Configure C-VLAN Registration table",
     "Configure C-VLAN Registration table name",
     "Bridge group commands",
     "Bridge group for bridging")
{
  cli_mode_change (cli, CVLAN_REGIST_MODE);
  return CLI_SUCCESS;
}

CLI (vlan_access_map,
     vlan_access_map_cli,
     "vlan access-map WORD <1-65535>",
     "Configure VLAN parameters",
     "Configure VLAN Access Map",
     "Name for the VLAN Access Map",
     "Sequence to insert to/delete from existing access-map entry")
{
  cli_mode_change (cli, VLAN_ACCESS_MODE);
  return CLI_SUCCESS;
}

/* WHY IS IT HERE ????? */
CLI (vlan_no_access_map,
     vlan_no_access_map_cli,
     "no vlan access-map WORD <1-65535>",
     CLI_NO_STR,
     "Configure VLAN parameters",
     "Configure VLAN Access Map",
     "Name for the VLAN Access Map",
     "Sequence to insert to/delete from existing access-map entry")
{
  return CLI_SUCCESS;
}

#endif /* HAVE_VLAN */
#ifdef HAVE_CFM
CLI (eth_cfm_domain,
     eth_cfm_domain_cli,
     "ethernet cfm domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name DOMAIN_NAME)) level LEVEL mip-creation (none|default|explicit)(bridge <1-32>|)",
     "configure layer-2",
     "configure cfm",
     "maintanance domain",
     "maintenance domain name type",
     "MD name type is no Maintenace Domain Name",
     "Name of the domain",
     "Enter the md name as no_name",
     "MD name type is Domain Name based string",
     "Name of the domain",
     "Enter the dns based domain name",
     "MD name type is Character string",
     "Name of the domain",
     "Enter the character string based domain name",
     "The level of the md",
     "specify the md level",
     "mip creation permissions for the domain",
     "no mip can be created for this vid",
     "MIP can be created if no lower active level or MEP at next lower active level",
     "MEP is needed at the next lower active level",
     "Bridge group commands",
     "Bridge group for bridging")
{
  cli_mode_change (cli, ETH_CFM_MODE);
  return CLI_SUCCESS;
}

CLI (eth_cfm_domain_mac,
     eth_cfm_domain_mac_cli,
     "ethernet cfm domain-name type mac name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit)(bridge <1-32>|)",
     "configure layer-2",
     "configure cfm",
     "maintanance domain",
     "maintenance domain name type",
     "MD name type is mac address",
     "Name of the domain",
     "Enter the domain name in HHHH.HHHH.HHHH:2-octet integer format",
     "The level of the md",
     "specify the md level",
     "mip creation permissions for the domain",
     "no mip can be created for this vid",
     "MIP can be created if no lower active level or MEP at next lower active level",
     "MEP is needed at the next lower active level",
     "Bridge group commands",
     "Bridge group for bridging")
{
  cli_mode_change (cli, ETH_CFM_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_CFM_Y1731
CLI (eth_cfm_domain_itut,
     eth_cfm_domain_itut_cli,
     "ethernet cfm domain-name type itu-t name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit)(bridge <1-32>|)",
     "configure layer-2",
     "configure cfm",
     "maintanance domain",
     "maintenance domain name type",
     "ITU-T Carrier code (ICC) based format",
     "Name of the domain",
     "Enter the ICC based domain name",
     "The level of the md",
     "specify the md level",
     "mip creation permissions for the domain",
     "no mip can be created for this vid",
     "MIP can be created if no lower active level or MEP at next lower active level",
     "MEP is needed at the next lower active level",
     "Bridge group commands",
     "Bridge group for bridging")
{
  cli_mode_change (cli, ETH_CFM_MODE);
  return CLI_SUCCESS;
}

#endif /* HAVE_CFM_Y1731 */

/* IEEE 802.1ag 2007 */
/* This mode is being introduced to configure secondary vids for a vlan
 * identified by a primary-vid */
CLI (eth_cfm_vlan,
     eth_cfm_vlan_cli,
     "ethernet cfm configure vlan PRIMARY_VID ((bridge <1-32>| backbone)|)",
     "configure layer-2",
     "configure cfm",
     "configure vids for vlan",
     "vlan identified by the primary-vid",
     "Enter the primary vid identifying this vlan",
     "Bridge group commands",
     "Bridge group for bridging",
     "backbone bridge")
{
  cli_mode_change (cli, ETH_CFM_VLAN_MODE);
  return CLI_SUCCESS;
}
#endif

#ifdef HAVE_G8031
#ifdef HAVE_B_BEB
#ifdef HAVE_I_BEB
CLI (g8031_configure_i_b_beb_eps_group_create,
    g8031_configure_i_b_beb_eps_group_create_cmd,
    "g8031 configure switching eps-id EPS_ID bridge (<1-32> | backbone)",
    "g8031 realted parameters",
    "configure EPS protection parameters",
    "EPS switching parameters",
    "unique id to identify a EPS protection link",
    "EPS protection group ID"
    "Bridge group",
    "Bridge group name",
    "backbone bridge")
{
    cli_mode_change (cli, G8031_CONFIG_SWITCHING_MODE);
    return CLI_SUCCESS;
}

CLI (g8031_i_b_beb_eps_vlan_create,
    g8031_i_b_beb_eps_vlan_create_cmd,
    "g8031 configure vlan eps-id EPS-ID bridge (<1-32> | backbone)",
    "parameters are G.8031 related",
    "EPS configuration",
    "configure vlan members",
    "unique id to identify a EPS protection link",
    "EPS protection group ID",
    "Bridge group",
    "Bridge group name",
    "backbone bridge")

{
    cli_mode_change (cli, G8031_CONFIG_VLAN_MODE);
    return CLI_SUCCESS;
}
#else
CLI (g8031_configure_beb_eps_group_create,
     g8031_configure_beb_eps_group_create_cmd,
     "g8031 configure switching eps-id EPS_ID bridge backbone",
     "g8031 realted parameters",
     "configure EPS protection parameters",
     "EPS switching parameters",
     "unique id to identify a EPS protection link",
     "EPS protection group ID",
     "Bridge group",
     "backbone bridge")
{
  cli_mode_change (cli, G8031_CONFIG_SWITCHING_MODE);
  return CLI_SUCCESS;
}

CLI (g8031_beb_eps_vlan_create,
     g8031_beb_eps_vlan_create_cmd,
     "g8031 configure vlan eps-id EPS-ID bridge backbone",
     "parameters are G.8031 related",
     "EPS configuration",
     "configure vlan members",
     "unique id to identify a EPS protection link",
     "EPS protection group ID",
     "Bridge group",
     "backbone bridge")
{
  cli_mode_change (cli, G8031_CONFIG_VLAN_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_I_BEB */
#else
CLI (g8031_configure_eps_group_create,
     g8031_configure_eps_group_create_cmd,
     "g8031 configure switching eps-id EPS_ID bridge <1-32>",
     "g8031 realted parameters",
     "configure EPS protection parameters",
     "EPS switching parameters",
     "unique id to identify a EPS protection link",
     "EPS protection group ID",
     "Bridge group",
     "Bridge group name")
{
  cli_mode_change (cli, G8031_CONFIG_SWITCHING_MODE);
  return CLI_SUCCESS;
}

CLI (g8031_eps_vlan_create,
     g8031_eps_vlan_create_cmd,
     "g8031 configure vlan eps-id EPS-ID bridge <1-32>",
     "parameters are G.8031 related",
     "EPS configuration",
     "configure vlan members",
     "unique id to identify a EPS protection link",
     "EPS protection group ID",
     "Bridge group",
     "Bridge group name")

{
  cli_mode_change (cli, G8031_CONFIG_VLAN_MODE);
  return CLI_SUCCESS;
}
#endif /*  defined HAVE_B_BEB  */

CLI (g8031_config_switching,
     g8031_config_switching_cmd,
     "g8031 configure switching",
     "g8031 realted parameters",
     "configure EPS protection parameters",
     "EPS switching parameters")
{
  cli_mode_change (cli, G8031_CONFIG_SWITCHING_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_G8031 */

#ifdef HAVE_CFM
#if (defined HAVE_I_BEB || defined HAVE_B_BEB)
/* With this CLI control for cfm pbb should go to the ETH_CFM_PBB_MODE
 * The string for the cli has to be same as the one in lib/cli.pl.
 * The str is exceeding 80 words per line as if the cli is split and given
 * the perl script is unable to recognize it */
CLI (eth_cfm_pbb_domain,
     eth_cfm_pbb_domain_cli,
     "ethernet cfm pbb domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name DOMAIN_NAME)) pbb-domain-type (svid|service|bvlan|link) level LEVEL (mip-creation (none|default|explicit)|) (bridge <1-32> | backbone) ((p-enni | h-enni)|)",
     "configure layer-2",
     "configure cfm",
     "configure the cfm for pbb",
     "maintenance domain name",
     "type of the maintenance domain",
     "MD name type is no Maintenace Domain Name",
     "Name of the domain",
     "Enter the md name as no_name",
     "MD name type is Domain Name based string",
     "Name of the domain",
     "Enter the dns based domain name",
     "MD name type is Character string",
     "Name of the domain",
     "Enter the character string based domain name",
     "specify the domain-type for cfm in pbb",
     "SVID domain",
     "Service Instance domain",
     "B-VLAN domain",
     "Link Level domain",
     "The level of the md",
     "specify the md level",
     "mip creation permissions for the domain",
     "no mip can be created for this vid",
     "MIP can be created if no lower active level or MEP at next lower active level",
     "MEP is needed at the next lower active level",
     "Bridge group commands",
     "Bridge Group name for bridging",
     "backbone bridge",
     "peer-extened network-network interface",
     "hierarchal-extened network-network interface")
{
  cli_mode_change (cli, ETH_CFM_PBB_MODE);
  return CLI_SUCCESS;
}
CLI (eth_cfm_pbb_domain_mac,
     eth_cfm_pbb_domain_mac_cli,
     "ethernet cfm pbb domain-name type mac name DOMAIN_NAME pbb-domain-type (svid|service|bvlan|link) level LEVEL mip-creation (none|default|explicit) (bridge <1-32> | backbone) ((p-enni | h-enni)|)",
     "configure layer-2",
     "configure cfm",
     "configure cfm for pbb",
     "maintanance domain",
     "maintenance domain name type",
     "MD name type is mac address",
     "Name of the domain",
     "Enter the domain name in HHHH.HHHH.HHHH:2-octet integer format",
     "specify the domain-type for cfm in pbb",
     "SVID domain",
     "Service Instance domain",
     "B-VLAN domain",
     "Link Level domain",
     "The level of the md",
     "specify the md level",
     "mip creation permissions for the domain",
     "no mip can be created for this vid",
     "MIP can be created if no lower active level or MEP at next lower active level",
     "MEP is needed at the next lower active level",
     "Bridge group commands",
     "Bridge group for bridging",
     "backbone bridge",
     "peer-extened network-network interface",
     "hierarchal-extened network-network interface")
{
  cli_mode_change (cli, ETH_CFM_PBB_MODE);
  return CLI_SUCCESS;
}
#ifdef HAVE_CFM_Y1731
CLI (eth_cfm_pbb_domain_itut,
     eth_cfm_pbb_domain_itut_cli,
     "ethernet cfm pbb domain-name type itut-t name DOMAIN_NAME pbb-domain-type (svid|service|bvlan|link) level LEVEL mip-creation (none|default|explicit)(bridge <1-32> | backbone) ((p-enni | h-enni)|)",
     "configure layer-2",
     "configure cfm",
     "configure cfm for pbb",
     "maintanance domain",
     "maintenance domain name type",
     "ITU-T Carrier code (ICC) based format",
     "Name of the domain",
     "Enter the ICC based domain name",
     "specify the domain-type for cfm in pbb",
     "SVID domain",
     "Service Instance domain",
     "B-VLAN domain",
     "Link Level domain",
     "The level of the md",
     "specify the md level",
     "mip creation permissions for the domain",
     "no mip can be created for this vid",
     "MIP can be created if no lower active level or MEP at next lower active level",
     "MEP is needed at the next lower active level",
     "Bridge group commands",
     "Bridge group for bridging",
     "backbone bridge",
     "peer-extened network-network interface",
     "hierarchal-extened network-network interface")
{
  cli_mode_change (cli, ETH_CFM_PBB_MODE);
  return CLI_SUCCESS;
}
#endif /*HAVE_CFM_Y1731 */
#endif /* (HAVE_I_BEB || HAVE_B_BEB)  */
#endif /* HAVE_CFM */

#ifdef HAVE_I_BEB
CLI (pbb_isid_list,
     pbb_isid_list_cmd,
     "pbb isid list",
     "pbb related commands",
     "isid related parameters",
     "isid configuration mode")
{
  cli_mode_change (cli, PBB_ISID_CONFIG_MODE);
  return CLI_SUCCESS;
}

#endif /* HAVE_I_BEB */

#if defined HAVE_I_BEB && defined HAVE_B_BEB && defined HAVE_PBB_TE
CLI (pbb_te_config_tesid,
     pbb_te_config_tesid_cmd,
     "pbb-te configure tesid TESID (name TRUNK_NAME|)",
     "pbb-te related commands",
     "pbb-te configuration options ",
     "tesid value",
     "<1-42947295>",
     "pbb-te trunk name for the identifier",
     "trunk name",
     "tesid value",
     "<1-42947295>")
{
  cli_mode_change (cli, PBB_TE_VLAN_CONFIG_MODE);
  return CLI_SUCCESS;
}

CLI (pbb_te_esp_cbp_if_config,
     pbb_te_esp_cbp_if_config_cmd,
     "pbb-te configure esp tesi TESID",
     "pbb-te esp related command",
     "configure esp in the esp if-config mode",
     "configure esps",
     "tesi for which esps will be configured",
     "enter the TESI for which ESPs must be configured")
{
  cli_mode_change (cli, PBB_TE_ESP_IF_CONFIG_MODE);
  return CLI_SUCCESS;
}

CLI (pbb_te_esp_if_exit,
    pbb_te_esp_if_exit_cmd,
    "exit-pbb-te-esp-mode",
    "exit for the PBB_TE_ESP_IF_CONFIG_MODE")
{
  cli_mode_change (cli, INTERFACE_MODE);
  return CLI_SUCCESS;

}
#endif /* HAVE_I_BEB && HAVE_B_BEB && HAVE_PBB_TE */

#ifdef HAVE_CFM
#ifdef HAVE_PBB_TE
#if defined HAVE_I_BEB && defined HAVE_B_BEB
CLI (eth_cfm_pbb_te_domain,
    eth_cfm_pbb_te_domain_cli,
    "ethernet cfm pbb-te domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name  DOMAIN_NAME) | mac name DOMAIN_NAME) level LEVEL mip-creation (none|default|explicit) bridge backbone",
    "configure layer-2",
    "configure cfm",
    "configure cfm for pbb-te",
    "maintanance domain",
    "maintenance domain name type",
    "MD name type is no Maintenace Domain Name",
    "Name of the domain",
    "Enter the md name as no_name",
    "MD name type is Domain Name based string",
    "Name of the domain",
    "Enter the dns based domain name",
    "MD name type is Character string",
    "Name of the domain",
    "Enter the character string based domain name",
    "MD name type is mac address",
    "Name of the domain",
    "Enter the domain name in HHHH.HHHH.HHHH:2-octet integer format",
    "The level of the md",
    "specify the md level",
    "mip creation permissions for the domain",
    "no mip can be created for this vid",
    "MIP can be created if no lower active level or MEP at next lower active level",
    "MEP is needed at the next lower active level",
    "bridge group command",
    "backbone bridge")
{
   cli_mode_change (cli, ETH_CFM_PBB_TE_MODE);
   return CLI_SUCCESS;
}
#ifdef HAVE_CFM_Y1731
CLI (eth_cfm_pbb_te_domain_itu_t,
    eth_cfm_pbb_te_domain_itu_t_cli,
    "ethernet cfm pbb-te domain-name type itu-t name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit) bridge backbone",
    "configure layer-2",
    "configure cfm",
    "configure cfm for pbb-te",
    "maintanance domain",
    "maintenance domain name type",
    "ITU-T Carrier code (ICC) based format",
    "Name of the domain",
    "Enter the ICC based domain name",
    "The level of the md",
    "specify the md level",
    "mip creation permissions for the domain",
    "no mip can be created for this vid",
    "MIP can be created if no lower active level or MEP at next lower active level",
    "MEP is needed at the next lower active level",
    "bridge group command",
    "backbone bridge")
{
   cli_mode_change (cli, ETH_CFM_PBB_TE_MODE);
   return CLI_SUCCESS;
}

#endif /*HAVE_CFM_Y1731 */
#else
CLI (eth_cfm_pbb_te_domain,
    eth_cfm_pbb_te_domain_cli,
    "ethernet cfm pbb-te domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name  DOMAIN_NAME) | mac name DOMAIN_NAME) level LEVEL mip-creation (none|default|explicit) bridge <1-32>",
    "configure layer-2",
    "configure cfm",
    "configure cfm for pbb-te",
    "maintanance domain",
    "maintenance domain name type",
    "MD name type is no Maintenace Domain Name",
    "Name of the domain",
    "Enter the md name as no_name",
    "MD name type is Domain Name based string",
    "Name of the domain",
    "Enter the dns based domain name",
    "MD name type is Character string",
    "Name of the domain",
    "Enter the character string based domain name",
    "MD name type is mac address",
    "Name of the domain",
    "Enter the domain name in HHHH.HHHH.HHHH:2-octet integer format",
    "The level of the md",
    "specify the md level",
    "mip creation permissions for the domain",
    "no mip can be created for this vid",
    "MIP can be created if no lower active level or MEP at next lower active level",
    "MEP is needed at the next lower active level",
    "bridge group command",
    "bridge group name for bridging")
{
   cli_mode_change (cli, ETH_CFM_PBB_TE_MODE);
   return CLI_SUCCESS;
}
#ifdef HAVE_CFM_Y1731
CLI (eth_cfm_pbb_te_domain_itu_t,
    eth_cfm_pbb_te_domain_itu_t_cli,
    "ethernet cfm pbb-te domain-name type itu-t name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit) bridge <1-32>",
    "configure layer-2",
    "configure cfm",
    "configure cfm for pbb-te",
    "maintanance domain",
    "maintenance domain name type",
    "ITU-T Carrier code (ICC) based format",
    "Name of the domain",
    "Enter the ICC based domain name",
    "The level of the md",
    "specify the md level",
    "mip creation permissions for the domain",
    "no mip can be created for this vid",
    "MIP can be created if no lower active level or MEP at next lower active level",
    "MEP is needed at the next lower active level",
    "bridge group command",
    "bridge group name for bridging")
{
   cli_mode_change (cli, ETH_CFM_PBB_TE_MODE);
   return CLI_SUCCESS;
}

#endif /* HAVE_CFM_Y1731 */
#endif /* HAVE_I_BEB && HAVE_B_BEB */
#endif /* HAVE_PBB_TE */

#endif /* HAVE_CFM */

#if ((defined HAVE_CFM)&& (defined HAVE_I_BEB) && (defined HAVE_B_BEB) && \
         (defined HAVE_PBB_TE))

CLI (pbb_te_config_switching_mode,
     pbb_te_config_switching_mode_cmd,
     "pbb-te configure switching cbp IFNAME aps-group APS-ID",
     "PBB-TE related parameters",
     "APS protection parameter configuration",
     "APS switching parameters",
     "Indicates a CBP port",
     "Identifies the CBP port interface name",
     "unique group id to identify a APS protection set",
     "APS protection group ID")
{
  cli_mode_change (cli, ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE);
  return CLI_SUCCESS;
}

CLI (pbb_te_exit_config_switching_mode,
     pbb_te_exit_config_switching_mode_cmd,
     "exit-pbb-te-switching-mode",
     "exit from pbb-te config switching mode")
{
  cli_mode_change (cli, INTERFACE_MODE);
  return CLI_SUCCESS;

}

#endif /* #if ((defined HAVE_CFM)&& (defined HAVE_I_BEB) && (defined HAVE_B_BEB) && (defined HAVE_PBB_TE))*/

#ifdef HAVE_MSTPD
CLI (mode_spanning_tree_mst_config,
     mode_spanning_tree_mst_config_cli,
     "spanning-tree mst configuration",
     "Spanning tree commands",
     "Multiple spanning tree",
     "Configuration")
{
  cli_mode_change (cli, MST_CONFIG_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_MSTPD */
#ifdef HAVE_RPVST_PLUS
CLI (mode_spanning_tree_rpvst_plus_config,
     mode_spanning_tree_rpvst_plus_config_cli,
     "spanning-tree rpvst+ configuration",
     "Spanning tree commands",
     "Per Vlan rapid spanning tree",
     "Configuration")
{
  cli_mode_change (cli, RPVST_PLUS_CONFIG_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_RPVST_PLUS */
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
CLI (mode_spanning_tree_te_msti_config,
     mode_spanning_tree_te_msti_config_cli,
     "spanning-tree te-msti configuration",
     "Spanning tree commands",
     "Specifies the MSTI to be the traffic engineering MSTI instance",
     "Configuration")
{
  cli_mode_change (cli, TE_MSTI_CONFIG_MODE);
  return CLI_SUCCESS;
}
#endif /*#if defined (HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/

#ifdef HAVE_IPSEC
/* ipsec mode changes --crypto map and transform mode */
CLI (crypto_map_ipsec_manual,
     crypto_map_ipsec_manual_cmd,
     "crypto map MAP-NAME SEQ-NUM ipsec-manual",
     "Security Specific Commands",
     "Map used to setup IPsec SA",
     "Crypto Map Name",
     "Sequence no to identify a particular crypto map",
     "IPsec Manual Security Association configuration")
{
  cli_mode_change (cli, CRYPTOMAP_MODE);
  return CLI_SUCCESS;
}

CLI (crypto_ipsec_isakmp_policy,
     crypto_ipsec_isakmp_policy_cmd,
     "crypto map MAP-NAME SEQ-NUM ipsec-isakmp",
     "Security Specific Commands",
     "Map used to setup IPsec SA",
     "Crypto Map Name",
     "Sequence no to identify a particular crypto map",
     "IPsec Manual Security Association configuration",
     "Policy name")
{
  cli_mode_change (cli, CRYPTOMAP_MODE);
  return CLI_SUCCESS;
}
CLI (crypto_ipsec_transform_set_ah,
     crypto_ipsec_transform_set_ah_cmd,
     "crypto ipsec transform-set NAME ah"
     "(None|ah-md5|ah-sha1)",
     "Security Specific Commands",
     "IPsec Security Protocol Specific Commands",
     "Set the Transform Set Name",
     "Set IPsec security protocols and algorithms",
     "Set AH Authentication Algorithm",
     "No Authentication",
     "MD5 Authentication Algorithm",
     "Alternative SHA1 Authentication Algorithm")
{
  cli_mode_change (cli, TRANSFORM_MODE);
  return CLI_SUCCESS;
}

CLI (crypto_ipsec_transform_set_esp_esp,
     crypto_ipsec_transform_set_esp_cmd,
     "crypto ipsec transform-set NAME esp-auth"
     "(None|esp-md5|esp-sha1) esp-enc"
     "(esp-null|esp-des|esp-3des|esp-aes|esp-blf|esp-cast)",
     "Security Specific Commands",
     "IPsec Security Protocol Specific Commands",
     "Set IPsec security protocols and algorithms",
     "Set ESP Authentication Algorithm",
     "No Authentication Algorithm",
     "MD5 Authentication Algorithm",
     "No Authentication",
     "MD5 Authentication Algorithm",
     "Alternative SHA1 Authentication Algorithm",
     "Set ESP Encryption Algorithm",
     "No Encryption",
     "DES Algorithm",
     "Alternative 3DES Algorithm",
     "Alternative AES Algorithm",
     "Alternative BlowFish Algorithm",
     "Alternative Cast Algorithm")
{
  cli_mode_change (cli, TRANSFORM_MODE);
  return CLI_SUCCESS;
}
CLI (nsm_crypto_isakmp_policy,
     nsm_crypto_isakmp_policy_cmd,
     "crypto isakmp policy PRIORITY",
     "Security Specific Commands",
     "ISAKMP commands",
     "Security policy to be applied",
     "Priority of the policy")
{
  cli_mode_change (cli, ISAKMP_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_IPSEC */
#ifdef HAVE_FIREWALL
CLI (firewall_group,
     firewall_group_cmd,
     "firewall group <1-30>",
     "Firewall commands",
     "Firewall Group",
     "Firewall Group no")
{
  cli_mode_change (cli, FIREWALL_MODE);
  return CLI_SUCCESS;
}
#endif /* HAVE_FIREWALL */

CLI (service_instance_bw_profile,
     service_instance_bw_profile_cli,
     "service instance INSTANCE_ID evc-id EVC_ID",
     "evc service to which the bw profile parameters need to be configured",
     "instance id per evc",
     "enter the instance id that should be mapped for the evc",
     "evc-id of the svlan",
     "enter the EVC ID of the svlan")
{
  cli_mode_change (cli, EVC_SERVICE_INSTANCE_MODE);
  return CLI_SUCCESS;
}

CLI (nsm_exit_service_instance_bw_profile,
     nsm_exit_service_instance_bw_profile_cmd,
     "exit-service-instance-mode",
     "exit from EVC_SERVICE_INSTANCE_MODE")
{
  cli_mode_change (cli, INTERFACE_MODE);
  return CLI_SUCCESS;
}

#ifdef HAVE_GMPLS
CLI (mode_gmpls_te_link,
     mode_gmpls_te_link_cli,
     "te-link TLNAME local-link-id A.B.C.D (numbered |)",
     CLI_TE_LINK_STR,
     "TE Link Name",
     CLI_LOCAL_LINK_ID_STR,
     "Local Link-ID in IPV4 Format",
     "Link Type Numbered")
{
  cli_mode_change (cli, TELINK_MODE);
  return CLI_SUCCESS;
}

CLI (mode_gmpls_te_link_by_name,
     mode_gmpls_te_link_by_name_cli,
     "te-link TLNAME",
     CLI_TE_LINK_STR,
     "TE Link Name")
{
  cli_mode_change (cli, TELINK_MODE);
  return CLI_SUCCESS;
}

CLI (mode_gmpls_ctrl_channel,
     mode_gmpls_ctrl_channel_cli,
     "control-channel CCNAME cc-id <1-4294967295> local-address A.B.C.D peer-address A.B.C.D",
     CLI_CTRL_CHNL_STR,
     "Control Channel Name",
     CLI_CTRL_CHNL_PARAM_STR,
     "Control Channel ID Value",
     CLI_CTRL_CHNL_PARAM_STR,
     "Control Channel Local Address in IPv4 Address Formart",
     CLI_CTRL_CHNL_PARAM_STR,
     "Control Channel Peer Address in IPv4 Address Formart")
{
  cli_mode_change (cli, CTRL_CHNL_MODE);
  return CLI_SUCCESS;
}

CLI (mode_gmpls_ctrl_channel_by_name,
     mode_gmpls_ctrl_channel_by_name_cli,
     "control-channel CCNAME",
     CLI_CTRL_CHNL_STR,
     "Control Channel Name")
{
  cli_mode_change (cli, CTRL_CHNL_MODE);
  return CLI_SUCCESS;
}

CLI (mode_gmpls_ctrl_adj,
     mode_gmpls_ctrl_adj_cli,
     "control-adjacency CADJNAME peer-address A.B.C.D (static |using-lmp |)",
     CLI_CTRL_ADJ_STR,
     "Control Adjacency Name",
     CLI_CTRL_ADJ_PARAM_STR,
     "Control Adjacency Peer Address in IPv4 Address Format",
     "Static Configuration of Control Adjancecny",
     "Control Adjacency managed using LMP")
{
  cli_mode_change (cli, CTRL_ADJ_MODE);
  return CLI_SUCCESS;
}

CLI (mode_gmpls_ctrl_adj_by_name,
     mode_gmpls_ctrl_adj_by_name_cli,
     "control-adjacency CADJNAME",
     CLI_CTRL_ADJ_STR,
     "Control Adjacency Name")
{
  cli_mode_change (cli, CTRL_ADJ_MODE);
  return CLI_SUCCESS;
}
#endif /*HAVE_GMPLS */

void
cli_mode_init (struct cli_tree *ctree)
{
  /* EXEC mode.  */
  cli_install_default (ctree, EXEC_MODE);

  /* CONFIGURE mode.  */
  cli_install_default (ctree, CONFIG_MODE);

#ifdef HAVE_VR
  /* VR. */
  cli_install_default (ctree, VR_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &mode_virtual_router_cli);
  cli_set_imi_cmd (&mode_virtual_router_cli, VR_MODE, CFG_DTYP_VR);
#endif /* HAVE_VR */

  /* VRF. */
#ifdef HAVE_VRF
  cli_install_default (ctree, VRF_MODE);
  cli_install_imi (ctree, CONFIG_MODE, modbmap_vor(2, &PM_NSM, &PM_BGP), PRIVILEGE_VR_MAX, 0,
                   &mode_ip_vrf_cli);
  cli_set_imi_cmd (&mode_ip_vrf_cli, VRF_MODE, CFG_DTYP_VRF);
#endif /* HAVE_VRF */

  /* RIP. */
#ifdef HAVE_RIPD
  cli_install_default (ctree, RIP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RIP, PRIVILEGE_VR_MAX, 0,
                   &mode_router_rip_cli);
  cli_set_imi_cmd (&mode_router_rip_cli, RIP_MODE, CFG_DTYP_RIP);

#ifdef HAVE_VRF_RIP
  cli_install_default_family (ctree, RIP_VRF_MODE);
  cli_install_imi (ctree, RIP_MODE, PM_RIP, PRIVILEGE_VR_MAX, 0,
                   &mode_rip_af_ipv4_vrf_cli);
  cli_set_imi_cmd (&mode_rip_af_ipv4_vrf_cli, RIP_MODE, CFG_DTYP_RIP);

  cli_install_imi (ctree, RIP_VRF_MODE, PM_RIP, PRIVILEGE_VR_MAX, 0,
                   &mode_rip_exit_af_cli);
  cli_set_imi_cmd (&mode_rip_exit_af_cli, RIP_MODE, CFG_DTYP_RIP);
#endif /* HAVE_VRF_RIP */
#endif /* HAVE_RIPD */

  /* RIPng. */
#if defined (HAVE_RIPNGD) && defined(HAVE_IPV6)
  cli_install_default (ctree, RIPNG_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RIPNG, PRIVILEGE_VR_MAX, 0,
                   &mode_router_ipv6_rip_cli);
  cli_set_imi_cmd (&mode_router_ipv6_rip_cli, RIPNG_MODE, CFG_DTYP_RIPNG);

#ifdef HAVE_VRF_RIPNG
  cli_install_default_family (ctree, RIPNG_VRF_MODE);

  cli_install_imi (ctree, RIPNG_MODE ,PM_RIPNG, PRIVILEGE_VR_MAX, 0,
                   &mode_rip_af_ipv6_vrf_cli);
  cli_set_imi_cmd (&mode_rip_af_ipv6_vrf_cli, RIPNG_VRF_MODE, CFG_DTYP_RIPNG);

  cli_install_imi (ctree, RIPNG_VRF_MODE, PM_RIPNG, PRIVILEGE_VR_MAX, 0,
                   &mode_ripng_exit_af_cli);
  cli_set_imi_cmd (&mode_ripng_exit_af_cli, RIPNG_MODE, CFG_DTYP_RIPNG);
#endif
#endif /* HAVE_RIPNGD && HAVE_IPV6 */

  /* OSPF. */
#ifdef HAVE_OSPFD
  cli_install_default (ctree, OSPF_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_OSPF, PRIVILEGE_VR_MAX, 0,
                   &mode_router_ospf_cli);
  cli_set_imi_cmd (&mode_router_ospf_cli, OSPF_MODE, CFG_DTYP_OSPF);

  cli_install_imi (ctree, CONFIG_MODE, PM_OSPF, PRIVILEGE_VR_MAX, 0,
                   &mode_router_ospf_id_cli);
  cli_set_imi_cmd (&mode_router_ospf_id_cli, OSPF_MODE, CFG_DTYP_OSPF);

#ifdef HAVE_VRF_OSPF
  cli_install_imi (ctree, CONFIG_MODE, PM_OSPF, PRIVILEGE_VR_MAX, 0,
                   &mode_router_ospf_id_vrf_cli);
  cli_set_imi_cmd (&mode_router_ospf_id_vrf_cli, OSPF_MODE, CFG_DTYP_OSPF);
#endif /* HAVE_VRF_OSPF */
#endif /* HAVE_OSPFD */

  /* OSPFv3.  */
#if defined (HAVE_OSPF6D) && defined(HAVE_IPV6)

  cli_install_default (ctree, OSPF6_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_OSPF6, PRIVILEGE_VR_MAX, 0,
                   &mode_router_ipv6_ospf_cli);
  cli_set_imi_cmd (&mode_router_ipv6_ospf_cli, OSPF6_MODE, CFG_DTYP_OSPF6);

  cli_install_imi (ctree, CONFIG_MODE, PM_OSPF6, PRIVILEGE_VR_MAX, 0,
                   &mode_router_ipv6_ospf_tag_cli);
  cli_set_imi_cmd (&mode_router_ipv6_ospf_tag_cli, OSPF6_MODE, CFG_DTYP_OSPF6);
  cli_install_imi (ctree, CONFIG_MODE, PM_OSPF6, PRIVILEGE_VR_MAX, 0,
                   &mode_router_ipv6_vrf_ospf_cli);
  cli_set_imi_cmd (&mode_router_ipv6_vrf_ospf_cli, OSPF6_MODE, CFG_DTYP_OSPF6);

#endif /* HAVE_OSPF6D && HAVE_IPV6 */

  /* IS-IS.  */
#ifdef HAVE_ISISD
  cli_install_default (ctree, ISIS_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ISIS, PRIVILEGE_VR_MAX, 0,
                   &mode_router_isis_cli);
  cli_set_imi_cmd (&mode_router_isis_cli, ISIS_MODE, CFG_DTYP_ISIS);

#ifdef HAVE_IPV6
  cli_install_default_family (ctree, ISIS_IPV6_MODE);
  cli_install_imi (ctree, ISIS_MODE, PM_ISIS, PRIVILEGE_VR_MAX, 0,
                   &mode_isis_af_ipv6_cli);
  cli_set_imi_cmd (&mode_isis_af_ipv6_cli, ISIS_IPV6_MODE, CFG_DTYP_ISIS);

  cli_install_imi (ctree, ISIS_IPV6_MODE, PM_ISIS, PRIVILEGE_VR_MAX, 0,
                   &mode_isis_exit_af_cli);
  cli_set_imi_cmd (&mode_isis_exit_af_cli, ISIS_MODE, CFG_DTYP_ISIS);
#endif /* HAVE_IPV6 */
#endif /* HAVE_ISISD */

  /* BGP.  */
#ifdef HAVE_BGPD
  cli_install_default (ctree, BGP_MODE);
  cli_install_default_family (ctree, BGP_IPV4_MODE);
  cli_install_default_family (ctree, BGP_IPV4M_MODE);

  cli_install_imi (ctree, CONFIG_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_router_bgp_cli);
  cli_set_imi_cmd (&mode_router_bgp_cli, BGP_MODE, CFG_DTYP_BGP);
  cli_install_imi (ctree, CONFIG_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_router_bgp_view_cli);
  cli_set_imi_cmd (&mode_router_bgp_view_cli, BGP_MODE, CFG_DTYP_BGP);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_ipv4_cli);
  cli_set_imi_cmd (&mode_bgp_af_ipv4_cli, BGP_IPV4_MODE, CFG_DTYP_BGP);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_ipv4_unicast_cli);
  cli_set_imi_cmd (&mode_bgp_af_ipv4_unicast_cli, BGP_IPV4_MODE, CFG_DTYP_BGP);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_ipv4_multicast_cli);
  cli_set_imi_cmd (&mode_bgp_af_ipv4_multicast_cli, BGP_IPV4M_MODE, CFG_DTYP_BGP);

  cli_install_imi (ctree, BGP_IPV4_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);
  cli_install_imi (ctree, BGP_IPV4M_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);


#ifdef HAVE_IPV6
  cli_install_default_family (ctree, BGP_IPV6_MODE);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_ipv6_cli);
  cli_set_imi_cmd (&mode_bgp_af_ipv6_cli, BGP_IPV6_MODE, CFG_DTYP_BGP);

#ifdef HAVE_6PE
  cli_install_default_family (ctree, BGP_6PE_MODE);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_ipv6_labeled_unicast_cli);
  cli_set_imi_cmd (&mode_bgp_af_ipv6_labeled_unicast_cli, BGP_6PE_MODE, CFG_DTYP_BGP);

  cli_install_imi (ctree, BGP_6PE_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);
#endif /* HAVE_6PE */

  cli_install_imi (ctree, BGP_IPV6_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);

#endif /* HAVE_IPV6 */

#ifdef HAVE_VRF
  cli_install_default_family (ctree, BGP_VPNV4_MODE);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_vpnv4_cli);
  cli_set_imi_cmd (&mode_bgp_af_vpnv4_cli, BGP_VPNV4_MODE, CFG_DTYP_BGP);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_vpnv4_unicast_cli);
  cli_set_imi_cmd (&mode_bgp_af_vpnv4_unicast_cli, BGP_VPNV4_MODE, CFG_DTYP_BGP);
  cli_install_imi (ctree, BGP_VPNV4_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);

  cli_install_default_family (ctree, BGP_IPV4_VRF_MODE);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_ipv4_vrf_cli);
  cli_set_imi_cmd (&mode_bgp_af_ipv4_vrf_cli, BGP_IPV4_VRF_MODE, CFG_DTYP_BGP);

  cli_install_imi (ctree, BGP_IPV4_VRF_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);
#ifdef HAVE_IPV6 /*6VPE*/
  cli_install_default_family (ctree, BGP_VPNV6_MODE);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_vpnv6_cli);
  cli_set_imi_cmd (&mode_bgp_af_vpnv6_cli, BGP_VPNV6_MODE, CFG_DTYP_BGP);

  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_vpnv6_unicast_cli);
  cli_set_imi_cmd (&mode_bgp_af_vpnv6_unicast_cli, BGP_VPNV6_MODE, CFG_DTYP_BGP);

  cli_install_imi (ctree, BGP_VPNV6_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);

  cli_install_default_family (ctree, BGP_IPV6_VRF_MODE);
  cli_install_imi (ctree, BGP_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_af_ipv6_vrf_cli);
  cli_set_imi_cmd (&mode_bgp_af_ipv6_vrf_cli, BGP_IPV6_VRF_MODE, CFG_DTYP_BGP);

  cli_install_imi (ctree, BGP_IPV6_VRF_MODE, PM_BGP, PRIVILEGE_VR_MAX, 0,
                   &mode_bgp_exit_af_cli);
  cli_set_imi_cmd (&mode_bgp_exit_af_cli, BGP_MODE, CFG_DTYP_BGP);

#endif /*HAVE_IPV6*/
#endif /* HAVE_VRF */
#endif /* HAVE_BGPD */

  /* LDP.  */
#ifdef HAVE_LDPD
  cli_install_default (ctree, LDP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_LDP, PRIVILEGE_MAX, 0,
                   &mode_router_ldp_cli);
  cli_set_imi_cmd (&mode_router_ldp_cli, LDP_MODE, CFG_DTYP_LDP);

  cli_install_imi (ctree, LDP_MODE, PM_LDP, PRIVILEGE_MAX, 0,
                   &mode_ldp_add_targeted_peer_cli);
  cli_set_imi_cmd (&mode_ldp_add_targeted_peer_cli, TARGETED_PEER_MODE,
                   CFG_DTYP_LDP_ROUTER);

  cli_install_imi (ctree, TARGETED_PEER_MODE, PM_LDP, PRIVILEGE_MAX, 0,
                   &mode_ldp_exit_tp_cli);
  cli_set_imi_cmd (&mode_ldp_exit_tp_cli, LDP_MODE,
                   CFG_DTYP_LDP_ROUTER);

#ifdef HAVE_CR_LDP

  cli_install_default (ctree, LDP_PATH_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_LDP, PRIVILEGE_MAX, 0,
                   &mode_ldp_path_cli);
  cli_set_imi_cmd (&mode_ldp_path_cli, LDP_PATH_MODE, CFG_DTYP_LDP_PATH);

  cli_install_default (ctree, LDP_TRUNK_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_LDP, PRIVILEGE_MAX, 0,
                   &mode_ldp_trunk_cli);
  cli_set_imi_cmd (&mode_ldp_trunk_cli, LDP_TRUNK_MODE, CFG_DTYP_LDP_TRUNK);
#endif /* HAVE_CR_LDP */
#endif /* HAVE_LDPD */

  /* RSVP.  */
#ifdef HAVE_RSVPD

  cli_install_default (ctree, RSVP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RSVP, PRIVILEGE_MAX, 0,
                   &mode_router_rsvp_cli);
  cli_set_imi_cmd (&mode_router_rsvp_cli, RSVP_MODE, CFG_DTYP_RSVP);

  cli_install_default (ctree, RSVP_TRUNK_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RSVP, PRIVILEGE_MAX, 0,
                   &mode_rsvp_trunk_cli);
  cli_set_imi_cmd (&mode_rsvp_trunk_cli, RSVP_TRUNK_MODE, CFG_DTYP_RSVP_TRUNK);

  cli_install_default (ctree, RSVP_PATH_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RSVP, PRIVILEGE_MAX, 0,
                   &mode_rsvp_path_cli);
  cli_set_imi_cmd (&mode_rsvp_path_cli, RSVP_PATH_MODE, CFG_DTYP_RSVP_PATH);
#ifdef HAVE_MPLS_FRR
  cli_install_default (ctree, RSVP_BYPASS_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RSVP, PRIVILEGE_MAX, 0,
                   &mode_rsvp_frr_bypass_cli);
  cli_set_imi_cmd (&mode_rsvp_frr_bypass_cli, RSVP_BYPASS_MODE,  CFG_DTYP_RSVP_BYPASS);
#endif /* HAVE_MPLS_FRR */
#endif /* HAVE_RSVPD */

  /* VPLS.  */
#ifdef HAVE_VPLS
  cli_install_default (ctree, NSM_VPLS_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_mpls_vpls_cli);
  cli_set_imi_cmd (&mode_mpls_vpls_cli, NSM_VPLS_MODE, CFG_DTYP_NSM_VPLS);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_mpls_vpls_by_name_cli);
  cli_set_imi_cmd (&mode_mpls_vpls_by_name_cli, NSM_VPLS_MODE, CFG_DTYP_NSM_VPLS);
#endif /* HAVE_VPLS */

  /* VRRP.  */
#ifdef HAVE_VRRP
  cli_install_default (ctree, VRRP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_router_vrrp_cli);
  cli_set_imi_cmd (&mode_router_vrrp_cli, VRRP_MODE, CFG_DTYP_VRRP);
  cli_install_imi (ctree, VRRP_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &imish_vrrp_enable_cli);
  cli_install_imi (ctree, VRRP_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &imish_vrrp_disable_cli);
#ifdef HAVE_CUSTOM1
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_router_vrrp_vlan_cli);
  cli_set_imi_cmd (&mode_router_vrrp_vlan_cli, VRRP_MODE, CFG_DTYP_VRRP);
#endif

#ifdef HAVE_IPV6
  cli_install_default (ctree, VRRP6_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_router_ipv6_vrrp_cli);
  cli_set_imi_cmd (&mode_router_ipv6_vrrp_cli, VRRP6_MODE, CFG_DTYP_VRRP6);
  cli_install_imi (ctree, VRRP6_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &imish_vrrp_enable_cli);
  cli_install_imi (ctree, VRRP6_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &imish_vrrp_disable_cli);
#ifdef HAVE_CUSTOM1
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_router_ipv6_vrrp_vlan_cli);
  cli_set_imi_cmd (&mode_router_ipv6_vrrp_vlan_cli, VRRP6_MODE, CFG_DTYP_VRRP6);
#endif /* HAVE_CUSTOM1 */

#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

#ifdef HAVE_LMP
   cli_install_default (ctree, LMP_MODE);
   cli_install_imi (ctree, CONFIG_MODE, PM_LMP, PRIVILEGE_MAX, 0,
                    &mode_lmp_configuration_cli);
   cli_set_imi_cmd (&mode_lmp_configuration_cli, LMP_MODE, CFG_DTYP_LMP);
#endif /* HAVE_LMP */

#ifdef HAVE_QOS
  cli_install_default (ctree, CMAP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_qos_class_map_cli);
  cli_set_imi_cmd (&mode_qos_class_map_cli, CMAP_MODE, CFG_DTYP_CMAP);

  cli_install_default (ctree, PMAP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_qos_policy_map_cli);
  cli_set_imi_cmd (&mode_qos_policy_map_cli, PMAP_MODE, CFG_DTYP_PMAP);

  cli_install_default (ctree, PMAPC_MODE);
  cli_install_imi (ctree, PMAP_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_qos_pmap_class_cli);
  cli_set_imi_cmd (&mode_qos_pmap_class_cli, PMAPC_MODE, CFG_DTYP_PMAP);

  cli_install_imi (ctree, PMAPC_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &mode_qos_pmap_class_cli);
  cli_set_imi_cmd (&mode_qos_pmap_class_cli, PMAPC_MODE, CFG_DTYP_PMAP);
#endif /* HAVE_QOS */

  /* VLAN */
#ifdef HAVE_VLAN

  cli_install_default (ctree, VLAN_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &vlan_database_cli);
  cli_set_imi_cmd (&vlan_database_cli, VLAN_MODE, CFG_DTYP_VLAN);

  cli_install_default (ctree, CVLAN_REGIST_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &cvlan_reg_tab_cli);
  cli_set_imi_cmd (&cvlan_reg_tab_cli, CVLAN_REGIST_MODE, CFG_DTYP_CVLAN_REGIST);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &cvlan_reg_tab_br_cli);
  cli_set_imi_cmd (&cvlan_reg_tab_br_cli, CVLAN_REGIST_MODE, CFG_DTYP_CVLAN_REGIST);

  cli_install_default (ctree, VLAN_ACCESS_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &vlan_access_map_cli);
  cli_set_imi_cmd (&vlan_access_map_cli, VLAN_ACCESS_MODE, CFG_DTYP_VLAN_ACCESS);

  /* WHY IS IT HERE ???? */
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &vlan_no_access_map_cli);

#endif /* HAVE_VLAN */

#ifdef HAVE_CFM
  cli_install_default (ctree, ETH_CFM_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_domain_cli);
  cli_set_imi_cmd (&eth_cfm_domain_cli, ETH_CFM_MODE, CFG_DTYP_ETH_CFM);

  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_domain_mac_cli);
  cli_set_imi_cmd (&eth_cfm_domain_mac_cli, ETH_CFM_MODE, CFG_DTYP_ETH_CFM);

#ifdef HAVE_CFM_Y1731
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_domain_itut_cli);
  cli_set_imi_cmd (&eth_cfm_domain_itut_cli, ETH_CFM_MODE, CFG_DTYP_ETH_CFM);
#endif /* HAVE_CFM_Y1731 */

  /* IEEE 802.1ag 2007 */
  cli_install_default (ctree, ETH_CFM_VLAN_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_vlan_cli);
  cli_set_imi_cmd (&eth_cfm_vlan_cli, ETH_CFM_VLAN_MODE, CFG_DTYP_ETH_CFM_VLAN);

#if (defined HAVE_I_BEB || defined HAVE_B_BEB)
  cli_install_default (ctree, ETH_CFM_PBB_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_pbb_domain_cli);
  cli_set_imi_cmd (&eth_cfm_pbb_domain_cli, ETH_CFM_PBB_MODE, CFG_DTYP_ETH_CFM_PBB);

  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                         &eth_cfm_pbb_domain_mac_cli);
  cli_set_imi_cmd (&eth_cfm_pbb_domain_mac_cli, ETH_CFM_PBB_MODE, CFG_DTYP_ETH_CFM_PBB);

#ifdef HAVE_CFM_Y1731
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_pbb_domain_itut_cli);
  cli_set_imi_cmd (&eth_cfm_pbb_domain_itut_cli, ETH_CFM_PBB_MODE, CFG_DTYP_ETH_CFM_PBB);
#endif /* HAVE_CFM_Y1731 */

#endif /* HAVE_I_BEB || HAVE_B_BEB  */
#endif /*HAVE_CFM */

#ifdef HAVE_I_BEB
  cli_install_default (ctree, PBB_ISID_CONFIG_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &pbb_isid_list_cmd);
  cli_set_imi_cmd (&pbb_isid_list_cmd, PBB_ISID_CONFIG_MODE,
                   CFG_DTYP_PBB_ISID_CONFIG);
#endif /* HAVE_I_BEB */


#if defined HAVE_I_BEB && defined HAVE_B_BEB && defined HAVE_PBB_TE
  cli_install_default (ctree, PBB_TE_VLAN_CONFIG_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &pbb_te_config_tesid_cmd);
  cli_set_imi_cmd (&pbb_te_config_tesid_cmd, PBB_TE_VLAN_CONFIG_MODE,
                   CFG_DTYP_PBB_TE_VLAN_CONFIG);

  cli_install_default (ctree, PBB_TE_ESP_IF_CONFIG_MODE);
  cli_install_imi (ctree, INTERFACE_MODE, PM_NSM, PRIVILEGE_MAX, 0,
      &pbb_te_esp_cbp_if_config_cmd);
  cli_set_imi_cmd (&pbb_te_esp_cbp_if_config_cmd, PBB_TE_ESP_IF_CONFIG_MODE,
      CFG_DTYP_PBB_TE_ESP_IF_CONFIG);

  cli_install_imi (ctree, PBB_TE_ESP_IF_CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
      &pbb_te_esp_cbp_if_config_cmd);
  cli_set_imi_cmd (&pbb_te_esp_cbp_if_config_cmd, PBB_TE_ESP_IF_CONFIG_MODE,
      CFG_DTYP_PBB_TE_ESP_IF_CONFIG);


  cli_install_imi (ctree, PBB_TE_ESP_IF_CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &pbb_te_esp_if_exit_cmd);
  cli_set_imi_cmd (&pbb_te_esp_if_exit_cmd, INTERFACE_MODE,
                   CFG_DTYP_INTERFACE);

#endif

#ifdef HAVE_CFM
#ifdef HAVE_PBB_TE
  cli_install_default (ctree, ETH_CFM_PBB_TE_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_pbb_te_domain_cli);
  cli_set_imi_cmd (&eth_cfm_pbb_te_domain_cli, ETH_CFM_PBB_TE_MODE,
                   CFG_DTYP_ETH_CFM_PBB_TE);
#ifdef HAVE_CFM_Y1731
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &eth_cfm_pbb_te_domain_itu_t_cli);
  cli_set_imi_cmd (&eth_cfm_pbb_te_domain_itu_t_cli, ETH_CFM_PBB_TE_MODE,
                   CFG_DTYP_ETH_CFM_PBB_TE);

#endif/*HAVE_CFM_Y1731 */
#endif /* HAVE_PBB_TE*/
#endif /* HAVE_CFM */

#ifdef HAVE_G8031
#ifdef HAVE_B_BEB
#ifdef HAVE_I_BEB
  cli_install_default (ctree, G8031_CONFIG_VLAN_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &g8031_i_b_beb_eps_vlan_create_cmd);
  cli_set_imi_cmd (&g8031_i_b_beb_eps_vlan_create_cmd, G8031_CONFIG_VLAN_MODE,
                   CFG_DTYP_G8031_VLAN);

  cli_install_default (ctree, G8031_CONFIG_SWITCHING_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &g8031_configure_i_b_beb_eps_group_create_cmd);
  cli_set_imi_cmd (&g8031_configure_i_b_beb_eps_group_create_cmd,
                   G8031_CONFIG_SWITCHING_MODE, CFG_DTYP_G8031_SWITCHING);
#else
  cli_install_default (ctree, G8031_CONFIG_VLAN_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &g8031_beb_eps_vlan_create_cmd);
  cli_set_imi_cmd (&g8031_beb_eps_vlan_create_cmd, G8031_CONFIG_VLAN_MODE,
                   CFG_DTYP_G8031_VLAN);

  cli_install_default (ctree, G8031_CONFIG_SWITCHING_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &g8031_configure_beb_eps_group_create_cmd);
  cli_set_imi_cmd (&g8031_configure_beb_eps_group_create_cmd,
                   G8031_CONFIG_SWITCHING_MODE, CFG_DTYP_G8031_SWITCHING);
#endif /* HAVE_B_BEB*/

#else
  cli_install_default (ctree, G8031_CONFIG_VLAN_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &g8031_eps_vlan_create_cmd);
  cli_set_imi_cmd (&g8031_eps_vlan_create_cmd, G8031_CONFIG_VLAN_MODE,
                   CFG_DTYP_G8031_VLAN);

  cli_install_default (ctree, G8031_CONFIG_SWITCHING_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &g8031_configure_eps_group_create_cmd);
  cli_set_imi_cmd (&g8031_configure_eps_group_create_cmd,
                   G8031_CONFIG_SWITCHING_MODE, CFG_DTYP_G8031_SWITCHING);
#endif /* defined HAVE_B_BEB */
  cli_install_default (ctree, G8031_CONFIG_SWITCHING_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &g8031_config_switching_cmd);
  cli_set_imi_cmd (&g8031_config_switching_cmd,
                   G8031_CONFIG_SWITCHING_MODE, CFG_DTYP_G8031_SWITCHING);
#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
#ifdef HAVE_B_BEB
     cli_install_default (ctree, G8032_CONFIG_MODE);
     cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                      &g8032_configure_beb_ring_create_cmd);
     cli_set_imi_cmd (&g8032_configure_beb_ring_create_cmd, G8032_CONFIG_MODE,
                      CFG_DTYP_G8032_CONFIG);

     cli_install_default (ctree, G8032_CONFIG_VLAN_MODE);
     cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                      &g8032_beb_vlan_create_cmd);
     cli_set_imi_cmd (&g8032_beb_vlan_create_cmd, G8032_CONFIG_VLAN_MODE,
                      CFG_DTYP_G8032_VLAN);
 #else
     cli_install_default (ctree, G8032_CONFIG_MODE);
     cli_install_imi (ctree, CONFIG_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                      &g8032_configure_ring_create_cmd);
     cli_set_imi_cmd (&g8032_configure_ring_create_cmd, G8032_CONFIG_MODE,
                      CFG_DTYP_G8032_CONFIG);

     cli_install_default (ctree, G8032_CONFIG_VLAN_MODE);
     cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                      &g8032_vlan_create_cmd);
     cli_set_imi_cmd (&g8032_vlan_create_cmd, G8032_CONFIG_VLAN_MODE,
                      CFG_DTYP_G8032_VLAN);
   #endif
    cli_install_default (ctree, G8032_CONFIG_MODE);
#endif /* HAVE_G8032 */

#if ((defined HAVE_CFM)&& (defined HAVE_I_BEB) && (defined HAVE_B_BEB) && \
         (defined HAVE_PBB_TE))
  cli_install_default (ctree, ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE);
  cli_install_imi (ctree, INTERFACE_MODE, PM_ONM, PRIVILEGE_MAX, 0,
                   &pbb_te_config_switching_mode_cmd);
  cli_set_imi_cmd (&pbb_te_config_switching_mode_cmd,
                   ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE, CFG_DTYP_ETH_PBB_TE_EPS);

  cli_install_imi (ctree, ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE, PM_ONM, PRIVILEGE_MAX, 0,
      &pbb_te_config_switching_mode_cmd);
  cli_set_imi_cmd (&pbb_te_config_switching_mode_cmd, ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE,
      CFG_DTYP_ETH_PBB_TE_EPS);

  cli_install_default (ctree, INTERFACE_MODE);
  cli_install_imi (ctree, ETH_PBB_TE_EPS_CONFIG_SWITCHING_MODE, PM_ONM,
      PRIVILEGE_MAX, 0, &pbb_te_exit_config_switching_mode_cmd);
  cli_set_imi_cmd (&pbb_te_exit_config_switching_mode_cmd,
                   INTERFACE_MODE, CFG_DTYP_INTERFACE);

#endif /* #if ((defined HAVE_CFM)&& (defined HAVE_I_BEB) && (defined HAVE_B_BEB) && (defined HAVE_PBB_TE)) */

#ifdef HAVE_MSTPD
  cli_install_default (ctree, MST_CONFIG_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_MSTP, PRIVILEGE_MAX, 0,
                   &mode_spanning_tree_mst_config_cli);
  cli_set_imi_cmd (&mode_spanning_tree_mst_config_cli, MST_CONFIG_MODE, CFG_DTYP_MST_CONFIG);
#endif /* HAVE_MSTPD */


#ifdef HAVE_RPVST_PLUS
  cli_install_default (ctree, RPVST_PLUS_CONFIG_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_MSTP, PRIVILEGE_MAX, 0,
                   &mode_spanning_tree_rpvst_plus_config_cli);
  cli_set_imi_cmd (&mode_spanning_tree_rpvst_plus_config_cli, RPVST_PLUS_CONFIG_MODE, CFG_DTYP_RPVST_PLUS);
#endif /* HAVE_RPVST_PLUS */

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
  cli_install_default (ctree, TE_MSTI_CONFIG_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_MSTP, PRIVILEGE_MAX, 0,
                   &mode_spanning_tree_te_msti_config_cli);
  cli_set_imi_cmd (&mode_spanning_tree_te_msti_config_cli, TE_MSTI_CONFIG_MODE, CFG_DTYP_TE_MSTI);
#endif /*(HAVE_PROVIDER_BRIDGE) ||(HAVE_B_BEB)*/

#ifdef HAVE_IPSEC
  cli_install_default (ctree, TRANSFORM_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &crypto_ipsec_transform_set_ah_cmd);
  cli_set_imi_cmd (&crypto_ipsec_transform_set_ah_cmd, TRANSFORM_MODE, CFG_DTYP_TRANSFORM);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &crypto_ipsec_transform_set_esp_cmd);
  cli_set_imi_cmd (&crypto_ipsec_transform_set_esp_cmd, TRANSFORM_MODE, CFG_DTYP_TRANSFORM);

  cli_install_default (ctree, CRYPTOMAP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &crypto_map_ipsec_manual_cmd);
  cli_set_imi_cmd (&crypto_map_ipsec_manual_cmd, CRYPTOMAP_MODE, CFG_DTYP_CRYPTOMAP);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &crypto_ipsec_isakmp_policy_cmd);
  cli_set_imi_cmd (&crypto_ipsec_isakmp_policy_cmd, CRYPTOMAP_MODE,  CFG_DTYP_CRYPTOMAP);

  cli_install_default (ctree, ISAKMP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &nsm_crypto_isakmp_policy_cmd);
  cli_set_imi_cmd (&nsm_crypto_isakmp_policy_cmd, ISAKMP_MODE, CFG_DTYP_ISAKMP);
#endif /* HAVE_IPSEC */

#ifdef HAVE_FIREWALL
  cli_install_default (ctree, FIREWALL_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &firewall_group_cmd);
  cli_set_imi_cmd (&firewall_group_cmd, FIREWALL_MODE, CFG_DTYP_FIREWALL);
#endif /* HAVE_FIREWALL */
  cli_install_default (ctree, EVC_SERVICE_INSTANCE_MODE);
  cli_install_imi (ctree, INTERFACE_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &service_instance_bw_profile_cli);
  cli_set_imi_cmd (&service_instance_bw_profile_cli, EVC_SERVICE_INSTANCE_MODE,
      CFG_DTYP_INTERFACE);

  cli_install_imi (ctree, EVC_SERVICE_INSTANCE_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &service_instance_bw_profile_cli);
  cli_set_imi_cmd (&service_instance_bw_profile_cli, EVC_SERVICE_INSTANCE_MODE,
      CFG_DTYP_INTERFACE);

 cli_install_default (ctree, INTERFACE_MODE);
  cli_install_imi (ctree, EVC_SERVICE_INSTANCE_MODE, PM_NSM, PRIVILEGE_MAX, 0,
                   &nsm_exit_service_instance_bw_profile_cmd);
  cli_set_imi_cmd (&nsm_exit_service_instance_bw_profile_cmd, INTERFACE_MODE,
      CFG_DTYP_INTERFACE);

#ifdef HAVE_GMPLS
  cli_install_default (ctree, TELINK_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_TEL, PRIVILEGE_MAX, 0,
                   &mode_gmpls_te_link_cli);
  cli_set_imi_cmd (&mode_gmpls_te_link_cli, TELINK_MODE, CFG_DTYP_TELINK);
  cli_install_imi (ctree, CONFIG_MODE, PM_TEL, PRIVILEGE_MAX, 0,
                   &mode_gmpls_te_link_by_name_cli);
  cli_set_imi_cmd (&mode_gmpls_te_link_by_name_cli, TELINK_MODE,
                   CFG_DTYP_TELINK);
  cli_install_default (ctree, CTRL_CHNL_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_CC, PRIVILEGE_MAX, 0,
                   &mode_gmpls_ctrl_channel_cli);
  cli_set_imi_cmd (&mode_gmpls_ctrl_channel_cli, CTRL_CHNL_MODE, CFG_DTYP_CTRL_CHNL);
  cli_install_imi (ctree, CONFIG_MODE, PM_CC, PRIVILEGE_MAX, 0,
                   &mode_gmpls_ctrl_channel_by_name_cli);
  cli_set_imi_cmd (&mode_gmpls_ctrl_channel_by_name_cli, CTRL_CHNL_MODE,
                   CFG_DTYP_CTRL_CHNL);
  cli_install_default (ctree, CTRL_ADJ_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_CA, PRIVILEGE_MAX, 0,
                   &mode_gmpls_ctrl_adj_cli);
  cli_set_imi_cmd (&mode_gmpls_ctrl_adj_cli, CTRL_ADJ_MODE, CFG_DTYP_CTRL_ADJ);
  cli_install_imi (ctree, CONFIG_MODE, PM_CA, PRIVILEGE_MAX, 0,
                   &mode_gmpls_ctrl_adj_by_name_cli);
  cli_set_imi_cmd (&mode_gmpls_ctrl_adj_by_name_cli, CTRL_ADJ_MODE,
                   CFG_DTYP_CTRL_ADJ);
#endif /*HAVE_GMPLS */
 cli_install_default (ctree, PW_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &pw_mode_by_id_cli);
 cli_set_imi_cmd (&pw_mode_by_id_cli, PW_MODE, CFG_DTYP_PW);

  cli_install_default (ctree, AC_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &ac_mode_by_id_cli);
 cli_set_imi_cmd (&ac_mode_by_id_cli, AC_MODE, CFG_DTYP_AC);
 
   cli_install_default (ctree, RPORT_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &rport_mode_by_fe_cli);
 cli_set_imi_cmd (&rport_mode_by_fe_cli, RPORT_MODE, CFG_DTYP_RPORT);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &rport_mode_by_ge_cli);
 cli_set_imi_cmd (&rport_mode_by_ge_cli, RPORT_MODE, CFG_DTYP_RPORT);

  cli_install_default (ctree, VPN_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &vpn_mode_by_name_cli);
 cli_set_imi_cmd (&vpn_mode_by_name_cli, VPN_MODE, CFG_DTYP_VPN);

  cli_install_default (ctree, LSP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				   &lsp_mode_by_id_cli);
  cli_set_imi_cmd (&lsp_mode_by_id_cli, LSP_MODE, CFG_DTYP_LSP);

  cli_install_default (ctree, LSP_GRP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				   &lsp_grp_mode_by_id_cli);
  cli_set_imi_cmd (&lsp_grp_mode_by_id_cli, LSP_GRP_MODE, CFG_DTYP_LSP_GRP);

  cli_install_default (ctree, LSP_OAM_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				   &lsp_oam_mode_by_id_cli);
  cli_set_imi_cmd (&lsp_oam_mode_by_id_cli, LSP_OAM_MODE, CFG_DTYP_LSP_OAM);

  cli_install_default (ctree, TUN_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &tun_mode_by_id_cli);
 cli_set_imi_cmd (&tun_mode_by_id_cli, TUN_MODE, CFG_DTYP_TUN);


 cli_install_default (ctree, OAM_1731_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &oam_1731_mode_cli);
 cli_set_imi_cmd (&oam_1731_mode_cli, OAM_1731_MODE, CFG_DTYP_OAM_1731);

 cli_install_default (ctree, MEG_MODE);
 cli_install_imi (ctree, SEC_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &meg_mode_by_index_cli);
 cli_set_imi_cmd (&meg_mode_by_index_cli, MEG_MODE, CFG_DTYP_MEG);


 cli_install_default (ctree, MEG_MODE);
 cli_install_imi (ctree, PW_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &meg_mode_by_index_cli);
 cli_set_imi_cmd (&meg_mode_by_index_cli, MEG_MODE, CFG_DTYP_MEG);


 cli_install_default (ctree, MEG_MODE);
 cli_install_imi (ctree, TUN_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &meg_mode_by_index_cli);
 cli_set_imi_cmd (&meg_mode_by_index_cli, MEG_MODE, CFG_DTYP_MEG);


 cli_install_default (ctree, SEC_MODE);
 cli_install_imi (ctree, OAM_1731_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &sec_mode_by_id_cli);
 cli_set_imi_cmd (&sec_mode_by_id_cli, SEC_MODE, CFG_DTYP_SEC);


 cli_install_default (ctree, SEC_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &sec_mode_by_id_cli);
 cli_set_imi_cmd (&sec_mode_by_id_cli, SEC_MODE, CFG_DTYP_SEC);
 
 cli_install_default (ctree, PTP_MODE);
 cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &ptp_mode_cli);
 cli_set_imi_cmd (&ptp_mode_cli, PTP_MODE, CFG_DTYP_PTP);

 cli_install_default (ctree, PTP_PORT_MODE);
 cli_install_imi (ctree, PTP_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &ptp_port_id_cli);
 cli_set_imi_cmd (&ptp_port_id_cli, PTP_PORT_MODE, CFG_DTYP_PTP_PORT);
 cli_install_default (ctree, PTP_PORT_MODE);
 cli_install_imi (ctree, PTP_PORT_MODE, PM_NSM, PRIVILEGE_MAX, 0,
				  &ptp_port_id_cli);
 cli_set_imi_cmd (&ptp_port_id_cli, PTP_PORT_MODE, CFG_DTYP_PTP_PORT);

}

