/* Copyright 2003  All Rights Reserved. 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.

*/
#ifndef __APN_EFM_TYPES_H__ 
#define __APN_EFM_TYPES_H__

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

#include "if_efm.h"

#define APN_EFM_VERSION 1

extern rwlock_t  efm_port_lock;
struct efm_port
{
  struct efm_port *    next;
  int    ifindex;    /* Ifindex of controlled device */
  int    (*xmit_fcn)(struct sk_buff * skb, struct net_device * dev);
  int    (*rx_fcn)(struct sk_buff * skb);
  enum efm_local_par_action local_par_action;
  enum efm_local_mux_action local_mux_action;
};

extern struct efm_port * efm_find_port (int real_dev_ifindex);

extern int efm_add_port (const int ifindex);
extern int efm_remove_port (const int ifindex);
extern void efm_remove_all_ports (void);
extern int efm_set_port_state (const int ifindex,
                               enum efm_local_par_action local_par_action,
                               enum efm_local_mux_action local_mux_action);
#endif /* __APN_EFM_TYPES_H__ */
