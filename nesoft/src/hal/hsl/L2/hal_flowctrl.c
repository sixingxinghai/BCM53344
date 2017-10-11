/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

/* 
   Name: hal_flow_control_init

   Description:
   This API initializes the flow control hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_FLOWCTRL_INIT
   HAL_SUCCESS
*/
int
hal_flow_control_init (void)
{
  return hal_msg_generic_request (HAL_MSG_FLOW_CONTROL_INIT, NULL, NULL);
}

/* 
   Name: hal_flow_control_deinit

   Description:
   This API deinitializes the flow control hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_FLOWCTRL_DEINIT
   HAL_SUCCESS
*/
int
hal_flow_control_deinit (void)
{
  return  hal_msg_generic_request (HAL_MSG_FLOW_CONTROL_DEINIT, NULL, NULL);
}

/*
   Name: hal_flow_ctrl_pause_watermark_set

   Description:
   This API sets the flow control properties of a port.

   Parameters:
   IN -> port  - port Number
   IN -> wm_pause - WaterMark Pause 

   Returns:
   HAL_ERR_FLOW_CONTROL_SET
   HAL_SUCCESS
*/
int 
hal_flow_ctrl_pause_watermark_set (u_int32_t port, u_int16_t wm_pause)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_flow_control_wm *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_flow_control_wm msg;
  } req;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_flow_control_wm));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_FLOW_CONTROL_WM_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = port;
  msg->wm_pause = wm_pause;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_flow_control_set 
   
   Description:
   This API sets the flow control properties of a port.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> direction - HAL_FLOW_CONTROL_(OFF|SEND|RECEIVE)

   Returns:
   HAL_ERR_FLOW_CONTROL_SET
   HAL_SUCCESS
*/
int
hal_flow_control_set (unsigned int ifindex, unsigned char direction)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_flow_control *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_flow_control msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_flow_control));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_FLOW_CONTROL_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->direction = direction;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* Response callback for flow control statistics. */
static int
_hal_flow_control_cb (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_flow_control_stats *stats = (struct hal_msg_flow_control_stats *)data;
  struct hal_msg_flow_control_stats *resp;

  if (h->nlmsg_type != HAL_MSG_FLOW_CONTROL_STATISTICS)
    return -1;

  resp = HAL_NLMSG_DATA (h);

  stats->direction = resp->direction;
  stats->rxpause = resp->rxpause;
  stats->txpause = resp->txpause;

  return 0;
}

/* 
   Name: hal_flow_control_statistics 

   Description:
   This API gets the flow control statistics for a port.

   Parameters:
   IN -> ifindex - port ifindex
   OUT -> direction - HAL_FLOW_CONTROL_(OFF|SEND|RECEIVE)
   OUT -> rxpause - Number of received pause frames
   OUT -> txpause - Number of transmitted pause frames

   Returns:
   HAL_ERR_FLOW_CONTROL
   HAL_SUCCESS
*/
int
hal_flow_control_statistics (unsigned int ifindex, unsigned char *direction,
                             int *rxpause, int *txpause)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    unsigned int ifindex;
  } req;
  struct hal_msg_flow_control_stats resp;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (unsigned int));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_FLOW_CONTROL_STATISTICS;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  req.ifindex = ifindex;

  /* Request flow control statistics. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_flow_control_cb, &resp);
  if (ret < 0)
    return ret;

  *direction = resp.direction;
  *rxpause = resp.rxpause;
  *txpause = resp.txpause;

  return HAL_SUCCESS;
}

