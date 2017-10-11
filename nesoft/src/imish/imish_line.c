/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>

#include "cli.h"
#include "line.h"
#include "network.h"
#include "snprintf.h"
#include "message.h"
#include "imi_client.h"

#include "imish/imish.h"
#include "imish/imish_line.h"
#include "imish/imish_system.h"
#include "imish/imish_parser.h"
#include "imish/imish_readline.h"


int
imish_mode_get (struct line *line)
{
  return line->cli.mode;
}

u_char
imish_privilege_get (struct line *line)
{
  return line->cli.privilege;
}

/* Utility function to write the line to IMI.  */
int
imish_line_write (int sock, char *buf, u_int16_t length)
{
  int nbytes;

  /* Check socket.  */
  if (sock < 0)
    {
      fprintf (stderr, "%% Connection to IMI is gone!\n");
      return -1;
    }

  /* Send the message.  */
  nbytes = writen (sock, (u_char *)buf, length);
  if (nbytes <= 0)
    {
      fprintf (stderr, "%% Connection is closed!\n");
      return -1;
    }

  /* Check written value.  */
  if (nbytes != length)
    {
      fprintf (stderr, "%% IMI socket write was partial!\n");
      return -1;
    }
  return nbytes;
}

/* Read one line from IMI.  */
void
imish_line_read (struct line *line)
{
  int nbytes;

  /* Message header check.  */
  nbytes = readn (line->sock, (u_char *)line->buf, LINE_HEADER_LEN);
  if (nbytes != LINE_HEADER_LEN)
    {
      fprintf (stderr, "%% Connection is closed!\n");
      imish_exit (1);
    }

  /* Get the length and reset the buffer.  */
  line_header_decode (line);

  if (line->length < LINE_HEADER_LEN || line->length > LINE_MESSAGE_MAX)
    {
      fprintf (stderr, "%% Connection is closed!\n");
      imish_exit (1);
    }

  nbytes = line->length - LINE_HEADER_LEN;
  if (nbytes && readn (line->sock, (u_char *)line->str, nbytes) != nbytes)
    {
      fprintf (stderr, "%% Connection is closed!\n");
      imish_exit (1);
    }

  /* Copy privilege to the CLI member.  */
  if (line->cli.privilege != PRIVILEGE_NORMAL)
    line->cli.privilege = line->privilege;
}

/* Send line.  */
void
imish_line_send (struct line *line, const char *format, ...)
{
  int nbytes;
  va_list args;

  /* Fill up the line structure.  */
  line->module = PM_IMISH;
  line->code = LINE_CODE_COMMAND;

  /* Format strings.  */
  va_start (args, format);
  zvsnprintf (line->str, LINE_BODY_LEN, format, args);
  va_end (args);

  /* Encode line header.  */
  line_header_encode (line);

  /* Write the line.  */
  nbytes = imish_line_write (line->sock, line->buf, line->length);
  if (nbytes < 0)
    imish_exit (1);
}

/* Send CLI command message to IMI. */
void
imish_line_send_cli (struct line *line, struct cli *cli)
{
  int nbytes;

  /* Fill up the line structure.  */
  line->module = cli->cel->module;
  line->key = cli->cel->key;
  line->mode = cli->mode;
  line->code = LINE_CODE_COMMAND;
  pal_strcpy (line->str, cli->str);

  /* Encode line header.  */
  line_header_encode (line);

  /* Write the line.  */
  nbytes = imish_line_write (line->sock, line->buf, line->length);
  if (nbytes < 0)
    imish_exit (1);
}

/* Print error responses from IMI. */
void
imish_line_print (struct line *line)
{
  printf ("%s", line->str);
}

void
imish_line_execute (struct line *line)
{
  /* Execute the line.  */
  if (line->code == LINE_CODE_EOL)
    return;

  if (line->str[0] == '\0')
    printf ("empty message\n");
  else
    imish_parse (line->str, IMISH_MODE);
}

/* "exit" till the CONFIG_MODE.  */
void
imish_line_config (struct line *line)
{
  while (line->cli.mode > CONFIG_MODE)
    {
      /* Send command to IMI. */
      imish_line_send (line, "exit");

      /* Get IMI reply.  */
      imish_line_read (line);

      /* Update the mode.  */
      line->cli.mode = line->mode;
    }
}

/* "end" moves back to EXEC_MODE.  */
void
imish_line_end (struct line *line)
{
  if (line->cli.mode != EXEC_MODE)
    {
      /* Send command to IMI. */
      imish_line_send (line, "end");

      /* Get IMI reply.  */
      imish_line_read (line);

      /* Update the mode.  */
      line->cli.mode = line->mode;
    }
}

/* Close IMI socket.  */
void
imish_line_close (struct line *line)
{
  pal_sock_close (imishm, line->sock);
}

/* Synchronize with IMI.  */
void
imish_line_sync (struct line *line)
{
  line->mode = IMISH_MODE;
  imish_line_send (line, "sync");
  do
    {
      imish_line_read (line);
      if (line->code == LINE_CODE_ERROR)
        {
          /* Something wrong happened, just exit.  */
          pal_fprintf (stderr, "%% Connection is closed!\n");
          imish_exit (1);
        }
      imish_line_execute (line);
    }
  while (line->code != LINE_CODE_EOL);
}

/* Get FIB_ID for given VRF name from IMI */
void
imish_line_sync_vrf (struct line *line, char *vrf_name)
{
  line->mode = IMISH_MODE;

  if (vrf_name)
    imish_line_send (line, "sync-vrf %s", vrf_name);
  else
    imish_line_send (line, "sync-vrf");

  do
    {
      imish_line_read (line);
      if (line->code == LINE_CODE_ERROR)
        {
          /* Something wrong happened, just exit.  */
          pal_fprintf (stderr, " %% Connection is closed!\n");
          imish_exit (1);
        }
      imish_line_execute (line);
    }
  while (line->code != LINE_CODE_EOL);
}

#define IMISH_LOGIN_NAME_DEFAULT        "PACOS"
#define IMISH_LOGIN_FAILURE_MAX         3

void
imish_line_auth (struct line *line, int is_login_local)
{
  int i;

  printf ("\nUser Access Verification\n\n");

  for (i = 0; i < IMISH_LOGIN_FAILURE_MAX; i++)
    {
      char *l = NULL;
      char *user = NULL;
      char *pass = NULL;
      int userlen = -1;
      int passlen = -1;

      if (is_login_local)
        {
          l = imish_gets ("Username: ");
          user = XSTRDUP (MTYPE_TMP, l);
          userlen = pal_strlen (user);

          if (userlen <= 0)
            {
              printf ("%% Incorrect Login\n\n");
              XFREE (MTYPE_TMP, user);
              continue;
            }
        }

      pass = getpass ("Password: ");
      passlen = pal_strlen (pass);

      if (passlen <= 0)
        {
          XFREE (MTYPE_TMP, user);
          continue;
        }

      line->mode = IMISH_MODE;
      imish_line_send (line, "login-auth %s %s\n",
                       user ? user : IMISH_LOGIN_NAME_DEFAULT, pass);
      /* Successful login returns error to send vrf_id. */
      imish_line_read (line);

      if (user != NULL)
        XFREE (MTYPE_TMP, user);

      /* Here privilege will be given. */

      if (line->code == LINE_CODE_SUCCESS)
        return;
    }
  printf ("%% Bad passwords\n");
  sleep (1);
  imish_exit (0);
}

void
imish_line_context_set (struct line *line, char *name, int from_console)
{
  int nbytes;

  line->mode = IMISH_MODE;
  line->module = PM_IMISH;
  line->code = LINE_CODE_CONTEXT_SET;

  /* Use key value for telling console type.  */
  if (from_console)
    line->key = LINE_TYPE_CONSOLE;
  else
    line->key = LINE_TYPE_VTY;

  if (name == NULL)
    line->str[0] = '\0';
  else
    zsnprintf (line->str, LINE_BODY_LEN, "%s", name);

  /* Encode line header. */
  line_header_encode (line);

  /* Write the line. */
  nbytes = imish_line_write (line->sock, line->buf, line->length);
  if (nbytes < 0)
    imish_exit (0);

  imish_line_read (line);
  if (line->code == LINE_CODE_ERROR)
    {
      if (strlen(line->str))
        fprintf (stderr, "%% %s", line->str);
      else
        fprintf (stderr, "No such VR\n");
      imish_exit (1);
    }
  line->cli.vr = apn_vr_get_by_id (imishm, line->vr_id);
}

/* Register shell information to IMI.  */
void
imish_line_register (struct line *line, char *name)
{
  int is_login_local = 1;
  int from_console = 0;
  char *tty;

  /* Read initial response from IMI.  */
  imish_line_read (line);

  /* Check this is console or not.  */
  tty = ttyname (PAL_STDIN_FILENO);
  if (tty && pal_strstr (tty, "console"))
    from_console = 1;

  /* Set context. */
  imish_line_context_set (line, name, from_console);

  /* Register user.  */
  line->mode = IMISH_MODE;
  imish_line_send (line, "user %d", getuid ());
  imish_line_read (line);

  /* Register tty. */
  if (tty)
    {
      line->mode = IMISH_MODE;
      imish_line_send (line, "tty %s", tty);
      imish_line_read (line);
    }

  /* User login. */
  if (line->code == LINE_CODE_AUTH_REQUIRED)
    imish_line_auth (line, is_login_local);

  /* Register pid. */
  line->mode = IMISH_MODE;
  imish_line_send (line, "pid %d", getpid ());
  imish_line_read (line);

  imish_line_sync (line);
}

/* Initialize IMI shell line.  */
void
imish_line_init (struct lib_globals *zg, char *name)
{
  struct line *line;
  struct imi_client *ic;

#ifndef HAVE_TCP_MESSAGE
  /* Check the line permition.  */
  if (pal_sock_check (IMI_LINE_PATH) < 0)
    {
      fprintf (stderr, "%% Please check the permision.\n");
      pal_exit (1);
    }
#endif /* HAVE_TCP_MESSAGE */

  /* Open IMI line.  */
  if (imi_client_create (zg, 1) < 0)
    {
      fprintf (stderr, "%% Can't connect to IMI socket: %s\n",
               pal_strerror (errno));
      pal_exit (1);
    }
  ic = zg->imh->info;

  if (pal_fcntl (ic->mc->sock, F_SETFD, FD_CLOEXEC) < 0)
    {
      pal_fprintf (stderr, "%% Can't set FD_CLOEXEC to IMI socket: %s\n",
               pal_strerror (errno));
      pal_exit(1);
    }


  /* Initialize line structure.  */
  line = &ic->line;
  line->zg = zg;
  line->sock = ic->mc->sock;
  line->str = &line->buf[LINE_HEADER_LEN];
  line->cli.zg = zg;
  line->cli.mode = EXEC_MODE;
  line->cli.privilege = PRIVILEGE_NORMAL;
  line->cli.out_func = (CLI_OUT_FUNC) fprintf;
  line->cli.out_val = stdout;
  line->cli.lines = -1;

  /* Send initial registration messages.  */
  imish_line_register (line, name);
}
