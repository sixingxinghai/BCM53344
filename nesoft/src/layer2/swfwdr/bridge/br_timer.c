/*
    Spanning tree protocol; timer-related code
    Linux ethernet bridge
  
    Authors:
    Lennert Buytenhek   <buytenh@gnu.org>
  
    br_timer.c,v 1.4 2009/04/24 18:05:32 bob.macgill Exp
  
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
  
    This file contains the operating system related timer code 
 */
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>
#include <asm/uaccess.h>

#include "if_ipifwd.h"
#include "br_types.h"
#include "br_fdb.h"
#include "br_timer.h"
#include "br_timer_handler.h"
#include "bdebug.h"


/* This callback function handles the OS tick and invoke the appropriate timer handler. */

void 
br_tick (unsigned long __data)
{
  struct apn_bridge *br = (struct apn_bridge *)__data;
  BR_READ_LOCK (&br->lock);
  br_check_timers (br);
  BR_READ_UNLOCK (&br->lock);
  br->tick.expires = jiffies + HZ/2;
  add_timer (&br->tick);
}
