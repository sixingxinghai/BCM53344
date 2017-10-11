#ifndef __PACOS_BR_FDB_H__
#define __PACOS_BR_FDB_H__

/*
  Linux ethernet bridge
  
  Authors:
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
*/
 

#include "br_types.h"

extern void 
br_fdb_changeaddr (struct apn_bridge_port * p, unsigned char * newaddr);

extern void 
br_fdb_cleanup (struct apn_bridge * br);

extern void 
br_fdb_delete_all_by_port (struct apn_bridge * br,
                           struct apn_bridge_port * p);

extern void 
br_fdb_delete_dynamic_by_port (struct apn_bridge * br, 
                               struct apn_bridge_port * p,
                               unsigned int instance, vid_t vid,
                               vid_t svid);

extern void
br_fdb_delete_dynamic_by_vlan (struct apn_bridge * br,
                               enum vlan_type type, vid_t vid);


extern struct apn_bridge_fdb_entry *
br_fdb_get_pvlan_entry (struct apn_bridge *br, unsigned char *addr, vid_t vid );

extern struct apn_bridge_fdb_entry *
br_fdb_get (struct apn_bridge * br, unsigned char * addr,
            vid_t vid, vid_t svid);

extern void 
br_fdb_put (struct apn_bridge_fdb_entry *ent);

extern int  
br_fdb_get_entries (struct apn_bridge * br, 
                    unsigned char * _buf, 
                    int maxnum, 
                    int offset);

extern br_result_t
br_fdb_get_static_fdb_entries (struct apn_bridge *br,
                               unsigned char *addr,
                               vid_t vid,
                               vid_t svid,
                               struct static_entries *static_entries);

extern br_result_t
br_fdb_get_static_pvlan_fdb_entries (struct apn_bridge * br,
                                     unsigned char *addr,
                                     vid_t vid,
                                     struct static_entries * static_entries );
extern int 
br_fdb_insert (struct apn_bridge * br,
               struct apn_bridge_port * source,
               unsigned char *addr,
               vid_t vid, vid_t svid,
               int is_local, bool_t is_forward);

int
br_fdb_delete (struct apn_bridge *br,
               struct apn_bridge_port *port,
               unsigned char *addr, 
               vid_t vid,
               vid_t svid);

/* br_static_fdb */
extern int 
br_fdb_insert_static (struct apn_bridge *br,
                      struct apn_bridge_port *source,
                      unsigned char *addr,
                      int is_fwd,
                      vid_t vid,
                      vid_t svid);

extern int 
br_fdb_rem_static (struct apn_bridge *br,
                   struct apn_bridge_port *source,
                   unsigned char *addr,
                   vid_t vid,
                   vid_t svid);

extern struct apn_bridge_fdb_entry *
br_fdb_get_static (struct apn_bridge *br,
                   unsigned char *addr, 
                   struct apn_bridge_port * port,
                   vid_t vid,
                   vid_t svid);

extern int 
br_fdb_get_static_entries (struct apn_bridge *br,
                           unsigned char *_buf,
                           int maxnum,
                           int offset);

extern void
br_clear_dynamic_fdb_by_mac ( struct apn_bridge *br,  unsigned char *addr);

int
br_fdb_get_index_by_mac_vid (struct apn_bridge *br, unsigned char *addr,
                             vid_t vid, vid_t svid, int *index);


struct hal_fdb_entry
{
  unsigned short vid;
  unsigned short svid;
  unsigned int ageing_timer_value;
  unsigned char mac_addr[6];
  int num;
  char is_local;
  unsigned char is_static;
  unsigned char is_forward;
  unsigned int port;
};

#define BR_UNICAST_FDB   0
#define BR_MULTICAST_FDB 1

#endif

