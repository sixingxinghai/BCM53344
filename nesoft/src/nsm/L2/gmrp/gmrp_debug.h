/* Copyright (C) 2003  All Rights Reserved. */
#ifndef _PACOS_GMRP_DEBUG_H
#define _PACOS_GMRP_DEBUG_H

/* This module declares the interface to the debug logging. */

#include "log.h"

/* GMRP debug event flags. */
#define GMRP_DEBUG_EVENT   0x01
#define GMRP_DEBUG_CLI     0x02
#define GMRP_DEBUG_TIMER   0x04
#define GMRP_DEBUG_PACKET  0x08

extern unsigned long gmrp_debug;

/* Debug related macro. */
#define DEBUG_ON(b)       (gmrp_debug |= (GMRP_DEBUG_ ## b))

#define DEBUG_OFF(b)      (gmrp_debug &= ~(GMRP_DEBUG_ ## b))

#define GMRP_DEBUG(b)   (gmrp_debug & GMRP_DEBUG_ ## b)

void gmrp_debug_init (void);
void gmrp_debug_reset (void);

#endif /* _PACOS_GMRP_DEBUG_H */
