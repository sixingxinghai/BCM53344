/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "table.h"
#include "linklist.h"
#include "filter.h"
#include "log.h"
#ifdef HAVE_OSPF_CSPF
#include "cspf_common.h"
#include "cspf_server.h"
#endif /* HAVE_OSPF_CSPF */

#include "ospfd.h"
#include "ospf_interface.h"
#include "ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospf_packet.h"
#include "ospf_neighbor.h"
#include "ospf_lsa.h"
#include "ospf_lsdb.h"
#include "ospf_flood.h"
#include "ospf_abr.h"
#include "ospf_asbr.h"
#include "ospf_spf.h"
#include "ospf_ia.h"
#include "ospf_ase.h"
#include "ospf_route.h"
#include "ospf_nsm.h"
#include "ospf_api.h"
#include "ospf_debug.h"
#include "ospf_vrf.h"
#ifdef HAVE_RESTART
#include "ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
#include "ospf_te.h"
#endif /*HAVE_OSPF_TE */
#ifdef HAVE_OSPF_CSPF
#include "ospf_cspf.h"
#endif /* HAVE_OSPF_CSPF */
#ifdef HAVE_BFD
#include "ospf_bfd_api.h"
#endif /* HAVE_BFD */

struct ls_table_index ospf_api_table_def[] =
{
  /* 0 -- dummy. */
  {  0,  0, { LS_INDEX_NONE } },
  /* 1 -- dummy ospfGeneralGroup. */
  {  0,  0, { LS_INDEX_NONE } },
  /* 2 -- ospfAreaTable. */
  {  4,  4, { LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 3 -- ospfStubAreaTable. */
  {  5,  5, { LS_INDEX_INADDR, LS_INDEX_INT8, LS_INDEX_NONE } },
  /* 4 -- ospfLsdbTable. */
  { 13, 13, { LS_INDEX_INADDR, LS_INDEX_INT8, LS_INDEX_INADDR,
              LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 5 -- ospfAreaRangeTable. */
  {  8,  8, { LS_INDEX_INADDR, LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 6 -- ospfHostTable. */
  {  5,  5, { LS_INDEX_INADDR, LS_INDEX_INT8, LS_INDEX_NONE } },
  /* 7 -- ospfIfTable. */
  {  5,  8, { LS_INDEX_INADDR, LS_INDEX_INT32, LS_INDEX_NONE } },
  /* 8 -- ospfIfMetricTable. */
  {  6,  9, { LS_INDEX_INADDR, LS_INDEX_INT32, LS_INDEX_INT8,
              LS_INDEX_NONE } },
  /* 9 -- ospfVirtIfTable. */
  {  8,  8, { LS_INDEX_INADDR, LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 10 -- ospfNbrTable. */
  {  5,  8, { LS_INDEX_INADDR, LS_INDEX_INT32, LS_INDEX_NONE } },
  /* 11 -- ospfVirtNbrTable. */
  {  8,  8, { LS_INDEX_INADDR, LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 12 -- ospfExtLsdbTable. */
  {  9,  9, { LS_INDEX_INT8, LS_INDEX_INADDR, LS_INDEX_INADDR,
              LS_INDEX_NONE } },
  /* 13 -- dummy ospfRouteGroup. */
  {  0,  0, { LS_INDEX_NONE } },
  /* 14 -- ospfAreaAggregateTable. */
  { 13, 13, { LS_INDEX_INADDR, LS_INDEX_INT8, LS_INDEX_INADDR,
              LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 15 -- dummy. */
  {  0,  0, { LS_INDEX_NONE } },
  /* 16 -- ospfStaticNbrTable. */
  {  4,  4, { LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 17 -- ospfLsdbTable - upper. */
  {  5,  5, { LS_INDEX_INADDR, LS_INDEX_INT8, LS_INDEX_NONE } },
  /* 18 -- ospfLsdbTable - lower. */
  {  8,  8, { LS_INDEX_INADDR, LS_INDEX_INADDR, LS_INDEX_NONE } },
  /* 19 -- ospfAreaAggregateTable - lower. */
  {  8,  8, { LS_INDEX_INADDR, LS_INDEX_INADDR, LS_INDEX_NONE } },
};


/* internal utility functions. */
int
ospf_api_iftype_generic (struct ospf_interface *oi)
{
  switch (oi->type)
    {
    case OSPF_IFTYPE_POINTOPOINT:
      return OSPF_API_IFTYPE_POINTOPOINT;
      break;
    case OSPF_IFTYPE_BROADCAST:
      return OSPF_API_IFTYPE_BROADCAST;
      break;
    case OSPF_IFTYPE_NBMA:
      return OSPF_API_IFTYPE_NBMA;
      break;
    case OSPF_IFTYPE_POINTOMULTIPOINT:
    case OSPF_IFTYPE_POINTOMULTIPOINT_NBMA:
      return OSPF_API_IFTYPE_POINTOMULTIPOINT;
      break;
    default:
      return OSPF_API_IFTYPE_NONE;
      break;
    }
  return OSPF_API_IFTYPE_NONE;
}

int
ospf_api_ifsm_state (struct ospf_interface *oi)
{
  switch (oi->state)
    {
    case IFSM_Down:
      return OSPF_API_IFSM_STATE_DOWN;
      break;
    case IFSM_Loopback:
      return OSPF_API_IFSM_STATE_LOOPBACK;
      break;
    case IFSM_Waiting:
      return OSPF_API_IFSM_STATE_WAITING;
      break;
    case IFSM_PointToPoint:
      return OSPF_API_IFSM_STATE_POINTTOPOINT;
      break;
    case IFSM_DROther:
      return OSPF_API_IFSM_STATE_OTHERDESIGNATEDROUTER;
      break;
    case IFSM_Backup:
      return OSPF_API_IFSM_STATE_BACKUPDESIGNATEDROUTER;
      break;
    case IFSM_DR:
      return OSPF_API_IFSM_STATE_DESIGNATEDROUTER;
      break;
    }
  return 0;
}

static int
ospf_if_connected_count (struct interface *ifp)
{
  struct connected *ifc;
  int count = 0;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    count++;

  return count;
}


/* This function is a little bit hacky for handling ifindex as index. */
unsigned int
indexlen_convert (int indexlen)
{
  if (indexlen >= 5)
    return indexlen + 3;

  return indexlen;
}

/* This function lock node if RNI_DEFAULT exists. */
struct ls_node *
ospf_api_lookup (struct ls_table *rt, int type, ...)
{
  struct ls_prefix16 p;
  va_list args;

  va_start (args, type);

  p.prefixsize = ospf_api_table_def[type].octets;
  p.prefixlen = ospf_api_table_def[type].octets * 8;

  ls_prefix_set_va_list ((struct ls_prefix *)&p,
                       ospf_api_table_def[type].vars, args);

  va_end (args);

  return ls_node_lookup (rt, (struct ls_prefix *)&p);
}

/* This function lock node if RNI_DEFAULT exists. */
struct ls_node *
ospf_api_lookup_next (struct ls_table *rt, int type, int indexlen, ...)
{
  struct ls_table_index *table_def = &ospf_api_table_def[type];
  struct ls_node *rn = NULL;
  struct ls_prefix16 p;
  va_list args;

  va_start (args, indexlen);

  /* Lookup first. */
  if (indexlen == 0)
    rn = ls_node_lookup_first (rt);
  /* Lookup next. */
  else
    {
      pal_mem_set (&p, 0, sizeof (struct ls_prefix16));
      p.prefixsize = table_def->octets;
      p.prefixlen = indexlen * 8;

      ls_prefix_set_va_list ((struct ls_prefix *)&p, table_def->vars, args);

      if (indexlen == table_def->octets)
        {
          rn = ls_node_lookup (rt, (struct ls_prefix *)&p);
          if (rn)
            rn = ls_route_next (rn);
        }
      else
        rn = ls_node_get (rt, (struct ls_prefix *)&p);

      for (; rn; rn = ls_route_next (rn))
        if (RN_INFO (rn, RNI_DEFAULT))
          break;
    }

  va_end (args);
  if (rn)
    {
      if (RN_INFO (rn, RNI_DEFAULT))
        return rn;

      ls_unlock_node (rn);
    }

  return NULL;
}


/* ospfAreaEntry related functions. */
struct ospf_area *
ospf_area_entry_lookup_next (struct ospf *top,
                             struct pal_in4_addr *area_id, int indexlen)
{
  struct ospf_area *area;
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->area_table, OSPF_AREA_TABLE, indexlen,
                             area_id);
  if (rn != NULL)
    {
      area = RN_INFO (rn, RNI_DEFAULT);
      if (area != NULL)
        *area_id = area->area_id;
      ls_unlock_node (rn);
      return area;
    }
  return NULL;
}

/* ospfSutbAreaEntry related functions. */
struct ospf_area *
ospf_stub_area_entry_lookup_next (struct ospf *top,
                                  struct pal_in4_addr *area_id,
                                  int *tos, int indexlen)
{
  struct ospf_area *area;
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->stub_table, OSPF_STUB_AREA_TABLE,
                             indexlen, area_id, tos);
  if (rn != NULL)
    {
      area = RN_INFO (rn, RNI_DEFAULT);
      if (area != NULL)
        {
          *area_id = area->area_id;
          *tos = 0;
        }
      ls_unlock_node (rn);
      return area;
    }
  return NULL;
}

/* ospfLsdbEntry related functions. */
struct ospf_lsa *
ospf_lsdb_lower_entry_lookup (struct ls_table *table,
                              struct pal_in4_addr ls_id,
                              struct pal_in4_addr adv_router)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (table, OSPF_LSDB_LOWER_TABLE, &ls_id, &adv_router);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_lsa *
ospf_lsdb_entry_lookup (struct ospf *top,
                        struct pal_in4_addr area_id, int type,
                        struct pal_in4_addr ls_id,
                        struct pal_in4_addr adv_router)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->lsdb_table,
                        OSPF_LSDB_UPPER_TABLE, &area_id, &type);
  if (rn)
    {
      ls_unlock_node (rn);
      return ospf_lsdb_lower_entry_lookup (RN_INFO (rn, RNI_DEFAULT),
                                           ls_id, adv_router);
    }
  return NULL;
}

struct ospf_lsa *
ospf_lsdb_entry_lookup_next (struct ospf *top,
                             struct pal_in4_addr *area_id, int *type,
                             struct pal_in4_addr *ls_id,
                             struct pal_in4_addr *adv_router, int indexlen)
{
  struct ls_node *rn_up, *rn_low;

  if (indexlen >= OSPF_LSDB_UPPER_TABLE_DEPTH)
    {
      rn_up = ospf_api_lookup (top->lsdb_table, OSPF_LSDB_UPPER_TABLE,
                               area_id, type);
      indexlen -= OSPF_LSDB_UPPER_TABLE_DEPTH;
    }
  else
    {
      rn_up = ospf_api_lookup_next (top->lsdb_table, OSPF_LSDB_UPPER_TABLE,
                                    indexlen, area_id, type);
      indexlen = 0;
    }

  if (rn_up)
    {
      struct ospf_lsa *lsa = NULL;

      ls_unlock_node (rn_up);

      rn_low = ospf_api_lookup_next (RN_INFO (rn_up, RNI_DEFAULT),
                                     OSPF_LSDB_LOWER_TABLE,
                                     indexlen, ls_id, adv_router);
      if (rn_low)
        lsa = RN_INFO (rn_low, RNI_DEFAULT);
      else
        {
          struct ls_table *rt;

          ls_lock_node (rn_up);
          rn_up = ls_route_next (rn_up);
          for (; rn_up; rn_up = ls_route_next (rn_up))
            if ((rt = RN_INFO (rn_up, RNI_DEFAULT)))
              if (rt->top)
                break;

          if (rn_up)
            {
              ls_unlock_node (rn_up);
              rn_low = ospf_api_lookup_next (RN_INFO (rn_up, RNI_DEFAULT),
                                             OSPF_LSDB_LOWER_TABLE, 0,
                                             &IPv4AddressUnspec,
                                             &IPv4AddressUnspec);
              if (rn_low)
                lsa = RN_INFO (rn_low, RNI_DEFAULT);
            }
        }

      if (lsa)
        {
          if (LSA_FLOOD_SCOPE (lsa->data->type) == LSA_FLOOD_SCOPE_AREA)
            *area_id = lsa->lsdb->u.area->area_id;
          *type = lsa->data->type;
          *ls_id = lsa->data->id;
          *adv_router = lsa->data->adv_router;
          ls_unlock_node (rn_low);
          return lsa;
        }
    }
  return NULL;
}

/* ospfAreaRange related functions. */
struct ospf_area_range *
ospf_area_range_entry_lookup (struct ospf *top,
                              struct pal_in4_addr area_id,
                              struct pal_in4_addr net)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->area_range_table,
                        OSPF_AREA_RANGE_TABLE, &area_id, &net);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_area_range *
ospf_area_range_entry_lookup_next (struct ospf *top,
                                   struct pal_in4_addr *area_id,
                                   struct pal_in4_addr *net, int indexlen)
{
  struct ospf_area_range *range;
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->area_range_table,
                             OSPF_AREA_RANGE_TABLE, indexlen, area_id, net);
  if (rn != NULL)
    {
      range = RN_INFO (rn, RNI_DEFAULT);
      if (range != NULL)
        {
          pal_mem_cpy (area_id, rn->p->prefix, sizeof (struct pal_in4_addr));
          pal_mem_cpy (net, range->lp->prefix, sizeof (struct pal_in4_addr));
        }
      ls_unlock_node (rn);
      return range;
    }
  return NULL;
}

/* ospfHostEntry related functions. */
/* Lookup ospf_host_entry matched next, and return struct and index. */
struct ospf_host_entry *
ospf_host_entry_lookup_next (struct ospf *top,
                             struct pal_in4_addr *addr,
                             int *tos, int indexlen)
{
  struct ospf_host_entry *host;
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->host_table,
                             OSPF_HOST_TABLE, indexlen, addr, tos);
  if (rn != NULL)
    {
      host = RN_INFO (rn, RNI_DEFAULT);
      if (host != NULL)
        {
          *addr = host->addr;
          *tos = host->tos;
        }
      ls_unlock_node (rn);
      return host;
    }

  return NULL;
}

/* ospfIfEntry related functions. */
struct ospf_interface *
ospf_if_entry_lookup_next (struct ospf *top,
                           struct pal_in4_addr *addr,
                           int *ifindex, int indexlen)
{
  struct ospf_interface *oi;
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->if_table, OSPF_IF_TABLE,
                             indexlen_convert (indexlen), addr, ifindex);
  if (rn != NULL)
    {
      oi = RN_INFO (rn, RNI_OSPF_IF);
      if (oi != NULL)
        {
          *ifindex = ospf_address_less_ifindex (oi);
          if (*ifindex)
            *addr = IPv4AddressUnspec;
          else
            *addr = oi->ident.address->u.prefix4;
        }
      ls_unlock_node (rn);
      return oi;
    }
  return NULL;
}

/* ospfVirtIfEntry related functions. */
struct ospf_vlink *
ospf_vlink_entry_lookup_next (struct ospf *top, struct pal_in4_addr *area_id,
                              struct pal_in4_addr *nbr_id, int indexlen)
{
  struct ospf_vlink *vlink;
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->vlink_table, OSPF_VIRT_IF_TABLE,
                             indexlen, area_id, nbr_id);
  if (rn != NULL)
    {
      vlink = RN_INFO (rn, RNI_DEFAULT);
      if (vlink != NULL)
        {
          *area_id = vlink->area_id;
          *nbr_id = vlink->peer_id;
        }
      ls_unlock_node (rn);
      return vlink;
    }
  return NULL;
}

/* ospfNbrEntry related functions. */
struct ospf_neighbor *
ospf_nbr_entry_lookup (struct ospf *top,
                       struct pal_in4_addr nbr_addr, unsigned int ifindex)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->nbr_table, OSPF_NBR_TABLE, &nbr_addr, &ifindex);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_neighbor *
ospf_nbr_entry_lookup_next (struct ospf *top,
                            struct pal_in4_addr *nbr_addr,
                            unsigned int *ifindex, int indexlen)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->nbr_table, OSPF_NBR_TABLE,
                             indexlen_convert (indexlen), nbr_addr, ifindex);
  if (rn != NULL)
    {
      nbr = RN_INFO (rn, RNI_DEFAULT);
      if (nbr != NULL)
        {
          *nbr_addr = nbr->ident.address->u.prefix4;
          *ifindex = ospf_address_less_ifindex (nbr->oi);
        }
      ls_unlock_node (rn);
      return nbr;
    }
  return NULL;
}

struct ospf_neighbor *
ospf_virt_nbr_lookup_by_vlink (struct ospf_vlink *vlink)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  if (vlink)
    {
      oi = vlink->oi;
      if (oi->nbrs)
        for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
          if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
            if (nbr->state != NFSM_Down
                && nbr->ident.address->u.prefix4.s_addr != 0)
              {
                ls_unlock_node (rn);
                return nbr;
              }
    }
  return NULL;
}

struct ospf_lsa *
ospf_ext_lsdb_entry_lookup (struct ospf *top, int type,
                            struct pal_in4_addr ls_id,
                            struct pal_in4_addr adv_router)
{
  struct ospf_lsdb_slot *slot = EXTERNAL_LSDB (top);
  struct ls_node *rn;

  rn = ospf_api_lookup (slot->db, OSPF_LSDB_LOWER_TABLE, &ls_id, &adv_router);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_lsa *
ospf_ext_lsdb_entry_lookup_next (struct ospf *top, int *type,
                                 struct pal_in4_addr *ls_id,
                                 struct pal_in4_addr *adv_router, int indexlen)
{
  struct ospf_lsdb_slot *slot = EXTERNAL_LSDB (top);
  struct ospf_lsa *lsa;
  struct ls_node *rn;

  if (indexlen > 0)
    indexlen--;

  rn = ospf_api_lookup_next (slot->db, OSPF_LSDB_LOWER_TABLE,
                             indexlen, ls_id, adv_router);
  if (rn)
    {
      lsa = RN_INFO (rn, RNI_DEFAULT);
      if (lsa != NULL)
        {
          *type = lsa->data->type;
          *ls_id = lsa->data->id;
          *adv_router = lsa->data->adv_router;
        }
      ls_unlock_node (rn);
      return lsa;
    }
  return NULL;
}

/* ospfAreaAggregateEntry ralated functions. */
struct ospf_area_range *
ospf_area_aggregate_lower_entry_lookup (struct ospf_area *area,
                                        struct pal_in4_addr net,
                                        struct pal_in4_addr mask)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (area->aggregate_table, OSPF_AREA_AGGREGATE_LOWER_TABLE,
                        &net, &mask);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_area_range *
ospf_area_aggregate_entry_lookup (struct ospf *top,
                                  struct pal_in4_addr area_id,
                                  int type, struct pal_in4_addr net,
                                  struct pal_in4_addr mask)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->area_table, OSPF_AREA_TABLE, &area_id);
  if (rn == NULL)
    return NULL;

  ls_unlock_node (rn);
  if (type == OSPF_SUMMARY_LSA)
    return ospf_area_aggregate_lower_entry_lookup (RN_INFO (rn, RNI_DEFAULT),
                                                   net, mask);

  return NULL;
}

struct ospf_area_range *
ospf_area_aggregate_entry_lookup_next (struct ospf *top,
                                       struct pal_in4_addr *area_id, int *type,
                                       struct pal_in4_addr *net,
                                       struct pal_in4_addr *mask,
                                       int indexlen,
                                       u_int32_t vr_id)
{
  struct ls_node *rn_up, *rn_low;

  if (indexlen >= OSPF_AREA_AGGREGATE_UPPER_TABLE_DEPTH - 1)
    {
      /* Only type 3 address range is supported. */
      if (indexlen >= OSPF_AREA_AGGREGATE_UPPER_TABLE_DEPTH)
        if (*type != OSPF_SUMMARY_LSA)
          return NULL;

      rn_up = ospf_api_lookup (top->area_table,
                               OSPF_AREA_TABLE, area_id);

      indexlen -= OSPF_AREA_AGGREGATE_UPPER_TABLE_DEPTH;
    }
  else
    {
      rn_up = ospf_api_lookup_next (top->area_table, OSPF_AREA_TABLE,
                                    indexlen, area_id);
      indexlen = 0;
    }

  if (rn_up)
    {
      struct ospf_area *area = RN_INFO (rn_up, RNI_DEFAULT);
      struct ospf_area_range *range = NULL;

      ls_unlock_node (rn_up);

      rn_low = ospf_api_lookup_next (area->aggregate_table,
                                     OSPF_AREA_AGGREGATE_LOWER_TABLE,
                                     indexlen, net, mask);
      if (rn_low)
        range = RN_INFO (rn_low, RNI_DEFAULT);
      else
        {
          ls_lock_node (rn_up);
          rn_up = ls_route_next (rn_up);
          for (; rn_up; rn_up = ls_route_next (rn_up))
            if ((area = RN_INFO (rn_up, RNI_DEFAULT)))
              if (area->ranges->top)
                break;

          if (rn_up)
            {
              ls_unlock_node (rn_up);
              rn_low = ospf_api_lookup_next (area->aggregate_table,
                                             OSPF_AREA_AGGREGATE_LOWER_TABLE,
                                             0, &IPv4AddressUnspec,
                                             &IPv4AddressUnspec);
              if (rn_low)
                range = RN_INFO (rn_low, RNI_DEFAULT);
            }
        }
      if (range)
        {
          *area_id = area->area_id;
          *type = OSPF_SUMMARY_LSA;
          pal_mem_cpy (net, range->lp->prefix, sizeof (struct pal_in4_addr));
          masklen2ip (range->lp->prefixlen, mask);
          ls_unlock_node (rn_low);
          return range;
        }
    }
  return NULL;
}

struct ospf_nbr_static *
ospf_nbr_static_lookup_next (struct ospf *top,
                             struct pal_in4_addr *addr, int indexlen)
{
  struct ls_node *rn;

  rn = ospf_api_lookup_next (top->nbr_static,
                             OSPF_NBR_STATIC_TABLE, indexlen, addr);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

/* CLI-API functions. */
int
ospf_process_set (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  /* Create a process.  */
  ospf_proc_get (om, proc_id, NULL);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_process_set_vrf (u_int32_t vr_id, int proc_id, char *name)
{
  struct ospf_master *om;
  struct ospf_vrf *ov;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  ov = ospf_vrf_get (om, name);
  if (ov == NULL)
    return OSPF_API_SET_ERR_VRF_NOT_EXIST;
#ifdef HAVE_VRF_OSPF
  else 
    SET_FLAG (ov->flags, OSPF_VRF_ENABLED);
#endif /* HAVE_VRF_OSPF */

  top = ospf_proc_lookup (om, proc_id);
  if (top != NULL)
    if (top->ov != ov)
      return OSPF_API_SET_ERR_VRF_ALREADY_BOUND;

  /* Create a process.  */
  ospf_proc_get (om, proc_id, name);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_process_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;
#ifdef HAVE_GMPLS
  struct ospf_telink *os_tel = NULL;
  struct telink *tl = NULL;
  struct avl_node *tl_node = NULL;
#endif /* HAVE_GMPLS */

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

#ifdef HAVE_GMPLS
  AVL_TREE_LOOP (&om->zg->gmif->teltree, tl, tl_node)
    {
      if (tl->info == NULL)
       continue;
      else
        { 
          os_tel = (struct ospf_telink *)tl->info;

          if ((os_tel->top) && (os_tel->top->ospf_id == top->ospf_id)) 
            {
              os_tel->top = NULL;
              os_tel->area = NULL;
            }
          else
            continue;
        }
    }
#endif
  ospf_proc_destroy (top);

  return OSPF_API_SET_SUCCESS;
}

#ifdef HAVE_RESTART
int
ospf_process_unset_graceful (u_int32_t vr_id, int proc_id)
{
  struct ospf_interface *oi;
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */
#ifdef HAVE_GMPLS
  struct telink *tl = NULL;
  struct avl_node *tl_node = NULL;
  struct ospf_telink *os_tel = NULL;
#endif /* HAVE_GMPLS */

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Set the restart flag for the process.  */
  SET_FLAG (top->flags, OSPF_ROUTER_RESTART);

  if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
    {
      if (IS_OPAQUE_CAPABLE (top))
        {
          UNSET_FLAG (top->flags, OSPF_GRACE_LSA_ACK_RECD);

          /* Send Grace-LSA to virtual link. */
          for (rn = ls_table_top (top->vlink_table);rn;
               rn = ls_route_next (rn))
            if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
              {
                UNSET_FLAG (vlink->oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD);
                ospf_grace_lsa_originate (vlink->oi);
              }

          /* Send Grace-LSA. */
          for (rn = ls_table_top (top->if_table); rn;rn = ls_route_next (rn))
            if ((oi = RN_INFO (rn, RNI_DEFAULT)))
              {
                UNSET_FLAG (oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD);
                ospf_grace_lsa_originate (oi);
              }

#ifdef HAVE_OSPF_MULTI_AREA
          /* Send Grace-LSA to multi area links. */
          for (rn = ls_table_top (top->multi_area_link_table); rn;
               rn = ls_route_next (rn))
            if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
              {
                if (mlink->oi) 
                  {
                    UNSET_FLAG (mlink->oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD);
                    ospf_grace_lsa_originate (mlink->oi);
                  }       
              }

#endif /* HAVE_OSPF_MULTI_AREA */

#ifdef HAVE_GMPLS
          /* Send Grace-LSA to all the te-links */
          AVL_TREE_LOOP (&om->zg->gmif->teltree, tl, tl_node)
            {
              if (tl->info == NULL)
                continue;

              os_tel = (struct ospf_telink *)tl->info;

              /* announce the telink*/
              ospf_te_announce_te_link (os_tel);
            }
#endif /* HAVE_GMPLS */

          OSPF_TOP_TIMER_ON (top->t_grace_ack_event,
                             ospf_grace_ack_wait_timer,
                             OSPF_RESTART_GRACE_ACK_TIME);

          if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
            zlog_info (ZG, "ROUTER: Graceful Restart");
        }
      else
        {
          if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
            zlog_info (ZG, "ROUTER: Opaque-LSA is not capable, "
                       "Cannot perform Graceful Restart");
        }
    }
  else if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    {
      for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
        if ((oi = RN_INFO (rn, RNI_DEFAULT)))
          {
            if (!CHECK_FLAG (oi->flags, OSPF_IF_RS_BIT_SET))
              SET_FLAG (oi->flags, OSPF_IF_RS_BIT_SET);

            if (IS_OSPF_NETWORK_NBMA (oi))
              ospf_hello_send_nbma (oi, OSPF_HELLO_DEFAULT);
            else
              ospf_hello_send (oi, NULL, OSPF_HELLO_DEFAULT);
          }

      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "ROUTER: Restart Signaling");
    }

  if ((top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
      || (!IS_OPAQUE_CAPABLE (top)))
    {
      ospf_proc_destroy (top);
      mod_stop(APN_PROTO_OSPF, MOD_STOP_CAUSE_GRACE_RST);
      pal_exit (0);
    }

  return OSPF_API_SET_SUCCESS;
}
int
ospf_restart_graceful (struct lib_globals *zg, u_int16_t grace_period, int reason)
{
  struct apn_vr *vr = NULL;
  struct ospf_master *om = NULL;
  struct listnode *node, *next;
  u_int16_t seconds = grace_period;
  struct ospf *top;
  int ret = 0, i = 0;
  u_int32_t ospf_ins_found = PAL_FALSE;

  for (i = 0; i < vector_max(zg->vr_vec);i++)
    {
      if ((vr = vector_slot (zg->vr_vec, i)))
        {
          om = vr->proto;

          /* Set VR Context */
          LIB_GLOB_SET_VR_CONTEXT (ZG, vr);
          
          if (NULL != om && (0 != listcount(om->ospf)))
            {
              ospf_ins_found = PAL_TRUE;
              if (grace_period)
                seconds = grace_period;
              else if (CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_GRACE_PERIOD))
                seconds = om->grace_period;
              else
                seconds = OSPF_GRACE_PERIOD_DEFAULT;

              ret = ospf_graceful_restart_set (om, seconds, reason);
              if (ret != OSPF_API_SET_SUCCESS)
                continue;

              if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
                zlog_info (ZG, "ROUTER: Start Restarting, grace-period(%d), "
                           "restart-reason(%d)", om->grace_period,
                           om->restart_reason);

              for (node = LISTHEAD (om->ospf); node; node = next)
                {
                  top = GETDATA (node);
                  next = node->next;
                  ret = ospf_process_unset_graceful (vr->id, top->ospf_id);
                  if (ret != OSPF_API_SET_SUCCESS)
                    continue;
                }
             }
         }
    }

   /* Set VR context to default */
   vr = apn_vr_get_privileged (ZG);
   LIB_GLOB_SET_VR_CONTEXT (ZG, vr);

  if (PAL_TRUE != ospf_ins_found)
    {
      ret = OSPF_API_SET_ERR_PROCESS_NOT_EXIST;
    }
  return ret;
}
#endif /* HAVE_RESTART */

#ifdef HAVE_OSPF_MULTI_INST
s_int8_t
ospf_enable_ext_multi_inst (u_int32_t vr_id)
{
  struct ospf_master *om = NULL;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;
  
  SET_FLAG (om->flags, OSPF_EXT_MULTI_INST);
  return OSPF_API_SET_SUCCESS;  
}

s_int8_t
ospf_disable_ext_multi_inst (u_int32_t vr_id)
 {
   struct ospf_master *om = NULL;
   struct listnode *node = NULL;
   struct connected *ifc = NULL;
   struct interface *ifp = NULL;
   struct ospf_interface *oi = NULL, *oi_other = NULL;
   struct ospf *top_alt = NULL;
   struct listnode *node1 = NULL;
   struct ospf_network *nw = NULL;
   struct ls_node *rn = NULL;
 
   om = ospf_master_lookup_by_id (ZG, vr_id);
   if (om == NULL)
     return OSPF_API_SET_ERR_VR_NOT_EXIST;
   
   /* Check if this feature is enabled */
   if (!CHECK_FLAG (om->flags, OSPF_EXT_MULTI_INST))
     return OSPF_API_SET_ERR_EXT_MULTI_INST_NOT_ENABLED;
 
   /* Check if multi instance feature is already in use */
   LIST_LOOP (om->vr->ifm.if_list, ifp, node)
     for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
       {
         if (IPV4_ADDR_SAME (&ifc->address->u.prefix4, &IPv4LoopbackAddress))
           continue;
 
         oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                           ifp->ifindex);
         if (oi)
           {
             if (oi->network->inst_id != OSPF_IF_INSTANCE_ID_DEFAULT)
               {
                oi->network->inst_id  = OSPF_IF_INSTANCE_ID_DEFAULT;
                 ospf_if_update (oi);   
               }

             /* Deactivate the process on that subnet 
                for all other processes as there can be only one process 
                active with one instance-id  */
             if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
               {
                 LIST_LOOP (om->ospf, top_alt, node1)
                 {
                     /* Deactivate all other processes except the process
                        which is having its oi entry in global if-table */
                     if (oi->top != top_alt)
                       {
                         /* Then deactivate the process */
                         oi_other = ospf_if_entry_match (top_alt, oi);
                         if (oi_other)
                           ospf_if_delete (oi_other);
                       }
                   }
               }
             UNSET_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST);
           }
       }

 
   /* Check if any other networks are configured with non-zero instance-id
       If yes, make it as default instance-id */
     LIST_LOOP (om->ospf, top_alt, node1)
       {
         for (rn = ls_table_top (top_alt->networks); rn; rn = ls_route_next (rn))
           if ((nw = RN_INFO (rn, RNI_DEFAULT)))
             if (nw->inst_id != OSPF_IF_INSTANCE_ID_DEFAULT)
               nw->inst_id  = OSPF_IF_INSTANCE_ID_DEFAULT;
       }

  
   UNSET_FLAG (om->flags, OSPF_EXT_MULTI_INST);
   return OSPF_API_SET_SUCCESS;
 }
#endif /* HAVE_OSPF_MULTI_INST */

int
ospf_network_set (u_int32_t vr_id, int proc_id, struct pal_in4_addr addr,
                  u_char masklen, struct pal_in4_addr area_id, s_int16_t id)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ls_prefix lp;
  struct ospf_network *nw = NULL;
 
  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;
 
#ifdef HAVE_OSPF_MULTI_INST
  /* Check if this feature is not enabled through CLI */
  if (id != OSPF_IF_INSTANCE_ID_DEFAULT
      && !CHECK_FLAG (om->flags, OSPF_EXT_MULTI_INST))
    return OSPF_API_SET_ERR_IF_INST_ID_CANT_SET;

  /* Check the range of instance ID */
  if (!OSPF_API_CHECK_RANGE (id, INSTANCE_ID))
    return OSPF_API_SET_ERR_IF_INSTANCE_ID_INVALID;  
#endif /* HAVE_OSPF_MULTI_INST */
 
  ls_prefix_ipv4_set (&lp, masklen, addr);
  nw = ospf_network_lookup (top, &lp);
  if (nw)
    {
      /* Check if network has already enabled with another area */ 
      if (!IPV4_ADDR_SAME (&nw->area_id, &area_id))
        return OSPF_API_SET_ERR_NETWORK_OWNED_BY_ANOTHER_AREA;

#ifdef HAVE_OSPF_MULTI_INST
      /* Check if network has already enabled with another instance */
      if (nw->inst_id != id)
        return OSPF_API_SET_ERR_NETWORK_WITH_ANOTHER_INST_ID_EXIST;
#endif /* HAVE_OSPF_MULTI_INST */

      /* User has given same network statement */
      return OSPF_API_SET_ERR_NETWORK_EXIST;
    }

  return ospf_network_get (top, area_id, &lp, id);
}

int
ospf_network_unset (u_int32_t vr_id, int proc_id, struct pal_in4_addr addr,
                    u_char masklen, struct pal_in4_addr area_id, s_int16_t id)
{
  struct ospf_network *nw;
  struct ospf_master *om;
  struct ospf *top;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_prefix_ipv4_set (&p, masklen, addr);
  nw = ospf_network_lookup (top, &p);
  if (!nw)
    return OSPF_API_SET_ERR_NETWORK_NOT_EXIST;

  if (!IPV4_ADDR_SAME (&area_id, &nw->area_id))
    return OSPF_API_SET_ERR_AREA_ID_NOT_MATCH;

#ifdef HAVE_OSPF_MULTI_INST
  /* Check the range of instance ID */
  if (!OSPF_API_CHECK_RANGE (id, INSTANCE_ID))
    return OSPF_API_SET_ERR_IF_INSTANCE_ID_INVALID;

  /* Check for instance ID */
  if (nw->inst_id != id)
    return OSPF_API_SET_ERR_IF_INST_ID_NOT_MATCH;
#endif /* HAVE_OSPF_MULTI_INST */

  ospf_network_delete (top, nw);
  ospf_network_free (nw);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_network_format_set (u_int32_t vr_id, int proc_id,
                         struct pal_in4_addr addr, u_char masklen,
                         struct pal_in4_addr area_id, u_char format)
{
  struct ospf_network *nw;
  struct ospf_master *om;
  struct ospf *top;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_prefix_ipv4_set (&p, masklen, addr);
  nw = ospf_network_lookup (top, &p);
  if (!nw)
    return OSPF_API_SET_ERR_NETWORK_NOT_EXIST;

  nw->format = format;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_network_wildmask_set (u_int32_t vr_id, int proc_id,
                           struct pal_in4_addr addr, u_char masklen,
                           struct pal_in4_addr area_id, u_char wild)
{
  struct ospf_network *nw;
  struct ospf_master *om;
  struct ospf *top;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_prefix_ipv4_set (&p, masklen, addr);
  nw = ospf_network_lookup (top, &p);
  if (!nw)
    return OSPF_API_SET_ERR_NETWORK_NOT_EXIST;

  nw->wildcard = wild;

  return OSPF_API_SET_SUCCESS;
}

#ifdef HAVE_VRF_OSPF

int
ospf_get_hex_domainid_value (u_int8_t *domainid_value, u_int8_t *value)
{
  u_int8_t value1[6]; 
  int i;
  char *str;

  str = value;
  for (i = 0 ; i < pal_strlen(value) ; i++)
   {
     if (!pal_char_isxdigit((int)value[i]))
       return -1;
   }
  for (i = 0; i < OSPF_DOMAIN_ID_VLENGTH; i++)
   {
     pal_sscanf (str,"%2x", (unsigned int *)&value1[i]); 
     (str)+= 2;
   } 
  pal_mem_cpy (domainid_value, value1, sizeof(value1)); 

 return 0;
}

int
ospf_domain_id_set (u_int32_t vr_id, int proc_id, char *type,
                    u_int8_t *value, bool_t is_primary_did)
{
  struct ospf_master *om;
  struct ospf *top; 
  struct listnode *nn;

  struct ospf_vrf_domain_id *sec_did = NULL, *domainid = NULL;
  int ret = 0;
  bool_t free_domainid = PAL_FALSE;
  bool_t duplicate_id = PAL_FALSE;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

   /* Check to see if primary ID is configured 
    * before secondary ID is configured 
    */
  if (!CHECK_FLAG(top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY) 
      && !is_primary_did)
    return OSPF_API_SET_ERR_SEC_DOMAIN_ID;

   /* If the Domain-id entered is NULL */
  if (type == NULL && !pal_strcmp (value, "NULL"))
    {
      /* NULL cannot be configured if already some 
       *  non-NULL domain ids are configured. 
       */
      if (CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY))
        return OSPF_API_SET_ERR_NON_ZERO_DOMAIN_ID;
      else
        {
          SET_FLAG (top->config, OSPF_CONFIG_NULL_DOMAIN_ID);
          pal_mem_set(&top->pdomain_id, 0, sizeof (struct ospf_vrf_domain_id));
          return OSPF_API_SET_SUCCESS;
        }
    }
  if (!(domainid = XCALLOC (MTYPE_OSPF_DOMAIN_ID,
         sizeof(struct ospf_vrf_domain_id))))
    return OSPF_API_SET_ERROR;

  /* For IP address format, type is NULL. */
  if ((type == NULL) && pal_strcmp (value, "NULL"))
    {
      ret = pal_inet_pton (AF_INET, value, domainid->value);
      if (!ret)
        {
          XFREE (MTYPE_OSPF_DOMAIN_ID, domainid);
          return OSPF_API_SET_ERR_WRONG_VALUE; 
        }
       domainid->type = TYPE_IP_DID;
    }
  else
    {
      if (!pal_strcmp(type, "type-as"))
         domainid->type = TYPE_AS_DID;
      else if (!pal_strcmp(type, "type-as4"))
         domainid->type = TYPE_AS4_DID;
      else if (!pal_strcmp(type, "type-back-comp"))
         domainid->type = TYPE_BACK_COMP_DID;

      ret = ospf_get_hex_domainid_value(domainid->value, value);
      if (ret < 0)
        {
          XFREE (MTYPE_OSPF_DOMAIN_ID, domainid);
          return OSPF_API_SET_ERR_INVALID_HEX_VALUE;
        }
    }

    /* If primary domain-id configured then update pdomain_id value 
       in top structure  */
  if (is_primary_did == PAL_TRUE)
    {
      /* If primary is already configured then replace it with the new ID */
      if (CHECK_FLAG(top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY))
        {
          pal_mem_cpy (top->pdomain_id, domainid, 
               sizeof(struct ospf_vrf_domain_id));
          /* Free the allocated memory for domainid */
          free_domainid = PAL_TRUE; 
        }
      else
        {
          top->pdomain_id = domainid; 
          SET_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY);
          free_domainid = PAL_FALSE;
        }
      if (!CHECK_FLAG(top->config, OSPF_CONFIG_DOMAIN_ID_SEC))
        {
          if (free_domainid)
            XFREE (MTYPE_OSPF_DOMAIN_ID, domainid);
          return OSPF_API_SET_SUCCESS;
        }   
    }
  else
    {
      /* When configuring the primary Id as secondary ID*/
      if (!(pal_mem_cmp (top->pdomain_id, domainid,
                                sizeof(struct ospf_vrf_domain_id))))
        {
          XFREE (MTYPE_OSPF_DOMAIN_ID, domainid);
          return OSPF_API_SET_ERR_DUPLICATE_DOMAIN_ID;
        }
    }

  if (CHECK_FLAG(top->config, OSPF_CONFIG_DOMAIN_ID_SEC)) 
    {
      LIST_LOOP (top->domainid_list, sec_did, nn)
        {
          ret = pal_mem_cmp (sec_did, domainid, 
                             sizeof(struct ospf_vrf_domain_id));
          if (!ret)
            {
              duplicate_id = PAL_TRUE;
              break;
            }
          else
            continue;
        }

      if (!duplicate_id  && !is_primary_did) 
        listnode_add (top->domainid_list, domainid);
      else if (duplicate_id && is_primary_did)
        {
          /* Delete the secondary domain ID from the list 
           * as it has been configured as primary domain ID. 
           */
          listnode_delete (top->domainid_list, sec_did);

          if (LIST_ISEMPTY(top->domainid_list))
            UNSET_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_SEC);
        }
      else if (duplicate_id)
        {
          /* The given sec.domain ID is already been configured as
           * secondary domain ID earlier. Free the allocated memory
           * and return duplicate error. */
          XFREE (MTYPE_OSPF_DOMAIN_ID, domainid);
          return OSPF_API_SET_ERR_DUPLICATE_DOMAIN_ID;
        }
    }
  else
    {
      /* Secondary domain ID is configured for the first time. */
      listnode_add (top->domainid_list, domainid);
      SET_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_SEC);
    }

  if (free_domainid)
    XFREE (MTYPE_OSPF_DOMAIN_ID, domainid);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_domain_id_unset (u_int32_t vr_id, int proc_id, char *type,
                      u_int8_t *value, bool_t is_primary_did)
{
  struct ospf_master *om;
  struct ospf *top;
  struct listnode *nn, *next;

  struct ospf_vrf_domain_id *tmp = NULL, domainid;
  int ret = 0;
  bool_t match = PAL_FALSE;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (type == NULL)
    {
      /* If the Domain-id entered is NULL */
      if (!pal_strcmp (value, "NULL"))
        {
          UNSET_FLAG (top->config, OSPF_CONFIG_NULL_DOMAIN_ID);
          return OSPF_API_SET_SUCCESS;
        }
      else
        {
          /* If the domain-id entered is of IP-Address format */
          ret = pal_inet_pton (AF_INET, value, domainid.value);
          if (!ret)
            return OSPF_API_SET_ERR_WRONG_VALUE;
          domainid.type = TYPE_IP_DID;
        }
    }
  else
    {
      if (!pal_strcmp(type, "type-as"))
         domainid.type = TYPE_AS_DID;
      else if (!pal_strcmp(type, "type-as4"))
         domainid.type = TYPE_AS4_DID;
      else if (!pal_strcmp(type, "type-back-comp"))
         domainid.type = TYPE_BACK_COMP_DID;

      ret = ospf_get_hex_domainid_value(domainid.value, value);
      if (ret < 0)
        return OSPF_API_SET_ERR_INVALID_HEX_VALUE;
    }

  /* Check if the domain-id to be removed is primary then change 
     the pdomain_id value in top structure to zero */
  if (!pal_mem_cmp(&domainid, top->pdomain_id,
            sizeof (struct ospf_vrf_domain_id))
      && is_primary_did)
    {
      if (CHECK_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_SEC))
        {
          list_delete_all_node (top->domainid_list);
          UNSET_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_SEC);
        }

      pal_mem_set (top->pdomain_id, 0,sizeof (struct ospf_vrf_domain_id));
      UNSET_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_PRIMARY);
      return OSPF_API_SET_SUCCESS;
    }

   /* Check if the domain-id value to be removed exists in the list 
      if exists remove the domain-id value from the list */
  for (nn = LISTHEAD (top->domainid_list); nn; nn = next)
    {
      next = nn->next;
      if ((tmp = GETDATA (nn)) != NULL)
        {
          if (!pal_mem_cmp(tmp, &domainid, sizeof(struct ospf_vrf_domain_id)))
            {
              listnode_delete (top->domainid_list, tmp);

              /* set match if the domain-id value exists in the list */
              match = PAL_TRUE;

              if (LIST_ISEMPTY(top->domainid_list))
                UNSET_FLAG (top->config, OSPF_CONFIG_DOMAIN_ID_SEC);
              break;
            }
          else 
            continue;
        }
    }
  if (!match)
    return OSPF_API_SET_ERR_INVALID_DOMAIN_ID;
 
  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_VRF_OSPF */

int
ospf_router_id_set (u_int32_t vr_id, int proc_id,
                    struct pal_in4_addr router_id)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ospf *ospf_top = NULL;
  struct listnode *node;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  LIST_LOOP(om->ospf, ospf_top, node)
    if (CHECK_FLAG (ospf_top->config, OSPF_CONFIG_ROUTER_ID)
        && IPV4_ADDR_SAME (&ospf_top->router_id_config, &router_id))
      return OSPF_API_SET_ERR_SAME_ROUTER_ID_EXIST;

  top->router_id_config = router_id;
  SET_FLAG (top->config, OSPF_CONFIG_ROUTER_ID);

  ospf_router_id_update (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_router_id_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->router_id_config = OSPFRouterIDUnspec;
  UNSET_FLAG (top->config, OSPF_CONFIG_ROUTER_ID);

  ospf_router_id_update (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_passive_interface_set (u_int32_t vr_id, int proc_id, char *name)
{
  struct ospf_master *om;
  struct ospf *top;
  int ret = OSPF_API_SET_SUCCESS;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Add the passive interface.  */
  ret = ospf_passive_if_add (top, name, IPv4AddressUnspec);

  return ret;
}

int
ospf_passive_interface_set_by_addr (u_int32_t vr_id, int proc_id,
                                    char *name, struct pal_in4_addr addr)
{
  struct ospf_master *om;
  struct ospf *top;
  int ret = OSPF_API_SET_SUCCESS;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Add the passive interface.  */
  ret = ospf_passive_if_add (top, name, addr);

  return ret;
}

/* Make all the interfaces into passive */
int
ospf_passive_interface_default_set (u_int32_t vr_id, int proc_id)
{
  struct apn_vr *vr = NULL;
  struct listnode *node;
  struct interface *ifp;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vr = apn_vr_lookup_by_id (ZG, vr_id);
  if (vr)
    {
      LIST_LOOP (vr->ifm.if_list, ifp, node)
        {
          /* Add the passive interface.  */
          ospf_passive_if_add (top, ifp->name, IPv4AddressUnspec);
        }
      top->passive_if_default = PAL_TRUE; 
    }
    
  return OSPF_API_SET_SUCCESS;
}
int
ospf_passive_interface_unset (u_int32_t vr_id, int proc_id, char *name)
{
  struct ospf_master *om;
  struct ospf *top;
  int ret = OSPF_API_SET_SUCCESS;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Delete the passive interface.  */
  ret = ospf_passive_if_delete (top, name, IPv4AddressUnspec);

  return ret;
}

int
ospf_passive_interface_unset_by_addr (u_int32_t vr_id, int proc_id,
                                      char *name, struct pal_in4_addr addr)
{
  struct ospf_master *om;
  struct ospf *top;
  int ret = OSPF_API_SET_SUCCESS;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Delete the passive interface.  */
  ret = ospf_passive_if_delete (top, name, addr);

  return ret;
}
/* Remove all the interfaces from passive and no passive interface list */
int
ospf_passive_interface_default_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Check if passive interface default is set or not */
  if (top->passive_if_default == PAL_TRUE)
    {
      /* Delete the passive interface.  */
      ospf_passive_if_list_delete (top);

      /* Delete from no passive interface list */
      ospf_no_passive_if_list_delete(top);

      top->passive_if_default = PAL_FALSE;
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_host_entry_set (u_int32_t vr_id, int proc_id, struct pal_in4_addr addr,
                     struct pal_in4_addr area_id)
{
  struct ospf_host_entry *host;
  struct ospf_master *om;
  struct ospf *top;
  struct pal_in4_addr area_id_old;
  int tos = 0;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Check the host address */
  if (IN_EXPERIMENTAL (pal_ntoh32 (addr.s_addr))
      || IN_MULTICAST (pal_ntoh32 (addr.s_addr))
      || IPV4_ADDR_SAME (&addr, &IPv4LoopbackAddress))
    return OSPF_API_SET_ERR_INVALID_IPV4_ADDRESS;

  host = ospf_host_entry_lookup (top, addr, tos);
  if (host != NULL)
    {
      if (!IPV4_ADDR_SAME (&host->area_id, &area_id))
        {
          area_id_old = host->area_id;
          host->area_id = area_id;
        }
    }
  else
    {
      host = ospf_host_entry_new ();
      host->addr = addr;
      host->area_id = area_id;

      ospf_host_entry_insert (top, host);
    }

  ospf_host_entry_run (top, host);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_host_entry_cost_set (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr addr,
                          struct pal_in4_addr area_id, int cost)
{
  struct ospf_host_entry *host;
  struct ospf_master *om;
  struct ospf *top;
  int tos = 0;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (cost < 0 || cost > 65535)
    return OSPF_API_SET_ERR_COST_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  host = ospf_host_entry_lookup (top, addr, tos);
  if (host == NULL)
    return OSPF_API_SET_ERR_HOST_ENTRY_NOT_EXIST;

  if (!IPV4_ADDR_SAME (&host->area_id, &area_id))
    return OSPF_API_SET_ERR_AREA_ID_NOT_MATCH;

  if (host->metric != cost)
    {
      host->metric = cost;
      ospf_host_entry_run (top, host);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_host_entry_unset (u_int32_t vr_id, int proc_id, struct pal_in4_addr addr,
                       struct pal_in4_addr area_id)
{
  struct ospf_host_entry *host;
  struct ospf_master *om;
  struct ospf *top;
  int tos = 0;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  host = ospf_host_entry_lookup (top, addr, tos);
  if (host == NULL)
    return OSPF_API_SET_ERR_HOST_ENTRY_NOT_EXIST;

  if (!IPV4_ADDR_SAME (&host->area_id, &area_id))
    return OSPF_API_SET_ERR_AREA_ID_NOT_MATCH;

  ospf_host_entry_delete (top, host);
  ospf_host_entry_free (host);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_host_entry_cost_unset (u_int32_t vr_id, int proc_id,
                            struct pal_in4_addr addr,
                            struct pal_in4_addr area_id)
{
  struct ospf_host_entry *host;
  struct ospf_master *om;
  struct ospf *top;
  int tos = 0;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  host = ospf_host_entry_lookup (top, addr, tos);
  if (host == NULL)
    return OSPF_API_SET_ERR_HOST_ENTRY_NOT_EXIST;

  if (!IPV4_ADDR_SAME (&host->area_id, &area_id))
    return OSPF_API_SET_ERR_AREA_ID_NOT_MATCH;

  if (host->metric != 0)
    {
      host->metric = 0;
      ospf_host_entry_run (top, host);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_host_entry_format_set (u_int32_t vr_id, int proc_id,
                            struct pal_in4_addr addr,
                            struct pal_in4_addr area_id, u_char format)
{
  struct ospf_host_entry *host;
  struct ospf_master *om;
  struct ospf *top;
  int tos = 0;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  host = ospf_host_entry_lookup (top, addr, tos);
  if (host == NULL)
    return OSPF_API_SET_ERR_HOST_ENTRY_NOT_EXIST;

  host->format = format;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_abr_type_set (u_int32_t vr_id, int proc_id, u_char type)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (type != OSPF_ABR_TYPE_STANDARD
      && type != OSPF_ABR_TYPE_CISCO
      && type != OSPF_ABR_TYPE_IBM
      && type != OSPF_ABR_TYPE_SHORTCUT)
    return OSPF_API_SET_ERR_ABR_TYPE_INVALID;

  if (top->abr_type != type)
    {
      top->abr_type = type;
      ospf_proc_update (top);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_abr_type_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (top->abr_type != OSPF_ABR_TYPE_DEFAULT)
    {
      top->abr_type = OSPF_ABR_TYPE_DEFAULT;
      ospf_proc_update (top);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_compatible_rfc1583_set (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (!IS_RFC1583_COMPATIBLE (top))
    {
      SET_FLAG (top->config, OSPF_CONFIG_RFC1583_COMPATIBLE);
      ospf_proc_update (top);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_compatible_rfc1583_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (IS_RFC1583_COMPATIBLE (top))
    {
      UNSET_FLAG (top->config, OSPF_CONFIG_RFC1583_COMPATIBLE);
      ospf_proc_update (top);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_summary_address_set (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr addr, u_char masklen)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ospf_summary *summary;
  struct ls_prefix lp;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_prefix_ipv4_set (&lp, masklen, addr);
  summary = ospf_summary_lookup (top->summary, &lp);
  if (summary)
    return OSPF_API_SET_ERR_SUMMARY_ADDRESS_EXIST;

  summary = ospf_summary_new ();
  ospf_summary_address_add (top, summary, &lp);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_summary_address_unset (u_int32_t vr_id, int proc_id,
                            struct pal_in4_addr addr, u_char masklen)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ospf_summary *summary;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  summary = ospf_summary_lookup_by_addr (top->summary, addr, masklen);
  if (!summary)
    return OSPF_API_SET_ERR_SUMMARY_ADDRESS_NOT_EXIST;

  ospf_summary_address_delete (top, summary);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_summary_address_tag_set (u_int32_t vr_id, int proc_id,
                              struct pal_in4_addr addr, u_char masklen,
                              u_int32_t tag)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ospf_summary *summary;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  summary = ospf_summary_lookup_by_addr (top->summary, addr, masklen);
  if (!summary)
    return OSPF_API_SET_ERR_SUMMARY_ADDRESS_NOT_EXIST;

  if (summary->tag != tag)
    {
      SET_FLAG (summary->flags, OSPF_SUMMARY_ADDRESS_ADVERTISE);
      summary->tag = tag;
      OSPF_TIMER_ON (summary->t_update, ospf_summary_address_timer,
                     summary, OSPF_SUMMARY_TIMER_DELAY);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_summary_address_tag_unset (u_int32_t vr_id, int proc_id,
                                struct pal_in4_addr addr, u_char masklen)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ospf_summary *summary;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  summary = ospf_summary_lookup_by_addr (top->summary, addr, masklen);
  if (!summary)
    return OSPF_API_SET_ERR_SUMMARY_ADDRESS_NOT_EXIST;

  if (summary->tag != 0)
    {
      summary->tag = 0;
      OSPF_TIMER_ON (summary->t_update, ospf_summary_address_timer,
                     summary, OSPF_SUMMARY_TIMER_DELAY);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_summary_address_not_advertise_set (u_int32_t vr_id, int proc_id,
                                        struct pal_in4_addr addr,
                                        u_char masklen)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ospf_summary *summary;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  summary = ospf_summary_lookup_by_addr (top->summary, addr, masklen);
  if (!summary)
    return OSPF_API_SET_ERR_SUMMARY_ADDRESS_NOT_EXIST;

  if (CHECK_FLAG (summary->flags, OSPF_SUMMARY_ADDRESS_ADVERTISE))
    {
      summary->tag = 0;
      UNSET_FLAG (summary->flags, OSPF_SUMMARY_ADDRESS_ADVERTISE);
      OSPF_TIMER_ON (summary->t_update, ospf_summary_address_timer,
                     summary, OSPF_SUMMARY_TIMER_DELAY);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_summary_address_not_advertise_unset (u_int32_t vr_id, int proc_id,
                                          struct pal_in4_addr addr,
                                          u_char masklen)
{
  struct ospf_master *om;
  struct ospf *top;
  struct ospf_summary *summary;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  summary = ospf_summary_lookup_by_addr (top->summary, addr, masklen);
  if (!summary)
    return OSPF_API_SET_ERR_SUMMARY_ADDRESS_NOT_EXIST;

  if (!CHECK_FLAG (summary->flags, OSPF_SUMMARY_ADDRESS_ADVERTISE))
    {
      SET_FLAG (summary->flags, OSPF_SUMMARY_ADDRESS_ADVERTISE);
      OSPF_TIMER_ON (summary->t_update, ospf_summary_address_timer,
                     summary, OSPF_SUMMARY_TIMER_DELAY);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_timers_spf_set (u_int32_t vr_id, int proc_id,
                     u_int32_t min_delay, u_int32_t max_delay)
{
  struct ospf_master *om;
  struct ospf *top;
  u_int32_t min_sec, min_usec, max_sec, max_usec;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  min_sec = min_delay / ONE_SEC_MILLISECOND;
  min_usec = (min_delay % ONE_SEC_MILLISECOND) * MILLISEC_TO_MICROSEC;

  max_sec = max_delay / ONE_SEC_MILLISECOND;
  max_usec = (max_delay % ONE_SEC_MILLISECOND) * MILLISEC_TO_MICROSEC;

  if ((min_sec > PAL_TIME_MAX_TV_SEC || max_sec > PAL_TIME_MAX_TV_USEC) ||
      (min_delay > max_delay))
    return OSPF_API_SET_ERR_TIMER_VALUE_INVALID;

  top->spf_min_delay.tv_sec = min_sec;
  top->spf_max_delay.tv_sec = max_sec;

  top->spf_min_delay.tv_usec = min_usec;
  top->spf_max_delay.tv_usec = max_usec;

  /* Reset the SPF timer.  */
  ospf_spf_calculate_timer_reset_all (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_timers_spf_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->spf_min_delay.tv_sec = OSPF_SPF_MIN_DELAY_DEFAULT / ONE_SEC_MILLISECOND;
  top->spf_min_delay.tv_usec = (OSPF_SPF_MIN_DELAY_DEFAULT % ONE_SEC_MILLISECOND)
                               * MILLISEC_TO_MICROSEC;

  top->spf_max_delay.tv_sec = OSPF_SPF_MAX_DELAY_DEFAULT / ONE_SEC_MILLISECOND;
  top->spf_max_delay.tv_usec = (OSPF_SPF_MAX_DELAY_DEFAULT % ONE_SEC_MILLISECOND)
                               * MILLISEC_TO_MICROSEC;

  /* Reset the SPF timer.  */
  ospf_spf_calculate_timer_reset_all (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_timers_refresh_set (u_int32_t vr_id, int proc_id, int interval)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (interval < 10 || interval > 1800)
    return OSPF_API_SET_ERR_TIMER_VALUE_INVALID;

  if (top->lsa_refresh.interval == interval)
    return OSPF_API_SET_SUCCESS;

  top->lsa_refresh.interval = interval;

  /* Restart the refresh timer.  */
  ospf_lsa_refresh_restart (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_timers_refresh_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->lsa_refresh.interval = OSPF_LSA_REFRESH_INTERVAL_DEFAULT;

  /* Restart the refresh timer.  */
  ospf_lsa_refresh_restart (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_auto_cost_reference_bandwidth_set (u_int32_t vr_id,
                                        int proc_id, int refbw)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (refbw < 1 || refbw > 4294967)
    return OSPF_API_SET_ERR_REFERENCE_BANDWIDTH_INVALID;

  /* If reference bandwidth is changed. */
  if ((refbw * 1000) == top->ref_bandwidth)
    return OSPF_API_SET_SUCCESS;

  top->ref_bandwidth = refbw * 1000;
  ospf_if_output_cost_apply_all (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_auto_cost_reference_bandwidth_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (top->ref_bandwidth == OSPF_DEFAULT_REF_BANDWIDTH)
    return OSPF_API_SET_SUCCESS;

  top->ref_bandwidth = OSPF_DEFAULT_REF_BANDWIDTH;
  ospf_if_output_cost_apply_all (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_max_concurrent_dd_set (u_int32_t vr_id, int proc_id, u_int16_t max_dd)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (top->max_dd != max_dd)
    top->max_dd = max_dd;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_max_concurrent_dd_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (top->max_dd != OSPF_MAX_CONCURRENT_DD_DEFAULT)
    top->max_dd = OSPF_MAX_CONCURRENT_DD_DEFAULT;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_max_unuse_packet_set (u_int32_t vr_id, int proc_id, u_int32_t max)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Update the maximum number of unused OSPF packets.  */
  ospf_packet_unuse_max_update (top, max);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_max_unuse_packet_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Update the maximum number of unused OSPF packets.  */
  ospf_packet_unuse_max_update (top, OSPF_PACKET_UNUSE_MAX_DEFAULT);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_max_unuse_lsa_set (u_int32_t vr_id, int proc_id, u_int32_t max)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Update the maximum number of unused LSAs.  */
  ospf_lsa_unuse_max_update (top, max);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_max_unuse_lsa_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Update the maximum number of unused LSAs.  */
  ospf_lsa_unuse_max_update (top, OSPF_LSA_UNUSE_MAX_DEFAULT);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_config_check (u_int32_t vr_id, int proc_id,
                              struct pal_in4_addr addr, int config)
{
  struct ospf_if_params *oip;
  struct ospf_interface *oi;
  struct ospf_master *om;
  struct ospf *top;
  struct ls_node *rn;
  struct ls_prefix p;
  int flag = 0;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if (prefix_match (oi->ident.address, (struct prefix *)&p))
        {
          flag = 1;
          ls_unlock_node (rn);
          oip = oi->params_default;
          if (oip == NULL
              || !OSPF_IF_PARAM_CHECK (oip, NETWORK_TYPE)
              || oip->type == OSPF_IFTYPE_BROADCAST
              || oip->type == OSPF_IFTYPE_POINTOPOINT)
            return OSPF_API_SET_ERR_NBR_CONFIG_INVALID;

          switch (oip->type)
            {
            case OSPF_IFTYPE_NBMA:
              if (CHECK_FLAG (config, OSPF_API_NBR_CONFIG_COST))
                return OSPF_API_SET_ERR_NBR_NBMA_CONFIG_INVALID;
              break;

            case OSPF_IFTYPE_POINTOMULTIPOINT:
              if (CHECK_FLAG (config, OSPF_API_NBR_CONFIG_POLL_INTERVAL)
                  || CHECK_FLAG (config, OSPF_API_NBR_CONFIG_PRIORITY))
                return OSPF_API_SET_ERR_NBR_P2MP_CONFIG_INVALID;
              else if (!CHECK_FLAG (config, OSPF_API_NBR_CONFIG_COST))
                return OSPF_API_SET_ERR_NBR_P2MP_CONFIG_REQUIRED;
              break;

            case OSPF_IFTYPE_POINTOMULTIPOINT_NBMA:
              if (CHECK_FLAG (config, OSPF_API_NBR_CONFIG_POLL_INTERVAL)
                  || CHECK_FLAG (config, OSPF_API_NBR_CONFIG_PRIORITY))
                return OSPF_API_SET_ERR_NBR_P2MP_CONFIG_INVALID;
              break;
            }
          return OSPF_API_SET_SUCCESS;
        }

  if (!flag)
    return OSPF_API_WARN_INVALID_NETWORK;

  return OSPF_API_SET_ERR_INVALID_NETWORK;
}

int
ospf_nbr_static_set (u_int32_t vr_id, int proc_id, struct pal_in4_addr addr)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_interface *oi;
  struct ospf_master *om;
  struct ospf *top;
  struct ls_node *rn;
  struct ls_prefix p;
  int flag = 0;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_OSPF_NETWORK_NBMA (oi))
        if (prefix_match (oi->ident.address, (struct prefix *)&p))
          {
            flag = 1;
            nbr_static = ospf_nbr_static_new ();
            if (nbr_static != NULL)
              {
                nbr_static->addr = addr;
                ospf_nbr_static_add (top, nbr_static);
                ospf_nbr_static_up (nbr_static, oi);
              }
            ls_unlock_node (rn);
            break;
          }

  if (!flag)
   {
    nbr_static = ospf_nbr_static_new ();
    nbr_static->addr = addr;
    ospf_nbr_static_add (top, nbr_static);
   }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_unset (u_int32_t vr_id, int proc_id, struct pal_in4_addr addr)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (!nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST;

  if (CHECK_FLAG (nbr_static->flags, OSPF_NBR_STATIC_COST))
    ospf_nbr_static_cost_unset (vr_id, proc_id, addr);

  ospf_nbr_static_delete (top, nbr_static);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_priority_set (u_int32_t vr_id, int proc_id,
                              struct pal_in4_addr addr, u_char priority)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (!nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST;

  if (nbr_static->priority != priority)
    ospf_nbr_static_priority_apply (nbr_static, priority);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_priority_unset (u_int32_t vr_id, int proc_id,
                                struct pal_in4_addr addr)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (!nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST;

  if (CHECK_FLAG (nbr_static->flags, OSPF_NBR_STATIC_PRIORITY))
    {
      nbr_static->priority = OSPF_NEIGHBOR_PRIORITY_DEFAULT;
      UNSET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_PRIORITY);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_poll_interval_set (u_int32_t vr_id, int proc_id,
                                   struct pal_in4_addr addr, int interval)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (!nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST;

  if (nbr_static->poll_interval != interval)
    {
      nbr_static->poll_interval = interval;
      if (nbr_static->poll_interval != OSPF_POLL_INTERVAL_DEFAULT)
        SET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_POLL_INTERVAL);
      else
        UNSET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_POLL_INTERVAL);

      if (nbr_static->nbr)
        if (nbr_static->nbr->oi)
          {
            if (nbr_static->nbr->oi->type == OSPF_IFTYPE_NBMA)
              {
                if (ospf_if_is_up (nbr_static->nbr->oi))
                  {
                    OSPF_TIMER_OFF (nbr_static->t_poll);
                    OSPF_POLL_TIMER_ON (nbr_static->t_poll, ospf_hello_poll_timer,
                                        nbr_static->poll_interval);
                  }
              }
            else
              {
                return OSPF_API_SET_ERR_NBR_P2MP_CONFIG_INVALID;
              }
          }  
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_poll_interval_unset (u_int32_t vr_id, int proc_id,
                                     struct pal_in4_addr addr)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (!nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST;

  if (nbr_static->poll_interval != OSPF_POLL_INTERVAL_DEFAULT)
    {
      nbr_static->poll_interval = OSPF_POLL_INTERVAL_DEFAULT;
      UNSET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_POLL_INTERVAL);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_cost_set (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr addr, u_int16_t cost)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (!nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST;

  if (nbr_static->cost != cost)
    {
      nbr_static->cost = cost;
      if (nbr_static->cost != OSPF_OUTPUT_COST_DEFAULT)
        SET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_COST);
      else
        UNSET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_COST);

      /* Generate router-LSA. */
      ospf_nbr_static_cost_apply (top, nbr_static);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_nbr_static_cost_unset (u_int32_t vr_id, int proc_id,
                            struct pal_in4_addr addr)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  nbr_static = ospf_nbr_static_lookup (top, addr);
  if (!nbr_static)
    return OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST;

  if (nbr_static->cost != OSPF_OUTPUT_COST_DEFAULT)
    {
      nbr_static->cost = OSPF_OUTPUT_COST_DEFAULT;
      UNSET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_COST);

      /* Generate router-LSA. */
      ospf_nbr_static_cost_apply (top, nbr_static);
    }

  return OSPF_API_SET_SUCCESS;
}

#ifdef HAVE_OSPF_DB_OVERFLOW
int
ospf_overflow_database_external_limit_set (u_int32_t vr_id, int proc_id,
                                           u_int32_t limit)
{
  struct ospf_master *om;
  struct ospf *top;
  u_int32_t old_limit;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  old_limit = top->ext_lsdb_limit;

  top->ext_lsdb_limit = limit;
  if (ospf_lsdb_count_external_non_default (top) >= top->ext_lsdb_limit)
    ospf_db_overflow_enter (top);
  else if (old_limit < top->ext_lsdb_limit)
    ospf_db_overflow_exit (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_overflow_database_external_limit_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->ext_lsdb_limit = OSPF_DEFAULT_LSDB_LIMIT;
  ospf_db_overflow_exit (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_overflow_database_external_interval_set (u_int32_t vr_id, int proc_id,
                                              int interval)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->exit_overflow_interval = interval;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_overflow_database_external_interval_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->exit_overflow_interval = OSPF_DEFAULT_EXIT_OVERFLOW_INTERVAL;

  return OSPF_API_SET_SUCCESS;
}

#endif /* HAVE_OSPF_DB_OVERFLOW */

int
ospf_area_format_set (u_int32_t vr_id, int proc_id,
                      struct pal_in4_addr area_id, u_char format)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (format != OSPF_AREA_ID_FORMAT_ADDRESS &&
      format != OSPF_AREA_ID_FORMAT_DECIMAL)
    return OSPF_API_SET_ERR_AREA_ID_FORMAT_INVALID;

  /* If Area ID format is not set yet. */
  if (area->conf.format == OSPF_AREA_ID_FORMAT_DEFAULT)
    area->conf.format = format;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_network_set (u_int32_t vr_id, char *name, int type)
{
  struct ospf_if_params *oip_default;
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_table *rt;
  struct ls_node *rn;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type != OSPF_IFTYPE_POINTOPOINT
      && type != OSPF_IFTYPE_BROADCAST
      && type != OSPF_IFTYPE_NBMA
      && type != OSPF_IFTYPE_POINTOMULTIPOINT
      && type != OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    return OSPF_API_SET_ERR_NETWORK_TYPE_INVALID;

  ifp = if_lookup_by_name (&om->vr->ifm, name);
  if (ifp != NULL)
    if (if_is_loopback (ifp))
      if (type != OSPF_IFTYPE_POINTOPOINT)
        return OSPF_API_SET_SUCCESS;

  oip_default = ospf_if_params_get_default (om, name);
  if (oip_default->type == type)
    return OSPF_API_SET_SUCCESS;

  /* Update the interface type for other interface params.  */
  rt = ospf_if_params_table_lookup_by_name (om, name);
  if (rt != NULL)
    for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
      if ((oip = RN_INFO (rn, RNI_DEFAULT)) != NULL)
        if (oip != oip_default)
          oip->type = type;

  oip_default->type = type;
  OSPF_IF_PARAM_SET (oip_default, NETWORK_TYPE);
  OSPF_IF_PARAM_HELLO_INTERVAL_UPDATE (oip_default);
  OSPF_IF_PARAM_DEAD_INTERVAL_UPDATE (oip_default);

  /* Update the OSPF interfaces.  */
  ospf_if_update_by_params (oip_default);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_network_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip_default;
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_table *rt;
  struct ls_node *rn;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip_default = ospf_if_params_lookup_default (om, name);
  if (oip_default == NULL
      || !OSPF_IF_PARAM_CHECK (oip_default, NETWORK_TYPE))
    return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;

  /* Update the interface type for other interface params.  */
  rt = ospf_if_params_table_lookup_by_name (om, name);
  if (rt != NULL)
    for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
      if ((oip = RN_INFO (rn, RNI_DEFAULT)) != NULL)
        if (oip != oip_default)
          oip->type = OSPF_IFTYPE_NONE;

  oip_default->type = OSPF_IFTYPE_NONE;
  OSPF_IF_PARAM_UNSET (oip_default, NETWORK_TYPE);
  OSPF_IF_PARAM_HELLO_INTERVAL_UPDATE (oip_default);
  OSPF_IF_PARAM_DEAD_INTERVAL_UPDATE (oip_default);

  /* Update the OSPF interfaces.  */
  ospf_if_update_by_params (oip_default);

  if (OSPF_IF_PARAMS_EMPTY (oip_default))
    ospf_if_params_delete_default (om, name);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_network_p2mp_nbma_set (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip_default;
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_table *rt;
  struct ls_node *rn;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ifp = if_lookup_by_name (&om->vr->ifm, name);
  if (ifp != NULL)
    if (if_is_loopback (ifp))
      return OSPF_API_SET_SUCCESS;

  oip_default = ospf_if_params_get_default (om, name);
  if (oip_default->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    return OSPF_API_SET_SUCCESS;

  rt = ospf_if_params_table_lookup_by_name (om, name);
  if (rt != NULL)
    for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
      if ((oip = RN_INFO (rn, RNI_DEFAULT)))
        if (oip != oip_default)
          oip->type = OSPF_IFTYPE_POINTOMULTIPOINT_NBMA;

  oip_default->type = OSPF_IFTYPE_POINTOMULTIPOINT_NBMA;
  OSPF_IF_PARAM_SET (oip_default, NETWORK_TYPE);
  OSPF_IF_PARAM_HELLO_INTERVAL_UPDATE (oip_default);
  OSPF_IF_PARAM_DEAD_INTERVAL_UPDATE (oip_default);

  /* Update the OSPF interfaces.  */
  ospf_if_update_by_params (oip_default);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_authentication_type_set (u_int32_t vr_id, char *name, u_char type)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);

  return ospf_if_params_authentication_type_set (oip, type);
}

int
ospf_if_authentication_type_set_by_addr (u_int32_t vr_id, char *name,
                                         struct pal_in4_addr addr,
                                         u_char type)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  return ospf_if_params_authentication_type_set (oip, type);
}

int
ospf_if_authentication_type_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_authentication_type_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      return OSPF_API_SET_SUCCESS;
    }

  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_authentication_type_unset_by_addr (u_int32_t vr_id, char *name,
                                           struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ospf_if_params_authentication_type_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      return OSPF_API_SET_SUCCESS;
    }

  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_priority_set (u_int32_t vr_id, char *name, u_char priority)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);
  ospf_if_params_priority_set (oip, priority);

  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete_default (om, name);

  if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
    for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
      ospf_if_priority_apply (om, ifc->address->u.prefix4,
                              ifp->ifindex, priority);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_priority_set_by_addr (u_int32_t vr_id, char *name,
                              struct pal_in4_addr addr, u_char priority)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  ospf_if_params_priority_set (oip, priority);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete (om, name, &p);

  if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
    ospf_if_priority_apply (om, addr, ifp->ifindex, priority);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_priority_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_priority_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
          ospf_if_priority_apply (om, ifc->address->u.prefix4, ifp->ifindex,
                                  OSPF_ROUTER_PRIORITY_DEFAULT);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_priority_unset_by_addr (u_int32_t vr_id, char *name,
                                struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ospf_if_params_priority_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_priority_apply (om, addr, ifp->ifindex,
                                OSPF_ROUTER_PRIORITY_DEFAULT);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_mtu_set (u_int32_t vr_id, char *name, u_int16_t mtu)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);
  ospf_if_params_mtu_set (oip, mtu);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_mtu_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_mtu_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_mtu_ignore_set (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);
  ospf_if_params_mtu_ignore_set (oip);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_mtu_ignore_set_by_addr (u_int32_t vr_id, char *name,
                                struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);
  ospf_if_params_mtu_ignore_set (oip);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_mtu_ignore_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_mtu_ignore_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_mtu_ignore_unset_by_addr (u_int32_t vr_id, char *name,
                                  struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ospf_if_params_mtu_ignore_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_cost_set (u_int32_t vr_id, char *name, u_int32_t cost)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (cost < OSPF_OUTPUT_COST_MIN || cost > OSPF_OUTPUT_COST_MAX)
    return OSPF_API_SET_ERR_COST_INVALID;

  oip = ospf_if_params_get_default (om, name);
  if (!OSPF_IF_PARAM_CHECK (oip, OUTPUT_COST) || oip->output_cost != cost)
    {
      ret = ospf_if_params_cost_set (oip, cost);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      if (ret != OSPF_API_SET_SUCCESS)
        return ret;

      /* If physical interface exists, apply the cost. */
      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
          ospf_if_output_cost_apply (om, ifc->address->u.prefix4,
                                     ifp->ifindex);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_cost_set_by_addr (u_int32_t vr_id, char *name,
                          struct pal_in4_addr addr, u_int32_t cost)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (cost < OSPF_OUTPUT_COST_MIN || cost > OSPF_OUTPUT_COST_MAX)
    return OSPF_API_SET_ERR_COST_INVALID;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  if (!OSPF_IF_PARAM_CHECK (oip, OUTPUT_COST) || oip->output_cost != cost)
    {
      ret = ospf_if_params_cost_set (oip, cost);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      if (ret != OSPF_API_SET_SUCCESS)
        return ret;

      /* If physical interface exists, apply the cost. */
      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_output_cost_apply (om, addr, ifp->ifindex);
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_cost_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (OSPF_IF_PARAM_CHECK (oip, OUTPUT_COST))
    {
      ospf_if_params_cost_unset (oip);

      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      /* If physical interface exists, apply the cost. */
      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
          ospf_if_output_cost_apply (om, ifc->address->u.prefix4,
                                     ifp->ifindex);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_cost_unset_by_addr (u_int32_t vr_id, char *name,
                            struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);

  if (OSPF_IF_PARAM_CHECK (oip, OUTPUT_COST))
    {
      ospf_if_params_cost_unset (oip);

      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      /* If physical interface exists, apply the cost. */
      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_output_cost_apply (om, addr, ifp->ifindex);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

#ifdef HAVE_OSPF_TE
int
ospf_if_te_metric_set (u_int32_t vr_id, char *name, u_int32_t metric)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);

  if (OSPF_IF_PARAM_CHECK (oip, TE_METRIC))
    {
      if (oip->te_metric == metric)
       return OSPF_API_SET_ERR_TELINK_METRIC_EXIST;
    }

  ret = ospf_if_params_te_metric_set (oip, metric);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete_default (om, name);

  return ret;
}

int
ospf_if_te_metric_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (OSPF_IF_PARAM_CHECK (oip, TE_METRIC))
    {
      ospf_if_params_te_metric_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

#endif /* HAVE_OSPF_TE */

int
ospf_if_transmit_delay_set (u_int32_t vr_id, char *name, u_int32_t delay)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);

  ret = ospf_if_params_transmit_delay_set (oip, delay);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete_default (om, name);

  return ret;
}

int
ospf_if_transmit_delay_set_by_addr (u_int32_t vr_id, char *name,
                                    struct pal_in4_addr addr, u_int32_t delay)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  ret = ospf_if_params_transmit_delay_set (oip, delay);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete (om, name, &p);

  return ret;
}

int
ospf_if_transmit_delay_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_transmit_delay_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_transmit_delay_unset_by_addr (u_int32_t vr_id, char *name,
                                      struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ospf_if_params_transmit_delay_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_retransmit_interval_set (u_int32_t vr_id,
                                 char *name, u_int32_t interval)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct interface *ifp;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);

  ret = ospf_if_params_retransmit_interval_set (oip, interval);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete_default (om, name);

  if (ret != OSPF_API_SET_SUCCESS)
    return ret;

  if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
    ospf_if_nbr_timers_update (ifp);

  if ((vlink = ospf_vlink_lookup_by_name (om, name)))
    ospf_if_nbr_timers_update_by_vlink (vlink);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_retransmit_interval_set_by_addr (u_int32_t vr_id, char *name,
                                         struct pal_in4_addr addr,
                                         u_int32_t interval)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  ret = ospf_if_params_retransmit_interval_set (oip, interval);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete (om, name, &p);

  if (ret != OSPF_API_SET_SUCCESS)
    return ret;

  if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
    ospf_if_nbr_timers_update_by_addr (ifp, addr);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_retransmit_interval_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_retransmit_interval_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_nbr_timers_update (ifp);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_retransmit_interval_unset_by_addr (u_int32_t vr_id, char *name,
                                           struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ospf_if_params_retransmit_interval_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_nbr_timers_update_by_addr (ifp, addr);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_hello_interval_set (u_int32_t vr_id, char *name, u_int32_t interval)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (interval < OSPF_HELLO_INTERVAL_MIN
      || interval > OSPF_HELLO_INTERVAL_MAX)
    return OSPF_API_SET_ERR_IF_HELLO_INTERVAL_INVALID;

  oip = ospf_if_params_get_default (om, name);

  ret = ospf_if_params_hello_interval_set (oip, interval);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete_default (om, name);

  return ret;
}

int
ospf_if_hello_interval_set_by_addr (u_int32_t vr_id, char *name,
                                    struct pal_in4_addr addr,
                                    u_int32_t interval)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (interval < OSPF_HELLO_INTERVAL_MIN
      || interval > OSPF_HELLO_INTERVAL_MAX)
    return OSPF_API_SET_ERR_IF_HELLO_INTERVAL_INVALID;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  ret = ospf_if_params_hello_interval_set (oip, interval);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete (om, name, &p);

  return ret;
}

int
ospf_if_hello_interval_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ret = ospf_if_params_hello_interval_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      return ret;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_hello_interval_unset_by_addr (u_int32_t vr_id, char *name,
                                      struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ret = ospf_if_params_hello_interval_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      return ret;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_dead_interval_set (u_int32_t vr_id, char *name, u_int32_t interval)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (interval < OSPF_ROUTER_DEAD_INTERVAL_MIN
      || interval > OSPF_ROUTER_DEAD_INTERVAL_MAX)
    return OSPF_API_SET_ERR_IF_DEAD_INTERVAL_INVALID;

  oip = ospf_if_params_get_default (om, name);

  ret = ospf_if_params_dead_interval_set (oip, interval);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete_default (om, name);

  if (ret != OSPF_API_SET_SUCCESS)
    return ret;

  if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
    ospf_if_nbr_timers_update (ifp);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_dead_interval_set_by_addr (u_int32_t vr_id, char *name,
                                   struct pal_in4_addr addr,
                                   u_int32_t interval)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (interval < OSPF_ROUTER_DEAD_INTERVAL_MIN
      || interval > OSPF_ROUTER_DEAD_INTERVAL_MAX)
    return OSPF_API_SET_ERR_IF_DEAD_INTERVAL_INVALID;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  ret = ospf_if_params_dead_interval_set (oip, interval);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete (om, name, &p);

  if (ret != OSPF_API_SET_SUCCESS)
    return ret;

  if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
    ospf_if_nbr_timers_update_by_addr (ifp, addr);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_dead_interval_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ret = ospf_if_params_dead_interval_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_nbr_timers_update (ifp);

      return ret;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_dead_interval_unset_by_addr (u_int32_t vr_id, char *name,
                                     struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ret = ospf_if_params_dead_interval_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_nbr_timers_update_by_addr (ifp, addr);

      return ret;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_authentication_key_set (u_int32_t vr_id, char *name, char *auth_key)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);
  ospf_if_params_authentication_key_set (oip, auth_key);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_authentication_key_set_by_addr (u_int32_t vr_id, char *name,
                                        struct pal_in4_addr addr,
                                        char *auth_key)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);
  ospf_if_params_authentication_key_set (oip, auth_key);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_authentication_key_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ret = ospf_if_params_authentication_key_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      return ret;
    }

  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_authentication_key_unset_by_addr (u_int32_t vr_id, char *name,
                                          struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ret = ospf_if_params_authentication_key_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      return ret;
    }

  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_message_digest_key_set (u_int32_t vr_id, char *name, u_char key_id,
                                char *auth_key)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);

  return ospf_if_params_message_digest_key_set (oip, key_id, auth_key, om);
}

int
ospf_if_message_digest_key_set_by_addr (u_int32_t vr_id, char *name,
                                        struct pal_in4_addr addr,
                                        u_char key_id, char *auth_key)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  return ospf_if_params_message_digest_key_set (oip, key_id, auth_key, om);
}

int
ospf_if_message_digest_key_unset (u_int32_t vr_id, char *name, u_char key_id)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (OSPF_IF_PARAM_CHECK (oip, AUTH_CRYPT))
    {
      ret = ospf_if_params_message_digest_key_unset (oip, key_id);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      return ret;
    }

  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_message_digest_key_unset_by_addr (u_int32_t vr_id, char *name,
                                          struct pal_in4_addr addr,
                                          u_char key_id)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (OSPF_IF_PARAM_CHECK (oip, AUTH_CRYPT))
    {
      ret = ospf_if_params_message_digest_key_unset (oip, key_id);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      return ret;
    }

  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_database_filter_set (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);

  if (!OSPF_IF_PARAM_CHECK (oip, DATABASE_FILTER))
    {
      ospf_if_params_database_filter_set (oip);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
          ospf_if_database_filter_apply (om, ifc->address->u.prefix4,
                                         ifp->ifindex);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_database_filter_set_by_addr (u_int32_t vr_id, char *name,
                                     struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  if (!OSPF_IF_PARAM_CHECK (oip, DATABASE_FILTER))
    {
      ospf_if_params_database_filter_set (oip);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_database_filter_apply (om, addr, ifp->ifindex);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_database_filter_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_database_filter_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
          ospf_if_database_filter_apply (om, ifc->address->u.prefix4,
                                         ifp->ifindex);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_database_filter_unset_by_addr (u_int32_t vr_id, char *name,
                                       struct pal_in4_addr addr)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ospf_if_params_database_filter_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        ospf_if_database_filter_apply (om, addr, ifp->ifindex);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_disable_all_set (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_interface *oi;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);
  ospf_if_params_disable_all_set (oip);

  if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
    for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
      {
        oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                          ifc->ifp->ifindex);
        if (oi != NULL)
          {
#ifdef HAVE_OSPF_MULTI_INST
            if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
              {
                LIST_LOOP(oip->om->ospf, top_alt, node)
                  {
                    if (top_alt != oi->top)
                      {
                        /* Get oi from interface table of each instance */
                        oi_other = ospf_if_entry_match (top_alt, oi);
                        if (oi_other)
                          ospf_if_delete (oi_other);
                      }
                  }
              }
#endif /* HAVE_OSPF_MULTI_INST */
            ospf_if_delete (oi);
          }
      }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_if_disable_all_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct interface *ifp;
  struct connected *ifc;
  struct listnode *node;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_disable_all_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      if ((ifp = if_lookup_by_name (&om->vr->ifm, name)))
        for (node = LISTHEAD (om->ospf); node; NEXTNODE (node))
          {
            struct ospf *top = GETDATA (node);
            struct ospf_network *nw;
            struct ls_prefix p;

            for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
              {
                ls_prefix_ipv4_set (&p, ifc->address->prefixlen,
                                    ifc->address->u.prefix4);
                if ((nw = ospf_network_match (top, &p)))
                  ospf_network_apply (top, nw, ifc);
              }
          }
      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

#ifdef HAVE_RESTART
int
ospf_if_resync_timeout_set (u_int32_t vr_id, char *name, u_int32_t timeout)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_get_default (om, name);

  ret = ospf_if_params_resync_timeout_set (oip, timeout);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete_default (om, name);

  return ret;
}

int
ospf_if_resync_timeout_set_by_addr (u_int32_t vr_id, char *name,
                                    struct pal_in4_addr addr,
                                    u_int32_t timeout)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;
  struct ls_prefix p;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_get (om, name, &p);

  ret = ospf_if_params_resync_timeout_set (oip, timeout);
  if (OSPF_IF_PARAMS_EMPTY (oip))
    ospf_if_params_delete (om, name, &p);

  return ret;
}

int
ospf_if_resync_timeout_unset (u_int32_t vr_id, char *name)
{
  struct ospf_if_params *oip;
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  oip = ospf_if_params_lookup_default (om, name);
  if (oip)
    {
      ospf_if_params_resync_timeout_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete_default (om, name);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}

int
ospf_if_resync_timeout_unset_by_addr (u_int32_t vr_id, char *name,
                                      struct pal_in4_addr addr)
{
  struct ospf_master *om;
  struct ospf_if_params *oip;
  struct ls_prefix p;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);
  oip = ospf_if_params_lookup (om, name, &p);
  if (oip)
    {
      ospf_if_params_resync_timeout_unset (oip);
      if (OSPF_IF_PARAMS_EMPTY (oip))
        ospf_if_params_delete (om, name, &p);

      return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED;
}
#endif /* HAVE_RESTART */


int
ospf_vlink_set (u_int32_t vr_id, int proc_id, struct pal_in4_addr area_id,
                struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;
  area = ospf_area_entry_lookup (top, area_id);
  if (area != NULL)
    if (!IS_AREA_DEFAULT (area))
      return OSPF_API_SET_ERR_AREA_NOT_DEFAULT;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  if (!ospf_vlink_get (top, area_id, peer_id))
    return OSPF_API_SET_ERR_VLINK_CANT_GET;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_vlink_unset (u_int32_t vr_id, int proc_id, struct pal_in4_addr area_id,
                  struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

#ifdef HAVE_BFD
  ospf_if_bfd_unset (vr_id, vlink->name);
#endif /* HAVE_BFD */

  ospf_vlink_delete (top, vlink);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_vlink_format_set (u_int32_t vr_id, int proc_id,
                       struct pal_in4_addr area_id,
                       struct pal_in4_addr peer_id, u_char format)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  if (format != OSPF_AREA_ID_FORMAT_ADDRESS &&
      format != OSPF_AREA_ID_FORMAT_DECIMAL)
    return OSPF_API_SET_ERR_AREA_ID_FORMAT_INVALID;

  vlink->format = format;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_vlink_dead_interval_set (u_int32_t vr_id, int proc_id,
                              struct pal_in4_addr area_id,
                              struct pal_in4_addr peer_id, int interval)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (interval, PositiveInteger))
    return OSPF_API_SET_ERR_IF_DEAD_INTERVAL_INVALID;

  return ospf_if_dead_interval_set (vr_id, vlink->name, interval);
}

int
ospf_vlink_hello_interval_set (u_int32_t vr_id, int proc_id,
                               struct pal_in4_addr area_id,
                               struct pal_in4_addr peer_id, int interval)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (interval, HelloRange))
    return OSPF_API_SET_ERR_IF_HELLO_INTERVAL_INVALID;

  return ospf_if_hello_interval_set (vr_id, vlink->name, interval);
}

int
ospf_vlink_retransmit_interval_set (u_int32_t vr_id, int proc_id,
                                    struct pal_in4_addr area_id,
                                    struct pal_in4_addr peer_id, int interval)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (interval, UpToMaxAge))
    return OSPF_API_SET_ERR_IF_RETRANSMIT_INTERVAL_INVALID;

  return ospf_if_retransmit_interval_set (vr_id, vlink->name, interval);
}

int
ospf_vlink_transmit_delay_set (u_int32_t vr_id, int proc_id,
                               struct pal_in4_addr area_id,
                               struct pal_in4_addr peer_id, int delay)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (delay, UpToMaxAge))
    return OSPF_API_SET_ERR_IF_TRANSMIT_DELAY_INVALID;

  return ospf_if_transmit_delay_set (vr_id, vlink->name, delay);
}

int
ospf_vlink_dead_interval_unset (u_int32_t vr_id, int proc_id,
                                struct pal_in4_addr area_id,
                                struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_dead_interval_unset (vr_id, vlink->name);
}

int
ospf_vlink_hello_interval_unset (u_int32_t vr_id, int proc_id,
                                 struct pal_in4_addr area_id,
                                 struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_hello_interval_unset (vr_id, vlink->name);
}

int
ospf_vlink_retransmit_interval_unset (u_int32_t vr_id, int proc_id,
                                      struct pal_in4_addr area_id,
                                      struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_retransmit_interval_unset (vr_id, vlink->name);
}

int
ospf_vlink_transmit_delay_unset (u_int32_t vr_id, int proc_id,
                                 struct pal_in4_addr area_id,
                                 struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_transmit_delay_unset (vr_id, vlink->name);
}

int
ospf_vlink_authentication_type_set (u_int32_t vr_id, int proc_id,
                                    struct pal_in4_addr area_id,
                                    struct pal_in4_addr peer_id, int type)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (type, AuthType))
    return OSPF_API_SET_ERR_AUTH_TYPE_INVALID;

  return ospf_if_authentication_type_set (vr_id, vlink->name, type);
}

int
ospf_vlink_authentication_type_unset (u_int32_t vr_id, int proc_id,
                                      struct pal_in4_addr area_id,
                                      struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_authentication_type_unset (vr_id, vlink->name);
}

int
ospf_vlink_authentication_key_set (u_int32_t vr_id, int proc_id,
                                   struct pal_in4_addr area_id,
                                   struct pal_in4_addr peer_id, char *auth_key)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_authentication_key_set (vr_id, vlink->name, auth_key);
}

int
ospf_vlink_authentication_key_unset (u_int32_t vr_id, int proc_id,
                                     struct pal_in4_addr area_id,
                                     struct pal_in4_addr peer_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_authentication_key_unset (vr_id, vlink->name);
}

int
ospf_vlink_message_digest_key_set (u_int32_t vr_id, int proc_id,
                                   struct pal_in4_addr area_id,
                                   struct pal_in4_addr peer_id,
                                   u_char key_id, char *auth_key)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_message_digest_key_set (vr_id, vlink->name, key_id, auth_key);
}

int
ospf_vlink_message_digest_key_unset (u_int32_t vr_id, int proc_id,
                                     struct pal_in4_addr area_id,
                                     struct pal_in4_addr peer_id,
                                     u_char key_id)
{
  struct ospf_master *om;
  struct ospf_vlink *vlink;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  vlink = ospf_vlink_entry_lookup (top, area_id, peer_id);
  if (!vlink)
    return OSPF_API_SET_ERR_VLINK_NOT_EXIST;

  return ospf_if_message_digest_key_unset (vr_id, vlink->name, key_id);
}


int
ospf_area_auth_type_set (u_int32_t vr_id, int proc_id,
                         struct pal_in4_addr area_id, u_char type)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (type != OSPF_AUTH_NULL
      && type != OSPF_AUTH_SIMPLE
      && type != OSPF_AUTH_CRYPTOGRAPHIC)
    return OSPF_API_SET_ERR_AUTH_TYPE_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  if (ospf_area_auth_type_get (area) == type)
    return OSPF_API_SET_SUCCESS;

  if (type != OSPF_AREA_AUTH_NULL)
    OSPF_AREA_CONF_SET (area, AUTH_TYPE);
  else
    OSPF_AREA_CONF_UNSET (area, AUTH_TYPE);

  if (area)
    area->conf.auth_type = type;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_auth_type_unset (u_int32_t vr_id, int proc_id,
                           struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (ospf_area_auth_type_get (area) == OSPF_AUTH_NULL)
    return OSPF_API_SET_SUCCESS;

  OSPF_AREA_CONF_UNSET (area, AUTH_TYPE);
 
  if (area)
    area->conf.auth_type = OSPF_AREA_AUTH_NULL;

  return OSPF_API_SET_SUCCESS;
}


/* area stub related functions. */
int
ospf_area_stub_set (u_int32_t vr_id, int proc_id, struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  if (ospf_area_vlink_count (area))
    return OSPF_API_SET_ERR_AREA_HAS_VLINK;

  if (IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_IS_NSSA;

  else if (IS_AREA_DEFAULT (area))
    ospf_area_conf_external_routing_set (area, OSPF_AREA_STUB);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_stub_unset (u_int32_t vr_id, int proc_id,
                      struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_IS_NSSA;

  else if (IS_AREA_STUB (area))
    {
      OSPF_AREA_CONF_UNSET (area, NO_SUMMARY);
      pal_assert (area != NULL);
      ospf_area_conf_external_routing_set (area, OSPF_AREA_DEFAULT);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_no_summary_set (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (IS_AREA_DEFAULT (area))
    return OSPF_API_SET_ERR_AREA_IS_DEFAULT;

  if (!OSPF_AREA_CONF_CHECK (area, NO_SUMMARY))
    {
      OSPF_AREA_CONF_SET (area, NO_SUMMARY);

      /* Update the summary-LSAs.  */
      ospf_abr_ia_update_all_to_area (area);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_no_summary_unset (u_int32_t vr_id, int proc_id,
                            struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (IS_AREA_DEFAULT (area))
    return OSPF_API_SET_ERR_AREA_IS_DEFAULT;

  if (OSPF_AREA_CONF_CHECK (area, NO_SUMMARY))
    {
      OSPF_AREA_CONF_UNSET (area, NO_SUMMARY);

      /* Update the summary-LSAs.  */
      if (area != NULL)
        ospf_abr_ia_update_all_to_area (area);
    }

  return OSPF_API_SET_SUCCESS;
}

#ifdef HAVE_NSSA
int
ospf_area_nssa_set (u_int32_t vr_id, int proc_id, struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  if (ospf_area_vlink_count (area))
    return OSPF_API_SET_ERR_AREA_HAS_VLINK;

  if (IS_AREA_STUB (area))
    return OSPF_API_SET_ERR_AREA_IS_STUB;

  else if (IS_AREA_DEFAULT (area))
    ospf_area_conf_external_routing_set (area, OSPF_AREA_NSSA);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_unset (u_int32_t vr_id, int proc_id,
                      struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (IS_AREA_STUB (area))
    return OSPF_API_SET_ERR_AREA_IS_STUB;

  else if (IS_AREA_NSSA (area))
    {
      pal_assert (area != NULL);
      OSPF_AREA_CONF_UNSET (area, NO_SUMMARY);
      pal_assert (area != NULL);
      OSPF_AREA_CONF_UNSET (area, NSSA_TRANSLATOR);
      pal_assert (area != NULL);
      OSPF_AREA_CONF_UNSET (area, NO_REDISTRIBUTION);
      pal_assert (area != NULL);
      OSPF_AREA_CONF_UNSET (area, DEFAULT_ORIGINATE);
      pal_assert (area != NULL);
      OSPF_AREA_CONF_UNSET (area, METRIC);
      pal_assert (area != NULL);
      OSPF_AREA_CONF_UNSET (area, METRIC_TYPE);
      pal_assert (area != NULL);
      ospf_area_conf_external_routing_set (area, OSPF_AREA_DEFAULT);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_translator_role_set (u_int32_t vr_id, int proc_id,
                                    struct pal_in4_addr area_id, u_char role)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  if (ospf_nssa_translator_role_get (area) != role)
    {
      area->conf.nssa_translator_role = role;

      if (role != OSPF_NSSA_TRANSLATE_CANDIDATE)
        OSPF_AREA_CONF_SET (area, NSSA_TRANSLATOR);
      else
        OSPF_AREA_CONF_UNSET (area, NSSA_TRANSLATOR);

      if (area != NULL)
        ospf_abr_nssa_translator_state_update (area);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_translator_role_unset (u_int32_t vr_id, int proc_id,
                                      struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  if (ospf_nssa_translator_role_get (area) != OSPF_NSSA_TRANSLATE_CANDIDATE)
    {
      area->conf.nssa_translator_role = OSPF_NSSA_TRANSLATE_CANDIDATE;

      OSPF_AREA_CONF_UNSET (area, NSSA_TRANSLATOR);

      if (area != NULL)
        ospf_abr_nssa_translator_state_update (area);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_no_redistribution_set (u_int32_t vr_id, int proc_id,
                                      struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  ospf_redist_conf_nssa_no_redistribute_set (area);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_no_redistribution_unset (u_int32_t vr_id, int proc_id,
                                        struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  ospf_redist_conf_nssa_no_redistribute_unset (area);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_default_originate_set (u_int32_t vr_id, int proc_id,
                                      struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  ospf_redist_conf_nssa_default_set (area);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_default_originate_metric_set (u_int32_t vr_id, int proc_id,
                                             struct pal_in4_addr area_id,
                                             int metric)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (metric < 0 || metric > 16777214)
    return OSPF_API_SET_ERR_METRIC_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  ospf_redist_conf_nssa_default_metric_set (area, metric);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_default_originate_metric_type_set (u_int32_t vr_id, int proc_id,
                                                  struct pal_in4_addr area_id,
                                                  int mtype)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (mtype != EXTERNAL_METRIC_TYPE_1
      && mtype != EXTERNAL_METRIC_TYPE_2)
    return OSPF_API_SET_ERR_METRIC_TYPE_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  ospf_redist_conf_nssa_default_metric_type_set (area, mtype);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_nssa_default_originate_unset (u_int32_t vr_id, int proc_id,
                                        struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (!IS_AREA_NSSA (area))
    return OSPF_API_SET_ERR_AREA_NOT_NSSA;

  ospf_redist_conf_nssa_default_unset (area);

  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_NSSA */


/* area default-cost related function. */
int
ospf_area_default_cost_set (u_int32_t vr_id, int proc_id,
                            struct pal_in4_addr area_id, u_int32_t cost)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  if (cost > OSPF_STUB_DEFAULT_COST_MAX)
    return OSPF_API_SET_ERR_COST_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (!ospf_area_entry_lookup (top, area_id))
    return OSPF_API_SET_ERR_AREA_IS_DEFAULT;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  if (IS_AREA_DEFAULT (area))
    return OSPF_API_SET_ERR_AREA_IS_DEFAULT;

  /* Update the default cost.  */
  ospf_area_conf_default_cost_set (area, cost);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_default_cost_unset (u_int32_t vr_id, int proc_id,
                              struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (IS_AREA_DEFAULT (area))
    return OSPF_API_SET_ERR_AREA_IS_DEFAULT;

  /* Update the default cost.  */
  ospf_area_conf_default_cost_set (area, OSPF_STUB_DEFAULT_COST_DEFAULT);

  return OSPF_API_SET_SUCCESS;
}


/* Area range related functions. */
int
ospf_area_range_set (u_int32_t vr_id, int proc_id, struct pal_in4_addr area_id,
                     struct pal_in4_addr net, u_char masklen)
{
  struct ospf_area_range *range;
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;
  struct ls_prefix lp;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_prefix_ipv4_set (&lp, masklen, net);

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  range = ospf_area_range_lookup (area, &lp);
  if (range)
    {
      if (!CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
        if (range->status == ROW_STATUS_ACTIVE)
          OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);
    }
  else
    {
      range = ospf_area_range_new (area);

      ospf_area_range_add (area, range, &lp);

      OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);
    }

  SET_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_range_unset (u_int32_t vr_id, int proc_id,
                       struct pal_in4_addr area_id,
                       struct pal_in4_addr net, u_char masklen)
{
  struct ospf_area_range *range;
  struct ospf_master *om;
  struct ospf_area *area;
  struct ls_prefix lp;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  ls_prefix_ipv4_set (&lp, masklen, net);

  range = ospf_area_range_lookup (area, &lp);
  if (!range)
    return OSPF_API_SET_ERR_AREA_RANGE_NOT_EXIST;

  ospf_area_range_delete (area, range);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_range_not_advertise_set (u_int32_t vr_id, int proc_id,
                                   struct pal_in4_addr area_id,
                                   struct pal_in4_addr net, u_char masklen)
{
  struct ospf_area_range *range;
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;
  struct ls_prefix lp;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  ls_prefix_ipv4_set (&lp, masklen, net);

  range = ospf_area_range_lookup (area, &lp);
  if (!range)
    return OSPF_API_SET_ERR_AREA_RANGE_NOT_EXIST;

  if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
    if (range->status == ROW_STATUS_ACTIVE)
      OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);

  UNSET_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_range_not_advertise_unset (u_int32_t vr_id, int proc_id,
                                     struct pal_in4_addr area_id,
                                     struct pal_in4_addr net, u_char masklen)
{
  struct ospf_area_range *range;
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;
  struct ls_prefix lp;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  ls_prefix_ipv4_set (&lp, masklen, net);

  range = ospf_area_range_lookup (area, &lp);
  if (!range)
    return OSPF_API_SET_ERR_AREA_RANGE_NOT_EXIST;

  if (!CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
    if (range->status == ROW_STATUS_ACTIVE)
      OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);

  SET_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_range_substitute_set (u_int32_t vr_id, int proc_id,
                                struct pal_in4_addr area_id,
                                struct pal_in4_addr net, u_char masklen,
                                struct pal_in4_addr subst_net,
                                u_char subst_masklen)
{
  struct ospf_area_range *range;
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;
  struct ls_prefix lp;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_prefix_ipv4_set (&lp, masklen, net);

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  range = ospf_area_range_lookup (area, &lp);
  if (range)
    {
      if (!CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE)
          || !CHECK_FLAG (range->flags, OSPF_AREA_RANGE_SUBSTITUTE))
        if (range->status == ROW_STATUS_ACTIVE)
          OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);
    }
  else
    {
      range = ospf_area_range_new (area);

      ospf_area_range_add (area, range, &lp);

      OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);
    }

  SET_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE);
  SET_FLAG (range->flags, OSPF_AREA_RANGE_SUBSTITUTE);

  range->subst = ls_prefix_new (LS_PREFIX_MIN_SIZE);
  ls_prefix_ipv4_set (range->subst, subst_masklen, subst_net);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_range_substitute_unset (u_int32_t vr_id, int proc_id,
                                  struct pal_in4_addr area_id,
                                  struct pal_in4_addr net, u_char masklen)
{
  struct ospf_area_range *range;
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;
  struct ls_prefix lp;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  ls_prefix_ipv4_set (&lp, masklen, net);

  range = ospf_area_range_lookup (area, &lp);
  if (!range)
    return OSPF_API_SET_ERR_AREA_RANGE_NOT_EXIST;

  if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_SUBSTITUTE))
    if (range->status == ROW_STATUS_ACTIVE)
      OSPF_TIMER_ON (range->t_update, ospf_area_range_timer, range, 1);

  UNSET_FLAG (range->flags, OSPF_AREA_RANGE_SUBSTITUTE);

  if (range->subst)
    {
      ls_prefix_free (range->subst);
      range->subst = NULL;
    }

  return OSPF_API_SET_SUCCESS;
}


int
ospf_area_filter_list_prefix_set (u_int32_t vr_id, int proc_id,
                                  struct pal_in4_addr area_id,
                                  int type, char *name)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (type != FILTER_IN
      && type != FILTER_OUT)
    return OSPF_API_SET_ERR_INVALID_FILTER_TYPE;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  PLIST_LIST (area, type) = prefix_list_lookup (om->vr, AFI_IP, name);

  if (PLIST_NAME (area, type))
    XFREE (MTYPE_OSPF_DESC, PLIST_NAME (area, type));

  PLIST_NAME (area, type) = XSTRDUP (MTYPE_OSPF_DESC, name);

  if (type == FILTER_IN)
    OSPF_AREA_CONF_SET (area, PREFIX_LIST_IN);
  else /*  == FILTER_OUT */
    OSPF_AREA_CONF_SET (area, PREFIX_LIST_OUT);

  if (IS_AREA_ACTIVE (area))
    if (PLIST_LIST (area, type) != NULL)
      {
        if (type == FILTER_IN)
          ospf_abr_ia_update_all_to_area (area);
        else /*  == FILTER_OUT */
          ospf_abr_ia_update_area_to_all (area);
      }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_filter_list_prefix_unset (u_int32_t vr_id, int proc_id,
                                    struct pal_in4_addr area_id, int type)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (PLIST_NAME (area, type))
    {
      XFREE (MTYPE_OSPF_DESC, PLIST_NAME (area, type));
      PLIST_NAME (area, type) = NULL;
    }

  if (type == FILTER_IN)
    OSPF_AREA_CONF_UNSET (area, PREFIX_LIST_IN);
  else /*  == FILTER_OUT */
    OSPF_AREA_CONF_UNSET (area, PREFIX_LIST_OUT);

  if (area != NULL)
    if (IS_AREA_ACTIVE (area))
      if (PLIST_LIST (area, type) != NULL)
        {
          PLIST_LIST (area, type) = NULL;
          if (type == FILTER_IN)
            ospf_abr_ia_update_all_to_area (area);
          else /*  == FILTER_OUT */
            ospf_abr_ia_update_area_to_all (area);
        }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_filter_list_access_set (u_int32_t vr_id, int proc_id,
                                  struct pal_in4_addr area_id,
                                  int type, char *name)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  ALIST_LIST (area, type) = access_list_lookup (om->vr, AFI_IP, name);

  if (ALIST_NAME (area, type))
    XFREE (MTYPE_OSPF_DESC, ALIST_NAME (area, type));

  ALIST_NAME (area, type) = XSTRDUP (MTYPE_OSPF_DESC, name);

  if (type == FILTER_IN)
    OSPF_AREA_CONF_SET (area, ACCESS_LIST_IN);
  else /*  == FILTER_OUT */
    OSPF_AREA_CONF_SET (area, ACCESS_LIST_OUT);

  if (IS_AREA_ACTIVE (area))
    {
      if (type == FILTER_IN)
        ospf_abr_ia_update_all_to_area (area);
      else /*  == FILTER_OUT */
        ospf_abr_ia_update_area_to_all (area);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_filter_list_access_unset (u_int32_t vr_id, int proc_id,
                                    struct pal_in4_addr area_id, int type)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (ALIST_NAME (area, type))
    {
      XFREE (MTYPE_OSPF_DESC, ALIST_NAME (area, type));
      ALIST_NAME (area, type) = NULL;
    }

  if (type == FILTER_IN)
    OSPF_AREA_CONF_UNSET (area, ACCESS_LIST_IN);
  else /*  == FILTER_OUT */
    OSPF_AREA_CONF_UNSET (area, ACCESS_LIST_OUT);

  if (area != NULL)
    if (IS_AREA_ACTIVE (area))
      {
        ALIST_LIST (area, type) = NULL;
        if (type == FILTER_IN)
          ospf_abr_ia_update_all_to_area (area);
        else /*  == FILTER_OUT */
          ospf_abr_ia_update_area_to_all (area);
      }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_export_list_set (u_int32_t vr_id, int proc_id,
                           struct pal_in4_addr area_id, char *name)
{
  return ospf_area_filter_list_access_set (vr_id, proc_id,
                                           area_id, FILTER_OUT, name);
}

int
ospf_area_export_list_unset (u_int32_t vr_id, int proc_id,
                             struct pal_in4_addr area_id)
{
  return ospf_area_filter_list_access_unset (vr_id, proc_id,
                                             area_id, FILTER_OUT);
}

int
ospf_area_import_list_set (u_int32_t vr_id, int proc_id,
                           struct pal_in4_addr area_id, char *name)
{
  return ospf_area_filter_list_access_set (vr_id, proc_id,
                                           area_id, FILTER_IN, name);
}

int
ospf_area_import_list_unset (u_int32_t vr_id, int proc_id,
                             struct pal_in4_addr area_id)
{
  return ospf_area_filter_list_access_unset (vr_id, proc_id,
                                             area_id, FILTER_IN);
}

/* Shortcut ABR mode. */
int
ospf_area_shortcut_set (u_int32_t vr_id, int proc_id,
                        struct pal_in4_addr area_id, u_char type)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  if (ospf_area_shortcut_get (area) == type)
    return OSPF_API_SET_SUCCESS;

  area->conf.shortcut = type;
  if (type != OSPF_SHORTCUT_DEFAULT)
    OSPF_AREA_CONF_SET (area, SHORTCUT_ABR);
  else
    OSPF_AREA_CONF_UNSET (area, SHORTCUT_ABR);

  if (area != NULL)
    {
      LSA_REFRESH_TIMER_ADD (top, OSPF_ROUTER_LSA,
                             ospf_router_lsa_self (area), area);

      ospf_spf_calculate_timer_add (area);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_area_shortcut_unset (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr area_id)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (IS_AREA_ID_BACKBONE (area_id))
    return OSPF_API_SET_ERR_AREA_IS_BACKBONE;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  if (ospf_area_shortcut_get (area) == OSPF_SHORTCUT_DEFAULT)
    return OSPF_API_SET_SUCCESS;

  area->conf.shortcut = OSPF_SHORTCUT_DEFAULT;
  OSPF_AREA_CONF_UNSET (area, SHORTCUT_ABR);

  if (area != NULL)
    {
      LSA_REFRESH_TIMER_ADD (top, OSPF_ROUTER_LSA,
                             ospf_router_lsa_self (area), area);

      ospf_spf_calculate_timer_add (area);
    }

  return OSPF_API_SET_SUCCESS;
}
#ifdef HAVE_OSPF_MULTI_AREA
/* Set Multi area adjacency on specified interface */
s_int32_t
ospf_multi_area_adjacency_set (u_int32_t vr_id, int proc_id,
                               struct pal_in4_addr area_id, u_char *ifname,
                               struct pal_in4_addr nbr_addr, int format)
{
  struct ospf_master *om = NULL;
  struct ospf_area *area = NULL;
  struct ospf *top = NULL;
  s_int32_t ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;

  ret = ospf_area_format_set (vr_id, top->ospf_id, area_id, format);
  if (ret != OSPF_API_SET_SUCCESS)
    return ret;

  /* Get multi area link */
  if (!(ospf_multi_area_link_get (top, area_id, ifname, nbr_addr, format)))
    return OSPF_API_SET_ERR_MULTI_AREA_LINK_CANT_GET;

  return OSPF_API_SET_SUCCESS;
}

/* Unset multi area adjacency on specified interface */
s_int32_t
ospf_multi_area_adjacency_unset (u_int32_t vr_id, int proc_id,
                                 struct pal_in4_addr area_id, u_char *ifname,
                                 struct pal_in4_addr nbr_addr)
{
  struct ospf_master *om = NULL;
  struct ospf *top = NULL;
  struct ospf_multi_area_link *mlink = NULL;


  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

 /* Remove all the multi area adjacencies for area if neighbor not config */
  if (IPV4_ADDR_SAME (&nbr_addr, &IPv4AddressUnspec))
    ospf_multi_area_link_delete_all (top, area_id, ifname);
  else
    {
      /* Look for multi-area-adjacency for neighbor */
      mlink = ospf_multi_area_link_entry_lookup (top, area_id, nbr_addr);

      if (mlink == NULL)
        return OSPF_API_SET_ERR_MULTI_AREA_ADJ_NOT_SET;

      /* Delete multi-area-adjacency link for neighbor */
      ospf_multi_area_link_delete (top, mlink);
    }
  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_OSPF_MULTI_AREA */

#ifdef HAVE_OPAQUE_LSA
int
ospf_opaque_link_lsa_set (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr addr, u_char type,
                          u_int32_t id, u_char *data, u_int32_t len)
{
  struct ospf_master *om;
  struct ospf_interface *oi;
  struct ospf *top;
  struct pal_in4_addr ls_id;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  oi = ospf_if_entry_lookup (top, addr, 0);
  if (!oi)
    return OSPF_API_SET_ERR_IF_NOT_EXIST;

  ls_id.s_addr = MAKE_OPAQUE_ID (type, id);

  ospf_opaque_lsa_redistribute_link (oi, ls_id, data, len);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_opaque_area_lsa_set (u_int32_t vr_id, int proc_id,
                          struct pal_in4_addr area_id, u_char type,
                          u_int32_t id, u_char *data, u_int32_t len)
{
  struct ospf_master *om;
  struct ospf_area *area;
  struct ospf *top;
  struct pal_in4_addr ls_id;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  area = ospf_area_entry_lookup (top, area_id);
  if (!area)
    return OSPF_API_SET_ERR_AREA_NOT_EXIST;

  ls_id.s_addr = MAKE_OPAQUE_ID (type, id);

  ospf_opaque_lsa_redistribute_area (area, ls_id, data, len);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_opaque_as_lsa_set (u_int32_t vr_id, int proc_id, u_char type,
                        u_int32_t id, u_char *data, u_int32_t len)
{
  struct ospf_master *om;
  struct ospf *top;
  struct pal_in4_addr ls_id;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ls_id.s_addr = MAKE_OPAQUE_ID (type, id);

  ospf_opaque_lsa_redistribute_as (top, ls_id, data, len);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_capability_opaque_lsa_set (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (!IS_OPAQUE_CAPABLE (top))
    {
      SET_FLAG (top->config, OSPF_CONFIG_OPAQUE_LSA);

      /* Update the process.  */
      ospf_proc_update (top);
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_capability_opaque_lsa_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (IS_OPAQUE_CAPABLE (top))
    {
#ifdef HAVE_OSPF_TE
      /* Traffic-Engineering capability should be off.  */
      ospf_capability_traffic_engineering_unset (vr_id, proc_id);
#endif /* HAVE_OSPF_TE */

      UNSET_FLAG (top->config, OSPF_CONFIG_OPAQUE_LSA);

      /* Update the process.  */
      ospf_proc_update (top);
    }

  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_OSPF_TE
int
ospf_capability_traffic_engineering_set (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (!IS_TE_CAPABLE (top))
    {
      SET_FLAG (top->config, OSPF_CONFIG_TRAFFIC_ENGINEERING);

      /* Opaque capability is required.  */
      if (IS_OPAQUE_CAPABLE (top))
        ospf_proc_update (top);
      else
        {
          ret = ospf_capability_opaque_lsa_set (vr_id, proc_id);
          if (ret != OSPF_API_SET_SUCCESS)
            {
              UNSET_FLAG (top->config, OSPF_CONFIG_TRAFFIC_ENGINEERING);
              return ret;
            }
        }
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_capability_traffic_engineering_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (IS_TE_CAPABLE (top))
    {
      UNSET_FLAG (top->config, OSPF_CONFIG_TRAFFIC_ENGINEERING);

#ifdef HAVE_OSPF_CSPF
      /* Disable CSPF.  */
      ospf_capability_cspf_unset (vr_id, proc_id);
#endif /* HAVE_OSPF_CSPF */

      /* Update the process.  */
      ospf_proc_update (top);
    }

  return OSPF_API_SET_SUCCESS;
}

#ifdef HAVE_GMPLS
/** @brief Function to unset TE link flood scope

    @param vr_id           - VR identifier
    @param tlink_name      - te-link name
    @param area-id         - area into which te-links need to be flood

    @return
       SUCCESS/FAILURE of API
*/
int
ospf_te_link_flood_scope_set (u_int32_t vr_id, char *tlink_name,
                              int proc_id, struct pal_in4_addr area_id,
                              int format)  
{
  struct ospf *top;
  struct ospf_master *om;
  struct ls_node *rn1 = NULL;
  struct telink *tl = NULL;
  struct ospf_telink *os_tel = NULL;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  /* Get the ospf telink from telink name */ 
  tl = te_link_lookup_by_name (om->zg->gmif, tlink_name);
  if (tl)
    os_tel = (struct ospf_telink *)tl->info;

  if (!os_tel)
    return OSPF_API_SET_ERROR;

   /* If ospf telink flood scope flag set then return */
  if ((os_tel->params != NULL)
       && CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_AREA_ID))
    return OSPF_API_SET_ERR_TELINK_FLOOD_SCOPE_ENABLED;

  if (os_tel->params == NULL)
    os_tel->params = ospf_tlink_params_new ();
  
  os_tel->params->area_id = area_id;
  os_tel->params->format = format;
  os_tel->params->proc_id = proc_id;
  SET_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_AREA_ID);

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_SUCCESS;
  else
    {
      /* Set the area for the ospf_telink structure */
      rn1 = ospf_api_lookup (top->area_table, OSPF_AREA_TABLE,
                             &area_id);
      if (rn1)
        os_tel->area = RN_INFO (rn1, RNI_DEFAULT);
      else
        return OSPF_API_SET_SUCCESS;

      os_tel->top = top;
      /* Activate the ospf_telink */
      ospf_activate_te_link (os_tel);
    }

   return OSPF_API_SET_SUCCESS;
}

/** @brief Function to unset TE link flood scope

    @param vr_id           - VR identifier
    @param tlink_name      - te-link name
    @param area-id         - area into which te-links need to be flood

    @return
       SUCCESS/FAILURE of API
*/
int
ospf_te_link_flood_scope_unset (u_int32_t vr_id, char *tlink_name,
                                int proc_id, struct pal_in4_addr area_id)
{
  struct ospf *top;
  struct ospf_master *om;
  struct ls_node *rn = NULL;
  struct telink *tl = NULL;
  struct ospf_telink *os_tel = NULL;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  /* Get the ospf telink from telink name */ 
  tl = te_link_lookup_by_name (om->zg->gmif, tlink_name);
  if (tl)
    os_tel = (struct ospf_telink *)tl->info;

  if (!os_tel)
    return OSPF_API_SET_ERROR;

   /* If ospf telink flood scope flag set then return */
  if (!CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_AREA_ID))
    return OSPF_API_SET_ERR_TELINK_FLOOD_SCOPE_NOT_ENABLED;

  UNSET_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_AREA_ID);
  if (os_tel->params->config == 0)
    {
      XFREE (MTYPE_OSPF_TLINK_PARAMS, os_tel->params);
      os_tel->params = NULL;
    }

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_SUCCESS;
  else
    {
      /* Check for the area with area-id in ospf area table */
      rn = ospf_api_lookup (top->area_table, OSPF_AREA_TABLE,
                             &area_id);
      if (!rn)
        return OSPF_API_SET_SUCCESS;

      /* Unset the config flag and deactive the ospf_telink */
      ospf_deactivate_te_link (os_tel);
      os_tel->top = NULL;
      os_tel->area = NULL;
    }
  return OSPF_API_SET_SUCCESS;
}

/** @brief Function to set TE link metric

    @param vr_id           - VR identifier 
    @param tlink_name      - te-link name
    @param metric          - metric value to be set

    @return 
       SUCCESS/FAILURE of API  
*/
int
ospf_telink_te_metric_set (u_int32_t vr_id, char *tlink_name, 
                           u_int32_t metric)
{
  struct ospf_master *om;
  struct telink *tl;
  struct ospf_telink *os_tel = NULL;

  om = ospf_master_lookup_by_id (ZG, vr_id);

  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  /* Get the ospf telink from telink name */
  tl = te_link_lookup_by_name (om->zg->gmif, tlink_name);
  if (tl)
    os_tel = (struct ospf_telink *)tl->info;

  if (!os_tel)
    return OSPF_API_SET_ERROR;

  if ((os_tel->params != NULL)
       && CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_METRIC))
    {
      if (os_tel->params->te_metric == metric)
        return OSPF_API_SET_ERR_TELINK_METRIC_EXIST;
    }
  else if (os_tel->params == NULL) 
    os_tel->params = ospf_tlink_params_new ();

  /* Set the ospf_telink config flag */
  SET_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_METRIC);

  /* Set the metric value and update the te_link */
  os_tel->params->te_metric = metric;
  ospf_update_te_link (os_tel);

  return OSPF_API_SET_SUCCESS;
}

/** @brief Function to unset TE link metric value

    @param vr_id           - VR identifier
    @param tlink_name      - te-link name

    @return  
       SUCCESS/FAILURE of API  
*/
int
ospf_telink_te_metric_unset (u_int32_t vr_id, char *tlink_name)
{
  struct ospf_master *om;
  struct telink *tl;
  struct ospf_telink *os_tel = NULL;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  /* Get the ospf telink from telink name */
  tl = te_link_lookup_by_name (om->zg->gmif, tlink_name);
  
  if (tl)
    os_tel = (struct ospf_telink *)tl->info;

  if (!os_tel)
    return OSPF_API_SET_ERROR;

  if ((os_tel->params != NULL)
       && (!CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_METRIC)))
    return OSPF_API_SET_ERR_TELINK_METRIC_NOT_EXIST;
  else
    {
      /* Unset the ospf_telink config flag and
         set the metric to default value */
      UNSET_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_METRIC);
      os_tel->params->te_metric = 0;
      if (os_tel->params->config == 0)
        {
          XFREE (MTYPE_OSPF_TLINK_PARAMS, os_tel->params);
          os_tel->params = NULL;
        }
      ospf_update_te_link (os_tel);
    }

  return OSPF_API_SET_SUCCESS;
}

/** @brief Function to enable te-link-local LSA

    @param vr_id           - VR identifier
    @param tlink_name      - te-link name

    @return
       SUCCESS/FAILURE of API
*/
int
ospf_opaque_te_link_local_lsa_enable (u_int32_t vr_id, char *tlink_name)
{
  struct ospf_master *om;
  struct telink *tl;
  struct ospf_telink *os_tel = NULL;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  /* Get the ospf telink from telink name */
  tl = te_link_lookup_by_name (om->zg->gmif, tlink_name);
  if (tl)
    os_tel = (struct ospf_telink *)tl->info;

  if (!os_tel)
    return OSPF_API_SET_ERROR;

  if ((os_tel->params != NULL)
       && CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL))
    return OSPF_API_SET_ERR_TELINK_LOCAL_LSA_ENABLED;
  else if (os_tel->params == NULL)
    os_tel->params = ospf_tlink_params_new ();

  /* Set the ospf_telink config flag */
  SET_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL);

  /* Set the te link local LSA */
  ospf_te_link_local_lsa_set (os_tel);
 
  return OSPF_API_SET_SUCCESS;
}

/** @brief Function to disable te-link-local LSA

    @param vr_id           - VR identifier
    @param tlink_name      - te-link name

    @return
       SUCCESS/FAILURE of API
*/
int
ospf_opaque_te_link_local_lsa_disable (u_int32_t vr_id, char *tlink_name)
{
  struct ospf_master *om;
  struct telink *tl;
  struct ospf_telink *os_tel = NULL;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  /* Get the ospf telink from telink name */
  tl = te_link_lookup_by_name (om->zg->gmif, tlink_name);
  if (tl)
    os_tel = (struct ospf_telink *)tl->info;

  if (!os_tel)
    return OSPF_API_SET_ERROR;

  /* If the te_link_local flag not set then return error*/
  if ((os_tel->params != NULL) &&
      !CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL))
    return OSPF_API_SET_ERR_TELINK_LOCAL_LSA_NOT_ENABLED;
  else if (os_tel->params != NULL) 
    {
      /* Unset the ospf_telink config flag and
         unset the te_link_local lsa */
      UNSET_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL);
      if (os_tel->params->config == 0)
        {
          XFREE (MTYPE_OSPF_TLINK_PARAMS, os_tel->params);
          os_tel->params = NULL;
        }
      ospf_te_link_local_lsa_unset (os_tel);
    }

  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_GMPLS */
#endif /* HAVE_OSPF_TE */

#ifdef HAVE_OSPF_CSPF
int
ospf_capability_cspf_set (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;
  int ret;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Enable Traffic-Engineering first.  */
  if (!IS_TE_CAPABLE (top))
    if ((ret = ospf_capability_traffic_engineering_set (vr_id, proc_id)) < 0)
      return ret;

  if (om->cspf != NULL)
    {
      if (top->cspf == om->cspf)
        return OSPF_API_SET_SUCCESS;

      return OSPF_API_SET_ERR_CSPF_INSTANCE_EXIST;
    }

  top->cspf = om->cspf = cspf_server_init (om->zg, top);
  if (om->cspf == NULL)
    return OSPF_API_SET_ERR_CSPF_CANT_START;

  /* Set callback function. */
  top->cspf->lsp_compute = cspf_lsp_compute;

  /* Setting the address get function */
  top->cspf->lsp_get_addr = cspf_server_find_addr;

  /* Set debug flags. */
  om->cspf->cdf = &om->debug_cspf;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_capability_cspf_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (top->cspf == NULL)
    return OSPF_API_SET_ERR_CSPF_INSTANCE_NOT_EXIST;

  cspf_server_free (top->cspf);
  top->cspf = NULL;
  om->cspf = NULL;

  return OSPF_API_SET_SUCCESS;
}
#ifdef HAVE_GMPLS
int 
ospf_cspf_better_protection_type (u_int32_t vr_id, int proc_id, bool_t set)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (top->cspf == NULL)
    return OSPF_API_SET_ERR_CSPF_INSTANCE_NOT_EXIST;

  /* Enable better protection type */
  if (set == PAL_TRUE)
    {
      if (!CHECK_FLAG (top->cspf->config, CSPF_CONFIG_BETTER_PROTECTION))
        SET_FLAG (top->cspf->config, CSPF_CONFIG_BETTER_PROTECTION);
    }
  else  /* Disable better protection type */
    {
      if (CHECK_FLAG (top->cspf->config, CSPF_CONFIG_BETTER_PROTECTION))
        UNSET_FLAG (top->cspf->config, CSPF_CONFIG_BETTER_PROTECTION);
    }

  return OSPF_API_SET_SUCCESS;
}
#endif /* HAVE_GMPLS */
#endif /* HAVE_OSPF_CSPF */

int
ospf_enable_db_summary_opt (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  SET_FLAG (top->config, OSPF_CONFIG_DB_SUMMARY_OPT);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_disable_db_summary_opt (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  UNSET_FLAG (top->config, OSPF_CONFIG_DB_SUMMARY_OPT);

  return OSPF_API_SET_SUCCESS;
}


int
ospf_redistribute_set (u_int32_t vr_id, int proc_id,
                       int sproc_id, int type, int mtype, int mvalue)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (mtype != EXTERNAL_METRIC_TYPE_1
      && mtype != EXTERNAL_METRIC_TYPE_2
      && mtype != EXTERNAL_METRIC_TYPE_UNSPEC)
    return OSPF_API_SET_ERR_METRIC_TYPE_INVALID;

  if ((mvalue < 0 || mvalue > 16777214)
      && mvalue != EXTERNAL_METRIC_VALUE_UNSPEC)
    return OSPF_API_SET_ERR_METRIC_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (type == APN_ROUTE_OSPF)
    {
      if (!OSPF_API_CHECK_RANGE (sproc_id, PROCESS_ID))
        return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

      if (!ospf_proc_lookup (om, sproc_id))
        return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;
    }

  ospf_redist_conf_set (top, type, sproc_id);

  if (mtype != EXTERNAL_METRIC_TYPE_UNSPEC)
    ospf_redist_conf_metric_type_set (top, type, mtype, 0);

  if (mvalue != EXTERNAL_METRIC_VALUE_UNSPEC)
    ospf_redist_conf_metric_set (top, type, mvalue, 0);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redistribute_default_set (u_int32_t vr_id, int proc_id,
                               int origin, int mtype, int mvalue)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (origin != OSPF_DEFAULT_ORIGINATE_UNSPEC
      && origin != OSPF_DEFAULT_ORIGINATE_NSM
      && origin != OSPF_DEFAULT_ORIGINATE_ALWAYS)
    return OSPF_API_SET_ERR_DEFAULT_ORIGIN_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_default_set (top, origin);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_proto_set (u_int32_t vr_id, int proc_id, int type, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* If type is OSPF route then validate pid  */
  if (type == APN_ROUTE_OSPF)
    {
      if (!OSPF_API_CHECK_RANGE (sproc_id, PROCESS_ID))
        return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

      if (OSPF_API_CHECK_SELF_REDIST (proc_id, sproc_id))
        return OSPF_API_SET_ERR_SELF_REDIST ;
    }

  ospf_redist_conf_set (top, type, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_metric_set (u_int32_t vr_id, int proc_id, int type, int metric, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (metric < 0 || metric > 16777214)
    return OSPF_API_SET_ERR_METRIC_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_metric_set (top, type, metric, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_metric_type_set (u_int32_t vr_id, int proc_id, int type, int mtype, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (mtype != EXTERNAL_METRIC_TYPE_1
      && mtype != EXTERNAL_METRIC_TYPE_2)
    return OSPF_API_SET_ERR_METRIC_TYPE_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_metric_type_set (top, type, mtype, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_tag_set (u_int32_t vr_id, int proc_id, int type, u_int32_t tag, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_tag_set (top, type, tag, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_proto_unset (u_int32_t vr_id, int proc_id, int type, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (type == APN_ROUTE_OSPF)
    {
     if (!OSPF_API_CHECK_RANGE (sproc_id, PROCESS_ID))
       return OSPF_API_SET_ERR_PROCESS_ID_INVALID;
    }

  ospf_redist_conf_unset (top, type, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_default_set (u_int32_t vr_id, int proc_id, int origin)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (origin != OSPF_DEFAULT_ORIGINATE_UNSPEC
      && origin != OSPF_DEFAULT_ORIGINATE_NSM
      && origin != OSPF_DEFAULT_ORIGINATE_ALWAYS)
    return OSPF_API_SET_ERR_DEFAULT_ORIGIN_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_default_set (top, origin);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_default_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_default_unset (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_metric_unset (u_int32_t vr_id, int proc_id, int type, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (!REDIST_PROTO_CHECK (&top->redist[type]))
    return OSPF_API_SET_SUCCESS;

  ospf_redist_conf_metric_unset (top, type, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_metric_type_unset (u_int32_t vr_id, int proc_id, int type, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (!REDIST_PROTO_CHECK (&top->redist[type]))
    return OSPF_API_SET_SUCCESS;

  ospf_redist_conf_metric_type_unset (top, type, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_redist_tag_unset (u_int32_t vr_id, int proc_id, int type, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (!REDIST_PROTO_CHECK (&top->redist[type]))
    return OSPF_API_SET_SUCCESS;

  ospf_redist_conf_tag_unset (top, type, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distribute_list_out_set (u_int32_t vr_id, int proc_id,
                              int type, int sproc_id, char *name)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Avoiding configuration of distribute list for its own instance  */
  if ((type == APN_ROUTE_OSPF) && (top->ospf_id == sproc_id))
    return  OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  ospf_redist_conf_distribute_list_set (top, type, sproc_id, name);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distribute_list_in_set (u_int32_t vr_id, int proc_id,
                             char *name)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (og, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  SET_FLAG (top->dist_in.flags,OSPF_DIST_LIST_IN);

  /* Lookup access-list for distribute-list. */
  DIST_LIST (&top->dist_in) = access_list_lookup (om->vr, AFI_IP, name);

  /* Clear previous distribute-name. */
  if (DIST_NAME (&top->dist_in))
    XFREE (MTYPE_OSPF_DESC, DIST_NAME (&top->dist_in));

  /* Set distribute-name. */
  DIST_NAME (&top->dist_in) = XSTRDUP (MTYPE_OSPF_DESC, name);

  ospf_spf_calculate_timer_add_all (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distribute_list_out_unset (u_int32_t vr_id, int proc_id,
                                int type, int sproc_id, char *name)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_distribute_list_unset (top, type, sproc_id, name);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distribute_list_in_unset (u_int32_t vr_id, int proc_id,
                               char *name)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (og, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  UNSET_FLAG (top->dist_in.flags,OSPF_DIST_LIST_IN);

  /* Unset distribute-list. */
  DIST_LIST (&top->dist_in) = NULL;

  /* Clear distribute-name. */
  if (DIST_NAME (&top->dist_in))
    XFREE (MTYPE_OSPF_DESC, DIST_NAME (&top->dist_in));

  DIST_NAME (&top->dist_in) = NULL;

  ospf_spf_calculate_timer_add_all (top);

  return OSPF_API_SET_SUCCESS;
}


/* OSPF route-map set for redistribution */
int
ospf_routemap_set (u_int32_t vr_id, int proc_id, int type, char *name, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_route_map_set (top, type, name, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_routemap_unset (u_int32_t vr_id, int proc_id, int type, int sproc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (type < 0 || type >= APN_ROUTE_MAX)
    return OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  ospf_redist_conf_route_map_unset (top, type, sproc_id);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_routemap_default_set (u_int32_t vr_id, int proc_id, char *name)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (RMAP_NAME (&top->redist[APN_ROUTE_DEFAULT]))
    XFREE (MTYPE_OSPF_DESC, RMAP_NAME (&top->redist[APN_ROUTE_DEFAULT]));

  RMAP_NAME (&top->redist[APN_ROUTE_DEFAULT]) =
    XSTRDUP (MTYPE_OSPF_DESC, name);
  RMAP_MAP (&top->redist[APN_ROUTE_DEFAULT]) =
    route_map_lookup_by_name (om->vr, name);

  SET_FLAG (top->redist[APN_ROUTE_DEFAULT].flags, OSPF_REDIST_ROUTE_MAP);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_routemap_default_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (RMAP_NAME (&top->redist[APN_ROUTE_DEFAULT]))
    XFREE (MTYPE_OSPF_DESC, RMAP_NAME (&top->redist[APN_ROUTE_DEFAULT]));

  RMAP_NAME (&top->redist[APN_ROUTE_DEFAULT]) = NULL;
  RMAP_MAP (&top->redist[APN_ROUTE_DEFAULT]) = NULL;

  UNSET_FLAG (top->redist[APN_ROUTE_DEFAULT].flags, OSPF_REDIST_ROUTE_MAP);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_default_metric_set (u_int32_t vr_id, int proc_id, int metric)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (metric < 1 || metric > 16777214)
    return OSPF_API_SET_ERR_METRIC_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Set the default metric.  */
  ospf_redist_conf_default_metric_set (top, metric);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_default_metric_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Reset the default metric.  */
  ospf_redist_conf_default_metric_unset (top);

  return OSPF_API_SET_SUCCESS;
}


int
ospf_distance_all_set (u_int32_t vr_id, int proc_id, int distance)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (distance < 0 || distance > 255)
    return OSPF_API_SET_ERR_DISTANCE_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_all = distance;
  ospf_distance_update_all (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_all_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_all = 0;
  ospf_distance_update_all (top);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_intra_area_set (u_int32_t vr_id, int proc_id, int distance)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (distance < 0 || distance > 255)
    return OSPF_API_SET_ERR_DISTANCE_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_intra = distance;
  ospf_distance_update_by_type (top, OSPF_PATH_INTRA_AREA);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_intra_area_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_intra = 0;
  ospf_distance_update_by_type (top, OSPF_PATH_INTRA_AREA);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_inter_area_set (u_int32_t vr_id, int proc_id, int distance)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (distance < 0 || distance > 255)
    return OSPF_API_SET_ERR_DISTANCE_INVALID;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_inter = distance;
  ospf_distance_update_by_type (top, OSPF_PATH_INTER_AREA);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_inter_area_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_inter = 0;
  ospf_distance_update_by_type (top, OSPF_PATH_INTER_AREA);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_external_set (u_int32_t vr_id, int proc_id, int distance)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  if (distance < 0 || distance > 255)
    return OSPF_API_SET_ERR_DISTANCE_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_external = distance;
  ospf_distance_update_by_type (top, OSPF_PATH_EXTERNAL);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_external_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  top->distance_external = 0;
  ospf_distance_update_by_type (top, OSPF_PATH_EXTERNAL);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_source_set (u_int32_t vr_id, int proc_id,
                          int distance, struct pal_in4_addr addr,
                          u_char masklen, char *access_name)
{
  struct ospf_distance *odistance;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  /* Get OSPF distance. */
  odistance = ospf_distance_get (top, addr, masklen);
  odistance->distance = distance;

  /* Set access-list. */
  ospf_distance_access_list_set (odistance, access_name);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_distance_source_unset (u_int32_t vr_id, int proc_id,
                            int distance, struct pal_in4_addr addr,
                            u_char masklen, char *access_name)
{
  struct ospf_distance *odistance;
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (!OSPF_API_CHECK_RANGE (proc_id, PROCESS_ID))
    return OSPF_API_SET_ERR_PROCESS_ID_INVALID;

  top = ospf_proc_lookup (om, proc_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  odistance = ospf_distance_lookup (top, addr, masklen);
  if (!odistance)
    return OSPF_API_SET_ERR_DISTANCE_NOT_EXIST;

  ospf_distance_delete (top, addr, masklen);

  return OSPF_API_SET_SUCCESS;
}


#ifdef HAVE_RESTART
int
ospf_graceful_restart_set (struct ospf_master *om, int seconds, int reason)
{
  if (seconds < OSPF_GRACE_PERIOD_MIN || seconds > OSPF_GRACE_PERIOD_MAX)
    return OSPF_API_SET_ERR_GRACE_PERIOD_INVALID;

  if (reason != OSPF_RESTART_REASON_UNKNOWN
      && reason != OSPF_RESTART_REASON_RESTART
      && reason != OSPF_RESTART_REASON_UPGRADE
      && reason != OSPF_RESTART_REASON_SWITCH_REDUNDANT)
    return OSPF_API_SET_ERR_INVALID_REASON;

  SET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_GRACE_PERIOD);
  om->grace_period = seconds;
  om->restart_reason = reason;

  ospf_nsm_preserve_set (om);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_graceful_restart_unset (struct ospf_master *om)
{
  UNSET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_GRACE_PERIOD);
  om->grace_period = 0;

  ospf_nsm_preserve_unset (om);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_restart_helper_policy_set (u_int32_t vr_id, int policy)
{
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (policy != OSPF_RESTART_HELPER_NEVER
      && policy != OSPF_RESTART_HELPER_ONLY_RELOAD
      && policy != OSPF_RESTART_HELPER_ONLY_UPGRADE)
    return OSPF_API_SET_ERR_INVALID_HELPER_POLICY;

  SET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY);
  om->helper_policy = policy;

  /* If helper policy is OSPF_RESTART_HELPER_NEVER, remove all the
     router id's in the list    */
  if ((policy == OSPF_RESTART_HELPER_NEVER) && (listcount (om->never_restart_helper_list) > 0))
    list_delete_all_node (om->never_restart_helper_list);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_restart_helper_policy_unset (u_int32_t vr_id)
{
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (listcount(om->never_restart_helper_list) == 0)
    UNSET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY);

  om->helper_policy = OSPF_RESTART_HELPER_UNSPEC;
  return OSPF_API_SET_SUCCESS;
}

int
ospf_restart_helper_never_router_unset_all (u_int32_t vr_id)
{
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (listcount (om->never_restart_helper_list) == 0)
    return OSPF_API_SET_ERR_EMPTY_NEVER_RTR_ID;
  else
    list_delete_all_node (om->never_restart_helper_list);
  
  if (om->helper_policy == OSPF_RESTART_HELPER_UNSPEC)
    UNSET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY);
  return OSPF_API_SET_SUCCESS;
}

int
ospf_restart_helper_grace_period_set (u_int32_t vr_id, int seconds)
{
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (seconds < OSPF_GRACE_PERIOD_MIN || seconds > OSPF_GRACE_PERIOD_MAX)
    return OSPF_API_SET_ERR_GRACE_PERIOD_INVALID;

  if (om->helper_policy == OSPF_RESTART_HELPER_NEVER)
    {
      om->helper_policy = OSPF_RESTART_HELPER_UNSPEC;
      UNSET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY);
    }

  SET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_GRACE_PERIOD);
  om->max_grace_period = seconds;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_restart_helper_grace_period_unset (u_int32_t vr_id)
{
  struct ospf_master *om;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  UNSET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_GRACE_PERIOD);
  om->max_grace_period = 0;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_capability_restart_set (u_int32_t vr_id, int proc_id, int method)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  if (method != OSPF_RESTART_METHOD_GRACEFUL
      && method != OSPF_RESTART_METHOD_SIGNALING
      && method != OSPF_RESTART_METHOD_NEVER)
    return OSPF_API_SET_ERR_RESTART_METHOD_INVALID;

  if (method != OSPF_RESTART_METHOD_DEFAULT)
    SET_FLAG (top->config, OSPF_CONFIG_RESTART_METHOD);
  else
    UNSET_FLAG (top->config, OSPF_CONFIG_RESTART_METHOD);

  if (top->restart_method != method)
    {
      if (top->if_table->top != NULL)
        {
          if (CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
            ospf_restart_exit (top);

          ospf_restart_helper_exit_all (top);
        }

      top->restart_method = method;
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_capability_restart_unset (u_int32_t vr_id, int proc_id)
{
  struct ospf_master *om;
  struct ospf *top;

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  SET_FLAG (top->config, OSPF_CONFIG_RESTART_METHOD);
  if (top->restart_method != OSPF_RESTART_METHOD_NEVER)
    {
      if (top->if_table->top != NULL)
        {
          if (CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
            ospf_restart_exit (top);

          ospf_restart_helper_exit_all (top);
        }

      top->restart_method = OSPF_RESTART_METHOD_NEVER;
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_restart_helper_never_router_set (u_int32_t vr_id, 
                                      struct pal_in4_addr router_id)
{
  struct ospf_master *om;
  struct listnode *nn = NULL;
  struct pal_in4_addr *router_id_ptr = NULL; 

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  LIST_LOOP(om->never_restart_helper_list, router_id_ptr, nn)
    if (IPV4_ADDR_SAME (&router_id, router_id_ptr))
        /* Ignore if the IP Address is already added to the list */
        return OSPF_API_SET_ERR_IP_ADDR_IN_USE;

  router_id_ptr = XMALLOC (MTYPE_OSPF_RTR_ID, sizeof(struct pal_in4_addr));

  if (!router_id_ptr)
    return OSPF_API_SET_ERR_MALLOC_FAIL_FOR_ROUTERID;

    /* Copy the IP Address to store it in list */
  pal_mem_cpy (router_id_ptr, &router_id, sizeof(struct pal_in4_addr));

  SET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY);

  /* Add the router-id info to list */
  listnode_add (om->never_restart_helper_list, router_id_ptr);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_restart_helper_never_router_unset (u_int32_t vr_id, 
                                        struct pal_in4_addr router_id)
{
  struct ospf_master *om;
  struct listnode *nn = NULL;
  struct pal_in4_addr *router_id_ptr = NULL; 

  om = ospf_master_lookup_by_id (ZG, vr_id);
  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  LIST_LOOP(om->never_restart_helper_list, router_id_ptr, nn)
    {
      if (IPV4_ADDR_SAME (&router_id, router_id_ptr))
        {
          listnode_delete(om->never_restart_helper_list, router_id_ptr);
          XFREE (MTYPE_OSPF_RTR_ID, router_id_ptr);

          if ((listcount (om->never_restart_helper_list) == 0) 
             && (om->helper_policy == OSPF_RESTART_HELPER_UNSPEC)) 
            UNSET_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY);

          return OSPF_API_SET_SUCCESS;
        }
    }

  return OSPF_API_SET_ERR_NEVER_RTR_ID_NOT_EXIST;
}
                               
#endif /* HAVE_RESTART */

int
ospf_max_area_limit_set (struct ospf *top, u_int32_t num)
{
  top->max_area_limit = num;
  return OSPF_API_SET_SUCCESS;
}

int
ospf_max_area_limit_unset (struct ospf *top)
{
  top->max_area_limit = OSPF_AREA_LIMIT_DEFAULT;
  return OSPF_API_SET_SUCCESS;
}


/* SNMP-API functions. */

/* Get functions for ospfGeneralGroup. */
int
ospf_get_router_id (int proc_id, struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  *ret = top ? top->router_id : OSPFRouterIDUnspec;

  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_admin_stat (int proc_id, int *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;
  struct ls_node *rn;

  /* The administrative status of OSPF.
     When OSPF is enbled on at least one interface, return 1. */
  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
      if ((oi = RN_INFO (rn, RNI_DEFAULT)))
        if (oi->ident.address)
          {
            *ret = OSPF_API_STATUS_ENABLED;
            ls_unlock_node (rn);
            return OSPF_API_GET_SUCCESS;
          }

  *ret = OSPF_API_STATUS_DISABLED;
  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_version_number (int proc_id, int *ret, u_int32_t vr_id)
{
  *ret = OSPF_VERSION;
  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_area_bdr_rtr_status (int proc_id, int *ret, u_int32_t vr_id)
{
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top && CHECK_FLAG (top->flags, OSPF_ROUTER_ABR))
    *ret = OSPF_API_TRUE;
  else
    *ret = OSPF_API_FALSE;

  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_asbdr_rtr_status (int proc_id, int *ret, u_int32_t vr_id)
{
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top && IS_OSPF_ASBR (top))
    *ret = OSPF_API_TRUE;
  else
    *ret = OSPF_API_FALSE;

  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_extern_lsa_count (int proc_id, int *ret, u_int32_t vr_id)
{
  struct ospf *top = ospf_proc_lookup_any (proc_id, vr_id);

  *ret = top ? ospf_lsdb_count_all (top->lsdb) : 0;

  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_extern_lsa_cksum_sum (int proc_id, int *ret, u_int32_t vr_id)
{
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      *ret = ospf_lsdb_checksum (top->lsdb, OSPF_AS_EXTERNAL_LSA);
      return OSPF_API_GET_SUCCESS;
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_tos_support (int proc_id, int *ret, u_int32_t vr_id)
{
  /* TOS is not supported. */
  *ret = OSPF_API_FALSE;
  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_originate_new_lsas (int proc_id, int *ret, u_int32_t vr_id)
{
  struct ospf *top = ospf_proc_lookup_any (proc_id, vr_id);

  *ret = top ? top->lsa_originate_count : 0;

  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_rx_new_lsas (int proc_id, int *ret, u_int32_t vr_id)
{
  struct ospf *top = ospf_proc_lookup_any (proc_id, vr_id);

  /* The number of link-state advertisements received
     determined to be new instantiations. */
  *ret = top ? top->rx_lsa_count : 0;

  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_ext_lsdb_limit (int proc_id, int *ret, u_int32_t vr_id)
{
#ifdef HAVE_OSPF_DB_OVERFLOW
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      *ret = top->ext_lsdb_limit;
      return OSPF_API_GET_SUCCESS;
    }
#endif /* HAVE_OSPF_DB_OVERFLOW */
  *ret = -1;
  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_multicast_extensions (int proc_id, int *ret, u_int32_t vr_id)
{
  /* Multicast Extensions to OSPF is not supported. */
  *ret = 0;
  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_exit_overflow_interval (int proc_id, int *ret, u_int32_t vr_id)
{
#ifdef HAVE_OSPF_DB_OVERFLOW
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      *ret = top->exit_overflow_interval;
      return OSPF_API_GET_SUCCESS;
    }
#endif /* HAVE_OSPF_DB_OVERFLOW */
  *ret = 0;
  return OSPF_API_GET_SUCCESS;
}

int
ospf_get_demand_extensions (int proc_id, int *ret, u_int32_t vr_id)
{
  /* Demand routing is not supported. */
  *ret = OSPF_API_FALSE;
  return OSPF_API_GET_SUCCESS;
}

/* Set functions for ospfGeneralGroup. */
int
ospf_set_router_id (int proc_id, struct pal_in4_addr addr, u_int32_t vr_id)
{
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  return ospf_router_id_set (vr_id, top->ospf_id, addr);
}

int
ospf_set_admin_stat (int proc_id, int status, u_int32_t vr_id)
{
  struct ospf_master *om = ospf_master_lookup_default (ZG);
  struct listnode *node;
  int ret;

  if (om == NULL)
    return OSPF_API_SET_ERR_VR_NOT_EXIST;

  ospf_get_admin_stat (proc_id, &ret, vr_id);
  if (ret == OSPF_API_STATUS_DISABLED)
    return OSPF_API_SET_ERR_NETWORK_NOT_EXIST;

  if (status == OSPF_API_STATUS_ENABLED)
    {
      if (proc_id == OSPF_PROCESS_ID_ANY)
        return ospf_process_set (vr_id, OSPF_PROCESS_ID_SINGLE);
      else
        return ospf_process_set (vr_id, proc_id);
    }
  else if (status == OSPF_API_STATUS_DISABLED)
    {
      if (proc_id == OSPF_PROCESS_ID_ANY)
        {
          for (node = LISTHEAD (om->ospf); node;)
            {
              struct ospf *top = GETDATA (node);
              NEXTNODE (node);
              if (!top)
                continue;

              ospf_process_unset (vr_id, top->ospf_id);
            }
          ret =  OSPF_API_SET_SUCCESS;
        }
      else
        ret = ospf_process_unset (vr_id, proc_id);

      return ret;
    }

  return OSPF_API_SET_ERR_WRONG_VALUE;
}

int
ospf_set_asbdr_rtr_status (int proc_id, int status, u_int32_t vr_id)
{
  struct ospf *top;

  if (status != OSPF_API_TRUE
      && status != OSPF_API_FALSE)
    return OSPF_API_SET_ERR_WRONG_VALUE;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top == NULL)
    return OSPF_API_SET_ERROR;

  if (status == OSPF_API_TRUE)
    {
      if (!IS_OSPF_ASBR (top))
        ospf_asbr_status_update (top);

      /* Checking whether Non-ASBR status has updated to ASBR. */
      if (IS_OSPF_ASBR (top))
        return OSPF_API_SET_SUCCESS;
    }
  else if (status == OSPF_API_FALSE)
    {
      if (IS_OSPF_ASBR (top))
        ospf_asbr_status_update (top);
  
      /* Checking whether ASBR status has updated to Non-ASBR. */    
      if (!IS_OSPF_ASBR (top))
        return OSPF_API_SET_SUCCESS;
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_tos_support (int proc_id, int status, u_int32_t vr_id)
{
  /* TOS is not supported, so it can't be set. */
  return OSPF_API_SET_ERROR;
}

#define OSPF_API_ExtLsdbLimit_MIN -1
#define OSPF_API_ExtLsdbLimit_MAX 0x7FFFFFFF
int
ospf_set_ext_lsdb_limit (int proc_id, int limit, u_int32_t vr_id)
{
#ifdef HAVE_OSPF_DB_OVERFLOW
  struct ospf *top;

  if (!OSPF_API_CHECK_RANGE (limit, ExtLsdbLimit))
    return OSPF_API_SET_ERR_EXTERNAL_LSDB_LIMIT_INVALID;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  return ospf_overflow_database_external_limit_set (vr_id, top->ospf_id,
                                                    limit);
#else /* HAVE_OSPF_DB_OVERFLOW */
  return OSPF_API_SET_ERROR;
#endif /* HAVE_OSPF_DB_OVERFLOW */
}

int
ospf_set_multicast_extensions (int proc_id, int status, u_int32_t vr_id)
{
  /* Multicast Extensions is not supported, so it can't be set. */
  return OSPF_API_SET_ERROR;
}

int
ospf_set_exit_overflow_interval (int proc_id, int interval, u_int32_t vr_id)
{
#ifdef HAVE_OSPF_DB_OVERFLOW
  struct ospf *top;

  if (!OSPF_API_CHECK_RANGE (interval, PositiveInteger))
    return OSPF_API_SET_ERR_OVERFLOW_INTERVAL_INVALID;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (!top)
    return OSPF_API_SET_ERR_PROCESS_NOT_EXIST;

  return ospf_overflow_database_external_interval_set (vr_id, top->ospf_id,
                                                       interval);
#endif /* HAVE_OSPF_DB_OVERFLOW */
  return OSPF_API_SET_ERROR;
}

int
ospf_set_lsdb_limit (struct ospf *top, u_int32_t limit, int type, int set)
{
  if (set)
    {
      if (!CHECK_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT))
        SET_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT);

      top->lsdb_overflow_limit_type = type;
      top->lsdb_overflow_limit = limit;
    }
  else
    {
      if (CHECK_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT))
        UNSET_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT);

      if (CHECK_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT))
        UNSET_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT);

      top->lsdb_overflow_limit_type = 0;
      top->lsdb_overflow_limit = 0;
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_demand_extensions (int proc_id, int status, u_int32_t vr_id)
{
  /* Demand Extensions is not supported, so it can't be set. */
  return OSPF_API_SET_ERROR;
}


/* Get functions for ospfAreaEntry . */
int
ospf_get_area_id (int proc_id,
                  struct pal_in4_addr area_id,
                  struct pal_in4_addr *ret,
                  u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          *ret = area->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_auth_type (int proc_id, struct pal_in4_addr area_id,
                    int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          *ret = ospf_area_auth_type_get (area);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

#define OSPF_IMPORT_EXTERNAL    1
#define OSPF_IMPORT_NO_EXTERNAL 2
#define OSPF_IMPORT_NSSA        3
int
ospf_get_import_as_extern (int proc_id, struct pal_in4_addr area_id,
                           int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          if (IS_AREA_DEFAULT (area))
            *ret = OSPF_IMPORT_EXTERNAL;
          else if (IS_AREA_STUB (area))
            *ret = OSPF_IMPORT_NO_EXTERNAL;
#ifdef HAVE_NSSA
          else if (IS_AREA_NSSA (area))
            *ret = OSPF_IMPORT_NSSA;
#endif /* HAVE_NSSA */
          else
            return OSPF_API_GET_ERROR;

          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_spf_runs (int proc_id, struct pal_in4_addr area_id,
                   int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          *ret = area->spf_calc_count;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_bdr_rtr_count (int proc_id,
                             struct pal_in4_addr area_id,
                             int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          if (IS_OSPF_ABR (area->top))
            *ret = area->abr_count + 1;
          else
            *ret = area->abr_count;

          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_asbdr_rtr_count (int proc_id, struct pal_in4_addr area_id,
                          int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          if (IS_OSPF_ASBR (area->top))
            *ret = area->asbr_count + 1;
          else
            *ret = area->asbr_count;

          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_lsa_count (int proc_id, struct pal_in4_addr area_id,
                         int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          *ret = ospf_lsdb_count_all (area->lsdb);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_lsa_cksum_sum (int proc_id,
                             struct pal_in4_addr area_id, int *ret,
                             u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          *ret = ospf_lsdb_checksum_all (area->lsdb);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

#define OSPF_noAreaSummary   1
#define OSPF_sendAreaSummary 2
int
ospf_get_area_summary (int proc_id, struct pal_in4_addr area_id, int *ret,
                       u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          *ret = OSPF_AREA_CONF_CHECK (area, NO_SUMMARY) ?
            OSPF_noAreaSummary : OSPF_sendAreaSummary;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_status (int proc_id, struct pal_in4_addr area_id, int *ret,
                      u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area != NULL)
        {
          *ret = area->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfAreaEntry . */
int
ospf_get_next_area_id (int proc_id, struct pal_in4_addr *area_id,
                       int indexlen, struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          *ret = area->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_auth_type (int proc_id,
                         struct pal_in4_addr *area_id,
                         int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          *ret = ospf_area_auth_type_get (area);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_import_as_extern (int proc_id,
                                struct pal_in4_addr *area_id,
                                int indexlen, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          if (IS_AREA_DEFAULT (area))
            *ret = OSPF_IMPORT_EXTERNAL;
          else if (IS_AREA_STUB (area))
            *ret = OSPF_IMPORT_NO_EXTERNAL;
#ifdef HAVE_NSSA
          else if (IS_AREA_NSSA (area))
            *ret = OSPF_IMPORT_NSSA;
#endif /* HAVE_NSSA */
          else
            return OSPF_API_GET_ERROR;

          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_spf_runs (int proc_id,
                        struct pal_in4_addr *area_id,
                        int indexlen, int *ret,
                        u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          *ret = area->spf_calc_count;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_bdr_rtr_count (int proc_id,
                                  struct pal_in4_addr *area_id,
                                  int indexlen, int *ret,
                                  u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          if (IS_OSPF_ABR (area->top))
            *ret = area->abr_count + 1;
          else
            *ret = area->abr_count;

          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_asbdr_rtr_count (int proc_id,
                               struct pal_in4_addr *area_id,
                               int indexlen, int *ret,
                               u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          if (IS_OSPF_ASBR (area->top))
            *ret = area->asbr_count + 1;
          else
            *ret = area->asbr_count;

          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_lsa_count (int proc_id,
                              struct pal_in4_addr *area_id,
                              int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          *ret = ospf_lsdb_count_all (area->lsdb);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_lsa_cksum_sum (int proc_id,
                                  struct pal_in4_addr *area_id,
                                  int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          *ret = ospf_lsdb_checksum_all (area->lsdb);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_summary (int proc_id,
                            struct pal_in4_addr *area_id,
                            int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          *ret = OSPF_AREA_CONF_CHECK (area, NO_SUMMARY) ?
            OSPF_noAreaSummary : OSPF_sendAreaSummary;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_status (int proc_id,
                           struct pal_in4_addr *area_id,
                           int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup_next (top, area_id, indexlen);
      if (area)
        {
          *ret = area->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* Set functions for ospfAreaEntry. */
int
ospf_set_auth_type (int proc_id, struct pal_in4_addr area_id,
                    int type, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        return ospf_area_auth_type_set (vr_id, top->ospf_id,
                                        area_id, type);
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_import_as_extern (int proc_id, struct pal_in4_addr area_id,
                           int type, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      if (area)
        {
          switch (type)
            {
            case OSPF_IMPORT_EXTERNAL:
              if (IS_AREA_STUB (area))
                return ospf_area_stub_unset (vr_id, top->ospf_id, area_id);
#ifdef HAVE_NSSA
              else if (IS_AREA_NSSA (area))
                return ospf_area_nssa_unset (vr_id, top->ospf_id, area_id);
#endif /* HAVE_NSSA */
              break;
            case OSPF_IMPORT_NO_EXTERNAL:
              return ospf_area_stub_set (vr_id, top->ospf_id, area_id);
              break;
#ifdef HAVE_NSSA
            case OSPF_IMPORT_NSSA:
              return ospf_area_nssa_set (vr_id, top->ospf_id, area_id);
              break;
#endif /* HAVE_NSSA */
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
              break;
            }
          return OSPF_API_SET_SUCCESS;
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_area_summary (int proc_id, struct pal_in4_addr area_id, int val,
                       u_int32_t vr_id)
{
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      if (val == OSPF_noAreaSummary)
        return ospf_area_no_summary_set (vr_id, top->ospf_id, area_id);
      else if (val == OSPF_sendAreaSummary)
        return ospf_area_no_summary_unset (vr_id, top->ospf_id, area_id);
      else
        return OSPF_API_SET_ERR_WRONG_VALUE;
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_area_status (int proc_id, struct pal_in4_addr area_id,
                      int status, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      /* Row exists. */
      if (area)
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
              if (area->status != ROW_STATUS_ACTIVE)
                return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
              break;
            case ROW_STATUS_CREATEANDGO:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_DESTROY:
              if (!list_isempty (area->iflist)
                  || ospf_area_vlink_count (area) != 0
                  || ospf_network_area_match (area)
                  || area->conf.config != OSPF_AREA_CONF_SNMP_CREATE)
                return OSPF_API_SET_ERROR;
              else
                OSPF_AREA_CONF_UNSET2 (area, SNMP_CREATE);
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }

          return OSPF_API_SET_SUCCESS;
        }
      /* Row doesn't exists. */
      else
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
            case ROW_STATUS_NOTINSERVICE:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDWAIT:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            case ROW_STATUS_DESTROY:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDGO:
              area = ospf_area_get (top, area_id);
              if (area)
                OSPF_AREA_CONF_SET (area, SNMP_CREATE);
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }
        }
    }
  return OSPF_API_SET_SUCCESS;
}


/* Get functions for ospfStubAreaEntry. */
int
ospf_get_stub_area_id (int proc_id, struct pal_in4_addr area_id,
                       int tos, struct pal_in4_addr *ret,
                       u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area)
        {
          *ret = area->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_stub_tos (int proc_id, struct pal_in4_addr area_id, int tos, int *ret,
                   u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area)
        {
          *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_stub_metric (int proc_id,
                      struct pal_in4_addr area_id,
                      int tos, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area)
        {
          *ret = OSPF_AREA_DEFAULT_COST (area);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_stub_status (int proc_id,
                      struct pal_in4_addr area_id,
                      int tos, int *ret,
                      u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area != NULL)
        {
          *ret = area->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

#define OSPF_API_ospfMetric     1
#define OSPF_API_comparableCost 2
#define OSPF_API_nonComparable  3
int
ospf_get_stub_metric_type (int proc_id,
                           struct pal_in4_addr area_id,
                           int tos, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area)
        {
          if (tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = OSPF_API_ospfMetric;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfStubAreaEntry. */
int
ospf_get_next_stub_area_id (int proc_id, struct pal_in4_addr *area_id,
                            int *tos, int indexlen, struct pal_in4_addr *ret,
                            u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup_next (top, area_id, tos, indexlen);
      if (area)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = area->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_stub_tos (int proc_id, struct pal_in4_addr *area_id,
                        int *tos, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup_next (top, area_id, tos, indexlen);
      if (area)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_stub_metric (int proc_id, struct pal_in4_addr *area_id,
                           int *tos, int indexlen, int *ret,
                           u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup_next (top, area_id, tos, indexlen);
      if (area)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = OSPF_AREA_DEFAULT_COST (area);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_stub_status (int proc_id, struct pal_in4_addr *area_id,
                           int *tos, int indexlen, int *ret,
                           u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup_next (top, area_id, tos, indexlen);
      if (area)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = area->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_stub_metric_type (int proc_id, struct pal_in4_addr *area_id,
                                int *tos, int indexlen, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup_next (top, area_id, tos, indexlen);
      if (area)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = OSPF_API_ospfMetric;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* Set functions for ospfStubAreaEntry. */
int
ospf_set_stub_metric (int proc_id,
                      struct pal_in4_addr area_id,
                      int tos, int metric, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area)
        return ospf_area_default_cost_set (vr->id, top->ospf_id,
                                           area_id, metric);
    }

  return OSPF_API_SET_ERROR;
}

int
ospf_set_stub_status (int proc_id,
                      struct pal_in4_addr area_id,
                      int tos, int status, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  if (tos != 0)
    return OSPF_API_SET_ERROR;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area)
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
              if (area->status != ROW_STATUS_ACTIVE)
                return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
              break;
            case ROW_STATUS_CREATEANDGO:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_DESTROY:
              return ospf_area_stub_unset (vr_id, top->ospf_id, area->area_id);
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }

          return OSPF_API_SET_SUCCESS;
        }
      /* Row doesn't exists. */
      else
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
            case ROW_STATUS_NOTINSERVICE:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDWAIT:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            case ROW_STATUS_DESTROY:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDGO:
              ospf_area_stub_set (vr_id, top->ospf_id, area_id);
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }
        }
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_stub_metric_type (int proc_id,
                           struct pal_in4_addr area_id,
                           int tos, int type, u_int32_t vr_id)
{
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_stub_area_entry_lookup (top, area_id, tos);
      if (area)
        {
          /* We only support ospfMetric.  */
          if (type == OSPF_API_ospfMetric)
            return OSPF_API_SET_SUCCESS;
          else if (type == OSPF_API_comparableCost)
            return OSPF_API_SET_ERROR;
          else if (type == OSPF_API_nonComparable)
            return OSPF_API_SET_ERROR;
          else
            return OSPF_API_SET_ERR_WRONG_VALUE;
        }
    }
  return OSPF_API_SET_ERROR;
}


/* Get functions for ospfLsdbEntry. */
int
ospf_get_lsdb_area_id (int proc_id, struct pal_in4_addr area_id,
                       int type, struct pal_in4_addr lsid,
                       struct pal_in4_addr router_id, struct pal_in4_addr *ret,
                       u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          if (LSA_FLOOD_SCOPE (lsa->data->type) == LSA_FLOOD_SCOPE_AREA)
            *ret = lsa->lsdb->u.area->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_lsdb_type (int proc_id, struct pal_in4_addr area_id,
                    int type, struct pal_in4_addr lsid,
                    struct pal_in4_addr router_id, int *ret,
                    u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          *ret = lsa->data->type;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_lsdb_lsid (int proc_id, struct pal_in4_addr area_id,
                    int type, struct pal_in4_addr lsid,
                    struct pal_in4_addr router_id, struct pal_in4_addr *ret,
                    u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          *ret = lsa->data->id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_lsdb_router_id (int proc_id, struct pal_in4_addr area_id,
                         int type, struct pal_in4_addr lsid,
                         struct pal_in4_addr router_id,
                         struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          *ret = lsa->data->adv_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_lsdb_sequence (int proc_id, struct pal_in4_addr area_id,
                        int type, struct pal_in4_addr lsid,
                        struct pal_in4_addr router_id, int *ret,
                        u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          *ret = pal_ntoh32 (lsa->data->ls_seqnum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_lsdb_age (int proc_id, struct pal_in4_addr area_id,
                   int type, struct pal_in4_addr lsid,
                   struct pal_in4_addr router_id, int *ret,
                   u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          *ret = LS_AGE (lsa);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_lsdb_checksum (int proc_id, struct pal_in4_addr area_id,
                        int type, struct pal_in4_addr lsid,
                        struct pal_in4_addr router_id, int *ret,
                        u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          *ret = pal_ntoh16 (lsa->data->checksum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_lsdb_advertisement (int proc_id, struct pal_in4_addr area_id,
                             int type, struct pal_in4_addr lsid,
                             struct pal_in4_addr router_id,
                             u_char **ret, size_t *size, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup (top, area_id, type, lsid, router_id);
      if (lsa)
        {
          *ret = (u_char *)lsa->data;
          *size = pal_ntoh16 (lsa->data->length);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfLsdbEntry. */
int
ospf_get_next_lsdb_area_id (int proc_id, struct pal_in4_addr *area_id,
                            int *type, struct pal_in4_addr *lsid,
                            struct pal_in4_addr *router_id,
                            int indexlen, struct pal_in4_addr *ret,
                            u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          if (LSA_FLOOD_SCOPE (lsa->data->type) == LSA_FLOOD_SCOPE_AREA)
            *ret = lsa->lsdb->u.area->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_lsdb_type (int proc_id, struct pal_in4_addr *area_id, int *type,
                         struct pal_in4_addr *lsid,
                         struct pal_in4_addr *router_id,
                         int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          *ret = lsa->data->type;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_lsdb_lsid (int proc_id, struct pal_in4_addr *area_id,
                         int *type, struct pal_in4_addr *lsid,
                         struct pal_in4_addr *router_id,
                         int indexlen, struct pal_in4_addr *ret,
                         u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          *ret = lsa->data->id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_lsdb_router_id (int proc_id, struct pal_in4_addr *area_id,
                              int *type, struct pal_in4_addr *lsid,
                              struct pal_in4_addr *router_id,
                              int indexlen, struct pal_in4_addr *ret,
                              u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          *ret = lsa->data->adv_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_lsdb_sequence (int proc_id, struct pal_in4_addr *area_id,
                             int *type, struct pal_in4_addr *lsid,
                             struct pal_in4_addr *router_id,
                             int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          *ret = pal_ntoh32 (lsa->data->ls_seqnum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_lsdb_age (int proc_id, struct pal_in4_addr *area_id, int *type,
                        struct pal_in4_addr *lsid,
                        struct pal_in4_addr *router_id,
                        int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          *ret = LS_AGE (lsa);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_lsdb_checksum (int proc_id, struct pal_in4_addr *area_id,
                             int *type, struct pal_in4_addr *lsid,
                             struct pal_in4_addr *router_id,
                             int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          *ret = pal_ntoh16 (lsa->data->checksum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_lsdb_advertisement (int proc_id,
                                  struct pal_in4_addr *area_id, int *type,
                                  struct pal_in4_addr *lsid,
                                  struct pal_in4_addr *router_id,
                                  int indexlen, u_char **ret, size_t *size,
                                  u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_lsdb_entry_lookup_next (top, area_id, type, lsid, router_id,
                                         indexlen);
      if (lsa)
        {
          *ret = (u_char *)lsa->data;
          *size = pal_ntoh16 (lsa->data->length);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}


/* Get functions for ospAreaRangeEntry. */
int
ospf_get_area_range_area_id (int proc_id, struct pal_in4_addr area_id,
                             struct pal_in4_addr net, struct pal_in4_addr *ret,
                             u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (range)
        {
          *ret = area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_range_net (int proc_id, struct pal_in4_addr area_id,
                         struct pal_in4_addr net, struct pal_in4_addr *ret,
                         u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (range)
        {
          pal_mem_cpy (ret, range->lp->prefix, sizeof (struct pal_in4_addr));
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_range_mask (int proc_id, struct pal_in4_addr area_id,
                          struct pal_in4_addr net, struct pal_in4_addr *ret,
                          u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (range)
        {
          masklen2ip (range->lp->prefixlen, ret);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_range_status (int proc_id, struct pal_in4_addr area_id,
                            struct pal_in4_addr net, int *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (range != NULL)
        {
          *ret = range->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_range_effect (int proc_id, struct pal_in4_addr area_id,
                            struct pal_in4_addr net, int *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (range)
        {
          if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
            *ret = OSPF_API_advertiseMatching;
          else
            *ret = OSPF_API_doNotAdvertiseMatching;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospAreaRangeEntry. */
int
ospf_get_next_area_range_area_id (int proc_id, struct pal_in4_addr *area_id,
                                  struct pal_in4_addr *net, int indexlen,
                                  struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup_next (top, area_id, net, indexlen);
      if (range)
        {
          *ret = *area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_range_net (int proc_id, struct pal_in4_addr *area_id,
                              struct pal_in4_addr *net, int indexlen,
                              struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup_next (top, area_id, net, indexlen);
      if (range)
        {
          pal_mem_cpy (ret, range->lp->prefix, sizeof (struct pal_in4_addr));
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_range_mask (int proc_id, struct pal_in4_addr *area_id,
                               struct pal_in4_addr *net, int indexlen,
                               struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup_next (top, area_id, net, indexlen);
      if (range)
        {
          masklen2ip (range->lp->prefixlen, ret);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_range_status (int proc_id, struct pal_in4_addr *area_id,
                                 struct pal_in4_addr *net, int indexlen,
                                 int *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup_next (top, area_id, net, indexlen);
      if (range)
        {
          *ret = range->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_range_effect (int proc_id, struct pal_in4_addr *area_id,
                                 struct pal_in4_addr *net, int indexlen,
                                 int *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup_next (top, area_id, net, indexlen);
      if (range)
        {
          if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
            *ret = OSPF_API_advertiseMatching;
          else
            *ret = OSPF_API_doNotAdvertiseMatching;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* Set functions for ospAreaRangeEntry. */
int
ospf_set_area_range_mask (int proc_id, struct pal_in4_addr area_id,
                          struct pal_in4_addr net, struct pal_in4_addr mask,
                          u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf_area *area;
  struct ospf *top;
  u_char masklen;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (area && range)
        {
          masklen = ip_masklen (mask);
          if (range->lp->prefixlen != masklen)
            {
              ospf_area_range_unset (vr_id, top->ospf_id, area_id,
                                     net, range->lp->prefixlen);
              ospf_area_range_set (vr_id, top->ospf_id,
                                   area_id, net, masklen);
            }

          return OSPF_API_SET_SUCCESS;
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_area_range_status (int proc_id, struct pal_in4_addr area_id,
                            struct pal_in4_addr net, int status,
                            u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_area_range *range;
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (area && range)
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
              if (range->status != ROW_STATUS_ACTIVE)
                return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
              break;
            case ROW_STATUS_CREATEANDGO:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_DESTROY:
              return ospf_area_range_unset (vr->id, top->ospf_id, area_id,
                                     net, range->lp->prefixlen);
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }

          return OSPF_API_SET_SUCCESS;
        }
      /* Row doesn't exists. */
      else
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
            case ROW_STATUS_NOTINSERVICE:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDWAIT:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            case ROW_STATUS_DESTROY:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDGO:
              ospf_area_range_set (vr->id, top->ospf_id, area_id, net, 24);
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }
        }
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_area_range_effect (int proc_id, struct pal_in4_addr area_id,
                            struct pal_in4_addr net, int effect,
                            u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_range_entry_lookup (top, area_id, net);
      if (range)
        {
          if (effect != OSPF_API_doNotAdvertiseMatching
              && effect != OSPF_API_advertiseMatching)
            return OSPF_API_SET_ERR_WRONG_VALUE;

          if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE)
              && effect == OSPF_API_doNotAdvertiseMatching)
            ospf_area_range_not_advertise_set (vr_id, top->ospf_id,
                                               area_id, net,
                                               range->lp->prefixlen);
          else if (!CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE)
                   && effect == OSPF_API_advertiseMatching)
            ospf_area_range_not_advertise_unset (vr_id, top->ospf_id,
                                                 area_id, net,
                                                 range->lp->prefixlen);

          return OSPF_API_SET_SUCCESS;
        }
    }
  return OSPF_API_SET_ERROR;
}


/* Get functions for ospfHostEntry. */
int
ospf_get_host_ip_address (int proc_id, struct pal_in4_addr addr,
                          int tos, struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup (top, addr, tos);
      if (host)
        {
          *ret = host->addr;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_host_tos (int proc_id, struct pal_in4_addr addr, int tos, int *ret,
                   u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup (top, addr, tos);
      if (host)
        {
          *ret = host->tos;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_host_metric (int proc_id, struct pal_in4_addr addr, int tos, int *ret,
                      u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup (top, addr, tos);
      if (host)
        {
          *ret = host->metric;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_host_status (int proc_id, struct pal_in4_addr addr, int tos, int *ret,
                      u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup (top, addr, tos);
      if (host != NULL)
        {
          *ret = ROW_STATUS_ACTIVE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_host_area_id (int proc_id, struct pal_in4_addr addr,
                       int tos, struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup (top, addr, tos);
      if (host)
        {
          *ret = host->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfHostEntry. */
int
ospf_get_next_host_ip_address (int proc_id, struct pal_in4_addr *addr,
                               int *tos, int indexlen,
                               struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup_next (top, addr, tos, indexlen);
      if (host)
        {
          *ret = host->addr;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_host_tos (int proc_id, struct pal_in4_addr *addr,
                        int *tos, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup_next (top, addr, tos, indexlen);
      if (host)
        {
          *ret = host->tos;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_host_metric (int proc_id, struct pal_in4_addr *addr,
                           int *tos, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup_next (top, addr, tos, indexlen);
      if (host)
        {
          *ret = host->metric;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_host_status (int proc_id, struct pal_in4_addr *addr,
                           int *tos, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup_next (top, addr, tos, indexlen);
      if (host != NULL)
        {
          *ret = ROW_STATUS_ACTIVE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_host_area_id (int proc_id, struct pal_in4_addr *addr,
                            int *tos, int indexlen, struct pal_in4_addr *ret,
                            u_int32_t vr_id)
{
  struct ospf_host_entry *host;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup_next (top, addr, tos, indexlen);
      if (host)
        {
          *ret = host->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* Set functions for ospfHostEntry. */
int
ospf_set_host_metric (int proc_id,
                      struct pal_in4_addr addr, int tos, int metric,
                      u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_host_entry *host;
  struct ospf *top;

  if (tos != 0)
    return OSPF_API_SET_ERROR;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    {
      if (!OSPF_API_CHECK_RANGE (metric, Metric))
        return OSPF_API_SET_ERR_WRONG_VALUE;

      host = ospf_host_entry_lookup (top, addr, tos);
      if (host != NULL)
        return ospf_host_entry_cost_set (vr->id, top->ospf_id, addr,
                                         host->area_id, metric);
    }

  return OSPF_API_SET_ERROR;
}

int
ospf_set_host_status (int proc_id,
                      struct pal_in4_addr addr, int tos, int status,
                      u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_host_entry *host;
  struct ospf *top;

  if (tos != 0)
    return OSPF_API_SET_ERROR;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      host = ospf_host_entry_lookup (top, addr, tos);
      /* Row exists. */
      if (host)
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
              if (host->status != ROW_STATUS_ACTIVE)
                return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
              break;
            case ROW_STATUS_CREATEANDGO:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_DESTROY:
              return ospf_host_entry_unset (vr->id, top->ospf_id,
                                            addr, host->area_id);
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }

          return OSPF_API_SET_SUCCESS;
        }
      /* Row doesn't exists. */
      else
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
            case ROW_STATUS_NOTINSERVICE:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDWAIT:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            case ROW_STATUS_DESTROY:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDGO:
              ospf_host_entry_set (vr->id, top->ospf_id,
                                   addr, OSPFBackboneAreaID);
              return OSPF_API_SET_SUCCESS;
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }
        }
    }

  return OSPF_API_SET_SUCCESS;
}

/* XXX ospfHostAreaID is read-create in the latest draft. */


/* Get functions for ospfIfEntry. */
int
ospf_get_if_ip_address (int proc_id, struct pal_in4_addr addr,
                        int ifindex, struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (ifindex == 0)
            *ret = oi->ident.address->u.prefix4;
          else
            *ret = IPv4AddressUnspec;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_address_less_if (int proc_id,
                          struct pal_in4_addr addr, int ifindex, int *ret,
                          u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_address_less_ifindex (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_area_id (int proc_id, struct pal_in4_addr addr,
                     int ifindex, struct pal_in4_addr *ret,
                     u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (oi->area)
            *ret = oi->area->area_id;
          else
            *ret = IPv4AddressUnspec;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_type (int proc_id, struct pal_in4_addr addr, int ifindex, int *ret,
                  u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);

  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (oi->type == OSPF_IFTYPE_LOOPBACK)
            {
              *ret = OSPF_API_IFTYPE_BROADCAST;
              return OSPF_API_GET_SUCCESS;
            }

          *ret = ospf_api_iftype_generic (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_admin_stat (int proc_id,
                        struct pal_in4_addr addr,
                        int ifindex, int *ret,
                        u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = OSPF_API_STATUS_ENABLED;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_rtr_priority (int proc_id,
                          struct pal_in4_addr addr, int ifindex, int *ret,
                          u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = oi->ident.priority;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_transit_delay (int proc_id,
                           struct pal_in4_addr addr, int ifindex, int *ret,
                           u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_if_transmit_delay_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_retrans_interval (int proc_id,
                              struct pal_in4_addr addr, int ifindex, int *ret,
                              u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_if_retransmit_interval_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_hello_interval (int proc_id,
                            struct pal_in4_addr addr, int ifindex, int *ret,
                            u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_if_hello_interval_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_rtr_dead_interval (int proc_id,
                               struct pal_in4_addr addr, int ifindex, int *ret,
                               u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_if_dead_interval_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_poll_interval (int proc_id,
                           struct pal_in4_addr addr, int ifindex, int *ret,
                           u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf_nbr_static *nbr_static;
  struct ospf_neighbor *nbr;
  struct ospf *top;
  struct ls_node *rn;
  struct prefix *p;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
            if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
              {
                p = nbr->ident.address;
                nbr_static = ospf_nbr_static_lookup (top, p->u.prefix4);
                if (nbr_static != NULL)
                  {
                    *ret = nbr_static->poll_interval;
                    ls_unlock_node (rn);
                    return OSPF_API_GET_SUCCESS;
                  }
              }
          *ret = OSPF_POLL_INTERVAL_DEFAULT;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_state (int proc_id,
                   struct pal_in4_addr addr, int ifindex, int *ret,
                   u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_api_ifsm_state (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_designated_router (int proc_id, struct pal_in4_addr addr,
                               int ifindex, struct pal_in4_addr *ret,
                               u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = oi->ident.d_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_backup_designated_router (int proc_id, struct pal_in4_addr addr,
                                      int ifindex, struct pal_in4_addr *ret,
                                      u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = oi->ident.bd_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_events (int proc_id, struct pal_in4_addr addr, int ifindex,
                    int *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = oi->state_change;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_auth_key (int proc_id,
                      struct pal_in4_addr addr, int ifindex, char **ret,
                      u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_if_authentication_key_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_status (int proc_id,
                    struct pal_in4_addr addr, int ifindex, int *ret,
                    u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi != NULL)
        {
          if (!CHECK_FLAG (oi->flags, OSPF_IF_UP))
            *ret = ROW_STATUS_NOTINSERVICE;
          else
            *ret = ROW_STATUS_ACTIVE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_multicast_forwarding (int proc_id, struct pal_in4_addr addr,
                                  int ifindex, int *ret,
                                  u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = OSPF_API_MULTICASTFORWARD_BLOCKED;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_demand (int proc_id,
                    struct pal_in4_addr addr, int ifindex, int *ret,
                    u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = OSPF_API_FALSE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_auth_type (int proc_id,
                       struct pal_in4_addr addr, int ifindex, int *ret,
                       u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          *ret = ospf_if_authentication_type_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfIfEntry. */
int
ospf_get_next_if_ip_address (int proc_id,
                             struct pal_in4_addr *addr,
                             int *ifindex, int indexlen,
                             struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          if (*ifindex == 0)
            *ret = oi->ident.address->u.prefix4;
          else
            *ret = IPv4AddressUnspec;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_address_less_if (int proc_id, struct pal_in4_addr *addr,
                               int *ifindex, int indexlen, int *ret,
                               u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_address_less_ifindex (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_area_id (int proc_id,
                          struct pal_in4_addr *addr,
                          int *ifindex, int indexlen,
                          struct pal_in4_addr *ret,
                          u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          if (oi->area)
            *ret = oi->area->area_id;
          else
            *ret = IPv4AddressUnspec;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_type (int proc_id, struct pal_in4_addr *addr,
                       int *ifindex, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          if (oi->type == OSPF_IFTYPE_LOOPBACK)
            {
              *ret = OSPF_API_IFTYPE_BROADCAST;
              return OSPF_API_GET_SUCCESS;
            }

          *ret = ospf_api_iftype_generic (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_admin_stat (int proc_id, struct pal_in4_addr *addr,
                             int *ifindex, int indexlen, int *ret,
                             u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = OSPF_API_STATUS_ENABLED;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_rtr_priority (int proc_id, struct pal_in4_addr *addr,
                               int *ifindex, int indexlen, int *ret,
                               u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = oi->ident.priority;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_transit_delay (int proc_id, struct pal_in4_addr *addr,
                                int *ifindex, int indexlen, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_if_transmit_delay_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_retrans_interval (int proc_id, struct pal_in4_addr *addr,
                                   int *ifindex, int indexlen, int *ret,
                                   u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_if_retransmit_interval_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_hello_interval (int proc_id, struct pal_in4_addr *addr,
                                 int *ifindex, int indexlen, int *ret,
                                 u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_if_hello_interval_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_rtr_dead_interval (int proc_id, struct pal_in4_addr *addr,
                                    int *ifindex, int indexlen, int *ret,
                                    u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_if_dead_interval_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_poll_interval (int proc_id, struct pal_in4_addr *addr,
                                int *ifindex, int indexlen, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf_nbr_static *nbr_static;
  struct ospf_neighbor *nbr;
  struct ospf *top;
  struct ls_node *rn;
  struct prefix *p;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
            if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
              {
                p = nbr->ident.address;
                nbr_static = ospf_nbr_static_lookup (top, p->u.prefix4);
                if (nbr_static != NULL)
                  {
                    *ret = nbr_static->poll_interval;
                    ls_unlock_node (rn);
                    return OSPF_API_GET_SUCCESS;
                  }
              }
          *ret = OSPF_POLL_INTERVAL_DEFAULT;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_state (int proc_id, struct pal_in4_addr *addr,
                        int *ifindex, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_api_ifsm_state (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_designated_router (int proc_id,
                                    struct pal_in4_addr *addr,
                                    int *ifindex, int indexlen,
                                    struct pal_in4_addr *ret,
                                    u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = oi->ident.d_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_backup_designated_router (int proc_id,
                                           struct pal_in4_addr *addr,
                                           int *ifindex, int indexlen,
                                           struct pal_in4_addr *ret,
                                           u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = oi->ident.bd_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_events (int proc_id, struct pal_in4_addr *addr,
                         int *ifindex, int indexlen, int *ret,
                         u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = oi->state_change;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_auth_key (int proc_id, struct pal_in4_addr *addr,
                           int *ifindex, int indexlen, char **ret,
                           u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_if_authentication_key_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_status (int proc_id, struct pal_in4_addr *addr,
                         int *ifindex, int indexlen, int *ret,
                         u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi != NULL)
        {
          if (!CHECK_FLAG (oi->flags, OSPF_IF_UP))
            *ret = ROW_STATUS_NOTINSERVICE;
          else
            *ret = ROW_STATUS_ACTIVE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_multicast_forwarding (int proc_id, struct pal_in4_addr *addr,
                                       int *ifindex, int indexlen, int *ret,
                                       u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = OSPF_API_MULTICASTFORWARD_BLOCKED;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_demand (int proc_id, struct pal_in4_addr *addr,
                         int *ifindex, int indexlen, int *ret,
                         u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = OSPF_API_FALSE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_auth_type (int proc_id, struct pal_in4_addr *addr,
                            int *ifindex, int indexlen, int *ret,
                            u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          *ret = ospf_if_authentication_type_get (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* Set functions for ospfIfEntry. */
int
ospf_set_if_area_id (int proc_id, struct pal_in4_addr addr, int ifindex,
                     struct pal_in4_addr area_id, u_int32_t vr_id)
{
  struct apn_vrf *iv;
  struct ospf *top;
  struct ospf_network *nw;
  struct interface *ifp;
  struct connected *ifc;
  struct route_node *rn;
  struct ls_prefix lp;
#ifdef HAVE_OSPF_MULTI_INST
  u_int16_t inst_id = OSPF_IF_INSTANCE_ID_DEFAULT;
#else
  u_int16_t inst_id = OSPF_IF_INSTANCE_ID_UNUSE; 
#endif /* HAVE_OSPF_MULTI_INST */

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      iv = top->ov->iv;
      for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
        if ((ifp = rn->info))
          for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
            if (IPV4_ADDR_SAME (&addr, &ifc->address->u.prefix4))
              {
                route_unlock_node (rn);
                ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN,
                                    ifc->address->u.prefix4);

                /* Remove the previuos configuration.  */
                nw = ospf_network_lookup (top, &lp);
                if (nw != NULL)
                  ospf_network_unset (vr_id, top->ospf_id, addr,
                                      IPV4_MAX_BITLEN, nw->area_id, inst_id);
                /* Set the configuration.  */
                ospf_network_set (vr_id, top->ospf_id, addr,
                                  IPV4_MAX_BITLEN, area_id, inst_id);

                return OSPF_API_SET_SUCCESS;
              }
    }

  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_type (int proc_id, struct pal_in4_addr addr, int ifindex,
                  int type, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  int type_old, type_new;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (oi->type == OSPF_IFTYPE_LOOPBACK)
            return OSPF_API_SET_ERR_READONLY;

          type_old = ospf_if_network_get (oi);

          switch (type)
            {
            case OSPF_API_IFTYPE_BROADCAST:
              type_new = OSPF_IFTYPE_BROADCAST;
              break;
            case OSPF_API_IFTYPE_NBMA:
              type_new = OSPF_IFTYPE_NBMA;
              break;
            case OSPF_API_IFTYPE_POINTOPOINT:
              type_new = OSPF_IFTYPE_POINTOPOINT;
              break;
            case OSPF_API_IFTYPE_POINTOMULTIPOINT:
              type_new = OSPF_IFTYPE_POINTOMULTIPOINT;
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
              break;
            }

          if (type_old != type_new)
            ospf_if_network_set (vr_id, oi->u.ifp->name, type_new);

          return OSPF_API_SET_SUCCESS;
        }
    }

  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_admin_stat (int proc_id,
                        struct pal_in4_addr addr,
                        int ifindex, int status, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_master *om = vr->proto;
  struct ospf_interface *oi;
  struct ospf *top;
  struct connected *ifc;
  struct route_node *rn;
  struct interface *ifp;
  struct ls_prefix lp;
  struct apn_vrf *iv;
#ifdef HAVE_OSPF_MULTI_INST
  u_int16_t inst_id = OSPF_IF_INSTANCE_ID_DEFAULT;
#else
  u_int16_t inst_id = OSPF_IF_INSTANCE_ID_UNUSE;
#endif /* HAVE_OSPF_MULTI_INST */

  if (status != OSPF_API_STATUS_ENABLED
      && status != OSPF_API_STATUS_DISABLED)
    return OSPF_API_SET_ERR_WRONG_VALUE;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi == NULL && status == OSPF_API_STATUS_ENABLED)
        {
          iv = top->ov->iv;
          for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
            if ((ifp = rn->info))
              for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
                if (IPV4_ADDR_SAME (&addr, &ifc->address->u.prefix4))
                  {
                    ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN,
                                        ifc->address->u.prefix4);
                    if (!ospf_network_lookup (top, &lp))
                      {
                        route_unlock_node (rn);
                        return ospf_network_set (vr->id, top->ospf_id, addr,
                                                 IPV4_MAX_BITLEN,
                                                 OSPFBackboneAreaID, inst_id);
                      }
                  }
        }
      else if (oi != NULL && status == OSPF_API_STATUS_DISABLED)
        return ospf_network_unset (om->vr->id, top->ospf_id,
                                   oi->ident.address->u.prefix4,
                                   IPV4_MAX_BITLEN,
                                   oi->area->area_id, inst_id);
      else
        return OSPF_API_SET_SUCCESS;
    }

  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_rtr_priority (int proc_id, struct pal_in4_addr addr,
                          int ifindex, int priority, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_master *om = vr->proto;
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (!OSPF_API_CHECK_RANGE (priority, DesignatedRouterPriority))
            return OSPF_API_SET_ERR_WRONG_VALUE;

          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            ospf_if_priority_set (vr_id, oi->u.ifp->name, priority);
          else
            ospf_if_priority_set_by_addr (vr_id, oi->u.ifp->name,
                                          addr, priority);

          ospf_if_priority_apply (om, addr, oi->u.ifp->ifindex, priority);

          return OSPF_API_SET_SUCCESS;
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_transit_delay (int proc_id, struct pal_in4_addr addr,
                           int ifindex, int delay, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (!OSPF_API_CHECK_RANGE (delay, UpToMaxAge))
            return OSPF_API_SET_ERR_WRONG_VALUE;

          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            ospf_if_transmit_delay_set (vr_id, oi->u.ifp->name, delay);
          else
            ospf_if_transmit_delay_set_by_addr (vr_id, oi->u.ifp->name,
                                                addr, delay);

          return OSPF_API_SET_SUCCESS;
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_retrans_interval (int proc_id, struct pal_in4_addr addr,
                              int ifindex, int interval, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (!OSPF_API_CHECK_RANGE (interval, UpToMaxAge))
            return OSPF_API_SET_ERR_WRONG_VALUE;

          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            return ospf_if_retransmit_interval_set (vr_id,
                                                    oi->u.ifp->name, interval);
          else
            return ospf_if_retransmit_interval_set_by_addr (vr_id,
                                                            oi->u.ifp->name,
                                                            addr,interval);
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_hello_interval (int proc_id, struct pal_in4_addr addr,
                            int ifindex, int interval, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;
  int ret;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (!OSPF_API_CHECK_RANGE (interval, HelloRange))
            return OSPF_API_SET_ERR_WRONG_VALUE;

          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            ret = ospf_if_hello_interval_set (vr_id,
                                              oi->u.ifp->name, interval);
          else
            ret = ospf_if_hello_interval_set_by_addr (vr_id, oi->u.ifp->name,
                                                      addr, interval);
          return ret;
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_rtr_dead_interval (int proc_id, struct pal_in4_addr addr,
                               int ifindex, int interval, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;
  int ret;

  if (interval < OSPF_ROUTER_DEAD_INTERVAL_MIN
      || interval > OSPF_ROUTER_DEAD_INTERVAL_MAX)
    return OSPF_API_SET_ERR_IF_DEAD_INTERVAL_INVALID;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (!OSPF_API_CHECK_RANGE (interval, PositiveInteger))
            return OSPF_API_SET_ERR_WRONG_VALUE;

          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            ret = ospf_if_dead_interval_set (vr_id,
                                             oi->u.ifp->name, interval);
          else
            ret = ospf_if_dead_interval_set_by_addr (vr_id, oi->u.ifp->name,
                                                     addr, interval);
          return ret;
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_poll_interval (int proc_id, struct pal_in4_addr addr,
                           int ifindex, int interval, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct ospf *top;
  struct ls_node *rn;

  if (interval < OSPF_POLL_INTERVAL_MIN
      || interval > OSPF_POLL_INTERVAL_MAX)
    return OSPF_API_SET_ERR_WRONG_VALUE;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          /* Set Poll Interval of all associated neighbors. */
          for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
            if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
              return (ospf_nbr_static_poll_interval_set (vr_id, top->ospf_id,
                                                 nbr->ident.address->u.prefix4,
                                                 interval));

        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_auth_key (int proc_id, struct pal_in4_addr addr,
                      int ifindex, int length, char *auth_key, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  char buf[OSPF_AUTH_SIMPLE_SIZE + 1];
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          length = length > OSPF_AUTH_SIMPLE_SIZE ?
            OSPF_AUTH_SIMPLE_SIZE : length;

          pal_mem_set (buf, 0, OSPF_AUTH_SIMPLE_SIZE + 1);
          pal_strncpy (buf, auth_key, length);

          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            ospf_if_authentication_key_set (vr->id, oi->u.ifp->name, auth_key);
          else
            ospf_if_authentication_key_set_by_addr (vr->id, oi->u.ifp->name,
                                                    addr, auth_key);

          return OSPF_API_SET_SUCCESS;
        }
    }
  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_if_status (int proc_id, struct pal_in4_addr addr,
                    int ifindex, int status, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;
  struct ospf_master *om;
  struct prefix p;
  int ret= 0;
#ifdef HAVE_OSPF_MULTI_INST
  u_int16_t inst_id = OSPF_IF_INSTANCE_ID_DEFAULT;
#else
  u_int16_t inst_id = OSPF_IF_INSTANCE_ID_UNUSE;
#endif /* HAVE_OSPF_MULTI_INST */

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    {
      om = top->om;
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      switch (status)
        {
        case ROW_STATUS_ACTIVE:
          if (oi != NULL && CHECK_FLAG (oi->flags, OSPF_IF_UP))
            return OSPF_API_SET_SUCCESS;
          break;
        case ROW_STATUS_NOTINSERVICE:
          if (oi != NULL && !CHECK_FLAG (oi->flags, OSPF_IF_UP))
            return OSPF_API_SET_SUCCESS;
          break;
        case ROW_STATUS_CREATEANDGO:
          if (oi == NULL)
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &addr);
              apply_mask (&p);
              /* set the configuration */
              ret = ospf_network_set (vr_id, top->ospf_id, addr,
                                      p.prefixlen, OSPFBackboneAreaID, inst_id);
              if (ret == OSPF_API_SET_SUCCESS)
                return OSPF_API_SET_SUCCESS;
              else
                ospf_network_unset (vr_id, top->ospf_id, addr,
                                    p.prefixlen, OSPFBackboneAreaID, inst_id);
            }
          break;
        case ROW_STATUS_CREATEANDWAIT:
          return OSPF_API_SET_ERR_WRONG_VALUE;
          break;
        case ROW_STATUS_DESTROY:
          if (oi != NULL)
            {
              p.family = AF_INET;
              p.prefixlen = IPV4_MAX_BITLEN;
              IPV4_ADDR_COPY (&p.u.prefix4, &addr);
              apply_mask (&p);

              /* remove the configuration and delete the row*/
              ret = ospf_network_unset (vr_id, top->ospf_id, addr,
                                        p.prefixlen,
                                        oi->area->area_id, inst_id);
              if (ret == OSPF_API_SET_SUCCESS)
                return OSPF_API_SET_SUCCESS;
            }
          break;
        case ROW_STATUS_NOTREADY:
        default:
          return OSPF_API_SET_ERR_WRONG_VALUE;
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_multicast_forwarding (int proc_id, struct pal_in4_addr addr,
                                  int ifindex, int status, u_int32_t vr_id)
{
  /* Currently we don't support. */
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_demand (int proc_id, struct pal_in4_addr addr,
                    int ifindex, int status, u_int32_t vr_id)
{
  /* Currently we don't support. */
  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_auth_type (int proc_id, struct pal_in4_addr addr,
                       int ifindex, int type, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (!OSPF_API_CHECK_RANGE (type, AuthType))
            return OSPF_API_SET_ERR_WRONG_VALUE;

          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            ospf_if_authentication_type_set (vr_id, oi->u.ifp->name, type);
          else
            ospf_if_authentication_type_set_by_addr (vr_id, oi->u.ifp->name,
                                                     addr, type);

          return OSPF_API_SET_SUCCESS;
        }
    }
  return OSPF_API_SET_ERROR;
}


/* Get functions for ospfIfMetricEntry. */
int
ospf_get_if_metric_ip_address (int proc_id, struct pal_in4_addr addr,
                               int ifindex, int tos, struct pal_in4_addr *ret,
                               u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (tos != 0)
            return OSPF_API_GET_ERROR;

          if (ifindex == 0)
            *ret = oi->ident.address->u.prefix4;
          else
            *ret = IPv4AddressUnspec;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_metric_address_less_if (int proc_id, struct pal_in4_addr addr,
                                    int ifindex, int tos, int *ret,
                                    u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = ospf_address_less_ifindex (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_metric_tos (int proc_id, struct pal_in4_addr addr, int ifindex,
                        int tos, int *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_metric_value (int proc_id, struct pal_in4_addr addr,
                          int ifindex, int tos, int *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = oi->output_cost;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_if_metric_status (int proc_id, struct pal_in4_addr addr,
                           int ifindex, int tos, int *ret, u_int32_t vr_id)
{
  struct ospf_if_params *oip;
  struct ospf_interface *oi;
  struct ospf *top;

  if (tos != 0)
    return OSPF_API_GET_ERROR;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi != NULL)
        {
          oip = ospf_if_params_get_default (top->om, oi->u.ifp->name);
          if (oip != NULL)
            {
              *ret = ROW_STATUS_ACTIVE;
              return OSPF_API_GET_SUCCESS;
            }
        }
    }
  return OSPF_API_GET_ERROR;
}


/* GetNext functions for ospfIfMetricEntry. */
int
ospf_get_next_if_metric_ip_address (int proc_id, struct pal_in4_addr *addr,
                                    int *ifindex, int *tos, int indexlen,
                                    struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      /* This is a little bit hacky. */
      if (indexlen > 5)
        indexlen = 5;

      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          if (*ifindex == 0)
            *ret = oi->ident.address->u.prefix4;
          else
            *ret = IPv4AddressUnspec;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_metric_address_less_if (int proc_id,
                                         struct pal_in4_addr *addr,
                                         int *ifindex, int *tos,
                                         int indexlen, int *ret,
                                         u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      /* This is a little bit hacky. */
      if (indexlen > 5)
        indexlen = 5;

      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = ospf_address_less_ifindex (oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_metric_tos (int proc_id, struct pal_in4_addr *addr,
                             int *ifindex, int *tos, int indexlen, int *ret,
                             u_int32_t vr_id)
{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      /* This is a little bit hacky. */
      if (indexlen > 5)
        indexlen = 5;

      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_metric_value (int proc_id, struct pal_in4_addr *addr,
                               int *ifindex, int *tos, int indexlen, int *ret,
                               u_int32_t vr_id)

{
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      /* This is a little bit hacky. */
      if (indexlen > 5)
        indexlen = 5;

      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi)
        {
          if (*tos != 0)
            return OSPF_API_GET_ERROR;

          *ret = oi->output_cost;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_if_metric_status (int proc_id, struct pal_in4_addr *addr,
                                int *ifindex, int *tos, int indexlen, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_if_params *oip;
  struct ospf_interface *oi;
  struct ospf *top;

  if (*tos != 0)
    return OSPF_API_GET_ERROR;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      /* This is a little bit hacky. */
      if (indexlen > 5)
        indexlen = 5;

      oi = ospf_if_entry_lookup_next (top, addr, ifindex, indexlen);
      if (oi != NULL)
        {
          oip = ospf_if_params_get_default (top->om, oi->u.ifp->name);
          if (oip != NULL)
            {
              *ret = ROW_STATUS_ACTIVE;
              return OSPF_API_GET_SUCCESS;
            }
        }
    }
  return OSPF_API_GET_ERROR;
}


/* Set functions for ospfIfMetricEntry. */
int
ospf_set_if_metric_value (int proc_id, struct pal_in4_addr addr,
                          int ifindex, int tos, int metric, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_interface *oi;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi)
        {
          if (tos != 0)
            return OSPF_API_SET_ERROR;

          if (!OSPF_API_CHECK_RANGE (metric, Metric))
            return OSPF_API_SET_ERR_WRONG_VALUE;

          /* XXX this is not correct. */
          if (ospf_if_connected_count (oi->u.ifp) <= 1)
            return (ospf_if_cost_set (vr->id, oi->u.ifp->name, metric));
          else
            return (ospf_if_cost_set_by_addr (vr->id, oi->u.ifp->name, addr,
                                              metric));

          return OSPF_API_SET_SUCCESS;
        }
    }

  return OSPF_API_SET_ERROR;
}

int
ospf_set_if_metric_status (int proc_id, struct pal_in4_addr addr,
                           int ifindex, int tos, int status, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_if_params *oip = NULL;
  struct ospf_interface *oi;
  struct ospf *top;

  if (tos != 0)
    return OSPF_API_SET_ERR_INVALID_VALUE;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      oi = ospf_if_entry_lookup (top, addr, ifindex);
      if (oi != NULL)
        oip = ospf_if_params_lookup_default (top->om, oi->u.ifp->name);

      switch (status)
        {
        case ROW_STATUS_ACTIVE:
          if (oip != NULL)
            {
              if (OSPF_IF_PARAM_CHECK (oip, OUTPUT_COST))
                return OSPF_API_SET_SUCCESS;

              return ospf_if_cost_set (vr->id, oi->u.ifp->name,
                                       OSPF_OUTPUT_COST_DEFAULT);
            }
          break;
        case ROW_STATUS_NOTINSERVICE:
          if (oip != NULL)
            {
              if (!OSPF_IF_PARAM_CHECK (oip, OUTPUT_COST))
                return OSPF_API_SET_SUCCESS;

              return ospf_if_cost_unset (vr->id, oi->u.ifp->name);
            }
           break;
        case ROW_STATUS_CREATEANDGO:
            if (oip == NULL && oi != NULL)
              return ospf_if_cost_set (vr->id, oi->u.ifp->name,
                                           OSPF_OUTPUT_COST_DEFAULT);
            break;
        case ROW_STATUS_CREATEANDWAIT:
          /* Currently we don't support.  */
          break;
        case ROW_STATUS_DESTROY:
           if (oip != NULL)
             return ospf_if_cost_unset (vr->id, oi->u.ifp->name);
          break;
        case ROW_STATUS_NOTREADY:
        default:
          return OSPF_API_SET_ERR_WRONG_VALUE;
        }
    }
  return OSPF_API_SET_ERROR;
}


/* Get functions for ospfVirtIfEntry. */
int
ospf_get_virt_if_area_id (int proc_id, struct pal_in4_addr area_id,
                          struct pal_in4_addr nbr_id, struct pal_in4_addr *ret,                           u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = vlink->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_neighbor (int proc_id,
                           struct pal_in4_addr area_id,
                           struct pal_in4_addr nbr_id,
                           struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = vlink->peer_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_transit_delay (int proc_id, struct pal_in4_addr area_id,
                                struct pal_in4_addr nbr_id, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = ospf_if_transmit_delay_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_retrans_interval (int proc_id, struct pal_in4_addr area_id,
                                   struct pal_in4_addr nbr_id, int *ret,
                                   u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = ospf_if_retransmit_interval_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_hello_interval (int proc_id, struct pal_in4_addr area_id,
                                 struct pal_in4_addr nbr_id, int *ret,
                                 u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = ospf_if_hello_interval_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_rtr_dead_interval (int proc_id, struct pal_in4_addr area_id,
                                    struct pal_in4_addr nbr_id, int *ret,
                                    u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = ospf_if_dead_interval_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_state (int proc_id, struct pal_in4_addr area_id,
                        struct pal_in4_addr nbr_id, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = ospf_api_ifsm_state (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_events (int proc_id, struct pal_in4_addr area_id,
                         struct pal_in4_addr nbr_id, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = vlink->oi->state_change;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_auth_key (int proc_id, struct pal_in4_addr area_id,
                           struct pal_in4_addr nbr_id, char **ret,
                           u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = ospf_if_authentication_key_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_status (int proc_id, struct pal_in4_addr area_id,
                         struct pal_in4_addr nbr_id, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink != NULL)
        {
          *ret = vlink->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_if_auth_type (int proc_id, struct pal_in4_addr area_id,
                            struct pal_in4_addr nbr_id, int *ret,
                            u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = ospf_if_authentication_type_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}


/* GetNext functions for ospfVirtIfEntry. */
int
ospf_get_next_virt_if_area_id (int proc_id, struct pal_in4_addr *area_id,
                               struct pal_in4_addr *nbr_id, int indexlen,
                               struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = vlink->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_neighbor (int proc_id, struct pal_in4_addr *area_id,
                                struct pal_in4_addr *nbr_id, int indexlen,
                                struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = vlink->peer_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_transit_delay (int proc_id, struct pal_in4_addr *area_id,
                                     struct pal_in4_addr *nbr_id, int indexlen,
                                     int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = ospf_if_transmit_delay_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_retrans_interval (int proc_id,
                                        struct pal_in4_addr *area_id,
                                        struct pal_in4_addr *nbr_id,
                                        int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = ospf_if_retransmit_interval_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_hello_interval (int proc_id,
                                      struct pal_in4_addr *area_id,
                                      struct pal_in4_addr *nbr_id,
                                      int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = ospf_if_hello_interval_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_rtr_dead_interval (int proc_id,
                                         struct pal_in4_addr *area_id,
                                         struct pal_in4_addr *nbr_id,
                                         int indexlen, int *ret,
                                         u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = ospf_if_dead_interval_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_state (int proc_id, struct pal_in4_addr *area_id,
                             struct pal_in4_addr *nbr_id,
                             int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = ospf_api_ifsm_state (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_events (int proc_id, struct pal_in4_addr *area_id,
                              struct pal_in4_addr *nbr_id,
                              int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = vlink->oi->state_change;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_auth_key (int proc_id, struct pal_in4_addr *area_id,
                                struct pal_in4_addr *nbr_id, int indexlen,
                                char **ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = ospf_if_authentication_key_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_status (int proc_id, struct pal_in4_addr *area_id,
                              struct pal_in4_addr *nbr_id,
                              int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = vlink->status;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_if_auth_type (int proc_id, struct pal_in4_addr *area_id,
                                 struct pal_in4_addr *nbr_id, int indexlen,
                                 int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = ospf_if_authentication_type_get (vlink->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}


/* Set functions for ospfVirtIfEntry. */
int
ospf_set_virt_if_transit_delay (int proc_id, struct pal_in4_addr area_id,
                                struct pal_in4_addr nbr_id, int delay,
                                u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    return ospf_vlink_transmit_delay_set (vr->id, top->ospf_id,
                                          area_id, nbr_id, delay);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_virt_if_retrans_interval (int proc_id, struct pal_in4_addr area_id,
                                   struct pal_in4_addr nbr_id, int interval,
                                   u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    return ospf_vlink_retransmit_interval_set (vr->id, top->ospf_id,
                                               area_id, nbr_id, interval);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_virt_if_hello_interval (int proc_id, struct pal_in4_addr area_id,
                                 struct pal_in4_addr nbr_id, int interval,
                                 u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    return ospf_vlink_hello_interval_set (vr->id, top->ospf_id,
                                          area_id, nbr_id, interval);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_virt_if_rtr_dead_interval (int proc_id, struct pal_in4_addr area_id,
                                    struct pal_in4_addr nbr_id, int interval,
                                    u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    return ospf_vlink_dead_interval_set (vr->id, top->ospf_id,
                                         area_id, nbr_id, interval);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_virt_if_auth_key (int proc_id, struct pal_in4_addr area_id,
                           struct pal_in4_addr nbr_id,
                           int length, char *auth_key, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    return ospf_vlink_authentication_key_set (vr->id, top->ospf_id,
                                              area_id, nbr_id, auth_key);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_virt_if_status (int proc_id, struct pal_in4_addr area_id,
                         struct pal_in4_addr nbr_id, int status,
                         u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (area && vlink)
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
              if (vlink->status != ROW_STATUS_ACTIVE)
                return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
              break;
            case ROW_STATUS_CREATEANDGO:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_DESTROY:
              if (vlink->status == ROW_STATUS_ACTIVE)
                ospf_vlink_delete (top, vlink);
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }

          return OSPF_API_SET_SUCCESS;
        }
      /* Row doesn't exists. */
      else
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
            case ROW_STATUS_NOTINSERVICE:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDWAIT:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            case ROW_STATUS_DESTROY:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDGO:
              area = ospf_area_get (top, area_id);
              if (area)
                {
                  vlink = ospf_vlink_get (top, area_id, nbr_id);
                  if (vlink == NULL)
                    return OSPF_API_SET_ERR_VLINK_CANT_GET;
                }
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }
        }
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_virt_if_auth_type (int proc_id, struct pal_in4_addr area_id,
                            struct pal_in4_addr nbr_id, int type,
                            u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    return ospf_vlink_authentication_type_set (vr->id, top->ospf_id,
                                               area_id, nbr_id, type);

  return OSPF_API_SET_SUCCESS;
}


/* Get functions for ospfNbrEntry. */
int
ospf_get_nbr_ip_addr (int proc_id, struct pal_in4_addr nbr_addr, int ifindex,
                      struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = nbr->ident.address->u.prefix4;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_address_less_index (int proc_id, struct pal_in4_addr nbr_addr,
                                 int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = ospf_address_less_ifindex (nbr->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_rtr_id (int proc_id, struct pal_in4_addr nbr_addr,
                     int ifindex, struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = nbr->ident.router_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_options (int proc_id, struct pal_in4_addr nbr_addr,
                      int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = nbr->options;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_priority (int proc_id, struct pal_in4_addr nbr_addr,
                       int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr != NULL)
        {
          nbr_static = ospf_nbr_static_lookup (top, nbr_addr);
          if (nbr_static != NULL)
            *ret = nbr_static->priority;
          else
            *ret = nbr->ident.priority;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_state (int proc_id, struct pal_in4_addr nbr_addr,
                    int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = nbr->state;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_events (int proc_id, struct pal_in4_addr nbr_addr,
                     int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = nbr->state_change;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_ls_retrans_qlen (int proc_id, struct pal_in4_addr nbr_addr,
                              int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = ospf_ls_retransmit_count (nbr);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbma_nbr_status (int proc_id, struct pal_in4_addr nbr_addr,
                          int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = ROW_STATUS_ACTIVE;
          return OSPF_API_GET_SUCCESS;
        }
      else
        {
          nbr_static = ospf_nbr_static_lookup (top, nbr_addr);
          if (nbr_static)
            {
              *ret = nbr_static->status;
              return OSPF_API_GET_SUCCESS;
            }
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbma_nbr_permanence (int proc_id, struct pal_in4_addr nbr_addr,
                              int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = OSPF_API_NBMA_NBR_PERMANENT;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_nbr_hello_suppressed (int proc_id, struct pal_in4_addr nbr_addr,
                               int ifindex, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr)
        {
          *ret = OSPF_API_FALSE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfNbrEntry. */
int
ospf_get_next_nbr_ip_addr (int proc_id, struct pal_in4_addr *nbr_addr,
                           int *ifindex, int indexlen,
                           struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = nbr->ident.address->u.prefix4;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_address_less_index (int proc_id,
                                      struct pal_in4_addr *nbr_addr,
                                      int *ifindex, int indexlen, int *ret,
                                      u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = ospf_address_less_ifindex (nbr->oi);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_rtr_id (int proc_id, struct pal_in4_addr *nbr_addr,
                          int *ifindex, int indexlen, struct pal_in4_addr *ret,
                          u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = nbr->ident.router_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_options (int proc_id, struct pal_in4_addr *nbr_addr,
                           int *ifindex, int indexlen, int *ret,
                           u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = nbr->options;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_priority (int proc_id, struct pal_in4_addr *nbr_addr,
                            int *ifindex, int indexlen, int *ret,
                            u_int32_t vr_id)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr != NULL)
        {
          nbr_static = ospf_nbr_static_lookup (top, *nbr_addr);
          if (nbr_static != NULL)
            *ret = nbr_static->priority;
          else
            *ret = nbr->ident.priority;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_state (int proc_id, struct pal_in4_addr *nbr_addr,
                         int *ifindex, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = nbr->state;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_events (int proc_id, struct pal_in4_addr *nbr_addr,
                          int *ifindex, int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = nbr->state_change;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_ls_retrans_qlen (int proc_id, struct pal_in4_addr *nbr_addr,
                                   int *ifindex, int indexlen, int *ret,
                                   u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = ospf_ls_retransmit_count (nbr);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbma_nbr_status (int proc_id, struct pal_in4_addr *nbr_addr,
                               int *ifindex, int indexlen, int *ret,
                               u_int32_t vr_id)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = ROW_STATUS_ACTIVE;
          return OSPF_API_GET_SUCCESS;
        }
      else
        {
          nbr_static = ospf_nbr_static_lookup_next (top, nbr_addr, indexlen);
          if (nbr_static)
            {
              *nbr_addr = nbr_static->addr;
              *ret = nbr_static->status;
              return OSPF_API_GET_SUCCESS;
            }
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbma_nbr_permanence (int proc_id, struct pal_in4_addr *nbr_addr,
                                   int *ifindex, int indexlen, int *ret,
                                   u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = OSPF_API_NBMA_NBR_PERMANENT;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_nbr_hello_suppressed (int proc_id, struct pal_in4_addr *nbr_addr,
                                    int *ifindex, int indexlen, int *ret,
                                    u_int32_t vr_id)
{
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr = ospf_nbr_entry_lookup_next (top, nbr_addr,
                                        (unsigned int *)ifindex, indexlen);
      if (nbr)
        {
          *ret = OSPF_API_FALSE;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* Set functions for ospfNbrEntry. */
int
ospf_set_nbr_priority (int proc_id, struct pal_in4_addr nbr_addr, int ifindex,
                       int priority, u_int32_t vr_id)
{
  struct ospf_nbr_static *nbr_static;
  struct ospf_neighbor *nbr;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top != NULL)
    {
      nbr = ospf_nbr_entry_lookup (top, nbr_addr, ifindex);
      if (nbr != NULL)
        {
          nbr_static = ospf_nbr_static_lookup (top, nbr_addr);
          if (nbr_static != NULL)
            {
              if (nbr_static->priority != priority)
                ospf_nbr_static_priority_apply (nbr_static, priority);

              return OSPF_API_SET_SUCCESS;
            }
        }
    }
  return OSPF_API_SET_ERROR;
}

int
ospf_set_nbma_nbr_status (int proc_id, struct pal_in4_addr nbr_addr,
                          int ifindex, int status, u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_nbr_static *nbr_static;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      nbr_static = ospf_nbr_static_lookup (top, nbr_addr);
      if (nbr_static)
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
              if (nbr_static->status != ROW_STATUS_ACTIVE)
                return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
              break;
            case ROW_STATUS_CREATEANDGO:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_DESTROY:
              if (nbr_static->status == ROW_STATUS_ACTIVE)
                return ospf_nbr_static_unset (vr->id, top->ospf_id, nbr_addr);
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }

          return OSPF_API_SET_SUCCESS;
        }
      /* Row doesn't exists. */
      else
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
            case ROW_STATUS_NOTINSERVICE:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDWAIT:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            case ROW_STATUS_DESTROY:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDGO:
              ospf_nbr_static_set (vr->id, top->ospf_id, nbr_addr);
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }
        }
    }

  return OSPF_API_SET_SUCCESS;
}


/* Get functions for ospfVirtNbrEntry. */
int
ospf_get_virt_nbr_area (int proc_id, struct pal_in4_addr area_id,
                        struct pal_in4_addr nbr_id, struct pal_in4_addr *ret,
                        u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = vlink->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_nbr_rtr_id (int proc_id, struct pal_in4_addr area_id,
                          struct pal_in4_addr nbr_id, struct pal_in4_addr *ret,
                          u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = vlink->peer_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_nbr_ip_addr (int proc_id, struct pal_in4_addr area_id,
                           struct pal_in4_addr nbr_id,
                           struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = vlink->peer_addr;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_nbr_options (int proc_id, struct pal_in4_addr area_id,
                           struct pal_in4_addr nbr_id, int *ret,
                           u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = nbr->options;
          else
            *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_nbr_state (int proc_id, struct pal_in4_addr area_id,
                         struct pal_in4_addr nbr_id, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = nbr->state;
          else
            *ret = NFSM_Down;
          return OSPF_API_GET_SUCCESS;

        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_nbr_events (int proc_id, struct pal_in4_addr area_id,
                          struct pal_in4_addr nbr_id, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = nbr->state_change;
          else
            *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_nbr_ls_retrans_qlen (int proc_id, struct pal_in4_addr area_id,
                                   struct pal_in4_addr nbr_id, int *ret,
                                   u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = ospf_ls_retransmit_count (nbr);
          else
            *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_virt_nbr_hello_suppressed (int proc_id, struct pal_in4_addr area_id,
                                    struct pal_in4_addr nbr_id, int *ret,
                                    u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup (top, area_id, nbr_id);
      if (vlink)
        {
          *ret = OSPF_API_FALSE;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}


/* GetNext functions for ospfVirtNbrEntry. */
int
ospf_get_next_virt_nbr_area (int proc_id, struct pal_in4_addr *area_id,
                             struct pal_in4_addr *nbr_id, int indexlen,
                             struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = vlink->area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_nbr_rtr_id (int proc_id, struct pal_in4_addr *area_id,
                               struct pal_in4_addr *nbr_id, int indexlen,
                               struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = vlink->peer_id;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_nbr_ip_addr (int proc_id, struct pal_in4_addr *area_id,
                                struct pal_in4_addr *nbr_id, int indexlen,
                                struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = vlink->peer_addr;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_nbr_options (int proc_id, struct pal_in4_addr *area_id,
                                struct pal_in4_addr *nbr_id, int indexlen,
                                int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = nbr->options;
          else
            *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_nbr_state (int proc_id, struct pal_in4_addr *area_id,
                              struct pal_in4_addr *nbr_id, int indexlen,
                              int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = nbr->state;
          else
            *ret = NFSM_Down;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_nbr_events (int proc_id, struct pal_in4_addr *area_id,
                               struct pal_in4_addr *nbr_id, int indexlen,
                               int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = nbr->state_change;
          else
            *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_nbr_ls_retrans_qlen (int proc_id,
                                        struct pal_in4_addr *area_id,
                                        struct pal_in4_addr *nbr_id,
                                        int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          struct ospf_neighbor *nbr;

          nbr = ospf_virt_nbr_lookup_by_vlink (vlink);
          if (nbr)
            *ret = ospf_ls_retransmit_count (nbr);
          else
            *ret = 0;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_virt_nbr_hello_suppressed (int proc_id,
                                         struct pal_in4_addr *area_id,
                                         struct pal_in4_addr *nbr_id,
                                         int indexlen, int *ret,
                                         u_int32_t vr_id)
{
  struct ospf_vlink *vlink;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      vlink = ospf_vlink_entry_lookup_next (top, area_id, nbr_id, indexlen);
      if (vlink)
        {
          *ret = OSPF_API_FALSE;
          return OSPF_API_GET_SUCCESS;
        }
    }

  return OSPF_API_GET_ERROR;
}


/* Get functions for ospfExtLsdbEntry. */
int
ospf_get_ext_lsdb_type (int proc_id, int type, struct pal_in4_addr lsid,
                        struct pal_in4_addr router_id, int *ret,
                        u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup (top, type, lsid, router_id);
      if (lsa)
        {
          *ret = lsa->data->type;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_ext_lsdb_lsid (int proc_id, int type, struct pal_in4_addr lsid,
                        struct pal_in4_addr router_id,
                        struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup (top, type, lsid, router_id);
      if (lsa)
        {
          *ret = lsa->data->id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_ext_lsdb_router_id (int proc_id, int type, struct pal_in4_addr lsid,
                             struct pal_in4_addr router_id,
                             struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup (top, type, lsid, router_id);
      if (lsa)
        {
          *ret = lsa->data->adv_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_ext_lsdb_sequence (int proc_id, int type, struct pal_in4_addr lsid,
                            struct pal_in4_addr router_id, int *ret,
                            u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup (top, type, lsid, router_id);
      if (lsa)
        {
          *ret = pal_ntoh32 (lsa->data->ls_seqnum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_ext_lsdb_age (int proc_id, int type, struct pal_in4_addr lsid,
                       struct pal_in4_addr router_id, int *ret,
                       u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup (top, type, lsid, router_id);
      if (lsa)
        {
          *ret = LS_AGE (lsa);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_ext_lsdb_checksum (int proc_id, int type, struct pal_in4_addr lsid,
                            struct pal_in4_addr router_id, int *ret,
                            u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup (top, type, lsid, router_id);
      if (lsa)
        {
          *ret = pal_ntoh16 (lsa->data->checksum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_ext_lsdb_advertisement (int proc_id, int type,
                                 struct pal_in4_addr lsid,
                                 struct pal_in4_addr router_id, u_char **ret,
                                 size_t *size, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup (top, type, lsid, router_id);
      if (lsa)
        {
          *ret = (u_char *)lsa->data;
          *size = pal_ntoh16 (lsa->data->length);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfExtLsdbEntry. */
int
ospf_get_next_ext_lsdb_type (int proc_id, int *type, struct pal_in4_addr *lsid,
                             struct pal_in4_addr *router_id,
                             int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup_next (top, type, lsid, router_id,
                                             indexlen);
      if (lsa)
        {
          *ret = lsa->data->type;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_ext_lsdb_lsid (int proc_id, int *type, struct pal_in4_addr *lsid,
                             struct pal_in4_addr *router_id, int indexlen,
                             struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup_next (top, type, lsid, router_id,
                                             indexlen);
      if (lsa)
        {
          *ret = lsa->data->id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_ext_lsdb_router_id (int proc_id, int *type,
                                  struct pal_in4_addr *lsid,
                                  struct pal_in4_addr *router_id, int indexlen,
                                  struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup_next (top, type, lsid, router_id,
                                             indexlen);
      if (lsa)
        {
          *ret = lsa->data->adv_router;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_ext_lsdb_sequence (int proc_id, int *type,
                                 struct pal_in4_addr *lsid,
                                 struct pal_in4_addr *router_id, int indexlen,
                                 int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup_next (top, type, lsid, router_id,
                                             indexlen);
      if (lsa)
        {
          *ret = pal_ntoh32 (lsa->data->ls_seqnum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_ext_lsdb_age (int proc_id, int *type, struct pal_in4_addr *lsid,
                            struct pal_in4_addr *router_id,
                            int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup_next (top, type, lsid, router_id,
                                             indexlen);
      if (lsa)
        {
          *ret = LS_AGE (lsa);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_ext_lsdb_checksum (int proc_id, int *type,
                                 struct pal_in4_addr *lsid,
                                 struct pal_in4_addr *router_id, int indexlen,
                                 int *ret, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup_next (top, type, lsid, router_id,
                                             indexlen);
      if (lsa)
        {
          *ret = pal_ntoh16 (lsa->data->checksum);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_ext_lsdb_advertisement (int proc_id, int *type,
                                      struct pal_in4_addr *lsid,
                                      struct pal_in4_addr *router_id,
                                      int indexlen, u_char **ret,
                                      size_t *size, u_int32_t vr_id)
{
  struct ospf_lsa *lsa;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      lsa = ospf_ext_lsdb_entry_lookup_next (top, type, lsid, router_id,
                                             indexlen);
      if (lsa)
        {
          *ret = (u_char *)lsa->data;
          *size = pal_ntoh16 (lsa->data->length);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}


/* Get functions for ospfAreaAggregateEntry. */
int
ospf_get_area_aggregate_area_id (int proc_id, struct pal_in4_addr area_id,
                                 int type, struct pal_in4_addr addr,
                                 struct pal_in4_addr mask,
                                 struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (range)
        {
          *ret = area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_aggregate_lsdb_type (int proc_id, struct pal_in4_addr area_id,
                                   int type, struct pal_in4_addr addr,
                                   struct pal_in4_addr mask, int *ret,
                                   u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (range)
        {
          *ret = OSPF_API_AREA_AGGREGATE_TYPE_SUMMARY;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_aggregate_net (int proc_id, struct pal_in4_addr area_id,
                             int type, struct pal_in4_addr addr,
                             struct pal_in4_addr mask,
                             struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (range)
        {
          pal_mem_cpy (ret, range->lp->prefix, sizeof (struct pal_in4_addr));
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_aggregate_mask (int proc_id, struct pal_in4_addr area_id,
                              int type, struct pal_in4_addr addr,
                              struct pal_in4_addr mask,
                              struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (range)
        {
          masklen2ip (range->lp->prefixlen, ret);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_aggregate_status (int proc_id, struct pal_in4_addr area_id,
                                int type, struct pal_in4_addr addr,
                                struct pal_in4_addr mask, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (range != NULL)
        {
          *ret = range->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_area_aggregate_effect (int proc_id, struct pal_in4_addr area_id,
                                int type, struct pal_in4_addr addr,
                                struct pal_in4_addr mask, int *ret,
                                u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (range)
        {
          if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
            *ret = OSPF_API_advertiseMatching;
          else
            *ret = OSPF_API_doNotAdvertiseMatching;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* GetNext functions for ospfAreaAggregateEntry. */
int
ospf_get_next_area_aggregate_area_id (int proc_id,
                                      struct pal_in4_addr *area_id,
                                      int *type, struct pal_in4_addr *addr,
                                      struct pal_in4_addr *mask, int indexlen,
                                      struct pal_in4_addr *ret,
                                      u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup_next (top, area_id, type,
                                                     addr, mask, indexlen,
                                                     vr_id);
      if (range)
        {
          *ret = *area_id;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_aggregate_lsdb_type (int proc_id,
                                        struct pal_in4_addr *area_id,
                                        int *type, struct pal_in4_addr *addr,
                                        struct pal_in4_addr *mask,
                                        int indexlen, int *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup_next (top, area_id, type,
                                                     addr, mask, indexlen,
                                                     vr_id);
      if (range)
        {
          *ret = OSPF_API_AREA_AGGREGATE_TYPE_SUMMARY;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_aggregate_net (int proc_id, struct pal_in4_addr *area_id,
                                  int *type, struct pal_in4_addr *addr,
                                  struct pal_in4_addr *mask, int indexlen,
                                  struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup_next (top, area_id, type,
                                                     addr, mask, indexlen,
                                                     vr_id);
      if (range)
        {
          pal_mem_cpy (ret, range->lp->prefix, sizeof (struct pal_in4_addr));
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_aggregate_mask (int proc_id, struct pal_in4_addr *area_id,
                                   int *type, struct pal_in4_addr *addr,
                                   struct pal_in4_addr *mask, int indexlen,
                                   struct pal_in4_addr *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup_next (top, area_id, type,
                                                     addr, mask, indexlen,
                                                     vr_id);
      if (range)
        {
          masklen2ip (range->lp->prefixlen, ret);
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_aggregate_status (int proc_id, struct pal_in4_addr *area_id,
                                     int *type, struct pal_in4_addr *addr,
                                     struct pal_in4_addr *mask, int indexlen,
                                     int *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup_next (top, area_id, type,
                                                     addr, mask, indexlen,
                                                     vr_id);
      if (range)
        {
          *ret = range->status;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

int
ospf_get_next_area_aggregate_effect (int proc_id, struct pal_in4_addr *area_id,
                                     int *type, struct pal_in4_addr *addr,
                                     struct pal_in4_addr *mask, int indexlen,
                                     int *ret, u_int32_t vr_id)
{
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup_next (top, area_id, type,
                                                     addr, mask, indexlen,
                                                     vr_id);
      if (range)
        {
          if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE))
            *ret = OSPF_API_advertiseMatching;
          else
            *ret = OSPF_API_doNotAdvertiseMatching;
          return OSPF_API_GET_SUCCESS;
        }
    }
  return OSPF_API_GET_ERROR;
}

/* Set functions for ospfAreaAggregateEntry. */
int
ospf_set_area_aggregate_status (int proc_id, struct pal_in4_addr area_id,
                                int type, struct pal_in4_addr addr,
                                struct pal_in4_addr mask, int status,
                                u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_area_range *range;
  struct ospf_area *area;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      area = ospf_area_entry_lookup (top, area_id);
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (area && range)
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
              if (range->status != ROW_STATUS_ACTIVE)
                return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
              break;
            case ROW_STATUS_CREATEANDGO:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_DESTROY:
              return ospf_area_range_unset (vr->id, top->ospf_id, area_id,
                                            addr, range->lp->prefixlen);
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }

          return OSPF_API_SET_SUCCESS;
        }
      /* Row doesn't exists. */
      else
        {
          switch (status)
            {
            case ROW_STATUS_ACTIVE:
            case ROW_STATUS_NOTINSERVICE:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDWAIT:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            case ROW_STATUS_DESTROY:
              return OSPF_API_SET_ERR_INCONSISTENT_VALUE;
            case ROW_STATUS_CREATEANDGO:
              ospf_area_range_set (vr->id, top->ospf_id, area_id, addr,
                                   ip_masklen (mask));
              break;
            default:
              return OSPF_API_SET_ERR_WRONG_VALUE;
            }
        }
    }

  return OSPF_API_SET_SUCCESS;
}

int
ospf_set_area_aggregate_effect (int proc_id, struct pal_in4_addr area_id,
                                int type, struct pal_in4_addr addr,
                                struct pal_in4_addr mask, int effect,
                                u_int32_t vr_id)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_area_range *range;
  struct ospf *top;

  top = ospf_proc_lookup_any (proc_id, vr_id);
  if (top)
    {
      range = ospf_area_aggregate_entry_lookup (top, area_id, type,
                                                addr, mask);
      if (range)
        {
          if (effect != OSPF_API_doNotAdvertiseMatching
              && effect != OSPF_API_advertiseMatching)
            return OSPF_API_SET_ERR_WRONG_VALUE;

          else if (CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE)
                   && effect == OSPF_API_doNotAdvertiseMatching)
            ospf_area_range_not_advertise_set (vr->id, top->ospf_id,
                                               area_id, addr,
                                               range->lp->prefixlen);

          else if (!CHECK_FLAG (range->flags, OSPF_AREA_RANGE_ADVERTISE)
                   && effect == OSPF_API_advertiseMatching)
            ospf_area_range_not_advertise_unset (vr->id, top->ospf_id,
                                                 area_id, addr,
                                                 range->lp->prefixlen);

          return OSPF_API_SET_SUCCESS;
        }
    }

  return OSPF_API_SET_SUCCESS;
}

/* SNMP TRAP Callback API. */
#ifdef HAVE_SNMP
/* Set OSPF snmp trap callback function.
   If trapid == OSPF_TRAP_ALL
   register all supported trap. */
int
ospf_trap_callback_set (int trapid, SNMP_TRAP_CALLBACK func)
{
  struct ospf_master *om = ospf_master_lookup_default (ZG);
  int i, j, found;
  vector v;
  
  /* Null check */
  if (om == NULL)
     return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (trapid < OSPF_TRAP_ALL || trapid > OSPF_TRAP_ID_MAX)
    return OSPF_API_SET_ERROR;

  if (trapid == OSPF_TRAP_ALL)
    {
      for (i = 0; i < OSPF_TRAP_ID_MAX; i++)
        {
          found = 0;
          v = om->traps[i];

          for (j = 0; j < vector_max (v); j++)
            if (vector_slot (v, j) == func)
              {
                found = 1;
                break;
              }

          if (found == 0)
            vector_set (v, func);
        }
      return OSPF_API_SET_SUCCESS;
    }

  /* Get trap callback vector. */
  v = om->traps[trapid - 1];
  for (j = 0; j < vector_max (v); j++)
    if (vector_slot (v, j) == func)
      return OSPF_API_SET_SUCCESS;

  vector_set (v, func);

  return OSPF_API_SET_SUCCESS;
}

int
ospf_trap_callback_unset (int trapid, SNMP_TRAP_CALLBACK func)
{
  struct ospf_master *om = ospf_master_lookup_default (ZG);
  int i, j;
  vector v;
  
  /* NULL check on om */
  if (om == NULL)
     return OSPF_API_SET_ERR_VR_NOT_EXIST;

  if (trapid == OSPF_TRAP_ALL)
    {
      for (i = 0; i < OSPF_TRAP_ID_MAX; i++)
        {
          v = om->traps[i];

          for (j = 0; j < vector_max (v); j++)
            if (vector_slot (v, j) == func)
              vector_unset (v, j);
        }
      return OSPF_API_SET_SUCCESS;
    }

  if (trapid < OSPF_TRAP_ALL || trapid > OSPF_TRAP_ID_MAX)
    return OSPF_API_SET_ERROR;

  /* Get trap callback vector. */
  v = om->traps[trapid - 1];

  for (i = 0; i < vector_max (v); i++)
    if (vector_slot (v, i) == func)
      {
        vector_unset (v, i);
        return OSPF_API_SET_SUCCESS;
      }
  return OSPF_API_SET_ERROR;
}
#endif /* HAVE_SNMP */
