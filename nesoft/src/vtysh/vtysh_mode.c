/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "cli_mode.h"

#include "vtysh.h"
#include "vtysh_exec.h"
#include "vtysh_cli.h"
#include "vtysh_parser.h"
#include "vtysh_system.h"

/* Configure mode.  */
CLI (vtysh_configure_terminal,
     vtysh_configure_terminal_cli,
     "configure terminal",
     "Enter configuration mode",
     "Configure from the terminal")
{
  cli->mode = CONFIG_MODE;
  printf ("Enter configuration commands, one per line.  End with CNTL/Z.\n");
  return CLI_SUCCESS;
}

CLI (vtysh_interface,
     vtysh_interface_cli,
     "interface IFNAME",
     "Select an interface to configure",
     CLI_IFNAME_STR)
{
  cli->mode = INTERFACE_MODE;
  return CLI_SUCCESS;
}

#ifdef HAVE_TUNNEL
ALI (vtysh_interface,
     vtysh_interface_tunnel_cli,
     "interface tunnel <0-214748364>",
     "Select an interface to configure",
     "Tunnel interface",
     "Tunnel interface number");
#endif /* HAVE_TUNNEL */

ALI (NULL,
     vtysh_interface_desc_cli,
     "description LINE",
     "Interface specific description",
     "Characters describing this interface");

CLI (vtysh_key_chain,
     vtysh_key_chain_cli,
     "key chain WORD",
     "Authentication key management",
     "Key-chain management",
     "Key-chain name")
{
  cli->mode = KEYCHAIN_MODE;
  return CLI_SUCCESS;
}

CLI (vtysh_key,
     vtysh_key_cli,
     "key <0-2147483647>",
     "Configure a key",
     "Key identifier number")
{
  cli->mode = KEYCHAIN_KEY_MODE;
  return CLI_SUCCESS;
}

CLI (vtysh_route_map,
     vtysh_route_map_cli,
     "route-map WORD (deny|permit) <1-65535>",
     "Create route-map or enter route-map command mode",
     "Route map tag",
     "Route map denies set operations",
     "Route map permits set operations",
     "Sequence to insert to/delete from existing route-map entry")
{
  cli->mode = RMAP_MODE;
  return CLI_SUCCESS;
}

CLI (vtysh_no_route_map_all,
     vtysh_no_route_map_all_cli,
     "no route-map WORD",
     CLI_NO_STR,
     CLI_ROUTEMAP_STR,
     "Route map tag")
{
  return CLI_SUCCESS;
}

CLI (vtysh_no_route_map,
     vtysh_no_route_map_cli,
     "no route-map WORD (deny|permit) <1-65535>",
     CLI_NO_STR,
     CLI_ROUTEMAP_STR,
     "Route map tag",
     "Route map denies set operations",
     "Route map permits set operations",
     "Sequence to insert to/delete from existing route-map entry")
{
  return CLI_SUCCESS;
}

/* Install VTYSH commands. */
void
vtysh_command_init ()
{
  /* CONFIGURE mode.  */
  cli_install_imi (ctree, EXEC_PRIV_MODE, PM_ALL,
                   &vtysh_configure_terminal_cli);

  /* Interface mode.  */
  cli_install_default (ctree, INTERFACE_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_IF, &vtysh_interface_cli);
#ifdef HAVE_TUNNEL
  cli_install_imi (ctree, CONFIG_MODE, PM_TUN_IF, &vtysh_interface_tunnel_cli);
#endif /* HAVE_TUNNEL */
  cli_install_imi (ctree, INTERFACE_MODE, PM_EMPTY, &vtysh_interface_desc_cli);

  /* Key chain mode. */
  cli_install_default (ctree, KEYCHAIN_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_KEYCHAIN, &vtysh_key_chain_cli);

  /* Key mode.  */
  cli_install_default (ctree, KEYCHAIN_KEY_MODE);
  cli_install_imi (ctree, KEYCHAIN_MODE, PM_KEYCHAIN, &vtysh_key_cli);

  /* Route map mode. */
  cli_install_default (ctree, RMAP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RMAP, &vtysh_route_map_cli);
  cli_install_imi (ctree, CONFIG_MODE, PM_RMAP, &vtysh_no_route_map_all_cli);
  cli_install_imi (ctree, CONFIG_MODE, PM_RMAP, &vtysh_no_route_map_cli);

  /* Initialize common modes.  */
  cli_mode_init (ctree);
}
