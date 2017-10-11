/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _VTYSH_H
#define _VTYSH_H

void vtysh_start ();
void vtysh_stop ();

#define VTYSH_STRCMP(S)   (pal_strncmp (line, S, pal_strlen (S)) == 0)

/* Globals structure.  */
extern struct lib_globals *vtyshm;

/* Configuration file name.  */
extern char *config_file;

#endif /* _VTYSH_H */
