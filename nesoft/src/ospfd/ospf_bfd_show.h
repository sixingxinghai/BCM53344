/* Copyright (C) 2009  All Rights Reserved. */

#ifndef _PACOS_OSPF_BFD_SHOW_H
#define _PACOS_OSPF_BFD_SHOW_H

#ifdef HAVE_BFD

void ospf_bfd_show_nbr_detail (struct cli *cli, struct ospf_neighbor *nbr);
void ospf_bfd_show_interface (struct cli *cli, struct ospf_interface *oi);
void ospf_bfd_show_instance (struct cli *cli, struct ospf *top);

#endif /* HAVE_BFD */

#endif /* _PACOS_OSPF_BFD_SHOW_H */
