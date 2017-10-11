/* Copyright (C) 2004  All Rights Reserved */

#ifndef __HAL_QOS_H__
#define __HAL_QOS_H__


#ifdef HAVE_QOS

/* Define the size of COS and DSCP table */
#define HAL_COS_TBL_SIZE                8
#define HAL_IP_PREC_TBL_SIZE            8
#define HAL_DSCP_TBL_SIZE               64

#define HAL_QOS_MAX_QUEUE_SIZE          8
#define HAL_QOS_QUEUE_WEIGHT_INVLAID    11

/* Define the size of name of filter */
#define HAL_ACL_NAME_SIZE               20
#define HAL_CMAP_NAME_SIZE              20
#define HAL_PMAP_NAME_SIZE              20
#define HAL_DSCP_MUT_NAME_SIZE          20
#define HAL_AGG_POLICER_NAME_SIZE       20

/* Define the number of filter */
#define HAL_MAX_ACL_FILTER              30
#define HAL_MAX_VLAN_FILTER             30
#define HAL_MAX_DSCP_IN_CMAP            8
#define HAL_MAX_IP_PREC_IN_CMAP         8
#define HAL_MAX_EXP_IN_CMAP             8

/* Definition of policy-map */
/* Action */
#define HAL_QOS_ACTION_ATTACH           1
#define HAL_QOS_ACTION_DETACH           2

/* Starting or Ending point when attach */
#define HAL_QOS_POLICY_MAP_START        1
#define HAL_QOS_POLICY_MAP_END          2

/* Indicate the direction of policy-map */
#define HAL_QOS_DIRECTION_INGRESS       1   /* Incoming */
#define HAL_QOS_DIRECTION_EGRESS        2   /* Outgoing */

/* Define the ether-type */
#define HAL_QOS_FILTER_FMT_ETH_II       1   /* type/len >= 0x600 */
#define HAL_QOS_FILTER_FMT_802_3        2   /* type/len < 0x600 */
#define HAL_QOS_FILTER_FMT_SNAP         4   /* SNAP packet */
#define HAL_QOS_FILTER_FMT_LLC          8   /* LLC packet */

#define HAL_QOS_ACL_TYPE_MAC            1   /* MAC acl type */
#define HAL_QOS_ACL_TYPE_IP             2   /* IP acl type */

enum hal_exceed_action
{
   HAL_QOS_EXD_ACT_NONE,
   HAL_QOS_EXD_ACT_DROP,                 /* Exceed action - drop */
   HAL_QOS_EXD_ACT_POLICED_DSCP_TX,      /* Exceed action - policed dscp tx */
   HAL_QOS_EXD_ACT_POLICE_FLOW_CONTROL,  /* Exceed action - send flow control */
};

#define HAL_QOS_FILTER_DENY             0   /* deny */
#define HAL_QOS_FILTER_PERMIT           1   /* permit */

#define HAL_QOS_SET_COS                 1
#define HAL_QOS_SET_DSCP                2
#define HAL_QOS_SET_IP_PREC             3

#define HAL_QOS_CLFR_TYPE_ACL           (1 << 0)   /* The classifier type is acl-group */
#define HAL_QOS_CLFR_TYPE_VLAN          (1 << 1)   /* The classifier type is VLAN */
#define HAL_QOS_CLFR_TYPE_DSCP          (1 << 2)   /* The classifier type is DSCP */
#define HAL_QOS_CLFR_TYPE_IP_PREC       (1 << 3)  /* The classifier type is IP-PREC */
#define HAL_QOS_CLFR_TYPE_EXP           (1 << 4)  /* The classifier type is EXP */
#define HAL_QOS_CLFR_TYPE_L4_PORT       (1 << 5)  /* The classifier type is Layer 4 port */
#define HAL_QOS_CLFR_TYPE_COS_INNER     (1 << 6)  /* The classifier type is Cos Inner. L2 Cos classifier.  */

enum hal_qos_traffic_match
{
  HAL_QOS_MATCH_TYPE_AND_PRIORITY, /* Match both traffic type and queue
                                    * number for classification
                                    */
  HAL_QOS_MATCH_TYPE_OR_PRIORITY,  /* Match traffic type or queue number
                                    * for classification
                                    */
};

enum hal_qos_flow_control_mode
{
  HAL_RESUME_ON_FULL_BUCKET_SIZE,  /* De assert flow control when the
                                    * Resource bucket is empty
                                    */

  HAL_RESUME_ON_CBS_BUCKET_SIZE,  /* De assert flow control when the
                                   * Resource bucket has enough room
                                   * as specifeied by CBSLimit 
                                   */
};

enum hal_qos_trust_state
{
  HAL_QOS_TRUST_NONE,  /* Use Port¿s default priority */
  HAL_QOS_TRUST_8021P, /* Use the priority bits of vlan Tag */
  HAL_QOS_TRUST_DSCP,  /* Use the DSCP bits of IPV4/ IPV6 header */
  HAL_QOS_TRUST_BOTH,  /* Use the DSCP/ vlan priotity depending on
                        * preferred trust state */
};

enum hal_qos_rate_unit
{
  HAL_QOS_RATE_KBPS,              /* Kilo Bits per second*/
  HAL_QOS_RATE_FPS,               /* Frames per second */
};

#define HAL_QOS_NO_TRAFFIC_SHAPE          (0)

enum hal_qos_vlan_pri_override
{
  HAL_QOS_VLAN_PRI_OVERRIDE_NONE,  /* NO VLAN priority override */
  HAL_QOS_VLAN_PRI_OVERRIDE_COS,   /* Override COS based on VLAN priority */
  HAL_QOS_VLAN_PRI_OVERRIDE_QUEUE, /* Override queue based on VLAN priority */
  HAL_QOS_VLAN_PRI_OVERRIDE_BOTH,  /* Override COS and queue based on VLAN
                                      priority*/
};
  
enum hal_qos_da_pri_override
{
  HAL_QOS_DA_PRI_OVERRIDE_NONE,  /* NO DA priority override */
  HAL_QOS_DA_PRI_OVERRIDE_COS,   /* Override COS based on DA priority */
  HAL_QOS_DA_PRI_OVERRIDE_QUEUE, /* Override queue based on DA priority */
  HAL_QOS_DA_PRI_OVERRIDE_BOTH,  /* Override COS and queue based on DA
                                      priority*/
};

enum hal_qos_service_policy_direction
{
  HAL_QOS_SERVICE_POLICY_IN,   /* Policy map to applied to ingress traffic */
  HAL_QOS_SERVICE_POLICY_OUT,  /* Policy map to applied to egress traffic  */
};

enum hal_qos_frame_type
{
  HAL_QOS_FTYPE_DSA_TO_CPU_BPDU,       /* BPDUs (Multicast MGMT frames)
                                        * in DSA format directed to CPU */
  HAL_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT, /* Unicast MGMT frames in DSA format
                                        * directed to CPU. */
  HAL_QOS_FTYPE_DSA_FROM_CPU,          /* For all DSA frames from CPU Port */
  HAL_QOS_FTYPE_PORT_ETYPE_MATCH,      /* Non DSA control frames matching
                                        * ports Etype */
  HAL_QOS_FTYPE_MAX,
};

/* Define the qos global */
struct hal_min_res
{
    u_int8_t level_id;                      /* Indicator of minimum reserve level */
    u_int8_t packets;                       /* Indicates the number of packets */
};


struct hal_map_tbls
{
    u_int8_t cos_dscp_map_tbl[HAL_COS_TBL_SIZE];            /* CoS-to-DSCP map */
    u_int8_t pri_cos_map_tbl[HAL_COS_TBL_SIZE];             /* Priority to CoS map */
    u_int8_t ip_prec_dscp_map_tbl[HAL_IP_PREC_TBL_SIZE];    /* IP Prec-to-DSCP map */

    u_int8_t dscp_cos_map_tbl[HAL_DSCP_TBL_SIZE];           /* DSCP-to-CoS map */
    u_int8_t dscp_pri_map_tbl[HAL_DSCP_TBL_SIZE];           /* DSCP-to-Prio map */
    u_int8_t policed_dscp_map_tbl[HAL_DSCP_TBL_SIZE];       /* Policed DSCP map */
    u_int8_t default_dscp_mut_map_tbl[HAL_DSCP_TBL_SIZE];   /* default dscp-mut map */
};

struct hal_mac_addr
{
    u_int8_t mac[6];                        /* MAC address */
};


/* Definition of MAC acl filter */
struct hal_mac_acl_filter                   /* MAC acl filter */
{
    u_int8_t deny_permit;                   /* deny or permit */
    u_int8_t priority;                      /* Priority of the MAC */
    u_int8_t l2_type;                       /* The packet format */

    int extended;

    struct hal_mac_addr a;                  /* Source MAC address */
    struct hal_mac_addr a_mask;             /* Source mask (prefix) */
    struct hal_mac_addr m;                  /* Destination MAC address */
    struct hal_mac_addr m_mask;             /* Destination mask (prefix) */
};


struct hal_ipv4_addr
{
    u_int8_t ip[4];
};


/* Definition of IP acl filter */
struct hal_ipv4_acl_filter                  /* IP acl filter */
{
    u_int8_t deny_permit;                   /* deny or permit */
    u_int8_t ver;                           /* IP version number */
    u_int8_t hlen;                          /* IP header length */
    u_int8_t prot;                          /* IP protocol number */

    int extended;

    struct hal_ipv4_addr addr;              /* Source IP address */
    struct hal_ipv4_addr addr_mask;         /* Source mask address (prefix) */
    struct hal_ipv4_addr mask;              /* Destination IP address */
    struct hal_ipv4_addr mask_mask;         /* Destination mask address (prefix) */
};


/* Definition of acl filter */
struct hal_acl_filter
{
    char name[HAL_ACL_NAME_SIZE];           /* ACL name */
    u_int8_t mac_ip_ind;                    /* Indicate MAC(1) or IP (2) */
    u_int8_t ace_num;                       /* Indicate ACE (ACL Entity) number */
    union hal_acl_element                   /* ACE */
    {
        struct hal_mac_acl_filter mfilter;  /* MAC acl filter */
        struct hal_ipv4_acl_filter ifilter; /* IP acl filter */
    } ace[HAL_MAX_ACL_FILTER];
};


/* Definition of vlan filter */
struct hal_vlan_filter
{
    u_int8_t num;                           /* The number of valid VLANs */
    u_int16_t vlan[HAL_MAX_VLAN_FILTER];    /* vlan filter in class-map */
};


/* Definition of the policer */
struct hal_police
{
    u_int32_t avg;                          /* Average traffic (kbps) */
    u_int32_t burst;                        /* Normal burst size */
    u_int32_t excess_burst;                 /* Normal burst size */
    enum hal_exceed_action exd_act;         /* Exceed Action */
    u_int8_t dscp;                          /* Policed (droped) DSCP value */
    enum hal_qos_flow_control_mode fc_mode; /* flow control mode */
};


/* Definition of the set */
struct hal_set
{
    u_int8_t type;         /* Indicator(cos(1)|ip_dscp(2)|ip_prec(3)|exp(4)) */
    u_int8_t val;          /* Value of cos/ip_dscp/ip_prec/exp */
};

/* Definition of the set */
struct hal_trust
{
    u_int8_t cos_dscp_prec_ind;             /* Indicator(cos(1)|ip_dscp(2)|ip_prec(3)) */
    u_int8_t val;                           /* port trust state of cos/ip_dscp/ip_prec */
};


/* Definition of the dscp of cmap */
struct hal_cmap_dscp
{
    u_int8_t num;                           /* The number of valid DSCPs  */
    u_int8_t dscp[HAL_MAX_DSCP_IN_CMAP];    /* DSCP values*/
};

/* Definition of the dscp of cmap */
struct hal_cmap_ip_prec
{
    u_int8_t num;                           /* The number of valid DSCPs  */
    u_int8_t prec[HAL_MAX_DSCP_IN_CMAP];    /* IP-Precedence values*/
};

/* Definition of the exp of cmap */
struct hal_cmap_exp
{
    u_int8_t num;                           /* The number of valid EXPs  */
    u_int8_t exp[HAL_MAX_EXP_IN_CMAP];      /* MPLS exp-bit values*/
};

struct hal_cmap_l4_port
{
#define HAL_QOS_LAYER4_PORT_NONE  0
#define HAL_QOS_LAYER4_PORT_SRC   1
#define HAL_QOS_LAYER4_PORT_DST   2
  u_char port_type;
  u_int16_t port_id;
};

struct hsl_msg_qos_vlan_queue_priority
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

struct hal_cmap_match_traffic
{ 
  u_int32_t traffic_type;
  enum hal_qos_traffic_match mode;
};

/* Definition of the class map structure for communication with HSL */
struct hal_class_map
{
  char name[HAL_CMAP_NAME_SIZE];          /* Class map name */
  
  u_int8_t clfr_type_ind;                 /* Indicate classifier acl(1)/vlan(2)/dscp(4)/ip(8) */
  
  struct hal_acl_filter acl;              /* MAC/IP ACL */
  struct hal_vlan_filter v;               /* VLAN filter */
  struct hal_police p;                    /* Police */
  struct hal_set s;                       /* For setting a new value */
  struct hal_trust t;                     /* For setting a new value */
  struct hal_cmap_dscp d;                 /* DSCP values */
  struct hal_cmap_ip_prec i;              /* IP-Precedence values */
  struct hal_cmap_exp e;                  /* MPLS exp-bit values */
  struct hal_cmap_l4_port l4_port;        /* Layer 4 port */
  char agg_policer_name[HAL_AGG_POLICER_NAME_SIZE + 1];

  /* DRR settings */
  u_int8_t drr_priority;
  u_int8_t drr_quantum;
  
  /* For Policing & Shaping*/
  u_int8_t policing_data_ratio;
  
  /* vlan queue priority */
  struct hsl_msg_qos_vlan_queue_priority queue_priority;
  
  /* scheduling algo*/
  int scheduling_algo;
  
  /* traffic type*/
  struct hal_cmap_match_traffic match_traffic;
  
  /*  port priority*/
  enum hal_qos_vlan_pri_override port_mode;

};


struct hal_dscp_map_table
{
  int in_dscp;
  int out_dscp;
  int out_pri;
};




/* Definition of the policy map structure for communication with HSL */
struct hal_policy_map
{
    char name[HAL_PMAP_NAME_SIZE];          /* Policy map name */

    /* DRR settings */
    u_int16_t drr_priority;
    u_int16_t drr_quantum;
    /* For Policing & Shaping*/
    u_int16_t policing_data_ratio;
};

struct hal_cos_queue
{
  u_int8_t cos;                           /* COS value */
  u_int8_t qid;                           /* Queue values*/
};

struct hal_dscp_queue
{
  u_int8_t dscp;                           /* COS value */
  u_int8_t qid;                           /* Queue values*/
};

/* Define the type of traffic */
#define HAL_QOS_TRAFFIC_UNKNOWN_UNICAST        (1 << 0)
#define HAL_QOS_TRAFFIC_UNKNOWN_MULTICAST      (1 << 1)
#define HAL_QOS_TRAFFIC_BROADCAST              (1 << 2)
#define HAL_QOS_TRAFFIC_MULTICAST              (1 << 4)
#define HAL_QOS_TRAFFIC_UNICAST                (1 << 5)
#define HAL_QOS_TRAFFIC_MGMT                   (1 << 6)
#define HAL_QOS_TRAFFIC_ARP                    (1 << 7)
#define HAL_QOS_TRAFFIC_TCP_DATA               (1 << 8)
#define HAL_QOS_TRAFFIC_TCP_CTRL               (1 << 9)
#define HAL_QOS_TRAFFIC_UDP                    (1 << 10)
#define HAL_QOS_TRAFFIC_NON_TCP_UDP            (1 << 11)
#define HAL_QOS_TRAFFIC_QUEUE0                 (1 << 12)
#define HAL_QOS_TRAFFIC_QUEUE1                 (1 << 13)
#define HAL_QOS_TRAFFIC_QUEUE2                 (1 << 14)
#define HAL_QOS_TRAFFIC_QUEUE3                 (1 << 15)
#define HAL_QOS_TRAFFIC_ALL                    (1 << 16)
#define HAL_QOS_TRAFFIC_NONE                   (0)

#define HAL_QOS_QUEUE_WEIGHT_INVALID           11

/* Prototypes */
int hal_qos_init();
int hal_qos_deinit();

int hal_qos_enable(char *q0, char *q1, char *q2, char *q3, char *q4, char *q5, char *q6, char *q7);
int hal_qos_enable_new ();
int hal_qos_disable();

int hal_qos_wrr_queue_limit(int ifindex, int *ratio);
int hal_qos_no_wrr_queue_limit(int ifindex, int *ratio);
int hal_qos_wrr_queue_limit_send_message(int ifindex, int *ratio);

int hal_qos_wrr_tail_drop_threshold(int ifindex, int qid, int tail1, int tail2);

int hal_qos_wrr_threshold_dscp_map(u_int32_t ifindex, int thid, u_char *dscp, int count);

int hal_qos_wrr_wred_drop_threshold(int ifindex, int qid, int thid1, int thid2);
int hal_qos_no_wrr_wred_drop_threshold(int ifindex, int qid, int thid1, int thid2);

int hal_qos_wrr_set_bandwidth_of_queue(int ifindex, int *bw);
int hal_qos_no_wrr_set_bandwidth_of_queue(int ifindex, int *bw);
int hal_qos_wrr_set_bandwidth_send_message(int ifindex, int *bw);

int hal_qos_wrr_queue_cos_map_set (u_int32_t ifindex, int qid, u_char cos[], int count);
int hal_qos_wrr_queue_cos_map_unset (u_int32_t ifindex, u_char cos[], int count);

int hal_qos_set_min_reserve_level(int level, int packet_size);

int hal_qos_wrr_queue_min_reserve(int ifindex, int qid, int level);
int hal_qos_no_wrr_queue_min_reserve(int ifindex, int qid, int level);

int hal_qos_set_trust_state_for_port(int ifindex, int trust_state);

int hal_qos_set_default_cos_for_port(int ifindex, int cos_value);

int hal_qos_set_dscp_mapping_for_port(int, int, struct hal_dscp_map_table *, int);
int hal_qos_set_policy_map(int ifindex, int action, int dir);
int hal_qos_send_policy_map_end(int ifindex, int action, int dir);
int hal_qos_send_policy_map_detach(int ifindex, int action, int dir);
int hal_qos_set_class_map (struct hal_class_map *hal_cmap, int ifindex,
                           int action, int dir);
int hal_qos_set_cmap_cos_inner (struct hal_class_map *hal_cmap, int ifindex,
                                int action);
int hal_qos_wrr_set_expedite_queue(u_int32_t ifindex, int qid, int weight);
int hal_qos_wrr_unset_expedite_queue(u_int32_t ifindex, int qid, int weight);

int hal_qos_port_enable (u_int32_t ifindex);
int hal_qos_port_disable (u_int32_t ifindex);

int
hal_qos_cos_to_queue (u_int8_t cos, u_int8_t queue_id);

int
hal_qos_dscp_to_queue (u_int8_t dscp, u_int8_t queue_id);

int
hal_qos_dscp_to_queue_unset (u_int8_t dscp, u_int8_t queue_id);

int
hal_qos_set_trust_state (u_int32_t ifindex,
                         enum hal_qos_trust_state trust_state);
int 
hal_qos_set_force_trust_cos (u_int32_t ifindex, u_int8_t force_trust_cos);

int 
hal_qos_set_frame_type_priority_override (enum hal_qos_frame_type frame_type,
                                          u_int8_t queue_id);

int
hal_qos_unset_frame_type_priority_override (enum hal_qos_frame_type frame_type);

int
hal_qos_set_vlan_priority (u_int16_t vid, u_int8_t priority);

int
hal_qos_unset_vlan_priority (u_int16_t vid);

int
hal_qos_set_port_vlan_priority_override (u_int32_t ifindex,
                                 enum hal_qos_vlan_pri_override port_mode);

int
hal_qos_set_port_da_priority_override (u_int32_t ifindex,
                                 enum hal_qos_da_pri_override port_mode);

int
hal_mls_qos_set_queue_weight (int *queue_weight);

int
hal_qos_set_egress_rate_shape (u_int32_t ifindex, u_int32_t egress_rate,
                               enum hal_qos_rate_unit rate_unit);
int
hal_qos_set_strict_queue (u_int32_t ifindex, u_int8_t strict_queue);

int
hal_qos_set_strict_queue (u_int32_t ifindex, u_int8_t strict_queue);

int
hal_l2_qos_get_num_coq (s_int32_t *num_cosq);

int
hal_l2_qos_set_num_coq (s_int32_t num_cosq);

#endif /* HAVE_QOS */
#endif /* __HAL_QOSL_H__ */
