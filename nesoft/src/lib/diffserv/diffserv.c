/* Copyright (C) 2001, 2002  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_DIFFSERV

#include "diffserv.h"

/* Is a DSCP CONFIGURED or not. */
int diffserv_class_conf[DIFFSERV_MAX_SUPPORTED_DSCP] =
{   
  /* DSCP Type. */                     /* Class name.         */
  DIFFSERV_DSCP_CONFIGURED,            /* "be"                */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_000001"  */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_000010"  */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_000011", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_000100", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_000101", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_000110", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_000111", */
  DIFFSERV_DSCP_CONFIGURED,            /* "cs1",              */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_001001", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af11",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_001011", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af12",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_001101", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af13",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_001111", */
  DIFFSERV_DSCP_CONFIGURED,            /* "cs2",              */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_010001", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af21",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_010011", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af22",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_010101", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af23",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_010111", */
  DIFFSERV_DSCP_CONFIGURED,            /* "cs3",              */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_011001", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af31",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_011011", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af32",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_011101", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af33",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_011111", */
  DIFFSERV_DSCP_CONFIGURED,            /* "cs4",              */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_100001", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af41",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_100011", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af42",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_100101", */
  DIFFSERV_DSCP_CONFIGURED,            /* "af43",             */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_100111", */
  DIFFSERV_DSCP_CONFIGURED,            /* "cs5",              */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_101001", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_101010", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_101011", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_101100", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_101101", */
  DIFFSERV_DSCP_CONFIGURED,            /* "ef",               */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_101111", */
  DIFFSERV_DSCP_CONFIGURED,            /* "cs6",              */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_110001", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_110010", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_110011", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_110100", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_110101", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_110110", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_110111", */
  DIFFSERV_DSCP_CONFIGURED,            /* "cs7",              */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_111001", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_111010", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_111011", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_111100", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_111101", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_111110", */
  DIFFSERV_DSCP_UNCONFIGURED,          /* "undefined_111111" */
};

/* Definition status for a DSCP. */
int
diffserv_class_configurable (int dscp_value)
{
  if (diffserv_class_conf [dscp_value] == DIFFSERV_DSCP_CONFIGURED)
    return PAL_TRUE;

  return PAL_FALSE;
} 
#endif /* HAVE_DIFFSERV */
