/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "sockunion.h"
#include "linklist.h"
#include "log.h"
#include "snprintf.h"
#include "bheap.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_neighbor.h"
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
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_MPLS
#include "ospfd/ospf_mpls.h"
#endif /* HAVE_MPLS */


void
ospf_vertex_set_parent (struct ospf_vertex *v, struct ospf_vertex *parent)
{
  struct ospf_vertex *pv;
  int i;

  for (i = 0; i < vector_max (v->parent); i++)
    if ((pv = vector_slot (v->parent, i)))
      if (pv == parent)
        return;

  vector_set (v->parent, parent);
}

void
ospf_vertex_set_nexthop (struct ospf_vertex *v, struct ospf_nexthop *nh)
{
  struct ospf_nexthop *mh;
  int i;

  for (i = 0; i < vector_max (v->nexthop); i++)
    if ((mh = vector_slot (v->nexthop, i)))
      if (mh == nh)
        return;

  vector_set (v->nexthop, ospf_nexthop_lock (nh));
}


struct ospf_vertex *
ospf_vertex_new (struct ospf_lsa *lsa)
{
  struct ospf_vertex *new;
  
  new = XMALLOC (MTYPE_OSPF_VERTEX, sizeof (struct ospf_vertex));
  pal_mem_set (new, 0, sizeof (struct ospf_vertex));

  new->lsa = ospf_lsa_lock (lsa);
  new->child = vector_init (OSPF_VECTOR_SIZE_MIN);
  new->nexthop = vector_init (OSPF_VECTOR_SIZE_MIN);
  new->parent = vector_init (OSPF_VECTOR_SIZE_MIN);
  new->status = &(lsa->status);
  
  return new;
}

void
ospf_vertex_free (struct ospf_vertex *v)
{
  struct ospf_nexthop *nh;
  int i;

  if (v->lsa)
    ospf_lsa_unlock (v->lsa);

  for (i = 0; i < vector_max (v->nexthop); i++)
    if ((nh = vector_slot (v->nexthop, i)))
      ospf_nexthop_unlock (nh);

  vector_free (v->nexthop);
  vector_free (v->child);
  vector_free (v->parent);

  XFREE (MTYPE_OSPF_VERTEX, v);
}

struct ospf_vertex *
ospf_vertex_lookup (vector vec, struct ospf_vertex *v)
{
  int i;

  for (i = 0; i < vector_max (vec); i++)
    if ((vector_slot (vec, i)))
      if ((vector_slot (vec, i) == v))
        return vector_slot (vec, i);

  return NULL;
}

int
ospf_vertex_cmp (void *v1, void *v2)
{
  struct ospf_vertex *a = (struct ospf_vertex *)v1;
  struct ospf_vertex *b = (struct ospf_vertex *)v2;
  int ret;

  ret = a->distance - b->distance;
  if (ret != 0)
    return ret;

  return b->lsa->data->type - a->lsa->data->type;
}

void
ospf_vertex_add_parent (struct ospf_vertex *v)
{
  struct ospf_vertex *pv;
  int i;

  for (i = 0; i < vector_max (v->parent); i++)
    if ((pv = vector_slot (v->parent, i)))
      if (!ospf_vertex_lookup (pv->child, v))
        vector_set (pv->child, v);
}


struct ospf_vertex *
ospf_spf_init (struct ospf_area *area)
{
  struct ospf_lsa *lsa = ospf_router_lsa_self (area);

  area->spf = ospf_vertex_new (lsa);
  area->abr_count = 0;
  area->asbr_count = 0;

  /* Set Area A's TransitCapability to FALSE
     and set Area A's ShortCutCapability to TRUE.  */
  area->oflags = area->flags;
  UNSET_FLAG (area->flags, OSPF_AREA_TRANSIT);
  SET_FLAG (area->flags, OSPF_AREA_SHORTCUT);

  return area->spf;
}

struct ospf_vertex *
ospf_spf_vertex_lookup (struct ospf_area *area,
                        struct pal_in4_addr router_id)
{
  struct ls_prefix lp;
  struct ls_node *rn;

  ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN, router_id);

  rn = ls_node_info_lookup (area->vertices, &lp,
                            RNI_VERTEX_INDEX (OSPF_VERTEX_ROUTER));
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_VERTEX_INDEX (OSPF_VERTEX_ROUTER));
    }
  return NULL;
}

struct ospf_vertex *
ospf_spf_vertex_lookup_by_lsa (struct ospf_area *area,
                               struct ospf_lsa *lsa)
{
  struct ls_prefix lp;
  struct ls_node *rn;

  ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN, lsa->data->id);
  rn = ls_node_info_lookup (area->vertices, &lp,
                            RNI_VERTEX_INDEX (lsa->data->type));
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_VERTEX_INDEX (lsa->data->type));
    }
  return NULL;
}

void
ospf_spf_install_candidate (struct bin_heap *candidate, struct ospf_vertex *w)
{
  bheap_insert (candidate, w);
}

int
ospf_spf_check_link_back (struct ospf_lsa *w_lsa, struct ospf_lsa *v_lsa)
{
  char *p;
  char *lim;

  p = ((u_char *)w_lsa->data) + OSPF_LSA_HEADER_SIZE + 4;
  lim = ((u_char *) w_lsa->data) + pal_ntoh16 (w_lsa->data->length);

  /* In case of W is Network LSA. */
  if (w_lsa->data->type == OSPF_NETWORK_LSA)
    {
      if (v_lsa->data->type != OSPF_NETWORK_LSA)
        while (p < lim)
          {
            if (IPV4_ADDR_SAME (p, &v_lsa->data->id))
              return 1;

            p += sizeof (struct pal_in4_addr);
          }
    }
  /* In case of W is Router LSA. */
  else if (w_lsa->data->type == OSPF_ROUTER_LSA)
    {
      struct ospf_link_desc link;

      while (p + OSPF_LINK_DESC_SIZE <= lim)
        {
          OSPF_LINK_DESC_GET (p, link);

          if (link.type == LSA_LINK_TYPE_POINTOPOINT
              || link.type == LSA_LINK_TYPE_VIRTUALLINK)
            {
              if (v_lsa->data->type == OSPF_ROUTER_LSA)
                if (IPV4_ADDR_SAME (link.id, &v_lsa->data->id))
                  return 1;
            }
          else if (link.type == LSA_LINK_TYPE_TRANSIT)
            {
              if (v_lsa->data->type == OSPF_NETWORK_LSA)
                if (IPV4_ADDR_SAME (link.id, &v_lsa->data->id))
                  return 1;
            }

          p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;
        }
    }
  return 0;
}

void
ospf_spf_nexthop_calculate (struct ospf_area *area,
                            struct ospf_interface *oi,
                            struct pal_in4_addr *nbr_addr,
                            struct ospf_vertex *v,
                            struct ospf_vertex *w)
{
  struct ospf_master *om = area->top->om;
  struct ospf_nexthop *nh, *mh;
  int i;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zlog_info (ZG, "SPF[%r]:     Calculate nexthop for (%r)",
               &area->area_id, &w->lsa->data->id);

  ospf_vertex_set_parent (w, v);

  /* V (W's parent) is root. */
  if (oi != NULL)
    {
      /* Defer Nexthop calculation via virtual link
         until summary from transit area configured. */
      if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
        nh = ospf_nexthop_get (oi->u.vlink->out_oi->area,
                               oi, oi->u.vlink->peer_id);
      else
        {
          nh = ospf_nexthop_get (area, oi, nbr_addr
                                 ? *nbr_addr : oi->ident.address->u.prefix4);
          if (nbr_addr == NULL)
            SET_FLAG (nh->flags, OSPF_NEXTHOP_CONNECTED);
          else if (IS_OSPF_IPV4_UNNUMBERED (oi))
            SET_FLAG (nh->flags, OSPF_NEXTHOP_UNNUMBERED);
        }
      ospf_vertex_set_nexthop (w, nh);
      ospf_nexthop_unlock (nh);
    }
  /* V (W's parent) is network connected to root. */
  else if (CHECK_FLAG (v->flags, OSPF_VERTEX_NETWORK_CONNECTED))
    {
      for (i = 0; i < vector_max (v->nexthop); i++)
        if ((mh = vector_slot (v->nexthop, i)))
          {
            nh = ospf_nexthop_get (area, mh->oi, *nbr_addr);
            ospf_vertex_set_nexthop (w, nh);
            ospf_nexthop_unlock (nh);
          }
    }
  /* Inherit V's nexthop. */
  else
    {
      for (i = 0; i < vector_max (v->nexthop); i++)
        if ((mh = vector_slot (v->nexthop, i)))
          ospf_vertex_set_nexthop (w, mh);
    }
#ifdef HAVE_MPLS
  ospf_igp_shortcut_nexthop_calculate (v, w, area->top);
#endif /* HAVE_MPLS */
}

/* (d) Calculate the link state cost D of the resulting path
   from the root to vertex W.  D is equal to the sum of the link
   state cost of the (already calculated) shortest path to
   vertex V and the advertised cost of the link between vertices
   V and W.  If D is: */
void
ospf_spf_vertex_install (struct ospf_area *area, struct ospf_interface *oi,
                         struct pal_in4_addr *nbr_addr, struct ospf_lsa *w_lsa,
                         struct ospf_vertex *v, u_int16_t metric)
{
  struct ospf_vertex *w, *cw;
  struct ospf_nexthop *nh;
  int i, stat;
 
  stat = notEXPLORED;

  /* Prepare vertex W. */
  w = ospf_vertex_new (w_lsa);
  w->distance = v->distance + metric;
  w->t_distance = v->t_distance + metric;

  if (TV_CMP(area->tv_spf, w_lsa->tv_spf) == 0)
    stat = w_lsa->status;

  /* Mark W as connected network. */
  if (v == area->spf && w_lsa->data->type == OSPF_NETWORK_LSA)
    SET_FLAG (w->flags, OSPF_VERTEX_NETWORK_CONNECTED);

  /* Is there already vertex W in candidate list? */
  if(stat >= 0)
    cw = area->candidate->array[stat];
  else
    cw = NULL;

  if (cw == NULL)
    {
      /* Calculate nexthop to W. */
      w_lsa->tv_spf = area->tv_spf;
      ospf_spf_nexthop_calculate (area, oi, nbr_addr, v, w);
      ospf_spf_install_candidate (area->candidate, w);
    }
  else
    {
      /* If D is greater than. */
      if (cw->distance < w->distance)
        ospf_vertex_free (w);

      /* Equal to. */
      else if (cw->distance == w->distance)
        {
          /* Calculate nexthop to W. */
          ospf_spf_nexthop_calculate (area, oi, nbr_addr, v, w);

          for (i = 0; i < vector_max (w->nexthop); i++)
            if ((nh = vector_slot (w->nexthop, i)))
              {
                ospf_vertex_set_nexthop (cw, nh);
                vector_slot (w->nexthop, i) = NULL;
                ospf_nexthop_unlock (nh);
              }

          ospf_vertex_free (w);
        }
      /* Less than. */
      else
        {
          /* Calculate nexthop. */
          ospf_spf_nexthop_calculate (area, oi, nbr_addr, v, w);

          /* Remove old vertex from candidate list. */
          ospf_vertex_free (cw);
          area->candidate->array[stat] = w;
          bheap_percolate_up (area->candidate, stat);

        }
    }
}

int
ospf_spf_next_link_check (struct ospf_area *area,
                          struct ospf_vertex *v, struct ospf_lsa *w_lsa)
{
  struct ospf_master *om = area->top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];

  /* (b cont.) If the LSA does not exist, or its LS age is equal
     to MaxAge, or it does not have a link back to vertex V,
     examine the next link in V's LSA.[23] */
  if (w_lsa == NULL
      || IS_LSA_MAXAGE (w_lsa)
      || CHECK_FLAG (w_lsa->flags, OSPF_LSA_DISCARD))
    return 0;

  if (!ospf_spf_check_link_back (w_lsa, v->lsa))
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
        zlog_info (ZG, "SPF[%r]:     LSA[Type%d:%s] doesn't have "
                   "a link back to [%r]", &area->area_id,
                   w_lsa->data->type, LSA_NAME (w_lsa), &v->lsa->data->id);
      return 0;
    }

  /* (c) If vertex W is already on the shortest-path tree, examine
     the next link in the LSA. */
  if ((TV_CMP(area->tv_spf, w_lsa->tv_spf) == 0) && (w_lsa->status == inSPFtree))
    return 0;

  return 1;
}

/* RFC2328 Section 16.1 (2) for root vertex. */
void
ospf_spf_next_root (struct ospf_area *area, struct ospf_vertex *v)
{
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *w_lsa;
  struct ospf_link_map *link;
  int count = 0;
  int i;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zlog_info (ZG, "SPF[%r]: Vertex[%r] Router(root)",
               &area->area_id, &v->lsa->data->id);

  for (i = 0; i < vector_max (area->link_vec); i++)
    if ((link = vector_slot (area->link_vec, i)))
      {
        if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
          zlog_info (ZG, "SPF[%r]:   Link #%d (%r): %s",
                     &area->area_id, count++, &link->id,
                     LOOKUP (ospf_link_type_msg, link->type));

        /* (a) If this is a link to a stub network, examine the next
           link in V's LSA.  Links to stub networks will be
           considered in the second stage of the shortest path
           calculation. */
        if (link->type == LSA_LINK_TYPE_STUB)
          continue;

        /* (b) Otherwise, W is a transit vertex (router or transit
           network).  Look up the vertex W's LSA (router-LSA or
           network-LSA) in Area A's link state database. */
        else if (link->type == LSA_LINK_TYPE_POINTOPOINT
                 || link->type == LSA_LINK_TYPE_VIRTUALLINK)
          {
            w_lsa = ospf_lsdb_lookup_by_id (area->lsdb, OSPF_ROUTER_LSA,
                                            link->id, link->id);
            /* XXX unnumbered. */
            if (ospf_spf_next_link_check (area, v, w_lsa))
              ospf_spf_vertex_install (area, link->nh.nbr->oi,
                                       &link->nh.nbr->ident.address->u.prefix4,
                                       w_lsa, v, link->metric);
          }
        else if (link->type == LSA_LINK_TYPE_TRANSIT)
          {
            w_lsa = ospf_network_lsa_lookup_by_addr (area, link->id);

            if (ospf_spf_next_link_check (area, v, w_lsa))
              ospf_spf_vertex_install (area, link->nh.oi, NULL,
                                       w_lsa, v, link->metric);
          }
      }
}

/* RFC2328 Section 16.1 (2) for router. */
void
ospf_spf_next_router (struct ospf_area *area, struct ospf_vertex *v)
{
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *w_lsa;
  struct ospf_link_desc link;
  int count = 0;
  u_char *p;
  u_char *lim;

  p = ((u_char *) v->lsa->data) + OSPF_LSA_HEADER_SIZE + 4;
  lim =  ((u_char *) v->lsa->data) + pal_ntoh16 (v->lsa->data->length);

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zlog_info (ZG, "SPF[%r]: Vertex[%r] Router",
               &area->area_id, &v->lsa->data->id);

  while (p + OSPF_LINK_DESC_SIZE <= lim)
    {
      OSPF_LINK_DESC_GET (p, link);

      p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
        zlog_info (ZG, "SPF[%s]:   Link #%d (%r): %s", &area->area_id,
                   count++, link.id, LOOKUP (ospf_link_type_msg, link.type));

      /* (a) If this is a link to a stub network, examine the next
         link in V's LSA.  Links to stub networks will be
         considered in the second stage of the shortest path
         calculation. */
      if (link.type == LSA_LINK_TYPE_STUB)
        continue;

      /* (b) Otherwise, W is a transit vertex (router or transit
         network).  Look up the vertex W's LSA (router-LSA or
         network-LSA) in Area A's link state database. */
      else if (link.type == LSA_LINK_TYPE_POINTOPOINT
               || link.type == LSA_LINK_TYPE_VIRTUALLINK)
        w_lsa = ospf_lsdb_lookup_by_id (area->lsdb, OSPF_ROUTER_LSA,
                                        *link.id, *link.id);
      else if (link.type == LSA_LINK_TYPE_TRANSIT)
        w_lsa = ospf_network_lsa_lookup_by_addr (area, *link.id);
      else
        continue;

      if (ospf_spf_next_link_check (area, v, w_lsa))
        ospf_spf_vertex_install (area, NULL, NULL, w_lsa, v, link.metric);
    }
}

void
ospf_spf_vertex_install_transit (struct ospf_area *area,
                                 struct ospf_lsa *w_lsa,
                                 struct ospf_vertex *v)
{
  struct ospf_link_desc link;
  char *p;
  char *lim;

  p = ((u_char *)w_lsa->data) + OSPF_LSA_HEADER_SIZE + 4;
  lim = ((u_char *) w_lsa->data) + pal_ntoh16 (w_lsa->data->length);

  while (p + OSPF_LINK_DESC_SIZE <= lim)
    {
      OSPF_LINK_DESC_GET (p, link);

      if (link.type == LSA_LINK_TYPE_TRANSIT)
        if (IPV4_ADDR_SAME (link.id, &v->lsa->data->id))
          ospf_spf_vertex_install (area, NULL, link.data, w_lsa, v, 0);

      p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;
    }
}

/* RFC2328 Section 16.1 (2) for network. */
void
ospf_spf_next_transit (struct ospf_area *area, struct ospf_vertex *v)
{
  struct ospf_master *om = area->top->om;
  struct ospf_lsa *w_lsa = NULL;
  struct ospf_interface *oi;
  struct pal_in4_addr *nbr_id;
  int count = 0;
  u_char *p;
  u_char *lim;

  p = ((u_char *) v->lsa->data) + OSPF_LSA_HEADER_SIZE + 4;
  lim =  ((u_char *) v->lsa->data) + pal_ntoh16 (v->lsa->data->length);
    
  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zlog_info (ZG, "SPF[%r]: Vertex[%r] Network",
               &area->area_id, &v->lsa->data->id);

  /* In case of V is Network-LSA. */
  while (p < lim)
    {
      struct pal_in4_addr *mask;
      struct prefix q;

      mask = OSPF_PNT_IN_ADDR_GET (v->lsa->data,
                                   OSPF_NETWORK_LSA_NETMASK_OFFSET);

      q.family = AF_INET;
      q.u.prefix4 = v->lsa->data->id;
      q.prefixlen = ip_masklen (*mask);
      apply_mask (&q);

      nbr_id = (struct pal_in4_addr *)p;
      p += sizeof (struct pal_in4_addr);

      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
        zlog_info (ZG, "SPF[%r]:   Link #%d (%r): Router",
                   &area->area_id, count++, nbr_id);

      /* Lookup the vertex W's LSA. */
      w_lsa = ospf_lsdb_lookup_by_id (area->lsdb, OSPF_ROUTER_LSA,
                                      *nbr_id, *nbr_id);

      /* Check and install vertex W. */
      if (ospf_spf_next_link_check (area, v, w_lsa))
        {
          if (CHECK_FLAG (v->flags, OSPF_VERTEX_NETWORK_CONNECTED))
            {
              oi = ospf_area_if_lookup_by_prefix (area, &q);
              if (oi != NULL)
                if (oi->type == OSPF_IFTYPE_BROADCAST
                    || oi->type == OSPF_IFTYPE_NBMA)
                  ospf_spf_vertex_install_transit (area, w_lsa, v);
            }
          else
            ospf_spf_vertex_install (area, NULL, NULL, w_lsa, v, 0);
        }
    }
}

/* Add vertex V to table. */
void
ospf_spf_vertex_add (struct ospf_area *area, struct ospf_vertex *v)
{
  struct ls_prefix p;
  struct ls_node *rn;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, v->lsa->data->id);

  rn = ls_node_get (area->vertices, &p);
  if ((RN_INFO (rn, RNI_VERTEX_INDEX (v->lsa->data->type))) == NULL)
    ls_node_info_set (rn, RNI_VERTEX_INDEX (v->lsa->data->type), v);

  ls_unlock_node (rn);
}

void
ospf_spf_vertex_delete_all (struct ospf_area *area)
{
  struct ospf_vertex *v;
  struct ls_node *rn;

  for (rn = ls_table_top (area->vertices); rn; rn = ls_route_next (rn))
    {
      if ((v = RN_INFO (rn, RNI_VERTEX_INDEX (OSPF_VERTEX_ROUTER))))
        {
          ospf_vertex_free (v);
          RN_INFO_UNSET (rn, RNI_VERTEX_INDEX (OSPF_VERTEX_ROUTER));
        }

      if ((v = RN_INFO (rn, RNI_VERTEX_INDEX (OSPF_VERTEX_NETWORK))))
        {
          ospf_vertex_free (v);
          RN_INFO_UNSET (rn, RNI_VERTEX_INDEX (OSPF_VERTEX_NETWORK));
        }
    }

  /* Clear the pointer of the vertex for myself.  */
  area->spf = NULL;
}

/* Second stage of SPF calculation. */
void
ospf_spf_calculate_stubs (struct ospf_area *area, struct ospf_vertex *v)
{
  struct ospf_master *om = area->top->om;
  struct ospf_vertex *child;
  struct ospf_link_desc link;
  u_char *lim;
  u_char *p;
  int i;

  if (IS_LSA_MAXAGE (v->lsa)
      || CHECK_FLAG (v->lsa->flags, OSPF_LSA_DISCARD))
    return;

  if (v != area->spf && v->lsa->data->type == OSPF_VERTEX_ROUTER)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
        zlog_info (ZG, "SPF[%r]: Calculating stub network for (%r)",
                   &area->area_id, &v->lsa->data->id);

      p = ((u_char *) v->lsa->data) + 24;
      lim = ((u_char *) v->lsa->data) + pal_ntoh16 (v->lsa->data->length);

      while (p + OSPF_LINK_DESC_SIZE <= lim)
        {
          OSPF_LINK_DESC_GET (p, link);

          p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

          if (link.type == LSA_LINK_TYPE_STUB)
            ospf_route_add_stub (area, v, *link.id,
                                 ip_masklen (*link.data), link.metric);
        }
    }

  for (i = 0; i < vector_max (v->child); i++)
    if ((child = vector_slot (v->child, i)))
      if (!CHECK_FLAG (child->flags, OSPF_VERTEX_PROCESSED))
        {
          ospf_spf_calculate_stubs (area, child);
          SET_FLAG (child->flags, OSPF_VERTEX_PROCESSED);
        }
}

/* Calculating the shortest-path tree for an area. */
int
ospf_spf_calculate (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;
  struct ospf_vertex *v;
  int count = 0;

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zlog_info (ZG, "SPF[%r]: SPF calculation (1st STAGE)", &area->area_id);

  /* Check router-lsa-self.  If self-router-lsa is not yet allocated,
     return this area's calculation. */
  if ((ospf_router_lsa_self (area)) == NULL)
    {
      if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
        zlog_info (ZG, "SPF[%r]: stop calculation due to empty router-LSA",
                   &area->area_id);
      return -1;
    }

  /* RFC2328 16.1. */
  if (area->candidate->count == 0)
    {
      /* (1) Initialize the algorithm's data structures. */ 
      ospf_spf_vertex_delete_all (area);

      /* Initialize the shortest-path tree to only the root (which is the
         router doing the calculation). */
      v = ospf_spf_init (area);
      v->lsa->status = inSPFtree;
      v->lsa->tv_spf = area->tv_spf;
      ospf_spf_vertex_add (area, v);

      /* (2). */
      ospf_spf_next_root (area, v);
    }

  /* (3).
     If at this step the candidate list is empty, the shortest-
     path tree (of transit vertices) has been completely built and
     this stage of the procedure terminates. */
  while (area->candidate->count)
    {
      if (count++ > OSPF_SPF_VERTEX_COUNT_THRESHOLD)
        {
          if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
            zlog_info (ZG, "SPF[%r]: SPF calculation suspended",
                       &area->area_id);
          return 0;
        }
      /* Otherwise, choose the vertex belonging to the candidate list
         that is closest to the root, and add it to the shortest-path
         tree (removing it from the candidate list in the
         process). */ 
      v = area->candidate->array[0];  
      if (!v)
        continue;

      /* Remove vertex from candidate. */
      bheap_delete_head (area->candidate);
      v->lsa->status = inSPFtree;

      /* LSA whose LS age is equal to MaxAge should not be used
         in the routing table calculation.  */
      if (IS_LSA_MAXAGE (v->lsa)
          || CHECK_FLAG (v->lsa->flags, OSPF_LSA_DISCARD))
        ospf_vertex_free (v);
      else
        {
          /* Add the vertex to the shortest-path tree.  */
          ospf_vertex_add_parent (v);
          ospf_spf_vertex_add (area, v);

          /* (4). */
          ospf_route_add_spf (area, v);

          /* (5) Iterate the algorithm by returning to Step 2. */

          /* (2). */
          if (v->lsa->data->type == OSPF_ROUTER_LSA)
            ospf_spf_next_router (area, v);
          else
            ospf_spf_next_transit (area, v);
        }
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zlog_info (ZG, "SPF[%r]: SPF calculation (2nd STAGE)", &area->area_id);

  /* Second stage of SPF calculation procedure's  */
  ospf_spf_calculate_stubs (area, area->spf);

  /* Increment SPF Calculation Counter. */
  area->spf_calc_count++;
  pal_time_tzcurrent (&area->tv_spf, NULL);

  return 1;
}


int
_ospf_spf_calculate_timer (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct ospf *top = area->top;
  struct ospf_area *transit;
  struct ls_node *rn;
  int ret;
  struct ospf_master *om = top->om;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Calculate SPF for area. */
  ret = ospf_spf_calculate (area);
  if (ret == 0)
    OSPF_EVENT_SUSPEND_ON (area->t_spf_calc,
                           ospf_spf_calculate_timer, area, 0);
  else if (ret == 1)
    {
      /* Reset the suspend thread pointer.  */
      if (area->t_suspend == thread)
        area->t_suspend = NULL;

      area->incr_defer = PAL_FALSE;
      area->incr_count = 0;

      /* Cleanup the pending threads.  */
      OSPF_EVENT_PENDING_CLEAR (area->event_pend);

      /* Update virtual-link status. */
      ospf_vlink_status_update (area);

      /* Cleanup the transit area's nexthops.  */
      if (!IS_AREA_TRANSIT (area))
        ospf_nexthop_transit_down (area);

      /* Install intra-area routes.  */
      ospf_route_install_all_event_add (area, OSPF_PATH_INTRA_AREA);

      /* Install ASBR intra-area routes.  */
      ospf_route_asbr_install_all_event_add (area, OSPF_PATH_INTRA_AREA);

      /* Schedule IA calculation event.  */
      ospf_ia_calc_event_add (area);

      /* Shedule IA calculation event for the transit area.  */
      if (IS_AREA_BACKBONE (area))
        for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
          if ((transit = RN_INFO (rn, RNI_DEFAULT)))
            if (IS_AREA_TRANSIT (transit))
              ospf_ia_calc_event_add (transit);

#ifdef HAVE_NSSA
      top->incr_n_defer = PAL_FALSE;
      top->tot_n_incr = 0;
      /* NSSA Translator Election. */
      if (IS_OSPF_ABR (top))
        if (IS_AREA_NSSA (area))
          ospf_abr_nssa_translator_state_update (area);
#endif /* HAVE_NSSA */

      /* IA task runs first time.  */
      if (area->spf_calc_count == 1)
        ospf_abr_ia_update_all_to_area (area);

      /* or when the area transit capability has changed.  */
      else if (CHECK_FLAG (area->oflags, OSPF_AREA_TRANSIT)
               != CHECK_FLAG (area->flags, OSPF_AREA_TRANSIT))
        ospf_abr_ia_update_from_backbone_to_area (area);

      top->incr_defer = PAL_FALSE;
      top->tot_incr = 0;

      /* Schedule external route calculation event.  */
      ospf_ase_calc_event_add (top);

      /* Cleanup invalid routes.  */
      ospf_route_clean_invalid_event_add (top);

      /* If an LSA got installed while this calculation was going on,
         it was not considered in this calculation. Hence, schedule another
         calculation to make sure that the LSA is considered now */
      if (area->spf_re_run)
        {
          area->spf_re_run = 0;
          ospf_spf_calculate_timer_add (area);
        }

    }

  return 0;
}

int
ospf_spf_calculate_timer (struct thread *thread)
{
  struct ospf_area *area = THREAD_ARG (thread);
  struct thread *thread_self = area->t_spf_calc ? area->t_spf_calc : thread; 
  struct ospf_master *om = area->top->om;
  struct ospf_area *backbone;
  char buf[34];
  buf[0] = '\0';

  area->t_spf_calc = NULL;

  if (!IS_AREA_ACTIVE (area))
    return 0;

  /* Set VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (!IS_AREA_BACKBONE (area))
    if ((backbone = area->top->backbone))
      if (IS_AREA_ACTIVE (backbone))
        if (backbone->t_spf_calc != NULL)
          {
            OSPF_AREA_TIMER_ON (area->t_spf_calc, ospf_spf_calculate_timer, 0);
            return 0;
          }

  /* Wait for the other suspended thread.  */
  if (IS_AREA_SUSPENDED (area, thread_self))
    {
      OSPF_EVENT_PENDING_ON (area->event_pend,
                             ospf_spf_calculate_timer, area, 0);
      return 0;
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zsnprintf (buf, 34, "SPF[%r]: Calculation", &area->area_id);

  return ospf_thread_wrapper (thread_self, _ospf_spf_calculate_timer,
                              buf, IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF));
}

/* Schedule SPF calculation. */
void
ospf_spf_calculate_timer_add (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;
  struct ospf *top = area->top;
  struct pal_timeval now, delta, delay, hold;

  if (!IS_PROC_ACTIVE (top)
      || !IS_AREA_ACTIVE (area))
    return;

#ifdef HAVE_RESTART
  /*  The restarting router does *not* install OSPF routes into 
      the system's forwarding table(s) and relies on the forwarding 
      entries that it installed prior to the restart. */
  if (IS_GRACEFUL_RESTART (top) || IS_RESTART_SIGNALING (top))
    return;
#endif /* HAVE_RESTART */

  /* SPF calculation timer is already scheduled.
     If SPF calculation is in suspended state, we should re-run the SPF
     calculation. Hence, set spf_re_run to 1. */
  if (area->t_spf_calc != NULL)
    {
      if (IS_AREA_THREAD_SUSPENDED (area, area->t_spf_calc))
        area->spf_re_run = 1;
      return;
    }

  pal_time_tzcurrent (&now, NULL);
  delta = TV_SUB (now, area->tv_spf);

  if ((TV_CMP (delta, area->tv_spf_curr)) < 0)
    {
      delay = TV_SUB (area->tv_spf_curr, delta);

      /* Change the value of tv_spf_curr to 5 times its value */
      area->tv_spf_curr.tv_sec *= OSPF_SPF_INCREMENT_VALUE;
      area->tv_spf_curr.tv_sec += ((OSPF_SPF_INCREMENT_VALUE *
                             area->tv_spf_curr.tv_usec) / ONE_SEC_MICROSECOND);
      area->tv_spf_curr.tv_usec = (OSPF_SPF_INCREMENT_VALUE * 
                             area->tv_spf_curr.tv_usec) % ONE_SEC_MICROSECOND;
    }
  else
    {
      delay.tv_sec = delay.tv_usec = 0;
      hold = area->tv_spf_curr;
      hold.tv_sec *= OSPF_SPF_INCREMENT_VALUE;
      hold.tv_sec += ((OSPF_SPF_INCREMENT_VALUE * hold.tv_usec) / ONE_SEC_MICROSECOND);
      hold.tv_usec = (OSPF_SPF_INCREMENT_VALUE * hold.tv_usec) % ONE_SEC_MICROSECOND;

      if (TV_CMP (top->spf_max_delay, hold) < 0)
        hold = top->spf_max_delay;

      if ((TV_CMP (hold, delta)) < 0)
        {
          area->tv_spf_curr = top->spf_min_delay;
        }
    }

  /* If delay < min delay , delay for another min_delay period */
  if ((TV_CMP (top->spf_min_delay, delay)) > 0)  
    {
      delay = top->spf_min_delay;
    }

  if ((TV_CMP (top->spf_max_delay, area->tv_spf_curr)) < 0)
    {
      /* Should not be more than max value */
      area->tv_spf_curr = top->spf_max_delay;
    }

  if (IS_DEBUG_OSPF (rt_calc, RT_CALC_SPF))
    zlog_info (ZG, "SPF[%r]: Calculation timer scheduled (delay %d.%06d secs)",
               &area->area_id, delay.tv_sec, delay.tv_usec);

  /* Now schedule SPF timer.  */
  OSPF_TV_TIMER_ON (area->t_spf_calc, ospf_spf_calculate_timer, area, delay);
}

void
ospf_spf_calculate_timer_add_all (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      ospf_spf_calculate_timer_add (area);
}

void
ospf_spf_calculate_timer_reset_all (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (area->t_spf_calc != NULL)
        if (!IS_AREA_THREAD_SUSPENDED (area, area->t_spf_calc))
          {
            /* Reset the SPF timer.  */
            OSPF_TIMER_OFF (area->t_spf_calc);
            ospf_spf_calculate_timer_add (area);
          }
}
