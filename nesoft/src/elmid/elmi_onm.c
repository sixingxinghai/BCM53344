/**@file elmi_onm.c
 **@brief  This elmi_onm.c file contains initialization and callbacks
 ** for interaction with ONM server 
 ***/

/* Copyright (C) 2009  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "log.h"
#include "table.h"
#include "onm_client.h"
#include "onm_message.h"
#include "elmid.h"
#include "elmi_types.h"
#include "elmi_port.h"
#include "l2lib.h"

/* Service reply is received.  If client has special request to ONM,
* we should check this reply.  */
int
elmi_onm_recv_service (struct onm_api_msg_header *header,
                       void *arg, void *message)
{
  struct onm_client_handler *och = NULL;
  struct onm_client *oc = NULL;
  struct onm_msg_service *service = NULL;

  och = arg;
  oc = och->oc;
  service = message;

  /* ONM server assign client ID.  */
  onm_client_id_set (oc, service);

  if (oc->debug)
    onm_service_dump (oc->zg, service);

  /* Check protocol version. */
  if (service->version < ONM_PROTOCOL_VERSION_1)
    {
      if (oc->debug)
        zlog_err (oc->zg, "ONM: Server protocol version(%d) error",
                  service->version);

      return RESULT_ERROR;
    }

  /* Check ONM service bits.  */
  if (CHECK_FLAG (service->bits, oc->service.bits) != oc->service.bits)
    {
      if (oc->debug)
        zlog_err (oc->zg, "ONM: Service(0x%0000x) is not sufficient",
                  service->bits);

      return RESULT_ERROR;
    }

  /* Now client is up.  */
  och->up = PAL_TRUE;
  return RESULT_OK;
}

int
elmi_onm_recv_msg_done (struct onm_api_msg_header *header,
                        void *arg, void *message)
{
  struct onm_client *oc = NULL;
  struct lib_globals *zg = NULL;
  struct onm_client_handler *och = NULL;

  if (arg == NULL)
    return RESULT_ERROR;

  och = arg;
  if (!och || !och->oc)
    return RESULT_ERROR;

  oc = och->oc;
  if (!oc->zg)
    return RESULT_ERROR;

  zg = oc->zg;

  return RESULT_OK;
}

/* EVC Active/Partially Active/Inactive message from ONM.  */
static int
elmi_onm_recv_evc_state (struct onm_api_msg_header *header,
                         void *arg, void *message)
{
  struct onm_client *oc = NULL;
  struct lib_globals *zg = NULL;
  struct onm_client_handler *och = NULL;
  struct onm_msg_evc_status *msg = NULL;
  s_int32_t ret = 0;
  
  och = arg;
  if (!och || !och->oc)
    return RESULT_ERROR;

  oc = och->oc;
  msg = message;

  if (!oc->zg)
    return RESULT_ERROR;

  zg = oc->zg;

  if (oc->debug)
    onm_evc_dump (oc->zg, msg);

  /* add the evc to the evc_tree and start the async timer */
   ret = elmi_update_evc_status_type (msg->br_name, msg->evc_id, 
                                      msg->evc_status);

   return ret;
}

int
elmi_onm_disconnect_process (void)
{
  if (ZG && ZG->oc)
    onm_client_reconnect_start(ZG->oc);

  return RESULT_OK;
}


void
elmi_onm_init (struct lib_globals * zg)
{

  struct onm_client *oc = NULL;

  /* Create ONM client for ELMI */
  oc = onm_client_create (zg, 0);
  if (!oc)
    return;

  /* Set version and protocol.  */
  onm_client_set_version (oc, ONM_PROTOCOL_VERSION_1);
  onm_client_set_protocol (oc, APN_PROTO_ONMCLIENT);

   /* Set required services. */
  /* Interface might not be required check later */
  onm_client_set_service (oc, ONM_SERVICE_INTERFACE);
  onm_client_set_service (oc, ONM_SERVICE_EVC);
  onm_client_set_client_id (oc, APN_PROTO_ELMI);

  /* Register ONM Service-reply callbacks */
  onm_client_set_callback (oc, ONM_MSG_SERVICE_REPLY,
                           elmi_onm_recv_service);

  onm_client_set_callback (oc, ONM_MSG_EVC_STATUS, elmi_onm_recv_evc_state);

  /* Set disconnect handling callback */
  onm_client_set_disconnect_callback (oc, elmi_onm_disconnect_process);

  onm_client_start (oc);
}
