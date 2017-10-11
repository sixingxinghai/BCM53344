/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_ASE_H
#define _PACOS_OSPF_ASE_H

void ospf_ase_calc_incremental (struct ospf *, struct ospf_lsa *);
void ospf_ase_calc_nssa_route_clean (struct ospf_area *);
void ospf_ase_calc_route_clean (struct ospf *);
void ospf_ase_calc_event_add (struct ospf *);

#endif /* _PACOS_OSPF_ASE_H */
