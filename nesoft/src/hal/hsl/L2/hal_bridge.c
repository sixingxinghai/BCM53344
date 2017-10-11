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
   Name: hal_bridge_init

   Description:
   This API initializes the bridging hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_BRIDGE_INIT
   HAL_SUCCESS
*/
int 
hal_bridge_init (void)
{
  return  hal_msg_generic_request(HAL_MSG_BRIDGE_INIT, NULL, NULL);
}

/* 
   Name: hal_bridge_deinit

   Description:
   This API deinitializes the bridging hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_BRIDGE_DEINIT
   HAL_SUCCESS
*/
int
hal_bridge_deinit (void)
{
  return  hal_msg_generic_request (HAL_MSG_BRIDGE_DEINIT, NULL, NULL);
}

/* Common function for bridge add or delete. */
static int
_hal_bridge (char *name, char is_vlan_aware, enum hal_bridge_type type,
             char edge, int msgtype)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge msg;
  } req;
  
  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_bridge));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);

  msg->is_vlan_aware = is_vlan_aware;
  msg->type = type;
  msg->edge = edge;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_bridge_add 

   Description: 
   This API adds a bridge instance.

   Parameters:
   IN -> name - bridge name
   IN -> is_vlan_aware 

   Returns:
   HAL_ERR_BRIDGE_EXISTS 
   HAL_ERR_BRIDGE_ADD_ERR 
   HAL_SUCCESS 
*/
int
hal_bridge_add (char *name, unsigned int is_vlan_aware, enum hal_bridge_type type,
                unsigned char edge, unsigned char beb, unsigned char *mac)
{

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
//beb and mac are unused parameters
#endif

  return _hal_bridge (name, is_vlan_aware, type, edge, HAL_MSG_BRIDGE_ADD);
}

/* 
   Name: hal_bridge_delete

   Description:
   This API deletes a bridge instance.

   Parameters:
   IN -> name - bridge name

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS 
   HAL_ERR_BRIDGE_DELETE_ERR
   HAL_SUCCESS
*/
int
hal_bridge_delete (char *name)
{
  return _hal_bridge (name, 0, HAL_BRIDGE_MAX, 0, HAL_MSG_BRIDGE_DELETE);
}

int
_hal_bridge_common_change_vlan_type (char *name, char is_vlan_aware,
                                     u_char type, int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_bridge));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  strcpy (msg->name, name);
  msg->is_vlan_aware = is_vlan_aware;
  msg->type = type;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_bridge_change_vlan_type

   Description:
   This API changes the type of the bridge.

   Parameters:
   IN -> name - bridge name
   IN -> is_vlan_aware - Type

   Returns:
   HAL_SUCCESS
   NEGATIVE VALUE IS RETURNED
*/
int
hal_bridge_change_vlan_type (char *name, int is_vlan_aware,
                             u_int8_t type)
{
  return _hal_bridge_common_change_vlan_type (name, is_vlan_aware, type,
                                              HAL_MSG_BRIDGE_CHANGE_VLAN_TYPE);
}

/* 
   Name: hal_bridge_set_state

   Description:
   This API sets the state of the bridge. If the bridge is disabled
   it behaves like a dumb switch.

   Parameters:
   IN -> name - bridge name
   IN -> state - enable/disable spanning tree

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_bridge_set_state (char *name, u_int16_t enable)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_state *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_state msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_bridge_state));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_state));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_BRIDGE_SET_STATE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->enable = enable;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}
/* 
   Name: hal_bridge_set_ageing_time

   Description:
   This API sets the ageing time for a forwarding table(FDB).

   Parameters:
   IN -> name - bridge name
   IN -> ageing_time - Ageing time in seconds.

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_set_ageing_time (char *name, u_int32_t ageing_time)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_ageing *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_ageing msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_bridge_ageing));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_ageing));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_BRIDGE_SET_AGEING_TIME;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->ageing_time = ageing_time;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* Common function for adding or deleting port from a bridge. */
static int
_hal_bridge_common_port (char *name, unsigned int ifindex,
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
   Name: hal_bridge_add_port

   Description:
   This API adds a port to a bridge. 
   
   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index of port

   Returns:
   HAL_ERR_BRIDGE_PORT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_add_port (char *name, unsigned int ifindex)
{
  return _hal_bridge_common_port (name, ifindex, HAL_MSG_BRIDGE_ADD_PORT);
}

/* 
   Name: hal_bridge_delete_port

   Description:
   This API deletes a port from a bridge. 

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index of port

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_delete_port (char *name, int ifindex)
{
  return _hal_bridge_common_port (name, ifindex, HAL_MSG_BRIDGE_DELETE_PORT);
}

/* 
   Name: hal_bridge_set_iearning

   Description:
   This API sets the learning for a bridge.

   Parameters:
   IN -> name - bridge name

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_set_learning (char *name, int learning)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_learn *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_learn msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_bridge_learn));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_learn));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_BRIDGE_SET_LEARNING;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->learn = learning;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* function for setting stp bridge port state. */
static int
_hal_bridge_set_port_state(char *name, int ifindex,int instance, int state, 
                             int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_stp_port_state *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_stp_port_state msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_stp_port_state));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_stp_port_state));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->port_ifindex = ifindex;
  msg->instance = instance;
  msg->port_state = state;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


/* 
   Name: hal_bridge_set_port_state

   Description:
   This API sets port state of a bridge port. 

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index of port
   IN -> instance - 
   IN -> state    - port state

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_set_port_state (char *bridge_name,
                           int ifindex, int instance, int state)
{
  return _hal_bridge_set_port_state(bridge_name,ifindex,instance,state,HAL_MSG_BRIDGE_SET_PORT_STATE);
}

int
hal_bridge_set_learn_fwd (const char *const bridge_name,const int ifindex,
                          const int instance, const int learn, const int forward)
{
  return HAL_SUCCESS;
}

/* Common function for adding/deleting a instance to a bridge. */
static int
_hal_bridge_common_instance (char *name, int instance, 
                             int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_instance *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_instance msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_bridge_instance));
    
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_instance));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->instance = instance;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_bridge_add_instance

   Description:
   This API adds a instance to a bridge. 

   Parameters:
   IN -> name - bridge name
   IN -> instance - instance number

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_add_instance (char *name, int instance)
{
  return _hal_bridge_common_instance (name, instance, HAL_MSG_BRIDGE_ADD_INSTANCE);
}

/* 
   Name: hal_bridge_delete_instance

   Description:
   This API deletes the instance from the bridge.

   Parameters:
   IN -> name - bridge name
   IN -> instance - instance number

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_delete_instance (char *name, int instance)
{
  return _hal_bridge_common_instance (name, instance, HAL_MSG_BRIDGE_DELETE_INSTANCE);
}

/* Common function for adding/deleting vlan from a instance. */
static int
_hal_bridge_common_vlan_to_instance (char *name, int instance, int vid,
                                     int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_vid_instance *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_vid_instance msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_bridge_vid_instance));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_vid_instance));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->vid = vid;
  msg->instance = instance;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_bridge_add_vlan_to_instance

   Description:
   This API adds a VLAN to a instance in a bridge.

   Parametes:
   IN -> name - bridge name
   IN -> instance - instance number
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_ERR_BRIDGE_VLAN_NOT_FOUND
   HAL_SUCCESS
*/
int
hal_bridge_add_vlan_to_instance (char *name, int instance, unsigned short vid)
{
  return _hal_bridge_common_vlan_to_instance (name, instance, vid, 
                                     HAL_MSG_BRIDGE_ADD_VLAN_TO_INSTANCE);
}

/* 
   Name: hal_bridge_delete_vlan_from_instance

   Description:
   This API deletes a VLAN from a instance in a bridge. 

   Parameters:
   IN -> name - bridge name
   IN -> instance - instance number
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_ERR_BRIDGE_VLAN_NOT_FOUND
   HAL_SUCCESS
*/
int
hal_bridge_delete_vlan_from_instance (char *name, int instance, unsigned short vid)
{
  return _hal_bridge_common_vlan_to_instance (name, instance, vid, 
                                 HAL_MSG_BRIDGE_DELETE_VLAN_FROM_INSTANCE);
}


int
hal_l2_get_index_by_mac_vid_svid (char *bridge_name, int *ifindex, char *mac,
                             u_int16_t vid, u_int16_t svid)
{

  return HAL_SUCCESS;
}

#ifdef HAVE_PROVIDER_BRIDGE

/*
   Name: hal_bridge_set_proto_process_port

   Description:
   This API configures a particular protocol data units to be tunnelled
   or dicarded in a customer facing port.

   Parameters:
   IN -> bridge_name - bridge name
   IN -> ifindex  - interface index of the port.
   IN -> proto - Protocols whose PDUs have to be discarded/tunnelled
   IN -> discard - Whether PDUs have to discarded/tunnelled

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_bridge_set_proto_process_port (const char *const bridge_name,
                                   const int ifindex,
                                   enum hal_l2_proto proto,
                                   enum hal_l2_proto_process process,
                                   u_int16_t vid)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_proto_process *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_proto_process msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_proto_process));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_proto_process));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_BRIDGE_PROTO_PROCESS;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->proto = proto;
  msg->proto_process = process;
  msg->vid = vid;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_l2_qos_set_cos_preserve

   Description:
   This API configures CVLAN COS Preservation for a Port and CVID.

   Parameters:
   IN -> ifindex          - interface index of the port.
   IN -> cvid             -  CVID For which the COS Preservation is to be Set.
   IN -> preserve_ce_cos  -  Preserve the Customer VLAN COS.

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_l2_qos_set_cos_preserve (const int ifindex,
                             u_int16_t vid,
                             u_int8_t preserve_ce_cos)
{
  return HAL_SUCCESS;
}

#endif /* HAVE_PROVIDER_BRIDGE */

#if defined (HAVE_I_BEB)
int
hal_pbb_dispatch_service_cnp(char *br_name, unsigned int ifindex, unsigned isid,
                         unsigned short svid_h, unsigned short svid_l, 
                         unsigned char srv_type)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_pbb_service *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_pbb_service msg;
  } req;
  int ret = 0 ;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_proto_process));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_pbb_service));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_APPEND;
  nlh->nlmsg_type = HAL_MSG_PBB_DISPATCH_SERVICE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_mem_cpy(msg->name ,br_name, HAL_BRIDGE_NAME_LEN + 1);
  msg->ifindex = ifindex;
  msg->isid = isid;
  msg->svid_low = svid_l;
  msg->svid_high = svid_h; 
  msg->service_type = srv_type;

  /* Send message and process ack. */
//  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_pbb_dispatch_service_vip(char *br_name, unsigned int ifindex, unsigned isid,
                             unsigned char* macaddr, unsigned char srv_type)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_bridge_pbb_service *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_bridge_pbb_service msg;
  } req;
  int ret = 0 ;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_proto_process));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_bridge_pbb_service));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_APPEND;
  nlh->nlmsg_type = HAL_MSG_PBB_DISPATCH_SERVICE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_mem_cpy(msg->name ,br_name, HAL_BRIDGE_NAME_LEN + 1);
  pal_mem_cpy(msg->default_dst_bmac, macaddr, HAL_HW_LENGTH + 1);  
  msg->ifindex = ifindex;
  msg->isid = isid;
  msg->service_type = srv_type;

  /* Send message and process ack. */
//  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int 
hal_pbb_dispatch_service_pip(char *br_name, unsigned int ifindex, 
                             unsigned isid)
{
    return HAL_SUCCESS;
}

#ifdef HAVE_B_BEB
int 
hal_pbb_dispatch_service_cbp(char *br_name, unsigned int ifindex, 
                          unsigned short bvid, unsigned int e_isid, 
                          unsigned int l_isid, 
                          unsigned char *default_dst_bmac,
                          unsigned char srv_type)
{
    return HAL_SUCCESS;
}

#endif

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)

int
hal_pbb_remove_service(char *br_name, unsigned int ifindex, unsigned isid)
{
    return HAL_SUCCESS;
}

#endif


#endif



