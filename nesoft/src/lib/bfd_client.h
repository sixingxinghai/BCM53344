/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_BFD_CLIENT_H
#define _PACOS_BFD_CLIENT_H

#include "bfd_client_api.h"

#define BFD_CLIENT_RECONNECT_INTERVAL  5

/* Structure to hold pending message. */
struct bfd_client_pend_msg
{
  /* BFD Msg Header. */
  struct bfd_msg_header header;

  /* Message. */
  u_char buf[BFD_MESSAGE_MAX_LEN];
};

/* BFD client to server connection structure. */
struct bfd_client_handler
{
  /* Parent. */
  struct bfd_client *bc;

  /* Up or down.  */
  int up;

  /* Type of client, sync or async.  */
  int type;

  /* Message client structure. */
  struct message_handler *mc;

  /* Service bits specific for this connection.  */
  struct bfd_msg_service service;

  /* Message buffer for output. */
  u_char buf[BFD_MESSAGE_MAX_LEN];
  u_int16_t len;
  u_char *pnt;
  u_int16_t size;

  /* Message buffer for input. */
  u_char buf_in[BFD_MESSAGE_MAX_LEN];
  u_int16_t len_in;
  u_char *pnt_in;
  u_int16_t size_in;

  /* Message buffer for Session addition.  */
  u_char buf_session_add[BFD_MESSAGE_MAX_LEN];
  u_char *pnt_session_add;
  u_int16_t len_session_add;
  struct thread *t_session_add;
  u_int32_t vr_id_session_add;
  vrf_id_t vrf_id_session_add;

  /* Message buffer for Session deletion.  */
  u_char buf_session_delete[BFD_MESSAGE_MAX_LEN];
  u_char *pnt_session_delete;
  u_int16_t len_session_delete;
  struct thread *t_session_delete;
  u_int32_t vr_id_session_delete;
  vrf_id_t vrf_id_session_delete;

  /* Client message ID.  */
  u_int32_t message_id;

  /* List of pending messages of struct bfd_client_pend_msg. */
  struct list pend_msg_list;

  /* Send and recieved message count.  */
  u_int32_t send_msg_count;
  u_int32_t recv_msg_count;
};

/* BFD client structure.  */
struct bfd_client
{
  struct lib_globals *zg;

  /* BFD client state.  */
  u_char state;
#define BFD_CLIENT_STATE_INIT          0
#define BFD_CLIENT_STATE_CONNECTED     1
#define BFD_CLIENT_STATE_DISCONNECTED  2

  /* Service bits. */
  struct bfd_msg_service service;

  /* Parser functions. */
  BFD_PARSER parser[BFD_MSG_MAX];

  /* Callback functions. */
  BFD_CALLBACK callback[BFD_MSG_MAX];

  /* Async connection. */
  struct bfd_client_handler *async;

  /* Reconnect thread. */
  struct thread *t_connect;

  /* Reconnect interval in seconds. */
  int reconnect_interval;

  /* Preserve time for the restart.  */
  u_int32_t preserve_time;

  /* Debug message flag. */
  int debug;

  /* PM callbacks.  */
  struct bfd_client_api_callbacks pm_callbacks;
};

#define BFD_CLIENT_BUNDLE_TIME         0

#ifndef BFD_SERV_PATH
#define BFD_SERV_PATH "/tmp/.bfdserv"
#endif /* BFD_SERV_PATH */

int bfd_client_send_session_add (struct bfd_client *bc,
                                 u_int32_t, vrf_id_t,
                                 struct bfd_msg_session *);
int bfd_client_flush_session_add (struct bfd_client *bc);
int bfd_client_send_session_delete (struct bfd_client *bc,
                                    u_int32_t, vrf_id_t,
                                    struct bfd_msg_session *);
int bfd_client_flush_session_delete (struct bfd_client *bc);
int bfd_client_send_preserve (struct bfd_client *bc, u_int32_t preserve_time);
int bfd_client_send_stale_remove (struct bfd_client *bc);
struct bfd_client *bfd_client_new (struct lib_globals *zg,
                                   struct bfd_client_api_callbacks *cb);
int bfd_client_delete (struct bfd_client *bc);
int bfd_client_start (struct bfd_client *bc);
void bfd_client_stop (struct bfd_client *bc);

void bfd_client_set_service (struct bfd_client *bc, int);
void bfd_client_set_version (struct bfd_client *bc, u_int16_t);
void bfd_client_set_callback (struct bfd_client *bc, int, BFD_CALLBACK);
#ifdef HAVE_MPLS_OAM
int bfd_client_send_mpls_onm_request (struct bfd_client *,
                                      struct oam_msg_mpls_ping_req *);
struct bfd_client *bfd_oam_client_new (struct lib_globals *zg,
                                       struct bfd_client_api_callbacks *cb);
#endif /* HAVE_MPLS_OAM */                           
#endif /* _PACOS_BFD_CLIENT_H */
