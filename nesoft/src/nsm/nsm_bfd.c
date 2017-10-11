/* Copyright (C) 2008  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_BFD

#include "lib.h"
#include "bfd_client_api.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_vrf.h"
#include "nsm/nsm_interface.h"
#include "nsm/rib/rib.h"
#include "nsm/rib/nsm_nexthop.h"
#include "nsm/nsm_bfd.h"
#include "nexthop.h"
#include "nsm/nsm_debug.h"
#include "lib/avl_tree.h"
#include "lib/bfd_client.h" 

#ifdef HAVE_MPLS_OAM
#include "nsm_mpls_bfd.h"
#endif /* HAVE_MPLS_OAM */


/**@brief - Called from CLI command to enable BFD static route support 
 * on an interface NSM_IF_BFD is already modified from the caller 
 *
 * @param vrf - VRF instance
 * @param afi - address family
 * @param ifp - interface structure
 * @param del - flag to indicate whether to add/delete session.
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */

bool_t
nsm_bfd_update_session_by_interface (struct nsm_vrf *vrf, afi_t afi,
                                     struct interface *ifp,
                                     struct connected *ifc, int del)
{
  struct ptree_node *rn;
  struct nsm_ptree_node *prn = NULL;
  struct nsm_static *ns;
  struct rib *rib = NULL;
  struct prefix p;
  struct nsm_vrf_afi *nva;
  union nsm_nexthop *sgate;
  struct nexthop *nexthop;
  u_int32_t sifindex;
  struct nsm_master *nm;

  /* For each static route having active nexthop through the interface 
     add a bfd session */
  for (rn = ptree_top (vrf->afi[afi].ip_static);
                          rn; rn = ptree_next (rn))
    for (ns = rn->info; ns; ns = ns->next)
      {
        /* Configuration specific to static route takes more preference */
        if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET) ||
            CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET))
          continue;
        /* Obtain RIB entry to get the actual interface and direct nexthop */
        nva = &vrf->afi[ns->afi];
        /* Get the prefix information */
        p.prefixlen = rn->key_len;
        if (afi == AFI_IP)
         {
           p.family = AF_INET;
           NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
           prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], 
                                        (u_char *)&p.u.prefix4, p.prefixlen);
         }
#ifdef HAVE_IPV6
        else if (afi == AFI_IP6)
         {
           p.family = AF_INET6;
           NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
           prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], 
                                        (u_char *)&p.u.prefix6, p.prefixlen);
         }
#endif /* HAVE_IPV6 */ 
       if (!prn)
         continue;
       if ((rib = nsm_rib_static_lookup (prn->info, ns)) == NULL)
         continue;

       for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
        {
          if (afi == AFI_IP)
            {
              if ((nexthop->type != NEXTHOP_TYPE_IPV4) && 
                   (nexthop->type != NEXTHOP_TYPE_IPV4_IFINDEX))
                continue;
              if (IPV4_ADDR_CMP(&ns->gate.ipv4, &nexthop->gate.ipv4))
                continue;
            }
          /* TBD check to be added for IPV6 */
 
           if (!del && ! (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
               && CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)) )
                 continue;

           /* Verify if the nexthop ifindex matches the interface index */
           if ((nexthop->ifindex == ifp->ifindex) ||
                (nexthop->rifindex == ifp->ifindex))
            {
              if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                {
                  sgate = (union nsm_nexthop *)&nexthop->rgate;
                  sifindex = nexthop->rifindex;
                }
              else
                {
                  sgate = (union nsm_nexthop *)&nexthop->gate;
                  sifindex = nexthop->ifindex;
                }
                       
              if (sifindex == 0)
               sifindex = nexthop->bfd_ifindex;

              if (! nsm_bfd_session_update (vrf, afi, sgate, ifp, ifc,
                   ((del == session_del ? NSM_BFD_STATIC_SESSION_ADMIN_DEL :
                     NSM_BFD_STATIC_SESSION_ADD))))
                {
                  return PAL_FALSE;
                }
              else if ((del) && 
                        CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE))
                {
                   nm = nsm_master_lookup_by_id (nzg, 0);
                   UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE);
                   /* Set the flag to indicate that rib processing is triggered
                    * from BFD, so that it should not again lead to 
                    * session add/delete */
                   SET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
                   nsm_rib_process(nm, prn, 0);
                   /* Reset the flag indicating BFD event change. Only then RIB
                      processing triggered from other events would add/delete 
                      BFD session */
                   UNSET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
                }
            }
          }

        }
  
    return PAL_TRUE;
}

/**@brief - Called from command "<no> static route A.B.C.D/N X.Y.Z.W 
 * fall-over bfd
 *
 * @param ns - static route config structure
 * @param vrf - VRF instance
 * @param p - Destination prefix for the static route
 * @param del - Flag if set indicates BFD session should be deleted, else added
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */

bool_t
nsm_bfd_update_session_by_static (struct nsm_static *ns, struct nsm_vrf *vrf,
                                  struct prefix_ipv4 *p, int flag )
{
  struct nsm_ptree_node *prn = NULL;
  struct rib *rib = NULL;
  struct nsm_vrf_afi *nva;
  union nsm_nexthop *sgate;
  struct nexthop *nexthop;
  u_int32_t sifindex;
  struct nsm_master *nm;
  struct interface *ifp;
  bool_t session_exists;
  struct nsm_if *nif;
  int session_flag = -1;

  /* Obtain RIB entry to get the actual interface and direct nexthop */
  nva = &vrf->afi[ns->afi];
  /* Get the prefix information */
  if (ns->afi == AFI_IP)
    {
      p->family = AF_INET;
      prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], 
                                  (u_char *)&p->prefix, p->prefixlen);
    }
#ifdef HAVE_IPV6
  else if (ns->afi == AFI_IP6)
    {
      p->family = AF_INET6;
      prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], 
                                  (u_char *)&p->prefix, p->prefixlen);
    }
#endif /* HAVE_IPV6 */ 
  if (!prn)
    return PAL_FALSE;
  if ((rib = nsm_rib_static_lookup (prn->info, ns)) == NULL)
    return PAL_FALSE;

  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    {
      if (ns->afi == AFI_IP)
        {
          if ((nexthop->type != NEXTHOP_TYPE_IPV4) && 
              (nexthop->type != NEXTHOP_TYPE_IPV4_IFINDEX))
            continue;
          if (IPV4_ADDR_CMP(&ns->gate.ipv4, &nexthop->gate.ipv4))
            continue;
        }

      if ((flag == session_add) || (flag == session_no_disable))
        if (! (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
            && CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)) )
           continue;

      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
        {
          sgate = (union nsm_nexthop *)&nexthop->rgate;
          sifindex = nexthop->rifindex;
        }
      else
        {
          sgate = (union nsm_nexthop *)&nexthop->gate;
          sifindex = nexthop->ifindex;
        }

      if (sifindex == 0)
        sifindex = nexthop->bfd_ifindex;

      nm = nsm_master_lookup_by_id (nzg, 0);
      if (!nm)
        return PAL_FALSE;

      ifp = if_lookup_by_index (&nm->vr->ifm, sifindex);
          
      if (ifp && (nif = ifp->info))
        session_exists = nsm_bfd_static_config_exists(vrf, nif, ns);
      else
        return PAL_FALSE;
          
      /* Verify configuration at all levels. Check if session to be updated */
      if ((flag == session_disable) && session_exists)
        session_flag = session_del;
      else if (flag == session_no_disable)
        {
          UNSET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET);            
          if (nsm_bfd_static_config_exists(vrf, nif, ns))
             session_flag = session_add;
        }
      else if ((flag == session_add) && !session_exists)
        session_flag = session_add;
      else if (flag == session_del)
        {
          UNSET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET);            
          if (!nsm_bfd_static_config_exists(vrf, nif, ns))
            session_flag = session_del;
        }

      if (session_flag < 0)
        return PAL_TRUE;

      if (! nsm_bfd_session_update (vrf, ns->afi, sgate, ifp, NULL,
           ((session_flag == session_del) ? NSM_BFD_STATIC_SESSION_ADMIN_DEL :
            NSM_BFD_STATIC_SESSION_ADD)))
        {
          return PAL_FALSE;
        }
      else
        {
          if ((session_flag == session_del) || 
              (session_flag == session_disable))
            if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE))
              {
                UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE);
                SET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
                nsm_rib_process(nm, prn, 0);
                UNSET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
              }
            }
          break;
        }/* for loop */

  return PAL_TRUE;

}

bool_t
nsm_bfd_static_config_exists(struct nsm_vrf *vrf, struct nsm_if *nif, 
                             struct nsm_static *ns) 
{

  if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET))
    return PAL_TRUE;
  if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET))
    return PAL_FALSE;

  if (CHECK_FLAG (nif->flags, NSM_IF_BFD))
    return PAL_TRUE;
  if (CHECK_FLAG (nif->flags, NSM_IF_BFD_DISABLE))
    return PAL_FALSE;

  if (CHECK_FLAG (vrf->config, NSM_VRF_CONFIG_BFD_IPV4))
    return PAL_TRUE;

  return PAL_FALSE;
}

/**@brief - Called from RIB processing. When static route RIB is modified 
 * from any event other than BFD, it calls this function to add or delete 
 * sessions based on the new nexthops 
 *
 * @param vrf - VRF instance
 * @param rib - RIB entry 
 * @param afi - address family
 * @param del - flag to indicate whether to add/delete session.
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */
bool_t
nsm_bfd_update_session_by_rib (struct nsm_vrf *vrf, struct rib *rib, afi_t afi, 
                               struct connected *ifc, struct nsm_ptree_node *rn, 
                                                   u_int32_t p_ifindex, int del)
{
    union nsm_nexthop *sgate;
    struct nexthop *nexthop;
    u_int32_t sifindex;
    struct apn_vr *vr = vrf->vrf->vr;
    struct interface *ifp;
    struct nsm_if *nif;
    struct pal_in4_addr addr;
    int delete_flag;

    pal_mem_set (&addr, 0, sizeof(struct pal_in4_addr));

    /* If RIB update is triggered from BFD itself dont do anything */
    if ( rib && CHECK_FLAG (rib->pflags, NSM_ROUTE_CHG_BFD))
      {
        UNSET_FLAG (rib->pflags, NSM_ROUTE_CHG_BFD);
        return PAL_TRUE; 
      }

    /* Update BFD session entry for all the nexthops in the RIB */
    for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
      {
        if (afi == AFI_IP)
          if ((nexthop->type != NEXTHOP_TYPE_IPV4) && 
              (nexthop->type != NEXTHOP_TYPE_IPV4_IFINDEX))
            continue;
        /* TBD check to be added for IPV6 */

        if (!del && ! (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB)
              && CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)) )
          continue;

        if (IPV4_ADDR_CMP (&nexthop->rgate.ipv4, &addr))
          {
             sgate = (union nsm_nexthop *)&nexthop->rgate;
             sifindex = nexthop->rifindex;
          }
        else
          {
            sgate = (union nsm_nexthop *)&nexthop->gate;
            sifindex = nexthop->ifindex;
          }
        /* For inactive RIB entries the ifindex backup is 
         * stored in bfd_ifindex */
        if (sifindex == 0)
          sifindex = nexthop->bfd_ifindex;

        ifp = if_lookup_by_index (&vr->ifm, sifindex);
        if (ifp && (nif = ifp->info))
          {
            /* When "no ip address" command is issued this function is called
               to delete the specific static route. bfd_ifindex stores 
               the ifindex value */
            if ((p_ifindex != 0) && (sifindex != p_ifindex))
              continue;
            /* If the route is configured for BFD update the session */
            if (nsm_bfd_rib_static_config_exists (vrf, nif, rn, nexthop))
              {
                /* Flag NSM_BFD_CONFIG_CHG indicates static route is unconfigured */
                if (CHECK_FLAG (rib->pflags, NSM_BFD_CONFIG_CHG))
                  delete_flag = NSM_BFD_STATIC_SESSION_ADMIN_DEL;
                else
                  delete_flag = NSM_BFD_STATIC_SESSION_DEL;

                if (! nsm_bfd_session_update (vrf, afi, sgate, ifp, ifc,
                      (del == session_del ? delete_flag : 
                       NSM_BFD_STATIC_SESSION_ADD)))
                  return PAL_FALSE;
              }
          }
      }
    return PAL_TRUE;
}

/**@brief - Called when "no ip address" command is issued on an interface
 * Deletes the BFD sessions corresponding to the interface.
 * @param vrf - VRF instance
 * @param afi - address family
 * @param ifp - interface structure
 * @param del - flag to indicate whether to add/delete session.
 * @return -  none
 * */

bool_t
nsm_bfd_update_interface (struct nsm_vrf *vrf, afi_t afi,
                          struct interface *ifp, struct connected *ifc, int del)
{
  struct nsm_if * nif = ifp->info;
  struct ptree_node *rn;
  struct nsm_ptree_node *prn = NULL;
  struct rib *rib = NULL;
  struct nsm_vrf_afi *nva;
  struct nsm_static *ns;
  struct prefix p;

  if ((CHECK_FLAG (nif->flags, NSM_IF_BFD)) || 
     (CHECK_FLAG (vrf->config, NSM_VRF_CONFIG_BFD_IPV4)))
    if (!nsm_bfd_update_session_by_interface (vrf, AFI_IP, ifp, ifc, 
                               del == session_del ?  session_del : session_add))
      return PAL_FALSE; 

 /* For each static route having active nexthop through the interface
     delete the bfd session */
  for (rn = ptree_top (vrf->afi[afi].ip_static);
                          rn; rn = ptree_next (rn))
    for (ns = rn->info; ns; ns = ns->next)
      {
        if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET)) 
          {
            /* Obtain RIB entry to get the actual interface */
            nva = &vrf->afi[ns->afi];
            /* Get the prefix information */
            p.prefixlen = rn->key_len;
            if (afi == AFI_IP)
              {
                p.family = AF_INET;
                NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
                prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST],
                                        (u_char *)&p.u.prefix4, p.prefixlen);
               }
            if (!prn)
              continue;
            if ((rib = nsm_rib_static_lookup (prn->info, ns)) == NULL)
              continue;
            if (!nsm_bfd_update_session_by_rib (vrf, rib, AFI_IP, ifc, prn, 
                                              ifp->ifindex, del == session_del ? 
                                              session_del : session_add))
              return PAL_FALSE;
          }
      }
  return PAL_TRUE;
}

/**@brief - Verifies if BFD configuration exists for the route at any level
 * @param vrf - VRF instance
 * @param nif - NSM interface
 * @param rn - NSM routing table entry
 * @param nexthop - Nexthop for the static route
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */

bool_t
nsm_bfd_rib_static_config_exists (struct nsm_vrf *vrf, 
                                  struct nsm_if *nif,
                                  struct nsm_ptree_node *rn, 
                                  struct nexthop *nexthop)
{
  int ret;

  ret = nsm_bfd_static_lookup(vrf, rn, nexthop);
  if ((ret == 0) || (ret == 1)) 
    return (bool_t) ret;
 
  if (CHECK_FLAG (nif->flags, NSM_IF_BFD))
    return PAL_TRUE;
  
  if (CHECK_FLAG (nif->flags, NSM_IF_BFD_DISABLE))
    return PAL_FALSE;

  if (CHECK_FLAG (vrf->config, NSM_VRF_CONFIG_BFD_IPV4))
    return PAL_TRUE;

  return PAL_FALSE;

}

/**@brief - Verifies if a static route exists. May be removed based on CLI decision  
 * @param vrf - VRF instance 
 * @param rn - NSM routing table entry
 * @param nexthop - Nexthop for the static route
 * @return - 1 if enabled, 0 if explicity disabled, else -1
 * */

s_int32_t
nsm_bfd_static_lookup (struct nsm_vrf *vrf, struct nsm_ptree_node *rn, 
                       struct nexthop *nexthop)
{
  struct prefix p;
  struct nsm_vrf_afi *nva;
  struct ptree_node *static_rn;
  struct nsm_static *ns;

  if (!rn)
    return -1;

  pal_mem_set(&p, 0, sizeof(struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;
  if (rn->tree->ptype == AF_INET)
    NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

  if (p.family == AF_INET)
    {
      nva = &vrf->afi[AFI_IP];
      static_rn = ptree_node_get (nva->ip_static, (u_char *) &p.u.prefix4, 
                                  p.prefixlen);
      if (static_rn && static_rn->info)
        { 
          for (ns = static_rn->info; ns; ns = ns->next)
            {
              if (!IPV4_ADDR_CMP(&ns->gate.ipv4, &nexthop->gate.ipv4))
                {
                  if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET))
                    return 1;
                  if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_DISABLE_SET))
                    return 0;
                }
            }
        }
    }
  return -1;  
}

/**@brief - Final function to add or delete BFD session. 
 * Calls the BFD client APIs
 *
 * @param vrf - VRF instance
 * @param afi - address family
 * @param gate - gateway address
 * @param ifp - interface structure
 * @param del - flag to indicate whether to add/delete session.
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */

bool_t
nsm_bfd_session_update (struct nsm_vrf *vrf, afi_t afi, union nsm_nexthop *gate,
                        struct interface *ifp, struct connected *ifc, int op)
{
  u_int32_t flags = 0;
 
  if (nzg->bc && nzg->bc->state != BFD_CLIENT_STATE_CONNECTED)
    {
      zlog_info (NSM_ZG, "BFD server not connected,"
                         "no session update possible %d");  
      return PAL_FALSE;
    }

  switch (op)
    {
    case NSM_BFD_STATIC_SESSION_ADD:
      if (!nsm_bfd_add_session (vrf, afi, gate, ifp, ifc))
        return PAL_FALSE;
      break;

    case NSM_BFD_STATIC_SESSION_ADMIN_DEL:
      SET_FLAG(flags, BFD_MSG_SESSION_FLAG_AD);
     /* Fall through */ 

    case NSM_BFD_STATIC_SESSION_DEL:
      if (!nsm_bfd_delete_session (vrf, afi, gate, ifp, flags, ifc))
        return PAL_FALSE;
      break;

    default:
      return PAL_FALSE;
    };

  return PAL_TRUE;
}

/**@brief - Updates the static route nexthop status based on the 
 * BFD session event
 *
 * @param s - BFD client session
 * @param afi - address family
 * @param down - Set to 1 for BFD event session_down, set to 0 for session_up
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */

static void
nsm_bfd_static_update_by_session (struct bfd_client_api_session *s, 
                                  afi_t afi, int down)
{
  struct ptree_node *rn;
  struct nsm_ptree_node *prn = NULL;
  struct nsm_master *nm;
  struct nsm_static *ns;
  struct rib *rib = NULL;
  u_int32_t sifindex;
  union nsm_nexthop *sgate;
  struct pal_in4_addr addr;
  struct nexthop *nh;
  struct prefix p;
  struct nsm_vrf *vrf = NULL;
  struct nsm_vrf_afi *nva;

  nm = nsm_master_lookup_by_id (nzg, 0);

  if (!nm)
    return;

  vrf = nsm_vrf_lookup_by_id (nm, 0);

  if (!vrf)
    return;

  pal_mem_set (&addr, 0, sizeof(struct pal_in4_addr));

  /* For each static route relying on this session modify the BFD active flag */
  for (rn = ptree_top (vrf->afi[afi].ip_static);
                          rn; rn = ptree_next (rn))
    for (ns = rn->info; ns; ns = ns->next)
      {
        prn = NULL;
        /* Obtain RIB entry to verify if recursive route */
        nva = &vrf->afi[ns->afi];
        /* Get the prefix information */
        p.prefixlen = rn->key_len;
        if (afi == AFI_IP)
          {
            p.family = AF_INET;
            NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
            prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], 
                                         (u_char *)&p.u.prefix4, p.prefixlen);
          }
#ifdef HAVE_IPV6
        else if (afi == AFI_IP6)
          {
            p.family = AF_INET6;
            NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
            prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], 
                                         (u_char *)&p.u.prefix6, p.prefixlen);
          }
#endif /* HAVE_IPV6 */
        if (prn == NULL)
          continue;

        if ((rib = nsm_rib_static_lookup (prn->info, ns)) == NULL)
          continue;

        for (nh = rib->nexthop; nh; nh = nh->next)
          {
            if (IPV4_ADDR_CMP (&nh->rgate.ipv4, &addr))
              { 
                sgate = (union nsm_nexthop *)&nh->rgate;
                sifindex = nh->rifindex;
              }
            else
              {
                sgate = (union nsm_nexthop *)&nh->gate;
                sifindex = nh->ifindex;
              }

            /* Verify if session parameters match the static route RIB */ 
            if (!IPV4_ADDR_CMP(&s->addr.ipv4.dst, &nh->gate.ipv4))
              {
                if (down)
                  SET_FLAG (nh->flags, NEXTHOP_FLAG_BFD_INACTIVE);
                else
                  UNSET_FLAG (nh->flags, NEXTHOP_FLAG_BFD_INACTIVE);
                /* Set the pflag to indicate the RIB processing is triggered
                   from BFD event */
                SET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
                nsm_rib_process(nm, prn, 0);
                UNSET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
              }
          }
     }
}

/**@brief - Gets the prefix address from the given IPV4 address
 * @param prefix - Value returned with the prefix structure
 * @param ipv4 - Value passed to the function in IPV4 adress format
 * @return - None
 * */

void
nsm_bfd_prefix_ipv4_set (struct prefix *pfx, struct pal_in4_addr *ipv4)
{
  pal_mem_set (pfx, '\0', sizeof (struct prefix));

  pfx->family = AF_INET;
  pfx->prefixlen = IPV4_MAX_PREFIXLEN;
  pal_mem_cpy (&pfx->u.prefix, ipv4, sizeof (struct pal_in4_addr));
  return;
}


/**@brief - Gets the local interface address with best match for the 
            static route nexthop
 * @param ifp - interface structure
 * @param nh - nexthop address
 * @param local_prefix - Value to be returned with local interface prefix
 * @return - None
 * */

static void
nsm_bfd_get_local_addr (struct interface *ifp, union nsm_nexthop *nh, 
                       struct prefix *local_prefix)
{
  struct prefix nh_prefix;
  struct connected *ifc, *match_ifc = NULL;
  u_int8_t match_prefixlen = 0;

  nsm_bfd_prefix_ipv4_set (&nh_prefix, &nh->ipv4);

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      if (prefix_match (ifc->address, &nh_prefix))
        {
          if (ifc->address->prefixlen > match_prefixlen)
            {
              match_prefixlen = ifc->address->prefixlen;
              match_ifc = ifc;
            }
        }
    }    

  if (!match_ifc)
    zlog_err (NSM_ZG, "Local interface does not contain matching address");
  else
    prefix_copy (local_prefix, match_ifc->address);
}

/**@brief - Adds BFD session for a given interface and nexthop
 * @param vrf - VRF instance
 * @param afi - address family
 * @param nh - nexthop address
 * @param ifp - interface structure
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */

bool_t
nsm_bfd_add_session (struct nsm_vrf *vrf, afi_t afi, union nsm_nexthop *nh, 
                     struct interface *ifp, struct connected *ifc) 
{
  struct bfd_client_api_session s;
  struct nsm_bfd_static_session *temp_bs, *bs_exist;
  struct avl_node *node = NULL;
  bool_t ret;
  struct prefix local_prefix;

  /* Get the local address for the static route */
  nsm_bfd_get_local_addr(ifp, nh, &local_prefix );
  /* If the local address does not correspond to static nexthop ignore */
  if (ifc && (!prefix_addr_same (ifc->address, &local_prefix)))
    return PAL_TRUE;

  temp_bs = XCALLOC (MTYPE_NSM_BFD_STATIC_SESSION, 
                            sizeof (struct nsm_bfd_static_session));
  if (!temp_bs)
   {
     zlog_err (NSM_ZG, "Unable to allocate memory BFD static route session");
     return PAL_FALSE;
   }

  if (afi == AFI_IP)
    {
      temp_bs->addr.ipv4.src = local_prefix.u.prefix4;
      temp_bs->addr.ipv4.dst = nh->ipv4;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      IPV6_ADDR_COPY (&temp_bs->addr.ipv6.src, &ifp->ifc_ipv6->address->u.prefix6);
      IPV6_ADDR_COPY (&temp_bs->addr.ipv6.dst, &nh->ipv6);
    }
#endif /* HAVE_IPV6 */
    temp_bs->ifindex = ifp->ifindex;
    temp_bs->afi = afi; 
    node = avl_search (vrf->nsm_bfd_session[afi-1], temp_bs);
  if (node)
    {
      XFREE (MTYPE_NSM_BFD_STATIC_SESSION, temp_bs); 
      bs_exist = (struct nsm_bfd_static_session *) node->info;
      bs_exist->count ++;
      if (IS_NSM_DEBUG_BFD)
            zlog_info (NSM_ZG, "BFD session for multiple static routes, session count %d",
                       bs_exist->count);
      return PAL_TRUE;
    }

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));
  s.zg = nzg;
  s.vr_id = vrf->nm->vr->id;
  s.vrf_id = vrf->vrf->id;
  SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_PS);
  s.ifindex = ifp->ifindex;
  s.afi = afi;
  if (afi == AFI_IP)
    {
      s.addr.ipv4.src = local_prefix.u.prefix4;
      s.addr.ipv4.dst = nh->ipv4;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      IPV6_ADDR_COPY (&s.addr.ipv6.src, &ifp->ifc_ipv6->address->u.prefix6);
      IPV6_ADDR_COPY (&s.addr.ipv6.dst, &nh->ipv6);
    }
#endif /* HAVE_IPV6 */

  ret = bfd_client_api_session_add (&s);
  if (ret == LIB_API_SUCCESS)
    {
      if (IS_NSM_DEBUG_BFD)
        zlog_info (NSM_ZG, "BFD session added for static route");

      if ((avl_insert (vrf->nsm_bfd_session[s.afi-1], temp_bs)) < 0)
        {
          (void) bfd_client_api_session_delete (&s);
          zlog_err (NSM_ZG, "Unable to insert BFD static route session"
                            " in AVL tree");
          XFREE (MTYPE_NSM_BFD_STATIC_SESSION, temp_bs);
          return PAL_FALSE;
        }
      temp_bs->count = 1;
      return PAL_TRUE;
    }
  else
    {
      zlog_err (NSM_ZG, "Unable to add BFD session for static route");
      XFREE (MTYPE_NSM_BFD_STATIC_SESSION, temp_bs);
      return PAL_FALSE;
    }

  return PAL_TRUE;
}

/**@brief - Deletes BFD session for a given interface and nexthop
 * @param vrf - VRF instance
 * @param afi - address family
 * @param nh - nexthop address
 * @param ifp - interface structure
 * @return - PAL_TRUE on success, PAL_FALSE on failure
 * */

bool_t
nsm_bfd_delete_session (struct nsm_vrf *vrf, afi_t afi, union nsm_nexthop *nh, 
                        struct interface *ifp, u_int32_t flags,
                        struct connected *ifc)
{
  struct bfd_client_api_session s;
  bool_t ret;
  struct avl_node *node = NULL;
  struct nsm_bfd_static_session temp_bs, *bs_exist;
  struct prefix local_prefix;

  /* Get the local address for the static route */
  nsm_bfd_get_local_addr(ifp, nh, &local_prefix );
  /* If the local address does not correspond to static nexthop ignore */
  if (ifc && (!prefix_addr_same (ifc->address, &local_prefix)))
    return PAL_TRUE;

  if (afi == AFI_IP)
    {
      if (!ifp->ifc_ipv4)
        return PAL_TRUE;
      temp_bs.addr.ipv4.src = local_prefix.u.prefix4;
      temp_bs.addr.ipv4.dst = nh->ipv4;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (!ifp->ifc_ipv6)
        return PAL_TRUE;
      IPV6_ADDR_COPY (&temp_bs.addr.ipv6.src, 
                      &ifp->ifc_ipv6->address->u.prefix6);
      IPV6_ADDR_COPY (&temp_bs.addr.ipv6.dst, &nh->ipv6);
    }
#endif /* HAVE_IPV6 */
    temp_bs.ifindex = ifp->ifindex;
    temp_bs.afi = afi;
    node = avl_search (vrf->nsm_bfd_session[afi-1], &temp_bs);
    if (node)
      {
        bs_exist = (struct nsm_bfd_static_session *)node->info;
        bs_exist->count --;
      }
    else
      {
        zlog_err (NSM_ZG, "Unable to find BFD static route session"
                          " in AVL tree for deletion");
          return PAL_TRUE;
      }

    if (bs_exist->count != 0)         
      {
        if (IS_NSM_DEBUG_BFD)
            zlog_info (NSM_ZG, "BFD session for multiple static routes,"
                               " session count %d", bs_exist->count);
        return PAL_TRUE;
      }
    SET_FLAG (flags, BFD_CLIENT_API_FLAG_PS);

    pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));
    s.zg = nzg;
    s.vr_id = vrf->nm->vr->id;
    s.vrf_id = vrf->vrf->id;
    s.flags = flags;
    s.ifindex = ifp->ifindex;
    s.afi = afi;
    if (s.afi == AFI_IP)
      {
        s.addr.ipv4.src = local_prefix.u.prefix4;
        s.addr.ipv4.dst = nh->ipv4;
      }
#ifdef HAVE_IPV6
    else if (s.afi == AFI_IP6)
      {
        IPV6_ADDR_COPY (&s.addr.ipv6.src, 
                        &ifp->ifc_ipv6->address->u.prefix6);
        IPV6_ADDR_COPY (&s.addr.ipv6.dst, &nh->ipv6);
      }
#endif /* HAVE_IPV6 */
    ret = bfd_client_api_session_delete (&s);
    if (ret == LIB_API_SUCCESS)
      {
        if (IS_NSM_DEBUG_BFD)
          zlog_info (NSM_ZG, "BFD session deleted for static route");
        if ((avl_remove (vrf->nsm_bfd_session[s.afi-1], &temp_bs)) < 0)
          zlog_err (NSM_ZG, "Unable to remove BFD static route session"
                            " in AVL tree");
       }
    else
      {
        if (IS_NSM_DEBUG_BFD)
          zlog_err (NSM_ZG, "Unable to delete BFD session for static route");
        return PAL_FALSE;
      }

  return PAL_TRUE;
}


/**@brief - Callback handle for sesion_up event from BFD
 * @param s - BFD client session
 * */

static void
nsm_bfd_session_up(struct bfd_client_api_session *s)
{
  if (s->afi == AFI_IP)
    nsm_bfd_static_update_by_session (s, AFI_IP, 0);
#ifdef HAVE_IPV6
  else if (s->afi == AFI_IP6)
    nsm_bfd_static_update_by_session (s, AFI_IP6, 0);
#endif /* HAVE_IPV6 */
}

/**@brief - Callback handle for sesion_down event from BFD
 * @param s - BFD client session
 * */

static void
nsm_bfd_session_down (struct bfd_client_api_session *s)
{
  if (s->afi == AFI_IP)
    nsm_bfd_static_update_by_session (s, AFI_IP, 1);
#ifdef HAVE_IPV6
  else if (s->afi == AFI_IP6)
    nsm_bfd_static_update_by_session (s, AFI_IP6, 1);
#endif /* HAVE_IPV6 */

}

static void
nsm_bfd_session_admin_down(struct bfd_client_api_session *s)
{
  /* Doesn't do anything at this point.  */
}

/**@brief - This function is invoked to enable BFD for static routes
            when BFD connects with NSM.

 *  @param *nm - NSM master structure

 *  @return NONE
 */

static void
nsm_bfd_static_enable (struct nsm_master *nm)
{
  struct nsm_vrf *nv;
  struct apn_vr *vr;
  struct listnode *node;
  struct nsm_if *nif;
  struct interface *ifp;
  struct ptree_node *rn;
  struct prefix p;
  struct nsm_static *ns;

  nv = nsm_vrf_lookup_by_id (nm, 0);
  if (!nv)
    return;

  vr = nm->vr;

  /* For all intefaces enabled with BFD static support add session */
  LIST_LOOP (vr->ifm.if_list, ifp, node)
    if ((nif = ifp->info))
      if (CHECK_FLAG (nif->flags, NSM_IF_BFD) ||
          CHECK_FLAG (nv->config, NSM_VRF_CONFIG_BFD_IPV4))
        if(!nsm_bfd_update_session_by_interface (nv, AFI_IP, ifp, NULL,
                                                           session_add))
           zlog_info (NSM_ZG, " Unable to add BFD static session");

   /* For all static routes enabled with BFD static support add session */
   for (rn = ptree_top (nv->IPV4_STATIC); rn; rn = ptree_next (rn))
     if (rn->info)
       {
         pal_mem_set (&p, 0, sizeof (struct prefix));
         p.prefixlen = rn->key_len;
         NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

         for (ns = rn->info; ns; ns = ns->next)
           {
             if (CHECK_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET))
               {
                 /* We unset the flag before adding session. Else the
                    function assumes the session already exists */
                 UNSET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET);
                 if (!nsm_bfd_update_session_by_static(ns, nv, 
                               (struct prefix_ipv4 *)&p, session_add))
                   {
                     zlog_info (NSM_ZG, " Unable to add BFD static session");
                     continue;
                   }
                 SET_FLAG (ns->flags, NSM_STATIC_FLAG_BFD_SET);
               }
           }
       }
}

/* Callback to handle connected event from BFD server */
static void
nsm_bfd_connected(struct bfd_client_api_server *s)
{
  struct apn_vr *vr;
  struct nsm_master *nm = NULL;
  u_int32_t i;

  if (IS_NSM_DEBUG_BFD)
    zlog_info (NSM_ZG, " BFD server connected");

  nzg->bc->state = BFD_CLIENT_STATE_CONNECTED;

  for (i = 0; i < vector_max (s->zg->vr_vec); i++)
    if ((vr = vector_slot (s->zg->vr_vec, i)))
      if ((nm = vr->proto))
        {
#ifdef HAVE_MPLS_OAM
          nsm_mpls_bfd_enable (nm);
#ifdef HAVE_VCCV
          nsm_vccv_bfd_enable (nm);
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */

          nsm_bfd_static_enable (nm);
        }
}

/**@brief - This function is invoked to reset BFD status in all the RIB nexthop

 *  @param *s - VRF structure

 *  @return NONE
 */

static void
nsm_bfd_reset_rib_nexthop (struct nsm_vrf *vrf)
{
  struct ptree_node *rn;
  struct nsm_ptree_node *prn = NULL;
  struct nsm_static *ns;
  struct rib *rib = NULL;
  struct prefix p;
  struct nsm_vrf_afi *nva;
  struct nexthop *nexthop;
  struct nsm_master *nm;

  /* Find each static route having active nexthop BFD inactive */
  for (rn = ptree_top (vrf->afi[AFI_IP].ip_static);
                          rn; rn = ptree_next (rn))
    for (ns = rn->info; ns; ns = ns->next)
      {
        /* Obtain RIB entry to modify nexthop */
        nva = &vrf->afi[ns->afi];
        /* Get the prefix information */
        p.prefixlen = rn->key_len;
        p.family = AF_INET;
        NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
        prn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST],
                              (u_char *)&p.u.prefix4, p.prefixlen);
        if (!prn)
          continue;
        if ((rib = nsm_rib_static_lookup (prn->info, ns)) == NULL)
          continue;

        for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
          {
            /* Unset BFD inactive flag */
            if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE))
              {
                nm = nsm_master_lookup_by_id (nzg, 0);
                UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE);
                /* Set the flag to indicate that rib processing is triggered
                 * from BFD, so that it should not again lead to
                 * session add/delete */
                SET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
                nsm_rib_process(nm, prn, 0);
                /* Reset the flag indicating BFD event change. Only then RIB
                   processing triggered from other events would add/delete
                   BFD session */
                UNSET_FLAG( rib->pflags, NSM_ROUTE_CHG_BFD);
              }
          }
      }
}

/**@brief - This function is invoked to disable BFD for static routes 
            when BFD disconnects with NSM.

 *  @param *nm - NSM master structure 

 *  @return NONE
 */

static void
nsm_bfd_static_disable (struct nsm_master *nm)
{
  struct nsm_vrf *vrf = NULL;
  u_int32_t i;

  /* Remove all nodes in  the AVL tree for static routes */
  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (!vrf)
    return;

  /* Reset all BFD static entries in the RIB */
  nsm_bfd_reset_rib_nexthop (vrf);
  for (i = 0; i < NSM_BFD_ADDR_FAMILY_MAX; i++)
    {
      if (vrf->nsm_bfd_session[i])
        {
          avl_tree_cleanup (vrf->nsm_bfd_session[i], NULL);
        }
    }
}

/**@brief - This function is invoked when BFD disconnects with NSM.

 *  @param *s - BFD Client Server structure.

 *  @return NONE
 */
static void
nsm_bfd_disconnected (struct bfd_client_api_server *s)
{
  struct nsm_master *nm = NULL;
  struct apn_vr *vr;
  u_int32_t i;

  for (i = 0; i < vector_max (s->zg->vr_vec); i++)
    if ((vr = vector_slot (s->zg->vr_vec, i)))
      if ((nm = vr->proto))
        {
#ifdef HAVE_MPLS_OAM
          nsm_mpls_bfd_disable (nm);
#ifdef HAVE_VCCV
          nsm_vccv_bfd_disable (nm);
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */
    
          nsm_bfd_static_disable (nm);
        }
}

static void
nsm_bfd_session_error (struct bfd_client_api_session *s)
{
  /* Doesn't do anything at this point.  */
}

void
nsm_bfd_init (struct lib_globals *zg)
{
  struct bfd_client_api_callbacks cb;
  memset (&cb, 0, sizeof (struct bfd_client_api_callbacks));
  cb.connected = nsm_bfd_connected;
  cb.disconnected = nsm_bfd_disconnected;
  cb.reconnected = nsm_bfd_connected;
  cb.session_up = nsm_bfd_session_up;
  cb.session_down = nsm_bfd_session_down;
  cb.session_admin_down = nsm_bfd_session_admin_down;
  cb.session_error = nsm_bfd_session_error;
#ifdef HAVE_MPLS_OAM
  cb.mpls_session_down = nsm_mpls_bfd_session_down;
#ifdef HAVE_VCCV
  cb.vccv_session_down = nsm_vccv_bfd_session_down;
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */

  bfd_client_api_client_new (zg, &cb);
  bfd_client_api_client_start (zg);
}

void
nsm_bfd_term (struct lib_globals *zg)
{
  bfd_client_api_client_delete (zg);
}


/**@brief - Comparison function for BFD static session AVL nodes 
 * @param data1 - First AVL node of BFD static session 
 * @param data2 - Second AVL node of BFD static session 
 * @return - Returns 1 if data1 > data2 else -1. If same returns 0.
 * */

s_int32_t
nsm_bfd_session_cmp (void *data1, void *data2)
{
  struct nsm_bfd_static_session *sess1 = 
                               (struct nsm_bfd_static_session *) data1;
  struct nsm_bfd_static_session *sess2 = 
                                (struct nsm_bfd_static_session *) data2;
  s_int32_t ret;

  if (data1 == NULL || data2 == NULL)
    return -1;
  
  if (sess1->afi == AFI_IP)
    if ((ret = 
          IPV4_ADDR_CMP (&sess1->addr.ipv4.dst, &sess2->addr.ipv4.dst)) != 0)
      return ret;
#ifdef HAVE_IPV6
  if (sess1->afi == AFI_IP6)
    if ((ret = 
          IPV6_ADDR_CMP (&sess1->addr.ipv6.dst, &sess2->addr.ipv6.dst)) != 0)
      return ret;
#endif

  return pal_mem_cmp (&sess1->ifindex, &sess2->ifindex, sizeof (u_int32_t));
}

/**@brief - This function turns on the debug output for NSM BFD.

 *  @param *nm - NSM Master.

 *  @return NONE
 */
void
nsm_bfd_debug_set (struct nsm_master *nm)
{
  bfd_client_api_debug_set (nm->zg);
}

/**@brief - This function turns off the debug output for NSM BFD.

 *  @param *nm - NSM Master.

 *  @return NONE
 */
void
nsm_bfd_debug_unset (struct nsm_master *nm)
{
  bfd_client_api_debug_unset (nm->zg);
}
#endif /* HAVE_BFD */
