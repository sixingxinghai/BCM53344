/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "hal_incl.h"
#include "hal_types.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_stats_api.h"

int
_hal_stats_port_parse (struct hal_nlmsghdr *h, void *data)
{
  struct hal_stats_port *stats;
  struct hal_stats_port *resp;

  stats  = (struct hal_stats_port *)data;

  if (h->nlmsg_type != HAL_MSG_STATS_PORT_GET)
    return -1;

  resp = HAL_NLMSG_DATA(h);

  stats->rxpkts   = resp->rxpkts;
  stats->rxdrops  = resp->rxdrops;
  stats->rxbytes  = resp->rxbytes;
  stats->rxmcac   = resp->rxmcac;
  stats->rxbcac   = resp->rxbcac;
  stats->txpkts   = resp->txpkts;
  stats->txdrops  = resp->txdrops;
  stats->txbytes  = resp->txbytes;
  stats->txmcac   = resp->txmcac;
  stats->txbcac   = resp->txbcac;
  stats->fldpkts  = resp->fldpkts;
  stats->vlandrops= resp->vlandrops;
  stats->stmdrops = resp->stmdrops;
  stats->eddrops  = resp->eddrops;

  return 0;
}

int
_hal_stats_host_parse (struct hal_nlmsghdr *h, void *data)
{
  struct hal_stats_host *stats;
  struct hal_stats_host *resp;

  stats  = (struct hal_stats_host *)data;

  if (h->nlmsg_type != HAL_MSG_STATS_HOST_GET)
    return -1;

  resp = HAL_NLMSG_DATA(h);

  stats->hostInpkts  = resp->hostInpkts;
  stats->hostOutpkts = resp->hostOutpkts;
  stats->hostOuterrs = resp->hostOuterrs;
  stats->learnDrops  = resp->learnDrops;

  return 0;
}

int
_hal_stats_vlan_parse (struct hal_nlmsghdr *h, void *data)
{
  struct hal_stats_vlan *stats;
  struct hal_stats_vlan *resp;

  stats  = (struct hal_stats_vlan *)data;

  if (h->nlmsg_type != HAL_MSG_STATS_VLAN_GET)
    return -1;

  resp = HAL_NLMSG_DATA(h);

  stats->ucastpkts  = resp->ucastpkts;
  stats->ucastbytes = resp->ucastbytes;
  stats->mcastpkts  = resp->mcastpkts;
  stats->mcastbytes = resp->mcastbytes;

  return 0;
}


int
hal_statistics_vlan_get (u_int32_t vlan_id,
                              struct hal_stats_vlan *vlan)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_stats_vlan *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_stats_vlan msg;
  } req;

  struct hal_stats_vlan vlan_msg;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_stats_vlan));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len   = HAL_NLMSG_LENGTH (sizeof (struct hal_stats_vlan));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type  = HAL_MSG_STATS_VLAN_GET;
  nlh->nlmsg_seq   = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  req.msg.vlan_id = vlan_id;
  msg->vlan_id = vlan_id;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_stats_vlan_parse, &vlan_msg);
  if (ret < 0)
    return ret;

  vlan->ucastpkts  = vlan_msg.ucastpkts;
  vlan->ucastbytes = vlan_msg.ucastbytes;
  vlan->mcastpkts  = vlan_msg.mcastpkts;
  vlan->mcastbytes = vlan_msg.mcastbytes;

  return HAL_SUCCESS;
}

int
hal_statistics_port_get (u_int32_t port_id,
                              struct hal_stats_port *port)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_stats_port *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_stats_port msg;
  } req;

  struct hal_stats_port port_msg;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_stats_port ));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len   = HAL_NLMSG_LENGTH (sizeof (struct hal_stats_port));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type  = HAL_MSG_STATS_PORT_GET;
  nlh->nlmsg_seq   = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;
  req.msg.port_id = port_id;


  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_stats_port_parse, &port_msg);
  if (ret < 0)
    return ret;

  port->rxpkts   = port_msg.rxpkts;
  port->rxdrops  = port_msg.rxdrops;
  port->rxbytes  = port_msg.rxbytes;
  port->rxmcac   = port_msg.rxmcac;
  port->rxbcac   = port_msg.rxbcac;
  port->txpkts   = port_msg.txpkts;
  port->txdrops  = port_msg.txdrops;
  port->txbytes  = port_msg.txbytes;
  port->txmcac   = port_msg.txmcac;
  port->txbcac   = port_msg.txbcac;
  port->fldpkts  = port_msg.fldpkts;
  port->vlandrops= port_msg.vlandrops;
  port->stmdrops = port_msg.stmdrops;
  port->eddrops  = port_msg.eddrops;
  
  return HAL_SUCCESS;

}

int
hal_statistics_host_get (struct hal_stats_host *host)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct hal_stats_host *msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_stats_host msg;
  } req;

  struct hal_stats_host host_msg;

  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));
  pal_mem_set (&req.msg, 0, sizeof(struct hal_stats_host));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len   = HAL_NLMSG_LENGTH (sizeof (struct hal_stats_host));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type  = HAL_MSG_STATS_HOST_GET;
  nlh->nlmsg_seq   = ++hallink_cmd.seq;

  /* Set message. */
  msg = &req.msg;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, _hal_stats_host_parse, &host_msg);
  if (ret < 0)
    return ret;

  host->hostInpkts  = host_msg.hostInpkts;
  host->hostOutpkts = host_msg.hostOutpkts;
  host->hostOuterrs = host_msg.hostOuterrs;
  host->learnDrops  = host_msg.learnDrops;

  return HAL_SUCCESS;
}

int
hal_statistics_clear (u_int32_t port_id)
{
  int ret;
  struct hal_nlmsghdr *nlh;
  struct
  {
    struct hal_nlmsghdr nlh;
    struct hal_port msg;
  } req;


  pal_mem_set (&req.nlh, 0, sizeof(struct hal_nlmsghdr));

  /* Set header. */
  nlh = &req.nlh;
  nlh->nlmsg_len   = HAL_NLMSG_LENGTH (sizeof (struct hal_port));
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
  nlh->nlmsg_type  = HAL_MSG_STATS_CLEAR;
  nlh->nlmsg_seq   = ++hallink_cmd.seq;

  /* Set message. */
  req.msg.ifindex = port_id;

  /* Send message and process ack. */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
  if (ret < 0)
    return ret;

  return HAL_SUCCESS;
}


