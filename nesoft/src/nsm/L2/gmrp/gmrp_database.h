/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GMRP_DATABASE_H
#define _PACOS_GMRP_DATABASE_H

#include "pal.h"

struct gmrp_gmd_entry
{
  u_char mac_addr[HAL_HW_LENGTH];
  u_char flags;
  u_char unused;
};

struct gmrp_attr_entry_tbd
{
  u_int32_t attribute_index;

  u_int32_t pending_gid;
};

struct gmrp_gmd
{
  struct bitmap *attribute_index_mgr;
  struct ptree *gmd_entry_table;
  struct ptree *to_be_deleted_attr_table;
};


extern bool_t
gmd_create_gmd (struct gmrp_gmd **gmd);


extern void
gmd_destroy_gmd (struct gmrp_gmd *gmd);

extern struct gmrp_attr_entry_tbd *
gmd_entry_tbd_create (struct gmrp_gmd *gmd, u_int32_t attribute_index);

extern void
gmrp_gmd_delete_tbd_entry (struct gmrp_gmd *gmd, u_int32_t tbd_index);

struct gmrp_attr_entry_tbd *
gmd_entry_tbd_lookup (struct gmrp_gmd *gmd, u_int32_t attribute_index);

extern bool_t
gmd_find_entry (struct gmrp_gmd *gmd, u_char *mac_addr,
                u_int32_t *found_index);
extern bool_t 
gmd_create_entry (struct gmrp_gmd *gmd, u_char *mac_addr, 
                  u_int32_t *created_index);

extern bool_t 
gmd_delete_entry (struct gmrp_gmd *gmd, u_int32_t deleted_index);


extern bool_t 
gmd_get_mac_addr (struct gmrp_gmd *gmd, u_int32_t index, 
                  u_char *mac_addr);
#endif /* !_PACOS_GMRP_DATABASE_H */
