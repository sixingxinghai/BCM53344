/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_ADMIN_GRP_H
#define _PACOS_ADMIN_GRP_H

#include "lib.h"

#ifdef HAVE_TE

/* Maximum size of an admin group name */
#define ADMIN_GROUP_NAMSIZ 15

/* Maximum number of configurable admin groups */
#define ADMIN_GROUP_MAX    32

/* Invalid admin group test */
#define ADMIN_GROUP_UNDEF  -1
#define ADMIN_GROUP_INVALID(g)    ((g) == 0)

struct admin_group
{
  char name [ADMIN_GROUP_NAMSIZ];
  int val;
};

/* Error values */
#define ADMIN_GROUP_ERR_DUPLICATE -1
#define ADMIN_GROUP_ERR_EEXIST    -2
/* End Error values */


/* Prototypes */
void admin_group_array_init (struct admin_group []);
result_t admin_group_add (struct admin_group [], char *, int);
result_t admin_group_del (struct admin_group [], char *, int);
int admin_group_get_val (struct admin_group [], char *);
char *admin_group_get_name (struct admin_group [], int);
void admin_group_pad_with_spaces (char [], char *, int);
void admin_group_dump (struct cli *, struct admin_group []);
/* End Prototypes */

#endif /* #ifdef HAVE_TE */

#endif /* #define _PACOS_ADMIN_GRP_H */
