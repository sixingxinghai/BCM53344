/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#include "lib.h"
#include "version.h"
#include "log.h"

#include "nsm/nsmd.h"

int nsm_start (u_int16_t, u_int16_t, u_int16_t, char *,
               u_int16_t, u_int16_t, char *, u_int32_t, u_int16_t);
void nsm_stop (void);

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
#ifdef HAVE_RMM_TEST
  { "parent_pid",  required_argument, NULL, 'p'},
#endif
  { "vty_port",    required_argument, NULL, 'P'},
  { "version",     no_argument,       NULL, 'v'},
#ifdef HAVE_VRF
  { "no_fib",      no_argument,       NULL, 'n'},
#endif
  { "ha_cold",     no_argument,       NULL, 'c'}, /* HA cold start if used. */
  { 0 }
};
#endif /* HAVE_GETOPT_H */

/* SIGHUP handler. */
void 
sighup (int sig)
{
  zlog_info (NSM_ZG, "SIGHUP received");
}

/* SIGINT handler. */
void
sigint (int sig)
{
  zlog_info (NSM_ZG, "Terminating on signal");

  /* Stop NSM module.  */
  nsm_stop ();

  exit (0);
}

#ifdef HAVE_RMM_TEST
/*------------------------------------------------------------*
   * To test the redundancy feature (RMM & NSM-RD), we use a    *
   * Unix control program 'rpacos' defined in the PacOS/rmm     *
   * directory.  This program responds to signals SIGUSR1 and   *
   * SIGUSR2 sent to it by the NSM during state transtions.     *
   * Recall that protocols have two states - ON (sigusr1) and   *
   * OFF (sigusr2) - for the phase I development.  Under some   *
   * flavors of Unix, signals are slightly different.  In       *
   * particular, the signals are executed on both the parent    *
   * process AND the child[ren].  For example, when the NSM     *
   * sends rpacos the SIGUSR1 signal to start the protocols,    *
   * the handler sigusr1 defined in /rmm/rmm_main.c will be     *
   * executed first, followed by the handler in /nsm/main.c.    *
   * For this reason, we redefine the sigusr1 handler here      *
   * to do nothing.  Furthermore, we must also define the       *
   * sigusr2 handler here, else the nsm process will exit when  *
   * it is received.                                            *
   *                                                            *
   * Note: For phase II development, the state information will *
   *       be propegated to the protocol processes / tasks      *
   *       by the NSM and this strategy will no longer be       *
   *       needed (ie, this is temporary).                      *
   *------------------------------------------------------------*/
/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
}

/* SIGUSR2 handler. */
void
sigusr2 (int sig)
{
}
#else
/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  zlog_rotate ( NSM_ZG, NSM_ZG->log);
}
#endif /* HAVE_RMM_TEST */

/* Initialization of signal handles.  */
void
signal_init ()
{
  pal_signal_init ();
  pal_signal_set (SIGHUP, sighup);
  pal_signal_set (SIGINT, sigint);
  pal_signal_set (SIGTERM, sigint);
  pal_signal_set (SIGUSR1, sigusr1);
#ifdef HAVE_RMM_TEST
  pal_signal_set (SIGUSR2, sigusr2);
#endif /* HAVE_RMM_TEST */
}

/* Help information display. */
void
nsm_usage (s_int32_t status, char *progname)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {    
      printf ("Usage : %s [OPTION...]\n\n\
Daemon which manages kernel routing table management and \
redistribution between different routing protocols.\n\n\
-b, --batch        Runs in batch mode\n\
-c, --ha_cold      High availability - cold start\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-k, --keep_kernel  Don't delete old routes which installed by zebra.\n\
-l, --log_mode     Set verbose log mode flag\n\
-P, --vty_port     Set vty's port number\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
\n\
Report bugs to %s\n", progname, PACOS_BUG_ADDRESS);
    }

  exit (status);
}

/* Main startup routine. */
int
main (int argc, char **argv)
{
  result_t ret;
  char *p;
  u_int16_t vty_port_l = 0;
  u_int16_t batch_mode_l = 0;
  u_int16_t ha_cold_l = 0;
  u_int16_t daemon_mode_l = 0;
  u_int16_t keep_kernel_mode_l = 0;
  u_int32_t parent_pid_l;
  char *config_file_l = NULL;
  char *progname_l;
  u_int16_t no_fib_mode_l = 0;

  /* Set umask before anything for security */
  umask (0077);

  /* preserve my name */
  progname_l = ((p = pal_strrchr (argv[0], '/')) ? ++p : argv[0]);

  while (1)
    {
      int opt;
      
#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "bcdklf:hP:rvn", longopts, 0);
#else
      opt = getopt (argc, argv, "bcdklf:hP:rvn");
#endif /* HAVE_GETOPT_H */
      
      if (opt == EOF)
        break;

      switch (opt)
        {
        case 0:
          break;
        case 'b':
          batch_mode_l = 1;
          break;
        case 'c':
          ha_cold_l = 1;
          break;
        case 'd':
          daemon_mode_l = 1;
          break;
        case 'k':
          keep_kernel_mode_l = 1;
          break;
        case 'l':
          /* log_mode = 1; */
          break;
        case 'f':
          config_file_l = optarg;
          break;
#ifdef HAVE_RMM_TEST
        case 'p':
          parent_pid_l = atoi (optarg);
          break;
#endif
        case 'P':
          vty_port_l = atoi (optarg);
          break;
        case 'v':
          print_version (progname_l);
          exit (0);
          break;
#ifdef HAVE_VRF
        case 'n':
          no_fib_mode_l = 1;
          break;
#endif
        case 'h':
          nsm_usage (0, progname_l);
          break;
        default:
          nsm_usage (1, progname_l);
          break;
        }
    }

  /* Initialize signal.  */
  signal_init ();

  /* Get options */
  parent_pid_l = 0;

  /* Start NSM module.  */
  ret = nsm_start (batch_mode_l, daemon_mode_l, keep_kernel_mode_l,
                   config_file_l, vty_port_l, no_fib_mode_l,
                   progname_l, parent_pid_l, ha_cold_l);
  if (ret != 0)
    fprintf (stderr, "Error loading PacOS... Aborting...\n");

  /* Not reached... */
  exit (0);
}
