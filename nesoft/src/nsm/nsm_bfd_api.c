/* Copyright (C) 2008  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_BFD

#include "lib.h"
#include "prefix.h"
#include "if.h"
#include "nexthop.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_interface.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_bfd.h"
#include "nsm/nsm_api.h"
#include "nsm/nsm_bfd_api.h"

/**@brief - Enable BFD support on a specific static route 
 * @param vr_id - VR identifier
 * @param vrf_id - VRF identifier
 * @param p - Destination prefix for the static route
 * @param nh - Nexthop for the static route
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_route_bfd_set (u_int32_t vr_id, vrf_id_t vrf_id,
                        struct prefix_ipv4 *p, struct pal_in4_addr *nh)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct nsm_static *ns;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  ns = nsm_static_lookup (nv, AFI_IP, (struct prefix *) p,
                         (union nsm_nexthop *) nh); 

  if (! ns)
    return NSM_API_SET_ERR_NO_STATIC_ROUTE;
  
  if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET))
    return NSM_API_SET_ERR_STATIC_BFD_SET;

  if (!nsm_bfd_update_session_by_static(ns, nv, p, session_add))
    return NSM_API_SET_ERROR;

  SET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET);
  UNSET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET);

  return NSM_API_SET_SUCCESS;
}

/**@brief - Unset BFD support on a specific static route
 * @param vr_id - VR identifier
 * @param vrf_id - VRF identifier
 * @param p - Destination prefix for the static route
 * @param nh - Nexthop for the static route
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_route_bfd_unset (u_int32_t vr_id, vrf_id_t vrf_id,
                          struct prefix_ipv4 *p, struct pal_in4_addr *nh)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct nsm_static *ns;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  ns = nsm_static_lookup (nv, AFI_IP, (struct prefix *) p,
                         (union nsm_nexthop *) nh);
  if (! ns)
    return NSM_API_SET_ERROR;

  if (!CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET))
    return NSM_API_SET_ERR_STATIC_BFD_UNSET;

  if (!nsm_bfd_update_session_by_static(ns, nv, p, session_del))
    return NSM_API_SET_ERROR;

  UNSET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET);

  return NSM_API_SET_SUCCESS;

}

/**@brief - Disable BFD support on a specific static route
 * @param vr_id - VR identifier
 * @param vrf_id - VRF identifier
 * @param p - Destination prefix for the static route
 * @param nh - Nexthop for the static route
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_route_bfd_disable_set (u_int32_t vr_id, vrf_id_t vrf_id,
                        struct prefix_ipv4 *p, struct pal_in4_addr *nh)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct nsm_static *ns;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  ns = nsm_static_lookup (nv, AFI_IP, (struct prefix *) p,
                         (union nsm_nexthop *) nh);
  if (! ns)
    return NSM_API_SET_ERR_NO_STATIC_ROUTE;

  if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET))
    return NSM_API_SET_ERR_STATIC_BFD_DISABLE_SET;

  if (!nsm_bfd_update_session_by_static(ns, nv, p, session_disable))
    return NSM_API_SET_ERROR;

  SET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET);
  UNSET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET);

  return NSM_API_SET_SUCCESS;
}

/**@brief - Reset BFD disable on a specific static route
 * @param vr_id - VR identifier
 * @param vrf_id - VRF identifier
 * @param p - Destination prefix for the static route
 * @param nh - Nexthop for the static route
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_route_bfd_disable_unset (u_int32_t vr_id, vrf_id_t vrf_id,
                          struct prefix_ipv4 *p, struct pal_in4_addr *nh)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct nsm_static *ns;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  ns = nsm_static_lookup (nv, AFI_IP, (struct prefix *) p,
                         (union nsm_nexthop *) nh);
  if (! ns)
    return NSM_API_SET_ERROR;

  if (!CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET))
    return NSM_API_SET_ERR_STATIC_BFD_DISABLE_UNSET;

  if (!nsm_bfd_update_session_by_static(ns, nv, p, session_no_disable))
    return NSM_API_SET_ERROR;

  UNSET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET);

  return NSM_API_SET_SUCCESS;

}

/**@brief -  Enable static route bfd support on an interface
 * @param vr_id - VRF identifier
 * @param ifname - interface name
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int 
nsm_ipv4_if_bfd_static_set (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct interface *ifp;
  struct nsm_if  *zif;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (CHECK_FLAG (zif->flags, NSM_IF_BFD))
    return NSM_API_SET_ERR_STATIC_BFD_SET;

  SET_FLAG (zif->flags, NSM_IF_BFD);

  /* If enabled globally session already exists */ 
  if ((CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4)) &&
      !(CHECK_FLAG (zif->flags, NSM_IF_BFD_DISABLE)))
    return NSM_API_SET_SUCCESS;
 
  UNSET_FLAG (zif->flags, NSM_IF_BFD_DISABLE);

  if (!nsm_bfd_update_session_by_interface (nv, AFI_IP, ifp, NULL, session_add))
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

/**@brief -  Unset static route bfd support on an interface
 * @param vr_id - VRF identifier
 * @param ifname - interface name
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int 
nsm_ipv4_if_bfd_static_unset (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct interface *ifp;
  struct nsm_if  *zif;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (!CHECK_FLAG (zif->flags, NSM_IF_BFD))
    return NSM_API_SET_ERR_STATIC_BFD_UNSET;

  UNSET_FLAG (zif->flags, NSM_IF_BFD);

  /* If enabled globally do not delete the session */
  if (CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4))
    return NSM_API_SET_SUCCESS;

  if (!nsm_bfd_update_session_by_interface (nv, AFI_IP, ifp, NULL, session_del))
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

/**@brief -  Disable static route bfd support on an interface
 * @param vr_id - VRF identifier
 * @param ifname - interface name
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_if_bfd_static_disable_set (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct interface *ifp;
  struct nsm_if  *zif;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (CHECK_FLAG (zif->flags, NSM_IF_BFD_DISABLE))
    return NSM_API_SET_ERR_STATIC_BFD_DISABLE_SET;

  SET_FLAG (zif->flags, NSM_IF_BFD_DISABLE);

  /* If BFD not enabled globally or on interface no session to delete */
  if (!(CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4)) &&
      !(CHECK_FLAG (zif->flags, NSM_IF_BFD)))
    {
      UNSET_FLAG (zif->flags, NSM_IF_BFD);
      return NSM_API_SET_SUCCESS;
    }

  UNSET_FLAG (zif->flags, NSM_IF_BFD);
  if (!nsm_bfd_update_session_by_interface (nv, AFI_IP, ifp, NULL, session_del))
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

/**@brief -  Unset disable static route bfd support on an interface
 * @param vr_id - VRF identifier
 * @param ifname - interface name
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_if_bfd_static_disable_unset (u_int32_t vr_id, char *ifname)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct interface *ifp;
  struct nsm_if  *zif;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  zif = (struct nsm_if *)ifp->info;
  if (zif == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (!CHECK_FLAG (zif->flags, NSM_IF_BFD_DISABLE))
    return NSM_API_SET_ERR_STATIC_BFD_DISABLE_UNSET;

  UNSET_FLAG (zif->flags, NSM_IF_BFD_DISABLE);

  /* If BFD not enabled globally or on interface no session to add */
  if (!(CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4)) &&
      !(CHECK_FLAG (zif->flags, NSM_IF_BFD)))
    return NSM_API_SET_SUCCESS;

  if (!nsm_bfd_update_session_by_interface (nv, AFI_IP, ifp, NULL, session_add))
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

/**@brief - Enable static route bfd support on all interfaces
 * @param vr_id - VRF identifier
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_if_bfd_static_set_all (u_int32_t vr_id)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct interface *ifp;
  struct nsm_if  *zif;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  struct listnode *node;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;
  
  if ((CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4)))
    return NSM_API_SET_ERR_STATIC_BFD_SET;

  SET_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4);

  LIST_LOOP (nm->vr->ifm.if_list, ifp, node)
    {
      zif = (struct nsm_if *)ifp->info;
      /* If no BFD interface level configurations add the session */
      if (!CHECK_FLAG (zif->flags, NSM_IF_BFD_DISABLE) &&
           !CHECK_FLAG (zif->flags, NSM_IF_BFD))
        if (!nsm_bfd_update_session_by_interface (nv, AFI_IP, ifp, NULL,
                                                           session_add))
          return NSM_API_SET_ERROR;
    }
  return NSM_API_SET_SUCCESS;
}

/**@brief - Unset static route bfd support on all interfaces
 * @param vr_id - VRF identifier
 * @return - Returns NSM_API_SET_SUCCESS on success else the ERROR value
 * */

int
nsm_ipv4_if_bfd_static_unset_all (u_int32_t vr_id)
{
  struct nsm_master *nm;
  struct nsm_vrf *nv;
  struct interface *ifp;
  struct nsm_if  *zif;
  vrf_id_t vrf_id = VRF_ID_UNSPEC;
  struct listnode *node;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nv = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (nv == NULL)
    return NSM_API_SET_ERR_VRF_NOT_EXIST;

  if (!(CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4))) 
    return NSM_API_SET_ERR_STATIC_BFD_UNSET;

  UNSET_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4);

  LIST_LOOP (nm->vr->ifm.if_list, ifp, node)
    {
      zif = (struct nsm_if *)ifp->info;
      /* If no BFD interface level configurations delete the session */
      if (!CHECK_FLAG (zif->flags, NSM_IF_BFD_DISABLE) &&
           !CHECK_FLAG (zif->flags, NSM_IF_BFD))
        if (!nsm_bfd_update_session_by_interface (nv, AFI_IP, ifp, NULL,
                                                           session_del))
          return NSM_API_SET_ERROR;
    }

  return NSM_API_SET_SUCCESS;

}

#endif /* HAVE_BFD */
