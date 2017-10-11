/* Copyright (C) 2003  All Rights Reserved.

This module declares the interfaces to the BPDU 
parser/formatters and handlers.

*/

#ifndef __PACOS_BR_MSTP_BDPU_H__
#define __PACOS_BR_MSTP_BDPU_H__

struct mstp_bpdu
{
  /* BPDU Header */
  u_int16_t      proto_id;
  u_char        version;
  enum bpdu_type type;

  /* Message Priority Vector */
  struct bridge_id  cist_reg_root;
  struct bridge_id  cist_root;
  u_int32_t         external_rpc;
  u_int32_t         internal_rpc;
  u_int16_t         cist_port_id;

  /* Message times */
  s_int32_t         message_age;
  s_int32_t         max_age;
  s_int32_t         hello_time;
  s_int32_t         fwd_delay;
 
  u_char            cist_port_role;
  /* Flags */ 
  u_char            cist_topology_change:1;
  u_char            cist_proposal:1;
  u_char            cist_learning:1;
  u_char            cist_forwarding:1;
  u_char            cist_agreement:1;
  u_char            cist_topology_change_ack:1;

  u_char        version_one_len;
  u_int16_t      version_three_len;
  struct mstp_config_info rcv_cfg;
  struct bridge_id cist_bridge_id; /* cist br id of tx bridge */
  u_char hop_count;

  char valid_type;
  s_int32_t cisco_bpdu;
  struct mstp_instance_bpdu *instance_list;

  u_char *buf;
  u_int32_t len;
};

struct mstp_instance_bpdu 
{
  /* maintenance */
  struct mstp_instance_bpdu *next;
  struct mstp_bpdu *bpdu;
  /*proto */
  u_char        msti_topology_change:1;
  u_char        msti_proposal:1;
  u_char        msti_learning:1;
  u_char        msti_agreement:1;
  u_char        msti_forwarding:1;
  u_char        msti_master:1;

  u_char        msti_port_role;

  u_char        msti_bridgeid_prio;
  u_char        msti_portid_prio;
  u_char        rem_hops;
  u_int32_t     msti_internal_rpc;
  s_int32_t     instance_id;
  struct bridge_id msti_reg_root;
  struct bridge_id bridge_id; /* calculated while parsing */
  u_int16_t      msti_port_id; /* calculated while parsing */
  
  #ifdef HAVE_RPVST_PLUS
  u_int16_t      vlan_id; /* Used to compare with received TLV */
  /* Message times */
  s_int32_t      message_age;
  s_int32_t      max_age;
  s_int32_t      hello_time;
  s_int32_t      fwd_delay;
  u_char         version_one_len;
  u_char         msti_topology_change_ack:1;
#endif /* HAVE_RPVST_PLUS */

};


void
mstp_handle_bpdu (struct mstp_port *port, u_char *buf, int len);

void
mstp_check_topology_change (struct mstp_port *port, struct mstp_bpdu *bpdu);

inline s_int32_t
mstp_parse_timer (u_char *dest);

inline void
mstp_msti_set_tc (struct mstp_instance_port *inst_port);

inline void
mstp_cist_set_tc (struct mstp_port *port);

#ifdef HAVE_L2GP
inline void
l2gp_bld_msti_info (struct mstp_instance_port *inst_port,
                    u_char **buf, struct mstp_port * );

int
l2gp_bld_cist_info  (struct mstp_port *port, u_char **buf);

bool_t
better_or_same_info_cist_rcvd (struct mstp_port *port, struct mstp_bpdu *bpdu);

int
mstp_l2gp_prep_and_send_pseudoinfo (struct mstp_port *port);

int  mstp_l2gp_chk_bpdu_consistency (struct mstp_port *port,
                                     struct mstp_bpdu *bpdu );


#endif /* HAVE_L2GP */

#endif /* __PACOS_BR_MSTP_BDPU_H__ */
