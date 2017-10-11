/* Copyright (C) 2003  All Rights Reserved.
 *   
 * 802.1Q Vlan-Aware Bridge 
 *      
 * Author:  srini
 *         
 */
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include "if_ipifwd.h"
#include "br_vlan_api.h"
#include "br_api.h"
#include "br_vlan_dev.h"
#include "bdebug.h"
#include <config.h>


/* This static function inserts the vlan information into the vlan_info table */
static __inline__ int
br_vlan_insert_into_vlan_info_table (struct apn_bridge *br,
                                     enum vlan_type type, vid_t vid)
{
  struct vlan_info_entry *vlan_info_entry;

  if (type == CUSTOMER_VLAN)
    vlan_info_entry = br->vlan_info_table[vid];
  else
    vlan_info_entry = br->svlan_info_table[vid];

  if (!vlan_info_entry) 
    {
      BDEBUG ("vlan_info_entry doesn't exist: vid(%u)\n", vid);

      vlan_info_entry = kmalloc (sizeof (struct vlan_info_entry), GFP_ATOMIC);

      if (!vlan_info_entry)
        {
          BDEBUG ("Memory allocation failure for vlan_info_entry\n");
          return -ENOMEM;
        }
      BDEBUG("creating vlan info entry  %d  for br  %s \n",vid,br->name); 
      memset (vlan_info_entry, 0, sizeof (struct vlan_info_entry));
      /* Adding it to common instance by default */
      vlan_info_entry->instance = 0;
      vlan_info_entry->vid = vid;
      vlan_info_entry->br = br;
      vlan_info_entry->mtu_val = 0;

      if (type == CUSTOMER_VLAN)
        br->vlan_info_table[vid] = vlan_info_entry;
      else
        br->svlan_info_table[vid] = vlan_info_entry;
    }

  return 0;
}


/* This static function deletes the vlan information from the vlan_info table */
static __inline__ int
br_vlan_delete_from_vlan_info_table (struct apn_bridge *br,
                                     enum vlan_type type, vid_t vid)
{
  struct vlan_info_entry *vlan_info_entry;

  if (type == CUSTOMER_VLAN)
    vlan_info_entry = br->vlan_info_table[vid];
  else
    vlan_info_entry = br->svlan_info_table[vid];

  if (!vlan_info_entry) 
    {
      BDEBUG ("vlan_info_entry doesnt exist: vid(%u)\n", vid);
      return -EINVAL;
    }

  kfree (vlan_info_entry);

  if (type == CUSTOMER_VLAN)
    br->vlan_info_table[vid] = 0;
  else
    br->svlan_info_table[vid] = 0;

  BDEBUG ("vlan_info_entry freed\n");

  return 0;
}

/* This function retruns a pointer to the port with the specified port_no. */
struct apn_bridge_port *
br_vlan_get_port (struct apn_bridge *br, int port_no)
{
  struct apn_bridge_port *p;

  BR_READ_LOCK (&br->lock);

  p = br->port_list;

  while (p != NULL)
    {
      if (p->port_no == port_no)
        {
          BR_READ_UNLOCK (&br->lock);

          return p;
        }
      p = p->next;
    }

  BR_READ_UNLOCK (&br->lock);
  return NULL;
}

int
br_vlan_get_port_id (struct apn_bridge *br)
{
  int i, idx;
  
  BR_READ_LOCK (&br->lock);
  
  for (i = 0; i < BR_MAX_PORTS; i++)
    {
      idx = (br->port_index + i) % BR_MAX_PORTS;
      
      if (!br->port_id[idx])
        {
          br->port_index = idx;
          br->port_id[idx] = 1;
          
          BR_READ_UNLOCK (&br->lock);
          return idx;
        }
    }
  BR_READ_UNLOCK (&br->lock);
  
  return -1;
}

/* This function retruns a pointer to the port with the specified port_no. */
struct apn_bridge_port *
br_vlan_get_port_by_no (struct apn_bridge *br, int port_no)
{
  struct apn_bridge_port *p;
  
  BR_READ_LOCK (&br->lock);
  
  p = br->port_list;
  
  while (p != NULL)
    {
      if (p->port_no == port_no)
        {
          BR_READ_UNLOCK (&br->lock);
        
          return p;
        }
      p = p->next;
    }
  
  BR_READ_UNLOCK (&br->lock);
  return NULL;
}

/* This function retruns a pointer to the port with the specified port_id. */
struct apn_bridge_port *
br_vlan_get_port_by_id (struct apn_bridge *br, int port_id)
{
  struct apn_bridge_port *p;
  
  BR_READ_LOCK (&br->lock);
  
  p = br->port_list;
  
  while (p != NULL)
    {
      if (p->port_id == port_id)
        {
          BR_READ_UNLOCK (&br->lock);
        
          return p;
        }
      p = p->next;
    }
  
  BR_READ_UNLOCK (&br->lock);
  return NULL;
}

/* This function retruns a pointer to the port with the specified hw addr. */
struct apn_bridge_port *
br_vlan_get_port_by_hw_addr (struct apn_bridge *br, unsigned char *hw_addr)
{
  struct apn_bridge_port *p;
  
  BR_READ_LOCK (&br->lock);
  
  p = br->port_list;
  
  while (p != NULL)
    {
      if (!memcmp (hw_addr, p->dev->dev_addr, p->dev->addr_len))
        {
          BR_READ_UNLOCK (&br->lock);
        
          return p;
        }
      p = p->next;
    }
  
  BR_READ_UNLOCK (&br->lock);
  return NULL;
}

/* This static function gets the bitmap of vlans that are configured on the
   given port */
void
br_vlan_get_config_vids (struct apn_bridge *br, unsigned int port_no,
                         char *config_vids)
{
  vid_t vid;
  unsigned char *config_ports = 0;
  struct apn_bridge_port *port;

  port = br_vlan_get_port (br, port_no);
  if (! port)
    return;

  /* TAKE READ LOCK */
  BR_READ_LOCK (&br->lock);

  for (vid = VLAN_DEFAULT_VID; vid <= VLAN_MAX_VID; vid++)
    {
      config_ports = br->static_vlan_reg_table[vid];

      if (!config_ports)
        {
          continue;
        }

      if (IS_VLAN_PORT_CONFIGURED (config_ports[port->port_id]))
        {
          BR_VLAN_SET_VLAN_BITMAP (vid, config_vids);
        }
    }
  /* RELEASE READ LOCK */
  BR_READ_UNLOCK (&br->lock);
}

/* This function changes the vlan device flags */
void
br_vlan_update_vlan_dev (struct apn_bridge *br, struct apn_bridge_port *port,
                          int instance)
{
  return br_vlan_dev_change_flags (br, port, instance);
}

/* This function inserts the vid, port info (port_no, egress_tagged) into the
   static vlan registration table */
int
br_vlan_insert_into_static_vlan_reg_table (struct apn_bridge *br, vid_t vid, 
                                           unsigned int port_no, unsigned char egress_tagged)
{
  char *config_ports;
  struct apn_bridge_port *port;
  struct vlan_info_entry *vinfo;

  /* TAKE WRITE LOCK */
  BR_WRITE_LOCK (&br->lock);

  port = br_vlan_get_port (br, port_no);

  if (! port)
    {
      BR_WRITE_UNLOCK (&br->lock);
      return -1;
    }

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    {
      config_ports = br->static_svlan_reg_table[vid];
      vinfo = br->svlan_info_table[vid];
    }
  else
    {
      config_ports = br->static_vlan_reg_table[vid];
      vinfo = br->vlan_info_table[vid];
    }

  if (!config_ports)
    {
      BDEBUG ("config_ports doesnt exist: vid(%u), port_no(%d), "
              "egress_tagged(%u)\n", vid, port_no, egress_tagged);

      config_ports = kmalloc (BR_MAX_PORTS * sizeof (char), GFP_ATOMIC);

      if (!config_ports)
        {
          BDEBUG ("Memory allocation failure for config_ports\n");
          /* RELEASE WRITE LOCK */
          BR_WRITE_UNLOCK (&br->lock);
          return -ENOMEM;
        }

      memset (config_ports, 0, BR_MAX_PORTS * sizeof (char));

      if (port->port_type == PRO_NET_PORT
          || port->port_type == CUST_NET_PORT)
        br->static_svlan_reg_table[vid] = config_ports;
      else
        br->static_vlan_reg_table[vid] = config_ports;
    }

  config_ports[port->port_id] |= VLAN_PORT_CONFIGURED;
  config_ports[port->port_id] |= VLAN_REGISTRATION_FIXED;

  if (egress_tagged) 
    {
      config_ports[port->port_id] |= VLAN_EGRESS_TAGGED;
    }

/* RELEASE WRITE LOCK */
  BR_WRITE_UNLOCK (&br->lock);

  if (vinfo != NULL)
    br_vlan_dev_add_port (vinfo, port);

  BDEBUG ("Inserted into static_vlan_reg_table: vid(%u), port_no(%d), "
          "egress_tagged(%u)\n", vid, port_no, egress_tagged);
  return 0;
}


/* This function deletes the port for the given vid from the static vlan 
   registration table */
int
br_vlan_delete_from_static_vlan_reg_table (struct apn_bridge *br, vid_t vid, 
                                           unsigned int port_no)
{
  char *config_ports;
  enum vlan_type type;
  struct apn_bridge_port *port;
  struct vlan_info_entry *vinfo;

  /* TAKE WRITE LOCK */
  BR_WRITE_LOCK (&br->lock);

  port = br_vlan_get_port (br, port_no);

  if (! port)
    {
      BR_WRITE_UNLOCK (&br->lock);
      return -EINVAL;
    }

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    {
      type = SERVICE_VLAN;
      vinfo = br->svlan_info_table[vid];
      config_ports = br->static_svlan_reg_table[vid];
    }
  else
    {
      type = CUSTOMER_VLAN;
      vinfo = br->vlan_info_table[vid];
      config_ports = br->static_vlan_reg_table[vid];
    }

  if (!config_ports)
    {
      BR_WRITE_UNLOCK(&br->lock);
      BDEBUG ("config_ports doesnt exists: vid(%u), port_no(%u)\n", vid, 
              port_no);
      return -EINVAL;
    }

  if (config_ports[port->port_id] & VLAN_PORT_UNCONFIGURED)
    {
      BR_WRITE_UNLOCK (&br->lock);
      BDEBUG ("port is unconfigured: vid(%u), port_no(%u)\n", vid, port_no);
      return -EINVAL;
    }

  config_ports[port->port_id] &= VLAN_PORT_UNCONFIGURED;

  if (vinfo != NULL)
    br_vlan_dev_del_port (vinfo, type, port);

  BDEBUG ("Deleted from static_vlan_reg_table: vid(%u), port_no(%u)\n", vid,
          port_no);

  /* Delete all the dynamic fdb entries associated with this port
   * on this vlan */
  if (vinfo != NULL)
    br_fdb_delete_dynamic_by_vlan (br, type, vid);

  BR_WRITE_UNLOCK (&br->lock);

  return 0;
}


/* This static function checks if there are any ports configured with the
   given vid */
static __inline__ bool_t
br_vlan_is_ports_configured (struct apn_bridge *br, enum vlan_type type,
                             vid_t vid)
{
  /* FUTURE: This implementation should change once the vid->ports data
     structures gets fixed in a better manner. This will happen only after
     fixing bridge->ports data structure */
  char *config_ports;
  struct apn_bridge_port *port=0;
  bool_t is_configured = BR_FALSE;

  /* TAKE READ LOCK */
  BR_READ_LOCK (&br->lock);

  if (type == CUSTOMER_VLAN)
     config_ports = br->static_vlan_reg_table[vid];
  else
     config_ports = br->static_svlan_reg_table[vid];

  if (!config_ports)
    {
      BDEBUG ("No ports are configured for the given vid (%u)\n", vid);
      BR_READ_UNLOCK (&br->lock);
      return is_configured;
    }

  port = br->port_list;

  while (port)
    {
      if (type == SERVICE_VLAN
          && port->port_type != PRO_NET_PORT
          && port->port_type != CUST_NET_PORT)
        {
          port = port->next;
          continue;
        }

      if (config_ports[port->port_id] & VLAN_PORT_CONFIGURED)
        {
          is_configured = BR_TRUE;
          break;
        }

      port = port->next;
    }

  /* RELEASE READ LOCK */
  BR_READ_UNLOCK (&br->lock);

  BDEBUG ("is_port_configured(%u)\n", is_configured);
  return is_configured;
}

/* Vlan related br functions */
/* This function adds a new vlan entry for te given bridge */
int
br_vlan_add (char *bridge_name, enum vlan_type type, vid_t vid)
{
  struct apn_bridge *br = 0;
  struct vlan_info_entry *vlan_info_entry;
  int ret;

  BDEBUG("Entering br_vlan_add\n");

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_WRITE_LOCK (&br->lock);

  if (br_vlan_insert_into_vlan_info_table (br, type, vid) < 0)
    {
      BDEBUG ("Insertion into vlan_info_table failed: vid(%u)\n", 
              vid); 
      br_vlan_delete_from_vlan_info_table (br, type, vid);
      BR_WRITE_UNLOCK (&br->lock);
      return -EINVAL;
    }

  if (type == CUSTOMER_VLAN)
    vlan_info_entry = br->vlan_info_table[vid];
  else
    vlan_info_entry = br->svlan_info_table[vid];

  if (!vlan_info_entry)
    {
      BDEBUG ("vlan_info_entry doesnt exist: vid(%u)\n", vid);
      br_vlan_delete_from_vlan_info_table (br, type, vid);
      BR_WRITE_UNLOCK (&br->lock);
      return -EINVAL;
    }

  if (br->edge == BR_TRUE)
    {
      BR_WRITE_UNLOCK (&br->lock);
      return 0;
    }

  /* Register the vlan interface with the kernel */
  if (ret = br_vlan_dev_create (vlan_info_entry, type))
    {
      BDEBUG ("Unable to create vlan interface: vid(%u)\n", vid);
      br_vlan_delete_from_vlan_info_table (br, type, vid);
      BR_WRITE_UNLOCK (&br->lock);
      return ret;
    }

  BDEBUG ("Added new vlan: vid(%u)\n", vid);
  BR_WRITE_UNLOCK (&br->lock);
  return 0;
}


/* This function deletes an existing vlan entry for te given bridge */
int
br_vlan_delete (char *bridge_name,  enum vlan_type type, vid_t vid)
{
  struct apn_bridge *br = 0;
  unsigned char *config_ports = 0;
  struct vlan_info_entry *vlan_info_entry;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_WRITE_LOCK (&br->lock);

  /* unregister the vlan interface with the kernel */
  if (type == CUSTOMER_VLAN)
    {
      vlan_info_entry = br->vlan_info_table[vid];
      config_ports = br->static_vlan_reg_table[vid];
    }
  else
    {
      vlan_info_entry = br->svlan_info_table[vid];
      config_ports = br->static_svlan_reg_table[vid];
    }

  if (!vlan_info_entry)
    {
      BDEBUG ("vlan_info_entry doesnt exist: vid(%u)\n", vid);
      /*  RELEASE WRITE LOCK */
      BR_WRITE_UNLOCK (&br->lock);
      return -EINVAL;
    }

  /* Check if there are any ports configured */
  if (br_vlan_is_ports_configured (br, type, vid)) 
    {
      BDEBUG ("Ports are configured on this vlan: vid(%u)\n", vid);
      BR_WRITE_UNLOCK (&br->lock);
      return -EINVAL;
    }

  /* Delete all dynamic fdb on this vlan */

  br_fdb_delete_dynamic_by_vlan (br, type, vid);

  br_vlan_dev_destroy (vlan_info_entry);

  if (br_vlan_delete_from_vlan_info_table (br, type, vid) < 0)
    {
      BDEBUG ("Deletion from vlan_info_table failed: vid(%u)\n", vid);
      BR_WRITE_UNLOCK (&br->lock);
      return -EINVAL;
    }


  kfree (config_ports);

  if (type == CUSTOMER_VLAN)
     br->static_vlan_reg_table[vid] = NULL;
  else
     br->static_svlan_reg_table[vid] = NULL;

  BDEBUG ("Deleted vlan: vid(%u)\n", vid);
  BR_WRITE_UNLOCK (&br->lock);
  return 0;
}

/* This function port type and its associted charectistices (frame-type,
   ingress_filter) for the given potrt type */
int
br_vlan_set_port_type (char *bridge_name, unsigned int port_no,
                       port_type_t port_type,
                       port_type_t port_sub_type,
                       unsigned short acceptable_frame_types,
                       unsigned short enable_ingress_filter)
{
  enum vlan_type type;
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    type = SERVICE_VLAN;
  else
    type = CUSTOMER_VLAN;

  if (port->port_type != port_type
     || port->port_sub_type != port_sub_type)
    {
      unsigned char egress_tagged = 0;

      /* Flush out old dynamic fdb entries as the port_type has changed. */
      br_fdb_delete_dynamic_by_port (br, port, 0, port->default_pvid,
                                     port->default_pvid);

      if (port->default_pvid != VLAN_DEFAULT_VID)
        {
          BR_WRITE_UNLOCK_BH (&br->lock);

          if (br_vlan_delete_from_static_vlan_reg_table (br, port->default_pvid,
                                                         port_no) < 0)
            {
              BDEBUG ("deletion from static_vlan_reg_table failed: "
                      "vid(%u), port_no(%d)\n", port->default_pvid, port_no);
              return -EINVAL;
            }

          BR_WRITE_LOCK_BH (&br->lock);
        }

      port->acceptable_frame_types = acceptable_frame_types ? 
        VLAN_ADMIT_TAGGED_ONLY : VLAN_ADMIT_ALL_FRAMES;
      port->default_pvid = VLAN_DEFAULT_VID;
      port->enable_ingress_filter = enable_ingress_filter;

      /* VLAN IS Egress Untagged if it is access port or it is DEFAULT VLAN */

      if ( (port_type == ACCESS_PORT) || (port_type == HYBRID_PORT) )
        {
          egress_tagged = 0;
        }
      else if (port_type == CUST_EDGE_PORT)
        {
          if ( (port_sub_type == ACCESS_PORT) ||
               (port_sub_type == HYBRID_PORT) )
            egress_tagged = 0;
          else
            egress_tagged = 1;
        }
      else
        egress_tagged = 1;

      BR_WRITE_UNLOCK_BH (&br->lock);

      if (br_vlan_insert_into_static_vlan_reg_table (br, port->default_pvid,
                                                     port_no, egress_tagged) < 0)
        {
          BDEBUG ("insertion into static_vlan_reg_table failed: "
                  "vid(%u), port_no(%d)\n", port->default_pvid, port_no);
          return -EINVAL;
        }

      if (port->port_type != port_type)
        {
          if (port_type == CUST_EDGE_PORT)
            br->num_ce_ports++;

          if (port->port_type == CUST_EDGE_PORT)
            br->num_ce_ports--;

          if (port_type == CUST_NET_PORT)
            br->num_cn_ports++;

          if (port->port_type == CUST_NET_PORT)
            br->num_cn_ports--;
        }

      if ((br->type == BR_TYPE_PROVIDER_MSTP ||
            br->type == BR_TYPE_PROVIDER_RSTP )
            && br->beb )
        {
           if (port_type == PRO_NET_PORT)
              br->num_cnp_ports++;
           else
             if ( port->port_type != port_type && br->num_cnp_ports)
                 br->num_cnp_ports--;
        }
#if (defined HAVE_I_BEB && defined HAVE_B_BEB)
       if (( br->type == BR_TYPE_BACKBONE_MSTP ||
            br->type == BR_TYPE_BACKBONE_RSTP )
           && br->beb)
         {
           if (port_type == PRO_NET_PORT)
              br->num_cnp_ports++;
           else
             if ( port->port_type != port_type && br->num_cnp_ports)
                 br->num_cnp_ports--;
         }
#endif /* (defined HAVE_I_BEB && defined HAVE_B_BEB) */

      port->port_type = port_type;
      port->port_sub_type = port_sub_type;
    } 
  else
    {
      /* The mode of port is already set. Only acceptable frame type or
       * enable ingress filter needs to be updated.
       */
      port->acceptable_frame_types = acceptable_frame_types ?
        VLAN_ADMIT_TAGGED_ONLY : VLAN_ADMIT_ALL_FRAMES;
      port->enable_ingress_filter  = enable_ingress_filter;

      BR_WRITE_UNLOCK_BH (&br->lock);
    }

  BDEBUG ("set the port_type successfully: port_no(%d), port_type(%u)"
          "acceptable_frame_types(%u), enable_ingress_filter(%u)\n", 
          port_no, port_type, acceptable_frame_types, enable_ingress_filter);
  return 0;
}


/* This function sets the default pvid for the given port */
int
br_vlan_set_default_pvid (char *bridge_name, unsigned int port_no, 
                          vid_t pvid, unsigned short egress_tagged)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  if (port->default_pvid != pvid)
    {
      if (br_vlan_delete_from_static_vlan_reg_table (br, port->default_pvid, 
                                                     port_no) < 0)
        {
          BDEBUG ("delete from static_vlan_reg_table failed: port_no(%d) "
                  "vid(%u), egress_tagged(%u)\n", port_no, pvid, egress_tagged);
          return -EINVAL;
        }

      if (br_vlan_insert_into_static_vlan_reg_table (br, pvid,
                                                     port_no, egress_tagged) < 0)
        {
          BDEBUG ("insert into static_vlan_reg_table failed: port_no(%d) "
                  "vid(%u), egress_tagged(%u)\n", port_no, pvid, egress_tagged);
          return -EINVAL;
        }

      port->default_pvid = pvid;
    }

  BDEBUG ("set the default pvid successfully: port_no(%d) vid(%u), "
          "egress_tagged(%u)\n", port_no, pvid, egress_tagged);
  return 0;
}

/* This function sets the native_vid for the given port */
int
br_vlan_set_native_vid (char *bridge_name, unsigned int port_no,
                        vid_t native_vid )
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  unsigned char *config_ports = 0;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  config_ports = br->static_vlan_reg_table[native_vid];

  if (!config_ports)
    {
      BDEBUG ("Vlan not found in the bridge : port_no(%d) "
              "vid(%u) \n", port_no, native_vid);
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  if (!IS_VLAN_PORT_CONFIGURED (config_ports[port->port_id]))
    {
      BDEBUG ("Port not a part of the vlan : port_no(%d) "
              "vid(%u) \n", port_no, native_vid);
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  port->native_vid = native_vid;
  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;
}

/* This function sets the MTU for the given vlan */
int
br_vlan_set_mtu (char *bridge_name, vid_t vid,
                 unsigned int mtu_val)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  struct vlan_info_entry *entry = 0;
  unsigned char *config_ports = 0;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_WRITE_LOCK_BH (&br->lock);

  config_ports = br->static_vlan_reg_table[vid];
  entry = br->vlan_info_table[vid];

  if ( (!entry) )
    {
      BDEBUG ("Vlan not found in the bridge : vid(%u) \n", vid);
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }
  if ( (mtu_val == 0) || (!config_ports) )
    {
      entry->mtu_val = mtu_val;
      BR_WRITE_UNLOCK_BH (&br->lock);
      return 0;
    }
  port = br->port_list;
  while ( port != NULL )
    {
      if ( IS_VLAN_PORT_CONFIGURED (config_ports[port->port_id]) 
           && port->dev->mtu < mtu_val )
        {
          BDEBUG ("Port MTU is smaller than Vlan MTU : port_no(%d) "
                  "vid(%u) Port MTU(%u) Vlan MTU(%u) \n", port->port_id, vid,
                  port->dev->mtu, mtu_val);
          BR_WRITE_UNLOCK_BH (&br->lock);
          return -EINVAL;
        }
      port = port->next;
    }

  entry->mtu_val = mtu_val;

  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;
}

/* This function adds a vid to the given port */
int
br_vlan_add_vid_to_port (char *bridge_name, unsigned int port_no, vid_t vid,
                         unsigned short egress_tagged)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  return br_vlan_insert_into_static_vlan_reg_table (br, vid, port_no, 
                                                    egress_tagged);
}


/* This function deletes a vid from a port */
int
br_vlan_delete_vid_from_port (char *bridge_name, unsigned int port_no, 
                              vid_t vid)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();
  
  return br_vlan_delete_from_static_vlan_reg_table (br, vid, port_no);
}


/* This function deletes all vids from the given port except the default vid */
int
br_vlan_delete_all_vids_from_port (char *bridge_name, unsigned int port_no)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  vid_t vid;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  for (vid = VLAN_DEFAULT_VID; vid <= VLAN_MAX_VID; vid++)
    {
      if (port->port_type == PRO_NET_PORT
          || port->port_type == CUST_NET_PORT)
        {
          if (br->svlan_info_table[vid])
            {
              br_vlan_delete_from_static_vlan_reg_table (br, vid, port_no);
            }
        }
      else
        {
          if (br->vlan_info_table[vid])
            {
              br_vlan_delete_from_static_vlan_reg_table (br, vid, port_no);
            }
        }
    }
 
  BDEBUG ("deleted all the vids from the port: port_no(%d)\n", port_no);

  return 0;
}


/* This function adds all vids to the given port */
int
br_vlan_add_all_vids_to_port (char *bridge_name, unsigned int port_no,
                              unsigned short egress_tagged)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  vid_t vid;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  /* TAKE READ LOCK */
  BR_READ_LOCK (&br->lock);

  for (vid = VLAN_DEFAULT_VID; vid <= VLAN_MAX_VID; vid++)
    {
      if (port->port_type == PRO_NET_PORT
          || port->port_type == CUST_NET_PORT)
        {
          if (br->svlan_info_table[vid])
            {
              if (br_vlan_insert_into_static_vlan_reg_table (br, vid, port_no,
                                                     egress_tagged) < 0)
                {
                  BDEBUG ("insert into static_vlan_reg_table failed: port_no(%d) "
                          "vid(%u), egress_tagged(%u)\n", port_no, vid,
                          egress_tagged);
                  read_unlock (&br->lock);
                  br_vlan_delete_all_vids_from_port (bridge_name, port_no);
                  return -EINVAL;
                }
            }
        }
      else
        {
          if (br->vlan_info_table[vid])
            {
              if (br_vlan_insert_into_static_vlan_reg_table (br, vid, port_no,
                                                     egress_tagged) < 0)
                {
                  BDEBUG ("insert into static_vlan_reg_table failed: port_no(%d) "
                          "vid(%u), egress_tagged(%u)\n", port_no, vid,
                          egress_tagged);
                  read_unlock (&br->lock);
                  br_vlan_delete_all_vids_from_port (bridge_name, port_no);
                  return -EINVAL;
                }
            }
        }
    }
 
  BDEBUG ("added all vids to the port successfully: port_no(%d) "
          "egress_tagged(%u)\n", port_no, egress_tagged);
  /* RELEASE READ LOCK */
  BR_READ_UNLOCK (&br->lock);
  return 0;
}


/* This function intializes the vlan realted data structure for the given
   vlan-aware bridge */
int
br_vlan_init (struct apn_bridge *br)
{
  /* Nothing to do here. All the vlans should be added from user space
     including default vlan */

  return 0;
}

/* This function unintializes the vlan realted data structure for the given
   vlan-aware bridge */
int
br_vlan_uninit (struct apn_bridge *br)
{
  vid_t vid;

  for (vid = VLAN_DEFAULT_VID; vid <= VLAN_MAX_VID; vid++)
    {
      if (br->vlan_info_table[vid] != NULL)
        br_vlan_delete (br->name, CUSTOMER_VLAN, vid);
      if (br->svlan_info_table[vid]  != NULL)
        br_vlan_delete (br->name, SERVICE_VLAN, vid);
    }

  return 0;
}


int
br_vlan_add_to_inst (char *bridge_name, vid_t vid,
                     unsigned short instance_id)
{
  struct apn_bridge *br;
  struct vlan_info_entry *vlan_info_entry;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  if( (vid > VLAN_MAX_VID ) || (instance_id > BR_MAX_INSTANCES ) )
    return -EINVAL;

  BR_READ_LOCK (&br->lock);

  if (!br->is_vlan_aware)
    {
      BDEBUG ("Attempt to change vlan information on non vlan aware bridge\n");
      BR_READ_UNLOCK (&br->lock);
      return -EINVAL;
    }

  if (br->type == BR_TYPE_PROVIDER_RSTP
     || br->type == BR_TYPE_PROVIDER_MSTP)
    vlan_info_entry = br->svlan_info_table[vid];
  else
    vlan_info_entry = br->vlan_info_table[vid];

  if (vlan_info_entry)
    {
      BDEBUG ("Added vlan %d to instance %d on bridge %s\n", 
              vid,instance_id,br->name);

      if (br->type == BR_TYPE_PROVIDER_RSTP
          || br->type == BR_TYPE_PROVIDER_MSTP)
        br->svlan_info_table[vid]->instance = instance_id;
      else
        br->vlan_info_table[vid]->instance = instance_id;

    }

  BR_READ_UNLOCK (&br->lock);
  return 0;
}
