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
#include "ospfd/ospf_ia.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_OSPF_TE
#include "ospfd/ospf_te.h"
#endif /* HAVE_OSPF_TE */
#ifdef HAVE_BFD
#include "ospfd/ospf_bfd.h"
#endif /* HAVE_BFD */
#ifdef HAVE_SNMP
#include "ospfd/ospf_snmp.h"
#endif /* HAVE_SNMP */

void ospf_nfsm_change_state (struct ospf_neighbor *, int);


/* DB summary list. */
int
ospf_db_summary_count (struct ospf_neighbor *nbr)
{
  return ospf_lsdb_count_all (nbr->db_sum);
}

int
ospf_db_summary_isempty (struct ospf_neighbor *nbr)
{
  return ospf_lsdb_isempty (nbr->db_sum);
}

int
ospf_db_summary_add (struct ospf_lsa *lsa, struct ospf_neighbor *nbr)
{
  struct ospf_master *om = nbr->oi->top->om;
  char lsa_str[OSPF_LSA_STRING_MAXLEN];

  if (lsa == NULL)
    return 0;

  if (IS_LSA_MAXAGE (lsa))
    {
      if (IS_DEBUG_OSPF (lsa, LSA_MAXAGE))
        zlog_info (ZG, "LSA[%s]: is MaxAge, add to retransmit list",
                   LSA_NAME (lsa));
      ospf_ls_retransmit_add (nbr, lsa);
    }
  else
    ospf_lsdb_add (nbr->db_sum, lsa);

  return 0;
}

void
ospf_db_summary_clear (struct ospf_neighbor *nbr)
{
  struct ospf_lsdb *lsdb;
  struct ls_node *rn;
  struct ospf_lsa *lsa;
  int i;

  lsdb = nbr->db_sum;

  for (i = OSPF_MIN_LSA; i < OSPF_MAX_LSA; i++)
    for (rn = ls_table_top (lsdb->type[i].db); rn; rn = ls_route_next (rn))
      if ((lsa = RN_INFO (rn, RNI_DEFAULT)) != NULL)
        ospf_lsdb_delete (lsdb, lsa);
}


void
ospf_nfsm_reset_nbr (struct ospf_neighbor *nbr)
{
  int i;

  /* Clear Database Summary list. */
  if (!ospf_db_summary_isempty (nbr))
    ospf_db_summary_clear (nbr);

  /* Clear Link State Request list. */
  if (!ospf_ls_request_isempty (nbr))
    ospf_ls_request_delete_all (nbr);

  /* Clear Link State Retransmission list. */
  if (!ospf_ls_retransmit_isempty (nbr))
    ospf_ls_retransmit_clear (nbr);

  /* Cleanup from the DD pending list.  */
  ospf_nbr_delete_dd_pending (nbr);

  /* Cancel thread. */
  OSPF_NFSM_TIMER_OFF (nbr->t_dd_inactivity);
  OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
  OSPF_NFSM_TIMER_OFF (nbr->t_ls_req);
  OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);

  for (i = 0; i < OSPF_NFSM_EVENT_MAX; i++)
    OSPF_NFSM_TIMER_OFF (nbr->t_events [i]);
}


/* OSPF NFSM Timer functions. */
int
ospf_inactivity_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  nbr->t_inactivity = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: Inactivity timer expire", NBR_NAME (nbr));

#ifdef HAVE_RESTART
  if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
#endif /* HAVE_RESTART */
    OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_InactivityTimer);

  return 0;
}

int
ospf_dd_inactivity_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  nbr->t_dd_inactivity = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: DD inactivity timer expire", NBR_NAME (nbr));

#ifdef HAVE_RESTART
  if (nbr->oi->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
    {
      UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS);
      UNSET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_R);
      OSPF_TIMER_OFF (nbr->t_resync);
      ospf_db_desc_send (nbr);     

      nbr->last_oob = pal_time_current (NULL);
    }
#endif /* HAVE_RESTART */

  OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_AdjOK);

  return 0;
}

int
ospf_db_desc_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  nbr->t_db_desc = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: DD Retransmit timer expire", NBR_NAME (nbr));

  /* resent last send DD packet. */
  pal_assert (nbr->dd.sent.packet != NULL);
  ospf_db_desc_resend (nbr, PAL_TRUE);

  /* DD Retransmit timer set. */
  OSPF_NFSM_TIMER_ON (nbr->t_db_desc, ospf_db_desc_timer, nbr->v_db_desc);

  return 0;
}

int
ospf_db_desc_free_timer (struct thread *thread)
{
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  nbr->t_db_desc_free = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (nfsm, NFSM_TIMERS))
    zlog_info (ZG, "NFSM[%s]: DD packet free timer expire", NBR_NAME (nbr));

  /* Free the packet.  */
  if (nbr->dd.sent.packet != NULL)
    ospf_packet_add_unuse (nbr->oi->top, nbr->dd.sent.packet);

  nbr->dd.sent.packet = NULL;

  return 0;
}

/* Hook function called after ospf NFSM event is occured. */

void
ospf_nfsm_timer_set (struct ospf_neighbor *nbr)
{
  switch (nbr->state)
    {
    case NFSM_Down:
      OSPF_NFSM_TIMER_OFF (nbr->t_dd_inactivity);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
      OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);
      break;
    case NFSM_Attempt:
      OSPF_NFSM_TIMER_OFF (nbr->t_dd_inactivity);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
      OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);
      break;
    case NFSM_Init:
      OSPF_NFSM_TIMER_OFF (nbr->t_dd_inactivity);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
      OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);
      break;
    case NFSM_TwoWay:
      OSPF_NFSM_TIMER_OFF (nbr->t_dd_inactivity);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
      OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);
      break;
    case NFSM_ExStart:
      OSPF_NFSM_TIMER_ON (nbr->t_dd_inactivity,
                          ospf_dd_inactivity_timer, nbr->v_dd_inactivity);
      OSPF_NFSM_TIMER_ON (nbr->t_db_desc, ospf_db_desc_timer, nbr->v_db_desc);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
      OSPF_NFSM_TIMER_OFF (nbr->t_ls_upd);
      break;
    case NFSM_Exchange:
      if (!IS_DD_FLAGS_SET (&nbr->dd, FLAG_MS))
        OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
      break;
    case NFSM_Loading:
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc_free);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
      break;
    case NFSM_Full:
      OSPF_NFSM_TIMER_OFF (nbr->t_dd_inactivity);
      OSPF_NFSM_TIMER_OFF (nbr->t_db_desc);
      OSPF_NFSM_TIMER_OFF (nbr->t_ls_req);
      break;
    default:
      break;
    }
}


/* OSPF NFSM functions. */
int
ospf_nfsm_ignore (struct ospf_neighbor *nbr)
{
  struct ospf_master *om = nbr->oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  if (IS_DEBUG_OSPF (nfsm, NFSM_EVENTS))
    zlog_info (ZG, "NFSM[%s]: nfsm_ignore called", NBR_NAME (nbr));

  return 0;
}

int
ospf_nfsm_hello_received (struct ospf_neighbor *nbr)
{
  /* Start or Restart Inactivity Timer. */
  OSPF_NFSM_TIMER_OFF (nbr->t_inactivity);
  OSPF_NFSM_TIMER_ON (nbr->t_inactivity, ospf_inactivity_timer,
                      nbr->v_inactivity);

  if (nbr->oi->type == OSPF_IFTYPE_NBMA)
    ospf_nbr_static_poll_timer_cancel (nbr);

  return 0;
}

static int
ospf_nfsm_start (struct ospf_neighbor *nbr)
{
  ospf_nfsm_reset_nbr (nbr);

  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC))
    ospf_nbr_static_poll_timer_cancel (nbr);

  OSPF_NFSM_TIMER_OFF (nbr->t_inactivity);
  OSPF_NFSM_TIMER_ON (nbr->t_inactivity, ospf_inactivity_timer,
                      nbr->v_inactivity);

  return 0;
}

static int
ospf_nfsm_twoway_received (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  int next_state = NFSM_TwoWay;

  /* These netowork types must be adjacency. */
  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_VIRTUALLINK
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    next_state = NFSM_ExStart;

  /* Router itself is the DRouter or the BDRouter. */
  if (IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, &oi->ident.d_router)
      || IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, &oi->ident.bd_router))
    next_state = NFSM_ExStart;

  /* Neighboring Router is the DRouter or the BDRouter. */
  if (IPV4_ADDR_SAME (&nbr->ident.address->u.prefix4, &oi->ident.d_router)
      || IPV4_ADDR_SAME (&nbr->ident.address->u.prefix4, &oi->ident.bd_router))
    next_state = NFSM_ExStart;

  /* The number of neighbors in the concurrent DD exchange process
     is limited.  */
  if (next_state == NFSM_ExStart)
    if (IS_OSPF_NBR_DD_LIMIT (nbr))
      {
        ospf_nbr_add_dd_pending (nbr);
        return NFSM_TwoWay;
      }

  return next_state;
}


/* The area link state database consists of the router-LSAs,
   network-LSAs and summary-LSAs contained in the area structure,
   along with the AS-external- LSAs contained in the global structure.
   AS-external-LSAs are omitted from a virtual neighbor's Database
   summary list.  AS-external-LSAs are omitted from the Database
   summary list if the area has been configured as a stub. */
static int
ospf_nfsm_negotiation_done (struct ospf_neighbor *nbr)
{
  struct ospf_area *area = nbr->oi->area;
  struct ospf_lsa *lsa;

  /* If database-filter is set, we don't put db-summary at all. */
  if (ospf_if_database_filter_get (nbr->oi))
    return 0;

  /* Add Type 1-4 LSDB.  */
  for (lsa = ROUTER_LSDB_HEAD (area); lsa; lsa = lsa->next)
    ospf_db_summary_add (lsa, nbr);
  for (lsa = NETWORK_LSDB_HEAD (area); lsa; lsa = lsa->next)
    ospf_db_summary_add (lsa, nbr);
  for (lsa = SUMMARY_LSDB_HEAD (area); lsa; lsa = lsa->next)
    ospf_db_summary_add (lsa, nbr);
  if (IS_AREA_DEFAULT (area))
    for (lsa = ASBR_SUMMARY_LSDB_HEAD (area); lsa; lsa = lsa->next)
      ospf_db_summary_add (lsa, nbr);

#ifdef HAVE_NSSA
  /* Add Type 7 LSDB.  */
  for (lsa = NSSA_LSDB_HEAD (area); lsa; lsa = lsa->next)
    ospf_db_summary_add (lsa, nbr);
#endif /* HAVE_NSSA */

#ifdef HAVE_OPAQUE_LSA
  /* Add Type 9-11 LSDB.  */
  if (IS_OPAQUE_CAPABLE (area->top) && IS_NBR_OPAQUE_CAPABLE (nbr))
    {
      for (lsa = LINK_OPAQUE_LSDB_HEAD (nbr->oi); lsa; lsa = lsa->next)
        ospf_db_summary_add (lsa, nbr);
      for (lsa = AREA_OPAQUE_LSDB_HEAD (area); lsa; lsa = lsa->next)
        ospf_db_summary_add (lsa, nbr);
      if (nbr->oi->type != OSPF_IFTYPE_VIRTUALLINK && IS_AREA_DEFAULT (area))
        for (lsa = AS_OPAQUE_LSDB_HEAD (area->top); lsa; lsa = lsa->next)
          ospf_db_summary_add (lsa, nbr);
    }
#endif /* HAVE_OPAQUE_LSA */

  /* Add Type 5 LSDB.  */
  if (nbr->oi->type != OSPF_IFTYPE_VIRTUALLINK && IS_AREA_DEFAULT (area))
    for (lsa = EXTERNAL_LSDB_HEAD (area->top); lsa; lsa = lsa->next)
      ospf_db_summary_add (lsa, nbr);

  return 0;
}

static int
ospf_nfsm_exchange_done (struct ospf_neighbor *nbr)
{
  if (ospf_ls_request_isempty (nbr))
    return NFSM_Full;

  /* Send Link State Request. */
  ospf_ls_req_send (nbr);

  return NFSM_Loading;
}

static int
ospf_nfsm_bad_ls_req (struct ospf_neighbor *nbr)
{
  /* Clear neighbor. */
  ospf_nfsm_reset_nbr (nbr);

  return 0;
}

static int
ospf_nfsm_adj_ok (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  int next_state = nbr->state;
  int flag = 0;

  /* These netowork types must be adjacency. */
  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_VIRTUALLINK
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    flag = 1;

  /* Router itself is the DRouter or the BDRouter. */
  if (IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, &oi->ident.d_router)
      || IPV4_ADDR_SAME (&oi->ident.address->u.prefix4, &oi->ident.bd_router))
    flag = 1;

  if (IPV4_ADDR_SAME (&nbr->ident.address->u.prefix4, &oi->ident.d_router)
      || IPV4_ADDR_SAME (&nbr->ident.address->u.prefix4, &oi->ident.bd_router))
    flag = 1;

  if (nbr->state == NFSM_TwoWay && flag == 1)
    next_state = NFSM_ExStart;

  else if (nbr->state >= NFSM_ExStart && flag == 0)
    {
       /* RFC 2328 Section 10.3.  The Neighbor state machine
       State(s):  ExStart or greater

            Event:  AdjOK?

        New state:  Depends upon action routine.

           Action:  Determine whether the neighboring router should
                    still be adjacent.  If yes, there is no state change
                    and no further action is necessary.

                    Otherwise, the (possibly partially formed) adjacency
                    must be destroyed.  The neighbor state transitions
                    to 2-Way.  The Link state retransmission list,
                    Database summary list and Link state request list
                    are cleared of LSAs.*/
     
      next_state = NFSM_TwoWay;

      /* Clear neighbor. */
      ospf_nfsm_reset_nbr (nbr);
    }

  /* The number of neighbors in the concurrent DD exchange process
     is limited.  */
  if (next_state == NFSM_ExStart)
    if (IS_OSPF_NBR_DD_LIMIT (nbr))
      {
        ospf_nbr_add_dd_pending (nbr);
        return NFSM_TwoWay;
      }

  return next_state;
}

static int
ospf_nfsm_seq_number_mismatch (struct ospf_neighbor *nbr)
{
  /* Clear neighbor. */
  ospf_nfsm_reset_nbr (nbr);

  return 0;
}

static int
ospf_nfsm_oneway_received (struct ospf_neighbor *nbr)
{
  /* Clear neighbor. */
  ospf_nfsm_reset_nbr (nbr);

  return 0;
}

static int
ospf_nfsm_kill_nbr (struct ospf_neighbor *nbr)
{
  struct ospf_interface *oi = nbr->oi;
  struct ospf_master *om = oi->top->om;
  char nbr_str[OSPF_NBR_STRING_MAXLEN];

  /* call it here because we cannot call it from ospf_nfsm_event */
  ospf_nfsm_change_state (nbr, NFSM_Down);

  /* Reset neighbor. */
  ospf_nfsm_reset_nbr (nbr);

  /* Start poll timer on NBMA. */
  if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
    if (oi->type == OSPF_IFTYPE_NBMA
        || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
      {
        struct ospf_nbr_static *nbr_static;

        nbr_static = ospf_nbr_static_lookup (oi->top,
                                             nbr->ident.address->u.prefix4);
        if (nbr_static)
          {
            OSPF_TIMER_OFF (nbr_static->t_poll);
            nbr_static->nbr = nbr;
            SET_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC);

            OSPF_EVENT_ON (nbr_static->t_poll,
                           ospf_hello_poll_timer, nbr_static);

            if (IS_DEBUG_OSPF (nfsm, NFSM_EVENTS))
              zlog_info (ZG, "NFSM[%s]: Down (PollIntervalTimer scheduled)",
                         NBR_NAME (nbr));
          }
    }

  if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_STATIC))
    ospf_nbr_delete (nbr);

#ifdef HAVE_OSPF_TE
  if (oi->type != OSPF_IFTYPE_VIRTUALLINK)
    {
      int nbr_count = 0;

      if (oi->top->router_id.s_addr != 0)
        nbr_count = ospf_nbr_count_full (oi);

      if (nbr_count == 0)
        ospf_te_withdraw_link (oi->top, oi->u.ifp);
    }
#endif /* HAVE_OSPF_TE */

  return 0;
}

static int
ospf_nfsm_inactivity_timer (struct ospf_neighbor *nbr)
{
  /* Kill neighbor. */
  ospf_nfsm_kill_nbr (nbr);

  return 0;
}

static int
ospf_nfsm_ll_down (struct ospf_neighbor *nbr)
{
  /* Kill neighbor. */
  ospf_nfsm_kill_nbr (nbr);

  return 0;
}

/* Neighbor State Machine */
struct {
  int (*func) ();
  int next_state;
} NFSM [OSPF_NFSM_STATE_MAX][OSPF_NFSM_EVENT_MAX] =
{
  {
    /* DependUpon: dummy state. */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* HelloReceived     */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* ExchangeDone      */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* LoadingDone       */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* AdjOK?            */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* SeqNumberMismatch */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* 1-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* KillNbr           */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* InactivityTimer   */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* LLDown            */
  },
  {
    /* Down: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_Init       }, /* HelloReceived     */
    { ospf_nfsm_start,               NFSM_Attempt    }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* ExchangeDone      */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* LoadingDone       */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* AdjOK?            */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* SeqNumberMismatch */
    { ospf_nfsm_ignore,              NFSM_Down       }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
  {
    /* Attempt: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_Init       }, /* HelloReceived     */
    { ospf_nfsm_start,               NFSM_Attempt    }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* ExchangeDone      */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* LoadingDone       */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* AdjOK?            */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* SeqNumberMismatch */
    { ospf_nfsm_ignore,              NFSM_Attempt    }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
  {
    /* Init: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_Init       }, /* HelloReceived     */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* Start             */
    { ospf_nfsm_twoway_received,     NFSM_DependUpon }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* ExchangeDone      */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* LoadingDone       */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* AdjOK?            */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* SeqNumberMismatch */
    { ospf_nfsm_ignore,              NFSM_Init       }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
  {
    /* 2-Way: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_TwoWay     }, /* HelloReceived     */
    { ospf_nfsm_ignore,              NFSM_TwoWay     }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_TwoWay     }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_TwoWay     }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_TwoWay     }, /* ExchangeDone      */
    { ospf_nfsm_ignore,              NFSM_TwoWay     }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_TwoWay     }, /* LoadingDone       */
    { ospf_nfsm_adj_ok,              NFSM_DependUpon }, /* AdjOK?            */
    { ospf_nfsm_ignore,              NFSM_TwoWay     }, /* SeqNumberMismatch */
    { ospf_nfsm_oneway_received,     NFSM_Init       }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
  {
    /* ExStart: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_ExStart    }, /* HelloReceived     */
    { ospf_nfsm_ignore,              NFSM_ExStart    }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_ExStart    }, /* 2-WayReceived     */
    { ospf_nfsm_negotiation_done,    NFSM_Exchange   }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_ExStart    }, /* ExchangeDone      */
    { ospf_nfsm_ignore,              NFSM_ExStart    }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_ExStart    }, /* LoadingDone       */
    { ospf_nfsm_adj_ok,              NFSM_DependUpon }, /* AdjOK?            */
    { ospf_nfsm_ignore,              NFSM_ExStart    }, /* SeqNumberMismatch */
    { ospf_nfsm_oneway_received,     NFSM_Init       }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
  {
    /* Exchange: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_Exchange   }, /* HelloReceived     */
    { ospf_nfsm_ignore,              NFSM_Exchange   }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_Exchange   }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_Exchange   }, /* NegotiationDone   */
    { ospf_nfsm_exchange_done,       NFSM_DependUpon }, /* ExchangeDone      */
    { ospf_nfsm_bad_ls_req,          NFSM_ExStart    }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_Exchange   }, /* LoadingDone       */
    { ospf_nfsm_adj_ok,              NFSM_DependUpon }, /* AdjOK?            */
    { ospf_nfsm_seq_number_mismatch, NFSM_ExStart    }, /* SeqNumberMismatch */
    { ospf_nfsm_oneway_received,     NFSM_Init       }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
  {
    /* Loading: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_Loading    }, /* HelloReceived     */
    { ospf_nfsm_ignore,              NFSM_Loading    }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_Loading    }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_Loading    }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_Loading    }, /* ExchangeDone      */
    { ospf_nfsm_bad_ls_req,          NFSM_ExStart    }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_Full       }, /* LoadingDone       */
    { ospf_nfsm_adj_ok,              NFSM_DependUpon }, /* AdjOK?            */
    { ospf_nfsm_seq_number_mismatch, NFSM_ExStart    }, /* SeqNumberMismatch */
    { ospf_nfsm_oneway_received,     NFSM_Init       }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
  { /* Full: */
    { ospf_nfsm_ignore,              NFSM_DependUpon }, /* NoEvent           */
    { ospf_nfsm_hello_received,      NFSM_Full       }, /* HelloReceived     */
    { ospf_nfsm_ignore,              NFSM_Full       }, /* Start             */
    { ospf_nfsm_ignore,              NFSM_Full       }, /* 2-WayReceived     */
    { ospf_nfsm_ignore,              NFSM_Full       }, /* NegotiationDone   */
    { ospf_nfsm_ignore,              NFSM_Full       }, /* ExchangeDone      */
    { ospf_nfsm_bad_ls_req,          NFSM_ExStart    }, /* BadLSReq          */
    { ospf_nfsm_ignore,              NFSM_Full       }, /* LoadingDone       */
    { ospf_nfsm_adj_ok,              NFSM_DependUpon }, /* AdjOK?            */
    { ospf_nfsm_seq_number_mismatch, NFSM_ExStart    }, /* SeqNumberMismatch */
    { ospf_nfsm_oneway_received,     NFSM_Init       }, /* 1-WayReceived     */
    { ospf_nfsm_kill_nbr,            NFSM_Down       }, /* KillNbr           */
    { ospf_nfsm_inactivity_timer,    NFSM_Down       }, /* InactivityTimer   */
    { ospf_nfsm_ll_down,             NFSM_Down       }, /* LLDown            */
  },
};

void
ospf_nfsm_change_state (struct ospf_neighbor *nbr, int state)
{
  char nbr_str[OSPF_NBR_STRING_MAXLEN];
  struct ospf_interface *oi = nbr->oi;
  struct ospf *top = oi->top;
  struct ospf_master *om = top->om;
  struct ospf_area *transit = NULL;

  /* Change to new state and preserve old status. */
  nbr->ostate = nbr->state;
  nbr->state = state;
  nbr->state_change++;

  /* Logging change of state. */
  if (IS_DEBUG_OSPF (nfsm, NFSM_STATUS))
    zlog_info (ZG, "NFSM[%s]: Status change %s -> %s", NBR_NAME (nbr),
               LOOKUP (ospf_nfsm_state_msg, nbr->ostate),
               LOOKUP (ospf_nfsm_state_msg, nbr->state));

  /* Reset the neighbor capability options if the state decreases.  */
  if (state < nbr->ostate)
    nbr->options = 0;

  /* Schedule NeighborChange IFSM event.  */
  if ((nbr->ostate < NFSM_TwoWay && state >= NFSM_TwoWay))
    {
      if (CHECK_FLAG (top->config, OSPF_CONFIG_DB_SUMMARY_OPT))
        SET_FLAG (nbr->flags, OSPF_NEIGHBOR_DB_SUMMARY_OPT);

      OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_NeighborChange);
    }
  else if (nbr->ostate >= NFSM_TwoWay && state < NFSM_TwoWay)
    {
      UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_DB_SUMMARY_OPT);
      OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_NeighborChange);

      /* Shutdown the neighbor from the nexthop table.  */
      ospf_nexthop_nbr_down (nbr);
    }

#ifdef HAVE_SNMP
  if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK)
    ospfVirtNbrStateChange (nbr);
  else
    ospfNbrStateChange (nbr);
#endif /* HAVE_SNMP */

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    transit = ospf_area_entry_lookup (top, oi->u.vlink->area_id);

#ifdef HAVE_BFD
  ospf_bfd_update_session (nbr);
#endif /* HAVE_BFD */

  /* One of the neighboring routers changes to/from the FULL state. */
  if ((nbr->ostate != NFSM_Full && state == NFSM_Full)
      || (nbr->ostate == NFSM_Full && state != NFSM_Full))
    {
      if (state == NFSM_Full)
        {
          oi->full_nbr_count++;
          oi->area->full_nbr_count++;
          if (transit != NULL)
            transit->full_virt_nbr_count++;

          UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_DB_SUMMARY_OPT);
          /* Schedule the thread to free the last sent
             DB description packet.  */
          if (nbr->dd.sent.packet != NULL)
            OSPF_NFSM_TIMER_ON (nbr->t_db_desc_free,
                                ospf_db_desc_free_timer, nbr->v_inactivity);
        }
      else
        {
          oi->full_nbr_count--;
          oi->area->full_nbr_count--;
          if (transit != NULL)
            transit->full_virt_nbr_count--;

          /* clear neighbor retransmit list */
          if (!ospf_ls_retransmit_isempty (nbr))
            ospf_ls_retransmit_clear (nbr);
        }

      /* Update the ABR status.  */
      ospf_abr_status_update (top);

#ifdef HAVE_RESTART
      if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART)
          && !CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS))
#endif /* HAVE_RESTART */
        {
          LSA_REFRESH_TIMER_ADD (oi->top, OSPF_ROUTER_LSA,
                                 ospf_router_lsa_self (oi->area), oi->area);

          /* Refresh V flag of the transit area's router LSA.  */
          if (transit != NULL)
            if (transit->full_virt_nbr_count == 0
                || transit->full_virt_nbr_count == 1)
              LSA_REFRESH_TIMER_ADD (transit->top, OSPF_ROUTER_LSA,
                                     ospf_router_lsa_self (transit), transit);
        }

#ifdef HAVE_RESTART 
      /* Ensure resync flag unset in Full. */
      if (oi->top->restart_method == OSPF_RESTART_METHOD_SIGNALING)
        if (state == NFSM_Full)
          {
            if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS))
              {
                UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS);
                UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART);
                OSPF_TIMER_OFF (nbr->t_resync);

                nbr->last_oob = pal_time_current (NULL);
              }
          }
#endif /* HAVE_RESTART */

      /* Originate network-LSA. */
      if (oi->state == IFSM_DR)
        {
#ifdef HAVE_RESTART
          if (!CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
#endif /* HAVE_RESTART */
            {
              struct ospf_lsa *lsa = ospf_network_lsa_self (oi);

              if (oi->full_nbr_count == 0)
                {
                  if (lsa != NULL)
                    ospf_lsdb_lsa_discard (lsa->lsdb, lsa, 1, 0, 0);
                }
              else
                /* The DR originates the network-LSA only if it is fully
                   adjacent to at least one other router on the network. */ 
                LSA_REFRESH_TIMER_ADD (oi->top, OSPF_NETWORK_LSA, lsa, oi);
            }
        }
    }

#ifdef HAVE_RESTART
  if (state < NFSM_ExStart)
    UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS);
#endif /* HAVE_RESTART */

  /* Start DD exchange protocol. */
  if (state == NFSM_ExStart)
    {
      if (!IS_NFSM_DD_EXCHANGE (nbr->ostate))
        OSPF_NBR_DD_INCREMENT (nbr);

      if (nbr->dd.seqnum == 0)
        nbr->dd.seqnum = pal_time_current (NULL);
      else
        nbr->dd.seqnum++;

      nbr->dd.flags = OSPF_DD_FLAG_ALL;
#ifdef HAVE_RESTART
      if (IS_RESTART_SIGNALING (top))
        {
          SET_FLAG (nbr->dd.flags, OSPF_DD_FLAG_R);
          SET_FLAG (nbr->flags, OSPF_NEIGHBOR_RESYNC_PROCESS);
        }
#endif /* HAVE_RESTART */

      ospf_db_desc_send (nbr);
    }
  else if (!IS_NFSM_DD_EXCHANGE (state))
    if (IS_NFSM_DD_EXCHANGE (nbr->ostate))
      {
        OSPF_NBR_DD_DECREMENT (nbr);

        /* Check the DD pending neighbors.  */
        ospf_nbr_dd_pending_check (nbr->oi->top);
      }

  /* Make sure the neighbor is not in the pending DD list.  */
  if (state != NFSM_TwoWay)
    ospf_nbr_delete_dd_pending (nbr);

  /* Clear cryptographic sequence number. */
  if (state == NFSM_Down)
    nbr->crypt_seqnum = 0;
}

/* Execute NFSM event process. */
int
ospf_nfsm_event (struct thread *thread)
{
  char nbr_str[OSPF_NBR_STRING_MAXLEN];
  struct ospf_neighbor *nbr = THREAD_ARG (thread);
  struct ospf_master *om = nbr->oi->top->om;
  struct pal_in4_addr router_id;
  int event;
  int next_state;
  int old_state;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  event = THREAD_VAL (thread);

  nbr->t_events[event] = NULL;

  router_id = nbr->ident.router_id;
  old_state = nbr->state;

  /* Call function. */
  next_state = (*(NFSM [nbr->state][event].func))(nbr);

  /* When event is NFSM_KillNbr or InactivityTimer, the neighbor is deleted. */
  if (event == NFSM_KillNbr || event == NFSM_InactivityTimer)
    /* Timers are canceled in ospf_nbr_free, moreover we cannot call
       nfsm_timer_set here because nbr is freed already!!!  */
    return 0;

  if (! next_state)
    next_state = NFSM[nbr->state][event].next_state;

  if (IS_DEBUG_OSPF (nfsm, NFSM_STATUS))
    zlog_info (ZG, "NFSM[%s]: %s (%s)", NBR_NAME (nbr),
               LOOKUP (ospf_nfsm_state_msg, nbr->state),
               LOOKUP (ospf_nfsm_event_msg, event));

  /* Neighbor state is changed. */
  if (next_state != nbr->state)
    {
      int old_state;

      old_state = nbr->state;
      ospf_nfsm_change_state (nbr, next_state);
      ospf_call_notifiers (om, OSPF_NOTIFY_NFSM_STATE_CHANGE, nbr,
                           (void *)old_state);
    }

  /* Make sure timer is set. */
  ospf_nfsm_timer_set (nbr);

  return 0;
}

/* Check loading state. */
void
ospf_nfsm_check_nbr_loading (struct ospf_neighbor *nbr)
{
  if (nbr->state == NFSM_Loading)
    {
      if (ospf_ls_request_isempty (nbr))
        OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_LoadingDone);
      else if (nbr->ls_req_last == NULL)
        ospf_ls_req_event (nbr);
    }
}
