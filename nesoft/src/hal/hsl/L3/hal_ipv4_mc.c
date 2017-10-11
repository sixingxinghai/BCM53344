/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_MCAST_IPV4

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

/* Function for initialization/deinitialization of IPv4 Multicast FIB. */
static int
_hal_ipv4_mc (int msgtype)
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
   Name: hal_ipv4_mc_init
   
   Description:
   This API initializes the IPv4 multicast table for the specified FIB ID.

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_mc_init (int fib)
{
  return _hal_ipv4_mc (HAL_MSG_IPV4_MC_INIT);
}

/* 
   Name: hal_ipv4_mc_deinit
   
   Description:
   This API deinitializes the IPv4 multicast table for the specified 
   FIB ID.

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_mc_deinit (int fib)
{
  return _hal_ipv4_mc (HAL_MSG_IPV4_MC_DEINIT);
}

/* 
   Name: hal_ipv4_mc_pim_init
   
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
hal_ipv4_mc_pim_init (int fib)
{
  return _hal_ipv4_mc (HAL_MSG_IPV4_MC_PIM_INIT);
}

/* 
   Name: hal_ipv4_mc_pim_deinit
   
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
hal_ipv4_mc_pim_deinit (int fib)
{
  return _hal_ipv4_mc (HAL_MSG_IPV4_MC_PIM_DEINIT);
}

/* 
   Name: hal_ipv4_mc_get_max_vifs

   Description:
   This API gets the maximum number of VIFs supported.

   Parameters:
   IN -> vifs - max VIFs

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_mc_get_max_vifs (int *vifs)
{
  return HAL_SUCCESS;
}

/*
  Name: hal_ipv4_mc_vif_add

  Description:
  This API creates a VIF.

  Parameters:
  IN -> index - VIF index
  IN -> phy_ifindex - Interface ifindex
  IN -> loc_addr - VIF local address
  IN -> rmt_addr - VIF remote address (tunnel)
  IN -> flags - VIF flags

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_ipv4_mc_vif_add (u_int32_t index, u_int32_t phy_ifindex, 
    struct hal_in4_addr *loc_addr, struct hal_in4_addr *rmt_addr, 
    u_int16_t flags)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_ipv4mc_vif_add msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[20];
  } req;

#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
  if (index >= HAL_MAX_VIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */

  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv4mc_vif_add));
  msg.vif_index = index;
  msg.ifindex = phy_ifindex;
  msg.loc_addr.s_addr = loc_addr->s_addr;
  msg.rmt_addr.s_addr = rmt_addr->s_addr;
  if (flags & PAL_VIFF_TUNNEL)
    msg.flags = HAL_VIFF_TUNNEL;
  if (flags & PAL_VIFF_REGISTER)
    msg.flags = HAL_VIFF_REGISTER;

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv4_vif_add (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV4_MC_VIF_ADD; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
  Name: hal_ipv4_mc_vif_delete

  Description:
  This API deletes a VIF.

  Parameters:
  IN -> index - VIF index

  Returns:
  < 0 on error
  HAL_SUCCESS
*/
int
hal_ipv4_mc_vif_delete (u_int32_t index)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_int32_t msg;
  } req;

#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
  if (index >= HAL_MAX_VIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */

  req.msg = index;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (u_int32_t));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV4_MC_VIF_DEL; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv4_mc_vif_addr_add

   Description:
   Add an address to a configured VIF.

   Parameters:
   IN -> index - VIF index
   IN -> addr - address to add
   IN -> subnet - subnet address to add
   IN -> broadcast - broadcast address to add
   IN -> peer - peer address to add

   Returns:
   HAL_IPV4_MC_VIF_NOTEXISTS
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_mc_vif_addr_add (unsigned int index,
                          struct hal_in4_addr *addr,
                          struct hal_in4_addr *subnet,
                          struct hal_in4_addr *broadcast,
                          struct hal_in4_addr *peer)
{
  return HAL_SUCCESS;
}

/* 
   Name:
   hal_ipv4_mc_vif_addr_delete

   Description:
   Deletes an address from a configured VIF.

   Parameters:
   IN -> index - VIF index
   IN -> addr - address to delete

   Returns:
   HAL_IPV4_MC_VIF_NOTEXISTS
   HAL_IPV4_MC_VIF_ADDRESS_NOTFOUND
   HAL_SUCCESS
*/
int
hal_ipv4_mc_vif_addr_delete (unsigned int index, struct hal_in4_addr *addr)
{
  return HAL_SUCCESS;
}

/*
  Name: hal_ipv4_mc_vif_set_physical_if

  Description:
  This API sets the physical interface index to a configured VIF.

  Parameters:
  IN -> index - VIF index
  IN -> ifindex - physical interface index

  Returns:
  HAL_IPV4_MC_VIF_NOTEXISTS
  HAL_IPV4_MC_IF_NOTEXISTS
  HAL_SUCCESS
*/
int
hal_ipv4_mc_vif_set_physical_if (unsigned int index, unsigned int ifindex)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv4_mc_vif_set_flags

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
   HAL_IPV4_MC_VIF_NOTEXISTS
   < 0 on other errors
   HAL_SUCCESS
*/
int
hal_ipv4_mc_vif_set_flags (unsigned int ifindex,
                           unsigned char is_pim_register,
                           unsigned char is_p2p,
                           unsigned char is_loopback,
                           unsigned char is_multicast,
                           unsigned char is_broadcast)
{
  return HAL_SUCCESS;
}


/* 
   Name: hal_ipv4_mc_set_min_ttl_threshold

   Description:
   Set the minimum TTL a multicast packet must have to be forwarded on this
   virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> ttl - TTL threshold

   Returns:
   HAL_IPV4_MC_VIF_NOTEXITS
   HAL_SUCCESS
*/
int
hal_ipv4_mc_set_min_ttl_threshold (unsigned int ifindex, unsigned char ttl)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv4_mc_get_min_ttl_threshold

   Description:
   Get the minimum TTL a multicast packet will be to be forwarded on this
   virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> ttl - TTL threshold

   Returns:
   HAL_IPV4_MC_VIF_NOTEXISTS
   HAL_SUCCESS
*/
int
hal_ipv4_mc_get_min_ttl_threshold (unsigned int index, unsigned char ttl)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv4_mc_set_max_rate_limit

   Description:
   Set the maximum multicast bandwidth rate allowed on this virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> rate_limit - rate limit

   Returns:
   HAL_IPV4_MC_VIF_NOTEXISTS
   HAL_SUCCESS
*/
int
hal_ipv4_mc_set_max_rate_limit (unsigned int index, 
                                unsigned int rate_limit)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv4_mc_get_max_rate_limit

   Description:
   Set the maximum multicast bandwidth rate allowed on this virtual interface.

   Parameters:
   IN -> index - VIF index
   IN -> rate_limit - rate limit

   Returns:
   HAL_IPV4_MC_VIF_NOTEXISTS
   HAL_SUCCESS
*/
int
hal_ipv4_mc_get_max_rate_limit (unsigned int index, 
                                unsigned int rate_limit)
{
  return HAL_SUCCESS;
}


/* 
   Name: hal_ipv4_mc_add_mfc 

   Description:
   Install/modify a Multicast Forwarding Cache (MFC). If the MFC entry
   (source, group) is not found a new one will be created; otherwise
   the existing entry will be modified.

   Parameters:
   IN -> source - source 
   IN -> group - group
   IN -> iif_vif_index - MFC incoming interface index
   IN -> num_oifs - Number of elements in olist_vifs array
   IN -> olist_vifs - An array with the vif indexs of forwarding interfaces
   IN -> oifs_ttl - An array with the min. TTL a packet should be forwarded

   Returns:
   < 0 error
   HAL_SUCCESS
*/
int
hal_ipv4_mc_add_mfc (struct hal_in4_addr *source, struct hal_in4_addr *group,
                     u_int32_t iif_vif_index, int num_oifs, 
                     u_int16_t *olist_vifs, u_char *oifs_ttl)
{
  int ret,i;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  int num_elems;
  struct hal_msg_ipv4mc_mrt_add msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[100];
  } req;

#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
  if (num_oifs > HAL_MAX_VIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
    return 0;
#else
    return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */

  /* HSL expects complete information and so we
   * have to send TTLs for ALL interfaces.
   */
  num_elems = HAL_MAX_VIFS;

  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv4mc_mrt_add));
  msg.src.s_addr = source->s_addr;
  msg.group.s_addr = group->s_addr;
  msg.iif_vif = iif_vif_index;
  msg.num_olist = num_elems;
  msg.olist_ttls = XCALLOC (MTYPE_TMP, sizeof (u_char) * num_elems);
  if (NULL == msg.olist_ttls)
    return -1;

  for (i = 0; i < num_oifs; i++)
    {
#ifdef HAVE_IPV4_MCAST_INTF_LIMIT
      if (olist_vifs[i] >= HAL_MAX_VIFS)
#ifdef IGNORE_IPV4_MCAST_INTF_LIMIT
        return 0;
#else
        return -1;
#endif /* IGNORE_IPV4_MCAST_INTF_LIMIT */
#endif /* HAVE_IPV4_MCAST_INTF_LIMIT */

        msg.olist_ttls[olist_vifs[i]] = oifs_ttl[i];
    }

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv4_mrt_add (&pnt, &size, &msg);

  XFREE (MTYPE_TMP, msg.olist_ttls);

  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV4_MC_MRT_ADD; 
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_ipv4_mc_delete_mfc

   Description:
   Delete a Multicast Forwarding Cache (MFC) entry.

   Parameters:
   IN -> source - source
   IN -> group - group
   IN -> index - VIF index

   Returns:
   < 0 error
   HAL_SUCCESS
*/
int
hal_ipv4_mc_delete_mfc (struct hal_in4_addr *source, struct hal_in4_addr *group,
                        u_int32_t index)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_ipv4mc_mrt_del msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[10];
  } req;

  PAL_UNREFERENCED_PARAMETER (index);

  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv4mc_mrt_del));
  msg.src.s_addr = source->s_addr;
  msg.group.s_addr = group->s_addr;

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv4_mrt_del (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV4_MC_MRT_DEL; 
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
_hal_ipv4_sg_stat_req_cb (struct hal_nlmsghdr *h, void *data)
{
  u_char *pnt;
  u_int32_t size;
  int ret;
  struct hal_msg_ipv4mc_sg_stat *msg; 

  msg = (struct hal_msg_ipv4mc_sg_stat *)data;

  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - HAL_NLMSGHDR_SIZE;

  ret = hal_msg_decode_ipv4_sg_stat (&pnt, &size, msg);
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
   IN -> pktcnt - packet count
   IN -> bytecnt - byte count
   IN -> wrong_vif - wrong VIFs

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_mc_get_sg_count (struct hal_in4_addr *source, 
                          struct hal_in4_addr *group,
                          u_int32_t iif_vif,
                          u_int32_t *pktcnt,
                          u_int32_t *bytecnt,
                          u_int32_t *wrong_vif)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_ipv4mc_sg_stat msg;
  struct hal_msg_ipv4mc_sg_stat reply;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct 
  {
    struct hal_nlmsghdr nlh;
    u_char buf[40];;
  } req;

  /* Set message. */
  pal_mem_set (&msg, 0, sizeof (struct hal_msg_ipv4mc_sg_stat));
  msg.src.s_addr = source->s_addr;
  msg.group.s_addr = group->s_addr;
  msg.iif_vif = iif_vif;
  /* Copy the current count in the message so that it can be updated */
  msg.pktcnt = *pktcnt;
  msg.bytecnt = *bytecnt;
  msg.wrong_if = *wrong_vif;

  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_ipv4_sg_stat (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_IPV4_MC_SG_STAT;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  pal_mem_set (&reply, 0, sizeof (struct hal_msg_ipv4mc_sg_stat));

  /* Request HWADDR. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_ipv4_sg_stat_req_cb, (void *)&reply);
  if (ret < 0)
    return ret;

  *pktcnt = reply.pktcnt;
  *bytecnt = reply.bytecnt;
  *wrong_vif = reply.wrong_if;

  return HAL_SUCCESS;
}

#endif /* HAVE_MCAST_IPV4 */
