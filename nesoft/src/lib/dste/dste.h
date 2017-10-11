/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_DSTE_H
#define _PACOS_DSTE_H

typedef enum bc_mode
  {
    BC_MODE_MAM = 0, /* Max Alloc. default value */
    BC_MODE_RSDL,    /* Russian Dolls */
    BC_MODE_MAR      /* Max Alloc. with Resv. */
  } bc_mode_t;

/* Prototype */

struct nsm_master;

#define MAX_CT_NUM              8
#define CT_NUM_INVALID          MAX_CT_NUM

#define MAX_TE_CLASSES         8
#define TE_CLASS_INVALID       MAX_TE_CLASSES

#define MAX_CT_NAME_LEN         20

#define IS_VALID_CT_NUM(ct) \
((ct) >= 0 && (ct) < MAX_CT_NUM) ? 1 : 0

/* TE Class structure */
struct te_class_s
{
  /* Class type */
  u_char ct_num;

  /* Preemption priority p */
  u_char priority;
};

/* Messaging structure. */
struct nsm_msg_dste_te_class_update
{
  u_char te_class_index;
  struct te_class_s te_class;
  char ct_name[MAX_CT_NAME_LEN + 1];  
};

#endif /* _PACOS_MPLS_H */
