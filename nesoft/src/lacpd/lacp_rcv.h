/* Copyright 2003,  All Rights Reserved. */

#ifndef __LACP_RCV_H__
#define __LACP_RCV_H__

#include "lacp_types.h"

char *rcv_state_str (enum lacp_rcv_state);

extern void 
lacp_rcv_sm (struct lacp_link * link);

extern void 
lacp_rcv_set_defaults (struct lacp_link * link);


#endif
