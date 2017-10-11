/**@file elmi_nsm.c
 * @brief  This file contains the callbacks for ELMI NSM client.
 **/

/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "lib/L2/l2lib.h"
#include "avl_tree.h"
#include "thread.h"
#include "log.h"
#include "table.h"
#include "nsm_client.h"
#include "nsm_message.h"
#include "imi_client.h"
#include "l2_debug.h"
#include "elmi_types.h"
#include "elmi_bridge.h"
#include "elmi_port.h"
#include "elmi_api.h"
#include "elmid.h"
#include "elmi_debug.h"

/* Turn on debug out put.  */
void
elmi_nsm_debug_set (struct elmi_master *em)
{
  if (em->zg->nc)
    em->zg->nc->debug = 1;
}

/* Turn off debug output.  */
void
elmi_nsm_debug_unset (struct elmi_master *em)
{
  if (em->zg->nc)
    em->zg->nc->debug = 0;
}

s_int32_t
elmi_nsm_recv_service (struct nsm_msg_header *header,
                       void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = NULL;
  struct nsm_msg_service *service = message;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;
   
  nc = nch->nc;

  if (!nc->zg)
    return RESULT_ERROR;

  if (!service)
    return RESULT_ERROR;

  /* NSM server assign client ID.  */
  nsm_client_id_set (nc, service);

  if (nc->debug)
    nsm_service_dump (nc->zg, service);

  /* Check protocol version. */
  if (service->version < NSM_PROTOCOL_VERSION_1)
    {
      if (nc->debug)
        zlog_err (nc->zg, "NSM: Server protocol version(%d) error",
                  service->version);

      return RESULT_OK;
    }

  /* Check NSM service bits.  */
  if (CHECK_FLAG (service->bits, nc->service.bits) != nc->service.bits)
    {
      if (nc->debug)
        zlog_err (nc->zg, "NSM: Service(0x%0000x) is not sufficient",
                  service->bits);

      return RESULT_OK;
    }

  /* Now client is up.  */
  nch->up = PAL_TRUE;
  return RESULT_OK;
}

s_int32_t
elmi_nsm_recv_msg_done (struct nsm_msg_header *header,
                        void *arg, void *message)
{
  struct nsm_client *nc = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client_handler *nch = NULL;

  if (arg == NULL)
    return RESULT_ERROR;

  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;

  zg = nc->zg;
  if (!zg)
    return RESULT_ERROR;

#ifdef HAVE_IMI
  /* Calling the imi to send the elmi configuration again */
  imi_client_send_config_request(zg->vr_in_cxt);
#endif /* HAVE_IMI */

  return RESULT_OK;
}

/* Interface update message from NSM.  */
s_int32_t
elmi_interface_new_hook (struct interface *ifp)
{
  ifp->info = elmi_interface_new (ifp);

  return RESULT_OK;
}

s_int32_t
elmi_interface_delete_hook (struct interface *ifp)
{
  struct elmi_ifp *elmi_if = NULL;

  if (!ifp || !ifp->info)
    return RESULT_ERROR;

  elmi_if = ifp->info;

  elmi_delete_port (elmi_if);

  XFREE (MTYPE_ELMI_INTERFACE, elmi_if);

  /* Set pointer to NULL */
  ifp->info = NULL;

  return RESULT_OK;
}

static int
elmi_nsm_recv_interface_update (struct interface *ifp)
{
  elmi_interface_add (ifp);
  return RESULT_OK;
}

/* Interface delete message from NSM.  */
static int
elmi_nsm_recv_interface_delete (struct interface *ifp)
{
  return RESULT_OK;
}

/* Interface up/down message from NSM.  */
static int
elmi_nsm_recv_interface_state (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_client * nc = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_link *msg = NULL;
  struct interface * ifp = NULL;
  struct elmi_master *em = NULL;
  s_int32_t up = 0;
  s_int32_t changed = 0;

  msg = message;
  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;

  zg = nc->zg;
  if (!zg)
    return RESULT_ERROR;

  /* Look up interface so that check current status.  */
  ifp = ifg_lookup_by_index (&zg->ifg, msg->ifindex);
  if (!ifp)
    return RESULT_ERROR;

  /* Drop the message for unbound interfaces.  */
  if (ifp->vr == NULL)
    return RESULT_ERROR;

  em = ifp->vr->proto;

  /* Record the interface is up or not.  */
  up = if_is_up (ifp);

  /* Set new value to interface structure.  */
  nsm_util_interface_state (msg, &zg->ifg);

  changed = (if_is_up (ifp) != up);

  if (ELMI_DEBUG(protocol, PROTOCOL))
    {
      zlog_debug (ZG,
                  "Interface state update for port %s - %s",
                  ifp->name,
                  (header->type == NSM_MSG_LINK_UP) ? "up" : "down");
    }

  return RESULT_OK;
}

#ifdef HAVE_VLAN
static int
elmi_nsm_recv_vlan_add_port (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  s_int32_t i;
  s_int32_t ret = RESULT_ERROR;
  u_int16_t vid = 0;
  struct nsm_client *nc = NULL;
  struct lib_globals *zg = NULL;
  struct elmi_vlan_bmp *bmp = NULL;
  struct nsm_msg_vlan_port *msg = NULL;
  struct nsm_client_handler *nch = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;

  msg = message;
  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_OK;

  nc = nch->nc;

  zg = nc->zg;
  if (!zg)
    return RESULT_OK;

  bmp = elmi_vlan_port_map (msg->ifindex);

  if (bmp == NULL)
    {
      goto exit;
    }

  if (msg->vid_info == NULL)
    goto exit;

  for (i = 0; i < msg->num; i++)
    {
      vid = msg->vid_info[i] & NSM_VLAN_VID_MASK;
      ELMI_VLAN_BMP_SET (*bmp, vid);
    }

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    goto exit;

  if ((elmi_if = ifp->info) == NULL)
    goto exit;

  if (elmi_if->pvid != 0)
    {
      /* Seems already there is a default vlan id for this port */
      goto exit;
    }

  /* If there is a list of vlan in the message then at the 0th offset 
   * the vid will be pvid */
  vid = msg->vid_info[0] & NSM_VLAN_VID_MASK;

  elmi_if->pvid = vid;

exit:
  return ret;

}

static int
elmi_nsm_recv_vlan_delete_port (struct nsm_msg_header *header,
                                void *arg, void *message)
{
  u_int32_t i;
  u_int16_t vid = 0;
  struct nsm_client *nc = NULL;
  struct lib_globals *zg = NULL;
  struct elmi_vlan_bmp *bmp = NULL;
  struct nsm_msg_vlan_port *msg = NULL;
  struct nsm_client_handler *nch = NULL;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_OK;

  nc = nch->nc;

  zg = nc->zg;
  if (!zg)
    return RESULT_OK;

  bmp = elmi_vlan_port_map (msg->ifindex);

  if (bmp == NULL)
    return RESULT_OK;

  for (i = 0; i < msg->num; i++)
    {
      vid = msg->vid_info[i] & NSM_VLAN_VID_MASK;

      ELMI_VLAN_BMP_UNSET (*bmp, vid);
    }

  return RESULT_OK;
}

#endif /* HAVE_VLAN */

static int
elmi_nsm_recv_bridge_add (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bridge *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_ERROR;
  nc = nch->nc;
  zg = nc->zg;
  if (!zg)
    return RESULT_ERROR;

  ret = elmi_bridge_add (msg->bridge_name, msg->type, msg->is_edge);

  return ret;
}

static int
elmi_nsm_recv_bridge_delete (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bridge *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_ERROR;
  nc = nch->nc;
  zg = nc->zg;
  if (!zg)
    return RESULT_ERROR;

  ret = elmi_bridge_delete (msg->bridge_name);

  return ret;
}

static int
elmi_nsm_recv_bridge_add_if (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bridge_if *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_vlan_bmp *bmp = NULL;
  s_int32_t ret = 0;
  u_int32_t i;

  msg = message;
  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MSG_BRIDGE_CTYPE_PORT))
    {
      for (i = 0; i < msg->num; i++)
        {
          if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex[i])))
            {
              /* Add bridge port. */
              ret = elmi_bridge_add_port (msg->bridge_name, ifp);

              bmp = elmi_vlan_port_map (msg->ifindex[i]);

              if (bmp)
                ELMI_VLAN_BMP_SET (*bmp, L2_VLAN_DEFAULT_VID);
            }
        }
    }

  if (msg->ifindex)
    XFREE (MTYPE_TMP, msg->ifindex);

  return ret;
}

static int
elmi_nsm_recv_bridge_delete_if (struct nsm_msg_header *header,
                                void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bridge_if *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  s_int32_t ret = 0;
  u_int32_t i;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_MSG_BRIDGE_CTYPE_PORT))
    {
      for (i = 0; i < msg->num; i++)
        {
          if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex[i])))
            {
              /* Delete bridge port. */
              ret = elmi_bridge_delete_port (msg->bridge_name, ifp);
            }
        }
    }

  if (msg->ifindex)
    XFREE (MTYPE_TMP, msg->ifindex);

  return ret;
}

#ifdef HAVE_VLAN
static int
elmi_nsm_recv_bridge_add_vlan (struct nsm_msg_header *header,
                               void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_vlan *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  u_int32_t state;
  s_int32_t ret;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_OK;

  if (msg->flags & NSM_MSG_VLAN_STATE_ACTIVE)
    state = ELMI_ACTIVE;
  else
    state = ELMI_INACTIVE;

  /* Add VLAN. */
  ret = elmi_bridge_vlan_add (msg->bridge_name, msg->vid,
                              msg->vlan_name, state);

  return ret;
}

static int
elmi_nsm_recv_bridge_delete_vlan (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_vlan *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_OK;

  nc = nch->nc;
  zg = nc->zg;
  if (!zg)
    return RESULT_OK;

  /* Delete VLAN. */
  ret = elmi_bridge_vlan_delete (msg->bridge_name, msg->vid);

  return ret;
}


s_int32_t
elmi_nsm_recv_evc_add (struct nsm_msg_header *header,
                       void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_elmi_evc *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;
  struct elmi_master *em = NULL; 

  msg = message;
  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_ERROR;
  nc = nch->nc;
  zg = nc->zg;
  if (!zg)
    return RESULT_ERROR;

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_OK;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_OK;

  if (!elmi_if->br || !elmi_if->elmim)
    return RESULT_OK;

  em = elmi_if->elmim;

  ret = elmi_nsm_evc_add (elmi_if, msg);

  if (ELMI_DEBUG (nsm, NSM))
    zlog_info (ZG, "ELMI: evc added successfully");

  return RESULT_OK;

}

s_int32_t
elmi_nsm_recv_evc_delete (struct nsm_msg_header *header,
                          void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_elmi_evc *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;
  if (!nch || !nch->nc) 
    return RESULT_ERROR;
  nc = nch->nc;
  zg = nc->zg;
  if (!zg)
    return RESULT_ERROR;

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_evc_delete (elmi_if, msg);
  return ret;

}

s_int32_t
elmi_nsm_recv_evc_update (struct nsm_msg_header *header,
                          void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_elmi_evc *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_evc_update (elmi_if, msg);
  return ret;
}

s_int32_t
elmi_nsm_recv_uni_add (struct nsm_msg_header *header,
                       void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_uni *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_uni_add (elmi_if, msg);
  return ret;

}

s_int32_t
elmi_nsm_recv_uni_delete (struct nsm_msg_header *header,
                          void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_uni *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_uni_delete (elmi_if);
  return ret;

}

s_int32_t
elmi_nsm_recv_uni_update (struct nsm_msg_header *header,
                          void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_uni *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_uni_update (elmi_if, msg);

  return ret;

}

static s_int32_t
elmi_nsm_recv_vlan_port_type (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct interface *ifp = NULL;
  struct nsm_client * nc = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client_handler * nch = NULL;
  struct nsm_msg_vlan_port_type *msg = NULL;
  char bridge_name [L2_BRIDGE_NAME_LEN+1];
  struct elmi_bridge *br = NULL;

  msg = message;
  nch = arg;
  nc = nch->nc;
  zg = nc->zg;

  pal_mem_set (bridge_name, 0, L2_BRIDGE_NAME_LEN + 1);

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)))
    {
      elmi_if = ifp->info;

      if (!elmi_if)
        return RESULT_ERROR;

      br = elmi_if->br;
      if (!br)
        return RESULT_ERROR;

      if (elmi_if->port_type == msg->port_type)
        return RESULT_OK;

      if (elmi_if->port_type == ELMI_VLAN_PORT_MODE_CE)
         
        {
          /* Disable elmi if it is enabled previously on CE port */
          if (elmi_if->elmi_enabled)
            {  
              elmi_api_disable_port (elmi_if->ifindex);

              elmi_if->enable_pvt = 0;
              elmi_if->polling_verification_time = 0;
              elmi_if->asynchronous_time = 0;
              elmi_if->polling_counter_limit = 0; 
            }
        }

      if (msg->port_type == ELMI_VLAN_PORT_MODE_CE)
        {
          /* enable elmi if elmi is enabled on bridge */
          elmi_if->uni_mode = ELMI_UNI_MODE_UNIN;

          /* Initialize the timers */
          elmi_if->enable_pvt = PAL_TRUE;
          elmi_if->polling_verification_time = ELMI_T392_DEFAULT;
          elmi_if->asynchronous_time = ELMI_ASYNC_TIME_INTERVAL_DEF;
          elmi_if->status_counter_limit = ELMI_N393_STATUS_COUNTER_DEF; 

          if (elmi_if->br->elmi_enabled)
            elmi_api_enable_port (elmi_if->ifindex);
        }
      else if ((msg->port_type == ELMI_VLAN_PORT_MODE_ACCESS) ||
          (msg->port_type == ELMI_VLAN_PORT_MODE_TRUNK) || 
          (msg->port_type == ELMI_VLAN_PORT_MODE_HYBRID))
        {
          elmi_if->uni_mode = ELMI_UNI_MODE_UNIC;

          /* Initialize all the elmi related parameters with default values */
          elmi_if->polling_time = ELMI_T391_POLLING_TIME_DEF;
          elmi_if->polling_counter_limit = ELMI_N391_POLLING_COUNTER_DEF; 
          elmi_if->status_counter_limit = ELMI_N393_STATUS_COUNTER_DEF; 

          if (elmi_if->br->elmi_enabled)
            elmi_api_enable_port (elmi_if->ifindex);
        }
      else
        {
          elmi_if->uni_mode = ELMI_UNI_MODE_INVALID;
        }

      elmi_if->port_type = msg->port_type;

    }

  return RESULT_OK;
}

#endif /* HAVE_VLAN */

s_int32_t
elmi_nsm_recv_uni_bw_add (struct nsm_msg_header *header,
                          void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bw_profile *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (nc->debug)
    nsm_elmi_bw_dump (nc->zg, msg);

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_uni_bw_add (elmi_if, msg);

  return ret;

}

s_int32_t
elmi_nsm_recv_uni_bw_del (struct nsm_msg_header *header,
                          void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bw_profile *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (nc->debug)
    nsm_elmi_bw_dump (nc->zg, msg);

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_uni_bw_del (elmi_if, msg);

  return ret;

}

s_int32_t
elmi_nsm_recv_evc_bw_add (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bw_profile *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (nc->debug)
    nsm_elmi_bw_dump (nc->zg, msg);

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_evc_bw_add (elmi_if, msg);

  return ret;

}

s_int32_t
elmi_nsm_recv_evc_bw_del (struct nsm_msg_header *header,
                          void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bw_profile *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (nc->debug)
    nsm_elmi_bw_dump (nc->zg, msg);

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_evc_bw_del (elmi_if, msg);

  return ret;
}

s_int32_t
elmi_nsm_recv_evc_cos_bw_add (struct nsm_msg_header *header,
                              void *arg, void *message)
{

  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bw_profile *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (nc->debug)
    nsm_elmi_bw_dump (nc->zg, msg); 

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_evc_cos_bw_add (elmi_if, msg);

  return ret;

}

s_int32_t
elmi_nsm_recv_evc_cos_bw_del (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_client_handler *nch = NULL;
  struct nsm_msg_bw_profile *msg = NULL;
  struct lib_globals *zg = NULL;
  struct nsm_client *nc = NULL;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  msg = message;
  nch = arg;

  if (!nch || !nch->nc) 
    return RESULT_ERROR;

  nc = nch->nc;
  zg = nc->zg;

  if (!zg)
    return RESULT_ERROR;

  if (nc->debug)
    nsm_elmi_bw_dump (nc->zg, msg);

  if ((ifp = ifg_lookup_by_index(&zg->ifg, msg->ifindex)) == NULL)
    return RESULT_ERROR;

  if ((elmi_if = ifp->info) == NULL)
    return RESULT_ERROR;

  if (elmi_if->br == NULL)
    return RESULT_ERROR;

  ret = elmi_nsm_evc_cos_bw_del (elmi_if, msg);

  return ret;
}

s_int32_t
elmi_nsm_disconnect_process (void)
{

  /* Try to reconnect */
  if (ZG && ZG->nc)
    nsm_client_reconnect_start(ZG->nc);

  return RESULT_OK;
}

void
elmi_nsm_init (struct lib_globals * zg)
{
  struct nsm_client *nc = NULL;

  nc = nsm_client_create (zg, 0);
  if (!nc)
    return;

  /* Set version and protocol.  */
  nsm_client_set_version (nc, NSM_PROTOCOL_VERSION_1);
  nsm_client_set_protocol (nc, APN_PROTO_ELMI);

  nsm_client_set_service (nc, NSM_SERVICE_INTERFACE);
  nsm_client_set_service (nc, NSM_SERVICE_BRIDGE);

  /* Register NSM Service-reply callbacks */
  nsm_client_set_callback (nc, NSM_MSG_SERVICE_REPLY,
      elmi_nsm_recv_service);

  nsm_client_set_callback (nc, NSM_MSG_LINK_UP,
      elmi_nsm_recv_interface_state);

  nsm_client_set_callback (nc, NSM_MSG_LINK_DOWN,
      elmi_nsm_recv_interface_state);

  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_ADD, 
      elmi_nsm_recv_bridge_add);

  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_DELETE,
      elmi_nsm_recv_bridge_delete);

  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_ADD_PORT,
      elmi_nsm_recv_bridge_add_if);

  nsm_client_set_callback (nc, NSM_MSG_BRIDGE_DELETE_PORT,
      elmi_nsm_recv_bridge_delete_if);

  nsm_client_set_callback (nc, NSM_MSG_VLAN_PORT_TYPE,
                           elmi_nsm_recv_vlan_port_type);

#ifdef HAVE_VLAN
  nsm_client_set_service (nc, NSM_SERVICE_VLAN);

  nsm_client_set_callback (nc, NSM_MSG_VLAN_ADD,
      elmi_nsm_recv_bridge_add_vlan);

  nsm_client_set_callback (nc, NSM_MSG_VLAN_DELETE,
      elmi_nsm_recv_bridge_delete_vlan);

  nsm_client_set_callback (nc, NSM_MSG_VLAN_ADD_PORT,
      elmi_nsm_recv_vlan_add_port);

  nsm_client_set_callback (nc, NSM_MSG_VLAN_DELETE_PORT,
      elmi_nsm_recv_vlan_delete_port);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_EVC_NEW,
      elmi_nsm_recv_evc_add);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_EVC_DELETE,
      elmi_nsm_recv_evc_delete);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_EVC_UPDATE,
      elmi_nsm_recv_evc_update);

#endif /* HAVE_VLAN */ 

  nsm_client_set_callback (nc, NSM_MSG_ELMI_UNI_ADD,
      elmi_nsm_recv_uni_add);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_UNI_DELETE,
      elmi_nsm_recv_uni_delete);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_UNI_UPDATE,
      elmi_nsm_recv_uni_update);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_UNI_BW_ADD,
      elmi_nsm_recv_uni_bw_add);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_UNI_BW_DEL,
      elmi_nsm_recv_uni_bw_del);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_EVC_BW_ADD,
      elmi_nsm_recv_evc_bw_add);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_EVC_BW_DEL,
      elmi_nsm_recv_evc_bw_del);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_EVC_COS_BW_ADD,
      elmi_nsm_recv_evc_cos_bw_add);

  nsm_client_set_callback (nc, NSM_MSG_ELMI_EVC_COS_BW_DEL,
      elmi_nsm_recv_evc_cos_bw_del);

  nsm_client_set_callback (nc, NSM_MSG_VR_SYNC_DONE,
      elmi_nsm_recv_msg_done);

  if_add_hook (&zg->ifg, IF_CALLBACK_NEW, elmi_interface_new_hook);
  if_add_hook (&zg->ifg, IF_CALLBACK_DELETE, elmi_interface_delete_hook);

  if_add_hook (&zg->ifg, IF_CALLBACK_VR_BIND,
      elmi_nsm_recv_interface_update);

  if_add_hook (&zg->ifg, IF_CALLBACK_VR_UNBIND,
      elmi_nsm_recv_interface_delete);

  /* Set disconnect handling callback */
  nsm_client_set_disconnect_callback (nc, elmi_nsm_disconnect_process);

  nsm_client_start (nc);

}

