/* Copyright (C) 2004   All Rights Reserved.*/

#ifndef __LACP_NSM_H__
#define __LACP_NSM_H__


int lacp_nsm_aggregator_add (struct lacp_aggregator *);
int lacp_nsm_aggregator_delete (struct lacp_aggregator *);
int lacp_nsm_aggregator_link_add (struct lacp_aggregator *, 
                                  struct lacp_link **, u_int16_t);
int lacp_nsm_aggregator_link_del (struct lacp_aggregator *, 
                                  struct lacp_link **, u_int16_t);
int lacp_nsm_collecting_distributing (u_char *agg_name, u_int32_t ifindex,
                                      int enable);
int lacp_nsm_init ();
void lacp_nsm_deinit ();

#endif /* __LACP_NSM_H__ */
