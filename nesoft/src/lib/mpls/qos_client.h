/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_QOS_CLIENT_H
#define _PACOS_QOS_CLIENT_H

int  qos_client_init_protocol (struct nsm_client_handler *,
                               u_int32_t,
                               struct route_table **);
void qos_client_deinit        (struct lib_globals *,
                               struct route_table **,
                               u_char);
int  qos_client_release       (struct nsm_client *,
                               struct lib_globals *,
                               struct route_table *,
                               u_int32_t,
                               u_int32_t,
                               u_int32_t,
                               u_int8_t);
int  qos_client_release_slow  (struct nsm_client *,
                               struct lib_globals *,
                               struct route_table *,
                               struct qos_resource *,
                               u_char);
qos_status qos_client_message (struct nsm_client *,
                               struct lib_globals *,
                               struct route_table *,
                               int,
                               struct qos_resource **);
qos_status qos_client_message_nb (struct nsm_client *,
                                  struct lib_globals *,
                                  struct route_table *,
                                  int,
                                  struct qos_resource *,
                                  u_int32_t *);
int  qos_client_clean         (struct nsm_client *,
                               struct lib_globals *, 
                               u_int32_t,
                               u_int32_t);
#ifdef HAVE_GMPLS
int  gmpls_qos_client_init_protocol (struct nsm_client_handler *,
                                     u_int32_t,
                                     struct route_table **);
int  gmpls_qos_client_release       (struct nsm_client *,
                                     struct lib_globals *,
                                     struct route_table *,
                                     u_int32_t,
                                     u_int32_t,
                                     u_int32_t,
                                     u_int32_t,
                                     u_int8_t);
int  gmpls_qos_client_clean         (struct nsm_client *,
                                     struct lib_globals *, 
                                     u_int32_t,
                                     u_int32_t,
                                     u_int32_t);
#endif /*HAVE_GMPLS*/

#endif /* _PACOS_QOS_CLIENT_H */
