/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "prefix.h"
#include "network.h"
#include "filter.h"
#include "linklist.h"
#include "log.h"
#include "thread.h"
#include "nsm_client.h"
#include "table.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_MPLS
#include "ospfd/ospf_mpls.h"
#endif /* HAVE_MPLS */
#ifdef HAVE_GMPLS
#include "lib/gmpls/gmpls.h"
#endif /* HAVE_GMPLS */


/* OSPF redistribution configuration */  
struct ospf_redist_conf *
ospf_redist_conf_new ()
{
  struct ospf_redist_conf *new;
  
  new = XMALLOC (MTYPE_OSPF_REDIST_CONF, sizeof (struct ospf_redist_conf));
  
  if (new == NULL)
    {
      zlog_err (ZG, "Unable to allocate memory");
      return NULL;
    }

  pal_mem_set (new, 0, sizeof (struct ospf_redist_conf));
  
  /* As the new node always appends to the list, make the next pointer as NULL */
  new->next = NULL;

  return new;
}

/* OSPF redistribute configuration */
void
ospf_redist_conf_free (struct ospf_redist_conf *rc)
{
  if (rc)
    XFREE (MTYPE_OSPF_REDIST_CONF, rc);
} 

/* OSPF distance. */
struct ospf_distance *
ospf_distance_new ()
{
  struct ospf_distance *new;

  new = XMALLOC (MTYPE_OSPF_DISTANCE, sizeof (struct ospf_distance));
  pal_mem_set (new, 0, sizeof (struct ospf_distance));

  return new;
}

void
ospf_distance_free (struct ospf_distance *distance)
{
  if (distance->access_list)
    XFREE (MTYPE_OSPF_DESC, distance->access_list);

  XFREE (MTYPE_OSPF_DISTANCE, distance);
}

struct ospf_distance *
ospf_distance_get (struct ospf *top, struct pal_in4_addr addr, u_char masklen)
{
  struct ospf_distance *distance;
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, masklen, addr);
  rn = ls_node_get (top->distance_table, &p);
  if ((distance = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      distance = ospf_distance_new ();
      RN_INFO_SET (rn, RNI_DEFAULT, distance);
    }

  ls_unlock_node (rn);
  return distance;
}

struct ospf_distance *
ospf_distance_lookup (struct ospf *top, struct pal_in4_addr addr,
		      u_char masklen)
{
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, masklen, addr);
  rn = ls_node_lookup (top->distance_table, &p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

void
ospf_distance_delete (struct ospf *top, struct pal_in4_addr addr,
		      u_char masklen)
{
  struct ls_prefix p;
  struct ls_node *rn;

  ls_prefix_ipv4_set (&p, masklen, addr);
  rn = ls_node_lookup (top->distance_table, &p);
  if (rn)
    {
      ospf_distance_free (RN_INFO (rn, RNI_DEFAULT));
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
    }
}

void
ospf_distance_access_list_set (struct ospf_distance *distance,
			       char *access_name)
{
  /* Reset access-list configuration. */
  if (distance->access_list)
    XFREE (MTYPE_OSPF_DESC, distance->access_list);

  /* Set access-list. */
  if (access_name)
    distance->access_list = XSTRDUP (MTYPE_OSPF_DESC, access_name);
  else
    distance->access_list = NULL;
}

void
ospf_distance_table_clear (struct ospf *top)
{
  struct ospf_distance *distance;
  struct ls_node *rn;

  for (rn = ls_table_top (top->distance_table); rn; rn = ls_route_next (rn))
    if ((distance = RN_INFO (rn, RNI_DEFAULT)))
      {
	ospf_distance_free (distance);
	RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
}

int
ospf_distance_update (struct ospf *top, struct ospf_route *route)
{
  struct ospf_path *path = route->selected;
  u_char distance = 0;

  if (path == NULL)
    return 0;

  if (path->type == OSPF_PATH_INTRA_AREA)
    {
      if (top->distance_intra)
	distance = top->distance_intra;
    }
  else if (path->type == OSPF_PATH_INTER_AREA)
    {
      if (top->distance_inter)
	distance = top->distance_inter;
    }
  else if (path->type == OSPF_PATH_EXTERNAL)
    {
      if (top->distance_external)
	distance = top->distance_external;
    }

  if (distance == 0)
    {
      if (top->distance_all)
	distance = top->distance_all;
      else
	distance = APN_DISTANCE_OSPF;
    }

  if (route->distance != distance)
    {
      route->distance = distance;
      return 1;
    }

  return 0;
}

void
ospf_distance_update_all (struct ospf *top)
{
  struct ospf_route *route;
  struct ls_node *rn;

  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      if (ospf_distance_update (top, route))
	OSPF_NSM_ADD (top, rn->p, route, route->selected);
}

void
ospf_distance_update_by_type (struct ospf *top, u_char type)
{
  struct ospf_route *route;
  struct ls_node *rn;

  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_DEFAULT)))
      if ((route->selected) && (route->selected->type == type))
	if (ospf_distance_update (top, route))
	  OSPF_NSM_ADD (top, rn->p, route, route->selected);
}


struct ospf_redist_map *
ospf_redist_map_new ()
{
  struct ospf_redist_map *new;

  new = XMALLOC (MTYPE_OSPF_REDIST_MAP, sizeof (struct ospf_redist_map));
  pal_mem_set (new, 0, sizeof (struct ospf_redist_map));

  new->arg.metric = -1;
  new->arg.metric_type = -1;

  return new;
}

void
ospf_redist_map_free (struct ospf_redist_map *map)
{
  LSA_MAP_FLUSH (map->lsa);
  XFREE (MTYPE_OSPF_REDIST_MAP, map);
}

void
ospf_redist_map_lsa_refresh (struct ospf_redist_map *map)
{
  struct ospf_redist_arg arg, ra_old;
  struct ospf_summary *summary;
  struct ospf *top;
  
  pal_mem_set (&ra_old, 0, sizeof (ra_old));

#ifdef HAVE_NSSA
  if (map->type == OSPF_AS_NSSA_LSA)
    if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
	&& map->ri->type == APN_ROUTE_DEFAULT)
      if (!OSPF_AREA_CONF_CHECK (map->u.area, DEFAULT_ORIGINATE))
	return;
#endif /* HAVE_NSSA */

#ifdef HAVE_NSSA
  if (map->type == OSPF_AS_NSSA_LSA)
    top = map->u.area->top;
  else
#endif /* HAVE_NSSA */
    top = map->u.top;

  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
    if (map->ri->type == APN_ROUTE_CONNECT)
      if (ospf_network_same (top, map->ri->rn->p))
	return;

#ifdef HAVE_OSPF_DB_OVERFLOW
  if (IS_DB_OVERFLOW (top))
    return;
#endif /* HAVE_OSPF_DB_OVERFLOW */

  if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
      || CHECK_FLAG (map->source, OSPF_REDIST_MAP_TRANSLATE)
      || CHECK_FLAG (map->source, OSPF_REDIST_MAP_CONFIG_DEFAULT))
    {
      /* Match summary-address. */
      summary = ospf_summary_match (top->summary, map->lp);
      if (summary != NULL)
	{
	  OSPF_TIMER_ON (summary->t_update, ospf_summary_address_timer,
			 summary, OSPF_SUMMARY_TIMER_DELAY);
	  return;
	}

      /* Preserve old values. */
      ra_old = map->arg;
      if (ospf_redist_map_arg_set (top, map, &arg))
	map->arg = arg;
      else
	return;
    }

  /* If there is no LSA previously originated or
     one of arguments is changed, originate/refresh external LSA. */
  if (map->lsa == NULL
      || map->arg.metric_type != ra_old.metric_type
      || map->arg.metric != ra_old.metric
      || map->arg.nexthop.s_addr != ra_old.nexthop.s_addr
      || map->arg.tag != ra_old.tag
#ifdef HAVE_NSSA
      || map->arg.propagate != ra_old.propagate
#endif /* HAVE_NSSA */
      )
    LSA_REFRESH_TIMER_ADD (top, map->type, map->lsa, map);
}

void
ospf_redist_map_summary_update (struct ospf_redist_map *map)
{
  struct ospf_summary *summary;
  struct ospf *top;

#ifdef HAVE_NSSA
  if (map->type == OSPF_AS_NSSA_LSA)
    {
      if (!IS_AREA_ACTIVE (map->u.area))
	return;
      top = map->u.area->top;
    }
  else
#endif /* HAVE_NSSA */
    top = map->u.top;

  if (!IS_PROC_ACTIVE (top))
    return;

  summary = ospf_summary_match (top->summary, map->lp);
  if (summary != NULL)
    OSPF_TIMER_ON (summary->t_update, ospf_summary_address_timer,
		   summary, OSPF_SUMMARY_TIMER_DELAY);
}

#ifdef HAVE_VRF_OSPF
void
ospf_vrf_redist_map_update(struct ls_table *table,
                           struct ospf_redist_info *ri,
                           void *parent)
{
  struct ospf_ia_lsa_map *map;
  struct ospf_area *area;
  struct ospf *top;
  struct ls_node *rn;

  top = (struct ospf *)parent;
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    {
     if ((area = RN_INFO (rn, RNI_DEFAULT)))
       if (IS_AREA_ACTIVE (area))
         {
           map = ospf_ia_lsa_map_lookup (area->ia_prefix, ri->rn->p);
           if (map == NULL)
             {
               /* Create the map structure for the summary LSA learnt via BGP */
               map = ospf_ia_lsa_map_new (OSPF_IA_MAP_PATH);
               map->area = area;
               map->lp = ri->rn->p;
               if ( map->u.path == NULL)
                 {
                   map->cost = ri->metric;
                   SET_FLAG (map->flags, OSPF_SET_SUMMARY_DN_BIT);
                 }
               ospf_ia_lsa_map_add (area->ia_prefix, ri->rn->p, map);
             }
           LSA_REFRESH_TIMER_ADD(area->top, OSPF_SUMMARY_LSA, map->lsa, map);
         }
    } 
}
#endif /* HAVE_VRF_OSPF */

void
ospf_redist_map_update (struct ls_table *table,
			struct ospf_redist_info *ri,
			u_char type, void *parent)
{
  struct ospf_redist_map *map;
  struct ospf *top;
  struct ls_node *rn;

#ifdef HAVE_NSSA
  if (type == OSPF_AS_NSSA_LSA)
    top = ((struct ospf_area *)parent)->top;
  else
#endif /* HAVE_NSSA */
    top = (struct ospf *)parent;

  rn = ls_node_get (table, ri->rn->p);
  if ((map = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      map = ospf_redist_map_new ();
      map->lp = rn->p;
      map->type = type;
      map->u.parent = parent;
      RN_INFO_SET (rn, RNI_DEFAULT, map);
    }

  if (!CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
    {
      ri->count++;
      map->ri = ri;
      SET_FLAG (map->source, OSPF_REDIST_MAP_NSM);
    }

   /* If the VRF enabled OSPF receives a route then 
      the DN-bit need to be set for the route */
  if (CHECK_FLAG (top->ov->flags, OSPF_VRF_ENABLED))
    SET_FLAG (map->flags, OSPF_REDIST_EXTERNAL_SET_DN_BIT);

  if (ri->type != APN_ROUTE_CONNECT
      || !ospf_network_match (top, ri->rn->p))
    ospf_redist_map_lsa_refresh (map);
   
  ls_unlock_node (rn);
}

#ifdef HAVE_VRF_OSPF
void
ospf_vrf_redist_map_delete (struct ls_table *table, struct ospf_redist_info *ri,
                             struct ospf *top)
{
  struct ospf_ia_lsa_map *map;
  struct ls_node *rn;
  struct ospf_area *area;
  
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    {
     if ((area = RN_INFO (rn, RNI_DEFAULT)))
       if (IS_AREA_ACTIVE (area))
         {
           map = ospf_ia_lsa_map_lookup (area->ia_prefix, ri->rn->p);
           if (map != NULL)
             {
               ospf_ia_lsa_map_delete (area->ia_prefix, ri->rn->p);
               UNSET_FLAG (map->flags, OSPF_SET_SUMMARY_DN_BIT);
               UNSET_FLAG (ri->flags, OSPF_REDIST_VRF_INFO);
               ospf_ia_lsa_map_free (map); 
             }
         }
    }
}
#endif  /* HAVE_VRF_OSPF */

void
ospf_redist_map_delete (struct ls_table *table, struct ospf_redist_info *ri)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;

  rn = ls_node_lookup (table, ri->rn->p);
  if (rn)
    {
      map = RN_INFO (rn, RNI_DEFAULT);
      if (CHECK_FLAG (map->flags, OSPF_REDIST_MAP_MATCH_SUMMARY))
	ospf_redist_map_summary_update (map);

      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
	{
	  UNSET_FLAG (map->source, OSPF_REDIST_MAP_NSM);
          ri->count--;

          if (ri->count == 0)
            {
              ospf_redist_info_unlock (ri);
              map->ri = NULL;
            }
        }

      if (map->source == 0)
	{
	  ospf_redist_map_free (map);
	  RN_INFO_UNSET (rn, RNI_DEFAULT);
	}
      else
	ospf_redist_map_lsa_refresh (map);

      ls_unlock_node (rn);
    }
}

void
ospf_redist_map_default_update (struct ls_table *table,
				u_char type, void *parent)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;

  if (type == OSPF_AS_EXTERNAL_LSA)
    if (!ospf_area_default_count (parent))
      return;

  rn = ls_node_get (table, &LsPrefixIPv4Default);
  if ((map = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      map = ospf_redist_map_new ();
      map->lp = rn->p;
      map->type = type;
      map->u.parent = parent;
      RN_INFO_SET (rn, RNI_DEFAULT, map);
    }
  SET_FLAG (map->source, OSPF_REDIST_MAP_CONFIG_DEFAULT);

  ospf_redist_map_lsa_refresh (map);

  ls_unlock_node (rn);
}

void
ospf_redist_map_default_delete (struct ls_table *table)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;

  rn = ls_node_lookup (table, &LsPrefixIPv4Default);
  if (rn)
    {
      map = RN_INFO (rn, RNI_DEFAULT);
      UNSET_FLAG (map->source, OSPF_REDIST_MAP_CONFIG_DEFAULT);
      if (map->source == 0)
	{
	  ospf_redist_map_free (map);
	  RN_INFO_UNSET (rn, RNI_DEFAULT);
	}
      ls_unlock_node (rn);
    }
}

#ifdef HAVE_NSSA
int
ospf_redist_map_nssa_update (struct ospf_lsa *lsa)
{
  struct ospf_area *area = lsa->lsdb->u.area;
  struct ospf_redist_map *map;
  struct pal_in4_addr *mask;
  struct ls_node *rn;
  struct pal_in4_addr *nh;
  struct ospf_vertex *v;
  struct ls_prefix lp;
  u_char bits;

  if (!ospf_area_default_count (area->top))
    return 0;

  if (!CHECK_FLAG (lsa->data->options, OSPF_OPTION_NP))
    return 0;

  nh = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NEXTHOP_OFFSET); 
  if (nh->s_addr == 0)
    return 0;

  /* RFC3101 1.3 Proposed solution

     Note that a Type-7 default LSA originated by an NSSA border router is
     never translated into a Type-5 LSA, however, a Type-7 default LSA
     originated by an NSSA internal AS boundary router (one that is not an
     NSSA border router) may be translated into a Type-5 LSA.  */
  if (lsa->data->id.s_addr == 0)
    {
      if (IS_LSA_SELF (lsa))
	return 0;
      else
	{
	  v = ospf_spf_vertex_lookup (area, lsa->data->adv_router);
	  if (v == NULL)
	    return 0;

	  bits = OSPF_PNT_UCHAR_GET (v->lsa->data,
				     OSPF_ROUTER_LSA_BITS_OFFSET);
	  if (IS_ROUTER_LSA_SET (bits, BIT_B))
	    return 0;
	}
    }

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  ls_prefix_ipv4_set (&lp, ip_masklen (*mask), lsa->data->id);

  rn = ls_node_get (area->top->redist_table, &lp);
  if ((map = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      map = ospf_redist_map_new ();
      map->lp = rn->p;
      map->type = OSPF_AS_EXTERNAL_LSA;
      map->u.top = area->top;
      RN_INFO_SET (rn, RNI_DEFAULT, map);
    }
  
  if (map->nssa_lsa != NULL)
    ospf_lsa_unlock (map->nssa_lsa);
  map->nssa_lsa = ospf_lsa_lock (lsa);
  SET_FLAG (map->source, OSPF_REDIST_MAP_TRANSLATE);

  ospf_redist_map_lsa_refresh (map);

  ls_unlock_node (rn);

  return 1;
}

int
ospf_redist_map_nssa_delete (struct ospf_lsa *lsa)
{
  struct ospf_area *area = lsa->lsdb->u.area;
  struct ospf_redist_map *map;
  struct pal_in4_addr *mask;
  struct ls_prefix lp;
  struct ls_node *rn;
  
  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  ls_prefix_ipv4_set (&lp, ip_masklen (*mask), lsa->data->id);

  rn = ls_node_lookup (area->top->redist_table, &lp);
  if (rn != NULL)
    {
      map = RN_INFO (rn, RNI_DEFAULT);
      UNSET_FLAG (map->source, OSPF_REDIST_MAP_TRANSLATE);
      if (map->nssa_lsa != NULL)
        {
          ospf_lsa_unlock (map->nssa_lsa);
          map->nssa_lsa = NULL;
        }
      if (map->source == 0)
	{
	  ospf_redist_map_free (map);
	  RN_INFO_UNSET (rn, RNI_DEFAULT);
	}
      ls_unlock_node (rn);
    }

  return 1;
}
#endif /* HAVE_NSSA */

#ifdef HAVE_NSSA
void
ospf_nssa_redist_map_update_all_by_type (struct ospf_area *area, u_char type)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;

  for (rn = ls_table_top (area->redist_table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
	if (map->ri->type == type)
	  ospf_redist_map_lsa_refresh (map);
}
#endif /* HAVE_NSSA */

void
ospf_redist_map_update_all_by_type (struct ospf *top, u_char type, int sproc_id)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;
#ifdef HAVE_NSSA
  struct ospf_area *area;
#endif /* HAVE_NSSA */

  for (rn = ls_table_top (top->redist_table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
        /* Check for pid also  */
	if (map->ri->type == type && map->ri->pid == sproc_id)
	  ospf_redist_map_lsa_refresh (map);

#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
	if (IS_AREA_NSSA (area))
	  ospf_nssa_redist_map_update_all_by_type (area, type);
#endif /* HAVE_NSSA */
}

/* Cleaning up the invalid LSA. */ 
void
ospf_redist_map_lsa_delete (struct ospf *top, struct ospf_lsa *lsa,
                            struct ospf_lsdb *lsdb)
{
  struct ospf_master *om = top->om;
  struct ospf_redist_info *ri;
  struct pal_in4_addr *mask;
  struct ls_prefix lp;
  struct ls_node *rn;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  ls_prefix_ipv4_set (&lp, ip_masklen (*mask), lsa->data->id);

  rn = ls_node_lookup (top->ov->redist_table, &lp);
  if (rn == NULL)
    {
      if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
        zlog_info (ZG, "NSM[Redistribute]: %P does not exist", &lp);

        ospf_lsdb_lsa_discard (lsdb, lsa, 1, 1, 1);
    }
  else
    {
      ri = RN_INFO (rn, RNI_DEFAULT);
      if (ri != NULL
          && CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
        {
          if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
            zlog_info (ZG, "NSM[Redistribute:%s]: %P is deleted", 
                       LOOKUP (ospf_redist_proto_msg, ri->type), &lp);

            ospf_lsdb_lsa_discard (lsdb, lsa, 1, 1, 1);
        }
    }
}

/* Update LSA map information and refresh */
static void
ospf_redistribute_lsa_refresh_check (struct ospf *top, struct ospf_lsdb *lsdb,
                                     struct ospf_lsa *lsa,
                                     struct ospf_redist_map *map)
{
  struct ospf_redist_info *ri;
  struct pal_in4_addr *mask;
  struct ls_prefix lp;
  struct ls_node *rn;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  ls_prefix_ipv4_set (&lp, ip_masklen (*mask), lsa->data->id);

  rn = ls_node_lookup (top->ov->redist_table, &lp);
  if (rn != NULL
      && (ri = RN_INFO (rn, RNI_DEFAULT)))
    {
      if (!CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED)
          && (ri->type != APN_ROUTE_CONNECT
          || !ospf_network_match (top, ri->rn->p)))
        {
          if (lsa->param == NULL)
            {
              map->lsa = ospf_lsa_lock (lsa);
              lsa->param = map;
              LSA_REFRESH_TIMER_ADD (top, lsa->data->type, lsa, lsa->param);
            }
        }
    }
}

/* Update External LSA map and refresh/originate */
static void
ospf_redist_map_external_lsa_update_all (struct ospf *top)
{
  struct ospf_lsdb *lsdb = top->lsdb;
  struct ospf_redist_map *map;
  struct ospf_redist_info *ri;
  struct ls_node *r1, *r2;
  struct pal_in4_addr *id;
  struct ospf_lsa *lsa;

  for (r1 = ls_table_top (top->ov->redist_table); r1;
       r1 = ls_route_next (r1))
    if ((ri = RN_INFO (r1, RNI_DEFAULT)))
      {
        if (!CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
          {
            r2 = ls_node_get (top->redist_table, ri->rn->p);

            if (r2 && (map = RN_INFO (r2, RNI_DEFAULT)))
              if (map->ri != NULL
                  && CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
                  && r2->p != NULL)
                {
                  /* Check if lsa refresh/origination required */
                  id = (struct pal_in4_addr *)r2->p->prefix;
                  lsa = ospf_lsdb_lookup_by_id (lsdb, OSPF_AS_EXTERNAL_LSA,
                                                *id, top->router_id);
                  if (lsa != NULL)
                    ospf_redistribute_lsa_refresh_check (top, lsdb, lsa, map);
                  else
                    lsa = ospf_lsa_originate (top, OSPF_AS_EXTERNAL_LSA, map);
                }
          }
      }
}

/* Update NSSA-External LSA map and refresh/originate */
#ifdef HAVE_NSSA
void
ospf_redist_map_nssa_lsa_update_all (struct ospf_area *area)
{
  struct ls_node *r1, *r2;
  struct ospf_lsa *lsa;
  struct pal_in4_addr *id;
  struct ospf_redist_map *map;
  struct ospf_redist_info *ri;
  struct ospf *top = area->top;
  struct ospf_lsdb *lsdb = area->lsdb;

  for (r1 = ls_table_top (top->ov->redist_table); r1;
       r1 = ls_route_next (r1))
    if ((ri = RN_INFO (r1, RNI_DEFAULT)))
      {
        if (!CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
          {
            r2 = ls_node_get (area->redist_table, ri->rn->p);

            if (r2 && (map = RN_INFO (r2, RNI_DEFAULT)))
              if (map->ri != NULL
                  && CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM)
                  && r2->p != NULL)
                {
                  /* Check if lsa refresh/origination required */  
                  id = (struct pal_in4_addr *)r2->p->prefix;
                  lsa = ospf_lsdb_lookup_by_id (lsdb, OSPF_AS_NSSA_LSA,
                                                *id, top->router_id);
                  if (lsa != NULL)
                    ospf_redistribute_lsa_refresh_check (top, lsdb, lsa, map);
                  else
                    lsa = ospf_lsa_originate (top, OSPF_AS_NSSA_LSA, map);
                }
          }
      }
}
#endif /* HAVE_NSSA */

/* Update Type-5 and Type-7 lsa redist-map information */
void
ospf_redist_map_lsa_update_all (struct ospf *top)
{
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;
#endif /* HAVE_NSSA */

  if (!IS_PROC_ACTIVE (top))
    return;

  ospf_redist_map_external_lsa_update_all (top);
       
#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        if (IS_AREA_NSSA (area))
          ospf_redist_map_nssa_lsa_update_all (area);
#endif /* HAVE_NSSA */
}

#ifdef HAVE_NSSA
void
ospf_nssa_redist_map_update_all (struct ospf_area *area)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;
  
  for (rn = ls_table_top (area->redist_table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
	if (map->ri->rn->p->prefixlen != 0)
	  ospf_redist_map_lsa_refresh (map);
}
#endif /* HAVE_NSSA */

void
ospf_redist_map_update_all (struct ospf *top)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;
#ifdef HAVE_NSSA
  struct ospf_area *area;
#endif /* HAVE_NSSA */
  
  for (rn = ls_table_top (top->redist_table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
	if (map->ri->rn->p->prefixlen != 0)
	  ospf_redist_map_lsa_refresh (map);

#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
	if (IS_AREA_NSSA (area))
	  ospf_nssa_redist_map_update_all (area);
#endif /* HAVE_NSSA */
}

#ifdef HAVE_NSSA
void
ospf_nssa_redist_map_update_by_prefix (struct ospf_area *area,
				       struct prefix *p)
{
  struct ospf_redist_map *map;
  struct ls_prefix lp;
  struct ls_node *rn;

  ls_prefix_ipv4_set (&lp, p->prefixlen, p->u.prefix4);

  rn = ls_node_lookup (area->redist_table, &lp);
  if (rn)
    {
      map = RN_INFO (rn, RNI_DEFAULT);
      if (map != NULL)
	{
	  if (ospf_network_match (area->top, &lp))
	    {
	      ospf_redist_map_free (map);
	      RN_INFO_UNSET (rn, RNI_DEFAULT);
	    }
	  else
	    ospf_redist_map_lsa_refresh (map);
	}
      ls_unlock_node (rn);
    }
}
#endif /* HAVE_NSSA */

void
ospf_redist_map_update_by_prefix (struct ospf *top, struct prefix *p)
{
  struct ospf_redist_map *map;
  struct ls_prefix lp;
  struct ls_node *rn;
#ifdef HAVE_NSSA
  struct ospf_area *area;
#endif /* HAVE_NSSA */

  ls_prefix_ipv4_set (&lp, p->prefixlen, p->u.prefix4);

  rn = ls_node_lookup (top->redist_table, &lp);
  if (rn)
    {
      map = RN_INFO (rn, RNI_DEFAULT);
      if (map != NULL)
	{
	  if (ospf_network_match (top, &lp))
	    {
	      ospf_redist_map_free (map);
	      RN_INFO_UNSET (rn, RNI_DEFAULT);
	    }
	  else
	    ospf_redist_map_lsa_refresh (map);
	}
      ls_unlock_node (rn);
    }

#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
	if (IS_AREA_NSSA (area))
	  ospf_nssa_redist_map_update_by_prefix (area, p);
#endif /* HAVE_NSSA */
}

void
ospf_redist_map_delete_all_by_type (struct ospf *top,
				    struct ls_table *table, u_char type, int sproc_id)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;

  for (rn = ls_table_top (table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (map->source, OSPF_REDIST_MAP_NSM))
       if (map->ri->type == type && map->ri->pid == sproc_id)
	 {
	   if (CHECK_FLAG (map->flags, OSPF_REDIST_MAP_MATCH_SUMMARY))
	     ospf_redist_map_summary_update (map);

	   UNSET_FLAG (map->source, OSPF_REDIST_MAP_NSM);
	   if (map->source == 0)
	     {
	       ospf_redist_map_free (map);
	       RN_INFO_UNSET (rn, RNI_DEFAULT);
	     }
	 }
}

void
ospf_redist_map_delete_all (struct ls_table *table)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;

  for (rn = ls_table_top (table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      {
	if (CHECK_FLAG (map->flags, OSPF_REDIST_MAP_MATCH_SUMMARY))
	  ospf_redist_map_summary_update (map);
	ospf_redist_map_free (map);
	RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
}

void
ospf_redist_map_flush_lsa_all (struct ls_table *table)
{
  struct ospf_redist_map *map;
  struct ls_node *rn;

  for (rn = ls_table_top (table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (!CHECK_FLAG (map->source, OSPF_REDIST_MAP_CONFIG_DEFAULT))
	LSA_MAP_FLUSH (map->lsa);
}


void
ospf_nsm_redistribute_set (struct apn_vr *vr, int type, struct ospf_vrf *ov, int sproc_id)
{
  struct ospf_master *om = vr->proto;
  struct nsm_msg_redistribute msg;

  if (ov->iv->id == VRF_ID_DISABLE)
    return;
  
  /* Allow multiple redistribution of OSPF instances  */
  if (type != APN_ROUTE_OSPF && ov->redist_count[type]++)
    return;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_redistribute));
  msg.type = type;
  NSM_SET_CTYPE(msg.cindex, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID); 
  msg.pid = sproc_id;
  msg.afi = AFI_IP;

  nsm_client_send_redistribute_set (vr->id, ov->iv->id, om->zg->nc, &msg);
}

void
ospf_nsm_redistribute_unset (struct apn_vr *vr, int type, struct ospf_vrf *ov, int sproc_id)
{
  struct ospf_master *om = vr->proto;
  struct nsm_msg_redistribute msg;
  struct ospf_redist_info *ri;
  struct ls_node *rn;

  if (ov->iv->id == VRF_ID_DISABLE)
    return;

  /* Check redist count for redistribution other than OSPF  */
  if (type != APN_ROUTE_OSPF)
    if (ov->redist_count[type] == 0 || --ov->redist_count[type])
      return;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_redistribute));
  msg.type = type;
  msg.afi = AFI_IP;

  /* Set this flag to know nsm that the PM has sent PID 
     info in the recieved message */
  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_PROCESS_ID);
  msg.pid = sproc_id;

  /* Cleanup. */
  for (rn = ls_table_top (ov->redist_table); rn; rn = ls_route_next (rn))
    if ((ri = RN_INFO (rn, RNI_DEFAULT)))
      if (ri->type == type && ri->pid == sproc_id)
	{
          ospf_redist_info_free (ri);
	  RN_INFO_UNSET (rn, RNI_DEFAULT);
	}

  nsm_client_send_redistribute_unset (vr->id, ov->iv->id, om->zg->nc, &msg);
}

#ifdef HAVE_NSSA
int
ospf_nssa_redistribute_timer (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct ospf_redist_info *ri;
  struct ls_node *rn, *node;
  struct ospf_redist_map *map;
  struct ospf_master *om = area->top->om;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  area->t_redist = NULL;

  if (IS_OSPF_ABR (area->top) && (area->conf.default_config == PAL_TRUE))
    ospf_redist_map_default_update (area->redist_table,
                                          OSPF_AS_NSSA_LSA, area);
  else
    ospf_redist_map_default_delete (area->redist_table);

  if (IS_AREA_NSSA (area))
    for (rn = ls_table_top (area->top->ov->redist_table);
	 rn; rn = ls_route_next (rn))
      if ((ri = RN_INFO (rn, RNI_DEFAULT)))
	{
	  if (CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
	    ospf_redist_map_delete (area->redist_table, ri);

	  else if (!REDIST_PROTO_CHECK (&(area)->top->redist[ri->type]))
	    ospf_redist_map_delete (area->redist_table, ri);
         
          else if (OSPF_AREA_CONF_CHECK ((area), NO_REDISTRIBUTION))
            {
              node = ls_node_lookup (area->redist_table, ri->rn->p);
              if (node) 
                if ((map = RN_INFO (node, RNI_DEFAULT)))
                  LSA_MAP_FLUSH (map->lsa); 
            }

	  else if (TV_CMP (ri->tv_update, area->tv_redist) > 0
		   || CHECK_FLAG (area->redist_update, (1 << ri->type)))
	    if (ri->type != APN_ROUTE_CONNECT
		|| !ospf_network_match (area->top, ri->rn->p))
	      ospf_redist_map_update (area->redist_table, ri,
				      OSPF_AS_NSSA_LSA, area);
	}

  /* Update timestamp. */
  pal_time_tzcurrent (&area->tv_redist, NULL);

  /* Reset the update flags.  */
  area->redist_update = 0;

  return 0;
}

void
ospf_nssa_redistribute_timer_add (struct ospf_area *area, u_char type)
{
  OSPF_AREA_TIMER_ON (area->t_redist, ospf_nssa_redistribute_timer,
		      OSPF_REDISTRIBUTE_TIMER_DELAY);
  SET_FLAG (area->redist_update, (1 << type));
}

void
ospf_nssa_redistribute_timer_add_all_proto (struct ospf_area *area)
{
  int type;

  if (IS_AREA_ACTIVE (area))
    if (IS_AREA_NSSA (area))
      for (type = 0; type < APN_ROUTE_MAX; type++)
	ospf_nssa_redistribute_timer_add (area, type);
}

void
ospf_nssa_redistribute_timer_delete_all_proto (struct ospf_area *area)
{
  /* Clearnup the NSSA AS External LSAs.  */
  ospf_redist_map_delete_all (area->redist_table);

  /* Reset the update flags.  */
  area->redist_update = 0;
}
#endif /* HAVE_NSSA */

#ifdef HAVE_VRF_OSPF
bool_t
ospf_vrf_compare_domain_id (struct ospf *top, 
                        struct ospf_redist_info *ri)
{
  struct ospf_vrf_domain_id *tmp, *domain_id;
  struct listnode *nn;
  u_char domainid[8];

  pal_mem_set (domainid, 0, sizeof (struct ospf_vrf_domain_id));
  pal_mem_cpy (domainid, ri->domain_info.domain_id, 
                            OSPF_DOMAIN_ID_FLENGTH);
  tmp = (struct ospf_vrf_domain_id *)(domainid);
  tmp->type = pal_ntoh16(tmp->type);

  if (CHECK_FLAG(top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY)||
      CHECK_FLAG (top->config , OSPF_CONFIG_DOMAIN_ID_SEC))
    {
      if (!pal_mem_cmp (top->pdomain_id, tmp, OSPF_DOMAIN_ID_FLENGTH))
        return PAL_TRUE;
      else if (((top->pdomain_id->type == TYPE_AS_DID)
                 && (tmp->type == TYPE_BACK_COMP_DID))
                || ((top->pdomain_id->type == TYPE_BACK_COMP_DID)
                 &&(tmp->type == TYPE_AS_DID)))
        {
          if (!pal_mem_cmp (top->pdomain_id->value, tmp->value,
                                             OSPF_DOMAIN_ID_VLENGTH))
             return PAL_TRUE;
        }
      else if ((tmp->type == TYPE_IP_DID) &&
                (top->pdomain_id->type == TYPE_IP_DID))
        {
          /* Compare the IP address format for CISCO compatibility. */
          if (!pal_mem_cmp (top->pdomain_id->value, tmp->value,
                                  OSPF_IP_DOMAIN_ID_VLENGTH))
             return PAL_TRUE;
        }

      if (CHECK_FLAG (top->config , OSPF_CONFIG_DOMAIN_ID_SEC))
        {
          LIST_LOOP (top->domainid_list, domain_id, nn)
            {
              if (!pal_mem_cmp (domain_id, tmp, OSPF_DOMAIN_ID_FLENGTH))
                {
                  return PAL_TRUE;
                }
              else if (((domain_id->type == TYPE_AS_DID)
                         && (tmp->type == TYPE_BACK_COMP_DID))
                        || ((domain_id->type == TYPE_BACK_COMP_DID)
                         &&(tmp->type == TYPE_AS_DID)))
                {
                  if (!pal_mem_cmp (domain_id->value, tmp->value,
                                                OSPF_DOMAIN_ID_VLENGTH))
                      return PAL_TRUE;
                }
              else if ((tmp->type == TYPE_IP_DID) &&
                        (domain_id->type == TYPE_IP_DID))
                {
                  if (!pal_mem_cmp (domain_id->value,
                             tmp->value, OSPF_IP_DOMAIN_ID_VLENGTH))
                     return PAL_TRUE;
                }
            }
        }
    }
  else
    {
     /* RFC 4577 (sec 4.2.4)If domain-Id is not configured
      the default value is NULL so check if the received ID is NULL */
     if (tmp->type == 0)
       return PAL_TRUE;
    }    

  return PAL_FALSE;
}

#endif /* HAVE_VRF_OSPF */

int
ospf_redistribute_timer (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;
  struct ospf_redist_info *ri;
  struct ls_node *rn;
  struct ospf_redist_conf *rc;
#ifdef HAVE_VRF_OSPF
  bool_t match  = PAL_FALSE;
#endif /* HAVE_VRF_OSPF */

  top->t_redist = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (ospf_area_default_count (top))
    for (rn = ls_table_top (top->ov->redist_table);
	 rn; rn = ls_route_next (rn))
     {
      if ((ri = RN_INFO (rn, RNI_DEFAULT)))
	{
          /* Get the corresponding redist conf structure */
          rc = ospf_redist_conf_lookup_by_inst (top, ri->type, ri->pid);

#ifdef HAVE_VRF_OSPF
          if (CHECK_FLAG (ri->flags, OSPF_REDIST_VRF_INFO)
              && (ri->domain_info.r_type == OSPF_SUMMARY_LSA)
              && (ri->type == APN_ROUTE_BGP))
            match = ospf_vrf_compare_domain_id (top, ri); 
#endif /* HAVE_VRF_OSPF */

	  if (CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
            {
#ifdef HAVE_VRF_OSPF
              /* Check if the route is vrf redistributed route */
              if (CHECK_FLAG (ri->flags, OSPF_REDIST_VRF_INFO) && match)
                {
                  ospf_vrf_redist_map_delete (top->rt_network, ri, top);
                }
              else
#endif /* HAVE_VRF_OSPF */
                ospf_redist_map_delete (top->redist_table, ri);
            }
	  else if (rc && !REDIST_PROTO_CHECK (rc))
            {
#ifdef HAVE_VRF_OSPF
              if (CHECK_FLAG (ri->flags, OSPF_REDIST_VRF_INFO) && match)
                ospf_vrf_redist_map_delete (top->rt_network, ri, top);
              else
#endif /* HAVE_VRF_OSPF */
  	        ospf_redist_map_delete (top->redist_table, ri);
            }
          else if (TV_CMP (ri->tv_update, top->tv_redist) > 0
		   || CHECK_FLAG (top->redist_update, (1 << ri->type)))
            {
#ifdef HAVE_VRF_OSPF
              /* Check if the route is vrf redistributed route */
              if (CHECK_FLAG (ri->flags, OSPF_REDIST_VRF_INFO) && match)
                {
                  ospf_vrf_redist_map_update (top->rt_network, ri, top);
                }
              else
                {
#endif /*HAVE_VRF_OSPF */
                  if (top->ospf_id == ri->pid  
                      && (ri->type == APN_ROUTE_OSPF))
                    continue;
                  else
                    ospf_redist_map_update (top->redist_table, ri,
                                  OSPF_AS_EXTERNAL_LSA, top);
#ifdef HAVE_VRF_OSPF
                }
#endif /*HAVE_VRF_OSPF */
            }
	}
     }

  /* Update timestamp. */
  pal_time_tzcurrent (&top->tv_redist, NULL);

  /* Reset the update flags.  */
  top->redist_update = 0;

  return 0;
}

void
ospf_redistribute_timer_add (struct ospf *top, u_char type)
{
  struct ospf_redist_conf *rc = &top->redist[type];
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;
#endif /* HAVE_NSSA */

  if (!IS_PROC_ACTIVE (top))
    return;

  /* Trigger the timer for every instance in that top */
  for (; rc; rc = rc->next)
   if (REDIST_PROTO_CHECK (rc))
    if (ospf_area_default_count (top))
      {
       OSPF_TOP_TIMER_ON (top->t_redist, ospf_redistribute_timer,
	       OSPF_REDISTRIBUTE_TIMER_DELAY);
       SET_FLAG (top->redist_update, (1 << type));
      }

#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
	if (IS_AREA_NSSA (area))
	  ospf_nssa_redistribute_timer_add (area, type);
#endif /* HAVE_NSSA */
}

void
ospf_redistribute_timer_add_all_proto (struct ospf *top)
{
  struct ospf_redist_conf *rc; 
  int type;

  for (type = 0; type < APN_ROUTE_MAX; type++)
   for (rc = &top->redist[type]; rc; rc = rc->next)
    if (REDIST_PROTO_CHECK (rc))
      ospf_redistribute_timer_add (top, type);
}

void
ospf_redistribute_timer_delete_all_proto (struct ospf *top)
{
  /* Clearnup the AS External LSAs.  */
  ospf_redist_map_delete_all (top->redist_table);

  /* Reset the update flags.  */
  top->redist_update = 0;
}


struct ospf_redist_info *
ospf_redist_info_new (struct ls_node *rn)
{
  struct ospf_redist_info *new;

  new = XMALLOC (MTYPE_OSPF_REDIST_INFO, sizeof (struct ospf_redist_info));
  pal_mem_set (new, 0, sizeof (struct ospf_redist_info));
  RN_INFO_SET (rn, RNI_DEFAULT, new);
  new->rn = rn;

  return new;
}

void
ospf_redist_info_free (struct ospf_redist_info *ri)
{
  XFREE (MTYPE_OSPF_REDIST_INFO, ri);
}

struct ospf_redist_info *
ospf_redist_info_lock (struct ospf_redist_info *ri)
{
  ri->lock++;
  return ri;
}

void
ospf_redist_info_unlock (struct ospf_redist_info *ri)
{
  ri->lock--;
  if (ri->lock == 0)
    {
      RN_INFO_UNSET (ri->rn, RNI_DEFAULT);
      ospf_redist_info_free (ri);
    }
}

void
ospf_redist_info_timer_add (struct ospf_master *om, u_char type)
{
  struct listnode *node;
  struct ospf *top;

  LIST_LOOP (om->ospf, top, node)
    ospf_redistribute_timer_add (top, type);
}

void
ospf_redist_info_add (struct ospf_vrf *ov, struct nsm_msg_route_ipv4 *msg)
{
  int type = msg->prefixlen == 0 ? APN_ROUTE_DEFAULT : msg->type;
  struct ospf_master *om = ov->iv->vr->proto;
  struct ospf_redist_info *ri;
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, msg->prefixlen, msg->prefix);
  if (type != APN_ROUTE_OSPF && ov->redist_count[type] == 0)
    {
      if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
	zlog_info (ZG, "NSM[Redistribute:%s]: is off, ignore %P",
		   LOOKUP (ospf_redist_proto_msg, type), &p);
      return;
    }

  rn = ls_node_get (ov->redist_table, &p);
  if ((ri = RN_INFO (rn, RNI_DEFAULT)))
    {
      if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
	zlog_info (ZG, "NSM[Redistribute:%s]: %P updated",
		   LOOKUP (ospf_redist_proto_msg, type), &p);
    }
  else
    {
      if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
	zlog_info (ZG, "NSM[Redistribute:%s]: %P created",
		   LOOKUP (ospf_redist_proto_msg, type), &p);
      
      ri = ospf_redist_info_new (rn);
    }

  ri->type = type;
  ri->nexthop[0].ifindex = msg->nexthop[0].ifindex;
  ri->nexthop[0].addr = msg->nexthop[0].addr;
  ri->metric = msg->metric;
  ri->pid = msg->pid;
  /* Get tag of the route */
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_IPV4_TAG))
    {
      ri->tag = msg->tag;
      SET_FLAG (ri->flags, OSPF_STATIC_TAG_VALUE);
    }

#ifdef HAVE_VRF_OSPF
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_DOMAIN_INFO))
    {
      SET_FLAG (ri->flags, OSPF_REDIST_VRF_INFO);
      pal_mem_cpy (&ri->domain_info.domain_id, msg->domain_info.domain_id,
                               sizeof (ri->domain_info.domain_id));
      pal_mem_cpy (&ri->domain_info.area_id, &msg->domain_info.area_id,
                               sizeof(struct pal_in4_addr));
      ri->domain_info.r_type = msg->domain_info.r_type;
      ri->domain_info.metric_type_E2 = msg->domain_info.metric_type_E2;

      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ROUTE_CTYPE_OSPF_ROUTER_ID))
         pal_mem_cpy (&ri->domain_info.router_id, &msg->domain_info.router_id,
                                  sizeof(struct pal_in4_addr));

     if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
       {
         char domain_info[48];
         *domain_info = 0;

         zlog_info (ZG, "[OSPF NSM] Domain-id : %s, area_id :%r, "
                    "router_id :%r, LSA type %s: , metric-type-%s",
                    ospf_domain_id_fmt_string (ri->domain_info.domain_id,
                                               domain_info),
                    &ri->domain_info.area_id,
                    &ri->domain_info.router_id,
                    ((ri->domain_info.r_type == 3) ? "Summary" :
                    (ri->domain_info.r_type == 5) ? "AS-External" :
                    (ri->domain_info.r_type == 7) ? "AS-NSSA" : ""),
                    ((ri->domain_info.metric_type_E2 == PAL_FALSE)?
                     ((ri->domain_info.r_type != 3) ? " e1" : " -"):
                     ((ri->domain_info.r_type != 3) ? " e2" : " -")));
       }

    }
#endif /* HAVE_VRF_OSPF */

  if (!CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
    ospf_redist_info_lock (ri);  

  if (CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
    UNSET_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED);

  pal_time_tzcurrent (&ri->tv_update, NULL);

  ospf_redist_info_timer_add (om, type);

  ls_unlock_node (rn);
}

void
ospf_redist_info_delete (struct ospf_vrf *ov, struct nsm_msg_route_ipv4 *msg)
{
  int type = msg->prefixlen == 0 ? APN_ROUTE_DEFAULT : msg->type;
  struct ospf_master *om = ov->iv->vr->proto;
  struct ospf_redist_info *ri;
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, msg->prefixlen, msg->prefix);
  rn = ls_node_lookup (ov->redist_table, &p);
  if (rn == NULL)
    {
      if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
	zlog_info (ZG, "NSM[Redistribute:%s]: %P does not exist, ignore",
		   LOOKUP (ospf_redist_proto_msg, type), &p);
    }
  else
    {
      ri = RN_INFO (rn, RNI_DEFAULT);
      if (!CHECK_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED))
	{
	  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
	    zlog_info (ZG, "NSM[Redistribute:%s]: %P deleted",
		       LOOKUP (ospf_redist_proto_msg, type), &p);

	  SET_FLAG (ri->flags, OSPF_REDIST_INFO_DELETED);

          ospf_redist_info_timer_add (om, type);
	}
      ls_unlock_node (rn);
    }
}


void
ospf_redist_conf_apply (struct ospf *top, int type, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  
  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);
 
  if (rc && REDIST_PROTO_CHECK (rc))
    if (top->t_redist == NULL)
      ospf_redist_map_update_all_by_type (top, type, sproc_id);
}

int
ospf_redist_conf_set (struct ospf *top, int type, int sproc_id)
{
  struct ospf_redist_conf *rc;
  struct ospf_redist_conf *orc = &top->redist[type], *new;
  struct ospf_master *om = top->om;

  /* Lookup for redistribution conf of given pid */
  rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);
  
  if (rc && REDIST_PROTO_CHECK (rc))
    return 0;

  /* Already some instances are redistributed */
  if (!rc)
    {  
       /*  If already some redistribution done or
        *  Some other distribute-list specified on this instance
        *  then create new rc node */
       if (REDIST_PROTO_CHECK (orc) 
          || CHECK_FLAG (orc->flags, OSPF_REDIST_FILTER_LIST))
        {
          for (orc = &top->redist[type]; orc; orc = orc->next)
           if (orc->next == NULL)
             rc = orc;
 
          /* Create a new redistribution conf node */
          new = ospf_redist_conf_new ();
          if (new == NULL)
            return 0;

          rc->next = new;
          new->prev = rc;
        }
      /* No instances redistributed */
      else
        new = orc;
    }
  else
    new = rc;
   
  REDIST_PROTO_SET (new);
  INST_VALUE (new) = sproc_id;

  ospf_asbr_status_update (top);
  ospf_redistribute_timer_add (top, type);

  ospf_nsm_redistribute_set (om->vr, type, top->ov, sproc_id);
  
  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Start",
	       LOOKUP (ospf_redist_proto_msg, type));
  
  return 1;
}

int
ospf_redist_conf_unset (struct ospf *top, int type, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_redist_conf *rc_node = NULL;
  struct ospf_master *om = top->om;
  struct ospf *ospf_top;
  struct listnode *node;
  int count = 0;
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;
#endif /* HAVE_NSSA */

  /* If not first element */
  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if (!rc || !REDIST_PROTO_CHECK (rc))
    return 0;
  
  REDIST_PROTO_UNSET (rc);
  UNSET_FLAG (rc->flags, OSPF_REDIST_METRIC);
  UNSET_FLAG (rc->flags, OSPF_REDIST_METRIC_TYPE);
  UNSET_FLAG (rc->flags, OSPF_REDIST_TAG);

  METRIC_VALUE (rc) = EXTERNAL_METRIC_VALUE_UNSPEC;
  METRIC_TYPE_SET (rc, EXTERNAL_METRIC_TYPE_2);
  REDIST_TAG (rc) = 0;
 
  ospf_redist_map_delete_all_by_type (top, top->redist_table, type, sproc_id);

#ifdef HAVE_NSSA
  if (type != APN_ROUTE_DEFAULT)
    for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
      if ((area = RN_INFO (rn, RNI_DEFAULT)))
	if (IS_AREA_ACTIVE (area))
	  if (IS_AREA_NSSA (area))
	    ospf_redist_map_delete_all_by_type (top, area->redist_table, type, sproc_id);
#endif /* HAVE_NSSA */

  /* Update the ASBR status.  */
  ospf_asbr_status_update (top);

  /*  Any other instance having same redistribution conf  */
  LIST_LOOP(om->ospf, ospf_top, node)
   {
     /* Get the rc_node whose pid is sproc_id */
     rc_node = ospf_redist_conf_lookup_by_inst (ospf_top, type, sproc_id);

     /* Check whether redistribution enabled on that */
     if (rc_node && REDIST_PROTO_CHECK (rc_node))
       {
         count++;
         break;
       }
   }
  
  /* No other instance having same redistribution */
  if (count == 0)
    ospf_nsm_redistribute_unset (om->vr, type, top->ov, sproc_id);
 
  INST_VALUE (rc) = OSPF_REDIST_INSTANCE_VALUE_UNSPEC;        

  /* If not first node and no distribute list specified, 
     then release the corresponding redist conf node */ 
  if (rc->prev != NULL && !CHECK_FLAG (rc->flags, OSPF_REDIST_FILTER_LIST))
    {
      rc->prev->next = rc->next;
      
      if (rc->next)
        rc->next->prev = rc->prev;
      
      ospf_redist_conf_free (rc);
    }

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Finish",
	       LOOKUP (ospf_redist_proto_msg, type));

  return 1;
}

int
ospf_redist_conf_default_set (struct ospf *top, int origin)
{
  int origin_old = top->default_origin;

  if (top->default_origin == origin)
    return 0;

  top->default_origin = origin;
  ospf_redist_conf_set (top, APN_ROUTE_DEFAULT, 0);

  if (origin_old != OSPF_DEFAULT_ORIGINATE_ALWAYS
      && origin == OSPF_DEFAULT_ORIGINATE_ALWAYS)
    ospf_redist_map_default_update (top->redist_table,
				    OSPF_AS_EXTERNAL_LSA, top);

  else if (origin_old == OSPF_DEFAULT_ORIGINATE_ALWAYS
	   && origin != OSPF_DEFAULT_ORIGINATE_ALWAYS)
    ospf_redist_map_default_delete (top->redist_table);

  return 1;
}

int
ospf_redist_conf_default_unset (struct ospf *top)
{
  if (top->default_origin == OSPF_DEFAULT_ORIGINATE_UNSPEC)
    return 0;

  if (top->default_origin == OSPF_DEFAULT_ORIGINATE_ALWAYS)
    ospf_redist_map_default_delete (top->redist_table);

  top->default_origin = OSPF_DEFAULT_ORIGINATE_UNSPEC;
  ospf_redist_conf_unset (top, APN_ROUTE_DEFAULT, 0);

  return 1;
}

int
ospf_redist_conf_unset_all_proto (struct ospf *top)
{
  struct ospf_master *om = top->om;
  struct ospf_redist_conf *rc;
  struct ospf *ospf_top;
  int count = 0;
  struct listnode *node;
#ifdef HAVE_NSSA
  struct ospf_area *area;
  struct ls_node *rn;
#endif /* HAVE_NSSA */
  int type;

  for (type = 0; type < APN_ROUTE_MAX; type++)
   for (rc = &top->redist[type]; rc; rc = rc->next)
    if (REDIST_PROTO_CHECK (rc))
      {
        LIST_LOOP(om->ospf,ospf_top,node)
         if (ospf_redist_conf_lookup_by_inst (ospf_top, type, rc->pid)
             && REDIST_PROTO_CHECK (rc))
           count++;
        
        /*  No other instance using the same redistribution then only
            send redistribute unset message to nsm */
        if (count == 1)
          ospf_nsm_redistribute_unset (om->vr, type, top->ov, rc->pid);
      }
#ifdef HAVE_NSSA
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_NSSA (area))
	if (OSPF_AREA_CONF_CHECK (area, DEFAULT_ORIGINATE))
	  ospf_nsm_redistribute_unset (om->vr, APN_ROUTE_DEFAULT,
				       area->top->ov, 0);
#endif /* HAVE_NSSA */

  return 1;
}

int
ospf_redist_conf_metric_type_set (struct ospf *top, int type, int mtype, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;

  /* If not first node then do lookup  */
  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if ((rc == NULL) || (METRIC_TYPE (rc) == mtype))
    return 0;

  METRIC_TYPE_SET (rc, mtype);
  ospf_redist_conf_apply (top, type, sproc_id);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Metric Type(%d)",
	       LOOKUP (ospf_redist_proto_msg, type),
	       METRIC_TYPE (rc));

  return 1;
}

int
ospf_redist_conf_metric_type_unset (struct ospf *top, int type, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;

  /* If not first node then do lookup  */
  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if (rc == NULL)
    return 0;

  if (!CHECK_FLAG (rc->flags, OSPF_REDIST_METRIC_TYPE))
    return 0;

  METRIC_TYPE_SET (rc, EXTERNAL_METRIC_TYPE_2);
  ospf_redist_conf_apply (top, type, sproc_id);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Metric Type(%d)",
	       LOOKUP (ospf_redist_proto_msg, type),
	       METRIC_TYPE (rc));

  return 1;
}

int
ospf_redist_conf_metric_set (struct ospf *top, int type, int metric, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;

  /* If not first node then do lookup  */
  if (rc->pid != sproc_id) 
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if ((rc == NULL) || (METRIC_VALUE (rc) == metric))
    return 0;

  METRIC_VALUE (rc) = metric;
  SET_FLAG (rc->flags, OSPF_REDIST_METRIC);
  ospf_redist_conf_apply (top, type, sproc_id);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Metric(%lu)",
	       LOOKUP (ospf_redist_proto_msg, type),
	       ospf_metric_value (top, type, OSPF_REDIST_MAP_NSM, sproc_id));

  return 1;
}

int
ospf_redist_conf_metric_unset (struct ospf *top, int type, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;

  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if (rc == NULL)
    return 0;

  if (!CHECK_FLAG (rc->flags, OSPF_REDIST_METRIC))
    return 0;

  METRIC_VALUE (rc) = EXTERNAL_METRIC_VALUE_UNSPEC;
  UNSET_FLAG (rc->flags, OSPF_REDIST_METRIC);
  ospf_redist_conf_apply (top, type, sproc_id);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Metric(%lu)",
	       LOOKUP (ospf_redist_proto_msg, type),
	       ospf_metric_value (top, type, OSPF_REDIST_MAP_NSM, sproc_id));

  return 1;
}

int
ospf_redist_conf_default_metric_set (struct ospf *top, int metric)
{
  struct ospf_master *om = top->om;

  if (top->default_metric == metric
      && CHECK_FLAG (top->config, OSPF_CONFIG_DEFAULT_METRIC))
    return 0;

  SET_FLAG (top->config, OSPF_CONFIG_DEFAULT_METRIC);
  top->default_metric = metric;
  ospf_redist_map_update_all (top);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute]: Set default metric(%lu)",
	       top->default_metric);

  return 1;
}

int
ospf_redist_conf_default_metric_unset (struct ospf *top)
{
  struct ospf_master *om = top->om;

  if (!CHECK_FLAG (top->config, OSPF_CONFIG_DEFAULT_METRIC))
    return 0;

  UNSET_FLAG (top->config, OSPF_CONFIG_DEFAULT_METRIC);
  top->default_metric = 0;
  ospf_redist_map_update_all (top);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute]: Unset default metric");

  return 1;
}

int
ospf_redist_conf_tag_set (struct ospf *top, int type, u_int32_t tag, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;
  u_int32_t old;

  old = 0;

  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if (rc == NULL)
    return 0;

  if (rc)  
    old = REDIST_TAG (rc);

  /* Set the flag if user has configured some tag value 
     (may be zero/non-zero)*/
  SET_FLAG (rc->flags, OSPF_REDIST_TAG);

  REDIST_TAG (rc) = tag;

  if (tag == old)
    return 0;

  ospf_redist_conf_apply (top, type, sproc_id);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Tag(%lu)",
	       LOOKUP (ospf_redist_proto_msg, type), tag);

  return 1;
}

int
ospf_redist_conf_tag_unset (struct ospf *top, int type, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;

  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);
  
  if (rc == NULL)
    return 0;

  if (!CHECK_FLAG (rc->flags, OSPF_REDIST_TAG))
    return 0;

  REDIST_TAG (rc) = 0;
  UNSET_FLAG (rc->flags, OSPF_REDIST_TAG);
  ospf_redist_conf_apply (top, type, sproc_id);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Tag(%lu)",
	       LOOKUP (ospf_redist_proto_msg, type), 0);

  return 1;
}

int
ospf_redist_conf_distribute_list_set (struct ospf *top, int type, int proc_id, char *name)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_redist_conf *orc = NULL;
  struct ospf_redist_conf *ospf_rc = NULL;
  struct ospf_redist_conf *new = NULL;
  struct ospf_master *om = top->om;

  orc = ospf_redist_conf_lookup_by_inst (top, type, proc_id);
  
  /* No corresponding rc for this PID */
  if (orc == NULL)
    {
     /* Check if already some other redistribution or ditsribute-list defined 
        for this pid; if so create a new rc */ 
      if (REDIST_PROTO_CHECK (rc) || CHECK_FLAG (rc->flags, OSPF_REDIST_FILTER_LIST))
        {
          /* Create new redist_conf */
          for (; rc; rc = rc->next)
           if (rc->next == NULL)
             ospf_rc = rc;

          /* Create a new redistribution conf node */
           new = ospf_redist_conf_new ();
           if (new == NULL)
             return 0;

           ospf_rc->next = new;
           new->prev = ospf_rc;
           rc = new;
        }
        
    }
  /* Already either the redistribution or 
     distribute-list defined on this PID */
  else 
   rc = orc; 

  INST_VALUE (rc) = proc_id;  
  SET_FLAG (rc->flags, OSPF_REDIST_FILTER_LIST);

  /* Lookup access-list for distribute-list. */
  DIST_LIST (rc) = access_list_lookup (om->vr, AFI_IP, name);

  /* Clear previous distribute-name. */
  if (DIST_NAME (rc))
    XFREE (MTYPE_OSPF_DESC, DIST_NAME (rc));

  /* Set distribute-name. */
  DIST_NAME (rc) = XSTRDUP (MTYPE_OSPF_DESC, name);

  /* If access-list have been set, schedule update timer. */
  if (DIST_LIST (rc))
    ospf_redistribute_timer_add (top, type);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Set distribute-list(%s)",
	       LOOKUP (ospf_redist_proto_msg, type), name);

  return 1;
}

int
ospf_redist_conf_distribute_list_unset (struct ospf *top, int type, int proc_id, char *name)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;
 
  rc = ospf_redist_conf_lookup_by_inst (top, type, proc_id);
  
  if (!rc)
    return 0;
 
  UNSET_FLAG (rc->flags, OSPF_REDIST_FILTER_LIST);

  /* Schedule update timer. */
  if (DIST_LIST (rc))
    ospf_redistribute_timer_add (top, type);

  /* Unset distribute-list. */
  DIST_LIST (rc) = NULL;

  /* Clear distribute-name. */
  if (DIST_NAME (rc))
    XFREE (MTYPE_OSPF_DESC, DIST_NAME (rc));

  DIST_NAME (rc) = NULL;

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Unset distribute-list(%s)",
	       LOOKUP (ospf_redist_proto_msg, type), name);

  if (!REDIST_PROTO_CHECK (rc))
    {
      INST_VALUE (rc) = OSPF_REDIST_INSTANCE_VALUE_UNSPEC;
      if (rc->prev != NULL)
        {
          rc->prev->next = rc->next;
          if (rc->next)
            rc->next->prev = rc->prev;
          ospf_redist_conf_free (rc);
        }
    }
  
      
  return 1;
}

int
ospf_redist_conf_route_map_set (struct ospf *top, int type, char *name, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;

  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if (rc == NULL)
    return 0;
  
  if (RMAP_NAME (rc))
    XFREE (MTYPE_OSPF_DESC, RMAP_NAME (rc));

  RMAP_NAME (rc) = XSTRDUP (MTYPE_OSPF_DESC, name);
  RMAP_MAP (rc) = route_map_lookup_by_name (om->vr, name);

  SET_FLAG (rc->flags, OSPF_REDIST_ROUTE_MAP);
  ospf_redistribute_timer_add (top, type);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Set route-map(%s)",
	       LOOKUP (ospf_redist_proto_msg, type), name);

  return 1;
}

int
ospf_redist_conf_route_map_unset (struct ospf *top, int type, int sproc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];
  struct ospf_master *om = top->om;

  if (rc->pid != sproc_id)
    rc = ospf_redist_conf_lookup_by_inst (top, type, sproc_id);

  if (!rc)
    return 0;

  if (RMAP_NAME (rc))
    XFREE (MTYPE_OSPF_DESC, RMAP_NAME (rc));

  RMAP_NAME (rc) = NULL;
  RMAP_MAP (rc) = NULL;

  UNSET_FLAG (rc->flags, OSPF_REDIST_ROUTE_MAP);
  ospf_redistribute_timer_add (top, type);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[Redistribute:%s]: Unset route-map",
	       LOOKUP (ospf_redist_proto_msg, type));

  return 1;
}

#ifdef HAVE_NSSA
int
ospf_redist_conf_nssa_no_redistribute_set (struct ospf_area *area)
{
  if (OSPF_AREA_CONF_CHECK (area, NO_REDISTRIBUTION))
    return 0;

  OSPF_AREA_CONF_SET (area, NO_REDISTRIBUTION);

  /* Schedule the redistribute timer.  */
  ospf_nssa_redistribute_timer_add_all_proto (area);

  return 1;
}

int
ospf_redist_conf_nssa_no_redistribute_unset (struct ospf_area *area)
{
  if (!OSPF_AREA_CONF_CHECK (area, NO_REDISTRIBUTION))
    return 0;

  OSPF_AREA_CONF_UNSET (area, NO_REDISTRIBUTION);
  
  pal_assert (area != NULL);

  /* Schedule the redistribute timer.  */
  ospf_nssa_redistribute_timer_add_all_proto (area);

  return 1;
}

int
ospf_redist_conf_nssa_default_set (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;

  if (OSPF_AREA_CONF_CHECK (area, DEFAULT_ORIGINATE))
    return 0;

  OSPF_AREA_CONF_SET (area, DEFAULT_ORIGINATE);

  area->conf.default_config = PAL_TRUE;

  /* Notify to the NSM.  */
  ospf_nsm_redistribute_set (om->vr, APN_ROUTE_DEFAULT, area->top->ov, 0);

  /* Schedule the redistribute timer.  */
  ospf_nssa_redistribute_timer_add (area, APN_ROUTE_DEFAULT);

  /* Update the ASBR status.  */
  ospf_asbr_status_update (area->top);

  /* Update the default summary-LSA.  */
  ospf_abr_ia_update_default_to_area (area);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[NSSA(%r):Redistribute:%s]: Start", &area->area_id,
	       LOOKUP (ospf_redist_proto_msg, APN_ROUTE_DEFAULT));

  return 1;
}

int
ospf_redist_conf_nssa_default_unset (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;

  if (!OSPF_AREA_CONF_CHECK (area, DEFAULT_ORIGINATE))
    return 0;

  area->conf.metric = OSPF_NSSA_METRIC_DEFAULT;
  area->conf.metric_type = OSPF_NSSA_METRIC_TYPE_DEFAULT;

  OSPF_AREA_CONF_UNSET (area, METRIC);
  OSPF_AREA_CONF_UNSET (area, METRIC_TYPE);
  OSPF_AREA_CONF_UNSET (area, DEFAULT_ORIGINATE);
  if (area == NULL)
    return 1;

  area->conf.default_config = PAL_FALSE;

  /* Update the default summary-LSA.  */
  ospf_abr_ia_update_default_to_area (area);

  /* Remove the default route.  */
  ospf_redist_map_delete_all_by_type (area->top, area->redist_table,
				      APN_ROUTE_DEFAULT, 0);

  ospf_nssa_redistribute_timer_add (area, APN_ROUTE_DEFAULT);

  /* Update the ASBR status.  */
  ospf_asbr_status_update (area->top);

  /* Notify to the NSM.  */
  ospf_nsm_redistribute_unset (om->vr, APN_ROUTE_DEFAULT, area->top->ov, 0);

  if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
    zlog_info (ZG, "NSM[NSSA(%r):Redistribute:%s]: Finish", &area->area_id,
	       LOOKUP (ospf_redist_proto_msg, APN_ROUTE_DEFAULT));

  return 1;
}

int
ospf_redist_conf_nssa_default_metric_set (struct ospf_area *area, int metric)
{
  if (area->conf.metric == metric)
    return 0;

  area->conf.metric = metric;

  if (metric != OSPF_NSSA_METRIC_DEFAULT)
    OSPF_AREA_CONF_SET (area, METRIC);
  else
    OSPF_AREA_CONF_UNSET (area, METRIC);

  if (area == NULL)
    return 1;

  /* Schedule the redistribute timer.  */
  ospf_nssa_redistribute_timer_add (area, APN_ROUTE_DEFAULT);

  /* Update the ASBR status.  */
  ospf_asbr_status_update (area->top);

  return 1;
}

int
ospf_redist_conf_nssa_default_metric_type_set (struct ospf_area *area,
					       int mtype)
{
  if (area->conf.metric_type == mtype)
    return 0;

  area->conf.metric_type = mtype;

  if (mtype == EXTERNAL_METRIC_TYPE_1)
    OSPF_AREA_CONF_SET (area, METRIC_TYPE);
  else
    OSPF_AREA_CONF_UNSET (area, METRIC_TYPE);

  if (area == NULL)
    return 1;

  /* Schedule the redistribute timer.  */
  ospf_nssa_redistribute_timer_add (area, APN_ROUTE_DEFAULT);

  /* Update the ASBR status.  */
  ospf_asbr_status_update (area->top);

  return 1;
}
#endif /* HAVE_NSSA */


/* OSPF NSM APIs.  */
int
ospf_nsm_connected_check (struct apn_vrf *iv, struct prefix_ipv4 *p)
{
  struct interface *ifp;

  ifp = ifv_lookup_by_prefix (&iv->ifv, (struct prefix *)p);
  if (ifp != NULL)
    return 1;

  return 0;
}

void
ospf_nsm_add (struct ospf *top, struct ls_prefix *lp, struct ospf_route *route)
{
  struct ospf_master *om = top->om;
  struct ospf_path *path = route->selected;
  struct prefix_ipv4 *p = (struct prefix_ipv4 *)lp;
  struct apn_vrf *iv = top->ov->iv;
  struct apn_vr *vr = om->vr;
  struct nsm_msg_route_ipv4 msg;
  struct ospf_nexthop *nh;
  u_int8_t num = 0;
  int count = 0;
  int code = 0;
  vector vec;
  int i;
#ifdef HAVE_VRF_OSPF
  u_int16_t type;
#endif /*HAVE_VRF_OSPF */

#ifdef HAVE_RESTART
  /* "Graceful Restart". */
  if (IS_GRACEFUL_RESTART (top))
    return;
#endif /* HAVE_RESTART */

  if (p->prefixlen == IPV4_MAX_BITLEN)
    if (ospf_nsm_connected_check (iv, p))
      return;

  /* Add OSPF route to NSM. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_ipv4));
  SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_ADD);
  msg.type = APN_ROUTE_OSPF;

  /* Distance value. */
  msg.distance = route->distance;

  /* Metric.  */
  if (path->type == OSPF_PATH_EXTERNAL
      && CHECK_FLAG (path->flags, OSPF_PATH_TYPE2))
    msg.metric = path->type2_cost;
  else
    msg.metric = OSPF_PATH_COST (path);

  /* Prefix.  */
  msg.prefixlen = lp->prefixlen;
  pal_mem_cpy (&msg.prefix, lp->prefix, sizeof (struct pal_in4_addr));
  
  /* Get the actual nexthop vector.  */
  vec = ospf_path_nexthop_vector (path);
  if (vector_count (vec) == 0)
    {
      /* Delete the route.  */
      UNSET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_ADD);
      nsm_client_send_route_ipv4_bundle (vr->id, iv->id, om->zg->nc, &msg);
      vector_free (vec);
      return;
    }

  /* Set nexthop address.  */
  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP);

  for (i = 0; i < vector_max (vec); i++)
    if ((nh = vector_slot (vec, i)))
      {
        num++;
        if (num == OSPF_MAXIMUM_NUM_OF_NEXTHOPS_PER_ROUTE)
          break;
      }

  NSM_MSG_NEXTHOP4_NUM_SET (msg, num);

  for (i = 0; i < vector_max (vec); i++)
    if ((nh = vector_slot (vec, i)))
      {
	code = ospf_path_code (path, nh);

	if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_CONNECTED)
	    && !CHECK_FLAG (nh->flags, OSPF_NEXTHOP_UNNUMBERED))
	  NSM_MSG_NEXTHOP4_ADDR_SET (msg, count, nh->nbr_id);

	if (nh->oi)
	  NSM_MSG_NEXTHOP4_IFINDEX_SET (msg, count, nh->oi->u.ifp->ifindex);

        count++;
        /* To limit the maximum number of nexthops per route is 255 */
        if (count == OSPF_MAXIMUM_NUM_OF_NEXTHOPS_PER_ROUTE)
          break;
      }

   /* Set the Tag value */
   if (path->tag)
     {
       NSM_SET_CTYPE(msg.cindex, NSM_ROUTE_CTYPE_IPV4_TAG);
       msg.tag = path->tag;
     }

  /* Sub-type is set here. */
  switch (code)
    {
    case OSPF_PATH_CODE_INTER_AREA:
      msg.sub_type = APN_ROUTE_OSPF_IA;
      break;
    case OSPF_PATH_CODE_E1:
      msg.sub_type = APN_ROUTE_OSPF_EXTERNAL_1;
      break;
    case OSPF_PATH_CODE_E2:
      msg.sub_type = APN_ROUTE_OSPF_EXTERNAL_2;
      break;
    case OSPF_PATH_CODE_N1:
      msg.sub_type = APN_ROUTE_OSPF_NSSA_1;
      break;
    case OSPF_PATH_CODE_N2:
      msg.sub_type = APN_ROUTE_OSPF_NSSA_2;
      break;
    default:
      msg.sub_type = 0;
      break;
    }
 
  /* instance id */
  NSM_SET_CTYPE(msg.cindex,NSM_ROUTE_CTYPE_IPV4_PROCESS_ID);
  msg.pid = top->ospf_id;

#ifdef HAVE_VRF_OSPF
  /*Send the domain info only when the ospf instance is VRF enabled */
  if (CHECK_FLAG (top->ov->flags, OSPF_VRF_ENABLED))
   {
     NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_DOMAIN_INFO);

     if (CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY))
       {
         type = pal_hton16(top->pdomain_id->type);
         pal_mem_cpy (msg.domain_info.domain_id, &type, 2);

         pal_mem_cpy (&msg.domain_info.domain_id[2],&top->pdomain_id->value[0],
                         OSPF_DOMAIN_ID_VLENGTH);
       }
     else
       {
         /* If domain_id not configured the default value is NULL*/
         pal_mem_set (&msg.domain_info.domain_id, 0,
                       sizeof (struct ospf_vrf_domain_id));
       }

     /* ospf LSA type */
     switch (code)
       {
       case OSPF_PATH_CODE_INTER_AREA:
         if (CHECK_FLAG (path->flags, OSPF_PATH_ASBR))
           {
             /* For type 4 LSA's */
             msg.domain_info.r_type = OSPF_SUMMARY_LSA_ASBR;
           }
         else
           msg.domain_info.r_type = OSPF_SUMMARY_LSA;
         break;

       case OSPF_PATH_CODE_INTRA_AREA:
         msg.domain_info.r_type = OSPF_SUMMARY_LSA;
         break;

       case OSPF_PATH_CODE_E1:
       case OSPF_PATH_CODE_E2:
         msg.domain_info.r_type = OSPF_AS_EXTERNAL_LSA;
         break;

       case OSPF_PATH_CODE_N1:
       case OSPF_PATH_CODE_N2:
         msg.domain_info.r_type = OSPF_AS_NSSA_LSA;
         break;

       default:
         msg.domain_info.r_type = 0;
         break;
       }

     /* ospf area id */
     pal_mem_cpy (&msg.domain_info.area_id, &path->area_id,
                       sizeof (struct pal_in4_addr));

     /* Metric type RFC 4577 (section 4.2.6)
        Setting the least significant bit in the
        field indicates that the route carries a type 2 metric */
     if (code == OSPF_PATH_CODE_E2)
       msg.domain_info.metric_type_E2 = PAL_TRUE;
     else 
       msg.domain_info.metric_type_E2 = PAL_FALSE;

     /* OSPF router-ID configured */
     if (CHECK_FLAG (top->config, OSPF_CONFIG_ROUTER_ID))
       {
         NSM_SET_CTYPE(msg.cindex, NSM_ROUTE_CTYPE_OSPF_ROUTER_ID); 
         pal_mem_cpy (&msg.domain_info.router_id, &top->router_id_config,
                            sizeof (struct pal_in4_addr));
       }
     if (IS_DEBUG_OSPF (nsm, NSM_REDISTRIBUTE))
       {
         char domain_info[48];
         *domain_info = 0;
         zlog_info (ZG, "[OSPF NSM] Domain-id : %s, area_id :%r, "
                    "router_id :%r, LSA type %s: , metric-type-%s",
                    ospf_domain_id_fmt_string (msg.domain_info.domain_id,
                                               domain_info),
                    &msg.domain_info.area_id,
                    &msg.domain_info.router_id,
                    ((msg.domain_info.r_type == 3) ? "Summary" :
                    (msg.domain_info.r_type == 5) ? "AS-External" :
                    (msg.domain_info.r_type == 7) ? "AS-NSSA" : ""),
                    ((msg.domain_info.metric_type_E2 == PAL_FALSE)?
                     ((msg.domain_info.r_type != 3) ? "e1" : "-"):
                     ((msg.domain_info.r_type != 3) ? "e2" : "-")));
       }
  }
#endif /* HAVE_VRF_OSPF */
  /* Send message.  */
  nsm_client_send_route_ipv4_bundle (vr->id, iv->id, om->zg->nc, &msg);
  NSM_MSG_NEXTHOP4_FINISH (msg);
  vector_free (vec);
}

void
ospf_nsm_delete (struct ospf *top, struct ls_prefix *lp)
{
  struct ospf_master *om = top->om;
  struct apn_vrf *iv = top->ov->iv;
  struct apn_vr *vr = om->vr;
  struct nsm_msg_route_ipv4 msg;

#ifdef HAVE_RESTART
  /* "Graceful Restart". */
  if (IS_GRACEFUL_RESTART (top))
    return;
#endif /* HAVE_RESTART */
  
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_ipv4));
  msg.type = APN_ROUTE_OSPF;
  msg.prefixlen = lp->prefixlen;
  pal_mem_cpy (&msg.prefix, lp->prefix, sizeof (struct pal_in4_addr));

  /* Sending ospf process ID to nsm  */ 
  NSM_SET_CTYPE(msg.cindex,NSM_ROUTE_CTYPE_IPV4_PROCESS_ID);  
  msg.pid = top->ospf_id;

  /* Send message.  */
  nsm_client_send_route_ipv4_bundle (vr->id, iv->id, om->zg->nc, &msg);
}

void
ospf_nsm_add_discard (struct ospf *top, struct ls_prefix *lp)
{
  struct ospf_master *om = top->om;
  struct apn_vrf *iv = top->ov->iv;
  struct apn_vr *vr = top->om->vr;
  struct nsm_msg_route_ipv4 msg;

#ifdef HAVE_RESTART
  /* "Graceful Restart". */
  if (IS_GRACEFUL_RESTART (top))
    return;
#endif /* HAVE_RESTART */

  /* Clear message. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_ipv4));

  msg.flags = 0;
  SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_ADD);
  SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_BLACKHOLE);
  msg.type = APN_ROUTE_OSPF;
  msg.distance = APN_DISTANCE_OSPF;
  msg.metric = 0;
  msg.prefixlen = lp->prefixlen;
  pal_mem_cpy (&msg.prefix, lp->prefix, sizeof (struct pal_in4_addr));

  /* Set nexthop address.  */
  msg.cindex = 0;
  NSM_SET_CTYPE (msg.cindex, NSM_ROUTE_CTYPE_IPV4_NEXTHOP);
  msg.nexthop_num = 1;
  msg.nexthop[0].ifindex = 0;
  msg.nexthop[0].addr = IPv4LoopbackAddress;

  /* Send message.  */
  nsm_client_send_route_ipv4_bundle (vr->id, iv->id, om->zg->nc, &msg);
}

void
ospf_nsm_delete_discard (struct ospf *top, struct ls_prefix *lp)
{
  struct ospf_master *om = top->om;
  struct apn_vrf *iv = top->ov->iv;
  struct apn_vr *vr = top->om->vr;
  struct nsm_msg_route_ipv4 msg;

#ifdef HAVE_RESTART
  /* "Graceful Restart". */
  if (IS_GRACEFUL_RESTART (top))
    return;
#endif /* HAVE_RESTART */

  /* Clear message. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_route_ipv4));

  SET_FLAG (msg.flags, NSM_MSG_ROUTE_FLAG_BLACKHOLE);
  msg.type = APN_ROUTE_OSPF;
  msg.prefixlen = lp->prefixlen;
  pal_mem_cpy (&msg.prefix, lp->prefix, sizeof (struct pal_in4_addr));

  /* Send message.  */
  nsm_client_send_route_ipv4_bundle (vr->id, iv->id, om->zg->nc, &msg);
}

void
ospf_nsm_igp_shortcut_route_add (struct ospf *top, struct ls_prefix *lp,
				 struct ospf_route *route)
{
#ifdef HAVE_MPLS
  struct ospf_master *om = top->om;
  struct apn_vr *vr = om->vr;
  struct apn_vrf *iv = top->ov->iv;
  struct ospf_path *path = route->selected;
  struct prefix *p = (struct prefix *)lp;
  struct nsm_msg_igp_shortcut_route msg;
  struct ospf_igp_shortcut_lsp *t_lsp;

  if (! path->t_lsp)
    return;

  /* Do not add tunnel endpoint to igp-shortcut route table */
  t_lsp = path->t_lsp;
  if (IPV4_ADDR_SAME (&t_lsp->t_endp, &p->u.prefix4))
    return;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_igp_shortcut_route));

  msg.action = NSM_MSG_IGP_SHORTCUT_ADD; 
  msg.tunnel_id = path->t_lsp->tunnel_id;
  pal_mem_cpy (&msg.fec, p, sizeof (struct ls_prefix));
  IPV4_ADDR_COPY (&msg.t_endp, &t_lsp->t_endp);

  nsm_client_send_igp_shortcut_route (vr->id, iv->id, om->zg->nc, &msg);
#endif /* HAVE_MPLS */
}

void
ospf_nsm_igp_shortcut_route_del (struct ospf *top, struct ls_prefix *lp,
				 struct ospf_route *route, 
				 struct ospf_path *path)
{
#ifdef HAVE_MPLS
  struct ospf_master *om = top->om;
  struct apn_vr *vr = om->vr;
  struct apn_vrf *iv = top->ov->iv;
  struct prefix *p = (struct prefix *)lp;
  struct nsm_msg_igp_shortcut_route msg;
  struct ospf_igp_shortcut_lsp *t_lsp;

  if (! path->t_lsp)
    return;

  /* Do not delete tunnel endpoint to igp-shortcut route table */
  t_lsp = path->t_lsp;
  if (IPV4_ADDR_SAME (&t_lsp->t_endp, &p->u.prefix4))
    return;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_igp_shortcut_route));

  msg.action = NSM_MSG_IGP_SHORTCUT_DELETE; 
  msg.tunnel_id = path->t_lsp->tunnel_id;
  pal_mem_cpy (&msg.fec, p, sizeof (struct ls_prefix));
  IPV4_ADDR_COPY (&msg.t_endp, &t_lsp->t_endp);

  nsm_client_send_igp_shortcut_route (vr->id, iv->id, om->zg->nc, &msg);
#endif /* HAVE_MPLS */
}

#ifdef HAVE_RESTART
void
ospf_nsm_preserve_option_reason_set (struct ospf_master *om, struct stream *s)
{
  stream_putw (s, OSPF_NSM_RESTART_REASON_TLV);
  stream_putw (s, 1);
  stream_putc (s, om->restart_reason);
  stream_putc (s, 0);
  stream_putc (s, 0);
  stream_putc (s, 0);
}

void
ospf_nsm_preserve_option_crypt_seqnum_set (struct ospf_master *om,
					   struct stream *s)
{
  struct ospf_interface *oi;
  struct ls_node *rn;
  unsigned long p;
  int length = 0;
  int count = 0;

  for (rn = ls_table_top (om->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if ((ospf_auth_type (oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
	count++;

  if (count == 0)
    return;

  stream_putw (s, OSPF_NSM_RESTART_CRYPT_SEQNUM_TLV);
  p = stream_get_putp (s);
  stream_putw (s, 0);

  for (rn = ls_table_top (om->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if ((ospf_auth_type (oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
	{
	  stream_put_in_addr (s, &oi->ident.address->u.prefix4);
	  stream_putl (s, oi->u.ifp->ifindex);
	  stream_putl (s, oi->crypt_seqnum);
	  length += 12;
	}
  stream_putw_at (s, p, length);
}

void
ospf_nsm_preserve_option_vlink_crypt_seqnum_set (struct ospf_master *om,
						 struct stream *s)
{
  struct ospf *top;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
  struct listnode *node;
  unsigned long p;
  int length = 0;
  int count = 0;

  LIST_LOOP (om->ospf, top, node)
    for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
      if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
	if ((ospf_auth_type (vlink->oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
	  count++;

  if (count == 0)
    return;

  stream_putw (s, OSPF_NSM_RESTART_VLINK_CRYPT_SEQNUM_TLV);
  p = stream_get_putp (s);
  stream_putw (s, 0);

  LIST_LOOP (om->ospf, top, node)
    for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
      if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
	if ((ospf_auth_type (vlink->oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
	  {
	    stream_putl (s, top->ospf_id);
	    stream_put_in_addr (s, &vlink->area_id);
	    stream_put_in_addr (s, &vlink->peer_id);
	    stream_putl (s, vlink->oi->crypt_seqnum);
	    length += 16;
	  }

  stream_putw_at (s, p, length);
}
#ifdef HAVE_OSPF_MULTI_AREA
void
ospf_nsm_preserve_option_multi_area_link_crypt_seqnum_set (
                                 struct ospf_master *om, struct stream *s)
{
  struct ospf *top;
  struct ospf_multi_area_link *mlink;
  struct ls_node *rn;
  struct listnode *node;
  unsigned long p;
  int length = 0;
  int count = 0;

  LIST_LOOP (om->ospf, top, node)
    for (rn = ls_table_top (top->multi_area_link_table); rn; 
         rn = ls_route_next (rn))
      if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
        if (mlink->oi) 
          if ((ospf_auth_type (mlink->oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
            count++;

  if (count == 0)
    return;

  stream_putw (s, OSPF_NSM_RESTART_MULTI_AREA_LINK_CRYPT_SEQNUM_TLV);
  p = stream_get_putp (s);
  stream_putw (s, 0);

  LIST_LOOP (om->ospf, top, node)
    for (rn = ls_table_top (top->multi_area_link_table); rn; 
         rn = ls_route_next (rn))
      if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
        if (mlink->oi)
          if ((ospf_auth_type (mlink->oi)) == OSPF_AUTH_CRYPTOGRAPHIC)
            {
              stream_putl (s, top->ospf_id);
              stream_put_in_addr (s, &mlink->area_id);
              stream_put_in_addr (s, &mlink->nbr_addr);
              stream_putl (s, mlink->oi->crypt_seqnum);
              length += 16;
            }
  stream_putw_at (s, p, length);
}
#endif /* HAVE_OSPF_MULTI_AREA */

void
ospf_nsm_preserve_options_set (struct ospf_master *om, struct stream *s)
{
  /* Set Restart Reason TLV. */
  ospf_nsm_preserve_option_reason_set (om, s);

  /* Set Crypt Sequence Number TLV. */
  ospf_nsm_preserve_option_crypt_seqnum_set (om, s);

  /* Set Crypt Sequence Number for VLINK TLV. */
  ospf_nsm_preserve_option_vlink_crypt_seqnum_set (om, s);

#ifdef HAVE_OSPF_MULTI_AREA
  /* Set Crypt Sequence Number for Multi area link TLV. */
  ospf_nsm_preserve_option_multi_area_link_crypt_seqnum_set (om, s);
#endif /* HAVE_OSPF_MULTI_AREA */
}

#define OSPF_NSM_PRESERVE_OPTION_SIZE_MAX	4096

void
ospf_nsm_preserve_set (struct ospf_master *om)
{
  int period = om->grace_period ? om->grace_period : OSPF_GRACE_PERIOD_DEFAULT;
  struct stream *s;

  if (om->restart_reason != OSPF_RESTART_REASON_UNKNOWN)
    {
      s = stream_new (OSPF_NSM_PRESERVE_OPTION_SIZE_MAX);
      ospf_nsm_preserve_options_set (om, s);

      lib_api_client_send_preserve_with_val (om->zg, period, STREAM_DATA (s),
					     stream_get_endp (s));

      stream_free (s);
    }
  else
    lib_api_client_send_preserve (om->zg, period);
}

void
ospf_nsm_preserve_unset (struct ospf_master *om)
{
  lib_api_client_send_preserve (om->zg, 0);
}

void
ospf_nsm_flush (struct ospf_master *om)
{
  if (om->zg->nc != NULL)
    nsm_client_flush_route_ipv4 (om->zg->nc);
}

void
ospf_nsm_stale_remove (struct ospf_master *om)
{
  lib_api_client_send_stale_remove (om->zg, AFI_IP, SAFI_UNICAST);
}
#endif /* HAVE_RESTART */

/* Turn on debug out put.  */
void
ospf_nsm_debug_set (struct ospf_master *om)
{
  if (om->zg->nc != NULL)
    om->zg->nc->debug = 1;
}

/* Turn off debug output.  */
void
ospf_nsm_debug_unset (struct ospf_master *om)
{
  if (om->zg->nc != NULL)
    om->zg->nc->debug = 0;
}

/* NSM message handlers.  */
void
ospf_nsm_recv_service_route_sync (struct lib_globals *zg)
{
#ifdef HAVE_NSSA
  struct ospf_area *area;
#endif /* HAVE_NSSA */
  struct ospf_route *route;
  struct ospf *top;
  struct ospf_vrf *ov;
  struct apn_vrf *iv;
  struct apn_vr *vr;
  struct listnode *node;
  struct ls_table *rt;
  struct ls_node *rn;
  struct ospf_redist_conf *rc;
  int i, j, proto;

  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      {
        /* Set VR Context */
        LIB_GLOB_SET_VR_CONTEXT (ZG, vr);

        for (j = 0; j < vector_max (vr->vrf_vec); j++)
	  if ((iv = vector_slot (vr->vrf_vec, j)))
	    {
	      ov = iv->proto;

	      /* Reset the redistribute count.  */
	      for (proto = 0; proto < APN_ROUTE_MAX; proto++)
	        ov->redist_count[proto] = 0;

	      LIST_LOOP (ov->ospf, top, node)
	        {
		  /* Update RIB.  */
		  rt = top->rt_network;
		  for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
		    if ((route = RN_INFO (rn, RNI_DEFAULT)))
		      if (route->selected)
		        OSPF_NSM_ADD (top, rn->p, route, route->selected);

		  /* Set redistribution.  */
		  for (proto = 0; proto < APN_ROUTE_MAX; proto++)
                    for (rc = &top->redist[proto]; rc; rc = rc->next)
		      if (REDIST_PROTO_CHECK (rc))
		        ospf_nsm_redistribute_set (vr, proto, ov, rc->pid);

#ifdef HAVE_NSSA
		  /* Set redistribution.  */
		  rt = top->area_table;
		  for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
		    if ((area = RN_INFO (rn, RNI_DEFAULT)))
		      if (OSPF_AREA_CONF_CHECK (area, DEFAULT_ORIGINATE))
		        ospf_nsm_redistribute_set (vr, APN_ROUTE_DEFAULT, ov, 0);
#endif /* HAVE_NSSA */
	        }
	    }
      }
  vr = apn_vr_get_privileged (ZG);
  /* Set VR context to default */
  LIB_GLOB_SET_VR_CONTEXT (ZG, vr);
}

/* Service reply is received.  If client has special request to NSM,
   we should check this reply.  */
int
ospf_nsm_recv_service (struct nsm_msg_header *header, void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_service *service = message;
#ifdef HAVE_MPLS
  struct ospf_vrf *ov;
  struct ospf_master *omvr;
#endif /* HAVE_MPLS */
#ifdef HAVE_RESTART
  struct ospf_master *om = ospf_master_lookup_default (ZG);
  afi_t afi;
  safi_t safi;

  if (om == NULL)
    return -1;
#endif /* HAVE_RESTART */

  /* NSM server assign client ID.  */
  nsm_client_id_set (nc, service);

  if (nc->debug)
    nsm_service_dump (nc->zg, service);

  /* Check protocol version.  */
  if (service->version < NSM_PROTOCOL_VERSION_1)
    {
      zlog_err (nc->zg, "NSM server protocol version error");
      return -1;
    }

  /* Check NSM service bits.  */
  if ((service->bits & nc->service.bits) != nc->service.bits)
    {
      zlog_err (nc->zg, "NSM service is not sufficient");
      return -1;
    }

#ifdef HAVE_RESTART
  /* Copy graceful restart portion. */
  nc->service.cindex = service->cindex;
  if (NSM_CHECK_CTYPE (nc->service.cindex, NSM_SERVICE_CTYPE_RESTART))
    for (afi = AFI_IP; afi < AFI_MAX; afi++)
      for (safi = SAFI_UNICAST; safi < SAFI_MAX; safi++)
	nc->service.restart[afi][safi] = service->restart[afi][safi];

  /* Check if Graceful restart. */
  if (NSM_CHECK_CTYPE (nc->service.cindex, NSM_SERVICE_CTYPE_GRACE_EXPIRES))
    {
      nc->service.grace_period = service->grace_period;
      SET_FLAG (om->flags, OSPF_GLOBAL_FLAG_RESTART);
      OSPF_TIMER_ON (om->t_restart, ospf_restart_timer,
		     om, nc->service.grace_period);
    }

  /* Check if option is set. */
  if (NSM_CHECK_CTYPE (nc->service.cindex, NSM_SERVICE_CTYPE_RESTART_OPTION))
    {
      om->restart_option = stream_new (service->restart_length);
      pal_mem_cpy (STREAM_DATA (om->restart_option),
		   service->restart_val, service->restart_length);
      ospf_restart_option_tlv_parse (om, om->restart_option);
    }

  /* Reset the preserve time to the NSM.  */
  if (CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_GRACE_PERIOD))
    ospf_nsm_preserve_set (om);
#endif /* HAVE_RESTART */

  /* Now clent is up.  */
  nch->up = 1;

  /* If this is route service request connection, send redistribute request
     and OSPF routes.  */
  if (NSM_CHECK_CTYPE (nch->service.bits, NSM_SERVICE_ROUTE))
    ospf_nsm_recv_service_route_sync (nc->zg);

#ifdef HAVE_MPLS
  omvr = ospf_master_lookup_by_id (nc->zg, header->vr_id);
  if (omvr)
    {
      ov = ospf_vrf_lookup_by_id (omvr, header->vrf_id);

      if (ov)
	ospf_igp_shortcut_lsp_clean (ov);
    }
#endif /* HAVE_MPLS */

  return 0;
}

int
ospf_nsm_if_add (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;

  /* Call notifiers.  */
  ospf_call_notifiers (om, OSPF_NOTIFY_LINK_NEW, ifp, NULL);

  /* Add the passive-interface configuration.  */
  ospf_passive_if_add_by_interface (om, ifp);

  return 0;
}

int
ospf_nsm_if_delete (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
   {
     if ((oi = RN_INFO (rn, RNI_DEFAULT)))
       {
#ifdef HAVE_OSPF_MULTI_INST
         /* If multiple instances are running on this interface */
         if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
           {
             LIST_LOOP(om->ospf, top_alt, node)
               {
                 if (oi->top != top_alt)
                   {
                     /* Delete oi from interface table of each instance */
                     oi_other = ospf_if_entry_match (top_alt, oi);
                     if (oi_other)
                       ospf_if_delete (oi_other);
                   }
               }
           }
#endif /* HAVE_OSPF_MULTI_INST */
        ospf_if_delete (oi);
       }
     else
       ospf_if_params_delete_default (om, ifp->name);
   }

  ls_unlock_node (rn_tmp);

  /* Call notifiers.  */
  ospf_call_notifiers (om, OSPF_NOTIFY_LINK_DEL, ifp, NULL);

  /* Remove the passive-interface configuration.  */
  ospf_passive_if_delete_by_interface (om, ifp);

  return 0;
}

int
ospf_nsm_if_vrf_bind (struct interface *ifp)
{
  return 0;
}

int
ospf_nsm_if_vrf_unbind (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
       {
#ifdef HAVE_OSPF_MULTI_INST
         /* If multiple instances are running on this interface */
         if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
           {
             LIST_LOOP(om->ospf, top_alt, node)
               {
                 if (oi->top != top_alt)
                   {
                     /* Delete oi from interface table of each instance */
                     oi_other = ospf_if_entry_match (top_alt, oi);
                     if (oi_other)
                       ospf_if_delete (oi_other);
                   }
               }
           }
#endif /* HAVE_OSPF_MULTI_INST */
         ospf_if_delete (oi);
       }

  ls_unlock_node (rn_tmp);

  return 0;
}

int
ospf_nsm_if_up (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct listnode *node = NULL;
  struct ospf *top = NULL;
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
#ifdef HAVE_OSPF_MULTI_INST
        /* If multiple instances are running on this interface */
        if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
          LIST_LOOP(om->ospf, top_alt, node_alt)
            {
              if (oi->top != top_alt)
                {
                  /* Get oi from interface table of each instance */
                  oi_other = ospf_if_entry_match (top_alt, oi);
                  if (oi_other)
                    ospf_if_up (oi_other);
                }
            }
#endif /* HAVE_OSPF_MULTI_INST */
        ospf_if_up (oi);
      }

  ls_unlock_node (rn_tmp);

#ifdef HAVE_OSPF_MULTI_AREA
  LIST_LOOP (om->ospf, top, node)
    {
      for (rn = ls_table_top (top->multi_area_link_table); rn;
                                       rn = ls_route_next (rn))
        if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
          if (pal_strcmp (ifp->name, mlink->ifname) == 0 && mlink->oi)  
            ospf_multi_area_if_up (mlink->oi);
    }
#endif /* HAVE_OSPF_MULTI_AREA */
  return 0;
}

int
ospf_nsm_if_down (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct listnode *node = NULL;
  struct ospf *top = NULL;
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
#ifdef HAVE_OSPF_MULTI_INST
         /* If multiple instances are running on this interface */
         if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
           LIST_LOOP(om->ospf, top_alt, node_alt)
             {
               if (oi->top != top_alt)
                 {
                   /* Get oi from interface table of each instance */
                   oi_other = ospf_if_entry_match (top_alt, oi);
                   if (oi_other)
                     ospf_if_down (oi_other);
                 }
             }
#endif /* HAVE_OSPF_MULTI_INST */
         ospf_if_down (oi);
      }

  ls_unlock_node (rn_tmp);

#ifdef HAVE_OSPF_MULTI_AREA
  LIST_LOOP (om->ospf, top, node)
    {
      for (rn = ls_table_top (top->multi_area_link_table); rn;
                                       rn = ls_route_next (rn))
        if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
          if (pal_strcmp (ifp->name, mlink->ifname) == 0 && mlink->oi)
            ospf_if_down (mlink->oi);
    }
#endif /* HAVE_OSPF_MULTI_AREA */

  return 0;
}

int
ospf_nsm_if_update (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node_alt = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */
#ifdef HAVE_OSPF_MULTI_AREA
  struct listnode *node = NULL;
  struct ospf *top = NULL;
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
	if (NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BANDWIDTH))
          {
#ifdef HAVE_OSPF_MULTI_INST
            /* If multiple instances are running on this interface */
            if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
              LIST_LOOP(om->ospf, top_alt, node_alt)
                {
                  if (oi->top != top_alt)
                    {
                      /* Get oi from interface table of each instance */
                      oi_other = ospf_if_entry_match (top_alt, oi);
                      if (oi_other)
                        ospf_if_output_cost_update (oi_other);
                    }
                }
#endif /* HAVE_OSPF_MULTI_INST */
            ospf_if_output_cost_update (oi);
          }

#ifdef HAVE_OSPF_TE
	if (NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BANDWIDTH)
	    || NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH)
#ifdef HAVE_DSTE
	    || NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BC_MODE)
	    || NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BW_CONSTRAINT)
#endif /* HAVE_DSTE */
	    || NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_ADMIN_GROUP))
	  ospf_te_if_attr_update (ifp, oi->top);

#ifdef HAVE_GMPLS
        if (NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_GMPLS_TYPE))
          {
            if (ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)
              ospf_te_if_gmpls_type_update (oi);
            else if (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
              {
                UNSET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA);
                UNSET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL);
              }
          }
#endif /* HAVE_GMPLS */

#endif /* HAVE_OSPF_TE */

      }

  ls_unlock_node (rn_tmp);

#ifdef HAVE_OSPF_MULTI_AREA
  LIST_LOOP (om->ospf, top, node)
    {
      for (rn = ls_table_top (top->multi_area_link_table); rn;
                                       rn = ls_route_next (rn))
        if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
          if (pal_strcmp (ifp->name, mlink->ifname) == 0 && mlink->oi)
            {
              if (NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BANDWIDTH))
                ospf_if_output_cost_update (mlink->oi);

#ifdef HAVE_OSPF_TE
              if (NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BANDWIDTH)
                  || NSM_CHECK_CTYPE (ifp->cindex, 
                     NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH)
#ifdef HAVE_DSTE
                  || NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BC_MODE)
                  || NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_BW_CONSTRAINT)
#endif /* HAVE_DSTE */
                  || NSM_CHECK_CTYPE (ifp->cindex, NSM_LINK_CTYPE_ADMIN_GROUP))
                ospf_te_if_attr_update (ifp, mlink->oi->top);
#endif /* HAVE_OSPF_TE */ 
            }
    }
#endif /* HAVE_OSPF_MULTI_AREA */

  return 0;
}

#ifdef HAVE_OSPF_TE
int
ospf_nsm_if_priority_bw (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;
#ifdef HAVE_OSPF_MULTI_AREA
  struct listnode *node = NULL;
  struct ospf *top = NULL;
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      ospf_te_if_attr_update (ifp, oi->top);

  ls_unlock_node (rn_tmp);

#ifdef HAVE_OSPF_MULTI_AREA
  LIST_LOOP (om->ospf, top, node)
    {
      for (rn = ls_table_top (top->multi_area_link_table); rn;
                                       rn = ls_route_next (rn))
        if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
          if (pal_strcmp (ifp->name, mlink->ifname) == 0 && mlink->oi)
            ospf_te_if_attr_update (ifp, mlink->oi->top);
    }
#endif /* HAVE_OSPF_MULTI_AREA */

  return 0;
}
#endif /* HAVE_OSPF_TE */

#ifdef HAVE_GMPLS
int
ospf_nsm_if_gmpls (struct interface *ifp)
{
  struct ospf_master *om = ifp->vr->proto;
  struct ospf_interface *oi;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 lp;

#ifdef HAVE_OSPF_MULTI_AREA
  struct listnode *node = NULL;
  struct ospf *top = NULL;
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix8));
  lp.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  lp.prefixlen = 32;
  ls_prefix_set_args ((struct ls_prefix *)&lp, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&lp);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      ospf_te_if_attr_update (ifp, oi->top);

  ls_unlock_node (rn_tmp);

#ifdef HAVE_OSPF_MULTI_AREA
  LIST_LOOP (om->ospf, top, node)
    {
      for (rn = ls_table_top (top->multi_area_link_table); rn;
                                       rn = ls_route_next (rn))
        if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
          if (pal_strcmp (ifp->name, mlink->ifname) == 0 && mlink->oi)
            ospf_te_if_attr_update (ifp, mlink->oi->top);
    }
#endif /* HAVE_OSPF_MULTI_AREA */


  return 0;
}

int
ospf_telink_add_hook (void *tel, void *msg)
{
  struct telink *tlink;
  struct nsm_msg_te_link *message;
  struct ospf_telink *os_telink;

  tlink = (struct telink *) tel;
  message = (struct nsm_msg_te_link *) msg;
  os_telink = (struct ospf_telink *)tlink->info;
  if (os_telink == NULL)
    {
      os_telink = ospf_te_link_new (tlink->gmif->vr->id, tlink);
      tlink->info = (void *)os_telink;
    }

  if (CHECK_FLAG (tlink->status, GMPLS_INTERFACE_UP))
    ospf_activate_te_link (os_telink);

  return 0;
}

int
ospf_telink_delete_hook (void *tel, void *msg)
{
  struct ospf_master *om;
  struct ospf_telink *os_telink;

  struct telink *tlink;
  struct nsm_msg_te_link *message;

  tlink = (struct telink *) tel;
  message = (struct nsm_msg_te_link *) msg;
  /* OSPF master */
  om = tlink->gmif->vr->proto;

  /* OSPF telink */
  os_telink = (struct ospf_telink *)tlink->info;

  if (CHECK_FLAG (tlink->status, GMPLS_INTERFACE_UP))
    {
      ospf_deactivate_te_link (os_telink);

      XFREE (MTYPE_OSPF_TELINK ,os_telink);
      os_telink = NULL;
      tlink->info = NULL;
    }

  return 0;
}


int
ospf_telink_update_hook (void *tel, void *msg)
{
  struct ospf_telink *os_telink = NULL;
  struct ospf_master *om = NULL;
  struct telink *tlink;
  struct nsm_msg_te_link *message;
  int old_status;

  tlink = (struct telink *) tel;
  message = (struct nsm_msg_te_link *) msg;

  /* OSPF master */
  om = tlink->gmif->vr->proto;

  /* OSPF telink */
  os_telink = (struct ospf_telink *)tlink->info;

  /* copy the telink current status to temporary variable */
  old_status = tlink->status;

  if (NSM_CHECK_CTYPE_FLAG (message->cindex, NSM_TELINK_CTYPE_STATUS))
    {      
      if (!CHECK_FLAG (message->status, GMPLS_INTERFACE_UP)          
          && CHECK_FLAG (old_status, GMPLS_INTERFACE_UP))        
        {          
          /* Deactivate the te_link and unset link-local lsa */          
          ospf_deactivate_te_link (os_telink);          
          ospf_te_link_local_lsa_unset (os_telink);
        }
    }
  /* Update the telink parameters */
  nsm_util_te_link_update (tlink, message);
  
 /* Update the ospf_telink structure */
  if (NSM_CHECK_CTYPE_FLAG (message->cindex, NSM_TELINK_CTYPE_STATUS)
      && CHECK_FLAG (tlink->status, GMPLS_INTERFACE_UP)
      && !CHECK_FLAG (old_status, GMPLS_INTERFACE_UP))
    {
      /* Activate the te-link and set link-local lsa */
      ospf_activate_te_link (os_telink);
      ospf_te_link_local_lsa_set (os_telink);
    }
  else if (CHECK_FLAG (tlink->status, GMPLS_INTERFACE_UP))
    ospf_activate_te_link (os_telink);

  return 0;
}
#endif /* HAVE_GMPLS */

int
ospf_nsm_if_addr_add (struct connected *ifc)
{
  struct ospf_master *om = ifc->ifp->vr->proto;
  struct ospf_interface *oi;
  struct ospf_network *network;
  struct ospf *top;
  struct ls_prefix p;
  struct listnode *node;
  struct apn_vrf *iv;
  struct ospf_vrf *ov;

  iv = ifc->ifp->vrf;
  if (iv == NULL)
    return 0;

  ov = iv->proto;
  if (ov == NULL)
    return 0;

  if (ifc->family != AF_INET)
    return 0;

  /* There is already another interface, but this might be wrong. */
  oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
				    ifc->ifp->ifindex);
  if (oi)
    return 0;

  /* Check 'ip ospf disable all'. */
  if (ospf_if_disable_all_get (ifc->ifp))
    return 0;

  /* Find `network area' matches to this connected. */
  if (ov)
    LIST_LOOP (ov->ospf, top, node)
      {
	ls_prefix_ipv4_set (&p, ifc->address->prefixlen,
			    ifc->address->u.prefix4);
#ifdef HAVE_GMPLS
        if ((ifc->ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)
            && (ifc->ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA))
           continue;
#endif /* HAVE_GMPLS */

	if ((network = ospf_network_match (top, &p))
            ||(network = ospf_network_lookup (top, &DefaultIPv4)))
	  ospf_network_apply (top, network, ifc);

        /* Find host entry matches to this connected.  */
        ospf_host_entry_match_apply (top, ifc);
      }

  ospf_call_notifiers (om, OSPF_NOTIFY_ADDRESS_NEW, ifc->ifp, ifc);
  
  return 0;
}

int
ospf_nsm_if_addr_delete (struct connected *ifc)
{
  struct ospf_master *om = ifc->ifp->vr->proto;
  struct ospf_interface *oi;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  if (ifc->family != AF_INET)
    return 0;

  oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
				    ifc->ifp->ifindex);

  if (oi != NULL && oi->type != OSPF_IFTYPE_HOST)
    {
#ifdef HAVE_OSPF_MULTI_INST
       /* If multiple instances are running on this interface */
       if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
         LIST_LOOP(om->ospf, top_alt, node)
           {
             if (oi->top != top_alt)
               {
                 /* Delete oi from interface table of each instance */
                 oi_other = ospf_if_entry_match (top_alt, oi);
                 if (oi_other)
                   ospf_if_delete (oi_other);
               }
           }
#endif /* HAVE_OSPF_MULTI_INST */
      ospf_if_delete (oi);

      ospf_call_notifiers (om, OSPF_NOTIFY_ADDRESS_DEL, ifc->ifp, ifc);
    }

  /* Delete all  host specific ospf interfaces that match to this connected.*/
  ospf_global_if_host_entry_delete (om, ifc);

  return 0;
}

int
ospf_nsm_recv_route_ipv4 (struct nsm_msg_header *header,
			  void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_route_ipv4 *msg = message;
  struct prefix_ipv4 p;
  struct pal_in4_addr nexthop;
  struct ospf_master *om;
  struct ospf_vrf *ov;
  u_int32_t ifindex;

  if (nc->debug)
    nsm_route_ipv4_dump (nc->zg, msg);

  p.family = AF_INET;
  p.prefixlen = msg->prefixlen;
  p.prefix = msg->prefix;

  /* Get first nexthop. */
  nexthop = IPv4AddressUnspec;
  ifindex = 0;
  if (msg->nexthop_num > 0)
    {
      ifindex = msg->nexthop[0].ifindex;
      nexthop = msg->nexthop[0].addr;
    }

  if (!OSPF_CAP_HAVE_VRF_OSPF)
    if (header->vrf_id != 0)
      return 0;

  om = ospf_master_lookup_by_id (nc->zg, header->vr_id);
  if (om == NULL)
    return 0;

  ov = ospf_vrf_lookup_by_id (om, header->vrf_id);
  if (ov == NULL)
    return 0;

  if (CHECK_FLAG (msg->flags, NSM_MSG_ROUTE_FLAG_ADD))
    ospf_redist_info_add (ov, msg);
  else
    ospf_redist_info_delete (ov, msg);

  return 0;
}

s_int32_t
ospf_nsm_recv_router_id (struct nsm_msg_header *header,
                         void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_router_id *msg;
  s_int32_t ret;
  struct ospf_master *om;
  struct ospf_vrf *ov;

  msg = message;
  ret = 0;

  if (header->type != NSM_MSG_ROUTER_ID_UPDATE || ! msg)
    {
      ret = -1;
      goto EXIT;
    }

  om = ospf_master_lookup_by_id (nc->zg, header->vr_id);
  if (om == NULL)
    return 0;

  ov = ospf_vrf_lookup_by_id (om, header->vrf_id);
  if (ov == NULL)
    return 0;

  IPV4_ADDR_COPY (&ov->iv->router_id, &msg->router_id);

  ospf_vrf_update_router_id (ov->iv);

EXIT:
  return ret;
}

#ifdef HAVE_MPLS
int
ospf_nsm_recv_igp_shortcut_lsp (struct nsm_msg_header *header,
				void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_igp_shortcut_lsp *msg = message;
  struct ospf_master *om;
  struct ospf_vrf *ov;
  int process;

  if (nc->debug)
    nsm_igp_shortcut_lsp_dump (nc->zg, msg);

  om = ospf_master_lookup_by_id (nc->zg, header->vr_id);
  if (om == NULL)
    return 0;

  ov = ospf_vrf_lookup_by_id (om, header->vrf_id);
  if (ov == NULL)
    return 0;

  if (msg->action == NSM_MSG_IGP_SHORTCUT_ADD)
    process = ospf_igp_shortcut_lsp_add (ov, msg);
  else if (msg->action == NSM_MSG_IGP_SHORTCUT_DELETE)
    process = ospf_igp_shortcut_lsp_delete (ov, msg);
  else
    process = ospf_igp_shortcut_lsp_update (ov, msg); 

  if (process)
    ospf_igp_shortcut_lsp_process (ov);

  return 0;
}
#endif /* HAVE_MPLS */

int
ospf_nsm_disconnect_clean (void)
{
  ospf_disconnect_clean ();
    /* Start NSM processing. */
  nsm_client_start (ZG->nc);
  return 0;
}

/* OSPFv2 NSM client initialization. */
void
ospf_nsm_init (struct lib_globals *zg)
{
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif = NULL;
#endif /* HAVE_GMPLS */

  /* Create NSM client. */
  zg->nc = nsm_client_create (zg, 0);
  if (zg->nc == NULL)
    return;

  /* Set version and protocol. */
  nsm_client_set_version (zg->nc, NSM_PROTOCOL_VERSION_1);
  nsm_client_set_protocol (zg->nc, APN_PROTO_OSPF);

  /* Set required services. */
  nsm_client_set_service (zg->nc, NSM_SERVICE_INTERFACE);
  nsm_client_set_service (zg->nc, NSM_SERVICE_ROUTE);
  nsm_client_set_service (zg->nc, NSM_SERVICE_ROUTER_ID);
  nsm_client_set_service (zg->nc, NSM_SERVICE_VRF);
#ifdef HAVE_MPLS
  nsm_client_set_service (zg->nc, NSM_SERVICE_IGP_SHORTCUT);
#endif /* HAVE_MPLS */
#ifdef HAVE_OSPF_TE
  nsm_client_set_service (zg->nc, NSM_SERVICE_TE);
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_GMPLS
  nsm_client_set_service (zg->nc, NSM_SERVICE_GMPLS);
  nsm_client_set_service (zg->nc, NSM_SERVICE_TE_LINK);
#endif /* HAVE_GMPLS */

  /* Register callbacks. */
  nsm_client_set_callback (zg->nc, NSM_MSG_SERVICE_REPLY,
			   ospf_nsm_recv_service);
  nsm_client_set_callback (zg->nc, NSM_MSG_ROUTE_IPV4,
			   ospf_nsm_recv_route_ipv4);

  /* Register Router-ID and routes update callback */
  nsm_client_set_callback (zg->nc, NSM_MSG_ROUTER_ID_UPDATE,
			   ospf_nsm_recv_router_id);
#ifdef HAVE_MPLS
  nsm_client_set_callback (zg->nc, NSM_MSG_MPLS_IGP_SHORTCUT_LSP,
			   ospf_nsm_recv_igp_shortcut_lsp);
#endif /* HAVE_MPLS */

  /* Set the interface callbacks.  */
  if_add_hook (&zg->ifg, IF_CALLBACK_VR_BIND, ospf_nsm_if_add);
  if_add_hook (&zg->ifg, IF_CALLBACK_VR_UNBIND, ospf_nsm_if_delete);
  if_add_hook (&zg->ifg, IF_CALLBACK_VRF_BIND, ospf_nsm_if_vrf_bind);
  if_add_hook (&zg->ifg, IF_CALLBACK_VRF_UNBIND, ospf_nsm_if_vrf_unbind);
  if_add_hook (&zg->ifg, IF_CALLBACK_UP, ospf_nsm_if_up);
  if_add_hook (&zg->ifg, IF_CALLBACK_DOWN, ospf_nsm_if_down);
  if_add_hook (&zg->ifg, IF_CALLBACK_UPDATE, ospf_nsm_if_update);
#ifdef HAVE_OSPF_TE
  if_add_hook (&zg->ifg, IF_CALLBACK_PRIORITY_BW, ospf_nsm_if_priority_bw);
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_GMPLS
  gmif = gmpls_if_get (zg, 0);

  /* Set the telink callbacks */
  gif_add_hook (gmif, GIF_CALLBACK_GMPLS_TELINK_ADD, ospf_telink_add_hook);
  gif_add_hook (gmif, GIF_CALLBACK_GMPLS_TELINK_DEL, ospf_telink_delete_hook);
  gif_add_hook (gmif, GIF_CALLBACK_GMPLS_TELINK_UPDATE, ospf_telink_update_hook);
#endif /* HAVE_GMPLS */

  /* Set the interface address callbacks.  */
  ifc_add_hook (&zg->ifg, IFC_CALLBACK_ADDR_ADD, ospf_nsm_if_addr_add);
  ifc_add_hook (&zg->ifg, IFC_CALLBACK_ADDR_DELETE, ospf_nsm_if_addr_delete);

   /* Set disconnect handling callback. */
  nsm_client_set_disconnect_callback (zg->nc, ospf_nsm_disconnect_clean);

  /* Start NSM processing. */
  nsm_client_start (zg->nc);
}
