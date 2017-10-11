/* Copyright (C) 2004   All Rights Reserved. */

#ifndef __BCM_INCL_H
#define __BCM_INCL_H

/* BCM includes. */
#include <sal/core/alloc.h>
#include <sal/core/libc.h>
#include <sal/core/thread.h>
#include <sal/core/spl.h>
#include <sal/appl/io.h>

#include <appl/cpudb/cpudb.h>
#include <appl/stktask/attach.h>
#include <appl/cputrans/cputrans.h>
#include <appl/cputrans/atp.h>

#include <bcm/types.h>
//#include <bcm/igmp.h>
#include <bcm/port.h>
#include <bcm/pkt.h>
#include <bcm/stack.h>

#include <bcmx/bcmx.h>
#include <bcmx/bcmx_int.h>
#include <bcmx/custom.h>
#include <bcmx/ipmc.h>
#include <bcmx/lplist.h>
#include <bcmx/mirror.h>
#include <bcmx/stat.h>
#include <bcmx/tx.h>
#include <bcmx/auth.h>
#include <bcmx/debug.h>
#include <bcmx/l2.h>
#include <bcmx/lport.h>
#include <bcmx/port.h>
#include <bcmx/stg.h>
#include <bcmx/vlan.h>
#include <bcmx/bcmx.h>
//#include <bcmx/diffserv.h>
#include <bcmx/l3.h>
#include <bcmx/mcast.h>
#include <bcmx/rate.h>
#include <bcmx/switch.h>
#include <bcmx/cosq.h>
//#include <bcmx/filter.h>
#include <bcmx/link.h>
//#include <bcmx/meter.h>
#include <bcmx/rx.h>
#include <bcmx/vlan.h>
#include <bcmx/trunk.h>
#include <bcmx/async.h>
#ifdef HAVE_MPLS
#include <bcmx/mpls.h>
#endif /* HAVE_MPLS */

#include <bcmx/tunnel.h>

#include <soc/enet.h>
#include <soc/drv.h>

/*
  Default number of ports if not configured.
*/
#define BCM_CONFIG_MAX_FE             200
#define BCM_CONFIG_MAX_GE             50
#define BCM_CONFIG_MAX_XE             24

#define HSL_DEFAULT_FDB_TIMEOUT       300

#endif /* __BCM_INCL_H */
