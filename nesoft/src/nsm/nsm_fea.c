/* Copyright (C) 2004  All Rights Reserved. */

/*
  NSM - Forwarding plane APIs.
*/

#include "pal.h"
#include "lib.h"

#include "if.h"
#include "nexthop.h"
#include "nsmd.h"
#include "nsm_fea.h"
#ifdef HAVE_L3
#include "rib/rib.h"
#include "nsm_connected.h"
#endif /* HAVE_L3 */

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifdef HAVE_HA
#include "lib_cal.h"
#endif /* HAVE_HA */
/*
   NSM FEA init.
*/
int
nsm_fea_init (struct lib_globals *zg)
{
#ifdef ENABLE_HAL_PATH
  hal_nsm_cb_register();
#endif /*ENABLE_HAL_PATH */

#ifdef ENABLE_HAL_INIT
  hal_init (zg);
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_INIT
  pal_kernel_start ();
#endif /* ENABLE_PAL_PATH */
  return 0;
}

/*
   NSM FEA deinit.
*/
int
nsm_fea_deinit (struct lib_globals *zg)
{
#ifdef ENABLE_HAL_DEINIT
  hal_deinit (zg);
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_DEINIT
  pal_kernel_stop ();
#endif /* ENABLE_PAL_PATH */
  return 0;
}

/*
  Update the interface information from the forwarding layer.
*/
void
nsm_fea_if_update (void)
{
#ifdef ENABLE_PAL_PATH
  pal_kernel_if_update ();
  return;
#endif /* ENABLE_PAL_PATH */
}

/*
  Set flags for a interface.
*/
int
nsm_fea_if_flags_set (struct interface *ifp, u_int32_t flags)
{
#ifdef ENABLE_HAL_PATH
  hal_if_flags_set (ifp->name, ifp->ifindex, flags);
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_flags_set (ifp, flags);
#endif /* ENABLE_PAL_PATH */

#ifdef HAVE_HA
  lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */

  return 0;
}

/*
  Unset flags for a interface.
*/
int
nsm_fea_if_flags_unset (struct interface *ifp, u_int32_t flags)
{
#ifdef ENABLE_HAL_PATH
  hal_if_flags_unset (ifp->name, ifp->ifindex, flags);
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_flags_unset (ifp, flags);
#endif /* ENABLE_PAL_PATH */

#ifdef HAVE_HA
  lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */

  return 0;
}

/*
  Get flags for a interface.
*/
int
nsm_fea_if_flags_get (struct interface *ifp)
{
#ifdef ENABLE_PAL_PATH
  return pal_kernel_if_flags_get (ifp);
#elif defined (ENABLE_HAL_PATH)
  {
    u_int32_t flags;
    int ret;

    ret = hal_if_flags_get (ifp->name, ifp->ifindex, &flags);
    if (ret < 0)
      return ret;

    ifp->flags = flags;
  }
#endif /* ENABLE_PAL_PATH */

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */

  return 0;
}

/*--------------------------------------------------------------------
 * nsm_fea_if_set_primary_hwaddr - To swap physical<->logical hwaddr
 *
 * Params:
 *   ifp        - Interface object
 *   ma_tab     - Array of pointers to 2 MAC addresses
 *                0 - The replacement address (HSL only);
 *                1 - The virtual address if set (enforce into PAL)
 *   mac_addr_len-
 */

int
nsm_fea_if_set_primary_hwaddr(struct interface *ifp,
                              u_int8_t         *ma_tab[],
                              s_int32_t         mac_addr_len)
{
  int ret = 0;

#ifdef ENABLE_HAL_PATH
  /*  To fix Compilation warning for ERL 1.5 build 
   * the following line has been modified.
   * &ma_tab[0] is corrected to ma_tab[0]. */
  ret = hal_if_set_hwaddr (ifp->name, ifp->ifindex, ma_tab[0], mac_addr_len);

  /* Overwrite the PAL hwaddr in case HSL would alter it. */
  /* Until we are able to forward VRRP advert via SOCK_PACKET. */
  if (! ret && ma_tab[1])
    ret = pal_kernel_if_set_hwaddr (nzg, ifp, ma_tab[1], mac_addr_len);
#endif /* HAVE_HAL */
#ifdef ENABLE_PAL_PATH
  /* Nothing to do here. */
#endif /* HAVE_HAL */

  return ret;
}

/*--------------------------------------------------------------------
 * nsm_fea_if_set_virtual_hwaddr - Addition of virtual MAC addr
 *
 * Params:
 *   ifp        - Interface object
 *   mac_addr   - MAC addresses
 *   mac_addr_len-
 */
int
nsm_fea_if_set_virtual_hwaddr(struct interface *ifp,
                              u_int8_t *mac_addr,
                              s_int32_t mac_addr_len)
{
  int ret = 0;

#ifdef ENABLE_HAL_PATH
  ret = hal_if_sec_hwaddrs_add (ifp->name, ifp->ifindex,
                                mac_addr_len, 1, &mac_addr);
  /* Until we are able to forward VRRP advert via SOCK_PACKET. */
  if (! ret)
    ret = pal_kernel_if_set_hwaddr (nzg, ifp, mac_addr, mac_addr_len);
#endif /* HAVE_HAL */
#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_if_set_hwaddr (nzg, ifp, mac_addr, mac_addr_len);
#endif /* HAVE_HAL */
  return ret;
}


/*--------------------------------------------------------------------
 * nsm_fea_if_del_virtual_hwaddr - Deletion of virtual hw/addr
 *
 * Params:
 *   ifp        - Interface object
 *   mac_addr   - Array of pointers to 2 MAC addresses
 *                0 - The replacement address (PAL only);
 *                1 - The virtual address to be deleted (HSL)
 *   mac_addr_len-
 * Returns always 0.
 */
int
nsm_fea_if_del_virtual_hwaddr(struct interface *ifp,
                              u_int8_t  *ma_tab[],
                              s_int32_t  mac_addr_len)
{
  int ret = 0;

#ifdef ENABLE_HAL_PATH
  hal_if_sec_hwaddrs_delete (ifp->name, ifp->ifindex,
                             mac_addr_len, 1, &ma_tab[1]);
  pal_kernel_if_set_hwaddr (nzg, ifp, ma_tab[0], mac_addr_len);
#endif /* HAVE_HAL */
#ifdef ENABLE_PAL_PATH
  /* The current preferable primary address. */
  pal_kernel_if_set_hwaddr (nzg, ifp, ma_tab[0], mac_addr_len);
#endif /* HAVE_HAL */

  return ret;
}

/*
  Set MTU for a interface.
*/
int
nsm_fea_if_set_mtu (struct interface *ifp, int mtu)
{
  int ret = 0;
#ifdef ENABLE_HAL_PATH
  ret = hal_if_set_mtu (ifp->name, ifp->ifindex, mtu);
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_if_set_mtu (ifp, mtu);
#endif /* ENABLE_PAL_PATH */

  if ( ret < 0 )
    return ret;

#ifdef HAVE_HA
  lib_cal_modify_interface (nzg, ifp);
#endif /* HAVE_HA */

  return 0;
}

/*
  Get MTU for a interface.
*/
int
nsm_fea_if_get_mtu (struct interface *ifp, int *mtu)
{
  *mtu = ifp->mtu;
  return NSM_API_GET_SUCCESS;
}

int
nsm_if_proxy_arp_set (struct interface *ifp, int proxy_arp)
{
  int ret = 0;
  ret = pal_kernel_if_set_proxy_arp (ifp, proxy_arp);
  return ret;
}


/*
  Get ARP AGEING TIMEOUT for a interface.
*/
int
nsm_fea_if_get_arp_ageing_timeout (struct interface *ifp)
{
  int ret = 0;
#ifdef ENABLE_HAL_PATH
  int arp_ageing_timeout;

  ret = hal_if_get_arp_ageing_timeout (ifp->name, ifp->ifindex, &arp_ageing_timeout);
  if (ret < 0)
    return ret;

  ifp->arp_ageing_timeout = arp_ageing_timeout;
#endif  /* ENABLE_HAL_PATH */
  return ret;
}

/*
  Set ARP AGEING TIMEOUT for a interface.
*/
int
nsm_fea_if_set_arp_ageing_timeout (struct interface *ifp, int arp_ageing_timeout)
{
  int ret = 0;

#ifdef ENABLE_HAL_PATH
  ret = hal_if_set_arp_ageing_timeout (ifp->name, ifp->ifindex, arp_ageing_timeout);
#endif /* ENABLE_HAL_PATH */

  return ret;
}

/*
  Get duplex for a interface.
*/
int
nsm_fea_if_get_duplex (struct interface *ifp)
{
  int ret = 0;
  int duplex = 0;

  if (!ifp)
    return -1;

#ifdef ENABLE_HAL_PATH
  ret = hal_if_get_duplex (ifp->name, ifp->ifindex, &duplex);
  if (ret < 0)
    return ret;
#endif  /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_if_get_duplex (ifp, &duplex);
  if (ret < 0)
    return ret;
#endif /* ENABLE_PAL_PATH */

  ifp->duplex = duplex;

  return ret;
}

/*
  Set duplex for a interface.
*/
int
nsm_fea_if_set_duplex (struct interface *ifp, int duplex)
{
  int ret = 0;

  if (!ifp)
    return -1;

#ifdef ENABLE_HAL_PATH
  ret = hal_if_set_duplex (ifp->name, ifp->ifindex, duplex);
  if (ret < 0)
    return ret;
#endif  /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_if_set_duplex (ifp, duplex);
  if (ret < 0)
    return ret;
#endif /* ENABLE_PAL_PATH */

  return ret;
}

/*
  Set AUTO-NEGOTIATE for a interface.
*/
int
nsm_fea_if_set_autonego (struct interface *ifp, int autonego)
{
  int ret = 0;

#ifdef ENABLE_HAL_PATH
  ret = hal_if_set_autonego (ifp->name, ifp->ifindex, autonego);
#endif  /* ENABLE_HAL_PATH */
  return ret;
}

/*
  Get AUTO-NEGOTIATE for a interface.
*/
int
nsm_fea_if_get_autonego (struct interface *ifp, int *autonego)
{
  if (ifp->duplex == NSM_IF_AUTO_NEGO)
    *autonego = NSM_IF_AUTONEGO_ENABLE;
  else
    *autonego = NSM_IF_AUTONEGO_DISABLE;

  return NSM_API_GET_SUCCESS;
}

/*
  Get BANDWIDTH for a interface.
*/
int
nsm_fea_if_get_bandwidth (struct interface *ifp)
{
  int ret = 0;
#ifdef ENABLE_HAL_PATH
  float bandwidth;

  ret = hal_if_get_bw (ifp->name, ifp->ifindex, (u_int32_t *) &bandwidth);
  if (ret < 0)
    return ret;

  ifp->bandwidth = bandwidth;
#endif  /* ENABLE_HAL_PATH */

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */
  return ret;
}

/*
  Set BANDWIDTH for a interface.
*/
int
nsm_fea_if_set_bandwidth (struct interface *ifp, float bandwidth)
{
  int ret = 0;

#ifdef ENABLE_HAL_PATH
  ret = hal_if_set_bw (ifp->name, ifp->ifindex, bandwidth);
#endif  /* ENABLE_HAL_PATH */

#ifdef HAVE_HA
  lib_cal_modify_interface (NSM_ZG, ifp);
#endif /* HAVE_HA */
  return ret;
}


/*
  Get IPv4 forwarding enable flag.
*/
int
nsm_fea_ipv4_forwarding_get (int *ipforward)
{
#ifdef HAVE_L3
#ifdef ENABLE_PAL_PATH
  return pal_kernel_ipv4_forwarding_get (ipforward);
#else
  *ipforward = 1;
  return 0;
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Set IPv4 forwarding enable flag.
*/
int
nsm_fea_ipv4_forwarding_set (int ipforward)
{
#ifdef HAVE_L3
#ifdef ENABLE_PAL_PATH
  return pal_kernel_ipv4_forwarding_set (ipforward);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

#ifdef HAVE_IPV6
/*
  Get IPv6 forwarding enable flag.
*/
int
nsm_fea_ipv6_forwarding_get (int *ipforward)
{
#ifdef HAVE_L3
#ifdef ENABLE_PAL_PATH
  return pal_kernel_ipv6_forwarding_get (ipforward);
#else
  *ipforward = 1;
  return 0;
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
}

/*
  Set IPv6 forwarding enable flag.
*/
int
nsm_fea_ipv6_forwarding_set (int ipforward)
{
#ifdef HAVE_L3
#if defined ENABLE_PAL_PATH || defined ENABLE_HAL_PATH
  return pal_kernel_ipv6_forwarding_set (ipforward);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

#endif /* HAVE_IPV6 */

/*
  Add a IPv4 address to a interface.
*/
int
nsm_fea_if_ipv4_address_primary_add (struct interface *ifp,
                                     struct connected *ifc)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct prefix *p;
    int retifindex = ifp->ifindex;
    int ret = 0;

    p = ifc->address;
    if ((ifp->ifindex <= 0) &&
        (ifp->hw_type == IF_TYPE_VLAN))
      {
#ifdef HAVE_L2
        /* Create the SVI in hardware first */
        ret = hal_if_svi_create (ifp->name, &retifindex);
        if ((ret < 0) || (retifindex <= 0))
          return -1;
#endif /* HAVE_L2 */
      }
    ret = hal_if_ipv4_address_add (ifp->name, retifindex, &p->u.prefix4, p->prefixlen);
    if (ret < 0)
      return -1;
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv4_address_add (ifp, ifc);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Delete a IPv4 address to a interface.
*/
int
nsm_fea_if_ipv4_address_primary_delete (struct interface *ifp,
                                        struct connected *ifc)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct prefix *p;

    p = ifc->address;

    hal_if_ipv4_address_delete (ifp->name, ifp->ifindex, &p->u.prefix4, p->prefixlen);
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv4_address_delete (ifp, ifc);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Add secondary IPv4 address.
*/
int
nsm_fea_if_ipv4_address_secondary_add (struct interface *ifp,
                                       struct connected *ifc)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  nsm_fea_if_ipv4_address_primary_add (ifp, ifc);
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv4_address_secondary_add (ifp, ifc);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Delete secondary IPv4 address.
*/
int
nsm_fea_if_ipv4_address_secondary_delete (struct interface *ifp,
                                          struct connected *ifc)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  nsm_fea_if_ipv4_address_primary_delete (ifp, ifc);
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv4_address_secondary_delete (ifp, ifc);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

int
nsm_fea_if_ipv4_address_add (struct interface *ifp, struct connected *ifc)
{
  if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    return nsm_fea_if_ipv4_address_secondary_add (ifp, ifc);
  else
    return nsm_fea_if_ipv4_address_primary_add (ifp, ifc);
}

int
nsm_fea_if_ipv4_address_delete (struct interface *ifp, struct connected *ifc)
{
  if (CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
    return nsm_fea_if_ipv4_address_secondary_delete (ifp, ifc);
  else
    return nsm_fea_if_ipv4_address_primary_delete (ifp, ifc);
}

/*
  Delete all IPv4 address configured on a interface.
*/
int
nsm_fea_if_ipv4_address_delete_all (struct interface *ifp, struct connected *ifc)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct prefix *p;

    if (ifc != NULL)
      {
        if (ifc->next)
          nsm_fea_if_ipv4_address_delete_all (ifp, ifc->next);

        p = ifc->address;
        /* Skip the loopback address.  */
        if (!if_is_loopback (ifp))
          if (!IN_LOOPBACK (pal_ntoh32 (p->u.prefix4.s_addr)))
            hal_if_ipv4_address_delete (ifp->name, ifp->ifindex, &p->u.prefix4, p->prefixlen);
      }
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv4_address_delete_all (ifp, ifc);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Update IPv4 address on a interface.
*/
int
nsm_fea_if_ipv4_address_update (struct interface *ifp, struct connected *ifc_old,
                                struct connected *ifc_new)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct prefix *p;
    struct connected *ifc;

    /* Delete all addresses from interface. */
    nsm_fea_if_ipv4_address_delete_all (ifp, ifp->ifc_ipv4);
    p = ifc_old->address;
    hal_if_ipv4_address_delete (ifp->name, ifp->ifindex, &p->u.prefix4, p->prefixlen);

    /* Add all addresses to interface. */
    p = ifc_new->address;
    hal_if_ipv4_address_add (ifp->name, ifp->ifindex, &p->u.prefix4, p->prefixlen);
    for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
      {
        p = ifc->address;
        hal_if_ipv4_address_add (ifp->name, ifp->ifindex, &p->u.prefix4, p->prefixlen);
      }
    return 0;
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv4_address_update (ifp, ifc_old, ifc_new);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
/*
  Update IPv4 secondary address on a interface.
*/
int
nsm_fea_if_ipv4_secondary_address_update (struct interface *ifp,
                                          struct connected *ifc_old,
                                          struct connected *ifc_new)
{
  int ret = NSM_FALSE;
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct prefix *p;

    /* Delete old secondary address from interface. */
    p = ifc_old->address;
    ret = hal_if_ipv4_address_delete (ifp->name,
                                      ifp->ifindex,
                                      &p->u.prefix4,
                                      p->prefixlen);
    if (ret < NSM_FALSE)
      return ret;

    /* Add new secondary address to interface. */
    p = ifc_new->address;
    ret = hal_if_ipv4_address_add (ifp->name,
                                   ifp->ifindex,
                                   &p->u.prefix4,
                                   p->prefixlen);

    return ret;
  }

#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_if_ipv4_secondary_address_update (ifp, ifc_old, ifc_new);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return ret;
}
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

#ifdef HAVE_IPV6
/*
   Add IPV6 address on a interface.
*/
int
nsm_fea_if_ipv6_address_add (struct interface *ifp, struct connected *ifc)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct prefix *p;
    int retifindex = ifp->ifindex;
    int ret;

    p = ifc->address;
    if ((ifp->ifindex <= 0) &&
        (ifp->hw_type == IF_TYPE_VLAN))
      {
#ifdef HAVE_L2
        /* Create the SVI in hardware first */
        ret = hal_if_svi_create (ifp->name, &retifindex);
        if ((ret < 0) || (retifindex <= 0))
          return -1;
#endif /* HAVE_L2 */
      }
    ret = hal_if_ipv6_address_add (ifp->name, retifindex, &p->u.prefix6, p->prefixlen, ifc->flags);
    if (ret < 0)
      return -1;
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv6_address_add (ifp, ifc);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Delete IPv6 address from a interface.
*/
int
nsm_fea_if_ipv6_address_delete (struct interface *ifp, struct connected *ifc)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct prefix *p;

    p = ifc->address;

    hal_if_ipv6_address_delete (ifp->name, ifp->ifindex, &p->u.prefix6, p->prefixlen);
  }
#endif /* ENABLE_HAL_PATH */


#ifdef ENABLE_PAL_PATH
  pal_kernel_if_ipv6_address_delete (ifp, ifc);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}
#endif /* HAVE_IPV6 */

/*
  Bind a interface to a VR/VRF.
*/
int
nsm_fea_if_bind_vrf (struct interface *ifp, int fib_id)
{
  int ret = 0;
#ifdef HAVE_L3
#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_if_bind_vrf (ifp, fib_id);
#endif /* ENABLE_PAL_PATH */

#ifdef ENABLE_HAL_PATH
  ret = hal_if_bind_fib (ifp->ifindex, (u_int32_t)fib_id);
#endif /* ENABLE_HAL_PATH */
#endif /* HAVE_L3 */
  return ret;
}

/*
  Unbind a interface to a VR/VRF.
*/
int
nsm_fea_if_unbind_vrf (struct interface *ifp, int fib_id)
{
  int ret = 0;
#ifdef HAVE_L3

#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_if_unbind_vrf (ifp, fib_id);
#endif /* ENABLE_PAL_PATH */

#ifdef ENABLE_HAL_PATH
  ret = hal_if_unbind_fib (ifp->ifindex, (u_int32_t)fib_id);
#endif /* ENABLE_HAL_PATH */

#endif /* HAVE_L3 */
  return ret;
}

#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
/* Get nexthop IP and ifindex given a nexthop. */
static int
_nsm_fea_ipv4_nexthop_get (struct if_vr_master *ifm, struct nexthop *nexthop, struct hal_ipv4uc_nexthop *nh)
{
  struct pal_in4_addr nullIP;

  pal_mem_set (&nullIP, 0, sizeof (struct pal_in4_addr));
  pal_mem_set (nh, 0, sizeof (struct hal_ipv4uc_nexthop));

  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
    {
      if (nexthop->rtype == NEXTHOP_TYPE_IPV4
          || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX
          || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFNAME)
        pal_mem_cpy (&nh->nexthopIP, &nexthop->rgate.ipv4, sizeof (struct pal_in4_addr));

      if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
          || nexthop->rtype == NEXTHOP_TYPE_IFNAME
          || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFINDEX
          || nexthop->rtype == NEXTHOP_TYPE_IPV4_IFNAME)
        {
          nh->egressIfindex =  nexthop->rifindex;
          nh->egressIfname = if_index2name (ifm, nh->egressIfindex);
        }
      else
        {
          nh->egressIfindex = 0;
          nh->egressIfname = NULL;
        }
    }
  else
    {
      if (nexthop->type == NEXTHOP_TYPE_IPV4
          || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
          || nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME)
        pal_mem_cpy (&nh->nexthopIP, &nexthop->gate.ipv4, sizeof (struct pal_in4_addr));

      if (nexthop->type == NEXTHOP_TYPE_IFINDEX
          || nexthop->type == NEXTHOP_TYPE_IFNAME
          || nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
          || nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME)
        {
          nh->egressIfindex = nexthop->ifindex;
          nh->egressIfname = if_index2name (ifm, nh->egressIfindex);
        }
      else
        {
          nh->egressIfindex = 0;
          nh->egressIfname = NULL;
        }
    }

  if (! pal_mem_cmp (&nh->nexthopIP, &nullIP, sizeof (struct pal_in4_addr)) && (nh->egressIfindex))
    {
      nh->type = HAL_IPUC_LOCAL;
    }
  else
    {
      nh->type = HAL_IPUC_REMOTE;
    }

  return 0;
}

#ifdef HAVE_IPV6

/* Get nexthop IPV6 addr  and ifindex given a nexthop. */
static int
_nsm_fea_ipv6_nexthop_get (struct nexthop *nexthop, struct hal_ipv6uc_nexthop *nh)
{
  struct pal_in6_addr nullIP;

  pal_mem_set (&nullIP, 0, sizeof (struct pal_in6_addr));
  pal_mem_set (nh, 0, sizeof (struct hal_ipv6uc_nexthop));

  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
    {
      if (nexthop->rtype == NEXTHOP_TYPE_IPV6  || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX)
        pal_mem_cpy (&nh->nexthopIP, &nexthop->rgate.ipv6, sizeof (struct pal_in6_addr));

      if (nexthop->rtype == NEXTHOP_TYPE_IFINDEX
          || nexthop->rtype == NEXTHOP_TYPE_IFNAME
          || nexthop->rtype == NEXTHOP_TYPE_IPV6_IFINDEX)
        nh->egressIfindex =  nexthop->rifindex;
      else
        nh->egressIfindex = 0;
    }
  else
    {
      if ((nexthop->type == NEXTHOP_TYPE_IPV6)  ||
          (nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX) ||
          (nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME))
        pal_mem_cpy (&nh->nexthopIP, &nexthop->gate.ipv6, sizeof (struct pal_in6_addr));

      if ((nexthop->type == NEXTHOP_TYPE_IFINDEX)
          || (nexthop->type == NEXTHOP_TYPE_IFNAME)
          || (nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX)
          || (nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME))
        nh->egressIfindex = nexthop->ifindex;
      else
        nh->egressIfindex = 0;
    }

  if (! pal_mem_cmp (&nh->nexthopIP, &nullIP, sizeof (struct pal_in6_addr)) && (nh->egressIfindex))
    {
      nh->type = HAL_IPUC_LOCAL;
    }
  else
    {
      nh->type = HAL_IPUC_REMOTE;
    }

  return 0;
}

#endif /* HAVE_IPV6 */
#endif /* ENABLE_HAL_PATH */
#endif /* HAVE_L3 */

#ifdef HAVE_ORDER_NEXTHOP
/*
 * Compare function for pal_qsort.
 * Compares ip-addresses from nh array to make it
 * an array of ascending order of IP addresses
 */
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH

static int
nsm_comp_ascend_ipaddr (const void * entry1, const void *entry2)
{
  int	retval = 0;

  struct hal_ipv4uc_nexthop *nhop1 = (struct hal_ipv4uc_nexthop *) entry1;
  struct hal_ipv4uc_nexthop *nhop2 = (struct hal_ipv4uc_nexthop *) entry2;

 
  retval = (int) (nhop1->nexthopIP.s_addr - nhop2->nexthopIP.s_addr); 
  return (retval);
}

#endif /* ENABLE_HAL_PATH */
#endif /* HAVE_L3 */
#endif  /* ENABLE_ORDER_NEXTHOP */

#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
/*
 * Check if the nexthop ip-address is already added in the
 * nexthop-list to be added
 * Note: this function does not check validity of num value
 * or range. It is caller's responsibility to provide a valid
 * number - less than NSM max-path.
 */
static int
nsm_nexthop_already_added(struct nexthop *rhop,
   struct hal_ipv4uc_nexthop **nhp, int num)
{
 int i = 0;
 
 struct hal_ipv4uc_nexthop *nh = *nhp;

 for ( i = 0; i < num; i++)
   {
     if (! pal_mem_cmp (&(nh[i].nexthopIP), &rhop->gate.ipv4, 
       sizeof (struct pal_in4_addr)))
       {
	  return 1;
       }
   }
  return 0;
}
#endif  /* HAL_PATH */
#endif

/*
  Add a IPv4 route to the forwarding layer.
*/
int
nsm_fea_ipv4_add (struct prefix *p, struct rib *rib)
{
  int ret = 0;
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct nsm_master *nm = rib->vrf->vrf->vr->proto;
    struct if_vr_master *ifm = &rib->vrf->vrf->vr->ifm;
    struct hal_ipv4uc_nexthop *nh;
    fib_id_t table_id;
    struct nexthop *nexthop;
    int nhc = 0, count = 0, max_paths;

    for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
      nhc++;

    /* Find the maximum nh's to allocate. */
    if (nhc > nm->multipath_num)
      max_paths = nm->multipath_num;
    else
      max_paths = nhc;

    /* Allocate temp memory. */
    nh = XMALLOC (MTYPE_TMP, max_paths * sizeof (struct hal_ipv4uc_nexthop));
    if (! nh)
      return -1;

    /* Get table id. */
    if (rib->vrf)
      table_id = rib->vrf->vrf->fib_id;
    else
      table_id = 0;

    /* Populate nexthops. */
    for (nexthop = rib->nexthop; nexthop;
         nexthop = nexthop->next)
    {
      if ((rib->nexthop_active_num == 1 || nm->multipath_num == 1
           || nexthop->rrn == NULL) && (count < max_paths))
        {
          pal_mem_set (&nh[count], 0, sizeof (struct hal_ipv4uc_nexthop));

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
            {
              /* Get nexthop information. */
              _nsm_fea_ipv4_nexthop_get (ifm, nexthop, &nh[count]);

              if (rib->flags & RIB_FLAG_BLACKHOLE)
                nh[count].type = HAL_IPUC_BLACKHOLE;

              if (nexthop->flags & NEXTHOP_FLAG_RECURSIVE_BLACKHOLE)
                nh[count].type = HAL_IPUC_BLACKHOLE;

              count++;
            }
        }
      else if (nexthop->rrn != NULL && (count < max_paths) &&
	  CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
        {
	   struct rib *rrib;
	   struct nexthop *rhop;

	   /* BGP multipath has recursive IGP paths */

           for (rrib= nexthop->rrn->info; rrib; rrib = rrib->next)
	       if (CHECK_FLAG (rrib->flags, RIB_FLAG_SELECTED))
		  break;

           if (rrib == NULL)
	     continue;
	   for (rhop = rrib->nexthop; rhop; rhop = rhop->next)
	     {
	       if (nsm_nexthop_already_added(rhop, &nh, count))
		 continue;
               pal_mem_set (&nh[count], 0, sizeof (struct hal_ipv4uc_nexthop));

	       if (CHECK_FLAG(rhop->flags, NEXTHOP_FLAG_ACTIVE))
		 {
		   _nsm_fea_ipv4_nexthop_get (ifm, rhop, &nh[count]);
	 	   
                   if (rrib->flags & RIB_FLAG_BLACKHOLE)
		     nh[count].type = HAL_IPUC_BLACKHOLE;

		   if (rhop->flags & NEXTHOP_FLAG_RECURSIVE_BLACKHOLE)
		     nh[count].type = HAL_IPUC_BLACKHOLE;
		     count++;
		 } 
             }
        }
    }

#ifdef HAVE_ORDER_NEXTHOP
   /* Sort the nh list in ascending order of ip-address */
   pal_qsort(nh, count, sizeof (struct hal_ipv4uc_nexthop), nsm_comp_ascend_ipaddr);
#endif  /* ENABLE_ORDER_NEXTHOP */

    /* Add the route. */
    ret = hal_ipv4_uc_route_add (table_id, &p->u.prefix4, p->prefixlen,  count, nh);
    if (ret == 0)
      {
        count = 0;
        for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next, ++count)
          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE) && (count < max_paths))
             SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
      }

    /* Free temp buffer. */
    XFREE (MTYPE_TMP, nh);
    return 0;
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_ipv4_add (p, rib);
#endif /* HAVE_HAL */
#endif /* HAVE_L3 */
  return ret;
}

/*
  Delete a IPv4 route to the forwarding layer.
*/
int
nsm_fea_ipv4_delete (struct prefix *p, struct rib *rib)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct nsm_master *nm = rib->vrf->vrf->vr->proto;
    struct if_vr_master *ifm = &rib->vrf->vrf->vr->ifm;
    struct hal_ipv4uc_nexthop *nh;
    fib_id_t table_id;
    struct nexthop *nexthop;
    int ret;
    int nhc = 0, count = 0, max_paths;

    for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
      nhc++;

    /* Find the maximum nh's to allocate. */
    if (nhc > nm->multipath_num)
      max_paths = nm->multipath_num;
    else
      max_paths = nhc;

    /* Allocate temp memory. */
    nh = XMALLOC (MTYPE_TMP, max_paths * sizeof (struct hal_ipv4uc_nexthop));
    if (! nh)
      return -1;

    /* Get table id. */
    if (rib->vrf)
      table_id = rib->vrf->vrf->fib_id;
    else
      table_id = 0;

    /* Populate nexthops. */
    for (nexthop = rib->nexthop; nexthop;
         nexthop = nexthop->next)
      if (count < max_paths)
        {
          pal_mem_set (&nh[count], 0, sizeof (struct hal_ipv4uc_nexthop));

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
            {
              /* Get nexthop information. */
              _nsm_fea_ipv4_nexthop_get (ifm, nexthop, &nh[count]);

              /* Added the second condition as in nsm_static_uninstall() we are
                 unsetting the RIB_FLAG_BLACKHOLE.*/
              if ((rib->flags & RIB_FLAG_BLACKHOLE)
                   || (nexthop->ifname &&
                       IS_NULL_INTERFACE_STR (nexthop->ifname)))
                nh[count].type = HAL_IPUC_BLACKHOLE;

              if (nexthop->flags & NEXTHOP_FLAG_RECURSIVE_BLACKHOLE)
                nh[count].type = HAL_IPUC_BLACKHOLE;

              count++;
            }
        }

    /* Delete the route. */
    ret = hal_ipv4_uc_route_delete (table_id, &p->u.prefix4, p->prefixlen,  count, nh);
    if (ret == 0)
      {
        count = 0;
        for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next, ++count)
          if(CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) && (count < max_paths))
            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
      }

    /* Free temp buffer. */
    XFREE (MTYPE_TMP, nh);
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_ipv4_del (p, rib);
#endif /* HAVE_HAL */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Update a IPv4 route with the new nexthop.
*/
int
nsm_fea_ipv4_update (struct prefix *p, struct rib *fib, struct rib *select)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    /* Delete the previous one. */
    nsm_fea_ipv4_delete (p, fib);

    /* Add the new one. */
    nsm_fea_ipv4_add (p, select);
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_ipv4_update (p, fib, select);
#endif /* HAVE_HAL */
#endif /* HAVE_L3 */
  return 0;
}

#ifdef HAVE_IPV6

/*
  Add a IPv6 route to the forwarding layer.
*/
int
nsm_fea_ipv6_add (struct prefix *p, struct rib *rib)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct nsm_master *nm = rib->vrf->vrf->vr->proto;
    struct hal_ipv6uc_nexthop *nh;
    fib_id_t table_id;
    struct nexthop *nexthop;
    int ret;
    int nhc = 0, count = 0, max_paths;

    for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
      nhc++;

    /* Find the maximum nh's to allocate. */
    if (nhc > nm->multipath_num)
      max_paths = nm->multipath_num;
    else
      max_paths = nhc;

    /* Allocate temp memory. */
    nh = XMALLOC (MTYPE_TMP, max_paths * sizeof (struct hal_ipv6uc_nexthop));
    if (! nh)
      return -1;

    /* Get table id. */
    if (rib->vrf)
      table_id = rib->vrf->vrf->fib_id;
    else
      table_id = 0;

    /* Populate nexthops. */
    for (nexthop = rib->nexthop; nexthop;
         nexthop = nexthop->next)
      if (count < max_paths)
        {
          pal_mem_set (&nh[count], 0, sizeof (struct hal_ipv6uc_nexthop));

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
            {
              /* Get nexthop information. */
              _nsm_fea_ipv6_nexthop_get (nexthop, &nh[count]);

              if (rib->flags & RIB_FLAG_BLACKHOLE)
                nh[count].type = HAL_IPUC_BLACKHOLE;

              if (nexthop->flags & NEXTHOP_FLAG_RECURSIVE_BLACKHOLE)
                 nh[count].type = HAL_IPUC_BLACKHOLE;

              count++;
            }
        }

    /* Add the route. */
    ret = hal_ipv6_uc_route_add (table_id, &p->u.prefix6, p->prefixlen,  count, nh);
    if (ret == 0)
      {
        count = 0;
        for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next, ++count)
          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE) && (count < max_paths))
             SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
      }

    /* Free temp buffer. */
    XFREE (MTYPE_TMP, nh);
  }
#endif /* ENABLE_HAL_PATH */
#ifdef ENABLE_PAL_PATH
  pal_kernel_ipv6_add (p, rib);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Delete a IPv6 route to the forwarding layer.
*/
int
nsm_fea_ipv6_delete (struct prefix *p, struct rib *rib)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    struct nsm_master *nm = rib->vrf->vrf->vr->proto;
    struct hal_ipv6uc_nexthop *nh;
    fib_id_t table_id;
    struct nexthop *nexthop;
    int ret;
    int nhc = 0, count = 0, max_paths;

    for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
      nhc++;

    /* Find the maximum nh's to allocate. */
    if (nhc > nm->multipath_num)
      max_paths = nm->multipath_num;
    else
      max_paths = nhc;

    /* Allocate temp memory. */
    nh = XMALLOC (MTYPE_TMP, max_paths * sizeof (struct hal_ipv6uc_nexthop));
    if (! nh)
      return -1;

    /* Get table id. */
    if (rib->vrf)
      table_id = rib->vrf->vrf->fib_id;
    else
      table_id = 0;

    /* Populate nexthops. */
    for (nexthop = rib->nexthop; nexthop;
         nexthop = nexthop->next)
      if (count < max_paths)
        {
          pal_mem_set (&nh[count], 0, sizeof (struct hal_ipv6uc_nexthop));

          if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
            {
              /* Get nexthop information. */
              _nsm_fea_ipv6_nexthop_get (nexthop, &nh[count]);

              /* Added the second condition as in nsm_static_uninstall() we are
                 unsetting the RIB_FLAG_BLACKHOLE.*/
              if ((rib->flags & RIB_FLAG_BLACKHOLE)
                   || (nexthop->ifname &&
                       IS_NULL_INTERFACE_STR (nexthop->ifname)))
                nh[count].type = HAL_IPUC_BLACKHOLE;

              if (nexthop->flags & NEXTHOP_FLAG_RECURSIVE_BLACKHOLE)
                nh[count].type = HAL_IPUC_BLACKHOLE;

              count++;
            }
        }

    /* Delete the route. */
    ret = hal_ipv6_uc_route_delete (table_id, &p->u.prefix6, p->prefixlen,  nhc, nh);
    if (ret == 0)
      {
        count = 0;
        for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next, ++count)
          if(CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB) && (count < max_paths))
            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
      }

    /* Free temp buffer. */
    XFREE (MTYPE_TMP, nh);
  }
#endif /* ENABLE_HAL_PATH */
#ifdef ENABLE_PAL_PATH
  pal_kernel_ipv6_del (p, rib);
#endif /* ENABLE_PAL_PATH */
#endif /* HAVE_L3 */
  return 0;
}

/*
  Update a IPv6 route with the new nexthop.
*/
int
nsm_fea_ipv6_update (struct prefix *p, struct rib *fib, struct rib *select)
{
#ifdef HAVE_L3
#ifdef ENABLE_HAL_PATH
  {
    /* Delete the previous one. */
    nsm_fea_ipv6_delete (p, fib);

    /* Add the new one. */
    nsm_fea_ipv6_add (p, select);
  }
#endif /* ENABLE_HAL_PATH */

#ifdef ENABLE_PAL_PATH
  pal_kernel_ipv6_update (p, fib, select);
#endif /* HAVE_HAL */
#endif /* HAVE_L3 */
  return 0;
}
#endif /* HAVE_IPV6 */

/*
   Create a FIB.
*/
int
nsm_fea_fib_create (int fib_id, bool_t new_vr, char *fib_name)
{
  int ret = 0;
#ifdef HAVE_L3
#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_fib_create (fib_id, new_vr, fib_name);
  if (ret < 0)
    return ret;
#endif /* ENABLE_PAL_PATH */

#ifdef ENABLE_HAL_PATH
  PAL_UNREFERENCED_PARAMETER (new_vr);
  PAL_UNREFERENCED_PARAMETER (fib_name);
  ret = hal_fib_create ((u_int32_t)fib_id);
#endif /* ENABLE_HAL_PATH */
#endif /* HAVE_L3 */

  return ret;
}

/*
  Delete a FIB.
*/
int
nsm_fea_fib_delete (int fib_id)
{
  int ret = 0;

#ifdef HAVE_L3
#ifdef ENABLE_PAL_PATH
  ret = pal_kernel_fib_delete (fib_id);
  if (ret < 0)
    return ret;
#endif /* ENABLE_PAL_PATH */

#ifdef ENABLE_HAL_PATH
  ret = hal_fib_delete ((u_int32_t)fib_id);
#endif /* ENABLE_HAL_PATH */
#endif /* HAVE_L3 */

  return ret;
}

#ifdef HAVE_VRRP
/*
   Send a gratuitous ARP on a interface.
*/
int
nsm_fea_gratuitous_arp_send (struct lib_globals *zg, struct stream *ap, struct interface *ifp)
{
  pal_kernel_gratuitous_arp_send (zg, ap, ifp);
  return 0;
}

#ifdef HAVE_IPV6
int
nsm_fea_nd_neighbor_adv_send (struct lib_globals *zg, struct stream *ap,
                              struct interface *ifp)
{
#ifdef ENABLE_PAL_PATH
  return pal_kernel_nd_nbadvt_send (zg, ap, ifp);
#endif /* ENABLE_PAL_PATH */
  return 0;
}
#endif /* HAVE_IPV6 */


/*
  Start VRRP.
*/
int
nsm_fea_vrrp_start (struct lib_globals *zg)
{
  pal_kernel_vrrp_start (zg);
  return 0;
}

/*
  Stop VRRP.
*/
int
nsm_fea_vrrp_stop (struct lib_globals *zg)
{
  pal_kernel_vrrp_stop (zg);
  return 0;
}

/*
  Add a virtual IP address to a interface.
*/
int
nsm_fea_vrrp_virtual_ipv4_add (struct lib_globals *zg,
                               struct pal_in4_addr *vip,
                               struct interface *ifp,
                               bool_t owner,
                               u_int8_t vrid)
{
  pal_kernel_virtual_ipv4_add (zg, vip, ifp, owner, vrid);
  return 0;
}

/*
  Delete a virtual IP address from a interface.
*/
int
nsm_fea_vrrp_virtual_ipv4_delete (struct lib_globals *zg,
                                  struct pal_in4_addr *vip,
                                  struct interface *ifp,
                                  bool_t owner,
                                  u_int8_t vrid)
{
  pal_kernel_virtual_ipv4_delete (zg, vip, ifp, owner, vrid);
  return 0;
}

/*
  Add a virtual IPv6 address to a interface.
*/
#ifdef HAVE_IPV6
int
nsm_fea_vrrp_virtual_ipv6_add (struct lib_globals *zg,
                               struct pal_in6_addr *vip,
                               struct interface *ifp,
                               bool_t owner,
                               u_int8_t vrid)
{
  pal_kernel_virtual_ipv6_add (zg, vip, ifp, owner, vrid);

  return 0;
}


int
nsm_fea_vrrp_virtual_ipv6_delete (struct lib_globals *zg,
                                  struct pal_in6_addr *vip,
                                  struct interface *ifp,
                                  bool_t owner,
                                  u_int8_t vrid)
{
  pal_kernel_virtual_ipv6_delete (zg, vip, ifp, owner, vrid);

  return 0;
}
#endif /* HAVE_IPV6 */
/*
Get  vmac status
*/
int
nsm_fea_vrrp_get_vmac_status (struct lib_globals *zg)
{
  return pal_kernel_get_vmac_status (zg);
}

/*
Set vmac Status
*/
int
nsm_fea_vrrp_set_vmac_status (struct lib_globals *zg, int status)
{
  return pal_kernel_set_vmac_status (zg, status);
}
#endif /* HAVE_VRRP */

