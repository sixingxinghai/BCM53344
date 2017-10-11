/* Copyright (C) 2004  All Rights Reserved.
 *
 * 802.1Q Vlan-Aware Bridge
 *
 * Private vlan specific apis 
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
#include "br_types.h"

int 
br_pvlan_set_type (char *bridge_name, vid_t vid, enum pvlan_type type)
{
  struct apn_bridge *br = 0;
  struct vlan_info_entry *vlan_info_entry;
  int ret;

  BDEBUG("Entering br_vlan_add\n");
  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_WRITE_LOCK (&br->lock);
  
  /* unregister the vlan interface with the kernel */
  vlan_info_entry = br->vlan_info_table[vid];
  if (!vlan_info_entry)
    {
      BDEBUG ("vlan_info_entry doesnt exist: vid(%u)\n", vid);
      /*  RELEASE WRITE LOCK */
      BR_WRITE_UNLOCK (&br->lock);
      return -EINVAL;
    }

  /* set the vlan port type and exit */

  if (type != PVLAN_NONE)
    {
      vlan_info_entry->pvlan_type = type;
      vlan_info_entry->pvlan_configured = BR_TRUE;
    }
  else
    {
      vlan_info_entry->pvlan_type = PVLAN_NONE;
      vlan_info_entry->pvlan_configured = BR_FALSE;
    }

  BR_WRITE_UNLOCK (&br->lock);  
  return 0;

}

int 
br_pvlan_set_associate (char *bridge_name, vid_t vid, vid_t pvid, 
                        bool_t associate)
{
  struct apn_bridge *br = 0;
  struct vlan_info_entry *vlan_info_entry, *secondary_vlan_info_entry;
  int ret;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_READ_LOCK (&br->lock);
  /* unregister the vlan interface with the kernel */
  vlan_info_entry = br->vlan_info_table[vid];
  secondary_vlan_info_entry = br->vlan_info_table[pvid];

  BR_READ_UNLOCK (&br->lock);

  if (!vlan_info_entry || !secondary_vlan_info_entry)
    {
      BDEBUG ("vlan_info_entry doesnt exist: vid(%u)\n", vid);
      return -EINVAL;
    }

  /* Do some error checking like in nsm */ 
   
  BR_WRITE_LOCK (&br->lock);
  if (associate)
    {
      if ((vlan_info_entry->pvlan_info.vid.isolated_vid !=0)
          && (secondary_vlan_info_entry->pvlan_type == PVLAN_ISOLATED))
        {
          BDEBUG (" Isolated vlan exists \n");
          /*  RELEASE WRITE LOCK */
          BR_WRITE_UNLOCK (&br->lock);
          return -EINVAL;
        }
      BR_SERV_REQ_VLAN_SET (vlan_info_entry->pvlan_info.secondary_vlan_bmp,
                            pvid);
      secondary_vlan_info_entry->pvlan_info.vid.primary_vid = vid;
      if (secondary_vlan_info_entry->pvlan_type == PVLAN_ISOLATED)
        vlan_info_entry->pvlan_info.vid.isolated_vid = pvid;
      BDEBUG ("primary_vlan %d secondary_vlan set %d \n", vid,
              BR_SERV_REQ_VLAN_IS_SET 
              (vlan_info_entry->pvlan_info.secondary_vlan_bmp, pvid));
    }
  else
    {
      BR_SERV_REQ_VLAN_UNSET (vlan_info_entry->pvlan_info.secondary_vlan_bmp,
                              pvid);
      secondary_vlan_info_entry->pvlan_info.vid.primary_vid = 0;
      if (secondary_vlan_info_entry->pvlan_type == PVLAN_ISOLATED)
        vlan_info_entry->pvlan_info.vid.isolated_vid = 0;
    }

  BR_WRITE_UNLOCK (&br->lock);
  return 0;
}

int
br_pvlan_set_port_mode (char *bridge_name, int port_no,
                        enum pvlan_port_configure_mode mode)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (port->pvlan_port_mode != mode)
    {
      port->pvlan_port_mode = mode;
    }
   
  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;
}

int
br_pvlan_set_port_host_associate (char *bridge_name, int port_no,
                                  vid_t vid, vid_t pvid, bool_t associate)
{
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  struct vlan_info_entry *primary_vlan;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_READ_LOCK (&br->lock);

  /* do some error checking for secondary vid part of primary*/
  primary_vlan = br->vlan_info_table[vid];

  BR_READ_UNLOCK (&br->lock);
  if (!primary_vlan)
    {
      BDEBUG ("vlan_info_entry doesnt exist: vid(%u)\n", vid);
      return -EINVAL;
    }
    
  BR_WRITE_LOCK_BH (&br->lock);

  if (!BR_SERV_REQ_VLAN_IS_SET
      (primary_vlan->pvlan_info.secondary_vlan_bmp, pvid))
    {
      BDEBUG ("secondary vlan not associated to primary vlan \n");
      BR_WRITE_UNLOCK_BH (&br->lock);
      return  -EINVAL;
    }

  if (port->pvlan_port_mode == PVLAN_PORT_MODE_HOST)
    {
      /* Do some error checking whether pvlan is configured etc */
      if (associate)
        {
          port->pvlan_port_info.primary_vid = vid;
        }
      else
        {
          port->pvlan_port_info.primary_vid = 0;
        }
    }

  if (port->pvlan_port_mode == PVLAN_PORT_MODE_PROMISCUOUS)
    {
      if (associate)
        {
          BR_SERV_REQ_VLAN_SET 
            (port->pvlan_port_info.secondary_vlan_bmp, pvid);
          
        }
      else
        {
          BR_SERV_REQ_VLAN_UNSET 
            (port->pvlan_port_info.secondary_vlan_bmp, pvid);
        }

 
    }

  BR_WRITE_UNLOCK_BH (&br->lock);  
  return 0;
}      

