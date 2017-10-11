/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_PBR
#include "hal_incl.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"
#include "hal_pbr_ipv4_uc.h"
#include "hal_debug.h"


/* Libglobals. */
extern struct lib_globals *hal_zg;

static int
_hal_pbr_ipv4_uc_route (int command, char *rmap_name, s_int32_t seq_num,
                        struct hal_prefix *src_prefix, struct hal_prefix *dst_prefix, 
                        s_int16_t protocol, s_int16_t sport_op,
                        u_int32_t sport_low, u_int32_t sport,
                        s_int16_t dport_op, u_int32_t dport_low,
                        u_int32_t dport, s_int16_t tos_op,
                        u_int8_t tos_low, u_int8_t tos,  
                        enum filter_type filter_type,
                        s_int32_t precedence, struct hal_in4_addr *nexthop,
                        int nh_type, 
                        char *out_ifname, u_int32_t if_index)
{
  int ret = 0;
  struct hal_nlmsghdr *nlh;
  int msgsz;
  u_int32_t size;
  u_char *pnt;
  struct hal_msg_pbr_ipv4_route msg;
  struct
  {
    struct hal_nlmsghdr nlh;
    u_char buf[1024];
  }req;

  pal_mem_set (&msg, 0, sizeof(struct hal_msg_pbr_ipv4_route));
  
  /* map info */
  pal_strcpy (msg.rmap_name, rmap_name);
  msg.seq_num = seq_num;

  /* src and dst prefixes sent to the hsl */
  prefix_copy ((struct prefix *)&msg.src_prefix, (struct prefix *)src_prefix);
  prefix_copy ((struct prefix *)&msg.dst_prefix, (struct prefix *)dst_prefix);

  /* protocol set */
  if (protocol == TCP_PROTO || protocol == UDP_PROTO )
    {
      msg.protocol = protocol;
      SET_CINDEX (msg.cindex, HAL_MSG_CINDEX_PBR_PROTOCOL);
    }
  /* source port set */
  if (sport)
    {
      msg.sport = sport;
      msg.sport_op = sport_op;
      msg.sport_low = sport_low;
      SET_CINDEX (msg.cindex, HAL_MSG_CINDEX_PBR_SRC_PORT);
    }

  /* destination port set */
  if (dport)
    {
      msg.dport = dport;
      msg.dport_op = dport_op;
      msg.dport_low = dport_low; 
      SET_CINDEX (msg.cindex, HAL_MSG_CINDEX_PBR_DST_PORT);
    }

  /* tos set */
  if (tos_op == NOOP)
    {
      msg.tos = tos;
      msg.tos_op = tos_op;
      msg.tos_low = tos_low;
      SET_CINDEX (msg.cindex, HAL_MSG_CINDEX_PBR_TOS);
    }

  if (precedence > -1)
    {
      msg.pcp = precedence;
      SET_CINDEX (msg.cindex, HAL_MSG_CINDEX_PBR_PCP);
    }
  msg.rule_type = filter_type;
  msg.if_id = if_index;

  /* Copy nexthop */
  pal_mem_cpy (&msg.nexthop, nexthop, sizeof (struct pal_in4_addr));
  msg.nh_type = nh_type;

  /* Copy out going interface */
  pal_strcpy (msg.ifname, out_ifname);
 
  /* Set message */
  pnt = (u_char *)req.buf;
  size = sizeof (req.buf);
  msgsz = hal_msg_encode_pbr_ipv4_route (&pnt, &size, &msg);
  if (msgsz < 0)
    return -1;

  /* Set header */
  nlh = &req.nlh;
  nlh->nlmsg_len = HAL_NLMSG_LENGTH (msgsz);
  nlh->nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST | HAL_NLM_F_ACK;
  nlh->nlmsg_type = command;
  nlh->nlmsg_seq = ++hallink_cmd.seq;

  /* Now PBR related changes are not present in hsl. Once those 
     changes are done have to uncomment the below code */
#if 0
  /* Send message and process acknowledgement */
  ret = hal_talk (&hallink_cmd, nlh, NULL, NULL);

  if (IS_HAL_DEBUG_EVENT)
    zlog_info (hal_zg, "_hal_pbr_ipv4_uc_route: sent PBR rule to hsl\n");

  if (ret < 0)
    return ret;
#endif 
  return ret;

}
/*
   Name: hal_pbr_ipv4_uc_route

   Description:
   Common function for PBR route.

   Parameters:

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_pbr_ipv4_uc_route (int op, char *rmap_name, s_int32_t seq_num,
                       struct hal_prefix *src_prefix,
                       struct hal_prefix *dst_prefix, s_int16_t protocol,
                       int sport_op, u_int32_t sport_low, u_int32_t sport,
                       int dport_op, u_int32_t dport_low, u_int32_t dport,
                       int tos_op, u_int8_t tos_low, u_int8_t tos,
                       enum filter_type filter_type,
                       s_int32_t precedence,
                       struct hal_in4_addr *nexthop,
                       s_int16_t nh_type,
                       char *ifname,
                       s_int32_t if_index)
{
  if (IS_HAL_DEBUG_EVENT) 
    zlog_info (hal_zg, "hal received pbr message parameters are\n"
              " op %d \n rmap name %s\n seq num %d\n protocol %d\n"
              "src_prefixlen %d\t src_addr %r\n dst_prefixlen %d\t dst_addr %r\n"
              "sport_op %d\t sport_low %d\t sport %d\n dport_op %d\t dport_low %d\t "
	      "dport %d\n tos_op %d\t tos_low %d\t tos %d\n filter_type %d\n"
	      "precedence %d\n nexthop %r\t nh_type %d\n ifname %s\n if_index %d\n",
              op, rmap_name, seq_num, protocol, src_prefix->prefixlen,
              src_prefix->u.prefix4, dst_prefix->prefixlen, dst_prefix->u.prefix4,
              sport_op, sport_low, sport, dport_op, dport_low, dport, tos_op,
              tos_low, tos, filter_type, precedence, nexthop, nh_type,
              ifname, if_index);
 
  if (op == HAL_RMAP_POLICY_ADD)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_info (hal_zg, "hal_pbr_ipv4_uc_route: hal message received for "
                            "pbr rule add\n");
      return _hal_pbr_ipv4_uc_route (HAL_MSG_PBR_IPV4_UC_ADD, 
                                   rmap_name, seq_num,
                                   src_prefix, dst_prefix,
                                   protocol, sport_op, sport_low, sport,
                                   dport_op, dport_low, dport,
                                   tos_op, tos_low, tos,
                                   filter_type, precedence, nexthop, 
                                   nh_type, ifname,
                                   if_index);
    
    }
  if (op == HAL_RMAP_POLICY_DELETE)
    {
      if (IS_HAL_DEBUG_EVENT)
        zlog_info (hal_zg, "hal_pbr_ipv4_uc_route: hal message received for "
                            "pbr rule deletion\n");

      return _hal_pbr_ipv4_uc_route (HAL_MSG_PBR_IPV4_UC_DELETE,
                                   rmap_name, seq_num,
                                   src_prefix, dst_prefix,
                                   protocol, sport_op, sport_low, sport,
                                   dport_op, dport_low, dport,
                                   tos_op, tos_low, tos,
                                   filter_type, precedence, nexthop,
                                   nh_type, ifname,
                                   if_index);
    }
  return 1;
}
#endif /* HAVE_PBR */
