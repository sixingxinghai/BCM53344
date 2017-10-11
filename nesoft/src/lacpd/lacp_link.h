/* Copyright (C) 2003  All Rights Reserved */
#ifndef __LACP_LINK_H__
#define __LACP_LINK_H__

#include "lacp_config.h"

/* This module decalres the interface to the LACP link management. */

/* lacp_link.c */
extern struct lacp_link *lacp_find_link (u_int32_t port_num);
extern struct lacp_link *lacp_find_link_by_name (const char *const name);
extern int lacp_add_link (char *name, struct lacp_link **);
void
unlink_link (struct lacp_link *link);

extern int lacp_delete_link (struct lacp_link *, bool_t);
extern int  lacp_set_link_priority (struct lacp_link *, const unsigned int priority);
extern int  lacp_set_link_timeout (struct lacp_link *, int);
extern int  lacp_set_link_admin_key (struct lacp_link *, const unsigned int key);

extern void lacp_attempt_to_aggregate_link (struct lacp_link * link);
extern void lacp_update_tx_credits ();
extern void lacp_aggregate_link (struct lacp_aggregator * agg, struct lacp_link * link);
extern void lacp_disaggregate_link (struct lacp_link *link, bool_t);
extern int begin ();
extern int attach_mux_to_aggregator (struct lacp_link * link);
extern int detach_mux_from_aggregator (struct lacp_link *link, bool_t);
extern unsigned short lacp_get_link_admin_key (struct lacp_link * link);
result_t compare_link_priorities (const void * l1, const void * l2);
extern int lacp_generate_sorted_links (struct lacp_link *[], bool_t);
extern struct lacp_aggregator *lacp_aggregator_lookup_by_key (u_int16_t);
extern struct lacp_aggregator *lacp_search_for_aggregator (struct lacp_link *);
extern void unlink_aggregator (struct lacp_aggregator *);
#if defined (MUX_INDEPENDENT_CTRL)
extern void enable_collecting (struct lacp_link * link);
extern void disable_collecting (struct lacp_link * link);
extern void enable_distributing (struct lacp_link * link);
extern void disable_distributing (struct lacp_link * link);
#else
extern void enable_collecting_distributing (struct lacp_link * link);
extern void disable_collecting_distributing (struct lacp_link * link);
#endif

int lacp_link_port_attach (struct lacp_link *, struct interface *);
int lacp_link_port_detach (struct lacp_link *, bool_t);
void lacp_link_delete_all (bool_t);
int lacp_initialize_link (struct lacp_link *);
void link_cleanup (struct lacp_link *, bool_t);
int begin ();
void lacp_link_port_detach_all (bool_t);
#endif /* __LACP_LINK_H__ */
