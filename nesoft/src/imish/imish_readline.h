/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _IMISH_READLINE_H
#define _IMISH_READLINE_H

/* Prototypes.  */

char *imish_gets (char *prompt);
char *imish_get_y_or_n (char *prompt);
void imish_reset_terminal (int mode);
void imish_readline_init ();
void imish_history_show ();
void imish_advance_set ();
void imish_advance_unset ();

#endif /* _IMISH_READLINE_H */
