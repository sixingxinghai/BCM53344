/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>
#include <lib.h>

#include "if.h"
#include "cli.h"
#include "modbmap.h"
#include "show.h"
#include "line.h"
#include "vty.h"
#include "host.h"
#include "log.h"
#include "network.h"
#include "snprintf.h"
#include "message.h"
#include "version.h"
#ifndef HAVE_IMISH
#include "buffer.h"
#endif /* HAVE_IMISH */

#include "imi/imi.h"
#include "imi/imi_api.h"
#include "imi/imi_line.h"
#include "imi/imi_interface.h"
#include "imi/imi_parser.h"
#include "imi/imi_server.h"
#ifdef HAVE_IMISH
#include "imi/imi_syslog.h"
#endif /* HAVE_IMISH */
#ifdef HAVE_CUSTOM1
#include <utmp.h>
#endif /* HAVE_CUSTOM1 */


/* Externs. */
extern void imi_extracted_cmd_init (struct cli_tree *);
#ifdef HAVE_IMISH
extern void imi_imish_init (struct lib_globals *);
#endif /* HAVE_IMISH */
#ifdef HAVE_VR
extern void imi_vr_cli_init (struct cli_tree *);
#endif /* HAVE_VR */


/* CLI-APIs.  */
int
imi_terminal_monitor_set (u_int32_t vr_id, u_char type, u_int32_t index)
{
  struct imi_server_entry *ise;
  struct imi_server *is = imim->imh->info;

  /* Are we in line/imish mode? */
  if (type >= LINE_TYPE_MAX)
    return IMI_API_SET_ERR_TERMINAL_MONITOR;

  ise = imi_line_entry_lookup (is, type, index);
  if (ise == NULL)
    return IMI_API_SET_ERR_TERMINAL_MONITOR;

  SET_FLAG (ise->line.flags, LINE_FLAG_MONITOR);

#ifdef HAVE_IMISH
  imi_syslog_refresh (is);
#endif /* HAVE_IMISH */

  return IMI_API_SET_SUCCESS;
}

/* Stop terminal monitor.  */
int
imi_terminal_monitor_unset (u_int32_t vr_id, u_char type, u_int32_t index)
{
  struct imi_server_entry *ise;
  struct imi_server *is = imim->imh->info;

  /* Are we in line/imish mode? */
  if (type >= LINE_TYPE_MAX)
    return IMI_API_SET_ERR_TERMINAL_MONITOR;

  ise = imi_line_entry_lookup (is, type, index);
  if (ise == NULL)
    return IMI_API_SET_ERR_TERMINAL_MONITOR;

  /* Set flag and refresh syslog. */
  UNSET_FLAG (ise->line.flags, LINE_FLAG_MONITOR);

#ifdef HAVE_IMISH
  imi_syslog_refresh (is);
#endif /* HAVE_IMISH */

  return IMI_API_SET_SUCCESS;
}


int
imi_cli_return (struct cli *cli, int ret)
{
  char *str;

  switch (ret)
    {
    case IMI_API_SET_SUCCESS:
      return CLI_SUCCESS;

    case IMI_API_SET_ERROR:
      str = "Error in processing command";
      break;
    case IMI_API_SET_ERR_TERMINAL_MONITOR:
      str = "Terminal monitor is not supported";
      break;
#ifdef HAVE_VLOGD
    case IMI_API_SET_ERR_ATTACH_TERM_TO_VR:
      str = "Failure to attach the terminal to VR";
      break;
    case IMI_API_SET_ERR_DETACH_TERM_FROM_VR:
      str = "Failure to detach the terminal from VR";
      break;
#endif /* HAVE_VLOGD. */
    default:
      return CLI_SUCCESS;
    }
  cli_out (cli, "%% %s\n", str);

  return CLI_ERROR;
}

int
imi_password_authentication (struct host *host, char *name,
                             char *password_input,
                             struct cli *cli)
{
  struct host_user *user;

  if (host != NULL)
    {
      user = host_user_lookup (host, name);
      if (user != NULL &&
          host_password_check (user->password,
                               user->password_encrypt, password_input))
        return CLI_SUCCESS;
    }

  return CLI_ERROR;
}

#ifdef HAVE_VLOGD
/* "terminal monitor" for PRIVILEGE_MAX */

CLI (imi_terminal_monitor_pvr,
     imi_terminal_monitor_pvr_cli,
     "terminal monitor (all|WORD|)",
     "Set terminal line parameters",
     "Copy debug output to this terminal line",
     "All Virtual Routers",
     "Name of non-privileged Virtual Router")
{
  int ret;
  struct line *line = cli->line;

  if (line == NULL)
    ret = IMI_API_SET_ERR_TERMINAL_MONITOR;
  else
    {
      if (argc == 0)
        {
          /* Attaching to debug output of the VR in context. */
          ret = imi_vlog_send_term_cmd (line, NULL,
                                        IMI_VLOG_TERM_CMD__ATTACH_A_VR);
        }
      else if (argc == 1)
        {
          if (pal_strcmp (argv[0], "all") == 0)
            {
              /* Attaching to debug output of all current and future VRs. */
              ret = imi_vlog_send_term_cmd (line, argv[0],
                                          IMI_VLOG_TERM_CMD__ATTACH_ALL_VRS);
            }
          else
            {
              /* Attaching to debug output of the specified VR. */
              ret = imi_vlog_send_term_cmd (line, argv[0],
                                            IMI_VLOG_TERM_CMD__ATTACH_A_VR);
            }
        }
      else
        {
          ret = IMI_API_SET_ERROR;
        }
    }
  return imi_cli_return (cli, ret);
}

CLI (imi_terminal_no_monitor_pvr,
     imi_terminal_no_monitor_pvr_cli,
     "terminal no monitor (WORD|)",
     "Set terminal line parameters",
     CLI_NO_STR,
     "Stop forwarding the VR log output to this terminal",
     "A name of non-privileged Virtual Router")
{
  int ret;
  struct line *line = cli->line;

  if (line == NULL)
    ret = IMI_API_SET_ERR_TERMINAL_MONITOR;
  else
    {
      if (argc == 0)
        {
          /* Detaching from all the VRs (including the PVR).
             In result: removing this terminal from VLOG.
           */
          ret = imi_vlog_send_term_cmd (line, NULL,
                                        IMI_VLOG_TERM_CMD__DETACH_ALL_VRS);
        }
      else if (argc == 1)
        {
          /* Detaching from the specified VR. */
          ret = imi_vlog_send_term_cmd (line, argv[0],
                                        IMI_VLOG_TERM_CMD__DETACH_A_VR);
        }
      else
        {
          ret = IMI_API_SET_ERROR;
        }
    }
  return imi_cli_return (cli, ret);
}
#endif

CLI (imi_terminal_monitor,
     imi_terminal_monitor_cli,
     "terminal monitor",
     "Set terminal line parameters",
     "Start forwarding log output to this terminal")
{
  int ret;
  struct line *line = cli->line;

  if (line == NULL)
    ret = IMI_API_SET_ERR_TERMINAL_MONITOR;
  else
#ifdef HAVE_VLOGD
    ret = imi_vlog_send_term_cmd (line, NULL,
                                  IMI_VLOG_TERM_CMD__ATTACH_A_VR);
#else
    ret = imi_terminal_monitor_set (line->vr_id, line->type, line->index);
#endif /* HAVE_VLOGD */
  return imi_cli_return (cli, ret);
}

CLI (imi_terminal_no_monitor,
     imi_terminal_no_monitor_cli,
     "terminal no monitor",
     "Set terminal line parameters",
     CLI_NO_STR,
     "Stop forwarding log output to this terminal")
{
  int ret;
  struct line *line = cli->line;

  if (line == NULL)
    ret = IMI_API_SET_ERR_TERMINAL_MONITOR;
  else
#ifdef HAVE_VLOGD
    /* Remove this termianl info from the VLOGD. */
    ret = imi_vlog_send_term_cmd (line, NULL,
                                  IMI_VLOG_TERM_CMD__DETACH_ALL_VRS);
#else
    ret = imi_terminal_monitor_unset (line->vr_id, line->type, line->index);
#endif /* HAVE_VLOGD */

  return imi_cli_return (cli, ret);
}


/* Line CLIs. */
CLI (imi_line_console,
     imi_line_console_cli,
     "line console <0-0>",
     "Configure a terminal line",
     "Primary terminal line",
     "First Line number")
{
  cli->mode = LINE_MODE;
  cli->type = LINE_TYPE_CONSOLE;
  return CLI_SUCCESS;
}

#ifdef HAVE_LINE_AUX
CLI (imi_line_aux,
     imi_line_aux_cli,
     "line aux <0-0>",
     "Configure a terminal line",
     "Auxiliary line",
     "First Line number")
{
  cli->mode = LINE_MODE;
  return CLI_SUCCESS;
}
#endif /* HAVE_LINE_AUX */

CLI (imi_line_vty,
     imi_line_vty_cli,
     "line vty <0-871> (<0-871>|)",
     "Configure a terminal line",
     "Virtual terminal",
     "First Line number",
     "Last Line number")
{
  struct imi_master *im = cli->vr->proto;
  int i;
  int min;
  int max;

  CLI_GET_INTEGER_RANGE ("first line number", min, argv[0], 0, 871);

  if (argc > 1)
    CLI_GET_INTEGER_RANGE ("last line number", max, argv[1], 0, 871);
  else
    max = min;

  /* Use bigger value for the last line number.  */
  if (min > max)
    {
      cli_out(cli, "%%First line number is greater than second line number \n");
      return CLI_ERROR;
    }
  else
    {
      cli->min = min;
      cli->max = max;
    }

  /* Prepare all lines.  */
  for (i = min; i <= max; i++)
    imi_line_get (im, LINE_TYPE_VTY, i);

  cli->mode = LINE_MODE;
  cli->type = LINE_TYPE_VTY;
  cli->min = min;
  cli->max = max;

  return CLI_SUCCESS;
}

CLI (imi_no_line_vty,
     imi_no_line_vty_cli,
     "no line vty <0-871> (<0-871>|)",
     CLI_NO_STR,
     "Configure a terminal line",
     "Virtual terminal",
     "First Line number",
     "Last Line number")
{
  struct imi_master *im = cli->vr->proto;
  int i;
  int min;
  int max;

  CLI_GET_INTEGER_RANGE ("first line number", min, argv[0], 0, 871);

  if (argc > 1)
    CLI_GET_INTEGER_RANGE ("last line number", max, argv[1], 0, 871);
  else
    max = min;

  /* Use bigger value for the last line number.  */
  if (min > max)
    {
      cli_out(cli, "%%First line number is greater than second line number \n");
      return CLI_ERROR;
    }
  else
    {
      cli->min = min;
      cli->max = max;
    }

  /* Free lines.  */
  for (i = min; i <= max; i++)
    imi_line_free (im, LINE_TYPE_VTY, i);

  return CLI_SUCCESS;
}

CLI (imi_line_privilege,
     imi_line_privilege_cli,
     "privilege level <1-15>",
     "Change privilege level for line",
     "Assign default privilege level for line",
     "Default privilege level for line")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;
  int privilege;

  CLI_GET_INTEGER_RANGE ("privilege level", privilege, argv[0],
                         PRIVILEGE_MIN, PRIVILEGE_MAX);

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        SET_FLAG (line->config, LINE_CONFIG_PRIVILEGE);
        line->privilege = privilege;
      }

  return CLI_SUCCESS;
}

CLI (imi_no_line_privilege,
     imi_no_line_privilege_cli,
     "no privilege level (<1-15>|)",
     CLI_NO_STR,
     "Change privilege level for line",
     "Assign default privilege level for line",
     "Default privilege level for line")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        UNSET_FLAG (line->config, LINE_CONFIG_PRIVILEGE);
        line->privilege = PRIVILEGE_NORMAL;
      }

  return CLI_SUCCESS;
}

#ifdef HAVE_VR
ALI (imi_line_privilege,
     imi_line_privilege_pvr_cli,
     "privilege level (16)",
     "Change privilege level for line",
     "Assign default privilege level for line",
     "Max privilege level for line");

ALI (imi_no_line_privilege,
     imi_no_line_privilege_pvr_cli,
     "no privilege level (16)",
     CLI_NO_STR,
     "Change privilege level for line",
     "Assign default privilege level for line",
     "Max privilege level for line");
#endif /* HAVE_VR */

CLI (imi_line_login,
     imi_line_login_cli,
     "login",
     "Enable password checking")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        SET_FLAG (line->config, LINE_CONFIG_LOGIN);
        UNSET_FLAG (line->config, LINE_CONFIG_LOGIN_LOCAL);
      }

  return CLI_SUCCESS;
}

CLI (imi_no_line_login,
     imi_no_line_login_cli,
     "no login",
     CLI_NO_STR,
     "Enable password checking")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        UNSET_FLAG (line->config, LINE_CONFIG_LOGIN);
        UNSET_FLAG (line->config, LINE_CONFIG_LOGIN_LOCAL);
      }

  return CLI_SUCCESS;
}

CLI (imi_line_login_local,
     imi_line_login_local_cli,
     "login local",
     "Enable password checking",
     "Local password checking")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        UNSET_FLAG (line->config, LINE_CONFIG_LOGIN);
        SET_FLAG (line->config, LINE_CONFIG_LOGIN_LOCAL);
      }

  return CLI_SUCCESS;
}

ALI (imi_no_line_login,
     imi_no_line_login_local_cli,
     "no login local",
     CLI_NO_STR,
     "Enable password checking",
     "Local password checking");

CLI (imi_line_wmi_auth,
     imi_line_wmi_auth_cli,
     "wmi login-auth WORD WORD",
     "Web user",
     "User login",
     "Login name",
     "Login password")
{
  char *input = argv[1];
  struct host *host = cli->vr->host;

  return imi_password_authentication (host, argv[0], input, cli);
}

CLI (imi_history_max_set,
     imi_history_max_set_cli,
     "history max <0-2147483647>",
     "Configure history",
     "Set max value",
     "Number of commands")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;
  int history_var = 0;

  CLI_GET_INTEGER_RANGE ("Max History", history_var, argv[0], 0, 2147483647);

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        SET_FLAG (line->config, LINE_CONFIG_HISTORY);
        line->maxhist = history_var;
      }
  return CLI_SUCCESS;
}

CLI (imi_no_history_max_set,
     imi_no_history_max_set_cli,
     "no history max",
     CLI_NO_STR,
     "Configure history",
     "Set max value")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        UNSET_FLAG (line->config, LINE_CONFIG_HISTORY);
        line->maxhist = SINT32_MAX;
      }
  return CLI_SUCCESS;
}

CLI (imi_line_exec_timeout,
     imi_line_exec_timeout_cli,
     "exec-timeout <0-35791> (<0-2147483>|)",
     "Set the EXEC timeout",
     "Timeout in minutes",
     "Timeout in seconds")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;
  int min, sec = 0;

  CLI_GET_INTEGER_RANGE ("timeout in minutes", min, argv[0], 0, 35791);

  if (argc > 1)
    CLI_GET_INTEGER_RANGE ("timeout in seconds", sec, argv[1], 0, 2147483);

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        if (min != LINE_TIMEOUT_DEFAULT_MIN || sec != LINE_TIMEOUT_DEFAULT_SEC)
          SET_FLAG (line->config, LINE_CONFIG_TIMEOUT);
        line->exec_timeout_min = min;
        line->exec_timeout_sec = sec;
      }

  return CLI_SUCCESS;
}

CLI (imi_no_line_exec_timeout,
     imi_no_line_exec_timeout_cli,
     "no exec-timeout (<0-35791>|) (<0-2147483>|)",
     CLI_NO_STR,
     "Set the EXEC timeout",
     "Timeout in minutes",
     "Timeout in seconds")
{
  struct imi_master *im = cli->vr->proto;
  struct line *line;
  int index;

  for (index = cli->min; index <= cli->max; index++)
    if ((line = imi_line_lookup (im, cli->type, index)))
      {
        UNSET_FLAG (line->config, LINE_CONFIG_TIMEOUT);
        line->exec_timeout_min = LINE_TIMEOUT_DEFAULT_MIN;
        line->exec_timeout_sec = LINE_TIMEOUT_DEFAULT_SEC;
      }

  return CLI_SUCCESS;
}


/* Hostname configuration from IMI.  */
CLI (imi_hostname,
     imi_hostname_cli,
     "hostname WORD",
     "Set system's network name",
     "This system's network name")
{
  s_int32_t ret = 0;
  ret = host_hostname_set (cli->vr, argv[0]);

  if (ret == HOST_NAME_SUCCESS)
    return CLI_SUCCESS;   
#ifdef HAVE_HOSTNAME_CHANGE
  else
    cli_out (cli, "%% %s\n", pal_strerror (ret));
#endif /* HAVE_HOSTNAME_CHANGE */
  return CLI_ERROR;

}

CLI (imi_no_hostname,
     imi_no_hostname_cli,
     "no hostname (WORD|)",
     CLI_NO_STR,
     "Set system's network name",
     "This system's network name")
{
  s_int32_t ret = 0;

  if (argc)
    ret = host_hostname_unset (cli->vr, argv[0]);
  else
    ret = host_hostname_unset (cli->vr, NULL);

  if (ret == HOST_NAME_SUCCESS)
    return CLI_SUCCESS;
  else if (ret == HOST_NAME_NOT_FOUND)
    cli_out (cli, "%% Hostname not found.\n");
  else if (ret == HOST_NAME_NOT_CONFIGURED)
    cli_out (cli, "%% Default Hostname cannot be removed.\n");
#ifdef HAVE_HOSTNAME_CHANGE
  else
    cli_out (cli, "%% %s", pal_strerror (ret));
#endif /* HAVE_HOSTNAME_CHANGE */
  return CLI_ERROR;

}


char *
imi_show_uptime (pal_time_t now, pal_time_t start_time)
{
  pal_time_t diff;
  static char uptime[11];

  diff = now - start_time;

  if (diff > 86400)
    zsnprintf (uptime, sizeof uptime, "%dd%02dh%02dm",
               (diff / 86400) % 365, (diff / 3600) % 24, (diff / 60) % 60);
  else
    zsnprintf (uptime, sizeof uptime, "%02d:%02d:%02d",
               (diff / 3600) % 24, (diff / 60) % 60, diff % 60);

  return uptime;
}

char *
imi_show_tty (char *tty)
{
  int i;

  if (tty)
    for (i = pal_strlen (tty); i > 0; i--)
      if (tty[i - 1] == '/')
        return &tty[i];

  return tty ? tty : " ";
}

/* IMI shell connection display.  */
CLI (show_users,
     show_users_cli,
     "show users",
     CLI_SHOW_STR,
     "Display information about terminal lines")
{
#ifndef HAVE_CUSTOM1
  struct imi_server *is = cli->zg->imh->info;
  struct imi_master *im = cli->vr->proto;
  struct imi_server_entry *ise;
  struct line *line;
  pal_time_t now;
  int i;
  int type;

  /* Current time.  */
  now = pal_time_sys_current (NULL);

  cli_out (cli, "%8s%7s%-11s%-21s%-11s%-10s\n",
           "Line", " ", "User", "Host(s)", "Idle", "Location");

  /* Line info.  */
  for (type = LINE_TYPE_CONSOLE; type < LINE_TYPE_MAX; type++)
    for (i = 0; i < vector_max (im->lines[type]); i++)
      if ((line = vector_slot (im->lines[type], i)))
        if (CHECK_FLAG (line->flags, LINE_FLAG_UP))
          if ((ise = imi_line_entry_lookup (is, type, i)))
            cli_out (cli, " %3d%4s%2d%5s%-11s%-21s%-11s%-10s\n",
                     LINE_TYPE_INDEX (type, i),
                     LINE_TYPE_STR (type), i,
                     " ", " ", "idle",
                     imi_show_uptime (now, ise->connect_time),
                     imi_show_tty (ise->line.tty));

#else /* ! HAVE_CUSTOM1 */
  struct utmp usr;
  FILE *ufp;
  int rtn;

  if (chdir("/dev"))
    return CLI_ERROR;

  if(! (ufp = pal_fopen (_PATH_UTMP, PAL_OPEN_RO)))
    return CLI_ERROR;

  cli_out (cli, "\n%-9.9s %.12s \t%.16s\n", "User", "Login time", "From");
  cli_out (cli, "--------------------------------------\n");

  while (fread ((char *)&usr, sizeof (usr), 1, ufp) == 1)
    if (*usr.ut_name && *usr.ut_line)
      {
        cli_out (cli, "%-9.9s ", usr.ut_name);
        cli_out (cli, "%.12s ", ctime(&usr.ut_time) + 4);

        rtn = pal_strncmp (usr.ut_line, "console", sizeof("console"));
        if (rtn == 0)
          cli_out (cli, "\t%.16s", "console");
        else if(*usr.ut_host)
          cli_out (cli, "\t%.16s", usr.ut_host);

        cli_out (cli, "\n");
      }
  cli_out (cli, "\n");
  pal_fclose (ufp);

  return CLI_SUCCESS;
#endif /* ! HAVE_CUSTOM1 */

  return CLI_SUCCESS;
}

CLI (show_process,
     show_process_cli,
     "show process",
     CLI_SHOW_STR,
     "Process")
{
  int i;
  pal_time_t now;
  struct imi_server_entry *ise;
  struct imi_server_client *isc;
  struct message_handler *ms = cli->zg->imh;
  struct imi_server *is = ms->info;

  /* Current time.  */
  now = pal_time_sys_current (NULL);

  cli_out (cli, " %3s %-9s %10s %-4s\n", "PID", "NAME", "TIME", "FD");

  FOREACH_APN_PROTO (i)
    if (i < vector_max (is->client))
      if ((isc = vector_slot (is->client, i)))
        for (ise = isc->head; ise; ise = ise->next)
          cli_out (cli, " %3d %-9s %10s %-4d\n",
                   isc->module_id, modname_strs (isc->module_id),
                   imi_show_uptime (now, ise->connect_time),
                   ise->me->sock);

  return CLI_SUCCESS;
}

#if defined (HAVE_RSTPD) || defined (HAVE_MSTPD) || defined (HAVE_STPD) || defined (HAVE_RPVST_PLUS)
CLI (show_bridge_protocol,
     show_bridge_protocol_cli,
     "show bridge protocol",
     CLI_SHOW_STR,
     "bridge protocol")
{
  int i;
  struct imi_server *is = cli->zg->imh->info;
  struct imi_server_client *isc;

  FOREACH_APN_PROTO (i)
    if (i < vector_max (is->client))
      if ((isc = vector_slot (is->client, i)))
        {
          IF_BRIDGE_PROTO (isc->module_id)
            {
#ifdef HAVE_VLAN
              cli_out (cli, " %s-vlan\n", modname_strs (isc->module_id));
#else
              cli_out (cli, " %s\n", modname_strs (isc->module_id));
#endif /* HAVE_VLAN */
            }
        }
  return CLI_SUCCESS;
}
#endif /* HAVE_RSTPD || HAVE_MSTPD || HAVE_STPD || HAVE_RPVST_PLUS */

#ifndef HAVE_IMISH

/* Flushing all the configuration sessions states maintained
   in PMs for this vty
*/
void
imi_vty_flush_module(struct vty *vty)
{
  int i;
  struct imi_server_entry *ise;
  struct imi_server_client *isc;
  struct message_handler *ms = imim->imh;
  struct imi_server *is = ms->info;

  for (i=APN_PROTO_NSM; i<vector_max(is->client); i++)
  {
    if (i == APN_PROTO_IMISH || i == APN_PROTO_IMI)
      continue;
    if ((isc = vector_slot (is->client, i)))
      for (ise = isc->head; ise; ise = ise->next)
        {
          ise->line.pid = vty->sock;
          MODBMAP_SET (ise->line.module, i);
          imi_server_send_notify (ise, &ise->line, LINE_CODE_CONFSES_CLR);
          MODBMAP_UNSET (ise->line.module, i);
        }
  }
}


/* Kick VTY out to the CONFIG mode.  */
int
imi_vty_pm_connect (struct apn_vr *vr)
{
  int i;
  struct vty *vty;
  struct vty_server *server = imim->vty_master;

  for (i = 0; i < vector_max (server->vtyvec); i++)
    if ((vty = vector_slot (server->vtyvec, i)))
      if (vr == NULL || vty->cli.vr->id == vr->id)
        if (vty->cli.mode > CONFIG_MODE)
          {
            /* Clear buffer */
            buffer_reset (vty->obuf);
            vty_out (vty, "\n");

            vty->cli.mode = CONFIG_MODE;
            vty_prompt (vty);

            buffer_flush_all (vty->obuf, vty->sock);

            /* */
            imi_vty_flush_module(vty);
          }
  return 0;
}

/* Function registered with VTY server (vty.c)
   to be invoked when the VTY is closed.
 */
void
imi_vty_gone_cb(struct vty *vty)
{
  imi_vty_flush_module(vty);
}

/* vty_command handling for IMI. */
int
imi_vty_command (struct vty *vty, struct cli_node *node, char *buf)
{
  int ret;
  struct line line, reply;
  struct cli *cli = &vty->cli;
  struct cli_tree *ctree = cli->ctree;

  /* Line length check.  */
  if (pal_strlen (buf) >= LINE_BODY_LEN - 1)
    {
      vty_out (vty, "%% Input line is too long\n");
      return CLI_ERROR;
    }

  /* Prepare line structure.  */
  line.zg = cli->zg;
  line.key = cli->cel->key;
  line.module = cli->cel->module;
  line.code = LINE_CODE_COMMAND;
  line.mode = cli->mode;
  line.privilege = cli->privilege;
  line.vr_id = cli->vr->id;
  line.vr = apn_vr_lookup_by_id (cli->zg, line.vr_id);
  line.config_id = 0; /* XXX */
  line.str = &line.buf[LINE_HEADER_LEN];
  line.pid = vty->sock; /* config session id for PM */
  pal_strcpy (line.str, cli->str);

  /* Unset IMI module bit.  */
  MODBMAP_UNSET (line.module, APN_PROTO_IMI);

  reply.buf[LINE_HEADER_LEN] = 0;

  /* Send it to protocol module.  */
  if (! modbmap_isempty (line.module))
    if (imi_server_proxy (&line, &reply, PAL_FALSE) < 0)
      {
        if (reply.code == LINE_CODE_ERROR)
          cli_out (cli, "%s", reply.str);

        return CLI_ERROR;
      }

  /* Execute local function.  */
  if (node->cel->func)
    ret = (*node->cel->func) (cli, ctree->argc, ctree->argv);

  /* Check whether we are switching back into CONFIG_MODE or above.
     If this is the case, we need to flush this VTY's confses states
     to let another user to access the same objects.
     It would be complicated to trace all modules that are
     in specific config mode per IMISH.
   */
  if ((line.mode > CONFIG_MODE) && (cli->mode <= CONFIG_MODE))
    imi_vty_flush_module(vty);

  return CLI_SUCCESS;
}

/* VTY initialization.  */
int
imi_vty_init (struct lib_globals *zg, u_int32_t vty_port)
{
  /* Initialize VTY master.  */
  zg->vty_master = vty_server_new (zg->ctree);
  if (! zg->vty_master)
    return -1;

  /* Set IMI VTY command callback function.  */
  zg->vty_master->imi_vty_command = &imi_vty_command;

  /* Set PM connection callback function.  */
  imi_add_callback (zg, IMI_CALLBACK_CONNECT, imi_vty_pm_connect);

  /* Register for notification when VTY is closed.  */
  vty_serv_add_vty_gone_cb (zg, imi_vty_gone_cb);

  /* Create VTY socket. */
  vty_serv_sock (zg, vty_port);

  /* Initiailze VTY commands.  */
  host_cli_init (zg, zg->ctree);

  return 0;
}
#endif /* ! HAVE_IMISH */

/* Connect to PM for execution of "show" command. */
int
imi_show_protocol (struct apn_vr *vr, module_id_t protocol,
                   char *hdr, char *str, void *out_arg,
                   void (*out_func) (void *, char *, module_id_t))
{
  struct imi_master *im = vr->proto;
  pal_sock_handle_t sock;
  static char buf[BUFSIZ];
  char *start, *ptr, *end;
  int nbytes, rem;
  int ret;

  /* Check the PM availability.  */
  if (! MODBMAP_ISSET (im->module, protocol))
    return RESULT_ERROR;

  /* Open socket.  */
  sock = show_client_socket (imim, protocol);
  if (sock < 0)
    return RESULT_ERROR;

  /* Send command.  */
  ret = show_line_write (imim, sock, str, pal_strlen (str), vr->id);
  if (ret < 0)
    {
      pal_sock_close (imim, sock);
      return RESULT_ERROR;
    }

  /* Print out the header line.  */
  if (hdr != NULL)
    cli_out ((struct cli *) out_arg, hdr);

  /* Get the buffer size and set the buffer starting pointer.  */
  rem = sizeof (buf);
  start = buf;

  /* Read the show output from PM and print out line-by-line.  */
  while (1)
    {
      /* Read the show output.  */
      nbytes = pal_sock_read (sock, start, rem);
      if (nbytes <= 0)
        break;

      /* Sets the end pointer.  */
      end = start + nbytes;

      /* Print out line-by-line.  */
      for (ptr = start = buf; ptr < end; ptr++)
        {
          /* Print out one line.  */
          if (*ptr == '\n')
            {
              /* NULL terminate.  */
              *ptr = '\0';

              /* Print out.  */
              (*out_func) (out_arg, start, protocol);

              /* Start from the next charactor.  */
              start = ptr + 1;
            }
        }

      /* No line feed was found, re-read the remaining part.  */
      rem = end - start;
      pal_mem_cpy (buf, start, rem);
      start = buf + rem;
      rem = sizeof (buf) - rem;
    }

  /* Close socket.  */
  pal_sock_close (imim, sock);

  return RESULT_OK;
}

void
imi_show_gather_display (void *arg, char *line, module_id_t module)
{
  struct cli *cli = (struct cli *)arg;

  cli_out (cli, "%s\n", line);
}

s_int32_t
imi_show_scatter_gather (struct cli *cli)
{
  struct imi_server *is = cli->zg->imh->info;
  struct imi_server_client *isc;
  u_int32_t idx;
  s_int32_t ret;

  ret = 0;

  FOREACH_APN_PROTO (idx)
    if (idx < vector_max (is->client))
      if ((isc = vector_slot (is->client, idx)))
        {
          switch (isc->module_id)
            {
              case APN_PROTO_BGP:
              case APN_PROTO_RIP:
              case APN_PROTO_RIPNG:
              case APN_PROTO_OSPF:
              case APN_PROTO_OSPF6:
                {
                  cli_out (cli, "%4s%s:\n", "", modname_strs (isc->module_id));
                  imi_show_protocol (cli->vr, isc->module_id, NULL, cli->str, cli,
                                     &imi_show_gather_display);
                }
                break;
              default:
                break;
            }
        }

  return ret;
}

#ifdef HAVE_RELOAD
/* Reboot command.  */
CLI (imi_reload,
     imi_reload_cli,
     "reload",
     "Halt and perform a cold restart")
{
  pal_reboot ();

  return CLI_SUCCESS;
}

#ifdef HAVE_REBOOT
ALI (imi_reload,
     imi_reboot_cli,
     "reboot",
     "Halt and perform a cold restart");
#endif /* HAVE_REBOOT */
#endif /* HAVE_RELOAD */

/* Init IMI related features. */
int
imi_cli_init (u_int16_t vty_port)
{
  struct cli_tree *ctree;
  struct show_server *imi_show = NULL;

  /* Initialize CLI ctree.  */
  ctree = imim->ctree;
  if (ctree == NULL)
    return -1;

  /* Initialize Show server.  */
  imi_show = show_server_init (imim);
  if (imi_show == NULL)
    return -1;

#ifdef HAVE_IMISH
  /* Initialize default host CLIs.  */
  host_default_cli_init (ctree);
#else
  /* Initialize IMI vty. */
  imi_vty_init (imim, vty_port);
#endif /* ! HAVE_IMISH. */

  /* Users CLIs. */
  host_user_cli_init (ctree);

#ifdef HAVE_VR
  host_vr_cli_init (imim->ctree);
#endif /* HAVE_VR */

  show_server_show_func (imi_show, imi_show_scatter_gather);

  imi_extracted_cmd_init (ctree);

  /* Install interface description commnads. */
  cli_install_imi (ctree, INTERFACE_MODE, PM_IFDESC, PRIVILEGE_NORMAL, 0,
                   &interface_desc_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IFDESC, PRIVILEGE_NORMAL, 0,
                   &no_interface_desc_cli);

#ifdef HAVE_IMISH
  /* IMISH_MODE CLIs.  */
  imi_imish_init (imim);

  /* Install local command. */

#ifdef HAVE_VLOGD
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imi_terminal_monitor_pvr_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imi_terminal_no_monitor_pvr_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_terminal_monitor_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_terminal_no_monitor_cli);
#else

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_terminal_monitor_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_terminal_no_monitor_cli);
#endif /* HAVE_VLOGD.*/


  /* Install line commands.  */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_line_console_cli);
  cli_set_imi_cmd (&imi_line_console_cli, LINE_MODE, CFG_DTYP_IMI_LINE);
#ifdef HAVE_LINE_AUX
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_line_aux_cli);
  cli_set_imi_cmd (&imi_line_aux_cli, LINE_MODE, CFG_DTYP_IMI_LINE);
#endif /* HAVE_LINE_AUX */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_line_vty_cli);
  cli_set_imi_cmd (&imi_line_vty_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_no_line_vty_cli);

  cli_install_default (ctree, LINE_MODE);
  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_line_privilege_cli);
  cli_set_imi_cmd (&imi_line_privilege_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_no_line_privilege_cli);
#ifdef HAVE_VR
  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imi_line_privilege_pvr_cli);
  cli_set_imi_cmd (&imi_line_privilege_pvr_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imi_no_line_privilege_pvr_cli);
#endif /* HAVE_VR */
  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_line_login_cli);
  cli_set_imi_cmd (&imi_line_login_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_no_line_login_cli);
  cli_set_imi_cmd (&imi_no_line_login_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_history_max_set_cli);
  cli_set_imi_cmd (&imi_history_max_set_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_no_history_max_set_cli);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_line_exec_timeout_cli);
  cli_set_imi_cmd (&imi_line_exec_timeout_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_no_line_exec_timeout_cli);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_line_login_local_cli);
  cli_set_imi_cmd (&imi_line_login_local_cli, LINE_MODE, CFG_DTYP_IMI_LINE);

  cli_install_gen (ctree, LINE_MODE, PRIVILEGE_NORMAL, 0,
                   &imi_no_line_login_local_cli);
  cli_set_imi_cmd (&imi_no_line_login_local_cli, LINE_MODE, CFG_DTYP_IMI_LINE);
#endif /* HAVE_IMISH */

  /* Hostname command.  */
  cli_install_imi (ctree, CONFIG_MODE, PM_HOSTNAME, PRIVILEGE_NORMAL, CLI_FLAG_IGN_PM_DOWN_ERR,
                   &imi_hostname_cli);
  cli_set_imi_cmd (&imi_hostname_cli, CONFIG_MODE, CFG_DTYP_IMI_HOST);

  cli_install_imi (ctree, CONFIG_MODE, PM_HOSTNAME, PRIVILEGE_NORMAL, CLI_FLAG_IGN_PM_DOWN_ERR,
                   &imi_no_hostname_cli);

#ifdef HAVE_VR
  /* Virtual Router commands. */
  imi_vr_cli_init (ctree);
#endif /* HAVE_VR */

#ifdef HAVE_IMISH
  /* Show IMI shell connection status.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_users_cli);
#endif /* HAVE_IMISH */

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_HIDDEN,
                   &imi_line_wmi_auth_cli);

  /* "show process" */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &show_process_cli);

#ifdef HAVE_RELOAD
  /* Reboot command.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &imi_reload_cli);
#ifdef HAVE_REBOOT
  cli_install_hidden (ctree, EXEC_PRIV_MODE, &imi_reboot_cli);
#endif /* HAVE_REBOOT */
#endif /* HAVE_RELOAD */

#if defined (HAVE_RSTPD) || defined (HAVE_MSTPD) || defined (HAVE_STPD) || defined (HAVE_RPVST_PLUS)
  /* "show bridge-protocol" */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_HIDDEN,
                   &show_bridge_protocol_cli);
#endif /* HAVE_RSTPD || HAVE_MSTPD || HAVE_STPD || HAVE_RPVST_PLUS */

  return 0;
}
