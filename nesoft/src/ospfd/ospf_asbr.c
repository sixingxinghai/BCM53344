/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "linklist.h"
#include "filter.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_VRF_OSPF
#include "ospfd/ospf_vrf.h"
#endif /* HAVE_VRF_OSPF */


int
ospf_asbr_status_get (struct ospf *top)
{
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;
#endif /* HAVE_NSSA */
  int i;

  for (i = 0; i < APN_ROUTE_MAX; i++)
    if (REDIST_PROTO_CHECK (&top->redist[i]))
      return 1;

#ifdef HAVE_NSSA
  /* Check for the default-information-originate configuration
     in NSSA areas.  */
  if (top->nssa_count)
    for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
      if ((area = RN_INFO (rn, RNI_DEFAULT)))
        if (IS_AREA_NSSA (area))
          if (IS_OSPF_ABR (area->top) 
          || OSPF_AREA_CONF_CHECK (area, DEFAULT_ORIGINATE))
            {
              ls_unlock_node (rn);
              return 1;
            }
#endif /* HAVE_NSSA */

  return 0;
}

/* Update ASBR status. */
void
ospf_asbr_status_update (struct ospf *top)
{
  int status = ospf_asbr_status_get (top);
  struct ospf_master *om = top->om;

  /* Non-ASBR to ASBR.  */
  if (!IS_OSPF_ASBR (top) && status)
    {
      if (IS_DEBUG_OSPF (event, EVENT_ASBR))
        zlog_info (ZG, "ROUTER[%d]: Change status to ASBR", top->ospf_id);

      SET_FLAG (top->flags, OSPF_ROUTER_ASBR);

      ospf_router_lsa_refresh_all (top);
    }

  /* ASBR to Non-ASBR.  */
  else if (IS_OSPF_ASBR (top) && !status)
    {
      if (IS_DEBUG_OSPF (event, EVENT_ASBR))
        zlog_info (ZG, "ROUTER[%d]: Change status to non-ASBR", top->ospf_id);

      UNSET_FLAG (top->flags, OSPF_ROUTER_ASBR);

      ospf_router_lsa_refresh_all (top);
    }
}


#define DEFAULT_DEFAULT_METRIC       20
#define DEFAULT_ORIGINATE_METRIC     10
#define DEFAULT_ALWAYS_METRIC         1
#define DEFAULT_BGP_METRIC            1

#define DEFAULT_METRIC_TYPE                  EXTERNAL_METRIC_TYPE_2

u_int32_t
ospf_metric_value (struct ospf *top, u_char type, u_char source, int pid)
{
  struct ospf_redist_conf *rc;

  rc = ospf_redist_conf_lookup_by_inst (top, type, pid);

  if (rc == NULL)
    return 0;

  if (CHECK_FLAG (rc->flags, OSPF_REDIST_METRIC))
    return rc->metric;

  if (type == APN_ROUTE_DEFAULT)
    {
      if (CHECK_FLAG (source, OSPF_REDIST_MAP_NSM))
        return DEFAULT_ORIGINATE_METRIC;
      else
        return DEFAULT_ALWAYS_METRIC;
    }

  if (!CHECK_FLAG (top->config, OSPF_CONFIG_DEFAULT_METRIC))
    {
      if (type == APN_ROUTE_BGP)
        return DEFAULT_BGP_METRIC;
      else
        return DEFAULT_DEFAULT_METRIC;
    }

  return top->default_metric;
}


#ifdef HAVE_NSSA
struct pal_in4_addr *
ospf_redist_map_nssa_match_nexthop (struct ospf_area *area,
                                    struct pal_in4_addr *nexthop)
{
  struct ospf_interface *oi;
  struct listnode *node;
  struct prefix p;

  if (nexthop != NULL && nexthop->s_addr != 0)
    {
      pal_mem_set (&p, 0, sizeof (struct prefix));
      p.family = AF_INET;
      p.prefixlen = IPV4_MAX_BITLEN;
      p.u.prefix4 = *nexthop;

      LIST_LOOP (area->iflist, oi, node)
        if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
          if (prefix_match (oi->ident.address, &p))
            return nexthop;
    }

  return NULL;
}

struct pal_in4_addr
ospf_redist_map_nssa_get_nexthop (struct ospf_area *area)
{
  struct ospf_interface *oi;
  struct listnode *node;

  LIST_LOOP (area->iflist, oi, node)
    if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
      return oi->ident.address->u.prefix4;

  return IPv4AddressUnspec;
}

struct pal_in4_addr
ospf_redist_map_nssa_update_nexthop (struct ospf_redist_map *map)
{
  struct pal_in4_addr *nexthop = NULL;
  struct pal_in4_addr *nexthop_old;

  if (map->lsa != NULL)
    {
      nexthop_old =
        OSPF_PNT_IN_ADDR_GET (map->lsa->data,
                              OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);
      nexthop = ospf_redist_map_nssa_match_nexthop (map->u.area, nexthop_old);
    }

  if (nexthop == NULL)
    return ospf_redist_map_nssa_get_nexthop (map->u.area);
  else
    return *nexthop;
}
#endif /* HAVE_NSSA */

int
ospf_redist_map_arg_get_nsm (struct ospf *top, struct ospf_redist_map *map,
                             struct ospf_redist_arg *ra)
{
  struct ospf_redist_conf *orc;
  struct ospf_route_map_arg rma;
  u_char type;

  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_CONFIG_DEFAULT))
    type = APN_ROUTE_DEFAULT;
  else
    type = map->ri->type;

  if (type != APN_ROUTE_DEFAULT)
    orc = ospf_redist_conf_lookup_by_inst (top, map->ri->type, map->ri->pid);
  else
    orc = &top->redist[type];

  if (orc == NULL)
    return 0;

  /* Check if connected. */
  if (type == APN_ROUTE_CONNECT
      && ospf_network_match (top, map->lp))
    return 0;

  /* Apply distribute-list. */
  if (DIST_LIST (orc)
      && access_list_apply (DIST_LIST (orc), map->lp) != FILTER_PERMIT)
    return 0;

  /* Prepare redistribute arguments. */
  pal_mem_set (&rma, 0, sizeof (struct ospf_route_map_arg));
  rma.top = top;
  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
    rma.arg.ifindex = map->ri->nexthop[0].ifindex; /* XXX */
#ifdef HAVE_NSSA
  if (map->type == OSPF_AS_NSSA_LSA
      && type == APN_ROUTE_DEFAULT)
    {
      if (OSPF_AREA_CONF_CHECK (map->u.area, METRIC))
        rma.arg.metric = map->u.area->conf.metric;
      else if (OSPF_AREA_CONF_CHECK (map->u.area, DEFAULT_COST))
        rma.arg.metric = OSPF_AREA_DEFAULT_COST (map->u.area);
      else
        rma.arg.metric = 1;

      if (OSPF_AREA_CONF_CHECK (map->u.area, METRIC_TYPE))
        rma.arg.metric_type = map->u.area->conf.metric_type;
      else
        rma.arg.metric_type = EXTERNAL_METRIC_TYPE_2;
    }
  else
#endif /* HAVE_NSSA */
#ifdef HAVE_VRF_OSPF
  /* If the route recieved from CE via OSPF_VRF 
   * is External and the metric type is typeE2
   * then set the metric_type argument as typeE2
   */
  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
      && (map->ri->domain_info.r_type == OSPF_AS_EXTERNAL_LSA))
    {
      rma.arg.metric = map->ri->metric;
      if (map->ri->domain_info.metric_type_E2)  
        rma.arg.metric_type = EXTERNAL_METRIC_TYPE_2;
      else 
        rma.arg.metric_type = EXTERNAL_METRIC_TYPE_1;
    }
  else
#endif /* HAVE_VRF_OSPF */
    {
      rma.arg.metric_type = METRIC_TYPE (orc);
      rma.arg.metric = ospf_metric_value (top, type, map->source, orc->pid);
    }

  if (rma.arg.metric >= OSPF_LS_INFINITY)
    rma.arg.metric = OSPF_LS_INFINITY;

  /* If user have not configured any tag value then use the
     tag value associated with the route */
  if (!CHECK_FLAG (orc->flags, OSPF_REDIST_TAG)
      && (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
      && CHECK_FLAG (map->ri->flags, OSPF_STATIC_TAG_VALUE)))
    REDIST_TAG (orc) = map->ri->tag;

  rma.arg.tag = REDIST_TAG (orc);
#ifdef HAVE_NSSA
  if (map->type == OSPF_AS_NSSA_LSA)
    {
      if (!ospf_area_default_count (top))
        rma.arg.propagate = 1;
    }
#endif /* HAVE_NSSA */

  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
    {
      rma.arg.nexthop = map->ri->nexthop[0].addr;
      rma.nsm_nexthop = map->ri->nexthop[0].addr;
    }

  /* Check if the nexhtop is reachable through the OSPF routing domain.  */
  if (map->type == OSPF_AS_EXTERNAL_LSA)
    {
      if (!ospf_route_match_internal (top, &rma.arg.nexthop))
        rma.arg.nexthop = IPv4AddressUnspec;
    }
#ifdef HAVE_NSSA
  else
    {
      if (!ospf_redist_map_nssa_match_nexthop (map->u.area, &rma.arg.nexthop))
        rma.arg.nexthop = ospf_redist_map_nssa_update_nexthop (map);
    }
#endif /* HAVE_NSSA */

  /* Apply route-map. */
  if (RMAP_NAME (orc))
    {
      int ret = RMAP_DENYMATCH;
      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
        rma.metric = map->ri->metric;

      if (RMAP_MAP (orc))
        ret = route_map_apply (RMAP_MAP (orc), (struct prefix *)map->lp, &rma);

      if (ret == RMAP_DENYMATCH)
        return 0;
    }

  *ra = rma.arg;

  return 1;
}

#ifdef HAVE_NSSA
int
ospf_redist_map_arg_get_translate (struct ospf *top,
                                   struct ospf_redist_map *map,
                                   struct ospf_redist_arg *ra)
{
  struct pal_in4_addr *nexthop;
  u_char e_bit = OSPF_EXTERNAL_LSA_E_BIT (map->nssa_lsa->data);

  nexthop = OSPF_PNT_IN_ADDR_GET (map->nssa_lsa->data,
                                  OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);

  if (!IS_EXTERNAL_METRIC (e_bit))
    ra->metric_type = EXTERNAL_METRIC_TYPE_1;
  else
    ra->metric_type = EXTERNAL_METRIC_TYPE_2;

  ra->metric = OSPF_PNT_UINT24_GET (map->nssa_lsa->data,
                                    OSPF_EXTERNAL_LSA_METRIC_OFFSET);
  ra->nexthop = *nexthop;
  ra->tag = OSPF_PNT_UINT32_GET (map->nssa_lsa->data,
                                 OSPF_EXTERNAL_LSA_TAG_OFFSET);

  return 1;
}
#endif /* HAVE_NSSA */

int
ospf_redist_arg_cmp (struct ospf_redist_arg *ra1,
                     struct ospf_redist_arg *ra2)
{
  if (ra1->metric_type < ra2->metric_type)
    return -1;
  else if (ra1->metric_type > ra2->metric_type)
    return 1;

  if (ra1->metric < ra2->metric)
    return -1;
  else if (ra1->metric > ra2->metric)
    return 1;

  return 0;
}

int
ospf_redist_map_arg_set (struct ospf *top, struct ospf_redist_map *map,
                         struct ospf_redist_arg *arg)
{
  struct ospf_redist_arg ra1, ra2;
  int ret1 = 0;
  int ret2 = 0;

  pal_mem_set (&ra1, 0, sizeof (struct ospf_redist_arg));
  pal_mem_set (&ra2, 0, sizeof (struct ospf_redist_arg));

  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
      || CHECK_FLAG (map->source, OSPF_REDIST_MAP_CONFIG_DEFAULT))
    ret1 = ospf_redist_map_arg_get_nsm (top, map, &ra1);

#ifdef HAVE_NSSA
  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_TRANSLATE))
    ret2 = ospf_redist_map_arg_get_translate (top, map, &ra2);
#endif /* HAVE_NSSA */

  if (ret1 == 0 && ret2 == 0)
    {
      LSA_MAP_FLUSH (map->lsa);
      return 0;
    }
  else if (ret1 != 0 && ret2 == 0)
    *arg = ra1;
  else if (ret1 == 0 && ret2 != 0)
    *arg = ra2;
  else /*  ret1 != 0 && ret2 != 0 */
    {
      if (ospf_redist_arg_cmp (&ra1, &ra2) <= 0)
        *arg = ra1;
      else
        *arg = ra2;
    }

  if (arg->metric >= OSPF_LS_INFINITY)
    {
      LSA_MAP_FLUSH (map->lsa);
      return 0;
    }

  return 1;
}

void
ospf_redist_map_nexthop_update_by_if_up (struct ospf *top,
                                         struct ospf_interface *oi)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;
  struct ls_prefix p, q;

  ls_prefix_ipv4_set (&p, oi->ident.address->prefixlen,
                      oi->ident.address->u.prefix4);

  /* If previously originate external-LSA's forwarding address is 0.0.0.0,
     and interface is brought up, update LSAs.  */
  for (rn = ls_table_top (top->redist_table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (IPV4_ADDR_SAME (&map->arg.nexthop, &IPv4AddressUnspec))
        if (map->ri)
          {
            ls_prefix_ipv4_set (&q, IPV4_MAX_BITLEN, map->ri->nexthop[0].addr);

            if (ls_prefix_match (&p, &q))
              ospf_redist_map_lsa_refresh (map);
          }
}

void
ospf_redist_map_nexthop_update_by_if_down (struct ospf *top,
                                           struct ospf_interface *oi)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;
  struct ls_prefix p, q;

  if (!if_is_up (oi->u.ifp))
    return;

  ls_prefix_ipv4_set (&p, oi->ident.address->prefixlen,
                      oi->ident.address->u.prefix4);

  /* If previously originate external-LSA's forwarding address is not 0.0.0.0,
     and interface is brought down, update LSAs.  */
  for (rn = ls_table_top (top->redist_table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (!IPV4_ADDR_SAME (&map->arg.nexthop, &IPv4AddressUnspec))
        if (map->ri)
          {
            ls_prefix_ipv4_set (&q, IPV4_MAX_BITLEN, map->ri->nexthop[0].addr);

            if (ls_prefix_match (&p, &q))
              ospf_redist_map_lsa_refresh (map);
          }
}


struct ospf_summary *
ospf_summary_new ()
{
  struct ospf_summary *summary;

  summary = XMALLOC (MTYPE_OSPF_SUMMARY, sizeof (struct ospf_summary));
  pal_mem_set (summary, 0, sizeof (struct ospf_summary));
  SET_FLAG (summary->flags, OSPF_SUMMARY_ADDRESS_ADVERTISE);

  return summary;
}

void
ospf_summary_free (struct ospf_summary *summary)
{
  OSPF_TIMER_OFF (summary->t_update);
  XFREE (MTYPE_OSPF_SUMMARY, summary);
}

struct ospf_summary *
ospf_summary_lookup (struct ls_table *table, struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_lookup (table, lp);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_summary *
ospf_summary_match (struct ls_table *table, struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_match (table, lp);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_summary *
ospf_summary_lookup_by_addr (struct ls_table *table,
                             struct pal_in4_addr addr, u_char masklen)
{
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, masklen, addr);

  return ospf_summary_lookup (table, &p);
}

void
ospf_summary_address_add (struct ospf *top, struct ospf_summary *summary,
                          struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_get (top->summary, lp);
  if (RN_INFO (rn, RNI_DEFAULT) == NULL)
    {
      summary->top = top;
      summary->lp = rn->p;
      RN_INFO_SET (rn, RNI_DEFAULT, summary);

      OSPF_TIMER_ON (summary->t_update, ospf_summary_address_timer,
                     summary, OSPF_SUMMARY_TIMER_DELAY);

    }
  ls_unlock_node (rn);
}

void
ospf_summary_address_delete_update (struct ospf_summary *summary, u_char type,
                                    struct ls_table *redist_table)
{
  struct ospf *top = summary->top;
  struct ospf_summary *summary_alt;
  struct ospf_redist_map *map, *map_summary;
  struct ls_node *rn, *rn_tmp;

  rn_tmp = ls_node_lookup (redist_table, summary->lp);
  if (rn_tmp == NULL)
    return;

  map_summary = RN_INFO (rn_tmp, RNI_DEFAULT);

  if (!CHECK_FLAG (map_summary->source, OSPF_REDIST_MAP_NSM))
    LSA_MAP_FLUSH (map_summary->lsa);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      {
        summary_alt = ospf_summary_match (top->summary, rn->p);
        if (summary_alt)
          {
            OSPF_TIMER_ON (summary_alt->t_update, ospf_summary_address_timer,
                           summary_alt, OSPF_SUMMARY_TIMER_DELAY);
          }
        else if (map_summary != map)
          {
            UNSET_FLAG (map->flags, OSPF_REDIST_MAP_MATCH_SUMMARY);
            ospf_redist_map_arg_set (top, map, &map->arg);
            LSA_REFRESH_TIMER_ADD (top, type, map->lsa, map);
          }
      }

  UNSET_FLAG (map_summary->source, OSPF_REDIST_MAP_SUMMARY);
  map_summary->summary = NULL;

  if (CHECK_FLAG (map_summary->source, OSPF_REDIST_MAP_NSM))
    {
      ospf_redist_map_arg_set (top, map_summary, &map_summary->arg);
      LSA_REFRESH_TIMER_ADD (top, type, map_summary->lsa, map_summary);
    }
  else
    {
      RN_INFO_UNSET (rn_tmp, RNI_DEFAULT);
      ospf_redist_map_free (map_summary);
    }

  ls_unlock_node (rn_tmp);
}

void
ospf_summary_address_withdraw_all (struct ospf *top,
                                   struct ospf_summary *summary)
{
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;
#endif /* HAVE_NSSA */

  ospf_summary_address_delete_update (summary, OSPF_AS_EXTERNAL_LSA,
                                      top->redist_table);

#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_NSSA (area))
        ospf_summary_address_delete_update (summary, OSPF_AS_NSSA_LSA,
                                            area->redist_table);
#endif /* HAVE_NSSA */
}

void
ospf_summary_address_delete (struct ospf *top, struct ospf_summary *summary)
{
  struct ls_node *rn;

  rn = ls_node_lookup (top->summary, summary->lp);
  if (rn)
    {
      ospf_route_delete_discard (top, NULL, summary->lp);
      RN_INFO_UNSET (rn, RNI_DEFAULT);

      ospf_summary_address_withdraw_all (top, summary);

      ls_unlock_node (rn);
      ospf_summary_free (summary);
    }
}

void
ospf_summary_address_delete_all (struct ospf *top)
{
  struct ospf_summary *summary;
  struct ls_node *rn;

  for (rn = ls_table_top (top->summary); rn; rn = ls_route_next (rn))
    if ((summary = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_summary_free (summary);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }

  ls_table_finish (top->summary);
}

void
ospf_summary_address_update (struct ospf_summary *summary,
                             struct ls_table *redist_table,
                             u_char type, void *parent)
{
  struct ospf *top = summary->top;
  struct ospf_summary *summary_alt;
  struct ospf_redist_map *map = NULL, *map_summary;
  struct ospf_redist_arg arg, ra_old;
  struct ls_node *rn, *rn_tmp;
  struct pal_in4_addr *nexthop = NULL;
  int cost = 0;
  int cost_type2 = 0;
  u_int32_t matched = 0;
  u_int32_t matched_type2 = 0;
  int exact = 0;

  /* Set map to redist table. */
  rn_tmp = ls_node_get (redist_table, summary->lp);
  if ((map_summary = RN_INFO (rn_tmp, RNI_DEFAULT)) == NULL)
    {
      map_summary = ospf_redist_map_new ();
      map_summary->summary = summary;
      map_summary->lp = rn_tmp->p;
      map_summary->type = type;
      map_summary->u.parent = parent;
      RN_INFO_SET (rn_tmp, RNI_DEFAULT, map_summary);
    }
  SET_FLAG (map_summary->source, OSPF_REDIST_MAP_SUMMARY);
  ra_old = map_summary->arg;
  map_summary->arg.tag = summary->tag;
#ifdef HAVE_NSSA
  if (map_summary->type == OSPF_AS_NSSA_LSA)
    {
      struct ospf_area *area = parent;

      if (!ospf_area_default_count (area->top))
        map_summary->arg.propagate = 1;
    }
#endif /* HAVE_NSSA */

  /* Traverse all redist map under summary address. */
  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((map = RN_INFO (rn, RNI_DEFAULT)) != NULL)
      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
          || CHECK_FLAG (map->source, OSPF_REDIST_MAP_TRANSLATE))
        {
          summary_alt = ospf_summary_match (top->summary, rn->p);
          if (summary_alt == summary)
            {
              if (!ospf_redist_map_arg_set (top, map, &arg))
                continue;

              SET_FLAG (map->flags, OSPF_REDIST_MAP_MATCH_SUMMARY);
              if (summary->lp->prefixlen == rn->p->prefixlen)
                {
                  exact++;
                  nexthop = &arg.nexthop;
                }

              if (arg.metric_type == EXTERNAL_METRIC_TYPE_1)
                matched++;
              else
                {
                  matched_type2++;
                  if (cost_type2 < arg.metric)
                    cost_type2 = arg.metric;
                }

              if (cost < arg.metric)
                cost = arg.metric;

              if (map != map_summary)
                LSA_MAP_FLUSH (map->lsa);
            }
        }
  ls_unlock_node (rn_tmp);

  /* Update summarized route. */
  if (matched + matched_type2 > 0
      && CHECK_FLAG (summary->flags, OSPF_SUMMARY_ADDRESS_ADVERTISE))
    {
      if (matched_type2 == 0)
        {
          map_summary->arg.metric_type = EXTERNAL_METRIC_TYPE_1;
          map_summary->arg.metric = cost;
        }
      else
        {
          map_summary->arg.metric_type = EXTERNAL_METRIC_TYPE_2;
          map_summary->arg.metric = cost_type2;

          if (matched + matched_type2 > 1)
            {
              /* RFC 3101 sec 3.2 Translating Type-7 LSAs into Type-5 LSAs.
                 If the range's path type is 2 its metric is the highest Type-2
                 cost + 1 amongst these LSAs. */
              if (map != NULL && CHECK_FLAG (map->source, OSPF_REDIST_MAP_TRANSLATE))
                map_summary->arg.metric++;
              nexthop = &IPv4AddressUnspec;
            }
        }

      if (map_summary->type == OSPF_AS_EXTERNAL_LSA)
        {
          if (ospf_route_match_internal (top, nexthop))
            map_summary->arg.nexthop = *nexthop;
          else
            map_summary->arg.nexthop = IPv4AddressUnspec;
        }
#ifdef HAVE_NSSA
      else
        {
          if (ospf_redist_map_nssa_match_nexthop (parent, nexthop))
            map_summary->arg.nexthop = *nexthop;
          else
            map_summary->arg.nexthop
              = ospf_redist_map_nssa_update_nexthop (map_summary);
        }
#endif /* HAVE_NSSA */

      if (map_summary->lsa == NULL
          || map_summary->arg.metric_type != ra_old.metric_type
          || map_summary->arg.metric != ra_old.metric
          || map_summary->arg.nexthop.s_addr != ra_old.nexthop.s_addr
          || map_summary->arg.tag != ra_old.tag)
        LSA_REFRESH_TIMER_ADD (top, type, map_summary->lsa, map_summary);

      if (++summary->matched == 1)
        ospf_route_add_discard (top, NULL, summary->lp);
    }
  else
    {
      SET_FLAG (map_summary->flags, OSPF_REDIST_MAP_NOT_ADVERTISE);
      LSA_MAP_FLUSH (map_summary->lsa);

      if (--summary->matched == 0)
        ospf_route_delete_discard (top, NULL, summary->lp);
    }
}

int
ospf_summary_address_timer (struct thread *thread)
{
  struct ospf_summary *summary = THREAD_ARG (thread);
  struct ospf *top = summary->top;
  struct ospf_master *om = top->om;
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;
#endif /* HAVE_NSSA */

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_ASBR))
    zlog_info (ZG, "ROUTER[%d] Summary-address timer expire", top->ospf_id);

  summary->t_update = NULL;

  ospf_summary_address_update (summary, top->redist_table,
                               OSPF_AS_EXTERNAL_LSA, top);

#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        if (IS_AREA_NSSA (area))
          ospf_summary_address_update (summary, area->redist_table,
                                       OSPF_AS_NSSA_LSA, area);
#endif /* HAVE_NSSA */

  return 0;
}
