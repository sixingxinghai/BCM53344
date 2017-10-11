/* Copyright (C) 2003  All Rights Reserved. */
   
#ifndef _NSM_L2_QOS_H_
#define _NSM_L2_QOS_H_

#define L2_PORT_CFG_TRF_CLASS         (1 << 0)
#define L2_PORT_CFG_DEF_USR_PRI       (1 << 1)
#define L2_PORT_CFG_REGEN_USR_PRI     (1 << 2)
#define L2_PORT_CFG_VLAN_CLASS_GRP    (1 << 3)

#define TRAFFIC_CLASS_VALUE_MIN 0
#define TRAFFIC_CLASS_VALUE_MAX 7

#define DOT1DTRAFFICCLASS_ENABLED   1
#define DOT1DTRAFFICCLASS_DISABLED  2

int nsm_vlan_port_set_traffic_class_table (struct interface *ifp, 
                                           unsigned char user_priority,
                                           unsigned char num_traffic_classes,
                                           unsigned char traffic_class_value);

int nsm_vlan_port_set_default_user_priority (struct interface *ifp, 
                                             unsigned char user_priority);

int nsm_vlan_port_set_regen_user_priority (struct interface *ifp, 
                                           unsigned char user_priority,
                                           unsigned char regen_user_priority);

int nsm_vlan_port_get_default_user_priority (struct interface *ifp);
int
nsm_vlan_port_get_regen_user_priority (struct interface *ifp,
                                         unsigned char user_priority );

s_int32_t nsm_bridge_set_traffic_class (struct nsm_bridge *bridge,
                                        u_char traffic_class_enabled);

int
nsm_vlan_port_set_traffic_class_status (int ifindex,
                                         unsigned char traffic_class_enabled);
int nsm_vlan_port_get_traffic_class_table (int ifindex,
                                           unsigned char user_priority,
                                           unsigned char num_traffic_classes,
                                           unsigned char traffic_class_value);
int
nsm_qos_set_num_cosq(s_int32_t num_cosq);

void
nsm_qos_get_num_cosq(s_int32_t *num_cosq);

#ifdef HAVE_SMI

int
nsm_set_preserve_ce_cos (struct interface *ifp);

#endif /* HAVE_SMI */

#endif /* _L2_VLAN_API_H_ */
