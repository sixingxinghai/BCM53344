/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _NSM_INTERFACE_H
#define _NSM_INTERFACE_H

#include "nsm_vrf.h"
#ifdef HAVE_L2
#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_bridge.h"
#ifdef HAVE_L2LERN
#include "nsm_acl.h"
#endif /* HAVE_L2LERN */
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#ifdef HAVE_GVRP
#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#include "gvrp.h"
#endif /* HAVE_GVRP */
#endif /* HAVE_VLAN */
#ifdef HAVE_GMRP
#ifndef HAVE_GVRP
#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#endif /* NOT HAVE_GRVP */
#include "gmrp.h"
#endif /* HAVE_GMRP */
#endif /* HAVE_L2 */
#ifdef HAVE_IGMP
#include "igmp.h"
#endif /* HAVE_IGMP */
#ifdef HAVE_SNMP
#include "nsm_snmp.h"
#endif

#include "nsm_ifma.h"

/* NSM interface types. */
#define NSM_IF_TYPE_L3        0
#define NSM_IF_TYPE_L2        1

/* Maximum number of secondary adresses */
#define NSM_INTERFACE_ADDRESS_MAX  4294967294UL

/* For interface add/delete */
#define NSM_IF_DELETE          0
#define NSM_IF_ADD             1

/* For interface address add/delete */
#define NSM_IF_ADDRESS_DELETE  0
#define NSM_IF_ADDRESS_ADD     1

/* For interface state UP/DOWN */
#define NSM_IF_DOWN            0
#define NSM_IF_UP              1

#define NSM_DEFAULT_VID        0

#define NSM_IF_BIND_CHECK(I,F)  (CHECK_FLAG ((I)->bind, NSM_IF_BIND_ ## F))

#define NSM_IF_ADD_IFC_IPV4(I,C)                                              \
    do {                                                                      \
      if_add_ifc_ipv4 ((I), (C));                                             \
      nsm_ipv4_connected_table_set ((I)->vrf, (C));                           \
    } while (0)

#define NSM_IF_DELETE_IFC_IPV4(I,C)                                           \
    do {                                                                      \
      if_delete_ifc_ipv4 ((I), (C));                                          \
      nsm_ipv4_connected_table_unset ((I)->vrf, (C));                         \
    } while (0)

#define NSM_IF_ADD_IFC_IPV6(I,C)                                              \
    do {                                                                      \
      if_add_ifc_ipv6 ((I), (C));                                             \
      nsm_ipv6_connected_table_set ((I)->vrf, (C));                           \
    } while (0)

#define NSM_IF_DELETE_IFC_IPV6(I,C)                                           \
    do {                                                                      \
      if_delete_ifc_ipv6 ((I), (C));                                          \
      nsm_ipv6_connected_table_unset ((I)->vrf, (C));                         \
    } while (0)

/* Assignable values to bandwidth */
#define BANDWIDTH_RANGE "<1-999> k|m for 1 to 999 kilo bits or mega bits \
                <1-10> g for 1 to 10 giga bits"


#ifdef HAVE_LACPD
union nsm_if_agg_info
{
  struct list *memberlist;
  struct interface *parent;
};

struct nsm_if_agg
{
  u_char type;
#define NSM_IF_NONAGGREGATED             0
#define NSM_IF_AGGREGATOR                1
#define NSM_IF_AGGREGATED                2

  int psc;  /* Port selection criteria */
  union nsm_if_agg_info info;

  struct list *mismatch_list;

  u_char agg_mac_addr[ETHER_ADDR_LEN];
};

#define AGGREGATOR_MEMBER_LOOP(nif, memifp, ln)                             \
 if (nif &&                                                                 \
     nif->agg.type == NSM_IF_AGGREGATOR &&                                  \
     (listcount(nif->agg.info.memberlist) > 0))                             \
   LIST_LOOP (nif->agg.info.memberlist, memifp, ln)

#define NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(CLI,IFP)                        \
   do {                                                                      \
        struct nsm_if *_zif = (struct nsm_if *)(IFP)->info;                  \
        if (_zif && (_zif->agg_config_type != AGG_CONFIG_NONE) &&            \
            (_zif->agg.type != NSM_IF_AGGREGATOR))                           \
          {                                                                  \
            cli_out((CLI), "%% Cannot configure aggregator member.\n");      \
            return CLI_ERROR;                                                \
          }                                                                  \
   } while (0)
#endif /* HAVE_LACP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
struct nsm_l2_mcast_if
{
  struct interface *ifp;

#ifdef HAVE_IGMP_SNOOP
  struct ptree *igmpsnp_gmr_tib;
#endif

#ifdef HAVE_MLD_SNOOP
  struct ptree *mldsnp_gmr_tib;
#endif
};
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

/* `nsm' daemon local interface structure. */
struct nsm_if
{
  struct interface *ifp;
  u_char type;

#ifdef HAVE_RTADV
  struct rtadv_if *rtadv_if;
#endif /* HAVE_RTADV */

  u_char nsm_if_link_changed;

#ifdef HAVE_VRRP
  VRRP_IF vrrp_if;
#endif /* HAVE_VRRP */

#ifdef HAVE_LACPD
  struct nsm_if_agg agg;

/* A flag to denote the type of Aggregator Association STATIC or LACP*/
  u_char agg_config_type;
  u_char agg_mode;

  /* flag to store oper state of the aggregator member */
  u_char agg_oper_state;

  /* flag to denote that h/w attach for member interface for standby */
  bool_t hw_aggregated;
/* A flag to denote whether the interface is explicitly bridge grouped
   or it is bridge grouped as a result of belonging to an aggregator
 */

  u_char exp_bridge_grpd;

  /* If restoration of channel-group command fails during boot-up,
     the key should br stored for later trials of restoration
  */
  u_int16_t conf_key;

  /* activate channel; required for possible restoration of
     channel-group command
  */
  bool_t conf_chan_activate;

  /* config type; required for possible restoration of
     channel-group command
  */
  u_char conf_agg_config_type;

/* Store the configuration of aggregator interface */

  struct nsm_bridge_port_conf *nsm_bridge_port_conf;

  /* Opcode for addition, deletion of agg.
   * Significant only for members, not for agg */
  u_char opcode;
#define NSM_LACP_AGG_ADD          1
#define NSM_LACP_AGG_DEL          2

#ifdef HAVE_HA
  HA_CDR_REF nsm_if_lacp_cdr_ref;
  HA_CDR_REF nsm_if_lacp_agg_associate_cdr_ref;
#endif /* HAVE_HA */

#endif /* HAVE_LACP */

  u_int16_t vid;

#ifdef HAVE_L2

#ifdef HAVE_VLAN

#define NSM_VLAN_PORT_BASED_VLAN_ENABLE         (1 << 2)
#define NSM_VLAN_DOT1Q_ENABLE                   (1 << 1)
#define NSM_VLAN_DOT1Q_DISABLE                  (1 << 0)
  u_int16_t l2_flags;

  /* Currently only 32 ports are supported for Port Based VLAN */
  u_int32_t port_vlan;
#endif /* HAVE_VLAN */

  struct nsm_bridge *bridge;

  struct nsm_bridge_port *switchport;

#ifdef HAVE_PROVIDER_BRIDGE
  struct nsm_band_width_profile *nsm_bw_profile; ///< Pointer which contains     
                          ///< the BW Profile Parameters related to this UNI
#endif /* HAVE_PROVIDER_BRIDGE */
  struct list *bridge_static_mac_config;

#ifdef HAVE_GVRP
  struct gvrp_port_config *gvrp_port_config;
#endif /* HAVE_GVRP */

#ifdef HAVE_GMRP
  struct gmrp_port_config *gmrp_port_cfg;
#endif /* HAVE_GMRP */


#ifdef HAVE_L2LERN
  struct mac_acl *mac_acl;
#endif /* HAVE_L2LERN */

#ifdef HAVE_VLAN_CLASS
  struct avl_tree *group_tree;
#endif /* HAVE_VLAN_CLASS */
#endif /* HAVE_L2 */

#ifdef HAVE_TE
  struct qos_interface *qos_if;
#endif /* HAVE_TE */

#ifdef HAVE_MCAST_IPV4
  struct nsm_mcast_vif *vif;
  u_char mcast_ttl;
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  struct nsm_mcast6_mif *mif;
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_L3
  u_int32_t flags;
#define NSM_IF_SET_PROXY_ARP             (1 << 0)
#endif

#ifdef HAVE_BFD
#define NSM_IF_BFD                       (1 << 1)
#define NSM_IF_BFD_DISABLE               (1 << 2)
#endif

  char *acl_name_str;
  char *acl_dir_str;

  /* Protocol membership information. */
  modbmap_t member;

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  struct nsm_l2_mcast_if l2mcastif;
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_ONMD
  struct nsm_efm_oam_if *efm_oam_if;
  struct nsm_lldp_oam_if *lldp_oam_if;
#endif /* HAVE_ONMD */

#ifdef HAVE_HA
  HA_CDR_REF nsm_if_cdr_ref;
#endif /* HAVE_HA */

  NSM_IFMA_VEC nsm_ifma_vec;

#ifdef HAVE_GMPLS
  /* Pointer to control channels which bind to this interface */
  struct avl_tree cctree;
#endif /* HAVE_GMPLS */
};

#define NSM_INTF_TYPE(ifp)     (((struct nsm_if *)(ifp)->info)->type)
#define NSM_INTF_TYPE_L2(ifp)  (NSM_INTF_TYPE (ifp) == NSM_IF_TYPE_L2)
#define NSM_INTF_TYPE_L3(ifp)  (NSM_INTF_TYPE (ifp) == NSM_IF_TYPE_L3)

#ifdef HAVE_LACPD
#define NSM_AGG_INTF_TYPE(ifp) ( ifp->info ?                                \
                                 ((struct nsm_if *)(ifp)->info)->agg.type : \
                                 NSM_IF_NONAGGREGATED )
#define NSM_INTF_TYPE_AGGREGATOR(ifp)                                \
  (NSM_AGG_INTF_TYPE (ifp) == NSM_IF_AGGREGATOR)
#define NSM_INTF_TYPE_AGGREGATED(ifp)                                \
  (NSM_AGG_INTF_TYPE (ifp) == NSM_IF_AGGREGATED)
#endif /* HAVE_LACPD */

#define NSM_CHECK_L3_COMMANDS(cli,ifp,err_str)                        \
do                                                                    \
{                                                                     \
  bool_t _ret;                                                        \
  _ret = nsm_check_l3_commands_allowed ((cli), (ifp), (err_str));     \
  if (_ret == PAL_FALSE)                                              \
    return CLI_ERROR;                                                 \
} while (0)

#define NSM_CHECK_L3_GMPLS_COMMANDS(cli,ifp,err_str)                        \
do                                                                          \
{                                                                           \
  bool_t _ret;                                                              \
  _ret = nsm_check_l3_gmpls_commands_allowed ((cli), (ifp), (err_str));     \
  if (_ret == PAL_FALSE)                                                    \
    return CLI_ERROR;                                                       \
} while (0)

#ifdef HAVE_GMPLS
#define NSM_CHECK_LABEL_SWITCH_ON_GMPLS_IFP(cli,ifp)                         \
  do                                                                         \
    {                                                                        \
      if (ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)                    \
        {                                                                    \
          cli_out (cli, "%% Cannot enable/disable label switching on "       \
                        "gmpls interface\n");                                \
          return CLI_ERROR;                                                  \
      }                                                                      \
    } while (0)
#endif /* HAVE_GMPLS */

#define NSM_INTERFACE_SET_L3_MEMBERSHIP(IFP)                               \
   do {                                                                    \
      struct nsm_if *_zif;                                                 \
      _zif = (struct nsm_if *)(IFP)->info;                                 \
      _zif->member = PM_EMPTY;                                             \
      MODBMAP_SET (_zif->member, APN_PROTO_IMI);                           \
      MODBMAP_SET (_zif->member, APN_PROTO_LACP);                          \
      MODBMAP_SET (_zif->member, APN_PROTO_8021X);                         \
      MODBMAP_SET (_zif->member, APN_PROTO_RIP);                           \
      MODBMAP_SET (_zif->member, APN_PROTO_RIPNG);                         \
      MODBMAP_SET (_zif->member, APN_PROTO_OSPF);                          \
      MODBMAP_SET (_zif->member, APN_PROTO_OSPF6);                         \
      MODBMAP_SET (_zif->member, APN_PROTO_BGP);                           \
      MODBMAP_SET (_zif->member, APN_PROTO_LDP);                           \
      MODBMAP_SET (_zif->member, APN_PROTO_RSVP);                          \
      MODBMAP_SET (_zif->member, APN_PROTO_ISIS);                          \
      MODBMAP_SET (_zif->member, APN_PROTO_PIMDM);                         \
      MODBMAP_SET (_zif->member, APN_PROTO_PIMSM);                         \
      MODBMAP_SET (_zif->member, APN_PROTO_DVMRP);                         \
      MODBMAP_SET (_zif->member, APN_PROTO_PIMSM6);                        \
   } while (0)

#define NSM_INTERFACE_SET_L2_MEMBERSHIP(IFP)                               \
   do {                                                                    \
      struct nsm_if *_zif;                                                 \
      _zif = (struct nsm_if *)((IFP)->info);                               \
      _zif->member = PM_EMPTY;                                             \
      MODBMAP_SET (_zif->member, APN_PROTO_IMI);                           \
      MODBMAP_SET (_zif->member, APN_PROTO_LACP);                          \
      MODBMAP_SET (_zif->member, APN_PROTO_LDP);                           \
      MODBMAP_SET (_zif->member, APN_PROTO_8021X);                         \
      MODBMAP_SET (_zif->member, APN_PROTO_STP);                           \
      MODBMAP_SET (_zif->member, APN_PROTO_RSTP);                          \
      MODBMAP_SET (_zif->member, APN_PROTO_MSTP);                          \
      MODBMAP_SET (_zif->member, APN_PROTO_ELMI);                          \
   } while (0)

#define NSM_INTERFACE_CHECK_MEMBERSHIP(IFP, PROTO)                         \
    (MODBMAP_ISSET (((struct nsm_if *)((IFP)->info))->member, PROTO))

#ifdef HAVE_VLAN
#define NSM_MPLS_VALID_VLAN(I, VID)        \
      nsm_is_vlan_exist (I, VID)
#endif /* HAVE_VLAN */

#ifdef HAVE_NSM_IF_PARAMS
struct nsm_if_params
{
  /* Interface name. */
  char *name;

  /* List of pending IP addresses. */
  struct list *pending;

  /* Shutdown flag. */
  int flags;
#define NSM_IF_PARAM_SHUTDOWN    (1 << 0)
};

struct nsm_ip_pending
{
  struct prefix_ipv4 p;

  int secondary;
};
#endif /* HAVE_NSM_IF_PARAMS */

#ifdef HAVE_SNMP
/* error codes for th interface alias */
typedef enum nsm_if_alias {
  /* result ok */
  NSM_IF_ALIAS_OK = 0,
  /* no such interface */
  NSM_IF_ALIAS_NO_INTF,
  /* Invalid alias name */
  NSM_IF_ALIAS_INVALID,
  /* Invalid length */
  NSM_IF_ALIAS_INVLAID_LENGTH,
  /* alias not config */
  NSM_IF_ALIAS_NOT_CONFIG,
  /* max */
  NSM_IF_ALIAS_MAX
 
}NSM_IF_ALIAS_T;
 
#endif /* HAVE_SNMP */

extern const char *mpls_if_commands;
extern const char *ipv4_if_commands;
extern const char *ipv6_if_commands;
extern const char *l3_if_commands;

/* Prototypes. */
int nsm_if_add (struct interface *);
int nsm_if_delete (struct interface *, struct nsm_vrf *);
void nsm_if_delete_update (struct interface *);
void nsm_if_add_update (struct interface *, fib_id_t);
void nsm_if_delete_update_done (struct interface *);
void nsm_if_ifindex_update (struct interface *, int);
void nsm_if_up (struct interface *);
void nsm_if_down (struct interface *);
void nsm_if_refresh (struct interface *);
#ifdef HAVE_TE
void nsm_if_delete_interface_all (struct apn_vr *);
#endif /* HAVE_TE */
void nsm_if_addr_wakeup (struct interface *);
void nsm_if_init (struct lib_globals *);
void nsm_if_arbiter_start (int);

struct connected *nsm_ipv4_connected_table_lookup (struct nsm_vrf *,
                                                 struct prefix *);
struct connected *nsm_ipv4_check_address_overlap (struct interface *,
                                                  struct prefix *,
                                                  int secondary);
int nsm_ipv4_connected_table_set (struct apn_vrf *, struct connected *);
int nsm_ipv4_connected_table_unset (struct apn_vrf *, struct connected *);

int nsm_ip_address_install (u_int32_t, char *, struct pal_in4_addr *, u_char,
                            char *, int);
int nsm_ip_address_uninstall (u_int32_t, char *, struct pal_in4_addr *, u_char,
                              char *, int);
int nsm_ip_address_uninstall_all (u_int32_t, char *);

int nsm_ip_address_del (struct interface *, struct connected *);

struct connected *nsm_if_connected_primary_lookup (struct interface *);

#ifdef KERNEL_ALLOW_OVERLAP_ADDRESS
struct connected *nsm_if_connected_secondary_lookup (struct interface *,
                                                     struct pal_in4_addr *);
#endif /* KERNEL_ALLOW_OVERLAP_ADDRESS */

#ifdef HAVE_IPV6
struct connected *nsm_ipv6_check_address_overlap (struct interface *,
                                                  struct prefix *);
int nsm_ipv6_connected_table_set (struct apn_vrf *, struct connected *);
int nsm_ipv6_connected_table_unset (struct apn_vrf *, struct connected *);
int nsm_ipv6_address_install (u_int32_t, char *, struct pal_in6_addr *, u_char,
                              char *, char *, int);
int nsm_ipv6_address_uninstall (u_int32_t, char *, struct pal_in6_addr *,
                                u_char);
int nsm_ipv6_address_uninstall_all (u_int32_t, char *);
#endif /* HAVE_IPV6 */

int nsm_if_bind_vrf (struct interface *, struct nsm_vrf *);
int nsm_if_unbind_vrf (struct interface *, struct nsm_vrf *);
int nsm_if_bind_vr (struct interface *, struct nsm_master *);
int nsm_if_unbind_vr (struct interface *, struct nsm_master *);
int nsm_interface_set_mode_l2 (struct interface *ifp);
int nsm_interface_unset_mode_l2 (struct interface *ifp);
int nsm_interface_set_mode_l3 (struct interface *ifp);
int nsm_interface_unset_mode_l3 (struct interface *ifp);
bool_t nsm_check_l3_commands_allowed (struct cli *cli, struct interface *ifp,
    const char *err_str);
bool_t nsm_check_l3_gmpls_commands_allowed (struct cli *cli, 
    struct interface *ifp, const char *err_str);
bool_t nsm_check_l2_commands_allowed (struct cli *cli, struct interface *ifp);

#ifdef HAVE_VRX
int nsm_if_localsrc_func (struct interface *, int);
#endif /* HAVE_VRX */

#ifdef HAVE_NSM_IF_PARAMS
void nsm_if_params_clean (struct nsm_master *);
void nsm_if_shutdown_pending_set (struct nsm_master *, char *);
#endif /* HAVE_NSM_IF_PARAMS */
#ifdef HAVE_PBR
int nsm_ip_policy_if_config_write (struct cli *cli, struct interface *ifp);
void nsm_pbr_ip_policy_display (struct cli *cli);
#endif /* HAVE_PBR */

#ifdef HAVE_SNMP
/* set the alias for the interface */
NSM_IF_ALIAS_T
nsm_if_alias_set (struct interface *ifp, char * alias);
/* unset the alias for the inteface */
NSM_IF_ALIAS_T
nsm_if_alias_unset (struct interface *ifp);
#endif /* HAVE_SNMP */

#endif /* _NSM_INTERFACE_H */
