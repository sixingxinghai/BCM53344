/**@file elmi_port.c
 ** @brief  This elmi_port.c file contains Interface related methods.
 ***/
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "elmid.h"
#include "snprintf.h"
#include "elmi_types.h"
#include "avl_tree.h"
#include "l2_timer.h"
#include "elmi_error.h"
#include "elmi_uni_c.h"
#include "elmi_uni_n.h"
#include "elmi_timer.h"
#include "elmi_port.h"
#include "elmi_api.h"
#include "elmi_debug.h"
#include "nsm_message.h"

struct elmi_evc_status *
elmi_evc_new (void)
{
  struct elmi_evc_status *evc_info = NULL;

  evc_info = XMALLOC (MTYPE_ELMI_EVC, sizeof (struct elmi_evc_status));
  if (!evc_info)
    return NULL;

  pal_mem_set (evc_info, 0, sizeof (struct elmi_evc_status));
  evc_info->evc_bw = NULL;

  return evc_info;
}

void
elmi_evc_status_free (struct elmi_evc_status *evc_info)
{
  struct elmi_bw_profile *evc_bw = NULL;
  s_int8_t i = 0; 

  if (!evc_info)
    return;

  for (i = 0; i < ELMI_MAX_COS_ID; i++)
    {
      evc_bw = evc_info->evc_cos_bw[i];
      if (evc_bw)
        XFREE (MTYPE_ELMI_BW, evc_bw);
    }

  if (evc_info->evc_bw)
    XFREE (MTYPE_ELMI_BW, evc_info->evc_bw);

  XFREE (MTYPE_ELMI_EVC, evc_info);
  evc_info = NULL;
}

struct elmi_cvlan_evc_map *
new_cevlan_evc_map (void)
{
  struct elmi_cvlan_evc_map *cvlan_evc_map = NULL;

  cvlan_evc_map = XMALLOC (MTYPE_ELMI_EVC, sizeof (struct elmi_cvlan_evc_map));
  if (!cvlan_evc_map)
    return NULL;

  pal_mem_set (cvlan_evc_map, 0, sizeof (struct elmi_cvlan_evc_map));

  return cvlan_evc_map;

}

void
elmi_cevlan_evc_map_free (struct elmi_cvlan_evc_map *cvid_evc_map_info)
{
  if (cvid_evc_map_info)
  XFREE (MTYPE_ELMI_CEVLAN_EVC, cvid_evc_map_info);
}

int
elmi_evc_ref_id_cmp (void *data1, void *data2)
{
  struct elmi_evc_status *evc1 = (struct elmi_evc_status *)data1;
  struct elmi_evc_status *evc2 = (struct elmi_evc_status *)data2;

  if ((!evc1) || (!evc2))
    return RESULT_ERROR;

  if (evc1->evc_ref_id  > evc2->evc_ref_id)
    return ELMI_OK;
  else if (evc1->evc_ref_id < evc2->evc_ref_id)
    return ELMI_FAILURE;
  else
    return ELMI_SUCCESS;
}

int
elmi_cevlan_evc_ref_id_cmp (void *data1, void *data2)
{
  struct elmi_cvlan_evc_map *evc_map1 = (struct elmi_cvlan_evc_map *)data1;
  struct elmi_cvlan_evc_map *evc_map2 = (struct elmi_cvlan_evc_map *)data2;

  if ((!evc_map1) || (!evc_map2))
    return RESULT_ERROR;

  if (evc_map1->evc_ref_id  > evc_map2->evc_ref_id)
    return ELMI_OK;
  else if (evc_map1->evc_ref_id < evc_map2->evc_ref_id)
    return ELMI_FAILURE;
  else
    return ELMI_SUCCESS;
}

struct elmi_ifp *
elmi_interface_new (struct interface *ifp)
{
  struct elmi_ifp *elmi_if = NULL;

  if (!ifp)
    return NULL;

  elmi_if = XCALLOC (MTYPE_ELMI_INTERFACE, sizeof (struct elmi_ifp));

  if (elmi_if == NULL)
    {
      zlog_err (ifp->vr->zg, "Could not allocate memory for interface\n");
      return NULL;
    }

  elmi_if->ifp = ifp;
  elmi_if->uni_info = NULL;
  elmi_if->br = NULL;
  elmi_if->port = NULL;
  elmi_if->vlan_bmp = NULL;
  elmi_if->elmim = elmi_master_get();
  elmi_if->port_type = ELMI_DEFAULT_VAL;
  elmi_if->uni_mode = ELMI_DEFAULT_VAL;

  pal_mem_cpy (elmi_if->dev_addr, ifp->hw_addr, ifp->hw_addr_len);

  return elmi_if;
}

int
elmi_interface_add (struct interface *ifp)
{
  struct elmi_ifp *elmi_if = NULL;

  if (!ifp || !ifp->info)
    return RESULT_ERROR;

  elmi_if = ifp->info;

  pal_mem_cpy (elmi_if->dev_addr, ifp->hw_addr, ifp->hw_addr_len);
  elmi_if->ifindex = ifp->ifindex;

  return RESULT_OK;
}

/* Port cleanup on port delete from bridge */
int
elmi_delete_port (struct elmi_ifp *elmi_if)
{
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_cvlan_evc_map *cvlan_evc_map = NULL;
  struct listnode *node = NULL; 
  struct listnode *nn = NULL;

  if (!elmi_if)
    return RESULT_ERROR;

  if (elmi_if->elmi_enabled)
    {
      elmi_api_disable_port (elmi_if->ifindex);
    }

  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIN)
    { 
      /* Clean up evc status information */
      if (listcount (elmi_if->evc_status_list) > 0)
        LIST_LOOP_DEL (elmi_if->evc_status_list, evc_info, node, nn)
          {
            listnode_delete (elmi_if->evc_status_list, evc_info);
            elmi_evc_status_free (evc_info);
          }

      /* Clean uni status information */
      if (elmi_if->uni_info)
        {
          if (elmi_if->uni_info->uni_bw != NULL)
            {
              XFREE (MTYPE_ELMI_BW, elmi_if->uni_info->uni_bw);
              elmi_if->uni_info->uni_bw = NULL;
            }
        }

      /* Clean up evc status information */
      if (listcount (elmi_if->cevlan_evc_map_list) > 0) 
        LIST_LOOP_DEL (elmi_if->cevlan_evc_map_list, cvlan_evc_map, node, nn)
          {
            listnode_delete (elmi_if->cevlan_evc_map_list, cvlan_evc_map);
            elmi_cevlan_evc_map_free (cvlan_evc_map);
          }
    }

  if (elmi_if->evc_status_list != NULL)
    list_delete (elmi_if->evc_status_list);

  if (elmi_if->cevlan_evc_map_list != NULL)
    list_delete (elmi_if->cevlan_evc_map_list);

  if (elmi_if->uni_info)
    {
      XFREE (MTYPE_ELMI_UNI, elmi_if->uni_info);  
      elmi_if->uni_info = NULL;
    }
  elmi_if->br = NULL;
  elmi_if->evc_status_list = NULL;
  elmi_if->cevlan_evc_map_list = NULL;

  if (elmi_if->vlan_bmp)
    {
      XFREE (MTYPE_ELMI_VLAN_BMP, elmi_if->vlan_bmp);  
    }

  elmi_if->vlan_bmp = NULL;

  return RESULT_OK;
}

/* Update EVC information on ELMI UNIN Side */
int
elmi_nsm_evc_add (struct elmi_ifp *elmi_if, struct nsm_msg_elmi_evc *msg)
{
  struct elmi_evc_status *evc_instance = NULL;
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;
  u_int16_t evc_ref_id = 0;
  u_int16_t vid = 0;
  s_int32_t i = 0;
  u_int8_t new_flag = PAL_FALSE;

  if (!elmi_if)
    return RESULT_ERROR;   

  em = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;   

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  /* This update is required only for UNI-N */

  evc_ref_id = msg->evc_ref_id;

  /* Check if the evc exists, evc_ref_id is the key */
  evc_instance = elmi_evc_look_up (elmi_if, msg->evc_ref_id);

  if (!evc_instance)
    {
      /* allocate memory */
      evc_instance = XCALLOC (MTYPE_ELMI_EVC,
          sizeof (struct elmi_evc_status));

      if (evc_instance == NULL)
        {
          return RESULT_ERROR;
        }
      new_flag = PAL_TRUE;
    }

  /* evc_instance->ifindex = msg->ifindex; */
  evc_instance->evc_ref_id = msg->evc_ref_id;
  evc_instance->evc_status_type = msg->evc_status_type;

  evc_instance->evc_type = msg->evc_type;

  evc_instance->bw_profile_level = 0;

  if (pal_strlen (msg->evc_id) > 0)
    pal_strncpy (evc_instance->evc_id, msg->evc_id,
                 ELMI_EVC_NAME_LEN);
  else
    pal_strcpy (evc_instance->evc_id, "");

  evc_instance->evc_status_type = EVC_STATUS_NEW_AND_NOTACTIVE;

  /* Mark the flag as active */
  evc_instance->active = PAL_TRUE;

  if (!elmi_if->evc_status_list)
    elmi_if->evc_status_list = list_new();

  if (!elmi_if->evc_status_list)
    {
      return RESULT_ERROR;
    }

  if (new_flag)
    if (listnode_add_sort (elmi_if->evc_status_list, evc_instance)
        == NULL) 
      {
        if (ELMI_DEBUG(event, EVENT))
          zlog_debug(ZG, "Not able to add evc node to list in %s "
              "line = %d \n", __FUNCTION__, __LINE__ );
        XFREE(MTYPE_ELMI_EVC, evc_instance);
        return RESULT_ERROR;
      }

  if (ELMI_DEBUG (protocol, PROTOCOL))
    {   
      zlog_info (ZG, "EVC Ref ID = %d\n", evc_instance->evc_ref_id);
      zlog_info (ZG, "EVC status type = %d\n", evc_instance->evc_status_type);
      zlog_info (ZG, "EVC Type = %d\n", evc_instance->evc_type);
      zlog_info (ZG, "EVC ID = %s\n", evc_instance->evc_id);
    }

  new_flag = PAL_FALSE;

  /* Check if the evc exists, evc_ref_id is the key */
  cevlan_evc_map = elmi_lookup_cevlan_evc_map (elmi_if, msg->evc_ref_id);

  if (!cevlan_evc_map)
    {
      /* allocate memory */
      cevlan_evc_map = XCALLOC (MTYPE_ELMI_CEVLAN_EVC,
          sizeof (struct elmi_cvlan_evc_map));

      if (cevlan_evc_map == NULL)
        {
          return RESULT_ERROR;
        }

      new_flag = PAL_TRUE;
    }

  /* Now update fields */
  cevlan_evc_map->evc_ref_id = msg->evc_ref_id; 
  cevlan_evc_map->last_ie = ELMI_DEFAULT_VAL;
  cevlan_evc_map->cvlan_evc_map_seq = ELMI_DEFAULT_VAL; 
  cevlan_evc_map->untag_or_pri_frame = ELMI_DEFAULT_VAL; 
  cevlan_evc_map->default_evc = ELMI_DEFAULT_VAL;

  if (new_flag)
    if (listnode_add_sort (elmi_if->cevlan_evc_map_list, cevlan_evc_map)
        == NULL)
      {
        if (ELMI_DEBUG(event, EVENT))
          zlog_debug(ZG, "Failed to add cevlan_evc_map to listr in %s "
              "line = %d \n", __FUNCTION__, __LINE__ );
        XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);
        XFREE(MTYPE_ELMI_EVC, evc_instance);
        return RESULT_ERROR;
      }

  if (ELMI_DEBUG (protocol, PROTOCOL))
    zlog_info (ZG, "Number of cvids in msg = %d\n", msg->num);

  for (i = 0; i < msg->num; i++)
    {
      zlog_info (ZG, "EVC_ADD cvid = %d", i);
      /* Extract vid */    
      vid = NSM_VLAN_VID_MASK & msg->vid_info[i];
      ELMI_VLAN_BMP_SET (cevlan_evc_map->cvlanbitmap, vid); 
    }

  if (msg->vid_info)
    XFREE (MTYPE_TMP, msg->vid_info);

  cevlan_evc_map->default_evc = msg->default_evc;
  cevlan_evc_map->untag_or_pri_frame = msg->untagged_frame;

  if (!elmi_if->cevlan_evc_map_list)
    elmi_if->cevlan_evc_map_list = list_new();

  if (!elmi_if->cevlan_evc_map_list)
    {
      XFREE(MTYPE_ELMI_EVC, evc_instance);
      XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);
      return RESULT_ERROR;
    }

  /* if ELMI is enabled on PE, then update data instance field */
  if (bridge->bridge_type == ELMI_BRIDGE_TYPE_MEF_PE)
    {
      if (elmi_if->elmi_enabled)
        {
          port = elmi_if->port;
          if (port)
            {
              port->data_instance++;

              if (ELMI_DEBUG(protocol, PROTOCOL))
                {
                  zlog_debug (ZG, "New EVC Added, data instance becomes %d", 
                      port->data_instance);
                }
            }
        }

    }

  return RESULT_OK;
}

int
elmi_nsm_evc_update (struct elmi_ifp *elmi_if, struct nsm_msg_elmi_evc *msg)
{
  struct elmi_evc_status *evc_instance = NULL;
  struct elmi_cvlan_evc_map *ce_vlan_entry = NULL;
  u_int16_t evc_ref_id = 0;
  u_int16_t vid = 0;
  struct elmi_port *port = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_master *em = NULL;
  s_int32_t i = 0;

  if (!elmi_if)
   return RESULT_ERROR;   

  bridge = elmi_if->br;
  if (!bridge)
   return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  evc_ref_id = msg->evc_ref_id;

  /* Check if the evc exists, evc_ref_id is the key */
  evc_instance = elmi_evc_look_up (elmi_if, msg->evc_ref_id);
  if (!evc_instance)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        zlog_debug (ZG, "EVC Instance %d does not exists", msg->evc_ref_id); 

      return RESULT_ERROR;
    }

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_ID_STR))
    pal_mem_cpy (evc_instance->evc_id, msg->evc_id, ELMI_EVC_NAME_LEN);

  /* No need to update evc status */ 

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_TYPE))
    {
      evc_instance->evc_type = msg->evc_type;
    }

  /* Get ce-vlan evc map entry */
  ce_vlan_entry = elmi_lookup_cevlan_evc_map (elmi_if,
                                              evc_instance->evc_ref_id);

  if (ce_vlan_entry)
    {
      /* Delete old cevlan membership and update new one */
      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_CVLAN_BITMAPS))
        {
          for (i = 0; i < msg->num; i++)
            {
              zlog_info (ZG, "EVC_ADD cvid = %d", i);
              /* Extract vid */
              vid = NSM_VLAN_VID_MASK & msg->vid_info[i];
              ELMI_VLAN_BMP_SET (ce_vlan_entry->cvlanbitmap, vid);
            }

          if (msg->vid_info)
            XFREE (MTYPE_TMP, msg->vid_info);
        }

      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_DEFAULT_EVC))
        {
          ce_vlan_entry->default_evc = msg->default_evc;
          if (ELMI_DEBUG (protocol, PROTOCOL))
            zlog_info (ZG, "Default Evc = %d\n", ce_vlan_entry->default_evc);
        }

      if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_UNTAGGED_PRI))
        {
          ce_vlan_entry->untag_or_pri_frame = msg->untagged_frame;
          if (ELMI_DEBUG (protocol, PROTOCOL))
            zlog_info (ZG, "untagged/priority frame = %d\n", 
                       ce_vlan_entry->untag_or_pri_frame);
        }
    }

  if (ELMI_DEBUG (protocol, PROTOCOL))
    {
      zlog_info (ZG, "EVC Ref ID = %d\n", evc_instance->evc_ref_id);
      zlog_info (ZG, "EVC status type = %d\n", evc_instance->evc_status_type);
      zlog_info (ZG, "EVC Type = %d\n", evc_instance->evc_type);
      zlog_info (ZG, "EVC ID = %s\n", evc_instance->evc_id);
    }

  /* if ELMI is enabled on PE, then update data instance field */
  if (bridge->bridge_type == ELMI_BRIDGE_TYPE_MEF_PE)
    {
      port = elmi_if->port;
      if (port && elmi_if->elmi_enabled)
        {
          port->data_instance++;
          if (ELMI_DEBUG (protocol, PROTOCOL))
            {
              zlog_debug (ZG, "EVC details updated, data instance becomes %d",
                          port->data_instance);
            }
        }
    }

  return RESULT_OK;

}

/* Delete EVC details */
int
elmi_nsm_evc_delete (struct elmi_ifp *elmi_if, struct nsm_msg_elmi_evc *msg)
{
  struct elmi_evc_status *evc_instance = NULL;
  u_int16_t evc_ref_id = 0;
  struct elmi_port *port = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_master *em = NULL;

  evc_ref_id = msg->evc_ref_id;

  if (!elmi_if)
    return RESULT_ERROR;
  
  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (ELMI_DEBUG (protocol, PROTOCOL))
    zlog_debug (ZG, "Recvd EVC Delete MSG [%s%d]", 
        __FUNCTION__, __LINE__);

  /* Check if the evc exists */
  evc_instance = elmi_evc_look_up (elmi_if, msg->evc_ref_id);
  if (!evc_instance)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "EVC Instance %d does not exists", 
                      msg->evc_ref_id);
        }
      return RESULT_ERROR;
    }

  /* Mark the flag as inactive. 
   * Delete the evc from UNI-N after sending the async message to UNI-C for 
   * this evc.
   */
  evc_instance->active = PAL_FALSE;

  /* Check if elmi is enabled on this port 
   * Triggers a asynchronous evc notification msg to UNI-C */
  /* if ELMI is enabled on PE, then update data instance field */

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;

  if (bridge->bridge_type == ELMI_BRIDGE_TYPE_MEF_PE)
    {
      port = elmi_if->port;
      if (elmi_if->elmi_enabled && port)
        {
         /* Add the event to the event list and start the async timer
          * if it is not running 
          */
          if (!port->async_evc_list || !port->async_evc_list->head)
            {
              /* If there are no traps pending in the queue, 
               * send the trap immediately
               */ 
              elmi_unin_send_async_evc_msg (evc_instance, port);
            }
          else
            {
              /* Add the evc node to the trap list and start the timer
               * if it is not running 
               */ 
              listnode_add (port->async_evc_list, evc_instance);
              if (!l2_is_timer_running (port->async_timer))
                {
                  l2_start_timer (elmi_async_evc_timer_handler,
                                  (void *)port,
                                  elmi_if->asynchronous_time, ZG);
                }

            }

         port->data_instance++;
        }

    }

  return RESULT_OK;
}

int
elmi_nsm_uni_add (struct elmi_ifp *elmi_if, struct nsm_msg_uni *msg)
{
 struct elmi_uni_info *uni_info = NULL;
 struct elmi_bridge *bridge = NULL;
 struct elmi_port *port = NULL;
 struct elmi_master *em = NULL;

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   
 
  if (elmi_if->uni_info == NULL)
    {
      uni_info = XCALLOC (MTYPE_ELMI_UNI,
                          sizeof (struct elmi_uni_info));

      if (uni_info == NULL)
        {
          if (ELMI_DEBUG (protocol, PROTOCOL))
            zlog_err (ZG, "Could not allocate memory for UNI\n");
          return RESULT_ERROR;
        }

      elmi_if->uni_info = uni_info;
    }

  /* Update details in the UNI instance. */
  if (msg->cevlan_evc_map_type >= ELMI_ALL_TO_ONE_BUNDLING ||
      msg->cevlan_evc_map_type <= ELMI_BUNDLING)
    {
      elmi_if->uni_info->cevlan_evc_map_type = 
            elmi_get_map_type (msg->cevlan_evc_map_type);
    }
  else
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        zlog_err (ZG, "Wrong CE-VLAN to EVC map type received from NSM\n");

      elmi_if->uni_info->cevlan_evc_map_type = ELMI_BUNDLING_INVALID;
    }

  pal_mem_cpy (elmi_if->uni_info->uni_id, msg->uni_id, ELMI_UNI_NAME_LEN);

  if (bridge->bridge_type == ELMI_BRIDGE_TYPE_MEF_PE)
    {
      port = elmi_if->port;
      if (port && elmi_if->elmi_enabled)
        {
          port->data_instance++;
          if (ELMI_DEBUG (protocol, PROTOCOL))
            {
              zlog_debug (ZG, "UNI details updated, data instance becomes %d", 
                          port->data_instance);
            }
        }
    }

  return RESULT_OK;

}

int
elmi_nsm_uni_delete (struct elmi_ifp *elmi_if)
{
  struct elmi_uni_info *uni_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_master *em = NULL;

  if (!elmi_if)
   return RESULT_ERROR;   
  
  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
   return RESULT_ERROR;   

  uni_info = elmi_if->uni_info;

  if (!uni_info)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "Recvd UNI Delete MSG, UNI instance not found"); 
        }

      return RESULT_ERROR;
    }

  XFREE (MTYPE_ELMI_UNI, uni_info);
  elmi_if->uni_info = NULL;

  return RESULT_OK;
}

int
elmi_nsm_uni_update (struct elmi_ifp *elmi_if, struct nsm_msg_uni *msg)
{
  struct elmi_uni_info *uni_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  uni_info = elmi_if->uni_info;

  if (!uni_info)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "UNI Info does not exists"); 
        }
      return RESULT_ERROR;
    }
  
  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_UNI_ID))
    pal_mem_cpy (uni_info->uni_id, msg->uni_id, ELMI_UNI_NAME_LEN);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE))
    {
      elmi_if->uni_info->cevlan_evc_map_type =
        elmi_get_map_type (msg->cevlan_evc_map_type);
    }

  if (bridge->bridge_type == ELMI_BRIDGE_TYPE_MEF_PE)
    {
      port = elmi_if->port;
      if (port && elmi_if->elmi_enabled)
        {
          port->data_instance++;
          if (ELMI_DEBUG (protocol, PROTOCOL))
            {
              zlog_debug (ZG, "UNI details updated, data instance becomes %d",
                          port->data_instance);
            }
        }
    }

  return RESULT_OK;
}

s_int32_t
elmi_nsm_uni_bw_add (struct elmi_ifp *elmi_if, 
                     struct nsm_msg_bw_profile *msg)
{
  struct elmi_uni_info *uni_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  if (ELMI_DEBUG (protocol, PROTOCOL))
    {
      zlog_debug (ZG, "Recvd UNI UPDATE MSG [%s%d]", 
          __FUNCTION__, __LINE__);
    }

  if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_UNI_BW))
    return RESULT_ERROR;

  if (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE)
    return RESULT_ERROR;

  uni_info = elmi_if->uni_info;

  if (!uni_info)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "UNI Info does not exists"); 
        }
      return RESULT_ERROR;
    }

  if (uni_info->uni_bw == NULL)
    {
      uni_info->uni_bw = XCALLOC (MTYPE_ELMI_BW, sizeof 
          (struct elmi_bw_profile));

      if (uni_info->uni_bw == NULL)
        {
          zlog_err (ZG, "Could not allocate memory for bandwidth profiling\n");
          return RESULT_ERROR;
        }
    } 

  uni_info->uni_bw->cir = msg->cir;
  uni_info->uni_bw->cbs = msg->cbs;
  uni_info->uni_bw->eir = msg->eir;
  uni_info->uni_bw->ebs = msg->ebs;
  uni_info->uni_bw->cm = msg->cm;
  uni_info->uni_bw->cf = msg->cf;

  elmi_if->bw_profile_level = ELMI_BW_PROFILE_PER_UNI;

  /* Update magnitude and multiplier fields */
  elmi_get_matissa_exponent_16bit (uni_info->uni_bw->cir, 
                                   &uni_info->uni_bw->cir_multiplier, 
                                   &uni_info->uni_bw->cir_magnitude);

  elmi_get_matissa_exponent_8bit (uni_info->uni_bw->cbs, 
                                   &uni_info->uni_bw->cbs_multiplier, 
                                   &uni_info->uni_bw->cbs_magnitude);

  elmi_get_matissa_exponent_16bit (uni_info->uni_bw->eir, 
                                   &uni_info->uni_bw->eir_multiplier, 
                                   &uni_info->uni_bw->eir_magnitude);

  elmi_get_matissa_exponent_8bit (uni_info->uni_bw->ebs, 
                                   &uni_info->uni_bw->ebs_multiplier, 
                                   &uni_info->uni_bw->ebs_magnitude);

  port = elmi_if->port;
  if (elmi_if->elmi_enabled && port)
    {
      port->data_instance++;
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "UNI details updated, data instance becomes %d",
              port->data_instance);
        }
    }

  return RESULT_OK;
}

int
elmi_nsm_uni_bw_del (struct elmi_ifp *elmi_if, struct nsm_msg_bw_profile *msg)
{
  struct elmi_uni_info *uni_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_UNI_BW))
    return RESULT_ERROR;

  if (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE)
    return RESULT_ERROR;

  uni_info = elmi_if->uni_info;

  if (!uni_info)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "UNI Info does not exists"); 
        }
      return RESULT_ERROR;
    }

  if (uni_info->uni_bw == NULL)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "UNI Info BW does not exists"); 
        }
      return RESULT_ERROR;
    }

  XFREE(MTYPE_ELMI_BW, uni_info->uni_bw);
  uni_info->uni_bw = NULL;

  elmi_if->bw_profile_level = ELMI_BW_PROFILE_INVALID;

  port = elmi_if->port;
  if (elmi_if->elmi_enabled && port)
    {
      port->data_instance++;
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "UNI details updated, data instance becomes %d", 
                      port->data_instance);
        }
    }

  return RESULT_OK;
}

int
elmi_nsm_evc_bw_add (struct elmi_ifp *elmi_if, struct nsm_msg_bw_profile *msg)
{
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;
  u_int16_t evc_ref_id = 0; 

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  if (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE)
    return RESULT_ERROR;

  if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_BW))
    return RESULT_ERROR;

  /* Get evc based on evc ref Id */
   evc_ref_id = msg->evc_ref_id;

   /* Check if the evc exists, evc_ref_id is the key */
   evc_info = elmi_evc_look_up (elmi_if, msg->evc_ref_id);
   if (!evc_info)
     {
       if (ELMI_DEBUG (protocol, PROTOCOL))
         {
           zlog_debug (ZG, "EVC Instance %d does not exists", 
                       msg->evc_ref_id);
         }

       return RESULT_ERROR;
     } 

   if (evc_info->evc_bw == NULL)
     {
       evc_info->evc_bw = XCALLOC (MTYPE_ELMI_BW, sizeof 
           (struct elmi_bw_profile));

       if (evc_info->evc_bw == NULL)
         {
           if (ELMI_DEBUG (protocol, PROTOCOL))
             zlog_err (ZG, "Could not allocate memory for BW profile\n");

           return RESULT_ERROR;
         }
     } 

   evc_info->evc_bw->cir = msg->cir;
   evc_info->evc_bw->cbs = msg->cbs;
   evc_info->evc_bw->eir = msg->eir;
   evc_info->evc_bw->ebs = msg->ebs;
   evc_info->evc_bw->cm = msg->cm;
   evc_info->evc_bw->cf = msg->cf;

   elmi_if->bw_profile_level = ELMI_BW_PROFILE_PER_EVC;
   evc_info->bw_profile_level = ELMI_EVC_BW_PROFILE_PER_EVC;

  /* Update magnitude and multiplier fields */
  elmi_get_matissa_exponent_16bit (evc_info->evc_bw->cir, 
                                   &evc_info->evc_bw->cir_multiplier, 
                                   &evc_info->evc_bw->cir_magnitude);

  elmi_get_matissa_exponent_8bit (evc_info->evc_bw->cbs, 
                                   &evc_info->evc_bw->cbs_multiplier, 
                                   &evc_info->evc_bw->cbs_magnitude);

  elmi_get_matissa_exponent_16bit (evc_info->evc_bw->eir, 
                                   &evc_info->evc_bw->eir_multiplier, 
                                   &evc_info->evc_bw->eir_magnitude);

  elmi_get_matissa_exponent_8bit (evc_info->evc_bw->ebs, 
                                   &evc_info->evc_bw->ebs_multiplier, 
                                   &evc_info->evc_bw->ebs_magnitude);

   port = elmi_if->port;
   if (elmi_if->elmi_enabled && port)
     {
       port->data_instance++;
       if (ELMI_DEBUG (protocol, PROTOCOL))
         {
           zlog_debug (ZG, "UNI details updated, data instance becomes %d", 
                       port->data_instance);
         }
     }

  return RESULT_OK;
}

int
elmi_nsm_evc_bw_del (struct elmi_ifp *elmi_if, struct nsm_msg_bw_profile *msg)
{
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  if (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE)
    return RESULT_ERROR;

  if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_BW))
    return RESULT_ERROR;

  /* Check if the evc exists, evc_ref_id is the key */
  evc_info = elmi_evc_look_up (elmi_if, msg->evc_ref_id);
  if (!evc_info)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "EVC Instance %d does not exists", 
                      msg->evc_ref_id); 
        }
      return RESULT_ERROR;
    } 

  XFREE(MTYPE_ELMI_BW, evc_info->evc_bw);
  evc_info->evc_bw = NULL;
  elmi_if->bw_profile_level = ELMI_BW_PROFILE_INVALID;

  port = elmi_if->port;
  if (elmi_if->elmi_enabled && port)
    {
      port->data_instance++;
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "EVC details updated, data instance becomes %d", 
                      port->data_instance);
        }
    }

  return RESULT_OK;
}

int
elmi_nsm_evc_cos_bw_add (struct elmi_ifp *elmi_if, 
                         struct nsm_msg_bw_profile *msg)
{
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL; 
  u_int16_t evc_ref_id = 0; 
  u_int8_t cos_id = 0;

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  if (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE)
    return RESULT_ERROR;

  if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_COS_BW))
    return RESULT_ERROR;

  /* Get evc based on evc ref Id */
   evc_ref_id = msg->evc_ref_id;

   /* Check if the evc exists, evc_ref_id is the key */
   evc_info = elmi_evc_look_up (elmi_if, msg->evc_ref_id);
   if (!evc_info)
     {
       if (ELMI_DEBUG (protocol, PROTOCOL))
         zlog_debug (ZG, "EVC Instance %d does not exists", msg->evc_ref_id); 

       return RESULT_ERROR;
     } 

   /* Get the cos id and based upon that allocate memory */
    cos_id = msg->cos_id;
    if (cos_id == 0)
      {
       /* Should not happen */
       return RESULT_ERROR;
      }
  
   if (evc_info->evc_cos_bw[cos_id] == NULL)
     {
       evc_info->evc_cos_bw[cos_id] = XCALLOC (MTYPE_ELMI_BW, sizeof 
           (struct elmi_bw_profile));

       if (evc_info->evc_cos_bw[cos_id] == NULL)
         {
           if (ELMI_DEBUG (protocol, PROTOCOL))
             zlog_err (ZG, "Could not allocate memory for evc_cos_bw\n");
           return RESULT_ERROR;
         }
     } 

   evc_info->evc_cos_bw[cos_id]->cir = msg->cir;
   evc_info->evc_cos_bw[cos_id]->cbs = msg->cbs;
   evc_info->evc_cos_bw[cos_id]->eir = msg->eir;
   evc_info->evc_cos_bw[cos_id]->ebs = msg->ebs;
   evc_info->evc_cos_bw[cos_id]->cf = msg->cf;
   evc_info->evc_cos_bw[cos_id]->cm = msg->cm;


  /* Update magnitude and multiplier fields */
  elmi_get_matissa_exponent_16bit (evc_info->evc_cos_bw[cos_id]->cir, 
                                  &evc_info->evc_cos_bw[cos_id]->cir_multiplier,
                                  &evc_info->evc_cos_bw[cos_id]->cir_magnitude);

  elmi_get_matissa_exponent_8bit (evc_info->evc_cos_bw[cos_id]->cbs, 
                                  &evc_info->evc_cos_bw[cos_id]->cbs_multiplier,
                                  &evc_info->evc_cos_bw[cos_id]->cbs_magnitude);

  elmi_get_matissa_exponent_16bit (evc_info->evc_cos_bw[cos_id]->eir, 
                                  &evc_info->evc_cos_bw[cos_id]->eir_multiplier,
                                  &evc_info->evc_cos_bw[cos_id]->eir_magnitude);

  elmi_get_matissa_exponent_8bit (evc_info->evc_cos_bw[cos_id]->ebs, 
                                  &evc_info->evc_cos_bw[cos_id]->ebs_multiplier,
                                  &evc_info->evc_cos_bw[cos_id]->ebs_magnitude);

   evc_info->evc_cos_bw[cos_id]->per_cos = msg->per_cos;

   elmi_if->bw_profile_level = ELMI_BW_PROFILE_PER_EVC;
   evc_info->bw_profile_level = ELMI_EVC_BW_PROFILE_PER_COS;

   port = elmi_if->port;
   if (elmi_if->elmi_enabled && port)
     {
       port->data_instance++;
       if (ELMI_DEBUG (protocol, PROTOCOL))
         {
           zlog_debug (ZG, "EVC COS BW details updated, data instance " 
                       "becomes %d", port->data_instance);
         }
     }

  return RESULT_OK;
}

int
elmi_nsm_evc_cos_bw_del (struct elmi_ifp *elmi_if, 
                         struct nsm_msg_bw_profile *msg)
{
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_bridge *bridge = NULL;
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;

  if (!elmi_if)
    return RESULT_ERROR;   

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;   

  if (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_PE)
    return RESULT_ERROR;

  if (!NSM_CHECK_CTYPE (msg->cindex, NSM_ELMI_CTYPE_EVC_BW))
    return RESULT_ERROR;

  /* Check if the evc exists, evc_ref_id is the key */
  evc_info = elmi_evc_look_up (elmi_if, msg->evc_ref_id);
  if (!evc_info)
    {
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "EVC Instance %d does not exists", msg->evc_ref_id); 
        }
      return RESULT_ERROR;
    } 

  XFREE(MTYPE_ELMI_BW, evc_info->evc_cos_bw[msg->cos_id]);
  evc_info->evc_bw = NULL;
  elmi_if->bw_profile_level = ELMI_BW_PROFILE_INVALID;

  port = elmi_if->port;
  if (elmi_if->elmi_enabled && port)
    {
      port->data_instance++;
      if (ELMI_DEBUG (protocol, PROTOCOL))
        {
          zlog_debug (ZG, "EVC details updated, data instance becomes %d", 
                      port->data_instance);
        }
    }

  return RESULT_OK;
}

/*
 * This function will be invoked when ELMI receives a EVC status change
 * message from ONMD for the already configured evcs. 
 */
s_int32_t
elmi_update_evc_status_type (u_int8_t *br_name, 
                             u_int16_t evc_ref_id,
                             u_int8_t evc_status)
{
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_port *port = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_bridge *bridge = NULL;
  struct avl_node *avl_port = NULL;
  u_int8_t local_evc_status = 0;
  u_int8_t new_bit = PAL_FALSE;

  bridge = elmi_find_bridge (br_name);
  if (!bridge)
    return RESULT_ERROR;

  /* Get port from bridge */
  AVL_TREE_LOOP (bridge->port_tree, elmi_if, avl_port)
    {
      /* Check if the evc exists, evc_ref_id is the key */
      evc_info = elmi_evc_look_up (elmi_if, evc_ref_id);
      if (!evc_info)
        continue;

      local_evc_status = 
        elmi_get_local_evc_status (evc_status);

      /* Do not overwrite the update from NSM, Just
       * update only the active/inactive bit, not new bit
       */ 
      if (evc_info->evc_status_type & EVC_STATUS_NEW_AND_NOTACTIVE)
        new_bit = PAL_TRUE;

      evc_info->evc_status_type = local_evc_status; 

      if (new_bit)
        evc_info->evc_status_type |= new_bit;

      /* Add this evc node to async evc traps list 
       * to maintain min config interval between 2 async msgs
       */
      port = elmi_if->port;

      if (port && elmi_if->elmi_enabled)
        {
          if (port->async_evc_list == NULL)
            {
              /* if there are no traps pending, send the trap immediately */
              elmi_unin_send_async_evc_msg (evc_info, port);
            }
          else
            {
              /* Add the evc node to the trap list and start the timer 
               * if it is not running
               */
              listnode_add (port->async_evc_list, evc_info);
              if (!l2_is_timer_running(port->async_timer))
                {
                  l2_start_timer (elmi_async_evc_timer_handler,
                                  (void *)port,
                                  elmi_if->asynchronous_time, ZG);
                }
            }
        }

    }

  return RESULT_OK;

}

/* Clear the information learnt from UNI-N */
void
elmi_unic_clear_learnt_database (struct elmi_ifp *elmi_if)
{
  struct elmi_evc_status *evc_node = NULL;
  struct listnode *node = NULL;
  struct listnode *nn = NULL;
  struct elmi_cvlan_evc_map *cevlan_map_node = NULL;

  if (!elmi_if)
    return;

  /* Clean up evc status information */
  LIST_LOOP_DEL (elmi_if->evc_status_list, evc_node, node, nn)
    {
      if (evc_node)
        {
          listnode_delete (elmi_if->evc_status_list, evc_node); 
        }
    }

  /* Clear UNI status info */
  if (elmi_if->uni_info)
    {
      elmi_if->uni_info->cevlan_evc_map_type = 0;
      if (elmi_if->uni_info->uni_bw != NULL) 
        {
          XFREE (MTYPE_ELMI_BW, elmi_if->uni_info->uni_bw);  
          elmi_if->uni_info->uni_bw = NULL;
        }

      /* Reset uni id */
      pal_mem_set(elmi_if->uni_info->uni_id, 0, ELMI_UNI_NAME_LEN);
    } 

  /* Clean up ce-vlan evc map information */
  if (listcount (elmi_if->cevlan_evc_map_list) > 0)
    LIST_LOOP_DEL (elmi_if->cevlan_evc_map_list, cevlan_map_node, node, nn)
      {
        if (cevlan_map_node)
          {
            listnode_delete (elmi_if->cevlan_evc_map_list, cevlan_map_node);
          }
      }

}

void
elmi_unic_delete_cvlan_evc_map (struct elmi_ifp *elmi_if, 
                                struct elmi_cvlan_evc_map *cevlan_evc_map)
{
  struct nsm_msg_vlan_port msg;
  u_int16_t cvlan = 0;
  s_int16_t count = 0;
  s_int32_t ret = 0;

  if (!elmi_if)
    return;

  if (cevlan_evc_map)
    { 
      /* check for vlans added by ELMI */
      /* get each cvlan mapped to evc */
      ELMI_VLAN_SET_BMP_ITER_BEGIN (cevlan_evc_map->cvlanbitmap, cvlan)
        {
          /* For each evc get cvlan membership and do autoconfig */
          if (ELMI_VLAN_BMP_IS_MEMBER (elmi_if->vlan_bmp, cvlan))
            {
              count++;
            }
        }
      ELMI_VLAN_SET_BMP_ITER_END (cevlan_evc_map->cvlanbitmap, cvlan);

      if (count > 0)
        {
          pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
          msg.ifindex = elmi_if->ifindex;
          msg.num = count;
          msg.vid_info = XCALLOC (MTYPE_TMP, sizeof (u_int16_t) * msg.num);

          count = 0;

          /* Get each cvlan mapped to evc */
          ELMI_VLAN_SET_BMP_ITER_BEGIN (cevlan_evc_map->cvlanbitmap , cvlan)
            {
              /* For each evc get cvlan membership and 
               * do autoconfig if required
               */
              if (ELMI_VLAN_BMP_IS_MEMBER (elmi_if->vlan_bmp, cvlan))
                {
                  msg.vid_info[count] = cvlan;
                  count++;
                }
            }

          ELMI_VLAN_SET_BMP_ITER_END (cevlan_evc_map->cvlanbitmap, cvlan);

          /* Send message to NSM regarding auto config */
          ret = elmi_nsm_send_auto_vlan_port (ZG->nc, &msg,
                              NSM_MSG_ELMI_AUTO_VLAN_DEL_PORT);

          if (ret < 0)
            {
              /* Log a message about auto vlan failure in NSM */
              zlog_err(ZG, "deletion of vlan failed in NSM\n");
            }

          XFREE (MTYPE_TMP, msg.vid_info);
        }

    }

}

void
elmi_unic_delete_evc (struct elmi_ifp *elmi_if,
                      struct elmi_evc_status *evc_info)
{
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL;
  s_int32_t i = 0;
  struct elmi_bw_profile *evc_bw = NULL;
    
  if (!elmi_if || !evc_info)
    return;

  cevlan_evc_map = elmi_lookup_cevlan_evc_map (elmi_if, evc_info->evc_ref_id);
  if (cevlan_evc_map)
    elmi_unic_delete_cvlan_evc_map (elmi_if, cevlan_evc_map);

  if (evc_info)
    {
      for (i = 0; i < ELMI_MAX_COS_ID; i++)
        {
          evc_bw = evc_info->evc_cos_bw[i];
          if (evc_bw)
            XFREE (MTYPE_ELMI_BW, evc_bw);
        }

      if (evc_info->evc_bw)
        XFREE (MTYPE_ELMI_BW, evc_info->evc_bw);
    }
}
