/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _NSM_PVLAN_H
#define _NSM_PVLAN_H
#include "hal_types.h"

enum nsm_pvlan_type
  {
    NSM_PVLAN_NONE,
    NSM_PVLAN_COMMUNITY,
    NSM_PVLAN_ISOLATED,
    NSM_PVLAN_PRIMARY
  };

/* VLAN port mode. */
enum nsm_pvlan_port_mode
  {
    NSM_PVLAN_PORT_MODE_INVALID,
    NSM_PVLAN_PORT_MODE_HOST,
    NSM_PVLAN_PORT_MODE_PROMISCUOUS
  };

#define NSM_PVLAN_PORT_NOT_CONFIGURED      0
#define NSM_PVLAN_PORT_CONFIGURED          1

#define NSM_PVLAN_NOT_CONFIGURED         0
#define NSM_PVLAN_CONFIGURED             1

#define NSM_PVLAN_ERR_BASE                  -50

#define NSM_PVLAN_ERR_INVALID_MODE         (NSM_PVLAN_ERR_BASE + 1)
#define NSM_PVLAN_ERR_NOT_CONFIGURED       (NSM_PVLAN_ERR_BASE + 2)
#define NSM_PVLAN_ERR_NOT_ISOLATED_VLAN    (NSM_PVLAN_ERR_BASE + 3)
#define NSM_PVLAN_ERR_NOT_COMMUNTITY_VLAN  (NSM_PVLAN_ERR_BASE + 4)
#define NSM_PVLAN_ERR_NOT_PRIMARY_VLAN     (NSM_PVLAN_ERR_BASE + 5)
#define NSM_PVLAN_ERR_PRIMARY_SECOND_SAME  (NSM_PVLAN_ERR_BASE + 6)
#define NSM_PVLAN_ERR_SECOND_NOT_ASSOCIATED (NSM_PVLAN_ERR_BASE + 7)
#define NSM_PVLAN_ERR_PORT_NOT_CONFIGURED  (NSM_PVLAN_ERR_BASE + 8)
#define NSM_PVLAN_ERR_NOT_HOST_PORT        (NSM_PVLAN_ERR_BASE + 9)
#define NSM_PVLAN_ERR_NOT_PROMISCUOUS_PORT (NSM_PVLAN_ERR_BASE + 10)
#define NSM_PVLAN_ERR_ISOLATED_VLAN_EXISTS (NSM_PVLAN_ERR_BASE + 11)
#define NSM_PVLAN_ERR_SECONDARY_NOT_MEMBER (NSM_PVLAN_ERR_BASE + 12)
#define NSM_PVLAN_ERR_NOT_SECONDARY_VLAN   (NSM_PVLAN_ERR_BASE + 13)
#define NSM_PVLAN_AGGREGATOR_PORT          (NSM_PVLAN_ERR_BASE + 14)
#define NSM_PVLAN_ERR_ASSOCIATED_TO_PRIMARY (NSM_PVLAN_ERR_BASE + 15)
#define NSM_PVLAN_ERR_PVLAN_CONFIGURED     (NSM_PVLAN_ERR_BASE + 16)
#define NSM_NO_VLAN_CONFIGURED             (NSM_PVLAN_ERR_BASE + 17)
#define NSM_PVLAN_ERR_PROVIDER_BRIDGE      (NSM_PVLAN_ERR_BASE + 18)
int
nsm_pvlan_configure (struct nsm_bridge_master *master, char *brname,
                     u_int16_t vid,  enum nsm_pvlan_type type);
 
int
nsm_pvlan_configure_clear (struct nsm_bridge_master *master, char *brname,
                           u_int16_t vid, u_char type); 

int
nsm_pvlan_associate (struct nsm_bridge_master *master, char *brname,
                     u_int16_t vid, u_int16_t pvid);
 
int
nsm_pvlan_associate_clear (struct nsm_bridge_master *master, char *brname,
                           u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_associate_clear_all (struct nsm_bridge_master *master,
                               char *brname, u_int16_t vid);

int
nsm_pvlan_api_set_port_mode (struct interface *ifp,
                             enum nsm_pvlan_port_mode mode);

int
nsm_pvlan_api_clear_port_mode (struct interface *ifp,
                               enum nsm_pvlan_port_mode mode);

int
nsm_pvlan_api_host_association (struct interface *ifp,
                                u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_api_host_association_clear (struct interface *ifp,
                                      u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_api_host_association_clear_all (struct interface *ifp);

int
nsm_pvlan_api_switchport_mapping (struct interface *ifp, 
                                  u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_api_switchport_mapping_clear (struct interface *ifp,
                                        u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_api_switchport_mapping_clear_all (struct interface *ifp);

int
nsm_pvlan_api_mapping (u_int32_t vr_id, char *ifname, 
                       u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_api_mapping_clear (u_int32_t vr_id, char *name,
                             u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_api_mapping_clear_all (u_int32_t vr_id, char *name);

int
nsm_pvlan_set_port_fast_bpdu_guard (struct nsm_bridge *bridge,
                                    struct interface *ifp,
                                    bool_t enable);
int
nsm_pvlan_remove_port_association (struct nsm_bridge *bridge,
                                   u_int16_t vid, u_int16_t pvid);

int
nsm_pvlan_delete_all (struct nsm_bridge *bridge, u_int16_t vid);

void
nsm_pvlan_cli_init (struct cli_tree *ctree);
#endif /* _NSM_PVLAN_H */
