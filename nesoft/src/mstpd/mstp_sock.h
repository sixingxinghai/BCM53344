/* Copyright (C) 2003  All Rights Reserved. */

/* This module declares the interface to the STP SOCK implementation. */
#ifndef __MSTP_SOCK_H__
#define __MSTP_SOCK_H__

#include "mstp_types.h"

extern pal_sock_handle_t
mstp_sock_init (struct lib_globals *zg);

#ifndef HAVE_USER_HSL

extern int
mstp_send (const char * const brname,u_char * src_addr,const char *dest_addr,
           const u_int16_t vid, const u_int32_t ifindex , unsigned char *data,
           int length);

extern int
mstp_recv (struct thread *thread);
#else
int
mstp_send (const char * const brname,char * src_addr, const char *dest_addr,
           const u_int16_t vid, const u_int32_t ifindex , unsigned char *data, int length);

#endif /* HAVE_USER_HSL */

#ifdef HAVE_USER_HSL

#define MSTP_MESSAGE_MAX_LEN   4096
#define MSTP_ASYNC             1

/*
   HSL transport socket structure.
*/
struct mstp_client
{
  struct message_handler *mc;
};

int mstp_client_create (struct lib_globals *zg, struct mstp_client *nl,
                        u_char async);
#endif /* HAVE_USER_HSL */ 
#endif
