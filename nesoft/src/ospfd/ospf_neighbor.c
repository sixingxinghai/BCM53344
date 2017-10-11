/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_ifsm.h"
#include "ospfd/ospf_nfsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"


int
ospf_nbr_entry_insert (struct ospf *top, struct ospf_neighbor *nbr)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  unsigned int ifindex;
  int ret = OSPF_API_ENTRY_INSERT_ERROR;

  if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK)
    return OSPF_API_ENTRY_INSERT_SUCCESS;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_NBR_TABLE_DEPTH;
  p.prefixlen = OSPF_NBR_TABLE_DEPTH * 8;

  ifindex = ospf_address_less_ifindex (nbr->oi);

  /* Set Neighbor IP address and Neighbor Interface index as index.*/
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_NBR_TABLE].vars,
                      &nbr->ident.address->u.prefix4, &ifindex);

  rn = ls_node_get (top->nbr_table, (struct ls_prefix *)&p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      RN_INFO_SET (rn, RNI_DEFAULT, nbr);
      ret = OSPF_API_ENTRY_INSERT_SUCCESS;
    }
  ls_unlock_node (rn);
  return ret;
}

int
ospf_nbr_entry_delete (struct ospf *top, struct ospf_neighbor *nbr)
{
  struct ls_node *rn;
  struct ls_prefix8 p;
  unsigned int ifindex;
  int ret = OSPF_API_ENTRY_DELETE_ERROR;

  if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK)
    return OSPF_API_ENTRY_DELETE_SUCCESS;

  pal_mem_set (&p, 0, sizeof (struct ls_prefix8));
  p.prefixsize = OSPF_NBR_TABLE_DEPTH;
  p.prefixlen = OSPF_NBR_TABLE_DEPTH * 8;

  ifindex = ospf_address_less_ifindex (nbr->oi);

  /* Set Neighbor IP address and Neighbor Interface index as index.*/
  ls_prefix_set_args ((struct ls_prefix *)&p,
                      ospf_api_table_def[OSPF_NBR_TABLE].vars,
                      &nbr->ident.address->u.prefix4, &ifindex);

  rn = ls_node_lookup (top->nbr_table, (struct ls_prefix *)&p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      ret = OSPF_API_ENTRY_DELETE_SUCCESS;
    }
  return ret;
}


struct ospf_neighbor *
ospf_nbr_new (struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  int i;

  /* Allcate new neighbor. */
  nbr = XMALLOC (MTYPE_OSPF_NEIGHBOR, sizeof (struct ospf_neighbor));
  pal_mem_set (nbr, 0, sizeof (struct ospf_neighbor));

  /* Relate neighbor to the interface. */
  nbr->oi = oi;

  /* Set default values. */
  nbr->state = NFSM_Down;
  nbr->ostate = NFSM_Down;

  /* Set create time. */
  nbr->ctime = pal_time_current (NULL);

  /* Initialize counters */
  nbr->rxmt_count = 0;
  nbr->ack_count = 0;
  nbr->is_congested = 0;

  /* Set inheritance values. */
  nbr->v_inactivity = ospf_if_dead_interval_get (oi);
  nbr->v_dd_inactivity = OSPF_DD_INACTIVITY_TIMER_DEFAULT;
  nbr->v_retransmit = ospf_if_retransmit_interval_get (oi);
  nbr->v_retransmit_max = (8 * nbr->v_retransmit);
  nbr->ident.priority = 0;
  nbr->ident.address = prefix_new ();

  /* Init DD flags. */
  nbr->dd.flags = OSPF_DD_FLAG_ALL;

  /* Initialize lists. */
  nbr->db_sum = ospf_lsdb_init ();
  for(i = 0 ; i < OSPF_RETRANSMIT_LISTS; i++)
    nbr->ls_rxmt_list[i]  = ospf_lsdb_init ();

  nbr->ls_req = ls_table_init (OSPF_LS_REQUEST_TABLE_DEPTH, 1);

  /* Set initial flag.*/
  SET_FLAG (nbr->flags, OSPF_NEIGHBOR_INIT);

  return nbr;
}

void
ospf_nbr_free (struct ospf_neighbor *nbr)
{
  int i;

  /* Free DB summary list. */
  if (ospf_db_summary_count (nbr))
    ospf_db_summary_clear (nbr);

  /* Free ls request list. */
  if (ospf_ls_request_count (nbr))
    ospf_ls_request_delete_all (nbr);

  /* Free retransmit list. */
  if (ospf_ls_retransmit_count (nbr))
    ospf_ls_retransmit_clear (nbr);

  /* Cleanup LSDBs. */
  ospf_lsdb_finish (nbr->db_sum);
  for (i = 0; i < OSPF_RETRANSMIT_LISTS; i++)
    {
      if (nbr->ls_rxmt_list[i] != NULL)
        ospf_lsdb_finish (nbr->ls_rxmt_list[i]);
    }  
  ls_table_finish (nbr->ls_req);

  /* Clear the last sent packet.  */
  if (nbr->dd.sent.packet != NULL)
    ospf_packet_add_unuse (nbr->oi->top, nbr->dd.sent.packet);

  /* Free prefix. */
  prefix_free (nbr->ident.address);

  /* Cancel all timers. */
  OSPF_NFSM_TIMER_OFF (nbr->t_inactivity);
  OSPF_NFSM_TIMER_OFF (nbr->t_dd_inactivity);
  OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
  OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
  OSPF_NFSM_TIMER_OFF (nbr->t_ls_req);
  OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);
  OSPF_NFSM_TIMER_OFF (nbr->t_ls_ack_cnt);
#ifdef HAVE_RESTART
  OSPF_NFSM_TIMER_OFF (nbr->t_restart);
#endif /* HAVE_RESTART */

#ifdef HAVE_BFD
  OSPF_NFSM_TIMER_OFF (nbr->t_bfd_down);
#endif /* HAVE_BFD */

  /* Clear out any pending events to prevent ospfd crashes. */
  thread_cancel_event (ZG, nbr);

  XFREE (MTYPE_OSPF_NEIGHBOR, nbr);
}

/* Get neighbor count by state. */
int
ospf_nbr_count_by_state (struct ospf_interface *oi, int state)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  int count = 0;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->state == state)
        count++;

  return count;
}

int
ospf_nbr_count_all (struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  int count = 0;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      count++;

  return count;
}

int
ospf_nbr_count_full (struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  int count = 0;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_NBR_FULL (nbr))
        count++;

  return count;
}

#ifdef HAVE_RESTART
int
ospf_nbr_count_restart (struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  int count = 0;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART)
          || CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS))
        count++;

  return count;
}
#endif /* HAVE_RESTART */

struct ospf_neighbor *
ospf_nbr_lookup_by_addr (struct ls_table *nbrs, struct pal_in4_addr *addr)
{
  struct ls_prefix p;
  struct ls_node *rn;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, *addr);
  rn = ls_node_lookup (nbrs, &p);
  if (rn != NULL)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }

  return NULL;
}

struct ospf_neighbor *
ospf_nbr_lookup_by_router_id (struct ls_table *nbrs,
                              struct pal_in4_addr *router_id)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  for (rn = ls_table_top (nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (IPV4_ADDR_SAME (&nbr->ident.router_id, router_id))
        {
          ls_unlock_node (rn);
          return nbr;
        }

  return NULL;
}

/* Lookup neighbor other than myself. */
struct ospf_neighbor *
ospf_nbr_lookup_ptop (struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  /* Search neighbor, there must be one of two nbrs. */
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (!IPV4_ADDR_SAME (&nbr->ident.router_id, &oi->ident.router_id))
        {
          ls_unlock_node (rn);
          return nbr;
        }

  return NULL;
}

struct ospf_neighbor *
ospf_nbr_get (struct ospf_interface *oi, struct pal_in4_addr addr,
              u_char prefixlen)
{
  struct ospf *top = oi->top;
  struct ospf_master *om = top->om;
  struct ospf_neighbor *nbr;
  struct ls_prefix lp;
  struct ls_node *rn;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_VIRTUALLINK)
    {
      nbr = ospf_nbr_lookup_ptop (oi);
      if (nbr != NULL)
        {
          if (IPV4_ADDR_SAME (&nbr->ident.address->u.prefix4, &addr))
            return nbr;

          OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_KillNbr);
        }
    }
  
  /* Initialize lp */
  pal_mem_set (&lp, 0, sizeof (struct ls_prefix));

  ls_prefix_ipv4_set (&lp, IPV4_MAX_BITLEN, addr);
  rn = ls_node_get (oi->nbrs, &lp);
  lp.prefixlen = prefixlen;
  if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
    {
      /* Unset initial flag. */
      UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_INIT);

      if (oi->type == OSPF_IFTYPE_NBMA && nbr->state == NFSM_Attempt)
        pal_mem_cpy (nbr->ident.address, &lp, sizeof (struct ls_prefix));
    }
  else
    {
      /* Create new OSPF Neighbor structure. */
      nbr = ospf_nbr_new (oi);
      pal_mem_cpy (nbr->ident.address, &lp, sizeof (struct ls_prefix));

      RN_INFO_SET (rn, RNI_DEFAULT, nbr);
      ospf_nbr_entry_insert (top, nbr);

      if (IS_DEBUG_OSPF (nfsm, NFSM_EVENTS))
        zlog_info (ZG, "NFSM[%s]: Start", NBR_NAME (nbr));
    }
  ls_unlock_node (rn);

  return nbr;
}

/* Delete specified OSPF neighbor from interface. */
void
ospf_nbr_delete (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  struct ls_node *rn;
  struct ls_prefix p;

  /* Unlink ospf neighbor from the interface. */
  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, nbr->ident.address->u.prefix4);
  rn = ls_node_lookup (oi->nbrs, &p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
    }

  ospf_nbr_entry_delete (oi->top, nbr);
  ospf_nexthop_nbr_down (nbr);
  ospf_link_vec_unset_by_nbr (oi->area, nbr);

  /* Free ospf_neighbor structure. */
  ospf_nbr_free (nbr);
}

int
ospf_nbr_message_digest_rollover_state_check (struct ospf_interface *oi)
{
  struct ls_node *rn;
  struct ospf_neighbor *nbr;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_MD5_KEY_ROLLOVER)) 
        {
          ls_unlock_node (rn);
          return 1;
        }

  return 0;
}

struct ospf_neighbor *
ospf_nbr_add_dd_pending (struct ospf_neighbor *nbr)
{
  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_DD_PENDING))
    return NULL;

  SET_FLAG (nbr->flags, OSPF_NEIGHBOR_DD_PENDING);

  nbr->prev = nbr->oi->top->dd_nbr_tail;
  nbr->next = NULL;

  if (nbr->oi->top->dd_nbr_tail != NULL)
    nbr->oi->top->dd_nbr_tail->next = nbr;
  else
    nbr->oi->top->dd_nbr_head = nbr;
  nbr->oi->top->dd_nbr_tail = nbr;

  return nbr;
}

struct ospf_neighbor *
ospf_nbr_delete_dd_pending (struct ospf_neighbor *nbr)
{
  if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_DD_PENDING))
    return NULL;

  if (nbr->prev != NULL)
    nbr->prev->next = nbr->next;
  else
    nbr->oi->top->dd_nbr_head = nbr->next;
  if (nbr->next != NULL)
    nbr->next->prev = nbr->prev;
  else
    nbr->oi->top->dd_nbr_tail = nbr->prev;

  nbr->prev = NULL;
  nbr->next = NULL;

  /* Reset the flag.  */
  UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_DD_PENDING);

  /* Cancel the DD inactivity timer.  */
  OSPF_TIMER_OFF (nbr->t_dd_inactivity);

  return nbr;
}

void
ospf_nbr_dd_pending_check (struct ospf *top)
{
  int remain_out = top->max_dd - top->dd_count_out;
  int remain_in = top->max_dd - top->dd_count_in;
  struct ospf_neighbor *nbr = NULL;
  struct ospf_neighbor *nbr_tmp = NULL;


  /* Schedule AdjOK event for pending DD neighbors.  */
  for (nbr = top->dd_nbr_head; nbr; nbr = nbr_tmp->next)
    {
      nbr_tmp = nbr;
      if (remain_in <= 0 && remain_out <= 0)
        break;
      else
        {
          if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_DD_INIT))
            {
              if (remain_in > 0)
                if ((nbr = ospf_nbr_delete_dd_pending (nbr)) != NULL)
                  {
                    OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_AdjOK);
                    remain_in--;
                  }
            }
          else
            {
              if (remain_out > 0)
                if ((nbr = ospf_nbr_delete_dd_pending (nbr)) != NULL)
                  {
                    OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_AdjOK);
                    remain_out--;
                  }
            }
        }
    }
}


/* NBMA neighbor. */
struct ospf_nbr_static *
ospf_nbr_static_new ()
{
  struct ospf_nbr_static *nbr;

  nbr = XMALLOC (MTYPE_OSPF_NEIGHBOR_STATIC, sizeof (struct ospf_nbr_static));
  pal_mem_set (nbr, 0, sizeof (struct ospf_nbr_static));

  nbr->priority = OSPF_NEIGHBOR_PRIORITY_DEFAULT;
  nbr->poll_interval = OSPF_POLL_INTERVAL_DEFAULT;
  nbr->cost = OSPF_OUTPUT_COST_DEFAULT;
  nbr->status = ROW_STATUS_ACTIVE;

  return nbr;
}

void
ospf_nbr_static_free (struct ospf_nbr_static *nbr_static)
{
  XFREE (MTYPE_OSPF_NEIGHBOR_STATIC, nbr_static);
}

void
ospf_nbr_static_add (struct ospf *top, struct ospf_nbr_static *nbr_static)
{
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, nbr_static->addr);
  rn = ls_node_get (top->nbr_static, &p);
  if ((RN_INFO (rn, RNI_DEFAULT)) == NULL)
    RN_INFO_SET (rn, RNI_DEFAULT, nbr_static);

  ls_unlock_node (rn);
}

void
ospf_nbr_static_up (struct ospf_nbr_static *nbr_static,
                    struct ospf_interface *oi)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  struct ls_prefix p;

  /* Initialize p */
  pal_mem_set (&p, 0, sizeof (struct ls_prefix));

  /* Get neighbor information from table. */
  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, nbr_static->addr);

  rn = ls_node_get (oi->nbrs, &p);
  if ((nbr = RN_INFO (rn, RNI_DEFAULT)) == NULL)
    {
      nbr = ospf_nbr_new (oi);
      pal_mem_cpy (nbr->ident.address, &p, sizeof (struct ls_prefix));

      RN_INFO_SET (rn, RNI_DEFAULT, nbr);
      ospf_nbr_entry_insert (oi->top, nbr);
    }
  SET_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC);
  nbr_static->nbr = nbr;
  nbr->ident.priority = nbr_static->priority;

  OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_Start);

  ls_unlock_node (rn);
}

void
ospf_nbr_static_reset (struct ospf_interface *oi, struct pal_in4_addr addr)
{
  struct ospf_nbr_static *nbr_static;

  nbr_static = ospf_nbr_static_lookup (oi->top, addr);
  if (nbr_static)
    {
      OSPF_TIMER_OFF (nbr_static->t_poll);
      nbr_static->nbr = NULL;
    }
}

void
ospf_nbr_static_if_update (struct ospf_interface *oi)
{
  struct ospf *top = oi->top;
  struct ospf_nbr_static *nbr_static;
  struct ls_node *rn, *rn_tmp;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, oi->ident.address->prefixlen,
                      oi->ident.address->u.prefix4);

  rn_tmp = ls_node_get (top->nbr_static, &p);
  for (rn = ls_lock_node (rn_tmp); rn; rn = ls_route_next_until (rn, rn_tmp))
    if ((nbr_static = RN_INFO (rn, RNI_DEFAULT)))
      ospf_nbr_static_up (nbr_static, oi);

  ls_unlock_node (rn_tmp);
}

struct ospf_nbr_static *
ospf_nbr_static_lookup (struct ospf *top, struct pal_in4_addr addr)
{
  struct ls_node *rn;
  struct ls_prefix p;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, addr);

  rn = ls_node_lookup (top->nbr_static, &p);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }
  return NULL;
}

void
ospf_nbr_static_delete (struct ospf *top, struct ospf_nbr_static *nbr_static)
{
  struct ls_prefix p;
  struct ls_node *rn;

  ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, nbr_static->addr);
  rn = ls_node_lookup (top->nbr_static, &p);
  if (rn)
    {
      RN_INFO_UNSET (rn, RNI_DEFAULT);

      if (nbr_static->nbr)
        {
          if (nbr_static->nbr->state == NFSM_Down
              || nbr_static->nbr->state == NFSM_Attempt)
            ospf_nbr_delete (nbr_static->nbr);
          else
            {
              OSPF_NFSM_EVENT_EXECUTE (nbr_static->nbr, NFSM_KillNbr);
              UNSET_FLAG (nbr_static->nbr->flags, OSPF_NEIGHBOR_STATIC);
            }
        }

      OSPF_TIMER_OFF (nbr_static->t_poll);

      ospf_nbr_static_free (nbr_static);
      ls_unlock_node (rn);
    }
}

void
ospf_nbr_static_poll_timer_cancel (struct ospf_neighbor *nbr)
{
  struct ospf_nbr_static *nbr_static;

  if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC))
    return;

  nbr_static = ospf_nbr_static_lookup (nbr->oi->top,
                                       nbr->ident.address->u.prefix4);
  if (nbr_static)
    OSPF_TIMER_OFF (nbr_static->t_poll);
}

void
ospf_nbr_static_cost_apply (struct ospf *top,
                            struct ospf_nbr_static *nbr_static)
{
  struct ospf_interface *oi;
  struct ls_node *rn;

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if (oi->type == OSPF_IFTYPE_POINTOMULTIPOINT)
        if (ospf_nbr_lookup_by_addr (oi->nbrs, &nbr_static->addr))
          if (IS_AREA_ACTIVE (oi->area))
            LSA_REFRESH_TIMER_ADD (top, OSPF_ROUTER_LSA,
                                   ospf_router_lsa_self (oi->area), oi->area);
}

void
ospf_nbr_static_priority_apply (struct ospf_nbr_static *nbr_static,
                                u_char priority)
{
  nbr_static->priority = priority;
  if (nbr_static->priority != OSPF_NEIGHBOR_PRIORITY_DEFAULT)
    SET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_PRIORITY);
  else
    UNSET_FLAG (nbr_static->flags, OSPF_NBR_STATIC_PRIORITY);

  if (nbr_static->nbr != NULL)
    {
      nbr_static->nbr->ident.priority = priority;

      /* Schedule NeighborChange IFSM event.  */
      if (!CHECK_FLAG (nbr_static->nbr->flags, OSPF_NEIGHBOR_INIT))
        OSPF_IFSM_EVENT_SCHEDULE (nbr_static->nbr->oi, IFSM_NeighborChange);
    }
}
