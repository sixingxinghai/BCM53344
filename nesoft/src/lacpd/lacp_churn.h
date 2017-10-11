/* Copyright (C) 2003  All Rights Reserved. */
#ifndef __LACP_CHURN_H__
#define __LACP_CHURN_H__

/* This module defines the interface to the CHURN state machines */

/* lacp_churn.c */
void lacp_churn_sm (struct lacp_link * link);
void lacp_mux_set_defaults (struct lacp_link * link);

#endif
