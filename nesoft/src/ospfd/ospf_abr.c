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
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_ia.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_VRF_OSPF
#include "ospfd/ospf_vrf.h"
#endif


#ifdef HAVE_NSSA
int
ospf_abr_nssa_translate_timer (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *lsa;
  struct ls_node *rn;

   /* Set VR context */
   LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);
  
  area->t_nssa = NULL;

  if (IS_DEBUG_OSPF (event, EVENT_NSSA))
    zlog_info (ZG, "NSSA[%r]: Translate timer expire", &area->area_id);

  LSA_REFRESH_TIMER_ADD (area->top, OSPF_ROUTER_LSA,
                         ospf_router_lsa_self (area), area);

  LSDB_LOOP (NSSA_LSDB (area), rn, lsa)
    if (!ospf_redist_map_nssa_update (lsa))
      ospf_redist_map_nssa_delete (lsa);

  return 0;
}

int
ospf_abr_nssa_stability_timer (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *lsa;
  struct ls_node *rn;

  area->t_nssa = NULL;

   /* Set VR context */
   LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_NSSA))
    zlog_info (ZG, "NSSA[%r]: Stability timer expire", &area->area_id);

  LSA_REFRESH_TIMER_ADD (area->top, OSPF_ROUTER_LSA,
                         ospf_router_lsa_self (area), area);

  /* XXX Flush Type-5 LSAs aggregated by Type-7 address range. */
  LSDB_LOOP (NSSA_LSDB (area), rn, lsa)
    ospf_redist_map_nssa_delete (lsa);

  return 0;
}

int
ospf_abr_nssa_translator_elect (struct ospf_area *area)
{
  struct ospf *top = area->top;
  struct ospf_master *om = top->om;
  struct ospf_route *route = NULL;
  struct ls_node *rn, *rn2;
  struct ospf_path *path;
  struct ospf_lsa *lsa;
  struct ls_prefix p;
  u_char bits;
  /* Flag for freeing the route */
  bool_t flag = PAL_FALSE;

  if (IS_DEBUG_OSPF (event, EVENT_NSSA))
    zlog_info (ZG, "NSSA[%r]: Translator election start", &area->area_id);

  LSDB_LOOP (ROUTER_LSDB (area), rn, lsa)
    if (!IS_LSA_SELF (lsa))
      {
        bits = OSPF_PNT_UCHAR_GET (lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);
        if (!(IS_ROUTER_LSA_SET (bits, BIT_B)
              && IS_ROUTER_LSA_SET (bits, BIT_E)))
          continue;

        /* Add the route in rt_asbr table if not present */
        route = ospf_route_asbr_lookup (area->top, lsa->data->adv_router);
        if (route == NULL)
          {
            route = ospf_route_new ();
            ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, lsa->data->adv_router);
            rn2 = ls_node_get (top->rt_asbr, (struct ls_prefix *)&p);
            if ((RN_INFO (rn2, RNI_CURRENT)) == NULL)
              {
                RN_INFO_SET (rn2, RNI_CURRENT, route);
                route->rn = rn2;
              }
            else
              flag = PAL_TRUE;

            ls_unlock_node (rn2);
          }  

        /* Lookup ASBR route through this NSSA. */
        path = ospf_route_path_lookup (route, OSPF_PATH_INTRA_AREA, area); 

        if (flag == PAL_TRUE)
          {
            flag = PAL_FALSE;
            ospf_route_free (route);
            route = NULL;
          }

        if (path == NULL)
          continue;

        if (IS_ROUTER_LSA_SET (bits, BIT_NT)
            || IPV4_ADDR_CMP (&top->router_id, &lsa->data->adv_router) < 0)
          {
            if (IS_DEBUG_OSPF (event, EVENT_NSSA))
              zlog_info (ZG, "NSSA[%r]: Elect ABR(%r) as Translator",
                         &area->area_id, &lsa->data->adv_router);

            ls_unlock_node (rn);
            return OSPF_NSSA_TRANSLATOR_DISABLED;
          }
      }

  if (IS_DEBUG_OSPF (event, EVENT_NSSA))
    zlog_info (ZG, "NSSA[%r]: Elect self as Translator", &area->area_id);

  return OSPF_NSSA_TRANSLATOR_ELECTED;
}

#define OSPF_NSSA_TRANSLATE_TIMER_DELAY         2

void
ospf_abr_nssa_translator_state_update (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;
  u_char state = OSPF_NSSA_TRANSLATOR_DISABLED;

  if (IS_OSPF_ABR (area->top))
    {
      switch (ospf_nssa_translator_role_get (area))
        {
        case OSPF_NSSA_TRANSLATE_NEVER:
          UNSET_FLAG (area->translator_flag, OSPF_NSSA_TRANSLATOR_UNCONDL);
          state = OSPF_NSSA_TRANSLATOR_DISABLED;
          break;
        case OSPF_NSSA_TRANSLATE_ALWAYS:
          SET_FLAG (area->translator_flag, OSPF_NSSA_TRANSLATOR_UNCONDL);
          state = OSPF_NSSA_TRANSLATOR_ENABLED;
          break;
        case OSPF_NSSA_TRANSLATE_CANDIDATE:
          UNSET_FLAG (area->translator_flag, OSPF_NSSA_TRANSLATOR_UNCONDL);
          state = ospf_abr_nssa_translator_elect (area);
          break;
        }
    }
  else
    state = OSPF_NSSA_TRANSLATOR_DISABLED;

  /* Non-NSSA-Translator to NSSA-Translator. */
  if (area->translator_state == OSPF_NSSA_TRANSLATOR_DISABLED
      && state != OSPF_NSSA_TRANSLATOR_DISABLED)
    {
      if (IS_DEBUG_OSPF (event, EVENT_NSSA))
        zlog_info (ZG, "NSSA[%r]: Change state to Translator", &area->area_id);

      /* Unset StabilityTimer. */
      OSPF_TIMER_OFF (area->t_nssa);

      /* Start translation. */
      OSPF_AREA_TIMER_ON (area->t_nssa, ospf_abr_nssa_translate_timer,
                          OSPF_NSSA_TRANSLATE_TIMER_DELAY);
    }

  /* NSSA-Translator to Non-NSSA-Translator. */
  else if (area->translator_state != OSPF_NSSA_TRANSLATOR_DISABLED
           && state == OSPF_NSSA_TRANSLATOR_DISABLED)
    {
      if (IS_DEBUG_OSPF (event, EVENT_NSSA))
        zlog_info (ZG, "NSSA[%r]: Change state to Non-Translator",
                   &area->area_id);

      /* Unset TranslateTimer. */
      OSPF_TIMER_OFF (area->t_nssa);

      /* Set TranslateStabilityTimer. */
      OSPF_AREA_TIMER_ON (area->t_nssa, ospf_abr_nssa_stability_timer,
                          area->conf.nssa_stability_interval);
    }

  /* NSSA-Translator state elected to always & vice versa */
  else if ((area->translator_state == OSPF_NSSA_TRANSLATOR_ELECTED
            && state == OSPF_NSSA_TRANSLATOR_ENABLED)
           || (area->translator_state == OSPF_NSSA_TRANSLATOR_ENABLED
               && state == OSPF_NSSA_TRANSLATOR_ELECTED))
    {
      if (IS_DEBUG_OSPF (event, EVENT_NSSA))
        zlog_info (ZG, "NSSA[%r]:Translator state updated", &area->area_id);

      LSA_REFRESH_TIMER_ADD (area->top, OSPF_ROUTER_LSA,
                             ospf_router_lsa_self (area), area);

    }

  area->translator_state = state;

  ospf_asbr_status_update (area->top);
}

void
ospf_abr_nssa_state_update_all (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        if (IS_AREA_NSSA (area))
          {
            /* Update the type-7 external LSA's P-bit.  */
            ospf_nssa_redistribute_timer_add_all_proto (area);

            /* Update the NSSA border router translator state.  */
            ospf_abr_nssa_translator_state_update (area);
          }
}
#endif /* HAVE_NSSA */

void
ospf_abr_status_update (struct ospf *top)
{
  struct ospf_master *om = top->om;
  struct ospf_area *area;
  struct ls_node *rn;
  int area_config = 0;
  int area_active = 0;
  int backbone_config = 0;
  int backbone_active = 0;
  int status = 0;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  if (!IS_PROC_ACTIVE (top))
    return;

  /* RFC3509 2 Changes to ABR Behaviour.  */

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      {
        /* Configured area:
           An area is considered configured if the router has at least
           one interface in any state assigned to that area.  */
        if (listcount (area->iflist))
          {
            area_config++;
            if (IS_AREA_BACKBONE (area))
              backbone_config++;

            /* Actively Attached area:
               An area is considered actively attached if the router has
               at least one interface in that area in the state other
               than Down.  */
            if (area->active_if_count)
              {
                area_active++;
                if (IS_AREA_BACKBONE (area))
                  backbone_active++;
              }
          }
      }

  /* Area Border Router (ABR):

     Cisco Systems Interpretation:
         A router is considered to be an ABR if it has more than one
         area Actively Attached and one of them is the backbone area.

     IBM Interpretation:
         A router is considered to be an ABR if it has more than one
         Actively Attached area and the backbone area Configured.  

         A router connected to MPLS VPN Cloud acts as an ABR even if
         it has only one area actively attached and we can consider
         the MPLS-VPN Cloud as the Superbackbone area. 
  */

  switch (top->abr_type)
    {
    case OSPF_ABR_TYPE_STANDARD:
    case OSPF_ABR_TYPE_SHORTCUT:
      if ((area_active > 1)
#ifdef HAVE_VRF_OSPF
          ||(CHECK_FLAG (top->ov->flags, OSPF_VRF_ENABLED)
            && (area_active >= 1))
#endif /* HAVE_VRF_OSPF */
         )
        SET_FLAG (status, OSPF_ROUTER_ABR);
      break;
    case OSPF_ABR_TYPE_CISCO:
      if (area_active > 1)
        {
        if (backbone_active)
          SET_FLAG (status, OSPF_ROUTER_ABR);
        }
#ifdef HAVE_VRF_OSPF
      else if (CHECK_FLAG (top->ov->flags, OSPF_VRF_ENABLED)
               && (area_active >= 1))
          SET_FLAG (status, OSPF_ROUTER_ABR);
#endif /* HAVE_VRF_OSPF */ 
        
      break;
    case OSPF_ABR_TYPE_IBM:
      if (area_active > 1)
        {
          if (backbone_config)
            SET_FLAG (status, OSPF_ROUTER_ABR);
        }
#ifdef HAVE_VRF_OSPF
      else if (CHECK_FLAG (top->ov->flags, OSPF_VRF_ENABLED)
               && (area_active >= 1))
          SET_FLAG (status, OSPF_ROUTER_ABR);
#endif /* HAVE_VRF_OSPF */
      break;
    }

  /* Non-ABR to ABR. */
  if (!IS_OSPF_ABR (top)
      && CHECK_FLAG (status, OSPF_ROUTER_ABR))
    {
      if (IS_DEBUG_OSPF (event, EVENT_ABR))
        zlog_info (ZG, "ROUTER[%d]: Change status to ABR", top->ospf_id);

      SET_FLAG (top->flags, OSPF_ROUTER_ABR);
      ospf_router_lsa_refresh_all (top);

      for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
        if ((area = RN_INFO (rn, RNI_DEFAULT)))
          if (IS_AREA_ACTIVE (area))
            {
              /* Update Inter-Area-Prefix/Router-LSAs.  */
              ospf_abr_ia_update_all_to_area (area);

              /* Cleanup IA routes.  */
              if (!IS_AREA_IA_CALCULATE (area))
                ospf_ia_calc_route_clean (area);
            }
      
#ifdef HAVE_OSPF_MULTI_AREA
      /* Enable multi area links if router becomes ABR */
      for (rn = ls_table_top (top->multi_area_link_table); rn; 
           rn = ls_route_next (rn)) 
      if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
        if (mlink->oi)
          ospf_multi_area_if_up (mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */
    }

  /* ABR to non-ABR. */
  else if (IS_OSPF_ABR (top)
           && !CHECK_FLAG (status, OSPF_ROUTER_ABR))
    {
      if (IS_DEBUG_OSPF (event, EVENT_ABR))
        zlog_info (ZG, "ROUTER[%d]: Change status to non-ABR", top->ospf_id);

      UNSET_FLAG (top->flags, OSPF_ROUTER_ABR);
      ospf_router_lsa_refresh_all (top);

      for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
        if ((area = RN_INFO (rn, RNI_DEFAULT)))
          if (IS_AREA_ACTIVE (area))
            {
              /* Flush Inter-Area-Prefix/Router-LSAs.  */
              ospf_abr_ia_update_all_to_area (area);

              /* Calculate IA routes.  */
              if (IS_AREA_IA_CALCULATE (area))
                ospf_ia_calc_event_add (area);
            }
     /* disable multi area link if router is not an ABR */
#ifdef HAVE_OSPF_MULTI_AREA
      for (rn = ls_table_top (top->multi_area_link_table); rn; 
           rn = ls_route_next (rn))
        if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
          if (mlink->oi)
            ospf_if_down (mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */
    }

#ifdef HAVE_NSSA
  /* Update the NSSA state.  */
  ospf_abr_nssa_state_update_all (top);
#endif /* HAVE_NSSA */
}


int
ospf_abr_filter_check_in (struct ospf_area *area, struct ls_prefix *lp)
{
  struct prefix *p = (struct prefix *)lp;

  /* Check access-list. */
  if (ALIST_NAME (area, FILTER_IN))
    if (access_list_apply (ALIST_LIST (area, FILTER_IN), p) != FILTER_PERMIT)
      return 0;

  /* Check prefix-list. */
  if (PLIST_NAME (area, FILTER_IN))
    if (prefix_list_apply (PLIST_LIST (area, FILTER_IN), p) != PREFIX_PERMIT)
      return 0;

  return 1;
}

int
ospf_abr_filter_check_out (struct ospf_area *area, struct ls_prefix *lp)
{
  struct prefix *p = (struct prefix *)lp;

  /* Check access-list. */
  if (ALIST_NAME (area, FILTER_OUT))
    if (access_list_apply (ALIST_LIST (area, FILTER_OUT), p) != FILTER_PERMIT)
      return 0;

  /* Check prefix-list. */
  if (PLIST_NAME (area, FILTER_OUT))
    if (prefix_list_apply (PLIST_LIST (area, FILTER_OUT), p) != PREFIX_PERMIT)
      return 0;

  return 1;
}

void
ospf_abr_ia_prefix_delete (struct ospf_area *area, struct ls_prefix *lp)
{
  struct ospf_ia_lsa_map *map;

  map = ospf_ia_lsa_map_lookup (area->ia_prefix, lp);
  if (map != NULL)
    {
      ospf_ia_lsa_map_delete (area->ia_prefix, lp);
      ospf_ia_lsa_map_free (map);
    }
}

void
ospf_abr_ia_prefix_update (struct ospf_area *area,
                           struct ls_prefix *lp, struct ospf_path *path)
{
  struct ospf_ia_lsa_map *map;

  if (!ospf_abr_filter_check_in (area, lp))
    {
      ospf_abr_ia_prefix_delete (area, lp);
      return;
    }

  map = ospf_ia_lsa_map_lookup (area->ia_prefix, lp);
  if (map == NULL)
    {
      map = ospf_ia_lsa_map_new (OSPF_IA_MAP_PATH);
      map->area = area;
      map->lp = lp;
      ospf_ia_lsa_map_add (area->ia_prefix, lp, map);
    }
  map->type = OSPF_IA_MAP_PATH;
  map->u.path = path;

  /* Originate only when metric has changed. */
  if (map->u.path != NULL)
    if (map->lsa != NULL)
      {
        u_int16_t metric =
          OSPF_PNT_UINT24_GET (map->lsa->data, OSPF_SUMMARY_LSA_METRIC_OFFSET);

        if (metric == OSPF_PATH_COST (path))
          return;
      }

  LSA_REFRESH_TIMER_ADD (area->top, OSPF_SUMMARY_LSA, map->lsa, map);
}

void
ospf_abr_ia_default_update (struct ospf_area *area)
{
  struct ospf_ia_lsa_map *map = area->stub_default;
  struct ospf *top = area->top;

  if (map == NULL)
    {
      map = ospf_ia_lsa_map_new (OSPF_IA_MAP_STUB);
      map->area = area;
      map->lp = &LsPrefixIPv4Default;
      area->stub_default = map;
    }

  LSA_REFRESH_TIMER_ADD (top, OSPF_SUMMARY_LSA, map->lsa, map);
}

void
ospf_abr_ia_default_delete (struct ospf_area *area)
{
  struct ospf_ia_lsa_map *map;

  if ((map = area->stub_default))
    {
      ospf_ia_lsa_map_free (map);
      area->stub_default = NULL;
    }
}

void
ospf_abr_ia_summary_update (struct ospf_area *area_to,
                            struct ospf_area_range *range)
{
  struct ospf *top = area_to->top;
  struct ospf_ia_lsa_map *map;

  map = ospf_ia_lsa_map_lookup (area_to->ia_prefix, range->lp);
  if (map == NULL)
    {
      map = ospf_ia_lsa_map_new (OSPF_IA_MAP_RANGE);
      map->area = area_to;
      ospf_ia_lsa_map_add (area_to->ia_prefix, range->lp, map);
    }

  map->type = OSPF_IA_MAP_RANGE;
  map->u.range = range;

  if (map->lsa)
    {
      if (!CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
        LSA_MAP_FLUSH (map->lsa);
      else
        {
          u_int32_t cost =
            OSPF_PNT_UINT24_GET (map->lsa->data,
                                 OSPF_SUMMARY_LSA_METRIC_OFFSET);

          if (cost != range->cost)
            ospf_lsa_refresh_timer_add (map->lsa);
        }
    }
  else
    {
      if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
        ospf_lsa_originate (top, OSPF_SUMMARY_LSA, map);
    }
}

void
ospf_abr_ia_prefix_flush_under_range (struct ospf_area *area,
                                      struct ospf_area_range *range)
{
  struct ospf_ia_lsa_map *map;
  struct ls_node *rn, *rn_tmp;

  rn_tmp = ls_node_get (area->ia_prefix, range->lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (map->lsa != NULL)
        if (map->type == OSPF_IA_MAP_PATH)
          if (CHECK_FLAG (map->u.path->flags, OSPF_PATH_RANGE_MATCHED))
            {
              ospf_ia_lsa_map_free (map);
              RN_INFO_UNSET (rn, RNI_DEFAULT);
            }

  ls_unlock_node (rn_tmp);
}

void
ospf_abr_ia_summary_update_all (struct ospf_area *area,
                                struct ospf_area_range *range)
{
  struct ospf *top = area->top;
  struct ospf_area *area_to;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area_to = RN_INFO (rn, RNI_DEFAULT)))
      if (area_to != area)
        if (!(IS_AREA_BACKBONE (area) && IS_AREA_TRANSIT (area_to)))
          {
            ospf_abr_ia_summary_update (area_to, range);

            /* Flush all Inter-Area-Prefix-LSA under this range.  */
            ospf_abr_ia_prefix_flush_under_range (area_to, range);
          }
}

void
ospf_abr_ia_summary_delete_all (struct ospf_area *area,
                                struct ospf_area_range *range)
{
  struct ospf *top = area->top;
  struct ospf_area *area_to;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area_to = RN_INFO (rn, RNI_DEFAULT)))
      if (area_to != area)
        ospf_abr_ia_prefix_delete (area_to, range->lp);
}

/* Update Inter-Area-Prefix-LSA from backbone to the transit Area.  */
void
ospf_abr_ia_prefix_update_to_transit (struct ospf_area *area_to,
                                      struct ls_prefix *lp,
                                      struct ospf_path *path)
{
  struct ospf_master *om = area_to->top->om;
  struct ospf_nexthop *nh;
  int i;

  /* RFC2328 12.4.3.  Summary-LSAs

     o   Else, if the next hops associated with this set of paths
         belong to Area A itself, do not generate a summary-LSA
         for the route.[18] This is the logical equivalent of a
         Distance Vector protocol's split horizon logic. 

    [18]This clause is only invoked when a non-backbone Area A supports
    transit data traffic (i.e., has TransitCapability set to TRUE).  For
    example, in the area configuration of Figure 6, Area 2 can support
    transit traffic due to the configured virtual link between Routers
    RT10 and RT11. As a result, Router RT11 need only originate a single
    summary-LSA into Area 2 (having the collapsed destination N9-
    N11,H1), since all of Router RT11's other eligible routes have next
    hops belonging to Area 2 itself (and as such only need be advertised
    by other area border routers; in this case, Routers RT10 and RT7).  */

  for (i = 0; i < vector_max (OSPF_PATH_NEXTHOPS (path)); i++)
    if ((nh = vector_slot (OSPF_PATH_NEXTHOPS (path), i)) != NULL)
      if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
        if (IS_OSPF_NEXTHOP_IN_AREA (nh, area_to))
          {
            if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
              zlog_info (ZG, "Route[IA:%r]: Ignore %r/%d "
                         "which nexthop exists in the transit area %r",
                         &path->area_id,
                         (struct pal_in4_addr *)lp->prefix, lp->prefixlen,
                         &area_to->area_id);

            ospf_abr_ia_prefix_delete (area_to, lp);
            return;
          }

  ospf_abr_ia_prefix_update (area_to, lp, path);
}

/* Update Inter-Area-Prefix-LSA from origin Area to the other Areas. */
void
ospf_abr_ia_prefix_update_all (struct ospf_area *area,
                               struct ls_prefix *lp, struct ospf_path *path)
{
  struct ospf *top = area->top;
  struct ospf_area *area_to;
  struct ls_node *rn;

  if (!IS_AREA_IA_ORIGINATE (area, path))
    return;

  if (!ospf_abr_filter_check_out (area, lp))
    {
      ospf_abr_ia_prefix_delete_all (area, lp, path);
      return;
    }

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area_to = RN_INFO (rn, RNI_DEFAULT)))
      if (area_to != area)
        if (IS_AREA_ACTIVE (area_to))
          if (!OSPF_AREA_CONF_CHECK (area_to, NO_SUMMARY))
            {
              if (IS_AREA_BACKBONE (area) && IS_AREA_TRANSIT (area_to))
                ospf_abr_ia_prefix_update_to_transit (area_to, lp, path);
              else if (!CHECK_FLAG (path->flags, OSPF_PATH_RANGE_MATCHED))
                ospf_abr_ia_prefix_update (area_to, lp, path);
            }
}

void
ospf_abr_ia_prefix_delete_all (struct ospf_area *area,
                               struct ls_prefix *lp, struct ospf_path *path)
{
  struct ospf *top = area->top;
  struct ospf_area *area_to;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area_to = RN_INFO (rn, RNI_DEFAULT)))
      if (area_to != area)
        ospf_abr_ia_prefix_delete (area_to, lp);
}

void
ospf_abr_ia_prefix_update_all_to_area (struct ospf_area *area_to)
{
  struct ospf *top = area_to->top;
  struct ospf_area *area;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ospf_area_range *range;
  struct ls_node *rn;

  if (!IS_AREA_ACTIVE (area_to))
    return;

  /* Update network route. */
  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      if ((path = route->selected) != NULL)
        if ((area = ospf_area_entry_lookup (top, path->area_id)))
          if (area != area_to)
            if (IS_AREA_IA_ORIGINATE (area, path))
              {
                /* Delete the filtered prefix.  */
                if (!ospf_abr_filter_check_out (area, rn->p))
                  ospf_abr_ia_prefix_delete (area_to, rn->p);

                /* Announce prefix to the transit area.  */
                else if (IS_AREA_BACKBONE (area) && IS_AREA_TRANSIT (area_to))
                  ospf_abr_ia_prefix_update_to_transit (area_to, rn->p, path);

                /* Announce summary.  */
                else if (CHECK_FLAG (path->flags, OSPF_PATH_RANGE_MATCHED))
                  {
                    range = ospf_area_range_match (area, rn->p);
                    if (range)
                      ospf_abr_ia_summary_update (area_to, range);
                  }

                /* Announce prefix.  */
                else
                  ospf_abr_ia_prefix_update (area_to, rn->p, path);
              }
}

void
ospf_abr_ia_prefix_update_area_to_all (struct ospf_area *area_from)
{
  struct ospf *top = area_from->top;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ls_node *rn;

  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      if ((path = route->selected) != NULL)
        if (path->area_id.s_addr == area_from->area_id.s_addr)
          ospf_abr_ia_prefix_update_all (area_from, rn->p, path);
}

void
ospf_abr_ia_prefix_delete_all_from_area (struct ospf_area *area_from)
{
  struct ospf_ia_lsa_map *map;
  struct ls_node *rn;

  for (rn = ls_table_top (area_from->ia_prefix); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)) != NULL)
      {
        ospf_ia_lsa_map_delete (area_from->ia_prefix, rn->p);
        ospf_ia_lsa_map_free (map);
      }
}

/* Update Inter-Area-Prefix-LSA from backbone to the specified Area.  */
void
ospf_abr_ia_prefix_update_from_backbone_to_area (struct ospf_area *area_to)
{
  struct ospf *top = area_to->top;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ospf_area_range *range;
  struct ls_node *rn;

  /* Update Inter-Area-Prefix-LSA to the specific area.  */
  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      if ((path = route->selected) != NULL)
        if (IS_AREA_ID_BACKBONE (path->area_id)
            && (path->type == OSPF_PATH_CONNECTED
                || path->type == OSPF_PATH_INTRA_AREA))
          if (CHECK_FLAG (path->flags, OSPF_PATH_RANGE_MATCHED))
            {
              range = ospf_area_range_match (top->backbone, rn->p);
              if (range != NULL)
                {
                  /* Update summary LSA based on the transit capability.  */
                  if (IS_AREA_TRANSIT (area_to))
                    {
                      ospf_abr_ia_prefix_update_to_transit (area_to,
                                                            rn->p, path);
                      ospf_abr_ia_prefix_delete (area_to, range->lp);
                    }
                  else
                    {
                      ospf_abr_ia_summary_update (area_to, range);
                      ospf_abr_ia_prefix_delete (area_to, rn->p);
                    }
                }
            }
}


/* Update Inter-Area-Router-LSA to the specific Area. */
void
ospf_abr_ia_router_update (struct ospf_area *area,
                           struct ls_prefix *lp, struct ospf_path *path)
{
  struct ospf_ia_lsa_map *map;

  map = ospf_ia_lsa_map_lookup (area->ia_router, lp);
  if (map == NULL)
    {
      map = ospf_ia_lsa_map_new (OSPF_IA_MAP_PATH);
      map->area = area;
      map->lp = lp;
      ospf_ia_lsa_map_add (area->ia_router, lp, map);
    }
  map->u.path = path;

  LSA_REFRESH_TIMER_ADD (area->top, OSPF_SUMMARY_LSA_ASBR, map->lsa, map);
}

/* Delete Inter-Area-Router-LSA from the specific Area. */
void
ospf_abr_ia_router_delete (struct ospf_area *area, struct ls_prefix *lp)
{
  struct ospf_ia_lsa_map *map;

  map = ospf_ia_lsa_map_lookup (area->ia_router, lp);
  if (map != NULL)
    {
      ospf_ia_lsa_map_delete (area->ia_router, lp);
      ospf_ia_lsa_map_free (map);
    }
}

/* Update Inter-Area-Router-LSA from backbone to the transit Area.  */
void
ospf_abr_ia_router_update_to_transit (struct ospf_area *area_to,
                                      struct ls_prefix *lp,
                                      struct ospf_path *path)
{
  struct ospf_master *om = area_to->top->om;
  struct ospf_nexthop *nh;
  int i;

  /* RFC2328 12.4.3.  Summary-LSAs

     o   Else, if the next hops associated with this set of paths
         belong to Area A itself, do not generate a summary-LSA
         for the route.[18] This is the logical equivalent of a
         Distance Vector protocol's split horizon logic. 

    [18]This clause is only invoked when a non-backbone Area A supports
    transit data traffic (i.e., has TransitCapability set to TRUE).  For
    example, in the area configuration of Figure 6, Area 2 can support
    transit traffic due to the configured virtual link between Routers
    RT10 and RT11. As a result, Router RT11 need only originate a single
    summary-LSA into Area 2 (having the collapsed destination N9-
    N11,H1), since all of Router RT11's other eligible routes have next
    hops belonging to Area 2 itself (and as such only need be advertised
    by other area border routers; in this case, Routers RT10 and RT7).  */

  for (i = 0; i < vector_max (OSPF_PATH_NEXTHOPS (path)); i++)
    if ((nh = vector_slot (OSPF_PATH_NEXTHOPS (path), i)) != NULL)
      if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
        if (IS_OSPF_NEXTHOP_IN_AREA (nh, area_to))
          {
            if (IS_DEBUG_OSPF (rt_calc, RT_CALC_IA))
              zlog_info (ZG, "Route[IA:%r]: Ignore %r "
                         "which nexthop exists in the transit area %r",
                         &path->area_id,
                         (struct pal_in4_addr *)lp->prefix,
                         &area_to->area_id);

            ospf_abr_ia_router_delete (area_to, lp);
            return;
          }

  ospf_abr_ia_router_update (area_to, lp, path);
}

void
ospf_abr_ia_router_update_all (struct ospf_area *area,
                               struct ls_prefix *lp, struct ospf_path *path)
{
  struct ospf *top = area->top;
  struct ospf_area *area_to;
  struct ls_node *rn;

  if (!IS_AREA_IA_ORIGINATE (area, path))
    return;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area_to = RN_INFO (rn, RNI_DEFAULT)))
      if (area_to != area)
        if (IS_AREA_ACTIVE (area_to))
          if (IS_AREA_DEFAULT (area_to))
            {
              if (IS_AREA_BACKBONE (area) && IS_AREA_TRANSIT (area_to))
                ospf_abr_ia_router_update_to_transit (area_to, lp, path);

              else if (!IS_AREA_NSSA (area))
              /* RFC 3101: NSSA border routers
                 should not originate Type-4 summary-LSAs.*/
                ospf_abr_ia_router_update (area_to, lp, path);
            }
}

void
ospf_abr_ia_router_delete_all (struct ospf_area *area,
                               struct ls_prefix *lp, struct ospf_path *path)
{
  struct ospf *top = area->top;
  struct ospf_area *area_to;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area_to = RN_INFO (rn, RNI_DEFAULT)))
      if (area != area_to)
        ospf_abr_ia_router_delete (area_to, lp);
}

void
ospf_abr_ia_router_update_all_to_area (struct ospf_area *area_to)
{
  struct ospf *top = area_to->top;
  struct ospf_area *area;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ls_node *rn;

  if (!IS_AREA_ACTIVE (area_to))
    return;

  for (rn = ls_table_top (top->rt_asbr); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      if ((path = route->selected) != NULL)
        if ((area = ospf_area_entry_lookup (top, path->area_id)))
          if (area != area_to)
            if (IS_AREA_IA_ORIGINATE (area, path))
              {
                if (IS_AREA_BACKBONE (area) && IS_AREA_TRANSIT (area_to))
                  ospf_abr_ia_router_update_to_transit (area_to, rn->p, path);
                else
                  ospf_abr_ia_router_update (area_to, rn->p, path);
              }
}

void
ospf_abr_ia_router_delete_all_from_area (struct ospf_area *area_from)
{
  struct ospf_ia_lsa_map *map;
  struct ls_node *rn;

  for (rn = ls_table_top (area_from->ia_router); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)) != NULL)
      {
        ospf_ia_lsa_map_delete (area_from->ia_router, rn->p);
        ospf_ia_lsa_map_free (map);
      }
}

/* Update Inter-Area-Router-LSA from backbone to the specified Area.  */
void
ospf_abr_ia_router_update_from_backbone_to_area (struct ospf_area *area_to)
{
  struct ospf *top = area_to->top;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ls_node *rn;

  /* Update Inter-Area-Router-LSA to the specific area.  */
  for (rn = ls_table_top (top->rt_asbr); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)) != NULL)
      if ((path = route->selected) != NULL)
        if (IS_AREA_ID_BACKBONE (path->area_id))
          {
            if (IS_AREA_TRANSIT (area_to))
              ospf_abr_ia_router_update_to_transit (area_to, rn->p, path);
            else
              ospf_abr_ia_router_update (area_to, rn->p, path);
          }
}


void
ospf_abr_ia_update_default_to_area (struct ospf_area *area_to)
{
  if (IS_OSPF_ABR (area_to->top)
      && (IS_AREA_STUB (area_to)
          || (IS_AREA_NSSA (area_to)
              && OSPF_AREA_CONF_CHECK (area_to, NO_SUMMARY))))
    ospf_abr_ia_default_update (area_to);
  else
    ospf_abr_ia_default_delete (area_to);
}

void
ospf_abr_ia_update_all_to_area (struct ospf_area *area_to)
{
  /* Update Inter-Area-Preifx-LSA to the specified Area.  */
  if (IS_OSPF_ABR (area_to->top)
      && !OSPF_AREA_CONF_CHECK (area_to, NO_SUMMARY))
    ospf_abr_ia_prefix_update_all_to_area (area_to);
  else
    ospf_abr_ia_prefix_delete_all_from_area (area_to);

  /* Update Inter-Area-Router-LSA to the specified Area.  */
  if (IS_OSPF_ABR (area_to->top) && IS_AREA_DEFAULT (area_to))
    ospf_abr_ia_router_update_all_to_area (area_to);
  else
    ospf_abr_ia_router_delete_all_from_area (area_to);

  /* Update default Inter-Area-Prefix-LSA to the specified Area.  */
  ospf_abr_ia_update_default_to_area (area_to);
}

void
ospf_abr_ia_update_area_to_all (struct ospf_area *area_from)
{
  /* Update Inter-Area-Preifx-LSA from the specified Area.  */
  if (IS_OSPF_ABR (area_from->top))
    ospf_abr_ia_prefix_update_area_to_all (area_from);
}

void
ospf_abr_ia_update_from_backbone_to_area (struct ospf_area *area_to)
{
  if (area_to->top->backbone == NULL
      || area_to == area_to->top->backbone)
    return;

  /* Update Inter-Area-Prefix-LSA from backbone to the specified Area.  */
  if (IS_OSPF_ABR (area_to->top)
      && !OSPF_AREA_CONF_CHECK (area_to, NO_SUMMARY))
    ospf_abr_ia_prefix_update_from_backbone_to_area (area_to);
  else
    ospf_abr_ia_prefix_delete_all_from_area (area_to);

  /* Update Inter-Area-Router-LSA from backbone to the specified Area.  */
  if (IS_OSPF_ABR (area_to->top) && IS_AREA_DEFAULT (area_to))
    ospf_abr_ia_router_update_from_backbone_to_area (area_to);
  else
    ospf_abr_ia_router_delete_all_from_area (area_to);
}


/* OSPF area range.  */
struct ospf_area_range *
ospf_area_range_new (struct ospf_area *area)
{
  struct ospf_area_range *range;

  range = XMALLOC (MTYPE_OSPF_AREA_RANGE, sizeof (struct ospf_area_range));
  pal_mem_set (range, 0, sizeof (struct ospf_area_range));
  range->area = ospf_area_lock (area);
  range->status = ROW_STATUS_ACTIVE;

  return range;
}

void
ospf_area_range_free (struct ospf_area_range *range)
{
  if (range->subst)
    ls_prefix_free (range->subst);

  OSPF_TIMER_OFF (range->t_update);

  XFREE (MTYPE_OSPF_AREA_RANGE, range);
}

struct ospf_area_range *
ospf_area_range_lookup (struct ospf_area *area, struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_lookup (area->ranges, lp);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_area_range *
ospf_area_range_match (struct ospf_area *area, struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_match (area->ranges, lp);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_area_range *
ospf_area_range_active (struct ospf_area *area, struct ls_prefix *lp)
{
  struct ospf_area_range *range;
  struct ls_node *rn;

  rn = ls_node_lookup (area->ranges, lp);
  if (rn)
    {
      ls_unlock_node (rn);
      range = RN_INFO (rn, RNI_DEFAULT);
      if (range != NULL)
        if (range->matched)
          return range;
    }
  return NULL;
}

int
ospf_area_range_active_match (struct ospf *top, struct ls_prefix *lp)
{
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (ospf_area_range_active (area, lp))
        {
          ls_unlock_node (rn);
          return 1;
        }
  return 0;
}

void
ospf_area_range_check_update (struct ospf_area *area,
                              struct ls_prefix *lp, struct ospf_path *path)
{
  struct ospf_area_range *range;

  if (path->type == OSPF_PATH_CONNECTED
      || path->type == OSPF_PATH_INTRA_AREA)
    if ((range = ospf_area_range_match (area, lp)))
      {
        SET_FLAG (path->flags, OSPF_PATH_RANGE_MATCHED);
        OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 0);
      }
}

void
ospf_abr_area_range_withdraw (struct ospf_area *area,
                              struct ospf_area_range *range)
{
  struct ospf *top = area->top;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ospf_area_range *range_alt;
  struct ls_node *rn, *rn_tmp;
  int exact = 0;

  if (range->matched)
    {
      rn_tmp = ls_node_get (top->rt_network, range->lp);

      for (rn = ls_lock_node (rn_tmp);
           rn; rn = ls_route_next_until (rn, rn_tmp))
        if ((route = RN_INFO (rn, RNI_CURRENT)))
          if ((path = route->selected) != NULL)
            if (path->type == OSPF_PATH_CONNECTED
                || path->type == OSPF_PATH_INTRA_AREA)
              if (path->area_id.s_addr == area->area_id.s_addr)
                {
                  range_alt = ospf_area_range_match (area, rn->p);
                  if (range_alt)
                    OSPF_TIMER_ON (range_alt->t_update, ospf_area_range_timer,
                                   range_alt, 0);
                  else
                    {
                      UNSET_FLAG (path->flags, OSPF_PATH_RANGE_MATCHED);
                      ospf_abr_ia_prefix_update_all (area, rn->p, path);

                      if (range->lp->prefixlen == rn->p->prefixlen)
                        if (IPV4_ADDR_SAME (range->lp->prefix, rn->p->prefix))
                          exact = 1;
                    }
                }

      ls_unlock_node (rn_tmp);

      /* Withdraw discard route and summary from the other areas.  */
      ospf_route_delete_discard (area->top, area, range->lp);
      if (!exact)
        ospf_abr_ia_summary_delete_all (area, range);
    }
}

int
ospf_area_range_entry_insert (struct ospf_area *area,
                              struct ospf_area_range *range)
{
  struct ospf_area_range *find;
  struct ls_node *rn;
  struct ls_prefix8 p;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_AREA_RANGE_TABLE_DEPTH;
  p.prefixlen = OSPF_AREA_RANGE_TABLE_DEPTH * 8;

  /* Set Net and Mask as index.  */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_AREA_RANGE_TABLE].vars,
                      &area->area_id, range->lp->prefix);

  /* Address Range Table is used for SNMP MIB purpose. And the entry 
     is only identify with Area ID and Range net prefix. Due to this 
     limitation, for same Range net prefix but different subnetmask, 
     only install one time.  */
  rn = ls_node_get (area->top->area_range_table, (struct ls_prefix *)&p);

  if ((find = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    RN_INFO_SET (rn, RNI_DEFAULT, range);

  ls_unlock_node (rn);
  return OSPF_API_ENTRY_INSERT_SUCCESS;
}

int
ospf_area_range_entry_delete (struct ospf_area *area,
                              struct ospf_area_range *range)
{
  struct ospf_area_range *find;
  struct ls_node *rn;
  struct ls_prefix8 p;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_AREA_RANGE_TABLE_DEPTH;
  p.prefixlen = OSPF_AREA_RANGE_TABLE_DEPTH * 8;

  /* Set Net and Mask as index.  */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_AREA_RANGE_TABLE].vars,
                      &area->area_id, range->lp->prefix);
  rn = ls_node_lookup (area->top->area_range_table, (struct ls_prefix *)&p);
  if (rn)
    {
      if ((find = RN_INFO (rn, RNI_DEFAULT)))
        {
          if (find == range)    
            RN_INFO_UNSET (rn, RNI_DEFAULT);
        }

      ls_unlock_node (rn);
      return OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  return OSPF_API_ENTRY_INSERT_ERROR;
}

void
ospf_area_range_entry_clean_inactive (struct ospf_area *area)
{
  struct ls_node *rn, *rn_tmp;
  struct ospf_area_range *range;
  struct ls_prefix8 p;

  p.prefixlen = 32;
  p.prefixsize = OSPF_AREA_RANGE_TABLE_DEPTH;
  pal_mem_cpy (&p.prefix, &area->area_id, sizeof (struct pal_in4_addr));

  rn_tmp = ls_node_get (area->top->area_range_table, (struct ls_prefix *)&p);
  ls_lock_node (rn_tmp);

  for (rn = rn_tmp; rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((range = RN_INFO (rn, RNI_DEFAULT)))
      if (range->status != ROW_STATUS_ACTIVE)
        {
          ospf_area_range_free (range);
          RN_INFO_UNSET (rn, RNI_DEFAULT);
        }
  ls_unlock_node (rn_tmp);
}

int
ospf_area_aggregate_entry_insert (struct ospf_area *area,
                                  struct ospf_area_range *range)
{
  struct pal_in4_addr mask;
  struct ls_prefix8 p;
  struct ls_node *rn;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_AREA_AGGREGATE_LOWER_TABLE_DEPTH;
  p.prefixlen = OSPF_AREA_AGGREGATE_LOWER_TABLE_DEPTH * 8;

  masklen2ip (range->lp->prefixlen, &mask);

  /* Set Net and Mask as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_AREA_AGGREGATE_LOWER_TABLE].vars,
                      range->lp->prefix, &mask);
  rn = ls_node_get (area->aggregate_table, (struct ls_prefix *)&p);
  if (RN_INFO (rn, RNI_DEFAULT))
    {
      ls_unlock_node (rn);
      return OSPF_API_ENTRY_INSERT_ERROR;
    }

  RN_INFO_SET (rn, RNI_DEFAULT, range);
  ls_unlock_node (rn);

  return OSPF_API_ENTRY_INSERT_SUCCESS;
}

int
ospf_area_aggregate_entry_delete (struct ospf_area *area,
                                  struct ospf_area_range *range)
{
  struct pal_in4_addr mask;
  struct ls_prefix8 p;
  struct ls_node *rn;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_AREA_AGGREGATE_LOWER_TABLE_DEPTH;
  p.prefixlen = OSPF_AREA_AGGREGATE_LOWER_TABLE_DEPTH * 8;

  masklen2ip (range->lp->prefixlen, &mask);

  /* Set Net and Mask as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_AREA_AGGREGATE_LOWER_TABLE].vars,
                      range->lp->prefix, &mask);
  rn = ls_node_lookup (area->aggregate_table, (struct ls_prefix *)&p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      return OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  return OSPF_API_ENTRY_INSERT_ERROR;
}

void
ospf_area_range_add (struct ospf_area *area,
                     struct ospf_area_range *range, struct ls_prefix *lp)
{
  struct ospf_area_range *range_alt;
  struct ls_node *rn;

  range_alt = ospf_area_range_match (area, lp);
  if (range_alt)
    if (range_alt != range)
      OSPF_TIMER_ON (range_alt->t_update, ospf_area_range_timer, range_alt, 1);

  rn = ls_node_get (area->ranges, lp);
  if (RN_INFO (rn, RNI_DEFAULT) == NULL)
    {
      range->lp = rn->p;
      ospf_area_aggregate_entry_insert (area, range);
      ospf_area_range_entry_insert (area, range);

      RN_INFO_SET (rn, RNI_DEFAULT, range);

      OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);
    }

  ls_unlock_node (rn);
}

void
ospf_area_range_delete (struct ospf_area *area, struct ospf_area_range *range)
{
  struct ls_node *rn;

  rn = ls_node_lookup (area->ranges, range->lp);
  if (rn)
    {
      ospf_area_aggregate_entry_delete (area, range);
      ospf_area_range_entry_delete (area, range);

      RN_INFO_UNSET (rn, RNI_DEFAULT);

      ospf_abr_area_range_withdraw (area, range);
      
      ls_unlock_node (rn);

      ospf_area_range_free (range);
      ospf_area_unlock (area);
    }
}

void
ospf_area_range_delete_all (struct ospf_area *area)
{
  struct ospf_area_range *range;
  struct ls_node *rn;

  for (rn = ls_table_top (area->ranges); rn; rn = ls_route_next (rn))
    if ((range = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_area_aggregate_entry_delete (area, range);
        ospf_area_range_entry_delete (area, range);
        ospf_area_range_free (range);

        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }

  ls_table_finish (area->ranges);
  ls_table_finish (area->aggregate_table);

  /* Garbage collection.  */
  ospf_area_range_entry_clean_inactive (area);
}

/* Area range path cost

   RFC1583 3.5.  IP subnetting support

     The cost of this route is the minimum of the set of
     costs to the component subnets.

   RFC2328 3.5.  IP subnetting support

     The cost of this route is the maximum of the set of
     costs to the component subnets.  */

#define IS_OSPF_AREA_RANGE_PATH_COST(T, R, P)                                 \
    (IS_RFC1583_COMPATIBLE (T)                                                \
     ? (R)->cost > OSPF_PATH_COST (P) : (R)->cost < OSPF_PATH_COST (P))

void
ospf_area_range_update (struct ospf_area_range *range)
{
  struct ospf *top = range->area->top;
  struct ospf_area *area = range->area;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ospf_area_range *range_alt;
  struct ls_node *rn, *rn_tmp;

  range->matched = 0;

  if (IS_RFC1583_COMPATIBLE (top))
    range->cost = OSPF_LS_INFINITY;
  else
    range->cost = 0;

  rn_tmp = ls_node_get (top->rt_network, range->lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      if ((path = route->selected) != NULL)
        if (path->type == OSPF_PATH_CONNECTED
            || path->type == OSPF_PATH_INTRA_AREA)
          if (path->area_id.s_addr == area->area_id.s_addr)
            {
              range_alt = ospf_area_range_match (area, rn->p);
              if (range_alt == range)
                {
                  range->matched++;
                  SET_FLAG (path->flags, OSPF_PATH_RANGE_MATCHED);
                  if (IS_OSPF_AREA_RANGE_PATH_COST (top, range, path))
                    range->cost = OSPF_PATH_COST (path);
                }
            }

  ls_unlock_node (rn_tmp);

  /* Announce summary to other areas. */
  if (range->matched)
    {
      ospf_abr_ia_summary_update_all (area, range);
      ospf_route_add_discard (area->top, area, range->lp);
    }
  /* Withdraw summary from other areas. */
  else
    {
      ospf_abr_ia_summary_delete_all (area, range);
      ospf_route_delete_discard (area->top, area, range->lp);
    }
}

int
ospf_area_range_timer (struct thread *thread)
{
  struct ospf_area_range *range = THREAD_ARG (thread);
  struct ospf_area *area = range->area;
  struct ospf_master *om = area->top->om;
 
  range->t_update = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  ospf_area_range_update (range);

  return 0;
}
