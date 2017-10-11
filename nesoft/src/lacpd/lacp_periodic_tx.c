/* Copyright (C) 2003,  All Rights Reserved. */

/* Receive state machine (Section 43.4.12) */

#include "lacp_types.h"
#include "lacp_rcv.h"
#include "lacp_timer.h"

#include "lacp_periodic_tx.h"
#include "lacp_link.h"
#include "lacpd.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

char *
periodic_state_str (enum lacp_periodic_tx_state s)
{
  switch (s)
    {
    case PERIODIC_TX_INVALID:
      return "Invalid";
    case PERIODIC_TX_NO_PERIODIC:
      return "No periodic";
    case PERIODIC_TX_FAST_PERIODIC:
      return "Fast periodic";
    case PERIODIC_TX_SLOW_PERIODIC:
      return "Slow periodic";
    case PERIODIC_TX:
      return "Periodic";
    default:
      return "Unknown";
    }
}

/* Implemenation of Figure 43-11 */


static void
periodic_tx_fast_periodic (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "periodic_tx_fast_periodic: link %d periodic tx state %s -> %s",
                  link->actor_port_number,
                  periodic_state_str (link->periodic_tx_state), periodic_state_str (PERIODIC_TX_FAST_PERIODIC));
    }

  link->periodic_tx_state = PERIODIC_TX_FAST_PERIODIC;

   if (link->periodic_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_periodic_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->periodic_timer);
    }

  LACP_TIMER_ON (link->periodic_timer, lacp_actor_periodic_timer_expiry, 
                 link, FAST_PERIODIC_TIME);

#ifdef HAVE_HA
  lacp_cal_create_periodic_timer (link);
#endif /* HAVE_HA */
}

static void
periodic_tx_no_periodic (struct lacp_link *link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "periodic_tx_no_periodic: link %d periodic tx state "
                  "%s -> %s",
                  link->actor_port_number,
                  periodic_state_str (link->periodic_tx_state), 
                  periodic_state_str (PERIODIC_TX_NO_PERIODIC));
    }

  link->periodic_tx_state = PERIODIC_TX_NO_PERIODIC;

  if (link->periodic_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_periodic_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->periodic_timer);
    }

  /* If the port is not enabled then we should be in no periodic.
   * Section 43.4.13 paragraph 4.
   */

  if ( (link->lacp_enabled == PAL_FALSE) || 
       (link->port_enabled == PAL_FALSE) )
    return;

  if (! ((GET_LACP_ACTIVITY (link->actor_oper_port_state) ==
     LACP_ACTIVITY_PASSIVE) &&
     (GET_LACP_ACTIVITY (link->partner_oper_port_state) ==
     LACP_ACTIVITY_PASSIVE)))
    periodic_tx_fast_periodic (link);
}

static void
periodic_tx_slow_periodic (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "periodic_tx_slow_periodic: link %d periodic tx state %s -> %s",
                  link->actor_port_number,
                  periodic_state_str (link->periodic_tx_state), periodic_state_str (PERIODIC_TX_SLOW_PERIODIC));
    }
  link->periodic_tx_state = PERIODIC_TX_SLOW_PERIODIC;

  if (link->periodic_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_periodic_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->periodic_timer);
    }

  LACP_TIMER_ON (link->periodic_timer, lacp_actor_periodic_timer_expiry, 
                 link, SLOW_PERIODIC_TIME);
#ifdef HAVE_HA
  lacp_cal_create_periodic_timer (link);
#endif /* HAVE_HA */
}

static void
periodic_tx (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "periodic_tx: link %d ntt -> true",
                  link->actor_port_number);
    }

  link->ntt = PAL_TRUE;

  if (GET_LACP_TIMEOUT(link->partner_oper_port_state) == LACP_TIMEOUT_SHORT)
    periodic_tx_fast_periodic (link);
  else
    periodic_tx_slow_periodic (link);
}


/* Execute the Periodic Transmission state machine transitions */

void
lacp_periodic_tx_sm (struct lacp_link * link)
{
  if ((begin() == PAL_TRUE)
      || (link->lacp_enabled == PAL_FALSE)
      || (link->port_enabled == PAL_FALSE)
      || ((GET_LACP_ACTIVITY(link->actor_oper_port_state) ==
           LACP_ACTIVITY_PASSIVE)
          && (GET_LACP_ACTIVITY(link->partner_oper_port_state) == 
              LACP_ACTIVITY_PASSIVE)))
    {
      periodic_tx_no_periodic (link);
    }
  else 
    {
      switch (link->periodic_tx_state)
        {
        case PERIODIC_TX_FAST_PERIODIC:
          {
            if (link->periodic_timer_expired == PAL_TRUE)
              {
                periodic_tx (link);
              }
            else if (GET_LACP_TIMEOUT(link->partner_oper_port_state) 
                     == LACP_TIMEOUT_LONG)
              {
                periodic_tx_slow_periodic (link);
              }
          }
          break;
        case PERIODIC_TX_SLOW_PERIODIC:
          {
            if ((link->periodic_timer_expired == PAL_TRUE)
                || (GET_LACP_TIMEOUT(link->partner_oper_port_state)
                    == LACP_TIMEOUT_SHORT))
              {
                periodic_tx (link);
              }
          }
          break;
        case PERIODIC_TX:
          {
            if (GET_LACP_TIMEOUT(link->partner_oper_port_state)
                == LACP_TIMEOUT_SHORT)
              {
                periodic_tx_fast_periodic (link);
              }
            else
              {
                /* Must be long */
                periodic_tx_slow_periodic (link);
              }
          }
          break;
        default:
          /* Bad state? */
          periodic_tx_no_periodic (link);
          break;
        }
    }
}

