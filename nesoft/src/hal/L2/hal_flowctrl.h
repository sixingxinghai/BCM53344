/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_FLOWCTRL_H_
#define _HAL_FLOWCTRL_H_

/* 
   Name: hal_flow_control_init

   Description:
   This API initializes the flow control hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_FLOWCTRL_INIT
   HAL_SUCCESS
*/
int
hal_flow_control_init (void);

/* 
   Name: hal_flow_control_deinit

   Description:
   This API deinitializes the flow control hardware layer.

   Parameters:
   None

   Returns:
   HAL_ERR_FLOWCTRL_DEINIT
   HAL_SUCCESS
*/
int
hal_flow_control_deinit (void);

/* 
   Name: hal_flow_control_set 
   
   Description:
   This API sets the flow control properties of a port.

   Parameters:
   IN -> ifindex - port ifindex
   IN -> direction - HAL_FLOW_CONTROL_(OFF|SEND|RECEIVE)

   Returns:
   HAL_ERR_FLOW_CONTROL_SET
   HAL_SUCCESS
*/
int
hal_flow_control_set (unsigned int ifindex, unsigned char direction);

/*
   Name: hal_flow_ctrl_pause_watermark_set

   Description:
   This API sets the flow control properties of a port.

   Parameters:
   IN -> port  - port Number
   IN -> wm_pause - WaterMark Pause 

   Returns:
   HAL_ERR_FLOW_CONTROL_SET
   HAL_SUCCESS
*/

int hal_flow_ctrl_pause_watermark_set (u_int32_t port, u_int16_t wm_pause);

/* 
   Name: hal_flow_control_statistics 

   Description:
   This API gets the flow control statistics for a port.

   Parameters:
   IN -> ifindex - port ifindex
   OUT -> direction - HAL_FLOW_CONTROL_(OFF|SEND|RECEIVE)
   OUT -> rxpause - Number of received pause frames
   OUT -> txpause - Number of transmitted pause frames

   Returns:
   HAL_ERR_FLOW_CONTROL
   HAL_SUCCESS
*/
int
hal_flow_control_statistics (unsigned int ifindex, unsigned char *direction,
                             int *rxpause, int *txpause);


struct hal_flow_control_wm
{
  u_int32_t ifindex;
  u_int16_t wm_pause;
};

#endif /* _HAL_FLOWCTRL_H_ */
