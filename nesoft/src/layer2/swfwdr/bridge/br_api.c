/*
  Userspace interface
  Linux ethernet bridge
  
  Authors:
  Lennert Buytenhek   <buytenh@gnu.org>
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
*/
#include <linux/autoconf.h>
#include <linux/if_arp.h>
#include <linux/kernel.h>
#include <linux/inetdevice.h>
#include <asm/uaccess.h>
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_fdb.h"
#include "br_api.h"
#include "br_vlan_api.h"
#include "br_stp.h"
#include "br_pro_vlan.h"
#include "bdebug.h"

extern int
br_vlan_insert_into_static_vlan_reg_table (struct apn_bridge *br, vid_t vid, 
                                           unsigned int port_no, unsigned char egress_tagged);

extern int
br_vlan_delete_from_static_vlan_reg_table (struct apn_bridge *br, vid_t vid, 
                                           unsigned int port_no);

static struct apn_bridge * bridge_list = 0;
static DEFINE_RWLOCK(apn_bridge_lock);
static short num_bridges = 0;

/* 
   This function deletes a single interface from a bridge instance. 
   It is called under bridge lock. 
*/
static int 
br_delete_if (struct apn_bridge *br, struct net_device *dev)
{
  struct apn_bridge_port *port;
  struct apn_bridge_port **pptr =NULL;
  unsigned char found = 0;

  if ((port = (struct apn_bridge_port *)dev->apn_fwd_port) == NULL)
    {
      BDEBUG("delete_if: bridge port not assigned to %s\n", dev->name);
      return -EINVAL;
    }

  if (!br)
    {
      BDEBUG("delete_if: Bridge is null\n");
      return -EINVAL;
    }

  BDEBUG("delete if %s on bridge %s\n", dev->name, br->name);

  if (port->reg_tab)
    br_vlan_reg_tab_delete (port);

  if (port->trans_tab)
    br_vlan_trans_tab_delete (port);

  if (port->pro_edge_tab)
    br_vlan_pro_edge_tab_delete (port);

  if (!rtnl_is_locked()) 
    rtnl_trylock();
  dev_set_promiscuity (dev, -1);
  if (rtnl_is_locked())
    rtnl_unlock();

  dev->apn_fwd_port = NULL;

  BR_WRITE_LOCK (&apn_bridge_lock);

  pptr = &br->port_list;
  while (*pptr != NULL)
    {
      if (*pptr == port)
        {
          *pptr = port->next;
          found = 1;
          break;
        }

      pptr = &((*pptr)->next);
    }

  if (found)
    {
      br->num_ports--;
      br_fdb_delete_all_by_port (br, port);
      /* Unset port_index */
      br->port_id[port->port_id] = 0;
      kfree (port);
      port = 0;
    }
  else
    {
      BDEBUG ("port not found in portlist \n");
    }

  BR_WRITE_UNLOCK(&apn_bridge_lock);

  dev_put (dev);

  return 0;
}


/* This function deletes all interfaces associated withh a bridge instance. */

static void 
delete_all_interfaces (struct apn_bridge *br)
{
  while (br->port_list != NULL)
    br_del_if (br, br->port_list->dev);
}

/* This function allocates and initializes a bridge instance. */

static struct apn_bridge *
allocate_bridge (char * name, struct sock * sk, unsigned is_vlan_aware,
                 int type, unsigned char edge, unsigned char beb)
{
  struct apn_bridge *br;
  struct timer_list *timer;
  int i;
  
  BDEBUG("allocate bridge %s\n",name);

  if ((br = kmalloc (sizeof(struct apn_bridge), GFP_KERNEL)) == NULL)
    return NULL;

  memset (br, 0, sizeof(struct apn_bridge));

  rwlock_init (&br->lock);
  rwlock_init (&br->hash_lock);
  rwlock_init (&br->static_hash_lock);

  /* We use strncpy here so as to not copy trailing 
     trash into the kernel data structure. */
  strncpy(br->name, name, IFNAMSIZ-1);
  br->name[IFNAMSIZ] = 0;

  /* How long are entries kept if not seen. */
  br->ageing_time = BRIDGE_TIMER_DEFAULT_AGEING_TIME * HZ;
  br->edge = edge;
 
  /* Have the bridge learn and enabled by dfault */  
  br->flags = APNBR_UP | APNBR_LEARNING_ENABLED;
  /* How often do we check to see if dynamic entries should be discarded. */
  br->dynamic_ageing_interval = BRIDGE_TIMER_DEFAULT_AGEING_INTERVAL * HZ;

  /* Init kernel timer for dynamic ageing */
  timer = &br->tick;
  init_timer (timer);

  timer->data = (unsigned long) br;
  timer->function = br_tick;
  timer->expires = jiffies + 1;
  add_timer(timer);

  br_timer_set (&br->dynamic_aging_timer, jiffies);

  br->num_ports = 0;
  br->num_static_entries = 0;
  br->num_dynamic_entries = 0;
  br->num_ce_ports = 0;
  br->num_cn_ports = 0;
  br->num_cnp_ports = 0;
  br->num_pip_ports = 0;
  br->edge = edge;
  br->beb = beb;

  /* Set whether the bridge is a vlan-ware or transparent bridge */
  br->is_vlan_aware = is_vlan_aware;
  br->type = type;

  /* Set port index to 0 */
  br->port_index = 0;
  for (i=0; i < BR_MAX_PORTS; i++)
    br->port_id[i] = 0;
  
  /* Intialize the vlan specific tables. This is valid for vlan-ware bridge 
     but will be used for tranparent bridge */
  /* Intialize the vlan_info_table */
  memset(br->vlan_info_table, '\0', 
         sizeof (struct vlan_info_entry*) * (VLAN_MAX_VID + 1));

  memset(br->svlan_info_table, '\0', 
         sizeof (struct vlan_info_entry*) * (VLAN_MAX_VID + 1));

  /* Intialize the static_vlan_reg_table */
  memset(br->static_vlan_reg_table, '\0', sizeof (char *) * (VLAN_MAX_VID + 1));
  memset(br->static_svlan_reg_table, '\0', sizeof (char *) * (VLAN_MAX_VID + 1));

  return br;
}

void
br_link_bridge(struct apn_bridge *new)
{
  BR_WRITE_LOCK (&apn_bridge_lock);
 
  new->next = bridge_list;
  if (new->next != NULL)
    new->next->pprev = &new->next;
  bridge_list = new;
  new->pprev = &bridge_list;
  
  BR_WRITE_UNLOCK (&apn_bridge_lock);
}

void
br_unlink_bridge(struct apn_bridge *curr)
{
  BR_WRITE_LOCK (&apn_bridge_lock);
  
  *(curr->pprev) = curr->next;
  if (curr->next != NULL)
    curr->next->pprev = curr->pprev;
  curr->next = NULL;
  curr->pprev = NULL;
  
  BR_WRITE_UNLOCK (&apn_bridge_lock);
}



struct apn_bridge *
br_find_bridge(char *name)
{
  struct apn_bridge *curr = bridge_list;
  
  while (curr)
    {
      if(strncmp(curr->name,name,IFNAMSIZ) ==0)
        return curr;
      
      curr = curr->next;
    }

  return NULL;
}

struct apn_bridge *
br_get_default_bridge (void)
{
  return bridge_list;
}

/* 
   allocate_bridge_port allocates and initializes a port on the specified
   bridge instance. The specified device is attached to the port.
   This routine is called under bridge lock. 
*/
static struct apn_bridge_port *
allocate_bridge_port (struct apn_bridge * br, struct net_device * dev)
{
  struct apn_bridge_port * port = kmalloc (sizeof(*port), GFP_ATOMIC);
  unsigned char priority;

  if (port == NULL)
    return port;

  BDEBUG("allocate port %s on bridge %s\n",dev->name, br->name);

  memset (port, 0, sizeof(struct apn_bridge_port));


  if ( br_avl_create (&port->trans_tab, 0, br_vlan_trans_tab_ent_cmp) != 0)
    {
      kfree (port);
      return NULL;
    }

  if ( br_avl_create (&port->rev_trans_tab, 0,
                      br_vlan_rev_trans_tab_ent_cmp) != 0)
    {
      kfree (port);
      return NULL;
    }

  if ( br_avl_create (&port->reg_tab, 0,
                      br_vlan_reg_tab_ent_cmp) != 0)
    {
      kfree (port);
      return NULL;
    }

  if ( br_avl_create (&port->pro_edge_tab, 0,
                      br_vlan_pro_edge_port_tab_ent_cmp) != 0)
    {
      kfree (port);
      return NULL;
    }

  port->br = br;
  port->dev = dev;
  dev->apn_fwd_port = port;

  /* Use ifindex of device here for sanity  */
  port->port_no = dev->ifindex;
  
  /* Assign ID for this port */
  port->port_id = br_vlan_get_port_id(br);
  
  /* set the port type to access port by default for transperent bridge and
     vlan-aware bridge */
  port->port_type = ACCESS_PORT;

  port->port_sub_type = ACCESS_PORT;

  /* set the admit tagged frame to default, will not be used for 
     transparent bridge */
  port->acceptable_frame_types = VLAN_ADMIT_ALL_FRAMES;

  /* set the enable ingress filter frame to default, will not be used for 
     transparent bridge */
  port->enable_ingress_filter = 0;

  /* set the enable vid_swap to default, will not be used for 
     transparent bridge */
  port->enable_vid_swap = 0;

  if (br->is_vlan_aware) /* vlan-aware bridge */
    {
      port->default_pvid =  VLAN_DEFAULT_VID;
    }
  else /* transparent bridge */
    {
      port->default_pvid =  VLAN_NULL_VID;
    }

  /* Add to head of port list. */
  port->next = br->port_list;
  br->port_list = port;

  return port;
}


/* br_add_bridge allocates and initializes a named bridge instance. */
int 
br_add_bridge (char *name, struct sock * sk, unsigned is_vlan_aware,
               int protocol, unsigned char edge, unsigned char beb)
{
  struct apn_bridge *br;

  if(num_bridges > BR_MAX_BRIDGES)
    return -EMFILE;

  if ((br_find_bridge(name)) != NULL)
    return -EEXIST;

  if ((br = allocate_bridge (name, sk, is_vlan_aware, protocol, edge, beb)) == NULL)
    return -ENOMEM;

  /* Put it at the head of the list. */
  br_link_bridge(br);

  num_bridges++;
  l2_mod_inc_use_count();

  BDEBUG ("is_vlan_aware(%u)\n", is_vlan_aware);

  if (is_vlan_aware)
    {
      br_vlan_init (br);
    }

  return 0;
}

/* This function deletes the named bridge instance if it exists. */
int 
br_del_bridge (char *name, struct sock * sk)
{
  struct apn_bridge * br;
  
  BDEBUG ("delete bridge %s\n", name);
  
  if ((br = br_find_bridge(name)) == NULL)
    return -ENXIO;
  
  delete_all_interfaces (br);

  /* Delete the dynamic ageing timer */
  del_timer (&br->tick);
  br_timer_clear (&br->dynamic_aging_timer);
  br_fdb_cleanup (br);
  
  if (br->is_vlan_aware)
    br_vlan_uninit (br);
  
  br_unlink_bridge(br);
  num_bridges--;
  kfree(br);
  l2_mod_dec_use_count ();
  
  return 0;
}

/* This function adds the device instance to the specified bridge instance. */
int 
br_add_if (struct apn_bridge *br, struct net_device *dev)
{
  struct apn_bridge_port *p;
  int is_local = 1;

  if (dev->apn_fwd_port != NULL)
    return -EBUSY;

  /* Cannot be loopback and must be ethernet device */
  if (dev->flags & IFF_LOOPBACK || dev->type != ARPHRD_ETHER)
    return -EINVAL;

  if(br->num_ports > BR_MAX_PORTS)
    return -EMFILE;

  BDEBUG("add if %s to bridge %s\n", dev->name, br->name);

  dev_hold(dev);
  BR_WRITE_LOCK_BH(&br->lock);

  if ((p = allocate_bridge_port(br, dev)) == NULL)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      dev_put (dev);
      return -EXFULL;
    }
 
  if (!rtnl_is_locked()) 
    rtnl_trylock();
  dev_set_promiscuity (dev, 1);
  if (rtnl_is_locked())
    rtnl_unlock();

  br->num_ports++;

  br_fdb_insert (br, p, dev->dev_addr, p->default_pvid, p->default_pvid,
                 is_local, BR_TRUE);

  BR_WRITE_UNLOCK_BH (&br->lock);

  if (br->is_vlan_aware)
    {
      unsigned char egress_tagged = 0;
      br_vlan_insert_into_static_vlan_reg_table (br, p->default_pvid, 
                                                 p->port_no,
                                                 egress_tagged); 
    }

  return 0;
}

/* 
   This function deletes the specified interface from the specified bridge. 
   Most of the work is done by br_delete_if. 
*/

int 
br_del_if (struct apn_bridge *br, struct net_device *dev)
{
  int retval;
  BDEBUG("del if %s from bridge %s\n", dev->name, br->name);
  BR_WRITE_LOCK_BH (&br->lock);
  if (br->is_vlan_aware)
    {
      /* assumption is dev->apn_fwd_port can never be destroyed as the global
         lock is being held */
      struct apn_bridge_port *port = dev->apn_fwd_port;

      if (port)
        {
          BR_WRITE_UNLOCK_BH (&br->lock);
          br_vlan_delete_all_vids_from_port(br->name, port->port_no);
          BR_WRITE_LOCK_BH (&br->lock);
        }
    }
  /* retval = br_del_temp_if (br, dev); */
  retval = br_delete_if (br, dev); 
  
  BR_WRITE_UNLOCK_BH (&br->lock);
  return retval;
}

int
br_cleanup_bridges(struct sock *sk)
{
  struct apn_bridge *curr;
  struct apn_bridge *next;
  curr = bridge_list;
  while(curr)
    {
      next = curr->next;
      br_del_bridge(curr->name, sk);
      curr = next;
    }
  return 0;
}

/* Return a list of ifindices used by bridges. */
int 
br_get_bridge_names (char * name_list, int num)
{
  register int i;
  struct apn_bridge * br = bridge_list;

  for (i = 0; i < num; i++)
    {
      if (br == NULL)
        break;
        
      memcpy(name_list, br->name, IFNAMSIZ);
      name_list += IFNAMSIZ;
      
      br = br->next;
    }
  return i;
}

/* Return a list of ifindices used by the ports of a brigde. 
   This is called under ioctl_lock. 
*/
int
br_get_port_ifindices (struct apn_bridge *br, int *ifindices, int num)
{
  struct apn_bridge_port * port = br->port_list;
  int i;
    
  for (i = 0 ; i < num ; i++)
    {
      if (!port)
        break;
      
      ifindices[i] = port->dev->ifindex;
      port = port->next;
    }

  return i;
}

/* This function retruns a pointer to the port with the specified id.
 *    It is called under ioctl_lock or bridge lock */
struct apn_bridge_port *
br_get_port (struct apn_bridge * br, int port_no)
{
  struct apn_bridge_port * p = br->port_list;
   
  BDEBUG("br_get_port: retrieving port %d\n",port_no);
  while (p != NULL)
    {
      if (p->port_no == port_no)
        return p;
      p = p->next;
    }
  return NULL;

}

int
br_port_protocol_process (char *bridge_name, unsigned int port_no,
                          unsigned int protocol, unsigned int process,
                          vid_t vid)
{

  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  struct br_vlan_trans_key *tmpkey;
  struct br_vlan_trans_key key;
  struct br_avl_node *node;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port )
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  switch (protocol)
    {
    case APNBR_PROTO_STP:
    case APNBR_PROTO_RSTP:
    case APNBR_PROTO_MSTP:
      port->proto_process.stp_process = process;
      if (process == APNBR_L2_PROTO_TUNNEL)
        port->proto_process.stp_tun_vid = vid;
      break;

    case APNBR_PROTO_GMRP:
      port->proto_process.gmrp_process = process;
      break;

    case APNBR_PROTO_MMRP:
      port->proto_process.mmrp_process = process;
      break;

    case APNBR_PROTO_GVRP:
      port->proto_process.gvrp_process = process;
      if (process == APNBR_L2_PROTO_TUNNEL)
        port->proto_process.gvrp_tun_vid = vid;
      break;

    case APNBR_PROTO_MVRP:
      port->proto_process.mvrp_process = process;
      if (process == APNBR_L2_PROTO_TUNNEL)
        port->proto_process.mvrp_tun_vid = vid;
      break;

    case APNBR_PROTO_LACP:
      port->proto_process.lacp_process = process;
      port->proto_process.lacp_process = process;
      if (process == APNBR_L2_PROTO_TUNNEL)
        port->proto_process.lacp_tun_vid = vid;
      break;

    case APNBR_PROTO_DOT1X:
      port->proto_process.dot1x_process = process;
      if (process == APNBR_L2_PROTO_TUNNEL)
        port->proto_process.dot1x_tun_vid = vid;
      break;

    default:
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;

}
