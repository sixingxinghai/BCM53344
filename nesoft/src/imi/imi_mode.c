/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "line.h"
#include "host.h"
#include "cli_mode.h"

#include "imi/imi.h"
#include "imi/imi_mode.h"
#include "imi/imi_interface.h"

#ifdef HAVE_CUSTOM1
#include "imi/custom1/imi_mode.h"
#endif /* HAVE_CUSTOM1 */


CLI (imi_enable,
     imi_enable_cli,
     "enable",
     "Turn on privileged mode command")
{
  struct line *line = cli->line;
  struct host *host = cli->vr->host;

  /* First of all check enable password is set.  */
  if (host_password_check (host->enable, host->enable_encrypt, NULL))
    {
      line->privilege = PRIVILEGE_ENABLE (cli->vr);
      return CLI_SUCCESS;
    }

  /* Return auth required.  */
  return CLI_AUTH_REQUIRED;
}

CLI (imi_enable_line,
     imi_enable_line_cli,
     "enable LINE",
     "Turn on privileged mode command",
     "Password")
{
  struct line *line = cli->line;
  struct host *host = cli->vr->host;

  if (host_password_check (host->enable, host->enable_encrypt, argv[0]))
    {
      line->privilege = PRIVILEGE_ENABLE (cli->vr);
      return CLI_SUCCESS;
    }

  /* Return error.  */
  return CLI_ERROR;
}

CLI (imi_disable,
     imi_disable_cli,
     "disable",
     "Turn off privileged mode command")
{
  return CLI_SUCCESS;
}

CLI (imi_interface,
     imi_interface_cli,
     "interface IFNAME",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  char *ifname;
  struct interface *ifp;
  char buf[INTERFACE_NAMSIZ];
  bool_t flag = PAL_FALSE;

  /* Resolve interface name. */
  if (argc == 1)
    ifname = argv[0];
  else
    ifname = cli_interface_resolve (buf, sizeof (buf), argv[0], argv[1]);

  ifp = ifg_lookup_by_name (&imim->ifg, ifname);
  if (ifp == NULL)
    {
      ifp = imi_if_create (ifname);
      flag = PAL_TRUE;
    }

  cli->index = ifp;
  cli->mode = INTERFACE_MODE;

  return CLI_SUCCESS;
}

CLI (imi_virtual_router,
     imi_virtual_router_cli,
     "virtual-router WORD",
     CLI_VR_STR,
     CLI_VR_NAME_STR)
{
  struct apn_vr *vr;

  vr = apn_vr_lookup_by_name (imim, argv[0]);
  if (vr == NULL)
    {
      /* Check restricted names. */
      if (!pal_strcasecmp(argv[0], "all") || !pal_strcasecmp(argv[0], "pvr"))
        {
          cli_out(cli, "%% Name: %s reserved for internal use.", argv[0]);
          return CLI_ERROR;
        }

      vr = apn_vr_get_by_name (imim, argv[0]);
      if (vr != NULL)
        {
          imi_master_init (vr);
#ifdef HAVE_IMISH
          imi_config_read (vr);
#endif /* HAVE_IMISH */
        }
    }

  if (vr == NULL)
    return CLI_ERROR;

  cli->index = vr;
  cli->mode = VR_MODE;

  return CLI_SUCCESS;
}

#ifdef HAVE_TUNNEL
ALI (imi_interface,
     imi_interface_tunnel_cli,
     "interface (tunnel) <0-2147483647>",
     CLI_INTERFACE_STR,
     "Tunnel interface",
     "Tunnel interface number");
#endif /* HAVE_TUNNEL */

void
imi_mode_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* Initialize common modes.  */
  cli_mode_init (ctree);

#ifdef HAVE_IMISH
  /* Enable and disable.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_enable_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_enable_line_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_disable_cli);
#endif /* HAVE_IMISH */

  /* Interface. */
  cli_install_default (ctree, INTERFACE_MODE);

#ifdef HAVE_CUSTOM1
  imi_custom1_mode_init (zg);
#else /* HAVE_CUSTOM1 */
  cli_install_imi (ctree, CONFIG_MODE, PM_IF, PRIVILEGE_NORMAL, 0,
                   &imi_interface_cli);
  cli_set_imi_cmd (&imi_interface_cli, INTERFACE_MODE, CFG_DTYP_INTERFACE);
#endif /* HAVE_CUSTOM1 */

  cli_install_imi (ctree, CONFIG_MODE, PM_NSM, PRIVILEGE_NORMAL, 0,
                   &imi_virtual_router_cli);
  cli_set_imi_cmd (&imi_virtual_router_cli, VR_MODE, CFG_DTYP_IMI_VR);
#ifdef HAVE_TUNNEL
  cli_install_imi (ctree, CONFIG_MODE, PM_TUN_IF, PRIVILEGE_NORMAL, 0,
                   &imi_interface_tunnel_cli);
  cli_set_imi_cmd (&imi_interface_tunnel_cli, INTERFACE_MODE, CFG_DTYP_INTERFACE);
#endif /* HAVE_TUNNEL */
}
