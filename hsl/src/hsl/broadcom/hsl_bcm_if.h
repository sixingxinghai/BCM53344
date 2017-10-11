/* Copyright (C) 2004-2005  All Rights Reserved. */

#ifndef _HSL_BCM_IF_H_
#define _HSL_BCM_IF_H_

/* Broadcom interface types */
#define HSL_BCM_IF_TYPE_L2_ETHERNET   0
#define HSL_BCM_IF_TYPE_L3_IP         1
#define HSL_BCM_IF_TYPE_TRUNK         2
#define HSL_BCM_IF_TYPE_MPLS          3

#define HSL_BCM_BW_UNIT_10M   10
#define HSL_BCM_BW_UNIT_100M  100
#define HSL_BCM_BW_UNIT_1G    1000
#define HSL_BCM_BW_UNIT_10G   10000
#define HSL_BCM_BW_UNIT_BITS  8
#define HSL_BCM_BW_UNIT_MEGA  1000000

#ifdef HAVE_QOS
#define HSL_QOS_IF_FILTER_MAX    100
#define HSL_QOS_IF_METER_MAX    10

typedef int bcm_filterid_t; //qcl 20170808

struct hsl_bcm_qos_filter
{
  int type;

  union
  {
    bcm_filterid_t filter;
    struct 
    {
      bcm_field_entry_t entry;
      bcm_field_group_t group;
      int meter;
    }field;
  }u;
};
#endif /* HAVE_QOS */

struct hsl_bcm_accgrp_filter
{
  int type;

  union
  {
    bcm_filterid_t filter;
    struct 
    {
      bcm_field_entry_t entry;
      bcm_field_group_t group;
      int meter;
    }field;
  }u;
};

/* Limits for BCM Types*/
#define HSL_ACCGRP_IF_FILTER_MAX           100
#define HSL_ACCGRP_IF_METER_MAX            10

/* Structure to store
   L3 interface indexes
   LAG interface indexes
   Tunnel interface indexes
   Indexes for router L3 interface address indexes. 
*/
struct hsl_bcm_if
{
  int type;

  /* Trunk ID. If trunk ID is -1, its a non-aggregated link. */
  bcm_trunk_t trunk_id;

  union 
  {
    struct 
    {
      /* L3 interface index. */
      int ifindex;

      /* VID. */
      hsl_vid_t vid;

      /* MAC address. */
      bcm_mac_t mac;

      /* Pointer for the reserved vlan, if any for this interface. */
      struct hsl_bcm_resv_vlan *resv_vlan;
    } l3;
    
    struct
    {
      /* Logical port. */
      bcmx_lport_t lport;
#ifdef HAVE_QOS
      /* Filters */
      struct hsl_bcm_qos_filter filters[HSL_QOS_IF_FILTER_MAX];
      int filter_num;

      /* Meters */
      int meters[HSL_QOS_IF_METER_MAX];
      int meter_num;
#endif /* HAVE_QOS */
        
       struct hsl_bcm_accgrp_filter ip_filters[HSL_ACCGRP_IF_FILTER_MAX];
       int ip_filter_num;

      /* Meters */
      int ip_meters[HSL_ACCGRP_IF_METER_MAX];
      int ip_meter_num;

    } l2;

#ifdef HAVE_MPLS
    struct 
    {
      /* L3 interface index. */
      int ifindex;

      /* Flags */
      #define HSL_BCM_IF_FLAG_TUNNEL_INSTALLED  (1 << 0)
      u_char flags;
    } mpls;
#endif /* HAVE_MPLS */
  } u;
};

#define BCMIFP_L2(BCMIFP)                 (BCMIFP)->u.l2
#define BCMIFP_L3(BCMIFP)                 (BCMIFP)->u.l3

/* Function prototypes. */
struct hsl_bcm_if *hsl_bcm_if_alloc ();
void hsl_bcm_if_free (struct hsl_bcm_if *ifp);
struct hsl_bcm_ifaddr *hsl_bcm_ifaddr_alloc ();
void hsl_bcm_ifaddr_free (struct hsl_bcm_ifaddr *ifp);
int hsl_bcm_if_l2_flags_set (struct hsl_if *ifp, unsigned long flags);
int hsl_bcm_if_l2_flags_unset (struct hsl_if *ifp, unsigned long flags);
int hsl_bcm_if_l2_unregister (struct hsl_if *ifp);
void *hsl_bcm_if_l3_configure (struct hsl_if *ifp, void *data);
int hsl_bcm_if_l3_unconfigure (struct hsl_if *ifp);
int hsl_bcm_if_l3_address_add (struct hsl_if *ifp, hsl_prefix_t *prefix, u_char flags);
int hsl_bcm_if_l3_address_delete (struct hsl_if *ifp, hsl_prefix_t *prefix);
int hsl_bcm_if_l3_flags_set (struct hsl_if *ifp, unsigned long flags);
int hsl_bcm_if_l3_flags_unset (struct hsl_if *ifp, unsigned long flags);
int hsl_bcm_if_add_or_remove_l3_table_entries (struct hsl_if *ifp, int add);
#ifdef HAVE_IPV6
int hsl_bcm_if_add_or_remove_ipv6_l3_table_entries (struct hsl_if *ifp, int add);
#endif
int hsl_if_hw_cb_register (void);
int hsl_if_hw_cb_unregister (void);
int hsl_bcm_port_mirror_init (void);
int hsl_bcm_port_mirror_deinit (void);
int hsl_bcm_port_mirror_set (struct hsl_if *to_ifp, struct hsl_if *from_ifp, enum hal_port_mirror_direction direction);
int hsl_bcm_port_mirror_unset (struct hsl_if *to_ifp, struct hsl_if *from_ifp, enum hal_port_mirror_direction direction);
int hsl_bcm_get_port_mtu(bcmx_lport_t lport, int *mtu);
int hsl_bcm_set_port_mtu(bcmx_lport_t lport, int mtu);
int hsl_bcm_get_port_duplex(bcmx_lport_t lport, u_int32_t *duplex);
int hsl_bcm_add_port_to_vlan (hsl_vid_t vid, bcmx_lport_t lport, int egress);
int hsl_bcm_remove_port_from_vlan (hsl_vid_t vid, bcmx_lport_t lport);
int hsl_bcm_copy_64_int(uint64 *src,ut_int64_t *dst);
uint64 hsl_bcm_convert_to_64_int(ut_int64_t *src);
#ifdef HAVE_VLAN_STACK
int hsl_bcm_vlan_stacking_enable (u_int32_t ifindex, u_int16_t tpid, int mode);
#endif /* HAVE_VLAN_STACK */
#if defined HAVE_PROVIDER_BRIDGE || defined HAVE_VLAN_STACK
int hsl_bcm_provider_vlan_disable (u_int32_t ifindex);
#endif /* HAVE_VLAN_STACK || HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_MPLS
void *hsl_bcm_if_mpls_configure (struct hsl_if *ifp, void *data);
int hsl_bcm_if_mpls_unconfigure (struct hsl_if *ifp);
int hsl_bcm_if_mpls_up (struct hsl_if *);

#endif /* HAVE_MPLS */
/* Macro for indicating the setting of Source and destination IP*/
#define HSL_ACCGRP_FILTER_SRC_IP                   (1<<2)
#define HSL_ACCGRP_FILTER_DST_IP                   (1<<3)

/* Type of filter applied*/
#define HAL_ACCGRP_FILTER_DENY             0   /* deny */
#define HAL_ACCGRP_FILTER_PERMIT           1   /* permit */
/* Filed Offsets*/
#define HSL_ACCGRP_DST_IP_OFFSET          34 
#define HSL_ACCGRP_SRC_IP_OFFSET          30 
#define HSL_ACCGRP_VLAN_OFFSET            14
#define HSL_ACCGRP_VLAN_MASK              0x0fff

#endif /* _HSL_BCM_IF_H_ */

