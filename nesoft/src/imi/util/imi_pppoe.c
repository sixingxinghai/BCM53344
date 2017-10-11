/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_PPPOE

#include "imi/pal_fw.h"
#include "imi/pal_pppoe.h"

#include "lib.h"
#include "if.h"
#include "cli.h"
#include "prefix.h"

#include "imi/imi.h"
#include "imi/imi_interface.h"
#include "imi/util/imi_pppoe.h"


static int imi_pppoe_config_refresh (struct cli *cli);
/* API functions for CLI/SNMP.  */

/* API: Enable PPPOE. */
int
imi_pppoe_enable (struct cli *cli)
{
  int ret;

  ret = imi_pppoe_check_param (cli);
  if (ret != IMI_API_SUCCESS)
    return ret;

  PPPOE->enabled = PAL_TRUE;

#ifdef HAVE_IMI_WAN_STATUS
  imi_wan_status_set (WAN_PPPoE);
#endif /* HAVE_IMI_WAN_STATUS */

  return imi_pppoe_config_refresh (cli);
}

/* API: Disable PPPOE. */
int
imi_pppoe_disable (struct cli *cli)
{
  int ret;

  if (PPPOE->enabled == PAL_FALSE)
    return IMI_API_SUCCESS;
  else
    PPPOE->disabling = PAL_TRUE;

  PPPOE->enabled = PAL_FALSE;

  ret = imi_pppoe_config_refresh (cli);
  PPPOE->disabling = PAL_FALSE;
  return ret;
}

/* API: Set PPPOE Interface. */
int
imi_pppoe_set_interface (char *if_name, struct cli *cli)
{
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (imim);

  ifp = if_lookup_by_name (&vr->ifm, if_name);
  if (ifp == NULL)
    return IMI_API_INVALID_ARG_ERROR;
  else
    {
      PPPOE->ifp = ifp;
      return imi_pppoe_config_refresh (cli);
    }
}

/* API: Unset PPPOE Interface. */
int
imi_pppoe_unset_interface (struct cli *cli)
{
  if (PPPOE->ifp == NULL)
    return IMI_API_NOT_SET_ERROR;

  PPPOE->ifp = NULL;
  return imi_pppoe_config_refresh (cli);
}

/* API: Set PPPOE Interface mtu. */
int
imi_pppoe_set_mtu (char *mtu_str, struct cli *cli)
{
  u_int32_t mtu;
  char *endptr = NULL;

  mtu = pal_strtos32 (mtu_str, &endptr, 10);
  if ((mtu == LONG_MIN) || (mtu == LONG_MAX))
    return IMI_API_INVALID_ARG_ERROR;
  if (mtu == PPPOE->mtu)
    return IMI_API_SUCCESS;

  PPPOE->mtu = mtu;

  return imi_pppoe_config_refresh (cli);
}

/* API: Set username in PPPOE. */
int
imi_pppoe_set_username (struct cli *cli, char *username)
{
  if (PPPOE->username)
    XFREE (MTYPE_IMI_STRING, PPPOE->username);
  PPPOE->username = NULL;
  PPPOE->username = XSTRDUP (MTYPE_IMI_STRING, username);
  if (PPPOE->username == NULL)
    {
      cli_out (cli, "%% Username not set. No memory available.\n");
      return IMI_API_MEMORY_ERROR;
    }

  return imi_pppoe_config_refresh (cli);
}

/* API: Set password in PPPOE. */
int
imi_pppoe_set_password (struct cli *cli, char *passwd)
{
  if (PPPOE->passwd)
    XFREE (MTYPE_IMI_STRING, PPPOE->passwd);
  PPPOE->passwd = NULL;
  PPPOE->passwd = XSTRDUP (MTYPE_IMI_STRING, passwd);
  if (PPPOE->passwd == NULL)
    {
      cli_out (cli, "%% Password not set. No memory available.\n");
      return IMI_API_MEMORY_ERROR;
    }

  return imi_pppoe_config_refresh (cli);
}

/* API: Show PPPOE. */
void
imi_pppoe_show_all (struct cli *cli)
{
  if (! PPPOE)
    return;

  /* Enable/Disable. */
  if (PPPOE->enabled)
    cli_out (cli, "PPPOE Enabled.\n");
  else
    cli_out (cli, "PPPOE Disabled.\n");

  /* Username. */
  if (PPPOE->username)
    cli_out (cli, " pppoe username %s\n", PPPOE->username);

  /* Password. Do we show this? */
  if (PPPOE->passwd)
    cli_out (cli, " pppoe password %s\n", PPPOE->passwd);

  /* Interface. */
  if (PPPOE->ifp)
    cli_out (cli, " interface %s point-to-point\n", PPPOE->ifp->name);

  /* MTU. */
  if (PPPOE->mtu != 1492)
    cli_out (cli, " ip mtu %d\n", PPPOE->mtu);

  return;
}

/* Util functions. */

/* Check if other parameters to enable PPPOE are set. */
int
imi_pppoe_check_param (struct cli* cli)
{
  if (! PPPOE->username)
    {
      cli_out (cli, "%% Please set the username first.\n");
      return IMI_API_NOT_SET_ERROR;
    }
  if (! PPPOE->passwd)
    {
      cli_out (cli, "%% Please set the password first.\n");
      return IMI_API_NOT_SET_ERROR;
    }
  if (! PPPOE->ifp)
    {
      cli_out (cli, "%% Please set the interface first.\n");
      return IMI_API_NOT_SET_ERROR;
    }

  return IMI_API_SUCCESS;
}


/* Refresh PPPOE PAL configuration. */
static int
imi_pppoe_config_refresh (struct cli *cli)
{
  int ret;

  /* Call PAL API to perform this action for different OS. */
  ret = pal_pppoe_refresh (imig->zg, PPPOE);
  switch (ret)
    {
    case IMI_API_FILE_OPEN_ERROR:
      cli_out (cli, "%% File open Error.\n");
      break;
    case IMI_API_FILE_SEEK_ERROR:
      cli_out (cli, "%% File seek Error.\n");
      break;
    case IMI_API_FILE_WRITE_ERROR:
      cli_out (cli, "%% File write Error.\n");
      break;
    }
  return IMI_API_SUCCESS;
}

CLI (service_pppoe,
     service_pppoe_cli,
     "service pppoe",
     "Setup miscellaneous service",
     PPPOE_STR)
{
  cli->mode = PPPOE_MODE;

  if (! PPPOE)
    imi_pppoe_service ();

  return CLI_SUCCESS;
}

CLI (no_service_pppoe,
     no_service_pppoe_cli,
     "no service pppoe",
     CLI_NO_STR,
     "Setup miscellaneous service",
     PPPOE_STR)
{
  imi_pppoe_no_service ();

  return CLI_SUCCESS;
}

CLI (disable_pppoe,
     disable_pppoe_cli,
     "disable",
     "Disable PPPOE")
{
  imi_pppoe_disable (cli);

  return CLI_SUCCESS;
}

CLI (enable_pppoe,
     enable_pppoe_cli,
     "enable",
     "Enable PPPoE")
{
  int ret;

  ret = imi_pppoe_enable (cli);
  if (ret != IMI_API_SUCCESS)
    return CLI_ERROR;

  return CLI_SUCCESS;
}

/* Specify interface on which PPPOE is enabled. */
CLI (interface_p2p,
     interface_p2p_cli,
     "interface IFNAME point-to-point",
     "Select an interface to configure",
     "Interface's name",
     "Point to Point Interface")
{
  int ret;
  char buf[INTERFACE_NAMSIZ];
  char *ifname;

  /* Resolve ifname. */
  if (argc == 1)
    ifname = argv[0];
  else
    ifname = cli_interface_resolve (buf, INTERFACE_NAMSIZ, argv[0], argv[1]);

  ret = imi_pppoe_set_interface (ifname, cli);
  switch (ret)
    {
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% No such interface.\n");
      break;
    case IMI_API_SUCCESS:
    default:
      break;
    }

  return CLI_SUCCESS;
}

/* Specify interface on which PPPOE is enabled. */
CLI (no_interface_p2p,
     no_interface_p2p_cli,
     "no interface (IFNAME point-to-point|)",
     CLI_NO_STR,
     "Unconfigure interface",
     "Interface's name",
     "Point to Point Interface")
{
  int ret;

  ret = imi_pppoe_unset_interface (cli);
  if (ret < 0)
    {
      switch (ret)
        {
        case IMI_API_NOT_SET_ERROR:
          cli_out (cli, "%% No interface configured for PPPoE.\n");
          break;
        default:
          break;
        }

      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Ethernet MTU is 1500 by default -- 1492 + PPPOE headers = 1500 */
CLI (ip_mtu,
     ip_mtu_cli,
     "ip mtu <576-1492>",
     CLI_IP_STR,
     "Maximum Transmission Unit",
     "MTU")
{
  int ret;

  if (argc == 1)
    ret = imi_pppoe_set_mtu (argv[0], cli);
  else
    ret = imi_pppoe_set_mtu ("1492", cli);
  switch (ret)
    {
    case IMI_API_SUCCESS:
    default:
      break;
    }

  return CLI_SUCCESS;
}

ALI (ip_mtu,
     no_ip_mtu_cli,
     "no ip mtu",
     CLI_NO_STR,
     CLI_IP_STR,
     "Maximum Transmission Unit");

CLI (pppoe_username,
     pppoe_username_cli,
     "pppoe username WORD",
     PPPOE_STR,
     "Username for Login",
     "Username value for Login")
{
  int ret;

  ret = imi_pppoe_set_username (cli, argv[0]);
  switch (ret)
    {
    case IMI_API_MEMORY_ERROR:
    case IMI_API_SUCCESS:
    default:
      break;
    }

  return CLI_SUCCESS;
}

CLI (pppoe_password,
     pppoe_password_cli,
     "pppoe password WORD",
     PPPOE_STR,
     "Password for Login",
     "Password value for Login")
{
  imi_pppoe_set_password (cli, argv[0]);

  return CLI_SUCCESS;
}

/* Show all. */
CLI (show_pppoe,
     show_pppoe_cli,
     "show pppoe",
     CLI_SHOW_STR,
     CLI_PPPOE_STR)
{
  /* Call API. */
  imi_pppoe_show_all (cli);

  return CLI_SUCCESS;
}

/* Init / Shutdown.  */

/* Configuration encode/write for PPPOE Client. */
s_int32_t
imi_pppoe_config_encode (struct imi_pppoe *pppoe, cfg_vect_t *cv)
{
  /* PPPOE enabled? */
  if (PPPOE && (PPPOE->username || PPPOE->passwd || PPPOE->ifp || PPPOE->mtu))
    {
      /* Enable message. */
      cfg_vect_add_cmd (cv, "service pppoe\n");

      /* Username. */
      if (PPPOE->username)
        cfg_vect_add_cmd (cv, " pppoe username %s\n", pppoe->username);

      /* Password. Do we show this? */
      if (PPPOE->passwd)
        cfg_vect_add_cmd (cv, " pppoe password %s\n", pppoe->passwd);

      /* Interface. */
      if (PPPOE->ifp)
        cfg_vect_add_cmd (cv, " interface %s point-to-point\n", pppoe->ifp->name);

      /* MTU. */
      if (PPPOE->mtu != 1492)
        cfg_vect_add_cmd (cv, " ip mtu %d\n", pppoe->mtu);

      /* Enabled? */
      if (PPPOE->enabled)
        cfg_vect_add_cmd (cv, " enable\n");

      cfg_vect_add_cmd (cv, "!\n");
    }

  return 0;
}


s_int32_t
imi_pppoe_config_write (struct cli *cli)
{
  cli->cv = cfg_vect_init(cli->cv);
  imi_pppoe_config_encode(PPPOE, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}


void
imi_pppoe_no_service ()
{
  if (! PPPOE)
    return;

  /* Set shutdown flag so PAL is aware. */
  PPPOE->shutdown_flag = PAL_TRUE;

  XFREE (MTYPE_IMI_STRING, PPPOE->username);
  PPPOE->username = NULL;
  XFREE (MTYPE_IMI_STRING, PPPOE->passwd);
  PPPOE->passwd = NULL;

  /* Stop PAL. */
  if (PPPOE->enabled)
    pal_pppoe_stop (IMI->zg, PPPOE->pal_pppoe);

  /* Free DNS. */
  XFREE (MTYPE_IMI_PPPOE, PPPOE);
  PPPOE = NULL;

  return;
}

void
imi_pppoe_service ()
{
  struct imi_pppoe *pppoe;

  /* Allocate PPPOE data structure.  */
  pppoe = XCALLOC (MTYPE_IMI_PPPOE, sizeof (struct imi_pppoe));
  if (! pppoe)
    exit (0);

  /* Initialize PPPOE data structure.  */
  pppoe->enabled = PAL_FALSE;
  pppoe->username = NULL;
  pppoe->passwd = NULL;
  pppoe->ifp = NULL;
  pppoe->mtu = 1492;
  pppoe->sysconfig = PAL_FALSE;
  pppoe->shutdown_flag = PAL_FALSE;
  imig->pppoe = pppoe;

  /* Initialize PPPOE PAL.  */
  PPPOE->pal_pppoe = pal_pppoe_start (IMI->zg, pppoe);
}

void
imi_pppoe_cli_init ()
{
  struct cli_tree *ctree = imim->ctree;

  /* Install PPPoE node. */
  cli_install_default (ctree, PPPOE_MODE);

  /* Install PPPoE commands.  */
  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &service_pppoe_cli);
  cli_set_imi_cmd (&service_pppoe_cli, PPPOE_MODE, CFG_DTYP_IMI_PPPOE);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_service_pppoe_cli);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &disable_pppoe_cli);
  cli_set_imi_cmd (&disable_pppoe_cli, PPPOE_MODE, CFG_DTYP_IMI_PPPOE);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &enable_pppoe_cli);
  cli_set_imi_cmd (&enable_pppoe_cli, PPPOE_MODE, CFG_DTYP_IMI_PPPOE);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &interface_p2p_cli);
  cli_set_imi_cmd (&interface_p2p_cli, PPPOE_MODE, CFG_DTYP_IMI_PPPOE);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_interface_p2p_cli);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_mtu_cli);
  cli_set_imi_cmd (&ip_mtu_cli, PPPOE_MODE, CFG_DTYP_IMI_PPPOE);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_mtu_cli);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &pppoe_username_cli);
  cli_set_imi_cmd (&pppoe_username_cli, PPPOE_MODE, CFG_DTYP_IMI_PPPOE);

  cli_install_imi (ctree, PPPOE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &pppoe_password_cli);
  cli_set_imi_cmd (&pppoe_password_cli, PPPOE_MODE, CFG_DTYP_IMI_PPPOE);

  /* Show commands.  */
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_pppoe_cli);
}

void
imi_pppoe_init ()
{
  imi_pppoe_cli_init ();
}

/* Shutdown PPPOE. */
void
imi_pppoe_shutdown ()
{
  imi_pppoe_no_service ();
}

#endif /* HAVE_PPPOE */
