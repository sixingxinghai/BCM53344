/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_NETWORK_H
#define _PACOS_OSPF_NETWORK_H

/* Prototypes. */
int ospf_if_join_allspfrouters (struct ospf_interface *);
int ospf_if_leave_allspfrouters (struct ospf_interface *);
int ospf_if_join_alldrouters (struct ospf_interface *);
int ospf_if_leave_alldrouters (struct ospf_interface *);
int ospf_if_ipmulticast (struct ospf_interface *);
void ospf_sock_init (struct ospf *);
void ospf_sock_reset (struct ospf *);

#endif /* _PACOS_OSPF_NETWORK_H */
