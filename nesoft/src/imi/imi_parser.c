/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>
#include <lib.h>

#include "cli.h"
#include "line.h"
#include "log.h"
#include "snprintf.h"

#include "imi/imi.h"
#include "imi/imi_mode.h"
#include "imi/imi_server.h"

#ifdef HAVE_CONFIG_COMPARE
#include "imi/imi_config_cmp.h"
#endif /* HAVE_CONFIG_COMPARE */

#ifdef HAVE_CRX
#include "crx.h"
#endif /* HAVE_CRX */


/* Parse IMI "line" command.  */
int
imi_line_parser (struct line *line)
{
  int ret;
  struct line reply;
  struct cli_node *node;
  struct cli *cli = &line->cli;
  struct cli_tree *ctree = line->zg->ctree;
#ifdef HAVE_CRX
  u_char ignore = 0;
#endif /* HAVE_CRX */
  int mode;
  int rcode;
  bool_t do_mcast = PAL_FALSE;

  /* Init the reply in case we need it. */
  reply.key       = line->key;
  reply.module    = PM_EMPTY;
  reply.mode      = line->mode;
  reply.privilege = line->privilege;
  reply.vr_id     = line->vr_id;
  reply.config_id = line->config_id;
  reply.pid       = line->pid;

  reply.buf[LINE_HEADER_LEN] = 0;

  /* Parse the line.  */
  ret = cli_parse (ctree, line->mode, line->privilege, line->str, 1, 0);
  mode = line->mode;

  /* Proxy the line to the PMs.  */
  switch (ret)
    {
    case CLI_PARSE_SUCCESS:

      /* Get execute node.  */
      node = ctree->exec_node;
      if (node && node->cel)
        line->module = node->cel->module;
       
      if (node && node->cel)
      do_mcast = FLAG_ISSET(node->cel->flags, CLI_FLAG_MCAST);

      /* Unset IMI and IMI shell module bit.  */
      MODBMAP_UNSET (line->module, APN_PROTO_IMI);
      MODBMAP_UNSET (line->module, APN_PROTO_IMISH);

      /* Protocol module proxy.  */
      if (! modbmap_isempty (line->module))
        if ((rcode = imi_server_proxy (line, &reply, do_mcast)) < 0)
        {
          /* The command was rejected: If the command was sent to more
             than one module we have to go back and delete all the
             config sessions started with this command.
             NOTE: If IMISH is in CONFIG_MODE no PM shall maintain any
             configuration session state refering this IMISH.
           */
          if (line->mode == CONFIG_MODE ) {
            imi_server_flush_module(line);
          }
          if (node->cel)
           if (! (rcode==-2 && CHECK_FLAG(node->cel->flags, CLI_FLAG_IGN_PM_DOWN_ERR)))
           {
            /* Reply back with the error message.  */
            if (reply.code == LINE_CODE_ERROR)
              pal_sock_write (line->sock, reply.buf, reply.length);

            /* Free arguments.  */
            cli_free_arguments (ctree);

            return 0;
          }
        }
#ifdef HAVE_CRX
      ignore = crx_config_ignore (line->str);
#endif /* HAVE_CRX. */

      /* CLI preparation.  */
      cli->zg = line->zg;
      cli->vr = line->vr;
      cli->line = line;
      cli->out_func = (CLI_OUT_FUNC) line_out;
      cli->out_val = cli;
      cli->str = line->str;
      cli->ctree = line->zg->ctree;
      line->buf[LINE_HEADER_LEN] = '\0';

      /* Call function.  If local function is empty it is success. */
      if (node->cel && node->cel->func)
        ret = (*node->cel->func) (cli, ctree->argc, ctree->argv);
      else
        ret = CLI_SUCCESS;
      /* Check whether we are switching back into CONFIG_MODE or above.
         If this is the case, we need to flush this IMISH's confses states
         to let another user to access the same objects.
         It would be complicated to trace all modules that are
         in specific config mode per IMISH.
       */
      if ((line->mode != IMISH_MODE) && (line->mode > CONFIG_MODE) &&
          (cli->mode <= CONFIG_MODE))
        imi_server_flush_module(line);
      /* Copy the CLI mode to the line header.  */
      line->mode = cli->mode;

      break;
    default:
      line_error_out (line, "%% No such command\n");
      ret = CLI_ERROR;
      break;
    }

  /* Free arguments.  */
  cli_free_arguments (ctree);

#ifdef HAVE_CONFIG_COMPARE
  if (boot_flag)
    if (mode != EXEC_MODE && mode != IMISH_MODE)
      if (ret == CLI_SUCCESS)
        if (line->pid)
          imi_config_compare (cli);
#endif /* HAVE_CONFIG_COMPARE */

  /* Send response.  */
  line->length = LINE_HEADER_LEN;

  /* Response back to IMI shell.  */
  switch (ret)
    {
    case CLI_SUCCESS:
      line->code = LINE_CODE_SUCCESS;
      break;
    case CLI_EOL:
      line->code = LINE_CODE_EOL;
      break;
    case CLI_AUTH_REQUIRED:
      line->code = LINE_CODE_AUTH_REQUIRED;
      break;
    default:
      line->code = LINE_CODE_ERROR;

      if (line->buf[LINE_HEADER_LEN] != '\0')
        line->length += pal_strlen (line->buf + LINE_HEADER_LEN) + 1;
      break;
    }

  /* Send response.  */
  line_header_encode (line);

  pal_sock_write (line->sock, line->buf, line->length);

#ifdef HAVE_CRX
  if (! ignore)
    {
      /* Send running configuration to Backups. */
      crx_send_config_full (NULL);
    }
#endif /* HAVE_CRX. */

  return 0;
}
