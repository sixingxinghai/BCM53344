/* Copyright (C) 2003,  All Rights Reserved. */
#ifndef __LACP_MUX_H__
#define __LACP_MUX_H__

/* This module declares the interface to the MUX state machine. */
/* lacp_mux.c */
void lacp_mux_sm (struct lacp_link * link);
void lacp_mux_set_defaults (struct lacp_link * link);
char *mux_state_str (enum lacp_mux_state);

#endif
