/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_VRF_H
#define _NSM_VRF_H

#include "vector.h"

/* NSM VRF AFI. */
struct nsm_vrf_afi
{
  /* Routing Information Base. */
  struct nsm_ptree_table *rib[SAFI_MAX];

  /* Static "ip route" configuration. */
  struct ptree *ip_static;

  /* Static "ip mroute" configuration. */
  struct ptree *ip_mroute;

  /* Nexthop registration. */
  struct nsm_ptree_table *nh_reg;

  /* Multicast Nexthop registration. */
  struct nsm_ptree_table *nh_reg_m;

  /* Redistribute information. */
  vector redist[APN_ROUTE_MAX];

  /* Counters. */
  struct
  {
    /* Counter per route type. */
    u_int32_t type[APN_ROUTE_MAX];

    /* Counter for FIB, excluding Kernel, Connect, and Static. */
    u_int32_t fib;

  } counter;

  /* RIB update timer. */
  struct thread *t_rib_update;

#ifdef HAVE_HA
  HA_CDR_REF nvrf_rib_upd_tmr_ref;
#endif /* HAVE_HA */
};

#define NSM_VRF_AFI_TO_AFI_VAL(pnvrf, pnvrf_afi)              \
  ARR_ELEM_TO_INDEX (struct nsm_vrf_afi, (pnvrf)->afi, *(pnvrf_afi))

#ifdef HAVE_BFD
  /* Address family */
  enum NSM_BFD_ADDR_FAMILY
    {
      NSM_BFD_ADDR_FAMILY_IPV4 = 0,
#ifdef HAVE_IPV6
      NSM_BFD_ADDR_FAMILY_IPV6,
#endif /* HAVE_IPV6 */
      NSM_BFD_ADDR_FAMILY_MAX
    };
#endif /* HAVE_BFD */

/* NSM VRF table. */
struct nsm_vrf
{
  /* NSM Master. */
  struct nsm_master *nm;

  /* APN VRF. */
  struct apn_vrf *vrf;

  /* Description. */
  char *desc;

  /* Per AFI structure. */
  struct nsm_vrf_afi afi[AFI_MAX];

  /* Configuration flags.  */
  u_char config;
#define NSM_VRF_CONFIG_ROUTER_ID                (1 << 0)
#ifdef HAVE_BFD
#define NSM_VRF_CONFIG_BFD_IPV4                 (1 << 1)
#define NSM_VRF_CONFIG_BFD_IPV6                 (1 << 2)
#endif /* HAVE_BFD */

  /* Configured Router ID.  */
  struct pal_in4_addr router_id_config;

  /* Type of Router-ID.  */
  u_char router_id_type;
#define NSM_ROUTER_ID_NONE                      0
#define NSM_ROUTER_ID_CONFIG                    1
#define NSM_ROUTER_ID_AUTOMATIC                 2
#define NSM_ROUTER_ID_AUTOMATIC_LOOPBACK        3

  /* Router ID update timer. */
  struct thread *t_router_id;

#ifdef HAVE_MCAST_IPV4
  struct nsm_mcast *mcast;
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  struct nsm_mcast6 *mcast6;
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_BFD
  struct avl_tree *nsm_bfd_session[NSM_BFD_ADDR_FAMILY_MAX];
#endif

#ifdef HAVE_HA
  HA_CDR_REF nvrf_cdr_ref;

  HA_CDR_REF nvrf_rtr_id_tmr_ref;
#endif /* HAVE_HA */
};

/* Macros. */
#ifdef HAVE_IPV6
#define NSM_AFI_LOOP(A)                                                       \
    for ((A) = AFI_IP; (A) < AFI_MAX; (A)++)                                  \
      if ((A) != AFI_IP6 || NSM_CAP_HAVE_IPV6)
#else /* HAVE_IPV6 */
#define NSM_AFI_LOOP(A)                                                       \
    for ((A) = AFI_IP; (A) < AFI_IP6; (A)++)
#endif /* HAVE_IPV6 */

#define NSM_VRF_NAMELEN_MAX     (64 + 5)

/* Prototypes. */
struct apn_vr;
struct nsm_master;

struct nsm_vrf *nsm_vrf_lookup_by_id (struct nsm_master *, vrf_id_t);
struct nsm_vrf *nsm_vrf_lookup_by_fib_id (struct lib_globals *, fib_id_t);
struct nsm_vrf *nsm_vrf_lookup_by_name (struct nsm_master *, char *);
struct nsm_vrf *nsm_vrf_new (struct apn_vr *, char *);
struct nsm_vrf *nsm_vrf_get_by_name (struct nsm_master *, char *);
int nsm_vrf_destroy (struct apn_vr *, struct nsm_vrf *);
fib_id_t nsm_fib_id_get (struct lib_globals *);

#ifdef HAVE_VRX
struct nsm_vrf *nsm_vrf_new_by_id (vrf_id_t);
#endif /* HAVE_VRX */

#endif /* _NSM_VRF_H */
