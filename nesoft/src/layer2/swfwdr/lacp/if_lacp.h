/* Copyright (C) 2003,  All Rights Reserved. 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.

*/
#ifndef __IF_LACP_H__
#define __IF_LACP_H__

/* This module declares the API for sockets of type ETH_P_PAE.
   These sockets are used to send/recv LACP PDUs for an authenticator
   per the 802.1x specification. */

#define ETH_P_LACP 0x8809


#define APN_LACP_VERSION  0x0610

/* Ioctl calls accepted on an ETH_P_LACP socket */

#define APNLACP_BASE          300
#define APNLACP_GET_VERSION   (APNLACP_BASE + 1)
#define APNLACP_ADD_AGG       (APNLACP_BASE + 2)
#define APNLACP_DEL_AGG       (APNLACP_BASE + 3)
#define APNLACP_AGG_LINK      (APNLACP_BASE + 4)
#define APNLACP_DEAGG_LINK    (APNLACP_BASE + 5)
#define APNLACP_SET_MACADDR   (APNLACP_BASE + 6)

#define LACP_NAME "APN LACP 6.1"


#endif
