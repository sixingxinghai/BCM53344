/**@file elmi_sock.h
  * @brief This file contains prototypes for ELMI SOCK implementation.
  */
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef __ELMI_SOCK_H__
#define __ELMI_SOCK_H__

pal_sock_handle_t 
elmi_sock_init (struct lib_globals *zg);

s_int32_t 
elmi_sock_deinit (struct lib_globals *zg);

int
elmi_frame_send (u_char *src_addr, const u_char *dest_addr,
                 u_int32_t ifindex , u_char *data, u_int32_t length);

s_int32_t 
elmi_send (const u_char * brname, u_char * src_addr,
           const u_char *dest_addr, const u_int32_t ifindex,
           u_char *data, u_int32_t length, u_int16_t vid);

s_int32_t 
elmi_frame_recv (struct thread *thread);

int
elmi_unin_handle_frame (struct lib_globals *zg, struct sockaddr_l2 *l2_skaddr,
                        u_char **pnt, s_int32_t *len);

int
elmi_unic_handle_frame (struct lib_globals *zg, struct sockaddr_l2 *l2_skaddr,
                        u_char **pnt, s_int32_t *len);

#endif /* __ELMI_SOCK_H__ */
