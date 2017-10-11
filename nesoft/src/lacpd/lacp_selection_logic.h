/* Copyright (C) 2003-2004  All Rights Reserved. */
#ifndef __LACP_SELECTION_LOGIC_H__
#define __LACP_SELECTION_LOGIC_H__

/* This module declares the interface to the AGGREGATOR MANAGEMENT
   and the SELECTION LOGIC. */

#define CALC_LINK_PRIORITY(LINK)                                              \
      (((LINK)->actor_port_priority << LACP_PORT_PRIORITY_SHIFTS) | ((LINK)->actor_port_number))
      

/* lacp_selection_logic.c */
extern int lacp_initialize_aggregator (struct lacp_aggregator *agg, 
                                       const unsigned char *const name, 
                                       const unsigned char *const mac_addr,
                                       const int ifindex, const unsigned short key,
                                       const unsigned int individual);

extern struct lacp_aggregator *lacp_add_aggregator (char *name, u_char *mac_addr, int ifindex, u_int16_t key, u_int32_t individual);

extern int lacp_delete_aggregator (char *name);

extern struct lacp_aggregator * lacp_find_aggregator (int id);
extern struct lacp_aggregator * lacp_find_aggregator_by_name (const char * const name);

extern unsigned int lacp_port_identifier (struct lacp_link *link);
extern unsigned int lacp_partner_port_identifier (struct lacp_link *link);

extern void lacp_selection_logic ();
extern void lacp_ready_logic ();

extern struct lacp_aggregator ** lacp_get_aggregator_list ();

extern void lacp_update_selection_logic ();
int lacp_selection_logic_add_link (struct lacp_link *);
void lacp_aggregator_detach_links (struct lacp_aggregator *, bool_t);
void lacp_aggregator_delete_process (struct lacp_aggregator *, bool_t);
void lacp_aggregator_delete_all (bool_t);
struct lacp_aggregator *lacp_search_for_aggregator (struct lacp_link *);
void unlink_aggregator (struct lacp_aggregator *);
extern int lacp_is_partner_system_NULL(u_char *);
#endif
