/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "if.h"
#include "sockunion.h"
#include "log.h"
#include "prefix.h"
#include "hash.h"
#include "linklist.h"
#include "nsm/nsm_interface.h"
#include "linklist.h"
#include "if.h"

#include "nsm/nsm_connected.h"
#include "nsm/nsmd.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_rt.h"

#ifdef HAVE_HA
#include "lib_cal.h"
#endif /* HAVE_HA */
#ifdef HAVE_GMPLS
#include "nsm/gmpls/nsm_gmpls.h"
#endif /* HAVE_GMPLS */

extern int get_link_status_using_mii (char *ifname);

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
      exit (1);
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

#ifdef HAVE_IPV6
int
if_ioctl_ipv6 (int request, caddr_t buffer)
{
  int sock;
  int ret = 0;
  int err = 0;

  sock = socket (AF_INET6, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (1);

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
#endif /* HAVE_IPV6 */


/* Interface looking up using infamous SIOCGIFCONF. */
int
interface_list_ioctl (int arbiter)
{
  int ret;
  int sock;
#define IFNUM_BASE 32
  int ifnum;
  struct ifreq *ifreq;
  struct ifconf ifconf;
  struct interface *ifp;
  int n;
  int update;

  /* Normally SIOCGIFCONF works with AF_INET socket. */
  sock = socket (AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      zlog_warn (NSM_ZG, "Can't make AF_INET socket stream: %s", strerror (errno));
      return -1;
    }

  /* Set initial ifreq count.  This will be double when SIOCGIFCONF
     fail.  Solaris has SIOCGIFNUM. */
#ifdef SIOCGIFNUM
  ret = ioctl (sock, SIOCGIFNUM, &ifnum);
  if (ret < 0)
    ifnum = IFNUM_BASE;
  else
    ifnum++;
#else
  ifnum = IFNUM_BASE;
#endif /* SIOCGIFNUM */

  ifconf.ifc_buf = NULL;

  /* Loop until SIOCGIFCONF success. */
  for (;;)
    {
      ifconf.ifc_len = sizeof (struct ifreq) * ifnum;
      ifconf.ifc_buf = XREALLOC (MTYPE_TMP, ifconf.ifc_buf, ifconf.ifc_len);

      ret = ioctl(sock, SIOCGIFCONF, &ifconf);
      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "SIOCGIFCONF: %s", strerror(errno));
          goto end;
        }
      /* When length is same as we prepared, assume it overflowed and
         try again */
      if (ifconf.ifc_len == sizeof (struct ifreq) * ifnum)
        {
          ifnum += 10;
          continue;
        }
      /* Success. */
      break;
    }

  /* Allocate interface. */
  ifreq = ifconf.ifc_req;

  for (n = 0; n < ifconf.ifc_len; n += sizeof(struct ifreq))
    {
      update = 0;
      ifp = ifg_lookup_by_name (&NSM_ZG->ifg, ifreq->ifr_name);
      if (ifp == NULL)
        {
          ifp = ifg_get_by_name (&NSM_ZG->ifg, ifreq->ifr_name);
          update = 1;
        }

      if(arbiter)
        SET_FLAG (ifp->status, NSM_INTERFACE_ARBITER);

      if (update)
        nsm_if_add_update (ifp, FIB_ID_MAIN);
      ifreq++;
    }
 end:
  close (sock);
  /*XFREE (MTYPE_TMP, ifconf.ifc_buf);*/
  pal_mem_free (MTYPE_TMP, ifconf.ifc_buf);

  return ret;
}

/* Get interface's index by ioctl. */
result_t
pal_kernel_if_get_index(struct interface *ifp)
{
  static int if_fake_index = 1;

#ifdef HAVE_BROKEN_ALIASES
  /* Linux 2.2.X does not provide individual interface index for aliases. */
  ifp->ifindex = if_fake_index++;
  return RESULT_OK;
#else
#ifdef SIOCGIFINDEX
  int ret;
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

  ret = if_ioctl (SIOCGIFINDEX, (caddr_t) &ifreq);
  if (ret < 0)
    {
      /* Linux 2.0.X does not have interface index. */
      ifp->ifindex = if_fake_index++;
      return RESULT_OK;
    }

  /* OK we got interface index. */
#ifdef ifr_ifindex
  ifp->ifindex = ifreq.ifr_ifindex;
#else
  ifp->ifindex = ifreq.ifr_index;
#endif
  return RESULT_OK;

#else
  ifp->ifindex = if_fake_index++;
  return RESULT_OK;
#endif /* SIOCGIFINDEX */
#endif /* HAVE_BROKEN_ALIASES */
}

/*
 * get interface metric
 *   -- if value is not avaliable set -1
 */
result_t
pal_kernel_if_get_metric (struct interface *ifp)
{
#ifdef SIOCGIFMETRIC
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

  if (if_ioctl (SIOCGIFMETRIC, (caddr_t) &ifreq) < 0)
    return -1;
  ifp->metric = ifreq.ifr_metric;
  if (ifp->metric == 0)
    ifp->metric = 1;
#else /* SIOCGIFMETRIC */
  ifp->metric = -1;
#endif /* SIOCGIFMETRIC */
  return RESULT_OK;
}

/* set interface MTU */
result_t
pal_kernel_if_set_mtu (struct interface *ifp,
                       s_int32_t mtu_size)
{
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

  ifreq.ifr.mtu = mtu_size;

#if defined(SIOCSIFMTU)
  if (if_ioctl (SIOCSIFMTU, (caddr_t) & ifreq) < 0)
    {
      zlog_info (NSM_ZG, "Can't set mtu by ioctl(SIOCSIFMTU)");
      return -1;
    }

  ifp->mtu = ifreq.ifr_mtu;
#else
  zlog (NSM_ZG, NULL, ZLOG_INFO, "Can't set mtu on this system");
#endif

  return RESULT_OK;
}


/* get interface MTU */
result_t
pal_kernel_if_get_mtu (struct interface *ifp)
{
  struct ifreq ifreq;

  ifreq_set_name (&ifreq, ifp);

#if defined(SIOCGIFMTU)
  if (if_ioctl (SIOCGIFMTU, (caddr_t) & ifreq) < 0)
    {
      zlog_info (NSM_ZG, "Can't lookup mtu by ioctl(SIOCGIFMTU)");
      ifp->mtu = -1;
      return -1;
    }

  ifp->mtu = ifreq.ifr_mtu;
#else
  zlog (NSM_ZG, NULL, ZLOG_INFO, "Can't lookup mtu on this system");
  ifp->mtu = -1;
#endif
  return RESULT_OK;
}

/* Ret interface proxy arp. */
result_t
pal_kernel_if_set_proxy_arp (struct interface *ifp,
                             s_int32_t proxy_arp)
{
  return RESULT_NO_SUPPORT;
}


/* Get interface bandwidth. */
result_t
pal_kernel_if_get_bw (struct interface *ifp)
{
  ifp->bandwidth = 0;
  return RESULT_OK;
}

#ifdef SIOCGIFHWADDR
int
pal_kernel_if_get_hwaddr (struct interface *ifp)
{
  int ret;
  struct ifreq ifreq;
  int i;

  strncpy (ifreq.ifr_name, if_kernel_name (ifp), IFNAMSIZ);
  ifreq.ifr_addr.sa_family = AF_INET;

  /* Fetch Hardware address if available. */
  ret = if_ioctl (SIOCGIFHWADDR, (caddr_t) &ifreq);
  if (ret < 0)
    ifp->hw_addr_len = 0;
  else
    {
      memcpy (ifp->hw_addr, ifreq.ifr_hwaddr.sa_data, 6);

      for (i = 0; i < 6; i++)
        if (ifp->hw_addr[i] != 0)
          break;

      if (i == 6)
        ifp->hw_addr_len = 0;
      else
        ifp->hw_addr_len = 6;
    }
  return 0;
}
#endif

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
  int sock, i;
  struct ifreq ifr;
  struct sockaddr *sa;
  unsigned char *ptr;
  int isup = 0;
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
  if (if_ioctl (SIOCSIFHWADDR, &ifr) < 0)
    {
      bool_t isup = if_is_up (ifp);
      if (isup)
        pal_kernel_if_flags_unset (ifp, IFF_UP);

      if (if_ioctl (SIOCSIFHWADDR, &ifr) < 0)
        {
          zlog_err (ifp->zg,
                    "VRRP Error: Couldn't set Mac address via ioctl\n");
          return RESULT_ERROR;
        }
      if (isup)
        pal_kernel_if_flags_set (ifp, IFF_UP|IFF_RUNNING);
    }
  return RESULT_OK;
}
#endif /* SIOCSIFHWADDR */


#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>

int
if_getaddrs (int arbiter)
{
  int ret;
  struct ifaddrs *ifap;
  struct ifaddrs *ifapfree;
  struct interface *ifp;
  int prefixlen;
  struct connected *ifc;

  ret = getifaddrs (&ifap);
  if (ret != 0)
    {
      zlog_err (NSM_ZG, "getifaddrs(): %s", strerror (errno));
      return -1;
    }

  for (ifapfree = ifap; ifap; ifap = ifap->ifa_next)
    {
      ifp = ifg_lookup_by_name (&NSM_ZG->ifg, ifap->ifa_name);
      if (ifp == NULL)
        {
          zlog_err (NSM_ZG, "if_getaddrs(): Can't lookup interface %s\n",
            ifap->ifa_name);
          continue;
        }

      if (ifap->ifa_addr->sa_family == AF_INET)
        {
          struct sockaddr_in *addr;
          struct sockaddr_in *mask;
          struct sockaddr_in *dest;
          struct in_addr *dest_pnt;

          addr = (struct sockaddr_in *) ifap->ifa_addr;
          mask = (struct sockaddr_in *) ifap->ifa_netmask;
          prefixlen = ip_masklen (mask->sin_addr);

          dest_pnt = NULL;

          if (ifap->ifa_flags & IFF_POINTOPOINT)
            {
              dest = (struct sockaddr_in *) ifap->ifa_dstaddr;
              dest_pnt = &dest->sin_addr;
            }

          if (ifap->ifa_flags & IFF_BROADCAST)
            {
              dest = (struct sockaddr_in *) ifap->ifa_broadaddr;
              dest_pnt = &dest->sin_addr;
            }

          ifc = nsm_connected_add_ipv4 (ifp, 0, &addr->sin_addr,
                                        prefixlen, dest_pnt, NULL);
          if (ifc != NULL)
            if (arbiter)
              {
              SET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
              lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */
              }
        }
#ifdef HAVE_IPV6
      IF_NSM_CAP_HAVE_IPV6
        {
          if (ifap->ifa_addr->sa_family == AF_INET6)
            {
              struct sockaddr_in6 *addr;
              struct sockaddr_in6 *mask;
              struct sockaddr_in6 *dest;
              struct in6_addr *dest_pnt;

              addr = (struct sockaddr_in6 *) ifap->ifa_addr;
              mask = (struct sockaddr_in6 *) ifap->ifa_netmask;
              prefixlen = ip6_masklen (mask->sin6_addr);

              dest_pnt = NULL;

              if (ifap->ifa_flags & IFF_POINTOPOINT)
                {
                  if (ifap->ifa_dstaddr)
                    {
                      dest = (struct sockaddr_in6 *) ifap->ifa_dstaddr;
                      dest_pnt = &dest->sin6_addr;
                    }
                }

              if (ifap->ifa_flags & IFF_BROADCAST)
                {
                  if (ifap->ifa_broadaddr)
                    {
                      dest = (struct sockaddr_in6 *) ifap->ifa_broadaddr;
                      dest_pnt = &dest->sin6_addr;
                    }
                }

              ifc = nsm_connected_add_ipv6 (ifp, &addr->sin6_addr,
                                            prefixlen, dest_pnt);
              if (ifc != NULL)
                if (arbiter)
                  {
                    SET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
                    lib_cal_modify_connected (NSM_ZG, ifc);
#endif /* HAVE_HA */
                  }
            }
        }
#endif /* HAVE_IPV6 */
    }

  freeifaddrs (ifapfree);

  return 0;
}
#else /* HAVE_GETIFADDRS */
/* Interface address lookup by ioctl.  This function only looks up
   IPv4 address. */
int
if_get_addr (struct interface *ifp, int arbiter)
{
  int ret;
  struct ifreq ifreq;
  struct sockaddr_in addr;
  struct sockaddr_in mask;
  struct sockaddr_in dest;
  struct in_addr *dest_pnt;
  u_char prefixlen;
  struct connected *ifc;

  /* Interface's name and address family. */
  strncpy (ifreq.ifr_name, if_kernel_name (ifp), IFNAMSIZ);
  ifreq.ifr_addr.sa_family = AF_INET;

  /* Interface's address. */
  ret = if_ioctl (SIOCGIFADDR, (caddr_t) &ifreq);
  if (ret < 0)
    {
      if (errno != EADDRNOTAVAIL)
        {
          zlog_warn (NSM_ZG, "SIOCGIFADDR fail: %s", strerror (errno));
          return ret;
        }
      return 0;
    }
  memcpy (&addr, &ifreq.ifr_addr, sizeof (struct sockaddr_in));

  /* Interface's network mask. */
  ret = if_ioctl (SIOCGIFNETMASK, (caddr_t) &ifreq);
  if (ret < 0)
    {
      if (errno != EADDRNOTAVAIL)
        {
          zlog_warn (NSM_ZG, "SIOCGIFNETMASK fail: %s", strerror (errno));
          return ret;
        }
      return 0;
    }
#ifdef ifr_netmask
  memcpy (&mask, &ifreq.ifr_netmask, sizeof (struct sockaddr_in));
#else
  memcpy (&mask, &ifreq.ifr_addr, sizeof (struct sockaddr_in));
#endif /* ifr_netmask */
  prefixlen = ip_masklen (mask.sin_addr);

  /* Point to point or borad cast address pointer init. */
  dest_pnt = NULL;

  if (ifp->flags & IFF_POINTOPOINT)
    {
      ret = if_ioctl (SIOCGIFDSTADDR, (caddr_t) &ifreq);
      if (ret < 0)
        {
          if (errno != EADDRNOTAVAIL)
            {
              zlog_warn (NSM_ZG, "SIOCGIFDSTADDR fail: %s", strerror (errno));
              return ret;
            }
          return 0;
        }
      memcpy (&dest, &ifreq.ifr_dstaddr, sizeof (struct sockaddr_in));
      dest_pnt = &dest.sin_addr;
    }
  if (ifp->flags & IFF_BROADCAST)
    {
      ret = if_ioctl (SIOCGIFBRDADDR, (caddr_t) &ifreq);
      if (ret < 0)
        {
          if (errno != EADDRNOTAVAIL)
            {
              zlog_warn (NSM_ZG, "SIOCGIFBRDADDR fail: %s", strerror (errno));
              return ret;
            }
          return 0;
        }
      memcpy (&dest, &ifreq.ifr_broadaddr, sizeof (struct sockaddr_in));
      dest_pnt = &dest.sin_addr;
    }


  /* Set address to the interface. */
  ifc = nsm_connected_add_ipv4 (ifp, 0, &addr.sin_addr,
                                prefixlen, dest_pnt, NULL);
  if (ifc != NULL)
    if (arbiter)
      {
        SET_FLAG (ifc->conf, NSM_IFC_ARBITER);
#ifdef HAVE_HA
        lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */
      }

  return 0;
}
#endif /* HAVE_GETIFADDRS */

/* Fetch interface information via ioctl(). */
static void
interface_info_ioctl (int arbiter)
{
  struct interface *ifp;
  struct listnode *node;

  LIST_LOOP (&NSM_ZG->ifg.if_list, ifp, node)
    {
      pal_kernel_if_get_index (ifp);

#ifdef HAVE_GMPLS
      if (ifp->gifindex == 0)
        {
          ifp->gifindex = NSM_GMPLS_GIFINDEX_GET ();
        }
#endif /* HAVE_GMPLS */

      /* Add interface to database. */
      ifg_table_add (&NSM_ZG->ifg, ifp->ifindex, ifp);

#ifdef SIOCGIFHWADDR
      pal_kernel_if_get_hwaddr (ifp);
#endif /* SIOCGIFHWADDR */
      pal_kernel_if_flags_get (ifp);
#ifndef HAVE_GETIFADDRS
      if_get_addr (ifp, arbiter);
#endif /* ! HAVE_GETIFADDRS */
      pal_kernel_if_get_mtu (ifp);
      pal_kernel_if_get_metric (ifp);
      pal_kernel_if_get_bw (ifp);

      /* Update interface information.  */
      nsm_if_add_update (ifp, FIB_ID_MAIN);
    }
}

/* Lookup all interface information. */
result_t
pal_kernel_if_scan(void)
{
  /* Linux can do both proc & ioctl, ioctl is the only way to get
     interface aliases in 2.2 series kernels. */
#ifdef HAVE_PROC_NET_DEV
  interface_list_proc ();
#endif /* HAVE_PROC_NET_DEV */
  interface_list_ioctl (0);

  /* After listing is done, get index, address, flags and other
     interface's information. */
  interface_info_ioctl (0);

#ifdef HAVE_GETIFADDRS
  if_getaddrs (0);
#endif /* HAVE_GETIFADDRS */

#if defined(HAVE_IPV6) && defined(HAVE_PROC_NET_IF_INET6)
  /* Linux provides interface's IPv6 address via
     /proc/net/if_inet6. */
  IF_NSM_CAP_HAVE_IPV6
    {
      ifaddr_proc_ipv6 ();
    }
#endif /* HAVE_IPV6 && HAVE_PROC_NET_IF_INET6 */
  return RESULT_OK;
}

void
pal_kernel_if_update  (void)
{
  /* Linux can do both proc & ioctl, ioctl is the only way to get
     interface aliases in 2.2 series kernels. */
#ifdef HAVE_PROC_NET_DEV
  interface_list_proc ();
#endif /* HAVE_PROC_NET_DEV */
  interface_list_ioctl (1);

  /* After listing is done, get index, address, flags and other
     interface's information. */
  interface_info_ioctl (1);

#ifdef HAVE_GETIFADDRS
  if_getaddrs (1);
#endif /* HAVE_GETIFADDRS */

#if defined(HAVE_IPV6) && defined(HAVE_PROC_NET_IF_INET6)
  /* Linux provides interface's IPv6 address via
     /proc/net/if_inet6. */
  IF_NSM_CAP_HAVE_IPV6
    {
      ifaddr_proc_ipv6 ();
    }
#endif /* HAVE_IPV6 && HAVE_PROC_NET_IF_INET6 */
}

#ifdef HAVE_IPV6
int
if_ioctl_ipv6 (int request, caddr_t buffer)
{
  int sock;
  int ret = 0;
  int err = 0;

  sock = socket (AF_INET6, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (1);
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
#endif /* HAVE_IPV6 */


/* Set up interface's address, netmask (and broadcas? ).  Linux or
   Solaris uses ifname:number semantics to set IP address aliases. */
result_t
pal_kernel_if_ipv4_address_add (struct interface *ifp,
                                struct connected *ifc)
{
  int ret;
  struct ifreq ifreq;
  struct sockaddr_in addr;
  struct sockaddr_in broad;
  struct sockaddr_in mask;
  struct prefix_ipv4 ifaddr;
  struct prefix_ipv4 *p;

  p = (struct prefix_ipv4 *) ifc->address;

  ifaddr = *p;

  ifreq_set_name (&ifreq, ifp);

  addr.sin_addr = p->prefix;
  addr.sin_family = p->family;
  memcpy (&ifreq.ifr_addr, &addr, sizeof (struct sockaddr_in));
  ret = if_ioctl (SIOCSIFADDR, (caddr_t) &ifreq);
  if (ret < 0)
    return ret;

  /* We need mask for make broadcast addr. */
  masklen2ip (p->prefixlen, &mask.sin_addr);

  if (if_is_broadcast (ifp))
    {
      apply_mask_ipv4 (&ifaddr);
      addr.sin_addr = ifaddr.prefix;

      broad.sin_addr.s_addr = (addr.sin_addr.s_addr | ~mask.sin_addr.s_addr);
      broad.sin_family = p->family;

      memcpy (&ifreq.ifr_broadaddr, &broad, sizeof (struct sockaddr_in));
      ret = if_ioctl (SIOCSIFBRDADDR, (caddr_t) &ifreq);
      if (ret < 0)
        return ret;
    }

  mask.sin_family = p->family;

  memcpy (&ifreq.ifr_netmask, &mask, sizeof (struct sockaddr_in));

  ret = if_ioctl (SIOCSIFNETMASK, (caddr_t) &ifreq);
  if (ret < 0)
    return ret;

  /* Linux version before 2.1.0 need to interface route setup. */
#if defined(GNU_LINUX) && LINUX_VERSION_CODE < 131328
  {
    apply_mask_ipv4 (&ifaddr);
    kernel_add_route (&ifaddr, NULL, ifp->ifindex, 0, 0);
  }
#endif /* ! (GNU_LINUX && LINUX_VERSION_CODE) */

  return RESULT_OK;
}

/* Set up interface's address, netmask (and broadcas? ).  Linux or
   Solaris uses ifname:number semantics to set IP address aliases. */
result_t
pal_kernel_if_ipv4_address_delete (struct interface *ifp,
                                   struct connected *ifc)
{
  int ret;
  struct ifreq ifreq;
  struct sockaddr_in addr;
  struct prefix_ipv4 *p;

  p = (struct prefix_ipv4 *) ifc->address;

  ifreq_set_name (&ifreq, ifp);

  memset (&addr, 0, sizeof (struct sockaddr_in));
  addr.sin_family = p->family;
  memcpy (&ifreq.ifr_addr, &addr, sizeof (struct sockaddr_in));
  ret = if_ioctl (SIOCSIFADDR, (caddr_t) &ifreq);
  if (ret < 0)
    return ret;

  return RESULT_OK;
}

/* get interface flags */
result_t
pal_kernel_if_flags_get(struct interface *ifp)
{
  int ret;
  struct ifreq ifreq;
  int link_status = 0;

  ifreq_set_name (&ifreq, ifp);

  ret = if_ioctl (SIOCGIFFLAGS, (caddr_t) &ifreq);
  if (ret < 0)
    return -1;

  ifp->flags = ifreq.ifr_flags & 0x0000ffff;

#if defined (SIOCGMIIPHY) && defined (SIOCGMIIREG)
  if (ifp->hw_type == IF_TYPE_ETHERNET)
    if (CHECK_FLAG (ifp->flags, IFF_UP))
      {
        link_status = get_link_status_using_mii (if_kernel_name (ifp));
        if (link_status == 1)
          ifp->flags |= IFF_RUNNING;
        else if (link_status == 0)
          ifp->flags &= ~(IFF_UP|IFF_RUNNING);
        else
          return link_status;
      }
#endif /* SIOCGMIIPHY && SIOCGMIIREG */

  return RESULT_OK;
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

  ret = if_ioctl (SIOCSIFFLAGS, (caddr_t) &ifreq);

  if (ret < 0)
    {
      zlog_info (NSM_ZG, "can't set interface flags");
      return ret;
    }
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

  ret = if_ioctl (SIOCSIFFLAGS, (caddr_t) &ifreq);

  if (ret < 0)
    {
      zlog_info (NSM_ZG, "can't unset interface flags");
      return ret;
    }
  return RESULT_OK;
}

#ifdef HAVE_IPV6

#ifdef LINUX_IPV6
#ifndef _LINUX_IN6_H
/* linux/include/net/ipv6.h */
struct in6_ifreq
{
  struct in6_addr ifr6_addr;
  u_int32_t ifr6_prefixlen;
  int ifr6_ifindex;
};
#endif /* _LINUX_IN6_H */

/* Interface's address add/delete functions. */
result_t
pal_kernel_if_ipv6_address_add (struct interface *ifp,
                                struct connected *ifc)
{
  int ret;
  struct prefix_ipv6 *p;
  struct in6_ifreq ifreq;

  p = (struct prefix_ipv6 *) ifc->address;

  memset (&ifreq, 0, sizeof (struct in6_ifreq));

  memcpy (&ifreq.ifr6_addr, &p->prefix, sizeof (struct in6_addr));
  ifreq.ifr6_ifindex = ifp->ifindex;
  ifreq.ifr6_prefixlen = p->prefixlen;

  ret = if_ioctl_ipv6 (SIOCSIFADDR, (caddr_t) &ifreq);

  if (ret == -1)
    return -1;
  else
    return RESULT_OK;
}

result_t
pal_kernel_if_ipv6_address_delete (struct interface *ifp,
                                   struct connected *ifc)
{
  int ret;
  struct prefix_ipv6 *p;
  struct in6_ifreq ifreq;

  p = (struct prefix_ipv6 *) ifc->address;

  memset (&ifreq, 0, sizeof (struct in6_ifreq));

  memcpy (&ifreq.ifr6_addr, &p->prefix, sizeof (struct in6_addr));
  ifreq.ifr6_ifindex = ifp->ifindex;
  ifreq.ifr6_prefixlen = p->prefixlen;

  ret = if_ioctl_ipv6 (SIOCDIFADDR, (caddr_t) &ifreq);

  if (ret == -1)
    return -1;
  else
    return RESULT_OK;
}
#else /* LINUX_IPV6 */

#endif /* LINUX_IPV6 */

#endif /* HAVE_IPV6 */
