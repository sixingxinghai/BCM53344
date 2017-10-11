/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_SERVER_H
#define _NSM_SERVER_H

#include "nsm_message.h"
#include "nsm_vrf.h"
#ifdef HAVE_L3
#include "nsm_table.h"
#include "rib.h"
#endif /* HAVE_L3 */

#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_MPLS
#include "mpls_common.h"
#include "nsm_mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */


#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_L3
#include "nsm_static_mroute.h"
#endif

/* Send/Recv data structure for server entry. */
struct nsm_server_entry_buf
{
  u_char buf[NSM_MESSAGE_MAX_LEN];
  u_int16_t len;

  u_char *pnt;
  u_int16_t size;
};

/* NSM Server buffer structure for handling bulk messages. */
struct nsm_server_bulk_buf
{
    /* Buffer used to send bulk message */
    struct nsm_server_entry_buf bulk_buf;

    /* Timer for handling server bulk buffer.  */
    struct thread *bulk_thread;
};
#define NSM_SERVER_BUNDLE_TIME         1

/*
    +-------- nsm_server --------+
    | Message Handler            |
    | NSM services bit           |
    | NSM message parser         |
    | NSM message callback       |
    | vector of NSM server Client|
    +--------------|-------------+
                   |
                   v
    +------- nsm_server_client ----+
    |  NSM client ID               |-+
    |  nsm_server_entry for ASYNC  | |-+
    |  nsm_server_entry for SYNC   | | |
    +------------------------------+ | |
      +------------------------------+ |
        +------------------------------+
*/

/* NSM server structure.  */
struct nsm_server
{
  struct lib_globals *zg;

  struct message_handler *ms;

  /* NSM service structure.  */
  struct nsm_msg_service service;

  /* Parser functions.  */
  NSM_PARSER parser[NSM_MSG_MAX];

  /* Call back functions.  */
  NSM_CALLBACK callback[NSM_MSG_MAX];

  /* Vector for all of NSM server client.  */
  vector client;

  /* Debug flag.  */
  int debug;

  /* COMMMSG recv callback. */
  nsm_msg_commsg_recv_cb_t  ns_commsg_recv_cb;
  void                     *ns_commsg_user_ref;

  /* Buffer for handling Bulk Messages from NSM server. */
  struct nsm_server_bulk_buf server_buf;
};

/* Keep track client requested redistribution.  */
struct nsm_redistribute
{
  struct nsm_redistribute *next;
  struct nsm_redistribute *prev;

  /* Route type.  */
  u_char type;

  /* AFI. */
  afi_t afi;

  /* VR ID. */
  u_int32_t vr_id;

  /* VRF ID. */
  u_int32_t vrf_id;
 
  int flag;
  #define ENABLE_INSTANCE_ID 1
  
  /* Process id */
  u_int32_t pid;
};

/* NSM server entry for each client connection.  */
struct nsm_server_entry
{
  /* Linked list. */
  struct nsm_server_entry *next;
  struct nsm_server_entry *prev;

  /* Pointer to message entry.  */
  struct message_entry *me;

  /* Pointer to NSM server structure.  */
  struct nsm_server *ns;

  /* Pointer to NSM server client.  */
  struct nsm_server_client *nsc;

  /* NSM service structure.  */
  struct nsm_msg_service service;

  /* Send/Recv buffers. */
  struct nsm_server_entry_buf send;
  struct nsm_server_entry_buf recv;

  /* Message buffer for IPv4 redistribute.  */
  u_char buf_ipv4[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv4;
  u_int16_t len_ipv4;
  struct thread *t_ipv4;

#ifdef HAVE_IPV6
  /* Message buffer for IPv6 redistribute.  */
  u_char buf_ipv6[NSM_MESSAGE_MAX_LEN];
  u_char *pnt_ipv6;
  u_int16_t len_ipv6;
  struct thread *t_ipv6;
#endif /* HAVE_IPV6 */

  /* Send and recieved message count.  */
  u_int32_t send_msg_count;
  u_int32_t recv_msg_count;

  /* Connect time.  */
  pal_time_t connect_time;

  /* Last read time.  */
  pal_time_t read_time;

  /* Redistribute request comes from this client.  */
  struct nsm_redistribute *redist;

  /* Message queue.  */
  struct fifo send_queue;
  struct thread *t_write;

  /* Message id */
  u_int32_t message_id;

  /* For record.  */
  u_int16_t last_read_type;
  u_int16_t last_write_type;
};

/* NSM server client.  */
struct nsm_server_client
{
  /* NSM client ID. */
  u_int32_t client_id;

  /* NSM storing BGP state information */
  u_int32_t conv_state;

  /* NSM server entry for sync connection.  */
  struct nsm_server_entry *head;
  struct nsm_server_entry *tail;

#ifdef HAVE_MCAST_IPV4
  struct nsm_msg_ipv4_mrt_stat_update nsc_mc4_stats;
#endif /* HAVE_MCAST_IPV4 */
#ifdef HAVE_MCAST_IPV6
  struct nsm_msg_ipv6_mrt_stat_update nsc_mc6_stats;
#endif /* HAVE_MCAST_IPV4 */
  /* NHT enabled/disabled flag
     if NHT is enabled this variable would be PAL_TRUE
     else the value of variable would be PAL_FALSE */
  u_int8_t nhtenable;
};

#define NSM_SERVER_CLIENT_LOOP(NG,I,NSC,NSE)                                  \
  for ((I) = 0; (I) < vector_max ((NG)->server->client); (I)++)               \
    if (((NSC) = vector_slot ((NG)->server->client, (I))))                    \
      for ((NSE) = (NSC)->head; (NSE); (NSE) = (NSE)->next)

/* During this time, message is bundled.  */
#define NSM_SERVER_BUNDLE_TIME              1

/* Function declarations. */
struct ftn_entry;

void nsm_server_set_callback (struct nsm_server *ns, int message_type,
                         NSM_PARSER parser, NSM_CALLBACK callback);
int nsm_server_send_message (struct nsm_server_entry *, u_int32_t, vrf_id_t,
                             int, u_int32_t, u_int16_t);
int nsm_server_send_message_msgid (struct nsm_server_entry *, u_int32_t, 
    vrf_id_t, int, u_int32_t *, u_int16_t);
int nsm_server_unset_service (struct nsm_server *, int);
int nsm_server_set_service (struct nsm_server *, int);
int nsm_server_send_interface_add (struct nsm_server_entry *,
                                   struct interface *);
int nsm_server_send_interface_state (struct nsm_server_entry *,
                                     struct interface *, int, cindex_t);
int nsm_server_send_interface_address (struct nsm_server_entry *,
                                       struct connected *, int);
int nsm_server_send_router_id_update (u_int32_t, vrf_id_t,
                                      struct nsm_server_entry *,
                                      struct pal_in4_addr);
int nsm_service_check (struct nsm_server_entry *, int);

int nsm_server_recv_isis_wait_bgp_set (struct nsm_msg_header *,
                                       void *, void *);
int nsm_server_recv_bgp_conv_done (struct nsm_msg_header *,
                                   void *, void *);
int nsm_server_send_wait_for_bgp_set (u_int32_t, vrf_id_t,
                                      struct nsm_server_entry *, u_int32_t,
                                      struct nsm_msg_wait_for_bgp *); 
int nsm_server_send_bgp_conv_done (u_int32_t, vrf_id_t,
                                   struct nsm_server_entry *, u_int32_t,
                                   struct nsm_msg_wait_for_bgp *);
void nsm_server_send_bgp_up (struct nsm_server_entry *);
void nsm_server_send_bgp_down (struct nsm_server_entry *);

void nsm_server_send_ldp_up (struct nsm_server_entry *, u_int32_t, u_int32_t);
void nsm_server_send_ldp_down (struct nsm_server_entry *, u_int32_t, u_int32_t);
 
int nsm_server_send_vr_add (struct nsm_server_entry *, struct apn_vr *);
int nsm_server_send_vr_delete (struct nsm_server_entry *, struct apn_vr *);
int nsm_server_send_vr_bind (struct nsm_server_entry *, struct interface *);
int nsm_server_send_vr_unbind (struct nsm_server_entry *, struct interface *);

int nsm_server_send_vrf_add (struct nsm_server_entry *, struct apn_vrf *);
int nsm_server_send_vrf_delete (struct nsm_server_entry *, struct apn_vrf *);
int nsm_server_send_vrf_add_all (struct nsm_master *nm,
                             struct nsm_server_entry *nse);

int nsm_server_send_vrf_bind (struct nsm_server_entry *, struct interface *);
int nsm_server_send_vrf_unbind (struct nsm_server_entry *, struct interface *);


int nsm_server_read_header (struct message_handler *, struct message_entry *,
                            pal_sock_handle_t);
int nsm_server_read_msg (struct message_handler *, struct message_entry *,
                         pal_sock_handle_t);
void nsm_server_set_version (struct nsm_server *, u_int16_t);
void nsm_server_set_protocol (struct nsm_server *, u_int32_t);

int
nsm_server_send_interface_sync (struct nsm_master *nm,
                                struct nsm_server_entry *nse);
struct nsm_server_entry *
nsm_server_lookup_by_proto_id (u_int32_t);

struct nsm_server * nsm_server_init (struct lib_globals *);
int nsm_server_finish (struct nsm_server *);

void nsm_server_if_add (struct interface *);
void nsm_server_if_delete (struct interface *);
void nsm_server_if_update (struct interface *, cindex_t);
void nsm_server_if_bind_all (struct interface *);
void nsm_server_if_unbind_all (struct interface *);
void nsm_server_if_address_add_update (struct interface *, struct connected *);
void nsm_server_if_address_delete_update (struct interface *,
                                          struct connected *);
void nsm_server_if_up_update (struct interface *, cindex_t);
void nsm_server_if_down_update (struct interface *, cindex_t);
int nsm_server_send_link_add (struct nsm_server_entry *nse, struct interface *ifp);
int nsm_server_send_link_delete (struct nsm_server_entry *nse, struct interface *ifp, int logical_deletion);
int nsm_server_if_clean_list_remove(struct interface *ifp, struct nsm_server_entry *nse);
int nsm_server_if_clean_list_add(struct interface *ifp, struct nsm_server_entry *nse);


#ifdef HAVE_TE
void nsm_server_if_priority_bw_update (struct interface *);
void nsm_admin_group_update (struct nsm_master *);
#endif /* HAVE_TE */

#ifdef HAVE_GMPLS
void nsm_gmpls_if_update (struct interface *);
#endif /* HAVE_GMPLS */

#ifdef HAVE_DIFFSERV
void nsm_mpls_supported_dscp_update (struct nsm_master *, u_char, u_char);
void nsm_mpls_dscp_exp_map_update (struct nsm_master *, u_char, u_char);
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_MCAST_IPV4
s_int32_t
nsm_server_recv_ipv4_vif_add (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv4_vif_del (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv4_mrt_add (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv4_mrt_del (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv4_mrt_stat_flags_update (struct nsm_msg_header *,
                                            void *, void *);
s_int32_t
nsm_server_recv_ipv4_mrt_wholepkt_reply (struct nsm_msg_header *,
                                         void *, void *);
s_int32_t
nsm_server_recv_igmp_if (struct nsm_msg_header *, void *, void *);

s_int32_t
nsm_server_send_ipv4_mrt_nocache (struct nsm_mcast *,
                                  struct nsm_server_entry *,
                                  struct nsm_msg_ipv4_mrt_nocache *,
                                  u_int32_t *);
s_int32_t
nsm_server_send_ipv4_mrt_wrongvif (struct nsm_mcast *,
                                   struct nsm_server_entry *,
                                   struct nsm_msg_ipv4_mrt_wrongvif *,
                                   u_int32_t *);
s_int32_t
nsm_server_send_ipv4_mrt_wholepkt_req (struct nsm_mcast *,
                                       struct nsm_server_entry *,
                                       struct nsm_msg_ipv4_mrt_wholepkt_req *,
                                       u_int32_t *);
s_int32_t
nsm_server_send_ipv4_mrt_stat_update (struct nsm_mcast *,
                                      struct nsm_server_entry *,
                                      struct nsm_msg_ipv4_mrt_stat_update *,
                                      u_int32_t *);
s_int32_t
nsm_server_send_ipv4_mrib_notification (u_int32_t vr_id, vrf_id_t vrf_id,
                                        struct nsm_server_entry *,
                                        struct nsm_msg_ipv4_mrib_notification *,
                                        u_int32_t *);
s_int32_t
nsm_server_send_mtrace_query_msg (struct nsm_mcast *,
                                  struct nsm_server_entry *,
                                  struct nsm_msg_mtrace_query *);
s_int32_t
nsm_server_send_mtrace_request_msg (struct nsm_mcast *,
                                    struct nsm_server_entry *,
                                    struct nsm_msg_mtrace_request *);
s_int32_t
nsm_server_send_igmp (struct nsm_mcast *,
                      struct nsm_server_entry *,
                      struct nsm_msg_igmp *);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
s_int32_t
nsm_server_recv_ipv6_mif_add (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv6_mif_del (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv6_mrt_add (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv6_mrt_del (struct nsm_msg_header *, void *, void *);
s_int32_t
nsm_server_recv_ipv6_mrt_stat_flags_update (struct nsm_msg_header *,
                                            void *, void *);
s_int32_t
nsm_server_recv_ipv6_mrt_wholepkt_reply (struct nsm_msg_header *,
                                         void *, void *);

s_int32_t
nsm_server_send_ipv6_mrt_nocache (struct nsm_mcast6 *,
                                  struct nsm_server_entry *,
                                  struct nsm_msg_ipv6_mrt_nocache *,
                                  u_int32_t *);
s_int32_t
nsm_server_send_ipv6_mrt_wrongmif (struct nsm_mcast6 *,
                                   struct nsm_server_entry *,
                                   struct nsm_msg_ipv6_mrt_wrongmif *,
                                   u_int32_t *);
s_int32_t
nsm_server_send_ipv6_mrt_wholepkt_req (struct nsm_mcast6 *,
                                       struct nsm_server_entry *,
                                       struct nsm_msg_ipv6_mrt_wholepkt_req *,
                                       u_int32_t *);
s_int32_t
nsm_server_send_ipv6_mrt_stat_update (struct nsm_mcast6 *,
                                      struct nsm_server_entry *,
                                      struct nsm_msg_ipv6_mrt_stat_update *,
                                      u_int32_t *);
s_int32_t
nsm_server_send_ipv6_mrib_notification (u_int32_t vr_id, vrf_id_t vrf_id,
                                        struct nsm_server_entry *,
                                        struct nsm_msg_ipv6_mrib_notification *,
                                        u_int32_t *);
s_int32_t
nsm_server_send_mld (struct nsm_mcast6 *,
                     struct nsm_server_entry *,
                     struct nsm_msg_mld *);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_LACPD
int nsm_server_if_lacp_admin_key_update (struct interface *);
#endif /* HAVE_LACPD */

#ifdef HAVE_MPLS_VC
void nsm_mpls_l2_circuit_del_send (struct nsm_mpls_if *,
                                   struct nsm_mpls_circuit *);
void nsm_mpls_l2_circuit_add_send (struct nsm_mpls_if *, 
                                   struct nsm_mpls_circuit *);
void nsm_mpls_l2_circuit_if_bind (struct interface *, u_int16_t, u_int8_t);
void nsm_mpls_l2_circuit_if_unbind (struct nsm_mpls_if *, u_int16_t, u_int8_t);
void nsm_mpls_l2_circuit_send_pw_status (struct nsm_mpls_if *, 
					 struct nsm_mpls_circuit *);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
int nsm_vpls_if_add_send (struct interface *, u_int16_t);
int nsm_vpls_vlan_if_delete_send (struct interface *);
void nsm_vpls_spoke_vc_send_pw_status (struct nsm_vpls *,
                                       struct nsm_mpls_circuit *);
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS_FRR
int nsm_server_recv_rsvp_control_packet (struct nsm_msg_header *header,
                                         void *arg, void *message);
#endif /* HAVE_MPLS_FRR */

int
nsm_server_if_bind_check (struct nsm_server_entry *nse, struct interface *ifp);
int
nsm_server_send_route_ipv4 (u_int32_t vr_id, vrf_id_t vrf_id,
                            struct nsm_server_entry *nse, u_int32_t msg_id,
                            struct nsm_msg_route_ipv4 *msg);

#ifdef HAVE_IPV6
int nsm_server_send_route_ipv6 (u_int32_t, vrf_id_t, struct nsm_server_entry *, u_int32_t,
                                struct nsm_msg_route_ipv6 *);
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
#ifdef HAVE_NSM_MPLS_OAM
int nsm_server_recv_mpls_oam_req (struct nsm_msg_header *, void *, void *); 

int
nsm_mpls_oam_process_echo_request (struct nsm_master *,
                                   struct nsm_server_entry *,
                                   struct nsm_msg_mpls_ping_req *);
#endif /* HAVE_NSM_MPLS_OAM */
void nsm_mpls_send_igp_shortcut_lsp (struct ftn_entry *, struct prefix *,
                                     u_char);
#ifdef HAVE_MPLS_OAM_ITUT
int
nsm_server_recv_mpls_oam_process_req (struct nsm_msg_header *, void *, void *);
#endif /* HAVE_MPLS_OAM_ITUT */

#ifdef HAVE_MPLS_OAM
int
nsm_server_recv_mpls_oam_req_process (struct nsm_msg_header *header,
                                           void *arg, void *message);

int
nsm_server_recv_mpls_oam_rep_process_ldp (struct nsm_msg_header *header,
                                         void *arg, void *message);

int
nsm_server_recv_mpls_oam_rep_process_gen (struct nsm_msg_header *header,
                                         void *arg, void *message);

int
nsm_server_recv_mpls_oam_rep_process_rsvp (struct nsm_msg_header *header,
                                          void *arg, void *message);

#ifdef HAVE_MPLS_VC
int
nsm_server_recv_mpls_oam_rep_process_l2vc (struct nsm_msg_header *header,
                                          void *arg, void *message);
#endif /* HAVE_MPLS_VC */

int
nsm_server_recv_mpls_oam_rep_process_l3vpn (struct nsm_msg_header *header,
                                           void *arg, void *message);
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_GMPLS
int nsm_server_send_gmpls_control_channel (u_int32_t, vrf_id_t, 
                                           struct nsm_server_entry *, 
                                           struct control_channel *, cindex_t);
int nsm_server_send_gmpls_control_adj (u_int32_t, vrf_id_t,
                                       struct nsm_server_entry *,
                                       struct control_adj *, cindex_t);
int nsm_server_send_gmpls_control_adj_sync (struct nsm_master *, 
                                            struct nsm_server_entry *);
int nsm_server_send_gmpls_control_channel_sync (struct nsm_master *,
                                                struct nsm_server_entry *);
s_int32_t nsm_server_send_data_link_sub (u_int32_t vr_id, vrf_id_t vrf_id,
                               struct nsm_server_entry *nse,
                               struct datalink *dl, cindex_t cindex);

s_int32_t nsm_server_send_data_link (u_int32_t vr_id, vrf_id_t vrf_id,
                           struct nsm_server_entry *nse,
                           struct datalink *dl, cindex_t cindex);

s_int32_t nsm_server_send_te_link (u_int32_t vr_id, vrf_id_t vrf_id,
                                   struct nsm_server_entry *nse,
                                   struct telink *tl, cindex_t cindex);

s_int32_t nsm_server_send_data_link_mod (vrf_id_t vrid, struct lib_globals *zg,
                                         struct datalink *dl, cindex_t cindex);

s_int32_t
nsm_server_send_te_link_mod (vrf_id_t vrid, struct lib_globals *zg,
                             struct telink *tl, cindex_t cindex);
#endif /* HAVE_GMPLS */

#ifdef HAVE_RESTART
int nsm_gmpls_send_stale_bundle_msg (struct nsm_gen_msg_stale_info *);
int nsm_mpls_send_stale_bundle_msg (struct nsm_msg_stale_info *);
int nsm_mpls_send_stale_flush (struct thread *); 
int nsm_gmpls_send_gen_stale_flush (struct thread *); 
#endif /* HAVE_RESTART */
#endif /* HAVE_MPLS */

/**********************************************************************
 *           Support for the COMMSG communication interface
 *----------------------------------------------------------------
 */
/*----------------------------------------------------------------
 * nsm_server_commsg_init - Register the COMMSG with the nsm_server
 *                          dispatcher. 
 *  This is called by the NSM application initialization. It hooks up 
 *  the NSM with the COMMSG utility, so the COMMSG can receive its 
 *  messages.
 *
 *  ns        - nsm_server structure pointer
 *  recv_cb   - The COMMSG utility receive callback.
 *  
 *  NOTE: This function has a prototype specific to the NSM. 
 *---------------------------------------------------------------
 */
int 
nsm_server_commsg_init(struct nsm_server *ns, 
                       nsm_msg_commsg_recv_cb_t recv_cb,
                       void *user_ref);

/*------------------------------------------------------------------
 * nsm_server_commsg_getbuf - Obtain the network buffer.
 *
 *  nsm_srv   - nsm_server pointer
 *  dst_mod_id- must be APN_PROTO_NSM
 *  size      - Requested buffer space in bytes.
 *  
 *  NOTE: This function has a generic prototype used on the client and 
 *        the server.
 *------------------------------------------------------------------
 */
u_char *
nsm_server_commsg_getbuf(void       *nsm_srv, 
                         module_id_t dst_mod_id, 
                         u_int16_t   size);
/*------------------------------------------------------------------
 * nsm_server_commsg_send - Send the COMMSG
 *
 *  nsm_clt   - nsm_client pointer
 *  dst_mod_id- must be APN_PROTO_NSM
 *  msg_buf   - Ptr to the message buffer
 *  msg_len   - Message length
 *
 *  NOTE: This function has a generic prototype used on the client and 
 *        the server.
 *------------------------------------------------------------------
 */
int 
nsm_server_commsg_send(void       *nsm_srv, 
                       module_id_t dst_mod_id,
                       u_char     *msg_buf, 
                       u_int16_t   msg_len);
void 
nsm_imi_config_completed (struct lib_globals *zg);

void
nsm_server_send_vr_sync_done (struct nsm_server_entry *nse);

void nsm_server_send_nsm_status (u_char nsm_status);

int
nsm_pbb_sync (struct nsm_master *nm, struct nsm_server_entry *nse);

#ifdef HAVE_ELMID
int
nsm_elmi_sync (struct nsm_master *nm, struct nsm_server_entry *nse);
#endif /* HAVE_ELMID */
#endif /* _NSM_SERVER_H */
