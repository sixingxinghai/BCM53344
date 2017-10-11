/*
  Linux VLAN Aware forwarder
  
  Authors:
  Lennert Buytenhek   <buytenh@gnu.org>
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
*/

#ifndef _PACOS_IF_APNFWD_H
#define _PACOS_IF_APNFWD_H

#ifdef __KERNEL__

#include <linux/netdevice.h>

#endif

#define PACOS_LAYER2_VERSION  101


/* Configuration variables not in br_config.h */
#define BR_MAX_PORTS        32
#define BR_MAX_DYNAMIC_ENTRIES  2048
/* DYNAMIC entries must be greater than or equal to static entries. 
   See imi_bridge.c. */
#define BR_MAX_STATIC_ENTRIES  256
#define BR_MAX_BRIDGES      32

/* Do not change these */

/* Bridge control message identifiers */
#define APNBR_GET_VERSION                           1
#define APNBR_GET_BRIDGES                           2
#define APNBR_ADD_BRIDGE                            3
#define APNBR_DEL_BRIDGE                            4
#define APNBR_ADD_IF                                5
#define APNBR_DEL_IF                                6
#define APNBR_GET_BRIDGE_INFO                       7
#define APNBR_GET_PORT_LIST                         8
#define APNBR_SET_AGEING_TIME                       9
#define APNBR_SET_DYNAMIC_AGEING_INTERVAL           10
#define APNBR_GET_PORT_INFO                         11
#define APNBR_SET_BRIDGE_LEARNING                   12
#define APNBR_GET_DYNFDB_ENTRIES                    13
#define APNBR_GET_STATFDB_ENTRIES                   14
#define APNBR_ADD_STATFDB_ENTRY                     15
#define APNBR_DEL_STATFDB_ENTRY                     16
#define APNBR_GET_DEVADDR                           17
#define APNBR_GET_PORT_STATE                        18
#define APNBR_SET_PORT_STATE                        19
#define APNBR_SET_PORT_FWDER_FLAGS                  20
#define APN_VLAN_ADD                                21
#define APN_VLAN_DEL                                22
#define APN_VLAN_SET_PORT_TYPE                      23
#define APN_VLAN_SET_DEFAULT_PVID                   24
#define APN_VLAN_ADD_VID_TO_PORT                    25
#define APN_VLAN_DEL_VID_FROM_PORT                  26
#define APNBR_FLUSH_FDB_BY_PORT                     27
#define APNBR_ADD_DYNAMIC_FDB_ENTRY                 28
#define APNBR_DEL_DYNAMIC_FDB_ENTRY                 29
#define APNBR_ADD_VLAN_TO_INST                      30
#define APNBR_ENABLE_IGMP_SNOOPING                  31
#define APNBR_DISABLE_IGMP_SNOOPING                 32
#define APN_VLAN_SET_NATIVE_VID                     33
#define APN_VLAN_SET_MTU                            34
#define APNBR_GET_UNICAST_ENTRIES                   35
#define APNBR_GET_MULTICAST_ENTRIES                 36
#define APNBR_CLEAR_FDB_BY_MAC                      37
#define APNBR_GARP_SET_BRIDGE_TYPE                  38
#define APNBR_ADD_GMRP_SERVICE_REQ                  39
#define APNBR_SET_EXT_FILTER                        40
#define APNBR_SET_PVLAN_TYPE                        41
#define APNBR_SET_PVLAN_ASSOCIATE                   42
#define APNBR_SET_PVLAN_PORT_MODE                   43
#define APNBR_SET_PVLAN_HOST_ASSOCIATION            44
#define APNBR_ADD_CVLAN_REG_ENTRY                   45
#define APNBR_DEL_CVLAN_REG_ENTRY                   46
#define APNBR_ADD_VLAN_TRANS_ENTRY                  47
#define APNBR_DEL_VLAN_TRANS_ENTRY                  48
#define APNBR_SET_PROTO_PROCESS                     49
#define APN_VLAN_ADD_PRO_EDGE_PORT                  50
#define APN_VLAN_DEL_PRO_EDGE_PORT                  51
#define APN_VLAN_SET_PRO_EDGE_DEFAULT_PVID          52
#define APN_VLAN_SET_PRO_EDGE_UNTAGGED_VID          53
#define APNBR_CHANGE_VLAN_TYPE                      54
#define APNBR_GET_IFINDEX_BY_MAC_VID                55
#define APNBR_MAX_CMD                               56

#define APN_BR_GMRP_FWD_ALL                         0x00
#define APN_BR_GMRP_FWD_UNREGISTERED                0x01


#define APN_VLAN_DEFAULT_VID              1
#define APN_VLAN_MAX_VID                  4094
#define APN_VLAN_NAME_SIZE                16
#define APN_VLAN_SUSPEND_STATE            0
#define APN_VLAN_ACTIVE_STATE             1
#define APN_VLAN_ACCESS_PORT              0
#define APN_VLAN_HYBRID_PORT              1
#define APN_VLAN_TRUNK_PORT               2
#define APN_VLAN_STACKED_TRUNK_PORT       3
#define APN_VLAN_DEFAULT_VID              1
#define APN_VLAN_EGRESS_UNTAGGED          0
#define APN_VLAN_EGRESS_TAGGED            1
#define APN_VLAN_ACCEPT_ALL_FRAMES_TYPES  0
#define APN_VLAN_TAGGED_FRAMES_ONLY       1
#define APN_VLAN_DISABLE_INGRESS_FILTER   0
#define APN_VLAN_ENABLE_INGRESS_FILTER    1

/* Software forwarder currently treats SHARED_VLAN and VLAN as shared. */
#define APN_SHARED_VLAN_BRIDGE            2
#define APN_VLAN_BRIDGE                   1
#define APN_NO_VLAN_BRIDGE                0

#define APN_BITS_PER_VLANMAP_ENTRY        8

#define APN_VLANMAP_SIZE\
        ((APN_VLAN_MAX_VID + 2)/APN_BITS_PER_VLANMAP_ENTRY)

#define APN_VLAN_GET_VLAN_BITMAP(vid, vlan_bitmap) \
        (((vlan_bitmap)[(vid)/(APN_BITS_PER_VLANMAP_ENTRY)]) \
         & (1 << ((vid) % (APN_BITS_PER_VLANMAP_ENTRY))))


/* FLag values for the bridge */

#define APNBR_UP                  0x01
#define APNBR_LEARNING_ENABLED    0x02
#define APNBR_IS_VLAN_AWARE       0x04
#define APNBR_IGMP_SNOOP_ENABLED  0x08

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

/* Ethernet frame type */

#ifndef ETH_P_IP
#define ETH_P_IP                  0x0008    /* IP type */
#endif


/* Control data structures - passed between CLI/brctl and kernel */

struct bridge_info
{
  unsigned char       up;
  unsigned char       learning_enabled;
  unsigned int        ageing_time;
  unsigned int        dynamic_ageing_period;
  unsigned int        dynamic_ageing_timer_value;
};

struct port_info
{
  unsigned short      port_id;
  unsigned char       state;
};

struct fdb_entry
{
  unsigned int        ageing_timer_value;
  unsigned char       mac_addr[6];
  unsigned char       port_no;  
  unsigned char       is_local;
  unsigned char       is_fwd;
  unsigned char       snmp_status;
  unsigned char       is_static;
  unsigned short      vid;
  unsigned short      svid;
};

struct vlan_summary
{
  unsigned short  vid;
  unsigned short  stacked_vid;
  unsigned char   config_ports[BR_MAX_PORTS];
};

struct vlan_port_summary
{
  int port_no;
  unsigned char port_type;
  unsigned char ingress_filter;
  unsigned char acceptable_frame_types;
  unsigned char enable_vid_swap;
  unsigned short default_pvid;
  unsigned short  stacked_vid;
  unsigned char config_vids[APN_VLANMAP_SIZE];
  unsigned char egress_tagged[APN_VLANMAP_SIZE];
};

struct port_stats
{
  unsigned int inframes;
  unsigned int outframes;
  unsigned int discards;
  unsigned int in_overflow_frames;
  unsigned int out_overflow_frames;
  unsigned int overflow_discards;
};

struct port_HC_stats
{
  unsigned int inframes;
  unsigned int outframes;
  unsigned int discards;
};

#endif
