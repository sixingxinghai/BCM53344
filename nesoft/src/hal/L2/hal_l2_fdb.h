/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_L2_FDB_H_
#define _HAL_L2_FDB_H_

/* Function pointer to hal_l2_fdb_unicast_get/ hal_l2_fdb_multicast_get functions */

typedef int (*hal_get_fdb_func_t) (char *name, char *mac_addr, unsigned short vid,
                                   unsigned short count, struct hal_fdb_entry *fdb_entry);



/* 
   Name: hal_l2_fdb_init

   Description:
   This API initializes the L2 Forwarding table. 

   Parameters:
   None.

   Returns:
   HAL_ERR_L2_FDB_INIT
   HAL_SUCCESS
*/
int
hal_l2_fdb_init (void);

/* 
   Name: hal_l2_fdb_deinit

   Description:
   This API deinitializes the L2 Forwarding table. 

   Parameters:
   None.

   Returns:
   HAL_ERR_L2_FDB_INIT
   HAL_SUCCESS
*/
int
hal_l2_fdb_deinit (void);

/* 
   Name: hal_l2_add_fdb

   Description:
   This API adds a L2 fdb entry. If the flags parameter is set to HAL_L2_FDB_STATIC
   then the entry is added as static and will not age out. If flags parameter is 0,
   then the entry is added as a dynamic entry and will undergo ageing.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port ifindex
   IN -> vid - VLAN id
   IN -> svid - SVLAN id
   IN -> mac - mac address
   IN -> len - mac address length
   IN -> flags - flags

   Returns:
   HAL_ERR_L2_FDB_ENTRY_EXISTS
   HAL_ERR_L2_FDB_ENTRY
   HAL_SUCCESS
*/
int
hal_l2_add_fdb (const char * const name, unsigned int ifindex, 
                const unsigned char * const mac, int len,
                unsigned short vid, unsigned short svid,
                unsigned char flags, bool_t is_forward);

/* 
   Name: hal_l2_del_fdb

   Description:
   This API deletes a L2 fdb entry. If the flags parameter is set to 
   HAL_L2_FDB_STATIC then the entry is added as static and will not 
   age out. If flags parameter is 0, then the entry is added as a 
   dynamic entry and will undergo ageing.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port ifindex
   IN -> vid - VLAN id
   IN -> svid - SVLAN id
   IN -> mac - mac address
   IN -> flags - flags

   Returns:
   HAL_ERR_L2_FDB_NO_ENTRY
   HAL_ERR_L2_FDB_ENTRY
   HAL_SUCCESS
*/
int
hal_l2_del_fdb (const char * const name, unsigned int ifindex, 
                const unsigned char * const mac, int len, 
                unsigned short vid, unsigned short svid,
                unsigned char flags);

/* 
   Name: hal_bridge_flush_fdb_by_port

   Description:
   This API flushes all fdb entries for a port. When ifindex is 0, it flushes all
   entries for the vlan on all ports. When vlan id is null, it flushes all entries
   for the port.
   
   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port ifindex
   IN -> vid - VLAN id
   IN -> instance - mstp instance identifier
   IN -> svid - SVLAN id

   Returns:
   HAL_ERR_L2_FDB_NO_ENTRY
   HAL_ERR_L2_FDB_ENTRY
   HAL_SUCCESS
*/
int
hal_bridge_flush_fdb_by_port (char *bridge_name,
                              unsigned int ifindex, unsigned int instance, 
                              unsigned short vid, unsigned short svid);

/* 
   Name: hal_l2_fdb_unicast_get

   Description:
   This API gets the unicast hal_fdb_entry starting at the next address of mac 
   address and vid specified. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> name - bridge name
   IN -> mac_addr - the FDB entry after this mac address
   IN -> vid - the FDB entry after this vid
   IN -> count - the number of FDB entries to be returned
   OUT -> fdb_entry - the array of fdb_entries returned

   Returns:
   HAL_ERR_L2_FDB_ENTRY
   number of entries. Can be 0 for no entries.
*/
int
hal_l2_fdb_unicast_get (char *name, char *mac_addr, unsigned short vid,
                        unsigned short count, struct hal_fdb_entry *fdb_entry);

/* 
   Name: hal_l2_fdb_multicast_get

   Description:
   This API gets the multicast hal_fdb_entry starting at the next address of mac 
   address and vid specified. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> name - bridge name
   IN -> mac_addr - the FDB entry after this mac address
   IN -> vid - the FDB entry after this vid
   IN -> count - the number of FDB entries to be returned
   OUT -> fdb_entry - the array of fdb_entries returned

   Returns:
   HAL_ERR_L2_FDB_ENTRY
   number of entries. Can be 0 for no entries.
*/
int
hal_l2_fdb_multicast_get (char *name, char *mac_addr, unsigned short vid,
                          unsigned short count, struct hal_fdb_entry *fdb_entry);

/*
   Name: hal_bridge_flush_dynamic_fdb_by_mac
                                                                                                                             
   Description:
   This API deletes all the dynamic entries with a given mac address.
                                                                                                                             
   Parameters:
   IN -> name - bridge name
   IN -> mac  - mac address of the dynamic entry to be deleted.
                                                                                                                             
   Returns:
   HAL_SUCCESS.
*/
int
hal_bridge_flush_dynamic_fdb_by_mac (char *bridge_name,
                                     const unsigned char * const mac,
                                     int maclen);


/* 
   Name: hal_l2_add_priority_ovr

   Description:
   This API adds a L2 fdb entry with priority override entry.

   Parameters:
   IN -> name - bridge name
   IN -> ifindex - port ifindex
   IN -> vid - VLAN id
   IN -> mac - mac address
   IN -> len - mac address length
   IN -> ovr_mac_type - Type of the ATU entry
   IN -> priority - priority

   Returns:
   HAL_L2_FDB_ENTRY_EXISTS
   HAL_L2_FDB_ENTRY
   HAL_SUCCESS
*/
int
hal_l2_add_priority_ovr (const char * const name, unsigned int ifindex,
                         const unsigned char * const mac, int len,
                         unsigned short vid,
                         unsigned char ovr_mac_type,
                         unsigned char priority);

/*
   Name: hal_l2_get_index_by_mac_vid

   Description:
   This API Gets the egress ifindex for the given destination MAC

   Parameters:
   IN  -> bridge_name      -  Name of the bridge
   OUT -> ifindex          -  interface index of the port on which the mac
                              was learnt.
   IN  -> vid              -  VID For which the the mac was learnt.
   IN  -> mac              -  Destination MAC.

   Returns:
   HAL_ERR_BRIDGE_PORT_NOT_EXISTS
   HAL_SUCCESS
*/


int
hal_l2_get_index_by_mac_vid (char *bridge_name, int *ifindex, char *mac,
                             u_int16_t vid);

#endif /* _HAL_L2_FDB_H_ */
