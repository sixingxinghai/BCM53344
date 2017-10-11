/* Copyright (C) 2005  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_types.h"
#include "hal_ipv4_arp.h"



/*
   Name: hal_arp_entry_del
   
   Description:
   This API deletes an arp entry
   
   Parameters:
   IN -> ip_addr - IP address
   IN -> mac_addr - Ethernet address

   Returns:
   HAL_ERR_VLAN_CLASSIFIER_ADD
   HAL_SUCCESS
*/
int
hal_arp_entry_del (struct pal_in4_addr *ip_addr, 
                   u_char *mac_addr, u_int32_t ifindex)
{
  return HAL_SUCCESS;
}


/*
   Name: hal_arp_entry_add
   
   Description:
   This API add a static arp entry
   
   Parameters:
   IN -> ip_addr - IP address
   IN -> mac_addr - Ethernet address
   IN -> ifindex - Interface index

   Returns:
   HAL_ERR_VLAN_CLASSIFIER_ADD
   HAL_SUCCESS
*/
int
hal_arp_entry_add (struct pal_in4_addr *ip_addr, 
                   u_char *mac_addr, unsigned int ifindex,
                   u_int8_t is_proxy_arp)
{
  return HAL_SUCCESS;
}


/* 
   Name: hal_arp_del_all

   Description:
   This API deletes all dynamic and/or static arp entries

   Parameters:
   clr_flag : Flag to indicate dynamic and/or static arp entries

   Returns:
   < 0 on error

   HAL_SUCCESS
*/
int hal_arp_del_all (unsigned short fib_id, u_char clr_flag)
{
  return HAL_SUCCESS;
}
  
/* 
   Name: hal_arp_cache_get

   Description:
   This API gets the arp cache entries starting at the next address of ip address 
   specified. It gets the count number of entries. It returns the actual number of 
   entries found as the return parameter. It is expected that the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> ip_addr - IP address
   IN -> count  - count
   OUT -> cache - the array of arp entries returned

   Returns:
   number of entries. Can be 0 for no entries.
*/
int
hal_arp_cache_get (unsigned short fib_id, struct pal_in4_addr *ip_addr, 
                   int count,
                   struct hal_arp_cache_entry *cache)
{
  return 0;
}
