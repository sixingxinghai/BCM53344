/* Copyright (C)  All Rights Reserved. */

/* This module defines the various timer expiry handlers - 43.4.10.
  The strategy is that when a timer expires, the callback sets a flag and then
  runs the state machine. After the state machine has run, the callback
  clears the flag. */
  
#include "pal.h"
#include "pal_time.h"
#include "lib.h"
#include "thread.h"
#include "log.h"

#include "lacp_types.h"
#include "lacp_timer.h"
#include "lacpd.h"
#include "lacp_link.h"
#include "lacp_selection_logic.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

int
lacp_current_while_timer_expiry (struct thread *t)
{
  struct lacp_link *link = (struct lacp_link *) t ? t->arg : NULL;

  if (link)
    {
      if (LACP_DEBUG (TIMER))
        {
          zlog_debug (LACPM, "TIMER: current while timer expiry: link %d",
              link->actor_port_number);
        }

      link->current_while_timer = NULL;
      link->current_while_timer_expired = PAL_TRUE;
#ifdef HAVE_HA
      lacp_cal_delete_current_while_timer (link);
#endif /* HAVE_HA */
      lacp_run_sm (link);
      link->current_while_timer_expired = PAL_FALSE;
    }

  return RESULT_OK;
}


int
lacp_actor_wait_while_timer_expiry (struct thread * t)
{
  register struct lacp_link * link = (struct lacp_link *) t ? t->arg : NULL;

  if (link)
    {
      if (LACP_DEBUG (TIMER))
        {
          zlog_debug (LACPM, "TIMER: actor wait while timer expiry: link %d",
              link->actor_port_number);
        }

      link->wait_while_timer = NULL;
#ifdef HAVE_HA
      lacp_cal_delete_wait_while_timer (link);
#endif /* HAVE_HA */

      if (link->selected == SELECTED)
        {
          link->ready_n = PAL_TRUE;
          link->ntt = PAL_TRUE;
        }
      lacp_run_sm (link);
    }
  return RESULT_OK;
}

int
lacp_actor_churn_timer_expiry (struct thread * t)
{
  struct lacp_link * link = (struct lacp_link *) t ? t->arg : NULL;

  if (link)
    {
      if (LACP_DEBUG (TIMER))
        {
          zlog_debug (LACPM, "TIMER: actor churn timer expiry: link %d",
              link->actor_port_number);
        }

      link->actor_churn_timer = NULL;
      link->actor_churn_timer_expired = PAL_TRUE;
#ifdef HAVE_HA
      lacp_cal_delete_actor_churn_timer (link);
#endif /* HAVE_HA */

      lacp_run_sm (link);
      link->actor_churn_timer_expired = PAL_FALSE;
    }
  return RESULT_OK;
}


int
lacp_partner_churn_timer_expiry (struct thread * t)
{
  struct lacp_link * link = (struct lacp_link *) t ? t->arg : NULL;

  if (link)
    {
      if (LACP_DEBUG (TIMER))
        {
          zlog_debug (LACPM, "TIMER: partner churn timer expiry: link %d",
              link->actor_port_number);
        }

      link->partner_churn_timer = NULL;
      link->partner_churn_timer_expired = PAL_TRUE;
#ifdef HAVE_HA
      lacp_cal_delete_partner_churn_timer (link);
#endif /* HAVE_HA */

      lacp_run_sm (link);
      link->partner_churn_timer_expired = PAL_FALSE;
    }
  return RESULT_OK;
}


int
lacp_actor_periodic_timer_expiry (struct thread *t)
{
  struct lacp_link *link = (struct lacp_link *) t ? t->arg : NULL;

  if (link)
    {
      link->periodic_timer = NULL;
      link->periodic_timer_expired = PAL_TRUE;
#ifdef HAVE_HA
      lacp_cal_delete_periodic_timer (link);
#endif /* HAVE_HA */

      lacp_run_sm (link);
      link->periodic_timer_expired = PAL_FALSE;
    }

  return RESULT_OK;
}


/* The tx timer is different from the other timers.  There is only one
   in the system and it updates the tx credits for all links.  
   N.B. We also run the selection logic from this timer, mainly
   because it is convenient to do so. Note that the selection logic
   simply finds an aggregator for a link. */

int
lacp_tx_timer_expiry (struct thread * t)
{
  lacp_instance->tx_timer = NULL;
#ifdef HAVE_HA
  lacp_cal_delete_tx_timer ();
#endif /* HAVE_HA */

  lacp_update_tx_credits ();

  LACP_TIMER_ON (lacp_instance->tx_timer, lacp_tx_timer_expiry, 
                 0, FAST_PERIODIC_TIME);

#ifdef HAVE_HA
  lacp_cal_create_tx_timer ();
#endif /* HAVE_HA */

  lacp_selection_logic ();

#ifdef HAVE_HA
  lacp_cal_modify_instance ();
#endif /* HAVE_HA */

  return RESULT_OK;
}

