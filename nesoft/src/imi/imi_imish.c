/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "line.h"

#include "imi/imi.h"
#include "imi/imi_api.h"
#include "imi/imi_line.h"
#include "imi/imi_cli.h"


/* Normal procedure.  */
CLI (imi_line_sync,
     imi_line_sync_cli,
     "sync",
     "sync")
{
  struct line *line = cli->line;
  struct host *host = cli->vr->host;
  struct apn_vrf *vrf;

  /* Privilege and exec-timeout configuration must be sent.  */
  imi_line_send (line, "privilege %d", line->privilege);
  imi_line_send (line, "exec-timeout %d %d",
                 line->exec_timeout_min, line->exec_timeout_sec);
  imi_line_send (line, "history max %d", line->maxhist);

  /* These are optional configuration.  */
  if (host->lines >= 0)
    imi_line_send (line, "terminal length %d", host->lines);
  if (! CHECK_FLAG (host->flags, HOST_ADVANCED_VTY))
    imi_line_send (line, "no service advanced-vty");
  if (host->name)
    imi_line_send (line, "hostname %s", host->name);
  else
    imi_line_send (line, "no hostname");

  /* Send FIB ID. */
  vrf = apn_vrf_lookup_default (cli->vr);
  if (vrf != NULL)
    imi_line_send (line, "fib-id %d", vrf->fib_id);

  if (host->motd)
    imi_line_send (line, "banner motd %s", host->motd);

  line->buf[LINE_HEADER_LEN] = '\0';

  return CLI_EOL;
}

CLI (imi_line_sync_vrf,
     imi_line_sync_vrf_cli,
     "sync-vrf (WORD|)",
     "sync-vrf")
{
  struct line *line = cli->line;
  struct apn_vrf *vrf;
  char *name = NULL;

  if (argc > 0)
    name = argv[0];

  vrf = apn_vrf_lookup_by_name (cli->vr, name);
  if (vrf != NULL)
    imi_line_send (line, "fib-vrf-id %d", vrf->fib_id);

  line->buf[LINE_HEADER_LEN] = '\0';
  return CLI_EOL;
}

CLI (imi_line_user,
     imi_line_user_cli,
     "user WORD",
     "User",
     "WORD")
{
  struct line *line = cli->line;

  if (line->user)
    XFREE (MTYPE_TMP, line->user);

  line->user = XSTRDUP (MTYPE_TMP, argv[0]);

  /*code for enabling user account verification */

  if (CHECK_FLAG(line->config,LINE_CONFIG_LOGIN_LOCAL) && (cli->vr->host->users->count!=0))
    return CLI_AUTH_REQUIRED;

  return CLI_SUCCESS;
}

CLI (imi_line_tty,
     imi_line_tty_cli,
     "tty WORD",
     "tty",
     "WORD")
{
  struct line *line = cli->line;

  if (line->tty)
    XFREE (MTYPE_TMP, line->tty);

  line->tty = XSTRDUP (MTYPE_TMP, argv[0]);

  /*code for enabling user account verification */

  if (CHECK_FLAG(line->config,LINE_CONFIG_LOGIN_LOCAL) && (cli->vr->host->users->count!=0))
    return CLI_AUTH_REQUIRED;

  return CLI_SUCCESS;
}

CLI (imi_line_pid,
     imi_line_pid_cli,
     "pid WORD",
     "pid",
     "WORD")
{
  struct line *line = cli->line;
  line->pid = atoi (argv[0]);
  return CLI_SUCCESS;
}

CLI (imi_line_login_virtual_router,
     imi_line_login_virtual_router_cli,
     "login virtual-router (WORD|)",
     "login",
     "Virtual Router",
     "WORD")
{
  struct line *line = cli->line;
  struct apn_vr *vr;
  char *name = NULL;

  if (argc > 0)
    name = argv[0];

#ifdef HAVE_VLOGD
  /* Detach this terminal from frowarding to it any log output. */
  imi_vlog_send_term_cmd (line, NULL,
                          IMI_VLOG_TERM_CMD__DETACH_ALL_VRS);
#endif /* HAVE_VLOGD */

  vr = apn_vr_lookup_by_name (imim, name);
  if (vr == NULL)
    return CLI_ERROR;

   /* If we switch from PVR to VR. */
  if (line->vr_id == 0 && vr->id != 0)
    {
      SET_FLAG (line->cli.flags, CLI_FROM_PVR);
    }
  line->vr_id = vr->id;

  if (name == NULL)
    line->privilege = PRIVILEGE_PVR_MAX;
  else
    line->privilege = PRIVILEGE_MIN;

  if (CHECK_FLAG (line->config, LINE_CONFIG_LOGIN_LOCAL))
    return CLI_AUTH_REQUIRED;

  return CLI_SUCCESS;
}

CLI (imi_line_auth,
     imi_line_auth_cli,
     "login-auth WORD (WORD|)",
     "User login",
     "Login name",
     "Login password")
{
  char *input = NULL;
  struct host *host = cli->vr->host;
  int ret;

  if (argc > 1)
    input = argv[1];

  ret = imi_password_authentication (host, argv[0], input, cli);
  if (ret == CLI_SUCCESS)
    cli->mode = EXEC_MODE;

  return ret;
}

/* Install commands to special mode IMISH_MODE.  This is not visible
   to user.  */
void
imi_imish_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install (ctree, IMISH_MODE, &imi_line_sync_cli);
  cli_install (ctree, IMISH_MODE, &imi_line_sync_vrf_cli);
  cli_install (ctree, IMISH_MODE, &imi_line_user_cli);
  cli_install (ctree, IMISH_MODE, &imi_line_tty_cli);
  cli_install (ctree, IMISH_MODE, &imi_line_pid_cli);
  cli_install (ctree, IMISH_MODE, &imi_line_login_virtual_router_cli);
  cli_install (ctree, IMISH_MODE, &imi_line_auth_cli);
}
