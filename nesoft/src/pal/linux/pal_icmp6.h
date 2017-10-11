/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PAL_ICMP6_H
#define _PAL_ICMP6_H

/*
** ICMPv6 header
*/
#define pal_icmp6_hdr   icmp6_hdr

/*
  ICMPv6 types
*/
#define PAL_ND_ROUTER_SOLICIT           133
#define PAL_ND_ROUTER_ADVERT            134
#define PAL_ND_NEIGHBOR_SOLICIT         135
#define PAL_ND_NEIGHBOR_ADVERT          136
#define PAL_ND_REDIRECT                 137

#define PAL_ICMP6_HADISCOV_REQUEST      150
#define PAL_ICMP6_HADISCOV_REPLY        151
#define PAL_ICMP6_MOBILEPREFIX_SOLICIT  152
#define PAL_ICMP6_MOBILEPREFIX_ADVERT   153

/*
** ICMPv6 RS header
*/
#define pal_nd_router_solicit   nd_router_solicit

/*
** ICMPv6 RA header
*/
#define pal_nd_router_advert    nd_router_advert

/*
** ICMPv6 neighbor discovery option header
*/
#define pal_nd_opt_hdr          nd_opt_hdr

#define  PAL_ND_OPT_SOURCE_LINKADDR       1
#define  PAL_ND_OPT_TARGET_LINKADDR       2
#define  PAL_ND_OPT_PREFIX_INFORMATION    3
#define  PAL_ND_OPT_REDIRECTED_HEADER     4
#define  PAL_ND_OPT_MTU                   5
#define  PAL_ND_OPT_ADVINTERVAL           7
#define  PAL_ND_OPT_HOMEAGENT_INFO        8

/*
** ICMPv6 prefix information
*/
#define pal_nd_opt_prefix_info  nd_opt_prefix_info

#define PAL_ND_OPT_PI_FLAG_ONLINK        0x80
#define PAL_ND_OPT_PI_FLAG_AUTO          0x40
#define PAL_ND_OPT_PI_FLAG_ROUTER        0x20

/*
** ICMPv6 MTU option
*/
#define pal_nd_opt_mtu          nd_opt_mtu

/*
** ICMPv6 mobile IPv6 extension: Advertisement Interval
*/
struct pal_nd_opt_adv_interval {
  u_int8_t nd_opt_adv_interval_type;
  u_int8_t nd_opt_adv_interval_len;
  u_int16_t nd_opt_adv_interval_reserved;
  u_int32_t nd_opt_adv_interval_ival;
};

/*
** ICMPv6 mobile IPv6 extention: Home agent info
*/
struct pal_nd_opt_home_agent_info {
  u_int8_t nd_opt_home_agent_info_type;
  u_int8_t nd_opt_home_agent_info_len;
  u_int16_t nd_opt_home_agent_info_reserved;
  s_int16_t nd_opt_home_agent_info_preference;
  u_int16_t nd_opt_home_agent_info_lifetime;
};
#define PAL_ND_RA_FLAG_MANAGED    0x80
#define PAL_ND_RA_FLAG_OTHER      0x40
#define PAL_ND_RA_FLAG_HOME_AGENT 0x20

struct pal_ha_discov_req {
  struct pal_icmp6_hdr ha_dreq_hdr;
};
#define discov_req_type ha_dreq_hdr.icmp6_type
#define discov_req_code ha_dreq_hdr.icmp6_code
#define discov_req_cksum ha_dreq_hdr.icmp6_cksum
#define discov_req_id ha_dreq_hdr.icmp6_data16[0]

struct pal_ha_discov_rep {
  struct pal_icmp6_hdr ha_drep_hdr;
  u_int32_t ha_drep_reserved1;
  u_int32_t ha_drep_reserved2;
};
#define discov_rep_type ha_drep_hdr.icmp6_type
#define discov_rep_code ha_drep_hdr.icmp6_code
#define discov_rep_cksum ha_drep_hdr.icmp6_cksum
#define discov_rep_id ha_drep_hdr.icmp6_data16[0]

#endif /* _PAL_ICMP6_H */
