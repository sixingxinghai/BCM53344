/* Copyright (C) 2003  All Rights Reserved. */

#include "nsm_message.h"
#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

#ifdef HAVE_VLAN
#include "nsm_vlan_cli.h"
#endif /* HAVE_VLAN */
#ifdef HAVE_L3
#include "rib.h"
#endif /* HAVE_L3 */

#ifndef _NSM_API_H
#define _NSM_API_H

/* system service hash defines */
#define NSM_SYS_SERVICE_L2 1
#define NSM_SYS_SERVICE_L3 2

/* NSM API General error code. */
#define NSM_API_GET_SUCCESS                               1
#define NSM_API_GET_ERROR                                 0
#define NSM_API_GET_FAIL                                 -1
#define NSM_API_SET_SUCCESS                               0
#define NSM_API_SET_ERROR                                -1

/* NSM CLI-API. */
#define NSM_API_SET_ERR_INVALID_VALUE                   -10
#define NSM_API_SET_ERR_INVALID_IF                      -11
#define NSM_API_SET_ERR_INVALID_IF_TYPE                 -12
#define NSM_API_SET_ERR_INVALID_IPV4_ADDRESS            -13
#define NSM_API_SET_ERR_INVALID_IPV4_ADDRESS_VRRP       -14
#define NSM_API_SET_ERR_INVALID_IPV4_NEXTHOP            -15
#define NSM_API_SET_ERR_INVALID_IPV6_ADDRESS            -16
#define NSM_API_SET_ERR_INVALID_IPV6_NEXTHOP            -17
#define NSM_API_SET_ERR_INVALID_IPV6_NEXTHOP_LINKLOCAL  -18
#define NSM_API_SET_ERR_INVALID_PREFIX_LENGTH           -19
#define NSM_API_SET_ERR_INVALID_MTU_SIZE                -20
#define NSM_API_SET_ERR_IF_NOT_EXIST                    -21
#define NSM_API_SET_ERR_IF_NOT_ACTIVE                   -22
#define NSM_API_SET_ERR_ADDRESS_NOT_EXIST               -23
#define NSM_API_SET_ERR_PREFIX_NOT_EXIST                -24
#define NSM_API_SET_ERR_CANT_SET_ADDRESS                -25
#define NSM_API_SET_ERR_CANT_UNSET_ADDRESS              -26
#define NSM_API_SET_ERR_CANT_UNSET_ADDRESS_VRRP         -27
#define NSM_API_SET_ERR_CANT_SET_ADDRESS_ON_P2P         -28
#define NSM_API_SET_ERR_NO_PERMISSION                   -29
#define NSM_API_SET_ERR_NO_IPV4_SUPPORT                 -30
#define NSM_API_SET_ERR_NO_IPV6_SUPPORT                 -31
#define NSM_API_SET_ERR_VRF_NOT_EXIST                   -32
#define NSM_API_SET_ERR_VRF_NOT_BOUND                   -33
#define NSM_API_SET_ERR_DIFFERENT_VRF_BOUND             -34
#define NSM_API_SET_ERR_VRF_NAME_TOO_LONG               -35
#define NSM_API_SET_ERR_VRF_CANT_CREATE                 -36
#define NSM_API_SET_ERR_VRF_BOUND                       -37
#define NSM_API_SET_ERR_MPLS_VC_BOUND                   -38
#define NSM_API_SET_ERR_VPLS_BOUND                      -39
#define NSM_API_SET_ERR_NO_SUCH_INTERFACE               -40
#define NSM_API_SET_ERR_NO_MATCHING_ROUTE               -41
#define NSM_API_SET_ERR_MALFORMED_ADDRESS               -42
#define NSM_API_SET_ERR_MALFORMED_GATEWAY               -43
#define NSM_API_SET_ERR_INCONSISTENT_ADDRESS_MASK       -44
#define NSM_API_SET_ERR_FLAG_UP_CANT_SET                -45
#define NSM_API_SET_ERR_FLAG_UP_CANT_UNSET              -46
#define NSM_API_SET_ERR_FLAG_MULTICAST_CANT_SET         -47
#define NSM_API_SET_ERR_FLAG_MULTICAST_CANT_UNSET       -48
#define NSM_API_SET_ERR_NEXTHOP_OWN_ADDR                -49
#define NSM_API_SET_ERR_INTERFACE_GMPLS_TYPE_EXIST      -50
#define NSM_API_SET_ERR_CANT_SET_SECONDARY_FIRST        -51
#define NSM_API_SET_ERR_CANT_CHANGE_PRIMARY             -52
#define NSM_API_SET_ERR_CANT_CHANGE_SECONDARY           -53
#define NSM_API_SET_ERR_SAME_ADDRESS_EXIST              -54
#define NSM_API_SET_ERR_SECONDARY_CANT_BE_SAME_AS_PRIMARY -55
#define NSM_API_SET_ERR_MUST_DELETE_SECONDARY_FIRST     -56
#define NSM_API_SET_ERR_ADDRESS_OVERLAPPED              -57
#define NSM_API_SET_ERR_MAX_STATIC_ROUTE_LIMIT          -58
#define NSM_API_SET_ERR_MAX_FIB_ROUTE_LIMIT             -59
#define NSM_API_SET_ERR_MAX_ADDRESS_LIMIT               -60
#define NSM_API_SET_ERR_CANT_SET_ADDRESS_WITH_ZERO_IFINDEX -61

#define NSM_API_SET_ERR_MASTER_NOT_EXIST                -70
#define NSM_API_SET_ERR_VR_CANT_CREATE                  -71
#define NSM_API_SET_ERR_VR_NOT_EXIST                    -72
#define NSM_API_SET_ERR_VR_BOUND                        -73
#define NSM_API_SET_ERR_VR_NOT_BOUND                    -74
#define NSM_API_SET_ERR_DIFFERENT_VR_BOUND              -75
#define NSM_API_SET_ERR_IPV4_FORWARDING_CANT_CHANGE     -76
#define NSM_API_SET_ERR_IPV6_FORWARDING_CANT_CHANGE     -77
#define NSM_API_SET_ERR_VR_INVALID_NAME                 -78
#define NSM_API_SET_ERR_VR_INVALID_NAMELEN              -79
#define NSM_API_SET_ERR_VR_RESERVED_NAME                -80

#define NSM_API_SET_ERR_EMPTY_AGGREGATOR_DOWN           -90

#ifdef HAVE_BFD
#define NSM_API_SET_ERR_NO_STATIC_ROUTE                 -91
#define NSM_API_SET_ERR_STATIC_BFD_SET                  -92
#define NSM_API_SET_ERR_STATIC_BFD_UNSET                -93
#define NSM_API_SET_ERR_STATIC_BFD_DISABLE_SET          -94
#define NSM_API_SET_ERR_STATIC_BFD_DISABLE_UNSET        -95
#endif /* HAVE_BFD */

#define NSM_API_SET_ERR_TUNNEL_INVALID_MODE             -101
#define NSM_API_SET_ERR_TUNNEL_INVALID_SUBMODE          -102
#define NSM_API_SET_ERR_TUNNEL_IF_NOT_EXIST             -103
#define NSM_API_SET_ERR_TUNNEL_IF_ADD_FAIL              -104
#define NSM_API_SET_ERR_TUNNEL_IF_DELETE_FAIL           -105
#define NSM_API_SET_ERR_TUNNEL_IF_UPDATE_FAIL           -106
#define NSM_API_SET_ERR_TUNNEL_IF_UPDATE_FAIL_TTL       -107
#define NSM_API_SET_ERR_TUNNEL_DEST_MUST_UNCONFIGURED   -108
#define NSM_API_SET_ERR_TUNNEL_DEST_CANT_CONFIGURED     -109

#define NSM_API_SET_ERR_UNNUMBERED_NOT_P2P              -121
#define NSM_API_SET_ERR_UNNUMBERED_SOURCE               -122
#define NSM_API_SET_ERR_UNNUMBERED_RECURSIVE            -123

#define NSM_API_SET_ERR_DUPLEX                          -205
#define NSM_API_SET_ERR_BANDWIDTH                       -206
#define NSM_API_SET_ERR_ARP_AGEING_TIMEOUT              -207
#define NSM_API_SET_ERR_HAL_FAILURE                     -208
#define NSM_API_SET_ARP_GENERAL_ERR                     -209
#define NSM_API_SET_ERR_ENTRY_EXISTS                    -210

/* Static mroute API error codes */
#define NSM_API_SET_ERR_MROUTE_GENERAL_ERR              -300
#define NSM_API_SET_ERR_UNKNOWN_UCAST_PROTO             -301
#define NSM_API_SET_ERR_GW_BCAST                        -302
#define NSM_API_SET_ERR_MROUTE_NO_MEM                   -303
#define NSM_API_SET_ERR_MROUTE_NO_CONFIG                -304
#define NSM_API_ERR_MROUTE_NO_MRIB                      -305
#define NSM_API_SET_ERR_IF_SAME                         -306
#define NSM_API_SET_ERR_IF_NOT_PROXY_ARP                -307

/* Access group API Error codes*/
#define NSM_API_SET_ERR_ACL_INVALID_VALUE               -308
#define NSM_API_ACL_ERR_NOT_SET                         -309

/*GMPLS API Error Codes */
#define NSM_API_SET_ERR_IF_TYPE_NOT_MATCH               -400
#define NSM_API_SET_ERR_IF_TYPE_NOT_DATA                -401
#define NSM_API_SET_ERR_INVALID_IF_ID                   -402
#define NSM_API_SET_ERR_INVALID_SWITCH_CAP              -403    
#define NSM_API_SET_ERR_SWITCH_CAP_ALREADY_CONFIGURED   -404
#define NSM_API_SET_ERR_SWITCH_CAP_NOT_MATCH            -405
#define NSM_API_SET_ERR_INVALID_BW                      -406    
#define NSM_API_SET_ERR_BW_NOT_MATCH                    -407
#define NSM_API_SET_ERR_TE_LINK_NOT_EXIST               -408 
#define NSM_API_SET_ERR_DATA_LINK_NOT_EXIST             -409 
#define NSM_API_SET_ERR_CC_NOT_EXIST                    -410 
#define NSM_API_SET_ERR_CA_NOT_EXIST                    -411 
#define NSM_API_SET_ERR_INVALID_DATA_LINK               -412 
#define NSM_API_SET_ERR_INVALID_TE_LINK                 -413 
#define NSM_API_SET_ERR_INVALID_CC                      -414 
#define NSM_API_SET_ERR_INVALID_CA                      -415 
#define NSM_API_SET_ERR_INVALID_CLASS_TYPE              -416
#define NSM_API_SET_ERR_INVALID_DESC                    -417
#define NSM_API_SET_ERR_INVALID_ADMIN_GROUP             -418 
#define NSM_API_SET_ERR_ADMIN_GROUP_EXIST               -419
#define NSM_API_SET_ERR_ADMIN_GROUP_NOT_EXIST           -420
#define NSM_API_SET_ERR_LINKID_EXIST                    -421
#define NSM_API_SET_ERR_CC_LRADDR_EXIST                 -422
#define NSM_API_SET_ERR_CC_CA_EXIST                     -423
#define NSM_API_SET_ERR_CC_DUP_ADDR                     -424
#define NSM_API_SET_ERR_CA_RNODE_ID_MISMATCH            -425
#define NSM_API_SET_ERR_CA_LMP_MISMATCH                 -426
#define NSM_API_SET_ERR_CA_UNIQUE_NODE_ID               -427
#define NSM_API_SET_ERR_CA_DUP_PRIMARY_CC               -428
#define NSM_API_SET_ERR_DL_ANOTHER_TL                   -429
#define NSM_API_SET_ERR_DL_TL_EXISTS                    -430
#define NSM_API_SET_ERR_ONE_DL_ALLOWED_ON_TL            -431
#define NSM_API_SET_ERR_DL_SW_CAP_MISMATCH              -432 
#define NSM_API_SET_ERR_TL_EXISTS                       -433
#define NSM_API_SET_ERR_LLID_MISMATCH                   -434 
#define NSM_API_SET_ERR_IF_NOT_FOUND                    -435
#define NSM_API_SET_ERR_TE_LINK_UNNUMBERED              -436 
#define NSM_API_SET_ERR_TE_CA_EXIST                     -437 
#define NSM_API_SET_ERR_TE_LABEL_SPACE                  -438 
#define NSM_API_SET_ERR_DL_UNNUMBERED_TL_NUMBERED       -439
#define NSM_API_SET_ERR_DL_NUMBERED_TL_UNNUMBERED       -440
#define NSM_API_SET_ERR_CC_NOT_BINDED_TO_THIS_CA        -441
#define NSM_API_SET_ERR_DL_NOT_BINDED_TO_THIS_TL        -442

#define NSM_API_SET_ERR_BW_IN_USE                       -450  
#define NSM_API_SET_ERR_INVALID_BW_CONSTRAINT           -451 

/* RTADV error codes. */
#define NSM_API_SET_ERR_IPV6_RA_MIN_INTERVAL_VALUE        -500
#define NSM_API_SET_ERR_IPV6_RA_MIN_INTERVAL_CONGRUENT    -501
#define NSM_API_SET_ERR_IPV6_RA_PREFIX_VALID_LIFETIME     -502
#define NSM_API_SET_ERR_IPV6_RA_PREFIX_PREFERRED_LIFETIME -503
/* arp_age_timeout default value */
#define NSM_ARP_AGE_TIMEOUT_DEFAULT                     25

/* API Get Values */
#define NSM_IF_MCAST_IS_SET                              1
#define NSM_IF_MCAST_IS_CLR                              0

/* HW Register Error codes */
#define NSM_API_ERR_HW_REG_GET_FAILED                 -800

/* STATISTICS */
#define NSM_API_ERR_STATS_VLAN_ID_INVALID               -700
#define NSM_API_ERR_STATS_VLAN_ID_FAILED                -701
#define NSM_API_ERR_STATS_PORT_ID_INVALID               -702
#define NSM_API_ERR_STATS_PORT_ID_FAILED                -703
#define NSM_API_ERR_STATS_HOST_ID_FAILED                -704
#define NSM_API_ERR_STATS_CLEAR_FAILED                  -705

/* NSM GMPLS Return codes */
#define NSM_API_ERR_TYPE_DL_ALREADY_CONFIG              -600
#define NSM_API_ERR_TYPE_CHANNELS_DISALLOWED            -601

#ifdef HAVE_PBR
#define NSM_API_SET_ERR_IF_MAP_ENABLED                  -602
#define NSM_API_SET_ERR_MAP_NOT_CONFIGURED              -603
#define NSM_API_SET_ERR_MAP_NOT_CONFIGURED_ON_IF        -604
#endif /* HAVE_PBR */


/* CLI-APIs.  */
int nsm_if_flag_up_set (u_int32_t, char *, bool_t iterate_members);
int nsm_if_flag_up_unset (u_int32_t, char *, bool_t iterate_members);
int nsm_if_flag_multicast_set (u_int32_t, char *);
int nsm_if_flag_multicast_unset (u_int32_t, char *);
int nsm_if_mtu_set (u_int32_t, char *, int);
int nsm_if_mtu_unset (u_int32_t, char *);
int nsm_if_duplex_set (u_int32_t, char *, int);
int nsm_if_duplex_unset (u_int32_t, char *);
#ifdef HAVE_L3
int nsm_if_arp_ageing_timeout_set (u_int32_t, char *, int);
int nsm_if_arp_ageing_timeout_unset (u_int32_t, char *);

int nsm_fib_retain_set (u_int32_t, int);
int nsm_fib_retain_unset (u_int32_t);
#endif /* HAVE_L3 */

/* VR - APIs */
#ifdef HAVE_VR
int nsm_virtual_router_set (char *);
int nsm_virtual_router_unset (char *);
void nsm_server_send_vr_add_by_proto (struct nsm_master *, u_int32_t);
void nsm_server_send_vr_delete_by_proto (struct nsm_master *, u_int32_t);
#endif /*HAVE_VR*/

#ifdef HAVE_TE
s_int32_t nsm_api_if_max_bandwidth_set (u_int32_t, char *, char *);
s_int32_t nsm_api_if_max_bandwidth_unset (u_int32_t, char *);
s_int32_t nsm_api_if_bandwidth_constraint_set (u_int32_t, char *, char *, char *);
s_int32_t nsm_api_if_bandwidth_constraint_unset (u_int32_t, char *, char *);
s_int32_t nsm_api_if_reservable_bandwidth_set (u_int32_t, char *, char *);
s_int32_t nsm_api_if_reservable_bandwidth_unset (u_int32_t, char *);
#endif /*HAVE_TE */

/* Stacking related API's  and structures */
#define NSM_CPU_KEY_SIZE  6
struct nsm_cpu_info_entry
{
  char mac_addr [NSM_CPU_KEY_SIZE];
};

struct nsm_stk_port_info
{
  int unit;
  int port;
  int flags;
  int weight;
};

struct nsm_cpu_dump_entry
{
  char mac_addr [NSM_CPU_KEY_SIZE];
  char pack_1[2]; /* Packing bytes */
  int  num_units;
  int  master_prio;
  int  slot_id;

  /* Stack port info */
  int num_stk_ports;
  struct nsm_stk_port_info stk[5]; /* Allow a maximum of 5 stack ports */
};
/* Size of nsm_cpu_dump_entry assumed to be 104 bytes */

/* Stacking related defines */
#define NSM_STK_PORTS_ON_DEV 5

#define NSM_STK_CPUDB_SPF_NO_LINK           0x01
#define NSM_STK_CPUDB_SPF_DUPLEX            0x02
#define NSM_STK_CPUDB_SPF_TX_RESOLVED       0x04
#define NSM_STK_CPUDB_SPF_RX_RESOLVED       0x08
#define NSM_STK_CPUDB_SPF_TX_DISABLE_FORCE  0x80
#define NSM_STK_CPUDB_SPF_RX_DISABLE_FORCE  0x100
#define NSM_STK_CPUDB_SPF_INACTIVE          0x200
#define NSM_STK_CPUDB_SPF_CUT_PORT          0x400

int nsm_get_num_cpu_entries (u_int32_t *);
int nsm_stacking_set_entry_master (unsigned char*);
int nsm_get_index_cpu_entry (u_int32_t, struct nsm_cpu_info_entry *);
int nsm_get_index_dump_cpu_entry (u_int32_t, struct nsm_cpu_dump_entry *);
int nsm_get_master_cpu_entry (struct nsm_cpu_info_entry *);
int nsm_get_local_cpu_entry (struct nsm_cpu_info_entry *);

/* Validate the MAC address API */
int nsm_api_mac_address_is_valid (char *);

/* Recieve Address Type */
#define OTHER                     1
#define VOLATILE                  2
#define NONVOLATILE               3

#ifdef HAVE_L3
#define NSM_API_ARP_FLAG_STATIC   (1 << 0)
#define NSM_API_ARP_FLAG_DYNAMIC  (1 << 1)

struct nsm_arp_master
{
   struct    ptree *nsm_static_arp_list;
#ifdef HAVE_IPV6
   struct    ptree *ipv6_static_nbr_table;
#endif /* HAVE_IPV6 */
};

struct nsm_static_arp_entry
{
  struct pal_in4_addr addr;
  unsigned char mac_addr [ETHER_ADDR_LEN];
  struct ptree_node *pn;
  struct connected *ifc;
  u_int8_t  is_arp_proxy;
};

#ifdef HAVE_IPV6
struct nsm_ipv6_static_nbr_entry
{
  struct pal_in6_addr addr;
  unsigned char mac_addr [ETHER_ADDR_LEN];
  struct interface *ifp;
};
#endif /* HAVE_IPV6 */

struct nsm_arp_master * nsm_arp_create_master ();

void nsm_arp_destroy_master (struct nsm_master *master);

int  nsm_arp_show (struct cli *);
void nsm_cli_init_arp (struct lib_globals *zg);
int nsm_api_arp_entry_add (struct nsm_master *nm, struct pal_in4_addr *addr,
                           unsigned char *mac_addr, struct connected *ifc,
                           struct interface *,u_int8_t is_proxy_arp);
int nsm_api_arp_entry_del (struct nsm_master *nm, struct pal_in4_addr *addr,
                           unsigned char *mac_addr, struct interface *);
int nsm_delete_static_arp (struct nsm_master *nm, struct connected *ifc,
                           struct interface *);
void nsm_api_arp_del_all (struct nsm_master *, u_char);

int nsm_if_arp_set (struct nsm_master *, struct interface *);
int nsm_if_arp_unset (struct nsm_master *nm, struct interface *ifp);

int nsm_ipv4_forwarding_set (u_int32_t);
int nsm_ipv4_forwarding_unset (u_int32_t);
int nsm_ipv4_route_set (struct nsm_master *, vrf_id_t, struct prefix_ipv4 *,
                        union nsm_nexthop *, char *, int, int, int,
                        u_int32_t, char *);
int nsm_ipv4_route_unset (struct nsm_master *, vrf_id_t, struct prefix_ipv4 *,
                          union nsm_nexthop *, char *, int, u_int32_t, char *);
int nsm_ipv4_route_unset_all (struct nsm_master *, vrf_id_t,
                              struct prefix_ipv4 *);
int nsm_ipv4_route_stale_clear (u_int32_t);

#ifdef HAVE_IPV6
int nsm_api_ipv6_nbr_add (struct nsm_master *, struct prefix_ipv6 *,
                          struct interface *, u_char *);
int nsm_api_ipv6_nbr_del (struct nsm_master *, struct prefix_ipv6 *, struct interface *);
void nsm_api_ipv6_nbr_del_all (struct nsm_master *, u_char);
void nsm_api_ipv6_nbr_all_show (struct cli *);
void nsm_api_ipv6_nbr_del_by_ifp (struct nsm_master *, struct interface *);
int nsm_ipv6_forwarding_set (u_int32_t);
int nsm_ipv6_forwarding_unset (u_int32_t);
int nsm_ipv6_route_set (struct nsm_master *, vrf_id_t, struct prefix_ipv6 *,
                        union nsm_nexthop *, char *, int);
int nsm_ipv6_route_unset (struct nsm_master *, vrf_id_t, struct prefix_ipv6 *,
                          union nsm_nexthop *, char *, int);
int nsm_ipv6_route_unset_all (struct nsm_master *, vrf_id_t,
                              struct prefix_ipv6 *);
int nsm_ipv6_route_stale_clear (u_int32_t);
#endif /* HAVE_IPV6 */

int nsm_router_id_set (u_int32_t, vrf_id_t, struct pal_in4_addr *);
int nsm_router_id_unset (u_int32_t, vrf_id_t);

/* This should be moved to nsm_cli.h. */
int nsm_cli_ipv4_addr_check (struct prefix_ipv4 *, struct interface *);
#ifdef HAVE_IPV6
int nsm_cli_ipv6_addr_check (struct prefix_ipv6 *, struct interface *);
#endif /* HAVE_IPV6 */
int nsm_get_service_type (int protocol_id);
int nsm_get_forward_number_ipv4 ();
bool_t nsm_gateway_ifindex_match (struct pal_in4_addr route_dest,
                                  struct pal_in4_addr route_mask,
                                  struct pal_in4_addr next_hop,
                                  int ifindex);
#endif /* HAVE_L3 */

int nsm_cli_return (struct cli *, int);

#ifdef HAVE_WMI
void nsm_wmi_debug_init (struct lib_globals *);
#endif /* HAVE_WMI */

void  nsm_get_sys_desc (char *sysDesc);
void nsm_get_sys_name (char *sysname);
pal_time_t nsm_get_sys_up_time ();
void nsm_get_sys_location (char *location);
int nsm_get_sys_services (void);
int nsm_if_flag_up_unset (u_int32_t vr_id, char *ifname, bool_t iterate_members);
int nsm_if_flag_up_set (u_int32_t vr_id, char *ifname, bool_t iterate_members);

#ifdef HAVE_SNMP
struct rcvaddr_index
{
  int ifindex;
  u_char mac_addr[ETHER_ADDR_LEN];
};

#if defined HAVE_HAL && defined HAVE_L2
int nsm_get_rcvaddress_status (struct rcvaddr_index *rcvaddr, int *status);
int nsm_get_next_rcvaddress_status (struct rcvaddr_index *rcvaddr, int *status);
int nsm_get_rcvaddress_type (struct rcvaddr_index *rcvaddr, int *type);
int nsm_get_next_rcvaddress_type (struct rcvaddr_index *rcvaddr, int *type);
int nsm_set_rcvaddress_status (struct nsm_master *nm,
                               struct rcvaddr_index *rcvaddr,
                               int status);
int nsm_set_rcvaddress_type (struct rcvaddr_index *rcvaddr, int *type);
#endif /* HAVE_L2 && HAVE_HAL */

oid *
nsm_get_sys_oid (void);
void
nsm_phy_entity_create (struct nsm_master *, u_int32_t, struct interface *);
void
nsm_phy_entity_delete (struct nsm_master *, struct interface *);
void
nsm_logical_entity_create (struct nsm_master *, u_int32_t);
#endif /* HAVE_SNMP */

/* CLI API for Access group */
int nsm_access_group_set (struct apn_vr *vr, char *name, char *direct,
                          char *ifname, int enabled);

int nsm_filter_set_group (struct apn_vr *vr, char *name_str, char *dir_str,
                          char *ifname, int enabled);

u_int32_t
nsm_masklen_to_mask_ipv4 (u_int8_t masklen);

int nsm_filter_set_interface_port_access_group (struct apn_vr *vr, char *name,
                                                char *direct, char *vifname,
                                                struct interface *,
                                                int enabled);

#ifdef HAVE_SMI
int
nsm_interface_get_counters(struct interface *ifp,
                           struct smi_if_stats *ifstats);
#endif /* HAVE_SMI */

int
nsm_interface_clear_counters (struct interface *ifp);

int
nsm_if_mdix_crossover_set(struct interface *ifp, u_int32_t mdix);

int
nsm_if_mdix_crossover_get(struct interface *ifp, u_int32_t *mdix);

int
nsm_bridge_flush_dynamic_fdb_by_mac(char *name, char *mac_addr);

#ifdef HAVE_SMI
int
nsm_apn_get_traffic_class_table(struct interface *ifp,
            u_char traffic_class_table[][SMI_BRIDGE_MAX_TRAFFIC_CLASS + 1]);

int nsm_if_set_bandwidth (struct interface *ifp, float bw);

int nsm_if_non_conf_set (struct interface *ifp, enum smi_port_conf_state state);

int nsm_if_non_conf_get (struct interface *ifp,
                         enum smi_port_conf_state *state);
int
nsm_api_get_port_non_switch (struct interface *ifp,
                             enum smi_port_conf_state *state);

#endif /* HAVE_SMI */

#endif /* _NSM_API_H */
