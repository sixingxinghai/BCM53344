/* Copyright (C) 2004   All Rights Reserved. */

#ifndef _HAL_NSM_H_
#define _HAL_NSM_H_

/* NSM initialization mode */
#define NSM_IF_INIT_MODE_L3        0
#define NSM_IF_INIT_MODE_L2        1

#if !(defined HAVE_L2) && defined HAVE_L3
#define  NSM_INIT_MODE_DEFAULT  (NSM_IF_INIT_MODE_L3)  
#endif

#if !(defined HAVE_L3) && defined HAVE_L2
#define  NSM_INIT_MODE_DEFAULT  (NSM_IF_INIT_MODE_L2)  
#endif

#if defined HAVE_L3 && defined HAVE_L2
#define  NSM_INIT_MODE_DEFAULT  (NSM_IF_INIT_MODE_L3)  
#endif

int hal_get_params_cb (struct hal_nlmsghdr *h, struct hal_msg_if *msg);
int hal_new_link_cb (struct hal_nlmsghdr *h, struct hal_msg_if *msg);
int hal_efm_dying_gasp_cb (struct hal_nlmsghdr *h);
int hal_del_link_cb (struct hal_msg_if *msg);
int hal_ecmp_cb (int ecmp);
#ifdef HAVE_L3
int hal_add_ipv4_addr(struct hal_msg_if_ipv4_addr *resp);
int hal_del_ipv4_addr(struct hal_msg_if_ipv4_addr *resp);
#ifdef HAVE_IPV6
int hal_add_ipv6_addr(struct hal_msg_if_ipv6_addr *resp);
int hal_del_ipv6_addr(struct hal_msg_if_ipv6_addr *resp);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */
#ifdef HAVE_L2
int hal_stp_refresh_link_cb(struct hal_msg_if *msg);
#endif /* HAVE_L2 */
void hal_nsm_lib_cb_register(struct hal_nsm_callbacks *cb);

#endif /* _HAL_NSM_H_ */
