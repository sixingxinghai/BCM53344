/* Copyright (C) 2008  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_BFD

#include "lib.h"
#include "cli.h"

#include "nsm/nsmd.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_cli.h"
#include "nsm/nsm_api.h"
#include "nsm/nsm_bfd_api.h"

CLI (ip_static_bfd,
     ip_static_bfd_cmd,
     "ip static bfd (disable|)",
     CLI_IP_STR,
     "static routes",
     CLI_BFD_CMD_STR,
     CLI_BFD_DISABLE_CMD_STR)
{
  struct interface *ifp = cli->index;
  int ret;
  
  if (argc > 0)
    ret = nsm_ipv4_if_bfd_static_disable_set(cli->vr->id, ifp->name);
  else
    ret = nsm_ipv4_if_bfd_static_set(cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (no_ip_static_bfd,
     no_ip_static_bfd_cmd,
     "no ip static bfd (disable|)",
     CLI_NO_STR,
     CLI_IP_STR,
     "static routes",
     CLI_BFD_CMD_STR,
     CLI_BFD_DISABLE_CMD_STR)
{
  struct interface *ifp = cli->index;
  int ret;

  if (argc > 0)
    ret = nsm_ipv4_if_bfd_static_disable_unset(cli->vr->id, ifp->name);
  else 
    ret = nsm_ipv4_if_bfd_static_unset(cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

CLI (ip_bfd_static_all_interfaces,
     ip_bfd_static_all_interfaces_cmd,
     "ip bfd static all-interfaces",
     CLI_IP_STR,
     CLI_BFD_CMD_STR,
     "static routes",
     CLI_BFD_STATIC_ALL_INTERFACES_CMD_STR)
{
  int ret;

  ret = nsm_ipv4_if_bfd_static_set_all(cli->vr->id);

  return nsm_cli_return (cli, ret);
}

CLI (no_ip_bfd_static_all_interfaces,
     no_ip_bfd_static_all_interfaces_cmd,
     "no ip bfd static all-interfaces",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_BFD_CMD_STR,
     "static routes",
     CLI_BFD_STATIC_ALL_INTERFACES_CMD_STR)
{
  int ret;

  ret = nsm_ipv4_if_bfd_static_unset_all(cli->vr->id);

  return nsm_cli_return (cli, ret);
}

CLI (ip_static_prefix_fall_over_bfd,
     ip_static_prefix_fall_over_bfd_cmd,
     "ip static A.B.C.D/M A.B.C.D fall-over bfd (disable|)",
     CLI_IP_STR,
     "static routes", 
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     CLI_BFD_FALL_OVER_CMD_STR,
     CLI_BFD_CMD_STR,
     CLI_BFD_DISABLE_CMD_STR)
{
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  struct prefix_ipv4 p;
  struct pal_in4_addr nh;
  int ret;

  ret = str2prefix_ipv4 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ret = pal_inet_pton (AF_INET, argv[1], &nh);
  if (! ret)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (argc > 2)
    ret = nsm_ipv4_route_bfd_disable_set (cli->vr->id, vrf_id, &p, &nh);
  else
    ret = nsm_ipv4_route_bfd_set (cli->vr->id, vrf_id, &p, &nh);

  return nsm_cli_return (cli, ret);
}

CLI (no_ip_static_prefix_fall_over_bfd,
     no_ip_static_prefix_fall_over_bfd_cmd,
     "no ip static A.B.C.D/M A.B.C.D fall-over bfd (disable|)",
     CLI_NO_STR,
     CLI_IP_STR,
     "static routes"
     "IP destination prefix (e.g. 10.0.0.0/8)",
     "IP gateway address",
     CLI_BFD_FALL_OVER_CMD_STR,
     CLI_BFD_CMD_STR)
{
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  struct prefix_ipv4 p;
  struct pal_in4_addr nh;
  int ret;

  ret = str2prefix_ipv4 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ret = pal_inet_pton (AF_INET, argv[1], &nh);
  if (! ret)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (argc > 2)
    ret = nsm_ipv4_route_bfd_disable_unset (cli->vr->id, vrf_id, &p, &nh);
  else
    ret = nsm_ipv4_route_bfd_unset (cli->vr->id, vrf_id, &p, &nh);

  return nsm_cli_return (cli, ret);
}

void
nsm_bfd_cli_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_static_bfd_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_static_bfd_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_bfd_static_all_interfaces_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_bfd_static_all_interfaces_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_static_prefix_fall_over_bfd_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_static_prefix_fall_over_bfd_cmd);

}

#endif /* HAVE_L3 */
