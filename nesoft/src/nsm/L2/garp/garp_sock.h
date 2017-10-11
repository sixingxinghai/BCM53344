/* Copyright (C) 2003  All Rights Reserved. */

/* This module declares the interface to the GARP SOCK implementation. */
#ifndef __GARP_SOCK_H__
#define __GARP_SOCK_H__

#ifdef HAVE_USER_HSL

#define GARP_ASYNC            1
/*
   HSL transport socket structure.
*/
struct garp_client
{
  struct message_handler *mc;
};

int garp_client_create (struct lib_globals *zg, struct garp_client *nl,
                        u_char async);
int
garp_client_connect (struct message_handler *mc, struct message_entry *me,
                     pal_sock_handle_t sock);
int
garp_client_disconnect (struct message_handler *mc, struct message_entry *me,
                        pal_sock_handle_t sock);
#endif /* HAVE_USER_HSL */


s_int32_t garp_sock_init (struct lib_globals *zg);
s_int32_t garp_sock_deinit (struct lib_globals *zg);
s_int32_t garp_send (const char * const brname, u_char * src_addr,
                     const u_char *dest_addr, const u_int32_t ifindex,
                     unsigned char *data, int length, u_int16_t vid);
#ifndef HAVE_USER_HSL
s_int32_t garp_recv (struct thread *thread);
bool_t garp_ethertype_parse (unsigned char * frame);
#else
s_int32_t garp_recv (struct message_handler *mc, struct message_entry *me,
           pal_sock_handle_t sock);
#endif /* HAVE_USER_HSL */

#endif
