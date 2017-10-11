/* Copyright (C) 2005  All Rights Reserved. */
#ifndef _HAL_IPV6_NBR_H_
#define _HAL_IPV6_NBR_H_


/* 
   Name: hal_ipv6_nbr_add

   Description:
   This API adds an IPv6 neighbor entry

   Parameters:
   IN -> addr     - IPv6 address.
   IN -> mac_addr - Mac address.
   IN -> ifindex  - Ifindex. 

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_nbr_add (struct pal_in6_addr *addr, unsigned char *mac_addr,
                  u_int32_t ifindex);


/* 
   Name: hal_ipv6_nbr_del

   Description:
   This API deletes an IPv6 neighbor entry

   Parameters:
   IN -> addr     - IPv6 address.
   IN -> mac_addr - Mac address.

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int hal_ipv6_nbr_del (struct pal_in6_addr *addr, unsigned char *mac_addr,
    unsigned int ifindex);




/* 
   Name: hal_ipv6_nbr_del_all

   Description:
   This API deletes all dynamic and/or static ipv6 neighbor entries

   Parameters:
   clr_flag : Flag to indicate dynamic and/or static nbr entries
   fib_id : FIB id

   Returns:
   < 0 on error

   HAL_SUCCESS
*/
int hal_ipv6_nbr_del_all (unsigned short fib_id, u_char clr_flag);



/* 
   Name: hal_ipv6_nbr_cache_get 

   Description:
   This API gets the ipv6 neighbor table  starting at the next address of addr 
   address. It gets the count number of entries. It returns the 
   actual number of entries found as the return parameter. It is expected at the memory
   is allocated by the caller before calling this API.

   Parameters:
   IN -> addr     - IPv6 address.
   IN -> count    - Count
   IN -> cache    - IPv6 neighbor cache.

   Returns:
   number of entries. Can be 0 for no entries.
*/
int
hal_ipv6_nbr_cache_get(unsigned short fib_id,
                       struct pal_in6_addr *addr, int count,
                       struct hal_ipv6_nbr_cache_entry *cache);

#endif /* _HAL_IPV6_NBR_H_ */
