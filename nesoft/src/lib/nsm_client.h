/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_CLIENT_H
#define _NSM_CLIENT_H

#include "message.h"
#include "nsm_message.h"
#ifdef HAVE_MPLS_OAM
#include "oam_mpls_msg.h"
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */

struct message_entry;

/* Structure to hold pending message. */
struct nsm_client_pend_msg
{
  /* NSM Msg Header. */
  struct nsm_msg_header header;

  /* Message. */
  u_char buf[NSM_MESSAGE_MAX_LEN];
};

/* NSM client to server connection structure. */
struct nsm_client_handler
{
  /* Parent. */
  struct nsm_client *nc;

  /* Up or down.  */
  int up;

  /* Type of client, sync or async.  */
  int type;

  /* Message client structure. */
  struct message_handler *mc;

  /* Service bits specific for this connection.  */
  struct nsm_msg_service service;

  /* Message buffer for output. */
  u_char buf[NSM_MESSAGE_MAX_LEN];
  u_int16_t len;
  u_char *pnt;
  u_int16_t size;

  /* Message buffer for input. */
  u_char buf_in[NSM_MESSAGE_MAX_LEN];
  u_int16_t len_in;
  u_char *pnt_in;
  u_int16_t size_in;

  /* Message buffer for IPv4 route updates.  */
  u_char buf_ipv4[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv4;
  u_int16_t len_ipv4;
  struct thread *t_ipv4;
  u_int32_t vr_id_ipv4;
  vrf_id_t vrf_id_ipv4;

#ifdef HAVE_IPV6
  /* Message buffer for IPv6 route updates.  */
  u_char buf_ipv6[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv6;
  u_int16_t len_ipv6;
  struct thread *t_ipv6;
  u_int32_t vr_id_ipv6;
  vrf_id_t vrf_id_ipv6;
#endif /* HAVE_IPV6 */

  /* Client message ID.  */
  u_int32_t message_id;

  /* Message ID for ILM/FTN entries. */
  u_int32_t mpls_msg_id;

  /* List of pending messages of struct nsm_client_pend_msg. */
  struct list pend_msg_list;

  /* Send and recieved message count.  */
  u_int32_t send_msg_count;
  u_int32_t recv_msg_count;
};

/* NSM client structure.  */
struct nsm_client
{
  struct lib_globals *zg;

  /* Service bits. */
  struct nsm_msg_service service;

  /* NSM client ID. */
  u_int32_t client_id;

  /* Parser functions. */
  NSM_PARSER parser[NSM_MSG_MAX];

  /* Callback functions. */
  NSM_CALLBACK callback[NSM_MSG_MAX];

  /* Async connection. */
  struct nsm_client_handler *async;
 
  /* NSM shutdown message*/
  u_char nsm_server_flags;
  #define NSM_SERVER_SHUTDOWN  (1<<0)

  /* Disconnect callback. */
  NSM_DISCONNECT_CALLBACK disconnect_callback;

  /* Reconnect thread. */
  struct thread *t_connect;

  /* Reconnect interval in seconds. */
  int reconnect_interval;

  /* Debug message flag. */
  int debug;

  /* COMMMSG recv callback. */
  nsm_msg_commsg_recv_cb_t  nc_commsg_recv_cb;
  void                     *nc_commsg_user_ref;
};

#define NSM_CLIENT_BUNDLE_TIME         1

#ifndef NSM_SERV_PATH
#define NSM_SERV_PATH "/tmp/.nsmserv"
#endif /* NSM_SERV_PATH */

struct nsm_client *nsm_client_create (struct lib_globals *, u_int32_t);
int nsm_client_delete (struct nsm_client *);
void nsm_client_stop (struct nsm_client *);
void nsm_client_id_set (struct nsm_client *, struct nsm_msg_service *);
int nsm_client_set_service (struct nsm_client *, int);
void nsm_client_set_version (struct nsm_client *, u_int16_t);
void nsm_client_set_protocol (struct nsm_client *, u_int32_t);
void nsm_client_set_callback (struct nsm_client *, int, NSM_CALLBACK);
void nsm_client_set_disconnect_callback (struct nsm_client *,
                                         NSM_DISCONNECT_CALLBACK);
int nsm_client_send_message (struct nsm_client_handler *, u_int32_t, vrf_id_t,
                             int, u_int16_t, u_int32_t *);
void nsm_client_pending_message (struct nsm_client_handler *,
                                 struct nsm_msg_header *);
int nsm_client_reconnect (struct thread *);
int nsm_client_reconnect_start (struct nsm_client *);
int nsm_client_reconnect_async (struct thread *);
int nsm_client_start (struct nsm_client *);

int nsm_client_send_redistribute_set (u_int32_t, vrf_id_t, struct nsm_client *,
                                      struct nsm_msg_redistribute *);
int nsm_client_send_redistribute_unset (u_int32_t, vrf_id_t,
                                        struct nsm_client *,
                                        struct nsm_msg_redistribute *);

int nsm_client_send_wait_for_bgp_set (u_int32_t, vrf_id_t,
                                      struct nsm_client *,
                                      struct nsm_msg_wait_for_bgp *);

int nsm_client_send_bgp_conv_done (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_client *,
                                   struct nsm_msg_wait_for_bgp *);

int nsm_client_send_ldp_session_state (u_int32_t, vrf_id_t,
                                       struct nsm_client *,
                                       struct nsm_msg_ldp_session_state *);

int nsm_client_send_ldp_session_query (u_int32_t vr_id, vrf_id_t vrf_id,
                                       struct nsm_client *,
                                       struct nsm_msg_ldp_session_state *);

int nsm_client_send_route_ipv4 (u_int32_t, vrf_id_t, struct nsm_client *,
                                struct nsm_msg_route_ipv4 *);
int nsm_client_flush_route_ipv4 (struct nsm_client *);
int nsm_client_send_route_ipv4_bundle (u_int32_t, vrf_id_t,
                                       struct nsm_client *,
                                       struct nsm_msg_route_ipv4 *);

int nsm_client_read_sync (struct message_handler *, struct message_entry *,
                          pal_sock_handle_t, struct nsm_msg_header *, int *);
int
nsm_client_send_nsm_if_del_done(struct nsm_client *,
                                struct nsm_msg_link *,
                                u_int32_t *);
#ifdef HAVE_IPV6
int nsm_client_send_route_ipv6 (u_int32_t, vrf_id_t, struct nsm_client *,
                                struct nsm_msg_route_ipv6 *);
int nsm_client_flush_route_ipv6 (struct nsm_client *);
int nsm_client_send_route_ipv6_bundle (u_int32_t, vrf_id_t,
                                       struct nsm_client *,
                                       struct nsm_msg_route_ipv6 *);
#endif /* HAVE_IPV6 */

int nsm_client_route_lookup_ipv4 (u_int32_t, vrf_id_t, struct nsm_client *,
                                  struct nsm_msg_route_lookup_ipv4 *, int,
                                  struct nsm_msg_route_ipv4 *);
#ifdef HAVE_IPV6
int nsm_client_route_lookup_ipv6 (u_int32_t, vrf_id_t, struct nsm_client *,
                                  struct nsm_msg_route_lookup_ipv6 *, int,
                                  struct nsm_msg_route_ipv6 *);
#endif /* HAVE_IPV6 */

struct interface *nsm_util_interface_state (struct nsm_msg_link *,
                                            struct if_master *);
cindex_t nsm_util_link_val_set (struct lib_globals *,
                                struct nsm_msg_link *,
                                struct interface *);
int nsm_client_send_interface_address (struct nsm_client *,
                                       struct nsm_msg_address *, int);

#ifdef HAVE_MPLS
int nsm_client_send_label_request (struct nsm_client *,
                                   struct nsm_msg_label_pool *,
                                   struct nsm_msg_label_pool *);

int nsm_client_send_label_release (struct nsm_client *,
                                   struct nsm_msg_label_pool *);
#ifdef HAVE_PACKET
int nsm_client_ilm_lookup (struct nsm_client *,
                           struct nsm_msg_ilm_lookup *,
                           struct nsm_msg_ilm_lookup *);
#endif /* HAVE_PACKET */

int nsm_client_ilm_gen_lookup (struct nsm_client *,
                               struct nsm_msg_ilm_gen_lookup *,
                               struct nsm_msg_ilm_gen_lookup *);

int nsm_client_send_igp_shortcut_route (u_int32_t, vrf_id_t,
                                        struct nsm_client *,
                                        struct nsm_msg_igp_shortcut_route *);
#ifdef HAVE_RESTART
int nsm_client_send_force_stale_remove (struct nsm_client *, afi_t,
                                        safi_t, u_int32_t, bool_t);
#endif /* HAVE_RESTART */
#endif /* HAVE_MPLS */
int nsm_client_send_route_manage (struct nsm_client *, int,
                                  struct nsm_msg_route_manage *);
int nsm_client_send_route_manage_vr (struct nsm_client *, int,
                                     struct nsm_msg_route_manage *,
                                     u_int32_t);
int nsm_client_send_preserve (struct nsm_client *, int);
int nsm_client_send_preserve_with_val (struct nsm_client *, int, u_char *,
                                       u_int16_t);
int nsm_client_send_stale_remove (struct nsm_client *, afi_t, safi_t);
int nsm_client_send_stale_mark (struct nsm_client *, afi_t,
                                safi_t, u_int32_t);

int nsm_util_send_redistribute_set (struct nsm_client *, int, vrf_id_t);
int nsm_util_send_redistribute_unset (struct nsm_client *, int, vrf_id_t);
int nsm_client_process_pending_msg (struct thread *);

#ifdef HAVE_CRX
int nsm_client_send_route_clean (struct nsm_client *,
                                 struct nsm_msg_route_clean *);
int nsm_client_send_protocol_restart (struct nsm_client *,
                                      struct nsm_msg_protocol_restart *);
#endif /* HAVE_CRX */

#ifdef HAVE_TE
struct interface *
nsm_util_interface_priority_bw_update (struct nsm_msg_link *,
                                       struct if_master *);
void
nsm_util_admin_group_update (struct admin_group [],
                             struct nsm_msg_admin_group *);
#endif /* HAVE_TE */

#ifdef HAVE_GMPLS
s_int32_t
nsm_util_te_link_update (struct telink *,
                         struct nsm_msg_te_link *);
#endif /* HAVE_GMPLS */

#if 0 /*TBD old code*/
#ifdef HAVE_GMPLS
int nsm_util_interface_gmpls_update (struct nsm_msg_gmpls_if *,
                                     struct interface *);
#endif /* HAVE_GMPLS */
#endif

struct ftn_add_data;
struct ilm_add_data;
struct ftn_del_data;
struct ilm_del_data;
#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
  struct nsm_msg_vpn_rd;
  struct l3vpn_rd;
#endif /* HAVE_MPLS_NSM || HAVE_NSM_MPLS_OAM */

int nsm_client_send_ftn_add_ipv4 (struct nsm_client *, struct ftn_add_data *,
                                  u_int32_t *);
int nsm_client_send_ftn_del_slow_ipv4 (struct nsm_client *,
                                       struct ftn_add_data *);
int nsm_client_send_ftn_del_ipv4 (struct nsm_client *,
                                  struct ftn_del_data *);
int nsm_client_send_ilm_add_ipv4 (struct nsm_client *, struct ilm_add_data *,
                                  u_int32_t *);
int nsm_client_send_ilm_fast_del_ipv4 (struct nsm_client *,
                                       struct ilm_del_data *);
int nsm_client_send_ilm_del_ipv4 (struct nsm_client *,
                                  struct ilm_add_data *);
s_int32_t
nsm_client_register_route_lookup_ipv4 (u_int32_t vr_id,
                                       u_int32_t vrf_id,
                                       struct nsm_client *nc,
                                       struct nsm_msg_route_lookup_ipv4 *msg,
                                       u_int32_t lookup_type);
s_int32_t
nsm_client_deregister_route_lookup_ipv4 (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_route_lookup_ipv4 *msg,
                                         u_int32_t lookup_type);
#ifdef HAVE_IPV6
int nsm_client_send_ftn_add_ipv6 (struct nsm_client *, struct ftn_add_data *,
                                  u_int32_t *);
int nsm_client_send_ftn_del_slow_ipv6 (struct nsm_client *,
                                       struct ftn_add_data *);
int nsm_client_send_ftn_del_ipv6 (struct nsm_client *,
                                  struct ftn_del_data *);
int nsm_client_send_ilm_add_ipv6 (struct nsm_client *, struct ilm_add_data *,
                                  u_int32_t *);
int nsm_client_send_ilm_fast_del_ipv6 (struct nsm_client *,
                                       struct ilm_del_data *);
int nsm_client_send_ilm_del_ipv6 (struct nsm_client *,
                                  struct ilm_add_data *);
s_int32_t
nsm_client_register_route_lookup_ipv6 (u_int32_t vr_id,
                                       u_int32_t vrf_id,
                                       struct nsm_client *nc,
                                       struct nsm_msg_route_lookup_ipv6 *msg,
                                       u_int32_t lookup_type);
s_int32_t
nsm_client_deregister_route_lookup_ipv6 (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_route_lookup_ipv6 *msg,
                                         u_int32_t lookup_type);
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS_VC
int nsm_client_send_vc_fib_add (struct nsm_client *,
                                struct nsm_msg_vc_fib_add *);
int nsm_client_send_tunnel_info (struct nsm_client *,
                                 struct nsm_msg_vc_tunnel_info *);
int nsm_client_send_vc_fib_delete (struct nsm_client *,
                                     struct nsm_msg_vc_fib_delete *);
int nsm_client_send_pw_status (struct nsm_client *, struct nsm_msg_pw_status *);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
int nsm_client_send_mac_address_withdraw (struct nsm_client *,
                                          struct nsm_msg_vpls_mac_withdraw *);
#endif /* HAVE_VPLS */

#ifdef HAVE_DIFFSERV
u_char nsm_util_supported_dscp_update (struct nsm_msg_supported_dscp_update *,
                                       u_char *);
void nsm_util_dscp_exp_map_update (struct nsm_msg_dscp_exp_map_update *,
                                   u_char *);

#endif /* HAVE_DIFFSERV */

#ifdef HAVE_MCAST_IPV4
s_int32_t
nsm_client_send_ipv4_vif_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_vif_add *msg,
                              struct nsm_msg_ipv4_mrib_notification *notif,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv4_vif_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_vif_del *msg,
                              struct nsm_msg_ipv4_mrib_notification *notif,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv4_mrt_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_mrt_add *msg,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv4_mrt_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv4_mrt_del *msg,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv4_mrt_stat_flags_update (u_int32_t vr_id,
                                            u_int32_t vrf_id,
                                            struct nsm_client *nc,
                                            struct nsm_msg_ipv4_mrt_stat_flags_update *msg,
                                            u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv4_mrt_state_refresh_flag_update (u_int32_t vr_id,
                                                    u_int32_t vrf_id,
                                                    struct nsm_client *nc,
                                                    struct nsm_msg_ipv4_mrt_state_refresh_flag_update *msg,
                                                    u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv4_mrt_wholepkt_reply (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_ipv4_mrt_wholepkt_reply *msg,
                                         u_int32_t *msg_id);
s_int32_t
nsm_client_send_mtrace_query_msg (u_int32_t vr_id,
                                  u_int32_t vrf_id,
                                  struct nsm_client *nc,
                                  struct nsm_msg_mtrace_query *msg,
                                  u_int32_t *msg_id);
s_int32_t
nsm_client_send_mtrace_request_msg (u_int32_t vr_id,
                                    u_int32_t vrf_id,
                                    struct nsm_client *nc,
                                    struct nsm_msg_mtrace_request *msg,
                                    u_int32_t *msg_id);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
s_int32_t
nsm_client_send_ipv6_mif_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mif_add *msg,
                              struct nsm_msg_ipv6_mrib_notification *notif,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv6_mif_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mif_del *msg,
                              struct nsm_msg_ipv6_mrib_notification *notif,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv6_mrt_add (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mrt_add *msg,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv6_mrt_del (u_int32_t vr_id,
                              u_int32_t vrf_id,
                              struct nsm_client *nc,
                              struct nsm_msg_ipv6_mrt_del *msg,
                              u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv6_mrt_stat_flags_update (u_int32_t vr_id,
                                            u_int32_t vrf_id,
                                            struct nsm_client *nc,
                                            struct nsm_msg_ipv6_mrt_stat_flags_update *msg,
                                            u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv6_mrt_state_refresh_flag_update (u_int32_t vr_id,
                                                    u_int32_t vrf_id,
                                                    struct nsm_client *nc,
                                                    struct nsm_msg_ipv6_mrt_state_refresh_flag_update *msg,
                                                    u_int32_t *msg_id);
s_int32_t
nsm_client_send_ipv6_mrt_wholepkt_reply (u_int32_t vr_id,
                                         u_int32_t vrf_id,
                                         struct nsm_client *nc,
                                         struct nsm_msg_ipv6_mrt_wholepkt_reply *msg,
                                         u_int32_t *msg_id);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_LACPD
int nsm_client_send_lacp_aggregator_add (struct nsm_client *,
                                         struct nsm_msg_lacp_agg_add *);
int nsm_client_send_lacp_aggregator_del (struct nsm_client *, char *);
#endif /* HAVE_LACPD */

#ifndef HAVE_CUSTOM1
#ifdef HAVE_L2
int nsm_client_send_bridge_if_msg (struct nsm_client *,
    struct nsm_msg_bridge_if *msg, int msgtype);
int nsm_client_send_bridge_msg (struct nsm_client *,
    struct nsm_msg_bridge *msg, int msgtype);
int nsm_client_send_bridge_port_msg (struct nsm_client *,
    struct nsm_msg_bridge_port *msg, int msgtype);
int nsm_client_send_bridge_enable_msg (struct nsm_client *nc,
                                   struct nsm_msg_bridge_enable *msg);

#ifdef HAVE_ONMD
int nsm_client_send_cfm_mac_message (struct nsm_client *nc,
                                     struct nsm_msg_cfm_mac *msg,
                                     struct nsm_msg_cfm_ifindex *reply);
#ifdef HAVE_G8032
int
nsm_client_send_bridge_g8032_port_state_msg (struct nsm_client *nc,
                             struct nsm_msg_bridge_g8032_port *msg,
                             int msgtype);
#endif /*HAVE_G8032*/

#endif /* HAVE_ONMD */

#endif /* HAVE_L2 */

#ifdef HAVE_VLAN
/* Send Vlan Message. */
int nsm_client_send_vlan_msg (struct nsm_client *nc,
                              struct nsm_msg_vlan *msg, int msgtype);

/* Send Vlan Port message. Vlan added/deleted from interface */
int nsm_client_send_vlan_port_msg (struct nsm_client *nc,
                                   struct nsm_msg_vlan_port *msg,
                                   int msgtype);
#endif /* HAVE_VLAN */
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_AUTHD
int nsm_client_send_auth_port_state (struct nsm_client *, u_int32_t,
                                     struct nsm_msg_auth_port_state *);

#ifdef HAVE_MAC_AUTH
s_int32_t
nsm_client_send_auth_mac_port_state (struct nsm_client *, u_int32_t,
                                     struct nsm_msg_auth_mac_port_state *);

s_int32_t
nsm_client_send_auth_mac_auth_status (struct nsm_client *, u_int32_t,
                                      struct nsm_msg_auth_mac_status *);
#endif /* HAVE_MAC_AUTH */
#endif /* HAVE_AUTHD */

#ifdef HAVE_ONMD

s_int32_t
nsm_client_send_efm_if_msg (struct nsm_client *nc, u_int32_t vr_id,
                            struct nsm_msg_efm_if *msg,
                            enum   nsm_efm_opcode opcode);

int
nsm_client_send_lldp_msg (struct nsm_client *nc, u_int32_t vr_id,
                          struct nsm_msg_lldp *msg,
                          enum nsm_lldp_opcode opcode);
int
nsm_client_send_cfm_msg (struct nsm_client *nc, u_int32_t vr_id,
                         struct nsm_msg_cfm *msg,
                         enum nsm_cfm_opcode opcode);

int
nsm_client_send_cfm_status_msg (struct nsm_client *nc, u_int32_t vr_id,
                         struct nsm_msg_cfm_status *msg);

#endif /* HAVE_ONMD */

#ifdef HAVE_ELMID
int
nsm_client_send_elmi_status_msg (struct nsm_client *nc, u_int32_t vr_id,
                                 struct nsm_msg_elmi_status *msg);

int elmi_nsm_send_auto_vlan_port (struct nsm_client *,
                                  struct nsm_msg_vlan_port *msg,
                                  int msgtype);

#endif /* HAVE_ELMID */

#ifdef HAVE_L2
/* Send the port up/down message to nsm */
int nsm_client_send_stp_message (struct nsm_client *nc,
                                 struct nsm_msg_stp *msg,
                                 int msgtype);
#endif /* HAVE_L2 */

#ifdef HAVE_RMOND
int nsm_client_send_rmon_message (struct nsm_client *nc,
                                  struct nsm_msg_rmon *msg,
                                  struct nsm_msg_rmon_stats *reply);
#endif /* HAVE_RMOND */
#ifdef HAVE_MPLS_FRR
int
nsm_client_send_rsvp_control_packet (struct nsm_client *nc,
                                     struct nsm_msg_rsvp_ctrl_pkt *msg);
#endif /* HAVE_MPLS_FRR */

#ifdef HAVE_NSM_MPLS_OAM
int
nsm_client_send_mpls_onm_request (struct nsm_client *,
                                  struct nsm_msg_mpls_ping_req *);
#endif /* HAVE_NSM_MPLS_OAM */
#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
int
nsm_client_send_mpls_vpn_rd (u_int32_t, vrf_id_t, struct nsm_msg_vpn_rd *,
                             struct nsm_client *);
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */

int
nsm_client_send_nexthop_tracking (u_int32_t , vrf_id_t,
                                  struct nsm_client *,
                                  struct nsm_msg_nexthop_tracking *);

#if defined HAVE_PBB_TE && defined HAVE_I_BEB && defined HAVE_B_BEB && defined HAVE_CFM
int
nsm_client_send_pbb_te_port_state_msg (struct nsm_client *nc,
                                       struct nsm_msg_bridge_pbb_te_port *msg);


#endif /* HAVE_PBB_TE */

/**********************************************************************
 *           Support for the COMMSG communication interface
 **********************************************************************
 */
/*----------------------------------------------------------------
 * nsm_client_commsg_init - Register the COMMSG with the nsm_client
 *                          dispatcher.
 *                          Register the COMMSG utility receive callback.
 *                          This must be called from the  module *_start()
 *                          function.
 *  nc        - nsm_client structure pointer
 *  recv_cb   - The COMMSG utility receive callback.
 *---------------------------------------------------------------
 */
int
nsm_client_commsg_init(struct nsm_client *nc,
                       nsm_msg_commsg_recv_cb_t recv_cb,
                       void *user_ref);

/*------------------------------------------------------------------
 * nsm_client_commsg_getbuf - Obtain the network buffer.
 *                            This is called by the COMMSG utility.
 *  nsm_clt   - nsm_client pointer
 *  dst_mod_id- must be APN_PROTO_NSM
 *  size      - Requested buffer space in bytes.
 *------------------------------------------------------------------
 */
u_char *
nsm_client_commsg_getbuf(void       *nsm_clt,
                         module_id_t dst_mod_id,
                         u_int16_t   size);
/*------------------------------------------------------------------
 * nsm_client_commsg_send - Send the COMMSG
 *                          This is called by the COMMSG utility.
 *  nsm_clt   - nsm_client pointer
 *  dst_mod_id- must be APN_PROTO_NSM
 *  msg_buf   - Ptr to the message buffer
 *  msg_len   - Message length
 *------------------------------------------------------------------
 */
int
nsm_client_commsg_send(void       *nsm_clt,
                       module_id_t dst_mod_id,
                       u_char     *msg_buf,
                       u_int16_t   msg_len);

#ifdef HAVE_G8031
int
nsm_client_send_eps_msg ( struct nsm_client *nc,
                          u_int32_t vr_id,
                          struct nsm_msg_pg_initialized *msg,
                          enum nsm_eps_opcode opcode);
int
nsm_client_send_g8031_port_state_msg ( struct nsm_client *nc,
    u_int32_t vr_id,
    struct nsm_msg_g8031_portstate *msg);

#endif /*HAVE_G8031 */

#ifdef HAVE_GMPLS
int nsm_client_send_gen_ftn_add (struct nsm_client *,
                                 struct ftn_add_gen_data *, u_int32_t *);

int nsm_client_send_gen_ftn_del_slow (struct nsm_client *,
                                      struct ftn_add_gen_data *);

int nsm_client_send_gen_ftn_del (struct nsm_client *,
                                 struct ftn_del_gen_data *);
int nsm_client_send_gen_ilm_add (struct nsm_client *,
                                 struct ilm_add_gen_data *, u_int32_t *);
int nsm_client_send_gen_ilm_fast_del (struct nsm_client *,
                                      struct ilm_del_gen_data *);
int nsm_client_send_gen_ilm_del (struct nsm_client *,
                                  struct ilm_add_gen_data *);

int nsm_client_send_gen_bidir_ftn_add (struct nsm_client *,
                                  struct ftn_bidir_add_data *, u_int32_t *);

int nsm_client_sent_gen_bidir_ilm_add (struct nsm_client *,
                                  struct ilm_bidir_add_data *, u_int32_t *);

int nsm_client_send_gen_bidir_ftn_del (struct nsm_client *,
                                       struct ftn_bidir_del_data *);

int nsm_client_send_gen_bidir_ilm_del (struct nsm_client *,
                                       struct ilm_bidir_del_data *);

int nsm_client_send_gen_bidir_ftn_del_slow (struct nsm_client *,
                                            struct ftn_bidir_add_data *);

int nsm_client_send_gen_bidir_ilm_del_slow (struct nsm_client *,
                                            struct ilm_bidir_add_data *);

s_int32_t nsm_client_send_data_link (u_int32_t, vrf_id_t ,
                                     struct nsm_client *,
                                     struct nsm_msg_data_link *);
s_int32_t nsm_client_send_te_link (u_int32_t, vrf_id_t,
                                   struct nsm_client *,
                                   struct nsm_msg_te_link *);
#ifdef HAVE_LMP
int
nsm_client_send_dlink_bundle_opaque (u_int32_t ,
                                     vrf_id_t ,
                                     struct nsm_client *,
                                     struct nsm_msg_dlink_opaque *,
                                     int,
                                     struct nsm_msg_dlink_opaque *);

s_int32_t
nsm_client_send_control_channel (u_int32_t vr_id, vrf_id_t vrf_id,
                                 struct nsm_client *nc,
                                 struct nsm_msg_control_channel *msg);

int
nsm_client_send_dlink_opaque (u_int32_t,
                              vrf_id_t,
                              struct nsm_client *,
                              struct nsm_msg_dlink_opaque *,
                              struct nsm_msg_dlink_opaque *);

int
nsm_client_send_dlink_testmsg (u_int32_t,
                               vrf_id_t,
                               struct nsm_client *,
                               struct nsm_msg_dlink_testmsg *);
#endif /* HAVE_LMP */

s_int32_t
nsm_client_send_generic_label_msg (struct nsm_client *,
                          int , struct nsm_msg_generic_label_pool *,
                          struct nsm_msg_generic_label_pool *);

s_int32_t
nsm_util_data_link_update (struct datalink *dl, struct nsm_msg_data_link *msg);

#endif /* HAVE_GMPLS */
#ifdef HAVE_MPLS_OAM
int
nsm_client_send_oam_lsp_ping_req_process (struct nsm_client *,
                                    struct nsm_msg_oam_lsp_ping_req_process *);
int
nsm_client_send_oam_lsp_ping_rep_process_ldp (struct nsm_client *,
                                struct nsm_msg_oam_lsp_ping_rep_process_ldp *);
int
nsm_client_send_oam_lsp_ping_rep_process_rsvp (struct nsm_client *,
                               struct nsm_msg_oam_lsp_ping_rep_process_rsvp *);
int
nsm_client_send_oam_lsp_ping_rep_process_gen (struct nsm_client *,
                                struct nsm_msg_oam_lsp_ping_rep_process_gen *);
#ifdef HAVE_MPLS_VC
int
nsm_client_send_oam_lsp_ping_rep_process_l2vc (struct nsm_client *,
                               struct nsm_msg_oam_lsp_ping_rep_process_l2vc *);
#endif /* HAVE_MPLS_VC */

int
nsm_client_send_oam_lsp_ping_rep_process_l3vpn (struct nsm_client *,
                              struct nsm_msg_oam_lsp_ping_rep_process_l3vpn *);
#endif /* HAVE_MPLS_OAM */
#endif /*  _NSM_CLIENT_H */

