/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_MPLS_FWD_H
#define _NSM_MPLS_FWD_H

int nsm_mpls_ftn4_add_to_fwd (struct nsm_master *, struct ftn_entry *,
                             struct prefix *, u_int32_t, struct ftn_entry *, 
                             int);
int nsm_mpls_ilm_add_to_fwd (struct nsm_master *, struct ilm_entry *, 
                             struct ftn_entry *);

#ifdef HAVE_IPV6
int nsm_mpls_ftn6_add_to_fwd (struct nsm_master *, struct ftn_entry *,
                              struct prefix *, u_int32_t, struct ftn_entry *,
                              int);

int nsm_mpls_ftn6_del_from_fwd (struct nsm_master *, struct ftn_entry *, 
                               struct prefix *, u_int32_t, struct ftn_entry *);
#endif

int nsm_mpls_ftn4_del_from_fwd (struct nsm_master *, struct ftn_entry *, 
                               struct prefix *, u_int32_t, struct ftn_entry *);
int nsm_mpls_ilm_del_from_fwd (struct nsm_master *, struct ilm_entry *);

#ifdef HAVE_MPLS_VC
int nsm_mpls_vc_fib_add (struct nsm_master *, struct nsm_mpls_circuit *,
                         u_int32_t, u_char, struct ftn_entry *);
int nsm_mpls_vc_fib_del (struct nsm_master *, struct nsm_mpls_circuit *,
                         u_int32_t);
#endif /* HAVE_MPLS_VC */
#endif /* _NSM_MPLS_FWD_H */
