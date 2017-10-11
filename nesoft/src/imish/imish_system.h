/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _IMISH_SYSTEM_H
#define _IMISH_SYSTEM_H

/* System parameter values set and get APIs.  */

/* System values.  */
struct imi_system
{
  /* User id.  */
  uid_t uid;

  /* Process ID.  */
  pid_t pid;

  /* FIB ID.  */
  fib_id_t fib_id;

  /* TTY.  */
  char *tty;

  /* Timeout value.  */
  int timeout;

  /* Hostname.  */
  char *hostname;
};

/* Default time out is 10 minites.  */
#define IMISH_TIMEOUT_DEFAULT              600

int imish_timeout_get ();
void imish_timeout_set (int timeout);

int imish_pid_get ();

int imish_ttyidle ();

void imish_hostname_set (char *hostname);
void imish_hostname_unset ();

void imish_fib_id_set (fib_id_t);

void imish_exit (int code);

void imish_system_init ();

char *imish_prompt ();

#endif /* _IMISH_SYSTEM_H */
