/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#include "version.h"
#include "if.h"
#include "prefix.h"
#include "filter.h"
#include "vty.h"
#include "log.h"

#include "ospfd/ospfd.h"

/* Command line options. */
#ifdef HAVE_GETOPT_H
struct option longopts[] =
{
  { "batch",       no_argument,       NULL, 'b'},
  { "daemon",      no_argument,       NULL, 'd'},
  { "keep_kernel", no_argument,       NULL, 'k'},
  { "log_mode",    no_argument,       NULL, 'l'},
  { "config_file", required_argument, NULL, 'f'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "retain",      no_argument,       NULL, 'r'},
  { "version",     no_argument,       NULL, 'v'},
#ifdef HAVE_VRF
  { "no_fib",      no_argument,       NULL, 'n'},
#endif
  { 0 }
};
#endif /* HAVE_GETOPT_H */

/* Manually specified configuration file name. */
char *config_file_l = NULL;

/* VTY port number. */
int vty_port_l = OSPF_VTY_PORT;

int ospf_start (int, char *, int, char *);
void ospf_stop (mod_stop_cause_t);

/* Help information display. */
void
ospf_usage (int status, char *progname)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {
      printf ("Usage : %s [OPTION...]\n\
Daemon which manages OSPF.\n\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-P, --vty_port     Set vty's port number\n\
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
  zlog (ZG, NULL, ZLOG_INFO, "SIGHUP received");
}

/* SIGINT handler. */
void
sigint (int sig)
{
  zlog (ZG, NULL, ZLOG_INFO, "Terminating on signal");

  /* Stop OSPF module.  */
  ospf_stop (MOD_STOP_CAUSE_USER_KILL);

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

/* OSPFd main routine. */
int
main (int argc, char **argv)
{
  result_t ret;
  char *p;
  int daemon_mode_l = 0;
  char *progname_l;

  /* Set umask before anything for security. */
  umask (0027);

#ifdef VTYSH
  /* Unlink vtysh domain socket. */
  unlink (OSPF_VTYSH_PATH);
#endif /* VTYSH */

  /* Preserve name of myself. */
  progname_l = ((p = pal_strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Command line argument treatment. */
  while (1)
    {
      int opt;

#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "df:hP:v", longopts, 0);
#else
      opt = getopt (argc, argv, "df:hP:v");
#endif /* HAVE_GETOPT_H */

      if (opt == EOF)
        break;

      switch (opt)
        {
        case 0:
          break;
        case 'd':
          daemon_mode_l = 1;
          break;
        case 'f':
          config_file_l = optarg;
          break;
        case 'P':
          vty_port_l = atoi (optarg);
          break;
        case 'v':
          print_version (progname_l);
          exit (0);
        case 'h':
          ospf_usage (0, progname_l);
          break;
        default:
          ospf_usage (1, progname_l);
          break;
        }
    }

  /* Initialize signal.  */
  signal_init ();

  mod_stop_reg_cb(APN_PROTO_OSPF, ospf_stop);

  /* Start OSPF module.  */
  ret = ospf_start (daemon_mode_l, config_file_l, vty_port_l, progname_l);

  /* Not reached. */
  exit (0);
}
