/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "thread.h"
#include "prefix.h"
#include "linklist.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_ia.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"


/* 16.2. Calculating inter-area route. */
int
ospf_ia_calc_path_set (struct ospf_area *area,
                       struct ospf_route *route, struct ospf_vertex *v,
                       struct ospf_lsa *lsa, u_int32_t metric)
{
  struct ospf_path *path;

  path = ospf_route_path_lookup_new (route, OSPF_PATH_INTER_AREA, area);
  if (path != NULL)
    {
      /* The new path is better.  */
      if (path->cost > v->distance + metric)
        {
          ospf_path_unlock (path);
          path = NULL;
        }

      /* The existing path is better.  */
      else if (path->cost < v->distance + metric)
        {
          ospf_route_path_set_new (route, path);
          vector_set (path->lsa_vec, ospf_lsa_lock (lsa));
          return 0;
        }
    }

  if (path == NULL)
    path = ospf_path_new (OSPF_PATH_INTER_AREA);
  path->cost = v->distance + metric;
  path->area_id = area->area_id;
  if (lsa->data->type == OSPF_SUMMARY_LSA_ASBR)
    SET_FLAG (path->flags, OSPF_PATH_ASBR);
  ospf_route_path_set_new (route, path);

  /* Copy nexthops from vertex. */
  ospf_path_nexthop_copy_all (area->top, path, v);

  /* Register LSA for incremental update. */
  vector_set (path->lsa_vec, ospf_lsa_lock (lsa));

  return 1;
}

int
ospf_ia_calc_transit_path_set (struct ospf_area *area,
                               struct ospf_route *route, struct ospf_vertex *v,
                               struct ospf_lsa *lsa, u_int32_t metric)
{
  struct ospf_area *backbone = area->top->backbone;
  struct ospf_nexthop *nh;
  struct ospf_path *path;

  path = ospf_route_path_lookup_new (route, route->selected->type, backbone);
  if (path == NULL)
    {
      /* Get the nexthop entry for ABR.  */
      nh = ospf_nexthop_get (area, NULL, lsa->data->adv_router);
      if (nh == NULL)
        return 0;

      /* Get the new path.  */
      path = ospf_path_copy (route->selected);
      ospf_route_path_set_new (route, path);

      /* New transit path.  */
      path->path_transit = ospf_path_new (OSPF_PATH_INTER_AREA);
      path->path_transit->cost = v->distance + metric;
      path->path_transit->area_id = area->area_id;

      /* Update the nexthop vector with complete list of nexthops */
      ospf_path_nexthop_copy_all (area->top, path->path_transit, v);
      
      /* Unlock the nexthop.  */
      ospf_nexthop_unlock (nh);
    }
  else
    {
      /* Reset the path.  */
      ospf_route_path_set_new (route, path);

      /* The new path is better.  */
      if (path->path_transit->cost >= v->distance + metric)
        {
          /* Get the nexthop entry for ABR.  */
          nh = ospf_nexthop_get (area, NULL, lsa->data->adv_router);
          if (nh == NULL)
            return 0;

          /* Add to the nexthop vector.  */
          ospf_path_nexthop_add (path->path_transit, nh);

          /* Unlock the nexthop.  */
          ospf_nexthop_unlock (nh);

          /* Cleanup other nexthops.  */
          if (path->path_transit->cost > v->distance + metric)
            {
              ospf_path_nexthop_delete_other (path->path_transit, nh);
              path->path_transit->cost = v->distance + metric;
            }
        }
    }

  vector_set (path->path_transit->lsa_vec, ospf_lsa_lock (lsa));

  return 1;
}

void
ospf_ia_calc_prefix (struct ospf_area *area,
                     struct ospf_route *route, struct ospf_lsa *lsa)
{
  struct ospf_master *om = area->top->om;
  struct ospf_vertex *v;
  struct ls_prefix lp;
  struct pal_in4_addr *mask;
  u_int32_t metric;
  u_char masklen;
  u_char bits;

  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_SUMMARY_LSA_METRIC_OFFSET);
  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  ls_prefix_ipv4_set (&lp, masklen, lsa->data->id);

  /* (1) If the cost specified by the LSA is LSInfinity,
         then examine the next LSA.  */
  if (metric == OSPF_LS_INFINITY)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P metric is LS_INFINITY",
                   &area->area_id, &lp);
      return;
    }

  /* (3) If it is a Type 3 summary-LSA, and the collection of
         destinations described by the summary-LSA equals one of the
         router's configured area address ranges (see Section 3.5),
         and the particular area address range is active, then the
         summary-LSA should be ignored.  */
  if (ospf_area_range_active_match (area->top, &lp))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P Network is active area range",
                   &area->area_id, &lp);
      return;
    }

  /* (4) Else, call the destination described by the LSA N (for Type 3
         summary-LSAs, N's address is obtained by masking the LSA's
         Link State ID with the network/subnet mask contained in the
         body of the LSA), and the area border originating the LSA BR.
         Look up the routing table entry for BR having Area A as
         its associated area.  If no such entry exists for router BR
         (i.e., BR is unreachable in Area A), do nothing with this LSA
         and consider the next in the list.  Else, this LSA describes
         an inter-area path to destination N, whose cost is the distance
         to BR plus the cost specified in the LSA. Call the cost of
         this inter-area path IAC. */
  v = ospf_spf_vertex_lookup (area, lsa->data->adv_router);
  if (v == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P Can't find route to ABR (%r)",
                   &area->area_id, &lp, &lsa->data->adv_router);
      return;
    }

  bits = OSPF_PNT_UCHAR_GET (v->lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);
  if (!IS_ROUTER_LSA_SET (bits, BIT_B))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P adv router (%r) is not ABR",
                   &area->area_id, &lp, &lsa->data->adv_router);
      return;
    }

  /* Set the new path.  */
  ospf_ia_calc_path_set (area, route, v, lsa, metric);
}

void
ospf_ia_calc_router (struct ospf_area *area,
                     struct ospf_route *route, struct ospf_lsa *lsa)
{
  struct ospf_master *om = area->top->om;
  struct ospf_vertex *v;
  struct ls_prefix lp;
  u_int32_t metric;
  u_char bits;

  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_SUMMARY_LSA_METRIC_OFFSET);
  ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN, lsa->data->id);

  /* (1) If the cost specified by the LSA is LSInfinity,
         then examine the next LSA.  */
  if (metric == OSPF_LS_INFINITY)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r metric is LS_INFINITY",
                   &area->area_id, lp.prefix);
      return;
    }

  if (IPV4_ADDR_SAME (&lsa->data->id, &area->top->router_id))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r is myself",
                   &area->area_id, lp.prefix);
      return;
    }

  /* (4) Else, call the destination described by the LSA N (for Type 3
         summary-LSAs, N's address is obtained by masking the LSA's
         Link State ID with the network/subnet mask contained in the
         body of the LSA), and the area border originating the LSA BR.
         Look up the routing table entry for BR having Area A as
         its associated area.  If no such entry exists for router BR
         (i.e., BR is unreachable in Area A), do nothing with this LSA
         and consider the next in the list.  Else, this LSA describes
         an inter-area path to destination N, whose cost is the distance
         to BR plus the cost specified in the LSA. Call the cost of
         this inter-area path IAC. */
  v = ospf_spf_vertex_lookup (area, lsa->data->adv_router);
  if (v == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r Can't find route to ABR (%r)",
                   &area->area_id, lp.prefix, &lsa->data->adv_router);
      return;
    }

  bits = OSPF_PNT_UCHAR_GET (v->lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);
  if (!IS_ROUTER_LSA_SET (bits, BIT_B))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r advertising router (%r) is not ABR",
                   &area->area_id, lp.prefix, &lsa->data->adv_router);
      return;
    }

  /* Set the new path.  */
  ospf_ia_calc_path_set (area, route, v, lsa, metric);
}

int
ospf_ia_calc_transit_prefix (struct ospf_area *area,
                             struct ospf_route *route, struct ospf_lsa *lsa)
{
  struct ospf_master *om = area->top->om;
  struct pal_in4_addr *mask;
  struct ospf_vertex *v;
  struct ls_prefix lp;
  u_int32_t metric;
  u_char masklen;
  u_char bits;

  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_SUMMARY_LSA_METRIC_OFFSET);
  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  ls_prefix_ipv4_set (&lp, masklen, lsa->data->id);

  /* (1) If the cost specified by the LSA is LSInfinity,
         then examine the next LSA.  */
  if (metric == OSPF_LS_INFINITY)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P metric is LS_INFINITY",
                   &area->area_id, &lp);
      return 0;
    }

  /* (4) Look up the routing table entry for the advertising router
         BR associated with the Area A. If it is unreachable, examine
         the next LSA. Otherwise, the cost to destination N is the
         sum of the cost in BR's Area A routing table entry and the
         cost advertised in the LSA. Call this cost IAC.  */
  v = ospf_spf_vertex_lookup (area, lsa->data->adv_router);
  if (v == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P Can't find route to ABR (%r)",
                   &area->area_id, &lp, &lsa->data->adv_router);
      return 0;
    }

  bits = OSPF_PNT_UCHAR_GET (v->lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);
  if (!IS_ROUTER_LSA_SET (bits, BIT_B))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P adv router (%r) is not ABR",
                   &area->area_id, &lp, &lsa->data->adv_router);
      return 0;
    }

  /* Set the new path.  */
  return ospf_ia_calc_transit_path_set (area, route, v, lsa, metric);
}

int
ospf_ia_calc_transit_router (struct ospf_area *area,
                             struct ospf_route *route, struct ospf_lsa *lsa)
{
  struct ospf_master *om = area->top->om;
  struct pal_in4_addr *router_id;
  struct ospf_vertex *v;
  u_int32_t metric;
  u_char bits;

  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_SUMMARY_LSA_METRIC_OFFSET);
  router_id = &lsa->data->id;

  /* (1) If the cost specified by the LSA is LSInfinity,
         then examine the next LSA.  */
  if (metric == OSPF_LS_INFINITY)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r metric is LS_INFINITY",
                   &area->area_id, router_id);
      return 0;
    }

  if (IPV4_ADDR_SAME (&lsa->data->id, &area->top->router_id))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r is myself",
                   &area->area_id, router_id);
      return 0;
    }

  /* (4) Look up the routing table entry for the advertising router
         BR associated with the Area A. If it is unreachable, examine
         the next LSA. Otherwise, the cost to destination N is the
         sum of the cost in BR's Area A routing table entry and the
         cost advertised in the LSA. Call this cost IAC.  */
  v = ospf_spf_vertex_lookup (area, lsa->data->adv_router);
  if (v == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r Can't find route to ABR (%r)",
                   &area->area_id, router_id, &lsa->data->adv_router);
      return 0;
    }

  bits = OSPF_PNT_UCHAR_GET (v->lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);
  if (!IS_ROUTER_LSA_SET (bits, BIT_B))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%s:R]: %r adv router (%r) is not ABR",
                   &area->area_id, router_id, &lsa->data->adv_router);
      return 0;
    }

  /* Set the new path.  */
  return ospf_ia_calc_transit_path_set (area, route, v, lsa, metric);
}


void
ospf_ia_calc_prefix_incremental (struct ospf_area *area, struct ospf_lsa *new)
{
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *lsa;
  struct ospf_route *route;
  struct ospf_path *path;
  struct pal_in4_addr *mask;
  u_char masklen;
  int i;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    zlog_info (ZG, "Route[IA:%r:N]: Calculate incremental summary for %r",
               &area->area_id, &new->data->id);

  if (area->spf == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: No SPF tree, schedule SPF calculation",
                   &area->area_id);
      ospf_spf_calculate_timer_add (area);
      return;
    }

  if (!IS_AREA_IA_CALCULATE (area))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: Cancel calculation", &area->area_id);
      return;
    }

  mask = OSPF_PNT_IN_ADDR_GET (new->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  /* (5) Next, look up the routing table entry for the destination N.
         (If N is an AS boundary router, look up the "router" routing
         table entry associated with Area A).  If no entry exists for
         N or if the entry's path type is "type 1 external" or "type 2
         external", then install the inter-area path to N, with
         associated area Area A, cost IAC, next hop equal to the list
         of next hops to router BR, and Advertising router equal to BR. */
  route = ospf_route_lookup (area->top, &new->data->id, masklen);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_add (area->top, &new->data->id, masklen, route);
    }
  else
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_INTER_AREA, area);
      if (path != NULL)
        for (i = 0; i < vector_max (path->lsa_vec); i++)
          if ((lsa = vector_slot (path->lsa_vec, i)))
            {
              vector_slot (path->lsa_vec, i) = NULL;

              if (!IPV4_ADDR_SAME (&lsa->data->id, &new->data->id)
                  || !IPV4_ADDR_SAME (&lsa->data->adv_router,
                                      &new->data->adv_router))
                if (!IS_LSA_SELF (lsa))
                  if (!IS_LSA_MAXAGE (lsa))
                    if (!CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
                      ospf_ia_calc_prefix (area, route, lsa);

              ospf_lsa_unlock (lsa);
            }
    }

  if (!IS_LSA_SELF (new))
    if (!IS_LSA_MAXAGE (new))
      if (!CHECK_FLAG (new->flags, OSPF_LSA_DISCARD))
        ospf_ia_calc_prefix (area, route, new);

  ospf_route_install (area->top, area, OSPF_PATH_INTER_AREA, route->rn);
  ospf_ase_calc_event_add (area->top);
}

void
ospf_ia_calc_router_incremental (struct ospf_area *area, struct ospf_lsa *new)
{
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *lsa;
  struct ospf_route *route;
  struct ospf_path *path;
  int i;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    zlog_info (ZG, "Route[IA:%r:R]: "
               "Calculate incremental ASBR-summary for %r",
               &area->area_id, &new->data->id);

  if (area->spf == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: No SPF tree, schedule SPF calculation",
                   &area->area_id);
      ospf_spf_calculate_timer_add (area);
      return;
    }

  if (!IS_AREA_IA_CALCULATE (area))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: Cancel calculation", &area->area_id);
      return;
    }

  /* (5) Next, look up the routing table entry for the destination N.
         (If N is an AS boundary router, look up the "router" routing
         table entry associated with Area A).  If no entry exists for
         N or if the entry's path type is "type 1 external" or "type 2
         external", then install the inter-area path to N, with
         associated area Area A, cost IAC, next hop equal to the list
         of next hops to router BR, and Advertising router equal to BR. */
  route = ospf_route_asbr_lookup (area->top, new->data->id);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_asbr_add (area->top, new->data->id, route);
    }
  else
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_INTER_AREA, area);
      if (path != NULL)
        for (i = 0; i < vector_max (path->lsa_vec); i++)
          if ((lsa = vector_slot (path->lsa_vec, i)))
            {
              vector_slot (path->lsa_vec, i) = NULL;

              if (!IPV4_ADDR_SAME (&lsa->data->id, &new->data->id)
                  || !IPV4_ADDR_SAME (&lsa->data->adv_router,
                                      &new->data->adv_router))
                if (!IS_LSA_SELF (lsa))
                  if (!IS_LSA_MAXAGE (lsa))
                    if (!CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
                      ospf_ia_calc_router (area, route, lsa);

              ospf_lsa_unlock (lsa);
            }
    }

  if (!IS_LSA_SELF (new))
    if (!IS_LSA_MAXAGE (new))
      if (!CHECK_FLAG (new->flags, OSPF_LSA_DISCARD))
        ospf_ia_calc_router (area, route, new);

  ospf_route_asbr_install (area->top, area, OSPF_PATH_INTER_AREA, route->rn);
}

void
ospf_ia_calc_transit_prefix_incremental (struct ospf_area *area,
                                         struct ospf_lsa *new)
{
  struct ospf_master *om = area->top->om;
  struct ospf_area *backbone = area->top->backbone;
  struct ospf_lsa *lsa;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ls_prefix lp;
  struct pal_in4_addr *mask;
  u_char masklen;
  int count = 0;
  int i;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    zlog_info (ZG, "Route[IA:%r:N]: "
               "Calculate transit incremental summary for %r",
               &area->area_id, &new->data->id);

  if (!IS_AREA_TRANSIT (area) || backbone == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: Cancel calculation", &area->area_id);
      return;
    }

  if (backbone->spf == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: No SPF tree, schedule SPF calculation",
                   &backbone->area_id);
      ospf_spf_calculate_timer_add (backbone);
      return;
    }

  mask = OSPF_PNT_IN_ADDR_GET (new->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  ls_prefix_ipv4_set (&lp, masklen, new->data->id);

  /* (3) Look up the routing table entry for N. (If N is an AS boundary
         router, look up the "router" routing table entry associated
         with the backbone area). If it does not exist, or if the route
         type is other than intra-area or inter-area, or if the area
         associated with the routing table entry is not the backbone area,
         then examine the next LSA. In other words, this calculation only
         updates backbone intra-area routes found in Section 16.1 and
         inter-area routes found in Section 16.2.  */
  route = ospf_route_lookup (area->top, &new->data->id, masklen);
  if (route == NULL
      || route->selected == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P no route to destination",
                   &area->area_id, &lp);
      return;
    }
  else if (route->selected->type != OSPF_PATH_CONNECTED
           && route->selected->type != OSPF_PATH_INTRA_AREA
           && route->selected->type != OSPF_PATH_INTER_AREA)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P wrong path type",
                   &area->area_id, &lp);
      return;
    }
  else if (!IS_AREA_ID_BACKBONE (route->selected->area_id))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: %P Path doesn't belongs to backbone",
                   &area->area_id, &lp);
      return;
    }

  path = route->selected->path_transit;
  if (path != NULL)
    for (i = 0; i < vector_max (path->lsa_vec); i++)
      if ((lsa = vector_slot (path->lsa_vec, i)))
        {
          vector_slot (path->lsa_vec, i) = NULL;

          if (!IPV4_ADDR_SAME (&lsa->data->id, &new->data->id)
              || !IPV4_ADDR_SAME (&lsa->data->adv_router,
                                  &new->data->adv_router))
            if (!IS_LSA_SELF (lsa))
              if (!IS_LSA_MAXAGE (lsa))
                if (!CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
                  count += ospf_ia_calc_transit_prefix (area, route, lsa);

          ospf_lsa_unlock (lsa);
        }

  if (!IS_LSA_SELF (new))
    if (!IS_LSA_MAXAGE (new))
      if (!CHECK_FLAG (new->flags, OSPF_LSA_DISCARD))
        count += ospf_ia_calc_transit_prefix (area, route, new);

  if (count > 0)
    {
      ospf_route_install (area->top, backbone,
                          route->selected->type, route->rn);
      ospf_ase_calc_event_add (area->top);
    }
}

void
ospf_ia_calc_transit_router_incremental (struct ospf_area *area,
                                         struct ospf_lsa *new)
{
  struct ospf_master *om = area->top->om;
  struct ospf_area *backbone = area->top->backbone;
  struct ospf_lsa *lsa;
  struct ospf_route *route;
  struct ospf_path *path;
  struct pal_in4_addr *router_id;
  int count = 0;
  int i;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    zlog_info (ZG, "Route[IA:%r:R]: "
               "Calculate transit incremental ASBR-summary for %r",
               &area->area_id, &new->data->id);

  if (!IS_AREA_TRANSIT (area) || backbone == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:N]: Cancel calculation", &area->area_id);
      return;
    }

  if (backbone->spf == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: No SPF tree, schedule SPF calculation",
                   &backbone->area_id);
      ospf_spf_calculate_timer_add (backbone);
      return;
    }

  router_id = &new->data->id;

  /* (3) Look up the routing table entry for N. (If N is an AS boundary
         router, look up the "router" routing table entry associated
         with the backbone area). If it does not exist, or if the route
         type is other than intra-area or inter-area, or if the area
         associated with the routing table entry is not the backbone area,
         then examine the next LSA. In other words, this calculation only
         updates backbone intra-area routes found in Section 16.1 and
         inter-area routes found in Section 16.2.  */
  route = ospf_route_asbr_lookup (area->top, *router_id);
  if (route == NULL
      || route->selected == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r no route to destination",
                   &area->area_id, router_id);
      return;
    }
  else if (route->selected->type != OSPF_PATH_CONNECTED
           && route->selected->type != OSPF_PATH_INTRA_AREA
           && route->selected->type != OSPF_PATH_INTER_AREA)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r wrong path type",
                   &area->area_id, router_id);
      return;
    }
  else if (!IS_AREA_ID_BACKBONE (route->selected->area_id))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
        zlog_info (ZG, "Route[IA:%r:R]: %r path doesn't belongs to backbone",
                   &area->area_id, router_id);
      return;
    }

  path = route->selected->path_transit;
  if (path != NULL)
    for (i = 0; i < vector_max (path->lsa_vec); i++)
      if ((lsa = vector_slot (path->lsa_vec, i)))
        {
          vector_slot (path->lsa_vec, i) = NULL;

          if (!IPV4_ADDR_SAME (&lsa->data->id, &new->data->id)
              || !IPV4_ADDR_SAME (&lsa->data->adv_router,
                                  &new->data->adv_router))
            if (!IS_LSA_SELF (lsa))
              if (!IS_LSA_MAXAGE (lsa))
                if (!CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
                  count += ospf_ia_calc_transit_router (area, route, lsa);

          ospf_lsa_unlock (lsa);
        }

  if (!IS_LSA_SELF (new))
    if (!IS_LSA_MAXAGE (new))
      if (!CHECK_FLAG (new->flags, OSPF_LSA_DISCARD))
        count += ospf_ia_calc_transit_router (area, route, new);

  if (count > 0)
    ospf_route_asbr_install (area->top, backbone,
                             route->selected->type, route->rn);
}


int
ospf_ia_calc_route (struct ospf_area *area, u_char type,
                    struct thread *thread, OSPF_IA_CALC_FUNC func)
{
  struct ospf_lsdb_slot *slot = &area->lsdb->type[type];
  struct ospf_lsa *lsa;
  int count = 0;

  for (lsa = LSDB_LOOP_HEAD (slot); lsa; lsa = lsa->next)
    if (!IS_LSA_SELF (lsa))
      if (!IS_LSA_MAXAGE (lsa))
        {
          if (++count > OSPF_IA_CALC_COUNT_THRESHOLD)
            {
              slot->next = lsa;
              return 0;
            }

          /* Calculate and install the summary LSA.  */
          (*func) (area, lsa);
        }

  /* Reset the suspend thread pointer.  */
  if (area->t_suspend == thread)
    area->t_suspend = NULL;

  /* Reset LSDB next pointer.  */
  slot->next = NULL;

  return 1;
}

int
ospf_ia_calc_prefix_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct thread *thread_self = area->t_ia_calc_prefix;
  struct ospf_master *om = area->top->om;
  struct pal_timeval st, et, rt;
  int ret;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  area->t_ia_calc_prefix = NULL;

  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_ia_calc_prefix_event, area, 0);
      return 0;
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      zlog_info (ZG, "Route[IA:%r]: IA prefix calculation timer expire",
                 &area->area_id);
      pal_time_tzcurrent (&st, NULL);
    }

  /* Calculate all summary-LSAs.  */
  ret = ospf_ia_calc_route (area, OSPF_SUMMARY_LSA, thread_self,
                            ospf_ia_calc_prefix_incremental);

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      pal_time_tzcurrent (&et, NULL);
      rt = TV_SUB (et, st);
      zlog_info (ZG, "Route[IA:%r]: Calculation %s [%d.%06d sec]",
                 &area->area_id, ret ? "completed" : "suspended",
                 rt.tv_sec, rt.tv_usec);
    }

  if (!ret)
    OSPF_EVENT_SUSPEND_ON (area->t_ia_calc_prefix,
                           ospf_ia_calc_prefix_event, area, 0);
  else
    OSPF_EVENT_PENDING_EXECUTE (area->event_pend);

  return 0;
}

int
ospf_ia_calc_router_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct thread *thread_self = area->t_ia_calc_router;
  struct ospf_master *om = area->top->om;
  struct pal_timeval st, et, rt;
  int ret;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);
  
  area->t_ia_calc_router = NULL;

  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_ia_calc_router_event, area, 0);
      return 0;
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      zlog_info (ZG, "Route[IA:%r]: IA router route calculation timer expire",
                 &area->area_id);
      pal_time_tzcurrent (&st, NULL);
    }

  /* Calculate all ASBR-summary-LSAs.  */
  ret = ospf_ia_calc_route (area, OSPF_SUMMARY_LSA_ASBR, thread_self,
                            ospf_ia_calc_router_incremental);

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      pal_time_tzcurrent (&et, NULL);
      rt = TV_SUB (et, st);
      zlog_info (ZG, "Route[IA:%r]: Calculation %s [%d.%06d sec]",
                 &area->area_id, ret ? "completed" : "suspended",
                 rt.tv_sec, rt.tv_usec);
    }

  if (!ret)
    OSPF_EVENT_SUSPEND_ON (area->t_ia_calc_router,
                           ospf_ia_calc_router_event, area, 0);
  else
    OSPF_EVENT_PENDING_EXECUTE (area->event_pend);

  return 0;
}

int
ospf_ia_calc_transit_prefix_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct thread *thread_self = area->t_ia_calc_prefix;
  struct ospf_master *om = area->top->om;
  struct pal_timeval st, et, rt;
  int ret;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);
  
  area->t_ia_calc_prefix = NULL;

  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_ia_calc_transit_prefix_event, area, 0);
      return 0;
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      zlog_info (ZG, "Route[IA:%r]: IA transit prefix route calculation "
                 "timer expire", &area->area_id);
      pal_time_tzcurrent (&st, NULL);
    }

  /* Calculate all summary-LSAs.  */
  ret = ospf_ia_calc_route (area, OSPF_SUMMARY_LSA, thread_self,
                            ospf_ia_calc_transit_prefix_incremental);

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      pal_time_tzcurrent (&et, NULL);
      rt = TV_SUB (et, st);
      zlog_info (ZG, "Route[IA:%r]: Calculation %s [%d.%06d sec]",
                 &area->area_id, ret ? "completed" : "suspended",
                 rt.tv_sec, rt.tv_usec);
    }

  if (!ret)
    OSPF_EVENT_SUSPEND_ON (area->t_ia_calc_prefix,
                           ospf_ia_calc_transit_prefix_event, area, 0);
  else
    OSPF_EVENT_PENDING_EXECUTE (area->event_pend);

  return 0;
}

int
ospf_ia_calc_transit_router_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct thread *thread_self = area->t_ia_calc_router;
  struct ospf_master *om = area->top->om;
  struct pal_timeval st, et, rt;
  int ret;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);
  
  area->t_ia_calc_router = NULL;

  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_ia_calc_transit_router_event, area, 0);
      return 0;
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      zlog_info (ZG, "Route[IA:%r]: IA transit router route calculation "
                 "timer expire", &area->area_id);
      pal_time_tzcurrent (&st, NULL);
    }

  /* Calculate all ASBR-summary-LSAs.  */
  ret = ospf_ia_calc_route (area, OSPF_SUMMARY_LSA_ASBR, thread_self,
                            ospf_ia_calc_transit_router_incremental);

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    {
      pal_time_tzcurrent (&et, NULL);
      rt = TV_SUB (et, st);
      zlog_info (ZG, "Route[IA:%r]: Calculation %s [%d.%06d sec]",
                 &area->area_id, ret ? "completed" : "suspended",
                 rt.tv_sec, rt.tv_usec);
    }

  if (!ret)
    OSPF_EVENT_SUSPEND_ON (area->t_ia_calc_router,
                           ospf_ia_calc_transit_router_event, area, 0);
  else
    OSPF_EVENT_PENDING_EXECUTE (area->event_pend);

  return 0;
}

int
ospf_ia_calc_incremental_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct thread *thread_self = area->t_lsa_incremental;
  struct ospf_lsa *lsa;
  int i;
  struct ospf_master *om = area->top->om;

  /* Set VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);
  
  area->t_lsa_incremental = NULL;

  if (area->incr_defer)
    return 0;

  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_ia_calc_incremental_event, area, 0);
      return 0;
    }

  for (i = 0; i < vector_max (area->lsa_incremental); i++)
    if ((lsa = vector_slot (area->lsa_incremental, i)))
      {
        if (!area->incr_defer)
          {
            /* Incremental summary-LSA update.  */
            if (lsa->data->type == OSPF_SUMMARY_LSA)
              {
                if ((area->incr_count >= OSPF_MAX_IA_INCR_PER_SEC)
                     && area->t_spf_calc != NULL)
                  {
                    area->incr_defer = PAL_TRUE;
                    OSPF_AREA_TIMER_ON (area->t_ia_inc_calc,
                                        ospf_ia_incr_counter_rst, 5);
                  }
                else
                  {
                    area->incr_count++;
                    if (IS_AREA_IA_CALCULATE (area))
                      ospf_ia_calc_prefix_incremental (area, lsa);
                    else if (IS_AREA_TRANSIT (area) && area->top->backbone)
                      ospf_ia_calc_transit_prefix_incremental (area, lsa);
                  }
              } 
            /* Incremental ASBR-summary-LSA update.  */
            else if (lsa->data->type == OSPF_SUMMARY_LSA_ASBR)
              {
                if ((area->incr_count >= OSPF_MAX_IA_INCR_PER_SEC)
                     && area->t_spf_calc != NULL)
                  {
                    area->incr_defer = PAL_TRUE;
                    OSPF_AREA_TIMER_ON (area->t_ia_inc_calc,
                                        ospf_ia_incr_counter_rst, 5);
                  }
                else
                  {
                    area->incr_count++;

                    if (IS_AREA_IA_CALCULATE (area))
                      ospf_ia_calc_router_incremental (area, lsa);
                    else if (IS_AREA_TRANSIT (area) && area->top->backbone)
                      ospf_ia_calc_transit_router_incremental (area, lsa);
                  }
              }
          }

        vector_slot (area->lsa_incremental, i) = NULL;
        ospf_lsa_unlock (lsa);
      }

  return 0;
}

int
ospf_ia_incr_counter_rst (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct ospf_master *om = area->top->om;

   /* Set VR context */
   LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  area->t_ia_inc_calc = NULL;
  area->incr_count =0;
  area->incr_defer = PAL_FALSE;
  OSPF_AREA_TIMER_ON (area->t_ia_inc_calc, ospf_ia_incr_counter_rst, 5);

  return 0;
}

void
ospf_ia_calc_route_clean (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *lsa;
  int i;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
    zlog_info (ZG, "Route[IA:%r]: Cleanup IA route", &area->area_id);

  /* Cancel threads.  */
  OSPF_EVENT_SUSPEND_OFF (area->t_ia_calc_prefix, area);
  OSPF_EVENT_SUSPEND_OFF (area->t_ia_calc_router, area);
  OSPF_EVENT_OFF (area->t_lsa_incremental);
  OSPF_TIMER_OFF (area->t_ia_inc_calc);

  /* Reset the next candidate pointer.  */
  area->lsdb->type[OSPF_SUMMARY_LSA].next = NULL;
  area->lsdb->type[OSPF_SUMMARY_LSA_ASBR].next = NULL;

  /* Cleanup the pending incremental LSAs.  */
  for (i = 0; i < vector_max (area->lsa_incremental); i++)
    if ((lsa = vector_slot (area->lsa_incremental, i)))
      {
        vector_slot (area->lsa_incremental, i) = NULL;
        ospf_lsa_unlock (lsa);
      }

  /* Cleanup the inter-area routes.  */
  ospf_route_install_all_event_add (area, OSPF_PATH_INTER_AREA);

  /* Cleanup the inter-area ASBR routes.  */
  ospf_route_asbr_install_all_event_add (area, OSPF_PATH_INTER_AREA);
}

#define OSPF_IA_CALC_EVENT_RESET(T,A,V)                                       \
    do {                                                                      \
      struct ospf_lsdb_slot *slot = &(A)->lsdb->type[(V)];                    \
                                                                              \
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))                                \
        zlog_info (ZG, "Route[IA:%r]: Restart IA calc event",                 \
                   &area->area_id);                                           \
                                                                              \
      /* Reset the LSDB next pointer.  */                                     \
      if (IS_AREA_THREAD_SUSPENDED (A, T))                                    \
        slot->next = NULL;                                                    \
                                                                              \
      /* Cancel the thread.  */                                               \
      OSPF_EVENT_SUSPEND_OFF (T, A);                                          \
                                                                              \
      /* Reset the thread pointer.  */                                        \
      (T) = NULL;                                                             \
    } while (0)

void
ospf_ia_calc_event_add (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;

  if (area->abr_count == 0)
    {
      /* Cleanup the IA routes if there is no ABR.  */
      ospf_ia_calc_route_clean (area);
      return;
    }

  /* Cancel the current suspended event if we have.  */
  if (area->t_ia_calc_prefix != NULL)
    OSPF_IA_CALC_EVENT_RESET (area->t_ia_calc_prefix,
                              area, OSPF_SUMMARY_LSA);
  if (area->t_ia_calc_router != NULL)
    OSPF_IA_CALC_EVENT_RESET (area->t_ia_calc_router,
                              area, OSPF_SUMMARY_LSA_ASBR);

  if (IS_AREA_IA_CALCULATE (area))
    {
      OSPF_EVENT_ON (area->t_ia_calc_prefix, ospf_ia_calc_prefix_event, area);
      OSPF_EVENT_ON (area->t_ia_calc_router, ospf_ia_calc_router_event, area);
    }
  else
    {
      /* Cleanup the IA routes.  */
      ospf_ia_calc_route_clean (area);

      /* Calculate for the transit area.  */
      if (IS_AREA_TRANSIT (area) && area->top->backbone != NULL)
        {
          OSPF_EVENT_ON (area->t_ia_calc_prefix,
                         ospf_ia_calc_transit_prefix_event, area);
          OSPF_EVENT_ON (area->t_ia_calc_router,
                         ospf_ia_calc_transit_router_event, area);
        }
    }
}

void
ospf_ia_calc_incremental_event_add (struct ospf_lsa *lsa)
{
  struct ospf_area *area = lsa->lsdb->u.area;

  if (!IS_AREA_ACTIVE (area))
    return;

  /* Set LSA to the incremental vector.  */
  if (!area->incr_defer)
    vector_set (area->lsa_incremental, ospf_lsa_lock (lsa));
  else
    return;

  OSPF_EVENT_ON (area->t_lsa_incremental,
                 ospf_ia_calc_incremental_event, area);
}
