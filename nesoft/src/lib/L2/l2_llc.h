/* Copyright (C) 2003  All Rights Reserved. */

#ifndef __PACOS_BRLLC_H__
#define __PACOS_BRLLC_H__
/*
 
    LOGICAL LINK CONTROL
 
 
    This module declares the Logical Link Control type 1
    indication and request primitive data structure.
    Only one data structure is used because they are 
    identical for both the indication and the request.
 
*/

#define LLC_DEFAULT_ADDR    (0x42)
#define LLC_DEFAULT_CTRL    (0x03)

#define LLC_DSAP_ADDR    (0xaa)
#define LLC_SSAP_ADDR    (0xaa)
#define LLC_DEFAULT_CTRL (0x03)

#define SNAP_ORGANIZATION_CODE (0x00000c)
#define SNAP_PROTO_ID   (0x010b)

/* The SNAL header is used only for RPVST+ mode 
   SNAP Header, 5 bytes (3 bytes - organization code, 2bytes - proto id)
*/
struct snap_frame
{
    unsigned int   org_code;   /* Replace with Organization code */
    unsigned short proto_id;   /* Protocol Id */
};

struct LLC_Frame_s
{
    unsigned char   dsap;   /* Always 0x42 */
    unsigned char   ssap;   /* Always 0x42 */
    unsigned char   ctrl;   /* Always 0x03  */

};      

int 
l2_llcParse (unsigned char *, struct LLC_Frame_s *);

unsigned char *
l2_llcFormat (unsigned char *);

int
l2_snap_parse (unsigned char *, struct snap_frame *);

unsigned char *
l2_snap_format (unsigned char *);

unsigned char *
l2_llcformat_rpvst (unsigned char *);


#endif
