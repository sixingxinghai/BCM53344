#include "config.h"

#include "hsl_os.h"

#include "hsl_types.h"

#include "hal_types.h"
#ifdef HAVE_L2
#include "hal_l2.h"
#endif /* HAVE_L2 */

#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl_error.h"
#include "bcm_incl.h"
#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#ifdef HAVE_L2
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#endif /* HAVE_L2 */
#include "hsl_if_hw.h"
#include "hsl_bcm.h"
#include "hsl_bcm_if.h"
#ifdef HAVE_L2LERN

struct hsl_mac_access_grp *
hsl_bcm_mac_access_grp_add (struct hal_msg_mac_set_access_grp *msg)
{
  struct hsl_mac_access_grp *hsl_macc_grp;
  struct hal_mac_access_grp *hal_macc_grp = &msg->hal_macc_grp;

  hsl_macc_grp = oss_malloc (sizeof (struct hsl_mac_access_grp), OSS_MEM_HEAP);


  hsl_macc_grp->deny_permit = hal_macc_grp->deny_permit;
  hsl_macc_grp->l2_type = hal_macc_grp->l2_type;
  strcpy(hsl_macc_grp->name,hal_macc_grp->name);
  memcpy (&hsl_macc_grp->a, &hal_macc_grp->a, sizeof (struct hsl_acl_mac_addr));
  memcpy (&hsl_macc_grp->a_mask, &hal_macc_grp->a_mask, sizeof (struct hsl_acl_mac_addr));
  memcpy (&hsl_macc_grp->m, &hal_macc_grp->m, sizeof (struct hsl_acl_mac_addr));
  memcpy (&hsl_macc_grp->m_mask, &hal_macc_grp->m_mask, sizeof (struct hsl_acl_mac_addr));

  return hsl_macc_grp;

}

int
hsl_bcm_mac_access_grp_del (struct hsl_mac_access_grp *hsl_macc_grp)
{
  oss_free (hsl_macc_grp, OSS_MEM_HEAP);
  return 0;
}

int 
hsl_bcm_mac_set_ingress_access_grp(struct hal_msg_mac_set_access_grp *msg)
{
  struct hsl_if *ifp = NULL;
  struct hsl_bridge_port *port;
  int ifindex = msg->ifindex;

  HSL_FN_ENTER ();

 /* Get the ifp */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (!ifp)
     HSL_FN_EXIT (-1);

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      port = ifp->u.l2_ethernet.port;
      if ( msg->action == HSL_MAC_ACL_GRP_ADD)
        {
          if (port->hsl_macc_grp)
            {
              return -1;
            }
          else
            port->hsl_macc_grp = hsl_bcm_mac_access_grp_add (msg);
        }
      else
       {
         /*Add the vlan access map delete code*/
         if (! port->hsl_macc_grp)
            {
              return -1;
            }
          else
            {
              hsl_bcm_mac_access_grp_del (port->hsl_macc_grp);
              port->hsl_macc_grp = NULL;
            }
       }
    }
  else
    HSL_FN_EXIT (-1);

  HSL_FN_EXIT (0);
} 

int
hsl_bcm_mac_set_access_grp (struct hal_msg_mac_set_access_grp *msg)
{
  struct hsl_if *ifp = NULL;
  int ifindex = msg->ifindex;
  int direction = msg->dir;
  int ret;

  HSL_FN_ENTER ();

  /* Get the ifp */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (!ifp)
    HSL_FN_EXIT (-1);

  if (direction == HAL_QOS_DIRECTION_INGRESS)
    {
      ret = hsl_bcm_mac_set_ingress_access_grp (msg);
      if (ret != 0)
        HSL_FN_EXIT (ret);
      HSL_FN_EXIT (0);
    }
  else
    HSL_FN_EXIT (-1);

  HSL_FN_EXIT (0);
}

#endif /* HAVE_L2LERN */

static int
hsl_bcm_accgrp_dst_ip_qualifier_set (bcm_filterid_t filter,
                                     bcm_ip_t dst_prefix,
                                     bcm_ip_t dst_ip_mask)
{
  return bcmx_filter_qualify_data32 (filter, HSL_ACCGRP_DST_IP_OFFSET,
                                     dst_prefix, dst_ip_mask);
}

static int
hsl_bcm_accgrp_src_ip_qualifier_set (bcm_filterid_t filter,
                                     bcm_ip_t src_prefix,
                                     bcm_ip_t src_ip_mask)
{
  return bcmx_filter_qualify_data32 (filter, HSL_ACCGRP_SRC_IP_OFFSET,
                                     src_prefix, src_ip_mask);
}

static int
hsl_bcm_accgrp_vlan_qualifier_set (bcm_filterid_t filter,
                                   u_int16_t vlan_id)
{
  return bcmx_filter_qualify_data16 (filter, HSL_ACCGRP_VLAN_OFFSET, vlan_id,
                                     HSL_ACCGRP_VLAN_MASK);
}

static int 
hsl_bcm_set_in_profile_action_accgrp (struct hsl_bcm_accgrp_filter *qf)
{
  int ret = 0;

  if (qf->type == HSL_BCM_FEATURE_FILTER)
    {
       ret = bcmx_filter_action_match (qf->u.filter,
                                       0xf,
                                       0);//qiucl 20170808 bcmActionDoSwitch
    }
  else if (qf->type == HSL_BCM_FEATURE_FIELD)
    {

      ret = bcmx_field_action_add (qf->u.field.entry, bcmFieldActionDscpNew,
                                   0, 0);

    }

  return ret;
}

static void
hsl_bcm_accgrp_filter_delete (struct hsl_bcm_accgrp_filter *qf)
{
  if (qf->type == HSL_BCM_FEATURE_FILTER)
    {
      bcmx_filter_remove (qf->u.filter);
      bcmx_filter_destroy (qf->u.filter);
    }
  else if (qf->type == HSL_BCM_FEATURE_FIELD)
    {
      if (qf->u.field.meter > 0)
        {
          bcmx_field_meter_destroy (qf->u.field.entry);
          qf->u.field.meter = 0;
        }

      bcmx_field_entry_remove (qf->u.field.entry);
      bcmx_field_entry_destroy (qf->u.field.entry);
      bcmx_field_group_destroy (qf->u.field.group);
    }
}

static void
hsl_bcm_accgrp_if_filter_delete_all (struct hsl_bcm_if *bcmif)
{
  int i;

  for (i = 0; i < bcmif->u.l2.ip_filter_num; i++)
    {
      hsl_bcm_accgrp_filter_delete (&bcmif->u.l2.ip_filters[i]);
      memset (&bcmif->u.l2.ip_filters[i], 0,
              sizeof (struct hsl_bcm_accgrp_filter));
    }

  bcmif->u.l2.ip_filter_num = 0;
}

static void
hsl_bcm_accgrp_if_meter_delete_all (struct hsl_bcm_if *bcmif)
{
  int i;

  for (i = 0; i < bcmif->u.l2.ip_meter_num; i++)
    {
      bcmx_meter_delete (bcmif->u.l2.lport,
                         bcmif->u.l2.ip_meters[i]);
      bcmif->u.l2.ip_meters[i] = 0;

    }

  bcmif->u.l2.ip_meter_num = 0;
}

static int
hsl_bcm_accgrp_if_filter_add (struct hsl_bcm_if *bcmif,
                              struct hsl_bcm_accgrp_filter *qf)
{
  if (bcmif->u.l2.ip_filter_num == HSL_ACCGRP_IF_FILTER_MAX)
    return -1;

  bcmif->u.l2.ip_filters[bcmif->u.l2.ip_filter_num] = *qf;
  bcmif->u.l2.ip_filter_num++;

  return 0;
}

static int
hsl_bcm_ip_access_group_create (struct hsl_bcm_accgrp_filter *qf,
                                unsigned long filter_flags,
                                bcm_ip_t src_prefix,
                                bcm_ip_t src_ip_mask,
                                bcm_ip_t dst_prefix,
                                bcm_ip_t dst_ip_mask,
                                u_int16_t vid)
{
  int ret;

  if (qf->type == HSL_BCM_FEATURE_FILTER)
    {
      bcm_filterid_t filter;
      int filter_created = 0;

      ret = bcmx_filter_create (&filter);
      if (ret != BCM_E_NONE)
      {
        return ret;
      }


      filter_created = 1;

      if (filter_flags & HSL_ACCGRP_FILTER_DST_IP)
        {
          ret = hsl_bcm_accgrp_dst_ip_qualifier_set (filter, dst_prefix,
                                                     dst_ip_mask);
          if (ret < 0)
            goto err_ret_filter;
        }
      if (filter_flags & HSL_ACCGRP_FILTER_SRC_IP)
        {
          ret = hsl_bcm_accgrp_src_ip_qualifier_set (filter,
                                                     src_prefix, src_ip_mask);
          if (ret < 0)
            goto err_ret_filter;
        }

      if (vid)
        {
          ret = hsl_bcm_accgrp_vlan_qualifier_set (filter,
                                                   vid);

          if (ret < 0)
            {
              HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                       " hsl_bcm_accgrp_vlan_qualifier_set %u %d\n", vid, ret);
              goto err_ret_filter;
            }
        }

      qf->u.filter = filter;

      return 0;

    err_ret_filter:
      if (filter_created)
        {
          bcmx_filter_remove (filter);
          bcmx_filter_destroy (filter);
        }

      return -1;
    }

  if (qf->type == HSL_BCM_FEATURE_FIELD)
    {
      bcm_field_qset_t qset;
      bcm_field_group_t group;
      bcm_field_entry_t entry;
      int entry_created = 0;
      int group_created = 0;

      BCM_FIELD_QSET_INIT (qset);

      ret = bcmx_field_group_create (qset, BCM_FIELD_GROUP_PRIO_ANY,
                                     &group);
      if (ret != BCM_E_NONE)
        return ret;

      group_created = 1;

      BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyInPorts);

      if (filter_flags & HSL_ACCGRP_FILTER_DST_IP)
        BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyDstIp);

      if (filter_flags & HSL_ACCGRP_FILTER_SRC_IP)
        BCM_FIELD_QSET_ADD (qset, bcmFieldQualifySrcIp);

      if (vid)
        BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyOuterVlan);

      ret = bcmx_field_group_set (group, qset);
      if (ret < 0)
        goto err_ret_field;

      ret = bcmx_field_entry_create (group, &entry);
      if (ret < 0)
        goto err_ret_field;

      entry_created = 1;

      if (filter_flags & HSL_ACCGRP_FILTER_DST_IP)
        {
          ret = bcmx_field_qualify_DstIp (entry, dst_prefix,
                                          dst_ip_mask);
          if (ret < 0)
            goto err_ret_field;
        }

      if (filter_flags & HSL_ACCGRP_FILTER_SRC_IP)
        {
          ret = bcmx_field_qualify_SrcIp (entry,
                                          src_prefix, src_ip_mask);
          if (ret < 0)
            goto err_ret_field;
        }

      if (vid)
        {
          ret = bcmx_field_qualify_OuterVlan (entry,
                                              vid, HSL_ACCGRP_VLAN_MASK);
          if (ret < 0)
            {
              HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                       " bcmx_field_qualify_OuterVlan %u %d %s\n", vid, ret,
                       bcm_errmsg (ret));
              goto err_ret_field;
            }
        }

    qf->u.field.entry = entry;
    qf->u.field.group = group;

    return 0;

    err_ret_field:
      if (entry_created)
        {
          bcmx_field_entry_remove (entry);
          bcmx_field_entry_destroy (entry);
        }

      if (group_created)
        {
          bcmx_field_group_destroy (group);
        }

      return -1;
    }

  return -1;
}

static int
hsl_bcm_access_group_apply (struct hsl_bcm_accgrp_filter *qf,
                            int action,
                            struct hsl_bcm_if *bcmif,
                            int direction)
{
  int ret;
  bcmx_lplist_t plist;

  plist.lp_ports = NULL;

  ret = bcmx_lplist_init (&plist, 0, 0);
  if (ret != BCM_E_NONE)
    return ret;
  
  ret = bcmx_lplist_add (&plist, bcmif->u.l2.lport);

  if (ret != BCM_E_NONE)
    goto err_ret;

  if (qf->type == HSL_BCM_FEATURE_FILTER)
    {
      if (direction == HAL_IP_ACL_DIRECTION_INGRESS)
        ret = bcmx_filter_qualify_ingress (qf->u.filter, plist);
      else
        ret = bcmx_filter_qualify_egress (qf->u.filter, plist);

      if (ret != BCM_E_NONE)
         goto err_ret;

      bcmx_lplist_free (&plist);

      if (action == HAL_ACCGRP_FILTER_DENY)
        {
          ret = bcmx_filter_action_match (qf->u.filter,
                                          0xf, 0);//qcl 20170708 bcmActionDoNotSwitch
          if (ret < 0)
            goto err_ret;
        }
      else
        {
          /* Set in-profile */
          ret = hsl_bcm_set_in_profile_action_accgrp (qf);
          if (ret < 0)
            goto err_ret;
        }

      ret = bcmx_filter_install (qf->u.filter);

      return ret;
    }
  else if (qf->type == HSL_BCM_FEATURE_FIELD)
    {
      ret = bcmx_field_qualify_InPorts (qf->u.field.entry, plist);
      if (ret != BCM_E_NONE)
        goto err_ret;

      bcmx_lplist_free (&plist);

      if (action == HAL_ACCGRP_FILTER_DENY)
        {
          ret = bcmx_field_action_add (qf->u.field.entry, bcmFieldActionDrop,
                                       0, 0);
          if (ret < 0)
            goto err_ret;
        }
      else
        {
          /* Set in-profile */
          ret = hsl_bcm_set_in_profile_action_accgrp (qf);

          if (ret < 0)
            goto err_ret;
        }

      ret = bcmx_field_entry_install (qf->u.field.entry);

      return ret;
    }

 err_ret:
  if (plist.lp_ports)
    bcmx_lplist_free (&plist);

  return -1;
}

static int
hsl_bcm_set_access_group (struct hal_ip_access_grp *acl,
                          struct hsl_bcm_if *bcmif, int direction,
                          u_int16_t vid)
{
  int i, ret = 0;
  unsigned long filter_flags = 0;
  struct hsl_bcm_accgrp_filter qf;
  unsigned long src_prefix, src_ip_mask, dst_prefix, dst_ip_mask;

  for (i = 0; i < HAL_IP_MAX_ACL_FILTER && i < acl->ace_number; i++)
    {
      /* Destination IPv4 addresss and prefix */
      dst_prefix = acl->hfilter[i].ace.ipfilter.mask;
      dst_ip_mask = acl->hfilter[i].ace.ipfilter.mask_mask;

      if (dst_prefix != 0 || dst_ip_mask != 0)
        filter_flags |= HSL_ACCGRP_FILTER_DST_IP;

      dst_ip_mask = ~dst_ip_mask;
      dst_prefix &= dst_ip_mask;

      src_prefix = acl->hfilter[i].ace.ipfilter.addr;
      src_ip_mask = acl->hfilter[i].ace.ipfilter.addr_mask;

      if (src_prefix != 0 || src_ip_mask != 0)
        filter_flags |= HSL_ACCGRP_FILTER_SRC_IP;

      if (! filter_flags)
        continue;

      src_ip_mask = ~src_ip_mask;
      src_prefix &= src_ip_mask;
      memset (&qf, 0, sizeof (struct hsl_bcm_accgrp_filter));
      qf.type = hsl_bcm_filter_type_get();
      ret = hsl_bcm_ip_access_group_create (&qf,
                                            filter_flags,
                                            src_prefix,
                                            src_ip_mask,
                                            dst_prefix,
                                            dst_ip_mask,
                                            vid);
      if (ret < 0)
        return ret;

     ret = hsl_bcm_access_group_apply (&qf, acl->hfilter[i].type,
                                       bcmif, direction);
      if (ret < 0)
        {
          hsl_bcm_accgrp_filter_delete (&qf);
          hsl_bcm_accgrp_if_filter_delete_all (bcmif);
          return ret;
        }
      hsl_bcm_accgrp_if_filter_add (bcmif, &qf);
    }


  return ret;
}

static int
hsl_bcm_ip_set_inout_access_group (struct hal_msg_ip_set_access_grp* msg,
                                   int direction)
{
  struct hsl_if_list *node;
  struct hsl_if *ifp = NULL, *ifp2 = NULL;
  struct hsl_bcm_if *bcmif = NULL;
  struct hal_ip_access_grp *hal_ip_grp = &msg->hal_ip_grp;
  int ret;

  HSL_FN_ENTER ();

 /* Get the ifp */
  ifp = hsl_ifmgr_lookup_by_name (msg->ifname);
  if (!ifp)
    goto err_ret;

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      bcmif = ifp->system_info;
      if (! bcmif)
        {
          goto err_ret;
        }
    }
  else if (ifp->type == HSL_IF_TYPE_IP
           && (memcmp (ifp->name, "vlan", 4)))
    {
      ifp2 = hsl_ifmgr_get_first_L2_port (ifp);
      if (! ifp2)
        {
          goto err_ret;
        }

      bcmif = ifp2->system_info;
      if (! bcmif)
        {
          goto err_ret;
        }
    }

  if (memcmp (ifp->name, "vlan", 4))
    {
      if (! bcmif)
        goto err_ret;
      ret = hsl_bcm_set_access_group (hal_ip_grp, bcmif, direction, 0);
      if (ret < 0)
        goto err_ret;
    }
  else
    {
      HSL_IFMGR_LOCK;
      for (node = ifp->children_list; node; node = node->next)
        {

          ifp2 = node->ifp;
          bcmif = ifp2->system_info;

          if (! bcmif)
            continue;

          ret = hsl_bcm_set_access_group (hal_ip_grp, bcmif, direction,
                                          ifp->u.ip.vid);
          if (ret < 0)
            continue;
        }

      HSL_IFMGR_UNLOCK;
      ifp2 = NULL;
    }

  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  if (ifp2)
    HSL_IFMGR_IF_REF_DEC (ifp2);

  HSL_FN_EXIT (0);

 err_ret:
  if (memcmp (ifp->name, "vlan", 4))
    {
      hsl_bcm_accgrp_if_filter_delete_all (bcmif);
      hsl_bcm_accgrp_if_meter_delete_all (bcmif);
    }
  else
    {
      HSL_IFMGR_LOCK;
      node = ifp->children_list;
      while (node)
        {
          ifp2 = node->ifp;
          bcmif = ifp2->system_info;
          hsl_bcm_accgrp_if_filter_delete_all (bcmif);
          hsl_bcm_accgrp_if_meter_delete_all (bcmif);
          node = node->next;
        }
      HSL_IFMGR_UNLOCK;
      ifp2 = NULL;
    }

  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  if (ifp2)
    HSL_IFMGR_IF_REF_DEC (ifp2);

  HSL_FN_EXIT (-1);
}

int
hsl_bcm_ip_unset_inout_access_group(
                                    struct hal_msg_ip_set_access_grp* msg,
                                    int direction)
{
  struct hsl_if_list *node;
  struct hsl_if *ifp = NULL, *ifp2 = NULL;
  struct hsl_bcm_if *bcmif = NULL;

  HSL_FN_ENTER ();

 /* Get the ifp */
  ifp = hsl_ifmgr_lookup_by_name (msg->ifname);
  if (!ifp)
    goto err_return;

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      bcmif = ifp->system_info;
      if (! bcmif)
        {
          goto err_return;
        }
    }
  else if (ifp->type == HSL_IF_TYPE_IP
           && (memcmp (ifp->name, "vlan", 4)))
    {
      ifp2 = hsl_ifmgr_get_first_L2_port (ifp);
      if (! ifp2)
        goto err_return;
      bcmif = ifp2->system_info;
      if (! bcmif)
        goto err_return;
    }

  if (memcmp (ifp->name, "vlan", 4))
    {
      if (! bcmif)
        goto err_return;
      hsl_bcm_accgrp_if_filter_delete_all (bcmif);
      hsl_bcm_accgrp_if_meter_delete_all (bcmif);
    }
  else
    {
      HSL_IFMGR_LOCK;
      for (node = ifp->children_list; node; node = node->next)
        {
          ifp2 = node->ifp;
          bcmif = ifp2->system_info;

          if (! bcmif)
            continue;
          hsl_bcm_accgrp_if_filter_delete_all (bcmif);
          hsl_bcm_accgrp_if_meter_delete_all (bcmif);
        }
      HSL_IFMGR_UNLOCK;
      ifp2 = NULL;
    }
  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  if (ifp2)
    HSL_IFMGR_IF_REF_DEC (ifp2);

  HSL_FN_EXIT (0);

 err_return:

  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);
                                                                                                                          
  if (ifp2)
    HSL_IFMGR_IF_REF_DEC (ifp2);
                                                                                                                          
  HSL_FN_EXIT (0);
}

int
hsl_bcm_ip_set_inout_access_group_interface(
                                struct hal_msg_ip_set_access_grp_interface* msg,
                                int direction)
{
  struct hsl_if_list *node;
  struct hsl_if *vifp = NULL, *ifp = NULL;
  struct hsl_bcm_if *bcmif = NULL;
  struct hal_ip_access_grp *hal_ip_grp = &msg->hal_ip_grp;
  int ret;

  HSL_FN_ENTER ();

 /* Get the ifp */
  vifp = hsl_ifmgr_lookup_by_name (msg->vifname);
  if (!vifp)
    goto err_ret_vlan;

  HSL_IFMGR_LOCK;
  for (node = vifp->children_list; node; node = node->next)
    {

      ifp = node->ifp;
      if (!strcmp (ifp->name, msg->ifname))
        {
          bcmif = ifp->system_info;

          if (! bcmif)
            continue;

          ret = hsl_bcm_set_access_group (hal_ip_grp, bcmif, direction,
                                          vifp->u.ip.vid);
          if (ret < 0)
            continue;
        }
    }

  HSL_IFMGR_UNLOCK;
  ifp = NULL;


  if (vifp)
    HSL_IFMGR_IF_REF_DEC (vifp);

  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);

 err_ret_vlan:
  HSL_IFMGR_LOCK;
  node = vifp->children_list;
    while (node)
      {
        ifp = node->ifp;
        if (!strcmp (ifp->name, msg->ifname))
          {
            bcmif = ifp->system_info;
            hsl_bcm_accgrp_if_filter_delete_all (bcmif);
            hsl_bcm_accgrp_if_meter_delete_all (bcmif);
          }
        node = node->next;
      }
  HSL_IFMGR_UNLOCK;
  ifp = NULL;

  if (vifp)
    HSL_IFMGR_IF_REF_DEC (vifp);

  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (-1);
}

int
hsl_bcm_ip_unset_inout_access_group_interface(
                                struct hal_msg_ip_set_access_grp_interface* msg,
                                int direction)
{
  struct hsl_if_list *node;
  struct hsl_if *vifp = NULL, *ifp = NULL;
  struct hsl_bcm_if *bcmif = NULL;

  HSL_FN_ENTER ();

 /* Get the ifp */
  vifp = hsl_ifmgr_lookup_by_name (msg->vifname);
  if (!vifp)
    goto err_ret_vlan_unset;

  HSL_IFMGR_LOCK;
  for (node = vifp->children_list; node; node = node->next)
    {
      ifp = node->ifp;
      if (!strcmp (ifp->name, msg->ifname))
        {
          bcmif = ifp->system_info;

          if (! bcmif)
            continue;
          hsl_bcm_accgrp_if_filter_delete_all (bcmif);
          hsl_bcm_accgrp_if_meter_delete_all (bcmif);
        }
    }
  HSL_IFMGR_UNLOCK;
  ifp = NULL;

  if (vifp)
    HSL_IFMGR_IF_REF_DEC (vifp);

  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);

 err_ret_vlan_unset:

  if (vifp)
    HSL_IFMGR_IF_REF_DEC (vifp);

  if (ifp)
    HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

int
hsl_bcm_set_ip_access_group (struct hal_msg_ip_set_access_grp *msg)
{
  struct hsl_if *ifp = NULL;
  int direction = msg->dir;
  int ret;

  HSL_FN_ENTER ();

  /* Get the ifp */
  ifp = hsl_ifmgr_lookup_by_name (msg->ifname);
  if (!ifp)
    HSL_FN_EXIT (-1);

  if ((direction == HAL_IP_ACL_DIRECTION_INGRESS) ||
      (direction == HAL_IP_ACL_DIRECTION_EGRESS))
    {
      ret = hsl_bcm_ip_set_inout_access_group (msg, direction);
      if (ret != 0)
        HSL_FN_EXIT (ret);
      HSL_FN_EXIT (0);
    }
  else
    HSL_FN_EXIT (-1);
}

int
hsl_bcm_unset_ip_access_group (struct hal_msg_ip_set_access_grp *msg)
{
  struct hsl_if *ifp = NULL;
  int direction = msg->dir;
  int ret;

  HSL_FN_ENTER ();

  /* Get the ifp */
  ifp = hsl_ifmgr_lookup_by_name (msg->ifname);
  if (!ifp)
    HSL_FN_EXIT (-1);

  if ((direction == HAL_IP_ACL_DIRECTION_INGRESS) ||
      (direction == HAL_IP_ACL_DIRECTION_EGRESS))
    {
      ret = hsl_bcm_ip_unset_inout_access_group (msg, direction);
      if (ret != 0)
        HSL_FN_EXIT (ret);
      HSL_FN_EXIT (0);
    }
  else
    HSL_FN_EXIT (-1);
}

int
hsl_bcm_set_ip_access_group_interface
             (struct hal_msg_ip_set_access_grp_interface *msg)
{
  int direction = msg->dir;
  int ret;

  HSL_FN_ENTER ();

  if ((direction == HAL_IP_ACL_DIRECTION_INGRESS) ||
      (direction == HAL_IP_ACL_DIRECTION_EGRESS))
    {
      ret = hsl_bcm_ip_set_inout_access_group_interface (msg, direction);
      if (ret != 0)
        HSL_FN_EXIT (ret);
      HSL_FN_EXIT (0);
    }
  else
    HSL_FN_EXIT (-1);
}

int
hsl_bcm_unset_ip_access_group_interface
         (struct hal_msg_ip_set_access_grp_interface *msg)
{
  int direction = msg->dir;
  int ret;

  HSL_FN_ENTER ();

  if ((direction == HAL_IP_ACL_DIRECTION_INGRESS) ||
      (direction == HAL_IP_ACL_DIRECTION_EGRESS))
    {
      ret = hsl_bcm_ip_unset_inout_access_group_interface (msg, direction);
      if (ret != 0)
        HSL_FN_EXIT (ret);
      HSL_FN_EXIT (0);
    }
  else
    HSL_FN_EXIT (-1);
}

