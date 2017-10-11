/* Copyright (C) 2003,   All Rights Reserved. */

#ifndef _PAL_PM_H
#define _PAL_PM_H

#ifdef HAVE_SPLAT
#define PACOS_BIN_PATH             "/var/opt/OPSEC/iAPN/PacOS-SRS/sbin"
#else
#define PACOS_BIN_PATH             "/usr/local/sbin"
#endif /* HAVE_SPLAT. */

int pal_imi_stop_pm (module_id_t protocol);
int pal_imi_start_pm (module_id_t protocol);
int pal_get_pid (module_id_t protocol);
int pal_imi_pid_exists (int pid);

#endif /* _PAL_PM_H */
