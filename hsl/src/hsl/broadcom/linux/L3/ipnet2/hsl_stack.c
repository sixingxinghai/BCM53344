/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h" 
#include "hsl_os.h"

/* Broadcom includes. */
#include "bcm_incl.h"

#undef s6_addr
#undef ifr_name

/* IPNET includes. */
#include "ipcom.h"
#include "ipcom_lkm_h.h"
#include "ipcom_sock.h"
#ifdef HAVE_IPV6
#include "ipcom_sock6.h"
#endif /* HAVE_IPV6 */
#include "ipcom_sock2.h"
#include "ipcom_inet.h"
#include "ipnet.h" 
#include "ipnet_netif.h"
#include "ipnet_routesock.h"
#include "ipcom_sysctl.h"
#include "ipnet_route.h"
#include "ipnet_h.h"

/* HSL includes. */
#include "hsl_types.h"
#include "hsl_table.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_if_os.h"
#include "hsl_ifmgr.h"
#include "hsl_ether.h"
#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
#include "hsl_mcast_fib.h"
#include "hsl_fib_mc_os.h"
#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

/* Marvell L3 interface driver. */
/* HSL includes. */
#include "hsl_types.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_if_os.h"
#include "hsl_ifmgr.h"
#include "hsl_ether.h"
#include "hsl_table.h"
#include "hsl_fib.h"
#include "hsl_fib_os.h"
#include "hsl_eth_drv.h"

static struct hsl_ifmgr_os_callbacks hsl_linux_ipnet_if_os_callbacks;
static struct hsl_fib_os_callbacks hsl_linux_fib_os_callbacks;
static Ip_fd sock_fd;

#ifdef HAVE_MCAST_IPV4
static struct hsl_fib_mc_os_callbacks hsl_linux_ipnet_mc_os_callbacks;
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
static struct hsl_fib_ipv6_mc_os_callbacks hsl_linux_ipnet_ipv6_mc_os_callbacks;
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_IPV6
int hsl_os_if_event_cb (char *ifname, u_int32_t ifindex, u_int32_t ifflags,
    int event, void *data);
#endif /* HAVE_IPV6 */

#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
#define IPNET_IF_INDEXTONETIF(index)  ipnet_if_indextonetif (index)
#else
#define IPNET_IF_INDEXTONETIF(index)  ipnet_if_indextonetif (IPCOM_VR_ANY, (index))
#endif

#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 60701)
#define APN_USE_SA_LEN 1
#elif defined (IPCOM_USE_SA_LEN) /* Release >= 60701 */
#define APN_USE_SA_LEN 1
#endif

#define APN_IN6_IS_ADDR_LINK_LOCAL(addr) \
    ((((addr)->in6.addr16[0]) & 0xffc0 ) == 0xfe80)


#ifndef IPNET_CODE_LOCK
void *apn_sus_handle;

#define IPNET_CODE_LOCK() apn_sus_handle = ipnet_suspend_stack()
#define IPNET_CODE_UNLOCK() if (apn_sus_handle) ipnet_resume_stack(apn_sus_handle)
#endif /* IPNET_CODE_LOCK */

/* Bind socket to FIB */
static int
hsl_os_bind_sock_to_fib (int sock, hsl_fib_id_t fib_id)
{
  int ret;

  ret = ipcom_setsockopt (sock, IP_SOL_SOCKET, IP_SO_X_VR, 
        &fib_id, sizeof (fib_id));

  if (ret < 0)
    HSL_LOG (HSL_LOG_GENERAL, HSL_LEVEL_FATAL, "Cannot bind HSL OS socket to fib %d\n", fib_id);

  return ret;
}

/* 
   Interface manager OS initialization. 
*/
int 
hsl_os_if_init (void)
{
  int ifindex;
  char *loopback_if = "lo";
  int ret;
  struct Ip_ifreq req;
  int mtu = 32768;
  unsigned long flags;
  struct net_device *dev;

  sock_fd = ipcom_socket(IP_AF_INET, IP_SOCK_DGRAM, 0);
  if(sock_fd == IP_SOCKERR)
    return -1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  dev = dev_get_by_name (current->nsproxy->net_ns, loopback_if);
#else 
  dev = dev_get_by_name (loopback_if);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

  if (!dev)
    return -1;

  /* XXX: Do not call dev_put() here because we are holding
   * the reference in ifp
   */

  /* Extract ifindex for this interface. */
  ifindex = ipcom_if_nametoindex (loopback_if);
  if (! ifindex)
    return -1;

  memset (&req, 0, sizeof (req));

  /* Copy interface name. */
  strncpy (req.ifr_name, loopback_if, sizeof (loopback_if));

  req.ifr_ifru.ifru_mtu = mtu;

  ret = ipcom_socketioctl (sock_fd, IP_SIOCGIFMTU, &req);
  if (ret < 0)
    mtu = 1500;
  else
    mtu = req.ifr_ifru.ifru_mtu;  

  ret = ipcom_socketioctl (sock_fd, IP_SIOCGIFFLAGS, &req);
  if (ret < 0)
    flags = 0;
  else
    flags = req.ifr_ifru.ifru_flags;

  /* Register the loopback interface. */
  hsl_ifmgr_L3_loopback_register (loopback_if, ifindex, mtu, flags, dev);


  /* For IPv6 link local addresses, register callback from stack */
#ifdef HAVE_IPV6
  ret = ipcom_socketioctl (sock_fd, IP_SIOCXSIFFEVENTCB, 
      (void *)hsl_os_if_event_cb);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Could not add interface event callback to stack ret=%d\n", ret);
      return ret;
    }
#endif /* HAVE_IPV6 */

  return 0;
}

/* 
   Interface manager OS deinitialization. 
*/
int 
hsl_os_if_deinit (void)
{
  char *loopback_if = "lo";
  struct net_device *dev;
#ifdef HAVE_IPV6
  int ret;
#endif /* HAVE_IPV6 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  dev = dev_get_by_name (current->nsproxy->net_ns, loopback_if);
#else 
  dev = dev_get_by_name (loopback_if);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

  if (!dev)
    return 0;

  /* Release for lookup above */
  dev_put (dev);

  /* Release for lookup is hsl_os_if_init() above */
  dev_put (dev);

  /* For IPv6 link local addresses, unregister callback from stack */
#ifdef HAVE_IPV6
  ret = ipcom_socketioctl (sock_fd, IP_SIOCXDIFFEVENTCB, 
      (void *)hsl_os_if_event_cb);
  if (ret < 0)
    return ret;
#endif /* HAVE_IPV6 */

  (void)ipcom_socketclose(sock_fd);

  return 0;
}

/* 
   Create L3 interface.

   Parameters:
   IN -> name - interface name
   IN -> hwaddr - hardware address
   IN -> hwaddrlen - hardware address length
   OUT -> ifindex - interface index 
   
   Returns:
   HW L3 interface pointer as void *
   NULL on error
*/
void *
hsl_os_l3_if_configure (struct hsl_if *ifp, char *ifname, u_char *hwaddr,
                        int hwaddrlen, hsl_ifIndex_t *ifindex)
{
  struct net_device *dev;
  struct Ip_ifreq ifreq;
  int if_index;

  /* Create netdevice. */
  dev = hsl_eth_drv_create_netdevice (ifp, hwaddr, hwaddrlen);

  if (!dev)
    return NULL;

  if_index = ipcom_if_nametoindex (dev->name);

  if (if_index <= 0)
    return NULL;

  if (ifindex)
    *ifindex = if_index;      
  
  /* 2. Set metric. */
  memset (&ifreq, 0, sizeof (ifreq));

  strncpy (ifreq.ifr_name, dev->name, IP_IFNAMSIZ);
  ifreq.ifr_ifru.ifru_metric = 0;

  ipcom_socketioctl (sock_fd, IP_SIOCSIFMETRIC, &ifreq);

  return (void *)dev;
}

/* 
   Delete L3 interface.

   Parameters:
   IN -> ifp - interface pointer

   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_if_unconfigure (struct hsl_if *ifp)
{
  struct Ip_ifreq req;
  struct net_device *dev;
  int ret;


  if (ifp->type == HSL_IF_TYPE_LOOPBACK)
    return 0;
  
  dev = ifp->os_info;
  if (!dev)
    return -1;

  /* Detach interface from IPNET TCP/IP. */
  memset (&req, 0, sizeof (req));

  /* Set request. */
  strncpy (req.ifr_name, dev->name, IP_IFNAMSIZ);

  ret = ipcom_socketioctl (sock_fd, IP_SIOCXDETACH, &req);
  if (ret < 0)
    return -1;

  /* Destroy netdevice. */
  hsl_eth_drv_destroy_netdevice (dev);

  return 0;
}

/* Set MTU for interface.

Parameters:
IN -> ifp - interface pointer
IN -> mtu - mtu
   
Returns:
0 on success
< 0 on error
*/
int
hsl_os_l3_if_mtu_set (struct hsl_if *ifp, int mtu)
{
  int ret;
  struct Ip_ifreq req;
  struct net_device *dev;

  if (mtu < 0)
    return -1;

  dev = ifp->os_info;
  if (!dev)
    return -1;

  ret = hsl_os_bind_sock_to_fib (sock_fd, ifp->fib_id);
  if (ret < 0)
      return ret;

  memset (&req, 0, sizeof (req));

  /* Set request. */
  strncpy (req.ifr_name, dev->name, IP_IFNAMSIZ);

  /* Set ifreq structure. */
  req.ifr_ifru.ifru_mtu = mtu;

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFMTU, &req);
  if (ret < 0)
    return -1;

  return 0;
}

/* Set HW address for a interface.

Parameters:
IN -> ifp - interface pointer
IN -> hwadderlen - address length
IN -> hwaddr - address
   
Returns:
0 on success
< 0 on error
*/
int 
hsl_os_l3_if_hwaddr_set (struct hsl_if *ifp, int hwaddrlen, u_char *hwaddr)
{
  struct net_device *dev;
  struct Ip_ifreq req;
  int ret;

  dev = ifp->os_info;
  if (!dev)
    return -1;

  ret = hsl_os_bind_sock_to_fib (sock_fd, ifp->fib_id);
  if (ret < 0)
      return ret;

  memset (&req, 0, sizeof (req));

  /* Set request. */
  strncpy (req.ifr_name, dev->name, IP_IFNAMSIZ);
  
  /* Set ifreq structure. */
#ifdef APN_USE_SA_LEN
  req.ifr_addr.sa_len = hwaddrlen;
#endif /* APN_USE_SA_LEN */
  req.ifr_addr.sa_family = IP_AF_LINK;
  memcpy (req.ifr_addr.sa_data, hwaddr, hwaddrlen);

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFLLADDR, &req);

  return ret;
}

/*
  Set L3 interface flags. 

  Parameters:
  IN -> ifp - interface pointer
  IN -> flags - flags
*/
int
hsl_os_l3_if_flags_set (struct hsl_if *ifp, unsigned long flags)
{
  int ret;
  struct Ip_ifreq req;
  struct net_device *dev;
  
  dev = ifp->os_info;
  if (!dev)
    return -1;
  
  ret = hsl_os_bind_sock_to_fib (sock_fd, ifp->fib_id);
  if (ret < 0)
      return ret;

  memset (&req, 0, sizeof (req));

  /* Set request. */
  strncpy (req.ifr_name, dev->name, IP_IFNAMSIZ);

  ret = ipcom_socketioctl (sock_fd, IP_SIOCGIFFLAGS, &req);
  if (ret < 0)
    goto cleanup;

  IP_BIT_SET(req.ip_ifr_flags, flags);

  if (dev->flags & IFF_RUNNING
     || flags & IFF_RUNNING)
    netif_carrier_on (dev);
  else
    netif_carrier_off (dev);

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFFLAGS, &req);

 cleanup:
  return ret;
}

int
hsl_os_l3_if_flags_set2 (struct hsl_if *ifp, unsigned long flags)
{
  struct net_device *dev;
  int ret;
  struct ifreq req;
  mm_segment_t oldfs;
  int vr;


  dev = ifp->os_info;
  if (!dev)
    return -1;

  memset (&req, 0, sizeof (struct ifreq));

  strcpy (req.ifr_ifrn.ifrn_name, dev->name);

  req.ifr_flags = dev->flags | flags;

  oldfs = get_fs ();
  set_fs (get_ds ());
  if (ifp->fib_id)
    {
      vr = current->vr;
      current->vr = ifp->fib_id; 
    }

  if (dev->flags & IFF_RUNNING
     || flags & IFF_RUNNING)
    netif_carrier_on (dev);
  else
    netif_carrier_off (dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  ret = dev_ioctl (current->nsproxy->net_ns, SIOCSIFFLAGS, &req);
#else
  ret = dev_ioctl (SIOCSIFFLAGS, &req); 
#endif /*  LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
  set_fs (oldfs);

  if (ifp->fib_id)
    current->vr = vr;

  return ret;
}

/*
  Unset L3 interface flags. 

  Parameters:
  IN -> ifp - interface pointer
  IN -> flags - flags
  
  Returns:
  0 on success
  < 0 on error
*/
int
hsl_os_l3_if_flags_unset (struct hsl_if *ifp, unsigned long flags)
{
  int ret;
  struct Ip_ifreq req;
  struct net_device *dev;

  dev = ifp->os_info;
  if (!dev)
    return -1;

  ret = hsl_os_bind_sock_to_fib (sock_fd, ifp->fib_id);
  if (ret < 0)
      return ret;

  memset (&req, 0, sizeof (req));

  /* Set request. */
  strncpy (req.ifr_name, dev->name, IP_IFNAMSIZ);

  ret = ipcom_socketioctl (sock_fd, IP_SIOCGIFFLAGS, &req);
  if (ret < 0)
    goto cleanup;

  IP_BIT_CLR(req.ip_ifr_flags, flags);

  if (flags & IFF_RUNNING)
    netif_carrier_off (dev);

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFFLAGS, &req);

 cleanup:
  return ret;
}

int
hsl_os_l3_if_flags_unset2 (struct hsl_if *ifp, unsigned long flags)
{
  struct net_device *dev;
  int ret;
  struct ifreq req;
  unsigned int _flags;
  mm_segment_t oldfs;
  int vr;

  dev = ifp->os_info;
  if (!dev)
    return -1;

  memset (&req, 0, sizeof (struct ifreq));
  
  strcpy (req.ifr_ifrn.ifrn_name, dev->name);

  _flags = dev->flags;
  _flags &= ~flags;
  req.ifr_flags = _flags;
  if (flags & IFF_RUNNING)
    netif_carrier_off (dev);

  oldfs = get_fs ();
  set_fs (get_ds ());
  if (ifp->fib_id)
    {
      vr = current->vr;
      current->vr = ifp->fib_id; 
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  ret = dev_ioctl (current->nsproxy->net_ns, SIOCSIFFLAGS, &req);
#else
  ret = dev_ioctl (SIOCSIFFLAGS, &req);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
  set_fs (oldfs);

  if (ifp->fib_id)
    current->vr = vr;

  return ret;
}

/* 
   Add/Delete IPv4 address. 
*/
static int
_hsl_os_l3_if_address_add_delete (struct hsl_if *ifp, hsl_prefix_t *prefix,
                                  int add)
{
  struct Ip_ifaliasreq ifal;
  struct Ip_sockaddr_in *in_addr;
  struct Ip_sockaddr_in *in_mask;
  int ret;
  hsl_ipv4Address_t mask;
  struct net_device *dev;

  /* If loopback address, just send it as success. */
  if (prefix->u.prefix4 == INADDR_LOOPBACK)
    return 0;

  dev = ifp->os_info;
  if (!dev)
    return -1;

  ret = hsl_os_bind_sock_to_fib (sock_fd, ifp->fib_id); 
  if (ret < 0)
      return ret;

  memset (&ifal, 0, sizeof (struct Ip_ifaliasreq));

  /* Set request. */
  strncpy (ifal.ifra_name, dev->name, IP_IFNAMSIZ);

  /* Set address. */
  in_addr = (struct Ip_sockaddr_in *) &ifal.ifra_addr;
  in_addr->sin_family = IP_AF_INET;
#ifdef APN_USE_SA_LEN
  in_addr->sin_len = sizeof (struct Ip_sockaddr_in);
#endif /* APN_USE_SA_LEN */
  in_addr->sin_addr.s_addr = prefix->u.prefix4;

  hsl_masklen2ip (prefix->prefixlen, &mask);

  /* Set mask. */
  in_mask = (struct Ip_sockaddr_in *) &ifal.ifra_mask;
  in_mask->sin_family = IP_AF_INET;
#ifdef APN_USE_SA_LEN
  in_mask->sin_len = sizeof (struct Ip_sockaddr_in);
#endif /* APN_USE_SA_LEN */
  in_mask->sin_addr.s_addr = mask;

  /* Set destination. */
  in_mask = (struct Ip_sockaddr_in *) &ifal.ifra_dstaddr;
  in_mask->sin_family = IP_AF_INET;
#ifdef APN_USE_SA_LEN
  in_mask->sin_len = sizeof (struct Ip_sockaddr_in);
#endif /* APN_USE_SA_LEN */
  in_mask->sin_addr.s_addr = prefix->u.prefix4 | ~mask;

  if (add)
    ret = ipcom_socketioctl (sock_fd, IP_SIOCAIFADDR, &ifal);
  else
    ret = ipcom_socketioctl (sock_fd, IP_SIOCDIFADDR, &ifal);

  if (ret == IP_ERRNO_EEXIST || ret == IP_ERRNO_ENOENT)
    return 0;


  if (ret < 0)
    return ret;

  return 0;
}
#ifdef HAVE_IPV6
/* 
   Add/Delete IPv6 address. 
*/
static int
_hsl_os_l3_if_address6_add_delete (struct hsl_if *ifp, hsl_prefix_t *prefix,
                                   int add)
{
  struct Ip_in6_aliasreq ifal;
  struct Ip_sockaddr_in6 *in_addr;
  struct Ip_sockaddr_in6 *in_mask;
  int ret;
  struct net_device *dev;

  /* If loopback address, just send it as success. */
  if (IP_IN6_IS_ADDR_LOOPBACK (&prefix->u.prefix6))
    return 0;

  dev = ifp->os_info;
  if (!dev)
    return -1;

  ret = hsl_os_bind_sock_to_fib (sock_fd, ifp->fib_id);
  if (ret < 0)
      return ret;

  memset (&ifal, 0, sizeof (struct Ip_in6_aliasreq));

  /* Set request. */
  strncpy (ifal.ifra_name, dev->name, IP_IFNAMSIZ);

  /* Set address. */
  in_addr = (struct Ip_sockaddr_in6 *) &ifal.ifra_addr;
  in_addr->sin6_family = IP_AF_INET6;
#ifdef APN_USE_SA_LEN
  in_addr->sin6_len = sizeof (struct Ip_sockaddr_in6);
#endif /* APN_USE_SA_LEN */
  IPV6_ADDR_COPY(in_addr->sin6_addr.in6.addr32,prefix->u.prefix6.word);

  if (APN_IN6_IS_ADDR_LINK_LOCAL(&in_addr->sin6_addr))
    ifal.ifra_addr.sin6_scope_id = ifp->ifindex;

  /* Set mask. */
  in_mask = (struct Ip_sockaddr_in6 *) &ifal.ifra_prefixmask;
  in_mask->sin6_family = IP_AF_INET6;
#ifdef APN_USE_SA_LEN
  in_mask->sin6_len = sizeof (struct Ip_sockaddr_in6);
#endif /* APN_USE_SA_LEN */
  ipnet_route_create_mask(&in_mask->sin6_addr, prefix->prefixlen);

  ifal.ifra_lifetime.ia6t_preferred = IPCOM_ADDR_INFINITE;
  ifal.ifra_lifetime.ia6t_expire = IPCOM_ADDR_INFINITE;

  if (add)
    ret = ipcom_socketioctl (sock_fd, IP_SIOCAIFADDR_IN6, &ifal);
  else
    ret = ipcom_socketioctl (sock_fd, IP_SIOCDIFADDR_IN6, &ifal);
  if (ret < 0)
    return ret;

  return 0;
}

#endif /*HAVE_IPV6 */
/* 
   Add a IP address to the interface.

   Parameters:
   IN -> ifp - interface pointer
   IN -> prefix - interface address and prefix
   IN -> flags - flags
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_if_address_add (struct hsl_if *ifp,
                          hsl_prefix_t *prefix, u_char flags)
{
  if(IP_AF_INET == prefix->family)
    return _hsl_os_l3_if_address_add_delete (ifp, prefix, 1);
#ifdef HAVE_IPV6
  else if (IP_AF_INET6 == prefix->family) 
    return _hsl_os_l3_if_address6_add_delete (ifp, prefix, 1);
#endif /* HAVE_IPV6 */
  else
    return -1;
}

/* 
   Delete a IP address from the interface. 

   Parameters:
   IN -> ifp - interface pointer
   IN -> prefix - interface address and prefix
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_if_address_delete (struct hsl_if *ifp,
                             hsl_prefix_t *prefix)
{
  if(IP_AF_INET == prefix->family)
    return _hsl_os_l3_if_address_add_delete (ifp, prefix, 0);
#ifdef HAVE_IPV6 
  else if(IP_AF_INET6 == prefix->family) 
    return _hsl_os_l3_if_address6_add_delete (ifp, prefix, 0);
#endif /* HAVE_IPV6 */
  else
    return -1;
}

/* 
   Bind a interface to a FIB

   Parameters:
   IN -> ifp - interface pointer
   IN -> fib_id - FIB id
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_if_bind_fib (struct hsl_if *ifp,
                             hsl_fib_id_t fib_id)

{
  int ret;
  struct Ip_ifreq req;
  hsl_fib_id_t bindfib;
  struct net_device *dev;

  dev = ifp->os_info;
  if (!dev)
    return -1;	
	

  if (ifp->fib_id == HSL_INVALID_FIB_ID)
      bindfib = HSL_DEFAULT_FIB;
  else
      bindfib = ifp->fib_id;

  /* Bind socket to the current fib of ifp so that
   * name lookup in IPNET does not fail */
  ret = hsl_os_bind_sock_to_fib (sock_fd, bindfib);
  if (ret < 0)
      return ret;

  memset (&req, 0, sizeof (req));

  /* Copy interface name. */
  strncpy (req.ifr_name, dev->name, IP_IFNAMSIZ);

#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  req.ifr_ifru.ifru_rtab = fib_id;

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFRTAB , &req);
#else
  req.ifr_ifru.ifru_vr = fib_id;

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFVR, &req);

#endif

  if (ret < 0)
  {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Cannot bind ifp %s to fib %d in OS\n",
               ifp->name, fib_id);
      return -1;
  }

  return 0;
}

/* 
   Unbind a interface from a FIB

   Parameters:
   IN -> ifp - interface pointer
   IN -> fib_id - FIB id
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_if_unbind_fib (struct hsl_if *ifp,
                             hsl_fib_id_t fib_id)

{
  int ret;
  struct Ip_ifreq req;
  struct net_device *dev;

  dev = ifp->os_info;
  if (!dev)
    return -1;

  /* Bind socket to the current fib of ifp so that
   * name lookup in IPNET does not fail */
  ret = hsl_os_bind_sock_to_fib (sock_fd, ifp->fib_id);
  if (ret < 0)
    return ret;

  memset (&req, 0, sizeof (req));

  /* Copy interface name. */
  strncpy (req.ifr_name, dev->name, IP_IFNAMSIZ);

#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  req.ifr_ifru.ifru_rtab = HSL_DEFAULT_FIB;

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFRTAB , &req);
#else
  req.ifr_ifru.ifru_vr = HSL_DEFAULT_FIB;

  ret = ipcom_socketioctl (sock_fd, IP_SIOCSIFVR, &req);

#endif

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, 
               "Cannot bind ifp %s to fib %d in OS\n", ifp->name, fib_id);
      return -1;
    }

  return 0;
}

#ifdef HAVE_IPV6
int 
hsl_os_if_event_cb (char *ifname, u_int32_t ifindex, u_int32_t ifflags,
    int event, void *data)
{
  hsl_prefix_t pfx;
  struct hsl_if *hsl_if;
  int ret;
  char *loopback_if = "lo";
  struct Ip_in6_addr *addr = NULL;

  ret = 0;

  /* We are only interested in address add/delete events */
  if ((event != IP_EIOXNEWADDR) && (event != IP_EIOXDELADDR))
    return ret;

  /* Originally the IPNET stack always passes IP_NULL as in void *data for
   * all calls to ipnet_kioevent () which invokes this callback. 
   * The IPNET stack has been modified to pass pointer to IPv6 address
   * being added/deleted. For IPv4 addresses, the data is still NULL.
   * XXX: Note that this assumption of NULL data can change if Interpeak
   * modifies the code
   */
  if (data == NULL)
    return ret;

  /* Now that we are sure that it is a IPv6 address being added deleted,
   * verify that it is a link local address.
   */
  addr = (struct Ip_in6_addr *)data;

  if (!APN_IN6_IS_ADDR_LINK_LOCAL (addr))
      return ret;

  /* Ignore fe80::/64 addresses being added by LNE 2.0, LNE 3.0 ANT (IPNET) */
  if (addr->in6.addr32[2] == 0 && addr->in6.addr32[3] == 0)
      return ret;
  
  /* Get HSL If of type HSL_IF_TYPE_IP */
  if (strcmp (loopback_if, ifname) == 0)
    hsl_if = hsl_ifmgr_lookup_by_name_type (ifname, HSL_IF_TYPE_LOOPBACK);
  else
    hsl_if = hsl_ifmgr_lookup_by_name_type (ifname, HSL_IF_TYPE_IP);
  if (hsl_if == NULL)
    return ret;

  memset (&pfx, 0, sizeof (hsl_prefix_t));
  pfx.family = AF_INET6;
  pfx.prefixlen = HSL_IPV6_LINKLOCAL_PREFIXLEN;
  memcpy (&pfx.u.prefix6, data, sizeof (hsl_ipv6Address_t));

  /* Add or delete link local address */
  switch (event)
    {
    case IP_EIOXNEWADDR:
      ret = hsl_ifmgr_os_ip_address_add (ifname, hsl_if->ifindex, &pfx, 0);
      break;

    case IP_EIOXDELADDR:
      ret = hsl_ifmgr_os_ip_address_delete (ifname, hsl_if->ifindex, &pfx);
      break;

    default:
      break;

    }

  HSL_IFMGR_IF_REF_DEC (hsl_if);

  return ret;
}

#endif /* HAVE_IPV6 */

/* 
   Create a FIB 

   Parameters:
   IN -> fib_id - FIB id.
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_fib_init (hsl_fib_id_t fib_id)
{
  int ret;
  int *ip_errno;
  Ip_u16 vr = (Ip_u16) fib_id;

  ip_errno = ipcom_errno_get ();

  ret =  ipcom_socketioctl (sock_fd, IP_SIOCADDVR, &vr);
  if ((ret < 0) && (*ip_errno == IP_ERRNO_EEXIST))
    ret = 0;
  
  if (ret < 0)
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_FATAL, "Cannot create fib %d in OS\n", vr);

  return ret;
}

/* 
   Delete a FIB 

   Parameters:
   IN -> fib_id - FIB id.
   
   Returns:
   0 on success
   < 0 on error
*/

int 
hsl_os_l3_fib_deinit (hsl_fib_id_t fib_id)
{
  Ip_u16 vr = (Ip_u16)fib_id;
  int ret;

  ret = ipcom_socketioctl (sock_fd, IP_SIOCDELVR, &vr);

  if (ret < 0)
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_FATAL, "Cannot delete fib %d in OS\n", vr);

  return ret;
}

static int
_hsl_os_l3_fill_prefix (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct Ipnet_route_add_param *rt_add_param,
                        struct Ipnet_rt_metrics *metrics, struct Ipnet_ipv4_key *mask, struct Ipnet_ipv4_key *key)
{
  int bit_size = sizeof(struct Ip_in_addr) * 8;
  int is_host_pfx = (rnp->p.prefixlen == IPV4_MAX_BITLEN);

  /* Memset. */
  ipcom_memset(rt_add_param, 0, sizeof(struct Ipnet_route_add_param));
  ipcom_memset(mask, 0, sizeof(struct Ipnet_ipv4_key));
  ipcom_memset(key, 0, sizeof(struct Ipnet_ipv4_key));
  ipcom_memset(metrics, 0, sizeof(struct Ipnet_rt_metrics)); 

  /* Set expiry to infinity. */
  metrics->rmx_expire = 0xffffffff;

  key->addr.s_addr = rnp->p.u.prefix4;

  ipnet_route_create_mask(&mask->addr, rnp->p.prefixlen);
  ipnet_route_apply_mask(&key->addr, &mask->addr, bit_size);

  rt_add_param->domain     = IP_AF_INET;
  rt_add_param->key        = key;
  rt_add_param->metrics    = metrics;
  rt_add_param->flags      = IPNET_RTF_UP | IPNET_RTF_DONE;

  /* If host prefix, set IPNET_RTF_HOST flag and do NOT set the netmask
   * param
   */
  if (is_host_pfx)
    rt_add_param->flags      |= IPNET_RTF_HOST;
  else
    {
      rt_add_param->flags      |= IPNET_RTF_MASK;
      rt_add_param->netmask    = mask;
    }


  rt_add_param->pid        = ipcom_getpid();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  rt_add_param->rtab_index = (Ip_u16)fib_id;
#else
  rt_add_param->vr         = (Ip_u16)fib_id;
  rt_add_param->table = 0;
#endif


  return 0;
}


/* 
   Add a ipv4 route 

   Parameters:
   IN -> rnp - route node pointer.
   IN -> nh - next hop pointer 
   
   Returns:
   0 on success
   < 0 on error
*/
static int 
_hsl_os_l3_prefix_add (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct Ipnet_route_add_param rt_add_param;
  struct Ipnet_rt_metrics metrics;
  struct Ipnet_ipv4_key mask;
  struct Ipnet_ipv4_key key;
  struct Ip_sockaddr_in gw;
  int ret;

  memset (&gw, 0, sizeof (struct Ip_sockaddr_in));

  /* Fill prefix. */
  _hsl_os_l3_fill_prefix (fib_id, rnp, &rt_add_param, &metrics, &mask, &key);

  /* Set gateway. */
  gw.sin_family           = IP_AF_INET;
#ifdef APN_USE_SA_LEN
  gw.sin_len              = sizeof(struct Ip_sockaddr_in);
#endif /*APN_USE_SA_LEN*/
  gw.sin_addr.s_addr      = nh->rn->p.u.prefix4;

  if (!CHECK_FLAG(nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    {
      rt_add_param.gateway    = (struct Ip_sockaddr *)&gw;
      rt_add_param.flags      |= IPNET_RTF_GATEWAY;
      rt_add_param.netif      = IP_NULL;
    }
  else
    {
      rt_add_param.gateway    = IP_NULL;
      rt_add_param.flags      |= IPNET_RTF_BLACKHOLE;
      rt_add_param.netif      = IP_NULL;
    }

  IPNET_CODE_LOCK ();
  ret = ipnet_route_add(&rt_add_param);
  IPNET_CODE_UNLOCK ();
  

  return ret;
}

/* 
   Add a ipv4 route 

   Parameters:
   IN -> rnp - route node pointer.
   IN -> ifp - Nexthop interface pointer
   
   Returns:
   0 on success
   < 0 on error
*/
static int 
_hsl_os_l3_prefix_add_if (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  struct Ipnet_route_add_param rt_add_param;
  struct Ipnet_rt_metrics metrics;
  struct Ipnet_ipv4_key mask;
  struct Ipnet_ipv4_key key;
  int ret;

  /* Fill prefix. */
  _hsl_os_l3_fill_prefix (fib_id, rnp, &rt_add_param, &metrics, &mask, &key);

  /* Set gateway. */
  rt_add_param.gateway    = IP_NULL;

  /* Need to set CLONING bit so that it is considered as "connected"
   * route in IPNET
   */
  if (! IP_BIT_ISSET (rt_add_param.flags, IPNET_RTF_HOST))
    IP_BIT_SET (rt_add_param.flags, IPNET_RTF_CLONING);

  IPNET_CODE_LOCK ();

   if (ifp)
  rt_add_param.netif      = IPNET_IF_INDEXTONETIF (ifp->ifindex);
   else
     { 
        IP_BIT_SET(rt_add_param.flags, IPNET_RTF_BLACKHOLE);
        rt_add_param.netif = IP_NULL;
     }

  ret = ipnet_route_add(&rt_add_param);
  IPNET_CODE_UNLOCK ();

  return ret;
}

#ifdef HAVE_IPV6
static int
_hsl_os_l3_fill_prefix6 (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct Ipnet_route_add_param *rt_add_param,
                         struct Ipnet_ipv6_key *key, struct Ipnet_ipv6_key *mask, 
                         struct Ipnet_rt_metrics *metrics)
{
  int    bit_size = sizeof(struct Ip_in6_addr) * 8;
  int    is_host_pfx = (rnp->p.prefixlen == IPV6_MAX_BITLEN);

  ipcom_memset(rt_add_param, 0, sizeof(struct Ipnet_route_add_param));
  ipcom_memset(mask, 0, sizeof(struct Ipnet_ipv6_key));
  ipcom_memset(key, 0, sizeof(struct Ipnet_ipv6_key)); 
  ipcom_memset(metrics, 0, sizeof(struct Ipnet_rt_metrics)); 

  /* Set expiry to infinity. */
  metrics->rmx_expire = 0xffffffff;

  /* Subnet. */
  IPV6_ADDR_COPY(key->addr.in6.addr32,rnp->p.u.prefix6.word);

  /* Mask. */
  ipnet_route_create_mask(&mask->addr, rnp->p.prefixlen);
  ipnet_route_apply_mask(&key->addr, &mask->addr, bit_size);

  rt_add_param->domain     = IP_AF_INET6;
  rt_add_param->key        = key;
  rt_add_param->metrics    = metrics;
  rt_add_param->flags      = IPNET_RTF_UP| IPNET_RTF_DONE;

  /* If host prefix, set IPNET_RTF_HOST flag and do NOT set the netmask
   * param
   */
  if (is_host_pfx)
    rt_add_param->flags      |= IPNET_RTF_HOST;
  else
    {
      rt_add_param->flags      |= IPNET_RTF_MASK;
      rt_add_param->netmask    = mask;
    }

  rt_add_param->pid        = ipcom_getpid();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  rt_add_param->rtab_index = (Ip_u16)fib_id;
#else
  rt_add_param->vr      = (Ip_u16)fib_id;
  rt_add_param->table = 0;
#endif


  return 0;
}
/* 
   Add a ipv6 route 

   Parameters:
   IN -> rnp - route node pointer.
   IN -> nh - next hop pointer 
   
   Returns:
   0 on success
   < 0 on error
*/
static int 
_hsl_os_l3_prefix6_add (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct Ipnet_route_add_param rt_add_param;
  struct Ipnet_ipv6_key        key;
  struct Ipnet_ipv6_key        mask;
  struct Ip_sockaddr_in6       gw;
  struct Ipnet_rt_metrics      metrics;
  int intf = 0;
  int ret;

  memset (&gw, 0, sizeof (struct Ip_sockaddr_in6));

  /* Fix prefix. */
  _hsl_os_l3_fill_prefix6 (fib_id, rnp, &rt_add_param, &key, &mask, &metrics);
  
  /* Gateway. */
  gw.sin6_family          = IP_AF_INET6;
#ifdef APN_USE_SA_LEN
  gw.sin6_len             = sizeof(struct Ip_sockaddr_in6);
#endif /* APN_USE_SA_LEN */

  IPV6_ADDR_COPY(gw.sin6_addr.in6.addr32,nh->rn->p.u.prefix6.word);

  if (APN_IN6_IS_ADDR_LINK_LOCAL (&gw.sin6_addr))
    {
      intf = 1;
      gw.sin6_scope_id = nh->ifp->ifindex;
    }

  if (! CHECK_FLAG(nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    {
      rt_add_param.gateway = (struct Ip_sockaddr *)&gw;
      rt_add_param.flags |= IPNET_RTF_GATEWAY;
    }
  else
    {
      rt_add_param.gateway = IP_NULL;
      rt_add_param.flags |= IPNET_RTF_BLACKHOLE;
    }

  IPNET_CODE_LOCK();
  if (intf)
    rt_add_param.netif      = IPNET_IF_INDEXTONETIF(nh->ifp->ifindex);
  else
    rt_add_param.netif      = IP_NULL;

  ret = ipnet_route_add(&rt_add_param);
  IPNET_CODE_UNLOCK();

  return ret;
}

/* 
   Add a route 
   
   Returns:
   0 on success
   < 0 on error
*/
static int
_hsl_os_l3_prefix6_add_if (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  struct Ipnet_route_add_param rt_add_param;
  struct Ipnet_ipv6_key        key;
  struct Ipnet_ipv6_key        mask;
  struct Ipnet_rt_metrics      metrics;
  int ret;

  /* Fix prefix. */
  _hsl_os_l3_fill_prefix6 (fib_id, rnp, &rt_add_param, &key, &mask, &metrics);
  
  rt_add_param.gateway = IP_NULL;

  /* Need to set CLONING bit so that it is considered as "connected"
   * route in IPNET
   */
  if (! IP_BIT_ISSET (rt_add_param.flags, IPNET_RTF_HOST))
    IP_BIT_SET (rt_add_param.flags, IPNET_RTF_CLONING);

  IPNET_CODE_LOCK();
  rt_add_param.netif = IPNET_IF_INDEXTONETIF(ifp->ifindex);
  ret = ipnet_route_add(&rt_add_param);
  IPNET_CODE_UNLOCK();

  return ret;
}

#endif /* HAVE_IPV6 */ 

/* 
   Add a route 

   Parameters:
   IN -> rnp - route node pointer.
   IN -> nh - next hop pointer 
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_prefix_add (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  if( !rnp | !nh)
    return -1; 

  if(rnp->p.family == IP_AF_INET)
    return _hsl_os_l3_prefix_add (fib_id, rnp, nh);
#ifdef HAVE_IPV6
  else if(rnp->p.family == IP_AF_INET6)
    return _hsl_os_l3_prefix6_add (fib_id, rnp, nh);
#endif 
  else 
    return -1; 
}

static int
_hsl_os_l3_fill_prefix_delete_common (struct hsl_route_node *rnp, struct Ipnet_ipv4_key *key, struct Ipnet_ipv4_key *mask,
    Ip_pid_t *pid)
{
  int    bit_size = sizeof(struct Ip_in_addr) * 8;
  
  ipcom_memset(mask, 0, sizeof(struct Ipnet_ipv4_key));
  ipcom_memset(key, 0, sizeof(struct Ipnet_ipv4_key));

  key->addr.s_addr = rnp->p.u.prefix4; 

  ipnet_route_create_mask(&mask->addr, rnp->p.prefixlen);
  ipnet_route_apply_mask (&key->addr, &mask->addr, bit_size);
  *pid = ipcom_getpid();

  return 0;
}

/* 
   Delete ipv4 route.

   Parameters:
   IN -> rnp - route node pointer.
   IN -> nh - next hop pointer 
   
   Returns:
   0 on success
   < 0 on error
*/
static int 
_hsl_os_l3_prefix_delete (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct Ipnet_ipv4_key mask;
  struct Ipnet_ipv4_key key;
  struct Ip_sockaddr_in gw;
  int    is_host_pfx = (rnp->p.prefixlen == IPV4_MAX_BITLEN);
  Ip_pid_t pid;
  int ret;

  memset (&gw, 0, sizeof (struct Ip_sockaddr_in));

  /* Fill prefix. */
  _hsl_os_l3_fill_prefix_delete_common (rnp, &key, &mask, &pid);

  /* Set gateway. */
  gw.sin_family           = IP_AF_INET;
#ifdef APN_USE_SA_LEN
  gw.sin_len              = sizeof(struct Ip_sockaddr_in);
#endif /* APN_USE_SA_LEN */
  gw.sin_addr.s_addr      = nh->rn->p.u.prefix4;

  IPNET_CODE_LOCK ();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  if (!CHECK_FLAG(nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    ret = ipnet_route_delete2(IP_AF_INET, fib_id, &key,
         (!is_host_pfx ? &mask :IP_NULL), (struct Ip_sockaddr *)&gw, 0, pid,
         0, IP_FALSE);
  else
    ret = ipnet_route_delete2(IP_AF_INET, fib_id, &key,
         (!is_host_pfx ? &mask :IP_NULL), IP_NULL, 0, pid,
         0, IP_FALSE);

#else
  if (!CHECK_FLAG(nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    ret = ipnet_route_delete2(IP_AF_INET, fib_id, 0, &key,
         (!is_host_pfx ? &mask :IP_NULL), (struct Ip_sockaddr *)&gw, 0, pid,
         0, IP_FALSE);
  else
    ret = ipnet_route_delete2(IP_AF_INET, fib_id, 0, &key,
          (!is_host_pfx ? &mask :IP_NULL), IP_NULL, 0, pid,
          0, IP_FALSE);

#endif
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* 
   Delete ipv4 route with interface.

   Parameters:
   IN -> rnp - route node pointer.
   IN -> nh - next hop pointer 
   
   Returns:
   0 on success
   < 0 on error
*/
static int 
_hsl_os_l3_prefix_delete_if (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  struct Ipnet_ipv4_key key;
  struct Ipnet_ipv4_key mask;
  int    is_host_pfx = (rnp->p.prefixlen == IPV4_MAX_BITLEN);
  Ip_pid_t pid;
  int ret;

  /* Fill prefix. */
  _hsl_os_l3_fill_prefix_delete_common (rnp, &key, &mask, &pid);

  IPNET_CODE_LOCK ();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  ret = ipnet_route_delete2(IP_AF_INET, fib_id, &key, 
      (!is_host_pfx ? &mask : IP_NULL), IP_NULL, ifp ? ifp->ifindex : 0, pid, 0, 
      IP_FALSE);
#else
  ret = ipnet_route_delete2(IP_AF_INET, fib_id, 0, &key, 
      (!is_host_pfx ? &mask : IP_NULL), IP_NULL, ifp ? ifp->ifindex : 0, pid, 0, 
      IP_FALSE);
#endif
  IPNET_CODE_UNLOCK ();

  return ret;
}

#ifdef HAVE_IPV6
static int _hsl_os_l3_fill_prefix6_delete_common (struct hsl_route_node *rnp, 
    struct Ipnet_ipv6_key *key, struct Ipnet_ipv6_key *mask, Ip_pid_t *pid)
{
  int    bit_size = sizeof(struct Ip_in6_addr) * 8;

  ipcom_memset(key, 0, sizeof(struct Ipnet_ipv6_key)); 
  ipcom_memset(mask, 0, sizeof(struct Ipnet_ipv6_key)); 
  
  IPV6_ADDR_COPY(key->addr.in6.addr32, rnp->p.u.prefix6.word);

  ipnet_route_create_mask(&mask->addr, rnp->p.prefixlen);
  ipnet_route_apply_mask (&key->addr, &mask->addr ,bit_size);
  *pid = ipcom_getpid();

  return 0;
}

/* 
   Delete ipv6 route.

   Parameters:
   IN -> rnp - route node pointer.
   IN -> nh - next hop pointer 
   
   Returns:
   0 on success
   < 0 on error
*/
static int 
_hsl_os_l3_prefix6_delete (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  int ret;
  struct Ipnet_ipv6_key key;
  struct Ipnet_ipv6_key mask;
  struct Ip_sockaddr_in6 gw;
  int    is_host_pfx = (rnp->p.prefixlen == IPV6_MAX_BITLEN);
  Ip_pid_t pid;
  int intf = 0;

  memset (&gw, 0, sizeof (struct Ip_sockaddr_in6));

  /* Fill prefix. */
  _hsl_os_l3_fill_prefix6_delete_common (rnp, &key, &mask, &pid);

  /* Gateway. */
  gw.sin6_family          = IP_AF_INET6;
#ifdef APN_USE_SA_LEN
  gw.sin6_len             = sizeof(struct Ip_sockaddr_in6);
#endif /* APN_USE_SA_LEN */
  IPV6_ADDR_COPY(gw.sin6_addr.in6.addr32, nh->rn->p.u.prefix6.word);


  if (APN_IN6_IS_ADDR_LINK_LOCAL (&gw.sin6_addr))
    {
      intf = 1;
      gw.sin6_scope_id = nh->ifp->ifindex;
    }

  IPNET_CODE_LOCK ();
  if (intf)
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
 if (!CHECK_FLAG(nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    ret = ipnet_route_delete2 (IP_AF_INET6, fib_id, &key, 
        (!is_host_pfx ? &mask : IP_NULL), 
        (struct Ip_sockaddr *)&gw, nh->ifp->ifindex, pid, 0, IP_FALSE);
 else
    ret = ipnet_route_delete2(IP_AF_INET6, fib_id, &key,
         (!is_host_pfx ? &mask :IP_NULL), IP_NULL, 0, pid,
         0, IP_FALSE);

#else
  if (!CHECK_FLAG(nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    ret = ipnet_route_delete2 (IP_AF_INET6, fib_id, 0, &key, 
        (!is_host_pfx ? &mask : IP_NULL), 
        (struct Ip_sockaddr *)&gw, nh->ifp->ifindex, pid, 0, IP_FALSE);
  else
    ret = ipnet_route_delete2(IP_AF_INET6, fib_id, 0, &key,
         (!is_host_pfx ? &mask :IP_NULL), IP_NULL, 0, pid,
         0, IP_FALSE);

#endif
  else
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
    ret = ipnet_route_delete2 (IP_AF_INET6, fib_id, &key, 
        (!is_host_pfx ? &mask : IP_NULL), 
        (struct Ip_sockaddr *)&gw, 0, pid, 0, IP_FALSE);
#else
    ret = ipnet_route_delete2 (IP_AF_INET6, fib_id, 0, &key, 
        (!is_host_pfx ? &mask : IP_NULL), 
        (struct Ip_sockaddr *)&gw, 0, pid, 0, IP_FALSE);
#endif
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* 
   Delete ipv6 route.

   Parameters:
   IN -> rnp - route node pointer.
   IN -> ifp - interface pointer
   
   Returns:
   0 on success
   < 0 on error
*/
static int 
_hsl_os_l3_prefix6_delete_if (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  int ret;
  struct Ipnet_ipv6_key key;
  struct Ipnet_ipv6_key mask;
  int    is_host_pfx = (rnp->p.prefixlen == IPV6_MAX_BITLEN);
  Ip_pid_t pid;

  /* Fill prefix. */
  _hsl_os_l3_fill_prefix6_delete_common (rnp, &key, &mask, &pid);

  IPNET_CODE_LOCK ();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  ret = ipnet_route_delete2 (IP_AF_INET6, fib_id, &key, 
      (!is_host_pfx ? &mask : IP_NULL), IP_NULL, ifp->ifindex, pid, 0, 
      IP_FALSE);
#else
  ret = ipnet_route_delete2 (IP_AF_INET6, fib_id, 0, &key, 
      (!is_host_pfx ? &mask : IP_NULL), IP_NULL, ifp->ifindex, pid, 0, 
      IP_FALSE);
#endif
  IPNET_CODE_UNLOCK ();

  return ret;
}

#endif /* HAVE_IPV6 */
/* 
   Delete a route.

   Parameters:
   IN -> rnp - route node pointer.
   IN -> nh - next hop pointer 
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_l3_prefix_delete (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{

  if(rnp->p.family == IP_AF_INET)
    return _hsl_os_l3_prefix_delete (fib_id, rnp, nh);
#ifdef HAVE_IPV6 
  else if(rnp->p.family == IP_AF_INET6)
    return _hsl_os_l3_prefix6_delete (fib_id, rnp, nh);
#endif 
  else 
    return -1; 
}

/* Add ipv4 nexthop.

Parameters:
IN -> nh - pointer to nexthop.

Returns:
0 on success
< 0 on failure
*/
static int
_hsl_os_l3_nh_add (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  struct Ip_arpreq arpreq;
  struct Ip_sockaddr_in *addr = (struct Ip_sockaddr_in *) &arpreq.arp_pa;
  struct Ip_sockaddr_dl *dl = (struct Ip_sockaddr_dl *) &arpreq.arp_ha;
  int ret = 0;

  if (! nh)
    return -1;

  ret = hsl_os_bind_sock_to_fib (sock_fd, fib_id);
  if (ret < 0)
      return ret;

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_STATIC))
    {
      /* Initialize. */
      memset (&arpreq, 0, sizeof (struct Ip_arpreq));

      /* Set arpreq values. */
#ifdef APN_USE_SA_LEN
      arpreq.arp_pa.sa_len = sizeof (struct Ip_sockaddr);
#endif /* APN_USE_SA_LEN */
      arpreq.arp_pa.sa_family = IP_AF_INET;
      if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_PROXY))
        arpreq.arp_flags = IP_ATF_PERM|IP_ATF_PUBL;
      else
        arpreq.arp_flags = IP_ATF_PERM;
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
      arpreq.rtab = fib_id; /* Default RTAB for main table. */
#endif

      addr->sin_addr.s_addr = nh->rn->p.u.prefix4;

      /* Set the MAC address */
#ifdef APN_USE_SA_LEN
      dl->sdl_len    = sizeof(struct Ip_sockaddr_dl);
#endif /* APN_USE_SA_LEN */
      dl->sdl_family = IP_AF_LINK;
      dl->sdl_index  = (Ip_u16)nh->ifp->ifindex;
      dl->sdl_nlen   = 0;
      dl->sdl_alen   = HSL_ETHER_ALEN;
      dl->sdl_slen   = 0;
      memcpy(dl->sdl_data, nh->mac, HSL_ETHER_ALEN);

      /* Bind the socket for fib */
      if (ipcom_socketioctl (sock_fd, IP_SIOCSARP, &arpreq) < 0)
        return -1;
    }

  return 0;
}

#ifdef HAVE_IPV6 
/* Add ipv6 nexthop.

Parameters:
IN -> nh - pointer to nexthop.

Returns:
0 on success
< 0 on failure
*/
static int
_hsl_os_l3_nh6_add (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  struct Ip_arpreq arpreq;
  struct Ip_sockaddr_in6 *addr = (struct Ip_sockaddr_in6 *) &arpreq.arp_pa;
  struct Ip_sockaddr_dl *dl = (struct Ip_sockaddr_dl *) &arpreq.arp_ha;
  int ret = 0;

  if (! nh)
    return -1;

  ret = hsl_os_bind_sock_to_fib (sock_fd, fib_id);
  if (ret < 0)
      return ret;

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_STATIC))
    {
      /* Initialize. */
      memset (&arpreq, 0, sizeof (struct Ip_arpreq));

      /* Set arpreq values. */
#ifdef APN_USE_SA_LEN
      arpreq.arp_pa.sa_len = sizeof (struct Ip_sockaddr);
#endif /* APN_USE_SA_LEN */
      arpreq.arp_pa.sa_family = IP_AF_INET6;
      arpreq.arp_flags = IP_ATF_PERM;
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
      arpreq.rtab = fib_id; /* Default RTAB for main table. */
#endif

      IPV6_ADDR_COPY(addr->sin6_addr.in6.addr32,nh->rn->p.u.prefix6.word);

      /* Set the MAC address */
#ifdef APN_USE_SA_LEN
      dl->sdl_len    = sizeof(struct Ip_sockaddr_dl);
#endif /* APN_USE_SA_LEN */
      dl->sdl_family = IP_AF_LINK;
      dl->sdl_index  = (Ip_u16)nh->ifp->ifindex;
      dl->sdl_nlen   = 0;
      dl->sdl_alen   = HSL_ETHER_ALEN;
      dl->sdl_slen   = 0;
      memcpy(dl->sdl_data, nh->mac, HSL_ETHER_ALEN);

      if (ipcom_socketioctl (sock_fd, IP_SIOCSARP, &arpreq) < 0)
        return -1;
    }

  return 0;
}
#endif /* HAVE_IPV6 */
/* Add a nexthop.

Parameters:
IN -> nh - pointer to nexthop.

Returns:
0 on success
< 0 on failure
*/
int
hsl_os_l3_nh_add (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  if(nh->rn->p.family == IP_AF_INET)
    return _hsl_os_l3_nh_add (fib_id, nh);
#ifdef HAVE_IPV6        
  else if(nh->rn->p.family == IP_AF_INET6)
    return _hsl_os_l3_nh6_add (fib_id, nh);
#endif 
  return -1;
}


/*
  Delete ipv4 Nexthop.

  Parameters:
  IN -> nh - pointer to nexthop

  Returns:
  0 on success
  < 0 on failure
*/
static int
_hsl_os_l3_nh_delete (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  struct Ip_arpreq arpreq;
  struct Ip_sockaddr_in *addr = (struct Ip_sockaddr_in *) &arpreq.arp_pa;
  int ret = 0;

  ret = hsl_os_bind_sock_to_fib (sock_fd, fib_id);
  if (ret < 0)
      return ret;

  /* Initialize. */
  memset (&arpreq, 0, sizeof (struct Ip_arpreq));

  /* Set arpreq values. */
#ifdef APN_USE_SA_LEN
  arpreq.arp_pa.sa_len = sizeof (struct Ip_sockaddr);
#endif /* APN_USE_SA_LEN */
  arpreq.arp_pa.sa_family = IP_AF_INET;
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  arpreq.rtab = fib_id; /* Default RTAB for main table. */
#endif

  addr->sin_addr.s_addr = nh->rn->p.u.prefix4;

  if (ipcom_socketioctl (sock_fd, IP_SIOCDARP, &arpreq) < 0)
    return -1;

  return 0;
}

#ifdef HAVE_IPV6
/*
  Delete ipv6 Nexthop.

  Parameters:
  IN -> nh - pointer to nexthop

  Returns:
  0 on success
  < 0 on failure
*/
static int
_hsl_os_l3_nh6_delete (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  struct Ip_arpreq arpreq;
  struct Ip_sockaddr_in6 *addr = (struct Ip_sockaddr_in6 *) &arpreq.arp_pa;
  int ret = 0;

  ret = hsl_os_bind_sock_to_fib (sock_fd, fib_id);
  if (ret < 0)
      return ret;

  /* Initialize. */
  memset (&arpreq, 0, sizeof (struct Ip_arpreq));

  /* Set arpreq values. */
#ifdef APN_USE_SA_LEN
  arpreq.arp_pa.sa_len = sizeof (struct Ip_sockaddr);
#endif /* APN_USE_SA_LEN */
  arpreq.arp_pa.sa_family = IP_AF_INET6;
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  arpreq.rtab = fib_id; /* Default RTAB for main table. */
#endif

  IPV6_ADDR_COPY(addr->sin6_addr.in6.addr32, nh->rn->p.u.prefix6.word);

  if (ipcom_socketioctl (sock_fd, IP_SIOCDARP, &arpreq) < 0)
    return -1;

  return 0;
}
#endif /* HAVE_IPV6 */

/*
  Delete a Nexthop.

  Parameters:
  IN -> nh - pointer to nexthop

  Returns:
  0 on success
  < 0 on failure
*/
int
hsl_os_l3_nh_delete (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  if(!nh)
    return -1;

  if(nh->rn->p.family == IP_AF_INET)
    return _hsl_os_l3_nh_delete (fib_id, nh);
#ifdef HAVE_IPV6
  else if(nh->rn->p.family == IP_AF_INET6)
    return _hsl_os_l3_nh6_delete (fib_id, nh);
#endif
  return -1;

}

/* 
   Add a prefix with the nexthop as a interface.

   Parameters:
   IN -> rnp - route node pointer.
   IN -> ifp - interface pointer
   
   Returns:
   0 on success
   < 0 on error
*/
int
hsl_os_l3_prefix_add_if (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  if (!rnp)
    return -1;

  if(rnp->p.family == IP_AF_INET)
    return _hsl_os_l3_prefix_add_if (fib_id, rnp, ifp);
#ifdef HAVE_IPV6
  else if(rnp->p.family == IP_AF_INET6)
    return _hsl_os_l3_prefix6_add_if (fib_id, rnp, ifp);
#endif
  return -1;
}

/* 
   Delete a prefix with the nexthop as a interface.

   Parameters:
   IN -> rnp - route node pointer.
   IN -> ifp - interface pointer
   
   Returns:
   0 on success
   < 0 on error
*/
int
hsl_os_l3_prefix_delete_if (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  if (!rnp)
    return -1;

  if(rnp->p.family == IP_AF_INET)
    return _hsl_os_l3_prefix_delete_if (fib_id, rnp, ifp);
#ifdef HAVE_IPV6
  else if(rnp->p.family == IP_AF_INET6)
    return _hsl_os_l3_prefix6_delete_if (fib_id, rnp, ifp);
#endif
  return -1;
}

/* 
   Initialize HSL OS callbacks.
*/
int
hsl_if_os_cb_register (void)
{
  hsl_linux_ipnet_if_os_callbacks.os_if_init = hsl_os_if_init;
  hsl_linux_ipnet_if_os_callbacks.os_if_deinit = hsl_os_if_deinit;
  hsl_linux_ipnet_if_os_callbacks.os_l2_if_flags_set = NULL; /* No L2 ports in OS. */
  hsl_linux_ipnet_if_os_callbacks.os_l2_if_flags_unset = NULL; /* No L2 ports in OS. */
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_configure = hsl_os_l3_if_configure;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_unconfigure = hsl_os_l3_if_unconfigure;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_mtu_set = hsl_os_l3_if_mtu_set;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_hwaddr_set = hsl_os_l3_if_hwaddr_set;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_flags_set = hsl_os_l3_if_flags_set2;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_flags_unset = hsl_os_l3_if_flags_unset2;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_address_add = hsl_os_l3_if_address_add;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_address_delete = hsl_os_l3_if_address_delete;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_bind_fib = hsl_os_l3_if_bind_fib;
  hsl_linux_ipnet_if_os_callbacks.os_l3_if_unbind_fib = hsl_os_l3_if_unbind_fib;

  /* Register callbacks with interface manager. */
  hsl_ifmgr_set_os_callbacks (&hsl_linux_ipnet_if_os_callbacks);

  return 0;
}

/*
  Initialize HSL OS callbacks.
*/
int
hsl_if_os_cb_unregister (void)
{
  /* Unregister callbacks with interface manager. */
  hsl_ifmgr_unset_os_callbacks ();

  return 0;
}

/*
  Initialize TCP/IP components.
*/
int
hsl_tcpip_init (void)
{
  return 0;
}

/*
  Deinitialize TCP/IP components.
*/
int
hsl_tcpip_deinit (void)
{
  return 0;
}

#ifdef HAVE_MCAST_IPV4
/* VxWorks/IPNET multicast forwarder internal function forward declaration */
extern int ipnet_ip4_mcast_add_mfc(struct Ipnet_socket_struct *sock, struct Ip_mfcctl *mfcctl);
extern int ipnet_ip4_mcast_del_mfc(struct Ipnet_socket_struct *sock, struct Ip_mfcctl *mfcctl);
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
extern int ipnet_ip4_mcast_add_vif (struct Ip_vifctl *vifctl);
#else
extern int ipnet_ip4_mcast_add_vif (struct Ipnet_socket_struct *sock, struct Ip_vifctl *vifctl);
#endif
extern int ipnet_ip4_mcast_del_vif(struct Ipnet_socket_struct *sock, Ip_vifi_t vifi);
extern void ipnet_ip4_mcast_set_assert(struct Ipnet_socket_struct *sock, Ip_bool val);

/* Enable pim 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv4_mc_pim_init ()
{
  int ret = 0;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  IPNET_CODE_LOCK ();
  ipnet_ip4_mcast_set_assert(&sock,IP_TRUE);
  IPNET_CODE_UNLOCK ();
  
  return ret;
}

/* Disable pim 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv4_mc_pim_deinit ()
{
  int ret = 0;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  IPNET_CODE_LOCK ();
  ipnet_ip4_mcast_set_assert(&sock,IP_FALSE);
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Add ipv4 multicast route 
   Parameters:
   IN -> src         - Source address.
   IN -> group       - Mcast group.
   IN -> iif_vif     - Incoming interface.
   IN -> num_olist   - Number of outgoing interfaces.
   IN -> olist_ttls  - Outgoing interfaces ttls. 

   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_ipv4_mc_route_add (hsl_ipv4Address_t src,hsl_ipv4Address_t group,
                          u_int32_t iif_vif,u_int32_t num_olist,
                          u_char *olist_ttls)
{
  struct Ip_mfcctl mfc;
  int ret = 0;
  Ipnet_socket sock;


  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  memset (&mfc, 0, sizeof (struct Ip_mfcctl));

  mfc.mfcc_origin.s_addr = src;
  mfc.mfcc_mcastgrp.s_addr = group;
  mfc.mfcc_parent = (Ip_vifi_t)iif_vif;
  memcpy (mfc.mfcc_ttls, olist_ttls, IP_MAXVIFS * sizeof (u_char));

  IPNET_CODE_LOCK ();
  ret = ipnet_ip4_mcast_add_mfc (&sock,&mfc);
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Delete ipv4 multicast route 
   Parameters:
   IN -> src         - Source address.
   IN -> group       - Mcast group.

   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_ipv4_mc_route_del (hsl_ipv4Address_t src,hsl_ipv4Address_t group)
{
  struct Ip_mfcctl mfc;
  int ret;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  memset (&mfc, 0, sizeof (struct Ip_mfcctl));

  mfc.mfcc_origin.s_addr = src;
  mfc.mfcc_mcastgrp.s_addr = group;

  IPNET_CODE_LOCK ();
  ret = ipnet_ip4_mcast_del_mfc (&sock,&mfc);
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Add vif to os. 

Parameters:
IN -> vif_index  - mcast interface index. 
IN -> loc_addr   - Local address.
IN -> rmt_addr   - Remote address.  
IN -> flags      - Vif flags. 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv4_mc_vif_add (u_int32_t vif_index, hsl_ipv4Address_t loc_addr,
                        hsl_ipv4Address_t rmt_addr, u_int16_t flags) 
{
  struct Ip_vifctl vif;
  int ret;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  memset (&vif, 0, sizeof (struct Ip_vifctl));

  vif.vifc_vifi = (Ip_vifi_t)vif_index;
  /* XXX: Threshold is 1 for now */
  vif.vifc_threshold = 1;
  /* XXX: Rate limit is 0  */
  vif.vifc_rate_limit = 0;
  vif.vifc_lcl_addr.s_addr = loc_addr;
  vif.vifc_rmt_addr.s_addr = rmt_addr;
  if (flags & HSL_VIFF_REGISTER)
    vif.vifc_flags |= IP_VIFF_REGISTER;
  if (flags & HSL_VIFF_TUNNEL)
    vif.vifc_flags |= IP_VIFF_TUNNEL;

  IPNET_CODE_LOCK ();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  ret = ipnet_ip4_mcast_add_vif (&vif);
#else
  ret = ipnet_ip4_mcast_add_vif (&sock, &vif);
#endif
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Delete vif from os. 

Parameters:
IN -> vif_index  - mcast interface index. 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv4_mc_vif_del (u_int32_t vif_index)
{
  int ret;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  IPNET_CODE_LOCK ();
  ret = ipnet_ip4_mcast_del_vif (&sock,(Ip_vifi_t) vif_index);
  IPNET_CODE_UNLOCK ();

  return ret;
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
/* VxWorks/IPNET multicast forwarder internal function forward declaration */
extern int ipnet_ip6_mcast_add_mfc (struct Ipnet_socket_struct *sock, struct Ip_mf6cctl *mf6cctl);
extern int ipnet_ip6_mcast_del_mfc (struct Ipnet_socket_struct *sock, struct Ip_mf6cctl *mf6cctl);
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
extern int ipnet_ip6_mcast_add_mif (struct Ip_mif6ctl *mif);
#else
extern int ipnet_ip6_mcast_add_mif (struct Ipnet_socket_struct *sock, struct Ip_mif6ctl *mif);
#endif
extern int ipnet_ip6_mcast_del_mif (struct Ipnet_socket_struct *sock,Ip_mifi_t mifi);
extern void ipnet_ip6_mcast_set_pim (struct Ipnet_socket_struct *sock, Ip_bool val);

/* Enable pim 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv6_mc_pim_init ()
{
  int ret = 0;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  IPNET_CODE_LOCK ();
  ipnet_ip6_mcast_set_pim (&sock,1);
  IPNET_CODE_UNLOCK ();
  
  return ret;
}

/* Disable pim 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv6_mc_pim_deinit ()
{
  int ret = 0;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  IPNET_CODE_LOCK ();
  ipnet_ip6_mcast_set_pim (&sock,0);
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Add ipv6 multicast route 
   Parameters:
   IN -> src         - Source address.
   IN -> group       - Mcast group.
   IN -> iif_mif     - Incoming interface.
   IN -> num_olist   - Number of outgoing interfaces.
   IN -> olist       - Outgoing interfaces. 

   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_ipv6_mc_route_add (hsl_ipv6Address_t *src,hsl_ipv6Address_t *group,
                          u_int32_t iif_mif,u_int32_t num_olist,
                          u_int16_t *olist)
{
  struct Ip_mf6cctl mfc;
  int i = 0;
  int ret = 0;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  memset (&mfc, 0, sizeof (struct Ip_mf6cctl));

  mfc.mf6cc_origin.sin6_family = mfc.mf6cc_mcastgrp.sin6_family = IP_AF_INET6;
  IPV6_ADDR_COPY (&mfc.mf6cc_origin.sin6_addr, src);
  IPV6_ADDR_COPY (&mfc.mf6cc_mcastgrp.sin6_addr, group);
  mfc.mf6cc_parent = (Ip_mifi_t)iif_mif;

  for (i = 0; i < num_olist; i++)
    IP_MRT6_IF_SET (olist[i], &mfc.mf6cc_ifset);

  IPNET_CODE_LOCK ();
  ret = ipnet_ip6_mcast_add_mfc (&sock,&mfc);
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Delete ipv6 multicast route 
   Parameters:
   IN -> src         - Source address.
   IN -> group       - Mcast group.

   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_ipv6_mc_route_del (hsl_ipv6Address_t *src,hsl_ipv6Address_t *group)
{
  struct Ip_mf6cctl mfc;
  int ret;
  Ipnet_socket sock;


  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  memset (&mfc, 0, sizeof (struct Ip_mf6cctl));

  mfc.mf6cc_origin.sin6_family = mfc.mf6cc_mcastgrp.sin6_family = IP_AF_INET6;
  IPV6_ADDR_COPY (&mfc.mf6cc_origin.sin6_addr, src);
  IPV6_ADDR_COPY (&mfc.mf6cc_mcastgrp.sin6_addr, group);

  IPNET_CODE_LOCK ();
  ret = ipnet_ip6_mcast_del_mfc (&sock,&mfc);
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Add mif to os. 

Parameters:
IN -> mif_index  - mcast interface index. 
IN -> loc_addr   - Local address.
IN -> rmt_addr   - Remote address.  
IN -> flags      - Vif flags. 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv6_mc_mif_add (u_int32_t mif_index, u_int32_t ifindex, u_int16_t flags) 
{
  struct Ip_mif6ctl mif;
  int ret;

  memset (&mif, 0, sizeof (struct Ip_mif6ctl));

  mif.mif6c_mifi = (Ip_mifi_t) mif_index;
  mif.mif6c_pifi = (u_int16_t) ifindex;
  if (flags & HSL_IPV6_VIFF_REGISTER)
    mif.mif6c_flags |= IP_MRT6_MIFF_REGISTER;

  IPNET_CODE_LOCK ();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  ret = ipnet_ip6_mcast_add_mif (&mif);
#else
  ret = ipnet_ip6_mcast_add_mif (0, &mif);
#endif
  IPNET_CODE_UNLOCK ();

  return ret;
}

/* Delete mif from os. 

Parameters:
IN -> mif_index  - mcast interface index. 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv6_mc_mif_del (u_int32_t mif_index)
{
  int ret;
  Ipnet_socket sock;

  memset(&sock, 0, sizeof(Ipnet_socket));
  sock.vr_index = 0;
  IPNET_CODE_LOCK ();
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 51001)
  ret = ipnet_ip6_mcast_del_mif((Ip_mifi_t) mif_index);
#else
  ret = ipnet_ip6_mcast_del_mif (&sock,(Ip_mifi_t) mif_index);
#endif

  IPNET_CODE_UNLOCK ();

  return ret;
}
#endif /* HAVE_MCAST_IPV6 */
/* 
   Initialize HSL FIB OS callbacks.
*/
int
hsl_fib_os_cb_register (void)
{
  memset (&hsl_linux_fib_os_callbacks, 0, sizeof (struct hsl_fib_os_callbacks));

  hsl_linux_fib_os_callbacks.os_prefix_add = hsl_os_l3_prefix_add;
  hsl_linux_fib_os_callbacks.os_prefix_delete = hsl_os_l3_prefix_delete;
  hsl_linux_fib_os_callbacks.os_nh_add = hsl_os_l3_nh_add;
  hsl_linux_fib_os_callbacks.os_nh_delete = hsl_os_l3_nh_delete;
  hsl_linux_fib_os_callbacks.os_prefix_add_if = hsl_os_l3_prefix_add_if;
  hsl_linux_fib_os_callbacks.os_prefix_delete_if = hsl_os_l3_prefix_delete_if;
  hsl_linux_fib_os_callbacks.os_fib_init = hsl_os_l3_fib_init;
  hsl_linux_fib_os_callbacks.os_fib_deinit = hsl_os_l3_fib_deinit;

  /* Register callbacks with interface manager. */
  hsl_fibmgr_os_cb_register (&hsl_linux_fib_os_callbacks);

  return 0;
}

#ifdef HAVE_MCAST_IPV4
int hsl_ipv4_mc_os_cb_register(void)
{
  HSL_FN_ENTER();
  memset (&hsl_linux_ipnet_mc_os_callbacks , 0, sizeof (struct hsl_fib_mc_os_callbacks));

  hsl_linux_ipnet_mc_os_callbacks.os_ipv4_mc_pim_init = hsl_os_ipv4_mc_pim_init;
  hsl_linux_ipnet_mc_os_callbacks.os_ipv4_mc_pim_deinit = hsl_os_ipv4_mc_pim_deinit;
  hsl_linux_ipnet_mc_os_callbacks.os_ipv4_mc_route_add = hsl_os_ipv4_mc_route_add;
  hsl_linux_ipnet_mc_os_callbacks.os_ipv4_mc_route_del = hsl_os_ipv4_mc_route_del;
  hsl_linux_ipnet_mc_os_callbacks.os_ipv4_mc_vif_add = hsl_os_ipv4_mc_vif_add;
  hsl_linux_ipnet_mc_os_callbacks.os_ipv4_mc_vif_del = hsl_os_ipv4_mc_vif_del;

  /* Register callbacks with fib multicast manager. */
  hsl_ipv4_mc_os_cb_reg(&hsl_linux_ipnet_mc_os_callbacks);

  HSL_FN_EXIT(STATUS_OK);
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
int hsl_ipv6_mc_os_cb_register(void)
{
  HSL_FN_ENTER();
  memset (&hsl_linux_ipnet_ipv6_mc_os_callbacks , 0, sizeof (struct hsl_fib_ipv6_mc_os_callbacks));

  hsl_linux_ipnet_ipv6_mc_os_callbacks.os_ipv6_mc_pim_init = hsl_os_ipv6_mc_pim_init;
  hsl_linux_ipnet_ipv6_mc_os_callbacks.os_ipv6_mc_pim_deinit = hsl_os_ipv6_mc_pim_deinit;
  hsl_linux_ipnet_ipv6_mc_os_callbacks.os_ipv6_mc_route_add = hsl_os_ipv6_mc_route_add;
  hsl_linux_ipnet_ipv6_mc_os_callbacks.os_ipv6_mc_route_del = hsl_os_ipv6_mc_route_del;
  hsl_linux_ipnet_ipv6_mc_os_callbacks.os_ipv6_mc_vif_add = hsl_os_ipv6_mc_mif_add;
  hsl_linux_ipnet_ipv6_mc_os_callbacks.os_ipv6_mc_vif_del = hsl_os_ipv6_mc_mif_del;

  /* Register callbacks with fib multicast manager. */
  hsl_ipv6_mc_os_cb_reg (&hsl_linux_ipnet_ipv6_mc_os_callbacks);

  HSL_FN_EXIT(STATUS_OK);
}

#endif /* HAVE_MCAST_IPV6 */
