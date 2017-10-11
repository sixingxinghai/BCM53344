/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PAL_MCAST_H
#define _PAL_MCAST_H

#define _LINUX_IN_H
#include <linux/mroute.h>

#ifndef HAVE_IPNET

#undef pal_vifctl
#define pal_vifctl vifctl

#undef pal_mfcctl
#define pal_mfcctl mfcctl

#undef pal_mfcctl_v2
#define pal_mfcctl_v2 mfcctl_v2

#undef pal_sioc_sg_req
#define pal_sioc_sg_req sioc_sg_req

#undef pal_sioc_vif_req
#define pal_sioc_vif_req sioc_vif_req

#undef pal_igmpmsg
#define pal_igmpmsg igmpmsg

#undef  PAL_MAXVIFS
#define PAL_MAXVIFS               MAXVIFS

#undef PAL_SIOCGETSGCNT
#define PAL_SIOCGETSGCNT          SIOCGETSGCNT

#undef PAL_SIOCGETVIFCNT
#define PAL_SIOCGETVIFCNT         SIOCGETVIFCNT

#undef  PAL_MCAST_NOCACHE
#define PAL_MCAST_NOCACHE         IGMPMSG_NOCACHE

#undef  PAL_MCAST_WRONGVIF
#define PAL_MCAST_WRONGVIF        IGMPMSG_WRONGVIF

#undef  PAL_MCAST_WHOLEPKT
#define PAL_MCAST_WHOLEPKT        IGMPMSG_WHOLEPKT

#undef PAL_VIFF_REGISTER
#define PAL_VIFF_REGISTER VIFF_REGISTER

#undef PAL_VIFF_TUNNEL
#define PAL_VIFF_TUNNEL VIFF_TUNNEL

#define PAL_MCAST_FWD_PIM_CHKSUM_SUPPORT 0

#else /* HAVE_IPNET */

#undef pal_vifctl
#define pal_vifctl Ip_vifctl

#undef pal_mfcctl
#define pal_mfcctl Ip_mfcctl

#undef pal_mfcctl_v2
#define pal_mfcctl_v2 Ip_mfcctl_v2

#undef pal_sioc_sg_req
#define pal_sioc_sg_req Ip_sioc_sg_req

#undef pal_sioc_vif_req
#define pal_sioc_vif_req Ip_sioc_vif_req

#undef pal_igmpmsg
#define pal_igmpmsg Ip_igmpmsg

#undef  PAL_MAXVIFS
#define PAL_MAXVIFS               IP_MAXVIFS

#undef PAL_SIOCGETSGCNT
#define PAL_SIOCGETSGCNT          IP_SIOCGETSGCNT

#undef PAL_SIOCGETVIFCNT
#define PAL_SIOCGETVIFCNT         IP_SIOCGETVIFCNT

#undef  PAL_MCAST_NOCACHE
#define PAL_MCAST_NOCACHE         IP_IGMPMSG_NOCACHE

#undef  PAL_MCAST_WRONGVIF
#define PAL_MCAST_WRONGVIF        IP_IGMPMSG_WRONGVIF

#undef  PAL_MCAST_WHOLEPKT
#define PAL_MCAST_WHOLEPKT        IP_IGMPMSG_WHOLEPKT

#undef  PAL_VIFF_TUNNEL
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 50900)
#define PAL_VIFF_TUNNEL IPNET_VIFF_TUNNEL
#else  
#define PAL_VIFF_TUNNEL IP_VIFF_TUNNEL
#endif

#undef  PAL_VIFF_REGISTER
#if !defined(IPCOM_RELEASE) || (defined(IPCOM_RELEASE) && IPCOM_RELEASE < 50900)
#define PAL_VIFF_REGISTER       IPNET_VIFF_REGISTER
#else  
#define PAL_VIFF_REGISTER       IP_VIFF_REGISTER
#endif

#define PAL_MCAST_FWD_PIM_CHKSUM_SUPPORT 0

#endif /* HAVE_IPNET */

#include "pal_mcast.def"

#endif /* PAL_MCAST_H */
