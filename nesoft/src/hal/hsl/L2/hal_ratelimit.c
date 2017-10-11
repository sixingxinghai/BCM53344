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
   Name: hal_ratelimit_init

   Description:
   This API initializes rate limiting.

   Parameters:
   None

   Returns:
   HAL_ERR_RATELIMIT_INIT
   HAL_SUCCESS
*/
int
hal_ratelimit_init (void)
{
  return hal_msg_generic_request (HAL_MSG_RATELIMIT_INIT, NULL, NULL);
}

/* 
   Name: hal_ratelimit_deinit

   Description:
   This API deinitializes the broadcast storm control hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_RATELIMIT_DEINIT
   HAL_SUCCESS
*/
int
hal_ratelimit_deinit (void)
{
  return hal_msg_generic_request (HAL_MSG_RATELIMIT_DEINIT, NULL, NULL);
}

/* Response callback for ratelimit statistics. */
static int
_hal_discards_cb (struct hal_nlmsghdr *h, void *data)
{
  unsigned int *discards = (unsigned int *)data;
  unsigned int *resp;

  resp = HAL_NLMSG_DATA (h);

  *discards = *resp;

  return 0;
}

static int
_hal_l2_ratelimit (int cmd, unsigned int ifindex, 
                   unsigned char level, unsigned char fraction)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_ratelimit *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_ratelimit msg;
  } req;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_ratelimit));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = cmd;
  nlh->nlmsg_seq = ++hallink_cmd.seq;
 
  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->level = level;
  msg->fraction = fraction;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_ratelimit_bcast

   Description:
   This API sets the level in percentage of the port bandwidth for broadcast
   storm suppression.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS
*/
int
hal_l2_ratelimit_bcast (unsigned int ifindex,
                        unsigned char level,
                        unsigned char fraction)
{
  return _hal_l2_ratelimit (HAL_MSG_RATELIMIT_BCAST, ifindex, 
                            level, fraction);
}

/* 
   Name: hal_l2_ratelimit_mcast

   Description:
   This API sets the level in percentage of the port bandwidth for multicast.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS
*/
int
hal_l2_ratelimit_mcast (unsigned int ifindex,
                        unsigned char level,
                        unsigned char fraction)
{
  return _hal_l2_ratelimit (HAL_MSG_RATELIMIT_MCAST, ifindex, 
                            level, fraction);
}
/*
  Name: hal_l2_ratelimit_bcast_mcast

   Description:
   This API sets the level in percentage of the port bandwidth for 
   Broadcast and multicast.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS

*/
int
hal_l2_ratelimit_bcast_mcast (unsigned int ifindex,
                              unsigned char level,
                              unsigned char fraction)
{
  return _hal_l2_ratelimit (HAL_MSG_RATELIMIT_BCAST_MCAST, ifindex,
                            level, fraction);
}

/*
  Name: hal_l2_ratelimit_only_broadcast

   Description:
   This API sets the level in percentage of the port bandwidth for
   Broadcast.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS

*/
int
hal_l2_ratelimit_only_broadcast (unsigned int ifindex,
                                 unsigned char level,
                                 unsigned char fraction)
{
  return _hal_l2_ratelimit (HAL_MSG_RATELIMIT_ONLY_BROADCAST, ifindex,
                            level, fraction);
}


/* 
   Name: hal_l2_ratelimit_dlf_bcast

   Description:
   This API sets the level in percentage of the port bandwidth for dlf
   broadcast.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> level - level in percentage of the port bandwidth

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS
*/
int
hal_l2_ratelimit_dlf_bcast (unsigned int ifindex,
                            unsigned char level,
                            unsigned char fraction)
{
  return _hal_l2_ratelimit (HAL_MSG_RATELIMIT_DLF_BCAST, ifindex, 
                            level, fraction);
}

static int
_hal_l2_discards_get (int msg, unsigned int ifindex, unsigned int *discards)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct 
  {
    struct hal_nlmsghdr nlh;
    unsigned int ifindex;
  } req;
  unsigned int resp;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (unsigned int));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msg;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  req.ifindex = ifindex;

  /* Get bcast rate limiting statistics. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_discards_cb, &resp);
  if (ret < 0)
    return ret;

  *discards = resp;

  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_bcast_discards_get

   Description:
   IN -> ifindex - port ifindex
   OUT -> discards - number of discarded frames

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS
*/
int 
hal_l2_bcast_discards_get (unsigned int ifindex,
                           unsigned int *discards)
{
  return _hal_l2_discards_get (HAL_MSG_RATELIMIT_BCAST_DISCARDS_GET, ifindex, discards);
}

/* 
   Name: hal_l2_mcast_discards_get

   Description:
   IN -> ifindex - port ifindex
   OUT -> discards - number of discarded frames

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS
*/
int 
hal_l2_mcast_discards_get (unsigned int ifindex,
                           unsigned int *discards)
{
  return _hal_l2_discards_get (HAL_MSG_RATELIMIT_MCAST_DISCARDS_GET, ifindex, discards);
}

/* 
   Name: hal_l2_mcast_discards_get

   Description:
   IN -> ifindex - port ifindex
   OUT -> discards - number of discarded frames

   Returns:
   HAL_ERR_RATELIMIT
   HAL_SUCCESS
*/
int 
hal_l2_dlf_bcast_discards_get (unsigned int ifindex,
                               unsigned int *discards)
{
  return _hal_l2_discards_get (HAL_MSG_RATELIMIT_DLF_BCAST_DISCARDS_GET, ifindex, discards);
}
