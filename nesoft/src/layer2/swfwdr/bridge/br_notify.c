/*
  Device event handling
  Linux ethernet bridge
  
  Authors:
  Lennert Buytenhek   <buytenh@gnu.org>
  
  br_notify.c,v 1.5 2009/04/24 18:05:32 bob.macgill Exp
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
*/
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_api.h"
#include "br_fdb.h"
#include "br_notify.h"
#include "bdebug.h"
#include "br_vlan_dev.h"

/* Forward declaration */
static int 
br_device_event (struct notifier_block *unused, unsigned long event, void *ptr);

struct notifier_block br_device_notifier =
  {
    br_device_event,
    NULL,
    0
  };

/* This function handles event notifications from the OS. */

static int 
br_device_event (struct notifier_block *unused, unsigned long event, void *ptr)
{
  struct net_device * dev;
  struct apn_bridge_port * port;

  dev = ptr;
  port = (struct apn_bridge_port *)(dev->apn_fwd_port);

  if (port == NULL)
    return NOTIFY_DONE;

  switch (event)
    {
    case NETDEV_CHANGEADDR:
      BDEBUG("port %d changed address\n", port->port_no);
      br_fdb_changeaddr (port, port->dev->dev_addr);
      /* Handle VI here */
      br_vlan_dev_l2_port_change_addr (port);
      break;

    case NETDEV_GOING_DOWN:
      /* extend the protocol to send some kind of notification? */
      break;

    case NETDEV_DOWN:
      BDEBUG("port %d went down\n", port->port_no);
      break;

    case NETDEV_UP:
      BDEBUG("port %d came up\n", port->port_no);
      break;

    case NETDEV_UNREGISTER:
      BDEBUG("port %d deleted\n", port->port_no);
      br_del_if (port->br, dev);
      break;
    }

  return NOTIFY_DONE;
}
