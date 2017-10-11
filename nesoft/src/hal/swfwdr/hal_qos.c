/* Copyright (C) 2003  All Rights Reserved. 

QOS HAL

*/

#include "pal.h"
#include "lib.h"

#ifdef HAVE_QOS
#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_qos.h"

int
hal_qos_init()
{
  return 0;
}

int
hal_qos_deinit()
{
  return 0;
}

int
hal_qos_enable_new ()
{
  return 0;
}


int
hal_qos_enable(char *q0, char *q1, char *q2, char *q3, char *q4, char *q5, char *q6, char *q7)
{
  return HAL_SUCCESS;
}

/*
 * Disable QoS globlly
 */
int
hal_qos_disable()
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_queue_limit_send_messgage (int ifindex, int *ratio)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_queue_limit(int ifindex, int *ratio)
{
  return HAL_SUCCESS;
}

int
hal_qos_no_wrr_queue_limit(int ifindex, int *ratio)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_tail_drop_threshold(int ifindex, int qid, int thres1, int thres2)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_threshold_dscp_map (u_int32_t ifindex, int thid, u_char *dscp, int num)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_wred_drop_threshold(int ifindex, int qid, int thid1, int thid2)
{
  return 0;
}

int
hal_qos_no_wrr_wred_drop_threshold(int ifindex, int qid, int thid1, int thid2)
{
  return 0;
}

int
hal_qos_wrr_set_bandwidth_send_message (int ifindex, int *bw)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_set_bandwidth_of_queue(int ifindex, int *bw)
{
  return HAL_SUCCESS;
}

int
hal_qos_no_wrr_set_bandwidth_of_queue(int ifindex, int *bw)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_queue_cos_map_send_message (int ifindex, int qid, int cos)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_queue_cos_map(int ifindex, int qid, int cos)
{
  return HAL_SUCCESS;
}

int
hal_qos_no_wrr_queue_cos_map(int ifindex, int qid, int cos)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_min_reserve_level(int level, int packet_size)
{
  return 0;
}


int
hal_qos_wrr_queue_min_reserve(int ifindex, int qid, int level)
{
  return 0;
}

int
hal_qos_no_wrr_queue_min_reserve(int ifindex, int qid, int level)
{
  return 0;
}

int
hal_qos_set_trust_state_for_port(int ifindex, int trust_state)
{
  return HAL_SUCCESS;
}


int
hal_qos_set_cmap_cos_inner (struct hal_class_map *hal_cmap, int ifindex, int action)
{
  return HAL_SUCCESS;
}
int
hal_qos_send_policy_map_end(int ifindex,
                            int action,
                            int dir)
{
  return HAL_SUCCESS;
}

int
hal_qos_send_policy_map_detach(int ifindex,
                               int action,
                               int dir)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_policy_map(int ifindex,
                       int action,
                       int dir)
{
  return 0;
}

int
hal_qos_wrr_set_expedite_queue(u_int32_t ifindex, int qid, int weight)
{
  return 0;
}

int
hal_qos_wrr_unset_expedite_queue(u_int32_t ifindex, int qid, int weight)
{
  return 0;
}

int
hal_qos_wrr_queue_cos_map_set (u_int32_t ifindex, int qid, u_char cos[], int count)
{
  return 0;
}

int
hal_qos_wrr_queue_cos_map_unset (u_int32_t ifindex, u_char cos[], int count)
{
  return 0;
}

int
hal_qos_set_default_cos_for_port(int ifindex, int cos_value)
{
  return 0;
}

int
hal_qos_set_dscp_mapping_for_port (int ifindex, int flag, 
                                   struct hal_dscp_map_table *map_table,
                                   int count)
{
  return HAL_SUCCESS;
}


int
hal_qos_set_class_map (struct hal_class_map *hal_cmap,
                       int ifindex,
                       int action,
                       int dir)
{
  return HAL_SUCCESS;
}

int
hal_qos_cos_to_queue (u_int8_t cos, u_int8_t queue_id)
{
  return HAL_SUCCESS;
}

int
hal_qos_dscp_to_queue (u_int8_t dscp, u_int8_t queue_id)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_trust_state (u_int32_t ifindex, enum hal_qos_trust_state trust_state)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_force_trust_cos (u_int32_t ifindex, u_int8_t force_trust_cos)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_egress_rate_shape (u_int32_t ifindex, u_int32_t egress_rate,
                               enum hal_qos_rate_unit rate_unit)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_frame_type_priority_override (enum hal_qos_frame_type frame_type,
                                          u_int8_t queue_id)
{
  return HAL_SUCCESS;
}

int
hal_qos_unset_frame_type_priority_override (enum hal_qos_frame_type frame_type)
{
  return HAL_SUCCESS;
}


int
hal_qos_set_vlan_priority (u_int16_t vid, u_int8_t priority)
{
  return HAL_SUCCESS;
}

int
hal_qos_unset_vlan_priority (u_int16_t vid)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_port_vlan_priority_override (u_int32_t ifindex, 
                                         enum hal_qos_vlan_pri_override port_mode)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_port_da_priority_override (u_int32_t ifindex, 
                                       enum hal_qos_da_pri_override port_mode)
{
  return HAL_SUCCESS;
}

int
hal_mls_qos_set_queue_weight (int *queue_weight)
{
  return HAL_SUCCESS;
}

int
hal_qos_set_strict_queue (u_int32_t ifindex, u_int8_t strict_queue)
{
  return HAL_SUCCESS;
}
#endif /* HAVE_QOS */
