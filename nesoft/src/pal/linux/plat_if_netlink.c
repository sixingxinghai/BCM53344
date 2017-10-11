/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include <net/if_arp.h>
#include <linux/ethtool.h>

#include "pal_kernel.h"

#include "log.h"
#include "linklist.h"
#include "if.h"
#include "prefix.h"
#ifdef HAVE_VRF
#include "vty.h"
#include "table.h"
#endif /* HAVE_VRF */

#include "nsm/nsmd.h"
#ifdef HAVE_L3
#include "nsm/rib/rib.h"
#include "nsm/nsm_rt.h"
#include "nsm/nsm_connected.h"
#endif /* HAVE_L3 */

#ifndef SO_VR
#define SO_VR 0x1020
#endif /* SO_VR */

int netlink_update_interface_all (void);
#ifdef HAVE_L3
/* Extern from rt_netlink.c */
void netlink_update_ipv4_address_set (void);
void netlink_update_ipv6_address_set (void);
int kernel_address_add_ipv4 (struct interface *, struct connected *);
int kernel_address_delete_ipv4 (struct interface *, struct connected *);
int kernel_address_add_ipv6 (struct interface *, struct connected *);
int kernel_address_delete_ipv6 (struct interface *, struct connected *);
#endif /* HAVE_L3 */
int get_link_status_using_mii (char *);

/* Interface information read by netlink. */
result_t
pal_kernel_if_scan (void)
{
#ifdef HAVE_L3
  netlink_update_ipv4_address_set ();
#ifdef HAVE_IPV6
  netlink_update_ipv6_address_set ();
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

  netlink_update_interface_all ();
  return RESULT_OK;
}

void
pal_kernel_if_update (void)
{
  netlink_update_interface_all ();
}

/* clear and set interface name string */
void
ifreq_set_name (struct ifreq *ifreq, struct interface *ifp)
{
  strncpy (ifreq->ifr_name, if_kernel_name (ifp), IFNAMSIZ);
}

/* call ioctl system call */
int
if_ioctl (int request, caddr_t buffer)
{
  int sock;
  int ret = 0;
  int err = 0;

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      return -1;
    }

  ret = ioctl (sock, request, buffer);
  if (ret < 0)
    {
      err = errno;
    }
  close (sock);

  if (ret < 0)
    {
      errno = err;
      return ret;
    }
  return 0;
}

#ifdef HAVE_IPNET
/* Call IPNET VR ioctl system call */
int
if_vr_ioctl (int request, caddr_t buffer, unsigned long fib_id)
{
  int sock;
  int ret = 0;
  int err = 0;

  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      return -1;
    }

  if (setsockopt(sock, IP_SOL_SOCKET, IP_SO_X_VR, &fib_id, sizeof(fib_id)) < 0)
    {
        perror("setsockopt(SO_VR) failed");
        return -1;
    }

  ret = ioctl (sock, request, buffer);
  if (ret < 0)
    {
      err = errno;
    }
  close (sock);

  if (ret < 0)
    {
      errno = err;
      return ret;
    }
  return 0;
}
#endif /* HAVE_IPNET */

#ifdef HAVE_L3
result_t
pal_kernel_if_ipv4_address_add (struct interface *ifp,
                                struct connected *ifc)
{
  int ret;

  ret = kernel_address_add_ipv4 (ifp, ifc);
  if (ret < 0)
    return -1;

  return RESULT_OK;
}

result_t
pal_kernel_if_ipv4_address_delete (struct interface *ifp,
                                   struct connected *ifc)
{
  int ret;

  ret = kernel_address_delete_ipv4 (ifp, ifc);
  if (ret < 0)
    return -1;

  return RESULT_OK;
}

result_t
pal_kernel_if_ipv4_address_delete_all (struct interface *ifp,
                                       struct connected *ifc)
{
  if (ifc != NULL)
    {
      if (ifc->next)
        pal_kernel_if_ipv4_address_delete_all (ifp, ifc->next);

      if (!IN_LOOPBACK (ntohl (ifc->address->u.prefix4.s_addr)))
        kernel_address_delete_ipv4 (ifp, ifc);
    }

  return RESULT_OK;
}

result_t
pal_kernel_if_ipv4_address_update (struct interface *ifp,
                                   struct connected *ifc_old,
                                   struct connected *ifc_new)
{
  struct connected *ifc;

  /* Delete all addresses from Interface. */
  pal_kernel_if_ipv4_address_delete_all (ifp, ifp->ifc_ipv4);
  kernel_address_delete_ipv4 (ifp, ifc_old);

  /* Add all addresses to Interface. */
  kernel_address_add_ipv4 (ifp, ifc_new);
  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    kernel_address_add_ipv4 (ifp, ifc);

  return RESULT_OK;
}

#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
result_t
pal_kernel_if_ipv4_secondary_address_update (struct interface *ifp,
                                             struct connected *ifc_old,
                                             struct connected *ifc_new)
{
  result_t ret = RESULT_OK;
  /* Delete secondary address from Interface. */
  ret = kernel_address_delete_ipv4 (ifp, ifc_old);
  if (ret != RESULT_OK)
    return ret;

  /* Add secondary address to Interface. */
  ret = kernel_address_add_ipv4 (ifp, ifc_new);
  return ret;
}
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

result_t
pal_kernel_if_ipv4_address_secondary_add (struct interface *ifp,
                                          struct connected *ifc)
{
  int ret;

  ret = kernel_address_add_ipv4 (ifp, ifc);
  if (ret < 0)
    return -1;

  return RESULT_OK;
}

result_t
pal_kernel_if_ipv4_address_secondary_delete (struct interface *ifp,
                                             struct connected *ifc)
{
  int ret;

  ret = kernel_address_delete_ipv4 (ifp, ifc);
  if (ret < 0)
    return -1;

  return RESULT_OK;
}
#endif /* HAVE_L3 */

/* get interface flags */
result_t
pal_kernel_if_flags_get_fib (struct interface *ifp, fib_id_t fib_id)
{
  int ret;
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

#ifndef HAVE_IPNET
  (void)fib_id;
  ret = if_ioctl (SIOCGIFFLAGS, (caddr_t) &ifreq);
#else
  ret = if_vr_ioctl (IP_SIOCGIFFLAGS, (caddr_t) &ifreq, fib_id);
#endif /* HAVE_IPNET */
  if (ret < 0) /* Don't move this line. */
    return ret;

  ifp->flags = ifreq.ifr_flags & 0x0000ffff;

#if defined (SIOCGMIIPHY) && defined (SIOCGMIIREG)
  if (ifp->hw_type == IF_TYPE_ETHERNET)
    if (CHECK_FLAG (ifp->flags, IFF_UP))
      {
        int link_status = 0;

        link_status = get_link_status_using_mii (if_kernel_name (ifp));
        if (link_status == 1)
          ifp->flags |= IFF_RUNNING;
        else if (link_status == 0)
          ifp->flags &= ~IFF_RUNNING;
        else
          return link_status;
      }
#endif /* SIOCGMIIPHY && SIOCGMIIREG */

  return RESULT_OK;
}
/* get interface flags */
result_t
pal_kernel_if_flags_get (struct interface *ifp)
{
  int ret;
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

#ifndef HAVE_IPNET
  ret = if_ioctl (SIOCGIFFLAGS, (caddr_t) &ifreq);
#else
  ret = if_vr_ioctl (IP_SIOCGIFFLAGS, (caddr_t) &ifreq, ifp->vrf ? ifp->vrf->fib_id : 0);
#endif /* HAVE_IPNET */
  if (ret < 0) /* Don't move this line. */
    return ret;

  ifp->flags = ifreq.ifr_flags & 0x0000ffff;

#if defined (SIOCGMIIPHY) && defined (SIOCGMIIREG)
  if (ifp->hw_type == IF_TYPE_ETHERNET)
    if (CHECK_FLAG (ifp->flags, IFF_UP))
      {
        int link_status = 0;

        link_status = get_link_status_using_mii (if_kernel_name (ifp));
        if (link_status == 1)
          ifp->flags |= IFF_RUNNING;
        else if (link_status == 0)
          ifp->flags &= ~IFF_RUNNING;
        else
          return link_status;
      }
#endif /* SIOCGMIIPHY && SIOCGMIIREG */

  return RESULT_OK;
}

/* Set interface Duplex */
result_t
pal_kernel_if_set_duplex (struct interface *ifp,
                          u_int32_t duplex)
{
  struct ifreq ifreq;
  struct ethtool_cmd ecmd;
  int ret = 0;

  ifreq_set_name (&ifreq, ifp);

  ecmd.cmd = ETHTOOL_GSET;

  ifreq.ifr_data = (caddr_t)&ecmd;

#if defined (SIOCETHTOOL)
  /* Get the current duplex settings */
  ret = if_ioctl (SIOCETHTOOL, (caddr_t)&ifreq);
#else
  return -1;
#endif /* SIOCETHTOOL */

  if (ret < 0)
    {
       return ret;
    }
  else
    {
      ecmd.autoneg = PAL_FALSE;

      if (duplex == NSM_IF_AUTO_NEGO)
        ecmd.autoneg = PAL_TRUE;
      else
        ecmd.duplex = duplex;
      ecmd.cmd = ETHTOOL_SSET;

      ifreq.ifr_data = (caddr_t)&ecmd;

#if defined (SIOCETHTOOL)
      ret = if_ioctl (SIOCETHTOOL, (caddr_t)&ifreq);
#endif /* SIOCETHTOOL */

      if (ret < 0)
        return ret;
    }

  return RESULT_OK;
}

/* Get the duplex settings */
result_t
pal_kernel_if_get_duplex (struct interface *ifp,
                          u_int32_t *duplex)
{
  struct ifreq ifreq;
  struct ethtool_cmd ecmd;
  int ret = 0;

  ifreq_set_name (&ifreq, ifp);

  pal_mem_set (&ecmd, 0, sizeof (struct ethtool_cmd));

  ecmd.cmd = ETHTOOL_GSET;

  ifreq.ifr_data = (caddr_t)&ecmd;

#if defined (SIOCETHTOOL)
  ret = if_ioctl (SIOCETHTOOL, (caddr_t)&ifreq);
#else
  return -1;
#endif /* SIOCETHTOOL */

  if (ret < 0)
    return ret;

  ifp->duplex = ecmd.duplex;

  *duplex = ifp->duplex;

  return RESULT_OK;

}

/* set interface MTU */
result_t
pal_kernel_if_set_mtu ( struct interface *ifp,
                        int mtu_size)
{
   struct ifreq ifreq;
   int    ret;

   ifreq_set_name (&ifreq, ifp);

   ifreq.ifr_mtu = mtu_size;

#ifndef HAVE_IPNET
   ret =  if_ioctl (SIOCSIFMTU, (caddr_t) &ifreq);
#else
   ret = if_vr_ioctl (IP_SIOCSIFMTU, (caddr_t) &ifreq, ifp->vrf ? ifp->vrf->fib_id : 0);
#endif /* HAVE_IPNET */
  if (ret < 0) /* Don't move this line. */
    return ret;

   ifp->mtu = ifreq.ifr_mtu;

   return RESULT_OK;
}

/* get interface MTU */
result_t
pal_kernel_if_get_mtu ( struct interface *ifp)
{
   struct ifreq ifreq;
   int    ret;

   ifreq_set_name (&ifreq, ifp);
#ifndef HAVE_IPNET
   ret =  if_ioctl (SIOCGIFMTU, (caddr_t) &ifreq);
#else
   ret = if_vr_ioctl (IP_SIOCGIFMTU, (caddr_t) &ifreq, ifp->vrf ? ifp->vrf->fib_id: 0);
#endif /* HAVE_IPNET */
  if (ret < 0) /* Don't move this line. */
    return ret;

   ifp->mtu = ifreq.ifr_mtu;

   return RESULT_OK;
}

/* Ret interface proxy arp. */
result_t
pal_kernel_if_set_proxy_arp (struct interface *ifp,
                             s_int32_t proxy_arp)
{
  return RESULT_NO_SUPPORT;
}


/* Set interface flags */
result_t
pal_kernel_if_flags_set (struct interface *ifp,
                         u_int32_t flag)
{
  int ret;
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

  ifreq.ifr_flags = ifp->flags;
  ifreq.ifr_flags |= flag;

#ifndef HAVE_IPNET
  ret = if_ioctl (SIOCSIFFLAGS, (caddr_t) &ifreq);
#else
  ret = if_vr_ioctl (IP_SIOCSIFFLAGS, (caddr_t) &ifreq, ifp->vrf ? ifp->vrf->fib_id : 0);
#endif /* HAVE_IPNET */
  if (ret < 0) /* Don't move this line. */
    return ret;

  return RESULT_OK;
}

/* Unset interface's flag. */
result_t
pal_kernel_if_flags_unset (struct interface *ifp,
                           u_int32_t flag)
{
  int ret;
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

  ifreq.ifr_flags = ifp->flags;
  ifreq.ifr_flags &= ~flag;

#ifndef HAVE_IPNET
  ret = if_ioctl (SIOCSIFFLAGS, (caddr_t) &ifreq);
#else
  ret = if_vr_ioctl (IP_SIOCSIFFLAGS, (caddr_t) &ifreq, ifp->vrf ? ifp->vrf->fib_id : 0);
#endif /* HAVE_IPNET */
  if (ret < 0) /* Don't move this line. */
   return ret;

#ifdef HAVE_IPV6
  /* XXX Linux kernel removes all IPv6 addresss whenever interface goes down */
  if (flag == IFF_UP)
    {
      struct connected *ifc;
      for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
        nsm_connected_invalid_ipv6 (ifp, ifc);
    }
#endif /* HAVE_IPV6 */

  return RESULT_OK;
}

/* Get interface bandwidth. */
result_t
pal_kernel_if_get_bw_fib (struct interface *ifp, fib_id_t fib_id)
{
  /* The following code seems to work when used with latest redHat Linux
   * versions. In older versions, it generates "Operation not supported"
   * error when used. In any error case, this function retuns a bandwidth
   * value of 0x00.
   */
  struct ethtool_cmd ethcmd;
  struct ifreq       ifreq;
  s_int32_t          err;

  if (CHECK_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED))
    return RESULT_OK;

  ifp->bandwidth = 0;

  if (ifp->hw_type != IF_TYPE_ETHERNET)
    return RESULT_OK;

  ifreq_set_name (&ifreq, ifp);
  ethcmd.cmd = ETHTOOL_GSET;
  ifreq.ifr_data = (caddr_t) &ethcmd;

#ifndef HAVE_IPNET
  (void)fib_id;
  err = if_ioctl (SIOCETHTOOL, (caddr_t) &ifreq);
#else
  err = if_vr_ioctl (SIOCETHTOOL, (caddr_t) &ifreq, fib_id);
#endif /* HAVE_IPNET */
  if (err < 0)
    {
       zlog_info (NSM_ZG, "%s \n", strerror(errno));
       return err;
    }

  switch (ethcmd.speed)
    {
    case SPEED_10:
      ifp->bandwidth =  10000000/8;
      zlog_info (NSM_ZG, "10 MB\n");
      break;
    case SPEED_100:
      ifp->bandwidth = 100000000/8;
      zlog_info (NSM_ZG, "100 MB \n");
      break;
    case SPEED_1000:
      ifp->bandwidth = 1000000000/8;
      zlog_info (NSM_ZG, "1000 MB \n");
      break;
    case SPEED_10000:
      ifp->bandwidth = 10000000000LL/8;
      zlog_info (NSM_ZG, "10 GB \n");
      break;
    default:
      zlog_info (NSM_ZG,
                 "ioctl() returned illegal value. Setting bandwidth to 0");
      break;
    }

  return RESULT_OK;
}
/* Get interface bandwidth. */
result_t
pal_kernel_if_get_bw (struct interface *ifp)
{
  /* The following code seems to work when used with latest redHat Linux
   * versions. In older versions, it generates "Operation not supported"
   * error when used. In any error case, this function retuns a bandwidth
   * value of 0x00.
   */
  struct ethtool_cmd ethcmd;
  struct ifreq       ifreq;
  s_int32_t          err;

  if (CHECK_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED))
    return RESULT_OK;

  ifp->bandwidth = 0;

  if (ifp->hw_type != IF_TYPE_ETHERNET)
    return RESULT_OK;

  ifreq_set_name (&ifreq, ifp);
  ethcmd.cmd = ETHTOOL_GSET;
  ifreq.ifr_data = (caddr_t) &ethcmd;

#ifndef HAVE_IPNET
  err = if_ioctl (SIOCETHTOOL, (caddr_t) &ifreq);
#else
  err = if_vr_ioctl (SIOCETHTOOL, (caddr_t) &ifreq, ifp->vrf ? ifp->vrf->fib_id : 0);
#endif /* HAVE_IPNET */
  if (err < 0)
    {
       zlog_info (NSM_ZG, "%s \n", strerror(errno));
       return err;
    }

  switch (ethcmd.speed)
    {
    case SPEED_10:
      ifp->bandwidth =  10000000/8;
      zlog_info (NSM_ZG, "10 MB\n");
      break;
    case SPEED_100:
      ifp->bandwidth = 100000000/8;
      zlog_info (NSM_ZG, "100 MB \n");
      break;
    case SPEED_1000:
      ifp->bandwidth = 1000000000/8;
      zlog_info (NSM_ZG, "1000 MB \n");
      break;
    default:
      zlog_info (NSM_ZG,
                 "ioctl() returned illegal value. Setting bandwidth to 0");
      break;
    }

  return RESULT_OK;
}


#ifdef SIOCSIFHWADDR

/*-------------------------------------------------------------------------
 * pal_kernel_if_set_hwaddr - Setting MAC address
 *
 * Purpose:
 *  To set the interface MAC address: physical, logical or virtual.
 *
 *  Parameters:
 *     IO struct interface *ifp : Interface pointer
 *     A pointer to MAC address value.
 *     Length of te MAC address.
 *
 *  Results:
 *     RESULT_OK for success, RESULT_ERROR for error
 */
result_t
pal_kernel_if_set_hwaddr (struct lib_globals *zg,
                          struct interface *ifp,
                          u_int8_t         *mac_addr,
                          s_int32_t         mac_addr_len)
{
  int i;
  struct ifreq ifr;
  struct sockaddr *sa;
  unsigned char *ptr;
  PAL_UNREFERENCED_PARAMETER (zg);
  PAL_UNREFERENCED_PARAMETER (mac_addr_len);

  if (mac_addr_len != ETHER_ADDR_LEN)
    return RESULT_ERROR;

  memset (&ifr, 0, sizeof(ifr));
  strcat (ifr.ifr_name, if_kernel_name (ifp));

  /* Fill in the new address information */
  sa = &ifr.ifr_hwaddr;
  ptr = sa->sa_data;
  sa->sa_family = ARPHRD_ETHER;
  for (i = 0; i < ETHER_ADDR_LEN; i++)
    *ptr++ = *mac_addr++;

  /* Set the address - Call ioctl() to set HW Addr without downing interface.
     If it fails, shutdown the interface, call ioctl() again and then
     UP the interface
   */
  if (if_ioctl (SIOCSIFHWADDR, (caddr_t)&ifr) < 0)
    {
      bool_t isup = if_is_up (ifp);
      if (isup)
        pal_kernel_if_flags_unset (ifp, IFF_UP);

      if (if_ioctl (SIOCSIFHWADDR, (caddr_t)&ifr) < 0)
        {
          zlog_err (nzg, "VRRP Error: Couldn't set Mac address via ioctl\n");
          return RESULT_ERROR;
        }
      if (isup)
        pal_kernel_if_flags_set (ifp, IFF_UP|IFF_RUNNING);
    }
  return RESULT_OK;
}
#endif /* SIOCSIFHWADDR */


#ifdef HAVE_IPV6
/* Interface's address add/delete functions. */
result_t
pal_kernel_if_ipv6_address_add (struct interface *ifp,
                                struct connected *ifc)
{
  int ret;

  /* Linux kernel returns EOPNOTSUPP/EAGAIN errno
     when the IPv6 address already exists.  */
  ret = kernel_address_add_ipv6 (ifp, ifc);
  if (ret < 0 && errno != EOPNOTSUPP && errno != EAGAIN)
    return -1;

  return RESULT_OK;
}

result_t
pal_kernel_if_ipv6_address_delete (struct interface *ifp,
                                   struct connected *ifc)
{
  int ret;

  ret = kernel_address_delete_ipv6 (ifp, ifc);
  if (ret < 0)
    return -1;

  return RESULT_OK;
}

#ifdef HAVE_MIP6
result_t
pal_if_mip6_home_agent_set (struct interface *ifp)
{
  /* Not yet supported. */
  return 0;
}

result_t
pal_if_mip6_home_agent_unset (struct interface *ifp)
{
  /* Not yet supported. */
  return 0;
}
#endif /* HAVE_MIP6 */

#endif /* HAVE_IPV6 */
