/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_MCAST_IPV6

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

/* Function for initialization/deinitialization of IPv6 Multicast FIB. */
static int
_hal_ipv6_mc (int msgtype)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
  } req;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (0);
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
   Name: hal_ipv6_mc_init
   
   Description:
   This API initializes the IPv6 multicast table for the specified FIB ID.

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_mc_init (int fib)
{
  return _hal_ipv6_mc (HAL_MSG_IPV6_MC_INIT);
}

/* 
   Name: hal_ipv6_mc_deinit
   
   Description:
   This API deinitializes the IPv6 multicast table for the specified 
   FIB ID.

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_mc_deinit (int fib)
{
  return _hal_ipv6_mc (HAL_MSG_IPV6_MC_DEINIT);
}

/* 
   Name: hal_ipv6_mc_pim_init
   
   Description:
   This API starts PIM routing for the specified FIB ID.

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_mc_pim_init (int fib)
{
  return _hal_ipv6_mc (HAL_MSG_IPV6_MC_PIM_INIT);
}

/* 
   Name: hal_ipv6_mc_pim_deinit
   
   Description:
   This API stops PIM routing for the specified FIB ID.

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_mc_pim_deinit (int fib)
{
  return _hal_ipv6_mc (HAL_MSG_IPV6_MC_PIM_DEINIT);
}

/* 
   Name: hal_ipv6_mc_get_max_vifs

   Description:
   This API gets the maximum number of VIFs supported.

   Parameters:
   IN -> vifs - max VIFs

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_mc_get_max_vifs (int *vifs)
{
  return HAL_SUCCESS;
}

/*
  Name: hal_ipv6_mc_vif_add

  Description:
  This API creates a VIF.

  Parameters:
  IN -> index - VIF index
  IN -> flags - VIF type

  Renurns:
  HAL_IPV6_MC_VIF_EXISTS
  < 0 on error
  HAL_SUCCESS
*/
int
hal_ipv6_mc_vif_add (u_int32_t vif_index, u_int32_t phy_ifindex, 
    u_int16_t flags)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_ipv6mc_vif_add msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[20];
  } req;

#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
  if (vif_index >= HAL_MAX_IPV6_VIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv6mc_vif_add));
  msg.vif_index = vif_index;
  msg.phy_ifindex = phy_ifindex;
  if (flags & PAL_MIFF_REGISTER)
    msg.flags = HAL_IPV6_VIFF_REGISTER;

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv6_vif_add (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV6_MC_VIF_ADD; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
  Name: hal_ipv6_mc_vif_delete

  Description:
  This API deletes a VIF.

  Parameters:
  IN -> index - VIF index

  Returns:
  HAL_IPV6_MC_VIF_NOTEXISTS
  HAL_IPV6_MC_VIF_MAX_EXCEEDED
  < 0 on error
  HAL_SUCCESS
*/
int
hal_ipv6_mc_vif_delete (u_int32_t vif_index)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_int32_t msg;
  } req;

#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
  if (vif_index >= HAL_MAX_IPV6_VIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

  req.msg = vif_index;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (u_int32_t));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV6_MC_VIF_DEL; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_vif_addr_add

   Description:
   Add an address to a configured VIF.

   Parameters:
   IN -> index - VIF index
   IN -> addr - address to add
   IN -> subnet - subnet address to add
   IN -> broadcast - broadcast address to add
   IN -> peer - peer address to add

   Returns:
   HAL_IPV6_MC_VIF_NOTEXISTS
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_mc_vif_addr_add (unsigned int index,
                          struct pal_in6_addr *addr,
                          struct pal_in6_addr *subnet,
                          struct pal_in6_addr *broadcast,
                          struct pal_in6_addr *peer)
{
  return HAL_SUCCESS;
}

/* 
   Name:
   hal_ipv6_mc_vif_addr_delete

   Description:
   Deletes an address from a configured VIF.

   Parameters:
   IN -> index - VIF index
   IN -> addr - address to delete

   Returns:
   HAL_IPV6_MC_VIF_NOTEXISTS
   HAL_IPV6_MC_VIF_ADDRESS_NOTFOUND
   HAL_SUCCESS
*/
int
hal_ipv6_mc_vif_addr_delete (unsigned int index, struct pal_in6_addr *addr)
{
  return HAL_SUCCESS;
}


/*
  Name: hal_ipv6_mc_vif_set_physical_if

  Description:
  This API sets the physical interface index to a configured VIF.

  Parameters:
  IN -> index - VIF index
  IN -> ifindex - physical interface index

  Returns:
  HAL_IPV6_MC_VIF_NOTEXISTS
  HAL_IPV6_MC_IF_NOTEXISTS
  HAL_SUCCESS
*/
int
hal_ipv6_mc_vif_set_physical_if (unsigned int index, unsigned int ifindex)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_vif_set_flags

   Description:
   This API sets the VIF flags of a configured VIF.

   Parameters:
   IN -> index - VIF index
   IN -> is_pim_register - true if the VIF is a PIM register interface 
   IN -> is_p2p - true if the VIF is a point-to-point interface
   IN -> is_loopback - true if the VIF is a loopback interface
   IN -> is_multicast - true if the VIF is a multicast interface
   IN -> is_broadcast - true if the VIF is a broadcast interface

   Returns:
   HAL_IPV6_MC_VIF_NOTEXISTS
   < 0 on other errors
   HAL_SUCCESS
*/
int
hal_ipv6_mc_vif_set_flags (unsigned int ifindex,
                           unsigned char is_pim_register,
                           unsigned char is_p2p,
                           unsigned char is_loopback,
                           unsigned char is_multicast,
                           unsigned char is_broadcast)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_set_min_ttl_threshold

   Description:
   Set the minimum TTL a multicast packet must have to be forwarded on this
   virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> ttl - TTL threshold

   Returns:
   HAL_IPV6_MC_VIF_NOTEXITS
   HAL_SUCCESS
*/
int
hal_ipv6_mc_set_min_ttl_threshold (unsigned int ifindex, unsigned char ttl)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_get_min_ttl_threshold

   Description:
   Get the minimum TTL a multicast packet will be to be forwarded on this
   virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> ttl - TTL threshold

   Returns:
   HAL_IPV6_MC_VIF_NOTEXISTS
   HAL_SUCCESS
*/
int
hal_ipv6_mc_get_min_ttl_threshold (unsigned int index, unsigned char ttl)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_set_max_rate_limit

   Description:
   Set the maximum multicast bandwidth rate allowed on this virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> rate_limit - rate limit

   Returns:
   HAL_IPV6_MC_VIF_NOTEXISTS
   HAL_SUCCESS
*/
int
hal_ipv6_mc_set_max_rate_limit (unsigned int index, 
                                unsigned int rate_limit)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_get_max_rate_limit

   Description:
   Set the maximum multicast bandwidth rate allowed on this virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> rate_limit - rate limit

   Returns:
   HAL_IPV6_MC_VIF_NOTEXISTS
   HAL_SUCCESS
*/
int
hal_ipv6_mc_get_max_rate_limit (unsigned int index, 
                                unsigned int rate_limit)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_add_mfc 

   Description:
   Install/modify a Multicast Forwarding Cache (MFC). If the MFC entry
   (source, group) is not found a new one will be created; otherwise
   the existing entry will be modified.

   Parameters:
   IN -> source - source 
   IN -> group - group
   IN -> iif_vif_index - MFC incoming interface index
   IN -> num_olist - Number of unsigned 32-bit elements in the olist bitmap
   IN -> olist - A array of vif indices in olist

   Returns:
   HAL_IPV6_MC_MFC
   HAL_SUCCESS
*/
int
hal_ipv6_mc_add_mfc (struct hal_in6_addr *source, struct hal_in6_addr *group,
                     u_int32_t iif_vif_index, u_int32_t num_olist, 
                     u_int16_t *olist)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  int i;
  struct hal_msg_ipv6mc_mrt_add msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[200];
  } req;

#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
  if (num_olist > HAL_MAX_IPV6_VIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

#ifdef HAVE_IPV6_MCAST_INTF_LIMIT
  for (i = 0; i < num_olist; i++)
    {
      if (olist[i] >= HAL_MAX_IPV6_VIFS)
#ifdef IGNORE_IPV6_MCAST_INTF_LIMIT
        return 0;
#else
        return -1;
#endif /* IGNORE_IPV6_MCAST_INTF_LIMIT */
    }
#endif /* HAVE_IPV6_MCAST_INTF_LIMIT */

  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv6mc_mrt_add));
  msg.src = *source;
  msg.group = *group;
  msg.iif_vif = iif_vif_index;
  msg.num_olist = num_olist;
  /* Add 1 to num_olist in order to avoid allocating 0 size memory */
  msg.olist = XCALLOC (MTYPE_TMP, sizeof (u_int16_t) * (num_olist + 1));
  if (NULL == msg.olist)
    return -1;
  pal_mem_cpy (msg.olist, olist, sizeof (u_int16_t) * num_olist);

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv6_mrt_add (&pnt, &size, &msg);

  XFREE (MTYPE_TMP, msg.olist);

  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV6_MC_MRT_ADD; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv6_mc_delete_mfc

   Description:
   Delete a Multicast Forwarding Cache (MFC) entry.

   Parameters:
   IN -> source - source
   IN -> group - group

   Returns:
   HAL_IPV6_MC_MFC_NOTEXISTS
   HAL_SUCCESS
*/
int
hal_ipv6_mc_delete_mfc (struct hal_in6_addr *source, 
    struct hal_in6_addr *group)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_ipv6mc_mrt_del msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[32];
  } req;

  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv6mc_mrt_del));
  msg.src = *source;
  msg.group = *group;

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv6_mrt_del (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV6_MC_MRT_DEL; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Response callback for sg stat request message. 
*/
static int
_hal_ipv6_sg_stat_req_cb (struct hal_nlmsghdr *h, void *data)
{
  u_char *pnt;
  u_int32_t size;
  int ret;
  struct hal_msg_ipv6mc_sg_stat *msg; 

  msg = (struct hal_msg_ipv6mc_sg_stat *)data;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - HAL_NLMSGHDR_SIZE;

  ret = hal_msg_decode_ipv6_sg_stat (&pnt, &size, msg);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


/* 
   Name: hal_ivp4_mc_get_sg_count

   Description:
   This API gets the various counters per (S, G) entry.

   Parameters:
   IN -> src - source address
   IN -> grp - group addresss
   IN -> iif_vif - incoming VIF
   IN -> pktcnt - packet count
   IN -> bytecnt - byte count
   IN -> wrong_vif - wrong VIFs

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_mc_get_sg_count (struct hal_in6_addr *source, 
                          struct hal_in6_addr *group,
                          u_int32_t iif_vif,
                          u_int32_t *pktcnt,
                          u_int32_t *bytecnt,
                          u_int32_t *wrong_vif)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_ipv6mc_sg_stat msg;
  struct hal_msg_ipv6mc_sg_stat reply;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[50];
  } req;

  /* Set message. */
  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv6mc_sg_stat));
  msg.src = *source;
  msg.group = *group;
  msg.pktcnt = *pktcnt;
  msg.iif_vif = iif_vif;
  msg.bytecnt = *bytecnt;
  msg.wrong_if = *wrong_vif;

  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv6_sg_stat (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV6_MC_SG_STAT;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  pal_mem_set (&reply, 0, sizeof (struct hal_msg_ipv6mc_sg_stat));

  /* Request HWADDR. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_ipv6_sg_stat_req_cb, (void *)&reply);
  if (ret < 0)
    return ret;

  *pktcnt = reply.pktcnt;
  *bytecnt = reply.bytecnt;
  *wrong_vif = reply.wrong_if;

  return HAL_SUCCESS;
}

#endif /* HAVE_MCAST_IPV6 */
