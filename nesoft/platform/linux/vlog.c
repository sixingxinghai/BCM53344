/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#include "lib.h"
#include "version.h"
#include "log.h"

#include "lib/vlog_client.h"
#include "vlogd/vlogd.h"

#include <stdlib.h>
#include <sys/stat.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

extern VLOG_GLOBALS *vlog_vgb;

int vlog_start (s_int32_t, char *, s_int32_t, char *);
void vlog_stop (void);

#ifdef HAVE_GETOPT_H
/* vlogd options. */
static struct option longopts[] =
{
  { "daemon",      no_argument,       NULL, 'd'},
  { "help",        no_argument,       NULL, 'h'},
  { "version",     no_argument,       NULL, 'v'},
  { 0 }
};
#endif /* HAVE_GETOPT_H */

/* Help information display. */
void
vlog_usage (int status, char *progname)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {
      printf ("Usage : %s [OPTION...]\n\
Daemon which manages VLOG.\n\n\
-d, --daemon       Runs in daemon mode\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
\n\
Report bugs to %s\n", progname, PACOS_BUG_ADDRESS);
    }

  exit (status);
}

/* SIGHUP handler. */
void
sighup (int sig)
{
  vlog_terms_broadcast(vlog_vgb->vgb_terms, "SIGHUP received");
}

/* SIGINT handler. */
void
sigint (int sig)
{
  vlog_terms_broadcast(vlog_vgb->vgb_terms, "SIGINT received - terminating");

  /* Stop VLOG module.  */
  vlog_stop ();

  exit (0);
}

/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  vlog_terms_broadcast(vlog_vgb->vgb_terms, "SIGUSR1 received");
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

/* Main routine of vlogd. */
int
main (int argc, char **argv)
{
  result_t ret;
  char *p;
  s_int32_t daemon_mode = 0;
  char *config_file = NULL;
  s_int32_t vty_port = -1;
  char *progname;

  /* Set umask before anything for security */
  umask (0027);

  /* Get program name. */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Command line option parse. */
  while (1)
    {
      int opt;

#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "dhv", longopts, 0);
#else
      opt = getopt (argc, argv, "dhv");
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
        case 'v':
          print_version (progname);
          exit (0);
        case 'h':
          vlog_usage (0, progname);
          break;
        default:
          vlog_usage (1, progname);
          break;
        }
    }

  /* Initialize signal.  */
  signal_init ();

  /* Start VLOG module.  */
  ret = vlog_start (daemon_mode, config_file, vty_port, progname);

  /* Not reached. */
  exit (0);
}

