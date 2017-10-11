/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_RT_H
#define _NSM_RT_H

result_t kernel_add_route (struct prefix_ipv4 *, struct pal_in4_addr *, 
                           s_int32_t, s_int32_t);

#ifdef HAVE_IPV6
result_t kernel_address_add_ipv6 (struct interface *, struct connected *);
result_t kernel_address_delete_ipv6(struct interface *, struct connected *);
#endif /* HAVE_IPV6 */

#endif /* _NSM_RT_H */
