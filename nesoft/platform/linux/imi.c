/* Copyright (C) 2001, 2002  All Rights Reserved. */

#include "pal.h"
#include <setjmp.h>
#include "lib.h"
#include "version.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#include "imi/imi.h"

/* VTY port number. */
int vty_port = IMI_VTY_PORT;

/* For sigsetjmp() & siglongjmp(). */
sigjmp_buf jmpbuf;

/* Flag for avoid recursive siglongjmp() call. */
int jmpflag = 0;

int imi_start (int, char *, int, char *, int);
void imi_stop (void);

/* SIGHUP handler. */
void 
sighup (int sig)
{
  zlog_info (imim, "SIGHUP received");
}

/* SIGINT and SIGTSTP handler.  This function care user's ^Z and ^C
   input.  */
void
sigint (int sig)
{
  /* Stop IMI module.  */
  imi_stop ();

  exit (0);
}

/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  zlog_info (imim, "SIGUSR1 received");
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

/* Help information display. */
static void
usage (int status, char *progname)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {    
      printf ("Usage : %s [OPTION...]\n\n\
Daemon which manages kernel routing table management and \
redistribution between different routing protocols.\n\n\
-d, --daemon             Runs in daemon mode\n\
-b, --boot               Execute boot startup configuration\n\
-e, --eval               Execute argument as command\n\
-f, --file               Execute this config file\n\
-h, --help               Display this help and exit\n\
-T, --Telnet             IMI should run on default Telnet port(23)\n\
\n\
Report bugs to %s\n", progname, PACOS_BUG_ADDRESS);
    }
  exit (status);
}

#ifdef HAVE_GETOPT_H
/* VTY shell options, we use GNU getopt library. */
struct option longopts[] = 
{
  { "daemon",               no_argument,             NULL, 'd'},
  { "boot",                 no_argument,             NULL, 'b'},
  { "eval",                 required_argument,       NULL, 'e'},
  { "file",                 required_argument,       NULL, 'f'},
  { "help",                 no_argument,             NULL, 'h'},
  { "Telnet",               no_argument,             NULL, 'T'},
  { 0 }
};
#endif /* HAVE_GETOPT_H */

/* VTY shell main routine. */
int
main (int argc, char **argv, char **env)
{
  char *p;
  int opt;
  int eval_flag = 0;
  char *eval_line = NULL;
  char *config_file = NULL;
  int daemon_mode = 0;
  int boot_flag = 0;
  int ret;

  char *progname;

  /* Set umask before anything for security. */
  umask (0027);

  /* Preserve name of myself. */
  progname = ((p = pal_strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Option handling. */
  while (1) 
    {
#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "dbef:hT", longopts, 0);
#else
      opt = getopt (argc, argv, "dbef:hT");
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
        case 'b':
          boot_flag = 1;
          break;
        case 'e':
          eval_flag = 1;
          eval_line = optarg;
          break;
        case 'h':
          usage (0, progname);
          break;
        case 'f':
          config_file = optarg;
          boot_flag = 1;
          break;
        case 'T':
          vty_port = 23;        /* default telnet port */
          break;
        default:
          usage (1, progname);
          break;
        }
    }

  /* Initialize signal.  */
  signal_init ();

  /* Start IMI module.  */
  ret = imi_start (daemon_mode, config_file, vty_port, progname, boot_flag);
  if (ret != 0)
    fprintf (stderr, "Error loading IMI... Aborting...\n");

  /* Not reached... */
  exit (0);
}
