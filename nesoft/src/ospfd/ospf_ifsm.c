/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "thread.h"
#include "linklist.h"
#include "vector.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_network.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_ifsm.h"
#include "ospfd/ospf_nfsm.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_flood.h"
#include "ospfd/ospf_route.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_nsm.h"
#include "ospfd/ospf_debug.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_SNMP
#include "ospfd/ospf_snmp.h"
#endif /* HAVE_SNMP */


/* DR election.  Refer to RFC2328 section 9.4. */
int
ospf_ifsm_state (struct ospf_interface *oi)
{
  if (IS_DECLARED_DR (&oi->ident))
    return IFSM_DR;
  else if (IS_DECLARED_BACKUP (&oi->ident))
    return IFSM_Backup;
  else
    return IFSM_DROther;
}

void
ospf_dr_election_init (struct ospf_interface *oi, vector v)
{
  struct ospf_interface *oi_alt;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  struct prefix p, q;
  struct listnode *node;

  /* Set neighbor identity to vector. */
  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->state >= NFSM_TwoWay)
        if (nbr->ident.router_id.s_addr != 0)
          if (nbr->ident.priority != 0)
            vector_set (v, &nbr->ident);

  /* Set self identity to vector. */
  if (CHECK_FLAG (oi->flags, OSPF_IF_DR_ELECTION_READY))
    if (oi->ident.router_id.s_addr != 0)
      if (oi->ident.priority != 0)
        vector_set (v, &oi->ident);

  if (oi->clone > 1)
    {
      prefix_copy (&p, oi->ident.address);
      apply_mask (&p);

      LIST_LOOP (oi->area->iflist, oi_alt, node)
      if (oi != oi_alt)
        if (oi->ident.priority != 0)
          if (oi_alt->clone > 1)
            {
              prefix_copy (&q, oi_alt->ident.address);
              apply_mask (&q);
              if (prefix_same (&p, &q))
                vector_set (v, &oi_alt->ident);
            }
    }
}

/* Choose highest Router Priority.
   In case of tie, choose highest Router ID. */
struct ospf_identity *
ospf_dr_election_tiebreak (vector v)
{
  struct ospf_identity *highest = NULL;
  struct ospf_identity *ident;
  int i;

  for (i = 0; i < vector_max (v); i++)
    if ((ident = vector_slot (v, i)))
      {
        if (highest == NULL)
          highest = ident;
        else
          {
            if (highest->priority < ident->priority)
              highest = ident;
            else if (highest->priority == ident->priority)
              if (pal_ntoh32 (highest->router_id.s_addr) <
                  pal_ntoh32 (ident->router_id.s_addr))
                highest = ident;
          }
      }

  return highest;
}

struct ospf_identity *
ospf_dr_election_dr (struct ospf_interface *oi,
                     struct ospf_identity *bdr, vector v)
{
  struct ospf_identity *dr = NULL;
  struct ospf_identity *ident;
  vector w;
  int i;

  w = vector_copy (v);
  for (i = 0; i < vector_max (w); i++)
    if ((ident = vector_slot (w, i)))
      if (!IS_DECLARED_DR (ident))
        vector_slot (w, i) = NULL;

  dr = ospf_dr_election_tiebreak (w);
  if (dr == NULL)
    dr = bdr;

  if (dr)
    oi->ident.d_router = dr->address->u.prefix4;
  else
    oi->ident.d_router = OSPFRouterIDUnspec;

  vector_free (w);

  return dr;
}

struct ospf_identity *
ospf_dr_election_bdr (struct ospf_interface *oi, vector v)
{
  struct ospf_identity *bdr = NULL;
  struct ospf_identity *ident;
  vector w;
  int i;

  w = vector_copy (v);
  for (i = 0; i < vector_max (w); i++)
    if ((ident = vector_slot (w, i)))
      if (!(IS_DECLARED_BACKUP (ident) && !IS_DECLARED_DR (ident)))
        vector_slot (w, i) = NULL;

  bdr = ospf_dr_election_tiebreak (w);
  if (bdr == NULL)
    {
      vector_free (w);
      w = vector_copy (v);
      for (i = 0; i < vector_max (w); i++)
        if ((ident = vector_slot (w, i)))
          if (IS_DECLARED_DR (ident))
            vector_slot (w, i) = NULL;

      bdr = ospf_dr_election_tiebreak (w);
    }

  if (bdr)
    oi->ident.bd_router = bdr->address->u.prefix4;
  else
    oi->ident.bd_router = OSPFRouterIDUnspec;

  vector_free (w);

  return bdr;
}

/* Generate AdjOK? NFSM event. */
void
ospf_dr_election_dr_change (struct ospf_interface *oi)
{
  struct ls_node *rn;
  struct ospf_neighbor *nbr;

  for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
      if (nbr->ident.router_id.s_addr != 0)
        if (nbr->state >= NFSM_TwoWay)
          OSPF_NFSM_EVENT_SCHEDULE (nbr, NFSM_AdjOK);
}

int
ospf_dr_election (struct ospf_interface *oi)
{
  struct ospf_master *om = oi->top->om;
  struct pal_in4_addr old_dr, old_bdr;
  struct ospf_identity *dr, *bdr;
  int old_state, new_state;
  vector v;
  char if_str[OSPF_IF_STRING_MAXLEN];

#ifdef HAVE_RESTART
  if (IS_GRACEFUL_RESTART (oi->top))
    return oi->state;
#endif /* HAVE_RESTART */

  /* Preserve old values. */
  old_dr = oi->ident.d_router;
  old_bdr = oi->ident.bd_router;
  old_state = oi->state;

  /* Initialize eligible vector. */
  v = vector_init (ospf_nbr_count_all (oi) + 1);
  ospf_dr_election_init (oi, v);

  /* First stage of DR election. */
  bdr = ospf_dr_election_bdr (oi, v);
  ospf_dr_election_dr (oi, bdr, v);
  new_state = ospf_ifsm_state (oi);

  if (IS_DEBUG_OSPF (ifsm, IFSM_EVENTS))
    {
      zlog_info (ZG, "IFSM[%s]: DR-Election[1st]: Backup %r",
                 IF_NAME (oi), &oi->ident.bd_router);
      zlog_info (ZG, "IFSM[%s]: DR-Election[1st]: DR     %r",
                 IF_NAME (oi), &oi->ident.d_router);
    }

  /* Second stage of DR election. */
  if (new_state != old_state &&
      !(new_state == IFSM_DROther && old_state < IFSM_DROther))
    {
      bdr = ospf_dr_election_bdr (oi, v);
      dr = ospf_dr_election_dr (oi, bdr, v);

      if (!IPV4_ADDR_SAME (&oi->ident.d_router, &OSPFRouterIDUnspec))
        if (bdr == dr)
          oi->ident.bd_router = OSPFRouterIDUnspec;

      new_state = ospf_ifsm_state (oi);

      if (IS_DEBUG_OSPF (ifsm, IFSM_EVENTS))
        {
          zlog_info (ZG, "IFSM[%s]: DR-Election[2nd]: Backup %r",
                     IF_NAME (oi), &oi->ident.bd_router);
          zlog_info (ZG, "IFSM[%s]: DR-Election[2nd]: DR     %r",
                     IF_NAME (oi), &oi->ident.d_router);
        }
    }

  vector_free (v);

  /* If DR or BDR changes, cause AdjOK? neighbor event. */
  if (!IPV4_ADDR_SAME (&old_dr, &oi->ident.d_router)
      || !IPV4_ADDR_SAME (&old_bdr, &oi->ident.bd_router))
    ospf_dr_election_dr_change (oi);

  if (!IPV4_ADDR_SAME (&old_dr, &oi->ident.d_router))
    ospf_router_lsa_refresh_by_interface (oi);

  if (if_is_multicast (oi->u.ifp))
    {
      /* Multicast group change. */
      if ((old_state != IFSM_DR && old_state != IFSM_Backup)
          && (new_state == IFSM_DR || new_state == IFSM_Backup))
        ospf_if_join_alldrouters (oi);
      else if ((old_state == IFSM_DR || old_state == IFSM_Backup)
               && (new_state != IFSM_DR && new_state != IFSM_Backup))
        ospf_if_leave_alldrouters (oi);
    }

  return new_state;
}


int
ospf_wait_timer (struct thread *thread)
{
  struct ospf_interface *oi = THREAD_ARG (thread);
  struct ospf_master *om = oi->top->om;
  char if_str[OSPF_IF_STRING_MAXLEN];

  oi->t_wait = NULL;

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  if (IS_DEBUG_OSPF (ifsm, IFSM_TIMERS))
    zlog_info (ZG, "IFSM[%s]: Wait timer expire", IF_NAME (oi));

  OSPF_IFSM_EVENT_SCHEDULE (oi, IFSM_WaitTimer);

  return 0;
}

/* Hook function called after ospf IFSM event is occured. And vty's
   network command invoke this function after making interface
   structure. */
void
ospf_ifsm_timer_set (struct ospf_interface *oi)
{
  int state = !IS_PROC_ACTIVE (oi->top) ? IFSM_Down : oi->state;
  int restart = 0;
  int interval;

#ifdef HAVE_RESTART
  restart = IS_GRACEFUL_RESTART (oi->top);
#endif /* HAVE_RESTART */

  switch (state)
    {
    case IFSM_Down:
      /* First entry point of ospf interface state machine. In this state
         interface parameters must be set to initial values, and timers are
         reset also. */
      OSPF_IFSM_TIMER_OFF (oi->t_hello);
      OSPF_IFSM_TIMER_OFF (oi->t_wait);
      OSPF_IFSM_TIMER_OFF (oi->t_ls_ack);
      OSPF_IFSM_TIMER_OFF (oi->t_ls_upd_event);
#ifdef HAVE_RESTART
      OSPF_IFSM_TIMER_OFF (oi->t_restart_bit_check); 
#endif /* HAVE_RESTART */
#ifdef HAVE_OPAQUE_LSA
      OSPF_IFSM_TIMER_OFF (oi->lsdb->t_opaque_lsa);
#endif /* HAVE_OPAQUE_LSA */
      break;
    case IFSM_Loopback:
      /* In this state, the interface may be looped back and will be
         unavailable for regular data traffic. */
      OSPF_IFSM_TIMER_OFF (oi->t_hello);
      OSPF_IFSM_TIMER_OFF (oi->t_wait);
      OSPF_IFSM_TIMER_OFF (oi->t_ls_ack);
      OSPF_IFSM_TIMER_OFF (oi->t_ls_upd_event);
#ifdef HAVE_RESTART
      OSPF_IFSM_TIMER_OFF (oi->t_restart_bit_check);
#endif /* HAVE_RESTART */
#ifdef HAVE_OPAQUE_LSA
      OSPF_IFSM_TIMER_OFF (oi->lsdb->t_opaque_lsa);
#endif /* HAVE_OPAQUE_LSA */
      break;
    case IFSM_Waiting:
#ifdef HAVE_RESTART
      /* The RS-bit must not be set in Hello packets longer than
         RouterDeadInterval seconds. */
      if (IS_RESTART_SIGNALING (oi->top))
        OSPF_IFSM_TIMER_ON (oi->t_restart_bit_check,
                            ospf_restart_signaling_state_check_timer,
                            ospf_if_dead_interval_get (oi));
#endif /* HAVE_RESTART */

      interval = ospf_if_hello_interval_get (oi);
      /* The router is trying to determine the identity of DRouter and
         BDRouter. The router begin to receive and send Hello Packets. */
      /* Send first Hello immediately only if it's not "Graceful Restart". */
      OSPF_IFSM_TIMER_ON (oi->t_hello, ospf_hello_timer,
                          restart ? interval : 0);
      OSPF_IFSM_TIMER_ON (oi->t_wait, ospf_wait_timer,
                          restart ? interval: ospf_if_dead_interval_get (oi));
      OSPF_IFSM_TIMER_OFF (oi->t_ls_ack);
      break;
    case IFSM_PointToPoint:
#ifdef HAVE_RESTART
      /* The RS-bit must not be set in Hello packets longer than
         RouterDeadInterval seconds. */
      if (IS_RESTART_SIGNALING (oi->top))
        OSPF_IFSM_TIMER_ON (oi->t_restart_bit_check,
                            ospf_restart_signaling_state_check_timer,
                            ospf_if_dead_interval_get (oi));
#endif /* HAVE_RESTART */

      /* The interface connects to a physical Point-to-point network or
         virtual link. The router attempts to form an adjacency with
         neighboring router. Hello packets are also sent. */
      /* send first hello immediately */
      OSPF_IFSM_TIMER_ON (oi->t_hello, ospf_hello_timer, 0);
      OSPF_IFSM_TIMER_OFF (oi->t_wait);
      break;
    case IFSM_DROther:
      /* The network type of the interface is broadcast or NBMA network,
         and the router itself is neither Designated Router nor
         Backup Designated Router. */
      OSPF_IFSM_TIMER_ON (oi->t_hello, ospf_hello_timer,
                          ospf_if_hello_interval_get (oi));
      OSPF_IFSM_TIMER_OFF (oi->t_wait);
      break;
    case IFSM_Backup:
      /* The network type of the interface is broadcast or NBMA network,
         and the router is Backup Designated Router. */
      OSPF_IFSM_TIMER_ON (oi->t_hello, ospf_hello_timer,
                          ospf_if_hello_interval_get (oi));
      OSPF_IFSM_TIMER_OFF (oi->t_wait);
      break;
    case IFSM_DR:
      /* The network type of the interface is broadcast or NBMA network,
         and the router is Designated Router. */
      OSPF_IFSM_TIMER_ON (oi->t_hello, ospf_hello_timer,
                          ospf_if_hello_interval_get (oi));
      OSPF_IFSM_TIMER_OFF (oi->t_wait);
      break;
    }
}

/* Interface State Machine */
int
ospf_ifsm_loop_ind (struct ospf_interface *oi)
{
  int next_state = 0;

  if (oi->state == IFSM_Down)
    if (oi->area->active_if_count++ == 0)
      ospf_area_up (oi->area);

  /* Update Router-LSAs.  */
  ospf_router_lsa_refresh_by_interface (oi);

  /* Update AS-External-LSAs.  */
  ospf_redist_map_update_by_prefix (oi->top, oi->ident.address);
  ospf_redist_map_nexthop_update_by_if_up (oi->top, oi);

  return next_state;
}

int
ospf_ifsm_interface_up (struct ospf_interface *oi)
{
  int next_state = 0;

  /* if network type is point-to-point, Point-to-MultiPoint or virtual link,
     the state transitions to Point-to-Point. */
  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_VIRTUALLINK
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    next_state = IFSM_PointToPoint;
  /* Else if the router is not eligible to DR, the state transitions to
     DROther. */
  else if (oi->ident.priority == 0) /* router is eligible? */
    next_state = IFSM_DROther;
  else
    /* Otherwise, the state transitions to Waiting. */
    next_state = IFSM_Waiting;

  if (oi->area->active_if_count++ == 0)
    ospf_area_up (oi->area);

  if (IS_OSPF_NETWORK_NBMA (oi))
    ospf_nbr_static_if_update (oi);

  /* Update Router-LSAs.  */
  ospf_router_lsa_refresh_by_interface (oi);

  /* Update AS-External-LSAs.  */
  if (oi->type != OSPF_IFTYPE_VIRTUALLINK)
    {
      ospf_redist_map_update_by_prefix (oi->top, oi->ident.address);
      ospf_redist_map_nexthop_update_by_if_up (oi->top, oi);
    }

  return next_state;
}

/* Interface down event handler. */
int
ospf_ifsm_interface_down (struct ospf_interface *oi)
{
  struct ospf_master *om = oi->top->om;
  char if_str[OSPF_IF_STRING_MAXLEN];

  if (oi->type != OSPF_IFTYPE_LOOPBACK)
    {
      OSPF_IFSM_TIMER_OFF (oi->t_hello);
      if (IS_DEBUG_OSPF (ifsm, IFSM_TIMERS))
        zlog_info (ZG, "IFSM[%s]: Hello timer canceled", IF_NAME (oi));
    }

  /* Update the VLINK local address.  */
  ospf_vlink_local_address_update_by_interface (oi);

  /* Update AS-External-LSAs.  */
  if (oi->type != OSPF_IFTYPE_VIRTUALLINK)
    {
      ospf_redist_map_update_by_prefix (oi->top, oi->ident.address);
      ospf_redist_map_nexthop_update_by_if_down (oi->top, oi);
    }

  /* Updte Router-LSAs.  */
  ospf_router_lsa_refresh_by_interface (oi);

  if (oi->area->active_if_count > 0)
    if (--oi->area->active_if_count == 0)
      ospf_area_down (oi->area);

  /* Shutdown the nexthop that belongs to this interface.  */
  ospf_nexthop_if_down (oi);

  /* Reset interface variables.  */
  ospf_if_reset_variables (oi);

  return 0;
}

int
ospf_ifsm_backup_seen (struct ospf_interface *oi)
{
  return ospf_dr_election (oi);
}

int
ospf_ifsm_wait_timer (struct ospf_interface *oi)
{
  return ospf_dr_election (oi);
}

int
ospf_ifsm_neighbor_change (struct ospf_interface *oi)
{
  return ospf_dr_election (oi);
}

int
ospf_ifsm_ignore (struct ospf_interface *oi)
{
  struct ospf_master *om = oi->top->om;
  char if_str[OSPF_IF_STRING_MAXLEN];

  if (IS_DEBUG_OSPF (ifsm, IFSM_EVENTS))
    zlog_info (ZG, "IFSM[%s]: ifsm_ignore called", IF_NAME (oi));

  return 0;
}

/* Interface Finite State Machine. */
struct {
  int (*func) ();
  int next_state;
} IFSM [OSPF_IFSM_STATE_MAX][OSPF_IFSM_EVENT_MAX] =
{
  {
    /* DependUpon: dummy state. */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* NoEvent        */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* InterfaceUp    */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* WaitTimer      */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* BackupSeen     */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* NeighborChange */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* UnloopInd      */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* InterfaceDown  */
  },
  {
    /* Down:*/
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* NoEvent        */
    { ospf_ifsm_interface_up,    IFSM_DependUpon },    /* InterfaceUp    */
    { ospf_ifsm_ignore,          IFSM_Down       },    /* WaitTimer      */
    { ospf_ifsm_ignore,          IFSM_Down       },    /* BackupSeen     */
    { ospf_ifsm_ignore,          IFSM_Down       },    /* NeighborChange */
    { ospf_ifsm_loop_ind,        IFSM_Loopback   },    /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_Down       },    /* UnloopInd      */
    { ospf_ifsm_ignore,          IFSM_Down       },    /* InterfaceDown  */
  },
  {
    /* Loopback: */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* NoEvent        */
    { ospf_ifsm_ignore,          IFSM_Loopback   },    /* InterfaceUp    */
    { ospf_ifsm_ignore,          IFSM_Loopback   },    /* WaitTimer      */
    { ospf_ifsm_ignore,          IFSM_Loopback   },    /* BackupSeen     */
    { ospf_ifsm_ignore,          IFSM_Loopback   },    /* NeighborChange */
    { ospf_ifsm_ignore,          IFSM_Loopback   },    /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_Down       },    /* UnloopInd      */
    { ospf_ifsm_interface_down,  IFSM_Down       },    /* InterfaceDown  */
  },
  {
    /* Waiting: */
    { ospf_ifsm_ignore,          IFSM_DependUpon },    /* NoEvent        */
    { ospf_ifsm_ignore,          IFSM_Waiting    },    /* InterfaceUp    */
    { ospf_ifsm_wait_timer,      IFSM_DependUpon },    /* WaitTimer      */
    { ospf_ifsm_backup_seen,     IFSM_DependUpon },    /* BackupSeen     */
    { ospf_ifsm_ignore,          IFSM_Waiting    },    /* NeighborChange */
    { ospf_ifsm_loop_ind,        IFSM_Loopback   },    /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_Waiting    },    /* UnloopInd      */
    { ospf_ifsm_interface_down,  IFSM_Down       },    /* InterfaceDown  */
  },
  {
    /* Point-to-Point: */
    { ospf_ifsm_ignore,          IFSM_DependUpon   },  /* NoEvent        */
    { ospf_ifsm_ignore,          IFSM_PointToPoint },  /* InterfaceUp    */
    { ospf_ifsm_ignore,          IFSM_PointToPoint },  /* WaitTimer      */
    { ospf_ifsm_ignore,          IFSM_PointToPoint },  /* BackupSeen     */
    { ospf_ifsm_ignore,          IFSM_PointToPoint },  /* NeighborChange */
    { ospf_ifsm_loop_ind,        IFSM_Loopback     },  /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_PointToPoint },  /* UnloopInd      */
    { ospf_ifsm_interface_down,  IFSM_Down         },  /* InterfaceDown  */
  },
  {
    /* DROther: */
    { ospf_ifsm_ignore,          IFSM_DependUpon   },  /* NoEvent        */
    { ospf_ifsm_ignore,          IFSM_DROther      },  /* InterfaceUp    */
    { ospf_ifsm_ignore,          IFSM_DROther      },  /* WaitTimer      */
    { ospf_ifsm_ignore,          IFSM_DROther      },  /* BackupSeen     */
    { ospf_ifsm_neighbor_change, IFSM_DependUpon   },  /* NeighborChange */
    { ospf_ifsm_loop_ind,        IFSM_Loopback     },  /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_DROther      },  /* UnloopInd      */
    { ospf_ifsm_interface_down,  IFSM_Down         },  /* InterfaceDown  */
  },
  {
    /* Backup: */
    { ospf_ifsm_ignore,          IFSM_DependUpon   },  /* NoEvent        */
    { ospf_ifsm_ignore,          IFSM_Backup       },  /* InterfaceUp    */
    { ospf_ifsm_ignore,          IFSM_Backup       },  /* WaitTimer      */
    { ospf_ifsm_ignore,          IFSM_Backup       },  /* BackupSeen     */
    { ospf_ifsm_neighbor_change, IFSM_DependUpon   },  /* NeighborChange */
    { ospf_ifsm_loop_ind,        IFSM_Loopback     },  /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_Backup       },  /* UnloopInd      */
    { ospf_ifsm_interface_down,  IFSM_Down         },  /* InterfaceDown  */
  },
  {
    /* DR: */
    { ospf_ifsm_ignore,          IFSM_DependUpon   },  /* NoEvent        */
    { ospf_ifsm_ignore,          IFSM_DR           },  /* InterfaceUp    */
    { ospf_ifsm_ignore,          IFSM_DR           },  /* WaitTimer      */
    { ospf_ifsm_ignore,          IFSM_DR           },  /* BackupSeen     */
    { ospf_ifsm_neighbor_change, IFSM_DependUpon   },  /* NeighborChange */
    { ospf_ifsm_loop_ind,        IFSM_Loopback     },  /* LoopInd        */
    { ospf_ifsm_ignore,          IFSM_DR           },  /* UnloopInd      */
    { ospf_ifsm_interface_down,  IFSM_Down         },  /* InterfaceDown  */
  },
};

void
ospf_ifsm_change_state (struct ospf_interface *oi, int state)
{
  struct ospf_master *om = oi->top->om;
  struct ospf_lsa *lsa;
  char if_str[OSPF_IF_STRING_MAXLEN];

  oi->ostate = oi->state;
  oi->state = state;
  oi->state_change++;

  /* Logging change of state. */
  if (IS_DEBUG_OSPF (ifsm, IFSM_STATUS))
    zlog_info (ZG, "IFSM[%s]: Status change %s -> %s", IF_NAME (oi),
               LOOKUP (ospf_ifsm_state_msg, oi->ostate),
               LOOKUP (ospf_ifsm_state_msg, oi->state));

#ifdef HAVE_SNMP
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    ospfVirtIfStateChange (oi->u.vlink);
  else
    ospfIfStateChange (oi);
#endif /* HAVE_SNMP */

  /* RFC2328 section 9.4 (6). */
  if (oi->type == OSPF_IFTYPE_NBMA)
    if (oi->ostate != IFSM_DR
        && oi->ostate != IFSM_Backup)
      if (oi->state == IFSM_DR
          || oi->state == IFSM_Backup)
        {
          struct ls_node *rn;
          struct ospf_neighbor *nbr;

          for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
            if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
              if (nbr->ident.priority == 0)
                OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_Start);
        }

  /* Originate network-LSA. */
  if (oi->ostate != IFSM_DR
      && state == IFSM_DR
      && oi->full_nbr_count > 0)
    LSA_REFRESH_TIMER_ADD (oi->top, OSPF_NETWORK_LSA,
                           ospf_network_lsa_self (oi), oi);
  else if (oi->ostate == IFSM_DR && state != IFSM_DR)
    {
      /* Free self originated network LSA. */
      lsa = ospf_network_lsa_self (oi);
      if (lsa)
        ospf_lsdb_lsa_discard (lsa->lsdb, lsa, 1, 0, 0);
    }

  /* Check clone interface. */
  if (oi->clone > 1)
    if ((oi->ostate == IFSM_DR && state != IFSM_DR)
        || (oi->ostate == IFSM_Backup && state != IFSM_Backup))
      {
        struct ospf_interface *oi_alt;
        struct prefix p, q;
        struct listnode *node;

        prefix_copy (&p, oi->ident.address);
        apply_mask (&p);

        LIST_LOOP (oi->area->iflist, oi_alt, node)
          if (oi != oi_alt)
            if (oi_alt->clone > 1)
              {
                prefix_copy (&q, oi_alt->ident.address);
                apply_mask (&q);
                if (prefix_same (&p, &q))
                  OSPF_IFSM_EVENT_SCHEDULE (oi_alt, IFSM_NeighborChange);
              }
      }

  /* Check area border status.  */
  ospf_abr_status_update (oi->top);
}

/* Execute IFSM event process. */
int
ospf_ifsm_event (struct thread *thread)
{
  struct ospf_interface *oi = THREAD_ARG (thread);
  struct ospf_master *om = oi->top->om;
  int event = THREAD_VAL (thread);
  int next_state;
  char if_str[OSPF_IF_STRING_MAXLEN];

  /* Set VR context*/
  LIB_GLOB_SET_VR_CONTEXT (ZG, om->vr);

  /* Call function. */
  next_state = (*(IFSM [oi->state][event].func))(oi);

  if (! next_state)
    next_state = IFSM [oi->state][event].next_state;

  if (IS_DEBUG_OSPF (ifsm, IFSM_EVENTS))
    zlog_info (ZG, "IFSM[%s]: %s (%s)", IF_NAME (oi),
               LOOKUP (ospf_ifsm_state_msg, oi->state),
               LOOKUP (ospf_ifsm_event_msg, event));

  /* If state is changed. */
  if (next_state != oi->state)
    {
      int old_state;

      old_state = oi->state;
      ospf_ifsm_change_state (oi, next_state);
      ospf_call_notifiers (om, OSPF_NOTIFY_IFSM_STATE_CHANGE, oi,
                           (void*)old_state);
    }

  /* Make sure timer is set. */
  ospf_ifsm_timer_set (oi);

  return 0;
}
