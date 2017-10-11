/* Copyright (C) 2004  All Rights Reserved. */


#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#ifdef HAVE_LACPD

#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl_error.h"
#include "bcm_incl.h"
#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_bcm_if.h"
#include "hsl_vlan.h"
#include "hsl_bcm_ifmap.h"

static int hsl_bcm_lacp_initialized = HSL_FALSE;

int
hsl_bcm_lacp_init ()
{
  int ret;
   
  HSL_FN_ENTER(); 

  if (hsl_bcm_lacp_initialized)
    HSL_FN_EXIT (0);

  /* initialize the underlying bcm trunk module */
  ret = bcmx_trunk_init ();
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to initialize the trunk module, bcm error = %d\n", ret);
      HSL_FN_EXIT(ret);
    }
  hsl_bcm_lacp_initialized = HSL_TRUE;

  HSL_FN_EXIT(STATUS_OK);
}

int
hsl_bcm_lacp_deinit ()
{
  int ret;

  HSL_FN_ENTER(); 

  if (! hsl_bcm_lacp_initialized)
    HSL_FN_EXIT (-1);

  /* uninitialize the underlying bcm trunk module */
  ret = bcmx_trunk_detach ();
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to uninitialize the trunk module, bcm error = %d\n",
               ret);
      HSL_FN_EXIT(ret);
    }

  hsl_bcm_lacp_initialized = HSL_FALSE;

  HSL_FN_EXIT(STATUS_OK);
}


int
hsl_bcm_aggregator_add (struct hsl_if *ifp, int agg_type)
{
  int ret;
  struct hsl_bcm_if *bcmif = NULL;
  bcm_trunk_t trunk_id = BCM_TRUNK_INVALID;
  char ifname[HSL_IFNAM_SIZE + 1];     /* Interface name.                   */
  u_char mac[HSL_ETHER_ALEN];          /* Ethernet mac address.             */

  HSL_FN_ENTER();  

  if(!ifp)
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);

  /* Create a new trunk instance */
  ret = bcmx_trunk_create (&trunk_id);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to create aggregator %s in hw, bcm error %d\n", ifp->name, ret); 
      HSL_FN_EXIT(HSL_IFMGR_ERR_HW_TRUNK_ADD); 
    }

  /* Register with ifmap and get name and mac address for this trunk. */
  ret = hsl_bcm_ifmap_lport_register (HSL_BCM_TRUNK_2_LPORT(trunk_id), HSL_PORT_F_AGG, ifname, mac);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Can't register trunk (%d) in Broadcom interface map\n", trunk_id);
      /* delete trunk from bcom */
      bcmx_trunk_destroy (trunk_id);
      HSL_FN_EXIT(HSL_IFMGR_ERR_HW_TRUNK_ADD);;
    }

  /* Allocate hw specific structure for L2 interfaces. */
  if(agg_type != HSL_IF_TYPE_IP)
    {
      ifp->system_info = hsl_bcm_if_alloc ();
      if (! ifp->system_info )
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "  Out of memory for allocating hardware L2 interface\n");
          hsl_bcm_ifmap_lport_unregister (HSL_BCM_TRUNK_2_LPORT(trunk_id));
          bcmx_trunk_destroy (trunk_id);
          HSL_FN_EXIT(HSL_IFMGR_ERR_MEMORY);
        }
    }

  /* Store bcm trunk id for aggregator interface */
  bcmif = ifp->system_info;
  bcmif->trunk_id = trunk_id;
  bcmif->type = HSL_BCM_IF_TYPE_TRUNK;
  /* Associate trunk to interface structure. */
  hsl_bcm_ifmap_if_set (HSL_BCM_TRUNK_2_LPORT(trunk_id), ifp);
  /* Call interface addition notifier */
  hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEW, ifp, NULL);
  HSL_FN_EXIT(STATUS_OK);
}

int
hsl_bcm_aggregator_del (struct hsl_if *ifp)
{
  struct hsl_bcm_if *bcmif;

  HSL_FN_ENTER(); 

  /* Sanity check */
  if(!ifp)
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);
  
  bcmif = ifp->system_info;
  if(!bcmif)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, " Interface %s doesn't have hw specific info\n",ifp->name);
      HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);
    }

  /* get trunk id */
  if (bcmif->trunk_id == BCM_TRUNK_INVALID)
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);

  /* Unregister interface from ifmap. */
  hsl_bcm_ifmap_lport_unregister (HSL_BCM_TRUNK_2_LPORT(bcmif->trunk_id));

  /* delete trunk from bcom */
  bcmx_trunk_destroy (bcmif->trunk_id);

  /* reset bcm trunk id */
  bcmif->trunk_id = BCM_TRUNK_INVALID;

  HSL_FN_EXIT(STATUS_OK);
}

int
hsl_bcm_trunk_membership_update (struct hsl_if *ifp)
{
  struct hsl_if *tmpif;
  struct hsl_if_list *nm, *nn;
  bcmx_trunk_add_info_t tinfo;
  bcmx_lport_t lport;
  int ret;
  struct hsl_bcm_if *sysinfo = NULL;
  HSL_BOOL update_flag = HSL_FALSE;


  HSL_FN_ENTER();

  /* Sanity part. */
  if(!ifp)
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);

  /* Make sure interface is a trunk. */
  sysinfo = (struct hsl_bcm_if *)ifp->system_info;
  if(!sysinfo)
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);
 
  if (sysinfo->type != HSL_BCM_IF_TYPE_TRUNK ||
      sysinfo->trunk_id == BCM_TRUNK_INVALID)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Interface is not a trunk %s\n", ifp->name);
      HSL_FN_EXIT(STATUS_ERROR);
    }


  /* add ports to bcm trunk */
  tinfo.psc = BCM_TRUNK_PSC_DEFAULT;
  tinfo.dlf_port = -1;
  tinfo.mc_port = -1;
  tinfo.ipmc_port = -1;

  bcmx_lplist_init (&tinfo.ports, 0, 0);

  if (!ifp->children_list)
    HSL_FN_EXIT(STATUS_OK);
  
  /* create bcm trunk port list */
  for (nm = ifp->children_list; nm; nm = nm->next)
    {
      tmpif = nm->ifp;

      /* get associated bcm lport information */
      if (tmpif->type == HSL_IF_TYPE_L2_ETHERNET)
        {
          sysinfo = (struct hsl_bcm_if *)tmpif->system_info;
          if (sysinfo->type != HSL_BCM_IF_TYPE_L2_ETHERNET)
            continue;

          /* Set this port to accept LACP PDUs. */
          hsl_ifmgr_set_acceptable_packet_types (tmpif, HSL_IF_PKT_LACP);
        }
      else if (tmpif->type == HSL_IF_TYPE_IP)
        {
          nn = tmpif->children_list;
          if (! nn || nn->ifp->type != HSL_IF_TYPE_L2_ETHERNET)
            continue;

          /* for ip ports get lport informaiton from associated l2 port */
          sysinfo = (struct hsl_bcm_if *) nn->ifp->system_info;
          if (sysinfo->type != HSL_BCM_IF_TYPE_L2_ETHERNET)
            continue;

          /* Set this port to accept LACP PDUs. */
          hsl_ifmgr_set_acceptable_packet_types (nn->ifp, HSL_IF_PKT_LACP);
        }
      else
        continue;

      lport =  sysinfo->u.l2.lport;
      bcmx_lplist_add (&tinfo.ports, lport);
      update_flag = HSL_TRUE;
    }

  if (update_flag == HSL_TRUE)
    {
      /* attach port to trunk */
      sysinfo = (struct hsl_bcm_if *)ifp->system_info;
      ret = bcmx_trunk_set (sysinfo->trunk_id, &tinfo);
      if (ret != BCM_E_NONE)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                   "Failed to update port membership for aggregator %s in hw, "
                   "bcm error %d\n", ifp->name, ret); 
          bcmx_lplist_free (&tinfo.ports);
          HSL_FN_EXIT(HSL_IFMGR_ERR_HW_TRUNK_PORT_UPDATE);
        }
    }

  bcmx_lplist_free (&tinfo.ports);

  HSL_FN_EXIT(STATUS_OK);
}

int
hsl_bcm_aggregator_port_update_vid (struct hsl_if *agg_ifp, 
                                    struct hsl_if *port_ifp, 
                                    int remove)
{
  struct hsl_bcm_if *l2port_bcmifp, *l3port_bcmifp,*agg_bcmifp;
#ifdef HAVE_L3
  bcmx_l3_intf_t intf;
#endif /* HAVE_L3 */
  struct hsl_if *l2_ifp;
  bcmx_lport_t lport;
  int ret,old_vid,new_vid,i;
  
  /* Get the lport of member interface */
  l2_ifp = hsl_ifmgr_get_first_L2_port (port_ifp);
  if (! l2_ifp)
    HSL_FN_EXIT (-1);

  l2port_bcmifp = l2_ifp->system_info;
  if (! l2port_bcmifp)
    {
      HSL_IFMGR_IF_REF_DEC (l2_ifp);
      HSL_FN_EXIT (-1);
    }

  lport = l2port_bcmifp->u.l2.lport;

  /* Get the bcmifp of the member interface */
  l3port_bcmifp = port_ifp->system_info;
  if (! l3port_bcmifp)
    {
      HSL_IFMGR_IF_REF_DEC (l2_ifp);
      HSL_FN_EXIT (-1);
    }

  agg_bcmifp = agg_ifp->system_info;
  if (! agg_bcmifp)
    {
      HSL_IFMGR_IF_REF_DEC (l2_ifp);
      HSL_FN_EXIT (-1);
    }

  if (remove)
    {
      old_vid = l3port_bcmifp->u.l3.vid;
      new_vid = agg_bcmifp->u.l3.vid;
    }
  else
    {
      old_vid = agg_bcmifp->u.l3.vid;
      new_vid = l3port_bcmifp->u.l3.vid;
    }
 
  /* Remove port from the aggregated interface vlan */
  ret = hsl_bcm_remove_port_from_vlan (old_vid, lport);
  if ((ret < 0) && (ret != BCM_E_NOT_FOUND))
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "  Could not remove %s interface from \
                aggregated interface vlan (%d)\n",
                port_ifp->name, ret);
      HSL_IFMGR_IF_REF_DEC (l2_ifp);
      HSL_FN_EXIT (-1);
    }

  /* Add memeber port to aggregated interface VLAN. */
  ret = hsl_bcm_add_port_to_vlan (new_vid, lport, 0);
  if ((ret < 0) && (ret != BCM_E_EXISTS))
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't add port %d to vlan %d\n",
               lport, new_vid);
      HSL_IFMGR_IF_REF_DEC (l2_ifp);
      HSL_FN_EXIT (ret);
    }

  /* Set PVID to aggregated interface vid. */
  ret = bcmx_port_untagged_vlan_set (lport, new_vid);
  if ((ret < 0) && (ret != BCM_E_EXISTS))
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Could not set default PVID for port %s\n", 
                agg_ifp->name);

      /* Delete port from vlan. */
      hsl_bcm_remove_port_from_vlan (new_vid, lport);

      HSL_IFMGR_IF_REF_DEC (l2_ifp);
      HSL_FN_EXIT (ret);
    }

#ifdef HAVE_L3
  /* Initialize interface. */
  bcmx_l3_intf_t_init (&intf);

  /* Set fib id */
  intf.l3a_vrf = (bcm_vrf_t)(port_ifp->fib_id);

  /* Set VID. */
  intf.l3a_vid = IFP_IP(port_ifp).vid;

  /* Set MAC. */
  memcpy (intf.l3a_mac_addr, IFP_IP(port_ifp).mac, sizeof (bcm_mac_t));

  if (remove)
    {
       if (BCMIFP_L3(l3port_bcmifp).ifindex > 0)
         {
          /* Delete the primary L3 interface. */
	  /* Set index. */
	  intf.l3a_intf_id = BCMIFP_L3(l3port_bcmifp).ifindex;

	  /* Destroy the L3 interface. */
	  ret = bcmx_l3_intf_delete (&intf);
	  if ((ret < 0) && (ret != BCM_E_NOT_FOUND))
	    {
	      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
		       "  Could not delete L3 interface %s from hardware\n", 
                        port_ifp->name);  
              HSL_IFMGR_IF_REF_DEC (l2_ifp);
              HSL_FN_EXIT (-1);
	    }
           BCMIFP_L3(l3port_bcmifp).ifindex = -1;
         }
    }
  else
    {
      /* Set flags. */
      intf.l3a_flags |= BCM_L3_ADD_TO_ARL;

      /* Set MTU. */
      intf.l3a_mtu = IFP_IP(port_ifp).mtu;

      /* Configure a L3 interface in BCM.
         Add static entry in L2 table with L3 bit set. */
       ret = bcmx_l3_intf_create (&intf);
       if ((ret < 0) && (ret != BCM_E_EXISTS))
         {
           HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                    "  Could not create L3 interface in hardware \
                    for interface %s %d ret(%d)\n",
                    port_ifp->name, port_ifp->ifindex, ret);

           HSL_IFMGR_IF_REF_DEC (l2_ifp);
           HSL_FN_EXIT (-1);
         }

       l3port_bcmifp->u.l3.ifindex = intf.l3a_intf_id;
    }
  
  /* Delete the secondary L3 interfaces. */
  for (i = 0; i < IFP_IP(port_ifp).nAddrs; i++)
     {
       /* Sanity check. */
       if ((port_ifp == NULL) || (IFP_IP(port_ifp).addresses[i] == NULL))
         {
           HSL_IFMGR_IF_REF_DEC (l2_ifp);
           HSL_FN_EXIT (-1);
         }
 
       /* Initialize entry. */
       bcmx_l3_intf_t_init (&intf);

       /* Set fib id */
       intf.l3a_vrf = (bcm_vrf_t)(port_ifp->fib_id);

       /* Set VID. */
       intf.l3a_vid = IFP_IP(port_ifp).vid;

       /* Set MAC. */
       memcpy (intf.l3a_mac_addr, IFP_IP(port_ifp).addresses[i], 
               sizeof (bcm_mac_t));

       if (remove)
         {
            /* Find the entry. */
            ret = bcmx_l3_intf_find (&intf);
            if (ret < 0)
	      {
	         HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
	                  "  Could not find L3 interface %s in hardware\n", 
                          port_ifp->name);
	         HSL_IFMGR_IF_REF_DEC (l2_ifp);
	         HSL_FN_EXIT (-1);
	      }

            /* Delete the interface. */
            ret = bcmx_l3_intf_delete (&intf);
            if ((ret < 0) && (ret != BCM_E_NOT_FOUND))
              {
                HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                         "  Could not delete L3 interface %s in hardware\n", 
                          port_ifp->name);
	        HSL_IFMGR_IF_REF_DEC (l2_ifp);
	        HSL_FN_EXIT (-1);
	      }
         }
       else
         {
            /* Set flags. */
            intf.l3a_flags |= BCM_L3_ADD_TO_ARL;

            /* Set MTU. */
            intf.l3a_mtu = IFP_IP(port_ifp).mtu;

            ret = bcmx_l3_intf_create (&intf);
            if ((ret < 0) && (ret != BCM_E_EXISTS))
              {
                HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                        "  Could not create L3 interface in hardware \
                        for interface %s %d ret(%d)\n",
                        port_ifp->name, port_ifp->ifindex, ret);

                 HSL_IFMGR_IF_REF_DEC (l2_ifp);
                 HSL_FN_EXIT (-1);
              }
         }
     }
#endif /* HAVE_L3 */
  HSL_IFMGR_IF_REF_DEC (l2_ifp);

  HSL_FN_EXIT (0);
}

int
hsl_bcm_aggregator_port_add ( struct hsl_if *agg_ifp, struct hsl_if *port_ifp )
{
  int ret;

  HSL_FN_ENTER();

  ret = hsl_bcm_trunk_membership_update (agg_ifp);

  /* If aggregate members are L3 Interfaces, then change member
   * intf vid from default to aggregated intf vid.
   */
  if (port_ifp->type == HSL_IF_TYPE_IP)
    {
      ret = hsl_bcm_aggregator_port_update_vid(agg_ifp, port_ifp, 1);
      if (ret < 0)
       {
         HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
              "Can not update the Vid for  %s\n", port_ifp->name);
         HSL_FN_EXIT(ret);
      }
    }

  HSL_FN_EXIT(ret);
}

int
hsl_bcm_aggregator_port_del ( struct hsl_if *agg_ifp, struct hsl_if *port_ifp )
{
  struct hsl_bcm_if *bcmifp;
  bcmx_lport_t lport;
  int ret;

  HSL_FN_ENTER();

  /* Flush the MAC entries from this port. */
  if (port_ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      bcmifp = port_ifp->system_info;
      if (! bcmifp)
        HSL_FN_EXIT (-1);

      lport = bcmifp->u.l2.lport;

      /* Flush from chips. */
      bcmx_l2_addr_delete_by_port (lport, 0);
    }

  /* Update membership. */
  ret = hsl_bcm_trunk_membership_update (agg_ifp);

  /* Remove the member port from the aggregated interface vlan
   * and add it to default vlan.
   */
  if (port_ifp->type == HSL_IF_TYPE_IP)
   {
     ret = hsl_bcm_aggregator_port_update_vid (agg_ifp, port_ifp, 0);
     if (ret < 0)
       {
         HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
              "Can not update the Vid for  %s\n", port_ifp->name);
         HSL_FN_EXIT(ret);
      }
   }

  HSL_FN_EXIT(ret);
}

int 
hsl_bcm_lacp_psc_set (struct hsl_if *ifp,int psc)
{
  struct hsl_bcm_if *sysinfo;
  int ret;
  
  HSL_FN_ENTER();
  sysinfo = (struct hsl_bcm_if *)ifp->system_info;

  /* Get hw specific info. */
  if(!sysinfo)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Interface doesn't have hw info %s\n", ifp->name);
      HSL_FN_EXIT(STATUS_ERROR);
    }

  /* Make sure interface is a trunk. */
  if (sysinfo->type != HSL_BCM_IF_TYPE_TRUNK ||
      sysinfo->trunk_id == BCM_TRUNK_INVALID)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Interface is not a trunk %s\n", ifp->name);
      HSL_FN_EXIT(STATUS_ERROR);
    }

  /* Set psc for a trunk. */
        switch(psc)
                {
                        case HAL_LACP_PSC_DST_MAC:
                                psc = BCM_TRUNK_PSC_DSTMAC;
                                break;
                        case HAL_LACP_PSC_SRC_MAC:
                                psc = BCM_TRUNK_PSC_SRCMAC;
                                break;
                        case HAL_LACP_PSC_SRC_DST_MAC:
                                psc = BCM_TRUNK_PSC_SRCDSTMAC;
                                break;
                        case HAL_LACP_PSC_SRC_IP:
                                psc = BCM_TRUNK_PSC_SRCIP;
                                break;
                        case HAL_LACP_PSC_DST_IP:
                                psc = BCM_TRUNK_PSC_DSTIP;
                                break;
                        case HAL_LACP_PSC_SRC_DST_IP:
                                psc = BCM_TRUNK_PSC_SRCDSTIP;
                                break;
                        case HAL_LACP_PSC_SRC_PORT:
                                psc = BCM_TRUNK_PSC_L4SS;
                                break;
                        case HAL_LACP_PSC_DST_PORT:
                                psc = BCM_TRUNK_PSC_L4DS;
                                break;
                        default :
                                return -1;
                }
  ret = bcmx_trunk_psc_set (sysinfo->trunk_id, psc);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to set psc for trunk %s, bcm error = %d\n", 
               ifp->name, ret);
      HSL_FN_EXIT(STATUS_ERROR);
    }

  HSL_FN_EXIT(STATUS_OK);
}

#endif /* HAVE_LACPD */
