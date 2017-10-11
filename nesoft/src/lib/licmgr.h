/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_LICMGR_H
#define _PACOS_LICMGR_H

#define ERR_NOPTR    "No product specified to initialize license manager library.\n"

#define ERR_INIT     "Unable to initialize license manager library.\n"

#define ERR_NOLIC    "Unable to obtain license manager library.\n"

#define ERR_RENLIC   "Unable to renew license.\n"

#define LIC_MGR_CHECK_INTERVAL 86400 

int lic_mgr_get(char **, int,  char *, struct lib_globals *);
int lic_mgr_update(struct thread *) ;
int lic_feature_get(char *, char *, struct lib_globals *);
int nsm_lic_mgr_get(char **, int, char *, struct nsm_client_lic *);
int nsm_lic_mgr_update(struct thread *t);
void lic_mgr_finish (lic_mgr_handle_t);

#endif /* _PACOS_LICMGR_H */ 
