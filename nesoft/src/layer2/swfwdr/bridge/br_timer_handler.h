/*
    Linux ethernet bridge
  
    Authors:
    Lennert Buytenhek   <buytenh@gnu.org>
  
    br_timer_handler.h,v 1.2 2004/12/21 00:14:27 abhijeet Exp
  
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */

#ifndef __BR_TIMER_H__
#define __BR_TIMER_H__

#include "br_types.h"

extern void 
br_check_timers (struct apn_bridge * br);
        
#endif
