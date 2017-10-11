/* Copyright (C) 2003   All Rights Reserved. */
#include "pal.h"
#include "pal_ping.h"
#include "memory.h"

/* Map ping commands. */
result_t
pal_ping (int argci, char **argvi, int *argco, char ***argvo, char *fib_id)
{
  int opt;
  int count = 25, cnt = 0;

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
      opt = getopt (argci, argvi, "A:Z:c:s:w:I:Q:M:p:");

      if (opt == EOF)
        break;

      switch (opt)
        {
        case 'A':
          /* ping program name. */
          if (! strcmp (optarg, PING_FAMILY_IPV4))
            {
              (*argvo)[cnt++] = "ping";
#ifndef HAVE_IPNET
              /* Add broadcast flag.  */
              (*argvo)[cnt++] = "-b";
#endif /* HAVE_IPNET */
            }
          else if (! strcmp (optarg, PING_FAMILY_IPV6))
            (*argvo)[cnt++] = "ping6";
          break;
        case 'Z':
          (*argvo)[cnt++] = optarg; 
          break;
        case 'c':
          /* Repeat count. */
          (*argvo)[cnt++] = "-c";
          (*argvo)[cnt++] = optarg;
          break;
        case 's':
          /* Datagram size. */
          (*argvo)[cnt++] = "-s";
          (*argvo)[cnt++] = optarg;
          break;
        case 'w':
          /* Timeout. */
          (*argvo)[cnt++] = "-W";
          (*argvo)[cnt++] = optarg;
          break;
        case 'I':
          /* Interface. */
          (*argvo)[cnt++] = "-I";
          (*argvo)[cnt++] = optarg;
          break;
        case 'Q':
          /* TOS option. */
          (*argvo)[cnt++] = "-Q";
          (*argvo)[cnt++] = optarg;
          break;
        case 'M':
          (*argvo)[cnt++] = "-M";
          if (! strcmp (optarg, "do"))
            (*argvo)[cnt++] = "do";
          else if (! strcmp (optarg, "dont"))
            (*argvo)[cnt++] = "dont";
          else
            (*argvo)[cnt++] = "dont";
          break;    
        case 'p':
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

 /* Map ping commands. */
result_t
pal_mpls_oam (int argci, char *argvi, char **argv, int *argco, char ***argvo)
{
  int cnt = 0;
  
  /* mplsonm ping/trace mpls would be added to the string */
  argci+=3;

  /* Malloc size for argv. */
  *argvo = (char **) XMALLOC (MTYPE_TMP, (argci+1) * sizeof (char *));

  /* Now fill up the options for output. */

  (*argvo)[cnt++] = "mplsonm";
  (*argvo)[cnt++] = argvi;
  (*argvo)[cnt++] = "mpls";

  for (;cnt < argci; cnt++)
     (*argvo)[cnt] = argv[cnt-3];
  /* Nullify last element. */
  (*argvo)[cnt] = NULL;

  *argco = cnt;

  return 0;
}

