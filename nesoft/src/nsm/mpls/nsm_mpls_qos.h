/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _NSM_QOS_H_
#define _NSM_QOS_H_

void nsm_qos_send_max_bw_update (struct interface *);
void nsm_qos_send_max_resv_bw_update (struct interface *);
void nsm_qos_init_interface (struct interface *);
void nsm_qos_deinit_interface (struct interface *, bool_t, bool_t, bool_t);
void nsm_qos_preempt_resource (struct nsm_msg_qos_preempt *);
#ifdef HAVE_DSTE
void nsm_qos_send_bw_constraint_update (struct interface *);
#endif /* HAVE_DSTE */

#endif /* _NSM_QOS_H_ */
