/* Copyright (C) 2004  All Rights Reserved.

LAYER 2 PVLAN HAL

This module defines the platform abstraction layer to the
VxWorks layer 2 PVLAN.

*/

#include "pal.h"
#include <errno.h>
#include "lib.h"

#ifdef HAVE_PVLAN
#include "hal_incl.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

int
hal_set_pvlan_type  (char *bridge_name, unsigned short vid, 
                     enum hal_pvlan_type type)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_pvlan *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_pvlan msg;
  } req;
  int ret;
                                                                                
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_pvlan));
                                                                                
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_pvlan));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_PVLAN_SET_VLAN_TYPE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
                                                                                
  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, bridge_name);
  msg->vid = vid;
  msg->vlan_type = type;
                                                                                
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
                                                                                
  return HAL_SUCCESS;
}

static int
_hal_pvlan_associate_common (char *bridge_name, unsigned short primary_vid,
                             unsigned short secondary_vid, int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_pvlan_association *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_pvlan_association msg;
  } req;
  int ret;
                                                                                
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_pvlan_association));
                                                                                
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_pvlan));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
                                                                                
  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, bridge_name);
  msg->pvid = primary_vid;
  msg->svid = secondary_vid;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
                                                                                
  return HAL_SUCCESS;
}

int
hal_vlan_associate (char *bridge_name, unsigned short primary_vid,
                    unsigned short secondary_vid, int associate)
{
  if (associate)
    return _hal_pvlan_associate_common (bridge_name, primary_vid,
                                        secondary_vid, 
                                        HAL_MSG_PVLAN_VLAN_ASSOCIATE);
  else
    return _hal_pvlan_associate_common (bridge_name, primary_vid,
                                        secondary_vid,
                                        HAL_MSG_PVLAN_VLAN_DISSOCIATE);
}

int
hal_pvlan_set_port_mode (char *bridge_name, int ifindex, 
                         enum hal_pvlan_port_mode mode)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_pvlan_port_mode *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_pvlan_port_mode msg;
  } req;
  int ret;
                                                                                
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_pvlan_port_mode));
                                                                                
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_pvlan_port_mode));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_PVLAN_SET_PORT_MODE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
                                                                                
  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, bridge_name);
  msg->ifindex = ifindex;
  msg->port_mode = mode;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
                                                                                
  return HAL_SUCCESS;
}

int
_hal_pvlan_host_association_common (char *bridge_name, int ifindex,
                                    unsigned short primary_vid,
                                    unsigned short secondary_vid,
                                    int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_pvlan_port_set *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_pvlan_port_set msg;
  } req;
  int ret;
                                                                                
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_pvlan_port_set));
                                                                                
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_pvlan_port_set));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
                                                                                
  /* Set message. */
  msg = &(req.msg);
  strcpy (msg->name, bridge_name);
  msg->ifindex = ifindex;
  msg->pvid = primary_vid;
  msg->svid = secondary_vid;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
                                                                                
  return HAL_SUCCESS;
}

int
hal_pvlan_host_association (char *bridge_name, int ifindex,
                            unsigned short primary_vid, 
                            unsigned short secondary_vid, int associate)
{
  if (associate)
    return _hal_pvlan_host_association_common (bridge_name, ifindex,
                                        primary_vid, secondary_vid,
                                        HAL_MSG_PVLAN_PORT_ADD);
  else
    return _hal_pvlan_host_association_common (bridge_name, ifindex,
                                        primary_vid, secondary_vid,
                                        HAL_MSG_PVLAN_PORT_DELETE);
}
#endif /* HAVE_PVLAN */
