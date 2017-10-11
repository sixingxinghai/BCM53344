/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _VTYSH_PM_H
#define _VTYSH_PM_H

/* Protocol module connection manager.  */
struct vtysh_pm_master
{
  vector pm;
};

/* Protocol module connection manager.  */
struct vtysh_pm
{
  /* Name of the protocol.  */
  char *name;

  /* Ponter to the master.  */
  struct vtysh_pm_master *pmaster;

#ifdef HAVE_TCP_MESSAGE
  /* "line" port. */
  u_int16_t port;
#else /* HAVE_TCP_MESSAGE */
  /* "line" path.  */
  char *path;
#endif /* HAVE_TCP_MESSAGE */

  /* Boolean value of the connection.  */
  bool_t connected;

  /* Boolean value that this protocol module is invoked by IMI or
     not.  */
  bool_t launched;

  /* Socket of the connection.  */
  pal_sock_handle_t sock;

  /* Status.  */
  u_char status;
#define VTYSH_PM_STATUS_NOT_RUNNING         0
#define VTYSH_PM_STATUS_PRE_LAUNCHED        1
#define VTYSH_PM_STATUS_LAUNCHED            2
#define VTYSH_PM_STATUS_RESTARTED           3
};

struct vtysh_pm *vtysh_pm_lookup (struct vtysh_pm_master *pmaster, int index);
int vtysh_pm_line_send (struct vtysh_pm *, char *, int, vrf_id_t);
void vtysh_pm_init ();
void vtysh_pm_close (struct vtysh_pm *pm);

extern struct vtysh_pm_master *vtysh_pmaster;

#endif /* _VTYSH_PM_H */
