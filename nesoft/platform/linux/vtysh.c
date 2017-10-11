/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "version.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

void vtysh_start (char *, int, int, char *);

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
-e, --eval               Execute argument as command\n\
-h, --help               Display this help and exit\n\
\n\
Report bugs to %s\n", progname, PACOS_BUG_ADDRESS);
    }
  exit (status);
}

#ifdef HAVE_GETOPT_H
/* VTY shell options, we use GNU getopt library. */
struct option longopts[] = 
{
  { "boot",                 no_argument,             NULL, 'b'},
  { "eval",                 required_argument,       NULL, 'e'},
  { "file",                 required_argument,       NULL, 'f'},
  { "help",                 no_argument,             NULL, 'h'},
  { 0 }
};
#endif /* HAVE_GETOPT_H */

/* IMI shell main routine.  */
int
main (int argc, char **argv, char **env)
{
  char *p;
  int opt;
  int eval_flag = 0;
  char *eval_line = NULL;
  char *progname;
  char *integrate_file = NULL;
  int boot_flag = 0;

  /* Set umask before anything for security.  */
  umask (0027);

  /* Preserve name of myself.  */
  progname = ((p = pal_strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Option handling.  */
  while (1) 
    {
#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "be:f:h", longopts, 0);
#else
      opt = getopt (argc, argv, "be:f:h");
#endif /* HAVE_GETOPT_H */
    
      if (opt == EOF)
        break;

      switch (opt) 
        {
        case 0:
          break;
        case 'b':
          boot_flag = 1;
          break;
        case 'e':
          eval_flag = 1;
          eval_line = optarg;
          break;
        case 'f':
          integrate_file = optarg;
          boot_flag = 1;
          break;
        case 'h':
          usage (0, progname);
          break;
        default:
          usage (1, progname);
          break;
        }
    }

  /* Start IMI shell.  */
  vtysh_start (integrate_file, boot_flag, eval_flag, eval_line);

  /* Shell is terminated.  */
  printf ("\n");
  
  /* Rest in peace.  */
  return 0;
}
