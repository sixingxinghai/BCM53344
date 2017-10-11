/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_IGMP_SNOOP_H_
#define _HAL_IGMP_SNOOP_H_

/* Macro to Convert IPv4 Multicast Addr into MAC Addr */
#define HAL_CONVERT_IPV4MCADDR_TO_MAC(IPV4MCA, MAC)                   \
do {                                                                  \
  ((u_int8_t *) (MAC)) [0] = 0x01;                                    \
  ((u_int8_t *) (MAC)) [1] = 0x00;                                    \
  ((u_int8_t *) (MAC)) [2] = 0x5E;                                    \
  ((u_int8_t *) (MAC)) [3] = (((u_int8_t *) (IPV4MCA)) [1] & 0x7F);   \
  ((u_int8_t *) (MAC)) [4] = ((u_int8_t *) (IPV4MCA)) [2];            \
  ((u_int8_t *) (MAC)) [5] = ((u_int8_t *) (IPV4MCA)) [3];            \
} while (0)


/*
   Name: hal_igmp_snooping_if_enable

   Description:
   This API enables IGMP snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_IGMP_SNOOPING_ENABLE
   HAL_SUCCESS
*/
int
hal_igmp_snooping_if_enable (char *name, unsigned int ifindex);




/*
   Name: hal_igmp_snooping_if_disable

   Description:
   This API disables IGMP snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_IGMP_SNOOPING_DISABLE
   HAL_SUCCESS
*/
int
hal_igmp_snooping_if_disable (char *name, unsigned int ifindex);



/*
  Name: hal_igmp_snooping_init 

  Description: 
  This API initializes the reception of IGMP packets for IGMP
  snooping.

  Parameters:
  None

  Returns:
  HAL_ERR_IGMP_SNOOPING_INIT
  HAL_SUCCESS 
*/
int
hal_igmp_snooping_init (void);

/* 
   Name: hal_igmp_snooping_deinit

   Description:
   This API deinitialized the reception of IGMP packets for IGMP snooping.

   Parameters:
   None

   Returns:
   HAL_ERR_IGMP_SNOOPING_INIT
   HAL_SUCCESS
*/
int
hal_igmp_snooping_deinit (void);

/* 
   Name: hal_igmp_snooping_enable 

   Description:
   This API enables IGMP snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_IGMP_SNOOPING_ENABLE
   HAL_SUCCESS
*/
int
hal_igmp_snooping_enable (char *bridge_name);

/*
  Name: hal_igmp_snooping_disable

  Description:
  This API disables IGMP snooping for the bridge. 

  Parameters:
  IN -> bridge_name      - bridge name

  Returns:
  HAL_ERR_IGMP_SNOOPING_DISABLE
  HAL_SUCCESS
*/
int
hal_igmp_snooping_disable (char *bridge_name);

/*
  Name: hal_igmp_snooping_add_entry

  Description:
  This API adds a multicast entry for a (source, group) for a given VLAN.
  If the group doesn't exist, a new one will be created.
  If the group exists, the list of ports is added to the entry.

  Parameters:
  IN -> bridge_name - bridge name
  IN -> src - multicast source address
  IN -> group - multicast group address
  IN -> vid - vlan id
  IN -> count - count of ports to add
  IN -> ifindexes - array of ports to add

  Returns:
  HAL_ERR_IGMP_SNOOPING_ENTRY_ERR
  HAL_SUCCESS
*/
int
hal_igmp_snooping_add_entry (char *bridge_name,
                             struct hal_in4_addr *src,
                             struct hal_in4_addr *group,
                             char is_exclude,
                             int vid,
                             int svid,
                             int count,
                             u_int32_t *ifindexes);
/*
  Name: hal_igmp_snooping_delete_entry

  Description:
  This API deletes a multicast entry for a (source, group) for a given VLAN.
  If the group doesn't exist, a error will be returned.
  If the group exists, the list of ports are deleted from the multicast entry.
  If it is the last port for the multicast entry, the multicast entry is deleted
  as well.

  Parameters:
  IN -> bridge_name - bridge name
  IN -> src - multicast source address
  IN -> group - multicast group address
  IN -> vid - vlan id
  IN -> count - count of ports to add
  IN -> ifindexes - array of ports to add

  Returns:
  HAL_ERR_IGMP_SNOOPING_ENTRY_ERR
  HAL_SUCCESS
*/
int
hal_igmp_snooping_delete_entry (char *bridge_name,
                                struct hal_in4_addr *src,
                                struct hal_in4_addr *group,
                                char is_exclude,
                                int vid,
                                int svid,
                                int count,
                                u_int32_t *ifindexes);








#endif /* _HAL_IGMP_SNOOP_H_ */
