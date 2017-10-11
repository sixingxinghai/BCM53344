/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_IPV6_UC_H_
#define _HAL_IPV6_UC_H_

/* 
   Name: hal_ipv6_uc_init

   Description:
   This API initializes the IPv6 unicast table for the specified FIB ID

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_init (unsigned short fib);

/* 
   Name: hal_ipv6_uc_deinit 

   Description:
   This API deinitializes the Ipv6 unicast table for the specified FIB ID

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_deinit (unsigned short fib);

/* 
   Name: hal_ipv6_uc_route_add

   Description:
   This API adds a IPv6 unicast route to the forwarding plane. 

   Parameters:
   IN -> fib - FIB identifier
   IN -> ipaddr - IP address
   IN -> ipmask - IP mask length
   IN -> num - number of nexthops
   IN -> nexthop list - list of nexthops

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_route_add (unsigned short fib, 
                       struct pal_in6_addr *ipaddr, 
                       unsigned char ipmask,
                       unsigned short num, struct hal_ipv6uc_nexthop *nexthops);

/* 
   Name: hal_ipv6_uc_route_delete

   Description:
   This API deletes a IPv6 route from the forwarding plane.

   Parameters:
   IN -> fib - FIB identifier
   IN -> ipaddr - IP address
   IN -> ipmask - IP mask length
   IN -> num - number of nexthops
   IN -> nexthop list - list of nexthops

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_route_delete (unsigned short fib, 
                         struct pal_in6_addr *ipaddr, 
                         unsigned char ipmask,
                         int num, struct hal_ipv6uc_nexthop *nexthops);

/* 
   Name: hal_ipv6_uc_route_update

   Description:
   This API updates an existing IPv6 route with the new nexthop
   parameters.

   Parameters:
   IN -> fib - FIB indentifier
   IN -> ipaddr - IP address
   IN -> ipmask - IP mask length
   IN -> numfib - number of nexthops in FIB
   IN -> nexthopsfib - nexthops in FIB
   IN -> numnew - number of new/updated nexthops
   IN -> nexthopsnew - new/updated nexthops in FIB

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv6_uc_route_update (unsigned short fib, 
                          struct pal_in6_addr *ipaddr, 
                          unsigned char ipmask,
                          int numfib, struct hal_ipv6uc_nexthop *nexthopsfib,
                          int numnew, struct hal_ipv6uc_nexthop *nexthopsnew);
                 

#endif /* _HAL_IPV6_UC_H_ */
