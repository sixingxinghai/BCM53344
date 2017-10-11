/* Copyright (C) 2005  All Rights Reserved.
                                                                                
LAYER 2 GARP HAL
                                                                                
This module defines the platform abstraction layer to the
Linux layer 2 garp.
                                                                                
*/
                                                                                
#include "pal.h"
#include <errno.h>
#include "lib.h"
#include "hal_incl.h"
#include "hal_comm.h"
#include "hal_ipifwd.h"
#include "hal_garp.h"

/* This fuction notifies forwarder that gmrp or gvrp has been enabled or
 * disabled  on a port */
void
hal_garp_set_bridge_type (char *bridge_name, unsigned long garp_type, int enable)
{
  int ret = 0;
                                                                              
  ret = hal_ioctl (APNBR_GARP_SET_BRIDGE_TYPE, (unsigned long)bridge_name,
             (unsigned long)garp_type, (unsigned long)enable, 0, 0, 0);
}

