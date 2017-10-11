/* Copyright (C) 2005  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_VLAN_STACK

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_vlan_stack.h"


int
hal_vlan_stacking_send_message(u_int32_t ifindex, u_int16_t ethtype, 
                               u_int16_t stackmode, int type)
{
  int ret;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_vlan_stack msg;
  } req;

  /* Set message. */
  req.msg.ifindex = ifindex;
  req.msg.ethtype = ethtype;
  req.msg.stackmode = stackmode;

  req.nlh.nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_vlan_stack));
  req.nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  req.nlh.nlmsg_type = type;
  req.nlh.nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);
  if (ret < 0)
    return ret;
  
  return HAL_SUCCESS;
}


/*
   Name: hal_vlan_stacking_enable
   
   Description:
   This API enables vlan stacking on an interface
   
   Parameters:
   IN -> ifindex - Interface index
   IN -> ethtype - Ethernet type value for the vlan tag
   IN -> stack_mode - Vlan stacking mode
  
   Returns:
   HAL_SUCCESS on success
   < 0 on failure
*/
int
hal_vlan_stacking_enable (u_int32_t ifindex, 
                          u_int16_t ethtype,
                          u_int16_t stackmode)
{
  return hal_vlan_stacking_send_message (ifindex, ethtype, stackmode,
                                         HAL_MSG_VLAN_STACKING_ENABLE);
}


/*
   Name: hal_vlan_stacking_disable
   
   Description:
   This API disables vlan stacking on an interface
   
   Parameters:
   IN -> ifindex - Interface index
   IN -> ethtype - Ethernet type value for the vlan tag
   IN -> stack_mode - Vlan stacking mode

   Returns:
   HAL_SUCCESS on success
   < 0 on failure
*/
int
hal_vlan_stacking_disable (u_int32_t ifindex)
{
  return hal_vlan_stacking_send_message (ifindex, 0, HAL_VLAN_STACK_MODE_NONE,
                                         HAL_MSG_VLAN_STACKING_DISABLE);
}
#endif /* HAVE_VLAN_STACK */

