/* Copyright (C) 2003  All Rights Reserved.
   
   This module declares the interface to the LACP generic timer
   helper routines. The underlying timers are provided by the pal.
 */

#ifndef _LACP_TIMER_H_
#define _LACP_TIMER_H_

#include "pal.h"
#include "pal_time.h"
#include "lib.h"
#include "thread.h"
#include "log.h"

#include "lacp_main.h"
#include "lacp_debug.h"

extern int lacp_current_while_timer_expiry (struct thread * t);

extern int lacp_actor_churn_timer_expiry (struct thread * t);

extern int lacp_partner_churn_timer_expiry (struct thread * t);

extern int lacp_actor_periodic_timer_expiry (struct thread * t);

extern int lacp_actor_wait_while_timer_expiry (struct thread * t);

extern int  lacp_tx_timer_expiry (struct thread * t);

#define LACP_TIMER_ON(T,F,S,V) \
  do { \
    if (! (T)) \
      (T) = thread_add_timer (LACPM, (F), (S), (V)); \
  } while (0)


#define LACP_THREAD_OFF(T) \
  do { \
    if (T) \
      { \
        thread_cancel (T); \
        (T) = NULL; \
      } \
  } while (0)

#define LACP_READ_OFF LACP_THREAD_OFF
#define LACP_TIMER_OFF LACP_THREAD_OFF

#endif /* _LACP_TIMER_H_ */
