/* Copyright (C) 2003  All Rights Reserved. */
#ifndef _PACOS_GVRP_DEBUG_H
#define _PACOS_GVRP_DEBUG_H

/* This module declares the interface to the debug logging. */

#include "log.h"

/* GVRP debug event flags. */
#define GVRP_DEBUG_EVENT   0x01
#define GVRP_DEBUG_CLI     0x02
#define GVRP_DEBUG_TIMER   0x04
#define GVRP_DEBUG_PACKET  0x08

extern unsigned long gvrp_debug;

/* Debug related macro. */
#define DEBUG_ON(b)       (gvrp_debug |= (GVRP_DEBUG_ ## b))
#define DEBUG_OFF(b)    (gvrp_debug &= ~(GVRP_DEBUG_ ## b))

#define GVRP_DEBUG(b)           (gvrp_debug & GVRP_DEBUG_ ## b) 

void gvrp_debug_init (void);
void gvrp_debug_reset (void);

#endif /* _PACOS_GVRP_DEBUG_H */
