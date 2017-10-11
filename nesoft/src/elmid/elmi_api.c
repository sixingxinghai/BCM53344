/**@file elmi_api.c
 * @brief  This file contains User interface APIs and other general APIs.
 */
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "avl_tree.h"
#include "ptree.h"
#include "nsm_client.h"
#include "lib/L2/l2_timer.h"
#include "lib/L2/l2lib.h"
#include "elmid.h"
#include "elmi_api.h"
#include "elmi_sock.h"
#include "elmi_types.h"
#include "elmi_bridge.h"
#include "elmi_error.h"
#include "elmi_timer.h"
#include "elmi_uni_c.h"
#include "elmi_uni_n.h"
#include "elmi_port.h"

/**@brief This function is used to set polling timer value.
 @param ifName - Interface Name
 @param *polling_time - Value for Polling timer at UNI-C
 @return ELMI_SUCCESS
 **/
s_int32_t 
elmi_api_set_port_polling_time (u_int8_t *ifName,
                                u_int8_t polling_time)
{
  struct elmi_ifp *elmi_if = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct elmi_bridge *br = NULL;
  struct interface *ifp = NULL;

  /* Check for boundaries */
  if ((polling_time < ELMI_T391_POLLING_TIME_MIN)
      || (polling_time > ELMI_T391_POLLING_TIME_MAX))
    return ELMI_ERR_INTERVAL_OUTOFBOUNDS;

  if (!vr)
    return ELMI_ERR_INVALID_INTERFACE;

  if (!ifName)
    return ELMI_ERR_INVALID_INTERFACE;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return ELMI_ERR_INVALID_INTERFACE; 

  elmi_if = ifp->info;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  br = elmi_if->br;

  if (!br)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  if (elmi_if->uni_mode != ELMI_UNI_MODE_UNIC)
    return ELMI_ERR_NOT_ALLOWED_UNIN;

  /* If already set to the same val, ignore */
  if (elmi_if->polling_time == polling_time)
    return RESULT_OK;

  elmi_if->polling_time = polling_time;

  return RESULT_OK;

}

/**@brief This function is used to set polling verification timer value.
 @param ifName - Interface Name
 @param *polling_time - Value for Polling verification timer at UNI-N
 @return ELMI_SUCCESS
 **/
s_int32_t
elmi_api_set_port_polling_verification_time (u_int8_t *ifName,
                                            u_int8_t polling_verification_time)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_port *port = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct elmi_bridge *br = NULL;
  struct interface *ifp = NULL;

  /* zero is allowed to disable PVT */
   if (polling_verification_time != ELMI_T392_PVT_DISABLE)
     {
      if ((polling_verification_time < ELMI_T392_PVT_MIN)
          || (polling_verification_time > ELMI_T392_PVT_MAX))
        return ELMI_ERR_INTERVAL_OUTOFBOUNDS;
     }

  if (!vr)
    return ELMI_ERR_INVALID_INTERFACE;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return ELMI_ERR_INVALID_INTERFACE; 

  elmi_if = ifp->info;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  br = elmi_if->br;

  if (!br)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  /* Check Port type */
  if (elmi_if->uni_mode != ELMI_UNI_MODE_UNIN)
    return ELMI_ERR_NOT_ALLOWED_UNIC;

  if (elmi_if->polling_verification_time == polling_verification_time)
    return RESULT_OK;

  elmi_if->polling_verification_time = polling_verification_time;

  /* If PVT is to be disabled */
  if (polling_verification_time == ELMI_T392_PVT_DISABLE)
    {
      elmi_if->enable_pvt = PAL_FALSE;
      port = elmi_if->port; 
      if (port)
        {
          /* If PVT timer is running, stop it */
          if (l2_is_timer_running (port->polling_verification_timer))
            l2_stop_timer (&port->polling_verification_timer);

          /* If PVT is disabled, then elmi operational state will 
           * not be updated 
           */
          if (port->elmi_operational_state == PAL_FALSE)
            port->elmi_operational_state = PAL_TRUE;
        }
    }

  return RESULT_OK;
}

/**@brief This function is used to set polling counter value.
 @param ifName - Interface Name
 @param *polling_time - Value for Polling counter at UNI-C
 @return ELMI_SUCCESS
 **/
s_int32_t 
elmi_api_set_port_polling_counter (u_int8_t *ifName,
                                   u_int16_t polling_counter)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_port *port = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct elmi_bridge *br = NULL;
  struct interface *ifp = NULL;

   if (polling_counter < ELMI_N391_POLLING_COUNTER_MIN)
     return ELMI_ERR_INTERVAL_OUTOFBOUNDS;

  if (!vr)
    return ELMI_ERR_INVALID_INTERFACE;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return ELMI_ERR_INVALID_INTERFACE; 

  elmi_if = ifp->info;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  br = elmi_if->br;
  if (!br)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  if (elmi_if->uni_mode != ELMI_UNI_MODE_UNIC)
    return ELMI_ERR_NOT_ALLOWED_UNIN;

  if (elmi_if->polling_counter_limit == polling_counter)
    return RESULT_OK;

  elmi_if->polling_counter_limit = polling_counter;

  /* Check if the current polling counter needs to be reset */
  port = elmi_if->port; 
  if (port)
    {
      if (port->current_polling_counter > elmi_if->polling_counter_limit)
        port->current_polling_counter = elmi_if->polling_counter_limit;
    }

   return RESULT_OK;
}

/**@brief This function is used to set status counter value.
 @param ifName - Interface Name
 @param *polling_time - Value for status counter at UNI-C & UNI-N
 @return ELMI_SUCCESS
 **/
s_int32_t 
elmi_api_set_port_status_counter (u_int8_t *ifName,
                                  s_int8_t status_counter)
{

  struct elmi_ifp *elmi_if = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct elmi_bridge *br = NULL;
  struct interface *ifp = NULL;
  
if ((status_counter < ELMI_N393_STATUS_COUNTER_MIN)
       || (status_counter > ELMI_N393_STATUS_COUNTER_MAX))
    return ELMI_ERR_INTERVAL_OUTOFBOUNDS;

  if (!vr)
    return ELMI_ERR_INVALID_INTERFACE;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return ELMI_ERR_INVALID_INTERFACE; 

  elmi_if = ifp->info;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  br = elmi_if->br;
  if (!br)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  if ((elmi_if->uni_mode != ELMI_UNI_MODE_UNIC) 
      && (elmi_if->uni_mode != ELMI_UNI_MODE_UNIN))
    return ELMI_ERR_NO_VALID_CONFIG;

  if (elmi_if->status_counter_limit == status_counter)
    return RESULT_OK;

  elmi_if->status_counter_limit = status_counter;

  return RESULT_OK;
}


/**@brief This function is used to set polling timer value.
 @param ifName - Interface Name
 @param *polling_time - Value for Polling timer at UNI-C
 @return ELMI_SUCCESS
 **/
s_int32_t 
elmi_api_set_port_async_min_time (u_int8_t *ifName,
                                  u_int8_t async_time_interval)
{
  struct elmi_ifp *elmi_if = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct elmi_bridge *br = NULL;
  struct interface *ifp = NULL;
  
if ((async_time_interval < ELMI_ASYNC_TIME_INTERVAL_MIN)
       || (async_time_interval > ELMI_ASYNC_TIME_INTERVAL_MAX))
    return ELMI_ERR_INTERVAL_OUTOFBOUNDS;

  if (!vr)
    return ELMI_ERR_INVALID_INTERFACE;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return ELMI_ERR_INVALID_INTERFACE; 

  elmi_if = ifp->info;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  br = elmi_if->br;

  if (!br)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  if (elmi_if->uni_mode != ELMI_UNI_MODE_UNIN) 
    return ELMI_ERR_NOT_ALLOWED_UNIC;

  if (elmi_if->asynchronous_time == async_time_interval)
    return RESULT_OK;

  elmi_if->asynchronous_time = async_time_interval;

  return RESULT_OK;
}

/**@brief This function is used to enable ELMI on abridge.
 @param br_name - Bridge Name
 @return ELMI_SUCCESS
 **/
s_int32_t
elmi_api_enable_bridge_global (u_char *br_name)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_port *port = NULL;
  struct interface *ifp = NULL;
  struct avl_node *avl_port = NULL;
  struct elmi_bridge *bridge = NULL;

 /* For default bridge, the string should be zero */
  if (!br_name)
    return ELMI_ERR_GENERIC;

  bridge = elmi_find_bridge (br_name);
 
   if (!bridge)
     {
       return ELMI_ERR_BRIDGE_NOT_FOUND;
     }

   /* Elmi can not be enabled on provider core bridge */
  if ((bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_CE) && 
      (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE))
    return ELMI_ERR_WRONG_BRIDGE_TYPE;
  
  AVL_TREE_LOOP (bridge->port_tree, elmi_if, avl_port)
    {
      /* if the port is not part of bridge group, continue */
      if (!elmi_if)  
        continue;

      if ((ifp = elmi_if->ifp) == NULL)
        continue;

      port = elmi_if->port;

      /* elmi is already enabled on the port */
      if (elmi_if->elmi_enabled)
        continue;

      elmi_api_enable_port (ifp->ifindex);

    }

  bridge->elmi_enabled = PAL_TRUE;

  return RESULT_OK;
}

/**@brief This function is used to disable ELMI on a bridge.
 @param bridge_name - Bridge Name
 @return ELMI_SUCCESS
 **/
s_int32_t
elmi_api_disable_bridge_global (u_char *bridge_name)
{

  struct elmi_ifp *br_port = NULL;
  struct elmi_port *port = NULL;
  struct interface *ifp = NULL;
  struct avl_node *avl_port = NULL;
  struct elmi_bridge *bridge = NULL;

 /* For default bridge, the string should be zero */
  if (!bridge_name)
    return ELMI_ERR_GENERIC;

  bridge = elmi_find_bridge (bridge_name);

   if (!bridge)
     {
       return ELMI_ERR_BRIDGE_NOT_FOUND;
     }

  if (bridge->elmi_enabled != PAL_TRUE)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  AVL_TREE_LOOP (bridge->port_tree, br_port, avl_port)
    {

      /* if the port is not part of bridge group, continue */
      if (!br_port)
        continue;

      if ((ifp = br_port->ifp) == NULL)
        continue;

      port = br_port->port;

      /* elmi is not enabled on the port */
      if (!port || !port->elmi_if->elmi_enabled)
        continue;

     elmi_api_disable_port (ifp->ifindex);
  
    }

   bridge->elmi_enabled = PAL_FALSE;

  return RESULT_OK;
}

/**@brief This function is used to enable ELMI on a interface.
 @param ifindex - Interface index
 @return ELMI_SUCCESS
 **/
s_int32_t
elmi_api_enable_port (u_int32_t ifindex)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_bridge *bridge = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct elmi_port *port = NULL; 
  s_int32_t ret = 0;

  ifp = if_lookup_by_index (&vr->ifm, ifindex);

  if (!ifp || !ifp->info)
    return ELMI_ERR_PORT_NOT_FOUND;

  elmi_if = ifp->info;

  bridge = elmi_if->br;
  if (!bridge)
    return ELMI_ERR_BRIDGE_NOT_FOUND; 

  /* check the port mode UNI-C or UNI-N */
  if ((bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_CE) && 
      (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE))
    return ELMI_ERR_WRONG_BRIDGE_TYPE;
  
  if (elmi_if->elmi_enabled == PAL_TRUE)  
    {
      return ELMI_ERR_ALREADY_ELMI_ENABLED;
    }

  if (elmi_if->uni_mode == ELMI_UNI_MODE_INVALID ||
     elmi_if->uni_mode == ELMI_DEFAULT_VAL)
    return ELMI_ERR_WRONG_PORT_TYPE;

  port = XCALLOC (MTYPE_ELMI_PORT, sizeof (struct elmi_port));
  if (port == NULL)
    {
      zlog_err (ZG, "Could not allocate memory for ELMI Interface\n");
      return RESULT_ERROR;
    }

  elmi_if->port = port;

  port->elmi_if = elmi_if;
  elmi_if->elmi_enabled = PAL_TRUE;

  /* Set send sequence counter to zero */
  port->sent_seq_num = ELMI_DEFAULT_VAL;
  port->recvd_send_seq_num = ELMI_DEFAULT_VAL;
  port->continued_msg = PAL_FALSE;
  port->rcvd_response = PAL_FALSE;
  port->last_evc_id = ELMI_DEFAULT_VAL;
  port->elmi_operational_state = ELMI_OPERATIONAL_STATE_UP;
  port->last_request_type = ELMI_INVALID_REPORT_TYPE;
  port->async_evc_list = NULL;
  
  pal_mem_set(&port->stats, 0, sizeof (struct elmi_counters));

  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
    {

      port->current_polling_counter = elmi_if->polling_counter_limit;

      /* Allocate memory for UNI-Info */ 
      elmi_if->uni_info = XCALLOC (MTYPE_ELMI_UNI, 
                                   sizeof (struct elmi_uni_info));
      if (elmi_if->uni_info == NULL)
        {
          zlog_err (ZG, "Could not allocate memory for UNI info\n");
          return RESULT_ERROR;
        }

      /* According to Section 5.6.7.1, When the UNI-C comes up for first time,
       * UNI-C MUST send the ELMI STATUS ENQUIRY with report type Full Status.
       * Set the data instance field to zero.
       */
      port->data_instance = ELMI_INVALID_VAL;

      port->request_sent = PAL_FALSE;

      if (l2_is_timer_running (port->polling_timer))
        l2_stop_timer (&port->polling_timer);

      /* After start-up, send a full status enquiry msg */
      elmi_uni_c_tx (port, ELMI_REPORT_TYPE_FULL_STATUS);

      port->polling_timer = l2_start_timer (elmi_polling_timer_handler, port,
                                            SECS_TO_TICS(elmi_if->polling_time), 
                                            elmi_zg);
    }
  else /* (port->uni_mode == ELMI_UNI_MODE_UNIN) */
    {

     /* create a list to maintain evc traps info */ 
      port->async_evc_list = list_new ();

      /* Set data instance field to 1 */
      port->data_instance = ELMI_UNIN_DATA_INSTANCE_MIN;

      /* Start PVT if it is enabled */
      if (elmi_if->enable_pvt == PAL_TRUE)
        {
          /* start the PVT timer */
          if (l2_is_timer_running (port->polling_verification_timer))
            l2_stop_timer (&port->polling_verification_timer);

          port->polling_verification_timer = 
                        l2_start_timer (elmi_polling_verification_timer_handler,
                        port,
                        SECS_TO_TICS(elmi_if->polling_verification_time),
                        elmi_zg);
        }
    }

/* Send status message to NSM at start-up */
  elmi_nsm_send_operational_status (port, port->elmi_operational_state); 

  return ret;
}

/**@brief This function is used to disable ELMI on a interface.
 @param ifindex - Interface index
 @return ELMI_SUCCESS
 **/
s_int32_t
elmi_api_disable_port (u_int32_t ifindex)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);

  ifp = if_lookup_by_index (&vr->ifm, ifindex);

  if (!ifp || !ifp->info)
    return ELMI_ERR_PORT_NOT_FOUND;

  elmi_if = ifp->info;

  bridge = elmi_find_bridge (ifp->bridge_name);
  if (!bridge)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  port = elmi_if->port;
  if (!port)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  /* Check uni mode and Stop all timers */
  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
    {
      if (l2_is_timer_running (port->polling_timer))
        {
          l2_stop_timer (&port->polling_timer);
          port->polling_timer = NULL;
        }

      elmi_unic_clear_learnt_database (elmi_if);
    }

  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIN)
    {
      if (l2_is_timer_running (port->polling_verification_timer))
        {
          l2_stop_timer (&port->polling_verification_timer);
          port->polling_verification_timer = NULL;
        }
    }
  /*Sending e-lmi operational status msg to NSM */
  elmi_nsm_send_operational_status (elmi_if->port,
      ELMI_OPERATIONAL_STATE_DOWN);
      
  XFREE (MTYPE_ELMI_PORT, elmi_if->port);
  elmi_if->port = NULL;
  elmi_if->elmi_enabled = PAL_FALSE;

  return RESULT_OK;
}

/* convert cfm status to status type required for ELMI */
u_int8_t
elmi_get_local_evc_status (u_int8_t evc_status_type)
{
  u_int8_t local_evc_status = 0;

  if (evc_status_type == EVC_CFM_STATUS_ACTIVE)
    local_evc_status = EVC_STATUS_ACTIVE;
  else if (evc_status_type == EVC_CFM_STATUS_PARTIALLY_ACTIVE)
    local_evc_status = EVC_STATUS_PARTIALLY_ACTIVE;
  else
    local_evc_status = EVC_STATUS_NOT_ACTIVE;

  return local_evc_status;

}

/* convert cfm status to status type required for ELMI */
enum cvlan_evc_map_type
elmi_get_map_type (enum elmi_nsm_uni_service_attr map_service_attr)
{

  switch (map_service_attr)
    {
    case ELMI_NSM_UNI_BNDL:
      return ELMI_BUNDLING;
    case ELMI_NSM_UNI_AIO_BNDL:
      return ELMI_ALL_TO_ONE_BUNDLING;
    case ELMI_NSM_UNI_MUX:
      return ELMI_SERVICE_MULTIPLEXING;
    default:
      return ELMI_BUNDLING_INVALID;
    }

  return ELMI_BUNDLING_INVALID;
}

/* Get EVC info by evc reference Id. */
struct elmi_evc_status *
elmi_evc_look_up (struct elmi_ifp *elmi_if, u_int16_t evc_ref_id)
{
  struct listnode *node = NULL;
  struct elmi_evc_status *evc = NULL;

  /* Make sure elmi_if is initialized and evcs present. */
  if ((!elmi_if) || (!elmi_if->evc_status_list))
    return NULL;

  if (elmi_if->evc_status_list && (listcount (elmi_if->evc_status_list) > 0))
    LIST_LOOP(elmi_if->evc_status_list, evc, node)
      {
        if (evc)
          {
            if (evc->evc_ref_id == evc_ref_id)
              return evc;
          }
         else
           continue;
      }

  return NULL;
}

struct elmi_evc_status *
elmi_lookup_evc_by_name (struct elmi_ifp *elmi_if, u_int8_t *evc_name)
{
  struct listnode *node = NULL;
  struct elmi_evc_status *evc_info = NULL;

  if (!elmi_if)
    return NULL;

  if (elmi_if->evc_status_list == NULL)
    return NULL;

  LIST_LOOP (elmi_if->evc_status_list, evc_info, node)
    {
      if (pal_strncmp (evc_info->evc_id, evc_name, ELMI_EVC_NAME_LEN) == 0)
        return evc_info;
    }

  return NULL;

}

struct elmi_cvlan_evc_map *
elmi_lookup_cevlan_evc_map (struct elmi_ifp *elmi_if, u_int16_t evc_ref_id)
{
  struct listnode *node = NULL;
  struct elmi_cvlan_evc_map *cevlan_evc = NULL;

  /* Make sure elmi_if is initialized and evcs present. */
  if ((!elmi_if) || (!elmi_if->cevlan_evc_map_list))
    return NULL;

  if (elmi_if->cevlan_evc_map_list)
  LIST_LOOP (elmi_if->cevlan_evc_map_list, cevlan_evc, node)
    {
      if (cevlan_evc->evc_ref_id == evc_ref_id)
        return cevlan_evc;
    }

  return NULL;

}

void
elmi_get_matissa_exponent_16bit (u_int32_t num,
                                 u_int16_t *multiplier,
                                 u_int8_t *magnitude)
{
  s_int8_t i = 0;

  if (num < ELMI_UINT16_MAXFRACT)
    {
      *magnitude = 0;
      *multiplier =  num;
    }
  else
    {
      for (i = 0; i < ELMI_MAX_EXPSIZE && num > ELMI_UINT16_MAXFRACT; i++)
         num  = num /ELMI_BIT_BASE;

      *magnitude = i;
      *multiplier = num;
    }

  return;
}

void
elmi_get_matissa_exponent_8bit (u_int32_t num,
                                 u_int8_t *multiplier,
                                 u_int8_t *magnitude)
{
  s_int8_t i = 0;

  if (num < ELMI_UINT8_MAXFRACT)
    {
      *magnitude = 0;
      *multiplier =  num;
    }
  else
    {
      for (i = 0; i < ELMI_MAX_EXPSIZE && num > ELMI_UINT8_MAXFRACT; i++)
         num  = num /ELMI_BIT_BASE;

      *magnitude = i;
      *multiplier = num;
    }

  return;
}
