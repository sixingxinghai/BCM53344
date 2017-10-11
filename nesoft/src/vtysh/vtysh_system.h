/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _VTYSH_SYSTEM_H
#define _VTYSH_SYSTEM_H

/* System parameter values set and get APIs.  */

/* System values.  */
struct vtysh_system
{
  /* User id.  */
  uid_t uid;

  /* Process ID.  */
  pid_t pid;

  /* TTY.  */
  char *tty;

  /* Timeout value.  */
  int timeout;

  /* Hostname.  */
  char *hostname;
};

/* Default time out is 10 minites.  */
#define VTYSH_TIMEOUT_DEFAULT              600

int vtysh_timeout_get ();
void vtysh_timeout_set (int timeout);

int vtysh_ttyidle ();

void vtysh_hostname_set (char *hostname);
void vtysh_hostname_unset ();

void vtysh_exit (int code);

void vtysh_system_init ();

char *vtysh_prompt ();

#endif /* _VTYSH_SYSTEM_H */
