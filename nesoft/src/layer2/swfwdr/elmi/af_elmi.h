/*
*   Linux ethernet bridge
*
*     Authors:
*       Lennert Buytenhek   <buytenh@gnu.org>
*
*         This program is free software; you can redistribute it and/or
*           modify it under the terms of the GNU General Public License
*             as published by the Free Software Foundation; either version
*               2 of the License, or (at your option) any later version.
*
*                 This file contains the header for the forwarder
*
*
*                 */

#ifndef _PACOS_AF_ELMI_H
#define _PACOS_AF_ELMI_H

int
elmi_rcv (struct sk_buff * skb, struct apn_bridge * bridge,
          struct net_device * port);

#endif /*_PACOS_AF_ELMI_H */
