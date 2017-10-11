/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>
#include <termios.h>

#include "cli.h"
#include "libedit/readline/readline.h"
#include "libedit/histedit.h"

#include "vtysh.h"
#include "vtysh_cli.h"
#include "vtysh_exec.h"
#include "vtysh_line.h"

/* User input line.  */
char *input_line = NULL;

/* User input related information.  */
extern EditLine *e;

#define MTYPE_VTYSH_INPUT_LINE  MTYPE_TMP

/* Extern vtysh line.  */
extern struct line *vtysh_line;

/* Read one line of user input.  This is for interactive command.  */
char *
vtysh_gets (char *prompt)
{
  /* Free last user command.  */
  if (input_line)
    {
      XFREE (MTYPE_VTYSH_INPUT_LINE, input_line);
      input_line = NULL;
    }
     
  /* Readline.  */
  input_line = readline (prompt);

  /* Add to history.  */
  if (input_line && *input_line)
    add_history (input_line);
     
  return input_line;
}

/* Show history.  */
void
vtysh_history_show ()
{
  history_show ();
}

/* Reset terimal.  Mode should be 1 when system exit.  */
void
vtysh_reset_terminal (int mode)
{
  rl_reset_terminal (mode);
}

/* We don't care about the point of the cursor when '?' is typed. */
int
vtysh_describe_func (void *val, int argc, char **argv)
{
  const LineInfo *li;
  struct cli cli;

  /* Login check.  */
  if (ctree->mode == 0)
    return 0;

  /* Prepare CLI structure.  */
  pal_mem_set (&cli, 0, sizeof (struct cli));
  cli.zg = vtyshm;
  cli.out_func = (CLI_OUT_FUNC) fprintf;
  cli.out_val = stdout;

#ifdef HAVE_VR
  /* Set vrf_id. */
  ctree->vrf_id = cli.vrf_id = imiline.vrf_id;
#endif /* HAVE_VR */

  /* Current user input structure.  */
  li = el_line(e);

  /* User input.  */
  rl_line_buffer = (char *) li->buffer;
  rl_end = li->lastchar - li->buffer;
  rl_line_buffer[rl_end] = '\0';

  cli_describe (&cli, ctree, vtysh_mode_get(), vtysh_privilege_get(),
                rl_line_buffer, 80);

  return 0;
}

/* Put more for paging.  */
int
vtysh_describe (int arg1, int arg2)
{
  struct cli *cli;
  cli = &vtysh_line->cli;
  vtysh_more (vtysh_describe_func, cli, 0, NULL);
  return 0;
}

/* result of cmd_complete_command() call will be stored here and used
   in vtysh_completion () in order to put the space in correct
   places.  */
int complete_status;

/* This function is called from libedit library for completion.  */
static char *
vtysh_completion_matches (const char *text, int state)
{
  static char **matched = NULL;
  static int index = 0;
  const LineInfo *li;

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
      matched = cli_complete (ctree,
                              vtysh_mode_get(),
                              vtysh_privilege_get (),
                              rl_line_buffer);

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

/* Glue function for vtysh_completion_matches.  */
char **
vtysh_completion (char *text, int start, int end)
{
  char **matches;

  matches = completion_matches (text, vtysh_completion_matches);

  return matches;
}

/* Disable readline's filename completion.  */
char *
vtysh_completion_empty (const char *ignore, int invoking_key)
{
  return NULL;
}

/* Set advanced VTY mode.  */
void
vtysh_advance_set ()
{
  ctree->advanced = 1;
  rl_advance_set ();
}

/* Unset advanced VTY mode.  */
void
vtysh_advance_unset ()
{
  ctree->advanced = 0;
  rl_advance_unset ();
}

/* Initialization for completion and desription.  */
void
vtysh_readline_init ()
{
  /* '?' is the description function.  */
  rl_bind_key ('?', vtysh_describe);

  /* Disable filename completion.  */
  rl_completion_entry_function = vtysh_completion_empty;

  /* Register completion function.  */
  rl_attempted_completion_function = (CPPFunction *) vtysh_completion;
}
