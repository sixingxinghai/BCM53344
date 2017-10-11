/* Copyright 2003,  All Rights Reserved. */

/* Churn detection state machine (Section 43.4.17) 
  Both figures 43-15 and 43-16 are contained here.  */

#include "pal.h"
#include "lib.h"

#include "lacp_types.h"
#include "lacp_timer.h"
#include "lacpd.h"
#include "lacp_link.h"
#include "lacp_churn.h"
#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

static char *
actor_churn_state_str (const enum lacp_actor_churn_state s)
{
  switch (s)
  {
    case ACTOR_CHURN_INVALID:
      return "Invalid";
    case ACTOR_CHURN_MONITOR:
      return "Monitor";
    case NO_ACTOR_CHURN:
      return "No churn";
    case ACTOR_CHURN:
      return "Churn";
    default:
      return "Unknown";
  }
}

/* Figure 43-15  ACTOR CHURN */

static void
actor_churn_monitor (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "actor_churn_monitor: link %d actor churn state %s -> %s",
                  link->actor_port_number,
                  actor_churn_state_str (link->actor_churn_state), 
                  actor_churn_state_str (ACTOR_CHURN_MONITOR));
    }

  if (link->actor_churn_timer != NULL)
    {
#ifdef HAVE_HA
      lacp_cal_delete_actor_churn_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->actor_churn_timer);
    }

  link->actor_churn_state = ACTOR_CHURN_MONITOR;
  link->actor_churn = PAL_FALSE;
  LACP_TIMER_ON (link->actor_churn_timer, lacp_actor_churn_timer_expiry,
                 (void *)link, ACTOR_CHURN_TIME);

#ifdef HAVE_HA
  lacp_cal_create_actor_churn_timer (link);
#endif /* HAVE_HA */

}

static void
actor_churn (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "actor_churn: link %d actor churn state %s -> %s",
          link->actor_port_number,
          actor_churn_state_str (link->actor_churn_state), 
          actor_churn_state_str (ACTOR_CHURN));
    }
  link->actor_churn_state = ACTOR_CHURN;
  link->actor_churn = PAL_TRUE;
  link->actor_churn_count++;
}

static void
no_actor_churn (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "no_actor_churn: link %d actor churn state %s -> %s",
          link->actor_port_number,
          actor_churn_state_str (link->actor_churn_state), 
          actor_churn_state_str (NO_ACTOR_CHURN));
    }
  link->actor_churn_state = NO_ACTOR_CHURN;
  link->actor_churn = PAL_FALSE;
}

static char *
partner_churn_state_str (const enum lacp_actor_churn_state s)
{
  switch (s)
  {
    case PARTNER_CHURN_INVALID:
      return "Invalid";
    case PARTNER_CHURN_MONITOR:
      return "Monitor";
    case NO_PARTNER_CHURN:
      return "No churn";
    case PARTNER_CHURN:
      return "Churn";
    default:
      return "Unknown";
  }
}

/* Figure 43-16  PARTNER CHURN */

static void
partner_churn_monitor (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "partner_churn_monitor: link %d partner churn state %s -> %s",
          link->actor_port_number,
          partner_churn_state_str (link->partner_churn_state), 
          partner_churn_state_str (PARTNER_CHURN_MONITOR));
    }
  link->partner_churn_state = PARTNER_CHURN_MONITOR;
  link->partner_churn = PAL_FALSE;
  if (link->partner_churn_timer != NULL)
    {
#ifdef HAVE_HA
      lacp_cal_delete_partner_churn_timer (link);
#endif /* HAVE_HA */

      LACP_TIMER_OFF (link->partner_churn_timer);
    }

  LACP_TIMER_ON (link->partner_churn_timer, lacp_partner_churn_timer_expiry,
                 (void *)link, PARTNER_CHURN_TIME);

#ifdef HAVE_HA
  lacp_cal_create_partner_churn_timer (link);
#endif /* HAVE_HA */
}

static void
partner_churn (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "partner_churn: link %d partner churn state %s -> %s",
          link->actor_port_number,
          partner_churn_state_str (link->partner_churn_state), 
          partner_churn_state_str (PARTNER_CHURN_MONITOR));
    }
  link->partner_churn_state = PARTNER_CHURN;
  link->partner_churn = PAL_TRUE;
  link->partner_churn_count++;
}

static void
no_partner_churn (struct lacp_link * link)
{
  if (LACP_DEBUG(EVENT))
    {
      zlog_debug (LACPM, "no_partner_churn: link %d partner churn state %s -> %s",
          link->actor_port_number,
          partner_churn_state_str (link->partner_churn_state), 
          partner_churn_state_str (NO_PARTNER_CHURN));
    }
  link->partner_churn_state = NO_PARTNER_CHURN;
  link->partner_churn = PAL_FALSE;
}

/* Execute the Churn state machine transitions */

void
lacp_churn_sm (struct lacp_link * link)
{
  if ((begin() != PAL_FALSE) || (link->port_enabled == PAL_FALSE))
    {
      actor_churn_monitor (link);
      partner_churn_monitor (link);
    }
  else if (begin() == PAL_FALSE) 
    {
      /* Actor state machine */
      switch (link->actor_churn_state)
        {
         case ACTOR_CHURN_MONITOR:
            {
              if (GET_SYNCHRONIZATION (link->actor_oper_port_state))
                {
                  no_actor_churn (link);
                }
              else if (link->actor_churn_timer_expired)
                {
                  actor_churn (link);
                }
            }
            break;
          case ACTOR_CHURN:
            {
              if (GET_SYNCHRONIZATION (link->actor_oper_port_state))
                {
                  no_actor_churn (link);
                }
            }
            break;
          case NO_ACTOR_CHURN:
            {
              if (GET_SYNCHRONIZATION (link->actor_oper_port_state) == PAL_FALSE)
                {
                  actor_churn_monitor (link);
                }
            }
            break;
          default:
            actor_churn_monitor (link);
            break;
        }

      /* Partner state machine */
      switch (link->partner_churn_state)
        {
         case ACTOR_CHURN_MONITOR:
            {
              if (GET_SYNCHRONIZATION (link->partner_oper_port_state))
                {
                  no_partner_churn (link);
                }
              else if (link->partner_churn_timer_expired)
                {
                  partner_churn (link);
                }
            }
            break;
          case ACTOR_CHURN:
            {
              if (GET_SYNCHRONIZATION (link->partner_oper_port_state))
                {
                  no_partner_churn (link);
                }
            }
            break;
          case NO_ACTOR_CHURN:
            {
              if (GET_SYNCHRONIZATION (link->partner_oper_port_state) == PAL_FALSE)
                {
                  partner_churn_monitor (link);
                }
            }
            break;
          default:
            partner_churn_monitor (link);
            break;
        }
    }
}
