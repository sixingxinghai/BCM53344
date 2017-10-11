/**@file elmi_timer.h
 **@brief  This file contains prototypes for timer related functions.
 **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef __ELMI_TIMER_H__
#define __ELMI_TIMER_H__

#include "l2_timer.h"
int
elmi_polling_timer_handler(struct thread * t);
int
elmi_polling_verification_timer_handler(struct thread * t);

int
elmi_async_evc_timer_handler (struct thread * t);

int
elmi_nsm_send_operational_status (struct elmi_port *port,
                                  u_int8_t operational_state);

#endif /* __ELMI_TIMER_H__ */
