/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_RIB_H
#define _NSM_RIB_H

#include "pal.h"

#include "lib/vrep.h"
#include "nsm_table.h"
#include "ptree.h"

#define DISTANCE_INFINITY  255
#define STALE_PRESERVE_ON     1

#define MAIN_TABLE_INDEX   0

/*Prototype*/

struct nsm_master;

#ifdef HAVE_VRF
struct nsm_ospf_domain_conf
{
  /*Domain ID */
  u_char domain_id[8];
 /*OSPF area ID */
  struct pal_in4_addr area_id;
 /*OSPF router ID */
  struct pal_in4_addr router_id;
 /*OSPF route type*/
  u_char r_type;
 /* Metric Type */
  u_char metric_type_E2;
};
#endif /* HAVE_VRF */

/* Routing information base. */
struct rib
{
  /* Link list. */
  struct rib *next;
  struct rib *prev;

  /* Type of this route. */
  u_char type;

  /* Sub type of this route.  */
  u_char sub_type;

  /* Distance. */
  u_char distance;

  /* Flags of this route.  */
  u_char flags;

  /* Metric */
  u_int32_t metric;

  /* Uptime. */
  pal_time_t uptime;

  /* Extended flags of this route */
  u_char ext_flags;
#define RIB_EXT_FLAG_MROUTE                 0x01
#ifdef HAVE_HA
#define RIB_EXT_FLAG_HA_RIB_CHANGED         0x02
#define RIB_EXT_FLAG_HA_RIB_DELETED         0x04
#endif /* HAVE_HA */
#define RIB_EXT_FLAG_BLACKHOLE_RECURSIVE    0x08

  /* Client ID.  NSM protocol provide four octet client ID.  But to
     reduce memory consumption in RIB, this client_id is defined as
     one octet.  You can extend this restriction by changing this
     definition.  */
  /* XXX: Client_id is local to a system and therefore cannot be 
   * checkpointed. But it is used for Graceful Restart mechanism to 
   * mark the routes STALE based on client id. 
   * Therefore, for HA the client id will be the protocol id. This will
   * be ensured by assigning the client_id as the protocol_id at time
   * of NSM client connect (in nsm_server_recv_service() ).
   */ 
  u_char client_id;

  /* Nexthop information. */
  u_char nexthop_num;
  u_char nexthop_active_num;
  struct nexthop *nexthop;

#ifdef HAVE_RMM
  /* RMM module flag.  */
  u_char rmm_flags;
#endif /* HAVE_RMM */

  /* VRF pointer.  */
  struct nsm_vrf *vrf;

#ifdef HAVE_STAGGER_KERNEL_MSGS
  /* Kernel Msg Stagger Link-List node pointer. */
  struct listnode *kernel_ms_lnode;
#endif /* HAVE_STAGGER_KERNEL_MSGS */

#ifdef HAVE_HA
  HA_CDR_REF nsm_rib_cdr_ref;
#endif /* HAVE_HA */
 
 /*Process ID */
 u_int32_t pid;

 /* Tag */
 u_int32_t tag;

 /* inform nexthop change */
 u_int32_t pflags;
#define NSM_ROUTE_CHG_INFORM_BGP (1 << 0)
#ifdef HAVE_BFD
#define NSM_ROUTE_CHG_BFD (1 << 1)
#define NSM_BFD_CONFIG_CHG (1 << 2)
#endif /* HAVE_BFD */
#ifdef HAVE_MPLS
#define NSM_ROUTE_HAVE_IGP_SHORTCUT (1 << 3)
#endif/* HAVE_MPLS */
#ifdef HAVE_VRF
 /*OSPF Domain info */
 struct nsm_ospf_domain_conf *domain_conf;
#endif /*HAVE_VRF*/
};


/* NSM nexthop information. */
union nsm_nexthop
{
  /* IPv4 nexthop address. */
  struct pal_in4_addr ipv4;
#ifdef HAVE_IPV6
  /* IPv6 nexthop address. */
  struct pal_in6_addr ipv6;
#endif /* HAVE_IPV6 */
};

/* Static route configuration. */
struct nsm_static
{
  /* Linked list. */
  struct nsm_static *prev;
  struct nsm_static *next;

  /* Address family. */
  afi_t afi;

  /* Administrative distance. */
  u_char distance;

  /* Flags for this static route's type -- same as nexthop type. */
  u_char type;

  /* Next-hop SNMP route type - value is 1 to 4  */
  u_int8_t snmp_route_type;  

  /* Nexthop information. */
  union nsm_nexthop gate;

  /* Nexthop interface name. */
  char *ifname;

  u_char flags;
#ifdef HAVE_BFD
#define NSM_STATIC_FLAG_BFD_SET                (1 << 0)
#define NSM_STATIC_FLAG_BFD_DISABLE_SET        (1 << 1)
#endif /* HAVE_BFD */

#ifdef HAVE_SNMP
  /* metric */
  int metric;
#endif /* HAVE_SNMP */

#ifdef HAVE_HA
  HA_CDR_REF  nsm_static_cdr_ref;
#endif /* HAVE_HA */

  /* Tag for staic route */
  u_int32_t tag;
 
  /* Description of the static route */
  char *desc;
};

#define NSM_IPV4_CMP(A,B)                                                     \
    (pal_ntoh32 ((A)->s_addr) < pal_ntoh32 ((B)->s_addr) ? -1 :               \
     (pal_ntoh32 ((A)->s_addr) > pal_ntoh32 ((B)->s_addr) ? 1 : 0))

#define NSM_IPV6_CMP(A,B)                                                     \
  (*((u_char *)(A) + 0) < *((u_char *)(B) + 0) ? -1 :                         \
  (*((u_char *)(A) + 0) > *((u_char *)(B) + 0) ?  1 :                         \
  (*((u_char *)(A) + 1) < *((u_char *)(B) + 1) ? -1 :                         \
  (*((u_char *)(A) + 1) > *((u_char *)(B) + 1) ?  1 :                         \
  (*((u_char *)(A) + 2) < *((u_char *)(B) + 2) ? -1 :                         \
  (*((u_char *)(A) + 2) > *((u_char *)(B) + 2) ?  1 :                         \
  (*((u_char *)(A) + 3) < *((u_char *)(B) + 3) ? -1 :                         \
  (*((u_char *)(A) + 3) > *((u_char *)(B) + 3) ?  1 :                         \
  (*((u_char *)(A) + 4) < *((u_char *)(B) + 4) ? -1 :                         \
  (*((u_char *)(A) + 4) > *((u_char *)(B) + 4) ?  1 :                         \
  (*((u_char *)(A) + 5) < *((u_char *)(B) + 5) ? -1 :                         \
  (*((u_char *)(A) + 5) > *((u_char *)(B) + 5) ?  1 :                         \
  (*((u_char *)(A) + 6) < *((u_char *)(B) + 6) ? -1 :                         \
  (*((u_char *)(A) + 6) > *((u_char *)(B) + 6) ?  1 :                         \
  (*((u_char *)(A) + 7) < *((u_char *)(B) + 7) ? -1 :                         \
  (*((u_char *)(A) + 7) > *((u_char *)(B) + 7) ?  1 :                         \
  (*((u_char *)(A) + 8) < *((u_char *)(B) + 8) ? -1 :                         \
  (*((u_char *)(A) + 8) > *((u_char *)(B) + 8) ?  1 :                         \
  (*((u_char *)(A) + 9) < *((u_char *)(B) + 9) ? -1 :                         \
  (*((u_char *)(A) + 9) > *((u_char *)(B) + 9) ?  1 :                         \
  (*((u_char *)(A) + 10) < *((u_char *)(B) + 10) ? -1 :                       \
  (*((u_char *)(A) + 10) > *((u_char *)(B) + 10) ?  1 :                       \
  (*((u_char *)(A) + 11) < *((u_char *)(B) + 11) ? -1 :                       \
  (*((u_char *)(A) + 11) > *((u_char *)(B) + 11) ?  1 :                       \
  (*((u_char *)(A) + 12) < *((u_char *)(B) + 12) ? -1 :                       \
  (*((u_char *)(A) + 12) > *((u_char *)(B) + 12) ?  1 :                       \
  (*((u_char *)(A) + 13) < *((u_char *)(B) + 13) ? -1 :                       \
  (*((u_char *)(A) + 13) > *((u_char *)(B) + 13) ?  1 :                       \
  (*((u_char *)(A) + 14) < *((u_char *)(B) + 14) ? -1 :                       \
  (*((u_char *)(A) + 14) > *((u_char *)(B) + 14) ?  1 :                       \
  (*((u_char *)(A) + 15) < *((u_char *)(B) + 15) ? -1 :                       \
  (*((u_char *)(A) + 15) > *((u_char *)(B) + 15) ?  1 :                       \
   0))))))))))))))))))))))))))))))))

#define NSM_VRF_DEFAULT(V)      (nsm_vrf_lookup_by_id (0))

#define RIB_IPV4_UNICAST        afi[AFI_IP].rib[SAFI_UNICAST]
#define RIB_IPV6_UNICAST        afi[AFI_IP6].rib[SAFI_UNICAST]
#define IPV4_STATIC             afi[AFI_IP].ip_static
#define IPV6_STATIC             afi[AFI_IP6].ip_static

#define RIB_SYSTEM_ROUTE(R)                                                   \
        ((R)->type == APN_ROUTE_KERNEL)

#define RIB_CONNECT_ROUTE(R)							\
	((R)->type == APN_ROUTE_CONNECT)

#define RIB_STALE_SELFROUTE(R)                                                  \
              (((R)->type == APN_ROUTE_KERNEL)                                  \
               && (R)->distance == DISTANCE_INFINITY                            \
               && CHECK_FLAG ((R)->flags, RIB_FLAG_SELFROUTE)                   \
               && CHECK_FLAG ((R)->flags, RIB_FLAG_STALE)                       \
              )

#define NSM_RIB_KERNEL_SYNC_INTERVAL    600
  
/* Prototypes. */
struct interface;

void nsm_nexthop_add (struct rib *, struct nexthop *);
void nsm_nexthop_free (struct nexthop *nexthop);

struct nexthop *nsm_nexthop_ifindex_add (struct rib *, u_int32_t, int);
struct nexthop *nsm_nexthop_ifname_add (struct rib *, char *, int);
struct nexthop *nsm_nexthop_ipv4_add (struct rib *,
                                      struct pal_in4_addr *, int);
struct nexthop *nsm_nexthop_ipv4_ifindex_add (struct rib *,
                                              struct pal_in4_addr *,
                                              u_int32_t, int);
struct nexthop *nsm_nexthop_ipv4_ifname_add (struct rib *,
                                             struct pal_in4_addr *, 
                                             char *, int);
#ifdef HAVE_VRF
struct nexthop *nsm_nexthop_mpls_add (struct rib *, struct pal_in4_addr *,
                                      s_int32_t);
#endif /* HAVE_VRF */

#ifdef HAVE_IPV6
struct nexthop *nsm_nexthop_ipv6_add (struct rib *, struct pal_in6_addr *);
struct nexthop *nsm_nexthop_ipv6_ifindex_add (struct rib *,
                                              struct pal_in6_addr *,u_int32_t);
struct nexthop *nsm_nexthop_ipv6_ifname_add (struct rib *,
                                             struct pal_in6_addr *, 
                                             char *);
#endif /* HAVE_IPV6 */
int nsm_rib_nexthop_connected_check (struct rib *rib);
int nsm_rib_nexthop_fib_check (struct rib *fib);

int nsm_rib_table_free (struct nsm_ptree_table *);
void nsm_static_table_clean_all (struct nsm_vrf *, struct ptree *);
struct rib *nsm_rib_match (struct nsm_vrf *, afi_t, struct prefix *,
                           struct nsm_ptree_node **, u_int32_t, u_char);
struct rib *nsm_rib_lookup (struct nsm_vrf *, afi_t,
                            struct prefix *, struct nsm_ptree_node **, 
                            u_int32_t, u_char);
struct rib *nsm_rib_lookup_by_type (struct nsm_vrf *, afi_t,
                                    struct prefix *, int);

#ifdef HAVE_STAGGER_KERNEL_MSGS
void nsm_kernel_msg_stagger_timer_start (struct nsm_master *,
                                         struct pal_timeval *);
void nsm_kernel_msg_stagger_clean_all (struct nsm_master *);
#endif /* HAVE_STAGGER_KERNEL_MSGS */

result_t nsm_rib_uninstall_kernel (struct nsm_ptree_node *, struct rib *);
int nsm_rib_install_kernel (struct nsm_master *nm,
                        struct nsm_ptree_node *rn, struct rib *rib);

void nsm_rib_uninstall (struct nsm_ptree_node *, struct rib *);
void nsm_rib_addnode (struct nsm_ptree_node *, struct rib *);
void nsm_rib_delnode (struct nsm_ptree_node *, struct rib *);
void nsm_rib_free (struct rib *);
void nsm_rib_process (struct nsm_master *, struct nsm_ptree_node *,
                      struct rib *);

/* RIB functions.  */
void nsm_rib_add (struct prefix *, struct rib *, afi_t, safi_t);
void nsm_rib_delete (struct prefix *, u_char, struct rib *, struct rib *,
                     struct nsm_ptree_node *, struct nsm_master *);
void nsm_rib_delete_by_ifindex (struct prefix *, u_char, int, afi_t, safi_t,
                                struct nsm_vrf *);
void nsm_rib_delete_by_type (struct prefix *, u_char, int, afi_t, safi_t,
                             struct nsm_vrf *);
void nsm_rib_delete_by_nexthop (struct prefix *, u_char, int,
                                union nsm_nexthop *, afi_t, safi_t,
                                struct nsm_vrf *);

int nsm_rib_sweep_stale (struct nsm_master *, afi_t);
int nsm_rib_sweep_timer (struct thread *);
int nsm_rib_sweep_timer_update (struct nsm_master *);

void nsm_rib_close (struct nsm_master *);
int nsm_static_ipv4_count (struct nsm_vrf *);

u_int32_t nsm_restart_time (int);
void nsm_restart_register (int, u_int32_t);
void nsm_restart_stale_remove (struct nsm_master *, int, afi_t, safi_t);
#ifdef HAVE_VRF
void nsm_restart_stale_remove_vrf (struct nsm_vrf *, afi_t, int);
void nsm_restart_stale_mark_vrf (struct nsm_vrf *, afi_t, int);
#endif /* HAVE_VRF */
void nsm_restart_stale_mark (struct nsm_master *, int, afi_t, safi_t);
void nsm_restart_option_set (int, u_int16_t, u_char *);
void nsm_restart_option_unset (int);
u_char nsm_restart_state (int);
void nsm_restart_stop (int);
void nsm_restart_state_set (int);
void nsm_restart_state_unset (int);
s_int32_t nsm_restart_time_expire (struct thread *);
void nsm_api_show_forwarding_time (struct vrep_table *, int);
void nsm_rib_clean_client (u_int32_t, int);

struct rib *nsm_rib_new (u_char, u_char, u_char, u_int32_t, struct nsm_vrf *);

/* These four functions are only used for Kernel interface. */
void nsm_rib_add_ipv4 (u_char, struct prefix_ipv4 *, u_int32_t,
                       struct pal_in4_addr *, u_char, u_int32_t, fib_id_t);
void nsm_rib_delete_ipv4 (u_char, struct prefix_ipv4 *, int,
                          struct pal_in4_addr *, fib_id_t);
void nsm_rib_add_connected_ipv4 (struct interface *, struct prefix *);
void nsm_rib_delete_connected_ipv4 (struct interface *, struct prefix *);
#ifdef HAVE_IPV6
void nsm_rib_add_ipv6 (u_char, struct prefix_ipv6 *, u_int32_t,
                       struct pal_in6_addr *, u_char, u_int32_t, fib_id_t);
void nsm_rib_delete_ipv6 (u_char, struct prefix_ipv6 *, int,
                          struct pal_in6_addr *, fib_id_t);
void nsm_rib_add_connected_ipv6 (struct interface *, struct prefix *);
void nsm_rib_delete_connected_ipv6 (struct interface *, struct prefix *);
#endif /* HAVE_IPV6 */

void nsm_rib_multipath_process (struct nsm_master *);

struct nsm_static *nsm_static_get (struct nsm_vrf *, afi_t, struct prefix *,
                                   union nsm_nexthop *, char *, u_char,
                                   int, int,
                                   u_int32_t, char *);
int nsm_static_add (struct nsm_vrf *, afi_t, struct prefix *,
                    union nsm_nexthop *, char *, u_char, int, int,
                    u_int32_t, char *);
#ifdef HAVE_BFD
struct nsm_static *nsm_static_lookup (struct nsm_vrf *, afi_t, struct prefix *,
                                      union nsm_nexthop *);
struct rib *
nsm_rib_static_lookup (struct rib *top, struct nsm_static *ns);
#endif /* HAVE_BFD */
int nsm_static_delete (struct nsm_vrf *, afi_t, struct prefix *,
                       union nsm_nexthop *, char *, u_char);
int nsm_static_delete_all (struct nsm_vrf *, afi_t, struct prefix *);
result_t nsm_static_delete_by_if (struct nsm_vrf *, afi_t, struct interface *);
struct nsm_static *nsm_static_new (afi_t afi, u_char type, char *ifname);
void nsm_static_free (struct nsm_static *ns);
void nsm_static_nexthop_add_sort (struct ptree_node *rn,
                                  struct nsm_static *ns);
int nsm_static_nexthop_same (struct nsm_static *ns, u_char type,
                         union nsm_nexthop *gate, char *ifname);

result_t nsm_rib_update_vrf (struct nsm_vrf *, afi_t);

int nsm_rib_clean (struct nsm_master *nm, afi_t, safi_t, u_int32_t, vrf_id_t);
void nsm_rib_update_timer_add (struct nsm_vrf *, afi_t);
int nsm_rib_kernel_sync_timer (struct thread *);

int nsm_rib_update_ipv4_timer (struct thread *thread);
void nsm_rib_ipaddr_chng_ipv4_fib_update (struct nsm_master *nm,
                                          struct nsm_vrf *vrf,
                                          struct connected *ifc);
#ifdef HAVE_IPV6
int nsm_rib_update_ipv6_timer (struct thread *thread);
#endif /* HAVE_IPV6 */
#endif /* _NSM_RIB_H */
