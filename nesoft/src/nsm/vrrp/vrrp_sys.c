/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "vty.h"
#include "thread.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/vrrp/vrrp.h"

/* Static function declarations */

static          int     vrrp_sys_init_timer ();
/* static               int     vrrp_sys_init_msgq(); */


/*------------------------------------------------------------------------
 * vrrp_sys_timer_up - called each time the vrrp timer fires.
 *------------------------------------------------------------------------*/

int     
vrrp_sys_timer_up (struct thread *t)
{
  struct vrrp_global *vrrp = THREAD_ARG (t);
    long interval = 1;

    /* Reset timer */
    vrrp->t_vrrp_timer = NULL;
    vrrp->t_vrrp_timer = thread_add_timer (nzg, vrrp_sys_timer_up,
                                           vrrp, interval);

    /* Call VRRP timer handler */
    vrrp_timer (vrrp);
    return VRRP_OK;
}


/*------------------------------------------------------------------------
 * vrrp_sys_init_timer - called by vrrp_sys_init to initthe VRRP timer
 *------------------------------------------------------------------------*/
static  int    
vrrp_sys_init_timer (struct vrrp_global *vrrp)
{
    long interval;

    /* Create VRRP timer */
    interval = 1;
    vrrp->t_vrrp_timer = thread_add_timer (nzg, vrrp_sys_timer_up,
                                           vrrp, interval);

    return VRRP_OK;
}

/*------------------------------------------------------------------------
 * vrrp_sys_init() - called by VRRP to initialize with the system.
 *              All initialization will be done in static functions.
 *------------------------------------------------------------------------*/
int     
vrrp_sys_init (struct vrrp_global *vrrp)
{
    int         status;

    /* First, setup the timer */
    status = vrrp_sys_init_timer (vrrp);

    return status;
}


/*------------------------------------------------------------------------
 * vrrpSysTxMsg() - called by VRRP to send messages to other components.
 *              For now, this will remain empty. 
 *------------------------------------------------------------------------*/
int     vrrpSysTxMsg ()
{
    return VRRP_OK;
}

/* 
 * --------------------------- 
 * APIs - called by the system
 * ---------------------------
 */

/*------------------------------------------------------------------------
 * vrrpSysRxMsg() - called whenever a message arrives in VRRP's message
 *              queue.  For now, it will remain empty.
 *------------------------------------------------------------------------*/
void    vrrpSysRxMsg()
{
}
