/* Copyright (C) 2003  All Rights Reserved. */
#ifndef __LACP_FUNCTIONS_H__
#define __LACP_FUNCTIONS_H__

/* lacp_functions.c */
void lacp_record_pdu (struct lacp_link * link);
void lacp_record_default (struct lacp_link * link);
void lacp_update_selected (struct lacp_link * link);
void lacp_update_default_selected (struct lacp_link * link);
void lacp_update_ntt (struct lacp_link * link);

#endif
