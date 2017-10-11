/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "lib.h"
#include "cli.h"
#include "line.h"
#include "snprintf.h"
#include "message.h"
#include "imi_client.h"

#include "imish/imish.h"
#include "imish/imish_cli.h"
#include "imish/imish_exec.h"
#include "imish/imish_system.h"
#include "imish/imish_line.h"

/* Extern user input line.  */
extern char *input_line;


/* Parser for user input and IMI commands.  */
void
imish_parse (char *line, int mode)
{
  struct imi_client *ic = imishm->imh->info;
  struct cli *cli = &ic->line.cli;
  struct cli_node *node;
  u_char privilege;
  int ret;
  int first = 1;
  int orig_mode = 0;
  int orig_ret = 0;
  char *invalid = NULL;
  static int do_cmd_mode = 0;
  static int do_cmd = 0;
  int old_mode = 0;

  /* When line is specified, mode is also specified.  */
  if (line)
    privilege = PRIVILEGE_MAX;
  else
    {
      /* Normal case we use cli of imish line.  */
      mode = cli->mode;
      privilege = cli->privilege;
      line = input_line;
    }

  /* Line length check.  */
  if (strlen (line) >= LINE_BODY_LEN - 1)
    {
      printf ("%% Input line is too long\n");
      return;
    }

  if (!pal_strncmp(line, "do ", 3))
    {
      do_cmd = 1;
      do_cmd_mode = cli->mode;
    }

again:

  /* Parse user input.  */
  ret = cli_parse (ctree, mode, privilege, line, 1, 0);

  /* Second try check.  */
  if (ret != CLI_PARSE_SUCCESS
      && ret != CLI_PARSE_AMBIGUOUS
      && ret != CLI_PARSE_INCOMPLETE
      && ret != CLI_PARSE_EMPTY_LINE
      && mode != EXEC_MODE && first == 1)
    {
      first = 0;
      invalid = ctree->invalid;

      orig_ret = ret;
      orig_mode = mode;
      mode = CONFIG_MODE;

      goto again;
    }

  /* Second try.  */
  if (!first)
    {
      /* Tell IMI to move down to the CONFIG if it succeeds.  */
      if (ret == CLI_PARSE_SUCCESS)
        imish_line_config (&ic->line);
      else
        {
          ret = orig_ret;
          cli->mode = mode = orig_mode;
          ctree->invalid = invalid;
        }
    }

  /* Check return value.  */
  switch (ret)
    {
    case CLI_PARSE_NO_MODE:
      /* Ignore no mode line.  */
      break;
    case CLI_PARSE_EMPTY_LINE:
      /* Ignore empty line.  */
      break;
    case CLI_PARSE_SUCCESS:
      /* Node to be executed.  */
      node = ctree->exec_node;

      /* Set cli structure.  */
      cli->str = line;
      cli->cel = node->cel;
      cli->ctree = ctree;

      /* Show command has special treatment.  */
      if ((ctree->show_node || CHECK_FLAG (node->cel->flags, CLI_FLAG_SHOW))
          && ! CHECK_FLAG (node->cel->flags, CLI_FLAG_NO_PAGER))
        {
          if (ctree->nested_show)
            /* Execute as a local function.  */
            (*node->cel->func) (cli, ctree->argc, ctree->argv);
          else
            imish_exec_show(cli);
        }
      else
        {
          /* This is a protocol module command...  */
          if (! modbmap_isempty (node->cel->module))
            {
              /* Execute local command first.  */
              if (CHECK_FLAG (node->cel->flags, CLI_FLAG_LOCAL_FIRST))
                {
                  /* Execute local function.  */
                  ret = (*node->cel->func) (cli, ctree->argc, ctree->argv);
                  if (ret == CLI_ERROR)
                    break;
                }

              /* Send command to IMI. */
              imish_line_send_cli (&ic->line, cli);

              /* Get IMI reply.  */
              imish_line_read (&ic->line);

              /* Break when local command is executed.  */
              if (CHECK_FLAG (node->cel->flags, CLI_FLAG_LOCAL_FIRST))
                {
                  imish_line_print (&ic->line);
                  break;
                }

              /* Check return value.  */
              switch (ic->line.code)
                {
                case LINE_CODE_AUTH_REQUIRED:
                  /* Tell the function that authentication is
                     required.  */
                  cli->auth = 1;

                  /* Fall through.  */
                case LINE_CODE_SUCCESS:
                  /* Execute local function when it is defined.  */
                  if (node->cel->func)
                    (*node->cel->func) (cli, ctree->argc, ctree->argv);
                  imish_line_print (&ic->line);
                  break;
                default:
                  /* Display error message from IMI.  */
                  imish_line_print (&ic->line);
                  break;
                }
            }
          else
            /* Execute local function.  */
            (*node->cel->func) (cli, ctree->argc, ctree->argv);
        }
      break;

    case CLI_PARSE_AMBIGUOUS:
      printf ("%% Ambiguous command:  \"%s\"\n", line);
      break;

    case CLI_PARSE_INCOMPLETE:
      /* In case of hidden command we do not want to give the user
         any hint that such command may exist.
       */
      if (ctree->exec_node != NULL )
        if (CHECK_FLAG(ctree->exec_node->flags, CLI_FLAG_HIDDEN))
          {
            printf ("%*c^\n",
                    strlen (imish_prompt ()) + (ctree->invalid - line), ' ');
            printf ("%% Invalid input detected at '^' marker.\n\n");
            break;
          }

      printf ("%% Incomplete command.\n\n");
      break;

    case CLI_PARSE_INCOMPLETE_PIPE:
      printf ("%% Incomplete command before pipe.\n\n");
      break;

    case CLI_PARSE_NO_MATCH:
      if (ctree->invalid)
        {
          if (do_cmd)
            {
              old_mode = cli->mode;
              cli->mode = do_cmd_mode;
              printf ("%*c^\n",
                      strlen (imish_prompt ()) + (strlen(cli->str) - strlen(ctree->invalid)), ' ');
              do_cmd = 0;
              cli->mode = old_mode;
            }
          else
            {
              printf ("%*c^\n",
                      strlen (imish_prompt ()) + (ctree->invalid - line), ' ');
            }
          printf ("%% Invalid input detected at '^' marker.\n\n");
        }
      else
        printf ("%% Unrecognized command\n");
      break;

    case CLI_PARSE_ARGV_TOO_LONG:
      printf ("%% Argument is too long\n");
      break;

    default:
      break;
    }

  /* Free arguments.  */
  cli_free_arguments (ctree);

  /* Check close.  */
  if (cli->status == CLI_CLOSE)
    imish_exit (0);
}

