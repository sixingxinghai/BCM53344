/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _VTYSH_READLINE_H
#define _VTYSH_READLINE_H

/* Prototypes.  */

char *vtysh_gets (char *prompt);
void vtysh_reset_terminal (int mode);
void vtysh_readline_init ();
void vtysh_history_show ();
void vtysh_advance_set ();
void vtysh_advance_unset ();

#endif /* _VTYSH_READLINE_H */
