/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_BRIDGE_H_
#define _HAL_BRIDGE_H_

/* Layer 2 protocol list */
typedef enum hal_l2_proto
  {
    HAL_PROTO_STP,
    HAL_PROTO_RSTP,
    HAL_PROTO_MSTP,
    HAL_PROTO_GMRP,
    HAL_PROTO_GVRP,
    HAL_PROTO_MMRP,
    HAL_PROTO_MVRP,
    HAL_PROTO_LACP,
    HAL_PROTO_DOT1X,
    HAL_PROTO_LLDP,
    HAL_PROTO_CFM,
    HAL_PROTO_MAX,
  } hal_l2_proto_t;

typedef enum hal_l2_proto_process
  {
    HAL_L2_PROTO_PEER,
    HAL_L2_PROTO_TUNNEL,
    HAL_L2_PROTO_DISCARD,
    HAL_L2_PROTO_PROCESS_MAX,
  } hal_l2_proto_process_t;

enum hal_bridge_type
  {
    HAL_BRIDGE_STP,
    HAL_BRIDGE_RSTP,
    HAL_BRIDGE_MSTP,
    HAL_BRIDGE_PROVIDER_RSTP,
    HAL_BRIDGE_PROVIDER_MSTP,
    HAL_BRIDGE_BACKBONE_RSTP,
    HAL_BRIDGE_BACKBONE_MSTP,
    HAL_BRIDGE_RPVST_PLUS,
    HAL_BRIDGE_MAX
  };

/* 
   Name: hal_bridge_init

   Description:
   This API initializes the bridging hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_BRIDGE_INIT
   HAL_SUCCESS
*/
int 
hal_bridge_init (void);

/* 
   Name: hal_bridge_deinit

   Description:
   This API deinitializes the bridging hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_BRIDGE_DEINIT
   HAL_SUCCESS
*/
int
hal_bridge_deinit (void);

/* 
   Name: hal_bridge_add 

   Description: 
   This API adds a bridge instance.

   Parameters:
   IN -> name - bridge name

   Returns:
   HAL_ERR_BRIDGE_EXISTS 
   HAL_ERR_BRIDGE_ADD_ERR 
   HAL_SUCCESS 
*/
int
hal_bridge_add (char *name, unsigned int is_vlan_aware, enum hal_bridge_type type,
                unsigned char edge, unsigned char beb, unsigned char *mac);

/* 
   Name: hal_bridge_delete

   Description:
   This API deletes a bridge instance.

   Parameters:
   IN -> name - bridge name

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS 
   HAL_ERR_BRIDGE_DELETE_ERR
   HAL_SUCCESS
*/
int
hal_bridge_delete (char *name);

/*
   Name: hal_bridge_change_vlan_type

   Description:
   This API changes the type of the bridge.

   Parameters:
   IN -> name - bridge name
   IN -> is_vlan_aware - vlan aware
   IN -> type - type of bridge

   Returns:
   HAL_SUCCESS
   NEGATIVE VALUE IS RETURNED
*/
int
hal_bridge_change_vlan_type (char *name, int is_vlan_aware,
                             u_int8_t type);

/* 
   Name: hal_bridge_set_state

   Description:
   This API sets the state of the bridge. If the bridge is disabled
   it behaves like a dumb switch.

   Parameters:
   IN -> name - bridge name
   IN -> state - enable/disable spanning tree

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_bridge_set_state (char *name, u_int16_t enable);

/* 
   Name: hal_bridge_set_ageing_time

   Description:
   This API sets the ageing time for a bridge.

   Parameters:
   IN -> name - bridge name
   IN -> ageing_time - Ageing time in seconds.

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_set_ageing_time (char *name, u_int32_t ageing_time);
int
hal_bridge_disable_ageing(char *name);
/* 
   Name: hal_bridge_set_iearning

   Description:
   This API sets the learning for a bridge.

   Parameters:
   IN -> name - bridge name

   Returns:
   HAL_ERR_BRIDGE_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_set_learning (char *name, int learning);

/* 
   Name: hal_bridge_add_port

   Description:
   This API adds a port to a bridge. 
   
   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index of port

   Returns:
   HAL_ERR_BRIDGE_PORT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_add_port (char *name, unsigned int ifindex);

/* 
   Name: hal_bridge_delete_port

   Description:
   This API deletes a port from a bridge. 

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index of port

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_delete_port (char *name, int ifindex);

/* 
   Name: hal_bridge_set_port_state

   Description:
   This API sets port state of a bridge port. 

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index of port
   IN -> instance - 
   IN -> state    - port state

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_set_port_state (char *bridge_name,
                           int ifindex, int instance, int state);

/* 
   Name: hal_bridge_add_instance

   Description:
   This API adds a instance to a bridge. 

   Parameters:
   IN -> name - bridge name
   IN -> instance - instance number

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_add_instance (char * name, int instance);

/* 
   Name: hal_bridge_delete_instance

   Description:
   This API deletes the instance from the bridge.

   Parameters:
   IN -> name - bridge name
   IN -> instance - instance number

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_bridge_delete_instance (char * name, int instance);

/* 
   Name: hal_bridge_add_vlan_to_instance

   Description:
   This API adds a VLAN to a instance in a bridge.

   Parametes:
   IN -> name - bridge name
   IN -> instance - instance number
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_ERR_BRIDGE_VLAN_NOT_FOUND
   HAL_SUCCESS
*/
int
hal_bridge_add_vlan_to_instance (char * name, int instance, 
                                 unsigned short vid);

/* 
   Name: hal_bridge_delete_vlan_from_instance

   Description:
   This API deletes a VLAN from a instance in a bridge. 

   Parameters:
   IN -> name - bridge name
   IN -> instance - instance number
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_ERR_BRIDGE_VLAN_NOT_FOUND
   HAL_SUCCESS
*/
int
hal_bridge_delete_vlan_from_instance (char * name, int instance, 
                                      unsigned short vid);

/*
   Name: hal_bridge_set_learn_fwd

   Description:
   This API sets the learn and forwarding flag for a port.

   Parameters:
   IN -> bridge_name - bridge name
   IN -> ifindex  - interface index of the port.
   IN -> instance - instance number
   IN -> learn - learning to be enabled
   IN -> forward - forwarding to be enabled

   Returns:
   HAL_ERR_BRIDGE_INSTANCE_NOT_EXISTS
   HAL_ERR_BRIDGE_VLAN_NOT_FOUND
   HAL_SUCCESS
*/
int
hal_bridge_set_learn_fwd (const char *const bridge_name,const int ifindex,
                          const int instance, const int learn, 
                          const int forward);

/*
   Name: hal_bridge_set_proto_process_port 

   Description:
   This API configures a particular protocol data units to be tunnelled
   or dicarded in a customer facing port.

   Parameters:
   IN -> bridge_name - bridge name
   IN -> ifindex  - interface index of the port.
   IN -> proto - Protocols whose PDUs have to be discarded/tunnelled
   IN -> discard - Whether PDUs have to discarded/tunnelled

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_bridge_set_proto_process_port (const char *const bridge_name,
                                   const int ifindex,
                                   enum hal_l2_proto proto,
                                   enum hal_l2_proto_process process,
                                   u_int16_t vid);

/*
   Name: hal_l2_qos_set_cos_preserve

   Description:
   This API configures CVLAN COS Preservation for a Port and CVID.

   Parameters:
   IN -> ifindex          - interface index of the port.
   IN -> cvid             -  CVID For which the COS Preservation is to be Set.
   IN -> preserve_ce_cos  -  Preserve the Customer VLAN COS.

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/
#ifdef HAVE_I_BEB

int
hal_pbb_dispatch_service_cnp(char *br_name, unsigned int ifindex, unsigned isid,
                         unsigned short svid_h, unsigned short svid_l,
                         unsigned char srv_type);


int
hal_pbb_dispatch_service_vip(char *br_name, unsigned int ifindex, unsigned isid,
                             unsigned char* macaddr, unsigned char srv_type);
 
int
hal_pbb_dispatch_service_pip(char *br_name, unsigned int ifindex, unsigned isid);

#endif

#ifdef HAVE_B_BEB
int
hal_pbb_dispatch_service_cbp(char *br_name, unsigned int ifindex,
                             unsigned short bvid, unsigned int e_isid,
                             unsigned int l_isid, unsigned char *default_dst_bmac,
                             unsigned char srv_type);

#endif
 
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)

int
hal_pbb_remove_service(char *br_name, unsigned int ifindex, unsigned isid);

#endif

int
hal_l2_qos_set_cos_preserve (const int ifindex,
                             u_int16_t vid,
                             u_int8_t preserve_ce_cos);

int
hal_ip_set_access_group (struct hal_ip_access_grp access_grp,
                         char *ifname,
                         int action, int dir);
int
hal_ip_set_access_group_interface (struct hal_ip_access_grp access_grp,
                                   char *vifname, char *ifname,
                                   int action, int dir);

int
hal_l2_get_index_by_mac_vid (char *bridge_name, int *ifindex, char *mac, 
                             u_int16_t vid);

int
hal_l2_get_index_by_mac_vid_svid (char *bridge_name, int *ifindex, char *mac, 
                             u_int16_t vid, u_int16_t svid);


#endif
