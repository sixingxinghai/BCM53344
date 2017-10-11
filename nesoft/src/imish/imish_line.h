/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _PACOS_IMISH_LINE_H
#define _PACOS_IMISH_LINE_H

int imish_mode_get (struct line *);
u_char imish_privilege_get (struct line *);

void imish_line_read (struct line *);
void imish_line_send (struct line *, const char *, ...);
void imish_line_send_cli (struct line *, struct cli *);
void imish_line_print (struct line *);
void imish_line_execute (struct line *);
void imish_line_config (struct line *);
void imish_line_end (struct line *);
void imish_line_close (struct line *);
void imish_line_sync (struct line *);
void imish_line_sync_vrf (struct line *, char *);
void imish_line_init (struct lib_globals *, char *);

#endif /* _PACOS_IMISH_LINE_H */
