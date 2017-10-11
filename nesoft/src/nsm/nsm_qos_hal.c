/* Copyright (C) 2004  All Rights Reserved */
#include "pal.h"
#include "lib.h"

#ifdef HAVE_HAL
#ifdef HAVE_QOS
#include "nsmd.h"

#include "hal_incl.h"

#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#include "nsm_qos_list.h"
#include "nsm_qos_filter.h"

int
nsm_qos_hal_code_acl_filter (struct nsm_master *nm,
                             struct qos_access_list *access,
                             struct hal_acl_filter *hal_acl)
{
  struct qos_filter_list *cfilter;
  struct mac_filter_common *mac_filter;
  struct ip_filter_common *ip_filter;

  struct hal_ipv4_addr *dst_ip_addr;
  struct pal_in4_addr *src_ip_addr;

  struct hal_mac_addr *dst_mac_addr;
  struct mac_filter_addr *src_mac_addr;

  unsigned char *deny_permit;
  int i=0;


  for (cfilter = access->head; cfilter; cfilter = cfilter->next)
  {
    if ((cfilter->acl_type == NSM_QOS_ACL_TYPE_IP) && (i < HAL_MAX_ACL_FILTER))
    {
      ip_filter = &cfilter->u.ifilter;

      /* FFS, copy the name */

      hal_acl->mac_ip_ind = NSM_QOS_ACL_TYPE_IP;
      deny_permit = &hal_acl->ace[i].ifilter.deny_permit;
      *deny_permit = cfilter->type;

      /* Copy source IP address */
      dst_ip_addr = &hal_acl->ace[i].ifilter.addr;
      src_ip_addr = &ip_filter->addr;
      memcpy (dst_ip_addr, src_ip_addr, sizeof (struct hal_ipv4_addr));

      /* Copy source mask */
      dst_ip_addr = &hal_acl->ace[i].ifilter.addr_mask;
      src_ip_addr = &ip_filter->addr_mask;
      memcpy (dst_ip_addr, src_ip_addr, sizeof (struct hal_ipv4_addr));

      /* Copy destination IP address */
      dst_ip_addr = &hal_acl->ace[i].ifilter.mask;
      src_ip_addr = &ip_filter->mask;
      memcpy (dst_ip_addr, src_ip_addr, sizeof (struct hal_ipv4_addr));

      /* Copy destination mask */
      dst_ip_addr = &hal_acl->ace[i].ifilter.mask_mask;
      src_ip_addr = &ip_filter->mask_mask;
      memcpy (dst_ip_addr, src_ip_addr, sizeof (struct hal_ipv4_addr));

      i++;
    }
    else if ((cfilter->acl_type == NSM_QOS_ACL_TYPE_MAC) && (i < HAL_MAX_ACL_FILTER))
    {
      mac_filter = &cfilter->u.mfilter;

      /* FFS, copy the name */
      hal_acl->mac_ip_ind = NSM_QOS_ACL_TYPE_MAC;
      deny_permit = &hal_acl->ace[i].mfilter.deny_permit;
      *deny_permit = cfilter->type;

      /* Copy source MAC address */
      dst_mac_addr = &hal_acl->ace[i].mfilter.a;
      src_mac_addr = &mac_filter->a;
      memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_mac_addr));

      /* Copy source MAC mask */
      dst_mac_addr = &hal_acl->ace[i].mfilter.a_mask;
      src_mac_addr = &mac_filter->a_mask;
      memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_mac_addr));

      /* Copy destination MAC address */
      dst_mac_addr = &hal_acl->ace[i].mfilter.m;
      src_mac_addr = &mac_filter->m;
      memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_mac_addr));

      /* Copy destination MAC mask */
      dst_mac_addr = &hal_acl->ace[i].mfilter.m_mask;
      src_mac_addr = &mac_filter->m_mask;
      memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_mac_addr));

      hal_acl->ace[i].mfilter.priority = mac_filter->priority;  
      hal_acl->ace[i].mfilter.l2_type = mac_filter->packet_format; 
      
      i++;
    }
  }

  /* Set ACE number */
  hal_acl->ace_num = i;

  return HAL_SUCCESS;
}

int
nsm_qos_hal_set_all_class_map(struct nsm_master *nm,
                              struct nsm_pmap_list *pmapl,
                              int ifindex,
                              int action,
                              int dir)
{
  struct nsm_qos_cmap_master *cmap_master = nm->cmap;
  struct nsm_qos_acl_master *acl_master = nm->acl;

  struct qos_access_list *access_list;
  struct nsm_cmap_list *cmapl;

  struct hal_class_map hal_cmap;

  int ret;
  int i;

  int count;
  count = 0;

  /* Clear hal_cmap */
  memset (&hal_cmap, 0, sizeof (struct hal_class_map));

  for (i=0; i < MAX_NUM_OF_CLASS_IN_PMAPL; i++)
  {
    /* Lookup class-map from policy-map list */
    if ((cmapl = nsm_cmap_list_lookup (cmap_master, pmapl->cl_name[i])) != NULL)
      {
        /* Copy class-map name */
        pal_strcpy (&hal_cmap.name[0], cmapl->name);

        /* Copy ACL filter */
        if (pal_strlen(cmapl->acl_name) > 0)
          {
            if ((access_list = qos_access_list_lookup (acl_master, cmapl->acl_name)) != NULL)
              {
                nsm_qos_hal_code_acl_filter (nm, access_list, &hal_cmap.acl);

                /* Set classifier type */
                hal_cmap.clfr_type_ind |= HAL_QOS_CLFR_TYPE_ACL;
              }
          }

        /* Copy VLAN filter */
        if (cmapl->v.num != 0x00)
          {
            hal_cmap.v.num = cmapl->v.num;
            pal_mem_cpy (hal_cmap.v.vlan, cmapl->v.vlan, 2 * HAL_MAX_VLAN_FILTER);
            hal_cmap.clfr_type_ind |= HAL_QOS_CLFR_TYPE_VLAN;
          }

        /* Copy police */
        pal_strncpy (hal_cmap.agg_policer_name, cmapl->agg_policer_name, 
                       HAL_AGG_POLICER_NAME_SIZE);

        hal_cmap.p.avg = cmapl->p.avg;
        hal_cmap.p.dscp = cmapl->p.dscp;
        hal_cmap.p.burst = cmapl->p.burst;
        hal_cmap.p.exd_act = cmapl->p.exd_act;
        hal_cmap.p.fc_mode = cmapl->p.fc_mode;
        hal_cmap.p.excess_burst = cmapl->p.excess_burst;

        if ((cmapl->s.type == NSM_QOS_SET_COS) && 
            (cmapl->s.val == NSM_QOS_COS_INNER))
          {
            ret = hal_qos_set_cmap_cos_inner(&hal_cmap,ifindex, action);
            if (ret < 0)
              return ret;
            hal_cmap.clfr_type_ind |= HAL_QOS_CLFR_TYPE_COS_INNER;
          }
        else if (cmapl->s.type == NSM_QOS_SET_COS)
          {
            hal_cmap.s.type = cmapl->s.type;
            hal_cmap.s.val = cmapl->s.val;

            ret = hal_qos_set_cmap_cos_inner(&hal_cmap,ifindex, action);
            if (ret < 0)
              return ret;
          }
        else
          {
            /* Copy set : cos/dscp/ip-preccedence */
            hal_cmap.s.type = cmapl->s.type;
            hal_cmap.s.val = cmapl->s.val;
          }

        /* Copy trust : cos/dscp/ip-recedence */
        hal_cmap.t.cos_dscp_prec_ind = cmapl->t.cos_dscp_prec_ind;
        hal_cmap.t.val = cmapl->t.val;

        /* Copy DSCP */
        if (cmapl->d.num != 0x00)
          {
            hal_cmap.d.num = cmapl->d.num;
            pal_mem_cpy (hal_cmap.d.dscp, cmapl->d.dscp, HAL_MAX_DSCP_IN_CMAP);
            hal_cmap.clfr_type_ind |= HAL_QOS_CLFR_TYPE_DSCP;
          }

        /* Copy IP-PREC */
        if (cmapl->i.num != 0x00)
          {
            hal_cmap.i.num = cmapl->i.num;
            pal_mem_cpy (hal_cmap.i.prec, cmapl->i.prec, HAL_MAX_IP_PREC_IN_CMAP);
            hal_cmap.clfr_type_ind |= HAL_QOS_CLFR_TYPE_IP_PREC;
          }

        /* Copy EXP */
        if (cmapl->e.num != 0x00)
          {
            hal_cmap.e.num = cmapl->e.num;
            pal_mem_cpy (hal_cmap.e.exp, cmapl->e.exp, HAL_MAX_EXP_IN_CMAP);
            hal_cmap.clfr_type_ind |= HAL_QOS_CLFR_TYPE_EXP;
          }

        /* Layer 4 port */
        if (cmapl->match_type == NSM_QOS_MATCH_TYPE_L4_PORT)
          {
            hal_cmap.l4_port.port_type = cmapl->l4_port.port_type;
            hal_cmap.l4_port.port_id = cmapl->l4_port.port_id;
            hal_cmap.clfr_type_ind |= HAL_QOS_CLFR_TYPE_L4_PORT;
          }

        if (cmapl->match_type == NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC)
          {
            hal_cmap.match_traffic.traffic_type
                                   = cmapl->traffic_type.custom_traffic_type;
            hal_cmap.match_traffic.mode
                                   = cmapl->traffic_type.criteria;
          }

        hal_cmap.drr_priority = cmapl->drr_priority;
        hal_cmap.drr_quantum  = cmapl->drr_quantum;         
        hal_cmap.policing_data_ratio = cmapl->policing_data_ratio;
        hal_cmap.scheduling_algo = cmapl->scheduling_algo;
        pal_mem_cpy(&hal_cmap.queue_priority, &cmapl->queue_priority, 
                    sizeof(struct nsm_cmap_qos_vlan_queue_priority));

        /* Send class-map data */
        ret = hal_qos_set_class_map (&hal_cmap, ifindex, action, dir);

        if (ret < 0)
          return ret;

        else
          /* Clear hal_cmap */
          memset (&hal_cmap, 0, sizeof (struct hal_class_map));
      }
  }

  return HAL_SUCCESS;
}

#endif /* HAVE_QOS */
#endif /* HAVE_HAL */
