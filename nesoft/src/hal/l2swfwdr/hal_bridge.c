/* Copyright (C) 2003  All Rights Reserved.
  
LAYER 2 BRIDGE HAL
  
This module defines the platform abstraction layer to the 
Linux layer 2 bridging.

*/

#include "pal.h"
#include <errno.h>
#include "lib.h"
#include "filter.h"
#include "hal_acl.h"
#include "hal_incl.h"
#include "hal_comm.h"
#include "hal_ipifwd.h"
#ifdef HAVE_IGMP_SNOOP
#include "hal_igmp_snoop.h"
#endif /* HAVE_IGMP_SNOOP */
#ifdef HAVE_MLD_SNOOP
#include "hal_mld_snoop.h"
#endif /* HAVE_MLD_SNOOP */

int
hal_igmp_snooping_add_entry (char *bridge_name,
                             struct hal_in4_addr *src,
                             struct hal_in4_addr *group,
                             char is_exclude,
                             int vid,
                             int svid,
                             int count,
                             unsigned int *ifindexes)
{
  u_int8_t mac [HAL_HW_LENGTH];

  if (! bridge_name || ! src || ! group || !ifindexes)
    return HAL_ERR_INVALID_ARG;

  HAL_CONVERT_IPV4MCADDR_TO_MAC (group, mac);

  return hal_l2_add_fdb (bridge_name, ifindexes [0], mac, HAL_HW_LENGTH,
                         vid, svid, HAL_L2_FDB_STATIC, PAL_TRUE);
}

int
hal_igmp_snooping_delete_entry (char *bridge_name,
                                struct hal_in4_addr *src,
                                struct hal_in4_addr *group,
                                char is_exclude,
                                int vid,
                                int svid,
                                int count,
                                unsigned int *ifindexes)
{
  u_int8_t mac [HAL_HW_LENGTH];

  if (! bridge_name || ! src || ! group || !ifindexes)
    return HAL_ERR_INVALID_ARG;

  HAL_CONVERT_IPV4MCADDR_TO_MAC (group, mac);

  return hal_l2_del_fdb (bridge_name, ifindexes [0], mac, HAL_HW_LENGTH,
                         vid, svid, HAL_L2_FDB_STATIC);
}

int
hal_igmp_snooping_if_enable (char *name, unsigned int ifindex)
{
  return HAL_SUCCESS;
}

#ifdef HAVE_MLD_SNOOP
int
hal_mld_snooping_delete_entry (char *bridge_name,
                               struct hal_in6_addr *src,
                               struct hal_in6_addr *group,
                               char is_exclude, 
                               int vid,
                               int svid,
                               int count,
                               unsigned int *ifindexes)
{
  return 0;
}

int
hal_mld_snooping_add_entry (char *bridge_name,
                            struct hal_in6_addr *src,
                            struct hal_in6_addr *group,
                            char is_exclude,
                            int vid,
                            int svid,
                            int count,
                            unsigned int *ifindexes)
{
  return 0;
}

#endif /* HAVE_MLD_SNOOP */

/* Create a bridge instance */
int
hal_bridge_add (char *name, unsigned int is_vlan_aware, enum hal_bridge_type type,
                unsigned char edge, unsigned char beb, unsigned char* mac)
{
  int ret;
  char str[HAL_BRIDGE_NAME_LEN];

  memset (str, '\0', HAL_BRIDGE_NAME_LEN);
  pal_strncpy (str, name, HAL_BRIDGE_NAME_LEN);

  ret = hal_ioctl (APNBR_ADD_BRIDGE, (unsigned long)str, is_vlan_aware, 
                   type, edge, beb, 0);
  if (ret < 0)
    {
      return -errno;
    }

  return ret;
}

#ifdef HAVE_I_BEB
int
hal_pbb_dispatch_service_cnp(char *br_name, unsigned int ifindex, unsigned isid,
                         unsigned short svid_h, unsigned short svid_l,
                         unsigned char srv_type)
{
return 0;
}


int
hal_pbb_dispatch_service_vip(char *br_name, unsigned int ifindex, unsigned isid,
                             unsigned char* macaddr, unsigned char srv_type)
{
return 0;
}

int 
hal_pbb_dispatch_service_pip(char *br_name, unsigned int ifindex, unsigned isid)
{
return 0;
}

#endif

#ifdef HAVE_B_BEB
int 
hal_pbb_dispatch_service_cbp(char *br_name, unsigned int ifindex, 
                             unsigned short bvid, unsigned int e_isid, 
                             unsigned int l_isid, unsigned char *default_dst_bmac,
                             unsigned char srv_type)
{
return 0;
}

#endif

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)

int
hal_pbb_remove_service(char *br_name, unsigned int ifindex, unsigned isid)
{
return 0;
}

#endif

/* Delete a bridge instance */

int
hal_bridge_delete (char *name)
{
  int ret;
  char str[HAL_BRIDGE_NAME_LEN];

  memset (str, '\0', HAL_BRIDGE_NAME_LEN);
  pal_strncpy (str, name, HAL_BRIDGE_NAME_LEN);


  ret = hal_ioctl (APNBR_DEL_BRIDGE, (unsigned long)str, 0, 0, 0, 0, 0);
  if (ret < 0)
    return -errno;

  return HAL_SUCCESS;
}

/*Change the bridge type */
int
hal_bridge_change_vlan_type (char *name, int is_vlan_aware,u_int8_t type)
{
  int ret;
  char str[HAL_BRIDGE_NAME_LEN];

  memset (str, '\0', HAL_BRIDGE_NAME_LEN);

  pal_strncpy (str, name, HAL_BRIDGE_NAME_LEN);

  ret = hal_ioctl (APNBR_CHANGE_VLAN_TYPE, (unsigned long)str,
                   is_vlan_aware, type, 0, 0, 0);
  if (ret < 0)
    return -errno;

  return ret;
}

/* Set the bridge dynamic ageing time (in seconds) */

int
hal_bridge_set_ageing_time (char *name, u_int32_t ageing_time)
{
  int ret;

  ret = hal_ioctl (APNBR_SET_AGEING_TIME, (unsigned long)name, ageing_time, 
                   0, 0, 0, 0);
  if (ret < 0)
    return -errno;

  return HAL_SUCCESS;
}

int 
hal_bridge_disable_ageing(char *name)
{
  int ret;

  ret = hal_ioctl (APNBR_DISABLE_AGEING, (unsigned long)name, 0, 0, 0, 0, 0);

  if (ret < 0)
    return -errno;

  return HAL_SUCCESS;
}

/* Set the bridge state (in seconds) */
int
hal_bridge_set_state (char *name, u_int16_t enable)
{
  return HAL_SUCCESS;
}

/* Set the bridge learning interval (in seconds). */

int
hal_bridge_set_learning (char *name, int learning)
{
  int ret;

  /* The FID dependancy is not implemented in the software forwarder Yet.
     so ignore it. */
  ret = hal_ioctl (APNBR_SET_BRIDGE_LEARNING, (unsigned long)name, learning, 
                   0, 0, 0, 0);
  if (ret < 0)
    return -errno;

  return HAL_SUCCESS;
}

/* Add a static mac entry if it does not already exist for the specified port. */

int
hal_l2_add_fdb (const char * const name, unsigned int ifindex, 
                const unsigned char * const mac, int len,
                unsigned short vid, unsigned short svid,
                unsigned char flags, bool_t is_forward)
{
  int ret;

 // int port = if_nametoindex ((const char *)ifName);

   // if (ifindex < 0)
   //   return -errno;
      
  
  if ( flags & HAL_L2_FDB_STATIC )
    ret = hal_ioctl (APNBR_ADD_STATFDB_ENTRY, (unsigned long)name, ifindex, 
                     (unsigned long)mac, is_forward, vid, svid);
  else
    ret = hal_ioctl (APNBR_ADD_DYNAMIC_FDB_ENTRY, (unsigned long)name,
                     (unsigned long)ifindex, (unsigned long)mac,
                     is_forward, (unsigned long)vid, svid);
    
  if (ret < 0)
    return -errno;

  return HAL_SUCCESS;
}

/* Remove a static mac entry if it exists. */

int
hal_l2_del_fdb (const char * const name, unsigned int ifindex, 
                const unsigned char * const mac, int len,
                const unsigned short vid, const unsigned short svid,
                unsigned char flags)
{
  int ret;

  if (flags & HAL_L2_FDB_STATIC)
    ret = hal_ioctl (APNBR_DEL_STATFDB_ENTRY, (unsigned long)name, ifindex, 
                        (unsigned long)mac, (unsigned long) vid,
                        (unsigned long)svid, 0);
  else
    ret = hal_ioctl (APNBR_DEL_DYNAMIC_FDB_ENTRY, (unsigned long)name,
                        (unsigned long)ifindex, (unsigned long)mac,
                        (unsigned long)vid, (unsigned long)svid, 0);
  if (ret < 0)
    return -errno;

  return HAL_SUCCESS;
}

/* Read the dynamic fdb entries. Requires that fdbs point to
   an array of cardinality HAL_BRIDGE_MAX_DYN_FDB_ENTRIES. */

int
hal_bridge_read_fdb (const char * const name, struct hal_fdb_entry *fdbs)
{
  int ret;
  memset (fdbs, '\0', 
          sizeof (struct hal_fdb_entry) * HAL_BRIDGE_MAX_DYN_FDB_ENTRIES);

  ret = hal_ioctl (APNBR_GET_DYNFDB_ENTRIES, (unsigned long)name,
                      (unsigned long)fdbs, HAL_BRIDGE_MAX_DYN_FDB_ENTRIES, 
                      0, 0, 0);
  if (ret < 0)
    {
      return -errno;
    }
  return ret;
}

/* Read the static fdb entries. Requires that fdbs argument
   point to an array of cardinality HAL_BRIDGE_MAX_STATIC_FDB_ENTRIES! */

int
hal_bridge_read_statfdb (const char * const name, struct hal_fdb_entry *fdbs)
{
  int ret;
  memset (fdbs, '\0', 
          sizeof (struct hal_fdb_entry) * HAL_BRIDGE_MAX_STATIC_FDB_ENTRIES);

  ret = hal_ioctl (APNBR_GET_STATFDB_ENTRIES, (unsigned long)name,
                      (unsigned long)fdbs, HAL_BRIDGE_MAX_STATIC_FDB_ENTRIES, 
                      0, 0, 0);
  if (ret < 0) 
    {
      return -errno;
    }
  return ret;
}

/* Add an port to a bridge instance. */

int
hal_bridge_add_port (char *name, unsigned int ifindex)
{

  if (hal_ioctl (APNBR_ADD_IF, (unsigned long)name, (int)ifindex, 
                 0, 0, 0, 0) < 0)
    {
      return -errno;
    }
  return HAL_SUCCESS;
}

/* Delete an port from a bridge instance */

int
hal_bridge_delete_port (char *name, int ifindex)
{
  if (hal_ioctl (APNBR_DEL_IF, (unsigned long)name, (int)ifindex, 
                 0, 0, 0, 0) < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_bridge_set_port_state (char *bridge_name,
                           int ifindex, int instance, int state)
{
  if (hal_ioctl (APNBR_SET_PORT_STATE, (unsigned long)bridge_name,
                 ifindex, instance, state, 0, 0) < 0)
    {
      return -errno;
    }
 
  return HAL_SUCCESS;
}

int
hal_bridge_flush_fdb_by_port (char *bridge_name,
                              unsigned int ifindex,
                              unsigned int instance,
                              unsigned short vid,
                              unsigned short svid)
{
  /*If instance is not required 0 should be passed from the caller API in NSM */
  if (hal_ioctl (APNBR_FLUSH_FDB_BY_PORT, (unsigned long)bridge_name, 
                 ifindex, instance, vid, svid, 0) < 0)
    {
      perror ("pal: flush_fdb");
      return -errno;
    }

  return HAL_SUCCESS;
}

/* Enable auth-mac on the port */
int
hal_bridge_auth_mac_enabled_on_port (char *bridge_name,
                                     unsigned int ifindex,
                                     unsigned short vid)
{
  int instance = 0;
  if (hal_ioctl (APNBR_FLUSH_FDB_BY_PORT, (unsigned long)bridge_name,
                 ifindex, instance, vid, 0, 0) < 0)
    {
      perror ("pal: flush_fdb");
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_bridge_set_learn_fwd (const char *const bridge_name,const int ifindex, 
                          const int instance, const int learn, const int forward)
{
  if (hal_ioctl (APNBR_SET_PORT_FWDER_FLAGS, (unsigned long)bridge_name, 
                 ifindex, instance, learn, forward, 0) < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

/* L2 QOS Api */
int hal_l2_qos_init (void)
{
  return HAL_SUCCESS;
}

int hal_l2_qos_deinit (void)
{
  return HAL_SUCCESS;
}

int hal_l2_qos_default_user_priority_set (unsigned int ifindex,
                                      unsigned char user_priority)
{
  return HAL_SUCCESS;
}

int hal_l2_qos_default_user_priority_get (unsigned int ifindex,
                                      unsigned char *user_priority)
{
  *user_priority = 0;
  return HAL_SUCCESS;
}

int hal_l2_qos_regen_user_priority_set (unsigned int ifindex, 
                                    unsigned char recvd_user_priority,
                                    unsigned char regen_user_priority)
{
  return HAL_SUCCESS;
}

int hal_l2_qos_regen_user_priority_get (unsigned int ifindex,
                                    unsigned char *regen_user_priority)
{
  *regen_user_priority = 0;
  return HAL_SUCCESS;
}

int hal_l2_qos_traffic_class_set (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value)
{
  return HAL_SUCCESS;
}

int hal_l2_qos_traffic_class_get (unsigned int ifindex,
                              unsigned char user_priority,
                              unsigned char traffic_class,
                              unsigned char traffic_class_value)
{
  return HAL_SUCCESS;
}

int hal_l2_qos_get_num_coq (s_int32_t *num_cosq)
{
  *num_cosq = 0;
  return HAL_SUCCESS;
}

int hal_l2_qos_set_num_coq (s_int32_t num_cosq)
{
  return HAL_SUCCESS;
}

/* Vlan related pal functions */
int
hal_vlan_add (char *name, enum hal_vlan_type type, unsigned short vid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_ADD, (unsigned long)name, type,
                   (unsigned long)vid, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_delete (char *bridge_name, enum hal_vlan_type type, unsigned short vid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_DEL, (unsigned long)bridge_name, type,
                   (unsigned long)vid, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_disable (char *bridge_name, enum hal_vlan_type type, unsigned short vid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_DISABLE, (unsigned long)bridge_name, type,
                   (unsigned long)vid, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_enable (char *bridge_name, enum hal_vlan_type type, unsigned short vid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_ENABLE, (unsigned long)bridge_name, type,
                   (unsigned long)vid, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

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
  int ret;
  
  ret = hal_ioctl (APN_VLAN_SET_PORT_TYPE, (unsigned long)bridge_name,
                   (unsigned long)ifindex, (unsigned long)port_type,
                   (unsigned long) sub_port_type,
                   (unsigned long)acceptable_frame_types, 
                   (unsigned long)enable_ingress_filter);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_set_default_pvid (char *name,
                           unsigned int ifindex,
                           unsigned short pvid,
                           enum hal_vlan_egress_type egress_tagged)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_SET_DEFAULT_PVID, (unsigned long)name,
                   (unsigned long)ifindex, (unsigned long)pvid, 
                   (unsigned long)egress_tagged, 0, 0);
  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_set_native_vid (char *name,
                         unsigned int ifindex,
                         unsigned short native_vid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_SET_NATIVE_VID, (unsigned long)name,
                   (unsigned long)ifindex, (unsigned long)native_vid,
                   0, 0, 0);
  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_add_vid_to_port (char *name,
                          unsigned int ifindex,
                          unsigned short vid,
                          enum hal_vlan_egress_type egress_tagged)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_ADD_VID_TO_PORT, (unsigned long)name,
                   (unsigned long)ifindex, (unsigned long)vid,
                   (unsigned long)egress_tagged, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_delete_vid_from_port (char *name,
                               unsigned int ifindex,
                               unsigned short vid)
                     
{
  int ret;

  ret = hal_ioctl (APN_VLAN_DEL_VID_FROM_PORT, (unsigned long)name,
                   (unsigned long)ifindex, (unsigned long)vid, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

#ifdef HAVE_IGMP_SNOOP
int
hal_igmp_snooping_enable (char* bridge_name)
{
  int ret;
 
  ret = hal_ioctl (APNBR_ENABLE_IGMP_SNOOPING, (unsigned long)bridge_name, 0,0,0,0,0);

  if (ret < 0)
    {
      return -errno;
    }
  return HAL_SUCCESS;
}

int
hal_igmp_snooping_disable (char* bridge_name)
{
  int ret;

  ret = hal_ioctl (APNBR_DISABLE_IGMP_SNOOPING, (unsigned long)bridge_name,0,0,0,0,0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
int
hal_mld_snooping_enable (char* bridge_name)
{
  return HAL_SUCCESS;
}

int
hal_mld_snooping_disable (char* bridge_name)
{
  return HAL_SUCCESS;
}
#endif /* HAVE_MLD_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
/* This function sets multicast filtering mode for vlan */
/* Note : Only supported with BCM */
int
hal_l2_unknown_mcast_mode (int mode)
{
    return HAL_SUCCESS;
}
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

/* This function notifies the forwarder about the mapping 
   of the VLAN id to the instance.
*/
int
hal_bridge_add_vlan_to_instance (char *bridge_name, int instance_id,
    unsigned short vlanid)
{ 

  int ret;

  ret = hal_ioctl (APNBR_ADD_VLAN_TO_INST, (unsigned long )bridge_name,
                   (unsigned long)instance_id, (unsigned long)vlanid,
                   0, 0, 0);
  
  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;

}

/* Delete VLAN from instance. */
int
hal_bridge_delete_vlan_from_instance (char *bridge_name, int instance_id,
    unsigned short vlanid)
{
#if 0
  int ret;

  ret = hal_ioctl (APNBR_DELETE_VLAN_FROM_INST, (unsigned long )bridge_name,
                      (unsigned long)instance_id, (unsigned long)vlanid,
                      0, 0);
  
  if (ret < 0)
    {
      return -errno;
    }
#endif

  return HAL_SUCCESS;
}

int
hal_vlan_stacked_enable (const char * const bridge_name,
                         const int ifindex,
                         const unsigned short vid)
{
  return HAL_SUCCESS;
}

int
hal_vlan_stacked_disable (const char * const bridge_name,
                          const int ifindex)
{
  return HAL_SUCCESS;
}

/* This function is for colleting port HC stats 
 */
struct port_HC_stats * 
hal_get_port_HC_stats(const int ifindex, const int vlanid, const char * const
                      bridge_name)
{ 

  return NULL;

}

/* This function is for colleting port stats 
 */
struct port_stats * 
hal_get_port_stats(const int ifindex, const int vlanid, const char * const
                   bridge_name)
{ 

  return NULL;

}

/* Rate Limit Interface */
int
hal_ratelimit_init ()
{
  return HAL_SUCCESS;
}

int
hal_ratelimit_deinit ()
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
hal_l2_bcast_discards_get (unsigned int ifindex, unsigned int *discards)
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
hal_l2_mcast_discards_get (unsigned int ifindex, unsigned int *discards)
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
hal_l2_dlf_bcast_discards_get (unsigned int ifindex, unsigned int *discards)
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

/* This function mirror one port to another port 
 */
int
hal_port_mirror_set(unsigned int to_ifindex,
                    unsigned int from_ifindex,
                    enum hal_port_mirror_direction direction)
{ 
  return HAL_ERR_PMIRROR_SET;
}

/* This function mirror one port to another port 
 */
int
hal_port_mirror_unset(unsigned int to_ifindex,
                      unsigned int from_ifindex,
                      enum hal_port_mirror_direction direction)
{ 
  return HAL_ERR_PMIRROR_UNSET;
}

int
hal_flow_control_set( unsigned int ifindex,
                      unsigned char direction)
{ 
  return HAL_ERR_FLOW_CONTROL_SET;
}

void
hal_flow_control_statictics(unsigned int ifindex, 
                            unsigned char *direction,
                            int *rxpause,
                            int *txpause)
{
  *direction = 0;
  *rxpause = 0;
  *txpause = 0;
}

int
hal_flow_control_init ()
{
  return HAL_SUCCESS;
}

int
hal_flow_control_statistics (unsigned int ifindex, unsigned char *direction, 
                             int *rxpause, int *txpause)
{
  return HAL_SUCCESS;
}

int
hal_flow_control_deinit ()
{
  return HAL_SUCCESS;
}

int
hal_vlan_classifier_init()
{
  return HAL_SUCCESS;
}

int
hal_vlan_classifier_deinit()
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
hal_vlan_set_classification_group (const char * const bridge_name,
                             const int ifindex, const unsigned short pvid,
                             const int group)
{
  return HAL_SUCCESS;
}

int
hal_vlan_clr_classification_group (const char * const bridge_name,
                             const int ifindex, const unsigned short pvid,
                             const int group)
{
  return HAL_SUCCESS;
}

#ifdef HAVE_VLAN_STACK
int
hal_vlan_stacking_enable (u_int32_t ifindex, u_int16_t ethtype,
                          u_int16_t stackmode)
{
  return HAL_SUCCESS;
}


int
hal_vlan_stacking_disable (u_int32_t ifindex)
{
  return HAL_SUCCESS;
}
#endif /* HAVE_VLAN_STACK */

#ifdef HAVE_L2LERN
int
hal_mac_set_access_grp( struct hal_mac_access_grp *hal_macc_grp,
                        int ifindex,
                        int action,
                        int dir)
{
  return HAL_SUCCESS;
}

#ifdef HAVE_VLAN
int
hal_vlan_set_access_map( struct hal_mac_access_grp *hal_macc_grp,
                        int vid,
                        int action)
{
  return HAL_SUCCESS;
}
#endif /* HAVE_VLAN */
#endif /* HAVE_L2LERN */

int
hal_ip_set_access_group (struct hal_ip_access_grp access_grp,
                         char *ifname, int action, int dir)
{
  return HAL_SUCCESS;
}

int
hal_ip_set_access_group_interface (struct hal_ip_access_grp access_grp,
                                   char *vifname, char *ifname,
                                   int action, int dir)
{
  return HAL_SUCCESS;
}

int 
hal_get_master_cpu_entry (unsigned char *cpu_entry);

#ifdef HAVE_PROVIDER_BRIDGE

int
hal_vlan_create_cvlan_registration_entry (char *name, unsigned int ifindex,
                                          unsigned short cvid,
                                          unsigned short svid)
{
  int ret;

  ret = hal_ioctl (APNBR_ADD_CVLAN_REG_ENTRY, (unsigned long)name, 
                   ifindex, cvid, svid, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_delete_cvlan_registration_entry (char *name, unsigned int ifindex,
                                          unsigned short cvid,
                                          unsigned short svid)
{
  int ret;

  ret = hal_ioctl (APNBR_DEL_CVLAN_REG_ENTRY, (unsigned long)name, 
                   ifindex, cvid, svid, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_create_vlan_trans_entry (char *name, unsigned int ifindex,
                                   unsigned short vid,
                                   unsigned short trans_vid)
{
  int ret;

  ret = hal_ioctl (APNBR_ADD_VLAN_TRANS_ENTRY, (unsigned long)name, 
                   ifindex, vid, trans_vid, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_delete_vlan_trans_entry (char *name, unsigned int ifindex,
                                   unsigned short vid,
                                   unsigned short trans_vid)
{
  int ret;

  ret = hal_ioctl (APNBR_DEL_VLAN_TRANS_ENTRY, (unsigned long)name, 
                   ifindex, vid, trans_vid, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_bridge_set_proto_process_port (const char *const bridge_name,
                                   const int ifindex,
                                   enum hal_l2_proto proto,
                                   enum hal_l2_proto_process process,
                                   u_int16_t vid)
{
  int ret;

  ret = hal_ioctl (APNBR_SET_PROTO_PROCESS, (unsigned long)bridge_name, 
                   ifindex, proto, process, vid, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_add_cvid_to_port (char *name, unsigned int ifindex,
                           unsigned short cvid,
                           unsigned short svid,
                           enum hal_vlan_egress_type egress)
{
  return hal_vlan_add_vid_to_port (name, ifindex, cvid, egress);
}

int
hal_vlan_delete_cvid_from_port (char *name, unsigned int ifindex,
                                unsigned short cvid,
                                unsigned short svid)
{
  return hal_vlan_delete_vid_from_port (name, ifindex, cvid);
}


int
hal_vlan_create_cvlan (char *name, unsigned short cvid,
                       unsigned short svid)
{
  return HAL_SUCCESS;
}

int
hal_vlan_delete_cvlan (char *name, unsigned short cvid,
                       unsigned short svid)
{
  return HAL_SUCCESS;
}

int
hal_l2_qos_set_cos_preserve (const int ifindex,
                             u_int16_t vid,
                             u_int8_t preserve_ce_cos)
{
  return HAL_SUCCESS;
}

int
hal_vlan_set_pro_edge_pvid (char *name, unsigned int ifindex,
                            unsigned short svid,
                            unsigned short pvid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_SET_PRO_EDGE_DEFAULT_PVID, (unsigned long)name,
                   ifindex, svid, pvid, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_set_pro_edge_untagged_vid (char *name, unsigned int ifindex,
                                    unsigned short svid,
                                    unsigned short untagged_vid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_SET_PRO_EDGE_UNTAGGED_VID, (unsigned long)name,
                   ifindex, svid, untagged_vid, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_add_pro_edge_port (char *name, unsigned int ifindex,
                            unsigned short svid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_ADD_PRO_EDGE_PORT, (unsigned long)name,
                   ifindex, svid, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}

int
hal_vlan_del_pro_edge_port (char *name, unsigned int ifindex,
                            unsigned short svid)
{
  int ret;

  ret = hal_ioctl (APN_VLAN_DEL_PRO_EDGE_PORT, (unsigned long)name,
                   ifindex, svid, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return HAL_SUCCESS;
}


#endif /* HAVE_PROVIDER_BRIDGE */

int
hal_l2_traffic_class_status_set (unsigned int ifindex,
                                 unsigned int traffic_class_enabled)
{
 return HAL_SUCCESS;
}

int 
hal_l2_get_index_by_mac_vid (char *bridge_name, int *ifindex, char *mac, 
                             u_int16_t vid)
{
  int ret;
  char str[HAL_BRIDGE_NAME_LEN];
  char macaddr[HAL_HW_LENGTH];

  memset (str, '\0', HAL_BRIDGE_NAME_LEN);
  pal_strncpy (str, bridge_name, HAL_BRIDGE_NAME_LEN);
  pal_mem_cpy (macaddr, mac, HAL_HW_LENGTH);

  ret = hal_ioctl (APNBR_GET_IFINDEX_BY_MAC_VID, (unsigned long)str, 
                   (unsigned long)ifindex, (unsigned long)macaddr, vid, 0, 0);
  if (ret < 0)
    {
      return -errno;
    }

  return ret;
 
}

int 
hal_l2_get_index_by_mac_vid_svid (char *bridge_name, int *ifindex, char *mac, 
                             u_int16_t vid, u_int16_t svid)
{
  int ret;
  char str[HAL_BRIDGE_NAME_LEN];
  char macaddr[HAL_HW_LENGTH];

  memset (str, '\0', HAL_BRIDGE_NAME_LEN);
  pal_strncpy (str, bridge_name, HAL_BRIDGE_NAME_LEN);
  pal_mem_cpy (macaddr, mac, HAL_HW_LENGTH);

  ret = hal_ioctl (APNBR_GET_IFINDEX_BY_MAC_VID, (unsigned long)str, 
                   (unsigned long)ifindex, (unsigned long)macaddr, vid, svid, 0);
  if (ret < 0)
    {
      return -errno;
    }

  return ret;
 
}


int
hal_if_set_portbased_vlan (unsigned int ifindex, struct hal_port_map pbitmap)
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
hal_mld_snooping_if_enable (char *name, unsigned int ifindex)
{
  return HAL_SUCCESS;
}
