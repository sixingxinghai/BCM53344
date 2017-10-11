/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_MLD_SNOOP_H_
#define _HAL_MLD_SNOOP_H_

/*
   Name: hal_mld_snooping_if_enable

   Description:
   This API enables MLD snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_MLD_SNOOPING_ENABLE
   HAL_SUCCESS
*/
int
hal_mld_snooping_if_enable (char *name, unsigned int ifindex);



/*
   Name: hal_mld_snooping_if_disable

   Description:
   This API disables MLD snooping for the bridge.

   Parameters:
   IN -> bridge_name - bridge name

   Returns:
   HAL_ERR_MLD_SNOOPING_DISABLE
   HAL_SUCCESS
*/
int
hal_mld_snooping_if_disable (char *name, unsigned int ifindex);


/* 
   Name: hal_mld_snooping_init 

   Description: 
   This API initializes the reception of MLD packets for MLD
   snooping.

   Parameters:
   None

   Returns:
   HAL_ERR_MLD_SNOOPING_INIT
   HAL_SUCCESS 
*/
int
hal_mld_snooping_init (void);

/* 
   Name: hal_mld_snooping_deinit

   Description:
   This API deinitialized the reception of MLD packets for MLD snooping.

   Parameters:
   None

   Returns:
   HAL_ERR_MLD_SNOOPING_INIT
   HAL_SUCCESS
*/
int
hal_mld_snooping_deinit (void);

/* 
   Name: hal_mld_snooping_enable 

   Description:
   This API enables MLD snooping for the bridge.

   Parameters:
   IN -> name - bridge name

   Returns:
   HAL_ERR_MLD_SNOOPING_ENABLE
   HAL_SUCCESS
*/
int
hal_mld_snooping_enable (char *name);

/*
  Name: hal_mld_snooping_disable

  Description:
  This API disables MLD snooping for the bridge. 

  Parameters:
  IN -> name - bridge name

  Returns:
  HAL_ERR_MLD_SNOOPING_DISABLE
  HAL_SUCCESS
*/
int
hal_mld_snooping_disable (char *name);
    
/*
  Name: hal_mld_snooping_add_entry

  Description:
  This API adds a multicast entry for a (source, group) for a given VLAN.
  If the group doesn't exist, a new one will be created.
  If the group exists, the list of ports is added to the entry.

  Parameters:
  IN -> bridge_name - bridge name
  IN -> src - multicast source address
  IN -> group - multicast group address
  IN -> vid - vlan id
  IN -> svid - svlan id
  IN -> count - count of ports to add
  IN -> ifindexes - array of ports to add

  Returns:
  HAL_ERR_MLD_SNOOPING_ENTRY_ERR
  HAL_SUCCESS
*/
int
hal_mld_snooping_add_entry (char *bridge_name,
                             struct hal_in6_addr *src,
                             struct hal_in6_addr *group,
                             char is_exclude,
                             int vid,
                             int svid,
                             int count,
                             u_int32_t *ifindexes);
/*
  Name: hal_mld_snooping_delete_entry

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
  IN -> svid - svlan id
  IN -> count - count of ports to add
  IN -> ifindexes - array of ports to add

  Returns:
  HAL_ERR_MLD_SNOOPING_ENTRY_ERR
  HAL_SUCCESS
*/
int
hal_mld_snooping_delete_entry (char *bridge_name,
                               struct hal_in6_addr *src,
                               struct hal_in6_addr *group,
                               char is_exlcude,
                               int vid,
                               int svid,
                               int count,
                               u_int32_t *ifindexes);


#endif /* _HAL_MLD_SNOOP_H_ */
