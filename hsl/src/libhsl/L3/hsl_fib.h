/* Copyright (C) 2004   All Rights Reserved. */

#ifndef __HSL_FIB_H
#define __HSL_FIB_H 
#include "hsl_ether.h"

#define MAX_PREFIX_LIST_LEN       (16384) /* 16K */ 

/* HSL Fib type */
typedef u_int32_t hsl_fib_id_t;

/* Maximum number of FIB tables */
#define HSL_MAX_FIB               (256)
#define HSL_DEFAULT_FIB           (0)
#define HSL_INVALID_FIB_ID        (hsl_fib_id_t)(-1)

/* 
   Nexthop type. 
*/
enum hsl_ipuc_nexthop_type
  {
    HSL_IPUC_UNSPEC,         /* Placeholder for 'unspecified' */
    HSL_IPUC_LOCAL,          /* Nexthop is directly attached. */
    HSL_IPUC_REMOTE,         /* Nexthop is remote. */
    HSL_IPUC_SEND_TO_CP,     /* Send to Control plane. */
    HSL_IPUC_BLACKHOLE,      /* Drop. */
    HSL_IPUC_PROHIBIT        /* Administratively probihited. */
  };

/*
  ARP timer resolution value in seconds.
*/
#define HSL_DEFAULT_ARP_TIMER_RESOLUTION  120

/*
  Maximum ARP timeout. 
*/
#define HSL_DEFAULT_ARP_TIMEOUT           3000

/* 
   Interface ARP alive counter.
*/
#define HSL_ARP_ALIVE_COUNTER(T)       ((T)/HSL_DEFAULT_ARP_TIMER_RESOLUTION)

/*
  Nexthop entry.
*/
struct hsl_nh_entry
{ 
  struct hsl_if *ifp;                 /* Interface. */
  struct hsl_if *l2_ifp;              /* Layer2 interface */
  
  u_char mac[HSL_ETHER_ALEN];         /* Ethernet address. */
  u_char flags;                       /* Flags. */
#define HSL_NH_ENTRY_VALID               (1 << 0)
#define HSL_NH_ENTRY_STATIC              (1 << 1)
#define HSL_NH_ENTRY_DEL_IN_PROGRESS     (1 << 2)
#define HSL_NH_ENTRY_DEPENDENT           (1 << 3)
#define HSL_NH_ENTRY_PROXY               (1 << 4)
#define HSL_NH_ENTRY_BLACKHOLE           (1 << 5)

  u_char ext_flags;                       /* Flags. */
#define HSL_NH_ENTRY_EFLAG_IN_HW         (1 << 0)

#define HSL_NH_TYPE_IP                0
#define HSL_NH_TYPE_IPV6              1
#define HSL_NH_TYPE_MPLS              2
  u_char nh_type;

  void *system_info;                  /* Hardware specific info for this nexthop. */

  u_int32_t refcnt;                   /* Number of routes, pointing to this nexthop */
  struct hsl_avl_tree *prefix_tree;   /* Tree of prefix pointers dependent on this NH. */
#ifdef HAVE_MPLS
  struct hsl_mpls_ilm_entry *ilm_list; /* List of ILM entries dependent on this NH */

#ifdef HAVE_MPLS_VC
  struct hsl_mpls_vpn_vc *vpn_vc_list;
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_MPLS */

  u_int32_t aliveCounter;             /* Liveliness counter. */

  struct hsl_route_node *rn;        /* Pointer to parent tree node */
  struct hsl_nh_entry *next;
};

/*
  Nexthop entry list.
*/
struct hsl_nh_entry_list_node
{
  struct hsl_nh_entry *entry;
  struct hsl_nh_entry_list_node *next;
};

/*
  ifp list in case of interface routes.
  for eg: ip route A.B.C.D/M INTERFACE kinda routes.
*/
struct hsl_nh_if_list_node
{
  struct hsl_if *ifp;
  struct hsl_nh_if_list_node *next;
};

/*
  Prefix entry.
*/
struct hsl_prefix_entry
{
  u_char flags;                      /* Flags. */
#define HSL_PREFIX_ENTRY_IN_HW          (1 << 0)
#define HSL_PREFIX_ENTRY_EXCEPTION   (1 << 1)

  void *system_info;                 /* System specific info for this prefix. */

  u_char nhcount;                    /* Ref count, really is the total nexthops. */
  struct hsl_nh_entry_list_node *nhlist;    /* List of NHs. */

  u_char ifcount;                    /* Ref count, for interface routes. */
  struct hsl_nh_if_list_node *iflist;       /* List of ifps as NH. */
};

/*
  Some helper macros for prefix flags. 
*/
#define HSL_PREFIX_ENTRY_POPULATED_IN_HW(rnp)         (((struct hsl_prefix_entry *)(rnp)->info)->flags & HSL_PREFIX_ENTRY_IN_HW)
#define HSL_PREFIX_ENTRY_POPULATED_AS_EXCEPTION(rnp)  (((struct hsl_prefix_entry *)(rnp)->info)->flags & HSL_PREFIX_ENTRY_EXCEPTION)
#define HSL_PREFIX_ENTRY_IS_REAL(rnp)    (HSL_PREFIX_ENTRY_POPULATED_IN_HW((rnp)) && ! HSL_PREFIX_ENTRY_POPULATED_AS_EXCEPTION((rnp)))
#define HSL_PREFIX_ENTRY_IS_EXCEPTION(rnp)   (HSL_PREFIX_ENTRY_POPULATED_IN_HW((rnp)) && HSL_PREFIX_ENTRY_POPULATED_AS_EXCEPTION((rnp)))

/*
  FIB table.
*/
struct hsl_fib_table
{
  struct hsl_route_table *nh[HSL_MAX_FIB];               /* Nexthop table. */

  struct hsl_route_table *prefix[HSL_MAX_FIB];           /* Prefix table. */

#ifdef HAVE_IPV6
  struct hsl_route_table *nh6[HSL_MAX_FIB];             /* IPV6 Nexthop table. */

  struct hsl_route_table *prefix6[HSL_MAX_FIB];         /* IPV6 Prefix table. */
#endif /* HAVE_IPV6 */

  /* Protection semaphore */
  apn_sem_id mutex;

  struct hsl_fib_os_callbacks *os_cb;       /* Callbacks for OS. */
  struct hsl_fib_hw_callbacks *hw_cb;       /* Callbacks for HW. */
};

#define HSL_FIB_LOCK oss_sem_lock (OSS_SEM_MUTEX, p_hsl_fib_table->mutex, OSS_WAIT_FOREVER)
#define HSL_FIB_UNLOCK oss_sem_unlock (OSS_SEM_MUTEX, p_hsl_fib_table->mutex)

#define HSL_FIB_TBL_TRAVERSE_START(TBLNAME, TBLPTR, INDEX)                  \
    for ((INDEX) = 0; (INDEX) < HSL_MAX_FIB; (INDEX)++)                     \
      if ((TBLPTR = p_hsl_fib_table->TBLNAME[(INDEX)]) != NULL)             

#define HSL_FIB_TBL_TRAVERSE_END(TBLNAME, TBLPTR, INDEX) 

#define HSL_FIB_ID_VALID(ID)    ((ID) < HSL_MAX_FIB)
#define HSL_FIB_TBL_VALID(TBLNAME, ID) (p_hsl_fib_table->TBLNAME[(ID)] != NULL)

#define HSL_FIB_VALID(ID) HSL_FIB_TBL_VALID(prefix, (ID))

#define HSL_FIB_TBL(TBLNAME, ID)   p_hsl_fib_table->TBLNAME[(ID)]

#define HSL_PREFIX_DELETE_ELIGIBLE(PE)                                                            \
      (((PE)->nhcount == 0) && ((PE)->ifcount == 0))

#define HSL_NH_DELETE_ELIGIBLE(NH,S)                                                        \
        ((nh->refcnt == 0) &&                                            \
         (! CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID) ||                \
            CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))              \
        && ((!CHECK_FLAG (nh->flags, HSL_NH_ENTRY_STATIC)) || (S)))

#define HSL_NH_RN_ATTACHED(NH)                   \
        do {                                  \
         if (!nh) return -1;                  \
         if (!nh->rn) return -1;              \
        } while(0); 

#define HSL_ARP_FLAG_STATIC    (1 << 0)
#define HSL_ARP_FLAG_DYNAMIC   (1 << 1)

#define HSL_FIB_HWCB_CHECK(FN)           (p_hsl_fib_table && p_hsl_fib_table->hw_cb && p_hsl_fib_table->hw_cb->FN)
#define HSL_FIB_HWCB_CALL(FN)            (*p_hsl_fib_table->hw_cb->FN)

#define HSL_FIB_OSCB_CHECK(FN)           (p_hsl_fib_table && p_hsl_fib_table->os_cb && p_hsl_fib_table->os_cb->FN)
#define HSL_FIB_OSCB_CALL(FN)            (*p_hsl_fib_table->os_cb->FN)

extern struct hsl_fib_table *p_hsl_fib_table;
/* 
   Function prototypes.
*/
struct hal_arp_cache_entry;
struct hal_ipv6_nbr_cache_entry;
int hsl_fib_init (void);
int hsl_fib_deinit (void);
int hsl_fibmgr_hw_cb_register (struct hsl_fib_hw_callbacks *cb);
int hsl_fibmgr_hw_cb_unregister (void);
int hsl_fibmgr_os_cb_register (struct hsl_fib_os_callbacks *cb);
int hsl_fibmgr_os_cb_unregister (void);
int hsl_fib_create_tables (hsl_fib_id_t fib_id);
int hsl_fib_destroy_tables (hsl_fib_id_t fib_id);
struct hsl_route_node *
hsl_fib_prefix_node_get (struct hsl_route_table *prefix_table, hsl_prefix_t *prefix);
struct hsl_route_node *
hsl_fib_prefix_node_lookup (struct hsl_route_table *prefix_table, hsl_prefix_t *prefix);
int hsl_fib_link_nh_and_prefix (struct hsl_route_node *rnp, struct hsl_nh_entry *nh);
int hsl_fib_unlink_nh_and_prefix (struct hsl_route_node *rnp, struct hsl_nh_entry *nh);
int hsl_fib_nh_add (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, struct hsl_if *ifp, char *mac, u_char flags, u_char nh_type);
int hsl_fib_nh_delete (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, u_int32_t ifindex, int delete_static, u_char nh_type);
int hsl_fib_add_ipv4uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex, hsl_ipv4Address_t *addr);
int hsl_fib_delete_ipv4uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex, hsl_ipv4Address_t *addr);
int hsl_fib_if_notifier_cb (int cmd, void *param1, void *param2);
int hsl_fib_handle_arp (struct hsl_if *ifp, struct hsl_eth_header *eth, struct hsl_arp *arp);
int hsl_fib_handle_exception (hsl_fib_id_t fib_id, unsigned char *hdr, int ether_type);
int hsl_fib_handle_l2_movement (hsl_mac_address_t src_mac, struct hsl_if *ifp);
void hsl_fib_process_nh_ageing (void);
int hsl_fib_reprocess_prefix (hsl_fib_id_t fib_id, struct hsl_route_node *rnp);
int hsl_fib_add_ipv4uc_connected (hsl_prefix_t *prefix, struct hsl_if *ifp);
int hsl_fib_delete_ipv4uc_connected (hsl_prefix_t *prefix, struct hsl_if *ifp);
int hsl_fib_arp_entry_add (hsl_ipv4Address_t *ip_addr, unsigned int ifindex, unsigned char *mac_addr, unsigned char flags);
int hsl_fib_arp_entry_del (hsl_ipv4Address_t *ip_addr, unsigned int ifindex);
int hsl_fib_arp_del_all (hsl_fib_id_t fib_id, unsigned char clr_flag);
int hsl_fib_add_connected (hsl_prefix_t *prefix, struct hsl_if *ifp);
int hsl_fib_delete_connected (hsl_prefix_t *prefix, struct hsl_if *ifp, HSL_BOOL os_cb);
int hsl_fib_nh_get_bundle (hsl_fib_id_t fib_id, hsl_ipv4Address_t *ip_addr, int count, struct hal_arp_cache_entry *cache);
int hsl_fib_get_max_num_multipath(u_int32_t *ecmp);
struct hsl_nh_entry *hsl_fib_nh_get (hsl_fib_id_t fib_id, hsl_ifIndex_t, hsl_prefix_t *, u_char);
struct hsl_nh_entry *hsl_fib_nh_lookup (hsl_fib_id_t fib_id, hsl_ifIndex_t, hsl_prefix_t *, u_char);
void hsl_fib_nh_release (struct hsl_nh_entry *nh);
#ifdef HAVE_IPV6
int hsl_fib_handle_neigbor_discovery(struct hsl_if *ifp, char *l3_hdr_ptr, int *pkt_type);
int hsl_fib_add_ipv6uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex, hsl_ipv6Address_t *addr);
int hsl_fib_delete_ipv6uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex, hsl_ipv6Address_t *addr);
int hsl_fib_ipv6_nbr_add (hsl_ipv6Address_t *addr, unsigned int ifindex, unsigned char *mac_addr, unsigned char flags);
int hsl_fib_ipv6_nbr_del (hsl_ipv6Address_t *addr, int delete_static, unsigned int ifindex);
int hsl_fib_ipv6_nbr_del_all (hsl_fib_id_t fib_id, unsigned char clr_flag);
int hsl_fib_nh6_get_bundle (hsl_fib_id_t fib_id, hsl_ipv6Address_t *addr, int count, struct hal_ipv6_nbr_cache_entry *cache);
#endif /* HAVE_IPV6 */

#ifdef HAVE_L3
  void hsl_fib_handle_l2_port_state_change(struct hsl_if *ifp);
#endif /* HAVE_L3 */

#ifdef HAVE_MPLS
int hsl_fib_add_mpls_ftn (hsl_prefix_t *, struct hsl_nh_entry *);
/* 
   Delete ftn entry 
*/
int hsl_fib_delete_mpls_ftn (hsl_prefix_t *prefix, struct hsl_nh_entry *nh);
#endif /* HAVE_MPLS */
#endif /* __HSL_FIB_H  */
