/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "version.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

void imish_start (char *, char *);


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
-f, --file               Execute this config file\n\
-h, --help               Display this help and exit\n\
-v, --vr                 Virtual Router name\n\
\n\
Report bugs to %s\n", progname, PACOS_BUG_ADDRESS);
    }
  exit (status);
}

#ifdef HAVE_GETOPT_H
/* VTY shell options, we use GNU getopt library. */
struct option longopts[] = 
{
  { "eval",                 required_argument,       NULL, 'e'},
  { "file",                 required_argument,       NULL, 'f'},
  { "help",                 no_argument,             NULL, 'h'},
  { "vr",                   required_argument,       NULL, 'v'},
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
  char *vr_name = NULL;
  char *file_name = NULL;

  /* Set umask before anything for security.  */
  umask (0027);

  /* Preserve name of myself.  */
  progname = ((p = pal_strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Option handling.  */
  while (1) 
    {
#ifdef HAVE_GETOPT_H
      opt = getopt_long (argc, argv, "ef:hv:", longopts, 0);
#else
      opt = getopt (argc, argv, "ef:hv:");
#endif /* HAVE_GETOPT_H */
    
      if (opt == EOF)
        break;

      switch (opt) 
        {
        case 0:
          break;
        case 'e':
          eval_flag = 1;
          eval_line = optarg;
          break;
        case 'f':
          file_name = optarg;
          break;
        case 'v':
          vr_name = optarg;
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
  imish_start (vr_name, file_name);

  /* Shell is terminated.  */
  printf ("\n");
  
  /* Rest in peace.  */
  return 0;
}
