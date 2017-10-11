/*=============================================================================
**
** Copyright (C) 2002-2003   All Rights Reserved.
**
** pal_np.h -- PacOS PAL NP forwarding and interface control definitions
**             for Linux
*/
#ifdef HAVE_NP
#ifndef _PAL_NP_H
#define _PAL_NP_H

/*-----------------------------------------------------------------------------
**
** Include files
*/
#include "pal.h"
#include "lib.h"
#include "nsm/nsmd.h"
#include "prefix.h"
#include "rib/rib.h"
#include "if.h"

/*-----------------------------------------------------------------------------
**
** Constants and enumerations
*/

/*-----------------------------------------------------------------------------
**
** Types
*/

/*
** Handle for interfaces.  This is used by the NP code to refer to an
** interface.  It is stored in the interface structure.  It's best if
** this can be handled as an atomic entity (similar to integer).
*/
typedef void *if_handle;

/*-----------------------------------------------------------------------------
**
** Functions
*/

#include "pal_np.def"

/*-----------------------------------------------------------------------------
**
** Done
*/
#endif /* def _PAL_NP_H */
#endif /* def HAVE_NP */
