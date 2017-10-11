/* Copyright (C) 2003  All Rights Reserved.  */

/*  LAYER 2 Vlan Stacking  */
#ifndef __HAL_VLAN_STACK_H__
#define __HAL_VLAN_STACK_H__

/*
   Name: hal_vlan_stacking_enable
   
   Description:
   This API enables vlan stacking on an interface
   
   Parameters:
   IN -> ifindex - Interface index
   IN -> ethtype - Ethernet type value for the vlan tag
   IN -> stack_mode - Vlan stacking mode

   Returns:
   HAL_SUCCESS on success
   < 0 on failure
*/
extern int
hal_vlan_stacking_enable (u_int32_t ifindex, u_int16_t ethtype,
                          u_int16_t stackmode);

/*
   Name: hal_vlan_stacking_disable
   
   Description:
   This API disables vlan stacking on an interface
   
   Parameters:
   IN -> ifindex - Interface index
   IN -> ethtype - Ethernet type value for the vlan tag
   IN -> stack_mode - Vlan stacking mode

   Returns:
   HAL_SUCCESS on success
   < 0 on failure
*/
extern int
hal_vlan_stacking_disable (u_int32_t ifindex);

#endif /* __HAL_VLAN_STACK_H__ */
