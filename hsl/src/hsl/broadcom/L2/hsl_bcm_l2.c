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

/*
  HSL includes.
*/
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl_error.h"
#include "hsl.h"

#include "hsl_avl.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_if_os.h"

#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "hsl_l2_hw.h"
#include "hsl_bcm.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_l2.h"
#include "hsl_bcm_fdb.h"
#include "hsl_mac_tbl.h"
#include "hsl_bcm_vlanclassifier.h"
#include "hsl_bcm_resv_vlan.h"

#ifdef HAVE_PVLAN
#include "hsl_bcm_pvlan.h"
#endif /* HAVE_PVLAN */

static struct hsl_bcm_bridge *hsl_bcm_bridge_p = NULL;
static struct hsl_l2_hw_callbacks hsl_bcm_l2_cb;

#ifdef HAVE_IGMP_SNOOP
#define HSL_IGS_BCM_INVALID_FIELD_GRP       0xffffffff
#define HSL_IGS_BCM_INVALID_FIELD_ENTRY     0xffffffff

static bcm_field_group_t _hsl_igmp_snp_field_grp = HSL_IGS_BCM_INVALID_FIELD_GRP;
static bcm_field_entry_t _hsl_igmp_snp_field_ent = HSL_IGS_BCM_INVALID_FIELD_ENTRY;
#endif

/*
  Initialize BCM bridge.
*/
static struct hsl_bcm_bridge *
_hsl_bcm_bridge_init (void)
{
  struct hsl_bcm_bridge *b;
  int i;

  b = oss_malloc (sizeof (struct hsl_bcm_bridge), OSS_MEM_HEAP);
  if (! b)
    return NULL;

  for (i = 0; i < HSL_BCM_STG_MAX_INSTANCES; i++)
    b->stg[i] = -1;

  return b;
}

/* 
   Deinitialize BCM bridge.
*/
static int
_hsl_bcm_bridge_deinit (struct hsl_bcm_bridge **b)
{
  int i;

  if (*b)
    {

      for (i = 0; i < HSL_BCM_STG_MAX_INSTANCES; i++)
        if ((*b)->stg[i] != -1)
          {
            bcmx_stg_destroy ((*b)->stg[i]);
          }

      oss_free (*b, OSS_MEM_HEAP);
      *b = NULL;
    }
  HSL_FN_EXIT (0);
}

/* 
   STG Initialize.
*/
int
hsl_bcm_stg_init (void)
{
  int ret;

  HSL_FN_ENTER ();
  ret = bcmx_stg_init ();
  HSL_FN_EXIT (ret);
}

/* Bridge init. */
int 
hsl_bcm_bridge_init (struct hsl_bridge *b)
{
  int ret;
  int defstg = 0;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
  bcmx_lport_t lport;
  struct hsl_avl_node *node;

#if 0
  /* Initialize STG */
  ret = hsl_bcm_stg_init ();

  if (ret != 0)
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "STG Initialization failed (%d)\n", ret);
#endif

  HSL_FN_ENTER ();

  if (! hsl_bcm_bridge_p)
    {
      hsl_bcm_bridge_p = _hsl_bcm_bridge_init ();
      if (! hsl_bcm_bridge_p)
        HSL_FN_EXIT (-1);

      /* Get default STG. */
      ret = bcmx_stg_default_get (&hsl_bcm_bridge_p->stg[0]);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Couldn't get default STG. Creating one with id 0\n");

          /* Create a new STG. */
          ret = bcmx_stg_create (&defstg);        
          if (ret < 0)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Couldn't create STG 0\n");
              HSL_FN_EXIT (-1);
            }

          /* Set default STG. */
          ret = bcmx_stg_default_set (defstg);
          if (ret < 0)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set default STG 0\n");
              HSL_FN_EXIT (-1);
            }

          /* Set default. */
          hsl_bcm_bridge_p->stg[0] = defstg;

        } 
      /* For L2 ports which are directly mapped to a L3 interface, set the port state to 
         forwarding. */
      HSL_IFMGR_LOCK;

      for (node = hsl_avl_top (HSL_IFMGR_TREE); node; node = hsl_avl_next (node))
        {
          ifp = HSL_AVL_NODE_INFO (node);
          if (! ifp)
            continue;
              
          /* If L2 port is directly mapped to a L3(router port) set the state to
             forwarding. */
          if (ifp->type == HSL_IF_TYPE_L2_ETHERNET 
              && CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
            {
              bcmifp = ifp->system_info;
              lport = bcmifp->u.l2.lport;
             
              /* Set to forward state. */
              bcmx_stg_stp_set (hsl_bcm_bridge_p->stg[0], lport, BCM_STG_STP_FORWARD);
            }
        }
      HSL_IFMGR_UNLOCK;

      HSL_FN_EXIT (0);
    }

  HSL_FN_EXIT (-1);
}

/* Bridge deinit. */
int 
hsl_bcm_bridge_deinit (struct hsl_bridge *b)
{
  HSL_FN_ENTER ();

  if (hsl_bcm_bridge_p)
    {
      /* set l2 age timer back to default of 300 sec */
      hsl_bcm_set_age_timer(b, HSL_DEFAULT_FDB_TIMEOUT);
      _hsl_bcm_bridge_deinit (&hsl_bcm_bridge_p);
    }
  HSL_FN_EXIT (0);
}

/* Set L2 ageing timer. */
int 
hsl_bcm_set_age_timer (struct hsl_bridge *b, int age)
{
  int ret;

  HSL_FN_ENTER ();

  ret = bcmx_l2_age_timer_set (age);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set the L2 ageing timer value\n");
      HSL_FN_EXIT (-1);
    }

  HSL_FN_EXIT (0);
}

/* Learning set. */
int 
hsl_bcm_set_learning (struct hsl_bridge *b, int learn)
{
  struct hsl_avl_node *node;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
  int flags, ret;

  HSL_FN_ENTER ();

  flags = (learn) ? BCM_PORT_LEARN_ARL : 0;

  for (node = hsl_avl_top (b->port_tree); node; node = hsl_avl_next (node))
    {
      ifp = HSL_AVL_NODE_INFO (node);

      if (! ifp)
        continue;

      if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
        continue;

      bcmifp = ifp->system_info;
      if (! bcmifp)
        continue;

      /* Set port to learning. */
      ret = bcmx_port_learn_set (bcmifp->u.l2.lport, flags);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Error setting port %s to learning.\n", ifp->name);
          continue;
        }
    }

  HSL_FN_EXIT (0);
}


int
_hsl_bcm_set_stp_port_state2 (struct hsl_if *ifp, int instance, int state)
{
  struct hsl_bcm_if *bcmifp = ifp->system_info;
  struct hsl_bridge_port *bridge_port;
  bcmx_lport_t lport;
  int bcm_port_state;
  int ret;

  HSL_FN_ENTER ();

  lport = bcmifp->u.l2.lport;

  switch (state)
    {
    case HAL_BR_PORT_STATE_LISTENING:
      bcm_port_state = BCM_STG_STP_LISTEN;
      break;
    case HAL_BR_PORT_STATE_LEARNING:
      bcm_port_state = BCM_STG_STP_LEARN;
      break;
    case HAL_BR_PORT_STATE_FORWARDING:
      bcm_port_state = BCM_STG_STP_FORWARD;
      break;
    case HAL_BR_PORT_STATE_BLOCKING:
    case HAL_BR_PORT_STATE_DISABLED:
      bcm_port_state = BCM_STG_STP_BLOCK;
      break;
    default:
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Invalid port state\n");
      HSL_FN_EXIT (-1);
    }

  if (hsl_bcm_bridge_p->stg[instance] != -1)
    {
      ret = bcmx_stg_stp_set (hsl_bcm_bridge_p->stg[instance], lport, 
                              bcm_port_state);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set STP port state %d for port %s retval %d\n", bcm_port_state, ifp->name, ret);
          HSL_FN_EXIT (-1);
        }
#ifdef HAVE_L2LERN
      bridge_port = ifp->u.l2_ethernet.port;
      if (bridge_port)
        bridge_port->stp_port_state = state;
#endif /* HAVE_L2LERN */
    }
  
  HSL_FN_EXIT (0);
}



/* Set STP port state. */
int 
hsl_bcm_set_stp_port_state (struct hsl_bridge *b, struct hsl_bridge_port *port, int instance, int state)
{
  struct hsl_if *ifp, *ifp_child;
  struct hsl_if_list *ln;
  struct  hsl_bcm_if *bcmifp;
  int ret;

  HSL_FN_ENTER ();

  if (! hsl_bcm_bridge_p)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Bridge not initialized\n");
      HSL_FN_EXIT (-1);
    }

  if (instance > HSL_BCM_STG_MAX_INSTANCES)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Instance number %d exceeds %d\n", instance, HSL_BCM_STG_MAX_INSTANCES);
      HSL_FN_EXIT (-1);
    }

  ifp = port->ifp;
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET && bcmifp->trunk_id >= 0)
    {
      for (ln = ifp->children_list; ln; ln = ln->next)
        {
          ifp_child = ln->ifp;
          ret = _hsl_bcm_set_stp_port_state2 (ifp_child, instance, state);
          if (ret < 0)
            HSL_FN_EXIT (-1);
        }
    }
  else
    {
      ret = _hsl_bcm_set_stp_port_state2 (ifp, instance, state);
      if (ret < 0)
        HSL_FN_EXIT (-1);
    }

  HSL_FN_EXIT (0);
}

/* Add instance. */
int 
hsl_bcm_add_instance (struct hsl_bridge *b, int instance)
{
  struct hsl_avl_node *node;
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *ifp;
  bcmx_lport_t lport;
  bcm_stg_t stg;
  int ret;

  HSL_FN_ENTER ();

  if (! hsl_bcm_bridge_p)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Bridge not initialized\n");
      HSL_FN_EXIT (-1);
    }

  if (instance > HSL_BCM_STG_MAX_INSTANCES)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Instance number %d exceeds %d\n", instance, HSL_BCM_STG_MAX_INSTANCES);
      HSL_FN_EXIT (-1);
    }

  if (hsl_bcm_bridge_p->stg[instance] != -1)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "STG %d exists for this instance %d\n", hsl_bcm_bridge_p->stg[instance], instance);
      HSL_FN_EXIT (-1);
    }

  ret = bcmx_stg_create (&stg);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't create STG\n");
      HSL_FN_EXIT (-1);
    }

  hsl_bcm_bridge_p->stg[instance] = stg;

  /* For L2 ports which are directly mapped to a L3 interface, set the port state to 
     Blocking . */
  HSL_IFMGR_LOCK;

  for (node = hsl_avl_top (HSL_IFMGR_TREE); node; node = hsl_avl_next (node))
     {
       ifp = HSL_AVL_NODE_INFO (node);
       if (! ifp)
         continue;

       /* If L2 port is directly mapped to a L3(router port) set the state to
        forwarding. */

        if (ifp->type == HSL_IF_TYPE_L2_ETHERNET 
            && ! CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
          {
            bcmifp = ifp->system_info;
            lport = bcmifp->u.l2.lport;

            /* Set to Blocking state. */
            bcmx_stg_stp_set (hsl_bcm_bridge_p->stg[instance], lport, BCM_STG_STP_BLOCK);
          }
     }

  HSL_IFMGR_UNLOCK;

  HSL_FN_EXIT (0);
}

/* Delete instance. */
int 
hsl_bcm_delete_instance (struct hsl_bridge *b, int instance)
{
  int ret;

  HSL_FN_ENTER ();

  if (! hsl_bcm_bridge_p)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Bridge not initialized\n");
      HSL_FN_EXIT (-1);
    }

  if (instance > HSL_BCM_STG_MAX_INSTANCES)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Instance number %d exceeds %d\n", instance, HSL_BCM_STG_MAX_INSTANCES);
      HSL_FN_EXIT (-1);
    }

  if (hsl_bcm_bridge_p->stg[instance] == -1)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "STG %d doesn't exist for this instance %d\n", hsl_bcm_bridge_p->stg[instance], instance);
      HSL_FN_EXIT (-1);
    }

  ret = bcmx_stg_destroy (hsl_bcm_bridge_p->stg[instance]);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Error deleting STG %d\n", hsl_bcm_bridge_p->stg[instance]);
      HSL_FN_EXIT (-1);
    }

  hsl_bcm_bridge_p->stg[instance] = -1;
  HSL_FN_EXIT (0);
}

int
hsl_bcm_add_port_to_bridge (struct hsl_bridge *b, u_int32_t ifindex)
{
  int ret;
  bcmx_lplist_t t, u;
  bcmx_lport_t lport;
  struct hsl_if *ifp = NULL;
  struct hsl_bcm_if *bcmifp = NULL;

  if (CHECK_FLAG (b->flags, HSL_BRIDGE_VLAN_AWARE))
    return 0;

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

  if (bcmx_lplist_init (&t, 1, 0) < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Out of memory\n");
      HSL_FN_EXIT (-1);
    }

  if (bcmx_lplist_init (&u, 1, 0) < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Out of memory\n");
      bcmx_lplist_free (&t);
      HSL_FN_EXIT (-1);
    }

  bcmx_lplist_add (&t, lport);
  bcmx_lplist_add (&u, lport);

  ret = bcmx_port_untagged_vlan_set (lport, HSL_DEFAULT_VID);

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't set default PVID %d for port %s\n", HSL_DEFAULT_VID,
                ifp->name);
      HSL_FN_EXIT (-1);
    }

  /* Add the default vlan */
  ret = bcmx_vlan_port_add (HSL_DEFAULT_VID, t, u);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Failed to add default vlan on interface %d, "
               "bcm error = %d\n", ifindex, ret);
      HSL_FN_EXIT (-1);
    }

  return 0;

}

int 
hsl_bcm_delete_port_from_bridge (struct hsl_bridge *b, u_int32_t ifindex)
{

#ifdef HAVE_PROVIDER_BRIDGE
  if (b->type == HAL_BRIDGE_PROVIDER_RSTP
       || b->type == HAL_BRIDGE_PROVIDER_MSTP)
    {
      hsl_bcm_provider_vlan_disable (ifindex);
      HSL_FN_EXIT (0);
    }
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_VLAN_STACK
    hsl_bcm_provider_vlan_disable (ifindex);
#endif /* HAVE_VLAN_STACK */
  
  HSL_FN_EXIT (0);
}

#ifdef HAVE_VLAN
/* Add VID to instance. */
int 
hsl_bcm_add_vlan_to_instance (struct hsl_bridge *b, int instance, hsl_vid_t vid)
{
  struct hsl_avl_node *node;
  struct hsl_bcm_if *bcmifp;
  struct hsl_if *ifp;
  bcmx_lport_t lport;
  bcm_stg_t stg;
  int ret;

  HSL_FN_ENTER ();

  if (! hsl_bcm_bridge_p)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Bridge not initialized\n");
      HSL_FN_EXIT (-1);
    }

  if (instance > HSL_BCM_STG_MAX_INSTANCES)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Instance number %d exceeds %d\n", instance, HSL_BCM_STG_MAX_INSTANCES);
      HSL_FN_EXIT (-1);
    }

  if (hsl_bcm_bridge_p->stg[instance] == -1)
    {
      ret = bcmx_stg_create (&stg);

      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Error creating STG %d\n", hsl_bcm_bridge_p->stg[instance]);
          HSL_FN_EXIT (-1);
        }

      hsl_bcm_bridge_p->stg[instance] = stg;

      /* For L2 ports which are directly mapped to a L3 interface, set the port state to 
         Blocking . */
      HSL_IFMGR_LOCK;

      for (node = hsl_avl_top (HSL_IFMGR_TREE); node; node = hsl_avl_next (node))
         {
           ifp = HSL_AVL_NODE_INFO (node);
           if (! ifp)
             continue;

           /* If L2 port is directly mapped to a L3(router port) set the state to
            forwarding. */

            if (ifp->type == HSL_IF_TYPE_L2_ETHERNET 
                && ! CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
              {
                bcmifp = ifp->system_info;
                lport = bcmifp->u.l2.lport;

                /* Set to Blocking state. */
                bcmx_stg_stp_set (hsl_bcm_bridge_p->stg[instance], lport, BCM_STG_STP_BLOCK);
              }
         }

      HSL_IFMGR_UNLOCK;
    }

  ret = bcmx_stg_vlan_add (hsl_bcm_bridge_p->stg[instance], vid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Error adding VLAN %d to STG %d\n", vid, hsl_bcm_bridge_p->stg[instance]);
      HSL_FN_EXIT (-1);
    }

  HSL_FN_EXIT (0);
}

/* Delete VID from instance. */
int 
hsl_bcm_delete_vlan_from_instance (struct hsl_bridge *b, int instance, hsl_vid_t vid)
{
  int ret;

  HSL_FN_ENTER ();

  if (! hsl_bcm_bridge_p)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Bridge not initialized\n");
      HSL_FN_EXIT (-1);
    }

  if (instance > HSL_BCM_STG_MAX_INSTANCES)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Instance number %d exceeds %d\n", instance, HSL_BCM_STG_MAX_INSTANCES);
      HSL_FN_EXIT (-1);
    }

  if (hsl_bcm_bridge_p->stg[instance] == -1)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "STG %d doesn't exist for this instance %d\n", hsl_bcm_bridge_p->stg[instance], instance);
      HSL_FN_EXIT (-1);
    }

  ret = bcmx_stg_vlan_remove (hsl_bcm_bridge_p->stg[instance], vid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Error adding VLAN %d to STG %d\n", vid, hsl_bcm_bridge_p->stg[instance]);
      HSL_FN_EXIT (-1);
    }

  HSL_FN_EXIT (0);
}

/* Add VLAN. */
int 
hsl_bcm_add_vlan (struct hsl_bridge *b, struct hsl_vlan_port *v)
{
  int ret;

  HSL_FN_ENTER ();

  if (HSL_VLAN_DEFAULT_VID == v->vid)
    HSL_FN_EXIT (0);

  /* Check if the VID is part of the reserved vlan. */
  if (hsl_bcm_resv_vlan_is_allocated (v->vid))
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't add VLAN %d. Reserved in hardware\n", v->vid);
      HSL_FN_EXIT (HSL_ERR_BRIDGE_VLAN_RESERVED_IN_HW);
    }
  
  ret = bcmx_vlan_create (v->vid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't add VLAN %d\n", v->vid);
      HSL_FN_EXIT (-1);
    }

  /* Add CPU port to the VLAN. */
  hsl_bcm_add_port_to_vlan (v->vid, BCMX_LPORT_LOCAL_CPU, 1);

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  /* Set the vlan multicast filtering mode*/
  if (p_hsl_bridge_master->hsl_l2_unknown_mcast == HSL_BRIDGE_L2_UNKNOWN_MCAST_DISCARD)
    bcmx_vlan_mcast_flood_set (v->vid, BCM_VLAN_MCAST_FLOOD_NONE);
  else 
    bcmx_vlan_mcast_flood_set (v->vid, BCM_VLAN_MCAST_FLOOD_UNKNOWN);
  
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
  
  HSL_FN_EXIT (0);
}

/* Delete VLAN. */
int 
hsl_bcm_delete_vlan (struct hsl_bridge *b, struct hsl_vlan_port *v)
{
  int ret;

  HSL_FN_ENTER ();
  
  if (HSL_VLAN_DEFAULT_VID == v->vid)
    HSL_FN_EXIT (0);

  ret = bcmx_vlan_destroy (v->vid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't delete VLAN %d\n", v->vid);
    }

  HSL_FN_EXIT (0);
}

/* Set port type type. */
int 
hsl_bcm_set_vlan_port_type (struct hsl_bridge *b, struct hsl_bridge_port *port, 
                            enum hal_vlan_port_type port_type, 
                            enum hal_vlan_port_type port_sub_type, 
                            enum hal_vlan_acceptable_frame_type acceptable_frame_types, 
                            u_int16_t enable_ingress_filter)
{
  bcmx_lport_t lport;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
  int ret;

  HSL_FN_ENTER ();

  ifp = port->ifp;

  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  bcmifp = ifp->system_info;

  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  lport = bcmifp->u.l2.lport;

#ifdef HAVE_PROVIDER_BRIDGE
  if (b->type == HAL_BRIDGE_PROVIDER_RSTP
       || b->type == HAL_BRIDGE_PROVIDER_MSTP)
    hsl_bcm_vlan_set_provider_port_type (ifp->ifindex, port_type);
#endif /* HAVE_PROVIDER_BRIDGE */

    ret = bcmx_port_discard_set (lport, BCM_PORT_DISCARD_NONE);
    if (ret < 0)
      {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
             "Set port (%d)Accept Frame All failed \n", port_type);
             HSL_FN_EXIT (STATUS_ERROR);
      }

  switch (port_type)
    {
    case HAL_VLAN_TRUNK_PORT:

      ret=  bcmx_port_discard_set (lport, BCM_PORT_DISCARD_UNTAG);

      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
              "Set port (%d)Accept Frame failed\n",port_type);
          HSL_FN_EXIT (STATUS_ERROR);
        }
      break;

    case HAL_VLAN_ACCESS_PORT:
      ret=  bcmx_port_discard_set (lport, BCM_PORT_DISCARD_TAG);
      if (ret < 0)
    {
           HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Set port (%d)Accept Frame failed\n",port_type);
           HSL_FN_EXIT (STATUS_ERROR);
        }
      
      break;

    case HAL_VLAN_HYBRID_PORT:

      if (acceptable_frame_types == HAL_VLAN_ACCEPTABLE_FRAME_TYPE_TAGGED)
        {
          /* Discard the untagged packets */
          ret= bcmx_port_discard_set (lport, BCM_PORT_DISCARD_UNTAG);
          if (ret < 0)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
                  "Set port (%d)Accept Frame failed accept_frame_type %d\n",
                  port_type, acceptable_frame_types);
              HSL_FN_EXIT (STATUS_ERROR);
        }
        }
      /* Else accept all pkts.Discard None is already set before switch case*/
      break;

    default:
      ret = bcmx_port_discard_set (lport, BCM_PORT_DISCARD_NONE);
      if(ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
              "Set port (%d)Accept Frame failed\n");
          HSL_FN_EXIT (STATUS_ERROR);
        }
    }

#ifdef HAVE_L2LERN
  port->type = port_type;
  port->sub_type = port_sub_type;
#endif /* HAVE_L2LERN */
  
  if (enable_ingress_filter)
    bcmx_port_ifilter_set (lport, BCM_PORT_IFILTER_ON);
  else
    bcmx_port_ifilter_set (lport, BCM_PORT_IFILTER_OFF);

  HSL_FN_EXIT (0);
}

/* Set default PVID. */
int 
hsl_bcm_set_default_pvid (struct hsl_bridge *b, struct hsl_port_vlan *port_vlan, int egress)
{
  bcmx_lport_t lport;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
  int ret;
  struct hsl_bridge_port *port;

  HSL_FN_ENTER ();

  port = port_vlan->port;
  if (! port)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  ifp = port->ifp;
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  lport = bcmifp->u.l2.lport;

  ret = bcmx_port_untagged_vlan_set (lport, port_vlan->pvid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set default PVID %d for port %s\n", port_vlan->pvid, ifp->name);
      HSL_FN_EXIT (-1);
    }
  
  /* XXX egress tagged? */
  HSL_FN_EXIT (0);
}

/* Add VID to port. */
int 
hsl_bcm_add_vlan_to_port (struct hsl_bridge *b, struct hsl_port_vlan *port_vlan, hsl_vid_t vid,
                          enum hal_vlan_egress_type egress)
{
  bcmx_lport_t lport;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
  int ret;
  struct hsl_bridge_port *port;
#ifdef HAVE_PVLAN
  struct hsl_vlan_port *vlan_port;
  struct hsl_vlan_port tv;
  struct hsl_avl_node *node;
  struct hsl_bridge_port *bridge_port;
  struct hsl_bcm_pvlan *bcm_pvlan;
  bcmx_lport_t lport_egress;
  bcmx_lplist_t plist;
  bcmx_lplist_t plist_trunk;
#endif /* HAVE_PVLAN */

  HSL_FN_ENTER ();

  port = port_vlan->port;
  if (! port)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  ifp = port->ifp;
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  lport = bcmifp->u.l2.lport;

  /* Add VLAN to the port. */
  if (egress == HAL_VLAN_EGRESS_TAGGED)
    ret = hsl_bcm_add_port_to_vlan (vid, lport, 1);
  else
    ret = hsl_bcm_add_port_to_vlan (vid, lport, 0);

#ifdef HAVE_PVLAN
  /* For interlink switchports block traffic to host ports of isolated vlan */
  if (egress == HAL_VLAN_EGRESS_TAGGED)
    {
      tv.vid = vid;

      node = hsl_avl_lookup (b->vlan_tree, &tv);

      if (!node)
        {
          HSL_FN_EXIT (ret);
        }
 
      vlan_port = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);

      bcm_pvlan = (struct hsl_bcm_pvlan *)vlan_port->system_info;

      /* Private vlan not configured on this vlan */
      if (!bcm_pvlan)
        HSL_FN_EXIT (0);

      if (bcm_pvlan->vlan_type == HAL_PVLAN_NONE)
        HSL_FN_EXIT (0);

      bcmx_lplist_init (&plist_trunk, 0, 0);
      bcmx_port_egress_get (lport, -1, &plist_trunk);

      for (node = hsl_avl_top (vlan_port->port_tree); node; node = hsl_avl_next (node))
        {
          bcmx_lplist_init (&plist, 0, 0);
          ifp = HSL_AVL_NODE_INFO (node);
          if (!ifp)
            continue;

          bcmifp = (struct hsl_bcm_if *)ifp->system_info;
          if (!bcmifp)
            continue;

          bridge_port = ifp->u.l2_ethernet.port;
          lport_egress = bcmifp->u.l2.lport;
          if ((lport_egress != lport) && (bridge_port->pvlan_port_mode == HAL_PVLAN_PORT_MODE_HOST))
            {
              bcmx_port_egress_get (lport_egress, -1, &plist);
              bcmx_lplist_add (&plist, lport);
              bcmx_port_egress_set (lport_egress, -1, plist);
              /* Remove the isolated port from egress list of trunk port */
              bcmx_lplist_port_remove (&plist_trunk, lport_egress, 1);

            }
           bcmx_lplist_free (&plist);
        }
      bcmx_port_egress_set (lport, -1, plist_trunk);
      bcmx_lplist_free (&plist_trunk);
    }
#endif /* HAVE_PVLAN */
  HSL_FN_EXIT (ret);
}

/* Delete VID from port. */
int 
hsl_bcm_delete_vlan_from_port (struct hsl_bridge *b, struct hsl_port_vlan *port_vlan, hsl_vid_t vid)
{
  bcmx_lport_t lport;
  struct hsl_if *ifp;
  struct hsl_bcm_if *bcmifp;
  int ret;
  struct hsl_bridge_port *port;
#ifdef HAVE_PVLAN
  struct hsl_vlan_port *vlan_port;
  struct hsl_vlan_port tv;
  struct hsl_avl_node *node;
  struct hsl_bridge_port *bridge_port;
  bcmx_lport_t lport_tmp, lport_egress;
  struct hsl_bcm_pvlan *bcm_pvlan;
  bcmx_lplist_t plist_trunk;
#endif /* HAVE_PVLAN */

  HSL_FN_ENTER ();

  port = port_vlan->port;
  if (! port)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  ifp = port->ifp;
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Interface not set\n");
      HSL_FN_EXIT (-1);
    }

  lport = bcmifp->u.l2.lport;

  /* Remove VLAN from port. */
  ret = hsl_bcm_remove_port_from_vlan (vid, lport);

#ifdef HAVE_PVLAN
  tv.vid = vid;

  node = hsl_avl_lookup (b->vlan_tree, &tv);

  if (!node)
    {
      HSL_FN_EXIT (ret);
    }

  vlan_port = (struct hsl_vlan_port *) HSL_AVL_NODE_INFO (node);

  if (!vlan_port)
    HSL_FN_EXIT (-1);

  bcm_pvlan = (struct hsl_bcm_pvlan *)vlan_port->system_info;

  /* Private vlan not configured on this vlan */
  if (!bcm_pvlan)
    HSL_FN_EXIT (0);

  if (bcm_pvlan->vlan_type == HAL_PVLAN_NONE)
    HSL_FN_EXIT (0);

  /* For host, promiscuous ports, filtering is taken care before itself */
  if (port_vlan->pvlan_port_mode != HAL_PVLAN_PORT_MODE_INVALID)
    HSL_FN_EXIT (0);

  bcmx_lplist_init (&plist_trunk, 0, 0);

  /* Add all the ports for egressing */
  BCMX_FOREACH_LPORT (lport_tmp)
    {
      bcmx_lplist_add (&plist_trunk, lport_tmp);
    }
  /* Remove specific isolated vlan ports for egressing */
  for (node = hsl_avl_top (vlan_port->port_tree); node; node = hsl_avl_next (node))
    {
      ifp = HSL_AVL_NODE_INFO (node);
      if (!ifp)
        continue;

      bcmifp = (struct hsl_bcm_if *)ifp->system_info;
      if (!bcmifp)
        continue;

      bridge_port = ifp->u.l2_ethernet.port;
      lport_egress = bcmifp->u.l2.lport;

      /* Add the new trunk port to be part of egress port list for secondary vlan isolated ports */
      if ((lport_egress != lport) && (bridge_port->pvlan_port_mode == HAL_PVLAN_PORT_MODE_HOST))
        {
          /* Remove the isolated port from egress list of trunk/access port */
          bcmx_lplist_port_remove (&plist_trunk, lport_egress, 1);
        }
    }
  bcmx_port_egress_set (lport, -1, plist_trunk);
  bcmx_lplist_free (&plist_trunk);
#endif /* HAVE_PVLAN */

  HSL_FN_EXIT (ret);
}

#endif /* HAVE_VLAN */  

#ifdef HAVE_IGMP_SNOOP

/* Init IGMP snooping. */
int
hsl_bcm_init_igmp_snooping ()
{
  int i, bcm_unit;

  HSL_FN_ENTER ();

  BCMX_UNIT_ITER (bcm_unit, i)
    {
      if (BCM_UNIT_VALID (bcm_unit))
        bcm_igmp_snooping_init (bcm_unit);
    }

  HSL_FN_EXIT (0);
}

/* Deinit IGMP snooping. */
int
hsl_bcm_deinit_igmp_snooping ()
{
  HSL_FN_EXIT (0);
}

int
_hsl_bcm_install_igmp_rule (int enable_flag, int msg,
                            int router_alert_msg)
{
  bcm_filterid_t fid;
  int ret;

  HSL_FN_ENTER ();

  ret = bcmx_filter_create (&fid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create IGMP Snooping filter\n");
      HSL_FN_EXIT (-1);
    }

  bcmx_filter_qualify_data8 (fid, DEST_MAC_OUI_1, IGMP_MAC_OUI_1, 0xff);
  bcmx_filter_qualify_data8 (fid, DEST_MAC_OUI_2, IGMP_MAC_OUI_2, 0xff);
  bcmx_filter_qualify_data8 (fid, DEST_MAC_OUI_3, IGMP_MAC_OUI_3, 0xff);
  bcmx_filter_qualify_data8 (fid, IP_PROTOCOL_OFFSET, IGMP_PROTOCOL, 0xff);

  if (msg >= 0)
    {
      bcmx_filter_qualify_data8 (fid, IGMP_MSG_OFFSET, msg, 0xff);
    }

  if (router_alert_msg >= 0)
    {
      /* Setup mask and option header for router alerts */
      bcmx_filter_qualify_data32 (fid, IP_OPTIONS_OFFSET,
                                  (ROUTER_ALERT1 << 24 |
                                   ROUTER_ALERT2 << 16 |
                                   ROUTER_ALERT3 << 8  |
                                   ROUTER_ALERT4),
                                  0xffffffff);
      bcmx_filter_qualify_data8 (fid, ROUTER_ALERT_IGMP_OFFSET,
                                 router_alert_msg, 0xff);
    }

  /* Define processing rules */
  bcmx_filter_action_match (fid, bcmActionCopyToCpu, 0);
  bcmx_filter_action_match (fid, bcmActionDoNotSwitch, 0);

  if (enable_flag)
    ret = bcmx_filter_install (fid);
  else
    ret = bcmx_filter_remove (fid);

  bcmx_filter_destroy (fid);

  HSL_FN_EXIT (ret);
}

void
_hsl_bcm_uninstall_igmp_field ()
{

  if (_hsl_igmp_snp_field_ent != HSL_IGS_BCM_INVALID_FIELD_ENTRY) 
    bcmx_field_entry_destroy (_hsl_igmp_snp_field_ent);

  if (_hsl_igmp_snp_field_grp != HSL_IGS_BCM_INVALID_FIELD_GRP)
    bcmx_field_group_destroy (_hsl_igmp_snp_field_grp);

  _hsl_igmp_snp_field_ent = HSL_IGS_BCM_INVALID_FIELD_ENTRY;
  _hsl_igmp_snp_field_grp = HSL_IGS_BCM_INVALID_FIELD_GRP;
}

int
_hsl_bcm_install_igmp_field ()
{
  int ret;
  bcm_field_qset_t qset;
  bcmx_lplist_t plist;

  HSL_FN_ENTER ();

  BCM_FIELD_QSET_INIT(qset);

  BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyIpProtocol);
  BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyPacketFormat);

  /* Init the plist*/
  ret = bcmx_lplist_init (&plist, 0, 0);
  if (ret != BCM_E_NONE)
    return ret;

  /* Add the CPU port in the list */
  ret = bcmx_lplist_add(&plist, BCMX_LPORT_LOCAL_CPU);
  if (ret != BCM_E_NONE)
    goto ERR;
  
  ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY,
                                 &_hsl_igmp_snp_field_grp);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create IGMP Snooping field group\n");
      goto ERR;
    }

  ret = bcmx_field_entry_create (_hsl_igmp_snp_field_grp, &_hsl_igmp_snp_field_ent);
  if (ret != BCM_E_NONE)
   {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
              "Can't create IGMP Snooping field entry\n");
      goto ERR;
   }

  /* Qualify IP Protocol */
  ret = bcmx_field_qualify_IpProtocol (_hsl_igmp_snp_field_ent,
                                       IGMP_PROTOCOL,
                                       0xff);
  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create IGMP protocol qualifier\n");
    }

  ret = bcmx_field_action_add (_hsl_igmp_snp_field_ent,
                               bcmFieldActionPrioIntNew, 7, 0);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't set IGMP Snooping field entry internal priority\n");
    }

  ret = bcmx_field_action_ports_add (_hsl_igmp_snp_field_ent,
                                     bcmFieldActionRedirectPbmp, plist);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't set IGMP Snooping field entry Redirect action\n");
    }

  ret = bcmx_field_entry_install (_hsl_igmp_snp_field_ent);

  if (ret != BCM_E_NONE)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't install IGMP Snooping field entry\n");
    }

  HSL_FN_EXIT (0);

ERR:

  _hsl_bcm_uninstall_igmp_field ();
  HSL_FN_EXIT (ret);
}

int
hsl_bcm_igmp_snooping_set (int enable_flag)
{
  HSL_FN_ENTER ();


  if (hsl_bcm_filter_type_get() == HSL_BCM_FEATURE_FILTER)
    {
      /*  Install rules for normal IGMP messages. */
      _hsl_bcm_install_igmp_rule (enable_flag, IGMP_V1_REPORT, -1);
      _hsl_bcm_install_igmp_rule (enable_flag, IGMP_V2_REPORT, -1);
      _hsl_bcm_install_igmp_rule (enable_flag, IGMP_V3_REPORT, -1);
      _hsl_bcm_install_igmp_rule (enable_flag, IGMP_QUERY, -1);
      _hsl_bcm_install_igmp_rule (enable_flag, IGMP_LEAVE, -1);

      /* Install rules for IGMP router alert messages. */
      _hsl_bcm_install_igmp_rule (enable_flag, -1, IGMP_V1_REPORT);
      _hsl_bcm_install_igmp_rule (enable_flag, -1, IGMP_V2_REPORT);
      _hsl_bcm_install_igmp_rule (enable_flag, -1, IGMP_V3_REPORT);
      _hsl_bcm_install_igmp_rule (enable_flag, -1, IGMP_QUERY);
      _hsl_bcm_install_igmp_rule (enable_flag, -1, IGMP_LEAVE);
    }
  else if (hsl_bcm_filter_type_get() == HSL_BCM_FEATURE_FIELD)
    {
      if (enable_flag)
        _hsl_bcm_install_igmp_field ();
      else
        _hsl_bcm_uninstall_igmp_field ();
    }

  HSL_FN_EXIT (0);
}

/* Enable IGMP snooping. */
int 
hsl_bcm_enable_igmp_snooping (struct hsl_bridge *b)
{
  int ret;

  HSL_FN_ENTER ();
#ifdef HAVE_IGMP_SNOOP
  ret = hsl_bcm_igmp_snooping_set (1);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't enable IGMP Snooping\n");
      HSL_FN_EXIT (-1);
    }
#endif /* HAVE_IGMP_SNOOP */
  HSL_FN_EXIT (0);
}

/* Disable IGMP snooping. */
int 
hsl_bcm_disable_igmp_snooping (struct hsl_bridge *b)
{
  int ret;

  HSL_FN_ENTER ();

  ret = hsl_bcm_igmp_snooping_set (0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't disable IGMP Snooping\n");
    }
  HSL_FN_EXIT (0);
}
#endif /* HAVE_IGMP_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
/* Set vlan multicast filtering mode to discard */
int
hsl_bcm_l2_unknown_mcast_mode (hsl_vid_t vid, int mode)
{
    HSL_FN_ENTER ();

    /* Set vlan filtering mode for multicast packets. */
    if (mode)
      bcmx_vlan_mcast_flood_set (vid, BCM_VLAN_MCAST_FLOOD_NONE);
    else
      bcmx_vlan_mcast_flood_set (vid, BCM_VLAN_MCAST_FLOOD_UNKNOWN);
    
    HSL_FN_EXIT (0);
}
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_IGMP_SNOOP
/* Enable IGMP snooping on port. */
int
hsl_bcm_enable_igmp_snooping_port (struct hsl_bridge *b, struct hsl_if *ifp)
{
  HSL_FN_ENTER ();
  HSL_FN_EXIT (0);
}

/* Disable IGMP snooping on port. */
int
hsl_bcm_disable_igmp_snooping_port (struct hsl_bridge *b, struct hsl_if *ifp)
{
  HSL_FN_ENTER ();
  HSL_FN_EXIT (0);
}

#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP

/* Init MLD snooping. */
int
hsl_bcm_init_mld_snooping ()
{
  HSL_FN_EXIT (0);
}

/* Deinit MLD snooping. */
int
hsl_bcm_deinit_mld_snooping ()
{
  HSL_FN_EXIT (0);
}

int
_hsl_bcm_install_mld_rule (int enable_flag, int msg)
{
  bcm_filterid_t fid;
  int ret;

  HSL_FN_ENTER ();

  ret = bcmx_filter_create (&fid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
               "Can't create MLD Snooping filter\n");
      HSL_FN_EXIT (-1);
    }

  bcmx_filter_qualify_data8 (fid, DEST_MAC_OUI_1, MLD_MAC_OUI_1, 0xff);
  bcmx_filter_qualify_data8 (fid, DEST_MAC_OUI_2, MLD_MAC_OUI_2, 0xff);
  bcmx_filter_qualify_data8 (fid, IPV6_PROTOCOL_OFFSET, ICMPV6_PROTOCOL, 0xff);

  if (msg >= 0)
    {
      bcmx_filter_qualify_data8 (fid, MLD_OFFSET, msg, 0xff);
    }

  /* Define processing rules */
  bcmx_filter_action_match (fid, bcmActionCopyToCpu, 0);
  bcmx_filter_action_match (fid, bcmActionDoNotSwitch, 0);

  if (enable_flag)
    ret = bcmx_filter_install (fid);
  else
    ret = bcmx_filter_remove (fid);

  bcmx_filter_destroy (fid);

  HSL_FN_EXIT (0);
}

int
_hsl_bcm_mld_snooping_set (int enable_flag)
{
  HSL_FN_ENTER ();

  /*  Install rules for normal MLD messages. */
  _hsl_bcm_install_mld_rule (enable_flag, MLD_LISTENER_QUERY);
  _hsl_bcm_install_mld_rule (enable_flag, MLD_LISTENER_REPORT);
  _hsl_bcm_install_mld_rule (enable_flag, MLDV2_LISTENER_REPORT);

  HSL_FN_EXIT (0);
}

/* Enable MLD snooping. */
int
hsl_bcm_enable_mld_snooping (struct hsl_bridge *b)
{
  int ret;

  HSL_FN_ENTER ();

  ret = _hsl_bcm_mld_snooping_set (1);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't enable MLD Snooping\n");
      HSL_FN_EXIT (-1);
    }
  HSL_FN_EXIT (0);
}

/* Disable MLD snooping. */
int
hsl_bcm_disable_mld_snooping (struct hsl_bridge *b)
{
  /* XXX: Do not disable MLD snooping for L2/L3 multicast to work
   * properly.
   */
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

#endif /* HAVE_MLD_SNOOP */

/* Ratelimit init. */
int
hsl_bcm_ratelimit_init (void)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/* Ratelimit deinit. */
int
hsl_bcm_ratelimit_deinit (void)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}
/* djg20140104*****Rate bandwidth limiting set. */
int
hsl_bcm_ratebwlimit_set (struct hsl_if *ifp, int flags, unsigned int level,
                         unsigned int fraction)
{
  struct hsl_bcm_if *bcmifp;
  int ret;
  unsigned int kbits = 0;
  unsigned int kbits_burst = 0;
  bcmx_lport_t lport;

  HSL_FN_ENTER ();

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }
  lport = bcmifp->u.l2.lport;

  ret = bcmx_rate_bandwidth_set(lport, flags, level, fraction);
  if (ret < 0)
  {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set ratebwlimit for broadcast on port %s, level %d, burst %d\n", ifp->name, level, fraction);
      return HSL_ERR_BRIDGE_RATELIMIT;
  }
    
  HSL_FN_EXIT (0);
}


/* Rate limiting for bcast. */
int
hsl_bcm_ratelimit_bcast (struct hsl_if *ifp, int level,
                         int fraction)
{
  struct hsl_bcm_if *bcmifp;
  int speed;
  int ret;
  unsigned int pps = 0;
  bcmx_lport_t lport;

  HSL_FN_ENTER ();

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }
  lport = bcmifp->u.l2.lport;

  if ( (level == 0) && (fraction == 0) )
    bcmx_rate_bcast_set (0, BCM_RATE_BCAST, lport);
  else if ( (level == 100) && (fraction == 0) )
    {
      /* Disable storm control */
      bcmx_rate_bcast_set (0, 0, lport);
    }
  else
    {
      /* Get speed for the port. Units are Mbps */
      ret = bcmx_port_speed_get (lport, &speed);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't get speed for port %s\n", ifp->name);
          HSL_FN_EXIT (-1);
        }
      
      /* Use max size of packets (1512) to determine the number of packets per second(pps). */
      if (level == 0)
        {
          pps = (speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8); 
          pps = ((pps * fraction)/100)/100;
        }
      if (fraction == 0)
        pps = (((speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8)) * level)/100;
      if ( (level != 0) && (fraction != 0) )
        {
          pps = (speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8); 
          pps = (((pps * fraction)/100) + (pps * level))/100; 
        }
      ret = bcmx_rate_bcast_set (pps, BCM_RATE_BCAST, lport);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set ratelimit for broadcast on port %s\n", ifp->name);
          return HSL_ERR_BRIDGE_RATELIMIT;
        }
    }

  HSL_FN_EXIT (0);
}

/*
  Get bcast discards.
*/
int
hsl_bcm_ratelimit_get_bcast_discards (struct hsl_if *ifp, int *discards)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/* Rate limiting for mcast. */
int
hsl_bcm_ratelimit_mcast (struct hsl_if *ifp, int level,
                         int fraction)
{
  struct hsl_bcm_if *bcmifp;
  int speed;
  int ret;
  unsigned int pps = 0;
  bcmx_lport_t lport;

  HSL_FN_ENTER ();

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  lport = bcmifp->u.l2.lport;

  if ( (level == 0) && (fraction == 0) )
    bcmx_rate_mcast_set (0, BCM_RATE_MCAST, lport);
  else if ( (level == 100) && (fraction == 0) )
    {
      /* Disable storm control */
      bcmx_rate_mcast_set (0, 0, lport);
    }
  else
    {
      /* Get speed for the port. Units are Mbps */
      ret = bcmx_port_speed_get (lport, &speed);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't get speed for port %s\n", ifp->name);
          HSL_FN_EXIT (-1);
        }
      
      /* Use max size of packets (1512) to determine the number of packets per second(pps). */
      if (level == 0)
        {
          pps = (speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8); 
          pps = ((pps * fraction)/100)/100;
        }
      if (fraction == 0)
        pps = (((speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8)) * level)/100;
      if ( (level != 0) && (fraction != 0) )
        {
          pps = (speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8); 
          pps = (((pps * fraction)/100) + (pps * level))/100; 
        }
      ret = bcmx_rate_mcast_set (pps, BCM_RATE_MCAST, lport);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set ratelimit for multicast on port %s\n", ifp->name);
          return HSL_ERR_BRIDGE_RATELIMIT;
        }
    }

  HSL_FN_EXIT (0);
}

/*
  Get mcast discards.
*/
int
hsl_bcm_ratelimit_get_mcast_discards (struct hsl_if *ifp, int *discards)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/* Rate limiting for dlf bcast. */
int
hsl_bcm_ratelimit_dlf_bcast (struct hsl_if *ifp, int level,
                             int fraction)
{
  struct hsl_bcm_if *bcmifp;
  int speed;
  int ret;
  unsigned int pps = 0;
  bcmx_lport_t lport = 0;

  HSL_FN_ENTER ();

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  lport = bcmifp->u.l2.lport;

  if ( (level == 0) && (fraction == 0) )
    bcmx_rate_dlfbc_set (0, BCM_RATE_DLF, lport);
  else if ( (level == 100) && (fraction == 0) )
    {
      /* Disable storm control */
      bcmx_rate_dlfbc_set (0, 0, lport);
    }
  else
    {
      /* Get speed for the port. Units are Mbps  */
      ret = bcmx_port_speed_get (lport, &speed);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
                   "Can't get speed for port %s\n", ifp->name);
          HSL_FN_EXIT (-1);
        }
      
      /* Use max size of packets (1512) to determine the number of 
         packets per second(pps). */
      if (level == 0)
        {
          pps = (speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8); 
          pps = ((pps * fraction)/100)/100;
        }
      if (fraction == 0)
        pps = (((speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8)) * level)/100;
      if ( (level != 0) && (fraction != 0) )
        {
          pps = (speed * HSL_IF_BW_UNIT_MEGA) / (1512 * 8); 
          pps = (((pps * fraction)/100) + (pps * level))/100; 
        }
      ret = bcmx_rate_dlfbc_set (pps, BCM_RATE_DLF, lport);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
                   "Can't set ratelimit for DLF broadcast on port %s\n", 
                   ifp->name);
          return HSL_ERR_BRIDGE_RATELIMIT;
        }
    }

  HSL_FN_EXIT (0);
}

/*
  Get dlf bcast discards.
*/
int
hsl_bcm_ratelimit_get_dlf_bcast_discards (struct hsl_if *ifp, int *discards)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/* Flowcontrol init. */
int
hsl_bcm_flowcontrol_init (void)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/* Flowcontrol deinit. */
int
hsl_bcm_flowcontrol_deinit (void)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/* Set flowcontrol. */
int
hsl_bcm_set_flowcontrol (struct hsl_if *ifp, u_char direction)
{
  struct hsl_bcm_if *bcmifp;
  int ret;
  bcmx_lport_t lport;
  int rxpause, txpause;

  HSL_FN_ENTER ();

  rxpause = 0;
  txpause = 0;

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  lport = bcmifp->u.l2.lport;

  if (direction == HAL_FLOW_CONTROL_OFF)
    {
      ret = bcmx_port_pause_set (lport, 0, 0);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't unset flowcontrol for port %s\n", ifp->name);
          HSL_FN_EXIT (-1);
        }
      return 0;
    }

  if (direction & HAL_FLOW_CONTROL_SEND)
    txpause = 1;
   
  if (direction & HAL_FLOW_CONTROL_RECEIVE)
    rxpause = 1;

  ret = bcmx_port_pause_set (lport, txpause, rxpause);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't set flowcontrol for port %s\n", ifp->name);
      HSL_FN_EXIT (-1);
    }
  
  HSL_FN_EXIT (0);
}

/*
  Get flowcontrol statistics.
*/
int
hsl_bcm_flowcontrol_statistics (struct hsl_if *ifp, u_char *direction,
                                int *rxpause, int *txpause)
{
  struct hsl_bcm_if *bcmifp;
  int ret;
  bcmx_lport_t lport;
  int rx, tx;
  uint64 rxv, txv;
  
  HSL_FN_ENTER ();

  tx = 0;
  rx = 0;

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  lport = bcmifp->u.l2.lport;

  /* Get flowcontrol direction. */
  ret = bcmx_port_pause_get (lport, &tx, &rx);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't get flow control direction\n");
      HSL_FN_EXIT (-1);
    }

  if (tx)
    *direction |= HAL_FLOW_CONTROL_SEND;

  if (rx)
    *direction |= HAL_FLOW_CONTROL_RECEIVE;

  /* Get stats. */
  bcmx_stat_get (lport, snmpDot3InPauseFrames, &rxv);
  bcmx_stat_get (lport, snmpDot3OutPauseFrames, &txv);

  /* Possible loss of data. */
  *rxpause = (int) rxv;
  *txpause = (int) txv;

  HSL_FN_EXIT (0);
}
/* 
   FDB init.
*/
int
hsl_bcm_fdb_init (void)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/*
  FDB deinit. 
*/
int
hsl_bcm_fdb_deinit (void)
{
  HSL_FN_ENTER ();

  HSL_FN_EXIT (0);
}

/*
  Add multicast FDB entry.
*/
static int
_hsl_bcm_add_l2mc (struct hsl_bridge *b, struct hsl_if *ifp, u_char *mac, int len, hsl_vid_t vid,
                   u_char flags, int is_forward)
{
  struct hsl_bcm_if *bcmifp, *bcmifpc;
  bcmx_mcast_addr_t mcaddr;
  struct hsl_if_list *nm;
  struct hsl_if *tmpif;
  bcmx_lport_t lport;
  int ret;

  HSL_FN_ENTER ();
#ifdef HAVE_L3
  /* Add static entries ONLY. */
  if (!(flags & HAL_L2_FDB_STATIC))
    HSL_FN_EXIT(STATUS_OK);
#endif /* HAVE_L3 */

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  /* Check for trunk port. */
  if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK) 
    {
      /* Get lport of first member. If the trunk is stable, then all known
         multicasts will go through the first port. TODO traffic distribution 
         for known multicasts can be added later. */

      /* If no children, then something is wrong! */
      if (! ifp->children_list)
        HSL_FN_EXIT (HSL_IFMGR_ERR_NULL_IF);

      nm = ifp->children_list;
      if (! nm || ! nm->ifp)
        HSL_FN_EXIT (HSL_IFMGR_ERR_NULL_IF);
      
      tmpif = nm->ifp;
      if (tmpif->type != HSL_IF_TYPE_L2_ETHERNET || ! tmpif->system_info)
        HSL_FN_EXIT (HSL_IFMGR_ERR_NULL_IF);

      bcmifpc = tmpif->system_info;

      /* Get lport of port. */
      lport = bcmifpc->u.l2.lport;
    }
  else
    {
      /* Get lport of port. */
      lport = bcmifp->u.l2.lport;  
    }

  memset(&mcaddr, 0, sizeof(bcmx_mcast_addr_t));
  bcmx_mcast_addr_t_init (&mcaddr, mac, vid);

  /* Get port. */
  ret = bcmx_mcast_port_get (mac, vid, &mcaddr);
  if (ret == BCM_E_NOT_FOUND)
    {
      /* Add the multicast entry. */
      ret = bcmx_mcast_addr_add (&mcaddr);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't add multicast FDB entry : Port %s mac (%02x%02x.%02x%02x.%02x%02x) VID %d\n", ifp->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid);
          bcmx_mcast_addr_free (&mcaddr);
          HSL_FN_EXIT (-1);
        }
    } /* end first addr not found */

  /* Join. */
  ret = bcmx_mcast_join (mac, vid, lport, &mcaddr, NULL);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't add multicast FDB entry : Port %s mac (%02x%02x.%02x%02x.%02x%02x) VID %d\n", ifp->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid);
      bcmx_mcast_addr_free (&mcaddr);
      HSL_FN_EXIT (-1);
    }

  bcmx_mcast_addr_t_free (&mcaddr);

  HSL_FN_EXIT (0);
}

/*
  Add unicast FDB entry.
*/
static int
_hsl_bcm_add_l2uc (struct hsl_bridge *b, struct hsl_if *ifp, u_char *mac, int len, hsl_vid_t vid,
                   u_char flags, int is_forward)
{
  struct hsl_bcm_if *bcmifp;
#ifdef HAVE_MAC_AUTH
  struct hsl_bridge_port *port;
#endif
  bcmx_l2_addr_t l2addr;
  bcmx_lport_t lport;
  int ret;

  HSL_FN_ENTER ();

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  lport = bcmifp->u.l2.lport;  

  bcmx_l2_addr_init (&l2addr, mac, vid);

  if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK) 
    {
      l2addr.tgid = bcmifp->trunk_id;

      l2addr.flags |= BCM_L2_TRUNK_MEMBER;
    }

  if (! is_forward)
      l2addr.flags |= BCM_L2_DISCARD_DST;
#ifdef HAVE_MAC_AUTH
   port = ifp->u.l2_ethernet.port;
   if (port->auth_mac_port_ctrl & AUTH_MAC_ENABLE)
     if (! is_forward)
       l2addr.flags |= BCM_L2_DISCARD_SRC;
#endif
  if (flags & HAL_L2_FDB_STATIC)
    l2addr.flags |= BCM_L2_STATIC;
  
  l2addr.lport = lport;

  ret = bcmx_l2_addr_add (&l2addr, NULL);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Can't add FDB entry : Port %s mac (%02x%02x.%02x%02x.%02x%02x) VID %d\n", ifp->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid);
      HSL_FN_EXIT (-1);
    }

  HSL_FN_EXIT (0);
}

/*
  Add FDB.
*/
int
hsl_bcm_add_fdb (struct hsl_bridge *b, struct hsl_if *ifp, u_char *mac, int len, hsl_vid_t vid,
                 u_char flags, int is_forward)
{
  int ret;

  HSL_FN_ENTER ();
  if (ENET_MACADDR_MULTICAST (mac))
    ret = _hsl_bcm_add_l2mc (b, ifp, mac, len, vid, flags, is_forward);
  else
    ret = _hsl_bcm_add_l2uc (b, ifp, mac, len, vid, flags, is_forward);

  HSL_FN_EXIT (ret);
}

/*
  Delete multicast FDB entry.
*/
static int
_hsl_bcm_delete_l2mc (struct hsl_bridge *b, struct hsl_if *ifp, u_char *mac, int len, hsl_vid_t vid, u_char flags)
{
  struct hsl_bcm_if *bcmifp, *bcmifpc;
  bcmx_mcast_addr_t mcaddr;
  struct hsl_if_list *nm;
  bcmx_lport_t lport;
  struct hsl_if *tmpif;
  int count = 0;
  int ret;

  HSL_FN_ENTER ();

#ifdef HAVE_L3
  /* Delete static entries ONLY. */
  if (!(flags & HAL_L2_FDB_STATIC))
    HSL_FN_EXIT(STATUS_OK);
#endif /* HAVE_L3 */

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  bcmifp = ifp->system_info;
  if (! bcmifp)
    {
      HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);
    }

  /* Check for trunk port. */
  if (bcmifp->type == HSL_BCM_IF_TYPE_TRUNK) 
    {
      /* Get lport of first member. If the trunk is stable, then all known
         multicasts will go through the first port. TODO traffic distribution 
         for known multicasts can be added later. */

      /* If no children, then something is wrong! */
      if (! ifp->children_list)
        HSL_FN_EXIT (HSL_IFMGR_ERR_NULL_IF);

      nm = ifp->children_list;
      if (! nm || ! nm->ifp)
        HSL_FN_EXIT (HSL_IFMGR_ERR_NULL_IF);
      
      tmpif = nm->ifp;
      if (tmpif->type != HSL_IF_TYPE_L2_ETHERNET || ! tmpif->system_info)
        HSL_FN_EXIT (HSL_IFMGR_ERR_NULL_IF);

      bcmifpc = tmpif->system_info;

      /* Get lport of port. */
      lport = bcmifpc->u.l2.lport;
    }
  else
    {
      /* Get lport of port. */
      lport = bcmifp->u.l2.lport;  
    }

  /* Leave. */
  ret = bcmx_mcast_leave (mac, vid, lport);
  switch (ret)
    {
    case BCM_MCAST_LEAVE_DELETED:
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
      "Multicast FDB entry : Port %s (%02x%02x.%02x%02x.%02x%02x) VID %d deleted\n", 
       ifp->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid);
      break;
    case BCM_MCAST_LEAVE_UPDATED:
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
      "Multicast FDB entry : Port %s deleted from (%02x%02x.%02x%02x.%02x%02x) VID %d\n", 
      ifp->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid);
      break;
    case BCM_MCAST_LEAVE_NOTFOUND:
    default:
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
      "Multicast FDB entry : Port %s (%02x%02x.%02x%02x.%02x%02x) VID %d not found\n", 
      ifp->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid);
      break;
    }

  /* Check if we need to delete the multicast MAC entirely. */
  bcmx_mcast_addr_t_init (&mcaddr, mac, vid);

  /* Get multicast MAC. */
  ret = bcmx_mcast_port_get (mac, vid, &mcaddr);
  if (ret == BCM_E_NOT_FOUND)
    {
      bcmx_mcast_addr_t_free (&mcaddr);

      HSL_FN_EXIT (0);
    }

  /* If no ports are remaining, delete MC mac */
  BCMX_LPLIST_ITER(mcaddr.ports, lport, count)
    {
      if (!(BCMX_LPORT_FLAGS(lport) & BCMX_PORT_F_HG))
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
          "multicast FDB entry : mac (%02x%02x.%02x%02x.%02x%02x) VID %d still installed on lport %d\n", 
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid, lport);

          bcmx_mcast_addr_t_free (&mcaddr);
          HSL_FN_EXIT (0); 
        }
    }

  /* Remove. */
  ret = bcmx_mcast_addr_remove (mac, vid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
      "Can't delete multicast FDB entry : mac (%02x%02x.%02x%02x.%02x%02x) VID %d\n", 
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vid);

      bcmx_mcast_addr_t_free (&mcaddr);
      HSL_FN_EXIT (-1);
    }

  bcmx_mcast_addr_t_free (&mcaddr);
  HSL_FN_EXIT (0);
}

/* 
   Delete unicast FDB entry. 
*/
static int
_hsl_bcm_delete_l2uc (struct hsl_bridge *b, u_char *mac, int len, hsl_vid_t vid)
{
  return bcmx_l2_addr_delete (mac, vid);
}

/*
  Delete FDB.
*/
int
hsl_bcm_delete_fdb (struct hsl_bridge *b, struct hsl_if *ifp, u_char *mac, int len, hsl_vid_t vid, u_char flags)
{
  int ret;

  HSL_FN_ENTER ();

  if (ENET_MACADDR_MULTICAST (mac))
    ret = _hsl_bcm_delete_l2mc (b, ifp, mac, len, vid, flags);
  else
    ret = _hsl_bcm_delete_l2uc (b, mac, len, vid);

  HSL_FN_EXIT (ret);
}

/*
  Flush FDB by port.
*/
int
hsl_bcm_flush_fdb (struct hsl_bridge *b, struct hsl_if *ifp, hsl_vid_t vid)
{
  struct hsl_bcm_if *bcmifp;
  bcmx_lport_t lport;
  int ret = 0;
  
  HSL_FN_ENTER ();

  /* If no port passed delete by vlan id. */
  if((!ifp) && (vid))
   {
     ret = bcmx_l2_addr_delete_by_vlan(vid, 0);
     HSL_FN_EXIT (ret);
   }
  else if(! ifp || ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);

  bcmifp = ifp->system_info;
  if (! bcmifp)
    HSL_FN_EXIT (HSL_ERR_BRIDGE_INVALID_PARAM);

  lport = bcmifp->u.l2.lport;

  /* If no vlan passed delete by logical port. */
  if ((!vid) && (ifp))
    ret = bcmx_l2_addr_delete_by_port (lport, 0);

  /* Delete by logical port & vlan id. */
  else if ((ifp) && (vid))
    ret = bcmx_l2_addr_delete_by_vlan_port(vid, lport, 0);
  else 
    HSL_FN_EXIT (-1);  

  HSL_FN_EXIT (ret);
}

/*
   Get unicast FDB
*/
int
hsl_bcm_unicast_get_fdb (struct hal_msg_l2_fdb_entry_req *req,
                         struct hal_msg_l2_fdb_entry_resp *resp)
{
  struct hal_fdb_entry *entry;
  fdb_entry_t result;
  struct hsl_if *ifp;
  u_int32_t count;
  int ret;

  HSL_FN_ENTER ();

  entry = &req->start_fdb_entry;
  memcpy (&result.mac_addr[0], &entry->mac_addr[0], ETH_ADDR_LEN);
  result.vid = entry->vid;
  count = 0;
  ret = 0;

  entry = &resp->fdb_entry[0];
  /* Get entry */
  while (count < req->count &&
         STATUS_OK == hsl_getnext_fdb_entry (&result, SEARCH_BY_VLAN_MAC, &result))
    {
      if (result.is_static == 1)
         continue; 

      /* Get interface structure. */
      ifp = hsl_ifmgr_lookup_by_index (result.port_no);
      if(!ifp)
        {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Unknown ifindex %d in l2 fdb database\n",result.port_no);
          continue;
        }

      /* We don't need to show mac addresses learned on L3 interfaces. */ 
      if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
        {
           HSL_IFMGR_IF_REF_DEC (ifp);
           continue;
        }

      HSL_IFMGR_IF_REF_DEC (ifp);

      /* Interface index. */
      entry->port = result.port_no;

      /* Mac address. */
      memcpy (entry->mac_addr, result.mac_addr, ETH_ADDR_LEN);


      /* vid */
      entry->vid = result.vid;

      /* is_fwd */
      entry->is_forward = result.is_fwd;

      /* ageing_timer_value */
      entry->ageing_timer_value = result.ageing_timer_value;

      entry += 1;
      count += 1;
    }

  resp->count = count;

  HSL_FN_EXIT (ret);
}

/* 
   Flush FDB by mac.
*/
int
hsl_bcm_flush_fdb_by_mac (struct hsl_bridge *b, u_char *mac, int len, int flags)
{
  int ret;
  HSL_FN_ENTER ();

  if (flags == HAL_L2_DELETE_STATIC)
    ret = bcmx_l2_addr_delete_by_mac(mac, BCM_L2_DELETE_STATIC);
  else
    ret = bcmx_l2_addr_delete_by_mac(mac, 0);

  HSL_FN_EXIT (ret);
}

#ifdef HAVE_ONMD

/*
   Set Port state for EFM remote loopback
*/
int
hsl_bcm_efm_set_port_state(unsigned int index,
                        enum hal_efm_par_action local_par_action,
                        enum hal_efm_mux_action local_mux_action)

{
  bcmx_lport_t lport;
  struct hsl_if *ifp = NULL;
  struct hsl_bcm_if *bcmifp = NULL;
  
  ifp = hsl_ifmgr_lookup_by_index (index);
  if (! ifp)
    return -1;
 
  bcmifp = ifp->system_info;
  lport = bcmifp->u.l2.lport;

  if(local_par_action == HAL_PAR_ACTION_LB)
  {
    /*enable oam loopback*/
    bcmx_port_control_set(lport, bcmPortControlOamLoopback, 1);
  }
  else
  {
    /*disable oam loopback*/
    bcmx_port_control_set(lport, bcmPortControlOamLoopback, 0);
  }

  HSL_FN_EXIT (0); 
}

#endif 

bcm_filterid_t pbit_fid;

int
hsl_bcm_pbit_copying_init()
{
  int ret;
  
  ret = bcmx_filter_create (&pbit_fid);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
               "Can't create IGMP Snooping filter\n");
      HSL_FN_EXIT (-1);
    }
  
  bcmx_filter_qualify_format (pbit_fid, 0x0e);//qcl 20170808 BCM_FILTER_PKTFMT_INNER_TAG
  
  /* Define processing rules */
  bcmx_filter_action_match (pbit_fid,0x04 , 2);//qcl 20170808 bcmActionInsPrio
  ret = bcmx_filter_install (pbit_fid);
  
  HSL_FN_EXIT (ret);
}


/*
  Initialize callbacks and register.
*/
int
hsl_bridge_hw_register (void)
{
  HSL_FN_ENTER ();

  hsl_bcm_l2_cb.bridge_init = hsl_bcm_bridge_init;
  hsl_bcm_l2_cb.bridge_deinit = hsl_bcm_bridge_deinit;
  hsl_bcm_l2_cb.set_age_timer = hsl_bcm_set_age_timer;
  hsl_bcm_l2_cb.set_learning = hsl_bcm_set_learning;
  hsl_bcm_l2_cb.set_stp_port_state = hsl_bcm_set_stp_port_state;
  hsl_bcm_l2_cb.add_instance = hsl_bcm_add_instance;
  hsl_bcm_l2_cb.delete_instance = hsl_bcm_delete_instance;
  hsl_bcm_l2_cb.add_port_to_bridge = hsl_bcm_add_port_to_bridge;
  hsl_bcm_l2_cb.delete_port_from_bridge = hsl_bcm_delete_port_from_bridge;
#ifdef HAVE_VLAN
  hsl_bcm_l2_cb.add_vlan_to_instance = hsl_bcm_add_vlan_to_instance;
  hsl_bcm_l2_cb.delete_vlan_from_instance = hsl_bcm_delete_vlan_from_instance;
  hsl_bcm_l2_cb.add_vlan = hsl_bcm_add_vlan;
  hsl_bcm_l2_cb.delete_vlan = hsl_bcm_delete_vlan;
  hsl_bcm_l2_cb.set_vlan_port_type = hsl_bcm_set_vlan_port_type;
  hsl_bcm_l2_cb.set_default_pvid = hsl_bcm_set_default_pvid;
  hsl_bcm_l2_cb.add_vlan_to_port = hsl_bcm_add_vlan_to_port;
  hsl_bcm_l2_cb.delete_vlan_from_port = hsl_bcm_delete_vlan_from_port;
#ifdef HAVE_VLAN_CLASS
  hsl_bcm_l2_cb.vlan_mac_classifier_add     = hsl_bcm_vlan_mac_classifier_add;
  hsl_bcm_l2_cb.vlan_ipv4_classifier_add    = hsl_bcm_vlan_ipv4_classifier_add;
  hsl_bcm_l2_cb.vlan_mac_classifier_delete  = hsl_bcm_vlan_mac_classifier_delete;
  hsl_bcm_l2_cb.vlan_ipv4_classifier_delete = hsl_bcm_vlan_ipv4_classifier_delete;
  hsl_bcm_l2_cb.vlan_proto_classifier_add   = hsl_bcm_vlan_proto_classifier_add;
  hsl_bcm_l2_cb.vlan_proto_classifier_delete= hsl_bcm_vlan_proto_classifier_delete;
#endif /* HAVE_VLAN_CLASS */
#ifdef HAVE_PVLAN
  hsl_bcm_l2_cb.set_pvlan_vlan_type = hsl_bcm_pvlan_set_vlan_type;
  hsl_bcm_l2_cb.add_pvlan_vlan_association = hsl_bcm_pvlan_add_vlan_association;
  hsl_bcm_l2_cb.del_pvlan_vlan_association = hsl_bcm_pvlan_del_vlan_association;
  hsl_bcm_l2_cb.add_pvlan_port_association = hsl_bcm_pvlan_add_port_association;
  hsl_bcm_l2_cb.del_pvlan_port_association = hsl_bcm_pvlan_del_port_association;
  hsl_bcm_l2_cb.set_pvlan_port_mode = hsl_bcm_pvlan_set_port_mode;
#endif /* HAVE_PVLAN */
#endif /* HAVE_VLAN */

  /* Register L2 fdb manager. */
  hsl_fdb_hw_cb_register ();

#ifdef HAVE_IGMP_SNOOP
  hsl_bcm_l2_cb.enable_igmp_snooping = hsl_bcm_enable_igmp_snooping;
  hsl_bcm_l2_cb.disable_igmp_snooping = hsl_bcm_disable_igmp_snooping;
  hsl_bcm_l2_cb.enable_igmp_snooping_port = hsl_bcm_enable_igmp_snooping_port;
  hsl_bcm_l2_cb.disable_igmp_snooping_port = hsl_bcm_disable_igmp_snooping_port;
#endif /* HAVE_IGMP_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
hsl_bcm_l2_cb.l2_unknown_mcast_mode = hsl_bcm_l2_unknown_mcast_mode;
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_MLD_SNOOP
  hsl_bcm_l2_cb.enable_mld_snooping = hsl_bcm_enable_mld_snooping;
  hsl_bcm_l2_cb.disable_mld_snooping = hsl_bcm_disable_mld_snooping;
#endif /* HAVE_MLD_SNOOP */

  hsl_bcm_l2_cb.ratelimit_bcast = hsl_bcm_ratelimit_bcast;
  hsl_bcm_l2_cb.ratelimit_mcast = hsl_bcm_ratelimit_mcast;
  hsl_bcm_l2_cb.ratelimit_dlf_bcast = hsl_bcm_ratelimit_dlf_bcast;
  hsl_bcm_l2_cb.ratelimit_bcast_discards_get = hsl_bcm_ratelimit_get_bcast_discards;
  hsl_bcm_l2_cb.ratelimit_mcast_discards_get = hsl_bcm_ratelimit_get_mcast_discards;
  hsl_bcm_l2_cb.ratelimit_dlf_bcast_discards_get = hsl_bcm_ratelimit_get_dlf_bcast_discards;
  hsl_bcm_l2_cb.set_flowcontrol = hsl_bcm_set_flowcontrol;
  hsl_bcm_l2_cb.get_flowcontrol_statistics = hsl_bcm_flowcontrol_statistics;
  hsl_bcm_l2_cb.add_fdb = hsl_bcm_add_fdb;
  hsl_bcm_l2_cb.delete_fdb = hsl_bcm_delete_fdb;
  hsl_bcm_l2_cb.get_uni_fdb = hsl_bcm_unicast_get_fdb;
  hsl_bcm_l2_cb.flush_port_fdb = hsl_bcm_flush_fdb;
  hsl_bcm_l2_cb.flush_fdb_by_mac = hsl_bcm_flush_fdb_by_mac;

  /* 
  FOR UT
  hsl_bcm_l2_cb.frame_period_window_set  = hsl_bcm_frame_period_window_set;
  hsl_bcm_l2_cb.symbol_period_window_set = hsl_bcm_symbol_period_window_set;
  hsl_bcm_l2_cb.frame_error_get          = hsl_bcm_frame_error_get;
  hsl_bcm_l2_cb.frame_error_seconds_get  = hsl_bcm_frame_error_seconds_get;
  hsl_bcm_l2_cb.efm_set_port_state       = hsl_bcm_efm_set_port_state; */

  /* Register with bridge manager. */
  hsl_bridge_hw_cb_register (&hsl_bcm_l2_cb);

  HSL_FN_EXIT (0);
}

