/* Copyright (C) 2003  All Rights Reserved.  */

/* Signal handlers for IMI shell.  */

#include <pal.h>
#include <setjmp.h>

#include "line.h"
#include "message.h"
#include "imi_client.h"

#include "imish/imish.h"
#include "imish/imish_exec.h"
#include "imish/imish_system.h"
#include "imish/imish_line.h"
#include "imish/imish_readline.h"

/* For sigsetjmp() & siglongjmp().  */
extern sigjmp_buf jmpbuf;
#ifdef HAVE_CUSTOM1
int execflag2 = 0;
#endif /* HAVE_CUSTOM1 */

/* Flag for avoid recursive siglongjmp() call.  */
extern int jmpflag;

/* Signal set function declaration. */
RETSIGTYPE *imish_signal_set (int, void (*)(int));

/* SIGINT handler.  This function takes care user's ^C input.  */
void
imish_sigint (int sig)
{
  struct imi_client *ic = imishm->imh->info;

#ifdef HAVE_CUSTOM1
  if (execflag2 != 0)
    {
      execflag2 = 2;
      return;
    }
#endif /* HAVE_CUSTOM1 */

  /* Reinstall signal. */
  imish_signal_set (SIGINT, imish_sigint);

  /* Wait child process.  */
  if (process_list)
    imish_wait_for_process ();
  else
    printf ("\n");

  /* Reset terminal.  */
  imish_reset_terminal (1);

  /* Send "end" to IMI.  */
  imish_line_end (&ic->line);

  /* Long jump flag check.  */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* SIGTSTP handler.  This function takes care users's ^Z input.  */
void
imish_sigtstp (int sig)
{
  struct imi_client *ic = imishm->imh->info;

  /* Reinstall signal. */
  imish_signal_set (SIGTSTP, imish_sigtstp);

  if (process_list)
    return;

  /* Reset terminal.  */
  imish_reset_terminal (1);

  /* Send "end" to IMI.  */
  imish_line_end (&ic->line);

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
imish_sigalarm (int sig)
{
  int idle;
  int timeout;

  /* Get tty idle value and time out value.  */
  idle = imish_ttyidle ();
  timeout = imish_timeout_get ();

  if (idle >= 0 && idle < timeout)
    /* Reset time out.  */
    alarm (timeout - idle);
  else
    {
      /* Time out.  */
      printf ("\nVty connection is timed out.\n");
      imish_exit (0);
    }
}

/* Notification from IMI for synchronization.  */
void
imish_user1 (int sig)
{
  struct imi_client *ic = imishm->imh->info;

  /* Send "sync" message to IMI.  IMI will send all of pending
     commands to shell.  */
  imish_line_sync (&ic->line);

  return;
}

/* SIGUSR2 handler.  This function takes care to move user to CONFIG_MODE.  */
void
imish_user2 (int sig)
{
  struct imi_client *ic = imishm->imh->info;

  /* Reinstall signal. */
  imish_signal_set (SIGUSR2, imish_user2);

  /* Wait child process.  */
  if (process_list)
    imish_wait_for_process ();
  else
    printf ("\n");

  /* Reset terminal.  */
  imish_reset_terminal (1);

  /* Send "exit" to IMI to move to CONFIG_MODE.  */
  imish_line_config (&ic->line);

  /* Long jump flag check.  */
  if (! jmpflag)
    return;

  jmpflag = 0;

  /* Back to main command loop. */
  siglongjmp (jmpbuf, 1);
}

/* Notification of imish exit from IMI. */
void
imish_sigterm (int sig)
{
  /* Exit imish. */
  printf ("\n%% Connection is closed by administrator!\n");
  imish_exit (0);
  return;
}

void
imish_sigwinch (int sig)
{
  u_int16_t row;

  pal_term_winsize_get (PAL_STDIN_FILENO, &row, &imish_win_col);
  if (row != imish_win_row)
    {
      imish_win_row_orig = row;
      pal_term_winsize_set (PAL_STDIN_FILENO, imish_win_row, imish_win_col);
    }

  return;
}

/* Signale wrapper. */
RETSIGTYPE *
imish_signal_set (int signo, void (*func)(int))
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
imish_signal_init ()
{
  imish_signal_set (SIGINT, imish_sigint);
  imish_signal_set (SIGTSTP, imish_sigtstp);
  imish_signal_set (SIGALRM, imish_sigalarm);
  imish_signal_set (SIGUSR1, imish_user1);
  imish_signal_set (SIGUSR2, imish_user2);
  imish_signal_set (SIGPIPE, SIG_IGN);
  imish_signal_set (SIGQUIT, SIG_IGN);
  imish_signal_set (SIGTTIN, SIG_IGN);
  imish_signal_set (SIGTTOU, SIG_IGN);
  imish_signal_set (SIGTERM, imish_sigterm);
  imish_signal_set (SIGHUP,  SIG_IGN);
}
