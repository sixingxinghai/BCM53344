/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HOME_GATEWAY

#include "lib.h"
#include "cli.h"
#include "if.h"
#include "line.h"
#include "host.h"
#include "log.h"
#include "network.h"
#include "snprintf.h"
#include "thread.h"

#include "imi/imi.h"
#include "imi/imi_pm.h"
#include "imi/util/imi_filter.h"
#include "imi/util/imi_server.h"
#ifdef HAVE_NAT
#include "imi/util/imi_fw.h"
#include "imi/util/imi_vs.h"
#endif /* HAVE_NAT. */

#define DEBUG 1

#ifdef DEBUG
/* Counters for Web Connection. */
static int accept_count = 0;
static int accept_succ_count = 0;
static int read_count = 0;
static int read_succ_count = 0;
static int write_count = 0;
static int write_succ_count = 0;
#endif /* DEBUG */

/* IMI Server events. */
enum imi_event
{
  IMI_SERV,
  IMI_READ,
  IMI_WRITE,
  IMI_MAX
};

extern struct host *imi_host;

/* Prototypes. */
static void imi_server_event (enum imi_event event,
                              pal_sock_handle_t sock,
                              struct imi_serv *imi);

/* For local function. */
int
imi_web_out (struct cli *cli, const char *format, ...)
{
  struct imi_serv *web = cli->line;
  va_list args;
  int len;

  if (web->outlen <= 0)
    {
      web->outptr = web->outbuf;
      return -1;
    }

  va_start (args, format);
  len = zvsnprintf (web->outptr, web->outlen, format, args);
  web->outptr += len;
  web->outlen -= len;
  va_end (args);
  return 0;
}

/* Create new IMI server client connection. */
struct imi_serv *
imi_server_new ()
{
  struct cli *cli;
  struct imi_serv *new;

  new = XCALLOC (MTYPE_IMI_SERV, sizeof (struct imi_serv));
  new->t_read = NULL;
  new->sock = -1;
  new->outptr = new->outbuf;
  new->outlen = IMI_SERVER_MESSAGE_MAX_LEN;

  /* Create cli structure for command execution. */
  cli = XCALLOC (MTYPE_IMI_WEB, sizeof (struct cli));
  cli->zg = imim;
  cli->host = imi_host;
  cli->line = (void *)new;
  cli->out_func = (CLI_OUT_FUNC) imi_web_out;
  cli->out_val = cli;
  cli->str = NULL;
  cli->cel = NULL;
  cli->callback = NULL;
  cli->index = NULL;
  cli->mode = EXEC_MODE;

  new->cli = cli;

  /* Addto the list of clients. */
  listnode_add (IMI->client_list, new);

  return new;
}

/* Delete IMI server client connection. */
int
imi_server_free (struct imi_serv *imi_serv)
{
  int ret = RESULT_OK;

  /* Free cli first. */
  XFREE (MTYPE_IMI_WEB, imi_serv->cli);
  imi_serv->cli = NULL;

  /* Now, free imi_serv. */
  if (imi_serv->sock > 0)
    pal_sock_close (imim, imi_serv->sock);
  if (imi_serv->t_read)
    thread_cancel (imi_serv->t_read);
  if (imi_serv->t_write)
    thread_cancel (imi_serv->t_write);

  /* Delete from list & free memory. */
  listnode_delete (IMI->client_list, imi_serv);
  XFREE (MTYPE_IMI_SERV, imi_serv);

  return ret;
}

/* Execute command for IMI server.  */
void
imi_server_proxy (struct cli_node *node, struct cli *cli, char *str)
{
  int i;
  int ret;
  struct imi_pm *pm;
  struct line reply;

  /* Send it to protocol module.  */
  for (i = 0; i < APN_PROTO_IMI; i++)
    if (MODBMAP_ISSET (node->cel->module, i)
        && (pm = imi_pm_lookup (imi_pmaster, i)) != NULL)
      {
        if (pm->connected != PAL_TRUE)
          continue;

        ret = imi_pm_line_send (pm, str, cli->mode, 0);
        if (ret < 0)
          continue;

        line_client_read (pm->sock, &reply);
      }

  /* Execute local function.  */
  if (node->cel->func)
    ret = (*node->cel->func) (cli, imi_ctree->argc, imi_ctree->argv);

  cli_free_arguments (imi_ctree);
}

/* Execute config command from web client. */
int
imi_server_config_command (struct imi_serv *imi_serv, char *exec_first)
{
  struct cli *cli = imi_serv->cli;
  struct cli_node *node;
  int ret;

  cli->privilege = PRIVILEGE_MAX;

  /* Lock CLI. */
  if (host_config_lock (imi_host, cli) < 0)
    {
      cli_out (cli, "%% PacOS configuration has been locked\n");
      return CLI_ERROR;
    }

  /* Reset output. */
  imi_serv->outbuf[0] = '\0';
  imi_serv->outptr = imi_serv->outbuf;
  imi_serv->outlen = IMI_SERVER_MESSAGE_MAX_LEN;

  /* Anything to execute before the command? */
  if (exec_first)
    cli->str = exec_first;
  else
    cli->str = imi_serv->pnt;

  /* Parse the command. */
  ret = cli_parse (imi_ctree, cli->mode, PRIVILEGE_MAX, cli->str, 1, 0);
  switch (ret)
    {
    case CLI_PARSE_SUCCESS:
      node = imi_ctree->exec_node;
      cli->cel = node->cel;
      imi_server_proxy (node, cli, cli->str);
      break;
    case CLI_PARSE_INCOMPLETE:
    case CLI_PARSE_INCOMPLETE_PIPE:
    case CLI_PARSE_AMBIGUOUS:
    case CLI_PARSE_NO_MATCH:
    case CLI_PARSE_NO_MODE:
    case CLI_PARSE_ARGV_TOO_LONG:
      /* Second try. */
      cli->mode = CONFIG_MODE;
      ret = cli_parse (imi_ctree, cli->mode, PRIVILEGE_MAX, cli->str, 1, 0);
      switch (ret)
        {
        case CLI_PARSE_SUCCESS:
          node = imi_ctree->exec_node;
          cli->cel = node->cel;
          imi_server_proxy (node, cli, cli->str);
          break;
        default:
          cli_out (cli, "%% Unknown command.\n");
          break;
        }
      break;
    default:
      break;
    }

  /* Unlock CLI. */
  host_config_unlock (imi_host, cli);

  /* Free CLI arguments. */
  cli_free_arguments (imi_ctree);

  if (exec_first)
    {
      return imi_server_config_command (imi_serv, NULL);
    }

  /* Is there a response? */
  if (pal_strlen (imi_serv->outbuf) == 0)
    cli_out (cli, "%s\n", IMI_OK_STR);

  /* Write response to web client. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return ret;
}

/* Execute show command from web client. */
int
imi_server_show_command (struct imi_serv *imi_serv)
{
  struct cli *cli = imi_serv->cli;
  struct cli_node *node;
  int mode;
  int ret;

  /* Reset output. */
  imi_serv->outbuf[0] = '\0';
  imi_serv->outptr = imi_serv->outbuf;
  imi_serv->outlen = IMI_SERVER_MESSAGE_MAX_LEN;

  /* Parse the command. */
  cli->str = imi_serv->buf;
  mode = cli->mode;
  cli->mode = EXEC_MODE;

  ret = cli_parse (imi_ctree, EXEC_MODE, PRIVILEGE_MAX, cli->str, 1, 0);
  switch (ret)
    {
    case CLI_PARSE_SUCCESS:
      node = imi_ctree->exec_node;
      cli->cel = node->cel;
      ret = (*node->cel->func) (cli, imi_ctree->argc, imi_ctree->argv);
      break;
    default:
      cli_out (cli, "%% Unknown command.\n");
      break;
    }
  cli->mode = mode;

  /* Is there a response? */
  if (pal_strlen (imi_serv->outbuf) == 0)
    cli_out (cli, "%s\n", IMI_OK_STR);

  /* Write response. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return 0;
}

/* Execute command string received from web client. */
static int
imi_server_execute (struct imi_serv *imi_serv)
{
  char *cmd_string = imi_serv->buf;
  char *cp, *start;
  int len, len2, i;
  struct imi_ext_api *api;

  if (cmd_string == NULL)
    return PAL_FALSE;

  cp = cmd_string;

  SKIP_WHITE_SPACE (cp);

  /* Return false if there's only white spaces. */
  if (*cp == '\0')
    return PAL_FALSE;

  /* Look at the first word. */
  while (1)
    {
      READ_NEXT_WORD (cp, start, len);

      /* Is this a special API?  If yes, execute the installed handler. */
      for (i = 0; i < MAX_EXT_API; i++)
        {
          api = IMI->special[i];
          if (api == NULL)
            continue;

          len2 = pal_strlen (api->cmd_string);

          /* Does the command match? */
          if (len == len2)
            if (! pal_strncmp (start, api->cmd_string, len))
              {
                SKIP_WHITE_SPACE (cp);

                /* Save current position & call function. */
                imi_serv->pnt = cp;
                (*api->func) (imi_serv);
                return 0;
              }
        }
      break;
    }

  /* If here, it's not a special API. */
  imi_serv->pnt = imi_serv->buf;
  return imi_server_config_command (imi_serv, NULL);
}

/*-------------------------------------------------------------------------
  IMI Server socket handling. */

/* IMI server socket creation. */
static int
imi_server_sock_init ()
{
  pal_sock_handle_t sock;
  struct pal_sockaddr_in4 addr;
  struct pal_sockaddr *serv;
  s_int32_t state = 1;  /* on. */
  int ret;
  int len;

  /* Open socket. */
  sock = pal_sock (imim, AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      /* Log the error. */
      zlog_warn (imim, "create socket error: %s", pal_strerror (errno));
      return -1;
    }

  /* Prepare accept socket. */
  pal_mem_set (&addr, 0, sizeof (struct pal_sockaddr_in4));
  addr.sin_family = AF_INET;
  addr.sin_port = pal_hton16 (IMI_SERVER_PORT);
#ifdef HAVE_SIN_LEN
  addr.sin_len = sizeof (struct pal_sockaddr_in4);
#endif /* HAVE_SIN_LEN */
  addr.sin_addr.s_addr = pal_hton32 (INADDR_LOOPBACK);
  pal_sock_set_reuseaddr (sock, state);
  pal_sock_set_reuseport (sock, state);

  /* Prepare for bind. */
  len = sizeof (struct pal_sockaddr_in4);
  serv = (struct pal_sockaddr *) &addr;

  /* Bind socket. */
  ret = pal_sock_bind (sock, serv, len);
  if (ret < 0)
    {
      /* Log the error. */
      zlog_warn (imim, "bind socket error: %s", pal_strerror (errno));
      pal_sock_close (imim, sock);
      return ret;
    }

  /* Listen on the socket. */
  ret = pal_sock_listen (sock, 5);
  if (ret < 0)
    {
      zlog_warn (imim, "listen socket error: %s", pal_strerror (errno));
      pal_sock_close (imim, sock);
      return ret;
    }

  /* Store socket. */
  IMI->client_sock = sock;

  /* Register accept thread. */
  imi_server_event (IMI_SERV, sock, NULL);

  return 0;
}

/* Close server socket. */
static int
imi_server_sock_close ()
{
  /* Close socket. */
  if (IMI->client_sock > 0)
    pal_sock_close (imim, IMI->client_sock);

  /* Cancel server thread. */
  if (IMI->t_serv)
    {
      thread_cancel (IMI->t_serv);
      IMI->t_serv = NULL;
    }

  return 0;
}

/* Accept connection from a web client. */
static int
imi_server_accept (struct thread *thread)
{
  pal_sock_handle_t imi_sock;
  pal_sock_handle_t accept_sock;
  struct pal_sockaddr_in4 addr;
  pal_sock_len_t len;
  struct imi_serv *imi_serv;
  int ret = 0;

  accept_sock = THREAD_FD (thread);
  IMI->t_serv = NULL;

#ifdef DEBUG
  accept_count++;
#endif /* DEBUG */
  /* Accept and get client socket. */
  imi_sock = pal_sock_accept (imim, accept_sock,
                              (struct pal_sockaddr *) &addr, &len);
  if (imi_sock < 0)
    {
      zlog_warn (imim, "accept socket error: %s", pal_strerror (errno));
      return -1;
    }
#ifdef DEBUG
  accept_succ_count++;
#endif /* DEBUG */

  /* Create new imi_serv client. */
  imi_serv = imi_server_new ();
  if (imi_serv == NULL)
    {
      char resp[4];

      zlog_warn (imim, "Can't accept web connection; CLI is locked.");
      resp[0] = '\0';
      resp[1] = '\0';
      resp[2] = '\0';
      resp[3] = IMI_ERR_LOCKED;
#ifdef DEBUG
      write_count++;
#endif /* DEBUG */
      ret = writen (imi_sock, resp, 4);
      if (ret != len)
        zlog_warn (imim, "Error writing LOCK response to web client.");
      return -1;
    }
  imi_serv->sock = imi_sock;

  /* Register read thread. */
  imi_server_event (IMI_READ, imi_sock, imi_serv);

  /* We continue to listen on the socket. */
  imi_server_event (IMI_SERV, IMI->client_sock, NULL);

  return 0;
}

/* Read message received from web client. */
static int
imi_server_read (struct thread *thread)
{
  pal_sock_handle_t sock;
  struct imi_serv *imi_serv;
  int nbytes = 0;

  sock = THREAD_FD (thread);
  imi_serv = THREAD_ARG (thread);
  imi_serv->t_read = NULL;

#ifdef DEBUG
  read_count++;
#endif /* DEBUG */

  /* Call read handler callback function. */
  nbytes = pal_sock_read (imi_serv->sock, imi_serv->buf, IMI_SERVER_MESSAGE_MAX_LEN);

  /* Read? */
  if (nbytes <= 0)
    {
      /* Close this socket. */
      pal_sock_close (imim, imi_serv->sock);
      imi_server_free (imi_serv);
      return nbytes;
    }
#ifdef DEBUG
  read_succ_count++;
#endif /* DEBUG */

#ifdef DEBUG
  fprintf (stderr, "Read from web client: (%s)\n", imi_serv->buf);
#endif /* DEBUG */

  /* Execute the command. */
  imi_server_execute (imi_serv);

  /* Re-register read thread. */
  imi_server_event (IMI_READ, sock, imi_serv);

  return 0;
}

/* Write message to web client. */
static int
imi_server_write (struct thread *thread)
{
  int ret;
  pal_sock_handle_t sock;
  struct imi_serv *imi_serv;
  int len = 0;
  char *response;
  char return_val;

  sock = THREAD_FD (thread);
  imi_serv = THREAD_ARG (thread);
  imi_serv->t_write = NULL;
  response = imi_serv->outbuf;

#ifdef DEBUG
  fprintf (stderr, "Write response to web client:\n%s\n", response);
#endif /* DEBUG */

  /* Is this an error response? */
  return_val = IMI_RESPONSE;
  if (! pal_strncmp (response, "IMI-OK", pal_strlen ("IMI-OK")))
    return_val = IMI_OK;
  else if (! pal_strncmp (response, "IMI-ERROR", pal_strlen ("IMI-ERROR")))
    return_val = IMI_ERR_FAILED;
  else if (! pal_strncmp (response, "% Ambiguous command.",
                          pal_strlen ("% Ambiguous command.")))
    return_val = IMI_ERR_AMBIGUOUS;
  else if (! pal_strncmp (response, "% Unknown command.",
                          pal_strlen ("% Unknown command.")))
    return_val = IMI_ERR_UNKNOWN;
  else if (! pal_strncmp (response, "% Command incomplete.",
                          pal_strlen ("% Command incomplete.")))
    return_val = IMI_ERR_INCOMPLETE;

  /* If it's generic return, don't need to send the string. */
  if (return_val == IMI_RESPONSE)
    len = pal_strlen (response);
  else
    len = 0;

  /* Write response to web client (trunkate if it's too big). */
  if (len > IMI_SERVER_MESSAGE_MAX_LEN - 4)
    len = IMI_SERVER_MESSAGE_MAX_LEN - 4;
  response[len] = '\0';
  response[len+1] = '\0';
  response[len+2] = '\0';
  response[len+3] = return_val;
  len += 4;

#ifdef DEBUG
  write_count++;
#endif /* DEBUG */

  /* Call write function. */
  ret = writen (sock, response, len);
  if (ret != len)
    return -1;

#ifdef DEBUG
  write_succ_count++;
#endif /* DEBUG */

  /* Reset output. */
  imi_serv->outbuf[0] = '\0';
  imi_serv->outptr = imi_serv->outbuf;
  imi_serv->outlen = IMI_SERVER_MESSAGE_MAX_LEN;

  return 0;
}

/* IMI Server events. */
static void
imi_server_event (enum imi_event event,
                  pal_sock_handle_t sock,
                  struct imi_serv *imi)
{
  switch (event)
    {
    case IMI_SERV:
      IMI->t_serv = thread_add_read_high (imim, imi_server_accept, imi, sock);
      break;
    case IMI_READ:
      imi->t_read = thread_add_read_high (imim, imi_server_read, imi, sock);
      break;
    case IMI_WRITE:
      if (sock > 0)
        imi->t_write = thread_add_write (imim, imi_server_write, imi, sock);
      break;
    default:
      break;
    }
  return;
}

/*-------------------------------------------------------------------------
  Functions to handle extended APIs. */

/* Install exteneded API for IMI web server. */
void
imi_server_install_extended_api (enum ext_api api_type, char *cmd_string,
                                 int (*func) (struct imi_serv *))
{
  struct imi_ext_api *new;

  new = XCALLOC (MTYPE_IMI_EXTAPI, sizeof (struct imi_ext_api));
  new->cmd_string = XSTRDUP (MTYPE_IMI_STRING, cmd_string);
  new->func = func;

  IMI->special[api_type] = new;
}

/* Handle special 'ip address-mask' command from web. */
static int
imi_server_ip_address_mask_check (struct imi_serv *imi_serv)
{
  char prefix_buf[INET_ADDRSTRLEN];
  char mask_buf[INET_ADDRSTRLEN];
  struct pal_in4_addr mask;
  u_int8_t masklen;
  int len, ret;
  char *cp = imi_serv->pnt;
  char *start;

  len = pal_strlen ("ip address-mask");
  if (pal_strlen (imi_serv->pnt) >= len)
    if (! pal_strncmp (imi_serv->pnt, "ip address-mask", len))
      {
        /* Store current pointer & overwrite with valid PacOS command. */
        imi_serv->pnt = cp;

        /* Forward past command words. */
        while (!
               (pal_char_isspace ((int) * cp) || *cp == '\r'
                || *cp == '\n') && *cp != '\0')
          cp++;
        SKIP_WHITE_SPACE (cp);

        while (!
               (pal_char_isspace ((int) * cp) || *cp == '\r'
                || *cp == '\n') && *cp != '\0')
          cp++;
        SKIP_WHITE_SPACE (cp);

        /* Get Prefix. */
        READ_NEXT_WORD_TO_BUF (cp, start, prefix_buf, len);
        SKIP_WHITE_SPACE (cp);

        /* Get Mask. */
        READ_NEXT_WORD_TO_BUF (cp, start, mask_buf, len);

        /* Convert mask to length. */
        if (len > 0)
          {
            ret = pal_inet_pton (AF_INET, mask_buf, &mask);
            if (ret == 0)
              return -1;
            masklen = ip_masklen (mask);

            /* Create command string for PacOS. */
            sprintf (imi_serv->pnt, "ip address %s/%d", prefix_buf, masklen);
          }
      }
  return 0;
}

/* ip commands. */
static int
imi_server_ip_cmd (struct imi_serv *imi_serv)
{
  int cmdlen = pal_strlen (imi_serv->buf);
  int len_arg;
  u_char *cp = imi_serv->pnt;
  u_char *cp1 = NULL, *cp2 = NULL;
  u_char buf[IMI_SERVER_CMD_STRING_MAX_LEN];
  u_char ext_cmd[IMI_SERVER_CMD_STRING_MAX_LEN];

  SKIP_WHITE_SPACE (cp);

  /* Is this access-group command? */
  len_arg = pal_strlen ("access-group");
  if (cmdlen >= len_arg)
    if (! pal_strncmp (imi_serv->pnt, "access-group", len_arg))
      {
        /* Locate interface. */
        cp1 = pal_strstr (imi_serv->pnt, "interface");
        if (!cp1)
          {
            /* No interface string found. Return error. */
          }

        pal_strcpy (buf, "ip ");
        pal_strncat (buf, imi_serv->pnt, (cp1 - 1) - imi_serv->pnt);

        /* Skip interface to get interface name. */
        cp1 += pal_strlen ("interface");

        SKIP_WHITE_SPACE (cp1);

        cp2 = cp1;

        /* Skip two words for interface name. */
        while (!pal_char_isspace ((int) * cp2) && *cp2 != '\0')
          cp2++;
#if 0
        SKIP_WHITE_SPACE (cp2);
        while (!pal_char_isspace ((int) * cp2) && *cp2 != '\0')
          cp2++;
#endif /* 0 */

        /* Get interface name. */
        pal_strcpy (ext_cmd, "interface ");
        pal_strncat (ext_cmd, cp1, (cp2 - cp1));

        /* Copy the actual command for execution. */
        pal_strcpy (imi_serv->buf, buf);

        /* Execute command like normal. */
        imi_serv->pnt = imi_serv->buf;
        return imi_server_config_command (imi_serv, ext_cmd);
      }

  /* Else, execute command like normal. */
  imi_serv->pnt = imi_serv->buf;
  return imi_server_config_command (imi_serv, NULL);
}

/* no commands. */
static int
imi_server_no_cmd (struct imi_serv *imi_serv)
{
  int cmdlen = pal_strlen (imi_serv->buf);
  int len_arg;
  u_char *cp = imi_serv->pnt;
  u_char *cp1 = NULL, *cp2 = NULL;
  u_char buf[IMI_SERVER_CMD_STRING_MAX_LEN];
  u_char ext_cmd[IMI_SERVER_CMD_STRING_MAX_LEN];

  SKIP_WHITE_SPACE (cp);

  /* Is this tunnel command? */
  len_arg = pal_strlen ("tunnel");
  if (cmdlen >= len_arg)
    if (! pal_strncmp (imi_serv->pnt, "tunnel", len_arg))
      {
        cp += pal_strlen ("tunnel");

        SKIP_WHITE_SPACE (cp);

        cp1 = cp;

        /* Skip two words for interface name. */
        while (!pal_char_isspace ((int) * cp1) && *cp1 != '\0')
          cp1++;

        /* Fill in command buffer. */
        pal_strcpy (buf, "no interface tunnel ");
        pal_strncat (buf, cp, (cp1 - cp));

        /* Copy the actual command for execution. */
        pal_strcpy (imi_serv->buf, buf);

        /* Execute command like normal. */
        imi_serv->pnt = imi_serv->buf;
        return imi_server_config_command (imi_serv, ext_cmd);
      }

  /* Is this ip command? */
  len_arg = pal_strlen ("ip");
  if (cmdlen >= len_arg)
    if (! pal_strncmp (imi_serv->pnt, "ip", len_arg))
      {
        cp += pal_strlen ("ip");

        SKIP_WHITE_SPACE (cp);

        /* Check for access-list command. */
        cp1 = pal_strstr (cp, "access-group");
        if (!cp1)
          {
            /* Else, execute command like normal. */
            imi_serv->pnt = imi_serv->buf;
            return imi_server_config_command (imi_serv, NULL);
          }

        /* Locate interface. */
        cp1 = pal_strstr (cp, "interface");
        if (!cp1)
          {
            /* No interface string found. Return error. */
          }

        pal_strcpy (buf, "no ");
        pal_strncat (buf, imi_serv->pnt, (cp1 - 1) - imi_serv->pnt);

        /* Skip interface to get interface name. */
        cp1 += pal_strlen ("interface");

        SKIP_WHITE_SPACE (cp1);

        cp2 = cp1;

        /* Skip two words for interface name. */
        while (!pal_char_isspace ((int) * cp2) && *cp2 != '\0')
          cp2++;
#if 0
        SKIP_WHITE_SPACE (cp2);
        while (!pal_char_isspace ((int) * cp2) && *cp2 != '\0')
          cp2++;
#endif /* 0 */

        /* Get interface name. */
        pal_strcpy (ext_cmd, "interface ");
        pal_strncat (ext_cmd, cp1, (cp2 - cp1));

        /* Copy the actual command for execution. */
        pal_strcpy (imi_serv->buf, buf);

        /* Execute command like normal. */
        imi_serv->pnt = imi_serv->buf;
        return imi_server_config_command (imi_serv, ext_cmd);
      }

  /* Else, execute command like normal. */
  imi_serv->pnt = imi_serv->buf;
  return imi_server_config_command (imi_serv, NULL);
}

/* Handle extended command from web server. */
static int
imi_server_extended_cmd_get (struct imi_serv *imi_serv,
                             const char *prefix)
{
  char *cmd_string = imi_serv->buf;
  char *cp = imi_serv->pnt;
  char *start;
  int len;
  char buf[IMI_SERVER_CMD_STRING_MAX_LEN];
  char tmp_buf[IMI_SERVER_CMD_STRING_MAX_LEN-10];
  int ret;

  if (cmd_string == NULL)
    return PAL_FALSE;

  SKIP_WHITE_SPACE (cp);

  /* Return false if there's only white spaces. */
  if (*cp == '\0')
    return PAL_FALSE;

  /* Look at the first word. */
  while (1)
    {
      start = cp;
      while (!
             (pal_char_isspace ((int) * cp) || *cp == '\r'
              || *cp == '\n') && *cp != '\0')
        cp++;
      len = cp - start;

      /* Copy identifier (name, id, &c) name to temporary buffer. */
      pal_strncpy (tmp_buf, start, len);
      tmp_buf[len] = '\0';

      /* Create first command-string. */
      sprintf (buf, "%s %s", prefix, tmp_buf);

#if 0
      /* Interface command may need 2 words ('eth0' or 'FastEthernet 0'). */
      if (! pal_strcmp (prefix, "interface"))
        {
          SKIP_WHITE_SPACE (cp);
          start = cp;
          while (!
                 (pal_char_isspace ((int) *cp) || *cp == '\r'
                  || *cp == '\n') && *cp != '\0')
            cp++;
          len = cp - start;

          pal_strncpy (tmp_buf, start, len);
          tmp_buf[len] = '\0';

          sprintf (buf, "%s %s", buf, tmp_buf);
        }
#endif /* 0 */
      break;
    }

  SKIP_WHITE_SPACE (cp);
  imi_serv->pnt = cp;

  /* Is this special 'ip address-mask' command? */
  ret = imi_server_ip_address_mask_check (imi_serv);
  if (ret != 0)
    return -1;

  /* Set pointer back. */
  imi_serv->pnt = cp;

  /* Execute command. */
  return imi_server_config_command (imi_serv, buf);

  return 0;
}

/* Handle interface command from web server. */
static int
imi_server_interface_cmd (struct imi_serv *imi_serv)
{
  return imi_server_extended_cmd_get (imi_serv, "interface");
}

/* Handle tunnel command from web server. */
static int
imi_server_tunnel_cmd (struct imi_serv *imi_serv)
{
  return imi_server_extended_cmd_get (imi_serv, "interface tunnel");
}

#ifdef HAVE_DHCP_SERVER
/* Handle dhcp-server pool command from web server. */
static int
imi_server_dhcps_cmd (struct imi_serv *imi_serv)
{
  return imi_server_extended_cmd_get (imi_serv, "ip dhcp pool");
}
#endif /* HAVE_DHCP_SERVER */

#ifdef HAVE_PPPOE
/* Handle pppoe commands from web server. */
static int
imi_server_pppoe_cmd (struct imi_serv *imi_serv)
{
  return imi_server_extended_cmd_get (imi_serv, "service");
}
#endif /* HAVE_PPPOE */

/* Handle clear commands from web server. */
static int
imi_server_clear_cmd (struct imi_serv *imi_serv)
{
  int cmdlen = pal_strlen (imi_serv->buf);
  u_char *cp = imi_serv->pnt;
  u_char *cp1;
  u_char *start;
  int len_arg;
  int bytes;
  int ret = RESULT_ERROR;
  u_char buf[IMI_SERVER_CMD_STRING_MAX_LEN];

  imi_serv->cli->privilege = PRIVILEGE_MAX;

  SKIP_WHITE_SPACE (cp);

  /* Read the next word. */
  READ_NEXT_WORD (cp, start, len_arg);

#ifdef HAVE_DNS
  /* DNS. */
  cmdlen = pal_strlen ("dns");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "dns", len_arg))
      ret = imi_dns_reset ();
#endif /* HAVE_DNS */

#ifdef HAVE_DHCP_SERVER
  /* DHCP Server. */
  cmdlen = pal_strlen ("dhcps");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "dhcps", len_arg))
      ret = imi_dhcps_reset ();
#endif /* HAVE_DHCP_SERVER */

#ifdef HAVE_IMI_DEFAULT_GW
  /* Default gateway. */
  cmdlen = pal_strlen ("default-gw");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "default-gw", len_arg))
      ret = imi_default_gw_reset (imi_serv);
#endif /* HAVE_IMI_DEFAULT_GW */

#ifdef HAVE_IMI_WAN_STATUS
  /* Clear wan-status. */
  cmdlen = pal_strlen ("wan-status");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "wan-status", len_arg))
      ret = imi_wan_status_reset_default ();
#endif /* HAVE_IMI_WAN_STATUS */

#ifdef HAVE_NAT
  /* Clear Virtual Servers. */
  cmdlen = pal_strlen ("virtual-server");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "virtual-server", len_arg))
      ret = imi_virtual_server_reset (imi_serv);
#endif /* HAVE_NAT */

  /* Clear access-list .*/
  cmdlen = pal_strlen ("access-list");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "access-list", len_arg))
      {
        cp = start;
        cp += pal_strlen ("access-list");

        SKIP_WHITE_SPACE (cp);

        READ_NEXT_WORD_TO_BUF (cp, cp1, buf, bytes);

        ret = imi_access_list_delete (buf);
      }

  /* Generate reply. */
  if (pal_strlen (imi_serv->outbuf) == 0)
    cli_out (imi_serv->cli, "%s\n", (ret == RESULT_OK) ? IMI_OK_STR : IMI_ERR_STR);

  /* Write response to web client. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return RESULT_OK;
}

/* Handle 'get' commands from web server.
   Note: These 'get' commands are used for extra features provided by
     IMI for the web application.  These include default gateway,
     WAN status, etc. */
static int
imi_server_get_cmd (struct imi_serv *imi_serv)
{
  char *cp = imi_serv->pnt;
  struct host *host = imi_host;
  char *start;
  int len_arg;
  bool_t response_sent = PAL_FALSE;
  int cmdlen;
#ifdef HAVE_IMI_WAN_STATUS
  enum wan_status wan;
#endif /* HAVE_IMI_WAN_STATUS */

  SKIP_WHITE_SPACE (cp);

  /* Read the next word. */
  READ_NEXT_WORD (cp, start, len_arg);

  /* 'password'. */
  cmdlen = pal_strlen ("password");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "password", len_arg))
      {
        if (host->password)
          cli_out (imi_serv->cli, "password: %s\n", host->password);
        else
          cli_out (imi_serv->cli, "%s\n", IMI_OK_STR);

        /* Set response-sent flag. */
        response_sent = PAL_TRUE;
      }

#ifdef HAVE_IMI_DEFAULT_GW
  /* 'default-gw'. */
  cmdlen = pal_strlen ("default-gw");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "default-gw", len_arg))
      {
        /* Show gateway or 'IMI-OK' if none exists. */
        if (IMI->gateway)
          {
            cli_out (imi_serv->cli, "gateway: %s", IMI->gateway);
          }
        else
          cli_out (imi_serv->cli, "%s\n", IMI_OK_STR);

        /* Set response-sent flag. */
        response_sent = PAL_TRUE;
      }
#endif /* HAVE_IMI_DEFAULT_GW */

#ifdef HAVE_IMI_WAN_STATUS
  /* 'wan-status'. */
  cmdlen = pal_strlen ("wan-status");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "wan-status", len_arg))
      {
        /* Get status. */
        wan = imi_wan_status_get ();
        switch (wan)
          {
          case WAN_STATIC:
            cli_out (imi_serv->cli, "%s\n", IMI_WAN_STATIC_STRING);
            break;
          case WAN_DHCP:
            cli_out (imi_serv->cli, "%s\n", IMI_WAN_DHCP_STRING);
            break;
          case WAN_PPPoE:
            cli_out (imi_serv->cli, "%s\n", IMI_WAN_PPPoE_STRING);
            break;
          default:
            cli_out (imi_serv->cli, "%s\n", IMI_OK_STR);
            break;
          }
        /* Set response-sent flag. */
        response_sent = PAL_TRUE;
      }
#endif /* HAVE_IMI_WAN_STATUS */

  /* If no commands were matched, send Unknown. */
  if (!response_sent)
    cli_out (imi_serv->cli, "% Unknown command.\n");

  /* Write response to web client. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return RESULT_OK;
}

#ifdef HAVE_IMI_DEFAULT_GW
/* Handle 'default-gw' command. */
static int
imi_server_default_gw_cmd (struct imi_serv *imi_serv)
{
  int ret;

  /* Set new. */
  ret = imi_default_gw_set (imi_serv);

  /* Generate reply. */
  cli_out (imi_serv->cli, "%s\n", (ret == RESULT_OK) ? IMI_OK_STR : IMI_ERR_STR);

  /* Write response to web client. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return ret;
}
#endif /* HAVE_IMI_DEFAULT_GW */

#ifdef HAVE_IMI_WAN_STATUS
/* Handle 'wan-status' command. */
static int
imi_server_wan_status_cmd (struct imi_serv *imi_serv)
{
  int cmdlen = pal_strlen (imi_serv->buf);
  char *cp = imi_serv->pnt;
  char *start;
  int len_arg;
  int ret = RESULT_ERROR;

  SKIP_WHITE_SPACE (cp);

  /* Read the next word. */
  READ_NEXT_WORD (cp, start, len_arg);

  /* 'static'. */
  cmdlen = pal_strlen ("static");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "static", len_arg))
      ret = imi_wan_status_set (WAN_STATIC);

  /* 'dhcp-client'. */
  cmdlen = pal_strlen ("dhcp");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "dhcp", len_arg))
      ret = imi_wan_status_set (WAN_DHCP);

  /* 'pppoe'. */
  cmdlen = pal_strlen ("pppoe");
  if (cmdlen == len_arg)
    if (! pal_strncmp (start, "pppoe", len_arg))
      ret = imi_wan_status_set (WAN_PPPoE);

  /* Reply. */
  cli_out (imi_serv->cli, "%s\n", (ret == RESULT_OK) ? IMI_OK_STR : IMI_ERR_STR);
  /* Write response to web client. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return ret;
}
#endif /* HAVE_IMI_WAN_STATUS */

/* Commit configuration to disk storage (config-file). */
static int
imi_server_commit_cmd (struct imi_serv *imi_serv)
{
  const char *commit = "write memory";

  /* Execute config-write. */
  pal_strcpy (imi_serv->buf, commit);
  imi_server_show_command (imi_serv);

  /* Generate reply. */
  cli_out (imi_serv->cli, "%s\n", IMI_OK_STR);

  /* Write response to web client. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return RESULT_OK;
}

/* Enable/disable protocol on an interface. */
static int
imi_server_protocol_func (u_int8_t enable, char *router_cmd,
                          struct imi_serv *imi_serv)
{
  struct cli *cli = imi_serv->cli;
  char *cp = imi_serv->pnt;
  char *start;
  int len_arg;
  char iftype[INTERFACE_NAMSIZ];
  char ifnum[10];
  char *ifname;
  struct interface *ifp;
  int ret;
  int buflen = IMI_SERVER_MESSAGE_MAX_LEN;

  /* Enable or disable? */
  if (enable)
    {
      SKIP_WHITE_SPACE (cp);

      /* Return false if there's only white spaces. */
      if (*cp == '\0')
        {
          cli_out (cli, "%% Command incomplete.\n");
          return RESULT_ERROR;
        }

      READ_NEXT_WORD_TO_BUF (cp, start, iftype, len_arg);
      ifname = iftype;

#ifdef HAVE_IFNAME_ADV
      READ_NEXT_WORD_TO_BUF (cp, start, ifnum, len_arg);
      ifname = cli_interface_resolve (buf, INTERFACE_NAMSIZ, iftype, ifnum);
#endif /* HAVE_IFNAME_ADV */

      /* Lookup interface. */
      ifp = if_lookup_by_name (&imim->ifm, IMI_IF_LIST);
      if (ifp != NULL)
        {
          /* "network IFNAME" */
#ifdef HAVE_IFNAME_ADV
          zsnprintf (imi_serv->buf, buflen, "network %s %s", iftype, ifnum);
#else
          zsnprintf (imi_serv->buf, buflen, "network %s", ifname);
#endif /* HAVE_IFNAME_ADV */
          imi_serv->pnt = imi_serv->buf;
          imi_server_config_command (imi_serv, router_cmd);
          ret = RESULT_OK;
        }
      else
        ret = IMI_API_INVALID_ARG_ERROR;
    }
  else /* disabled. */
    {
      zsnprintf (imi_serv->buf, buflen, "%s", router_cmd);
      imi_serv->pnt = imi_serv->buf;
      imi_server_config_command (imi_serv, NULL);
      ret = RESULT_OK;
    }

  /* Generate reply. */
  cli_out (cli, "%s\n", (ret == RESULT_OK) ? IMI_OK_STR : IMI_ERR_STR);

  /* Write response to web client. */
  imi_server_event (IMI_WRITE, imi_serv->sock, imi_serv);

  return RESULT_OK;
}

/* Enable RIP. */
static int
imi_server_rip_enable_cmd (struct imi_serv *imi_serv)
{
  return imi_server_protocol_func (1, "router rip", imi_serv);
}

/* Disable RIP. */
static int
imi_server_rip_disable_cmd (struct imi_serv *imi_serv)
{
  return imi_server_protocol_func (0, "no router rip", imi_serv);
}

#ifdef HAVE_IPV6
/* Enable RIPng. */
static int
imi_server_ripng_enable_cmd (struct imi_serv *imi_serv)
{
  return imi_server_protocol_func (0, "router ipv6 rip", imi_serv);
}

/* Disable RIPng. */
static int
imi_server_ripng_disable_cmd (struct imi_serv *imi_serv)
{
  return imi_server_protocol_func (0, "no router ipv6 rip", imi_serv);
}
#endif /* HAVE_IPV6 */

#ifdef DEBUG
void
show_imi_server_web_conn_stats (struct cli *cli)
{
  cli_out (cli, "No. of accept attempts    : %d\n", accept_count);
  cli_out (cli, "No. of successful accept  : %d\n", accept_succ_count);
  cli_out (cli, "No. of read attempts      : %d\n", read_count);
  cli_out (cli, "No. of successful read    : %d\n", read_succ_count);
  cli_out (cli, "No. of write attempts     : %d\n", write_count);
  cli_out (cli, "No. of successful write   : %d\n", write_succ_count);
}
#endif /* DEBUG */

#ifdef DEBUG
/*-------------------------------------------------------------------------
  Test IMI server APIs*/

CLI (imi_test,
     imi_test_cli,
     "imi-webtest LINE",
     "Test IMI Web Server APIs",
     "Full API syntax")
{
  struct imi_serv *s;

  s = imi_server_new ();
  if (! s)
    {
      cli_out (cli, "Error creating IMI server\n");
      return CLI_ERROR;
    }

  pal_strncpy (s->buf, argv[0], pal_strlen (argv[0]));
  imi_server_execute (s);

  /* Print result. */
  if (pal_strlen (s->outbuf) > 0)
    cli_out (cli, "IMI Server Response:\n%s\n", s->outbuf);
  else
    cli_out (cli, "IMI Server Success (no response)\n");

  imi_server_free (s);

  return CLI_SUCCESS;
}
#endif /* DEBUG */

/*-------------------------------------------------------------------------
  Initialization / Shutdown */

/* IMI Server Shutdown. */
void
imi_server_shutdown ()
{
  struct listnode *node;
  struct imi_serv *client;

  /* Free each of the clients. */
  if (listcount (IMI->client_list) > 0)
    LIST_LOOP (IMI->client_list, client, node)
      {
        imi_server_free (client);
      }

  /* Close the server socket. */
  imi_server_sock_close ();
}

/* IMI Server Initialize. */
void
imi_server_init ()
{
  /* Create IMI server socket. */
  imi_server_sock_init ();
  IMI->client_list = list_new();

  /* Install special / extended APIs. */
  imi_server_install_extended_api (EXT_API_SHOW, "show",
                                   &imi_server_show_command);
  imi_server_install_extended_api (EXT_API_IP, "ip",
                                   &imi_server_ip_cmd);
  imi_server_install_extended_api (EXT_API_NO, "no",
                                   *imi_server_no_cmd);
  imi_server_install_extended_api (EXT_API_INTERFACE, "interface",
                                   &imi_server_interface_cmd);
  imi_server_install_extended_api (EXT_API_TUNNEL, "tunnel",
                                   &imi_server_tunnel_cmd);
#ifdef HAVE_DHCP_SERVER
  imi_server_install_extended_api (EXT_API_DHCPS, "dhcp-pool",
                                   &imi_server_dhcps_cmd);
#endif /* HAVE_DHCP_SERVER */
#ifdef HAVE_PPPOE
  imi_server_install_extended_api (EXT_API_PPPOE, "pppoe-client",
                                   &imi_server_pppoe_cmd);
#endif /* HAVE_PPPOE */
  imi_server_install_extended_api (EXT_API_CLEAR, "clear",
                                   &imi_server_clear_cmd);
  imi_server_install_extended_api (EXT_API_SPECIAL_GET, "get",
                                   &imi_server_get_cmd);
  imi_server_install_extended_api (EXT_API_COMMIT, "commit",
                                   &imi_server_commit_cmd);
  imi_server_install_extended_api (EXT_API_RIP_ENABLE, "rip-enable",
                                   &imi_server_rip_enable_cmd);
  imi_server_install_extended_api (EXT_API_RIP_DISABLE, "rip-disable",
                                   &imi_server_rip_disable_cmd);
#ifdef HAVE_IPV6
  imi_server_install_extended_api (EXT_API_RIPNG_ENABLE, "ripng-enable",
                                   &imi_server_ripng_enable_cmd);
  imi_server_install_extended_api (EXT_API_RIPNG_DISABLE, "ripng-disable",
                                   &imi_server_ripng_disable_cmd);
#endif /* HAVE_IPV6 */

#ifdef HAVE_IMI_DEFAULT_GW
  imi_server_install_extended_api (EXT_API_DEFAULT_GW, "default-gw",
                                   &imi_server_default_gw_cmd);
#endif /* HAVE_IMI_DEFAULT_GW */

#ifdef HAVE_IMI_WAN_STATUS
  imi_server_install_extended_api (EXT_API_WAN_STATUS, "wan-status",
                                   &imi_server_wan_status_cmd);
#endif /* HAVE_IMI_WAN_STATUS */

#ifdef DEBUG
  cli_install (imim->ctree, EXEC_MODE, &imi_test_cli);
#endif /* DEBUG */
}

#endif /* HOME_GATEWAY */

