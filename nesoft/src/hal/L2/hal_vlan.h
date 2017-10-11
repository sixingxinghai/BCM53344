/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_VLAN_H_
#define _HAL_VLAN_H_

/* 
   Name: hal_vlan_init

   Description:
   This API initializes the VLAN hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_VLAN_INIT
   HAL_SUCCESS
*/
int
hal_vlan_init (void);

/* 
   Name: hal_vlan_deinit

   Description:
   This API deinitializes the VLAN hardware layer component.

   Parameters:
   None

   Returns:
   HAL_ERR_VLAN_DEINIT
   HAL_SUCCESS
*/
int
hal_vlan_deinit (void);

/* 
   Name: hal_vlan_add 

   Description: 
   This API adds a VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> type - VLAN type
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_VLAN_EXISTS 
   HAL_SUCCESS 
*/
int
hal_vlan_add (char *name, enum hal_vlan_type type, unsigned short vid);

/* 
   Name: hal_vlan_delete

   Description:
   This API deletes a VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> type - VLAN type
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_delete (char *name, enum hal_vlan_type type, unsigned short vid);

/* 
   Name: hal_vlan_set_port_type

   Description:
   This API sets the acceptable frame type for a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> port_type - trunk, access or hybrid
   IN -> acceptable_frame_type - acceptable frame type
   IN -> enable_ingress_filter - enable ingress filtering

   Returns:
   HAL_ERR_VLAN_FRAME_TYPE
   HAL_SUCCESS
*/
int
hal_vlan_set_port_type (char *name, 
                        unsigned int ifindex, 
                        enum hal_vlan_port_type port_type,
                        enum hal_vlan_port_type sub_port_type,
                        enum hal_vlan_acceptable_frame_type acceptable_frame_types,
                        unsigned short enable_ingress_filter);


/* 
   Name: hal_vlan_set_default_pvid

   Description:
   This API sets the default PVID for a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> pvid - default PVID
   IN -> egress - egress tagged/untagged

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_set_default_pvid (char *name, unsigned int ifindex,
                           unsigned short pvid, 
                           enum hal_vlan_egress_type egress);

/* 
   Name: hal_vlan_add_vid_to_port

   Description:
   This API adds a VLAN to a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - VLAN id
   IN -> egress - egress tagged/untagged

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_add_vid_to_port (char *name, unsigned int ifindex,
                          unsigned short vid, 
                          enum hal_vlan_egress_type egress);

/* 
   Name: hal_vlan_delete_vid_from_port

   Description:
   This API deletes a VLAN from a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_delete_vid_from_port (char *name, unsigned int ifindex,
                               unsigned short vid);

/*
   Name: hal_vlan_port_set_dot1q_state

   Description:
   This API sets the dot1q state on a port.

   Parameters:
   IN -> ifindex - interface index
   IN -> enable - To enable or Disbale dot1q
   IN -> enable_ingress_filter - enable ingress filtering

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_port_set_dot1q_state (unsigned int ifindex, unsigned short enable,
                               unsigned short enable_ingress_filter);

/*
   Name: hal_vlan_add_cvid_to_port

   Description:
   This API adds a CVLAN to a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - C VLAN id
   IN -> svid - S VLAN id
   IN -> egress - egress tagged/untagged

   Returns:
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_SUCCESS
*/

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP 
/*

   Name: hal_l2_unknown_mcast_mode

   Description:
   This API sets default mode (i.e. flooding for unlearned multicast ) of vlan for the bridge .

   Parameters:
   IN -> status - flood/discard

   Returns:
   HAL_ERR_L2_UNKNOWN_MCAST
   HAL_SUCCESS
*/
int
hal_l2_unknown_mcast_mode (int);

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

int
hal_vlan_add_cvid_to_port (char *name, unsigned int ifindex,
                           unsigned short cvid,
                           unsigned short svid,
                           enum hal_vlan_egress_type egress);


/* 
   Name: hal_vlan_delete_cvid_from_port

   Description:
   This API deletes a VLAN from a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - C VLAN id
   IN -> svid - S VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/
int
hal_vlan_delete_cvid_from_port (char *name, unsigned int ifindex,
                                unsigned short cvid,
                                unsigned short svid);

/*
   Name: hal_vlan_create_cvlan

   Description:
   This API creates a mapping between C-VLAN to S-VLAN and creates a
   corresponding Internal VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_create_cvlan (char *name, unsigned short cvid,
                       unsigned short svid);

/*
   Name: hal_vlan_delete_cvlan

   Description:
   This API deletes a mapping between C-VLAN to S-VLAN.

   Parameters:
   IN -> name - bridge name
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_delete_cvlan (char *name, unsigned short cvid,
                       unsigned short svid);

/*
   Name: hal_vlan_create_cvlan_registration_entry

   Description:
   This API creates a mapping between C-VLAN to S-VLAN on a CE port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_create_cvlan_registration_entry (char *name, unsigned int ifindex,
                                          unsigned short cvid,
                                          unsigned short svid);

/*
   Name: hal_vlan_delete_cvlan_registration_entry

   Description:
   This API deletes a mapping between C-VLAN to S-VLAN on a CE port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> cvid - VLAN cid
   IN -> svid - VLAN sid

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_delete_cvlan_registration_entry (char *name, unsigned int ifindex,
                                          unsigned short cvid,
                                          unsigned short svid);

/*
   Name: hal_vlan_create_vlan_trans_entry

   Description:
   This API creates a translation from VLAN1 to VLAN 2 on a port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - VLAN id to be translated
   IN -> trans_vid - Translated VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_create_vlan_trans_entry (char *name, unsigned int ifindex,
                                   unsigned short vid,
                                   unsigned short trans_vid);

/*
   Name: hal_vlan_delete_svlan_trans_entry

   Description:
   This API deletes a translation from SVLAN1 to SVLAN 2 on a CN port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> vid - S-VLAN id to be translated
   IN -> trans_vid - Translated S-VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_delete_vlan_trans_entry (char *name, unsigned int ifindex,
                                  unsigned short vid,
                                  unsigned short trans_vid);

/*
   Name: hal_vlan_set_native_vid

   Description:
   This API Configures the native vlan for the trunk port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> native_vid - Native VLAN id

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_set_native_vid (char *name,
                         unsigned int ifindex,
                         unsigned short native_vid);

/*
   Name: hal_vlan_set_pro_edge_pvid

   Description:
   This API Configures the PVID for the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port
   IN -> pvid - VLAN id Used for Untagged Packets.

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_set_pro_edge_pvid (char *name, unsigned int ifindex,
                            unsigned short svid,
                            unsigned short pvid);

/*
   Name: hal_vlan_set_pro_edge_untagged_vid

   Description:
   This API Configures the Untagged VID for the Egress for
   the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port
   IN -> untagged_vid - VLAN id that will be transmitted
                        untagged in the provider Edge Port.

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_set_pro_edge_untagged_vid (char *name, unsigned int ifindex,
                                    unsigned short svid,
                                    unsigned short untagged_vid);

/*
   Name: hal_vlan_add_pro_edge_port

   Description:
   This API Configures the PVID for the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_add_pro_edge_port (char *name, unsigned int ifindex,
                            unsigned short svid);

/*
   Name: hal_vlan_del_pro_edge_port

   Description:
   This API Configures the Untagged VID for the Egress for
   the Provider Edge Port.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - interface index
   IN -> svid - VLAN id of the Provider Edge Port

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_ERR_VLAN_NOT_EXISTS
   HAL_SUCCESS
*/

int
hal_vlan_del_pro_edge_port (char *name, unsigned int ifindex,
                            unsigned short svid);

/*
   Name: hal_pro_vlan_set_dtag_mode

   Description:
   This API Configures double tag mode

   Parameters:
   IN -> ifindex - interface index
   IN -> dtag mode - Whether it is a single tag port or a double tag
                     port.

   Returns:
   HAL_ERR_VLAN_PORT_NOT_EXISTS
   HAL_SUCCESS
*/

int hal_pro_vlan_set_dtag_mode (unsigned int ifindex,
                                unsigned short dtag_mode);

#endif /* _HAL_VLAN_H_ */
