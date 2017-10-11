/*   Copyright (C) 2003  All Rights Reserved.  
   LAYER 2 Vlan Stacking CLI
  
   This module declares the interface to the Layer 2 Vlan stacking CLI
  
*/


#ifndef __L2_VLAN_STACK_H_
#define __L2_VLAN_STACK_H_

#define NSM_VLAN_ETHERTYPE_DEFAULT   0x8100

extern void
nsm_vlan_stack_cli_init (struct cli_tree *ctree);

int nsm_vlan_stack_write (struct cli *, struct interface *);
/* Enable vlan stacking for a port */
int nsm_vlan_stacking_enable (struct interface *ifp, u_int16_t stack_mode, 
                              u_int16_t ethertype);
/* Disable vlan stacking for a port */
int nsm_vlan_stacking_disable (struct interface *ifp);

#endif /* Have __L2_VLAN_STACK_H_ */
