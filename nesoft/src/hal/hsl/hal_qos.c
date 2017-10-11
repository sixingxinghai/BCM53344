/* Copyright (C) 2003  All Rights Reserved. 

QOS HAL

*/

#include "pal.h"
#include "lib.h"

#ifdef HAVE_QOS
#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"

#include "hal_qos.h"

int
hal_qos_init()
{
  return hal_msg_generic_request (HAL_MSG_QOS_INIT, NULL, NULL);
}

int
hal_qos_deinit()
{
  /* Deinitialize QoS. */
  hal_msg_generic_request (HAL_MSG_QOS_DEINIT, NULL, NULL);
  return 0;
}

int
hal_qos_enable_new ()
{
  int ret;
  struct hal_nlmsghdr nlh;

  /* Set header. */
  nlh.nlmsg_len = HAL_NLMSG_LENGTH (0);
  nlh.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh.nlmsg_type = HAL_MSG_QOS_ENABLE;
  nlh.nlmsg_seq = ++hallink_cmd.seq;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, &nlh, NULL, NULL);
  if (ret < 0)
    return ret;
  
  return HAL_SUCCESS;

}

int
hal_qos_port_enable (u_int32_t ifindex)
{
  int ret;

  struct hal_nlmsghdr *nlh;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_port port;
  }req;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_port));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_PORT_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  req.port.ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS; 
} 
  
int 
hal_qos_port_disable (u_int32_t ifindex)
{ 
  int ret;
  struct hal_nlmsghdr *nlh;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_port port;
  } req;
 /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_port));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_PORT_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  req.port.ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


int
hal_qos_enable(char *q0, char *q1, char *q2, char *q3, char *q4, char *q5, char *q6, char *q7)
{
  /* Make sure that flow control send/receive is off */
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_enable *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_enable msg;
  } req;
  int ret, i;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_enable));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  for ( i = 0 ; i < 2 ; i ++)
  {
    msg->q0[i] = *q0++;
    msg->q1[i] = *q1++;
    msg->q2[i] = *q2++;
    msg->q3[i] = *q3++;
    msg->q4[i] = *q4++;
    msg->q5[i] = *q5++;
    msg->q6[i] = *q6++;
    msg->q7[i] = *q7++;
  }

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

/*
 * Disable QoS globlly
 */
int
hal_qos_disable()
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_disable *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_disable msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_disable));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
 
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_wrr_queue_limit_send_messgage (int ifindex, int *ratio)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_wrr_queue_limit *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_wrr_queue_limit msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_wrr_queue_limit));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_WRR_QUEUE_LIMIT;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  memcpy (&msg->ratio[0], ratio, 8 * sizeof (int));

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_wrr_queue_limit(int ifindex, int *ratio)
{
#if 0
  return hal_qos_wrr_queue_limit_send_messgage (ifindex, ratio);
#endif
  return 0;
}

int
hal_qos_no_wrr_queue_limit(int ifindex, int *ratio)
{
#if 0
  return hal_qos_wrr_queue_limit_send_messgage (ifindex, ratio);
#endif
  return 0;
}

int
hal_qos_wrr_tail_drop_threshold(int ifindex, int qid, int thres1, int thres2)
{
#if 0
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_wrr_tail_drop_threshold *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_wrr_tail_drop_threshold msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_wrr_tail_drop_threshold));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_WRR_TAIL_DROP_THRESHOLD;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->qid = qid;
  msg->thres[0] = thres1;
  msg->thres[1] = thres2;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
#endif

  return HAL_SUCCESS;
}

int
hal_qos_wrr_threshold_dscp_map (u_int32_t ifindex, int thid, u_char *dscp, int num)
{
#if 0
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_wrr_threshold_dscp_map *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_wrr_threshold_dscp_map msg;
  } req;
  int ret;
  int i;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_wrr_threshold_dscp_map));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_WRR_THRESHOLD_DSCP_MAP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->thid = thid;
  msg->num = num;
  if (num != 0)
  {
    for (i=0; i<num; i++)
    {
      memcpy ((char *) &msg->dscp[i], (char *) dscp, 1);
      dscp ++;
    }
  }

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
#endif

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
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_wrr_set_bandwidth *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_wrr_set_bandwidth msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_wrr_set_bandwidth));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_WRR_SET_BANDWIDTH;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  memcpy (&msg->bw[0], bw, 8);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_wrr_set_bandwidth_of_queue(int ifindex, int *bw)
{
#if 0
  /* bcmx_cosq_port_bandwidth_set() */
  return hal_qos_wrr_set_bandwidth_send_message (ifindex, bw);
#endif

  return 0;
}


int
hal_qos_wrr_queue_cos_map_send_message (int ifindex, int qid, int cos)
{
  return HAL_SUCCESS;
}

int
hal_qos_wrr_queue_cos_map_set (u_int32_t ifindex, int qid, u_char cos[], int count)
{
#if 0
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_wrr_queue_cos_map_set *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_wrr_queue_cos_map_set msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_wrr_queue_cos_map_set));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_WRR_QUEUE_COS_MAP_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->qid = qid;
  msg->cos_count = count;
  pal_mem_cpy (msg->cos, cos, count * sizeof (u_char));

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
#endif

  return 0;
}

int
hal_qos_wrr_queue_cos_map_unset (u_int32_t ifindex, u_char cos[], int count)
{
#if 0
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_wrr_queue_cos_map_unset *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_wrr_queue_cos_map_unset msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_wrr_queue_cos_map_unset));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_WRR_QUEUE_COS_MAP_UNSET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->cos_count = count;
  pal_mem_cpy (msg->cos, cos, count * sizeof (u_char));

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
#endif

  return 0;
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
#if 0
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_trust_state *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_trust_state msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_trust_state));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_TRUST_STATE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  /*
   * The trust_states are as follows;
   * NSM_QOS_TRUST_NONE
   * NSM_QOS_TRUST_COS
   * NSM_QOS_TRUST_DSCP
   * NSM_QOS_TRUST_IP_PREC
   * NSM_QOS_TRUST_COS_PT_DSCP
   * NSM_QOS_TRUST_DSCP_PT_COS
   */
  /*
   * Use cos keyword settting if the network is composed of Ethernet LANs.
   * Use dscp or ip-precedence keyword if network is not
   * composed of only Ethernet LANs.
   */
  msg->trust_state = trust_state;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
#endif

  return HAL_SUCCESS;
}


int
hal_qos_set_default_cos_for_port(int ifindex, int cos_value)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_default_cos *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_default_cos msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_default_cos));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_DEFAULT_COS;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->cos_value = cos_value;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}



int
hal_qos_set_dscp_mapping_for_port (int ifindex, int flag, 
                                   struct hal_dscp_map_table *map_table,
                                   int count)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_dscp_map_tbl *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_dscp_map_tbl msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_dscp_map_tbl));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_DSCP_MAP_TBL;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->flag = flag;
  pal_mem_cpy ( (char *)&msg->map_table, map_table, 
               count * sizeof (struct hal_dscp_map_table));
  msg->map_count = count;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


int
hal_qos_set_class_map (struct hal_class_map *hal_cmap,
                       int ifindex,
                       int action,
                       int dir)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_class_map *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_class_map msg;
  } req;
  int ret;


  if ((action < HAL_QOS_ACTION_ATTACH) || (action > HAL_QOS_ACTION_DETACH))
    return -1;

  if ((dir < HAL_QOS_DIRECTION_INGRESS) || (dir > HAL_QOS_DIRECTION_EGRESS))
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_class_map));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_CLASS_MAP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->action = action;
  if (action == HAL_QOS_ACTION_ATTACH)
    msg->flag = HAL_QOS_POLICY_MAP_START;
  else
    msg->flag = 0;
  msg->dir = dir;

  /* Copy class-map data */
  memcpy (&msg->cmap, hal_cmap, sizeof (struct hal_class_map));

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_set_cmap_cos_inner (struct hal_class_map *hal_cmap, int ifindex,
                            int action)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_class_map *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_class_map msg;
  } req;
  int ret;

  if ((action < HAL_QOS_ACTION_ATTACH) || (action > HAL_QOS_ACTION_DETACH))
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_class_map));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_CMAP_COS_INNER;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->action = action;
  msg->flag = 0;

  /* Copy class-map data */
  memcpy (&msg->cmap, hal_cmap, sizeof (struct hal_class_map));

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_send_policy_map_end(int ifindex,
                            int action,
                            int dir)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_policy_map *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_policy_map msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_policy_map));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_POLICY_MAP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->action = HAL_QOS_ACTION_ATTACH;      /* attach(1)/detach(2) */
  msg->start_end = HAL_QOS_POLICY_MAP_END;  /* start(1)/end(2) */
  msg->dir = dir;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_send_policy_map_detach(int ifindex,
                               int action,
                               int dir)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_policy_map *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_policy_map msg;
  } req;
  int ret;


  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_policy_map));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_POLICY_MAP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->action = HAL_QOS_ACTION_DETACH;  /* attach(1)/detach(2) */
  msg->start_end = 0;                   /* start(1)/end(2) */
  msg->dir = dir;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_set_policy_map(int ifindex,
                       int action,
                       int dir)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_policy_map *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_policy_map msg;
  } req;
  int ret;


  if ((action < HAL_QOS_ACTION_ATTACH) || (action > HAL_QOS_ACTION_DETACH))
    return -1;

  if ((dir < HAL_QOS_DIRECTION_INGRESS) || (dir > HAL_QOS_DIRECTION_EGRESS))
    return -1;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_policy_map));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_POLICY_MAP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->action = action;                         /* attach(1)/detach(2) */
  if (action == HAL_QOS_ACTION_ATTACH)
    msg->start_end = HAL_QOS_POLICY_MAP_START;  /* start(1)/end(2) */
  else
    msg->start_end = 0;
  msg->dir = dir;

  /* Start policy-map */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (ret < 0)
  {
    if (msg->action == HAL_QOS_ACTION_DETACH)
      return ret;

    /* In attach case, Clear the policy-map */
    hal_qos_send_policy_map_detach(ifindex, action, dir);
    return ret;
  }

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
hal_qos_cos_to_queue (u_int8_t cos, u_int8_t queue_id)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_cos_to_queue *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_cos_to_queue msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_cos_to_queue));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_COS_TO_QUEUE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->cos = cos;
  msg->queue_id = queue_id;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_dscp_to_queue (u_int8_t dscp, u_int8_t queue_id)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_dscp_to_queue *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_dscp_to_queue msg;
  } req;
  int ret; 
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_dscp_to_queue));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_DSCP_TO_QUEUE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->dscp = dscp;
  msg->queue_id = queue_id;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_set_trust_state (u_int32_t ifindex, enum hal_qos_trust_state trust_state)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_trust_state *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_trust_state msg;
  } req;
  int ret;
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_trust_state)); 
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_TRUST_STATE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->trust_state = trust_state;
  msg->ifindex = ifindex;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
    
  return HAL_SUCCESS;
}

int
hal_qos_set_force_trust_cos (u_int32_t ifindex, u_int8_t force_trust_cos)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_force_trust_cos *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_force_trust_cos msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_force_trust_cos));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_FORCE_TRUST_COS;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->force_trust_cos = force_trust_cos;
  msg->ifindex = ifindex;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
    
  return HAL_SUCCESS;
} 

int
hal_qos_set_egress_rate_shape (u_int32_t ifindex, u_int32_t egress_rate,
                               enum hal_qos_rate_unit rate_unit)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_egress_rate *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_egress_rate msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_egress_rate));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_EGRESS_RATE_SHAPE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->rate_unit = rate_unit;
  msg->egress_rate = egress_rate;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
  
  return HAL_SUCCESS;
}
  
int
hal_qos_set_strict_queue (u_int32_t ifindex, u_int8_t strict_queue)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_strict_queue *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_strict_queue msg;
  } req;
  int ret;
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_strict_queue));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_STRICT_QUEUE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->strict_queue = strict_queue;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_set_frame_type_priority_override (enum hal_qos_frame_type frame_type,
                                          u_int8_t queue_id)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_frame_type_priority_override *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_frame_type_priority_override msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_frame_type_priority_override));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_FRAME_TYPE_PRIORITY_OVERRIDE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->frame_type = frame_type;
  msg->queue_id = queue_id;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_unset_frame_type_priority_override (enum hal_qos_frame_type frame_type)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_frame_type_priority_override *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_frame_type_priority_override msg;
  } req;
  int ret;
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_frame_type_priority_override));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_UNSET_FRAME_TYPE_PRIORITY_OVERRIDE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->frame_type = frame_type;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
    
  return HAL_SUCCESS;
} 

int
hal_qos_set_vlan_priority (u_int16_t vid, u_int8_t priority)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_vlan_priority *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_vlan_priority msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_vlan_priority));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_VLAN_PRIORITY;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->vid = vid;
  msg->priority = priority;
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
    
  return HAL_SUCCESS;
} 

int
hal_qos_unset_vlan_priority (u_int16_t vid)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_unset_vlan_priority *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_unset_vlan_priority msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_unset_vlan_priority));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_UNSET_VLAN_PRIORITY;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->vid = vid;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;
    
  return HAL_SUCCESS;
} 
  
int
hal_qos_set_port_vlan_priority_override (u_int32_t ifindex,
                                         enum hal_qos_vlan_pri_override port_mode) 
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_port_vlan_priority_override *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_port_vlan_priority_override msg;
  } req;
  int ret;
  
  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_port_vlan_priority_override));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_PORT_VLAN_PRIORITY_OVERRIDE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->port_mode = port_mode;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_qos_set_port_da_priority_override (u_int32_t ifindex,
                                       enum hal_qos_da_pri_override port_mode)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_port_da_priority_override *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_port_da_priority_override msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_port_da_priority_override));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_PORT_DA_PRIORITY_OVERRIDE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->ifindex = ifindex;
  msg->port_mode = port_mode;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_mls_qos_set_queue_weight (int *queue_weight)
{
  int i;
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_queue_weight *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_qos_set_queue_weight msg;
  } req;
  int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_queue_weight));  
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_QOS_SET_QUEUE_WEIGHT;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;

  
  for (i = 0; i < HAL_QOS_MAX_QUEUE_SIZE; i++)
    msg->weight [i] = queue_weight [i];
  
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

static int
_hal_l2_qos_get_num_cosq (struct hal_nlmsghdr *h, void *data)
{
  unsigned int *num = (unsigned int *)data;
  unsigned int *resp;

  resp = HAL_NLMSG_DATA (h);

  *num = *resp;

  return 0;
}

int
hal_l2_qos_get_num_coq (s_int32_t *num_cosq)
{
  struct hal_nlmsghdr *nlh;
  int ret;
  struct
  {
    struct hal_nlmsghdr nlh;
  } req;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_nlmsghdr));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = HAL_MSG_L2_QOS_NUM_COSQ_GET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Send message and parse response */
  ret = hal_talk (&hallink_cmd, nlh, _hal_l2_qos_get_num_cosq, (void *)num_cosq);

  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

int
hal_l2_qos_set_num_coq (s_int32_t num_cosq)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_qos_set_default_cos *msg;
  struct
   {
     struct hal_nlmsghdr nlh;
     struct hal_msg_qos_set_default_cos msg;
   } req;
   int ret;

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_qos_set_default_cos));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_L2_QOS_NUM_COSQ_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->cos_value = num_cosq;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

#endif /* HAVE_QOS */
