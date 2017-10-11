#ifndef __PACOS_BR_INPUT_H__
#define __PACOS_BR_INPUT_H__
/*
    Linux ethernet bridge
  
    Authors:
  
    br_input.h,v 1.3 2005/12/30 20:00:38 vividh Exp
  
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */

/* br_input.c */

extern int br_handle_frame (struct sk_buff *skb);

#endif
