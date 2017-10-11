/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _HSL_BCM_PVLAN_H_
#define _HSL_BCM_PVLAN_H_

#ifdef HAVE_PVLAN

#define HSL_PVLAN_OFFSET    14
#define HSL_PVLAN_MASK      0x0fff 

struct hsl_bcm_pvlan
{
  hsl_vid_t vid; 
  int vlan_type;
  
  int filter_type;
  union
  {
    bcm_filterid_t filter;
    struct
    {
      bcm_field_entry_t entry;
      bcm_field_group_t group;
    }field;
  }u;
};

int hsl_bcm_pvlan_set_vlan_type (struct hsl_bridge *bridge,
                                 struct hsl_vlan_port *vlan,
                                 enum hal_pvlan_type vlan_type);

int hsl_bcm_pvlan_add_vlan_association (struct hsl_bridge *bridge,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan);

int hsl_bcm_pvlan_del_vlan_association (struct hsl_bridge *bridge,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan);

int hsl_bcm_pvlan_add_port_association (struct hsl_bridge *bridge,
                                        struct hsl_port_vlan *port_vlan,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan);

int hsl_bcm_pvlan_del_port_association (struct hsl_bridge *bridge,
                                        struct hsl_port_vlan *port_vlan,
                                        struct hsl_vlan_port *prim_vlan,
                                        struct hsl_vlan_port *second_vlan);

int hsl_bcm_pvlan_set_port_mode (struct hsl_bridge *bridge,
                                 struct hsl_bridge_port *port,
                                 enum hal_pvlan_port_mode port_mode); 
#endif /* HAVE_PVLAN */

#endif /* _HSL_BCM_PVLAN_H_ */
