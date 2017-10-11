/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_QOS_LIST_H__
#define __NSM_QOS_LIST_H__


#ifdef HAVE_QOS

#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

/*------------------*/
/* DSCP MUT list    */
/*------------------*/
struct nsm_dscp_mut_list
{
  struct nsm_qos_dscpml_master *master;

  /* dscpml-list linked list */
  struct nsm_dscp_mut_list *next;
  struct nsm_dscp_mut_list *prev;

  /* dscpml-list's attributes */
  char *name;
  char *acl_name;

  /* dscp info filed */
  struct dscp_mut d;
  /* statistics */
  /* struct nsm_dscp_mut_list_stat stat; */
#ifdef HAVE_HA
  HA_CDR_REF nsm_dscp_mut_data_cdr_ref;
#endif
};

struct nsm_qos_dscpml_master
{
  struct nsm_dscp_mut_list *head;
  struct nsm_dscp_mut_list *tail;
};


/*------------------*/
/* DSCP-to-COS list */
/*------------------*/
struct nsm_dscp_cos_list
{
  struct nsm_qos_dcosl_master *master;

  /* dcosl-list linked list */
  struct nsm_dscp_cos_list *next;
  struct nsm_dscp_cos_list *prev;

  /* dcosl-list's attributes */
  char *name;
  char *acl_name;

  /* dscp info filed */
  struct dscp_mut d;
  /* statistics */
  /* struct nsm_dscp_mut_list_stat stat; */
#ifdef HAVE_HA
  HA_CDR_REF nsm_dscp_cos_data_cdr_ref;
#endif
};

struct nsm_qos_dcosl_master
{
  struct nsm_dscp_cos_list *head;
  struct nsm_dscp_cos_list *tail;
};


/*-----------------------*/
/* Aggregate-police list */
/*-----------------------*/
struct nsm_agp_list
{
  struct nsm_qos_agp_master *master;

  /* dscpml-list linked list */
  struct nsm_agp_list *next;
  struct nsm_agp_list *prev;

  /* dscpml-list's attributes */
  char *name;
  char *acl_name;

  /* aggregate-policer info */
  struct police p;
  /* statistics */
  /* struct nsm_agp_list_stat stat; */
#ifdef HAVE_HA
  HA_CDR_REF nsm_agp_data_cdr_ref;
#endif
};

struct nsm_qos_agp_master
{
  struct nsm_agp_list *head;
  struct nsm_agp_list *tail;
};


struct nsm_qos_if_queue
{
  int wred_threshold1;
  int wred_threshold2;
  int td_threshold1;
  int td_threshold2;
  int min_reserve_level;
  int weight;
  int qid;

#define NSM_QOS_IFQ_CONFIG_WRED_THRESHOLD    (1 << 0)
#define NSM_QOS_IFQ_CONFIG_TD_THRESHOLD      (1 << 1)
#define NSM_QOS_IFQ_CONFIG_MIN_RESERVE       (1 << 2)
#define NSM_QOS_IFQ_CONFIG_WEIGHT            (1 << 3)
  u_char config;
};

/*-----------------------*/
/* QoS interface list    */
/*-----------------------*/
struct nsm_qif_list
{
  struct nsm_qos_if_master *master;

  /* qif-list linked list */
  struct nsm_qif_list *next;
  struct nsm_qif_list *prev;

  /* qif-list's attributes */
  char *name;
  char *acl_name;

  /* aggregate-policer info */
  char if_name [INTERFACE_NAMSIZ +1];
  char input_pmap_name [INTERFACE_NAMSIZ +1];
  char output_pmap_name [INTERFACE_NAMSIZ +1];
  char trust_state [INTERFACE_NAMSIZ +1];
  char dscp_mut_name [INTERFACE_NAMSIZ +1];
  char dscp_cos_name [INTERFACE_NAMSIZ +1];

  /* Cos to queue map */
  u_char cosq_map [NSM_QOS_COS_MAX + 1];

  /* Dscp to queue map */
  u_char dscpq_map [NSM_QOS_DSCP_MAX + 1];
  
  int weight [MAX_NUM_OF_QUEUE];
  int queue_limit [MAX_NUM_OF_QUEUE];

  struct nsm_qos_if_queue queue [MAX_NUM_OF_QUEUE];
  int wred_dscp_threshold [NSM_QOS_DSCP_MAX + 1];

  /* trust state */
  u_int8_t force_trust_cos;  

#define NSM_QOS_DEFAULT_COS        (0)

  /* Default COS */
  u_int8_t def_cos;  

  enum nsm_qos_rate_unit egress_rate_unit;

  u_int32_t egress_shape_rate;

  u_int8_t trust_type; 
  
  /* service policy */
  enum nsm_qos_service_policy_direction dir;
 
  /*  port priority*/
  enum nsm_qos_vlan_pri_override port_mode;

  /*  DA port priority*/
  enum nsm_qos_da_pri_override da_port_mode;

#define NSM_QOS_STRICT_QUEUE0                (1 << 0)
#define NSM_QOS_STRICT_QUEUE1                (1 << 1)
#define NSM_QOS_STRICT_QUEUE2                (1 << 2)
#define NSM_QOS_STRICT_QUEUE3                (1 << 3)

#define NSM_QOS_STRICT_QUEUE_NONE            (0)
#define NSM_QOS_STRICT_QUEUE_ALL             (NSM_QOS_STRICT_QUEUE0 |\
                                              NSM_QOS_STRICT_QUEUE1 |\
                                              NSM_QOS_STRICT_QUEUE2 |\
                                              NSM_QOS_STRICT_QUEUE3)

  u_int8_t strict_queue;

#define NSM_QOS_IF_CONFIG_WEIGHT             (1 << 0)
#define NSM_QOS_IF_CONFIG_PRIORITY_QUEUE     (1 << 1)
#define NSM_QOS_IF_CONFIG_QUEUE_LIMIT        (1 << 2)

  u_int16_t config;
  
  /* statistics */
#ifdef HAVE_HA
  HA_CDR_REF nsm_qif_data_cdr_ref;
  HA_CDR_REF nsm_qif_dscp_q_data_cdr_ref;
#endif
};

struct nsm_qos_if_master
{
  struct nsm_qif_list *head;
  struct nsm_qif_list *tail;
};

struct nsm_qos_ft_master
{
  struct nsm_qft_list *head;
  struct nsm_qft_list *tail;
};

/*-----------------------*/
/* QoS Frame Type list    */
/*-----------------------*/
struct nsm_qft_list
{
  struct nsm_ft_master *master;

  /* qif-list linked list */
  struct nsm_qft_list *next;
  struct nsm_qft_list *prev;

  /* frame type-list's attributes */
  enum nsm_qos_frame_type frame_type;

  u_int8_t qid;

  char *name;

  u_int16_t config;
};

struct nsm_cmap_qos_vlan_queue_priority
{
  char q0;
  char q1;
  char q2;
  char q3;
  char q4;
  char q5;
  char q6;
  char q7;
};

/*---------------*/
/* CMAP          */
/*---------------*/
struct nsm_cmap_list
{
  struct nsm_qos_cmap_master *master;

  /* cmap-list linked list */
  struct nsm_cmap_list *next;
  struct nsm_cmap_list *prev;

  /* cmap-list's attributes */
  char *name;
  char acl_name [INTERFACE_NAMSIZ+1];
  char acl_name_set;
  /* inforamtion field of cmap */
  struct vlan_filter v;
  char agg_policer_name[INTERFACE_NAMSIZ+1];
  char agg_policer_name_set;
  struct police p;
  struct set s;
  struct trust t;
  struct cmap_dscp d;
  struct cmap_ip_prec i;
  struct cmap_exp e;

  struct cmap_l4_port l4_port;

  struct nsm_cmap_traffic_type traffic_type;
  struct weight *weight;

#define NSM_QOS_MATCH_TYPE_NONE             0
#define NSM_QOS_MATCH_TYPE_DSCP             1
#define NSM_QOS_MATCH_TYPE_IP_PREC          2
#define NSM_QOS_MATCH_TYPE_L4_PORT          3
#define NSM_QOS_MATCH_TYPE_ACL              4
#define NSM_QOS_MATCH_TYPE_EXP              5
#define NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC   6
  u_char match_type;

  /* statistics */

  u_int8_t drr_set;

  /* DRR settings */
  u_int8_t drr_priority;
  u_int8_t drr_quantum;

  u_int8_t policing_set;

  /* For Policing & Shaping*/
  u_int8_t policing_data_ratio;

  u_int8_t vlan_priority_set;  

  /* vlan queue priority */
  struct nsm_cmap_qos_vlan_queue_priority queue_priority; 
  
  u_int8_t scheduling_algo_set;
  
  /* scheduling algo*/ 
  int scheduling_algo; 
  
  /*  port priority*/
  enum nsm_qos_vlan_pri_override port_mode;

  /* police rate */
  enum nsm_qos_rate_unit rate_unit;
  enum nsm_exceed_action exceed_action;
  enum nsm_qos_flow_control_mode fc_mode;
#ifdef HAVE_HA
  HA_CDR_REF nsm_cmap_data_cdr_ref;
  HA_CDR_REF nsm_cmap_vlan_q_data_cdr_ref;
  HA_CDR_REF nsm_cmap_dscp_ip_data_cdr_ref;
#endif /*HAVE_HA*/
};

struct nsm_qos_cmap_master
{
  struct nsm_cmap_list *head;
  struct nsm_cmap_list *tail;
};


/*---------------*/
/* PMAP          */
/*---------------*/
struct nsm_pmap_list
{
  struct nsm_qos_pmap_master *master;

  /* pmap-list linked list */
  struct nsm_pmap_list *next;
  struct nsm_pmap_list *prev;

  /* pmap-list's attributes */
  char *name;
  char *acl_name;

  /* information field of pmap */
  /* attached -> increase, detached ->decrease */
  int attached;

  char cl_name[MAX_NUM_OF_CLASS_IN_PMAPL][INTERFACE_NAMSIZ];
#ifdef HAVE_HA
  HA_CDR_REF nsm_pmap_data_cdr_ref;
#endif

  /* statistics */
};

struct nsm_qos_pmap_master
{
  struct nsm_pmap_list *head;
  struct nsm_pmap_list *tail;
};


int nsm_acl_list_master_init (struct nsm_master *nm);
void nsm_acl_list_master_deinit (struct nsm_master *nm);
struct nsm_cmap_list * nsm_cmap_list_new ();
void nsm_cmap_list_free (struct nsm_cmap_list *cmapl);
void nsm_cmap_list_delete (struct nsm_cmap_list *cmapl);
void nsm_cmap_list_clean (struct nsm_qos_cmap_master *master);
struct nsm_cmap_list * nsm_cmap_list_insert (struct nsm_qos_cmap_master *master,
                                             const char *cmap_name,
                                             const char *acl_name);
struct nsm_cmap_list * nsm_cmap_list_lookup (struct nsm_qos_cmap_master *master,
                                             const char *cmap_name);
struct nsm_cmap_list * nsm_cmap_list_lookup_by_acl (struct nsm_qos_cmap_master *master,
                                                    const char *acl_name);
struct nsm_cmap_list * nsm_cmap_list_get (struct nsm_qos_cmap_master *master,
                                          const char *cmap_name);
int nsm_cmap_list_master_init (struct nsm_master *nm);
void nsm_cmap_list_master_deinit (struct nsm_master *nm);
struct nsm_pmap_list * nsm_pmap_list_new ();
void nsm_pmap_list_free (struct nsm_pmap_list *pmapl);
void nsm_pmap_list_delete (struct nsm_pmap_list *pmapl);
void nsm_pmap_list_clean (struct nsm_qos_pmap_master *master);
struct nsm_pmap_list * nsm_pmap_list_insert (struct nsm_qos_pmap_master *master,
                                             const char *pmap_name,
                                             const char *acl_name);
struct nsm_pmap_list * nsm_pmap_list_lookup (struct nsm_qos_pmap_master *master,
                                             const char *pmap_name);
struct nsm_pmap_list * nsm_pmap_list_lookup_by_acl (struct nsm_qos_pmap_master *master,
                                                    const char *acl_name);
struct nsm_pmap_list * nsm_pmap_list_get (struct nsm_qos_pmap_master *master,
                                          const char *pmap_name);
int nsm_pmap_list_master_init (struct nsm_master *nm);
int nsm_pmap_list_master_deinit (struct nsm_master *nm);
struct nsm_dscp_mut_list * nsm_dscp_mut_list_new ();
void nsm_dscp_mut_list_free (struct nsm_dscp_mut_list *dscpml);
void nsm_dscp_mut_list_delete (struct nsm_dscp_mut_list *dscpml);
void nsm_dscp_mut_list_clean (struct nsm_qos_dscpml_master *master);
struct nsm_dscp_mut_list * nsm_dscp_mut_list_insert (struct nsm_qos_dscpml_master *master,
                                                     const char *dscpml_name,
                                                     const char *acl_name);
struct nsm_dscp_mut_list * nsm_dscp_mut_list_lookup (struct nsm_qos_dscpml_master *master,
                                                     const char *dscpml_name);
struct nsm_dscp_mut_list * nsm_dscp_mut_list_lookup_by_acl (struct nsm_qos_dscpml_master *master,
                                                            const char *acl_name);
struct nsm_dscp_mut_list * nsm_dscp_mut_list_get (struct nsm_qos_dscpml_master *master,
                                                  const char *dscpml_name);
int nsm_dscp_mut_list_master_init (struct nsm_qos_global *qg);
int nsm_dscp_mut_list_master_deinit (struct nsm_qos_global *qg);
struct nsm_dscp_cos_list * nsm_dscp_cos_list_new ();
void nsm_dscp_cos_list_free (struct nsm_dscp_cos_list *dcosl);
void nsm_dscp_cos_list_delete (struct nsm_dscp_cos_list *dcosl);
void nsm_dscp_cos_list_clean (struct nsm_qos_dcosl_master *master);
struct nsm_dscp_cos_list * nsm_dscp_cos_list_insert (struct nsm_qos_dcosl_master *master,
                                                     const char *dcosl_name,
                                                     const char *acl_name);
struct nsm_dscp_cos_list * nsm_dscp_cos_list_lookup (struct nsm_qos_dcosl_master *master,
                                                     const char *dcosl_name);
struct nsm_dscp_cos_list * nsm_dscp_cos_list_lookup_by_acl (struct nsm_qos_dcosl_master *master,
                                                            const char *acl_name);
struct nsm_dscp_cos_list * nsm_dscp_cos_list_get (struct nsm_qos_dcosl_master *master,
                                                  const char *dcosl_name);
int nsm_dscp_cos_list_master_init (struct nsm_qos_global *qg);
int nsm_dscp_cos_list_master_deinit (struct nsm_qos_global *qg);
struct nsm_agp_list * nsm_agp_list_new ();
void nsm_agp_list_free (struct nsm_agp_list *agpl);
void nsm_agp_list_delete (struct nsm_agp_list *agpl);
void nsm_agp_list_clean (struct nsm_qos_agp_master *master);
struct nsm_agp_list * nsm_agp_list_insert (struct nsm_qos_agp_master *master,
                                           const char *agpl_name,
                                           const char *acl_name);
struct nsm_agp_list * nsm_agp_list_lookup (struct nsm_qos_agp_master *master,
                                           const char *agpl_name);
struct nsm_agp_list * nsm_agp_list_lookup_by_acl (struct nsm_qos_agp_master *master,
                                                  const char *acl_name);
struct nsm_qif_list * nsm_qif_list_new ();
void nsm_qif_list_free (struct nsm_qif_list *qifl);
void nsm_qif_list_delete (struct nsm_qif_list *qifl);
void nsm_qif_list_clean (struct nsm_qos_if_master *master);
struct nsm_qif_list * nsm_qif_list_insert (struct nsm_qos_if_master *master,
                                           const char *qifl_name,
                                           const char *acl_name);
struct nsm_qif_list * nsm_qif_list_lookup (struct nsm_qos_if_master *master,
                                           const char *qifl_name);
struct nsm_qif_list * nsm_qif_list_lookup_by_acl (struct nsm_qos_if_master *master,
                                                  const char *acl_name);
struct nsm_qif_list * nsm_qif_list_get (struct nsm_qos_if_master *master,
                                        char *qifl_name);
int nsm_qif_list_master_init (struct nsm_qos_global *qg);
int nsm_qif_list_master_deinit (struct nsm_qos_global *qg);
struct nsm_agp_list * nsm_agp_list_get (struct nsm_qos_agp_master *master,
                                        const char *agpl_name);
int nsm_agp_list_master_init (struct nsm_qos_global *qg);
int nsm_agp_list_master_deinit (struct nsm_qos_global *qg);
int  nsm_qos_check_macl_from_cmap_and_pmap (struct nsm_qos_cmap_master *cmaster,
                                            struct nsm_qos_pmap_master *pmaster,
                                            const char *acl_name, int);
struct nsm_pmap_list * nsm_qos_check_class_from_pmap (struct nsm_qos_pmap_master *master,
                                                      const char *cl_name, int check_attached);
struct nsm_cmap_list * nsm_qos_insert_acl_name_into_cmap (struct nsm_cmap_list *cmapl,
                                                          const char *acl_name);
struct nsm_cmap_list * nsm_qos_delete_acl_name_from_cmap (struct nsm_cmap_list *cmapl,
                                                          const char *acl_name);
void nsm_qos_set_dscp_into_cmap (struct nsm_cmap_list *cmapl,
                                 int num,
                                 u_int8_t *dscp);
void nsm_qos_delete_dscp_from_cmap (struct nsm_cmap_list *cmapl);
void nsm_qos_set_prec_into_cmap (struct nsm_cmap_list *cmapl,
                                 int num,
                                 u_int8_t *dscp);
void nsm_qos_delete_prec_from_cmap (struct nsm_cmap_list *cmapl);
void nsm_qos_set_exp_into_cmap (struct nsm_cmap_list *cmapl,
                                int num,
                                u_int8_t *exp);
void nsm_qos_delete_exp_from_cmap (struct nsm_cmap_list *cmapl);
int nsm_qos_set_vid_into_cmap (struct nsm_cmap_list *cmapl,
                               int num,
                               u_int16_t *vid);
void nsm_qos_delete_vid_from_cmap (struct nsm_cmap_list *cmapl);
struct nsm_pmap_list * nsm_qos_check_cl_name_in_pmapl (struct nsm_pmap_list *pmapl,
                                                       const char *cl_name);
void nsm_qos_delete_cmap_from_pmapl (struct nsm_pmap_list *pmapl,
                                     const char *cl_name);
struct nsm_cmap_list * nsm_qos_check_cmap_in_all_cmapls (struct nsm_qos_cmap_master *master,
                                                         const char *cl_name);
void nsm_qos_set_trust_into_cmap (struct nsm_cmap_list *cmapl,
                                  int ind,
                                  int trust);
void nsm_qos_set_val_into_cmap (struct nsm_cmap_list *cmapl,
                                int ind,
                                int val);

void
nsm_qos_set_exd_act_into_cmap (struct nsm_cmap_list *cmapl,
                               u_int32_t rate,
                               u_int32_t burst,
                               u_int32_t excess_burst,
                               enum nsm_exceed_action exd,
                               enum nsm_qos_flow_control_mode fc_mode);

void nsm_qos_clear_exd_act_into_cmap (struct nsm_cmap_list *cmapl);
int nsm_pmap_class_agg_policer_exists (struct nsm_master *, char *, int);
int nsm_qos_if_dscp_mutation_map_attached (struct nsm_master *, char *);
void nsm_qos_set_layer4_port_into_cmap (struct nsm_cmap_list *, u_int16_t,
                                        u_char);
void nsm_qos_delete_layer4_port_from_cmap (struct nsm_cmap_list *, u_int16_t,
                                           u_char);

void nsm_qos_set_layer4_port_into_cmap (struct nsm_cmap_list *cmapl,
                                        u_int16_t l4_port_id, u_char port_type);

void nsm_qos_delete_layer4_port_from_cmap (struct nsm_cmap_list *cmapl,
                                           u_int16_t l4_port_id,
                                           u_char port_type);
void nsm_qos_set_layer4_port_into_cmap (struct nsm_cmap_list *cmapl,
                                   u_int16_t l4_port_id, u_char port_type);
void nsm_qos_delete_layer4_port_from_cmap (struct nsm_cmap_list *cmapl,
                                      u_int16_t l4_port_id, u_char port_type);
int nsm_qos_hal_set_all_class_map(struct nsm_master *nm,
                              struct nsm_pmap_list *pmapl,
                              int ifindex,
                              int action,
                              int dir);

void
nsm_qos_queue_weight_init (struct nsm_qos_global *qg);

int
nsm_mls_qos_set_queue_weight (struct nsm_qos_global *qg,
                              u_int8_t queue_id,
                              u_int8_t weight);

int
nsm_mls_qos_unset_queue_weight (struct nsm_qos_global *qg,
                                u_int8_t queue_id);

void
nsm_qos_dscp_to_queue_init (struct nsm_qos_global *qg);

int
nsm_mls_qos_set_dscp_to_queue (struct nsm_qos_global *qg,
                               u_int8_t dscp,
                               u_int8_t queue_id);

int
nsm_mls_qos_unset_dscp_to_queue (struct nsm_qos_global *qg,
                                 u_int8_t dscp);

void
nsm_qos_cos_to_queue_init (struct nsm_qos_global *qg);

int
nsm_mls_qos_set_cos_to_queue (struct nsm_qos_global *qg,
                              u_int8_t cos,
                              u_int8_t queue_id);

int
nsm_mls_qos_unset_cos_to_queue (struct nsm_qos_global *qg,
                                u_int8_t cos);

char *
nsm_qos_ttype_cri_to_str (enum nsm_qos_traffic_match ttype_criteria);

char *
nsm_qos_ftype_to_str (enum nsm_qos_frame_type ftype);

void
nsm_qos_set_frame_type_priority_override_init (struct nsm_qos_global *qg);

int
nsm_qos_set_frame_type_priority_override (struct nsm_qos_global *qg,
                                          enum nsm_qos_frame_type ftype,
                                          u_int8_t queue_id);

int
nsm_qos_unset_frame_type_priority_override (struct nsm_qos_global *qg,
                                            enum nsm_qos_frame_type ftype);

int
nsm_mls_qos_vlan_priority_set (struct nsm_master *nm,
                               char *bridge_name,
                               u_int16_t vid,
                               u_int8_t priority);

void
nsm_mls_qos_vlan_pri_config_deinit ();

void
nsm_mls_qos_vlan_priority_config_restore (struct nsm_master *nm,
                                          char *bridge_name,
                                          u_int16_t vid);

int
nsm_mls_qos_vlan_priority_get (struct nsm_master *nm,
                               char *bridge_name,
                               u_int16_t vid,
                               u_int8_t *priority);

int
nsm_mls_qos_vlan_priority_unset (struct nsm_master *nm,
                                 char *bridge_name,
                                 u_int16_t vid);

int
nsm_qos_set_port_vlan_priority_override (struct nsm_qos_global *qosg,
                                         struct interface *ifp,
                                         enum nsm_qos_vlan_pri_override port_mode);

int
nsm_qos_set_port_da_priority_override (struct nsm_qos_global *qosg,
                                       struct interface *ifp,
                                       enum nsm_qos_da_pri_override port_mode);

int
nsm_qos_set_trust_state (struct nsm_qos_global *qosg,
                         struct interface *ifp,
                         u_int32_t trust_state);

int
nsm_qos_set_force_trust_cos (struct nsm_qos_global *qosg,
                             struct interface *ifp,
                             u_int8_t force_trust_cos);

s_int32_t
nsm_qos_cmap_match_traffic_set (struct nsm_master *nm,
                                char *cname,
                                u_int32_t traffic_type,
                                enum nsm_qos_traffic_match criteria);

s_int32_t
nsm_qos_cmap_match_traffic_unset (struct nsm_master *nm,
                                  char *cname,
                                  u_int32_t traffic_type);

s_int32_t
nsm_qos_cmap_match_unset_traffic (struct nsm_master *nm,
                                  char *cname);

int
nsm_qos_set_egress_rate_shaping (struct nsm_qos_global *qosg,
                                 struct interface *ifp,
                                 u_int32_t rate,
                                 enum nsm_qos_rate_unit rate_unit);

int
nsm_qos_set_def_cos_for_port (struct nsm_qos_global *qosg,
                              struct interface *ifp,
                              u_int8_t cos);

int
nsm_qos_set_strict_queue (struct nsm_qos_global *qosg,
                          struct interface *ifp,
                          u_int8_t qid);
int
nsm_qos_unset_strict_queue (struct nsm_qos_global *qosg,
                            struct interface *ifp,
                            u_int8_t qid);
int
nsm_mls_qos_get_queue_weight (struct nsm_qos_global *qg,
                              int *weights);
int
nsm_mls_qos_get_cos_to_queue (struct nsm_qos_global *qg,
                              u_int8_t *cos_to_queue);
int
nsm_mls_qos_get_dscp_to_queue (struct nsm_qos_global *qg,
                              u_int8_t *dscp_to_queue); 

#ifdef HAVE_SMI

int
nsm_qos_get_egress_rate_shaping (struct nsm_qos_global *qosg,
                                 struct interface *ifp,
                                 u_int32_t *rate,
                                 enum smi_qos_rate_unit *rate_unit);
int
nsm_qos_get_trust_state (struct nsm_qos_global *qosg,
                         struct interface *ifp,
                         enum smi_qos_trust_state *trust_state);
int
nsm_qos_get_port_vlan_priority_override (struct nsm_qos_global *qosg,
                                     struct interface *ifp,
                                     enum smi_qos_vlan_pri_override *port_mode);

int
nsm_qos_get_port_da_priority_override (struct nsm_qos_global *qosg,
                                       struct interface *ifp,
                                       enum smi_qos_da_pri_override *port_mode);
s_int32_t
nsm_qos_cmap_match_traffic_get (struct nsm_master *nm,
                                char *cname,
                                enum smi_qos_traffic_match_mode *criteria,
                                u_int32_t *traffic_type);
#endif /* HAVE_SMI */

int
nsm_qos_get_force_trust_cos (struct nsm_qos_global *qosg,
                             struct interface *ifp,
                             u_int8_t *force_trust_cos);

#define NSM_QOS_SUCCESS   0
#define NSM_QOS_ERR       -1

#define NSM_QOS_ERR_BASE                         -1000
#define NSM_QOS_ERR_MEM                          (NSM_QOS_ERR_BASE + 1)
#define NSM_QOS_ERR_BRIDGE_NOT_FOUND             (NSM_QOS_ERR_BASE + 2)
#define NSM_QOS_ERR_VLAN_NOT_FOUND               (NSM_QOS_ERR_BASE + 3)
#define NSM_QOS_ERR_CMAP_NOT_FOUND               (NSM_QOS_ERR_BASE + 4)
#define NSM_QOS_ERR_CMAP_MATCH_TYPE              (NSM_QOS_ERR_BASE + 5)
#define NSM_QOS_ERR_EXISTS                       (NSM_QOS_ERR_BASE + 6)
#define NSM_QOS_ERR_NOT_BOUND                    (NSM_QOS_ERR_BASE + 7)
#define NSM_QOS_ERR_NOTFOUND                     (NSM_QOS_ERR_BASE + 8)
#define NSM_QOS_ERR_ALREADY_BOUND                (NSM_QOS_ERR_BASE + 9)
#define NSM_QOS_ERR_MISMATCH                     (NSM_QOS_ERR_BASE + 10)
#define NSM_QOS_ERR_GENERAL                      (NSM_QOS_ERR_BASE + 11)
#define NSM_QOS_ERR_HAL                          (NSM_QOS_ERR_BASE + 12)

#endif /* HAVE_QOS */
#endif /* __NSM_QOS_LIST_H__ */
