/* Copyright (C) 2008  All Rights Reserved. */

#ifndef _PACOS_OSPF_BFD_API_H
#define _PACOS_OSPF_BFD_API_H

#ifdef HAVE_BFD

/* Interface mode.  */

/* Set the BFD fall-over check for neighbors on specificed interface.  */
int ospf_if_bfd_set (u_int32_t vr_id, char *ifname);

/* Unset the BFD fall-over check for neighbors on specificed interface.  */
int ospf_if_bfd_unset (u_int32_t vr_id, char *ifname);

/* Disable the BFD fall-over check for neighbors on specificed interface.  */
int ospf_if_bfd_disable_set (u_int32_t vr_id, char *ifname);

/* Unset the disable flag of BFD fall-over check for neighbors
   on specified interface.  */
int ospf_if_bfd_disable_unset (u_int32_t vr_id, char *ifname);

/* OSPF process mode.  */

/* Set the BFD fall-over check for all the neighbors
   under specified process. */
int ospf_bfd_all_interfaces_set (u_int32_t vr_id, int proc_id);

/* Unset the BFD fall-over check for all the neighbors
   under specified process. */
int ospf_bfd_all_interfaces_unset (u_int32_t vr_id, int proc_id);

/* Set the BFD fall-over check for the specified VLINK neighbor.  */
int ospf_vlink_bfd_set (u_int32_t vr_id, int proc_id,
                        struct pal_in4_addr area_id,
                        struct pal_in4_addr peer_id);

/* Unset the BFD fall-over check for the specified VLINK neighbor.  */
int ospf_vlink_bfd_unset (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr area_id,
                          struct pal_in4_addr peer_id);

#endif /* HAVE_BFD */

#endif /* _PACOS_OSPF_BFD_API_H */
