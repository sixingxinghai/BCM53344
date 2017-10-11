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
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_ia.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_MPLS
#include "ospfd/ospf_mpls.h"
#endif /* HAVE_MPLS */


struct ospf_nexthop *
ospf_nexthop_new (struct ospf *top)
{
  struct ospf_nexthop *new;

  new = XCALLOC (MTYPE_OSPF_NEXTHOP, sizeof (struct ospf_nexthop));
  new->top = top;

  return new;
}

void
ospf_nexthop_free (struct ospf_nexthop *nh)
{
  if (nh->oi)
    ospf_if_unlock (nh->oi);

  XFREE (MTYPE_OSPF_NEXTHOP, nh);
}

struct ospf_nexthop *
ospf_nexthop_get (struct ospf_area *area,
                  struct ospf_interface *oi, struct pal_in4_addr nbr_id)
{
  struct ospf_nexthop *nh;
  struct ls_prefix_nh p;
  struct ls_node *rn;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix_nh));
  p.prefixsize = OSPF_NEXTHOP_TABLE_DEPTH;
  p.prefixlen = OSPF_NEXTHOP_TABLE_DEPTH * 8;
  p.if_id.s_addr = OSPF_NEXTHOP_IF_ID (area, oi);
  p.nbr_id = nbr_id;

  rn = ls_node_get (area->top->nexthop_table, (struct ls_prefix *)&p);
  if ((nh = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      nh = ospf_nexthop_new (area->top);
      nh->if_id = p.if_id;
      nh->nbr_id = p.nbr_id;
      if (oi == NULL || oi->type == OSPF_IFTYPE_VIRTUALLINK)
        SET_FLAG (nh->flags, OSPF_NEXTHOP_VIA_TRANSIT);
      if (oi != NULL)
        nh->oi = ospf_if_lock (oi);
      RN_INFO_SET (rn, RNI_DEFAULT, nh);
    }

  ls_unlock_node (rn);

  return ospf_nexthop_lock (nh);
}

struct ospf_nexthop *
ospf_nexthop_lookup (struct ospf_area *area,
                     struct ospf_interface *oi, struct pal_in4_addr nbr_id)
{
  struct ls_prefix_nh p;
  struct ls_node *rn;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix_nh));
  p.prefixsize = OSPF_NEXTHOP_TABLE_DEPTH;
  p.prefixlen = OSPF_NEXTHOP_TABLE_DEPTH * 8;
  p.if_id.s_addr = OSPF_NEXTHOP_IF_ID (area, oi);
  p.nbr_id = nbr_id;

  rn = ls_node_lookup (area->top->nexthop_table, (struct ls_prefix *)&p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

void
ospf_nexthop_delete (struct ospf_nexthop *nh)
{
  struct ls_prefix_nh p;
  struct ls_node *rn;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix_nh));
  p.prefixsize = OSPF_NEXTHOP_TABLE_DEPTH;
  p.prefixlen = OSPF_NEXTHOP_TABLE_DEPTH * 8;
  p.if_id = nh->if_id;
  p.nbr_id = nh->nbr_id;

  rn = ls_node_lookup (nh->top->nexthop_table, (struct ls_prefix *)&p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
    }
  SET_FLAG (nh->flags, OSPF_NEXTHOP_INVALID);
}

struct ospf_nexthop *
ospf_nexthop_lock (struct ospf_nexthop *nh)
{
  nh->lock++;
  return nh;
}

void
ospf_nexthop_unlock (struct ospf_nexthop *nh)
{
  nh->lock--;
  if (nh->lock == 0)
    {
      if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
        ospf_nexthop_delete (nh);
      ospf_nexthop_free (nh);
    }
}

void
ospf_nexthop_if_down (struct ospf_interface *oi)
{
  struct ospf_nexthop *nh;
  struct ls_node *rn;

  for (rn = ls_table_top (oi->top->nexthop_table); rn; rn = ls_route_next (rn))
    if ((nh = RN_INFO (rn, RNI_DEFAULT)))
      if (nh->oi == oi)
        {
          SET_FLAG (nh->flags, OSPF_NEXTHOP_INVALID);
          RN_INFO_UNSET (rn, RNI_DEFAULT);
        }
}

void
ospf_nexthop_nbr_down (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  struct ospf_nexthop *nh;

  nh = ospf_nexthop_lookup (oi->area, oi, OSPF_NEXTHOP_NBR_ID (nbr));
  if (nh != NULL)
    ospf_nexthop_delete (nh);
}

void
ospf_nexthop_abr_down (struct ospf_area *area, struct pal_in4_addr nbr_id)
{
  struct ospf_nexthop *nh;

  nh = ospf_nexthop_lookup (area, NULL, nbr_id);
  if (nh != NULL)
    ospf_nexthop_delete (nh);
}

void
ospf_nexthop_transit_down (struct ospf_area *area)
{
  struct ls_node *rn, *rn_tmp;
  struct ospf_nexthop *nh;
  struct ls_prefix_nh p;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix_nh));
  p.prefixsize = OSPF_NEXTHOP_TABLE_DEPTH;
  p.prefixlen = 32;
  p.if_id = area->area_id;

  rn_tmp = ls_node_get (area->top->nexthop_table, (struct ls_prefix *)&p);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((nh = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_VIA_TRANSIT))
        {
          SET_FLAG (nh->flags, OSPF_NEXTHOP_INVALID);
          RN_INFO_UNSET (rn, RNI_DEFAULT);
        }

  ls_unlock_node (rn_tmp);
}


struct ospf_path *
ospf_path_new (int type)
{
  struct ospf_path *new;

  new = XMALLOC (MTYPE_OSPF_PATH, sizeof (struct ospf_path));
  pal_mem_set (new, 0, sizeof (struct ospf_path));

  new->type = type;
  new->nexthops = vector_init (OSPF_VECTOR_SIZE_MIN);

  /* LSA vector for the incremental update.  */
  if (type == OSPF_PATH_INTRA_AREA
      || type == OSPF_PATH_INTER_AREA
      || type == OSPF_PATH_EXTERNAL)
    new->lsa_vec = vector_init (OSPF_VECTOR_SIZE_MIN);

  ospf_path_lock (new);

  return new;
}

void
ospf_path_free (struct ospf_path *path)
{
  struct ospf_nexthop *nh;
  struct ospf_lsa *lsa;
  int i;

  for (i = 0; i < vector_max (path->nexthops); i++)
    if ((nh = vector_slot (path->nexthops, i)))
      ospf_nexthop_unlock (nh);

  vector_free (path->nexthops);

  if (path->lsa_vec)
    {
      for (i = 0; i < vector_max (path->lsa_vec); i++)
        if ((lsa = vector_slot (path->lsa_vec, i)))
          ospf_lsa_unlock (lsa);

      vector_free (path->lsa_vec);
    }

  /* Free the path base.  */
  if (path->path_base)
    ospf_path_unlock (path->path_base);

  /* Free the transit path.  */
  if (path->path_transit)
    ospf_path_unlock (path->path_transit);

#ifdef HAVE_MPLS
  if (path->t_lsp)
    ospf_igp_shortcut_lsp_unlock (path->t_lsp);
#endif /* HAVE_MPLS */

  XFREE (MTYPE_OSPF_PATH, path);
}

struct ospf_path *
ospf_path_lock (struct ospf_path *path)
{
  path->lock++;
  return path;
}

void
ospf_path_unlock (struct ospf_path *path)
{
  path->lock--;
  if (path->lock == 0)
    ospf_path_free (path);
}

void
ospf_path_nexthop_add (struct ospf_path *path, struct ospf_nexthop *nh)
{
  struct ospf_nexthop *nh_tmp;
  int i;

  if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
    return;

  for (i = 0; i < vector_max (path->nexthops); i++)
    if ((nh_tmp = vector_slot (path->nexthops, i)))
      if (nh_tmp == nh)
        return;

  vector_set (path->nexthops, ospf_nexthop_lock (nh));
}

void
ospf_path_nexthop_delete (struct ospf_path *path,
                          struct pal_in4_addr if_id,
                          struct pal_in4_addr nbr_id)
{
  struct ospf_nexthop *nh;
  int i;

  for (i = 0; i < vector_max (path->nexthops); i++)
    if ((nh = vector_slot (path->nexthops, i)))
      if (nh->if_id.s_addr == if_id.s_addr
          && nh->nbr_id.s_addr == nbr_id.s_addr)
        {
          ospf_nexthop_unlock (nh);
          vector_slot (path->nexthops, i) = NULL;
          return;
        }
}

void
ospf_path_nexthop_delete_other (struct ospf_path *path,
                                struct ospf_nexthop *nh)
{
  struct ospf_nexthop *nh_other;
  int i;

  for (i = 0; i < vector_max (path->nexthops); i++)
    if ((nh_other = vector_slot (path->nexthops, i)))
      if (nh_other != nh)
        {
          ospf_nexthop_unlock (nh_other);
          vector_slot (path->nexthops, i) = NULL;
        }
}

struct ospf_nexthop *
ospf_path_nexthop_lookup (struct ospf_path *path,
                          struct ospf_interface *oi,
                          struct pal_in4_addr nbr_id)
{
  struct ospf_nexthop *nh;
  int i;

  for (i = 0; i < vector_max (OSPF_PATH_NEXTHOPS (path)); i++)
    if ((nh = vector_slot (OSPF_PATH_NEXTHOPS (path), i)))
      if (nh->oi == oi && IPV4_ADDR_SAME (&nh->nbr_id, &nbr_id))
        return nh;

  return NULL;
}

void
ospf_path_nexthop_copy_all (struct ospf *top,
                            struct ospf_path *path, struct ospf_vertex *v)
{
  struct ospf_nexthop *nh;
  int i;
  for (i = 0; i < vector_max (v->nexthop); i++)
    if ((nh = vector_slot (v->nexthop, i)))
      if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
        ospf_path_nexthop_add (path, nh);

#ifdef HAVE_MPLS
  if (v->t_lsp)
    path->t_lsp = ospf_igp_shortcut_lsp_lock (v->t_lsp);
#endif /* HAVE_MPLS */
}

void
ospf_path_nexthop_vector_add (vector vec, struct ospf_nexthop *nh)
{
  struct ospf_nexthop *nh_tmp;
  int i;

  for (i = 0; i < vector_max (vec); i++)
    if ((nh_tmp = vector_slot (vec, i)))
      if (nh_tmp == nh)
        return;

  vector_set (vec, nh);
}

vector
ospf_path_nexthop_vector (struct ospf_path *path)
{
  vector vec = vector_init (OSPF_VECTOR_SIZE_MIN);
  struct ospf_nexthop *nh, *vnh;
  struct ospf_area *area;
  struct ospf_vertex *v;
  int i, j;

  for (i = 0; i < vector_max (OSPF_PATH_NEXTHOPS (path)); i++)
    if ((nh = vector_slot (OSPF_PATH_NEXTHOPS (path), i)))
      if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
        {
          if (CHECK_FLAG (nh->flags, OSPF_NEXTHOP_VIA_TRANSIT))
            {
              area = ospf_area_entry_lookup (nh->top, nh->if_id);
              if (area == NULL)
                continue;

              v = ospf_spf_vertex_lookup (area, nh->nbr_id);
              if (v == NULL)
                continue;

              for (j = 0; j < vector_max (v->nexthop); j++)
                if ((vnh = vector_slot (v->nexthop, j)))
                  if (!CHECK_FLAG (vnh->flags, OSPF_NEXTHOP_INVALID))
                    ospf_path_nexthop_vector_add (vec, vnh);
            }
          else
            ospf_path_nexthop_vector_add (vec, nh);
        }

  return vec;
}

int
ospf_path_same (struct ospf_path *p1, struct ospf_path *p2)
{
  struct ospf_nexthop *nh;
  int i;

  if (!ospf_path_valid (p1) || !ospf_path_valid (p2))
    return 0;

  if (OSPF_PATH_COST (p1) != OSPF_PATH_COST (p2))
    return 0;

  if (p1->type == OSPF_PATH_EXTERNAL)
    {
      if (CHECK_FLAG (p1->flags, OSPF_PATH_TYPE2)
          != CHECK_FLAG (p2->flags, OSPF_PATH_TYPE2))
        return 0;

      if (CHECK_FLAG (p1->flags, OSPF_PATH_TYPE2))
        if (p1->type2_cost != p2->type2_cost)
          return 0;

      if (p1->tag != p2->tag)
        return 0;
    }

  if (vector_count (OSPF_PATH_NEXTHOPS (p1))
      != vector_count (OSPF_PATH_NEXTHOPS (p2)))
    return 0;

  for (i = 0; i < vector_max (OSPF_PATH_NEXTHOPS (p1)); i++)
    if ((nh = vector_slot (OSPF_PATH_NEXTHOPS (p1), i)))
      if (!ospf_path_nexthop_lookup (p2, nh->oi, nh->nbr_id))
        return 0;

  return 1;
}

struct ospf_path *
ospf_path_copy (struct ospf_path *old)
{
  struct ospf_nexthop *nh;
  struct ospf_path *new;
  struct ospf_lsa *lsa;
  int i;

  new = ospf_path_new (old->type);

  /* Copy the flags and others.  */
  new->flags = old->flags;
  new->cost = old->cost;
  new->area_id = old->area_id;
  new->type2_cost = old->type2_cost;

  /* Copy the nexthops.  */
  for (i = 0; i < vector_max (old->nexthops); i++)
    if ((nh = vector_slot (old->nexthops, i)))
      ospf_path_nexthop_add (new, nh);

  /* Copy the LSAs.  */
  if (old->lsa_vec != NULL)
    for (i = 0; i < vector_max (old->lsa_vec); i++)
      if ((lsa = vector_slot (old->lsa_vec, i)))
        vector_set (new->lsa_vec, ospf_lsa_lock (lsa));

  return new;
}

void
ospf_path_swap (struct ospf_path *new, struct ospf_path *old)
{
  struct ospf_path *path_transit = new->path_transit;
  vector vec = new->lsa_vec;
#ifdef HAVE_MPLS
  struct ospf_igp_shortcut_lsp *t_lsp = new->t_lsp;
#endif /* HAVE_MPLS */
  
  
  new->path_transit = old->path_transit;
  new->lsa_vec = old->lsa_vec;
#ifdef HAVE_MPLS
  new->t_lsp = old->t_lsp;
#endif /* HAVE_MPLS */

  old->path_transit = path_transit;
  old->lsa_vec = vec;
#ifdef HAVE_MPLS
  old->t_lsp = t_lsp;
#endif /* HAVE_MPLS */
}

int
ospf_path_valid (struct ospf_path *path)
{
  struct ospf_nexthop *nh;
  int i;

  /* Sanity check.  */
  if (path == NULL)
    return 0;

  /* Discard path is always valid.  */
  else if (path->type == OSPF_PATH_DISCARD)
    return 1;

  /* Check the path base.  */
  if (path->path_base)
    if (!ospf_path_valid (path->path_base))
      return 0;

  for (i = 0; i < vector_max (path->nexthops); i++)
    if ((nh = vector_slot (path->nexthops, i)))
      if (!CHECK_FLAG (nh->flags, OSPF_NEXTHOP_INVALID))
        return 1;

  return 0;
}

int
ospf_path_code (struct ospf_path *path, struct ospf_nexthop *nh)
{
  if (path->type == OSPF_PATH_CONNECTED)
    return OSPF_PATH_CODE_CONNECTED;
  else if (path->type == OSPF_PATH_DISCARD)
    return OSPF_PATH_CODE_DISCARD;
  else if (path->type == OSPF_PATH_INTRA_AREA)
    return OSPF_PATH_CODE_INTRA_AREA;
  else if (path->type == OSPF_PATH_INTER_AREA)
    return OSPF_PATH_CODE_INTER_AREA;
  else if (path->type == OSPF_PATH_EXTERNAL)
    {
      if (nh->oi == NULL || IS_AREA_DEFAULT (nh->oi->area))
        {
          if (!CHECK_FLAG (path->flags, OSPF_PATH_TYPE2))
            return OSPF_PATH_CODE_E1;
          else
            return OSPF_PATH_CODE_E2;
        }
      else
        {
          if (!CHECK_FLAG (path->flags, OSPF_PATH_TYPE2))
            return OSPF_PATH_CODE_N1;
          else
            return OSPF_PATH_CODE_N2;
        }
    }
  return OSPF_PATH_CODE_UNKNOWN;
}

/* RFC2328 16.4.1.  External path preferences

   The path preference rules, stated from highest to lowest
   preference, are as follows. Note that as a result of these
   rules, there may still be multiple paths of the highest
   preference. In this case, the path to use must be determined
   based on cost, as described in Section 16.4.

   o   Intra-area paths using non-backbone areas are always the
       most preferred.

   o   The other paths, intra-area backbone paths and inter-
       area paths, are of equal preference.  */
int
ospf_path_preference (struct ospf_path *path)
{
  if (path->path_base)
    return ospf_path_preference (path->path_base);
  else if (path->type == OSPF_PATH_INTER_AREA)
    return OSPF_PATH_INTER_AREA;
  else if (IS_AREA_ID_BACKBONE (path->area_id))
    return OSPF_PATH_INTER_AREA;
  else
    return OSPF_PATH_INTRA_AREA;
}

int
ospf_path_cmp_by_cost (struct ospf *top,
                       struct ospf_path *old, struct ospf_path *new)
{
  if (OSPF_PATH_COST (old) > OSPF_PATH_COST (new))
    return 1;
  else if (OSPF_PATH_COST (old) < OSPF_PATH_COST (new))
    return -1;

  return 0;
}

/* RFC2328 16.4. (6) Compare the AS external path described by the LSA.  */
int
ospf_path_cmp_external (struct ospf *top,
                        struct ospf_path *old, struct ospf_path *new)
{
  int ret;

  /* (b) Type 1 external paths are always preferred over type 2
         external paths. When all paths are type 2 external
         paths, the paths with the smallest advertised type 2
         metric are always preferred.  */
  if (!CHECK_FLAG (old->flags, OSPF_PATH_TYPE2)
      && CHECK_FLAG (new->flags, OSPF_PATH_TYPE2))
    return -1;
  if (CHECK_FLAG (old->flags, OSPF_PATH_TYPE2)
      && !CHECK_FLAG (new->flags, OSPF_PATH_TYPE2))
    return 1;

  /* (c) If the new AS external path is still indistinguishable
         from the current paths in the N's routing table entry,
         and RFC1583Compatibility is set to "disabled", select
         the preferred paths based on the intra-AS paths to the
         ASBR/forwarding addresses, as specified in Section
         16.4.1.  */
  if (!IS_RFC1583_COMPATIBLE (top))
    {
      ret = ospf_path_preference (old) - ospf_path_preference (new);
      if (ret != 0)
        return ret;
    }

  /* (d) If the new AS external path is still indistinguishable
         from the current paths in the N's routing table entry,
         select the preferred path based on a least cost
         comparison.  Type 1 external paths are compared by
         looking at the sum of the distance to the forwarding
         address and the advertised type 1 metric (X+Y).  Type 2
         external paths advertising equal type 2 metrics are
         compared by looking at the distance to the forwarding
         addresses.  */
  if (CHECK_FLAG (old->flags, OSPF_PATH_TYPE2))
    {
      if (old->type2_cost > new->type2_cost)
        return 1;
      else if (old->type2_cost < new->type2_cost)
        return -1;
    }
  ret = ospf_path_cmp_by_cost (top, old, new);
  if (ret != 0)
    return ret;

  /* when there are multiple least cost routing table
     entries the entry whose associated area has the largest OSPF
     Area ID (when considered as an unsigned 32-bit integer) is
     chosen.  */
  if (!IS_RFC1583_COMPATIBLE (top))
    {
      ret = IPV4_ADDR_CMP (&old->area_id, &new->area_id);
      if (ret < 0)
        return 1;
      else if (ret > 0)
        return -1;
    }

  return 0;
}

int (*ospf_path_cmp_type[])(struct ospf *,
                            struct ospf_path *, struct ospf_path *) =
{
  ospf_path_cmp_by_cost,
  ospf_path_cmp_by_cost,
  ospf_path_cmp_by_cost,
  ospf_path_cmp_external,
  ospf_path_cmp_by_cost,
};

int
ospf_path_asbr_cmp (struct ospf *top,
                    struct ospf_path *old, struct ospf_path *new)
{
  int ret;

  /* RFC2328 16.4 (3)

     If RFC1583Compatibility is set to "disabled", prune the set
     of routing table entries for the ASBR as described in
     Section 16.4.1.  */
  if (!IS_RFC1583_COMPATIBLE (top))
    {
      ret = ospf_path_preference (old) - ospf_path_preference (new);
      if (ret != 0)
        return ret;
    }

  /* In any case, among the remaining routing table entries,
     select the routing table entry with the least cost;  */
  ret = ospf_path_cmp_by_cost (top, old, new);
  if (ret != 0)
    return ret;

  /* when there are multiple least cost routing table
     entries the entry whose associated area has the largest OSPF
     Area ID (when considered as an unsigned 32-bit integer) is
     chosen.  */
  if (!IS_RFC1583_COMPATIBLE (top))
    {
      ret = IPV4_ADDR_CMP (&old->area_id, &new->area_id);
      if (ret < 0)
        return 1;
      else if (ret > 0)
        return -1;
    }

  return 0;
}

int
ospf_path_cmp (struct ospf *top, struct ospf_path *old, struct ospf_path *new)
{
  if (old == NULL)
    return 1;

  /* Network path type compare.  */
  if (old->type != new->type)
    if (old->type > OSPF_PATH_INTRA_AREA
        || new->type > OSPF_PATH_INTRA_AREA)
    return old->type - new->type;

  /* Special case for the ASBR route.  */
  if (CHECK_FLAG (old->flags, OSPF_PATH_ASBR))
    return ospf_path_asbr_cmp (top, old, new);
  else
    return ospf_path_cmp_type[new->type] (top, old, new);
}


struct ospf_route *
ospf_route_new ()
{
  struct ospf_route *new;

  new = XMALLOC (MTYPE_OSPF_ROUTE, sizeof (struct ospf_route));
  pal_mem_set (new, 0, sizeof (struct ospf_route));

  new->paths = vector_init (OSPF_VECTOR_SIZE_MIN);
  new->news = vector_init (OSPF_VECTOR_SIZE_MIN);

  return new;
}

void
ospf_route_free (struct ospf_route *route)
{
  struct ospf_path *path;
  int i;

  for (i = 0; i < vector_max (route->paths); i++)
    if ((path = vector_slot (route->paths, i)))
      ospf_path_unlock (path);

  for (i = 0; i < vector_max (route->news); i++)
    if ((path = vector_slot (route->news, i)))
      ospf_path_unlock (path);

  vector_free (route->paths);
  vector_free (route->news);

  RN_INFO_UNSET (route->rn, RNI_CURRENT);

  XFREE (MTYPE_OSPF_ROUTE, route);
}

struct ospf_path *
ospf_route_path_select (struct ospf *top, struct ospf_route *route)
{
  struct ospf_path *selected = NULL;
  struct ospf_path *path;
  int i;

  for (i = 0; i < vector_max (route->paths); i++)
    if ((path = vector_slot (route->paths, i)))
      if (ospf_path_valid (path))
        if (ospf_path_cmp (top, selected, path) > 0)
          selected = path;

  return selected;
}

int
ospf_route_path_add (struct ospf *top,
                     struct ospf_route *route, struct ospf_path *new)
{
  vector_set (route->paths, new);
  if (ospf_path_cmp (top, route->selected, new) > 0)
    {
      route->selected = new;
      ospf_distance_update (top, route);
      return 1;
    }

  return 0;
}

int
ospf_route_path_update (struct ospf *top, struct ospf_route *route,
                        struct ospf_path *new, struct ospf_path *old)
{
  struct ospf_path *selected = route->selected;
  struct ospf_path *path;
  int i;

  for (i = 0; i < vector_max (route->paths); i++)
    if ((path = vector_slot (route->paths, i)))
      if (path == old)
        {
          vector_slot (route->paths, i) = new;

          route->selected = ospf_route_path_select (top, route);
          ospf_distance_update (top, route);
          if (route->selected != selected)
            return 1;

          return 0;
        }

  return 0;
}

struct ospf_path *
ospf_route_path_lookup (struct ospf_route *route,
                        int type, struct ospf_area *area)
{
  struct ospf_path *path;
  int i;

  for (i = 0; i < vector_max (route->paths); i++)
    if ((path = vector_slot (route->paths, i)))
      if (path->type == type)
        if (area == NULL
            || IPV4_ADDR_SAME (&path->area_id, &area->area_id))
          return path;

  return NULL;
}

struct ospf_path *
ospf_route_path_lookup_from_default (struct ospf_route *route,
                                     struct ospf *top)
{
  struct ospf_path *path, *best = NULL;
  struct ospf_area *area;
  int i;

  for (i = 0; i < vector_max (route->paths); i++)
    if ((path = vector_slot (route->paths, i)))
      if (ospf_path_valid (path))
        if ((area = ospf_area_entry_lookup (top, path->area_id)))
          if (IS_AREA_DEFAULT (area))
            if (ospf_path_cmp (top, best, path) > 0)
              best = path;

  return best;
}

void
ospf_route_path_set_new (struct ospf_route *route, struct ospf_path *new)
{
  vector_set (route->news, new);
}

struct ospf_path *
ospf_route_path_lookup_new (struct ospf_route *route,
                            int type, struct ospf_area *area)
{
  struct ospf_path *new;
  int i;

  for (i = 0; i < vector_max (route->news); i++)
    if ((new = vector_slot (route->news, i)) != NULL)
      if (new->type == type)
        if (area == NULL
            || IPV4_ADDR_SAME (&new->area_id, &area->area_id))
          {
            vector_slot (route->news, i) = NULL;
            return new;
          }

  return NULL;
}

struct ospf_path *
ospf_route_path_lookup_by_area (struct ospf_route *route,
                                struct ospf_area *area)
{
  struct ospf_path *path;
  int i;

  for (i = 0; i < vector_max (route->paths); i++)
    if ((path = vector_slot (route->paths, i)))
      if (area == NULL
          || IPV4_ADDR_SAME (&path->area_id, &area->area_id))
        return path;

  return NULL;
}

struct ospf_route *
ospf_route_lookup (struct ospf *top, struct pal_in4_addr *addr, u_char length)
{
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, length, *addr);
  rn = ls_node_lookup (top->rt_network, &p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_CURRENT);
    }
  return NULL;
}

struct ospf_route *
ospf_route_match (struct ospf *top, struct pal_in4_addr *addr, u_char length)
{
  struct ls_node *rn;
  struct ls_prefix p;

  if (addr == NULL)
    return NULL;

  ls_prefix_ipv4_set (&p, length, *addr);
  rn = ls_node_match (top->rt_network, &p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_CURRENT);
    }
  return NULL;
}

struct ospf_route *
ospf_route_match_internal (struct ospf *top, struct pal_in4_addr *nexthop)
{
  struct ospf_route *route;

  route = ospf_route_match (top, nexthop, IPV4_MAX_BITLEN);
  if (route != NULL)
    if (route->rn->p->prefixlen != 0)
      if (route->selected != NULL)
        if (route->selected->type == OSPF_PATH_CONNECTED
            || route->selected->type == OSPF_PATH_INTRA_AREA
            || route->selected->type == OSPF_PATH_INTER_AREA)
          return route;

  return NULL;
}

void
ospf_route_add (struct ospf *top, struct pal_in4_addr *addr,
                u_char length, struct ospf_route *route)
{
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set ((struct ls_prefix *)&p, length, *addr);
  rn = ls_node_get (top->rt_network, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_CURRENT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_CURRENT, route);
      route->rn = rn;
    }
  ls_unlock_node (rn);
}

void
ospf_route_install_update (struct ospf *top,
                           struct ospf_route *route,
                           struct ospf_area *area_old,
                           struct ospf_path *old)
{
  struct ospf_path *selected = route->selected;
  struct ls_node *rn = route->rn;
  struct ospf_area *area_new;

  /* Update Inter-Area-Prefix/Router LSAs.

     External and discard routes are ignored to avoid
     the IA summary LSA generation.  */
  if (selected == NULL
      || selected->type == OSPF_PATH_EXTERNAL
      || selected->type == OSPF_PATH_DISCARD)
    {
      if (area_old != NULL && old != NULL)
        {
          ospf_area_range_check_update (area_old, rn->p, old);
          ospf_abr_ia_prefix_delete_all (area_old, rn->p, old);
        }
    }
  else
    {
      area_new = ospf_area_entry_lookup (top, selected->area_id);
      if (area_new == NULL)
        return;

      if (area_old != NULL && old != NULL)
        if (area_old != area_new || old->type != selected->type)
          {
            ospf_area_range_check_update (area_old, rn->p, old);
            ospf_abr_ia_prefix_delete_all (area_old, rn->p, old);
          }
      ospf_area_range_check_update (area_new, rn->p, selected);
      ospf_abr_ia_prefix_update_all (area_new, rn->p, selected);
    }

  /* Update to the NSM.  */
  if (selected == NULL)
    {
      if (old)
        {
          OSPF_NSM_DELETE (top, rn->p, route, old);
          if (!vector_count (route->news))
            ospf_route_free (route);
        }
    }
  else
      OSPF_NSM_ADD (top, rn->p, route, selected);
}

#define OSPF_PATH_INSTALL_NONE          0
#define OSPF_PATH_INSTALL_ADD           1
#define OSPF_PATH_INSTALL_UPDATE        2
#define OSPF_PATH_INSTALL_DELETE        3

/* Generic function to install a route. */
int
ospf_route_install (struct ospf *top, struct ospf_area *area,
                    int type, struct ls_node *rn)
{ 
  struct ospf_redist_conf *ori;
  struct ospf_route *route = RN_INFO (rn, RNI_CURRENT);
  struct ospf_path *new, *old, *selected;
  struct ospf_nexthop *nh;
  struct ospf_area *area_old = NULL;
  int i;
  
  if (route == NULL)
    return OSPF_PATH_INSTALL_NONE;

#ifdef HAVE_RESTART
  /*  The restarting router does *not* install OSPF routes into
      the system's forwarding table(s) and relies on the forwarding
      entries that it installed prior to the restart. */
  if (IS_GRACEFUL_RESTART (top) || IS_RESTART_SIGNALING (top))
    return OSPF_PATH_INSTALL_NONE;
#endif /* HAVE_RESTART */

  new = ospf_route_path_lookup_new (route, type, area);
  old = ospf_route_path_lookup (route, type, area);
  selected = route->selected;
  ori = &top->dist_in;

  /* External and discard routes are ignored to avoid
     the IA summary LSA generation.  */
  if (selected != NULL
      && selected->type != OSPF_PATH_EXTERNAL
      && selected->type != OSPF_PATH_DISCARD)
    area_old = ospf_area_entry_lookup (top, selected->area_id);

  /* There is no path for this type of route. */
  if (old == NULL && new == NULL)
    {
      if (route->selected == NULL)
        if (!vector_count (route->news))
          ospf_route_free (route);

      return OSPF_PATH_INSTALL_NONE;
    }
  /* This path is removed. */
  else if (old != NULL && new == NULL)
    {
      if (ospf_route_path_update (top, route, NULL, old))
        ospf_route_install_update (top, route, area_old, selected);
      
      ospf_path_unlock (old);
      return OSPF_PATH_INSTALL_DELETE;
    }
  /* This path is created. */
  else if (old == NULL && new != NULL)
    { 
      for (i = 0; i < vector_max (new->nexthops); i++)
        if ((nh = vector_slot (new->nexthops, i)))
          if (! CHECK_FLAG (nh->flags, OSPF_NEXTHOP_CONNECTED))
            {
              if (CHECK_FLAG (top->dist_in.flags,OSPF_DIST_LIST_IN))
                if (DIST_LIST (ori) &&
                    access_list_apply (DIST_LIST (ori),
                                       rn->p) != FILTER_PERMIT)
                    return OSPF_PATH_INSTALL_NONE;
            }    
      if (ospf_route_path_add (top, route, new))
        ospf_route_install_update (top, route, area_old, selected);
      
      return OSPF_PATH_INSTALL_ADD;
    }
  /* This path is updated. */
  else if (old != NULL && new != NULL)
    { 
      for (i = 0; i < vector_max (new->nexthops); i++)
        if ((nh = vector_slot (new->nexthops, i)))
          if (! CHECK_FLAG (nh->flags, OSPF_NEXTHOP_CONNECTED))
            {
              if (CHECK_FLAG (top->dist_in.flags,OSPF_DIST_LIST_IN))
                if (DIST_LIST (ori) &&
                    access_list_apply (DIST_LIST (ori),
                                       rn->p) != FILTER_PERMIT)
                  {
                    OSPF_NSM_DELETE (top, rn->p, route, new);
                    if (!vector_count (route->news))
                      ospf_route_free (route);
                    return OSPF_PATH_INSTALL_DELETE;
                  }
            } 
      /* But the same as previous one.  */
      if (ospf_path_same (new, old))
        {
          /* Swap LSA vector. */
          ospf_path_swap (old, new);
          if (area)
            if (selected == old)
              {
                ospf_area_range_check_update (area, rn->p, old);
                ospf_abr_ia_prefix_update_all (area, rn->p, old);
              }

#ifdef HAVE_MPLS
          if (old->t_lsp != new->t_lsp)
            {
              /* after swap. */
              ospf_nsm_igp_shortcut_route_del (top, rn->p, route, new);
              ospf_nsm_igp_shortcut_route_add (top, rn->p, route);
            }
#endif /* HAVE_MPLS */

          ospf_path_unlock (new);
        }
      else
        {
          if (ospf_route_path_update (top, route, new, old))
            {
#ifdef HAVE_MPLS
              if (old->t_lsp != new->t_lsp)
                ospf_nsm_igp_shortcut_route_del (top, rn->p, route, old);
#endif /* HAVE_MPLS */
              ospf_route_install_update (top, route, area_old, selected);
            }

          ospf_path_unlock (old);
          return OSPF_PATH_INSTALL_UPDATE;
        }
    }

  return OSPF_PATH_INSTALL_NONE;
}

void
ospf_route_install_all (struct ospf *top, struct ospf_area *area, int type)
{
  struct ls_node *rn;

  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_CURRENT))
      ospf_route_install (top, area, type, rn);
}

int
ospf_route_install_all_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  int type = THREAD_VAL (thread);
  struct ospf_route_calc *rt_calc = area->rt_calc;
  struct thread *thread_self = rt_calc->t_rt_calc[type];
  struct ospf *top = area->top;
  struct ls_node *rn;
  int count = 0;
  struct ospf_master *om = top->om;

  rt_calc->t_rt_calc[type] = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Wait for the other suspended thread.  */
  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_route_install_all_event, area, type);

      return 0;
    }

  for (rn = OSPF_RT_HEAD (area, type); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_CURRENT))
      {
        if (count > OSPF_RT_CALC_COUNT_THRESHOLD)
          {
            /* Schedule the next calculation event.  */
            OSPF_EVENT_SUSPEND_ON (rt_calc->t_rt_calc[type],
                                   ospf_route_install_all_event, area, type);

            rt_calc->rn_next[type] = rn;

            return 1;
          }
        if (ospf_route_install (top, area, type, rn))
          count++;
      }

  /* Reset the suspend thread pointer.  */
  if (area->t_suspend == thread_self)
    area->t_suspend = NULL;

  /* Reset the next pointer.  */
  rt_calc->rn_next[type] = NULL;

  /* Execute the pending event.  */
  OSPF_EVENT_PENDING_EXECUTE (area->event_pend);

  return 0;
}

void
ospf_route_install_all_event_reset (struct ospf_area *area, int type)
{
  struct ospf_route_calc *rt_calc = area->rt_calc;
  struct ospf_master *om = area->top->om;

  if (IS_AREA_THREAD_SUSPENDED (area, rt_calc->t_rt_calc[type]))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_INSTALL))
        zlog_info (ZG, "RT[Install:%r]: Restart the route install event",
                   &area->area_id);

      /* Reset the next route candidate pointer.  */
      if (rt_calc->rn_next[type] != NULL)
        {
          ls_unlock_node (rt_calc->rn_next[type]);
          rt_calc->rn_next[type] = NULL;
        }
    }
  OSPF_EVENT_SUSPEND_OFF (rt_calc->t_rt_calc[type], area);
}

void
ospf_route_install_all_event_add (struct ospf_area *area, int type)
{
  if (area->rt_calc->t_rt_calc[type] == NULL)
    area->rt_calc->t_rt_calc[type] =
      thread_add_event (ZG, ospf_route_install_all_event, area, type);
}


struct ospf_route *
ospf_route_asbr_lookup (struct ospf *top, struct pal_in4_addr router_id)
{
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, router_id);
  rn = ls_node_lookup (top->rt_asbr, (struct ls_prefix *)&p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_CURRENT);
    }
  return NULL;
}

struct ospf_path *
ospf_route_path_asbr_lookup_by_area (struct ospf_area *area,
                                     struct pal_in4_addr router_id)
{
  struct ospf_route *route;
  struct ospf_path *path;

  route = ospf_route_asbr_lookup (area->top, router_id);
  if (route != NULL)
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_INTRA_AREA, area);
      if (path != NULL)
        return path;
    }
  return NULL;
}

void
ospf_route_asbr_add (struct ospf *top,
                     struct pal_in4_addr router_id, struct ospf_route *route)
{
  struct ls_node *rn, *rn1, *rn2;
  struct ls_prefix p, p1;
  struct ospf_lsa *lsa;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, router_id);
  rn = ls_node_get (top->rt_asbr, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_CURRENT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_CURRENT, route);
      route->rn = rn;
    }
  ls_unlock_node (rn);
  if (top->lsdb->type[OSPF_AS_EXTERNAL_LSA].db != NULL)
     for (rn1 = ls_table_top (top->lsdb->type[OSPF_AS_EXTERNAL_LSA].db); rn1; 
          rn1 = ls_route_next (rn1))
       {
         if ((lsa = rn1->vinfo[0]))
           {   
             if (!IS_LSA_SELF (lsa)
                 && !IS_LSA_MAXAGE (lsa))
               {
                 ls_prefix_ipv4_set (&p1, IPV4_MAX_BITLEN, 
                                     lsa->data->adv_router);
                 rn2 = ls_node_lookup (top->rt_asbr, (struct ls_prefix *)&p1);
                 if (rn2)
                   {
                     /* Refresh self originated equivalent LSA.  */
                     ospf_external_lsa_equivalent_update_self (top->redist_table,                                                              lsa); 
                     ls_unlock_node (rn2);
                   }
               }
           }
       }
}

void
ospf_route_asbr_install_update (struct ospf *top,
                                struct ospf_route *route,
                                struct ospf_area *area_old,
                                struct ospf_path *old)
{
  struct ospf_path *selected = route->selected;
  struct ls_node *rn = route->rn;
  struct ospf_area *area_new = NULL;

  if (selected != NULL)
    {
      area_new = ospf_area_entry_lookup (top, selected->area_id);
      if (area_new == NULL)
        return;

      if (area_old != NULL && old != NULL)
        if (area_old != area_new && old->type != selected->type)
          ospf_abr_ia_router_delete_all (area_old, rn->p, old);

      ospf_abr_ia_router_update_all (area_new, rn->p, selected);
    }
  else
    {
      if (area_old != NULL && old != NULL)
        {
          ospf_abr_ia_router_delete (area_old, rn->p);
          ospf_abr_ia_router_delete_all (area_old, rn->p, old);
        }
      ospf_redistribute_timer_add_all_proto (top);
      if (!vector_count (route->news))
        ospf_route_free (route);
    }
  ospf_ase_calc_event_add (top);
}

int
ospf_route_asbr_install (struct ospf *top, struct ospf_area *area,
                         int type, struct ls_node *rn)
{
  struct ospf_route *route = RN_INFO (rn, RNI_CURRENT);
  struct ospf_path *new, *old, *selected;
  struct ospf_area *area_old = NULL;

  if (route == NULL)
    return OSPF_PATH_INSTALL_NONE;
  
  new = ospf_route_path_lookup_new (route, type, area);
  old = ospf_route_path_lookup (route, type, area);
  selected = route->selected;

  if (selected != NULL)
    area_old = ospf_area_entry_lookup (top, selected->area_id);

  /* There is no path for this type of route. */
  if (old == NULL && new == NULL)
    {
      if (route->selected == NULL)
        if (!vector_count (route->news))
          {
            ospf_redistribute_timer_add_all_proto (top);
            ospf_route_free (route);
          }
      return OSPF_PATH_INSTALL_NONE;
    }
  /* This path is removed. */
  else if (old != NULL && new == NULL)
    {
      if (ospf_route_path_update (top, route, NULL, old))
        ospf_route_asbr_install_update (top, route, area_old, selected);

      ospf_path_unlock (old);
      return OSPF_PATH_INSTALL_DELETE;
    }
  /* This path is created. */
  else if (old == NULL && new != NULL)
    {
      if (ospf_route_path_add (top, route, new))
        ospf_route_asbr_install_update (top, route, area_old, selected);
      
      return OSPF_PATH_INSTALL_ADD;
    }
  /* This path is updated. */
  else if (old != NULL && new != NULL)
    {
      /* But the same as previous one.  */
      if (ospf_path_same (new, old))
        {
          /* Swap LSA vector. */
          ospf_path_swap (old, new);
          ospf_path_unlock (new);
        }
      else
        {
          if (ospf_route_path_update (top, route, new, old))
            ospf_route_asbr_install_update (top, route, area_old, selected);

          ospf_path_unlock (old);
          return OSPF_PATH_INSTALL_UPDATE;
        }
    }
  
  return OSPF_PATH_INSTALL_NONE;
}

void
ospf_route_asbr_install_all (struct ospf *top,
                             struct ospf_area *area, int type)
{
  struct ls_node *rn;

  for (rn = ls_table_top (top->rt_asbr); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_CURRENT))
      ospf_route_asbr_install (top, area, type, rn);
}

int
ospf_route_asbr_install_all_event (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  int type = THREAD_VAL (thread);
  struct ospf_route_calc *rt_calc = area->rt_calc;
  struct thread *thread_self = rt_calc->t_rt_calc_asbr[type];
  struct ospf *top = area->top;
  struct ls_node *rn;
  int count = 0;
  struct ospf_master *om = top->om;

  rt_calc->t_rt_calc_asbr[type] = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Wait for the other suspended thread.  */
  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_route_asbr_install_all_event, area, type);
      return 0;
    }

  for (rn = OSPF_RT_HEAD_ASBR (area, type); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_CURRENT))
      {
        if (count > OSPF_RT_CALC_COUNT_THRESHOLD)
          {
            /* Schedule the next calculation event.  */
            OSPF_EVENT_SUSPEND_ON (rt_calc->t_rt_calc_asbr[type],
                                   ospf_route_asbr_install_all_event,
                                   area, type);

            rt_calc->rn_next_asbr[type] = rn;

            return 1;
          }
        if (ospf_route_asbr_install (top, area, type, rn))
          count++;
      }

  /* Reset the suspend thread pointer.  */
  if (area->t_suspend == thread_self)
    area->t_suspend = NULL;

  /* Reset the next pointer.  */
  rt_calc->rn_next_asbr[type] = NULL;

  /* Execute the pending event.  */
  OSPF_EVENT_PENDING_EXECUTE (area->event_pend);

#ifdef HAVE_NSSA
  /* NSSA Translator Election. */
  if (IS_OSPF_ABR (top))
    if (IS_AREA_NSSA (area))
      ospf_abr_nssa_translator_state_update (area);
#endif /* HAVE_NSSA */

  return 0;
}

void
ospf_route_asbr_install_all_event_reset (struct ospf_area *area, int type)
{
  struct ospf_route_calc *rt_calc = area->rt_calc;
  struct ospf_master *om = area->top->om;

  if (IS_AREA_THREAD_SUSPENDED (area, rt_calc->t_rt_calc_asbr[type]))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_INSTALL))
        zlog_info (ZG, "RT[Install:%r]: Restart the route install event",
                   &area->area_id);

      /* Reset the next route candidate pointer.  */
      if (rt_calc->rn_next_asbr[type] != NULL)
        {
          ls_unlock_node (rt_calc->rn_next_asbr[type]);
          rt_calc->rn_next_asbr[type] = NULL;
        }
    }
  OSPF_EVENT_SUSPEND_OFF (rt_calc->t_rt_calc_asbr[type], area);
}

void
ospf_route_asbr_install_all_event_add (struct ospf_area *area, int type)
{
  if (area->rt_calc->t_rt_calc_asbr[type] == NULL)
    area->rt_calc->t_rt_calc_asbr[type] =
      thread_add_event (ZG, ospf_route_asbr_install_all_event, area, type);
}


void
ospf_route_add_connected (struct ospf_interface *oi, struct prefix *p)
{
  struct ospf_route *route;
  struct ospf_path *path;
  struct ospf_nexthop *nh;

  route = ospf_route_lookup (oi->top, &p->u.prefix4, p->prefixlen);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_add (oi->top, &p->u.prefix4, p->prefixlen, route);
    }

  path = ospf_path_new (OSPF_PATH_CONNECTED);
  path->cost = oi->output_cost;
  path->area_id = oi->area->area_id;
  ospf_route_path_set_new (route, path);

  nh = ospf_nexthop_get (oi->area, oi, p->u.prefix4);
  SET_FLAG (nh->flags, OSPF_NEXTHOP_CONNECTED);
  ospf_path_nexthop_add (path, nh);
  ospf_nexthop_unlock (nh);

  /* Install the connected route.  */
  ospf_route_install (oi->top, oi->area, path->type, route->rn);
}

void
ospf_route_delete_connected (struct ospf_interface *oi, struct prefix *p)
{
  struct ospf_route *route;

  route = ospf_route_lookup (oi->top, &p->u.prefix4, p->prefixlen);
  if (route != NULL)
    ospf_route_install (oi->top, oi->area, OSPF_PATH_CONNECTED, route->rn);
}

void
ospf_route_add_discard (struct ospf *top,
                        struct ospf_area *area, struct ls_prefix *p)
{
  struct ospf_route *route;
  struct ospf_path *path;

  route = ospf_route_lookup (top, (struct pal_in4_addr *)&p->prefix,
                             p->prefixlen);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_add (top, (struct pal_in4_addr *)p->prefix,
                      p->prefixlen, route);
    }

  path = ospf_path_new (OSPF_PATH_DISCARD);
  path->cost = 0;
  if (area != NULL)
    path->area_id = area->area_id;
  ospf_route_path_set_new (route, path);

  /* Install the discard route.  */
  ospf_route_install (top, area, OSPF_PATH_DISCARD, route->rn);
}

void
ospf_route_delete_discard (struct ospf *top,
                           struct ospf_area *area, struct ls_prefix *p)
{
  struct ospf_route *route;

  route = ospf_route_lookup (top, (struct pal_in4_addr *)p->prefix,
                             p->prefixlen);
  if (route != NULL)
    ospf_route_install (top, area, OSPF_PATH_DISCARD, route->rn);
}

/* RFC2328 16.1. (4).  For transit network. */
struct ospf_route *
ospf_route_add_transit (struct ospf_area *area, struct ospf_vertex *v)
{
  struct ospf_master *om = area->top->om;
  struct ospf_route *route;
  struct ospf_path *path;
  struct ospf_lsa *lsa;
  struct pal_in4_addr *mask;
  struct pal_in4_addr addr;
  u_char masklen;
  int i;

  mask = OSPF_PNT_IN_ADDR_GET (v->lsa->data, OSPF_NETWORK_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);
  addr.s_addr = v->lsa->data->id.s_addr & mask->s_addr;

  if (IS_DEBUG_OSPF (rt_calc, RT_INSTALL))
    zlog_info (ZG, "RT[Install:%r]: %r/%d, cost(%d) transit network",
               &area->area_id, &addr, masklen, v->distance);

  route = ospf_route_lookup (area->top, &addr, masklen);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_add (area->top, &addr, masklen, route);
    }

  path = ospf_route_path_lookup_new (route, OSPF_PATH_INTRA_AREA, area);
  if (path != NULL)
    {
      /*  If the routing table entry already exists
          (i.e., there is already an intra-area route to the
          destination installed in the routing table), multiple
          vertices have mapped to the same IP network.  For example,
          this can occur when a new Designated Router is being
          established.  In this case, the current routing table entry
          should be overwritten if and only if the newly found path is
          just as short and the current routing table entry's Link
          State Origin has a smaller Link State ID than the newly
          added vertex' LSA.  */

      /* The new path is better.  */
      if (path->cost > v->distance)
        {
          ospf_path_unlock (path);
          path = NULL;
        }

      /* The existing path is better.  */
      else if (path->cost < v->distance)
        {
          ospf_route_path_set_new (route, path);
          return NULL;
        }

      else
        for (i = 0; i < vector_max (path->lsa_vec); i++)
          if ((lsa = vector_slot (path->lsa_vec, i)))
            {
              if (IPV4_ADDR_CMP (&lsa->data->id, &v->lsa->data->id) > 0)
                {
                  vector_set (path->lsa_vec, ospf_lsa_lock (v->lsa));
                  ospf_route_path_set_new (route, path);
                  return NULL;
                }
              else if (IPV4_ADDR_CMP (&lsa->data->id, &v->lsa->data->id) < 0)
                {
                  ospf_path_unlock (path);
                  path = NULL;
                  break;
                }          
            }
    }
  
  if (path == NULL)
    path = ospf_path_new (OSPF_PATH_INTRA_AREA);
  path->cost = v->distance;
  path->area_id = area->area_id;
  vector_set (path->lsa_vec, ospf_lsa_lock (v->lsa));
  ospf_route_path_set_new (route, path);

#ifdef HAVE_MPLS
  path->t_cost = v->t_distance;
#endif /* HAVE_MPLS */

  /* Copy nexthops from vertex. */
  ospf_path_nexthop_copy_all (area->top, path, v);

  return route;
}

struct ospf_route *
ospf_route_add_stub (struct ospf_area *area, struct ospf_vertex *v,
                     struct pal_in4_addr addr, u_char masklen,
                     u_int16_t metric)
{
  struct ospf_master *om = area->top->om;
  u_int32_t cost = v->distance + metric;
  struct ospf_route *route;
  struct ospf_path *path;

  if (IS_DEBUG_OSPF (rt_calc, RT_INSTALL))
    zlog_info (ZG, "RT[Install:%r]: %r/%d, cost(%d) stub network",
               &area->area_id, &addr, masklen, cost);

  route = ospf_route_lookup (area->top, &addr, masklen);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_add (area->top, &addr, masklen, route);
    }

  path = ospf_route_path_lookup_new (route, OSPF_PATH_INTRA_AREA, area);
  if (path != NULL)
    {
      /* The new path is better.  */
      if (path->cost > cost)
        {
          ospf_path_unlock (path);
          path = NULL;
        }

      /* The existing path is better.  */
      else if (path->cost < cost)
        {
          ospf_route_path_set_new (route, path);
          return NULL;
        }
    }

  if (path == NULL)
    path = ospf_path_new (OSPF_PATH_INTRA_AREA);
  path->cost = cost;
  path->area_id = area->area_id;
  vector_set (path->lsa_vec, ospf_lsa_lock (v->lsa));
  ospf_route_path_set_new (route, path);

#ifdef HAVE_MPLS
  path->t_cost = v->t_distance;
#endif /* HAVE_MPLS */

  /* Copy nexthops from vertex. */
  ospf_path_nexthop_copy_all (area->top, path, v);

  return route;
}

struct ospf_route *
ospf_route_add_intra_asbr (struct ospf_area *area, struct ospf_vertex *v)
{
  struct ospf_route *route;
  struct ospf_path *path;
  u_char bits;

  bits = OSPF_PNT_UCHAR_GET (v->lsa->data, OSPF_ROUTER_LSA_BITS_OFFSET);

  route = ospf_route_asbr_lookup (area->top, v->lsa->data->adv_router);
  if (route == NULL)
    {
      route = ospf_route_new ();
      ospf_route_asbr_add (area->top, v->lsa->data->adv_router, route);
    }

  path = ospf_route_path_lookup_new (route, OSPF_PATH_INTRA_AREA, area);
  if (path != NULL)
    {
      /* The new path is better.  */
      if (path->cost > v->distance)
        {
          ospf_path_unlock (path);
          path = NULL;
        }

      /* The existing path is better.  */
      else if (path->cost < v->distance)
        {
          ospf_route_path_set_new (route, path);
          return NULL;
        }
    }

  if (path == NULL)
    path = ospf_path_new (OSPF_PATH_INTRA_AREA);
  path->cost = v->distance;
  path->area_id = area->area_id;
  SET_FLAG (path->flags, OSPF_PATH_ASBR);
  ospf_route_path_set_new (route, path);

  /* This is only for "show ip ospf border-router". */
  if (IS_ROUTER_LSA_SET (bits, BIT_B))
    SET_FLAG (path->flags, OSPF_PATH_ABR);

  /* Copy nexthops from vertex. */
  ospf_path_nexthop_copy_all (area->top, path, v);

  return route;
}

void
ospf_route_add_spf (struct ospf_area *area, struct ospf_vertex *v)
{
  if (v->lsa->data->type == OSPF_ROUTER_LSA)
    {
      u_char bits = OSPF_PNT_UCHAR_GET (v->lsa->data,
                                        OSPF_ROUTER_LSA_BITS_OFFSET);

      if (IS_ROUTER_LSA_SET (bits, BIT_B))
        area->abr_count++;
      else
        ospf_nexthop_abr_down (area, v->lsa->data->adv_router);

      if (IS_ROUTER_LSA_SET (bits, BIT_E))
        {
          area->asbr_count++;
          ospf_route_add_intra_asbr (area, v);
        }

      if (IS_ROUTER_LSA_SET (bits, BIT_V))
        { 
          SET_FLAG (area->flags, OSPF_AREA_TRANSIT);
          if (area->top->backbone != NULL)
            ospf_spf_calculate_timer_add (area->top->backbone);
        }		
    }
  else /* if (v->lsa->data->type == OSPF_NETWORK_LSA) */
    {
      ospf_route_add_transit (area, v);
    }
}


void
ospf_path_lsa_set (struct ospf_path *path, struct ospf_lsa *lsa)
{
  int i;

  /* Check if LSA is already set. */
  for (i = 0; i < vector_max (path->lsa_vec); i++)
    if (vector_slot (path->lsa_vec, i) == lsa)
      return;

  vector_set (path->lsa_vec, ospf_lsa_lock (lsa));
}

void
ospf_path_lsa_unset (struct ospf_path *path, struct ospf_lsa *lsa)
{
  int i;

  for (i = 0; i < vector_max (path->lsa_vec); i++)
    if (vector_slot (path->lsa_vec, i))
      if (vector_slot (path->lsa_vec, i) == lsa)
        {
          ospf_lsa_unlock (lsa);
          vector_slot (path->lsa_vec, i) = NULL;
        }
}

void
ospf_route_path_summary_lsa_set (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_route *route;
  struct ospf_path *path;
  struct pal_in4_addr *mask;
  u_char masklen;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  route = ospf_route_lookup (top, &lsa->data->id, masklen);
  if (route != NULL)
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_INTER_AREA,
                                     lsa->lsdb->u.area);
      if (path != NULL)
        ospf_path_lsa_set (path, lsa);
    }
}

void
ospf_route_path_summary_lsa_unset (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_route *route;
  struct ospf_path *path;
  struct pal_in4_addr *mask;
  u_char masklen;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_SUMMARY_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  route = ospf_route_lookup (top, &lsa->data->id, masklen);
  if (route != NULL)
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_INTER_AREA,
                                     lsa->lsdb->u.area);
      if (path != NULL)
        ospf_path_lsa_unset (path, lsa);
    }
}

void
ospf_route_path_asbr_summary_lsa_set (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_route *route;
  struct ospf_path *path;

  route = ospf_route_asbr_lookup (top, lsa->data->id);
  if (route != NULL)
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_INTER_AREA,
                                     lsa->lsdb->u.area);
      if (path != NULL)
        ospf_path_lsa_set (path, lsa);
    }
}

void
ospf_route_path_asbr_summary_lsa_unset (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_route *route;
  struct ospf_path *path;

  route = ospf_route_asbr_lookup (top, lsa->data->id);
  if (route != NULL)
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_INTER_AREA,
                                     lsa->lsdb->u.area);
      if (path != NULL)
        ospf_path_lsa_unset (path, lsa);
    }
}

void
ospf_route_path_external_lsa_set (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_route *route;
  struct ospf_path *path;
  struct pal_in4_addr *mask;
  u_char masklen;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  route = ospf_route_lookup (top, &lsa->data->id, masklen);
  if (route != NULL)
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_EXTERNAL, NULL);
      if (path != NULL)
        ospf_path_lsa_set (path, lsa);
    }
}

void
ospf_route_path_external_lsa_unset (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_route *route;
  struct ospf_path *path;
  struct pal_in4_addr *mask;
  u_char masklen;

  mask = OSPF_PNT_IN_ADDR_GET (lsa->data, OSPF_EXTERNAL_LSA_NETMASK_OFFSET);
  masklen = ip_masklen (*mask);

  route = ospf_route_lookup (top, &lsa->data->id, masklen);
  if (route != NULL)
    {
      path = ospf_route_path_lookup (route, OSPF_PATH_EXTERNAL, NULL);
      if (path != NULL)
        ospf_path_lsa_unset (path, lsa);
    }
}

void
ospf_route_clean_invalid_event_add (struct ospf *top)
{
  int intdummy = 0;

  if (top->t_route_cleanup == NULL)
    top->t_route_cleanup = thread_add_event (ZG, ospf_route_clean_invalid,
                                             top, intdummy);
}
  
void
ospf_route_clean_invalid_update (struct ospf *top, struct ospf_route *route,
                                 struct ospf_path *old, struct ls_node *rn)
{
  struct ospf_area *area;

  if (ospf_route_path_update (top, route, NULL, old))
    {
      if (route->selected == NULL)
        OSPF_NSM_DELETE (top, rn->p, route, old);
      else
        OSPF_NSM_ADD (top, rn->p, route, route->selected);

      if (old->type == OSPF_PATH_CONNECTED
          || old->type == OSPF_PATH_INTRA_AREA
          || old->type == OSPF_PATH_INTER_AREA)
        area = ospf_area_entry_lookup (top, old->area_id);
      else
        area = NULL;

      if (area)
        {
          if (route->selected == NULL
              || (route->selected->type != OSPF_PATH_CONNECTED
                  && route->selected->type != OSPF_PATH_INTRA_AREA
                  && route->selected->type != OSPF_PATH_INTER_AREA))
            {
              ospf_area_range_check_update (area, rn->p, old);
              ospf_abr_ia_prefix_delete_all (area, rn->p, old);
            }
          else
            {
              ospf_area_range_check_update (area, rn->p, route->selected);
              ospf_abr_ia_prefix_update_all (area, rn->p, route->selected);
            }
        }
    }

  /* Unlock the old path.  */
  ospf_path_unlock (old);

  /* Free the route.  */
  if (route->selected == NULL)
    ospf_route_free (route);
}

void
ospf_route_clean_invalid_transit_update (struct ospf *top,
                                         struct ospf_route *route,
                                         struct ospf_path *old,
                                         struct ls_node *rn)
{
  struct ospf_area *area;

  /* The cost and/or the nexthop will be changed.  */
  if (old->path_transit->cost < old->cost)
    {
      /* Notify to the NSM.  */
      OSPF_NSM_ADD (top, rn->p, route, route->selected);

      if (old->type == OSPF_PATH_CONNECTED
          || old->type == OSPF_PATH_INTRA_AREA
          || old->type == OSPF_PATH_INTER_AREA)
        if ((area = ospf_area_entry_lookup (top, old->area_id)))
          {
            ospf_area_range_check_update (area, rn->p, route->selected);
            ospf_abr_ia_prefix_update_all (area, rn->p, route->selected);
          }
    }
}

int
ospf_route_clean_invalid (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_route *route;
  struct ospf_path *old;
  struct ls_node *rn;
  struct ospf_area *area;
  int type;
  struct ospf_master *om = top->om;

  top->t_route_cleanup = NULL;

  /* Set VR Context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

    /* For all areas, all types, check if spf calculation is suspended */
    for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    {
      if ((area = RN_INFO (rn, RNI_DEFAULT)))
      {
        for (type = 0; type < OSPF_PATH_MAX ; type++)
        {
          if (IS_AREA_ACTIVE (area))
          {
            if (IS_AREA_SUSPENDED (area, area->rt_calc->t_rt_calc[type]))
            {
             /* If spf calc is suspended, suspend the route cleanup */
             OSPF_EVENT_SUSPEND_ON_TOP (top->t_route_cleanup,
                                        ospf_route_clean_invalid, top, 0);
             return 0;
            }
          }
        }
      }
    }

  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_CURRENT)))
      if ((old = route->selected))
        {
          if (!ospf_path_valid (old))
            ospf_route_clean_invalid_update (top, route, old, rn);
          else if (old->path_transit && !ospf_path_valid (old->path_transit))
            ospf_route_clean_invalid_transit_update (top, route, old, rn);
        }
  return 0;
}

void
ospf_route_table_withdraw (struct ospf *top)
{
  struct ospf_route *route;
  struct ospf_path *path;
  struct ls_node *rn;

  for (rn = ls_table_top (top->rt_network); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_CURRENT)))
      if ((path = route->selected))
        OSPF_NSM_DELETE (top, rn->p, route, path);
}

void
ospf_route_table_finish (struct ls_table *table)
{
  struct ospf_route *route;
  struct ls_node *rn;

  for (rn = ls_table_top (table); rn; rn = ls_route_next (rn))
    if ((route = RN_INFO (rn, RNI_CURRENT)))
      {
        RN_INFO_UNSET (rn, RNI_CURRENT);
        ospf_route_free (route);
      }
}


void
ospf_route_calc_route_clean (struct ospf_area *area)
{
  int type;

  /* Cleanup the all area specific routes.  */
  for (type = 0; type < OSPF_PATH_MAX; type++)
    {
      /* Cancel threads.  */
      ospf_route_install_all_event_reset (area, type);
      ospf_route_asbr_install_all_event_reset (area, type);

      /* Make sure to cleanup all the routes.  */
      ospf_route_install_all (area->top, area, type);
      ospf_route_asbr_install_all (area->top, area, type);
    }
}

struct ospf_route_calc *
ospf_route_calc_init (void)
{
  struct ospf_route_calc *new;

  new = XCALLOC (MTYPE_OSPF_ROUTE_CALC, sizeof (struct ospf_route_calc));

  return new;
}

void
ospf_route_calc_finish (struct ospf_area *area)
{
  if (area->rt_calc != NULL)
    {
      /* Make sure to cleanup the routing table.  */
      ospf_route_calc_route_clean (area);

      XFREE (MTYPE_OSPF_ROUTE_CALC, area->rt_calc);

      area->rt_calc = NULL;
    }
}
