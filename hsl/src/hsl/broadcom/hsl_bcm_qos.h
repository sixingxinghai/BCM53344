/* Copyright (C) 2004  All Rights Reserved. */



#ifndef _HSL_BCM_QOS_H_
#define _HSL_BCM_QOS_H_

#ifdef HAVE_QOS

#define HSL_BCM_COS_QUEUES                      8

#define HSL_QOS_FILTER_DSCP                     (1<<0)
#define HSL_QOS_FILTER_VLAN                     (1<<1)
#define HSL_QOS_FILTER_SRC_IP                   (1<<2)
#define HSL_QOS_FILTER_DST_IP                   (1<<3)
#define HSL_QOS_FILTER_SRC_MAC                  (1<<4)
#define HSL_QOS_FILTER_DST_MAC                  (1<<5)
#define HSL_QOS_FILTER_L4_PORT                  (1<<6)
#define HSL_QOS_FILTER_IP_PRECEDENCE            (1<<7)
#define HSL_QOS_FILTER_L2FRAME_FORMAT           (1<<8)

#define HSL_QOS_VLAN_OFFSET            14 
#define HSL_QOS_COS_INNER_OFFSET       14 
#define HSL_QOS_DSCP_OFFSET            19
#define HSL_QOS_IP_PRECEDENCE_OFFSET   19 
#define HSL_QOS_DST_IP_OFFSET          34 
#define HSL_QOS_SRC_IP_OFFSET          30 
#define HSL_QOS_SRC_L4_PORT_OFFSET     38
#define HSL_QOS_DST_L4_PORT_OFFSET     40
#define HSL_QOS_SRC_MAC_OFFSET         6 
#define HSL_QOS_DST_MAC_OFFSET         0 

#define HSL_QOS_VLAN_MASK             0x0fff
#define HSL_QOS_COS_INNER_MASK        0xe0
#define HSL_QOS_DSCP_MASK             0xfc
#define HSL_QOS_IP_PRECEDENCE_MASK    0xe0

#define HSL_BCM_FILTER_PRIORITY_MAX  15

typedef int bcm_ds_clfr_t;	//qcl 20170808
typedef int bcm_ds_inprofile_actn_t;	//qcl 20170808
typedef int bcm_ds_outprofile_actn_t;	//qcl 20170808
typedef int bcm_ds_nomatch_actn_t;	//qcl 20170808


int hsl_bcm_cosq_detach ();
int hsl_bcm_qos_init ();
int hsl_bcm_qos_deinit ();
int hsl_bcm_qos_enable (char *q0, char *q1, char *q2, char *q3, char *q4, char *q5, char *q6, char *q7);
int hsl_bcm_qos_disable (int num_queue);
int hsl_bcm_qos_wrr_queue_limit (int ifindex, int *ratio);

int hsl_bcm_qos_port_cosq_bandwidth_set(hal_wrr_car_info_t wrr_car_info);//zlh@150120 set each cosq minbucket/maxbucket
int hsl_bcm_qos_port_cosq_weight_set(hal_wrr_weight_info_t wrr_weight_info);//zlh@150120 set each cosq wdrr and weight value
int hsl_bcm_qos_port_cosq_wred_drop_set(hal_wrr_wred_info_t wrr_wred_info);//zlh@150120 set each cosq wred

int hsl_bcm_qos_wrr_tail_drop_threshold (int ifindex, int qid, int thres1, int thres2);
int hsl_bcm_qos_wrr_threshold_dscp_map (int ifindex, int thid, int num, int *dscp);
int hsl_bcm_qos_wrr_wred_drop_threshold (int ifindex, int qid, int thres1, int thres2);
int hsl_bcm_qos_wrr_set_bandwidth (int ifindex, int *bw);
int hsl_bcm_qos_wrr_queue_cos_map (int ifindex, int qid, int cos);
int hsl_bcm_qos_wrr_queue_min_reserve (int ifindex, int qid, int packets);
int hsl_bcm_qos_set_trust_state (int ifindex, int trust_state);
int hsl_bcm_qos_set_default_cos (int ifindex, int cos_value);
int hsl_bcm_qos_set_dscp_mapping_tbl (int, int, struct hal_msg_dscp_map_table *, int);
int hsl_bcm_qos_set_dscp_cos_tbl (int ifindex, int flag, unsigned char *cos);
int hsl_bcm_qos_set_class_map (struct hal_msg_qos_set_class_map *msg);
int hsl_bcm_qos_set_policy_map (struct hal_msg_qos_set_policy_map *msg);
int hsl_bcm_qos_clr_ds_arg (bcm_ds_clfr_t *clfr, bcm_ds_inprofile_actn_t *inp_actn, bcm_ds_outprofile_actn_t *outp_actn, bcm_ds_nomatch_actn_t *nm_actn, int *cflag, int *iflag, int *oflag, int *nflag);
void hsl_bcm_qos_if_filter_delete_all (struct hsl_bcm_if *);
void hsl_bcm_qos_if_meter_delete_all (struct hsl_bcm_if *);
int hsl_bcm_qos_set_cmap_cos_inner (struct hal_msg_qos_set_class_map *msg);
int hsl_bcm_l2_get_num_cosq (int *num);
int hsl_bcm_qos_port_traffic_class_set (int ifindex, unsigned char priority, unsigned char traffic_class_value);

#endif /* HAVE_QOS */
#endif /* _HSL_BCM_QOS_H_ */
