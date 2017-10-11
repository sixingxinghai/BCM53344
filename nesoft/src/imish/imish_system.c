/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "cli.h"
#include "line.h"
#include "message.h"
#include "imi_client.h"

#include "imish/imish.h"
#include "imish/imish_exec.h"
#include "imish/imish_system.h"
#include "imish/imish_line.h"
#include "imish/imish_readline.h"

#ifdef HAVE_CONFIG_COMPARE
#include "imish/imish_config_cmp.h"
#endif /* HAVE_CONFIG_COMPARE */

#if defined(__NetBSD__)
#include "imish/libedit/readline/readline.h"
#endif /* __NetBSD__ */

#define MAXPACOSPROMPTLEN               30

RETSIGTYPE *imish_signal_set (int, void (*)(int));

/* IMI shell system parameters.  */
struct imi_system param;


/* Shell prompt.  */
char *
imish_prompt ()
{
  struct pal_utsname names;
  static char prompt[MAXHOSTNAMELEN+MAXPACOSPROMPTLEN];
  char hostname_tmp[MAXHOSTNAMELEN];
  const char *hostname;
  char sign = '>';
  struct imi_client *ic = imishm->imh->info;

  if (param.hostname)
    hostname = param.hostname;
  else
    {
      pal_uname (&names);
      if (pal_strlen (names.nodename) >= (MAXHOSTNAMELEN - 1))
        hostname = "too_long_hostname";
      else
        hostname = names.nodename;
    }

  if (imish_privilege_get (&ic->line) >= PRIVILEGE_VR_MAX)
    sign = '#';

  /* Output the prompt. */
  snprintf (hostname_tmp, MAXHOSTNAMELEN, "%s", hostname);
#ifdef HAVE_CONFIG_COMPARE
  snprintf (prompt, MAXHOSTNAMELEN+MAXPACOSPROMPTLEN,
            "%s%s%s%c", imish_star () ? "*" : "", hostname_tmp,
            cli_prompt_str (imish_mode_get (&ic->line)), sign);
#else /* HAVE_CONFIG_COMPARE */
  snprintf (prompt, MAXHOSTNAMELEN+MAXPACOSPROMPTLEN, "%s%s%c", hostname_tmp,
            cli_prompt_str (imish_mode_get (&ic->line)), sign);
#endif /* HAVE_CONFIG_COMPARE */

  return prompt;
}

/* Return idle tty time.  */
int
imish_ttyidle ()
{
  struct stat ttystat;

  if (external_command)
    return 0;

#if defined(__NetBSD__)
  if (! process_list)
    return readline_idle_time ();
#endif

  if (fstat (PAL_STDIN_FILENO, &ttystat))
    return -1;
  return time (NULL) - ttystat.st_atime;
}

/* Exec-timeout set.  */
void
imish_timeout_set (int timeout)
{
  param.timeout = timeout;
  alarm (timeout);
}

/* Exec-timeout get.  */
int
imish_timeout_get ()
{
  return param.timeout;
}

/* PID get. */
int
imish_pid_get ()
{
  return param.pid;
}

/* Hostname set.  */
void
imish_hostname_set (char *name)
{
  if (param.hostname)
    XFREE (MTYPE_TMP, param.hostname);
  param.hostname = XSTRDUP (MTYPE_TMP, name);
}

void
imish_hostname_unset ()
{
  if (param.hostname)
    {
      XFREE (MTYPE_TMP, param.hostname);
      param.hostname = NULL;
    }
}

void
imish_fib_id_set (fib_id_t fib_id)
{
  /* Set FIB ID.  */
  param.fib_id = fib_id;

  /* Bind IMISH to the FIB.  */
  pal_vrf_pid_set (param.fib_id, param.pid);
}

/* Exit with resetting terminal.  */
void
imish_exit (int code)
{
  struct imi_client *ic = imishm->imh->info;

  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (imishm);

  /* Stop all of jobs.  */
  imish_stop_process ();

  /* Close socket to IMI.  */
  imish_line_close (&ic->line);

  /* Reset terminal to cooked mode.  */
  imish_reset_terminal (1);
  imish_reset_terminal (0);

  /* Reset terminal length. */
  if (imish_win_row_orig != imish_win_row)
    {
      imish_signal_set (SIGWINCH, SIG_IGN);
      pal_term_winsize_set (PAL_STDIN_FILENO,
                            imish_win_row_orig, imish_win_col);
    }

  /* Stop the system.  */
  lib_stop (imishm);

  /* Exit with the code.  */
  pal_exit (code);
}

/* Get system parameter and set default values.  */
void
imish_system_init ()
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
  param.timeout = IMISH_TIMEOUT_DEFAULT;

  /* Init winsize. */
  pal_term_winsize_get (PAL_STDIN_FILENO, &imish_win_row, &imish_win_col);

  /* Set PATH when parent process does not set it.  */
#ifdef HAVE_CUSTOM1
  putenv ("PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin:/usr/X11R6/bin:/usr/local/sbin:/usr/local/bin");
#endif /* HAVE_CUSTOM1 */
}
