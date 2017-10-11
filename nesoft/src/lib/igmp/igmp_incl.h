/* Copyright (C) 2001-2005  All Rights Reserved. */

#ifndef _PACOS_IGMP_INCL_H
#define _PACOS_IGMP_INCL_H

/*********************************************************************/
/* FILE       : igmp_incl.h                                          */
/* PURPOSE    : This file contains ALL the Include Header files      */
/*              needed for *.c files in the 'lib/igmp' directory.    */
/*              This file SHOULD NOT be included in '*.c' files      */
/*              outside of  the 'lib/igmp' directory. Please include */
/*              'lib/igmp/igmp.h' file in all files outside of       */
/*              'lib/igmp/' directory.                               */
/*********************************************************************/

/*********************************************************************/
/* PAL and Library Include Files                                     */
/*********************************************************************/
#include "pal.h"
#include "pal_assert.h"
#include "lib.h"
#include "if.h"
#include "thread.h"
#include "table.h"
#include "cli.h"
#include "show.h"
#include "log.h"
#include "hash.h"
#include "avl_tree.h"
#include "prefix.h"
#include "snprintf.h"
#include "network.h"
#include "checksum.h"
#include "line.h"
#include "log.h"
#include "ptree.h"
#include "stream.h"
#include "timeutil.h"
#include "lib_mtrace.h"
#ifdef HAVE_SNMP
#include "snmp.h"
#include "asn1.h"
#endif /* HAVE_SNMP */
#include "nsm/rib/nsm_table.h"

/*********************************************************************/
/* External Modules Include Files                                    */
/*********************************************************************/

/*********************************************************************/
/* IGMP Include Files                                                */
/*********************************************************************/
#include "igmp_defines.h"
#include "igmp_macros.h"
#include "igmp_struct.h"
#include "igmp_api.h"
#include "igmp_func.h"

#endif /* _PACOS_IGMP_INCL_H */
