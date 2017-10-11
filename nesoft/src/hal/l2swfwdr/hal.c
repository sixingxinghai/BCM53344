/* Copyright (C) 2004  All Rights Reserved.
  
LAYER 2 BRIDGE HAL
  
This module defines the platform abstraction layer to the 
Linux hal.

*/

#include "pal.h"
#include <errno.h>
#include "hal_incl.h"

static pal_sock_handle_t hal_sock = -1;
/* Generic ioctl handler */
int
hal_ioctl (unsigned long arg0, unsigned long arg1, unsigned long arg2,
           unsigned long arg3, unsigned long arg4, unsigned long arg5,
           unsigned long arg6)
{
  unsigned long arg[7];

  arg[0] = arg0;
  arg[1] = arg1;
  arg[2] = arg2;
  arg[3] = arg3;
  arg[4] = arg4;
  arg[5] = arg5;
  arg[6] = arg6;
/*  printf ("hal_sock %d cmd %d\n", hal_sock, SIOCPROTOPRIVATE);  */

  return ioctl (hal_sock, SIOCPROTOPRIVATE, arg);
}

/* Stubs for hal linux hal_init */
int
hal_init (struct lib_globals *zg)
{
  int errnum;
  
  hal_sock = pal_sock (zg, AF_HAL, SOCK_RAW, 0);
  errnum = errno;
  
  if (hal_sock < 0)
    {
      return -errno;
    }
  return hal_sock;
}
                                                                          
int
hal_deinit (struct lib_globals *zg)
{
  return pal_sock_close(zg, hal_sock);
}

