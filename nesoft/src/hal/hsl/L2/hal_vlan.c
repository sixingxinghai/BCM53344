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
   Name: hal_vlan_init

   Description:
   This API initializes the VLAN hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_VLAN_INIT
   HAL_SUCCESS
*/
int
hal_vlan_init (void)
{
  return hal_msg_generic_request (HAL_MSG_VLAN_INIT, NULL, NULL);
}

/* 
   Name: hal_vlan_deinit

   Description:
   This API deinitializes the VLAN hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_VLAN_DEINIT
   HAL_SUCCESS
*/
int
hal_vlan_deinit (void)
{
  return hal_msg_generic_request (HAL_MSG_VLAN_DEINIT, NULL, NULL);
}

/* Common function for adding/deleting a VLAN. */
static int
_hal_vlan_common (char *name, enum hal_vlan_type type, unsigned short vid,
                  int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_vlan *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_vlan msg;
  } req;
  int ret;
  
  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_vlan));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_vlan));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, name);
  msg->vid = vid;
  msg->type = type;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_vlan_add 

   Description: 
   This API adds a VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_VLAN_EXISTS 
   HAL_SUCCESS 
*/
int
hal_vlan_add (char *name, enum hal_vlan_type type, unsigned short vid)
{
  return _hal_vlan_common (name, type, vid, HAL_MSG_VLAN_ADD);
}

/* 
   Name: hal_vlan_delete

   Description:
   This API deletes a VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_delete (char *name, enum hal_vlan_type type, unsigned short vid)
{
  return _hal_vlan_common (name, type, vid, HAL_MSG_VLAN_DELETE);
}

/* 
   Name: hal_vlan_set_port_type

   Description:
   This API sets the acceptable frame type for a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> port_type - trunk, access or hybrid
   IN -> acceptable_frame_type - acceptable frame type
   IN -> enable_ingress_filter - enable ingress filtering

   Returns:
   HAL_ERR_VLAN_FRAME_TYPE
   HAL_SUCCESS
*/
int
hal_vlan_set_port_type (char *name, 
                        unsigned int ifindex,
                        enum hal_vlan_port_type port_type,
                        enum hal_vlan_port_type sub_port_type,
                        enum hal_vlan_acceptable_frame_type acceptable_frame_type,
                        unsigned short enable_ingress_filter)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_vlan_port_type *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_vlan_port_type msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_vlan_port_type));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_vlan_port_type));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_VLAN_SET_PORT_TYPE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, name);
  msg->ifindex = ifindex;
  msg->port_type = port_type;
  msg->sub_port_type = sub_port_type;
  msg->acceptable_frame_type = acceptable_frame_type;
  msg->enable_ingress_filter = enable_ingress_filter;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* Common function to add/delete a vid to/from a port and set the egress type. */
static int
_hal_vlan_common_port (char *name, unsigned int ifindex,
                       unsigned short vid,
                       enum hal_vlan_egress_type egress,
                       int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_vlan_port *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_vlan_port msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_vlan_port));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_vlan_port));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, name);
  msg->ifindex = ifindex;
  msg->vid = vid;
  msg->egress = egress;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_vlan_set_default_pvid

   Description:
   This API sets the default PVID for a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> pvid - default PVID
   IN -> egress - egress tagged/untagged

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_set_default_pvid (char *name, unsigned int ifindex,
                           unsigned short pvid, 
                           enum hal_vlan_egress_type egress)
{
  return _hal_vlan_common_port (name, ifindex, pvid, egress, HAL_MSG_VLAN_SET_DEFAULT_PVID);
}

/*
   Name: hal_vlan_set_native_vid

   Description:
   This API sets the Native VID for a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - native VID

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_set_native_vid (char *name, unsigned int ifindex,
                         unsigned short vid)
{
  return HAL_SUCCESS;
}

/* 
   Name: hal_vlan_add_vid_to_port

   Description:
   This API adds a VLAN to a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - VLAN id
   IN -> egress - egress tagged/untagged

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_add_vid_to_port (char *name, unsigned int ifindex,
                          unsigned short vid, 
                          enum hal_vlan_egress_type egress)
{
  return _hal_vlan_common_port (name, ifindex, vid, egress, HAL_MSG_VLAN_ADD_VID_TO_PORT);
}

/* 
   Name: hal_vlan_delete_vid_from_port

   Description:
   This API deletes a VLAN from a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_delete_vid_from_port (char *name, unsigned int ifindex,
                               unsigned short vid)
{
  return _hal_vlan_common_port (name, ifindex, vid, 0, HAL_MSG_VLAN_DELETE_VID_FROM_PORT);
}

/*
   Name: hal_vlan_port_set_dot1q_state

   Description:
   This API sets the dot1q state on a port.

   Parameters:
   IN -> ifindex - interface index
   IN -> enable - To enable or Disbale dot1q
   IN -> enable_ingress_filter - enable ingress filtering

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_port_set_dot1q_state (unsigned int ifindex, unsigned short enable,
                               unsigned short enable_ingress_filter)
{
  struct hal_nlmsghdr *nlh;
  struct hsl_msg_enable_dot1q *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hsl_msg_enable_dot1q msg;
  } req;

  int ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hsl_msg_enable_dot1q));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hsl_msg_enable_dot1q));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DOT1Q_SET_PORT_STATE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->enable = enable;
  msg->enable_ingress_filter = enable_ingress_filter;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;

}

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
static int
_hal_vlan_l2_unknown_mcast (int msgtype)
{
  int ret;
  struct hal_nlmsghdr *nlh;

  struct
    {
      struct hal_nlmsghdr nlh;
    } req;

  pal_mem_set (&req, 0, sizeof req);

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (0);
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

 /*
    Name: hal_l2_unknown_mcast_mode

    Description:
    This API sets flooding or discard mode (i.e. floodinng of unlearned multicast)
    of MLD snooping for the bridge.

    Parameters:
    IN -> status - flood/discard

    Returns:
    HAL_ERR_L2_UNKNOWN_MCAST
    HAL_SUCCESS
 */

int
hal_l2_unknown_mcast_mode (int mode)
 {
    if (mode)
      return _hal_vlan_l2_unknown_mcast (HAL_MSG_L2_UNKNOWN_MCAST_MODE_DISCARD);
    else
      return _hal_vlan_l2_unknown_mcast (HAL_MSG_L2_UNKNOWN_MCAST_MODE_FLOOD);
 }
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_PROVIDER_BRIDGE

/* Common function to add/delete a vid to/from a port and set the egress type. */
static int
_hal_cvlan_reg_common_port (char *name, unsigned int ifindex,
                            unsigned short cvid,
                            unsigned short svid,
                            int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_cvlan_reg_tab *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_cvlan_reg_tab msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_cvlan_reg_tab));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_cvlan_reg_tab));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, name);
  msg->ifindex = ifindex;
  msg->cvid = cvid;
  msg->svid = svid;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


/*
   Name: hal_vlan_create_cvlan_registration_entry

   Description:
   This API creates a mapping between C-VLAN to S-VLAN on a CE port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_create_cvlan_registration_entry (char *name, unsigned int ifindex,
                                          unsigned short cvid,
                                          unsigned short svid)
{
  return _hal_cvlan_reg_common_port (name, ifindex, cvid, svid,
                                     HAL_MSG_VLAN_ADD_VLAN_REG_TAB_ENT);
}

/*
   Name: hal_vlan_delete_cvlan_registration_entry

   Description:
   This API deletes a mapping between C-VLAN to S-VLAN on a CE port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_delete_cvlan_registration_entry (char *name, unsigned int ifindex,
                                          unsigned short cvid,
                                          unsigned short svid)
{
  return _hal_cvlan_reg_common_port (name, ifindex, cvid, svid,
                                     HAL_MSG_VLAN_DEL_VLAN_REG_TAB_ENT);
}

/*
   Name: hal_vlan_add_cvid_to_port

   Description:
   This API adds a CVLAN to a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - C VLAN id
   IN -> svid - S VLAN id
   IN -> egress - egress tagged/untagged

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_add_cvid_to_port (char *name, unsigned int ifindex,
                           unsigned short cvid,
                           unsigned short svid,
                           enum hal_vlan_egress_type egress)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_delete_cvid_from_port

   Description:
   This API deletes a VLAN from a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - C VLAN id
   IN -> svid - S VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_delete_cvid_from_port (char *name, unsigned int ifindex,
                                unsigned short cvid,
                                unsigned short svid)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_create_cvlan

   Description:
   This API creates a mapping between C-VLAN to S-VLAN and creates a
   corresponding Internal VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_create_cvlan (char *name, unsigned short cvid,
                       unsigned short svid)
{
  return HAL_SUCCESS;
}


/*
   Name: hal_vlan_delete_cvlan

   Description:
   This API deletes a mapping between C-VLAN to S-VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_delete_cvlan (char *name, unsigned short cvid,
                       unsigned short svid)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_create_vlan_trans_entry

   Description:
   This API creates a translation from VLAN1 to VLAN 2 on a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - VLAN id to be translated
   IN -> trans_vid - Translated VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_create_vlan_trans_entry (char *name, unsigned int ifindex,
                                  unsigned short vid,
                                  unsigned short trans_vid)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_delete_vlan_trans_entry

   Description:
   This API deletes a translation from VLAN1 to VLAN 2 on a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - VLAN id to be translated
   IN -> trans_vid - Translated VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_delete_vlan_trans_entry (char *name, unsigned int ifindex,
                                   unsigned short vid,
                                   unsigned short trans_vid)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_set_pro_edge_pvid

   Description:
   This API Configures the PVID for the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port
   IN -> pvid - VLAN id Used for Untagged Packets.

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_set_pro_edge_pvid (char *name, unsigned int ifindex,
                            unsigned short svid,
                            unsigned short pvid)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_set_pro_edge_untagged_vid

   Description:
   This API Configures the Untagged VID for the Egress for 
   the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port
   IN -> untagged_vid - VLAN id that will be transmitted
                        untagged in the provider Edge Port.

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_set_pro_edge_untagged_vid (char *name, unsigned int ifindex,
                                    unsigned short svid,
                                    unsigned short untagged_vid)
{
  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_add_pro_edge_port

   Description:
   This API Configures the PVID for the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_add_pro_edge_port (char *name, unsigned int ifindex,
                            unsigned short svid)
{
  return hal_vlan_add_vid_to_port (name, ifindex,
                                   svid, HAL_VLAN_EGRESS_UNTAGGED);
}

/*
   Name: hal_vlan_del_pro_edge_port 

   Description:
   This API Configures the Untagged VID for the Egress for
   the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_del_pro_edge_port (char *name, unsigned int ifindex,
                            unsigned short svid)
{
  return hal_vlan_delete_vid_from_port (name, ifindex, svid);
}

/*
   Name: hal_pro_vlan_set_dtag_mode

   Description:
   This API Configures double tag mode

   Parameters:
   IN -> ifindex - interface index
   IN -> dtag mode - Whether it is a single tag port or a double tag
                     port.

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_SUCCESS
*/

int hal_pro_vlan_set_dtag_mode (unsigned int ifindex,
                                unsigned short dtag_mode)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_vlan_stack *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_vlan_stack msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_vlan_stack));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_vlan_stack));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_SET_DTAG_MODE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->stackmode = dtag_mode;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;

}


#endif /* HAVE_PROVIDER_BRIDGE */
