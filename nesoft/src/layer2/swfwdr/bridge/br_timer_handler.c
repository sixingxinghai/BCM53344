/*
  Spanning tree protocol; timer-related code
  Linux ethernet bridge
 
  Authors:
  Lennert Buytenhek   <buytenh@gnu.org>

  br_timer_handler.c,v 1.3 2009/04/24 18:05:32 bob.macgill Exp
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
  
 */
#include <linux/autoconf.h>
#include "br_types.h"
#include "br_fdb.h"
#include "br_timer.h"
#include "bdebug.h"


/* This function handles the expiry of the dyanmic ageing timer.
   This cleans out the dynamic fdb of any aged entries.
   It called under bridge lock. */

static void 
br_dynamic_ageing_timer_expired (struct apn_bridge * br)
{
  BDEBUG("dynamic aging timer expired\n");
  br_timer_set (&br->dynamic_aging_timer, jiffies);
  br_fdb_cleanup (br);
}
/* This function checks for the expiry of any bridge or port timers
   on the specified bridge instance. It is called under bridge lock. */

void 
br_check_timers (struct apn_bridge * br)
{
  if (br_timer_has_expired (&br->dynamic_aging_timer, br->dynamic_ageing_interval))
    {
      br_dynamic_ageing_timer_expired (br);
    }
}

