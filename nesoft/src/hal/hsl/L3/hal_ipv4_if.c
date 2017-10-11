/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

/*
  Common function for adding/deleting IP addresses on a interface. 
*/
static int
_hal_if_ipv4_address (char *ifname, unsigned int ifindex, struct pal_in4_addr *ipaddr,
                      unsigned char ipmask, int command)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_if_ipv4_addr *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_if_ipv4_addr msg;
  } req;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, ifname);
  msg->ifindex = ifindex;
  msg->addr.s_addr = ipaddr->s_addr;
  msg->ipmask = ipmask;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_if_ipv4_addr));
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
   Name: hal_if_ipv4_address_add 

   Description:
   This API adds a IPv4 address to a L3 interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> ipaddr - ipaddress of interface
   IN -> ipmask - mask length

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_ipv4_address_add (char *ifname, unsigned int ifindex,
                         struct pal_in4_addr *ipaddr, unsigned char ipmask)
{
  return _hal_if_ipv4_address (ifname, ifindex, ipaddr, ipmask, HAL_MSG_IF_IPV4_NEWADDR);
}

/* 
   Name: hal_if_ipv4_address_delete

   Description:
   This API deletes the IPv4 address from a L3 interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> ipaddr - ipaddress of interface
   IN -> ipmask - mask length

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_ipv4_address_delete (char *ifname, unsigned int ifindex,
                            struct pal_in4_addr *ipaddr, unsigned char ipmask)
{
  return _hal_if_ipv4_address (ifname, ifindex, ipaddr, ipmask, HAL_MSG_IF_IPV4_DELADDR);
}

