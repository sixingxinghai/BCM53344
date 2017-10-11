/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#include "bcm_incl.h"

#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#ifdef HAVE_AUTHD

#include "hsl_error.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_bcm_if.h"
#include "hsl_bridge.h"

#define BCMX_RV_REPLACE_OK(tmp_rv, rv, ok_rv) \
        if ((tmp_rv) < 0 && (tmp_rv) != (ok_rv)) (rv) = (tmp_rv)

static int hsl_bcm_auth_initialized = HSL_FALSE;
 
static struct hsl_if *
hsl_bcm_auth_get_l2_port (u_int32_t port_ifindex)
{
  struct hsl_if *ifp;
  struct hsl_if *ifp2 = NULL;

  /* get port data */
  ifp = hsl_ifmgr_lookup_by_index (port_ifindex);
  if (ifp)
    {
      if ( ifp->type == HSL_IF_TYPE_L2_ETHERNET )
        return ifp;
      else if ( ifp->type == HSL_IF_TYPE_IP )
        {
          ifp2 = hsl_ifmgr_get_first_L2_port (ifp);
          HSL_IFMGR_IF_REF_DEC (ifp);
          return ifp2;
        }
    } 
  return NULL;
}


int
hsl_bcm_auth_init ()
{
  int ret = BCM_E_NONE, tmp_ret;
  int i, bcm_unit;

  HSL_FN_ENTER ();

  if (hsl_bcm_auth_initialized)
    HSL_FN_EXIT (0);

  /* initialize the bcm security module */
  BCMX_UNIT_ITER (bcm_unit, i) 
    {
      if (BCM_IS_FABRIC (bcm_unit))
        {
          continue;
        }
      
      tmp_ret = bcm_auth_init (bcm_unit);
      BCMX_RV_REPLACE_OK (tmp_ret, ret, BCM_E_UNAVAIL);
    }

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to initialize the security module, bcm error = %d\n", 
               ret);
      return HSL_IFMGR_ERR_HW_FAILURE;
    }

   hsl_bcm_auth_initialized = HSL_TRUE;

  HSL_FN_EXIT (0);
}


int
hsl_bcm_auth_deinit ()
{
  int ret = BCM_E_NONE, tmp_ret;
  int i, bcm_unit;

  HSL_FN_ENTER ();

  if (! hsl_bcm_auth_initialized)
    HSL_FN_EXIT (-1);

  /* uninitialize the bcm security module */
  BCMX_UNIT_ITER (bcm_unit, i) 
    {
      if (BCM_IS_FABRIC (bcm_unit))
        {
          continue;
        }
      
      tmp_ret = bcm_auth_detach (bcm_unit);
      BCMX_RV_REPLACE_OK (tmp_ret, ret, BCM_E_UNAVAIL);
    }

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to uninitialize the security module, bcm error = %d\n",
               ret);
      HSL_FN_EXIT (HSL_IFMGR_ERR_HW_FAILURE);
    }

  hsl_bcm_auth_initialized = HSL_FALSE;

  HSL_FN_EXIT (ret);
}



int
hsl_bcm_auth_set_port_state (u_int32_t port_ifindex,
                             u_int32_t port_state)
{
  struct hsl_bcm_if *sysinfo;
  bcmx_lport_t lport = BCMX_LPORT_INVALID; 
  struct hsl_if *ifp = NULL;
  struct hsl_if *ifpl2 = NULL;
  u_int32_t mode = 0;
  int ret = 0;
   
  /* get lport */
  ifpl2 = hsl_bcm_auth_get_l2_port (port_ifindex);
  if (!ifpl2)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Port %d not found\n", port_ifindex);
      return HSL_IFMGR_ERR_NO_HW_PORT;
    }

  sysinfo = (struct hsl_bcm_if *)ifpl2->system_info;
  lport = sysinfo->u.l2.lport; 
  if (lport == BCMX_LPORT_INVALID)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "LPort %d not found\n", port_ifindex);
      HSL_IFMGR_IF_REF_DEC (ifpl2);
      return HSL_IFMGR_ERR_NO_HW_PORT;
    }

  switch (port_state)
    {
    case HAL_AUTH_PORT_STATE_BLOCK_INOUT:
      mode = BCM_AUTH_MODE_UNAUTH|BCM_AUTH_BLOCK_INOUT;
      break;
    case HAL_AUTH_PORT_STATE_BLOCK_IN:
      mode = BCM_AUTH_MODE_UNAUTH|BCM_AUTH_BLOCK_IN;
      break;
    case HAL_AUTH_PORT_STATE_UNBLOCK:
      mode = BCM_AUTH_MODE_AUTH|BCM_AUTH_LEARN;
      break;
    case HAL_AUTH_PORT_STATE_UNCONTROLLED:
      mode = BCM_AUTH_MODE_UNCONTROLLED;
      break;
    default:
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Invalid auth state msg %d for port %d\n", 
               port_state, port_ifindex);
      return HSL_IFMGR_ERR_INVALID_8021x_PORT_STATE;
      break;
    }

  /* set auth mode in bcm */
  ret = bcmx_auth_mode_set (lport, mode);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to set auth state for port %d, bcm error = %d\n", 
               port_ifindex, ret);
      HSL_IFMGR_IF_REF_DEC (ifpl2);
      return HSL_IFMGR_ERR_HW_FAILURE;
    }

  if ((mode & BCM_AUTH_BLOCK_INOUT) || (mode & BCM_AUTH_BLOCK_IN))
    {
      hsl_ifmgr_unset_acceptable_packet_types (ifpl2, HSL_IF_PKT_ALL);
      hsl_ifmgr_set_acceptable_packet_types (ifpl2, HSL_IF_PKT_EAPOL);
    }
  else
    {
      ifp = hsl_ifmgr_lookup_by_index (port_ifindex);
      if (ifp)
        {
          if ( ifp->type == HSL_IF_TYPE_L2_ETHERNET )
            {
              struct hsl_if *ifp_parent = NULL;
              struct hsl_if_list *node = NULL;

              hsl_ifmgr_unset_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);
              hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_L2);

              if (ifp->parent_list)
                {
                  node = ifp->parent_list;
                  while (node)
                    {
                      ifp_parent = node->ifp;
                      if (ifp_parent && ifp_parent->type == HSL_IF_TYPE_IP)
                        {
                          hsl_ifmgr_set_acceptable_packet_types (ifp, 
                                                              HSL_IF_PKT_ALL);
                          break;
                        }
                      node = node->next;
                    }
                }
            }
          else if ( ifp->type == HSL_IF_TYPE_IP )
            {
              hsl_ifmgr_unset_acceptable_packet_types (ifpl2, HSL_IF_PKT_ALL);
              hsl_ifmgr_set_acceptable_packet_types (ifpl2, HSL_IF_PKT_ARP |
                  HSL_IF_PKT_RARP | HSL_IF_PKT_BCAST | HSL_IF_PKT_MCAST |
                  HSL_IF_PKT_IP | HSL_IF_PKT_LACP | HSL_IF_PKT_EAPOL);
            }
          HSL_IFMGR_IF_REF_DEC (ifp);
        }
    }
  
  HSL_IFMGR_IF_REF_DEC (ifpl2);
  return 0;
}

#ifdef HAVE_MAC_AUTH
/* Set port auth-mac state */
int
hsl_bcm_auth_mac_set_port_state (u_int32_t port_ifindex,
                                 u_int32_t port_state)
{
   bcmx_lport_t lport = BCMX_LPORT_INVALID; 
   struct hsl_bridge_port *port;
   struct hsl_bcm_if *bcmifp;
   struct hsl_if *ifp = NULL;
   int ret = 0;
    
   /* get lport */
   ifp = hsl_bcm_auth_get_l2_port (port_ifindex);
   if (ifp == NULL)
     {
       HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
                "Port %d not found\n", port_ifindex);
       return HSL_IFMGR_ERR_NO_HW_PORT;
     }
 
   bcmifp = (struct hsl_bcm_if *)ifp->system_info;
   if (bcmifp == NULL)
     {
       HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                "BCM Info for port %d not found\n", port_ifindex);
       HSL_IFMGR_IF_REF_DEC (ifp);
       return HSL_IFMGR_ERR_HW_FAILURE;
     }

   lport = bcmifp->u.l2.lport; 
   if (lport == BCMX_LPORT_INVALID)
     {
       HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
                "LPort %d not found\n", port_ifindex);
       HSL_IFMGR_IF_REF_DEC (ifp);
       return HSL_IFMGR_ERR_NO_HW_PORT;
     }
 
   port = ifp->u.l2_ethernet.port;
   if (port == NULL)
     {
       HSL_IFMGR_IF_REF_DEC (ifp);
       return HSL_ERR_BRIDGE_PORT_NOT_EXISTS;
     }

    switch (port_state)
      {
        case HAL_MACAUTH_PORT_STATE_ENABLED:
          /* Unknown src mac will be sent to CPU, no learning or flooding*/
          ret = bcmx_port_learn_modify (lport, BCM_PORT_LEARN_CPU,
                                        (BCM_PORT_LEARN_ARL
                                         | BCM_PORT_LEARN_FWD)); 
          if (ret != BCM_E_NONE)
            {
              HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                       "Failed to set port learn state for port %d, \
                        bcm error = %d\n", port_ifindex, ret);
              HSL_IFMGR_IF_REF_DEC (ifp);
              return HSL_IFMGR_ERR_HW_FAILURE;
            }

          port->auth_mac_port_ctrl |= AUTH_MAC_ENABLE;
          break;

        case HAL_MACAUTH_PORT_STATE_DISABLED:
#ifdef HAVE_L2LERN
          ret = bcmx_port_learn_modify (lport, (BCM_PORT_LEARN_ARL
                                        | BCM_PORT_LEARN_FWD), 0);
#else
          ret = bcmx_port_learn_modify (lport, (BCM_PORT_LEARN_ARL
                                        | BCM_PORT_LEARN_FWD),
                                        BCM_PORT_LEARN_CPU);
#endif /* HAVE_L2LERN */
 
          if (ret != BCM_E_NONE)
            {
              HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                       "Failed to set port learn state for port %d, \
                        bcm error = %d\n", port_ifindex, ret);
              HSL_IFMGR_IF_REF_DEC (ifp);
              return HSL_IFMGR_ERR_HW_FAILURE;
            }
          port->auth_mac_port_ctrl &= ~AUTH_MAC_ENABLE;
          break;

        default:
          break; 
      }
 
   HSL_IFMGR_IF_REF_DEC (ifp);
   return 0;
}
#endif /* HAVE_MAC_AUTH */

#endif /* HAVE_AUTHD */
