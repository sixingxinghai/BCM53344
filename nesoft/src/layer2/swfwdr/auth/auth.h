/* Copyright 2003  All Rights Reserved. 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.

*/
#ifndef __APN_AUTH_TYPES_H__
#define __APN_AUTH_TYPES_H__

#include <asm/uaccess.h> /* for copy_from_user */
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/datalink.h>
#include <linux/mm.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/rtnetlink.h>
#include <linux/notifier.h>

#include "if_eapol.h"
#define APN_EAPOL_VERSION       1

#define DIR_BOTH  0
#define DIR_IN    1

/*MAC-based authentication Enhancement*/
#define MACAUTH_ENABLED   0
#define MACAUTH_DISABLED  1

extern rwlock_t  auth_port_lock;
struct auth_port
{
  struct auth_port *    next;
  int           ifindex;    /* Ifindex of controlled device */
  int           (*xmit_fcn)(struct sk_buff * skb, struct net_device * dev);
  int           (*rx_fcn)(struct sk_buff * skb);
  unsigned char dir;        /* The control direction when blocked */
  unsigned char auth_state; /* The current authorization state (1=blocked) */
};

extern struct auth_port * auth_find_port (int real_dev_ifindex);

extern int auth_add_port (const char * const name);
extern int auth_remove_port (const char * const name);
extern void auth_remove_all_ports (void);
extern int auth_set_port_state (const int ifindex, const int state);

/*AUTH Mac Feature*/
extern int auth_mac_set_port_state (const int ifindex, const int state);

#endif /* __APN_AUTH_TYPES_H__ */
