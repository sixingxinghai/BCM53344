/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "linklist.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_MPLS
#include "ospfd/ospf_mpls.h"
#endif /* HAVE_MPLS */


int
ospf_ase_calc_route_nexthop_self (struct ospf *top,
                                  struct pal_in4_addr *nexthop)
{
  struct ospf_interface *oi;
  struct ls_node *rn;

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if (if_is_up (oi->u.ifp))
        if (IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, nexthop))
          return 1;

  return 0;
}

/* Calculating AS external routes -- RFC2328 secition 16.4. */
struct ospf_path *
ospf_ase_calculate_route (struct ospf *top,
                          struct ospf_route *route, struct ospf_lsa *lsa)
{
  struct ospf_master *om = top->om;
#ifdef HAVE_NSSA
  struct ospf_area *area = lsa->lsdb->u.area;
#endif /* HAVE_NSSA */
  struct ospf_route *route_base;
  struct ospf_path *old, *new, *path_base;
  struct ospf_nexthop *nh, *nh_new;
  struct pal_in4_addr *nexthop;
  u_int32_t metric;
  u_char e_bit;
  int ret = 1;
  int i;

  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_EXTERNAL_LSA_METRIC_OFFSET);

  route_base = ospf_route_asbr_lookup (top, lsa->data->adv_router);
  if (route_base == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
        zlog_info (ZG, "Route[ASE]: %P no route to ASBR(%r)",
                   route->rn->p, &lsa->data->adv_router);
      return NULL;
    }

#ifdef HAVE_NSSA
  if (lsa->data->type == OSPF_AS_NSSA_LSA)
    path_base = ospf_route_path_lookup (route_base,
                                        OSPF_PATH_INTRA_AREA, area);
  else
#endif /* HAVE_NSSA */
    path_base = ospf_route_path_lookup_from_default (route_base, top);

  if (path_base == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
        zlog_info (ZG, "Route[ASE]: %P no path to ASBR(%r)",
                   route->rn->p, &lsa->data->adv_router);

      return NULL;
    }

  nexthop = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET);
  if (nexthop->s_addr != 0)
    {
      if (ospf_ase_calc_route_nexthop_self (top, nexthop))
        {
          if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
            zlog_info (ZG, "Route[ASE]: %P forwarding address(%r) is this"
                       "router's address", route->rn->p, nexthop);
          return NULL;
        }

      /* RFC3101 section 2.5-(3) paragraph 6. */
      route_base = ospf_route_match_internal (top, nexthop);
      if (route_base == NULL)
        {
          if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
            zlog_info (ZG, "Route[ASE]: %P Can't find route to forwarding "
                       "address(%r)", route->rn->p, nexthop);
          return NULL;
        }
      path_base = route_base->selected;

#ifdef HAVE_NSSA
      if (lsa->data->type == OSPF_AS_NSSA_LSA)
        if (path_base->type == OSPF_PATH_INTER_AREA
            || !IPV4_ADDR_SAME (&path_base->area_id, &area->area_id))
          {
            if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
              zlog_info (ZG, "Route[ASE]: %P forwarding address(%r) does not"
                         "belong to NSSA(%r)", nexthop, &area->area_id);
            return NULL;
          }
#endif /* HAVE_NSSA */
    }

  new = ospf_path_new (OSPF_PATH_EXTERNAL);
  new->area_id = path_base->area_id;
  new->path_base = ospf_path_lock (path_base);

  /* Type-1. */
  e_bit = OSPF_EXTERNAL_LSA_E_BIT (lsa->data);
  if (!IS_EXTERNAL_METRIC (e_bit))
    new->cost = OSPF_PATH_COST (path_base) + metric;
  /* Type-2. */
  else
    {
      new->cost = OSPF_PATH_COST (path_base);
      SET_FLAG (new->flags, OSPF_PATH_TYPE2);
    }
  new->type2_cost = metric;

  old = ospf_route_path_lookup_new (route, OSPF_PATH_EXTERNAL, NULL);
  if (old != NULL)
    ret = ospf_path_cmp (top, old, new);

  /* Old path is better.  */
  if (ret < 0)
    {
      ospf_path_unlock (new);
      new = old;
    }
  else
    {
      /* Pathes are equal.  */
      if (ret == 0)
        {
          ospf_path_unlock (new);
          new = old;
        }

      /* New path is better.  */
      else
        if (old != NULL)
          {
            /* Swap LSA vector.  */
            ospf_path_swap (new, old);
            ospf_path_unlock (old);
          }

      /* Copy nexthops from ASBR/forwarding path.  */
      for (i = 0; i < vector_max (OSPF_PATH_NEXTHOPS (path_base)); i++)
        if ((nh = vector_slot (OSPF_PATH_NEXTHOPS (path_base), i)))
          if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
            {
              /* Complete the direct nexthop.  */
              if (nexthop->s_addr != 0
                  && CHECK_FLAG (nh->flags, OSPF_NEXTHOP_CONNECTED))
                nh_new = ospf_nexthop_get (nh->oi->area, nh->oi, *nexthop);
              else
                nh_new = ospf_nexthop_lock (nh);

              ospf_path_nexthop_add (new, nh_new);
              ospf_nexthop_unlock (nh_new);
            }
    }

  ospf_route_path_set_new (route, new);
#ifdef HAVE_MPLS 
  if (new->path_base->t_lsp && ! new->t_lsp)
    {
      new->t_lsp = ospf_igp_shortcut_lsp_lock (new->path_base->t_lsp);
      new->t_cost = new->path_base->t_cost + metric;
    } 
#endif /* HAVE_MPLS */

  return new;
}

int
ospf_ase_calc_route_check (struct ospf *top,
                           struct ospf_lsa *lsa, struct ls_prefix *lp)
{
  struct ospf_master *om = top->om;
  u_int32_t metric;

  metric = OSPF_PNT_UINT24_GET (lsa->data, OSPF_EXTERNAL_LSA_METRIC_OFFSET);
  if (metric >= OSPF_LS_INFINITY)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
        zlog_info (ZG, "Route[ASE]: %P metric is infinity", lp);

      return 0;
    }

  if (IS_LSA_MAXAGE (lsa))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
        zlog_info (ZG, "Route[ASE]: %P LSA is MaxAge", lp);

      return 0;
    }

  if (IS_LSA_SELF (lsa))
    return 0;

  if (CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
    return 0;

#ifdef HAVE_NSSA
  /* RFC3101 Section 2.5-(3) 2nd paragraph, p11-p12. */
  if (lsa->data->type == OSPF_AS_NSSA_LSA)
    if (lp->prefixlen == 0)
      if (IS_OSPF_ABR (top))
        {
          if (!CHECK_FLAG (lsa->data->options, OSPF_OPTION_NP))
            return 0;

          if (OSPF_AREA_CONF_CHECK (lsa->lsdb->u.area, NO_SUMMARY))
            return 0;
        }
#endif /* HAVE_NSSA */

  return 1;
}

void
ospf_ase_calc_incremental (struct ospf *top, struct ospf_lsa *new)
{
  struct ospf_route *route;
  struct ospf_path *path_new = NULL;
  struct ospf_path *path;
  struct ospf_lsa *lsa;
  struct ls_prefix lp;
  struct pal_in4_addr *mask;
  u_int32_t tag = 0;
  int i;

  if (top->rt_network->top == NULL)
    return;

  mask = OSPF_PNT_IN_ADDR_GET (new->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  ls_prefix_ipv4_set (&lp, ip_masklen (*mask), new->data->id);

  /* Check the existing LSAs. */
  route = ospf_route_lookup (top, &new->data->id, lp.prefixlen);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_add (top, &new->data->id, lp.prefixlen, route);
    }
  else
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_EXTERNAL, NULL);
      if (path != NULL)
        for (i = 0; i < vector_max (path->lsa_vec); i++)
          if ((lsa = vector_slot (path->lsa_vec, i)))
            {
              vector_slot (path->lsa_vec, i) = NULL;

              if (lsa->data->type != new->data->type
                  || !IPV4_ADDR_SAME (&lsa->data->adv_router,
                                      &new->data->adv_router))
                if (ospf_ase_calc_route_check (top, lsa, route->rn->p))
                  if ((path_new = ospf_ase_calculate_route (top, route, lsa)))
                    vector_set (path_new->lsa_vec, ospf_lsa_lock (lsa));
              
              ospf_lsa_unlock (lsa);
          }
    }

  /* Check the updated LSA. */
  if (ospf_ase_calc_route_check (top, new, route->rn->p))
    if ((path_new = ospf_ase_calculate_route (top, route, new)))
      vector_set (path_new->lsa_vec, ospf_lsa_lock (new));

  /* Get the tag value from the lsa data */
  tag = OSPF_PNT_UINT32_GET (new->data,
                             OSPF_EXTERNAL_LSA_TAG_OFFSET);
  /* Update the tag value to the path so that it can be sent to NSM.*/
  if (path_new && tag)
    path_new->tag = tag;

  /* Install route to the table. */
  ospf_route_install (top, NULL, OSPF_PATH_EXTERNAL, route->rn);
}

int
ospf_ase_calc_route (struct ospf *top, struct ospf_lsdb_slot *slot)
{
  struct ospf_lsa *lsa;
  int count = 0;

  for (lsa = LSDB_LOOP_HEAD (slot); lsa; lsa = lsa->next)
    if (!IS_LSA_SELF (lsa))
      if (!IS_LSA_MAXAGE (lsa))
        {
          if (++count > OSPF_ASE_CALC_COUNT_THRESHOLD)
            {
              slot->next = lsa;
              return 0;
            }

          /* Calculate and install the AS External LSA.  */
          ospf_ase_calc_incremental (top, lsa);
        }

  /* Reset LSDB next pointer.  */
  slot->next = NULL;

  return 1;
}

#ifdef HAVE_NSSA
void
ospf_ase_calc_nssa_route_clean (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
    zlog_info (ZG, "Route[NSSA:%r]: Cleanup NSSA ASE route", &area->area_id);

  /* Cancel threads.  */
  OSPF_EVENT_SUSPEND_OFF (area->t_nssa_calc, area);

  /* Reset the next candidate pointer.  */
  area->lsdb->type[OSPF_AS_NSSA_LSA].next = NULL;
}
#endif /* HAVE_NSSA */

void
ospf_ase_calc_route_clean (struct ospf *top)
{
  struct ospf_master *om = top->om;
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      ospf_ase_calc_nssa_route_clean (area);
#endif /* HAVE_NSSA */

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
    zlog_info (ZG, "Route[ASE]: Cleanup ASE route");

  /* Cancel threads.  */
  OSPF_TIMER_OFF (top->t_ase_calc);

  /* Reset the next candidate pointer.  */
  top->lsdb->type[OSPF_AS_EXTERNAL_LSA].next = NULL;

  /* Reset the flag.  */
  UNSET_FLAG (top->flags, OSPF_ASE_CALC_SUSPENDED);
}

#ifdef HAVE_NSSA
int
ospf_ase_calc_nssa_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct thread *thread_self = area->t_nssa_calc;
  struct ospf *top = area->top;
  struct ospf_master *om = top->om;
  int ret;

  area->t_nssa_calc = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_ase_calc_nssa_event, area, 0);
      return 0;
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
    zlog_info (ZG, "Route[NSSA:%r]: NSSA ASE calculation starts",
               &area->area_id);

  /* Calculate AS External routes.  */
  ret = ospf_ase_calc_route (top, &area->lsdb->type[OSPF_AS_NSSA_LSA]);

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
    zlog_info (ZG, "Route[NSSA:%r]: NSSA ASE calculation %s",
               &area->area_id, ret ? "completed" : "suspended");

  if (!ret)
    OSPF_EVENT_SUSPEND_ON (area->t_nssa_calc,
                           ospf_ase_calc_nssa_event, area, 0);
  else
    {
      /* Reset the suspend thread pointer.  */
      if (area->t_suspend == thread_self)
        area->t_suspend = NULL;

      OSPF_EVENT_PENDING_EXECUTE (area->event_pend);
    }

  return 1;
}
#endif /* HAVE_NSSA */

int
ospf_ase_calc_event (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;
  struct ospf_area *area;
  struct ls_node *rn;
  int ret;

  top->t_ase_calc = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Check if there is a suspended area.  */
  if (!CHECK_FLAG (top->flags, OSPF_ASE_CALC_SUSPENDED))
    for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
      if ((area = RN_INFO (rn, RNI_DEFAULT)))
        if (IS_AREA_SUSPENDED (area, NULL))
          {
            OSPF_TOP_TIMER_ON (top->t_ase_calc, ospf_ase_calc_event,
                               OSPF_ASE_CALC_EVENT_DELAY);
            return 0;
          }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
    zlog_info (ZG, "Route[ASE]: ASE calculation starts");

  /* Calculate AS External routes.  */
  ret = ospf_ase_calc_route (top, &top->lsdb->type[OSPF_AS_EXTERNAL_LSA]);

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_ASE))
    zlog_info (ZG, "Route[ASE]: ASE calculation %s",
               ret ? "completed" : "suspended");

  if (!ret)
    {
      SET_FLAG (top->flags, OSPF_ASE_CALC_SUSPENDED);
      OSPF_TOP_TIMER_ON (top->t_ase_calc, ospf_ase_calc_event, 0);
    }
  else
    {
      UNSET_FLAG (top->flags, OSPF_ASE_CALC_SUSPENDED);
      for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
        if ((area = RN_INFO (rn, RNI_DEFAULT)))
          {
            /* Execute the pending thread if the area is suspended.  */
            if (IS_AREA_SUSPENDED (area, NULL))
              OSPF_EVENT_PENDING_EXECUTE (area->event_pend);

#ifdef HAVE_NSSA
            /* Schedule the NSSA AS external route calculation event.  */
            if (IS_AREA_NSSA (area))
              OSPF_EVENT_ON (area->t_nssa_calc,
                             ospf_ase_calc_nssa_event, area);
#endif /* HAVE_NSSA */
          }
    }

  return 1;
}

void
ospf_ase_calc_event_add (struct ospf *top)
{
  /* Cancel the current suspended thread.  */
  if (CHECK_FLAG (top->flags, OSPF_ASE_CALC_SUSPENDED))
    ospf_ase_calc_route_clean (top);

  /* Schedule the AS-External calculation timer.  */
  OSPF_TOP_TIMER_ON (top->t_ase_calc,
                     ospf_ase_calc_event, OSPF_ASE_CALC_EVENT_DELAY);
}
