/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"

#ifdef HAVE_MLD_SNOOP

#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_mld_snoop.h"

/*
  Name: hal_mld_snooping_init 

  Description: 
  This API initializes the reception of MLD packets for MLD
  snooping.

  Parameters:
  None

  Returns:
  HAL_ERR_MLD_SNOOPING_INIT
  HAL_SUCCESS 
*/
int
hal_mld_snooping_init (void)
{
  return  hal_msg_generic_request (HAL_MSG_MLD_SNOOPING_INIT, NULL, NULL);
}
/* 
   Name: hal_mld_snooping_deinit

   Description:
   This API deinitialized the reception of MLD packets for MLD snooping.

   Parameters:
   None

   Returns:
   HAL_ERR_MLD_SNOOPING_INIT
   HAL_SUCCESS
*/
int
hal_mld_snooping_deinit (void)
{
  return  hal_msg_generic_request (HAL_MSG_MLD_SNOOPING_DEINIT, NULL, NULL);
}

/* 
   Name: _hal_mld_snooping 

   Description:
     Common function for enabling and disabling MLD snooping on a bridge. 

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_MLD_SNOOPING_ENABLE
   HAL_SUCCESS
*/

static int
_hal_mld_snooping (char *bridge_name, int msgtype)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_mlds_bridge msg;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_mlds_bridge msg;
  } req;

  pal_mem_set (&req, 0, sizeof req);
  pal_mem_set (&msg, 0, sizeof msg);

  /* Set message. */
  pal_strcpy (msg.bridge_name, bridge_name);

  pnt = (u_char *) &req.msg;
  size = sizeof (struct hal_msg_mlds_bridge);

  msgsz = hal_msg_encode_mlds_bridge(&pnt, &size, &msg);
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
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


static int
_hal_mld_snooping_port (char *name, unsigned int ifindex,
                         int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_port *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_port msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_bridge_port));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_port));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_mld_snooping_if_enable

   Description:
   This API enables MLD snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_MLD_SNOOPING_ENABLE
   HAL_SUCCESS
*/
int
hal_mld_snooping_if_enable (char *name, unsigned int ifindex)
{
  return  _hal_mld_snooping_port (name, ifindex, HAL_MSG_MLD_SNOOPING_ENABLE_PORT);
}


/*
   Name: hal_mld_snooping_if_disable

   Description:
   This API disables MLD snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_MLD_SNOOPING_DISABLE
   HAL_SUCCESS
*/
int
hal_mld_snooping_if_disable (char *name, unsigned int ifindex)
{
  return  _hal_mld_snooping_port (name, ifindex, HAL_MSG_MLD_SNOOPING_DISABLE_PORT);
}


/* 
   Name: hal_mld_snooping_enable 

   Description:
   This API enables MLD snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_MLD_SNOOPING_ENABLE
   HAL_SUCCESS
*/
int
hal_mld_snooping_enable (char *bridge_name)
{
  return _hal_mld_snooping (bridge_name, HAL_MSG_MLD_SNOOPING_ENABLE);
}

/*
  Name: hal_mld_snooping_disable

  Description:
  This API disables MLD snooping for the bridge. 

  Parameters:
  IN -> bridge_name      - bridge name

  Returns:
  HAL_ERR_MLD_SNOOPING_DISABLE
  HAL_SUCCESS
*/
int
hal_mld_snooping_disable (char *bridge_name)
{
  return _hal_mld_snooping (bridge_name, HAL_MSG_MLD_SNOOPING_DISABLE);
}

/*
  Name: _hal_mld_snooping_entry.

  Description:
  This API adds/removes MLD learned l2 mcast entry to/from a bridge..

  Parameters:
  IN -> bridge_name - bridge name
  IN -> src       - multicast source address
  IN -> group     - multicast group address
  IN -> vid       - vlan id
  IN -> count     - count of ports to add/del
  IN -> ifindexes - array of ports to add/del
  IN -> command   - command to execute (add/del)..
  Returns:
  HAL_SUCCESS
*/
/* Common function for add/delete MLD entry. */


static int
_hal_mld_snooping_entry (char *bridge_name,
                         struct hal_in6_addr *src,
                         struct hal_in6_addr *group,
                         char is_exclude,
                         int vid,
                         int count,
                         u_int32_t *ifindexes,
                         int command)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_mld_snoop_entry msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    u_char buf[1024];
  } req;

  if (count <= 0)
    return 0;

  /* Set message. */
  pal_mem_set (&msg, 0, sizeof (struct hal_msg_mld_snoop_entry));
  pal_strcpy (msg.bridge_name, bridge_name);
  pal_mem_cpy (&msg.src, src, sizeof (struct hal_in6_addr));
  pal_mem_cpy (&msg.group, group, sizeof (struct hal_in6_addr));
  msg.is_exclude = is_exclude;
  msg.vid = vid;
  msg.count = count;
  msg.ifindexes = ifindexes;

  /* Set message. */
  pnt = (u_char *) req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_mlds_entry (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return 0;
}


/*
  Name: hal_mld_snooping_add_entry

  Description:
  This API adds a multicast entry for a (source, group) for a given VLAN.
  If the group doesn't exist, a new one will be created.
  If the group exists, the list of ports is added to the entry.

  Parameters:
  IN -> bridge_name - bridge name
  IN -> src - multicast source address
  IN -> group - multicast group address
  IN -> vid - vlan id
  IN -> count - count of ports to add
  IN -> ifindexes - array of ports to add

  Returns:
  HAL_ERR_MLD_SNOOPING_ENTRY_ERR
  HAL_SUCCESS
*/
int
hal_mld_snooping_add_entry (char *bridge_name,
                             struct hal_in6_addr *src,
                             struct hal_in6_addr *group,
                             char is_exclude,
                             int vid,
                             int svid,
                             int count,
                             u_int32_t *ifindexes)
{
  return _hal_mld_snooping_entry (bridge_name, src, group, is_exclude, vid,
                                  count, ifindexes,
                                  HAL_MSG_MLD_SNOOPING_ENTRY_ADD);
}

/*
  Name: hal_mld_snooping_delete_entry

  Description:
  This API deletes a multicast entry for a (source, group) for a given VLAN.
  If the group doesn't exist, a error will be returned.
  If the group exists, the list of ports are deleted from the multicast entry.
  If it is the last port for the multicast entry, the multicast entry is deleted
  as well.

  Parameters:
  IN -> bridge_name - bridge name
  IN -> src - multicast source address
  IN -> group - multicast group address
  IN -> vid - vlan id
  IN -> count - count of ports to add
  IN -> ifindexes - array of ports to add

  Returns:
  HAL_ERR_MLD_SNOOPING_ENTRY_ERR
  HAL_SUCCESS
*/
int
hal_mld_snooping_delete_entry (char *bridge_name,
                                struct hal_in6_addr *src,
                                struct hal_in6_addr *group,
                                char is_exclude,
                                int vid,
                                int svid,
                                int count,
                                u_int32_t *ifindexes)
{
  return _hal_mld_snooping_entry (bridge_name, src, group, is_exclude, vid,
                                  count, ifindexes,
                                  HAL_MSG_MLD_SNOOPING_ENTRY_DELETE);
}

#endif /* HAVE_MLD_SNOOP */
