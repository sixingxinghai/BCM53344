/* Copyright (C) 2003  All Rights Reserved. 

    STP DEBUG Interface

*/

#include "pal.h"
#include "lib.h"
#include "l2_debug.h"

/* For debug statement. */
u_int32_t conf_layer2_debug_proto;
u_int32_t term_layer2_debug_proto;

void
l2_debug_reset (void)
{
  term_layer2_debug_proto = conf_layer2_debug_proto = 0;
}

void
l2_debug_init (void)
{
  l2_debug_reset();
}
