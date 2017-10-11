/* Copyright 2003  All Rights Reserved.  */

#ifndef _HSL_COMM_H_
#define _HSL_COMM_H_

/* Structure to hold the NL control per socket. */
struct hsl_sock
{
  u_int32_t groups;       /* Multicast groups. */
  u_int32_t pid;          /* PID. */
  struct sock *sk;        /* Pointer to the sock structure. */
  struct hsl_sock *next;  /* Next pointer. */
};

/* function prototypes. */
int hsl_sock_init (void);
int hsl_sock_deinit (void);
int hsl_sock_process_msg (struct socket *sock, char *buf, int buflen);
int hsl_sock_process_msg (struct socket *sock, char *buf, int buflen);
int hsl_sock_post_buffer (struct socket *sock, char *buf, int size);
int hsl_sock_post_msg (struct socket *sock, int cmd, int flags, int seqno, char *buf, int size);
int hsl_sock_post_ack (struct socket *sock, struct hal_nlmsghdr *hdr, int flags, int error);
int hsl_sock_release (struct socket *sock);

#endif /* _HSL_COMM_H_ */

