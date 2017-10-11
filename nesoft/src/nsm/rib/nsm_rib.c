/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "table.h"
#include "log.h"
#include "sockunion.h"
#include "thread.h"
#include "nexthop.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_vrf.h"
#include "nsm/rib/nsm_rib_vrf.h"
#include "nsm/rib/nsm_table.h"
#include "nsm/nsm_interface.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_rt.h"
#include "nsm/nsm_server.h"
#include "nsm/rib/nsm_server_rib.h"
#include "nsm/rib/nsm_nexthop.h"
#include "nsm/rib/nsm_redistribute.h"
#include "nsm/nsm_debug.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_fea.h"
#include "nsm/rib/nsm_static_mroute.h"
#ifdef HAVE_VRF
#include "nsm/mpls/nsm_mpls_rib.h"
#endif 
#ifdef HAVE_BFD
#include "nsm/nsm_bfd.h"
#endif /* HAVE_BFD */
#ifdef HAVE_VRRP
#include "nsm/vrrp/vrrp_init.h"
#endif /* HAVE_VRRP */
#ifdef HAVE_RMM
#include "nsm/rd/nsm_rd.h"
#endif /* HAVE_RMM */
#ifdef HAVE_CRX
#include "nsm/crx/crx_rib.h"
#endif /* HAVE_CRX */
#ifdef HAVE_MPLS
#include "nsm_lp_serv.h"
#endif /* HAVE_MPLS */
#include "ptree.h"

#ifdef HAVE_HA
#include "nsm_rib_cal.h"
#endif /* HAVE_HA */

/* Add nexthop to the end of the list.  */
void
nsm_nexthop_add (struct rib *rib, struct nexthop *nexthop)
{
  struct nexthop *last;

  for (last = rib->nexthop; last && last->next; last = last->next)
    ;

  if (last)
    last->next = nexthop;
  else
    rib->nexthop = nexthop;
  nexthop->prev = last;

  rib->nexthop_num++;
}

/* Delete specified nexthop from the list. */
void
nsm_nexthop_delete (struct rib *rib, struct nexthop *nexthop)
{
  if (nexthop->next)
    nexthop->next->prev = nexthop->prev;
  if (nexthop->prev)
    nexthop->prev->next = nexthop->next;
  else
    rib->nexthop = nexthop->next;
  rib->nexthop_num--;
}

/* Free nexthop. */
void
nsm_nexthop_free (struct nexthop *nexthop)
{
  if (nexthop->ifname)
    XFREE (MTYPE_NSM_IFNAME, nexthop->ifname);
  XFREE (MTYPE_NEXTHOP, nexthop);
}

/* Allocate a new RIB structure.  */
struct rib *
nsm_rib_new (u_char type, u_char distance, u_char flags,
             u_int32_t metric, struct nsm_vrf *vrf)
{
  struct rib *rib;

  rib = XCALLOC (MTYPE_NSM_RIB, sizeof (struct rib));
  if (! rib)
    return NULL;

  rib->type = type;
  rib->flags = flags;
  rib->metric = metric;
  rib->distance = distance;
  rib->vrf = vrf;
  rib->uptime = pal_time_current (NULL);
  rib->tag = 0;

  return rib;
}

/* Free RIB.  */
void
nsm_rib_free (struct rib *rib)
{
  struct nexthop *nexthop;
  struct nexthop *next;

  for (nexthop = rib->nexthop; nexthop; nexthop = next)
    {
      next = nexthop->next;
      nsm_nexthop_free (nexthop);
    }

#ifdef HAVE_STAGGER_KERNEL_MSGS
  NSM_ASSERT (rib->kernel_ms_lnode == NULL);
#endif /* HAVE_STAGGER_KERNEL_MSGS */

#ifdef HAVE_VRF
  XFREE (MTYPE_NSM_RIB, rib->domain_conf);
#endif /* HAVE_VRF */
  XFREE (MTYPE_NSM_RIB, rib);
}

struct nexthop *
nsm_nexthop_ifindex_add (struct rib *rib,
                         u_int32_t ifindex, int snmp_route_type)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IFINDEX;
  nexthop->ifindex = ifindex;
#ifdef HAVE_SNMP
  nexthop->snmp_route_type = snmp_route_type;
#endif /* HAVE_SNMP */

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}

struct nexthop *
nsm_nexthop_ifname_add (struct rib *rib, char *ifname, int snmp_route_type)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IFNAME;
  nexthop->ifname = XSTRDUP (MTYPE_NSM_IFNAME, ifname);
#ifdef HAVE_SNMP
  nexthop->snmp_route_type = snmp_route_type;
#endif /* HAVE_SNMP */

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}

struct nexthop *
nsm_nexthop_ipv4_add (struct rib *rib,
                      struct pal_in4_addr *ipv4, int snmp_route_type)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IPV4;
  nexthop->gate.ipv4 = *ipv4;
#ifdef HAVE_SNMP
  nexthop->snmp_route_type = snmp_route_type;
#endif /* HAVE_SNMP */

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}

#ifdef HAVE_VRF
struct nexthop *
nsm_nexthop_mpls_add (struct rib *rib, struct pal_in4_addr *ipv4, int fib)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_MPLS;
  nexthop->gate.ipv4 = *ipv4;

  if (fib)
    SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}
#endif /* HAVE_VRF */

struct nexthop *
nsm_nexthop_ipv4_ifindex_add (struct rib *rib, struct pal_in4_addr *ipv4,
                              u_int32_t ifindex, int snmp_route_type)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IPV4_IFINDEX;
  nexthop->gate.ipv4 = *ipv4;
  nexthop->ifindex = ifindex;
#ifdef HAVE_SNMP
  nexthop->snmp_route_type = snmp_route_type;
#endif /* HAVE_SNMP */

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}

struct nexthop *
nsm_nexthop_ipv4_ifname_add (struct rib *rib, struct pal_in4_addr *ipv4,
                             char *ifname, int snmp_route_type)
{
  struct nexthop *nexthop;
  struct apn_vr *vr = rib->vrf->vrf->vr;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IPV4_IFNAME;
  nexthop->gate.ipv4 = *ipv4;
  nexthop->ifname = XSTRDUP (MTYPE_NSM_IFNAME, ifname);
#ifdef HAVE_SNMP
  nexthop->snmp_route_type = snmp_route_type;
#endif /* HAVE_SNMP */

  nexthop->ifindex = if_name2index (&vr->ifm, ifname);

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}

#ifdef HAVE_IPV6
struct nexthop *
nsm_nexthop_ipv6_add (struct rib *rib, struct pal_in6_addr *ipv6)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IPV6;
  nexthop->gate.ipv6 = *ipv6;

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}

struct nexthop *
nsm_nexthop_ipv6_ifname_add (struct rib *rib, struct pal_in6_addr *ipv6,
                             char *ifname)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IPV6_IFNAME;
  nexthop->gate.ipv6 = *ipv6;
  nexthop->ifname = XSTRDUP (MTYPE_NSM_IFNAME, ifname);

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}

struct nexthop *
nsm_nexthop_ipv6_ifindex_add (struct rib *rib, struct pal_in6_addr *ipv6,
                              u_int32_t ifindex)
{
  struct nexthop *nexthop;

  nexthop = XCALLOC (MTYPE_NEXTHOP, sizeof (struct nexthop));
  nexthop->type = NEXTHOP_TYPE_IPV6_IFINDEX;
  nexthop->gate.ipv6 = *ipv6;
  nexthop->ifindex = ifindex;

  nsm_nexthop_add (rib, nexthop);

  return nexthop;
}
#endif /* HAVE_IPV6 */

/* If force flag is not set, do not modify flags at all for uninstall
   the route from FIB. */
result_t
nsm_nexthop_active_ipv4_vrf (struct rib *rib, struct nexthop *nexthop,
                             s_int32_t set, struct nsm_ptree_node *top)
{
  struct nsm_ptree_table *table = rib->vrf->afi[AFI_IP].rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct nexthop *newhop;
  struct rib *match, *selected = NULL;
  struct prefix_ipv4 p;
  int recursive_change = 0;

  if (nexthop->type == NEXTHOP_TYPE_IPV4)
    {
      nexthop->ifindex = 0;
#ifdef HAVE_HA
      nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
    }

#ifdef HAVE_VRF
  if (NSM_CAP_HAVE_VRF)
    {
      if (nexthop->type == NEXTHOP_TYPE_MPLS)
        {
          nexthop->ifindex = 0;
#ifdef HAVE_HA
          nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
        }
    }
#endif /* HAVE_VRF */


  if (set)
    {
#ifdef HAVE_HA
      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
        nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
      UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);

    }

#ifdef HAVE_HA
  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
    nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
  UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE);

  /* Make lookup prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix_ipv4));
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.prefix = nexthop->gate.ipv4;

  /* Check if nexthop address is ourself. */
  if (nsm_ipv4_connected_table_lookup (rib->vrf, (struct prefix *)&p))
    return 0;

  rn = nsm_ptree_node_match (table, (u_char *) &p.prefix, p.prefixlen);
  while (rn)
    {
      nsm_ptree_unlock_node (rn);

      selected = NULL;
      /* Pick up selected route. */
      for (match = rn->info; match; match = match->next)
        {
          /* Route should not recurse on itself */
          if (match == rib)
            {
              selected = NULL;
              break;
            }

          /* XXX: Do not recurse on blackhole routes */
          if (CHECK_FLAG (match->flags, RIB_FLAG_BLACKHOLE))
            {
              selected = NULL;
              break;
            }

          if (CHECK_FLAG (match->flags, RIB_FLAG_SELECTED))
            selected = match;
        }
      match = selected;

      /* If there is no selected route or matched route is EGP, go up
         tree. */
      if (! match || ((match->type == APN_ROUTE_BGP) && (rib->type == APN_ROUTE_BGP) &&
                      (rib->sub_type == APN_ROUTE_BGP_IBGP) && 
                      (match->sub_type == APN_ROUTE_BGP_EBGP)))
        {
          do 
	    {
	      rn = rn->parent;
	    } while (rn && rn->info == NULL);
          if (rn)
            nsm_ptree_lock_node (rn);
        }
      else
        {
          if (match->type == APN_ROUTE_CONNECT)
            {
              /* Directly point connected route. */
              newhop = match->nexthop;
              if (newhop && nexthop->type == NEXTHOP_TYPE_IPV4)
                {
                  nexthop->ifindex = newhop->ifindex;

                  if (nexthop->ifindex > 0)
                    nexthop->type = NEXTHOP_TYPE_IPV4_IFINDEX;

                  if (set)
                    {
                      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                        {
                          UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);
                          nexthop->rrn = NULL;
                        }
                    }
                  else
                    {
                      /* Nexthop has changed from recursive to directly 
                       * connected. Set rifindex = 0 so that change is 
                       * processed.
                       */
                      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                        {
                          nexthop->rifindex = 0;
#ifdef HAVE_BFD
                          pal_mem_set (&nexthop->rgate.ipv4, 0, sizeof (struct pal_in4_addr));
#endif /* HAVE_BFD */
                        }
                    }
#ifdef HAVE_HA
                  nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */

                  return 1;
                }

              /* Nexthop resolvable via directly connected route. If it is recursive, check if the
                 route can be directly resolved. */
              if (newhop && nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX)
                {
                  nexthop->ifindex = newhop->ifindex;

                  if (set)
                    {
                      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                        {
                          UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);
                          nexthop->rrn = NULL;
                        }
                    }
                  else
                    {
                      /* Nexthop has changed from recursive to directly 
                       * connected. Set rifindex = 0 so that change is 
                       * processed.
                       */
                      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                        {
                          nexthop->rifindex = 0;
#ifdef HAVE_BFD
                          pal_mem_set (&nexthop->rgate.ipv4, 0, sizeof (struct pal_in4_addr));
#endif /* HAVE_BFD */
                        }
                    }

                  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
                    UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE);

#ifdef HAVE_HA
                  nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */

                  return 1;
                }

#ifdef HAVE_VRF
              IF_NSM_CAP_HAVE_VRF
                {
                  if (newhop && nexthop->type == NEXTHOP_TYPE_MPLS)
                    {
                      nexthop->ifindex = newhop->ifindex;
#ifdef HAVE_HA
                      nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
                      return 1;
                    }
                }
#endif /* HAVE_VRF */

              return 1;
            }
          else if (CHECK_FLAG (rib->flags, RIB_FLAG_INTERNAL))
            {
              for (newhop = match->nexthop; newhop; newhop = newhop->next)
                if (CHECK_FLAG (newhop->flags, NEXTHOP_FLAG_FIB)
                    && ! CHECK_FLAG (newhop->flags, NEXTHOP_FLAG_RECURSIVE))
                  {
                    if (set)
                      {
                        SET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);
                        nexthop->rrn = rn;
#ifdef HAVE_HA
                        nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
                        nexthop->rtype = newhop->type;
                        if (newhop->type == NEXTHOP_TYPE_IPV4 ||
                            newhop->type == NEXTHOP_TYPE_IPV4_IFINDEX ||
                            newhop->type == NEXTHOP_TYPE_IPV4_IFNAME)
                          nexthop->rgate.ipv4 = newhop->gate.ipv4;
                        if (newhop->type == NEXTHOP_TYPE_IFINDEX
                            || newhop->type == NEXTHOP_TYPE_IFNAME
                            || newhop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                            || newhop->type == NEXTHOP_TYPE_IPV4_IFNAME)
                          nexthop->rifindex = newhop->ifindex;
                        else if (newhop->ifindex > 0)
                          {
                            nexthop->rifindex = newhop->ifindex;

                            if (nexthop->rtype == NEXTHOP_TYPE_IPV4)
                              nexthop->rtype = NEXTHOP_TYPE_IPV4_IFINDEX;
                          }
                      }
                    else
                      {
                        /* Check is the recusively resolved nexthop route
                         * has changed
                         */
                        if (nexthop->rtype != newhop->type)
                          recursive_change = 1;
                        else
                          {
                            if (newhop->type == NEXTHOP_TYPE_IPV4 ||
                                newhop->type == NEXTHOP_TYPE_IPV4_IFINDEX ||
                                newhop->type == NEXTHOP_TYPE_IPV4_IFNAME)
                              {
                                if (IPV4_ADDR_CMP (&nexthop->rgate.ipv4, 
                                      &newhop->gate.ipv4) != 0)
                                  recursive_change = 1;
                              }
                            if (newhop->type == NEXTHOP_TYPE_IFINDEX
                                || newhop->type == NEXTHOP_TYPE_IFNAME
                                || newhop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                                || newhop->type == NEXTHOP_TYPE_IPV4_IFNAME)
                              {
                                if (nexthop->rifindex != newhop->ifindex)
                                  recursive_change = 1;
                              }
                            else if (newhop->ifindex > 0)
                              {
                                if (nexthop->rifindex != newhop->ifindex)
                                  recursive_change = 1;
                              }
                          }
                        
                        /* As the route is now recursive, set the ifindex to zero 
                         * so that it will trigger a RIB update. */
                        if (recursive_change)
                          {
                            nexthop->ifindex = 0;
                            nexthop->rifindex = 0;
#ifdef HAVE_BFD
                            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE); 
#endif /* HAVE_BFD */
                          }
                      }
                    /* Check if recursively matched route is a Blackhole */
                    if (CHECK_FLAG (match->flags, RIB_FLAG_BLACKHOLE))
                      SET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE);

                    return 1;
                  }
            }
          do {
            rn = rn->parent;
          } while (rn && rn->info == NULL);
          if (rn)
            nsm_ptree_lock_node (rn);
        }
    }

  /*
   * Protocol routes with non-verifiable Nexthops may already
   * be forwardable via MPLS LSPs. To provide for redistribution
   * of such routes their Nexthop is validated. They are not
   * FIB-forwardable and will not be installed in FIB.
   */
  if (rib->type == APN_ROUTE_BGP
      && rib->sub_type == APN_ROUTE_BGP_MPLS)
    {
      SET_FLAG (rib->flags, RIB_FLAG_NON_FIB);
#ifdef HAVE_HA
      nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
      return 1;
    }

  return 0;
}

#ifdef HAVE_IPV6
/* If force flag is not set, do not modify falgs at all for uninstall
   the route from FIB. */
result_t
nsm_nexthop_active_ipv6_vrf (struct rib *rib, struct nexthop *nexthop,
                             s_int32_t set, struct nsm_ptree_node *top)
{
  struct nsm_ptree_table *table = rib->vrf->afi[AFI_IP6].rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct nexthop *newhop;
  struct rib *match, *selected = NULL;
  struct prefix_ipv6 p;
  int recursive_change = 0;

  if (nexthop->type == NEXTHOP_TYPE_IPV6)
    {
      nexthop->ifindex = 0;
#ifdef HAVE_HA
      nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
    }

  if (set)
    {
#ifdef HAVE_HA
      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
        nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
      UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);

    }
#ifdef HAVE_HA
  if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
    nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
  UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE);

  /* Make lookup prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix_ipv6));
  p.family = AF_INET6;
  p.prefixlen = IPV6_MAX_PREFIXLEN;
  p.prefix = nexthop->gate.ipv6;

  rn = nsm_ptree_node_match (table, (u_char *) &p.prefix, p.prefixlen);
  while (rn)
    {
      nsm_ptree_unlock_node (rn);

      /* Pick up selected route. */
      for (match = rn->info; match; match = match->next)
        {
          /* Route should not recurse on itself */
          if (match == rib)
            {
              selected = NULL;
              break;
            }

          /* XXX: Do not recurse on blackhole routes */
          if (CHECK_FLAG (match->flags, RIB_FLAG_BLACKHOLE))
            {
              selected = NULL;
              break;
            }

          if (CHECK_FLAG (match->flags, RIB_FLAG_SELECTED))
            selected = match;
        }
      match = selected;

      /* If there is no selected route or matched route is EGP, go up
         tree. */
      if (! match || match->type == APN_ROUTE_BGP)
        {
          do {
            rn = rn->parent;
          } while (rn && rn->info == NULL);
          if (rn)
            nsm_ptree_lock_node (rn);
        }
      else
        {
          if (match->type == APN_ROUTE_CONNECT)
            {
              /* Directly point connected route. */
              newhop = match->nexthop;

              if (newhop && nexthop->type == NEXTHOP_TYPE_IPV6)
                {
                  nexthop->ifindex = newhop->ifindex;

                  if (nexthop->ifindex > 0)
                    nexthop->type = NEXTHOP_TYPE_IPV6_IFINDEX;

                  if (set)
                    {
                      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                        {
                          UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);
                          nexthop->rrn = NULL;
                        }
                    }
                  else
                    {
                      /* Nexthop has changed from recursive to directly 
                       * connected. Set rifindex = 0 so that change is 
                       * processed.
                       */
                      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
                        nexthop->rifindex = 0;
                    }
#ifdef HAVE_HA
                  nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
                }

              return 1;
            }
          else if (CHECK_FLAG (rib->flags, RIB_FLAG_INTERNAL))
            {
              for (newhop = match->nexthop; newhop; newhop = newhop->next)
                if (CHECK_FLAG (newhop->flags, NEXTHOP_FLAG_FIB)
                    && ! CHECK_FLAG (newhop->flags, NEXTHOP_FLAG_RECURSIVE))
                  {
                    if (set)
                      {
                        SET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE);
                        nexthop->rrn = rn;
#ifdef HAVE_HA
                        nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
                        nexthop->rtype = newhop->type;
                        if (newhop->type == NEXTHOP_TYPE_IPV6
                            || newhop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                            || newhop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                          nexthop->rgate.ipv6 = newhop->gate.ipv6;
                        if (newhop->type == NEXTHOP_TYPE_IFINDEX
                            || newhop->type == NEXTHOP_TYPE_IFNAME
                            || newhop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                            || newhop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                          nexthop->rifindex = newhop->ifindex;
                        else if (newhop->ifindex > 0)
                          {
                            nexthop->rifindex = newhop->ifindex;

                            if (nexthop->rtype == NEXTHOP_TYPE_IPV6)
                              nexthop->rtype = NEXTHOP_TYPE_IPV6_IFINDEX;
                          }
                      }
                    else
                      {
                        /* Check is the recusively resolved nexthop route
                         * has changed
                         */
                        if (nexthop->rtype != newhop->type)
                          recursive_change = 1;
                        else
                          {
                            if (newhop->type == NEXTHOP_TYPE_IPV6 ||
                                newhop->type == NEXTHOP_TYPE_IPV6_IFINDEX ||
                                newhop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                              {
                                if (IPV6_ADDR_CMP (&nexthop->rgate.ipv6, 
                                      &newhop->gate.ipv6) != 0)
                                  recursive_change = 1;
                              }
                            if (newhop->type == NEXTHOP_TYPE_IFINDEX
                                || newhop->type == NEXTHOP_TYPE_IFNAME
                                || newhop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                                || newhop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                              {
                                if (nexthop->rifindex != newhop->ifindex)
                                  recursive_change = 1;
                              }
                            else if (newhop->ifindex > 0)
                              {
                                if (nexthop->rifindex != newhop->ifindex)
                                  recursive_change = 1;
                              }
                          }
                        
                        /* As the route is now recursive, set the ifindex to zero 
                         * so that it will trigger a RIB update. */
                        if (recursive_change)
                          {
                            nexthop->ifindex = 0;
                            nexthop->rifindex = 0;
                          }
                      }
                    /* Check if recursively matched route is a Blackhole */
                    if (CHECK_FLAG (match->flags, RIB_FLAG_BLACKHOLE))
                      SET_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE);
                    return 1;
                  }
            }
          do {
            rn = rn->parent;
          } while (rn && rn->info == NULL);
          if (rn)
            nsm_ptree_lock_node (rn);
        }
    }

  /*
   * Protocol routes with non-verifiable Nexthops may already
   * be forwardable via MPLS LSPs. To provide for redistribution
   * of such routes their Nexthop is validated. They are not
   * FIB-forwardable and will not be installed in FIB.
   */
  if (rib->type == APN_ROUTE_BGP
      && rib->sub_type == APN_ROUTE_BGP_MPLS)
    {
      SET_FLAG (rib->flags, RIB_FLAG_NON_FIB);
#ifdef HAVE_HA
      nsm_cal_modify_rib (rib, top);
#endif /* HAVE_HA */
      return 1;
    }

  return 0;
}
#endif /* HAVE_IPV6 */

int
nsm_rib_table_free (struct nsm_ptree_table *rt)
{
  struct nsm_ptree_node *tmp_node;
  struct nsm_ptree_node *node;
  struct rib *rib;
  struct rib *tmp_rib;

  if (rt == NULL)
    return 0;

  node = rt->top;

  while (node)
    {
      if (node->l_left)
        {
          node = node->l_left;
          continue;
        }

      if (node->l_right)
        {
          node = node->l_right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          if (node->l_left == tmp_node)
            node->l_left = NULL;
          else
            node->l_right = NULL;

          rib = tmp_node->info;
          while (rib)
            {
              tmp_rib = rib;
              rib = tmp_rib->next;

#ifdef HAVE_HA
              nsm_cal_delete_rib (tmp_rib, tmp_node);
#endif /* HAVE_HA */
              nsm_rib_free (tmp_rib);
            }

          nsm_ptree_node_free (tmp_node);
        }
      else
        {
          rib = tmp_node->info;
          while (rib)
            {
              tmp_rib = rib;
              rib = tmp_rib->next;

#ifdef HAVE_HA
              nsm_cal_delete_rib (tmp_rib, tmp_node);
#endif /* HAVE_HA */
              nsm_rib_free (tmp_rib);
            }

          nsm_ptree_node_free (tmp_node);
          break;
        }

    }

  XFREE (MTYPE_NSM_PTREE_TABLE, rt);
  return 0;
}

/* Static route configuration. */
struct nsm_static *
nsm_static_new (afi_t afi, u_char type, char *ifname)
{
  struct nsm_static *ns;

  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return NULL;

  ns = XCALLOC (MTYPE_NSM_STATIC, sizeof (struct nsm_static));
  ns->afi = afi;
  ns->type = type;
  if (ifname != NULL)
    ns->ifname = XSTRDUP (MTYPE_NSM_IFNAME, ifname);

  return ns;
}

void
nsm_static_free (struct nsm_static *ns)
{
  if (ns->ifname)
    XFREE (MTYPE_NSM_IFNAME, ns->ifname);
  if (ns->desc)
    XFREE (MTYPE_NSM_STATIC_DESC, ns->desc);

  XFREE (MTYPE_NSM_STATIC, ns);
}

struct nsm_static *
nsm_static_nexthop_copy (struct nsm_static *old_ns)
{
  struct nsm_static *ns;

  ns = XCALLOC (MTYPE_NSM_STATIC, sizeof (struct nsm_static));
  if (! ns)
    return NULL;

  /* Copy old static to new static */
  pal_mem_cpy (ns, old_ns, sizeof (struct nsm_static));

  return ns;
}

void
nsm_static_table_clean_all (struct nsm_vrf *nv, struct ptree *table)
{
  struct ptree_node *rn;
  struct nsm_static *ns, *next;

#ifndef HAVE_HA
  PAL_UNREFERENCED_PARAMETER (nv);
#endif /* HAVE_HA */

  for (rn = ptree_top (table); rn; rn = ptree_next (rn))
    {
      for (ns = rn->info; ns; ns = next)
        {
          next = ns->next;

#ifdef HAVE_HA
          nsm_cal_delete_nsm_static (ns, nv, rn);
#endif /* HAVE_HA */

          nsm_static_free (ns);
        }

      rn->info = NULL;
      ptree_unlock_node (rn);
    }

  XFREE (MTYPE_PTREE, table);
}

struct rib *
nsm_rib_match (struct nsm_vrf *vrf, afi_t afi, struct prefix *p,
               struct nsm_ptree_node **rn_ret, u_int32_t exclude_proto, u_char sub_type)
{
  struct nsm_ptree_node *rn = NULL;
  struct rib *match;
  struct nexthop *newhop;
  if (afi == AFI_IP)
    rn = nsm_ptree_node_match (vrf->afi[afi].rib[SAFI_UNICAST],
                               (u_char *)& p->u.prefix4, p->prefixlen);

#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = nsm_ptree_node_match (vrf->afi[afi].rib[SAFI_UNICAST],
                               (u_char *)& p->u.prefix6, p->prefixlen);
#endif

  while (rn)
    {
      nsm_ptree_unlock_node (rn);

      /* Pick up selected route. */
      for (match = rn->info; match; match = match->next)
        if (CHECK_FLAG (match->flags, RIB_FLAG_SELECTED))
          break;

      /* If there is no selected route or matched route is EGP, go up
         tree. */
      if (! match
          || ((exclude_proto & (1 << match->type)) && 
              (sub_type == match->sub_type)))
        {
          do {
            rn = rn->parent;
          } while (rn && rn->info == NULL);

          if (rn)
            nsm_ptree_lock_node (rn);
        }
      else
        {
          if (match->type == APN_ROUTE_CONNECT)
            {
              /* Directly point connected route. */
              *rn_ret = rn;
              return match;
            }
          else
            {
              for (newhop = match->nexthop; newhop; newhop = newhop->next)
                if (CHECK_FLAG (newhop->flags, NEXTHOP_FLAG_FIB))
                  {
                    *rn_ret = rn;
                    return match;
                  }

              return NULL;
            }
        }
    }

  return NULL;
}

struct rib *
nsm_rib_lookup_by_type (struct nsm_vrf *nv, afi_t afi,
                        struct prefix *p, int type)
{
  struct nsm_ptree_node *rn = NULL;
  struct rib *match;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return NULL;

 if (afi == AFI_IP)
   rn = nsm_ptree_node_lookup (nv->afi[afi].rib[SAFI_UNICAST], 
                               (u_char *)&p->u.prefix4,
                               p->prefixlen);

#ifdef HAVE_IPV6
 if (afi == AFI_IP6)
   rn = nsm_ptree_node_lookup (nv->afi[afi].rib[SAFI_UNICAST], 
                              (u_char *)&p->u.prefix6,
                              p->prefixlen);
#endif
  if (rn == NULL)
    return NULL;

  /* Unlock node. */
  nsm_ptree_unlock_node (rn);

  /* Pick the same type of route. */
  for (match = rn->info; match; match = match->next)
    if (match->type == type)
      return match;

  return NULL;
}

struct rib *
nsm_rib_lookup (struct nsm_vrf *vrf,
                afi_t afi,
                struct prefix *p,
                struct nsm_ptree_node **rn_ret,
                u_int32_t exclude_proto,
                u_char sub_type)
{
  struct nsm_ptree_node *rn = NULL;
  struct nexthop *nexthop;
  struct rib *match;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return NULL;

  if (afi == AFI_IP)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], 
                                (u_char *)&p->u.prefix4,
                                p->prefixlen);
#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], 
                                (u_char *)&p->u.prefix6,
                                p->prefixlen);
#endif
  if (rn == NULL)
    return NULL;

  /* Unlock node. */
  nsm_ptree_unlock_node (rn);

  /* Pick up selected route. */
  for (match = rn->info; match; match = match->next)
    if (CHECK_FLAG (match->flags, RIB_FLAG_SELECTED))
      break;

  if (! match || ((exclude_proto & (1 << match->type)) && 
      (sub_type == match->sub_type)))
    return NULL;

  if (match->type == APN_ROUTE_CONNECT)
    {
      if (rn_ret != NULL)
        *rn_ret = rn;
      return match;
    }

  for (nexthop = match->nexthop; nexthop; nexthop = nexthop->next)
    if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
      {
        if (rn_ret != NULL)
          *rn_ret = rn;
        return match;
      }

  return NULL;
}

result_t
nsm_nexthop_active_check (struct nsm_ptree_node *rn, struct rib *rib,
                          struct nexthop *nexthop, s_int32_t set)
{
  struct apn_vrf *vrf = rib->vrf->vrf;
  struct interface *ifp = NULL;

  switch (nexthop->type)
    {
    case NEXTHOP_TYPE_IFINDEX:
      ifp = ifv_lookup_by_index (&vrf->ifv, nexthop->ifindex);
      if (ifp != NULL)
        {
          if (vrf->id == 0)
            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);

          if (if_is_running (ifp))
            SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
          else
            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
        }
      break;
    case NEXTHOP_TYPE_IFNAME:
    case NEXTHOP_TYPE_IPV6_IFNAME:
      if (IS_NULL_INTERFACE_STR (nexthop->ifname))
        SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
      else
        {
          /* Should check VRF? */
          ifp = if_lookup_by_name (&vrf->vr->ifm, nexthop->ifname);
          if (ifp != NULL
              && if_is_running (ifp)
              && ((rn->tree->ptype == AF_INET
                   && nexthop->type == NEXTHOP_TYPE_IFNAME
                   && ifp->ifc_ipv4 != NULL)
#ifdef HAVE_IPV6
                  || (rn->tree->ptype == AF_INET6
                      && (nexthop->type == NEXTHOP_TYPE_IFNAME
                          || nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
                      && ifp->ifc_ipv6 != NULL)
#endif /* HAVE_IPV6 */
                  ))
            {
              if (set)
                {
                  nexthop->ifindex = ifp->ifindex;
#ifdef HAVE_HA
                  nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
                }
              SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
            }
          else
            {
              if (set)
                {
                  nexthop->ifindex = 0;
#ifdef HAVE_HA
                  nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
                }
              UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
            }
        }
      break;
    case NEXTHOP_TYPE_IPV4:
    case NEXTHOP_TYPE_IPV4_IFINDEX:
    case NEXTHOP_TYPE_IPV4_IFNAME:
#ifdef HAVE_VRF
    case NEXTHOP_TYPE_MPLS:
#endif /* HAVE_VRF */
      if (nsm_nexthop_active_ipv4_vrf (rib, nexthop, set, rn))
        SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
      else
        {
          UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
          if (nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX)
            {
              nexthop->type = NEXTHOP_TYPE_IPV4;
              nexthop->ifindex = 0;
#ifdef HAVE_HA
              nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
            }
        }
      break;
#ifdef HAVE_IPV6
    case NEXTHOP_TYPE_IPV6:
      if (NSM_CAP_HAVE_IPV6)
        {
          if (nsm_nexthop_active_ipv6_vrf (rib, nexthop, set, rn))
            SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
          else
            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
        }
      break;
    case NEXTHOP_TYPE_IPV6_IFINDEX:
      if (NSM_CAP_HAVE_IPV6)
        {
          if (IN6_IS_ADDR_LINKLOCAL (&nexthop->gate.ipv6))
            {
              ifp = ifv_lookup_by_index (&vrf->ifv, nexthop->ifindex);
              if (ifp && if_is_running (ifp))
                SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
              else
                UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
            }
#ifdef HAVE_TUNNEL_IPV6
          else if ((ifp = ifv_lookup_by_index (&vrf->ifv, nexthop->ifindex))
                   && if_is_6to4_tunnel (ifp))
            {
              if (if_is_running (ifp))
                SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
              else
                UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
            }
#endif /* HAVE_TUNNEL_IPV6 */
          else
            {
              if (nsm_nexthop_active_ipv6_vrf (rib, nexthop, set, rn))
                SET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
              else
                {
                  UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
                  nexthop->type = NEXTHOP_TYPE_IPV6;
                  nexthop->ifindex = 0;
#ifdef HAVE_HA
                  nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
                }
            }
        }
      break;
#endif /* HAVE_IPV6 */
    default:
      break;
    }

/* If BFD session_down has set the nexthop inactive update the status */
#ifdef HAVE_BFD
  if (rib->type == APN_ROUTE_STATIC )
    {
      if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_BFD_INACTIVE))
        UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
    }
#endif /* HAVE_BFD */
  return CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE);
}

int
nsm_nexthop_active_update (struct nsm_ptree_node *rn, struct rib *rib, int set)
{
  int active_num = rib->nexthop_active_num;
  int active, ifindex;
  int rib_recursive_blackhole;
  struct nexthop *nh;

  UNSET_FLAG (rib->flags, RIB_FLAG_CHANGED);
  rib->nexthop_active_num = 0;

  rib_recursive_blackhole = CHECK_FLAG (rib->ext_flags, 
      RIB_EXT_FLAG_BLACKHOLE_RECURSIVE);
  UNSET_FLAG (rib->ext_flags, RIB_EXT_FLAG_BLACKHOLE_RECURSIVE);

  for (nh = rib->nexthop; nh; nh = nh->next)
    {
      active = CHECK_FLAG (nh->flags, NEXTHOP_FLAG_ACTIVE);

      if (CHECK_FLAG (nh->flags, NEXTHOP_FLAG_RECURSIVE))
       {
        ifindex = nh->rifindex;
        /* Backup the ifindex for BFD session handling */
#ifdef HAVE_BFD
        nh->bfd_ifindex = nh->rifindex;
#endif /* HAVE_BFD */
       }
      else
       {
        ifindex = nh->ifindex;
        /* Backup the ifindex for BFD session handling */
#ifdef HAVE_BFD
        nh->bfd_ifindex = nh->ifindex;
#endif /* HAVE_BFD */
       }

      rib->nexthop_active_num +=
        nsm_nexthop_active_check (rn, rib, nh, set);

      if ((active != CHECK_FLAG (nh->flags, NEXTHOP_FLAG_ACTIVE))
          || (! CHECK_FLAG (nh->flags, NEXTHOP_FLAG_RECURSIVE) && 
            ifindex != nh->ifindex)
          || (CHECK_FLAG (nh->flags, NEXTHOP_FLAG_RECURSIVE) && 
            ifindex != nh->rifindex))
        SET_FLAG (rib->flags, RIB_FLAG_CHANGED);

      if (CHECK_FLAG (nh->flags, NEXTHOP_FLAG_RECURSIVE_BLACKHOLE))
        SET_FLAG (rib->ext_flags, RIB_EXT_FLAG_BLACKHOLE_RECURSIVE);
    }

  if (rib->nexthop_active_num != active_num)
    SET_FLAG (rib->flags, RIB_FLAG_CHANGED);

  if (rib_recursive_blackhole != CHECK_FLAG (rib->ext_flags, 
      RIB_EXT_FLAG_BLACKHOLE_RECURSIVE))
    SET_FLAG (rib->flags, RIB_FLAG_CHANGED);

#ifdef HAVE_HA
  if (CHECK_FLAG (rib->flags, RIB_FLAG_CHANGED))
    {
      SET_FLAG (rib->ext_flags, RIB_EXT_FLAG_HA_RIB_CHANGED);
      nsm_cal_modify_rib (rib, rn);
    }
#endif /* HAVE_HA */

  return rib->nexthop_active_num;
}

int
nsm_rib_nexthop_same (struct rib *select, struct rib *fib)
{
  struct nexthop *nh, *nh_fib;
  struct pal_in4_addr *gate4;
#ifdef HAVE_IPV6
  struct pal_in6_addr *gate6;
#endif /* HAVE_IPV6 */
  int ifindex;
  int recursive;
  int type;

  if (fib->nexthop_num != select->nexthop_num)
    return 0;

  for (nh = select->nexthop; nh; nh = nh->next)
    {
      if (CHECK_FLAG (nh->flags, NEXTHOP_FLAG_RECURSIVE))
        recursive = 1;
      else
        recursive = 0;

      type = (recursive == 0) ? nh->type : nh->rtype;

      for (nh_fib = fib->nexthop; nh_fib; nh_fib = nh_fib->next)
        {
          if (type == NEXTHOP_TYPE_IFINDEX
              || type == NEXTHOP_TYPE_IFNAME)
            {
              if (nh_fib->type != NEXTHOP_TYPE_IFINDEX
                  && nh_fib->type != NEXTHOP_TYPE_IFNAME)
                continue;

              ifindex = (recursive == 0) ? nh->ifindex : nh->rifindex;
              if (nh_fib->ifindex != ifindex)
                continue;

              /* Match.  */
              break;
            }

          if (type == NEXTHOP_TYPE_IPV4_IFINDEX
              || type == NEXTHOP_TYPE_IPV4_IFNAME)
            {
              if (nh_fib->type != NEXTHOP_TYPE_IPV4_IFINDEX
                  && nh_fib->type != NEXTHOP_TYPE_IPV4_IFNAME)
                continue;

              ifindex = (recursive == 0) ? nh->ifindex : nh->rifindex;
              if (nh_fib->ifindex != ifindex)
                continue;

              gate4 = (recursive == 0) ? &nh->gate.ipv4 : &nh->rgate.ipv4;
              if (! IPV4_ADDR_SAME (&nh_fib->gate.ipv4, gate4))
                continue;

              /* Match.  */
              break;
            }

          if (type == NEXTHOP_TYPE_IPV4)
            {
              if (nh_fib->type != NEXTHOP_TYPE_IPV4)
                continue;

              gate4 = (recursive == 0) ? &nh->gate.ipv4 : &nh->rgate.ipv4;
              if (! IPV4_ADDR_SAME (&nh_fib->gate.ipv4, gate4))
                continue;

              /* Match.  */
              break;
            }

#ifdef HAVE_IPV6
          if (type == NEXTHOP_TYPE_IPV6_IFINDEX
              || type == NEXTHOP_TYPE_IPV6_IFNAME)
            {
              if (nh_fib->type != NEXTHOP_TYPE_IPV6_IFINDEX
                  && nh_fib->type != NEXTHOP_TYPE_IPV6_IFNAME)
                continue;

              ifindex = (recursive == 0) ? nh->ifindex : nh->rifindex;
              if (nh_fib->ifindex != ifindex)
                continue;

              gate6 = (recursive == 0) ? &nh->gate.ipv6 : &nh->rgate.ipv6;
              if (! IPV6_ADDR_SAME (&nh_fib->gate.ipv6, gate6))
                continue;

              /* Match.  */
              break;
            }

          if (type == NEXTHOP_TYPE_IPV6)
            {
              if (nh_fib->type != NEXTHOP_TYPE_IPV6)
                continue;

              gate6 = (recursive == 0) ? &nh->gate.ipv6 : &nh->rgate.ipv6;
              if (! IPV6_ADDR_SAME (&nh_fib->gate.ipv6, gate6))
                continue;

              /* Match.  */
              break;
            }
#endif /* HAVE_IPV6 */
        }

      /* Return 0 if there is no matched nexthop entry.  */
      if (nh_fib == NULL)
        return 0;
    }

  return 1;
}

void
nsm_rib_nexthop_swap (struct rib *select, struct rib *fib)
{
  struct nexthop *nh;

  for (nh = select->nexthop; nh; nh = nh->next)
    SET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);

  for (nh = fib->nexthop; nh; nh = nh->next)
    UNSET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);
}

int
nsm_rib_fib_add (struct nsm_ptree_node *rn, struct rib *rib)
{
  struct nexthop *nh;
  int ret = 0;
  struct prefix p;

  pal_mem_set(&p, 0, sizeof(struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;

  if (rn->tree->ptype == AF_INET)
    {
      NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

      /* Ignore loopback prefixes */
      if (IN_LOOPBACK (pal_ntoh32 (p.u.prefix4.s_addr)))
        return RESULT_OK;
    }

#ifdef HAVE_IPV6
   if (rn->tree->ptype == AF_INET6)
     {
       NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);

       /* Ignore loopback and linklocal prefixes */
       if (IN6_IS_ADDR_LOOPBACK (&p.u.prefix6) ||
           IN6_IS_ADDR_LINKLOCAL (&p.u.prefix6))
         return RESULT_OK;
     }
#endif

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "RIB[%s:%O]: Add to FIB",
               VRF_NAME (rib->vrf->vrf), &p);

  switch (rn->tree->ptype)
    {
    case AF_INET:
      ret = nsm_fea_ipv4_add (&p, rib);
      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      IF_NSM_CAP_HAVE_IPV6
        ret = nsm_fea_ipv6_add (&p, rib);
      break;
#endif /* HAVE_IPV6 */
    }

  if (ret < 0)
    for (nh = rib->nexthop; nh; nh = nh->next)
      UNSET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);

  return ret;
}

int
nsm_rib_fib_delete (struct nsm_ptree_node *rn, struct rib *rib)
{
  struct nexthop *nh;
  int ret = 0;
  struct prefix p;

  pal_mem_set(&p, 0, sizeof(struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;
  if (rn->tree->ptype == AF_INET)
    {
      NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

      /* Ignore loopback prefixes */
      if (IN_LOOPBACK (pal_ntoh32 (p.u.prefix4.s_addr)))
        return RESULT_OK;
    }

#ifdef HAVE_IPV6
   if (rn->tree->ptype == AF_INET6)
     {
       NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);

       /* Ignore loopback and linklocal prefixes */
       if (IN6_IS_ADDR_LOOPBACK (&p.u.prefix6) ||
           IN6_IS_ADDR_LINKLOCAL (&p.u.prefix6))
         return RESULT_OK;
     }
#endif

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "RIB[%s:%O]: Delete from FIB",
               VRF_NAME (rib->vrf->vrf), &p);

  switch (rn->tree->ptype)
    {
    case AF_INET:
      ret = nsm_fea_ipv4_delete (&p, rib);
      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      IF_NSM_CAP_HAVE_IPV6
        ret = nsm_fea_ipv6_delete (&p, rib);
      break;
#endif /* HAVE_IPV6 */
    }

  for (nh = rib->nexthop; nh; nh = nh->next)
    UNSET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);

  return ret;
}

int
nsm_rib_fib_update (struct nsm_ptree_node *rn,
                    struct rib *fib, struct rib *select)
{
  struct nexthop *nh;
  int ret = 0;
  struct prefix p;

  pal_mem_set(&p, 0, sizeof(struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;
  if (rn->tree->ptype == AF_INET)
    {
      NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

      /* Ignore loopback prefixes */
      if (IN_LOOPBACK (pal_ntoh32 (p.u.prefix4.s_addr)))
        return RESULT_OK;
    }

#ifdef HAVE_IPV6
   if (rn->tree->ptype == AF_INET6)
     {
       NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);

       /* Ignore loopback and linklocal prefixes */
       if (IN6_IS_ADDR_LOOPBACK (&p.u.prefix6) ||
           IN6_IS_ADDR_LINKLOCAL (&p.u.prefix6))
         return RESULT_OK;
     }
#endif

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "RIB[%s:%O]: Update FIB",
               VRF_NAME (fib->vrf->vrf), &p);

  switch (rn->tree->ptype)
    {
    case AF_INET:
      ret = nsm_fea_ipv4_update (&p, fib, select);
      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      IF_NSM_CAP_HAVE_IPV6
        ret = nsm_fea_ipv6_update (&p, fib, select);
      break;
#endif /* HAVE_IPV6 */
    }

  if (ret < 0)
    for (nh = select->nexthop; nh; nh = nh->next)
      UNSET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);

  /* fib and select will be the same for the static route update case.  */
  else if (fib != select)
    for (nh = fib->nexthop; nh; nh = nh->next)
      UNSET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);

  return ret;
}

#ifdef HAVE_STAGGER_KERNEL_MSGS
/* Routine to free up a node in the kernel msg stagger list. */
void
nsm_kernel_msg_stagger_node_free (struct nsm_kernel_msg_stagger_node *node)
{
  if (node)
    {
      if (node->rib)
        node->rib->kernel_ms_lnode = NULL;
      XFREE (MTYPE_NSM_STAGGER_NODE, node);
    }
}

/* Kernel msg stagger timer callback handler. */
int
nsm_kernel_msg_stagger_timer (struct thread *t)
{
  struct nsm_kernel_msg_stagger_node *node;
  struct pal_timeval timer_delay = {0};
  struct pal_timeval start_time = {0};
  struct listnode *ln, *ln_next;
  struct nsm_master *nm;
  u_int32_t run_time = 0;
  int count;
  struct prefix p;


  if (! t)
    return 0;

  nm = THREAD_ARG (t);
  nm->t_kernel_msg_stagger = NULL;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Kernel MSG Stagger Timer(): %u MSGs in list",
               LISTCOUNT (nm->kernel_msg_stagger_list));

  pal_time_tzcurrent (&start_time, NULL);

  count = 0;
  for (ln = LISTHEAD (nm->kernel_msg_stagger_list); ln; ln = ln_next)
    {
      ln_next = ln->next;
      count++;
      if (count > KERNEL_MSG_WRITE_MAX)
        {
          pal_time_tzcurrent (&timer_delay, NULL);
          run_time = timer_delay.tv_sec - start_time.tv_sec;

          if (run_time >= KERNEL_MSG_WRITE_INTERVAL_MAX)
            {
              timer_delay.tv_sec = KERNEL_MSG_STAGGER_TIMER_RESTART_S;
              timer_delay.tv_usec = KERNEL_MSG_STAGGER_TIMER_RESTART_US;

              /* Relaunch timer. */
              nsm_kernel_msg_stagger_timer_start (nm, &timer_delay);

              return 0;
            }

          /* Write another bunch of routes into kernel */
          count = 0;
        }

      /* Add route to kernel */
      node = GETDATA (ln);
      nsm_rib_fib_add (node->rn, node->rib);

      pal_mem_set(&p, 0, sizeof(struct prefix));
      p.prefixlen = rn->key_len;
      p.family = rn->tree->ptype;
      if (rn->tree->ptype == AF_INET)
        NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

#ifdef HAVE_IPV6
      if (rn->tree->ptype == AF_INET6)
        NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif

      /* Continue with route redistribution */
      nsm_redistribute_add (&p, node->rib);
#ifdef HAVE_CRX
      crx_rib_sync (node->rn, node->rib);
#endif /* HAVE_CRX */

      /* Send change notifications for new nexthop */
      nsm_nexthop_process_rib_add (node->rn, node->rib);

      /* Handle change in unicast rib for mrib */
      nsm_mrib_handle_unicast_rib_change (node->rib->vrf, &node->rn);

      /* Free Stagger Node */
      nsm_kernel_msg_stagger_node_free (node);
      list_delete_node (nm->kernel_msg_stagger_list, ln);
    }

  return 0;
}

/* Routine to start kernel msg stagger timer. */
void
nsm_kernel_msg_stagger_timer_start (struct nsm_master *nm,
                                    struct pal_timeval *timer_delay)
{
  /* If no top, or thread is already running, return. */
  if (! nm || nm->t_kernel_msg_stagger)
    return;

  /* Start timer. */
  nm->t_kernel_msg_stagger =
    thread_add_timer_timeval (nzg, nsm_kernel_msg_stagger_timer,
                              nm, *timer_delay);
}

/* Routine to stop kernel msg stagger timer. */
void
nsm_kernel_msg_stagger_timer_stop (struct nsm_master *nm)
{
  /* If no top, or thread is already stopped, return. */
  if (! nm || ! nm->t_kernel_msg_stagger)
    return;

  /* Stop thread. */
  THREAD_OFF (nm->t_kernel_msg_stagger);
}

/* Routine to add a node to the kernel message stagger list. */
void
nsm_kernel_msg_stagger_add (struct nsm_master *nm,
                            struct nsm_ptree_node *rn,
                            struct rib *rib)
{
  struct nsm_kernel_msg_stagger_node *node;
  struct listnode *ln;

  if (! nm || ! nm->kernel_msg_stagger_list)
    return;

  /* Create node. */
  node = XCALLOC (MTYPE_NSM_STAGGER_NODE,
                  sizeof (struct nsm_kernel_msg_stagger_node));
  if (! node)
    return;
  node->rn = rn;
  node->rib = rib;

  ln = listnode_add (nm->kernel_msg_stagger_list, node);
  if (! ln)
    XFREE (MTYPE_NSM_STAGGER_NODE, node);
  else
    rib->kernel_ms_lnode = ln;

  return;
}

/* Clean up all stagger data. */
void
nsm_kernel_msg_stagger_clean_all (struct nsm_master *nm)
{
  struct nsm_kernel_msg_stagger_node *node;
  struct listnode *ln, *ln_next;

  if (nm == NULL)
    return;

  /* Stop timer. */
  THREAD_OFF (nm->t_kernel_msg_stagger);

  /* Iterate and delete. */
  for (ln = LISTHEAD (nm->kernel_msg_stagger_list); ln; ln = ln_next)
    {
      ln_next = ln->next;
      node = GETDATA (ln);
      nsm_kernel_msg_stagger_node_free (node);
      list_delete_node (nm->kernel_msg_stagger_list, ln);
    }
}
#endif /* HAVE_STAGGER_KERNEL_MSGS */

int
nsm_rib_install_kernel (struct nsm_master *nm,
                        struct nsm_ptree_node *rn, struct rib *rib)
{
  int ret = 0;
#ifdef HAVE_STAGGER_KERNEL_MSGS
  struct pal_timeval timer_delay = {0};
#endif /* HAVE_STAGGER_KERNEL_MSGS */

  if (CHECK_FLAG (rib->flags, RIB_FLAG_NON_FIB))
    return ret;

#ifdef HAVE_STAGGER_KERNEL_MSGS
  nsm_kernel_msg_stagger_add (nm, rn, rib);

  /* Start stagger timer if not already running. */
  if (! nm->t_kernel_msg_stagger)
    {
      timer_delay.tv_sec = KERNEL_MSG_STAGGER_TIMER_START_S;
      timer_delay.tv_usec = KERNEL_MSG_STAGGER_TIMER_START_US;

      nsm_kernel_msg_stagger_timer_start (nm, &timer_delay);
    }

  return 1;
#else
  ret = nsm_rib_fib_add (rn, rib);

  return ret;
#endif /* HAVE_STAGGER_KERNEL_MSGS */
}

void
nsm_rib_ipaddr_chng_ipv4_fib_update (struct nsm_master *nm,
                                     struct nsm_vrf *vrf,
                                     struct connected *ifc)
{
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib = NULL;
  struct nexthop *nexthop = NULL;
  struct prefix p1;
  struct prefix p2;
#ifdef HAVE_HA
  bool_t flag = PAL_FALSE;
#endif /* HAVE_HA */

  prefix_copy (&p1, ifc->address);
  apply_mask (&p1);

  for (rn = nsm_ptree_top (vrf->RIB_IPV4_UNICAST); rn; rn = nsm_ptree_next(rn))
    for (rib = rn->info; rib; rib = rib->next)
      {
        /* Considering only the static routes that are selected */
        if (rib->type != APN_ROUTE_STATIC ||
            ! CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
          continue ;

        for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
          {
            /* Considering only the nexthops that are in FIB */
            if (! CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
              continue ;

            if (nexthop->ifindex == ifc->ifp->ifindex)
              {
                if (CHECK_FLAG (nexthop->type, NEXTHOP_TYPE_IPV4_IFINDEX))
                  {
                    prefix_copy (&p2, &p1);
                    p2.u.prefix4 = nexthop->gate.ipv4;
                    apply_mask (&p2);

                    if (IPV4_ADDR_SAME (&p1.u.prefix4, &p2.u.prefix4))
                      {
                        nsm_rib_install_kernel (nm, rn, rib);
#ifdef HAVE_HA
                        flag = PAL_TRUE;
#endif /* HAVE_HA */
                      }
                  }
                else if (CHECK_FLAG (nexthop->type, NEXTHOP_TYPE_IFNAME))
                  {
                    nsm_rib_install_kernel (nm, rn, rib);
#ifdef HAVE_HA
                    flag = PAL_TRUE;
#endif /* HAVE_HA */
                  }
              }
            else if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_RECURSIVE))
              {
                prefix_copy (&p2, &p1);
                p2.u.prefix4 = nexthop->rgate.ipv4;
                apply_mask (&p2);

                if (IPV4_ADDR_SAME (&p1.u.prefix4, &p2.u.prefix4))
                  {
                    nsm_rib_install_kernel (nm, rn, rib);
#ifdef HAVE_HA
                    flag = PAL_TRUE;
#endif /* HAVE_HA */
                  }
              }

#ifdef HAVE_HA
            if (flag)
              {
                SET_FLAG (rib->ext_flags, RIB_EXT_FLAG_HA_RIB_CHANGED);
                nsm_cal_modify_rib (rib, rn);
              }
            flag = PAL_FALSE;
#endif /* HAVE_HA */
          }
      }
}

int
nsm_rib_update_kernel (struct nsm_master *nm, struct nsm_ptree_node *rn,
                       struct rib *fib, struct rib *select)
{
  int ret = 0;
  /* Just swap the nexthop FIB flag if the Old-RIB and the new-RIB
     has the same nexthops and two RIBs are not identical.  Otherwise,
     send the message to the kernel to update the FIB.  */
  if (select != fib && nsm_rib_nexthop_same (select, fib))
    {
      /* Swap the nexthop FIB flag.  */
      nsm_rib_nexthop_swap (select, fib);
      return ret;
    }

  if (CHECK_FLAG (select->flags, RIB_FLAG_NON_FIB))
    return ret;

#ifdef HAVE_STAGGER_KERNEL_MSGS
  if (select->kernel_ms_lnode != NULL)
    return 1;
#endif /* HAVE_STAGGER_KERNEL_MSGS */

  ret = nsm_rib_fib_update (rn, fib, select);

  return ret;
}

/* Uninstall the route from kernel. */
result_t
nsm_rib_uninstall_kernel (struct nsm_ptree_node *rn, struct rib *rib)
{
#ifdef HAVE_STAGGER_KERNEL_MSGS
  struct nsm_master *nm = rib->vrf->nm;
  struct nsm_kernel_msg_stagger_node *node;
  struct listnode *ln;
#endif /* HAVE_STAGGER_KERNEL_MSGS */

  if (CHECK_FLAG (rib->flags, RIB_FLAG_NON_FIB))
    return RESULT_OK;

#ifdef HAVE_STAGGER_KERNEL_MSGS
  /* Remove Stagger Node from Stagger List */
  if (rib->kernel_ms_lnode)
    {
      ln = rib->kernel_ms_lnode;
      node = GETDATA (ln);
      list_delete_node (nm->kernel_msg_stagger_list, ln);
      nsm_kernel_msg_stagger_node_free (node);

      return RESULT_OK;
    }
#endif /* HAVE_STAGGER_KERNEL_MSGS */

  return nsm_rib_fib_delete (rn, rib);
}

void
nsm_rib_process_add (struct nsm_master *nm,
                     struct nsm_ptree_node *rn, struct rib *select)
{
  struct nsm_ptree_node *rn_child;
  struct nsm_ptree_table *nh_table = NULL;
  struct nsm_vrf *vrf;
  bool_t kernel_deferred;  
  struct prefix p;
#ifdef HAVE_BFD
  afi_t afi = AFI_IP;
#endif /* HAVE_BFD */

  pal_mem_set(&p, 0, sizeof(struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;
  if (rn->tree->ptype == AF_INET)
    NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
  
#ifdef HAVE_IPV6
   if (rn->tree->ptype == AF_INET6)
     NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif

  kernel_deferred = PAL_FALSE;

  /* Set real nexthop. */
  nsm_nexthop_active_update (rn, select, 1);

  if (select->type != APN_ROUTE_CONNECT
      && if_lookup_by_prefix (&nm->vr->ifm, &p))
    return;

  SET_FLAG (select->flags, RIB_FLAG_SELECTED);
#ifdef HAVE_HA
  nsm_cal_modify_rib (select, rn);
#endif /* HAVE_HA */

if (rn->tree->ptype == AF_INET)
  {
    if (!RIB_SYSTEM_ROUTE (select))
      kernel_deferred = nsm_rib_install_kernel (nm, rn, select);
  }

#ifdef HAVE_IPV6
 if (rn->tree->ptype == AF_INET6)
   {
     if (!RIB_SYSTEM_ROUTE (select) && !RIB_CONNECT_ROUTE (select))
       kernel_deferred = nsm_rib_install_kernel (nm, rn, select);
   }
#endif /* HAVE_IPV6 */
  
  if (!kernel_deferred)
    {
      vrf = select->vrf;
      if (rn->tree->ptype == AF_INET)
        nh_table = vrf->afi[AFI_IP].nh_reg;
#ifdef HAVE_IPV6
      else if (NSM_CAP_HAVE_IPV6)
        nh_table = vrf->afi[AFI_IP6].nh_reg;
#endif /* HAVE_IPV6 */
      nsm_nexthop_process_route_node_add (rn, nh_table);

      nsm_redistribute_add (&p, select);
#ifdef HAVE_CRX
      crx_rib_sync (rn, select);
#endif /* HAVE_CRX */

      /* Send change notifications for new nexthop. */
      if (!CHECK_FLAG (select->flags, RIB_FLAG_NON_FIB))
        {
          nsm_nexthop_process_rib_add (rn, select);

          /* Handle change in unicast rib for mrib */
          nsm_mrib_handle_unicast_rib_change (select->vrf, &p);

          /* Schedule the RIB update timer.  */
          nsm_rib_update_timer_add (select->vrf, family2afi (p.family));
        }
    }

/* Add BFD session for the RIB entry nexthops */
#ifdef HAVE_BFD
    if (select->type == APN_ROUTE_STATIC)
      { 
        if (rn->tree->ptype == AF_INET)
          afi = AFI_IP;
#ifdef HAVE_IPV6
        else if (rn->tree->ptype == AF_INET6)
          afi = AFI_IP6;
#endif /* HAVE_IPV6 */
        else
          return;

        /* Function to add BFD session for all the RIB nexthop entries */
        if (!nsm_bfd_update_session_by_rib (select->vrf, select, afi, NULL, rn, 
                                                                0, session_add))
          {
            if (IS_NSM_DEBUG_BFD)
              zlog_err (NSM_ZG, " BFD session add for static route failed");
          }
        UNSET_FLAG (select->pflags, NSM_ROUTE_CHG_BFD);
      }
#endif /* HAVE_BFD */

  /* Re-process all routes more-specific than CONNECTED route */
  if (select->type == APN_ROUTE_CONNECT)
    for (nsm_ptree_lock_node (rn), rn_child = nsm_ptree_next_until (rn, rn);
         rn_child;
         rn_child = nsm_ptree_next_until (rn_child, rn))
      {
        if (! rn_child->info)
          continue;

        nsm_rib_process (nm, rn_child, NULL);
      }

  return;
}

void
nsm_rib_process_delete (struct nsm_master *nm,
                        struct nsm_ptree_node *rn, struct rib *fib)
{
  struct nsm_ptree_node *prev_node = NULL;
  struct nsm_ptree_table *nh_table = NULL;
  struct nsm_vrf *vrf;
  struct prefix p;
#ifdef HAVE_BFD
  afi_t afi = AFI_IP;
#endif /* HAVE_BFD */

  pal_mem_set(&p, 0, sizeof(struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;
  if (rn->tree->ptype == AF_INET)
    NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

#ifdef HAVE_IPV6
   if (rn->tree->ptype == AF_INET6)
     NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif

/* Delete BFD session for this RIB entry nexthops */
#ifdef HAVE_BFD
    if ((fib->type == APN_ROUTE_STATIC))
      {
        if (rn->tree->ptype == AF_INET)
          afi = AFI_IP;
#ifdef HAVE_IPV6
        else if (rn->tree->ptype == AF_INET6)
          afi = AFI_IP6;
#endif /* HAVE_IPV6 */
        else
          return;

        /* Function to delete BFD session for all the RIB nexthop entries */
        if (!nsm_bfd_update_session_by_rib (fib->vrf, fib, afi, NULL, rn, 
                                                         0, session_del))
          {
            if (IS_NSM_DEBUG_BFD)
              zlog_err (NSM_ZG, "BFD session delete for static route failed");

          }
      }
#endif /* HAVE_BFD */

#ifdef HAVE_CRX
  crx_rib_withdraw (rn, fib);
#endif /* HAVE_CRX */
  nsm_redistribute_delete (&p, fib);
  if (!RIB_SYSTEM_ROUTE (fib))
    nsm_rib_uninstall_kernel (rn, fib);
  UNSET_FLAG (fib->flags, RIB_FLAG_SELECTED);
#ifdef HAVE_HA
  nsm_cal_modify_rib (fib, rn);
#endif /* HAVE_HA */

  /* Set real nexthop. */
  nsm_nexthop_active_update (rn, fib, 1);
#ifdef HAVE_BFD
  UNSET_FLAG (fib->pflags, NSM_ROUTE_CHG_BFD);
#endif /* HAVE_BFD */

 /* If new rib is not present, send delete if there is no
     parent for rn. */
  vrf = fib->vrf;
  if (rn->tree->ptype == AF_INET)
    nh_table = vrf->afi[AFI_IP].nh_reg;
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6)
    nh_table = vrf->afi[AFI_IP6].nh_reg;
#endif /* HAVE_IPV6 */

  prev_node = nsm_nexthop_process_route_node_delete (rn, fib, nh_table);
  if (prev_node)
    nsm_nexthop_process_route_node (prev_node);

  /* Handle change in unicast rib for mrib */
  nsm_mrib_handle_unicast_rib_change (fib->vrf, &p);

  /* Schedule the RIB update timer.  */
  nsm_rib_update_timer_add (fib->vrf, family2afi (p.family));

}

void
nsm_rib_process_update (struct nsm_master *nm, struct nsm_ptree_node *rn,
                        struct rib *fib, struct rib *select)
{
  bool_t kernel_deferred;
  struct nexthop *nh;
  struct prefix p;
#ifdef HAVE_BFD
  afi_t afi = AFI_IP;
#endif /* HAVE_BFD */

  pal_mem_set(&p, 0, sizeof(struct prefix));
  p.prefixlen = rn->key_len;
  p.family = rn->tree->ptype;
  if (rn->tree->ptype == AF_INET)
    NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);

#ifdef HAVE_IPV6
   if (rn->tree->ptype == AF_INET6)
     NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
#endif
  kernel_deferred = PAL_FALSE;
  nh = NULL;

  /* Same RIB is selected, so we do nothing. */
  if (select == fib
      && ! CHECK_FLAG (select->flags, RIB_FLAG_CHANGED)
      && ! CHECK_FLAG (nm->flags, NSM_MULTIPATH_REFRESH))
    return;

#ifdef HAVE_HA
  /* Make sure that CHANGED flag is set in the checkpoint record
   * for multipath refresh operation. This ensures that the standby
   * will update the FIB with new nexthops
   */
  if (select == fib
      && CHECK_FLAG (nm->flags, NSM_MULTIPATH_REFRESH))
    {
      SET_FLAG (fib->ext_flags, RIB_EXT_FLAG_HA_RIB_CHANGED);
      nsm_cal_modify_rib (fib, rn);
    }
#endif /* HAVE_HA */

#ifdef HAVE_CRX
  crx_rib_withdraw (rn, fib);
#endif /* HAVE_CRX */
  nsm_redistribute_delete (&p, fib);

  /* Select real nexthop for new RIB. */
  nsm_nexthop_active_update (rn, select, 1);

      for (nh = select->nexthop; nh; nh = nh->next)
        SET_FLAG (nh->flags, NEXTHOP_FLAG_FIB);

  /* If old-RIB is SYSTEM and new-RIB is not SYSTEM. */
  if (RIB_SYSTEM_ROUTE (fib) && !RIB_SYSTEM_ROUTE (select))
    {
      if (fib->type == APN_ROUTE_KERNEL
          && fib->distance == DISTANCE_INFINITY
          && CHECK_FLAG (fib->flags, RIB_FLAG_SELFROUTE))
        {
          /* Update the FIB in the kernel. */
          kernel_deferred = nsm_rib_update_kernel (nm, rn, fib, select);

          UNSET_FLAG (fib->flags, RIB_FLAG_STALE);
#ifdef HAVE_HA
          nsm_cal_modify_rib (fib, rn);
#endif /* HAVE_HA */
        }
      else
        kernel_deferred = nsm_rib_install_kernel (nm, rn, select);
    }

  /* If old-RIB is not SYSTEM and new-RIB is SYSTEM. */
  else if (!RIB_SYSTEM_ROUTE (fib) && RIB_SYSTEM_ROUTE (select))
    kernel_deferred = nsm_rib_uninstall_kernel (rn, fib);

  /* If old-RIB nor new-RIB is not SYSTEM. */
  else if (!RIB_SYSTEM_ROUTE (fib) && !RIB_SYSTEM_ROUTE (select))
    kernel_deferred = nsm_rib_update_kernel (nm, rn, fib, select);

  /* Otherwise, no need to update the kernel FIB. */

  /* Set real nexthop for FIB. */
  if (select != fib)
    {
      nsm_nexthop_active_update (rn, fib, 1);
      
      UNSET_FLAG (fib->flags, RIB_FLAG_SELECTED);
      SET_FLAG (select->flags, RIB_FLAG_SELECTED);
#ifdef HAVE_HA
      nsm_cal_modify_rib (fib, rn);
      nsm_cal_modify_rib (select, rn);
#endif /* HAVE_HA */
    }

  if (! kernel_deferred)
    {
      nsm_redistribute_add (&p, select);
#ifdef HAVE_CRX
      crx_rib_sync (rn, select);
#endif /* HAVE_CRX */

      /* Send change notifications for new nexthop. */
      nsm_nexthop_process_rib_add (rn, select);

      /* Handle change in unicast rib for mrib */
      nsm_mrib_handle_unicast_rib_change (select->vrf, &p);

      /* Schedule the RIB update timer.  */
      nsm_rib_update_timer_add (select->vrf, family2afi (p.family));
    }

/* Delete the BFD session for this RIB, for update, we delete all old RIB
   and add all new RIB nexthops. Adding is taken care here */
#ifdef HAVE_BFD
  if (select->type == APN_ROUTE_STATIC)
    {
      if (rn->tree->ptype == AF_INET)
        afi = AFI_IP;
#ifdef HAVE_IPV6
      else if (rn->tree->ptype == AF_INET6)
        afi = AFI_IP6;
#endif /* HAVE_IPV6 */
      else
        return;

      if (!nsm_bfd_update_session_by_rib (select->vrf, select, afi, NULL, rn, 
                                                             0, session_add))
        {
          if (IS_NSM_DEBUG_BFD)
              zlog_err (NSM_ZG, "BFD session update for static route failed");
        }
      UNSET_FLAG (select->pflags, NSM_ROUTE_CHG_BFD);
    }
#endif /* HAVE_BFD */

}

int
nsm_rib_nexthop_fib_check (struct rib *fib)
{
  struct nexthop *nexthop;

  for (nexthop = fib->nexthop; nexthop; nexthop = nexthop->next)
    if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
      return 1;

  return 0;
}

int
nsm_rib_nexthop_connected_check (struct rib *rib)
{
  struct nsm_master *nm = rib->vrf->nm;
  struct nexthop *nexthop = rib->nexthop;
  struct interface *ifp;

  if (nexthop->type == NEXTHOP_TYPE_IFINDEX)
    {
      ifp = if_lookup_by_index (&nm->vr->ifm, nexthop->ifindex);
      if (ifp != NULL)
        if (CHECK_FLAG (ifp->flags, IFF_UP))
          return 1;
    }
  return 0;
}

/* Core function for processing routing information base. */
void
nsm_rib_process (struct nsm_master *nm,
                 struct nsm_ptree_node *rn, struct rib *del)
{
  struct rib *rib;
  struct rib *fib = NULL;
  struct rib *select = NULL;
  afi_t afi;
#ifdef HAVE_BFD
  int bfd_change = 0;
#endif /* HAVE_BFD */

  if (rn->tree->ptype == AF_INET)
    afi = AFI_IP;
#ifdef HAVE_IPV6
  else if (rn->tree->ptype == AF_INET6)
    afi = AFI_IP6;
#endif /* HAVE_IPV6 */
  else
    return;

  /* Select RIB. */
  for (rib = rn->info; rib; rib = rib->next)
    {
      /* Currently installed rib. */
      if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
        fib = rib;

      /* Self route. */
      if (CHECK_FLAG (rib->flags, RIB_FLAG_SELFROUTE)
          && CHECK_FLAG (rib->flags, RIB_FLAG_STALE)
          && rib->distance == DISTANCE_INFINITY)
        fib = rib;

  /* Flag bfd_change indicates if the RIB processing is triggered from BFD 
   session up/down event. If BFD has triggered RIB processing we should not 
   delete/add BFD session from RIB processing */
#ifdef HAVE_BFD
      if (CHECK_FLAG (rib->pflags, NSM_ROUTE_CHG_BFD))
        {
          bfd_change = 1;
          UNSET_FLAG (rib->pflags, NSM_ROUTE_CHG_BFD);
        }
#endif /* HAVE_BFD */

      /* Skip unreachable nexthop. */
      if (!nsm_nexthop_active_update (rn, rib, 0))
        continue;

      /* Infinite distance. */
      if (rib->distance == DISTANCE_INFINITY)
        continue;
      
      /* Newly selected rib. */
      if (select == NULL || rib->distance < select->distance)
        select = rib;
      
      /* If distances of rib and select are same
         then check for metric  */
      else
        { 
          if (rib->type == APN_ROUTE_CONNECT  
               || (rib->distance == select->distance 
               && rib->metric < select->metric))
            select = rib;
   
          else if (rib->type != APN_ROUTE_CONNECT  
                   && rib->type != APN_ROUTE_KERNEL
                   && rib->distance == select->distance 
                   && rib->metric == select->metric
                   && CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED)) 
            select = rib;
        }
     }

  /* Deleted route check. */
  if (del && CHECK_FLAG (del->flags, RIB_FLAG_SELECTED))
    {
      fib = del;
      UNSET_FLAG (fib->flags, RIB_FLAG_SELECTED);
    }

  /* Update counter. */
  if (fib != NULL)
    {
      struct nsm_vrf_afi *nva = &fib->vrf->afi[afi];

      if (nsm_rib_nexthop_fib_check (fib))
        {
          if (fib->type != APN_ROUTE_STATIC
              && fib->type != APN_ROUTE_KERNEL
              && fib->type != APN_ROUTE_CONNECT)
            nva->counter.fib--;

          /* Only decrement counter when greater than 0 */
          /* This is required for STALE KERNEL SELFROUTES 
           * which are deleted when learned from the kernel.
           * But there routes have never been added and so the 
           * couter was getting decremented from 0 
           * and becoming greater than 0x7ffffff as it is an
           * unsigned value.
           *
           * Stale selfroutes are never added to the fib and therefore 
           * the counter is not incremented and therefore should not be 
           * decremented too.
           */
          if ((nva->counter.type[fib->type] > 0) && ! RIB_STALE_SELFROUTE (fib))
            nva->counter.type[fib->type]--;
        }
    }

  if (select != NULL)
    {
      struct nsm_vrf_afi *nva = &select->vrf->afi[afi];

      if (select->type != APN_ROUTE_STATIC
          && select->type != APN_ROUTE_KERNEL
          && select->type != APN_ROUTE_CONNECT)
        if (nva->counter.fib >= nm->max_fib_routes)
           {
             if (IS_NSM_DEBUG_EVENT)
               zlog_info (NSM_ZG, "Exceeding maximum fib routes limit %d)",
                          nm->max_fib_routes);
             return;
           }
    }

  /* RIB is updated. */
  if (fib != NULL && select != NULL)
    {
#ifdef HAVE_BFD
      if (fib->type == APN_ROUTE_STATIC)
        {
         if (! (select == fib
             && ! CHECK_FLAG (select->flags, RIB_FLAG_CHANGED)
             && ! CHECK_FLAG (nm->flags, NSM_MULTIPATH_REFRESH)))
           {
               if (bfd_change)
                 {
                 SET_FLAG (fib->pflags, NSM_ROUTE_CHG_BFD);
                 }
               /* Delete BFD sessions for all old RIB nexthop before updating 
                  the RIB. New BFD sessions for updated RIB entry will be added 
                  from nsm_rib_process_update */
               if (!nsm_bfd_update_session_by_rib (fib->vrf, fib, afi, NULL, rn, 
                                                                0, session_del))
               {
                 if (IS_NSM_DEBUG_BFD)
                   zlog_err (NSM_ZG, "BFD static route session delete failed");
               }
               UNSET_FLAG (fib->pflags, NSM_ROUTE_CHG_BFD);
               if (bfd_change)
                 SET_FLAG (select->pflags, NSM_ROUTE_CHG_BFD);
           }
        }

#endif /* HAVE_BFD */
      nsm_rib_process_update (nm, rn, fib, select);
    }
  /* Uninstall old RIB from forwarding table. */
  else if (fib != NULL && select == NULL)
    {
#ifdef HAVE_BFD
      if (bfd_change)
        SET_FLAG (fib->pflags, NSM_ROUTE_CHG_BFD);
#endif /* HAVE_BFD */
      nsm_rib_process_delete (nm, rn, fib);
    }
  /* Install new RIB into forwarding table. */
  else if (fib == NULL && select != NULL)
    {
#ifdef HAVE_BFD
      if (bfd_change)
        SET_FLAG (select->pflags, NSM_ROUTE_CHG_BFD);
#endif /* HAVE_BFD */
      nsm_rib_process_add (nm, rn, select);
    }

  /* Update counter. */
  if (select != NULL)
    {
      struct nsm_vrf_afi *nva = &select->vrf->afi[afi];

      if (select->type == APN_ROUTE_CONNECT
          && nsm_rib_nexthop_connected_check (select))
        nva->counter.type[select->type]++;
      else if (nsm_rib_nexthop_fib_check (select))
        {
          if (select->type != APN_ROUTE_STATIC
              && select->type != APN_ROUTE_KERNEL
              && select->type != APN_ROUTE_CONNECT)
            nva->counter.fib++;

          nva->counter.type[select->type]++;
        }
    }
}

/* Add RIB to head of the route node. */
void
nsm_rib_addnode (struct nsm_ptree_node *rn, struct rib *rib)
{
  struct rib *head;

  head = rn->info;
  if (head)
    head->prev = rib;
  rib->next = head;
  rn->info = rib;
}

void
nsm_rib_delnode (struct nsm_ptree_node *rn, struct rib *rib)
{
  if (rib->next)
    rib->next->prev = rib->prev;
  if (rib->prev)
    rib->prev->next = rib->next;
  else
    rn->info = rib->next;
}

/* Common function for adding new RIB to VRF.  */
void
nsm_rib_add (struct prefix *p, struct rib *rib, afi_t afi, safi_t safi)
{
  struct nsm_vrf *vrf = rib->vrf;
  struct nsm_master *nm = vrf->nm;
  struct nsm_ptree_node *rn = NULL;
  struct rib *same = NULL;
  struct nexthop *nexthop;
  
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return;

  /* Make it sure prefixlen is applied to the prefix. */
  apply_mask (p);

  
  /* Lookup route node.*/
  if (afi == AFI_IP)
    rn = nsm_ptree_node_get (vrf->afi[afi].rib[safi], (u_char *)&p->u.prefix4, p->prefixlen);

#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = nsm_ptree_node_get (vrf->afi[afi].rib[safi], (u_char *)&p->u.prefix6, p->prefixlen);
#endif

  if (rn == NULL)
   { 
      if (IS_NSM_DEBUG_EVENT)
           zlog_info (NSM_ZG, "NSM_PTREE create failed");
 
      return;
   }

  /* If same type of route are installed, treat it as a implicit
     withdraw. */
  if (rib->type != APN_ROUTE_KERNEL)
    {
      for (same = rn->info; same; same = same->next)
        if (same->type == rib->type 
            && same->type != APN_ROUTE_CONNECT
            && same->pid == rib->pid)
         { 
           nsm_rib_delnode (rn, same);
           nsm_ptree_unlock_node (rn);
           break;
         }
    }

#ifdef HAVE_HA        
  /* XXX: IMPORTANT!!!!!
   * Make sure the delete of "same" RIB is called before the create of
   * new "rib". This is required since "same" and "rib" share the same
   * key and therefore the Standby has to delete "same" before adding
   * "rib".
   * Note that nsm_rib_process() might modify same flags (thereby
   * calling checkpoint modify function) but since we have set the
   * IGNORE_MODIFY_DELETE flag for the RIB checkpoint record type, 
   * it should be ok.        
   */
  if (same)
    nsm_cal_delete_rib (same, rn);

  /* Create a new rib */
  nsm_cal_create_rib (rib, rn);
#endif /* HAVE_HA */

  /* If this route is kernel route, set FIB flag to the route. */
  if (rib->type == APN_ROUTE_KERNEL || rib->type == APN_ROUTE_CONNECT)
    for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
      SET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);

  /* Link new rib to node.*/
  nsm_rib_addnode (rn, rib);

  /* Process this route node. */
  nsm_rib_process (nm, rn, same);

  /* Free implicit route.*/
  if (same)
    nsm_rib_free (same);
}

/* Common function for deleting RIB from VRF.  */
void
nsm_rib_delete (struct prefix *p, u_char type,
                struct rib *del, struct rib *fib,
                struct nsm_ptree_node *rn, struct nsm_master *nm)
{
  struct nexthop *nexthop;

  /* Unlink RIB.  */
  if (del)
    nsm_rib_delnode (rn, del);
  else
    {
      /* When operator delete FIB route from termial, kernel message
         will be sent.  Clean up FIB route flag and selected flag.
         RIB process will install the route again.  */
      if (fib && type == APN_ROUTE_KERNEL)
        {
          for (nexthop = fib->nexthop; nexthop; nexthop = nexthop->next)
            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
          UNSET_FLAG (fib->flags, RIB_FLAG_SELECTED);
#ifdef HAVE_HA
          nsm_cal_modify_rib (fib, rn);
#endif /* HAVE_HA */
        }
      else
        {
          /* RIB is not found.  Unlock look up lock then return.  */
          nsm_ptree_unlock_node (rn);
          return;
        }
    }

  /* Process changes with passing deleted RIB. */
  nsm_rib_process (nm, rn, del);

  /* Free RIB.  */
  if (del)
    {
#ifdef HAVE_HA
      nsm_cal_delete_rib (del, rn);
#endif /* HAVE_HA */

      nsm_rib_free (del);
      nsm_ptree_unlock_node (rn);
    }

  /* Unlock look up lock.  */
  nsm_ptree_unlock_node (rn);
}

void
nsm_rib_delete_by_ifindex (struct prefix *p, u_char type, int ifindex,
                           afi_t afi, safi_t safi, struct nsm_vrf *vrf)
{
  struct nsm_master *nm = vrf->nm;
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib;
  struct rib *fib = NULL;
  struct rib *del = NULL;
  struct nexthop *nexthop;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return;

  /* Apply mask. */
  apply_mask (p);

  /* Lookup route node. */
  if (afi == AFI_IP)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST],
                                (u_char *)&p->u.prefix4, p->prefixlen);
 
#ifdef HAVE_IPV6    
  if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST],
                                (u_char *)&p->u.prefix6, p->prefixlen);
#endif
  if (rn == NULL)
    return;

  /* Lookup same type route. */
  for (rib = rn->info; rib; rib = rib->next)
    {
      /* Remember FIB entry.  */
      if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
        fib = rib;

      if (rib->type == type)
        if ((nexthop = rib->nexthop))
          if (nexthop->ifindex == ifindex)
            {
              del = rib;
              break;
            }
    }

  nsm_rib_delete (p, type, del, fib, rn, nm);
}

void
nsm_rib_delete_by_type (struct prefix *p, u_char type,
                        int pid, afi_t afi, safi_t safi, struct nsm_vrf *vrf)
{
  struct nsm_master *nm = vrf->nm;
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib;
  struct rib *fib = NULL;
  struct rib *del = NULL;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return;

  /* Apply mask. */
  apply_mask (p);

  /* Lookup route node. */
  if (afi == AFI_IP)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], (u_char *)&p->u.prefix4, p->prefixlen);

#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], (u_char *)&p->u.prefix6, p->prefixlen);
#endif
  if (rn == NULL)
    return;

  /* Lookup same type route. */
  for (rib = rn->info; rib; rib = rib->next)
    {
      /* Remember FIB entry.  */
      if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
        fib = rib;
      if (rib->type == type && rib->pid == pid)
        {
          del = rib;
          break;
        }
    }

  nsm_rib_delete (p, type, del, fib, rn, nm);
}

/* Delete specific route from RIB. */
int
nsm_rib_delete_route (struct rib *rib, struct nsm_ptree_node *pn)
{
  struct nsm_vrf *vrf = rib->vrf;
  struct nsm_master *nm = vrf->nm;

  /* Unlink RIB.  */
  nsm_rib_delnode (pn, rib);

  /* Process changes with passing deleted RIB. */
  nsm_rib_process (nm, pn, rib);

  /* Free RIB.  */
  nsm_rib_free (rib);
  nsm_ptree_unlock_node (pn);

  return 0;
}

void
nsm_rib_delete_by_nexthop (struct prefix *p, u_char type,
                           int ifindex, union nsm_nexthop *gate,
                           afi_t afi, safi_t safi, struct nsm_vrf *vrf)
{
  struct nsm_master *nm = vrf->nm;
  struct nsm_ptree_node *rn = NULL;
  struct rib *rib;
  struct rib *fib = NULL;
  struct rib *del = NULL;
  struct nexthop *nexthop;

  /* Sanity check. */
  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
      )
    return;

  /* Apply mask. */
  apply_mask (p);

  /* Lookup route node. */
  if (afi == AFI_IP)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], (u_char *)&p->u.prefix4, p->prefixlen);
#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = nsm_ptree_node_lookup (vrf->afi[afi].rib[SAFI_UNICAST], (u_char *)&p->u.prefix6, p->prefixlen);
#endif

  if (rn == NULL)
    return;

  /* Lookup same type route. */
  for (rib = rn->info; rib; rib = rib->next)
    {
      /* Remember FIB entry.  */
      if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
        fib = rib;

      if (rib->type == type)
        if ((nexthop = rib->nexthop))
          if ((nexthop->ifindex == ifindex)
               || (nexthop->ifindex == 0 && nexthop->type == NEXTHOP_TYPE_IPV4))
            if (gate == NULL
                || (afi == AFI_IP
                    && IPV4_ADDR_SAME (&nexthop->gate.ipv4, &gate->ipv4))
#ifdef HAVE_IPV6
                || (afi == AFI_IP6
                    && IPV6_ADDR_SAME (&nexthop->gate.ipv6, &gate->ipv6))
#endif /* HAVE_IPV6 */
               )
              {
                del = rib;
                break;
              }
    }

  nsm_rib_delete (p, type, del, fib, rn, nm);
}

/* Delete IPv4 RIB from VRF.  */
void
nsm_rib_delete_ipv4 (u_char type, struct prefix_ipv4 *p, int ifindex,
                     struct pal_in4_addr *gate, fib_id_t id)
{
  struct nsm_vrf *nv;
  union nsm_nexthop nexthop;
  struct interface *ifp = NULL;

  nv = nsm_vrf_lookup_by_fib_id (nzg, id);
  if (nv == NULL)
    return;

  if (ifindex > 0)
    ifp = ifv_lookup_by_index (&nv->vrf->ifv, ifindex);
  else if (gate != NULL)
    ifp = if_match_by_ipv4_address (&nv->vrf->vr->ifm, gate, id);

  if (ifp == NULL)
    return;

  /* If interface is down, route delete should be handled by rib_update. */
  if (!if_is_up (ifp))
    return;

  /* Delete RIB from VRF. */
  if (gate != NULL)
    {
      nexthop.ipv4 = *gate;
      nsm_rib_delete_by_nexthop ((struct prefix *)p, type, ifp->ifindex,
                                 &nexthop, AFI_IP, SAFI_UNICAST, nv);
    }
  else
    nsm_rib_delete_by_ifindex ((struct prefix *)p, type, ifp->ifindex,
                               AFI_IP, SAFI_UNICAST, nv);
}

#ifdef HAVE_IPV6
/* Delete IPv6 RIB from VRF.  */
void
nsm_rib_delete_ipv6 (u_char type, struct prefix_ipv6 *p, int ifindex,
                     struct pal_in6_addr *gate, fib_id_t id)
{
  struct nsm_vrf *nv;
  union nsm_nexthop nexthop;
  struct interface *ifp = NULL;

  nv = nsm_vrf_lookup_by_fib_id (nzg, id);
  if (nv == NULL)
    return;

  if (ifindex > 0)
    ifp = ifv_lookup_by_index (&nv->vrf->ifv, ifindex);
  else if (gate != NULL)
    ifp = if_match_by_ipv6_address (&nv->vrf->vr->ifm, gate, id);

  if (ifp == NULL)
    return;

  /* If interface is down, route delete should be handled by rib_update. */
  if (!if_is_up (ifp))
    return;

  /* Delete RIB from VRF. */
  if (gate != NULL)
    {
      nexthop.ipv6 = *gate;
      nsm_rib_delete_by_nexthop ((struct prefix *)p, type, ifp->ifindex,
                                 &nexthop, AFI_IP6, SAFI_UNICAST, nv);
    }
  else
    nsm_rib_delete_by_ifindex ((struct prefix *)p, type, ifp->ifindex,
                               AFI_IP6, SAFI_UNICAST, nv);
}
#endif /* HAVE_IPV6 */

/* Add IPv4 RIB to VRF.  */
void
nsm_rib_add_ipv4 (u_char type, struct prefix_ipv4 *p, u_int32_t ifindex,
                  struct pal_in4_addr *gate, u_char flags, u_int32_t metric,
                  fib_id_t id)
{
  struct nsm_vrf *vrf;
  struct rib *rib;
  u_char distance = 0;

  /* Lookup VRF. */
  vrf = nsm_vrf_lookup_by_fib_id (nzg, id);
  if (vrf == NULL)
    return;

  /* Make self-route as stale and distance of Infinity(255). */
  if (type == APN_ROUTE_KERNEL)
    if (CHECK_FLAG (flags, RIB_FLAG_SELFROUTE))
      {
        distance = DISTANCE_INFINITY;
        SET_FLAG (flags, RIB_FLAG_STALE);
      }

  /* Allocate new RIB structure. */
  rib = nsm_rib_new (type, distance, flags, metric, vrf);
  if (rib == NULL)
    {
      if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "RIB create failed");
      return ;
    }

  /* Next hop allocation. */
  if (gate)
    {
      if (ifindex)
        nsm_nexthop_ipv4_ifindex_add (rib, gate, ifindex, ROUTE_TYPE_OTHER);
      else
        nsm_nexthop_ipv4_add (rib, gate, ROUTE_TYPE_OTHER);
    }
  else if (ifindex)
    nsm_nexthop_ifindex_add (rib, ifindex, ROUTE_TYPE_OTHER);
  else
    nsm_nexthop_ifname_add (rib, "Null", ROUTE_TYPE_OTHER);

  /* Add RIB to VRF.  */
  nsm_rib_add ((struct prefix *) p, rib, AFI_IP, SAFI_UNICAST);
}

void
nsm_rib_add_connected_ipv4 (struct interface *ifp, struct prefix *addr)
{
  struct apn_vrf *iv = ifp->vrf;
  struct nsm_vrf *nv = iv->proto;
  struct prefix p;
  struct rib *rib;

  /* Sanity check.  */
  if (addr == NULL)
    return;

  /* Copy the prefix.  */
  prefix_copy (&p, addr);

  /* Apply mask to the network.  */
  apply_mask (&p);

  /* Create the NSM RIB.  */
  rib = nsm_rib_new (APN_ROUTE_CONNECT, 0, 0, 0, nv);
  if (rib == NULL)
    return;

  /* Create the nexthop.  */
  nsm_nexthop_ifindex_add (rib, ifp->ifindex, ROUTE_TYPE_LOCAL);

  /* Add to the NSM RIB.  */
  nsm_rib_add (&p, rib, family2afi (p.family), SAFI_UNICAST);

  /* Schedule the RIB update timer.  */
  nsm_rib_update_timer_add (nv, family2afi (p.family));
}

void
nsm_rib_delete_connected_ipv4 (struct interface *ifp, struct prefix *addr)
{
  struct apn_vrf *iv = ifp->vrf;
  struct nsm_vrf *nv = iv->proto;
  struct prefix p;

  /* Sanity check.  */
  if (addr == NULL)
    return;

  /* Copy the prefix.  */
  prefix_copy (&p, addr);

  /* Apply mask to the network.  */
  apply_mask (&p);

  /* Delete the NSM RIB.  */
#ifdef HAVE_IPV6
  if ( IN6_IS_ADDR_LINKLOCAL (&p.u.prefix6))
    nsm_rib_delete_by_ifindex (&p, APN_ROUTE_KERNEL, ifp->ifindex,
                             family2afi (p.family), SAFI_UNICAST, nv);
  else
#endif /* HAVE_IPV6 */
    nsm_rib_delete_by_ifindex (&p, APN_ROUTE_CONNECT, ifp->ifindex,
                             family2afi (p.family), SAFI_UNICAST, nv);

  /* Schedule the RIB update timer.  */
  nsm_rib_update_timer_add (nv, family2afi (p.family));
}

#ifdef HAVE_IPV6
/* Detect bogus route.  */
int
nsm_rib_bogus_ipv6 (u_char type, struct prefix_ipv6 *p,
                    struct pal_in6_addr *gate, u_int32_t ifindex)
{
  if (type == APN_ROUTE_CONNECT && IN6_IS_ADDR_UNSPECIFIED (&p->prefix))
    return 1;
  return 0;
}

/* Add IPv6 RIB to VRF.  */
void
nsm_rib_add_ipv6 (u_char type, struct prefix_ipv6 *p, u_int32_t ifindex,
                  struct pal_in6_addr *gate, u_char flags, u_int32_t metric,
                  fib_id_t id)
{
  struct nsm_vrf *vrf;
  struct rib *rib;
  u_char distance = 0;

  /* Lookup VRF. */
  vrf = nsm_vrf_lookup_by_fib_id (nzg, id);
  if (vrf == NULL)
    return;

  /* Make sure mask is applied. */
  apply_mask_ipv6 (p);

  /* Filter bogus route. */
  if (nsm_rib_bogus_ipv6 (type, p, gate, ifindex))
    return;

  /* Make self-route as stale and distance of Infinity(255). */
  if (type == APN_ROUTE_KERNEL)
    if (CHECK_FLAG (flags, RIB_FLAG_SELFROUTE))
      {
        distance = DISTANCE_INFINITY;
        SET_FLAG (flags, RIB_FLAG_STALE);
      }

  /* Allocate new RIB structure. */
  rib = nsm_rib_new (type, distance, flags, metric, vrf);
  if (rib == NULL)
   {
     if (IS_NSM_DEBUG_EVENT)
       zlog_info (NSM_ZG, "RIB create failed");
     return ;
   }

  /* Next hop allocation. */
  if (gate)
    {
      if (ifindex)
        nsm_nexthop_ipv6_ifindex_add (rib, gate, ifindex);
      else
        nsm_nexthop_ipv6_add (rib, gate);
    }
  else
    nsm_nexthop_ifindex_add (rib, ifindex, ROUTE_TYPE_OTHER);

  /* Add RIB to VRF.  */
  nsm_rib_add ((struct prefix *) p, rib, AFI_IP6, SAFI_UNICAST);
}

void
nsm_rib_add_connected_ipv6 (struct interface *ifp, struct prefix *addr)
{
  struct apn_vrf *iv = ifp->vrf;
  struct nsm_vrf *nv = iv->proto;
  struct nsm_ptree_node *rn = NULL;
  struct prefix p;
  struct rib *rib = NULL;

  /* Sanity check.  */
  if (addr == NULL)
    return;

  /* Copy the prefix.  */
  prefix_copy (&p, addr);

  /* Apply mask to the network.  */
  apply_mask (&p);

  /* Unspecified address should be droped.  */
  if (IN6_IS_ADDR_UNSPECIFIED (&p.u.prefix6))
    return;

  /* Duplicate RIB check.  */
  rib = nsm_rib_lookup_by_type (nv, family2afi (p.family),
                                &p, APN_ROUTE_CONNECT);
  if (rib != NULL)
    if (!IN6_IS_ADDR_LINKLOCAL (&p.u.prefix6)
        || rib->nexthop->ifindex == ifp->ifindex)
      return;

   rib = nsm_rib_lookup_by_type (nv, family2afi (p.family),
                                 &p, APN_ROUTE_KERNEL);

   if (rib != NULL)
     if (!IN6_IS_ADDR_LINKLOCAL (&p.u.prefix6)
         || rib->nexthop->ifindex == ifp->ifindex)
       {
         rn = nsm_ptree_node_lookup (nv->afi[family2afi (
                                     p.family)].rib[SAFI_UNICAST],
                                     (u_char *)&p.u.prefix6,
                                     p.prefixlen);
         if (rn != NULL)
           nsm_rib_delete_route (rib, rn);
       }

  /* Create the NSM RIB.  */
  rib = nsm_rib_new (APN_ROUTE_CONNECT, 0, 0, 0, nv);
  if (rib == NULL)
    {
      if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "RIB create failed");
      return ;
    }

  /* Create the nexthop.  */
  nsm_nexthop_ifindex_add (rib, ifp->ifindex, ROUTE_TYPE_LOCAL);

  /* Add to the NSM RIB.  */
  nsm_rib_add (&p, rib, family2afi (p.family), SAFI_UNICAST);

  /* Schedule the RIB update timer.  */
  nsm_rib_update_timer_add (nv, family2afi (p.family));
}

void
nsm_rib_delete_connected_ipv6 (struct interface *ifp, struct prefix *addr)
{
  struct apn_vrf *iv = ifp->vrf;
  struct nsm_vrf *nv = iv->proto;
  struct prefix p;

  /* Sanity check.  */
  if (addr == NULL)
    return;

  /* Copy the prefix.  */
  prefix_copy (&p, addr);

  /* Apply mask to the network.  */
  apply_mask (&p);

  /* Unspecified address should be droped.  */
  if (IN6_IS_ADDR_UNSPECIFIED (&p.u.prefix6))
    return;

  /* Delete the NSM RIB.  */
  nsm_rib_delete_by_ifindex (&p, APN_ROUTE_CONNECT, ifp->ifindex,
                             family2afi (p.family), SAFI_UNICAST, nv);

  /* Schedule the RIB update timer.  */
  nsm_rib_update_timer_add (nv, family2afi (p.family));
}
#endif /* HAVE_IPV6 */


struct nexthop *
nsm_rib_static_nexthop_add (struct rib *rib, struct nsm_static *ns)
{
  struct nexthop *nexthop = NULL;

  if (ns->type == NEXTHOP_TYPE_IFNAME)
    nexthop = nsm_nexthop_ifname_add (rib, ns->ifname, ns->snmp_route_type);
  else if (ns->type == NEXTHOP_TYPE_IPV4)
    nexthop = nsm_nexthop_ipv4_add (rib, &ns->gate.ipv4, ns->snmp_route_type);
  else if (ns->type == NEXTHOP_TYPE_IPV4_IFNAME)
    nexthop = nsm_nexthop_ipv4_ifname_add (rib, &ns->gate.ipv4, ns->ifname,
                                           ns->snmp_route_type);

#ifdef HAVE_IPV6
  else if (ns->type == NEXTHOP_TYPE_IPV6)
    nexthop = nsm_nexthop_ipv6_add (rib, &ns->gate.ipv6);
  else if (ns->type == NEXTHOP_TYPE_IPV6_IFNAME)
    nexthop = nsm_nexthop_ipv6_ifname_add (rib, &ns->gate.ipv6, ns->ifname);
#endif /* HAVE_IPV6 */

  return nexthop;
}

struct rib *
nsm_rib_static_lookup (struct rib *top, struct nsm_static *ns)
{
  struct rib *rib;

  for (rib = top; rib; rib = rib->next)
    if (rib->type == APN_ROUTE_STATIC)
      if (rib->distance == ns->distance)
        return rib;

  return NULL;
}

struct nexthop *
nsm_rib_nexthop_match_static (struct rib *rib, struct nsm_static *ns)
{
  struct nexthop *nexthop;

  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    if (nexthop->type == ns->type
        || (nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
            && ns->type == NEXTHOP_TYPE_IPV4)
#ifdef HAVE_IPV6
        || (nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX
            && ns->type == NEXTHOP_TYPE_IPV6)
#endif /* HAVE_IPV6 */
        )
      {
        if (nexthop->type == NEXTHOP_TYPE_IFNAME)
          if (pal_strcmp (nexthop->ifname, ns->ifname) == 0)
            return nexthop;

        if (nexthop->type == NEXTHOP_TYPE_IPV4
            || (nexthop->type == NEXTHOP_TYPE_IPV4_IFINDEX
                && ns->type == NEXTHOP_TYPE_IPV4))
          if (IPV4_ADDR_SAME (&nexthop->gate.ipv4, &ns->gate.ipv4))
            return nexthop;

        if (nexthop->type == NEXTHOP_TYPE_IPV4_IFNAME)
          if (IPV4_ADDR_SAME (&nexthop->gate.ipv4, &ns->gate.ipv4))
            if (pal_strcmp (nexthop->ifname, ns->ifname) == 0)
              return nexthop;
#ifdef HAVE_IPV6
        if (nexthop->type == NEXTHOP_TYPE_IPV6
            || (nexthop->type == NEXTHOP_TYPE_IPV6_IFINDEX
                && ns->type == NEXTHOP_TYPE_IPV6))
          if (IPV6_ADDR_SAME (&nexthop->gate.ipv6, &ns->gate.ipv6))
            return nexthop;

        if (nexthop->type == NEXTHOP_TYPE_IPV6_IFNAME)
          if (IPV6_ADDR_SAME (&nexthop->gate.ipv6, &ns->gate.ipv6))
            if (pal_strcmp (nexthop->ifname, ns->ifname) == 0)
              return nexthop;
#endif /* HAVE_IPV6 */
      }

  return NULL;
}

/* Install static route into RIB. */
int
nsm_static_install (struct nsm_vrf *vrf, struct prefix *p,
                    struct nsm_static *ns)
{
  struct nsm_vrf_afi *nva = &vrf->afi[ns->afi];
  struct nsm_master *nm = vrf->nm;
  struct nsm_ptree_node *rn = NULL;
  struct ptree_node *rn_existed = NULL;
  struct rib *rib = NULL;
  struct rib *same = NULL;
  struct nsm_static *old_ns;

  if (p->family == AF_INET)
    rn = nsm_ptree_node_get (nva->rib[SAFI_UNICAST], (u_char *) &p->u.prefix4, p->prefixlen);

#ifdef HAVE_IPV6
  if (p->family == AF_INET6)
    rn = nsm_ptree_node_get (nva->rib[SAFI_UNICAST], (u_char *) &p->u.prefix6, p->prefixlen);
#endif
 
  if (rn == NULL)
   { 
      if (IS_NSM_DEBUG_EVENT)
           zlog_info (NSM_ZG, "NSM_PTREE create failed");
 
      return NSM_FAILURE;
   }
  
  same = nsm_rib_static_lookup (rn->info, ns);
  if (same)
    {
      /* If same type of route are installed, treat it as a implicit
         withdraw. */
      rib = nsm_rib_new (APN_ROUTE_STATIC, ns->distance, RIB_FLAG_INTERNAL, 0, vrf);
      if (! rib)
        {
          nsm_ptree_unlock_node (rn);
          return NSM_FAILURE;
        }

      /* Copy flags */
      rib->flags = same->flags;
      rib->ext_flags = same->ext_flags;

      /* Copy existing nexthop information from rib if distance is same */
      if (p->family == AF_INET)
        rn_existed = ptree_node_get (nva->ip_static, (u_char *) &p->u.prefix4, p->prefixlen);

#ifdef HAVE_IPV6
      if (p->family == AF_INET6)
        rn_existed = ptree_node_get (nva->ip_static, (u_char *) &p->u.prefix6, p->prefixlen);
#endif
      if (! rn_existed)
        {
          nsm_rib_free (rib);
          return NSM_FAILURE;
        }

      for (old_ns = rn_existed->info; old_ns; old_ns = old_ns->next)
        {
          if (old_ns->distance != ns->distance)
            continue;
#if 0
          copied_ns = nsm_static_nexthop_copy (old_ns);
          if (! copied_ns)
            {
              nsm_ptree_unlock_node (rn);
              ptree_unlock_node (rn_existed);
              return NSM_FAILURE;
            }

          /* Add nexthop information to new rib */
          nsm_rib_static_nexthop_add (rib, copied_ns);
          nsm_static_free (copied_ns);
#endif
          nsm_rib_static_nexthop_add (rib, old_ns);
        }

      /* Add new rib */
      nsm_rib_addnode (rn, rib); 

      /* Delete old rib */
      nsm_rib_delnode (rn, same);

      ptree_unlock_node (rn_existed);

      /* Unlock for get since we are just replacing the rib */
      nsm_ptree_unlock_node (rn);
    }
  else
    {
      rib = nsm_rib_new (APN_ROUTE_STATIC, ns->distance, RIB_FLAG_INTERNAL, 0, vrf); 
      if (rib == NULL)
        {
          if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "RIB create failed");
          nsm_ptree_unlock_node (rn);
          return NSM_FAILURE;
        }
      nsm_rib_addnode (rn, rib);
      nsm_rib_static_nexthop_add (rib, ns);
    }

#ifdef HAVE_HA
  /* XXX: IMPORTANT!!!!!
   * Make sure the delete of "same" RIB is called before the create of
   * new "rib". This is required since "same" and "rib" share the same
   * key and therefore the Standby has to delete "same" before adding
   * "rib".
   * Note that nsm_rib_process() might modify same flags (thereby
   * calling checkpoint modify function) but since we have set the
   * IGNORE_MODIFY_DELETE flag for the RIB checkpoint record type, 
   * it should be ok.
   */
  if (same)
    nsm_cal_delete_rib (same, rn);

  /* Create a new rib */
  nsm_cal_create_rib (rib, rn);
#endif /* HAVE_HA */

  SET_FLAG (rib->flags, RIB_FLAG_INTERNAL);

  if (IS_NULL_INTERFACE_STR (ns->ifname))
    SET_FLAG (rib->flags, RIB_FLAG_BLACKHOLE);

  nsm_rib_process (nm, rn, same);

  if (same)
    nsm_rib_free (same);

  return NSM_SUCCESS;
}

int
nsm_static_uninstall (struct nsm_vrf *vrf, struct prefix *p,
                      struct nsm_static *ns)
{
  struct nsm_vrf_afi *nva = &vrf->afi[ns->afi];
  struct nsm_master *nm = vrf->nm;
  struct nsm_ptree_node *rn = NULL;
  struct ptree_node *rn_existed = NULL;
  struct rib *rib;
  struct rib *new_rib;
  struct nsm_static *old_ns = NULL;
#if 0
  struct nsm_static *copied_ns = NULL;
#endif
  struct nexthop *nexthop;
#ifdef HAVE_BFD
  afi_t afi = AFI_IP;
#endif /* HAVE_BFD */

  if (p->family == AF_INET)
    rn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], (u_char *)&p->u.prefix4, p->prefixlen);
#ifdef HAVE_IPV6
  if (p->family == AF_INET6)
    rn = nsm_ptree_node_lookup (nva->rib[SAFI_UNICAST], (u_char *)&p->u.prefix6, p->prefixlen);
#endif

  if (rn == NULL)
    return NSM_FAILURE;

  rib = nsm_rib_static_lookup (rn->info, ns);
  if (rib != NULL)
    {
      /* Lookup nexthop matches to static. */
      nexthop = nsm_rib_nexthop_match_static (rib, ns);
      if (nexthop != NULL)
        {
          if (IS_NULL_INTERFACE_STR (nexthop->ifname))
            UNSET_FLAG (rib->flags, RIB_FLAG_BLACKHOLE);

          /* Check nexthop. */
          if (rib->nexthop_num == 1)
            {
              nsm_rib_delnode (rn, rib);

          /* BFD session handling for inactive static routes should be handled
             here, since FIB processing will not be invoked for these entries */
#ifdef HAVE_BFD
          /* Flag is set to indicate admin down event for BFD */
          SET_FLAG (rib->pflags, NSM_BFD_CONFIG_CHG);
          if (!CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
            {
              if (p->family == AF_INET)
                afi = AFI_IP;
#ifdef HAVE_IPV6
              else if (p->family == AF_INET6)
                afi = AFI_IP6;
#endif /* HAVE_IPV6 */
              else
                return NSM_FAILURE;

              /* Function to delete BFD session for all the RIB nexthop entries */
              if (!nsm_bfd_update_session_by_rib (vrf, rib, afi, NULL, rn, 
                                                          0, session_del))
              {
                if (IS_NSM_DEBUG_BFD)
                  zlog_err (NSM_ZG, "BFD static route session delete failed");
              }
              SET_FLAG (rib->pflags, NSM_ROUTE_CHG_BFD);
            }
#endif /* HAVE_BFD */

              nsm_rib_process (nm, rn, rib);
#ifdef HAVE_BFD
           UNSET_FLAG (rib->pflags, NSM_ROUTE_CHG_BFD); 
           UNSET_FLAG (rib->pflags, NSM_BFD_CONFIG_CHG);
#endif /* HAVE_BFD */

#ifdef HAVE_HA
              nsm_cal_delete_rib (rib, rn);
#endif /* HAVE_HA */
              nsm_rib_free (rib);

              nsm_ptree_unlock_node (rn);
            }
          else
            {
              new_rib = nsm_rib_new (APN_ROUTE_STATIC, ns->distance, RIB_FLAG_INTERNAL, 0, vrf);
              if (! new_rib)
                {
                  nsm_ptree_unlock_node (rn);
                  return NSM_FAILURE;
                }

              /* Copy Flags from old rib */
              new_rib->flags = rib->flags;
              new_rib->ext_flags = rib->ext_flags;

              if (p->family == AF_INET)
                rn_existed = ptree_node_get (nva->ip_static, 
                    (u_char *)&p->u.prefix4,
                    p->prefixlen);
#ifdef HAVE_IPV6
              if (p->family == AF_INET6)
                rn_existed = ptree_node_get (nva->ip_static,
                    (u_char *)&p->u.prefix6, 
                    p->prefixlen);
#endif
              if (! rn_existed)
                {
                  nsm_rib_free (new_rib);
                  return NSM_FAILURE;
                }
   
              for (old_ns = rn_existed->info; old_ns; old_ns = old_ns->next)
                {
                  if (old_ns != ns)
                    {
                      if (old_ns->distance != ns->distance)
                        continue;
#if 0
                      copied_ns = nsm_static_nexthop_copy (old_ns);
                      if (!  copied_ns)
                        {
                          nsm_ptree_unlock_node (rn);
                          ptree_unlock_node (rn_existed);
                          return NSM_FAILURE;
                        }
                      nsm_rib_static_nexthop_add (new_rib, copied_ns); 
                      nsm_static_free (copied_ns);
#endif
                      nsm_rib_static_nexthop_add (new_rib, old_ns);
                    }
                }

#ifdef HAVE_HA
              /* XXX: IMPORTANT!!!!!
               * Make sure the delete of "same" RIB is called before the create of
               * new "rib". This is required since "same" and "rib" share the same
               * key and therefore the Standby has to delete "same" before adding
               * "rib".
               * Note that nsm_rib_process() might modify same flags (thereby
               * calling checkpoint modify function) but since we have set the
               * IGNORE_MODIFY_DELETE flag for the RIB checkpoint record type, 
               * it should be ok.
               */
              nsm_cal_delete_rib (rib, rn);

              /* Create a new rib */
              nsm_cal_create_rib (new_rib, rn);
#endif /* HAVE_HA */

              nsm_rib_addnode (rn, new_rib);
              nsm_rib_delnode (rn, rib);

              nsm_rib_process (nm, rn, rib);
              nsm_rib_free (rib);

              ptree_unlock_node (rn_existed);
            }
        }
    }

  /* Unlock node. */
  nsm_ptree_unlock_node (rn);

  return NSM_SUCCESS;
}

int
nsm_static_ipv4_count (struct nsm_vrf *nv)
{
  struct ptree_node *rn;
  struct nsm_static *ns;
  int count = 0;

  for (rn = ptree_top (nv->IPV4_STATIC); rn; rn = ptree_next(rn))
    for (ns = rn->info; ns; ns = ns->next)
      count++;
  return count;
}

/* RIB update function. */

/* Return restart time for the protocol.  */
u_int32_t
nsm_restart_time (int protocol_id)
{
  return ng->restart[protocol_id].restart_time;
}

/* Register preserve time for specific protocol.  */
void
nsm_restart_register (int proto_id, u_int32_t restart_time)
{
  ng->restart[proto_id].restart_time = restart_time;

#ifdef HAVE_HA
  nsm_cal_modify_gr_restart (&ng->restart[proto_id]);
#endif /* HAVE_HA */
}

/* Return restart state */
u_char
nsm_restart_state (int proto_id)
{
  return ng->restart[proto_id].state;
}

/* Restart state turns on */
void
nsm_restart_state_set (int proto_id)
{
  ng->restart[proto_id].state = NSM_RESTART_ACTIVE;
}

/* Restart state turns off */
void
nsm_restart_state_unset (int proto_id)
{
  ng->restart[proto_id].state = NSM_RESTART_INACTIVE;
}

/* Stop preserve of the route.  */
void
nsm_restart_stop (int proto_id)
{
  THREAD_TIMER_OFF (ng->restart[proto_id].t_preserve);
  ng->restart[proto_id].preserve_time = 0;
}

/* Store restart option value.  */
void
nsm_restart_option_set (int proto_id, u_int16_t restart_len,
                        u_char *restart_val)
{
  struct nsm_restart *restart;
#ifdef HAVE_HA
  bool_t modify_ckpt = PAL_FALSE;
#endif /* HAVE_HA */

  restart = &ng->restart[proto_id];
  if (restart->restart_val)
    {
      XFREE (MTYPE_NSM_RESTART_OPTION, restart->restart_val);
      restart->restart_val = NULL;

#ifdef HAVE_HA
      modify_ckpt = PAL_TRUE;
#endif /* HAVE_HA */
    }

  restart->restart_length = restart_len;
  restart->restart_val = XMALLOC (MTYPE_NSM_RESTART_OPTION, restart_len);
  pal_mem_cpy (restart->restart_val, restart_val, restart_len);

#ifdef HAVE_HA
  if (modify_ckpt == PAL_FALSE)
    nsm_cal_create_gr_restart_val (restart);
  else
    nsm_cal_modify_gr_restart_val (restart);
#endif /* HAVE_HA */
}

/* Clear restart option value.  */
void
nsm_restart_option_unset (int proto_id)
{
  struct nsm_restart *restart;

  restart = &ng->restart[proto_id];
  if (restart->restart_val)
    {
      XFREE (MTYPE_NSM_RESTART_OPTION, restart->restart_val);
      restart->restart_val = NULL;
    }
  restart->restart_length = 0;

#ifdef HAVE_HA
  nsm_cal_delete_gr_restart_val (restart);
#endif /* HAVE_HA */
}

#ifdef HAVE_RESTART 
/* Display forwarding timers. */
void
nsm_api_show_forwarding_time (struct vrep_table *vrep, int proto_id)
{
  struct nsm_restart *restart = NULL;
  struct pal_tm exptime;
  char time_str [TIME_BUF];
  u_int8_t temp_id = 0;
 
  if (vrep == NULL)
    return; 
  
  /* Display forwarding time of all protocols. */
  if (proto_id == APN_PROTO_UNSPEC)
    {
      for (temp_id = 0; temp_id < APN_PROTO_MAX; temp_id++) 
        {
          restart = &ng->restart[temp_id];
          if (restart->state == NSM_RESTART_ACTIVE)
            {
              pal_mem_set (time_str, 0, TIME_BUF);
              pal_time_loc (&restart->disconnect_time, &exptime);
              pal_time_strf (time_str, TIME_BUF, "%Y/%m/%d %H:%M:%S",
                             &exptime);

              vrep_add_next_row (vrep, NULL, " %s \t %s \t %d \t %s ", 
                                 modname_strl (temp_id),
                                 "ACTIVE",
                                 thread_timer_remain_second (
                                  restart->t_preserve), time_str);    
             }
        }
    }
  /* Display forwarding time of specific protocol. */
  else
    {
      restart = &ng->restart[proto_id];

      if (restart->state == NSM_RESTART_ACTIVE)
        {
          pal_time_loc (&restart->disconnect_time, &exptime);
          pal_time_strf (time_str, TIME_BUF, "%Y/%m/%d %H:%M:%S", &exptime);

          vrep_add_next_row (vrep, NULL, " %s \t %s \t %d \t %s ",
                             modname_strl (proto_id), 
                             "ACTIVE",
                             thread_timer_remain_second (
                              restart->t_preserve),
                             time_str);
        } 
    }

  return;
}
#endif /* HAVE_RESTART */
 

/* Convert protocol id to route type.  */
u_char
nsm_proto2type (int proto_id)
{
  switch (proto_id)
    {
    case APN_PROTO_RIP:
      return APN_ROUTE_RIP;
    case APN_PROTO_RIPNG:
      return APN_ROUTE_RIPNG;
    case APN_PROTO_OSPF:
      return APN_ROUTE_OSPF;
    case APN_PROTO_OSPF6:
      return APN_ROUTE_OSPF6;
    case APN_PROTO_BGP:
      return APN_ROUTE_BGP;
    case APN_PROTO_ISIS:
      return APN_ROUTE_ISIS;
    default:
      return APN_ROUTE_DEFAULT;
    }
}

/* Remove stale marked route.  This function is called when restart
   time is exipred or client send message stale remove message.  */
void
nsm_restart_stale_remove (struct nsm_master *nm,
                          int proto_id, afi_t afi, safi_t safi)
{
  struct nsm_ptree_node *rn;
  struct rib *rib;
  struct rib *next;
  struct nsm_ptree_table *table = NULL;
  struct nsm_vrf *vrf = NULL;
  u_char type;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    return;
  
  if ((afi == AFI_IP) && (safi == SAFI_UNICAST))
    table = vrf->RIB_IPV4_UNICAST;
#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
      if (afi == AFI_IP6 && safi == SAFI_UNICAST)
        table = vrf->RIB_IPV6_UNICAST;
    }
  else
    table = NULL;
#endif /* HAVE_IPV6 */

  if (! table)
    return;

  type = nsm_proto2type (proto_id);

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      for (rib = rn->info; rib; rib = next)
        {
          next = rib->next;

          if (rib->type == type && CHECK_FLAG (rib->flags, RIB_FLAG_STALE))
            {
              nsm_rib_delnode (rn, rib);
              nsm_rib_process (nm, rn, rib);
#ifdef HAVE_HA
              nsm_cal_delete_rib (rib, rn);
#endif /* HAVE_HA */
              nsm_rib_free (rib);
              nsm_ptree_unlock_node (rn);
            }
        }
    }

  /* Clear restart information.  */
  nsm_restart_stop (proto_id);
  nsm_restart_option_unset (proto_id);
}



/*
 * Function Name : msm_restart_stale_mark ()
 * Input         : nsm_master struct, proto_id, afi, safi 
 * Output        : None
 * Purpose       : Mark all the routes as stale . This function will be called
                   when client send stale mark message 
*/
void
nsm_restart_stale_mark (struct nsm_master *nm,
                        int proto_id, afi_t afi, safi_t safi)
{
  struct nsm_ptree_node *rn;
  struct rib *rib;
  struct rib *next;
  struct nsm_ptree_table *table = NULL;
  struct nsm_vrf *vrf = NULL;
  u_int32_t restart_time;

  vrf = nsm_vrf_lookup_by_id (nm, 0);
  if (! vrf)
    return; 
 
  if (afi == AFI_IP && safi == SAFI_UNICAST)
    table = vrf->RIB_IPV4_UNICAST;
#ifdef HAVE_IPV6
  if (NSM_CAP_HAVE_IPV6)
    {
       if (afi == AFI_IP6 && safi == SAFI_UNICAST)
         table = vrf->RIB_IPV6_UNICAST;
      }
   else
     table = NULL;
#endif /* HAVE_IPV6 */

   if (! table)
     return;

   restart_time = nsm_restart_time (proto_id);

   for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
     {
       for (rib = rn->info; rib; rib = next)
         {
           next = rib->next;

           if (rib->client_id == proto_id
               && restart_time)
             {
               SET_FLAG (rib->flags, RIB_FLAG_STALE);
#ifdef HAVE_HA
               nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
             }
         }
     }

   /* Turn on the preserve timer if restart_time is set. */
   if (restart_time && ! ng->restart[proto_id].t_preserve)
     {
       ng->restart[proto_id].t_preserve
         = thread_add_timer (nzg, nsm_restart_time_expire,
                             &ng->restart[proto_id], restart_time);
       ng->restart[proto_id].preserve_time = restart_time;
     }
 }


/*
 * Function Name : nsm_restart_stale_mark_vrf ()
 * Input         : nsm_vrf struct, afi, proto_id 
 * Output        : None
 * Purpose       : Mark all the vrf routes as stale . This function will be
                   called when client send stale mark message
*/
void
nsm_restart_stale_mark_vrf (struct nsm_vrf *nv, afi_t afi, int client_id)
{
  struct nsm_ptree_table *table = nv->afi[afi].rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct rib *rib;
  struct rib *next;
  u_int32_t restart_time;

  struct pal_in4_addr nexthop_self;
  pal_mem_set (&nexthop_self, 0, sizeof (struct pal_in4_addr));

  restart_time = nsm_restart_time (client_id);

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
       for (rib = rn->info; rib; rib = next)
         {
           next = rib->next;

           /* Mark all the routes as Stale except the locally generated routes */
           if (rib->client_id == client_id
               && ! CHECK_FLAG (rib->flags, RIB_FLAG_STALE)
               && restart_time
               && (rib->nexthop
                   && ! IPV4_ADDR_SAME (&rib->nexthop->gate.ipv4, &nexthop_self)))
             {
               SET_FLAG (rib->flags, RIB_FLAG_STALE);
#ifdef HAVE_HA
               nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */     
             }
         }
    }
    /* Turn on the preserve timer if restart_time is set. */
    if (restart_time && ! ng->restart[client_id].t_preserve)
      {
        ng->restart[client_id].t_preserve
          = thread_add_timer (nzg, nsm_restart_time_expire,
                              &ng->restart[client_id], restart_time);
        ng->restart[client_id].preserve_time = restart_time;
      }     
}

/* Restart time is expired.  */
int
nsm_restart_time_expire (struct thread *t)
{
  struct nsm_restart *restart;
  struct nsm_master *nm;
  int proto_id;
  afi_t afi;

#ifdef HAVE_VRF 
  mpls_owner_t owner;
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  int i, j;
#endif /* HAVE_VRF */

  /* Get restart structure and protocol id.  */
  restart = THREAD_ARG (t);
  proto_id = restart->proto_id;

#ifdef HAVE_VRF
  owner = gmpls_proto_to_owner (restart->proto_id);
  vr = NULL;
  iv = NULL;
  nv = NULL;
  i = j = 0;
#endif /* HAVE_VRF */

  /* Clear timer pointer.  */
  ng->restart[proto_id].t_preserve = NULL;
  ng->restart[proto_id].preserve_time = 0;

  /* Logging.  */
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Timer (preserve timer expire for proto_id %d)",
               proto_id);

  nm = nsm_master_lookup_by_id (nzg, 0);

#ifdef HAVE_VRF
  /* Clean up all vrf MPLS entries */ 
  if (owner == MPLS_OTHER_BGP)
    {
      for (i = 0; i < vector_max (nzg->vr_vec); i++)
        if ((vr = vector_slot (nzg->vr_vec, i)))
          nsm_gmpls_restart_stale_remove_vrf (vr->proto, proto_id);
    }
  /* Clean up All VR, All AFI and All VRF table. */
  for (i = 0; i < vector_max (nzg->vr_vec); i++)
    if ((vr = vector_slot (nzg->vr_vec, i)))
      for (j = 0; j < vector_max (vr->vrf_vec); j++)
        if ((iv = vector_slot (vr->vrf_vec, j)))
          if ((nv = iv->proto))
            NSM_AFI_LOOP (afi)
              nsm_restart_stale_remove_vrf (nv, afi, proto_id);  
#else 
  if (nm != NULL) 
    NSM_AFI_LOOP (afi)
      nsm_restart_stale_remove (nm, proto_id, afi, SAFI_UNICAST);
#endif /* HAVE_VRF */

#ifdef HAVE_MPLS
  /* Clean up stale label pool for this protocol. */
  nsm_lp_server_clean_stale_for (nm, proto_id);
#endif /* HAVE_MPLS */

  return 0;
}

/* Interface goes up. */
void
nsm_rib_if_up (struct interface *ifp)
{
  if (ifp->vrf)
    {
      nsm_rib_update_vrf (ifp->vrf->proto, AFI_IP);
#ifdef HAVE_IPV6
      nsm_rib_update_vrf (ifp->vrf->proto, AFI_IP6);
#endif /* HAVE_IPV6 */
    }
}

/* Interface goes down. */
void
nsm_rib_if_down (struct interface *ifp)
{
  if (ifp->vrf)
    {
      nsm_rib_update_vrf (ifp->vrf->proto, AFI_IP);
#ifdef HAVE_IPV6
      nsm_rib_update_vrf (ifp->vrf->proto, AFI_IP6);
#endif /* HAVE_IPV6 */
    }
}

/* Multipath-num configured change.
 * Currently only support MAIN_TABLE.
 */
void
nsm_rib_multipath_process (struct nsm_master *nm)
{
  struct apn_vr *temp_vr = nm->vr;  
  struct nsm_ptree_node *rn;
  struct apn_vrf *temp_vrf = NULL;
  struct nsm_vrf *vrf = NULL;
  afi_t afi;

  /* Re-flush rib routes into FIB. */
  for (temp_vrf = temp_vr->vrf_list; temp_vrf; temp_vrf = temp_vrf->next)
    {
      vrf = temp_vrf->proto;
      NSM_AFI_LOOP (afi)
        for (rn = nsm_ptree_top (vrf->afi[afi].rib[SAFI_UNICAST]);
             rn; rn = nsm_ptree_next (rn))
          if (rn->info)
            nsm_rib_process (nm, rn, NULL);
    }
}

void
nsm_rib_sweep_table (struct nsm_ptree_table *rib_table)
{
  struct nsm_ptree_node *rn;
  struct rib *rib;
  struct rib *next;

  if (rib_table == NULL)
    return;

  for (rn = nsm_ptree_top (rib_table); rn; rn = nsm_ptree_next (rn))
    for (rib = rn->info; rib; rib = next)
      {
        next = rib->next;

        if (rib->type == APN_ROUTE_KERNEL
            && rib->distance == DISTANCE_INFINITY
            && CHECK_FLAG (rib->flags, RIB_FLAG_SELFROUTE))
          {
            if (CHECK_FLAG (rib->flags, RIB_FLAG_STALE))
              nsm_rib_uninstall_kernel (rn, rib);

            nsm_rib_delnode (rn, rib);
#ifdef HAVE_HA
            nsm_cal_delete_rib (rib, rn);
#endif /* HAVE_HA */
            nsm_rib_free (rib);
            nsm_ptree_unlock_node (rn);
          }
      }
}

int
nsm_rib_sweep_stale (struct nsm_master *nm, afi_t afi)
{
  int i;
  struct apn_vrf *iv;
  struct nsm_vrf *nv;

  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)))
      if ((nv = iv->proto))
        nsm_rib_sweep_table (nv->afi[afi].rib[SAFI_UNICAST]);

  return 0;
}

int
nsm_rib_sweep_timer (struct thread *t)
{
  struct nsm_master *nm = THREAD_ARG (t);
  afi_t afi;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (nzg, "NSM: RIB sweep timer expire");

  nm->t_sweep = NULL;
#ifdef HAVE_HA
  nsm_cal_delete_rib_sweep_timer (nm);
#endif /* HAVE_HA */

  NSM_AFI_LOOP (afi)
    nsm_rib_sweep_stale (nm, afi);

  return 0;
}

int
nsm_rib_sweep_timer_update (struct nsm_master *nm)
{
  pal_time_t diff;

  if (nm->t_sweep != NULL)
    {
#ifdef HAVE_HA
      nsm_cal_delete_rib_sweep_timer (nm);
#endif /* HAVE_HA */
      THREAD_TIMER_OFF (nm->t_sweep);
    }

  /* Re-reregister the RIB sweep timer if the time is not forever.  */
  if (nm->fib_retain_time != NSM_FIB_RETAIN_TIME_FOREVER)
    {
      diff = pal_time_current (NULL) - nm->start_time;

      if (nm->fib_retain_time < diff)
        THREAD_TIMER_ON (nm->zg, nm->t_sweep, nsm_rib_sweep_timer, nm, 0);
      else
        THREAD_TIMER_ON (nm->zg, nm->t_sweep, nsm_rib_sweep_timer,
            nm, nm->fib_retain_time - diff);
#ifdef HAVE_HA
      nsm_cal_create_rib_sweep_timer (nm);
#endif /* HAVE_HA */
    }

  return 0;
}


/* Delete all added route and close RIB. */
void
nsm_rib_close_uninstall_all (struct nsm_vrf *vrf, afi_t afi)
{
  struct nsm_vrf_afi *nva = &vrf->afi[afi];
  struct nsm_ptree_table *table = nva->rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct rib *rib;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    for (rib = rn->info; rib; rib = rib->next)
      {
        /* Do not delete connected routes when in SHUTDOWN */
        if (CHECK_FLAG (ng->flags, NSM_SHUTDOWN) && 
            (rib->type == APN_ROUTE_CONNECT))
          continue;

        if (!RIB_SYSTEM_ROUTE (rib))
          if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
            nsm_rib_uninstall_kernel (rn, rib);
      }
}

void
nsm_rib_close_afi (struct nsm_master *nm, afi_t afi)
{
  struct apn_vrf *vrf;
  struct route_node *rn;
  struct interface *ifp;
  int i;

  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((vrf = vector_slot (nm->vr->vrf_vec, i)))
      {
        if (! CHECK_FLAG (ng->flags, NSM_SHUTDOWN))
          for (rn = route_top (vrf->ifv.if_table); rn; rn = route_next (rn))
            if ((ifp = rn->info))
              nsm_if_unbind_vrf (ifp, vrf->proto);

        nsm_rib_close_uninstall_all (vrf->proto, afi);
      }
}

/* Close RIB when NSM terminates. */
void
nsm_rib_close (struct nsm_master *nm)
{
  afi_t afi;

  NSM_AFI_LOOP (afi)
    nsm_rib_close_afi (nm, afi);
}

/* Clean RIB table. */
static int
nsm_rib_table_clean (struct nsm_ptree_table *table, u_int32_t route_type_mask)
{
  struct nsm_ptree_node *rn;
  struct rib *rib, *tmp_rib;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      rib = rn->info;
      while (rib)
        {
          tmp_rib = rib;
          rib = tmp_rib->next;

          /* If RIB matches the route_type_mask, uninstall it and delete it. */
          if (! RIB_SYSTEM_ROUTE (tmp_rib)
              && ((1 << tmp_rib->type) & (route_type_mask)))
            {
              /* If route is selected, uninstall from kernel. */
              if (CHECK_FLAG (tmp_rib->flags, RIB_FLAG_SELECTED))
                nsm_rib_uninstall_kernel (rn, tmp_rib);

#ifdef HAVE_HA
              nsm_cal_delete_rib (tmp_rib, rn);
#endif /* HAVE_HA */

              nsm_rib_free (tmp_rib);
              rn->info = rib;
            }
        }

      if (! rn->info)
        nsm_ptree_unlock_node (rn);
    }

  return 0;
}

/* Clear RIB for route type. */
int
nsm_rib_clean (struct nsm_master *nm, afi_t afi, safi_t safi,
               u_int32_t route_type_mask, vrf_id_t vrf_id)
{
  struct nsm_vrf *vrf;

  vrf = nsm_vrf_lookup_by_id (nm, vrf_id);
  if (vrf == NULL)
    return -1;

  /* Clean up RIB. */
  nsm_rib_table_clean (vrf->afi[afi].rib[safi], route_type_mask);

  if (afi == AFI_IP)
    {
      if (safi == SAFI_UNICAST && ((1 << APN_ROUTE_STATIC) & route_type_mask))
        nsm_static_table_clean_all (vrf, vrf->IPV4_STATIC);
    }
#ifdef HAVE_IPV6
  else if (NSM_CAP_HAVE_IPV6 && afi == AFI_IP6)
    {
      if (safi == SAFI_UNICAST && ((1 << APN_ROUTE_STATIC) & route_type_mask))
        nsm_static_table_clean_all (vrf, vrf->IPV6_STATIC);
    }
#endif /* HAVE_IPV6 */

  return 0;
}

/* Clean up VRF table by client-ID. */
void
nsm_rib_clean_client_vrf (struct nsm_vrf *nv, afi_t afi,
                          u_char client_id, int preserve)
{
  struct nsm_ptree_table *table = nv->afi[afi].rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct rib *rib;
  struct rib *next;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    for (rib = rn->info; rib; rib = next)
      {
        next = rib->next;

        if (rib->client_id == client_id)
          {
            /* When graceful restart is enabled.  */
            if (preserve)
              {
                SET_FLAG (rib->flags, RIB_FLAG_STALE);
#ifdef HAVE_HA
                nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
              }
            else
              {
                nsm_rib_delnode (rn, rib);
                nsm_rib_process (nv->nm, rn, rib);

#ifdef HAVE_HA
                nsm_cal_delete_rib (rib, rn);
#endif /* HAVE_HA */

                nsm_rib_free (rib);
                nsm_ptree_unlock_node (rn);
              }
          }
      }
}

#ifdef HAVE_VRF
/* 
 * Function Name  : nsm_restart_stale_remove_vrf ()
 * Input          : nsm_vrf table struct, afi and protocol id 
 * Output         : None
 * Purpose        : This function remove all vrf routes when restart timer
                    expired or client send stale remove message
*/  
void
nsm_restart_stale_remove_vrf (struct nsm_vrf *nv, afi_t afi,
                              int proto_id)
{
  struct nsm_ptree_table *table = nv->afi[afi].rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct rib *rib;
  struct rib *next;
  u_char type;

  type = nsm_proto2type (proto_id);
  if (! table)
    return;
  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      for (rib = rn->info; rib; rib = next)
         {
           next = rib->next;
           if (rib->type == type && CHECK_FLAG (rib->flags, RIB_FLAG_STALE))
             {
               nsm_rib_delnode (rn, rib);
               nsm_rib_process (nv->nm, rn, rib);
#ifdef HAVE_HA
               nsm_cal_delete_rib (rib, rn);
#endif /* HAVE_HA */
               nsm_rib_free (rib);
               nsm_ptree_unlock_node (rn);
             }
         }
     }

  /* Clear restart information.  */
  nsm_restart_stop (proto_id);
  nsm_restart_option_unset (proto_id);
}
#endif /* HAVE_VRF */

/* Clean up all RIB by client_id.  When restart is enabled, we mark
   the route by stale flag.  */
void
nsm_rib_clean_client (u_int32_t client_id, int proto_id)
{
  struct apn_vr *vr;
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  u_int32_t restart_time;
  u_char id;
  afi_t afi;
  int i, j;

  /* In RIB structure client_id is defined as one octet value.  */
  if (client_id > 255)
    return;

  /* Convert value to one octet.  */
  id = client_id;
  restart_time = nsm_restart_time (proto_id);

  /* Clean up All VR, All AFI and All VRF table. */
  for (i = 0; i < vector_max (nzg->vr_vec); i++)
    if ((vr = vector_slot (nzg->vr_vec, i)))
      for (j = 0; j < vector_max (vr->vrf_vec); j++)
        if ((iv = vector_slot (vr->vrf_vec, j)))
          if ((nv = iv->proto))
            NSM_AFI_LOOP (afi)
              nsm_rib_clean_client_vrf (nv, afi, id, restart_time);

  /* Turn on the preserve timer if restart_time is set. */
  if (restart_time && ! ng->restart[proto_id].t_preserve)
    {
      ng->restart[proto_id].t_preserve
        = thread_add_timer (nzg, nsm_restart_time_expire,
                            &ng->restart[proto_id], restart_time);
      ng->restart[proto_id].preserve_time = restart_time;
    }

  /* Clear restart time.  */
  ng->restart[proto_id].restart_time = 0;
  ng->restart[proto_id].disconnect_time = pal_time_current (NULL);
}

int
nsm_static_nexthop_same (struct nsm_static *ns, u_char type,
                         union nsm_nexthop *gate, char *ifname)
{
  if (type != ns->type)
    return 0;

  if (gate != NULL)
    {
      if (ns->afi == AFI_IP)
        {
          if (!IPV4_ADDR_SAME (&gate->ipv4, &ns->gate.ipv4))
            return 0;
        }
#ifdef HAVE_IPV6
      else if (ns->afi == AFI_IP6)
        {
          if (!IPV6_ADDR_SAME (&gate->ipv6, &ns->gate.ipv6))
            return 0;
        }
#endif /* HAVE_IPV6 */
    }

  if (ifname != NULL)
    if (pal_strcmp (ifname, ns->ifname) != 0)
      return 0;

  return 1;
}

void
nsm_static_nexthop_add_sort (struct ptree_node *rn,
                             struct nsm_static *ns)
{
  struct nsm_static *top = rn->info;
  struct nsm_static *pp;
  struct nsm_static *cp;
  int ret = 0;

  for (pp = NULL, cp = top; cp; pp = cp, cp = cp->next)
    if (ns->type != NEXTHOP_TYPE_IFNAME)
      if (cp->type != NEXTHOP_TYPE_IFNAME)
        {
          if (ns->afi == AFI_IP)
            ret = NSM_IPV4_CMP (&ns->gate.ipv4, &cp->gate.ipv4);
#ifdef HAVE_IPV6
          else if (ns->afi == AFI_IP6)
            ret = NSM_IPV6_CMP (&ns->gate.ipv6, &cp->gate.ipv6);
#endif /* HAVE_IPV6 */
          if (ret < 0)
            break;
          else if (ret == 0)
            {
              if (ns->distance < cp->distance)
                break;
            }
        }

  if (pp)
    pp->next = ns;
  else
    rn->info = ns;

  if (cp)
    cp->prev = ns;

  ns->prev = pp;
  ns->next = cp;
}


u_char
nsm_static_nexthop_type_get (afi_t afi, union nsm_nexthop *gate,
                             char *ifname)
{
  u_char type = 0;

  if (afi == AFI_IP)
    {
      if (gate != NULL && ifname != NULL)
        type = NEXTHOP_TYPE_IPV4_IFNAME;
      else if (gate != NULL)
        type = NEXTHOP_TYPE_IPV4;
      else if (ifname != NULL)
        type = NEXTHOP_TYPE_IFNAME;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (gate != NULL && ifname != NULL)
        type = NEXTHOP_TYPE_IPV6_IFNAME;
      else if (gate != NULL)
        type = NEXTHOP_TYPE_IPV6;
      else if (ifname != NULL)
        type = NEXTHOP_TYPE_IFNAME;
    }
#endif /* HAVE_IPV6 */

  return type;
}

struct nsm_static *
nsm_static_get (struct nsm_vrf *vrf, afi_t afi, struct prefix *p,
                union nsm_nexthop *gate, char *ifname, u_char distance,
                int metric, int snmp_route_type,
                u_int32_t tag, char *desc)
{
  struct ptree_node *rn = NULL;
  struct nsm_static *ns;
  u_char type;

  type = nsm_static_nexthop_type_get (afi, gate, ifname);
  if (type == 0)
    return NULL;

  if (afi == AFI_IP)
    rn = ptree_node_get (vrf->afi[afi].ip_static, (u_char *) &p->u.prefix4,
			 p->prefixlen);

#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = ptree_node_get (vrf->afi[afi].ip_static, (u_char *) &p->u.prefix6,
			 p->prefixlen);
#endif
  
  if (! rn)
    return NULL;

  /* Do nothing if there is a same static route. */
  for (ns = rn->info; ns; ns = ns->next)
    if (nsm_static_nexthop_same (ns, type, gate, ifname))
      {
        /* Check if tag and description are also same */
        if (distance == ns->distance
            && (tag == ns->tag))
          {
            if (((desc && ns->desc)
                  && (pal_strcmp (desc, ns->desc) == 0))
                 || (desc == NULL &&  ns->desc == NULL)) 
              {
                ptree_unlock_node (rn);
                return NULL;
              }
           else
             {
               nsm_static_delete (vrf, afi, p, gate, ifname, distance);
               break;
             }
          }
        else
          {
            nsm_static_delete (vrf, afi, p, gate, ifname, distance);
            break;
          }
      }

#ifdef HAVE_RMM
  /* Send static route to backup instance. */
  if (afi == AFI_IP)
    nsm_rmm_static_ipv4_sync (vrf->nm, (struct prefix_ipv4 *)p,
                              &gate->ipv4, ifname, distance, 0,
                              metric, snmp_route_type,
                              NSM_RMM_RIB_CMD_STATIC_IPV4_ADD);
  else
    nsm_rmm_static_ipv6_sync (vrf->nm, (struct prefix_ipv6 *)p,
                              &gate->ipv6, ifname, distance, 0,
                              NSM_RMM_RIB_CMD_STATIC_IPV6_ADD);
#endif /* HAVE_RMM */

  /* Make new static route structure. */
  ns = nsm_static_new (afi, type, ifname);
  ns->distance = distance;
  if (gate)
    ns->gate = *gate;
#ifdef HAVE_SNMP
  ns->metric = metric;
  ns->snmp_route_type = snmp_route_type;
#endif /* HAVE_SNMP */
  /* Update the tag and description fields */
  ns->tag = tag;
  if (desc)
    ns->desc = XSTRDUP (MTYPE_NSM_STATIC_DESC, desc);
  else
    ns->desc = NULL;

#ifdef HAVE_HA
  nsm_cal_create_nsm_static (ns, vrf, rn);
#endif /* HAVE_HA */

  /* Add static to list sorted by distance value and gateway address. */
  nsm_static_nexthop_add_sort (rn, ns);

  return ns;
}

#ifdef HAVE_BFD
struct nsm_static *
nsm_static_lookup (struct nsm_vrf *vrf, afi_t afi, struct prefix *p,
                  union nsm_nexthop *nh)
{
  struct ptree_node *rn = NULL;
  struct nsm_static *ns;


  if (afi == AFI_IP)
    rn = ptree_node_get (vrf->afi[afi].ip_static, (u_char *) &p->u.prefix4,
                         p->prefixlen);

#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = ptree_node_get (vrf->afi[afi].ip_static, (u_char *) &p->u.prefix6,
                         p->prefixlen);
#endif

  if (! rn)
    return NULL;

  if (afi == AFI_IP)
    for (ns = rn->info; ns; ns = ns->next)
      if (!IPV4_ADDR_CMP(&ns->gate.ipv4, &nh->ipv4))
        return ns;

  return NULL;
}
#endif /* HAVE_BFD */

/* Add static route to config and RIB. */
int
nsm_static_add (struct nsm_vrf *vrf, afi_t afi, struct prefix *p,
                union nsm_nexthop *gate, char *ifname, u_char distance,
                int metric, int snmp_route_type,
                u_int32_t tag, char *desc)
{
  struct nsm_static *ns;

  ns = nsm_static_get (vrf, afi, p, gate, ifname, distance, metric,
                       snmp_route_type, tag, desc);
  if (ns)
    /* Install static into RIB. */
    nsm_static_install (vrf, p, ns);

  return 1;
}

/* Delete static route from config and RIB. */
int
nsm_static_delete (struct nsm_vrf *vrf, afi_t afi, struct prefix *p,
                   union nsm_nexthop *gate, char *ifname, u_char distance)
{
  struct ptree_node *rn = NULL;
  struct nsm_static *ns;
  u_char type;

  type = nsm_static_nexthop_type_get (afi, gate, ifname);
  if (type == 0)
    return 0;

  if (afi == AFI_IP)
    rn = ptree_node_lookup (vrf->afi[afi].ip_static,(u_char *) &p->u.prefix4, p->prefixlen);
#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = ptree_node_lookup (vrf->afi[afi].ip_static,(u_char *) &p->u.prefix6, p->prefixlen);
#endif

  if (rn == NULL)
    return 0;

  /* Find same static route is the tree */
  for (ns = rn->info; ns; ns = ns->next)
    if (nsm_static_nexthop_same (ns, type, gate, ifname))
      break;

  /* Can't find static route. */
  if (ns == NULL)
    {
      ptree_unlock_node (rn);
      return 0;
    }

#ifdef HAVE_RMM
  /* Tell backup instance, then delete locally. */
  if (p->family == AF_INET)
    nsm_rmm_static_ipv4_sync (vrf->nm, (struct prefix_ipv4 *)p,
                              &gate->ipv4, ifname, distance, 0,
                              ns->metric, ns->snmp_route_type,
                              NSM_RMM_RIB_CMD_STATIC_IPV4_DELETE);
#endif /* HAVE_RMM */

  /* Uninstall from RIB. */
  nsm_static_uninstall (vrf, p, ns);

  /* Unlink static route from linked list. */
  if (ns->prev)
    ns->prev->next = ns->next;
  else
    rn->info = ns->next;
  if (ns->next)
    ns->next->prev = ns->prev;

#ifdef HAVE_HA
  nsm_cal_delete_nsm_static (ns, vrf, rn);
#endif /* HAVE_HA */

  nsm_static_free (ns);
  ptree_unlock_node (rn);
  ptree_unlock_node (rn);

  return 1;
}

int
nsm_static_delete_all (struct nsm_vrf *vrf, afi_t afi, struct prefix *p)
{
  struct ptree_node *rn = NULL;
  struct nsm_static *ns, *next;

  if (afi == AFI_IP)
    rn = ptree_node_lookup (vrf->afi[afi].ip_static,(u_char *) &p->u.prefix4, p->prefixlen);
#ifdef HAVE_IPV6
  if (afi == AFI_IP6)
    rn = ptree_node_lookup (vrf->afi[afi].ip_static,(u_char *) &p->u.prefix6, p->prefixlen);
#endif

  if (rn == NULL)
    return 0;

  for (ns = rn->info; ns; ns = next)
    {
      next = ns->next;
      nsm_static_uninstall (vrf, p, ns);

#ifdef HAVE_HA
      nsm_cal_delete_nsm_static (ns, vrf, rn);
#endif /* HAVE_HA */

      /* Unlink static route from linked list. */
      if (ns->prev)
        ns->prev->next = ns->next;
      else
        rn->info = ns->next;
      if (ns->next)
        ns->next->prev = ns->prev;

      nsm_static_free (ns);

      /* Unlock for delete */
      ptree_unlock_node (rn);
    }

  rn->info = NULL;
  ptree_unlock_node (rn);

  return 0;
}

int
nsm_static_delete_by_if (struct nsm_vrf *vrf, afi_t afi,
                         struct interface *ifp)
{
  struct ptree_node *rn;
  struct nsm_static *ns;
  struct nsm_static *next;
  union nsm_nexthop *gate = NULL;
  struct prefix p;

  for (rn = ptree_top (vrf->afi[afi].ip_static);
       rn; rn = ptree_next (rn))
    for (ns = rn->info; ns; ns = next)
      {
        next = ns->next;
        if (ns->type == NEXTHOP_TYPE_IFNAME
            || ns->type == NEXTHOP_TYPE_IPV4_IFNAME
#ifdef HAVE_IPV6
            || ns->type == NEXTHOP_TYPE_IPV6_IFNAME
#endif /* HAVE_IPV6 */
            )
          if (pal_strcmp (ns->ifname, ifp->name) == 0)
            {
              if (ns->type == NEXTHOP_TYPE_IPV4_IFNAME)
                gate = &ns->gate;
#ifdef HAVE_IPV6
              else if (ns->type == NEXTHOP_TYPE_IPV6_IFNAME)
                gate = &ns->gate;
#endif /* HAVE_IPV6 */
              pal_mem_set(&p, 0, sizeof(struct prefix));
              p.prefixlen = rn->key_len;
              if (afi == AFI_IP)
                {
                  p.family = AF_INET;
                  NSM_PTREE_KEY_COPY (&p.u.prefix4, rn);
                }
#ifdef HAVE_IPV6
              else if (afi == AFI_IP6)
               {
                 p.family = AF_INET6;
                 NSM_PTREE_KEY_COPY (&p.u.prefix6, rn);
               }
#endif
              nsm_static_delete (vrf, afi, &p, gate,
                                 ifp->name, ns->distance);
            }
      }
  return 0;
}

int
nsm_rib_update_ipv4_timer (struct thread *thread)
{
  struct nsm_vrf *vrf = THREAD_ARG (thread);

  vrf->afi[AFI_IP].t_rib_update = NULL;

#ifdef HAVE_HA
  nsm_cal_delete_rib_update_timer (&vrf->afi[AFI_IP], vrf);
#endif /* HAVE_HA */

  nsm_rib_update_vrf (vrf, AFI_IP);

  return 0;
}

#ifdef HAVE_IPV6
int
nsm_rib_update_ipv6_timer (struct thread *thread)
{
  struct nsm_vrf *vrf = THREAD_ARG (thread);

  vrf->afi[AFI_IP6].t_rib_update = NULL;

#ifdef HAVE_HA
  nsm_cal_delete_rib_update_timer (&vrf->afi[AFI_IP6], vrf);
#endif /* HAVE_HA */

  nsm_rib_update_vrf (vrf, AFI_IP6);

  return 0;
}
#endif /* HAVE_IPV6 */

/* RIB update timer delay in usec. */
#define NSM_RIB_UPDATE_DELAY  250000

void
nsm_rib_update_timer_add (struct nsm_vrf *vrf, afi_t afi)
{
  struct nsm_vrf_afi *nva = &vrf->afi[afi];
  struct pal_timeval t;

  t.tv_sec = 0;
  t.tv_usec = NSM_RIB_UPDATE_DELAY;

  if (afi == AFI_IP)
    {
      if (nva->t_rib_update == NULL)
        {
          nva->t_rib_update =
            thread_add_timer_timeval (NSM_ZG, nsm_rib_update_ipv4_timer, vrf, t);

#ifdef HAVE_HA
          nsm_cal_create_rib_update_timer (nva, vrf);
#endif /* HAVE_HA */
        }
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (nva->t_rib_update == NULL)
        {
          nva->t_rib_update =
            thread_add_timer_timeval (NSM_ZG, nsm_rib_update_ipv6_timer, vrf, t);

#ifdef HAVE_HA
          nsm_cal_create_rib_update_timer (nva, vrf);
#endif /* HAVE_HA */
        }
    }
#endif /* HAVE_IPV6 */
}

int
nsm_rib_update_vrf (struct nsm_vrf *vrf, afi_t afi)
{
  struct nsm_ptree_node *rn;

  for (rn = nsm_ptree_top (vrf->afi[afi].rib[SAFI_UNICAST]);
       rn; rn = nsm_ptree_next (rn))
    if (rn->info)
      nsm_rib_process (vrf->nm, rn, NULL);

  return 0;
}

#ifdef HAVE_GMPLS
s_int32_t
nsm_gmpls_get_gifindex_from_ifindex (struct nsm_master *nm, s_int32_t ifindex)
{
  struct interface *intf;

  intf = if_lookup_by_index (&nm->vr->ifm, ifindex);
  if (intf)
    {
      return intf->gifindex;
    }

  return 0;
}

s_int32_t
nsm_gmpls_get_ifindex_from_gifindex (struct nsm_master *nm, s_int32_t gifindex)
{
  struct interface *intf;
  struct gmpls_if *gmif;

  /* Get the GMPLS interface structure for the same */
   gmif = gmpls_if_get (nzg, nm->vr->id);
   if (gmif == NULL)
     return 0;


  intf = if_lookup_by_gindex (gmif, gifindex);
  if (intf)
    {
      return intf->ifindex;
    }

  return 0;
}

struct datalink *
nsm_gmpls_ifindex_from_dl (struct gmpls_if *gmif, s_int32_t gifindex)
{
  struct datalink *dl;

  dl = data_link_lookup_by_index (gmif, gifindex);
  if (dl != NULL && dl->ifp != NULL)
    {
      return dl;
    }

  return 0;
}
#endif /*HAVE_GMPLS */

#ifdef HAVE_KERNEL_ROUTE_SYNC
int
nsm_rib_kernel_sync_active_check (struct nsm_master *nm, struct rib *rib)
{
  struct nexthop *nexthop;
  int num = 0;

  for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
    if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_ACTIVE))
      {
        if (!CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
          return 0;

        num++;
        if (num >= nm->multipath_num)
          break;
      }

  return num;
}

int
nsm_rib_kernel_invalidate (struct nsm_vrf *nv, afi_t afi)
{
  struct nsm_vrf_afi *nva = &nv->afi[afi];
  struct nsm_ptree_table *table = nva->rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct rib *rib, *next;
  struct nexthop *nexthop;
  struct pal_in4_addr prefix4;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      int selected = 0;

      for (rib = rn->info; rib; rib = next)
        {
          next = rib->next;

          if (rib->type == APN_ROUTE_KERNEL)
            {
              NSM_PTREE_KEY_COPY(&prefix4, rn->key, rn->key_len);

              if ((ntohl (prefix4.s_addr) & 0xff000000) == 0x7f000000)
                {
                  UNSET_FLAG (rib->flags, RIB_FLAG_SELECTED);
#ifdef HAVE_HA
                  nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
                }
              else
                {
                  nsm_rib_delnode (rn, rib);
#ifdef HAVE_HA
                  nsm_cal_delete_rib (rib, rn);
#endif /* HAVE_HA */
                  nsm_rib_free (rib);
                  nsm_ptree_unlock_node (rn);

                  nva->counter.type[APN_ROUTE_KERNEL]--;
                  continue;
                }
            }

          if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
            {
              if (!selected)
                selected = 1;
              else
                {
                UNSET_FLAG (rib->flags, RIB_FLAG_SELECTED);
#ifdef HAVE_HA
                nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
                }
            }

          for (nexthop = rib->nexthop; nexthop; nexthop = nexthop->next)
            UNSET_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB);
        }
    }

  return 1;
}

int
nsm_rib_kernel_validate (struct nsm_vrf *nv, afi_t afi)
{
  struct nsm_vrf_afi *nva = &nv->afi[afi];
  struct nsm_ptree_table *table = nva->rib[SAFI_UNICAST];
  struct nsm_ptree_node *rn;
  struct rib *rib;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    for (rib = rn->info; rib; rib = rib->next)
      {
        if (CHECK_FLAG (rib->flags, RIB_FLAG_SELECTED))
          if (!nsm_rib_kernel_sync_active_check (nv->nm, rib))
            {
              nsm_rib_uninstall_kernel (rn, rib);
              UNSET_FLAG (rib->flags, RIB_FLAG_SELECTED);
#ifdef HAVE_HA
              nsm_cal_modify_rib (rib, rn);
#endif /* HAVE_HA */
            }
      }

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    nsm_rib_process (nv->nm, rn, NULL);

  return 1;
}

int
nsm_rib_kernel_sync_timer (struct thread *thread)
{
  struct nsm_master *nm = THREAD_ARG (thread);
  struct nsm_vrf *nv;
  struct apn_vrf *iv;
  vrf_id_t id;
  afi_t afi;

  nm->t_rib_kernel_sync = NULL;

  for (id = 0; id < vector_max (nm->vr->vrf_vec); id++)
    if ((iv = vector_slot (nm->vr->vrf_vec, id)))
      if ((nv = iv->proto))
        NSM_AFI_LOOP (afi)
        {
          nsm_rib_kernel_invalidate (nv, afi);
          pal_kernel_route_sync (nv, afi);
          nsm_rib_kernel_validate (nv, afi);
        }

  THREAD_TIMER_ON (nm->zg, nm->t_rib_kernel_sync,
                   nsm_rib_kernel_sync_timer, nm,
                   NSM_RIB_KERNEL_SYNC_INTERVAL);

  return 0;
}
#endif /* HAVE_KERNEL_ROUTE_SYNC */
