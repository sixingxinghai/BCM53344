/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_BCM_AUTH_H_
#define _HSL_BCM_AUTH_H_

#ifdef HAVE_AUTHD

/* Function prototypes. */
int hsl_bcm_auth_init ();
int hsl_bcm_auth_deinit ();
int hsl_bcm_auth_set_port_state (u_int32_t port_ifindex, u_int32_t port_state);

#ifdef HAVE_MAC_AUTH
int hsl_bcm_auth_mac_set_port_state (u_int32_t port_ifindex,
                                     u_int32_t port_state);
#endif

#endif /* HAVE_AUTHD */

#endif /* _HSL_BCM_AUTH_H_ */
