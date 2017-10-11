/* Copyright (C) 2003,  All Rights Reserved. */
#ifndef __LACPDU_H__
#define __LACPDU_H__

/* This module declares the interface to the LACP PDU parse/format routines. */

/* lacpdu.c */
extern int format_lacpdu (struct lacp_link *link, register unsigned char *bufptr);
extern int parse_lacpdu (const unsigned char * const buf, const int len, 
                    struct lacp_pdu * const lacp_info);

extern int format_marker (struct lacp_link *link, register unsigned char *bufptr, const int response);
extern int parse_marker (const unsigned char * const buf, const int len, 
                    struct marker_pdu * const marker_info);

extern void lacp_dump_packet (const unsigned char * const pdu);

extern const unsigned char lacp_grp_addr[LACP_GRP_ADDR_LEN];

#define LACP_ACTOR_INFO_LEN_OFFSET   3
#define LACP_ACTOR_INFO_LEN_VALID    20

#define LACP_PARTNER_INFO_LEN_OFFSET   23
#define LACP_PARTNER_INFO_LEN_VALID    20

#define LACP_COLL_INFO_LEN_OFFSET   43
#define LACP_COLL_INFO_LEN_VALID    16

#define LACP_TERM_INFO_LEN_OFFSET   59
#define LACP_TERM_INFO_LEN_VALID    0

#endif
