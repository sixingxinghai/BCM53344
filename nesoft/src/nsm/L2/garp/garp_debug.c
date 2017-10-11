/* Copyright (C) 2003  All Rights Reserved. 

    GARP DEBUG Interface

*/

#include "pal.h"

#include "garp_debug.h"

/* Holds the debug state */
unsigned long garp_debug;

void
garp_debug_reset (void)
{
  GARP_DEBUG_OFF(EVENT);
  GARP_DEBUG_OFF(CLI);
  GARP_DEBUG_OFF(TIMER);
}

void
garp_debug_init (void)
{
  garp_debug_reset();
  /* GARP_DEBUG_ON(EVENT); */
  /* GARP_DEBUG_ON(CLI); */
  /* GARP_DEBUG_ON(TIMER); */
  /* GARP_DEBUG_ON(PACKET); */
}
