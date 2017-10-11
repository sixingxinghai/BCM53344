/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_AUTH_H_
#define _HAL_AUTH_H_

/* 
   Name: hal_auth_init

   Description:
   This API initializes the hardware layer for 802.1x port authentication.

   Parameters:
   None

   Returns:
   HAL_ERR_AUTH_INIT
   HAL_SUCCESS
*/
int
hal_auth_init (void);

/* 
   Name: hal_auth_deinit

   Description:
   This API deinitializes the hardware layer for 802.1x port authentication.

   Parameters:
   None

   Returns:
   HAL_ERR_AUTH_INIT
   HAL_SUCCESS
*/
int
hal_auth_deinit (void);

/* 
   Name: hal_auth_set_port_state

   Description:
   This API sets the authorization state of a port 

   Parameters:
   IN -> index - port index
   IN -> state - port state
   
   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_auth_set_port_state (unsigned int index,
                         enum hal_auth_port_state state);

int
hal_auth_mac_set_port_state (unsigned int index, int mode,
                             enum hal_auth_mac_port_state state);

int
hal_auth_mac_unset_irq (unsigned int index);
#endif /* _HAL_AUTH_H_ */
