#ifndef __PACOS_L2_FORWARDER_H__
#define __PACOS_L2_FORWARDER_H__
                                                                         
/*
    Linux L2 forwarder Module
                                                                         
    Authors:
                                                                         
    l2_forwarder.h,v 1.3 2005/01/29 04:41:37 satish Exp
                                                                         
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */

/* Socket address structure for L2 */
struct sockaddr_l2
{
  /* Destination Mac address */
  unsigned char dest_mac[6];

  /* Source Mac address */
  unsigned char src_mac[6];

  /* Outgoing/Incoming interface index */
  unsigned int port;
};
                                                                         
/* l2_forwarder.c */
extern void
l2_mod_dec_use_count (void);
                                                                         
extern void
l2_mod_inc_use_count (void);
                                                                         
#endif /* __PACOS_L2_FORWARDER_H__ */
