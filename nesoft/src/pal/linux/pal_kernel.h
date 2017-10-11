/* Copyright (C) 2002-2003   All Rights Reserved. */

#ifndef _PAL_KERNEL_H
#define _PAL_KERNEL_H

/*  pal_kernel.h -- PacOS PAL kernel forwarding and interface control
                 defs for Linux.  */

/* Include files.  */
#include "lib.h"
#include "if.h"

#ifdef HAVE_VRRP
#include "stream.h"
#include "plat_vrrp.h"
#endif /* HAVE_VRRP */

/* Types.  */

/* Functions.  */

#include "pal_kernel.def"

#endif /*  _PAL_KERNEL_H */
