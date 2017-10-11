/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IF_H
#define _PACOS_IF_H

#include "pal.h"
#include "pal_socket.h"
#include "pal_if_stats.h"

#include "linklist.h"

#ifdef HAVE_TUNNEL
#include "tunnel/tunnel.h"
#endif /* HAVE_TUNNEL */

#ifdef HAVE_MPLS
#include "label_pool.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
#include "qos_common.h"
#endif /* HAVE_TE */

#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */

#ifdef HAVE_VRX
#include "vr/vr_vrx.h"
#endif /* HAVE_VRX */

#ifdef HAVE_MPLS
#include "mpls/mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_HA
#include "lib_ha.h"
#endif /* HAVE_HA */

#ifdef HAVE_GMPLS
#include "gmpls/gmpls.h"
#endif /* HAVE_GMPLS */

struct interface;
struct connected;

enum if_callback_type
{
  IF_CALLBACK_NEW,
  IF_CALLBACK_DELETE,
  IF_CALLBACK_UP,
  IF_CALLBACK_DOWN,
  IF_CALLBACK_UPDATE,
  IF_CALLBACK_VR_BIND,
  IF_CALLBACK_VR_UNBIND,
  IF_CALLBACK_VRF_BIND,
  IF_CALLBACK_VRF_UNBIND,
  IF_CALLBACK_PRIORITY_BW,
  IF_CALLBACK_MAX,
};

enum ifc_callback_type
{
  IFC_CALLBACK_ADDR_ADD,
  IFC_CALLBACK_ADDR_DELETE,
  IFC_CALLBACK_SESSION_CLOSE,
#ifdef HAVE_HA
  IFC_HA_CALLBACK_ADDR_ADD,
  IFC_HA_CALLBACK_ADDR_DELETE,
  IFC_HA_CALLBACK_ADDR_FWD_ADD,
  IFC_HA_CALLBACK_ADDR_FWD_DEL,
#endif /* HAVE_HA */
  IFC_CALLBACK_MAX,
};

#ifdef HAVE_HA
enum if_ha_callback_type
{
  IF_HA_CALLBACK_INTERFACE_UPDATE,
  IF_HA_CALLBACK_MAX,
};
#endif /* HAVE_HA */

struct stack_index
{
  int *top;
  int *base;
  int stacksize;
};

struct meg_master
{
 struct lib_globals *zg;
 struct list *meg_list;
 struct hash *meg_hash;
 struct stack_index mstack;
 pal_time_t  megTblLastChange;

};//czh

struct if_master
{
  /* Lib Globals. */
  struct lib_globals *zg;

  /* Interface table. */
  struct route_table *if_table;

  /* Interface list. */
  struct list *if_list;

  pal_time_t ifTblLastChange;

  /* Internal hash for name lookup. */
  struct hash *if_hash;

#ifdef HAVE_INTERFACE_NAME_MAPPING
  /* Internal hash for name mapping. */
  struct hash *if_map_hash;
#endif /* HAVE_INTERFACE_NAME_MAPPING */

  /* Callback functions. */
  int (*if_callback[IF_CALLBACK_MAX]) (struct interface *);
  int (*ifc_callback[IFC_CALLBACK_MAX]) (struct connected *);

#ifdef HAVE_HA
  int (*if_ha_callback[IF_HA_CALLBACK_MAX]) (struct interface *, u_int32_t );
#endif /* HAVE_HA */
};


struct if_vr_master
{
  /* Pointer to VR. */
  struct apn_vr *vr;

  /* Interface table. */
  struct route_table *if_table;

  /* Interface list. */
  struct list *if_list;

#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
#endif /* HAVE_GMPLS */
};

struct if_vrf_master
{
  /* Pointer to VRF. */
  struct apn_vrf *vrf;

  /* Interface table. */
  struct route_table *if_table;

  /* IPv4 address table. */
  struct route_table *ipv4_table;

#ifdef HAVE_IPV6
  /* IPv6 address table. */
  struct route_table *ipv6_table;
#endif /* HAVE_IPV6 */

#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
#endif /* HAVE_GMPLS */

};

/* For interface table key. */
struct prefix_if
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  s_int32_t ifindex;
};

#define INTERFACE_NAMSIZ      ((IFNAMSIZ)+4)
#define INTERFACE_HWADDR_MAX  (IFHWASIZ)

/* Internal If indexes start at 0xFFFFFFFF and go down to 1 greater
   than this */

#define IFINDEX_INTERNBASE 0x80000000

/* Bandwidth-specific defines */
#define MAX_BANDWIDTH                  10000000000.0
#define LEGAL_BANDWIDTH(b)             (((b) > 0) && ((b) <= MAX_BANDWIDTH))
#define BW_CONSTANT                    1000
#define BW_BUFSIZ                      15

/* Duplex-specific defines */
#define NSM_IF_HALF_DUPLEX      0
#define NSM_IF_FULL_DUPLEX      1
#define NSM_IF_AUTO_NEGO        2
#define NSM_IF_DUPLEX_UNKNOWN   3

#define NSM_IF_AUTONEGO_DISABLE 1
#define NSM_IF_AUTONEGO_ENABLE  0

/* Interface structure */
struct interface
{
  /* Interface name. */
  char name[INTERFACE_NAMSIZ + 1];
#ifdef HAVE_INTERFACE_NAME_MAPPING
  char orig[INTERFACE_NAMSIZ + 1];
#endif /* HAVE_INTERFACE_NAME_MAPPING */

  /* Interface index. */
  s_int32_t ifindex;

  /* Interface attribute update flags.  */
  u_int32_t cindex;

  /* Interface flags. */
  u_int32_t flags;

  /* PacOS internal interface status */
  u_int32_t status;
#define NSM_INTERFACE_ACTIVE            (1 << 0)
#define NSM_INTERFACE_ARBITER           (1 << 1)
#define NSM_INTERFACE_MAPPED            (1 << 2)
#define NSM_INTERFACE_MANAGE            (1 << 3)
#define NSM_INTERFACE_DELETE            (1 << 4)
#define NSM_INTERFACE_IPV4_UNNUMBERED   (1 << 5)
#define NSM_INTERFACE_IPV6_UNNUMBERED   (1 << 6)
#ifdef HAVE_HA
#define HA_IF_STALE_FLAG                (1 << 7)
#endif /* HAVE_HA */
#define IF_HIDDEN_FLAG                  (1 << 8)
#define IF_NON_CONFIGURABLE_FLAG        (1 << 9)
#define IF_NON_LEARNING_FLAG            (1 << 10)

  /* Interface metric */
  s_int32_t metric;

  /* Interface MTU. */
  s_int32_t mtu;

  /* Interface DUPLEX status. */
  u_int32_t duplex;

  /* Interface AUTONEGO. */
  u_int32_t autonego;

  /* Interface MDIX crossover. */
  u_int32_t mdix;

  /* Interface ARP AGEING TIMEOUT. */
  u_int32_t arp_ageing_timeout;

  /* Slot Id. */
  u_int32_t slot_id;

  /* Hardware address. */
  u_int16_t hw_type;
  u_int8_t hw_addr[INTERFACE_HWADDR_MAX];

  s_int32_t hw_addr_len;

  /* interface bandwidth, bytes/s */
  float32_t bandwidth;

  /*Interface link up or link down traps */
  s_int32_t if_linktrap;

  /* Interface alias name */
  char if_alias[INTERFACE_NAMSIZ + 1];

  /* Has the bandwidth been configured/read from kernel. */
  char conf_flags;
#define NSM_BANDWIDTH_CONFIGURED    (1 << 0)
#define NSM_MAX_RESV_BW_CONFIGURED  (1 << 1)
#define NSM_SWITCH_CAP_CONFIGURED   (1 << 2)
#define NSM_MIN_LSP_BW_CONFIGURED   (1 << 3)
#define NSM_MAX_LSP_SIZE_CONFIGURED (1 << 4)
  /* description of the interface. */
  char *desc;

  /* Connected address list. */
  struct connected *ifc_ipv4;
#ifdef HAVE_IPV6
  struct connected *ifc_ipv6;
#endif /* HAVE_IPV6 */

  /* Unnumbered interface list.  */
  struct list *unnumbered_ipv4;
#ifdef HAVE_IPV6
  struct list *unnumbered_ipv6;
#endif /* HAVE_IPV6 */

  /* Daemon specific interface data pointer. */
  void *info;

  /* Pointer to VR/VRF context. */
  struct apn_vr *vr;
  struct apn_vrf *vrf;

  /* Statistics fileds. */
  struct pal_if_stats stats;

#ifdef HAVE_TUNNEL
  /* tunnel interface. */
  struct tunnel_if *tunnel_if;
#endif /* HAVE_TUNNEL */

#ifdef HAVE_MPLS
  /* Label space */
  struct label_space_data ls_data;
#endif /* HAVE_MPLS */

#ifdef HAVE_TE
  /* Administrative group that this if belongs to */
  u_int32_t admin_group;

  /* Maximum reservable bandwidth (bytes/s) */
  float32_t max_resv_bw;

#ifdef HAVE_DSTE
  /* Bandwidth constraint per class types (bytes/s) */
  float32_t bw_constraint[MAX_BW_CONST];
#endif /* HAVE_DSTE */

  /* Available bandwidth at priority p, 0 <= p < 8 */
  float32_t tecl_priority_bw [MAX_PRIORITIES];

#endif /* HAVE_TE */

  /* Bind information.  */
  u_char bind;
#define NSM_IF_BIND_VRF           (1 << 0)
#define NSM_IF_BIND_MPLS_VC       (1 << 1)
#define NSM_IF_BIND_MPLS_VC_VLAN  (1 << 2)
#define NSM_IF_BIND_VPLS          (1 << 3)
#define NSM_IF_BIND_VPLS_VLAN     (1 << 4)

#ifdef HAVE_GMPLS
  /* GMPLS information */
  /* Number of data links. Based on this information we will either use the
    tree if dl > 1 or use the pointer to the datalink structure */
  u_char num_dl;

  /* Type includes unknow/data/control/data-control */
  u_char gmpls_type;

  u_int32_t gifindex;

  /* GMPLS interface common properties */
  struct phy_properties phy_prop;

  /* Pointer to datalink */
  union
    {
      struct avl_tree *dltree;
      struct datalink *datalink;
    }dlink;

#endif /* HAVE_GMPLS */

#ifdef HAVE_L2
  void *port;
  char bridge_name[INTERFACE_NAMSIZ + 1 ];
#endif /* HAVE_L2 */

#ifdef HAVE_LACPD
  u_int16_t lacp_admin_key;
  u_int16_t agg_param_update;
  u_int32_t lacp_agg_key;
#endif /* HAVE_LACPD */

#ifdef HAVE_DSTE
  /* Bandwdith constrain mode for every interface. */
  bc_mode_t bc_mode;
#endif /* HAVE_DSTE */

#ifdef HAVE_VRX
  u_char vrx_flag;
#define IF_VRX_FLAG_NORMAL              0
#define IF_VRX_FLAG_WRPJ                1
#define IF_VRX_FLAG_WRP                 2

  /* Local src. */
  u_char local_flag;
#define IF_VRX_FLAG_LOCALSRC            1

  /* Related VRX information. */
  struct vrx_if_info *vrxif;
#endif /* HAVE_VRX */
  pal_time_t ifLastChange;

#ifdef HAVE_CUSTOM1
  int pid;
#endif /* HAVE_CUSTOM1 */
  u_char type; /* Interface type L2/L3 */
#define IF_TYPE_L3 0
#define IF_TYPE_L2 1

  /* To store the configured duplex value */
  u_char config_duplex;

#ifdef HAVE_QOS
  int trust_state;
#endif /* HAVE_QOS */

#ifdef HAVE_VLAN_CLASS
 u_int32_t vlan_classifier_group_id;
#endif /* HAVE_VLAN_CLASS */

 struct list *clean_pend_resp_list;

#ifdef HAVE_HA
 HA_CDR_REF interface_cdr_ref;
#endif /* HAVE_HA */
};

/* Connected address structure. */
struct connected
{
  struct connected *next;
  struct connected *prev;

  /* Attached interface. */
  struct interface *ifp;

  /* Address family for prefix. */
  u_int8_t family;

  /* Flags for configuration. */
  u_int8_t conf;
#define NSM_IFC_REAL            (1 << 0)
#define NSM_IFC_CONFIGURED      (1 << 1)
#define NSM_IFC_ARBITER         (1 << 2)
#define NSM_IFC_ACTIVE          (1 << 3)

  /* Flags for connected address. */    /* XXX-VR */
  u_int8_t flags;
#define NSM_IFA_SECONDARY       (1 << 0)
#define NSM_IFA_ANYCAST         (1 << 1)
#define NSM_IFA_VIRTUAL         (1 << 2)
#ifdef HAVE_VRX
#define NSM_IFA_VRX_WRP         (1 << 3)
#endif /* HAVE_VRX */
#ifdef HAVE_HA
#define NSM_IFA_HA_DELETE       (1 << 4)
#endif /* HAVE_HA */

  /* Address of connected network. */
  struct prefix *address;
  struct prefix *destination;

  /* Label for Linux 2.2.X and upper. XXX-VR */
  char *label;

#ifdef HAVE_HA
 HA_CDR_REF lib_connected_cdr_ref;
#endif /* HAVE_HA */
};

#ifdef HAVE_INTERFACE_NAME_MAPPING
struct if_map
{
  char from[INTERFACE_NAMSIZ + 1];
  char to[INTERFACE_NAMSIZ + 1];
};
#endif /* HAVE_INTERFACE_NAME_MAPPING */

/*
 * Utility Macros for manipulating Library Structures
 */
#define INTF_TYPE_L2(IFP)                  ((IFP)->type == IF_TYPE_L2)
#define INTF_TYPE_L3(IFP)                  ((IFP)->type == IF_TYPE_L3)
#define LIB_IF_SET_LIB_VR(LIB_IF, LIB_VR)                             \
    (((LIB_IF)->vr) = (LIB_VR))
#define LIB_IF_GET_LIB_VR(LIB_IF)          ((LIB_IF)->vr)

#define LIB_IF_SET_LIB_VRF(LIB_IF, LIB_VRF)                           \
    (((LIB_IF)->vrf) = (LIB_VRF))
#define LIB_IF_GET_LIB_VRF(LIB_IF)          ((LIB_IF)->vrf)

#define PREFIX_IF_SET(P,I)                              \
  do {                                                  \
    pal_mem_set ((P), 0, sizeof (struct prefix_if));    \
    (P)->family = AF_INET;                              \
    (P)->prefixlen = 32;                                \
    (P)->ifindex = pal_hton32 (I);                      \
  } while (0)

#define RN_IF_INFO_SET(R,V)                     \
  do {                                          \
    (R)->info = (V);                            \
    route_lock_node (R);                        \
  } while (0)

#define RN_IF_INFO_UNSET(R)                     \
  do {                                          \
    (R)->info = NULL;                           \
    route_unlock_node (R);                      \
  } while (0)

/* Exported variables. */
extern struct cli_element interface_desc_cli;
extern struct cli_element no_interface_desc_cli;

/* Prototypes. */
struct connected *ifc_new (u_char);
struct connected *ifc_get_ipv4 (struct pal_in4_addr *, u_char,
                                struct interface *);
#ifdef HAVE_IPV6
struct connected *ifc_get_ipv6 (struct pal_in6_addr *, u_char,
                                struct interface *);
#endif /* HAVE_IPV6 */
void ifc_free (struct lib_globals *, struct connected *);
u_int32_t if_ifc_ipv4_count (struct interface *);
void if_add_ifc_ipv4 (struct interface *, struct connected *);
void if_delete_ifc_ipv4 (struct interface *, struct connected *);
char * if_map_lookup (struct hash *, char *);
struct connected *if_lookup_ifc_ipv4 (struct interface *,
                                      struct pal_in4_addr *);
struct connected *if_lookup_ifc_prefix (struct interface *, struct prefix *);

struct connected *if_match_ifc_ipv4_direct (struct interface *,
                                            struct prefix *);
void if_delete_ifc_by_ipv4_addr (struct interface *, struct pal_in4_addr *);
#ifdef HAVE_IPV6
u_int32_t if_ifc_ipv6_count (struct interface *);
void if_add_ifc_ipv6 (struct interface *, struct connected *);
void if_delete_ifc_ipv6 (struct interface *, struct connected *);
struct connected *if_lookup_ifc_ipv6 (struct interface *,
                                      struct pal_in6_addr *);
struct connected *if_lookup_ifc_ipv6_linklocal (struct interface *);
struct connected *if_lookup_ifc_ipv6_global (struct interface *);
void if_delete_ifc_by_ipv6_addr (struct interface *, struct pal_in6_addr *);
struct connected *if_match_ifc_ipv6_direct (struct interface *, struct prefix *);
#endif /* HAVE_IPV6 */
struct interface *if_lookup_by_name (struct if_vr_master *, char *);
struct interface *if_lookup_by_index (struct if_vr_master *, u_int32_t);
struct interface *if_lookup_by_ipv4_address (struct if_vr_master *,
                                             struct pal_in4_addr *);
struct interface *if_lookup_loopback (struct if_vr_master *);
struct interface *if_match_by_ipv4_address (struct if_vr_master *,
                                            struct pal_in4_addr *, vrf_id_t);
struct interface * if_lookup_by_hw_addr (struct if_vr_master *ifm,
                                                char *mac_addr);
struct prefix *if_get_connected_address (struct interface *, u_int8_t);
struct interface *if_match_all_by_ipv4_address (struct if_vr_master *,
                                                struct pal_in4_addr *,
                                                struct connected **);
struct interface *ifv_lookup_by_prefix (struct if_vrf_master *,
                                        struct prefix *);

#ifdef HAVE_IPV6
struct interface *if_lookup_by_ipv6_address (struct if_vr_master *,
                                             struct pal_in6_addr *);
struct interface *if_match_by_ipv6_address (struct if_vr_master *,
                                            struct pal_in6_addr *, vrf_id_t);
struct interface *if_match_all_by_ipv6_address (struct if_vr_master *,
                                                struct pal_in6_addr *);
#endif /* HAVE_IPV6 */

struct interface *if_lookup_by_prefix (struct if_vr_master *, struct prefix *);
#ifdef HAVE_MPLS
struct interface *if_mpls_lookup_by_index (struct if_vr_master *, u_int32_t);
struct interface *if_mpls_lookup_by_index_next (struct if_vr_master *,
                                                u_int32_t);
#endif /* HAVE_MPLS */
char *if_index2name (struct if_vr_master *, int);
s_int32_t if_name2index (struct if_vr_master *, char *);
char *if_index2name_copy (struct if_vr_master *, int, char *);
char *if_kernel_name (struct interface *);

struct interface *if_new (struct if_master *);
void if_delete (struct if_master *, struct interface *);
void if_master_init (struct if_master *, struct lib_globals *);
void if_master_finish (struct if_master *);
int ifg_table_add (struct if_master *, int, struct interface *);
int ifg_table_delete (struct if_master *, int);
struct interface *ifg_lookup_by_index (struct if_master *, u_int32_t);
struct interface *ifg_lookup_by_prefix (struct if_master *, struct prefix *);
void ifg_list_add (struct if_master *, struct interface *);
void ifg_list_delete (struct if_master *, struct interface *);
struct interface *ifg_lookup_by_name (struct if_master *, char *);
struct interface *ifg_get_by_name (struct if_master *, char *);

void if_vr_master_init (struct if_vr_master *, struct apn_vr *);
void if_vr_master_finish (struct if_vr_master *, struct apn_vr *);
int if_vr_bind (struct if_vr_master *, int);
int if_vr_unbind (struct if_vr_master *, int);
int if_vr_table_add (struct if_vr_master *ifvrm, int ifindex, struct interface *ifp);
int if_vr_table_delete (struct if_vr_master *ifvrm, int ifindex);

void if_vrf_master_init (struct if_vrf_master *, struct apn_vrf *);
void if_vrf_master_finish (struct if_vrf_master *, struct apn_vrf *);
int if_vrf_bind (struct if_vrf_master *, int);
int if_vrf_unbind (struct if_vrf_master *, int);
int if_vrf_table_add (struct if_vrf_master *ifvrm, int ifindex, struct interface *ifp);
int if_vrf_table_delete (struct if_vrf_master *ifvrm, int ifindex);
struct interface * ifv_lookup_by_name (struct if_vrf_master *, char *);
struct interface *ifv_lookup_by_index (struct if_vrf_master *, u_int32_t);
struct interface *ifv_lookup_next_by_index (struct if_vrf_master *, u_int32_t);
int if_ifindex_update (struct if_master *, struct interface *, int);

#ifdef HAVE_HA
void
if_ha_add_hook (struct if_master *ifg, enum if_ha_callback_type type,
                int (*func) (struct interface *, u_int32_t));
#endif /* HAVE_HA */

void if_add_hook (struct if_master *, enum if_callback_type,
                  int (*func) (struct interface *));
void ifc_add_hook (struct if_master *, enum ifc_callback_type,
                   int (*func) (struct connected *));

u_int16_t if_get_hw_type (struct interface *);
result_t if_is_up (struct interface *);
result_t if_is_running (struct interface *);
result_t if_is_loopback (struct interface *);
result_t if_is_broadcast (struct interface *);
result_t if_is_pointopoint (struct interface *);
result_t if_is_multicast (struct interface *);
result_t if_is_tunnel (struct interface *);
#ifdef HAVE_TUNNEL_IPV6
result_t if_is_6to4_tunnel (struct interface *);
#endif /* HAVE_TUNNEL_IPV6 */
result_t if_is_vlan (struct interface *);
result_t if_is_ipv4_unnumbered (struct interface *);
#ifdef HAVE_IPV6
result_t if_is_ipv6_unnumbered (struct interface *);
#endif /* HAVE_IPV6 */

result_t bandwidth_string_to_float (char *, float32_t *);
char *bandwidth_float_to_string (char *, float32_t);
#ifdef HAVE_TE
int if_conforms_to_resrc_affinity (struct interface *, u_int32_t, u_int32_t,
                                   u_int32_t);
#endif /* HAVE_TE */

#endif /* _PACOS_IF_H */
