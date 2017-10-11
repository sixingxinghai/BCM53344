/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_QOS_API_H__
#define __NSM_QOS_API_H__

#ifdef HAVE_QOS

#define CHECK_GLOBAL_QOS_ENABLED                          \
  if (qosg.state != NSM_QOS_ENABLED_STATE)                \
    return -1;                                            \

extern struct map_tbls              default_map_tbls;
extern struct map_tbls              work_map_tbls;
extern struct min_res               min_reserve[MAX_NUM_OF_QUEUE];
extern struct min_res               queue_min_reserve[MAX_NUM_OF_QUEUE];
extern struct nsm_qos_global        qosg;


/* Error codes */
#define NSM_CMAP_QOS_ALGORITHM_STRICT 1
#define NSM_CMAP_QOS_ALGORITHM_DRR 2
#define NSM_CMAP_QOS_ALGORITHM_DRR_STRICT 3

/* Function prototypes. */
extern void nsm_qos_init(struct nsm_master *nm);
extern void nsm_qos_deinit(struct nsm_master *nm);
extern int nsm_qos_init_maps();
extern int nsm_qos_chk_if_qos_enabled (struct nsm_qos_pmap_master *pmap);

int nsm_qos_global_enable(struct nsm_master *nm);
int nsm_qos_global_disable(struct nsm_master *nm);
int nsm_qos_get_global_status(u_int8_t *status);
int nsm_qos_set_pmap_name(struct nsm_master *nm, char *pmap);
#ifdef HAVE_SMI
int nsm_qos_get_policy_map_info (struct nsm_master *nm, char *pmap_name, 
                                 struct smi_qos_pmap *qos_map);
int nsm_qos_get_policy_map_names (struct nsm_master *nm, 
                                  char pmap_names[][SMI_QOS_POLICY_LEN],
                                  int first_call,
                                  char *pmap_name);
int nsm_qos_get_cmap_info (struct nsm_master *nm, char *cmap_name,
                           struct smi_qos_cmap *cmap_info);
int nsm_qos_pmapc_set_police (struct nsm_master *nm, 
                              char *pmap_name, 
                              char *cmap_name,
                              int rate_value, 
                              int commit_burst_size, 
                              int excess_brust_size, 
                              enum smi_exceed_action exceed_action,
                              enum smi_qos_flow_control_mode fc_mode);
int
nsm_qos_pmapc_get_police (struct nsm_master *nm, 
                          char *pmap_name, 
                          char *cmap_name,
                          int *rate_value, 
                          int *commit_burst_size, 
                          int *excess_brust_size, 
                          enum smi_exceed_action *exceed_action,
                          enum smi_qos_flow_control_mode *fc_mode);
#endif /* HAVE_SMI */
int nsm_qos_delete_pmap (struct nsm_master *nm, char *pmap_name);
int nsm_qos_set_cmap_name(struct nsm_master *nm, char *cmap_name);
int nsm_qos_delete_cmap (struct nsm_master *nm, char *cmap_name);
int nsm_qos_pmapc_delete_police (struct nsm_master *nm, 
                                 char *pmap_name, char *cmap_name);
int nsm_qos_pmapc_delete_cmap  (struct nsm_master *nm,
                                char *pmap_name, char *cmap_name);

int nsm_qos_global_cos_to_queue(struct nsm_master *nm, u_int8_t cos, 
                                u_int8_t queue_id);
int nsm_qos_global_dscp_to_queue(struct nsm_master *nm, u_int8_t dscp, 
                                 u_int8_t queue_id);
int nsm_qos_set_force_trust_cos_api(struct nsm_master *nm, 
                                    struct interface *ifp, 
                                    u_int8_t force_trust_cos);
int nsm_qos_set_vlan_priority_api (struct nsm_master *nm,
                                   char *bridge_name,
                                   u_int16_t vid,
                                   u_int8_t priority);
int nsm_qos_set_port_service_policy(struct nsm_master *nm, 
                                    struct interface *ifp, char *pmap_name);
int nsm_qos_unset_port_service_policy(struct nsm_master *nm, 
                                     struct interface *ifp, char *pmap_name);
int nsm_qos_get_strict_queue (struct nsm_qos_global *qosg,
                              struct interface *ifp, u_int8_t *qid);
int 
nsm_qos_get_port_service_policy(struct nsm_master *nm, 
                                struct interface *ifp, char *pmap_name);
#endif /* HAVE_QOS */
#endif /* __NSM_QOS_API_H__ */
