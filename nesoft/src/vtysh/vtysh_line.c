/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>

#include "cli.h"
#include "line.h"
#include "network.h"
#include "snprintf.h"

#include "vtysh.h"
#include "vtysh_line.h"
#include "vtysh_system.h"
#include "vtysh_parser.h"

/* #define DEBUG */

/* Global "line" structure.  */
struct line *vtysh_line;

static pal_sock_handle_t imi_sock;

#ifndef HAVE_TCP_MESSAGE
/* Check we have permission to access the socket.  */
static int
vtysh_line_check (char *path)
{
  uid_t euid;
  gid_t egid;
  struct stat s_stat;
  int ret;

  /* Get uid and gid.  */
  euid = geteuid();
  egid = getegid();

  /* Get status of the IMI socket.  */
  ret = stat (path, &s_stat);
  if (ret < 0 && errno != ENOENT)
    return -1;

  /* When we get status information, we make it sure the file is
     socket and we have proper rights to read/write to the socket.  */
  if (ret >= 0)
    {
      if (! S_ISSOCK (s_stat.st_mode))
        return -1;

      if (euid != s_stat.st_uid
          || ! (s_stat.st_mode & S_IWUSR) || ! (s_stat.st_mode & S_IRUSR))
        return -1;
    }
  return 0;
}

/* Connect to IMI to use line services.  */
int
vtysh_line_open (char *path)
{
  int ret;

  /* Check socket status.  */
  ret = vtysh_line_check (path);
  if (ret < 0)
    return ret;

  return line_client_sock (vtyshm, path);
}
#else /* HAVE_TCP_MESSAGE */
/* Connect to IMI to use line services. */
int
vtysh_line_open (int port)
{
  int ret;

  return line_client_sock (vtyshm, port);
}
#endif /* ! HAVE_TCP_MESSAGE */

/* Read one line from IMI.  */
void
vtysh_line_read ()
{
  int nbytes;
  u_int16_t length;
  struct vtysh_line *vtysh_line = &imiline;
  struct line line;
  u_char *pnt;
  u_int16_t size;

  /* Set NULL at the beginning of the line.  */
  *(vtysh_line->buf + LINE_HEADER_LEN) = '\0';

  /* Message header check.  */
  do
    {
      nbytes = pal_sock_recv (imi_sock, vtysh_line->buf, LINE_HEADER_LEN,
                              MSG_PEEK);
      if (nbytes <= 0)
        {
          fprintf (stderr, "%% Connection is closed!\n");
          vtysh_exit (0);
        }
      pal_mem_cpy (&length, vtysh_line->buf, sizeof length);
    }
  while (nbytes < LINE_HEADER_LEN);

  nbytes = pal_sock_read (imi_sock, vtysh_line->buf, length);
  if (nbytes <= 0)
    {
      fprintf (stderr, "%% Connection is closed\n");
      vtysh_exit (0);
    }

  pnt = vtysh_line->buf;
  size = LINE_MESSAGE_MAX;
  line_decode (&pnt, &size, &line);

  vtysh_line->code = line.code;
}

/* Utility function to write the line to IMI.  */
int
vtysh_line_write (pal_sock_handle_t sock, char *buf, u_int16_t length)
{
  int nbytes;

  /* Check socket.  */
  if (sock < 0)
    {
      fprintf (stderr, "Connection to IMI is gone\n");
      return -1;
    }

  /* Send the message.  */
  nbytes = writen (sock, buf, length);
  if (nbytes <= 0)
    {
      fprintf (stderr, "%% Connection is closed\n");
      return -1;
    }

  /* Check written value.  */
  if (nbytes != length)
    {
      fprintf (stderr, "IMI socket write was partial!\n");
      return -1;
    }
  return nbytes;
}

/* Send line.  */
void
vtysh_line_send (int mode, const char * format, ...)
{
  va_list args;
  int len;
  u_int16_t length;
  struct line line;
  char buf[LINE_MESSAGE_MAX];
  int nbytes;
  u_char *pnt;
  u_int16_t size;

  /* Format strings.  */
  va_start (args, format);
  len = zvsnprintf (buf + LINE_HEADER_LEN,
                    LINE_MESSAGE_MAX - LINE_HEADER_LEN, format, args);
  va_end (args);

  /* Set length.  */
  length = pal_strlen (buf + LINE_HEADER_LEN) + LINE_HEADER_LEN + 1;

  /* Fill in line structure.  */
  line.module = PM_EMPTY;
  line.key = 0;
  line.mode = mode;
  line.privilege = vtysh_privilege_get ();
  line.code = LINE_CODE_COMMAND;
  line.length = length;

  /* Format header.  */
  pnt = buf;
  size = LINE_MESSAGE_MAX;
  line_encode (&pnt, &size, &line);

  /* Write the line.  */
  nbytes = vtysh_line_write (imi_sock, buf, length);
  if (nbytes < 0)
    vtysh_exit (1);
}

/* Send CLI command message to IMI. */
void
vtysh_send_cli (struct cli *cli, int argc, char **argv, u_int32_t module)
{
  struct line line;
  u_int16_t length;
  char buf[LINE_MESSAGE_MAX];
  int nbytes;
  u_char *pnt;
  u_int16_t size;

  return;

  if (module == 0)
    return;

  length = pal_strlen (cli->str) + LINE_HEADER_LEN + 1;

  /* Fill in line structure.  */
  line.module = cli->cel->module;
  line.key = cli->cel->key;
  line.mode = cli->mode;
  line.privilege = vtysh_privilege_get ();
  line.code = LINE_CODE_COMMAND;
  line.length = length;

  pnt = buf;
  size = LINE_MESSAGE_MAX;
  line_encode (&pnt, &size, &line);

  pal_mem_cpy (buf + LINE_HEADER_LEN, cli->str, pal_strlen (cli->str) + 1);

  /* Write the line.  */
  nbytes = vtysh_line_write (imi_sock, buf, length);
  if (nbytes < 0)
    printf ("write failed to module %ul\n", module);
}

/* Get response from IMI after passing configuration command. */
int
vtysh_line_get_reply ()
{
  struct vtysh_line *line = &imiline;
  vtysh_line_read ();
  return line->code;
}

/* Print error responses from IMI. */
void
vtysh_line_print_response ()
{
  struct vtysh_line *line = &imiline;
  printf ("%s", line->message);
}

void
vtysh_line_execute (struct vtysh_line *line)
{
  /* Execute the line.  */
  if (line->code == LINE_CODE_EOL)
    return;

  if (line->message[0] == '\0')
    printf ("empty message\n");
}

/* "end" moves back to EXEC_MODE.  */
void
vtysh_line_end ()
{
  if (vtysh_line->cli.mode != EXEC_MODE)
    vtysh_parse ("end", vtysh_line->cli.mode);
}

/* Close IMI socket.  */
void
vtysh_line_close ()
{
  pal_sock_close (vtyshm, imi_sock);
}

/* Initialize IMI shell line.  */
void
vtysh_line_init ()
{
  imiline.cli_key = 0;
  imiline.module = 0;
  imiline.message = imiline.buf + LINE_HEADER_LEN;

  vtysh_line = XCALLOC (MTYPE_TMP, sizeof (struct line));
  vtysh_line->cli.zg = vtyshm;
  vtysh_line->cli.mode = EXEC_MODE;
  vtysh_line->cli.privilege = 1;
  vtysh_line->cli.out_func = (CLI_OUT_FUNC) fprintf;
  vtysh_line->cli.out_val = stdout;
  vtysh_line->cli.lines = -1;
}

int
vtysh_mode_get ()
{
  return vtysh_line->cli.mode;
}

u_char
vtysh_privilege_get ()
{
  return vtysh_line->cli.privilege;
}

void
vtysh_privilege_set (int privilege)
{
  vtysh_line->cli.privilege = privilege;
}

void
vtysh_no_pager ()
{
  vtysh_line->cli.lines = 0;
}
