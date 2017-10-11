/* Copyright (C) 2008  All Rights Reserved. */

#ifndef _PACOS_OSPF_BFD_H
#define _PACOS_OSPF_BFD_H

#define OSPF_BFD_DEFAULT_DELAY 5

void ospf_bfd_update_session (struct ospf_neighbor *nbr);
void ospf_bfd_if_update (struct ospf_interface *oi, bool_t admin_flag);
void ospf_bfd_update (struct ospf *top, bool_t admin_flag);
void ospf_bfd_init (struct lib_globals *zg);
void ospf_bfd_term (struct lib_globals *zg);
void ospf_bfd_debug_set (struct ospf_master *om);
void ospf_bfd_debug_unset (struct ospf_master *om);

#endif /* _PACOS_OSPF_BFD_H */
