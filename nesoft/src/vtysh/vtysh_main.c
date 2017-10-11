/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>
#include <setjmp.h>

#include "vtysh.h"
#include "vtysh_line.h"
#include "vtysh_parser.h"
#include "vtysh_cli.h"
#include "vtysh_system.h"
#include "vtysh_readline.h"
#include "vtysh_exec.h"
#include "vtysh_config.h"
#include "vtysh_pm.h"

/* For sigsetjmp () & siglongjmp ().  */
sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call.  */
int jmpflag = 0;

/* Global variables . */
struct lib_globals *vtyshm;

pal_sock_handle_t imi_sock;

void vtysh_signal_init ();

char *config_file;

extern char *input_line;

void
vtysh_init ()
{
  /* Signal init.  */
  vtysh_signal_init ();

  /* System init.  */
  vtysh_system_init ();

  /* Initialize cli.  */
  vtysh_cli_init ();

  /* Readline init.  */
  vtysh_readline_init ();

  /* Set pager.  */
  vtysh_set_pager ();

  /* Initialize line.  */
  vtysh_line_init ();

  /* Initialize pm.  */
  vtysh_pm_init ();

  /* Initializa configuration.  */
  vtysh_config_init ();

  /* "show" command init. */
  vtysh_cli_show_init ();
}

/* Start IMI shell.  */
void
vtysh_start (char *integrate_file, int boot_flag, int eval_flag,
             char *eval_line)
{
  result_t ret;

  /* Initialize memory. */
  memory_init (APN_PROTO_VTYSH);

  /* Allocate lib globals. */
  vtyshm = lib_create ("vtysh");
  if (! vtyshm)
    {
      fprintf (stderr, "Can't allocate memory: lib_create()\n");
      exit (1);
    }

  ret = lib_start (vtyshm);
  if (ret < 0)
    {
      lib_stop (vtyshm);
      fprintf (stderr, "Error: lib_start()\n");
      exit (1);
    }


  /* Assign protocol number.  */
  vtyshm->protocol = APN_PROTO_VTYSH;

  /* Initialize host.  */
  vtyshm->host = host_new ();

  /* Set configuration file name.  */
  if (integrate_file)
    config_file = integrate_file;
  else
    config_file = VTYSH_DEFAULT_CONFIG_FILE;

  /* Initialize IMI shell.  */
  vtysh_init ();

  /* Boot flag.  */
  if (boot_flag)
    {
      vtysh_startup_config (config_file, 0);
      exit (0);
    }

  /* Boot fetch.  */
  vtysh_startup_config (config_file, 1);

  /* Advanced-vty configuration check.  */
  if (CHECK_FLAG (vtyshm->host->flags, HOST_ADVANCED_VTYSH))
    vtysh_privilege_set (PRIVILEGE_MAX);

  /* Eval.  */
  if (eval_flag)
    {
      input_line = eval_line;
      vtysh_no_pager ();
      vtysh_parse (NULL, 0);
      exit (0);
    }

  /* Display motd.  */
  printf ("%s\n", vtyshm->host->motd);

  /* Set longjump point at here.  */
  sigsetjmp (jmpbuf, 1);
  jmpflag = 1;

  /* Main command loop. */
  while (vtysh_gets (vtysh_prompt()))
    vtysh_parse (NULL, 0);
}

/* Stop IMI shell.  */
void
vtysh_stop ()
{
  /* Teardown IMI connection. */
  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (vtyshm);


  /* Call lib_stop(). */
  lib_stop (vtyshm);
}
