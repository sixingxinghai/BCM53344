/* Copyright (C) 2003,  All Rights Reserved. */
#ifndef __LACP_CONFIG_H__
#define __LACP_CONFIG_H__

/* This module contains the compile time configuration constants. */
#define LACP_DEFAULT_PORT_PRIORITY    0x8000
#define LACP_DEFAULT_SYSTEM_PRIORITY  0x8000

/* System integrators need to put their own version here. */
#define LACP_SYSTEM_VERSION    "APN LACP 1.0"


/* Select the number of significant hash bits used to index links */
#define LACP_PORT_HASH_BITS       (3)
#define LACP_PORT_HASH_SIZE       (1 << LACP_PORT_HASH_BITS)
#define LACP_PORT_HASH_MASK       (LACP_PORT_HASH_SIZE-1)

/* Maximum PDUs transmitted in fast periodic time - 43.4.16 */
#define LACP_TX_LIMIT             3
/* Size of PDUs transmitted/received (includes 14 byte frame header) */
#define LACPDU_FRAME_SIZE         128
/* Maximum number of aggregatable links per system */
#define LACP_MAX_LINKS            48
/* Maximum number of links per aggregator */
#define LACP_MAX_AGGREGATOR_LINKS 6
/* Limit ranges for various management variables. */
#define LACP_LINK_PRIORITY_MIN    0
#define LACP_LINK_PRIORITY_MAX    65535
#define LACP_LINK_ADMIN_KEY_MIN   0
#define LACP_LINK_ADMIN_KEY_MAX   65535
#define LACP_LINK_OPER_KEY_MIN    LACP_LINK_ADMIN_KEY_MIN
#define LACP_LINK_OPER_KEY_MAX    LACP_LINK_ADMIN_KEY_MAX

/* Define this if the hardware is capable of independent control
   of the collecting and distributing functions. */
#undef MUX_INDEPENDENT_CTRL

/* Define this if the system supports dynamic aggregator assignment, i.e.
   any aggregator may be assigned to any link and vice-versa. 
   Alternatively, links will only aggregate to aggregators that
   have the same key value as the link. The latter can be useful if
   there are system restrictions of which links can aggregate. */

#define LACP_DYNAMIC_AGG_ASSIGNMENT

#define LACP_AGG_COLLECTOR_MAX_DELAY_MIN   0
#define LACP_AGG_COLLECTOR_MAX_DELAY_MAX   65535

#endif
