/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_VLAN_CLASSIFIER_H_
#define _HAL_VLAN_CLASSIFIER_H_

struct hal_msg_vlan_classifier_rule;

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
hal_vlan_classifier_init();

/*
   Name: hal_vlan_classifier_deinit
   
   Description:
   This API deinitializes the vlan classifier hardware layer.
   
   Parameters:
   None
   
   Returns:
   HAL_ERR_VLAN_CLASSIFIER_DEINIT
   HAL_SUCCESS
*/
int
hal_vlan_classifier_deinit();


/*
   Name: hal_vlan_classifier_add
   
   Description:
   This API adds a vlan classification group.
   
   Parameters:
   IN -> rule_msg - Vlan Classification rule msg
   
   Returns:
   HAL_ERR_VLAN_CLASSIFIER_ADD
   HAL_SUCCESS
*/
int
hal_vlan_classifier_add (struct hal_vlan_classifier_rule *rule_ptr,u_int32_t ifindex, u_int32_t refcount);


/*
   Name: hal_vlan_classifier_del
   
   Description:
   This API deletes a vlan classification group.
   
   Parameters:
   IN -> rule_msg     - Vlan Classification rule
   
   Returns:
   HAL_ERR_VLAN_CLASSIFIER_ADD
   HAL_SUCCESS
*/
int
hal_vlan_classifier_del (struct hal_vlan_classifier_rule *rule_ptr, u_int32_t ifindex, u_int32_t refcount);

#endif /* _HAL_VLAN_CLASSIFIER_H */
