/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "hal_incl.h"
#include "hal_bridge.h"
#ifdef HAVE_USER_HSL
#include "hal/hsl/hal_netlink.h"
#include "hal/hsl/hal_msg.h"
#include "hal/hsl/L2/hal_stats_api.h"
#endif
#include "nsm_stats_api.h"


int
nsm_statistics_vlan_get (u_int32_t vlan_id,
                         struct hal_stats_vlan *vlan)
{
#ifdef HAVE_USER_HSL
  int ret;

  if ((ret = hal_statistics_vlan_get (vlan_id, vlan)) != HAL_SUCCESS)
    return ret;
#endif

  return 0;
}

int
nsm_statistics_port_get (u_int32_t port_id,
                              struct hal_stats_port *port)
{
#ifdef HAVE_USER_HSL
  int ret;

  if ((ret = hal_statistics_port_get (port_id, port)) != HAL_SUCCESS)
    return ret;
#endif

  return 0;
}

int
nsm_statistics_host_get (struct hal_stats_host *host)
{
#ifdef HAVE_USER_HSL
  int ret;

  if ((ret = hal_statistics_host_get (host)) != HAL_SUCCESS)
    return ret;
#endif

  return 0;
}

int
nsm_statistics_clear (u_int32_t port_id)
{
#ifdef HAVE_USER_HSL
  int ret;

  if ((ret = hal_statistics_clear (port_id)) != HAL_SUCCESS)
    return ret;
#endif

  return 0;
}

