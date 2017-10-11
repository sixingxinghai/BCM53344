/* Copyright 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "bitmap.h"

#include "nsmd.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"

#include "hal_types.h"

#include "garp_gid.h"
#include "garp_pdu.h"
#include "garp_gid.h"
#include "garp.h"
#include "gmrp.h"
#include "gmrp_database.h"

bool_t
gmd_create_gmd (struct gmrp_gmd **gmd)
{
  *gmd = XCALLOC (MTYPE_GMRP_GMD, sizeof (struct gmrp_gmd));

  if ((*gmd) == NULL)
    {
      return PAL_FALSE;
    }

  (*gmd)->attribute_index_mgr = bitmap_new (32, 2, ULONG_MAX);
  (*gmd)->gmd_entry_table = ptree_init (ATTR_INDX_MAX_BITLEN);
  (*gmd)->to_be_deleted_attr_table = ptree_init (ATTR_INDX_MAX_BITLEN);

  return PAL_TRUE;
}

void
gmd_destroy_gmd (struct gmrp_gmd *old_gmd)
{
  struct ptree_node *rn;
  struct gmrp_gmd_entry *gmrp_gmd_entry;
  struct gmrp_attr_entry_tbd *attr_entry_tbd;

  /* Free the bitmap */
  bitmap_free (old_gmd->attribute_index_mgr);

  for (rn = ptree_top (old_gmd->to_be_deleted_attr_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      attr_entry_tbd = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      XFREE (MTYPE_GMRP_GMD_ENTRY, attr_entry_tbd);
   }

  for (rn = ptree_top (old_gmd->gmd_entry_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      gmrp_gmd_entry = rn->info;

      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      XFREE (MTYPE_GMRP_GMD_ENTRY, gmrp_gmd_entry);
   }

  ptree_finish (old_gmd->gmd_entry_table);
  ptree_finish (old_gmd->to_be_deleted_attr_table);
  XFREE (MTYPE_GMRP_GMD, old_gmd);
}

static inline bool_t
gmd_compare_mac_addr (u_char *mac_addr1, u_char *mac_addr2)
{
  /* Compare the last three bytes of the MAC address as those will only be
     different incase of multicast 01005e700000 to 01005e7fffff */
  return (mac_addr1[0] == mac_addr2[0]) && (mac_addr1[1] == mac_addr2[1]) &&
         (mac_addr1[2] == mac_addr2[2]) && (mac_addr1[3] == mac_addr2[3]) &&
         (mac_addr1[4] == mac_addr2[4]) && (mac_addr1[5] == mac_addr2[5]);
}

bool_t 
gmd_find_entry (struct gmrp_gmd *gmd, u_char *mac_addr, 
                u_int32_t *found_index)
{
  struct gmrp_gmd_entry *curr_entry;
  struct ptree_node *rn;

  if (!gmd || !gmd->gmd_entry_table)
    return PAL_FALSE;

  for (rn = ptree_top (gmd->gmd_entry_table); rn;
       rn = ptree_next (rn))
    {
      if (!rn->info)
        continue;

      curr_entry = rn->info;

      if (gmd_compare_mac_addr (mac_addr, curr_entry->mac_addr))
        {
          *found_index = gid_get_attr_index (rn);
          return PAL_TRUE;
        }
    }

  return PAL_FALSE;
}

bool_t 
gmd_create_entry (struct gmrp_gmd *gmd, u_char *mac_addr, 
                  u_int32_t *created_index)
{
  struct gmrp_gmd_entry *gmd_entry;
  struct ptree_node *rn;
  s_int32_t ret;

  ret = bitmap_request_index (gmd->attribute_index_mgr, created_index);

  if (ret < 0)
    {
      *created_index = 0;
      return PAL_FALSE;
    }

  if (! gmd->gmd_entry_table)
    {
      gmd->gmd_entry_table = ptree_init (ATTR_INDX_MAX_BITLEN);

      if (!gmd->gmd_entry_table)
        return PAL_FALSE;
    }


  rn = ptree_node_lookup (gmd->gmd_entry_table, (u_char *) created_index,
                          ATTR_INDX_MAX_BITLEN);

  if (!rn)
    {
      gmd_entry = XCALLOC (MTYPE_GMRP_GMD_ENTRY,
                           sizeof (struct gmrp_gmd_entry));

      if (!gmd_entry)
        return PAL_FALSE;

      pal_mem_cpy (gmd_entry->mac_addr, mac_addr, HAL_HW_LENGTH);
      gmd_entry->flags = 1;
      gmd_entry->unused = 0;

      rn = ptree_node_get (gmd->gmd_entry_table, (u_char *) created_index,
                           ATTR_INDX_MAX_BITLEN);

      if (!rn)
        {
          XFREE (MTYPE_GMRP_GMD_ENTRY, gmd_entry);
          return PAL_FALSE;
        }

      if (rn->info == NULL)
        PREFIX_ATTRIBUTE_INFO_SET (rn, gmd_entry);
      else
        XFREE (MTYPE_GMRP_GMD_ENTRY, gmd_entry);      

      ptree_unlock_node (rn);
    }
  else
    {
      ptree_unlock_node (rn);
      return PAL_TRUE;
    }   

  return PAL_TRUE;
}

struct gmrp_attr_entry_tbd *
gmd_entry_tbd_create (struct gmrp_gmd *gmd, u_int32_t attribute_index)
{
  struct ptree_node *rn;
  struct gmrp_attr_entry_tbd *attr_entry_tbd = NULL;

  rn = ptree_node_lookup (gmd->to_be_deleted_attr_table, 
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
                                sizeof (struct gmrp_attr_entry_tbd));

      if (!attr_entry_tbd)
        return NULL;

      rn = ptree_node_get (gmd->to_be_deleted_attr_table, 
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
gmrp_gmd_delete_tbd_entry (struct gmrp_gmd *gmd, u_int32_t tbd_index)
{
  struct ptree_node *rn;
  struct gmrp_attr_entry_tbd *attr_entry_tbd;


  rn = ptree_node_lookup (gmd->to_be_deleted_attr_table, (u_char *) &tbd_index,
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

struct gmrp_attr_entry_tbd *
gmd_entry_tbd_lookup (struct gmrp_gmd *gmd, u_int32_t attribute_index)
{
  struct ptree_node *rn;
  struct gmrp_attr_entry_tbd *attr_entry_tbd = NULL;

  rn = ptree_node_lookup (gmd->to_be_deleted_attr_table, 
                          (u_char *) &attribute_index, ATTR_INDX_MAX_BITLEN);

  if (rn)
    {
      ptree_unlock_node (rn);

      if (rn->info)
        attr_entry_tbd = rn->info;
    }
  return attr_entry_tbd;
}

bool_t 
gmd_delete_entry (struct gmrp_gmd *gmd, u_int32_t deleted_index)
{
  struct gmrp_gmd_entry *gmd_entry;
  struct ptree_node *rn;

  rn = ptree_node_lookup (gmd->gmd_entry_table, (u_char *) &deleted_index,
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

      gmd_entry = rn->info;
      gmd_entry->flags = 0;

      pal_mem_set (gmd_entry->mac_addr, 0, HAL_HW_LENGTH);
      PREFIX_ATTRIBUTE_INFO_UNSET (rn);

      bitmap_release_index (gmd->attribute_index_mgr, deleted_index);
      XFREE (MTYPE_GMRP_GMD_ENTRY, gmd_entry);
   }

  return PAL_TRUE;
}


bool_t 
gmd_get_mac_addr (struct gmrp_gmd *gmd, u_int32_t index, 
                  u_char *mac_addr)
{
  struct gmrp_gmd_entry *gmd_entry;
  struct ptree_node *rn;

  rn = ptree_node_lookup (gmd->gmd_entry_table, (u_char *) &index, 
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

      gmd_entry = rn->info;
  
      pal_mem_cpy (mac_addr, gmd_entry->mac_addr, 
                   HAL_HW_LENGTH* sizeof (u_char)); 
      return PAL_TRUE;
    }

  return PAL_FALSE;
}

