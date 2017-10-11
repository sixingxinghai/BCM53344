/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _MSTP_API_SERVER_H
#define _MSTP_API_SERVER_H

#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

#include "mstpd.h"
#include "mstp_config.h"
#include "mstp_types.h"
#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /*HAVE_HA*/
#define SMI_MSTP_PORTFAST       1
#define SMI_MSTP_NOPORTFAST     0
#define MSTP_VLAN_NULL_VID      0
#define MSTP_PORT_DEL_FORCE     1
#define MSTP_PORT_ADD_FORCE     1
#define MSTP_NOTIFY_FWD         1
#define MST_INSTANCE_IST        0

void smi_mstp_dump(struct lib_globals *zg, struct smi_msg_mstp *msg);

struct smi_server * mstp_smi_server_init (struct lib_globals *zg);
int mstp_smi_server_send_sync_message(struct smi_server_entry *ase, 
                                      int msgtype, int status, void *msg);
int mstp_smi_server_recv(struct smi_msg_header *header, void *arg, 
                         void *message);
int mstp_api_delete_instance (char *name , int instance);
int mstp_api_set_ageing_time (char *name,  s_int32_t ageing_time);
int mstp_api_get_ageing_time (char * name, s_int32_t *ageing_time);
int mstp_api_add_port (char *name, char *ifname, 
                       u_int16_t svid, int instance, u_int8_t spanning_tree_disable );
int mstp_api_delete_port (char *name, char *ifname, int instance, 
                          u_int16_t svid, int force, bool_t fwd);
int mstp_api_set_hello_time (char *name,  s_int32_t hello_time);
int mstp_api_get_hello_time (char *name,  s_int32_t *hello_time);
int mstp_api_set_max_age(char *name,  s_int32_t max_age);
int mstp_api_get_max_age (char * name, s_int32_t *max_age);
int mstp_api_set_port_edge (char *name, char *ifName, 
                            int to_be_enabled,  
                            bool_t portfast_conf);
int mstp_api_get_port_edge (char * name, char * ifName, int *enabled);
int mstp_api_set_port_forceversion (char *  name, char *  ifName, 
                                    s_int32_t version);
int mstp_api_get_port_forceversion (char *name, char *ifName, 
                                    s_int32_t *version);
int mstp_api_set_bridge_priority (char *name,  u_int32_t new_priority);
int mstp_api_get_bridge_priority (char * name, u_int32_t *priority);
int mstp_api_set_forward_delay (char *name,  s_int32_t forward_delay);
int mstp_api_get_forward_delay (char *name,  s_int32_t *forward_delay);
int mstp_api_set_msti_bridge_priority (char *name, int instance,
                                       u_int32_t msti_priority);
int mstp_api_get_msti_bridge_priority (char * name, int instance,
                                       u_int32_t *priority);
int mstp_api_set_msti_port_path_cost (char *name,char *ifName,  
                                      int instance, u_int32_t cost);
int mstp_api_get_msti_port_path_cost (char *name, 
                                      char *ifName, int instance,
                                      u_int32_t *cost);
int mstp_api_set_msti_instance_restricted_role (char *name, char *ifName,
                                                int instance, 
                                                bool_t restricted_role);
int mstp_api_get_msti_instance_restricted_role (char *name, char *ifName,
                                                int instance,
                                                u_int32_t *restricted_role);
int mstp_api_set_msti_instance_restricted_tcn (char *name, char *ifName,
                                               int instance, 
                                               bool_t restricted_tcn);
int mstp_api_get_msti_instance_restricted_tcn (char *name, char *ifName,
                                               int instance,
                                               u_int32_t *restricted_tcn);
int mstp_api_set_port_hello_time (char *name, char * ifName, 
                                  s_int32_t hello_time);
int mstp_api_get_port_hello_time (char *name, char * ifName,
                                  s_int32_t *hello_time);
int mstp_api_set_port_p2p (char *name, char *ifName, int is_p2p);
int mstp_api_get_port_p2p (char *name, char * ifName, int *is_p2p);
int mstp_api_set_port_path_cost (char *name, char *ifName,
                                 u_int16_t svid, u_int32_t cost);
int mstp_api_get_port_path_cost (char *name, char *ifName,
                                 u_int16_t svid, u_int32_t *cost);
int mstp_api_set_max_hops (char *name,  s_int32_t max_hops);
int mstp_api_get_max_hops (char * name, s_int32_t *max_hops);
int mstp_api_set_port_priority (char *name, char *ifName,
                                u_int16_t svid,  
                                s_int16_t priority);
int mstp_api_get_port_priority (char *name, char *ifName,
                                u_int16_t svid, s_int16_t *priority);
int mstp_api_set_port_restricted_role (char *name, char * ifName,
                                       bool_t restricted_role);
int mstp_api_get_port_restricted_role (char *name, char * ifName,
                                       s_int16_t *restricted_role);
int mstp_api_set_port_restricted_tcn (char *name, char * ifName,
                                      bool_t restricted_tcn);
int mstp_api_get_port_restricted_tcn (char *name, char * ifName,
                                      s_int16_t *restricted_tcn);
int mstp_api_set_port_rootguard (char *name, char *ifName,
                                 bool_t enabled);
int mstp_api_get_port_rootguard (char * name, char * ifName,
                                 s_int32_t *enabled);
int mstp_api_set_port_bpdufilter (char *name, char *ifName,
                                  u_char portfast_bpdufilter);
int mstp_api_get_port_bpdufilter (char * name,
                                  char * ifName,
                                  u_char *portfast_bpdufilter);
int mstp_api_disable_bridge (char * name, u_int8_t br_type, 
                             bool_t bridge_forward);
int mstp_api_enable_bridge (char * name, u_int8_t br_type);
int mstp_api_set_port_bpduguard (char *name, char *ifName,
                                 u_char portfast_bpduguard);
int mstp_api_get_port_bpduguard (char * name, char * ifName,
                                 u_char *portfast_bpduguard);
int mstp_api_set_transmit_hold_count (char *name,
                                      unsigned char txholdcount);
int mstp_api_get_transmit_hold_count (char * name, u_char *txholdcount);
int mstp_api_set_bridge_portfast_bpduguard (char *br_name,
                                            bool_t bpduguard_enabled);
int mstp_api_get_bridge_portfast_bpduguard (char *br_name, 
                                            int *bpduguard_enabled);
int mstp_api_set_bridge_errdisable_timeout_enable (char *br_name, 
                                                   bool_t enabled);
int mstp_api_get_bridge_errdisable_timeout_enable (char *br_name, 
                                                   int *enabled);
int mstp_api_set_bridge_errdisable_timeout_interval (char *br_name,
                                                     s_int32_t timeout);
int mstp_api_get_bridge_errdisable_timeout_interval (char *br_name, 
                                                     s_int32_t *timeout);
int mstp_api_set_msti_port_priority (char *name, char *ifName, 
                                     int instance,
                                     s_int16_t priority);
int mstp_api_get_msti_port_priority (char *name, 
                                     char *ifName, int instance,
                                     s_int16_t *priority);
int mstp_api_revision_number (char *name, u_int16_t rev_num);
int mstp_api_get_revision_number (char * name, u_int16_t *rev_num);
int mstp_api_set_auto_edge (char *name,  char *ifName, int to_be_enabled);
int mstp_api_get_auto_edge (char * name, char * ifName, int *enabled);
int mstp_api_region_name (char *name,  char *region_name);
int mstp_api_get_region_name (char *name, char *region_name);
/* TODO int  mstp_bridge_add (char * name, u_int8_t type,
                              bool_t ce_bridge, bool_t is_default, 
                              u_char topology_type,
                              struct mstp_port *ce_port);*/

int bridge_add_mac (char *name, char *ifname, char *mac,
                    u_int16_t vid, u_int8_t is_forward);

int bridge_delete_mac (char *name, char *ifname,
                       char *mac, u_int16_t vid,
                       u_int8_t is_forward);

int bridge_flush_dynamic_entries (char *name);

int mstp_get_spanningtree_mstdetail(char *name,
                                    u_int16_t instance,
                                    struct smi_mstp_instance_details 
                                    *details); 

int mstp_get_spanningtree_mstdetail_interface(char *name, u_int16_t instance,
                                              struct 
                                              smi_mstp_instance_port_details 
                                             *details); 

int apn_get_traffic_class_table(char *name,
                                u_char traffic_class_table  []);

int apn_get_user_priority(char *name,
                          u_int8_t *user_priority);

int mstp_api_set_bridge_portfast_bpdufilter (char *name,  bool_t enable);
int mstp_api_get_bridge_portfast_bpdufilter (char *br_name, int *enabled);

int mstp_api_mcheck (char *name, char *ifName);
int mstp_api_bridge_change_type(char * name, enum smi_bridge_type type,
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
int
mstp_api_delete_instance_vlan (char *name, int instance, u_int16_t vid);

int
mstp_check_func_type (int functype);

#endif /* _MSTP_API_SERVER_H */
