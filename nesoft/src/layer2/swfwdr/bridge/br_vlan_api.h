/* Copyright (C) 2003  All Rights Reserved.
 *  
 *  802.1Q Vlan-Aware Bridge 
 *     
 *  Author:  srini
 *        
 */

#ifndef __PACOS_BR_VLAN_API_H__
#define __PACOS_BR_VLAN_API_H__

#include "if_ipifwd.h"
#include "br_types.h"
#include "br_api.h"

#define BR_VLAN_BITS_PER_VLANMAP_ENTRY    8
#define BR_VLAN_VLANMAP_SIZE \
           (VLAN_MAX_VID + 2)/BR_VLAN_BITS_PER_VLANMAP_ENTRY

#define BR_VLAN_SET_VLAN_BITMAP(vid, vlan_bitmap) \
          ((vlan_bitmap)[(vid)/(BR_VLAN_BITS_PER_VLANMAP_ENTRY)]) \
          |= (1 << ((vid) % (BR_VLAN_BITS_PER_VLANMAP_ENTRY)))

#define BR_VLAN_GET_VLAN_BITMAP(vid, vlan_bitmap) \
          ((vlan_bitmap)[(vid)/(BR_VLAN_BITS_PER_VLANMAP_ENTRY)]) \
          &= (1 << ((vid) % (BR_VLAN_BITS_PER_VLANMAP_ENTRY)))

#define BR_VLAN_IS_BITMAP_SET(vid, vlan_bitmap)                    \
          (((vlan_bitmap)[(vid)/(BR_VLAN_BITS_PER_VLANMAP_ENTRY)]) \
          & ((1U << ((vid) % BR_VLAN_BITS_PER_VLANMAP_ENTRY))))

#define BR_VLAN_CLEAR_VLAN_BITMAP(vlan_bitmap) \
{ \
  unsigned short index; \
  for (index = 0; index < APN_VLANMAP_SIZE; index++) \
    { \
      vlan_bitmap[index] = 0; \
    } \
}

#define BR_VLAN_PRINT_VLAN_BITMAP(vlan_bitmap)                            \
{                                                                         \
  unsigned short index;                                                   \
  for (index = 0; index < APN_VLANMAP_SIZE; index++)                      \
    {                                                                     \
      printk ("index %u : vlan_bitmap %u \n", index, vlan_bitmap[index]); \
    }                                                                     \
}

#define BR_VLAN_GET_VLAN_AWARE_BRIDGE()\
  do \
    {\
      if (!(br = br_find_bridge (bridge_name)))\
        {\
          BDEBUG ("bridge does not exists: bridge_name(%s)\n", bridge_name);\
           return -EINVAL;\
        }\
      if (!br->is_vlan_aware)\
        {\
          BDEBUG ("bridge is not vlan_aware bridge_name(%s)\n", bridge_name);\
          return -EINVAL;\
        }\
    }\
  while (0)

#define BR_VLAN_GET_PORT()\
  do \
    {\
      port = br_get_port (br, port_no);\
      if (!port)\
        {\
          BDEBUG ("port does not exist: port_no(%d)\n", port_no);\
          return -EINVAL;\
        }\
    }\
  while (0)

/* Function to intialize vlan data structures for a vlan-aware bridge */
extern int 
br_vlan_init (struct apn_bridge *br);
                                                                                
/* Function to unintialize vlan data structures for a vlan-aware bridge */
extern int
br_vlan_uninit (struct apn_bridge *br);
                                                                                
/* Function that adds a vlan to a vlan-aware bridge */
extern int
br_vlan_add (char *bridge_name, enum vlan_type type, vid_t vid);

/* Function that delets a vlan from a vlan-aware bridge */
extern int
br_vlan_delete (char *bridge_name, enum vlan_type type, vid_t vid);

/* Function to set the port type (access, hybrid, trunk) and port
   characteristics for a given port */
extern int
br_vlan_set_port_type (char *bridge_name, unsigned int port_no,
                       port_type_t port_type,
                       port_type_t port_sub_type,
                       unsigned short acceptable_frame_types,
                       unsigned short enable_ingress_filter);


/* Function to set the default pvid and port characteristics for a given port */
extern int
br_vlan_set_default_pvid (char *bridge_name, unsigned int port_no, 
                          vid_t pvid, unsigned short egress_tagged);

/* Function to set the native vid for a given port */
extern int
br_vlan_set_native_vid (char *bridge_name, unsigned int port_no,
                        vid_t native_vid);

/* Function to set the VLAN MTU */
extern int
br_vlan_set_mtu (char *bridge_name, vid_t vid, 
                 unsigned int mtu_val);

/* Function to get vids configured on a port */
void
br_vlan_get_config_vids (struct apn_bridge *br, unsigned int port_no,
                         char *config_vids);

/* Function to add vlan for a given port */
extern int
br_vlan_add_vid_to_port (char *bridge_name, unsigned int port_no,
                         vid_t pvid, unsigned short egress_tagged);


/* Function to add vlan for a given port */
extern int
br_vlan_delete_vid_from_port (char *bridge_name, unsigned int port_no, 
                              vid_t vid);

 
/* This function changes the vlan device flags */
extern void
br_vlan_update_vlan_dev (struct apn_bridge *br, struct apn_bridge_port *port,
                         int instance);

/* Function to add all configured vlans for a given port */
extern int
br_vlan_add_all_vids_to_port (char *bridge_name, unsigned int port_no,
                              unsigned short egress_tagged);

/* Function to delete all configured vlans from a given port */
extern int
br_vlan_delete_all_vids_from_port (char *bridge_name, unsigned int port_no);

extern int
br_vlan_add_vlan_to_inst (char *bridge_name, vid_t vlanid,
                          unsigned char instance_id);

/* This function retruns a pointer to the port with the specified port_id. */
struct apn_bridge_port *br_vlan_get_port (struct apn_bridge *, int port_no);

/* This function retruns a pointer to the port with the specified port_id. */
struct apn_bridge_port *br_vlan_get_port_by_id (struct apn_bridge *, 
                                                int port_id);

/* This function retruns a pointer to the port with the specified port_no. */
struct apn_bridge_port *br_vlan_get_port_by_no (struct apn_bridge *, 
                                                int port_no);

/* This function retruns a pointer to the port with the specified hw_addr. */
struct apn_bridge_port *br_vlan_get_port_by_hw_addr (struct apn_bridge *,
                                                     unsigned char *);
#endif /* ! __PACOS_BR_VLAN_API_H__ */
