/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_VR

#include "line.h"
#include "message.h"
#include "imi_client.h"

#include "imish/imish.h"
#include "imish/imish_line.h"

int imish_config_exit (struct cli *, int, char **);

CLI (imish_login_virtual_router,
     imish_login_virtual_router_cli,
     "login virtual-router WORD",
     "Login as a particular user",
     "Login to a particular VR context",
     CLI_VR_NAME_STR)
{
  struct imi_client *ic = imishm->imh->info;
  struct line *line = &ic->line;

  line->mode = IMISH_MODE;
  imish_line_send (line, "login virtual-router %s", argv[0]);
  imish_line_read (line);

  if (line->code == LINE_CODE_ERROR)
    {
      printf ("No such VR\n");
      return CLI_ERROR;
    }

  line->cli.vr = apn_vr_get_by_id (imishm, line->vr_id);
  SET_FLAG (line->cli.flags, CLI_FROM_PVR);

  return CLI_SUCCESS;
}

ALI (imish_login_virtual_router,
     imish_configure_virtual_router_cli,
     "configure virtual-router WORD",
     "Enter configuration mode",
     CLI_VR_STR,
     CLI_VR_NAME_STR);

ALI (imish_config_exit,
     imish_exit_virtual_router_cli,
     "exit virtual-router",
     "End current mode and down to previous mode",
     CLI_VR_STR);

#ifdef HAVE_MCAST_IPV4
#define IMISH_MCAST_VR_LOAD_STR         "|pimsm|pimdm"

#define IMISH_MCAST_VR_LOAD_HLP_STR                                             \
  , CLI_PIM_STR " " CLI_PIMSM_STR,                                        \
    CLI_PIM_STR " " CLI_PIMDM_STR
#else 
#define IMISH_MCAST_VR_LOAD_STR         ""

#define IMISH_MCAST_VR_LOAD_HLP_STR     ""
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#define IMISH_MCAST6_VR_LOAD_STR         "|pimsm6|pimdm"

#define IMISH_MCAST6_VR_LOAD_HLP_STR                                             \
  , CLI_PIM_STR " " CLI_PIMSM6_STR,                                        \
    CLI_PIM_STR " " CLI_PIMDM6_STR
#else 
#define IMISH_MCAST6_VR_LOAD_STR         ""

#define IMISH_MCAST6_VR_LOAD_HLP_STR     ""
#endif /* HAVE_MCAST_IPV4 */

CLI (imish_virtual_router_load_module,
     imish_virtual_router_load_module_cli,
     "load (ospf|bgp|rip|isis" IMISH_MCAST_VR_LOAD_STR ")",
     "Load module to VR",
     "Open Shortest Path First (OSPF)",
     "Border Gateway Protocol (BGP)",
     "Routing Information Protocol (RIP)",
     "Intermediate System - Intermediate System (IS-IS)" IMISH_MCAST_VR_LOAD_HLP_STR )
{
  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6 
CLI (imish_virtual_router_load_module_ipv6,
     imish_virtual_router_load_module_ipv6_cli,
     "load ipv6 (ospf|rip" IMISH_MCAST6_VR_LOAD_STR ")",
     "Load module to VR",
     "Internet Protocol Version 6",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)" IMISH_MCAST6_VR_LOAD_HLP_STR )
{
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

CLI (imish_no_virtual_router_load_module,
     imish_no_virtual_router_load_module_cli,
     "no load (ospf|bgp|rip|isis" IMISH_MCAST_VR_LOAD_STR ")",
     CLI_NO_STR,
     "Load module to VR",
     "Open Shortest Path First (OSPF)",
     "Border Gateway Protocol (BGP)",
     "Routing Information Protocol (RIP)",
     "Intermediate System - Intermediate System (IS-IS)" IMISH_MCAST_VR_LOAD_HLP_STR )
{
  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (imish_no_virtual_router_load_module_ipv6,
     imish_no_virtual_router_load_module_ipv6_cli,
     "no load ipv6 (ospf|rip" IMISH_MCAST6_VR_LOAD_STR ")",
     CLI_NO_STR,
     "Load module to VR",
     "Internet Protocol Version 6",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)" IMISH_MCAST6_VR_LOAD_HLP_STR )
{
  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

CLI (imish_virtual_router_enable,
     imish_virtual_router_enable_cli,
     "enable-vr",
     "Enable this Virtual Router")
{
  return CLI_SUCCESS;
}

CLI (imish_virtual_router_disable,
     imish_virtual_router_disable_cli,
     "disable-vr",
     "Disable this Virtual Router")
{
  return CLI_SUCCESS;
}

void
imish_vr_cli_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imish_login_virtual_router_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &imish_configure_virtual_router_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, CLI_FLAG_HIDDEN,
                   &imish_exit_virtual_router_cli);

  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imish_virtual_router_load_module_cli);
#ifdef HAVE_IPV6
  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imish_virtual_router_load_module_ipv6_cli);
#endif /* HAVE_IPV6 */
  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imish_no_virtual_router_load_module_cli);
#ifdef HAVE_IPV6
  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imish_no_virtual_router_load_module_ipv6_cli);
#endif /* HAVE_IPV6 */

  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &imish_virtual_router_enable_cli);
  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &imish_virtual_router_disable_cli);
}

#endif /* HAVE_VR */
