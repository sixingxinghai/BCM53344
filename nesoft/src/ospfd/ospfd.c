/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "stream.h"
#include "sockunion.h"
#include "table.h"
#include "linklist.h"
#include "filter.h"
#include "log.h"
#include "bheap.h"
#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_network.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_ifsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_ia.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_routemap.h"
#include "ospfd/ospf_entity.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_cli.h"
#include "ospfd/ospf_show.h"
#include "ospfd/ospf_debug.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_SNMP
#include "ospfd/ospf_snmp.h"
#endif /* HAVE_SNMP */
#ifdef HAVE_OSPF_CSPF
#include "cspf_common.h"
#include "cspf/ospf_cspf.h"
#endif /* HAVE_OSPF_CSPF */
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_BFD
#include "ospfd/ospf_bfd.h"
#endif /* HAVE_BFD */


/* Constant declaration. */
struct pal_in4_addr IPv4AllSPFRouters;
struct pal_in4_addr IPv4AllDRouters;
struct pal_in4_addr IPv4AddressUnspec;
struct pal_in4_addr IPv4LoopbackAddress;
struct pal_in4_addr IPv4HostAddressMask;
struct ls_prefix LsPrefixIPv4Default;
#ifdef HAVE_MD5
u_char OSPFMessageDigestKeyUnspec[OSPF_AUTH_MD5_SIZE];
#endif /* HAVE_MD5 */
struct pal_timeval OSPFLSARefreshEventInterval;
struct ls_prefix DefaultIPv4;


/* Prototypes. */
void ospf_summary_address_delete_all (struct ospf *);
void ospf_redist_map_flush_lsa_all (struct ls_table *);


int
ospf_notifier_cmp (void *val1, void *val2)
{
  struct ospf_notifier *ntf1 = val1;
  struct ospf_notifier *ntf2 = val2;

  return ntf1->priority - ntf2->priority;
}

void
ospf_notifier_free (void *ntf)
{
  XFREE (MTYPE_OSPF_NOTIFIER, ntf);
}

void *
ospf_register_notifier (struct ospf_master *om, u_char event,
                        int (*callback) (u_char, void *, void *, void *),
                        void *arg, int priority)
{
  struct ospf_notifier *ntf;

  if (event >= OSPF_NOTIFY_MAX)
    return NULL;

  ntf = XMALLOC (MTYPE_OSPF_NOTIFIER, sizeof (struct ospf_notifier));
  ntf->event = event;
  ntf->callback = callback;
  ntf->arg = arg;
  ntf->priority = priority;

  listnode_add_sort (om->notifiers[event], ntf);

  return ntf;
}

void
ospf_unregister_notifier (struct ospf_master *om, void *hntf)
{
  struct ospf_notifier *ntf = hntf;
  listnode_delete (om->notifiers[ntf->event], ntf);
  ospf_notifier_free (ntf);
}

void
ospf_call_notifiers (struct ospf_master *om, u_char event,
                     void *param1, void *param2)
{
  struct ospf_notifier *ntf;
  struct listnode *node;

  LIST_LOOP (om->notifiers[event], ntf, node)
    if (ntf->callback (event, ntf->arg, param1, param2) < 0)
      return;
}

void
ospf_update_status (void *data, int pos)
{
 struct ospf_vertex *v = (struct ospf_vertex*)data;
 *(v->status) = pos;
}


u_int32_t
ospf_area_count (struct ospf *top)
{
  return top->area_table->count[RNI_DEFAULT];
}

u_int32_t
ospf_area_default_count (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;
  u_int32_t count = 0;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        if (IS_AREA_DEFAULT (area))
          count++;

  return count;
}

u_int32_t
ospf_area_if_count (struct ospf_area *area)
{
  return listcount (area->iflist);
}

u_int32_t
ospf_area_vlink_count (struct ospf_area *area)
{
  return area->vlink_count;
}

#ifdef HAVE_NSSA
void
ospf_area_conf_nssa_reset (struct ospf_area_conf *conf)
{
  conf->metric = OSPF_NSSA_METRIC_DEFAULT;
  conf->metric_type = OSPF_NSSA_METRIC_TYPE_DEFAULT;
  conf->nssa_translator_role = OSPF_NSSA_TRANSLATE_CANDIDATE;
  conf->nssa_stability_interval = OSPF_TIMER_NSSA_STABILITY_INTERVAL;
}
#endif /* HAVE_NSSA */

void
ospf_area_conf_reset (struct ospf_area_conf *conf)
{
  /* Sanity cleanup. */
  conf->config = 0;
  conf->format = OSPF_AREA_ID_FORMAT_DEFAULT;
  conf->external_routing = OSPF_AREA_DEFAULT;
  conf->auth_type = OSPF_AREA_AUTH_NULL;
  conf->shortcut = OSPF_SHORTCUT_DEFAULT;
  conf->default_cost = OSPF_STUB_DEFAULT_COST_DEFAULT;
#ifdef HAVE_NSSA
  ospf_area_conf_nssa_reset (conf);
#endif /* HAVE_NSSA */
}

/* Allocate new OSPF Area object. */
struct ospf_area *
ospf_area_new (struct ospf *top, struct pal_in4_addr area_id)
{
  struct ospf_area *new;

  /* Allocate new config_network. */
  new = XMALLOC (MTYPE_OSPF_AREA, sizeof (struct ospf_area));
  pal_mem_set (new, 0, sizeof (struct ospf_area));

  /* Initialize variables.  */
  new->top = top;
  new->area_id = area_id;

  /* Init lock count. */
  new->lock = 0;

  /* Reset Area configuration.  */
  ospf_area_conf_reset (&new->conf);

  /* Initialize LSDB.  */
  new->lsdb = ospf_area_lsdb_init (new);

  /* Initialize SPF.  */
  new->vertices = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 2);
  new->candidate = bheap_initialize (MAXELEMENTS, ospf_vertex_cmp,
                                             ospf_update_status, NULL);

  /* Initialize tables.  */
  new->ranges = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
  new->aggregate_table =
    ls_table_init (OSPF_AREA_AGGREGATE_LOWER_TABLE_DEPTH, 1);
  new->ia_prefix = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
  new->ia_router = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
#ifdef HAVE_NSSA
  new->redist_table = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
#endif /* HAVE_NSSA */

  /* Initialize linked-lists and vectors.  */
  new->iflist = list_new ();
  new->link_vec = vector_init (OSPF_VECTOR_SIZE_MIN);
  new->lsa_incremental = vector_init (OSPF_VECTOR_SIZE_MIN);

  /* Initialize the route calculation module.  */
  new->rt_calc = ospf_route_calc_init ();

  /* Set the area's row status.  */
  new->status = ROW_STATUS_ACTIVE;

  return new;
}

void
ospf_area_free (struct ospf_area *area)
{
  XFREE (MTYPE_OSPF_AREA, area);
}

/* OSPF area related functions. */
int
ospf_area_entry_insert (struct ospf *top, struct ospf_area *area)
{
  struct ls_node *rn;
  struct ls_prefix p;
  int ret = OSPF_API_ENTRY_INSERT_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix));
  p.prefixsize = OSPF_AREA_TABLE_DEPTH;
  p.prefixlen = OSPF_AREA_TABLE_DEPTH * 8;

  /* Set Area ID as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_AREA_TABLE].vars,
                      &area->area_id);

  rn = ls_node_get (top->area_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_DEFAULT, area);
      ret = OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  ls_unlock_node (rn);
  return ret;
}

int
ospf_area_entry_delete (struct ospf *top, struct ospf_area *area)
{
  struct ls_node *rn;
  struct ls_prefix p;
  int ret = OSPF_API_ENTRY_DELETE_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix));
  p.prefixsize = OSPF_AREA_TABLE_DEPTH;
  p.prefixlen = OSPF_AREA_TABLE_DEPTH * 8;

  /* Set Area ID as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_AREA_TABLE].vars,
                      &area->area_id);

  rn = ls_node_lookup (top->area_table, (struct ls_prefix *)&p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      ret = OSPF_API_ENTRY_DELETE_SUCCESS;
    }
  return ret;
}

struct ospf_area *
ospf_area_entry_lookup (struct ospf *top, struct pal_in4_addr area_id)
{
  struct ls_node *rn;
  rn = ospf_api_lookup (top->area_table, OSPF_AREA_TABLE, &area_id);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

int
ospf_stub_area_entry_insert (struct ospf *top, struct ospf_area *area)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  int tos = 0;
  int ret = OSPF_API_ENTRY_INSERT_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_STUB_AREA_TABLE_DEPTH;
  p.prefixlen = OSPF_STUB_AREA_TABLE_DEPTH * 8;

  /* Set Area ID and TOS as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_STUB_AREA_TABLE].vars,
                      &area->area_id, &tos);
  rn = ls_node_get (top->stub_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_DEFAULT, area);
      ret = OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  ls_unlock_node (rn);
  return ret;
}

int
ospf_stub_area_entry_delete (struct ospf *top, struct ospf_area *area)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  int tos = 0;
  int ret = OSPF_API_ENTRY_DELETE_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_STUB_AREA_TABLE_DEPTH;
  p.prefixlen = OSPF_STUB_AREA_TABLE_DEPTH * 8;

  /* Set Area ID as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_STUB_AREA_TABLE].vars,
                      &area->area_id, &tos);
  rn = ls_node_lookup (top->stub_table, (struct ls_prefix *)&p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      ret = OSPF_API_ENTRY_DELETE_SUCCESS;
    }
  return ret;
}

struct ospf_area *
ospf_stub_area_entry_lookup (struct ospf *top, struct pal_in4_addr area_id,
                             int tos)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->stub_table, OSPF_STUB_AREA_TABLE, &area_id, &tos);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

#ifdef HAVE_GMPLS
void
ospf_telink_hash_interator (struct hash_backet *hb, struct ospf_area *area)
{
  struct telink *tl;
  struct ospf_telink *os_tel;
  struct ospf *top = area->top;
 
  tl = (struct telink *)hb->data;
  if (tl->info == NULL)            
    return;
  else
    {
      os_tel = (struct ospf_telink *)tl->info;
      if (!os_tel)
        return;
      if ((os_tel->params)
          && CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_AREA_ID))
        {
          if ((os_tel->params->proc_id == top->ospf_id) &&
              (IPV4_ADDR_SAME (&os_tel->params->area_id, &area->area_id)))
            {
              os_tel->top = top;
              os_tel->area = area;
              ospf_activate_te_link (os_tel); 
            }
        }
    }
  return;
}

#endif /* HAVE_GMPLS*/
struct ospf_area *
ospf_area_get (struct ospf *top, struct pal_in4_addr area_id)
{
  struct ospf_area *area;

  area = ospf_area_entry_lookup (top, area_id);
  if (area == NULL)
    {
      /* Hit area limit. */
      if (!IS_AREA_ID_BACKBONE (area_id))
        if ((AREA_COUNT_NON_BACKBONE (top)) >= top->max_area_limit)
          return NULL;

      area = ospf_area_new (top, area_id);
      if (IS_AREA_BACKBONE (area))
        top->backbone = area;

      ospf_area_entry_insert (top, area);

      ospf_call_notifiers (top->om, OSPF_NOTIFY_AREA_NEW, area, NULL);
    }

  return area;
}

void
ospf_area_delete (struct ospf_area *area)
{
  struct ospf_master *om = area->top->om;

  /* Free LSDBs and related tables. */
  ospf_area_lsdb_finish (area->lsdb);
  ospf_route_calc_finish (area);

  /* Free Area ranges and related API table. */
  ospf_area_range_delete_all (area);

  /* Free tables.  */
  ls_table_finish (area->ia_prefix);
  ls_table_finish (area->ia_router);
  ls_table_finish (area->vertices);
#ifdef HAVE_NSSA
  ls_table_finish (area->redist_table);
#endif /* HAVE_NSSA */

  /* Free linked-lists.  */
  list_delete (area->iflist);
  bheap_free (area->candidate);

  /* Free vectors.  */
  vector_free (area->lsa_incremental);
  vector_free (area->link_vec);

  /* Free filter lists. */
  if (ALIST_NAME (area, FILTER_IN))
    XFREE (MTYPE_OSPF_DESC, ALIST_NAME (area, FILTER_IN));
  if (ALIST_NAME (area, FILTER_OUT))
    XFREE (MTYPE_OSPF_DESC, ALIST_NAME (area, FILTER_OUT));
  if (PLIST_NAME (area, FILTER_IN))
    XFREE (MTYPE_OSPF_DESC, PLIST_NAME (area, FILTER_IN));
  if (PLIST_NAME (area, FILTER_OUT))
    XFREE (MTYPE_OSPF_DESC, PLIST_NAME (area, FILTER_OUT));

  /* Remove link from instance. */
  if (IS_AREA_BACKBONE (area))
    area->top->backbone = NULL;

  if (area->top->area_table)
    ospf_area_entry_delete (area->top, area);

  /* Call notifiers.  */
  ospf_call_notifiers (om, OSPF_NOTIFY_AREA_DEL, area, NULL);

  /* Now free area structure. */
  ospf_area_free (area);
}

struct ospf_area *
ospf_area_lock (struct ospf_area *area)
{
  area->lock++;
  return area;
}

struct ospf_area *
ospf_area_unlock (struct ospf_area *area)
{
  if (!area)
    return NULL;

  area->lock--;
  pal_assert (area->lock >= 0);

  if (area->lock == 0)
    {
      ospf_area_delete (area);
      return NULL;
    }
  return area;
}

void
ospf_area_link_vec_clear (struct ospf_area *area)
{
  struct ospf_link_map *map;
  int i;

  for (i = 0; i < vector_max (area->link_vec); i++)
    if ((map = vector_slot (area->link_vec, i)))
      {
        ospf_link_map_free (map);
        vector_slot (area->link_vec, i) = NULL;
      }
}

void
ospf_area_up (struct ospf_area *area)
{
  struct ospf *top = area->top;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  gmif = gmpls_if_get (top->om->zg, top->om->vr->id);
#endif /* HAVE_GMPLS */

  if (!IS_AREA_ACTIVE (area))
    {
      SET_FLAG (area->flags, OSPF_AREA_UP);
      area->status = ROW_STATUS_ACTIVE;
      area->tv_spf_curr = top->spf_min_delay;

      /* Update the ABR status.  */
      ospf_abr_status_update (top);

      /* Update the summary routes.  */
      ospf_abr_ia_update_all_to_area (area);

      /* Redistribute route.  */
      ospf_redistribute_timer_add_all_proto (top);

      /* Default information originate.  */
      if (IS_AREA_DEFAULT (area))
        if (top->default_origin == OSPF_DEFAULT_ORIGINATE_ALWAYS)
          ospf_redist_map_default_update (top->redist_table,
                                          OSPF_AS_EXTERNAL_LSA, top);
#ifdef HAVE_GMPLS
      hash_iterate (gmif->telhash,
                    (void (*) (struct hash_backet*, void*)) 
                    ospf_telink_hash_interator, area);
#endif /* HAVE_GMPLS */
    }
}

void
ospf_area_down (struct ospf_area *area)
{
  int i;
  struct ospf *top = area->top;
#ifdef HAVE_GMPLS
  struct ospf_telink *os_tel = NULL;
  struct telink *tl = NULL;
  struct avl_node *tl_node = NULL;
#endif /* HAVE_GMPLS */

  if (!CHECK_FLAG (area->flags, OSPF_AREA_UP))
    return;

  /* Resert flags.  */
  area->flags = 0;
  area->oflags = 0;

  /* Cancel threads. */
  OSPF_EVENT_SUSPEND_OFF (area->t_spf_calc, area);
#ifdef HAVE_NSSA
  OSPF_EVENT_OFF (area->t_nssa);
  OSPF_EVENT_OFF (area->t_redist);
#endif /* HAVE_NSSA */

  /* Cleanup the pending threads.  */
  OSPF_EVENT_PENDING_CLEAR (area->event_pend);

  /* Clean SPF tree for this area. */
  ospf_spf_vertex_delete_all (area);

  /* Clean candidate list for SPF. */
  for (i = 0; i < area->candidate->count; i++)
    {
      if (area->candidate->array[i] != NULL)
        ospf_vertex_free (area->candidate->array[i]);
    }
  bheap_make_empty (area->candidate);

  /* Clear IA map tables. */
  ospf_ia_lsa_map_clear (area->ia_prefix);
  ospf_ia_lsa_map_clear (area->ia_router);

#ifdef HAVE_NSSA
  /* Unset redistribute timer.  */
  ospf_nssa_redistribute_timer_delete_all_proto (area);
#endif /* HAVE_NSSA */

  /* Free stub default. */
  if (area->stub_default)
    {
      ospf_ia_lsa_map_free (area->stub_default);
      area->stub_default = NULL;
    }

  /* Cleanup the inter-area events/routes.  */
  ospf_ia_calc_route_clean (area);

#ifdef HAVE_NSSA
  /* Cleanup the NSSA AS external events/routes.  */
  if (IS_AREA_NSSA (area))
    ospf_ase_calc_nssa_route_clean (area);
#endif /* HAVE_NSSA */

  /* Cleanup the intra-area/inter-area routes.  */
  ospf_route_calc_route_clean (area);

  /* Clean LSDB. */
  ospf_lsdb_lsa_discard_all (area->lsdb, 1, 1, 1);

  /* Free Area link vector. */
  ospf_area_link_vec_clear (area);

  /* If there is no other default area, remove external-LSAs. */
  if (!ospf_area_default_count (top))
    {
#ifdef HAVE_NSSA
      struct ospf_area *area_other;
      struct ls_node *rn;
#endif /* HAVE_NSSA */

      ospf_redist_map_delete_all (top->redist_table);
      ospf_lsdb_lsa_discard_all (top->lsdb, 0, 1, 1);

#ifdef HAVE_NSSA
      for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
        if ((area_other = RN_INFO (rn, RNI_DEFAULT)))
          if (area_other != area)
            if (IS_AREA_ACTIVE (area_other))
              if (IS_AREA_NSSA (area_other))
                ospf_nssa_redistribute_timer_add_all_proto (area_other);
#endif /* HAVE_NSSA */

#ifdef HAVE_GMPLS
      AVL_TREE_LOOP (&top->om->zg->gmif->teltree, tl, tl_node)
        {
          if (tl->info == NULL)
            continue;
          else
            {
              os_tel = (struct ospf_telink *)tl->info;
              if ((os_tel->top == top) && (os_tel->area == area))
                {
                  ospf_deactivate_te_link (os_tel);
                  os_tel->top = NULL;
                  os_tel->area = NULL;
                }
            } 
        } 
#endif /* HAVE_GMPLS */
    }
}

void
ospf_area_type_update (struct ospf_area *area, u_char new)
{
  u_char old = area->conf.external_routing;
  struct ospf_master *om = area->top->om;
  struct ospf_interface *oi;
  struct listnode *node;
  int ret1, ret2;

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER: AREA[%r]: external routing changed %s -> %s",
               &area->area_id, LOOKUP (ospf_area_type_msg, old),
               LOOKUP (ospf_area_type_msg, new));

  ret1 = ospf_area_default_count (area->top);

  /* Shutdown the interface.  */
  LIST_LOOP (area->iflist, oi, node)
    OSPF_IFSM_EVENT_EXECUTE (oi, IFSM_InterfaceDown);

  /* Update the external routing type.  */
  OSPF_AREA_EXTERNAL_ROUTING (area) = new;

  /* Bring up the interface.  */
  LIST_LOOP (area->iflist, oi, node)
    {
      if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
        {
          ospf_if_set_variables (oi);
          OSPF_IFSM_EVENT_EXECUTE (oi, oi->type == OSPF_IFTYPE_LOOPBACK
                                   ? IFSM_LoopInd : IFSM_InterfaceUp);
        }
    }

  if (!IS_AREA_ACTIVE (area))
    return;

  /* Update the summary-LSAs.  */
  ospf_abr_ia_update_all_to_area (area);

  /* Update AS-external-LSA. */
  ret2 = ospf_area_default_count (area->top);
  if (ret1 != ret2)
    {
      /* Update the ASBR status.  */
      ospf_asbr_status_update (area->top);

      if (ret1 == 0 && ret2 == 1)
        ospf_redistribute_timer_add_all_proto (area->top);
      else if (ret1 == 1 && ret2 == 0)
        {
          ospf_redist_map_delete_all (area->top->redist_table);
          ospf_lsdb_lsa_discard_all (area->top->lsdb, 0, 1, 1);
        }
    }
}

int
ospf_area_conf_external_routing_set (struct ospf_area *area, u_char type)
{
  if (OSPF_AREA_EXTERNAL_ROUTING (area) == type)
    return 0;

  if (IS_AREA_STUB (area))
    ospf_stub_area_entry_delete (area->top, area);
  else if (type == OSPF_AREA_STUB)
    ospf_stub_area_entry_insert (area->top, area);

#ifdef HAVE_NSSA
  if (IS_AREA_NSSA (area))
    {
      ospf_area_conf_nssa_reset (&area->conf);
      ospf_ase_calc_nssa_route_clean (area);
      area->top->nssa_count--;
    }
  else if (type == OSPF_AREA_NSSA)
    {
      ospf_area_conf_nssa_reset (&area->conf);
      area->top->nssa_count++;
    }
#endif /* HAVE_NSSA */

  if (type == OSPF_AREA_DEFAULT)
    {
      OSPF_AREA_CONF_UNSET (area, DEFAULT_COST);
      OSPF_AREA_CONF_UNSET (area, EXTERNAL_ROUTING);
    }
  else
    OSPF_AREA_CONF_SET (area, EXTERNAL_ROUTING);

  /* Update the area type.  */
  if (area != NULL)
    ospf_area_type_update (area, type);

  return 1;
}

int
ospf_area_conf_default_cost_set (struct ospf_area *area, u_int32_t cost)
{
  if (OSPF_AREA_DEFAULT_COST (area) == cost)
    return 0;

  OSPF_AREA_DEFAULT_COST (area) = cost;
  if (cost == OSPF_STUB_DEFAULT_COST_DEFAULT)
    OSPF_AREA_CONF_UNSET (area, DEFAULT_COST);
  else
    OSPF_AREA_CONF_SET (area, DEFAULT_COST);

  /* Refresh the LSA.  */
  if (area != NULL)
    {
#ifdef HAVE_NSSA
      if (IS_AREA_NSSA (area))
        ospf_nssa_redistribute_timer_add (area, APN_ROUTE_DEFAULT);
#endif /* HAVE_NSSA */

      ospf_abr_ia_update_default_to_area (area);
    }
  return 1;
}

u_char
ospf_area_auth_type_get (struct ospf_area *area)
{
  if (OSPF_AREA_CONF_CHECK (area, AUTH_TYPE))
    return area->conf.auth_type;

  return OSPF_AREA_AUTH_NULL;
}

#ifdef HAVE_NSSA
u_char
ospf_nssa_translator_role_get (struct ospf_area *area)
{
  if (OSPF_AREA_CONF_CHECK (area, NSSA_TRANSLATOR))
    return area->conf.nssa_translator_role;

  return OSPF_NSSA_TRANSLATE_CANDIDATE;
}
#endif /* HAVE_NSSA */

u_char
ospf_area_shortcut_get (struct ospf_area *area)
{
  if (OSPF_AREA_CONF_CHECK (area, SHORTCUT_ABR))
    return area->conf.shortcut;

  return OSPF_SHORTCUT_DEFAULT;
}


/* Config network statement related functions. */
struct ospf_network *
ospf_network_new ()
{
  struct ospf_network *new;

  new = XMALLOC (MTYPE_OSPF_NETWORK, sizeof (struct ospf_network));
  pal_mem_set (new, 0, sizeof (struct ospf_network));

  return new;
}

void
ospf_network_free (struct ospf_network *network)
{
  XFREE (MTYPE_OSPF_NETWORK, network);
}

struct ospf_network *
ospf_network_lookup (struct ospf *top, struct ls_prefix *p)
{
  struct ls_node *rn;

  rn = ls_node_lookup (top->networks, p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

struct ospf_network *
ospf_network_match (struct ospf *top, struct ls_prefix *lp)
{
  struct ls_node *rn, *rn_tmp;

  rn_tmp = ls_node_get (top->networks, lp);
  for (rn = rn_tmp; rn; rn = rn->parent)
    if (RN_INFO (rn, RNI_DEFAULT))
      {
        ls_unlock_node (rn_tmp);
        return RN_INFO (rn, RNI_DEFAULT);
      }

  ls_unlock_node (rn_tmp);
  return NULL;
}

/* Find and configure the host entry matches to the connected address. */
void
ospf_host_entry_match_apply (struct ospf *top, struct connected *ifc)
{
  struct ls_node *rn = NULL;
  struct ospf_host_entry *host = NULL;
  struct prefix p;
  struct prefix host_p;

  for (rn = ls_table_top (top->host_table); rn; rn = ls_route_next (rn))
    if ((host = RN_INFO (rn, RNI_DEFAULT)))
      {
        prefix_copy (&p, ifc->address);
        apply_mask (&p);

        prefix_copy (&host_p, &p);
        host_p.u.prefix4 = host->addr;
        apply_mask (&host_p);

        if (IPV4_ADDR_SAME (&p.u.prefix4, &host_p.u.prefix4))
          ospf_host_entry_apply (top, host, ifc);
      }
}

bool_t
ospf_network_same (struct ospf *top, struct ls_prefix *lp)
{
  struct ls_node *rn = NULL;
  u_int8_t bytes;
  struct ls_prefix pfx;
  struct ls_prefix pfx1;

  if (top == NULL || lp == NULL)
    return PAL_FALSE;

  for (rn = ls_table_top (top->networks); rn; rn = ls_route_next (rn))
   if (RN_INFO (rn, RNI_DEFAULT))
     {
       if (!(rn->p))
         continue;

       if (rn->p->prefixlen > lp->prefixlen)
         bytes = lp->prefixlen;
       else
         bytes = rn->p->prefixlen;
   
       ls_prefix_copy (&pfx, rn->p); 
       ls_prefix_copy (&pfx1, lp);

       pfx.prefixlen = bytes;
       pfx1.prefixlen = bytes;
       apply_mask_ls(&pfx);
       apply_mask_ls(&pfx1);
       
       if (!pal_mem_cmp (&pfx.prefix, &pfx1.prefix, rn->p->prefixsize))
         return PAL_TRUE;
     }

  return PAL_FALSE;
}

int
ospf_network_area_match (struct ospf_area *area)
{
  struct ospf_network *nw;
  struct ls_node *rn;

  for (rn = ls_table_top (area->top->networks); rn; rn = ls_route_next (rn))
    if ((nw = RN_INFO (rn, RNI_DEFAULT)))
      if (IPV4_ADDR_SAME (&nw->area_id, &area->area_id))
        {
          ls_unlock_node (rn);
          return 1;
        }

  return 0;
}

void
ospf_network_apply_alternative (struct ospf *top, struct connected *ifc)
{
  struct ospf_master *om = top->om;
  struct ospf *top_alt;
  struct ospf_network *nw;
  struct ls_prefix lp;
  struct listnode *node;

  ls_prefix_ipv4_set (&lp, ifc->address->prefixlen,
                      ifc->address->u.prefix4);

  LIST_LOOP (om->ospf, top_alt, node)
    if (top_alt != top)
      if ((nw = ospf_network_match (top_alt, &lp)))
        ospf_network_apply (top_alt, nw, ifc);
}

bool_t
ospf_network_run (struct ospf *top, struct ospf_network *network)
{
  struct apn_vrf *iv = top->ov->iv;
  struct interface *ifp;
  struct connected *ifc;
  struct route_node *rn;
  bool_t network_applied = PAL_FALSE;

  if (!IS_PROC_ACTIVE (top))
    return PAL_FALSE;

  /* Check all interfaces and all connected addresses. */
  for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if (!ospf_if_disable_all_get (ifp))
        for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
          {
#ifdef HAVE_GMPLS
            if ((ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)
               &&(ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA))
              continue;
            else
#endif /* HAVE_GMPLS */
              if (ospf_network_apply (top, network, ifc))
                network_applied = PAL_TRUE;
          }
  return network_applied;
}

/* Look up through the interfaces and the connected addresses
   for the host entry match. */
void
ospf_host_entry_run (struct ospf *top, struct ospf_host_entry *host)
{
  struct apn_vrf *iv = top->ov->iv;
  struct interface *ifp = NULL;
  struct connected *ifc = NULL;
  struct route_node *rn = NULL;

  if (!IS_PROC_ACTIVE (top))
    return;

  /* Check all interfaces and all connected addresses. */
  for (rn = route_top (iv->ifv.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if (!ospf_if_disable_all_get (ifp))
        for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
          ospf_host_entry_apply (top, host, ifc);
}

void
ospf_network_withdraw (struct ospf *top, struct ospf_network *nw_del)
{
  struct ospf_interface *oi;
  struct ospf_network *nw_alt;
  struct connected *ifc;
  struct ls_node *rn;

  /* First check if this network is covered by other network. */
  nw_alt = ospf_network_match (top, nw_del->lp);

  /* Second delete OSPF interface covered by nw_del. */
  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_OSPF_IF)))
      if (oi->network == nw_del)
        {
          ifc = RN_INFO (rn, RNI_CONNECTED);
          ospf_if_delete (oi);

          /* If this interface is not enabled by this instance,
             check if other instance enable this interface. */
          if (nw_alt == NULL)
            ospf_network_apply_alternative (top, ifc);
        }

  /* Last, check if this network is matched other network. */
  if (nw_alt != NULL)
    ospf_network_run (top, nw_alt);
}

void
ospf_network_add (struct ospf *top, struct ospf_network *network,
                  struct ls_prefix *lp)
{
  struct ls_node *rn;

  rn = ls_node_get (top->networks, lp);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      network->lp = rn->p;
      RN_INFO_SET (rn, RNI_DEFAULT, network);
    }

  ls_unlock_node (rn);
}

void
ospf_network_delete (struct ospf *top, struct ospf_network *nw)
{
  struct ospf_area *area;
  struct ls_node *rn;

  rn = ls_node_lookup (top->networks, nw->lp);
  if (rn)
    {
      area = ospf_area_entry_lookup (top, nw->area_id);

      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ospf_network_withdraw (top, nw);
      ls_unlock_node (rn);

      ospf_area_unlock (area);

      /* Update AS-external-LSA.  */
      if (!ospf_area_default_count (top))
        ospf_lsdb_lsa_discard_by_type (top->lsdb,
                                       OSPF_AS_EXTERNAL_LSA, 1, 1, 1);
      else
        ospf_redistribute_timer_add_all_proto (top);
    }
}

int
ospf_network_get (struct ospf *top, struct pal_in4_addr area_id,
                  struct ls_prefix *lp, u_int16_t id)
{
  struct ospf_network *nw;
  struct ospf_area *area;

  area = ospf_area_get (top, area_id);
  if (area == NULL)
    return OSPF_API_SET_ERR_AREA_LIMIT;
  ospf_area_lock (area);

  nw = ospf_network_new ();
  nw->area_id = area_id;
#ifdef HAVE_OSPF_MULTI_INST
  nw->inst_id = id;
#endif /* HAVE_OSPF_MULTI_INST */

  ospf_network_add (top, nw, lp);
  if (ospf_network_run (top, nw)== PAL_FALSE)
    return OSPF_API_SET_ERR_NETWORK_NOT_ACTIVE;

  return OSPF_API_SET_SUCCESS;
}

int
ospf_network_apply (struct ospf *top, struct ospf_network *nw,
                    struct connected *ifc)
{
  struct ospf_area *area;
  struct ospf_interface *oi;
  struct ospf_network *nw_alt;
  struct ls_prefix lp;
  struct ospf_master *om = top->om;
#ifdef HAVE_OSPF_MULTI_INST
  struct listnode *node = NULL;
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

  if (!IS_PROC_ACTIVE (top))
    return 0;

  if (ifc->address->family != AF_INET)
    return 0;

  if (IPV4_ADDR_SAME (&ifc->address->u.prefix4, &IPv4LoopbackAddress))
    return 0;

  ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN, ifc->address->u.prefix4);

  if (((ospf_network_match (top, &lp)) != nw) &&
      !(IPV4_ADDR_SAME (&nw->lp->prefix, &DefaultIPv4.prefix)))
    return 0;

  oi = ospf_global_if_entry_lookup (om, ifc->address->u.prefix4,
                                    ifc->ifp->ifindex);
  if (oi != NULL)
    {
#ifdef HAVE_OSPF_MULTI_INST
      if (oi->top != top && oi->network->inst_id == nw->inst_id)
        return 0;

      if ((oi->top == top) && (oi->network == nw))
        return 0;

      /* If more than one process are enabled on this subnet
         then check other processes also */
      else if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
        {
          /* If any other ospf process is using 
             same instance-id on same subnet then dont
             activate the new process */  
          LIST_LOOP (om->ospf, top_alt, node)
            {
              if (oi->top != top_alt)
                {
                  oi_other = ospf_if_entry_match (top_alt, oi);
                  if (top_alt != top
                      && oi_other
                      && oi_other->network->inst_id == nw->inst_id)
                    return 0;
                }
            }
        }

      /* Skip below if-checks which restricts different ospf processes
         being enabled on single subnet with difefrent instance-ids */
#else
      if (oi->top != top)
        return 0;

      if (oi->network == nw)
        return 0;
#endif /* HAVE_OSPF_MULTI_INST */

      nw_alt = ospf_network_match (top, &lp);
      if (nw_alt != nw)
        return 0;

#ifdef HAVE_OSPF_MULTI_INST
      if (oi->top == top)
#endif /* HAVE_OSPF_MULTI_INST */
      ospf_if_delete (oi);
    }

  /* Check the duplicate IP address.  */
  if (!if_is_ipv4_unnumbered (ifc->ifp))
    if (ospf_if_entry_lookup (top, ifc->address->u.prefix4, 0))
      return 0;

  area = ospf_area_get (top, nw->area_id);
  if (area == NULL)
    return 0;

  (void) ospf_area_format_set (om->vr->id, top->ospf_id, 
                               nw->area_id, nw->format);

  oi = ospf_if_new (top, ifc, area);
  oi->network = nw;

  if (if_is_up (ifc->ifp))
    ospf_if_up (oi);

#ifdef HAVE_OSPF_MULTI_AREA
  ospf_multi_area_link_up_corr_to_primary (top, ifc);   
#endif /* HAVE_OSPF_MULTI_AREA */

  return 1;
}

/* Create a host specific ospf interface for the match found
   and bring it up. */
int
ospf_host_entry_apply (struct ospf *top, struct ospf_host_entry *host,
                       struct connected *ifc)
{
  struct ospf_area *area = NULL;
  struct ospf_interface *oi = NULL;
  struct prefix p;
  struct prefix host_p;
  struct ospf_master *om = top->om;

  if (!IS_PROC_ACTIVE (top))
    return 0;

  if (ifc->address->family != AF_INET)
    return 0;

  if (IPV4_ADDR_SAME (&ifc->address->u.prefix4, &IPv4LoopbackAddress))
    return 0;

  prefix_copy (&p, ifc->address);

  prefix_copy (&host_p, &p);
  host_p.u.prefix4 = host->addr;

  if (IPV4_ADDR_SAME (&ifc->address->u.prefix4, &host_p.u.prefix4))
    return 0;

  apply_mask (&p);
  apply_mask (&host_p);

  /* Check if the host entry matches the given connected. */
  if (! IPV4_ADDR_SAME (&p.u.prefix4, &host_p.u.prefix4))
    return 0;

  oi = ospf_global_if_entry_lookup (om, host->addr,
                                    ifc->ifp->ifindex);
  if (oi != NULL)
    {
      if (IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, &host->addr))
        {
          if (oi->top != top)
            return 0;

          ospf_if_delete (oi);
        }
    }

  area = ospf_area_get (top, host->area_id);
  (void) ospf_area_format_set (om->vr->id, top->ospf_id, 
                               host->area_id, host->format);

  /* Create a host specific ospf interface. */
  oi = ospf_host_entry_if_new (top, ifc, area, host);

  if (if_is_up (ifc->ifp))
    ospf_if_up (oi);

  return 1;
}


struct ospf_passive_if *
ospf_passive_if_new (char *name, struct pal_in4_addr addr)
{
  struct ospf_passive_if *opi;

  opi = XMALLOC (MTYPE_OSPF_PASSIVE_IF, sizeof (struct ospf_passive_if));
  pal_mem_set (opi, 0, sizeof (struct ospf_passive_if));
  opi->name = XSTRDUP (MTYPE_OSPF_DESC, name);
  opi->addr = addr;

  return opi;
}

void
ospf_passive_if_free (struct ospf_passive_if *opi)
{
  if (opi->name)
    XFREE (MTYPE_OSPF_DESC, opi->name);

  XFREE (MTYPE_OSPF_PASSIVE_IF, opi);
}

/* Lookup the interface in Passive or no passive interface list */
struct ospf_passive_if *
ospf_passive_if_lookup (struct list *passive_if_list, char *name, 
                                   struct pal_in4_addr addr)
{
  struct ospf_passive_if *opi;
  struct listnode *node;

  LIST_LOOP (passive_if_list, opi, node)
    if (pal_strcmp (opi->name, name) == 0)
      if (IPV4_ADDR_SAME (&opi->addr, &addr))
        return opi;

  return NULL;
}

int
ospf_passive_if_apply (struct ospf_interface *oi)
{
  u_char oflags = CHECK_FLAG (oi->flags, OSPF_IF_PASSIVE);
  struct ospf_passive_if *opi;

  opi = ospf_passive_if_lookup (oi->top->passive_if, oi->u.ifp->name,
                                oi->ident.address->u.prefix4);
  if (opi == NULL)
    opi = ospf_passive_if_lookup (oi->top->passive_if, oi->u.ifp->name, 
                                  IPv4AddressUnspec);

  /* Update the flag.  */
  if (opi != NULL)
    SET_FLAG (oi->flags, OSPF_IF_PASSIVE);
  else
    UNSET_FLAG (oi->flags, OSPF_IF_PASSIVE);

  /* Return 1 if the flags was changed.  */
  if (CHECK_FLAG (oi->flags, OSPF_IF_PASSIVE) != oflags)
    return 1;
  else
    return 0;
}

void
ospf_passive_if_apply_all (struct ospf *top, struct ospf_passive_if *opi)
{
  struct ospf_master *om = top->om;
  struct ospf_interface *oi;
  struct interface *ifp;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix8 p;

#ifdef HAVE_OSPF_MULTI_INST
  struct ospf *top_alt = NULL;
  struct ospf_interface *oi_other = NULL;
  struct listnode *node_alt = NULL;
#endif /* HAVE_OSPF_MULTI_INST */

#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
  struct listnode *node = NULL;
  struct ospf *top2 = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  ifp = if_lookup_by_name (&om->vr->ifm, opi->name);
  if (ifp == NULL)
    return;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixlen = 32;
  p.prefixsize = OSPF_GLOBAL_IF_TABLE_DEPTH;
  ls_prefix_set_args ((struct ls_prefix *)&p, "i", &ifp->ifindex);

  rn_tmp = ls_node_get (om->if_table, (struct ls_prefix *)&p);

  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
        if (oi->top == top)
          {
            if (ospf_passive_if_apply (oi))
              ospf_if_update (oi);
          }
#ifdef HAVE_OSPF_MULTI_INST
        /* Check the remaining ospf processes */
        else if (CHECK_FLAG (oi->flags, OSPF_IF_EXT_MULTI_INST))
          {
            LIST_LOOP(om->ospf, top_alt, node_alt)
              {
                /* If passive interface has been configured on 
                   this process (top_alt) then only apply to it */
                if (top_alt == top)
                  {                 
                    /* Get oi from interface table of each instance */
                     oi_other = ospf_if_entry_match (top_alt, oi);
                     if (oi_other && ospf_passive_if_apply (oi_other))
                       {
                         ospf_if_update (oi_other);
                         /* Quit the loop as passive int configuration 
                            is applied*/ 
                         break;
                       }
                  }
              }
          }
#endif /* HAVE_OSPF_MULTI_INST */
      } 
  ls_unlock_node (rn_tmp);

#ifdef HAVE_OSPF_MULTI_AREA
  LIST_LOOP (om->ospf, top2, node)
    {
      if (top2 == top)
         for (rn = ls_table_top (top->multi_area_link_table); rn;
                                       rn = ls_route_next (rn))
            if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
              if (pal_strcmp (ifp->name, mlink->ifname) == 0 && mlink->oi)
                if (ospf_passive_if_apply (mlink->oi))
                  ospf_if_update (mlink->oi);
    }
#endif
}

int
ospf_passive_if_add (struct ospf *top, char *name, struct pal_in4_addr addr)
{
  struct ospf_passive_if *opi;

  opi = ospf_passive_if_lookup (top->passive_if, name, addr);
  if (opi == NULL)
    {
      /* If interface present in no passive interface list then remove */
      if (top->passive_if_default == PAL_TRUE)
        {
          opi = ospf_passive_if_lookup (top->no_passive_if, name, addr);
          if (opi != NULL)
            {
              listnode_delete(top->no_passive_if, opi);
              ospf_passive_if_free (opi);
            }
        }
      
      opi = ospf_passive_if_new (name, addr);
      listnode_add (top->passive_if, opi);
      ospf_passive_if_apply_all (top, opi);
    }
  else
    return OSPF_API_SET_ERR_PASSIVE_IF_ENTRY_EXIST;

  return OSPF_API_SET_SUCCESS;
}
/* Add dynamically added interface to passive interface list */
void
ospf_passive_if_add_by_interface (struct ospf_master *om,
                                  struct interface *ifp)
{
  struct listnode *node;
  struct ospf *top;


  LIST_LOOP (om->ospf, top, node)
    {
      if ( top->passive_if_default == PAL_TRUE)
        {
          if (ospf_passive_if_lookup (top->no_passive_if, ifp->name, 
                                      IPv4AddressUnspec) == NULL)
            ospf_passive_if_add (top, ifp->name, IPv4AddressUnspec);
        }
    }
}

int
ospf_passive_if_delete (struct ospf *top, char *name, struct pal_in4_addr addr)
{
  struct ospf_passive_if *opi;

  opi = ospf_passive_if_lookup (top->passive_if, name, addr);
  if (opi != NULL)
    {
      listnode_delete (top->passive_if, opi);
      ospf_passive_if_apply_all (top, opi);
      ospf_passive_if_free (opi);
    } 
  
  else
    {
      if (top->passive_if_default == PAL_FALSE)
        return OSPF_API_SET_ERR_PASSIVE_IF_ENTRY_NOT_EXIST; 
    }

  /* Add interface to no passive interface list */
  if (top->passive_if_default == PAL_TRUE)
    {
      if (ospf_passive_if_lookup (top->no_passive_if, name, addr) == NULL)
        {
          opi = ospf_passive_if_new (name, addr);
          listnode_add (top->no_passive_if, opi);
        }
      else
        return OSPF_API_SET_ERR_NO_PASSIVE_IF_ENTRY_EXIST;
    }

  return OSPF_API_SET_SUCCESS;
}

void
ospf_passive_if_delete_by_interface (struct ospf_master *om,
                                     struct interface *ifp)
{
  struct listnode *node1, *node2, *next;
  struct ospf_passive_if *opi;
  struct ospf *top;

  /* Remove the interface from passive interface list */
  LIST_LOOP (om->ospf, top, node1)
    {
      for (node2 = LISTHEAD (top->passive_if); node2; node2 = next)
        {
          next = node2->next;
          if ((opi = GETDATA (node2)))
            if (pal_strcmp (opi->name, ifp->name) == 0)
              {
                listnode_delete (top->passive_if, opi);
                ospf_passive_if_apply_all (top, opi);
                ospf_passive_if_free (opi);
              }
        }

      /* Remove the interface from no passive interface list */ 
      for (node2 = LISTHEAD (top->no_passive_if); node2; node2 = next)
        {
          next = node2->next;
          if ((opi = GETDATA (node2)))
            if (pal_strcmp (opi->name, ifp->name) == 0)
              {
                listnode_delete (top->no_passive_if, opi);
                ospf_passive_if_free (opi);
              }
        }
    }
}

/* Remove interface from passive interface list by name */
void
ospf_passive_if_list_delete (struct ospf *top)
{
  struct listnode *node, *next;
  struct ospf_passive_if *opi;

  for (node = LISTHEAD (top->passive_if); node; node = next)
    {
      next = node->next;
      if ((opi = GETDATA (node)))
          {
            listnode_delete (top->passive_if, opi);
            ospf_passive_if_apply_all (top, opi);
            ospf_passive_if_free (opi);
          }
    }
}

/* Remove interface from no passive interface list by name */
void
ospf_no_passive_if_list_delete (struct ospf *top)
{
  struct listnode *node, *next;
  struct ospf_passive_if *opi;

  for (node = LISTHEAD (top->no_passive_if); node; node = next)
    {
      next = node->next;
      if ((opi = GETDATA (node)))
          {
            listnode_delete (top->no_passive_if, opi);
            ospf_passive_if_free (opi);
          }
      }

}
struct ospf_host_entry *
ospf_host_entry_new ()
{
  struct ospf_host_entry *new;

  new = XMALLOC (MTYPE_OSPF_HOST_ENTRY, sizeof (struct ospf_host_entry));
  pal_mem_set (new, 0, sizeof (struct ospf_host_entry));
  new->format = OSPF_AREA_ID_FORMAT_DEFAULT;
  new->status = ROW_STATUS_ACTIVE;

  return new;
}

void
ospf_host_entry_free (struct ospf_host_entry *host)
{
  XFREE (MTYPE_OSPF_HOST_ENTRY, host);
}

int
ospf_host_entry_insert (struct ospf *top, struct ospf_host_entry *host)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  int tos = 0;
  int ret = OSPF_API_ENTRY_INSERT_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_HOST_TABLE_DEPTH;
  p.prefixlen = OSPF_HOST_TABLE_DEPTH * 8;

  /* Set IP Address and TOS as index. */
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_HOST_TABLE].vars,
                      &host->addr, &tos);

  rn = ls_node_get (top->host_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_DEFAULT, host);
      ret = OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  ls_unlock_node (rn);
  return ret;
}

int
ospf_host_entry_delete (struct ospf *top, struct ospf_host_entry *host)
{
  struct ls_node *rn = NULL;
  struct ls_node *rn_tmp = NULL;
  struct ospf_interface *oi;
  struct ls_prefix8 p;
  int tos = 0;
  int ret = OSPF_API_ENTRY_DELETE_ERROR;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_HOST_TABLE_DEPTH;
  p.prefixlen = OSPF_HOST_TABLE_DEPTH * 8;

  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_HOST_TABLE].vars,
                      &host->addr, &tos);

  /* Set IP Address and TOS as index. */
  rn = ls_node_lookup (top->host_table, (struct ls_prefix *) &p);
  if (rn)
    {
      /* Delete OSPF interface covered by the host entry. */
      for (rn_tmp = ls_table_top (top->if_table); rn_tmp; 
           rn_tmp = ls_route_next (rn_tmp))
        if ((oi = RN_INFO (rn_tmp, RNI_OSPF_IF)))
          if (IPV4_ADDR_SAME (&host->addr, &oi->ident.address->u.prefix4))
            ospf_if_delete (oi);

      ospf_link_vec_unset_by_host (top, host);

      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);

      ret = OSPF_API_ENTRY_DELETE_SUCCESS;
    }
  return ret;
}

/* Lookup ospf_host_entry matched exactly. */
struct ospf_host_entry *
ospf_host_entry_lookup (struct ospf *top, struct pal_in4_addr addr, int tos)
{
  struct ls_node *rn;

  rn = ospf_api_lookup (top->host_table, OSPF_HOST_TABLE, &addr, &tos);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }

  return NULL;
}


/* Allocate new ospf structure. */
struct ospf *
ospf_proc_new (struct ospf_master *om)
{
  struct ospf *new;

  new = XMALLOC (MTYPE_OSPF, sizeof (struct ospf));
  pal_mem_set (new, 0, sizeof (struct ospf));
  new->om = om;

  return new;
}

void
ospf_proc_free (struct ospf *top)
{
  XFREE (MTYPE_OSPF, top);
}

void
ospf_proc_init (struct ospf *top, int proc_id)
{
  struct ospf_master *om = top->om;
  int i;

  /* Set the process ID.  */
  top->ospf_id = proc_id;

  /* Initialize socket.  */
  top->fd = -1;

  ospf_call_notifiers (om, OSPF_NOTIFY_PROCESS_NEW, top, NULL);

  /* Set flags. */
  top->abr_type = OSPF_ABR_TYPE_DEFAULT;
  UNSET_FLAG (top->config, OSPF_CONFIG_RFC1583_COMPATIBLE);

  /* Initialize tables related to internal objects. */
  top->if_table = ls_table_init (OSPF_IF_TABLE_DEPTH, 2);
  top->area_table = ls_table_init (OSPF_AREA_TABLE_DEPTH, 1);
  top->vlink_table = ls_table_init (OSPF_VIRT_IF_TABLE_DEPTH, 1);
  top->redist_table = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
  top->nexthop_table = ls_table_init (OSPF_NEXTHOP_TABLE_DEPTH, 1);
#ifdef HAVE_OSPF_MULTI_AREA
  /* Initialize the multi area if table */
  top->multi_area_link_table = ls_table_init (OSPF_MULTI_AREA_IF_TABLE_DEPTH, 2);
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Initialize passive interface related variables */
  top->passive_if = list_new ();
  top->no_passive_if = list_new ();
  top->passive_if_default = PAL_FALSE;

  /* Initialize tables ralated to API. */
  top->stub_table = ls_table_init (OSPF_STUB_AREA_TABLE_DEPTH, 1);
  top->lsdb_table = ls_table_init (OSPF_LSDB_UPPER_TABLE_DEPTH, 1);
  top->area_range_table = ls_table_init (OSPF_AREA_RANGE_TABLE_DEPTH, 1);
  top->host_table = ls_table_init (OSPF_HOST_TABLE_DEPTH, 1);
  top->nbr_table = ls_table_init (OSPF_NBR_TABLE_DEPTH, 1);

  /* Initilaize tables for configuration. */
  top->networks = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);
  top->nbr_static = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);

  /* Initialize LSDB for AS-external-LSAs. */
  top->lsdb = ospf_as_lsdb_init (top);
  top->default_origin = OSPF_DEFAULT_ORIGINATE_UNSPEC;

  /* Initialize Routing tables. */
  top->rt_network = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 2);
  top->rt_asbr = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 2);

  /* Initialize summary addresses. */
  top->summary = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);

  /* Initialize distribute parameter. */
  for (i = 0; i < APN_ROUTE_MAX; i++)
    {
      top->redist[i].metric = EXTERNAL_METRIC_VALUE_UNSPEC;
      DIST_NAME (&top->redist[i]) = NULL;
    }

  top->default_metric = -1;
  top->ref_bandwidth = OSPF_DEFAULT_REF_BANDWIDTH;
  top->max_dd = OSPF_MAX_CONCURRENT_DD_DEFAULT;

#ifdef HAVE_OSPF_DB_OVERFLOW
  /* DB overflow init */
  top->ext_lsdb_limit = OSPF_DEFAULT_LSDB_LIMIT;
  top->exit_overflow_interval = OSPF_DEFAULT_EXIT_OVERFLOW_INTERVAL;
#endif /* HAVE_OSPF_DB_OVERFLOW */

  /* LSDB limit init. */
  top->lsdb_overflow_limit = 0;
  top->lsdb_overflow_limit_type = 0;

  /* maximum area limit */
  top->max_area_limit = OSPF_AREA_LIMIT_DEFAULT;

  /* SPF timer value init. */
  top->spf_min_delay.tv_sec = OSPF_SPF_MIN_DELAY_DEFAULT / ONE_SEC_MILLISECOND;
  top->spf_min_delay.tv_usec = (OSPF_SPF_MIN_DELAY_DEFAULT % ONE_SEC_MILLISECOND)
                               * MILLISEC_TO_MICROSEC;

  top->spf_max_delay.tv_sec = OSPF_SPF_MAX_DELAY_DEFAULT / ONE_SEC_MILLISECOND;
  top->spf_max_delay.tv_usec = (OSPF_SPF_MAX_DELAY_DEFAULT % ONE_SEC_MILLISECOND)
                               * MILLISEC_TO_MICROSEC;

  /* Distance table init. */
  top->distance_table = ls_table_init (LS_IPV4_ROUTE_TABLE_DEPTH, 1);

  /* LSA timer init. */
  top->lsa_refresh.interval = OSPF_LSA_REFRESH_INTERVAL_DEFAULT;
  top->lsa_maxage_interval = OSPF_LSA_MAXAGE_INTERVAL_DEFAULT;

  /* Initialize LSA vectors.  */
  top->lsa_event = vector_init (OSPF_VECTOR_SIZE_MIN);
  top->lsa_originate = vector_init (OSPF_VECTOR_SIZE_MIN);

#ifdef HAVE_VRF_OSPF
  /* Domain_id List */
  top->domainid_list = list_new();
#endif /*HAVE_VRF_OSPF */

  /* Buffer init. */
  top->ibuf = stream_new (OSPF_BUFFER_SIZE_DEFAULT);
  top->obuf = stream_new (OSPF_BUFFER_SIZE_DEFAULT);
  top->lbuf = stream_new (OSPF_BUFFER_SIZE_DEFAULT);
  top->op_unuse_max = OSPF_PACKET_UNUSE_MAX_DEFAULT;
  top->lsa_unuse_max = OSPF_LSA_UNUSE_MAX_DEFAULT;
  top->oi_write_q = list_new ();

#ifdef HAVE_OPAQUE_LSA
  SET_FLAG (top->config, OSPF_CONFIG_OPAQUE_LSA);

 /* Initialize the SCOPE of Opaque LSAs */
  ospf_flood_scope[OSPF_LINK_OPAQUE_LSA] = LSA_FLOOD_SCOPE_LINK;
  ospf_flood_scope[OSPF_AREA_OPAQUE_LSA] = LSA_FLOOD_SCOPE_AREA;
  ospf_flood_scope[OSPF_AS_OPAQUE_LSA] = LSA_FLOOD_SCOPE_AS;

#ifdef HAVE_OSPF_TE
  SET_FLAG (top->config, OSPF_CONFIG_TRAFFIC_ENGINEERING);
#endif /* HAVE_OSPF_TE */
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_RESTART
  top->restart_method = OSPF_RESTART_METHOD_DEFAULT;

  if (CHECK_FLAG (om->flags, OSPF_GLOBAL_FLAG_RESTART))
    SET_FLAG (top->flags, OSPF_ROUTER_RESTART);
#endif /* HAVE_RESTART */
}

int
ospf_proc_cmp (void *a, void *b)
{
  struct ospf *t1 = (struct ospf *)a;
  struct ospf *t2 = (struct ospf *)b;

  return t1->ospf_id - t2->ospf_id;
}

struct ospf_redist_conf *
ospf_redist_conf_lookup_by_inst (struct ospf *top, int type, int proc_id)
{
  struct ospf_redist_conf *rc = &top->redist[type];

  for (; rc; rc = rc->next)
   if (rc->pid == proc_id)
     return rc;

  return NULL;
}

struct ospf *
ospf_proc_lookup (struct ospf_master *om, int proc_id)
{
  struct ospf *top;
  struct listnode *node;

  LIST_LOOP (om->ospf, top, node)
    if (top->ospf_id == proc_id)
      return top;

  return NULL;
}

struct ospf *
ospf_proc_lookup_any (int proc_id, u_int32_t vr_id)
{
  struct ospf_master *om = ospf_master_lookup_by_id (ZG, vr_id);

  if ((om == NULL) || (listcount (om->ospf) == 0))
    return NULL;

  if (proc_id == OSPF_PROCESS_ID_ANY)
    return GETDATA (LISTHEAD (om->ospf));

  return ospf_proc_lookup (om, proc_id);
}

struct ospf *
ospf_proc_lookup_by_lsdb (struct ospf_lsdb *lsdb)
{
  switch (lsdb->scope)
    {
    case LSA_FLOOD_SCOPE_LINK:
      return lsdb->u.oi->top;
    case LSA_FLOOD_SCOPE_AREA:
      return lsdb->u.area->top;
    case LSA_FLOOD_SCOPE_AS:
      return lsdb->u.top;
    }
  return NULL;
}

pal_time_t
ospf_proc_uptime (struct ospf *top)
{
  return top->start_time ? pal_time_current (NULL) - top->start_time : 0;
}

int
ospf_proc_gc_timer (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;

  top->t_gc = NULL;

  /* set the VR context */
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER[%d]: GC timer expire", top->ospf_id);

  ospf_packet_unuse_clear_half (top);
  ospf_lsa_unuse_clear_half (top);

  OSPF_TOP_TIMER_ON (top->t_gc, ospf_proc_gc_timer, OSPF_GC_TIMER_INTERVAL);

  return 0;
}

int
ospf_proc_up (struct ospf *top)
{
  struct ospf_network *network;
  struct ls_node *rn;
#ifdef HAVE_GMPLS
  struct telink *tl = NULL;
  struct avl_node *tl_node = NULL;
  struct ospf_telink *os_tel = NULL;
#endif /* HAVE_GMPLS */


  /* VRF should be up.  */
  if (!IS_APN_VRF_UP (top->ov->iv))
    return 0;

  /* Router ID should be set.  */
  if (IPV4_ADDR_SAME (&top->router_id, &OSPFRouterIDUnspec))
    return 0;

  if (CHECK_FLAG (top->flags, OSPF_PROC_UP))
    return 1;

  /* Initialize the socket.  */
  ospf_sock_init (top);
  if (top->fd < 0)
    return 0;

  SET_FLAG (top->flags, OSPF_PROC_UP);

  /* Set the timestamp.  */
  top->start_time = pal_time_current (NULL);

  /* Init refresh queue. */
  ospf_lsa_refresh_init (top);

  /* Enable network area statement. */
  for (rn = ls_table_top (top->networks); rn; rn = ls_route_next (rn))
    if ((network = RN_INFO (rn, RNI_DEFAULT)))
      ospf_network_run (top, network);

#ifdef HAVE_GMPLS
  AVL_TREE_LOOP (&top->om->zg->gmif->teltree, tl, tl_node)
    {
      if (tl->info == NULL)
        continue;
      else
        {
          os_tel = (struct ospf_telink *)tl->info;
          if ((os_tel->top)
              && (os_tel->top->ospf_id == top->ospf_id))
            ospf_activate_te_link (os_tel);
          else
            continue;
        }
    }
#endif /* HAVE_GMPLS */

  /* Start GC. */
  OSPF_TOP_TIMER_ON (top->t_gc, ospf_proc_gc_timer, OSPF_GC_TIMER_INTERVAL);

  return 1;
}

/* Cancel all timers other than `write'. */
void
ospf_proc_timers_cancel (struct ospf *top)
{
  OSPF_TIMER_OFF (top->t_redist);
  OSPF_TIMER_OFF (top->t_ase_calc);
  OSPF_EVENT_OFF (top->t_lsa_event);
  OSPF_TIMER_OFF (top->t_lsa_walker);
  OSPF_EVENT_OFF (top->t_lsa_originate);
#ifdef HAVE_OSPF_DB_OVERFLOW
  OSPF_TIMER_OFF (top->t_overflow_exit);
#endif /* HAVE_OSPF_DB_OVERFLOW */
#ifdef HAVE_RESTART
  OSPF_TIMER_OFF (top->t_restart_state);
#endif /* HAVE_RESTART */
  OSPF_TIMER_OFF (top->t_lsdb_overflow_event);
  OSPF_TIMER_OFF (top->t_gc);
  OSPF_TIMER_OFF (top->t_read);
  OSPF_EVENT_OFF (top->t_route_cleanup);
}

void
ospf_proc_lsdb_clear_all (struct ospf *top)
{
#ifdef HAVE_OPAQUE_LSA
  struct ospf_interface *oi;
#endif /* HAVE_OPAQUE_LSA */
  struct ospf_area *area;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  /* For AS-eternal-LSAs and AS Opaque-LSAs. */
  ospf_lsdb_lsa_discard_all (top->lsdb, 1, 1, 1);

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      ospf_lsdb_lsa_discard_all (area->lsdb, 1, 1, 1);

#ifdef HAVE_OPAQUE_LSA
  /* For Link LSDBs.  */
#ifdef HAVE_RESTART
  if (!CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
#endif /* HAVE_RESTART */
    for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
      if ((oi = RN_INFO (rn, RNI_DEFAULT)))
        ospf_lsdb_lsa_discard_all (oi->lsdb, 1, 1, 1);
#ifdef HAVE_OSPF_MULTI_AREA
    for (rn = ls_table_top (top->multi_area_link_table); rn; 
         rn = ls_route_next (rn))
      if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
        if (mlink->oi)
          ospf_lsdb_lsa_discard_all (mlink->oi->lsdb, 1, 1, 1);
#endif /* HAVE_OSPF_MULTI_AREA */
#endif /* HAVE_OPAQUE_LSA */
}

int
ospf_proc_down (struct ospf *top)
{
  struct ospf_lsa *lsa;
  struct ls_node *rn;
  int i;

  if (!CHECK_FLAG (top->flags, OSPF_PROC_UP))
    return 1;

  UNSET_FLAG (top->flags, OSPF_PROC_UP);

  /* Reset the timestamp.  */
  top->start_time = 0;

  /* Cleanup ASE routes.  */
  ospf_ase_calc_route_clean (top);

  /* Cancel all timers. */
  ospf_proc_timers_cancel (top);

  /* Unset redistribute timer.  */
  ospf_redistribute_timer_delete_all_proto (top);

  /* Clear pending LSA refresh event. */
  for (i = 0; i < vector_max (top->lsa_event); i++)
    if ((lsa = vector_slot (top->lsa_event, i)))
      {
        UNSET_FLAG (lsa->flags, OSPF_LSA_REFRESH_EVENT);
        vector_slot (top->lsa_event, i) = NULL;
        ospf_lsa_unlock (lsa);
      }

  /* Clear pending LSA originate event.  */
  for (i = 0; i < vector_max (top->lsa_originate); i++)
    if ((lsa = vector_slot (top->lsa_originate, i)))
      {
        UNSET_FLAG (lsa->flags, OSPF_LSA_ORIGINATE_EVENT);
        vector_slot (top->lsa_originate, i) = NULL;
        ospf_lsa_unlock (lsa);
      }

  /* Flush LSAs from LSDBs. */
  ospf_proc_lsdb_clear_all (top);

  /* Bring down all interfaces. */
  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_OSPF_IF))
      {
        struct connected *ifc = RN_INFO (rn, RNI_CONNECTED);

        ospf_if_delete (RN_INFO (rn, RNI_DEFAULT));
        ospf_network_apply_alternative (top, ifc);
      }

  /* Free LSA refresh . */
  ospf_lsa_refresh_clear (top);

  /* Remove all routing entries from NSM.  */
#ifdef HAVE_RESTART
  if (!CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
#endif /* HAVE_RESTART */
    ospf_route_table_withdraw (top);

  /* Clear route tables. */
  ospf_route_table_finish (top->rt_network);
  ospf_route_table_finish (top->rt_asbr);

  /* Unset lsdb overflow flag. */
  if (CHECK_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT))
    UNSET_FLAG (top->flags, OSPF_LSDB_EXCEED_OVERFLOW_LIMIT);

#ifdef HAVE_RESTART
  /* Reset the restarting state.  */
  if (CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
    ospf_restart_state_unset (top);
#endif /* HAVE_RESTART */

  /* Reset the socket.  */
  ospf_sock_reset (top);

  return 1;
}

int
ospf_proc_update (struct ospf *top)
{
  int ret = 0;

  /* Shutdown and up.  */
  if (IS_PROC_ACTIVE (top))
    {
      ospf_proc_down (top);
      ospf_router_id_update (top);
      ret = ospf_proc_up (top);
    }
  return ret;
}

void
ospf_router_id_update (struct ospf *top)
{
  struct ospf_master *om = top->om;
  struct pal_in4_addr router_id;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  int count = 0;

  UNSET_FLAG (top->config, OSPF_CONFIG_ROUTER_ID_USE);

  if (CHECK_FLAG (top->config, OSPF_CONFIG_ROUTER_ID))
    router_id = top->router_id_config;
  else
    router_id = top->ov->iv->router_id;

  /* Router-ID not changed. */
  if (IPV4_ADDR_SAME (&top->router_id, &router_id))
    return;

  /* Counting neighbors which are above in Init State. */
  for (rn = ls_table_top (top->nbr_table); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->state > NFSM_Init)
        count++;

  if ((count == 0) && IS_PROC_ACTIVE (top))
    ospf_proc_down (top);

  if (!IS_PROC_ACTIVE (top))
    {
      SET_FLAG (top->config, OSPF_CONFIG_ROUTER_ID_USE);

      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "ROUTER[%d]: Router-ID update (%r) -> (%r)",
                   top->ospf_id, &top->router_id, &router_id);

      /* Update the router ID.  */
      top->router_id = router_id;

      /* Up the process.  */
      ospf_proc_up (top);

      ospf_call_notifiers (om, OSPF_NOTIFY_ROUTER_ID_CHANGED, top, NULL);
    }
}

struct ospf *
ospf_proc_get (struct ospf_master *om, int proc_id, char *name)
{
  struct ospf_vrf *ov;
  struct ospf *top;

  top = ospf_proc_lookup (om, proc_id);
  if (top == NULL)
    {
      ov = ospf_vrf_lookup_by_name (om, name);
      if (ov == NULL)
        return NULL;

      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "ROUTER[Process:%d]: Create Process", proc_id);

      /* Allocate OSPFv2 instance.  */
      top = ospf_proc_new (om);
      top->ov = ov;

      /* Initialize the process.  */
      ospf_proc_init (top, proc_id);

      /* Add the process to the global list.  */
      listnode_add_sort (om->ospf, top);
      listnode_add (ov->ospf, top);

      /* Set the router ID and start the process.  */
      ospf_router_id_update (top);

#ifdef HAVE_OSPF_CSPF
      /* Start CSPF server. */
      if (ospf_cspf_set (om->vr->id, top->ospf_id) < 0)
        if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
          zlog_info (ZG, "ROUTER[Process:%d]: CSPF initialization failed");
#endif /* HAVE_OSPF_CSPF */
    }

  return top;
}

void
ospf_proc_nbr_static_clear (struct ospf *top)
{
  struct ospf_nbr_static *nbr_static;
  struct ls_node *rn;

  for (rn = ls_table_top (top->nbr_static); rn; rn = ls_route_next (rn))
    if ((nbr_static = RN_INFO (rn, RNI_DEFAULT)))
      {
        OSPF_TIMER_OFF (nbr_static->t_poll);
        nbr_static->nbr = NULL;

        ospf_nbr_static_free (nbr_static);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
  ls_table_finish (top->nbr_static);
}

void
ospf_proc_config_clear (struct ospf *top)
{
  struct ospf_network *network;
  struct ospf_host_entry *host;
  struct ls_node *rn;
  struct listnode *node;

  /* Clear config networks. */
  for (rn = ls_table_top (top->networks); rn; rn = ls_route_next (rn))
    if ((network = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_network_free (network);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
  ls_table_finish (top->networks);

  /* Clear config hosts. */
  for (rn = ls_table_top (top->host_table); rn; rn = ls_route_next (rn))
    if ((host = RN_INFO (rn, RNI_DEFAULT)))
      {
        ospf_host_entry_free (host);
        RN_INFO_UNSET (rn, RNI_DEFAULT);
      }
  ls_table_finish (top->host_table);

  /* Cleanup configured address range for external-LSAs. */
  ospf_summary_address_delete_all (top);

  /* Cleanup passive-interface. */
  for (node = LISTHEAD (top->passive_if); node; NEXTNODE (node))
    ospf_passive_if_free (GETDATA (node));
  list_delete (top->passive_if);

  /* Cleanup no passive-interface list. */
  for (node = LISTHEAD (top->no_passive_if); node; NEXTNODE (node))
    ospf_passive_if_free (GETDATA (node));
  list_delete (top->no_passive_if);
}

void
ospf_proc_clear (struct ospf *top)
{
  struct ls_node *rn;
  struct ospf_redist_conf *rc;
  int type;

  /* Clear nsm redistribution and related tables. */
  ospf_distance_table_clear (top);

  /* Unset redistribute configuration.  */
  ospf_redist_conf_unset_all_proto (top);

  /* Clear vlink table. */
  for (rn = ls_table_top (top->vlink_table); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_DEFAULT))
      ospf_vlink_delete (top, RN_INFO (rn, RNI_DEFAULT));

#ifdef HAVE_OSPF_MULTI_AREA
  /* Clear Multi area link table. */
  for (rn = ls_table_top (top->multi_area_link_table); rn; 
       rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_DEFAULT))
      ospf_multi_area_link_delete (top, RN_INFO (rn, RNI_DEFAULT));
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Clear Area table. */
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_DEFAULT))
      ospf_area_delete (RN_INFO (rn, RNI_DEFAULT));

  /* Free tables. */
  ls_table_finish (top->distance_table);        /* Distance table. */
  ls_table_finish (top->vlink_table);           /* Virtual Link table. */
  ls_table_finish (top->if_table);              /* Interface table. */
  ls_table_finish (top->area_table);            /* Area table. */
  ls_table_finish (top->stub_table);            /* Stub table for API. */
  ls_table_finish (top->lsdb_table);            /* LSDB table for API. */
  ls_table_finish (top->area_range_table);      /* Area Range table for API. */
  ls_table_finish (top->nbr_table);             /* Neighbor table for API. */
  ls_table_finish (top->nexthop_table);         /* Nexthop table. */
  ls_table_finish (top->rt_network);            /* Network route table. */
  ls_table_finish (top->rt_asbr);               /* ASBR route table. */
  ls_table_finish (top->redist_table);          /* Redist table. */
#ifdef HAVE_OSPF_MULTI_AREA
  ls_table_finish (top->multi_area_link_table); /* Multi area interface table */
#endif /* HAVE_OSPF_MULTI_AREA */

  /* Free lists. */
  list_delete (top->oi_write_q);

  /* Free refresh vector. */
  if (top->lsa_refresh.vec)
    vector_free (top->lsa_refresh.vec);
  vector_free (top->lsa_event);

  /* Clear LSA originate vector.  */
  vector_free (top->lsa_originate);

#ifdef HAVE_VRF_OSPF
  /* Free the primary domain_id ptr */
  XFREE (MTYPE_OSPF_DOMAIN_ID, top->pdomain_id);

  /* Free domain-id list */
  list_delete (top->domainid_list);
#endif /* HAVE_VRF_OSPF */

  /* Cleanup AS scope LSDB. */
  ospf_as_lsdb_finish (top->lsdb);

  /* Clear buffer. */
  stream_free (top->ibuf);
  stream_free (top->obuf);
  stream_free (top->lbuf);
  ospf_packet_unuse_clear (top);
  ospf_lsa_unuse_clear (top);

  /* Free names. */
  for (type = 0; type < APN_ROUTE_MAX; type++)
   for (rc = &top->redist[type]; rc; rc = rc->next)
    {
      if (RMAP_NAME (rc))
        XFREE (MTYPE_OSPF_DESC, RMAP_NAME (rc));

      if (DIST_NAME (rc))
        XFREE (MTYPE_OSPF_DESC, DIST_NAME (rc));
    }

  /* Remove configuration. */
  ospf_proc_nbr_static_clear (top);             /* NBMA neighbor. */
  ospf_proc_config_clear (top);                 /* Config table. */
}

/* Entry point of the process for destroying OSPF instance.
   This should be only single path to destroy the process.
   Sequence of finishing tables MUST not be changed. */
void
ospf_proc_destroy (struct ospf *top)
{
  struct ospf_master *om = top->om;

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER[Process:%d]: Destroy Process start", top->ospf_id);

  /* Really process destruction. */
  SET_FLAG (top->flags, OSPF_PROC_DESTROY);

  /* Call notifiers. */
  ospf_call_notifiers (om, OSPF_NOTIFY_PROCESS_DEL, top, NULL);

  /* Bring down process. */
  ospf_proc_down (top);

  /* Clear process tables. */
  ospf_proc_clear (top);

#ifdef HAVE_OSPF_CSPF
  /* Unset CSPF. */
  ospf_cspf_unset (om->vr->id, top->ospf_id);
#endif /* HAVE_OSPF_CSPF */

  /* We never reuse this process. */
  listnode_delete (om->ospf, top);
  listnode_delete (top->ov->ospf, top);

  /* Unlink VRF information. XXX-VR */

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER[Process:%d]: Destroy Process finish", top->ospf_id);

  /* Free structure.  */
  ospf_proc_free (top);
}


#ifdef HAVE_OSPF_DB_OVERFLOW
void
ospf_db_overflow_exit (struct ospf *top)
{
  if (!IS_DB_OVERFLOW (top))
    return;

  OSPF_TIMER_OFF (top->t_overflow_exit);
  UNSET_FLAG (top->flags, OSPF_ROUTER_DB_OVERFLOW);

  ospf_redistribute_timer_add_all_proto (top);
}

int
ospf_db_overflow_exit_timer (struct thread *t)
{
  struct ospf *top = THREAD_ARG (t);
  struct ospf_master *om = top->om;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER[%d]: Database overflow exit timer expire",
               top->ospf_id);

  top->t_overflow_exit = NULL;

  ospf_db_overflow_exit (top);
  return 0;
}

void
ospf_db_overflow_enter (struct ospf *top)
{
  struct ospf_master *om = top->om;

  if (IS_DB_OVERFLOW (top))
    return;

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER[%d]: Entering Database overflow state "
               "interval=%d", top->ospf_id, top->exit_overflow_interval);

#ifdef HAVE_SNMP
  ospfLsdbOverflow (top, OSPFLSDBOVERFLOW);
#endif /* HAVE_SNMP */

  SET_FLAG (top->flags, OSPF_ROUTER_DB_OVERFLOW);
  if (top->exit_overflow_interval)
    {
      int i = top->exit_overflow_interval;
      double j;
      double ij;
      struct pal_timeval tv;

      j = (double)((pal_rand () % (i * 2000))) / 10000.0;
      ij = (double)i + j - (i / 10.0);

      tv.tv_sec = ((int) ij);
      tv.tv_usec = (ij - (double)((int) ij)) * 1000000;

      OSPF_TV_TIMER_ON (top->t_overflow_exit,
                        ospf_db_overflow_exit_timer, top, tv);
    }

  ospf_redist_map_flush_lsa_all (top->redist_table);
}
#endif /* HAVE_OSPF_DB_OVERFLOW */


void
ospf_buffer_stream_ensure (struct stream *s, size_t size)
{
  if (s->size > size)
    return;

  s->data = XREALLOC (MTYPE_STREAM_DATA, s->data, s->size * 2);
  s->size *= 2;

  if (s->size < size)
    ospf_buffer_stream_ensure (s, size);
}

int
ospf_thread_wrapper (struct thread *thread,
                     int (*func) (struct thread *), char *str, int flag)
{
  struct pal_timeval st, et, rt;
  int ret;

  if (flag)
    pal_time_tzcurrent (&st, NULL);

  ret = func (thread);

  if (flag)
    {
      pal_time_tzcurrent (&et, NULL);
      rt = TV_SUB (et, st);
      zlog_info (ZG, "%s finished (%d.%06d sec)", str, rt.tv_sec, rt.tv_usec);
    }

  return ret;
}


/* If access-list is updated, apply some check. */
void
ospf_access_list_update (struct apn_vr *vr,
                         struct access_list *access,
                         struct filter_list *filter)
{
  struct ospf_master *om = vr->proto;
  struct ospf *top;
  struct ospf_area *area;
  struct ls_node *rn;
  struct access_list *al;
  struct listnode *node;
  struct ospf_redist_conf *rc;
  char *name;
  int type;

  /* If OSPF instatnce does not exist, return right now. */
  if (list_isempty (om->ospf))
    return;

  LIST_LOOP (om->ospf, top, node)
    {
      if ((name = DIST_NAME (&top->dist_in))
           && pal_strncmp (access->name, name,4) == 0)
        {
          DIST_LIST (&top->dist_in) =
            access_list_lookup (vr, AFI_IP, name);
          ospf_spf_calculate_timer_add_all (top);
        }

      /* Update distribute-list, and apply filter. */
      for (type = 0; type < APN_ROUTE_MAX; type++)
       for (rc = &top->redist[type]; rc; rc = rc->next)
        {
          /* Update the access-list for distribute-list.  */
          if ((name = DIST_NAME (rc))
              && pal_strcmp (access->name, name) == 0)
            {
              al = DIST_LIST (rc);
              DIST_LIST (rc) =
                access_list_lookup (vr, AFI_IP, name);
            }
          else
            al = NULL;

          /* Register the redistribute timer if distribute-list is
             updated or route-map is not NULL.  */
          if (al != NULL
              || DIST_LIST (rc) != NULL
              || RMAP_MAP (rc) != NULL)
            ospf_redistribute_timer_add (top, type);
        }

      /* Update filters. */
      for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
        if ((area = RN_INFO (rn, RNI_DEFAULT)))
          {
            /* Update Area access-list. */
            if ((name = ALIST_NAME (area, FILTER_IN)))
              if (pal_strcmp (access->name, name) == 0)
                {
                  al = ALIST_LIST (area, FILTER_IN);

                  ALIST_LIST (area, FILTER_IN) =
                    access_list_lookup (vr, AFI_IP, name);

                  if (IS_AREA_ACTIVE (area))
                    if (al != NULL
                        || ALIST_LIST (area, FILTER_IN) != NULL)
                      ospf_abr_ia_update_all_to_area (area);
                }

            if ((name = ALIST_NAME (area, FILTER_OUT)))
              if (pal_strcmp (access->name, name) == 0)
                {
                  al = ALIST_LIST (area, FILTER_OUT);

                  ALIST_LIST (area, FILTER_OUT) =
                    access_list_lookup (vr, AFI_IP, name);

                  if (IS_AREA_ACTIVE (area))
                    if (al != NULL
                        || ALIST_LIST (area, FILTER_OUT) != NULL)
                      ospf_abr_ia_update_area_to_all (area);
                }
          }
    }
}

/* If prefix-list is updated, apply some check. */
void
ospf_prefix_list_update (void)
{
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct ospf_master *om = NULL;
  struct ospf *top;
  struct ospf_area *area;
  struct ls_node *rn;
  struct prefix_list *pl;
  struct listnode *node;
  char *name;
  int type;

  if (vr == NULL)
    return;

  om = vr->proto;
  if (om == NULL)
    return;

  /* If OSPF instatnce does not exist, return right now. */
  if (list_isempty (om->ospf))
    return;

  LIST_LOOP (om->ospf, top, node)
    {
      /* Update distribute-list, and apply filter. */
      for (type = 0; type < APN_ROUTE_MAX; type++)
        {
          /* If route-map is not NULL it may be using this prefix-list. */
          if (RMAP_MAP (&top->redist[type]) != NULL)
            ospf_redistribute_timer_add (top, type);
        }

      /* Update filters. */
      for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
        if ((area = RN_INFO (rn, RNI_DEFAULT)))
          {
            /* Update Area prefix-list. */
            if ((name = PLIST_NAME (area, FILTER_IN)))
              {
                pl = PLIST_LIST (area, FILTER_IN);

                PLIST_LIST (area, FILTER_IN) =
                  prefix_list_lookup (vr, AFI_IP, name);

                if (IS_AREA_ACTIVE (area))
                  if (pl != NULL
                      || PLIST_LIST (area, FILTER_IN) != NULL)
                    ospf_abr_ia_update_all_to_area (area);
              }

            if ((name = PLIST_NAME (area, FILTER_OUT)))
              {
                pl = PLIST_LIST (area, FILTER_OUT);

                PLIST_LIST (area, FILTER_OUT) =
                  prefix_list_lookup (vr, AFI_IP, name);

                if (IS_AREA_ACTIVE (area))
                  if (pl != NULL
                      || PLIST_LIST (area, FILTER_OUT) != NULL)
                    ospf_abr_ia_update_area_to_all (area);
              }
          }
    }
}

void
ospf_global_init (void)
{
  /* Initialize OSPF common constants. */
  ospf_const_init ();

  /* Initialize OSPFv2 constants. */
  IPv4AllSPFRouters.s_addr = pal_hton32 (OSPFV2_ALLSPFROUTERS);
  IPv4AllDRouters.s_addr = pal_hton32 (OSPFV2_ALLDROUTERS);
  IPv4AddressUnspec.s_addr = pal_ntoh32 (IPV4_ADDRESS_UNSPEC);
  IPv4LoopbackAddress.s_addr = pal_hton32 (IPV4_ADDRESS_LOOPBACK);
  IPv4HostAddressMask.s_addr = pal_hton32 (0xFFFFFFFF);
  ls_prefix_ipv4_set (&LsPrefixIPv4Default, 0, IPv4AddressUnspec);
#ifdef HAVE_MD5
  pal_mem_set (&OSPFMessageDigestKeyUnspec, 0, OSPF_AUTH_MD5_SIZE);
#endif /* HAVE_MD5 */
  OSPFLSARefreshEventInterval.tv_sec = 0;
  OSPFLSARefreshEventInterval.tv_usec = OSPF_LSA_REFRESH_EVENT_INTERVAL;
  ls_prefix_ipv4_set (&DefaultIPv4, 32, IPv4AddressUnspec);
}

int
ospf_master_init (struct apn_vr *vr)
{
  struct ospf_master *om;
  int i;

  om = XCALLOC (MTYPE_OSPF_MASTER, sizeof (struct ospf_master));
  om->zg = ZG;
  om->vr = vr;
  vr->proto = om;

  /* Initialize ospf instance list. */
  om->ospf = list_new ();
  om->ospf->cmp = ospf_proc_cmp;

  /* Initialize ospf global flags. */
  om->flags = 0;

  /* Initialize system wide interface configurations. */
  ospf_if_master_init (om);

  /* Initialize Default VRF. */
  ospf_vrf_get (om, NULL);

  /* Initialize ospf notifier list. */
  for (i = 0; i < OSPF_NOTIFY_MAX; i++)
    {
      om->notifiers[i] = list_new ();
      om->notifiers[i]->cmp = ospf_notifier_cmp;
      om->notifiers[i]->del = ospf_notifier_free;
    }

  /* Initialize route-map. */
  ospf_route_map_init (om);

  /* Initialize SNMP Community. */
  ospf_snmp_community_init (om);

  /* Initialize Access list.  */
  access_list_add_hook (vr, ospf_access_list_update);
  access_list_delete_hook (vr, ospf_access_list_update);

  /* Initialize Prefix list.  */
  prefix_list_add_hook (vr, ospf_prefix_list_update);
  prefix_list_delete_hook (vr, ospf_prefix_list_update);

#ifdef HAVE_OSPF_TE
  ospf_te_init (om);
#endif /* HAVE_OSPF_TE */

#ifdef HAVE_SNMP
  for (i = 0; i < OSPF_TRAP_ID_MAX; i++)
    om->traps[i] = vector_init (OSPF_TRAP_VEC_MIN_SIZE);
#endif /* HAVE_SNMP */

#ifdef HAVE_OSPF_CSPF
  /* Initialize CSPF stuff */
  cspf_init (om);
#endif /* HAVE_OSPF_CSPF */

#ifdef HAVE_RESTART 
   om->never_restart_helper_list = list_new();
#endif /* HAVE_RESTART */

  return 0;
}

int
ospf_master_finish (struct apn_vr *vr)
{
  struct ospf_master *om = vr->proto;
  struct apn_vrf *iv;
  int i;

  if (om == NULL)
    return 0;

#ifdef HAVE_RESTART
  if (GET_LIB_STOP_CAUSE(om->zg) != MOD_STOP_CAUSE_GRACE_RST)
    {
      /* Finish the restart feature.  */
      ospf_restart_finish (om);
      list_delete (om->never_restart_helper_list);
    }
#endif /* HAVE_RESTART */

  /* Delete VRF list. */
  for (i = 0; i < vector_max (om->vr->vrf_vec); i++)
    if ((iv = vector_slot (om->vr->vrf_vec, i)))
      ospf_vrf_delete (iv);

  /* Delete system wide interface configurations.  */
  ospf_if_master_finish (om);

#ifdef HAVE_OSPF_CSPF
  cspf_finish (om);
#endif /* HAVE_OSPF_CSPF */

  /* Delete OSPF instance list. */
  list_delete (om->ospf);

  /* Delete ospf notifier list. */
  for (i = 0; i < OSPF_NOTIFY_MAX; i++)
    list_delete (om->notifiers[i]);

#ifdef HAVE_SNMP
  for (i = 0; i < OSPF_TRAP_ID_MAX; i++)
    vector_free (om->traps[i]);
#endif /* HAVE_SNMP */

  vr->proto = NULL;

  XFREE (MTYPE_OSPF_MASTER, om);

  return 0;
}

struct ospf_master *
ospf_master_lookup_default (struct lib_globals *zg)
{
  struct apn_vr *vr = apn_vr_get_privileged (zg);

  if (vr != NULL)
    return (struct ospf_master *)vr->proto;

  return NULL;
}

struct ospf_master *
ospf_master_lookup_by_id (struct lib_globals *zg, u_int32_t id)
{
  struct apn_vr *vr;

  vr = apn_vr_lookup_by_id (zg, id);
  if (vr != NULL)
    return (struct ospf_master *)vr->proto;

  return NULL;
}

/* Initialize OSPF stuffs. */
void
ospf_init (struct lib_globals *zg)
{
  /* Initialize nsm related stuff. */
  ospf_nsm_init (zg);

  /* Initlaize OSPF VRF. */
  ospf_vrf_init (zg);

#ifdef HAVE_BFD
  ospf_bfd_init (zg);
#endif /* HAVE_BFD */

  /* Register VR callback. */
  apn_vr_add_callback (zg, VR_CALLBACK_ADD, ospf_master_init);
  apn_vr_add_callback (zg, VR_CALLBACK_DELETE, ospf_master_finish);
}

void
ospf_disconnect_clean ()
{
#ifdef HAVE_GMPLS
  struct telink *tl = NULL;
  struct avl_node *tl_node = NULL;
  struct ospf_telink *os_tel = NULL;

  /* Only GMPLS related structure are cleaned right now */
  /* Loop through the telink table and deactivate TE Link*/
  AVL_TREE_LOOP (&ZG->gmif->teltree, tl, tl_node)
    {
      if (tl->info == NULL)
        continue;
      else
        {
          os_tel = (struct ospf_telink *)tl->info;
          ospf_deactivate_te_link (os_tel);
        }
    }

  avl_tree_cleanup (&ZG->gmif->teltree, te_link_clean);
  avl_finish2 (&ZG->gmif->teltree);
#endif /* HAVE_GMPLS */
}

void
ospf_terminate (struct lib_globals *zg)
{
  struct apn_vr *vr;
  int i;

  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      ospf_master_finish (vr);

#ifdef HAVE_BFD
  ospf_bfd_term (zg);
#endif /* HAVE_BFD */
}
