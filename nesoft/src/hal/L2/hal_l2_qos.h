/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_L2_QOS_H_
#define _HAL_L2_QOS_H_

/* 
   Name: hal_l2_qos_init

   Description:
   This API initializes the Layer 2 QoS hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_L2_QOS_INIT
   HAL_SUCCESS
*/
int
hal_l2_qos_init (void);

/* 
   Name: hal_l2_qos_deinit

   Description:
   This API deinitializes the Layer 2 QoS hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_L2_QOS_DEINIT
   HAL_SUCCESS
*/
int
hal_l2_qos_deinit (void);

/* 
   Name: hal_l2_qos_default_user_priority_set

   Description:
   This API sets the default user priority for a port.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> user_priority - Default user priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS
*/
#ifdef HAVE_QOS
int
hal_l2_qos_default_user_priority_set (unsigned int ifindex,
                                      unsigned char user_priority);
#endif /* HAVE_QOS */

/* 
   Name: hal_l2_qos_default_user_priority_get

   Description:
   This API gets the default user priority for a port. 

   Parameters:
   IN -> ifindex - port ifindex
   OUT -> user_priority - User priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS 
*/
int
hal_l2_qos_default_user_priority_get (unsigned int ifindex,
                                      unsigned char *user_priority);


/* 
   Name: hal_l2_qos_regen_user_priority_set

   Description:
   This API sets the regenerated user priority of a port.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> recvd_user_priority - Received user priority
   IN -> regen_user_priority - Regenerated user priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS
*/
int
hal_l2_qos_regen_user_priority_set (unsigned int ifindex, 
                                    unsigned char recvd_user_priority,
                                    unsigned char regen_user_priority);

/* 
   Name: hal_l2_qos_regen_user_priority_get

   Description:
   This API get the regenerated user priority for a port.

   Parameters:
   IN -> ifindex - port ifindex
   OUT -> regen_user_priority - regenerated user priority

   Returns:
   HAL_ERR_L2_QOS
   HAL_SUCCESS
*/
int
hal_l2_qos_regen_user_priority_get (unsigned int ifindex,
                                    unsigned char *regen_user_priority);

/* 
   Name: hal_l2_qos_traffic_class_set

   Description:
   This API sets the traffic class value for a port for a user priority and
   traffic class.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> user_priority - user priority
   IN -> traffic_class - traffic class
   IN -> traffic_class_value - traffic class value

   Returns:
   HAL_ERR_L2_QOS_TRAFFIC_CLASS
   HAL_SUCCESS
*/
int
hal_l2_qos_traffic_class_set (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value);

/* 
   Name: hal_l2_qos_traffic_class_get

   Description:
   This API get the traffic class value for a port for a user priority and 
   traffic class.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> user_priority - user priority
   IN -> traffic_class - traffic class
   OUT -> traffic_class_value - traffic class value

   Returns:
   HAL_ERR_L2_QOS_TRAFFIC_CLASS
   HAL_SUCCESS
*/
int
hal_l2_qos_traffic_class_get (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value);
int
hal_l2_traffic_class_status_set (unsigned int ifindex,
                                 unsigned int traffic_class_enabled);


#endif /* _HAL_L2_QOS_H_ */
