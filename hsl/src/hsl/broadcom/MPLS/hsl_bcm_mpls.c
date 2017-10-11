/* Copyright (C) 2005  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_ether.h"
#include "hsl_fib.h"
#include "hsl_ifmgr.h"
#include "hsl_logs.h"
#include "hsl_table.h"
#include "hal_mpls_types.h"

#include "hal_msg.h"
#include "bcm_incl.h"
#include "hsl_bcm_resv_vlan.h"
#include "hsl_bcm_if.h"
#include "hsl_mpls.h"
#include "hsl_bcm_mpls.h"
#include "hsl_bcm.h"



static struct hsl_mpls_hw_callbacks hsl_bcm_mpls_callbacks;
struct hsl_bcm_resv_vlan *bcm_mpls_vlan = NULL;
struct hsl_bcm_resv_vlan *hsl_bcm_vpn_vlans[HSL_BCM_MPLS_VPN_MAX + 1];


int
hsl_hw_mpls_init ()
{
  int ret;

  ret = bcmx_mpls_init ();
  if (ret != BCM_E_NONE)
    return ret;

  ret = hsl_bcm_resv_vlan_allocate (&bcm_mpls_vlan);
  if (ret < 0)
    goto err_ret;

  memset (hsl_bcm_vpn_vlans, 0, sizeof (hsl_bcm_vpn_vlans)); 

  ret = hsl_bcm_router_alert_filter_install ();
  if (ret < 0)
    goto err_ret;

  return 0;

 err_ret:
  hsl_hw_mpls_deinit ();
  return -1;
}

int 
hsl_hw_mpls_deinit ()
{
  if (bcm_mpls_vlan)
    {
      hsl_bcm_resv_vlan_deallocate (bcm_mpls_vlan);
      bcm_mpls_vlan = NULL;
    }

  memset (hsl_bcm_vpn_vlans, 0, sizeof (hsl_bcm_vpn_vlans)); 
  
  bcmx_mpls_cleanup ();
  hsl_bcm_router_alert_filter_uninstall ();
  return 0;
}


int
hsl_bcm_mpls_ftn_add_to_hw (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  bcm_l3_intf_t intf;
  bcm_mpls_l3_initiator_t label_info;
  struct hal_msg_mpls_ftn_add *fa;
  struct hsl_bcm_if *bcmifp;
  struct hsl_prefix_entry *pe;
  int ret;

  pe = rnp->info;

  fa = nh->ifp->u.mpls.label_info;
  if (! fa)
    return -1;

  ret = bcmx_mpls_l3_initiator_t_init (&label_info);
  if (ret != BCM_E_NONE)
    return ret;
  
  label_info.ttl1 = HSL_MPLS_DEFAULT_TTL_VALUE;  /* Tunnel label TTL */
  label_info.ttl2 = HSL_MPLS_DEFAULT_TTL_VALUE;  /* VC label TTL */

  if (fa->opcode == HAL_MPLS_PUSH)
    {
      label_info.flags |= BCM_MPLS_INSERT_LABEL1;
      label_info.label1 = fa->tunnel_label;
    }
  else 
    {
      label_info.flags |= BCM_MPLS_INSERT_LABEL1;
      label_info.label1 = fa->tunnel_label;

      if (fa->opcode == HAL_MPLS_PUSH_AND_LOOKUP) 
        {
          label_info.flags |= BCM_MPLS_INSERT_LABEL2;
          label_info.label2 = fa->vpn_label;
        }
    }

  bcmifp = (struct hsl_bcm_if *)nh->ifp->system_info;

  if (! FLAG_ISSET (bcmifp->u.mpls.flags, HSL_BCM_IF_FLAG_TUNNEL_INSTALLED))
    {
      memcpy (label_info.dmac, nh->mac, HSL_ETHER_ALEN);

      /* Initialize interface. */
      bcmx_l3_intf_t_init (&intf);

      intf.l3a_intf_id = bcmifp->u.mpls.ifindex;

      ret = bcmx_mpls_l3_initiator_set (&intf, &label_info);
      if (ret != BCM_E_NONE)
        {
          return -1;
        }

      SET_FLAG (bcmifp->u.mpls.flags, HSL_BCM_IF_FLAG_TUNNEL_INSTALLED);
    }

  ret = hsl_bcm_prefix_add (HSL_DEFAULT_FIB, rnp, nh);
  return ret;
}


int
hsl_bcm_mpls_ftn_del_from_hw (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  bcm_l3_intf_t intf;
  struct hsl_bcm_if *bcmifp;
  int ret;

  bcmifp = (struct hsl_bcm_if *)nh->ifp->system_info;
  if (FLAG_ISSET (bcmifp->u.mpls.flags, HSL_BCM_IF_FLAG_TUNNEL_INSTALLED))
    {
      /* Initialize interface. */
      bcmx_l3_intf_t_init (&intf);
      intf.l3a_intf_id = bcmifp->u.mpls.ifindex;
      
      bcmx_mpls_l3_initiator_clear (&intf);

      UNSET_FLAG (bcmifp->u.mpls.flags, HSL_BCM_IF_FLAG_TUNNEL_INSTALLED);
    }

  ret = hsl_bcm_prefix_delete (HSL_DEFAULT_FIB, rnp, nh);
  return ret;
}



int
hsl_bcm_mpls_ilm_add_to_hw (struct hsl_mpls_ilm_entry *ilm, struct hsl_nh_entry *nh)
{
  bcmx_mpls_switch_t msw;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
  u_int16_t mpls_vid;
  u_char mpls_mac[HSL_ETHER_ALEN];
  int ret;

  ret = bcmx_mpls_switch_t_init (&msw);
  if (ret != BCM_E_NONE)
    return ret;

  msw.src_label1 = ilm->in_label;
  msw.flags = BCM_MPLS_ONE_LABEL_LOOKUP;
  
  bcmifp = ilm->in_ifp->system_info;
  if (! bcmifp)
    return -1;
  
  msw.src_l3_intf = bcmifp->u.l3.ifindex; 

  mpls_vid = bcmifp->u.l3.vid;
  memcpy (mpls_mac, bcmifp->u.l3.mac, HSL_ETHER_ALEN);

  ifp = hsl_ifmgr_get_first_L2_port (ilm->in_ifp);
  if (! ifp)
    {
      return -1;
    }

  HSL_IFMGR_IF_REF_DEC (ifp);
  
  /* Get the lport. */
  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      return -1;
    }

  msw.src_lport = bcmifp->u.l2.lport;
  

  if (nh && nh->ifp)
    {
      memcpy (msw.next_mac, nh->mac, HSL_ETHER_ALEN);
      msw.next_vlan = nh->ifp->u.ip.vid;

      bcmifp = nh->ifp->system_info;
      if (! bcmifp)
        return -1;
      
      /* Set L3 interface. */
      msw.next_intf = bcmifp->u.l3.ifindex; 

      ifp = hsl_ifmgr_get_first_L2_port (nh->ifp);
      if (! ifp)
        {
          return -1;
        }

      HSL_IFMGR_IF_REF_DEC (ifp);
  
      /* Get the lport. */
      bcmifp = ifp->system_info;
      if (! bcmifp)
        {
          return -1;
        }
      
      msw.next_lport = bcmifp->u.l2.lport;
      msw.dst_lport = bcmifp->u.l2.lport;
    }
  
  switch (ilm->opcode)
    {
    case HAL_MPLS_SWAP:
      msw.action = BCM_MPLS_ACTION_SWAP;
      msw.swap_label = ilm->swap_label;
      break;
    case HAL_MPLS_SWAP_AND_LOOKUP:
      msw.action = BCM_MPLS_ACTION_SWAP_PUSH;
      msw.push_label1 = ilm->tunnel_label;
      msw.swap_label = ilm->swap_label;
      break;
    case HAL_MPLS_POP:
      msw.action = BCM_MPLS_ACTION_POP_L3_NEXTHOP;
      break;
    case HAL_MPLS_PHP:
      msw.action = BCM_MPLS_ACTION_PHP;
      break;
#if 0
    case HAL_MPLS_POP_FOR_VPN:
      break;
#endif
#ifdef HAVE_MPLS_VC
    case HAL_MPLS_POP_FOR_VC:
      msw.action = BCM_MPLS_ACTION_POP_DST_MOD_PORT;
      bcmifp = ilm->out_ifp->system_info;
      if (bcmifp->type == HSL_BCM_IF_TYPE_L3_IP)
        {
          ifp = hsl_ifmgr_get_first_L2_port (ilm->out_ifp);
          if (! ifp)
            return -1;
          bcmifp = ifp->system_info;
          HSL_IFMGR_IF_REF_DEC (ifp);
        }
      msw.dst_lport = bcmifp->u.l2.lport;
      msw.next_lport = bcmifp->u.l2.lport;
      msw.dst_vlan = ilm->vc->ve->vlan_id;
#if 0
      msw.vpn = ilm->vc->ve->vpn_id;
      msw.flags |= BCM_MPLS_L2_LEARN;
#endif
      break;
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
    case HAL_MPLS_POP_FOR_VPLS:
      msw.action = BCM_MPLS_ACTION_POP_L2_SWITCH;
      msw.dst_vlan = ilm->vc->ve->vlan_id;
      break;
#endif /* HAVE_VPLS */
    default:
      return -1;
    }
  
  ret = bcmx_mpls_switch_add (&msw);
  if (ret != BCM_E_NONE)
    {
      return ret;
    }

  /* This Wrapper API is not available in SDK-5.6.6 
     Hence do not use this for GTO */
#ifndef HAVE_GTO
  bcmx_mpls_station_add (mpls_mac, mpls_vid); 
#endif /* HAVE_GTO */
  return 0;
}


int
hsl_bcm_mpls_ilm_del_from_hw (struct hsl_mpls_ilm_entry *ilm)
{
  bcmx_mpls_switch_t msw;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;

  bcmx_mpls_switch_t_init (&msw);
  msw.src_label1 = ilm->in_label;
  msw.flags = BCM_MPLS_ONE_LABEL_LOOKUP;
  ifp = hsl_ifmgr_get_first_L2_port (ilm->in_ifp);
  if (! ifp)
    {
      return -1;
    }

  HSL_IFMGR_IF_REF_DEC (ifp);
  
  /* Get the lport. */
  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      return -1;
    }

  msw.src_lport = bcmifp->u.l2.lport;

  /* Populate next_lport and dst_lport. For opcode POP_FOR_VC nh would be NULL,
     use ilm->out_ifp info to assign next_lport and dst_lport. 
  */
  if (ilm->nh_entry && ilm->nh_entry->ifp)
    {
      ifp = hsl_ifmgr_get_first_L2_port (ilm->nh_entry->ifp);
      if (! ifp)
        {
          return -1;
        }

      HSL_IFMGR_IF_REF_DEC (ifp);

      /* Get the lport. */
      bcmifp = ifp->system_info;
      if (! bcmifp)
        {
          return -1;
        }

      msw.next_lport = bcmifp->u.l2.lport;
      msw.dst_lport = bcmifp->u.l2.lport;
    }
  else if (ilm->opcode == HAL_MPLS_POP_FOR_VC)
    {
      bcmifp = ilm->out_ifp->system_info;
      if (bcmifp->type == HSL_BCM_IF_TYPE_L3_IP)
        {
          ifp = hsl_ifmgr_get_first_L2_port (ilm->out_ifp);
          if (! ifp)
            return -1;
          bcmifp = ifp->system_info;
          HSL_IFMGR_IF_REF_DEC (ifp);
        }
      msw.dst_lport = bcmifp->u.l2.lport;
      msw.next_lport = bcmifp->u.l2.lport;
    }

  bcmx_mpls_switch_delete (&msw);

  return 0;
}


#ifdef HAVE_MPLS_VC
int
hsl_bcm_mpls_vpn_vc_ftn_add_to_hw (struct hsl_mpls_vpn_vc *vc, struct hsl_nh_entry *nh)
{
  bcmx_mpls_circuit_t vc_info;
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *l2_ifp;
  int ret;

  memset (&vc_info, 0, sizeof (bcmx_mpls_circuit_t));

  vc_info.flags = BCM_MPLS_ORIGINAL_PKT;
  vc_info.vpn = vc->ve->vpn_id;
  vc_info.label = vc->vc_ftn_label;

  bcmifp = nh->ifp->system_info;
  if (! bcmifp)
    return -1;

  if (nh->nh_type == HSL_NH_TYPE_IP)
    {
      if (bcmifp->type != HSL_BCM_IF_TYPE_L3_IP)
        return -1;
      vc_info.l3_intf = bcmifp->u.l3.ifindex; 
    }
  else if (nh->nh_type == HSL_NH_TYPE_MPLS)
    {
      if (bcmifp->type != HSL_BCM_IF_TYPE_MPLS ||
          ! FLAG_ISSET (bcmifp->u.mpls.flags, HSL_BCM_IF_FLAG_TUNNEL_INSTALLED))
        return -1;

      vc_info.l3_intf = bcmifp->u.mpls.ifindex; 
    }
  else
    return -1;

  memcpy (vc_info.dst_mac, nh->mac, HSL_ETHER_ALEN);
  
  l2_ifp = hsl_ifmgr_get_first_L2_port (nh->ifp);
  if (! l2_ifp)
    return -1;

  bcmifp = l2_ifp->system_info;
  if (! bcmifp)
    {
      HSL_IFMGR_IF_REF_DEC (l2_ifp);
      return -1;
    }

  vc_info.dst_port = bcmifp->u.l2.lport;
  ret = hsl_bcm_add_port_to_vlan (vc->ve->vlan_id,
                                  bcmifp->u.l2.lport, 0);
  HSL_IFMGR_IF_REF_DEC (l2_ifp);
  if (ret < 0)
    return -1;

  ret = bcmx_mpls_vpn_circuit_add (vc->ve->vpn_id, &vc_info);
  return ret;
}



int
hsl_bcm_mpls_vpn_vc_ftn_del_from_hw (struct hsl_mpls_vpn_vc *vc)
{
  bcmx_mpls_circuit_t vc_info;
  int ret;
  struct hsl_bcm_if *bcmifp;
  struct hsl_nh_entry *nh;

  memset (&vc_info, 0, sizeof (bcmx_mpls_circuit_t));

  vc_info.flags = BCM_MPLS_ORIGINAL_PKT;
  vc_info.vpn = vc->ve->vpn_id;
  vc_info.label = vc->vc_ftn_label;

  nh = vc->vc_nh_entry;

  bcmifp = nh->ifp->system_info;
  if (! bcmifp)
    return -1;

  if (nh->nh_type == HSL_NH_TYPE_IP)
    {
      if (bcmifp->type != HSL_BCM_IF_TYPE_L3_IP)
        return -1;
      vc_info.l3_intf = bcmifp->u.l3.ifindex; 
    }
  else if (nh->nh_type == HSL_NH_TYPE_MPLS)
    {
      if (bcmifp->type != HSL_BCM_IF_TYPE_MPLS ||
          ! FLAG_ISSET (bcmifp->u.mpls.flags, HSL_BCM_IF_FLAG_TUNNEL_INSTALLED))
        return -1;

      vc_info.l3_intf = bcmifp->u.mpls.ifindex; 
    }
  else
    return -1;

  ret = bcmx_mpls_vpn_circuit_delete (vc->ve->vpn_id, (bcmx_mpls_circuit_t *)&vc_info);
  return ret;
}
#endif /* HAVE_MPLS_VC */

int
hsl_bcm_mpls_vpn_add_to_hw (struct hsl_mpls_vpn_entry *ve)
{
  int ret;

  if (ve->vpn_id > HSL_BCM_MPLS_VPN_MAX)
    return -1;

#ifdef HAVE_VPLS
  if (ve->vpn_type == HSL_MPLS_VPN_VPLS)
    {
      ret = hsl_bcm_resv_vlan_allocate (&hsl_bcm_vpn_vlans[ve->vpn_id]);
      if (ret < 0)
        return ret;

      ve->vlan_id = hsl_bcm_vpn_vlans[ve->vpn_id]->vid;
      SET_FLAG (ve->flags, HSL_MPLS_VPN_RSVD_VLAN);
    }
#endif /* HAVE_VPLS */

  return bcmx_mpls_vpn_create (ve->vpn_id, ve->vpn_type == HSL_MPLS_VPN_VRF 
                               ? BCM_MPLS_L3_VPN : BCM_MPLS_L2_VPN);
}

int
hsl_bcm_mpls_vpn_del_from_hw (struct hsl_mpls_vpn_entry *ve)
{
  if (ve->vpn_id > HSL_BCM_MPLS_VPN_MAX)
    return -1;

#ifdef HAVE_VPLS
  if (ve->vpn_type == HSL_MPLS_VPN_VPLS &&
      hsl_bcm_vpn_vlans[ve->vpn_id])
    {
      hsl_bcm_resv_vlan_deallocate (hsl_bcm_vpn_vlans[ve->vpn_id]);
      hsl_bcm_vpn_vlans[ve->vpn_id] = NULL;
      ve->vlan_id = 0;
      UNSET_FLAG (ve->flags, HSL_MPLS_VPN_RSVD_VLAN);
    }
#endif /* HAVE_VPLS */

  return bcmx_mpls_vpn_destroy (ve->vpn_id);
}


static int
_hsl_bcm_mpls_vpn_if_bind_unbind_common (struct hsl_mpls_vpn_entry *ve,
                                         struct hsl_mpls_vpn_port *vp, 
                                         bcmx_mpls_vpn_t *vpn_info, 
                                         struct hsl_bcm_if **ppbcmif)
{
  struct hsl_bcm_if *bcmifp = NULL;
  struct hsl_if *ifp, *ifp_l2;

  *ppbcmif = NULL;

  if (ve->vpn_id > HSL_BCM_MPLS_VPN_MAX)
    return -1;

  ifp = hsl_ifmgr_lookup_by_index (vp->port_id);
  if (! ifp)
    return -1;

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      bcmifp = ifp->system_info;
    }
  else
    {
      ifp_l2 = hsl_ifmgr_get_first_L2_port (ifp);
      if (! ifp_l2)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          return -1;
        }

      bcmifp = ifp_l2->system_info;
      HSL_IFMGR_IF_REF_DEC (ifp_l2);
    }
  
  HSL_IFMGR_IF_REF_DEC (ifp);
  
  if (! bcmifp)
    return -1;

  bcmx_mpls_vpn_t_init (vpn_info);
  vpn_info->vpn = ve->vpn_id;
  vpn_info->lport = bcmifp->u.l2.lport;
  vpn_info->flags = BCM_MPLS_L2_VPN;

  *ppbcmif = bcmifp;

  return 0;
}

int
hsl_bcm_mpls_vpn_if_bind (struct hsl_mpls_vpn_entry *ve, 
                          struct hsl_mpls_vpn_port *vp)
{
  bcmx_mpls_vpn_t vpn_info;
  struct hsl_bcm_if *bcmifp;
  int ret;

  ret = _hsl_bcm_mpls_vpn_if_bind_unbind_common (ve, vp, &vpn_info, &bcmifp);
  if (ret < 0)
    return ret;

  if (ve->vpn_type == HSL_MPLS_VPN_MARTINI)
    {
      if (vp->port_vlan_id == 0)
        {
          if (hsl_bcm_vpn_vlans[ve->vpn_id] == NULL)
            {
              ret = hsl_bcm_resv_vlan_allocate (&hsl_bcm_vpn_vlans[ve->vpn_id]);
              if (ret < 0)
                return ret;
            }
          
          ve->vlan_id = hsl_bcm_vpn_vlans[ve->vpn_id]->vid;
          SET_FLAG (ve->flags, HSL_MPLS_VPN_RSVD_VLAN);
        }
      else
        {
          ve->vlan_id = vp->port_vlan_id;
        } 
    }
  else if (ve->vpn_type == HSL_MPLS_VPN_VPLS)
    {
      if (hsl_bcm_vpn_vlans[ve->vpn_id] == NULL ||
          ve->vlan_id == 0)
        return -1;
      
      if (vp->port_vlan_id != 0 &&
          vp->port_vlan_id != ve->vlan_id)
        {
          ret = bcmx_vlan_translate_add (bcmifp->u.l2.lport,
                                         vp->port_vlan_id, 
                                         ve->vlan_id, -1);
          if (ret != BCM_E_NONE)
            return -1;
        }
    }
  
  if (vp->port_vlan_id == 0)
    {
      ret = bcmx_port_untagged_vlan_set (bcmifp->u.l2.lport, 
                                         ve->vlan_id);
      if (ret != BCM_E_NONE)
        return -1;
    }
  
  
  vpn_info.vlan = ve->vlan_id;

  ret = bcmx_mpls_vpn_add (ve->vpn_id, &vpn_info); 
  if (ret != BCM_E_NONE)
    return -1;

  hsl_bcm_add_port_to_vlan (ve->vlan_id, bcmifp->u.l2.lport, 0);

  return 0;    
}


int
hsl_bcm_mpls_vpn_if_unbind (struct hsl_mpls_vpn_entry *ve,
                            struct hsl_mpls_vpn_port *vp)
{
  bcmx_mpls_vpn_t vpn_info;
  struct hsl_bcm_if *bcmifp;
  int ret;

  ret = _hsl_bcm_mpls_vpn_if_bind_unbind_common (ve, vp, &vpn_info, &bcmifp);
  if (ret < 0)
    return ret;

  if (vp->port_vlan_id == 0)
    {
      bcmx_port_untagged_vlan_set (bcmifp->u.l2.lport, 
                                   HSL_DEFAULT_VID);
    }
  
  if (ve->vpn_type == HSL_MPLS_VPN_MARTINI)
    {
      ve->vlan_id = 0;
      UNSET_FLAG (ve->flags, HSL_MPLS_VPN_RSVD_VLAN);
      if (vp->port_vlan_id == 0)
        {
          if (hsl_bcm_vpn_vlans[ve->vpn_id] != NULL)
            {
              hsl_bcm_resv_vlan_deallocate (hsl_bcm_vpn_vlans[ve->vpn_id]);
              hsl_bcm_vpn_vlans[ve->vpn_id] = NULL;
            }
        }
    }
  else if (ve->vpn_type == HSL_MPLS_VPN_VPLS)
    {
      if (vp->port_vlan_id != 0 &&
          vp->port_vlan_id != ve->vlan_id)
        {
          bcmx_vlan_translate_delete (bcmifp->u.l2.lport, 
                                      vp->port_vlan_id);
        }
    }
  
  vpn_info.vlan = ve->vlan_id;

  ret = bcmx_mpls_vpn_delete (ve->vpn_id, &vpn_info);
  if (ret != BCM_E_NONE)
    return -1;

  return 0;
}


/*
  Register MPLS hw callbacks.
*/
void
hsl_mpls_hw_cb_set_and_register (void)
{
  hsl_bcm_mpls_callbacks.hw_mpls_ftn_add = hsl_bcm_mpls_ftn_add_to_hw;
  hsl_bcm_mpls_callbacks.hw_mpls_ilm_add = hsl_bcm_mpls_ilm_add_to_hw;
  hsl_bcm_mpls_callbacks.hw_mpls_ftn_del = hsl_bcm_mpls_ftn_del_from_hw;
  hsl_bcm_mpls_callbacks.hw_mpls_ilm_del = hsl_bcm_mpls_ilm_del_from_hw;

#ifdef HAVE_MPLS_VC
  hsl_bcm_mpls_callbacks.hw_mpls_vpn_vc_ftn_add = hsl_bcm_mpls_vpn_vc_ftn_add_to_hw;
  hsl_bcm_mpls_callbacks.hw_mpls_vpn_vc_ftn_del = hsl_bcm_mpls_vpn_vc_ftn_del_from_hw;
#endif /* HAVE_MPLS_VC */
  hsl_bcm_mpls_callbacks.hw_mpls_vpn_add = hsl_bcm_mpls_vpn_add_to_hw;
  hsl_bcm_mpls_callbacks.hw_mpls_vpn_del = hsl_bcm_mpls_vpn_del_from_hw;
  hsl_bcm_mpls_callbacks.hw_mpls_vpn_if_bind = hsl_bcm_mpls_vpn_if_bind;
  hsl_bcm_mpls_callbacks.hw_mpls_vpn_if_unbind = hsl_bcm_mpls_vpn_if_unbind;

  hsl_mpls_hw_cb_register (&hsl_bcm_mpls_callbacks);
}
