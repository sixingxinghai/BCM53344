/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>
#include <termios.h>

#include "lib.h"
#include "cli.h"
#include "line.h"
#include "message.h"
#include "imi_client.h"
#include "libedit/readline/readline.h"
#include "libedit/histedit.h"

#include "imish/imish.h"
#include "imish/imish_cli.h"
#include "imish/imish_exec.h"
#include "imish/imish_line.h"

/* User input line.  */
char *input_line = NULL;
char *input_y_or_n = NULL;

/* User input related information.  */
extern EditLine *e;

#define MTYPE_IMISH_INPUT_LINE  MTYPE_TMP


/* Read one line of user input.  This is for interactive command.  */
char *
imish_gets (char *prompt)
{
  /* Free last user command.  */
  if (input_line)
    {
      XFREE (MTYPE_IMISH_INPUT_LINE, input_line);
      input_line = NULL;
    }
     
  /* Readline.  */
  input_line = readline (prompt);

  /* Add to history.  */
  if (input_line && *input_line)
    add_history (input_line);
     
  if (input_line == NULL)
    {
      printf ("\n");
      input_line = XSTRDUP (MTYPE_IMISH_INPUT_LINE, "exit\n");
    }

  return input_line;
}

/* Read one line of user input.  This is for interactive command.  */
char *
imish_get_y_or_n (char *prompt)
{
  /* Free last user command.  */
  if (input_y_or_n)
    {
      XFREE (MTYPE_IMISH_INPUT_LINE, input_y_or_n);
      input_y_or_n = NULL;
    }
     
  /* Readline.  */
  input_y_or_n = readline (prompt);

  /* Add to history.  */
  if (input_y_or_n == NULL)
    {
      printf ("\n");
      input_y_or_n = XSTRDUP (MTYPE_IMISH_INPUT_LINE, "exit\n");
    }

  return input_y_or_n;
}

/* Show history.  */
void
imish_history_show ()
{
  history_show ();
}

/* Reset terimal.  Mode should be 1 when system exit.  */
void
imish_reset_terminal (int mode)
{
  rl_reset_terminal (mode);
}

/* We don't care about the point of the cursor when '?' is typed. */
int
imish_describe_func (void *val, int argc, char **argv)
{
  const LineInfo *li;
  struct cli cli;
  struct imi_client *ic = imishm->imh->info;

  /* Login check.  */
  if (ctree->mode == 0)
    return 0;

  /* Prepare CLI structure.  */
  pal_mem_set (&cli, 0, sizeof (struct cli));
  cli.zg = imishm;
  cli.out_func = (CLI_OUT_FUNC) fprintf;
  cli.out_val = stdout;

  /* Current user input structure.  */
  li = el_line(e);

  /* User input.  */
  rl_line_buffer = (char *) li->buffer;
  rl_end = li->lastchar - li->buffer;
  rl_line_buffer[rl_end] = '\0';

  cli_describe (&cli, ctree, imish_mode_get (&ic->line),
                imish_privilege_get (&ic->line), rl_line_buffer, 80);

  return 0;
}

/* Put more for paging.  */
int
imish_describe (int arg1, int arg2)
{
  struct cli *cli;
  struct imi_client *ic = imishm->imh->info;

  cli = &ic->line.cli;
  imish_more (imish_describe_func, cli, 0, NULL);
  return 0;
}

/* result of cmd_complete_command() call will be stored here and used
   in imish_completion () in order to put the space in correct
   places.  */
int complete_status;

/* This function is called from libedit library for completion.  */
static char *
imish_completion_matches (const char *text, int state)
{
  static char **matched = NULL;
  static int index = 0;
  const LineInfo *li;
  struct imi_client *ic = imishm->imh->info;

  li = el_line(e);

  rl_line_buffer = (char *) li->buffer;
  rl_end = li->lastchar - li->buffer;
  rl_line_buffer[rl_end] = '\0';

  if (li->cursor != li->lastchar)
    return NULL;

  /* First call. */
  if (! state)
    {
      index = 0;

      /* Perform completion as one shot. */
      matched = cli_complete (ctree, imish_mode_get (&ic->line),
                              imish_privilege_get (&ic->line), rl_line_buffer);

      /* Update complete status.  */
      if (matched && matched[0] && matched[1] == NULL)
        complete_status = 1;
      else
        complete_status = 0;
    }

  /* This function is called until this function returns NULL.  */
  if (matched && matched[index])
    return matched[index++];

  return NULL;
}

/* Glue function for imish_completion_matches.  */
char **
imish_completion (char *text, int start, int end)
{
  char **matches;

  matches = completion_matches (text, imish_completion_matches);

  return matches;
}

/* Disable readline's filename completion.  */
char *
imish_completion_empty (const char *ignore, int invoking_key)
{
  return NULL;
}

/* Set advanced VTY mode.  */
void
imish_advance_set ()
{
  ctree->advanced = 1;
  rl_advance_set ();
}

/* Unset advanced VTY mode.  */
void
imish_advance_unset ()
{
  ctree->advanced = 0;
  rl_advance_unset ();
}

/* Initialization for completion and desription.  */
void
imish_readline_init ()
{
  /* Input mode set to RAW mode. */
  readline_pacos_set_excl_raw_input_mode ();

  /* '?' is the description function.  */
  rl_bind_key ('?', imish_describe);

  /* Disable filename completion.  */
  rl_completion_entry_function = imish_completion_empty;

  /* Register completion function.  */
  rl_attempted_completion_function = (CPPFunction *) imish_completion;
}
