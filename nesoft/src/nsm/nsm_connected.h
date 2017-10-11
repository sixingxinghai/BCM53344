/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NSM_CONNECTED_H
#define _PACOS_NSM_CONNECTED_H

#include "pal.h"

struct connected *nsm_connected_check_ipv4 (struct interface *,
                                            struct prefix *);
struct connected *nsm_connected_real_ipv4 (struct interface *,
                                           struct prefix *);
/*Added to fix compilation*/
struct connected *
nsm_connected_add_ipv4 (struct interface *ifp, u_char flags,
                        struct pal_in4_addr *addr, u_char prefixlen,
                        struct pal_in4_addr *broad);

void nsm_connected_delete_ipv4 (struct interface *, int, struct pal_in4_addr *,
                                int, struct pal_in4_addr *);
void nsm_connected_up_ipv4 (struct interface *, struct connected *);
void nsm_connected_down_ipv4 (struct interface *, struct connected *);

#ifdef HAVE_IPV6
struct connected *nsm_connected_check_ipv6 (struct interface *,
                                            struct prefix *);
struct connected *nsm_connected_real_ipv6 (struct interface *,
                                           struct prefix *);
void nsm_connected_up_ipv6 (struct interface *, struct connected *);
void nsm_connected_down_ipv6 (struct interface *, struct connected *);
void nsm_connected_valid_ipv6 (struct interface *, struct connected *);
void nsm_connected_invalid_ipv6 (struct interface *, struct connected *);
struct connected *nsm_connected_add_ipv6 (struct interface *,
                                          struct pal_in6_addr *, s_int32_t,
                                          struct pal_in6_addr *);
void nsm_connected_delete_ipv6 (struct interface *, struct pal_in6_addr *,
                                s_int32_t, struct pal_in6_addr *);
#endif /* HAVE_IPV6 */

#endif /* _PACOS_NSM_CONNECTED_H */
