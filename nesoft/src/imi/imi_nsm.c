/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "nsm_client.h"

#include "imi/imi.h"
#include "imi/imi_interface.h"
#include "imi_server.h"
#ifdef HAVE_DHCP_CLIENT
#include "imi/util/imi_dhcp.h"
#endif /* HAVE_DHCP_CLIENT. */
#ifdef HAVE_NAT
#include "imi/util/imi_fw.h"
#include "imi/util/imi_vs.h"
#endif /* HAVE_NAT. */

#ifdef HAVE_ACL
#include "imi/util/imi_filter.h"
#endif
#ifdef HAVE_CRX
#include "crx.h"
#endif /* HAVE_CRX */

#include "imi_api.h"

/* Temporary placeholder. */
#define         IS_DEBUG_IMI(x,y)       0
#define         IS_DEBUG_NSM            0

int
imi_vr_add_callback (struct apn_vr *vr)
{
  struct imi_master *im;

  if (vr->proto == NULL)
    imi_master_init (vr);

  if (! (im = vr->proto))
    return -1;

  /* Mark it as it has been added by NSM. */
  im->im_nsm_add_flag = PAL_TRUE;

#ifdef HAVE_VLOGD
  /* We do mot check return code as this call may fail, if te VLOGD is not
     running. Once it shows up, we will send all VRs one by one.
   */
  imi_vlog_send_vr_instance_cmd(vr->name, vr->id, PAL_TRUE);
#endif
  return 0;
}

int
imi_vr_delete_callback (struct apn_vr *vr)
{
  struct imi_master *im = vr->proto;

  if (! (im = vr->proto))
    return 0;

#ifdef HAVE_VLOGD
  /* Only VRs "added" by NSM are sent to VLOGD. */
  if (im->im_nsm_add_flag)
    imi_vlog_send_vr_instance_cmd(vr->name, vr->id, PAL_FALSE);
#endif
  im->im_nsm_add_flag = PAL_FALSE;
  imi_master_finish (vr);
  return 0;
}

int
imi_if_vr_bind_callback (struct interface *ifp)
{
#ifdef HAVE_DHCP_CLIENT
  struct imi_interface *imi_if = ifp->info;

  /* Is DHCP Client suspended on this interface? */
  if (imi_if && imi_if->idc && imi_if->idc->suspended)
    imi_dhcpc_resume (ifp);
#endif /* HAVE_DHCP_CLIENT */

#ifdef HAVE_VRX
  if (ifp->vrf->id == EVR_VRF_ID)
    {
      struct host *host = vr_lookup_by_vrid (imim, ifp->vrf->id);

      /* Update Host localsrc information. */
      if (host)
        {
          if (ifp->local_flag)
            host->localifindex = ifp->ifindex;
          else if (host->localifindex == ifp->ifindex)
            host->localifindex = -1;
        }
    }
#endif /* HAVE_VRX */

#ifdef HAVE_NAT
  {
    /* Install NAT rules in the kernel. */
    struct imi_interface *imi_if = ifp->info;
    if (imi_if != NULL)
      {
        imi_if_process_nat_static (ifp, imi_if->scope, 1);
        imi_if_process_nat_pool_list (ifp, imi_if->scope, 1);
      }
  }
#endif /* HAVE_NAT */
#ifdef HAVE_ACL
  if (ifp->vr && IS_APN_VR_PRIVILEGED(ifp->vr))
    /* Install all filters. */
    imi_filter_process_if(ifp, 1);
#endif /* HAVE_ACL */
  return 0;
}

int
imi_if_vr_unbind_callback (struct interface *ifp)
{
#ifdef HAVE_VRX
  if (ifp->vrf->id == EVR_VRF_ID)
    {
      struct host *host = vr_lookup_by_vrid (imim, ifp->vrf->id);

      /* Update Host localsrc information. */
      if (host && host->localifindex == ifp->ifindex)
        host->localifindex = -1;
    }
#endif /* HAVE_VRX */

#ifdef HAVE_NAT
  {
    /* Delete NAT rules from kernel. Do not change interface NAT config. */
    imi_if_process_nat_pool_list (ifp, NSM_IF_SCOPE_UNSPEC, 0);

    imi_if_process_nat_static (ifp, NSM_IF_SCOPE_UNSPEC, 0);
  }
#endif /* HAVE_NAT */
#ifdef HAVE_ACL
  if (ifp->vr && IS_APN_VR_PRIVILEGED(ifp->vr))
    /* Remove all filters. */
    imi_filter_process_if(ifp, 0);
#endif /* HAVE_ACL */
  return 0;
}

int
imi_if_delete_callback (struct interface *ifp)
{

  struct interface *new_ifp;

  if (imi_server_check_index (imim, ifp, NULL,
                              INTERFACE_MODE) == PAL_TRUE)
    {
       new_ifp = ifg_get_by_name (&imim->ifg, ifp->name);

       if (new_ifp == NULL)
         return 0;

       imi_server_update_index (imim, ifp, NULL,
                                INTERFACE_MODE, new_ifp, NULL);
    }

  return 0;
}

#ifdef HAVE_NAT
int
imi_if_addr_add_callback (struct connected *ifc)
{
  imi_virtual_server_if_address_add (ifc);

#ifdef HAVE_DHCP_SERVER
  imi_dhcps_refresh (ifc->ifp);
#endif /* HAVE_DHCP_SERVER */

  return 0;
}

int
imi_if_addr_delete_callback (struct connected *ifc)
{
  imi_virtual_server_if_address_delete (ifc);

#ifdef HAVE_DHCP_SERVER
  imi_dhcps_refresh (ifc->ifp);
#endif /* HAVE_DHCP_SERVER */

  return 0;
}
#endif /* HAVE_NAT */


/* Service reply is received.  If client has special request to NSM,
   we should check this reply.  */
int
imi_nsm_recv_service (struct nsm_msg_header *header,
                      void *arg, void *message)
{
  struct nsm_client *nc;
  struct nsm_client_handler *nch;
  struct nsm_msg_service *service;

  nch = arg;
  nc = nch->nc;
  service = message;

  /* NSM server assign client ID.  */
  nsm_client_id_set (nc, service);

  if (nc->debug)
    nsm_service_dump (nc->zg, service);

  /* Check protocol version.  */
  if (service->version < NSM_PROTOCOL_VERSION_1)
    {
      zlog_err (nc->zg, "NSM server protocol version error");
      return -1;
    }

  /* Check NSM service bits.  */
  if ((service->bits & nc->service.bits) != nc->service.bits)
    {
      zlog_err (nc->zg, "NSM service is not sufficient");
      return -1;
    }

  /* Now clent is up.  */
  nch->up = 1;

  /* If this is route service request connection, we might send redistribute
     request in the future.  When we support VRF/VR we have to send each
     VRF/VR's redistribute information.  */
  return 0;
}

/* Interface up/down message from NSM.  */
int
imi_nsm_recv_link_up (struct nsm_msg_header *header,
                      void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_NAME))
    ifp = ifg_lookup_by_name (&imim->ifg, msg->ifname);
  else
    ifp = ifg_lookup_by_index (&imim->ifg, msg->ifindex);

  if (ifp == NULL)
    return 0;

  /* Drop the unbind interface message.  */
  if (ifp->vr == NULL)
    return 0;

  /* Interface is already up. */
  if (if_is_up (ifp))
    {
      struct interface if_tmp;

      /* Temporarily keep ifp values. */
      pal_mem_cpy (&if_tmp, ifp, sizeof (struct interface));

      nsm_util_interface_state (msg, &imim->ifg);

      if (IS_DEBUG_IMI (nsm, NSM_INTERFACE))
        zlog_info (imim, "PacOS[%s]: Link state is up", ifp->name);

      if (if_tmp.bandwidth != ifp->bandwidth)
        if (IS_DEBUG_IMI (nsm, NSM_INTERFACE))
          zlog_info (imim, "PacOS[%s]: Link bandwidth changed %d -> %d",
                     ifp->name, if_tmp.bandwidth, ifp->bandwidth);

      return 0;
    }

  ifp = nsm_util_interface_state (msg, &imim->ifg);
  if (ifp == NULL)
    return 0;

 #ifdef HAVE_DHCP_SERVER
  imi_dhcps_refresh (ifp);
 #endif /* HAVE_DHCP_SERVER */

  if (IS_DEBUG_IMI (nsm, NSM_INTERFACE))
    zlog_info (imim, "PacOS[%s]: Link state changes to up", ifp->name);

  return 0;
}

int
imi_nsm_recv_link_down (struct nsm_msg_header *header,
                        void *arg, void *message)
{
  struct nsm_client_handler *nch = arg;
  struct nsm_client *nc = nch->nc;
  struct nsm_msg_link *msg = message;
  struct interface *ifp;

  if (nc->debug)
    nsm_interface_dump (nc->zg, msg);

  if (NSM_CHECK_CTYPE (msg->cindex, NSM_LINK_CTYPE_NAME))
    ifp = ifg_lookup_by_name (&imim->ifg, msg->ifname);
  else
    ifp = ifg_lookup_by_index (&imim->ifg, msg->ifindex);

  if (ifp == NULL)
    return 0;

  /* Drop the unbind interface message.  */
  if (ifp->vr == NULL)
    return 0;

  ifp = nsm_util_interface_state (msg, &imim->ifg);
  if (ifp == NULL)
    return 0;

  if (IS_DEBUG_IMI (nsm, NSM_INTERFACE))
    zlog_info (imim, "PacOS[%s]: Link state changes to down.", ifp->name);

  return 0;
}

/* IMI NSM client initialization.  */
void
imi_nsm_init (struct lib_globals *zg)
{
  /* Create NSM client.  */
  zg->nc = nsm_client_create (imim, IS_DEBUG_NSM);
  if (zg->nc == NULL)
    return;

  /* Set version and protocol.  */
  nsm_client_set_version (zg->nc, NSM_PROTOCOL_VERSION_1);
  nsm_client_set_protocol (zg->nc, APN_PROTO_IMI);

  /* Set required services. */
  nsm_client_set_service (zg->nc, NSM_SERVICE_INTERFACE);

  /* Register for Router_ID Updates */
  nsm_client_set_service (zg->nc, NSM_SERVICE_ROUTER_ID);

  /* Register for VRF service to receive all interface updates. */
  nsm_client_set_service (zg->nc, NSM_SERVICE_VRF);

  /* NSM message callbacks. */
  nsm_client_set_callback (zg->nc, NSM_MSG_SERVICE_REPLY,
                           imi_nsm_recv_service);
  nsm_client_set_callback (zg->nc, NSM_MSG_LINK_UP, imi_nsm_recv_link_up);
  nsm_client_set_callback (zg->nc, NSM_MSG_LINK_DOWN, imi_nsm_recv_link_down);

  /* VR callbacks.  */
  apn_vr_add_callback (zg, VR_CALLBACK_ADD, imi_vr_add_callback);
  apn_vr_add_callback (zg, VR_CALLBACK_ADD_UNCHG, imi_vr_add_callback);
  apn_vr_add_callback (zg, VR_CALLBACK_DELETE, imi_vr_delete_callback);

  /* Interface callbacks.  */
  if_add_hook (&zg->ifg, IF_CALLBACK_VR_BIND, imi_if_vr_bind_callback);
  if_add_hook (&zg->ifg, IF_CALLBACK_VR_UNBIND, imi_if_vr_unbind_callback);

  if_add_hook (&zg->ifg, IF_CALLBACK_DELETE, imi_if_delete_callback);

#ifdef HAVE_NAT
  ifc_add_hook (&zg->ifg, IFC_CALLBACK_ADDR_ADD, imi_if_addr_add_callback);
  ifc_add_hook (&zg->ifg, IFC_CALLBACK_ADDR_DELETE,
                imi_if_addr_delete_callback);
#endif /* HAVE_NAT */

#ifdef HAVE_VRX
  imi_vrx_nsm_init (zg->nc);
#endif /* HAVE_VRX */

#ifdef HAVE_CRX
  crx_nsm_init (zg->nc);
#endif /* HAVE_CRX */

  /* Start NSM processing.  */
  nsm_client_start (zg->nc);
}
