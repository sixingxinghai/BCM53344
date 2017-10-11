/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_incl.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_msg.h"

/* 
   Name: hal_l2_fdb_init

   Description:
   This API initializes the L2 Forwarding table. 

   Parameters:
   None.

   Returns:
   HAL_ERR_L2_FDB_INIT
   HAL_SUCCESS
*/
int
hal_l2_fdb_init (void)
{
  return  hal_msg_generic_request (HAL_MSG_L2_FDB_INIT, NULL, NULL);
}

/* 
   Name: hal_l2_fdb_deinit

   Description:
   This API deinitializes the L2 Forwarding table. 

   Parameters:
   None.

   Returns:
   HAL_ERR_L2_FDB_INIT
   HAL_SUCCESS
*/
int
hal_l2_fdb_deinit (void)
{
  return  hal_msg_generic_request (HAL_MSG_L2_FDB_DEINIT, NULL, NULL);
}

/* 
   Name: hal_l2_add_fdb

   Description:
   This API adds a L2 fdb entry. If the flags parameter is set to HAL_L2_FDB_STATIC
   then the entry is added as static and will not age out. If flags parameter is 0,
   then the entry is added as a dynamic entry and will undergo ageing.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port ifindex
   IN -> vid - VLAN id
   IN -> svid - SVLAN id
   IN -> mac - mac address
   IN -> len - mac address length
   IN -> flags - flags
   IN -> is_forward - forwarding allowed?

   Returns:
   HAL_L2_FDB_ENTRY_EXISTS
   HAL_L2_FDB_ENTRY
   HAL_SUCCESS
*/
int
hal_l2_add_fdb (const char * const name, unsigned int ifindex,
                const unsigned char * const mac, int len,
                unsigned short vid, unsigned short svid,
                unsigned char flags, bool_t is_forward)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_fdb_entry *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_fdb_entry msg;
  } req;
  
  if (len != HAL_HW_LENGTH)
    return HAL_ERR_NOT_SUPPORTED;
  
  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_l2_fdb_entry));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_l2_fdb_entry));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_FDB_ADD;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  pal_mem_cpy(msg->mac, mac, len);
  msg->ifindex = ifindex;
  msg->flags = flags;
  msg->vid = vid;
  msg->is_forward = is_forward;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_l2_del_fdb

   Description:
   This API deletes a L2 fdb entry. If the flags parameter is set to HAL_L2_FDB_STATIC
   then the entry is deleted as static and will not age out. If flags parameter is 0,
   then the entry is deleted as a dynamic entry and will undergo ageing.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port ifindex
   IN -> vid - VLAN id
   IN -> svid - SVLAN id
   IN -> mac - mac address
   IN -> len - mac address length
   IN -> flags - flags

   Returns:
   HAL_L2_FDB_ENTRY_EXISTS
   HAL_L2_FDB_ENTRY
   HAL_SUCCESS
*/
int
hal_l2_del_fdb (const char * const name, unsigned int ifindex,
                const unsigned char * const mac, int len,
                unsigned short vid, unsigned short svid,
                unsigned char flags)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_fdb_entry *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_fdb_entry msg;
  } req;
  
  if (len != HAL_HW_LENGTH)
    return HAL_ERR_NOT_SUPPORTED;
  
  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_l2_fdb_entry));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_l2_fdb_entry));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_FDB_DELETE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  pal_mem_cpy(msg->mac, mac, len);
  msg->ifindex = ifindex;
  msg->flags = flags;
  msg->vid = vid;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* structure for returning fdb entries. */
struct _fdb_resp
{
  int count;
  struct hal_fdb_entry *fdb_entry;
};

/* 
   Callback function for response of unicast FDB get. 
*/
static int
_hal_l2_fdb_unicast_get (struct hal_nlmsghdr *h, void *data)
{
  struct hal_msg_l2_fdb_entry_resp *resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (struct hal_msg_l2_fdb_entry_resp *)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);

  /* Decode message. */
  ret = hal_msg_decode_l2_fdb_resp(&pnt, &size, resp);
  if (ret < 0)
    return ret;

  return 0;
}

/* 
   Name: hal_l2_fdb_unicast_get

   Description:
   This API gets the unicast hal_fdb_entry starting at the next address of mac 
   address and vid specified. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> name - bridge name
   IN -> mac_addr - the FDB entry after this mac address
   IN -> vid - the FDB entry after this vid
   IN -> count - the number of FDB entries to be returned
   OUT -> fdb_entry - the array of fdb_entries returned

   Returns:
   HAL_L2_FDB_ENTRY
   number of entries. Can be 0 for no entries.
*/
int
hal_l2_fdb_unicast_get (char *name, char *mac_addr, unsigned short vid,
                        u_int16_t count, struct hal_fdb_entry *fdb_entry)
{
  int ret;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_fdb_entry_req msg;
  struct hal_msg_l2_fdb_entry_resp resp;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_fdb_entry_req l2_fdb_req;
  } req;

  pal_mem_set (&req, 0, sizeof req);
  resp.count = 0;
  resp.fdb_entry = fdb_entry; /* Call will fill this. */

  /* Set message. */
  msg.count = count;
  msg.start_fdb_entry.vid = vid;
  pal_mem_cpy (msg.start_fdb_entry.mac_addr, mac_addr, HAL_HW_LENGTH);

  pnt = (u_char *) &req.l2_fdb_req;
  size = sizeof (struct hal_msg_l2_fdb_entry_req);

  msgsz = hal_msg_encode_l2_fdb_req(&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type = HAL_MSG_L2_FDB_UNICAST_GET;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;


  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, _hal_l2_fdb_unicast_get, &resp);
  if (ret < 0)
    {
      return 0;
    }

  return resp.count;
}

/* 
   Name: hal_l2_fdb_multicast_get

   Description:
   This API gets the multicast hal_fdb_entry starting at the next address of mac 
   address and vid specified. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> name - bridge name
   IN -> mac_addr - the FDB entry after this mac address
   IN -> vid - the FDB entry after this vid
   IN -> count - the number of FDB entries to be returned
   OUT -> fdb_entry - the array of fdb_entries returned

   Returns:
   HAL_L2_FDB_ENTRY
   number of entries. Can be 0 for no entries.
*/
int
hal_l2_fdb_multicast_get (char *name, char *mac_addr, unsigned short vid,
                          u_int16_t count, struct hal_fdb_entry *fdb_entry)
{
  return HAL_SUCCESS;
}

/* Function for deleting port fdb table. */
static int
_hal_bridge_flush_fdb_by_port(char *name, unsigned int ifindex, unsigned int instance, 
                              unsigned short vid, int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_fdb_flush *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_fdb_flush msg;
  } req;
  int ret;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_l2_fdb_flush));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_l2_fdb_flush));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  msg->ifindex = ifindex;
  msg->instance = instance;
  msg->vid = vid;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/* 
   Name: hal_bridge_flush_fdb_by_port

   Description:
   This API flushes forwarding database. 

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port interface index. 
   IN -> instance - mstp instance
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_flush_fdb_by_port(char *name, unsigned int ifindex, unsigned int instance, 
                             unsigned short vid, unsigned short svid)
{
  return _hal_bridge_flush_fdb_by_port (name, ifindex, instance, vid,
                                        HAL_MSG_L2_FDB_FLUSH_PORT);
}

/* Function for deleting port fdb table. */
static int
_hal_bridge_flush_fdb_by_mac(char *name, char *mac, int maclen, int flags, 
                             int msgtype)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_fdb_entry *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_fdb_entry msg;
  } req;
  int ret;
  
  if (maclen != HAL_HW_LENGTH)
    return HAL_ERR_NOT_SUPPORTED;
  
  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_l2_fdb_entry));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_l2_fdb_entry));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = msgtype;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  pal_mem_cpy(msg->mac, mac, maclen);
  msg->flags = flags;
   
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_bridge_flush_dynamic_fdb_by_mac
                                                                                                                             
   Description:
   This API deletes all the dynamic entries with a given mac address.
                                                                                                                             
   Parameters:
   IN -> name - bridge name
   IN -> mac  - mac address of the dynamic entry to be deleted.
                                                                                                                             
   Returns:
   HAL_SUCCESS.
*/
int
hal_bridge_flush_dynamic_fdb_by_mac (char *bridge_name, 
                                     const unsigned char * const mac,
                                     int maclen)
{
  return _hal_bridge_flush_fdb_by_mac(bridge_name, (char *)mac, maclen, 
                                      HAL_L2_DELETE_DYNAMIC,
                                      HAL_MSG_L2_FLUSH_FDB_BY_MAC);
}


/* 
   Name: hal_l2_add_priority_ovr

   Description:
   This API adds a L2 fdb entry with priority override entry.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port ifindex
   IN -> vid - VLAN id
   IN -> mac - mac address
   IN -> len - mac address length
   IN -> ovr_mac_type - Type of the ATU entry
   IN -> priority - priority

   Returns:
   HAL_L2_FDB_ENTRY_EXISTS
   HAL_L2_FDB_ENTRY
   HAL_SUCCESS
*/
int
hal_l2_add_priority_ovr (const char * const name, unsigned int ifindex,
                         const unsigned char * const mac, int len,
                         unsigned short vid,
                         unsigned char ovr_mac_type,
                         unsigned char priority)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_l2_fdb_prio_override *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_fdb_prio_override msg;
  } req;
  
  if (len != HAL_HW_LENGTH)
    return HAL_ERR_NOT_SUPPORTED;
  
  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_msg_l2_fdb_prio_override));
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof 
                                     (struct hal_msg_l2_fdb_prio_override));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_FDB_MAC_PRIO_ADD;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strcpy (msg->name, name);
  pal_mem_cpy(msg->mac, mac, len);
  msg->ifindex = ifindex;
  msg->vid = vid;
  msg->ovr_mac_type = ovr_mac_type;
  msg->priority   = priority;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_l2_get_index_by_mac_vid

   Description:
   This API Gets the egress ifindex for the given destination MAC

   Parameters:
   IN  -> bridge_name      -  Name of the bridge
   OUT -> ifindex          -  interface index of the port on which the mac was
                              learnt.
   IN  -> vid              -  VID For which the the mac was learnt.
   IN  -> mac              -  Destination MAC.

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/


int
hal_l2_get_index_by_mac_vid (char *bridge_name, int *ifindex, char *mac,
                             u_int16_t vid)
{
  int ret;
  int msgsz;
  u_char *pnt;
  u_int32_t size;
  struct hal_nlmsghdr *nlh;
  struct hal_fdb_entry fdb_entry;
  struct hal_msg_l2_fdb_entry_req msg;
  struct hal_msg_l2_fdb_entry_resp resp;

  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_l2_fdb_entry_req l2_fdb_req;
  } req;


  pal_mem_set (&fdb_entry, 0, sizeof fdb_entry);
  pal_mem_set (&req, 0, sizeof req);
  resp.count = 0;
  resp.fdb_entry = &fdb_entry; /* Call will fill this. */

  /* Set message. */
  msg.count = 1;
  msg.start_fdb_entry.vid = vid;
  msg.req_type = HAL_MSG_L2_FDB_REQ_GET;
  pal_mem_cpy (msg.start_fdb_entry.mac_addr, mac, HAL_HW_LENGTH);

  pnt = (u_char *) &req.l2_fdb_req;
  size = sizeof (struct hal_msg_l2_fdb_entry_req);

  msgsz = hal_msg_encode_l2_fdb_req(&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_type = HAL_MSG_L2_FDB_GET_MAC_VID;
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &req.nlh, _hal_l2_fdb_unicast_get, &resp);

  if (ret < 0)
    {
      return 0;
    }

  *ifindex = fdb_entry.port;
  return 0;

}
