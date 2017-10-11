/* Copyright (C) 2008  All Rights Reserved. */

#ifndef _PACOS_NSM_BFD_API_H
#define _PACOS_NSM_BFD_API_H

#ifdef HAVE_BFD

int
nsm_ipv4_if_bfd_static_set (u_int32_t vr_id, char *ifname);
int
nsm_ipv4_if_bfd_static_unset (u_int32_t vr_id, char *ifname);
int
nsm_ipv4_if_bfd_static_disable_set (u_int32_t vr_id, char *ifname);
int
nsm_ipv4_if_bfd_static_disable_unset (u_int32_t vr_id, char *ifname);
int
nsm_ipv4_if_bfd_static_set_all (u_int32_t vr_id);
int
nsm_ipv4_if_bfd_static_unset_all (u_int32_t vr_id);
int
nsm_ipv4_route_bfd_set (u_int32_t vr_id, vrf_id_t vrf_id,
                        struct prefix_ipv4 *p, struct pal_in4_addr *nh);
int
nsm_ipv4_route_bfd_unset (u_int32_t vr_id, vrf_id_t vrf_id,
                          struct prefix_ipv4 *p, struct pal_in4_addr *nh);
int
nsm_ipv4_route_bfd_disable_set (u_int32_t vr_id, vrf_id_t vrf_id,
                        struct prefix_ipv4 *p, struct pal_in4_addr *nh);
int
nsm_ipv4_route_bfd_disable_unset (u_int32_t vr_id, vrf_id_t vrf_id,
                          struct prefix_ipv4 *p, struct pal_in4_addr *nh);

#endif /* HAVE_BFD */

#endif /* _PACOS_NSM_BFD_API_H */


