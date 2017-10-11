/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_SERVER_RIB_H
#define _NSM_SERVER_RIB_H

#include "nsm_message.h"
#include "nsm_table.h"
#include "nsm_vrf.h"

int nsm_send_ipv4_add (struct nsm_server_entry *, u_int32_t, struct prefix *,
                       u_int16_t, struct rib *, struct nsm_vrf *,
                       int, struct ftn_entry *);
int nsm_send_ipv4_delete (struct nsm_server_entry *, struct prefix *,
                          u_int16_t, struct rib *,
                          struct nsm_vrf *, int);
#ifdef HAVE_IPV6
int nsm_send_ipv6_add (struct nsm_server_entry *, u_int32_t, struct prefix *,
                       u_int16_t, struct rib *,
                       struct nsm_vrf *, int, struct ftn_entry *);
int nsm_send_ipv6_delete (struct nsm_server_entry *, struct prefix *,
                          u_int16_t, struct rib *, struct nsm_vrf *, int);
#endif /* HAVE_IPV6 */

int nsm_send_ipv4_add_override (struct nsm_server_entry *, u_int32_t, struct prefix *,
                       u_int16_t, struct rib *, struct nsm_vrf *,
                       int, struct ftn_entry *);
#ifdef HAVE_IPV6
int nsm_send_ipv6_add_override (struct nsm_server_entry *, u_int32_t, struct prefix *,
                       u_int16_t, struct rib *,
                       struct nsm_vrf *, int, struct ftn_entry *);
#endif /* HAVE_IPV6 */


int nsm_server_recv_route_ipv4 (struct nsm_msg_header *, void *, void *);

int nsm_server_recv_ipv4_nexthop_exact_lookup (struct nsm_msg_header *, void *,
                                               void *);
int nsm_server_recv_ipv4_nexthop_best_lookup (struct nsm_msg_header *, void *,
                                              void *);
 
#ifdef HAVE_IPV6
int nsm_server_recv_route_ipv6 (struct nsm_msg_header *, void *, void *);
int nsm_server_recv_ipv6_nexthop_exact_lookup (struct nsm_msg_header *, void *,
                                               void *);
int nsm_server_recv_ipv6_nexthop_best_lookup (struct nsm_msg_header *, void *,
                                              void *);
#endif /* HAVE_IPV6 */
void nsm_rib_set_server_callback (struct nsm_server *ns);

int  nsm_server_nexthop_tracking (struct nsm_msg_header *,
                                  void *, void *);
                                  
#endif /* _NSM_SERVER_RIB_H */
