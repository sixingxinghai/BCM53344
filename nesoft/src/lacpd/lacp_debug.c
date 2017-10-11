/* Copyright (C) 2003  All Rights Reserved. 

    802.3 LACP DEBUG Interface

*/

#include "pal.h"

#include "lacp_debug.h"

/* Holds the debug state */
u_int32_t lacp_debug;


void
lacp_debug_reset (void)
{
  DEBUG_OFF(EVENT);
  DEBUG_OFF(CLI);
}

void
lacp_debug_init (void)
{
  lacp_debug_reset();
}
