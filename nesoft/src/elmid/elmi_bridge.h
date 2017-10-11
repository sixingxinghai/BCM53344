/**@file elmi_bridge.h
 * @brief  This file contains prototypes for elmi bridge apis 
 */
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_BRIDGE_H
#define _PACOS_ELMI_BRIDGE_H

s_int16_t 
elmi_bridge_add (u_int8_t *name, u_int8_t bridge_type, 
                 u_int8_t is_edge);

s_int16_t 
elmi_bridge_delete (u_char *name);

s_int16_t 
elmi_bridge_add_port (char *bridge_name, struct interface *ifp);

s_int16_t 
elmi_bridge_delete_port (u_char *bridge_name, struct interface *ifp);

s_int16_t 
elmi_bridge_vlan_add (u_char *bridge_name, u_int16_t vid,
                      u_char *vlan_name, u_int32_t flags);
s_int16_t 
elmi_bridge_vlan_delete (char *bridge_name, u_int16_t vid);

struct elmi_vlan_bmp * 
elmi_vlan_port_map (u_int32_t ifindex);

s_int16_t 
elmi_bridge_list_delete (void);

s_int16_t 
elmi_bridge_port_state_set (u_char *name, const u_int32_t ifindex,
                            u_char state);

struct elmi_bridge * 
elmi_find_bridge (char *name);

struct elmi_ifp *
elmi_find_bridge_port (struct elmi_bridge *bridge, struct interface *ifp);

struct elmi_vlan *
elmi_bridge_vlan_lookup (struct elmi_bridge *bridge, u_int16_t vid);

void
elmi_bridge_free (struct elmi_bridge *bridge);

#endif /* _PACOS_ELMI_BRIDGE_H */
