/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"
#include "hal_ipv4_arp.h"

/* Common function for arp. */
static int
_hal_ipv4_arp (int command, struct pal_in4_addr *ipaddr,
               unsigned char *mac_addr, u_int32_t ifindex,
               u_int8_t is_proxy_arp)
{
  int ret;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_arp_update msg;
  } req;

  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_arp_update));
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));

  /* Set message. */
  req.msg.ip_addr.s_addr = ipaddr->s_addr;
  pal_mem_cpy (req.msg.mac_addr,mac_addr,6) ;
  req.msg.ifindex = ifindex;
  req.msg.is_proxy_arp = is_proxy_arp;

  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_arp_update));
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
   Name: hal_ipv4_arp_add

   Description:
   This API adds an IPv4 arp entry

   Parameters:
   IN -> ipaddr   - IP address.
   IN -> mac_addr - Mac address.
   IN -> ifindex  - Ifindex. 

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_arp_entry_add(struct pal_in4_addr *ipaddr, 
                  unsigned char *mac_addr,
                  u_int32_t ifindex,
                  u_int8_t is_proxy_arp)
{
  return _hal_ipv4_arp(HAL_MSG_ARP_ADD, ipaddr, mac_addr, ifindex, is_proxy_arp);
}


/* 
   Name: hal_ipv4_arp_del

   Description:
   This API adds an IPv4 arp entry

   Parameters:
   IN -> ipaddr   - IP address.
   IN -> mac_addr - Mac address.

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_arp_entry_del(struct pal_in4_addr *ipaddr, 
                  unsigned char *mac_addr, u_int32_t ifindex)
{
  return _hal_ipv4_arp(HAL_MSG_ARP_DEL, ipaddr, mac_addr, ifindex, 0);
}


/* 
   Name: hal_arp_del_all

   Description:
   This API deletes all dynamic and/or static arp entries

   Parameters:
   clr_flag : Flag to indicate dynamic and/or static arp entries

   Returns:
   < 0 on error

   HAL_SUCCESS
*/
int
hal_arp_del_all (unsigned short fib_id, u_char clr_flag)
{
  int ret;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_arp_del_all msg;
  } req;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_arp_del_all));

  /* Set message. */
  req.msg.fib_id = fib_id;
  req.msg.clr_flag = clr_flag;

  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_arp_del_all));
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  req.nlh.nlmsg_type = HAL_MSG_ARP_DEL_ALL; 
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return 0;
}


/*
  Callback function for response of ARP cache get. 
*/
static int
_hal_arp_cache_get (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_arp_cache_resp *resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (struct hal_msg_arp_cache_resp *)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_arp_cache_resp(&pnt, &size, resp);
  if (ret < 0)
    return ret;

  return 0;
}

/* 
   Name: hal_arp_cache_get 

   Description:
   This API gets the arp table  starting at the next address of ipaddr 
   address. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> ipaddr   - IP address.
   IN -> count    - Count
   IN -> cache    - arp cache.

   Returns:
   number of entries. Can be 0 for no entries.
*/
int
hal_arp_cache_get(unsigned short fib_id, struct pal_in4_addr *ipaddr,
                  int count,
                  struct hal_arp_cache_entry *cache)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_arp_cache_req msg;
  struct hal_msg_arp_cache_resp resp;

  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_arp_cache_req arp_req;
  } req;

  if(!cache)
     return -1; 
 
  pal_mem_set (&req, 0, sizeof (req));
  resp.cache = cache;  

  msg.fib_id = fib_id;
  /* Set message. */
  if (ipaddr)
    msg.ip_addr.s_addr = ipaddr->s_addr;
  else
    msg.ip_addr.s_addr = 0;

  msg.count = count;

  pnt = (u_char *) &req.arp_req;
  size = sizeof (struct hal_msg_arp_cache_req);

  msgsz = hal_msg_encode_arp_cache_req(&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  req.nlh.nlmsg_type = HAL_MSG_ARP_CACHE_GET;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Set response. */
  resp.count = 0;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, _hal_arp_cache_get, &resp);
  if (ret < 0)
    {
      return 0;
    }

  return resp.count;
}
