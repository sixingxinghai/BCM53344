/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"
#include "hal_ipv4_arp.h"

/* Common function for arp. */
static int
_hal_ipv4_arp (int command, struct pal_in4_addr *ipaddr,
               unsigned char *mac_addr, u_int32_t ifindex,
               u_int8_t is_proxy_arp)
{
  return 0;
}

/* 
   Name: hal_ipv4_arp_add

   Description:
   This API adds an IPv4 arp entry

   Parameters:
   IN -> ipaddr   - IP address.
   IN -> mac_addr - Mac address.
   IN -> ifindex  - Ifindex. 

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_arp_entry_add(struct pal_in4_addr *ipaddr, 
                  unsigned char *mac_addr,
                  u_int32_t ifindex,
                  u_int8_t is_proxy_arp)
{
  return 0;
}


/* 
   Name: hal_ipv4_arp_del

   Description:
   This API adds an IPv4 arp entry

   Parameters:
   IN -> ipaddr   - IP address.
   IN -> mac_addr - Mac address.

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_arp_entry_del(struct pal_in4_addr *ipaddr, 
                  unsigned char *mac_addr,
                  u_int32_t ifindex)
{
  return 0;
}



/* 
   Name: hal_arp_cache_get 

   Description:
   This API gets the arp table  starting at the next address of ipaddr 
   address. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> ipaddr   - IP address.
   IN -> count    - Count
   IN -> cache    - arp cache.

   Returns:
   number of entries. Can be 0 for no entries.
*/
int
hal_arp_cache_get(unsigned short fib_id, struct pal_in4_addr *ipaddr,
                  int count,
                  struct hal_arp_cache_entry *cache)
{
  return 0;
}

int
hal_arp_del_all (unsigned short fib_id, u_char clr_flag)
{
  return 0;
}

