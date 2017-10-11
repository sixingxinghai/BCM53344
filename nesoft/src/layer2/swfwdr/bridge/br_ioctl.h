#ifndef __PACOS_BR_IOCTL_H__
#define __PACOS_BR_IOCTL_H__
/*
    Linux ethernet bridge
  
    Authors:
  
    br_ioctl.h,v 1.3 2007/03/07 05:13:11 sambath Exp
  
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */
/* br_ioctl.c */
int 
br_ioctl_bridge (struct sock * sk, unsigned long arg);

#define BR_IS_PROVIDER_BRIDGE(TYPE)             \
        (((TYPE) == BR_TYPE_PROVIDER_RSTP) ||   \
         ((TYPE) == BR_TYPE_PROVIDER_MSTP))
#endif
