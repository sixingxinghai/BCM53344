/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_incl.h"
#include "hal_error.h"
#include "hal_comm.h"
#include "hal_ipifwd.h"

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
hal_l2_fdb_init (void)
{
  return  HAL_SUCCESS;
}

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
hal_l2_fdb_deinit (void)
{
  return  HAL_SUCCESS;
}

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
   HAL_L2_FDB_ENTRY
   number of entries. Can be 0 for no entries.
*/
int
hal_l2_fdb_unicast_get (char *name, char *mac_addr, unsigned short vid,
                        unsigned short count, struct hal_fdb_entry *fdb_entry)
{
  int ret;

  ret = hal_ioctl (APNBR_GET_UNICAST_ENTRIES, (unsigned long)name,
                   (unsigned long) mac_addr, vid, count,
                   (unsigned long) fdb_entry, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return ret;

}

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
   HAL_L2_FDB_ENTRY
   number of entries. Can be 0 for no entries.
*/
int
hal_l2_fdb_multicast_get (char *name, char *mac_addr, unsigned short vid,
                          unsigned short count, struct hal_fdb_entry *fdb_entry)
{
  int ret;

  ret = hal_ioctl (APNBR_GET_MULTICAST_ENTRIES, (unsigned long)name,
                   (unsigned long) mac_addr, vid, count,
                   (unsigned long) fdb_entry, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return ret;

}

/*
   Name: hal_bridge_flush_dynamic_fdb_by_mac
                                                                                                                             
   Description:
   This API deletes all the dynamic entries with a given mac address.

   Parameters:
   IN -> name - bridge name
   IN -> mac  - mac address of the dynamic entry to be deleted.

   Returns:
   HAL_SUCCESS. 
   errno from the system call.
*/
int
hal_bridge_flush_dynamic_fdb_by_mac (char *bridge_name, const unsigned char * const mac, int maclen)
{
  int ret;

  ret = hal_ioctl (APNBR_CLEAR_FDB_BY_MAC, (unsigned long)bridge_name,
                   (unsigned long)mac, 0, 0, 0, 0);

  if (ret < 0)
    {
      return -errno;
    }

  return ret;

}

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
                         unsigned char priority)
{
  return  HAL_SUCCESS;
}
