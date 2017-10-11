/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_SMI_SERVER_H
#define _NSM_SMI_SERVER_H

#include "smi_server.h"
#include "nsmd.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_debug.h"
#include "nsm_interface.h"
#include "nsm_qos.h"
#include "nsm_qos_api.h"
#include "nsm_qos_list.h"
#include "nsm_l2_qos.h"
#include "nsm_lacp.h"
#include "smi_nsm_fm.h"
#ifdef HAVE_HA
#include "nsm_cal.h"
#endif /*HAVE_HA*/

#ifdef HAVE_SMI

extern struct nsm_qos_global qosg;

#define BIT_BYTE_CONV 8
#define SMI_CPU_PORT "fe6"

#define SMI_FLOWCTRL_DIR_CHECK                                              \
      switch(msg->dir)                                                      \
      {                                                                     \
        case SMI_FLOW_CONTROL_OFF:                                          \
          dir = HAL_FLOW_CONTROL_OFF;                                       \
        break;                                                              \
        case SMI_FLOW_CONTROL_SEND:                                         \
          dir = HAL_FLOW_CONTROL_SEND;                                      \
        break;                                                              \
        case SMI_FLOW_CONTROL_RECEIVE:                                      \
          dir = HAL_FLOW_CONTROL_RECEIVE;                                   \
        break;                                                              \
        case SMI_FLOW_CONTROL_BOTH:                                         \
          dir = HAL_FLOW_CONTROL_SEND | HAL_FLOW_CONTROL_RECEIVE;           \
        break;                                                              \
        default:                                                            \
          return (nsm_smi_server_send_sync_message (ase, SMI_MSG_STATUS,    \
                                                    SMI_ERROR, NULL));      \
      }


struct smi_server * nsm_smi_server_init (struct lib_globals *zg);
int nsm_smi_server_send_sync_message(struct smi_server_entry *ase, int msgtype, int status, void *msg);
struct nsm_master *nsm_master_lookup_by_id (struct lib_globals *, u_int32_t);
struct nsm_bridge_master * nsm_bridge_get_master(struct nsm_master *nm);
void smi_vlan_dump(struct lib_globals *zg, struct smi_msg_vlan *msg);
void smi_gvrp_dump(struct lib_globals *zg, struct smi_msg_gvrp *msg);
/** Interface function prototypes **/
int nsm_smi_server_recv_if(struct smi_msg_header *header, void *arg, void *message);
void smi_interface_dump(struct lib_globals *zg, struct smi_msg_if *msg);
int nsm_if_duplex_set (u_int32_t, char *, int);
int nsm_if_duplex_get (u_int32_t , char *, int *);
int nsm_if_flag_up_set (u_int32_t, char *, bool_t iterate_members);
int nsm_if_flag_up_unset (u_int32_t, char *, bool_t iterate_members);
int nsm_if_flag_multicast_set (u_int32_t, char *);
int nsm_if_flag_multicast_get (u_int32_t, char *, int *);
int nsm_if_flag_multicast_unset (u_int32_t, char *);
int nsm_if_mtu_set (u_int32_t, char *, int);
int nsm_fea_if_get_mtu (struct interface *ifp, int *mtu);
int nsm_interface_get_counters(struct interface *ifp, 
                               struct smi_if_stats *ifstats);
int nsm_interface_clear_counters (struct interface *ifp);
int nsm_smi_bridge_get_type (struct nsm_bridge_master *master,
                             char * name, enum smi_bridge_type *type,
                             enum smi_bridge_topo_type *topo_type);
int nsm_if_set_bandwidth (struct interface *ifp, float bw);

/** VLAN function prototypes */
int nsm_smi_server_recv_vlan (struct smi_msg_header *header, 
                              void *arg, void *message);
/**
int nsm_vlan_delete (struct nsm_bridge_master *master, char *brname,
                     u_int16_t vid, u_char type);
**/

/** GvRP function prototypes */
int nsm_smi_server_recv_gvrp (struct smi_msg_header *header, 
                              void *arg, void *message);
int gvrp_set_timer (struct nsm_bridge_master *master,
                    struct interface *ifp,
                    const u_int32_t timer_type,
                    const pal_time_t timer_value);

int gvrp_get_timer (struct nsm_bridge_master *master,
                    struct interface *ifp,
                    const u_int32_t timer_type,
                    const pal_time_t *timer_value);

int gvrp_enable (struct nsm_bridge_master *master, char* reg_type, 
                 const char* const bridge_name);

int gvrp_disable (struct nsm_bridge_master *master, char* bridge_name);

int gvrp_enable_port (struct nsm_bridge_master *master, struct interface *ifp);

int gvrp_disable_port (struct nsm_bridge_master *master, struct interface *ifp);
int gvrp_set_registration (struct nsm_bridge_master *master, 
                           struct interface *ifp,
                           u_int32_t reg_mode);
int gvrp_get_registration (struct nsm_bridge_master *master, 
                           struct interface *ifp,
                           u_int32_t *reg_mode);
int gvrp_get_per_vlan_statistics_details (struct nsm_bridge_master *master,
                                          const char* const bridge_name,
                                          const u_int16_t vid,
                                          u_int32_t *receive_counters,
                                          u_int32_t *transmit_counters);
int gvrp_smi_clear_all_statistics (struct nsm_bridge_master *master);

int gvrp_dynamic_vlan_learning_set (struct nsm_bridge_master *master, 
                                    char *br_name, bool_t vlan_learning_enable);
int gvrp_get_configuration_bridge(struct nsm_bridge_master *master, 
                              char *bridge_name,  char* reg_type,
                              struct gvrp_bridge_configuration *gvrp_br_config);
int gvrp_get_port_statistics(struct nsm_bridge_master *master,
                             struct interface *ifp,
                             struct smi_gvrp_statistics *gvrp_stats);
int gvrp_get_vid_details (struct nsm_bridge_master *master,
                          struct interface *ifp,
                          u_int32_t first_call,
                          u_int32_t gid_idx,
                          struct smi_gvrp_vid_detail *gvrp_vid_detail);

int
gvrp_get_timer_details (struct nsm_bridge_master *master,
                        struct interface *ifp,
                        pal_time_t *timer_details);

/* QOS function Prototype */
int nsm_smi_server_recv_qos (struct smi_msg_header *header,
                              void *arg, void *message);
void smi_qos_dump(struct lib_globals *zg, struct smi_msg_qos *msg);
int nsm_vlan_port_set_default_user_priority (struct interface *ifp, 
                                             unsigned char user_priority);
int nsm_vlan_port_set_regen_user_priority (struct interface *ifp, 
                                           unsigned char user_priority,
                                           unsigned char regen_user_priority);

/* FC function Prototype */
int nsm_smi_server_recv_fc (struct smi_msg_header *header,
                              void *arg, void *message);
void smi_fc_dump(struct lib_globals *zg, struct smi_msg_flowctrl *msg);
int port_add_flow_control (struct interface *ifp,
                           unsigned char direction);
int nsm_if_mdix_crossover_set(struct interface *ifp, u_int32_t mdix);
int nsm_if_mdix_crossover_get(struct interface *ifp, u_int32_t *mdix);
int nsm_bridge_flush_dynamic_fdb_by_mac(char *name, char *mac_addr);
int nsm_apn_get_traffic_class_table(struct interface *ifp, 
            u_char traffic_class_table[][SMI_BRIDGE_MAX_TRAFFIC_CLASS + 1]);
int get_flow_control_statistics (struct interface *ifp, 
                                 enum smi_flow_control_dir *direction,
                                 int *rxpause, int *txpause);
int port_delete_flow_control (struct interface *ifp, unsigned char direction);
int flow_control_get_interface (struct interface *ifp,
                                enum smi_flow_control_dir *direction);
int
nsm_check_func_type(int functype);
int
smi_record_vlan_addtoport_alarm (char *ifname, struct smi_vlan_bmp vlan_bmp, 
                                 struct smi_vlan_bmp egr_bmp);
int
smi_record_vlan_delfromport_alarm (char *ifname, struct smi_vlan_bmp vlan_bmp);

#endif /* HAVE_SMI */
#endif /* _NSM_SMI_SERVER_H */
