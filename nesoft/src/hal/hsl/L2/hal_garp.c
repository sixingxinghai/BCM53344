/* Copyright (C) 2003  All Rights Reserved.
                                                                                
LAYER 2 GARP HAL
                                                                                
This module defines the platform abstraction layer to the
Linux layer 2 garp.
                                                                                
*/
                                                                                
#include "pal.h"
#include <errno.h>
#include "lib.h"
#include "hal_incl.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_garp.h"
#include "hal_msg.h"

/* This fuction notifies forwarder that gmrp or gvrp has been enabled or
 * disabled  on a port */
void
hal_garp_set_bridge_type (char *bridge_name, unsigned long garp_type, int enable)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_gxrp *msg;
  
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_gxrp msg;
  }req;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_gxrp));

  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_gxrp));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST ;
  nlh->nlmsg_type = HAL_MSG_GXRP_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;  

  /* Set message */
  msg = &req.msg;
  pal_strcpy (msg->bridge_name, bridge_name);
  msg->garp_type = garp_type;    
  msg->enable = enable;

   /* Send message and process ack */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return; 
}
