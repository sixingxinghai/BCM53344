/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_QOS_H__
#define __NSM_QOS_H__

#ifdef HAVE_QOS

/* Define the QoS state of system */
#define NSM_QOS_DISABLED_STATE      0
#define NSM_QOS_ENABLED_STATE       1

/* Define the range of DSCP and COS */
#define NSM_QOS_DSCP_MIN            0
#define NSM_QOS_DSCP_MAX            63
#define NSM_QOS_COS_MIN             0
#define NSM_QOS_COS_MAX             7
#define NSM_QOS_COS_INNER           (NSM_QOS_COS_MAX+1)


#define NSM_QOS_DEFAULT_EXPEDITE_QUEUE        7
#define NSM_QOS_IF_QUEUE_WEIGHT_DEFAULT       (100/8)

/* Define the size of MAPs name */
#define MAP_NAME_SIZE               20


/* Define the size of COS and DSCP table */
#define COS_TBL_SIZE                8
#define IPPREC_TBL_SIZE             8
#define DSCP_TBL_SIZE               64

/* Define the number of available queue */
/* Maximum number of available queue */
#define MAX_NUM_OF_QUEUE            8
#define MAX_NUM_LVL_ID              9

#if 0
/* Define the max and min of queue id */
#define NSM_QOS_TH_ID_MIN           1
#define NSM_QOS_TH_ID_MAX           2
#endif

/* Define the port trust state */
#define NSM_QOS_TRUST_NONE          0
#define NSM_QOS_TRUST_COS           1
#define NSM_QOS_TRUST_DSCP          2
#define NSM_QOS_TRUST_IP_PREC       3
#define NSM_QOS_TRUST_COS_PT_DSCP   4
#define NSM_QOS_TRUST_DSCP_PT_COS   5
#define NSM_QOS_TRUST_DSCP_COS      6

/* Define the indicator of set in class-map */
#define NSM_QOS_SET_COS             1
#define NSM_QOS_SET_DSCP            2
#define NSM_QOS_SET_IP_PREC         3
#define NSM_QOS_SET_EXP             4 

/* Define the action of service-policy map */
#define NSM_QOS_ACTION_ADD          1
#define NSM_QOS_ACTION_DELETE       2


/* Define the direction of service-policy map */
#define NSM_QOS_DIRECTION_INGRESS   1
#define NSM_QOS_DIRECTION_EGRESS    2


/* Define the size of name of filter */
#define MAX_ACL_NAME_SIZE           20
#define MAX_CMAP_NAME_SIZE          20
#define MAX_PMAP_NAME_SIZE          20
#define MAX_DSCP_MUT_NAME_SIZE      20


/* Define the number of filter */
#define MAX_NUM_OF_ACL_FILTER       30
#define MAX_NUM_OF_VLAN_FILTER      30

#define MAX_NUM_OF_CLASS_IN_PMAPL   8

#define MAX_NUM_OF_DSCP_IN_CMAP     8
#define MAX_NUM_OF_IP_PREC_IN_CMAP  8
#define MAX_NUM_OF_EXP_IN_CMAP      8

/* Define the number of max DSCP mutation table */
#define MAX_NUM_OF_DSCP_MUT_MAP_TBL 4


/* Define the ether-type */
#define NSM_QOS_FILTER_FMT_ETH_II   1       /* type/len >= 0x600 */
#define NSM_QOS_FILTER_FMT_802_3    2       /* type/len < 0x600 */
#define NSM_QOS_FILTER_FMT_SNAP     4       /* SNAP packet */
#define NSM_QOS_FILTER_FMT_LLC      8       /* LLC packet */

#define NSM_QOS_ACL_TYPE_MAC        1       /* MAC acl type */
#define NSM_QOS_ACL_TYPE_IP         2       /* IP acl type */

/* Define the type of traffic */
#define NSM_QOS_TRAFFIC_UNKNOWN_UNICAST        (1 << 0)
#define NSM_QOS_TRAFFIC_UNKNOWN_MULTICAST      (1 << 1)
#define NSM_QOS_TRAFFIC_BROADCAST              (1 << 2)
#define NSM_QOS_TRAFFIC_MULTICAST              (1 << 4)
#define NSM_QOS_TRAFFIC_UNICAST                (1 << 5)
#define NSM_QOS_TRAFFIC_MGMT                   (1 << 6)
#define NSM_QOS_TRAFFIC_ARP                    (1 << 7)
#define NSM_QOS_TRAFFIC_TCP_DATA               (1 << 8)
#define NSM_QOS_TRAFFIC_TCP_CTRL               (1 << 9)
#define NSM_QOS_TRAFFIC_UDP                    (1 << 10)
#define NSM_QOS_TRAFFIC_NON_TCP_UDP            (1 << 11)
#define NSM_QOS_TRAFFIC_QUEUE0                 (1 << 12)
#define NSM_QOS_TRAFFIC_QUEUE1                 (1 << 13)
#define NSM_QOS_TRAFFIC_QUEUE2                 (1 << 14)
#define NSM_QOS_TRAFFIC_QUEUE3                 (1 << 15)
#define NSM_QOS_TRAFFIC_ALL                    (1 << 16)
#define NSM_QOS_TRAFFIC_NONE                   (0)

#define NSM_QOS_QUEUE0                         0
#define NSM_QOS_QUEUE1                         1
#define NSM_QOS_QUEUE2                         2
#define NSM_QOS_QUEUE3                         3
#define NSM_QOS_QUEUE4                         4
#define NSM_QOS_QUEUE5                         5
#define NSM_QOS_QUEUE6                         6
#define NSM_QOS_QUEUE7                         7

enum nsm_qos_traffic_match
{
  NSM_QOS_CRITERIA_TTYPE_AND_PRI, /* Match both traffic type and queue 
                                     number for classification */
  NSM_QOS_CRITERIA_TTYPE_OR_PRI,  /* Match traffic type or queue number 
                                     for classification */
  NSM_QOS_CRITERIA_NONE,
};

#define NSM_QOS_NO_TRAFFIC_SHAPE               0

enum nsm_qos_rate_unit
{
  NSM_QOS_RATE_KBPS,       /* Kilo Bits per second*/
  NSM_QOS_RATE_FPS,        /* Frames per second */
};

enum nsm_exceed_action 
{
  NSM_QOS_EXD_ACT_NONE,                 /* Exceed Action - None */
  NSM_QOS_EXD_ACT_DROP,                 /* Exceed action - drop */
  NSM_QOS_EXD_ACT_POLICED_DSCP_TX,      /* Exceed action - policed dscp tx */
  NSM_QOS_EXD_ACT_POLICE_FLOW_CONTROL,  /* Send a pause frame on the ingress 
                                         * port and pass the packet. */
};

enum nsm_qos_flow_control_mode
{
  NSM_QOS_FLOW_CONTROL_MODE_NONE,
  NSM_RESUME_ON_FULL_BUCKET_SIZE,  /* De assert flow control when the
                                    * Resource bucket is empty  */

  NSM_RESUME_ON_CBS_BUCKET_SIZE,    /* De assert flow control when the
                                     * Resource bucket has enough room 
                                     * as specifeied by CBSLimit */ 

}; 

enum nsm_qos_frame_type
{
  NSM_QOS_FTYPE_DSA_TO_CPU_BPDU,       /* BPDUs (Multicast MGMT frames)
                                        * in DSA format directed to CPU */
  NSM_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT, /* Unicast MGMT frames in DSA format
                                        * directed to CPU. */
  NSM_QOS_FTYPE_DSA_FROM_CPU,          /* For all DSA frames from CPU Port */
  NSM_QOS_FTYPE_PORT_ETYPE_MATCH,      /* Non DSA control frames matching ports Etype */                                  
  NSM_QOS_FTYPE_MAX,
};

/* Define the qos global */
struct min_res
{
  u_int8_t   level_id;                    /* Indicator of minimum reserve level */
  u_int8_t   packets;                     /* Indicates the number of packets */
};

struct nsm_qos_vlan_pri
{
  u_int8_t brno;
  u_int16_t vid;
  u_int8_t prio;
};

#define NSM_QOS_QUEUE_ID_MASK            (0x03)

struct nsm_qos_global
{
  u_int8_t state;                         /* Enabled(1)/Disabled(0) stæte */
  struct nsm_qos_dscpml_master *dscpml;   /* DSCP mutation master */
  struct nsm_qos_dcosl_master *dcosl;     /* DSCP-to-COS master */
  struct nsm_qos_agp_master *agp;         /* Aggregate-policer master */
  struct nsm_qos_if_master *qif;          /* QoS interface master */
  int level[MAX_NUM_LVL_ID];         /* From 0 to 8 */

#define NSM_QOS_DSCP_QUEUE_CONF            (1 << 5)
  u_int8_t dscp_queue_map_tbl [DSCP_TBL_SIZE];

#define NSM_QOS_COS_QUEUE_CONF             (1 << 5)
  u_int8_t cos_queue_map_tbl [COS_TBL_SIZE];

#define NSM_QOS_FTYPE_PRI_OVERRIDE_CONF    (1 << 5)
  u_int8_t ftype_pri_tbl [NSM_QOS_FTYPE_MAX];

  struct min_res que_min_res [MAX_NUM_OF_QUEUE];

#define NSM_QOS_NUM_OF_QUEUES             8
#define NSM_QOS_QUEUE_WEIGHT_INDEX        0
#define NSM_QOS_QUEUE_COS_INDEX           1
#define NSM_QOS_QUEUE_WEIGHT_INVALID      33

  char q0[2];
  char q1[2];
  char q2[2];
  char q3[2];
  char q4[2];
  char q5[2];
  char q6[2];
  char q7[2];

  int config_new;

#ifdef HAVE_VLAN
  struct avl_tree *vlan_pri_tree;
#endif /* HAVE_VLAN */

#ifdef HAVE_HA
  HA_CDR_REF nsm_qosg_data_cdr_ref;
#endif /*HAVE_HA*/
};

struct dscp_mut
{
  u_int8_t    dscp[DSCP_TBL_SIZE];
};

struct map_tbls
{
  u_int8_t    cos_dscp_map_tbl[COS_TBL_SIZE];          /* CoS-to-DSCP map */
  u_int8_t    pri_cos_map_tbl[COS_TBL_SIZE];           /* Priority to CoS map */
  u_int8_t    ip_prec_dscp_map_tbl[COS_TBL_SIZE];      /* IP Prec-to-DSCP map */

  u_int8_t    dscp_cos_map_tbl[DSCP_TBL_SIZE];         /* DSCP-to-CoS map */
  u_int8_t    dscp_pri_map_tbl[DSCP_TBL_SIZE];         /* DSCP-to-Prio map */
  u_int8_t    policed_dscp_map_tbl[DSCP_TBL_SIZE];     /* Policed DSCP map */
  u_int8_t    default_dscp_mut_map_tbl[DSCP_TBL_SIZE]; /* default dscp-mut map */
};

/* Definition of vlan filter */
struct vlan_filter
{
  u_int8_t num;                           /* The number of valid VLANs */
  u_int16_t vlan[MAX_NUM_OF_VLAN_FILTER]; /* vlan filter in class-map */
};


/* Definition of the policer */
struct police
{
  u_int32_t avg;                          /* Average traffic (bps) */
  u_int32_t burst;                        /* Normal burst size */
  u_int32_t excess_burst;                  /* Excess Normal burst size */
  enum nsm_exceed_action exd_act;          /* Exceed Action */
  u_int8_t  dscp;                          /* Policed DSCP value */
  enum nsm_qos_flow_control_mode fc_mode;  /* flow control mode */
};


/* Definition of the set */
struct set
{
  u_int8_t type;           /* Indicator(cos(1)|ip_dscp(2)|ip_prec(3)|exp(4)) */
  u_int8_t val;            /* Value of cos/ip_dscp/ip_prec/exp */
};

/* Definition of the set */
struct trust
{
  u_int8_t cos_dscp_prec_ind;             /* Indicator(cos(1)|ip_dscp(2)|ip_prec(3)) */
  u_int8_t val;                           /* port trust state of cos/ip_dscp/ip_prec */
};


/* Definition of the dscp of cmap */
struct cmap_dscp
{
  u_int8_t num;                           /* The number of valid DSCPs  */
  u_int8_t dscp[MAX_NUM_OF_DSCP_IN_CMAP]; /* DSCP values*/
};

/* Definition of the dscp of cmap */
struct cmap_ip_prec
{
  u_int8_t num;                           /* The number of valid DSCPs  */
  u_int8_t prec[MAX_NUM_OF_IP_PREC_IN_CMAP]; /* IP-Precedence values*/
};

/* Definition of the EXP of cmap */
struct cmap_exp
{
  u_int8_t num;                           /* The number of valid EXPs */
  u_int8_t exp[MAX_NUM_OF_EXP_IN_CMAP];   /* EXP values*/
};

struct cmap_l4_port
{
#define NSM_QOS_LAYER4_PORT_NONE  0
#define NSM_QOS_LAYER4_PORT_SRC   1
#define NSM_QOS_LAYER4_PORT_DST   2
  u_int8_t port_type;
  u_int16_t port_id;
};

struct nsm_cmap_traffic_type
{
  enum nsm_qos_traffic_match criteria;
  u_int32_t custom_traffic_type;
};

struct cos_queue
{
  u_int8_t cos;                           /* cos value */
  u_int8_t qid;                           /* Queue values*/
};

struct dscp_queue
{
  u_int8_t dscp;                           /* dscp value */
  u_int8_t qid;                           /* Queue values*/
};

struct nsm_pmap_list;

enum nsm_qos_vlan_pri_override
{
  NSM_QOS_VLAN_PRI_OVERRIDE_NONE,  /* NO VLAN priority override */
  NSM_QOS_VLAN_PRI_OVERRIDE_COS,   /* Override COS based on VLAN priority */
  NSM_QOS_VLAN_PRI_OVERRIDE_QUEUE, /* Override queue based on VLAN priority */
  NSM_QOS_VLAN_PRI_OVERRIDE_BOTH,  /* Override COS and queue based on VLAN priorit                                    y*/
};

enum nsm_qos_da_pri_override
{
  NSM_QOS_DA_PRI_OVERRIDE_NONE,  /* NO VLAN priority override */
  NSM_QOS_DA_PRI_OVERRIDE_COS,   /* Override COS based on VLAN priority */
  NSM_QOS_DA_PRI_OVERRIDE_QUEUE, /* Override queue based on VLAN priority */
  NSM_QOS_DA_PRI_OVERRIDE_BOTH,  /* Override COS and queue based on VLAN priorit                                    y*/
};

enum nsm_qos_service_policy_direction
{
  NSM_QOS_SERVICE_POLICY_IN,   /* Policy map to applied to ingress traffic */
  NSM_QOS_SERVICE_POLICY_OUT,  /* Policy map to applied to egress traffic  */
};

typedef struct nsm_qos_vlan
{
  u_int8_t vid;
  u_int8_t priority;
} nsm_vlan_qos_t;

struct cmap_match_traffic
{
  u_int16_t traffic_type;
  enum nsm_qos_traffic_match mode; 
};

int nsm_qos_hal_set_all_class_map(struct nsm_master *nm,
                                  struct nsm_pmap_list *pmapl,
                                  int ifindex,
                                  int action,
                                  int dir);

#endif /* HAVE_QOS */
#endif /* __NSM_QOS_H__ */
