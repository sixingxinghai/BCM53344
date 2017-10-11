/* Copyright (C) 2003  All Rights Reserved.
 
  INET    802.3ah Ethernet First Mile OAM
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */


/* Forward declarations */
#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <net/net_namespace.h> 
#include <linux/nsproxy.h>
#include "efm.h"
#include "if_efm.h"
#include "bdebug.h"

#define EFM_NAME "APN EFM"
#define EFM_PORT_HASH_SHIFT     5
#define EFM_PORT_HASH_SIZE      (1 << EFM_PORT_HASH_SHIFT)
#define EFM_PORT_HASH_MASK      (EFM_PORT_HASH_SIZE - 1)

/* GLobal variables */
rwlock_t  efm_port_lock = RW_LOCK_UNLOCKED;

/* Global EFM variables */

/* Section 7.8 Table 7-2 */
const unsigned char efm_addr [] = 
                    { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x02 };

/* Our listing of EFM group(s) */
struct efm_port * efm_port_hash[EFM_PORT_HASH_SIZE];

#define efm_port_hashfn(IDX) ((IDX) & EFM_PORT_HASH_MASK)

/* Get the efm_port struct based on ifindex. 
   Returns pointer to port if found, 0 otherwise.
   Must be invoked with efm_port_lock held. */

struct efm_port *
efm_find_port (int real_dev_ifindex)
{
  register struct efm_port * port;
  BDEBUG("port port id %d\n", real_dev_ifindex);

  for (port = efm_port_hash[efm_port_hashfn(real_dev_ifindex)];
       port; port = port->next) 
    {
      if (port->ifindex == real_dev_ifindex)
        break;
    }
  BDEBUG("port index %d address %x\n", real_dev_ifindex, port);
  return port;
}

/* Insert a port in the port hash table.
   Must hold efm_port_lock. */

static void 
port_insert(struct efm_port * port)
{
  struct efm_port **head;

  head = &efm_port_hash[efm_port_hashfn(port->ifindex)];
  port->next = *head;
  *head = port;
}

/* Remove a port from the port hash table.
   Must hold efm_port_lock. */

static void 
port_delete(struct efm_port * port)
{
  struct efm_port * next, **pprev;

  pprev = &efm_port_hash[efm_port_hashfn(port->ifindex)];
  next = *pprev;
  while (next != port) 
  {
    pprev = &next->next;
    next = *pprev;
  }
  *pprev = port->next;
}

/* Initialize the efm_port data structure to the default values
   and insert it into the hash table.  Must hold efm_port_lock. */
static void
port_init (struct efm_port * port, struct net_device * dev)
{
  /* Clear everything out */
  memset(port, 0, sizeof(struct efm_port));
  /* Set the ifindex */
  port->ifindex = dev->ifindex;
  port->xmit_fcn = dev->hard_start_xmit;
  port->rx_fcn = dev->rx_hook; 
  port->local_mux_action = EFM_OAM_MUX_FWD;
  port->local_par_action = EFM_OAM_PAR_FWD;
  port_insert (port);
}

/* This routine handles all outgoing packets for devices. */

static int
efm_xmit (struct sk_buff * skb, struct net_device * dev)
{
  int ret;
  struct ethhdr *eth = NULL;
  unsigned char *rawp;
  struct efm_port * port = 0;

  if ((!skb) || (!dev)) 
    return NET_XMIT_SUCCESS;

  eth = (struct ethhdr *)skb->data;
  
  if (!eth) 
    return NET_XMIT_SUCCESS;

  port = efm_find_port (dev->ifindex);
  BR_READ_LOCK (&efm_port_lock);

  BDEBUG ("Inside efm_xmit %s\n", dev->name);

  if (port == NULL)
    {
      BDEBUG ("could not find port for dev %s\n", dev->name);
      BR_READ_UNLOCK (&efm_port_lock);
      return NET_XMIT_SUCCESS;
    }

  /* We always let OAM control packets through - Section 7.8 */
  if (htons(eth->h_proto) == ETH_P_EFM
      || (memcmp (eth->h_dest, efm_addr, ETH_ALEN) == 0)
      || port->local_mux_action == EFM_OAM_MUX_FWD)
    {
      BDEBUG ("calling port xmit\n");
      dev->trans_start = jiffies;
      ret = port->xmit_fcn(skb, dev);      
      BR_READ_UNLOCK (&efm_port_lock);
      return ret;
    }
  else
    {
      BR_READ_UNLOCK (&efm_port_lock);
      BDEBUG ("could not find port for dev %s\n", dev->name);
      if (skb)
        kfree_skb(skb);
      return NET_XMIT_SUCCESS;
    }
}

/* This routine discards incoming packets for blocked devices. */

static int
efm_recv (struct sk_buff * skb)
{
  int ret;
  struct ethhdr *eth;
  struct efm_port *port;
  struct sk_buff * skb2;
  struct net_device * dev;

  if (!skb) 
    return NET_RX_SUCCESS;

  eth = (struct ethhdr *) skb->mac_header;
  
  if (!eth)
    return NET_RX_SUCCESS;

  dev = skb->dev;

  BDEBUG("rx packet from %s\n", skb->dev->name);

  port = efm_find_port (dev->ifindex);
  BR_READ_LOCK (&efm_port_lock);

  if (port == NULL)
    {
      BDEBUG ("could not find port for dev %s\n", dev->name);
      BR_READ_UNLOCK (&efm_port_lock);
      return NET_RX_SUCCESS;
    }

  /* We allow EFM control packets to flow - Section 7.8 */
  if ( (htons(eth->h_proto) == ETH_P_EFM) ||
       (memcmp (eth->h_dest, efm_addr, ETH_ALEN) == 0) ||
       port->local_par_action == EFM_OAM_PAR_FWD)
    {
      BDEBUG("OAM packet found from %s\n", skb->dev->name);
      return NET_RX_SUCCESS;
    }
  else if (port->local_par_action == EFM_OAM_PAR_DISCARD)
    {
      BDEBUG("Non-OAM packet discarded %s\n", skb->dev->name);
      return NET_RX_DROP;
    }
  else if (port->local_par_action == EFM_OAM_PAR_LB)
    {
      BDEBUG ("calling port xmit\n");
      dev->trans_start = jiffies;
      if ((skb2 = skb_copy (skb, GFP_ATOMIC)) == NULL)
        {
          read_unlock (&efm_port_lock);
          return NET_RX_DROP;
        }
      skb_push (skb2, ETH_HLEN);
      ret = port->xmit_fcn(skb2, dev);
      BR_READ_UNLOCK (&efm_port_lock);
      return NET_RX_DROP;
    }

  return NET_RX_SUCCESS;
}

/* Put the port with the given name under EFM control. */
int
efm_add_port (const int ifindex)
{
  struct efm_port * port = 0;
  struct net_device * dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);

  if (dev == 0)
    return -ENOENT;

  BR_WRITE_LOCK (&efm_port_lock);
  port = efm_find_port (dev->ifindex);

  if (port == 0)
    {
      port = kmalloc (sizeof (struct efm_port), GFP_ATOMIC);

      if (port)
        {
          port_init (port, dev);
          l2_mod_inc_use_count ();
          dev->rx_hook = efm_recv;
          spin_lock (&dev->tx_global_lock);
          dev->hard_start_xmit = efm_xmit;
          spin_unlock (&dev->tx_global_lock);
        }
      else
        {
          BWARN("insufficient memory for %s\n", dev->name);
          dev_put (dev);
          BR_WRITE_UNLOCK (&efm_port_lock);
          return -ENOMEM;
        }
    }
  else
    {
      dev_put (dev);
      BR_WRITE_UNLOCK (&efm_port_lock);
      return -EEXIST;
    }

  dev_put (dev);
  BR_WRITE_UNLOCK (&efm_port_lock);
  return 0;
}

/* Remove the port with the given name from EFM control. */
int 
efm_remove_port (const int ifindex)
{
  struct efm_port * port = 0;
  struct net_device * dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);

  if (dev == 0)
    return -ENOENT;

  BDEBUG("%s\n", dev->name);
  BR_WRITE_LOCK (&efm_port_lock);
  port = efm_find_port (dev->ifindex);

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
  BR_WRITE_UNLOCK (&efm_port_lock);
  return 0;
}

void
efm_remove_all_ports (void)
{
  register struct efm_port * port;
  register struct efm_port * next;
  struct net_device * dev;
  int hash;
  BR_WRITE_LOCK (&efm_port_lock);

  for (hash = 0; hash < EFM_PORT_HASH_SIZE; hash++)
    {
      for (port = efm_port_hash[hash]; port; port = next) 
        {
          efm_port_hash[hash] = next = port->next;
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

  BR_WRITE_UNLOCK (&efm_port_lock);
}

/* Set the port state.  */
int
efm_set_port_state (const int ifindex, enum efm_local_par_action local_par_action,
                    enum efm_local_mux_action local_mux_action)
{
  struct net_device *dev = dev_get_by_index (current->nsproxy->net_ns, ifindex);
  struct efm_port *port = 0;
  int state = 0;

  if (dev == 0)
    return -ENOENT;
    
  BDEBUG("port %d dir %d\n", ifindex, state);
  BR_WRITE_LOCK (&efm_port_lock);

  port = efm_find_port (ifindex);

  if (port == NULL)
    {
      BR_WRITE_UNLOCK (&efm_port_lock);
      efm_add_port (dev->ifindex);
      BR_WRITE_LOCK (&efm_port_lock);
      port = efm_find_port (ifindex);

    }

  if (port != NULL)
    {
      port->local_par_action = local_par_action;
      port->local_par_action = local_mux_action;
      dev_put (dev);
      BR_WRITE_UNLOCK (&efm_port_lock);
      return 0;
    }

  dev_put (dev);
  BR_WRITE_UNLOCK (&efm_port_lock);

  return 0;
}
