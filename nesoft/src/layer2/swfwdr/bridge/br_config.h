#ifndef __PACOS_BR_CONFIG_H_
#define __PACOS_BR_CONFIG_H_
/*
    Linux ethernet bridge
  
    Authors:
  
    br_config.h,v 1.6 2009/05/21 18:27:39 sailajab Exp
  
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */


#include <linux/netdevice.h>
#include <linux/miscdevice.h>
#include <config.h>
#include "if_ipifwd.h"
#include "bdebug.h"


#define BRIDGE_ID_LEN        8

#define BR_HASH_BITS                    8
#define BR_HASH_SIZE                    (1 << BR_HASH_BITS)

#define BR_HOLD_TIME                    (1*HZ)

#define BITS_PER_PORTMAP_ENTRY          32
#define PORTMAP_SIZE                     ((BR_MAX_PORTS+31)/BITS_PER_PORTMAP_ENTRY)

#ifdef HAVE_RPVST_PLUS
#define BR_MAX_INSTANCES 4096
#else
#define BR_MAX_INSTANCES 64
#endif /* HAVE_RPVST_PLUS */
#define BR_INSTANCE_COMMON 0

#define BR_SET_PMBIT(port_index,port_map)  \
                (((port_map)[((port_index)/BITS_PER_PORTMAP_ENTRY)]) \
                 |= (1 <<((port_index) % BITS_PER_PORTMAP_ENTRY)) )

#define BR_GET_PMBIT(port_index,port_map) \
                (((port_map)[((port_index)/BITS_PER_PORTMAP_ENTRY)]) \
                 & (1 <<((port_index) % BITS_PER_PORTMAP_ENTRY)) )

#define BR_CLEAR_PORT_MAP(port_map)                     \
{                                                       \
        int portmap_index;                              \
        for (portmap_index = 0; portmap_index < PORTMAP_SIZE; portmap_index++)\
                 port_map[portmap_index] = 0;           \
}
                 
#define BR_PRINT_PORT_MAP(port_map)     \
{                                       \
        int portmap_index = 0;          \
        for (portmap_index = 0; portmap_index < PORTMAP_SIZE; portmap_index++)\
                BDEBUG("portmap %d for index %d \n",            \
                    port_map[portmap_index],portmap_index) ;    \
}
                 

#endif
