/* Copyright (C) 2003,  All Rights Reserved. */

/* Mux state machine (Section 43.4.15) */

#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_rcv.h"
#include "lacp_timer.h"
#include "lacp_mux.h"
#include "lacp_link.h"
#include "lacp_main.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

char *
mux_state_str (enum lacp_mux_state s)
{
  switch (s)
    {
    case MUX_DETACHED: 
      return "Detached";
    case MUX_WAITING: 
      return "Waiting";
    case MUX_ATTACHED:
      return "Attached";
    case MUX_COLLECTING: 
      return "Collecting";
    case MUX_DISTRIBUTING: 
      return "Distributing";
    case MUX_COLLECTING_DISTRIBUTING:
      return "Collecting/Distributing";
    default:
      return "Unknown";
    }
}

/* Figure 43-13 */

/* MUX DETACHED state */

static void
mux_detached (struct lacp_link *link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "mux_detached: link %d mux state change %s -> %s",
                  link->actor_port_number,
                  mux_state_str (link->mux_machine_state), 
                  mux_state_str (MUX_DETACHED));
    }

  link->mux_machine_state = MUX_DETACHED;
  CLR_SYNCHRONIZATION (link->actor_oper_port_state);
#if defined (MUX_INDEPENDENT_CTRL)
  disable_distributing (link);
  CLR_DISTRIBUTING (link->actor_oper_port_state);
  CLR_COLLECTING (link->actor_oper_port_state);
  disable_collecting (link);
#else
  CLR_COLLECTING (link->actor_oper_port_state);
  disable_collecting_distributing (link);
  CLR_DISTRIBUTING (link->actor_oper_port_state);
#endif
  link->ntt = PAL_TRUE;
}

/* MUX WAITING state */

static void
mux_waiting (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "mux_waiting: link %d mux state change %s -> %s",
                  link->actor_port_number,
                  mux_state_str (link->mux_machine_state), mux_state_str(MUX_WAITING));
    }
  link->mux_machine_state = MUX_WAITING;
  if (link->wait_while_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_wait_while_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->wait_while_timer);
    }

  LACP_TIMER_ON (link->wait_while_timer, lacp_actor_wait_while_timer_expiry, 
                 link, AGGREGATE_WAIT_TIME);

#ifdef HAVE_HA
  lacp_cal_create_wait_while_timer (link);
#endif /* HAVE_HA */
}

/* MUX ATTACHED state */

static void
mux_attached (struct lacp_link *link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "mux_attached: link %d mux state change %s -> %s",
                  link->actor_port_number,
                  mux_state_str (link->mux_machine_state), mux_state_str(MUX_ATTACHED));
    }
  lacp_attempt_to_aggregate_link (link);
  link->mux_machine_state = MUX_ATTACHED;
  SET_SYNCHRONIZATION (link->actor_oper_port_state);
  link->actor_sync_transition_count++;
  CLR_COLLECTING (link->actor_oper_port_state);
#if defined (MUX_INDEPENDENT_CTRL)
  disable_collecting (link);
#else
  disable_collecting_distributing (link);
  CLR_DISTRIBUTING (link->actor_oper_port_state);
#endif
  link->ntt = PAL_TRUE;
}

#if defined (MUX_INDEPENDENT_CTRL)

/* MUX COLLECTING and MUX DISTRIBUTING states are defined for systems 
   that allow for independent control of the collecting and 
   distributing functions. */

/* MUX COLLECTING state */

static void
mux_collecting (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "mux_collecting: link %d mux state change %s -> %s",
                  link->actor_port_number,
                  mux_state_str (link->mux_machine_state), mux_state_str(MUX_COLLECTING));
    }
  link->mux_machine_state = MUX_COLLECTING;
  enable_collecting (link);
  SET_COLLECTING (link->actor_oper_port_state);
  disable_distributing (link);
  CLR_DISTRIBUTING (link->actor_oper_port_state);
  link->ntt = PAL_TRUE;
}

/* MUX DISTRIBUTING state */

static void
mux_distributing (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "mux_distributing: link %d mux state change %s -> %s",
                  link->actor_port_number,
                  mux_state_str (link->mux_machine_state), mux_state_str(MUX_DISTRIBUTING));
    }
  link->mux_machine_state = MUX_DISTRIBUTING;
  SET_DISTRIBUTING (link->actor_oper_port_state);
  enable_distributing (link);
}

#else

static void
mux_collecting_distributing (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "mux_collecting_distributing: link %d mux state change %s -> %s",
                  link->actor_port_number,
                  mux_state_str (link->mux_machine_state), mux_state_str(MUX_COLLECTING_DISTRIBUTING));
    }
  link->mux_machine_state = MUX_COLLECTING_DISTRIBUTING;
  SET_DISTRIBUTING (link->actor_oper_port_state);
  enable_collecting_distributing (link);
  SET_COLLECTING (link->actor_oper_port_state);
  link->ntt = PAL_TRUE;
}

#endif
  
/* lacp_mux_sm executes the MUX state machine transitions */

void
lacp_mux_sm (struct lacp_link * link)
{
  if (begin() != PAL_FALSE)
    {
      mux_detached (link);
    }
  else if (begin() == PAL_FALSE) 
    {
      switch (link->mux_machine_state)
        {
        case MUX_DETACHED:
          {
            if ((link->selected == SELECTED) || (link->selected == STANDBY))
              {
                mux_waiting (link);
              }
          }
          break;
        case MUX_WAITING:
          {
            if ((link->selected == SELECTED) && (link->aggregator->ready != PAL_FALSE))
              {
                mux_attached (link);
              }
            else if (link->selected == UNSELECTED)
              {
                mux_detached (link);
              }
          }
          break;
        case MUX_ATTACHED:
          {
            if ((link->selected == SELECTED) 
                && (GET_SYNCHRONIZATION (link->partner_oper_port_state)))
              {
#if defined (MUX_INDEPENDENT_CTRL)
                mux_collecting (link);
#else
                mux_collecting_distributing (link);
#endif
              }
            else if ((link->selected == UNSELECTED) || (link->selected == STANDBY))
              {
                mux_detached (link);
              }
          }
          break;
#if defined (MUX_INDEPENDENT_CTRL)
        case MUX_COLLECTING:
          {
            if ((link->selected == UNSELECTED) || (link->selected == STANDBY) 
                || (GET_SYNCHRONIZATION (link->partner_oper_port_state) == PAL_FALSE))
              {
                mux_attached (link);
              }
            else if (((link->selected == SELECTED) 
                      && (GET_SYNCHRONIZATION (link->partner_oper_port_state) != PAL_FALSE))
                     && (GET_COLLECTING (link->partner_oper_port_state) != PAL_FALSE))
              {
                mux_distributing (link);
              }
          }
          break;
        case MUX_DISTRIBUTING:
          {
            if (((link->selected == UNSELECTED) || (link->selected == STANDBY) 
                 || (GET_SYNCHRONIZATION (link->partner_oper_port_state) == PAL_FALSE))
                || (GET_COLLECTING (link->partner_oper_port_state) == PAL_FALSE))
              {
                mux_collecting (link);
              }
          }
          break;
#else
        case MUX_COLLECTING_DISTRIBUTING:
          {
            if ((link->selected == UNSELECTED) || (link->selected == STANDBY) 
                || (GET_SYNCHRONIZATION (link->partner_oper_port_state) == PAL_FALSE))
              {
                mux_attached (link);
              }
          }
          break;
#endif
        default:
          mux_detached (link);
          break;
        }
    }
}
