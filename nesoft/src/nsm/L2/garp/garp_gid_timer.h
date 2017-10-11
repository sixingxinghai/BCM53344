/* Copyright (C) 2003  All Rights Reserved. */
#include "pal.h"
#include "pal_time.h"
#include "lib.h"
#include "thread.h"
#include "log.h"

#define CENTISECS_TO_SECS(x) ((x)/(100))
#define CENTISECS_TO_MICROSECS(x) ((x)*10000)
#define SECS_TO_CENTISECS(x) ((x)*(100))
#define CENTISECS_TO_SUBSECS(x) \
     ( CENTISECS_TO_MICROSECS( (x) - (SECS_TO_CENTISECS(CENTISECS_TO_SECS(x)))))

/*
    FUNCTION : garp_is_timer_running 

    DESCRIPTION : Timer routine that gets executed when the registar's 
                  leave timer expires

    IN :
    struct thread *thread :

    OUT :


    CALLED BY :
 */
static inline s_int32_t 
garp_is_timer_running (struct thread *thread)
{
  return (thread && (thread->type != THREAD_UNUSED));
}


static inline  u_int32_t
garp_rand (u_int32_t min, u_int32_t max)
{
  u_int32_t range;
  u_int32_t rand_num;

  range = MAX(min, max) - MIN(min, max) + 1;
  rand_num = pal_rand() % range;

  return MIN(min, max) + rand_num;
}

/*
    FUNCTION : garp_start_random_timer 

    DESCRIPTION : Timer routine that gets executed when a the  
                  
    IN :
    s_int32_t (*func) (struct thread *thread) : timer handler routine that 
                                                needs to be executed when 
                                                the timer i expires between
                                                greater than 0 and less than
                                                or equal to timeout (random)
    void *arg : argument needed for the timer handler
    pal_time_t timeout : time before the timer expires

    OUT :
    struct thread * : 

    CALLED BY :
 */
extern struct thread *
garp_start_random_timer (s_int32_t (*func) (struct thread *thread), 
                         struct gid *gid, pal_time_t min_timeout, pal_time_t
                         max_timeout);


/*
    FUNCTION : garp_start_timer 

    DESCRIPTION : Timer routine that gets executed when the registar's 
                  leave timer expires

    IN :
    s_int32_t (*func) (struct thread *thread) :
    struct gid *gid :
    pal_time_t time :

    OUT :


    CALLED BY :
 */
extern struct thread *
garp_start_timer (s_int32_t (*func) (struct thread *t), struct gid *gid,
                  pal_time_t time);


/*
    FUNCTION : garp_stop_timer 

    DESCRIPTION : Timer routine that gets executed when the registar's 
                  leave timer expires

    IN :
    struct thread **thread :

    OUT :


    CALLED BY :
*/
extern void
garp_stop_timer (struct thread **thread);

/*
    FUNCTION : garp_schedule_now 

    DESCRIPTION : Timer routine that gets executed when the registar's 
                  leave timer expires

    IN :
    void (*func) (struct thread *thread) :
    void *arg :

    OUT :

    CALLED BY :
 */
extern struct thread *
garp_schedule_now (s_int32_t (*func) (struct thread *thread), struct gid *gid);



/*
    FUNCTION : gid_leave_timer_handler 

    DESCRIPTION : Timer routine that gets executed when the registar's 
                  leave timer expires

    IN :
    struct thread *thread

    OUT :
    

    CALLED BY :
 */
extern s_int32_t 
gid_leave_timer_handler (struct thread *thread);


/*
    FUNCTION : gid_join_timer_handler

    DESCRIPTION : Timer routine that gets executed when the applicant's
                  join timer expires

    IN :
    struct thread *thread
    
    OUT :
    

    CALLED BY :
 */
extern s_int32_t 
gid_join_timer_handler (struct thread *thread);


/*
    FUNCTION : gid_leave_all_timer_handler 

    DESCRIPTION : Timer routine that gets executed when the registar's 
                  leave_all timer expires

    IN :
    struct thread *thread

    OUT :

    CALLED BY :
 */
extern s_int32_t 
gid_leave_all_timer_handler (struct thread *thread);


/*
    FUNCTION : gid_hold_timer_handler 

    DESCRIPTION : Timer routine that gets executed when the hold timer expires

    IN :
    struct thread *thread

    OUT :
    

    CALLED BY :
 */
extern s_int32_t 
gid_hold_timer_handler (struct thread *thread);
