/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

/* 
   Name: hal_port_mirror_init

   Description:
   This API initializes the port mirroring hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_PMIRROR_INIT
   HAL_SUCCESS
*/
int
hal_port_mirror_init (void)
{
  return hal_msg_generic_request (HAL_MSG_PMIRROR_INIT, NULL, NULL);
}

/* 
   Name: hal_port_mirror_deinit

   Description:
   This API deinitializes the port mirroring hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_PMIRROR_DEINIT
   HAL_SUCCESS
*/
int
hal_port_mirror_deinit (void)
{
  return hal_msg_generic_request (HAL_MSG_PMIRROR_DEINIT, NULL, NULL);
}

/* Common function for setting and unsetting port mirroring. */
static int
_hal_port_mirror (unsigned int to_ifindex, unsigned int from_ifindex,
                  enum hal_port_mirror_direction direction,
                  int msgtype)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_port_mirror *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_port_mirror msg;
  } req;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_port_mirror));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->to_ifindex = to_ifindex;
  msg->from_ifindex = from_ifindex;
  msg->direction = direction;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_port_mirror_set

   Description:
   This API sets the port mirroring.

   Parameters:
   IN -> to_ifindex - Mirrored-to port
   IN -> from_ifindex - Mirrored-from port
   IN -> direction - Direction to set for mirroring

   Returns:
   HAL_ERR_PMIRROR_SET
   HAL_SUCCESS
*/
int
hal_port_mirror_set (unsigned int to_ifindex, unsigned int from_ifindex,
                     enum hal_port_mirror_direction direction)
{
  return _hal_port_mirror (to_ifindex, from_ifindex, direction,
                           HAL_MSG_PMIRROR_SET);
}


/* 
   Name: hal_port_mirror_unset

   Description:
   This API unsets port mirroring.

   Parameters:
   IN -> to_ifindex - Mirrored-to port
   IN -> from_ifindex - Mirrored-from port
   IN -> Direction to unset for mirroring

   Returns:
   HAL_ERR_PMIRROR_UNSET
   HAL_SUCCESS
*/
int
hal_port_mirror_unset (unsigned int to_ifindex, unsigned int from_ifindex,
                       enum hal_port_mirror_direction direction)
{
  return _hal_port_mirror (to_ifindex, from_ifindex, direction,
                           HAL_MSG_PMIRROR_UNSET);
}
