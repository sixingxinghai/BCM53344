/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "linklist.h"
#include "thread.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#ifdef HAVE_OSPF_MULTI_AREA
#include "ospfd/ospf_multi_area_link.h"
#endif /* HAVE_OSPF_MULTI_AREA */
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_ifsm.h"
#include "ospfd/ospf_nfsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"
#include "ospfd/ospf_vrf.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_SNMP
#include "ospfd/ospf_snmp.h"
#endif /* HAVE_SNMP */

#define DN_OPTIONS_MASK (OSPF_OPTION_DN)


unsigned long
ospf_ls_request_count (struct ospf_neighbor *nbr)
{
  return nbr->ls_req->count[0];
}

int
ospf_ls_request_isempty (struct ospf_neighbor *nbr)
{
  return ((ospf_ls_request_count (nbr)) == 0);
}

/* Management functions for neighbor's Link State Request list. */
struct ospf_ls_request *
ospf_ls_request_new (struct lsa_header *lsah)
{
  struct ospf_ls_request *lsr;

  lsr = XMALLOC (MTYPE_OSPF_LS_REQUEST, sizeof (struct ospf_ls_request));
  pal_mem_cpy (&lsr->data, lsah, sizeof (struct lsa_header));

  return lsr;
}

void
ospf_ls_request_free (struct ospf_ls_request *lsr)
{
  XFREE (MTYPE_OSPF_LS_REQUEST, lsr);
}

void
ospf_ls_request_prefix_set (struct ls_prefix_lsr *lp, struct lsa_header *lsah)
{
  pal_mem_set (lp, 0, sizeof (struct ls_prefix_lsr));
  lp->family = 0;
  lp->prefixsize = OSPF_LS_REQUEST_TABLE_DEPTH;
  lp->prefixlen = OSPF_LS_REQUEST_TABLE_DEPTH * 8;
  lp->type = pal_hton32 (lsah->type);
  lp->id = lsah->id;
  lp->adv_router = lsah->adv_router;
}

void
ospf_ls_request_add (struct ospf_neighbor *nbr, struct ospf_ls_request *lsr)
{
  struct ospf_ls_request *old;
  struct ls_prefix_lsr lp;
  struct ls_node *rn;

  if (lsr->data.type < OSPF_MIN_LSA || lsr->data.type >= OSPF_MAX_LSA)
    return;

  ospf_ls_request_prefix_set (&lp, &lsr->data);
  rn = ls_node_get (nbr->ls_req, (struct ls_prefix *)&lp);
  if ((old = RN_INFO (rn, RNI_LSDB_LSA)) != NULL)
    {
      ospf_ls_request_free (old);
      ls_unlock_node (rn);
    }

  ls_node_info_set (rn, RNI_LSDB_LSA, lsr);
}

void
ospf_ls_request_delete (struct ospf_neighbor *nbr, struct ospf_ls_request *lsr)
{
  struct ls_prefix_lsr lp;
  struct ls_node *rn;

  if (lsr->data.type < OSPF_MIN_LSA || lsr->data.type >= OSPF_MAX_LSA)
    return;

  if (nbr->ls_req_last == lsr)
    nbr->ls_req_last = NULL;

  ospf_ls_request_prefix_set (&lp, &lsr->data);
  rn = ls_node_lookup (nbr->ls_req, (struct ls_prefix *)&lp);
  if (rn)
    {
      ospf_ls_request_free (RN_INFO (rn, RNI_DEFAULT));
      ls_node_info_unset (rn, RNI_DEFAULT);
      ls_unlock_node (rn);
      ls_unlock_node (rn);
    }
}

void
ospf_ls_request_delete_all (struct ospf_neighbor *nbr)
{
  struct ls_node *rn;

  for (rn = ls_table_top (nbr->ls_req); rn; rn = ls_route_next (rn))
    if (RN_INFO (rn, RNI_DEFAULT))
      {
        ospf_ls_request_free (RN_INFO (rn, RNI_DEFAULT));
        ls_node_info_unset (rn, RNI_DEFAULT);
        ls_unlock_node (rn);
      }
}

struct ospf_ls_request *
ospf_ls_request_lookup (struct ospf_neighbor *nbr, struct lsa_header *lsah)
{
  struct ls_prefix_lsr lp;
  struct ls_node *rn;

  if (lsah->type < OSPF_MIN_LSA || lsah->type >= OSPF_MAX_LSA)
    return NULL;

  ospf_ls_request_prefix_set (&lp, lsah);
  rn = ls_node_lookup (nbr->ls_req, (struct ls_prefix *)&lp);
  if (rn)
    {
      ls_unlock_node (rn);
      return RN_INFO (rn, RNI_DEFAULT);
    }

  return NULL;
}

struct ospf_lsdb *
ospf_ls_retransmit_list_find (struct ospf_neighbor *nbr )
{
  u_int32_t rxmt_list;
  pal_time_t now;

  now = pal_time_current (NULL);
  rxmt_list = (now % OSPF_RETRANSMIT_LISTS);

  return nbr->ls_rxmt_list [rxmt_list];
}

/* Management functions for neighbor's ls-retransmit list. */
unsigned long
ospf_ls_retransmit_count (struct ospf_neighbor *nbr)
{
  unsigned long count;
  int i;
  count = 0;

  /*Count all the retransmit lists */
  for(i = 0; i < OSPF_RETRANSMIT_LISTS; i++)
    count += ospf_lsdb_count_all (nbr->ls_rxmt_list[i]);

  return count;
}

int
ospf_ls_retransmit_isempty (struct ospf_neighbor *nbr)
{
  int i, check;
  check = 0;
  
  /* Check all the retransmit lists */		
  for (i = 0; i < OSPF_RETRANSMIT_LISTS; i++)
    {
      check = ospf_lsdb_isempty (nbr->ls_rxmt_list[i]);
      if (check == 0)
        break;
    }
  return check;
}

struct ospf_lsa *
ospf_ls_retransmit_lookup (struct ospf_neighbor *nbr, struct ospf_lsa *lsa)
{
  struct ospf_lsdb *lsdb;
  struct ospf_lsa *find;
  int i;
 
  for (i = 0 ; i < OSPF_RETRANSMIT_LISTS; i++)
    {
      lsdb = nbr->ls_rxmt_list[i];
      find = ospf_lsdb_lookup (lsdb, lsa);
      if (find != NULL)
        {
          find->rxmt_list = lsdb;
          return find;
        }
    }
  return NULL;
}

struct ospf_lsa *
ospf_ls_retransmit_lookup_by_id (struct ospf_neighbor *nbr, u_char type,
                           struct pal_in4_addr id, struct pal_in4_addr adv_router)
{
  struct ospf_lsa *find;
  struct ospf_lsdb *lsdb;
  int i;
  
  for (i = 0 ; i < OSPF_RETRANSMIT_LISTS; i++)
    {
      lsdb = nbr->ls_rxmt_list[i];
      find = ospf_lsdb_lookup_by_id (lsdb, type, id, adv_router);
      if (find != NULL)
        return find;
    }
  return NULL;
}

/* Add LSA to be retransmitted to neighbor's ls-retransmit list. */
void
ospf_ls_retransmit_add (struct ospf_neighbor *nbr, struct ospf_lsa *lsa)
{
  struct ospf_lsa *old = ospf_ls_retransmit_lookup (nbr, lsa);
  struct ospf_lsdb *ls_rxmt;

  if (ospf_lsa_more_recent (old, lsa) < 0)
    {
      if (old)
        {
          old->rxmt_count--;
          ospf_lsdb_delete (old->rxmt_list, old);
        }
      lsa->rxmt_count++;
      ls_rxmt = ospf_ls_retransmit_list_find (nbr);
      ospf_lsdb_add (ls_rxmt, lsa);

      /* Start the retransmit timer.  */
      OSPF_NFSM_TIMER_ON (nbr->t_ls_upd, ospf_ls_upd_timer,
                          ospf_ls_upd_timer_interval (nbr));
    }
}

/* Remove LSA from neibghbor's ls-retransmit list. */
void
ospf_ls_retransmit_delete (struct ospf_neighbor *nbr, struct ospf_lsa *lsa)
{
  struct ospf_lsa *old = ospf_ls_retransmit_lookup (nbr, lsa);

  if (old && ospf_lsa_more_recent (old, lsa) == 0)
    {
      old->rxmt_count--;
      ospf_lsdb_delete (old->rxmt_list, old);

      /* Stop the retransmit timer.  */
      if (ospf_ls_retransmit_isempty (nbr))
        OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);
    }
}

/* Clear neighbor's ls-retransmit list. */
void
ospf_ls_retransmit_clear (struct ospf_neighbor *nbr)
{
  struct ospf_lsdb *lsdb;
  struct ls_node *rn;
  struct ospf_lsa *lsa;
  int i,j;
          
  for (i = 0 ; i < OSPF_RETRANSMIT_LISTS; i++)
    {
      lsdb = nbr->ls_rxmt_list[i];
      for (j = OSPF_MIN_LSA; j < OSPF_MAX_LSA; j++)
        for (rn = ls_table_top (lsdb->type[j].db); rn; rn = ls_route_next (rn))
          if ((lsa = RN_INFO (rn, RNI_DEFAULT)) != NULL)
            ospf_ls_retransmit_delete (nbr, lsa);
    }

  nbr->ls_req_last = NULL;
}

/* Remove All neighbor/interface's Link State Retransmit list in area. */
void
ospf_ls_retransmit_delete_nbr_if (struct ospf_interface *oi,
                                  struct ospf_lsa *lsa, int del)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  if (ospf_if_is_up (oi))
    if (oi->type != OSPF_IFTYPE_LOOPBACK)
      for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
        if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
          {
            if (pal_ntoh16 (lsa->data->ls_age) == OSPF_LSA_MAXAGE &&
              del != PAL_TRUE)
              return;
            else
              ospf_ls_retransmit_delete (nbr, lsa);
          }
}

void
ospf_ls_retransmit_delete_nbr_area (struct ospf_area *area,
                                    struct ospf_lsa *lsa, int del)
{
  struct ospf_interface *oi;
  struct listnode *node;

  LIST_LOOP (area->iflist, oi, node)
    ospf_ls_retransmit_delete_nbr_if (oi, lsa, del);
}

void
ospf_ls_retransmit_delete_nbr_as (struct ospf *top, struct ospf_lsa *lsa,
                                int del)
{
  struct ospf_interface *oi;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      ospf_ls_retransmit_delete_nbr_if (oi, lsa, del);

#ifdef HAVE_OSPF_MULTI_AREA
  for (rn = ls_table_top (top->multi_area_link_table); rn;
                                             rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        ospf_ls_retransmit_delete_nbr_if (mlink->oi, lsa, del);
#endif /* HAVE_OSPF_MULTI_AREA */
}

void
ospf_ls_retransmit_delete_nbr_by_scope (struct ospf_lsa *lsa, int del)
{
  switch (lsa->lsdb->scope)
    {
    case LSA_FLOOD_SCOPE_LINK:
      ospf_ls_retransmit_delete_nbr_if (lsa->lsdb->u.oi, lsa, del);
      break;
    case LSA_FLOOD_SCOPE_AREA:
      ospf_ls_retransmit_delete_nbr_area (lsa->lsdb->u.area, lsa, del);
      break;
    case LSA_FLOOD_SCOPE_AS:
      ospf_ls_retransmit_delete_nbr_as (lsa->lsdb->u.top, lsa, del);
      break;
    }
}


/* OSPF LSA flooding -- RFC2328 Section 13.3. */
int
ospf_flood_through_interface (struct ospf_interface *oi,
                              struct ospf_neighbor *inbr,
                              struct ospf_lsa *lsa)
{
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  char if_str[OSPF_IF_STRING_MAXLEN];
  struct ospf_master *om = oi->top->om;
  struct ospf_neighbor *onbr;
  struct ls_node *rn;
  int retx_flag;
  int cong_flag;

  if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
    zlog_info (ZG, "LSA[%s]: Flooding via interface[%s]",
               LSA_NAME (lsa), IF_NAME (oi));

  if (!ospf_if_is_up (oi) || oi->type == OSPF_IFTYPE_LOOPBACK)
    return 0;

  /* If database-filter is set, we don't flood any LSA to this interface. */
  if (ospf_if_database_filter_get (oi))
    return 0;

  /* Remember if new LSA is aded to a retransmit list. */
  retx_flag = 0;	
  cong_flag = 0;

  /* Each of the neighbors attached to this interface are examined,
     to determine whether they must receive the new LSA.  The following
     steps are executed for each neighbor: */
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    {
      struct ospf_ls_request *ls_req;

      if ((onbr = RN_INFO (rn, RNI_DEFAULT)) == NULL)
        continue;

      if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
        zlog_info (ZG, "LSA[%s]: Flooding to neighbor[%r]",
                   LSA_NAME (lsa), &onbr->ident.router_id);

      /* If the neighbor is in a lesser state than Exchange, it
         does not participate in flooding, and the next neighbor
         should be examined. */
      if (onbr->state < NFSM_Exchange)
        continue;

      /* If the adjacency is not yet full (neighbor state is
         Exchange or Loading), examine the Link state request
         list associated with this adjacency.  If there is an
         instance of the new LSA on the list, it indicates that
         the neighboring router has an instance of the LSA
         already.  Compare the new LSA to the neighbor's copy: */
      if (onbr->state < NFSM_Full)
        {
          if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
            zlog_info (ZG, "LSA[%s]: neighbor is not Full state",
                       LSA_NAME (lsa));

          if (onbr->is_congested)
            cong_flag = 1;  

          ls_req = ospf_ls_request_lookup (onbr, lsa->data);
          if (ls_req != NULL)
            {
              int ret;

              ret = ospf_lsa_header_more_recent (&ls_req->data, lsa);
              /* The new LSA is less recent. */
              if (ret > 0)
                continue;
              /* The two copies are the same instance, then delete
                 the LSA from the Link state request list. */
              else if (ret == 0)
                {
                  ospf_ls_request_delete (onbr, ls_req);
                  ospf_nfsm_check_nbr_loading (onbr);
                  continue;
                }
              /* The new LSA is more recent.  Delete the LSA
                 from the Link state request list. */
              else
                {
                  ospf_ls_request_delete (onbr, ls_req);
                  ospf_nfsm_check_nbr_loading (onbr);
                }
            }
        }

      /* If the new LSA was received from this neighbor,
         examine the next neighbor. */
      if (inbr)
        if (IPV4_ADDR_SAME (&inbr->ident.router_id, &onbr->ident.router_id))
          continue;

#ifdef HAVE_OPAQUE_LSA
      if (IS_OPAQUE_LSA (lsa->data) && !IS_NBR_OPAQUE_CAPABLE (onbr))
        continue;
#endif /* HAVE_OPAQUE_LSA */

      /* Add the new LSA to the Link state retransmission list
         for the adjacency. The LSA will be retransmitted
         at intervals until an acknowledgment is seen from
         the neighbor. */
      if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
        zlog_info (ZG, "LSA[%s]: Added to neighbor[%r]'s retransmit-list",
                   LSA_NAME (lsa), &onbr->ident.router_id);

      ospf_ls_retransmit_add (onbr, lsa);
      retx_flag = 1;
    }

  /* If in the previous step, the LSA was NOT added to any of
     the Link state retransmission lists, there is no need to
     flood the LSA out the interface. */
  if (retx_flag == 0)
    return (inbr && inbr->oi == oi);

  /* if we've received the lsa on this interface we need to perform
     additional checking */
  if (inbr && (inbr->oi == oi))
    {
      /* If the new LSA was received on this interface, and it was
         received from either the Designated Router or the Backup
         Designated Router, chances are that all the neighbors have
         received the LSA already. */
      if (IS_NBR_DECLARED_DR (inbr) || IS_NBR_DECLARED_BACKUP (inbr))
        return 1;

      /* If the new LSA was received on this interface, and the
         interface state is Backup, examine the next interface.  The
         Designated Router will do the flooding on this interface.
         However, if the Designated Router fails the router will
         end up retransmitting the updates. */

      if (oi->state == IFSM_Backup)
        return 1;
    }

  /* The LSA must be flooded out the interface. Send a Link State
     Update packet (including the new LSA as contents) out the
     interface.  The LSA's LS age must be incremented by InfTransDelay
     (which     must be > 0) when it is copied into the outgoing Link
     State Update packet (until the LS age field reaches the maximum
     value of MaxAge). */

  if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
    zlog_info (ZG, "LSA[%s]: Sending update to interface[%s]",
               LSA_NAME (lsa), IF_NAME (oi));

  /*  RFC2328  Section 13.3
      On non-broadcast networks, separate Link State Update
      packets must be sent, as unicasts, to each adjacent neighbor
      (i.e., those in state Exchange or greater).  The destination
      IP addresses for these packets are the neighbors' IP
      addresses.   */
  if (IS_OSPF_NETWORK_NBMA (oi))
    {
      struct ls_node *rn;
      struct ospf_neighbor *nbr;

      for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
        if ((nbr = RN_INFO (rn, RNI_DEFAULT)) != NULL)
          if (nbr->state >= NFSM_Exchange)
	    if (!nbr->is_congested)
              ospf_ls_upd_send_lsa_direct (nbr, lsa);
    }
  else if (!cong_flag) /*Recommendation 3 and 4 of RFC 4222*/
    ospf_ls_upd_send_lsa (oi, lsa);

  return 0;
}

/* All other types are specific to a single area (Area A).
   The eligible interfaces are all those interfaces attaching to the Area A.
   If Area A is the backbone, this includes all the virtual links. */
int
ospf_flood_through_area (struct ospf_area * area,
                         struct ospf_neighbor *inbr, struct ospf_lsa *lsa)
{
  struct ospf_interface *oi;
  struct listnode *node;
  int flag = 0;

  LIST_LOOP (area->iflist, oi, node)
    if (IS_AREA_ID_BACKBONE (area->area_id)
        || oi->type != OSPF_IFTYPE_VIRTUALLINK)
      if (ospf_flood_through_interface (oi, inbr, lsa))
        flag++;

  return flag;
}

/* Don't send AS-external-LSA into stub areas.  Various types of support
   for partial stub areas can be implemented here.
   NSSA's will receive Type-7's that have areas matching the originl LSA. */
int
ospf_flood_through_as (struct ospf *top, struct ospf_neighbor *inbr,
                       struct ospf_lsa *lsa)
{
  struct ospf_area *area;
  struct ospf_interface *oi;
  struct ls_node *rn;
  int flag = 0;
  struct listnode *node;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        if (IS_AREA_DEFAULT (area))
          LIST_LOOP (area->iflist, oi, node)
            if (oi->type !=  OSPF_IFTYPE_VIRTUALLINK)
              if (ospf_flood_through_interface (oi, inbr, lsa))
                flag++;

  return flag;
}

int
ospf_flood_through (struct ospf_neighbor *inbr, struct ospf_lsa *lsa)
{
  switch (LSA_FLOOD_SCOPE (lsa->data->type))
    {
    case LSA_FLOOD_SCOPE_LINK:
      return ospf_flood_through_interface (lsa->lsdb->u.oi, inbr, lsa);
      break;
    case LSA_FLOOD_SCOPE_AREA:
      return ospf_flood_through_area (lsa->lsdb->u.area, inbr, lsa);
      break;
    case LSA_FLOOD_SCOPE_AS:
      return ospf_flood_through_as (lsa->lsdb->u.top, inbr, lsa);
      break;
    default:
      pal_assert (0);
      break;
    }

  return 0;
}


void
ospf_flood_self_lsa_check (struct ospf *top,
                           struct ospf_lsa *current, struct ospf_lsa *new)
{
  struct ospf_master *om = top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  struct ospf_interface *oi;
  struct ospf_ia_lsa_map *oim;
  struct ospf_redist_map *orm;
#ifdef HAVE_OPAQUE_LSA
  struct ospf_opaque_map *oom;
#endif /* HAVE_OPAQUE_LSA */
  struct ls_node *rn;

  /* RFC2328 13.4.  Receiving self-originated LSAs

     It is a common occurrence for a router to receive self-
     originated LSAs via the flooding procedure. A self-originated
     LSA is detected when either 1) the LSA's Advertising Router is
     equal to the router's own Router ID or 2) the LSA is a network-
     LSA and its Link State ID is equal to one of the router's own IP
     interface addresses.  */

  if (IPV4_ADDR_SAME (&new->data->adv_router, &top->router_id))
    SET_FLAG (new->flags, OSPF_LSA_SELF);

  else if (new->data->type == OSPF_NETWORK_LSA)
    for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
      if ((oi = RN_INFO (rn, RNI_DEFAULT)))
        if (oi->type == OSPF_IFTYPE_BROADCAST || oi->type == OSPF_IFTYPE_NBMA)
          if (IPV4_ADDR_SAME (&new->data->id,
                              &oi->ident.address->u.prefix4))
            {
              SET_FLAG (new->flags, OSPF_LSA_SELF);
              new->param = oi;
              ls_unlock_node (rn);
              return;
            }

  if (CHECK_FLAG (new->flags, OSPF_LSA_SELF))
    {
      if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
        zlog_info (ZG, "LSA[%s]: Update param for self-originated LSA",
                   LSA_NAME (new));

      /* Update the param member of the self-originated LSA.  */
      switch (new->data->type)
        {
        case OSPF_NETWORK_LSA:
          for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
            if ((oi = RN_INFO (rn, RNI_DEFAULT)))
              if (oi->type == OSPF_IFTYPE_BROADCAST
                  || oi->type == OSPF_IFTYPE_NBMA)
                if (IPV4_ADDR_SAME (&new->data->id,
                                    &oi->ident.address->u.prefix4))
                  {
                    new->param = oi;
                    ls_unlock_node (rn);
                    break;
                  }
          break;
        case OSPF_SUMMARY_LSA:
        case OSPF_SUMMARY_LSA_ASBR:
          if (current != NULL && (oim = current->param) != NULL)
            {
              if (oim->lsa != NULL)
                {
                  oim->lsa->param = NULL;
                  ospf_lsa_unlock (oim->lsa);
                }
              oim->lsa = ospf_lsa_lock (new);
              new->param = oim;
            }
          break;
        case OSPF_AS_EXTERNAL_LSA:
#ifdef HAVE_NSSA
        case OSPF_AS_NSSA_LSA:
#endif /* HAVE_NSSA */
          if (current != NULL && (orm = current->param) != NULL)
            {
              if (orm->lsa != NULL)
                {
                  orm->lsa->param = NULL;
                  ospf_lsa_unlock (orm->lsa);
                }
              orm->lsa = ospf_lsa_lock (new);
              new->param = orm;
            }
          break;
#ifdef HAVE_OPAQUE_LSA
        case OSPF_LINK_OPAQUE_LSA:
        case OSPF_AREA_OPAQUE_LSA:
        case OSPF_AS_OPAQUE_LSA:
          oom = ospf_opaque_map_lookup (new->lsdb, new->data->id);
          if (oom != NULL)
            {
              if (oom->lsa != NULL)
                {
                  oom->lsa->param = NULL;
                  ospf_lsa_unlock (oom->lsa);
                }
              oom->lsa = ospf_lsa_lock (new);
              new->param = oom;
            }
          break;
#endif /* HAVE_OPAQUE_LSA */
        default:
          if (current != NULL && current->param != NULL)
            {
              new->param = current->param;
              current->param = NULL;
            }
          break;
        }
    }
}

/* We installed a self-originated LSA that we received from a neighbor,
   i.e. it's more recent.  We must see whether we want to originate it.
   If yes, we should use this LSA's sequence number and reoriginate
   a new instance, otherwise we must flush this LSA from the domain. */
void
ospf_flood_self_originated_lsa (struct ospf *top,
                                struct ospf_lsa *current, struct ospf_lsa *new)
{
  struct ospf_master *om = top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
  struct ospf_interface *oi;
#ifdef HAVE_OPAQUE_LSA
  struct ospf_opaque_map *oom;
#endif /* HAVE_OPAQUE_LSA */

#ifdef HAVE_RESTART
  if (CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
    return;
#endif /* HAVE_RESTART */

  if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
    zlog_info (ZG, "LSA[%s]: Process self-originated LSA", LSA_NAME (new));

  switch (new->data->type)
    {
    case OSPF_NETWORK_LSA:
      /* If we received self originated network LSA with adv router_id
         different from our, we must flush this LSA from area. */
      if (!IPV4_ADDR_SAME (&new->data->adv_router, &top->router_id))
        ospf_lsdb_lsa_discard (new->lsdb, new, 1, 0, 0);

      /* If the interface is no more a broadcast type or we are no more
         the DR, we flush the LSA otherwise -- create the new instance and
         schedule flooding. */
      else
        {
          oi = (struct ospf_interface *)new->param;
          if (oi == NULL
              || oi->area != new->lsdb->u.area
              || (oi->type != OSPF_IFTYPE_BROADCAST
                  && oi->type != OSPF_IFTYPE_NBMA)
              || !IPV4_ADDR_SAME (&oi->ident.address->u.prefix4,
                                  &oi->ident.d_router))
            ospf_lsdb_lsa_discard (new->lsdb, new, 1, 0, 0);
          else
            LSA_REFRESH_TIMER_ADD (top, OSPF_NETWORK_LSA,
                                   new, new->lsdb->u.area);
        }
      break;
#ifdef HAVE_OPAQUE_LSA
    case OSPF_LINK_OPAQUE_LSA:
    case OSPF_AREA_OPAQUE_LSA:
    case OSPF_AS_OPAQUE_LSA:
      if (current == NULL || (oom = new->param) == NULL)
        ospf_lsdb_lsa_discard (new->lsdb, new, 1, 0, 0);
      else
        {
          SET_FLAG (oom->flags, OSPF_OPAQUE_MAP_UPDATED);
          ospf_opaque_lsa_timer_add (new->lsdb);
        }
      break;
#endif /* HAVE_OPAQUE_LSA */
    default:
      if (current == NULL || new->param == NULL)
        ospf_lsdb_lsa_discard (new->lsdb, new, 1, 0, 0);
      else
        LSA_REFRESH_TIMER_ADD (top, new->data->type, new, new->param);
      break;
    }
}

/* Do the LSA acking specified in table 19, Section 13.5, row 2
   This get called from ospf_flood_out_interface. Declared inline
   for speed. */
static void
ospf_flood_delayed_lsa_ack (struct ospf_neighbor *inbr, struct ospf_lsa *lsa)
{
  struct ospf_interface *oi = inbr->oi;

  /* LSA is more recent than database copy, but was not
     flooded back out receiving interface.  Delayed
     acknowledgment sent. If interface is in Backup state
     delayed acknowledgment sent only if advertisement
     received from Designated Router, otherwise do nothing See
     RFC 2328 Section 13.5 */

  /* Whether LSA is more recent or not, and whether this is in
     response to the LSA being sent out recieving interface has been
     worked out previously */

  /* Deal with router as BDR */
  if (oi->state == IFSM_Backup && !IS_NBR_DECLARED_DR (inbr))
    return;

  /* Schedule a delayed LSA Ack to be sent */
  LS_ACK_ADD (oi, lsa);
}

/* OSPF LSA flooding -- RFC2328 Section 13.(5). */
int
ospf_flood (struct ospf *top, struct ospf_neighbor *nbr,
            struct ospf_lsa *current, struct ospf_lsa *new)
{
  struct ospf_master *om = top->om;
  struct pal_timeval now;
  int lsa_ack_flag = 1;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];
#ifdef HAVE_VRF_OSPF
  u_int32_t tag ;
  struct ospf *top1;
  struct listnode *nn;
#endif /* HAVE_VRF_OSPF */

  if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
    zlog_info (ZG, "LSA[%s]: flood started", LSA_NAME (new));

  /* Get current time. */
  pal_time_tzcurrent (&now, NULL);

  /* If there is already a database copy, and if the
     database copy was received via flooding and installed less
     than MinLSArrival seconds ago, discard the new LSA
     (without acknowledging it). */
  /* The MinLSArrival check is meant only for LSAs received during
     flooding,and should not be performed on those LSAs that the router
     itself originates --- RFC 2328 Appendix G.1.*/

  if (current != NULL && (!IS_LSA_SELF(current))
      && (TV_CMP (TV_SUB (now, current->tv_update),
                 INT2TV (OSPF_MIN_LS_ARRIVAL)) < 0))
    {
      if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
        zlog_info (ZG, "LSA[%s]: LSA is received recently", LSA_NAME (new));

      return -1;
    }
#ifdef HAVE_VRF_OSPF
  /* If VRF is enabled and DN-bit is set in the recieved LSA
     then discard the LSA */
  if (CHECK_FLAG (new->data->options, DN_OPTIONS_MASK)
      && CHECK_FLAG (top->ov->flags, OSPF_VRF_ENABLED))
    {
     if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
       zlog_info (ZG, "LSA[%s]: DN-bit set LSA looping back", LSA_NAME (new));

     return -1;
    }

  /*if the route-tag of the received LSA matches with the route-tag assigned
    then discard the LSA  */
   LIST_LOOP(om->ospf, top1, nn)
   {
     if(CHECK_FLAG (top1->ov->flags, OSPF_VRF_ENABLED))
       {
         if (top1->redist[APN_ROUTE_BGP].tag)
           {
              tag = OSPF_PNT_UINT32_GET (new->data,
                              OSPF_EXTERNAL_LSA_TAG_OFFSET);
	      if (tag && (tag == top1->redist[APN_ROUTE_BGP].tag))
	        {
        	  if (IS_DEBUG_OSPF (lsa, LSA_FLOODING))
	            zlog_info (ZG, "LSA[%s]: Route-Tag Match LSA looping back", LSA_NAME (new));

        	  return -1;
        	}
       	   }
       }
   }

#endif /* HAVE_VRF_OSPF */

#ifdef HAVE_RESTART
  if (current != NULL && !ospf_lsa_different (current, new))
    SET_FLAG (new->flags, OSPF_LSA_REFRESHED);
#endif /* HAVE_RESTART */

  /* Check flags. */
  SET_FLAG (new->flags, OSPF_LSA_RECEIVED);
  ospf_flood_self_lsa_check (top, current, new);

#ifdef HAVE_OSPF_DB_OVERFLOW
  if (current == NULL
      && new->data->type == OSPF_AS_EXTERNAL_LSA
      && new->data->id.s_addr != 0)
    {
      if (ospf_lsdb_count_external_non_default (top) >= top->ext_lsdb_limit)
        {
          ospf_db_overflow_enter (top);
          if (!IS_LSA_MAXAGE (new) && !IS_LSA_SELF (new))
            return -1;
        }
    }
#endif /* HAVE_OSPF_DB_OVERFLOW */

  /* Flood the new LSA out some subset of the router's interfaces.
     In some cases (e.g., the state of the receiving interface is
     DR and the LSA was received from a router other than the
     Backup DR) the LSA will be flooded back out the receiving
     interface. */
  if (!IS_LSA_SELF (new)
      || !IS_LSA_MAXAGE (new)
      || current == NULL)
    lsa_ack_flag = ospf_flood_through (nbr, new);

  /* Remove the current database copy from all neighbors' Link state
     retransmission lists.  Only AS_EXTERNAL does not have area ID.
     All other (even NSSA's) do have area ID.  */
  if (current)
    {
      ospf_ls_retransmit_delete_nbr_by_scope (current, PAL_TRUE);
      ospf_lsa_lock (current);
    }

  /* Install the new LSA in the link state database
     (replacing the current database copy).  This may cause the
     routing table calculation to be scheduled.  In addition,
     timestamp the new LSA with the current time.  The flooding
     procedure cannot overwrite the newly installed LSA until
     MinLSArrival seconds have elapsed. */
  new = ospf_lsa_install (top, new);
  if (new != NULL)
    {
      /* Acknowledge the receipt of the LSA by sending a Link State
         Acknowledgment packet back out the receiving interface. */
      if (lsa_ack_flag)
        ospf_flood_delayed_lsa_ack (nbr, new);

      /* If this new LSA indicates that it was originated by the
         receiving router itself, the router must take special action,
         either updating the LSA or in some cases flushing it from
         the routing domain. */
      if (IS_LSA_SELF (new))
        ospf_flood_self_originated_lsa (top, current, new);
      else
        /* Update statistics value for OSPF-MIB. */
        top->rx_lsa_count++;
    }

  if (current)
    ospf_lsa_unlock (current);

  return 0;
}

void
ospf_flood_lsa_flush (struct ospf *top, struct ospf_lsa *lsa)
{	
  if (CHECK_FLAG (lsa->flags, OSPF_LSA_DISCARD))
    return;

  if (IS_LSA_SELF (lsa))
    {
      lsa->data->ls_age = pal_hton16 (OSPF_LSA_MAXAGE);
      lsa->lsdb->max_lsa_age = OSPF_LSA_MAXAGE - OSPF_LSA_MAXAGE_INTERVAL_DEFAULT;
      pal_time_tzcurrent (&lsa->lsdb->tv_maxage, NULL);
      pal_time_tzcurrent (&lsa->tv_update, NULL);
      UNSET_FLAG (lsa->flags, OSPF_LSA_REFRESHED);
      OSPF_TIMER_OFF (lsa->lsdb->t_lsa_walker);
      OSPF_TIMER_ON (lsa->lsdb->t_lsa_walker, ospf_lsa_maxage_walker,
                     lsa->lsdb, OSPF_LSA_MAXAGE_INTERVAL_DEFAULT);
    }

#ifdef HAVE_RESTART
  if (!CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
#endif /* HAVE_RESTART */
    if (!CHECK_FLAG (lsa->flags, OSPF_LSA_FLUSHED))
      {
        SET_FLAG (lsa->flags, OSPF_LSA_FLUSHED);
        ospf_flood_through (NULL, lsa);
      }

#ifdef HAVE_SNMP
  /* Originate SNMP trap.  */
  ospfOriginateLsa (top, lsa, OSPFMAXAGELSA);
#endif /* HAVE_SNMP */
}
