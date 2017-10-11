/* Copyright (C) 2005-2006  All Rights Reserved. */

#ifndef COMMON_PROC_H
#define COMMON_PROC_H

#define NSM_L2_SNMP_MAX_PORTS             32
#define NSM_L2_SNMP_PORT_ARRAY_SIZE       33
#define NSM_SNMP_INVALID_PORT_ID          33
#define HAL_VLAN_NAME_SIZE                16

#define ADMIT_ONLYVLANTAGGED_FRAMES       2
#define ADMIT_ALL_FRAMES                  1 

#include "nsm_bridge.h"
#include "l2lib.h"

typedef u_int16_t vid_t;

struct vlan_db_summary {
  vid_t vid;
  u_int16_t  stacked_vid;
  u_int16_t  vlan_state;
  u_int16_t  vlan_type;
  u_char   vlan_name[HAL_VLAN_NAME_SIZE];
  u_char   config_ports[NSM_L2_SNMP_MAX_PORTS];
};

struct nsm_bridge *
nsm_snmp_get_first_bridge ();

struct interface *
nsm_snmp_bridge_port_lookup (struct nsm_bridge *br, int port_instance,
                             int exact);

struct interface *
nsm_snmp_bridge_port_lookup_by_port_id (struct nsm_bridge *br, int port_id,
                                        int exact,
                                        struct nsm_bridge_port **ret_port);

struct interface *
nsm_snmp_bridge_rule_port_lookup_by_port_id(struct nsm_bridge *br, int port_id,
                                            int exact,
                                            struct nsm_bridge_port **ret_port);
u_int8_t
nsm_snmp_bridge_port_get_port_id (u_int32_t ifindex);

s_int32_t
nsm_snmp_bridge_port_get_ifindex (u_int8_t port_id, u_int32_t *ifindex);

#endif /* COMMON_PROC_H */
