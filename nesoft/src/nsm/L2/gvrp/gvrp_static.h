/* Copyright 2004  All Rights Reserved. */

#ifndef _PACOS_GVRP_STATIC_H
#define _PACOS_GVRP_STATIC_H

void 
gvrp_check_static_vlans(struct nsm_bridge_master *master,
    struct nsm_bridge *bridge);

int
gvrp_port_check_static_vlans (struct nsm_bridge_master *master,
                              struct nsm_bridge *bridge,struct interface *ifp);
void 
gvrp_delete_static_vlan_on_all_ports(struct nsm_bridge_master *master,
    char *bridge_name, int vid);

void 
gvrp_delete_static_vlans_on_port(struct nsm_bridge_master *master,
    struct nsm_bridge *bridge, struct interface *ifp);

int
gvrp_change_port_mode(struct nsm_bridge_master *master, char *bridge_name,
                      struct interface *ifp);

extern s_int32_t 
gvrp_create_static_vlan (struct nsm_bridge_master *master, char *bridge_name,
                         u_int16_t vid);

extern void
gvrp_port_create_static_vlans (struct nsm_bridge *bridge, struct gvrp *gvrp,
                               struct gvrp_port *gvrp_port);

extern int 
gvrp_add_static_vlan(struct nsm_bridge_master *master, char *bridge_name,
                     struct interface *ifp, u_int16_t vid);

extern int 
gvrp_delete_static_vlan(struct nsm_bridge_master *master, 
    char *bridge_name, struct interface *ifp, u_int16_t vid);

#endif /* !_PACOS_GVRP_STATIC_H */
