/**@file nsm_elmi.c
** @brief  This nsm_elmi.c file contains the messeaging part for NSM to ELMI.
**/
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "hal_incl.h"

#include "lib.h"
#include "if.h"
#include "cli.h"
#include "table.h"
#include "avl_tree.h"
#include "linklist.h"
#include "snprintf.h"
#include "bitmap.h"
#include "message.h"
#include "nsm_message.h"
#include "nsm_client.h"

#include "thread.h"
#include "nsm/nsm_debug.h"
#include "nsm_router.h"

#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_server.h"
#include "nsm_vlan.h"
#include "nsm_api.h"
#include "nsm_elmi.h"

#ifdef HAVE_ELMID
#include "nsm_pro_vlan.h"

struct nsm_pro_edge_info *
nsm_reg_tab_svlan_info_port_lookup (struct nsm_svlan_reg_info *svlan_info,
                                        struct interface *ifp);

s_int32_t
nsm_check_auto_vlan_membership (struct nsm_bridge *bridge, nsm_vid_t vid)
{
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  struct avl_node *avl_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;

  AVL_TREE_LOOP (bridge->port_tree, br_port, avl_port)
    {

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      vlan_port = &br_port->vlan_port;

      if ( !vlan_port )
        continue;

      if ((NSM_VLAN_BMP_IS_MEMBER(vlan_port->staticMemberBmp, vid)))
        {
          /* At least one port is member of this vlan */
          return PAL_TRUE;
        }
      
    }

   return PAL_FALSE;

}


/* NSM server send message. */
s_int32_t 
nsm_evc_send_evc_add_msg (struct nsm_msg_elmi_evc *msg,
                          int msgid)
{
  s_int32_t nbytes = 0;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  s_int32_t i = 0;
  s_int32_t ret = 0;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if ((NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN)) && 
        (nse->service.protocol_id == APN_PROTO_ELMI))
      {

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_evc_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge message. */
        ret = nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);

      }

  return ret;
}

/* Send EVC delete information to ELMI.  */
s_int32_t
nsm_server_send_evc_delete (struct nsm_server_entry *nse,
                            struct interface *ifp,
                            u_int16_t evc_ref_id)
{
  struct nsm_msg_elmi_evc msg;
  s_int32_t len = 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_elmi_evc));

  /* Init Cindex. */
  msg.cindex = 0;

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

  /* EVC reference ID is mandatory.  */
  msg.evc_ref_id = evc_ref_id;

  len = nsm_encode_evc_msg (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_ELMI_EVC_DELETE, 0, len);

  return RESULT_OK;
}

s_int32_t
nsm_server_send_evc_update (struct nsm_server_entry *nse,
                            struct interface *ifp, u_int16_t evc_id, 
                            struct nsm_svlan_reg_info *svlan_info, 
                            cindex_t cindex)
{
  struct nsm_msg_elmi_evc msg;
  s_int32_t len = 0;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan *vlan = NULL;
  struct avl_node *rn = NULL;
  struct avl_tree *vlan_table = NULL;
  struct nsm_pro_edge_info *pro_edge_port = NULL;
  struct nsm_vlan vlan_info;
  s_int32_t count = 0;
  u_int16_t vid = 0;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
     return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;

  bridge = zif->bridge;
  if (!bridge)
    return RESULT_ERROR;

  vlan_port = &br_port->vlan_port;

  if (!vlan_port)
    return RESULT_ERROR;

  vlan_table = bridge->svlan_table;

  NSM_VLAN_SET (&vlan_info, evc_id);

  rn = avl_search (vlan_table, (void *)&vlan_info);

  if (rn)
    {
      vlan = rn->info;

      if (!vlan)
        return NSM_VLAN_ERR_GENERAL;
    }
  else
    return RESULT_ERROR;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_elmi_evc));

  /* Check requested service.  */
  if (!nsm_service_check (nse, NSM_SERVICE_VLAN) ||
      (nse->service.protocol_id != APN_PROTO_ELMI))
    return RESULT_OK;

  msg.cindex = 0;
  /* Init Cindex. */
  msg.cindex = cindex;

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

  /* EVC Reference id is mandatory.  */
  msg.evc_ref_id = evc_id;

  msg.evc_status_type = NSM_EVC_STATUS_NOT_ACTIVE;

  if (NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_EVC_TYPE))
    {
      if (vlan->type & NSM_SVLAN_M2M)
        msg.evc_type = NSM_ELMI_SVLAN_M2M;
      else
        msg.evc_type = NSM_ELMI_SVLAN_P2P;
    }

  /* EVC ID TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_ELMI_CTYPE_EVC_ID_STR))
    {
      if (vlan->evc_id)
        pal_strncpy (msg.evc_id, vlan->evc_id, NSM_EVC_NAME_LEN);
    }

  if (NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_EVC_CVLAN_BITMAPS))
    {
      if (svlan_info)
        {
          /* Get count of the vids. */
          count = 0;
          NSM_VLAN_SET_BMP_ITER_BEGIN (svlan_info->cvlanMemberBmp, vid)
            {
              count++;
            }
          NSM_VLAN_SET_BMP_ITER_END (svlan_info->cvlanMemberBmp, vid);

          if (count > 0)
            {
              msg.num = count;
              msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

              count = 0;

              NSM_VLAN_SET_BMP_ITER_BEGIN (svlan_info->cvlanMemberBmp, vid)
                {
                  msg.vid_info[count] = vid;
                  count++;
                }
              NSM_VLAN_SET_BMP_ITER_END (svlan_info->cvlanMemberBmp, vid);
            }
        }


      /* over write evc status if bit map changes */
      msg.evc_status_type = NSM_EVC_STATUS_NEW_AND_NOTACTIVE;
    }
    
  if (NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_DEFAULT_EVC))
    {
      if (vlan_port->uni_default_evc == evc_id) 
        msg.default_evc = PAL_TRUE;
      else
        msg.default_evc = PAL_FALSE;
    }

  if (NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_UNTAGGED_PRI))
    {
      if ((pro_edge_port = nsm_reg_tab_svlan_info_port_lookup
            (svlan_info, ifp)))
        {
          if (NSM_VLAN_BMP_IS_MEMBER (svlan_info->cvlanMemberBmp, 
                pro_edge_port->untagged_vid))
            {
              if (br_port->uni_ser_attr == NSM_UNI_AIO_BNDL) 
                msg.untagged_frame = PAL_FALSE;
              else
                msg.untagged_frame = PAL_TRUE;
            }
          else
            msg.untagged_frame = PAL_FALSE;
        }
    }

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode EVC service.  */
  len = nsm_encode_evc_msg (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, 0, 0, NSM_MSG_ELMI_EVC_UPDATE, 0, len);

  if (msg.vid_info)
    XFREE (MTYPE_TMP, msg.vid_info);

  return RESULT_OK;

}

/* NSM server send message. */
s_int32_t
nsm_send_uni_add_msg (struct nsm_bridge *bridge,
                      struct nsm_msg_uni *msg,
                      int msgid)
{
  s_int32_t nbytes = 0;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  int i = 0;
  int ret = 0;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if ((NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_INTERFACE)) &&
        (nse->service.protocol_id == APN_PROTO_ELMI))
      {

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bridge message. */
        nbytes = nsm_encode_uni_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge message. */
        ret = nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }

  return ret;
}

/* Send UNI delete information to ELMI.  */
s_int32_t
nsm_server_send_uni_delete (struct nsm_server_entry *nse,
                            struct interface *ifp)
{
  struct nsm_msg_uni msg;
  s_int32_t len = 0;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_uni));

  /* Check requested service.  */
  if (! nsm_service_check (nse, NSM_SERVICE_INTERFACE) ||
      (nse->service.protocol_id != APN_PROTO_ELMI))
    return RESULT_OK;

  /* Init Cindex. */
  msg.cindex = 0;

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

  len = nsm_encode_uni_msg (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_ELMI_UNI_DELETE, 0, len);

  return RESULT_OK;
}

s_int32_t
nsm_server_send_uni_update (struct nsm_server_entry *nse,
                            struct nsm_bridge_port *br_port, 
                            struct interface *ifp, 
                            u_int32_t cindex)
{
  struct nsm_msg_uni msg;
  int len;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_uni));

  /* Check requested service.  */
  if (!nsm_service_check (nse, NSM_SERVICE_INTERFACE) ||
      (nse->service.protocol_id != APN_PROTO_ELMI))
    return RESULT_ERROR;

  if (!br_port)
    return RESULT_ERROR;

  /* Init Cindex. */
  msg.cindex = cindex;

  /* Interface index is mandatory.  */
  msg.ifindex = ifp->ifindex;

  /* UNI ID TLV.  */
  if (NSM_CHECK_CTYPE (msg.cindex, NSM_ELMI_CTYPE_UNI_ID))
    pal_strncpy (msg.uni_id, br_port->uni_name, NSM_UNI_NAME_LEN);

  if (NSM_SET_CTYPE(msg.cindex, NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE))
    msg.cevlan_evc_map_type = br_port->uni_ser_attr;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  /* Encode UNI service.  */
  len = nsm_encode_uni_msg (&nse->send.pnt, &nse->send.size, &msg);
  if (len < 0)
    return len;

  /* Send it to client.  */
  nsm_server_send_message (nse, 0, 0, NSM_MSG_ELMI_UNI_UPDATE, 0, len);

  return RESULT_OK;

}

/* Set NSM server callbacks. */
s_int32_t
nsm_elmi_set_server_callback (struct nsm_server *ns)
{
  /* CALL back for ELMI Operational status msg */
  nsm_server_set_callback (ns, NSM_MSG_ELMI_OPERATIONAL_STATE,
                           nsm_parse_elmi_status_msg,
                           nsm_elmi_status_msg_process);

  /* CALL back for ELMI auto vlan config msg */
  nsm_server_set_callback (ns, NSM_MSG_ELMI_AUTO_VLAN_ADD_PORT,
                           nsm_parse_elmi_vlan_msg,
                           nsm_elmi_recv_auto_vlan_add_msg);

  nsm_server_set_callback (ns, NSM_MSG_ELMI_AUTO_VLAN_DEL_PORT,
                           nsm_parse_elmi_vlan_msg,
                           nsm_elmi_recv_auto_vlan_del_msg);
  return RESULT_OK;
}

/* ELMI status process message */
s_int32_t
nsm_elmi_status_msg_process (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_msg_elmi_status *msg = message;
  struct nsm_master *nm = NULL;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  s_int32_t ret = PAL_FALSE;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (!nm)
    return RESULT_ERROR;

  ifp = if_lookup_by_index (&nm->vr->ifm, msg->ifindex);

  if (!ifp)
    return RESULT_OK;

  zif = (struct nsm_if *) ifp->info;

  if (! zif || ! zif->switchport)
    {
      return RESULT_ERROR;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return ret;
    }

  /* Update data structure for MEF-11 Integration */

  br_port->uni_type_status.elmi_status = msg->elmi_operational_status;

  /* Triggering uni type detect to evaluate uni type based on the status
   * received 
   */
  ret = nsm_uni_type_detect (ifp, br_port->uni_type_status.uni_type_mode);

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "ELMI Operational State changed to %s", 
               (msg->elmi_operational_status) ?  "UP" : "DOWN");

  return RESULT_OK;
}

s_int32_t
nsm_elmi_recv_auto_vlan_add_msg (struct nsm_msg_header *header,
                                 void *arg, void *message)
{

  int i;
  s_int32_t ret = 0;
  u_int16_t vid = 0;
  u_int16_t vlan_type = 0;
  struct interface *ifp = NULL;
  struct nsm_msg_vlan_port *msg = message;
  struct nsm_master *nm = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *vlan_table = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan vlan_info;
  char vlan_name[NSM_VLAN_NAME_MAX + 1];
  enum nsm_vlan_egress_type egress_tagged = NSM_VLAN_EGRESS_UNTAGGED;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return RESULT_ERROR ;

  ifp = if_lookup_by_index (&nm->vr->ifm, msg->ifindex);
  if (ifp == NULL)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return RESULT_ERROR;

  bridge = zif->bridge;

  if (bridge == NULL)
    return RESULT_ERROR;

  br_port = zif->switchport;

  if (br_port == NULL)
    return RESULT_ERROR;

  vlan_port = &(br_port->vlan_port);

  vlan_table = bridge->vlan_table;

  vlan_type = NSM_VLAN_AUTO | NSM_VLAN_CVLAN;

  for (i = 0; i < msg->num; i++)
    {
      /* Extract vid. */
      vid = NSM_VLAN_VID_MASK & msg->vid_info[i];

      /* Ignore invalid VLANs */
      if ((vid <= NSM_VLAN_DEFAULT_VID) || (vid > NSM_VLAN_MAX))
        continue;

      /* check if the vlan is added on the bridge */
      NSM_VLAN_SET(&vlan_info, vid);

      if (! vlan_table)
        {
          return NSM_VLAN_ERR_GENERAL;
        }

      rn = avl_search (vlan_table, (void *)&vlan_info);

      if (rn && (vlan = (struct nsm_vlan *)rn->info))
        {
          /* Check by any chance vlan is not active. */
          if (vlan->vlan_state != NSM_VLAN_ACTIVE) 
            {
              ret = nsm_vlan_add (bridge->master, bridge->name,
                                  vlan->vlan_name, vid, 
                                  NSM_VLAN_ACTIVE, vlan_type);
              if (ret < 0)
                {
                  continue; 
                }
            }
        }
      else
        {
          pal_snprintf(vlan_name, NSM_VLAN_NAME_MAX, "VLAN%04d", vid);

          /* Add the vlan to the bridge's vlan database */
          ret = nsm_vlan_add (bridge->master, bridge->name, vlan_name, vid,
                              NSM_VLAN_ACTIVE, vlan_type);
          if (ret < 0)
            {
              zlog_debug (NSM_ZG, "Addition of vlan %u to bridge Failed\n", 
                          vid);
              continue; 
            }
        }

      /* Now add the vlan to port
       * check if vlan is alreday on the port (double chk) 
       */   
      if (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid))) 
        { 
          /* Check for port modes & install vid according to port type */
          switch (vlan_port->mode) 
            {
            case NSM_VLAN_PORT_MODE_ACCESS:
                {
                  /* As of now, not supporting auto vlan support 
                   * on access port 
                   */
                  zlog_debug (NSM_ZG, "No need to add VLAN %u\n", vid);
                  break;
                }
            case NSM_VLAN_PORT_MODE_TRUNK:
            case NSM_VLAN_PORT_MODE_HYBRID:
                {
                  /* Check for Trunk port */
                  if (vlan_port->mode == NSM_VLAN_PORT_MODE_TRUNK)
                    {
                      egress_tagged = NSM_VLAN_EGRESS_TAGGED; 
                    }

                  nsm_vlan_add_common_port (ifp, vlan_port->mode, 
                                            vlan_port->mode, 
                                            vid, egress_tagged, 
                                            PAL_TRUE, PAL_TRUE); 
                  if (ret < 0)
                    {
                      zlog_debug (NSM_ZG, "Addition of VLAN %u "
                                  "to bridge-port failed\n", vid);
                      continue;
                    }

                  break;
                }
            default:
              break;
            }
        }
    }

  XFREE (MTYPE_TMP, msg->vid_info);

  return 0;
}

s_int32_t
nsm_elmi_recv_auto_vlan_del_msg (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  s_int32_t i = 0;
  s_int32_t ret = 0;
  u_int16_t vid = 0;
  u_int16_t vlan_type = 0;
  struct interface *ifp = NULL;
  struct nsm_msg_vlan_port *msg = message;
  struct nsm_master *nm = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_tree *vlan_table = NULL;
  struct avl_node *rn = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan vlan_info;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  ifp = if_lookup_by_index (&nm->vr->ifm, msg->ifindex);
  if (ifp == NULL)
    return 0;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return 0;

  bridge = zif->bridge;

  if (bridge == NULL)
    return 0;

  br_port = zif->switchport;

  if (br_port == NULL)
    return 0;

  vlan_port = &(br_port->vlan_port);

  vlan_table = bridge->vlan_table;

  vlan_type = NSM_VLAN_AUTO | NSM_VLAN_CVLAN;
  
  for (i = 0; i < msg->num; i++)
    {
      /* Extract vid. */
      vid = NSM_VLAN_VID_MASK & msg->vid_info[i];

      /* Ignore invalid VLANs */
      if ((vid <= NSM_VLAN_DEFAULT_VID) || (vid > NSM_VLAN_MAX))
        continue;

      /* check if the vlan is added on the bridge */
      NSM_VLAN_SET(&vlan_info, vid);

      if (! vlan_table)
        return NSM_VLAN_ERR_GENERAL;

      rn = avl_search (vlan_table, (void *)&vlan_info);

      if (rn && (vlan = (struct nsm_vlan *)rn->info))
        {
          /* Now delete the vlan from port
           * check if the vlan is there on the port (double chk) 
           */   

          if ((NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid))
              && (vlan->type & NSM_VLAN_AUTO)) 
            { 
              /* Check for port modes & install vid according to port type */
              switch (vlan_port->mode) 
                {
                case NSM_VLAN_PORT_MODE_ACCESS:
                    {
                      /* As of now, not supporting auto vlan support 
                       * on access port 
                       */
                      zlog_debug (NSM_ZG, "No need to add VLAN %u\n", vid);
                      break;
                    }
                case NSM_VLAN_PORT_MODE_TRUNK:
                case NSM_VLAN_PORT_MODE_HYBRID:
                    {
                      /* delete vlan from port. */
                      ret = nsm_vlan_delete_common_port (ifp, vlan_port->mode, 
                                                         vlan_port->mode, vid,
                                                         PAL_TRUE, PAL_TRUE);
                      if (ret < 0)
                        {
                          zlog_debug (NSM_ZG, "Deletion of port "
                                      "from vlan %u Failed\n", vid);
                        }
                      break;
                    }
                default:
                  break;
                }
            }

          /* Now check if any other port is part of this vlan,
           * then vlan will not be removed from the bridge
           */
          ret = nsm_check_auto_vlan_membership (bridge, vid);
          if (ret == PAL_FALSE)
            {
              ret = nsm_vlan_delete (bridge->master, bridge->name,
                                     vid, vlan_type);

              if (ret < 0)
                {
                  zlog_debug (NSM_ZG, "Deletion of vlan %u Failed\n", vid);
                  continue; 
                }
            }
        }
    }

  XFREE (MTYPE_TMP, msg->vid_info);

  return 0;
}

/* Send message to ELMI */

/* NEW CEVLAN/EVC Added */
s_int32_t
nsm_elmi_send_evc_add (struct nsm_bridge *bridge, 
                       struct interface *ifp, 
                       u_int16_t svid, 
                       struct nsm_svlan_reg_info *svlan_info)
{
  struct nsm_msg_elmi_evc msg;
  struct avl_tree *vlan_table = NULL;
  struct nsm_vlan *vlan = NULL;
  struct nsm_vlan vlan_info;
  struct avl_node *rn = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL; 
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_pro_edge_info *pro_edge_port = NULL;
  int ret = 0;
  s_int32_t count = 0;
  u_int16_t vid = 0;

  /* Send EVC Information to ELMI Protocol module. */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_elmi_evc));

  vlan_table = bridge->svlan_table; 

  if (! vlan_table)
    return NSM_VLAN_ERR_GENERAL;

  zif = ifp->info;

  if (!zif || !zif->switchport)
    return NSM_VLAN_ERR_GENERAL;

  br_port = zif->switchport;

  /* Add vlan to hardware */
  NSM_VLAN_SET (&vlan_info, svid);

  rn = avl_search (vlan_table, (void *)&vlan_info);

  if (rn)
    {
      vlan = rn->info;

      if (!vlan)
        return NSM_VLAN_ERR_GENERAL; 
    }  
  else
    return RESULT_ERROR;

  vlan_port = &br_port->vlan_port;
  if (!vlan_port)
    return RESULT_ERROR;

  if (!svlan_info)
    return RESULT_ERROR;

  msg.cindex = 0;

  msg.ifindex = ifp->ifindex;
  msg.evc_ref_id = svid;

  msg.evc_status_type = NSM_EVC_STATUS_NEW_AND_NOTACTIVE;
  NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_EVC_STATUS_TYPE);

  if (vlan->type & NSM_SVLAN_M2M)
    msg.evc_type = NSM_ELMI_SVLAN_M2M;
  else
    msg.evc_type = NSM_ELMI_SVLAN_P2P;

  NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_EVC_TYPE);

  if (vlan->evc_id)
    pal_strncpy (msg.evc_id, vlan->evc_id, NSM_EVC_NAME_LEN);

  NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_EVC_ID_STR);

  NSM_SET_CTYPE(msg.cindex, NSM_ELMI_CTYPE_EVC_CVLAN_BITMAPS);

  /* Get count of the vids. */
  count = 0;
  NSM_VLAN_SET_BMP_ITER_BEGIN (svlan_info->cvlanMemberBmp, vid)
    {
      count++;
    }
  NSM_VLAN_SET_BMP_ITER_END (svlan_info->cvlanMemberBmp, vid);

  if (count > 0)
    {
      msg.num = count;
      msg.vid_info = XCALLOC (MTYPE_TMP, sizeof(u_int16_t) * msg.num);

      count = 0;

      NSM_VLAN_SET_BMP_ITER_BEGIN (svlan_info->cvlanMemberBmp, vid)
        {
          msg.vid_info[count] = vid;
          count++;
        }
      NSM_VLAN_SET_BMP_ITER_END (svlan_info->cvlanMemberBmp, vid);
    }

  NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_UNTAGGED_PRI);
  if ((pro_edge_port = nsm_reg_tab_svlan_info_port_lookup
        (svlan_info, ifp)))
    {
      if (NSM_VLAN_BMP_IS_MEMBER (svlan_info->cvlanMemberBmp,
            pro_edge_port->untagged_vid))
        {
          if (br_port->uni_ser_attr == NSM_UNI_BNDL)
            msg.untagged_frame = PAL_TRUE;
          else
            msg.untagged_frame = PAL_FALSE;
        }
      else
        msg.untagged_frame = PAL_FALSE;
    }

  NSM_SET_CTYPE (msg.cindex, NSM_ELMI_CTYPE_DEFAULT_EVC);
  if (vlan_port->uni_default_evc == svid)
    msg.default_evc = PAL_TRUE;
  else
    msg.default_evc = PAL_FALSE;

  /* Send EVC add message to PM. */
  ret = nsm_evc_send_evc_add_msg (&msg, NSM_MSG_ELMI_EVC_NEW);

  return ret;

}

/* Interface delete information.  */
void
nsm_server_evc_delete (struct interface *ifp, u_int16_t evc_ref_id)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i = 0;

  if (ifp->ifindex == 0)
    return;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: EVC %s delete", ifp->name);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_VLAN) &&
        (nse->service.protocol_id == APN_PROTO_ELMI))
      {
        /* Send link delete information.  */
        nsm_server_send_evc_delete (nse, ifp, evc_ref_id);
      }

}

/* Interface update information.  */
void
nsm_server_evc_update (struct interface *ifp, u_int16_t evc_id, 
                       struct nsm_svlan_reg_info *svlan_info, cindex_t cindex)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  u_int32_t i = 0;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_VLAN) &&
        (nse->service.protocol_id == APN_PROTO_ELMI))
      {
        nsm_server_send_evc_update (nse, ifp, evc_id, 
                                    svlan_info, cindex);
      }

}

/* Send UNI Add message */
s_int32_t
nsm_elmi_send_uni_add (struct nsm_bridge * bridge, struct interface *ifp)
{
  struct nsm_msg_uni msg;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port  *br_port = NULL;
  int ret = 0;

  zif = (struct nsm_if *) ifp->info;
  ret = NSM_L2_ERR_NONE;

  if (!zif || !zif->switchport)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      return ret;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      ret = NSM_L2_ERR_INVALID_ARG;
      return ret;
    }

  /* Send UNI Information to ELMI Protocol module. */

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_uni));

  msg.cindex = 0;

  msg.ifindex = ifp->ifindex;

  pal_strncpy (msg.uni_id, br_port->uni_name, NSM_UNI_NAME_LEN);
  NSM_SET_CTYPE(msg.cindex, NSM_ELMI_CTYPE_UNI_ID);

  msg.cevlan_evc_map_type = br_port->uni_ser_attr;
  NSM_SET_CTYPE(msg.cindex, NSM_ELMI_CTYPE_UNI_CVLAN_MAP_TYPE);

  /* Send EVC add message to PM. */
  ret = nsm_send_uni_add_msg (bridge, &msg, NSM_MSG_ELMI_UNI_ADD);

  return ret;

}

/* Interface delete information.  */
void
nsm_server_uni_delete (struct interface *ifp)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i = 0;

  if (ifp->ifindex == 0)
    return;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "UPDATE: EVC %s delete", ifp->name);

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_INTERFACE) && 
        (nse->service.protocol_id == APN_PROTO_ELMI))
      {
        /* Send link delete information.  */
        nsm_server_send_uni_delete (nse, ifp);
      }

}

/* UNI update information.  */
void
nsm_server_uni_update (struct nsm_bridge_port *br_port, struct interface *ifp,
                       u_int32_t cindex)
{
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  s_int32_t i = 0;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (nsm_service_check (nse, NSM_SERVICE_INTERFACE))
      nsm_server_send_uni_update (nse, br_port, ifp, cindex);
}

s_int32_t
nsm_server_elmi_send_bw (struct nsm_bridge_port *br_port, 
                         struct nsm_msg_bw_profile *msg, int msgid)
{
  s_int32_t nbytes = 0;
  struct nsm_server_client *nsc = NULL;
  struct nsm_server_entry *nse = NULL;
  s_int32_t i = 0;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN) && 
        (nse->service.protocol_id == APN_PROTO_ELMI))
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM bw message. */
        nbytes = nsm_encode_elmi_bw_msg (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bw profile add/delete message. */
        nsm_server_send_message (nse, 0, 0, msgid, 0, nbytes);
      }

  return 0;
}

#endif /* HAVE_ELMID  */
