/* Copyright (C) 2003  All Rights Reserved.
 *   
 * VLAN net device
 */
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/module.h>
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_vlan_api.h"
#include "br_vlan_dev.h"
#include "br_input.h"
#include "br_forward.h"
#include "bdebug.h"

/* This macro is no more applicable in 2.6.27 kernel */
/* #define SET_MODULE_OWNER(some_struct) do { } while (0) */

extern unsigned char zero_mac_addr[];

static int
__br_vlan_dev_xmit (struct vlan_info_entry *vinfo, struct sk_buff *skb)
{
  br_result_t static_result, dynamic_result;
  struct net_device_stats *stats = &vinfo->net_stats;
  unsigned int len = skb->len;

  skb->mac_header = skb->data;

  BDEBUG ("skb src " MAC_ADDR_FMT" dst " MAC_ADDR_FMT " VI: %s\n", 
          MAC_ADDR_PRINT (skb_src_mac_addr(skb)), 
          MAC_ADDR_PRINT (skb->mac_header), vinfo->dev->name);


  /* Check if the source address is valid */
  if (!memcmp (&skb_src_mac_addr(skb), zero_mac_addr, vinfo->dev->addr_len))
    {
      /* Zero source MAC address. Can happen when VI does not have a MAC 
       * address because no L2 port has been attached to the VI
       */
      BDEBUG ("Zero source MAC address. Dropping!!!\n");
      kfree_skb (skb);
      stats->tx_dropped++;
      stats->tx_errors++;
      return 0;
    }

  if (skb_dst_mac_addr(skb) & 1)
    {
      /* Broadcast or multicast source address. Dropping !!! */
      BDEBUG ("Broadcast or multicast source MAC address. Dropping!!!\n");
      kfree_skb (skb);
      stats->tx_dropped++;
      stats->tx_errors++;
      return 0;
    }

  /* Bridge forwarding routines below expect a frame with the Ethernet
   * header pulled, so pull it!
   */
  skb_pull (skb, vinfo->dev->hard_header_len);
  
  /* To allow for filtering broadcast frames statically, check in static 
     fdb before checking for broadcast */
  static_result = br_static_forward_frame (skb, vinfo->br, vinfo->vid,
                                           VLAN_NULL_VID);

  dynamic_result = br_dynamic_forward_frame (skb, vinfo->br, vinfo->vid, 
                                           VLAN_NULL_VID);
  
  if ((static_result == BR_ERROR) || (static_result == BR_NORESOURCE))
    {

      if ((dynamic_result == BR_ERROR) || (dynamic_result == BR_NORESOURCE)
          || (dynamic_result == BR_NOMATCH))
        {
          kfree_skb (skb);
          stats->tx_dropped++;
          stats->tx_errors++;
          return 0;
        }
      else
        {
          /* Must have been forwarded by dynamic fdb */
        }
    }
  else if (static_result == BR_NOMATCH)
    {
      if ((dynamic_result == BR_ERROR) || (dynamic_result == BR_NORESOURCE))
        {
          kfree_skb (skb);
          stats->tx_dropped++;
          stats->tx_errors++;
          return 0;
        }
      else if (dynamic_result == BR_NOMATCH)
        {
          BDEBUG ( "no entry found - broadcast (flood deliver)\n" );
          br_flood_forward (vinfo->br, skb, vinfo->vid, vinfo->vid, 0);
        }
      else
        {
          /* Must have been forwarded by dynamic fdb */
        }
    }
  else
    {
      /* Must have been forwarded by static fdb */
      if ((dynamic_result == BR_ERROR) || (dynamic_result == BR_NORESOURCE)
          || (dynamic_result == BR_NOMATCH))
        {
          kfree_skb (skb);
        }
      else
        {
          /* Must have been forwarded by dynamic fdb */
        }
    }

  /* Update statistics */
  stats->tx_packets++;
  stats->tx_bytes += len;

  return 0;
}

static int
br_vlan_dev_xmit (struct sk_buff * skb, struct net_device * dev)
{
  struct vlan_info_entry *vinfo = dev->priv;
  struct apn_bridge *br = vinfo->br;
  int ret;

  ret = __br_vlan_dev_xmit (vinfo, skb);

  return ret;
}

static int
br_vlan_dev_open (struct net_device * dev)
{
  netif_start_queue (dev);

  return 0;
}

static int
br_vlan_dev_close (struct net_device * dev)
{
  netif_stop_queue (dev);

  return 0;
}

static struct net_device_stats *
br_vlan_dev_get_stats(struct net_device *dev)
{
  struct vlan_info_entry *vinfo = (struct vlan_info_entry *)dev->priv;

  return &vinfo->net_stats;
}

void
br_vlan_dev_set_mc_list (struct net_device *dev)
{
  return;
}

static int
br_vlan_dev_set_mac_addr (struct net_device *dev, void *p)
{
  struct sockaddr *addr = p;
  struct vlan_info_entry *vinfo = dev->priv;

  if (netif_running (dev))
    return -EBUSY;

  memcpy (dev->dev_addr, addr->sa_data, dev->addr_len);

  /* Set the port whose address we are using.
   * If this function is called from user land and the MAC address
   * being set is be a logical MAC address not belonging to any L2 port,
   * then vi_addr_port will be NULL.
   */
  vinfo->vi_addr_port = br_vlan_get_port_by_hw_addr (vinfo->br, 
                                                     dev->dev_addr);

  BDEBUG ("Setting MAC Address of VI %s to " MAC_ADDR_FMT " (%s)\n",
          dev->name, MAC_ADDR_PRINT (dev->dev_addr), 
          (vinfo->vi_addr_port ? vinfo->vi_addr_port->dev->name : "none"));

  return 0;
}

static int
br_vlan_dev_set_port_mac_addr (struct vlan_info_entry *vinfo, 
                               struct apn_bridge_port *p)
{
  struct net_device *vdev = vinfo->dev;

  if (p)
    memcpy (vdev->dev_addr, p->dev->dev_addr, vdev->addr_len);
  else
    memset (vdev->dev_addr, 0, vdev->addr_len);

  /* Set the port whose address we are using. */
  vinfo->vi_addr_port = p;

  BDEBUG ("Setting MAC Address of VI %s to " MAC_ADDR_FMT " (%s)\n",
          vdev->name, MAC_ADDR_PRINT (vdev->dev_addr), 
          (vinfo->vi_addr_port ? vinfo->vi_addr_port->dev->name : "none"));

  return 0;
}

/* Setup the VLAN device */
void
br_vlan_dev_setup (struct net_device *dev)
{
  dev->hard_start_xmit    = br_vlan_dev_xmit;
  dev->open               = br_vlan_dev_open;
  dev->stop               = br_vlan_dev_close;
  dev->get_stats          = br_vlan_dev_get_stats;
  dev->set_multicast_list = br_vlan_dev_set_mc_list;

  /* Overwrite the default eth_mac_addr () */
  dev->set_mac_address    = br_vlan_dev_set_mac_addr;

  memset (dev->dev_addr, 0, ETH_ALEN);

  dev->tx_queue_len = 0;

  /* This needs to be set to NULL so that dev_open doesn't try to validate address */
  dev->validate_addr = NULL;

  return;
}

/* Function to create a net_device for vlan */
int
br_vlan_dev_create (struct vlan_info_entry *vinfo, enum vlan_type type)
{
  char vlan_ifnam[IFNAMSIZ];
  struct net_device *dev;
  int ret;

  if (vinfo->dev != NULL)
    return 0;

  memset(vlan_ifnam, 0, IFNAMSIZ);

  if (!strncmp(vinfo->br->name,"backbone",8))
    {
      snprintf (vlan_ifnam, IFNAMSIZ - 1, "svlan%s.%d",
          "b", vinfo->vid); /*adding to the svlan table for now*/
    }
  else
    {
      if (type == CUSTOMER_VLAN)
        snprintf (vlan_ifnam, IFNAMSIZ - 1, "vlan%s.%d",
            vinfo->br->name, vinfo->vid);
      else
        snprintf (vlan_ifnam, IFNAMSIZ - 1, "svlan%s.%d",
            vinfo->br->name, vinfo->vid);
    }

  /* Allocate device. */
  dev = alloc_netdev (sizeof (vinfo), vlan_ifnam, ether_setup);
  if (dev == NULL)
    return -1;

  br_vlan_dev_setup (dev);
  
  /*
  SET_MODULE_OWNER(dev); 
  */

  vinfo->dev = dev;
  dev->priv = vinfo;

  rtnl_lock ();

  ret = dev_alloc_name (dev, vlan_ifnam);

  if (ret < 0)
    {
      kfree (dev);
      return -1;
    }

  rtnl_unlock ();


  /* Ethernet type setup */
  if ((ret = register_netdev (dev))) 
    {
      kfree(dev);
      vinfo->dev = NULL; 
      return ret;
    }

  netif_carrier_off (dev);

  rtnl_lock ();

    
  if (dev_open (dev))
    {
      unregister_netdev (dev);
      kfree(dev);
      rtnl_unlock ();
      vinfo->dev = NULL;
      return ret;
    }
  
  rtnl_unlock ();

  l2_mod_inc_use_count();
  BDEBUG ("Added new vlan interface: %s\n", dev->name);
  return 0;
}

/* Function to destroy a net_device for vlan */
void
br_vlan_dev_destroy (struct vlan_info_entry *vinfo)
{
  if (vinfo->dev == NULL)
    return;

  BDEBUG ("Deleting vlan interface: %s\n", vinfo->dev->name);

  unregister_netdev (vinfo->dev);
  free_netdev(vinfo->dev);

  vinfo->dev = NULL;
  vinfo->vi_addr_port = NULL;

  l2_mod_dec_use_count();
  return;
}

static
bool_t br_vlan_check_for_running (struct apn_bridge *br, vid_t vid, int instance)
{
  bool_t port_in_forwarding = BR_FALSE;
  struct apn_bridge_port *p = NULL;
  char *config_ports = 0;
  int port_id;

  if (br == NULL)
    return port_in_forwarding;

  config_ports = br->static_vlan_reg_table[vid];

  if (!config_ports)
    return port_in_forwarding;

  for (port_id = 0; port_id < BR_MAX_PORTS; port_id++ )
    {
      if (!(config_ports[port_id] & VLAN_PORT_CONFIGURED))
        continue;

      if (((p = br_vlan_get_port_by_id (br, port_id)) != NULL) &&
           (p->state_flags[instance] & APNFLAG_FORWARD))
        {
          port_in_forwarding = BR_TRUE;
          break;
        }
    }

  return port_in_forwarding;
}


/* Add a port to the VLAN */
void
br_vlan_dev_add_port (struct vlan_info_entry *vinfo,
                      struct apn_bridge_port *port)
{
  struct net_device *vdev = vinfo->dev;
  bool_t set_running = BR_TRUE;
  int instance;

  if (vdev == NULL)
    return;

  /* Check if the MAC address is not set */
  if (memcmp (vdev->dev_addr, &zero_mac_addr, vdev->addr_len))
    return;
  instance = vinfo->instance;
  if (! br_vlan_check_for_running (vinfo->br, vinfo->vid, instance))
    set_running = BR_FALSE;

  rtnl_lock ();
  dev_close (vinfo->dev);

  if (set_running)
    netif_carrier_on (vinfo->dev);

  if (dev_open (vinfo->dev))
    BDEBUG ("Error opening the VLAN Interface: %s\n", vinfo->dev->name);

  rtnl_unlock ();
  br_vlan_dev_set_port_mac_addr (vinfo, port);
  return;
}

/* Remove a port from the VLAN */
void
br_vlan_dev_del_port (struct vlan_info_entry *vinfo,
                      enum vlan_type type,
                      struct apn_bridge_port *port)
{
  struct net_device *vdev = vinfo->dev;
  struct apn_bridge *br = vinfo->br;
  struct apn_bridge_port *p = NULL;
  bool_t set_running = BR_TRUE;
  int instance;
  char *config_ports;
  int port_id;

  if (vdev == NULL)
    return;

  /* Check if the MAC address of VLAN device is the address of the port */
  if (memcmp (vdev->dev_addr, port->dev->dev_addr, vdev->addr_len))
    return;

  BDEBUG ("Deleting MAC Address of VI %s " MAC_ADDR_FMT " (%s)\n",
          vdev->name, MAC_ADDR_PRINT (vdev->dev_addr), port->dev->name);

  /* Choose another port address as VLAN device MAC address */
  BR_READ_LOCK (&br->lock);

  if (type == CUSTOMER_VLAN)
    config_ports = br->static_vlan_reg_table[vinfo->vid];
  else
    config_ports = br->static_svlan_reg_table[vinfo->vid];

  if (!config_ports)
    {
      goto out;
    }

  for (port_id = 0; port_id < BR_MAX_PORTS; port_id++ )
    {
      if (!(config_ports[port_id] & VLAN_PORT_CONFIGURED))
        continue;

      if (((p = br_vlan_get_port_by_id (br, port_id)) != NULL) &&
          (p != port))
        {
          /* Assign this MAC addres to the VLAN device */
          break;
        }
    }

 out:
  /* RELEASE READ LOCK */
  BR_READ_UNLOCK (&br->lock);

  br_vlan_dev_set_port_mac_addr (vinfo, p);
 
  instance = vinfo->instance;
  if (! br_vlan_check_for_running (vinfo->br, vinfo->vid, instance))
    set_running = BR_FALSE;

  rtnl_lock ();
  dev_close (vinfo->dev);
  
  if (! set_running)
    netif_carrier_off (vinfo->dev);
  if (dev_open (vinfo->dev))
    BDEBUG ("Error opening the VLAN Interface: %s\n", vinfo->dev->name);

  rtnl_unlock ();
  return;
}

void
br_vlan_dev_change_flags (struct apn_bridge *br, struct apn_bridge_port *port,
                          int instance)
{
  struct vlan_info_entry *vlan_info_entry = NULL;
  unsigned char config_vids[APN_VLANMAP_SIZE];
  bool_t set_running = BR_TRUE;
  vid_t vid;

  if (br == NULL || port == NULL)
    return;

  if (!br->is_vlan_aware)
    return;

  BR_VLAN_CLEAR_VLAN_BITMAP(config_vids);
  br_vlan_get_config_vids (br, port->port_no, config_vids);

  for (vid = APN_VLAN_DEFAULT_VID; vid <= APN_VLAN_MAX_VID; vid++)
    {
      if (BR_VLAN_IS_BITMAP_SET (vid, config_vids))
        {
          vlan_info_entry = br->vlan_info_table[vid];

          if ((vlan_info_entry == NULL) || (vlan_info_entry->dev == NULL))
            continue;

          if (! br_vlan_check_for_running (br, vid, instance))
            set_running = BR_FALSE;

          rtnl_lock ();
          dev_close (vlan_info_entry->dev);

          if (set_running)
            netif_carrier_on (vlan_info_entry->dev);
          else
            netif_carrier_off (vlan_info_entry->dev);

          if (dev_open (vlan_info_entry->dev))
            BDEBUG ("Error opening the VLAN Interface: %s\n",
                    vlan_info_entry->dev->name);
 
          rtnl_unlock ();
        }
    }
}

/* This function handles the change of hw address of l2 port by
 * changing the hw_address of all VIs using this port
 */
void
br_vlan_dev_l2_port_change_addr (struct apn_bridge_port *p)
{
  vid_t vid;
  struct vlan_info_entry *vinfo;
  struct net_device *vdev;
  struct apn_bridge *br = p->br;

  BR_READ_LOCK (&br->lock);

  for (vid = VLAN_DEFAULT_VID; vid <= VLAN_MAX_VID; vid ++)
    {
      if (p->port_type == PRO_NET_PORT
          || p->port_type == CUST_NET_PORT)
        {
          if ((vinfo = br->svlan_info_table[vid]) == NULL)
            continue;
        }
      else
        {
          if ((vinfo = br->vlan_info_table[vid]) == NULL)
            continue;
        }

      if ((vdev = vinfo->dev) == NULL)
        continue;

      if (vinfo->vi_addr_port != p)
        continue;

      /* Change address */
      br_vlan_dev_set_port_mac_addr (vinfo, p);
    }

  BR_READ_UNLOCK (&br->lock);

  return;
}
