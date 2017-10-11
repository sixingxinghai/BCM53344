/* Copyright (C) 2003  All Rights Reserved. 

    GMRP DEBUG Interface

*/

#include "pal.h"
#include "gmrp_debug.h"

/* Holds the debug state */
unsigned long gmrp_debug;

void
gmrp_debug_reset (void)
{
  DEBUG_OFF(EVENT);
  DEBUG_OFF(CLI);
}

void
gmrp_debug_init (void)
{
  gmrp_debug_reset();
}
