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
#include "lib_mtrace.h"

#ifdef HAVE_MPLS
#include "mpls.h"
#endif /* HAVE_MPLS */

#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#include "nsmd.h"
#ifdef HAVE_L3
#include "rib.h"
#include "nsm_rt.h"
#include "rib/nsm_table.h"
#endif /* HAVE_L3 */
#include "nsm_server.h"
#include "nsm_interface.h"
#ifdef HAVE_L3
#include "nsm_server_rib.h"
#include "nsm_nexthop.h"
#include "nsm_redistribute.h"
#endif /* HAVE_L3 */
#include "nsm_debug.h"
#include "nsm_api.h"
#include "nsm_fea.h"
#ifdef HAVE_MPLS
#include "nsm_mpls.h"
#endif /* HAVE_MPLS */
#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#ifdef HAVE_L3
#include "nsm_static_mroute.h"
#endif /* HAVE_L3 */
#ifdef HAVE_MCAST_IPV6
#include "nsm_mcast6.h"
#endif /* HAVE_MCAST_IPV6 */
#include "ptree.h"

#ifdef HAVE_MCAST_IPV4
#include <nsm_mcast4_incl.h>
#endif
#ifdef HAVE_MCAST_IPV6
#include <nsm_mcast6_incl.h>
#endif

#ifdef HAVE_HA
#include "nsm_cal.h"
#ifdef HAVE_L3
#include "nsm_rib_cal.h"
#endif /* HAVE_L3 */
#endif /* HAVE_HA */
#ifdef HAVE_BFD
#include "nsm_bfd.h"
#endif /* HAVE_BFD */


/* Associate Kernel FIB-ID and VRF-ID. */
fib_id_t
nsm_fib_id_get (struct lib_globals *zg)
{
  fib_id_t id;

  for (id = 0; id < vector_max (zg->fib2vrf); id++)
    if ((vector_slot (zg->fib2vrf, id)) == NULL)
      return id;

  return id;
}

result_t
nsm_fib_release (struct lib_globals *zg, fib_id_t id)
{
  vector_unset (zg->fib2vrf, id);

  return 0;
}

/* Should be move to lib.c XXX-VR */
void
nsm_vrf_set_by_fib_id (struct lib_globals *zg,
                       struct apn_vrf *vrf, fib_id_t id)
{
  vrf->fib_id = id;
  vector_set_index (zg->fib2vrf, id, vrf);
}


struct nsm_vrf *
nsm_vrf_lookup_by_id (struct nsm_master *nm, vrf_id_t id)
{
  struct apn_vr *vr = nm->vr;
  struct apn_vrf *vrf;

  if (vector_max (vr->vrf_vec) > id)
    {
      vrf = apn_vrf_lookup_by_id (vr, id);
      if (vrf != NULL)
        return vrf->proto;
    }

  return NULL;
}

struct nsm_vrf *
nsm_vrf_lookup_by_fib_id (struct lib_globals *zg, fib_id_t fib_id)
{
  struct apn_vrf *vrf;

  if (vector_max (zg->fib2vrf) > fib_id)
    {
      vrf = vector_slot (zg->fib2vrf, fib_id);
      if (vrf != NULL)
        return vrf->proto;
    }
  return NULL;
}

struct nsm_vrf *
nsm_vrf_lookup_by_name (struct nsm_master *nm, char *name)
{
  struct apn_vr *vr = nm->vr;
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  int i;

  for (i = 0; i < vector_max (vr->vrf_vec); i++)
    if ((iv = vector_slot (vr->vrf_vec, i)))
      if ((nv = iv->proto) != NULL)
        {
          if (name == NULL)
            {
              if (iv->name == NULL)
                return nv;
            }
          else
            {
              if (iv->name != NULL)
                if (pal_strcmp (name, iv->name) == 0)
                  return nv;
            }
        }

  return NULL;
}

struct nsm_vrf *
nsm_vrf_new (struct apn_vr *vr, char *name)
{
  struct nsm_vrf *nv;
  struct apn_vrf *iv;
  fib_id_t fib_id;
#ifdef HAVE_L3
  afi_t afi;
  safi_t safi;
  result_t ret = RESULT_OK;
  int i;
  u_int16_t max_key_len = 0;
  u_int16_t ptype = 0;
#endif /* HAVE_L3 */
  struct apn_vr *pvr;

  pvr = apn_vr_get_privileged (vr->zg);

  /* First check if there is available FIB. */
  fib_id = nsm_fib_id_get (vr->zg);
  if (fib_id > FIB_ID_MAX)
    {
      zlog_err (NSM_ZG, "Can't get FIB");
      return NULL;
    }

  /* Create NSM VRF. */
  nv = XCALLOC (MTYPE_NSM_VRF, sizeof (struct nsm_vrf));
  if (nv == NULL)
    return NULL;

  /* Get Lib VRF, and associate NSM-VRF and Lib-VRF.
     Register the VRF to global vector. */
  iv = apn_vrf_get_by_name (vr, name);
  iv->id = vector_set (vr->vrf_vec, iv);
  iv->proto = nv;
  nv->vrf = iv;
  nv->nm = vr->proto;
  nsm_vrf_set_by_fib_id (vr->zg, iv, fib_id);

#ifdef HAVE_HA
  nsm_cal_create_nsm_vrf (nv);
#endif /* HAVE_HA */

#ifdef HAVE_L3
  /* Initialize tables. */
  NSM_AFI_LOOP (afi)
    {
      if (afi == AFI_IP)
        {
          max_key_len = IPV4_MAX_PREFIXLEN;
          ptype = AF_INET;
        }
#ifdef HAVE_IPV6
      else if (afi == AFI_IP6)
        {
          max_key_len = IPV6_MAX_PREFIXLEN;
          ptype = AF_INET6;
        }
#endif /* HAVE_IPV6 */
      else
        continue;

      for (safi = SAFI_UNICAST; safi < SAFI_MAX; safi++)
        {
          if ((nv->afi[afi].rib[safi] = nsm_ptree_table_init (max_key_len))
               != NULL)
            nv->afi[afi].rib[safi]->ptype = ptype;
        }

      for (i = 0; i < APN_ROUTE_MAX; i++)
        nv->afi[afi].redist[i] = vector_init (1);

      nv->afi[afi].ip_static = ptree_init (max_key_len);
      nv->afi[afi].ip_mroute = ptree_init (max_key_len);
      if ((nv->afi[afi].nh_reg = nsm_ptree_table_init (max_key_len))
           != NULL)
        nv->afi[afi].nh_reg->ptype = ptype;

      if ((nv->afi[afi].nh_reg_m = nsm_ptree_table_init (max_key_len))
           != NULL)
        nv->afi[afi].nh_reg_m->ptype = ptype;
    }

#ifdef HAVE_BFD
    for (i = 0; i < NSM_BFD_ADDR_FAMILY_MAX; i++)
      if (avl_create (&nv->nsm_bfd_session[i] , 0, nsm_bfd_session_cmp) < 0)
        zlog_err (vr->zg, "OS: Creating BFD session tree for %s VRF failed",
                  name == NULL ? "default" : name);
#endif /* HAVE_BFD */

  /* Pass new VR indication only for non-default VR */
  /* A new VR is being created when a default VRF
   * (name == NULL) is created
   */
  /* Create FIB in dataplane. */
  if (! IS_APN_VRF_PRIV_DEFAULT (iv))
   {
     if ((vr != pvr) && (name == NULL))
       ret = nsm_fea_fib_create (fib_id, PAL_TRUE, vr->name);
     else
       ret = nsm_fea_fib_create (fib_id, PAL_FALSE, vr->name);
   }

  if (ret != RESULT_OK)
    if (IS_NSM_DEBUG_KERNEL)
      zlog_err (vr->zg, "OS: Creating FIB for %s VRF failed",
                name == NULL ? "default" : name);

#endif /* HAVE_L3 */
  return nv;
}

/* Send NSM_MSG_VRF_ADD to NSM client.   */
void
nsm_vrf_add_update (struct nsm_master *nm, struct nsm_vrf *nv)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  if (nv->vrf->id == VRF_ID_DISABLE)
    return;

  /* Default VRF Add message is sent to all the PMs.  */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (IS_APN_VRF_DEFAULT (nv->vrf)
        || nsm_service_check (nse, NSM_SERVICE_VRF))
      nsm_server_send_vrf_add (nse, nv->vrf);
}

struct nsm_vrf *
nsm_vrf_get_by_name (struct nsm_master *nm, char *name)
{
  struct nsm_vrf *nv;
#if defined (HAVE_MPLS) && defined (HAVE_VRF)
  int ret;
#endif /* HAVE_MPLS && HAVE_VRF */

  nv = nsm_vrf_lookup_by_name (nm, name);
  if (nv != NULL)
    return nv;

  nv = nsm_vrf_new (nm->vr, name);
  if (nv == NULL)
    return NULL;

#if defined (HAVE_MPLS) && defined (HAVE_VRF)
  /* Install in MPLS Forwarder. */
  if (NSM_CAP_HAVE_MPLS)
    ret = nsm_mpls_vrf_add (nm, nv->vrf->id);
#endif /* HAVE_MPLS && HAVE_VRF */

#ifdef HAVE_MCAST_IPV4
  nsm_mcast_init (nv);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  nsm_mcast6_init (nv);
#endif /* HAVE_MCAST_IPV6 */

  nsm_vrf_add_update (nm, nv);

  return nv;
}

/* Send NSM_MSG_VRF_DELETE to NSM client. */
static int
nsm_vrf_delete_update (struct nsm_vrf *nv)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nsm_redistribute *redist, *next = NULL;
  int i;

  if (nv->vrf->id == VRF_ID_DISABLE)
    return 0;

  /* 1. Inform all VRF capable clients that this VRF is deleted
     2. Delete redistribution information referring to this VRF. */
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    {
      /* Default VRF Delete message is sent to all the PMs.  */
      if (! IS_APN_VRF_PRIV_DEFAULT (nv->vrf))
        if (IS_APN_VRF_DEFAULT (nv->vrf)
            || nsm_service_check (nse, NSM_SERVICE_VRF))
          nsm_server_send_vrf_delete (nse, nv->vrf);

      for (redist = nse->redist; redist; redist = next)
        {
          next = redist->next;
          if (redist->vrf_id == nv->vrf->id)
            {
              SLIST_DEL (nse, redist, redist);
              XFREE (MTYPE_NSM_REDISTRIBUTE, redist);
            }
        }
    }

  return 0;
}

int
nsm_vrf_destroy (struct apn_vr *vr, struct nsm_vrf *nv)
{
#ifdef HAVE_VRF
  struct nsm_master *nm = vr->proto;
#endif /* HAVE_VRF */
  struct apn_vrf *iv = nv->vrf;
  struct interface *ifp;
  struct route_node *rn;
#ifdef HAVE_L3
  safi_t safi;
  afi_t afi;
  int i;
#endif /* HAVE_L3 */

#ifdef HAVE_HA
  nsm_cal_delete_nsm_vrf (nv);
#endif /* HAVE_HA */

  /* Free routing table assigned for this VRF.  */
  nsm_fib_release (NSM_ZG, iv->fib_id);

  /* Unbind interface from this VRF.  */
  if (! IS_APN_VRF_DEFAULT (iv))
    for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
      if ((ifp = rn->info))
        nsm_if_unbind_vrf (ifp, nv);

  /* Deinit mcast before sending VRF delete, so protocols get
   * chance to clean_up
   */
#ifdef HAVE_MCAST_IPV4
  nsm_mcast_deinit (nv);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  nsm_mcast6_deinit (nv);
#endif /* HAVE_MCAST_IPV6 */

  /* Send VRF delete message to PMs.  */
  nsm_vrf_delete_update (nv);

#if defined (HAVE_MPLS) && defined (HAVE_VRF)
  /* Remove from MPLS Forwarder. */
  if (NSM_CAP_HAVE_MPLS)
    if (! IS_APN_VRF_DEFAULT (iv))
      nsm_mpls_vrf_del (nm, iv->id);
#endif /* HAVE_MPLS && HAVE_VRF */

#ifdef HAVE_BFD
  for (i = 0; i < NSM_BFD_ADDR_FAMILY_MAX; i++)
    {
      if (nv->nsm_bfd_session[i])
        {
          avl_tree_free (&nv->nsm_bfd_session[i], NULL);
        }
    }
#endif /* HAVE_BFD */

#ifdef HAVE_L3
  NSM_AFI_LOOP (afi)
    {
      /* Routing table. */
      for (safi = SAFI_UNICAST; safi < SAFI_MAX; safi++)
        {
          if (safi == SAFI_MULTICAST)
            {
              nsm_mrib_table_free (nv->afi[afi].rib[safi]);
            }
          else
            {
              nsm_nexthop_rib_clean_all (nv->afi[afi].rib[safi]);
              nsm_rib_table_free (nv->afi[afi].rib[safi]);
            }
        }

      nsm_static_table_clean_all (nv, nv->afi[afi].ip_static);

      nsm_ip_mroute_table_clean_all (nv->afi[afi].ip_mroute);

      /* Nexthop tables.  */
      nsm_nexthop_nh_table_clean_all (nv->afi[afi].nh_reg);

      nsm_multicast_nh_reg_table_clean_all (nv->afi[afi].nh_reg_m);

      /* Redistribute vector.  */
      for (i = 0; i < APN_ROUTE_MAX; i++)
        vector_free (nv->afi[afi].redist[i]);

#ifdef HAVE_HA
      if (nv->afi[afi].t_rib_update != NULL)
        nsm_cal_delete_rib_update_timer (&nv->afi[afi], nv);
#endif /* HAVE_HA */

      THREAD_TIMER_OFF (nv->afi[afi].t_rib_update);
    }

#ifdef HAVE_HA
  if (nv->t_router_id)
    nsm_cal_delete_rtr_id_timer (nv);
#endif /* HAVE_HA */
  THREAD_TIMER_OFF (nv->t_router_id);

  /* Cleanup the actual FIB.  */
  if (! IS_APN_VRF_PRIV_DEFAULT (iv))
    if (nsm_fea_fib_delete (iv->fib_id) != RESULT_OK)
      if (IS_NSM_DEBUG_KERNEL)
        zlog_err (vr->zg, "OS: Deleting FIB for %s VRF failed",
                  iv->name == NULL ? "default" : iv->name);
#endif /* HAVE_L3 */

  /* Delete APN VRF.  */
  /* Don't delete the default VRF as it is done
   * when the VR is deleted
   */
  if (! IS_APN_VRF_DEFAULT (iv))
    apn_vrf_delete (iv);

  /* Free VRF description.  */
  if (nv->desc)
    XFREE (MTYPE_NSM_DESC, nv->desc);


  XFREE (MTYPE_NSM_VRF, nv);

  return 0;
}

