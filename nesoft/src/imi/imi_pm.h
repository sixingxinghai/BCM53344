/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_IMI_PM_H
#define _PACOS_IMI_PM_H

#ifdef HAVE_CRX
int imi_pm_set_pid (module_id_t, int);
int imi_pm_start (module_id_t);
int imi_pm_stop (module_id_t);
#endif /* HAVE_CRX */

#endif /* _PACOS_IMI_PM_H */
