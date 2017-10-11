/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _PACOS_NSM_AUTH_H
#define _PACOS_NSM_AUTH_H

int nsm_auth_recv_port_state_msg (struct nsm_msg_header *, void *, void *);
int nsm_auth_set_server_callback (struct nsm_server *);

s_int32_t nsm_auth_mac_recv_port_state_msg (struct nsm_msg_header *, void *,
                                            void *);
s_int32_t nsm_auth_mac_recv_auth_status_msg (struct nsm_msg_header *, void *,
                                             void *);

#endif /* _PACOS_NSM_AUTH_H */
