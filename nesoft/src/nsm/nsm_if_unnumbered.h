/* Copyright (C) 2005  All Rights Reserved.  */

#ifndef _PACOS_NSM_IF_UNNUMBERED_H
#define _PACOS_NSM_IF_UNNUMBERED_H

int nsm_if_ipv4_unnumbered_set (u_int32_t, char *, char *,
                                struct prefix_ipv4 *);
int nsm_if_ipv4_unnumbered_unset (u_int32_t, char *);
int nsm_if_ipv4_unnumbered_update (struct interface *, struct prefix *);
#ifdef HAVE_IPV6
int nsm_if_ipv6_unnumbered_set (u_int32_t, char *, char *);
int nsm_if_ipv6_unnumbered_unset (u_int32_t, char *);
int nsm_if_ipv6_unnumbered_update (struct interface *);
#endif /* HAVE_IPV6 */

void nsm_if_unnumbered_show (struct cli *, struct interface *);
void nsm_if_ipv4_unnumbered_config_write (struct cli *, struct interface *);
#ifdef HAVE_IPV6
void nsm_if_ipv6_unnumbered_config_write (struct cli *, struct interface *);
#endif /* HAVE_IPV6 */
void nsm_if_unnumbered_cli_init (struct cli_tree *);

void nsm_if_unnumbered_init (struct interface *);
void nsm_if_unnumbered_finish (struct interface *, u_int32_t vr_id);

#endif /* _PACOS_NSM_IF_UNNUMBERED_H */
