/* Copyright (C) 2001-2004  All Rights Reserved.  */

#ifndef _PACOS_NSM_IF_ARBITER_H
#define _PACOS_NSM_IF_ARBITER_H

/* Default Interface Arbiter interval. (sec)  */
#define NSM_IF_ARBITER_INTERVAL_DEFAULT         20

/* Prototypes.  */
void nsm_if_arbiter_config_write (struct cli *);
void nsm_if_arbiter_init (struct lib_globals *);
s_int32_t nsm_if_arbiter (struct thread *t);

#endif /* _PACOS_NSM_IF_ARBITER_H */

