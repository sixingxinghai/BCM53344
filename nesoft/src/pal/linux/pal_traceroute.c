/* Copyright (C) 2003   All Rights Reserved. */
#include "pal.h"
#include "pal_traceroute.h"
#include "memory.h"

/* Map traceroute commands. */
result_t
pal_traceroute (int argci, char **argvi, int *argco, char ***argvo, char *fib_id)
{
  int opt;
  int count = 20, cnt = 0;

  /* Reset optind. */
  optind = 1;

  /* Malloc size for argv. */
  *argvo = (char **) XMALLOC (MTYPE_TMP, count * sizeof (char *));

#ifdef HAVE_IPNET
  (*argvo)[cnt++] = "chvr";
  (*argvo)[cnt++] = "-n";
  (*argvo)[cnt++] = fib_id;
#endif


  /* Now fill up the options for output. */
  while (1)
    {
      opt = getopt (argci, argvi, "A:Z:s:n:w:q:f:m:p:");

      if (opt == EOF)
        break;

      switch (opt)
        {
        case 'A':
          /* ping program name. */
          if (! strcmp (optarg, TRACEROUTE_FAMILY_IPV4))
            (*argvo)[cnt++] = "traceroute";
          else if (! strcmp (optarg, TRACEROUTE_FAMILY_IPV6))
            (*argvo)[cnt++] = "traceroute6";
          break;
        case 'Z':
          (*argvo)[cnt++] = optarg; 
          break;
        case 's':
          /* Source address. */
          (*argvo)[cnt++] = "-s";
          (*argvo)[cnt++] = optarg;
          break;
        case 'n':
          /* Numeric display. */
          (*argvo)[cnt++] = "-n";
          break;
        case 'w':
          /* Timeout. */
          (*argvo)[cnt++] = "-w";
          (*argvo)[cnt++] = optarg;
          break;
        case 'q':
          /* Probe count. */
          (*argvo)[cnt++] = "-q";
          (*argvo)[cnt++] = optarg;
          break;
        case 'm':
          /* Max TTL. */
          (*argvo)[cnt++] = "-m";
          (*argvo)[cnt++] = optarg;
          break;
        case 'f':
          /* Min TTL. */
          (*argvo)[cnt++] = "-f";
          (*argvo)[cnt++] = optarg;
          break;    
        case 'p':
          /* Port number. */
          (*argvo)[cnt++] = "-p";
          (*argvo)[cnt++] = optarg;
          break;
        default:
          break;
        }
    }

  /* Nullify last element. */
  (*argvo)[cnt] = NULL;

  *argco = cnt;

  return 0;
}


