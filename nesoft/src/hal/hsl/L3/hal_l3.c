/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_types.h"
#include "hal_incl.h"
#include "hal_error.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_msg.h"
#include "hal_debug.h"

#ifdef HAVE_L3
int hal_l3_init (void);
int hal_l3_deinit (void);
#endif /* HAVE_L3 */

extern struct lib_globals *hal_zg;

/*
  Initialize HAL L3
*/
int
hal_l3_init (void)
{
  int ret;

  ret = hal_ipv4_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL IPv4");
      goto CLEANUP;
    }

#ifdef HAVE_IPV6
  ret = hal_ipv6_init ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed initializing HAL IPv6");
      goto CLEANUP;
    }
#endif /* HAVE_IPV6 */

  /* Create default FIB. */
  ret = hal_fib_create (0);
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed creating default FIB");
      goto CLEANUP;
    }

  return 0;

 CLEANUP:
  hal_l3_deinit ();
  return -1;
}

/* 
   Deinitialize HAL L3 
*/
int
hal_l3_deinit (void)
{
  int ret;

  ret = hal_ipv4_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL IPv4");
    }

#ifdef HAVE_IPV6
  ret = hal_ipv6_deinit ();
  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing HAL IPv6");
    }
#endif /* HAVE_IPV6 */

  /* Deinit L3 mode initialization mode. */
  ret = hal_msg_generic_request (HAL_MSG_IF_L3_DEINIT, NULL, NULL);

  if (ret < 0)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_err (hal_zg, "Failed deinitializing L3 mode ");
    }

  return 0;
}

#ifdef HAVE_VRRP
static int
hal_vrrp_send_msg (struct hal_msg_vrrp_msg *vrrp_msg)
  {
    struct hal_nlmsghdr *nlh;
    struct
    {
      struct hal_nlmsghdr nlh;
      struct hal_msg_vrrp_msg msg;
    } req;
    int ret = -1;

    pal_mem_cpy (&req.msg, vrrp_msg, sizeof (req.msg));

    /* Set header. */
    nlh = &req.nlh;
    nlh->nlmsg_len = HAL_NLMSG_LENGTH (sizeof (req.msg));
    nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
    nlh->nlmsg_type = HAL_MSG_VRRP_UPDATE;
    nlh->nlmsg_seq = ++hallink_cmd.seq;

    /* Send message and parse response */
    ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);
    if (ret < 0)
      return ret;

    return 0;
  }

int
hal_vrrp_virtual_addr_update (u_int8_t vrid, u_int8_t af_type,
                              void *virtual_address,
                              s_int32_t ifindex, char msg_type)
{
  struct hal_msg_vrrp_msg vrrp_msg;

  pal_mem_set (&vrrp_msg, 0, sizeof (struct hal_msg_vrrp_msg));

  vrrp_msg.vrid            = vrid;
  vrrp_msg.ifindex         = ifindex;

  if (af_type == AF_INET)
    {
      vrrp_msg.af_type         = AF_INET;
      vrrp_msg.vip_v4.s_addr   = ((struct hal_in4_addr *)virtual_address)->s_addr;
    }
#ifdef HAVE_IPV6
  else if (af_type == AF_INET6)
    {
      vrrp_msg.af_type         = AF_INET6;
      IPV6_ADDR_COPY (&vrrp_msg.vip_v6, ((struct hal_in6_addr *)virtual_address));
    }
#endif /* HAVE_IPV6 */

  vrrp_msg.msg_type = msg_type;

  return hal_vrrp_send_msg (&vrrp_msg);
}
#endif /* HAVE_VRRP */

