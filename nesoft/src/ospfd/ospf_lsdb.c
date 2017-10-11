/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"

#ifdef HAVE_OSPF_CSPF
#include "table.h"
#include "cspf_common.h"
#endif /* HAVE_OSPF_CSPF */

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_api.h"
#ifdef HAVE_OSPF_CSPF
#include "ospfd/ospf_te.h"
#include "ospfd/cspf/ospf_cspf.h"
#endif /* HAVE_OSPF_CSPF */
#include "ptree.h"

struct ospf_lsdb *
ospf_lsdb_new (void)
{
  struct ospf_lsdb *new;

  new = XMALLOC (MTYPE_OSPF_LSDB, sizeof (struct ospf_lsdb));
  pal_mem_set (new, 0, sizeof (struct ospf_lsdb));

  return new;
}

void
ospf_lsdb_free (struct ospf_lsdb *lsdb)
{
  OSPF_TIMER_OFF (lsdb->t_lsa_walker);
#ifdef HAVE_OPAQUE_LSA
  OSPF_TIMER_OFF (lsdb->t_opaque_lsa);
#endif /* HAVE_OPAQUE_LSA */

  XFREE (MTYPE_OSPF_LSDB, lsdb);
}


unsigned long
ospf_lsdb_count (struct ospf_lsdb *lsdb, int type)
{
  if (lsdb->type[type].db)
    return lsdb->type[type].db->count[RNI_LSDB_LSA];

  return 0;
}

unsigned long
ospf_lsdb_count_all (struct ospf_lsdb *lsdb)
{
  unsigned long sum = 0;
  int i;

  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    if (lsdb->type[i].db)
      sum += lsdb->type[i].db->count[RNI_LSDB_LSA];

  return sum;
}

#ifdef HAVE_OSPF_DB_OVERFLOW
unsigned long
ospf_lsdb_count_external_non_default (struct ospf *top)
{
  struct ls_table *table = top->lsdb->type[OSPF_AS_EXTERNAL_LSA].db;
  unsigned long count = 0;

  if (table != NULL)
    {
      struct ospf_lsa *lsa;
      struct ls_node *rn, *rn_tmp;
      struct ls_prefix_ls lp;

      count = table->count[RNI_LSDB_LSA];

      pal_mem_set (&lp, 0, sizeof (struct ls_prefix_ls));
      lp.prefixsize = OSPF_LSDB_LOWER_TABLE_DEPTH;
      lp.prefixlen = 32;

      rn_tmp = ls_node_get (table, (struct ls_prefix *)&lp);
      for (rn = ls_lock_node (rn_tmp);
           rn; rn = ls_route_next_until (rn, rn_tmp))
        if ((lsa = RN_INFO (rn, RNI_DEFAULT)))
          count--;
      ls_unlock_node (rn_tmp);
    }

  return count;
}
#endif /* HAVE_OSPF_DB_OVERFLOW */

u_int32_t 
ospf_proc_lsdb_count (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;
  u_int32_t lsa_counter = 0;
#ifdef HAVE_OPAQUE_LSA
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_OPAQUE_LSA
  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      lsa_counter += ospf_lsdb_count_all (oi->lsdb);

  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      if ((oi = vlink->oi))
        lsa_counter += ospf_lsdb_count_all (oi->lsdb);

#ifdef HAVE_OSPF_MULTI_AREA
  for (rn = ls_table_top (top->multi_area_link_table); rn;
                                   rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        lsa_counter += ospf_lsdb_count_all (mlink->oi->lsdb);
#endif /* HAVE_OSPF_MULTI_AREA */

#endif /* HAVE_OPAQUE_LSA */
  
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        lsa_counter += ospf_lsdb_count_all (area->lsdb);

  lsa_counter += ospf_lsdb_count_all (top->lsdb);

  return lsa_counter;
}

unsigned long
ospf_lsdb_isempty (struct ospf_lsdb *lsdb)
{
  return ((ospf_lsdb_count_all (lsdb)) == 0);
}

u_int32_t
ospf_lsdb_checksum (struct ospf_lsdb *lsdb, int type)
{
  return lsdb->type[type].checksum;
}

u_int32_t
ospf_lsdb_checksum_all (struct ospf_lsdb *lsdb)
{
  u_int32_t sum = 0;
  int i;

  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    if (lsdb->type[i].db)
      sum += lsdb->type[i].checksum;

  return sum;
}


int
ospf_lsdb_entry_insert (struct ospf *top, struct ospf_area *area,
                        int type, struct ls_table *table)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  int ret = OSPF_API_ENTRY_INSERT_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_LSDB_UPPER_TABLE_DEPTH;
  p.prefixlen = OSPF_LSDB_UPPER_TABLE_DEPTH * 8;

  /* Set Area ID and type as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_LSDB_UPPER_TABLE].vars,
                      &area->area_id, &type);
  rn = ls_node_get (top->lsdb_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_DEFAULT, table);
      ret = OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  ls_unlock_node (rn);
  return ret;
}

int
ospf_lsdb_entry_delete (struct ospf *top, struct ospf_area *area,
                        int type, struct ls_table *table)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  int ret = OSPF_API_ENTRY_DELETE_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_LSDB_UPPER_TABLE_DEPTH;
  p.prefixlen = OSPF_LSDB_UPPER_TABLE_DEPTH * 8;

  /* Set Area ID as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_LSDB_UPPER_TABLE].vars,
                      &area->area_id, &type);
  rn = ls_node_lookup (top->lsdb_table, (struct ls_prefix *)&p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      ret = OSPF_API_ENTRY_DELETE_SUCCESS;
    }
  return ret;
}

void
ospf_lsdb_entry_delete_area (struct ospf_area *area)
{
  int i;

  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    if (area->lsdb->type[i].db)
      ospf_lsdb_entry_delete (area->top, area, i, area->lsdb->type[i].db);
}


void
ospf_lsdb_list_add (struct ospf_lsdb *lsdb, struct ospf_lsa *lsa)
{
  struct ospf_lsdb_slot *slot = &lsdb->type[lsa->data->type];

  /* LSDB scope should be set.  */
  if (lsdb->scope == LSA_FLOOD_SCOPE_UNKNOWN)
    return;

  /* Update the checksum.  */
  slot->checksum += pal_ntoh16 (lsa->data->checksum);

  /* Set the pointer.  */
  lsa->prev = slot->tail;
  lsa->next = NULL;

  /* Update the linked-list.  */
  if (slot->tail != NULL)
    slot->tail->next = lsa;
  else
    slot->head = lsa;
  slot->tail = lsa;
}

void
ospf_lsdb_list_delete (struct ospf_lsdb *lsdb, struct ospf_lsa *lsa)
{
  struct ospf_lsdb_slot *slot = &lsdb->type[lsa->data->type];

  /* LSDB scope should be set.  */
  if (lsdb->scope == LSA_FLOOD_SCOPE_UNKNOWN)
    return;

  /* Update the checksum.  */
  slot->checksum -= pal_ntoh16 (lsa->data->checksum);

  /* Update the linked-list.  */
  if (lsa->prev != NULL)
    lsa->prev->next = lsa->next;
  else
    slot->head = lsa->next;
  if (lsa->next != NULL)
    lsa->next->prev = lsa->prev;
  else
    slot->tail = lsa->prev;

  /* Update the next candidate.  */
  if (slot->next == lsa)
    slot->next = lsa->next;

  /* Reset the pointer.  */
  lsa->prev = NULL;
  lsa->next = NULL;
}

void
lsdb_prefix_set (struct ls_prefix_ls *lp, struct lsa_header *lsah)
{
  pal_mem_set (lp, 0, sizeof (struct ls_prefix_ls));
  lp->family = 0;
  lp->prefixsize = OSPF_LSDB_LOWER_TABLE_DEPTH;
  lp->prefixlen = OSPF_LSDB_LOWER_TABLE_DEPTH * 8;
  lp->id = lsah->id;
  lp->adv_router = lsah->adv_router;
}

/* Add new LSA to lsdb. */
void
ospf_lsdb_add (struct ospf_lsdb *lsdb, struct ospf_lsa *lsa)
{
  struct ls_table *table = lsdb->type[lsa->data->type].db;
  struct ospf_lsa *old;
  struct ls_node *rn;
  struct ls_prefix_ls lp;
  struct ospf *top;
  u_int16_t interval;
  struct pal_timeval now;

  lsdb_prefix_set (&lp, lsa->data);
  rn = ls_node_get (table, (struct ls_prefix *)&lp);
  if ((old = RN_INFO (rn, RNI_LSDB_LSA)) != NULL)
    {
      if (old == lsa)
        {
          ls_unlock_node (rn);
          return;
        }
      ospf_lsdb_list_delete (lsdb, old);
      ls_node_info_unset (rn, RNI_LSDB_LSA);
      ospf_lsa_unlock (old);
    }

  ls_node_info_set (rn, RNI_LSDB_LSA, ospf_lsa_lock (lsa));
  ospf_lsdb_list_add (lsdb, lsa);
  pal_time_tzcurrent (&now, NULL);

  top = ospf_proc_lookup_by_lsdb (lsdb);
  if (top != NULL)
    {
      if (!CHECK_FLAG (top->flags, OSPF_PROC_DESTROY))
        {
          if (IS_LSA_MAXAGE (lsa))
            {
              OSPF_TIMER_OFF (lsdb->t_lsa_walker);
              OSPF_TIMER_ON (lsdb->t_lsa_walker, ospf_lsa_maxage_walker,
                             lsdb, 1);
            }
          else if (!lsdb->max_lsa_age ||
                   (LS_AGE (lsa) > (lsdb->max_lsa_age + 
                    TV_FLOOR (TV_SUB (now, lsdb->tv_maxage)))))
            {            
	      lsdb->max_lsa_age = LS_AGE (lsa);            
	      pal_time_tzcurrent (&lsdb->tv_maxage, NULL);        
              interval = OSPF_LSA_MAXAGE - lsdb->max_lsa_age;
  
              if (interval < OSPF_LSA_MAXAGE_INTERVAL_DEFAULT)
                interval = OSPF_LSA_MAXAGE_INTERVAL_DEFAULT;

              OSPF_TIMER_OFF (lsdb->t_lsa_walker);
              OSPF_TIMER_ON (lsdb->t_lsa_walker, ospf_lsa_maxage_walker,
                             lsdb, interval);
            }
          else
           { 
             OSPF_TIMER_ON (lsdb->t_lsa_walker, ospf_lsa_maxage_walker,
                            lsdb, OSPF_LSA_MAXAGE_INTERVAL_DEFAULT);
           }
        }

#ifdef HAVE_OSPF_CSPF
      if (lsdb->scope == LSA_FLOOD_SCOPE_AREA)
        ospf_te_process_lsa_add (top->om, lsdb->te_lsdb, lsa);
#endif /* HAVE_OSPF_CSPF */
    }

  ls_unlock_node (rn);
}

void
ospf_lsdb_delete (struct ospf_lsdb *lsdb, struct ospf_lsa *lsa)
{
  struct ls_table *table = lsdb->type[lsa->data->type].db;
  struct ospf *top = ospf_proc_lookup_by_lsdb (lsdb);
  struct ls_prefix_ls lp;
  struct ls_node *rn;

#ifdef HAVE_NSSA
  if (top != NULL)
    {
      /* If Type-7 LSA is deleted, flushed the translated Type-5
         LSA if have.  */
      if (lsa->data->type == OSPF_AS_NSSA_LSA)
        {
          if (lsa->lsdb->u.area->translator_state == OSPF_NSSA_TRANSLATOR_DISABLED)
            OSPF_TIMER_OFF (lsa->lsdb->u.area->t_nssa); /* Cancel the stability Timer */
          ospf_redist_map_nssa_delete (lsa);
        }
    }
#endif /* HAVE_NSSA */

  lsdb_prefix_set (&lp, lsa->data);
  rn = ls_node_info_lookup (table, (struct ls_prefix *)&lp, RNI_LSDB_LSA);
  if (rn)
    if (RN_INFO (rn, RNI_LSDB_LSA) == lsa)
      {
        ospf_lsdb_list_delete (lsdb, lsa);
        ls_node_info_unset (rn, RNI_LSDB_LSA);
        ls_unlock_node (rn);
#ifdef HAVE_OSPF_CSPF
        if (top && lsdb->scope == LSA_FLOOD_SCOPE_AREA)
          ospf_te_process_lsa_delete (top->om, lsdb->te_lsdb, lsa);
#endif /* HAVE_OSPF_CSPF */
        ospf_lsa_unlock (lsa);

        if (top && CHECK_FLAG (top->config, OSPF_CONFIG_OVERFLOW_LSDB_LIMIT))
          {
            u_int32_t lsa_count = ospf_proc_lsdb_count (top);
            if (lsa_count < (top->lsdb_overflow_limit *
                             OSPF_LSDB_OVERFLOW_LIMIT_OSCILATE_RANGE)
                && CHECK_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT)
                && top->lsdb_overflow_limit_type == 
                   OSPF_LSDB_OVERFLOW_SOFT_LIMIT)
              {
                UNSET_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT);
                zlog_info (ZG, "OSPF LSDB below overflow limit\n");
              }
          }
        return;
      }
}

struct ospf_lsa *
ospf_lsdb_lookup (struct ospf_lsdb *lsdb, struct ospf_lsa *lsa)
{
  struct ospf_lsa *find;
  struct ls_table *table;
  struct ls_prefix_ls lp;
  struct ls_node *rn;

  table = lsdb->type[lsa->data->type].db;
  lsdb_prefix_set (&lp, lsa->data);
  rn = ls_node_info_lookup (table, (struct ls_prefix *)&lp, RNI_LSDB_LSA);
  if (rn)
    {
      find = RN_INFO (rn, RNI_LSDB_LSA);
      ls_unlock_node (rn);
      return find;
    }
  return NULL;
}

struct ospf_lsa *
ospf_lsdb_lookup_by_id (struct ospf_lsdb *lsdb, u_char type,
                        struct pal_in4_addr id, struct pal_in4_addr adv_router)
{
  struct ls_table *table;
  struct ls_prefix_ls lp;
  struct ls_node *rn;
  struct ospf_lsa *find;

  table = lsdb->type[type].db;

  pal_mem_set (&lp, 0, sizeof (struct ls_prefix_ls));
  lp.prefixsize = OSPF_LSDB_LOWER_TABLE_DEPTH;
  lp.prefixlen = OSPF_LSDB_LOWER_TABLE_DEPTH * 8;
  lp.id = id;
  lp.adv_router = adv_router;

  rn = ls_node_info_lookup (table, (struct ls_prefix *) &lp, RNI_LSDB_LSA);
  if (rn)
    {
      find = RN_INFO (rn, RNI_LSDB_LSA);
      ls_unlock_node (rn);
      return find;
    }
  return NULL;
}

struct ospf_lsdb *
ospf_lsdb_get_by_type (struct ospf_interface *oi, u_char type)
{
  switch (LSA_FLOOD_SCOPE (type))
    {
#ifdef HAVE_OPAQUE_LSA
    case LSA_FLOOD_SCOPE_LINK:
      return oi->lsdb;
      break;
#endif /* HAVE_OPAQUE_LSA */
    case LSA_FLOOD_SCOPE_AREA:
      return oi->area->lsdb;
      break;
    case LSA_FLOOD_SCOPE_AS:
      return oi->top->lsdb;
      break;
    }
  return NULL;
}

void
ospf_lsdb_lsa_discard_apply (struct ospf *top,
                             struct ospf_lsdb *lsdb, struct ospf_lsa *lsa)
{
  struct ospf_lsa *old;

  old = ospf_lsdb_lookup (lsdb, lsa);
  if (old == NULL)
    return;

  ospf_ls_retransmit_delete_nbr_by_scope (old, PAL_FALSE);

  /* Update the incremental update LSA vector.  */
  if (old->data->type == OSPF_SUMMARY_LSA)
    ospf_route_path_summary_lsa_unset (top, old);

  else if (old->data->type == OSPF_SUMMARY_LSA_ASBR)
    ospf_route_path_asbr_summary_lsa_unset (top, old);

  else if (old->data->type == OSPF_AS_EXTERNAL_LSA)
    ospf_route_path_external_lsa_unset (top, old);

#ifdef HAVE_NSSA
  else if (old->data->type == OSPF_AS_NSSA_LSA)
    ospf_route_path_external_lsa_unset (top, old);
#endif /* HAVE_NSSA */

  ospf_lsa_discard (old);
}

void
ospf_lsdb_lsa_discard (struct ospf_lsdb *lsdb, struct ospf_lsa *lsa,
                       int flush, int discard, int delete)
{
  struct ospf *top = ospf_proc_lookup_by_lsdb (lsdb);

  if (IS_LSA_SELF (lsa))
    {
      ospf_lsa_refresh_queue_unset (lsa);
      ospf_lsa_refresh_event_unset (lsa);

      if (flush && top)
        ospf_flood_lsa_flush (top, lsa);
    }

  if (discard && top)
    ospf_lsdb_lsa_discard_apply (top, lsdb, lsa);

  if (delete)
    ospf_lsdb_delete (lsdb, lsa);
}

void
ospf_lsdb_lsa_discard_by_type (struct ospf_lsdb *lsdb, int type,
                               int flush, int discard, int delete)
{
  struct ospf_lsa *lsa;
  struct ls_node *rn;

  LSDB_LOOP (&lsdb->type[type], rn, lsa)
    ospf_lsdb_lsa_discard (lsdb, lsa, flush, discard, delete);
}

void
ospf_lsdb_lsa_discard_all (struct ospf_lsdb *lsdb,
                           int flush, int discard, int delete)
{
  struct ospf_lsa *lsa;
  struct ls_node *rn;
  int type;

  for (type = OSPF_MIN_LSA; type < OSPF_MAX_LSA; type++)
    if (lsdb->scope == LSA_FLOOD_SCOPE (type))
      LSDB_LOOP (&lsdb->type[type], rn, lsa)
        ospf_lsdb_lsa_discard (lsdb, lsa, flush, discard, delete);
}


#ifdef HAVE_OPAQUE_LSA
void
ospf_opaque_lsa_update_all (struct ospf *top, struct ospf_lsdb *lsdb)
{
  struct ospf_opaque_map *map;
  struct ospf_lsa *old;
  struct ls_node *rn;
  u_char type = OPAQUE_LSA_TYPE_BY_SCOPE (lsdb->scope);

  for (rn = ls_table_top (lsdb->opaque_table); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (map->flags, OSPF_OPAQUE_MAP_UPDATED))
        {
          struct pal_in4_addr *id = (struct pal_in4_addr *)rn->p->prefix;

          old = ospf_lsdb_lookup_by_id (lsdb, type, *id, top->router_id);
          if (old == NULL || IS_LSA_MAXAGE (old))
            ospf_lsa_originate (top, type, map);
          else
            {
              if (old->param == NULL)
                old->param = map;

              ospf_lsa_refresh (top, old);
            }

          UNSET_FLAG (map->flags, OSPF_OPAQUE_MAP_UPDATED);
        }
}

int
ospf_opaque_lsa_timer (struct thread *thread)
{
  struct ospf_lsdb *lsdb = THREAD_ARG (thread);
  struct ospf *top;
  struct ospf_master *om;

  lsdb->t_opaque_lsa = NULL;

  top = ospf_proc_lookup_by_lsdb (lsdb);
  if (top != NULL)
    {
      om = top->om;
      /* Set VR context*/
      LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

      ospf_opaque_lsa_update_all (top, lsdb);
    }
  return 0;
}

void
ospf_opaque_lsa_timer_add (struct ospf_lsdb *lsdb)
{
  struct ospf *top;

  top = ospf_proc_lookup_by_lsdb (lsdb);
  if (top != NULL)
    if (IS_OPAQUE_CAPABLE (top))
      {      
        if (!IS_PROC_ACTIVE (top))
          return;

        if (lsdb->scope == LSA_FLOOD_SCOPE_AREA)
          if (!IS_AREA_ACTIVE (lsdb->u.area))
            return;

        if (lsdb->scope == LSA_FLOOD_SCOPE_LINK)
          if (CHECK_FLAG (lsdb->u.oi->flags, OSPF_IF_DESTROY))
            return;

        OSPF_TIMER_ON (lsdb->t_opaque_lsa, ospf_opaque_lsa_timer,
                       lsdb, OSPF_OPAQUE_LSA_GENERATE_DELAY);
      }
}

void
ospf_opaque_lsa_redistribute (struct ospf_lsdb *lsdb, struct pal_in4_addr id,
                              void *data, u_int16_t length)
{
  struct ospf_opaque_map *map = NULL;
  struct ls_node *rn;
  struct ls_prefix lp;

  ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN, id);

  rn = ls_node_get (lsdb->opaque_table, &lp);
  map = RN_INFO (rn, RNI_DEFAULT);
  if (data != NULL)
    {
      if (map == NULL)
        {
          map = XCALLOC (MTYPE_OSPF_OPAQUE_MAP, OSPF_OPAQUE_MAP_LEN (length));
          map->lsdb = lsdb;
          map->id = id;
          RN_INFO_SET (rn, RNI_DEFAULT, map);
        }
      else
        {
          if (length > map->length)
            {
              map = XREALLOC (MTYPE_OSPF_OPAQUE_MAP, map,
                              OSPF_OPAQUE_MAP_LEN (length));
              RN_INFO_SET (rn, RNI_DEFAULT, map);

              if (map->lsa)
                map->lsa->param = map;
            }
        }

      map->length = length;
      pal_mem_cpy (map->data, data, length);
      SET_FLAG (map->flags, OSPF_OPAQUE_MAP_UPDATED);
      ospf_opaque_lsa_timer_add (lsdb);
    }
  else
    {
      if (map != NULL)
        {
          LSA_MAP_FLUSH (map->lsa);
          XFREE (MTYPE_OSPF_OPAQUE_MAP, map);
          RN_INFO_UNSET (rn, RNI_DEFAULT);
        }
    }

  ls_unlock_node (rn);
}

struct ospf_opaque_map *
ospf_opaque_map_lookup (struct ospf_lsdb *lsdb, struct pal_in4_addr id)
{
  struct ls_node *rn;
  struct ls_prefix lp;

  ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN, id);
  rn = ls_node_lookup (lsdb->opaque_table, &lp);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

void
ospf_opaque_map_delete_all (struct ls_table *rt)
{
  struct ospf_opaque_map *map;
  struct ls_node *rn;

  for (rn = ls_table_top (rt); rn; rn = ls_route_next (rn))
    if ((map = RN_INFO (rn, RNI_DEFAULT)))
      {
        if (map->lsa)
          ospf_lsa_unlock (map->lsa);

        XFREE (MTYPE_OSPF_OPAQUE_MAP, map);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
}
#endif /* HAVE_OPAQUE_LSA */


#ifdef HAVE_OSPF_CSPF
struct te_lsa_node *
ospf_te_lsa_node_new ()
{
  struct te_lsa_node *node;

  node = XMALLOC (MTYPE_TE_LSA_NODE, sizeof (struct te_lsa_node));
  pal_mem_set (node, 0, sizeof (struct te_lsa_node));

  node->link_lsa_list = list_new ();

  return node;
}

void
ospf_te_lsa_node_free (struct te_lsa_node *node)
{
  struct ospf_lsa *lsa;

  if (! node)
    return;

  if (node->rtaddr_lsa)
    {
      lsa = node->rtaddr_lsa;
      ospf_te_lsa_free (lsa);
    }

  if (node->link_lsa_list)
    {
      struct listnode *ln, *ln_next;
      struct list *list;

      list = node->link_lsa_list;

      for (ln = list->head; ln; ln = ln_next)
        {
          ln_next = ln->next;

          lsa = ln->data;
          ospf_te_lsa_free (lsa);

          list_delete_node (list, ln);
        }

      list_delete (list);
    }

  XFREE (MTYPE_TE_LSA_NODE, node);
}

struct listnode *
ospf_te_link_lsa_lookup_by_instance (struct te_lsa_node *te_lsa_node,
                                     struct pal_in4_addr *id)
{
  struct ospf_lsa *lsa;
  struct listnode *ln;


  LIST_LOOP (te_lsa_node->link_lsa_list, lsa, ln)
    if (IPV4_ADDR_SAME (id, &lsa->data->id))
      return ln;

  return NULL;
}


void
ospf_te_lsdb_add (struct ptree *te_lsdb, struct ospf_lsa *lsa)
{
  struct te_lsa_node *te_lsa_node = NULL;
  struct ospf_te_lsa_data *te_lsa;
  struct ptree_node *rn;

  rn = ospf_te_lsa_node_lookup (te_lsdb, lsa->data->adv_router);
  if (! rn)
    {
      rn = ptree_node_get (te_lsdb, (u_char *)&lsa->data->adv_router, 
                           IPV4_MAX_BITLEN);
      if (rn == NULL)
        return;

      te_lsa_node = ospf_te_lsa_node_new ();
      rn->info = te_lsa_node;
    }
  else
    {
      te_lsa_node = rn->info;
    }

  te_lsa  = (struct ospf_te_lsa_data *)(lsa->data);

  if (te_lsa->type == TE_TLV_ROUTER_TYPE)
    {
      if (te_lsa_node->rtaddr_lsa)
        ospf_te_lsa_free (te_lsa_node->rtaddr_lsa);

      te_lsa_node->rtaddr_lsa = lsa;
    }
  else
    {
      struct pal_in4_addr *id;
      struct listnode *ln;
      struct ospf_lsa *tmp;

      id = &lsa->data->id;

      ln = ospf_te_link_lsa_lookup_by_instance (te_lsa_node, id);
      if (ln)
        {
          tmp = GETDATA (ln);
          list_delete_node (te_lsa_node->link_lsa_list, ln);
          ospf_te_lsa_free (tmp);
        }

      listnode_add (te_lsa_node->link_lsa_list, lsa);
    }
}

struct ospf_lsa *
ospf_te_lsdb_lookup (struct ptree *te_lsdb, struct pal_in4_addr id,
                     u_char lsa_type, struct pal_in4_addr *instance_id,
                     u_char remove_lsa,
                     u_char remove_table_entry)
{
  struct ptree_node *rn;
  struct te_lsa_node *te_lsa_node = NULL;
  struct ospf_lsa *lsa = NULL;

  rn = ospf_te_lsa_node_lookup (te_lsdb, id);
  if (! rn)
    return NULL;

  te_lsa_node = rn->info;

  if (lsa_type == TE_TLV_ROUTER_TYPE)
    {
      lsa = te_lsa_node->rtaddr_lsa;

      if ((lsa == NULL) || (!IPV4_ADDR_SAME (&lsa->data->id, instance_id)))
        return NULL;

      if (remove_lsa)
        {
          te_lsa_node->rtaddr_lsa = NULL;
        }
    }
  else
    {
      struct listnode *ln;

      ln = ospf_te_link_lsa_lookup_by_instance (te_lsa_node, instance_id);
      if (! ln)
        return NULL;

      lsa = GETDATA (ln);

      if (remove_lsa)
        list_delete_node (te_lsa_node->link_lsa_list, ln);
    }

  if ((remove_table_entry) &&
      (te_lsa_node->rtaddr_lsa == NULL) &&
      (list_isempty (te_lsa_node->link_lsa_list)))
    {
      rn->info = NULL;
      ptree_unlock_node (rn);

      ospf_te_lsa_node_free (te_lsa_node);
    }

  return lsa;
}

void
ospf_te_lsdb_free (struct ptree *te_lsdb)
{
  struct ptree_node *rn;
  struct te_lsa_node *te_node;

  for (rn = ptree_top (te_lsdb); rn; rn = ptree_next (rn))
    if ((te_node = rn->info))
      {
        ospf_te_lsa_node_free (te_node);
        rn->info = NULL;
        ptree_unlock_node (rn);
      }
}

struct ptree_node *
ospf_te_lsa_node_lookup (struct ptree *te_lsdb, struct pal_in4_addr id)
{
  struct ptree_node *rn;

  rn = ptree_node_lookup (te_lsdb, (u_char *)&id, IPV4_MAX_BITLEN);
  if (rn)
    ptree_unlock_node (rn);

  return rn;
}

struct listnode *
ospf_te_link_lsa_lookup (struct te_lsa_node *te_lsa_node,
                         struct pal_in4_addr *id, u_char link_type,
                         struct pal_in4_addr *remote_if_addr, 
                         struct pal_in4_addr *remote_if_id)  
{
  struct ospf_lsa *lsa;
  struct listnode *ln;
  struct ospf_te_tlv_link_lsa *te_link_lsa;
  struct ospf_te_lsa_data *data;

  LIST_LOOP (te_lsa_node->link_lsa_list, lsa, ln)
    {
      data = (struct ospf_te_lsa_data *)(lsa->data);
      te_link_lsa = &data->ospf_te_tlv_lsa.link_lsa;

      if ((link_type == te_link_lsa->link_type.link_type) &&
          (IPV4_ADDR_SAME (id, &te_link_lsa->link_id.link_id)))
        {
          if (remote_if_addr != NULL || remote_if_id != 0)
            {      
              if (
#ifdef HAVE_GMPLS
                (CHECK_FLAG (te_link_lsa->flag, TE_LINK_ATTR_LINK_LOCAL_REMOTE_ID)
                && remote_if_id && (!pal_mem_cmp (remote_if_id, 
                      &te_link_lsa->local_remote_id.local_if_id, 
                      sizeof (struct pal_in4_addr))))||
#endif /* HAVE_GMPLS */
                (CHECK_FLAG (te_link_lsa->flag, TE_LINK_ATTR_REMOTE_IF_ADDR)
                && remote_if_addr && (IPV4_ADDR_SAME (remote_if_addr, 
                                    te_link_lsa->local_addr.addr))))
                return ln;
            }
          else
            return ln;
        }
    }
  return NULL;
}

#endif /* HAVE_OSPF_CSPF */


/* Used for neighbor lists. */
struct ospf_lsdb *
ospf_lsdb_init (void)
{
  struct ospf_lsdb *lsdb;
  int i;

  lsdb = ospf_lsdb_new ();
  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    lsdb->type[i].db = ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);

  lsdb->max_lsa_age = 0;
  lsdb->next_slot_type = OSPF_MIN_LSA;
  return lsdb;
}

#ifdef HAVE_OPAQUE_LSA
struct ospf_lsdb *
ospf_link_lsdb_init (struct ospf_interface *oi)
{
  struct ospf_lsdb *lsdb;

  lsdb = ospf_lsdb_new ();
  lsdb->scope = LSA_FLOOD_SCOPE_LINK;
  lsdb->u.oi = oi;

  lsdb->type[OSPF_LINK_OPAQUE_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);

#ifdef HAVE_OPAQUE_LSA
  lsdb->opaque_table = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
#endif /* HAVE_OPAQUE_LSA */

  return lsdb;
}
#endif /* HAVE_OPAQUE_LSA */

struct ospf_lsdb *
ospf_area_lsdb_init (struct ospf_area *area)
{
  struct ospf_lsdb *lsdb;
  int i;

  lsdb = ospf_lsdb_new ();
  lsdb->scope = LSA_FLOOD_SCOPE_AREA;
  lsdb->u.area = area;

  lsdb->type[OSPF_ROUTER_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);
  lsdb->type[OSPF_NETWORK_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);
  lsdb->type[OSPF_SUMMARY_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);
  lsdb->type[OSPF_SUMMARY_LSA_ASBR].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);
#ifdef HAVE_NSSA
  lsdb->type[OSPF_AS_NSSA_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);
#endif /* HAVE_NSSA */
#ifdef HAVE_OPAQUE_LSA
  lsdb->type[OSPF_AREA_OPAQUE_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);

  lsdb->opaque_table = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_OSPF_CSPF
  lsdb->te_lsdb = ptree_init (IPV4_MAX_BITLEN);
#endif /* HAVE_OSPF_CSPF */

  /* Insert each LSDBs to the LSDB entry.  */
  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    if (lsdb->type[i].db)
      ospf_lsdb_entry_insert (area->top, area, i, lsdb->type[i].db);

  return lsdb;
}

struct ospf_lsdb *
ospf_as_lsdb_init (struct ospf *top)
{
  struct ospf_lsdb *lsdb;

  lsdb = ospf_lsdb_new ();
  lsdb->scope = LSA_FLOOD_SCOPE_AS;
  lsdb->u.top = top;

  lsdb->type[OSPF_AS_EXTERNAL_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);
#ifdef HAVE_OPAQUE_LSA
  lsdb->type[OSPF_AS_OPAQUE_LSA].db =
    ls_table_init (OSPF_LSDB_LOWER_TABLE_DEPTH, 1);

  lsdb->opaque_table = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
#endif /* HAVE_OPAQUE_LSA */

  return lsdb;
}

void
ospf_lsdb_finish_slot (struct ospf_lsdb *lsdb, int type)
{
  struct ls_table *table;
  struct ospf_lsa *lsa;
  struct ls_node *rn;

  table = lsdb->type[type].db;
  if (table == NULL)
    return;

  for (rn = ls_table_top (table); rn; rn = ls_route_next (rn))
    if ((lsa = RN_INFO (rn, RNI_LSDB_LSA)))
      {
        ospf_lsdb_list_delete (lsdb, lsa);
        ospf_lsa_refresh_queue_unset (lsa);
        ospf_lsa_refresh_event_unset (lsa);
        ls_node_info_unset (rn, RNI_LSDB_LSA);
        ospf_lsa_unlock (lsa);
      }

  ls_table_finish (table);
  lsdb->type[type].db = NULL;
}

/* Delete all LSDBs. */
void
ospf_lsdb_finish (struct ospf_lsdb *lsdb)
{
  int i;

  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    if (lsdb->type[i].db)
      ospf_lsdb_finish_slot (lsdb, i);

  ospf_lsdb_free (lsdb);
}

/* Delete interface specific (type 9) LSDB. */
#ifdef HAVE_OPAQUE_LSA
void
ospf_link_lsdb_finish (struct ospf_lsdb *lsdb)
{
  ospf_lsdb_finish_slot (lsdb, OSPF_LINK_OPAQUE_LSA);

  ospf_opaque_map_delete_all (lsdb->opaque_table);
  ls_table_finish (lsdb->opaque_table);

  ospf_lsdb_free (lsdb);
}
#endif /* HAVE_OPAQUE_LSA */

/* Delete area specific (type 1,2,3,4,7,10) LSDBs. */
void
ospf_area_lsdb_finish (struct ospf_lsdb *lsdb)
{
  /* Remove from the LSDB entry.  */
  ospf_lsdb_entry_delete_area (lsdb->u.area);

  ospf_lsdb_finish_slot (lsdb, OSPF_ROUTER_LSA);
  ospf_lsdb_finish_slot (lsdb, OSPF_NETWORK_LSA);
  ospf_lsdb_finish_slot (lsdb, OSPF_SUMMARY_LSA);
  ospf_lsdb_finish_slot (lsdb, OSPF_SUMMARY_LSA_ASBR);
#ifdef HAVE_NSSA
  ospf_lsdb_finish_slot (lsdb, OSPF_AS_NSSA_LSA);
#endif /* HAVE_NSSA */

#ifdef HAVE_OPAQUE_LSA
  ospf_lsdb_finish_slot (lsdb, OSPF_AREA_OPAQUE_LSA);
#ifdef HAVE_OSPF_CSPF
  if (lsdb->te_lsdb)
    {
      ospf_te_lsdb_free (lsdb->te_lsdb);
      ptree_finish (lsdb->te_lsdb);
      lsdb->te_lsdb = NULL;
    }
#endif /* HAVE_OSPF_CSPF */

  ospf_opaque_map_delete_all (lsdb->opaque_table);
  ls_table_finish (lsdb->opaque_table);
#endif /* HAVE_OPAQUE_LSA */

  ospf_lsdb_free (lsdb);
}

/* Delete AS global (type 5,11) LSDBs. */
void
ospf_as_lsdb_finish (struct ospf_lsdb *lsdb)
{
  ospf_lsdb_finish_slot (lsdb, OSPF_AS_EXTERNAL_LSA);
#ifdef HAVE_OPAQUE_LSA
  ospf_lsdb_finish_slot (lsdb, OSPF_AS_OPAQUE_LSA);

  ospf_opaque_map_delete_all (lsdb->opaque_table);
  ls_table_finish (lsdb->opaque_table);
#endif /* HAVE_OPAQUE_LSA */

  ospf_lsdb_free (lsdb);
}


unsigned long
ospf_lsdb_linklist_count_all (struct ospf_link_list *lsdb)
{
  return lsdb->count;
}

/* Used for neighbor lists. */
void
ospf_lsdb_linklist_init (struct ospf_link_list *list)
{
  pal_mem_set (list, '\0', sizeof (struct ospf_link_list));
  return;
}

int
ospf_lsdb_linklist_isempty (struct ospf_link_list *list)
{
  return OSPF_LIST_ISEMPTY (list);
}


/* Add new LSA to lsdb. */
void
ospf_lsdb_linklist_add (struct ospf_link_list *lsdb, struct ospf_lsa *lsa)
{
  struct ospf_list_elt *new;  

  new = XMALLOC (MTYPE_OSPF_LSDB, sizeof (struct ospf_list_elt));
  pal_mem_set (new, '\0', sizeof (struct ospf_list_elt));

  ospf_lsa_lock (lsa);
  new->lsa = lsa;
  
  /* Function assumes the element is not there in the linked list*/
  if ((lsdb->head == NULL) && (lsdb->count == 0))
    {
      /* First element */
      lsdb->head = new;
    }
  else
    {
      /* Non first element */
      new->prev = lsdb->tail;
      lsdb->tail->next = new;
    }
  
  lsdb->tail = new;
  lsdb->count++;

  return;
}


/* Add new LSA to lsdb. */
void
ospf_lsdb_linklist_delete (struct ospf_link_list *lsdb, struct ospf_lsa *lsa)
{
  struct ospf_list_elt *elt;  

  elt = lsdb->head;
  while (elt != NULL)
    {
      if (elt->lsa == lsa)
        break;

      /* Find the element */
      elt = elt->next;
    }
  
  if (!elt)
     return;

  if (elt == lsdb->head)
    {
      lsdb->head = elt->next;
    }
  
  if (elt == lsdb->tail)
    {
      lsdb->tail = elt->prev;
    }
 
  if (elt->prev)
    {
      elt->prev->next = elt->next;
    }

  if (elt->next)
    {
      elt->next->prev = elt->prev;
    }

  lsdb->count--;

  XFREE (MTYPE_OSPF_LSDB, elt);
  ospf_lsa_unlock (lsa);

  return;
}

/* Delete all LSDBs. */
void
ospf_lsdb_linklist_finish (struct ospf_link_list *lsdb)
{

  struct ospf_list_elt  *elt, *tmp;

  tmp = lsdb->head;
  while (tmp)
    {
      elt = tmp;

      /* walk trough the database */
      tmp = elt->next;
      ospf_lsdb_linklist_delete (lsdb, elt->lsa);

      ospf_lsa_unlock (elt->lsa);
      if (elt == lsdb->head)
        lsdb->head = elt->next;

      if (elt == lsdb->tail)
        lsdb->tail = elt->prev;

      if (elt->prev)
         elt->prev->next = elt->next;

      if (elt->next)
         elt->next->prev = elt->prev;

      XFREE (MTYPE_OSPF_LSDB, elt);
    }

  XFREE (MTYPE_OSPF_NEIGHBOR, lsdb);
  return;
}

