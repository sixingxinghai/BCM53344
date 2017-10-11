#ifndef __PACOS_BR_API_H__
#define __PACOS_BR_API_H__
/*
    Linux ethernet bridge
  
    Authors:
  
    br_api.h,v 1.8 2009/03/23 13:48:39 nagaleelap Exp
  
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/if_arp.h>
#include <linux/inetdevice.h>
#include <linux/rtnetlink.h>
#include <net/sock.h>
#include <asm/uaccess.h>
#include "if_ipifwd.h"
#include "br_types.h"

/* br_if.c */
extern int 
br_add_bridge (char *name, struct sock * sk, unsigned is_vlan_aware,
               int protocol, unsigned char edge, unsigned char beb);

extern int 
br_del_bridge (char *name, struct sock * sk);

extern struct apn_bridge *
br_find_bridge (char * name);

extern struct apn_bridge *
br_get_default_bridge (void);

extern int 
br_add_if (struct apn_bridge *br, struct net_device *dev);

extern int 
br_del_if (struct apn_bridge *br, struct net_device *dev);

extern int 
br_get_bridge_names (char * name_list, int num);

extern int 
br_get_port_ifindices (struct apn_bridge *br, int *ifindices, int num);

extern struct apn_bridge_port *
br_get_port (struct apn_bridge * br, int port_no);

extern int
br_port_protocol_process (char *bridge_name, unsigned int port_no,
                          unsigned int protocol, unsigned int process,
                          vid_t vid);

#endif /* !__PACOS_BR_API_H__ */
