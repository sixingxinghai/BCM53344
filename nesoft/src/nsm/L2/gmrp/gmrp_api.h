/* Copyright (C) 2003  All Rights Reserved.

   GMRP: GARP Multicast Registration Protocol
  
   This module declares the interface to the GMRP functions
 */
#ifndef __PACOS_GMRP_API_H__
#define __PACOS_GMRP_API_H__

s_int32_t
gmrp_static_entry (struct nsm_bridge_master *master,
                   struct nsm_bridge *bridge, struct interface *ifp);

s_int32_t
gmrp_advertise_static_entry (struct nsm_bridge_master *master,
                             struct nsm_bridge *bridge, struct interface *ifp,
                             struct ptree *list, u_int16_t vid, u_int16_t svid);
extern s_int32_t
gmrp_enable (struct nsm_bridge_master *master,
             char *protocol, const char* const bridge_name);

extern s_int32_t
gmrp_disable (struct nsm_bridge_master *master,
              char* bridge_name);

extern s_int32_t
gmrp_extended_filtering (struct nsm_bridge_master *master,
                         const char* const bridge_name, bool_t enable);
extern s_int32_t
gmrp_enable_port (struct nsm_bridge_master *master,
                  struct interface *ifp);

extern s_int32_t
gmrp_enable_port_all (struct nsm_bridge_master *master);

extern s_int32_t
gmrp_enable_port_cb (struct nsm_bridge_master *master,
                     struct interface *ifp);

extern s_int32_t
gmrp_add_port_cb (struct nsm_bridge_master *master,
                  char *bridge_name, struct interface *ifp);

extern s_int32_t
gmrp_disable_port (struct nsm_bridge_master *master,
                   struct interface *ifp);

extern s_int32_t
gmrp_disable_port_all (struct nsm_bridge_master *master);

extern s_int32_t
gmrp_disable_port_cb (struct nsm_bridge_master *master,
                      struct interface *ifp);

extern s_int32_t
gmrp_enable_instance (struct nsm_bridge_master *master,
                      char *protocol,  char*  bridge_name, u_int16_t vid);

extern s_int32_t
gmrp_disable_instance (struct nsm_bridge_master *master,
                       char*  bridge_name, u_int16_t vid);

extern s_int32_t
gmrp_enable_port_instance (struct nsm_bridge_master *master,
                           struct interface *ifp,
                           u_int16_t vid);

extern s_int32_t
gmrp_disable_port_instance (struct nsm_bridge_master *master,
                            struct interface *ifp,
                            u_int16_t vid);
extern s_int32_t
gmrp_delete_port_cb (struct nsm_bridge_master *master,
                     char *bridge_name, struct interface *ifp);
extern s_int32_t
gmrp_set_port_state (struct nsm_bridge_master *master,
                     struct interface *ifp);

extern s_int32_t
gmrp_get_port_details (struct nsm_bridge_master *master,
                       struct interface *ifp, u_char* flags);

extern s_int32_t
gmrp_set_registration (struct nsm_bridge_master *master,
                       struct interface *ifp,
                       const u_int32_t registration_type);
extern int
gmrp_set_fwd_all_forbid (struct nsm_bridge_master *master,
                     struct interface *ifp, const u_int32_t registration_type);

extern int
gmrp_set_fwd_unregistered_forbid(struct nsm_bridge_master *master,
                     struct interface *ifp, const u_int32_t registration_type);
extern int
gmrp_set_fwd_unregistered(struct nsm_bridge_master *master,
                     struct interface *ifp, const u_int32_t registration_type);

extern s_int32_t
gmrp_set_timer (struct nsm_bridge_master *master, struct interface *ifp,
                const u_int32_t timer_type, const pal_time_t timer_value);

extern s_int32_t
gmrp_get_timer (struct nsm_bridge_master *master, struct interface *ifp,
                const u_int32_t timer_type, pal_time_t *timer_value);

extern s_int32_t
gmrp_get_timer_details (struct nsm_bridge_master *master,
                        struct interface *ifp, pal_time_t* timer_details);

extern s_int32_t
gmrp_get_per_vlan_statistics_details (struct nsm_bridge_master *master,
                                      const char* const bridge_name,
                                      const u_int16_t vid,
                                      u_int32_t *receive_counters,
                                      u_int32_t *transmit_counters);

extern s_int32_t
gmrp_clear_all_statistics (struct cli *cli);

extern s_int32_t
gmrp_clear_bridge_statistics (struct nsm_bridge_master *master,
                              const char * const bridge_name);

extern s_int32_t
gmrp_clear_per_vlan_statistics (struct nsm_bridge_master *master,
                                const char* const bridge_name,
                                const u_int16_t vid);

extern s_int32_t
gmrp_set_fwd_all (struct nsm_bridge_master *master,
                  struct interface *ifp,
                  const u_int32_t event);

extern s_int32_t
gmrp_is_configured (struct nsm_bridge_master *master,
                    const char* const bridge_name,
                    u_char *is_gmrp_enabled);

void *
gmrp_get_first_node (struct avl_tree *tree );

struct gmrp *
gmrp_get_by_vid (struct avl_tree *gmrp_list, u_int16_t vlan,
                 u_int16_t svlan, struct avl_node **node);

struct gmrp_port_instance *
gmrp_port_instance_get_by_vid (struct avl_tree *gmrp_list, u_int16_t vlan,
                               u_int16_t svlan, struct avl_node **node);

s_int32_t
gmrp_add_vlan_to_port (struct nsm_bridge *bridge, struct interface *ifp,
                       u_int16_t vlanid, u_int16_t svlanid);

s_int32_t
gmrp_remove_vlan_from_port (struct nsm_bridge *bridge, struct nsm_if *port,
                           u_int16_t vlanid, u_int16_t svlanid);
 
void
gmrp_activate_port (struct nsm_bridge_master *master, struct interface *ifp);


s_int32_t
gmrp_check_timer_rules(struct nsm_bridge_master *master,
                       struct interface *ifp,
                       const u_int32_t timer_type,
                       const pal_time_t timer_value);

s_int32_t
mmrp_set_pointtopoint(struct nsm_bridge_master *master, u_char ctrl,
                      struct interface *ifp);     


#define XMRP_UPDATE_PROTOCOL(GMRP_BR, T)         \
do {                                             \
    if (pal_strncmp (T, "mm", 2) == 0)           \
      {                                          \
        (GMRP_BR)->reg_type = REG_TYPE_MMRP;     \
        (GMRP_BR)->garp.proto = REG_PROTO_MRP;   \
      }                                          \
    else                                         \
      {                                          \
        (GMRP_BR)->reg_type = REG_TYPE_GMRP;     \
        (GMRP_BR)->garp.proto = REG_PROTO_GARP;  \
      }                                          \
}while (0)



#define XMRP_UPDATE_PROTOCOL(GMRP_BR, T)         \
do {                                             \
    if (pal_strncmp (T, "mm", 2) == 0)           \
      {                                          \
        (GMRP_BR)->reg_type = REG_TYPE_MMRP;     \
        (GMRP_BR)->garp.proto = REG_PROTO_MRP;   \
      }                                          \
    else                                         \
      {                                          \
        (GMRP_BR)->reg_type = REG_TYPE_GMRP;     \
        (GMRP_BR)->garp.proto = REG_PROTO_GARP;  \
      }                                          \
}while (0)


#endif /* ! __PACOS_GMRP_API_H__ */
