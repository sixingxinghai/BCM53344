/* Copyright (C) 2003  */

/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
*/

#ifndef __PACOS_BR_VLAN_H__
#define __PACOS_BR_VLAN_H__

#include "br_types.h"
#include <linux/skbuff.h>


extern vid_t
br_vlan_get_vid_from_frame (unsigned short proto, struct sk_buff *skb);


extern unsigned char
br_vlan_get_ingress_priority (struct apn_bridge *br,
                              unsigned short vlan_tag);


/* This function returns the frame type associated with the frame */
extern br_frame_t
br_vlan_get_frame_type (struct sk_buff *skb, unsigned short proto);

/* This function returns the ethernet type of the frame */
unsigned short
br_vlan_get_nondot1q_ether_type (struct sk_buff *skb);

/* This function returns the frame type associated with the frame */
extern br_frame_t
br_cvlan_get_frame_type (struct sk_buff *skb);

/* This function classifies the incoming frame */
extern vid_t
br_vlan_classify_ingress_frame (unsigned short proto, struct sk_buff *skb);


/* This function checks if the given vid is active set of vids */
extern bool_t
br_vlan_is_vlan_in_active_set (vid_t vid);


/* This function gets the vlan state for the given vid */
extern vlan_state_t
br_vlan_get_vlan_state (struct apn_bridge *br, 
                        vid_t vid);


/* This function gets the list of confgured ports for the given vid */
extern char*
br_vlan_get_vlan_config_ports (struct apn_bridge *br, 
                               enum vlan_type type,
                               vid_t vid);


/* This function checks if the given port is a member of the given vid */
extern bool_t
br_vlan_is_port_in_vlans_member_set (struct apn_bridge *br, 
                                     vid_t vid, 
                                     int port_no);

/* This function applies the ingress rules for the incoming frames on a
   vlan-aware bridge */
extern bool_t
br_vlan_apply_ingress_rules (unsigned short proto,
                             struct sk_buff *skb,
                             vid_t *vid);


extern struct sk_buff * 
br_vlan_push_tag_header (struct sk_buff *skb, 
                         vid_t vid, unsigned short proto);

extern struct sk_buff * 
br_vlan_pop_tag_header (struct sk_buff *skb);

extern void
br_vlan_swap_tag_header (struct sk_buff *skb, 
                         vid_t vid);
extern int
br_vlan_validate_vid (struct apn_bridge *br, vlan_type_t type, unsigned short vid);

extern void
br_vlan_reset_port_type (struct apn_bridge *br);

#endif /* ! */
