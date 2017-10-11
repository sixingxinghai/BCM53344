/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GMRP_H
#define _PACOS_GMRP_H

#include "lib.h"
#include "garp_pdu.h"

#define GMRP_NUMBER_OF_LEGACY_CONTROL     2
#define GMRP_MAX_MULTICASTS               100

#define GMRP_NUMBER_OF_GID_MACHINES       GMRP_NUMBER_OF_LEGACY_CONTROL + \
                                          GMRP_MAX_MULTICASTS
#define GMRP_UNUSED_INDEX                 GMRP_NUMBER_OF_GID_MACHINES

#define GMRP_MGMT_PORT_CONFIGURED         0x80
#define GMRP_MGMT_FORWARD_ALL_CONFIGURED  0x04

#define GMRP_MGMT_FORWARD_ALL_FORBIDDEN            0x08
#define GMRP_MGMT_FORWARD_UNREGISTERED_CONFIGURED  0x02
#define GMRP_MGMT_FORWARD_UNREGISTERED_FORBIDDEN   0x10
#define NOT_IMPLEMENTED                   2

extern struct lib_globals *gmrpm;

enum gmrp_registration_type
{
  REG_TYPE_GMRP,
  REG_TYPE_MMRP
};

enum gmrp_attribute_type
{
  GMRP_ATTR_ALL,
  GMRP_ATTR_LEGACY,
  GMRP_ATTR_MULTICAST,
  GMRP_ATTR_MAX
};

enum gmrp_legacy_control
{
  GMRP_LEGACY_FORWARD_ALL          = 0,
  GMRP_LEGACY_FORWARD_UNREGISTERED = 1,
  GMRP_LEGACY_MAX                  = 2
};

struct gmrp_vlan
{
  u_int16_t vlanid;
#ifdef HAVE_MMRP
  u_int32_t receive_counters[MRP_ATTR_EVENT_MAX+1];
  u_int32_t transmit_counters[MRP_ATTR_EVENT_MAX+1];
#else
  u_int32_t receive_counters[GARP_ATTR_EVENT_MAX+1];
  u_int32_t transmit_counters[GARP_ATTR_EVENT_MAX+1];
#endif /* HAVE_MMRP */
};

struct gmrp_vlan_table 
{
  struct gmrp_vlan *gmrp_vlan[NSM_VLAN_MAX + 1];
};

struct gmrp
{
  struct gmrp_bridge *gmrp_bridge;
  struct garp_instance garp_instance;
  struct gmrp_gmd *gmd;
  u_int16_t vlanid;
  u_int16_t svlanid;
  struct avl_tree *tree;
  u_char mac_addr[HAL_HW_LENGTH];
};

struct gmrp_bridge
{
  struct nsm_bridge *bridge;
  struct gmrp_vlan_table *gm_vlan_table;
  struct nsm_vlan_listener *gmrp_appln;
  struct nsm_bridge_listener *gmrp_br_appln;
  struct garp garp;
  char globally_enabled;
  enum gmrp_registration_type reg_type;
  int gmrp_last_pdu_origin;
};

struct gmrp_port
{
  void *port;
  u_char flags;
  char globally_enabled;
  u_char forward_all_cfg;
  u_char registration_cfg;
  struct gid_port *gid_port;
  u_char forward_unregistered_cfg;
  u_int32_t gmrp_failed_registrations;
  u_int8_t gmrp_last_pdu_origin [ETHER_ADDR_LEN];
};

struct gmrp_port_instance
{
  u_int16_t vlanid;
  u_int16_t svlanid;
  struct gid *gid;
  struct avl_tree *tree;
};

struct gmrp_port_config
{
  u_char registration;
  u_char fwd_all;
  pal_time_t join_timeout;
  pal_time_t leave_timeout;
  pal_time_t leave_all_timeout;
  bool_t enable_port;
  u_int8_t p2p;
};

extern void
gmrp_init (struct lib_globals *zg);

extern s_int32_t
gmrp_add_vlan_cb (struct nsm_bridge_master *master,
                  char *bridge_name, u_int16_t vlanid);

extern s_int32_t
gmrp_delete_vlan_cb (struct nsm_bridge_master *master,
                     char *bridge_name, u_int16_t vlanid);

extern s_int32_t
gmrp_add_vlan_to_port_cb (struct nsm_bridge_master *master,
                          char *bridge_name, struct interface *ifp,
                          u_int16_t vid);

extern s_int32_t
gmrp_delete_vlan_to_port_cb (struct nsm_bridge_master *master,
                             char *bridge_name, struct interface *ifp,
                             u_int16_t vid);

#ifdef HAVE_PROVIDER_BRIDGE

extern s_int32_t
gmrp_add_swtcx_cb (struct nsm_bridge_master *master,
                   char *bridge_name, u_int16_t vlanid,
                   u_int16_t svlanid);

extern s_int32_t
gmrp_del_swtcx_cb (struct nsm_bridge_master *master,
                   char *bridge_name, u_int16_t vlanid,
                   u_int16_t svlanid);

extern s_int32_t
gmrp_add_swtcx_to_port_cb (struct nsm_bridge_master *master,
                           char *bridge_name, struct interface *ifp,
                           u_int16_t vid, u_int16_t svid);

extern s_int32_t
gmrp_del_swtcx_from_port_cb (struct nsm_bridge_master *master,
                             char *bridge_name, struct interface *ifp,
                             u_int16_t vid, u_int16_t svid);

#endif /* HAVE_PROVIDER_BRIDGE */

extern bool_t
gmrp_create_port_instance (struct avl_tree *tree, struct gid_port *gid_port,
                           struct gmrp *gmrp,
                           struct gmrp_port_instance **gmrp_port_instance);

extern bool_t
gmrp_create_port (struct interface *ifp, char globally_enabled,
                  struct gmrp_port **gmrp_port);


extern bool_t
gmrp_create_vlan (struct gmrp_bridge *gmrp_bridge);

extern void
gmrp_destroy_port (struct gmrp_port *old_gmrp_port);

extern void
gmrp_destroy_port_instance (struct gmrp_port_instance *old_gmrp_port_instance);

extern void
gmrp_free_port_instance (struct gmrp_port_instance *old_gmrp_port_instance);

extern bool_t
gmrp_create_gmrp_instance (struct avl_tree *tree, struct gmrp_bridge *bridge,
                           u_int16_t vlanid, u_int16_t svlanid, struct gmrp **gmrp);

extern void
gmrp_destroy_gmrp_instance (struct gmrp *gmrp);

extern void
gmrp_free_gmrp_instance (struct gmrp *gmrp);

extern bool_t
gmrp_create_gmrp_bridge (struct nsm_bridge *bridge, char globally_enabled, 
                         char *protocol, struct gmrp_bridge **gmrp_bridge );

void
gmrp_destroy_gmrp_bridge (struct nsm_bridge *bridge, 
                          struct gmrp_bridge *gmrp_bridge);

extern void
gmrp_join_indication (void *application, struct gid *gid, 
                      u_int32_t joining_gid_index);

extern void
gmrp_join_propagated (void *application, struct gid *gid, 
                      u_int32_t joining_gid_index);

extern void
gmrp_leave_indication (void *application, struct gid *gid, 
                       u_int32_t leaving_gid_index);

extern void
gmrp_leave_propagated (void *application, struct gid *gid, 
                       u_int32_t leaving_gid_index);

extern void
gmrp_do_actions (struct gid *gid);

extern struct nsm_bridge *
gmrp_get_bridge_instance (void *application);

u_int16_t
gmrp_get_vid (void *application);

u_int16_t
gmrp_get_svid (void *application);

extern struct gid *
gmrp_get_gid (struct nsm_bridge_port *br_port, u_int16_t vlanid,
              u_int16_t svlanid);

extern void
gmrp_add_static_mac_addr (struct nsm_bridge_master *master, 
                          char *bridge_name, struct interface *ifp,
                          unsigned char *  mac, u_int16_t vid,
                          u_int16_t svid, bool_t is_forward);
 
extern void
gmrp_delete_static_mac_addr (struct nsm_bridge_master *master, 
                             char *bridge_name, struct interface *ifp,
                             unsigned char *  mac, u_int16_t vid,
                             u_int16_t svid, bool_t is_forward);
                           
void
gmrp_inject_event_all_ports (struct nsm_bridge_master *master, 
                             char *bridge_name, u_int16_t vid, 
                             u_int16_t svid, struct interface *ifp,
                             u_int32_t gid_index, gid_event_t event);
#endif /* !_PACOS_GMRP_H */
