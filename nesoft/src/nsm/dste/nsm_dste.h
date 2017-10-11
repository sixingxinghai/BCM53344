/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_DSTE_H
#define _NSM_DSTE_H

#ifdef HAVE_DSTE

/*Prototype*/

struct nsm_master;

int nsm_dste_get_class_type_num (struct nsm_master *, char *);
int nsm_dste_class_type_add (struct nsm_master *, int, char *);
int nsm_dste_class_type_delete (struct nsm_master *, int);
int nsm_dste_te_class_add (struct nsm_master *, u_char, int, u_char);
int nsm_dste_te_class_delete (struct nsm_master *, u_char);
int nsm_dste_bc_mode_update (char *, struct interface *);

void nsm_dste_te_class_update (struct nsm_master *, u_char);
void nsm_dste_send_bc_mode_update (struct interface *);

#endif /* HAVE_DSTE */

#endif /* _NSM_MPLS_H */
