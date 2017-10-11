/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"


#ifdef HAVE_LACPD
/* 
   Name: hal_lacp_init

   Description:
   This API initializes the link aggregation hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_LACP_INIT
   HAL_SUCCESS
*/
int 
hal_lacp_init (void)
{
  return hal_msg_generic_request (HAL_MSG_LACP_INIT, NULL, NULL);
}

/* 
   Name: hal_lacp_deinit

   Description:
   This API deinitializes the link aggregation hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_LACP_DEINIT
   HAL_SUCCESS
*/
int
hal_lacp_deinit (void)
{
  return  hal_msg_generic_request (HAL_MSG_LACP_DEINIT, NULL, NULL);
}



/* 
   Name: hal_lacp_add_aggregator

   Description:
   This API adds a aggregator with the specified name and mac address.

   Parameters:
   IN -> name - aggregator name
   IN -> mac  - mac address of aggregator
   IN -> agg_type - aggregator type (L2/L3)

   Returns:
   HAL_ERR_LACP_EXISTS
   HAL_SUCCESS
*/
int 
hal_lacp_add_aggregator (char *name, 
                         unsigned char mac[ETHER_ADDR_LEN],
                         int agg_type)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_lacp_agg_add msg;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_lacp_agg_add msg;
  } req;

  pal_mem_set (&req, 0, sizeof req);
  pal_mem_set (&msg, 0, sizeof msg);

  /* Set message. */
  pal_strcpy (msg.agg_name, name);
  pal_mem_cpy (msg.agg_mac, mac, ETHER_ADDR_LEN);
  msg.agg_type = agg_type;

  pnt = (u_char *) &req.msg;
  size = sizeof (struct hal_msg_lacp_agg_add);

  msgsz = hal_msg_encode_lacp_agg_add(&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type =HAL_MSG_LACP_ADD_AGGREGATOR;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;


  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  return ret;
}

/* 
   Name: hal_lacp_delete_aggregator

   Description:
   This API deletes a aggregator.

   Parameters:
   IN -> name - aggregator name
   IN -> ifindex - aggregator ifindex

   Returns:
   HAL_ERR_LACP_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_lacp_delete_aggregator (char *name, unsigned int ifindex)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_lacp_agg_identifier msg;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_lacp_agg_identifier msg;
  } req;


  if(!name)
    return -1;

  pal_mem_set (&req, 0, sizeof req);
  pal_mem_set (&msg, 0, sizeof msg);

  /* Set message. */
  pal_strcpy (msg.agg_name, name);
  msg.agg_ifindex = ifindex;

  pnt = (u_char *) &req.msg;
  size = sizeof (struct hal_msg_lacp_agg_identifier);

  msgsz = hal_msg_encode_lacp_id(&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type = HAL_MSG_LACP_DELETE_AGGREGATOR;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;


  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  return ret;
}


/* Common function for attaching or detaching mux to/from aggregator. */
static int
_hal_lacp_mux (char *agg_name, unsigned int agg_ifindex, char *port_name, 
               unsigned int port_ifindex, int msgtype)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_lacp_mux msg;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_lacp_mux msg;
  } req;


  if(!agg_name || !port_name)
    return -1;

  pal_mem_set (&req, 0, sizeof req);
  pal_mem_set (&msg, 0, sizeof msg);

  /* Set message. */
  /* Set message. */
  pal_strcpy (msg.agg_name, agg_name);
  msg.agg_ifindex = agg_ifindex;
  pal_strcpy (msg.port_name, port_name);
  msg.port_ifindex = port_ifindex;

  pnt = (u_char *) &req.msg;
  size = sizeof (struct hal_msg_lacp_mux);

  msgsz = hal_msg_encode_lacp_mux(&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;


  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  return ret;
}



/* 
   Name: hal_lacp_attach_mux_to_aggregator

   Description:
   This API adds a port to a aggregator.

   Parameters:
   IN -> agg_name - aggregator name
   IN -> agg_ifindex - aggregator ifindex
   IN -> port_name - port name
   IN -> port_ifindex - port ifindex

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_lacp_attach_mux_to_aggregator (char *agg_name, unsigned int agg_ifindex,
                                   char *port_name, unsigned int port_ifindex)
{
  return _hal_lacp_mux (agg_name, agg_ifindex, port_name, port_ifindex, 
                        HAL_MSG_LACP_ATTACH_MUX_TO_AGGREGATOR);
}



/* 
   Name: hal_lacp_detach_mux_from_aggregator

   Description:
   This API deletes a port from a aggregator.

   Parameters:
   IN -> agg_name - aggregator name
   IN -> agg_ifindex - aggregator ifindex
   IN -> port_name - port name
   IN -> port_ifindex - port ifindex

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_lacp_detach_mux_from_aggregator (char *agg_name, unsigned int agg_ifindex, 
                                     char *port_name, 
                                     unsigned int port_ifindex)
{
  return _hal_lacp_mux (agg_name, agg_ifindex, port_name, port_ifindex,
                        HAL_MSG_LACP_DETACH_MUX_FROM_AGGREGATOR);
}



/* 
   Name: hal_lacp_psc_set

   Description:
   This API sets load balancing mode for an aggregator

   Parameters:
   IN -> psc - port selection criteria (src mac/dst mac based)
   IN -> ifindex - Agregator interface ifindex. 

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_lacp_psc_set (unsigned int ifindex,int psc)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_lacp_psc_set msg;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_lacp_psc_set msg;
  } req;

  pal_mem_set (&req, 0, sizeof req);
  pal_mem_set (&msg, 0, sizeof msg);

  /* Set message. */
  msg.ifindex = ifindex;
  msg.psc = psc;

  pnt = (u_char *) &req.msg;
  size = sizeof (struct hal_msg_lacp_psc_set);

  msgsz = hal_msg_encode_lacp_psc_set(&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type = HAL_MSG_LACP_PSC_SET;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;


  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  return ret;
}

/* 
   Name: hal_lacp_distributing

   Description:
   This API enables or disables distributing for a port.

   Parameters:
   IN -> name - aggregator name
   IN -> ifindex - port ifindex
   IN -> enable - enable

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_lacp_distributing (char *name, unsigned int ifindex, int enable)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_lacp_collecting_distributing

   Parameters:
   IN -> name - aggregator name
   IN -> ifindex - port ifindex
   IN -> enable - enable

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_lacp_collecting_distributing (char *name, unsigned int ifindex, int enable)
{
  return HAL_SUCCESS;
}

#endif /* HAVE_LACPD */
