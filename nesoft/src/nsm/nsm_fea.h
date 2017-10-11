/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _NSM_FEA_H_
#define _NSM_FEA_H_

/* NSM API General error code. */
#define NSM_API_GET_FAIL                                 -1
#define NSM_API_GET_SUCCESS                               1

/*
  NSM - Forwarding plane APIs.
*/
int nsm_fea_init (struct lib_globals *zg);
int nsm_fea_deinit (struct lib_globals *zg);
void nsm_fea_if_update (void);
int nsm_fea_if_flags_set (struct interface *ifp, u_int32_t flags);
int nsm_fea_if_flags_unset (struct interface *ifp, u_int32_t flags);
int nsm_fea_if_flags_get (struct interface *ifp);
int nsm_fea_if_set_mtu (struct interface *ifp, int mtu);
int nsm_fea_if_get_mtu (struct interface *ifp, int *mtu);
int nsm_fea_if_get_duplex (struct interface *ifp);
int nsm_fea_if_set_duplex (struct interface *ifp, int duplex);
int nsm_fea_if_set_autonego (struct interface *ifp, int autonego);
int nsm_fea_if_get_autonego (struct interface *ifp, int *autonego);
int nsm_fea_if_get_bandwidth (struct interface *ifp);
int nsm_fea_if_set_bandwidth (struct interface *ifp, float bandwidth);
int nsm_fea_if_set_primary_hwaddr(struct interface *ifp,
                                  u_int8_t *ma_tab[],
                                  s_int32_t mac_addr_len);
int nsm_fea_if_set_virtual_hwaddr(struct interface *ifp, u_int8_t *mac_addr,
                                  s_int32_t mac_addr_len);
int nsm_fea_if_del_virtual_hwaddr(struct interface *ifp, u_int8_t *ma_tab[],
                                  s_int32_t mac_addr_len);

int nsm_fea_ipv4_forwarding_get (int *ipforward);
int nsm_fea_ipv4_forwarding_set (int ipforward);
int nsm_if_proxy_arp_set (struct interface *ifp, int proxy_arp);
int nsm_fea_if_set_arp_ageing_timeout (struct interface *ifp, int arp_ageing_timeout);
#ifdef HAVE_IPV6
int nsm_fea_ipv6_forwarding_get (int *ipforward);
int nsm_fea_ipv6_forwarding_set (int ipforward);
#endif /* HAVE_IPV6 */
int nsm_fea_if_ipv4_address_add (struct interface *ifp, struct connected *ifc);
int nsm_fea_if_ipv4_address_delete (struct interface *ifp, struct connected *ifc);
int nsm_fea_if_ipv4_address_delete_all (struct interface *ifp, struct connected *ifc);
int nsm_fea_if_ipv4_address_update (struct interface *ifp, struct connected *ifc_old,
                                    struct connected *ifc_new);
#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
int nsm_fea_if_ipv4_secondary_address_update (struct interface *ifp,
                                              struct connected *ifc_old,
                                              struct connected *ifc_new);
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

#ifdef HAVE_IPV6
int nsm_fea_if_ipv6_address_add (struct interface *ifp, struct connected *ifc);
int nsm_fea_if_ipv6_address_delete (struct interface *ifp, struct connected *ifc);
#endif /* HAVE_IPV6 */
int nsm_fea_if_bind_vrf (struct interface *ifp, int fib_id);
int nsm_fea_if_unbind_vrf (struct interface *ifp, int fib_id);
int nsm_fea_ipv4_add (struct prefix *p, struct rib *rib);
int nsm_fea_ipv4_delete (struct prefix *p, struct rib *rib);
int nsm_fea_ipv4_update (struct prefix *p, struct rib *fib, struct rib *select);
#ifdef HAVE_IPV6
int nsm_fea_ipv6_add (struct prefix *p, struct rib *rib);
int nsm_fea_ipv6_delete (struct prefix *p, struct rib *rib);
int nsm_fea_ipv6_update (struct prefix *p, struct rib *fib, struct rib *select);
#endif /* HAVE_IPV6 */
int nsm_fea_fib_create (int fib_id, bool_t new_vr, char *fib_name);
int nsm_fea_fib_delete (int fib_id);
#ifdef HAVE_VRRP
int nsm_fea_gratuitous_arp_send (struct lib_globals *zg, struct stream *ap, struct interface *ifp);
int nsm_fea_nd_neighbor_adv_send (struct lib_globals *zg, struct stream *ap,
                                  struct interface *ifp);
int nsm_fea_vrrp_start (struct lib_globals *zg);
int nsm_fea_vrrp_stop (struct lib_globals *zg);
int nsm_fea_vrrp_virtual_ipv4_add (struct lib_globals *zg, struct pal_in4_addr *vip, struct interface *ifp, bool_t owner, u_int8_t vrid);
int nsm_fea_vrrp_virtual_ipv4_delete (struct lib_globals *zg, struct pal_in4_addr *vip, struct interface *ifp, bool_t owner, u_int8_t vrid);
#ifdef HAVE_IPV6
int nsm_fea_vrrp_virtual_ipv6_add (struct lib_globals *zg,
                                   struct pal_in6_addr *vip,
                                   struct interface *ifp,
                                   bool_t owner, u_int8_t vrid);
int nsm_fea_vrrp_virtual_ipv6_delete (struct lib_globals *zg,
                                      struct pal_in6_addr *vip,
                                      struct interface *ifp,
                                      bool_t owner, u_int8_t vrid);
#endif /* HAVE_IPV6 */
int
nsm_fea_vrrp_get_vmac_status (struct lib_globals *zg);
int
nsm_fea_vrrp_set_vmac_status (struct lib_globals *zg, int status);
#endif /* HAVE_VRRP */

#endif /* _NSM_FEA_H */
