/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "stream.h"
#include "log.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_fea.h"
#include "nsm/vrrp/vrrp.h"
#include "nsm_ifma.h"

/* Static error logging function to catch PAL error types.  */
void
vrrp_fe_log_error (result_t ret)
{
  char *str;

  /* What error? */
  str = NULL;
  switch (ret)
  {
    case EVRRP:
      str = "VRRP Error: Unknown failure.\n";
      break;
    case EVRRP_SOCK_OPEN:
      str = "VRRP Error: Opening socket failed.\n";
      break;
    case EVRRP_SOCK_BIND:
      str = "VRRP Error: Binding socket failed.\n";
      break;
    case EVRRP_SOCK_SEND:
      str = "VRRP Error: Sending using socket failed.\n";
      break;
    case EVRRP_MAC_SET:
      str = "VRRP Error: Setting Mac address failed.\n";
      break;
    case EVRRP_MAC_UNSET:
      str = "VRRP Error: Unsetting / restoring Mac address failed.\n";
     break;
    case EVRRP_GARP_SEND:
      str = "VRRP Error: Sending gratuitous ARP failed.\n";
      break;
    default:
      return;
  }

  zlog_err (NSM_ZG, "%s", str);
  return;
}

/*
 * ------------------------------------------------------
 * API Calls - called by VRRP to interact with the system
 * ------------------------------------------------------
 */

/*------------------------------------------------------------------------
 * vrrp_fe_init - called by VRRP to perform any initialization or
 *              registration with the forwarding engine application.
 *              It will call all appropriate FE APIs.
 *------------------------------------------------------------------------*/
int
vrrp_fe_init (struct vrrp_global *vrrp)
{
  struct nsm_master *nm = vrrp->nm;
  result_t ret;

  /* Currently we only support VRRP for PVR. */
  if (nm->vr->id != 0)
    return VRRP_OK;

  /* Call PAL to start VRRP dataplane code. */
  ret = nsm_fea_vrrp_start (NSM_ZG);

  /* Return or log error. */
  if (ret == RESULT_OK)
    return VRRP_OK;
  else
  {
    vrrp_fe_log_error (ret);
    return VRRP_FAILURE;
  }
}

/*------------------------------------------------------------------------
 * vrrp_fe_tx_grat_arp - called by VRRP to send a gratuitous ARP message
 *              via the forwarding engine app.  This API call will create
 *              the message if necessary, and call the appropriate
 *              FE APIs.
 *------------------------------------------------------------------------*/
int
vrrp_fe_tx_grat_arp (struct stream *ap,
                     struct interface *ifp)
{
  result_t ret;

  /* Call PAL to send gratuitous arp. */
  ret = nsm_fea_gratuitous_arp_send (NSM_ZG, ap, ifp);

  /* Return or log error. */
  if (ret == RESULT_OK)
    return VRRP_OK;
  else
  {
    vrrp_fe_log_error (ret);
    return VRRP_FAILURE;
  }
}

#ifdef HAVE_IPV6

int
vrrp_fe_tx_nd_advt  (struct stream *ap,
                     struct interface *ifp)
{
  result_t ret;

  /* Call PAL to send neighbor advertisement */
  ret = nsm_fea_nd_neighbor_adv_send (NSM_ZG, ap, ifp);

  /* Return or log error. */
  if (ret == RESULT_OK)
    return VRRP_OK;
  else
  {
    vrrp_fe_log_error (ret);
    return VRRP_FAILURE;
  }
}

#endif /* HAVE_IPV6 */


/*------------------------------------------------------------------------
 * vrrp_fe_state_change () - called by VRRP to perform any layer 2
 *              manipulation when a VRRP session changes state.
 *------------------------------------------------------------------------*/
int
vrrp_fe_state_change (VRRP_SESSION *sess,
                      vrrp_state_t curr_state,
                      vrrp_state_t new_state,
                      bool_t owner)
{
  int vrid;
  struct interface *ifp;
  struct pal_in4_addr *vip;
#ifdef HAVE_IPV6
  struct pal_in6_addr *vip6;
#endif /* HAVE_IPV6 */
  u_int8_t *vmac;
  result_t ret1 = RESULT_OK;
  result_t ret2 = RESULT_OK;
  s_int32_t vmac_len;

  /* Extract session information needed for state change */
  vrid= sess->vrid;
  ifp = sess->ifp;
  vip = &sess->vip_v4;
#ifdef HAVE_IPV6
  vip6 = &sess->vip_v6;
#endif /* HAVE_IPV6 */
  vmac    = sess->vmac.v.mbyte;
  vmac_len= sizeof(sess->vmac.v.mbyte);

  switch (curr_state) {
    case VRRP_STATE_INIT:
        if (new_state == VRRP_STATE_BACKUP)
          return VRRP_OK;
        if (new_state == VRRP_STATE_MASTER)
        {
          if (vrrp_fe_get_vmac_status())
            ret1 = nsm_ifma_set_virtual(ifp->info, vmac, vmac_len);

          if (sess->af_type == AF_INET)
              ret2 = nsm_fea_vrrp_virtual_ipv4_add (NSM_ZG, vip, ifp, owner,
                                                    vrid);
#ifdef HAVE_IPV6
          else if (sess->af_type == AF_INET6)
              ret2 = nsm_fea_vrrp_virtual_ipv6_add (NSM_ZG, vip6, ifp, owner,
                                                   vrid);
#endif /* HAVE_IPV6 */
        }
        break;
    case VRRP_STATE_BACKUP:
        if (new_state == VRRP_STATE_INIT)
          return VRRP_OK;
        if (new_state == VRRP_STATE_MASTER)
        {
          if (vrrp_fe_get_vmac_status())
            ret1 = nsm_ifma_set_virtual(ifp->info, vmac, vmac_len);

          if (sess->af_type == AF_INET)
              ret2 = nsm_fea_vrrp_virtual_ipv4_add (NSM_ZG, vip, ifp, owner,
                                                    vrid);
#ifdef HAVE_IPV6
          else if (sess->af_type == AF_INET6)
              ret2 = nsm_fea_vrrp_virtual_ipv6_add (NSM_ZG, vip6, ifp, owner,
                                                    vrid);
#endif /* HAVE_IPV6 */
        }
        break;
    case VRRP_STATE_MASTER:
        if (new_state == VRRP_STATE_INIT || new_state == VRRP_STATE_BACKUP)
          {
            if (vrrp_fe_get_vmac_status())
              ret1 = nsm_ifma_del_virtual(ifp->info, vmac, vmac_len);

            if (sess->af_type == AF_INET)
                ret2 = nsm_fea_vrrp_virtual_ipv4_delete (NSM_ZG, vip, ifp,
                                                         owner, vrid);
#ifdef HAVE_IPV6
            else if (sess->af_type == AF_INET6)
                ret2 = nsm_fea_vrrp_virtual_ipv6_delete (NSM_ZG, vip6, ifp, owner,
                                                         vrid);
#endif /* HAVE_IPV6 */
         }
      else
        {
#ifdef HAVE_VRRP_LINK_ADDR
          if (! sess->link_addr_deleted)
            {
              sess->link_addr_deleted = PAL_TRUE;
              if (sess->af_type == AF_INET)
                ret2 = nsm_fea_vrrp_virtual_ipv4_delete (NSM_ZG, vip, ifp,
                                                         owner, vrid);
#ifdef HAVE_IPV6
              else
                ret2 = nsm_fea_vrrp_virtual_ipv6_delete (NSM_ZG, vip6, ifp,
                                                         owner, vrid);
#endif /* HAVE_IPV6 */
              sess->link_addr_deleted = PAL_FALSE;
            }
#endif /* HAVE_VRRP_LINK_ADDR */
        }
        break;
    default:
        return VRRP_FAILURE;
        break;
  }

  /* Catch and log any errors. */
  if (ret1 != RESULT_OK)
  {
    vrrp_fe_log_error (ret1);
    return VRRP_FAILURE;
  }
  if (ret2 != RESULT_OK)
  {
    vrrp_fe_log_error (ret2);
    return VRRP_FAILURE;
  }
  return VRRP_OK;
}

int vrrp_fe_get_vmac_status ()
{
  return nsm_fea_vrrp_get_vmac_status (NSM_ZG);
}

int vrrp_fe_set_vmac_status (int vmac_status)
{
  return nsm_fea_vrrp_set_vmac_status (NSM_ZG, vmac_status);
}

#ifdef HAVE_VRRP_LINK_ADDR
/* Delete/add back virtual IP address.  This function is used during address
   delete to ensure the virtual IP is NOT the Primary IP address. */
int
vrrp_fe_vip_refresh (struct interface *ifp,
                     VRRP_SESSION *sess)
{
  int ret;

  ret = VRRP_OK;

  if (!sess->ifp || sess->ifp != ifp)
    return VRRP_OK;

  /* Remove the virtual IP. */
  sess->link_addr_deleted = PAL_TRUE;
  if (sess->af_type == AF_INET)
    ret = nsm_fea_vrrp_virtual_ipv4_delete (NSM_ZG, &sess->vip_v4, ifp,
                                            sess->owner, sess->vrid);
#ifdef HAVE_IPV6
  else
    ret = nsm_fea_vrrp_virtual_ipv6_delete (NSM_ZG, &sess->vip_v6, ifp,
                                            sess->owner, sess->vrid);
#endif /* HAVE_IPV6 */
  if (ret != RESULT_OK)
    vrrp_fe_log_error (ret);

  sess->link_addr_deleted = PAL_FALSE;

  /* Add it back. */
  if (sess->af_type == AF_INET)
    ret = nsm_fea_vrrp_virtual_ipv4_add (NSM_ZG, &sess->vip_v4, ifp,
                                         sess->owner, sess->vrid);
#ifdef HAVE_IPV6
  else
    ret = nsm_fea_vrrp_virtual_ipv6_add (NSM_ZG, &sess->vip_v6, ifp,
                                         sess->owner, sess->vrid);
#endif /* HAVE_IPV6 */
  if (ret != RESULT_OK)
    vrrp_fe_log_error (ret);

  return VRRP_OK;
}
#endif /* HAVE_VRRP_LINK_ADDR */

