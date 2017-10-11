/* Copyright (C) 2003  All Rights Reserved.
                                                                                
LAYER 2 GARP HAL
                                                                                
This module defines the platform abstraction layer to the
Linux layer 2 gmrp.
                                                                                
*/
                                                                                
#include "pal.h"
#include <errno.h>
#include "lib.h"
#include "hal_incl.h"
#include "hal_comm.h"
#include "hal_ipifwd.h"
#include "hal_gmrp.h"

/* This fuction notifies forwarder  about gmrp service requirement on a port */
int
hal_l2_gmrp_set_service_requirement (char *bridge_name, int vlanid, int ifindex, int serv_req, bool_t activate)
{
 int ret;

  ret = hal_ioctl (APNBR_ADD_GMRP_SERVICE_REQ, (unsigned long)bridge_name,
                   vlanid, ifindex, serv_req, activate, 0);

  if (ret < 0)
    return -errno;

 return HAL_SUCCESS;
                                                                                
}

int 
hal_gmrp_set_bridge_ext_filter (char *bridge_name, bool_t enable)
{
  int ret;

  ret = hal_ioctl (APNBR_SET_EXT_FILTER, (unsigned long)bridge_name, enable,
                   0, 0, 0, 0);
 
  if (ret < 0)
    return -errno;

  return HAL_SUCCESS;

}
