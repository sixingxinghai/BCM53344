/* Copyright (C) 2003  All Rights Reserved.  */

/* Signal handlers for IMI shell.  */

#include <pal.h>
#include <setjmp.h>

#include "vtysh_exec.h"
#include "vtysh_system.h"
#include "vtysh_line.h"
#include "vtysh_readline.h"

/* For sigsetjmp() & siglongjmp().  */
extern sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call.  */
extern int jmpflag;

/* SIGINT handler.  This function takes care user's ^C input.  */
void
vtysh_sigint (int sig)
{
  /* Wait child process.  */
  if (process_list)
    vtysh_wait_for_process ();
  else
    printf ("\n");

  /* Reset terminal.  */
  vtysh_reset_terminal (1);

  /* Send "end" to IMI.  */
  vtysh_line_end ();

  /* Long jump flag check.  */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* SIGTSTP handler.  This function takes care users's ^Z input.  */
void
vtysh_sigtstp (int sig)
{
  if (process_list)
    return;

  /* Reset terminal.  */
  vtysh_reset_terminal (1);

  /* Send "end" to IMI.  */
  vtysh_line_end ();

  printf ("\n");

  /* Long jump flag check.  */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* Time out handler.  */
void
vtysh_sigalarm (int sig)
{
  int idle;
  int timeout;

  /* Get tty idle value and time out value.  */
  idle = vtysh_ttyidle ();
  timeout = vtysh_timeout_get ();

  if (idle >= 0 && idle < timeout)
    /* Reset time out.  */
    alarm (timeout - idle);
  else
    {
      /* Time out.  */
      printf ("\nVty connection is timed out.\n");
      vtysh_exit (0);
    }
}

/* Signale wrapper. */
RETSIGTYPE *
vtysh_signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);

  if (ret < 0) 
    return (SIG_ERR);
  else
    return (osig.sa_handler);
}

/* Initialization of signal handlers. */
void
vtysh_signal_init ()
{
  vtysh_signal_set (SIGINT, vtysh_sigint);
  vtysh_signal_set (SIGTSTP, vtysh_sigtstp);
  vtysh_signal_set (SIGALRM, vtysh_sigalarm);
  vtysh_signal_set (SIGPIPE, SIG_IGN);
  vtysh_signal_set (SIGQUIT, SIG_IGN);
  vtysh_signal_set (SIGTTIN, SIG_IGN);
  vtysh_signal_set (SIGTTOU, SIG_IGN);
}
