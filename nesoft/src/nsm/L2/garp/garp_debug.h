/* Copyright (C) 2003  All Rights Reserved. */
#ifndef _PACOS_GARP_DEBUG_H
#define _PACOS_GARP_DEBUG_H

/* This module declares the interface to the debug logging. */

#include "log.h"

/* GARP debug event flags. */
#define GARP_DEBUG_EVENT   0x01
#define GARP_DEBUG_CLI     0x02
#define GARP_DEBUG_TIMER   0x04
#define GARP_DEBUG_PACKET  0x08

extern unsigned long garp_debug;

/* Debug related macro. */
#define GARP_DEBUG_ON(b)          (garp_debug |= (GARP_DEBUG_ ## b))
#define GARP_DEBUG_OFF(b)       (garp_debug &= ~(GARP_DEBUG_ ## b))

#define GARP_DEBUG(b)           (garp_debug & GARP_DEBUG_ ## b) 

void garp_debug_init (void);
void garp_debug_reset (void);

#endif /* _PACOS_GARP_DEBUG_H */
