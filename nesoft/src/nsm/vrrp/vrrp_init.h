/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_VRRP_INIT_H
#define _PACOS_VRRP_INIT_H

void vrrp_init (struct nsm_master *);
void vrrp_close (struct nsm_master *);
void vrrp_close_all (struct lib_globals *);

#endif /* _PACOS_VRRP_INIT_H */
