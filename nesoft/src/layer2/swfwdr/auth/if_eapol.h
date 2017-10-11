/* Copyright (C) 2003,  All Rights Reserved. 

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.

*/
#ifndef __IF_EAPOL_H__
#define __IF_EAPOL_H__

/* This module declares the API for sockets of type ETH_P_PAE.
   These sockets are used to send/recv EAPOL PDUs for an authenticator
   per the 802.1x specification. */

/* recvmsg and sendmsg addresses */

struct eapol_addr
{
    unsigned char       destination_address[6];
    unsigned char       source_address[6];
    int                       port;                   /* Ifindex in linux */
/*MAC-based authentication Enhancement*/
    unsigned short      vlanid;
};


/* Ioctl calls accepted on an ETH_P_PAE socket */

#define APNEAPOL_BASE           200
#define APNEAPOL_GET_VERSION    (APNEAPOL_BASE + 1)
#define APNEAPOL_ADD_PORT       (APNEAPOL_BASE + 2)
#define APNEAPOL_DEL_PORT       (APNEAPOL_BASE + 3)
#define APNEAPOL_SET_PORT_STATE (APNEAPOL_BASE + 4)
#define APNEAPOL_SET_PORT_MACAUTH_STATE (APNEAPOL_BASE + 5)

/* 802.1X port state.  */
#define APN_AUTH_PORT_STATE_BLOCK_INOUT    0
#define APN_AUTH_PORT_STATE_BLOCK_IN       1
#define APN_AUTH_PORT_STATE_UNBLOCK        2
#define APN_AUTH_PORT_STATE_UNCONTROLLED   3

/*MAC-based authentication Enhancement*/
#define APN_MACAUTH_PORT_STATE_ENABLED     0
#define APN_MACAUTH_PORT_STATE_DISABLED    1

#endif
