
/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"


#include "table.h"
#include "ptree.h"
#include "avl_tree.h"
#include "bitmap.h"
#include "nsmd.h"
#include "nsm_router.h"
#include "if.h"
#include "prefix.h"
#ifdef HAVE_SNMP
#include <asn1.h>
#include "snmp.h"
#endif /* HAVE_SNMP */
#include "log.h"
#include "mpls_common.h"
#include "mpls_client.h"
#include "mpls.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "nsm_mpls.h"
#include "nsm_debug.h"
#include "rib/nsm_table.h"
#include "nsm_server.h"
#include "nsm_lp_serv.h"
#include "nsm_mpls_dep.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_fwd.h"
#ifdef HAVE_SNMP
#include "nsm_mpls_snmp.h"
#endif /* HAVE_SNMP */
#include "nsm_mpls_api.h"

#ifdef HAVE_SNMP
static char null_oid[] = { 0, 0 };
static struct prefix src_addr;
#endif  /* HAVE_SNMP */

/* Check if interface is valid for MPLS */
s_int32_t
nsm_mpls_check_valid_interface (struct interface *ifp, u_char opcode)
{
  if ((! ifp) || (! if_is_up (ifp)) || (if_is_loopback (ifp)) ||
      ((opcode == SWAP) &&
       (ifp->ls_data.status != LABEL_SPACE_VALID)))
    return NSM_FALSE;

  return NSM_TRUE;
}

/* Enable all interface for MPLS. */
void
nsm_mpls_api_enable_all_interfaces (struct nsm_master *nm,
                                    u_int16_t label_space)
{
  nsm_mpls_enable_all_interfaces (nm, label_space);
}

/* Disable all interface for MPLS. */
void
nsm_mpls_api_disable_all_interfaces (struct nsm_master *nm)
{
  nsm_mpls_disable_all_interfaces (nm);
}

void
nsm_mpls_ls_max_label_val_set2 (struct nsm_label_space *nls, u_int32_t val)
{
  struct nsm_master *nm = nls->nm;
  struct nsm_mpls_if *mif;
  struct listnode *ln;
  cindex_t cindex = 0;

  /* Set val. */
  nls->max_label_val = val;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while setting maximum label value");
              continue;
            }

          mif->ifp->ls_data.max_label_value = val;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);
        }
    }
}

/* Set max label value for all interfaces. */
s_int32_t
nsm_mpls_api_max_label_val_set (struct nsm_master *nm, u_int32_t val)
{
  struct route_node *rn;
  struct nsm_label_space *nls;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NSM_SUCCESS;

  /* If no change, return. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MAX_LABEL_VAL)
      && NSM_MPLS->max_label_val == val)
    return NSM_SUCCESS;

  /* If any label space is in use ... */
  if (nsm_lp_server_in_use (nm, 0xffffffff))
    return NSM_ERR_LS_IN_USE;

  /* Set top max label value. */
  NSM_MPLS->max_label_val = val;
  SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MAX_LABEL_VAL);

  for (rn = route_top (NSM_MPLS->ls_table); rn; rn = route_next (rn))
    {
      nls = rn->info;
      if (! nls)
        continue;

      if (! CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_VAL))
        nsm_mpls_ls_max_label_val_set2 (nls, val);
    }

  return NSM_SUCCESS;
}

void
nsm_mpls_ls_max_label_val_unset2 (struct nsm_label_space *nls)
{
  struct nsm_master *nm = nls->nm;
  struct nsm_mpls_if *mif;
  struct listnode *ln;
  cindex_t cindex = 0;

  /* Unset. */
  nls->max_label_val = LABEL_VALUE_MAX;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while unsetting maximum label value");
              continue;
            }

          mif->ifp->ls_data.max_label_value = LABEL_VALUE_MAX;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);
        }
    }

  /* If label space value is unused, remove it. */
  if (NSM_LABEL_SPACE_UNUSED (nls))
    nsm_mpls_label_space_del (nls);
}

/* Unset max label value for all interfaces. */
s_int32_t
nsm_mpls_api_max_label_val_unset (struct nsm_master *nm)
{
  struct route_node *rn;
  struct nsm_label_space *nls;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NSM_SUCCESS;

  /* If no change, return. */
  if (! CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MAX_LABEL_VAL))
    return NSM_SUCCESS;

  /* If any label space is in use ... */
  if (nsm_lp_server_in_use (nm, 0xffffffff))
    return NSM_ERR_LS_IN_USE;

  /* Set top max label value. */
  NSM_MPLS->max_label_val = LABEL_VALUE_MAX;
  UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MAX_LABEL_VAL);

  for (rn = route_top (NSM_MPLS->ls_table); rn; rn = route_next (rn))
    {
      nls = rn->info;
      if (! nls)
        continue;

      if (! CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_VAL))
        nsm_mpls_ls_max_label_val_unset2 (nls);
    }

  return NSM_SUCCESS;
}

void
nsm_mpls_ls_min_label_val_set2 (struct nsm_label_space *nls, u_int32_t val)
{
  struct nsm_master *nm = nls->nm;
  struct nsm_mpls_if *mif;
  struct listnode *ln;
  cindex_t cindex = 0;

  /* Set new label value. */
  nls->min_label_val = val;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while setting minimum label value");
              continue;
            }

          mif->ifp->ls_data.min_label_value = val;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);

        }
    }
}

/* Set min label value for all interfaces. */
s_int32_t
nsm_mpls_api_min_label_val_set (struct nsm_master *nm, u_int32_t val)
{
  struct route_node *rn;
  struct nsm_label_space *nls;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NSM_SUCCESS;

  /* If no change, return. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MIN_LABEL_VAL)
      && NSM_MPLS->min_label_val == val)
    return NSM_SUCCESS;

  /* If any label space is in use ... */
  if (nsm_lp_server_in_use (nm, 0xffffffff))
    return NSM_ERR_LS_IN_USE;

  /* Set top max label value. */
  NSM_MPLS->min_label_val = val;
  SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MIN_LABEL_VAL);

  for (rn = route_top (NSM_MPLS->ls_table); rn; rn = route_next (rn))
    {
      nls = rn->info;
      if (! nls)
        continue;

      if (! CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_VAL))
        nsm_mpls_ls_min_label_val_set2 (nls, val);
    }

  return NSM_SUCCESS;
}

void
nsm_mpls_ls_min_label_val_unset2 (struct nsm_label_space *nls)
{
  struct nsm_master *nm = nls->nm;
  struct nsm_mpls_if *mif;
  struct listnode *ln;
  cindex_t cindex = 0;

  /* Unset val. */
  nls->min_label_val = LABEL_VALUE_INITIAL;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while unsetting minimum label value");
              continue;
            }

          mif->ifp->ls_data.min_label_value = LABEL_VALUE_INITIAL;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);

        }
    }

  /* If label space value is unused, remove it. */
  if (NSM_LABEL_SPACE_UNUSED (nls))
    nsm_mpls_label_space_del (nls);
}

/* Unset min label value for all interfaces. */
s_int32_t
nsm_mpls_api_min_label_val_unset (struct nsm_master *nm)
{
  struct route_node *rn;
  struct nsm_label_space *nls;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NSM_SUCCESS;

  /* If no change, return. */
  if (! CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MIN_LABEL_VAL))
    return NSM_SUCCESS;

  /* If any label space is in use ... */
  if (nsm_lp_server_in_use (nm, 0xffffffff))
    return NSM_ERR_LS_IN_USE;

  /* Set top min label value. */
  NSM_MPLS->min_label_val = LABEL_VALUE_INITIAL;
  UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_MIN_LABEL_VAL);

  for (rn = route_top (NSM_MPLS->ls_table); rn; rn = route_next (rn))
    {
      nls = rn->info;
      if (! nls)
        continue;

      if (! CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_VAL))
        nsm_mpls_ls_min_label_val_unset2 (nls);
    }

  return NSM_SUCCESS;
}

/* Set max label value for label space. */
s_int32_t
nsm_mpls_api_ls_max_label_val_set (struct nsm_master *nm,
                                   u_int16_t label_space, u_int32_t val)
{
  struct nsm_label_space *nls;
  bool_t is_new;

  /* Get label space object. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    {
      is_new = NSM_TRUE;
      nls = nsm_mpls_label_space_get (nm, label_space);
    }
  else
    is_new = NSM_FALSE;

  if (!nls)
    return NSM_FAILURE;
  /* If no change, return. */
  if (is_new == NSM_FALSE && nls->max_label_val == val)
    return NSM_SUCCESS;

  /* If label space is in use ... */
  if (nsm_lp_server_in_use (nm, label_space))
    return NSM_ERR_LS_IN_USE;

  if (val <= nls->min_label_val)
    return NSM_ERR_LS_INVALID_MAX_LABEL;

  /* Set flag. */
  SET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_VAL);

  /* Set new label value. */
  nsm_mpls_ls_max_label_val_set2 (nls, val);

  return NSM_SUCCESS;
}

/* Unet max label value for interface. */
s_int32_t
nsm_mpls_api_ls_max_label_val_unset (struct nsm_master *nm,
                                     u_int16_t label_space)
{
  struct nsm_label_space *nls;

  /* Lookup label space. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    return NSM_SUCCESS;

  /* If already unset, return. */
  if (! CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_VAL))
    return NSM_SUCCESS;

  /* If label space is in use ... */
  if (nsm_lp_server_in_use (nm, label_space))
    return NSM_ERR_LS_IN_USE;

  /* Unset flag. */
  UNSET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_VAL);

  /* Unset label value. */
  nsm_mpls_ls_max_label_val_unset2 (nls);

  return NSM_SUCCESS;
}

/* Set min label value for label space. */
s_int32_t
nsm_mpls_api_ls_min_label_val_set (struct nsm_master *nm,
                                   u_int16_t label_space, u_int32_t val)
{
  struct nsm_label_space *nls;
  bool_t is_new;

  /* Get label space object. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    {
      is_new = NSM_TRUE;
      nls = nsm_mpls_label_space_get (nm, label_space);
    }
  else
    is_new = NSM_FALSE;

  if (!nls)
    return NSM_FAILURE;

  /* If no change, return. */
  if (is_new == NSM_FALSE && nls->min_label_val == val)
    return NSM_SUCCESS;

  /* If label space is in use ... */
  if (nsm_lp_server_in_use (nm, label_space))
    return NSM_ERR_LS_IN_USE;

  if (val >= nls->max_label_val)
    return NSM_ERR_LS_INVALID_MIN_LABEL;

  /* Set flag. */
  SET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_VAL);

  /* Set new label value. */
  nsm_mpls_ls_min_label_val_set2 (nls, val);

  return NSM_SUCCESS;
}

/* Unet min label value for interface. */
s_int32_t
nsm_mpls_api_ls_min_label_val_unset (struct nsm_master *nm,
                                u_int16_t label_space)
{
  struct nsm_label_space *nls;

  /* Lookup label space. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    return NSM_SUCCESS;

  /* If already unset, return. */
  if (! CHECK_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_VAL))
    return NSM_SUCCESS;

  /* If label space is in use ... */
  if (nsm_lp_server_in_use (nm, label_space))
    return NSM_ERR_LS_IN_USE;

  /* Unset flag. */
  UNSET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_VAL);

  /* Unset val. */
  nsm_mpls_ls_min_label_val_unset2 (nls);

  return NSM_SUCCESS;
}


int
nsm_gmpls_get_lbl_range_service_index (char* module_str)
{
  if (pal_strncmp (module_str, "rsvp", 1) == 0)
    {
      return 0;
    }
#ifdef HAVE_DEV_TEST
  else if (pal_strncmp (module_str, "ldp_lsp", 5) == 0)
    {
      return 1;
   }
  else if (pal_strncmp (module_str, "ldp_pw", 5) == 0)
    {
      return 2;
    }
  else if (pal_strncmp (module_str, "ldp_vpls", 5) == 0)
    {
      return 3;
    }
#else
  else if (pal_strncmp (module_str, "ldp", 1) == 0)
    {
      return 1;
   }
#endif /* !HAVE_DEV_TEST */

  else if (pal_strncmp (module_str, "bgp", 1) == 0)
    {
      return 4;
    }
  else
    {
      return LABEL_POOL_SERVICE_ERROR;
    }
}

char *
nsm_gmpls_get_lbl_range_service_name (u_int8_t index)
{
  switch (index)
    {
      case 0:
        return "rsvp";

      case 1:
        return "ldp";

      case 2:
        return "ldp-pw";

      case 3:
        return "ldp-vpls";

      case 4:
        return "bgp";

      default:
        return "unsupported";
    }
}

/**@brief nsm_mpls_api_min_lbl_range_set - This API is used to accept and
          initialize minimum value of the label range for a specified owner
          or module.
     @param vr_id - the VR identity for which the label range is being configured
     @param *module - the owner or module of the range being configured
     @param *lbl_val - the minimum value of the label range
     @param *lbl_space_id - the label space for the label rangeis being
                            configured
     @return success or relevant failure error code
*/
s_int32_t
nsm_mpls_api_min_lbl_range_set (struct cli* cli,
                            u_int32_t vr_id,
                            char* module,
                            char* lbl_val,
                            u_int16_t label_space)
{
  struct nsm_master *nm = NULL;
  struct nsm_label_space *nls = NULL;
  struct nsm_mpls_if *mif = NULL;
  struct listnode *ln;
  u_int32_t in_label;
  u_int32_t ctr;
  cindex_t cindex = 0;
  int mod_index = -1;

  nm = nsm_master_lookup_by_id (nzg, vr_id);

  if (nm == NULL)
    {
      return NSM_ERR_VR_DOES_NOT_EXIST;
    }

  /* Get label space object. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    {
      nls = nsm_mpls_label_space_get (nm, label_space);
    }

  mod_index = nsm_gmpls_get_lbl_range_service_index (module);

  if (mod_index == -1)
    {
      return NSM_ERR_MODULE_INVALID;
    }

  /* If this is the first element of the range check for usability */
  if (nls->service_ranges[mod_index].to_label == LABEL_UNINITIALIZED_VALUE)
    {
      if (nsm_lp_server_in_use (nm, label_space))
      {
          cli_out (cli, "Label space %d in use for %s\n", label_space, module);
          return NSM_ERR_LS_IN_USE;
      }
    }

  /* Reject the command if the incoming module is already configured */
  if (nls->service_ranges[mod_index].from_label > LABEL_UNINITIALIZED_VALUE)
    {
      cli_out (cli, "Minimum label range value exists for module %s\n",
                    module);
      return NSM_ERR_MODULE_LBL_RANGE_EXISTS;
    }

  /* Reject if the value is out of label space range */
  CLI_GET_UINT32_RANGE ("min-module-lbl-range", in_label, lbl_val,
                        nls->min_label_val, nls->max_label_val);

  if (in_label > nls->max_label_val || in_label < nls->min_label_val ||
      in_label > NSM_MPLS->max_label_val)
    {
      cli_out (cli, "Invalid minimum label range value for module %s\n",
                    module);
      return NSM_ERR_MODULE_LBL_OUT_OF_RANGE;
    }

  /* Reject if the value overlaps with an exisitng sub-range for a module */
  for (ctr = LABEL_POOL_RANGE_INITIAL; ctr < LABEL_POOL_RANGE_MAX; ctr++)
    {
      if (ctr == mod_index)
        {
          /* Skip the incoming service module */
          continue;
        }
      /* Check for closed range */
      if ((nls->service_ranges[ctr].from_label != LABEL_UNINITIALIZED_VALUE) &&
          (nls->service_ranges[ctr].to_label != LABEL_UNINITIALIZED_VALUE))
        {
          if (in_label >= nls->service_ranges[ctr].from_label &&
              in_label <= nls->service_ranges[ctr].to_label)
            {
              cli_out (cli, "Minimum label range value overlaps with another "
                            "module \n");
              return NSM_ERR_MODULE_LBL_OVERLAPPING_RANGE;
            }
        }
      /* Check for open range and force operator to choose another value */
      else if
       (((nls->service_ranges[ctr].from_label != LABEL_UNINITIALIZED_VALUE) &&
            (in_label >= nls->service_ranges[ctr].from_label)) ||
          ((nls->service_ranges[ctr].to_label != LABEL_UNINITIALIZED_VALUE) &&
            (in_label <= nls->service_ranges[ctr].to_label)))
        {
          cli_out (cli, "Min label range value for module %s collides with"
                        " another service; close existing range(s) first\n",
                        module);
          return NSM_ERR_MODULE_LBL_RANGE_COLLISION;
        }
    }

  if (nls->service_ranges[mod_index].to_label != LABEL_UNINITIALIZED_VALUE)
    {
      /* start of label range should always be less than the end of the range */
      if (in_label < nls->service_ranges[mod_index].to_label)
        {
          /* Set min label range value for the module */
          nls->service_ranges[mod_index].from_label = in_label;
        }
      else
        {
          cli_out (cli, "invalid minimum label range value for "
                        "module %s with an end of range value of %d\n",
                        module, nls->service_ranges[mod_index].to_label);
          return NSM_ERR_MODULE_LBL_INVALID_RANGE;
        }
    }
  else
    {
      nls->service_ranges[mod_index].from_label = in_label;

#ifdef HAVE_DEV_TEST
      nsm_mpls_dump_label_ranges (cli, label_space);
#endif
    }

  SET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_RANGE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while setting minimum label range value");
              continue;
            }

          mif->ifp->ls_data.ls_module_ranges[mod_index].from_label = in_label;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);

        }
    }

  return NSM_SUCCESS;
}

/**@brief nsm_mpls_api_min_lbl_range_unset - This API is used to accept and
          un-initialize minimum value of the label range for a specified
          owner or module.
     @param vr_id - the VR identity for which the label range is being configured
     @param *module - the owner or module of the range being configured
     @param *lbl_space_id - the label space for the label rangeis being
                            configured
     @return success or relevant failure error code
*/
s_int32_t
nsm_mpls_api_min_lbl_range_unset (struct cli* cli,
                              u_int32_t vr_id,
                              char* module,
                              u_int16_t label_space)
{
  struct nsm_master *nm = NULL;
  struct nsm_label_space *nls = NULL;
  struct nsm_mpls_if *mif = NULL;
  struct listnode *ln;
  cindex_t cindex = 0;
  int mod_index = -1;

  nm = nsm_master_lookup_by_id (nzg, vr_id);

  if (nm == NULL)
    {
      cli_out (cli, "Minimum label range value unset - VR doesnt exists\n");
      return NSM_ERR_VR_DOES_NOT_EXIST;
    }

  /* Get label space object. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    {
      nls = nsm_mpls_label_space_get (nm, label_space);
    }

  mod_index = nsm_gmpls_get_lbl_range_service_index (module);

  if (mod_index == -1)
    {
      cli_out (cli, "Minimum label range value unset - Invalid module\n");
      return NSM_ERR_MODULE_INVALID;
    }

  nls->service_ranges[mod_index].from_label = LABEL_UNINITIALIZED_VALUE;

  UNSET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MIN_LABEL_RANGE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while unsetting minimum label value");
              continue;
            }

          mif->ifp->ls_data.ls_module_ranges[mod_index].from_label = 0;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);

        }
    }

  return NSM_SUCCESS;
}

/**@brief nsm_mpls_api_max_lbl_range_set - This API is used to accept and
        initialize maximum value of the label range for a specified owner or
        module.
     @param vr_id - the VR identity for which the label range is being configured
     @param *module - the owner or module of the range being configured
     @param *lbl_val - the minimum value of the label range
     @param *lbl_space_id - the label space for the label rangeis being
                            configured
     @return success or relevant failure error code
*/
s_int32_t
nsm_mpls_api_max_lbl_range_set (struct cli* cli,
                             u_int32_t vr_id,
                             char* module,
                             char* lbl_val,
                             u_int16_t label_space)
{
  struct nsm_master *nm = NULL;
  struct nsm_label_space *nls = NULL;
  struct nsm_mpls_if *mif = NULL;
  struct listnode *ln;
  cindex_t cindex = 0;
  u_int32_t in_label;
  u_int32_t ctr;
  int mod_index = -1;

  nm = nsm_master_lookup_by_id (nzg, vr_id);

  if (nm == NULL)
    {
      return NSM_ERR_VR_DOES_NOT_EXIST;
    }

  /* Get label space object. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    {
      nls = nsm_mpls_label_space_get (nm, label_space);
    }

  mod_index = nsm_gmpls_get_lbl_range_service_index (module);

  if (mod_index == -1)
    {
      return NSM_ERR_MODULE_INVALID;
    }

  /* If this is the first element of the range check for usability */
  if (nls->service_ranges[mod_index].from_label == LABEL_UNINITIALIZED_VALUE)
    {
      if (nsm_lp_server_in_use (nm, label_space))
      {
          cli_out (cli, "Label space %d in use for %s\n", label_space, module);
          return NSM_ERR_LS_IN_USE;
      }
    }

  /* Reject the command if the incoming module is already configured */
  if (nls->service_ranges[mod_index].to_label > 0)
    {
      cli_out (cli, "Maximum label range value already configured for %s\n",
                    module);
      return NSM_ERR_MODULE_LBL_RANGE_EXISTS;
    }

  /* Reject if the value is out of label space range */
  CLI_GET_UINT32_RANGE ("max-module-lbl-range", in_label, lbl_val,
                        nls->min_label_val, nls->max_label_val);

  if ((in_label > nls->max_label_val) || (in_label < nls->min_label_val))
    {
      cli_out (cli, "Invalid maximum label range value for module %s\n",
                    module);
      return NSM_ERR_MODULE_LBL_OUT_OF_RANGE;
    }

  /* Reject if the value overlaps with an exisitng sub-range for a module */
  for (ctr = LABEL_POOL_RANGE_INITIAL; ctr < LABEL_POOL_RANGE_MAX; ctr++)
    {
      if (ctr == mod_index)
        {
          /* Skip the incoming service module */
          continue;
        }
      /* Check for closed range */
      if ((nls->service_ranges[ctr].from_label != LABEL_UNINITIALIZED_VALUE) &&
          (nls->service_ranges[ctr].to_label != LABEL_UNINITIALIZED_VALUE))
        {
          if ((in_label >= nls->service_ranges[ctr].from_label) &&
              (in_label <= nls->service_ranges[ctr].to_label))

            {
              cli_out (cli, "Maximum label range value for module %s "
                            "overlaps with another service range\n", module);
              return NSM_ERR_MODULE_LBL_OVERLAPPING_RANGE;
            }
        }
      /* Check for open range and force operator to choose another value */
      else if
         (((nls->service_ranges[ctr].to_label != LABEL_UNINITIALIZED_VALUE) &&
           (in_label <= nls->service_ranges[ctr].to_label)) ||
          ((nls->service_ranges[ctr].from_label != LABEL_UNINITIALIZED_VALUE) &&
           (in_label >= nls->service_ranges[ctr].from_label)))
        {
          cli_out (cli, "Max label range value for module %s collides with"
                        " another service, close existing range(s) first.\n",
                        module);
          return NSM_ERR_MODULE_LBL_RANGE_COLLISION;
        }
    }

  /* Set max label range value for the module */
  if (nls->service_ranges[mod_index].from_label != LABEL_UNINITIALIZED_VALUE)
    {
      /* End of range should always be greater than the start of the range */
      if (in_label > nls->service_ranges[mod_index].from_label)
        {
          /* Set max label range value for the module */
          nls->service_ranges[mod_index].to_label = in_label;

#ifdef HAVE_DEV_TEST
          nsm_mpls_dump_label_ranges (cli, label_space);
#endif
        }
      else
        {
          cli_out (cli, "invalid maximum label range value for "
                        "module %s with a start of range value of %d\n",
                        module, nls->service_ranges[mod_index].from_label);
          return NSM_ERR_MODULE_LBL_INVALID_RANGE;
        }
    }
  else
    {
      nls->service_ranges[mod_index].to_label = in_label;
    }

  SET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_RANGE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while setting maximum label range value");
              continue;
            }

          mif->ifp->ls_data.ls_module_ranges[mod_index].to_label = in_label;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);
        }
    }

  return NSM_SUCCESS;
}

/**@brief nsm_mpls_api_min_lbl_range_unset - This API is used to accept and
        un-initialize minimum value of the label range for a specified owner
        or module.
     @param vr_id - the VR identity for which the label range is being configured
     @param *module - the owner or module of the range being configured
     @param *lbl_space_id - the label space for the label rangeis being configured
     @return success or relevant failure error code
*/
s_int32_t
nsm_mpls_api_max_lbl_range_unset (struct cli* cli,
                               u_int32_t vr_id,
                               char *module,
                               u_int16_t label_space)
{
  struct nsm_master *nm = NULL;
  struct nsm_mpls_if *mif;
  struct nsm_label_space *nls = NULL;
  struct listnode *ln;
  cindex_t cindex = 0;
  int mod_index = -1;

  nm = nsm_master_lookup_by_id (nzg, vr_id);

  if (nm == NULL)
    {
      cli_out (cli, "Maximum label range value unset - VR doesnt exists\n");
      return NSM_ERR_VR_DOES_NOT_EXIST;
    }

  /* Get label space object. */
  nls = nsm_mpls_label_space_lookup (nm, label_space);
  if (! nls)
    {
      nls = nsm_mpls_label_space_get (nm, label_space);
    }

  mod_index = nsm_gmpls_get_lbl_range_service_index (module);

  if (mod_index == -1)
    {
      cli_out (cli, "Maximum label range value unset - Invalid module\n");
      return NSM_ERR_MODULE_INVALID;
    }

  nls->service_ranges[mod_index].to_label = LABEL_UNINITIALIZED_VALUE;

  UNSET_FLAG (nls->config, NSM_LABEL_SPACE_CONFIG_MAX_LABEL_RANGE);

  /* Re-set all interfaces with this labelpsace. */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (mif->nls == nls)
        {
          NSM_ASSERT (mif->ifp != NULL);
          if (! mif->ifp)
            {
              zlog_err (NSM_ZG, "Internal Error: Interface object mismatch "
                        "while unsetting maximum label range value");
              continue;
            }

          mif->ifp->ls_data.ls_module_ranges[mod_index].to_label = 0;
          if (if_is_running (mif->ifp))
            nsm_server_if_up_update (mif->ifp, cindex);
          else
            nsm_server_if_down_update (mif->ifp, cindex);
        }
    }

  return NSM_SUCCESS;
}

#ifdef HAVE_DEV_TEST
void
nsm_mpls_dump_label_ranges (struct cli* cli,
                          u_int16_t lbl_space)
{
  struct nsm_label_space *nls = NULL;
  struct nsm_master *nm = cli->vr->proto;
  char* serv_str = NULL;
  int ctr;

  nm = nsm_master_lookup_by_id (nzg, cli->vr->id);

  if (nm == NULL)
    {
      return;
    }

  nls = nsm_mpls_label_space_lookup (nm, lbl_space);
  if (! nls)
    {
      nls = nsm_mpls_label_space_get (nm, lbl_space);
    }

  zlog_info (NSM_ZG, "Label range values (min, max):\n");
  for (ctr = LABEL_POOL_RANGE_INITIAL; ctr < LABEL_POOL_RANGE_MAX; ctr++)
    {
      serv_str = nsm_gmpls_get_lbl_range_service_name(ctr);
      zlog_info (NSM_ZG, "%-12s   %-8d   %-8d \n",
               serv_str,
               nls->service_ranges[ctr].from_label,
               nls->service_ranges[ctr].to_label);
    }

  zlog_info (NSM_ZG, "\n0 ==> Uninitialized\n");
}
#endif /* HAVE_DEV_TEST */

/* Set custom TTL for LSPs for which this LSR is the ingress. */
#ifdef HAVE_PACKET
void
nsm_mpls_api_ingress_ttl_set (struct nsm_master *nm, s_int32_t ttl_val)
{
  /* If no change, return. */
  if (NSM_MPLS->ingress_ttl == ttl_val)
    return;

  /* Set new ttl. */
  NSM_MPLS->ingress_ttl = ttl_val;

  /* Set correct flag. */
  if (ttl_val == -1)
    UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_INGRESS_TTL);
  else
    SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_INGRESS_TTL);

  {
    s_int32_t ret;

    ret = apn_mpls_send_ttl (APN_PROTO_NSM, MPLS_TTL_VALUE_SET, 1 /* is ingress */, ttl_val);
    if (ret < 0)
      zlog_warn (NSM_ZG, "Could not set ingress ttl value to %d", ttl_val);
    else if (IS_NSM_DEBUG_EVENT)
      zlog_warn (NSM_ZG, "Successfully set ingress ttl value to %d", ttl_val);
  }
}

/* Set custom TTL for LSPs for which this LSR is the egress. */
void
nsm_mpls_api_egress_ttl_set (struct nsm_master *nm, s_int32_t ttl_val)
{
  /* If no change, return. */
  if (NSM_MPLS->egress_ttl == ttl_val)
    return;

  /* Set new ttl. */
  NSM_MPLS->egress_ttl = ttl_val;

  /* Set correct flag. */
  if (ttl_val == -1)
    UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_EGRESS_TTL);
  else
    SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_EGRESS_TTL);

  {
    s_int32_t ret;

    ret = apn_mpls_send_ttl (APN_PROTO_NSM, MPLS_TTL_VALUE_SET, 0 /* is egress */, ttl_val);
    if (ret < 0)
      zlog_warn (NSM_ZG, "Could not set egress ttl value to %d", ttl_val);
    else if (IS_NSM_DEBUG_EVENT)
      zlog_info (NSM_ZG, "Successfully set egress ttl value to %d", ttl_val);
  }
}

#ifdef HAVE_DIFFSERV
void
nsm_mpls_api_pipe_model_update (struct nsm_master *nm, bool_t set)
{
  s_int32_t ret;
  u_char msg_type;
  char *model = set ? "pipe":"uniform";

  if ((set && (NSM_MPLS->lsp_model == LSP_MODEL_PIPE))
      || (!set && (NSM_MPLS->lsp_model == LSP_MODEL_UNIFORM)))
    {
      return;
    }

  if (set)
    {
      NSM_MPLS->lsp_model = LSP_MODEL_PIPE;
      msg_type            = MPLS_LSP_MODEL_PIPE_SET;
    }
  else
    {
      NSM_MPLS->lsp_model = LSP_MODEL_UNIFORM;
      msg_type            = MPLS_LSP_MODEL_PIPE_UNSET;
    }

    ret = apn_mpls_send_ttl (APN_PROTO_NSM, msg_type, -1, -1);

    if (ret < 0)
      zlog_warn (NSM_ZG, "Could not set lsp model to %s", model);
    else if (IS_NSM_DEBUG_EVENT)
      zlog_warn (NSM_ZG, "Successfully set lsp model to %s", model);
}
#endif /* HAVE_DIFFSERV */

void
nsm_mpls_api_ttl_propagate_cap_update (struct nsm_master *nm, bool_t set)
{
  s_int32_t ret;
  u_char msg_type;
  char *status = set ? "enable":"disable";

  if ((set && NSM_MPLS->propagate_ttl == TTL_PROPAGATE_ENABLED)
      || (!set && NSM_MPLS->propagate_ttl == TTL_PROPAGATE_DISABLED))
    {
      return;
    }

  if (set)
    {
      NSM_MPLS->propagate_ttl = TTL_PROPAGATE_ENABLED;
      msg_type                = MPLS_TTL_PROPAGATE_SET;
    }
  else
    {
      NSM_MPLS->propagate_ttl = TTL_PROPAGATE_DISABLED;
      msg_type                = MPLS_TTL_PROPAGATE_UNSET;
    }

  ret = apn_mpls_send_ttl (APN_PROTO_NSM, msg_type, -1, -1);

  if (ret < 0)
    zlog_warn (NSM_ZG, "Could not %s TTL Propagation", status);
  else if (IS_NSM_DEBUG_EVENT)
    zlog_warn (NSM_ZG, "Successfully %s TTL Propagation", status);
}

#endif /* HAVE_PACKET */
/* Enable/Disable local packet handling of locally generated TCP packets. */
void
nsm_mpls_api_local_pkt_handling (struct nsm_master *nm, bool_t bool)
{
  /* If no change, return. */
  if (((bool == NSM_TRUE) &&
       (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_LCL_PKT_HANDLE)))
      || ((bool == NSM_FALSE) &&
          (! CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_LCL_PKT_HANDLE))))
    return;

  /* Set/Unset flag. */
  if (bool == NSM_TRUE)
    SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_LCL_PKT_HANDLE);
  else if (bool == NSM_FALSE)
    UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_LCL_PKT_HANDLE);

  {
    s_int32_t ret;

    ret = apn_mpls_local_pkt_handle (APN_PROTO_NSM,
                                     ((bool == NSM_TRUE)
                                      ? MPLS_INIT : MPLS_END));
    if (ret < 0)
      zlog_warn (NSM_ZG, "Could not enable labeling of locally "
                 "generated TCP packets");
    else if (IS_NSM_DEBUG_EVENT)
      zlog_info (NSM_ZG, "Successfully enabled labeling of locally "
                 "generated TCP packets");
  }
}

struct ptree *
nsm_gmpls_get_mapped_routes_table (struct nsm_master *nm,
                                   struct fec_gen_entry *ent)
{
  switch (ent->type)
    {
       case gmpls_entry_type_ip:
#ifdef HAVE_PACKET
         return nm->nmpls->mapped_routes;
#else
         break;
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
       case gmpls_entry_type_tdm:
#ifdef HAVE_TDM
         return nm->nmpls->mapped_routes_tdm;
#else
         break;
#endif /* HAVE_TDM */

       case gmpls_entry_type_pbb_te:
#ifdef HAVE_PBB_TE
         return nm->nmpls->mapped_routes_pbb;
#else
         break;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
       default:
          break;
    }

  return NULL;
}

void
nsm_gmpls_generate_mapped_key (struct fec_gen_entry *ent, u_char *key,
                               u_int16_t *key_len)
{
  *key_len = 0;

  switch (ent->type)
    {
       case gmpls_entry_type_ip:
         if (ent->u.prefix.family == AF_INET)
           {
             *key_len = ent->u.prefix.prefixlen;
             pal_mem_cpy (key, ent->u.prefix.u.val, IPV4_MAX_BYTELEN);
           }
#ifdef HAVE_IPV6
         else
           {
             *key_len = ent->u.prefix.prefixlen;
             pal_mem_cpy (key, ent->u.prefix.u.val, IPV6_MAX_BYTELEN);
           }
#endif /* HAVE_IPV6 */
         break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         *key_len = 8 * sizeof (struct fec_entry_pbb_te);
         pal_mem_cpy (key, &ent->u.pbb, sizeof (struct fec_entry_pbb_te));
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         *key_len = 8 * sizeof (struct fec_entry_tdm);
         pal_mem_cpy (key, ent->u.tdm, sizeof (struct fec_entry_tdm));
         break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
       default:
         break;
    }

  return;
}

/* Add a mapped route. */
s_int32_t
nsm_gmpls_api_add_mapped_route (struct nsm_master *nm,
                                struct fec_gen_entry *ent, struct prefix *fec)
{
  struct fec_gen_entry gen_fec;
  struct ptree *table;
  struct mapped_route *route;
  struct ptree_node *pn;
  u_char key [40];
  u_int16_t key_len;

  /* Lookup node in mapped route table. */
  table = nsm_gmpls_get_mapped_routes_table (nm, ent);
  if (! table)
    return NSM_FAILURE;

  pal_mem_set (&gen_fec, 0, sizeof (struct fec_gen_entry));
  nsm_gmpls_generate_mapped_key (ent, key, &key_len);
  pn = ptree_node_lookup (table, key, key_len);
  if (! pn)
    {
      route = XCALLOC (MTYPE_MPLS_MAPPED_ROUTE, sizeof (struct mapped_route));
      if (! route)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      pn = ptree_node_get (table, key, key_len);
      if (! pn)
        {
          XFREE (MTYPE_MPLS_MAPPED_ROUTE, route);
          return NSM_ERR_MEM_ALLOC_FAILURE;
        }

      NSM_ASSERT (rn->info == NULL);

      /* Fill data. */
      if (fec->family == AF_INET)
         route->type = MAPPED_ENTRY_TYPE_IPv4;
#ifdef HAVE_IPV6
      else
        route->type = MAPPED_ENTRY_TYPE_IPV6;
#endif  /* HAVE_IPV6 */
      route->owner = MPLS_OTHER_CLI;
      prefix_copy (&route->fec, fec);
      pn->info = route;
    }
  else /* Update. */
    {
      ptree_unlock_node (pn);
      route = pn->info;
      if (prefix_same (fec, &route->fec))
        return NSM_SUCCESS;

      /* Delete old VPN entry. */
      gen_fec.type = gmpls_entry_type_ip;
      gen_fec.u.prefix = route->fec;

      gmpls_lsp_dep_del (nm, &gen_fec, CONFIRM_MAPPED_ROUTE, pn);

      /* Update FEC. */
      prefix_copy (&route->fec, fec);
    }

  /* Copy over new mapped route. */
  pal_mem_set (&gen_fec, 0, sizeof (struct fec_gen_entry));
  gen_fec.type = gmpls_entry_type_ip;
  gen_fec.u.prefix = *fec;

  gmpls_lsp_dep_add (nm, &gen_fec, CONFIRM_MAPPED_ROUTE, pn);

  return NSM_SUCCESS;
}

/* Delete a mapped route. */
s_int32_t
nsm_gmpls_api_del_mapped_route (struct nsm_master *nm,
                               struct fec_gen_entry *ent, struct prefix *fec)
{
  struct ptree *table;
  struct mapped_route *route;
  struct ptree_node *pn;
  struct fec_gen_entry gen_fec;
  char key [sizeof (struct fec_gen_entry)];
  u_int16_t key_len;

  /* Lookup node in mapped route table. */
  table = nsm_gmpls_get_mapped_routes_table (nm, ent);
  if (! table)
    return NSM_FAILURE;

  nsm_gmpls_generate_mapped_key (ent, key, &key_len);
  pn = ptree_node_lookup (table, key, key_len);
  if (! pn)
    return NSM_ERR_NOT_FOUND;

  ptree_unlock_node (pn);
  route = pn->info;
  if (! prefix_same (fec, &route->fec))
    return NSM_ERR_INVALID_ARGS;

  /* Delete entry. */
  gen_fec.type = gmpls_entry_type_ip;
  gen_fec.u.prefix = *fec;
  gmpls_lsp_dep_del (nm, &gen_fec, CONFIRM_MAPPED_ROUTE, pn);

  /* Free up memory. */
  XFREE (MTYPE_MPLS_MAPPED_ROUTE, route);
  pn->info = NULL;
  ptree_unlock_node (pn);

  return NSM_SUCCESS;
}

#ifdef HAVE_TE
/* Add an admin group. */
s_int32_t
nsm_mpls_api_admin_group_add (struct nsm_master *nm, char *name, s_int32_t val)
{
  s_int32_t ret;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  /* Make sure name size is valid. */
  if (pal_strlen (name) >= ADMIN_GROUP_NAMSIZ)
    return NSM_ERR_NAME_TOO_LONG;

  /* Add admin group. */
  ret = admin_group_add (NSM_MPLS->admin_group_array, name, val);
  if (ret < 0)
    return ret;

  /* Send update to overlying protocol. */
  nsm_admin_group_update (nm);

  return NSM_SUCCESS;
}

/* Check if the admin group is being used in interface config. */
s_int32_t
nsm_mpls_admin_group_check_if (struct nsm_master *nm, s_int32_t val)
{
  struct interface *ifp;
  struct route_node *rn;

  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if (CHECK_FLAG (ifp->admin_group, (1 << val)))
        {
          route_unlock_node (rn);
          return NSM_ERR_NAME_IN_USE;
        }

  return NSM_SUCCESS;
}

/* Delete an admin group. */
s_int32_t
nsm_mpls_api_admin_group_del (struct nsm_master *nm, char *name, s_int32_t val)
{
  s_int32_t ret;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  /* Make sure name size is valid. */
  if (pal_strlen (name) >= ADMIN_GROUP_NAMSIZ)
    return NSM_ERR_NAME_TOO_LONG;

  /* First check whether this name actually exists */
  if (pal_strcasecmp (NSM_MPLS->admin_group_array[val].name, name) != 0)
    return ADMIN_GROUP_ERR_EEXIST;

  ret = nsm_mpls_admin_group_check_if (nm, val);
  if (ret < 0)
    return ret;

  ret = admin_group_del (NSM_MPLS->admin_group_array, name, val);
  if (ret < 0)
    return ret;

  /* Send update to overlying protocol. */
  nsm_admin_group_update (nm);

  return NSM_SUCCESS;
}
#endif /* HAVE_TE */

/* Internal utility functions. */
struct prefix *
nsm_mpls_api_cal_max_addr (struct prefix *p1, struct prefix *p2)
{
  s_int32_t i;
  s_int32_t p_len;

  prefix_copy (p2, p1);

  if (p1->family == AF_INET)
    {
      p2->u.prefix4.s_addr = pal_hton32 (p2->u.prefix4.s_addr);
      p_len = IPV4_MAX_BITLEN;
    }
#ifdef HAVE_IPV6
  else if (p1->family == AF_INET6)
    {
      for (i=0; i < 4; i++)
        p2->u.prefix6.s6_addr32[i] =
          pal_hton32 (p2->u.prefix6.s6_addr32[i]);
      p_len = IPV6_MAX_BITLEN;
    }
#endif /* HAVE_IPV6 */
  else
    return NULL;

  for (i = 0; i < (p_len - p1->prefixlen); i++)
    {
      if (p1->family == AF_INET)
        p2->u.prefix4.s_addr |= (1 << i);
#ifdef HAVE_IPV6
      else if (p1->family == AF_INET6)
        p2->u.prefix4.s_addr |= (1 << i);
#endif /* HAVE_IPV6 */
    }

  if (p1->family == AF_INET)
    p2->u.prefix4.s_addr = pal_hton32 (p2->u.prefix4.s_addr);
#ifdef HAVE_IPV6
  else if (p1->family == AF_INET6)
    for (i=0; i < 4; i++)
      p2->u.prefix6.s6_addr32[i] =
        pal_hton32 (p2->u.prefix6.s6_addr32[i]);
#endif /* HAVE_IPV6 */
  else
    return NULL;

  return p2;
}

bool_t
nsm_gmpls_mapped_lsp_key_same (struct mapped_lsp_entry_head *map1,
                               struct mapped_lsp_entry_head *map2)
{
  if ((map1->type != map2->type) ||
      (map1->iif_ix != map2->iif_ix))
    {
      return PAL_FALSE;
    }

  switch (map1->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        {
          struct mapped_lsp_entry_pkt *pkt1, *pkt2;

          pkt1 = (struct mapped_lsp_entry_pkt *) map1;
          pkt2 = (struct mapped_lsp_entry_pkt *) map2;

          if (pkt1->in_label != pkt2->in_label)
            {
              return PAL_FALSE;
            }
          break;
        }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct mapped_lsp_entry_pbb *pkt1, *pkt2;

          pkt1 = (struct mapped_lsp_entry_pbb *) map1;
          pkt2 = (struct mapped_lsp_entry_pbb *) map2;

          if (!(pal_mem_cmp(&pkt1->in_label, &pkt2->in_label, sizeof(struct pbb_te_label))))
            {
              return PAL_FALSE;
            }
          break;
        }

#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct mapped_lsp_entry_tdm *pkt1, *pkt2;

          pkt1 = (struct mapped_lsp_entry_tdm *) map1;
          pkt2 = (struct mapped_lsp_entry_tdm *) map2;

          if (pkt1->in_label != pkt2->in_label)
            {
              return PAL_FALSE;
            }
          break;
        }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        return PAL_FALSE;
    }

  return PAL_TRUE;
}

bool_t
nsm_gmpls_mapped_lsp_is_label_equal (struct mapped_lsp_entry_head *map,
                                     struct gmpls_gen_label *lbl, bool_t out)
{
  if (map->type != lbl->type)
    {
      return PAL_FALSE;
    }

  switch (map->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        {
          struct mapped_lsp_entry_pkt *pkt;

          pkt = (struct mapped_lsp_entry_pkt *) map;

          if ((out && pkt->out_label != lbl->u.pkt) ||
              (!out && pkt->in_label != lbl->u.pkt))
            {
              return PAL_FALSE;
            }
          break;
        }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct mapped_lsp_entry_pbb *pbb;

          pbb = (struct mapped_lsp_entry_pbb *) map;

          if ((out && (pal_mem_cmp(&pbb->out_label,
                    &lbl->u.pbb, sizeof(struct pbb_te_label)))) ||
              (!out && (pal_mem_cmp(&pbb->in_label ,
                    &lbl->u.pbb, sizeof(struct pbb_te_label)))))
            {
              return PAL_FALSE;
            }
          break;
        }

#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct mapped_lsp_entry_tdm tdm;

          tdm = (struct mapped_lsp_entry_tdm *) map;

          if ((out && pkt->out_label != lbl->u.tdm) ||
              (!out && pkt->in_label != lbl->u.tdm))
            {
              return PAL_FALSE;
            }
          break;
        }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        return PAL_FALSE;
    }

  return PAL_TRUE;
}

bool_t
nsm_gmpls_mapped_lsp_is_fec_equal (struct mapped_lsp_entry_head *map,
                                   struct fec_gen_entry *fec)
{
  if (map->type != fec->type)
    {
      return PAL_FALSE;
    }

  switch (map->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        {
          struct mapped_lsp_entry_pkt *pkt;

          pkt = (struct mapped_lsp_entry_pkt *) map;

          if (prefix_cmp (&pkt->fec, &fec->u.prefix))
            {
              return PAL_FALSE;
            }
          break;
        }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct mapped_lsp_entry_pbb *pbb;

          pbb = (struct mapped_lsp_entry_pbb *) map;

          if (pal_mem_cmp (&pbb->fec, &fec->u.pbb, sizeof(struct fec_entry_pbb_te)))
            {
              return PAL_FALSE;
            }
          break;
        }

#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct mapped_lsp_entry_tdm tdm;

          tdm = (struct mapped_lsp_entry_tdm *) map;

          if (pal_mem_cmp (&tdm->fec, &fec->u.tdm))
            {
              return PAL_FALSE
            }
          break;
        }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        return PAL_FALSE;
    }

  return PAL_TRUE;
}

s_int32_t
nsm_gmpls_api_gen_add_lsp_tunnel (struct nsm_master *nm, struct interface *ifp,
                                  struct gmpls_gen_label in_label,
                                  struct gmpls_gen_label out_label,
                                  struct fec_gen_entry *fec)
{
  struct mapped_lsp_entry_head *entry;
  s_int32_t ret;

  entry = nsm_gmpls_mapped_lsp_lookup (nm, ifp->ifindex, &in_label);
  if (entry)
    {
      if (nsm_gmpls_mapped_lsp_is_label_equal (entry, &out_label, PAL_TRUE) &&
          nsm_gmpls_mapped_lsp_is_fec_equal (entry, fec))
        {
          return NSM_SUCCESS;
        }

      nsm_gmpls_mapped_lsp_del (nm, ifp->ifindex, &in_label);
    }

  ret = nsm_gmpls_mapped_lsp_add (nm, ifp->ifindex, &in_label, &out_label, fec);
  return ret;
}


s_int32_t
nsm_gmpls_api_del_lsp_tunnel (struct nsm_master *nm, struct interface *ifp,
                              struct gmpls_gen_label in_label)
{
  s_int32_t ret;

  ret = nsm_gmpls_mapped_lsp_del (nm, ifp->ifindex, &in_label);
  return ret;
}

s_int32_t
nsm_gmpls_api_add_lsp_tunnel (struct nsm_master *nm, struct interface *ifp,
                             u_int32_t il,
                             u_int32_t ol,
                             struct prefix *pfx)
{
  struct gmpls_gen_label in_label, out_label;
  struct fec_gen_entry fec;

  in_label.type = gmpls_entry_type_ip;
  in_label.u.pkt = il;

  out_label.type = gmpls_entry_type_ip;
  out_label.u.pkt = ol;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *pfx;

  return nsm_gmpls_mapped_lsp_add (nm, ifp->ifindex, &in_label, &out_label,
                                   &fec);
}

s_int32_t
nsm_mpls_api_del_lsp_tunnel (struct nsm_master *nm, struct interface *ifp,
                             u_int32_t in_label)
{
  s_int32_t ret;
  struct gmpls_gen_label lbl;

  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = in_label;
  ret = nsm_gmpls_api_del_lsp_tunnel (nm, ifp, lbl);
  return ret;
}

void
nsm_gmpls_ftn_delete (struct nsm_master *nm, struct ftn_entry *ftn,
                      u_int32_t ftn_ix, struct fec_gen_entry *fec,
                      struct ptree_ix_table *pix_tbl)
{
#ifdef HAVE_DEV_TEST
  struct ftn_entry *ftn_tmp;
#endif /* HAVE_DEV_TEST */
#ifdef HAVE_DEV_TEST
  ftn_tmp = nsm_gmpls_ftn_lookup (nm, pix_tbl, fec, ftn_ix, NSM_TRUE);
  NSM_ASSERT (ftn_tmp == ftn);
#else /* HAVE_DEV_TEST */
  (void) nsm_gmpls_ftn_lookup (nm, pix_tbl, fec, ftn_ix, NSM_TRUE);
#endif /* HAVE_DEV_TEST */

  gmpls_ftn_entry_cleanup (nm, ftn, fec, pix_tbl);
  gmpls_ftn_free (ftn);
}

#ifdef HAVE_SNMP
/* SNMP TRAP Callback API. */
/* Set MPLS snmp trap callback function.
   If trapid == MPLS_TRAP_ALL
   register all supported trap. */
s_int32_t
mpls_trap_callback_set (int trapid, SNMP_TRAP_CALLBACK func)
{
  struct nsm_master *nm = nsm_master_lookup_by_id (nzg, 0);
  struct nsm_mpls *nmpls = NULL;

  s_int32_t i, j, found;
  vector v;

  if (!nm)
    return NSM_API_SET_ERROR;
 
  nmpls =  nm->nmpls;
  if (trapid < MPLS_TRAP_ALL || trapid > MPLS_TRAP_ID_MAX)
    return NSM_API_SET_ERROR;

  if (trapid == MPLS_TRAP_ALL)
    {
      for (i = 0; i < MPLS_TRAP_ID_MAX; i++)
        {
          found = 0;
          v = nmpls->traps[i];

          for (j = 0; j < vector_max (v); j++)
            if (vector_slot (v, j) == func)
              {
                found = 1;
                break;
              }

          if (found == 0)
            vector_set (v, func);
        }
      return NSM_API_SET_SUCCESS;
    }

  /* Get trap callback vector. */
  v = nmpls->traps[trapid - 1];
  for (j = 0; j < vector_max (v); j++)
    if (vector_slot (v, j) == func)
      return NSM_API_SET_SUCCESS;

  vector_set (v, func);

  return NSM_API_SET_SUCCESS;
}

s_int32_t
nsm_mpls_set_notify (struct nsm_mpls *nmpls, s_int32_t val)
{
  if (nmpls != NULL)
    {
      nmpls->notificationcntl = val;
      return NSM_API_SET_SUCCESS;
    }
  return NSM_API_SET_ERROR;
}

s_int32_t
nsm_gmpls_get_notify (struct nsm_mpls *nmpls, s_int32_t *val)
{
  if (nmpls != NULL)
    {
      if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA)
        *val = NSM_MPLS_NOTIFICATIONCNTL_ENA;
      else
        *val = NSM_MPLS_NOTIFICATIONCNTL_DIS;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

/* APIs related with MPLS Interface Configuration Table. */
u_int32_t
nsm_gmpls_get_if_lb_min_in (struct nsm_master *nm,
                           u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = LABEL_VALUE_INITIAL;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_lb_min_in (struct nsm_master *nm,
                                u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret =  LABEL_VALUE_INITIAL;
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_lb_max_in (struct nsm_master *nm,
                           u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret =  LABEL_VALUE_MAX;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_lb_max_in (struct nsm_master *nm,
                                u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret =   LABEL_VALUE_MAX;
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_lb_min_out (struct nsm_master *nm,
                            u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = ifp->ls_data.min_label_value;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_lb_min_out (struct nsm_master *nm,
                                 u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret = ifp->ls_data.min_label_value;
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_lb_max_out (struct nsm_master *nm,
                            u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = ifp->ls_data.max_label_value;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_lb_max_out (struct nsm_master *nm,
                                 u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret = ifp->ls_data.max_label_value;
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_total_bw (struct nsm_master *nm,
                          u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = ifp->bandwidth*8/1000; /* byte to kbit. */
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_total_bw (struct nsm_master *nm,
                               u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret = ifp->bandwidth*8/1000; /* byte to kbit. */
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_ava_bw (struct nsm_master *nm,
                        u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
#ifdef HAVE_TE
      /* Give the available bandwidth of lowest priority. */
      *ret = ifp->tecl_priority_bw[7]*8/1000; /* byte to kbit. */
#endif /* HAVE_TE */
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_ava_bw (struct nsm_master *nm,
                             u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

#ifdef HAVE_TE
      /* Give the available bandwidth of lowest priority. */
      *ret = ifp->tecl_priority_bw[7]*8/1000; /* byte to kbit. */
#endif /* HAVE_TE */
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_lb_pt_type (struct nsm_master *nm,
                            u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      if (ifp->ls_data.label_space == 0)
        *ret = NSM_MPLS_SNMP_LBSP_PLATFORM;
      else
        *ret = NSM_MPLS_SNMP_LBSP_INTERFACE;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_lb_pt_type (struct nsm_master *nm,
                                 u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      if (ifp->ls_data.label_space == 0)
        *ret = NSM_MPLS_SNMP_LBSP_PLATFORM;
      else
        *ret = NSM_MPLS_SNMP_LBSP_INTERFACE;
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

/* APIs related with MPLS Interface Performance Table. */
u_int32_t
nsm_gmpls_get_if_in_lb_used (struct nsm_master *nm,
                            u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = nsm_cnt_in_lb_used (ifp);
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_in_lb_used (struct nsm_master *nm,
                                 u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret = nsm_cnt_in_lb_used (ifp);
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_failed_lb_lookup (struct nsm_master *nm,
                                  u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = nsm_cnt_failed_lb_lookup (ifp);
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_failed_lb_lookup (struct nsm_master *nm,
                                       u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret = nsm_cnt_failed_lb_lookup (ifp);
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_out_lb_used (struct nsm_master *nm,
                             u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = nsm_cnt_out_lb_used (ifp) ;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_out_lb_used (struct nsm_master *nm,
                                  u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret = nsm_cnt_out_lb_used (ifp);
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

u_int32_t
nsm_gmpls_get_if_out_fragments (struct nsm_master *nm,
                               u_int32_t if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index (&nm->vr->ifm, if_ix);
  if (ifp != NULL)
    {
      *ret = nsm_cnt_out_fragments (ifp);
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_if_out_fragments (struct nsm_master *nm,
                                    u_int32_t *if_ix, u_int32_t *ret)
{
  struct interface *ifp;

  ifp = if_mpls_lookup_by_index_next (&nm->vr->ifm, *if_ix);
  if (ifp != NULL)
    {
      /* Set next entry index. */
      *if_ix = ifp->ifindex;

      *ret = nsm_cnt_out_fragments (ifp);
      return NSM_API_GET_SUCCESS;
    }
  else
    {
      *if_ix = 0;
      return NSM_API_GET_ERROR;
    }
}

/* Support APIs for InSegmentTable Set */
/* ILM Lookup - Both temporary and main Data structures */
void
nsm_mpls_inseg_lookup (struct nsm_master *nm, u_int32_t inseg_ix,
                       struct ilm_temp_ptr **ilm_temp,
                       struct ilm_entry **ilm)
{
  *ilm = nsm_gmpls_ilm_ix_lookup (nm, inseg_ix);
  *ilm_temp = nsm_gmpls_ilm_temp_lookup (inseg_ix, 0, 0, IDX_LOOKUP);
}

/* ILM Lookup Next - Both temporary and main Data structures */
void
nsm_gmpls_inseg_lookup_next (struct nsm_master *nm, u_int32_t inseg_ix,
                            struct ilm_temp_ptr **ilm_temp,
                            struct ilm_entry **ilm)
{
  *ilm = nsm_gmpls_ilm_ix_lookup_next (nm, inseg_ix);
  *ilm_temp = nsm_gmpls_ilm_temp_lookup_next (inseg_ix);
}

/* Set Index for Get Next */
void
nsm_gmpls_get_next_inseg_index_set (struct ilm_entry *ilm,
                                   struct ilm_temp_ptr *ilm_temp,
                                   u_int32_t *inseg_ix, u_int32_t *index_type)
{
  if ((ilm && !ilm_temp) ||
      (ilm && ilm_temp && ilm->ilm_ix < ilm_temp->ilm_ix))
    {
      *inseg_ix = ilm->ilm_ix;
      *index_type = TYPE_MAIN;
    }

  else if ((!ilm && ilm_temp) ||
           (ilm && ilm_temp && ilm->ilm_ix > ilm_temp->ilm_ix))
    {
      *inseg_ix = ilm_temp->ilm_ix;
      *index_type = TYPE_TEMP;
    }

  else
    {
      *inseg_ix = 0;
      *index_type = TYPE_NONE;
    }
}

void
nsm_gmpls_ilm_entry_key_set (struct ilm_entry *ilm, u_int32_t iif_ix,
                             struct gmpls_gen_label *label)
{
  switch (label->type)
    {
      case gmpls_entry_type_ip:
        ilm->key.u.pkt.iif_ix = iif_ix;
        ilm->key.u.pkt.in_label = label->u.pkt;
        ilm->ent_type = ILM_ENTRY_TYPE_PACKET;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        ilm->key.u.tdm_key.iif_ix = iif_ix;
        ilm->key.u.tdm_key.in_label = label->u.tdm;
        ilm->ent_type = ILM_ENTRY_TYPE_TDM;
        break;
#endif /* HAVE_TDM */

#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        ilm->key.u.pbb_key.iif_ix = iif_ix;
        ilm->key.u.pbb_key.in_label = label->u.pbb;
        ilm->ent_type = ILM_ENTRY_TYPE_PBB_TE;
        break;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

      default:
        break;

    }
}

/* Create new Active ILM entry */
u_int32_t
nsm_mpls_create_new_ilm_entry (struct nsm_master *nm,
                               struct ilm_temp_ptr **ilm_temp_entry,
                               struct ilm_entry **ilm_entry)
{
  s_int32_t ret = NSM_FAILURE;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct ilm_entry *ilm = NULL;

  ilm_temp = *ilm_temp_entry;

  ilm = XCALLOC (MTYPE_MPLS_ILM_ENTRY, sizeof (struct ilm_entry));
  if (! ilm)
    return NSM_FAILURE; /* nsm_err_mem_alloc_failure */

  /* set in segment index */
  ilm->ilm_ix = ilm_temp->ilm_ix;
  nsm_gmpls_ilm_entry_key_set (ilm, ilm_temp->iif_ix, &ilm_temp->in_label);
  ilm->n_pops = 1;
  ilm->xc = NULL;
  ilm->owner.owner = ilm_temp->owner;
  ilm->family = ilm_temp->family;
  ilm->prefixlen = ilm_temp->prefixlen;
  pal_mem_cpy (ilm->prfx, ilm_temp->prfx, sizeof (struct pal_in4_addr));

  /* Entry is assumed to be non-primary */
  ret = gmpls_ilm_create (nm, ilm);
  if (ret != NSM_SUCCESS)
    {
      gmpls_ilm_free (ilm);
      return ret;
    }

  ilm->row_status = RS_ACTIVE;
  *ilm_entry = ilm;
  return NSM_SUCCESS;
}

/* Create Backup (Non-Active) ILM entry */
u_int32_t
nsm_mpls_create_new_ilm_temp (struct nsm_master *nm, struct ilm_entry *ilm,
                              u_int32_t inseg_ix)
{
  struct ilm_temp_ptr *ilm_temp = NULL;

  ilm_temp = XCALLOC (MTYPE_MPLS_ILM_ENTRY, sizeof (struct ilm_temp_ptr));
  if (! ilm_temp)
    return NSM_FAILURE;

  ilm_temp->ilm_ix = inseg_ix;

  if (ilm)
    {
      switch (ilm->ent_type)
        {
          case ILM_ENTRY_TYPE_PACKET:
            ilm_temp->iif_ix = ilm->key.u.pkt.iif_ix;
            ilm_temp->in_label.type = gmpls_entry_type_ip;
            ilm_temp->in_label.u.pkt = ilm->key.u.pkt.in_label;
            break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case ILM_ENTRY_TYPE_PBB_TE:
            ilm_temp->iif_ix = ilm->key.u.pbb_key.iif_ix;
            ilm_temp->in_label.type = gmpls_entry_type_pbb_te;
            ilm_temp->in_label.u.pbb = ilm->key.u.pbb_key.in_label;

            break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
          case ILM_ENTRY_TYPE_TDM:
            ilm_temp->iif_ix = ilm->key.u.tdm_key.iif_ix;
            ilm_temp->in_label.type = gmpls_entry_type_tdm;
            ilm_temp->in_label.u.tdm = ilm->key.u.tdm_key.in_label;

            break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

          default:
            break;
        }

      ilm_temp->owner = ilm->owner.owner;
      ilm_temp->family = ilm->family;
      ilm_temp->prefixlen = ilm->prefixlen;
      pal_mem_cpy (ilm_temp->prfx, ilm->prfx, sizeof (struct pal_in4_addr));
      ilm_temp->row_status = RS_NOT_IN_SERVICE;

      SET_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_LABEL_SET);
      SET_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_INTERFACE_SET);
    }
  else  /* Entry to be newly created */
    {
      /* Inform bitmap of new index */
      bitmap_add_index (ILM_IX_MGR, inseg_ix);
      ilm_temp->row_status = RS_NOT_READY;
      ilm_temp->owner = MPLS_SNMP;
    }

  listnode_add (ilm_entry_temp_list, ilm_temp);
  return NSM_SUCCESS;
}

struct xc_entry *
nsm_gmpls_create_new_xc_entry (struct nsm_master *nm,
                              u_int32_t xc_ix, u_int32_t inseg_ix,
                              u_int32_t outseg_ix, u_int32_t iif_ix,
                              struct gmpls_gen_label *in_label,
                              struct nhlfe_entry *nhlfe)
{
  struct xc_entry *xc = NULL;
  s_int32_t ret = NSM_FAILURE;

  xc = XCALLOC (MTYPE_MPLS_XC_ENTRY, sizeof (struct xc_entry));
  if (! xc)
    return NULL; /* NSM_ERR_MEM_ALLOC_FAILURE */

  /* Set Indices of XC Table */
  xc->key.xc_ix = xc_ix;                /* XC Index */
  bitmap_add_index (XC_IX_MGR, xc_ix);

  xc->key.nhlfe_ix = outseg_ix;         /* OutSeg Index */
  xc->ilm_ix = inseg_ix;                /* InSeg Index */

  xc->key.iif_ix = iif_ix;              /* Fill InSeg Interface Idx */
  xc->key.in_label = *in_label;  /* and Label */

  xc->owner = MPLS_SNMP;
  xc->admn_status = ADMN_UP;
  xc->type = in_label->type;

  if (nhlfe)
    {
      /* This line is added bcos xc_ix in nhlfe overwrites xc_ix */
      /* in xc_entry during linking */
      nhlfe->xc_ix = xc_ix;
      gmpls_xc_nhlfe_link (xc, nhlfe);
    }

  ret = gmpls_xc_add (nm, xc);
  if (ret != NSM_SUCCESS)
    {
      gmpls_xc_row_cleanup (nm, xc, NSM_FALSE, NSM_FALSE);
      xc->key.nhlfe_ix = 0;
      gmpls_xc_free (xc);
      return NULL;
    }
  xc->row_status = RS_NOT_IN_SERVICE;
  xc->opr_status = OPR_DOWN;
  return xc;
}

/* Check if Interface-Label pair is unique */
/* If found to be duplicate entry, return error */
s_int32_t
nsm_mpls_check_duplicate_ilm_entry (struct nsm_master *nm,
                                    struct ilm_temp_ptr *ilm_temp)
{
  struct xc_entry *xc = NULL;
  struct nhlfe_entry *nhlfe = NULL;
  u_int32_t outseg_ix;
  u_int32_t xc_ix;
  u_int32_t row_status;
  u_int32_t admn_status;

  if (CHECK_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_LABEL_SET) &&
      CHECK_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_INTERFACE_SET))
    {
      if (nsm_gmpls_ilm_lookup (nm, ilm_temp->iif_ix, &ilm_temp->in_label, 0,
                                NSM_FALSE, NSM_FALSE) ||
          nsm_gmpls_ilm_temp_lookup (ilm_temp->ilm_ix, ilm_temp->iif_ix,
                                    &ilm_temp->in_label, NON_IDX_LOOKUP))
        return NSM_FAILURE;

      xc = nsm_gmpls_ilm_key_xc_lookup (nm, 0, 0, ilm_temp->ilm_ix);
      if (xc)
        {
          nhlfe = xc->nhlfe;
          outseg_ix = xc->key.nhlfe_ix; xc_ix = xc->key.xc_ix;
          row_status = xc->row_status; admn_status = xc->admn_status;
          
          xc = nsm_gmpls_xc_lookup (nm, xc->key.xc_ix, xc->key.iif_ix,
                                    &xc->key.in_label, xc->key.nhlfe_ix,
                                    NSM_TRUE);
          if (xc)
            {
              bitmap_release_index (XC_IX_MGR, xc->key.xc_ix);
              xc->key.xc_ix = 0;
              gmpls_xc_free (xc);
            }
          xc = nsm_gmpls_create_new_xc_entry (nm, xc_ix, ilm_temp->ilm_ix,
                                              outseg_ix, ilm_temp->iif_ix,
                                              &ilm_temp->in_label, nhlfe);
          xc->row_status = row_status;
        }

      /* Both Interface and Label present */
      /* Change rowStatus to Not in Service */
      ilm_temp->row_status = RS_NOT_IN_SERVICE;
    }
  return NSM_SUCCESS;
}

/* APIs related to In Segment Table set */
u_int32_t
nsm_mpls_set_inseg_if (struct nsm_master *nm,
                       u_int32_t inseg_ix, u_int32_t inseg_if)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t prev_iif_ix;
  struct apn_vr *vr = nm->vr;
  struct interface *ifp;

  /* Check for validity of value */
  ifp = if_lookup_by_index (&vr->ifm, inseg_if);
  if (nsm_mpls_check_valid_interface (ifp, SWAP) != NSM_TRUE)
    return NSM_API_SET_ERROR;

  /* Lookup - Both main and temp. DS to get corresponding entry */
  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm_temp && !ilm)  /* Non Active entry found */
    {
      prev_iif_ix = ilm_temp->iif_ix;

      ilm_temp->iif_ix = inseg_if;
      SET_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_INTERFACE_SET);

      /* Check if Interface-Label pair is unique. If duplicate, error */
      if (nsm_mpls_check_duplicate_ilm_entry (nm, ilm_temp) == NSM_SUCCESS)
        return NSM_API_SET_SUCCESS;
      else
        {
          ilm_temp->iif_ix = prev_iif_ix;
          UNSET_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_INTERFACE_SET);
        }
    }
  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_inseg_lb (struct nsm_master *nm,
                       u_int32_t inseg_ix, u_int32_t in_label)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct gmpls_gen_label prev_label;

  if (in_label > LABEL_VALUE_MAX || in_label < LABEL_VALUE_INITIAL)
    return NSM_API_SET_ERROR;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm_temp && !ilm)  /* Non Active entry found */
    {
      prev_label = ilm_temp->in_label;

      ilm_temp->in_label.u.pkt = in_label;
      ilm_temp->in_label.type = gmpls_entry_type_ip;

      SET_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_LABEL_SET);

      /* Check if Interface-Label pair is unique. If duplicate, error */
      if (nsm_mpls_check_duplicate_ilm_entry (nm, ilm_temp) == NSM_SUCCESS)
        return NSM_API_SET_SUCCESS;
      else
        {
          ilm_temp->in_label = prev_label;
          UNSET_FLAG (ilm_temp->flags, ILM_ENTRY_FLAG_LABEL_SET);
        }
    }
  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_inseg_lb_ptr (struct nsm_master *nm, u_int32_t inseg_ix)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm_temp && !ilm)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_inseg_npop (struct nsm_master *nm,
                         u_int32_t inseg_ix, u_int32_t npops)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  if (npops != 1)
    return NSM_API_SET_ERROR;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm_temp && !ilm)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_inseg_addr_family (struct nsm_master *nm,
                                u_int32_t inseg_ix, u_int32_t addr_family)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  if (addr_family != INET_AD_IPV4)
    return NSM_API_SET_ERROR;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm_temp && !ilm)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_inseg_tf_prm (struct nsm_master *nm, u_int32_t inseg_ix)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm_temp && !ilm)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

void
nsm_gmpls_get_info_from_ilm_entry (struct ilm_entry *ilm, u_int32_t *iif_ix,
                                   struct gmpls_gen_label *lbl)
{
  switch (ilm->ent_type)
    {
      case ILM_ENTRY_TYPE_PACKET:
        *iif_ix = ilm->key.u.pkt.iif_ix;
        lbl->type = gmpls_entry_type_ip;
        lbl->u.pkt = ilm->key.u.pkt.in_label;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case ILM_ENTRY_TYPE_PBB_TE:
        *iif_ix = ilm->key.u.pbb_key.iif_ix;
        lbl->type = gmpls_entry_type_pbb_te;
        lbl->u.pbb = ilm->key.u.pbb_key.in_label;
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case ILM_ENTRY_TYPE_TDM:
        *iif_ix = ilm->key.u.tdm_key.iif_ix;
        lbl->type = gmpls_entry_type_tdm;
        lbl->u.pbb = ilm->key.u.tdm_key.in_label;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }
}

u_int32_t
nsm_mpls_set_inseg_row_status (struct nsm_master *nm,
                               u_int32_t inseg_ix, u_int32_t row_status)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct gmpls_gen_label lbl;
  struct xc_entry *xc = NULL;
  s_int32_t ret = NSM_FAILURE;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  switch (row_status)
    {
      case RS_ACTIVE:
        if (ilm)  /* Already Active */
          return NSM_API_SET_SUCCESS;
        if (ilm_temp && ilm_temp->row_status == RS_NOT_IN_SERVICE)
          {
            ret = nsm_mpls_create_new_ilm_entry (nm, &ilm_temp, &ilm);
            if (ret != NSM_SUCCESS)  /* Creation of new entry fails */
              return NSM_API_SET_ERROR;

            /* Lookup for XC with this InSeg Interface and Label */
            xc = nsm_gmpls_ilm_key_xc_lookup (nm, ilm_temp->iif_ix,
                                             &ilm_temp->in_label, 0);
            
            /* Add to Fwding table */
            if (xc && xc->nhlfe && xc->row_status == RS_ACTIVE &&
                xc->nhlfe->row_status == RS_ACTIVE && 
                xc->admn_status == ADMN_UP)
              {
                gmpls_ilm_xc_link (ilm, xc);  /* Link XC and ILM entries */
                SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
                gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
              }

            listnode_delete (ilm_entry_temp_list, (void *)ilm_temp);
            XFREE (MTYPE_MPLS_ILM_ENTRY, ilm_temp);

            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_IN_SERVICE:
        /* Already Not in Service */
        if (ilm_temp && ilm_temp->row_status == RS_NOT_IN_SERVICE)
          return NSM_API_SET_SUCCESS;
        if (ilm)  /* Current state - Active */
          {
            ret = nsm_mpls_create_new_ilm_temp (nm, ilm, inseg_ix);
            if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
              return NSM_API_SET_ERROR;

            /* Lookup and delete */
            pal_mem_set (&lbl, '\0', sizeof (struct gmpls_gen_label));
            
            lbl.u.pkt = ilm->key.u.pkt.in_label;
            nsm_gmpls_ilm_lookup (nm, ilm->key.u.pkt.iif_ix, &lbl,
                                 inseg_ix, NSM_TRUE, NSM_FALSE);
            /* Delete from Forwarder and Unlink XC entry */
            /* Do not delete XC or NHLFE entries */
            gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);

            
            /* Free memory */
            gmpls_ilm_free (ilm);

            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_READY:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_GO:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_WAIT:
        if (ilm || ilm_temp)  /* Entry already present. Cant recreate */
          return NSM_API_SET_ERROR;

        ret = nsm_mpls_create_new_ilm_temp (nm, ilm, inseg_ix);
        if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
          return NSM_API_SET_ERROR;

        return NSM_API_SET_SUCCESS;
        break;

      case RS_DESTROY:
        if (ilm)
          {
            /* Lookup and delete */
            nsm_gmpls_ilm_lookup (nm, ilm_temp->iif_ix, &ilm_temp->in_label,
                                 inseg_ix, NSM_TRUE, NSM_FALSE);
            /* Delete from Forwarder and Unlink XC entry */
            /* Do not delete XC or NHLFE entries */
            gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);
            /* Free memory */
            gmpls_ilm_free (ilm);
          }
        else if (ilm_temp)
          {
            /* Delete node from temp. list and free memory */
            listnode_delete (ilm_entry_temp_list, (void *)ilm_temp);
            XFREE (MTYPE_MPLS_ILM_ENTRY, ilm_temp);
          }
        else
          return NSM_API_SET_ERROR;
        /* Release Index */
        bitmap_release_index (ILM_IX_MGR, inseg_ix);
        return NSM_API_SET_SUCCESS;
        break;

      default:
        return NSM_API_SET_ERROR;
        break;
    }
}

u_int32_t
nsm_mpls_set_inseg_st_type (struct nsm_master *nm,
                            u_int32_t inseg_ix, u_int32_t st_type)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  if (st_type != ST_VOLATILE)
    return NSM_API_SET_ERROR;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm || ilm_temp) /* If entry present */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

/* APIs related with MPLS In-segment table GET */
u_int32_t
nsm_gmpls_get_inseg_if (struct nsm_master *nm,
                       u_int32_t inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct gmpls_gen_label lbl;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    {
      nsm_gmpls_get_info_from_ilm_entry (ilm, ret, &lbl);
    }
  else if (ilm_temp)
    {
      *ret = ilm_temp->iif_ix;
    }
  else
    {
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_if (struct nsm_master *nm,
                            u_int32_t *inseg_ix,
                            u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct gmpls_gen_label lbl;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN && ilm)  /* If next entry is present in main entry */
    {
      nsm_gmpls_get_info_from_ilm_entry (ilm, ret, &lbl);
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    {
      *ret = ilm_temp->iif_ix;
    }
  else
    {
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_lb (struct nsm_master *nm,
                       u_int32_t inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct gmpls_gen_label lbl;
  u_int32_t iif_ix;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    {
      nsm_gmpls_get_info_from_ilm_entry (ilm, &iif_ix, &lbl);
      *ret = lbl.u.pkt;
    }
  else if (ilm_temp)
    {
      *ret = ilm_temp->in_label.u.pkt;
    }
  else
    {
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_lb (struct nsm_master *nm,
                            u_int32_t *inseg_ix,
                            u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;
  struct gmpls_gen_label lbl;
  u_int32_t iif_ix;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    {
       nsm_gmpls_get_info_from_ilm_entry (ilm, &iif_ix, &lbl);
      *ret = lbl.u.pkt;
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    {
      *ret = ilm_temp->in_label.u.pkt;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_lb_ptr (struct nsm_master *nm,
                           u_int32_t inseg_ix, char **ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm || ilm_temp)
    {
      pal_snprintf (*ret,
                    NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsLbPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_inseg_lb_ptr (struct nsm_master *nm,
                                u_int32_t *inseg_ix,
                                char **ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      pal_snprintf (*ret, NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsLbPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_inseg_npop (struct nsm_master *nm,
                         u_int32_t inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm || ilm_temp)
    {
      *ret = 1;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_inseg_npop (struct nsm_master *nm,
                              u_int32_t *inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = ilm->n_pops;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = 1;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_addr_family (struct nsm_master *nm, u_int32_t inseg_ix,
                                u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm || ilm_temp)
    {
      *ret = INET_AD_IPV4;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_inseg_addr_family (struct nsm_master *nm,
                                     u_int32_t *inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = INET_AD_IPV4;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_inseg_xc_ix (struct nsm_master *nm, u_int32_t inseg_ix,
                          u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    {
      if (ilm->xc)
        *ret = ilm->xc->key.xc_ix;
      else
        *ret = 0;
    }
  else if (ilm_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_xc_ix (struct nsm_master *nm,
                               u_int32_t *inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    {
      if (ilm->xc)
        *ret = ilm->xc->key.xc_ix;
      else
        *ret = 0;
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_owner (struct nsm_master *nm, u_int32_t inseg_ix,
                          u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    *ret = ilm->owner.owner;
  else if (ilm_temp)
    *ret = ilm_temp->owner;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_owner (struct nsm_master *nm,
                               u_int32_t *inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = ilm->owner.owner;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = ilm_temp->owner;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_tf_prm (struct nsm_master *nm, u_int32_t inseg_ix,
                           char **ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm || ilm_temp)
    {
      pal_snprintf (*ret,
                    NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsTnRscPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_inseg_tf_prm (struct nsm_master *nm,
                                u_int32_t *inseg_ix,
                                char **ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      pal_snprintf (*ret, NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsTnRscPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_inseg_row_status (struct nsm_master *nm, u_int32_t inseg_ix,
                               u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    *ret = ilm->row_status;
  else if (ilm_temp)
    *ret = ilm_temp->row_status;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_row_status (struct nsm_master *nm,
                                    u_int32_t *inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = ilm->row_status;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = ilm_temp->row_status;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_st_type (struct nsm_master *nm, u_int32_t inseg_ix,
                            u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm || ilm_temp)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_inseg_st_type (struct nsm_master *nm,
                                 u_int32_t *inseg_ix, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

/* APIs related with MPLS In-segment Map table. */
u_int32_t
nsm_gmpls_get_inseg_map_ix (struct nsm_master *nm, u_int32_t inseg_if_ix,
                           u_int32_t inseg_label, u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct gmpls_gen_label lbl;

  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = inseg_label;

  ilm = nsm_gmpls_ilm_lookup (nm, inseg_if_ix, &lbl, 0, NSM_FALSE, NSM_FALSE);
  if (ilm)
    {
      *ret = ilm->ilm_ix;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_inseg_map_ix (struct nsm_master *nm,
                                u_int32_t *inseg_if_ix,
                                u_int32_t *inseg_label,
                                u_int32_t index_len,
                                u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct gmpls_gen_label lbl;

  ilm = nsm_mpls_ilm_lookup_next (nm, *inseg_if_ix,
                                  *inseg_label, index_len);

  if (ilm)
    {
      nsm_gmpls_get_info_from_ilm_entry (ilm, inseg_if_ix, &lbl);
      *inseg_label = lbl.u.pkt;
      *ret = ilm->ilm_ix;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

/* APIs related with MPLS In-segment performance table. */
u_int32_t
nsm_gmpls_get_inseg_octs (struct nsm_master *nm, u_int32_t inseg_ix,
                         u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    *ret =  nsm_cnt_in_seg_octs (ilm);
  else if (ilm_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_octs (struct nsm_master *nm,
                              u_int32_t *inseg_ix,
                              u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)
    *ret = nsm_cnt_in_seg_octs (ilm);
  else if (type == TYPE_TEMP)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_pkts (struct nsm_master *nm, u_int32_t inseg_ix,
                         u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    *ret =  nsm_cnt_in_seg_pkts (ilm);
  else if (ilm_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_pkts (struct nsm_master *nm,
                              u_int32_t *inseg_ix,
                              u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)
    *ret = nsm_cnt_in_seg_pkts (ilm);
  else if (type == TYPE_TEMP)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_errors (struct nsm_master *nm, u_int32_t inseg_ix,
                           u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    *ret =  nsm_cnt_in_seg_errors (ilm);
  else if (ilm_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_errors (struct nsm_master *nm, u_int32_t *inseg_ix,
                                u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)
    *ret = nsm_cnt_in_seg_errors (ilm);
  else if (type == TYPE_TEMP)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_discards (struct nsm_master *nm, u_int32_t inseg_ix,
                             u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    *ret =  nsm_cnt_in_seg_discards (ilm);
  else if (ilm_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_discards (struct nsm_master *nm, u_int32_t *inseg_ix,
                                  u_int32_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)
    *ret = nsm_cnt_in_seg_discards (ilm);
  else if (type == TYPE_TEMP)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_inseg_hc_octs (struct nsm_master *nm, u_int32_t inseg_ix,
                            ut_int64_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm || ilm_temp)
    {
      *ret = nsm_cnt_in_seg_hc_octs (ilm);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_inseg_hc_octs (struct nsm_master *nm, u_int32_t *inseg_ix,
                                 ut_int64_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = nsm_cnt_in_seg_hc_octs (ilm);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_inseg_perf_dis_time (struct nsm_master *nm, u_int32_t inseg_ix,
                                  time_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;

  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    *ret =  nsm_cnt_in_seg_perf_dis_time (ilm);
  else if (ilm_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_inseg_perf_dis_time (struct nsm_master *nm,
                                       u_int32_t *inseg_ix,
                                       time_t *ret)
{
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t type;

  nsm_gmpls_inseg_lookup_next (nm, *inseg_ix, &ilm_temp, &ilm);

  /* Set Index value */
  nsm_gmpls_get_next_inseg_index_set (ilm, ilm_temp, inseg_ix, &type);

  if (type == TYPE_MAIN)
    *ret = nsm_cnt_in_seg_perf_dis_time (ilm);
  else if (type == TYPE_TEMP)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

/* Support APIs for OutSegmentTable Set */
/* NHLFE Lookup - Both temporary and main Data structures */
void
nsm_gmpls_outseg_lookup (struct nsm_master *nm, u_int32_t outseg_ix,
                        struct nhlfe_temp_ptr **nhlfe_temp,
                        struct nhlfe_entry **nhlfe)
{
  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  *nhlfe = nsm_mpls_nhlfe_ix_lookup (nm, outseg_ix);
  *nhlfe_temp = nsm_gmpls_nhlfe_temp_lookup (outseg_ix, 0, 0, &src_addr,
                                             IDX_LOOKUP);
}

/* NHLFE Lookup Next - Both temporary and main Data structures */
void
nsm_gmpls_outseg_lookup_next (struct nsm_master *nm, u_int32_t outseg_ix,
                             struct nhlfe_temp_ptr **nhlfe_temp,
                             struct nhlfe_entry **nhlfe)
{
  *nhlfe = nsm_mpls_nhlfe_ix_lookup_next (nm, outseg_ix);
  *nhlfe_temp = nsm_gmpls_nhlfe_temp_lookup_next (outseg_ix);
}

/* Set Index for Get Next */
void
nsm_gmpls_get_next_outseg_index_set (struct nhlfe_entry *nhlfe,
                                    struct nhlfe_temp_ptr *nhlfe_temp,
                                    u_int32_t *outseg_ix, u_int32_t *index_type)
{
  if ((nhlfe && !nhlfe_temp) ||
      (nhlfe && nhlfe_temp && nhlfe->nhlfe_ix < nhlfe_temp->nhlfe_ix))
    {
      *outseg_ix = nhlfe->nhlfe_ix;
      *index_type = TYPE_MAIN;
    }

  else if ((!nhlfe && nhlfe_temp) ||
           (nhlfe && nhlfe_temp && nhlfe->nhlfe_ix > nhlfe_temp->nhlfe_ix))
    {
      *outseg_ix = nhlfe_temp->nhlfe_ix;
      *index_type = TYPE_TEMP;
    }

  else
    {
      *outseg_ix = 0;
      *index_type = TYPE_NONE;
    }
}

/* Create new Active NHLFE entry */
u_int32_t
nsm_mpls_create_new_nhlfe_entry (struct nsm_master *nm,
                                 struct nhlfe_temp_ptr **nhlfe_temp_entry,
                                 struct nhlfe_entry **nhlfe_entry)
{
  s_int32_t ret = NSM_FAILURE;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct nhlfe_entry *nhlfe = NULL;
  u_int32_t size = 0;

  nhlfe_temp = *nhlfe_temp_entry;

  switch (nhlfe_temp->out_label.type)
    {
      case gmpls_entry_type_ip:
        if (nhlfe_temp->nh_addr.family == AF_INET)
          {
            size = sizeof (struct nhlfe_key_ipv4) +  sizeof (afi_t);
          }
#ifdef HAVE_IPV6
        else if (nhlfe_temp->nh_addr.family == AF_INET6)
          {
            size = sizeof (struct nhlfe_key_ipv6) +  sizeof (afi_t);
          }
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
        else if (nhlfe_temp->nh_addr.family == AF_UNNUMBERED)
          {
            size = sizeof (struct nhlfe_key_unnum) +  sizeof (afi_t);
          }
#endif /* HAVE_GMPLS */
        else
          {
            size = 0;
          }
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        size = sizeof (struct nhlfe_pbb_key);
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        size = sizeof (struct nhlfe_tdm_key);
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }

  nhlfe = XCALLOC (MTYPE_MPLS_NHLFE_ENTRY, sizeof (struct nhlfe_entry) +
                   size);
  if (! nhlfe)
    return NSM_FAILURE; /* nsm_err_mem_alloc_failure */

  nhlfe->type = nhlfe_temp->out_label.type;

  /* set out segment index */
  nhlfe->nhlfe_ix = nhlfe_temp->nhlfe_ix;
  switch (nhlfe->type)
    {
      case gmpls_entry_type_ip:
        {
          struct nhlfe_key *key;

          key = (struct nhlfe_key *) nhlfe->nkey;
          if (nhlfe_temp->nh_addr.family == AF_INET)
            {
              key->afi = AFI_IP;
              key->u.ipv4.nh_addr = nhlfe_temp->nh_addr.u.prefix4;
              key->u.ipv4.oif_ix = nhlfe_temp->oif_ix;
              key->u.ipv4.out_label = nhlfe_temp->out_label.u.pkt;
            }
#ifdef HAVE_IPV6
          else if (nhlfe_temp->nh_addr.family == AF_INET6)
            {
              key->afi = AFI_IP6;
              key->u.ipv6.oif_ix = nhlfe_temp->oif_ix;
              key->u.ipv6.out_label = nhlfe_temp->out_label.u.pkt;
              key->u.ipv6.nh_addr = nhlfe_temp->nh_addr.u.prefix6;
            }
#endif  /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
          else if (nhlfe_temp->nh_addr.family == AF_UNNUMBERED)
            {
              key->afi = AFI_UNNUMBERED;
              key->u.unnum.oif_ix = nhlfe_temp->oif_ix;
              key->u.unnum.out_label = nhlfe_temp->out_label.u.pkt;
              key->u.unnum.id = nhlfe_temp->nh_addr.u.prefix4.s_addr;
            }
#endif /* HAVE_GMPLS */
        }

        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct nhlfe_pbb_key *key;

          key = (struct nhlfe_pbb_key *) nhlfe->nkey;
          key->oif_ix = nhlfe_temp->oif_ix;
          key->lbl = nhlfe_temp->out_label.u.pbb;
        }
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct nhlfe_tdm_key *key;

          key = (struct nhlfe_tdm_key *) nhlfe->nkey;
          key->oif_ix = nhlfe_temp->oif_ix;
          key->lbl = nhlfe_temp->out_label.u.tdm;
        }

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;

    }

  nhlfe->xc_ix = 0;
  nhlfe->owner = nhlfe_temp->owner;
  nhlfe->opcode = nhlfe_temp->opcode;

  ret = gmpls_nhlfe_add (nm, nhlfe);
  if (ret != NSM_SUCCESS)
    {
      gmpls_nhlfe_free (nhlfe);
      return NSM_API_SET_ERROR;
    }

  nhlfe->row_status = RS_ACTIVE;
  *nhlfe_entry = nhlfe;
  return NSM_SUCCESS;
}

void
nsm_mpls_set_nhlfe_temp_from_nhlfe (struct nhlfe_temp_ptr *tmp,
                                    struct nhlfe_entry *ent)
{
  switch (ent->type)
    {
      case gmpls_entry_type_ip:
        {
          struct nhlfe_key *key;

          /* Cant copy like other cases as the memory has been allocated
             according to afi */
          key = (struct nhlfe_key *) ent->nkey;
          if (key->afi == AFI_IP)
            {
              tmp->out_label.type = gmpls_entry_type_ip;
              tmp->out_label.u.pkt = key->u.ipv4.out_label;

              tmp->nh_addr.family = AF_INET;
              tmp->nh_addr.u.prefix4 = key->u.ipv4.nh_addr;

              tmp->oif_ix = key->u.ipv4.oif_ix;
            }
#ifdef HAVE_IPV6
          else if (key->afi == AFI_IP6)
            {
              tmp->out_label.type = gmpls_entry_type_ip;
              tmp->out_label.u.pkt = key->u.ipv6.out_label;

              tmp->nh_addr.family = AF_INET6;
              tmp->nh_addr.u.prefix6 = key->u.ipv6.nh_addr;

              tmp->oif_ix = key->u.ipv6.oif_ix;
            }
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
          else if (key->afi == AFI_UNNUMBERED)
            {
              tmp->out_label.type = gmpls_entry_type_ip;
              tmp->out_label.u.pkt = key->u.unnum.out_label;

              tmp->nh_addr.family = AF_UNNUMBERED;
              tmp->nh_addr.u.prefix4.s_addr = key->u.unnum.id;

              tmp->oif_ix = key->u.unnum.oif_ix;
            }

#endif /* HAVE_GMPLS */
        }
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct nhlfe_pbb_key *key;

          key = (struct nhlfe_pbb_key *) ent->nkey;
          tmp->out_label.type = gmpls_entry_type_ip;
          tmp->out_label.u.pbb = key->lbl;
        }

        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct nhlfe_tdm_key *key;

          key = (struct nhlfe_tdm_key *) ent->nkey;
          tmp->key.u.tdm = key;
        }

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return;
}

/* Create Backup (Non-Active) NHLFE entry */
u_int32_t
nsm_mpls_create_new_nhlfe_temp (struct nsm_master *nm,
                                struct nhlfe_entry *nhlfe,
                                u_int32_t outseg_ix)
{
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nhlfe_temp = XCALLOC (MTYPE_MPLS_NHLFE_ENTRY, sizeof (struct nhlfe_temp_ptr));
  if (! nhlfe_temp)
    return NSM_FAILURE;

  nhlfe_temp->nhlfe_ix = outseg_ix;

  if (nhlfe)
    {
      nsm_mpls_set_nhlfe_temp_from_nhlfe (nhlfe_temp, nhlfe);

      nhlfe_temp->owner = nhlfe->owner;
      nhlfe_temp->opcode = nhlfe->opcode;
      nhlfe_temp->row_status = RS_NOT_IN_SERVICE;
      nhlfe_temp->refcount = nhlfe->refcount;

      SET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_LABEL_SET);
      SET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_INTERFACE_SET);
      SET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_NXT_HOP_SET);
    }
  else  /* Entry to be newly created */
    {
      /* Inform bitmap of new index */
      bitmap_add_index (NHLFE_IX_MGR, outseg_ix);
      nhlfe_temp->row_status = RS_NOT_READY;
      nhlfe_temp->owner = MPLS_SNMP;
      nhlfe_temp->opcode = PUSH;
      nhlfe_temp->refcount = 0;
    }

  listnode_add (nhlfe_entry_temp_list, nhlfe_temp);
  return NSM_SUCCESS;
}

s_int32_t
nsm_gmpls_ftn_select (struct nsm_master *nm, struct ftn_entry *ftn,
                      struct fec_gen_entry *fec)
{
  struct ptree_node *pn;
  struct ptree_ix_table *pix_tbl;
  s_int32_t ret = -1;

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, fec);
  if (!pix_tbl)
    return -1;
  pn = ptree_node_get (pix_tbl->m_table, (u_char *) &fec,
                       sizeof (struct fec_gen_entry));

  SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED);
  if (pn)
    ret = gmpls_ftn_entry_select_process (nm, ftn, pn);
  FTN_XC_STATUS_UPDATE (ftn);
  return ret;
}

void
nsm_gmpls_ftn_deselect (struct nsm_master *nm, struct ftn_entry *ftn,
                        struct fec_gen_entry *fec)
{
  struct ptree_node *pn;
  struct ptree_ix_table *pix_tbl;

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, fec);
  if (!pix_tbl)
    return;
  pn = ptree_node_get (pix_tbl->m_table, (u_char *) fec,
                       sizeof (struct fec_gen_entry));

  if (pn)
    gmpls_ftn_entry_deselect (nm, ftn, pn, NSM_TRUE);
  return;
}

/* Check if Interface-Label-NHAddr is unique */
/* If found to be duplicate, return error */
s_int32_t
nsm_mpls_check_duplicate_nhlfe_entry (struct nsm_master *nm,
                                      struct nhlfe_temp_ptr *nhlfe_temp)
{
   if (CHECK_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_NXT_HOP_SET) &&
       CHECK_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_LABEL_SET) &&
       CHECK_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_INTERFACE_SET))
     {
       if (nsm_gmpls_nhlfe_lookup (nm,
                                   (struct addr_in *)&nhlfe_temp->nh_addr,
                                    nhlfe_temp->oif_ix, &nhlfe_temp->out_label,
                                    NSM_FALSE))
         return NSM_FAILURE;

       if (nsm_gmpls_nhlfe_temp_lookup (nhlfe_temp->nhlfe_ix,
                                       nhlfe_temp->oif_ix,
                                       &nhlfe_temp->out_label,
                                       &nhlfe_temp->nh_addr, NON_IDX_LOOKUP))
         return NSM_FAILURE;

       /* NxtHop Addr, Interface and Label present */
       /* Change rowStatus to Not in Service */
       nhlfe_temp->row_status = RS_NOT_IN_SERVICE;
     }

  return NSM_SUCCESS;
}
/* APIs related to Out Segment Table set */
u_int32_t
nsm_mpls_set_outseg_if_ix (struct nsm_master *nm,
                           u_int32_t outseg_ix, u_int32_t outseg_if_ix)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t prev_oif_ix;
  struct apn_vr *vr = nm->vr;
  struct interface *ifp;

  /* Check for validity of value */
  ifp = if_lookup_by_index (&vr->ifm, outseg_if_ix);
  if (nsm_mpls_check_valid_interface (ifp, SWAP) != NSM_TRUE)
    return NSM_API_SET_ERROR;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe_temp && !nhlfe)  /* Non Active entry found */
    {
      prev_oif_ix = nhlfe_temp->oif_ix;

      nhlfe_temp->oif_ix = outseg_if_ix;
      SET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_INTERFACE_SET);

      /* Check if Interface-Label-NHAddr is unique, else error */
      if (nsm_mpls_check_duplicate_nhlfe_entry (nm, nhlfe_temp) == NSM_SUCCESS)
        return NSM_API_SET_SUCCESS;
      else
        {
          nhlfe_temp->oif_ix = prev_oif_ix;
          UNSET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_INTERFACE_SET);
        }
    }
  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_outseg_push_top_lb (struct nsm_master *nm,
                                 u_int32_t outseg_ix, u_int32_t push_top_lb)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  if (push_top_lb != NSM_TRUE)
    return NSM_API_SET_ERROR;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe_temp && !nhlfe)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_outseg_top_lb (struct nsm_master *nm,
                            u_int32_t outseg_ix, u_int32_t out_label)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t prev_label;

  /* Check for validity of value */
  /* Assuming opcode to be PUSH/SWAP */
  if (! VALID_LABEL (out_label))
    return NSM_API_SET_ERROR;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe_temp && !nhlfe)  /* Non Active entry found */
    {
      prev_label = nhlfe_temp->out_label.u.pkt;

      nhlfe_temp->out_label.u.pkt = out_label;
      SET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_LABEL_SET);

      /* Check if Interface-Label-NHAddr is unique, else error */
      if (nsm_mpls_check_duplicate_nhlfe_entry (nm, nhlfe_temp) == NSM_SUCCESS)
        return NSM_API_SET_SUCCESS;
      else
        {
          nhlfe_temp->out_label.u.pkt = prev_label;
          UNSET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_LABEL_SET);
        }
    }
  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_outseg_top_lb_ptr (struct nsm_master *nm, u_int32_t outseg_ix)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe_temp && !nhlfe)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_outseg_nxt_hop_ipa_type (struct nsm_master *nm,
                                      u_int32_t outseg_ix, u_int32_t ipa_type)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  if (ipa_type != INET_AD_IPV4)
    return NSM_API_SET_ERROR;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe_temp && !nhlfe)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_outseg_nxt_hop_ipa (struct nsm_master *nm, u_int32_t outseg_ix,
                                 u_int32_t length, struct pal_in4_addr addr)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct pal_in4_addr prev_nhaddr;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe_temp && !nhlfe)  /* Non Active entry found */
    {
      prev_nhaddr = nhlfe_temp->nh_addr.u.prefix4;

      nhlfe_temp->nh_addr.family = AF_INET;
      nhlfe_temp->nh_addr.u.prefix4 = addr;
      SET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_NXT_HOP_SET);

      /* Check if Interface-Label-NHAddr is unique, else error */
      if (nsm_mpls_check_duplicate_nhlfe_entry (nm, nhlfe_temp) == NSM_SUCCESS)
        return NSM_API_SET_SUCCESS;
      else
        {
          nhlfe_temp->nh_addr.u.prefix4 = prev_nhaddr;
          UNSET_FLAG (nhlfe_temp->flags, NHLFE_ENTRY_FLAG_NXT_HOP_SET);
        }
    }
  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_outseg_tf_prm (struct nsm_master *nm, u_int32_t outseg_ix)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe_temp && !nhlfe)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_outseg_row_status (struct nsm_master *nm,
                                u_int32_t outseg_ix, u_int32_t row_status)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct xc_entry *xc = NULL;
  struct ilm_entry *ilm = NULL;
  struct ftn_entry *ftn = NULL;
  struct avl_node *an = NULL;
  s_int32_t ret = NSM_FAILURE;
  s_int32_t refcount;
  struct fec_gen_entry fec;
  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  switch (row_status)
    {
      case RS_ACTIVE:
        if (nhlfe)  /* Already Active */
          return NSM_API_SET_SUCCESS;
        if (nhlfe_temp && nhlfe_temp->row_status == RS_NOT_IN_SERVICE)
          {
            ret = nsm_mpls_create_new_nhlfe_entry (nm, &nhlfe_temp, &nhlfe);
            if (ret != NSM_SUCCESS)  /* Creation of new entry fails */
              return NSM_API_SET_ERROR;

            /* Lookup for XC with this OutSeg Interface, Label and NHAddr */
            refcount = nhlfe_temp->refcount;

            for (an = avl_top (XC_TABLE); an; an = avl_next (an))
              {
                if (! refcount)
                  break;

                if (an->info != NULL)
                  {
                    xc = (struct xc_entry *) an->info;
                    if (xc->key.nhlfe_ix != outseg_ix)
                      continue;
                  }

                if (!xc || xc->row_status != RS_ACTIVE ||
                    xc->admn_status != ADMN_UP)  /* No XC associated */
                  return NSM_API_SET_SUCCESS; /* Nothing to be done */

                /* Add to Fwding table */
                /* Check if associated InSeg/FTN and XC entries are active */
                ilm = nsm_gmpls_ilm_lookup (nm, xc->key.iif_ix,
                                            &xc->key.in_label,
                                            0, NSM_FALSE, NSM_FALSE);
                if (ilm && ilm->row_status == RS_ACTIVE &&
                    xc->admn_status == ADMN_UP && xc->row_status == RS_ACTIVE)
                  {
                    gmpls_ilm_xc_link (ilm, xc);
                    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
                    gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
                  }
                else
                  {
                    ftn = nsm_mpls_xc_ix_ftn_lookup (nm, xc->key.xc_ix);
                    if (ftn)
                      {
                        fec.type = gmpls_entry_type_ip;

                        ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn->ftn_ix, &fec);
                        if (ftn && ftn->row_status == RS_ACTIVE &&
                            xc->admn_status == ADMN_UP &&
                            xc->row_status == RS_ACTIVE)
                          {
                            /* get tree node */
                            nsm_gmpls_ftn_select (nm, ftn, &fec);
                          }
                      }
                  }
                nhlfe->xc_ix = xc->key.xc_ix;    /* Linking overwrites */
                gmpls_xc_nhlfe_link (xc, nhlfe);  /* Link XC and NHLFE */

                refcount--;
              }

             listnode_delete (nhlfe_entry_temp_list, (void *) nhlfe_temp);
             XFREE (MTYPE_MPLS_NHLFE_ENTRY, nhlfe_temp);

            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_IN_SERVICE:
        if (nhlfe_temp)  /* Already Not in Service */
          return NSM_API_SET_SUCCESS;
        if (nhlfe)  /* Current state - Active */
          {
            ret = nsm_mpls_create_new_nhlfe_temp (nm, nhlfe, outseg_ix);
            if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
              return NSM_API_SET_ERROR;

            refcount = nhlfe->refcount;

            /* Delete from Forwarder and Unlink XC entry */
            /* Do not delete XC or ILM entries */

            for (an = avl_top (XC_TABLE); an; an = avl_next (an))
              {
                if (! refcount)
                  break;

                if (an->info != NULL)
                  {
                    xc = (struct xc_entry *) an->info;
                    if (xc->key.nhlfe_ix != outseg_ix)
                      continue;
                  }

                if (!xc)  /* If no XC is linked */
                  return NSM_API_SET_SUCCESS;

                ilm = nsm_gmpls_ilm_lookup (nm, xc->key.iif_ix,
                                            &xc->key.in_label,
                                            0, NSM_FALSE, NSM_FALSE);
                if (ilm)
                  gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);

                else
                  {
                    ftn = nsm_mpls_xc_ix_ftn_lookup (nm, xc->key.xc_ix);
                    if (ftn)
                      {
                        fec.type = gmpls_entry_type_ip;

                        ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn->ftn_ix, &fec);
                        if (ftn)
                          gmpls_ftn_del_from_fwd (nm, ftn, &fec,
                                                 GLOBAL_FTN_ID, NULL);
                      }
                  }
                gmpls_xc_nhlfe_unlink (xc);
                refcount --;
              }
            if (refcount == 0)
              {
                /* Delete entry */
                gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE);
                /* Free memory */
                gmpls_nhlfe_free (nhlfe);
              }

            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_READY:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_GO:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_WAIT:
        if (nhlfe || nhlfe_temp)  /* Entry already present. Cant recreate */
          return NSM_API_SET_ERROR;

        ret = nsm_mpls_create_new_nhlfe_temp (nm, nhlfe, outseg_ix);
        if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
          return NSM_API_SET_ERROR;

        return NSM_API_SET_SUCCESS;
        break;

      case RS_DESTROY:
        if (nhlfe)
          {
            refcount = nhlfe->refcount;
            for (an = avl_top (XC_TABLE); an; an = avl_next (an))
              {
                if (! refcount)
                  break;

                if (an->info != NULL)
                  {
                    xc = (struct xc_entry *) an->info;
                    if (xc->key.nhlfe_ix != outseg_ix)
                      continue;
                  }

                if (xc)
                  {
                    ilm = nsm_gmpls_ilm_lookup (nm, xc->key.iif_ix,
                                                &xc->key.in_label,
                                                0, NSM_FALSE, NSM_FALSE);
                    if (ilm)
                      gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);
                    else
                      {
                        ftn = nsm_mpls_xc_ix_ftn_lookup (nm, xc->key.xc_ix);
                        if (ftn)
                          {
                            fec.type = gmpls_entry_type_ip;

                            ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn->ftn_ix,
                                                          &fec);
                            if (ftn)
                              {
                                /* mpls_ftn_row_cleanup */
                                gmpls_ftn_del_from_fwd (nm, ftn, &fec,
                                                       GLOBAL_FTN_ID, NULL);
                                gmpls_ftn_xc_unlink (ftn);
                              }
                          }
                      }
                    gmpls_xc_nhlfe_unlink (xc);
                  }
                refcount --;
              }

            if (!nhlfe->refcount)
              {
                gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE);
                gmpls_nhlfe_free (nhlfe);
              }
          }
        else if (nhlfe_temp)
          {
            /* Delete node from temp. list and free memory */
            listnode_delete (nhlfe_entry_temp_list, (void *)nhlfe_temp);
            XFREE (MTYPE_MPLS_NHLFE_ENTRY, nhlfe_temp);
          }
        else
          return NSM_API_SET_ERROR;
        /* Release Index */
        bitmap_release_index (NHLFE_IX_MGR, outseg_ix);
        return NSM_API_SET_SUCCESS;
        break;

      default:
        return NSM_API_SET_ERROR;
        break;
    }
}

u_int32_t
nsm_mpls_set_outseg_st_type (struct nsm_master *nm,
                             u_int32_t outseg_ix, u_int32_t st_type)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  if (st_type != ST_VOLATILE)
    return NSM_API_SET_ERROR;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe || nhlfe_temp)  /* If entry present */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

/* APIs related with MPLS Out-segment table GET */
u_int32_t
nsm_gmpls_get_outseg_if_ix (struct nsm_master *nm,
                           u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    {
      switch (nhlfe->type)
        {
          case gmpls_entry_type_ip:
            {
              struct nhlfe_key *key;

              key = (struct nhlfe_key *) nhlfe->nkey;
              if (key->afi == AFI_IP)
                *ret = key->u.ipv4.oif_ix;
#ifdef HAVE_IPV6
              else if (key->afi == AFI_IP6)
                *ret = key->u.ipv6.oif_ix;
#endif /* HAVE_IPV6. */
#ifdef HAVE_GMPLS
              else if (key->afi == AFI_UNNUMBERED)
                *ret = key->u.unnum.oif_ix;
#endif /* HAVE_GMPLS */
              else
                return NSM_API_GET_ERROR;
            }

#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            {
              struct nhlfe_tdm_key *key;

              key = (struct nhlfe_tdm_key *) nhlfe->nkey;
              *ret = key->oif_ix;
            }
#endif /* HAVE_TDM */
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            {
              struct nhlfe_pbb_key *key;

              key = (struct nhlfe_pbb_key *) nhlfe->nkey;
              *ret = key->oif_ix;
            }
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

          default:
            break;
        }
    }
  else if (nhlfe_temp)
    *ret = nhlfe_temp->oif_ix;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_if_ix (struct nsm_master *nm,
                                u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    {
      switch (nhlfe->type)
        {
          case gmpls_entry_type_ip:
            {
              struct nhlfe_key *key;

              key = (struct nhlfe_key *) nhlfe->nkey;
              if (key->afi == AFI_IP)
                *ret = key->u.ipv4.oif_ix;
#ifdef HAVE_IPV6
              else if (key->afi == AFI_IP6)
                *ret = key->u.ipv6.oif_ix;
#endif /* HAVE_IPV6. */
#ifdef HAVE_GMPLS
              else if (key->afi == AFI_UNNUMBERED)
                *ret = key->u.unnum.oif_ix;
#endif /* HAVE_GMPLS */
              else
                return NSM_API_GET_ERROR;
            }

#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            {
              struct nhlfe_tdm_key *key;

              key = (struct nhlfe_tdm_key *) nhlfe->nkey;
              *ret = key->oif_ix;
            }
#endif /* HAVE_TDM */
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            {
              struct nhlfe_pbb_key *key;

              key = (struct nhlfe_pbb_key *) nhlfe->nkey;
              *ret = key->oif_ix;
            }
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

          default:
            break;
        }
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = nhlfe_temp->oif_ix;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_push_top_lb (struct nsm_master *nm,
                                 u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_char opcode;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    opcode = nhlfe->opcode;
  else if (nhlfe_temp)
    opcode = nhlfe_temp->opcode;
  else
    return NSM_API_GET_ERROR;

  if ((opcode == PUSH) || (opcode == SWAP) ||
      (opcode == PUSH_FOR_VC) || (opcode == PUSH_AND_LOOKUP) ||
      (opcode == PUSH_AND_LOOKUP_FOR_VC))
    *ret = PAL_TRUE;
  else
    *ret = PAL_FALSE;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_push_top_lb (struct nsm_master *nm,
                                      u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;
  u_char opcode;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    opcode = nhlfe->opcode;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    opcode = nhlfe_temp->opcode;
  else
    return NSM_API_GET_ERROR;

  if ((opcode == PUSH) || (opcode == SWAP) ||
      (opcode == PUSH_FOR_VC) || (opcode == PUSH_AND_LOOKUP) ||
      (opcode == PUSH_AND_LOOKUP_FOR_VC))
    *ret = PAL_TRUE;
  else
    *ret = PAL_FALSE;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_top_lb (struct nsm_master *nm,
                            u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    {
      switch (nhlfe->type)
        {
          case gmpls_entry_type_ip:
            {
              struct nhlfe_key *key;

              key = (struct nhlfe_key *) nhlfe->nkey;
              if (key->afi == AFI_IP)
                *ret = key->u.ipv4.out_label;
#ifdef HAVE_IPV6
              else if (key->afi == AFI_IP6)
                *ret = key->u.ipv6.out_label;
#endif /* HAVE_IPV6. */
#ifdef HAVE_GMPLS
              else if (key->afi == AFI_UNNUMBERED)
                *ret = key->u.unnum.out_label;
#endif /* HAVE_GMPLS */
              else
                return NSM_API_GET_ERROR;
            }

#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            {
              struct nhlfe_tdm_key *key;

              key = (struct nhlfe_tdm_key *) nhlfe->nkey;
              *ret = key->out_label;

              /* Cant use this */
              return NSM_API_GET_ERROR;
            }
#endif /* HAVE_TDM */
#if 0
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            {
              struct nhlfe_pbb_key *key;

              key = (struct nhlfe_pbb_key *) nhlfe->nkey;
              *ret = key->out_label;

              /* Cant use this - change later */
              return NSM_API_GET_ERROR;
            }
#endif /* HAVE_PBB_TE */
#endif
#endif /* HAVE_GMPLS */

          default:
            break;
        }
    }
  else if (nhlfe_temp)
    *ret = nhlfe_temp->out_label.u.pkt;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_top_lb (struct nsm_master *nm,
                                 u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    {
      switch (nhlfe->type)
        {
          case gmpls_entry_type_ip:
            {
              struct nhlfe_key *key;

              key = (struct nhlfe_key *) nhlfe->nkey;
              if (key->afi == AFI_IP)
                *ret = key->u.ipv4.out_label;
#ifdef HAVE_IPV6
              else if (key->afi == AFI_IP6)
                *ret = key->u.ipv6.out_label;
#endif /* HAVE_IPV6. */
#ifdef HAVE_GMPLS
              else if (key->afi == AFI_UNNUMBERED)
                *ret = key->u.unnum.out_label;
#endif /* HAVE_GMPLS */
              else
                return NSM_API_GET_ERROR;
            }

#ifdef HAVE_GMPLS
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            {
              struct nhlfe_tdm_key *key;

              key = (struct nhlfe_tdm_key *) nhlfe->nkey;
              *ret = key->out_label;

              /* Cant use this */
              return NSM_API_GET_ERROR;
            }
#endif /* HAVE_TDM */
#if 0
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            {
              struct nhlfe_pbb_key *key;

              key = (struct nhlfe_pbb_key *) nhlfe->nkey;
              *ret = key->out_label;

              /* Cant use this - change later */
              return NSM_API_GET_ERROR;
            }
#endif /* HAVE_PBB_TE */
#endif
#endif /* HAVE_GMPLS */

          default:
            return NSM_API_GET_ERROR;
        }
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = nhlfe_temp->out_label.u.pkt;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_top_lb_ptr (struct nsm_master *nm,
                                u_int32_t outseg_ix, char **ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe || nhlfe_temp)
    {
      pal_snprintf (*ret,
                    NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsOutSegLbPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_outseg_top_lb_ptr (struct nsm_master *nm,
                                     u_int32_t *outseg_ix, char **ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      pal_snprintf (*ret,
                    NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsOutSegLbPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_outseg_nxt_hop_ipa_type (struct nsm_master *nm,
                                      u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct nhlfe_key *key;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    {
      if (nhlfe->type == gmpls_entry_type_ip)
        {
          key = (struct nhlfe_key *) nhlfe->nkey;

          if (key->afi == AFI_IP)
            *ret = INET_AD_IPV4;
#ifdef HAVE_IPV6
          else if (key->afi == AFI_IP6)
            *ret = INET_AD_IPV6;
#endif /* HAVE_IPV6. */
          else
            return NSM_API_GET_ERROR;
        }
      else
        return NSM_API_GET_ERROR;
    }
  else if (nhlfe_temp)
    *ret = INET_AD_IPV4;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_nxt_hop_ipa_type (struct nsm_master *nm,
                                           u_int32_t *outseg_ix,
                                           u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct nhlfe_key *key;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    {
      if (nhlfe->type == gmpls_entry_type_ip)
        {
          key = (struct nhlfe_key *) nhlfe->nkey;

          if (key->afi == AFI_IP)
            *ret = INET_AD_IPV4;
#ifdef HAVE_IPV6
          else if (key->afi == AFI_IP6)
            *ret = INET_AD_IPV6;
#endif /* HAVE_IPV6. */
          else
            return NSM_API_GET_ERROR;
        }
      else
        return NSM_API_GET_ERROR;
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = INET_AD_IPV4;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_nxt_hop_ipa (struct nsm_master *nm,
                                 u_int32_t outseg_ix, char **ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct nhlfe_key *key;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    {
      if (nhlfe->type == gmpls_entry_type_ip)
        {
          key = (struct nhlfe_key *) nhlfe->nkey;

          if (key->afi == AFI_IP)
            pal_inet_ntop (AF_INET,
                           &(key->u.ipv4.nh_addr),
                           *ret,
                           NSM_MPLS_MAX_STRING_LENGTH);
#ifdef HAVE_IPV6
          else if (key->afi == AFI_IP6)
            pal_inet_ntop (AF_INET6,
                           &(key->u.ipv6.nh_addr),
                           *ret,
                           NSM_MPLS_MAX_STRING_LENGTH);
#endif /* HAVE_IPV6. */
#ifdef HAVE_GMPLS
          else if (key->afi == AFI_UNNUMBERED)
            pal_inet_ntop (AF_INET6,
                           &(key->u.ipv4.nh_addr),
                           *ret,
                           NSM_MPLS_MAX_STRING_LENGTH);

#endif /* HAVE_GMPLS */
          else
            return NSM_API_GET_ERROR;
        }
      else
        return NSM_API_GET_ERROR;
    }
  else if (nhlfe_temp)
    pal_inet_ntop (AF_INET,
                   &(nhlfe_temp->nh_addr),
                   *ret,
                   NSM_MPLS_MAX_STRING_LENGTH);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_nxt_hop_ipa (struct nsm_master *nm,
                                      u_int32_t *outseg_ix,
                                      char **ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    {
      if (nhlfe->type == gmpls_entry_type_ip)
        {
          struct nhlfe_key *key;

          key = (struct nhlfe_key *) nhlfe->nkey;

          if (key->afi == AFI_IP)
            pal_inet_ntop (AF_INET,
                           &(key->u.ipv4.nh_addr),
                           *ret,
                           NSM_MPLS_MAX_STRING_LENGTH);
#ifdef HAVE_IPV6
          else if (key->afi == AFI_IP6)
            pal_inet_ntop (AF_INET6,
                           &(key->u.ipv6.nh_addr),
                           *ret,
                           NSM_MPLS_MAX_STRING_LENGTH);
#endif /* HAVE_IPV6. */
          else
            return NSM_API_GET_ERROR;
        }
      else
        return NSM_API_GET_ERROR;
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    pal_inet_ntop (AF_INET,
                   &(nhlfe_temp->nh_addr.u.prefix4),
                   *ret, NSM_MPLS_MAX_STRING_LENGTH);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_xc_ix (struct nsm_master *nm,
                           u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nhlfe->xc_ix;
  else if (nhlfe_temp)
    *ret = nhlfe_temp->xc_ix;    /* In temp. DS, will it always be 0? */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_xc_ix (struct nsm_master *nm,
                                u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nhlfe->xc_ix;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = nhlfe_temp->xc_ix;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_owner (struct nsm_master *nm,
                           u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nhlfe->owner;
  else if (nhlfe_temp)
    *ret = nhlfe_temp->owner;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_owner (struct nsm_master *nm,
                                u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nhlfe->owner;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = nhlfe_temp->owner;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_tf_prm (struct nsm_master *nm,
                            u_int32_t outseg_ix, char **ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe || nhlfe_temp)
    {
      pal_snprintf (*ret,
                    NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsOutSegTnRscPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_outseg_tf_prm (struct nsm_master *nm,
                                 u_int32_t *outseg_ix, char **ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      pal_snprintf (*ret,
                    NSM_MPLS_MAX_STRING_LENGTH,
                    "mplsOutSegTnRscPtr.%d.%d",
                    null_oid[0], null_oid[1]);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_outseg_row_status (struct nsm_master *nm,
                                u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nhlfe->row_status;
  else if (nhlfe_temp)
    *ret = nhlfe_temp->row_status;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_row_status (struct nsm_master *nm,
                                     u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nhlfe->row_status;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = nhlfe_temp->row_status;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_st_type (struct nsm_master *nm,
                             u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe || nhlfe_temp)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_outseg_st_type (struct nsm_master *nm,
                                  u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}


/* APIs related with MPLS Out-segment performance table. */
u_int32_t
nsm_gmpls_get_outseg_octs (struct nsm_master *nm,
                          u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nsm_cnt_out_seg_octs (nhlfe);
  else if (nhlfe_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_octs (struct nsm_master *nm,
                               u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nsm_cnt_out_seg_octs (nhlfe);
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_pkts (struct nsm_master *nm,
                          u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nsm_cnt_out_seg_pkts (nhlfe);
  else if (nhlfe_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_pkts (struct nsm_master *nm,
                               u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nsm_cnt_out_seg_pkts (nhlfe);
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_errors (struct nsm_master *nm,
                            u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nsm_cnt_out_seg_errors (nhlfe);
  else if (nhlfe_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_errors (struct nsm_master *nm,
                                 u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nsm_cnt_out_seg_errors (nhlfe);
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_discards (struct nsm_master *nm,
                              u_int32_t outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nsm_cnt_out_seg_discards (nhlfe);
  else if (nhlfe_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_discards (struct nsm_master *nm,
                                   u_int32_t *outseg_ix, u_int32_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nsm_cnt_out_seg_discards (nhlfe);
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_hc_octs (struct nsm_master *nm,
                             u_int32_t outseg_ix, ut_int64_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe || nhlfe_temp)
    *ret = nsm_cnt_out_seg_hc_octs (nhlfe);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_hc_octs (struct nsm_master *nm,
                                  u_int32_t *outseg_ix, ut_int64_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    *ret = nsm_cnt_out_seg_hc_octs (nhlfe);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_outseg_perf_dis_time (struct nsm_master *nm,
                                   u_int32_t outseg_ix, time_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;

  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  if (nhlfe)
    *ret = nsm_cnt_out_seg_perf_dis_time (nhlfe);
  else if (nhlfe_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_outseg_perf_dis_time (struct nsm_master *nm,
                                        u_int32_t *outseg_ix, time_t *ret)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  u_int32_t type;

  nsm_gmpls_outseg_lookup_next (nm, *outseg_ix, &nhlfe_temp, &nhlfe);

  /* Set Index value */
  nsm_gmpls_get_next_outseg_index_set (nhlfe, nhlfe_temp, outseg_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = nsm_cnt_out_seg_perf_dis_time (nhlfe);
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

/* Support APIs for XC Table set */
struct xc_entry *
nsm_mpls_xc_key_lookup (struct nsm_master *nm, u_int32_t xc_ix,
                        u_int32_t inseg_ix, u_int32_t outseg_ix)
{
  struct xc_entry *xc = NULL;
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct gmpls_gen_label lbl;

  if (!inseg_ix && !outseg_ix)
    return NULL;

  /* Lookup - Both main and temp. DS to get corresponding entry */
  if (inseg_ix)
    nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);

  if (ilm)
    {
      switch (ilm->ent_type)
        {
          case ILM_ENTRY_TYPE_PACKET:
            lbl.type = gmpls_entry_type_ip;
            lbl.u.pkt = ilm->key.u.pkt.in_label;
            xc = nsm_gmpls_xc_lookup (nm, xc_ix, ilm->key.u.pkt.iif_ix,
                                      &lbl, outseg_ix, NSM_FALSE);
            break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case ILM_ENTRY_TYPE_PBB_TE:
            lbl.type = gmpls_entry_type_pbb_te;
            pal_mem_cpy(&lbl.u.pbb, &ilm->key.u.pbb_key.in_label, sizeof(struct pbb_te_label));
            xc = nsm_gmpls_xc_lookup (nm, xc_ix, ilm->key.u.pbb_key.iif_ix,
                                      &lbl, outseg_ix, NSM_FALSE);

            break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
          case ILM_ENTRY_TYPE_TDM:
            break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
          default:
            break;
        }
    }
  else if (ilm_temp)
    {
      xc = nsm_gmpls_xc_lookup (nm, xc_ix, ilm_temp->iif_ix,
                                &ilm_temp->in_label, outseg_ix, NSM_FALSE);
    }
  else
    {
      struct gmpls_gen_label in_label;
      pal_mem_set (&in_label, 0, sizeof (struct gmpls_gen_label));
      in_label.type = gmpls_entry_type_ip;
      xc = nsm_gmpls_xc_lookup (nm, xc_ix, 0, &in_label, outseg_ix, NSM_FALSE);
    }

  return xc;
}

struct xc_entry *
nsm_gmpls_xc_key_lookup_next (struct nsm_master *nm, u_int32_t xc_ix,
                             u_int32_t inseg_ix, u_int32_t outseg_ix,
                             u_int32_t index_len)
{
  struct xc_entry *xc = NULL;
  struct ilm_entry *ilm = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  u_int32_t iif_ix = 0;
  struct gmpls_gen_label in_label;

  if (inseg_ix)
    {
      ilm = nsm_gmpls_ilm_ix_lookup (nm, inseg_ix);
      ilm_temp = nsm_gmpls_ilm_temp_lookup (inseg_ix, 0, 0, IDX_LOOKUP);
      if (ilm)
        {
          switch (ilm->ent_type)
            {
              case ILM_ENTRY_TYPE_PACKET:
                in_label.type =  gmpls_entry_type_ip;
                in_label.u.pkt = ilm->key.u.pkt.in_label;

                iif_ix = ilm->key.u.pkt.iif_ix;
                break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
              case ILM_ENTRY_TYPE_PBB_TE:
                in_label.type =  gmpls_entry_type_pbb_te;
                pal_mem_cpy(&in_label.u.pbb,
                            &ilm->key.u.pbb_key.in_label, sizeof(struct pbb_te_label));

                iif_ix = ilm->key.u.pbb_key.iif_ix;
                break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
              case ILM_ENTRY_TYPE_TDM:
                break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
              default:
                break;
            }
        }
      else if (ilm_temp)
        {
          iif_ix = ilm_temp->iif_ix;
          in_label = ilm_temp->in_label;
        }
    }

  xc = nsm_gmpls_xc_lookup_next (nm, xc_ix, iif_ix, &in_label, outseg_ix,
                                 index_len);
  return xc;
}

/* Set Index for Get Next */
void
nsm_gmpls_get_next_xc_index_set (struct nsm_master *nm, struct xc_entry *xc,
                                u_int32_t *xc_ix, u_int32_t *inseg_ix,
                                u_int32_t *outseg_ix)
{
  struct ilm_entry *ilm = NULL;

  if (xc)
    {
      /* Set next entry index. */
      *xc_ix = xc->key.xc_ix;
      *outseg_ix = xc->key.nhlfe_ix;

      ilm = nsm_gmpls_ilm_lookup (nm, xc->key.iif_ix, &xc->key.in_label,
                                 0, NSM_FALSE, NSM_FALSE);
      if (ilm)
        *inseg_ix = ilm->ilm_ix;
      else
        *inseg_ix = xc->ilm_ix;
    }
  else
    {
      *xc_ix = 0;
      *inseg_ix = 0;
      *outseg_ix = 0;
    }
}

s_int32_t
nsm_gmpls_xc_create (struct nsm_master *nm, struct ilm_entry *ilm,
                    struct ftn_entry *ftn, struct fec_gen_entry *fec,
                    struct xc_entry *xc)
{
  if (ilm)
    {
      gmpls_ilm_xc_link (ilm, xc);
      /* Make Admn Status UP and Row Status Active */
      xc->admn_status = ADMN_UP;

      SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
      gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
    }
  else if (ftn)         /* This case is reached for FTN when inseg_ix = 0 */
    {
      /* Make Admn Status UP and Row Status Active */
      xc->admn_status = ADMN_UP;
      xc->opr_status = OPR_UP;
      nsm_gmpls_ftn_select (nm, ftn, fec);
    }

  xc->row_status = RS_ACTIVE;
  return NSM_SUCCESS;
}

/* APIs related to XC Table set */
u_int32_t
nsm_mpls_set_xc_lspid (struct nsm_master *nm,
                       u_int32_t xc_ix, u_int32_t inseg_ix,
                       u_int32_t outseg_ix)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc && xc->row_status != RS_ACTIVE)
    {
      pal_mem_set (&(xc->lspid), 0, sizeof (struct mpls_lspid));
      return NSM_API_SET_SUCCESS;
    }
  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_xc_lb_stk_ix (struct nsm_master *nm,
                           u_int32_t xc_ix, u_int32_t inseg_ix,
                           u_int32_t outseg_ix, u_int32_t lb_stk_ix)
{
  struct xc_entry *xc = NULL;

  /* Check for Validity of value */
  if (lb_stk_ix != 0)
    return NSM_API_SET_ERROR;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc && xc->row_status != RS_ACTIVE)
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_xc_row_status (struct nsm_master *nm,
                            u_int32_t xc_ix, u_int32_t inseg_ix,
                            u_int32_t outseg_ix, u_int32_t row_status)
{
  struct xc_entry *xc = NULL;
  struct ilm_entry *ilm = NULL;
  struct nhlfe_entry *nhlfe = NULL;
  struct ftn_entry *ftn = NULL;
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct fec_gen_entry fec;
  struct nsm_mpls *nmpls = nm->nmpls;
  s_int32_t ret = NSM_FAILURE;
  u_int32_t iif_ix = 0;
  struct gmpls_gen_label in_label;

  /*
  struct route_ix_table *rix_tbl;
  rix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID);
  */

  pal_mem_set (&fec, 0, sizeof (struct fec_gen_entry));
  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  /* Lookup - Both main and temp. DS to get corresponding entry */
  nsm_mpls_inseg_lookup (nm, inseg_ix, &ilm_temp, &ilm);
  nsm_gmpls_outseg_lookup (nm, outseg_ix, &nhlfe_temp, &nhlfe);

  ftn = nsm_mpls_xc_ix_ftn_lookup (nm, xc_ix);
  if (ftn)
    ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn->ftn_ix, &fec);

  if (nhlfe && nhlfe->xc_ix && nhlfe->xc_ix != xc_ix)
    return NSM_API_SET_ERROR;

  switch (row_status)
    {
      case RS_ACTIVE:
        if (xc && xc->row_status == RS_ACTIVE) /* Already Active */
          return NSM_API_SET_SUCCESS;
        if (xc && xc->row_status == RS_NOT_IN_SERVICE)
          {
            xc->row_status = RS_ACTIVE;

            if (xc->admn_status == ADMN_UP && xc->opr_status == OPR_DOWN &&
                nhlfe && nhlfe->row_status == RS_ACTIVE)
              {
                if (inseg_ix)
                  {
                    ilm = nsm_gmpls_ilm_ix_lookup (nm, inseg_ix);
                    if (!ilm || ilm->row_status != RS_ACTIVE)
                      return NSM_API_SET_SUCCESS;

                    gmpls_ilm_xc_link (ilm, xc);  /* Link XC and ILM entries */

                    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
                    gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
                  }
                else
                  {
                    ftn = nsm_mpls_xc_ix_ftn_lookup (nm, xc->key.xc_ix);
                    if(ftn)
                      {
                        if (ftn->row_status != RS_ACTIVE)
                          return NSM_API_SET_ERROR;
                        if (fec.type != gmpls_entry_type_error)
                          nsm_gmpls_ftn_select (nm, ftn, &fec);
                      }
                  }
                nhlfe->xc_ix = xc_ix;  /* Linking overwrites xc->xc_ix */
                gmpls_xc_nhlfe_link (xc, nhlfe);
              }
            return NSM_API_SET_SUCCESS; /* Nothing to be done */
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_IN_SERVICE:
        if (xc && xc->row_status == RS_NOT_IN_SERVICE)  /* Already NotInServ */
          return NSM_API_SET_SUCCESS;
        if (xc && xc->row_status == RS_ACTIVE)  /* Current state - Active */
          {
            xc->row_status = RS_NOT_IN_SERVICE;

            if (ilm && xc->nhlfe)
              /* Delete from Forwarder and Unlink XC entry */
              /* Do not delete XC or NHLFE entries */
              gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);
            else if (ftn && nhlfe && fec.type != gmpls_entry_type_error)
              nsm_gmpls_ftn_deselect (nm, ftn, &fec);
            else
              return NSM_API_SET_SUCCESS;

            /* Unlink XC and NHLFE */
            gmpls_xc_nhlfe_unlink (xc);

            if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA)
              nsm_mpls_opr_sts_trap (nmpls, xc_ix, OPR_DOWN);

            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_READY:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_GO:
        /* If already exists or ILM linked */
        if (xc || (ilm && ilm->xc))
          return NSM_API_SET_ERROR;

        if (ilm)
          {
            switch (ilm->ent_type)
              {
                case ILM_ENTRY_TYPE_PACKET:
                  in_label.type =  gmpls_entry_type_ip;
                  in_label.u.pkt = ilm->key.u.pkt.in_label;

                  iif_ix = ilm->key.u.pkt.iif_ix;
                  break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                case ILM_ENTRY_TYPE_PBB_TE:
                  in_label.type =  gmpls_entry_type_pbb_te;
                  pal_mem_cpy(&in_label.u.pbb,
                              &ilm->key.u.pbb_key.in_label, sizeof(struct pbb_te_label));
                  iif_ix = ilm->key.u.pbb_key.iif_ix;
                  break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                case ILM_ENTRY_TYPE_TDM:
                  break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                default:
                  break;
              }
          }
        else if (ilm_temp)
          {
            iif_ix = ilm_temp->iif_ix;
            in_label = ilm_temp->in_label;
          }

        xc = nsm_gmpls_create_new_xc_entry (nm, xc_ix, inseg_ix, outseg_ix,
                                            iif_ix, &in_label, nhlfe);
        if (xc && fec.type != gmpls_entry_type_error)
          ret = nsm_gmpls_xc_create (nm, ilm, ftn, &fec, xc);

        if (ret == NSM_SUCCESS)
          return NSM_API_SET_SUCCESS;

        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_WAIT:
        if (xc || (ilm && ilm->xc))
          return NSM_API_SET_ERROR;

        if (ilm)
          {
            switch (ilm->ent_type)
              {
                case ILM_ENTRY_TYPE_PACKET:
                  in_label.type =  gmpls_entry_type_ip;
                  in_label.u.pkt = ilm->key.u.pkt.in_label;

                  iif_ix = ilm->key.u.pkt.iif_ix;
                  break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                case ILM_ENTRY_TYPE_PBB_TE:
                  in_label.type =  gmpls_entry_type_pbb_te;
                  pal_mem_cpy(&in_label.u.pbb,
                              &ilm->key.u.pbb_key.in_label, sizeof(struct pbb_te_label));

                  iif_ix = ilm->key.u.pbb_key.iif_ix;
                  break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                case ILM_ENTRY_TYPE_TDM:
                  break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                default:
                  break;
              }
          }
        else if (ilm_temp)
          {
            iif_ix = ilm_temp->iif_ix;
            in_label = ilm_temp->in_label;
          }

        xc = nsm_gmpls_create_new_xc_entry (nm, xc_ix, inseg_ix, outseg_ix,
                                            iif_ix, &in_label, nhlfe);
        if (xc)
          return NSM_API_SET_SUCCESS;

        return NSM_API_SET_ERROR;
        break;

      case RS_DESTROY:
        if (xc)
          {
            if (ilm && nhlfe)
              /* Delete from Forwarder and Unlink XC entry */
              /* Do not delete XC or NHLFE entries */
              gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);
            else if (ftn && nhlfe && fec.type != gmpls_entry_type_error)
              nsm_gmpls_ftn_deselect (nm, ftn, &fec);

            xc =  nsm_gmpls_xc_lookup (nm, xc->key.xc_ix,
                                       xc->key.iif_ix,
                                       &xc->key.in_label,
                                       xc->key.nhlfe_ix,
                                       NSM_TRUE);

            if (xc)
              {
                bitmap_release_index (XC_IX_MGR, xc->key.xc_ix);
                xc->key.xc_ix = 0;
                gmpls_xc_free (xc);
              }
          }

        return NSM_API_SET_SUCCESS;
        break;

      default:
        return NSM_API_SET_ERROR;
        break;
    }
}

u_int32_t
nsm_mpls_set_xc_st_type (struct nsm_master *nm,
                         u_int32_t xc_ix, u_int32_t inseg_ix,
                         u_int32_t outseg_ix, u_int32_t st_type)
{
  struct xc_entry *xc = NULL;

  /* Check for Validity of value */
  if (st_type != ST_VOLATILE)
    return NSM_API_SET_ERROR;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_xc_adm_status (struct nsm_master *nm,
                            u_int32_t xc_ix, u_int32_t inseg_ix,
                            u_int32_t outseg_ix, u_int32_t admn_status)
{
  struct xc_entry *xc = NULL;
  struct ilm_entry *ilm = NULL;
  struct ftn_entry *ftn = NULL;
  struct nhlfe_entry *nhlfe = NULL;
  struct nsm_mpls *nmpls = nm->nmpls;
  struct fec_gen_entry fec;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (inseg_ix)
    ilm = nsm_gmpls_ilm_ix_lookup (nm, inseg_ix);

  nhlfe = nsm_mpls_nhlfe_ix_lookup (nm, outseg_ix);

  if (xc)
    {
      if ((admn_status == ADMN_UP && xc->admn_status == ADMN_UP) ||
          (admn_status == ADMN_DOWN && xc->admn_status == ADMN_DOWN))
        return NSM_API_SET_SUCCESS;

      if (admn_status == ADMN_UP)
        {
          xc->admn_status = ADMN_UP;

          if (xc->row_status == RS_ACTIVE && nhlfe)
            {
              if (inseg_ix)
                {
                  if (! ilm) /* Active entry not found */
                    return NSM_API_SET_SUCCESS;

                  gmpls_ilm_xc_link (ilm, xc);  /* Link XC and ILM entries */
                  SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
                  gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
                }
              else
                {
                  ftn = nsm_mpls_xc_ix_ftn_lookup (nm, xc->key.xc_ix);
                  if (ftn)
                    ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn->ftn_ix, &fec);
                  if (ftn)
                    {
                      if (ftn->row_status != RS_ACTIVE)
                        return NSM_API_SET_ERROR;

                      nsm_gmpls_ftn_select (nm, ftn, &fec);
                    }
                }
              nhlfe->xc_ix = xc_ix;  /* Linking overwrites xc->xc_ix */
              gmpls_xc_nhlfe_link (xc, nhlfe);
            }
          return NSM_API_SET_SUCCESS;
        }

      else if (admn_status == ADMN_DOWN)
        {
          xc->admn_status = ADMN_DOWN;

          ilm = nsm_gmpls_ilm_ix_lookup (nm, inseg_ix);
          if (ilm && ilm->xc)
            gmpls_ilm_xc_unlink (ilm);

          if (ilm && xc->nhlfe && xc->opr_status == OPR_UP)
            /* Delete from Forwarder and Unlink XC entry */
            /* Do not delete XC or NHLFE entries */
            gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);
          else
            return NSM_API_SET_SUCCESS;

          gmpls_xc_nhlfe_unlink (xc);

          if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA)
            nsm_mpls_opr_sts_trap (nmpls, xc_ix, OPR_DOWN);

          return NSM_API_SET_SUCCESS;
        }
      else
        return NSM_API_SET_ERROR;
    }
  return NSM_API_SET_ERROR;
}

/* APIs related with MPLS XC table GET */
u_int32_t
nsm_gmpls_get_xc_lspid (struct nsm_master *nm,
                       u_int32_t xc_ix, u_int32_t inseg_ix,
                       u_int32_t outseg_ix, char **ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      pal_sprintf (*ret, "%d", xc->lspid);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_xc_lspid (struct nsm_master *nm,
                            u_int32_t *xc_ix,
                            u_int32_t *inseg_ix,
                            u_int32_t *outseg_ix,
                            u_int32_t index_len,
                            char **ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_gmpls_xc_key_lookup_next (nm, *xc_ix, *inseg_ix, *outseg_ix,
                                    index_len);

  nsm_gmpls_get_next_xc_index_set (nm, xc, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    { 
      pal_sprintf (*ret, "%d", xc->lspid);
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_xc_lb_stk_ix (struct nsm_master *nm,
                           u_int32_t xc_ix, u_int32_t inseg_ix,
                           u_int32_t outseg_ix,
                           u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = 0; /* 0 indicates no label to be stacked beneath the top label */
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_xc_lb_stk_ix (struct nsm_master *nm,
                                u_int32_t *xc_ix,
                                u_int32_t *inseg_ix,
                                u_int32_t *outseg_ix,
                                u_int32_t index_len,
                                u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_gmpls_xc_key_lookup_next (nm, *xc_ix, *inseg_ix, *outseg_ix,
                                    index_len);

  nsm_gmpls_get_next_xc_index_set (nm, xc, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_xc_owner (struct nsm_master *nm,
                       u_int32_t xc_ix, u_int32_t inseg_ix,
                       u_int32_t outseg_ix,
                       u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->owner;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_xc_owner (struct nsm_master *nm,
                            u_int32_t *xc_ix,
                            u_int32_t *inseg_ix,
                            u_int32_t *outseg_ix,
                            u_int32_t index_len,
                            u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_gmpls_xc_key_lookup_next (nm, *xc_ix, *inseg_ix, *outseg_ix,
                                    index_len);

  nsm_gmpls_get_next_xc_index_set (nm, xc, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->owner;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_xc_row_status (struct nsm_master *nm,
                            u_int32_t xc_ix, u_int32_t inseg_ix,
                            u_int32_t outseg_ix,
                            u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->row_status;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_xc_row_status (struct nsm_master *nm,
                                 u_int32_t *xc_ix,
                                 u_int32_t *inseg_ix,
                                 u_int32_t *outseg_ix,
                                 u_int32_t index_len,
                                 u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_gmpls_xc_key_lookup_next (nm, *xc_ix, *inseg_ix, *outseg_ix,
                                    index_len);

  nsm_gmpls_get_next_xc_index_set (nm, xc, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->row_status;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_xc_st_type (struct nsm_master *nm,
                         u_int32_t xc_ix, u_int32_t inseg_ix,
                         u_int32_t outseg_ix,
                         u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_xc_st_type (struct nsm_master *nm,
                              u_int32_t *xc_ix,
                              u_int32_t *inseg_ix,
                              u_int32_t *outseg_ix,
                              u_int32_t index_len,
                              u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_gmpls_xc_key_lookup_next (nm, *xc_ix, *inseg_ix, *outseg_ix,
                                    index_len);

  nsm_gmpls_get_next_xc_index_set (nm, xc, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_xc_adm_status (struct nsm_master *nm,
                            u_int32_t xc_ix, u_int32_t inseg_ix,
                            u_int32_t outseg_ix,
                            u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->admn_status;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_xc_adm_status (struct nsm_master *nm,
                                 u_int32_t *xc_ix,
                                 u_int32_t *inseg_ix,
                                 u_int32_t *outseg_ix,
                                 u_int32_t index_len,
                                 u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_gmpls_xc_key_lookup_next (nm, *xc_ix, *inseg_ix, *outseg_ix,
                                    index_len);

  nsm_gmpls_get_next_xc_index_set (nm, xc, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->admn_status;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_xc_oper_status (struct nsm_master *nm,
                             u_int32_t xc_ix, u_int32_t inseg_ix,
                             u_int32_t outseg_ix,
                             u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_mpls_xc_key_lookup (nm, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->opr_status;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_xc_oper_status (struct nsm_master *nm,
                                  u_int32_t *xc_ix,
                                  u_int32_t *inseg_ix,
                                  u_int32_t *outseg_ix,
                                  u_int32_t index_len,
                                  u_int32_t *ret)
{
  struct xc_entry *xc = NULL;

  xc = nsm_gmpls_xc_key_lookup_next (nm, *xc_ix, *inseg_ix, *outseg_ix,
                                    index_len);

  nsm_gmpls_get_next_xc_index_set (nm, xc, xc_ix, inseg_ix, outseg_ix);

  if (xc)
    {
      *ret = xc->opr_status;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

/* Support APIs for FTN Table set */
void
nsm_gmpls_ftn_entry_lookup (struct nsm_master *nm,
                            u_int32_t ftn_ix,
                            struct fec_gen_entry *fec,
                            struct ftn_temp_ptr **ftn_temp,
                            struct ftn_entry **ftn)
{
  *ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn_ix, fec);
  *ftn_temp = nsm_gmpls_ftn_temp_lookup (ftn_ix, &src_addr, IDX_LOOKUP);
}

/* FTN Lookup Next - Both temporary and main Data structures */
void
nsm_gmpls_ftn_entry_lookup_next (struct nsm_master *nm,
                                u_int32_t ftn_ix,
                                struct fec_gen_entry *fec,
                                struct ftn_temp_ptr **ftn_temp,
                                struct ftn_entry **ftn)
{
  *ftn = nsm_gmpls_ftn_ix_lookup_next (nm, ftn_ix, fec);
  *ftn_temp = nsm_gmpls_ftn_temp_lookup_next (ftn_ix);
}

/* Set Index for Get Next */
void
nsm_gmpls_get_next_ftn_index_set (struct ftn_entry *ftn,
                                 struct ftn_temp_ptr *ftn_temp,
                                 u_int32_t *ftn_ix, u_int32_t *index_type)
{
  if ((ftn && !ftn_temp) ||
      (ftn && ftn_temp && ftn->ftn_ix < ftn_temp->ftn_ix))
    {
      *ftn_ix = ftn->ftn_ix;
      *index_type = TYPE_MAIN;
    }

  else if ((!ftn && ftn_temp) ||
           (ftn && ftn_temp && ftn->ftn_ix > ftn_temp->ftn_ix))
    {
      *ftn_ix = ftn_temp->ftn_ix;
      *index_type = TYPE_TEMP;
    }

  else
    {
      *ftn_ix = 0;
      *index_type = TYPE_NONE;
    }
}

s_int32_t
nsm_gmpls_create_new_ftn_entry (struct nsm_master *nm,
                                struct ftn_temp_ptr *ftn_temp,
                                struct ftn_entry **ftn_entry)
{
  struct ftn_entry *ftn = NULL;
  struct ptree_node *pn;
  struct ptree_ix_table *pix_tbl;
  s_int32_t ret;

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &ftn_temp->fec);

  if (! pix_tbl)
    return NSM_FAILURE;

  ftn = XCALLOC (MTYPE_MPLS_FTN_ENTRY, sizeof (struct ftn_entry));

  if (!ftn)
    return NSM_FAILURE; /* nsm_err_mem_alloc_failure */

  /* set FTN index */
  ftn->ftn_ix = ftn_temp->ftn_ix;

  if (ftn_temp->xc)
    gmpls_ftn_xc_link (ftn, ftn_temp->xc);

  ftn->lsp_bits.bits.act_type = ftn_temp->act_type;
  ftn->owner.owner = ftn_temp->owner;
  ftn->dscp_in = DIFFSERV_INVALID_DSCP;
  SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY);

  /* add ftn entry to table */
  pn = gmpls_ftn_add (nm, pix_tbl, ftn, &ftn_temp->fec, &ret);
  if (pn == NULL)
    {
      gmpls_ftn_entry_cleanup (nm, ftn, &ftn_temp->fec, pix_tbl);
      gmpls_ftn_free (ftn);
      return NSM_FAILURE;
    }

  ftn->pn = pn;
  ftn->row_status = RS_ACTIVE;

  XFREE (MTYPE_TMP, ftn_temp->sz_desc);
  ftn_temp->sz_desc = NULL;

  *ftn_entry = ftn;

  return NSM_SUCCESS;
}

/* Create Backup (Non-Active) FTN entry */
u_int32_t
nsm_gmpls_create_new_ftn_temp (struct nsm_master *nm, struct ftn_entry *ftn,
                              u_int32_t ftn_ix, struct fec_gen_entry *fec)
{
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct ptree_ix_table *pix_tbl;

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, fec);

  ftn_temp = XCALLOC (MTYPE_MPLS_FTN_ENTRY, sizeof (struct ftn_temp_ptr));
  if (! ftn_temp)
    return NSM_FAILURE;

  ftn_temp->ftn_ix = ftn_ix;

  if (ftn)
    {
      if (ftn->sz_desc)
        ftn_temp->sz_desc = XSTRDUP (MTYPE_TMP, ftn->sz_desc);

      if (ftn->xc)
        {
          ftn_temp->xc = ftn->xc;
          ftn_temp->xc_ix = ftn->xc->key.xc_ix;
          ftn_temp->nhlfe_ix = ftn->xc->key.nhlfe_ix;
        }
      ftn_temp->fec = *fec;
      ftn_temp->act_type = ftn->lsp_bits.bits.act_type;
      ftn_temp->owner = ftn->owner.owner;

      ftn_temp->row_status = RS_NOT_IN_SERVICE;

      SET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_DST_ADDR_MINMAX_SET);
      SET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_ACT_POINTER_SET);
      SET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_PREFIX);
    }
  else  /* Entry to be newly created */
    {
      /* Inform bitmap of new index */
      if (pix_tbl)
        bitmap_add_index (pix_tbl->ix_mgr, ftn_ix);
      ftn_temp->act_type = REDIRECT_LSP;
      ftn_temp->row_status = RS_NOT_READY;
      ftn_temp->owner = MPLS_SNMP;
    }

  listnode_add (ftn_entry_temp_list, ftn_temp);
  return NSM_SUCCESS;
}

/* APIs related with MPLS FTN Table. */
/* APIs related to set */
u_int32_t
nsm_gmpls_set_ftn_row_status (struct nsm_master *nm,
                             u_int32_t ftn_ix, u_int32_t row_status)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct ptree_ix_table *pix_tbl;
  struct ptree_node *pn = NULL;
  struct fec_gen_entry fec;
  s_int32_t ret;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix.family = src_addr.family;

  ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn_ix, &fec);
  ftn_temp = nsm_gmpls_ftn_temp_lookup (ftn_ix, &src_addr, IDX_LOOKUP);

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec);

  if (ftn_temp)
    {
      ftn_temp->fec = fec;
    }

  switch (row_status)
    {
      case RS_ACTIVE:
        if (ftn)
          return NSM_API_SET_SUCCESS;
        if (ftn_temp && ftn_temp->row_status == RS_NOT_IN_SERVICE)
          {
            if (CHECK_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_ACT_POINTER_SET))
              {
                ret = nsm_gmpls_create_new_ftn_entry (nm, ftn_temp, &ftn);
                if (ret != NSM_SUCCESS)
                  return NSM_API_SET_ERROR;

                if (ftn_temp->xc)
                  {
                    gmpls_ftn_xc_link (ftn, ftn_temp->xc);
                    if (ftn->xc->row_status == RS_ACTIVE &&
                        ftn->xc->admn_status == ADMN_UP &&
                        ftn->xc->nhlfe &&
                        ftn->xc->nhlfe->row_status == RS_ACTIVE)
                      {
                        /* process selected ftn entry */
                        ret = nsm_gmpls_ftn_select (nm, ftn, &fec);
                        if (ret != NSM_SUCCESS)
                          {
                            if (pix_tbl)
                              nsm_gmpls_ftn_delete (nm, ftn, ftn_ix, &fec,
                                                    pix_tbl);
                            return NSM_API_SET_ERROR;
                          }
                      }
                     FTN_XC_STATUS_UPDATE (ftn);
                    }

                    listnode_delete (ftn_entry_temp_list, (void *)ftn_temp);
                    XFREE (MTYPE_MPLS_FTN_ENTRY, ftn_temp);
                }
               else
                ftn_temp->row_status = RS_ACTIVE;

              return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_IN_SERVICE:
        if (ftn_temp && ftn_temp->row_status == RS_NOT_IN_SERVICE)
          return NSM_API_SET_SUCCESS;
        if (ftn)
          {
            ret = nsm_gmpls_create_new_ftn_temp (nm, ftn, ftn_ix, &fec);
            if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
              return NSM_API_SET_ERROR;


            if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED) &&
                (fec.type == gmpls_entry_type_ip))
            {
#ifdef HAVE_IPV6
              if (fec.u.prefix.family == AF_INET6)
                nsm_mpls_ftn6_del_from_fwd (nm, ftn, &fec.u.prefix,
                                            GLOBAL_FTN_ID, NULL);
              else
#endif
                nsm_mpls_ftn4_del_from_fwd (nm, ftn, &fec.u.prefix,
                                            GLOBAL_FTN_ID, NULL);

              UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
            }

            gmpls_ftn_xc_unlink (ftn);

            /* Lookup for the node */
            if (pix_tbl)
              pn = gmpls_ftn_node_lookup (pix_tbl->m_table, &fec);

            /* Remove FTN Entry */
            if (pn)
              gmpls_ftn_entry_remove_list_node (nm,
                                                (struct ftn_entry **)&pn->info,
                                                ftn, pn, NSM_TRUE);

            if (pn && pn->info == NULL)
              ptree_unlock_node (pn);
              
            if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
              {
#ifdef HAVE_IPV6
                 if (fec.u.prefix.family == AF_INET6)
                    nsm_mpls_ftn6_del_from_fwd (nm, ftn, &fec.u.prefix,
                                                 pix_tbl->id, NULL);
                 else
#endif
                    nsm_mpls_ftn4_del_from_fwd (nm, ftn, &fec.u.prefix,
                                                pix_tbl->id, NULL);
                 UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
                 FTN_XC_OPR_STATUS_COUNT_DEC (ftn);
              }

           /* Lookup and delete */
            if (pix_tbl)
              gmpls_ftn_row_cleanup (nm, ftn, &fec, pix_tbl->id, NSM_TRUE);

             /* Release Memory */
            gmpls_ftn_free (ftn);
            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_READY:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_GO:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_WAIT:
        if (ftn || ftn_temp)  /* Entry already present. Cant recreate */
          return NSM_API_SET_ERROR;

        ret = nsm_gmpls_create_new_ftn_temp (nm, ftn, ftn_ix, &fec);
        if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
          return NSM_API_SET_ERROR;

        return NSM_API_SET_SUCCESS;
        break;

      case RS_DESTROY:
        if (ftn)
          {
            /* Lookup for the node */
            if (pix_tbl)
              pn = gmpls_ftn_node_lookup (pix_tbl->m_table, &fec);

            /* Remove FTN Entry */
            if (pn) 
              gmpls_ftn_entry_remove_list_node (nm,
                                                (struct ftn_entry **)&pn->info,
                                                ftn, pn, NSM_TRUE);

            if (pn && !(pn->info))
              ptree_unlock_node (pn);

            /* Index release also happens here */
            if (pix_tbl)
              gmpls_ftn_entry_cleanup (nm, ftn, &fec, pix_tbl);

            /* Release Memory */
            gmpls_ftn_free (ftn);
          }
        else if (ftn_temp)
          {
            XFREE (MTYPE_TMP, ftn_temp->sz_desc);
            ftn_temp->sz_desc = NULL;

            /* Delete node from temp. list and free memory */
            listnode_delete (ftn_entry_temp_list, (void *)ftn_temp);
            XFREE (MTYPE_MPLS_FTN_ENTRY, ftn_temp);
          }
        else
          return NSM_API_SET_ERROR;
        
        if (pix_tbl)
          /* Release Index */
           bitmap_release_index (pix_tbl->ix_mgr, ftn_ix);
        return NSM_API_SET_SUCCESS;
        break;

      default:
        return NSM_API_SET_ERROR;
        break;
    }
}

u_int32_t
nsm_mpls_set_ftn_descr (struct nsm_master *nm,
                        u_int32_t ftn_ix, char *descr)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_mask (struct nsm_master *nm,
                       u_int32_t ftn_ix, u_int32_t ftn_mask)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (ftn_mask != NSM_MPLS_FTN_MASK_DESTADDRESS)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_addr_type (struct nsm_master *nm,
                            u_int32_t ftn_ix, u_int32_t addr_type)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (addr_type != INET_AD_IPV4)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_src_addr_min_max (struct nsm_master *nm, u_int32_t ftn_ix,
                                   struct pal_in4_addr *addr)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  pal_mem_set (&src_addr, 0, sizeof (struct prefix));
  if (pal_mem_cmp (&src_addr, addr, sizeof (struct prefix)))
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_dst_addr_min_max (struct nsm_master *nm, u_int32_t ftn_ix,
                                   struct pal_in4_addr *addr)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;
  struct prefix pfx;
  struct ptree_ix_table *pix_tbl;

  /* Check for validity of value */
  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);
  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec);

  if (ftn_temp && !ftn)  /* Non Active entry found */
    {
      pfx.family = AF_INET;
      pfx.u.prefix4 = *addr;

      /* If found to be duplicate, return error */
      if (nsm_gmpls_ftn_lookup (nm, pix_tbl, &fec, ftn_ix, NSM_FALSE) ||
          nsm_gmpls_ftn_temp_lookup (ftn_ix, &pfx, NON_IDX_LOOKUP))
            return NSM_API_SET_ERROR;

      ftn_temp->fec.u.prefix.u.prefix4 = *addr;
      SET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_PREFIX);
      SET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_DST_ADDR_MINMAX_SET);
      /* Change rowStatus to Not in Service */
      ftn_temp->row_status = RS_NOT_IN_SERVICE;
      return NSM_API_SET_SUCCESS;
    }
  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_src_dst_port_min (struct nsm_master *nm,
                           u_int32_t ftn_ix, u_int32_t src_dst_port_min)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (src_dst_port_min != 0)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_src_dst_port_max (struct nsm_master *nm,
                                   u_int32_t ftn_ix, u_int32_t src_dst_port_max)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (src_dst_port_max != 65535)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_protocol (struct nsm_master *nm,
                           u_int32_t ftn_ix, u_int32_t protocol)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (protocol != 255)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_dscp (struct nsm_master *nm,
                       u_int32_t ftn_ix, u_int32_t dscp)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (dscp != 0)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_act_type (struct nsm_master *nm,
                           u_int32_t ftn_ix, u_int32_t act_type)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (act_type != SNMP_REDIRECT_LSP)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_act_pointer (struct nsm_master *nm, u_int32_t ftn_ix,
                              u_int32_t xc_ix, u_int32_t ilm_ix,
                              u_int32_t nhlfe_ix)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct xc_entry *xc = NULL;
  struct fec_gen_entry fec;
  struct ptree_ix_table *pix_tbl;
  struct ptree_node *pn;
  s_int32_t ret;

  /* Check for validity of value */
  if (ilm_ix)
    return NSM_API_SET_ERROR;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix.family = AF_INET;
  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec);

  /* Check for unsetting of FTN Action pointer and update */
  if (!ilm_ix && !xc_ix && !nhlfe_ix)
    {
      pal_mem_set (&src_addr, 0, sizeof (struct prefix));
      ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn_ix, &fec);
      ftn_temp = nsm_gmpls_ftn_temp_lookup (ftn_ix, &src_addr,
                                            IDX_LOOKUP);

      if (ftn && pix_tbl)
        {
          ret = nsm_gmpls_create_new_ftn_temp (nm, ftn, ftn_ix, &fec);
          if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
            return NSM_API_SET_ERROR;


          if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
            {
#ifdef HAVE_IPV6
              if (fec.u.prefix.family == AF_INET6)
                nsm_mpls_ftn6_del_from_fwd (nm, ftn, &fec.u.prefix,
                                            GLOBAL_FTN_ID, NULL);
              else
#endif
                nsm_mpls_ftn4_del_from_fwd (nm, ftn, &fec.u.prefix,
                                            GLOBAL_FTN_ID, NULL);

              UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
            }

            gmpls_ftn_xc_unlink (ftn);

            /* Lookup for the node */
            pn = gmpls_ftn_node_lookup (pix_tbl->m_table, &fec);

            /* Remove FTN Entry */
            if (pn)
            gmpls_ftn_entry_remove_list_node (nm,
                                              (struct ftn_entry **)&pn->info,
                                              ftn, pn, NSM_TRUE);

            if (pn && pn->info == NULL)
              ptree_unlock_node (pn);

            if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
              {
#ifdef HAVE_IPV6
                if (fec.u.prefix.family == AF_INET6)
                  nsm_mpls_ftn6_del_from_fwd (nm, ftn, &fec.u.prefix,
                                              pix_tbl->id, NULL);
                else
#endif
                  nsm_mpls_ftn4_del_from_fwd (nm, ftn, &fec.u.prefix,
                                              pix_tbl->id, NULL);

                UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
                FTN_XC_OPR_STATUS_COUNT_DEC (ftn);
              }

            /* Lookup and delete */
            gmpls_ftn_row_cleanup (nm, ftn, &fec, pix_tbl->id,
                                   NSM_TRUE);

            /* Release Memory */
            gmpls_ftn_free (ftn);

            nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);
            if (ftn_temp)
              {
                ftn_temp->row_status = RS_ACTIVE;
                UNSET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_ACT_POINTER_SET);
                UNSET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_INSTALLED);
                ftn_temp->xc = NULL;
                ftn_temp->xc_ix = 0;
                ftn_temp->nhlfe_ix = 0;
                return NSM_API_SET_SUCCESS;
              }
            else
              return NSM_API_SET_ERROR; 
          }

    }
  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn_temp || ftn)
    {
      struct gmpls_gen_label in_label;
      pal_mem_set (&in_label, 0, sizeof (struct gmpls_gen_label));
      in_label.type = gmpls_entry_type_ip;
      xc = nsm_gmpls_xc_lookup (nm, xc_ix, 0, &in_label,
                                nhlfe_ix, NSM_FALSE);
      if (! xc)
        return NSM_API_SET_ERROR;

      if (ftn)
        {
           struct fec_gen_entry tmp_fec;

           if (ftn->xc)
             gmpls_ftn_xc_unlink (ftn);

           gmpls_ftn_xc_link(ftn,xc);
           SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_ACT_POINTER_SET);

           tmp_fec.type = gmpls_entry_type_ip;
           tmp_fec.u.prefix.u.prefix4 = ftn->dst_addr;
           tmp_fec.u.prefix.family = AF_INET;
           tmp_fec.u.prefix.prefixlen = IPV4_MAX_BITLEN;

           if (ftn->xc && ftn->xc->row_status == RS_ACTIVE &&
               ftn->xc->admn_status == ADMN_UP &&
               ftn->xc->nhlfe &&
               ftn->xc->nhlfe->row_status == RS_ACTIVE)
             {
                /* process selected ftn entry */
                ret = nsm_gmpls_ftn_select (nm, ftn, &tmp_fec);
                if (ret != NSM_SUCCESS)
                  {
                    if (pix_tbl)
                      nsm_gmpls_ftn_delete (nm, ftn, ftn_ix, &tmp_fec,
                                         pix_tbl);
                    return NSM_API_SET_ERROR;
                  }
             }
           FTN_XC_STATUS_UPDATE (ftn);

           return NSM_API_SET_SUCCESS;
        }

      if (ftn_temp)
        {
           struct fec_gen_entry tmp_fec;

           ftn_temp->xc = xc;
           ftn_temp->xc_ix = xc_ix;
           ftn_temp->nhlfe_ix = nhlfe_ix;

           tmp_fec.type = gmpls_entry_type_ip;
           tmp_fec.u.prefix.u.prefix4 = ftn_temp->fec.u.prefix.u.prefix4;
           tmp_fec.u.prefix.family = AF_INET;
           tmp_fec.u.prefix.prefixlen = IPV4_MAX_BITLEN;

           SET_FLAG (ftn_temp->flags, FTN_ENTRY_FLAG_ACT_POINTER_SET);
           if (ftn_temp->row_status == RS_ACTIVE)
              {
                ret = nsm_gmpls_create_new_ftn_entry (nm, ftn_temp, &ftn);
                if (ret != NSM_SUCCESS)
                  return NSM_API_SET_ERROR;

                if (ftn_temp->xc)
                  {
                    gmpls_ftn_xc_link (ftn, ftn_temp->xc);
                    if (ftn->xc->row_status == RS_ACTIVE &&
                        ftn->xc->admn_status == ADMN_UP &&
                        ftn->xc->nhlfe &&
                        ftn->xc->nhlfe->row_status == RS_ACTIVE)
                      {
                        /* process selected ftn entry */
                        ret = nsm_gmpls_ftn_select (nm, ftn, &tmp_fec);
                        if (ret != NSM_SUCCESS)
                          {
                            if (pix_tbl)
                              nsm_gmpls_ftn_delete (nm, ftn, ftn_ix,
                                                  &tmp_fec, pix_tbl);
                            return NSM_API_SET_ERROR;
                          }
                      }
                     FTN_XC_STATUS_UPDATE (ftn);
                    }

                    listnode_delete (ftn_entry_temp_list, (void *)ftn_temp);
                    XFREE (MTYPE_MPLS_FTN_ENTRY, ftn_temp);
                }
          }
      return NSM_API_SET_SUCCESS;
    }

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_ftn_st_type (struct nsm_master *nm,
                          u_int32_t ftn_ix, u_int32_t st_type)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  /* Check for validity of value */
  if (st_type != ST_VOLATILE)
    return NSM_API_SET_ERROR;

  if (ftn_temp && !ftn)  /* Non Active entry found */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}


/* APIs related to get */
u_int32_t
nsm_gmpls_get_ftn_row_status (struct nsm_master *nm,
                             u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn)
    *ret = ftn->row_status;
  else if (ftn_temp)
    *ret = ftn_temp->row_status;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_row_status (struct nsm_master *nm,
                                  u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    *ret = ftn->row_status;
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    *ret = ftn_temp->row_status;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_descr (struct nsm_master *nm, u_int32_t ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn)
    {
      if (ftn->sz_desc)
        pal_snprintf (*ret, NSM_MPLS_MAX_STRING_LENGTH, "%s",
                      ftn->sz_desc);
    }
  else if (ftn_temp)
    {
      if (ftn_temp->sz_desc)
        pal_snprintf (*ret, NSM_MPLS_MAX_STRING_LENGTH, "%s",
                      ftn_temp->sz_desc);
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_descr (struct nsm_master *nm,
                             u_int32_t *ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)  /* If next entry is present in main entry */
    {
      if (ftn->sz_desc)
        pal_snprintf (*ret, NSM_MPLS_MAX_STRING_LENGTH, "%s",
                      ftn->sz_desc);
    }
  else if (type == TYPE_TEMP) /* If next entry is present in Backup */
    {
      if (ftn_temp->sz_desc)
        pal_snprintf (*ret, NSM_MPLS_MAX_STRING_LENGTH, "%s",
                      ftn_temp->sz_desc);
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_mask (struct nsm_master *nm, u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = NSM_MPLS_FTN_MASK_DESTADDRESS;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_mask (struct nsm_master *nm,
                            u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = NSM_MPLS_FTN_MASK_DESTADDRESS;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_addr_type (struct nsm_master *nm,
                            u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn_ix, &fec);
  ftn_temp = nsm_gmpls_ftn_temp_lookup (ftn_ix, &src_addr, IDX_LOOKUP);

  if (ftn && fec.type == gmpls_entry_type_ip)
    {
      if (fec.u.prefix.family == AF_INET)
        *ret = INET_AD_IPV4;
#ifdef HAVE_IPV6
      else if (fec.u.prefix.family == AF_INET6)
        *ret = INET_AD_IPV6;
#endif  /* HAVE_IPV6 */
      else
        return NSM_API_GET_ERROR;
    }
  else if (ftn_temp)
    *ret = INET_AD_IPV4;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_addr_type (struct nsm_master *nm,
                                 u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  ftn = nsm_gmpls_ftn_ix_lookup_next (nm, *ftn_ix, &fec);
  ftn_temp = nsm_gmpls_ftn_temp_lookup_next (*ftn_ix);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)
    {
      if (fec.u.prefix.family == AF_INET)
        *ret = INET_AD_IPV4;
#ifdef HAVE_IPV6
      else if (fec.u.prefix.family == AF_INET6)
        *ret = INET_AD_IPV6;
#endif /* HAVE_IPV6 */
      else
        return NSM_API_GET_ERROR;
    }
  else if (type == TYPE_TEMP)
    *ret = INET_AD_IPV4;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_gmpls_get_ftn_src_addr_min (struct nsm_master *nm,
                               u_int32_t ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      pal_inet_ntop (AF_INET,
                     &src_addr,
                     *ret,
                     NSM_MPLS_MAX_STRING_LENGTH);
        return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_src_addr_min (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      pal_inet_ntop (AF_INET,
                     &src_addr,
                     *ret,
                     NSM_MPLS_MAX_STRING_LENGTH);
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_src_addr_max (struct nsm_master *nm,
                               u_int32_t ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      pal_inet_ntop (AF_INET,
                     &src_addr,
                     *ret,
                     NSM_MPLS_MAX_STRING_LENGTH);
        return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_src_addr_max (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      pal_inet_ntop (AF_INET,
                     &src_addr,
                     *ret,
                     NSM_MPLS_MAX_STRING_LENGTH);
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_dst_addr_min (struct nsm_master *nm,
                               u_int32_t ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn_ix, &fec);
  ftn_temp = nsm_gmpls_ftn_temp_lookup (ftn_ix, &src_addr, IDX_LOOKUP);

  if (ftn)
    {
      if (fec.u.prefix.family == AF_INET)
        pal_inet_ntop (AF_INET,
                       &(fec.u.prefix.u.prefix4),
                       *ret,
                       NSM_MPLS_MAX_STRING_LENGTH);
#ifdef HAVE_IPv6
      else if (fec.u.prefix.family == AF_INET6)
        pal_inet_ntop (AF_INET,
                       &(fec.u.prefix.u.prefix6),
                       *ret,
                       NSM_MPLS_MAX_STRING_LENGTH);
#endif /* HAVE_IPv6 */
      else
        return NSM_API_GET_ERROR;
    }
  else if (ftn_temp)
    {
      pal_inet_ntop (AF_INET,
                     &(ftn_temp->fec.u.prefix.u.prefix4),
                     *ret,
                     NSM_MPLS_MAX_STRING_LENGTH);
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_dst_addr_min (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  ftn = nsm_gmpls_ftn_ix_lookup_next (nm, *ftn_ix, &fec);
  ftn_temp = nsm_gmpls_ftn_temp_lookup_next (*ftn_ix);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)
    {
      if (fec.u.prefix.family == AF_INET)
        pal_inet_ntop (AF_INET,
                       &(fec.u.prefix.u.prefix4),
                       *ret,
                       NSM_MPLS_MAX_STRING_LENGTH);
#ifdef HAVE_IPV6
      else if (fec.u.prefix.family == AF_INET6)
        pal_inet_ntop (AF_INET,
                       &(fec.u.prefix.u.prefix6),
                       *ret,
                       NSM_MPLS_MAX_STRING_LENGTH);
#endif /* HAVE_IPv6 */
      else
        return NSM_API_GET_ERROR;
    }

  else if (type == TYPE_TEMP)
    pal_inet_ntop (AF_INET,
                   &(ftn_temp->fec.u.prefix.u.prefix4),
                   *ret,
                   NSM_MPLS_MAX_STRING_LENGTH);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_dst_addr_max (struct nsm_master *nm,
                               u_int32_t ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct prefix max_addr;
  struct fec_gen_entry fec;

  pal_mem_set (&src_addr, 0, sizeof (struct prefix));

  ftn = nsm_gmpls_ftn_ix_lookup (nm, ftn_ix, &fec);
  ftn_temp = nsm_gmpls_ftn_temp_lookup (ftn_ix, &src_addr,
                                        IDX_LOOKUP);

  if (ftn)
    {
      if (fec.u.prefix.family == AF_INET)
        {
          /* Calculate the max addr from prefix. */
          nsm_mpls_api_cal_max_addr (&fec.u.prefix, &max_addr);

          pal_inet_ntop (AF_INET,
                         &(max_addr.u.prefix4),
                         *ret,
                         NSM_MPLS_MAX_STRING_LENGTH);
        }
#ifdef HAVE_IPV6
      else if (fec.u.prefix.family == AF_INET6)
        {
          /* Calculate the max addr from prefix. */
          nsm_mpls_api_cal_max_addr (&fec.u.prefix, &max_addr);

          pal_inet_ntop (AF_INET,
                         &(max_addr.u.prefix6),
                         *ret,
                         NSM_MPLS_MAX_STRING_LENGTH);
          return NSM_API_GET_SUCCESS;
        }
#endif /* HAVE_IPV6 */
      else
        return NSM_API_GET_ERROR;
    }
  else if (ftn_temp)
    {
      pal_inet_ntop (AF_INET,
                     &(ftn_temp->fec.u.prefix.u.prefix4),
                     *ret,
                     NSM_MPLS_MAX_STRING_LENGTH);
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_dst_addr_max (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct prefix max_addr;
  struct fec_gen_entry fec;
  u_int32_t type;

  ftn = nsm_gmpls_ftn_ix_lookup_next (nm, *ftn_ix, &fec);
  ftn_temp = nsm_gmpls_ftn_temp_lookup_next (*ftn_ix);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)
    {
      /* Calculate the max addr from prefix. */
      nsm_mpls_api_cal_max_addr (&fec.u.prefix, &max_addr);

      if (fec.u.prefix.family == AF_INET)
        pal_inet_ntop (AF_INET,
                       &(max_addr.u.prefix4),
                       *ret,
                       NSM_MPLS_MAX_STRING_LENGTH);
#ifdef HAVE_IPv6
      else if (fec.u.prefix.family == AF_INET6)
        pal_inet_ntop (AF_INET,
                       &(max_addr->u.prefix6),
                       *ret,
                       NSM_MPLS_MAX_STRING_LENGTH);
#endif /* HAVE_IPv6 */
      else
        return NSM_API_GET_ERROR;
    }

  else if (type == TYPE_TEMP)
    pal_inet_ntop (AF_INET,
                   &(ftn_temp->fec.u.prefix.u.prefix4),
                   *ret,
                   NSM_MPLS_MAX_STRING_LENGTH);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_src_port_min (struct nsm_master *nm,
                               u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_src_port_min (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_src_port_max (struct nsm_master *nm,
                               u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = 65535;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_src_port_max (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = 65535;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_dst_port_min (struct nsm_master *nm,
                               u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_dst_port_min (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_dst_port_max (struct nsm_master *nm,
                               u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = 65535;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_dst_port_max (struct nsm_master *nm,
                                    u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = 65535;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_protocol (struct nsm_master *nm,
                           u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = 255;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_protocol (struct nsm_master *nm,
                                u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = 255;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_ftn_act_type (struct nsm_master *nm,
                           u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;
  ftn_action_type_t act_type;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn)
    act_type = ftn->lsp_bits.bits.act_type;
  else if (ftn_temp)
    act_type = ftn_temp->act_type;
  else
    return NSM_API_GET_ERROR;

  if (act_type == REDIRECT_LSP)
    *ret = SNMP_REDIRECT_LSP;
  else if (act_type == REDIRECT_TUNNEL)
    *ret = SNMP_REDIRECT_TUNNEL;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_act_type (struct nsm_master *nm,
                                u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  ftn_action_type_t act_type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)
    act_type = ftn->lsp_bits.bits.act_type;
  else if (type == TYPE_TEMP)
    act_type = ftn_temp->act_type;
  else
    return NSM_API_GET_ERROR;

  if (act_type == REDIRECT_LSP)
    *ret = SNMP_REDIRECT_LSP;
  else if (act_type == REDIRECT_TUNNEL)
    *ret = SNMP_REDIRECT_TUNNEL;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_act_pointer (struct nsm_master *nm,
                              u_int32_t ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn)
    pal_snprintf (*ret,
                  NSM_MPLS_MAX_STRING_LENGTH,
                  "mplsXCTable.%d.%d.%d",
                  ftn->xc->key.xc_ix, 0, /* In Seg Idx is 0 */
                  ftn->xc->key.nhlfe_ix);
  else if (ftn_temp)
    pal_snprintf (*ret,
                  NSM_MPLS_MAX_STRING_LENGTH,
                  "mplsXCTable.%d.%d.%d",
                  ftn_temp->xc_ix, 0,
                  ftn_temp->nhlfe_ix);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_act_pointer (struct nsm_master *nm,
                                   u_int32_t *ftn_ix, char **ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)
    pal_snprintf (*ret,
                  NSM_MPLS_MAX_STRING_LENGTH,
                  "mplsXCTable.%d.%d.%d",
                  ftn->xc->key.xc_ix, 0,
                  ftn->xc->key.nhlfe_ix);
  else if (type == TYPE_TEMP)
    pal_snprintf (*ret,
                  NSM_MPLS_MAX_STRING_LENGTH,
                  "mplsXCTable.%d.%d.%d",
                  ftn_temp->xc_ix, 0,
                  ftn_temp->nhlfe_ix);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_dscp (struct nsm_master *nm,
                       u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_dscp (struct nsm_master *nm,
                            u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}


u_int32_t
nsm_gmpls_get_ftn_st_type (struct nsm_master *nm,
                          u_int32_t ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_gmpls_get_next_ftn_st_type (struct nsm_master *nm,
                               u_int32_t *ftn_ix, u_int32_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}


/* APIs related with MPLS FTN Performance table. */
u_int32_t
nsm_gmpls_get_ftn_mtch_pkts (struct nsm_master *nm,
                            u_int32_t ftn_ix, ut_int64_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    *ret = nsm_cnt_ftn_mtch_pkts (ftn);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_mtch_pkts (struct nsm_master *nm,
                                 u_int32_t *ftn_ix, ut_int64_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  u_int32_t type;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    *ret = nsm_cnt_ftn_mtch_pkts (ftn);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_mtch_octs (struct nsm_master *nm,
                            u_int32_t ftn_ix, ut_int64_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn || ftn_temp)
    *ret = nsm_cnt_ftn_mtch_octs (ftn);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_mtch_octs (struct nsm_master *nm,
                                 u_int32_t *ftn_ix, ut_int64_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;
  u_int32_t type;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN || type == TYPE_TEMP)
    *ret = nsm_cnt_ftn_mtch_octs (ftn);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_ftn_disc_time (struct nsm_master *nm,
                            u_int32_t ftn_ix, time_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;

  nsm_gmpls_ftn_entry_lookup (nm, ftn_ix, &fec, &ftn_temp, &ftn);

  if (ftn)
    *ret = nsm_cnt_ftn_disc_time (ftn);
  else if (ftn_temp)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_gmpls_get_next_ftn_disc_time (struct nsm_master *nm,
                                 u_int32_t *ftn_ix, time_t *ret)
{
  struct ftn_entry *ftn = NULL;
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct fec_gen_entry fec;
  u_int32_t type;

  nsm_gmpls_ftn_entry_lookup_next (nm, *ftn_ix, &fec, &ftn_temp, &ftn);

  /* Set Index value */
  nsm_gmpls_get_next_ftn_index_set (ftn, ftn_temp, ftn_ix, &type);

  if (type == TYPE_MAIN)
    *ret = nsm_cnt_ftn_disc_time (ftn);
  else if (type == TYPE_TEMP)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

#endif /* HAVE_SNMP */
