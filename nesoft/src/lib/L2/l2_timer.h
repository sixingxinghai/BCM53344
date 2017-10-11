/* Copyright 2003  All Rights Reserved.
   
   This module declares the interface to the generic timer
   helper routines. The underlying timers are provided by the pal.
 */

#ifndef _L2_TIMER_H_
#define _L2_TIMER_H_

#include "pal.h"
#include "pal_time.h"
#include "lib.h"
#include "thread.h"
#include "log.h"
#include "l2_debug.h"

#define L2_TIMER_SCALE_FACT 256

#define TICS_TO_MICROSECS(x)  (((x) & 0xff) * (1000000/L2_TIMER_SCALE_FACT))
#define MICROSECS_TO_TICS(x) ((x) / (1000000/L2_TIMER_SCALE_FACT))
#define TICS_TO_SECS(x)       ((x) / L2_TIMER_SCALE_FACT)
#define SECS_TO_TICS(x)       ((x) * L2_TIMER_SCALE_FACT)


static inline struct thread *
l2_start_timer (s_int32_t (*func) (struct thread *), void * arg, 
                                  int tics, struct lib_globals *zg)
{
  /* Prolly need to get current part of secs and put in tv */
  struct pal_timeval tv = { TICS_TO_SECS(tics), TICS_TO_MICROSECS(tics) };
  struct thread * t = thread_add_timer_timeval(zg, func, arg, tv);
  if (LAYER2_DEBUG (proto, TIMER_DETAIL))
    zlog_debug (zg, "TIME: create timer - timer (%p) arg(%p) secs(%d.%.06d)", 
            t, arg, TICS_TO_SECS(tics), TICS_TO_MICROSECS(tics));
  return t;
}

static inline int 
l2_is_timer_running (struct thread *t)
{
  return t && (t->type != THREAD_UNUSED);
}


static inline void 
l2_stop_timer (struct thread ** t)
{
  if (t && l2_is_timer_running(*t))
  {
    thread_cancel(*t);
    *t = 0;
  }
}

/* This function modifies the expiry time if the timer is running */
static inline void
l2_restart_timer (struct thread **t,int new_tics)
{
  s_int32_t (*func) (struct thread *);
  void *arg;
  struct lib_globals *zg;
  
  if ( !t )
    return;
  
  if (l2_is_timer_running(*t))
    {
      func = (*t)->func;
      arg = (*t)->arg;
      zg = (*t)->zg;

      l2_stop_timer (t);
      *t = l2_start_timer (func, arg, new_tics, zg);
    }
}

# define pal_timersub(a, b, result)                 \
  do {                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;           \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;            \
    if ((result)->tv_usec < 0) {                \
      --(result)->tv_sec;                 \
      (result)->tv_usec += 1000000;               \
    }                       \
  } while (0)

static inline int
l2_timer_get_remaining_tics (struct thread * t)
{
  struct pal_timeval  timer_now;
  struct pal_timeval  diff;
  if (t)
  {
    int tics;
    pal_time_tzcurrent (&timer_now, NULL);
    pal_timersub (&t->u.sands, &timer_now, &diff);
    tics = SECS_TO_TICS(diff.tv_sec) + MICROSECS_TO_TICS(diff.tv_usec);
    if (tics < 0)
      tics = 0;
    if (LAYER2_DEBUG (proto, TIMER))
        zlog_debug (t->zg,"tics remaining %ld t.usand %ld.%.06ld" 
            "now %ld.%.06ld",
        tics,
        t->u.sands.tv_sec,
        t->u.sands.tv_usec,
        timer_now.tv_sec,
        timer_now.tv_usec);
            
    return tics;
  }
  return 0;
}

static inline int
l2_timer_get_remaining_secs (struct thread * t)
{
  if (t && l2_is_timer_running(t))
  {
    return l2_timer_get_remaining_tics(t) / L2_TIMER_SCALE_FACT;
  }
  return 0;

}
#endif

