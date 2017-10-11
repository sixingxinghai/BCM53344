/* Copyright (C)  2002-2003  All Rights Reserved. */

#ifndef _NSM_ROUTER_H
#define _NSM_ROUTER_H

#include "nsm_server.h"

#define NSM_ROUTER_ID_UPDATE_DELAY      1

#ifndef HAVE_HA
/* Macro. */
#define ROUTER_ID_UPDATE_TIMER_ADD(N)                                         \
    do {                                                                      \
      struct nsm_vrf *_nv = (N);                                              \
      THREAD_TIMER_ON (nzg, _nv->t_router_id, nsm_router_id_update_timer,     \
                       _nv, NSM_ROUTER_ID_UPDATE_DELAY);                      \
    } while (0)
#else
/* Macro. */
#define ROUTER_ID_UPDATE_TIMER_ADD(N)                                         \
    do {                                                                      \
      struct nsm_vrf *_nv = (N);                                              \
      if (_nv->t_router_id == NULL)                                           \
        nsm_cal_create_rtr_id_timer (_nv);                                    \
                                                                              \
      THREAD_TIMER_ON (nzg, _nv->t_router_id, nsm_router_id_update_timer,     \
                       _nv, NSM_ROUTER_ID_UPDATE_DELAY);                      \
    } while (0)
#endif /* HAVE_HA */

/* Prototypes. */
void nsm_set_router_id_by_index (vrf_id_t);
void nsm_router_id_update (struct nsm_vrf *);
int nsm_router_id_update_timer (struct thread *);
struct nsm_master *nsm_master_init (struct apn_vr *);
void nsm_master_finish (struct apn_vr *);
struct nsm_master *nsm_master_lookup_by_id (struct lib_globals *, u_int32_t);
struct nsm_master *nsm_master_lookup_by_name (struct lib_globals *, char *);

#endif /* _NSM_ROUTER_H */
