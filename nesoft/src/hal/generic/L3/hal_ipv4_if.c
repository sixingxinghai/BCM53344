/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_incl.h"


/* 
   Name: hal_if_ipv4_address_add 

   Description:
   This API adds a IPv4 address to a L3 interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> ipaddr - ipaddress of interface
   IN -> ipmask - mask length

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_ipv4_address_add (char *ifname, unsigned int ifindex,
                         struct pal_in4_addr *ipaddr, unsigned char ipmask)
{
  return 0;
}

/* 
   Name: hal_if_ipv4_address_delete

   Description:
   This API deletes the IPv4 address from a L3 interface.

   Parameters:
   IN -> ifname - interface name
   IN -> ifindex - interface ifindex
   IN -> ipaddr - ipaddress of interface
   IN -> ipmask - mask length

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_if_ipv4_address_delete (char *ifname, unsigned int ifindex,
                            struct pal_in4_addr *ipaddr, unsigned char ipmask)
{
  return 0;
}

