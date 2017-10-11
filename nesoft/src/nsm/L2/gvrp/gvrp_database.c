/* Copyright 2003  All Rights Reserved. */
#include "pal.h"
#include "lib.h"
#include "memory.h"
#include "bitmap.h"
#include "table.h"

#include "nsmd.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"

#include "gvrp_debug.h"
#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp.h"
#include "gvrp.h"
#include "gvrp_database.h"
#include "avl_tree.h"

bool_t
gvd_create_gvd (struct gvrp_gvd **gvd)
{
  /* struct gvrp_gvd *new_gvd=NULL; */

  *gvd = XCALLOC (MTYPE_GVRP_GVD, sizeof (struct gvrp_gvd));
  
  if (!*gvd)
    {
      return PAL_FALSE; 
    }

  (*gvd)->attribute_index_mgr = bitmap_new (32, 0, GVRP_MAX_VLANS);
  (*gvd)->gvd_entry_table = ptree_init (ATTR_INDX_MAX_BITLEN);
  (*gvd)->to_be_deleted_attr_table = ptree_init (ATTR_INDX_MAX_BITLEN);

  return PAL_TRUE;
}


void
gvd_destroy_gvd (struct gvrp_gvd *gvd)
{
  struct ptree_node *rn;
  struct gvrp_gvd_entry *gvrp_gvd_entry;
  struct gvrp_attr_entry_tbd *attr_entry_tbd;

  if (!gvd)
    return;

  for (rn = ptree_top (gvd->gvd_entry_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      gvrp_gvd_entry = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      XFREE (MTYPE_GVRP_GVD_ENTRY, gvrp_gvd_entry);
    }

  for (rn = ptree_top (gvd->to_be_deleted_attr_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      attr_entry_tbd = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      XFREE (MTYPE_GVRP_GVD_ENTRY, attr_entry_tbd);
   }

  /* Free the bitmap */
  bitmap_free (gvd->attribute_index_mgr);

  ptree_finish (gvd->gvd_entry_table);
  ptree_finish (gvd->to_be_deleted_attr_table);

  XFREE (MTYPE_GVRP_GVD, gvd);
}


/* This function finds the port for the given vid from the dynamic vlan 
 *    registration table */
bool_t
gvd_find_entry (struct gvrp_gvd *gvd, u_int16_t vid, 
                u_int32_t* gid_index)
{
  struct gvrp_gvd_entry *curr_entry;
  struct ptree_node *rn;

  for (rn = ptree_top (gvd->gvd_entry_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      curr_entry = rn->info;

      if (vid == curr_entry->vid)
        {
          *gid_index = gid_get_attr_index (rn);
          return PAL_TRUE;
        }
    }

  return PAL_FALSE;
}

/* This function creates the port for the given vid from the dynamic vlan 
   registration table */
int
gvd_create_entry (struct gvrp_gvd *gvd, u_int16_t vid,
                  u_int32_t *created_index)
{
  struct gvrp_gvd_entry *gvd_entry;
  struct ptree_node *rn;
  s_int32_t ret;

  ret = bitmap_request_index (gvd->attribute_index_mgr, created_index);

  if (ret < 0)
    {
      *created_index = 0;
      return PAL_FALSE;
    }

  if (! gvd->gvd_entry_table)
    {
      gvd->gvd_entry_table = ptree_init (ATTR_INDX_MAX_BITLEN);

      if (!gvd->gvd_entry_table)
        return PAL_FALSE;
    }

  rn = ptree_node_lookup (gvd->gvd_entry_table, (u_char *) created_index,
                          ATTR_INDX_MAX_BITLEN);

    if (!rn)
    {
      gvd_entry = XCALLOC (MTYPE_GVRP_GVD_ENTRY,
                           sizeof (struct gvrp_gvd_entry));

      if (!gvd_entry)
        return PAL_FALSE;

      gvd_entry->attribute_index = *created_index;
      gvd_entry->vid = vid;

      rn = ptree_node_get (gvd->gvd_entry_table, (u_char *) created_index, 
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        {
          XFREE (MTYPE_GVRP_GVD_ENTRY, gvd_entry);
          return PAL_FALSE;
        }

      if (rn->info == NULL)
        PREFIX_ATTRIBUTE_INFO_SET (rn, gvd_entry);
      else
        XFREE (MTYPE_GVRP_GVD_ENTRY, gvd_entry);

      ptree_unlock_node (rn);
    }
  else
    {
      ptree_unlock_node (rn);
      return PAL_TRUE;
    }

  return PAL_TRUE;
}

struct gvrp_attr_entry_tbd *
gvd_entry_tbd_create (struct gvrp_gvd *gvd, u_int32_t attribute_index)
{
  struct ptree_node *rn;
  struct gvrp_attr_entry_tbd *attr_entry_tbd = NULL;

  rn = ptree_node_lookup (gvd->to_be_deleted_attr_table, 
                          (u_char *) &attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (rn)
    {
      ptree_unlock_node (rn);

      if (rn->info)
        attr_entry_tbd = rn->info;
    }
  else
    {
      attr_entry_tbd = XCALLOC (MTYPE_GMRP_GMD_ENTRY,
                                sizeof (struct gvrp_attr_entry_tbd));

      if (!attr_entry_tbd)
        return NULL;

      rn = ptree_node_get (gvd->to_be_deleted_attr_table, 
                           (u_char *) &attribute_index,
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        {
          XFREE (MTYPE_GMRP_GMD_ENTRY, attr_entry_tbd);
          return NULL;
        }

      attr_entry_tbd->attribute_index = attribute_index;
      attr_entry_tbd->pending_gid = 0;

      PREFIX_ATTRIBUTE_INFO_SET (rn, attr_entry_tbd);
      ptree_unlock_node (rn);
    }

  return attr_entry_tbd;
}

void
gvrp_gvd_delete_tbd_entry (struct gvrp_gvd *gvd, u_int32_t tbd_index)
{
  struct ptree_node *rn;
  struct gvrp_attr_entry_tbd *attr_entry_tbd;

  rn = ptree_node_lookup (gvd->to_be_deleted_attr_table, (u_char *) &tbd_index,
                          ATTR_INDX_MAX_BITLEN);
  if (!rn)
    {
      return;
    }
  else
    {
      ptree_unlock_node (rn);

      if (!rn->info)
        return;

      attr_entry_tbd = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);
      XFREE (MTYPE_GMRP_GMD_ENTRY, attr_entry_tbd);
    }
}

struct gvrp_attr_entry_tbd *
gvd_entry_tbd_lookup (struct gvrp_gvd *gvd, u_int32_t attribute_index)
{
  struct ptree_node *rn;
  struct gvrp_attr_entry_tbd *attr_entry_tbd = NULL;

  rn = ptree_node_lookup (gvd->to_be_deleted_attr_table, 
                          (u_char *) &attribute_index,
                          ATTR_INDX_MAX_BITLEN);

  if (rn)
    {
      ptree_unlock_node (rn);

      if (rn->info)
        attr_entry_tbd = rn->info;
    }
  return attr_entry_tbd;
}

bool_t
gvd_delete_entry (struct gvrp_gvd *gvd, u_int32_t deleted_index)
{
  struct gvrp_gvd_entry *gvd_entry;
  struct ptree_node *rn;

  rn = ptree_node_lookup (gvd->gvd_entry_table, (u_char *) &deleted_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      return PAL_FALSE;
    }
  else
    {
      ptree_unlock_node (rn);

      if (!rn->info)
        return PAL_FALSE;

      gvd_entry = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      bitmap_release_index (gvd->attribute_index_mgr, deleted_index);
      XFREE (MTYPE_GVRP_GVD_ENTRY, gvd_entry);
   }

  return PAL_TRUE;
}

u_int16_t
gvd_get_vid (struct gvrp_gvd *gvd, u_int32_t index)
{
  struct gvrp_gvd_entry *gvd_entry;
  struct ptree_node *rn;

  rn = ptree_node_lookup (gvd->gvd_entry_table, (u_char *) &index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      return PAL_FALSE;
    }
  else
    {
      ptree_unlock_node (rn);

      if (!rn->info)
        return PAL_FALSE;

      gvd_entry = rn->info;

      return gvd_entry->vid;
    }

  return 0;
}
