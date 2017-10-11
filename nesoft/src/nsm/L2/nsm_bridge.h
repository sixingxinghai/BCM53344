/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_BRIDGE_H__
#define __NSM_BRIDGE_H__
#ifdef HAVE_HAL
#include "hal_types.h"
#include "hal_incl.h"
#endif /* HAVE_HAL */
#include "nsm/nsm_server.h"

#ifdef HAVE_DCB
#include "nsm_dcb.h"
#endif /* HAVE_DCB */

#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */


#ifndef HAVE_CUSTOM1

#define NSM_VLAN_NULL_VID            0                    /* Null Vid */

#define NSM_BRIDGE_INSTANCE_MIN      0
#define NSM_BRIDGE_INSTANCE_MAX      1
#ifdef HAVE_MSTPD
#undef NSM_BRIDGE_INSTANCE_MAX

#ifdef HAVE_RPVST_PLUS
#define NSM_RPVST_BRIDGE_INSTANCE_MAX   4096 
#endif /* HAVE_RPVST_PLUS */

#define NSM_BRIDGE_INSTANCE_MAX      64
#endif /* HAVE_MSTPD */

#define TOPOLOGY_NONE                        0
#define TOPOLOGY_RING                        1

#define NSM_BRIDGE_STR_LEN                   6

#if defined HAVE_CFM && (defined HAVE_I_BEB || defined HAVE_B_BEB)
#define NSM_PBB_BACKBONE_EDGE_SET            1
#define NSM_PBB_BACKBONE_STR_LEN             7
#endif /* HAVE_CFM && (HAVE_I_BEB || HAVE_B_BEB)  */

#define  NSM_BRIDGE_PBB_TE           0
#define  NSM_BRIDGE_8031             1
#define  NSM_BRIDGE_8032             2
#define  NSM_INSTANCE_MAP_LEN        4

/* INSTANCE bitmap manipulation macros. */
#define NSM_INSTANCE_MAX             4094
#define NSM_INSTANCE_BMP_WORD_WIDTH                32
#define NSM_INSTANCE_BMP_WORD_MAX                  \
((NSM_INSTANCE_MAX + NSM_INSTANCE_BMP_WORD_WIDTH) / NSM_INSTANCE_BMP_WORD_WIDTH)

  struct nsm_instance_bmp
{
    u_int32_t bitmap[NSM_INSTANCE_BMP_WORD_MAX];
};

#define NSM_INSTANCE_BMP_INIT(bmp)                                             \
do {                                                                    \
    pal_mem_set ((bmp).bitmap, 0, sizeof ((bmp).bitmap));               \
} while (0)

#define NSM_INSTANCE_BMP_SET(bmp, instance)                                         \
do {                                                                    \
    int _word = (instance) / NSM_INSTANCE_BMP_WORD_WIDTH;                       \
      (bmp).bitmap[_word] |= (1U << ((instance) % NSM_INSTANCE_BMP_WORD_WIDTH));  \
} while (0)

#define NSM_INSTANCE_BMP_UNSET(bmp, instance)                                       \
do {                                                                    \
    int _word = (instance) / NSM_INSTANCE_BMP_WORD_WIDTH;                       \
      (bmp).bitmap[_word] &= ~(1U <<((instance) % NSM_INSTANCE_BMP_WORD_WIDTH));  \
} while (0)


#define NSM_INSTANCE_BMP_IS_MEMBER(bmp, instance)                                   \
((bmp).bitmap[(instance) / NSM_INSTANCE_BMP_WORD_WIDTH] & (1U << ((instance) % NSM_INSTANCE_BMP_WORD_WIDTH)))


#define NSM_INSTANCE_UNSET_BMP_ITER_END(bmp, instance)                              \
} while (0)

#define NSM_INSTANCE_BMP_CLEAR(bmp)         NSM_INSTANCE_BMP_INIT(bmp)

#define DOT1DPORTSTATICSTATUS_OTHER             1
#define DOT1DPORTSTATICSTATUS_INVALID           2
#define DOT1DPORTSTATICSTATUS_PERMANENT         3
#define DOT1DPORTSTATICSTATUS_DELETEONRESET     4
#define DOT1DPORTSTATICSTATUS_DELETEONTIMEOUT   5

struct nsm_if;

enum nsm_topology
{
  NSM_TOPOLOGY_NONE,
  NSM_TOPOLOGY_RING
};
#ifdef HAVE_G8032
struct nsm_g8032_ring
{
  u_int16_t g8032_ring_id;
  u_int32_t instance;
  u_int32_t ifindex_e;
  u_int32_t ifindex_w;
  u_int32_t primary_vid;
  u_int32_t g8032_instance_id;
  struct avl_tree * g8032_vids;
};
#endif  /*HAVE_G8032*/

struct nsm_bridge
{
  struct nsm_bridge *next;
  struct nsm_bridge **pprev;

  /* Name. */
  char name[NSM_BRIDGE_NAMSIZ + 1];

  /* Type. */
  u_int8_t type;

  u_int8_t is_default;

  /* Ageing Time for fdb entries */
  u_int32_t ageing_time;

  /* Spanning tree Enable */
  u_int16_t enable;

#ifdef HAVE_PROVIDER_BRIDGE
  u_int8_t provider_edge:1;
#endif /* HAVE_PROVIDER_BRIDGE */

#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
  u_int8_t      backbone_edge:1;
  u_int32_t 	bridge_id;	
  u_int8_t 	bridge_mac[ETHER_ADDR_LEN];
#endif /* HAVE_I_BEB || HAVE_B_BEB */

#if defined (HAVE_PBB_TE)
  struct avl_tree *pbb_te_group_tree;
#endif /* HAVE_I_BEB && HAVE_B_BEB && HAVE_PBB_TE */

#if defined (HAVE_I_BEB)
  u_int8_t      vip_port_map[512];
#endif /* HAVE_I_BEB */

#define NSM_BRIDGE_AGEING_DEFAULT    300  
  /* learning. Whether the bridge is learning bridge or not */
  int learning;
#define NSM_LEARNING_BRIDGE_SET      1
#define NSM_LEARNING_BRIDGE_UNSET    0

  /* Port list. */
  struct avl_tree *port_tree;

#ifdef HAVE_SNMP
  struct bitmap *port_id_mgr;
#endif /* HAVE_SNMP */

  /* Static FDB List */

  struct ptree *static_fdb_list;

  struct list *bridge_listener_list;

#ifdef HAVE_VLAN
  /* VLAN table. */
  struct avl_tree *vlan_table;
  struct avl_tree *stp_decoupled_vlan_table;

  struct avl_tree *svlan_table;
  struct avl_tree *stp_decoupled_svlan_table;

  struct avl_tree *pro_edge_swctx_table;
  struct list *cvlan_reg_tab_list;
  struct list *vlan_listener_list;
#ifdef HAVE_B_BEB
  struct avl_tree *bvlan_table;
  struct avl_tree *stp_decoupled_bvlan_table;
#endif /* HAVE_B_BEB */
#endif /* HAVE_VLAN */

  /* Thread event. */
  struct thread *event;

  /* Back pointer to bridge master. */
  struct nsm_bridge_master *master;
  
#ifdef HAVE_GVRP
  /* Gvrp structure for the bridge */
  struct gvrp *gvrp;
#endif /* HAVE_GMRP */
  
#ifdef HAVE_GMRP
  /* Gmrp structure for the bridge */
  struct avl_tree *gmrp_list;
  struct gmrp_bridge *gmrp_bridge;
  unsigned char gmrp_ext_filter:1;
#endif /* HAVE_GVRP */

  u_int32_t vlan_num_deletes ;

  /* Pointer to configuration store */
  struct nsm_bridge_config *br_conf;
  u_int8_t traffic_class_enabled;
  /* Topology type - none/ring - currently used to support RRSTP */
  enum nsm_topology topology_type;
#ifdef HAVE_HA
  HA_CDR_REF nsm_bridge_cdr_ref;
#endif /* HAVE_HA */

#ifdef HAVE_QOS
  /* Num of Class of service queues */
  s_int32_t num_cosq;
#endif /* HAVE_QOS */

#ifdef HAVE_G8031
  struct avl_tree * eps_tree;
#endif /* HAVE_G8031 */

#ifdef HAVE_G8032
    struct avl_tree *raps_tree;
#endif  /*HAVE_G8032*/

#ifdef HAVE_DCB
   struct nsm_dcb_bridge *dcbg;
#endif /* HAVE_DCB */

 u_int16_t max_mst_instances;

 u_int8_t uni_type_mode; ///< UNI Type enabled or disabled or not the bridge 
    
};

#define CLI_MAC             1
#define IGMP_SNOOPING_MAC   2
#define GMRP_MAC            3
#define MLD_SNOOPING_MAC    4
#define AUTH_MAC            5

#if defined HAVE_GMPLS && defined HAVE_GELS
#define GELS_MAC            6
#endif /* HAVE_GMPLS && HAVE_GELS */

#define ETHER_MAX_BIT_LEN   48

enum nsm_bridge_pri_ovr_mac_type
{
  NSM_BRIDGE_MAC_PRI_OVR_NONE,
  NSM_BRIDGE_MAC_STATIC,
  NSM_BRIDGE_MAC_STATIC_PRI_OVR,
  NSM_BRIDGE_MAC_STATIC_MGMT,
  NSM_BRIDGE_MAC_STATIC_MGMT_PRI_OVR,
  NSM_BRIDGE_MAC_PRI_OVR_MAX,
};

struct nsm_bridge_fdb_ifinfo
{
  u_int32_t ifindex;
  bool_t is_forward;
  unsigned char type;
  enum nsm_bridge_pri_ovr_mac_type ovr_mac_type;
  unsigned char priority;
  struct nsm_bridge_fdb_ifinfo *next_if;
  struct nsm_bridge_fdb_ifinfo *prev_if;

#ifdef HAVE_HA
  HA_CDR_REF mac_cdr_ref;
#endif /* HAVE_HA */

#ifdef HAVE_G8032
  struct avl_tree *raps_tree;
#endif  /*HAVE_G8032*/
};

struct nsm_bridge_static_fdb
{
  u_int8_t mac_addr [ETHER_ADDR_LEN];
  struct nsm_bridge_fdb_ifinfo *ifinfo_list;
  unsigned char snmp_status;
};

struct nsm_bridge_static_fdb_config
{
  char bridge_name [NSM_BRIDGE_NAMSIZ + 1];
  u_int8_t mac_addr [ETHER_ADDR_LEN];
  u_int16_t vid;
  u_int16_t svid;
  bool_t is_forward;
  enum nsm_bridge_pri_ovr_mac_type ovr_mac_type;
  unsigned char priority;
};

struct nsm_bridge_master
{
  struct nsm_bridge *bridge_list;

#if defined(HAVE_I_BEB) || defined(HAVE_B_BEB) 
  struct nsm_beb_bridge *beb;
#endif 

#if defined HAVE_PBB_TE || defined HAVE_G8031 || defined HAVE_G8032
   struct nsm_instance_bmp instanceBmp;
#endif /*HAVE_PBB_TE || defined HAVE_G8031 || defined HAVE_G8032 */

#if defined HAVE_PBB_TE || defined HAVE_G8031 || defined HAVE_G8032
  u_int8_t instance_map[NSM_INSTANCE_MAP_LEN];
#endif /*HAVE_PBB_TE*/

#ifdef HAVE_B_BEB
  struct nsm_bridge *b_bridge;	
#endif

  struct nsm_l2_mcast *l2mcast;

  /* Back pointer to nsm_master. */
  struct nsm_master *nm;

#ifdef HAVE_VLAN_CLASS
  struct avl_tree *group_tree;
  struct avl_tree *rule_tree;
#endif /* HAVE_VLAN_CLASS */

  /* Thread event. */
  struct thread *event;
};

#ifdef HAVE_HAL
extern u_char nsm_default_traffic_class_table [] [HAL_BRIDGE_MAX_TRAFFIC_CLASS];
#endif /* HAVE_HAL */

#define NSM_SEND_TO_L2_PROTOCOL(NSE)                                         \
    ( ((NSE)->service.protocol_id == APN_PROTO_STP)                          \
   || ((NSE)->service.protocol_id == APN_PROTO_RSTP)                         \
   || ((NSE)->service.protocol_id == APN_PROTO_MSTP) )

#define NSM_SEND_TO_BRIDGE_PROTOCOL(BR,NSE)                                  \
    ((((BR)->type == NSM_BRIDGE_TYPE_STP                                     \
       || (BR)->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)                       \
      && ((NSE)->service.protocol_id == APN_PROTO_STP))                      \
          || (((BR)->type == NSM_BRIDGE_TYPE_RSTP                            \
               || (BR)->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE)              \
              && ((NSE)->service.protocol_id == APN_PROTO_RSTP))             \
     || (((BR)->type == NSM_BRIDGE_TYPE_MSTP)                                \
         && ((NSE)->service.protocol_id == APN_PROTO_MSTP)))

#define NSM_BRIDGE_CHECK_L2_PROPERTY(CLI,IFP)                                \
   do {                                                                      \
        struct nsm_if *_zif = (struct nsm_if *)(IFP)->info;                  \
        if (_zif->type != NSM_IF_TYPE_L2)                                    \
          {                                                                  \
            cli_out (CLI, "%% Interface not configured for switching\n");    \
            return CLI_ERROR;                                                \
          }                                                                  \
   } while (0)

#define NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(CLI, IFP)                           \
   do {                                                                      \
        struct nsm_if *_zif = (struct nsm_if *)((IFP)->info);                \
        struct nsm_bridge *_bridge;                                          \
        if (_zif)                                                            \
        {                                                                    \
          _bridge = _zif->bridge;                                            \
          if (_bridge)                                                       \
          {                                                                  \
            if ( !((_bridge->type == NSM_BRIDGE_TYPE_STP_VLANAWARE) ||       \
                   (_bridge->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE) ||      \
                   (_bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS) ||          \
                   (_bridge->type == NSM_BRIDGE_TYPE_MSTP) ||                \
                   (_bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||       \
                   (_bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP) ||       \
                   (_bridge->type == NSM_BRIDGE_TYPE_BACKBONE_RSTP) ||       \
                   (_bridge->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP)) )       \
            {                                                                \
              cli_out((CLI), "%% Bridge is not vlan aware\n");               \
              return CLI_ERROR;                                              \
            }                                                                \
          }                                                                  \
        }                                                                    \
   }while (0)

#define NSM_BRIDGE_TYPE_PROVIDER(BR)                                         \
              (((BR)->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||              \
              ((BR)->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP))

#define NSM_BRIDGE_TYPE_BACKBONE(BR)                                         \
               (!(pal_strncmp(((BR)->name),"backbone",8)))                  
#define NSM_BRIDGE_VLAN_AWARE(BR)                                            \
              (((BR)->type == NSM_BRIDGE_TYPE_STP_VLANAWARE) ||              \
               ((BR)->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE) ||             \
               ((BR)->type == NSM_BRIDGE_TYPE_RPVST_PLUS) ||                 \
               ((BR)->type == NSM_BRIDGE_TYPE_MSTP) ||                       \
               ((BR)->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP) ||              \
               ((BR)->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP))

#define NSM_BRIDGE_PROTO_DISCARD_SET(VAR, PROTO)                        \
   (SET_FLAG (VAR, NSM_VLAN_CVLAN_DISCARD_## PROTO))

#define NSM_BRIDGE_PROTO_DISCARD_UNSET(VAR, PROTO)                      \
   (UNSET_FLAG (VAR, NSM_VLAN_CVLAN_DISCARD_## PROTO))

#define NSM_BRIDGE_PROTO_DISCARD_CHECK(VAR, PROTO)                      \
   (CHECK_FLAG (VAR, NSM_VLAN_CVLAN_DISCARD_## PROTO))

#ifdef HAVE_LACPD

#define AGG_MEM_NO_SWITCHPORT -93
#define AGG_MEM_BRIDGE_NOT_VLAN_AWARE -92

#define NSM_BRIDGE_CHECK_AGG_MEM_L2_PROPERTY(IFP)                            \
   do {                                                                      \
        struct nsm_if *_zif = (struct nsm_if *)(IFP)->info;                  \
        if (_zif->type != NSM_IF_TYPE_L2)                                    \
          {                                                                  \
            return AGG_MEM_NO_SWITCHPORT;                                    \
          }                                                                  \
   } while (0)

#define NSM_BRIDGE_CHECK_AGG_MEM_BRIDGE_PROPERTY(IFP)                        \
   do {                                                                      \
        struct nsm_if *_zif = (struct nsm_if *)((IFP)->info);                \
        struct nsm_bridge *_bridge;                                          \
        if (_zif)                                                            \
        {                                                                    \
          _bridge = _zif->bridge;                                            \
          if (_bridge)                                                       \
          {                                                                  \
            if ( !((_bridge->type == NSM_BRIDGE_TYPE_STP_VLANAWARE) ||       \
                   (_bridge->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE) ||      \
                   (_bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS) ||          \
                   (_bridge->type == NSM_BRIDGE_TYPE_MSTP)) )                \
            {                                                                \
              return AGG_MEM_BRIDGE_NOT_VLAN_AWARE;                          \
            }                                                                \
          }                                                                  \
        }                                                                    \
   }while (0)

#endif /* HAVE_LACP  */

struct nsm_bridge_config
{
  struct nsm_bridge_config *next;
  char br_name[NSM_BRIDGE_NAMSIZ];
#ifdef HAVE_VLAN
  struct list *vlan_config_list;
#endif /* HAVE_VLAN */
};

/* Listener Id */
typedef enum nsm_listener_id
{
  NSM_LISTENER_ID_MIN = 0,
  NSM_LISTENER_ID_GVRP_PROTO = 1,
  NSM_LISTENER_ID_GMRP_PROTO = 2,
  NSM_LISTENER_ID_L2MCAST_PROTO = 3,
  NSM_LISTENER_ID_MAX = 4
} nsm_listener_id_t;

struct nsm_bridge_listener
{
  /* Holds nsm listener id */
  nsm_listener_id_t listener_id;
  
  /* Callback nsm listener function that gets upcalled when a bridge gets
     added */
  int (*add_bridge_func) (struct nsm_bridge_master *master,
      char *bridge_name);
  
  /* Callback nsm listener function that gets upcalled when a bridge gets
     deleted */
  int (*delete_bridge_func) (struct nsm_bridge_master *master,
      char *bridge_name);
  
  /* Callback L2 listener function that gets upcalled when a interface
     gets added to a bridge */
  int (*add_port_to_bridge_func)(struct nsm_bridge_master *master,
      char *bridge_name, struct interface *ifp);
  
  /* Callback L2 listener function that gets upcalled when a interface
     gets deleted from bridge */
  int (*delete_port_from_bridge_func) (struct nsm_bridge_master *master,
      char *bridge_name, struct interface *ifp);

  /* Callback L2 listener function that gets upcalled when a interface
     gets enabled on a bridge */
  int (*enable_port_func)(struct nsm_bridge_master *master,
      struct interface *ifp);
  
  /* Callback L2 listener function that gets upcalled when a interface
     gets disabled */
  int (*disable_port_func) (struct nsm_bridge_master *master,
      struct interface *ifp);

  /* Callback L2 listener function that gets upcalled when a interface
     state changes */
  int (*change_port_state_func) (struct nsm_bridge_master *master,
      struct interface *ifp);

  /* Callback L2 listener function that gets upcalled when a topology
     change notification is received */
  int (*topology_change_notify_func) (struct nsm_bridge_master *master,
      char *bridge_name);

/* Callback L2 listener function that gets upcalled when a port
    is added to bridge on which gmrp/gvrp is already confiured. */
  void (*activate_port_config_func) (struct nsm_bridge_master *master,
      struct interface *ifp);

  /* Callback L2 listener function that gets upcalled when a static mac
    address is added to bridge on which gmrp is confiured. */
  void (*add_static_mac_addr_func) (struct nsm_bridge_master *master,
       char *name, struct interface *ifp, unsigned char *mac,
       u_int16_t vid, u_int16_t svid, bool_t is_forward);

/* Callback L2 listener function that gets upcalled when a static mac
    address is deletd from a bridge on which gmrp is confiured. */
  void (*delete_static_mac_addr_func) (struct nsm_bridge_master *master,
       char *name, struct interface *ifp, unsigned char *mac,
       u_int16_t vid, u_int16_t svid, bool_t is_forward);


};

/* Function prototypes. */
struct nsm_bridge_master * nsm_bridge_get_master (struct nsm_master *nm);
int nsm_l2_init (struct lib_globals *zg);
int nsm_l2_deinit (struct lib_globals *zg);
int nsm_bridge_init (struct nsm_master *nm);
int nsm_bridge_deinit (struct nsm_master *nm);
struct nsm_bridge *nsm_lookup_bridge_by_name (struct nsm_bridge_master *master, const char * const name);
#ifdef HAVE_VLAN
struct nsm_bridge *nsm_lookup_bridge_by_vlan_id (struct nsm_master *, int, u_char);
#endif /* HAVE_VLAN */
struct nsm_bridge *nsm_get_first_bridge (struct nsm_bridge_master *master);
struct nsm_bridge *nsm_get_default_bridge (struct nsm_bridge_master *master);
struct nsm_bridge *nsm_get_next_bridge (struct nsm_bridge_master *master,
                                        struct nsm_bridge *bridge);
int nsm_bridge_add (struct nsm_bridge_master *master, u_char type, char *name,
                    bool_t is_default, u_char topology_type, u_int8_t edge, 
                    u_int8_t beb, u_char *macaddr);
int nsm_bridge_delete (struct nsm_bridge_master *master, char *name);

int
nsm_bridge_port_add (struct nsm_bridge_master *master, char *name,
                     struct interface *ifp, bool_t notify_kernel,
                     u_int8_t spanning_tree_disable);

int
nsm_bridge_change_type (struct nsm_bridge_master *master,
                        char *name, u_char type, u_char topo_type);

int
nsm_map_bridge_type_to_hal_protocol (u_char type);

/* Allocate new bridge. */
struct nsm_bridge *
nsm_bridge_new (u_char type, char *name, char is_default,
                u_char topology_type, u_int8_t edge, u_int8_t beb, 
                u_char *macaddr);

int nsm_bridge_cleanup (struct nsm_bridge_master *master,
                        struct nsm_bridge *bridge);

/* Link new bridge. */
void
nsm_link_bridge (struct nsm_bridge_master *master, struct nsm_bridge *new);

int nsm_bridge_ageing_time_set(struct nsm_bridge_master *master, 
                               char *name, u_int32_t ageing_time, 
                               bool_t notify_pm);
int nsm_bridge_set_state (struct nsm_bridge_master *master, char *name,
                          u_int16_t enable);
int nsm_bridge_set_learning(struct nsm_bridge_master *master, char *name);
int nsm_bridge_unset_learning(struct nsm_bridge_master *master, char *name);
void nsm_bridge_show(struct cli *cli, int type);
bool_t nsm_bridge_is_vlan_aware (struct nsm_bridge_master *master, char *name);
bool_t nsm_bridge_is_igmp_snoop_enabled (struct nsm_bridge_master *master, char *name);
bool_t nsm_bridge_is_gmrp_enabled (struct nsm_bridge_master *master, char *name);
int 
nsm_bridge_api_port_spanning_tree_enable (struct nsm_bridge_master *master, 
                                          char *name, struct interface *ifp, 
                                          u_int8_t spanning_tree_mode);
int nsm_bridge_api_port_add (struct nsm_bridge_master *master, char *name,
                             struct interface *ifp, bool_t iterate_members,
                             bool_t notify_kernel, bool_t exp_bridge_grpd,
                             u_int8_t spanning_tree_disable);
int nsm_bridge_port_delete (struct nsm_bridge_master *master, char *name,
                            struct interface *ifp, bool_t iterate_members,
                            bool_t notify_kernel);
struct interface * nsm_lookup_port_by_index (struct nsm_bridge *bridge, u_int32_t ifindex);

struct nsm_bridge_port *
nsm_bridge_port_lookup_by_index (struct nsm_bridge *bridge,
                                 u_int32_t ifindex);

struct interface *nsm_get_first_bridge_port (struct nsm_bridge *bridge);
struct interface *nsm_get_next_bridge_port (struct nsm_bridge *bridge, struct interface *ifp);

int nsm_bridge_add_mac(struct nsm_bridge_master *master, char *name, struct interface *ifp, u_int8_t *mac,
                       u_int16_t vid, u_int16_t svid, unsigned char flags, bool_t is_forward, int type);

int nsm_is_mac_present (struct nsm_bridge *bridge, u_int8_t *mac_addr,
                        u_int16_t vid, u_int16_t svid, int type, int ifindex);

int nsm_bridge_delete_mac(struct nsm_bridge_master *master, char *name,
                          struct interface *ifp, u_int8_t *mac, u_int16_t vid,
                          u_int16_t svid, unsigned char flags, bool_t is_forward,
                          int type);

int nsm_bridge_sync (struct nsm_master *nm, struct nsm_server_entry *nse);

struct nsm_bridge_port *
nsm_bridge_if_init_switchport (struct nsm_bridge *bridge,
                               struct interface *ifp, u_int16_t vid);

int nsm_bridge_if_set_switchport (struct interface *ifp);

int nsm_bridge_if_unset_switchport (struct interface *ifp);

int nsm_bridge_if_delete (struct nsm_bridge *bridge, struct interface *ifp,
                          u_int8_t remove_avl_node);

int nsm_stp_set_server_callback (struct nsm_server *ns);

int nsm_bridge_set_server_callback (struct nsm_server *ns);

int nsm_bridge_gmrp_service_req (struct nsm_bridge_master *master,
                                 char *name, struct interface *ifp, bool_t fwdall);

int
nsm_add_mac_to_static_list (struct nsm_bridge *bridge, struct ptree *list,
                            u_int32_t ifindex, u_int8_t *mac,
                            bool_t is_forward, int type, u_int16_t vid);
struct nsm_bridge_fdb_ifinfo *
nsm_bridge_fdb_ifinfo_lookup (struct ptree *static_fdb_list,
                              u_char *mac_addr,
                              u_int32_t ifindex);

#ifdef HAVE_L2
void nsm_bridge_if_send_state_sync_req_wrap ( struct interface *ifp);
#endif /* HAVE_L2 */


#ifdef HAVE_G8032
s_int16_t
nsm_bridge_g8032_create_vlan (u_int16_t ring_vid,
                              struct nsm_msg_g8032_vlan *pg_instance,
                              u_int8_t  primary,
                              struct nsm_master *nm );

int
nsm_g8032_send_vlan_group (struct nsm_msg_g8032_vlan *msg, int msgid );


struct nsm_g8032_ring *
nsm_g8032_find_ring_by_id (struct nsm_bridge * bridge, u_int16_t ring_id);


int nsm_bridge_g8032_create_ring (u_int8_t * br_name, u_int8_t * ifname_w,
                                 u_int8_t * ifname_p, u_int16_t instance,
                                 u_int16_t eps_id,
                                 struct nsm_master *nm);

int nsm_bridge_g8032_delete_ring (struct nsm_bridge * bridge,
                                 struct nsm_msg_g8032 *msg, int msgid );

s_int16_t
nsm_g8032_delete_ring (u_int8_t * br_name, u_int16_t ring_id,
                         struct nsm_master *nm);


int nsm_g8032_send_ring_msg (struct nsm_bridge * bridge,
                            struct nsm_msg_g8032 *msg, int msgid);

s_int16_t nsm_g8032_find_vlan_ring (struct avl_tree * raps_tree,
          	                   u_int16_t ring_vid );

s_int16_t
nsm_bridge_set_g8032_port_state (struct nsm_bridge *bridge,
                                 struct nsm_msg_bridge_g8032_port *msg);

s_int16_t
nsm_g8032_vlan_config_exists( struct nsm_bridge *bridge,
                              u_int16_t vid);
 
int
nsm_find_vlan_common_if( struct nsm_bridge *bridge,
    u_int32_t ifp_w,
    u_int32_t ifp_p,
    u_int8_t vid
    );
s_int16_t
nsm_g8032_sync (struct nsm_master *nm, struct nsm_server_entry *nse);

#endif  /*HAVE_G8032*/
/* Config store and activate */
extern struct nsm_bridge_config *nsm_bridge_config_new (
    struct nsm_bridge_master * master, char *bridge_name);
extern void nsm_bridge_config_link (struct nsm_bridge_master *master,
    char *br_name, struct nsm_bridge_config *new_br_conf);
extern struct nsm_bridge_config *nsm_bridge_config_find (
    struct nsm_bridge_master *master, char *bridge_name);
extern void nsm_bridge_config_free ();
extern void nsm_bridge_config_activate (struct nsm_bridge_master *master,
    char* br_name, int vid);

/* Bridge Listener interface */
int nsm_add_listener_to_bridgelist(struct list *listener_list,
    struct nsm_bridge_listener *appln);

void nsm_remove_listener_from_bridgelist(struct list *listener_list,
    nsm_listener_id_t appln_id);

int nsm_create_bridge_listener(struct nsm_bridge_listener **appln);

void nsm_destroy_bridge_listener(struct nsm_bridge_listener *appln);

void
nsm_bridge_delete_port_fdb_by_vid (struct nsm_bridge *bridge, struct nsm_if *zif,
                                   u_int16_t vid);
void
nsm_bridge_delete_all_fdb_by_port (struct nsm_bridge *bridge,
                                   struct nsm_if *zif, int hw_del_only);

void
nsm_bridge_add_static_fdb (struct nsm_bridge *bridge, u_int32_t ifindex);

void
nsm_bridge_free_static_list ( struct ptree *list );

typedef int (*nsm_bridge_delete_mac_func_t) 
                            (struct nsm_bridge_static_fdb *curr,
                             struct nsm_bridge_static_fdb *cmp);

typedef int (*nsm_bridge_delete_mac_from_port_func_t) 
                            (struct nsm_bridge_fdb_ifinfo *curr, 
                             struct nsm_bridge_fdb_ifinfo *cmp);

typedef void 
(*nsm_display_static_mac_t) (struct cli *cli, char *bridge_name,
                             u_int16_t vid, u_int16_t svid,
                             struct nsm_bridge_static_fdb *static_fdb,
                             struct nsm_bridge_fdb_ifinfo *pInfo); 

void
nsm_show_running_display (struct cli *cli, char *bridge_name,
                          u_int16_t vid, u_int16_t svid,
                          struct nsm_bridge_static_fdb *static_fdb,
                          struct nsm_bridge_fdb_ifinfo *pInfo);

void
nsm_pri_ovr_show_running_display (struct cli *cli, char *bridge_name,
                                  u_int16_t vid, u_int16_t svid,
                                  struct nsm_bridge_static_fdb *static_fdb,
                                  struct nsm_bridge_fdb_ifinfo *pInfo);

int
nsm_bridge_set_port_state (struct nsm_bridge_master *master, char *name,
                           struct interface *ifp, u_int32_t state,
                           u_int32_t instance);

int nsm_bridge_master_deinit (struct nsm_bridge_master **master);

void nsm_show_cp_fdb_entries ( struct cli *cli,
                               struct nsm_bridge *bridge,
                               nsm_display_static_mac_t display_static_mac);

void
nsm_bridge_delete_port_fdb_by_vid_svid (struct nsm_bridge *bridge, struct nsm_if *zif,
                                        u_int16_t cvid, u_int16_t svid);

u_int8_t * nsm_get_bridge_str (struct nsm_bridge *br);

u_int8_t nsm_bridge_str_to_type (u_int8_t *str);

u_int8_t * nsm_bridge_type_to_str (u_int8_t type);

int nsm_bridge_clear_static_mac (struct nsm_bridge_master *master, char *name);

int nsm_bridge_clear_multicast_mac (struct nsm_bridge_master *master, char *name);

int nsm_bridge_clear_dynamic_mac_vlan (struct nsm_bridge_master *master, u_int16_t vid, char *bridge_name);

int nsm_bridge_clear_dynamic_mac_port (struct nsm_bridge_master *master, 
                                       struct interface *ifp, char *bridge_name, int instance);

int nsm_bridge_clear_dynamic_mac_addr (struct nsm_bridge_master *master, u_int8_t *mac_addr,
                                       char *bridge_name);

int nsm_bridge_clear_dynamic_mac_bridge (struct nsm_bridge_master *master,
                                         char *bridge_name);

int nsm_bridge_clear_static_mac_port (struct nsm_bridge_master *master, char *bridge_name, struct interface *ifp);

int nsm_bridge_clear_static_mac_vlan (struct nsm_bridge_master *master, char *bridge_name, u_int16_t vid);

int nsm_bridge_clear_static_mac_pro_edge_vlan
                                (struct nsm_bridge_master *master,
                                 char *bridge_name,
                                 u_int16_t vid,
                                 u_int16_t svid);

int nsm_bridge_clear_multicast_mac_port (struct nsm_bridge_master *master, char *bridge_name, struct interface *ifp);

int nsm_bridge_clear_multicast_mac_vlan (struct nsm_bridge_master *master, char *bridge_name, u_int16_t vid);

int nsm_bridge_clear_multicast_mac_pro_edge_vlan
                                   (struct nsm_bridge_master *master,
                                    char *bridge_name, u_int16_t vid,
                                    u_int16_t svid);

int nsm_bridge_clear_static_mac_addr (struct nsm_bridge_master *master, char *bridge_name,
                                      u_int8_t *mac_addr);

int nsm_bridge_clear_multicast_mac_addr (struct nsm_bridge_master *master, char *bridge_name,
                                         u_int8_t *mac_addr);

enum hal_l2_proto nsm_map_str_to_hal_protocol (u_int8_t *proto_str);

enum hal_l2_proto_process
nsm_map_str_to_hal_protocol_process (u_int8_t *process_str);

int
nsm_bridge_api_port_proto_process (struct interface *ifp,
                                   enum hal_l2_proto proto,
                                   enum hal_l2_proto_process process,
                                   bool_t iterate_members,
                                   u_int16_t svid);

void nsm_bridge_static_fdb_config_free (struct interface *ifp);

s_int32_t
nsm_bridge_flush_fdb_by_port (struct nsm_master *master, char *ifname);

s_int32_t
nsm_bridge_flush_fdb_by_instance (struct nsm_master *master, char *ifname, int instance);


struct nsm_bridge_static_fdb_config *nsm_bridge_static_fdb_config_add 
                  (char *bridge_name, struct interface *ifp,
                  u_int8_t *mac_addr, u_int16_t vid, bool_t is_forward,
                  enum nsm_bridge_pri_ovr_mac_type ovr_mac_type,
                  u_int8_t priority);

void nsm_bridge_static_fdb_config_activate (struct nsm_bridge_master *master, char *bridge_name, u_int16_t vid, struct interface *ifp);

void nsm_bridge_if_add_send_notification (struct nsm_bridge *bridge, struct interface *ifp);

int nsm_check_duplicate_unicast_mac (struct ptree *list, u_int32_t ifindex, u_int8_t *mac);

int nsm_bridge_api_set_port_state_all (struct nsm_master *nm, char *name, struct interface *ifp, u_int32_t state);

void nsm_bridge_if_send_state_sync_req (struct nsm_bridge *bridge, struct interface *ifp);

int nsm_bridge_sync_multicast_entries (struct nsm_bridge *bridge, struct interface *ifp, u_int8_t add);

int
nsm_add_if_to_def_bridge (struct interface *ifp);

enum hal_l2_proto
nsm_map_str_to_hal_protocol (u_int8_t *proto_str);

enum hal_l2_proto_process
nsm_map_str_to_hal_protocol_process (u_int8_t *process_str);

u_int8_t *
nsm_map_hal_proto_process_to_str (enum hal_l2_proto_process process);

u_int8_t *
nsm_map_hal_proto_to_str (enum hal_l2_proto proto);

u_int8_t *
nsm_bridge_get_process_str (struct interface *ifp,
                            enum hal_l2_proto proto);

u_int16_t
nsm_bridge_get_process_vid (struct interface *ifp,
                            enum hal_l2_proto proto);

#ifdef HAVE_PBB_TE 
int 
nsm_bridge_decouple_vlan (struct nsm_bridge_master *master, char *brname,
                          u_int16_t start_vid, u_int16_t end_vid,
                          u_int8_t protocol);

int
nsm_bridge_remove_decoupling (struct nsm_bridge_master *master, char *brname, 
                              u_int16_t, u_int16_t, u_int8_t );

#endif /* HAVE_PBB_TE */

#if defined HAVE_PBB_TE || defined  HAVE_G8031 || defined HAVE_G8032

void
nsm_put_instance (u_int8_t map[], u_int16_t);

u_int16_t
nsm_get_instance (struct nsm_bridge *bridge, u_int8_t map[]);

int
nsm_send_vlan_update_msg (struct nsm_bridge *bridge,
                          struct nsm_msg_vlan *msg,
                          int msgid);
#endif /*defined HAVE_PBB_TE || defined  HAVE_G8031 || defined HAVE_G8032 */


#ifdef HAVE_ONMD
int
nsm_server_recv_req_ifindex (struct nsm_msg_header *header,
                             void *arg, void *message);

int
nsm_cfm_send_ifindex (struct nsm_server_entry *nse,
                      struct nsm_msg_cfm_ifindex *msg, u_int32_t msg_id);
#endif /* HAVE_ONMD */

int
nsm_bridge_add_mac_prio_ovr (struct nsm_bridge_master *master, char *name,
                             struct interface *ifp, u_int8_t *mac,
                             u_int16_t vid,
                             enum nsm_bridge_pri_ovr_mac_type ovr_mac_type,
                             u_int8_t priority);
int
nsm_bridge_del_mac_prio_ovr (struct nsm_bridge_master *master, char *name,
                             struct interface *ifp, u_int8_t *mac,
                             u_int16_t vid);

enum nsm_bridge_pri_ovr_mac_type
nsm_bridge_str_to_ovr_mac_type (char *str);

#define NSM_PRIO_CLI_MIN    0
#define NSM_PRIO_CLI_MAX    7

#ifdef HAVE_SMI

int
nsm_api_bridge_get_type (struct nsm_bridge_master *master,
                         char * name, enum smi_bridge_type *type,
                         enum smi_bridge_topo_type *topo_type);


int
nsm_bridge_api_get_port_proto_process (struct interface *ifp,
                                       enum smi_bridge_proto proto,
                                       enum smi_bridge_proto_process *process);
int
nsm_set_sw_reset (void); 

#endif /* HAVE_SMI */

#ifdef HAVE_G8031
struct g8031_protection_group
{
   u_int32_t ifindex_w;
   u_int32_t ifindex_p;
   u_int16_t eps_id;
   u_int16_t g8031_instance_id;
   u_int16_t primary_vid;  
   u_int32_t instance;

   u_int8_t  pg_state;
#define G8031_PG_NOT_INITALIZED  0
#define G8031_PG_INITALIZED  1

   /* eps_vids will be pointers to existing struct nsm_vlan */
   struct avl_tree * eps_vids;
};

s_int16_t
nsm_create_g8031_protection_group ( u_int8_t * br_name,
                                    u_int8_t * ifname_w,
                                    u_int8_t * ifname_p,
                                    u_int16_t instance,
                                    u_int16_t eps_id,
                                    struct nsm_master *nm
                                  );

struct g8031_protection_group *
nsm_g8031_find_protection_group ( struct nsm_bridge * bridge,
                                  u_int16_t eps_id
                                );
 
s_int16_t
nsm_map_vlans_to_g8031_protection_group ( u_int16_t eps_vid,
                                          u_int8_t * br_name,
                                          u_int16_t eps_id,
                                          u_int8_t primary,
                                          struct nsm_master *nm
                                        );

s_int16_t
nsm_g8031_eps_vlan_find ( struct avl_tree * eps_tree,
                          u_int16_t eps_vid
                        );


s_int16_t
nsm_delete_g8031_protection_group ( u_int8_t * br_name,
                                    u_int16_t eps_id,
                                   struct nsm_master *nm 
                                  );


s_int16_t
nsm_g8031_vlan_config_exists( struct nsm_bridge *bridge,
                              u_int8_t vid
                            );

int
nsm_find_vlan_common_if( struct nsm_bridge *bridge,
                         u_int32_t ifp_w,
                         u_int32_t ifp_p,
                         u_int8_t vid
                       );

int
nsm_bridge_set_g8031_port_state (struct nsm_bridge *bridge,
                                 u_int16_t eps_id,
                                 u_int32_t ifindex_fwd,
                                 u_int32_t state_f,
                                 u_int32_t ifindex_blck,
                                 u_int32_t state_b
                                );

int 
nsm_g8031_sync (struct nsm_master *, struct nsm_server_entry *);
#endif /* HAVE_G8031 */

#if defined HAVE_G8031 || defined HAVE_G8032
int
protection_group_update_stp (u_int16_t instance,
                             struct interface *ifp_w,
                             struct interface *ifp_p,
                             u_int8_t spanning_tree_disable,
                             u_int8_t msg_id);

int
nsm_pg_send_instance_msg_to_mstp (struct nsm_bridge * bridge,
                                     struct nsm_msg_bridge_pg *msg,
                                     int msgid );

int
nsm_server_recv_block_protection_instance (struct nsm_msg_header *header,
                                           void *arg, void *message);
                                           
int
nsm_server_recv_unblock_protection_instance (struct nsm_msg_header *header,
                                             void *arg, void *message);

int
nsm_api_bridge_get_hw_instance (u_int16_t *);
#endif /* HAVE_G8031 || HAVE_G8032 */
#endif /*HAVE_CUSTOM1*/
#endif /* __NSM_BRIDGE_H__ */
