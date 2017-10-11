/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_IPV6 

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

/* Function for initialization/deinitialization of IPv6 FIB. */
static int
_hal_ipv6_uc (unsigned short fib, int msgtype)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct
  {
    struct hal_nlmsghdr nlh;
    unsigned short msg;
  } req;
                                                                                                                             
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (u_int16_t));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
                                                                                                                             
  /* Set message. */
  req.msg = fib;
                                                                                                                             
  /* Send message. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
 
  return HAL_SUCCESS;
}


/* 
   Name: hal_ipv6_uc_init

   Description:
   This API initializes the IPv6 unicast table for the specified FIB ID

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_init (unsigned short fib)
{
  return _hal_ipv6_uc (fib, HAL_MSG_IPV6_UC_INIT);
}

/* 
   Name: hal_ipv6_uc_deinit 

   Description:
   This API deinitializes the Ipv6 unicast table for the specified FIB ID

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_deinit (unsigned short fib)
{
  return _hal_ipv6_uc (fib, HAL_MSG_IPV6_UC_DEINIT);
}



/* Common function for route. */
static int
_hal_ipv6_uc_route (int command, unsigned short fib, struct pal_in6_addr *ipaddr,
                    unsigned char masklen,
                    unsigned short num, struct hal_ipv6uc_nexthop *nexthops)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz, i;
  struct hal_msg_ipv6uc_route msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    u_char buf[1024];
  } req;
                                                                                                                             
 /* the maximium # of ECMP routes is defined as HAL_IPV6UC_MAX_NEXTHOPS,
     return error if a user tries to configure too many ECMP routes  */
  if (num > HAL_IPV6UC_MAX_NEXTHOPS)
     return -1;
                                                                                                                             
  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv6uc_route));
  msg.fib_id = fib;
  pal_mem_cpy (&msg.addr, ipaddr, sizeof (struct hal_in6_addr));
  msg.masklen = masklen;
  msg.num = num;
  for (i = 0; i < num; i++)
    {
      msg.nh[i].id = nexthops[i].id;
      msg.nh[i].type = nexthops[i].type;
      msg.nh[i].egressIfindex = nexthops[i].egressIfindex;
      msg.nh[i].nexthopIP = nexthops[i].nexthopIP;
    }

                                                                                                                             
  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv6_route (&pnt, &size, &msg);
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
   Name: hal_ipv6_uc_route_add

   Description:
   This API adds a IPv6 unicast route to the forwarding plane. 

   Parameters:
   IN -> fib - FIB identifier
   IN -> ipaddr - IP address
   IN -> ipmask - IP mask length
   IN -> num - number of nexthops
   IN -> nexthop list - list of nexthops

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_route_add (unsigned short fib, 
                       struct pal_in6_addr *ipaddr, 
                       unsigned char ipmask,
                       unsigned short num, struct hal_ipv6uc_nexthop *nexthops)
{
  return _hal_ipv6_uc_route (HAL_MSG_IPV6_UC_ADD, fib, ipaddr, ipmask, num, 
                             nexthops);
}

/* 
   Name: hal_ipv6_uc_route_delete

   Description:
   This API deletes a IPv6 route from the forwarding plane.

   Parameters:
   IN -> fib - FIB identifier
   IN -> ipaddr - IP address
   IN -> ipmask - IP mask length
   IN -> num - number of nexthops
   IN -> nexthop list - list of nexthops

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_route_delete (unsigned short fib, 
                          struct pal_in6_addr *ipaddr, 
                         unsigned char ipmask,
                         int num, struct hal_ipv6uc_nexthop *nexthops)
{
  return _hal_ipv6_uc_route (HAL_MSG_IPV6_UC_DELETE, fib, ipaddr, ipmask, num, 
                             nexthops);
}

/* 
   Name: hal_ipv6_uc_route_update

   Description:
   This API updates an existing IPv6 route with the new nexthop
   parameters.

   Parameters:
   IN -> fib - FIB indentifier
   IN -> ipaddr - IP address
   IN -> ipmask - IP mask length
   IN -> numfib - number of nexthops in FIB
   IN -> nexthopsfib - nexthops in FIB
   IN -> numnew - number of new/updated nexthops
   IN -> nexthopsnew - new/updated nexthops in FIB

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_route_update (unsigned short fib, 
                          struct pal_in6_addr *ipaddr, 
                          unsigned char ipmask,
                          int numfib, struct hal_ipv6uc_nexthop *nexthopsfib,
                          int numnew, struct hal_ipv6uc_nexthop *nexthopsnew)
{
  hal_ipv6_uc_route_delete (fib, ipaddr, ipmask, numfib, nexthopsfib);
  hal_ipv6_uc_route_add (fib, ipaddr, ipmask, numnew, nexthopsnew);
  return HAL_SUCCESS;
}
                 
#endif /* HAVE_IPV6 */
