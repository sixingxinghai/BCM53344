/* Copyright (C) 2004  All Rights Reserved.
 *
 *  PVLAN functionalities
 *
 *  Author:
 *
 */

#ifndef __PACOS_BR_PVLAN_API_H__
#define __PACOS_BR_PVLAN_API_H__

#include "if_ipifwd.h"
#include "br_types.h"

extern int
br_pvlan_set_type (char *bridge_name, vid_t vid, enum pvlan_type type);

extern int
br_pvlan_set_associate (char *bridge_name, vid_t vid, vid_t pvid,
                        bool_t associate);

extern int
br_pvlan_set_port_mode (char *bridge_name, int port_no,
                        enum pvlan_port_configure_mode mode);

extern int
br_pvlan_set_port_host_associate (char *bridge_name, int port_no,
                                  vid_t vid, vid_t pvid, bool_t associate);

extern char *
br_pvlan_get_configure_ports (struct apn_bridge *br, vid_t vid,
                              vid_t *default_vid);
#endif /* __PACOS_BR_PVLAN_API_H__ */
