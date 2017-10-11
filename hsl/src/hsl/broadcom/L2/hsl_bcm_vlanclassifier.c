/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#include "bcm_incl.h"

#ifdef HAVE_VLAN_CLASS

#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#include "hsl_oss.h"
#include "hsl_ifmgr.h"
#include "hsl_logger.h"

#include "hsl_avl.h"
#include "hsl_bcm_vlanclassifier.h"

/* Initialise proto_rule_tree to NULL */
struct hsl_avl_tree *proto_rule_tree = NULL;

/* Protocol Vlan classifier compare */
int
_hsl_bcm_vlan_proto_classifier_cmp (void *param1, void *param2)
{
  struct hsl_bcm_proto_vlan_class_entry *p1 = (struct hsl_bcm_proto_vlan_class_entry *) param1;
  struct hsl_bcm_proto_vlan_class_entry *p2 = (struct hsl_bcm_proto_vlan_class_entry *) param2;
  
  if ((p1->vlan_id == p2->vlan_id) && (p1->ether_type == p2->ether_type) &&
      (p1->encaps == p2->encaps))
    return 0;

  return 1;
}

/* Protocol vlan classifier port compare */
int
_hsl_bcm_vlan_classifier_port_cmp (void *param1, void *param2)
{
  /* This port is of type hsl_if. Just compare the ponters as integers */
  unsigned int p1 = (unsigned int) param1;
  unsigned int p2 = (unsigned int) param2;
  
  /* Less than */
  if (p1 < p2)
    return -1;
  
  if (p1 > p2)
    return 1;

  /* Equals to */
  return 0;
}

bcm_port_frametype_t
_hsl_bcm_frame_type_encode (u_int32_t encaps)
{
  switch (encaps)
    {
      case HAL_VLAN_CLASSIFIER_ETH:
        return BCM_PORT_FRAMETYPE_ETHER2;
       break;
      case HAL_VLAN_CLASSIFIER_SNAP_LLC:
        return BCM_PORT_FRAMETYPE_SNAP;
       break;
      case HAL_VLAN_CLASSIFIER_NOSNAP_LLC:
        return BCM_PORT_FRAMETYPE_LLC;
       break;
      default:
       break;
    }
  
  /* Default return ETh2 */
  return BCM_PORT_FRAMETYPE_ETHER2;
}

int
hsl_bcm_vlan_proto_classifier_add (u_int16_t vlan_id, u_int16_t ether_type,
                                   u_int32_t encaps, u_int32_t ifindex,
                                   u_int32_t refcount)
{
  struct hsl_bcm_proto_vlan_class_entry *p_vlan_class_entry;
  struct hsl_bcm_proto_vlan_class_entry vlan_class_entry;
  struct hsl_avl_node *node;
  struct hsl_if *ifp;
  bcmx_lport_t lport;
  bcm_port_frametype_t frame;
  bcm_port_ethertype_t ether;
  int ret;
   
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (!ifp)
    HSL_FN_EXIT(-1); 
    
  if (!proto_rule_tree)
    {
      /* first protocol based rule */
      ret = hsl_avl_create(&proto_rule_tree, 0, 
                           _hsl_bcm_vlan_proto_classifier_cmp);
      if (ret < 0)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_FN_EXIT(-1);
        }
    }
   
  vlan_class_entry.vlan_id = vlan_id;
  vlan_class_entry.ether_type = ether_type;
  vlan_class_entry.encaps = encaps;
  node = hsl_avl_lookup(proto_rule_tree, &vlan_class_entry);
  if (node)
    {
      p_vlan_class_entry = (struct hsl_bcm_proto_vlan_class_entry *)HSL_AVL_NODE_INFO(node);
      if (!p_vlan_class_entry->port_tree)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_FN_EXIT(-1);
        }
      
      /* Just add the port to the vlan classifier port tree */      
      hsl_avl_insert (p_vlan_class_entry->port_tree, ifp);
    }
  else
    {
      p_vlan_class_entry = oss_malloc (sizeof (struct hsl_bcm_proto_vlan_class_entry), OSS_MEM_HEAP);
      if (! p_vlan_class_entry)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_FN_EXIT(-1);
        }
     
      p_vlan_class_entry->vlan_id = vlan_id;
      p_vlan_class_entry->ether_type = ether_type;
      p_vlan_class_entry->encaps = encaps;
      
      ret = hsl_avl_create (&p_vlan_class_entry->port_tree, 0,
                            _hsl_bcm_vlan_classifier_port_cmp);
      if (ret < 0)
        {
          oss_free (p_vlan_class_entry, OSS_MEM_HEAP);
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_FN_EXIT(-1); 
        }
      hsl_avl_insert (p_vlan_class_entry->port_tree, ifp);
      hsl_avl_insert (proto_rule_tree, p_vlan_class_entry);
      
      frame = _hsl_bcm_frame_type_encode (encaps);
      ether = ether_type;
      
      /* Program hardware. For consistency among all vlan classifier
         we program all ports since there is no single command to 
         have global rule */
      BCMX_FOREACH_QUALIFIED_LPORT(lport, 
                                   BCMX_PORT_F_FE|BCMX_PORT_F_GE|
                                   BCMX_PORT_F_XE)
        {
          bcmx_port_protocol_vlan_add (lport, frame, ether, vlan_id); 
        }
    }
  
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT(0);
}

int 
hsl_bcm_vlan_mac_classifier_add (u_int16_t vlan_id, u_char *mac, u_int32_t ifindex, u_int32_t refcount)
{
  int ret;

  ret = bcmx_vlan_mac_add (mac, vlan_id, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Failed to add vlan classifier, bcm error = %d\n", ret);

      return ret;
    }

  return 0;
}

int hsl_bcm_vlan_ipv4_classifier_add (u_int16_t vlan_id, u_int32_t addr,
                                      u_int32_t masklen, u_int32_t ifindex,
                                      u_int32_t refcount)
{
  u_int32_t ipMask;
  int ret;

  ipMask = (0xffffffff  << (32 - masklen));
  ret = bcmx_vlan_ip4_add (addr, ipMask, vlan_id, 0);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to add vlan classifier, bcm error = %d\n", ret);

      return ret;
    }

  return 0;
}

int 
hsl_bcm_vlan_proto_classifier_delete (u_int16_t vlan_id, u_int16_t ether_type,
                                      u_int32_t encaps, u_int32_t ifindex,
                                      u_int32_t refcount)
{
  struct hsl_bcm_proto_vlan_class_entry *p_vlan_class_entry;
  struct hsl_bcm_proto_vlan_class_entry vlan_class_entry;
  struct hsl_avl_node *port_node;
  struct hsl_avl_node *node;
  struct hsl_if *ifp;
  bcmx_lport_t lport;
  bcm_port_frametype_t frame;
  bcm_port_ethertype_t ether;
   
  if (!proto_rule_tree)
    {
      /* Nothing to delete */
      HSL_FN_EXIT(-1); 
    }
  
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (!ifp)
    HSL_FN_EXIT(-1); 
    
  vlan_class_entry.vlan_id = vlan_id;
  vlan_class_entry.ether_type = ether_type;
  vlan_class_entry.encaps = encaps;
  node = hsl_avl_lookup(proto_rule_tree, &vlan_class_entry);
  if (node)
    {
      p_vlan_class_entry = (struct hsl_bcm_proto_vlan_class_entry *)HSL_AVL_NODE_INFO(node);
      if (!p_vlan_class_entry->port_tree)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_FN_EXIT(0);
        }
      
      port_node = hsl_avl_lookup(p_vlan_class_entry->port_tree, ifp);
      if (!port_node)
        {
          HSL_IFMGR_IF_REF_DEC (ifp);
          HSL_FN_EXIT(0);
        }
      
      hsl_avl_delete (p_vlan_class_entry->port_tree, ifp);
      if (hsl_avl_get_tree_size(p_vlan_class_entry->port_tree) == 0)
        {
          /* Delete the vlan class entry entirely */
          hsl_avl_tree_free (&p_vlan_class_entry->port_tree, NULL);

          hsl_avl_delete (proto_rule_tree, p_vlan_class_entry);
         
          oss_free (p_vlan_class_entry, OSS_MEM_HEAP); 
       
          frame = _hsl_bcm_frame_type_encode (encaps);
          ether = ether_type;
          
          /* Program hardware. For consistency among all vlan classifier
             we program all ports since there is no single command to 
             have global rule */
          BCMX_FOREACH_QUALIFIED_LPORT(lport, 
                                       BCMX_PORT_F_FE|BCMX_PORT_F_GE|
                                       BCMX_PORT_F_XE)
           {
             bcmx_port_protocol_vlan_delete (lport, frame, ether); 
           }
        } /* end is tree size == 0 */ 
    } /* end if node */ 
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT(0);
}

int 
hsl_bcm_vlan_mac_classifier_delete (u_int16_t vlan_id,u_char *mac,
                                    u_int32_t ifindex, u_int32_t refcount)
{
  int ret;

  /* 
     Broadcom doesn't support per interface classification -> 
     action required only if refcount is 0 
   */
  if(refcount) 
    return 0;

  ret = bcmx_vlan_mac_delete (mac);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to delete vlan classifier, bcm error = %d\n", ret);

      return ret;
    }

  return 0;
}

int hsl_bcm_vlan_ipv4_classifier_delete (u_int16_t vlan_id, u_int32_t addr,
                                         u_int32_t masklen, u_int32_t ifindex,
                                         u_int32_t refcount)
{
  u_int32_t ipMask;
  int ret;

  /* 
     Broadcom doesn't support per interface classification -> 
     action required only if refcount is 0 
   */
  if(refcount) 
    return 0;

  ipMask = (0xffffffff  << (32 - masklen));
  ret = bcmx_vlan_ip4_delete (addr, ipMask);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to delete vlan classifier, bcm error = %d\n", ret);

      return ret;
    }

  return 0;
}
#endif /* HAVE_VLAN_CLASS */

