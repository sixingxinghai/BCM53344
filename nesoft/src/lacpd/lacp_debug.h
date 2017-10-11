/* Copyright (C) 2003  All Rights Reserved. */
#ifndef _PACOS_LACP_DEBUG_H
#define _PACOS_LACP_DEBUG_H

/* This module declares the interface to the debug logging. */

#include "log.h"

/* LACP debug event flags. */
#define LACP_DEBUG_EVENT        0x01
#define LACP_DEBUG_CLI          0x02
#define LACP_DEBUG_TIMER        0x04
#define LACP_DEBUG_PACKET       0x08
#define LACP_DEBUG_TIMER_DETAIL 0x10
#define LACP_DEBUG_SYNC         0x20
#define LACP_DEBUG_HA           0x40

extern u_int32_t lacp_debug;

/* Debug related macro. */
#define DEBUG_ON(b)       (lacp_debug |= (LACP_DEBUG_ ## b))
#define DEBUG_OFF(b)    (lacp_debug &= ~(LACP_DEBUG_ ## b))

#define LACP_DEBUG(b)           (lacp_debug & LACP_DEBUG_ ## b) 

void lacp_debug_init (void);
void lacp_debug_reset (void);

#endif /* _PACOS_LACP_DEBUG_H */
