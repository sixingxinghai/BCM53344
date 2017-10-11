/* Copyright (C) 2003  All Rights Reserved. */
#ifndef __LACP_TX_H__
#define __LACP_TX_H__

/* This modules defines the interface to the LACP TRANSMIT state machine */
/* lacp_tx.c */
int lacp_tx (struct lacp_link * link);
int marker_tx (struct lacp_link * link);

#endif
