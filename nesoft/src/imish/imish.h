/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _PACOS_IMISH_H
#define _PACOS_IMISH_H

/* Globals structure.  */
extern struct lib_globals *imishm;
extern u_int16_t imish_win_row_orig;
extern u_int16_t imish_win_row;
extern u_int16_t imish_win_col;
extern int suppress;

void imish_sigwinch (int);
void imish_start (void);
void imish_stop (void);

#endif /* _PACOS_IMISH_H */
