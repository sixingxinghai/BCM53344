/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_IPV4_UC_H_
#define _HAL_IPV4_UC_H_

/* 
   Name: hal_ipv4_uc_init

   Description:
   This API initializes the IPv4 unicast table for the specified FIB ID

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_uc_init (unsigned short fib);

/* 
   Name: hal_ipv4_uc_deinit 

   Description:
   This API deinitializes the IPv4 unicast table for the specified FIB ID

   Parameters:
   IN -> fib - FIB ID

   Returns:
   HAL_IP_FIB_NOT_EXIST
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ipv4_uc_deinit (unsigned short fib);

/* 
   Name: hal_ipv4_uc_route_add

   Description:
   This API adds a IPv4 unicast route to the forwarding plane. 

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
hal_ipv4_uc_route_add (unsigned short fib, 
                       struct pal_in4_addr *ipaddr, 
                       unsigned char ipmask,
                       unsigned short num, struct hal_ipv4uc_nexthop *nexthops);

/* 
   Name: hal_ipv4_uc_route_delete

   Description:
   This API deletes a IPv4 route from the forwarding plane.

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
hal_ipv4_uc_route_delete (unsigned short fib, 
                         struct pal_in4_addr *ipaddr, 
                         unsigned char ipmask,
                         unsigned short num, 
                         struct hal_ipv4uc_nexthop *nexthops);

/* 
   Name: hal_ipv4_uc_route_update

   Description:
   This API updates an existing IPv4 route with the new nexthop
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
hal_ipv4_uc_route_update (unsigned short fib, 
                          struct pal_in4_addr *ipaddr, 
                          unsigned char ipmask,
                          unsigned short numfib, struct hal_ipv4uc_nexthop *nexthopsfib,
                          unsigned short numnew, struct hal_ipv4uc_nexthop *nexthopsnew);
                 
#endif /* _HAL_IPV4_UC_H_ */
