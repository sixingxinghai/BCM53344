/* Copyright (C) 2003  All Rights Reserved.

   GVRP: GARP VLAN Registration Protocol

   This module declares the interface to the GVRP functions
  
 */

#ifndef __PACOS_GVRP_API_H__
#define __PACOS_GVRP_API_H__

#define GVRP_START_GID_INDEX 0
#define GVRP_API_GET_SUCCESS 0
#define GVRP_API_GET_FAILURE -1
#define PTREE_MAX_KEY_LEN 48

#define XVRP_UPDATE_REG_TYPE(T)                              \
                       do {                                  \
                           if (!pal_strncmp (T, "mv", 2))     \
                             gvrp->reg_type = REG_TYPE_MVRP; \
                           else                              \
                             gvrp->reg_type = REG_TYPE_GVRP; \
                          }while (0)        

#define XVRP_UPDATE_PROTOCOL(GVRP, T)         \
do {                                             \
    if (pal_strncmp (T, "mv", 2) == 0)           \
      {                                          \
        (GVRP)->reg_type = REG_TYPE_MVRP;     \
        (GVRP)->garp.proto = REG_PROTO_MRP;   \
      }                                          \
    else                                         \
      {                                          \
        (GVRP)->reg_type = REG_TYPE_GVRP;     \
        (GVRP)->garp.proto = REG_PROTO_GARP;  \
      }                                          \
}while (0)

#define API_CHECK_XVRP_IS_RUNNING(X,R)                                        \
   do {                                                                       \
        if ((X->gvrp->reg_type == REG_TYPE_GVRP) &&                           \
            (!pal_strncmp (R, "mvrp", 4)))                                    \
            return CLI_ERROR;                                                 \
        else if ((X->gvrp->reg_type == REG_TYPE_MVRP) &&                      \
                 (!pal_strncmp (R, "gvrp", 4)))                               \
            return CLI_ERROR;                                                 \
      } while (0)


 
extern int
gvrp_enable (struct nsm_bridge_master *master,
             char* reg_type,
             const char* const bridge_name);

extern int
gvrp_disable (struct nsm_bridge_master *master,
              char* bridge_name);

extern int
gvrp_get_config_details (struct nsm_bridge_master *master,
                         const char* const bridge_name, 
                         char *config_details);

extern int
gvrp_enable_port (struct nsm_bridge_master *master, 
                  struct interface *ifp);

extern int
gvrp_enable_port_all (struct nsm_bridge_master *master,
                      struct interface *ifp);

extern int
gvrp_enable_port_cb (struct nsm_bridge_master *master, 
                     struct interface *ifp);

extern int
gvrp_disable_port (struct nsm_bridge_master *master, 
                  struct interface *ifp);

extern int
gvrp_disable_port_all (struct nsm_bridge_master *master,
                      struct interface *ifp);

extern int
gvrp_disable_port_cb (struct nsm_bridge_master *master, 
                      struct interface *ifp);

extern int
gvrp_delete_port_cb (struct nsm_bridge_master *master,
                     char *bridge_name, struct interface *ifp);

extern int
gvrp_set_port_state (struct nsm_bridge_master *master,
                     struct interface *ifp);


extern int
gvrp_get_port_details (struct nsm_bridge_master *master, 
                       struct interface *ifp,
                       u_char* flags);

extern int
gvrp_set_registration (struct nsm_bridge_master *master,
                       struct interface * ifp,
                       const u_int32_t registration_type);

/* Currently used by gvrp set registration */
int
gvrp_port_inject_gid_event_all_vlans(struct nsm_vlan_port *port,
                                     struct nsm_bridge * bridge, 
                                     struct gvrp_port *gvrp_port,
                                     gid_event_t event);

/* Currently called only from gvrp_deregister_all_vlans_port which is called
   from gvrp_set_registration where all checks are are already done no checks
   are need for the parameters */
int
gvrp_deregister_vlan_on_port(struct nsm_bridge *bridge,
                             struct interface *ifp, 
                             u_int16_t vid);

/* Currently used by gvrp set registration. Since currently bieng called 
   only from gvrp_set_registration where all checks are are already done no
   checks are done for the parameters */
void
gvrp_deregister_all_vlans_on_port(struct nsm_bridge *bridge, 
                                  struct interface *ifp);

extern int
gvrp_set_timer (struct nsm_bridge_master *master,
                struct interface *ifp,
                const u_int32_t timer_type,
                const pal_time_t timer_value);

extern int
gvrp_get_timer_details (struct nsm_bridge_master *master,
                        struct interface *ifp,
                        pal_time_t *timer_details);

extern int
gvrp_clear_all_statistics (struct cli *cli);

extern int
gvrp_clear_per_bridge_statistics (struct nsm_bridge_master *master,
                                  const char *name);

extern int
gvrp_clear_per_port_statistics (struct nsm_bridge_master *master,
                                struct interface *ifp);

int
gvrp_get_applicant_state (struct nsm_bridge_master *master,
                          struct interface *ifp,
                          u_int32_t *state);

int
gvrp_set_applicant_state (struct nsm_bridge_master *master,
                          struct interface *ifp,
                          const u_int32_t state);

int
gvrp_get_registration (struct nsm_bridge_master *master,
                       struct interface *ifp,
                       u_int32_t *registration_type);

int
gvrp_dynamic_vlan_learning_set (struct nsm_bridge_master *master,
    char *br_name, bool_t vlan_learning_enable);

void
gvrp_port_activate (struct nsm_bridge_master *master, struct interface *ifp);

int
gvrp_check_timer_rules(struct nsm_bridge_master *master,
                       struct interface *ifp,
                       const u_int32_t timer_type,
                       const pal_time_t timer_value);

int
gvrp_get_timer (struct nsm_bridge_master *master,
                struct interface *ifp,
                const u_int32_t timer_type,
                const pal_time_t *timer_value);

s_int32_t
mvrp_set_pointtopoint(struct nsm_bridge_master *master, u_char ctrl,
                      struct interface *ifp);

extern int
gvrp_api_clear_all_statistics (struct nsm_bridge_master *master);

#ifdef HAVE_SMI
int
gvrp_get_configuration_bridge(struct nsm_bridge_master *master, 
                              char *bridge_name,  char* reg_type,
                              struct gvrp_bridge_configuration *gvrp_br_config);

int
gvrp_get_port_statistics(struct nsm_bridge_master *master,
                         struct interface *ifp,
                         struct smi_gvrp_statistics *gvrp_stats);
int
gvrp_get_vid_details (struct nsm_bridge_master *master,
                      struct interface *ifp,
                      u_int32_t first_call,
                      u_int32_t gid_idx,
                      struct smi_gvrp_vid_detail *gvrp_vid_detail);
#endif /* HAVE_SMI */
#endif /* ! __PACOS_GVRP_API_H__ */
