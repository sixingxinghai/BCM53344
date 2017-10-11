/* 
   Copyright (C) 2003  All Rights Reserved. 
   
   LAYER 2 BRIDGE
   
   This module declares the interface to the Bridge
   functions.
  
*/

#ifndef __PACOS_MSTP_BRIDGE_H__
#define __PACOS_MSTP_BRIDGE_H__

#include "mstp_bpdu.h"

#define MAX_IFINDEX 0x7fffffff

#define BRIDGE_CIST_PORTLIST_LOOP(curr, br) \
  for (curr = br->port_list; curr; curr = curr->next)

#define MSTP_BRIDGE_INSTANCE_CONFIG_GET(list, node, id)     \
  for (node = list; node; node = node->next)                \
     {                                                      \
       if (node->instance_info.instance == id)              \
         break;                                             \
     }

/* MSTP Bridge Macro */
#define IS_BRIDGE_MSTP(B)                                                     \
        ((B) && (((B)->type == NSM_BRIDGE_TYPE_MSTP)                          \
         || ((B)->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP)                      \
         || ((B)->type == NSM_BRIDGE_TYPE_BACKBONE_MSTP)))

#define IS_BRIDGE_STP(B)                                                      \
        ((B) && (((B)->type == NSM_BRIDGE_TYPE_STP)                           \
         || ((B)->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)))

#define IS_BRIDGE_RSTP(B)                                                     \
        ((B) && (((B)->type == NSM_BRIDGE_TYPE_RSTP)                          \
         || ((B)->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP)                      \
         || ((B)->type == NSM_BRIDGE_TYPE_CE)                                 \
         || ((B)->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE)))

#define IS_BRIDGE_RPVST_PLUS(B)                                               \
        ((B) && ((B)->type == NSM_BRIDGE_TYPE_RPVST_PLUS))

#define BRIDGE_TYPE_PROVIDER(B)                                               \
        (((B)->type == MSTP_BRIDGE_TYPE_PROVIDER_RSTP)                        \
         || ((B)->type == MSTP_BRIDGE_TYPE_PROVIDER_MSTP)                     \
         || ((B)->type == MSTP_BRIDGE_TYPE_BACKBONE_MSTP)                    \
         || ((B)->type == MSTP_BRIDGE_TYPE_BACKBONE_RSTP))                    

#define BRIDGE_TYPE_CVLAN_COMPONENT(B)                                        \
        ((B)->type == NSM_BRIDGE_TYPE_CE)

#if (defined HAVE_I_BEB )
#define BRIDGE_TYPE_SVLAN_COMPONENT(B)                                        \
        ((B)->type == NSM_BRIDGE_TYPE_CNP)

#define BRIDGE_TYPE_BVLAN_COMPONENT(B)                                        \
        ((B)->type == NSM_BRIDGE_TYPE_CBP)
#endif

#define BRIDGE_GET_CE_BR_NAME(ifindex)                                        \
        pal_snprintf (bridge_name, L2_BRIDGE_NAME_LEN, "%s%d", "CE-",         \
                      ifindex);

#if (defined HAVE_I_BEB )
#define BRIDGE_GET_CNP_BR_NAME(ifindex)                                        \
        pal_snprintf (bridge_name, L2_BRIDGE_NAME_LEN, "%s%d", "CNP-",         \
                      ifindex);

#endif

#define MSTP_SET_VLAN_INSTANCE_MAP(array, vid, instance)                   \
 {                                                                         \
  int ret_flag = 0;                                                        \
  for (instance= 1; instance < RPVST_MAX_INSTANCES; instance++)            \
     {                                                                     \
       if (array[instance] == vid)                                         \
         {                                                                 \
         ret_flag = RESULT_ERROR;                                          \
         break;                                                            \
         }                                                                 \
       else if(array[instance] == 0)                                       \
        {                                                                  \
          array[instance] = vid;                                           \
          ret_flag = 1;                                                    \
          break;                                                           \
        }                                                                  \
     }                                                                     \
   if (ret_flag == 0)                                                      \
     instance = RESULT_OK;                                                 \
   else if (ret_flag < 0)                                                  \
     instance = RESULT_ERROR;                                              \
 }

#define MSTP_GET_VLAN_INSTANCE_MAP(array, vid, instance)                   \
 {                                                                         \
  int index = 0;                                                           \
  for (index = 1; index < RPVST_MAX_INSTANCES; index++)                    \
     {                                                                     \
       if (array[index] == vid)                                            \
         {                                                                 \
           instance = index;                                               \
           break;                                                          \
         }                                                                 \
     }                                                                     \
}                                                                          \

#define MSTP_DEL_VLAN_INSTANCE_MAP(array, vid, instance)                   \
 {                                                                         \
     if (array[instance] == vid)                                           \
       {                                                                   \
         array[instance] = 0;                                              \
         ret = RESULT_OK;                                                  \
       }                                                                   \
        else                                                               \
         ret = RESULT_ERROR;                                               \
                                                                           \
 }      

#define MSTP_BRIDGE_STR_LEN                  6
#define MSTP_MAX_CLI_LEN                     2048

#define BRIDGE_TYPE_XSTP                     0


#define TOPOLOGY_NONE                        0
#define TOPOLOGY_RING                        1

#define MSTP_BRIDGE_PRIORITY_MULTIPLIER    4096
#define MSTP_BRIDGE_PORT_PRIORITY_MULTIPLIER 16
#define MSTP_ERROR_GENERAL                 -50
#define MSTP_ERR_BRIDGE_NOT_FOUND          (MSTP_ERROR_GENERAL + 1)
#define MSTP_ERR_PRIORITY_VALUE_WRONG      (MSTP_ERROR_GENERAL + 2)
#define MSTP_ERR_PRIORITY_OUTOFBOUNDS      (MSTP_ERROR_GENERAL + 3)
#define MSTP_ERR_INSTANCE_OUTOFBOUNDS      (MSTP_ERROR_GENERAL + 4)
#define MSTP_ERR_INSTANCE_NOT_FOUND        (MSTP_ERROR_GENERAL + 5)
#define MSTP_ERR_PORT_NOT_FOUND            (MSTP_ERROR_GENERAL + 6)
#define MSTP_ERR_NOT_MSTP_BRIDGE           (MSTP_ERROR_GENERAL + 7)
#define MSTP_ERR_NOT_RPVST_BRIDGE          (MSTP_ERROR_GENERAL + 8)
#define MSTP_ERR_RPVST_BRIDGE_NO_VLAN      (MSTP_ERROR_GENERAL + 9)
#define MSTP_ERR_RPVST_BRIDGE_VLAN_EXISTS  (MSTP_ERROR_GENERAL + 10)
#define MSTP_ERR_RPVST_BRIDGE_MAX_VLAN     (MSTP_ERROR_GENERAL + 11)
#define MSTP_ERR_RPVST_NONE                (MSTP_ERROR_GENERAL + 12)
#define MSTP_ERR_RPVST_VLAN_CONFIG_ERR     (MSTP_ERROR_GENERAL + 13)
#define MSTP_ERR_RPVST_VLAN_MEM_ERR        (MSTP_ERROR_GENERAL + 14)
#define MSTP_ERR_RPVST_VLAN_BR_GR_ASSOCIATE (MSTP_ERROR_GENERAL + 15)
#define MSTP_ERR_RPVST_VLAN_BR_GR_ADD      (MSTP_ERROR_GENERAL + 16)
#define MSTP_ERR_INSTANCE_ALREADY_BOUND    (MSTP_ERROR_GENERAL + 17)
#define MSTP_INSTANCE_IN_USE_ERR           (MSTP_ERROR_GENERAL + 18)
#define MSTP_ERR_HELLO_NOT_CONFIGURABLE    (MSTP_ERROR_GENERAL + 19)

void
mstp_link_bridge (struct mstp_bridge *new);

#if (defined HAVE_I_BEB )
int mstp_bridge_add (char * name, u_int8_t type,
                     bool_t ce_bridge, bool_t cn_bridge,
                     bool_t is_default, u_char topology_type,
                     struct mstp_port *ce_port);
#else
int mstp_bridge_add (char * name, u_int8_t type,
                     bool_t ce_bridge, bool_t is_default, u_char topology_type,
                     struct mstp_port *ce_port);
#endif

int mstp_bridge_delete (char * name, struct mstp_port *port);

void *mstp_get_first_bridge (void);

void *mstp_find_bridge (char * name);

int mstp_add_port (struct mstp_bridge *br, char *ifname,
                   u_int16_t svid, int instance, u_int8_t spanning_tree_disable);

int mstp_delete_port (char * name, char * ifname, u_int16_t svid,
                      int instance, int force, bool_t);

int mstp_add_instance (struct mstp_bridge *br, short instance_id, 
                       mstp_vid_t vid, u_int32_t vlan_range_indx);

int mstp_delete_instance (struct mstp_bridge *br, int instance);

int mstp_delete_instance_vlan (struct mstp_bridge *, int, mstp_vid_t);

int mstp_delete_cist_port (struct mstp_port *port, int is_explicit,
                           int force, bool_t);

struct mstp_instance_port *
mstp_find_msti_port (struct mstp_bridge_instance *br_inst, 
                     u_int32_t ifindex);

void
mstp_cist_handle_port_forward (struct mstp_port *port);

void
mstp_unlink_port_instance (struct mstp_instance_port *inst_port);

void *mstp_find_cist_port (void *mstp_bridge, u_int32_t ifindex );

struct mstp_instance_port *
mstp_find_msti_port_in_inst_list (struct mstp_port *port, int instance_id ) ;

void mstp_gen_cfg_digest (struct mstp_bridge *br);

pal_inline void
mstp_unlink_bridge (struct mstp_bridge *curr);

void
mstp_unlink_cist_port (struct mstp_port *curr);

extern inline int 
mstp_bridge_is_cist_root (struct mstp_bridge * br);

extern inline int 
mstp_bridge_is_msti_root (struct mstp_bridge_instance * br_inst);

void mstp_bridge_terminate ();

int mstp_enable_bridge (struct mstp_bridge *br);

int mstp_disable_bridge (struct mstp_bridge *br, bool_t bridge_forward);

int mstp_bridge_add_static_fdb_entry (char * const name, u_int32_t ifindex, 
                                      const char * mac, bool_t is_fwd, 
                                      unsigned short vid);

void
mstp_link_msti_port (struct mstp_bridge_instance *br_inst,
                     struct mstp_instance_port *inst_port);

void
mstp_unlink_msti_port (struct mstp_instance_port *curr);

void
mstp_link_port_instance (struct mstp_port *port ,
                         struct mstp_instance_port *inst_port);

void mstp_bridge_delete_static_fdb_entry (char * const br_name,
                                          unsigned char * const mac);

struct mstp_bridge_static_fdb_data *mstp_bridge_static_entry_new ();

void mstp_bridge_static_entry_free (struct mstp_bridge_static_fdb_data *fdb_node);

struct mstp_vlan *mstp_bridge_vlan_lookup (char *bridge_name, u_int16_t vid);

int
_mstp_bridge_create_vlan_table (struct mstp_bridge *br);

int mstp_bridge_vlan_add (char *bridge_name, u_int16_t vid,
                          char *vlan_name, int flags);

int mstp_bridge_vlan_add2 (char *bridge_name, u_int16_t vid,
                           s_int32_t instance, int flags);

int mstp_bridge_vlan_instance_delete (char *bridge_name, int instance);

int mstp_bridge_vlan_delete (char *bridge_name, u_int16_t vid);

int mstp_bridge_vlan_validate_vid (char *str, struct cli *cli);

void mstp_bridge_set_cisco_interoperability (struct mstp_bridge *br,
                                             unsigned char enable);
int
mstp_bridge_free_vlan_table (struct mstp_bridge *br);

void
mstp_link_cist_port (struct mstp_bridge *br, struct mstp_port *port);

struct mstp_bridge_config_list *mstp_get_mstp_bridge_config_list_head ();

void mstp_br_config_unlink_instance (struct mstp_bridge_info *br_info,
                                     struct mstp_bridge_instance_list *conf_instance_list);

void mstp_br_config_instance_add_port (char *br_name, struct interface *ifp,
                                       s_int32_t instance);

void mstp_bridge_instance_activate_port (char *bridge_name, s_int32_t instance);

void mstp_bridge_instance_config_delete (char *bridge_name,
                                         s_int32_t instance,
                                         mstp_vid_t vid);

void mstp_bridge_port_instance_config_delete (struct interface *ifp,
                                              char *bridge_name,
                                              s_int32_t instance);

int mstp_cist_has_designated_port (struct mstp_bridge *bridge);

u_int8_t * mstp_get_ce_br_port_str (struct mstp_port *port);

void *
mstp_find_ce_br_port (void *mstp_bridge, u_int16_t svid);

struct mstp_instance_port *
mstp_find_ce_br_inst_port (struct mstp_bridge_instance *br_inst,
                           u_int16_t svid);

int
mstp_add_ce_br_port (struct mstp_bridge *br, char *ifname,
                     u_int16_t svid);


int
mstp_delete_ce_br_port (struct mstp_bridge *br, char *ifname,
                        u_int16_t svid);

#if (defined HAVE_I_BEB  )

uint32_t
mstp_add_cn_br_port (struct mstp_bridge *br,
                     char *ifname,
                     uint16_t isid,
                     struct nsm_msg_pbb_isid_to_pip * );
void *
mstp_find_cn_br_port (void *mstp_bridge, u_int16_t isid);

uint32_t
mstp_del_cn_br_vip_port( struct mstp_bridge *br,
                         char *ifname,
                         uint32_t isid,
                         struct nsm_msg_pbb_isid_to_pip * );
uint32_t
mstp_del_cn_br_port( struct mstp_bridge *br , uint32_t isid );

uint32_t mstp_clean_cn_br_port (struct mstp_port *port);

uint32_t
mstp_cn_br_update_vip_list_status( struct mstp_bridge *br, uint32_t isid );

uint32_t
mstp_updt_bvid_to_isid_mapping( struct mstp_bridge *br,
                                struct nsm_msg_pbb_isid_to_bvid * );

struct mstp_port *
mstp_get_cn_port_by_svid( struct mstp_bridge * cn_br, uint16_t svid );


#endif

u_int8_t * mstp_get_bridge_str (struct mstp_bridge *br);

u_int8_t * mstp_get_bridge_str_span (struct mstp_bridge *br);

u_int8_t * mstp_get_bridge_group_str_span (struct mstp_bridge *br);

u_int32_t
mstp_nsm_get_default_port_path_cost (const float bandwidth);
 
u_int32_t
mstp_nsm_get_default_port_path_cost_short_method (const float bandwidth);
 
u_int32_t
mstp_nsm_get_port_path_cost (struct lib_globals *zg, const u_int32_t ifindex);


void 
mstp_bridge_config_list_free (struct mstp_bridge_info *br_config_info);
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
int
mstp_enable_bridge_instance (struct mstp_bridge_instance *mstp_br_inst);

int
mstp_disable_bridge_instance (struct mstp_bridge_instance *mstp_br_inst);
#endif /*(HAVE_PROVIDER_BRIDGE) ||(HAVE_B_BEB) */

#ifdef HAVE_RPVST_PLUS
struct mstp_instance_port *
mstp_find_msti_port_in_vlan_list(struct mstp_port *port,
                                 struct mstp_instance_bpdu *inst_bpdu);
#endif /* HAVE_RPVST_PLUS */

#endif /* __PACOS_MSTP_BRIDGE_H__ */
