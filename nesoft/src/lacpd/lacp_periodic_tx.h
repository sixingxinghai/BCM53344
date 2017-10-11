/* Copyright (C) 2003  All Rights Reserved. */
#ifndef __LACP_PERIODIC_TX_H__
#define __LACP_PERIODIC_TX_H__

/* This module defines the interface to the PERIODIC TRANSMIT state machine. */

/* lacp_periodic_tx.c */
void lacp_periodic_tx_sm(struct lacp_link *link);
char *periodic_state_str (enum lacp_periodic_tx_state);

#endif
