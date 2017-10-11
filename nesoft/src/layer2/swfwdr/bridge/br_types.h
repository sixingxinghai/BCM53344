/*
  Linux ethernet bridge
 
  Authors:
  Lennert Buytenhek   <buytenh@gnu.org>
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.

  This file contains the header for the forwarder

    
*/

#ifndef _PACOS_BR_TYPES_H
#define _PACOS_BR_TYPES_H

#include <linux/netdevice.h>
#include <linux/miscdevice.h>
#include "if_ipifwd.h"

#include "br_timer.h"

#include "br_config.h"

typedef __u16 port_id;

struct apn_bridge_port;
struct apn_bridge;


/* Socket address structure for IGMP Snooping packets. */
struct sockaddr_igs
{
  /* Destination Mac address */
  unsigned char dest_mac[6];

  /* Source Mac address */
  unsigned char src_mac[6];

  /* Outgoing/Incoming interface index */
  unsigned int port;

  /* Vlan id */
  unsigned short vlanid;

  /* SVlan id */
  unsigned short svlanid;

  /* Length of the Packet */
  unsigned int length;
};

struct mac_addr
{
  unsigned char addr[6];
  unsigned char pad[2];
};

struct vlan_header
{
  unsigned short vlan_tci;
  unsigned short encapsulated_proto;
};

/* Macros for skbuff in 2.6.27.21 */
#define skb_dst_mac_addr(skb) skb->mac_header[0]
#define skb_src_mac_addr(skb) skb->mac_header[6]
#define skb_ether_type(skb) skb->mac_header[12]
#define skb_ip_protocol(skb) skb->network_header[9]


/* Debug Macros */
#define MAC_ADDR_FMT       "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ADDR_PRINT(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

#define ETH_MAC_LEN               6
/* Vlan related macros */

#define VLAN_HEADER_LEN           4
#define VLAN_ETH_HEADER_LEN       18

#define VLAN_NULL_VID             0
#define VLAN_NULL_FID             VLAN_NULL_VID
#define VLAN_DEFAULT_VID          1
#define VLAN_DEFAULT_FID          VLAN_DEFAULT_VID 
#define VLAN_MAX_VID              4094 
#define VLAN_MAX_FID              VLAN_MAX_VID
#define VLAN_NAME_SIZE            IFNAMSIZ
#define VLAN_VID_MASK             0xfff
#define VLAN_USER_PRIORITY_MASK   0x07
#define VLAN_DROP_ELIGIBILITY_MASK   0x08

#define VLAN_PORT_UNCONFIGURED    0x00
#define VLAN_PORT_CONFIGURED      0x01
#define VLAN_EGRESS_TAGGED        0x02 
#define VLAN_REGISTRATION_FIXED   0x04

#define VLAN_ADMIT_ALL_FRAMES     0
#define VLAN_ADMIT_UNTAGGED_ONLY  1
#define VLAN_ADMIT_TAGGED_ONLY    2

#define VLAN_MAX_MAC_METHOD       1
#define DEFAULT_USER_PRIORITY     7

#define IS_VLAN_PORT_CONFIGURED(portmap) \
        ((portmap) & VLAN_PORT_CONFIGURED)

#define IS_VLAN_EGRESS_TAGGED(portmap) \
        ((portmap) & VLAN_EGRESS_TAGGED)

#define IS_VLAN_REGISTRATION_FIXED(portmap) \
        ((portmap) & VLAN_REGISTARTION_FIXED)

#define VLAN_MAX_VID                 4094              /* Max VLANs.   */


/* VLAN bitmap manipulation macros. */
#define BR_VLAN_BMP_WORD_WIDTH                32
#define BR_VLAN_BMP_WORD_MAX                  \
        ((VLAN_MAX_VID + BR_VLAN_BMP_WORD_WIDTH) / BR_VLAN_BMP_WORD_WIDTH)

struct  gmrp_service_req
{
  unsigned int bitmap[BR_VLAN_BMP_WORD_MAX];
};

#define BR_SERV_REQ_VLAN_INIT(bmp)                                             \
   do {                                                                    \
       memset ((bmp).bitmap, 0, sizeof ((bmp).bitmap));               \
   } while (0)

#define BR_SERV_REQ_VLAN_SET(bmp, vid)                                         \
   do {                                                                    \
        int _word = (vid) / BR_VLAN_BMP_WORD_WIDTH;                       \
        (bmp).bitmap[_word] |= (1U << ((vid) % BR_VLAN_BMP_WORD_WIDTH));  \
   } while (0)

#define BR_SERV_REQ_VLAN_UNSET(bmp, vid)                                       \
   do {                                                                    \
        int _word = (vid) / BR_VLAN_BMP_WORD_WIDTH;                       \
        (bmp).bitmap[_word] &= ~(1U <<((vid) % BR_VLAN_BMP_WORD_WIDTH));  \
   } while (0)

#define BR_SERV_REQ_VLAN_IS_SET(bmp, vid)                                   \
  ((bmp).bitmap[(vid) / BR_VLAN_BMP_WORD_WIDTH] & (1U << ((vid) % BR_VLAN_BMP_WORD_WIDTH)))

#define BR_SERV_REQ_VLAN_CLEAR(bmp)         BR_SERV_REQ_VLAN_INIT(bmp)



#define APNFLAG_LEARN   0x01
#define APNFLAG_FORWARD 0x02

typedef __u16 vid_t;
typedef __u16 fid_t;
typedef __u16  instance_id_t;

#define RESULT_OK   0

typedef enum bridge_type {
  BR_TYPE_STP,
  BR_TYPE_RSTP,
  BR_TYPE_MSTP,
  BR_TYPE_PROVIDER_RSTP,
  BR_TYPE_PROVIDER_MSTP,
  BR_TYPE_BACKBONE_RSTP,
  BR_TYPE_BACKBONE_MSTP,
  BR_TYPE_RPVST
} bridge_type_t;

/* Vlan related data structures */
typedef enum port_type {
  ACCESS_PORT,
  HYBRID_PORT,
  TRUNK_PORT,
  CUST_EDGE_PORT,
  CUST_NET_PORT,
  PRO_NET_PORT,
  CUSTOMER_NETWORK_PROVIDER_PORT,
  PROVIDER_INSTANCE_PORT,
  MAX_PORT_TYPE
} port_type_t;

typedef enum vlan_type {
  CUSTOMER_VLAN,
  SERVICE_VLAN,
  MAX_VLAN_TYPE
} vlan_type_t;

typedef enum vlan_classification_rule {
  VLAN_MAC_BASED,
  VLAN_PORT_BASED,
  VLAN_PORT_PROTOCOL_BASED,
  VLAN_SUBNET_BASED,
  VLAN_TRUNK_BASED
} vlan_classification_rule_t;

typedef enum bool
  {
    BR_FALSE=0,
    BR_TRUE
  } bool_t;

typedef enum br_result
  {
    BR_NOERROR=0,
    BR_ERROR,
    BR_NORESOURCE,
    BR_MATCH,
    BR_NOMATCH
  } br_result_t;

typedef enum br_frame_type
  {
    UNTAGGED,
    PRIORITY_TAGGED,
    TAGGED
  } br_frame_t;

typedef enum vlan_state
  {
    VLAN_SUSPENDED_STATE,
    VLAN_ACTIVE_STATE
  } vlan_state_t;

typedef enum pvlan_type
  {
    PVLAN_NONE,
    PVLAN_COMMUNITY,
    PVLAN_ISOLATED,
    PVLAN_PRIMARY
  } pvlan_type_t;

typedef enum pvlan_port_configure_mode
  {
    PVLAN_PORT_MODE_INVALID,
    PVLAN_PORT_MODE_HOST,
    PVLAN_PORT_MODE_PROMISCUOUS
  } pvlan_port_mode_t;


typedef enum apn_bridge_proto_process
  {
    APNBR_L2_PROTO_PEER,
    APNBR_L2_PROTO_TUNNEL,
    APNBR_L2_PROTO_DISCARD,
    APNBR_L2_PROTO_PROCESS_MAX,
  } apn_bridge_proto_process_t;

typedef enum apn_bridge_l2_proto
  {
    APNBR_PROTO_STP,
    APNBR_PROTO_RSTP,
    APNBR_PROTO_MSTP,
    APNBR_PROTO_GMRP,
    APNBR_PROTO_GVRP,
    APNBR_PROTO_MMRP,
    APNBR_PROTO_MVRP,
    APNBR_PROTO_LACP,
    APNBR_PROTO_DOT1X,
    APNBR_PROTO_MAX
  } apn_bridge_l2_proto_t;

struct classification_rule
{
  vlan_classification_rule_t type;
  void *actual_rule; 
  vid_t vid; 
};

struct vid_entry 
{
  struct vid_entry *prev;
  vid_t vid;
  struct vid_entry *next;
};

struct pvlan_secondary_list
{
  unsigned int bitmap[BR_VLAN_BMP_WORD_MAX];
};

union pvlan_vid
{
  vid_t isolated_vid;
  vid_t primary_vid;
};

struct br_pvlan_info
{
  struct pvlan_secondary_list secondary_vlan_bmp;
  union pvlan_vid vid;
};

struct vlan_info_entry 
{
  vid_t vid;

  instance_id_t instance;

  unsigned int mtu_val;

  struct apn_bridge *br;

  struct net_device *dev;

  struct net_device_stats net_stats;

  /* private VLAN information*/
  bool_t pvlan_configured;
  pvlan_type_t  pvlan_type;

  struct br_pvlan_info pvlan_info;

  /* Port whose hw addr we are using for the VI */
  struct apn_bridge_port *vi_addr_port;
};

struct static_entry
{
  vid_t vid;
  vid_t svid;
  struct apn_bridge_port *port;
  struct static_entry *next;
};

struct static_entries
{
  struct static_entry *forward_list;
  long filter_portmap[PORTMAP_SIZE + 1];
  unsigned char is_forward:1;
  unsigned char is_filter:1;
};

union pvlan_port_info 
{
  struct pvlan_secondary_list secondary_vlan_bmp;
  vid_t primary_vid;
};

struct apn_bridge_protocol_process
{
  apn_bridge_proto_process_t stp_process;
  u_int16_t stp_tun_vid;

  apn_bridge_proto_process_t gmrp_process;

  apn_bridge_proto_process_t mmrp_process;

  apn_bridge_proto_process_t gvrp_process;
  u_int16_t gvrp_tun_vid;

  apn_bridge_proto_process_t mvrp_process;
  u_int16_t mvrp_tun_vid;

  apn_bridge_proto_process_t lacp_process;
  u_int16_t lacp_tun_vid;

  apn_bridge_proto_process_t dot1x_process;
  u_int16_t dot1x_tun_vid;
};

struct apn_bridge_port
{
  struct apn_bridge_port *next;
  struct apn_bridge_port *multicast_next;
  struct apn_bridge *br;
  struct net_device *dev;

  /* Port number associated with the port */
  int port_no;
  
  /* Port ID assigned from 0 - NPF_MAX_PORTS */
  int port_id;
  
  /* Port state */
  char state[BR_MAX_INSTANCES];

  /* Flags to indicate learning or forwarding state 
   * We do not use the state as STP or RSTP or XYZ can have diff states. */
  unsigned char state_flags[BR_MAX_INSTANCES];

  /* VLAN related information. */
  /* Flag to admit vlan tagged frames only through this port. */
  unsigned char acceptable_frame_types:1;

  /* Flag to enable ingress filtering on the incoming frame through this port.*/
  unsigned char enable_ingress_filter:1;

  /* Enable swap of vids if the ingress vid is different from egress vid that
     but belongs to the same fid */
  unsigned char enable_vid_swap:1;

  /* Port type is access | trunk | hybrid | provider network | customer network | customer edge */
  port_type_t port_type;

  /* Port sub type is access | trunk | hybrid */
  port_type_t port_sub_type;

  /* Default vid associated with the port that gets used for untagged frames */
  vid_t default_pvid;

  /* Default vid associated with the port that gets used for untagged frames 
     for trunk ports */
  vid_t native_vid;

  /* private VLAN information */

  pvlan_port_mode_t pvlan_port_mode;
  /* vid_t primary_vid;
   *  vid_t secondary_vid;
   */
  union pvlan_port_info pvlan_port_info;

  struct apn_bridge_protocol_process proto_process;

  /* GMRP service requirement information */
  /*
    Service requirement per vlan 
  */
  struct gmrp_service_req gmrp_serv_req_fwd_all;
  struct gmrp_service_req gmrp_serv_req_fwd_unreg;

  struct br_avl_tree *trans_tab;
  struct br_avl_tree *rev_trans_tab;
  struct br_avl_tree *reg_tab;
  struct br_avl_tree *pro_edge_tab;

};

struct apn_multicast_port_list
{
  struct apn_bridge_port *port;
  struct apn_multicast_port_list *next;
};

struct apn_bridge_fdb_entry
{
  struct apn_bridge_fdb_entry *next_hash;
  struct apn_bridge_fdb_entry **pprev_hash;

  atomic_t use_count;
  struct mac_addr addr;
  struct apn_bridge_port *port;          /* For unicast. */
  struct apn_multicast_port_list *mport; /* For multicast. */
  unsigned long ageing_timestamp;

  /* Port Map flags associated with the filtering/forwarding entry */
  unsigned is_local:1;
  unsigned is_static:1;
  unsigned is_fwd:1;  

  /* VLAN related information */
  /* Valid vid (1 through 4094) value associated with the fdb entry */
  vid_t vid;

  /* Valid svid (1 through 4094) value associated with the fdb entry */
  vid_t svid;
};

struct apn_bridge
{
  struct apn_bridge *next;
  struct apn_bridge **pprev;

  /* Bridge name */
  char name[IFNAMSIZ + 1];

  /* Bridge lock */
  rwlock_t lock;

  /* Ports that are part of this bridge */
  struct apn_bridge_port *port_list;

  /* Index for port ifindex */
  int port_index;
  char port_id[BR_MAX_PORTS];

  /* Stores whether the bridge is MSTP, RSTP or an STP bridge */
  enum bridge_type type;

  unsigned char edge;

  /* on indicates bridge is up and forwarding */
  unsigned char flags; 

  /* Frame statistics */
  struct net_device_stats statistics;

  /* Dynamic forwarding database (fdb) lock */
  rwlock_t hash_lock;

  /* Static forwarding database (fdb) lock */
  rwlock_t static_hash_lock;

  /* Table to store dynamic forwarding/filtering entries (dynamic fdb) */
  struct apn_bridge_fdb_entry *hash[BR_HASH_SIZE];

  /* Table to store static forwarding/filtering entries (static fdb) */
  struct apn_bridge_fdb_entry *static_hash[BR_HASH_SIZE];

  struct timer_list tick;
  struct br_timer dynamic_aging_timer;

  /* Per instance values */
  /* How often to look for expired entries. Default 4s */
  int dynamic_ageing_interval; 
  
  /* How long dynamic entries are kept. Default 300s */
  int ageing_time;

  /* count of allocated entries - for limit checks */
  short num_ports;
  short num_dynamic_entries;
  short num_static_entries;
  
  unsigned char is_vlan_aware:1;
  unsigned char igmp_snooping_enabled:1;

  struct vlan_info_entry* vlan_info_table[VLAN_MAX_VID + 1];

  struct vlan_info_entry* svlan_info_table[VLAN_MAX_VID + 1];

  /* Table that maps vid->port[BR_MAX_PORTS] (Static VLAN Registration)
     ---------------------------------
     | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
     ---------------------------------
     0 ->   Registration Fixed | Registration Forbidden
     1 ->   Egress tagged | Egress untagged
     7 ->   Port is configured for the given vid
     2-6 -> Unused
  */
  char *static_vlan_reg_table[VLAN_MAX_VID + 1];

  char *static_svlan_reg_table[VLAN_MAX_VID + 1];

  /* NOTE: Going forward support is needed for dynamic vlan registration
     entries and could use the same idea as the static except the
     bits per port would mean different from the static entries. This 
     will be done once GVRP Protocol is implemented
     char *dynamic_vlan_reg_table[VLAN_MAX_VID + 1]; 
  */
  /* Garp flag -to indicate gmrp/gvrp configuration*/
  unsigned char garp_config;
#define APNBR_GARP_GMRP_CONFIGURED 0x01
#define APNBR_GARP_GVRP_CONFIGURED 0x02
#define APNBR_MRP_MMRP_CONFIGURED  0x04
#define APNBR_MRP_MVRP_CONFIGURED  0x08

  unsigned char ext_filter:1;

#define APNBR_MAC_AUTH_CONFIGURED  (1 << 0)

   unsigned short num_ce_ports;

   unsigned short num_cn_ports;

   unsigned short num_cnp_ports;

   unsigned short num_pip_ports;

   unsigned char beb;

};

struct br_vlan_trans_key
{
  vid_t vid;
  vid_t trans_vid;

};

struct br_vlan_regis_key
{
  vid_t cvid;
  vid_t svid;

  /* Used for CE port to set the state for the provider edge ports */
  unsigned char state_flags;
};

struct br_vlan_pro_edge_port
{
  vid_t svid;
  vid_t untagged_vid;
  vid_t pvid;
};

#define ETH_P_MMRP 0x8110
#define ETH_P_MVRP 0x8120
#define ETH_P_8021Q_STAG 0x88a8
#define ETH_P_8021Q_CTAG 0x8100
#define ETH_P_LLDP       0x88cc
#define ETH_P_EFM        0x8809
#define ETH_P_CFM        0x8902
#define ETH_P_ELMI       0x88EE
#endif
