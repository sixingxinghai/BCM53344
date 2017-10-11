/* Copyright (C) 2003  All Rights Reserved.

LAYER 2 GARP HAL

This module defines the platform abstraction layer to the
VxWorks layer 2 gmrp.

*/

#include "pal.h"
#include <errno.h>
#include "lib.h"
#include "hal_incl.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_gmrp.h"

/* This fuction notifies forwarder  about gmrp service requirement on a port */
int
hal_l2_gmrp_set_service_requirement (char *bridge_name, int vlanid, int ifindex, int serv_req, bool_t activate)
{

 return NOT_IMPLIMENTED;

}

/* This fuction notifies forwarder about enabling extended filtering services on a bridge */

int
hal_gmrp_set_bridge_ext_filter (char *bridge_name, bool_t enable)
{
 
  return NOT_IMPLIMENTED;

}


