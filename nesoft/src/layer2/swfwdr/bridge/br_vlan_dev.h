/* Copyright (C) 2003  All Rights Reserved.
 *   
 * VLAN net device
 */
#ifndef _PACOS_BR_VLAN_DEV_H
#define _PACOS_BR_VLAN_DEV_H

int br_vlan_dev_create (struct vlan_info_entry *, enum vlan_type type);

void br_vlan_dev_destroy (struct vlan_info_entry *);

void br_vlan_dev_add_port (struct vlan_info_entry *, struct apn_bridge_port *);

void br_vlan_dev_del_port (struct vlan_info_entry *, enum vlan_type type,
                           struct apn_bridge_port *);

void br_vlan_dev_l2_port_change_addr (struct apn_bridge_port *);

void br_vlan_dev_change_flags (struct apn_bridge *br,
                               struct apn_bridge_port *port, int instance);

#endif /* _PACOS_BR_VLAN_DEV_H */
