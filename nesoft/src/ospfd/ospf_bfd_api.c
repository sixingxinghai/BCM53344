/* Copyright (C) 2008  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_BFD

#include "lib.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_bfd.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_bfd_api.h"

/* Set the BFD fall-over check for neighbors on specificed interface.  */
int
ospf_if_bfd_set (u_int32_t vr_id, char *ifname)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ospf_interface *oi;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, ifname);
  ospf_if_params_bfd_set (oip);

  oi = ospf_if_lookup_by_params (oip);
  if (oi)
    ospf_bfd_if_update (oi, PAL_FALSE);

  return OSPF_API_SET_SUCCESS;
}

/* Unset the BFD fall-over check for neighbors on specificed interface.  */
int
ospf_if_bfd_unset (u_int32_t vr_id, char *ifname)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ospf_interface *oi;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, ifname);
  if (oip)
    {
      ospf_if_params_bfd_unset (oip);
      oi = ospf_if_lookup_by_params (oip);
      if (oi)
	ospf_bfd_if_update (oi, PAL_TRUE);

      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, ifname);
    }
  return OSPF_API_SET_SUCCESS;
}

/* Disable the BFD fall-over check for neighbors on specificed interface.  */
int
ospf_if_bfd_disable_set (u_int32_t vr_id, char *ifname)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ospf_interface *oi;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, ifname);
  ospf_if_params_bfd_disable_set (oip);

  oi = ospf_if_lookup_by_params (oip);
  if (oi)
    ospf_bfd_if_update (oi, PAL_TRUE);

  return OSPF_API_SET_SUCCESS;
}

/* Unset the disable flag of BFD fall-over check for neighbors
   on specified interface.  */
int
ospf_if_bfd_disable_unset (u_int32_t vr_id, char *ifname)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ospf_interface *oi;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, ifname);
  if (oip)
    {
      ospf_if_params_bfd_disable_unset (oip);
      oi = ospf_if_lookup_by_params (oip);

      if (oi)
	ospf_bfd_if_update (oi, PAL_FALSE);

      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, ifname);
    }
  return OSPF_API_SET_SUCCESS;
}

/* OSPF process mode.  */

/* Set the BFD fall-over check for all the neighbors
   under specified process. */
int
ospf_bfd_all_interfaces_set (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (CHECK_FLAG (top->config, OSPF_CONFIG_BFD))
    return OSPF_API_SET_SUCCESS;

  SET_FLAG (top->config, OSPF_CONFIG_BFD);
  ospf_bfd_update (top, PAL_FALSE);

  return OSPF_API_SET_SUCCESS;
}

/* Unset the BFD fall-over check for all the neighbors
   under specified process. */
int
ospf_bfd_all_interfaces_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (! CHECK_FLAG (top->config, OSPF_CONFIG_BFD))
    return OSPF_API_SET_SUCCESS;

  UNSET_FLAG (top->config, OSPF_CONFIG_BFD);
  ospf_bfd_update (top, PAL_TRUE);

  return OSPF_API_SET_SUCCESS;
}

/* Set the BFD fall-over check for the specified VLINK neighbor.  */
int
ospf_vlink_bfd_set (u_int32_t vr_id, int proc_id,
                    struct pal_in4_addr area_id,
                    struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_bfd_set (vr_id, vlink->name);
}

/* Unset the BFD fall-over check for the specified VLINK neighbor.  */
int
ospf_vlink_bfd_unset (u_int32_t vr_id, int proc_id,
                      struct pal_in4_addr area_id,
                      struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_bfd_unset (vr_id, vlink->name);
}

#endif /* HAVE_BFD */
