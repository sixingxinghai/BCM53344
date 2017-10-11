/* Copyright (C) 2005  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

#ifdef HAVE_IPV6

#include "hal_ipv6_nbr.h"

/* Common function */
static int
_hal_ipv6_nbr (int command, struct pal_in6_addr *addr,
               unsigned char *mac_addr, u_int32_t ifindex)
{
  int ret;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_ipv6_nbr_update msg;
  } req;

  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_ipv6_nbr_update));
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));

  /* Set message. */
  IPV6_ADDR_COPY (&req.msg.addr, addr);
  pal_mem_cpy (req.msg.mac_addr,mac_addr,6) ;
  req.msg.ifindex = ifindex;

  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_ipv6_nbr_update));
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  req.nlh.nlmsg_type = command; 
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return 0;
}

/* 
   Name: hal_ipv6_nbr_add

   Description:
   This API adds an IPv6 neighbor entry

   Parameters:
   IN -> addr     - IPv6 address.
   IN -> mac_addr - Mac address.
   IN -> ifindex  - Ifindex. 

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_nbr_add(struct pal_in6_addr *addr, 
                  unsigned char *mac_addr,
                  u_int32_t ifindex)
{
  return _hal_ipv6_nbr (HAL_MSG_IPV6_NBR_ADD, addr, mac_addr, ifindex);
}


/* 
   Name: hal_ipv6_nbr_del

   Description:
   This API deletes an IPv6 neighbor entry

   Parameters:
   IN -> addr     - IPv6 address.
   IN -> mac_addr - Mac address.

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_nbr_del(struct pal_in6_addr *addr, 
                  unsigned char *mac_addr,
                  unsigned int ifindex)
{
  return _hal_ipv6_nbr (HAL_MSG_IPV6_NBR_DEL, addr, mac_addr, ifindex);
}



/* 
   Name: hal_ipv6_nbr_del_all

   Description:
   This API deletes all dynamic and/or static ipv6 neighbor entries

   Parameters:
   clr_flag : Flag to indicate dynamic and/or static nbr entries

   Returns:
   < 0 on error

   HAL_SUCCESS
*/
int
hal_ipv6_nbr_del_all (unsigned short fib_id, u_char clr_flag)
{
  int ret;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_ipv6_nbr_del_all msg;
  } req;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_ipv6_nbr_del_all));

  /* Set message. */
  req.msg.fib_id = fib_id;
  req.msg.clr_flag = clr_flag;

  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_ipv6_nbr_del_all));
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  req.nlh.nlmsg_type = HAL_MSG_IPV6_NBR_DEL_ALL; 
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return 0;
}


/*
  Callback function for response of IPv6 neighbor cache get. 
*/
static int
_hal_ipv6_nbr_cache_get (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_ipv6_nbr_cache_resp *resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (struct hal_msg_ipv6_nbr_cache_resp *)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_ipv6_nbr_cache_resp (&pnt, &size, resp);
  if (ret < 0)
    return ret;

  return 0;
}


/* 
   Name: hal_ipv6_nbr_cache_get 

   Description:
   This API gets the ipv6 neighbor table  starting at the next address of addr 
   address. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> addr     - IPv6 address.
   IN -> count    - Count
   IN -> cache    - IPv6 neighbor cache.
   IN -> fib_id   - FIB id.

   Returns:
   number of entries. Can be 0 for no entries.
*/
int
hal_ipv6_nbr_cache_get(unsigned short fib_id, 
                       struct pal_in6_addr *addr,
                       int count,
                       struct hal_ipv6_nbr_cache_entry *cache)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_ipv6_nbr_cache_req msg;
  struct hal_msg_ipv6_nbr_cache_resp resp;

  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_ipv6_nbr_cache_req nbr_req;
  } req;

  if(!cache)
     return -1; 
 
  pal_mem_set (&req, 0, sizeof (req));
  resp.cache = cache;  

  msg.fib_id = fib_id;
  /* Set message. */
  if (addr)
    IPV6_ADDR_COPY (&msg.addr, addr);
  else
    pal_mem_set (&msg.addr, 0, sizeof (struct pal_in6_addr));

  msg.count = count;

  pnt = (u_char *) &req.nbr_req;
  size = sizeof (struct hal_msg_ipv6_nbr_cache_req);

  msgsz = hal_msg_encode_ipv6_nbr_cache_req (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  req.nlh.nlmsg_type = HAL_MSG_IPV6_NBR_CACHE_GET;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Set response. */
  resp.count = 0;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, _hal_ipv6_nbr_cache_get, &resp);
  if (ret < 0)
    {
      return 0;
    }

  return resp.count;
}
#endif /* HAVE_IPV6 */
