/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_RESTART

#include "log.h"
#include "thread.h"
#include "nsm_client.h"
#include "checksum.h"

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
#include "ospfd/ospf_nfsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_ase.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_debug.h"
#include "ospfd/ospf_restart.h"


/* Prototypes. */
int ospf_opaque_lsa_timer (struct thread *);
int ospf_router_lsa_link_vec_set (struct ospf_area *);
int ospf_restart_check_intermediate (struct ospf *);

/* Supprot of Graceful OSPF Restart - RFC3632. */
void
ospf_grace_lsa_originate (struct ospf_interface *oi)
{
  struct ospf_master *om = oi->top->om;
  struct stream *s;
  struct pal_in4_addr id;

  s = stream_new (OSPF_GRACE_LSA_TLV_DATA_SIZE_MAX);

  OSPF_TLV_INT32_PUT (s, OSPF_GRACE_LSA_GRACE_PERIOD,
                      OSPF_GRACE_LSA_GRACE_PERIOD_LEN, om->grace_period);
  OSPF_TLV_CHAR_PUT (s, OSPF_GRACE_LSA_RESTART_REASON,
                     OSPF_GRACE_LSA_RESTART_REASON_LEN, om->restart_reason);
  if (oi->type == OSPF_IFTYPE_BROADCAST
      || oi->type == OSPF_IFTYPE_NBMA
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    OSPF_TLV_IN_ADDR_PUT (s, OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS,
                          OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS_LEN,
                          &oi->ident.address->u.prefix4);

  id.s_addr = MAKE_OPAQUE_ID (3, 0);

  ospf_opaque_lsa_redistribute_link (oi, id, STREAM_DATA (s),
                                     stream_get_endp (s));
  OSPF_TIMER_OFF (oi->lsdb->t_opaque_lsa);
  OSPF_EVENT_EXECUTE (ospf_opaque_lsa_timer, oi->lsdb, 0);

  stream_free (s);
}

void
ospf_grace_lsa_flood_force (struct ospf_interface *oi)
{
  struct pal_in4_addr id;
  struct ospf_lsa *lsa;
  struct stream *s;

  s = stream_new (OSPF_GRACE_LSA_TLV_DATA_SIZE_MAX);

  OSPF_TLV_INT32_PUT (s, OSPF_GRACE_LSA_GRACE_PERIOD,
                      OSPF_GRACE_LSA_GRACE_PERIOD_LEN,
                      og->nc->service.grace_period);

  OSPF_TLV_CHAR_PUT (s, OSPF_GRACE_LSA_RESTART_REASON,
                     OSPF_GRACE_LSA_RESTART_REASON_LEN,
                     OSPF_RESTART_REASON_UNKNOWN);
  if (oi->type == OSPF_IFTYPE_BROADCAST
      || oi->type == OSPF_IFTYPE_NBMA
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    OSPF_TLV_IN_ADDR_PUT (s, OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS,
                          OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS_LEN,
                          &oi->ident.address->u.prefix4);

  id.s_addr = MAKE_OPAQUE_ID (3, 0);

  ospf_opaque_lsa_redistribute_link (oi, id, STREAM_DATA (s),
                                     stream_get_endp (s));
  OSPF_TIMER_OFF (oi->lsdb->t_opaque_lsa);
  OSPF_EVENT_EXECUTE (ospf_opaque_lsa_timer, oi->lsdb, 0);

  stream_free (s);

  /* Force to flood LSA. */
  lsa = ospf_lsdb_lookup_by_id (oi->lsdb, OSPF_LINK_OPAQUE_LSA,
                                id, oi->top->router_id);
  if (lsa != NULL)
    ospf_ls_upd_send_lsa_force (oi, lsa, &IPv4AllSPFRouters);
}

struct ospf_interface *
ospf_area_if_lookup_by_addr (struct ospf_area *area, struct pal_in4_addr addr)
{
  struct ospf_interface *oi;
  struct listnode *node;

  for (node = LISTHEAD (area->iflist); node; NEXTNODE (node))
    {
      oi = GETDATA (node);
      if (!oi)
        continue;

      if (IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, &addr))
        return oi;
    }
  return NULL;
}

int
ospf_area_hello_recv_count (struct ospf_area *area)
{
  struct ospf_interface *oi;
  struct listnode *node;
  int count = 0;

  for (node = LISTHEAD (area->iflist); node; NEXTNODE (node))
    {
      oi = GETDATA (node);
      if (!oi)
        continue;

      count += oi->hello_in;
    }
  return count;
}

struct ospf_neighbor *
ospf_nbr_lookup_from_area (struct ospf_area *area,
                           struct pal_in4_addr router_id,
                           struct pal_in4_addr addr)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct listnode *node;

  for (node = LISTHEAD (area->iflist); node; NEXTNODE (node))
    {
      oi = GETDATA (node);
      if (!oi)
        continue;

      if (IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, &addr))
        {
          nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, &router_id);
          if (nbr != NULL)
            return nbr;
        }
    }
  return NULL;
}

int
ospf_restart_router_lsa_check_self (struct ospf_interface *oi,
                                    struct pal_in4_addr addr)
{
  struct ospf_area *area = oi->area;
  struct ospf_lsa *lsa;
  struct ospf_link_desc link;

  lsa = ospf_lsdb_lookup_by_id (area->lsdb, OSPF_ROUTER_LSA,
                                oi->top->router_id, oi->top->router_id);
  if (lsa != NULL)
    {
      u_char *p = (u_char *)lsa->data + 24;
      u_char *lim = (u_char *)lsa->data + pal_ntoh16 (lsa->data->length);

      while (p + OSPF_LINK_DESC_SIZE <= lim)
        {
          OSPF_LINK_DESC_GET (p, link);

          p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

          switch (link.type)
            {
            case LSA_LINK_TYPE_TRANSIT:
              /* We previously described this interface as transit. */
              if (IPV4_ADDR_SAME (link.id, &oi->ident.d_router))
                return 0;
              break;
            case LSA_LINK_TYPE_STUB:
            case LSA_LINK_TYPE_POINTOPOINT:
            case LSA_LINK_TYPE_VIRTUALLINK:
            default:
              break;
            }
        }
    }

  /* If self-originated pre-restart router-LSA is not received, it is OK. */
  return 1;
}

int
ospf_restart_router_lsa_check_nbr (struct ospf *top, struct ospf_lsa *lsa,
                                   struct ospf_interface *oi,
                                   struct ospf_neighbor *nbr)
{
  struct ospf_link_desc link;
  u_char *p = (u_char *)lsa->data + 24;
  u_char *lim = (u_char *)lsa->data + pal_ntoh16 (lsa->data->length);

  while (p + OSPF_LINK_DESC_SIZE <= lim)
    {
      OSPF_LINK_DESC_GET (p, link);

      p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

      switch (link.type)
        {
        case LSA_LINK_TYPE_POINTOPOINT:
        case LSA_LINK_TYPE_VIRTUALLINK:
          if (IPV4_ADDR_SAME (link.id, &top->router_id))
            return 1;
          break;
        case LSA_LINK_TYPE_TRANSIT:
          if (IPV4_ADDR_SAME (link.id, &oi->ident.d_router))
            return 1;
          break;
        case LSA_LINK_TYPE_STUB:
          /* Neighbor describe stub link with this interface's subnet. */
          if (oi->type == OSPF_IFTYPE_BROADCAST
              || oi->type == OSPF_IFTYPE_NBMA)
            if (IPV4_ADDR_SAME (link.id, &oi->ident.address->u.prefix4))
              if (!ospf_restart_router_lsa_check_self (oi, *link.id))
                return 0;
          break;
        default:
          break;
        }
    }

  return 0;
}

int
ospf_restart_router_lsa_check (struct ospf *top, struct ospf_lsa *lsa)
{
  struct ospf_area *area = lsa->lsdb->u.area;
  struct ospf_neighbor *nbr;
  struct ospf_interface *oi;
  struct listnode *node;

  LIST_LOOP (area->iflist, oi, node)
    {
      nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, &lsa->data->adv_router);
      if (nbr != NULL)
        if (!ospf_restart_router_lsa_check_nbr (top, lsa, oi, nbr))
          return 0;
    }

  return 1;
}

int
ospf_restart_state_check_transit (struct ospf_interface *oi,
                                  struct pal_in4_addr addr)
{
  struct ospf_neighbor *nbr;
  struct ospf_lsa *lsa;
  struct pal_in4_addr *nbr_id;
  u_char *lim, *p;

  lsa = ospf_network_lsa_lookup_by_addr (oi->area, addr);
  if (lsa == NULL)
    return 0;

  p = (u_char *)lsa->data + OSPF_NETWORK_LSA_ROUTERS_OFFSET;
  lim = (u_char *)lsa->data + pal_ntoh16 (lsa->data->length);

  while (p < lim)
    {
      nbr_id = (struct pal_in4_addr *)p;

      if (!IPV4_ADDR_SAME (nbr_id, &oi->ident.router_id))
        {
          nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, nbr_id);
          if (nbr == NULL)
            return 0;

          /* Any case, NFSM less thant 2-way is not accepted. */
          if (nbr->state < NFSM_TwoWay)
            return 0;

          /* If we are DR or Backup, or Neighbor is DR or Backup,
             we must be full adjacent to the neighbor. */
          if (nbr->state < NFSM_Full)
            if (IS_DECLARED_DR (&oi->ident)
                || IS_DECLARED_BACKUP (&oi->ident)
                || (IPV4_ADDR_SAME (&oi->ident.d_router,
                                    &nbr->ident.address->u.prefix4)
                    || IPV4_ADDR_SAME (&oi->ident.bd_router,
                                       &nbr->ident.address->u.prefix4)))
              return 0;

        }

      p += sizeof (struct pal_in4_addr);
    }

  return 1;
}

/* Return 0 if not ready, otherwise return 1 if ready to exit Restart. */
int
ospf_restart_state_check_graceful (struct ospf *top)
{
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;
  struct ospf_area *area;
  struct ospf_lsa *lsa;
  struct ospf_link_desc link;
  struct ls_node *rn;
  int count = 0;
  u_char *p;
  u_char *lim;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (area->active_if_count)
        if (ospf_area_hello_recv_count (area))
          {
            count++;
            lsa = ospf_lsdb_lookup_by_id (area->lsdb, OSPF_ROUTER_LSA,
                                          top->router_id, top->router_id);
            if (lsa == NULL)
              {
                ls_unlock_node (rn);
                return 0;
              }

            p = (u_char *)lsa->data;
            lim = p + pal_ntoh16 (lsa->data->length);

            p += 24;

            while (p + OSPF_LINK_DESC_SIZE <= lim)
              {
                OSPF_LINK_DESC_GET (p, link);

                p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;

                switch (link.type)
                  {
                  case LSA_LINK_TYPE_POINTOPOINT:
                  case LSA_LINK_TYPE_VIRTUALLINK:
                    nbr = ospf_nbr_lookup_from_area (area, *link.id,
                                                     *link.data);
                    if (nbr == NULL
                        || nbr->state != NFSM_Full)
                      {
                        ls_unlock_node (rn);
                        return 0;
                      }
                    break;
                  case LSA_LINK_TYPE_TRANSIT:
                    oi = ospf_area_if_lookup_by_addr (area, *link.data);
                    if (oi == NULL
                        || !ospf_restart_state_check_transit (oi, *link.id))
                      {
                        ls_unlock_node (rn);
                        return 0;
                      }
                    break;
                  case LSA_LINK_TYPE_STUB:
                  default:
                    break;
                  }
              }
          }

  if (count == 0)
    return 0;

  return 1;
}

int
ospf_restart_state_check_signaling (struct ospf *top)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  int ret = 0;

  for (rn = ls_table_top (top->nbr_table); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      {
        ret = 1;
        /* Condition to Check the DROther and its state */
        if ((nbr->state != NFSM_Full) &&
            !((nbr->oi->state == IFSM_DROther) &&
              (nbr->state == NFSM_TwoWay)))
          {
            ls_unlock_node (rn);
            return 0;
          }
      }
  return ret;
}

void
ospf_restart_state_unset (struct ospf *top)
{
  struct ospf_master *om = top->om;
  struct ospf *other;
  struct listnode *node;
  int count = 0;

  UNSET_FLAG (top->flags, OSPF_ROUTER_RESTART);

  /* Cancel the restart timer.  */
  OSPF_TIMER_OFF (top->t_restart_state);

  /* Check other instance is still restart state. */
  LIST_LOOP (om->ospf, other, node)
    if (CHECK_FLAG (other->flags, OSPF_ROUTER_RESTART))
      count++;

  /* Now all instance completely exit from Graceful Restart. */
  if (count == 0)
    {
      UNSET_FLAG (om->flags, OSPF_GLOBAL_FLAG_RESTART);

      /* Cancel the restart timer.  */
      OSPF_TIMER_OFF (om->t_restart);
    }
}

void
ospf_restart_exit (struct ospf *top)
{
  struct ospf_area *area;
  struct ospf_interface *oi;
  struct ospf_lsa *lsa;
  struct ls_node *r1, *r2, *r3;
#ifdef HAVE_NSSA
  struct ls_node *r4, *r5;
#endif /*HAVE_NSSA*/
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */


  ospf_restart_state_unset (top);

  if (!CHECK_FLAG (top->flags, OSPF_PROC_UP))
    return;

  if (top->restart_method == OSPF_RESTART_METHOD_NEVER)
    return;

  /* Re-originatge router-LSA for all Areas. */
  ospf_router_lsa_refresh_all (top);

  for (r1 = ls_table_top (top->if_table); r1; r1 = ls_route_next (r1))
    if ((oi = RN_INFO (r1, RNI_DEFAULT)))
      {
        SET_FLAG (oi->flags, OSPF_IF_DR_ELECTION_READY);

        /* If DR, Re-originate network-LSA. */
        if (oi->state == IFSM_DR
            && oi->full_nbr_count > 0)
          LSA_REFRESH_TIMER_ADD (oi->top, OSPF_NETWORK_LSA,
                                 ospf_network_lsa_self (oi), oi);

        if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
          LSDB_LOOP (LINK_OPAQUE_LSDB (oi), r2, lsa)
            if (IS_LSA_SELF (lsa))
              if (OPAQUE_TYPE (lsa->data->id) == OSPF_LSA_OPAQUE_TYPE_GRACE)
                ospf_lsdb_lsa_discard (oi->lsdb, lsa, 1, 1, 1);

        /* Reset the RS-bit,when router reaches FULL in this interface */
        if (oi->full_nbr_count > 0)
          {
            if (CHECK_FLAG (oi->flags, OSPF_IF_RS_BIT_SET))
              UNSET_FLAG (oi->flags, OSPF_IF_RS_BIT_SET);

            OSPF_TIMER_OFF (oi->t_restart_bit_check);
          }
      }
#ifdef HAVE_OSPF_MULTI_AREA
  for (r1 = ls_table_top (top->multi_area_link_table); r1;
       r1 = ls_route_next (r1))
    if ((mlink = RN_INFO (r1, RNI_DEFAULT)))
      if (mlink->oi)
        {
          SET_FLAG (mlink->oi->flags, OSPF_IF_DR_ELECTION_READY);

          if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
            LSDB_LOOP (LINK_OPAQUE_LSDB (mlink->oi), r2, lsa)
              if (IS_LSA_SELF (lsa))
                if (OPAQUE_TYPE (lsa->data->id) == OSPF_LSA_OPAQUE_TYPE_GRACE)
                  ospf_lsdb_lsa_discard (mlink->oi->lsdb, lsa, 1, 1, 1);

        /* Reset the RS-bit,when router reaches FULL in this interface */
        if (mlink->oi->full_nbr_count > 0)
          {
            if (CHECK_FLAG (mlink->oi->flags, OSPF_IF_RS_BIT_SET))
              UNSET_FLAG (mlink->oi->flags, OSPF_IF_RS_BIT_SET);

            OSPF_TIMER_OFF (mlink->oi->t_restart_bit_check);
          }
      }
#endif /* HAVE_OSPF_MULTI_AREA */

  ospf_route_table_finish (top->rt_network);
  ospf_route_table_finish (top->rt_asbr);

  /* Flush the invalid self originated Type-5 and Type-7 LSAs */
  LSDB_LOOP (EXTERNAL_LSDB (top), r3, lsa)
    if (IS_LSA_SELF (lsa))
      ospf_redist_map_lsa_delete (top, lsa, top->lsdb);

#ifdef HAVE_NSSA
  for (r4 = ls_table_top (top->area_table); r4; r4 = ls_route_next (r4))
    if ((area = RN_INFO (r4, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        if (IS_AREA_NSSA (area))
          {
            LSDB_LOOP (NSSA_LSDB (area), r5, lsa)
              if (IS_LSA_SELF (lsa))
                ospf_redist_map_lsa_delete (area->top, lsa, area->lsdb);
          }
#endif
  /* Update the self originated Type-5 and Type-7 LSAs */
  ospf_redist_map_lsa_update_all (top);

  /* Re-run SPF calculation. */
  for (r1 = ls_table_top (top->area_table); r1; r1 = ls_route_next (r1))
    if ((area = RN_INFO (r1, RNI_DEFAULT)))
      {
        ospf_router_lsa_link_vec_set (area);
        OSPF_EVENT_EXECUTE (ospf_spf_calculate_timer, area, 0);
      }
}

int
ospf_restart_timer (struct thread *thread)
{
  struct ospf_master *om = THREAD_ARG (thread);
  struct listnode *node;
  struct ospf *top;

  om->t_restart = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER[All]: Exit Restarting by expiring grace period");

  LIST_LOOP (om->ospf, top, node)
    if (CHECK_FLAG (top->flags, OSPF_ROUTER_RESTART))
      ospf_restart_exit (top);

  /* Cleanup the stale routes.  */
  OSPF_EVENT_EXECUTE (ospf_restart_stale_timer, om, 0);

  if (om->restart_option)
    {
      stream_free (om->restart_option);
      om->restart_option = NULL;
      vector_free (om->restart_tlvs);
      om->restart_tlvs = NULL;
    }

  return 0;
}

int
ospf_restart_stale_timer (struct thread *thread)
{
  struct ospf_master *om = THREAD_ARG (thread);

  om->t_restart = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER[All]: Cleanup the stale routes");

  /* Flush all the pending NSM messages.  */
  ospf_nsm_flush (om);

  /* Send the stale remove message to NSM.  */
  ospf_nsm_stale_remove (om);

  return 0;
}

int
ospf_restart_signaling_state_check_timer (struct thread *thread)
{
  struct ospf_interface *oi = THREAD_ARG (thread);
  struct ospf_master *om = oi->top->om;
  char if_str[OSPF_IF_STRING_MAXLEN];

  oi->t_restart_bit_check = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (ifsm, IFSM_TIMERS))
    zlog_info (ZG, "IFSM[%s]: Restart Signalling bit clear timer expire",
               IF_NAME (oi));

  if (CHECK_FLAG (oi->flags, OSPF_IF_RS_BIT_SET))
    UNSET_FLAG (oi->flags, OSPF_IF_RS_BIT_SET);

  return 0;
}

int
ospf_restart_state_timer (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  return ospf_restart_check_intermediate (top);
}

int
ospf_restart_check_intermediate (struct ospf *top)
{
  struct ospf_master *om = top->om;
  int ret = 0;

  top->t_restart_state = NULL;

  if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
    ret = ospf_restart_state_check_graceful (top);
  else if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    ret = ospf_restart_state_check_signaling (top);

  if (ret)
    {
      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "ROUTER[Process:%d]: Exit Restarting normally",
                   top->ospf_id);

      ospf_restart_exit (top);

      /* Start the stale remove timer if all the instances
         exit successfully.  */
      if (!CHECK_FLAG (om->flags, OSPF_GLOBAL_FLAG_RESTART))
        OSPF_TIMER_ON (om->t_restart, ospf_restart_stale_timer,
                       om, OSPF_RESTART_STALE_TIMER_DELAY);
    }
  else
    OSPF_TOP_TIMER_ON (top->t_restart_state, ospf_restart_state_timer,
                       OSPF_RESTART_STATE_TIMER_INTERVAL);

  return 0;
}

void
ospf_grace_lsa_recv_check (struct ospf *top)
{
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
  int oi_ack_count = 0;
  int oi_counter = 0;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  /* House keeping for grace-LSA ack via virtual-link */
  for (rn = ls_table_top (top->vlink_table);
       rn; rn = ls_route_next (rn))
    if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
      {
        oi_counter++;
        if (CHECK_FLAG (vlink->oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD))
          oi_ack_count++;
      }

  /* House keeping for grace-LSA ack */
  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      {
        oi_counter++;
        if (CHECK_FLAG (oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD))
          oi_ack_count++;
      }
#ifdef HAVE_OSPF_MULTI_AREA
  /* House keeping for grace-LSA ack via multi-area-if-link */
  for (rn = ls_table_top (top->multi_area_link_table); rn;
                                      rn = ls_route_next (rn))
    if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
      if (mlink->oi)
        {
          oi_counter++;
          if (CHECK_FLAG (mlink->oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD))
            oi_ack_count++;
        }

#endif /* HAVE_OSPF_MULTI_AREA */

  if (oi_ack_count == oi_counter)
    SET_FLAG (top->flags, OSPF_GRACE_LSA_ACK_RECD);
}

int
ospf_grace_ack_wait_timer (struct thread *thread)
{
  struct ospf *top = THREAD_ARG (thread);
  struct ospf_master *om = top->om;
  struct ospf_interface *oi;
  struct ospf_vlink *vlink;
  struct ls_node *rn;
#ifdef HAVE_OSPF_MULTI_AREA
  struct ospf_multi_area_link *mlink = NULL;
#endif /* HAVE_OSPF_MULTI_AREA */

  top->t_grace_ack_event = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER: Grace ACKs wait timer expire");

  ospf_grace_lsa_recv_check (top);

  if (CHECK_FLAG (top->flags, OSPF_GRACE_LSA_ACK_RECD))
    {
      ospf_proc_destroy (top);
    }
  else
    {
      /* Grace-LSA is not received, then flood the LSA again. */
      for (rn = ls_table_top (top->vlink_table);
           rn; rn = ls_route_next (rn))
        if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
          if (!CHECK_FLAG (vlink->oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD))
            ospf_grace_lsa_originate (vlink->oi);

      for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
        if ((oi = RN_INFO (rn, RNI_DEFAULT)))
          if (!CHECK_FLAG (oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD))
            ospf_grace_lsa_originate (oi);
#ifdef HAVE_OSPF_MULTI_AREA
      for (rn = ls_table_top (top->multi_area_link_table); rn;
           rn = ls_route_next (rn))
        if ((mlink = RN_INFO (rn, RNI_DEFAULT)))
          if (mlink->oi)
            if (!CHECK_FLAG (mlink->oi->flags, OSPF_IF_GRACE_LSA_ACK_RECD))
              ospf_grace_lsa_originate (mlink->oi);
#endif /* HAVE_OSPF_MULTI_AREA */

      ospf_proc_destroy (top);
   }
  mod_stop(APN_PROTO_OSPF, MOD_STOP_CAUSE_GRACE_RST);
  pal_exit (0);
}


/* Helper mode. */
void
ospf_restart_helper_exit (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  struct ospf_area *area;

  OSPF_NFSM_TIMER_OFF (nbr->t_helper);

  if (nbr->t_inactivity == NULL)
    OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_InactivityTimer);

  UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART);

  /* Re-calculate DR. */
  if (oi->type == OSPF_IFTYPE_BROADCAST
      || oi->type == OSPF_IFTYPE_NBMA)
    ospf_dr_election (oi);

  /* Re-originate router-LSA. */
  LSA_REFRESH_TIMER_ADD (oi->top, OSPF_ROUTER_LSA,
                         ospf_router_lsa_self (oi->area), oi->area);

  /* If DR, Re-originate network-LSA. */
  if (oi->state == IFSM_DR
      && oi->full_nbr_count > 0)
    LSA_REFRESH_TIMER_ADD (oi->top, OSPF_NETWORK_LSA,
                           ospf_network_lsa_self (oi), oi);

  /* If VLINK, Re-originate rotuer-LSA on transit-area. */
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    {
      area = ospf_area_entry_lookup (oi->top, oi->u.vlink->area_id);
      if (area != NULL)
        LSA_REFRESH_TIMER_ADD (oi->top, OSPF_ROUTER_LSA,
                               ospf_router_lsa_self (area), area);
    }
}

int
ospf_restart_helper_exit_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  nbr->t_helper = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: Helper timer expire", NBR_NAME (nbr));

  if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "ROUTER: Exit Restart Helper mode for neighbor(%s) "
               "by expiring grace period", NBR_NAME (nbr));

  ospf_restart_helper_exit (nbr);

  return 0;
}

void
ospf_restart_helper_graceful_exit_all (struct ospf *top)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  for (rn = ls_table_top (top->nbr_table); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
        ospf_restart_helper_exit (nbr);
}

void
ospf_restart_helper_signaling_exit_all (struct ospf *top)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;

  for (rn = ls_table_top (top->nbr_table); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
        {
          UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART);
          UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS);
          OSPF_TIMER_OFF (nbr->t_resync);
        }
}

void
ospf_restart_helper_exit_all (struct ospf *top)
{
  if (top->restart_method == OSPF_RESTART_METHOD_GRACEFUL)
    ospf_restart_helper_graceful_exit_all (top);
  else if (top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    ospf_restart_helper_signaling_exit_all (top);
}


void
ospf_restart_option_tlv_parse (struct ospf_master *om, struct stream *s)
{
  u_int16_t type;
  u_int16_t length;

  om->restart_tlvs = vector_init (OSPF_VECTOR_SIZE_MIN);

  while (stream_get_getp (s) < STREAM_SIZE (s))
    {
      vector_set (om->restart_tlvs, STREAM_PNT (s));
      type = stream_getw (s);
      length = stream_getw (s);
      stream_forward (s, OSPF_TLV_SPACE (length) - 4);
    }
}

struct ospf_restart_tlv *
ospf_restart_tlv_lookup_by_type (struct ospf *top, u_int16_t type)
{
  struct ospf_master *om = top->om;
  struct ospf_restart_tlv *tlv;
  int i;

  if (om->restart_tlvs == NULL)
    return NULL;

  for (i = 0; i < vector_max (om->restart_tlvs); i++)
    if ((tlv = vector_slot (om->restart_tlvs, i)))
      if ((pal_ntoh16 (tlv->type)) == type)
        return tlv;

  return NULL;
}

u_char
ospf_restart_reason_lookup (struct ospf *top)
{
  struct ospf_restart_tlv *tlv;

  tlv = ospf_restart_tlv_lookup_by_type (top, OSPF_NSM_RESTART_REASON_TLV);
  if (tlv == NULL)
    return OSPF_RESTART_REASON_UNKNOWN;

  return tlv->data[0];
}

u_int32_t
ospf_restart_crypt_seqnum_lookup (struct ospf *top,
                                  struct pal_in4_addr *ifaddr,
                                  u_int32_t ifindex)
{
  struct ospf_restart_tlv *tlv;
  struct pal_in4_addr addr;
  u_int32_t index;
  u_int32_t crypt_seqnum;
  u_char *p;
  u_char *lim;

  tlv = ospf_restart_tlv_lookup_by_type (top,
                                         OSPF_NSM_RESTART_CRYPT_SEQNUM_TLV);
  if (tlv == NULL)
    return 0;

  p = tlv->data;
  lim = p + pal_ntoh16 (tlv->length);

  while (p < lim)
    {
      pal_mem_cpy (&addr, ifaddr, sizeof (struct pal_in4_addr));
      index = OSPF_PNT_UINT32_GET (p, 4);
      crypt_seqnum = OSPF_PNT_UINT32_GET (p, 8);
      if (IPV4_ADDR_SAME (&addr, ifaddr)
          && index == ifindex)
        return crypt_seqnum;

      p += 12;
    }
  return 0;
}

u_int32_t
ospf_restart_vlink_crypt_seqnum_lookup (struct ospf *top, u_int16_t proc_id,
                                        struct pal_in4_addr *area_id,
                                        struct pal_in4_addr *nbr_id)
{
  struct ospf_restart_tlv *tlv;
  u_int32_t pid;
  struct pal_in4_addr aid;
  struct pal_in4_addr nid;
  u_int32_t crypt_seqnum;
  u_char *p;
  u_char *lim;

  tlv = ospf_restart_tlv_lookup_by_type (top, OSPF_NSM_RESTART_VLINK_CRYPT_SEQNUM_TLV);
  if (tlv == NULL)
    return 0;

  p = tlv->data;
  lim = p + pal_ntoh16 (tlv->length);

  while (p < lim)
    {
      pid = OSPF_PNT_UINT32_GET (p, 0);
      pal_mem_cpy (&aid, p + 4, sizeof (struct pal_in4_addr));
      pal_mem_cpy (&nid, p + 8, sizeof (struct pal_in4_addr));
      crypt_seqnum = OSPF_PNT_UINT32_GET (p, 12);

      if (pid == proc_id
          && IPV4_ADDR_SAME (&aid, area_id)
          && IPV4_ADDR_SAME (&nid, nbr_id))
        return crypt_seqnum;

      p += 16;
    }
  return 0;
}
#ifdef HAVE_OSPF_MULTI_AREA
u_int32_t
ospf_restart_multi_area_link_crypt_seqnum_lookup (struct ospf *top, u_int16_t proc_id,
                                                  struct pal_in4_addr *area_id,
                                                  struct pal_in4_addr *nbr_id)
{
  struct ospf_restart_tlv *tlv = NULL;
  struct pal_in4_addr aid;
  struct pal_in4_addr nid;
  u_int32_t crypt_seqnum = 0;
  u_char *p = NULL;
  u_char *lim = NULL;
  u_int32_t pid;

  tlv = ospf_restart_tlv_lookup_by_type (top,
        OSPF_NSM_RESTART_MULTI_AREA_LINK_CRYPT_SEQNUM_TLV);
  if (tlv == NULL)
    return 0;

  p = tlv->data;
  lim = p + pal_ntoh16 (tlv->length);

  while (p < lim)
    {
      pid = OSPF_PNT_UINT32_GET (p, 0);
      pal_mem_cpy (&aid, p + 4, sizeof (struct pal_in4_addr));
      pal_mem_cpy (&nid, p + 8, sizeof (struct pal_in4_addr));
      crypt_seqnum = OSPF_PNT_UINT32_GET (p, 12);

      if (pid == proc_id
          && IPV4_ADDR_SAME (&aid, area_id)
          && IPV4_ADDR_SAME (&nid, nbr_id))
        return crypt_seqnum;

      p += 16;
    }
  return 0;
}
#endif /* HAVE_OSPF_MULTI_AREA */


struct ospf_restart_info
{
  u_int32_t flags;

  u_int32_t grace_period;

  u_char restart_reason;

  struct pal_in4_addr addr;
};

void
ospf_restart_grace_lsa_parse (struct ospf_lsa *lsa,
                              struct ospf_restart_info *info)
{
  u_int16_t len = pal_ntoh16 (lsa->data->length);
  u_char *p = (u_char *)lsa->data;
  u_char *lim = p + len;
  u_int16_t tlv_type;
  u_int16_t tlv_len;

  pal_mem_set (info, 0, sizeof (struct ospf_restart_info));

  p += OSPF_LSA_HEADER_SIZE;
  while (p < lim)
    {
      tlv_type = OSPF_PNT_UINT16_GET (p, 0);
      tlv_len = OSPF_PNT_UINT16_GET (p, 2);

      switch (tlv_type)
        {
        case OSPF_GRACE_LSA_GRACE_PERIOD:
          info->grace_period = OSPF_PNT_UINT32_GET (p, 4);
          SET_FLAG (info->flags, (1 << tlv_type));
          break;
        case OSPF_GRACE_LSA_RESTART_REASON:
          info->restart_reason = OSPF_PNT_UCHAR_GET (p, 4);
          SET_FLAG (info->flags, (1 << tlv_type));
          break;
        case OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS:
          pal_mem_cpy (&info->addr, p + 4, sizeof (struct pal_in4_addr));
          SET_FLAG (info->flags, (1 << tlv_type));
          break;
        }

      p += OSPF_TLV_SPACE (tlv_len);
    }
}

void
ospf_restart_helper_enter_by_lsa (struct ospf_lsa *lsa)
{
  struct ospf_interface *oi = lsa->lsdb->u.oi;
  struct ospf_master *om = oi->top->om;
  struct ospf_neighbor *nbr = NULL;
  struct ospf_restart_info info;
  struct ospf_lsdb *lsdb = NULL;
  struct ospf_lsa *find = NULL;
  struct ls_node *rn = NULL;
  struct pal_in4_addr *router_id = NULL;
  struct listnode *nn = NULL;
  u_int8_t i,j;

  ospf_restart_grace_lsa_parse (lsa, &info);

  /* Grace Period TLV is not present. */
  if (!CHECK_FLAG (info.flags, (1 << OSPF_GRACE_LSA_GRACE_PERIOD))
      || info.grace_period == 0)
    {
      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "ROUTER: Can't enter Helper mode, "
                   "because grace-period TLV is not present");
      return;
    }

  /* Restart reason is not present. */
  if (!CHECK_FLAG (info.flags, (1 << OSPF_GRACE_LSA_RESTART_REASON)))
    {
      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "Router: Can't enter Helper mode, "
                   "because restart reason TLV is not present");
      return;
    }

  /* Restart reason not matched our local policy. */
  if (CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_POLICY))
    {
      if (om->helper_policy == OSPF_RESTART_HELPER_NEVER)
        return;

      LIST_LOOP (om->never_restart_helper_list, router_id, nn)
        if (IPV4_ADDR_SAME (&lsa->data->adv_router, router_id))
          return;

      if (om->helper_policy == OSPF_RESTART_HELPER_ONLY_RELOAD
               || om->helper_policy == OSPF_RESTART_HELPER_ONLY_UPGRADE)
        {
          if (info.restart_reason != OSPF_RESTART_REASON_UPGRADE)
            {
              if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
                zlog_info (ZG, "Router: Can't enter Helper mode, "
                           "because restart reason is not matched "
                           "to our policy");

              return;
            }
        }
    }

  /* Grace period exceeds limit. */
  if (CHECK_FLAG (om->config, OSPF_GLOBAL_CONFIG_RESTART_HELPER_GRACE_PERIOD))
    if (om->max_grace_period < info.grace_period)
      {
        if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
          zlog_info (ZG, "Router: Can't enter Helper mode, "
                     "because grace period exceeds limit");
        return;
      }

  /* RFC-3623(Section-3.1): The grace period has not yet expired.
     This means that the LS age of the grace-LSA is less than the
     grace period specified in the body of the grace-LSA*/
  if (LS_AGE(lsa) >= info.grace_period)
    {
      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "Router: Can't enter Helper mode, "
                       "because age of the grace-LSA is greater than "
                       "or equal to the grace period");
        return;
    }

   /* RFC-3623(Section-3.1): Helper Router is not in the process of
      graceful restart*/
  if (IS_GRACEFUL_RESTART (oi->top))
    {
      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "Router: Can't enter Helper mode, "
                       "because router is already in grace restart mode");
        return;
    }

  /* Lookup neighbor from interface address or advetising router. */
  if (oi->type == OSPF_IFTYPE_BROADCAST
      || oi->type == OSPF_IFTYPE_NBMA
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    {
      if (CHECK_FLAG (info.flags,
                      (1 << OSPF_GRACE_LSA_IP_INTERFACE_ADDRESS)))
        nbr = ospf_nbr_lookup_by_addr (oi->nbrs, &info.addr);
    }
  else
    nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, &lsa->data->adv_router);

  /* Enter Helper Mode if Rxmt-List has only periodic refreshes */
  if (nbr != NULL)
    {
      for (i = 0; i < OSPF_RETRANSMIT_LISTS; i++)
        {
          if ((lsdb = nbr->ls_rxmt_list[i]) != NULL)
            for (j = OSPF_ROUTER_LSA; j <= OSPF_AS_NSSA_LSA; j++)
              {
                if (j == OSPF_GROUP_MEMBER_LSA)
                  continue;
                for (rn = ls_table_top (lsdb->type[j].db); rn; 
                          rn = ls_route_next (rn))
                  {
                    find = RN_INFO (rn, RNI_LSDB_LSA);
                    if (find && !CHECK_FLAG (find->flags, OSPF_LSA_REFRESHED))
                      {
                        ls_unlock_node (rn);
                        if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
                          zlog_info (ZG, "Router: Can't enter Helper mode, "
                            "because LSA(s) other than periodic refreshes exist "
                            "on this neighbor's retransmit list");
                        return;
                      }
                  }
              } 
        }   

      SET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART);
      OSPF_NFSM_TIMER_ON (nbr->t_helper, ospf_restart_helper_exit_timer,
                          info.grace_period);
    }
}

void
ospf_restart_helper_exit_by_lsa (struct ospf_lsa *lsa)
{
  struct ospf_interface *oi = lsa->lsdb->u.oi;
  struct ospf_master *om = oi->top->om;
  struct ospf_neighbor *nbr = NULL;
  struct ospf_restart_info info;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  ospf_restart_grace_lsa_parse (lsa, &info);

  /* Lookup neighbor from interface address or advetising router. */
  if (oi->type == OSPF_IFTYPE_BROADCAST
      || oi->type == OSPF_IFTYPE_NBMA
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    {
      if (CHECK_FLAG (info.flags, (1 << OSPF_GRACE_LSA_RESTART_REASON)))
        nbr = ospf_nbr_lookup_by_addr (oi->nbrs, &info.addr);
    }
  else
    nbr = ospf_nbr_lookup_by_router_id (oi->nbrs, &lsa->data->adv_router);

  if (nbr != NULL)
    {
      if (IS_DEBUG_OSPF (event, EVENT_ROUTER))
        zlog_info (ZG, "ROUTER: Exit Restart Helper mode for "
                   "neighbor(%s) by receiving maxage grace-LSA",
                   NBR_NAME (nbr));

      OSPF_NFSM_TIMER_OFF (nbr->t_inactivity);
      OSPF_NFSM_TIMER_ON (nbr->t_inactivity, ospf_inactivity_timer,
                          nbr->v_inactivity);
      ospf_restart_helper_exit (nbr);
    }
}

void
ospf_restart_set_dr_by_hello (struct ospf_neighbor *nbr,
                              struct ospf_hello *hello)
{
  struct ospf_interface *oi = nbr->oi;
  struct ospf_master *om = oi->top->om;
  char if_str[OSPF_IF_STRING_MAXLEN];

  oi->ostate = oi->state;
  oi->state = IFSM_DROther;
  oi->state_change++;

  if (IPV4_ADDR_SAME (&oi->ident.d_router, &OSPFRouterIDUnspec))
    {
      oi->ident.d_router = hello->d_router;
      if (IS_DECLARED_DR (&oi->ident))
        {
          oi->state = IFSM_DR;
          ospf_if_join_alldrouters (oi);
        }
      OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_AdjOK);
    }

  if (IPV4_ADDR_SAME (&oi->ident.bd_router, &OSPFRouterIDUnspec)
      && !IPV4_ADDR_SAME (&oi->ident.d_router, &hello->bd_router))
    {
      oi->ident.bd_router = hello->bd_router;
      if (IS_DECLARED_BACKUP (&oi->ident))
        {
          oi->state = IFSM_Backup;
          ospf_if_join_alldrouters (oi);
        }
      OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_AdjOK);
    }

  if (IS_DEBUG_OSPF (ifsm, IFSM_EVENTS)
      || IS_DEBUG_OSPF (event, EVENT_ROUTER))
    zlog_info (ZG, "IFSM[%s]: Status change %s -> %s (Restart)", IF_NAME (oi),
               LOOKUP (ospf_ifsm_state_msg, oi->ostate),
               LOOKUP (ospf_ifsm_state_msg, oi->state));
}


/* Support of Restart Signaling
   - draft-nguyen-ospf-lls-03.txt.
   - draft-nguyen-ospf-oob-resync-03.txt.
   - draft-nguyen-ospf-restart-03.txt. */

u_int16_t
ospf_restart_lls_size_get (struct ospf_interface *oi)
{
  u_int16_t size;

  /* LLS Header. */
  size = OSPF_LLS_HEADER_LEN;

  /* LLS Extended Options TLV. */
  size += OSPF_LLS_TLV_HEADER_LEN + OSPF_LLS_TLV_EXTENDED_OPTIONS_LEN;

  /* LLS Cryptographic Authentication TLV. */
  if (ospf_auth_type (oi) == OSPF_AUTH_CRYPTOGRAPHIC)
    size += OSPF_LLS_TLV_HEADER_LEN + OSPF_LLS_TLV_CRYPT_AUTH_LEN;

  return size;
}

#define OSPF_LLS_TLV_SET_HEADER(P,T,L)                                        \
    do {                                                                      \
      OSPF_PNT_UINT16_PUT ((P), 0, (T));                                      \
      OSPF_PNT_UINT16_PUT ((P), 2, (L));                                      \
      (P) += OSPF_LLS_TLV_HEADER_LEN;                                         \
    } while (0)

void
ospf_restart_lls_block_set (struct ospf_interface *oi, struct ospf_packet *op)
{
  u_int16_t size;
  u_int32_t options = 0;
  u_char *p = op->buf + op->length;
  u_char *ep = NULL;
  struct ospf_lls_header *lls = NULL;

#ifdef HAVE_MD5
  if (CHECK_FLAG (op->flags, OSPF_PACKET_FLAG_MD5))
    {
      pal_mem_set (p, 0, OSPF_AUTH_MD5_SIZE);
      p += OSPF_AUTH_MD5_SIZE;
    }
#endif /* HAVE_MD5 */

  lls = (struct ospf_lls_header *)p;

  /* Skip LLS Data Block header, set later. */
  lls->checksum = 0;
  lls->length = 0;
  p += OSPF_LLS_HEADER_LEN;

  /* Set Exteneded Options TLV. */
  SET_FLAG (options, OSPF_EXTENDED_OPTION_LR);

  /* RS Bit Set in TLV */
  if (CHECK_FLAG (oi->flags, OSPF_IF_RS_BIT_SET))
    {
      SET_FLAG (options, OSPF_EXTENDED_OPTION_RS);
      if (oi->full_nbr_count == 0)
        ospf_restart_check_intermediate (oi->top);
    }

  OSPF_LLS_TLV_SET_HEADER (p, OSPF_LLS_TLV_EXTENDED_OPTIONS,
                           OSPF_LLS_TLV_EXTENDED_OPTIONS_LEN);
  OSPF_PNT_UINT32_PUT (p, 0, options);
  p += OSPF_LLS_TLV_EXTENDED_OPTIONS_LEN;

  /* Set Cryptographic Authentication TLV. */
#ifdef HAVE_MD5
  if (CHECK_FLAG (op->flags, OSPF_PACKET_FLAG_MD5))
    {
      OSPF_LLS_TLV_SET_HEADER (p, OSPF_LLS_TLV_CRYPT_AUTH,
                               OSPF_LLS_TLV_CRYPT_AUTH_LEN);
      pal_mem_set (p, 0, OSPF_LLS_TLV_CRYPT_AUTH_LEN);

      /* seqnum is incremented to consistent with v2 packet  */
      OSPF_PNT_UINT32_PUT (p, 0, (oi->crypt_seqnum + 1));
      p += 4;

      /* Preserve pointer for digest. */
      ep = p;

      pal_mem_set (p, 0, OSPF_AUTH_MD5_SIZE);
      p += OSPF_AUTH_MD5_SIZE;
    }
#endif /* HAVE_MD5 */

  /* Set length by word count :( */
  size = p - (u_char *)lls;
  lls->length = pal_hton16 (size / 4);

#ifdef HAVE_MD5
  /* Calculate MD5 digest here. */
  if (ep != NULL)
    {
      struct ospf_crypt_key *ck;
      u_char digest[OSPF_AUTH_MD5_SIZE];

      ck = ospf_if_message_digest_key_get_last (oi);
      ospf_packet_md5_digest_calc (ck, (u_char *)lls,
                                   ep - (u_char *)lls, digest);
      pal_mem_cpy (ep, digest, OSPF_AUTH_MD5_SIZE);
    }
#endif /* HAVE_MD5 */

  /* Calculate checksum for LLS block. */
  if (!CHECK_FLAG (op->flags, OSPF_PACKET_FLAG_MD5))
    lls->checksum = in_checksum ((u_int16_t *)lls, size);

  op->length += size;
}

#define OSPF_LLS_CRYPT_AUTH_DIGEST_OFFSET       4

int
ospf_restart_lls_block_get (struct ospf_header *ospfh,
                            struct ospf_interface *oi,
                            u_char *buf, u_int16_t length,
                            struct ospf_lls *lls)
{
  struct ospf_master *om = oi->top->om;
  struct ospf_lls_header *header;
  u_char *p = buf;
  u_char *lim = p + length;
  u_char *ca = NULL;
  u_int16_t type;
  u_int16_t tlvlen;
  u_int16_t sum, ret;

  if (length < OSPF_LLS_HEADER_LEN)
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_info (ZG, "RECV[%s]: LLS block short (%d)",
                   LOOKUP (ospf_packet_type_msg, ospfh->type), length);
      return -1;
    }

  pal_mem_set (lls, 0, sizeof (struct ospf_lls));

  /* Get LLS header. */
  header = (struct ospf_lls_header *)p;
  if ((pal_ntoh16 (header->length) * 4) != length)
    {
      if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
        zlog_info (ZG, "RECV[%s]: LLS header length invalid (%d != %d)",
                   LOOKUP (ospf_packet_type_msg, ospfh->type),
                   pal_ntoh16 (header->length) * 4, length);
      return -1;
    }

  if (pal_ntoh16 (ospfh->auth_type) != OSPF_AUTH_CRYPTOGRAPHIC)
    {
      /* Check LLS checksum. */
      sum = header->checksum;
      pal_mem_set (&header->checksum, 0, sizeof (u_int16_t));
      ret = in_checksum ((u_int16_t *)buf, length);
      header->checksum = sum;

      if (ret != sum)
        {
          if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
            zlog_info (ZG, "RECV[%s]: LLS checksum invalid (%04x != %04x)",
                       LOOKUP (ospf_packet_type_msg, ospfh->type), sum, ret);
          return -1;
        }
    }

  p += OSPF_LLS_HEADER_LEN;

  while (p < lim)
    {
      if (p + OSPF_LLS_TLV_HEADER_LEN > lim)
        {
          if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
            zlog_info (ZG, "RECV[%s]: LLS block TLV header short",
                       LOOKUP (ospf_packet_type_msg, ospfh->type));
          return -1;
        }

      type = OSPF_PNT_UINT16_GET (p, 0);
      tlvlen = OSPF_PNT_UINT16_GET (p, 2);
      p += OSPF_LLS_TLV_HEADER_LEN;

      if (p + tlvlen > lim)
        {
          if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
            zlog_info (ZG, "RECV[%s]: LLS block TLV corrupted",
                       LOOKUP (ospf_packet_type_msg, ospfh->type));
          return -1;
        }

      switch (type)
        {
        case OSPF_LLS_TLV_EXTENDED_OPTIONS:
          if (tlvlen != OSPF_LLS_TLV_EXTENDED_OPTIONS_LEN)
            {
              if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
                zlog_info (ZG, "RECV[%s]: LLS Extended Options TLV "
                           "length invalid (%d != %d)",
                           LOOKUP (ospf_packet_type_msg, ospfh->type),
                           tlvlen, OSPF_LLS_TLV_EXTENDED_OPTIONS_LEN);
              return -1;
            }

          lls->eo.options = OSPF_PNT_UINT32_GET (p, 0);
          break;
#ifdef HAVE_MD5
        case OSPF_LLS_TLV_CRYPT_AUTH:
          if (tlvlen != OSPF_LLS_TLV_CRYPT_AUTH_LEN)
            {
              if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
                zlog_info (ZG, "RECV[%s]: LLS Cryptographic Auth TLV "
                           "length invalid (%d != %d)",
                           LOOKUP (ospf_packet_type_msg, ospfh->type),
                           tlvlen, OSPF_LLS_TLV_CRYPT_AUTH_LEN);
              return -1;
            }

          lls->ca.crypt_seqnum = OSPF_PNT_UINT32_GET (p, 0);
          if (lls->ca.crypt_seqnum != pal_ntoh32 (ospfh->u.crypt.crypt_seqnum))
            {
              if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
                zlog_info (ZG, "RECV[%s]: LLS Cryptographic Auth TLV "
                           "Seqnum mismatch (%08x != %08x)",
                           LOOKUP (ospf_packet_type_msg, ospfh->type),
                           lls->ca.crypt_seqnum,
                           pal_ntoh32 (ospfh->u.crypt.crypt_seqnum));
              return -1;
            }

          pal_mem_cpy (lls->ca.digest, (p + 4), OSPF_AUTH_MD5_SIZE);

          ca = p + OSPF_LLS_CRYPT_AUTH_DIGEST_OFFSET;
          break;
#endif /* HAVE_MD5 */
        default:
          break;
        }
      p += tlvlen;
    }

#ifdef HAVE_MD5
  if (ca != NULL)
    {
      struct ospf_crypt_key *ck;
      u_char digest[OSPF_AUTH_MD5_SIZE];

      ck = ospf_if_message_digest_key_get (oi, ospfh->u.crypt.key_id);
      ospf_packet_md5_digest_calc (ck, buf, ca - buf, digest);

      if (pal_mem_cmp (lls->ca.digest, digest, OSPF_AUTH_MD5_SIZE) != 0)
        {
          if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
            zlog_info (ZG, "RECV[%s]: LLS MD5 authentication error",
                       LOOKUP (ospf_packet_type_msg, ospfh->type));
          return -1;
        }
    }
#endif /* HAVE_MD5 */

  return 0;
}

int
ospf_restart_resync_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  nbr->t_resync = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: Resync timer expire", NBR_NAME (nbr));

  UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART);
  OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_OneWayReceived);

  return 0;
}

void
ospf_restart_resync_timer_set (struct ospf_neighbor *nbr)
{
  if (nbr->t_resync == NULL)
    {
      OSPF_NFSM_TIMER_ON (nbr->t_resync, ospf_restart_resync_timer,
                          ospf_if_resync_timeout_get (nbr->oi));

      SET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART);
    }

  /* Send unicast hello back to neighbor. */
  OSPF_EVENT_EXECUTE (ospf_hello_reply_event, nbr, 0);
}

/* The RS bit is Set,as passing the Extended Options TLV in LLS */
void
ospf_restart_extended_options_check (struct ospf_neighbor *nbr,
                                     u_int32_t eo_options )
{
  /* Update OOBResync Capability. */
  if (CHECK_FLAG (eo_options, OSPF_EXTENDED_OPTION_LR))
    SET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_CAPABLE);
  else
    UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_CAPABLE);

  if (CHECK_FLAG (eo_options, OSPF_EXTENDED_OPTION_RS))
    ospf_restart_resync_timer_set (nbr);
  else if (CHECK_FLAG (nbr->eo_options, OSPF_EXTENDED_OPTION_RS))
    /* Stop the timer */
    OSPF_TIMER_OFF (nbr->t_resync);

  nbr->eo_options = eo_options;
}

int
ospf_restart_dd_resync_proc (struct ospf_neighbor *nbr,
                             struct ospf_header *ospfh,
                             struct ospf_db_desc *dd)
{
  struct ospf_master *om = nbr->oi->top->om;
  struct ospf_interface *oi = nbr->oi;
  struct ospf *top = oi->top;
  struct stream *s = top->ibuf;
  struct ospf_lls lls;
  u_int16_t length;
  u_char *buf;

  length = stream_get_endp (s) - pal_ntoh16 (ospfh->length);
  buf = STREAM_DATA (s) + pal_ntoh16 (ospfh->length);

  if (pal_ntoh16 (ospfh->auth_type) == OSPF_AUTH_CRYPTOGRAPHIC)
    {
      length -= OSPF_AUTH_MD5_SIZE;
      buf += OSPF_AUTH_MD5_SIZE;
    }

  /* LLS block invalid. */
  if (ospf_restart_lls_block_get (ospfh, oi, buf, length, &lls) < 0)
    return -1;

  if (CHECK_FLAG (dd->flags, OSPF_DD_FLAG_R))
    if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_CAPABLE))
      {
        OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);

        if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
          zlog_info (ZG, "RECV[%s]: R-bit set, "
                     "while neighbor is OOBResync incapable",
                     LOOKUP (ospf_packet_type_msg, ospfh->type));
        return -1;
      }

  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS))
    {
      if (!CHECK_FLAG (dd->flags, OSPF_DD_FLAG_R))
        {
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);

          if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
            zlog_info (ZG, "RECV[%s]: R-bit is not set, "
                       "while OOBResync flag is set",
                       LOOKUP (ospf_packet_type_msg, ospfh->type));

          UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS);
          UNSET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_R);
          OSPF_TIMER_OFF (nbr->t_resync);
          ospf_db_desc_send (nbr);
          nbr->last_oob = pal_time_current (NULL);
          return -1;
        }
    }
  else
    {
      if (CHECK_FLAG (dd->flags, OSPF_DD_FLAG_R))
        {
          /* Go into OOBResync. */
          if (nbr->state == NFSM_Full
              && CHECK_FLAG (dd->flags, OSPF_DD_FLAG_I)
              && CHECK_FLAG (dd->flags, OSPF_DD_FLAG_M)
              && CHECK_FLAG (dd->flags, OSPF_DD_FLAG_MS))
            {
              SET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS);
              nbr->state = NFSM_ExStart;

              OSPF_TIMER_OFF (nbr->t_resync);
              UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART);

              /* To speed up, resync process,
                 reply back DD to restarting router if we are Master. */
              if (IPV4_ADDR_CMP (&nbr->ident.router_id, &top->router_id) < 0)
                {
                  SET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_MS);
                  SET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_M);
                  SET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_I);
                  ospf_db_desc_send (nbr);
                }
            }
          else
            {
              char dd_str[OSPF_DD_BITS_STR_MAXLEN];

              if (nbr->state >= NFSM_Exchange)
                OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_SeqNumberMismatch);

              if (IS_DEBUG_OSPF_PACKET (ospfh->type, RECV))
                zlog_info (ZG, "RECV[%s]: Invalid OOB DD packet received, "
                           "neighbor state[%s], bits[%s]",
                           LOOKUP (ospf_packet_type_msg, ospfh->type),
                           LOOKUP (ospf_nfsm_state_msg, nbr->state),
                           ospf_dd_bits_dump (dd->flags, dd_str));
              return -1;
            }
        }
    }

  return 0;
}


int
ospf_restart_finish (struct ospf_master *om)
{
  /* Cancel timer.  */
  OSPF_TIMER_OFF (om->t_restart);

  /* Reset the preserve timer.  */
  ospf_nsm_preserve_unset (om);

  return 1;
}

#endif /* HAVE_RESTART */
