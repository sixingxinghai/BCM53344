/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>
#include <setjmp.h>

#include "line.h"
#include "message.h"
#include "thread.h"
#include "imi_client.h"

#include "imish/imish_line.h"
#include "imish/imish_parser.h"
#include "imish/imish_cli.h"
#include "imish/imish_system.h"
#include "imish/imish_readline.h"
#include "imish/imish_exec.h"

/* For sigsetjmp () & siglongjmp ().  */
sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call.  */
int jmpflag = 0;
#ifdef HAVE_CUSTOM1
extern int execflag2;
#endif /* HAVE_CUSTOM1 */

/* Global variables . */
struct lib_globals *imishm;

/* Terminal window size. */
u_int16_t imish_win_row_orig;
u_int16_t imish_win_row;
u_int16_t imish_win_col;

int suppress = 0;

void imish_signal_init ();


void
imish_init (char *name)
{
  /* Signal init.  */
  imish_signal_init ();

  /* System init.  */
  imish_system_init ();

  /* Initialize cli.  */
  imish_cli_init (imishm);
  imish_cli_show_init ();

  /* Readline init.  */
  imish_readline_init ();

  /* Set pager.  */
  imish_set_pager ();

  /* Initialize line.  */
  imish_line_init (imishm, name);
}

#define IMISH_BUFSIZ 8192

/* Start IMI shell.  */
void
imish_start (char *name, char *file_name)
{
  struct imi_client *ic;
  result_t ret;

  /* Initialize memory. */
  memory_init (APN_PROTO_IMISH);

  /* Allocate lib globals. */
  imishm = lib_create ("imish");
  if (! imishm)
    {
      fprintf (stderr, "Can't allocate memory: lib_create()\n");
      exit (1);
    }

  ret = lib_start (imishm);
  if (ret < 0)
    {
      lib_stop (imishm);
      fprintf (stderr, "Error: lib_start()\n");
      exit (1);
    }

  /* Assign protocol number.  */
  imishm->protocol = APN_PROTO_IMISH;

  /* When file name is specified, stdout output is suppressed.  */
  if (file_name)
    suppress = 1;

  /* Initialize IMI shell.  */
  imish_init (name);

  /* Set longjump point at here.  */
  sigsetjmp (jmpbuf, 1);
  jmpflag = 1;

  /* Preserve imi_client. */
  ic = imishm->imh->info;

  /* Use file for user input.  */
  if (file_name)
    {
      FILE *fp;
      char buf[IMISH_BUFSIZ + 1];
      struct imi_client *ic = imishm->imh->info;
      struct cli *cli = &ic->line.cli;

      cli->mode = EXEC_MODE;

      fp = pal_fopen (file_name, PAL_OPEN_RO);
      if (fp == NULL)
        return;

      imish_parse ("enable", cli->mode);
      imish_parse ("configure terminal", cli->mode);

      while (pal_fgets (buf, IMISH_BUFSIZ, fp))
        imish_parse (buf, cli->mode);

      exit (0);
    }

  /* Main command loop. */
  while (imish_gets (imish_prompt ()))
    {
      u_int32_t vr_id = ic->line.cli.vr->id;

#ifdef HAVE_CUSTOM1
      execflag2 = 1;
#endif /* HAVE_CUSTOM1 */

      imish_parse (NULL, 0);

      /* Whenever VR context has changed, IMISH sends sync. */
      if (vr_id != ic->line.cli.vr->id)
        {
          ic->line.vr_id = ic->line.cli.vr->id;
          imish_line_sync (&ic->line);
        }
#ifdef HAVE_CUSTOM1
      if (execflag2 == 2)
        imish_exec ("end");
      execflag2 = 0;
#endif /* HAVE_CUSTOM1 */
    }
}

/* Stop IMI shell.  */
void
imish_stop (void)
{
  /* Exit from the IMISH.  */
  imish_exit (0);
}
