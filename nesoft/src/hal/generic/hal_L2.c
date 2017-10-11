/* Copyright (C) 2012  All Rights Reserved.

L2 HAL
  like stub for pc compile

*/

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"

void
hal_nsm_lib_cb_register(struct hal_nsm_callbacks *cb)
{
  return;
}

int
hal_bridge_add (char *name, unsigned int is_vlan_aware, enum hal_bridge_type type,
                unsigned char edge, unsigned char beb, unsigned char *mac)
{
  return HAL_SUCCESS;
}

int
hal_bridge_delete (char *name)
{
  return HAL_SUCCESS;
}

int
hal_bridge_add_port (char *name, unsigned int ifindex)
{
  return HAL_SUCCESS;
}

int
hal_bridge_delete_port (char *name, int ifindex)
{
  return HAL_SUCCESS;
}

int
hal_bridge_set_port_state (char *bridge_name,
                           int ifindex, int instance, int state)
{
  return HAL_SUCCESS;
}


int
hal_bridge_flush_dynamic_fdb_by_mac (char *bridge_name,
                                     const unsigned char * const mac,
                                     int maclen)
{
  return HAL_SUCCESS;
}

int
hal_bridge_flush_fdb_by_port(char *name, unsigned int ifindex, unsigned int instance,
                             unsigned short vid, unsigned short svid)
{
  return HAL_SUCCESS;
}

int
hal_bridge_set_state (char *name, u_int16_t enable)
{
  return HAL_SUCCESS;
}

int
hal_bridge_set_ageing_time (char *name, u_int32_t ageing_time)
{
  return HAL_SUCCESS;
}

int
hal_bridge_set_learning (char *name, int learning)
{
  return HAL_SUCCESS;
}

int
hal_bridge_change_vlan_type (char *name, int is_vlan_aware,
                             u_int8_t type)
{
  return HAL_SUCCESS;
}

int
hal_bridge_set_learn_fwd (const char *const bridge_name,const int ifindex,
                          const int instance, const int learn, const int forward)
{
  return HAL_SUCCESS;
}

int
hal_bridge_delete_vlan_from_instance (char *bridge_name, int instance_id,
    unsigned short vlanid)
{
  return HAL_SUCCESS;
}

int
hal_bridge_add_vlan_to_instance (char *bridge_name, int instance_id,
    unsigned short vlanid)
{
  return HAL_SUCCESS;
}

int
hal_port_mirror_set(unsigned int to_ifindex,
                    unsigned int from_ifindex,
                    enum hal_port_mirror_direction direction)
{
  return HAL_SUCCESS;
}

int
hal_port_mirror_unset (unsigned int to_ifindex, unsigned int from_ifindex,
                       enum hal_port_mirror_direction direction)
{
  return HAL_SUCCESS;
}

int
hal_flow_control_set (unsigned int ifindex, unsigned char direction)
{
  return HAL_SUCCESS;
}

int
hal_flow_control_deinit (void)
{
  return  HAL_SUCCESS;
}

int
hal_flow_control_statistics (unsigned int ifindex, unsigned char *direction,
                             int *rxpause, int *txpause)
{
  return HAL_SUCCESS;
}

int
hal_l2_add_fdb (const char * const name, unsigned int ifindex,
                const unsigned char * const mac, int len,
                unsigned short vid, unsigned short svid,
                unsigned char flags, bool_t is_forward)
{
  return HAL_SUCCESS;
}

int
hal_l2_del_fdb (const char * const name, unsigned int ifindex,
                const unsigned char * const mac, int len,
                unsigned short vid, unsigned short svid,
                unsigned char flags)
{
  return HAL_SUCCESS;
}

int
hal_l2_fdb_unicast_get (char *name, char *mac_addr, unsigned short vid,
                        u_int16_t count, struct hal_fdb_entry *fdb_entry)
{
  return HAL_SUCCESS;
}

int
hal_l2_fdb_multicast_get (char *name, char *mac_addr, unsigned short vid,
                          u_int16_t count, struct hal_fdb_entry *fdb_entry)
{
  return HAL_SUCCESS;
}


int
hal_l2_add_priority_ovr (const char * const name, unsigned int ifindex,
                         const unsigned char * const mac, int len,
                         unsigned short vid,
                         unsigned char ovr_mac_type,
                         unsigned char priority)
{
  return HAL_SUCCESS;
}

int
hal_l2_qos_traffic_class_set (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value)
{
  return HAL_SUCCESS;
}

int
hal_l2_qos_traffic_class_get (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value)
{
  return HAL_SUCCESS;
}

int
hal_l2_qos_default_user_priority_set (unsigned int ifindex,
                                      unsigned char user_priority)
{
  return HAL_SUCCESS;
}

int
hal_l2_qos_regen_user_priority_set (unsigned int ifindex,
                                    unsigned char recvd_user_priority,
                                    unsigned char regen_user_priority)
{
  return HAL_SUCCESS;
}

int
hal_l2_qos_set_num_coq (s_int32_t num_cosq)
{
  return HAL_SUCCESS;
}

int
hal_l2_qos_get_num_coq (s_int32_t *num_cosq)
{
  return HAL_SUCCESS;
}

int
hal_l2_traffic_class_status_set (unsigned int ifindex,
                                 unsigned int traffic_class_enabled)
{
 return HAL_SUCCESS;
}



int
hal_l2_bcast_discards_get (unsigned int ifindex,
                           unsigned int *discards)
{
  return HAL_SUCCESS;
}

int
hal_l2_mcast_discards_get (unsigned int ifindex, unsigned int *discards)
{
  return HAL_SUCCESS;
}

int
hal_l2_dlf_bcast_discards_get (unsigned int ifindex, unsigned int *discards)
{
  return HAL_SUCCESS;
}

int
hal_l2_ratelimit_bcast (unsigned int ifindex,
                        unsigned char level, unsigned char fraction)
{
  return HAL_SUCCESS;
}

int
hal_l2_ratelimit_mcast (unsigned int ifindex,
                        unsigned char level, unsigned char fraction)
{
  return HAL_SUCCESS;
}

int
hal_l2_ratelimit_dlf_bcast (unsigned int ifindex,
                            unsigned char level, unsigned char fraction)
{
  return HAL_SUCCESS;
}

int
hal_l2_ratelimit_only_broadcast (unsigned int ifindex,
                                 unsigned char level,
                                 unsigned char fraction)
{
  return HAL_SUCCESS;
}

int
hal_l2_ratelimit_bcast_mcast (unsigned int ifindex,
                              unsigned char level,
                              unsigned char fraction)
{
  return HAL_SUCCESS;
}

int
hal_ratelimit_deinit ()
{
  return HAL_SUCCESS;
}

int
hal_vlan_classifier_add (struct hal_vlan_classifier_rule *rule_ptr,u_int32_t ifindex, u_int32_t refcount)

{
  return HAL_SUCCESS;
}

int
hal_vlan_classifier_del (struct hal_vlan_classifier_rule *rule_ptr,u_int32_t ifindex, u_int32_t refcount)
{
  return HAL_SUCCESS;
}

int
hal_vlan_add (char *name, enum hal_vlan_type type, unsigned short vid)
{
  return HAL_SUCCESS;
}

int
hal_vlan_delete (char *bridge_name, enum hal_vlan_type type, unsigned short vid)
{
  return HAL_SUCCESS;
}

int
hal_vlan_add_vid_to_port (char *name,
                          unsigned int ifindex,
                          unsigned short vid,
                          enum hal_vlan_egress_type egress_tagged)
{
  return HAL_SUCCESS;
}

int
hal_vlan_set_default_pvid (char *name,
                           unsigned int ifindex,
                           unsigned short pvid,
                           enum hal_vlan_egress_type egress_tagged)
{
  return HAL_SUCCESS;
}

int
hal_vlan_delete_vid_from_port (char *name,
                               unsigned int ifindex,
                               unsigned short vid)
{

  return HAL_SUCCESS;
}

int
hal_vlan_set_port_type (char * const bridge_name,
                        unsigned int ifindex,
                        enum hal_vlan_port_type port_type,
                        enum hal_vlan_port_type sub_port_type,
                        enum hal_vlan_acceptable_frame_type acceptable_frame_types,
                        const unsigned short enable_ingress_filter)
{
  return HAL_SUCCESS;
}

int
hal_vlan_port_set_dot1q_state (unsigned int ifindex, unsigned short enable,
                               unsigned short enable_ingress_filter)
{
  return HAL_SUCCESS;
}

int
hal_vlan_set_native_vid (char *name, unsigned int ifindex,
                         unsigned short vid)
{
  return HAL_SUCCESS;
}

int
hal_vlan_set_access_map( struct hal_mac_access_grp *hal_macc_grp,
                        int vid,
                        int action)
{
 return HAL_SUCCESS;
}

void
hal_garp_set_bridge_type (char *bridge_name, unsigned long garp_type, int enable)
{
  return;
}

int
hal_gmrp_set_bridge_ext_filter (char *bridge_name, bool_t enable)
{
  return HAL_SUCCESS;
}

int
hal_l2_gmrp_set_service_requirement (char *bridge_name, int vlanid, int ifindex, int serv_req, bool_t activate)
{
 return HAL_SUCCESS;
}

int
hal_if_sec_hwaddrs_add (char *ifname, unsigned int ifindex,
                        int hw_addr_len, int nAddrs, unsigned char **addresses)
{
 return HAL_SUCCESS;
}

int
hal_if_sec_hwaddrs_delete (char *ifname, unsigned int ifindex,
                            int hw_addr_len, int nAddrs, unsigned char **addresses)
{
 return HAL_SUCCESS;
}

int
hal_if_set_portbased_vlan (unsigned int ifindex, struct hal_port_map pbitmap)
{
  return HAL_SUCCESS;
}

int
hal_mac_set_access_grp( struct hal_mac_access_grp *hal_macc_grp,
                        int ifindex,
                        int action,
                        int dir)
{
 return HAL_SUCCESS;
}


int
hal_l2_unknown_mcast_mode (int mode)
 {
 return HAL_SUCCESS;
 }


