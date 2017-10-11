/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"

#include "vtysh.h"
#include "vtysh_exec.h"
#include "vtysh_system.h"
#include "vtysh_line.h"
#include "vtysh_readline.h"

/* IMI shell system parameters.  */
struct vtysh_system param;

/* Shell prompt.  */
char *
vtysh_prompt ()
{
  struct host *host;
  struct pal_utsname names;
  static char prompt[MAXHOSTNAMELEN];
  const char *hostname;
  char sign = '>';

  host = vtyshm->host;

  if (host->name)
    hostname = host->name;
  else
    {
      pal_uname (&names);
      if (pal_strlen (names.nodename) >= (MAXHOSTNAMELEN - 1))
        hostname = "too_long_hostname";
      else
        hostname = names.nodename;
    }

  if (vtysh_privilege_get () > 1)
    sign = '#';

  snprintf (prompt, MAXHOSTNAMELEN, "%s%s%c", hostname,
            cli_prompt_str (vtysh_mode_get ()), sign);

  return prompt;
}

/* Return idle tty time.  */
int
vtysh_ttyidle ()
{
  struct stat ttystat;

  if (fstat (STDIN_FILENO, &ttystat))
    return -1;
  return time (NULL) - ttystat.st_atime;
}

/* Exec-timeout set.  */
void
vtysh_timeout_set (int timeout)
{
  param.timeout = timeout;
  alarm (timeout);
}

/* Exec-timeout get.  */
int
vtysh_timeout_get ()
{
  return param.timeout;
}

/* Exit with restting terminal.  */
void
vtysh_exit (int code)
{
  /* Stop all of jobs.  */
  vtysh_stop_process ();

  /* Close socket to IMI.  */
  vtysh_line_close ();

  /* Reset terminal to cooked mode.  */
  vtysh_reset_terminal (1);
  vtysh_reset_terminal (0);

  /* Exit with the code.  */
  exit (code);
}

/* Get system parameter and set default values.  */
void
vtysh_system_init ()
{
  char *tty;

  /* User name.  */
  param.uid = getuid ();

  /* TTY name.  */
  tty = ttyname (STDIN_FILENO);
  param.tty = tty ? XSTRDUP (MTYPE_TMP, tty) : NULL;

  /* Process ID.  */
  param.pid = getpid ();

  /* Init parameter.  */
  param.timeout = VTYSH_TIMEOUT_DEFAULT;
}
