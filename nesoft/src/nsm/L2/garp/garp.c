/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GARP_H
#define _PACOS_GARP_H

#include "pal.h"
#include "lib.h"
#include "message.h"

#include "garp.h"
#include "garp_sock.h"
#include "garp_debug.h"

/* Initialize GARP. */
int 
garp_init (struct lib_globals *zg)
{
  /* Initlialize socket. */
  garp_sock_init (zg);

  /* Initialize debugging. */
  garp_debug_init ();

  return 0;
}

/* Deinitialize GARP. */
int
garp_deinit (struct lib_globals *zg)
{

#ifndef HAVE_USER_HSL
  /* Deinitialize socket. */
  garp_sock_deinit (zg);

#endif /* HAVE_USER_HSL */

  return 0;
}

#endif /* _PACOS_GARP_H */
