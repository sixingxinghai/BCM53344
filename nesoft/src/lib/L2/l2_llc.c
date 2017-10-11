/* Copyright (C) 2003,  All Rights Reserved.
 
    LOGICAL LINK CONTROL
 
    This module implements the logical link control 
    procedures for ethernet bridging.
 
 
*/
#include "l2_llc.h"

/* Format an LLC header */
static struct LLC_Frame_s
defaultLLCFrame = { LLC_DEFAULT_ADDR, LLC_DEFAULT_ADDR, LLC_DEFAULT_CTRL};
/* Format a LLC header for RPVST+ support */
static struct LLC_Frame_s
llc_frame_rpvst = { LLC_DSAP_ADDR, LLC_SSAP_ADDR, LLC_DEFAULT_CTRL};

/* Format a SNAP header */
static struct snap_frame
snap_frame = {SNAP_ORGANIZATION_CODE, SNAP_PROTO_ID};

unsigned char *
l2_llcFormat (unsigned char * frame)
{
  *frame++ = defaultLLCFrame.dsap;
  *frame++ = defaultLLCFrame.ssap;
  *frame++ = defaultLLCFrame.ctrl;
  return frame;
}

/* Parse and check an LLC header */

int
l2_llcParse (unsigned char * frame, struct LLC_Frame_s * llc)
{
  llc->dsap = *frame++;
  llc->ssap = *frame++;
  llc->ctrl = *frame++;

  return (((llc->dsap == LLC_DEFAULT_ADDR) 
    && (llc->ssap == LLC_DEFAULT_ADDR) 
    && (llc->ctrl == LLC_DEFAULT_CTRL)) ||
       ((llc->dsap == LLC_DSAP_ADDR)
    && (llc->ssap == LLC_SSAP_ADDR)
    && (llc->ctrl == LLC_DEFAULT_CTRL)));
}

unsigned char *
l2_llcformat_rpvst (unsigned char *frame)
{
  *frame++ = llc_frame_rpvst.dsap;
  *frame++ = llc_frame_rpvst.ssap;
  *frame++ = llc_frame_rpvst.ctrl;

  return frame;
}

unsigned char *
l2_snap_format (unsigned char * frame)
{

  *frame ++= ((snap_frame.org_code >> 16) & 0x0fff);
  *frame ++= ((snap_frame.org_code  >> 8) & 0x0fff);
  *frame ++= ((snap_frame.org_code & 0x0fff));

  *frame ++= (snap_frame.proto_id >> 8) & 0xff;
  *frame ++= snap_frame.proto_id & 0x0f;

  return frame;
}

int
l2_snap_parse (unsigned char * frame, struct snap_frame * snap)
{
  snap->org_code &= 0x0;

  snap->org_code |= (*frame++ << 16);
  snap->org_code |= (*frame++ << 8);
  snap->org_code |= *frame++;

  snap->proto_id = (*frame++ << 8);
  snap->proto_id += *frame++;

  return ((snap->org_code == SNAP_ORGANIZATION_CODE)
          && (snap->proto_id == SNAP_PROTO_ID));
}
