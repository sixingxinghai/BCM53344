#ifndef __PACOS_BR_FORWARD_H__
#define __PACOS_BR_FORWARD_H__
/*
    Linux ethernet bridge
  
    Authors:
  
    br_forward.h,v 1.5 2007/01/10 03:47:03 sambath Exp
  
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    2 of the License, or (at your option) any later version.
 */
/* br_forward.c */
extern void br_forward (struct apn_bridge_port *port, 
                        vid_t vid,
                        struct sk_buff *skb,
                        bool_t is_egress_tagged);

void
br_flood (struct apn_bridge *br, struct sk_buff *skb, int clone);

extern void br_flood_forward (struct apn_bridge *br,
                              struct sk_buff *skb,
                              vid_t cvid,
                              vid_t svid,
                              int clone);

br_result_t
br_static_forward_frame (struct sk_buff *, struct apn_bridge *, vid_t, vid_t);

br_result_t
br_dynamic_forward_frame (struct sk_buff *, struct apn_bridge *, vid_t, vid_t);
#endif
