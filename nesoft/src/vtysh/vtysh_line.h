/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _VTYSH_LINE_H
#define _VTYSH_LINE_H

#include "line.h"

/* Connection to IMI. */
struct vtysh_line
{
  /* Length of the message.  */
  u_int16_t length;

  /* IMI message codes.  */
  u_char code;

  /* CLI Node. */
  u_char node;

  /* Privilege level. */
  u_char privilege;

  /* Reserved. */
  u_char reserved;

  /* CLI Key. */
  u_int16_t cli_key;

  /* Protocol module. */
  modbmap_t module;

  /* Current read pointer.  */
  u_int16_t cp;

  /* Buffer of the message.  */
  char buf[LINE_MESSAGE_MAX];

  /* Message pointer.  */
  char *message;

#ifdef HAVE_VR
  /* Current vrf_id. */
  u_int16_t vrf_id;
#endif /* HAVE_VR */
} imiline;

void vtysh_line_init ();
void vtysh_line_end ();
void vtysh_line_sync ();
void vtysh_line_send (int mode, const char * format, ...);
int vtysh_line_get_reply ();
#ifdef HAVE_TCP_MESSAGE
int vtysh_line_open (int port);
#else /* HAVE_TCP_MESSAGE */
int vtysh_line_open (char *path);
#endif /* HAVE_TCP_MESSAGE */
void vtysh_line_register (int sock);
void vtysh_line_print_response ();
int vtysh_line_write (pal_sock_handle_t sock, char *buf, u_int16_t length);
void vtysh_line_close ();

int vtysh_mode_get ();
u_char vtysh_privilege_get ();
void vtysh_privilege_set (int);
void vtysh_no_pager ();

#endif /* _VTYSH_LINE_H */
