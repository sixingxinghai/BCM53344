/* Copyright (C) 2003,  All Rights Reserved. */

/* Receive state machine (Section 43.4.12) */

#include "lacp_types.h"
#include "lacpd.h"
#include "lacp_rcv.h"
#include "lacp_timer.h"
#include "lacp_link.h"
#include "lacp_functions.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

char *
rcv_state_str (enum lacp_rcv_state s)
{
  switch (s)
    {
    case RCV_INVALID:
      return "Invalid";
    case RCV_INITIALIZE:
      return "Initialize";
    case RCV_PORT_DISABLED:
      return "Port disabled";
    case RCV_LACP_DISABLED:
      return "LACP disabled";
    case RCV_EXPIRED:
      return "Expired";
    case RCV_DEFAULTED:
      return "Defaulted";
    case RCV_CURRENT:
      return "Current";
    default:
      return "Unknown";
    }
}

/* Forward declarations */
static void rcv_port_disabled (struct lacp_link * link);

/* Figure 43-10 */

static void
rcv_initialize (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_initialize: link %d receive state machine %s -> %s",
                  link->actor_port_number,
                  rcv_state_str (link->rcv_state), rcv_state_str (RCV_INITIALIZE));
    }

  link->rcv_state = RCV_INITIALIZE;
  
  if (link->aggregator)
    lacp_disaggregate_link (link, PAL_TRUE);  /* link->selected = UNSELECTED */
  else
    link->selected = UNSELECTED;

  lacp_record_default (link);
  CLR_EXPIRED (link->actor_oper_port_state);
  link->port_moved = PAL_FALSE;
  rcv_port_disabled (link);

  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_initialize: link %d unselected",
                  link->actor_port_number);
    }
}


static void
rcv_port_disabled (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_port_disabled: link %d receive state machine %s -> %s",
                  link->actor_port_number,
                  rcv_state_str (link->rcv_state), rcv_state_str (RCV_PORT_DISABLED));
    }

  link->rcv_state = RCV_PORT_DISABLED;
  CLR_SYNCHRONIZATION(link->partner_oper_port_state);
}

static void
rcv_expired (struct lacp_link *link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_expired: link %d receive state machine %s -> %s",
                  link->actor_port_number,
                  rcv_state_str (link->rcv_state), rcv_state_str (RCV_EXPIRED));
    }
  link->rcv_state = RCV_EXPIRED;
  CLR_SYNCHRONIZATION (link->partner_oper_port_state);
  SET_LACP_TIMEOUT (link->partner_oper_port_state);

  if (link->current_while_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_current_while_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->current_while_timer);
    }

  LACP_TIMER_ON (link->current_while_timer, lacp_current_while_timer_expiry, 
                 link, SHORT_TIMEOUT_TIME);
#ifdef HAVE_HA
  lacp_cal_create_current_while_timer (link);
#endif /* HAVE_HA */

  SET_EXPIRED (link->actor_oper_port_state);
}

static void
rcv_lacp_disabled (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_lacp_disabled: link %d receive state machine %s -> %s",
                  link->actor_port_number,
                  rcv_state_str (link->rcv_state), rcv_state_str (RCV_LACP_DISABLED));
    }
  link->rcv_state = RCV_LACP_DISABLED;
  
  if (link->aggregator)
    lacp_disaggregate_link (link, PAL_TRUE); /* link->selected = UNSELECTED; */

  lacp_record_default (link);
  CLR_AGGREGATION (link->partner_oper_port_state);
  CLR_EXPIRED (link->actor_oper_port_state);
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_lacp_disabled: link %d unselected",
                  link->actor_port_number);
    }
}

static void
rcv_defaulted (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_defaulted: link %d receive state machine %s -> %s",
                  link->actor_port_number,
                  rcv_state_str (link->rcv_state), rcv_state_str (RCV_DEFAULTED));
    }
  link->rcv_state = RCV_DEFAULTED;
  lacp_update_default_selected (link);
  lacp_record_default (link);
  CLR_EXPIRED (link->actor_oper_port_state);
}

static void
rcv_current (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "rcv_current: link %d receive state machine %s -> %s",
                  link->actor_port_number,
                  rcv_state_str (link->rcv_state), rcv_state_str (RCV_CURRENT));
    }
  link->rcv_state = RCV_CURRENT;
  lacp_update_selected (link);
  lacp_update_ntt (link);
  lacp_record_pdu (link);
  if (link->current_while_timer)
    {
#ifdef HAVE_HA
      lacp_cal_delete_current_while_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->current_while_timer);
    }

  LACP_TIMER_ON (link->current_while_timer, lacp_current_while_timer_expiry, 
                 link, GET_LACP_TIMEOUT (link->actor_oper_port_state) 
                 ? SHORT_TIMEOUT_TIME 
                 : LONG_TIMEOUT_TIME);
#ifdef HAVE_HA
  lacp_cal_create_current_while_timer (link);
#endif /* HAVE_HA */

  CLR_EXPIRED (link->actor_oper_port_state);
}

/* Execute the Receive state machine transitions */

void
lacp_rcv_sm (struct lacp_link * link)
{
  if (begin() == PAL_TRUE)
    {
      rcv_initialize (link);
    }
  else if ((link->port_enabled == PAL_FALSE)
           && (link->port_moved == PAL_FALSE))
    {
      rcv_port_disabled (link);
    }
  else 
    {
      switch (link->rcv_state)
        {
        case RCV_PORT_DISABLED:
          {
            if (link->port_moved == PAL_TRUE)
              {
                rcv_initialize (link);
              }
            else if (link->port_enabled == PAL_TRUE)
              {
                if (link->lacp_enabled == PAL_TRUE)
                  {
                    rcv_expired (link);
                  }
                else
                  {
                    rcv_lacp_disabled (link);
                  }
              }
          }
          break;
        case RCV_LACP_DISABLED:
          /* Nothing to do. */
          break;
        case RCV_EXPIRED:
          {
            if (link->pdu != NULL)
              {
                rcv_current (link);
                link->pdu = NULL;
              }
            else if (link->current_while_timer_expired != PAL_FALSE)
              {
                rcv_defaulted (link);
              }
          }
          break;
        case RCV_DEFAULTED:
          {
            if (link->pdu != NULL)
              {
                rcv_current (link);
                link->pdu = NULL;
              }
          }
          break;
        case RCV_CURRENT:
          {
            if (link->current_while_timer_expired != PAL_FALSE)
              {
                rcv_expired (link);
                link->current_while_timer_expired = PAL_FALSE;
              }
            else if (link->pdu != NULL)
              {
                rcv_current (link);
                link->pdu = NULL;
              }
          }
          break;
        default:
          rcv_initialize (link);
          break;
        }
    }
}
