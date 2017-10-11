#include "config.h"

#ifdef HAVE_TUNNEL

#include "hsl_os.h"

#include "bcm_incl.h"
#include "hsl_avl.h"
#include "hsl_types.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_bcm.h"
#include "hal_msg.h"
#include "hsl_bcm_if.h"

extern struct hsl_if* hsl_ifmgr_lookup_by_prefix (hsl_prefix_t *);

/* Function to create and fill terminator structure */
int hsl_tunnel_if2terminator (bcmx_tunnel_terminator_t *tt, 
                              struct hal_msg_tunnel_if *ifp)
{
  struct hsl_if *pifp,*l2_ifp;
  struct hsl_avl_node *node;
  struct hsl_bcm_if *bcmifp;
  int ret = 0;

  HSL_FN_ENTER (); 

  /* SIP for tunnel header match */
  tt->sip = ifp->daddr4.s_addr;
  tt->sip_mask = 0xffffffff;

  /* DIP for tunnel header match */
  tt->dip = ifp->saddr4.s_addr;
  tt->dip_mask  = 0xffffffff;
  tt->vrf = 0;
  if (ifp->mode == TUNNEL_MODE_IPIP)
    tt->type = bcmTunnelTypeIp4In4;
  else if (ifp->mode == TUNNEL_MODE_IPV6IP)
    tt->type = bcmTunnelTypeIp6In4;
 
  HSL_IFMGR_LOCK;
 
  for (node = hsl_avl_top (p_hsl_if_db->if_tree); node;node = hsl_avl_next (node))
    { 
      if (((pifp = node->info) != NULL) && (pifp->type == HSL_IF_TYPE_IP))
       {
         l2_ifp = hsl_ifmgr_get_first_L2_port (pifp);
         bcmifp = l2_ifp->system_info;
         ret = bcmx_lplist_add (&tt->ports, bcmifp->u.l2.lport);
         if (ret != BCM_E_NONE)
           {
             HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
                      "Error in adding lport to plist\n");
           }
        }
    }
  HSL_IFMGR_UNLOCK;
  
  HSL_FN_EXIT (0);
}

/* Function to create and fill initiator structure */
int hsl_tunnel_if2initiator (bcmx_tunnel_initiator_t *ti ,
                             struct hal_msg_tunnel_if *ifp)
{
  HSL_FN_ENTER ();

  ti->sip = ifp->saddr4.s_addr;
  ti->dip = ifp->daddr4.s_addr;
  if (ifp->mode == TUNNEL_MODE_IPIP)
    ti->type = bcmTunnelTypeIp4In4;
  else if (ifp->mode == TUNNEL_MODE_IPV6IP)
    ti->type = bcmTunnelTypeIp6In4;
  memcpy (&ti->dmac, &ifp->dmac_addr, ETHER_ADDR_LEN);
  ti->ttl = ifp->ttl;

  HSL_FN_EXIT (0);
}

/* Function to set the Tunnel Initiator */
int hsl_tunnel_initiator_set (struct hal_msg_tunnel_if *t_ifp)
{
    struct hsl_bcm_if *bcmifp = NULL;
    bcmx_tunnel_initiator_t ti;
    bcmx_l3_intf_t intf;
    hsl_prefix_t prefix;
    struct hsl_if *ifp;
    int ret;

   HSL_FN_ENTER ();

    /* Initailize the initiator structure to default values */
    bcmx_tunnel_initiator_t_init (&ti);

    /* Add the fields related to IP tunnel initiator point */
    if (hsl_tunnel_if2initiator(&ti, t_ifp) < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't Set Tunnel Initiator\n");
      HSL_FN_EXIT (-1);
    }

    /* Find the Egress Interface
      Note: We are matching the Tunnel SIP with Egress IF SIP
    */
    prefix.family = AF_INET;
    prefix.u.prefix4 = t_ifp->saddr4.s_addr;
    ifp = hsl_ifmgr_lookup_by_prefix (&prefix);

    if (ifp == NULL)
      {
        HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't Find the Egress Interface\n");
        HSL_FN_EXIT (-1);
      }

    bcmifp = ifp->system_info;
    intf.l3a_intf_id = bcmifp->u.l3.ifindex;

    /* Set BCM Tunnel Initiator */
    ret = bcmx_tunnel_initiator_set (&intf, &ti);
    if (ret != BCM_E_NONE)
      {
        HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't Set Tunnel Initiator\n");
        HSL_FN_EXIT (-1);
      }

  HSL_FN_EXIT (0);
}

/* Function to delete the Tunnel Initiator */
int hsl_tunnel_initiator_clear (struct hal_msg_tunnel_if *t_ifp)
{
   struct hsl_bcm_if *bcmifp = NULL;
   struct hsl_if *ifp;
   bcmx_l3_intf_t intf;
   hsl_prefix_t prefix;
   int ret;

   HSL_FN_ENTER ();
   /* Find the Egress Interface
     Note: We are matching the Tunnel SIP with Egress IF SIP
   */
   prefix.family = AF_INET;
   prefix.u.prefix4 = t_ifp->saddr4.s_addr;
   ifp = hsl_ifmgr_lookup_by_prefix (&prefix);

   if (ifp == NULL)
      HSL_FN_EXIT (-1);

   bcmifp = ifp->system_info;
   intf.l3a_intf_id = bcmifp->u.l3.ifindex;

   /* Set BCM Tunnel Initiator */
   ret = bcmx_tunnel_initiator_clear (&intf);
   if (ret != BCM_E_NONE)
     {
       HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Failed Tunnel Initiator Clear\n");
       HSL_FN_EXIT (-1);
     }

   HSL_FN_EXIT (0);
}

/* Function to set the Tunnel Terminator */
int hsl_tunnel_terminator_set (struct hal_msg_tunnel_if *t_ifp)
{
  bcmx_tunnel_terminator_t tt;
  int ret;

  HSL_FN_ENTER ();

  /* Initailize the terminator structure to default values */
  bcmx_tunnel_terminator_t_init (&tt);

  /* Add the fields related to IP tunnel terminator point */
  if (hsl_tunnel_if2terminator(&tt, t_ifp) < 0)
     HSL_FN_EXIT (-1);

  ret = bcmx_tunnel_terminator_add (&tt);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't add Tunnel Terminator\n");
      HSL_FN_EXIT (-1);
    }
  
  HSL_FN_EXIT (0);;
}

/* Function to clear the Tunnel Terminator */
int hsl_tunnel_terminator_clear (struct hal_msg_tunnel_if *t_ifp)
{
  bcmx_tunnel_terminator_t tt; 
  int ret;

  HSL_FN_ENTER ();

  bcmx_tunnel_terminator_t_init(&tt);

  /* Add the fields related to IP tunnel terminator point */
  if (hsl_tunnel_if2terminator(&tt, t_ifp) < 0)
     HSL_FN_EXIT (-1);

  ret = bcmx_tunnel_terminator_delete (&tt);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't delete Tunnel Terminator\n");
      HSL_FN_EXIT (-1);
    }

  bcmx_tunnel_terminator_t_free (&tt);

  HSL_FN_EXIT (0);
}

/* Function to create tunnel interface */
int hsl_tunnel_if_add (struct hal_msg_tunnel_add_if *t_ifp)
{  
  int ret;
  struct net_device *dev;

  HSL_FN_ENTER ();

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27)
  dev = dev_get_by_name (current->nsproxy->net_ns, t_ifp->name);
#else 
  dev = dev_get_by_name (t_ifp->name);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

  /* Register the L3 Interface */
  ret = hsl_ifmgr_L3_cpu_if_register (t_ifp->name,t_ifp->ifindex, t_ifp->mtu,
                                      t_ifp->speed, t_ifp->duplex, 
                                      t_ifp->flags, t_ifp->hwaddr, 
                                      dev);
  if (ret < 0)
    HSL_FN_EXIT (-1);

  ret = hsl_tunnel_initiator_set (&t_ifp->tif);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't Set Tunnel Initiator\n");
      HSL_FN_EXIT (-1);
    }

  ret = hsl_tunnel_terminator_set (&t_ifp->tif);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Can't Set Tunnel Terminator\n");
      HSL_FN_EXIT (-1);
    }

  HSL_FN_EXIT (0);
}

/* Function to destroy tunnel interface */
int hsl_tunnel_if_delete (struct hal_msg_tunnel_if *t_ifp)
{
  int ret;

  HSL_FN_ENTER ();
  
  ret = hsl_tunnel_initiator_clear (t_ifp);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Failed Tunnel Initiator Clear\n");
      HSL_FN_EXIT (-1);
    }

  ret = hsl_tunnel_terminator_clear (t_ifp);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Failed Tunnel Terminator Clear\n");
      HSL_FN_EXIT (-1);
    }

  HSL_FN_EXIT (0);
}

#endif /*#HAVE_TUNNEL*/
