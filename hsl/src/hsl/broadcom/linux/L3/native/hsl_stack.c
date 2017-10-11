/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include <linux/if_arp.h>

/* Broadcom includes. */
#include "bcm_incl.h"

/* HSL includes.*/
#include "hsl_types.h"
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl_table.h"
#include "hsl.h"
#include "hsl_fib.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_if_os.h"
#include "hsl_fib_os.h"
#include "hsl_bcm_pkt.h"
#ifdef HAVE_MCAST_IPV4
#include "hsl_mcast_fib.h"
#include "hsl_fib_mc_os.h"
#endif /* HAVE_MCAST_IPV4 */

#include "hal_netlink.h"
#include "hal_msg.h"

/* Broadcom L3 interface driver. */

#include "hsl_eth_drv.h"
#include "linux/if_arp.h"

#ifndef NDA_RTA
#define NDA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif
#ifndef IFA_RTA
#define IFA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#endif

/* Exported system calls from linux (no header file)  */
extern int inet_rtm_newaddr(struct sk_buff *skb, struct nlmsghdr *nlh, void *arg);
extern int inet_rtm_deladdr(struct sk_buff *skb, struct nlmsghdr *nlh, void *arg);
#ifdef HAVE_IPV6
extern int inet6_rtm_newaddr(struct sk_buff *skb, struct nlmsghdr *nlh, void *arg);
extern int inet6_rtm_deladdr(struct sk_buff *skb, struct nlmsghdr *nlh, void *arg);
extern int inet6_rtm_newroute(struct sk_buff *skb, struct nlmsghdr* nlh, void *arg);
extern int inet6_rtm_delroute(struct sk_buff *skb, struct nlmsghdr* nlh, void *arg);
extern int register_inet6addr_notifier(struct notifier_block *nb);
extern int unregister_inet6addr_notifier(struct notifier_block *nb);
#endif /* HAVE_IPV6 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27))
extern int arp_req_set(struct net *net, struct arpreq *r, struct net_device * dev);
extern int arp_req_delete(struct net *net, struct arpreq *r, struct net_device * dev);
#else
extern int arp_req_set(struct arpreq *r, struct net_device * dev);
extern int arp_req_delete(struct arpreq *r, struct net_device * dev);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
extern int ipmr_mfc_add(struct mfcctl *mfc, int mrtsock);
extern int ipmr_mfc_delete(struct mfcctl *mfc);
extern int vif_add(struct vifctl *vifc, int mrtsock);
extern int vif_delete(int vifi);

static struct hsl_ifmgr_os_callbacks  hsl_linux_if_os_callbacks;
#define OS_IFCB(X)                    hsl_linux_if_os_callbacks.X

static struct hsl_fib_os_callbacks    hsl_linux_fib_os_callbacks;
#define OS_FIBCB(X)                   hsl_linux_fib_os_callbacks.X

#ifdef HAVE_MCAST_IPV4
static struct hsl_fib_mc_os_callbacks hsl_linux_mc_os_callbacks;
#define OS_MCCB(X)                    hsl_linux_mc_os_callbacks.X

/* This is not necessary for linux 2.6.32, as it is a member of 
 * struct netns_ipv4 or netns_ipv6, and it is not a decison for 
 * protocol adding.
 */
/*extern int mroute_do_assert;*/
/*extern int mroute_do_pim;*/
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
static struct hsl_fib_ipv6_mc_os_callbacks hsl_linux_ipv6_mc_os_callbacks;
#define OS_IPV6_MCCB(X)                    hsl_linux_ipv6_mc_os_callbacks.X
#endif /* HAVE_MCAST_IPV6 */

#ifdef LINUX_KERNEL_2_6
#define IN6_IS_ADDR_UNSPECIFIED(a) \
        (((__const uint32_t *) (a))[0] == 0                                   \
         && ((__const uint32_t *) (a))[1] == 0                                \
         && ((__const uint32_t *) (a))[2] == 0                                \
         && ((__const uint32_t *) (a))[3] == 0)

#define IN6_IS_ADDR_LOOPBACK(a) \
        (((__const uint32_t *) (a))[0] == 0                                   \
         && ((__const uint32_t *) (a))[1] == 0                                \
         && ((__const uint32_t *) (a))[2] == 0                                \
         && ((__const uint32_t *) (a))[3] == htonl (1))

extern struct net_protocol pim_protocol;
#else
extern struct inet_protocol pim_protocol;
#endif

int hsl_os_l3_if_address_add (struct hsl_if *ifp, hsl_prefix_t *prefix, u_char flags);

#ifndef NDA_RTA
#define NDA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif
#ifndef IFA_RTA
#define IFA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#endif

#ifdef HAVE_IPV6
int hsl_os_ipv6_address_notifier_cb (struct notifier_block *self, unsigned long,
                                     void *);

static struct notifier_block hsl_os_ipv6_address_notifier =
  {
    hsl_os_ipv6_address_notifier_cb,
    NULL,
    0
  };
#endif /* HAVE_IPV6 */

/* Add attribute. */
static int
_addattr_l (struct nlmsghdr *n, int maxlen, int type, void *data, int alen)
{
  int len;
  struct rtattr *rta;

  len = RTA_LENGTH(alen);

  if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen)
    return -1;

  rta = (struct rtattr*) (((char*)n) + NLMSG_ALIGN (n->nlmsg_len));
  rta->rta_type = type;
  rta->rta_len = len;
  memcpy (RTA_DATA(rta), data, alen);
  n->nlmsg_len = NLMSG_ALIGN (n->nlmsg_len) + len;

  return 0;
}

/* 
   Interface manager OS initialization. 
*/
int 
hsl_os_if_init (void)
{
  struct net_device *dev;
  char *loopback_if = "lo";

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  dev = dev_get_by_name (current->nsproxy->net_ns, loopback_if);
#else
  dev = dev_get_by_name (loopback_if);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

  if (! dev)
    return 0;

  /* XXX: Do not call dev_put() here because we are holding
   * the reference in ifp
   */

  /* Register the loopback interface. */
  hsl_ifmgr_L3_loopback_register ("lo", dev->ifindex, 32768, dev->flags, dev);

  /* Register notifier for IPv6 address add/delete callback */
#ifdef HAVE_IPV6
  register_inet6addr_notifier (&hsl_os_ipv6_address_notifier );
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

  /* Unregister notifier for IPv6 address add/delete callback */
#ifdef HAVE_IPV6
  unregister_inet6addr_notifier (&hsl_os_ipv6_address_notifier );
#endif /* HAVE_IPV6 */

  return 0;
}

/* 
   Create L3 interface.

   Parameters:
   IN -> ifp - interface manager ifp pointer to store locally.
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

  /* Create netdevice. */
  dev = hsl_eth_drv_create_netdevice (ifp, hwaddr, hwaddrlen);

  if (ifindex)
    *ifindex = dev->ifindex;

  return dev;
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
  struct net_device *dev;
  
  if (ifp->type == HSL_IF_TYPE_LOOPBACK)
    return 0;
  
  dev = ifp->os_info;

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
  struct net_device *dev;
  struct ifreq req;
  int ret;
  mm_segment_t oldfs;

  dev = ifp->os_info;
  if (! dev)
    return -1;

  if (mtu < 0)
    return -1;

  memset (&req, 0, sizeof (struct ifreq));
  strcpy (req.ifr_name, dev->name);

  req.ifr_mtu = mtu;

  oldfs = get_fs ();
  set_fs (get_ds ());
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  ret = dev_ioctl (current->nsproxy->net_ns, SIOCSIFFLAGS, &req);
#else
  ret = dev_ioctl (SIOCSIFFLAGS, &req);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
  set_fs (oldfs);

  return ret;
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
  int ret;
  struct ifreq req;
  mm_segment_t oldfs;

  dev = ifp->os_info;

  if (! dev || ! dev->netdev_ops || 
      ! dev->netdev_ops->ndo_set_mac_address)
    return -1;

  memset (&req, 0, sizeof (struct ifreq));
  strcpy (req.ifr_name, dev->name);

  memcpy (req.ifr_hwaddr.sa_data, hwaddr, hwaddrlen); 

  oldfs = get_fs ();
  set_fs (get_ds ());
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  ret = devinet_ioctl (current->nsproxy->net_ns, SIOCSIFHWADDR, &req);
#else
  ret = devinet_ioctl (SIOCSIFHWADDR, &req);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
  set_fs (oldfs);

  return ret;
}

/* Set secondary HW addresses for a interface.

   Parameters:
   IN -> ifp - interface pointer
   IN -> hwaddrlen - address length
   IN -> num - number of secondary addresses
   IN -> addresses - array of secondary addresses
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_if_secondary_hwaddrs_set (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses)
{
  return 0;
}

/* Add secondary HW addresses for a interface.

   Parameters:
   IN -> ifp - interface pointer
   IN -> hwaddrlen - address length
   IN -> num - number of secondary addresses
   IN -> addresses - array of secondary addresses
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_if_secondary_hwaddrs_add (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses)
{
  return 0;
}

/* Delete secondary HW addresses for a interface.

   Parameters:
   IN -> ifp - interface pointer
   IN -> hwaddrlen - address length
   IN -> num - number of secondary addresses
   IN -> addresses - array of secondary addresses
   
   Returns:
   0 on success
   < 0 on error
*/
int 
hsl_os_if_secondary_hwaddrs_delete (struct hsl_if *ifp, int hwaddrlen, int num, u_char **addresses)
{
  return 0;
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
  struct net_device *dev;
  int ret;
  struct ifreq req;
  mm_segment_t oldfs;
#ifdef HAVE_IPV6
  hsl_prefix_list_t *ucaddr, *pucaddr;
  hsl_prefix_t prefix;
#endif /* HAVE_IPV6 */

  dev = ifp->os_info;
  if (! dev)
    return -1;

  memset (&req, 0, sizeof (struct ifreq));
  strcpy (req.ifr_name, dev->name);

  req.ifr_flags = dev->flags | flags;

  oldfs = get_fs ();
  set_fs (get_ds ());
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  ret = devinet_ioctl (current->nsproxy->net_ns, SIOCSIFFLAGS, &req);
#else
  ret = devinet_ioctl (SIOCSIFFLAGS, &req);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

  set_fs (oldfs);

#ifdef HAVE_IPV6
  /* The Linux TCP/IP stack removes all configured IPv6 addresses when
     the interface is shutdown. When the interface is brought up again,
     the addresses need to be repopulated.
     For the assigned linklocal addresses by the stack, we are adding
     the 'mostly' existing(linklocal assignment should not change for
     'normal' scenarios) address. We ignore this error and treat it as a 
     success. */
  ucaddr = ifp->u.ip.ipv6.ucAddr;
  pucaddr = NULL;
  if (ucaddr)
    {
      while (ucaddr)
        {
          /* Add it to OS. */
          ret = hsl_os_l3_if_address_add (ifp, &ucaddr->prefix, 0);
          if (ret < 0)
            {
              /* Copy prefix. */
              memcpy (&prefix, &ucaddr->prefix, sizeof (hsl_prefix_t));

              if (! pucaddr)
                  ifp->u.ip.ipv6.ucAddr = ucaddr->next;
              else
                  pucaddr->next = ucaddr->next;

              /* Goto next. */
              ucaddr = ucaddr->next;

              /* Delete the interface address from HSL. */
              hsl_ifmgr_ipv6_address_delete (ifp->name, ifp->ifindex, &prefix);

              /* Send notification to clients. */
              hsl_ifmgr_send_notification (HSL_IF_EVENT_IFDELADDR, ifp, &prefix);
            }
          else
            {
              /* Previous. */
              pucaddr = ucaddr;

              /* Next address. */
              ucaddr = ucaddr->next;
            }
        }
    }
#endif /* HAVE_IPV6 */

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
  struct net_device *dev;
  int ret;
  struct ifreq req;
  unsigned int _flags;
  mm_segment_t oldfs;

  dev = ifp->os_info;
  if (! dev)
    return -1;

  memset (&req, 0, sizeof (struct ifreq));
  strcpy (req.ifr_name, dev->name);

  _flags = dev->flags;
  _flags &= ~flags;
  req.ifr_flags = _flags;

  oldfs = get_fs ();
  set_fs (get_ds ());
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  ret = devinet_ioctl (current->nsproxy->net_ns, SIOCSIFFLAGS, &req);
#else
  ret = devinet_ioctl (SIOCSIFFLAGS, &req);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
  set_fs (oldfs);

  return ret;
}

static void
_parse_rtattr (struct rtattr **tb, int max, struct rtattr *rta, int len)
{
  while (RTA_OK(rta, len))
    {
      if (rta->rta_type <= max)
        tb[rta->rta_type - 1] = (rta);
      rta = RTA_NEXT(rta,len);
    }
}

static int
_hsl_os_l3_if_address (struct hsl_if *ifp, hsl_prefix_t *prefix, u_char flags, int cmd)
{
  struct rtattr *rta[IFA_MAX + 1];
  struct in_addr mask, ip;
  struct net_device *dev;
  struct ifaddrmsg *ifa;
  struct sk_buff skb;
#ifdef HAVE_IPV6
  struct sockaddr_in6 p;
#endif /* HAVE_IPV6 */
  int ret=0, len;
  int bytelen;
  struct
  {
    struct nlmsghdr nlh;
    struct ifaddrmsg ifa;
    char buf[512];
  } req;
  
  memset (&skb,0,sizeof(struct sk_buff));
 
#ifdef HAVE_IPV6
  p.sin6_family = AF_INET6;
  IPV6_ADDR_COPY(p.sin6_addr.s6_addr32, prefix->u.prefix6.word);
#endif /* HAVE_IPV6 */

  if (((prefix->family == AF_INET) && (prefix->u.prefix4 == INADDR_LOOPBACK))
#ifdef HAVE_IPV6
      || ((prefix->family == AF_INET6) && (IN6_IS_ADDR_LOOPBACK (&p.sin6_addr)))
#endif /* HAVE_IPV6 */
      )
    return 0;


  dev = ifp->os_info;
  if (! dev)
    return -1;

  bytelen = (prefix->family == AF_INET ? 4 : 16);
  memset (&req, 0, sizeof (req));
  
  req.nlh.nlmsg_len = NLMSG_LENGTH (sizeof (struct ifaddrmsg));
  req.nlh.nlmsg_flags = flags;
  req.nlh.nlmsg_type = cmd;
  req.ifa.ifa_family = prefix->family;

  req.ifa.ifa_index = ifp->ifindex;
  req.ifa.ifa_prefixlen = prefix->prefixlen;

  _addattr_l (&req.nlh, sizeof (req), IFA_LOCAL, &prefix->u.prefix, bytelen);
  
  if (prefix->family == AF_INET)
    {
      if (ifp->flags & IFF_BROADCAST)
        {
          hsl_masklen2ip (prefix->prefixlen, (hsl_ipv4Address_t *)&mask.s_addr);
          ip.s_addr = prefix->u.prefix4;
          ip.s_addr |= ~mask.s_addr;
          _addattr_l (&req.nlh, sizeof (req), IFA_BROADCAST, &ip.s_addr, bytelen);        
        }
    }

  ifa = NLMSG_DATA (&req.nlh);
  memset (rta, 0, sizeof (rta));
  len = req.nlh.nlmsg_len - NLMSG_ALIGN(sizeof (struct ifaddrmsg));
  _parse_rtattr (rta, IFA_MAX, IFA_RTA(ifa), len);

  if (rta[IFA_ADDRESS - 1] == NULL)
    rta[IFA_ADDRESS - 1] = rta[IFA_LOCAL - 1];
 
  rtnl_lock ();
  if (cmd == RTM_NEWADDR)
    {
      /* Add the address. */
      if (prefix->family == AF_INET)
        ret = inet_rtm_newaddr (&skb, &req.nlh, (void *)&rta);
#ifdef HAVE_IPV6
      else if (prefix->family == AF_INET6)
        ret = inet6_rtm_newaddr (&skb, &req.nlh, (void *)&rta);
#endif  /* HAVE_IPV6 */
    }
  else
    {
      /* Delete the address. */
      if (prefix->family == AF_INET)
        ret = inet_rtm_deladdr (&skb, &req.nlh, (void *)&rta);
#ifdef HAVE_IPV6
      else if (prefix->family == AF_INET6)
        ret = inet6_rtm_deladdr (&skb, &req.nlh, (void *)&rta);
#endif  /* HAVE_IPV6 */
    }
  rtnl_unlock ();

  /* If it already exists, then return 0.
     If it doesn't exists, then return 0. 
     For IPv6 linklocal additions if the address already exists, it
     returns ENOBUFS. An inappropriate error code but that's the way
     it is. */
  if (ret == -EEXIST || ret == -ENOENT || ret == -ENOBUFS)
    ret = 0;
  
  return ret;
}

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
  return _hsl_os_l3_if_address (ifp, prefix, flags, RTM_NEWADDR);
}

#ifdef HAVE_IPV6
int 
hsl_os_ipv6_address_notifier_cb (struct notifier_block *self, 
                                 unsigned long event, void *val)
{
  hsl_prefix_t pfx;
  struct hsl_if *hsl_if;
  int ret;
  char *loopback_if = "lo";
  struct inet6_ifaddr *ifaddr = (struct inet6_ifaddr *)val;
  struct net_device *dev;

  ret = NOTIFY_OK;

  /* We are only interested in address add/delete events */
  /* IPv6 address add comes as NETDEV_UP and delete comes as 
   * NETDEV_DOWN
   */
  if ((event != NETDEV_UP) && (event != NETDEV_DOWN))
    return ret;

  if (ifaddr == NULL)
    return ret;

  /* Now that we are sure that it is a IPv6 address being added deleted,
   * verify that it is a link local address.
   */
  if (!IPV6_IS_ADDR_LINKLOCAL (&ifaddr->addr))
    return ret;

  dev = ifaddr->idev->dev;

  /* Get HSL If of type HSL_IF_TYPE_IP */
  if (strcmp (loopback_if, dev->name) == 0)
    hsl_if = hsl_ifmgr_lookup_by_name_type (dev->name, HSL_IF_TYPE_LOOPBACK);
  else
    hsl_if = hsl_ifmgr_lookup_by_name_type (dev->name, HSL_IF_TYPE_IP);
  if (hsl_if == NULL)
    return ret;

  memset (&pfx, 0, sizeof (hsl_prefix_t));
  pfx.family = AF_INET6;
  pfx.prefixlen = HSL_IPV6_LINKLOCAL_PREFIXLEN;
  memcpy (&pfx.u.prefix6, &ifaddr->addr, sizeof (hsl_ipv6Address_t));

  /* Add or delete link local address */
  switch (event)
    {
    case NETDEV_UP:
      (void)hsl_ifmgr_os_ip_address_add (hsl_if->name, hsl_if->ifindex, &pfx, 0);
      break;

    case NETDEV_DOWN:
      (void)hsl_ifmgr_os_ip_address_delete (hsl_if->name, hsl_if->ifindex, &pfx);
      break;

    default:
      break;
    }

  HSL_IFMGR_IF_REF_DEC (hsl_if);

  return ret;
}
#endif /* HAVE_IPV6 */

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
  return _hsl_os_l3_if_address (ifp, prefix, 0, RTM_DELADDR); 
}

/* 
   Initialize HSL OS callbacks.
*/
int
hsl_if_os_cb_register (void)
{
/* With PAL_PATH enabled in VRF feature, no CBs are required */
  OS_IFCB(os_if_init)                     = hsl_os_if_init;
  OS_IFCB(os_if_deinit)                   = hsl_os_if_deinit;
  OS_IFCB(os_l2_if_flags_set)             = NULL; /* No L2 ports in OS. */
  OS_IFCB(os_l2_if_flags_unset)           = NULL; /* No L2 ports in OS. */
  OS_IFCB(os_l3_if_configure)             = hsl_os_l3_if_configure;
  OS_IFCB(os_l3_if_unconfigure)           = hsl_os_l3_if_unconfigure;
  OS_IFCB(os_l3_if_mtu_set)               = hsl_os_l3_if_mtu_set;
  OS_IFCB(os_l3_if_hwaddr_set)            = hsl_os_l3_if_hwaddr_set;
  OS_IFCB(os_if_secondary_hwaddrs_delete) = hsl_os_if_secondary_hwaddrs_delete;
  OS_IFCB(os_if_secondary_hwaddrs_add)    = hsl_os_if_secondary_hwaddrs_add;
  OS_IFCB(os_if_secondary_hwaddrs_set)    = hsl_os_if_secondary_hwaddrs_set;
  OS_IFCB(os_l3_if_flags_set)             = hsl_os_l3_if_flags_set;
  OS_IFCB(os_l3_if_flags_unset)           = hsl_os_l3_if_flags_unset;
  OS_IFCB(os_l3_if_address_add)           = hsl_os_l3_if_address_add;
  OS_IFCB(os_l3_if_address_delete)        = hsl_os_l3_if_address_delete;
  /* Register callbacks with interface manager. */
  hsl_ifmgr_set_os_callbacks (&hsl_linux_if_os_callbacks);

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

static int
_hsl_os_l3_prefix (struct hsl_route_node *rnp, struct hsl_nh_entry *nh,
                   unsigned int flags, int cmd)
{
  struct rtattr *tb [RTA_MAX + 1];
#ifdef HAVE_IPV6
  struct sockaddr_in6 gw;
#endif /* HAVE_IPV6 */
  struct sk_buff skb;
  int bytelen, len;
  struct
  {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[512];
  } req;
  int ret=0;
  int discard;

  if (! rnp || ! nh)
    return -1;

  memset (&skb, 0, sizeof (struct sk_buff));

  memset (&req, 0, sizeof req);

  bytelen = (rnp->p.family == AF_INET ? 4 : 16);

  req.n.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg));
  req.n.nlmsg_flags = flags;
  req.n.nlmsg_type = cmd;

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
    discard = 1;
  else
    discard = 0;

  /* Set table to default. */
  req.r.rtm_table = 0;

  /* prefix len. */
  req.r.rtm_dst_len = rnp->p.prefixlen;

  req.r.rtm_protocol = RTPROT_ZEBRA;

  req.r.rtm_scope = RT_SCOPE_UNIVERSE;

  if (discard)
    {
      req.r.rtm_type = RTN_BLACKHOLE;
#ifdef HAVE_IPV6
      if (rnp->p.family == AF_INET6)
        req.r.rtm_type = RTN_UNREACHABLE;
#endif /* HAVE_IPV6 */
    }
  else
    req.r.rtm_type = RTN_UNICAST;

  req.r.rtm_family = rnp->p.family; 

  if (rnp->p.family == AF_INET)
    {
      _addattr_l (&req.n, sizeof req, RTA_DST, &rnp->p.u.prefix4, bytelen);

      if (! discard)
        {
          if (nh->rn->p.u.prefix4 == 0)
            _addattr_l (&req.n, sizeof req, RTA_OIF,  &nh->ifp->ifindex, 4);
          else
            _addattr_l (&req.n, sizeof req, RTA_GATEWAY, &nh->rn->p.u.prefix4, bytelen);
        }
    }
#ifdef HAVE_IPV6
  else if (rnp->p.family == AF_INET6)
    {
      /* Gateway. */
      gw.sin6_family = rnp->p.family;
      IPV6_ADDR_COPY (gw.sin6_addr.s6_addr32,nh->rn->p.u.prefix6.word);

      _addattr_l (&req.n, sizeof req, RTA_DST, &rnp->p.u.prefix6, bytelen);

      if (!discard)
        {
          if (IN6_IS_ADDR_UNSPECIFIED (&gw.sin6_addr))
            _addattr_l (&req.n, sizeof req, RTA_OIF,  &nh->ifp->ifindex, 4);
          else
            _addattr_l (&req.n, sizeof req, RTA_GATEWAY, &nh->rn->p.u.prefix6, bytelen);
        }
    }
#endif /* HAVE_IPV6 */
  else
    return 0; 

  if (!discard && nh->ifp)
    _addattr_l (&req.n, sizeof req, RTA_OIF, &nh->ifp->ifindex, 4);

  len = req.n.nlmsg_len - NLMSG_ALIGN(sizeof (struct rtmsg));
  memset (tb, 0, sizeof tb);
  _parse_rtattr (tb, RTA_MAX, RTM_RTA (NLMSG_DATA (&req.n)), len);

  rtnl_lock ();
  if (cmd == RTM_NEWROUTE)
    {
      if (rnp->p.family == AF_INET)
        ret = inet_rtm_newroute (&skb, &req.n, (void *)&tb);
#ifdef HAVE_IPV6
      else if (rnp->p.family == AF_INET6)
        ret = inet6_rtm_newroute (&skb, &req.n, (void *)&tb);
#endif /* HAVE_IPV6 */
      if (ret == -EEXIST)
        ret = 0;
    }
  else
    {
      if (rnp->p.family == AF_INET)
        ret = inet_rtm_delroute (&skb, &req.n, (void *)&tb);
#ifdef HAVE_IPV6
      else if (rnp->p.family == AF_INET6)
        ret = inet6_rtm_delroute (&skb, &req.n, (void *)&tb);
#endif /* HAVE_IPV6 */
    }
  rtnl_unlock ();

  return ret;
}

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
  return _hsl_os_l3_prefix (rnp, nh, NLM_F_CREATE | NLM_F_APPEND, RTM_NEWROUTE);
}


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
  return _hsl_os_l3_prefix (rnp, nh, 0, RTM_DELROUTE);
}

/*
  Add a IPv4 nexthop.

  Parameters:
  IN -> nh - pointer to nexthop.

  Returns:
  0 on success
  < 0 on failure
*/
static int
_hsl_os_l3_ipv4_nh_add (struct hsl_nh_entry *nh)
{
  struct arpreq req;
  struct hsl_if *ifp;
  struct net_device *dev;
  struct sockaddr_in *in;

  if (! nh)
    return -1;

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_STATIC))
    {
      ifp = nh->ifp;
      if (! ifp)
        return -1;
      
      dev = ifp->os_info;
      if (! dev)
        return -1;
      
      memset (&req, 0, sizeof (struct arpreq));
      
      in = (struct sockaddr_in *) &req.arp_pa;
      in->sin_addr.s_addr = nh->rn->p.u.prefix4;
      in->sin_family = AF_INET;
      req.arp_ha.sa_family = dev->type;
      req.arp_flags = ATF_PERM;
      strncpy (req.arp_dev, dev->name, sizeof (req.arp_dev));
      
      /* Set arp entry. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27))
      arp_req_set (&init_net,&req, dev);
#else
      arp_req_set (&req, dev);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */
    }

  return 0;
}

#ifdef HAVE_IPV6
/*
  Add/Delete a IPv6 nexthop.

  Parameters:
  IN -> nh - pointer to nexthop.
  IN -> flags - Netlink message flags
  IN -> cmd   - Netlink message cmd

  Returns:
  0 on success
  < 0 on failure
*/
static int
_hsl_os_l3_ipv6_nh (struct hsl_nh_entry *nh,
                    unsigned int flags, int cmd)
{
  struct sk_buff skb;
  struct
  {
    struct nlmsghdr n;
    struct ndmsg nd;
    char buf[512];
  } req;
  int ret;
  int bytelen, len;
  struct rtattr *tb [NDA_MAX + 1];

  if (! nh)
    return -1;

  if (!CHECK_FLAG (nh->flags, HSL_NH_ENTRY_STATIC))
    return 0;

  memset (&skb, 0, sizeof (struct sk_buff));

  memset (&req, 0, sizeof req);

  bytelen = 16;

  req.n.nlmsg_len = NLMSG_LENGTH (sizeof (struct ndmsg));
  req.n.nlmsg_flags = flags;
  req.n.nlmsg_type = cmd;

  /* Family */
  req.nd.ndm_family = AF_INET6;

  /* Ifindex */
  if (nh->ifp)
    req.nd.ndm_ifindex = nh->ifp->ifindex;

  /* State */
  req.nd.ndm_state |= NUD_PERMANENT;

  _addattr_l (&req.n, sizeof req, NDA_DST, &nh->rn->p.u.prefix6, bytelen);

  len = req.n.nlmsg_len - NLMSG_ALIGN(sizeof (struct ndmsg));
  memset (tb, 0, sizeof tb);
  _parse_rtattr (tb, NDA_MAX, NDA_RTA (NLMSG_DATA (&req.n)), len);

  rtnl_lock ();
  if (cmd == RTM_NEWNEIGH)
    ret = neigh_add (&skb, &req.n, (void *)&tb);
  else
    ret = neigh_delete (&skb, &req.n, (void *)&tb);
  rtnl_unlock ();

  return ret;
}
#endif /* HAVE_IPV6 */

/*
  Add a nexthop.

  Parameters:
  IN -> nh - pointer to nexthop.

  Returns:
  0 on success
  < 0 on failure
*/
int
hsl_os_l3_nh_add (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  if (nh->rn->p.family == AF_INET)
    return _hsl_os_l3_ipv4_nh_add (nh);
#ifdef HAVE_IPV6
  else if (nh->rn->p.family == AF_INET6)
    return _hsl_os_l3_ipv6_nh (nh, NLM_F_CREATE | NLM_F_APPEND, RTM_NEWNEIGH);
#endif /* HAVE_IPV6 */
  return -1;
}

/*
  Delete a IPv4 Nexthop due to hardware ageing.

  Parameters:
  IN -> nh - pointer to nexthop.

  Returns:
  0 on success
  < 0 on failure
*/
int
_hsl_os_l3_ipv4_nh_delete (struct hsl_nh_entry *nh)
{
  struct arpreq req;
  struct hsl_if *ifp;
  struct net_device *dev;
  struct sockaddr_in *in;

  if (! nh)
    return -1;

  ifp = nh->ifp;
  if (! ifp)
    return -1;

  dev = ifp->os_info;
  if (! dev)
    return -1;

  memset (&req, 0, sizeof (struct arpreq));

  in = (struct sockaddr_in *) &req.arp_pa;
  in->sin_addr.s_addr = nh->rn->p.u.prefix4;
  in->sin_family = AF_INET;
  req.arp_ha.sa_family = dev->type;
  strncpy (req.arp_dev, dev->name, sizeof (req.arp_dev));

  /* Delete arp entry. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27))
  arp_req_delete (&init_net, &req, dev);
#else
  arp_req_delete (&req, dev);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

  return 0;
}

/*
  Delete a Nexthop due to hardware ageing.

  Parameters:
  IN -> nh - pointer to nexthop.

  Returns:
  0 on success
  < 0 on failure
*/
int
hsl_os_l3_nh_delete (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  if (nh->rn->p.family == AF_INET)
    return _hsl_os_l3_ipv4_nh_delete (nh);
#ifdef HAVE_IPV6
  else if (nh->rn->p.family == AF_INET6)
    return _hsl_os_l3_ipv6_nh (nh, 0, RTM_DELNEIGH);
#endif /* HAVE_IPV6 */
  return -1;
}

static int
_hsl_os_l3_prefix_if (struct hsl_route_node *rnp, struct hsl_if *ifp,
                      unsigned int flags, int cmd)
{
  struct sk_buff skb;
  struct
  {
    struct nlmsghdr n;
    struct rtmsg r;
    char buf[512];
  } req;
  int ret=0;
  int bytelen, len;
  struct rtattr *tb [RTA_MAX + 1];
  struct in_addr ip, mask;
#ifdef HAVE_IPV6
  hsl_prefix_t ipv6;
#endif /* HAVE_IPV6 */

  if (! rnp)
    return -1;

  memset (&skb, 0, sizeof (struct sk_buff));

  memset (&req, 0, sizeof req);

  bytelen = (rnp->p.family == AF_INET ? 4 : 16);
  
  req.n.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg));
  req.n.nlmsg_flags = flags;
  req.n.nlmsg_type = cmd;
  
  req.r.rtm_family = rnp->p.family; 

  /* Set table to default. */
  req.r.rtm_table = 0;

  /* prefix len. */
  req.r.rtm_dst_len = rnp->p.prefixlen;

  req.r.rtm_protocol = RTPROT_ZEBRA;
  req.r.rtm_scope = RT_SCOPE_LINK;

  if (ifp)
    req.r.rtm_type = RTN_UNICAST;
  else
    req.r.rtm_type = RTN_BLACKHOLE;

  if (rnp->p.family == AF_INET)
    {
      hsl_masklen2ip (rnp->p.prefixlen, (hsl_ipv4Address_t *)&mask.s_addr);
      ip.s_addr = rnp->p.u.prefix4;
      ip.s_addr &= mask.s_addr;
      
      _addattr_l (&req.n, sizeof req, RTA_DST, &ip.s_addr, bytelen);
    }
#ifdef HAVE_IPV6
  else if (rnp->p.family == AF_INET6)
    {
      hsl_prefix_copy (&ipv6, &rnp->p);
      hsl_apply_mask (&ipv6);

      _addattr_l (&req.n, sizeof req, RTA_DST, &ipv6.u.prefix6, bytelen);
    }
#endif /* HAVE_IPV6 */
  else
    return 0; 


  if (ifp)
    _addattr_l (&req.n, sizeof req, RTA_OIF, &ifp->ifindex, 4);

  len = req.n.nlmsg_len - NLMSG_ALIGN(sizeof (struct rtmsg));
  memset (tb, 0, sizeof tb);
  _parse_rtattr (tb, RTA_MAX, RTM_RTA (NLMSG_DATA (&req.n)), len);

  rtnl_lock ();
  if (cmd == RTM_NEWROUTE)
    {
      if (rnp->p.family == AF_INET)
        ret = inet_rtm_newroute (&skb, &req.n, (void *)tb);
#ifdef HAVE_IPV6
      else if (rnp->p.family == AF_INET6)
        ret = inet6_rtm_newroute (&skb, &req.n, (void *)tb);
#endif /* HAVE_IPV6 */

     /* After link up , for connected route stack returns EEXIST,
      * Ignore the  return code */
     if (ret == -EEXIST)
       ret = 0;
    }
  else
    {
      if (rnp->p.family == AF_INET)
        ret = inet_rtm_delroute (&skb, &req.n, (void *)tb);
#ifdef HAVE_IPV6
      else if (rnp->p.family == AF_INET6)
        ret = inet6_rtm_delroute (&skb, &req.n, (void *)tb);
#endif /* HAVE_IPV6 */

      /* Ignore the not found return code */
      if (ret == -ESRCH)
        ret = 0;
    }
  rtnl_unlock ();

  return ret;
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
  return _hsl_os_l3_prefix_if (rnp, ifp, NLM_F_CREATE | NLM_F_REPLACE, RTM_NEWROUTE);
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
  return _hsl_os_l3_prefix_if (rnp, ifp, 0, RTM_DELROUTE);
}

#ifdef HAVE_MCAST_IPV4
/* Enable pim 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv4_mc_pim_init ()
{
  rtnl_lock ();
#if 0
  mroute_do_assert = 1;
  mroute_do_pim = 1;
#endif 
#ifdef LINUX_KERNEL_2_6
  inet_add_protocol (&pim_protocol, IPPROTO_PIM);
#else
  inet_add_protocol (&pim_protocol);
#endif
  rtnl_unlock ();
  return 0;
}

/* Disable pim 

Returns:
0 on success
< 0 on error
*/
int 
hsl_os_ipv4_mc_pim_deinit ()
{
  rtnl_lock ();
#if 0
  mroute_do_assert = 0;
  mroute_do_pim = 0;
#endif
#ifdef LINUX_KERNEL_2_6
  inet_del_protocol (&pim_protocol, IPPROTO_PIM);
#else
  inet_del_protocol (&pim_protocol);
#endif
  rtnl_unlock ();
  return 0;
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
  struct mfcctl mfc;
  int ret;

  memset (&mfc, 0, sizeof (struct mfcctl));

  mfc.mfcc_origin.s_addr = src;
  mfc.mfcc_mcastgrp.s_addr = group;
  mfc.mfcc_parent = (vifi_t)iif_vif;
  memcpy (mfc.mfcc_ttls, olist_ttls, MAXVIFS * sizeof (u_char));

  rtnl_lock ();
  ret = ipmr_mfc_add (&mfc, 1);
  rtnl_unlock ();

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
  struct mfcctl mfc;
  int ret;

  memset (&mfc, 0, sizeof (struct mfcctl));

  mfc.mfcc_origin.s_addr = src;
  mfc.mfcc_mcastgrp.s_addr = group;

  rtnl_lock ();
  ret = ipmr_mfc_delete (&mfc);
  rtnl_unlock ();

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
  struct vifctl vif;
  int ret;

  memset (&vif, 0, sizeof (struct vifctl));

  vif.vifc_vifi = (vifi_t)vif_index;
  /* XX: Threshold is 1 for now */
  vif.vifc_threshold = 1;
  /* XXX: Rate limit is 0  */
  vif.vifc_rate_limit = 0;
  vif.vifc_lcl_addr.s_addr = loc_addr;
  vif.vifc_rmt_addr.s_addr = rmt_addr;
  if (flags & HSL_VIFF_REGISTER)
    vif.vifc_flags |= VIFF_REGISTER;
  if (flags & HSL_VIFF_TUNNEL)
    vif.vifc_flags |= VIFF_TUNNEL;

  rtnl_lock ();
  ret = vif_add (&vif, 1);
  rtnl_unlock ();

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
  struct vifctl vif;
  int ret;

  memset (&vif, 0, sizeof (struct vifctl));

  vif.vifc_vifi = (vifi_t)vif_index;

  rtnl_lock ();
  ret = vif_delete (vif.vifc_vifi);
  rtnl_unlock ();

  return ret;
}
#endif /* HAVE_MCAST_IPV4 */


#ifdef HAVE_MCAST_IPV6
/* Enable pim

Returns:
0 on success
< 0 on error
*/
int
hsl_os_ipv6_mc_pim_init ()
{
  return 0;
}

/* Disable pim

Returns:
0 on success
< 0 on error
*/
int
hsl_os_ipv6_mc_pim_deinit ()
{
  return 0;
}

/* Add ipv6 multicast route
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
hsl_os_ipv6_mc_route_add (hsl_ipv6Address_t *src,hsl_ipv6Address_t *group,
                          u_int32_t iif_mif,u_int32_t num_olist,
                          u_int16_t *olist)
{
  return 0;
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
hsl_os_ipv6_mc_route_del (hsl_ipv6Address_t src,hsl_ipv6Address_t group)
{
  return 0;
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
hsl_os_ipv6_mc_vif_add (u_int32_t vif_index, hsl_ipv6Address_t loc_addr,
                        hsl_ipv6Address_t rmt_addr, u_int16_t flags)
{
  return 0;
}

/* Delete vif from os.

Parameters:
IN -> vif_index  - mcast interface index.

Returns:
0 on success
< 0 on error
*/
int
hsl_os_ipv6_mc_vif_del (u_int32_t vif_index)
{
  return 0;
}
#endif /* HAVE_MCAST_IPV6 */


/* 
   Initialize HSL FIB OS callbacks.
*/
int
hsl_fib_os_cb_register (void)
{
  HSL_FN_ENTER ();
  OS_FIBCB(os_prefix_add)       = hsl_os_l3_prefix_add;
  OS_FIBCB(os_prefix_add_if)    = hsl_os_l3_prefix_add_if;
  OS_FIBCB(os_prefix_delete)    = hsl_os_l3_prefix_delete;
  OS_FIBCB(os_prefix_delete_if) = hsl_os_l3_prefix_delete_if;
  OS_FIBCB(os_nh_add)           = hsl_os_l3_nh_add;
  OS_FIBCB(os_nh_delete)        = hsl_os_l3_nh_delete;
  /* Register callbacks with interface manager. */
  hsl_fibmgr_os_cb_register(&hsl_linux_fib_os_callbacks);

  HSL_FN_EXIT (0);
}

#ifdef HAVE_MCAST_IPV4
int hsl_ipv4_mc_os_cb_register(void)
{
  HSL_FN_ENTER();

  OS_MCCB(os_ipv4_mc_pim_init)   = hsl_os_ipv4_mc_pim_init;
  OS_MCCB(os_ipv4_mc_pim_deinit) = hsl_os_ipv4_mc_pim_deinit;
  OS_MCCB(os_ipv4_mc_route_add)  = hsl_os_ipv4_mc_route_add;
  OS_MCCB(os_ipv4_mc_route_del)  = hsl_os_ipv4_mc_route_del;
  OS_MCCB(os_ipv4_mc_vif_add)    = hsl_os_ipv4_mc_vif_add;
  OS_MCCB(os_ipv4_mc_vif_del)    = hsl_os_ipv4_mc_vif_del;

  /* Register callbacks with fib multicast manager. */
  hsl_ipv4_mc_os_cb_reg(&hsl_linux_mc_os_callbacks);

  HSL_FN_EXIT(0);
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
int hsl_ipv6_mc_os_cb_register(void)
{
  HSL_FN_ENTER();
  memset (&hsl_linux_ipv6_mc_os_callbacks , 0, sizeof (struct hsl_fib_ipv6_mc_os_callbacks));

  OS_IPV6_MCCB(os_ipv6_mc_pim_init)   = hsl_os_ipv6_mc_pim_init;
  OS_IPV6_MCCB(os_ipv6_mc_pim_deinit) = hsl_os_ipv6_mc_pim_deinit;
  OS_IPV6_MCCB(os_ipv6_mc_route_add)  = hsl_os_ipv6_mc_route_add;
  OS_IPV6_MCCB(os_ipv6_mc_route_del)  = hsl_os_ipv6_mc_route_del;
  OS_IPV6_MCCB(os_ipv6_mc_vif_add)    = hsl_os_ipv6_mc_vif_add;
  OS_IPV6_MCCB(os_ipv6_mc_vif_del)    = hsl_os_ipv6_mc_vif_del;

  /* Register callbacks with fib multicast manager. */
  hsl_ipv6_mc_os_cb_reg (&hsl_linux_ipv6_mc_os_callbacks);

  HSL_FN_EXIT(STATUS_OK);
}

#endif /* HAVE_MCAST_IPV6 */

