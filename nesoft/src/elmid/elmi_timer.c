/**@file elmi_timer.c
 **@brief  This file contains the timer related functions.
 **/
/* Copyright (C) 2010  All Rights Reserved. */

#include "thread.h"
#include "log.h"
#include "l2lib.h"
#include "avl_tree.h"
#include "elmid.h"
#include "elmi_types.h"
#include "l2_timer.h" 
#include "elmi_bridge.h"
#include "elmi_debug.h"
#include "elmi_uni_c.h"
#include "elmi_uni_n.h"
#include "l2_debug.h"
#include "nsm_client.h"

#define DEC(x) { if(x !=0) x--; }

int
elmi_nsm_send_operational_status (struct elmi_port *port, 
                                  u_int8_t operational_state)
{
  struct nsm_msg_elmi_status msg;
  struct apn_vr *vr = apn_vr_get_privileged(ZG);
  int ret = RESULT_ERROR;

  if (!port)
    return ret;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_elmi_status));

  msg.ifindex = port->elmi_if->ifindex;
  msg.elmi_operational_status = operational_state;

  /* Sending the ELMI Operational Status to NSM */
  ret = nsm_client_send_elmi_status_msg (ZG->nc, vr->id, &msg);          

  return ret;

}

/* Polling Timer Handler */
int
elmi_polling_timer_handler (struct thread * t)
{
  struct elmi_port *port = t ? (struct elmi_port *)t->arg : NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  u_int8_t report_type = 0;
  
  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (ELMI_DEBUG (protocol, PROTOCOL))
    zlog_debug (ZG, "elmi_polling_timer_handler: "
                "expired for bridge %s index %u", port->elmi_if->br->name, 
                port->elmi_if->ifindex);

  if (port->current_polling_counter != 0)
    port->current_polling_counter--;

  if (port->rcvd_response != PAL_TRUE)
    {
      if (port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS
          || port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS_CONTD)
        port->stats.status_enq_timeouts++;

      if (ELMI_DEBUG (protocol, PROTOCOL))
        zlog_debug (ZG, "elmi_polling_timer_handler: "
                    "Response not received from UNI-N");

      port->current_status_counter++;

      if (port->last_request_type == ELMI_REPORT_TYPE_ECHECK)
        port->stats.status_enq_timeouts++;

      /* Reset recent success operations count to zero */
      port->rcnt_success_oper_cnt = ELMI_COUNTER_MIN;

      if (port->current_status_counter >= elmi_if->status_counter_limit)
        {  
          if (ELMI_DEBUG (protocol, PROTOCOL))
            zlog_debug (ZG, "elmi_polling_timer_handler: "
                        "UNIC STATUS COUNTER max reached");

          /* Reset status counter */
          port->current_status_counter = ELMI_COUNTER_MIN;

          /* change ELMI operational state */  
          if (port->elmi_operational_state!= ELMI_OPERATIONAL_STATE_DOWN)
            {
              port->elmi_operational_state = ELMI_OPERATIONAL_STATE_DOWN;
              if (ELMI_DEBUG (protocol, PROTOCOL))
                zlog_debug (ZG, "elmi_polling_timer_handler: "
                            "Send notification to NSM, state transition\n");
              /* send notification to NSM */
              elmi_nsm_send_operational_status (port, 
                                                port->elmi_operational_state);
            }

        }

      /* Resend the last request if it was full status or continued enq */
      if (port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS
          || port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS_CONTD 
          || port->current_polling_counter == ELMI_COUNTER_MIN) 
        {
          if (port->current_polling_counter == ELMI_COUNTER_MIN)
            port->current_polling_counter = elmi_if->polling_counter_limit;

          report_type = ELMI_REPORT_TYPE_FULL_STATUS;
        }
      else 
        report_type = ELMI_REPORT_TYPE_ECHECK;

    }
  else
    {
      /* Check polling counter here */
      if (port->current_polling_counter == ELMI_COUNTER_MIN)
        {
          if (ELMI_DEBUG (protocol, PROTOCOL))
            zlog_debug (ZG, "elmi_polling_timer_handler: "
                        "Polling counter fired send FULL STATUS Report");

          port->current_polling_counter = elmi_if->polling_counter_limit;
          report_type = ELMI_REPORT_TYPE_FULL_STATUS;
        }
      else  
        report_type = ELMI_REPORT_TYPE_ECHECK;
    }

  /* Send ELMI Frame from UNI-C */
  elmi_uni_c_tx (port, report_type);

  port->polling_timer = l2_start_timer (elmi_polling_timer_handler,
                                        (void *)port,
                                        SECS_TO_TICS(elmi_if->polling_time),
                                        ZG);

  return RESULT_OK;
}

/* Polling verification timer Handler */
int
elmi_polling_verification_timer_handler (struct thread * t)
{
  struct elmi_port *port = t ? (struct elmi_port *)t->arg : NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (ELMI_DEBUG (protocol, PROTOCOL))
    zlog_debug (ZG, "PVT: expired for bridge %s index %u", 
                port->elmi_if->br->name, 
                port->elmi_if->ifindex);

  if (port->rcvd_response != PAL_TRUE)
    {
      /* UNI-N did not receive any enquiry msg from UNI-C */
      port->stats.status_enq_timeouts++;

      port->current_status_counter++;

      /* Reset recent success operations count to zero */
      port->rcnt_success_oper_cnt = ELMI_COUNTER_MIN;
      if (port->current_status_counter >= elmi_if->status_counter_limit)
        {

          /* Change ELMI operational state */
          if (port->elmi_operational_state != ELMI_OPERATIONAL_STATE_DOWN)
            {
              port->elmi_operational_state = ELMI_OPERATIONAL_STATE_DOWN;
              /* send notification to NSM */
              if (ELMI_DEBUG (protocol, PROTOCOL))
                zlog_debug (ZG, "PVT: No response rcvd within last N393," 
                            "state transition:DOWN");

              /* Send notification to NSM */
              elmi_nsm_send_operational_status (port, 
                                                port->elmi_operational_state);
            }
        }
    }

  /* Restart PVT if it is enabled */ 
  if (elmi_if->enable_pvt == PAL_TRUE)
    {
      port->polling_verification_timer = 
        l2_start_timer (elmi_polling_verification_timer_handler,
                        (void *)port, 
                        SECS_TO_TICS(elmi_if->polling_verification_time), ZG);
    }

   return RESULT_OK;
}

int
elmi_async_evc_timer_handler (struct thread * t)
{
  struct elmi_port *port = t ? (struct elmi_port *)t->arg : NULL;
  struct elmi_evc_status *evc_node = NULL;
  struct listnode *list_node = NULL;
  struct elmi_master *em = NULL;
  struct elmi_ifp *elmi_if = NULL;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (ELMI_DEBUG (protocol, PROTOCOL))
    zlog_debug (ZG, "elmi_async_evc_timer_handler: "
                "expired for bridge %s index %u", port->elmi_if->br->name,
                port->elmi_if->ifindex);

  /* check if some traps are there in the list */
  if (port->async_evc_list)
    {
      /* get the oldest trap (head should be oldest ?)from the list */
      list_node = port->async_evc_list->head;
      if (!list_node)
         return RESULT_ERROR;

      evc_node = list_node->data;

      if (evc_node)
        {
          /* send the trap and remove the node from the list */
          elmi_unin_send_async_evc_msg (evc_node, port);
          listnode_delete (port->async_evc_list, evc_node);

          if (evc_node->active == PAL_FALSE)
            {
              listnode_delete(elmi_if->evc_status_list, (void *)evc_node);
              XFREE (MTYPE_ELMI_EVC, evc_node);
            }

        }

      /* If this is the last node in the list, stop the timer 
       * else restart the timer */
      if (LISTCOUNT (port->async_evc_list) == 0)
        {
          l2_stop_timer (&port->async_timer);
        }
       else
        l2_start_timer (elmi_async_evc_timer_handler,
                        (void *)port, 
                        SECS_TO_TICS(elmi_if->asynchronous_time), ZG);

    }

  return RESULT_OK;

}
