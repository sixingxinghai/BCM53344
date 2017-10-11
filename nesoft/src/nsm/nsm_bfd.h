/* Copyright (C) 2008  All Rights Reserved. */

#ifndef _PACOS_NSM_BFD_H
#define _PACOS_NSM_BFD_H

#ifdef HAVE_BFD

bool_t
nsm_bfd_add_session (struct nsm_vrf *vrf, afi_t afi, union nsm_nexthop *nh,
                     struct interface *ifp, struct connected *ifc);
bool_t
nsm_bfd_delete_session (struct nsm_vrf *vrf, afi_t afi, union nsm_nexthop *nh,
                        struct interface *ifp, u_int32_t flags,
                        struct connected *ifc);
bool_t
nsm_bfd_update_session_by_interface (struct nsm_vrf *vrf, afi_t afi,
                               struct interface *ifp, struct connected *ifc,
                                                                    int del);
bool_t
nsm_bfd_update_interface (struct nsm_vrf *vrf, afi_t afi, struct interface *ifp, 
                                                 struct connected *ifc, int del);
bool_t
nsm_bfd_session_update (struct nsm_vrf *vrf, afi_t afi, union nsm_nexthop *gate, 
                        struct interface *ifp, struct connected *ifc,  int op);
bool_t
nsm_bfd_update_session_by_rib (struct nsm_vrf *vrf, struct rib *rib, afi_t afi,
                               struct connected *ifc, struct nsm_ptree_node *rn,
                                                   u_int32_t p_ifindex, int del);
bool_t
nsm_bfd_update_session_by_static (struct nsm_static *ns, struct nsm_vrf *vrf,
                                  struct prefix_ipv4 *p, int del );
s_int32_t
nsm_bfd_static_lookup (struct nsm_vrf *vrf, struct nsm_ptree_node *rn,
                       struct nexthop *nexthop);
s_int32_t
nsm_bfd_session_cmp (void *data1, void *data2);
void nsm_bfd_debug_set (struct nsm_master *nm);
void nsm_bfd_debug_unset (struct nsm_master *nm);
bool_t
nsm_bfd_static_config_exists(struct nsm_vrf *vrf, struct nsm_if *nif,
                             struct nsm_static *ns);
bool_t
nsm_bfd_rib_static_config_exists (struct nsm_vrf *vrf, struct nsm_if *nif,
                                  struct nsm_ptree_node *rn, struct nexthop *nexthop);
void
nsm_bfd_prefix_ipv4_set (struct prefix *pfx, struct pal_in4_addr *ipv4);

/**@brief  BFD static route session entries.
 */
struct nsm_bfd_static_session
{
  u_int32_t ifindex;
  u_int32_t count; /** Number of static routes using this session */
  afi_t afi; /** address family */
  union  /** Source and destination addresses */
  {
    struct
    {
      struct pal_in4_addr src;
      struct pal_in4_addr dst;
    } ipv4;
#ifdef HAVE_IPV6
    struct
    {
      struct pal_in6_addr src;
      struct pal_in6_addr dst;
    } ipv6;
#endif /* HAVE_IPV6 */
  } addr;

};

void nsm_bfd_init (struct lib_globals *zg);
void nsm_bfd_term (struct lib_globals *zg);

/**@brief  Flags to update session 
*/
enum session_update
  {
    session_add = 0,
    session_del,
    session_disable,
    session_no_disable
  };

enum
{
    NSM_BFD_STATIC_SESSION_INVALID_OP = 0,
    NSM_BFD_STATIC_SESSION_ADD,
    NSM_BFD_STATIC_SESSION_DEL,
    NSM_BFD_STATIC_SESSION_ADMIN_DEL
};

#endif /* HAVE_BFD */
#endif /* _PACOS_NSM_BFD_H */
