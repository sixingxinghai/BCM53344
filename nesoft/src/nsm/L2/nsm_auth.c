/* Copyright (C) 2004  All Rights Reserved.  */

#include "pal.h"

#ifdef HAVE_AUTHD

#include "hal_incl.h"

#include "message.h"
#include "nsm_message.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_debug.h"
#include "nsm/nsm_server.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_interface.h"
#include "nsm/L2/nsm_vlan_cli.h"

#ifdef HAVE_L2
#include "nsm_bridge.h"
#include "nsm_interface.h"
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD
#include "nsm_oam.h"
#endif /* HAVE_ONMD */

#define NSM_AUTH_PORT_HAL_STATE(M)                                            \
    ((M)->ctrl == NSM_AUTH_PORT_CTRL_UNCONTROLLED                             \
     ? HAL_AUTH_PORT_STATE_UNCONTROLLED :                                     \
     ((M)->state == NSM_AUTH_PORT_STATE_AUTHORIZED                            \
      ? HAL_AUTH_PORT_STATE_UNBLOCK :                                         \
      ((M)->ctrl == NSM_AUTH_PORT_CTRL_DIR_BOTH                               \
     ? HAL_AUTH_PORT_STATE_BLOCK_INOUT : HAL_AUTH_PORT_STATE_BLOCK_IN)))

#ifdef HAVE_MAC_AUTH
/* String to ether address conversion */
static void
auth_mac_etherStrToAddr (char *str, char *p)
{
 #define HEX_TO_DIGIT(x)  (((x) >= 'A') ? ((x) - 'A' + 10) : ((x) - '0'))
 s_int32_t i, j = 0;
 char c;
 u_char b1, b2;

  for (i = 0; i < 6; i++)
    {
      c = str[j++];
      if (c >= 'a' && c <= 'f')
        c &= 0x0DF;           /* Force to Uppercase. */
      b1 = HEX_TO_DIGIT(c);
      c = str[j++];
      if (c >= 'a' && c <= 'f')
        c &= 0x0DF;           /* Force to Uppercase. */
      b2 = HEX_TO_DIGIT(c);
      p[i] = b1 * 16 + b2;
      j++;             /* Skip a colon. */
    }
}
#endif /* HAVE_MAC_AUTH */

int
nsm_auth_recv_port_state_msg (struct nsm_msg_header *header,
                              void *arg, void *message)
{
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_auth_port_state *msg = message;
  struct nsm_master *nm;
  struct interface *ifp;
#ifdef HAVE_L2
  struct nsm_if *zif = NULL;
#endif /* HAVE_L2 */

  if (IS_NSM_DEBUG_RECV)
    nsm_auth_port_state_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm == NULL)
    return 0;

  ifp = if_lookup_by_index (&nm->vr->ifm, msg->ifindex);

  if (ifp == NULL)
    return 0;

  if (msg->state == NSM_AUTH_PORT_ENABLE
      || msg->state == NSM_AUTH_PORT_DISABLE)
    {
      /* This is the notification from API and hence we do not
       * need to set the state in hardware. The actual states
       * will be set when state machine is run.
       */

#ifdef HAVE_ONMD
      nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_DOT1X,
                                         msg->state == NSM_AUTH_PORT_ENABLE ?
                                         PAL_TRUE:PAL_FALSE);
#endif /* HAVE_ONMD */

      return 0;

    }

#ifdef HAVE_HAL
  /* Set the port state.  */
  hal_auth_set_port_state (msg->ifindex, NSM_AUTH_PORT_HAL_STATE (msg));
#endif /* HAVE_HAL */

#ifdef HAVE_L2
  if (INTF_TYPE_L2 (ifp))
    {
      zif = (struct nsm_if *)ifp->info;

      if ((msg->ctrl == NSM_AUTH_PORT_CTRL_DIR_BOTH)
          || (msg->ctrl == NSM_AUTH_PORT_CTRL_DIR_IN))
        {
          /* Flush fdb */
          nsm_bridge_flush_fdb_by_port (nm, ifp->name);

          /* Delete all static entries */
          if (zif && zif->bridge)
            nsm_bridge_delete_all_fdb_by_port (zif->bridge, zif,
                                               PAL_TRUE);
        }

      if ((msg->ctrl == NSM_AUTH_PORT_CTRL_UNCONTROLLED)
          || (msg->state == NSM_AUTH_PORT_STATE_AUTHORIZED))
        {
          /* Add the static entry */
          if (zif && zif->bridge)
            nsm_bridge_add_static_fdb (zif->bridge, ifp->ifindex);
        }
    }
#endif /* HAVE_L2 */

  return 0;
}

#ifdef HAVE_MAC_AUTH
/* Receive port auth-mac state message */
s_int32_t
nsm_auth_mac_recv_port_state_msg (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_msg_auth_mac_port_state *msg = message;
  struct nsm_master *nm;
  struct interface *ifp;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge;
  u_int16_t vid;

  nsm_auth_mac_port_state_dump (ns->zg, msg);

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

#ifdef HAVE_L2
  if (INTF_TYPE_L2 (ifp))
    {
      /* Flush fdb */
      if ((msg->state == NSM_MACAUTH_PORT_STATE_ENABLED))
        {  
          nsm_bridge_flush_fdb_by_port (nm, ifp->name);
          hal_auth_mac_set_port_state (msg->ifindex, msg->mode,
                                       HAL_MACAUTH_PORT_STATE_ENABLED); 
        }
      else
        {
          hal_auth_mac_set_port_state (msg->ifindex, msg->mode,
                                       HAL_MACAUTH_PORT_STATE_DISABLED);

          NSM_VLAN_SET_BMP_ITER_BEGIN (vlan_port->authmacMemberBmp, vid)
            {
              /* If the vlan is used by AUTH MAC only then unset the vlan */
              if (! NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, vid)
                  && ! NSM_VLAN_BMP_IS_MEMBER (vlan_port->dynamicMemberBmp,
                                               vid)) 
                {
                  if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_ACCESS)
                    {
                      nsm_vlan_set_access_port (ifp, NSM_VLAN_DEFAULT_VID,
                                                PAL_TRUE, PAL_TRUE);
                    }
                  else
                    {
                      if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_TRUNK)
                         nsm_vlan_delete_trunk_port (ifp, vid, PAL_TRUE,
                                                     PAL_TRUE);
                      else if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_HYBRID)
                         nsm_vlan_delete_hybrid_port (ifp, vid,
                                                      PAL_TRUE, PAL_TRUE);
                    }
                }

              NSM_VLAN_BMP_UNSET (vlan_port->authmacMemberBmp, vid);
            }
          NSM_VLAN_SET_BMP_ITER_END (vlan_port->authmacMemberBmp, vid);
        }

        hal_auth_mac_set_port_state (msg->ifindex, msg->mode,
                                     HAL_MACAUTH_PORT_STATE_DISABLED);
    }
#endif /* HAVE_L2 */

  return 0;
}

/* Receive mac authentication status message */
s_int32_t
nsm_auth_mac_recv_auth_status_msg (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  struct nsm_msg_auth_mac_status *msg = message;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *port = NULL;
  struct nsm_master *nm;
  struct interface *ifp;
  struct nsm_if *zif;
  s_int32_t ret = 0;

  char ether_addr[6];

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  ifp = if_lookup_by_index (&nm->vr->ifm, msg->ifindex);

  if (ifp == NULL)
    return 0;
  
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL)
    return 0;

  br_port = zif->switchport;

  if (br_port == NULL)
    return 0;

  port = &(br_port->vlan_port);

#ifdef HAVE_L2
  if (INTF_TYPE_L2 (ifp))
    {
      auth_mac_etherStrToAddr (msg->mac_addr, ether_addr);   

      /* Flush fdb */
      if (msg->status == NSM_AUTH_MAC_AUTH_ACCEPT)
        {
          if (msg->dynamic_vlan_creation != NSM_AUTH_DYNAMIC_VLAN_DISABLE)
            {
              if (msg->radius_dynamic_vid != 1)
                {
                  
                      NSM_VLAN_BMP_SET (port->authmacMemberBmp,
                                        msg->radius_dynamic_vid);
                  if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_ACCESS)
                    ret =  nsm_vlan_set_access_port (ifp,
                                                     msg->radius_dynamic_vid,
                                                     PAL_TRUE, PAL_TRUE);
                  else
                    { 
                      if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_TRUNK)
                        ret = nsm_vlan_add_trunk_port (ifp,
                                                       msg->radius_dynamic_vid,
                                                       PAL_TRUE, PAL_TRUE);
                      else if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_HYBRID)
                        ret = nsm_vlan_add_hybrid_port (ifp,
                                                        msg->radius_dynamic_vid,
                                                        PAL_TRUE, PAL_TRUE,
                                                        PAL_TRUE);
                    }
                  
                  if (ret < 0)
                    {
                      if (ret == NSM_VLAN_ERR_IFP_NOT_BOUND)
                        msg->status = NSM_AUTH_MAC_AUTH_INVALID;
                      else if (ret == NSM_VLAN_ERR_VLAN_NOT_FOUND)
                        msg->status = NSM_AUTH_MAC_AUTH_REJECT;
                      
                      NSM_VLAN_BMP_UNSET (port->authmacMemberBmp,
                                          msg->radius_dynamic_vid);
                    }
                  else
                    {
                      NSM_VLAN_BMP_SET (port->authmacMemberBmp,
                                        msg->radius_dynamic_vid);
                    }
                }
            }
          else
            {
              msg->radius_dynamic_vid = br_port->vlan_port.pvid;
            }
        }

      if (msg->status == NSM_AUTH_MAC_AUTH_ACCEPT) 
        {
          if (msg->mac_address_aging == NSM_AUTH_MAC_AGING_DISABLE)
            nsm_bridge_add_mac (nm->bridge, nm->bridge->bridge_list->name,
                                ifp, ether_addr, msg->radius_dynamic_vid,
                                msg->radius_dynamic_vid,
                                HAL_L2_FDB_STATIC, PAL_TRUE, AUTH_MAC);
          else 
            nsm_bridge_add_mac (nm->bridge, nm->bridge->bridge_list->name,
                                ifp, ether_addr, msg->radius_dynamic_vid,
                                msg->radius_dynamic_vid,
                                HAL_L2_FDB_DYNAMIC, PAL_TRUE, AUTH_MAC);
       }
      else if (msg->status == NSM_AUTH_MAC_AUTH_REJECT)
        {
          if (msg->auth_fail_action == NSM_AUTH_MAC_RESTRICT_VLAN)
            {
              if (msg->auth_fail_restrict_vid != 1)
                {
                  if (br_port->vlan_port.mode ==
                      NSM_VLAN_PORT_MODE_ACCESS)
                    ret = nsm_vlan_set_access_port (ifp,
                                                    msg->auth_fail_restrict_vid,
                                                    PAL_TRUE, PAL_TRUE);
                  else
                    { 
                      if (br_port->vlan_port.mode == NSM_VLAN_PORT_MODE_TRUNK)
                        ret = nsm_vlan_add_trunk_port (ifp,
                                                       msg->auth_fail_restrict_vid,
                                                       PAL_TRUE, PAL_TRUE);
                      else if (br_port->vlan_port.mode ==
                               NSM_VLAN_PORT_MODE_HYBRID)
                        ret = nsm_vlan_set_hybrid_port (ifp,
                                                        msg->auth_fail_restrict_vid,
                                                        PAL_TRUE, PAL_TRUE); 
                    }
                  if (ret < 0)
                    msg->status = NSM_AUTH_MAC_AUTH_FAIL_INVLAID;
                  else
                    NSM_VLAN_BMP_SET (port->authmacMemberBmp,
                                      msg->auth_fail_restrict_vid);
                }
              if (msg->status == NSM_AUTH_MAC_AUTH_REJECT)
                {
                  if (msg->mac_address_aging == NSM_AUTH_MAC_AGING_DISABLE)
                    {
                      nsm_bridge_add_mac (nm->bridge,
                                          nm->bridge->bridge_list->name,
                                          ifp, ether_addr,
                                          msg->auth_fail_restrict_vid,
                                          msg->auth_fail_restrict_vid,
                                          HAL_L2_FDB_STATIC, PAL_TRUE,
                                          AUTH_MAC);
                    }
                  else
                    nsm_bridge_add_mac (nm->bridge,
                                        nm->bridge->bridge_list->name,
                                        ifp, ether_addr,
                                        msg->auth_fail_restrict_vid,
                                        msg->auth_fail_restrict_vid,
                                        HAL_L2_FDB_DYNAMIC, PAL_TRUE,
                                        AUTH_MAC);
                }
            }
          else
            {
              if (msg->mac_address_aging == NSM_AUTH_MAC_AGING_DISABLE)
                nsm_bridge_add_mac (nm->bridge, nm->bridge->bridge_list->name,
                                    ifp, ether_addr, msg->auth_fail_restrict_vid,
                                    msg->auth_fail_restrict_vid,
                                    HAL_L2_FDB_STATIC, PAL_FALSE, AUTH_MAC);
              else
                nsm_bridge_add_mac (nm->bridge, nm->bridge->bridge_list->name,
                                    ifp, ether_addr, msg->auth_fail_restrict_vid,
                                    msg->auth_fail_restrict_vid,
                                    HAL_L2_FDB_DYNAMIC, PAL_FALSE, AUTH_MAC);  
            }
        }
    }
#endif /* HAVE_L2 */

  return 0;
}
#endif
int
nsm_auth_set_server_callback (struct nsm_server *ns)
{
  nsm_server_set_callback (ns, NSM_MSG_AUTH_PORT_STATE,
                           nsm_parse_auth_port_state_msg,
                           nsm_auth_recv_port_state_msg);
#ifdef HAVE_MAC_AUTH
  nsm_server_set_callback (ns, NSM_MSG_MACAUTH_PORT_STATE,
                           nsm_parse_auth_mac_port_state_msg,
                           nsm_auth_mac_recv_port_state_msg);

  nsm_server_set_callback (ns, NSM_MSG_AUTH_MAC_AUTH_STATUS,
                           nsm_parse_auth_mac_status_msg,
                           nsm_auth_mac_recv_auth_status_msg);
#endif

  return 0;
}

#endif /* HAVE_AUTHD */
