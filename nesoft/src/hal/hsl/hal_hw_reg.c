/* Copyright (C) 2004  All Rights Reserved. */
#include "pal.h"
#include "lib.h"

#include "hsl/hal_netlink.h"
#include "hal/hsl/hal_msg.h"
#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal/hsl/hal_comm.h"

#include "hal/hal_hw_reg.h"

int
hal_hw_reg_parse (struct hal_nlmsghdr *h, void *data)
{
  struct hal_reg_addr *reg, *resp;

  reg  = (struct hal_reg_addr *)data;

  if (h->nlmsg_type != HAL_MSG_REG_GET)
    return -1;

  resp = HAL_NLMSG_DATA(h);
  
  reg->value = resp->value;

  return 0;
}

int
hal_hw_reg_get (u_int32_t reg_addr, struct hal_reg_addr *reg)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_reg_addr *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_reg_addr msg;
  } req;
  struct hal_reg_addr sec_msg;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_reg_addr));
  pal_mem_set (&sec_msg, 0, sizeof(struct hal_reg_addr));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_reg_addr));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_REG_GET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  req.msg.reg_addr = reg_addr;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, hal_hw_reg_parse, &sec_msg);
  
  if (ret < 0)
    return ret;
  
  reg->value = sec_msg.value;

  return HAL_SUCCESS;
}

int
hal_hw_reg_set (u_int32_t reg_addr, u_int32_t value)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_reg_addr *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_reg_addr msg;
  } req;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_reg_addr));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (struct hal_reg_addr));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type = HAL_MSG_REG_SET;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  msg->reg_addr = reg_addr;
  msg->value = value;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}
