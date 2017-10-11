/* 
   Copyright (C) 2003  All Rights Reserved. 
   
   LAYER 2 BRIDGE
   
   This file declares the configurable values used by the 
   Multiple Spanning Tree Protocol based on IEEE 802.1s-2002
   Ammendment 3 to IEEE 802.1Q -1998
  
*/

#ifndef __PACOS_MSTP_CONFIG_H_
#define __PACOS_MSTP_CONFIG_H_

#define HAVE_L2GP

#define MSTP_TIMER_SCALE_FACT           256
#define ONE_SEC                         SECS_TO_TICS(1)
#ifdef HAVE_RPVST_PLUS
#define RPVST_MAX_INSTANCES             4096
#endif /* HAVE_RPVST_PLUS */
#define MST_MIN_INSTANCES               1
#define MST_MAX_INSTANCES               64
#define MSTP_TE_MSTID 			(MST_MAX_INSTANCES - 1)  

#define MST_INSTANCE_IST                0
#define MST_DEFAULT_VLAN                1

#define BRIDGE_PORT_DEFAULT_P2P         1
#define BRIDGE_PORT_DEFAULT_EDGE        0
#define BRIDGE_DEFAULT_PRIORITY         32768
#define CE_BRIDGE_DEFAULT_PRIORITY      61440
#define BRIDGE_DEFAULT_PORT_PRIORITY    128
#define CE_BRIDGE_DEFAULT_PORT_PRIORITY 32
#ifndef BEB_STP_TUNNELING
#define VIP_BRIDGE_DEFAULT_PORT_PRIORITY 32
#endif
#define MSTP_DEFAULT_HOP_COUNT          20

#define MSTP_CONFIG_PROPAGATION_DELAY   1

#define BITS_PER_PORTMAP_ENTRY          32

#define MAX_MSTI                        64 

/* Force version parameter possible values */
#define BR_VERSION_STP                  0
#define BR_VERSION_RSTP                 0x2
#define BR_VERSION_MSTP                 0x3
#define BR_DEF_VERSION                  BR_VERSION_MSTP
#define BR_VERSION_MAX                  BR_VERSION_MSTP
#define BR_VERSION_INVALID              1

/* Even though the maxmimum name size allowed is NPFBRIDGE_IFNAMSIZ (16)
   we need the last charachter as '\0' as we treat bridge name as string
   so we need to check for namesize -1 
*/
#define MSTP_CONFIG_NAME_LEN               32
#define MSTP_CONFIG_DIGEST_LEN             16

/* Valid bpdu types determined after validation */

#define MST_DEFAULT_MAXHOPS                20

#define BR_CONFIG_BPDU_FRAME_SIZE           38
#define BR_RST_BPDU_FRAME_SIZE              39
#define BR_MST_BPDU_MIN_FRAME_SIZE          102
#define BR_MST_MIN_VER_THREE_LEN            64
#define BR_RPVST_BPDU_FRAME_SIZE            50

/* The size that each instance will contribute */
#define MSTI_BPDU_FRAME_SIZE                16
#define MSTI_CISCO_BPDU_FRAME_SIZE          26
/* Although upto 64 mst instances are allowed we can
 * have a maximum of 16 instances and hence the max 
 * bpdu frame size will be 
 * 102 +16*num_max_instance = 102 +16*16 
 * Changed the MAX BPDU size to accomodate CISCO BPDU
 */

#define BR_MST_BPDU_MAX_FRAME_SIZE          \
BR_MST_BPDU_MIN_FRAME_SIZE + MSTI_CISCO_BPDU_FRAME_SIZE*MST_MAX_INSTANCES 

#define STP_TCN_BPDU_SIZE                    7



#define MSTP_BRIDGE_ID_LEN                  8

#define MSTP_MIGRATE_TIME                   3
#define MSTP_TX_HOLD_COUNT                  6
#define MSTP_BR_NO_PORT                     0


#define BR_STP_PROTOID 0
#define BR_RSTP_PROTOID 2
#define BR_VERSION_ONE_LEN 0
#define BR_CBPDU_MINLEN 35
#define BR_RSTBPDU_MINLEN 36



/* Minimum and Maximum Allowable values for various paramters */
#define MSTP_MIN_PATH_COST          1
#define MSTP_MAX_PATH_COST          200000000
#define MSTP_MIN_PORT_PRIORITY      0
#define MSTP_MAX_PORT_PRIORITY      240
#define MSTP_MIN_BRIDGE_PRIORITY    0
#define MSTP_MAX_BRIDGE_PRIORITY    61440
#define MSTP_MIN_BRIDGE_MAX_AGE     6
#define MSTP_MAX_BRIDGE_MAX_AGE     40
#define MSTP_MIN_BRIDGE_MAX_HOPS    1
#define MSTP_MAX_BRIDGE_MAX_HOPS    40
#define MSTP_MIN_BRIDGE_FWD_DELAY   4
#define MSTP_MAX_BRIDGE_FWD_DELAY   30
#define MSTP_MIN_BRIDGE_HELLO_TIME  1
#define MSTP_MAX_BRIDGE_HELLO_TIME  10
#define MSTP_MIN_BRIDGE_TX_HOLD_COUNT 1
#define MSTP_MAX_BRIDGE_TX_HOLD_COUNT 10

/* default portfast values */
#define MSTP_BRIDGE_ERRDISABLE_DEFAULT_TIMEOUT_INTERVAL SECS_TO_TICS(300)

/*Link Type Macro*/
#define MSTP_ADMIN_LINK_TYPE_AUTO   2

struct port_instance_info
{
  u_int8_t   config;
#define MSTP_IF_CONFIG_BRIDGE_INSTANCE_PRIORITY    (1 << 0)
#define MSTP_IF_CONFIG_BRIDGE_INSTANCE_PATHCOST    (1 << 1)
#define MSTP_IF_CONFIG_INSTANCE_RESTRICTED_ROLE    (1 << 2)
#define MSTP_IF_CONFIG_INSTANCE_RESTRICTED_TCN     (1 << 3)
  s_int32_t  instance;
  u_int32_t  port_instance_pathcost;
  s_int16_t  port_instance_priority;
  bool_t     restricted_role;
  bool_t     restricted_tcn;
#ifdef HAVE_RPVST_PLUS 
   u_int16_t  vlan_id;
#endif /* HAVE_RPVST_PLUS */
};

struct port_instance_list
{
  struct port_instance_list * next;
  struct port_instance_info instance_info;
};

struct mstp_interface {
  struct      interface *ifp;
  u_int16_t   config;
#define MSTP_IF_CONFIG_BRIDGE_GROUP                (1 << 0)
#define MSTP_IF_CONFIG_BRIDGE_PRIORITY             (1 << 1)
#define MSTP_IF_CONFIG_BRIDGE_PATHCOST             (1 << 2)
#define MSTP_IF_CONFIG_LINKTYPE                    (1 << 3)
#define MSTP_IF_CONFIG_PORTFAST                    (1 << 4)
#define MSTP_IF_CONFIG_FORCE_VERSION               (1 << 5)
#define MSTP_IF_CONFIG_BRIDGE_GROUP_INSTANCE       (1 << 6)
#define MSTP_IF_CONFIG_BPDUGUARD                   (1 << 7)
#define MSTP_IF_CONFIG_ROOTGUARD                   (1 << 8)
#define MSTP_IF_CONFIG_BPDUFILTER                  (1 << 9)
#define MSTP_IF_CONFIG_AUTOEDGE                    (1 << 10)
#define MSTP_IF_CONFIG_PORT_HELLO_TIME             (1 << 11)
#define MSTP_IF_CONFIG_RESTRICTED_ROLE             (1 << 12)
#define MSTP_IF_CONFIG_RESTRICTED_TCN              (1 << 13)
#ifdef HAVE_L2GP
#define MSTP_IF_CONFIG_L2GP_PORT                   (1 << 14)
#endif /* HAVE_L2GP */


  /* Bridge related configurations */
  char        bridge_group[L2_BRIDGE_NAME_LEN];
  int         bridge_priority;
  u_int32_t   bridge_pathcost;
  bool_t      port_p2p;
  bool_t      port_edge;
  bool_t      portfast_conf;
  u_char      version;
  struct      port_instance_list *instance_list;
  u_char      bpduguard;
  u_char      bpdu_filter;

  s_int32_t   hello_time;
  bool_t      restricted_role;
  bool_t      restricted_tcn;

#ifdef HAVE_L2GP
  u_char      isL2gp; /* 802.1ah-d4-1 13.25.21 */
  u_char      enableBPDUrx; /* 802.1ah-d4-1 13.25.18 */
  struct bridge_id  psuedoRootId; /* 802.1ah-d4-1 13.25.20 */
#endif /* HAVE_L2GP */ 

  bool_t      active;
};

struct mstp_bridge_instance_info {
  int instance;
  u_int16_t config;
  struct rlist_info *            vlan_list;
#define MSTP_BRIDGE_CONFIG_INSTANCE_PRIORITY (1 << 0)
#define MSTP_BRIDGE_CONFIG_INSTANCE          (1 << 1)
#define MSTP_BRIDGE_CONFIG_INSTANCE_PORT     (1 << 2) 
  u_int32_t priority; 
#ifdef HAVE_RPVST_PLUS
  int vlan_id;
#endif /* HAVE_RPVST_PLUS */
  struct list *port_list;
};

struct mstp_bridge_instance_list {
  struct mstp_bridge_instance_list *next;
  struct mstp_bridge_instance_info instance_info;
};

struct mstp_bridge_info {
  u_int32_t config;
#define MSTP_BRIDGE_CONFIG_AGEING_TIME            (1 << 0)
#define MSTP_BRIDGE_CONFIG_PRIORITY               (1 << 1)
#define MSTP_BRIDGE_CONFIG_FORWARD_TIME           (1 << 2)
#define MSTP_BRIDGE_CONFIG_MSTP_ENABLE            (1 << 3)
#define MSTP_BRIDGE_CONFIG_HELLO_TIME             (1 << 4)
#define MSTP_BRIDGE_CONFIG_MAX_AGE                (1 << 5)
#define MSTP_BRIDGE_CONFIG_ACQUIRE                (1 << 6)
#define MSTP_BRIDGE_CONFIG_MAX_HOPS               (1 << 7)
#define MSTP_BRIDGE_CONFIG_PORTFAST_BPDUGUARD     (1 << 8)
#define MSTP_BRIDGE_CONFIG_ERRDISABLE             (1 << 9)
#define MSTP_BRIDGE_CONFIG_ERRDISABLE_TIMEOUT     (1 << 10)
#define MSTP_BRIDGE_CONFIG_BPDUFILTER             (1 << 11)
#define MSTP_BRIDGE_CONFIG_REGION_NAME            (1 << 12)
#define MSTP_BRIDGE_CONFIG_REVISION_LEVEL         (1 << 13)
#define MSTP_BRIDGE_CONFIG_VLAN_INSTANCE          (1 << 14)
#define MSTP_BRIDGE_CONFIG_TX_HOLD_COUNT          (1 << 15)
#define MSTP_BRIDGE_CONFIG_FORCE_VERSION          (1 << 16)
#define MSTP_BRIDGE_CONFIG_SHUT_FORWARD           (1 << 17)
#define MSTP_BRIDGE_CONFIG_PATH_COST_METHOD       (1 << 18)

  char bridge_name[L2_BRIDGE_NAME_LEN];
  s_int32_t ageing_time;
  u_int32_t priority;
  s_int32_t force_version;
  s_int32_t forward_time;
  u_int8_t mstp_enable_br_type;
  bool_t cisco_interop;
  u_int32_t hello_time;
  s_int32_t max_age;
  struct static_fdb_list *fdb_list;
  bool_t acquire;
  s_int32_t max_hops;
  struct mstp_bridge_instance_list *instance_list;
  bool_t bpdugaurd;
  bool_t err_disable;
  s_int32_t errdisable_timeout_interval;
  char cfg_name[MSTP_CONFIG_NAME_LEN + 1];
  u_int16_t  cfg_revision_lvl;
  unsigned char transmit_hold_count;
  u_int8_t path_cost_method;

#ifdef HAVE_RPVST_PLUS
  u_int16_t vlan_instance_map[RPVST_MAX_INSTANCES];
#endif /* HAVE_RPVST_PLUS */

};

struct mstp_bridge_config_list {
  struct mstp_bridge_config_list *next;
  struct mstp_bridge_info br_info;
};

void mstp_activate_interface (struct mstp_interface *mstpif);
struct mstp_interface *mstp_interface_new (struct interface *ifp);
void mstpif_config_add_instance (struct mstp_interface *mstpif,
                                 struct port_instance_list *instance_list);
struct port_instance_info * mstpif_get_instance_info (
                                                      struct mstp_interface *mstpif,
                                                      s_int32_t instance);
extern void mstp_bridge_configuration_activate (char *bridge_name, u_char bridge_type);
extern void mstp_bridge_instance_config_add (char *bridge_name, int instance, u_int16_t vid);
extern void mstp_bridge_instance_activate (char *bridge_name, int vid);
extern struct mstp_bridge_config_list *mstp_get_bridge_config_list_head ();
extern struct mstp_bridge_info * mstp_bridge_config_info_get (char *br_name);
extern struct mstp_bridge_config_list *mstp_bridge_config_list_new (
                                                                    char *br_name);
extern void mstp_bridge_config_list_link (
                                          struct mstp_bridge_config_list *new_conf_list);
extern struct static_fdb_list * mstp_br_config_fdb_new ();
extern void mstp_br_config_link_fdb_new (struct mstp_bridge_info *br_info,
                                         struct static_fdb_list *fdb_new_list);
extern int mstp_bridge_add_static_mac (char *name, u_int32_t ifindex, char *mac,
                                       bool_t is_fwd, short vid);
extern struct mstp_bridge_instance_list * mstp_br_config_instance_new (int instance);
extern void mstp_br_config_link_instance_new (
                                              struct mstp_bridge_info *br_info,
                                              struct mstp_bridge_instance_list *instance_new_list);
extern struct mstp_bridge_instance_info *mstp_br_config_find_instance (
                                                                       struct mstp_bridge_info *br_info,
                                                                       int instance);
#endif
