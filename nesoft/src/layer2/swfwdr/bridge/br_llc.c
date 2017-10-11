/* Copyright (C) 2003,  All Rights Reserved.
 
    LOGICAL LINK CONTROL
 
    This module implements the logical link control 
    procedures for ethernet bridging.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 
*/
#include "br_llc.h"

void
br_llcFormat (unsigned char * frame, struct LLC_Frame_s * llc)
{
  *frame++ = llc->dsap;
  *frame++ = llc->ssap;
  *frame++ = llc->ctrl;
}

int
br_llcParse (unsigned char * frame, struct LLC_Frame_s * llc)
{
  llc->dsap = *frame++;
  llc->ssap = *frame++;
  llc->ctrl = *frame++;
  return (llc->dsap == LLC_DEFAULT_ADDR) 
    && (llc->ssap == LLC_DEFAULT_ADDR) 
    && (llc->ctrl == LLC_DEFAULT_CTRL);
}
