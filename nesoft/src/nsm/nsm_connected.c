/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "thread.h"
#include "prefix.h"
#include "linklist.h"
#include "if.h"
#include "log.h"
#include "nexthop.h"
#include "hash.h"
#include "lib_mtrace.h"

#ifdef HAVE_L3
#ifdef HAVE_MPLS
#include "mpls.h"
#endif /* HAVE_MPS */

#include "nsmd.h"
#include "nsm_interface.h"
#include "nsm_fea.h"
#include "rib/rib.h"
#include "rib/nsm_table.h"
#include "nsmd.h"
#include "nsm_server.h"
#include "rib/nsm_redistribute.h"
#include "nsm_router.h"

#ifdef HAVE_VRRP
#include "nsm/vrrp/vrrp.h"
#include "nsm/vrrp/vrrp_api.h"
#endif /* HAVE_VRRP */

#ifdef HAVE_MCAST_IPV4
#include "igmp.h"
#include "nsm_mcast4.h"
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#include "mld.h"
#include "nsm_mcast6.h"
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_HA
#include "lib_cal.h"
#include "nsm_cal.h"
#endif /* HAVE_HA */

/* If same interface address is already exist... XXX */
struct connected *
nsm_connected_check_ipv4 (struct interface *ifp, struct prefix *p)
{
  struct connected *ifc;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    if (prefix_same (ifc->address, p))
      return ifc;

  return NULL;
}

struct connected *
nsm_connected_real_ipv4 (struct interface *ifp, struct prefix *p)
{
  struct connected *ifc;

  ifc = nsm_connected_check_ipv4 (ifp, p);
  if (ifc != NULL && CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    return ifc;

  return NULL;
}

/* Called from if_up(). */
void
nsm_connected_up_ipv4 (struct interface *ifp, struct connected *ifc)
{
  if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    return;

  /* Check if its already active. */
  if (CHECK_FLAG (ifc->conf, NSM_IFC_ACTIVE))
    return;
  
#ifdef HAVE_CRX
  if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
    return;
#endif /* HAVE_CRX */

  /* Add the NSM RIB.  */
  nsm_rib_add_connected_ipv4 (ifp, ifc->address);

  /* Create a host route for the destination address if the destination
     address is configured on the point-to-point interface.  */
  if (if_is_pointopoint (ifp) && !if_is_tunnel (ifp))
    nsm_rib_add_connected_ipv4 (ifp, ifc->destination);

  /* Set address as active. */
  SET_FLAG (ifc->conf, NSM_IFC_ACTIVE);

#ifdef HAVE_HA
  lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */

}

/* Add connected IPv4 route to the interface. */
struct connected *
nsm_connected_add_ipv4 (struct interface *ifp, u_char flags,
                        struct pal_in4_addr *addr, u_char prefixlen,
                        struct pal_in4_addr *broad)
{
#ifdef HAVE_MCAST_IPV4
  struct nsm_vrf *nvrf;
#endif /* HAVE_MCAST_IPV4 */
  struct connected *ifc;
  struct prefix p;
  int secondary = 0;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Unnumbered interfaces shouldn't have connnected routes.  */
  if (if_is_ipv4_unnumbered (ifp))
    return NULL;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  /* Don't process invalid address (0.0.0.0) */
  if (addr->s_addr == 0 || prefixlen == 0)
    return NULL;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = prefixlen;
  p.u.prefix4 = *addr;

  /* Check if there is already the same address. */
  ifc = if_lookup_ifc_prefix (ifp, &p);
  if (ifc == NULL)
    {
      ifc = ifc_get_ipv4 (addr, prefixlen, ifp);
      ifc->flags = flags;
      if (ifp->ifc_ipv4)
        {
          SET_FLAG (ifc->flags, NSM_IFA_SECONDARY);
          secondary = 1;
        }

      /* Remove the IP address from the kernel.  */
      if (NSM_INTF_TYPE_L2 (ifp)
          || nsm_ipv4_check_address_overlap (ifp, &p, secondary))
        {
          nsm_fea_if_ipv4_address_delete (ifp, ifc);

          ifc_free (ifp->vr->zg, ifc);

          return NULL;
        }

      /* Add connected list. */
      NSM_IF_ADD_IFC_IPV4 (ifp, ifc);
    }

  /* If there is broadcast or pointopoint address. */
  if (broad != NULL)
    {
      if (ifc->destination)
        prefix_ipv4_free (ifc->destination);

      if (broad->s_addr == 0)
        ifc->destination = NULL;
      else
        {
          ifc->destination = (struct prefix *)prefix_ipv4_new ();
          ifc->destination->family = AF_INET;
          ifc->destination->u.prefix4 = *broad;
          if (if_is_pointopoint (ifc->ifp) && !if_is_tunnel (ifc->ifp))
            ifc->destination->prefixlen = IPV4_MAX_PREFIXLEN;
          else
            ifc->destination->prefixlen = prefixlen;
        }
    }

  /* Update interface address information to protocol daemon. */
  if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    {
      SET_FLAG (ifc->conf, NSM_IFC_REAL);

#ifdef HAVE_HA
      lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */

      nsm_server_if_address_add_update (ifp, ifc);

#ifdef HAVE_MCAST_IPV4
      /* Add Primary address to IGMP interface */
      if (! CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY)
          && ((nvrf = ifp->vrf->proto) && nvrf->mcast))
        nsm_mcast_igmp_if_addr_add (nvrf->mcast, ifp, ifc);
#endif /* HAVE_MCAST_IPV4 */

      /* Process router ID. */
      if (ifp->vrf->proto)
        ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);

      if (if_is_running (ifp))
      {
        nsm_connected_up_ipv4 (ifp, ifc);
        /* Updating the static ip routes related to this interface
         * in FIB database.
         * Handle case where IP address on a interface is
         * deleted and added back from kernel before the
         * NSM_RIB_UPDATE_DELAY timer expires. By doing this we
         * ensure that static routes are reinstalled into FIB.
         */
        nsm_rib_ipaddr_chng_ipv4_fib_update (
                             ((struct nsm_vrf *)(ifp->vrf->proto))->nm,
                             ifp->vrf->proto, ifc);
      }
    }

  return ifc;
}

void
nsm_connected_down_ipv4 (struct interface *ifp, struct connected *ifc)
{
  if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    return;

#ifdef HAVE_CRX
  if (CHECK_FLAG (ifc->flags, NSM_IFA_VIRTUAL))
    return;
#endif /* HAVE_CRX */

  /* Delete a host route for the destination address if the destination
     address is configured on the point-to-point interface. */
  if (if_is_pointopoint (ifp) && !if_is_tunnel (ifp))
    nsm_rib_delete_connected_ipv4 (ifp, ifc->destination);

  /* Delete the NSM RIB.  */
  nsm_rib_delete_connected_ipv4 (ifp, ifc->address);

  /* Unset active flag. */
  UNSET_FLAG (ifc->conf, NSM_IFC_ACTIVE);

#ifdef HAVE_HA
  lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */
}

/* Delete connected IPv4 route to the interface. */
void
nsm_connected_delete_ipv4 (struct interface *ifp, s_int32_t flags,
                           struct pal_in4_addr *addr, s_int32_t prefixlen,
                           struct pal_in4_addr *broad)
{
#ifdef HAVE_MCAST_IPV4
  struct nsm_vrf *nvrf;
#endif /* HAVE_MCAST_IPV4 */
  struct prefix_ipv4 p;
  struct connected *ifc;
  int primary = 0;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Unnumbered interfaces shouldn't have connnected routes.  */
  if (if_is_ipv4_unnumbered (ifp))
    return;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefix = *addr;
  p.prefixlen = prefixlen;

  ifc = nsm_connected_check_ipv4 (ifp, (struct prefix *) &p);
  if (ifc == NULL)
    return;

  NSM_IF_DELETE_IFC_IPV4 (ifp, ifc);

  /* Update interface address information to protocol daemon. */
  if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    {
      /* Don't process invalid address (0.0.0.0) */
      if (addr->s_addr)
        {
#ifdef HAVE_MCAST_IPV4
          /* Delete Primary address from IGMP interface */
          if (! CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY)
              && (nvrf = ifp->vrf->proto) && (nvrf->mcast))
            nsm_mcast_igmp_if_addr_del (nvrf->mcast, ifp, ifc);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_VRRP
          /* Update VRRP of IP address deletion. */
          if (NSM_CAP_HAVE_VRRP)
            vrrp_if_del_ipv4_addr (ifp, ifc);
#endif /* HAVE_VRRP */

          nsm_server_if_address_delete_update (ifp, ifc);

          nsm_connected_down_ipv4 (ifp, ifc);
          if (ifp->vrf->proto)
            ROUTER_ID_UPDATE_TIMER_ADD (ifp->vrf->proto);
        }

      UNSET_FLAG (ifc->conf, NSM_IFC_REAL);
    }

  if (!CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    primary = 1;

  ifc_free (ifp->vr->zg, ifc);

  if (primary)
    if (ifp->ifc_ipv4)
      {
        UNSET_FLAG (ifp->ifc_ipv4->flags, NSM_IFA_SECONDARY);
#ifdef HAVE_HA
        lib_cal_modify_connected (ifp->vr->zg, ifp->ifc_ipv4);
#endif /* HAVE_HA */
      }
}

#ifdef HAVE_IPV6
/* If same interface address is already exist... */
struct connected *
nsm_connected_check_ipv6 (struct interface *ifp, struct prefix *p)
{
  struct connected *ifc;
  
  for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
    if (prefix_same (ifc->address, p))
      return ifc;

  return NULL;
}

struct connected *
nsm_connected_real_ipv6 (struct interface *ifp, struct prefix *p)
{
  struct connected *ifc;

  ifc = nsm_connected_check_ipv6 (ifp, p);
  if (ifc != NULL && CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    return ifc;

  return NULL;
}

void
nsm_connected_up_ipv6 (struct interface *ifp, struct connected *ifc)
{
#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Unnumbered interfaces shouldn't have connnected routes.  */
  if (if_is_ipv6_unnumbered (ifp))
    return;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    return;

  /* Check if already active. */
  if (CHECK_FLAG (ifc->conf, NSM_IFC_ACTIVE))
    return;

  /* Add the NSM RIB.  */
  nsm_rib_add_connected_ipv6 (ifp, ifc->address);

  /* Create a host route for the destination address if the destination
     address is configured on the point-to-point interface.  */
  if (if_is_pointopoint (ifp) && !if_is_tunnel (ifp))
    nsm_rib_add_connected_ipv6 (ifp, ifc->destination);

  /* Set as active. */
  SET_FLAG (ifc->conf, NSM_IFC_ACTIVE);

#ifdef HAVE_HA
  lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */
}

void
nsm_connected_down_ipv6 (struct interface *ifp, struct connected *ifc)
{
  struct interface *ifp_alt = NULL;
  struct connected *ifc_alt = NULL;
  struct prefix p1;
  struct prefix p2;
  int flag = 0;

  pal_mem_set (&p1, 0, sizeof (struct prefix));
  pal_mem_set (&p2, 0, sizeof (struct prefix));
#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Unnumbered interfaces shouldn't have connnected routes.  */
  if (if_is_ipv6_unnumbered (ifp))
    return;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    return;

  /* Delete a host route for the destination address if the destination
     address is configured on the point-to-point interface.  */
  if (if_is_pointopoint (ifp) && !if_is_tunnel (ifp))
    nsm_rib_delete_connected_ipv6 (ifp, ifc->destination);

  if ((ifp_alt = ifp) != NULL)
    {
      prefix_copy (&p1, ifc->address);
      apply_mask (&p1);
      if (ifp_alt->ifc_ipv6 != NULL)
        {
          for (ifc_alt = ifp_alt->ifc_ipv6; ifc_alt;
               ifc_alt = ifc_alt->next)
            {
              /* Ignore the passed address */
              if (ifc_alt == ifc)
                continue;

              prefix_copy (&p2, ifc_alt->address);
              apply_mask (&p2);
              if (IPV6_ADDR_SAME (&p1,&p2))
                {
                  flag = 1;
                  break;
                }
            }
        }
      if (flag == 0 || ifp_alt->ifc_ipv6 == NULL)
        {
          /* Delete the NSM RIB */
          nsm_rib_delete_connected_ipv6 (ifp, ifc->address);
        }

      /* Unset active flag */
      UNSET_FLAG (ifc->conf, NSM_IFC_ACTIVE);
    }
#ifdef HAVE_HA
  lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */
}

void
nsm_connected_valid_ipv6 (struct interface *ifp, struct connected *ifc)
{
#ifdef HAVE_MCAST_IPV6
  struct nsm_vrf *nvrf;
#endif /* HAVE_MCAST_IPV6 */

  if (!CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    {
      SET_FLAG (ifc->conf, NSM_IFC_REAL);

#ifdef HAVE_HA
      lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */

      /* Notify to the PMs.  */
      nsm_server_if_address_add_update (ifp, ifc);

#ifdef HAVE_MCAST_IPV6
      /* Add Primary address to MLD interface */
      if ((nvrf = ifp->vrf->proto) && nvrf->mcast6)
        nsm_mcast6_mld_if_addr_add (nvrf->mcast6, ifp, ifc);
#endif /* HAVE_MCAST_IPV6 */

      if (if_is_running (ifp))
        nsm_connected_up_ipv6 (ifp, ifc);
    }
}

void
nsm_connected_invalid_ipv6 (struct interface *ifp, struct connected *ifc)
{
#ifdef HAVE_MCAST_IPV6
  struct nsm_vrf *nvrf;
#endif /* HAVE_MCAST_IPV6 */

  if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
    {
#ifdef HAVE_MCAST_IPV6
      /* Add Primary address to MLD interface */
      if ((nvrf = ifp->vrf->proto) && nvrf->mcast6)
        nsm_mcast6_mld_if_addr_del (nvrf->mcast6, ifp, ifc);
#endif /* HAVE_MCAST_IPV6 */

      /* Notify to the PMs.  */
      nsm_server_if_address_delete_update (ifp, ifc);

      nsm_connected_down_ipv6 (ifp, ifc);
      if (!CHECK_FLAG (ifc->conf, NSM_IFC_ACTIVE))
        UNSET_FLAG (ifc->conf, NSM_IFC_REAL);

#ifdef HAVE_HA
      lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */

    }
}

/* Add connected IPv6 route to the interface. */
struct connected *
nsm_connected_add_ipv6 (struct interface *ifp, struct pal_in6_addr *addr,
                        s_int32_t prefixlen, struct pal_in6_addr *broad)
{
  struct prefix p;
  struct connected *ifc;

  if (!ifp)
    return NULL;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Unnumbered interfaces shouldn't have connnected routes.  */
  if (if_is_ipv6_unnumbered (ifp))
    return NULL;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  /* Don't process invalid address. */
  if (IN6_IS_ADDR_UNSPECIFIED (addr) || prefixlen == 0)
    return NULL;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET6;
  p.prefixlen = prefixlen;
  p.u.prefix6 = *addr;

  /* Check if there is already the same address. */
  ifc = if_lookup_ifc_prefix (ifp, &p);
  if (ifc == NULL)
    {
      ifc = ifc_get_ipv6 (addr, prefixlen, ifp);

      /* Remove the IP address from the kernel.  */
      if (NSM_INTF_TYPE_L2 (ifp)
          || nsm_ipv6_check_address_overlap (ifp, &p))
        {
          nsm_fea_if_ipv6_address_delete (ifp, ifc);

          ifc_free (ifp->vr->zg, ifc);

          return NULL;
        }

      /* Add connected list. */
      NSM_IF_ADD_IFC_IPV6 (ifp, ifc);
    }

  /* If there is broadcast or pointopoint address. */
  if (broad != NULL)
    {
      if (ifc->destination)
        prefix_ipv6_free (ifc->destination);

      if (IN6_IS_ADDR_UNSPECIFIED (broad))
        ifc->destination = NULL;
      else
        {
          ifc->destination = (struct prefix *)prefix_ipv6_new ();
          ifc->destination->family = AF_INET6;
          ifc->destination->u.prefix6 = *broad;
          if (if_is_pointopoint (ifc->ifp) && !if_is_tunnel (ifc->ifp))
            ifc->destination->prefixlen = IPV6_MAX_PREFIXLEN;
          else
            ifc->destination->prefixlen = prefixlen;
        }
    }

  /* Notify to the PMs.  */
  nsm_connected_valid_ipv6 (ifp, ifc);

#ifdef HAVE_VRRP
  /* Update VRRP of IP address addtion. */
  if (NSM_CAP_HAVE_VRRP)
    vrrp_if_add_ipv6_addr (ifp, ifc);
#endif /* HAVE_VRRP */

  return ifc;
}

/* XXX */
void
nsm_connected_delete_ipv6 (struct interface *ifp, struct pal_in6_addr *address,
                           s_int32_t prefixlen, struct pal_in6_addr *broad)
{
  struct prefix_ipv6 p;
  struct connected *ifc;

#ifdef HAVE_NSM_IF_UNNUMBERED
  /* Unnumbered interfaces shouldn't have connnected routes.  */
  if (if_is_ipv6_unnumbered (ifp))
    return;
#endif /* HAVE_NSM_IF_UNNUMBERED */

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv6));
  p.family = AF_INET6;
  pal_mem_cpy (&p.prefix, address, sizeof (struct pal_in6_addr));
  p.prefixlen = prefixlen;

  ifc = nsm_connected_check_ipv6 (ifp, (struct prefix *) &p);
  if (! ifc)
    return;

  if (! CHECK_FLAG (ifc->conf, NSM_IFC_CONFIGURED))
    NSM_IF_DELETE_IFC_IPV6 (ifp, ifc);

#ifdef HAVE_VRRP
  /* Update VRRP of IP address deletion. */
  if (NSM_CAP_HAVE_VRRP)
    vrrp_if_del_ipv6_addr (ifp, ifc);
#endif /* HAVE_VRRP */

  /* Notify to the PMs.  */
  nsm_connected_invalid_ipv6 (ifp, ifc);

  if (! CHECK_FLAG (ifc->conf, NSM_IFC_CONFIGURED))
    ifc_free (ifp->vr->zg, ifc);
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */
