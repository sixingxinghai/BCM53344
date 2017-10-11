/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GVRP_DATABASE_H
#define _PACOS_GVRP_DATABASE_H

#define GVRP_VLAN_PORT_UNCONFIGURED              0x00
#define GVRP_VLAN_PORT_CONFIGURED                0x80
#define GVRP_VLAN_REGISTRATION_NORMAL            0x01 
#define GVRP_VLAN_REGISTRATION_FIXED             0x02
#define GVRP_VLAN_REGISTRATION_FORBIDDEN         0x03
#define GVRP_VLAN_EGRESS_TAGGED                  0x04
#define GVRP_DUMMY_GID_INDEX                     0xffffffff

struct gvrp_attr_entry_tbd
{
  u_int32_t attribute_index;

  u_int32_t pending_gid;
};

struct gvrp_gvd_entry
{
  u_int16_t vid;

  u_int32_t attribute_index;
};

 /* Table that maps vid->port[BR_MAX_PORTS] 
  * (Dynamic VLAN Registration)
  * ---------------------------------
  * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
  * ---------------------------------
  * 0 & 1 ->   Registration Fixed | Registration Forbidden
               | Registration Normal
  * 2 ->   Egress tagged | Egress untagged
  * 7 ->   Port is configured for the given vid
  * 2-6 -> Unused
  */

struct gvrp_gvd
{
  struct bitmap *attribute_index_mgr;
  struct ptree *gvd_entry_table;
  struct ptree *to_be_deleted_attr_table;
};

extern void
gvd_destroy_gvd (struct gvrp_gvd *gvd);

extern bool_t 
gvd_create_gvd (struct gvrp_gvd **gvd);

extern bool_t
gvd_find_entry (struct gvrp_gvd *gvd, u_int16_t vid,
                u_int32_t* gid_index);

extern struct gvrp_attr_entry_tbd *
gvd_entry_tbd_create (struct gvrp_gvd *gvd, u_int32_t attribute_index);

extern void
gvrp_gvd_delete_tbd_entry (struct gvrp_gvd *gvd, u_int32_t tbd_index);

struct gvrp_attr_entry_tbd *
gvd_entry_tbd_lookup (struct gvrp_gvd *gvd, u_int32_t attribute_index);

extern int
gvd_create_entry (struct gvrp_gvd *gvd, u_int16_t vid,
                  u_int32_t* gid_index);

bool_t
gvd_delete_entry (struct gvrp_gvd *gvd, u_int32_t deleted_index);

extern s_int32_t
gvrp_get_attr_index (struct route_node *rn);

u_int16_t
gvd_get_vid (struct gvrp_gvd *gvd, u_int32_t index);

#endif /* !_PACOS_GVRP_DATABASE_H */
