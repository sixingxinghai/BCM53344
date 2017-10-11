/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_auth.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

/* 
   Name: hal_auth_init

   Description:
   This API initializes the hardware layer for 802.1x port authentication.

   Parameters:
   None

   Returns:
   HAL_ERR_AUTH_INIT
   HAL_SUCCESS
*/
int
hal_auth_init (void)
{
  return hal_msg_generic_request (HAL_MSG_8021x_INIT, NULL, NULL);
}

/* 
   Name: hal_auth_deinit

   Description:
   This API deinitializes the hardware layer for 802.1x port authentication.

   Parameters:
   None

   Returns:
   HAL_ERR_AUTH_INIT
   HAL_SUCCESS
*/
int
hal_auth_deinit (void)
{
  return hal_msg_generic_request (HAL_MSG_8021x_DEINIT, NULL, NULL);
}


/* 
   Name: hal_auth_set_port_state

   Description:
   This API sets the authorization state of a port 

   Parameters:
   IN -> index - port index
   IN -> state - port state
   
   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_auth_set_port_state (unsigned int index,  
                         enum hal_auth_port_state state)
{
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_auth_port_state msg;
  } req;
  int ret;

  /* Set message. */
  req.msg.port_ifindex = index;
  req.msg.port_state = state;
  
  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof 
                                        (struct hal_msg_auth_port_state));
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  req.nlh.nlmsg_type = HAL_MSG_8021x_PORT_STATE;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;
  
  return HAL_SUCCESS;

}

#ifdef HAVE_MAC_AUTH
/* Set port auth-mac state */
int
hal_auth_mac_set_port_state (unsigned int index, int mode,
                             enum hal_auth_mac_port_state state)
{
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_auth_mac_port_state msg;
  } req;
  int ret;
  
  req.msg.port_ifindex = index;
  req.msg.port_state = state;
  req.msg.port_mode = mode;

  /* Set header. */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof 
                                        (struct hal_msg_auth_mac_port_state));
  
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  req.nlh.nlmsg_type = HAL_MSG_AUTH_MAC_PORT_STATE;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;
   
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* Unset the Port-IRQ register */
int
hal_auth_mac_unset_irq (unsigned int index)
{ 
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_auth_mac_unset_irq msg;
  } req;
  int ret;
  
  req.msg.port_ifindex = index;

  /* Set header */
  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof
                                        (struct hal_msg_auth_mac_unset_irq));
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  req.nlh.nlmsg_type = HAL_MSG_AUTH_MAC_UNSET_IRQ;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

#endif
