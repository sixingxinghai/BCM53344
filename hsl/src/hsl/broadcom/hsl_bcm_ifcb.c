/* Copyright (C) 2004   All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

/* 
   Broadcom includes. 
*/
#include "bcm_incl.h"

/* 
   HAL includes.
*/
#include "hal_types.h"

#ifdef HAVE_L2
#include "hal_l2.h"
#endif /* HAVE_L2 */

#include "hal_msg.h"

#include "hsl_types.h"
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl_ifmgr.h"
#include "hsl_if_os.h"
#include "hsl_if_hw.h"
#include "hsl_ifmgr.h"

#include "hsl_bcm_if.h"
#include "hsl_bcm_ifmap.h"
#include "hsl_bcm_pkt.h"

static int cnt_ext_ports = 0;
/* 
   Port callback handling for 
   * Port attachment
   * Port detachment
   * Link Scan
*/

/* 
   Process port attachment message. 
*/
int
hsl_bcm_ifcb_port_attach (bcmx_lport_t lport, int unit, bcm_port_t port,
                          uint32 flags)
{
  char ifname[HSL_IFNAM_SIZE + 1];
  u_char mac[HSL_ETHER_ALEN];
  int ret, stat;
  unsigned long ifflags = 0;
  int speed, mtu;
  u_int32_t  duplex;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
 
  bcmx_lplist_t t, u;

  memset (ifname, 0, sizeof (ifname));
  /* Register with ifmap and get name and mac address for this port. */
  
  ret = hsl_bcm_ifmap_lport_register (lport, flags, ifname, mac);
  if (ret < 0)
    {
        if (ret != HSL_BCM_ERR_IFMAP_LPORT_ALREADY_REGISTERED )
        {
            HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Can't register lport(%d) in Broadcom interface map, err=%d\n", lport, ret);

        }
        else
        {
            HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Lport(%d) is already registered in BCM interface map.\n", lport);
        }
        return -1;
    }  

  /* Multicast flag. */
  ifflags |= IFF_MULTICAST;

  /* Broadcast flag. */
  ifflags |= IFF_BROADCAST;

  /* Administrative status. */
  ret = bcmx_port_enable_get (lport, &stat);

  if (ret == BCM_E_NONE)
    {
      if (stat)
        ifflags |= IFF_UP;
      else
        ifflags &= ~IFF_UP;
    }
  else
    ifflags &= ~IFF_UP;

  /* Operational status. */
  ret = bcmx_port_link_status_get (lport, &stat);
  if (ret == BCM_E_NONE)
    {
      if (stat)
        ifflags |= IFF_RUNNING;
      else
        ifflags &= ~IFF_RUNNING;
    }
  else
    ifflags &= ~IFF_RUNNING;

  /* Speed. */
  ret = bcmx_port_speed_get (lport, &speed);
  if (ret == BCM_E_NONE)
    {
      /* Change it to kbits/sec. */
      speed *= 1000;
    }
  else
    speed = 0;

  /* Duplex */
  hsl_bcm_get_port_duplex(lport, &duplex);

  /* MTU. */
  hsl_bcm_get_port_mtu (lport, &mtu);

  /* Set HW port MTU to account for 8021Q tagged frames */
  ret = hsl_bcm_set_port_mtu (lport, HSL_ETHER_MAX_LEN);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_WARN, "Interface %s hardware mtu %d set failed\n", ifname, HSL_ETHER_MAX_LEN);
    }

  bcmifp = hsl_bcm_if_alloc ();

  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Memory allocation failed for Broadcom interface\n");
      return -1;
    }

  bcmifp->u.l2.lport = lport;
  bcmifp->type = HSL_BCM_IF_TYPE_L2_ETHERNET;
  bcmifp->trunk_id = -1;

  if (unit)
    cnt_ext_ports++;

  /* Register this interface with the interface manager. */
  ret = hsl_ifmgr_L2_ethernet_register (ifname, mac, mtu, speed, duplex,
                                        ifflags, (void *) bcmifp, 0, &ifp);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Interface %s registration failed with HSL Interface Manager\n", ifname);
      return -1;
    }

  /* Set this port to disable by default.
  bcm_stg_t stg;
  bcmx_stg_default_get (&stg);
  bcmx_stg_stp_set (stg, lport, BCM_STG_STP_BLOCK);*/
  /*caiguoqing 2011-11-11, set according to service of the port*/

  /* Add this port to the default VLAN. */
  bcmx_lplist_init (&t, 1, 0);
  bcmx_lplist_init (&u, 1, 0);

  bcmx_lplist_add (&t, lport);
  bcmx_lplist_add (&u, lport);

  bcmx_vlan_port_add (HSL_DEFAULT_VID, t, u);

  bcmx_lplist_free (&t);
  bcmx_lplist_free (&u);

  /* Set port filtering mode for multicast packets. */
  bcmx_port_pfm_set (lport, BCM_PORT_MCAST_FLOOD_UNKNOWN);

  /* Set packet types to accept from this port. */
  hsl_ifmgr_unset_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);

  /* Set the uport for the lport. */
  hsl_bcm_ifmap_if_set (lport, ifp);

#ifdef HAVE_L3
#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6

  bcmx_ipmc_egress_port_set (lport, ifp->u.l2_ethernet.mac, HSL_FALSE,
                             HSL_DEFAULT_VID, 0);

#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */
#endif /* HAVE_L3 */

  return 0;
}

/* 
   Process port detachment message. 
*/
int
hsl_bcm_ifcb_port_detach (bcmx_lport_t lport, bcmx_uport_t uport)
{
  struct hsl_if *ifp;

  /* Get ifindex for the lport. */
  ifp = hsl_bcm_ifmap_if_get (lport);
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Lport (%d) not found in Interface Map\n", lport);
      return -1;
    }

#if 0
  if (unit)
    cnt_ext_ports--;
#endif

  HSL_IFMGR_IF_REF_DEC (ifp);

  /* Unregister from interface manager. */
  hsl_ifmgr_L2_ethernet_unregister (ifp);

  return 0;
}

/*
  Process link scan message. 
*/
int
hsl_bcm_ifcb_link_scan (bcmx_lport_t lport, bcm_port_info_t *info)
{
  unsigned long speed;
  unsigned long duplex;
  struct hsl_if *ifp;

  /* Get ifindex for the lport. */
  ifp = hsl_bcm_ifmap_if_get (lport);
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "LinkScanning notification for a port(lport) for which map doesn't exist\n");
      return -1;
    }

  /* Speed. */
  speed = info->speed * 1000;

  /* Duplex. */
  duplex = info->duplex;

  if (info->linkstatus)
    {
      /* Oper status. */
      hsl_ifmgr_L2_link_up (ifp, speed, duplex);
    }
  else 
    {
      /* Oper status. */
      hsl_ifmgr_L2_link_down (ifp, speed, duplex);
    }

  return 0;
}
