/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#ifdef HAVE_DCB
#include "hal_dcb.h"

s_int32_t
hal_dcb_init (char *bridge_name)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_INIT;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_deinit (char *bridge_name)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_DEINIT;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_bridge_enable (char *bridge_name)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_bridge_disable (char *bridge_name)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_bridge_enable (char *bridge_name)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_bridge_disable (char *bridge_name)
{  
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_pfc_bridge_enable (char *bridge_name)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_pfc_bridge_disable (char *bridge_name)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_bridge *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_bridge msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_bridge));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_bridge));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_PFC_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_interface_enable (char *bridge_name, s_int32_t ifindex)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_if *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_if msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_if));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_if));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_IF_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_interface_disable (char *bridge_name, s_int32_t ifindex)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_if *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_if msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_if));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_if));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_IF_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_interface_enable (char *bridge_name, s_int32_t ifindex)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_if *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_if msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_if));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_if));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_IF_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_interface_disable (char *bridge_name, s_int32_t ifindex)
{  
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_if *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_if msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_if));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_if));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_IF_DISABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_pfc_interface_enable (char *bridge_name, s_int32_t ifindex)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_if *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_if msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_if));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_if));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_PFC_IF_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_enable_pfc_priority (char *bridge_name, s_int32_t ifindex, s_int8_t pri)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_pfc_pri *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_pfc_pri msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_pfc_pri));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_pfc_pri));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_PFC_IF_ENABLE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  pal_strncpy (&msg->pfc_enable_vector[0], &pri,
               (sizeof (u_int8_t) * (HAL_DCB_NUM_USER_PRIORITY)));
  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_set_pfc_cap (char *bridge_name, s_int32_t ifindex, u_int8_t cap)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_pfc_cap *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_pfc_cap msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_pfc_cap));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_pfc_cap));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_PFC_SET_CAP;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->pfc_cap = cap;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_set_pfc_lda (char *bridge_name, s_int32_t ifindex, u_int32_t lda)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_pfc_lda *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_pfc_lda msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_pfc_lda));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_pfc_lda));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_PFC_SET_LDA;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->pfc_lda = lda;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_select_pfc_mode (char *bridge_name, s_int32_t ifindex,
                         enum hal_dcb_mode mode)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_pfc_mode *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_pfc_mode msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_pfc_mode));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_pfc_mode));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_PFC_SELECT_MODE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->mode = mode;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_select_ets_mode (char *bridge_name, s_int32_t ifindex, 
                         enum hal_dcb_mode mode)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_mode *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_mode msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_mode));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_mode));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_SELECT_MODE;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->mode = mode;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_add_pri_to_tcg (char *bridge_name, s_int32_t ifindex, 
                            u_int8_t tcgid, u_int8_t pri)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_ets_tcg_pri *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_ets_tcg_pri msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_ets_tcg_pri));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_ets_tcg_pri));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_ADD_PRI_TO_TCG;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->tcgid = tcgid;
  msg->priority_list = pri;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_remove_pri_from_tcg (char *bridge_name, s_int32_t ifindex, 
                                 u_int8_t tcgid, u_int8_t pri)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_ets_tcg_pri *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_ets_tcg_pri msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_ets_tcg_pri));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_ets_tcg_pri));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_REMOVE_PRI_FROM_TCG;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->tcgid = tcgid;
  msg->priority_list = pri;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;


  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_assign_bw_to_tcgs (char *bridge_name, s_int32_t ifindex, 
                               u_int16_t *bw)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_ets_tcg_bw *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_ets_tcg_bw msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_ets_tcg_bw));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_ets_tcg_bw));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_TCG_BW_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  pal_strncpy (&msg->bandwidth[0], bw, 
               (sizeof (u_int16_t) * HAL_DCB_NUM_DEFAULT_TCGS));

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;


  return HAL_SUCCESS;
}

s_int32_t
hal_dcb_ets_set_application_priority (char *bridge_name, s_int32_t ifindex, 
                                      u_int8_t sel, u_int16_t proto_id, 
                                      u_int8_t pri)
{ 
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_ets_appl_pri *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_ets_appl_pri msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_ets_appl_pri));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_ets_appl_pri));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_APP_PRI_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->selector = sel;
  msg->protocol_value = proto_id;
  msg->priority = pri;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


s_int32_t
hal_dcb_ets_unset_application_priority (char *bridge_name, s_int32_t ifindex, 
                                        u_int8_t sel, u_int16_t proto_id, 
                                        u_int8_t pri)
{
  struct hal_nlmsghdr *nlh;
  struct hal_msg_dcb_ets_appl_pri *msg;
  struct 
  {
    struct hal_nlmsghdr nlh;
    struct hal_msg_dcb_ets_appl_pri msg;
  } req;
  s_int32_t ret;

  pal_mem_set (&req.nlh, 0, sizeof (struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof (struct hal_msg_dcb_ets_appl_pri));

  /* Set Header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_msg_dcb_ets_appl_pri));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_DCB_ETS_APP_PRI_UNSET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  pal_strncpy (msg->bridge_name, bridge_name, HAL_BRIDGE_NAME_LEN+1);
  msg->ifindex = ifindex;
  msg->selector = sel;
  msg->protocol_value = proto_id;
  msg->priority = pri;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

 
  return HAL_SUCCESS;
}


#endif /* HAVE_DCB */
