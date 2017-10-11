/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#include "lib.h"
#include "version.h"
#include "log.h"

#include <stdlib.h>
#include <sys/stat.h>

#include "elmid/elmid.h"
#include "elmid/elmi_types.h"

/* Global variable container. */

int elmi_start (s_int32_t, char *, s_int32_t, char *);
void elmi_stop (void);

#ifdef HAVE_GETOPT_H
/* elmi options. */
static struct option longopts[] =
{
  { "daemon",      no_argument,       NULL, 'd'},
  { "config_file", required_argument, NULL, 'f'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "version",     no_argument,       NULL, 'v'},
  { 0 }
};
#endif /* HAVE_GETOPT_H */

/* Help information display. */
void
elmi_usage (int status, char *progname)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {
      printf ("Usage : %s [OPTION...]\n\
Daemon which manages ELMI \n\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-P, --vty_port     Set vty's port number\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
-c, --ha_cold      High availability - cold start\n\
\n\
Report bugs to %s\n", progname, PACOS_BUG_ADDRESS);
    }

  exit (status);
}

/* SIGHUP handler. */
void
sighup (int sig)
{
  zlog_info (ZG, "SIGHUP received");

  /* Stop ELMI module.  */
  elmi_stop ();
}

/* SIGINT handler. */
void
sigint (int sig)
{
  zlog_info (ZG, "Terminating on signal");

  /* Stop ELMI module.  */
  elmi_stop ();

  exit (0);
}

/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  zlog_rotate (ZG, ZG->log);
}

/* Initialization of signal handles.  */
void
signal_init ()
{
  pal_signal_init ();
  pal_signal_set (SIGHUP, sighup);
  pal_signal_set (SIGINT, sigint);
  pal_signal_set (SIGTERM, sigint);
  pal_signal_set (SIGUSR1, sigusr1);
}

/* Main routine of elmid. */
int
main (int argc, char **argv)
{
  result_t ret;
  char *p;
  s_int32_t daemon_mode = 0;
  char *config_file = NULL;
  s_int32_t vty_port = ELMI_VTY_PORT;
  char *progname;

  /* Set umask before anything for security */
  umask (0027);

#ifdef VTYSH
  /* Unlink vtysh domain socket. */
  unlink (ELMI_VTYSH_PATH);
#endif /* VTYSH */

  /* Get program name. */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Command line option parse. */
  while (1)
    {
      int opt;

#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "df:hP:rvc", longopts, 0);
#else
      opt = getopt (argc, argv, "df:hP:rvc");
#endif /* HAVE_GETOPT_H */

      if (opt == EOF)
        break;

      switch (opt)
        {
        case 0:
          break;
        case 'd':
          daemon_mode = 1;
          break;
        case 'f':
          config_file = optarg;
          break;
        case 'P':
          vty_port = atoi (optarg);
          break;
        case 'v':
          print_version (progname);
          exit (0);
          break;
        default:
          elmi_usage (1, progname);
          break;
        }
    }

  /* Initialize signal.  */
  signal_init ();

  /* Start ELMI module.  */
  ret = elmi_start (daemon_mode, config_file, vty_port, progname);

  /* Not reached. */
  exit (0);
}
