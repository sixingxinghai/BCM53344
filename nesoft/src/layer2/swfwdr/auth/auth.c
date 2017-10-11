/* Copyright (C) 2003  All Rights Reserved.
 
  INET    802.1x Port Based Authentication
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */


/* Forward declarations */
#include <linux/autoconf.h>
#include <linux/module.h>
#include <net/net_namespace.h>
#include <linux/nsproxy.h>
#include "auth.h"
#include "if_eapol.h"
#include "bdebug.h"
/* MAC Auth Feature */
#include "bridge/br_vlan.h"
#include "bridge/br_types.h"
#include "bridge/br_fdb.h"

#define AUTH_NAME "APN 802.1x"

#define AUTH_PORT_HASH_SHIFT    5
#define AUTH_PORT_HASH_SIZE     (1 << AUTH_PORT_HASH_SHIFT)
#define AUTH_PORT_HASH_MASK     (AUTH_PORT_HASH_SIZE - 1)

/* GLobal variables */

int             (*xmit_fcn)(struct sk_buff * skb, struct net_device * dev);
int             (*rx_fcn)(struct sk_buff * skb);
rwlock_t  auth_port_lock = RW_LOCK_UNLOCKED;

/* Global AUTH variables */

/* Section 7.8 Table 7-2 */
const unsigned char pae_group_address[] = 
                    { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x03 };

/* Our listing of AUTH group(s) */
struct auth_port * auth_port_hash[AUTH_PORT_HASH_SIZE];

#define auth_port_hashfn(IDX) ((IDX) & AUTH_PORT_HASH_MASK)
/* MAC AUTH Feature */
u_char authMac;
#define AUTH_MAC_CONFIG_ENABLED  (1<<0)
#define AUTH_MAC_CONFIG_DISABLED (1<<1)

/* Get the auth_port struct based on ifindex. 
   Returns pointer to port if found, 0 otherwise.
   Must be invoked with auth_port_lock held. */

struct auth_port *
auth_find_port (int real_dev_ifindex)
{
  register struct auth_port * port;
  BDEBUG("port port id %d\n", real_dev_ifindex);
  for (port = auth_port_hash[auth_port_hashfn(real_dev_ifindex)];
         port; port = port->next) 
    {
      if (port->ifindex == real_dev_ifindex)
        break;
    }
  BDEBUG("port index %d address %x\n", real_dev_ifindex, port);
  return port;
}

/* Insert a port in the port hash table.
   Must hold auth_port_lock. */

static void 
port_insert(struct auth_port * port)
{
  struct auth_port **head;

  head = &auth_port_hash[auth_port_hashfn(port->ifindex)];
  port->next = *head;
  *head = port;
}

/* Remove a port from the port hash table.
   Must hold auth_port_lock. */

static void 
port_delete(struct auth_port * port)
{
  struct auth_port * next, **pprev;

  pprev = &auth_port_hash[auth_port_hashfn(port->ifindex)];
  next = *pprev;
  while (next != port) 
  {
    pprev = &next->next;
    next = *pprev;
  }
  *pprev = port->next;
}

/* Initialize the auth_port data structure to the default values
   and insert it into the hash table.  Must hold auth_port_lock. */
static void
port_init (struct auth_port * port, struct net_device * dev)
{
  /* Clear everything out */
  memset(port, 0, sizeof(struct auth_port));
  /* Set the ifindex */
  port->ifindex = dev->ifindex;
  if (authMac & AUTH_MAC_CONFIG_ENABLED)
    {
      BDEBUG("MAC Authentication port initialization\n"); 
      port->auth_state = MACAUTH_DISABLED;
    }
  else
    {
      port->auth_state = port->dir = DIR_IN;
      port->xmit_fcn = dev->hard_start_xmit;
      xmit_fcn = dev->hard_start_xmit;
    }

  port->rx_fcn = dev->rx_hook;
  rx_fcn = dev->rx_hook;
  port_insert (port);
}

/* auth_handle_data_packet processes data packets in the 
   authorized state. Basically, we hijack the skb over to
   the authentication routines and reschedule the packet. */

/* This routine handles all outgoing packets for blocked devices. */

static int
null_xmit (struct sk_buff * skb, struct net_device * dev)
{
  int ret;
  struct ethhdr *eth;
  unsigned char *rawp;
   
  if (skb == NULL) {
    return NET_XMIT_SUCCESS;
  }

  eth = (struct ethhdr *)skb->data;

  /* We always let PAE control packets through - Section 7.8 */
  if (htons(eth->h_proto) == ETH_P_PAE)
    {
      struct auth_port * port = 0;
      BDEBUG("EAPOL packet sent to %s\n", dev->name);
      port = auth_find_port (dev->ifindex);
      BR_READ_LOCK (&auth_port_lock);
      if (port)
        {
                BDEBUG ("calling port xmit\n");
          dev->trans_start = jiffies;
          ret = port->xmit_fcn(skb, dev);
          BR_READ_UNLOCK (&auth_port_lock);
          return ret;
        }
        else
        {
          BDEBUG ("could not find port for dev %s\n", dev->name);
          /* Why are we here? (Cannot xmit lock as is already locked) */
          BR_READ_UNLOCK (&auth_port_lock);
          return NET_XMIT_SUCCESS;
        }
    }
  else
    {
      BDEBUG("Non-EAPOL packet discarded for %s\n", dev->name);
                  if (skb)
                          kfree_skb(skb);
      return NET_XMIT_SUCCESS;
    }
}

/* This routine discards incoming packets for blocked devices. */

static int
null_recv (struct sk_buff * skb)
{
  struct ethhdr *eth;
   
  if (skb == NULL) {
    return NET_RX_SUCCESS;
  }

  eth = (struct ethhdr *)skb->mac_header;

  BDEBUG("rx packet from %s\n", skb->dev->name);
  /* We allow PAE control packets to flow - Section 7.8 */
  if ( (htons(eth->h_proto) == ETH_P_PAE) ||
       (memcmp (skb->mac_header, pae_group_address, ETH_ALEN) == 0) )
    {
      BDEBUG("EAPOL packet found from %s\n", skb->dev->name);
      return NET_RX_SUCCESS;
    }
  else
    {
      BDEBUG("Non-EAPOL packet discarded %s\n", skb->dev->name);
      return NET_RX_DROP;
    }
}

/* When MAC authentication is enabled on any interface then we ovwerwrite
   rx->hook by this function.
   If the source mac and vlan combination exists either in StaticMac table
   or Dynamic Mac table, then we drop the frame otherwise send the frame
   to authd */
static int
auth_mac_null_recv (struct sk_buff * skb)
{
  unsigned char *dest_addr;
  unsigned char *source_addr;
  struct apn_bridge_fdb_entry *dynamic_fdb_entry = NULL;
  struct auth_port *port;
  struct static_entries static_entries;
  br_frame_t frame_type;
  struct vlan_info_entry *vlan;

  if (skb == NULL) 
    {
      return NET_RX_SUCCESS;
    }

  struct apn_bridge *br;
  struct apn_bridge_port *bridge_port =
    (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if ( bridge_port == NULL )
    {
      kfree_skb ( skb );
      return NET_RX_SUCCESS;
    }

  dest_addr = skb->mac_header;
  source_addr = skb_src_mac_addr(skb);
  port = auth_find_port (skb->dev->ifindex);

  BDEBUG ( "Frame recvd on %s\n", skb->dev->name );

  br = bridge_port->br;
  if (br == NULL)
    {
      kfree_skb (skb);
      return NET_RX_DROP;
    }

  if (port == NULL)
    {
      kfree_skb (skb);
      return NET_RX_DROP;
    }

  if (port->auth_state != MACAUTH_ENABLED)
    return NET_RX_SUCCESS;

/* We need the vid of the frame */
  vid_t vid = br_vlan_get_vid_from_frame (ETH_P_8021Q_CTAG, skb);

  BDEBUG ("VID of the incoming frame %u\n", vid);

  frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);

  if (frame_type == UNTAGGED)
    {
      if (br->is_vlan_aware)
        dynamic_fdb_entry = br_fdb_get (br, source_addr, 1, 1);
      else
        dynamic_fdb_entry = br_fdb_get (br, source_addr, 0, 0);
    }
  else
    {
      if (br->is_vlan_aware)
        dynamic_fdb_entry = br_fdb_get (br, source_addr,
                                        vid, vid);
      else
        {
          kfree_skb (skb);
          return NET_RX_DROP;
        }
    }

  if (dynamic_fdb_entry == NULL)
    {
      if (frame_type == UNTAGGED)
        {
          if (br->is_vlan_aware)
            vid = 1;
          else
            vid = 0;

          if (br_fdb_get_static_fdb_entries ( br, source_addr,
                                              vid, vid, &static_entries )
                                              == BR_NOMATCH)
            {
              BDEBUG ("br_fdb_get_static_fdb_entries nomatch \n");
                      skb->protocol = ntohs((unsigned short) ETH_P_PAE);
              return NET_RX_SUCCESS;
            }
        }
     else
       {
         if (br->is_vlan_aware)
           {
             vid = br_vlan_get_vid_from_frame(ETH_P_8021Q_CTAG, skb);
             if (br_fdb_get_static_fdb_entries ( br, source_addr,
                                                 vid, vid, &static_entries )
                                                 == BR_NOMATCH)
               {
                 BDEBUG ("br_fdb_get_static_fdb_entries nomatch \n");
                         skb->protocol = ntohs((unsigned short) ETH_P_PAE);
                 return NET_RX_SUCCESS;
               }
            }
         else
           {
             kfree_skb (skb);
             return NET_RX_DROP;
           }
        }
      skb->protocol = ntohs((unsigned short) ETH_P_PAE);
      BDEBUG("Src mac and VID combination not found\n");
      return NET_RX_SUCCESS;
    } 
  else
    {
      if (dynamic_fdb_entry->is_fwd == BR_TRUE)
        return NET_RX_SUCCESS;
      else if (dynamic_fdb_entry->is_fwd == BR_FALSE)
        return NET_RX_DROP;
    }

  return NET_RX_SUCCESS;
}

/* Put the port with the given name under 802.1x control. */
int
auth_add_port (const char * const name)
{
  struct auth_port * port = 0;
  struct net_device * dev = dev_get_by_name (current->nsproxy->net_ns, name);
  if (dev == 0)
    return -ENOENT;

  if (authMac & AUTH_MAC_CONFIG_ENABLED)
    BDEBUG("AUTH MAC port add %s\n", name);
  else
    BDEBUG("Add port %s\n", name);

  BR_WRITE_LOCK (&auth_port_lock);
  port = auth_find_port (dev->ifindex);
  if (port == 0)
    {
      port = kmalloc (sizeof (struct auth_port), GFP_ATOMIC);
      if (port)
        {
          port_init (port, dev);

          l2_mod_inc_use_count ();

          /* Port is initialised to DIR_IN,
           * so dev->rx_hook and dev->hard_start_xmit functions should be overwritten
           */
          spin_lock (&dev->tx_global_lock);
          if (authMac & AUTH_MAC_CONFIG_ENABLED)
            {
              BDEBUG("Overwritting rx_hook func by AUTH MAC\n");
              dev->rx_hook = auth_mac_null_recv; 
            }
          else
            {
              dev->rx_hook = null_recv;
              if (port->xmit_fcn == xmit_fcn)
                {
                  if (port->xmit_fcn)
                    dev->hard_start_xmit = port->xmit_fcn;
                }
            }
          spin_unlock (&dev->tx_global_lock);
        }
      else
        {
          BWARN("insufficient memory for %s\n", name);
          dev_put (dev);
          BR_WRITE_UNLOCK (&auth_port_lock);
          return -ENOMEM;
        }
    }
  else
    {
      dev_put (dev);
      BR_WRITE_UNLOCK (&auth_port_lock);
      return -EEXIST;
    }
  dev_put (dev);
  BR_WRITE_UNLOCK (&auth_port_lock);
  return 0;
}

/* Remove the port with the given name from 802.1x control. */
int 
auth_remove_port (const char * const name)
{
  struct auth_port * port = 0;
  struct net_device * dev = dev_get_by_name (current->nsproxy->net_ns, name);
  if (dev == 0)
    return -ENOENT;

  BDEBUG("%s\n", name);
  BR_WRITE_LOCK (&auth_port_lock);
  port = auth_find_port (dev->ifindex);
  if (port)
    {
      spin_lock (&dev->tx_global_lock);
      /* Put back the hijacked functions */
      if (port->xmit_fcn)
        dev->hard_start_xmit = port->xmit_fcn;
      dev->rx_hook = 0;
      spin_unlock (&dev->tx_global_lock);
      l2_mod_dec_use_count ();
      port_delete (port);
    }
  dev_put (dev);
  BR_WRITE_UNLOCK (&auth_port_lock);
  return 0;
}

void
auth_remove_all_ports (void)
{
  register struct auth_port * port;
  register struct auth_port * next;
  struct net_device * dev;
  int hash;
  BR_WRITE_LOCK (&auth_port_lock);
  for (hash = 0; hash < AUTH_PORT_HASH_SIZE; hash++)
    {
      for (port = auth_port_hash[hash]; port; port = next) 
        {
          auth_port_hash[hash] = next = port->next;
          dev = dev_get_by_index (current->nsproxy->net_ns, port->ifindex);
          if (dev)
            {
              spin_lock (&dev->tx_global_lock);
              /* Put back the hijacked functions */
              if (port->xmit_fcn)
                dev->hard_start_xmit = port->xmit_fcn;
              dev->rx_hook = 0;
              spin_unlock (&dev->tx_global_lock);
              dev_put (dev);
            }
          l2_mod_dec_use_count ();
        }
    }
  BR_WRITE_UNLOCK (&auth_port_lock);
}

/* Set the port state.  */
int
auth_set_port_state (const int ifindex, const int state)
{
  struct net_device *dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);
  struct auth_port *port = 0;

  if (dev == 0)
    return -ENOENT;
  
  BDEBUG("port %d dir %d\n", ifindex, state);
  BR_WRITE_LOCK (&auth_port_lock);

  port = auth_find_port (ifindex);

  if (port == NULL)
    {
      BR_WRITE_UNLOCK (&auth_port_lock);
      auth_add_port (dev->name);
      BR_WRITE_LOCK (&auth_port_lock);
      port = auth_find_port (ifindex);

    }

  if (port != NULL)
    {
      if (port->auth_state == state)
        {
          dev_put (dev);
          BR_WRITE_UNLOCK (&auth_port_lock);
          return 0;
        }
     
      switch (state)
        {
        case APN_AUTH_PORT_STATE_BLOCK_INOUT:
          /* Set the provisioned state.  */
          port->auth_state = APN_AUTH_PORT_STATE_BLOCK_INOUT;
          port->dir = DIR_BOTH;
          spin_lock (&dev->tx_global_lock);
          dev->hard_start_xmit = null_xmit;
          dev->rx_hook = null_recv;
          spin_unlock (&dev->tx_global_lock);
          break;

        case APN_AUTH_PORT_STATE_BLOCK_IN:
          port->auth_state = APN_AUTH_PORT_STATE_BLOCK_IN;
          port->dir = DIR_IN;
          spin_lock (&dev->tx_global_lock);
          dev->rx_hook = null_recv;
          if (port->xmit_fcn == xmit_fcn)
            {
              if (port->xmit_fcn)
                dev->hard_start_xmit = port->xmit_fcn;
            }
          spin_unlock (&dev->tx_global_lock);
          break;

        case APN_AUTH_PORT_STATE_UNBLOCK:
          port->auth_state = APN_AUTH_PORT_STATE_UNBLOCK;
          BDEBUG("Unblock traffic on port %d\n", ifindex);
          
          spin_lock (&dev->tx_global_lock);
          if (port->xmit_fcn)
            dev->hard_start_xmit = port->xmit_fcn;
          dev->rx_hook = 0;
          spin_unlock (&dev->tx_global_lock);
          break;

        case APN_AUTH_PORT_STATE_UNCONTROLLED:
          port->auth_state = APN_AUTH_PORT_STATE_UNCONTROLLED;
          spin_lock (&dev->tx_global_lock);
          /* Put back the hijacked functions.  */
          if (port->xmit_fcn)
            dev->hard_start_xmit = port->xmit_fcn;
          dev->rx_hook = 0;
          spin_unlock (&dev->tx_global_lock);
          l2_mod_dec_use_count ();
          port_delete (port);
          BR_WRITE_UNLOCK (&auth_port_lock);
          auth_remove_port (dev->name);
          BR_WRITE_LOCK (&auth_port_lock);
          break;

        default:
          break;
        }
    }

  dev_put (dev);
  BR_WRITE_UNLOCK (&auth_port_lock);

  return 0;
}

/* Set the port auth-mac state */
int
auth_mac_set_port_state (const int ifindex, const int state)
{

  struct net_device *dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);
  struct auth_port *port = 0;

  if (dev == 0)
    return -ENOENT;

  BDEBUG("AUTH Mac port %d state %d\n", ifindex, state);
  BR_WRITE_LOCK (&auth_port_lock);

  authMac = (authMac | AUTH_MAC_CONFIG_ENABLED); 

  port = auth_find_port (ifindex);

  if (port == NULL)
    {
      BR_WRITE_UNLOCK (&auth_port_lock);
      auth_add_port (dev->name);
      BR_WRITE_LOCK (&auth_port_lock);
      port = auth_find_port (ifindex);

    }

  if (port != NULL)
    {
      if (port->auth_state == state)
        {
          dev_put (dev);
          BR_WRITE_UNLOCK (&auth_port_lock);
          return 0;
        }

      switch (state)
        {
        case APN_MACAUTH_PORT_STATE_ENABLED: 
          port->auth_state =  MACAUTH_ENABLED; 
          BDEBUG("AUTH Mac port %d state enable\n", ifindex);
          break;
           
        case APN_MACAUTH_PORT_STATE_DISABLED: 
          port->auth_state =  MACAUTH_DISABLED; 
          BDEBUG("AUTH Mac port %d state disable\n", ifindex);
          spin_lock (&dev->tx_global_lock);
          /* If rx_hook is set to NULL then in kernel (netif_rx)
             won't call any callback function */
          BDEBUG("Giving back the hijacked rx_hook\n");
          dev->rx_hook = 0;
          spin_unlock (&dev->tx_global_lock);
          l2_mod_dec_use_count ();
          port_delete (port);
          break;

        default:
          break;
        } 
    }   

  dev_put (dev);
  BR_WRITE_UNLOCK (&auth_port_lock);

  return 0;
}
