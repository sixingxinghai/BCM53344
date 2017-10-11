/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_RIB_VRF_H
#define _NSM_RIB_VRF_H

struct apn_vr;
struct nsm_master;

int nsm_ip_vrf_set (struct apn_vr *, char *);
int nsm_ip_vrf_unset (struct apn_vr *, char *);

#endif /* _NSM_RIB_VRF_H */
