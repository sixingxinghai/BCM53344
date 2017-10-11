/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_incl.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

/* 
   Name: hal_l2_qos_init

   Description:
   This API initializes the Layer 2 QoS hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_L2_QOS_INIT
   HAL_SUCCESS
*/
int
hal_l2_qos_init (void)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_qos_deinit

   Description:
   This API deinitializes the Layer 2 QoS hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_L2_QOS_DEINIT
   HAL_SUCCESS
*/
int
hal_l2_qos_deinit (void)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_qos_default_user_priority_set

   Description:
   This API sets the default user priority for a port.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> user_priority - Default user priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS
*/
#ifdef HAVE_QOS
int
hal_l2_qos_default_user_priority_set (unsigned int ifindex,
                                      unsigned char user_priority)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_default_cos *msg;
  struct
   {
     struct hal_nlmsghdr nlh;
     struct hal_msg_qos_set_default_cos msg;
   } req;
   int ret;
 
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_default_cos));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_QOS_DEFAULT_USER_PRIORITY_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
 
  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->cos_value = user_priority;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

#endif /*HAVE_QOS */
/* 
   Name: hal_l2_qos_default_user_priority_get

   Description:
   This API gets the default user priority for a port. 

   Parameters:
   IN -> ifindex - port ifindex
   OUT -> user_priority - User priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS 
*/
int
hal_l2_qos_default_user_priority_get (unsigned int ifindex,
                                      unsigned char *user_priority)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_qos_regen_user_priority_set

   Description:
   This API sets the regenerated user priority of a port.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> recvd_user_priority - Received user priority
   IN -> regen_user_priority - Regenerated user priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS
*/
int
hal_l2_qos_regen_user_priority_set (unsigned int ifindex, 
                                    unsigned char recvd_user_priority,
                                    unsigned char regen_user_priority)
{
  /* Make sure that flow control send/receive is off */
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_qos_regen_usr_pri *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_qos_regen_usr_pri msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_l2_qos_regen_usr_pri));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_QOS_REGEN_USER_PRIORITY_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;

  msg->ifindex = ifindex;
  msg->recvd_user_priority = recvd_user_priority;
  msg->regen_user_priority = regen_user_priority;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_qos_regen_user_priority_get

   Description:
   This API get the regenerated user priority for a port.

   Parameters:
   IN -> ifindex - port ifindex
   OUT -> regen_user_priority - regenerated user priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS
*/
int
hal_l2_qos_regen_user_priority_get (unsigned int ifindex,
                                    unsigned char *regen_user_priority)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_qos_traffic_class_set

   Description:
   This API sets the traffic class value for a port for a user priority and
   traffic class.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> user_priority - user priority
   IN -> traffic_class - traffic class
   IN -> traffic_class_value - traffic class value

   Returns:
   HAL_ERR_L2_QOS_TRAFFIC_CLASS
   HAL_SUCCESS
*/
int
hal_l2_qos_traffic_class_set (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_qos_traffic_class *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_qos_traffic_class msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_l2_qos_traffic_class));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_QOS_TRAFFIC_CLASS_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;

  msg->ifindex = ifindex;
  msg->user_priority = user_priority;
  msg->traffic_class_value = traffic_class_value;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_qos_traffic_class_get

   Description:
   This API get the traffic class value for a port for a user priority and 
   traffic class.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> user_priority - user priority
   IN -> traffic_class - traffic class
   OUT -> traffic_class_value - traffic class value

   Returns:
   HAL_ERR_L2_QOS_TRAFFIC_CLASS
   HAL_SUCCESS
*/
int
hal_l2_qos_traffic_class_get (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value)
{
  return HAL_SUCCESS;
}

int
hal_l2_traffic_class_status_set (unsigned int ifindex,
                                 unsigned int traffic_class_enabled)
{
 return HAL_SUCCESS;
}

