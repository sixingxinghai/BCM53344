/* Copyright (C) 2003  All Rights Reserved.

QOS HAL

*/

#include "pal.h"
#include "lib.h"

#ifdef HAVE_L2LERN
#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

#include "hal_acl.h"

int
hal_mac_set_access_grp( struct hal_mac_access_grp *hal_macc_grp,
                        int ifindex,
                        int action,
                        int dir)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_mac_set_access_grp *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mac_set_access_grp msg;
  } req;
  int ret;


  if ((action != HAL_ACL_ACTION_ATTACH) || (action != HAL_ACL_ACTION_DETACH))
    return -1;

  if ((dir != HAL_ACL_DIRECTION_INGRESS) || (dir != HAL_ACL_DIRECTION_EGRESS))
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_mac_set_access_grp));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_MAC_SET_ACCESS_GROUP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->action = action;                         /* attach(1)/detach(2) */
  msg->dir = dir;

  memcpy (&msg->hal_macc_grp, hal_macc_grp, sizeof (struct hal_mac_access_grp));

  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}

#ifdef HAVE_VLAN
int
hal_vlan_set_access_map( struct hal_mac_access_grp *hal_macc_grp,
                        int vid,
                        int action)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_vlan_set_access_map *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_vlan_set_access_map msg;
  } req;
  int ret;


  if ((action < HAL_VLAN_ACC_MAP_ADD) || (action > HAL_VLAN_ACC_MAP_DELETE))
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_vlan_set_access_map));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_VLAN_SET_ACCESS_MAP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->vid = vid;
  msg->action = action;                         /* attach(1)/detach(2) */

  memcpy (&msg->hal_macc_grp, hal_macc_grp, sizeof (struct hal_mac_access_grp));

  /* Start policy-map */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  return ret;
}
#endif /* HAVE_VLAN */
#endif /* HAVE_L2LERN */
