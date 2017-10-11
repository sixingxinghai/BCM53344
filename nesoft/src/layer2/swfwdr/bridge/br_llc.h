/* Copyright (C) 2003  All Rights Reserved. */

#ifndef __PACOS_BRLLC_H__
#define __PACOS_BRLLC_H__
/*
 
    LOGICAL LINK CONTROL
 
 
    This module declares the Logical Link Control type 1
    indication and request primitive data structure.
    Only one data structure is used because they are 
    identical for both the indication and the request.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 
*/

#define LLC_DEFAULT_ADDR    (0x42)
#define LLC_DEFAULT_CTRL    (0x03)

struct LLC_Frame_s
{
    unsigned char   dsap;   /* Always 0x42 */
    unsigned char   ssap;   /* Always 0x42 */
    unsigned char   ctrl;   /* Always 0x03  */

};      

int 
br_llcParse (unsigned char *, struct LLC_Frame_s *);

void 
br_llcFormat (unsigned char *, struct LLC_Frame_s *);

#endif
