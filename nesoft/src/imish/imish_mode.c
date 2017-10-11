/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "cli_mode.h"

#include "imish/imish.h"
#include "imish/imish_exec.h"
#include "imish/imish_cli.h"
#include "imish/imish_parser.h"
#include "imish/imish_system.h"

#ifdef HAVE_CUSTOM1
void imish_custom1_mode_init ();
#endif /* HAVE_CUSTOM1 */

#define SNMP_STRING "Snmp information"


/* Configure mode.  */
CLI (imish_configure_terminal,
     imish_configure_terminal_cli,
     "configure terminal",
     "Enter configuration mode",
     "Configure from the terminal")
{
  cli->mode = CONFIG_MODE;
  if (! suppress)
    printf ("Enter configuration commands, one per line.  End with CNTL/Z.\n");
  return CLI_SUCCESS;
}

/* DHCP server mode.  */
#ifdef HAVE_DHCP_SERVER
CLI (ip_dhcp_pool,
     ip_dhcp_pool_cli,
     "ip dhcp pool WORD",
     CLI_IP_STR,
     "DHCP Server configuration",
     "Configure DHCP server address pool",
     "Pool identifier")
{
  cli->mode =DHCPS_MODE;
  return CLI_SUCCESS;
}
#endif /* HAVE_DHCP_SERVER */

/* PPPoE mode.  */
#ifdef HAVE_PPPOE
CLI (service_pppoe,
     service_pppoe_cli,
     "service pppoe",
     "Setup miscellaneous service",
     CLI_PPPOE_STR)
{
  cli->mode = PPPOE_MODE;
  return CLI_SUCCESS;
}

/* Enable PPPoE. */
IMI_ALI (NULL,
   imish_enable_pppoe_cli,
   "enable",
   "Enable PPPoE");

/* Disable PPPoE. */
IMI_ALI (NULL,
   imish_disable_pppoe_cli,
   "disable",
   "Disable PPPoE");
#endif /* HAVE_PPPOE */

CLI (imish_line_console,
     imish_line_console_cli,
     "line console <0-0>",
     "Configure a terminal line",
     "Primary terminal line",
     "First Line number")
{
  cli->mode = LINE_MODE;
  return CLI_SUCCESS;
}

#ifdef HAVE_LINE_AUX
CLI (imish_line_aux,
     imish_line_aux_cli,
     "line aux <0-0>",
     "Configure a terminal line",
     "Auxiliary line",
     "First Line number")
{
  cli->mode = LINE_MODE;
  return CLI_SUCCESS;
}
#endif /* HAVE_LINE_AUX */

CLI (imish_line_vty,
     imish_line_vty_cli,
     "line vty <0-871> (<0-871>|)",
     "Configure a terminal line",
     "Virtual terminal",
     "First Line number",
     "Last Line number")
{
  cli->mode = LINE_MODE;
  return CLI_SUCCESS;
}

CLI (imish_interface,
     imish_interface_cli,
     "interface IFNAME",
     "Select an interface to configure",
     CLI_IFNAME_STR)
{
  cli->mode = INTERFACE_MODE;
  return CLI_SUCCESS;
}

#ifdef HAVE_TUNNEL
ALI (imish_interface,
     imish_interface_tunnel_cli,
     "interface tunnel <0-214748364>",
     "Select an interface to configure",
     "Tunnel interface",
     "Tunnel interface number");
#endif /* HAVE_TUNNEL */

IMI_ALI (NULL,
   imish_interface_desc_cli,
   "description LINE",
   "Interface specific description",
   "Characters describing this interface");

IMI_ALI (NULL,
   imish_no_interface_desc_cli,
   "no description",
   CLI_NO_STR,
   "Interface specific description");

CLI (imish_key_chain,
     imish_key_chain_cli,
     "key chain WORD",
     "Authentication key management",
     "Key-chain management",
     "Key-chain name")
{
  cli->mode = KEYCHAIN_MODE;
  return CLI_SUCCESS;
}

CLI (imish_no_key_chain,
     imish_no_key_chain_cli,
     "no key chain WORD",
     CLI_NO_STR,
     "Authentication key management",
     "Key-chain management",
     "Key-chain name")
{
  cli->mode = CONFIG_MODE;
  return CLI_SUCCESS;
}

CLI (imish_key,
     imish_key_cli,
     "key <0-2147483647>",
     "Configure a key",
     "Key identifier number")
{
  cli->mode = KEYCHAIN_KEY_MODE;
  return CLI_SUCCESS;
}

CLI (imish_no_key,
     imish_no_key_cli,
     "no key <0-2147483647>",
     CLI_NO_STR,
     "Configure a key",
     "Key identifier number")
{
  cli->mode = KEYCHAIN_MODE;
  return CLI_SUCCESS;
}

CLI (imish_route_map,
     imish_route_map_cli,
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

CLI (imish_no_route_map,
     imish_no_route_map_cli,
     "no route-map WORD ((deny|permit) <1-65535>|)",
     CLI_NO_STR,
     CLI_ROUTEMAP_STR,
     "Route map tag",
     "Route map denies set operations",
     "Route map permits set operations",
     "Sequence to insert to/delete from existing route-map entry")
{
  return CLI_SUCCESS;
}


/* Install IMISH commands. */
void
imish_command_init ()
{
  /* CONFIGURE mode.  */
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_VR_MAX, 0,
                   &imish_configure_terminal_cli);

  /* DHCP server mode. */
#ifdef HAVE_DHCP_SERVER
  cli_install_default (ctree, DHCPS_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &ip_dhcp_pool_cli);
#endif /* HAVE_DHCP_SERVER */

  /* PPPoE mode. */
#ifdef HAVE_PPPOE
  cli_install_default (ctree, PPPOE_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &service_pppoe_cli);
  cli_install_imi (ctree, PPPOE_MODE, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_enable_pppoe_cli);
  cli_install_imi (ctree, PPPOE_MODE, PM_EMPTY, PRIVILEGE_MAX, 0,
                   &imish_disable_pppoe_cli);
#endif /* HAVE_PPPOE */

  /* LINE mode.  */
  cli_install_default (ctree, LINE_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_EMPTY, PRIVILEGE_NORMAL, 0,
                   &imish_line_console_cli);
#ifdef HAVE_LINE_AUX
  cli_install_imi (ctree, CONFIG_MODE, PM_EMPTY, PRIVILEGE_NORMAL, 0,
                   &imish_line_aux_cli);
#endif /* HAVE_LINE_AUX */
  cli_install_imi (ctree, CONFIG_MODE, PM_EMPTY, PRIVILEGE_NORMAL, 0,
                   &imish_line_vty_cli);

  /* Interface mode.  */
  cli_install_default (ctree, INTERFACE_MODE);
#ifdef HAVE_CUSTOM1
  imish_custom1_mode_init ();
#else /* HAVE_CUSTOM1 */
  cli_install_imi (ctree, CONFIG_MODE, PM_IF, PRIVILEGE_NORMAL, 0,
                   &imish_interface_cli);
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_TUNNEL
  cli_install_imi (ctree, CONFIG_MODE, PM_TUN_IF, PRIVILEGE_NORMAL, 0,
                   &imish_interface_tunnel_cli);
#endif /* HAVE_TUNNEL */

  cli_install_imi (ctree, INTERFACE_MODE, PM_IFDESC, PRIVILEGE_NORMAL, 0,
                   &imish_interface_desc_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IFDESC, PRIVILEGE_NORMAL, 0,
                   &imish_no_interface_desc_cli);

  /* Key chain mode. */
  cli_install_default (ctree, KEYCHAIN_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_key_chain_cli);
  cli_install_imi (ctree, CONFIG_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_no_key_chain_cli);
  cli_install_imi (ctree, KEYCHAIN_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_key_chain_cli);
  cli_install_imi (ctree, KEYCHAIN_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_no_key_chain_cli);

  /* Key mode.  */
  cli_install_default (ctree, KEYCHAIN_KEY_MODE);
  cli_install_imi (ctree, KEYCHAIN_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_key_cli);
  cli_install_imi (ctree, KEYCHAIN_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_no_key_cli);
  cli_install_imi (ctree, KEYCHAIN_KEY_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_key_cli);
  cli_install_imi (ctree, KEYCHAIN_KEY_MODE, PM_KEYCHAIN, PRIVILEGE_NORMAL, 0,
                   &imish_no_key_cli);

  /* Route map mode. */
  cli_install_default (ctree, RMAP_MODE);
  cli_install_imi (ctree, CONFIG_MODE, PM_RMAP, PRIVILEGE_NORMAL, 0,
                   &imish_route_map_cli);
  cli_install_imi (ctree, CONFIG_MODE, PM_RMAP, PRIVILEGE_NORMAL, 0,
                   &imish_no_route_map_cli);
  /* Initialize common modes.  */
  cli_mode_init (ctree);
}
