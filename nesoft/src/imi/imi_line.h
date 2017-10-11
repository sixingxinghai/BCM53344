/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _PACOS_IMI_LINE_H
#define _PACOS_IMI_LINE_H

struct imi_server;
struct imi_master;

void imi_line_free (struct imi_master *, u_char, int);
struct line *imi_line_lookup (struct imi_master *, u_char, int);
struct imi_server_entry *imi_line_entry_lookup (struct imi_server *, int, int);
struct line *imi_line_get (struct imi_master *, u_char, int);
int imi_line_send (struct line *, const char *, ...);
void imi_line_init (struct imi_master *, u_char, int);
void imi_line_shutdown (struct imi_master *, u_char);

#endif /* _PACOS_IMI_LINE_H */
