/* Copyright (C) 2003  All Rights Reserved. 

    GMRP DEBUG Interface

*/

#include "pal.h"
#include "gvrp_debug.h"

/* Holds the debug state */
unsigned long gvrp_debug;

void
gvrp_debug_reset (void)
{
  DEBUG_OFF(EVENT);
  DEBUG_OFF(CLI);
  DEBUG_OFF(TIMER);
  DEBUG_OFF(PACKET);
}

void
gvrp_debug_init (void)
{
  gvrp_debug_reset();
}
