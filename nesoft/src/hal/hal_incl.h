/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_INCL_H_
#define _HAL_INCL_H_

#include "hal_types.h"
#include "hal_error.h"
#include "hal.h"
#include "hal_if.h"
#include "hal_pmirror.h"

#ifdef HAVE_L2
#include "L2/hal_l2.h"
#include "L2/hal_auth.h"
#include "L2/hal_bridge.h"
#include "L2/hal_flowctrl.h"
#include "L2/hal_igmp_snoop.h"
#ifdef HAVE_MLD_SNOOP
#include "L2/hal_mld_snoop.h"
#endif /* HAVE_MLD_SNOOP */
#include "L2/hal_ratelimit.h"
#include "L2/hal_l2_fdb.h"
#include "L2/hal_l2_qos.h"
#ifdef HAVE_LACPD
#include "L2/hal_lacp.h"
#endif /* HAVE_LACPD */
#include "L2/hal_vlan.h"
#include "L2/hal_vlan_stack.h"
#include "L2/hal_vlan_classifier.h"
#include "L2/hal_socket.h"
#endif /* HAVE_L2 */

#ifdef HAVE_L3
#include "L3/hal_l3.h"
#include "L3/hal_fib.h"
#include "L3/hal_ipv4_arp.h"
#include "L3/hal_ipv4_if.h"
#include "L3/hal_ipv4_uc.h"
#include "L3/hal_ipv4_mc.h"

#ifdef HAVE_PBR
#include "PBR/hal_pbr_ipv4_uc.h"
#endif /* HAVE_PBR */

#ifdef HAVE_IPV6
#include "L3/hal_ipv6_if.h"
#include "L3/hal_ipv6_uc.h"
#include "L3/hal_ipv6_mc.h"
#include "L3/hal_ipv6_nbr.h"
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
#include "hal_mpls_types.h"
#include "hal_mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_QOS
#include "hal_qos.h"
#endif /* HAVE_QOS */

#ifdef HAVE_PVLAN
#include "L2/hal_pvlan.h"
#endif /* HAVE_PVLAN */

#ifdef HAVE_ONMD
#include "L2/hal_oam.h"
#endif /* HAVE_ONMD */

#ifdef HAVE_DCB
#include "L2/hal_dcb.h"
#endif /* HAVE_DCB */

#include "hal_acl.h"

#endif /* _HAL_INCL_H_ */
