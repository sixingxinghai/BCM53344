/* Copyright (C) 2003  All Rights Reserved.
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
#include "br_gmrp_api.h"
#include "br_types.h"

/* This function sets the service requirement for the port */
int 
br_gmrp_set_service_requirement (struct apn_bridge *br, int vlanid, int ifindex, int serv_req, bool_t activate)
{
  struct apn_bridge_port *port = 0;

  BDEBUG("executing set port service requirement to port %d, serv_req %d \n",
         ifindex, serv_req);

  if ((port = br_get_port (br, ifindex)) == NULL)
    {
      BDEBUG("Invalid port  %ld\n", ifindex );
      return -EINVAL;
    }

  
  if (serv_req == APN_BR_GMRP_FWD_ALL)
    {
      BDEBUG("br_gmrp_set_service_requirement: serv_req = APN_BR_GMRP_FWD_ALL activate =  %d\n", activate);
      if (activate)
        BR_SERV_REQ_VLAN_SET (port->gmrp_serv_req_fwd_all, vlanid);
      else
        BR_SERV_REQ_VLAN_UNSET (port->gmrp_serv_req_fwd_all, vlanid);

      BDEBUG ("br_gmrp_set_service_requirement: port->gmrp_service_req = %d \n", port->gmrp_serv_req_fwd_all);
    }

  else if (serv_req == APN_BR_GMRP_FWD_UNREGISTERED)
    {
      BDEBUG("br_gmrp_set_service_requirement: serv_req = APN_BR_GMRP_FWD_UNREGISTERED activate =  %d\n", activate);
      if (activate)
        BR_SERV_REQ_VLAN_SET (port->gmrp_serv_req_fwd_unreg, vlanid);
      else
        BR_SERV_REQ_VLAN_UNSET (port->gmrp_serv_req_fwd_unreg, vlanid);

      BDEBUG ("br_gmrp_set_service_requirement: port->gmrp_service_req = %d \n", port->gmrp_serv_req_fwd_unreg);
    }

  return 0;
}

int
br_gmrp_set_ext_filter (struct apn_bridge *br, bool_t enable)
{
  struct apn_bridge_port *port = 0;

  if (br->garp_config == APNBR_GARP_GMRP_CONFIGURED)
    br->ext_filter = enable ? 1 : 0;

  if (!enable)
    for (port = br->port_list; port; port = port->next)
      {
        BR_SERV_REQ_VLAN_CLEAR (port->gmrp_serv_req_fwd_all);
        BR_SERV_REQ_VLAN_CLEAR (port->gmrp_serv_req_fwd_unreg);
      }

  return 0;
}
 


