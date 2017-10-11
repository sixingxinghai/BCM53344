/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_IFMGR_H_
#define _HSL_IFMGR_H_

#include "hal_if.h"
#ifdef HAVE_L3
#include "hsl_fib.h"
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
/*
  MPLS port interface start, max.
*/
#define HSL_MPLS_IFINDEX_START                      6000
#define HSL_MPLS_IFINDEX_TOTAL                      50000
#define HSL_MPLS_IFINDEX_MAX                        (HSL_MPLS_IFINDEX_START + HSL_MPLS_IFINDEX_TOTAL)
#endif /* HAVE_MPLS */

/*
  L2 port interface start, max.
*/
#define HSL_L2_IFINDEX_START                        5000
#define HSL_L2_IFINDEX_TOTAL                        100
#define HSL_L2_IFINDEX_MAX                          (HSL_L2_IFINDEX_START + HSL_L2_IFINDEX_TOTAL)

#define HSL_IF_DUPLEX_HALF                          0 
#define HSL_IF_DUPLEX_FULL                          1 
#define HSL_IF_DUPLEX_AUTO                          2
#define HSL_IF_AUTONEGO_DISABLE                     0
#define HSL_IF_AUTONEGO_ENABLE                      1

#define HSL_IF_BW_UNIT_10M                          10
#define HSL_IF_BW_UNIT_100M                         100
#define HSL_IF_BW_UNIT_1G                           1000
#define HSL_IF_UNIT_10G                             10000
#define HSL_IF_BW_UNIT_BITS                         8
#define HSL_IF_BW_UNIT_MEGA                         1000000

#define HSL_IF_CINDEX_FLAGS                          0
#define HSL_IF_CINDEX_METRIC                         1
#define HSL_IF_CINDEX_MTU                            2
#define HSL_IF_CINDEX_HW                             3
#define HSL_IF_CINDEX_BANDWIDTH                      4
#define HSL_IF_CINDEX_DUPLEX                         5
#define HSL_IF_CINDEX_AUTONEGO                       6
#define HSL_IF_CINDEX_ARP_AGEING_TIMEOUT             7
#define HSL_IF_CINDEX_MDIX                           8


#define HSL_IF_INC_OP_COUNT                         (1)
#define HSL_IF_DEC_OP_COUNT                         (2)

/* 
  Forward Declarations 
 */

struct hsl_if_resv_vlan;

typedef struct hsl_if_resv_vlan hsl_if_resv_vlan_t;

/* 
  Interface properties definition. 
 */
#define HSL_IF_CPU_ONLY_INTERFACE                    (0x10)

/* 
   Interface types. 
*/
typedef enum
  {
    HSL_IF_TYPE_UNK         = 0,            /* Unknown. */
    HSL_IF_TYPE_LOOPBACK    = 1,            /* Loopback. */
    HSL_IF_TYPE_IP          = 2,            /* IP. */
    HSL_IF_TYPE_L2_ETHERNET = 3,            /* Ethernet. */
    HSL_IF_TYPE_MPLS        = 4,            /* MPLS. */
    HSL_IF_TYPE_TUNNEL      = 5             /* Tunnel */
  } hsl_ifType_t;

/* 
   IP forwarding mode. 
*/
typedef enum
  {
    HSL_IF_IP_FORWARDING_ENABLE = 1,    /* Enable IP forwarding. */
    HSL_IF_IP_FORWARDING_DISABLE = 2    /* Disable IP forwarding. */
  } hsl_IfMode_t;

/*  
    Operational Status code.
*/
typedef enum 
  {
    HSL_IF_OPER_STATUS_UP = 1,          /* Operationally UP   */
    HSL_IF_OPER_STATUS_DOWN = 2,        /* Operationally DOWN */
    HSL_IF_OPER_STATUS_UNKNOWN = 3      /* Status unknown     */
  } hsl_IfOperStatus_t;

/*
  Switching type for a interface
*/
typedef enum
  {
    HSL_IF_SWITCH_L2 = 1,               /* L2 switching only. */
    HSL_IF_SWITCH_L2_L3 = 2,            /* L2/L3 switching. */
    HSL_IF_SWITCH_L3 = 3                /* L3 switching only. */
  } hsl_IfSwitchType_t;

/* 
   Administrative Status code. 
*/
typedef enum 
  {
    HSL_IF_ADMIN_STATUS_UP = 1,           /* Administratively UP   */
    HSL_IF_ADMIN_STATUS_DOWN = 2          /* Administratively DOWN */
  } hsl_IfAdminStatus_t;

/* 
   IP interface address prefix list. 
*/
typedef struct _hsl_prefix_list
{
  /* Prefix. */
  hsl_prefix_t prefix;

  /* Next. */
  struct _hsl_prefix_list *next;

  /* Flags. */
  u_char flags;
#define HSL_IFMGR_IP_ADDR_SECONDARY     (1 << 0)

} hsl_prefix_list_t;

/* 
   IPv4 Interface. 
*/
struct _hsl_ip_if
{
  u_int16_t mtu;                   /* IPv(4|6) MTU. */
  u_int16_t nucAddr;               /* Number of UC IP address. */
  hsl_prefix_list_t *ucAddr;       /* Array of unicast IP addresses. */
  hsl_IfMode_t mode;               /* IPv(4|6) forwarding mode. */
};

/* 
   IPv4 interface. 
*/
typedef struct _hsl_ip_if hsl_IfIPv4_t;

/* 
   IPv6 interface. 
*/
typedef struct _hsl_ip_if hsl_IfIPv6_t;

/* 
   IPv4/IPv6 interface. 
*/
typedef struct _hsl_ip
{
  hsl_IfIPv4_t         ipv4;           /* IPv4 */
  hsl_IfIPv6_t         ipv6;           /* IPv6 */
  hsl_mac_address_t    mac;            /* Primary MAC addresses for logical 
                                          interfaces. */
  u_int8_t             nAddrs;         /* Number of secondary MAC addresses. */
  hsl_mac_address_t    *addresses;     /* Array of secondary MAC addresses. */
  hsl_vid_t            vid;            /* VID. */
  hsl_if_resv_vlan_t   *resv_vlan;     /* Pointer to reserved vlan */
  u_int32_t            arpTimeout;     /* Max time to keep ARP entries active. */
  u_int16_t            mtu;            /* MTU. */
} hsl_ifIP_t;

typedef struct
{
  int       frame_period_monitor;
  u_int32_t frame_period_window;
  u_int64_t efm_frame_count_initial; /* Packet counter at the start of the window */
  struct hal_if_counters efm_cntrs;  /* Interface Efm counters. */
  u_int32_t efm_efs_count;           /* Errored Frame Seconds count */
} hsl_efm_err_frame;

/*  LAN(L2) interface attributes. */
typedef struct 
{
  hsl_ifIndex_t     ifindex;        /* Ifindex. */
  u_int32_t         speed;          /* Speed. */
  u_int32_t         mtu;            /* MTU. */
  u_int32_t         duplex;         /* DUPLEX. */
  u_int32_t         autonego;       /* AUTONEGO. */
  u_int32_t         bandwidth;      /* BANDWIDTH. */
  hsl_mac_address_t mac;            /* Source mac address. */
#ifdef HAVE_L2
  struct hsl_bridge_port *port;
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD
   hsl_efm_err_frame efm_err_frame;
#endif /* HAVE_ONMD*/
}  hsl_ifL2_ethernet_t;

#ifdef HAVE_MPLS
/* 
   MPLS interface. 
*/
typedef struct _hsl_mpls
{
  u_int16_t mtu;             /* MTU. */
  hsl_mac_address_t mac;     /* MAC addresses for logical interfaces. */
  hsl_vid_t vid;             /* VID. */
  struct hal_msg_mpls_ftn_add *label_info; /* Label info */
} hsl_ifMPLS_t;
#endif /* HAVE_MPLS */

/*
  Interface list for parent-children.
*/
struct hsl_if;
struct hsl_if_list
{
  struct hsl_if *ifp;
  struct hsl_if_list *next;
};
/* 
  Interface mac counters structure. 
*/
struct hal_if_counters; 

#ifdef HAVE_VRRP1
/* VRRP INFO */
struct hsl_vip_addr_struc
  {
    struct hal_in4_addr vip;                   /* Virtual IP addresss */
    int vrid;                             /* VRRP virtual router ID */
    int pkt_cnt;
  };

#ifdef HAVE_IPV6
struct hsl_vip6_addr_struc
  {
    struct hal_in6_addr vip6;             /* Virtual IPv6 addresss */
    int vrid;                             /* VRRP virtual router ID */
  };
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

/* 
   Interface structure. 
*/
struct hsl_if
{
  hsl_ifIndex_t ifindex;             /* Ifindex. */
  
  hsl_ifType_t type;                 /* Type. */

#ifdef HAVE_L3
  hsl_fib_id_t fib_id;               /* FIB id. */
#endif /* HAVE_L3 */
  
  oss_atomic_t val;                  /* Hold counter. */

#define HSL_IFNAM_SIZE               20
  char name[HSL_IFNAM_SIZE + 1];     /* Interface name. */

  char mapped_name[HSL_IFNAM_SIZE + 1]; /* Mapped name. */

  u_char operCnt;                    /* Operational status count. */
  u_int32_t flags;                   /* IFF_ flags. */

  u_char if_flags;             /* directly mapped for 
                                        eg: in case of router ports. */
#define HSL_IFMGR_IF_DIRECTLY_MAPPED (1 << 0)
#define HSL_IFMGR_IF_L3_DELETE       (1 << 1)
#define HSL_IFMGR_IF_INTERNAL        (1 << 2)

  u_int32_t pkt_flags;               /* Acceptable packet types. See below. */
  u_int32_t if_property;             /* Type. */

  union
  {
    hsl_ifIP_t ip;
    hsl_ifL2_ethernet_t l2_ethernet;
#ifdef HAVE_MPLS
    hsl_ifMPLS_t mpls;
#endif /* HAVE_MPLS */
  } u;

  void *os_info;                     /* OS specific interface structure. */
  void *os_info_ext;                 /* OS specific interface structure. */

  void *system_info;                 /* System specific interface structure. */

  struct hsl_if_list *parent_list;   /* Parent list. */
  struct hsl_if_list *children_list; /* Children list. */
  struct hal_if_counters mac_cntrs;  /* Interface mac counters. */

#ifdef HAVE_VRRP
  /* Global List Pointers */
  struct hsl_avl_tree    *vrrp_vip_tree;

#ifdef HAVE_IPV6
  struct hsl_avl_tree    *vrrp_vip6_tree;
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

};

#define IFP_IP(IFP)                (IFP)->u.ip
#define IFP_L2(IFP)                (IFP)->u.l2_ethernet

/*
  Interface atomic macros.
*/
#define HSL_IFMGR_IF_REF_DEC_AND_TEST(IFP)                          oss_atomic_dec_and_test (&(IFP)->val)
#define HSL_IFMGR_IF_REF_SET(IFP,VAL)                               oss_atomic_set (&(IFP)->val, (VAL))
#define HSL_IFMGR_IF_REF_INC(IFP)                                   oss_atomic_inc (&(IFP)->val)

#ifdef HAVE_L3
#define HSL_IFMGR_IF_REF_DEC(IFP)                                                                    \
   do {                                                                                              \
     if (oss_atomic_dec_and_test (&(IFP)->val))                                                      \
       {                                                                                             \
         if ((IFP)->type == HSL_IF_TYPE_IP)                                                          \
           {                                                                                         \
             hsl_ifmgr_L3_delete2 ((IFP), 1);                                                        \
           }                                                                                         \
         else if ((IFP)->type == HSL_IF_TYPE_L2_ETHERNET)                                            \
           {                                                                                         \
             hsl_ifmgr_L2_ethernet_delete2 ((IFP), 1);                                               \
           }                                                                                         \
       }                                                                                             \
    } while (0)

#define HSL_IFMGR_L3_INTF_LOOP(T,V,N)                                                               \
  if (T)                                                                                            \
    for ((N) = hsl_avl_top ((T)); (N); (N) = hsl_avl_next ((N)))                                            \
      if ((((V) = (N)->info) != NULL) && (V->type == HSL_IF_TYPE_IP))

#else  /* HAVE_L3 */
#define HSL_IFMGR_IF_REF_DEC(IFP)                                                                    \
   do {                                                                                              \
     if (oss_atomic_dec_and_test (&(IFP)->val))                                                      \
       {                                                                                             \
         if ((IFP)->type == HSL_IF_TYPE_L2_ETHERNET)                                                 \
           {                                                                                         \
             hsl_ifmgr_L2_ethernet_delete2 ((IFP), 1);                                               \
           }                                                                                         \
       }                                                                                             \
    } while (0)
#endif /* HAVE_L3 */ 
/* 
   Interface manager notifier chain. 
*/
struct hsl_if_notifier_chain
{
  /* Notifier function. */
  int (*notifier) (int msg, void *, void *);

  /* Next. */
  struct hsl_if_notifier_chain *next;
  
  /* Priority. */
  int priority;
};

/* 
   Interface manager call notifier events. 
*/
enum hsl_if_notifier_events
  {
    HSL_IF_EVENT_IFNEW          = 100,
    HSL_IF_EVENT_IFDELETE       = 101,
    HSL_IF_EVENT_IFFLAGS        = 102,
    HSL_IF_EVENT_IFNEWADDR      = 103,
    HSL_IF_EVENT_IFDELADDR      = 104,
    HSL_IF_EVENT_IFMTU          = 105,
    HSL_IF_EVENT_IFHWADDR       = 106,
    HSL_IF_EVENT_IFDUPLEX       = 107,
    HSL_IF_EVENT_IFAUTONEGO     = 108,
    HSL_IF_EVENT_IFBANDWIDTH    = 109,
    HSL_IF_EVENT_IFARPAGEINGTIMEOUT = 110,
    HSL_IF_EVENT_IF_UPDADDR     = 111,
    HSL_IF_EVENT_STP_REFRESH    = 112,
    HSL_IF_EVENT_MDIX           = 113,
    HSL_IF_EVENT_PORTBASED_VLAN = 114,
    HSL_IF_EVENT_PORT_EGRESS    = 115,
    HSL_IF_EVENT_CPU_DEFAULT_VLAN = 116,
    HSL_IF_EVENT_FORCE_VLAN     = 117,
    HSL_IF_EVENT_ETHERTYPE      = 118,
    HSL_IF_EVENT_LEARN_DISABLE  = 119,
    HSL_IF_EVENT_SW_RESET       = 120,
    HSL_IF_EVENT_WAYSIDE_DEFAULT_VLAN = 121,
    HSL_IF_EVENT_PRESERVE_CE_COS = 122,
    HSL_IF_EVENT_FPWINDOW_EXPIRY = 123,    
  };

/* 
   Interface manager database. 
*/
struct hsl_if_db
{
  apn_sem_id ifmutex;                       /* Mutex. */

  struct hsl_avl_tree *if_tree;             /* AVL tree based on ifindex. */

  struct hsl_if_notifier_chain *chain;      /* Notification chain. */

  struct hsl_ifmgr_os_callbacks *os_cb;     /* OS callbacks. */

  struct hsl_ifmgr_hw_callbacks *hw_cb;     /* HW callbacks. */

  struct hsl_ifmgr_cust_callbacks *cm_cb;   /* Custom callbacks. */

  int (*proc_if_create_cb)(struct hsl_if *ifp);    /* Proc Create callbacks. */

  int (*proc_if_remove_cb)(struct hsl_if *ifp);    /* Proc Delete callbacks. */

  u_int8_t policy;                          /* Policy for initialization
                                               of ports. */
#define HSL_IFMGR_IF_INIT_POLICY_NONE       0
#define HSL_IFMGR_IF_INIT_POLICY_L2         1
#define HSL_IFMGR_IF_INIT_POLICY_L3         2
};

/* 
   Interface MAC counters structure;
 */
struct hal_if_counters;
/* 
   Extern declaration for interface manager database. 
*/
extern struct hsl_if_db *p_hsl_if_db;
#define HSL_IFMGR_TREE        (p_hsl_if_db->if_tree)

/* Convenience macros. */
#define HSL_IFMGR_LOCK        oss_sem_lock(OSS_SEM_MUTEX, p_hsl_if_db->ifmutex, OSS_WAIT_FOREVER)
#define HSL_IFMGR_UNLOCK      oss_sem_unlock(OSS_SEM_MUTEX, p_hsl_if_db->ifmutex)

#define HSL_IFMGR_HWCB_CHECK(FN)  (p_hsl_if_db->hw_cb && p_hsl_if_db->hw_cb->FN)
#define HSL_IFMGR_HWCB_CALL(FN)   (*p_hsl_if_db->hw_cb->FN)

#define HSL_IFMGR_STACKCB_CHECK(FN) (p_hsl_if_db->os_cb && p_hsl_if_db->os_cb->FN)
#define HSL_IFMGR_STACKCB_CALL(FN) (*p_hsl_if_db->os_cb->FN)

#define HSL_IFP_ADMIN_UP(IFP) ((IFP)->flags & IFF_UP)
#define HSL_IFP_OPER_UP(IFP)  ((IFP)->flags & IFF_RUNNING)

/*
  Acceptable packet types from a port.
*/
#define HSL_IF_PKT_NONE              0
#define HSL_IF_PKT_ARP               (1 << 0)
#define HSL_IF_PKT_RARP              (1 << 1)
#define HSL_IF_PKT_IP                (1 << 2)
#define HSL_IF_PKT_BPDU              (1 << 3)
#define HSL_IF_PKT_GMRP              (1 << 4)
#define HSL_IF_PKT_GVRP              (1 << 5)
#define HSL_IF_PKT_EAPOL             (1 << 6)
#define HSL_IF_PKT_LACP              (1 << 7)
#define HSL_IF_PKT_BCAST             (1 << 8)
#define HSL_IF_PKT_MCAST             (1 << 9)
#define HSL_IF_PKT_IGMP_SNOOP        (1 << 10)
#define HSL_IF_PKT_MLD_SNOOP         (1 << 11)
#define HSL_IF_PKT_LLDP              (1 << 12)
#define HSL_IF_PKT_EFM               (1 << 13)
#define HSL_IF_PKT_CFM               (1 << 14)
#define HSL_IF_PKT_L2                (HSL_IF_PKT_BPDU | HSL_IF_PKT_GMRP | HSL_IF_PKT_GVRP  \
                                      | HSL_IF_PKT_EAPOL | HSL_IF_PKT_LACP                 \
                                      | HSL_IF_PKT_IGMP_SNOOP | HSL_IF_PKT_MLD_SNOOP       \
                                      | HSL_IF_PKT_LLDP | HSL_IF_PKT_EFM | HSL_IF_PKT_CFM)
#define HSL_IF_PKT_ALL               0xffffffff

#define HSL_IF_PKT_CHECK_ARP(ifp)    (ifp->pkt_flags & HSL_IF_PKT_ARP)
#define HSL_IF_PKT_CHECK_RARP(ifp)   (ifp->pkt_flags & HSL_IF_PKT_RARP)
#define HSL_IF_PKT_CHECK_IP(ifp)     (ifp->pkt_flags & HSL_IF_PKT_IP)
#define HSL_IF_PKT_CHECK_BPDU(ifp)   (ifp->pkt_flags & HSL_IF_PKT_BPDU)
#define HSL_IF_PKT_CHECK_GMRP(ifp)   (ifp->pkt_flags & HSL_IF_PKT_GMRP)
#define HSL_IF_PKT_CHECK_GVRP(ifp)   (ifp->pkt_flags & HSL_IF_PKT_GVRP)
#define HSL_IF_PKT_CHECK_EAPOL(ifp)  (ifp->pkt_flags & HSL_IF_PKT_EAPOL)
#define HSL_IF_PKT_CHECK_LACP(ifp)   (ifp->pkt_flags & HSL_IF_PKT_LACP)
#define HSL_IF_PKT_CHECK_BCAST(ifp)  (ifp->pkt_flags & HSL_IF_PKT_BCAST)
#define HSL_IF_PKT_CHECK_MCAST(ifp)  (ifp->pkt_flags & HSL_IF_PKT_MCAST)
#define HSL_IF_PKT_CHECK_IGMP_SNOOP(ifp) (ifp->pkt_flags & HSL_IF_PKT_IGMP_SNOOP)
#define HSL_IF_PKT_CHECK_MLD_SNOOP(ifp)  (ifp->pkt_flags & HSL_IF_PKT_MLD_SNOOP)
#define HSL_IF_PKT_CHECK_LLDP(ifp)       (ifp->pkt_flags & HSL_IF_PKT_LLDP)
#define HSL_IF_PKT_CHECK_EFM(ifp)        (ifp->pkt_flags & HSL_IF_PKT_EFM)
#define HSL_IF_PKT_CHECK_CFM(ifp)        (ifp->pkt_flags & HSL_IF_PKT_CFM)

/*
  Function prototypes.
*/
struct hsl_if *hsl_ifmgr_lookup_by_index (hsl_ifIndex_t ifindex);
struct hsl_if *hsl_ifmgr_lookup_by_name (char *name);
struct hsl_if *hsl_ifmgr_lookup_by_name_type (char *name, hsl_ifType_t type);
int hsl_ifmgr_notify_chain_register (struct hsl_if_notifier_chain *new);
int hsl_ifmgr_notify_chain_unregister (struct hsl_if_notifier_chain *old);
int hsl_ifmgr_init (void);
int hsl_ifmgr_deinit (void);
u_int8_t hsl_ifmgr_get_policy (void);
void hsl_ifmgr_set_policy (u_int8_t policy);
struct hsl_if *hsl_ifmgr_get_first_L2_port (struct hsl_if *ifp);
struct hsl_if *hsl_ifmgr_get_matching_L3_port (struct hsl_if *ifp, hsl_vid_t vid);
void hsl_ifmgr_lock_parents (struct hsl_if *ifp);
void hsl_ifmgr_unlock_parents (struct hsl_if *ifp);
void hsl_ifmgr_lock_children (struct hsl_if *ifp);
void hsl_ifmgr_unlock_children (struct hsl_if *ifp);
int hsl_ifmgr_bind2 (struct hsl_if *ifpp, struct hsl_if *ifpc);
int hsl_ifmgr_clean_up_complete(hsl_ifIndex_t  ifindex);
HSL_BOOL hsl_ifmgr_isbound (struct hsl_if *ifpp, struct hsl_if *ifpc);
int hsl_ifmgr_bind (hsl_ifIndex_t parentIfindex, hsl_ifIndex_t childIfindex);
int hsl_ifmgr_unbind (hsl_ifIndex_t parentIfindex, hsl_ifIndex_t childIfindex);
int hsl_ifmgr_unbind2 (struct hsl_if *ifpp, struct hsl_if *ifpc);
void hsl_ifmgr_L2_ethernet_delete (struct hsl_if *ifp, int send_notification);
void hsl_ifmgr_L2_ethernet_delete2 (struct hsl_if *ifp, int send_notification);
int hsl_ifmgr_L2_ethernet_register (char *name, hsl_mac_address_t mac,
                                    u_int16_t mtu, u_int32_t speed,
                                    u_int32_t duplex, u_int32_t flags,
                                    void *sys_info, u_int8_t if_flags,
                                    struct hsl_if **ppifp);
int hsl_ifmgr_L2_ethernet_unregister (struct hsl_if *ifp);
int hsl_ifmgr_L2_ethernet_unregister2 (hsl_ifIndex_t ifindex);
int hsl_ifmgr_L3_register (char *ifname, u_char *hwaddr, int hwaddrlen, void *data, struct hsl_if **ppifp);
int hsl_ifmgr_L3_unregister (char *name, hsl_ifIndex_t ifindex);
int hsl_ifmgr_L3_loopback_register (char *name, hsl_ifIndex_t ifindex, int mtu, u_int32_t flags, void *osifp);
int hsl_ifmgr_L3_cpu_if_register (char *name, hsl_ifIndex_t ifindex, int mtu, int speed, 
                                      u_int32_t duplex, u_int32_t flags, char *hw_addr,void *osifp);
int hsl_ifmgr_L2_link_up   (struct hsl_if *ifp, u_int32_t speed, u_int32_t duplex);
int hsl_ifmgr_L2_link_down (struct hsl_if *ifp, u_int32_t speed, u_int32_t duplex);


void hsl_ifmgr_L3_delete (struct hsl_if *ifp, int send_notification);
int hsl_ifmgr_L3_create (char *ifname, u_char *hwaddr, int hwaddrlen, int send_notification, void *data, struct hsl_if **ppifp);
void hsl_ifmgr_L3_delete2 (struct hsl_if *ifp, int send_notification);
int hsl_ifmgr_set_flags (char *name, hsl_ifIndex_t ifindex, u_int32_t flags);
int hsl_ifmgr_unset_flags (char *name, hsl_ifIndex_t ifindex, u_int32_t flags);
int hsl_ifmgr_set_flags2 (struct hsl_if *ifp, u_int32_t flags);
int hsl_ifmgr_unset_flags2 (struct hsl_if *ifp, u_int32_t flags);
#ifdef HAVE_L3
int hsl_ifmgr_os_ip_address_add (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix, u_char flags);
int hsl_ifmgr_os_ip_address_delete (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix);
int hsl_ifmgr_ipv4_address_add (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix, u_char flags);
int hsl_ifmgr_ipv4_address_delete (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix);
#endif /* HAVE_L3 */
int hsl_ifmgr_set_os_callbacks (struct hsl_ifmgr_os_callbacks *cb);
int hsl_ifmgr_unset_os_callbacks (void);
int hsl_ifmgr_set_hw_callbacks (struct hsl_ifmgr_hw_callbacks *cb);
int hsl_ifmgr_unset_hw_callbacks (void);
int hsl_ifmgr_set_mtu (hsl_ifIndex_t ifindex, int mtu, HSL_BOOL send_notification);
int hsl_ifmgr_set_duplex (hsl_ifIndex_t ifindex, int duplex,HSL_BOOL send_notification);
int hsl_ifmgr_set_arp_ageing_timeout (hsl_ifIndex_t ifindex, int arp_ageing_timeout);
int hsl_ifmgr_set_router_port (struct hsl_if *ifp, void *data, struct hsl_if **ppifp, HSL_BOOL send_notification);

#ifdef HAVE_L2
int hsl_ifmgr_init_policy_l2 (void);

#ifdef HAVE_L3
int hsl_ifmgr_set_switch_port (struct hsl_if *ifp, struct hsl_if **ppifp, HSL_BOOL send_notification);
#endif /* HAVE_L3 */

#endif /* HAVE_L2 */

#ifdef HAVE_ONMD 
int
hsl_ifmgr_efm_frame_period_window_set (hsl_ifIndex_t ifindex, 
                                       u_int32_t window);

int
hsl_ifmgr_efm_get_frame_errors (struct hsl_if *ifp,
                                struct hal_if_counters *diff_cntrs,
                                u_int32_t *total_errors);
int
compute_frame_errors (struct hsl_if *ifp, u_int32_t *total_errors);

int
hsl_efm_frame_errors (struct hsl_if *ifp, struct hal_if_counters *diff_cntrs);

int
hsl_ifmgr_compute_efm_counters (struct hsl_if *ifp);

#endif /*HAVE_ONMD*/

void hsl_ifmgr_dump (void);
void hsl_ifmgr_dump_detail (void);
void hsl_ifmgr_set_acceptable_packet_types (struct hsl_if *ifp, u_int32_t pkt_flags);
void hsl_ifmgr_unset_acceptable_packet_types (struct hsl_if *ifp, u_int32_t pkt_flags);
int hsl_ifmgr_get_if_counters(hsl_ifIndex_t ifindex, struct hal_if_counters *cntrs);
int hsl_ifmgr_stats_flush(hsl_ifIndex_t ifindex); 
void hsl_ifmgr_add_mac_counters(struct hal_if_counters *a,
                                struct hal_if_counters *b,
                                struct hal_if_counters *res);
void hsl_ifmgr_sub_mac_counters(struct hal_if_counters *a,
                                struct hal_if_counters *b,
                                struct hal_if_counters *res);
int hsl_ifmgr_L2_ethernet_create (char *name, hsl_mac_address_t mac,
                                  u_int16_t mtu, u_int32_t speed,
                                  u_int32_t duplex, u_int32_t flags,
                                  void *sys_info, int send_notification,
                                  u_char if_flags, struct hsl_if **ppifp);
int hsl_ifmgr_set_ether_type (hsl_ifIndex_t ifindex, u_int16_t etype,
                          HSL_BOOL send_notification);
int hsl_ifmgr_send_notification (int event, void *param1, void *param2);
int hsl_ifmgr_get_L2_array(struct hsl_if ***ifp_arr, u_int16_t *arr_size);

int hsl_if_os_cb_unregister (void);
int hsl_if_os_cb_register (void);
int hsl_if_hw_cb_register (void);
int hsl_if_hw_cb_unregister (void);

int hsl_ifmgr_set_hwaddr (hsl_ifIndex_t ifindex, int hwaddrlen, u_char *hwaddr, HSL_BOOL send_notification);
#ifdef HAVE_L3
int hsl_ifmgr_set_secondary_hwaddrs (hsl_ifIndex_t ifindex, int hwaddrlen, int num, u_char **hwaddrs, HSL_BOOL send_notification);
int hsl_ifmgr_add_secondary_hwaddrs (hsl_ifIndex_t ifindex, int hwaddrlen, int num,  u_char **hwaddrs, HSL_BOOL send_notification);
int hsl_ifmgr_delete_secondary_hwaddrs (hsl_ifIndex_t ifindex, int hwaddrlen, int num, u_char **hwaddrs, HSL_BOOL send_notification);
int hsl_ifmgr_if_bind_fib (hsl_ifIndex_t ifindex, hsl_fib_id_t fib_id);
int hsl_ifmgr_if_unbind_fib (hsl_ifIndex_t ifindex, hsl_fib_id_t fib_id);
#endif /* HAVE_L3 */
int hsl_ifmgr_get_additional_L3_port (struct hsl_if *ifp, hsl_vid_t vid);
struct hsl_if *hsl_ifmgr_get_L2_parent (struct hsl_if *ifp);
int hsl_ifmgr_set_autonego (hsl_ifIndex_t ifindex, int autonego, HSL_BOOL send_notification);
int hsl_ifmgr_set_bandwidth (hsl_ifIndex_t ifindex, u_int32_t bandwidth, HSL_BOOL send_notification);
int hsl_ifmgr_set_mdix (hsl_ifIndex_t ifindex, int mdix, HSL_BOOL send_notification);
int hsl_ifmgr_set_portbased_vlan (hsl_ifIndex_t ifindex, 
                      struct hal_port_map pbitmap, HSL_BOOL send_notification);
int hsl_ifmgr_cpu_default_vlan (hsl_ifIndex_t ifindex, int vid,
                                                HSL_BOOL send_notification);
int hsl_ifmgr_wayside_default_vlan (hsl_ifIndex_t ifindex, int vid,
                                                HSL_BOOL send_notification);
int hsl_ifmgr_set_port_egress (hsl_ifIndex_t ifindex, int egress, 
                                                   HSL_BOOL send_notification);

int hsl_ifmgr_set_ethertype (hsl_ifIndex_t ifindex, u_int16_t etype, 
                                                   HSL_BOOL send_notification);

int hsl_ifmgr_set_force_vlan (hsl_ifIndex_t ifindex, int vid, 
                                                   HSL_BOOL send_notification);
 
int hsl_ifmgr_set_learn_disable (hsl_ifIndex_t ifindex, int enable, 
                                                   HSL_BOOL send_notification);

int hsl_ifmgr_get_learn_disable (hsl_ifIndex_t ifindex, int* enable); 

int hsl_ifmgr_set_sw_reset (void);

int hsl_ifmgr_set_preserve_ce_cos (hsl_ifIndex_t ifindex, 
                                   HSL_BOOL send_notification);

void hsl_ifmgr_collect_if_stat(void);
int  hsl_ifmgr_compute_efm_counters (struct hsl_if *ifp);
int  compute_frame_errors(struct hsl_if *ifp, u_int32_t *total_errors);
int hsl_ifmgr_init_portmirror (void);
int hsl_ifmgr_deinit_portmirror (void);
int hsl_ifmgr_set_portmirror (hsl_ifIndex_t to_ifindex, hsl_ifIndex_t from_ifindex, enum hal_port_mirror_direction direction);
int hsl_ifmgr_unset_portmirror (hsl_ifIndex_t to_ifindex, hsl_ifIndex_t from_ifindex, enum hal_port_mirror_direction direction);
int hsl_ifmgr_create_interface(struct hsl_if *ifp_params, struct hsl_if **new_ifp, HSL_BOOL send_notification, HSL_BOOL allocated_params);
int hsl_ifmgr_delete_interface(struct hsl_if *ifp, HSL_BOOL send_notification);
#ifdef HAVE_IPV6
int hsl_ifmgr_ipv6_address_add (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix, u_char flags);
int hsl_ifmgr_ipv6_address_delete (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix);
#endif /* HAVE_IPV6 */
#ifdef HAVE_LACPD
int hsl_ifmgr_aggregator_add (char *agg_name, u_char agg_mac[], int agg_type);
int hsl_ifmgr_aggregator_del (char *agg_name, u_int32_t agg_ifindex);
int hsl_ifmgr_lacp_psc_set (u_int32_t agg_ifindex ,int psc);
int hsl_ifmgr_aggregator_port_attach (char *agg_name, u_int32_t agg_ifindex,
                                u_int32_t port_ifindex);
int hsl_ifmgr_aggregator_port_detach (char *agg_name, u_int32_t agg_ifindex,
                                  u_int32_t port_ifindex);
int  hsl_ifmgr_if_clear_counters(hsl_ifIndex_t ifindex);
#endif /* HAVE_LACPD */
#ifdef HAVE_MPLS
int hsl_ifmgr_mpls_create (u_char *, int, void *, struct hsl_if **);
void hsl_ifmgr_mpls_delete (struct hsl_if *);
#endif /* HAVE_MPLS */


#endif /* _HSL_IFMGR_H_ */
