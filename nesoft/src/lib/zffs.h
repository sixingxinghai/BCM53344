/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_FFS_H
#define _PACOS_FFS_H

#ifdef HAVE_FFS
#undef  zffs
#define zffs ffs
#else
int     zffs (u_int32_t);
#endif /* HAVE_FFS */

#endif /* _PACOS_FFS_H */
