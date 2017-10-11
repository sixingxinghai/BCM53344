/* 
   Copyright (C) 2003  All Rights Reserved. 
   
   LAYER 2 BRIDGE
   
   This module declares the API interface to the Bridge
   functions.
  
*/

#ifndef __MSTP_API_H__
#define __MSTP_API_H__

#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

#define MSTP_CONFIG_EDGEPORT             0
#define MSTP_CONFIG_PORTFAST             1

/* Support for Path cost method */
enum
{
  MSTP_PATHCOST_SHORT = 0, /* Uses 802.1d 1998 Table 8-5 */
  MSTP_PATHCOST_LONG,      /* Uses 802.1t Table 8-5 */
  MSTP_PATHCOST_DEFAULT,
  MSTP_ERR
};


#ifdef HAVE_I_BEB
#define  ICOMP_MSTP_SEND_ON_CNP()                                          \
          for (pip_port = br->port_list;pip_port;pip_port=pip_port->next)  \
            if (pip_port->port_type == MSTP_VLAN_PORT_MODE_CNP)            \
              mstp_port_send (pip_port,orig_buf, orig_len);
#endif /* HAVE_B_BEB */

#ifdef HAVE_B_BEB

#define IBCOMP_MSTP_SEND_ON_CNP()                                          \
       bcomp_br = (struct mstp_bridge *)mstp_get_first_bridge();           \
       if (!bcomp_br)                                                      \
         return;                                                           \
       if (bcomp_br->type != MSTP_BRIDGE_TYPE_BACKBONE_RSTP &&             \
           bcomp_br->type != MSTP_BRIDGE_TYPE_BACKBONE_MSTP)               \
          while(bcomp_br)                                                  \
           {                                                               \
            for(pip_port = bcomp_br->port_list;pip_port;pip_port=pip_port->next) \
              if (pip_port->port_type == MSTP_VLAN_PORT_MODE_CNP)          \
                 mstp_port_send (pip_port,orig_buf, orig_len);             \
              bcomp_br = bcomp_br->next;                                   \
          }
          

#endif /* HAVE_B_BEB */

extern int
mstp_api_add_port (char *name,  char *ifname,
                   u_int16_t svid, int instance, u_int8_t spanning_tree_disable );

extern int
mstp_api_delete_port (char *name, char *ifname,  
                      int instance,  u_int16_t svid, int force, bool_t fwd);

extern int
mstp_api_add_instance (char *name, int instance_id, mstp_vid_t vid,
                       u_int32_t vlan_range_indx);

extern int 
mstp_api_delete_instance (char *name , int instance);

int
mstp_api_set_learning (char *name,  int learning, 
                       int instance);
int
mstp_api_set_ageing_time (char *name,  s_int32_t ageing_time);

int
mstp_api_get_ageing_time (char * name, s_int32_t *ageing_time);

int
mstp_api_mcheck (char *name, char *ifName);

int
mstp_api_set_port_forceversion (char *  name,
                                char *  ifName, 
                                s_int32_t version);
int
mstp_api_get_port_forceversion (char *name, 
                                char *ifName, 
                                s_int32_t *version);

int
mstp_api_set_transmit_hold_count (char * name, unsigned char txholdcount);

int
mstp_api_get_transmit_hold_count (char * name, u_char *txholdcount);

int
mstp_api_set_bridge_forceversion (char *name,
                                  s_int32_t version);

int
mstp_api_set_bridge_mode (char *name, char *mode_str);

int
mstp_api_set_auto_edge (char *name,  char *ifName,
                         int to_be_enabled);

int
mstp_api_get_auto_edge (char * name, char * ifName,
                        int *enabled);

int
mstp_api_set_port_edge (char *name, char *ifName,
                        int to_be_enabled,  bool_t portfast_conf);

int
mstp_api_get_port_edge (char * name, char * ifName, int *enabled);

int
mstp_api_set_port_p2p (char *name, char *ifName, 
                       int is_p2p);

int
mstp_api_get_port_p2p (char *name, char * ifName, int *is_p2p);

int
mstp_api_read_bridge_info (char *name, 
                           struct mstp_bridge *br);

int
mstp_api_set_port_hello_time (char *name, char * ifName,
                              s_int32_t hello_time);

int
mstp_api_get_port_hello_time (char *name, char * ifName,
                              s_int32_t *hello_time);

int
mstp_api_set_port_restricted_tcn (char *name, char * ifName,
                                  bool_t restricted_tcn);

int
mstp_api_get_port_restricted_tcn (char *name, char * ifName,
                                  s_int16_t *restricted_tcn);

int
mstp_api_set_port_restricted_role (char *name, char * ifName,
                                   bool_t restricted_role);

int
mstp_api_get_port_restricted_role (char *name, char * ifName,
                                   s_int16_t *restricted_role);

int
mstp_api_set_max_age (char *name,  s_int32_t max_age);

int
mstp_api_get_max_age (char * name, s_int32_t *max_age);

int
mstp_api_set_hello_time (char *name,  s_int32_t hello_time);

int
mstp_api_get_hello_time (char *name,  s_int32_t *hello_time);

int
mstp_api_set_forward_delay (char *name,  s_int32_t forward_delay);

int
mstp_api_get_forward_delay (char *name,  s_int32_t *forward_delay);

int
mstp_api_set_bridge_priority (char *name, u_int32_t new_priority);

int
mstp_api_get_bridge_priority (char * name, u_int32_t *priority);

int
mstp_api_set_port_priority (char *name, char *ifName,
                            u_int16_t svid,  s_int16_t priority);

int
mstp_api_get_port_priority (char *name, char *ifName,
                            u_int16_t svid, s_int16_t *priority);
int
mstp_api_set_port_path_cost (char *name, char *ifName,
                             u_int16_t svid, u_int32_t cost);

int
mstp_api_get_port_path_cost (char *name, char *ifName,
                             u_int16_t svid, u_int32_t *cost);

int
mstp_api_set_msti_bridge_priority (char *name, int instance,
                                   u_int32_t new_priority);

int
mstp_api_get_msti_bridge_priority (char * name, int instance,
                                   u_int32_t *priority);
int
mstp_api_set_msti_port_priority (char *name, 
                                 char *ifName,  int instance,
                                 s_int16_t priority);
int
mstp_api_get_msti_port_priority (char *name, 
                                 char *ifName, int instance,
                                 s_int16_t *priority);
int
mstp_api_set_msti_port_path_cost (char *name, 
                                  char *ifName, int instance, 
                                  u_int32_t cost);

int
mstp_api_get_msti_port_path_cost (char *name, 
                                  char *ifName, int instance,
                                  u_int32_t *cost);

extern int
mstp_api_disable_bridge (char *name, u_int8_t br_type,
                         bool_t bridge_forward);

extern int
mstp_api_enable_bridge (char *name, u_int8_t br_type);

int
mstp_api_set_msti_instance_restricted_role (char *name, char *ifName,
                                             int instance,
                                             bool_t restricted_role);

int
mstp_api_get_msti_instance_restricted_role (char *name, char *ifName,
                                             int instance,
                                             u_int32_t *restricted_role);

int
mstp_api_set_msti_instance_restricted_tcn (char *name, char *ifName,
                                           int instance,
                                           bool_t restricted_tcn);

int
mstp_api_get_msti_instance_restricted_tcn (char *name, char *ifName,
                                           int instance,
                                           u_int32_t *restricted_tcn);

extern int
mstp_vlan_delete_event (char *name , u_int32_t vid);
  
extern int
mstp_vlan_add_event (char *name ,  u_int16_t vid,
                     u_int16_t fid);
int
mstp_api_set_max_hops (char *name,  s_int32_t max_hops);

int
mstp_api_get_max_hops (char * name, s_int32_t *max_hops);

int
mstp_api_region_name (char *name,  char *region_name);

int
mstp_api_get_region_name (char *name, char *region_name);

int
mstp_api_revision_number (char *name, u_int16_t rev_num);

int
mstp_api_get_revision_number (char * name, u_int16_t *rev_num);

int
mstp_msti_clear_reselect_bridge (struct mstp_bridge_instance *);

int
mstp_msti_check_reselect_bridge (struct mstp_bridge_instance *);

void
mstp_msti_set_selected_bridge (struct mstp_bridge_instance *);

int
mstp_cist_clear_reselect_bridge (struct mstp_bridge *);

int
mstp_cist_check_reselect_bridge (struct mstp_bridge *);

void
mstp_cist_set_selected_bridge (struct mstp_bridge *);

int
mstp_api_delete_instance_vlan (char *name, int instance, mstp_vid_t vid);

int 
mstp_api_set_bridge_portfast_bpduguard (char *br_name,
                                        bool_t bpduguard_enabled);
int 
mstp_api_get_bridge_portfast_bpduguard (char *br_name, 
                                        int *bpduguard_enabled);
int 
mstp_api_set_bridge_errdisable_timeout_enable (char *br_name, bool_t enabled);

int 
mstp_api_get_bridge_errdisable_timeout_enable (char *br_name, 
                                               int *enabled);

int 
mstp_api_set_bridge_errdisable_timeout_interval (char *br_name, s_int32_t timeout);

int 
mstp_api_get_bridge_errdisable_timeout_interval (char *br_name, 
                                                 s_int32_t *timeout);

int 
mstp_api_set_bridge_portfast_bpdufilter (char *br_name, bool_t enabled);

int 
mstp_api_get_bridge_portfast_bpdufilter (char *br_name, int *enabled);

int 
mstp_api_set_port_bpduguard (char * name,
                             char * ifName,
                             u_char portfast_bpduguard);
int 
mstp_api_get_port_bpduguard (char * name, char * ifName,
                             u_char *portfast_bpduguard);

int 
mstp_api_set_port_rootguard (char * name,
                             char * ifName,
                             bool_t enabled);

int 
mstp_api_get_port_rootguard (char * name,
                             char * ifName,
                             s_int32_t *enabled);

int 
mstp_api_set_port_bpdufilter (char * name,
                              char * ifName,
                              u_char portfast_bpdufilter);

int 
mstp_api_get_port_bpdufilter (char * name,
                              char * ifName,
                              u_char *portfast_bpdufilter);

void
mstp_set_msti_port_path_cost (struct mstp_port *port);

int
mstp_api_set_pathcost_method (char *br_name, u_int8_t path_cost_method);
 
int
mstp_api_get_pathcost_method (char *br_name);

void
mstp_nsm_send_info(bool_t state, struct mstp_port *port);

#ifdef HAVE_SMI
int
mstp_api_bridge_change_type(char * name, enum smi_bridge_type type,
                            enum smi_bridge_topo_type topo_type);
int
mstp_api_get_spanning_tree_details(char * name, 
                                   struct smi_mstp_spanning_tree_details 
                                   *stdetails);
int
mstp_api_get_spanning_tree_details_interface(struct interface *ifp, 
                                             struct smi_mstp_port_details 
                                             *port_details);
int
mstp_api_get_spanning_tree_mst(char *br_name, u_int16_t instance, 
                               struct smi_mstp_instance_details *inst_details);
int
mstp_api_get_spanning_tree_mst_config(char *br_name,
                                      struct smi_mstp_config_details 
                                      *conf_details);
int
mstp_api_get_spanning_tree_mstdetail(char *br_name, u_int16_t instance, 
                                     struct smi_mstp_instance_details 
                                     *inst_details);
int
mstp_api_get_spanning_tree_mstdetail_interface(struct interface *ifp, 
                                         u_int16_t instance, 
                                         struct smi_mstp_instance_port_details 
                                         *port_details);
int
mstp_api_get_bridge_status (char *name, u_char *stp_enabled, 
                            int *br_forward);


#endif /* HAVE_SMI */

#ifdef HAVE_L2GP

int
mstp_api_set_l2gp_port(char * name, char * ifname, 
                       uint8_t l2gp_status, 
                       uint8_t enableBPDUrx,
                       struct bridge_id * psuedoRootId );

void
mstp_put_prio_in_bridgeid (struct bridge_id *br_id, short new_priority);

#endif /* HAVE_L2GP */

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
int
mstp_api_enable_bridge_msti (struct mstp_bridge_instance *mst_br_inst); 

int
mstp_api_disable_bridge_msti (struct mstp_bridge_instance *mst_br_inst); 
#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/

#ifdef HAVE_I_BEB
void mstp_make_pbb_dst_mac ( uint8_t * ptr, uint32_t isid );
uint32_t
mstp_encapsulate_bpdu( u_char * buf, 
                       uint32_t isid, 
                       struct sockaddr_vlan *l2_skaddr);

#endif


#endif /*__MSTP_API_H__*/
