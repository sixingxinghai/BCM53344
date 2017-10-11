/*=============================================================================
**
** Copyright (C) 2002-2003   All Rights Reserved.
**
** pal.h -- PacOS PAL common definitions
**          for Linux
*/
#ifndef _PAL_H
#define _PAL_H

/*-----------------------------------------------------------------------------
**
** Include files
*/

/* System configuration */
#ifndef APS_TOOLKIT
#include "config.h"
#include "plat_incl.h"
#include "pal_types.h"
#include "pal_if_types.h"
#include "pal_if_stats.h"
#include "pal_if_default.h"
#include "pal_posix.h"
#include "pal_assert.h"
#include "pal_memory.h"
#include "pal_file.h"
#include "pal_daemon.h"
#include "pal_debug.h"
#include "pal_stdlib.h"
#include "pal_memory.h"
#include "pal_string.h"
#include "pal_log.h"
#include "pal_time.h"
#include "pal_math.h"
#include "pal_regex.h"
#include "pal_socket.h"
#include "pal_sock_raw.h"
#include "pal_sock_ll.h"
#include "pal_sock_udp.h"
#include "pal_inet.h"
#include "pal_np.h"
#include "pal_pipe.h"
#include "pal_term.h"
#include "pal_signal.h"
#include "pal_kernel.h"
#include "pal_arp.h"
#ifdef HAVE_VRRP
#include "plat_vrrp.h"
#endif /* HAVE_VRRP */
#ifdef HAVE_IPV6
#include "pal_icmp6.h"
#endif /* HAVE_IPV6 */
#ifdef HAVE_TUNNEL
#include "pal_tunnel.h"
#endif /* HAVE_TUNNEL */
#if defined HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP
#include "pal_mcast.h"
#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP */
#if defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
#include "pal_mcast6.h"
#include "pal_mld.h"
#endif /* HAVE_MCAST_IPV6 || HAVE_MLD_SNOOP */
#ifdef HAVE_MPLS_FWD
#include "pal_mpls_client.h"
#include "pal_mpls_stats.h"
#include "pal_mpls_tn_stats.h"
#endif /* HAVE_MPLS_FWD */
#ifdef HAVE_IMISH
#include "pal_syslog.h"
#endif /* HAVE_IMISH */

#else  /* For APS Toolkit customer user. */
#include "plat_incl.h"
#include "pal_types.h"
#include "pal_time.h"
#endif /* APS_TOOLKIT */

#ifdef HAVE_CRX
#include "imi/pal_pm.h"
#endif /* HAVE_CRX */

#ifdef HAVE_VLOGD
#include "pal_vlog.h"
#endif

#ifndef SPEED_10000
#define SPEED_10000 10000
#endif /* ! SPEED_10000 */
/*-----------------------------------------------------------------------------
**
** Constants and enumerations
*/

/*-----------------------------------------------------------------------------
**
** Types
*/
typedef unsigned long int u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

/*-----------------------------------------------------------------------------
**
** Functions
*/

/*-----------------------------------------------------------------------------
**
** Done
*/
#endif /* _PAL_H */
