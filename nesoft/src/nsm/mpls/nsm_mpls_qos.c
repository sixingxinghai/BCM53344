/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_TE

#include "if.h"
#include "mpls.h"
#include "qos_common.h"
#include "nsm_message.h"

#include "nsmd.h"
#include "nsm_interface.h"
#include "nsm_qos_serv.h"
#include "rib/nsm_redistribute.h"
#include "nsm_server.h"

/* Handler for max reservable bandwidth */
void
nsm_qos_send_max_bw_update (struct interface *ifp)
{
  cindex_t cindex = 0;

  if (! ifp)
    return;

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BANDWIDTH);

  /* Send update to protocols */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);
}


/* Handler for max reservable bandwidth */
void
nsm_qos_send_max_resv_bw_update (struct interface *ifp)
{
  cindex_t cindex = 0;

  if (! ifp)
    return;
  
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_MAX_RESV_BANDWIDTH);
  
  /* Send update to protocols */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);
}


#ifdef HAVE_DSTE
/* Handler for bandwidth constraint */
void
nsm_qos_send_bw_constraint_update (struct interface *ifp)
{
  cindex_t cindex = 0;

  if (! ifp)
    return;
  
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_BW_CONSTRAINT);
  
  /* Send update to protocols */
  if (if_is_running (ifp))
    nsm_server_if_up_update (ifp, cindex);
  else
    nsm_server_if_down_update (ifp, cindex);
}
#endif /* HAVE_DSTE */


/* Initialize interface in QOS module */
void
nsm_qos_init_interface (struct interface *ifp)
{
  struct nsm_if *zif;
 
  zif = (struct nsm_if *)ifp->info;
 
  if ((! if_is_loopback (ifp)) && 
      (zif != NULL) && (zif->qos_if == NULL))
    zif->qos_if = nsm_qos_serv_if_init (ifp);
}

/* Deinitialize interface in QOS module */
void
nsm_qos_deinit_interface (struct interface *ifp, bool_t _delete,
                          bool_t remove_mpls_rib, bool_t send_update)
{
  if (! if_is_loopback (ifp))
    {
      nsm_qos_serv_if_deinit (ifp, _delete, remove_mpls_rib, send_update);
      if (_delete == NSM_TRUE)
        ifp->max_resv_bw = 0;
    }
}


void
nsm_qos_preempt_resource (struct nsm_msg_qos_preempt *msg)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i, nbytes;
  u_int32_t vr_id = 0;
  vrf_id_t vrf_id = 0;
  
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_QOS_PREEMPT)
        && msg->protocol_id == nse->service.protocol_id)
      {
        /* Encode QOS message */
        nbytes = nsm_encode_qos_preempt (&nse->send.pnt, &nse->send.size, msg);
        if (nbytes < 0)
          return;
            
        /* Send it to client. */
        nsm_server_send_message (nse, vr_id, vrf_id,
                                 NSM_MSG_QOS_CLIENT_PREEMPT, 0,nbytes);
      }
}

#endif /* HAVE_TE */
