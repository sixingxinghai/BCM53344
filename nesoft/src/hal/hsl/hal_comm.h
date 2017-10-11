/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_COMM_H_
#define _HAL_COMM_H_

#ifdef HAVE_USER_HSL
/*
   HSL transport socket structure.
*/
struct hal_client
{
  struct message_handler *mc;
  int seq;
  char *name;
};


/*
   Asynchronous messages.
*/
extern struct hal_client hallink;

/*
   HSL transport socket structure.
*/

/*
   Command channel.
*/
extern struct hal_client hallink_cmd;

/*
   if-arbiter command channel.
*/
extern struct hal_client hallink_poll;

/*
   Function prototypes.
*/
int hal_client_create (struct lib_globals *zg, struct hal_client *nl,
                       u_char async);
int hal_client_destroy (struct hal_client *s);
int hal_talk (struct hal_client *nl, struct hal_nlmsghdr *n,
              int (*filter) (struct hal_nlmsghdr *, void *), void *data);
int hal_comm_init (struct lib_globals *zg);
int hal_comm_deinit (struct lib_globals *zg);
int hal_msg_generic_request (int msg,
                             int (*filter) (struct hal_nlmsghdr *, void *),
                             void *data);
int hal_msg_generic_poll_request (int msg,
                                  int (*filter) (struct hal_nlmsghdr *,
                                  void *), void *data);
#else
#ifdef HAL_TCP_COMM
#include "hal_comm_tcp.h"
#else

/* 
   HSL transport socket structure. 
*/
struct halsock
{
  int sock;
  int seq;
  struct hal_sockaddr_nl snl;
  char *name;
  struct thread *t_read;
};

/* 
   Asynchronous messages. 
*/
extern struct halsock hallink; 

/* 
   Command channel. 
*/
extern struct halsock hallink_cmd;

/* 
   if-arbiter command channel.
*/
extern struct halsock hallink_poll;

/*
   Function prototypes.
*/
int hal_socket (struct halsock *nl, unsigned long groups, u_char non_block);
int hal_close (struct halsock *s);
int hal_talk (struct halsock *nl, struct hal_nlmsghdr *n, int (*filter) (struct hal_nlmsghdr *, void *), void *data);
int hal_comm_init (struct lib_globals *zg);
int hal_comm_deinit (struct lib_globals *zg);
int hal_msg_generic_request (int msg, int (*filter) (struct hal_nlmsghdr *, void *), void *data);
int hal_msg_generic_poll_request (int msg, int (*filter) (struct hal_nlmsghdr *, void *), void *data);

#endif /* HAL_TCP_COMM */
#endif /* HAVE_USER_HSL */
#endif /* _HAL_COMM_H */
