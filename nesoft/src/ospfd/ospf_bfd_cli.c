/* Copyright (C) 2008  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_BFD

#include "lib.h"
#include "cli.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_cli.h"
#include "ospfd/ospf_bfd_api.h"

CLI (ip_ospf_bfd,
     ip_ospf_bfd_cmd,
     "ip ospf bfd (disable|)",
     OSPF_CLI_IF_STR,
     CLI_BFD_CMD_STR,
     CLI_BFD_DISABLE_CMD_STR)
{
  struct interface *ifp = cli->index;
  int ret;

  if (argc > 0)
    ret = ospf_if_bfd_disable_set (cli->vr->id, ifp->name);
  else
    ret = ospf_if_bfd_set (cli->vr->id, ifp->name);

  return ospf_cli_return (cli, ret);
}

CLI (no_ip_ospf_bfd,
     no_ip_ospf_bfd_cmd,
     "no ip ospf bfd (disable|)",
     CLI_NO_STR,
     OSPF_CLI_IF_STR,
     CLI_BFD_CMD_STR,
     CLI_BFD_DISABLE_CMD_STR)
{
  struct interface *ifp = cli->index;
  int ret;

  if (argc > 0)
    ret = ospf_if_bfd_disable_unset (cli->vr->id, ifp->name);
  else
    ret = ospf_if_bfd_unset (cli->vr->id, ifp->name);

  return ospf_cli_return (cli, ret);
}

CLI (ospf_bfd_all_interfaces,
     ospf_bfd_all_interfaces_cmd,
     "bfd all-interfaces",
     CLI_BFD_CMD_STR,
     CLI_BFD_ALL_INTERFACES_CMD_STR)
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_bfd_all_interfaces_set (cli->vr->id, top->ospf_id);

  return ospf_cli_return (cli, ret);
}

CLI (no_ospf_bfd_all_interfaces,
     no_ospf_bfd_all_interfaces_cmd,
     "no bfd all-interfaces",
     CLI_NO_STR,
     CLI_BFD_CMD_STR,
     CLI_BFD_ALL_INTERFACES_CMD_STR)
{
  struct ospf *top = cli->index;
  int ret;

  ret = ospf_bfd_all_interfaces_unset (cli->vr->id, top->ospf_id);

  return ospf_cli_return (cli, ret);
}

ALI (area_virtual_link,
     ospf_area_virtual_link_fall_over_bfd_cmd,
     "area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D {fall-over bfd}",
     OSPF_CLI_AREA_STR,
     OSPF_CLI_VLINK_STR,
     CLI_BFD_FALL_OVER_CMD_STR,
     CLI_BFD_CMD_STR);

ALI (no_area_virtual_link_options,
     no_ospf_area_virtual_link_fall_over_bfd_cmd,
     "no area (A.B.C.D|<0-4294967295>) virtual-link A.B.C.D {fall-over bfd}",
     CLI_NO_STR,
     OSPF_CLI_AREA_STR,
     OSPF_CLI_VLINK_STR,
     CLI_BFD_FALL_OVER_CMD_STR,
     CLI_BFD_CMD_STR);

void
ospf_bfd_cli_init (struct cli_tree *ctree)
{
  /* "ip ospf bfd (disable|)" commands. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &ip_ospf_bfd_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ip_ospf_bfd_cmd);

  /* "bfd all-interfaces" commands. */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_bfd_all_interfaces_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_bfd_all_interfaces_cmd);

  /* "area virtual-link" commands.  */
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &ospf_area_virtual_link_fall_over_bfd_cmd);
  cli_install_gen (ctree, OSPF_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ospf_area_virtual_link_fall_over_bfd_cmd);
}

#endif /* HAVE_BFD */
