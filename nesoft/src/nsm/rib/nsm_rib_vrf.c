/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "prefix.h"
#include "linklist.h"
#include "table.h"
#include "if.h"
#include "log.h"
#include "sockunion.h"

#include "nsmd.h"
#include "nsm_vrf.h"
#include "rib/rib.h"
#include "rib/nsm_rib_vrf.h"
#include "nsm_api.h"

#ifdef HAVE_VRF
/* API. */
int
nsm_ip_vrf_set (struct apn_vr *vr, char *name)
{
  struct nsm_master *nm = vr->proto;
  struct nsm_vrf *nv;

  if ((pal_strlen (name)) >= MAX_VRF_NAMELEN)
    return NSM_API_SET_ERR_VRF_NAME_TOO_LONG;

  nv = nsm_vrf_get_by_name (nm, name);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_CANT_CREATE;

  return NSM_API_SET_SUCCESS;
}

/* API. */
int
nsm_ip_vrf_unset (struct apn_vr *vr, char *name)
{
  struct nsm_master *nm = vr->proto;
  struct nsm_vrf *nv;

  nv = nsm_vrf_lookup_by_name (nm, name);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  /* Cleanup the VRF.  */
  nsm_vrf_destroy (vr, nv);

  return NSM_API_SET_SUCCESS;
}
#endif /* HAVE_VRF */
