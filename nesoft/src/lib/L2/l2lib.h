/* Copyright (C) 2003  All Rights Reserved. */
#ifndef __L2LIB_H__
#define __L2LIB_H__

/* 10 Mb/s link per table 8-5 IEEE 802.1t-2001 */
#define L2_DEFAULT_PATH_COST 20000000

#define VLAN_HEADER_SIZE             4
/* 4 Mb/s link as per table 8-5 IEEE 802.1d-1998 */
#define L2_DEFAULT_PATH_COST_SHORT_METHOD 250
 
#define VLAN_PORT_UNCONFIGURED       0x00
#define VLAN_PORT_CONFIGURED         (1 << 0)
#define VLAN_EGRESS_TAGGED           (1 << 1)
#define VLAN_REGISTRATION_FIXED      (1 << 2)
#define VLAN_REGISTRATION_NORMAL     (1 << 3)
#define VLAN_REGISTRATION_FORBIDDEN  (1 << 4)
#define VLAN_TYPE_STATIC             (1 << 5)
#define VLAN_TYPE_DYNAMIC            (1 << 6)

#define VLAN_ADMIT_ALL_FRAMES     0
#define VLAN_ADMIT_UNTAGGED_ONLY  1
#define VLAN_ADMIT_TAGGED_ONLY    2

/* Bridge states */
#define BR_STATE_DISABLED   0
#define BR_STATE_LISTENING  1
#define BR_STATE_LEARNING   2
#define BR_STATE_FORWARDING 3
#define BR_STATE_BLOCKING   4
#define BR_STATE_MAX        5

                                                                        
/* Bridge default values */
                                                                        
#define BRIDGE_TIMER_DEFAULT_FWD_DELAY                    15
#define BRIDGE_TIMER_DEFAULT_HELLO_TIME                   2
#define BRIDGE_TIMER_DEFAULT_MAX_AGE                      20
#define BRIDGE_TIMER_DEFAULT_AGEING_TIME                  300
#define BRIDGE_TIMER_DEFAULT_AGEING_INTERVAL              4
                                                                        
#define BRIDGE_DEFAULT_PRIORITY           32768
#define BRIDGE_DEFAULT_PORT_PRIORITY      128

#define BR_MAX_VLANS      4094
#define BR_VLAN_MAX_VID                  4094
#define BR_VLAN_BITS_PER_VLANMAP_ENTRY    8
#define IS_VLAN_PORT_CONFIGURED(portmap) \
        ((portmap) & VLAN_PORT_CONFIGURED)

#define BR_VLANMAP_SIZE\
        ((BR_VLAN_MAX_VID + 2)/BR_VLAN_BITS_PER_VLANMAP_ENTRY)

#define BR_VLAN_GET_VLAN_BITMAP(vid, vlan_bitmap) \
        (((vlan_bitmap)[(vid)/(BR_VLAN_BITS_PER_VLANMAP_ENTRY)]) \
         & (1 << ((vid) % (BR_VLAN_BITS_PER_VLANMAP_ENTRY))))

#define BR_VLAN_SET_VLAN_BITMAP(vid, vlan_bitmap) \
            ((vlan_bitmap)[(vid)/(BR_VLAN_BITS_PER_VLANMAP_ENTRY)]) \
                      |= (1 << ((vid) % (BR_VLAN_BITS_PER_VLANMAP_ENTRY)))

#define MACADDR_LEN 6
#define L2_VLAN_DEFAULT_VID        (1)
/* Max Number of fdb entries */
#define L2_MAX_FDB_ENTRIES       (256)

#define L2_BRIDGE_MAX_PORTS       (64)
#define L2_BRIDGE_NAME_LEN        (16)
#define L2_IF_NAME_LEN            (20)



struct bridge_id
{
  unsigned char prio[2];
  unsigned char addr[ETHER_ADDR_LEN];
};

struct mac_addr
{
  unsigned char addr[ETHER_ADDR_LEN];
};

struct vlan_header
{
  u_int16_t vlan_tci;
  u_int16_t encapsulated_proto;
};

#endif /* define __L2LIB_H__ */
