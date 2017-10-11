/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>

#include "cli.h"
#include "line.h"
#include "show.h"
#include "version.h"
#include "modbmap.h"
#include "message.h"
#include "imi_client.h"

#include "imish/imish.h"
#include "imish/imish_system.h"
#include "imish/imish_exec.h"
#include "imish/imish_cli.h"
#include "imish/imish_line.h"


/* All of "show" commands are installed in EXEC_MODE.  One exception
   is "show running-config", it will be installed into all modes for
   user convenience.  */

CLI (imish_show_func,
     imish_show_func_cli,
     "Internal_function",
     "Internal_function")
{
  struct cli_element *cel = cli->cel;
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm,
                             modbmap_bit2id (modbmap_or (cel->module, PM_IMI)));
  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (imish_show_privilege,
     imish_show_privilege_cli,
     "show privilege",
     "Show running system information",
     "Show current privilege level")
{
  printf ("Current privilege level is %d\n", cli->privilege);
  return CLI_SUCCESS;
}

ALI (imish_show_func,
     imish_show_running_config_cli,
     "show running-config",
     "Show running system information",
     "Current Operating configuration");

ALI (imish_show_func,
     imish_show_running_config_full_cli,
     "show running-config full",
     "Show running system information",
     "Current Operating configuration",
     "full configuration");

#ifdef HAVE_VR
ALI (imish_show_func,
     imish_show_running_config_virtual_router_cli,
     "show running-config virtual-router WORD",
     "Show running system information",
     "Current Operating configuration",
     CLI_VR_STR,
     CLI_VR_NAME_STR);
#endif /* HAVE_VR */

#ifdef HAVE_VRF
ALI (imish_show_func,
     imish_show_running_config_vrf_cli,
     "show running-config vrf WORD",
     "Show running syatem information",
     "Current Operating configuration",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR);
#endif /* HAVE_VRF */


ALI (imish_show_func,
     imish_show_running_config_if_cli,
     "show running-config interface IFNAME",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR);

/* Protocol Module configuration on a given interface */
#ifdef HAVE_RIPD
ALI (imish_show_func,
     imish_show_running_config_if_rip_cli,
     "show running-config interface IFNAME rip",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_RIP_STR);
#endif /* HAVE_RIPD */

#ifdef HAVE_OSPFD
ALI (imish_show_func,
     imish_show_running_config_if_ospf_cli,
     "show running-config interface IFNAME ospf",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_OSPF_STR);
#endif /* HAVE_OSPFD */

#ifdef HAVE_ISISD
ALI (imish_show_func,
     imish_show_running_config_if_isis_cli,
     "show running-config interface IFNAME isis",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_ISIS_STR);
#endif /* HAVE_ISISD */

#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
ALI (imish_show_func,
     imish_show_running_config_if_ipv6_rip_cli,
     "show running-config interface IFNAME ipv6 rip",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_RIPNG_STR);
#endif /* HAVE_RIPNGD */

#ifdef HAVE_OSPF6D
ALI (imish_show_func,
     imish_show_running_config_if_ipv6_ospf_cli,
     "show running-config interface IFNAME ipv6 ospf",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_OSPF6_STR);
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
ALI (imish_show_func,
     imish_show_running_config_if_mpls_cli,
     "show running-config interface IFNAME mpls",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_MPLS_STR);
#endif /* HAVE_MPLS */

#ifdef HAVE_LDPD
ALI (imish_show_func,
     imish_show_running_config_if_ldp_cli,
     "show running-config interface IFNAME ldp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_LDP_STR);
#endif /* HAVE_LDPD */

#ifdef HAVE_RSVPD
ALI (imish_show_func,
     imish_show_running_config_if_rsvp_cli,
     "show running-config interface IFNAME rsvp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_RSVP_STR);
#endif /* HAVE_RSVPD */

#ifdef HAVE_PDMD
ALI (imish_show_func,
     imish_show_running_config_if_ip_pim_dense_mode_cli,
     "show running-config interface IFNAME ip pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMDM_STR);
#endif /* HAVE_PDMD */

#ifdef HAVE_PIMD
ALI (imish_show_func,
     imish_show_running_config_if_ip_pim_sparse_mode_cli,
     "show running-config interface IFNAME ip pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMSM_STR);
#endif /* HAVE_PIMD */

#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
ALI (imish_show_func,
     imish_show_running_config_if_ipv6_pim_dense_mode_cli,
     "show running-config interface IFNAME ipv6 pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMDM6_STR);
#endif /* HAVE_PDMD */

#ifdef HAVE_PIM6D
ALI (imish_show_func,
     imish_show_running_config_if_ipv6_pim_sparse_mode_cli,
     "show running-config interface IFNAME ipv6 pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMSM6_STR);
#endif /*HAVE_PIM6D*/
#endif /* HAVE_IPV6 */

#ifdef HAVE_DVMRPD
ALI (imish_show_func,
     imish_show_running_config_if_ip_dvmrp_cli,
     "show running-config interface IFNAME ip dvmrp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_DVMRP_STR);
#endif /* HAVE_DVMRPD */

#ifdef HAVE_MCAST_IPV4
ALI (imish_show_func,
     imish_show_running_config_if_ip_igmp_cli,
     "show running-config interface IFNAME ip igmp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     CLI_IGMP_STR);

ALI (imish_show_func,
     imish_show_running_config_if_ip_multicast_cli,
     "show running-config interface IFNAME ip multicast",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_IP_STR,
     "Global IP multicast commands");
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_AUTHD
ALI (imish_show_func,
     imish_show_running_config_if_dot1x_cli,
     "show running-config interface IFNAME dot1x",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_8021X_STR);
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
ALI (imish_show_func,
     imish_show_running_config_if_lacp_cli,
     "show running-config interface IFNAME lacp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_LACP_STR);
#endif /* HAVE_LACPD */

#ifdef HAVE_STPD
ALI (imish_show_func,
     imish_show_running_config_if_stp_cli,
     "show running-config interface IFNAME stp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_STP_STR);
#endif /* HAVE_STPD */

#ifdef HAVE_RSTPD
ALI (imish_show_func,
     imish_show_running_config_if_rstp_cli,
     "show running-config interface IFNAME rstp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_RSTP_STR);
#endif /* HAVE_RSTPD */

#ifdef HAVE_MSTPD
ALI (imish_show_func,
     imish_show_running_config_if_mstp_cli,
     "show running-config interface IFNAME mstp",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_MSTP_STR);
#endif /* HAVE_MSTPD */
#ifdef HAVE_RPVST_PLUS
ALI (imish_show_func,
     imish_show_running_config_if_rpvst_plus_cli,
     "show running-config interface IFNAME rpvst+",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_RPVST_STR);
#endif /* HAVE_RPVST_PLUS */
#ifdef HAVE_L2
ALI (imish_show_func,
     imish_show_running_config_if_bridge_cli,
     "show running-config interface IFNAME bridge",
     "Show running system information",
     "Current Operating configuration",
     "Interface configuration",
     CLI_IFNAME_STR,
     CLI_SHOW_BRIDGE_STR);
#endif /* HAVE_L2 */

#ifdef HAVE_ELMID
ALI (imish_show_func,
    imish_show_running_config_if_elmi_cli,
    "show running-config interface IFNAME lmi",
    "Show running system information",
    "Current Operating configuration",
    "Interface configuration",
    CLI_IFNAME_STR,
    CLI_STP_STR);
#endif /* HAVE_ELMID */

#ifdef HAVE_GMPLS
ALI (imish_show_func,
     imish_show_running_config_te_link_all_cli,
     "show running-config te-link",
     "Show running system information",
     "Current Operating configuration",
     "TE Link configuration");

ALI (imish_show_func,
     imish_show_running_config_cc_all_cli,
     "show running-config control-channel",
     "Show running system information",
     "Current Operating configuration",
     "Control Channel configuration");

ALI (imish_show_func,
     imish_show_running_config_ca_all_cli,
     "show running-config control-adjacency",
     "Show running system information",
     "Current Operating configuration",
     "Control Adjacency configuration");

ALI (imish_show_func,
     imish_show_running_config_te_link_cli,
     "show running-config te-link TLNAME",
     "Show running system information",
     "Current Operating configuration",
     "TE Link configuration",
     CLI_TELINK_NAME_STR);

ALI (imish_show_func,
     imish_show_running_config_cc_cli,
     "show running-config control-channel CCNAME",
     "Show running system information",
     "Current Operating configuration",
     "Control Channel configuration",
     CLI_CCNAME_STR);

ALI (imish_show_func,
     imish_show_running_config_ca_cli,
     "show running-config control-adjacency CANAME",
     "Show running system information",
     "Current Operating configuration",
     "Control Adjacency configuration",
     CLI_CANAME_STR);
#endif /*HAVE_GMPLS*/

void
imish_cli_show_running_interface_init (struct cli_tree *ctree, int mode)
{
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_cli);
#ifdef HAVE_RIPD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                    &imish_show_running_config_if_rip_cli);
#endif /* HAVE_RIPD */
#ifdef HAVE_OSPFD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ospf_cli);
#endif /* HAVE_OSPFD */
#ifdef HAVE_ISISD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_isis_cli);
#endif /* HAVE_ISISD */
#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ipv6_rip_cli);
#endif /* HAVE_RIPNGD */
#ifdef HAVE_OSPF6D
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ipv6_ospf_cli);
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                    &imish_show_running_config_if_mpls_cli);
#endif /* HAVE_MPLS */
#ifdef HAVE_LDPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ldp_cli);
#endif  /* HAVE_LDPD */
#ifdef HAVE_RSVPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_rsvp_cli);
#endif  /* HAVE_RSVPD */
#ifdef HAVE_PDMD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ip_pim_dense_mode_cli);
#endif  /* HAVE_PDMD */
#ifdef HAVE_PIMD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ip_pim_sparse_mode_cli);
#endif  /* HAVE_PIMD */
#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ipv6_pim_dense_mode_cli);
#endif /* HAVE_PDMD */
#ifdef HAVE_PIM6D
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ipv6_pim_sparse_mode_cli);
#endif /* HAVE_PIM6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_DVMRPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                  &imish_show_running_config_if_ip_dvmrp_cli);
#endif  /* HAVE_DVMRPD */
#ifdef HAVE_MCAST_IPV4
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ip_igmp_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_ip_multicast_cli);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_AUTHD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_dot1x_cli);
#endif  /* HAVE_AUTHD */
#ifdef HAVE_LACPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_lacp_cli);
#endif  /* HAVE_LACPD */
#ifdef HAVE_STPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_stp_cli);
#endif  /* HAVE_STPD */
#ifdef HAVE_RSTPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_rstp_cli);
#endif  /* HAVE_RSTPD */
#ifdef HAVE_MSTPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_mstp_cli);
#endif  /* HAVE_MSTPD */
#ifdef HAVE_RPVST_PLUS
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_rpvst_plus_cli);
#endif  /* HAVE_RPVST_PLUS */

#ifdef HAVE_ELMID
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_elmi_cli);
#endif  /* HAVE_ELMID */

#ifdef HAVE_L2
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_if_bridge_cli);
#endif /* HAVE_L2 */
#ifdef HAVE_GMPLS 
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_te_link_all_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_cc_all_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ca_all_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_te_link_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_cc_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ca_cli);
#endif /*HAVE_GMPLS*/
}

/* Static route Configuration */
ALI (imish_show_func,
     imish_show_running_config_ip_route_cli,
     "show running-config ip route",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IP_STR,
     "Static IP route");

#ifdef HAVE_IPV6
ALI (imish_show_func,
     imish_show_running_config_ipv6_route_cli,
     "show running-config ipv6 route",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Static IP route");
#endif /* HAVE_IPV6 */

void
imish_config_static_route_init (struct cli_tree *ctree , int mode)
{
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ip_route_cli);
#ifdef HAVE_IPV6
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ipv6_route_cli);
#endif /* HAVE_IPV6 */
}

/* Multicast Static route Configuration */
ALI (imish_show_func,
     imish_show_running_config_ip_mroute_cli,
     "show running-config ip mroute",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IP_STR,
     "Static IP multicast route");

#ifdef HAVE_IPV6
ALI (imish_show_func,
     imish_show_running_config_ipv6_mroute_cli,
     "show running-config ipv6 mroute",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Static IPv6 multicast route");
#endif /* HAVE_IPV6 */

void
imish_config_static_mroute_init (struct cli_tree *ctree , int mode)
{
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ip_mroute_cli);
#ifdef HAVE_IPV6
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ipv6_mroute_cli);
#endif /* HAVE_IPV6 */
}
/* Show global mode router-id configuration */
ALI (imish_show_func,
     imish_show_running_config_router_id_cli,
     "show running-config router-id",
     CLI_SHOW_STR,
     "Current Operating configuration",
     "Router identifier for this system");

/* Show key-chain mode configuration */
ALI (imish_show_func,
     imish_show_running_config_key_chain_cli,
     "show running-config key chain",
     CLI_SHOW_STR,
     "Current Operating configuration",
     CLI_KEY_STR,
     CLI_CHAIN_STR);

void
imish_config_keychain_init (struct cli_tree *ctree , int mode)
{
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_key_chain_cli);
}

/* Protocol module configuration.  */
#ifdef HAVE_RIPD
ALI (imish_show_func,
     imish_show_running_config_rip_cli,
     "show running-config router rip",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_RIP_STR);
#endif /* HAVE_RIPD */

#ifdef HAVE_OSPFD
ALI (imish_show_func,
     imish_show_running_config_ospf_cli,
     "show running-config router ospf",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_OSPF_STR);
#endif /* HAVE_OSPFD */

#ifdef HAVE_ISISD
ALI (imish_show_func,
     imish_show_running_config_isis_cli,
     "show running-config router isis",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_ISIS_STR);
#endif /* HAVE_ISISD */

#ifdef HAVE_BGPD
ALI (imish_show_func,
     imish_show_running_config_virtual_router_bgp_cli,
     "show running-config bgp",
     "Show running system information",
     "Current Operating configuration",
     CLI_BGP_STR);

ALI (imish_show_func,
     imish_show_running_config_bgp_cli,
     "show running-config router bgp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_BGP_STR);
#endif /* HAVE_BGPD */

#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
ALI (imish_show_func,
     imish_show_running_config_ripng_cli,
     "show running-config router ipv6 rip",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_IPV6_STR,
     CLI_RIPNG_STR);
#endif /* HAVE_RIPNGD */

#ifdef HAVE_OSPF6D
ALI (imish_show_func,
     imish_show_running_config_ospf6_cli,
     "show running-config router ipv6 ospf",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_IPV6_STR,
     CLI_OSPF6_STR);
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
ALI (imish_show_func,
     imish_show_running_config_mpls_cli,
     "show running-config mpls",
     "Show running system information",
     "Current Operating configuration",
     CLI_MPLS_STR);
#endif /* HAVE_MPLS */

#ifdef HAVE_LDPD
ALI (imish_show_func,
     imish_show_running_config_ldp_cli,
     "show running-config router ldp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_LDP_STR);
#endif /* HAVE_LDPD */

#ifdef HAVE_RSVPD
ALI (imish_show_func,
     imish_show_running_config_rsvp_cli,
     "show running-config router rsvp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_RSVP_STR);

ALI (imish_show_func,
     imish_show_running_config_rsvp_path_cli,
     "show running-config rsvp-path",
     "Show running system information",
     "Current Operating configuration",
     CLI_RSVP_STR);

ALI (imish_show_func,
     imish_show_running_config_rsvp_trunk_cli,
     "show running-config rsvp-trunk",
     "Show running system information",
     "Current Operating configuration",
     CLI_RSVP_STR);
#endif /* HAVE_RSVPD */

#ifdef HAVE_VRRP
ALI (imish_show_func,
     imish_show_running_config_vrrp_cli,
     "show running-config router vrrp",
     "Show running system information",
     "Current Operating configuration",
     "Router configuration",
     CLI_VRRP_STR);
#ifdef HAVE_IPV6
ALI (imish_show_func,
     imish_show_running_config_vrrp_ipv6_cli,
     "show running-config router ipv6 vrrp",
     "Show running system information",
     "Current Operating configuration",
     "IPv6 Router configuration",
     CLI_IPV6_STR,
     CLI_VRRP_STR);
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

#ifdef HAVE_LACPD
ALI (imish_show_func,
     imish_show_running_config_switch_lacp_cli,
     "show running-config switch lacp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_LACP_STR);
#endif /* HAVE_LACPD */

#ifdef HAVE_IGMP_SNOOP
ALI (imish_show_func,
     imish_show_running_config_ip_igmp_snoop_cli,
     "show running-config ip igmp snooping",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     CLI_IGMP_STR,
     CLI_IGMP_SNOOP_STR);
#endif /* HAVE_IGMP_SNOOP */

void
imish_cli_show_running_router_init (struct cli_tree *ctree, int mode)
{
#ifdef HAVE_RIPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_rip_cli);
#endif /* HAVE_RIPD */
#ifdef HAVE_OSPFD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ospf_cli);
#endif /* HAVE_OSPFD */
#ifdef HAVE_ISISD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_isis_cli);
#endif /* HAVE_ISISD */
#ifdef HAVE_BGPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_virtual_router_bgp_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_bgp_cli);
#endif /* HAVE_BGPD */
#ifdef HAVE_IPV6
#ifdef HAVE_RIPNGD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ripng_cli);
#endif /* HAVE_RIPNGD */
#ifdef HAVE_OSPF6D
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_ospf6_cli);
#endif /* HAVE_OSPF6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_mpls_cli);
#endif /* HAVE_MPLS */
#ifdef HAVE_LDPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_ldp_cli);
#endif /* HAVE_LDPD */
#ifdef HAVE_RSVPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_rsvp_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_rsvp_path_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_rsvp_trunk_cli);
#endif /* HAVE_RSVPD */

#ifdef HAVE_VRRP
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_vrrp_cli);
#ifdef HAVE_IPV6
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_vrrp_ipv6_cli);
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */
}


#ifdef HAVE_PDMD
ALI (imish_show_func,
     imish_show_running_config_ip_pim_dense_mode_cli,
     "show running-config ip pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMDM_STR);
#endif /* HAVE_PDMD */

#ifdef HAVE_PIMD
ALI (imish_show_func,
     imish_show_running_config_ip_pim_sparse_mode_cli,
     "show running-config ip pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     CLI_PIM_STR,
     CLI_PIMSM_STR);
#endif /* HAVE_PIMD */

#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
ALI (imish_show_func,
     imish_show_running_config_ipv6_pim_dense_mode_cli,
     "show running-config ipv6 pim dense-mode",
     "Show running system information",
     "Current Operating configuration",
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMDM6_STR);
#endif /* HAVE_PDMD */

#ifdef HAVE_PIM6D
ALI (imish_show_func,
     imish_show_running_config_ipv6_pim_sparse_mode_cli,
     "show running-config ipv6 pim sparse-mode",
     "Show running system information",
     "Current Operating configuration",
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     CLI_PIMSM6_STR);
#endif /* HAVE_PIM6D */
#endif /* HAVE_IPV6 */

#ifdef HAVE_MCAST_IPV4
ALI (imish_show_func,
     imi_show_running_config_ip_multicast_cli,
     "show running-config ip multicast",
     "Show running system information",
     "Current Operating configuration",
     CLI_IP_STR,
     "Global IP multicast commands");

#endif /* HAVE_MCAST_IPV4 */
void
imish_cli_show_running_pim_init (struct cli_tree *ctree, int mode)
{
#ifdef HAVE_PDMD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_ip_pim_dense_mode_cli);
#endif /* HAVE_PDM */
#ifdef HAVE_PIMD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_ip_pim_sparse_mode_cli);
#endif /* HAVE_PIMD */
#ifdef HAVE_IPV6
#ifdef HAVE_PDMD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                    &imish_show_running_config_ipv6_pim_dense_mode_cli);
#endif /* HAVE_PDMD */
#ifdef HAVE_PIM6D
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                    &imish_show_running_config_ipv6_pim_sparse_mode_cli);
#endif /* HAVE_PIM6D */
#endif /* HAVE_IPV6 */
#ifdef HAVE_MCAST_IPV4
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imi_show_running_config_ip_multicast_cli);
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_LACPD
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                  &imish_show_running_config_switch_lacp_cli);
#endif /* HAVE_LACPD */
#ifdef HAVE_IGMP_SNOOP
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_ip_igmp_snoop_cli);
#endif /* HAVE_IGMP_SNOOP */
}

#ifdef HAVE_L2
ALI (imish_show_func,
     imish_show_running_config_switch_bridge_cli,
     "show running-config switch bridge",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
    "Bridge group commands");
#endif /* HAVE_L2 */

#ifdef HAVE_AUTHD
ALI (imish_show_func,
     imish_show_running_config_switch_dot1x_cli,
     "show running-config switch dot1x",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_8021X_STR);
#endif /* HAVE_AUTHD */

#ifdef HAVE_AUTHD
ALI (imish_show_func,
     imish_show_running_config_switch_radius_server_cli,
     "show running-config switch radius-server",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     "radius-server - radius server commands");
#endif /* HAVE_AUTHD */

#ifdef HAVE_GVRP
ALI (imish_show_func,
     imish_show_running_config_switch_gvrp_cli,
     "show running-config switch gvrp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_GVRP_STR);
#endif /* HAVE_GVRP */

#ifdef HAVE_GMRP
ALI (imish_show_func,
     imish_show_running_config_switch_gmrp_cli,
     "show running-config switch gmrp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_GMRP_STR);
#endif /* HAVE_GMRP */

#ifdef HAVE_STPD
ALI (imish_show_func,
     imish_show_running_config_switch_stp_cli,
     "show running-config switch stp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_STP_STR);
#endif /* HAVE_STPD */

#ifdef HAVE_RSTPD
ALI (imish_show_func,
     imish_show_running_config_switch_rstp_cli,
     "show running-config switch rstp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_RSTP_STR);
#endif /* HAVE_RSTPD */

#ifdef HAVE_MSTPD
ALI (imish_show_func,
     imish_show_running_config_switch_mstp_cli,
     "show running-config switch mstp",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_MSTP_STR);
#endif /* HAVE_MSTPD */

#ifdef HAVE_RPVST_PLUS
ALI (imish_show_func,
     imish_show_running_config_switch_rpvst_plus_cli,
     "show running-config switch rpsvt+",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_RSTP_STR);
#endif /* HAVE_RPVST_PLUS */

ALI (imish_show_func,
     imish_show_running_config_switch_vlan_cli,
     "show running-config switch vlan",
     "Show running system information",
     "Current Operating configuration",
     "Switch configuration",
     CLI_VLAN_STR);

#ifdef HAVE_ELMID
ALI (imish_show_func,
    imish_show_running_config_switch_elmi_cli,
    "show running-config switch lmi",
    "Show running system information",
    "Current Operating configuration",
    "Switch configuration",
    CLI_ELMI_STR);
#endif /* HAVE_ELMID */

void
imish_cli_show_running_switch_init (struct cli_tree *ctree, int mode)
{
#ifdef HAVE_L2
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_switch_bridge_cli);
#endif /* HAVE_L2 */
#ifdef HAVE_AUTHD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_dot1x_cli);
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_radius_server_cli);
#endif /* HAVE_AUTHD */
#ifdef HAVE_GVRP
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_gvrp_cli);
#endif /* HAVE_GVRP */
#ifdef HAVE_GMRP
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_gmrp_cli);
#endif /* HAVE_GMRP */
#ifdef HAVE_STPD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_stp_cli);
#endif /* HAVE_STPD */
#ifdef HAVE_RSTPD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_rstp_cli);
#endif /* HAVE_RSTPD */
#ifdef HAVE_MSTPD
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_mstp_cli);
#endif /* HAVE_MSTPD */
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_vlan_cli);
#ifdef HAVE_ELMID
   cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                    &imish_show_running_config_switch_elmi_cli);
#endif /* HAVE_ELMID */
}



ALI (imish_show_func,
     imish_show_running_config_access_list_cli,
     "show running-config access-list",
     "Show running system information",
     "Current Operating configuration",
     "Access-list");

ALI (imish_show_func,
     imish_show_running_config_prefix_list_cli,
     "show running-config prefix-list",
     "Show running system information",
     "Current Operating configuration",
     "Prefix-list");

#ifdef HAVE_IPV6
ALI (imish_show_func,
     imish_show_running_config_ipv6_access_list_cli,
     "show running-config ipv6 access-list",
     "Show running system information",
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Access-list");

ALI (imish_show_func,
     imish_show_running_config_ipv6_prefix_list_cli,
     "show running-config ipv6 prefix-list",
     "Show running system information",
     "Current Operating configuration",
     CLI_IPV6_STR,
     "Prefix-list");
#endif /* HAVE_IPV6 */

ALI (imish_show_func,
     imish_show_running_config_as_path_access_list_cli,
     "show running-config as-path access-list",
     "Show running system information",
     "Current Operating configuration",
     "Autonomous system path filter",
     "Access-list");

ALI (imish_show_func,
     imish_show_running_config_community_list_cli,
     "show running-config community-list",
     "Show running system information",
     "Current Operating configuration",
     "Community-list");

ALI (imish_show_func,
     imish_show_running_config_route_map_cli,
     "show running-config route-map",
     "Show running system information",
     "Current Operating configuration",
     "Route-map");

void
imish_cli_show_running_filter_init (struct cli_tree *ctree, int mode)
{
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_access_list_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_prefix_list_cli);
#ifdef HAVE_IPV6
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_ipv6_access_list_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_ipv6_prefix_list_cli);
#endif /* HAVE_IPV6 */
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_as_path_access_list_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_community_list_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_route_map_cli);
}


ALI (imish_show_func,
     imish_show_interface_cli,
     "show interface (IFNAME|)",
     "Show running system information",
     "Interface status and configuration",
     "Interface name");

#ifdef HAVE_STPD
#define HAVE_L2_BRIDGE
#endif /* HAVE_STPD */
#ifdef HAVE_RSTPD
#define HAVE_L2_BRIDGE
#endif /* HAVE_RSTPD */
#ifdef HAVE_MSTPD
#define HAVE_L2_BRIDGE
#endif /* HAVE_MSTPD */
#ifdef HAVE_RPVST_PLUS
#define HAVE_L2_BRIDGE
#endif /* HAVE_RPVST_PLUS */

/* Output modifier.  */
CLI (imish_modifier_begin,
     imish_modifier_begin_cli,
     "|| begin LINE",
     "Output modifiers",
     "Begin with the line that matches",
     "Regular Expression")
{
  return IMISH_MODIFIER_BEGIN;
}

CLI (imish_modifier_include,
     imish_modifier_include_cli,
     "|| include LINE",
     "Output modifiers",
     "Include lines that match",
     "Regular Expression")
{
  return IMISH_MODIFIER_INCLUDE;
}

CLI (imish_modifier_exclude,
     imish_modifier_exclude_cli,
     "|| exclude LINE",
     "Output modifiers",
     "Exclude lines that match",
     "Regular Expression")
{
  return IMISH_MODIFIER_EXCLUDE;
}

CLI (imish_modifier_redirect,
     imish_modifier_redirect_cli,
     "|| redirect WORD",
     "Output modifiers",
     "Redirect output",
     "Output file")
{
  return IMISH_MODIFIER_REDIRECT;
}

CLI (imish_modifier_grep,
     imish_modifier_grep_cli,
     "|| grep WORD",
     "Output modifiers",
     "Grep output",
     "Regular Expression")
{
  return IMISH_MODIFIER_GREP;
}

CLI (imish_modifier_grep_option,
     imish_modifier_grep_option_cli,
     "|| grep WORD WORD",
     "Output modifiers",
     "Grep output",
     "Grep option",
     "Regular Expression")
{
  return IMISH_MODIFIER_GREP;
}

CLI (imish_redirect_to_file,
     imish_redirect_to_file_cli,
     "> WORD",
     "Redirect output",
     "Output file")
{
  return IMISH_MODIFIER_REDIRECT;
}

#ifdef HAVE_CFM
#ifdef HAVE_CFM_Y1731

CLI (cfm_tx_loopback,
     cfm_tx_loopback_cmd,
     "ping ethernet (multicast (recursive|) | unicast rmepid RMEPID) mepid MEPID"
     "(domain DOMAIN_NAME | level LEVEL) vlan VLAN_ID"
     "(tlv (data VAL | test (1 | 2 | 3 |4))|)(bridge <1-32>|)",
     "send a ping request",
     "ethernet",
     "send multicast frame",
     "send recursive multicast frames (5 frames)",
     "send unicast frame",
     "mep ID of remote MEP",
     "enter the remote mepid",
     "host MEP from where to send frame",
     "host mep ID",
     "maintanance domain of mep",
     "enter the name of the domain",
     "level associated with the domain",
     "enter the level of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "Type, Length, Value (TLV)",
     "TLV Type Data",
     "Number of bytes for Data TLV to be sent",
     "TLV Type Test",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "Bridge group commands"
     "Bridge group name")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_tx_loopback2,
     cfm_tx_loopback2_cmd, /* ping ethernet unicast mac MACADDRESS */
     "ping ethernet mac MACADDRESS unicast source MEPID (domain DOMAIN_NAME | level LEVEL) "
     "vlan VLAN_ID  (tlv (data VAL | test (1 | 2 | 3 |4))|) (bridge <1-32>|)",
     "send a ping request",
     "ethernet",
     "destination MAC address",
     "enter the destination mac address in HHHH.HHHH.HHHH format",
     "unicast frame to be sent",
     "source MEP ID",
     "enter the source MEP ID",
     "maintanance domain of mep",
     "enter the name of the domain",
     "level associated with the domain",
     "enter the level of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "Type, Length, Value (TLV)",
     "TLV Type Data",
     "Number of bytes for Data TLV to be sent",
     "TLV Type Test",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "Bridge group commands"
     "Bridge group name")
{

  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_tx_lmm,
     cfm_tx_lmm_cmd,
     "ethernet cfm lmm mpid MEPID (unicast rmepid RMEPID | multicast) duration <5-60> "
     "(domain DOMAIN_NAME | level LEVEL) vlan VLAN_ID ((interval (1 | 2 | 3 | 4))|)(bridge <1-32>|)",
     "configure Layer 2",
     "configure cfm",
     "loss measurement message",
     "host MEP's ID",
     "enter the host MEP's ID",
     "unicast frame to be sent",
     "remote MEP's ID",
     "enter the Remote MEP's ID",
     "multicast frame to be sent",
     "duration for which LMM has to be observed",
     "enter the duration in seconds",
     "maintanance domain of mep",
     "enter the name of the domain",
     "level associated with the domain",
     "enter the level of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "LMM transmission interval",
     "LM Trainsmision interval - 100 ms",
     "LM Trainsmision interval - 10 ms",
     "LM Trainsmision interval - 1 s",
     "LM Trainsmision interval - 10 s",
     "Bridge group commands"
     "Bridge group name")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;

}

CLI (cfm_dm_receive,
     cfm_dm_receive_cmd,
     "ethernet cfm 1dm receive mepid MEPID "
     "duration DURATION domain DOMAIN (vlan VLAN_ID|) (bridge <1-32>|)",
     "configure Layer-2",
     "configure cfm",
     "receive frames",
     "configure one-way delay measurment frames",
     "host MEPID",
     "enter host MEPID",
     "configure duration",
     "enter duration in <5-60>seconds",
     "associated to level",
     "enter the level",
     "enter the primary-vid of the MA",
     "enter the vlan",
     "Bridge group commands"
     "Bridge group name")
{

  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_dm_enable,
     cfm_dm_enable_cmd,
     "ethernet cfm (1dm|dvm|dmm) mepid MEPID (unicast (rmpid MEPID)|multicast) "
     "duration DURATION domain DOMAIN_NAME vlan VLAN_ID ((interval (1 | 2 | 3 | 4))|) "
     "(bridge <1-32>|)",
     "configure Layer-2",
     "configure cfm",
     "configure one-way delay measurment(clocks are synchronized)",
     "configure delay variation measurment(clocks are not synchronized)",
     "configure two-way delay measurment",
     "host MEPID",
     "enter host MEPID",
     "configure unicast DM",
     "MEPID of remote MEP",
     "enter remote MEPID",
     "configure multicast DM",
     "configure duration",
     "enter duration in <5-60>seconds",
     "maintenance domain to on which CC interval is being set",
     "enter the maintenance domain",
     "enter the primary-vid of the MA",
     "enter the vlan",
     "DMM transmission interval",
     "DM Trainsmision interval - 100 ms",
     "DM Trainsmision interval - 10 ms",
     "DM Trainsmision interval - 1 s",
     "DM Trainsmision interval - 10 s",
     "Bridge group commands"
     "Bridge group name")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_tst_frame_send,
     cfm_tst_frame_send_cmd,
     "ethernet cfm tst mep MEPID unicast RMEPID " 
     "pattern (1 | 2 | 3 | 4) domain DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) " 
     "(recursive duration <5-60> interval TX_INTERVAL|) "
     "(lck interval TX_INTERVAL (unicast rmepid RMEPID | multicast)|) "
     "(bridge <1-32> | backbone)",
     "configure Layer-2",
     "configure cfm",
     "test signal",
     "MEP ID of host MEP",
     "enter the MEP ID of host MEP",
     "unicast frame to be sent",
     "enter remote MEP's id",
     "TST pattern to be sent",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "domain to which MEP is associated",
     "enter the domain name",
     "vlan to which it is associated",
     "enter the vlan-id",
     "service instance id to which it is associated",
     "enter the isid ranging from <1-1677214>",
     "recursively sending TST frames",
     "duration for which test frames will be sent",
     "enter the duration",
     "transimission interval for test frames <1-10 seconds>",
     "enter the TX interval val, default is 1 s",
     "LCK frame will be sent in opposite direction for out-of-service testing",
     "transimission interval for lck, should be same as AIS tx interval",
     "1 or 60 seconds ",
     "unicast LCK frame to be sent",
     "remote mep id for LCK",
     "enter the remote mep id",
     "multicast LCK frame to be sent",
     "Bridge group commands"
     "Bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_tst_frame_send_cont,
     cfm_tst_frame_send_cont_cmd,
     "ethernet cfm tst mep MEPID multicast " 
     "pattern (1 | 2 | 3 | 4) domain DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) " 
     "(recursive duration <5-60> interval TX_INTERVAL|) "
     "(lck interval TX_INTERVAL (unicast rmepid RMEPID | multicast)|) "
     "(bridge <1-32> | backbone)",
     "configure Layer-2",
     "configure cfm",
     "test signal",
     "MEP ID of host MEP",
     "enter the MEP ID of host MEP",
     "multicast test frame to be sent",
     "TST pattern to be sent",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "domain to which MEP is associated",
     "enter the domain name",
     "vlan to which it is associated",
     "enter the vlan-id",
     "service instance id to which it is associated",
     "enter the isid ranging from <1-1677214>",
     "recursively sending TST frames",
     "duration for which test frames will be sent",
     "enter the duration",
     "transimission interval for test frames <1-10 seconds>",
     "enter the TX interval val, default is 1 s",
     "LCK frame will be sent in opposite direction for out-of-service testing",
     "transimission interval for lck, should be same as AIS tx interval",
     "1 or 60 seconds ",
     "unicast LCK frame to be sent",
     "remote mep id for LCK",
     "enter the remote mep id",
     "multicast LCK frame to be sent",
     "Bridge group commands"
     "Bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}



CLI (cfm_throughput_rx,
     cfm_throughput_rx_cmd,
     "ethernet cfm throughput-measurement reception mepid MEPID "
     "duration <1-10> domain DOMAIN_NAME vlan VLAN_ID (bridge <1-32>|)",
     "configure Layer 2",
     "configure cfm",
     "throughput measurement",
     "enable reception of TST frames for throughput measurement", 
     "Host MEP ID",
     "Enter the mep id",
     "Maximum duration for waiting for TST frames before timing out",
     "Maximum duration to wait for before timing out <1-10>",
     "maintenance-domain name",
     "enter the maintenance domain name",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "Bridge group commands"
     "Bridge group name")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}


#else
CLI (cfm_tx_loopback,
     cfm_tx_loopback_cmd,
     "ping ethernet mpid MEPID (domain DOMAIN_NAME | level LEVEL) "
     "vlan VLAN_ID (bridge <1-32>|)",
     "send a ping request",
     "ethernet",
     "configure mep",
     "enter the mepid",
     "maintanance domain of mep",
     "enter the name of the domain",
     "level associated with the domain",
     "enter the level of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "Bridge group commands"
     "Bridge group name")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_tx_loopback2,
     cfm_tx_loopback2_cmd,
     "ping ethernet MACADDRESS (domain DOMAIN_NAME | level LEVEL) "
     "vlan VLAN_ID (source MEPID|) (bridge <1-32>|)",
     "send a ping request",
     "ethernet",
     "enter the mac address in HHHH.HHHH.HHHH format",
     "maintanance domain of mep",
     "enter the name of the domain",
     "level associated with the domain",
     "enter the level of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "Enter the source MEP ID",
     "The Source MEP ID",
     "Bridge group commands"
     "Bridge group name")
{

  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

#endif /* HAVE_CFM_Y1731 */
CLI (cfm_tx_link_trace,
     cfm_tx_link_trace_cmd,
     "traceroute ethernet (MAC|) (domain DOMAIN | level LEVEL) vlan VLANID "
     "(bridge <1-32>|)",
     "traceroute command",
     "for layer-2",
     "mac address of the remote mep",
     "domain of the destination mep",
     "domain name of destination mep",
     "level to which remote mep belong",
     "integer from 0-7 - md level of remote mep",
     "enter the primary-vid of the MA",
     "<2-4096> - vlan to which destination mep is associated")
{
    char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

#endif /* HAVE_CFM */
#if defined HAVE_CFM && (defined HAVE_I_BEB || defined HAVE_B_BEB)
#ifdef HAVE_CFM_Y1731
CLI (cfm_pbb_tx_loopback,
     cfm_pbb_tx_loopback_cmd,
     "ping ethernet pbb unicast rmepid RMEPID "
     "mepid MEPID domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) "
     "(tlv (data VAL | test (1 | 2 | 3 |4))|) (bridge <1-32> | backbone)",
     "send a ping request",
     "ethernet",
     "send ping request for cfm in pbb",
     "send multicast frame",
     "send recursive multicast frames (5 frames)",
     "send unicast frame",
     "mep ID of remote MEP",
     "enter the remote mepid",
     "host MEP from where to send frame",
     "host mep ID",
     "maintenance domain of mep",
     "enter the name of the domain",
     "configure the vlan",
     "enter the vlan-id",
     "configure the isid",
     "enter the isid ranging from <1-16777214>",
     "Type, Length, Value (TLV)",
     "TLV Type Data",
     "Number of bytes for Data TLV to be sent",
     "TLV Type Test",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_tx_loopback_cont1,
     cfm_pbb_tx_loopback_cont1_cmd,
     "ping ethernet pbb multicast "
     "mepid MEPID domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) "
     "(tlv (data VAL | test (1 | 2 | 3 |4))|) (bridge <1-32> | backbone)",
     "send a ping request",
     "ethernet",
     "send ping request for cfm in pbb",
     "send multicast frame",
     "host MEP from where to send frame",
     "host mep ID",
     "maintenance domain of mep",
     "enter the name of the domain",
     "configure the vlan",
     "enter the vlan-id",
     "configure the isid",
     "enter the isid ranging from <1-16777214>",
     "Type, Length, Value (TLV)",
     "TLV Type Data",
     "Number of bytes for Data TLV to be sent",
     "TLV Type Test",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_tx_loopback_cont2,
     cfm_pbb_tx_loopback_cont2_cmd,
     "ping ethernet pbb multicast recursive "
     "mepid MEPID domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) "
     "(tlv (data VAL | test (1 | 2 | 3 |4))|) (bridge <1-32> | backbone)",
     "send a ping request",
     "ethernet",
     "send ping request for cfm in pbb",
     "send multicast frame",
     "send recursive multicast frames (5 frames)",
     "host MEP from where to send frame",
     "host mep ID",
     "maintenance domain of mep",
     "enter the name of the domain",
     "configure the vlan",
     "enter the vlan-id",
     "configure the isid",
     "enter the isid ranging from <1-16777214>",
     "Type, Length, Value (TLV)",
     "TLV Type Data",
     "Number of bytes for Data TLV to be sent",
     "TLV Type Test",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}


CLI (cfm_pbb_tx_loopback2,
     cfm_pbb_tx_loopback2_cmd,
     "ping ethernet pbb mac MACADDRESS unicast source MEPID "
     "domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) "
     "(tlv (data VAL | test (1 | 2 | 3 |4))|) (bridge <1-32> | backbone)",
     "send a ping request",
     "ethernet",
     "send a ping request for cfm in pbb",
     "destination MAC address",
     "enter the destination mac address in HHHH.HHHH.HHHH format",
     "unicast frame to be sent",
     "source MEP ID",
     "enter the source MEP ID",
     "maintanance domain of mep",
     "enter the name of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "configure the service instance id",
     "enter the isid ranging from <1-16777214>",
     "Type, Length, Value (TLV)",
     "TLV Type Data",
     "Number of bytes for Data TLV to be sent",
     "TLV Type Test",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "Bridge group commands",
     "Bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_tx_lmm,
     cfm_pbb_tx_lmm_cmd,
     "ethernet cfm pbb lmm mpid MEPID unicast rmepid RMEPID duration <5-60> "
     "domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) "
     "((interval (1 | 2 | 3 | 4))|) (bridge <1-32> | backbone)",
     "configure Layer 2",
     "configure cfm",
     "configure cfm for pbb",
     "loss measurement message",
     "host MEP's ID",
     "enter the host MEP's ID",
     "unicast frame to be sent",
     "remote MEP's ID",
     "enter the Remote MEP's ID",
     "duration for which LMM has to be observed",
     "enter the duration in seconds",
     "maintenance domain of mep",
     "enter the name of the domain",
     "configure the vlan",
     "enter the vlan-id",
     "configure the service instance id",
     "enter the isid in the range <1-16777214>",
     "LMM transmission interval",
     "LM Trainsmision interval - 100 ms",
     "LM Trainsmision interval - 10 ms",
     "LM Trainsmision interval - 1 s",
     "LM Trainsmision interval - 10 s",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_tx_lmm_cont,
     cfm_pbb_tx_lmm_cont_cmd,
     "ethernet cfm pbb lmm mpid MEPID multicast duration <5-60> "
     "domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) "
     "((interval (1 | 2 | 3 | 4))|) (bridge <1-32> | backbone)",
     "configure Layer 2",
     "configure cfm",
     "configure cfm for pbb",
     "loss measurement message",
     "host MEP's ID",
     "enter the host MEP's ID",
     "multicast frame to be sent",
     "duration for which LMM has to be observed",
     "enter the duration in seconds",
     "maintenance domain of mep",
     "enter the name of the domain",
     "configure the vlan",
     "enter the vlan-id",
     "configure the service instance id",
     "enter the isid in the range <1-16777214>",
     "LMM transmission interval",
     "LM Trainsmision interval - 100 ms",
     "LM Trainsmision interval - 10 ms",
     "LM Trainsmision interval - 1 s",
     "LM Trainsmision interval - 10 s",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_dm_receive,
     cfm_pbb_dm_receive_cmd,
     "ethernet cfm pbb 1dm receive mepid MEPID "
     "duration DURATION domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) "
     "(bridge <1-32> | backbone)",
     "configure Layer-2",
     "configure cfm",
     "configure cfm for pbb",
     "receive frames 1dm frames",
     "configure one-way delay measurment frames",
     "host MEPID",
     "enter host MEPID",
     "configure duration",
     "enter duration in <5-60>seconds",
     "associated to domain",
     "enter the domain name",
     "enter the primary-vid of the MA",
     "enter the vlan",
     "associated to service instance id",
     "enter the isid ranging from <1-16777214>",
     "bridge group command",
     "bridge group name",
     "backbone")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_dm_enable,
     cfm_pbb_dm_enable_cmd,
     "ethernet cfm pbb (1dm|dvm|dmm) mepid MEPID unicast rmpid MEPID" 
     " duration DURATION domain-name DOMAIN_NAME "
     "(((vlan VLAN_ID)|(isid ISID))|) "
     "((interval (1 | 2 | 3 | 4))|) (bridge <1-32> | backbone)",
     "configure Layer-2",
     "configure cfm",
     "configure the cfm for pbb",
     "configure one-way delay measurment(clocks are synchronized)",
     "configure delay variation measurment(clocks are not synchronized)",
     "configure two-way delay measurment",
     "host MEPID",
     "enter host MEPID",
     "configure unicast DM",
     "MEPID of remote MEP",
     "enter remote MEPID",
     "configure duration",
     "enter duration in <5-60> seconds",
     "associated to domain",
     "enter the domain name",
     "associated to vlan",
     "enter the vlan",
     "associated to service instance id",
     "enter the isid",
     "DMM transmission interval",
     "DM Trainsmision interval - 100 ms",
     "DM Trainsmision interval - 10 ms",
     "DM Trainsmision interval - 1 s",
     "DM Trainsmision interval - 10 s",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_dm_enable_cont,
     cfm_pbb_dm_enable_cont_cmd,
     "ethernet cfm pbb (1dm|dvm|dmm) mepid MEPID multicast "
     "duration DURATION domain-name DOMAIN_NAME "
     "(((vlan VLAN_ID)|(isid ISID))|) "
     "((interval (1 | 2 | 3 | 4))|) (bridge <1-32> | backbone)",
     "configure Layer-2",
     "configure cfm",
     "configure the cfm for pbb",
     "configure one-way delay measurment(clocks are synchronized)",
     "configure delay variation measurment(clocks are not synchronized)",
     "configure two-way delay measurment",
     "host MEPID",
     "enter host MEPID",
     "configure multicast DM",
     "configure duration",
     "enter duration in <5-60> seconds",
     "associated to domain",
     "enter the domain name",
     "associated to vlan",
     "enter the vlan",
     "associated to service instance id",
     "enter the isid",
     "DMM transmission interval",
     "DM Trainsmision interval - 100 ms",
     "DM Trainsmision interval - 10 ms",
     "DM Trainsmision interval - 1 s",
     "DM Trainsmision interval - 10 s",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}


CLI (cfm_pbb_tst_frame_send,
     cfm_pbb_tst_frame_send_cmd,
     "ethernet cfm pbb tst mep MEPID unicast RMEPID " 
     "pattern (1 | 2 | 3 | 4) domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) " 
     "(recursive duration <5-60> interval TX_INTERVAL|) "
     "(lck interval TX_INTERVAL (unicast rmepid RMEPID | multicast)|) "
     "(bridge <1-32> | backbone)",
     "configure Layer-2",
     "configure cfm",
     "configure cfm for pbb",
     "test signal",
     "MEP ID of host MEP",
     "enter the MEP ID of host MEP",
     "unicast frame to be sent",
     "enter remote MEP's id",
     "TST pattern to be sent",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "domain to which MEP is associated",
     "enter the domain name",
     "vlan to which it is associated",
     "enter the vlan-id",
     "service instance id to which it is associated",
     "enter the isid ranging from <1-1677214>",
     "recursively sending TST frames",
     "duration for which test frames will be sent",
     "enter the duration",
     "transimission interval for test frames <1-10 seconds>",
     "enter the TX interval val, default is 1 s",
     "LCK frame will be sent in opposite direction for out-of-service testing",
     "transimission interval for lck, should be same as AIS tx interval",
     "1 or 60 seconds ",
     "unicast LCK frame to be sent",
     "remote mep id for LCK",
     "enter the remote mep id",
     "multicast LCK frame to be sent",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_tst_frame_send_cont,
     cfm_pbb_tst_frame_send_cont_cmd,
     "ethernet cfm pbb tst mep MEPID multicast " 
     "pattern (1 | 2 | 3 | 4) domain-name DOMAIN_NAME ((vlan VLAN_ID | isid ISID)|) " 
     "(recursive duration <5-60> interval TX_INTERVAL|) "
     "(lck interval TX_INTERVAL (unicast rmepid RMEPID | multicast)|) "
     "(bridge <1-32> | backbone)",
     "configure Layer-2",
     "configure cfm",
     "configure cfm for pbb",
     "test signal",
     "MEP ID of host MEP",
     "enter the MEP ID of host MEP",
     "multicast test frame to be sent",
     "TST pattern to be sent",
     "Test Pattern- abc",
     "Test Pattern- 1234",
     "Test Pattern- a1b2c",
     "Test Pattern- 1a2b3c",
     "domain to which MEP is associated",
     "enter the domain name",
     "vlan to which it is associated",
     "enter the vlan-id",
     "service instance id to which it is associated",
     "enter the isid ranging from <1-1677214>",
     "recursively sending TST frames",
     "duration for which test frames will be sent",
     "enter the duration",
     "transimission interval for test frames <1-10 seconds>",
     "enter the TX interval val, default is 1 s",
     "LCK frame will be sent in opposite direction for out-of-service testing",
     "transimission interval for lck, should be same as AIS tx interval",
     "1 or 60 seconds ",
     "unicast LCK frame to be sent",
     "remote mep id for LCK",
     "enter the remote mep id"
     "multicast LCK frame to be sent",
     "bridge group command",
     "bridge group name",
     "backbone bridge")

{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}


CLI (cfm_pbb_throughput_rx,
     cfm_pbb_throughput_rx_cmd,
     "ethernet cfm pbb throughput-measurement reception mepid MEPID "
     "duration <1-10> domain-name DOMAIN_NAME ((vlan VLAN_ID)|(isid ISID)) "
     "(bridge <1-32> | backbone)",
     "configure Layer 2",
     "configure cfm",
     "configure cfm for pbb",
     "throughput measurement",
     "enable reception of TST frames for throughput measurement",
     "Host MEP ID",
     "Enter the mep id",
     "Maximum duration for waiting for TST frames before timing out",
     "Maximum duration to wait for before timing out <1-10>",
     "domain of the mep",
     "enter the domain name of the mep",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "configure the service instance id",
     "enter the isid ranging from <1-16777214>",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
#else
CLI (cfm_pbb_tx_loopback,
     cfm_pbb_tx_loopback_cmd,
     "ping ethernet pbb mpid MEPID domain-name DOMAIN_NAME "
     "((vlan VLAN_ID)|(isid ISID))(bridge <1-32> | backbone )",
     "send a ping request",
     "ethernet",
     "configure cfm for pbb",
     "configure mep",
     "enter the mepid",
     "maintanance domain of mep",
     "enter the name of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "configure the service instance id",
     "enter the isid ranging from <1-16777214>",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
CLI (cfm_pbb_tx_loopback2,
     cfm_pbb_tx_loopback2_cmd,
     "ping ethernet pbb MACADDRESS domain-name DOMAIN_NAME "
     "((vlan VLAN_ID) | (isid ISID))(source MEPID|) (bridge <1-32> | backbone)",
     "send a ping request",
     "ethernet",
     "configure cfm for pbb",
     "enter the mac address in HHHH.HHHH.HHHH format",
     "maintanance domain of mep",
     "enter the name of the domain",
     "enter the primary-vid of the MA",
     "enter the vlan-id",
     "configure the service instance id",
     "enter the isid ranging from <1-16777214>",
     "Enter the source MEP ID",
     "The Source MEP ID",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
#endif /* HAVE_CFM_Y1731 */
CLI (cfm_pbb_tx_link_trace,
     cfm_pbb_tx_link_trace_cmd,
     "traceroute pbb ethernet MAC domain-name DOMAIN_NAME "
     "((vlan VLANID | isid ISID)|) (bridge <1-32> | backbone)",
     "traceroute command",
     "configure traceroute for cfm in pbb",
     "for layer-2",
     "mac address of the remote mep",
     "domain of the destination mep",
     "domain name of destination mep",
     "level to which remote mep belong",
     "integer from 0-7 - md level of remote mep",
     "enter the primary-vid of the MA",
     "<2-4096> - vlan to which destination mep is associated",
     "bridge group command",
     "bridge group name",
     "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
#endif /* HAVE_CFM && (HAVE_I_BEB || HAVE_B_BEB) */
#if defined HAVE_I_BEB && defined HAVE_B_BEB && defined HAVE_PBB_TE
#ifdef HAVE_CFM_Y1731
CLI (cfm_pbb_te_tx_loopback,
    cfm_pbb_te_tx_loopback_cmd,
    "ping ethernet pbb-te (multicast (recursive|) | unicast rmepid RMEPID) "
    "mepid MEPID domain-name DOMAIN_NAME te-sid TE_SID "
    "(tlv ( data VAL | test (1 | 2 | 3 |4))|) "
    "bridge backbone",
    "send a ping request",
    "ethernet",
    "send ping request for cfm in pbb-te",
    "send multicast frame",
    "send recursive multicast frames (5 frames)",
    "send unicast frame",
    "mep ID of remote MEP",
    "enter the remote mepid",
    "host MEP from where to send frame",
    "host mep ID",
    "maintenance domain of mep",
    "enter the name of the domain",
    "configure the te-sid",
    "enter the te-sid ranging from <1-42949675>",
    "Type, Length, Value (TLV)",
    "TLV Type Data",
    "Number of bytes for Data TLV to be sent",
    "TLV Type Test",
    "Test Pattern- abc",
    "Test Pattern- 1234",
    "Test Pattern- a1b2c",
    "Test Pattern- 1a2b3c",
    "bridge-group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;

}
CLI (cfm_pbb_te_tx_loopback2,
    cfm_pbb_te_tx_loopback2_cmd,
    "ping ethernet pbb-te mac MACADDRESS unicast source MEPID "
    "domain DOMAIN_NAME te-sid TE_SID "
    "(tlv ((pbb-te-mip reverse-vid VLAN_VID) | data VAL |test (1 | 2 | 3 |4))|)"
    " bridge backbone",
    "send a ping request",
    "ethernet",
    "configure cfm for pbb-te",
    "destination MAC address",
    "enter the destination mac address in HHHH.HHHH.HHHH format",
    "unicast frame to be sent",
    "source MEP ID",
    "enter the source MEP ID",
    "maintanance domain of mep",
    "enter the name of the domain",
    "configure the te-sid",
    "enter the te-sid ranging from <1-42949675>",
    "Type, Length, Value (TLV)",
    "TLV type PBB-TE-MIP TLV",
    "configure the reverse vlan for the TE-MIP",
    "enter the vlan-id",
    "TLV Type Data",
    "Number of bytes for Data TLV to be sent",
    "TLV Type Test",
    "Test Pattern- abc",
    "Test Pattern- 1234",
    "Test Pattern- a1b2c",
    "Test Pattern- 1a2b3c",
    "Bridge group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
CLI (cfm_pbb_te_tx_lmm,
    cfm_pbb_te_tx_lmm_cmd,
    "ethernet cfm pbb-te lmm mpid MEPID (unicast rmepid RMEPID | multicast) "
    "duration <5-60> domain-name DOMAIN_NAME "
    "te-sid TE_SID (interval (1 | 2 | 3 | 4)|) bridge backbone",
    "configure Layer 2",
    "configure cfm",
    "configure cfm for pbb-te",
    "loss measurement message",
    "host MEP's ID",
    "enter the host MEP's ID",
    "unicast frame to be sent",
    "remote MEP's ID",
    "enter the Remote MEP's ID",
    "multicast frame to be sent",
    "duration for which LMM has to be observed",
    "enter the duration in seconds",
    "maintenance domain of mep",
    "enter the name of the domain",
    "configure the te service instance id",
    "enter the te-sid in the range <1-42949675>",
    "LMM transmission interval",
    "LM Trainsmision interval - 100 ms",
    "LM Trainsmision interval - 10 ms",
    "LM Trainsmision interval - 1 s",
    "LM Trainsmision interval - 10 s",
    "bridge-group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
CLI (cfm_pbb_te_dm_receive,
    cfm_pbb_te_dm_receive_cmd,
    "ethernet cfm pbb-te 1dm receive mepid MEPID "
    "duration DURATION domain-name DOMAIN_NAME te-sid TE_SID "
    "bridge backbone",
    "configure Layer-2",
    "configure cfm",
    "configure cfm for pbb-te",
    "receive frames 1dm frames",
    "configure one-way delay measurment frames",
    "host MEPID",
    "enter host MEPID",
    "configure duration",
    "enter duration in <5-60>seconds",
    "associated to domain",
    "enter the domain name",
    "associated to TE service instance id",
    "enter the te-sid ranging from <1-42949675>",
    "bridge-group commands",
    "backbone")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_te_dm_enable,
    cfm_pbb_te_dm_enable_cmd,
    "ethernet cfm pbb-te (1dm|dvm|dmm) mepid MEPID "
    "(unicast (rmpid MEPID) | multicast) duration DURATION "
    "domain-name DOMAIN_NAME "
    "te-sid TE_SID ((interval (1 | 2 | 3 | 4))|) bridge backbone",
    "configure Layer-2",
    "configure cfm",
    "configure cfm for pbb-te",
    "configure one-way delay measurment(clocks are synchronized)",
    "configure delay variation measurment(clocks are not synchronized)",
    "configure two-way delay measurment",
    "host MEPID",
    "enter host MEPID",
    "configure unicast DM",
    "MEPID of remote MEP",
    "enter remote MEPID",
    "configure multicast DM",
    "configure duration",
    "enter duration in <5-60> seconds",
    "associated to domain",
    "enter the domain name",
    "associated to TE-service instance id",
    "enter the te-sid ranging from <1-42949675>",
    "DMM transmission interval",
    "DM Trainsmision interval - 100 ms",
    "DM Trainsmision interval - 10 ms",
    "DM Trainsmision interval - 1 s",
    "DM Trainsmision interval - 10 s",
    "bridge-group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
CLI (cfm_pbb_te_tst_frame_send,
    cfm_pbb_te_tst_frame_send_cmd,
    "ethernet cfm pbb-te tst mep MEPID (unicast RMEPID | multicast) "
    "pattern (1 | 2 | 3 | 4) domain-name DOMAIN_NAME "
    "te-sid TE_SID (recursive duration <5-60> interval TX_INTERVAL|) "
    "(lck interval TX_INTERVAL (unicast rmepid RMEPID | multicast)|) "
    "bridge backbone",
    "configure Layer-2",
    "configure cfm",
    "configure cfm for pbb-te",
    "test signal",
    "MEP ID of host MEP",
    "enter the MEP ID of host MEP",
    "unicast frame to be sent",
    "enter remote MEP's id",
    "multicast test frame to be sent",
    "TST pattern to be sent",
    "Test Pattern- abc",
    "Test Pattern- 1234",
    "Test Pattern- a1b2c",
    "Test Pattern- 1a2b3c",
    "domain to which MEP is associated",
    "enter the domain name",
    "te-service instance id to which it is associated",
    "enter the te-sid ranging from <1-42949675>",
    "recursively sending TST frames",
    "duration for which test frames will be sent",
    "enter the duration",
    "transimission interval for test frames <1-10 seconds>",
    "enter the TX interval val, default is 1 s",
    "LCK frame will be sent in opposite direction for out-of-service testing",
    "transimission interval for lck, should be same as AIS tx interval",
    "1 or 60 seconds ",
    "unicast LCK frame to be sent",
    "enter the remote mep id for LCK",
    "multicast LCK frame to be sent",
    "bridge-group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }


  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_te_throughput_rx,
    cfm_pbb_te_throughput_rx_cmd,
    "ethernet cfm pbb-te throughput-measurement reception mepid MEPID "
    "duration <1-10> domain-name DOMAIN_NAME te-sid TE_SID "
    "bridge backbone",
    "configure Layer 2",
    "configure cfm",
    "configure cfm for pbb-te",
    "throughput measurement",
    "enable reception of TST frames for throughput measurement",
    "Host MEP ID",
    "Enter the mep id",
    "Maximum duration for waiting for TST frames before timing out",
    "Maximum duration to wait for before timing out <1-10>",
    "domain of the mep",
    "enter the domain name of the mep",
    "configure the te-service instance id",
    "enter the te-sid ranging from <1-42949675>",
    "bridge-group commands",
    "backbone bridge")

{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
#else
CLI (cfm_pbb_te_tx_loopback,
    cfm_pbb_te_tx_loopback_cmd,
    "ping ethernet pbb-te source-mpid SOURCE_MEPID (multicast |rmep-id RMEP_ID)"
    " domain-name DOMAIN_NAME te-sid TE_SID bridge backbone",
    "send a ping request",
    "ethernet",
    "configure cfm for pbb-te",
    "configure source mep id",
    "enter the source mepid",
    "multicast GROUP MAC ESP",
    "configure remote mep id",
    "enter the remote mepid",
    "maintenance domain of mep",
    "enter the name of the domain",
    "configure the te service instance id",
    "enter the te-sid ranging from <1-42949675>",
    "bridge group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);


  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (cfm_pbb_te_tx_loopback2,
    cfm_pbb_te_tx_loopback2_cmd,
    "ping ethernet pbb-te MACADDRESS domain-name DOMAIN_NAME "
    "te-sid TE_SID (source MEPID|) (tlv pbb-te-mip reverse-vid VLAN_ID|) "
    "bridge backbone",
    "send a ping request",
    "ethernet",
    "configure cfm for pbb-te",
    "enter the mac address in HHHH.HHHH.HHHH format",
    "maintenance domain of mep",
    "enter the name of the domain",
    "configure the te service instance id",
    "enter the te-sid ranging from <1-42949675>",
    "Enter the source MEP ID",
    "The Source MEP ID",
    "TLV, Type lenght Value",
    "PBB_TE_MIP type TLV",
    "reverse VLAN_ID for the MIP",
    "enter the reverse VLAN_ID",
    "bridge-group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

#endif /* HAVE_CFM_Y1731 */
CLI (cfm_pbb_te_tx_link_trace,
    cfm_pbb_te_tx_link_trace_cmd,
    "traceroute pbb-te ethernet esp-da-mac ESP_DA_MAC "
    "domain-name DOMAIN_NAME "
    "te-sid TE_SID tlv pbb-te-mip reverse-vid REVERSE_VLANID bridge backbone",
    "traceroute command",
    "configure traceroute for cfm in pbb-te",
    "for layer-2",
    "esp-destination mac of the remote CBP",
    "enter the remote ESP-DA MAC",
    "domain of the destination mep",
    "domain name of destination mep",
    "te-sid to which the destination mep belong",
    "enter the te-sid ranging from <1-42949675>",
    "TLV, Type length Value",
    "PBB_TE_MIP type TLV",
    "reverse VLAN_ID for the MIP",
    "enter the reverse VLAN_ID",
    "bridge-group commands",
    "backbone bridge")
{
  char buf[BUFSIZ];
  int nbytes;
  int sock;

  sock = show_client_socket (imishm, APN_PROTO_ONM);

  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

  /* Send command.  */
  show_line_write (imishm, sock, cli->str, strlen (cli->str), cli->vr->id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
       pal_sock_write (PAL_STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}
#endif /* HAVE_I_BEB && HAVE_B_BEB && HAVE_PBB_TE */


void
imish_cli_show_running_init (int mode)
{
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_full_cli);
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_show_running_config_router_id_cli);
#ifdef HAVE_VR
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_PVR_MAX, 0,
                   &imish_show_running_config_virtual_router_cli);
#endif /* HAVE_VR */
#ifdef HAVE_VRF
  cli_install_imi (ctree, mode, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_show_running_config_vrf_cli);
#endif /* HAVE_VRF */

  /* "show running-config interface" CLIs.  */
  imish_cli_show_running_interface_init (ctree, mode);

  /* "show running-config static route" CLIs. */
  imish_config_static_route_init (ctree, mode);

  /* "show running-config ip mroute" CLIs. */
  imish_config_static_mroute_init (ctree, mode);

  /* "show running-config key chain" CLIs. */
  imish_config_keychain_init (ctree, mode);

  /* "show running-config router" CLIs.  */
  imish_cli_show_running_router_init (ctree, mode);

  /* "show running-config ip/ipv6 pim" CLIs.  */
  imish_cli_show_running_pim_init (ctree, mode);

  /* "show running-config switch" CLIs.  */
  imish_cli_show_running_switch_init (ctree, mode);

  /* "show running-config access-list/prefix-list/route-map" CLIs.  */
  imish_cli_show_running_filter_init (ctree, mode);
}

void
imish_cli_show_init ()
{
  int i;

  /* IMI shell local "show" commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_show_privilege_cli);

  /* IMI "show interface" command. */
#ifndef HAVE_CUSTOM1
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_show_interface_cli);
#endif /* HAVE_CUSTOM1 */

  for (i = 0; i < MAX_MODE; i++)
    imish_cli_show_running_init (i);

  /* Output modifier set up.  */
  cli_install_gen (ctree, MODIFIER_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_modifier_begin_cli);
  cli_install_gen (ctree, MODIFIER_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_modifier_include_cli);
  cli_install_gen (ctree, MODIFIER_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_modifier_exclude_cli);
#ifndef HAVE_NO_LOCAL_FILESYSTEM
  cli_install_gen (ctree, MODIFIER_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_modifier_redirect_cli);
#endif /* HAVE_NO_LOCAL_FILESYSTEM */
  cli_install_gen (ctree, MODIFIER_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &imish_modifier_grep_cli);
  cli_install_gen (ctree, MODIFIER_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &imish_modifier_grep_option_cli);
#ifndef HAVE_NO_LOCAL_FILESYSTEM
  cli_install_gen (ctree, MODIFIER_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_redirect_to_file_cli);
#endif /* HAVE_NO_LOCAL_FILESYSTEM */

#ifdef HAVE_CFM
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_tx_loopback_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_tx_loopback2_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_tx_link_trace_cmd);
#ifdef HAVE_CFM_Y1731
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_tx_lmm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_dm_receive_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_dm_enable_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_tst_frame_send_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_tst_frame_send_cont_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_throughput_rx_cmd);
#endif /* HAVE_CFM_Y1731 */
#endif /* HAVE_CFM */

#if defined (HAVE_CFM) && (defined(HAVE_I_BEB) || defined (HAVE_B_BEB))
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tx_loopback_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tx_loopback2_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tx_link_trace_cmd);
#ifdef HAVE_CFM_Y1731
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tx_loopback_cont1_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tx_loopback_cont2_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tx_lmm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tx_lmm_cont_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_dm_receive_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_dm_enable_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_dm_enable_cont_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tst_frame_send_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_tst_frame_send_cont_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_throughput_rx_cmd);
#endif /* HAVE_CFM_Y1731 */
#endif /* HAVE_CFM  && (defined (HAVE_I_BEB) || defined(HAVE_B_BEB))*/
#if defined HAVE_I_BEB && defined HAVE_B_BEB && defined HAVE_PBB_TE
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_tx_loopback_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_tx_loopback2_cmd);
#ifdef HAVE_CFM_Y1731
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_tx_lmm_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_dm_receive_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_dm_enable_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_tst_frame_send_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_throughput_rx_cmd);
#endif /* HAVE_CFM_Y1731 */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &cfm_pbb_te_tx_link_trace_cmd);

#endif /* HAVE_I_BEB && HAVE_B_BEB && HAVE_PBB_TE */

  /* Shortcut commands.  */
  cli_install_shortcut (ctree, EXEC_MODE, "*p=ping", "p", "ping");
  cli_install_shortcut (ctree, EXEC_MODE, "*h=help", "h", "help");
  cli_install_shortcut (ctree, EXEC_MODE, "*lo=logout", "lo", "logout");
  cli_install_shortcut (ctree, EXEC_MODE, "*u=undebug", "u", "undebug");
  cli_install_shortcut (ctree, EXEC_MODE, "*un=undebug", "un", "undebug");

  /* Set show flag to "show" and "*s=show" node.  */
  for (i = 0; i < MAX_MODE; i++)
    {
      if (i != EXEC_PRIV_MODE)
        cli_install_shortcut (ctree, i, "*s=show", "s", "show");

      if (i != EXEC_MODE)
        {
          cli_set_node_flag (ctree, i, "show", CLI_FLAG_SHOW);
          cli_set_node_flag (ctree, i, "s", CLI_FLAG_SHOW);
        }
    }

  /* Sort CLI commands.  */
  cli_sort (ctree);
}
