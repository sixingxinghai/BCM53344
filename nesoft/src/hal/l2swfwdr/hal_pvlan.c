/* Copyright (C) 2004  All Rights Reserved.

LAYER 2 PVLAN HAL

This module defines the platform abstraction layer to the
Linux layer 2 PVLAN.

*/

#include "pal.h"
#include <errno.h>
#include "lib.h"

#ifdef HAVE_PVLAN
#include "hal_incl.h"
#include "hal_comm.h"
#include "hal_ipifwd.h"
#include "hal_pvlan.h"

int
hal_set_pvlan_type  (char *bridge_name, unsigned short vid, 
                     enum hal_pvlan_type type)
{
  int ret;

  ret = hal_ioctl (APNBR_SET_PVLAN_TYPE, (unsigned long)bridge_name, 
                   vid, type, 0, 0, 0);

  if (ret < 0)
    return -errno;

 return HAL_SUCCESS;

}

int
hal_vlan_associate (char *bridge_name, unsigned short vid,
                    unsigned short pvid, int associate)
{
  int ret;

  ret = hal_ioctl (APNBR_SET_PVLAN_ASSOCIATE, (unsigned long)bridge_name,
                   vid, pvid, associate, 0, 0);

  if (ret < 0)
    return -errno;

 return HAL_SUCCESS;

}

int
hal_pvlan_set_port_mode (char *bridge_name, int ifindex, 
                         enum hal_pvlan_port_mode mode)
{
  int ret;

  ret = hal_ioctl (APNBR_SET_PVLAN_PORT_MODE, (unsigned long)bridge_name,
                   ifindex, mode, 0, 0, 0);

  if (ret < 0)
    return -errno;

 return HAL_SUCCESS;

}

int
hal_pvlan_host_association (char *bridge_name, int ifindex,  unsigned short vid,
                            unsigned short pvid, int associate)
{
   int ret;

  ret = hal_ioctl (APNBR_SET_PVLAN_HOST_ASSOCIATION,(unsigned long)bridge_name,
                    ifindex, vid, pvid, associate, 0);

  if (ret < 0)
    return -errno;

 return HAL_SUCCESS;

}

#endif /* HAVE_PVLAN */

