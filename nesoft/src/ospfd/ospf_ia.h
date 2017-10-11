/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_IA_H
#define _PACOS_OSPF_IA_H

typedef void (*OSPF_IA_CALC_FUNC) (struct ospf_area *, struct ospf_lsa *);

void ospf_ia_calc_route_clean (struct ospf_area *);
void ospf_ia_calc_event_add (struct ospf_area *);
void ospf_ia_calc_incremental_event_add (struct ospf_lsa *);
int ospf_ia_incr_counter_rst (struct thread *);

#endif /* _PACOS_OSPF_IA_H */
