/**@file onm_client.h
 ** @brief  This file contains data structures and Prototypes for ONM client 
 **          implementation.
 **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _ONM_CLIENT_H
#define _ONM_CLIENT_H

#include "message.h"
#include "onm_message.h"

struct message_entry;
struct onm_client;

#define ONM_CLIENT_RECONNECT_INTERVAL  5

typedef int (*ONM_API_CB) (struct onm_client *, void *);

struct onm_api_callback
{
  ONM_API_CB onm_api_cb_conn_reset;
  ONM_API_CB onm_api_cb_conn_established;
  ONM_API_CB onm_api_cb_route_recv;
  ONM_API_CB onm_api_cb_notification_recv;
  ONM_API_CB onm_api_cb_lsp_addr_recv;
};

/* Structure to hold pending message. */
struct onm_client_pend_msg
{
  /* ONM Msg Header. */
  struct onm_api_msg_header header;

  /* Message. */
  u_char buf[ONM_MSG_MAX_LEN];
};

/* ONM client to server connection structure. */
struct onm_client_handler
{
  /* Parent. */
  struct onm_client *oc;

  /* Up or down.  */
  int up;

  /* Type of client, sync or async.  */
  int type;

  /* Message client structure. */
  struct message_handler *mc;

  /* Service bits specific for this connection.  */
  struct onm_msg_service service;

  /* Message buffer for output. */
  u_char buf[ONM_MSG_MAX_LEN];
  u_int16_t len;
  u_char *pnt;
   u_int16_t len_ipv4;
  u_int16_t size;

  /* Message buffer for input. */
  u_char buf_in[ONM_MSG_MAX_LEN];
  u_int16_t len_in;
  u_char *pnt_in;
  u_int16_t size_in;

  /* Client message ID.  */
  u_int32_t message_id;

  /* List of pending messages of struct onm_client_pend_msg. */
  struct list pend_msg_list;

  /* Send and recieved message count.  */
  u_int32_t send_msg_count;
  u_int32_t recv_msg_count;
};

/* ONM client structure.  */
struct onm_client
{
  struct lib_globals *zg;
  
  /* Service bits. */
  struct onm_msg_service service;

  /* ONM client ID. */
  u_int32_t client_id;

 int (*send_message) (struct onm_client *, u_int16_t, u_int16_t, u_char);

  /* Parser functions. */
   ONM_PARSER parser[ONM_MSG_MAX];
  /* Callback. */

   ONM_CALLBACK callback[ONM_MSG_MAX];
  struct onm_api_callback cb;


  /* Async connection. */
  struct onm_client_handler *async;

  /* Disconnect callback. */
  ONM_DISCONNECT_CALLBACK disconnect_callback;

  /* Reconnect thread. */
  struct thread *t_connect;

  /* Reconnect interval in seconds. */
  int reconnect_interval;

  /* Debug message flag. */
  int debug;

  /* COMMMSG recv callback. */
  onm_msg_commsg_recv_cb_t  nc_commsg_recv_cb;
  void                     *nc_commsg_user_ref;
};


#ifndef ONM_SERV_PATH
#define ONM_SERV_PATH "/tmp/.onmserv"
#endif /* ONM_SERV_PATH */

struct onm_client *onm_client_create (struct lib_globals *, u_int32_t);
int onm_client_delete (struct onm_client *);
void onm_client_stop (struct onm_client *);
void onm_client_id_set (struct onm_client *, struct onm_msg_service *);
int onm_client_set_service (struct onm_client *, int);
void onm_client_set_version (struct onm_client *, u_int16_t);
void onm_client_set_protocol (struct onm_client *, u_int32_t);
void
onm_client_set_client_id (struct onm_client *ac, u_int32_t client_id);
void onm_client_set_callback (struct onm_client *, int, ONM_CALLBACK);
void onm_client_set_disconnect_callback (struct onm_client *,
                                         ONM_DISCONNECT_CALLBACK);

int onm_client_send_message (struct onm_client_handler *, u_int32_t,
                             int, u_int16_t, u_int32_t *);

void onm_client_pending_message (struct onm_client_handler *,
                                 struct onm_api_msg_header *);

int onm_client_reconnect (struct thread *);
int onm_client_reconnect_start (struct onm_client *);
int onm_client_reconnect_async (struct thread *);
int onm_client_start (struct onm_client *);

int onm_encode_service (u_char **, u_int16_t *, struct onm_msg_service *);
int onm_encode_evc (u_char **, u_int16_t *, struct onm_msg_evc_status *);

int onm_parse_service (u_char **, u_int16_t *, struct onm_api_msg_header *,
                       void *, ONM_CALLBACK);

#endif /*_ONM_CLIENT_H */
