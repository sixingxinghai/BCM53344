/* Copyright (C) 2001, 2002  All Rights Reserved. */

#ifndef _PACOS_DIFFSERV_H
#define _PACOS_DIFFSERV_H

#define DSCPBITS(i) i>>5&0x01,i>>4&0x01,i>>3&0x01,i>>2&0x01,i>>1&0x01,i&0x01

/* Maximum DSCP to EXP bit mappings. */
#define DIFFSERV_MAX_DSCP_EXP_MAPPINGS   8

/* Best Effor class. */
#define DIFFSERV_BEST_EFFORT             0

/* Support / non-support. */
#define DIFFSERV_DSCP_NOT_SUPPORTED      0
#define DIFFSERV_DSCP_SUPPORTED          1

/* Type of DSCP. */
#define DIFFSERV_DSCP_UNCONFIGURED       0
#define DIFFSERV_DSCP_CONFIGURED         1

/* Prototypes. */
int diffserv_class_configurable (int);
/* End Prototypes. */

#endif /* _PACOS_DIFFSERV_H */
