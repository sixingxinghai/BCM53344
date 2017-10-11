/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_VLAN_CLASS

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_vlan_classifier.h"


/*
   Name: hal_vlan_classifier_init
   
   Description:
   This API initializes the vlan classifier hardware layer.
   
   Parameters:
   None
   
   Returns:
   HAL_ERR_VLAN_CLASSIFIER_INIT
   HAL_SUCCESS
*/
int
hal_vlan_classifier_init()
{
  return HAL_SUCCESS;;
}


/*
   Name: hal_vlan_classifier_deinit
   
   Description:
   This API deinitializes the vlan classifier hardware layer.
   
   Parameters:
    none 
   Returns:
   HAL_ERR_VLAN_CLASSIFIER_DEINIT
   HAL_SUCCESS
*/
int
hal_vlan_classifier_deinit()
{
  return HAL_SUCCESS;
}

/*
   Name: _hal_vlan_classifier_send_message
   
   Description:
   This API installs/deletes classifier rule on specific interface
   
   Parameters:
   IN -> rule_msg - Vlan classifier rule.
   IN -> ifindex - Interface rule should be applied on.
   IN -> refcount - Total reference count for the rule (number of interfaces the rule installed on.  
   IN -> type - Add/Delete flag
  
   
   Returns:
   HAL_ERE_
   HAL_SUCCESS
*/



static int
_hal_vlan_classifier_send_message (struct hal_vlan_classifier_rule *rule_ptr,u_int32_t ifindex,u_int32_t refcount,int type)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  u_char *pnt;
  u_int32_t size;
  int msgsz;
  struct hal_msg_vlan_classifier_rule rule_msg;

  struct
  {
    struct hal_nlmsghdr nlh;
    u_char buf[30];
  } req;
   
  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.buf, 0, sizeof(struct hal_msg_vlan_classifier_rule));
  
  rule_msg.type = rule_ptr->type; 
  rule_msg.vlan_id = rule_ptr->vlan_id; 
  rule_msg.ifindex = ifindex; 
  rule_msg.refcount = refcount; 

  switch(rule_msg.type) 
  {
 
    case HAL_VLAN_CLASSIFIER_MAC:
       memcpy (rule_msg.u.hw_addr, rule_ptr->u.mac, ETHER_ADDR_LEN);
       break;  

    case HAL_VLAN_CLASSIFIER_IPV4:
       rule_msg.u.ipv4.addr = rule_ptr->u.ipv4.addr; 
       rule_msg.u.ipv4.masklen = rule_ptr->u.ipv4.masklen;
       break; 

    case HAL_VLAN_CLASSIFIER_PROTOCOL:
       rule_msg.u.protocol.ether_type = rule_ptr->u.protocol.ether_type;
       rule_msg.u.protocol.encaps     = rule_ptr->u.protocol.encaps;
       break;

    default:
        return -1;
  }
 
  /* Set message. */
  pnt = (u_char *) &(req.buf);
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_vlan_classifier_rule(&pnt, &size, &rule_msg);
  if (msgsz < 0)
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = type;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and process acknowledgement. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
   Name: hal_vlan_classifier_add
   
   Description:
   This API adds a vlan classification group.
   
   Parameters:
   IN -> rule - Vlan Classification rule
   IN -> ifindex - Interface rule should be applied on.
   IN -> refcount - Total reference count for the rule (number of interfaces the rule installed on.  
   
   Returns:
   HAL_ERR_VLAN_CLASSIFIER_ADD
   HAL_SUCCESS
*/
int
hal_vlan_classifier_add (struct hal_vlan_classifier_rule *rule_ptr,u_int32_t ifindex, u_int32_t refcount)

{
  return _hal_vlan_classifier_send_message (rule_ptr,ifindex,refcount,HAL_MSG_VLAN_CLASSIFIER_ADD);
}
/*
   Name: hal_vlan_classifier_del
   
   Description:
   This API deletes a vlan classification group.
   
   Parameters:
   IN -> rule - Vlan Classification rule
   IN -> ifindex - Interface rule should be applied on.
   IN -> refcount - Total reference count for the rule (number of interfaces the rule installed on.  
   
   Returns:
   HAL_ERR_VLAN_CLASSIFIER_ADD
   HAL_SUCCESS
*/
int
hal_vlan_classifier_del (struct hal_vlan_classifier_rule *rule_ptr,u_int32_t ifindex, u_int32_t refcount)
{
  return _hal_vlan_classifier_send_message (rule_ptr,ifindex,refcount, HAL_MSG_VLAN_CLASSIFIER_DELETE);
}

#endif /* HAVE_VLAN_CLASS */
