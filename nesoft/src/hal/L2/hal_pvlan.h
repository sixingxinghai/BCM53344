/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_PVLAN_H_
#define _HAL_PVLAN_H_
/* 
   Name: hal_set_pvlan_type 

   Description: 
   This API adds the private vlan type to VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> vid - VLAN id
   IN -> hal_pvlan_mode - Private vlan type

   Returns:
   HAL_SUCCESS 
*/
int
hal_set_pvlan_type  (char *bridge_name, unsigned short vid, enum hal_pvlan_type);

/* 
   Name: hal_vlan_associate

   Description:
   This API associate a secondary vlan to a primary vlan.

   Parameters:
   IN -> name - bridge name
   IN -> vid - primary VLAN id
   IN -> pvid - secondary VLAN id
   IN -> associate - to associate or to disassociate 

   Returns:
   HAL_SUCCESS
*/
int
hal_vlan_associate (char *bridge_name, unsigned short vid,
                    unsigned short pvid, int associate);

/*
   Name: hal_pvlan_set_port_mode 

   Description:
   This API sets the port mode as host or promiscuous.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - Interface ifindex
   IN -> hal_pvlan_port_mode 

   Default: Defaults to Promiscuous mode

   Returns:
   HAL_SUCCESS
*/
int
hal_pvlan_set_port_mode (char *bridge_name, int ifindex, enum hal_pvlan_port_mode);

/*
   Name: hal_pvlan_set_port_mode

   Description:
   This API associates primary vlan and secondary vlan to a host port.

   Parameters:
   IN -> vid - Primary VLAN id
   IN -> ifindex - Interface ifindex
   In -> pvid - Secondary VLAN id
   IN -> associate - association or disassociation

   Returns:
   HAL_SUCCESS
*/
int
hal_pvlan_host_association (char *bridge_name, int ifindex,  
                            unsigned short vid, unsigned short pvid,
                            int associate);

#endif /* _HAL_PVLAN_H_ */
