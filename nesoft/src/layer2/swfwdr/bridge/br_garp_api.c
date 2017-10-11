/* Copyright (C) 2005  All Rights Reserved.
 *
 * GARP functionalities
 *
 * Author:  
 *
 */
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include "if_ipifwd.h"
#include "br_vlan_api.h"
#include "br_api.h"
#include "br_vlan_dev.h"
#include "bdebug.h"
#include "br_garp_api.h"

/* This function set the bridge as gmrp/gvrp enable or disable*/                                                                                
int 
br_garp_set_bridge_type (struct apn_bridge *br, unsigned long garp_type, int enable)
{
  BDEBUG (" br garp set bridge state:Setting the bridge as gvrp/gmrp enable or disable\n");
  if (enable)
    {
      br->garp_config |= garp_type;
    }
  else
    {
      br->garp_config &= ~(garp_type);
    }

  return 0;
}
