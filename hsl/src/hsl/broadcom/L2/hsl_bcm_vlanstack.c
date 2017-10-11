/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
                                                                                
/*
   Broadcom includes.
*/
#include "bcm_incl.h"
                                                                                
/*
  HAL includes.
*/
#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#if defined HAVE_PROVIDER_BRIDGE || defined HAVE_VLAN_STACK

#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl_error.h"
#include "hsl_avl.h"
#include "hsl.h"
#include "hsl_ifmgr.h"
#include "hsl_bridge.h"
#include "hsl_vlan.h"
#include "hsl_logger.h"
#include "hsl_if_hw.h"
#include "hsl_bridge.h"
#include "hsl_vlan.h"
#include "hsl_bcm_if.h"

#ifdef HAVE_PROVIDER_BRIDGE

static int
_hsl_vlan_reg_tab_cmp (void *param1, void *param2)
{
  struct hsl_vlan_regis_key *reg1 = (struct hsl_vlan_regis_key *) param1;
  struct hsl_vlan_regis_key *reg2 = (struct hsl_vlan_regis_key *) param2;

  /* Less than. */
  if (reg1->cvid < reg2->cvid)
    return -1;

  /* Greater than. */
  if (reg1->cvid > reg2->cvid)
    return 1;

  /* Equals to. */
  return 0;
}

#endif /* HAVE_PROVIDER_BRIDGE */

int
hsl_bcm_vlan_stacking_enable (u_int32_t ifindex, u_int16_t tpid, int mode)
{
  struct hsl_bcm_if *bcmifp = NULL;
  struct hsl_if *ifp = NULL;
  bcmx_lport_t lport;
  int bcm_dtag_mode;
  int ret;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return -1;

  bcmifp = ifp->system_info;
  if (! bcmifp)
  {
    HSL_IFMGR_IF_REF_DEC (ifp);
    return -1;
  }
  lport = bcmifp->u.l2.lport;   
  HSL_IFMGR_IF_REF_DEC (ifp);

  /* set default protocol tag id */
  ret = bcmx_port_tpid_set (lport, tpid);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to set default tag protocol id on interface %d, "
               "bcm error = %d\n", ifindex, ret);

      return ret;
    }  
  
  switch (mode)
    {
      case HAL_VLAN_STACK_MODE_NONE:
        bcm_dtag_mode = BCM_PORT_DTAG_MODE_NONE;
       break;
      case HAL_VLAN_STACK_MODE_INTERNAL:
        bcm_dtag_mode = BCM_PORT_DTAG_MODE_INTERNAL;
       break;
      case HAL_VLAN_STACK_MODE_EXTERNAL:
        bcm_dtag_mode = BCM_PORT_DTAG_MODE_EXTERNAL;
       break;
      default:
        return -1;
       break;
    }
  /* enable vlan stacking */
  ret = bcmx_port_dtag_mode_set (lport, bcm_dtag_mode);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to enable vlan stacking on interface %d, "
               "bcm error = %d\n", ifindex, ret);

      return ret;
    }
  
  /* Set mtu to HSL_8021Q_MAX_LEN for dtag frames */
  ret = bcmx_port_frame_max_set(lport, HSL_8021Q_MAX_LEN);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to set mtu %d on interface %d, "
               "bcm error = %d\n", HSL_8021Q_MAX_LEN, ifindex, ret);
      return ret;
    }

  return 0;
}

int
hsl_bcm_provider_vlan_disable (u_int32_t ifindex)
{
  int ret;
  struct hsl_if *ifp = NULL;
  struct hsl_bcm_if *bcmifp = NULL;
  bcmx_lport_t lport;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return -1;

  bcmifp = ifp->system_info;
  if (! bcmifp)
  {
     HSL_IFMGR_IF_REF_DEC (ifp);
     return -1;
  }
  lport = bcmifp->u.l2.lport;   

  HSL_IFMGR_IF_REF_DEC (ifp);

  /* set default protocol tag id */
  ret = bcmx_port_tpid_set (lport, 0x8100);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to set default tag protocol id on interface %d, "
               "bcm error = %d\n", ifindex, ret);

      return ret;
    }


  /* disable vlan stacking */
  ret = bcmx_port_dtag_mode_set (lport, BCM_PORT_DTAG_MODE_NONE);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to disable vlan stacking on interface %d, "
               "bcm error = %d\n", ifindex, ret);

      return ret;
    }

  /* Set mtu back to HSL_ETHER_MAX_LEN */
  ret = bcmx_port_frame_max_set(lport, HSL_ETHER_MAX_LEN);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to set mtu %d on interface %d, "
               "bcm error = %d\n", HSL_ETHER_MAX_LEN, ifindex, ret);
      return ret;
    }

  return 0;
}

#ifdef HAVE_PROVIDER_BRIDGE

int hsl_bcm_cvlan_reg_tab_add (u_int32_t ifindex, u_int16_t cvid,
                               u_int16_t svid)
{
  s_int32_t ret;
  bcmx_lport_t lport;
  struct hsl_if *ifp = NULL;
  struct hsl_port_vlan *port_vlan;
  struct hsl_bcm_if *bcmifp = NULL;
  struct hsl_bridge_port *port = NULL;
  struct hsl_vlan_regis_key *key = NULL;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    return -1;

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
	return -1;
    }

  port = ifp->u.l2_ethernet.port;

  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
	return -1;
    }

  port_vlan = port->vlan;

  if (! port_vlan)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
	return -1;
    }

  bcmifp = ifp->system_info;

  if (! bcmifp)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
	return -1;
    }

  lport = bcmifp->u.l2.lport;

  port = ifp->u.l2_ethernet.port;

  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      return -1;
    }

  HSL_IFMGR_IF_REF_DEC (ifp);

  if (port->reg_tab == NULL)
    {
      ret = hsl_avl_create (&port->reg_tab, 0, _hsl_vlan_reg_tab_cmp);

      if (ret < 0)
        return -1;
    }

  key = hsl_vlan_regis_key_init ();

  if (key == NULL)
    return -1;

  hsl_avl_insert (port->reg_tab, key);

  bcmx_vlan_dtag_add (lport, cvid, svid, 0);

  return 0;
}

int hsl_bcm_cvlan_reg_tab_del (u_int32_t ifindex, u_int16_t cvid,
                               u_int16_t svid)
{
  bcmx_lport_t lport;
  struct hsl_if *ifp = NULL;
  struct hsl_avl_node *node = NULL;
  struct hsl_bcm_if *bcmifp = NULL;
  struct hsl_vlan_regis_key tmp_key;
  struct hsl_bridge_port *port = NULL;
  struct hsl_vlan_regis_key *key = NULL;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  memset (&tmp_key, 0, sizeof (struct hsl_vlan_regis_key));

  if (! ifp)
    return -1;

  bcmifp = ifp->system_info;

  if (! bcmifp)
  {
     HSL_IFMGR_IF_REF_DEC (ifp);
     return -1;
  }

  lport = bcmifp->u.l2.lport;

  port = ifp->u.l2_ethernet.port;

  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      return -1;
    }

  HSL_IFMGR_IF_REF_DEC (ifp);

  tmp_key.cvid = cvid;

  node = hsl_avl_lookup (port->reg_tab, &tmp_key);

  if (node == NULL
      || (key = HSL_AVL_NODE_INFO (node)))
    return -1;

  hsl_avl_delete (port->reg_tab, key);

  hsl_vlan_regis_key_deinit (key);

  bcmx_vlan_dtag_delete (lport, cvid);

  return 0;

}

int
hsl_bcm_protocol_process (u_int32_t ifindex, hal_l2_proto_t proto,
                          hal_l2_proto_process_t proto_process,
                          hsl_vid_t vid)

{
  struct hsl_if *ifp = NULL;
  struct hsl_bridge_port *port;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    return -1;

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      return -1;
    }

  port = ifp->u.l2_ethernet.port;

  if (! port)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      return -1;
    }

  HSL_IFMGR_IF_REF_DEC (ifp);

  port->proto_process.process [proto] = proto_process;
  port->proto_process.tun_vid [proto] = vid;

  return 0;

}

#endif /* HAVE_PROVIDER_BRIDGE */

int
hsl_bcm_vlan_set_provider_port_type (u_int32_t ifindex,
                                     enum hal_vlan_port_type port_type)
{
  int ret;
  struct hsl_if *ifp = NULL;
  struct hsl_bcm_if *bcmifp = NULL;
  bcmx_lport_t lport;
  int bcm_dtag_mode;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return -1;

  bcmifp = ifp->system_info;

  if (! bcmifp)
  {
    HSL_IFMGR_IF_REF_DEC (ifp);
    return -1;
  }

  lport = bcmifp->u.l2.lport;   
  HSL_IFMGR_IF_REF_DEC (ifp);

  /* set default protocol tag id */
  ret = bcmx_port_tpid_set (lport, HSL_L2_ETH_P_8021Q_STAG);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to set default tag protocol id on interface %d, "
               "bcm error = %d\n", ifindex, ret);

      return ret;
    }

  /* set default protocol inner tag id */
  ret = bcmx_port_inner_tpid_set (lport, HSL_L2_ETH_P_8021Q);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to set default inner tag protocol id on interface %d, "
               "bcm error = %d\n", ifindex, ret);

      return ret;
    }

  switch (port_type)
    {
      case HAL_VLAN_PROVIDER_CE_PORT:
        bcm_dtag_mode = BCM_PORT_DTAG_MODE_EXTERNAL;
       break;
      case HAL_VLAN_PROVIDER_CN_PORT:
      case HAL_VLAN_PROVIDER_PN_PORT:
        bcm_dtag_mode = BCM_PORT_DTAG_MODE_INTERNAL;
       break;
      default:
        return -1;
       break;
    }

  /* enable vlan stacking */
  ret = bcmx_port_dtag_mode_set (lport, bcm_dtag_mode);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to enable vlan stacking on interface %d, "
               "bcm error = %d\n", ifindex, ret);

      return ret;
    }

  /* Set mtu to HSL_8021Q_MAX_LEN for dtag frames */
  ret = bcmx_port_frame_max_set(lport, HSL_8021Q_MAX_LEN);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to set mtu %d on interface %d, "
               "bcm error = %d\n", HSL_8021Q_MAX_LEN, ifindex, ret);
      return ret;
    }

  return 0;
}

#endif /* HAVE_PROVIDER_BRIGDE || HAVE_VLAN_STACK */
