/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "hal_acl.h"
#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

extern int hal_comm_initialized;

/* 
   Callback for HAL_MSG_IF_NEWLINK.
*/
int
hal_if_newlink (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_if resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  if ((h->nlmsg_type != HAL_MSG_IF_NEWLINK) &&
      (h->nlmsg_type != HAL_MSG_IF_UPDATE))
    return -1;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_if (&pnt, &size, &resp);
  if (ret < 0)
    return ret;

  hal_new_link_cb (h, &resp);

  return 0;
}

/*
  Callback for HAL_MSG_IF_DELLINK.
*/
int
hal_if_dellink (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_if resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  if (h->nlmsg_type != HAL_MSG_IF_DELLINK)
    return -1;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_if (&pnt, &size, &resp);
  if (ret < 0)
    return ret;

  hal_del_link_cb (&resp);

  return 0;
}
#ifdef HAVE_L2
/* 
   Callback for HAL_MSG_IF_STP_REFRESH.
*/
int 
hal_if_stp_refresh(struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_if resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  if (h->nlmsg_type != HAL_MSG_IF_STP_REFRESH)
    return -1;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_if (&pnt, &size, &resp);
  if (ret < 0)
    return ret;

  hal_stp_refresh_link_cb (&resp);

  return 0;
}
#endif /* HAVE_L2 */

#ifdef HAVE_L3
/* Common function for FIB If bind/unbind. */
static int
_hal_fib_if_msg (unsigned int ifindex, unsigned int fib, int msgtype)
{
  int ret;
  u_char *pnt;
  int size, msgsz;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_fib_bind_unbind bind;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_if_fib_bind_unbind msg;
  } req;

  pal_mem_set (&req, 0, sizeof (req));

  bind.ifindex = ifindex;
  bind.fib_id = fib;

  pnt = (u_char *) &req.msg;
  size = sizeof (struct hal_msg_if_fib_bind_unbind);

  msgsz = hal_msg_encode_if_fib_bind_unbind (&pnt, (u_int32_t *)&size, &bind);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_if_bind_fib

   Description:
   This API is called to bind a interface to a FIB fib_id in the forwarding 
   plane. 

   Parameters:
   IN -> ifindex - ifindex of interface
   IN -> fib     - fib id

   Results:
   HAL_SUCCESS
   < 0 on error
*/
int
hal_if_bind_fib (u_int32_t ifindex, u_int32_t fib)
{
  return _hal_fib_if_msg (ifindex, fib, HAL_MSG_IF_FIB_BIND);
}

/* 
   Name: hal_if_unbind_fib

   Description:
   This API is called to unbind an interface from  FIB fib_id in the forwarding 
   plane. 

   Parameters:
   IN -> ifindex - ifindex of interface
   IN -> fib     - fib id

   Results:
   HAL_SUCCESS
   < 0 on error
*/
int
hal_if_unbind_fib (u_int32_t ifindex, u_int32_t fib)
{
  return _hal_fib_if_msg (ifindex, fib, HAL_MSG_IF_FIB_UNBIND);
}

/*
  Callback for HAL_MSG_IF_IPV4_DELADDR & HAL_MSG_IF_IPV4_NEWADDR.
*/
int
hal_if_ipv4_addr(struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_if_ipv4_addr resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  if ((h->nlmsg_type != HAL_MSG_IF_IPV4_DELADDR) && 
      (h->nlmsg_type != HAL_MSG_IF_IPV4_NEWADDR)) 
    return -1;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_ipv4_addr(&pnt, &size, &resp);
  if (ret < 0)
  { 
    return ret;
  } 

  if(h->nlmsg_type == HAL_MSG_IF_IPV4_DELADDR)
    hal_del_ipv4_addr(&resp);

  else if(h->nlmsg_type == HAL_MSG_IF_IPV4_NEWADDR)
    hal_add_ipv4_addr(&resp);

  return 0;
}
/* 
   Callback for HAL_MSG_GET_MAX_MULTIPATH
*/
int
hal_rx_max_multipath(struct hal_nlmsghdr *h, void *data)
{
  u_int32_t ecmp;
  u_char *pnt;
  u_int32_t size;

  if (h->nlmsg_type != HAL_MSG_GET_MAX_MULTIPATH) 
    return -1;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  if(size < sizeof(u_int32_t))
     return -1; 

  ecmp = *((u_int32_t *)pnt);

  hal_ecmp_cb(ecmp);

  return 0;
}

int
hal_if_set_l3_enable_status (int l3_status)
{
  int ret = HAL_SUCCESS;
  struct
    {
      struct hal_nlmsghdr nlh;
      struct hal_msg_if_l3_status msg;
    } req;

  if (! hal_comm_initialized) /*To skip startup ipv4 fwd status set*/
    return HAL_SUCCESS;

  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_if_l3_status));
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  
  /* Set message. */
  req.msg.status = l3_status;
  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_l3_status));
  req.nlh.nlmsg_type = HAL_MSG_IF_L3_STATUS;
  req.nlh.nlmsg_flags = HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


#ifdef HAVE_IPV6

int
hal_if_set_ipv6_l3_enable_status (int l3_status)
{
  int ret = HAL_SUCCESS;
  struct
    {
      struct hal_nlmsghdr nlh;
      struct hal_msg_if_l3_status msg;
    } req;

  if (! hal_comm_initialized) /*To skip startup ipv4 fwd status set*/
    return HAL_SUCCESS;

  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_if_l3_status));
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  
  /* Set message. */
  req.msg.status = l3_status;
  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_l3_status));
  req.nlh.nlmsg_type = HAL_MSG_IF_IPV6_L3_STATUS;
  req.nlh.nlmsg_flags = HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


/*
  Callback for HAL_MSG_IF_IPV6_DELADDR & HAL_MSG_IF_IPV6_NEWADDR.
*/
int
hal_if_ipv6_addr(struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_if_ipv6_addr resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  if ((h->nlmsg_type != HAL_MSG_IF_IPV6_DELADDR) && 
      (h->nlmsg_type != HAL_MSG_IF_IPV6_NEWADDR)) 
    return -1;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_ipv6_addr(&pnt, &size, &resp);
  if (ret < 0)
  {
    return ret;
  }

  if(h->nlmsg_type == HAL_MSG_IF_IPV6_DELADDR)
    hal_del_ipv6_addr(&resp);

  else if(h->nlmsg_type == HAL_MSG_IF_IPV6_NEWADDR)
    hal_add_ipv6_addr(&resp);

  return 0;
}
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3   */

/* 
   Name: hal_if_get_list

   Description:
   This API gets the list of interfaces from the interface manager.

   Parameters:
   None

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_list (void)
{
  int ret;

  /* Request list of interfaces. */
  ret = hal_msg_generic_poll_request (HAL_MSG_IF_GETLINK, hal_if_newlink, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Response callback for if message. 
*/
static int
_hal_if_cb (struct hal_nlmsghdr *h, void *data)
{
  u_char *pnt;
  u_int32_t size;
  int ret;
  struct hal_msg_if *msg = (struct hal_msg_if *) data;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - HAL_NLMSGHDR_SIZE;

  ret = hal_msg_decode_if (&pnt, &size, msg);
  if (ret < 0)
    return ret;

  /* Call NSM callback. */
  hal_new_link_cb (h, msg);

  return 0;
}

/* 
   Response callback for if message. 
*/
static int
_hal_if_get_cb (struct hal_nlmsghdr *h, void *data)
{
  u_char *pnt;
  u_int32_t size;
  int ret;
  struct hal_msg_if *msg = (struct hal_msg_if *) data;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - HAL_NLMSGHDR_SIZE;

  ret = hal_msg_decode_if (&pnt, &size, msg);
  if (ret < 0)
    return ret;

  /* Call NSM callback. */
  hal_get_params_cb (h, msg);
  
  return 0;
}

/*
  Generic if attribute get function.
*/
static int
_hal_if_attr_get (char *ifname, u_int16_t ifindex, int command, struct hal_msg_if *resp)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if ifp;
  u_char *pnt;
  int size, msgsz;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[512];
  } req;

  /* Set message. */
  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_if (&pnt, (u_int32_t *)&size, &ifp);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Request HWADDR. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_if_get_cb, (void *)resp);
  if (ret < 0)
    return ret;

  return 0;
}

/*
  Generic attribute set function.
*/
static int
_hal_if_attr_set (struct hal_msg_if *ifp, int command)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  int size, msgsz;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[512];
  } req;

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_if (&pnt, (u_int32_t *)&size, ifp);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = command; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return 0;
}

/* 
   Name: hal_if_get_metric 

   Description: 
   This API gets the metric for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   OUT -> metric - metric

   Returns:
   < 0 on error 
   HAL_SUCCESS
*/
int
hal_if_get_metric (char *ifname, unsigned int ifindex, int *metric)
{
  struct hal_msg_if ifp;
  int ret;

  ret = _hal_if_attr_get (ifname, ifindex, HAL_MSG_IF_GET_METRIC, &ifp);
  if (ret < 0)
    return ret;
  
  *metric = ifp.metric;

  return HAL_SUCCESS;
}

/* 
   Name: hal_if_get_mtu 

   Description:
   This API get the mtu for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   OUT -> mtu - mtu

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_mtu (char *ifname, unsigned int ifindex, int *mtu)
{
  struct hal_msg_if ifp;
  int ret;

  ret = _hal_if_attr_get (ifname, ifindex, HAL_MSG_IF_GET_MTU, &ifp);
  if (ret < 0)
    return ret;
  
  *mtu = ifp.mtu;

  return HAL_SUCCESS;
}

/* 
   Name: hal_if_set_mtu

   Description:
   This API set the MTU for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> mtu - mtu

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_mtu (char *ifname, unsigned int ifindex, int mtu)
{
  struct hal_msg_if ifp;
  int ret;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  ifp.mtu = mtu;
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_MTU);

  ret = _hal_if_attr_set (&ifp, HAL_MSG_IF_SET_MTU);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_if_get_arp_ageing_timeout

   Description:
   This API set the arp ageing timeout for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> arp_ageing_timeout - arp ageing timeout value

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_arp_ageing_timeout (char *ifname, unsigned int ifindex, int arp_ageing_timeout)
{
  struct hal_msg_if ifp;
  int ret;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  ifp.arp_ageing_timeout = arp_ageing_timeout;
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT);

  ret = _hal_if_attr_set (&ifp, HAL_MSG_IF_SET_ARP_AGEING_TIMEOUT);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_if_get_arp_ageing_timeout

   Description:
   This API get the arp ageing timeout for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   OUT -> arp_ageing_timeout - arp ageing timeout value

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_arp_ageing_timeout (char *ifname, unsigned int ifindex, int *arp_ageing_timeout)
{
  struct hal_msg_if ifp;
  int ret;

  ret = _hal_if_attr_get (ifname, ifindex, HAL_MSG_IF_GET_ARP_AGEING_TIMEOUT, &ifp);
  if (ret < 0)
    return ret;
 
  *arp_ageing_timeout = ifp.arp_ageing_timeout;

  return HAL_SUCCESS;
}


/* 
   Name: hal_if_get_duplex 

   Description:
   This API get the duplex for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   OUT -> duplex - duplex

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_get_duplex (char *ifname, unsigned int ifindex, int *duplex)
{
  struct hal_msg_if ifp;
  int ret;

  ret = _hal_if_attr_get (ifname, ifindex, HAL_MSG_IF_GET_DUPLEX, &ifp);
  if (ret < 0)
    return ret;
  
  *duplex = ifp.duplex;

  return HAL_SUCCESS;
}

/*
   Name: hal_if_set_duplex

   Description:
   This API set the DUPLEX for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> duplex - duplex 

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_duplex (char *ifname, unsigned int ifindex, int duplex)
{
  struct hal_msg_if ifp;
  int ret;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  ifp.duplex = duplex;
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_DUPLEX);

  ret = _hal_if_attr_set (&ifp, HAL_MSG_IF_SET_DUPLEX);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_if_set_autonego

   Description:
   This API set the DUPLEX with auto-negotiate for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> autonego - autonego 

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_autonego (char *ifname, unsigned int ifindex, int autonego)
{
  struct hal_msg_if ifp;
  int ret;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  ifp.autonego = autonego;
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_AUTONEGO);

  ret = _hal_if_attr_set (&ifp, HAL_MSG_IF_SET_AUTONEGO);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
  Name: hal_if_get_hwaddr

  Description:
  This API gets the hardware address for a interface. This is the MAC 
  address in case of ethernet. The caller has to provide a buffer large
  enough to hold the address.

  Parameters:
  IN -> ifname - interface name
  IN -> ifindex - interface ifindex
  OUT -> hwaddr - hardware address
  OUT -> hwaddr_len - hardware address length

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_get_hwaddr (char *ifname, unsigned int ifindex, 
                   u_char *hwaddr, int *hwaddr_len)
{
  struct hal_msg_if ifp;
  int ret;

  ret = _hal_if_attr_get (ifname, ifindex, HAL_MSG_IF_GET_HWADDR, &ifp);
  if (ret < 0)
    return ret;

  *hwaddr_len = ifp.hw_addr_len;
  memcpy (hwaddr, ifp.hw_addr, ifp.hw_addr_len);

  return HAL_SUCCESS;
}

/*
  Name: hal_if_set_hwaddr

  Description:
  This API sets the hardware address for a interface. This is the MAC 
  address in case of ethernet.

  Parameters:
  IN -> ifname - interface name
  IN -> ifindex - interface ifindex
  IN -> hwaddr - hardware address
  IN -> hwlen - hardware address length

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_set_hwaddr (char *ifname, unsigned int ifindex,
                   u_int8_t *hwaddr, int hwlen)
{
  struct hal_msg_if ifp;
  int ret;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex     = ifindex;
  ifp.hw_addr_len = hwlen;
  pal_mem_cpy (ifp.hw_addr, hwaddr, hwlen);
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_HW);

  ret = _hal_if_attr_set (&ifp, HAL_MSG_IF_SET_HWADDR);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

static int
_hal_if_sec_hwaddrs_list (char *ifname, unsigned int ifindex, 
                          int hw_addr_len, int nAddrs, unsigned char **addresses, int msg)
{
  struct hal_msg_if ifp;
  int ret;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex     = ifindex;
  ifp.sec_hw_addr_len = hw_addr_len;
  ifp.nAddrs      = nAddrs;
  ifp.addresses   = addresses;
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_SEC_HW_ADDRESSES);

  ret = _hal_if_attr_set (&ifp, msg);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
  Name: hal_if_sec_hwaddrs_set

  Description:
  This API sets the list of secondary MAC addresses for a 
  interface.

  Parameters:
  IN -> ifname  - interface name
  IN -> ifindex - interface index
  IN -> hw_addr_len - length of address
  IN -> nAddr s - number of MAC addresses
  IN -> addresses - array of MAC addresses

  Returns:
  < 0 on error
  HAL_SUCCESS;
*/
int
hal_if_sec_hwaddrs_set (char *ifname, unsigned int ifindex,
                        int hw_addr_len, int nAddrs, unsigned char **addresses)
{
  int ret;

  ret = _hal_if_sec_hwaddrs_list (ifname, ifindex, 
                                  hw_addr_len, nAddrs, addresses, 
                                  HAL_MSG_IF_SET_SEC_HWADDRS);

  return ret;
}
  
/*
  Name: hal_if_sec_hwaddrs_add

  Description:
  This API adds the secondary hardware addresses to the list of MAC addresses
  for a interface.

  Parameters:
  IN -> ifname  - interface name
  IN -> ifindex - interface index
  IN -> hw_addr_len - length of address
  IN -> nAddr s - number of MAC addresses
  IN -> addresses - array of MAC addresses

  Returns:
  < 0 on error
  HAL_SUCCESS;
*/
int
hal_if_sec_hwaddrs_add (char *ifname, unsigned int ifindex,
                        int hw_addr_len, int nAddrs, unsigned char **addresses)
{
  int ret;

  ret = _hal_if_sec_hwaddrs_list (ifname, ifindex, 
                                  hw_addr_len, nAddrs, addresses, 
                                  HAL_MSG_IF_ADD_SEC_HWADDRS);

  return ret;
}

/*
  Name: hal_if_sec_hwaddrs_delete

  Description:
  This API deletes the secondary hardware addresses from the
  list of receive MAC addresses for a interface.

  Parameters:
  IN -> ifname  - interface name
  IN -> ifindex - interface index
  IN -> hw_addr_len - length of address
  IN -> nAddr s - number of MAC addresses
  IN -> addresses - array of MAC addresses

  Returns:
  < 0 on error
  HAL_SUCCESS;
*/
int
hal_if_sec_hwaddrs_delete (char *ifname, unsigned int ifindex,
                            int hw_addr_len, int nAddrs, unsigned char **addresses)
{
  int ret;

  ret = _hal_if_sec_hwaddrs_list (ifname, ifindex, 
                                  hw_addr_len, nAddrs, addresses, 
                                  HAL_MSG_IF_DELETE_SEC_HWADDRS);

  return ret;
}
  
/* 
   Name: hal_if_flags_get

   Description:
   This API gets the flags for a interface. The flags are
   IFF_RUNNING
   IFF_UP
   IFF_BROADCAST
   IFF_LOOPBACK

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   OUT -> flags - flags
   
   Returns:
   < 0 on error
   HAL_SUCCES
*/
int
hal_if_flags_get (char *ifname, unsigned int ifindex, u_int32_t *flags)
{
  struct hal_msg_if ifp;
  int ret;

  ret = _hal_if_attr_get (ifname, ifindex, HAL_MSG_IF_GET_FLAGS, &ifp);
  if (ret < 0)
    return ret;

  *flags = ifp.flags;

  return HAL_SUCCESS;
}

/* 
   Common function for setting/unsetting flags.
*/
static int
_hal_if_flags (char *ifname, u_int16_t ifindex, u_int16_t flags, int command)
{
  int ret;
  struct hal_msg_if ifp;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  ifp.flags = flags;
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_FLAGS);

  ret = _hal_if_attr_set (&ifp, command);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
  Name: hal_if_flags_set
  
  Description:
  This API sets the flags for a interface. The flags are
  IFF_RUNNING
  IFF_UP
  IFF_BROADCAST
  IFF_LOOPBACK
  
   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> flags - flags to set

   Returns:
   < 0 for error
   HAL_SUCCESS
*/
int
hal_if_flags_set (char *ifname, unsigned int ifindex, unsigned int flags)
{
  return _hal_if_flags (ifname, ifindex, flags, HAL_MSG_IF_SET_FLAGS);
}

/* 
   Name: hal_if_flags_unset

   Description:
   This API unsets the flags for a interface. The flags are
   IFF_RUNNING
   IFF_UP
   IFF_BROADCAST
   IFF_LOOPBACK
   
   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> flags - flags to unset

   Returns:
   < 0 for error
   HAL_SUCCESS
*/
int
hal_if_flags_unset (char *ifname, unsigned int ifindex, unsigned int flags)
{
  return _hal_if_flags (ifname, ifindex, flags, HAL_MSG_IF_UNSET_FLAGS);
}

/* 
   Name: hal_if_get_bw

   Description:
   This API gets the bandwidth for the interface. This API should
   return the value in bytes per second.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   OUT -> bandwidth - interface bandwidth

   Returns:
   < 0 for error
   HAL_SUCCESS
*/
int
hal_if_get_bw (char *ifname, unsigned int ifindex, u_int32_t *bandwidth)
{
  struct hal_msg_if ifp;
  int ret;

  ret = _hal_if_attr_get (ifname, ifindex, HAL_MSG_IF_GET_BW, &ifp);
  if (ret < 0)
    return ret;

  *bandwidth = ifp.bandwidth;

  return HAL_SUCCESS;
}

/*
   Name: hal_if_set_bw

   Description:
   This API set the bandwidth for a interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface index
   IN -> bandwidth - bandwidth

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_bw (char *ifname, unsigned int ifindex, unsigned int bandwidth)
{
  struct hal_msg_if ifp;
  int ret;

  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));

  /* Set message. */
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  ifp.bandwidth = bandwidth;
  SET_CINDEX (ifp.cindex, HAL_MSG_CINDEX_IF_BANDWIDTH);

  ret = _hal_if_attr_set (&ifp, HAL_MSG_IF_SET_BW);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


#if 0
/* Commenting since not used. */
/* Response callback for l3 if create/delete. */
static int
_hal_if_l3_cb (struct hal_nlmsghdr *h, void *data)
{
  unsigned int *ifindex = (int *)data;
  u_int16_t *resp;

  resp = HAL_NLMSG_DATA (h);

  /* Copy response. */
  *ifindex = *resp;

  return 0;
}
#endif

/* 
   Inerface clean up done notification. 
 */
int
hal_if_delete_done(char *ifname, u_int16_t ifindex)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if ifp;
  u_char *pnt;
  int size, msgsz;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[512];
  } req;

  /* Set message. */
  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));
  pal_strcpy (ifp.name, ifname);
  ifp.ifindex = ifindex;
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_if (&pnt, (u_int32_t *)&size, &ifp);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IF_CLEANUP_DONE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Request HWADDR. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_if_get_cb, NULL);
  if (ret < 0)
    return ret;

  return 0;
}

#ifdef HAVE_L2
/*
  Name: hal_if_set_port_type 

  Description:
  This API set the port type i.e. ROUTER or SWITCH port for a interface.

  Parameters:
  IN -> name - name of the interface
  IN -> ifindex - ifindex
  IN -> type - the port type
  OUT -> retifindex - the ifindex of the new type of interface

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_set_port_type (char *name, unsigned int ifindex, 
                      enum hal_if_port_type type, unsigned int *retifindex)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_port_type *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_if_port_type msg;
  } req;
  int ret;
  struct hal_msg_if resp;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_port_type));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_SET_PORT_TYPE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, name);
  msg->ifindex = ifindex;
  
  if (type == HAL_IF_SWITCH_PORT)
    msg->type = HAL_MSG_SET_SWITCHPORT;
  else if (type == HAL_IF_ROUTER_PORT)
    msg->type = HAL_MSG_SET_ROUTERPORT;
  else
    return -1;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_if_cb, &resp);
  if (ret < 0)
    return ret;

  if (retifindex)
    *retifindex = resp.ifindex;

  return HAL_SUCCESS;
}

/*
  Name: hal_if_svi_create

  Description:
  This API creates a SVI(Switch Virtual Interface) for a specific VLAN. The
  the VLAN information is embedded in the name of the interface.

  Parameters:
  IN -> name - interface name
  OUT -> retifindex - ifindex for the interface

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_svi_create (char *name, unsigned int *retifindex)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_svi *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_svi msg;
  } req;
  int ret;
  
  pal_mem_set(&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set(&req.msg, 0, sizeof(struct hal_msg_svi));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_svi));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_CREATE_SVI;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
  Name: hal_if_svi_delete

  Description:
  This API deletes the SVI(Switch Virtual Interface) for a specific VLAN. 

  Parameters:
  IN -> name - interface name
  IN -> ifindex - ifindex

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_svi_delete (char *name, unsigned int ifindex)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_svi *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_svi msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_svi));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_DELETE_SVI;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, name);
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}
#endif /* HAVE_L2 */

/* structure for returning interface statistics. */
struct _ifstat_resp
{
  struct hal_if_counters *if_stats;
};

/* 
   Callback function for response of interface stat get. 
*/
static int
_hal_if_stat_resp(struct hal_nlmsghdr *h, void *data)
{
  struct _ifstat_resp *retresp = (struct _ifstat_resp*) data;
  struct hal_msg_if_stat *resp;

  resp = HAL_NLMSG_DATA (h);
  pal_mem_cpy (retresp->if_stats, &resp->cntrs,  sizeof (struct hal_if_counters));
  return 0;
}

/* Callback function for response of Learn Disable  */

static int
_hal_if_learn_disable (struct hal_nlmsghdr *h, void *data)
{
   int *enable = (int *)data;
   struct hal_msg_if_learn_disable *resp;

   resp = HAL_NLMSG_DATA (h);
   pal_mem_cpy (enable, &(resp->enable), sizeof(int));

   return HAL_SUCCESS;
}

/* 
   Name: _hal_if_get_counters

   Description: This API gets interface statistics for specific  
   ifindex.

   Parameters:
   IN -> ifindex - Interface index.
   IN -> command - Counters type. 
   OUT ->if_stats - the array of counters for interface. 

   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/

static int
_hal_if_get_counters(unsigned int ifindex, int command, struct hal_if_counters *if_stats)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_stat *msg;
  struct 
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_stat msg;
  } req;
  int ret;
  struct _ifstat_resp resp; 

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_stat));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;

  /* Set response. */
  resp.if_stats = if_stats; /* Call will fill this. */

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh,_hal_if_stat_resp , &resp);

  return ret;
}

/* 
   Name: hal_if_get_counters

   Description: This API gets interface statistics for specific  
   ifindex.

   Parameters:
   IN -> ifindex - Interface index.
   OUT ->if_stats - the array of counters for interface. 

   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_get_counters(unsigned int ifindex, struct hal_if_counters *if_stats)
{
  return _hal_if_get_counters(ifindex,HAL_MSG_IF_COUNTERS_GET, if_stats);
}

/* 
   Name: _hal_if_clear_counters
 
   Description: This API clears interface statistics for specific  
   ifindex.
 
   Parameters:
   IN -> ifindex - Interface index.
   IN -> command - Counters type. 
 
   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/
 
static int
_hal_if_clear_counters(unsigned int ifindex, int command)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_clear_stat *msg;
  int ret;
 
  struct 
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_clear_stat msg;
  } req;
 
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_clear_stat));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
 
  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
 
  return ret;
}
 
/* 
   Name: hal_if_clear_counters
 
   Description: This API clears interface statistics for specific  
   ifindex.
 
   Parameters:
   IN -> ifindex - Interface index.
 
   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_if_clear_counters(unsigned int ifindex)
{
  return _hal_if_clear_counters(ifindex, HAL_MSG_IF_CLEAR_COUNTERS);
}
 

int
hal_vlan_if_get_counters(unsigned int ifindex,unsigned int vlan, 
                         struct hal_vlan_if_counters *if_stats)
{
  return 0;
}

/*
   Name: hal_if_set_mdix   
    
   Description: This API sets MDIX crossover for specified ifindex.
  
   Parameters:
   IN -> ifindex - Interface index.
   IN->  mdix - MDIX crossover for an  interface.
  
   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/

int
hal_if_set_mdix(unsigned int ifindex, unsigned int mdix)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_mdix *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_mdix msg;
  } req;
  int ret;

  int command = HAL_MSG_IF_SET_MDIX;
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_mdix));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->mdix = mdix;
  
 /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  
  return ret;
}   
  
/*
   Name: hal_if_set_portbased_vlan
  
   Description: Adding/Removing  members to portbased vlan.
  
   Parameters:
   IN -> ifindex - Interface index.
   IN -> pbitmap - Bit map for portbased vlan members
   IN -> status  - Add/Delete status
   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/

int
hal_if_set_portbased_vlan (unsigned int ifindex, struct hal_port_map pbitmap)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_portbased_vlan *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_portbased_vlan msg;
  } req;
  int ret;

  int command = HAL_MSG_IF_PORTBASED_VLAN;
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_portbased_vlan));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex; 
  msg->pbitmap = pbitmap;
  
 /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  
  return ret;
}   
  
int
hal_if_set_cpu_default_vid(unsigned int ifindex, int vid)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_cpu_default_vid *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_cpu_default_vid msg;
  } req;
  int ret;

  int command = HAL_MSG_IF_CPU_DEFAULT_VLAN;
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len =
                HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_cpu_default_vid));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->vid     = vid;
  
 /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
    
  return ret;
} 
  
int
hal_if_set_wayside_default_vid(unsigned int ifindex, int vid)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_cpu_default_vid *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_cpu_default_vid msg;
  } req;
  int ret;
  
  int command = HAL_MSG_IF_WAYSIDE_DEFAULT_VLAN;
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len =
                HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_cpu_default_vid));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->vid     = vid;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}

int
hal_if_set_preserve_ce_cos (unsigned int ifindex)
{
  struct hal_nlmsghdr *nlh;
  struct hal_port *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_port  msg;
  } req;
  int ret;
  
  int command = HAL_MSG_IF_PRESERVE_CE_COS;
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len =
                HAL_NLMSG_LENGTH (sizeof (struct hal_port));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}


/*
   Name: hal_if_set_port_egress

   Description: setting port egress type.

   Parameters:
   IN -> ifindex - Interface index.
   IN -> egresse - Port egress mode
   Returns:
  Returns:
  < 0 on error
  HAL_SUCCESS
*/

int 
hal_if_set_port_egress (unsigned int ifindex, int egress)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_port_egress *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_port_egress msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_port_egress));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_PORT_EGRESS;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->egress = egress;

 /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}


/*
   Name: hal_if_set_force_vlan

   Description: setting port vlan.

   Parameters:
   IN -> ifindex - Interface index.
   IN -> vid     - Port Vlan id.

   Returns:
   < 0 on error
   HAL_SUCCESS 
*/

int
hal_if_set_force_vlan (unsigned int ifindex, int vid)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_force_vlan *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_force_vlan msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_force_vlan));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_SET_FORCE_VLAN;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->vid = vid;

 /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}


/*
   Name: hal_if_set_ether_type

   Description: setting port ether type.

   Parameters:
   IN -> ifindex   - Interface index.
   IN -> ethertype - EtherType

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_ether_type (unsigned int ifindex, u_int16_t etype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_ether_type *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_ether_type msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_ether_type));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_SET_ETHER_TYPE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->etype = etype; 

 /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  
  return ret;
}   
    
/*
   Name: hal_if_set_sw_reset

   Description: Reset HSL Driver
  
   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_if_set_sw_reset ()
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_ether_type *msg;

  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_ether_type msg;
  } req;

  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_ether_type));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_SET_SW_RESET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  msg = &req.msg;

  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
} 
  
/*
   Name: hal_if_set_learn_disable
  
   Description: setting learn disable.
    
   Parameters:
   IN -> ifindex   - Interface index.
   IN -> enable    - Enable/Disable 
  
   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_set_learn_disable (unsigned int ifindex, int enable)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_learn_disable *msg;
  struct
  {
    struct hal_nlmsghdr    nlh;
    struct hal_msg_if_learn_disable msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_learn_disable));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_SET_LEARN_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->enable = enable;

 /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  
  return ret;
}   
    
/*
   Name: hal_if_get_learn_disable

   Description:  get  learn disable status.
  
   Parameters:
   IN -> ifindex   - Interface index. 
   IN -> enable    - Enable/Disable
  
   Returns:
   < 0 on error
   HAL_SUCCESS
*/

int
hal_if_get_learn_disable (unsigned int ifindex, int* enable)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_learn_disable *msg;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_if_learn_disable msg;
  } req;

  int learnDisable;

  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_learn_disable));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_IF_GET_LEARN_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  msg = &req.msg;
  msg->ifindex = ifindex;  
 
  ret = hal_talk (&hallink_cmd, nlh, _hal_if_learn_disable, &learnDisable);
  *enable = learnDisable; 
  
  return ret;
} 

/**************************************************************
 * Function for Flushing the Counters on a particular interface
**************************************************************/

int
hal_if_stats_flush (u_int16_t ifindex)
{

  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if ifp;
  u_char *pnt;
  int msgsz;
  u_int32_t size;
  struct
  { 
    struct hal_nlmsghdr nlh;
    u_char buf[512];
  } req;
  
  /* Set message. */ 
  pal_mem_set (&ifp, 0, sizeof (struct hal_msg_if));
  ifp.ifindex = ifindex;
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_if (&pnt, &size, &ifp);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IF_STATS_FLUSH;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Request HWADDR. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return 0;

}
  
int
hal_set_acl_for_access_group (struct access_list *access,
                              struct hal_ip_access_grp *access_grp)
{
  struct filter_list *mfilter;
  int i = 0;

  pal_mem_set(access_grp, 0, sizeof(struct hal_ip_access_grp));

  for (mfilter = access->head; mfilter; mfilter = mfilter->next)
    {
        access_grp->hfilter[i].type
                               = mfilter->type;
        access_grp->hfilter[i].ace.ipfilter.addr
                               = mfilter->u.cfilter.addr.s_addr;
        access_grp->hfilter[i].ace.ipfilter.addr_mask
                               = mfilter->u.cfilter.addr_mask.s_addr;
        access_grp->hfilter[i].ace.ipfilter.mask
                               = mfilter->u.cfilter.mask.s_addr;
        access_grp->hfilter[i].ace.ipfilter.mask_mask
                               = mfilter->u.cfilter.mask_mask.s_addr;
        i++;
        if (i >= HAL_IP_MAX_ACL_FILTER)
          break;
    }
  access_grp->ace_number = i;
  return 0;
}

int
hal_ip_set_access_group (struct hal_ip_access_grp access_grp,
                         char *ifname, int action, int dir)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_ip_set_access_grp *msg;
  struct
    {
      struct hal_nlmsghdr nlh;
      struct hal_msg_ip_set_access_grp msg;
    } req;
  int ret;

  if ((action != HAL_ACL_ACTION_ATTACH) && (action != HAL_ACL_ACTION_DETACH))
    return -1;

  if ((dir != HAL_ACL_DIRECTION_INGRESS) && (dir != HAL_ACL_DIRECTION_EGRESS))
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof
                   (struct hal_msg_ip_set_access_grp));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  if (action == HAL_ACL_ACTION_ATTACH)
    nlh->nlmsg_type = HAL_MSG_IP_SET_ACCESS_GROUP;
  else
    nlh->nlmsg_type = HAL_MSG_IP_UNSET_ACCESS_GROUP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifname = ifname;
  msg->action = action; /* attach(1)/detach(2) */
  msg->dir = dir;

  pal_mem_cpy (&msg->hal_ip_grp, &access_grp,
               sizeof (struct hal_ip_access_grp));

  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}

int
hal_ip_set_access_group_interface (struct hal_ip_access_grp access_grp,
                                   char *vifname, char *ifname,
                                   int action, int dir)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_ip_set_access_grp_interface *msg;
  struct
    {
      struct hal_nlmsghdr nlh;
      struct hal_msg_ip_set_access_grp_interface msg;
    } req;
  int ret;

  if ((action != HAL_ACL_ACTION_ATTACH) && (action != HAL_ACL_ACTION_DETACH))
    return -1;

  if ((dir != HAL_ACL_DIRECTION_INGRESS) && (dir != HAL_ACL_DIRECTION_EGRESS))
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof
                   (struct hal_msg_ip_set_access_grp_interface));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  if (action == HAL_ACL_ACTION_ATTACH)
    nlh->nlmsg_type = HAL_MSG_IP_SET_ACCESS_GROUP_INTERFACE;
  else
    nlh->nlmsg_type = HAL_MSG_IP_UNSET_ACCESS_GROUP_INTERFACE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->vifname = vifname;
  msg->ifname = ifname;
  msg->action = action; /* attach(1)/detach(2) */
  msg->dir = dir;

  pal_mem_cpy (&msg->hal_ip_grp, &access_grp,
               sizeof (struct hal_ip_access_grp));

  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}
