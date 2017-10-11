/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_cli.c - Implementation of VLOG CLI handlers.
*/

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "linklist.h"
#include "log.h"
#include "cli.h"
#include "snprintf.h"
#include "line.h"
#include "vrep.h"

#include "vlogd.h"


CLI (vlog_vr_instance_internal,
     vlog_vr_instance_internal_cli,
     "vr-instance WORD <1-255>",
     "VR instance name to id mapping command",
     "VR name",
     "VR identifier")
{
  /* This is an internal CLI command.
     IMI sends it after receiving VR name-id mapping from NSM or after
     starting the IMI client-server connection with VLOGd.
  */
  int vr_id;
  int ret;

  /* Decode the VR id. */
  vr_id =  cmd_str2int (argv[1], &ret);
  if (vr_id < 0)
    return CLI_ERROR;

  if (vlog_add_vr(argv[0], vr_id) != ZRES_OK)
    return CLI_SUCCESS;
  else
    return CLI_ERROR;
}

CLI (no_vlog_vr_instance_internal,
     no_vlog_vr_instance_internal_cli,
     "no vr-instance WORD <1-255>",
     CLI_NO_STR,
     "VR instance name to id mapping command",
     "VR name",
     "VR identifier")
{
  int vr_id;
  int ret;

  /* Decode the VR id. */
  vr_id =  cmd_str2int (argv[1], &ret);
  if (vr_id < 0)
    return CLI_ERROR;

  vlog_del_vr(argv[0], vr_id);

  return CLI_SUCCESS;
}



CLI (vlog_terminal_vr_internal,
     vlog_terminal_vr_internal_cli,
     "vlog terminal-vr WORD (all|WORD|)",
     "VLOG internal command",
     "VLOG terminal internal command",
     "TTY name or encoded file descriptor",
     "All virtual routers",
     "Name of non-privileged VR")
{
  /* This is an internal CLI command.
     IMI sends it after receiving "terminal monitor (all|WORD|).
     Here we decode parameters and invoke vlog_vr_add_term().
  */
  struct line *line = cli->line;
  int ret = CLI_SUCCESS;

  if (line->vr_id == 0)
  { /* This is PVR - we expect one or two parameters */
    if (argc == 2)
    {
      if (pal_strcmp(argv[1], "all") == 0)
      {
        ret = vlog_add_term_to_vr(line->vr_id, argv[0], "all");
      }
      else
      {
        ret = vlog_add_term_to_vr(line->vr_id, argv[0], argv[1]);
      }
    }
    else
    {  /* Add terminal to the PVR user "home" VR only.*/
      ret = vlog_add_term_to_vr(line->vr_id, argv[0], NULL);
    }
  }
  else
  { /* This is non-PVR - we do expect 1 parameter only. */
    if (argc == 1)
    {
      ret = vlog_add_term_to_vr(line->vr_id, argv[0], NULL);
    }
    else
    {
      ret = CLI_ERROR;
    }
  }
  return ret;
}


CLI (no_vlog_terminal_vr_internal,
     no_vlog_terminal_vr_internal_cli,
     "no vlog terminal-vr WORD (WORD|)",
     CLI_NO_STR,
     "VLOG internal command",
     "VLOG terminal internal command",
     "Terminal name",
     ">all< or name of non-privileged VR")
{
  /* This is an internal CLI command.
     IMI sends it after receiving "terminal monitor (all|WORD|).
     Here we decode parameters and invoke vlog_vr_add_term().
  */
  struct line *line = cli->line;
  int ret = CLI_SUCCESS;

  if (argc == 1)
  {
    /* Remove this terminal completely from teh VLOG. */
    ret = vlog_del_term_from_vr(line->vr_id, argv[0], NULL);
  }
  else
  {
    /* Remove this termianl from one non-PVR. */
    ret = vlog_del_term_from_vr(line->vr_id, argv[0], argv[1]);
  }
  return ret;
}


/* Callback to display a single line of show output.
   Must be of the vrep_show_cb_t.
 */
static ZRESULT
_vlog_cli_show_row_cb(intptr_t user_ref, char *str)
{
  struct cli *cli = (struct cli *)user_ref;
  cli_out((struct cli *)cli, str);
  return ZRES_OK;
}


CLI (show_vlog_terminals,
     show_vlog_terminals_cli,
     "show vlog terminals",
     "Show command",
     "Vlog show command",
     "Show vlog terminals command")
{
  ZRESULT res = ZRES_OK;

  res = vlog_show_terminals(_vlog_cli_show_row_cb, (intptr_t)cli);
  if (res != ZRES_OK)
  {
    cli_out(cli, "$$ Failure to complete successfully\n");
    return (CLI_ERROR);
  }
  else
    return (CLI_SUCCESS);
}

CLI (show_vlog_vrs,
     show_vlog_vrs_cli,
     "show vlog virtual-routers",
     "Show command",
     "Vlog show command",
     "Show vlog virtual-routers")
{
  ZRESULT res = ZRES_OK;

  res = vlog_show_vrs(_vlog_cli_show_row_cb, (intptr_t)cli);
  if (res != ZRES_OK)
  {
    cli_out(cli, "$$ Failure to complete successfully\n");
    return (CLI_ERROR);
  }
  else
    return (CLI_SUCCESS);
}

CLI (show_vlog_clients,
     show_vlog_clients_cli,
     "show vlog clients",
     "Show command",
     "Vlog show command",
     "Show vlog clients")
{
  ZRESULT res = ZRES_OK;

  res = vlog_show_clients(_vlog_cli_show_row_cb, (intptr_t)cli);
  if (res != ZRES_OK)
  {
    cli_out(cli, "$$ Failure to complete successfully\n");
    return (CLI_ERROR);
  }
  else
    return (CLI_SUCCESS);
}

CLI (show_vlog_all,
     show_vlog_all_cli,
     "show vlog all",
     "Show command",
     "Vlog show command",
     "Show vlog clients")
{
  ZRESULT res = ZRES_OK;

  res = vlog_show_all(_vlog_cli_show_row_cb, (intptr_t)cli);

  if (res != ZRES_OK)
  {
    cli_out(cli, "$$ Failure to complete successfully\n");
    return (CLI_ERROR);
  }
  else
    return (CLI_SUCCESS);
}

CLI (reset_vlog_file,
     reset_vlog_file_cli,
     "reset log file",
     "Reset command",
     "Reset log related command",
     "Resetting the currently open log file")
{
  struct line *line = cli->line;
  ZRESULT res = ZRES_OK;

  res = vlog_reset_log_file(line->vr_id);

  if (res != ZRES_OK)
  {
    cli_out(cli, "$$ Log file is not configured\n");
    return (CLI_ERROR);
  }
  return (CLI_SUCCESS);
}

/* VLOG CLI initialization.  */
void
vlog_cli_init (struct lib_globals *zg)
{
#ifndef HAVE_IMISH
  struct cli_tree *vty_ctree = zg->vty_master->ctree;
#endif  /* !HAVE_IMISH. */
  struct cli_tree *ctree = zg->ctree;

#ifdef HAVE_IMISH
  /* Initialize debug mode CLIs. */
  cli_install_hidden (ctree, EXEC_MODE, &vlog_terminal_vr_internal_cli);
  cli_install_hidden (ctree, EXEC_MODE, &no_vlog_terminal_vr_internal_cli);
#endif /* HAVE_IMISH. */

  cli_install_hidden (ctree, EXEC_MODE, &vlog_vr_instance_internal_cli);
  cli_install_hidden (ctree, EXEC_MODE, &no_vlog_vr_instance_internal_cli);

#ifndef HAVE_IMISH
  /* Default host CLIs.  */
  host_user_cli_init (vty_ctree);
  host_default_cli_init (vty_ctree);

  vlog_cli_vty_init_term (vty_ctree);
#endif /* !HAVE_IMISH. */

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &show_vlog_terminals_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &show_vlog_vrs_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &show_vlog_clients_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0, &show_vlog_all_cli);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &reset_vlog_file_cli);
}

