/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _NSM_VLAN_H
#define _NSM_VLAN_H
#ifdef HAVE_HAL
#include "hal_types.h"
#endif /* HAVE_HAL */
#ifdef HAVE_PVLAN
#include "nsm_pvlan.h"
#endif /* HAVE_PVLAN */
#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

void nsm_vlan_cli_init (struct cli_tree *);
void nsm_vlan_access_cli_init (struct cli_tree *);

/* VLAN id. */
typedef u_int16_t nsm_vid_t;

#ifndef HAVE_CUSTOM1
#define NSM_VLAN_MAX                 4094              /* Max VLANs.   */
#define NSM_VLAN_ALL                 (NSM_VLAN_MAX + 1)/* All VLANs.   */
#define NSM_VLAN_NONE                0                 /* No VLANs.    */
#define NSM_VLAN_DEFAULT_VID         1                 /* Default VID. */

#define NSM_VLAN_CLI_MIN             2
#define NSM_VLAN_CLI_MAX             NSM_VLAN_MAX

#define NSM_VLAN_CLI_RNG             "<2-4094>"

/* VLAN bitmap manipulation macros. */
#define NSM_VLAN_BMP_WORD_WIDTH                32
#define NSM_VLAN_BMP_WORD_MAX                  \
        ((NSM_VLAN_MAX + NSM_VLAN_BMP_WORD_WIDTH) / NSM_VLAN_BMP_WORD_WIDTH)

struct nsm_vlan_bmp
{
  u_int32_t bitmap[NSM_VLAN_BMP_WORD_MAX];
}; 

#define NSM_VLAN_BMP_INIT(bmp)                                             \
   do {                                                                    \
       pal_mem_set ((bmp).bitmap, 0, sizeof ((bmp).bitmap));               \
   } while (0)
 
#define NSM_VLAN_BMP_SET(bmp, vid)                                         \
   do {                                                                    \
        int _word = (vid) / NSM_VLAN_BMP_WORD_WIDTH;                       \
        (bmp).bitmap[_word] |= (1U << ((vid) % NSM_VLAN_BMP_WORD_WIDTH));  \
   } while (0)

#define NSM_VLAN_BMP_UNSET(bmp, vid)                                       \
   do {                                                                    \
        int _word = (vid) / NSM_VLAN_BMP_WORD_WIDTH;                       \
        (bmp).bitmap[_word] &= ~(1U <<((vid) % NSM_VLAN_BMP_WORD_WIDTH));  \
   } while (0)

#define NSM_VLAN_BMP_IS_MEMBER(bmp, vid)                                   \
  ((bmp).bitmap[(vid) / NSM_VLAN_BMP_WORD_WIDTH] & (1U << ((vid) % NSM_VLAN_BMP_WORD_WIDTH)))

#define NSM_VLAN_SET_BMP_ITER_BEGIN(bmp, vid)                              \
    do {                                                                   \
        int _w, _i;                                                        \
        (vid) = 0;                                                         \
        for (_w = 0; _w < NSM_VLAN_BMP_WORD_MAX; _w++)                     \
          for (_i = 0; _i < NSM_VLAN_BMP_WORD_WIDTH; _i++, (vid)++)        \
            if ((bmp).bitmap[_w] & (1U << _i))

#define NSM_VLAN_SET_BMP_ITER_END(bmp, vid)                                \
    } while (0)

#define NSM_VLAN_UNSET_BMP_ITER_BEGIN(bmp, vid)                            \
    do {                                                                   \
        int _w, _i;                                                        \
        (vid) = 0;                                                         \
        for (_w = 0; _w < NSM_VLAN_BMP_WORD_MAX; _w++)                     \
          for (_i = 0; _i < NSM_VLAN_BMP_WORD_WIDTH; _i++, (vid)++)        \
            if ((bmp).bitmap[_w] & (1U << _i))

#define NSM_VLAN_UNSET_BMP_ITER_END(bmp, vid)                              \
    } while (0)

#define NSM_VLAN_BMP_CLEAR(bmp)         NSM_VLAN_BMP_INIT(bmp)

#define NSM_VLAN_BMP_SETALL(bmp)                                           \
   do {                                                                    \
        pal_mem_set ((bmp).bitmap, 0xff, sizeof ((bmp).bitmap));           \
   } while (0)

#define NSM_VLAN_BMP_AND(bmp1,bmp2)                                        \
   do {                                                                    \
        int _w;                                                            \
        for (_w = 0; _w < NSM_VLAN_BMP_WORD_MAX; _w++)                     \
           (bmp1).bitmap[_w] &= (bmp2).bitmap[_w];                         \
   } while (0)

#define NSM_VLAN_BMP_CMP(bmp1,bmp2)                                        \
   pal_mem_cmp ((bmp1)->bitmap, (bmp2)->bitmap,                            \
       NSM_VLAN_BMP_WORD_MAX *  sizeof (u_int32_t))
#define NSM_VLAN_SET_PORT_DEFAULT_MODE(ifp, iterate_members,               \
                                       notify_kernel)                      \
   do {                                                                    \
        struct nsm_if *_zif = (struct nsm_if *)(ifp)->info;                \
        struct nsm_bridge *_bridge;                                        \
        if (_zif)                                                          \
          {                                                                \
            _bridge = _zif->bridge;                                        \
            if (_bridge)                                                   \
              {                                                            \
                if (NSM_BRIDGE_TYPE_PROVIDER (_bridge))                    \
                  nsm_vlan_api_set_port_mode (ifp, NSM_VLAN_PORT_MODE_PN,  \
                                              NSM_VLAN_PORT_MODE_PN,       \
                                              iterate_members,             \
                                              notify_kernel);              \
                else if (_bridge->type == NSM_BRIDGE_TYPE_RPVST_PLUS)      \
                {                                                          \
                  nsm_vlan_api_set_port_mode (ifp,                         \
                                              NSM_VLAN_PORT_MODE_TRUNK,    \
                                              NSM_VLAN_PORT_MODE_TRUNK,    \
                                              iterate_members,             \
                                              notify_kernel);              \
                }                                                          \
                else                                                       \
                  nsm_vlan_api_set_port_mode (ifp,                         \
                                              NSM_VLAN_PORT_MODE_ACCESS,   \
                                              NSM_VLAN_PORT_MODE_ACCESS,   \
                                              iterate_members,             \
                                              notify_kernel);              \
              }                                                            \
          }                                                                \
   } while (0)


#define NSM_VLAN_BRIDGE_GET_CE_BR_NAME(ifindex)                            \
        pal_snprintf (bridge_name, L2_BRIDGE_NAME_LEN, "%s%d", "CE-",      \
                      ifindex);


#ifdef HAVE_VLAN_CLASS

#define NSM_VLAN_CLASS_MAX                           256
/* VLAN Classifier bitmap manipulation macros. */
#define NSM_VLAN_CLASS_BMP_WORD_WIDTH                32
#define NSM_VLAN_CLASS_BMP_WORD_MAX                  \
        ((NSM_VLAN_CLASS_MAX + NSM_VLAN_CLASS_BMP_WORD_WIDTH - 1) / NSM_VLAN_CLASS_BMP_WORD_WIDTH)

struct nsm_vlan_class_bmp
{
  u_int32_t bitmap[NSM_VLAN_CLASS_BMP_WORD_MAX];
};

#define NSM_VLAN_CLASS_BMP_INIT(bmp)                                       \
   do {                                                                    \
       pal_mem_set ((bmp).bitmap, 0, sizeof ((bmp).bitmap));            \
   } while (0)

#define NSM_VLAN_CLASS_BMP_SET(bmp, classid)                               \
   do {                                                                    \
        int _word = (classid) / NSM_VLAN_CLASS_BMP_WORD_WIDTH;             \
        (bmp).bitmap[_word] |=                                             \
                     (1U << ((classid) % NSM_VLAN_CLASS_BMP_WORD_WIDTH));  \
   } while (0)

#define NSM_VLAN_CLASS_BMP_UNSET(bmp, classid)                             \
   do {                                                                    \
        int _word = (classid) / NSM_VLAN_CLASS_BMP_WORD_WIDTH;             \
        (bmp).bitmap[_word] &=                                             \
                     ~(1U <<((classid) % NSM_VLAN_CLASS_BMP_WORD_WIDTH));  \
   } while (0)

#define NSM_VLAN_CLASS_BMP_IS_MEMBER(bmp, classid)                         \
  ((bmp).bitmap[(classid) / NSM_VLAN_CLASS_BMP_WORD_WIDTH] &               \
                  (1U << ((classid) % NSM_VLAN_CLASS_BMP_WORD_WIDTH)))

#define NSM_VLAN_CLASS_SET_BMP_ITER_BEGIN(bmp, classid)                    \
    do {                                                                   \
        int _w, _i;                                                        \
        (classid) = 0;                                                     \
        for (_w = 0; _w < NSM_VLAN_CLASS_BMP_WORD_MAX; _w++)               \
          for (_i = 0; _i < NSM_VLAN__CLASS_BMP_WORD_WIDTH; _i++, (classid)++) \
            if ((bmp).bitmap[_w] & (1U << _i))

#define NSM_VLAN_CLASS_SET_BMP_ITER_END(bmp, classid)                      \
    } while (0)

#define NSM_VLAN_CLASS_UNSET_BMP_ITER_BEGIN(bmp, classid)                  \
    do {                                                                   \
        int _w, _i;                                                        \
        (classid) = 0;                                                     \
        for (_w = 0; _w < NSM_VLAN_CLASS_BMP_WORD_MAX; _w++)               \
          for (_i = 0; _i < NSM_VLAN_CLASS_BMP_WORD_WIDTH; _i++, (classid)++) \
            if ((bmp).bitmap[_w] & (1U << _i))

#define NSM_VLAN_CLASS_UNSET_BMP_ITER_END(bmp, vid)                        \
    } while (0)

#define NSM_VLAN_CLASS_BMP_CLEAR(bmp)     NSM_VLAN_CLASSBMP_INIT(bmp)

#define NSM_VLAN_CLASS_BMP_SETALL(bmp)                                     \
   do {                                                                    \
        pal_mem_set ((bmp).bitmap, 0xff, sizeof ((bmp).bitmap));           \
   } while (0)

#define NSM_VLAN_CLASS_BMP_AND(bmp1,bmp2)                                  \
   do {                                                                    \
        int _w;                                                            \
        for (_w = 0; _w < NSM_VLAN_CLASS_BMP_WORD_MAX; _w++)               \
           (bmp1).bitmap[_w] &= (bmp2).bitmap[_w];                         \
   } while (0)

#endif /* HAVE_VLAN_CLASS */

/* VLAN State. */
enum nsm_vlan_state
  {
    NSM_VLAN_INVALID,
    NSM_VLAN_DISABLED,
    NSM_VLAN_ACTIVE
  };

/* VLAN port mode. */
enum nsm_vlan_port_mode
  {
    NSM_VLAN_PORT_MODE_INVALID,
    NSM_VLAN_PORT_MODE_ACCESS,
    NSM_VLAN_PORT_MODE_HYBRID,
    NSM_VLAN_PORT_MODE_TRUNK,
    NSM_VLAN_PORT_MODE_CE,
    NSM_VLAN_PORT_MODE_CN,
    NSM_VLAN_PORT_MODE_PN,
    NSM_VLAN_PORT_MODE_PE,
    NSM_VLAN_PORT_MODE_CNP,
    NSM_VLAN_PORT_MODE_VIP,
    NSM_VLAN_PORT_MODE_PIP,
    NSM_VLAN_PORT_MODE_CBP
  };

#define   NSM_VLAN_PORT_MODE_PNP NSM_VLAN_PORT_MODE_PN

struct nsm_vlan_listener
{
  /* Holds nsm listener id */
  nsm_listener_id_t listener_id;

  /* Flags that determine when the callbacks get called */
  /* Based on vlan type */
  int flags;

  /* Callback nsm listener function that gets upcalled when a vlan gets
     added to a bridge */
  int (*add_vlan_to_bridge_func) (struct nsm_bridge_master *master, char *bridge_name, u_int16_t vid);

  /* Callback nsm listener function that gets upcalled when vlan get 
     removed from a bridge */
  int (*delete_vlan_from_bridge_func) (struct nsm_bridge_master *master, char *bridge_name, u_int16_t vid);

#ifdef HAVE_PROVIDER_BRIDGE

  int (*add_pro_edge_swctx_to_bridge) (struct nsm_bridge_master *master,
       char *bridge_name, u_int16_t cvid, u_int16_t svid);

  int (*delete_pro_edge_swctx_from_bridge) (struct nsm_bridge_master *master,
       char *bridge_name, u_int16_t cvid, u_int16_t svid);

#endif /* HAVE_PROVIDER_BRIDGE */

  /* Callback L2 listener function that gets upcalled when a vlan gets
     added to a interface */
  int (*add_vlan_to_port_func)(struct nsm_bridge_master *master,
      char *bridge_name, struct interface *ifp, u_int16_t vid);

  /* Callback L2 listener function that gets upcalled when gets removed
     from a interface */
  int (*delete_vlan_from_port_func) (struct nsm_bridge_master *master,
      char *bridge_name, struct interface *ifp, u_int16_t vid);

#ifdef HAVE_PROVIDER_BRIDGE

  int (*add_port_to_swctx_func) (struct nsm_bridge_master *master,
       char *bridge_name, struct interface *ifp, u_int16_t cvid, u_int16_t svid);

  int (*del_port_from_swctx_func) (struct nsm_bridge_master *master,
       char *bridge_name, struct interface *ifp, u_int16_t cvid, u_int16_t svid);

#endif /* HAVE_PROVIDER_BRIDGE */

  /* Callback L2 listener function that gets upcalled when mode of the
     port changes */
  int (*change_port_mode_func) (struct nsm_bridge_master *master,
      char *bridge_name, struct interface *ifp);
};

#ifdef HAVE_PVLAN
union nsm_pvlan_vid
{
  nsm_vid_t isolated_vid;
  nsm_vid_t primary_vid;
};

struct nsm_pvlan_info
{
  struct nsm_vlan_bmp secondary_vlan_bmp;
  union nsm_pvlan_vid vid;
};
#endif /* HAVE_PVLAN */

#ifdef HAVE_HA
struct nsm_vlan_port_cdr_ref_node
{
  /* port ifindex */
  u_int32_t ifindex;

  /* cdr ref */
  HA_CDR_REF nsm_vlan_port_associate_cdr_ref;
};
#endif /* HAVE_HA */

/* VLAN information. */
struct nsm_vlan
{
#define NSM_VLAN_NAME_MAX   16
  /* VLAN name. */
  char                      vlan_name[NSM_VLAN_NAME_MAX + 1];    

  /* Back pointer to bridge */
  struct nsm_bridge         *bridge;

  /* VID */
  nsm_vid_t                 vid;

  /* Type. */
  u_int16_t type;
#define NSM_VLAN_STATIC     (1 << 0)
#define NSM_VLAN_DYNAMIC    (1 << 1)
#define NSM_VLAN_CVLAN      (1 << 2)
#define NSM_VLAN_SVLAN      (1 << 3)
#define NSM_SVLAN_P2P       (1 << 4)
#define NSM_SVLAN_M2M       (1 << 5)
#ifdef HAVE_B_BEB
#define NSM_VLAN_BVLAN      (1 << 6)
#define NSM_BVLAN_P2P       (1 << 7)
#define NSM_BVLAN_M2M       (1 << 8)
#endif /* HAVE_I_BEB */
#ifdef HAVE_PBB_TE
#define NSM_VLAN_TEVLAN     (1 << 9)
#endif
#define NSM_VLAN_AUTO       (1 << 10)

  struct interface *ifp;

#ifdef HAVE_PVLAN
  int pvlan_configured;

  int pvlan_type;

  /*struct list *secondary_vlan_list;*/
  struct nsm_pvlan_info pvlan_info;
#endif /* HAVE_PVLAN */

  u_int8_t *evc_id; ///< EVC ID is the id for svlan as per MEF 10.1

#ifdef HAVE_PROVIDER_BRIDGE
  
  struct nsm_vlan_bmp *cvlanMemberBmp;
  u_int8_t preserve_ce_cos;
  
  u_int16_t max_uni; ///<MAX UNI Per EVC as per MEF 10.1, Table 16

#endif /* HAVE_PROVIDER_BRIDGE */

  /* VLAN state. */
  enum nsm_vlan_state       vlan_state;

  /* VLAN Mtu. */

  u_int32_t mtu_val; 

  /* VLAN port list. */
  struct list               *port_list;

  /* Forbidden port list for the vlan */
  struct list               *forbidden_port_list;

#ifdef HAVE_L2LERN
  struct nsm_vlan_access_list   *vlan_filter;
#endif /* HAVE_L2LERN */

 /* Static FDB List */
  struct ptree              *static_fdb_list;
  
  int instance;
#ifdef HAVE_HA
  HA_CDR_REF                nsm_vlan_cdr_ref;
  struct list               *port_cdr_ref_list;
#endif /* HAVE_HA */

#ifdef HAVE_PBB_TE
  /* The set of ports to which frames received from a specific port
  and destined for a specific unicast address must be forwarded */
  struct list *unicast_egress_ports;
  /* The set of ports to which frames received from a specific port
  and destined for a specific unicast address must not be forwarded */
  struct list *unicast_forbidden_ports;
  /* The set of ports to which frames received from a specific port
  and destined for a specific Multicast or Broadcast MAC address must be forwarded */
  struct list *multicast_egress_ports;
  /* The set of ports to which frames received from a specific port
  and destined for a specific Multicast or Broadcast MAC address
  must not be forwarded*/
  struct list *multicast_forbidden_ports;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_QOS
#define NSM_VLAN_PRIORITY_CONF      (1 << 5)
#define NSM_VLAN_PRIORITY_MASK      (0x7)
#define NSM_VLAN_PRIORITY_NONE      (0x0)

  u_int8_t priority;
#endif /* HAVE_QOS */

#ifdef HAVE_SNMP
  pal_time_t create_time;
#endif /* HAVE_SNMP */
#ifdef HAVE_G8032
  /* Whether this vid is configured as a primary vid
   * for any of the g8032 rings 
   */
  u_int8_t primary_vid;
#endif /* HAVE_G8032 */


};

#ifdef HAVE_PVLAN
union nsm_pvlan_port_info
{
  /* For promiscuous port secondary_vlan_list, primary vid is pvid */
  struct nsm_vlan_bmp secondary_vlan_bmp;
  /* For host port primary_vid, secondary_vid is pvid itself */
  nsm_vid_t primary_vid;
};
#endif /* HAVE_PVLAN */

/* VLAN port information. */
struct nsm_vlan_port
{
  /* Port mode. */
  enum nsm_vlan_port_mode   mode;

  /* Port sub mode. */
  enum nsm_vlan_port_mode   sub_mode;
#ifdef HAVE_PVLAN
  /* Private VLAN port mode */
  enum nsm_pvlan_port_mode pvlan_mode;
  bool_t pvlan_configured;

  union nsm_pvlan_port_info pvlan_port_info;
#endif /* HAVE_PVLAN */

  /* Access port: access, Trunk port: native. */
  nsm_vid_t                 pvid;

  /* Native Vlan for Trunk Ports */
  nsm_vid_t                 native_vid;

#if defined HAVE_VLAN_STACK || defined HAVE_PROVIDER_BRIDGE
  char vlan_stacking;
  u_int16_t stack_mode;
#endif /* HAVE_VLAN_STACK || HAVE_PROVIDER_BRIDGE */
  u_int16_t tag_ethtype;

  struct nsm_cvlan_reg_tab *regtab;

  /* flags. */
  u_char flags;
#define NSM_VLAN_ENABLE_INGRESS_FILTER          (1 << 0)
#define NSM_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED   (1 << 1)
#define NSM_VLAN_ACCEPTABLE_FRAME_TYPE_UNTAGGED (1 << 2)

  /* Configuration. */
  u_char config;
#define NSM_VLAN_CONFIGURED_ALL                0
#define NSM_VLAN_CONFIGURED_NONE               1
#define NSM_VLAN_CONFIGURED_SPECIFIC           2

  /* Port configuration like traffic class, user priority */
  u_int16_t port_config;
#define NSM_PORT_CFG_TRF_CLASS         (1 << 0)
#define NSM_PORT_CFG_DEF_USR_PRI       (1 << 1)
#define NSM_PORT_CFG_REGEN_USR_PRI     (1 << 2)

  /* Default user priority */
  u_char default_user_priority;

#ifdef HAVE_HAL
  /* User Priority Regeneration Table */
  u_char user_prio_regn_table[HAL_BRIDGE_MAX_USER_PRIO + 1];

  /* Traffic Class Mapping Table */
  u_char traffic_class_table[HAL_BRIDGE_MAX_USER_PRIO + 1][HAL_BRIDGE_MAX_TRAFFIC_CLASS];
#endif /* HAVE_HAL */

  /* VLANs this port belongs to(static). */
  struct nsm_vlan_bmp staticMemberBmp;

  /* VLANs this port belongs to(dynamic). */
  struct nsm_vlan_bmp dynamicMemberBmp;

#ifdef HAVE_PROVIDER_BRIDGE
  /* S-VLANs this customer edge port belongs to. */
  struct nsm_vlan_bmp svlanMemberBmp;

  u_int16_t uni_default_evc;
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_MAC_AUTH
  /* AUTH MAC Dynamic Vlan */
  struct nsm_vlan_bmp authmacMemberBmp;
#endif /* HAVE_MAC_AUTH */

  /* Egress tagging per vlan bitmap. */
  struct nsm_vlan_bmp egresstaggedBmp;

#ifdef HAVE_VLAN_CLASS
  /* Vlan class bitmap per classification group for pvid */
  struct nsm_vlan_class_bmp classBmp;
#endif /* HAVE_VLAN_CLASS */

#if defined HAVE_PBB_TE
#if defined HAVE_GMPLS && defined HAVE_GELS
#if defined (HAVE_I_BEB) && defined (HAVE_B_BEB)
  /* These two structures would be allocated only if the port type is CBP
     The first map cbp_vlan_alloc_map1 = shows whether the vlan is available or NOT
     The second map cbp_vlan_alloc_map2 = shows whether the vlan is in use by RSVP
     or use by the static TESI configuration */
  struct nsm_vlan_bmp *cbp_vlan_alloc_map1;
  struct nsm_vlan_bmp *cbp_vlan_alloc_map2;
#else
  /* The array would be used to store the number of references which this particular
     bridgeport has to a particular vlan. In case of GELS, the label is a combination 
     of <MAC-address, vlan>. Since there could be labels with different MAC's 
     and same vlan, we need to track this and delete the assosiation of this port 
     with the vlan only if all the GELS LSP's using the particular vlan is removed. 

     Currently this structure is kept as an array for all the vlans, this could be 
     optimised later, into as tree/list so that the nodes will be added as and when the 
     vlan is added to the bridge_port, instead of the vlan allocation for all the 
     vlans initially itself.
*/
  u_char vlan_use_ref[NSM_VLAN_MAX+2];
#endif /* HAVE_I_BEB && HAVE_B_BEB */
#endif /* HAVE_GMPLS && HAVE_GELS */
#endif /* HAVE_PBB_TE */

};

/* For vlan table key. */
struct prefix_vlan
{
  u_char family;
  u_char prefixlen;
  u_char pad1;
  u_char pad2;
  s_int32_t vid;
};

#define NSM_VLAN_SET(P,I)                                                 \
   do {                                                                      \
       pal_mem_set ((P), 0, sizeof (struct nsm_vlan));                    \
       (P)->vid = I;                                                      \
   } while (0)

#define VLAN_INFO_SET(R,V)                                                   \
   do {                                                                      \
      (R)->info = (V);                                                       \
   } while (0)

#define VLAN_INFO_UNSET(R)                                                   \
   do {                                                                      \
     (R)->info = NULL;                                                       \
   } while (0)

struct nsm_vlan_config_entry
{
  int vid;
};

/* Egress tagged enumeration. */
enum nsm_vlan_egress_type
  {
    NSM_VLAN_EGRESS_UNTAGGED = 0,
    NSM_VLAN_EGRESS_TAGGED   = 1
  };

#ifdef HAVE_LACPD
struct nsm_bridge_port_conf
{
  struct nsm_bridge *bridge;
  struct nsm_vlan_port vlan_port;
};
#endif /* HAVE_LACPD */

#define NSM_UNI_TYPE_ENABLE 1 
#define NSM_UNI_TYPE_DISABLE 0
#define NSM_ELMI_OPERATIONAL 1
#define NSM_CFM_OPERATIONAL 2
#define NSM_UNI_TYPE_DEFAULT 0

struct nsm_uni_type_status 
{
  u_int8_t uni_type; ///< The UNI Type of the interface,  By default, the UNI  
                     ///< Type is UNI_TYPE 1 

  #define NSM_UNI_TYPE_1     1
  #define NSM_UNI_TYPE_2     2

   u_int8_t uni_type_mode; ///<  This flag is mention whether the uni type mode
            ///< is enabled/disabled. By default, the uni type mode is disabled

   u_int8_t uni_type_2_status; ///< UNI TYPE 2 mode, type 2.2 or type 2.1

  #define NSM_UNI_TYPE_TWO_ONE   3
  #define NSM_UNI_TYPE_TWO_TWO  4
     
   u_int8_t cfm_status;  ///< Field to maintain UNI MEG status, as per 
                         ///<  MEF 20 section 8 [R15] [R16]
  
  /*Maintating Local MEP and Remote MEP operational status at NSM
   * to determine CFM operational status */ 
  u_int8_t local_uni_mep_status;
  
  u_int8_t remote_uni_mep_status; 
   
   u_int8_t elmi_status; ///<Field to maintatin e-lmi operational status as per
                         ///< MEF 20 section 8 [R9] [R10]
   
   u_int8_t link_oam_status; ///<Field to maitain link_oam_status,by default UP
};

#ifdef HAVE_PROVIDER_BRIDGE

enum nsm_uni_service_attr
{
  NSM_UNI_MUX_BNDL,
  NSM_UNI_MUX,
  NSM_UNI_BNDL,
  NSM_UNI_AIO_BNDL,
  NSM_UNI_SERVICE_MAX
};



struct nsm_bridge_protocol_process
{
#if defined HAVE_STPD || defined HAVE_RSTPD || defined HAVE_MSTPD
  enum hal_l2_proto_process stp_process;
  u_int16_t stp_tun_vid;
#endif /* HAVE_STPD || HAVE_RSTPD || HAVE_MSTPD */

#ifdef HAVE_GMRP
  enum hal_l2_proto_process gmrp_process;
#endif /* HAVE_GMRP */

#ifdef HAVE_MMRP
  enum hal_l2_proto_process mmrp_process;
#endif /* HAVE_MMRP */

#ifdef HAVE_GVRP
  enum hal_l2_proto_process gvrp_process;
  u_int16_t gvrp_tun_vid;
#endif /* HAVE_GVRP */

#ifdef HAVE_MVRP
  enum hal_l2_proto_process mvrp_process;
  u_int16_t mvrp_tun_vid;
#endif /* HAVE_MVRP */

#ifdef HAVE_LACPD
  enum hal_l2_proto_process lacp_process;
  u_int16_t lacp_tun_vid;
#endif /* HAVE_LACP */

#ifdef HAVE_AUTHD
  enum hal_l2_proto_process dot1x_process;
  u_int16_t dot1x_tun_vid;
#endif /* HAVE_AUTHD */

};

#define NSM_INGRESS_POLICING 1
#define NSM_EGRESS_SHAPING   2

#define NSM_UNI_DEFAULT      0

#define NSM_BW_PARAMETER_CIR    1
#define NSM_BW_PARAMETER_CBS    2
#define NSM_BW_PARAMETER_EIR    3
#define NSM_BW_PARAMETER_EBS    4
#define NSM_BW_PARAMETER_COUPLING_FLAG    5
#define NSM_BW_PARAMETER_COLOR_MODE       6

/* Structure to maintatin the bw profile parameters */
struct nsm_bw_profile_parameters
{
  u_int32_t comm_info_rate;   ///< Committed Information Rate (CIR) 
  
  u_int32_t comm_burst_size;  ///< Committed Burst Size (CBS)
  
  u_int32_t excess_info_rate; ///< Excess Information Rate (EIR)
  
  u_int32_t excess_burst_size; ///< Excess Burst Size  (EBS)

  u_int8_t coupling_flag;  ///< Coupling flag
  
  u_int8_t color_mode;  ///< Color mode
  
  u_int8_t active;   ///< FLAG to determine if the bandwidth profile is 
       ///< activated or not. active = 1(Activated); active = 0(Not activated).

  struct nsm_if *zif; ///< Backpointer to zif 
};

/* The structure maintated per EVC to store the BW Profile for the EVC */
struct nsm_uni_evc_bw_profile
{
  u_int16_t svlan; ///< svlan to which this uni evc bw profile is applied

  u_int8_t *evc_id; ///< EVC-ID

  u_int16_t instance_id; ///< INSTANCE ID

  struct interface *ifp; ///< backpointer to the interface

  u_int8_t ingress_bw_profile_type; ///< Per svlan or Per CoS
  u_int8_t egress_bw_profile_type;  ///< Per svlan or Per CoS 

#define NSM_EVC_BW_PROFILE_PER_EVC     1
#define NSM_EVC_COS_BW_PROFILE         2 

  struct nsm_bw_profile_parameters *ingress_evc_bw_profile;
  struct nsm_bw_profile_parameters *egress_evc_bw_profile;

#define NSM_MAX_COS_PER_EVC 8
  u_int8_t num_of_ingress_cos_id [NSM_MAX_COS_PER_EVC]; ///< Array maintained to
    ///<assign a COS ID from this array for the cos values set for bw profiling
  
  u_int8_t ingress_CoS_id [NSM_MAX_COS_PER_EVC];  ///< Array used to maintain
    ///<  the the cos id generated for each cos value set

  u_int8_t num_of_egress_cos_id [NSM_MAX_COS_PER_EVC];   ///< Array maintained
    ///< to assign COS ID from this array for the cos values set for profiling

  u_int8_t egress_CoS_id [NSM_MAX_COS_PER_EVC];   ///< Array used to maintain
    ///< the the cos id generated for each cos value set


  struct nsm_bw_profile_parameters 
    *ingress_evc_cos_bw_profile [NSM_MAX_COS_PER_EVC]; ///< Array of bw profile
                                      ///<   parameters for each cos value set
  
  struct nsm_bw_profile_parameters 
    *egress_evc_cos_bw_profile [NSM_MAX_COS_PER_EVC]; ///< Array of bw profile
                                     ///<   parameters for each cos value set
};

struct nsm_band_width_profile
{
  /* Back Pointer to ifp */
  struct nsm_if *zif;

  struct nsm_bw_profile_parameters *ingress_uni_bw_profile; ///< Pointer
            ///<  maintatined to store the bw profile parameters per UNI

  struct nsm_bw_profile_parameters *egress_uni_bw_profile;  ///< Pointer
            ///<  maintatined to store the bw profile parameters per UNI

  struct list *uni_evc_bw_profile_list;  ///< List maintained on each interface
            ///< for the evcs associated to the interface. 

  u_int8_t ingress_bw_profile_type; ///< Per EVC or Per UNI
  
  u_int8_t egress_bw_profile_type; ///< Per EVC or Per UNI

    #define NSM_BW_PROFILE_PER_EVC     1
    #define NSM_BW_PROFILE_PER_UNI     2

  u_int8_t ingress_bw_profile_status; ///<BW_PROFILE Configured or not
  
  u_int8_t egress_bw_profile_status; ///<BW_PROFILE Configured or not
  
  #define BW_PROFILE_CONFIGURED      1
  #define BW_PROFILE_NOT_CONFIGURED 0

  struct nsm_bridge *bridge; ///< Backpointer to the bridge
};
#endif /* HAVE_PROVIDER_BRIDGE */

struct nsm_bridge_port
{
  u_int32_t ifindex;
  struct nsm_if *nsmif; /* Back Pointer to ifp */

#ifdef HAVE_VLAN
  u_int16_t svid;

  struct nsm_vlan_port vlan_port;

#ifdef HAVE_GVRP
  struct gvrp_port *gvrp_port;
#endif /* HAVE_GVRP */
#endif /* HAVE_VLAN */

#ifdef HAVE_GMRP
  struct gmrp_port *gmrp_port;
  struct avl_tree *gmrp_port_list;
#endif /* HAVE_GMRP */

#ifdef HAVE_PROVIDER_BRIDGE

  struct nsm_bridge_protocol_process proto_process;

  enum nsm_uni_service_attr uni_ser_attr;

  struct avl_tree *trans_tab;
  struct avl_tree *rev_trans_tab;

#define NSM_UNI_NAME_MAX  16

  char uni_name [NSM_UNI_NAME_MAX + 1];

  /* This variable maintains the max EVCs allowed on the interface 
   * as  per MEF 10.1 
   * minimum value is 1
   * */
  u_int16_t uni_max_evcs;

    
#endif /* HAVE_PROIVDER_BRIDGE */

  struct nsm_uni_type_status uni_type_status;     ///<Structure to maintain the
    ///< uni type status of the interface. This is maintained as a structure
    ///< instance as the messages from CFM and E-LMI need to be updated
    ///< irrespective of uni type mode enabled/disabled.
  
  struct nsm_bridge *bridge;

  /* Port Forwarding State */
  u_int8_t state;

#ifdef HAVE_RPVST_PLUS
  /* Port instance to state mapping */
  u_char instance_state[NSM_RPVST_BRIDGE_INSTANCE_MAX];
#else
  u_char instance_state[NSM_BRIDGE_INSTANCE_MAX];
#endif /* HAVE_RPVST_PLUS */

#ifdef HAVE_RATE_LIMIT
  /* Rate Limit Configuration */
  /* Is rate limit configured */
  u_int8_t is_rate_limit;

  /* Configured rate limit level */
  u_int8_t bcast_level;
  u_int8_t bcast_fraction;
  u_int8_t mcast_level;
  u_int8_t mcast_fraction;
  u_int8_t dlf_bcast_level;
  u_int8_t dlf_bcast_fraction;
#endif /* HAVE_RATE_LIMIT */

#ifdef HAVE_SNMP
  u_int32_t port_id;
#endif

  u_int8_t spanning_tree_disable;

#ifdef HAVE_HA
  HA_CDR_REF nsm_bridge_port_cdr_ref;
  HA_CDR_REF nsm_rate_limit_cdr_ref;
#endif /* HAVE_HA */

#ifdef HAVE_QOS
  /* Num of Class of service queues */
  /* For BCM box, the value is maintained on the bridge. All the ports of the
   * bridge will share the same value */
  s_int32_t num_cosq;
#endif /* HAVE_QOS */
};

/* Function prototypes. */
int nsm_vlan_init (struct nsm_bridge *bridge);

int nsm_vlan_deinit (struct nsm_bridge *bridge);

int nsm_vlan_set_server_callback (struct nsm_server *ns);
int
nsm_vlan_send_port_type_msg (struct nsm_bridge *bridge,
                             struct nsm_msg_vlan_port_type *msg);

void nsm_clear_vlans (struct nsm_bridge *bridge, struct interface *ifp,
                      struct nsm_vlan_bmp *bmp, struct nsm_vlan_port *vlan_port,
                      bool_t notify_kernel);

int nsm_api_vlan_clear_port (struct interface *ifp, bool_t iterate_members,
                             bool_t notify_kernel);

int nsm_vlan_clear_hybrid_port (struct interface *ifp, bool_t iterate_members,
                                bool_t notify_kernel);

int nsm_vlan_clear_trunk_port (struct interface *ifp, bool_t iterate_members,
                               bool_t notify_kernel);

int nsm_vlan_add (struct nsm_bridge_master *master, char *brname,
                  char *vlan_name, u_int16_t vid, enum nsm_vlan_state state,
                  u_int16_t type);

int nsm_vlan_delete (struct nsm_bridge_master *master, char *brname,
                     u_int16_t vid, u_int16_t type);

struct nsm_vlan *
nsm_vlan_find (struct nsm_bridge *bridge, u_int16_t vid);

void nsm_vlan_cleanup (struct nsm_master *);

int nsm_vlan_api_set_port_mode (struct interface *ifp,
                                enum nsm_vlan_port_mode mode,
                                enum nsm_vlan_port_mode sub_mode,
                                bool_t interate_members, bool_t notify_kernel);

int nsm_vlan_api_get_port_mode (struct interface *ifp,
                                enum nsm_vlan_port_mode *mode,
                                enum nsm_vlan_port_mode *sub_mode);
char *
nsm_vlan_if_mode_to_str (enum nsm_vlan_port_mode mode);

int
nsm_vlan_set_pvid (struct interface *ifp, enum nsm_vlan_port_mode mode,
                   enum nsm_vlan_port_mode sub_mode,
                   nsm_vid_t pvid, bool_t iterate_members,
                   bool_t notify_kernel);

int nsm_vlan_get_pvid (struct interface *ifp, nsm_vid_t *pvid);

int nsm_vlan_set_access_port (struct interface *ifp, nsm_vid_t pvid,
                              bool_t interate_members, bool_t notify_kernel);

int nsm_vlan_set_hybrid_port (struct interface *ifp, nsm_vid_t pvid,
                              bool_t interate_members, bool_t notify_kernel);

int nsm_vlan_set_trunk_port (struct interface *ifp, nsm_vid_t pvid,
                             bool_t interate_members, bool_t notify_kernel);

int nsm_vlan_set_provider_port (struct interface *ifp, nsm_vid_t pvid,
                                enum nsm_vlan_port_mode mode,
                                enum nsm_vlan_port_mode sub_mode,
                                bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_add_port (struct nsm_bridge *bridge, 
                       struct interface *ifp, nsm_vid_t vid,
                       enum nsm_vlan_egress_type egress_tagged,
                       bool_t notify_kernel);

int nsm_vlan_delete_port (struct nsm_bridge *bridge,
                          struct interface *ifp, nsm_vid_t vid,
                          bool_t notify_kernel);

int nsm_vlan_set_acceptable_frame_type (struct interface *ifp,
                                        enum nsm_vlan_port_mode mode,
                                        int acceptable_frame_type);

#ifdef HAVE_SMI
int nsm_vlan_get_acceptable_frame_type (struct interface *ifp, enum smi_acceptable_frame_type *acceptable_frame_type);

int nsm_set_egress_port_mode (struct interface *ifp, int egress_mode);

int nsm_set_portbased_vlan (struct interface *ifp, struct smi_port_bmp bitmap);

int nsm_set_cpuport_default_vlan (struct interface *ifp, int vid);

int nsm_force_default_vlan (struct interface *ifp, int vid);

int nsm_svlan_set_port_ether_type (struct interface *ifp, u_int16_t etype);

int nsm_set_learn_disable (struct interface *ifp, int enable);

int nsm_get_learn_disable (struct interface *ifp,
                           enum smi_port_learn_state *enable);

int nsm_set_waysideport_default_vlan(struct interface *ifp, int vid);

int
nsm_api_clear_vlan_port (struct nsm_bridge *bridge, struct interface *ifp,
                         struct smi_vlan_bmp vlan_bmp);

#endif /* HAVE_SMI */

int nsm_vlan_set_ingress_filter (struct interface *ifp,
                                 enum nsm_vlan_port_mode mode,
                                 enum nsm_vlan_port_mode sub_mode,
                                 int enable);

int nsm_vlan_get_ingress_filter (struct interface *ifp,
                                 enum nsm_vlan_port_mode *mode,
                                 enum nsm_vlan_port_mode *sub_mode,
                                 int *enable);

int nsm_vlan_send_port_map (struct nsm_bridge *bridge,
                            struct nsm_msg_vlan_port *msg,
                            int msgid);

#if (defined HAVE_I_BEB || defined HAVE_B_BEB )
int
nsm_pbb_send_isid2svid(struct nsm_bridge * bridge,
                       struct nsm_msg_pbb_isid_to_pip *msg,
                       int msgid );
int
nsm_pbb_send_isid2bvid(struct nsm_bridge * bridge,
    struct nsm_msg_pbb_isid_to_bvid *msg,
    int msgid);

#endif

#ifdef HAVE_G8031
int
nsm_g8031_send_protection_group(struct nsm_bridge * bridge,
                                struct nsm_msg_g8031_pg *msg,
                                int msgid );

int
nsm_g8031_send_vlan_group(struct nsm_msg_g8031_vlanpg *msg,
                          int msgid );

int
nsm_g8031_send_vlan_del (struct nsm_bridge *bridge,
                         struct nsm_msg_vlan *msg,
                         int msgid);
#endif /* HAVE_G8031 */
#ifdef HAVE_PBB_TE
int
nsm_vlan_pbb_te_add_interface (struct nsm_bridge_master *master, char* brname,
                                  nsm_vid_t vid,
                                  struct interface *ifp, bool_t is_allowed,
                                  bool_t multicast);

int
nsm_vlan_pbb_te_delete_interface (struct nsm_bridge_master *master, char* brname,
                                  nsm_vid_t vid,
                                  struct interface *ifp, bool_t is_allowed,
                                  bool_t multicast);

#endif

#ifdef HAVE_VLAN_CLASS
int nsm_vlan_set_access_port_classifier (struct interface *ifp, 
                                         u_int8_t class_grp);

int nsm_vlan_unset_access_port_classifier (struct interface *ifp, 
                                           u_int8_t class_grp);

int nsm_vlan_send_classifier_msg (struct nsm_msg_vlan_classifier *msg, 
                                  int msgid);
#endif /* HAVE_VLAN_CLASS */

int nsm_vlan_add_access_port (struct interface *ifp, nsm_vid_t vid,
                              enum nsm_vlan_egress_type egresstagged,
                              bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_add_trunk_port (struct interface *ifp, nsm_vid_t vid,
                             bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_add_hybrid_port (struct interface *ifp, nsm_vid_t vid,
                              enum nsm_vlan_egress_type egresstagged,
                              bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_add_provider_port (struct interface *ifp, nsm_vid_t vid,
                                enum nsm_vlan_port_mode mode,
                                enum nsm_vlan_port_mode sub_mode,
                                enum nsm_vlan_egress_type egresstagged,
                                bool_t iterate_members, bool_t notify_kernel);

int
nsm_vlan_add_common_port (struct interface *ifp, enum nsm_vlan_port_mode mode,
                          enum nsm_vlan_port_mode sub_mode,
                          nsm_vid_t vid, enum nsm_vlan_egress_type egress_tagged,
                          bool_t iterate_members, bool_t notify_kernel);

int
nsm_vlan_set_common_port (struct interface *ifp, enum nsm_vlan_port_mode mode,
                          enum nsm_vlan_port_mode sub_mode,
                          nsm_vid_t pvid, bool_t iterate_members,
                          bool_t notify_kernel);


int nsm_vlan_delete_trunk_port (struct interface *ifp, nsm_vid_t vid,
                                bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_delete_hybrid_port (struct interface *ifp, nsm_vid_t vid,
                                 bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_delete_provider_port (struct interface *ifp, nsm_vid_t vid,
                                   enum nsm_vlan_port_mode mode,
                                   enum nsm_vlan_port_mode sub_mode,
                                   bool_t iterate_members,
                                   bool_t notify_kernel);

int
nsm_vlan_delete_common_port (struct interface *ifp,
                             enum nsm_vlan_port_mode mode,
                             enum nsm_vlan_port_mode sub_mode, nsm_vid_t vid,
                             bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_add_default (struct interface *ifp);

int nsm_vlan_clear_port (struct interface *ifpi);

int nsm_vlan_add_all_except_vid (struct interface *ifp,
                                 enum nsm_vlan_port_mode mode,
                                 enum nsm_vlan_port_mode sub_mode,
                                 struct nsm_vlan_bmp *excludeBmp, 
                                 enum nsm_vlan_egress_type egress_tagged,
                                 bool_t iterate_members, bool_t notify_kernel);

int nsm_vlan_set_native_vlan (struct interface *ifp, nsm_vid_t native_vid);

int nsm_vlan_get_native_vlan (struct interface *ifp, nsm_vid_t *native_vid);

int nsm_vlan_set_mtu (struct nsm_bridge_master *master, char *brname,
                      u_int16_t vid, u_char vlan_type, u_int32_t mtu_val);

/* Vlan Listener interface */
int nsm_add_listener_to_list(struct list *listener_list,
                             struct nsm_vlan_listener *appln);

void nsm_remove_listener_from_list(struct list *listener_list,
                                   nsm_listener_id_t appln_id);

int nsm_create_vlan_listener(struct nsm_vlan_listener **appln);

void nsm_destroy_vlan_listener(struct nsm_vlan_listener *appln);

/* Config store and activation functions */
extern struct nsm_vlan_config_entry *nsm_vlan_config_entry_new (int vid);

extern void nsm_vlan_config_entry_insert (struct nsm_bridge_master *master,
                                          char *br_name,
                                          struct nsm_vlan_config_entry *vlan_conf_new);

extern struct nsm_vlan_config_entry * nsm_vlan_config_entry_find (
                                      struct nsm_bridge_master *master,
                                      char* br_name, int vid);

int
nsm_vlan_if_set (struct nsm_master *nm, nsm_vid_t vid,
                 nsm_vid_t svid, u_char type, char *brname);

int
nsm_vlan_if_unset (struct nsm_master *nm, nsm_vid_t vid,
                   nsm_vid_t svid, u_char type, char *brname);

int
nsm_bridge_vlan_port_membership_sync (struct nsm_master *nm,
                                      struct nsm_bridge *bridge,
                                      struct interface *ifp);

int
nsm_bridge_vlan_port_type_sync (struct nsm_master *nm,
                                struct nsm_bridge *bridge,
                                struct interface *ifp);

#else /* HAVE_CUSTOM1 */

/* Master structure for Layer 2 information.  This is VR local
   information.  If you need to put global data outside of VR context,
   please use struct nsm_globals in nsmd.h. */
struct nsm_layer2_master
{
  /* Maxinum number and current number of VLAN interface.  */
#define NSM_LAYER2_VLAN_MAX                  255
  int vlan_max;
  int vlan_num;

  /* Maxinum number and current number of VLAN interface as L3.  */
#define NSM_LAYER2_VLAN_L3_MAX                25
  int vlan_l3_max;
  int vlan_l3_num;
};

int nsm_vlan_init (u_int32_t);
#endif /* HAVE_CUSTOM1 */

/* Function prototypes. */
void nsm_vlan_config_entry_free (struct nsm_vlan_config_entry *);
int nsm_bridge_vlan_sync (struct nsm_master *, struct nsm_server_entry *);
int nsm_vlan_validate_vid (char *str, struct cli *cli);
int nsm_vlan_validate_vid1 (char *str, struct cli *cli);
int nsm_vlan_id_validate_by_interface (struct interface *, u_int16_t);
int nsm_is_vlan_exist (struct interface *, u_int16_t);

#ifdef HAVE_LACPD
int nsm_vlan_agg_mode_vlan_sync(struct nsm_bridge_master *master,
                                struct interface *ifp,
                                bool_t use_conf);
#endif /* HAVE_LACPD */

#ifdef DEBUG
#ifdef HAVE_ISO_MACRO_VARARGS
#define NSM_VLAN_FN_ENTER(...)                                        \
    do {                                                              \
        zlog_info (NZG, "Entering %s\n", __FUNCTION__);               \
    } while (0)
#define NSM_VLAN_FN_EXIT(...)                                         \
    do {                                                              \
       zlog_info (NZG, "Returning %s, %d\n",\ __FUNCTION__,           \
                  __LINE__);                                          \
       return __VA_ARGS__;                                            \
    } while (0)
#else
#define NSM_VLAN_FN_ENTER(ARGS...)                                    \
    do {                                                              \
       zlog_info (NZG, "Entering %s\n", __FUNCTION__);                \
    } while (0)
#define HSL_FN_EXIT(ARGS...)                                          \
    do {                                                              \
       zlog_info (NZG , "Returning %s, %d\n", __FUNCTION__,           \
                  __LINE__);                                          \
       return ARGS;                                                   \
    } while (0)
#endif /* HAVE_ISO_MACRO_VARARGS */
#else
#ifdef HAVE_ISO_MACRO_VARARGS
#define NSM_VLAN_FN_ENTER(...)
#else
#define NSM_VLAN_FN_ENTER(ARGS...)
#endif /* HAVE_ISO_MACRO_VARARGS */
#ifdef HAVE_ISO_MACRO_VARARGS
#define NSM_VLAN_FN_EXIT(...)                                         \
    return __VA_ARGS__;
#else
#define NSM_VLAN_FN_EXIT(ARGS...)                                     \
    return ARGS;
#endif /* HAVE_ISO_MACRO_VARARGS */
#endif

int
nsm_vlan_id_cmp (void * data1, void * data2);

int
nsm_vlan_port_get_dot1q_state (struct interface *ifp, u_int8_t *enable);

int
nsm_update_portbased_vlan (struct nsm_if *zif, u_int32_t portmap);

int
nsm_vlan_port_set_dot1q_state (struct interface *ifp,
                               int enable);
int
nsm_vlan_agg_update_config_flag (struct interface *);

int
nsm_lacp_if_admin_key_set (struct interface *);

#ifdef HAVE_SMI
int nsm_get_all_vlan_config (char *br_name, struct smi_vlan_bmp *vlan_bitmap);
int nsm_get_vlan_by_id (char *br_name, int vlan_id, 
                        struct smi_vlan_info *vlan_info);
int nsm_vlan_if_get (struct interface *ifp, 
                     struct smi_if_vlan_info *if_vlan_info);

int nsm_get_bridge_config(char *br_name, struct smi_bridge *smi_bridge);

int
nsm_vlan_get_dot1q_state (struct interface *ifp, u_int8_t *enable);

/* For Updating the Vlan Id for Alarms */
int
nsm_vlan_update_vid_bmp (struct interface *ifp, enum nsm_vlan_port_mode mode,
                         struct smi_vlan_bmp *vlan_bmp,
                         struct smi_vlan_bmp *egr_vlan_bmp);

/* For removing all vlans except few */
int
nsm_vlan_remove_all_except_vid (struct interface *ifp, 
                                enum nsm_vlan_port_mode mode,
                                enum nsm_vlan_port_mode sub_mode,
                                struct nsm_vlan_bmp *excludeBmp,
                                enum nsm_vlan_egress_type egress_tagged,
                                enum smi_vlan_add_opt vlan_add_opt);

int
nsm_vlan_api_add_all_vid (struct interface *ifp,
                          enum nsm_vlan_port_mode mode,
                          enum nsm_vlan_port_mode sub_mode,
                          struct nsm_vlan_bmp *excludeBmp,
                          enum nsm_vlan_egress_type egress_tagged,
                          bool_t iterate_members, bool_t notify_kernel);
#endif /* HAVE_SMI */
#endif /* _NSM_VLAN_H */
