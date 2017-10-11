/* Copyright (C) 2004  All Rights Reserved */

#include "pal.h"
#include "lib.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#ifndef HAVE_CUSTOM1

#include "avl_tree.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"


#ifdef HAVE_QOS
#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#include "nsm_qos_list.h"
#include "nsm_qos_filter.h"


/*wangjian@150116, add vport/port_cli header files */

#include "pac/vport/vport.h"
#include "pac/vport/port_cli.h"

#ifdef HAVE_VLAN
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#endif /* HAVE_VLAN */
#ifdef HAVE_HA
#include "nsm_flowctrl.h"
#include "nsm_qos_cal_api.h"
#endif /*HAVE_HA*/

/* Global variable */
struct nsm_qos_global   qosg;
struct map_tbls         default_map_tbls;
struct map_tbls         work_map_tbls;

/*
 * This function is called in NSM initializætion routine
 * Initialize the master of linked list of MAC & IP acl,
 * class-map (cmap), and policy-map (pmap)
 */

int
nsm_mls_qos_vlan_pri_cmp (void *data1, void *data2);

void
nsm_qos_init(struct nsm_master *nm)
{
  pal_mem_set ( &qosg, 0, sizeof (struct nsm_qos_global));
  pal_mem_set ( &default_map_tbls, 0, sizeof (struct map_tbls));
  pal_mem_set ( &work_map_tbls, 0, sizeof (struct map_tbls));

  avl_create (&qosg.vlan_pri_tree, 0, nsm_mls_qos_vlan_pri_cmp);


  /*wangjian@150109, enable qos automatically */

  
  nsm_acl_list_master_init(nm);
  nsm_cmap_list_master_init(nm);
  nsm_pmap_list_master_init(nm);
  
  /* Create dscp mutation map list */
  nsm_dscp_mut_list_master_init (&qosg);
  
  /* Create dscp-cos map list */
  nsm_dscp_cos_list_master_init (&qosg);
  
  /* Create aggregate-policer list */
  nsm_agp_list_master_init (&qosg);
  
  /* Create QoS interface list */
  nsm_qif_list_master_init (&qosg);
  
  /* Initialize all maps */
  nsm_qos_init_maps();

  /*wangjian@150109, disable the dispatching of cosq init to register level */
  
  //nsm_qos_queue_weight_init (&qosg);
  
  //nsm_qos_dscp_to_queue_init (&qosg);
  
  //nsm_qos_cos_to_queue_init (&qosg);
  
  //nsm_qos_set_frame_type_priority_override_init (&qosg);

  //int rv = hal_qos_enable_new();
	
	  /* Set enabled-state */
  qosg.state = NSM_QOS_ENABLED_STATE;
  qosg.config_new = 1;
	
}

/*
 * This function is called in NSM de-initializætion routine
 * De-initialize the master of linked list of MAC & IP acl,
 * class-map (cmap), and policy-map (pmap)
 */
void
nsm_qos_deinit(struct nsm_master *nm)
{
  nsm_acl_list_master_deinit(nm);
  nsm_cmap_list_master_deinit(nm);
  nsm_pmap_list_master_deinit(nm);

  /* Delete dscp mutation map list */
  nsm_dscp_mut_list_master_deinit (&qosg);

  /* Delete dscp-to-cos map list */
  nsm_dscp_cos_list_master_deinit (&qosg);

  /* Delete aggregate-police list */
  nsm_agp_list_master_deinit (&qosg);

  /* Delete qos interface list */
  nsm_qif_list_master_deinit (&qosg);

  pal_mem_set ( &qosg, 0, sizeof (struct nsm_qos_global));
  pal_mem_set ( &default_map_tbls, 0, sizeof (struct map_tbls));
  pal_mem_set ( &work_map_tbls, 0, sizeof (struct map_tbls));

  nsm_mls_qos_vlan_pri_config_deinit ();
}

char *
nsm_qos_trust_state_to_string (int trust_state)
{
  switch (trust_state)
    {
    case NSM_QOS_TRUST_COS:
      return "cos";
    case NSM_QOS_TRUST_DSCP:
      return "dscp";
    case NSM_QOS_TRUST_IP_PREC:
      return "ip-precedence";
    default :
      break;
    }

  return "";
}

struct cli_dscp_map_data
{
  int mapdata[DSCP_TBL_SIZE];
  int count;
};

/* NSM QoS config write. */
int
nsm_qos_config_write (struct cli *cli)
{
  int i;
  struct nsm_master *nm = cli->vr->proto;
  struct qos_access_list *access;
  struct nsm_dscp_mut_list *dscpml;
  struct nsm_agp_list *agpl;
  struct qos_filter_list *filter;
  struct mac_filter_common *mac_filter; 
  struct ip_filter_common *ip_filter;
  struct nsm_dscp_cos_list *dcosl;
  char src_str[100];
  char src_mask_str[100];
  char dst_str[100];
  char dst_mask_str[100];
  int print_src_host, print_dst_host;
  int print_src_mask, print_dst_mask;
  char src_format_str[300];
  char dst_format_str[300];
#ifdef HAVE_VLAN
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master;
#endif /* HAVE_VLAN */

/* wangjian@150110, disable the qos enable write file */
#if 0
  if (qosg.state != NSM_QOS_ENABLED_STATE)
    return 0;

  if (qosg.config_new == 1)
    cli_out (cli, "mls qos enable\n");
  else
    cli_out (cli, "mls qos %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
           qosg.q0[0], qosg.q0[1],
           qosg.q1[0], qosg.q1[1],
           qosg.q2[0], qosg.q2[1],
           qosg.q3[0], qosg.q3[1],
           qosg.q4[0], qosg.q4[1],
           qosg.q5[0], qosg.q5[1],
           qosg.q6[0], qosg.q6[1],
           qosg.q7[0], qosg.q7[1]);

  if (qosg.config_new == PAL_TRUE)
    {
      if (qosg.q0 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 0 %d\n",
                      qosg.q0 [NSM_QOS_QUEUE_WEIGHT_INDEX]);

      if (qosg.q1 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 1 %d\n",
                      qosg.q1 [NSM_QOS_QUEUE_WEIGHT_INDEX]);

      if (qosg.q2 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 2 %d\n",
                      qosg.q2 [NSM_QOS_QUEUE_WEIGHT_INDEX]);

      if (qosg.q3 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 3 %d\n",
                      qosg.q3 [NSM_QOS_QUEUE_WEIGHT_INDEX]);

      if (qosg.q4 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 4 %d\n",
                      qosg.q4 [NSM_QOS_QUEUE_WEIGHT_INDEX]);

      if (qosg.q5 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 5 %d\n",
                      qosg.q5 [NSM_QOS_QUEUE_WEIGHT_INDEX]);

      if (qosg.q6 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 6 %d\n",
                      qosg.q6 [NSM_QOS_QUEUE_WEIGHT_INDEX]);

      if (qosg.q7 [NSM_QOS_QUEUE_WEIGHT_INDEX] != NSM_QOS_QUEUE_WEIGHT_INVALID)
        cli_out (cli, "mls qos wrr-weight 7 %d\n",
                      qosg.q7 [NSM_QOS_QUEUE_WEIGHT_INDEX]);
    }

  for (i = 1; i < 9; i++)
    {
      if (qosg.level[i] != 0)
        cli_out (cli, "mls qos min-reserve %d %d\n", i, qosg.level[i]);
    }

  for (i = 0; i < DSCP_TBL_SIZE; i++)
    {
      if (CHECK_FLAG (qosg.dscp_queue_map_tbl[i], NSM_QOS_DSCP_QUEUE_CONF))
        cli_out (cli, "mls qos dscp-queue %d %d\n", i,
                 qosg.dscp_queue_map_tbl [i] & NSM_QOS_QUEUE_ID_MASK);
    }

  for (i = 0; i < COS_TBL_SIZE; i++)
    {
      if (CHECK_FLAG (qosg.cos_queue_map_tbl[i], NSM_QOS_COS_QUEUE_CONF))
        cli_out (cli, "mls qos cos-queue %d %d\n", i,
                 qosg.cos_queue_map_tbl [i] & NSM_QOS_QUEUE_ID_MASK);
    }

  for (i = 0; i < NSM_QOS_FTYPE_MAX; i++)
    {
      if (CHECK_FLAG (qosg.ftype_pri_tbl [i], NSM_QOS_FTYPE_PRI_OVERRIDE_CONF))
        cli_out (cli, "mls qos frame-type-priority-override %s %d\n",
                 nsm_qos_ftype_to_str (i),
                 qosg.ftype_pri_tbl [i] & NSM_QOS_QUEUE_ID_MASK);
    }
  #endif

#ifdef HAVE_VLAN

  master = nsm_bridge_get_master (nm);

  if (master != NULL)
    {
      for (bridge = master->bridge_list; bridge; bridge = bridge->next)
        {
          if (nsm_bridge_is_vlan_aware(master, bridge->name) == PAL_FALSE)
            continue;

          for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
            {
              if ((vlan = rn->info) == NULL)
                continue;

              if (CHECK_FLAG (vlan->priority, NSM_VLAN_PRIORITY_CONF)
                              == PAL_FALSE)
                continue;

              if (bridge->is_default == PAL_TRUE)
                cli_out (cli, "mls qos vlan-priority-override %d %d\n",
                         vlan->vid, vlan->priority & NSM_VLAN_PRIORITY_MASK);
              else
                cli_out (cli, "mls qos vlan-priority-override "
                              "bridge %s %d %d\n",
                         bridge->name, vlan->vid,
                         vlan->priority & NSM_VLAN_PRIORITY_MASK);
            }

          for (rn = avl_top (bridge->svlan_table); rn; rn = avl_next (rn))
            {
              if ((vlan = rn->info) == NULL)
                continue;

              if (CHECK_FLAG (vlan->priority, NSM_VLAN_PRIORITY_CONF)
                              == PAL_FALSE)
                continue;

              if (bridge->is_default == PAL_TRUE)
                cli_out (cli, "mls qos vlan-priority-override %d %d\n",
                         vlan->vid, vlan->priority & NSM_VLAN_PRIORITY_MASK);
              else
                cli_out (cli, "mls qos vlan-priority-override "
                              "bridge %s %d %d\n",
                         bridge->name, vlan->vid,
                         vlan->priority & NSM_VLAN_PRIORITY_MASK);
            }
        }
     }

#endif /* HAVE_VLAN */

  if (qosg.dcosl)
    {
      struct cli_dscp_map_data cosmap[COS_TBL_SIZE]; 
      char strmap[100], tmpstr[10];
      int i, j;
      
      for (dcosl = qosg.dcosl->head; dcosl; dcosl = dcosl->next)
        {
          pal_mem_set (cosmap, 0, sizeof (cosmap));
          for (i = 0; i < DSCP_TBL_SIZE; i++)
            {
              if (dcosl->d.dscp[i] != i/8)
                {
                  cosmap[dcosl->d.dscp[i]].mapdata[cosmap[dcosl->d.dscp[i]].count] = i;
                  cosmap[dcosl->d.dscp[i]].count++;
                }
            }

          for (i = 0; i < COS_TBL_SIZE; i++)
            {
              if (cosmap[i].count == 0)
                continue;

              pal_mem_set (strmap, 0, pal_strlen (strmap));

              for (j = 0; j < cosmap[i].count; j++)
                {
                  if (j % 8 == 0)
                    {
                      if (strmap[0] != '\0')
                        {
                          cli_out (cli, "%s to %d\n", strmap, i);
                          pal_mem_set (strmap, 0, pal_strlen (strmap));
                        }

                      pal_snprintf (strmap, 100, "mls qos map dscp-cos %s %d", 
                                    dcosl->name, cosmap[i].mapdata[j]);
                    }
                  else
                    {
                      pal_snprintf (tmpstr, 10, " %d", cosmap[i].mapdata[j]);
                      pal_strcat (strmap, tmpstr);
                    }
                }
              
              if (strmap[0] != '\0')
                {
                  cli_out (cli, "%s to %d\n", strmap, i);
                  pal_mem_set (strmap, 0, pal_strlen (strmap));
                }
            }
        }
    }

  for (i = 0; i < DSCP_TBL_SIZE; i++)
    {
      if (work_map_tbls.policed_dscp_map_tbl[i] != i)
        cli_out (cli, "mls qos map policed-dscp %d to %d\n",
                 i, work_map_tbls.policed_dscp_map_tbl[i]);
    }

  if (qosg.dscpml && qosg.dscpml->head)
    {
      struct cli_dscp_map_data dscpmap[DSCP_TBL_SIZE]; 
      char strmap[100], tmpstr[10];
      int i, j;
      
      for (dscpml = qosg.dscpml->head; dscpml; dscpml = dscpml->next)
        {
          pal_mem_set (dscpmap, 0, sizeof (dscpmap));
          for (i = 0; i < DSCP_TBL_SIZE; i++)
            {
              if (dscpml->d.dscp[i] != i)
                {
                  dscpmap[dscpml->d.dscp[i]].mapdata[dscpmap[dscpml->d.dscp[i]].count] = i;
                  dscpmap[dscpml->d.dscp[i]].count++;
                }
            }
          
          for (i = 0; i < DSCP_TBL_SIZE; i++)
            {
              if (dscpmap[i].count == 0)
                continue;

              pal_mem_set (strmap, 0, pal_strlen (strmap));

              for (j = 0; j < dscpmap[i].count; j++)
                {
                  if (j % 8 == 0)
                    {
                      if (strmap[0] != '\0')
                        {
                          cli_out (cli, "%s to %d\n", strmap, i);
                          pal_mem_set (strmap, 0, pal_strlen (strmap));
                        }

                      pal_snprintf (strmap, 100, "mls qos map dscp-mutation %s %d", 
                                    dscpml->name, dscpmap[i].mapdata[j]);
                    }
                  else
                    {
                      pal_snprintf (tmpstr, 10, " %d", dscpmap[i].mapdata[j]);
                      pal_strcat (strmap, tmpstr);
                    }
                }
              
              if (strmap[0] != '\0')
                {
                  cli_out (cli, "%s to %d\n", strmap, i);
                  pal_mem_set (strmap, 0, pal_strlen (strmap));
                }
            }
        }
    }
  
  if (qosg.agp && qosg.agp->head)
    {
      for (agpl = qosg.agp->head; agpl; agpl = agpl->next)
        {
          switch (agpl->p.exd_act)
            {
            case NSM_QOS_EXD_ACT_DROP:
              cli_out (cli, "mls qos aggregate-police %s %d %d exceed-action drop\n",
                       agpl->name, agpl->p.avg, agpl->p.burst);
              break;
            case NSM_QOS_EXD_ACT_POLICED_DSCP_TX:
              cli_out (cli, "mls qos aggregate-police %s %d %d exceed-action policed-dscp-transmit\n",
                       agpl->name, agpl->p.avg, agpl->p.burst);
              break;
            default:
              continue;
            }
        }
    }

  if (nm->acl)
    {
      for (access = nm->acl->num.head; access; access = access->next)
        {
          for (filter = access->head; filter; filter = filter->next)
            {
              if (filter->acl_type == NSM_QOS_ACL_TYPE_MAC)
                {
                  mac_filter = &filter->u.mfilter;
                  pal_snprintf (src_str, 100,
                                "%02x%02x.%02x%02x.%02x%02x",
                                mac_filter->a.mac[0], mac_filter->a.mac[1], 
                                mac_filter->a.mac[2], mac_filter->a.mac[3], 
                                mac_filter->a.mac[4], mac_filter->a.mac[5]); 
                  pal_snprintf (src_mask_str, 100,
                                "%02x%02x.%02x%02x.%02x%02x",
                                mac_filter->a_mask.mac[0], mac_filter->a_mask.mac[1], 
                                mac_filter->a_mask.mac[2], mac_filter->a_mask.mac[3], 
                                mac_filter->a_mask.mac[4], mac_filter->a_mask.mac[5]); 
                  pal_snprintf (dst_str, 100,
                                "%02x%02x.%02x%02x.%02x%02x",
                                mac_filter->m.mac[0], mac_filter->m.mac[1], 
                                mac_filter->m.mac[2], mac_filter->m.mac[3], 
                                mac_filter->m.mac[4], mac_filter->m.mac[5]); 
                  pal_snprintf (dst_mask_str, 100,
                                "%02x%02x.%02x%02x.%02x%02x",
                                mac_filter->m_mask.mac[0], mac_filter->m_mask.mac[1], 
                                mac_filter->m_mask.mac[2], mac_filter->m_mask.mac[3], 
                                mac_filter->m_mask.mac[4], mac_filter->m_mask.mac[5]); 

                  if (mac_filter->priority != NSM_QOS_INVALID_DOT1P_PRI)
                    {
                      cli_out (cli, "mac-access-list %s %s %s priority %d\n",
                               access->name, filter->type == QOS_FILTER_SOURCE ?
                               "source" : "destination", src_str,
                               mac_filter->priority);
                    }
                  else if (qos_compare_mac_filter (&mac_filter->a.mac[0],
                                                   0) == 0
                           && qos_compare_mac_filter (&mac_filter->a_mask.mac[0],
                                                      0xff) == 0)
                    {
                      cli_out (cli, "mac-access-list %s %s any %s %s %d\n",
                               access->name, filter->type == QOS_FILTER_DENY ? "deny" : "permit",
                               dst_str, dst_mask_str, mac_filter->packet_format); 
                    }
                  else if (qos_compare_mac_filter (&mac_filter->m.mac[0], 0) == 0 &&
                           qos_compare_mac_filter (&mac_filter->m_mask.mac[0], 0xff) == 0) 
                    {
                      cli_out (cli, "mac-access-list %s %s %s %s any %d\n",
                               access->name, filter->type == QOS_FILTER_DENY ? "deny" : "permit",
                               src_str, src_mask_str, mac_filter->packet_format); 
                    }
                  else
                    {
                      cli_out (cli, "mac-access-list %s %s %s %s %s %s %d\n",
                               access->name, filter->type == QOS_FILTER_DENY ? "deny" : "permit",
                               src_str, src_mask_str, dst_str, dst_mask_str, mac_filter->packet_format); 
                    }
                }
              else if (filter->acl_type == NSM_QOS_ACL_TYPE_IP)
                {
                  ip_filter = &filter->u.ifilter;
                  pal_inet_ntop (AF_INET, (void *)&ip_filter->addr_config, src_str, 100);
                  pal_inet_ntop (AF_INET, (void *)&ip_filter->addr_mask, src_mask_str, 100);
                  pal_inet_ntop (AF_INET, (void *)&ip_filter->mask_config, dst_str, 100);
                  pal_inet_ntop (AF_INET, (void *)&ip_filter->mask_mask, dst_mask_str, 100);

                  if (ip_filter->extended == 0)
                    {
                      print_src_mask = 1;
                      if (ip_filter->addr_mask.s_addr == 0)
                        {
                          print_src_mask = 0;
                        }

                      if (ip_filter->addr_mask.s_addr == 0xffffffff)
                        {
                          pal_snprintf (src_str, 100, "any");
                          print_src_mask = 0;
                        }
                      
                      if (print_src_mask)
                        {
                          cli_out (cli, "ip-access-list %s %s %s %s\n",
                                   access->name, filter->type == QOS_FILTER_DENY ? "deny" : "permit",
                                   src_str, src_mask_str);
                        }
                      else
                        {
                          cli_out (cli, "ip-access-list %s %s %s\n",
                                   access->name, filter->type == QOS_FILTER_DENY ? "deny" : "permit",
                                   src_str);
                        }
                    }
                  else 
                    {
                      print_src_host = 0;
                      print_dst_host = 0;
                      print_src_mask = 1;
                      print_dst_mask = 1;

                      if (ip_filter->mask_mask.s_addr == 0xffffffff)
                        {
                          pal_snprintf (dst_str, 100, "any");
                          print_dst_mask = 0;
                        }
                      
                      if (ip_filter->mask_mask.s_addr == 0)
                        {
                          print_dst_host = 1;
                          print_dst_mask = 0;
                        }

                      if (ip_filter->addr_mask.s_addr == 0)
                        {
                          print_src_host = 1;
                          print_src_mask = 0;
                        }

                      if (ip_filter->addr_mask.s_addr == 0xffffffff)
                        {
                          pal_snprintf (src_str, 100, "any");
                          print_src_mask = 0;
                        }
                      
                      if (print_src_host)
                        {
                          pal_snprintf (src_format_str, 300, "%s %s", "host", src_str);
                        }
                      else if (print_src_mask == 0)
                        {
                          pal_snprintf (src_format_str, 300, "%s", src_str);
                        }
                      else
                        {
                          pal_snprintf (src_format_str, 300, "%s %s", src_str, src_mask_str);
                        }

                      if (print_dst_host)
                        {
                          pal_snprintf (dst_format_str, 300, "%s %s", "host", dst_str);
                        }
                      else if (print_dst_mask == 0)
                        {
                          pal_snprintf (dst_format_str, 300, "%s", dst_str);
                        }
                      else
                        {
                          pal_snprintf (dst_format_str, 300, "%s %s", dst_str, dst_mask_str);
                        }
                      
                      cli_out (cli, "ip-access-list %s %s ip %s %s\n",
                               access->name, filter->type == QOS_FILTER_DENY ? "deny" : "permit",
                               src_format_str, dst_format_str);
                    }
                }
            }
        }
    }

  cli_out (cli, "!\n");


  return 0;
}


int
nsm_qos_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct nsm_qif_list *qifl;
  char strmap[200], tmpstr[10];
  int i, j, display_cli = 0;
  

  if (qosg.state != NSM_QOS_ENABLED_STATE)
    return 0;

  if (ifp->trust_state)
    cli_out (cli, " mls qos trust %s\n", 
             nsm_qos_trust_state_to_string (ifp->trust_state));

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (qifl)
    {
      if (qifl->dscp_mut_name[0] != '\0')
        cli_out (cli, " mls qos dscp-mutation %s\n", qifl->dscp_mut_name);

      if (qifl->dscp_cos_name[0] != '\0')
        cli_out (cli, " mls qos dscp-cos %s\n", qifl->dscp_cos_name);

      if (qifl->input_pmap_name[0] != '\0')
        cli_out (cli, " service-policy input %s\n", qifl->input_pmap_name);

      if (qifl->output_pmap_name[0] != '\0')
        cli_out (cli, " service-policy output %s\n", qifl->output_pmap_name);

      if (qifl->port_mode == NSM_QOS_VLAN_PRI_OVERRIDE_COS)
        cli_out (cli, " mls qos vlan-priority-override cos\n");
      else if (qifl->port_mode == NSM_QOS_VLAN_PRI_OVERRIDE_QUEUE)
        cli_out (cli, " mls qos vlan-priority-override queue\n");
      else if (qifl->port_mode == NSM_QOS_VLAN_PRI_OVERRIDE_BOTH)
        cli_out (cli, " mls qos vlan-priority-override both\n");

      if (qifl->da_port_mode == NSM_QOS_DA_PRI_OVERRIDE_COS)
        cli_out (cli, " mls qos da-priority-override cos\n");
      else if (qifl->da_port_mode == NSM_QOS_DA_PRI_OVERRIDE_QUEUE)
        cli_out (cli, " mls qos da-priority-override queue\n");
      else if (qifl->da_port_mode == NSM_QOS_DA_PRI_OVERRIDE_BOTH)
        cli_out (cli, " mls qos da-priority-override both\n");

      if (qifl->trust_type == NSM_QOS_TRUST_COS)
        cli_out (cli, " mls qos trust cos\n");
      else if (qifl->trust_type == NSM_QOS_TRUST_DSCP)
        cli_out (cli, " mls qos trust dscp\n");
      else if (qifl->trust_type == NSM_QOS_TRUST_DSCP_COS)
        cli_out (cli, " mls qos trust both\n");

      if (qifl->force_trust_cos == PAL_TRUE)
        cli_out (cli, " mls qos force-trust cos\n");

      if (qifl->def_cos != NSM_QOS_DEFAULT_COS)
        cli_out (cli, " mls qos cos %d\n", qifl->def_cos);

      if (qifl->strict_queue == NSM_QOS_STRICT_QUEUE_ALL)
        cli_out (cli, " mls qos strict queue all\n");
      else 
        {
           if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE0))
             cli_out (cli, " mls qos strict queue 0\n");
           if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE1))
             cli_out (cli, " mls qos strict queue 1\n");
           if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE2))
             cli_out (cli, " mls qos strict queue 2\n");
           if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE3))
             cli_out (cli, " mls qos strict queue 3\n");
         }

      if (qifl->egress_shape_rate != NSM_QOS_NO_TRAFFIC_SHAPE)
        {
          if (qifl->egress_rate_unit == NSM_QOS_RATE_KBPS)
            cli_out (cli, " traffic-shape rate %d kbps\n",
                     qifl->egress_shape_rate);
          else
            cli_out (cli, " traffic-shape rate %d fps\n",
                     qifl->egress_shape_rate);
        }

      if (FLAG_ISSET (qifl->config, NSM_QOS_IF_CONFIG_PRIORITY_QUEUE))
        {
                cli_out (cli, " priority-queue out\n");
          if (FLAG_ISSET (qifl->config, NSM_QOS_IF_CONFIG_WEIGHT))
                  cli_out (cli, " wrr-queue bandwidth %d %d %d %d %d %d %d\n",
                         qifl->weight[0], 
                         qifl->weight[1], 
                         qifl->weight[2], 
                         qifl->weight[3], 
                         qifl->weight[4], 
                         qifl->weight[5], 
                         qifl->weight[6]); 
       }
     else
       {
         if (FLAG_ISSET (qifl->config, NSM_QOS_IF_CONFIG_WEIGHT))
                 cli_out (cli, " wrr-queue bandwidth %d %d %d %d %d %d %d %d\n",
                         qifl->weight[0], 
                         qifl->weight[1], 
                         qifl->weight[2], 
                         qifl->weight[3], 
                         qifl->weight[4], 
                         qifl->weight[5], 
                         qifl->weight[6], 
                         qifl->weight[7]);
       }
      /*
      if (FLAG_ISSET (qifl->config, NSM_QOS_IF_CONFIG_QUEUE_LIMIT))
        cli_out (cli, " wrr-queue weight %d %d %d %d %d %d %d %d\n",
                 qifl->weight[0], 
                 qifl->weight[1], 
                 qifl->weight[2], 
                 qifl->weight[3], 
                 qifl->weight[4], 
                 qifl->weight[5], 
                 qifl->weight[6], 
                 qifl->weight[7]);
*/
      for (i = 0; i < MAX_NUM_OF_QUEUE; i++)
        {
          display_cli = 0;
          for (j = 0; j <= NSM_QOS_COS_MAX; j++)
            {
              if (i == j)
                continue;

              if (qifl->cosq_map[j] == i)
                {
                  if (display_cli == 0)
                    {
                      pal_snprintf (strmap, 200, " wrr-queue cos-map %d %d", i, j);
                      display_cli = 1;
                    }
                  else
                    {
                      pal_snprintf (tmpstr, 10, " %d", j);
                      pal_strcat (strmap, tmpstr);
                    }
                }
            }
          if (display_cli == 1)
            cli_out (cli, "%s\n", strmap);
        }
     /*
      for (i = 0; i < MAX_NUM_OF_QUEUE; i++)
        {
          if (FLAG_ISSET (qifl->queue[i].config, NSM_QOS_IFQ_CONFIG_WRED_THRESHOLD))
            {
              cli_out (cli, " wrr-queue wred %d %d %d %d %d\n", 
                       i, qifl->queue[i].color, qifl->queue[i].startpoint,qifl->queue[i].slope,qifl->queue[i].time);
            }
        }

      for (i = 0; i < MAX_NUM_OF_QUEUE; i++)
        {
          if (FLAG_ISSET (qifl->queue[i].config, NSM_QOS_IFQ_CONFIG_TD_THRESHOLD))
            {
              cli_out (cli, " wrr-queue car %d %d %d\n", 
                       i, qifl->queue[i].td_threshold1, qifl->queue[i].td_threshold2);
            }
        }
    */

      for (i = 0; i < MAX_NUM_OF_QUEUE; i++)
        {
          if (FLAG_ISSET (qifl->queue[i].config, NSM_QOS_IFQ_CONFIG_MIN_RESERVE))
            {
              cli_out (cli, " wrr-queue min-reserve %d %d\n", 
                       i, qifl->queue[i].min_reserve_level);
            }
        }

     for (i = 1; i <= 2; i++)
       {
               display_cli = 0;
         for (j = 0; j < NSM_QOS_DSCP_MAX; j++)
           {
             if (qifl->wred_dscp_threshold[j] == i)
               {
                             if (display_cli == 0)
                               {
                                 pal_snprintf (strmap, 200, " wrr-queue dscp-map %d %d", i, j);
                                 display_cli = 1;
                               }
                             else
                               {
                                 pal_snprintf (tmpstr, 10, " %d", j);
                                 pal_strcat (strmap, tmpstr);
                               }
               }
           }
              if (display_cli == 1)
                cli_out (cli, "%s\n", strmap);
       }
    }

  return 0;
}

int
nsm_qos_cmap_config_write (struct cli *cli)
{
  int i; 
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl;
  int start_index, end_index;
  u_int16_t expected_vid;
  

  if (! nm->cmap)
    return 0;

  for (cmapl = nm->cmap->head; cmapl; cmapl = cmapl->next)
    {
      cli_out (cli, "class-map %s\n", cmapl->name);
      if (cmapl->acl_name_set)
        cli_out (cli, " match access-group %s\n", cmapl->acl_name);
      
      if (cmapl->d.num > 0)
        {
          cli_out (cli, " match ip-dscp");            
          for (i = 0; i < cmapl->d.num; i++)
            {
              cli_out (cli, " %d", cmapl->d.dscp[i]);
            }
        
          cli_out (cli, "\n");
        }

      if (cmapl->i.num > 0)
        {
          cli_out (cli, " match ip-precedence");              
          for (i = 0; i < cmapl->i.num; i++)
            {
              cli_out (cli, " %d", cmapl->i.prec[i]);
            }
          
          cli_out (cli, "\n");
        }

      if (cmapl->e.num > 0)
        {
          cli_out (cli, " match mpls exp-bit topmost");       
          for (i = 0; i < cmapl->e.num; i++)
            {
              cli_out (cli, " %d", cmapl->e.exp[i]);
            }
        
          cli_out (cli, "\n");
        }

      if (cmapl->match_type == NSM_QOS_MATCH_TYPE_L4_PORT)
        {
          cli_out (cli, " match layer4 %s %d\n",
                   cmapl->l4_port.port_type == NSM_QOS_LAYER4_PORT_SRC
                   ? "source-port" : "destination-port",
                   cmapl->l4_port.port_id);
        }

      for (i = 0, start_index = 0, end_index = 0, 
             expected_vid = cmapl->v.vlan[end_index] + 1;
           i < cmapl->v.num; i++)
        {
          if ((i+1) >= cmapl->v.num)
            {
              if (start_index < end_index)
                cli_out (cli, " match vlan-range %hu to %hu\n", 
                         cmapl->v.vlan[start_index], 
                         cmapl->v.vlan[end_index]);
              else
                cli_out (cli, " match vlan %hu\n", 
                         cmapl->v.vlan[start_index]);
              continue;
            }
          
          if (cmapl->v.vlan[i+1] != expected_vid)
            {
              if (start_index < end_index)
                cli_out (cli, " match vlan-range %hu to %hu\n", 
                         cmapl->v.vlan[start_index], 
                         cmapl->v.vlan[end_index]);
              else
                cli_out (cli, " match vlan %hu\n", 
                         cmapl->v.vlan[start_index]);
              
              start_index = i + 1;
              end_index = i + 1;
              expected_vid = cmapl->v.vlan[end_index] + 1;
            }
          else
            {
              end_index = i + 1;
              expected_vid = cmapl->v.vlan[end_index] + 1;
            }
        }

      if (cmapl->match_type == NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC)
        {
           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_UNKNOWN_UNICAST))
             cli_out (cli, " match traffic-type unknown-unicast %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_UNKNOWN_MULTICAST))
             cli_out (cli, " match traffic-type unknown-multicast %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_BROADCAST))
             cli_out (cli, " match traffic-type broadcast %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_MULTICAST))
             cli_out (cli, " match traffic-type multicast %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_UNICAST))
             cli_out (cli, " match traffic-type unicast %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_MGMT))
             cli_out (cli, " match traffic-type management %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_ARP))
             cli_out (cli, " match traffic-type arp %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_TCP_DATA))
             cli_out (cli, " match traffic-type tcp-data %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_TCP_CTRL))
             cli_out (cli, " match traffic-type tcp-control %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_UDP))
             cli_out (cli, " match traffic-type udp %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_NON_TCP_UDP))
             cli_out (cli, " match traffic-type non-tcp-udp %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_QUEUE0))
             cli_out (cli, " match traffic-type queue0 %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_QUEUE1))
             cli_out (cli, " match traffic-type queue1 %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_QUEUE2))
             cli_out (cli, " match traffic-type queue2 %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_QUEUE3))
             cli_out (cli, " match traffic-type queue3 %s\n",
                      nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

           if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                           NSM_QOS_TRAFFIC_ALL))
             cli_out (cli, " match traffic-type all\n");
        }

      cli_out (cli, "!\n"); 
    }
  
  return 0;
}

int
nsm_qos_pmap_config_write (struct cli *cli)
{
  int i;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl;
  struct nsm_cmap_list *cmapl;
  
  if (! nm->pmap)
    return 0;
  
  for (pmapl = nm->pmap->head; pmapl; pmapl = pmapl->next)
    {
      cli_out (cli, "policy-map %s\n", pmapl->name);

      for (i = 0; i < MAX_NUM_OF_CLASS_IN_PMAPL; i++)
        {
          if (pmapl->cl_name[i][0] == 0x00)
            continue;
          
          cli_out (cli, " class %s\n", pmapl->cl_name[i]);
          cmapl = nsm_qos_check_cmap_in_all_cmapls (nm->cmap, 
                                                    pmapl->cl_name[i]);
          if (! cmapl)
            continue;
          
          if (cmapl->t.cos_dscp_prec_ind && cmapl->t.val)
            {
              switch (cmapl->t.cos_dscp_prec_ind)
                {
                case NSM_QOS_TRUST_COS:
                  cli_out (cli, "  trust cos\n");
                  break;
                case NSM_QOS_TRUST_DSCP:
                  cli_out (cli, "  trust dscp\n");
                  break;
                case NSM_QOS_TRUST_IP_PREC:
                  cli_out (cli, "  trust ip-prec\n");
                  break;
                default:
                  break;
                }
            }
          
          if (cmapl->s.type) 
            {
              switch (cmapl->s.type)
                {
                case NSM_QOS_SET_COS:
                  if (cmapl->s.val == NSM_QOS_COS_INNER)
                    cli_out (cli, "  set cos cos-inner\n");
                  else
                    cli_out (cli, "  set cos %d\n", cmapl->s.val);
                  break;
                case NSM_QOS_SET_DSCP:
                  cli_out (cli, "  set ip-dscp %d\n", cmapl->s.val);
                  break;
                case NSM_QOS_SET_IP_PREC:
                  cli_out (cli, "  set ip-precedence %d\n", cmapl->s.val);
                  break;
                case NSM_QOS_SET_EXP:
                  cli_out (cli, "  set mpls exp-bit %d\n", cmapl->s.val);
                  break;
                default:
                  break;
                }
            }

          if (cmapl->agg_policer_name_set)
            cli_out (cli, "  police-aggregate %s\n", cmapl->agg_policer_name);
          else if (cmapl->p.avg 
                   && cmapl->match_type == NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC
                   && cmapl->p.exd_act && cmapl->p.excess_burst
                   && cmapl->p.fc_mode)
            cli_out (cli, "  police %d %d %d exceed-action %s "
                          "reset-flow-control-mode"
                          " available-bucket-room %s\n", cmapl->p.avg,
                     cmapl->p.burst,
                     cmapl->p.excess_burst,
                     cmapl->p.exd_act == NSM_QOS_EXD_ACT_DROP
                     ? "drop" : "flow-control",
                     cmapl->p.fc_mode == NSM_RESUME_ON_FULL_BUCKET_SIZE
                     ? "full" : "cbs");
          else if (cmapl->p.avg && cmapl->p.exd_act)
            cli_out (cli, "  police %d %d exceed-action %s\n", cmapl->p.avg, 
                     cmapl->p.burst, 
                     cmapl->p.exd_act == NSM_QOS_EXD_ACT_DROP 
                     ? "drop" : "policed-dscp-transmit");

          if (cmapl->policing_data_ratio != 0)
            {
              cli_out (cli, "  policing meter %d\n",
                       cmapl->policing_data_ratio);
            }

          if (cmapl->drr_quantum != 0)
            {
              cli_out (cli, "  set drr-priority %d quantum %d\n",
                       cmapl->drr_priority, cmapl->drr_quantum);
            }

          if (cmapl->scheduling_algo != 0)
            {
              if (cmapl->scheduling_algo == NSM_CMAP_QOS_ALGORITHM_STRICT)
                  cli_out (cli, "  set algorithm strict\n");
              if (cmapl->scheduling_algo == NSM_CMAP_QOS_ALGORITHM_DRR)
                  cli_out (cli, "  set algorithm drr\n");
              if (cmapl->scheduling_algo == NSM_CMAP_QOS_ALGORITHM_DRR_STRICT)
                  cli_out (cli, "  set algorithm drr-strict\n");
            }

          if (cmapl->vlan_priority_set == PAL_TRUE)
            {
              cli_out (cli, "  set vlan-priority");
              cli_out (cli, " %d", cmapl->queue_priority.q0);
              cli_out (cli, " %d", cmapl->queue_priority.q1);
              cli_out (cli, " %d", cmapl->queue_priority.q2);
              cli_out (cli, " %d", cmapl->queue_priority.q3);
              cli_out (cli, " %d", cmapl->queue_priority.q4);
              cli_out (cli, " %d", cmapl->queue_priority.q5);
              cli_out (cli, " %d", cmapl->queue_priority.q6);
              cli_out (cli, " %d\n", cmapl->queue_priority.q7);
            }
          cli_out (cli, "!\n");
        }
    }

  cli_out (cli, "!\n");
 
  return 0;
}

int
nsm_qos_pmapc_config_write (struct cli *cli)
{
  int write = 0;

  /* FFS */
  return ++write;
}

int
nsm_qos_chk_if_qos_enabled (struct nsm_qos_pmap_master *pmap)
{
  if (pmap)
    return 0;   /* QoS-enabled state */
  else
    return 1;   /* QoS-disabled state */
}

int
nsm_qos_print_class_map (struct cli *cli, struct nsm_cmap_list *cmapl, char *name)
{
  int i;


  cli_out (cli, "    CLASS-MAP-NAME: %s\n", cmapl->name);
  if (cmapl->acl_name_set)
    cli_out (cli, "      QOS-ACCESS-LIST-NAME: %s\n", cmapl->acl_name);
  if (cmapl->v.num != 0)
    {
      cli_out (cli, "      Match vlan:");
      for (i=0 ; i < MAX_NUM_OF_VLAN_FILTER ; i++)
        if (cmapl->v.vlan[i] != 0)
          cli_out (cli, " %d", cmapl->v.vlan[i]);
      cli_out (cli, "\n");
    }
  if (cmapl->p.avg != 0)
    {
      cli_out (cli, "      Police:");
      cli_out (cli, " average rate (%d kbps)", cmapl->p.avg);
      cli_out (cli, "\n\t      burst size (%d bytes)", cmapl->p.burst);
      if (cmapl->p.exd_act == NSM_QOS_EXD_ACT_DROP)
        cli_out (cli, "\n\t      exceed-action (drop)");
      else if (cmapl->p.exd_act == NSM_QOS_EXD_ACT_POLICED_DSCP_TX)
        cli_out (cli, "\n\t      exceed-action (policed-dscp-tx)");
      else if (cmapl->p.exd_act == NSM_QOS_EXD_ACT_POLICE_FLOW_CONTROL)
        cli_out (cli, "\n\t      exceed-action (send-pause-frame)");
      else
        cli_out (cli, "\n\t      exceed-action (none)");
      cli_out (cli, "\n\t      excess burst size (%d bytes)", 
               cmapl->p.excess_burst);
      cli_out (cli, "\n\t      flow control mode (%s)", 
      cmapl->p.fc_mode == NSM_RESUME_ON_FULL_BUCKET_SIZE ? 
      "resume-on-full-bucket-size" : 
      cmapl->p.fc_mode == NSM_RESUME_ON_CBS_BUCKET_SIZE ? 
      "resume-on-CBS-bucket-size" : "none");
      cli_out (cli, "\n");
    }
  if (cmapl->s.type)
    {
      switch (cmapl->s.type)
        {
        case NSM_QOS_SET_COS:
          cli_out (cli, "      Set CoS: %d\n", cmapl->s.val);
          break;
        case NSM_QOS_SET_DSCP:
          cli_out (cli, "      Set IP DSCP: %d\n", cmapl->s.val);
          break;
        case NSM_QOS_SET_IP_PREC:
          cli_out (cli, "      Set IP precedence: %d\n", cmapl->s.val);
          break;
        case NSM_QOS_SET_EXP:
          cli_out (cli, "      Set MPLS exp-bit: %d\n", cmapl->s.val);
          break;
        default:
          cli_out (cli, "      wrong set command: %d\n", cmapl->s.val);
          break;
        }
    }
  if (cmapl->t.cos_dscp_prec_ind)
    {
      switch (cmapl->t.val)
        {
        case NSM_QOS_TRUST_NONE:
          cli_out (cli, "      Trust state: none\n");
          break;
        case NSM_QOS_TRUST_COS:
          cli_out (cli, "      Trust state: CoS\n");
          break;
        case NSM_QOS_TRUST_DSCP:
          cli_out (cli, "      Trust state: IP-DSCP\n");
          break;
        case NSM_QOS_TRUST_IP_PREC:
          cli_out (cli, "      Trust state: IP-Precedence\n");
          break;
        default:
          cli_out (cli, "      Trust state: wrong\n");
          break;
        }
    }
  if (cmapl->d.num != 0)
    {
      cli_out (cli, "      Match IP DSCP:");

      for (i=0 ; i < cmapl->d.num; i++)
         cli_out (cli, " %d", cmapl->d.dscp[i]);

      cli_out (cli, "\n");
    }
  if (cmapl->i.num != 0)
    {
      cli_out (cli, "      Match IP precedence:");

      for (i=0 ; i < cmapl->i.num; i++)
         cli_out (cli, " %d", cmapl->i.prec[i]);

      cli_out (cli, "\n");
    }
  if (cmapl->e.num != 0)
    {
      cli_out (cli, "      Match mpls exp-bit:");

      for (i=0 ; i < cmapl->e.num; i++)
         cli_out (cli, " %d", cmapl->e.exp[i]);

      cli_out (cli, "\n");
    }

   if ((cmapl->l4_port.port_type == NSM_QOS_LAYER4_PORT_SRC) &&
       (cmapl->l4_port.port_id != 0))
     {
       cli_out (cli, "      Match Source Port: %d\n",
                cmapl->l4_port.port_id);
     }
   else if ((cmapl->l4_port.port_type == NSM_QOS_LAYER4_PORT_DST) &&
            (cmapl->l4_port.port_id != 0))
     {
       cli_out (cli, "      Match Destination Port: %d\n", 
                cmapl->l4_port.port_id);
     }
   if (cmapl->drr_set == 1)
     {
       cli_out (cli, "\n      DRR-Priority:%d  Quantum: %d\n\n", 
                cmapl->drr_priority, cmapl->drr_quantum);
     }
  if (cmapl->policing_set == 1)
    {
       if (cmapl->policing_set) 
          cli_out (cli, "      Policing Meter:%d\n\n", 
                                    cmapl->policing_data_ratio);
    }
  if (cmapl->scheduling_algo_set == 1)
    {
      if (cmapl->scheduling_algo == NSM_CMAP_QOS_ALGORITHM_STRICT)
          cli_out (cli, "      Scheduling Algorithm: strict\n\n");
      if (cmapl->scheduling_algo == NSM_CMAP_QOS_ALGORITHM_DRR)
          cli_out (cli, "      Scheduling Algorithm: drr\n\n");
      if (cmapl->scheduling_algo == NSM_CMAP_QOS_ALGORITHM_DRR_STRICT)
          cli_out (cli, "      Scheduling Algorithm: drr-strict\n\n");

    }
  if (cmapl->vlan_priority_set == 1)
    {
      cli_out (cli, "      VLAN Queue Priority Mapping\n");
      cli_out (cli, "          Queue 0, Priority %d\n",
               cmapl->queue_priority.q0);
      cli_out (cli, "          Queue 1, Priority %d\n",
               cmapl->queue_priority.q1);
      cli_out (cli, "          Queue 2, Priority %d\n",
               cmapl->queue_priority.q2);
      cli_out (cli, "          Queue 3, Priority %d\n",
               cmapl->queue_priority.q3);
      cli_out (cli, "          Queue 4, Priority %d\n",
               cmapl->queue_priority.q4);
      cli_out (cli, "          Queue 5, Priority %d\n",
               cmapl->queue_priority.q5);
      cli_out (cli, "          Queue 6, Priority %d\n",
               cmapl->queue_priority.q6);
      cli_out (cli, "          Queue 7, Priority %d\n",
               cmapl->queue_priority.q7);
    }
   if (cmapl->match_type == NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC)
    {
      if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                      NSM_QOS_TRAFFIC_UNKNOWN_UNICAST))
        cli_out (cli, " match traffic-type unknown-unicast %s\n",
                 nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

      if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                      NSM_QOS_TRAFFIC_UNKNOWN_MULTICAST))
         cli_out (cli, " match traffic-type unknown-multicast %s\n",
                  nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

      if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                      NSM_QOS_TRAFFIC_BROADCAST))
         cli_out (cli, " match traffic-type broadcast %s\n",
                  nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_MULTICAST))
          cli_out (cli, " match traffic-type multicast %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_UNICAST))
          cli_out (cli, " match traffic-type unicast %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_MGMT))
          cli_out (cli, " match traffic-type management %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_ARP))
          cli_out (cli, " match traffic-type arp %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_TCP_DATA))
          cli_out (cli, " match traffic-type tcp-data %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_TCP_CTRL))
          cli_out (cli, " match traffic-type tcp-control %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_UDP))
          cli_out (cli, " match traffic-type udp %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_NON_TCP_UDP))
          cli_out (cli, " match traffic-type non-tcp-udp %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_QUEUE0))
          cli_out (cli, " match traffic-type queue0 %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_QUEUE1))
          cli_out (cli, " match traffic-type queue1 %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_QUEUE2))
          cli_out (cli, " match traffic-type queue2 %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_QUEUE3))
          cli_out (cli, " match traffic-type queue3 %s\n",
                   nsm_qos_ttype_cri_to_str (cmapl->traffic_type.criteria));

       if (CHECK_FLAG (cmapl->traffic_type.custom_traffic_type,
                       NSM_QOS_TRAFFIC_ALL))
          cli_out (cli, " match traffic-type all\n");

    }      

  return 0;
}

int
nsm_qos_print_policy_map (struct cli *cli, struct nsm_pmap_list *pmapl, char *name)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl;
  int i;


  cli_out (cli, "  POLICY-MAP-NAME: %s\n", pmapl->name);
  if (pmapl->attached != 0)
    cli_out (cli, "    State: attached\n");
  else
    cli_out (cli, "    State: detached\n");

  for (i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++ )
    {
      if (pmapl->cl_name[i][0] != 0x00)
        {
          cmapl = nsm_cmap_list_lookup (nm->cmap, pmapl->cl_name[i]);
          if (cmapl)
            {
              cli_out (cli, "\n");
              nsm_qos_print_class_map (cli, cmapl, pmapl->cl_name[i]);
            }
        }
    }
  return 0;
}

/* Priviledged EXEC mode commands */
CLI (show_mls_qos,
     show_mls_qos_cmd,
     "show mls qos",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR)
{
  struct nsm_master *nm = cli->vr->proto;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    cli_out (cli, "  Disable\n");
  else
    cli_out (cli, "  Enable\n");
  return CLI_SUCCESS;
}

/*
 * Show interface state related to QoS such as trust state
 */
CLI (show_mls_qos_interface,
     show_mls_qos_interface_cmd,
     "show mls qos interface IFNAME",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Select an interface",
     "Interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl;
  struct nsm_cmap_list *cmapl;
  struct nsm_qif_list *qifl;
  int i;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (argc != 0)
    {

      if (ifg_lookup_by_name (&nzg->ifg, argv [0]) == NULL)
        {
          cli_out (cli, "%% Interface %s not found \n", argv [0]);
          return CLI_ERROR;
        }

      qifl = nsm_qif_list_lookup (qosg.qif, argv[0]);

      if (qifl)
        {
          if (qifl->input_pmap_name[0] != 0x00)
            {
              pmapl = nsm_pmap_list_lookup (nm->pmap, &qifl->input_pmap_name[0]);
              if (pmapl)
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "  INPUT-POLICY-MAP-NAME: %s\n", pmapl->name);
                  for (i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++)
                    {
                      if (pmapl->cl_name[i][0] != 0x00)
                        {
                          cmapl = nsm_cmap_list_lookup (nm->cmap, pmapl->cl_name[i]);
                          if (cmapl)
                            nsm_qos_print_class_map (cli, cmapl, argv[0]);
                        }
                    }
                }
            }
          if (qifl->output_pmap_name[0] != 0x00)
            {
              pmapl = nsm_pmap_list_lookup (nm->pmap, &qifl->output_pmap_name[0]);
              if (pmapl)
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "  OUTPUT-POLICY-MAP-NAME: %s\n", pmapl->name);
                  for (i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++)
                    {
                      if (pmapl->cl_name[i][0] != 0x00)
                        {
                          cmapl = nsm_cmap_list_lookup (nm->cmap, pmapl->cl_name[i]);
                          if (cmapl)
                            nsm_qos_print_class_map (cli, cmapl, argv[0]);
                        }
                    }
                }
            }

          cli_out (cli, "\n");
          /* Check the trust state of port */
          if (qifl->trust_state[0] != 0x00)
            cli_out (cli, "  Port trust state: %s\n", &qifl->trust_state[0]);

          /* Check the dscp mutation map of port */
          if (qifl->dscp_mut_name[0] != 0x00)
            cli_out (cli, "  Name of DSCP-to-DSCP mutaion map: %s\n", &qifl->dscp_mut_name[0]);

          /* Check the dscp mutation map of port */
          if (qifl->dscp_cos_name[0] != 0x00)
            cli_out (cli, "  Name of DSCP-to-COS map: %s\n", &qifl->dscp_cos_name[0]);

          if (qosg.config_new == PAL_TRUE)
            {

              switch (qifl->trust_type)
                {
                  case  NSM_QOS_TRUST_NONE:
                    cli_out (cli, "  Trust Mode: Ports default priority \n");
                    break;
                  case  NSM_QOS_TRUST_COS:
                    cli_out (cli, "  Trust Mode: Trust the VLAN Tag Priority\n");
                    break;
                  case  NSM_QOS_TRUST_DSCP:
                    cli_out (cli, "  Trust Mode: Trust the IP DSCP Bits\n");
                    break;
                  case  NSM_QOS_TRUST_DSCP_COS:
                    if (qifl->force_trust_cos == PAL_TRUE)
                      cli_out (cli, "  Trust Mode: Trust both COS and DSCP"
                                    " and prefer COS over DSCP\n");
                    else
                      cli_out (cli, "  Trust Mode: Trust both COS and DSCP"
                                    " and prefer DSCP over COS\n");
                    break;
                  default:
                    break;
                }
              
              cli_out (cli, "  Port Default Prioiry: %d\n", qifl->def_cos);

              switch (qifl->port_mode)
                {
                  case NSM_QOS_VLAN_PRI_OVERRIDE_NONE:
                    cli_out (cli, "  VLAN Priority Overide: Not Configured\n");
                    break;
                  case NSM_QOS_VLAN_PRI_OVERRIDE_COS:
                    cli_out (cli, "  VLAN Priority Overide: Override COS\n");
                    break;
                  case NSM_QOS_VLAN_PRI_OVERRIDE_QUEUE:
                    cli_out (cli, "  VLAN Priority Overide: Override queue\n");
                    break;
                    break;
                  case NSM_QOS_VLAN_PRI_OVERRIDE_BOTH:
                    cli_out (cli, "  VLAN Priority Overide: Override COS and queue\n");
                    break;
                    break;
                  default:
                    break;
                }

              if (qifl->egress_rate_unit)
                {
                  if (qifl->egress_rate_unit == NSM_QOS_RATE_KBPS)
                    cli_out (cli, "  Egress Traffic Shaping: %d bps\n",
                             qifl->egress_rate_unit);
                  else
                    cli_out (cli, "  Egress Traffic Shaping: %d fps\n",
                             qifl->egress_rate_unit);
                }
              else
                cli_out (cli, "  Egress Traffic Shaping: Not Configured \n");

              /* Display scheduling scheme, currently it is fixed when QoS is enabled */
              if (qifl->strict_queue == NSM_QOS_STRICT_QUEUE_ALL)
                cli_out (cli, "  Schedule mode: All Strict Queues \n");
              else if (qifl->strict_queue == NSM_QOS_STRICT_QUEUE_NONE)
                cli_out (cli, "  Schedule mode: All weighted round-robin Queues \n");
              else
                {
                  cli_out (cli, "  Schedule mode: Strict Queues ");

                  if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE0))
                    cli_out (cli, "0 ");

                  if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE1))
                    cli_out (cli, "1 ");

                  if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE2))
                    cli_out (cli, "2 ");

                  if (CHECK_FLAG (qifl->strict_queue, NSM_QOS_STRICT_QUEUE3))
                    cli_out (cli, "3 ");

                   cli_out (cli, "\n");
                }

              cli_out (cli, "    The number of COS Values mapped: 8\n");
              cli_out (cli, "    Cos (Queue): 0(%d), 1(%d), 2(%d), 3(%d),"
                            " 4(%d), 5(%d), 6(%d), 7(%d)\n",
                       qosg.cos_queue_map_tbl[0] & NSM_QOS_QUEUE_ID_MASK,
                       qosg.cos_queue_map_tbl[1] & NSM_QOS_QUEUE_ID_MASK,
                       qosg.cos_queue_map_tbl[2] & NSM_QOS_QUEUE_ID_MASK,
                       qosg.cos_queue_map_tbl[3] & NSM_QOS_QUEUE_ID_MASK,
                       qosg.cos_queue_map_tbl[4] & NSM_QOS_QUEUE_ID_MASK,
                       qosg.cos_queue_map_tbl[5] & NSM_QOS_QUEUE_ID_MASK,
                       qosg.cos_queue_map_tbl[6] & NSM_QOS_QUEUE_ID_MASK,
                       qosg.cos_queue_map_tbl[7] & NSM_QOS_QUEUE_ID_MASK);

              for (i = 0; i < 8; i++)
                {
                  cli_out (cli, "    Dscp (Queue): %d(%d), %d(%d), %d(%d), %d(%d),"
                                " %d(%d), %d(%d), %d(%d), %d(%d)\n",
                            i * 8 + 0,
                            (qosg.dscp_queue_map_tbl[i*8 + 0]
                            & NSM_QOS_QUEUE_ID_MASK),
                            i * 8 + 1,
                            (qosg.dscp_queue_map_tbl[i*8 + 1]
                             & NSM_QOS_QUEUE_ID_MASK),
                            i * 8 + 2,
                            (qosg.dscp_queue_map_tbl[i*8 + 2] 
                             & NSM_QOS_QUEUE_ID_MASK),
                            i * 8 + 3,
                            (qosg.dscp_queue_map_tbl[i*8 + 3]
                            & NSM_QOS_QUEUE_ID_MASK),
                            i * 8 + 4,
                            (qosg.dscp_queue_map_tbl[i*8 + 4]
                            & NSM_QOS_QUEUE_ID_MASK),
                            i * 8 + 5,
                            (qosg.dscp_queue_map_tbl[i*8 + 5]
                            & NSM_QOS_QUEUE_ID_MASK),
                            i * 8 + 6,
                            (qosg.dscp_queue_map_tbl[i*8 + 6]
                            & NSM_QOS_QUEUE_ID_MASK),
                            i * 8 + 7,
                            (qosg.dscp_queue_map_tbl[i*8 + 7]
                            & NSM_QOS_QUEUE_ID_MASK));
                }
            }
          else
            {
              /* Display scheduling scheme, currently it is fixed when QoS is enabled */
              if ((qosg.q0[0] == 0) && (qosg.q1[0] == 0) && (qosg.q2[0] == 0) && (qosg.q3[0] == 0) &&
                  (qosg.q4[0] == 0) && (qosg.q5[0] == 0) && (qosg.q6[0] == 0) && (qosg.q7[0] == 0))
                {
                  cli_out (cli, "  Schedule mode: strict\n");
                }
              else
                {
                  cli_out (cli, "  Schedule mode: weighted round-robin\n");
                }  
              cli_out (cli, "    The number of egress queue: 8\n");
              cli_out (cli, "    Weights (priority): %d(%d), %d(%d), %d(%d), "
                            "%d(%d), %d(%d), %d(%d), %d(%d), %d(%d)\n",
                       qosg.q0[0], qosg.q0[1], qosg.q1[0], qosg.q1[1],
                       qosg.q2[0], qosg.q2[1], qosg.q3[0], qosg.q3[1],
                       qosg.q4[0], qosg.q4[1], qosg.q5[0], qosg.q5[1],
                       qosg.q6[0], qosg.q6[1], qosg.q7[0], qosg.q7[1]);
            }
        }
      else
        {
          if (qosg.config_new == PAL_TRUE)
            {
              /* Display scheduling scheme, currently it is fixed when QoS is enabled */
              cli_out (cli, "  Trust Mode: Ports default priority \n");
              cli_out (cli, "  Port Default Prioiry: 0\n");
              cli_out (cli, "  VLAN Priority Overide: Not Configured\n");
              cli_out (cli, "  Egress Traffic Shaping: Not Configured \n");
              cli_out (cli, "  Schedule mode: All weighted round-robin Queues \n");
              cli_out (cli, "    The number of COS Values mapped: 8\n");
              cli_out (cli, "    All COS are mapped to Queue 0 \n");
              cli_out (cli, "    All DSCP are mapped to Queue 0 \n");
            }
        }

    }
  return CLI_SUCCESS;
}

CLI (show_mls_qos_aggregator_policer,
     show_mls_qos_aggregator_policer_cmd,
     "show mls qos aggregator-policer NAME",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Aggregator policer entry",
     "Aggregator policer name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_agp_list *agpl;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  agpl = nsm_agp_list_lookup (qosg.agp, argv[0]);
  if (!agpl)
    {
      cli_out (cli, " %% %s is not configured!\n", argv[0]);
      return CLI_ERROR;
    }

  cli_out (cli, "\n");
  cli_out (cli, " AGGREGATOR-POLICER-NAME: %s\n", argv[0]);

  if (agpl->p.avg != 0)
    {
      cli_out (cli, "    Police:");
      cli_out (cli, "    Average rate(%d kbps), burst size(%d bytes)", agpl->p.avg, agpl->p.burst);
      if (agpl->p.exd_act == NSM_QOS_EXD_ACT_DROP)
        cli_out (cli, "    Exceed-action drop");
      else if (agpl->p.exd_act == NSM_QOS_EXD_ACT_POLICED_DSCP_TX)
        cli_out (cli, "    Exceed-action policed-dscp-transmit");
      cli_out (cli, "\n");
    }
  return CLI_SUCCESS;
}

CLI (show_mls_qos_maps_cos_dscp,
     show_mls_qos_maps_cos_dscp_cmd,
     "show mls qos maps cos-dscp",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Select QoS map",
     "COS-to-DSCP map")
{
  struct nsm_master *nm = cli->vr->proto;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  cli_out (cli, "  COS-TO-DSCP-MAP:\n");
  cli_out (cli, "    COS :   0   1   2   3   4   5   6   7\n");
  cli_out (cli, "    -------------------------------------\n");
  cli_out (cli, "    DSCP:%4d%4d%4d%4d%4d%4d%4d%4d\n",
           work_map_tbls.cos_dscp_map_tbl[0],
           work_map_tbls.cos_dscp_map_tbl[1],
           work_map_tbls.cos_dscp_map_tbl[2],
           work_map_tbls.cos_dscp_map_tbl[3],
           work_map_tbls.cos_dscp_map_tbl[4],
           work_map_tbls.cos_dscp_map_tbl[5],
           work_map_tbls.cos_dscp_map_tbl[6],
           work_map_tbls.cos_dscp_map_tbl[7]);
  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

CLI (show_mls_qos_maps_ip_prec_dscp,
     show_mls_qos_maps_ip_prec_dscp_cmd,
     "show mls qos maps ip-prec-dscp",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Select QoS map",
     "IP-Prec-to-DSCP map")
{
  struct nsm_master *nm = cli->vr->proto;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  cli_out (cli, "  IP-PREC-TO-DSCP-MAP:\n");
  cli_out (cli, "    PREC:   0   1   2   3   4   5   6   7\n");
  cli_out (cli, "    --------------------------------------\n");
  cli_out (cli, "    DSCP:%4d%4d%4d%4d%4d%4d%4d%4d\n",
           work_map_tbls.ip_prec_dscp_map_tbl[0],
           work_map_tbls.ip_prec_dscp_map_tbl[1],
           work_map_tbls.ip_prec_dscp_map_tbl[2],
           work_map_tbls.ip_prec_dscp_map_tbl[3],
           work_map_tbls.ip_prec_dscp_map_tbl[4],
           work_map_tbls.ip_prec_dscp_map_tbl[5],
           work_map_tbls.ip_prec_dscp_map_tbl[6],
           work_map_tbls.ip_prec_dscp_map_tbl[7]);
  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

CLI (show_mls_qos_maps_policed_dscp,
     show_mls_qos_maps_policed_dscp_cmd,
     "show mls qos maps policed-dscp",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Select QoS map",
     "Policed-DSCP map")
{
  struct nsm_master *nm = cli->vr->proto;
  int i;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  cli_out (cli, "  POLICED-DSCP-MAP:\n");
  cli_out (cli, "    d1 :  d2   0   1   2   3   4   5   6   7   8   9\n");
  cli_out (cli, "    ------------------------------------------------\n");

  for ( i=0 ; i < 6 ; i++ )
    {
      cli_out (cli, "  %4d :%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d\n",
               i,
               work_map_tbls.policed_dscp_map_tbl[i*10 + 0],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 1],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 2],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 3],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 4],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 5],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 6],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 7],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 8],
               work_map_tbls.policed_dscp_map_tbl[i*10 + 9]);
    }
  cli_out (cli, "     6 :%4d%4d%4d%4d\n",
           work_map_tbls.policed_dscp_map_tbl[60],
           work_map_tbls.policed_dscp_map_tbl[61],
           work_map_tbls.policed_dscp_map_tbl[62],
           work_map_tbls.policed_dscp_map_tbl[63]);

  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

CLI (show_mls_qos_maps_dscp_cos,
     show_mls_qos_maps_dscp_cos_cmd,
     "show mls qos maps dscp-cos NAME",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Select QoS map",
     "DSCP-to-COS map",
     "DSCP-to-COS map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_dscp_cos_list *dcosl;
  int i;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  dcosl = nsm_dscp_cos_list_lookup (qosg.dcosl, argv[0]);
  if (! dcosl)
    {
      cli_out (cli, "%% Can't find DSCP-to-COS map (name %s)\n", argv[0]);
      return CLI_ERROR;
    }

  cli_out (cli, "  DSCP-TO-COS-MAP:  %s\n", argv[0]);
  cli_out (cli, "    d1 :   d2  0   1   2   3   4   5   6   7   8   9\n");
  cli_out (cli, "    -------------------------------------------------\n");

  for ( i=0 ; i < 6 ; i++ )
    {
      cli_out (cli, "  %4d :    %4d%4d%4d%4d%4d%4d%4d%4d%4d%4d\n",
               i,
               dcosl->d.dscp[i*10 + 0],
               dcosl->d.dscp[i*10 + 1],
               dcosl->d.dscp[i*10 + 2],
               dcosl->d.dscp[i*10 + 3],
               dcosl->d.dscp[i*10 + 4],
               dcosl->d.dscp[i*10 + 5],
               dcosl->d.dscp[i*10 + 6],
               dcosl->d.dscp[i*10 + 7],
               dcosl->d.dscp[i*10 + 8],
               dcosl->d.dscp[i*10 + 9]);
    }
  cli_out (cli, "     6 :    %4d%4d%4d%4d\n",
           dcosl->d.dscp[60],
           dcosl->d.dscp[61],
           dcosl->d.dscp[62],
           dcosl->d.dscp[63]);

  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

#if 0
CLI (show_mls_qos_maps_dscp_cos,
     show_mls_qos_maps_dscp_cos_cmd,
     "show mls qos maps dscp-cos",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Select QoS map",
     "DSCP-to-COS map")
{
  struct nsm_master *nm = cli->vr->proto;
  int i;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  cli_out (cli, "  DSCP-TO-COS-MAP:\n");
  cli_out (cli, "    d1 :  d2   0   1   2   3   4   5   6   7   8   9\n");
  cli_out (cli, "    ------------------------------------------------\n");

  for ( i=0 ; i < 6 ; i++ )
    {
      cli_out (cli, "  %4d :    %4d%4d%4d%4d%4d%4d%4d%4d%4d%4d\n",
               i,
               work_map_tbls.dscp_cos_map_tbl[i*10 + 0],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 1],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 2],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 3],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 4],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 5],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 6],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 7],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 8],
               work_map_tbls.dscp_cos_map_tbl[i*10 + 9]);
    }
  cli_out (cli, "     6 :    %4d%4d%4d%4d\n",
           work_map_tbls.dscp_cos_map_tbl[60],
           work_map_tbls.dscp_cos_map_tbl[61],
           work_map_tbls.dscp_cos_map_tbl[62],
           work_map_tbls.dscp_cos_map_tbl[63]);

  cli_out (cli, "\n");
  return CLI_SUCCESS;
}
#endif /* 0 */

CLI (show_mls_qos_maps_dscp_mutation,
     show_mls_qos_maps_dscp_mutation_cmd,
     "show mls qos maps dscp-mutation NAME",
     CLI_SHOW_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Select QoS map",
     "DSCP mutation map",
     "DSCP mutation map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_dscp_mut_list *dscpml;
  int i;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  dscpml = nsm_dscp_mut_list_lookup (qosg.dscpml, argv[0]);
  if (! dscpml)
    {
      cli_out (cli, "%% Can't find a dscp-mutation map (name %s)\n", argv[0]);
      return CLI_ERROR;
    }

  cli_out (cli, "  DSCP-TO-DSCP-MUTATION-MAP:  %s\n", argv[0]);
  cli_out (cli, "    d1 :   d2  0   1   2   3   4   5   6   7   8   9\n");
  cli_out (cli, "    -------------------------------------------------\n");

  for ( i=0 ; i < 6 ; i++ )
    {
      cli_out (cli, "  %4d :    %4d%4d%4d%4d%4d%4d%4d%4d%4d%4d\n",
               i,
               dscpml->d.dscp[i*10 + 0],
               dscpml->d.dscp[i*10 + 1],
               dscpml->d.dscp[i*10 + 2],
               dscpml->d.dscp[i*10 + 3],
               dscpml->d.dscp[i*10 + 4],
               dscpml->d.dscp[i*10 + 5],
               dscpml->d.dscp[i*10 + 6],
               dscpml->d.dscp[i*10 + 7],
               dscpml->d.dscp[i*10 + 8],
               dscpml->d.dscp[i*10 + 9]);
    }
  cli_out (cli, "     6 :    %4d%4d%4d%4d\n",
           dscpml->d.dscp[60],
           dscpml->d.dscp[61],
           dscpml->d.dscp[62],
           dscpml->d.dscp[63]);

  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

CLI (show_qos_access_list_name,
     show_qos_access_list_name_cmd,
     "show qos-access-list (<1-99>|<100-199>|<1300-1999>|<2000-2699>|WORD)",
     CLI_SHOW_STR,
     "List QoS (MAC & IP) access lists",
     "Standard access list",
     "Extended access list",
     "Standard access list (expanded range)",
     "Extended access list (expanded range)",
     "QoS access-list name")
{
  struct nsm_master *nm = cli->vr->proto;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  qos_filter_show (cli, argv[0]);
  return CLI_SUCCESS;
}


CLI (show_qos_access_list,
     show_qos_access_list_cmd,
     "show qos-access-list",
     CLI_SHOW_STR,
     "List QoS (MAC & IP) access lists")
{
  struct nsm_master *nm = cli->vr->proto;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  qos_all_filter_show (cli);
  return CLI_SUCCESS;
}


CLI (show_class_map_name,
     show_class_map_name_cmd,
     "show class-map NAME",
     CLI_SHOW_STR,
     "Class map entry",
     "Specify class map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Lookup list by using name */
  if (argc != 0)
    {
      cmapl = nsm_cmap_list_lookup (nm->cmap, argv[0]);
      if (cmapl)
        {
          cli_out (cli, "\n");
          nsm_qos_print_class_map (cli, cmapl, argv[0]);
        }
      else
        {
          cli_out (cli, "%% Can't find class-map(%s)\n", argv[0]);
          return CLI_ERROR;
        }
    }
  return CLI_SUCCESS;
}

CLI (show_class_map,
     show_class_map_cmd,
     "show class-map",
     CLI_SHOW_STR,
     "Class map entry")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_qos_cmap_master *cmap_master = nm->cmap;
  struct nsm_cmap_list *cmapl, *cmapl_next;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  cli_out (cli, "\n");
  for (cmapl = cmap_master->head; cmapl; cmapl = cmapl_next)
    {
      cmapl_next = cmapl->next;
      if (cmapl)
        if (cmapl->name)
          {
            nsm_qos_print_class_map (cli, cmapl, argv[0]);
            cli_out (cli, "\n");
          }
    }
  return CLI_SUCCESS;
}

CLI (show_policy_map_name,
     show_policy_map_name_cmd,
     "show policy-map NAME",
     CLI_SHOW_STR,
     "Policy map entry",
     "Specify policy map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Lookup list by using name */
  if (argv[0])
    {
      pmapl = nsm_pmap_list_lookup (nm->pmap, argv[0]);
      if (pmapl)
        {
          cli_out (cli, "\n");
          nsm_qos_print_policy_map (cli, pmapl, argv[0]);
        }
      else
        {
          cli_out (cli, "%% No policy-map list\n");
          return CLI_ERROR;
        }
    }
  return CLI_SUCCESS;
}


CLI (show_policy_map,
     show_policy_map_cmd,
     "show policy-map",
     CLI_SHOW_STR,
     "Policy map entry")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_qos_pmap_master *pmap_master = nm->pmap;
  struct nsm_pmap_list *pmapl, *pmapl_next;


  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  cli_out (cli, "\n");
  for (pmapl = pmap_master->head; pmapl; pmapl = pmapl_next)
    {
      pmapl_next = pmapl->next;
      if (pmapl)
        if (pmapl->name)
          {
            nsm_qos_print_policy_map (cli, pmapl, argv[0]);
            cli_out (cli, "\n");
          }
    }
  return CLI_SUCCESS;
}

int
nsm_qos_init_maps()
{
  int i;

  /* Initialize default-maps and working-maps */
  for (i=0 ; i < COS_TBL_SIZE ; i++ )
    {
      /* Default map */
      default_map_tbls.cos_dscp_map_tbl[i] = i*COS_TBL_SIZE;
      default_map_tbls.pri_cos_map_tbl[i] = i;
      default_map_tbls.ip_prec_dscp_map_tbl[i] = i*COS_TBL_SIZE;

      /* Working map */
      work_map_tbls.cos_dscp_map_tbl[i] = i*COS_TBL_SIZE;
      work_map_tbls.pri_cos_map_tbl[i] = i;
      work_map_tbls.ip_prec_dscp_map_tbl[i] = i*COS_TBL_SIZE;
    }

  for (i=0 ; i < DSCP_TBL_SIZE ; i++)
    {
      /* Default map */
      default_map_tbls.dscp_cos_map_tbl[i] = i/8;
      default_map_tbls.dscp_pri_map_tbl[i] = i/16;
      default_map_tbls.policed_dscp_map_tbl[i] = i;
      default_map_tbls.default_dscp_mut_map_tbl[i] = i;

      /* Working map */
      work_map_tbls.dscp_cos_map_tbl[i]  = i/8;
      work_map_tbls.dscp_pri_map_tbl[i] = i/16;
      work_map_tbls.policed_dscp_map_tbl[i] = i;
    }

  return CLI_SUCCESS;
}


/* CONFIG mode commands */
CLI (mls_qos_enable_global,
     mls_qos_enable_global_cmd,
     "mls qos <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7> <0-32> <0-7>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify weight for queue 0, 0 means Strict Priority",
     "Specify priority for queue 0",
     "Specify weight for queue 1, 0 means Strict Priority",
     "Specify priority for queue 1",
     "Specify weight for queue 2, 0 means Strict Priority",
     "Specify priority for queue 2",
     "Specify weight for queue 3, 0 means Strict Priority",
     "Specify priority for queue 3",
     "Specify weight for queue 4, 0 means Strict Priority",
     "Specify priority for queue 4",
     "Specify weight for queue 5, 0 means Strict Priority",
     "Specify priority for queue 5",
     "Specify weight for queue 6, 0 means Strict Priority",
     "Specify priority for queue 6",
     "Specify weight for queue 7, 0 means Strict Priority",
     "Specify priority for queue 7")
{
  struct nsm_master *nm = cli->vr->proto;
  int iq0[2], iq1[2], iq2[2], iq3[2], iq4[2], iq5[2], iq6[2], iq7[2];
  char q0[2], q1[2], q2[2], q3[2], q4[2], q5[2], q6[2], q7[2];
  int i;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check if QoS state */
  if (qosg.state == NSM_QOS_ENABLED_STATE)
    {
      cli_out (cli, "%% QoS is already enabled\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("q0", iq0[0], argv[0], 0, 32);
  CLI_GET_INTEGER_RANGE ("q0", iq0[1], argv[1], 0, 7);

  CLI_GET_INTEGER_RANGE ("q1", iq1[0], argv[2], 0, 32);
  CLI_GET_INTEGER_RANGE ("q1", iq1[1], argv[3], 0, 7);

  CLI_GET_INTEGER_RANGE ("q2", iq2[0], argv[4], 0, 32);
  CLI_GET_INTEGER_RANGE ("q2", iq2[1], argv[5], 0, 7);

  CLI_GET_INTEGER_RANGE ("q3", iq3[0], argv[6], 0, 32);
  CLI_GET_INTEGER_RANGE ("q3", iq3[1], argv[7], 0, 7);

  CLI_GET_INTEGER_RANGE ("q4", iq4[0], argv[8], 0, 32);
  CLI_GET_INTEGER_RANGE ("q4", iq4[1], argv[9], 0, 7);

  CLI_GET_INTEGER_RANGE ("q5", iq5[0], argv[10], 0, 32);
  CLI_GET_INTEGER_RANGE ("q5", iq5[1], argv[11], 0, 7);

  CLI_GET_INTEGER_RANGE ("q6", iq6[0], argv[12], 0, 32);
  CLI_GET_INTEGER_RANGE ("q6", iq6[1], argv[13], 0, 7);

  CLI_GET_INTEGER_RANGE ("q7", iq7[0], argv[14], 0, 32);
  CLI_GET_INTEGER_RANGE ("q7", iq7[1], argv[15], 0, 7);

  /* Create all QoS masters */
  nsm_acl_list_master_init(nm);
  nsm_cmap_list_master_init(nm);
  nsm_pmap_list_master_init(nm);

  /* Create dscp mutation map list */
  nsm_dscp_mut_list_master_init (&qosg);

  /* Create dscp-cos map list */
  nsm_dscp_cos_list_master_init (&qosg);

  /* Create aggregate-policer list */
  nsm_agp_list_master_init (&qosg);

  /* Create QoS interface list */
  nsm_qif_list_master_init (&qosg);

  /* Initialize all maps */
  nsm_qos_init_maps();

  for ( i = 0 ; i < 2 ; i ++)
    {
      q0[i] = (char) iq0[i];
      q1[i] = (char) iq1[i];
      q2[i] = (char) iq2[i];
      q3[i] = (char) iq3[i];
      q4[i] = (char) iq4[i];
      q5[i] = (char) iq5[i];
      q6[i] = (char) iq6[i];
      q7[i] = (char) iq7[i];
    }

#ifdef HAVE_HAL
  /* Enable QoS */


   qosg.state = NSM_QOS_ENABLED_STATE;

  ret = hal_qos_enable(q0, q1, q2, q3, q4, q5, q6, q7);
  /*
  if (ret < 0)
    {
      nsm_qos_deinit( nm );
      cli_out (cli, "%% Failed to enable QoS globally\n");
      return CLI_ERROR; 
    }   */ /* wangjian@150108, enable qos */
    
#endif /* HAVE_HAL */

  for ( i = 0 ; i < 2 ; i ++)
    {
      qosg.q0[i] = (char) q0[i];
      qosg.q1[i] = (char) q1[i];
      qosg.q2[i] = (char) q2[i];
      qosg.q3[i] = (char) q3[i];
      qosg.q4[i] = (char) q4[i];
      qosg.q5[i] = (char) q5[i];
      qosg.q6[i] = (char) q6[i];
      qosg.q7[i] = (char) q7[i];
    }

  /* Set enabled-state */
  qosg.state = NSM_QOS_ENABLED_STATE;

#ifdef HAVE_HA
  nsm_qosg_data_cal_create (&qosg);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}


CLI (mls_qos_enable_global_new,
     mls_qos_enable_global_new_cmd,
     "mls qos enable",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Enable QoS globally\n")
{
  struct nsm_master *nm = cli->vr->proto;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check if QoS state */
  if (qosg.state == NSM_QOS_ENABLED_STATE)
    {
      cli_out (cli, "%% QoS is already enabled\n");
      return CLI_ERROR;
    }

  /* Create all QoS masters */
  nsm_acl_list_master_init(nm);
  nsm_cmap_list_master_init(nm);
  nsm_pmap_list_master_init(nm);

  /* Create dscp mutation map list */
  nsm_dscp_mut_list_master_init (&qosg);

  /* Create dscp-cos map list */
  nsm_dscp_cos_list_master_init (&qosg);

  /* Create aggregate-policer list */
  nsm_agp_list_master_init (&qosg);

  /* Create QoS interface list */
  nsm_qif_list_master_init (&qosg);

  /* Initialize all maps */
  nsm_qos_init_maps();

  nsm_qos_queue_weight_init (&qosg);

  nsm_qos_dscp_to_queue_init (&qosg);

  nsm_qos_cos_to_queue_init (&qosg);

  nsm_qos_set_frame_type_priority_override_init (&qosg);

#ifdef HAVE_HAL
  /* Enable QoS */
  ret = hal_qos_enable_new();

  /*
  if (ret < 0)
    {
      nsm_qos_deinit( nm );
      cli_out (cli, "%% Failed to enable QoS globally\n");
      return CLI_ERROR;
    }
   */ /*wangjian@150108, enable qos*/
#endif /* HAVE_HAL */

  /* Set enabled-state */
  qosg.state = NSM_QOS_ENABLED_STATE;
  qosg.config_new = 1;

#ifdef HAVE_HA
  nsm_qosg_data_cal_create (&qosg);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}


CLI (no_mls_qos_enable_global,
     no_mls_qos_enable_global_cmd,
     "no mls qos",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR)
{
  struct nsm_master *nm = cli->vr->proto;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */


  /* Check if QoS is a enabled-state */
  if (qosg.state != NSM_QOS_ENABLED_STATE)
    {
      cli_out (cli, "%% QoS is not a enabled-state!\n");
      return CLI_ERROR;
    }

#ifdef HAVE_HAL
  /* Detach CoS queue from switch */
  ret = hal_qos_disable ();
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to disable QoS globally\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  /* Delete all QoS masters */
  nsm_acl_list_master_deinit(nm);
  nsm_cmap_list_master_deinit(nm);
  nsm_pmap_list_master_deinit(nm);

  /* Delete dscp mutation map list */
  nsm_dscp_mut_list_master_deinit (&qosg);

  /* Delete aggregate-police list */
  nsm_agp_list_master_deinit (&qosg);

  /* Delete QoS interface list */
  nsm_qif_list_master_deinit (&qosg);

  /* Set disabled-state */
  qosg.state = NSM_QOS_DISABLED_STATE;

#ifdef HAVE_HA
  nsm_qosg_data_cal_delete (&qosg);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}



#define MAC_ACCESS_LIST_STR                     \
  "QOS's MAC access list",                      \
    "extended mac ACL <2000-2699>"

#define MAC_ACCESS_LIST_DENY_PERMIT_STR         \
  "Specify packets to reject",                  \
    "Specify packets to permit"

#define CLI_GET_QOS_FILTER_TYPE1(T,STR)         \
  do {                                          \
    /* Check of mac filter type. */             \
    if (pal_strncmp ((STR), "s", 1) == 0)       \
      (T) = QOS_FILTER_SOURCE;                  \
    else if (pal_strncmp ((STR), "d", 1) == 0)  \
      (T) = QOS_FILTER_DESTINATION;             \
    else                                        \
      return LIB_API_SET_ERR_INVALID_VALUE;     \
  } while (0)

#define CLI_GET_QOS_FILTER_TYPE(T,STR)          \
  do {                                          \
    /* Check of mac filter type. */             \
    if (pal_strncmp ((STR), "p", 1) == 0)       \
      (T) = QOS_FILTER_PERMIT;                  \
    else if (pal_strncmp ((STR), "d", 1) == 0)  \
      (T) = QOS_FILTER_DENY;                    \
    else                                        \
      return LIB_API_SET_ERR_INVALID_VALUE;     \
  } while (0)

/* MAC QoS extended access list */
CLI (mac_access_list_host_host,
     mac_access_list_host_host_cmd,
     "mac-access-list <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     MAC_ACCESS_LIST_STR,
     MAC_ACCESS_LIST_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[6], 1, 8);
  if (! ((packet_format == NSM_QOS_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_QOS_FILTER_FMT_802_3)   ||
         (packet_format == NSM_QOS_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_QOS_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying MAC acl\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_set(nm->acl, argv[0], type,
                                     argv[2], argv[3], argv[4], argv[5],
                                     acl_type, packet_format);

  return lib_vty_return (cli, ret);
}


CLI (mac_access_list_host_any,
     mac_access_list_host_any_cmd,
     "mac-access-list <2000-2699> (deny|permit) MAC MASK any <1-8>",
     MAC_ACCESS_LIST_STR,
     MAC_ACCESS_LIST_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNAP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_QOS_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_QOS_FILTER_FMT_802_3)   ||
         (packet_format == NSM_QOS_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_QOS_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }
  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying MAC acl\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_set(nm->acl, argv[0], type,
                                     argv[2], argv[3], "0.0.0", "FFFF.FFFF.FFFF",
                                     acl_type, packet_format);

  return lib_vty_return (cli, ret);
}


CLI (mac_access_list_any_host,
     mac_access_list_any_host_cmd,
     "mac-access-list <2000-2699> (deny|permit) any MAC MASK <1-8>",
     MAC_ACCESS_LIST_STR,
     MAC_ACCESS_LIST_DENY_PERMIT_STR,
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_QOS_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_QOS_FILTER_FMT_802_3)   ||
         (packet_format == NSM_QOS_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_QOS_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying MAC acl\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_set(nm->acl, argv[0], type,
                                     "0.0.0", "FFFF.FFFF.FFFF", argv[2], argv[3],
                                     acl_type, packet_format);

  return lib_vty_return (cli, ret);
}

CLI (mac_access_list_address,
     mac_access_list_address_cmd,
     "mac-access-list <2000-2699> (source|destination) MAC priority <0-7>",
     MAC_ACCESS_LIST_STR,
     "Specify packets with source MAC address",
     "Specify packets with destination MAC address",
     "MAC address in HHHH.HHHH.HHHH format",     
     "Priority Class",
     "Priority Value")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  int type = QOS_FILTER_SOURCE;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format = NSM_QOS_FILTER_FMT_ETH_II;
  int priority;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE1 (type, argv[1]);

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }
  
  /* Check the range */
  CLI_GET_INTEGER_RANGE ("Priority Value", priority, argv[3], 0, 7);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, 
                                                    argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface"
                    " before creating/modifying MAC acl\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_hw_set (nm->acl, argv[0], type,
                                         argv[2], "FFFF.FFFF.FFFF", 
                                         "0.0.0", "FFFF.FFFF.FFFF",
                                         acl_type, packet_format, priority);

  return lib_vty_return (cli, ret);
}


CLI (no_mac_access_list_host_host,
     no_mac_access_list_host_host_cmd,
     "no mac-access-list <2000-2699> (deny|permit) MAC MASK MAC MASK <1-8>",
     CLI_NO_STR,
     MAC_ACCESS_LIST_STR,
     MAC_ACCESS_LIST_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[6], 1, 8);

  if (! ((packet_format == NSM_QOS_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_QOS_FILTER_FMT_802_3)   ||
         (packet_format == NSM_QOS_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_QOS_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach mac acl from class map\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_unset(nm->acl, argv[0], type,
                                       argv[2], argv[3], argv[4], argv[5],
                                       acl_type, packet_format);

  return lib_vty_return (cli, ret);
}


CLI (no_mac_access_list_host_any,
     no_mac_access_list_host_any_cmd,
     "no mac-access-list <2000-2699> (deny|permit) MAC MASK any <1-8>",
     CLI_NO_STR,
     MAC_ACCESS_LIST_STR,
     MAC_ACCESS_LIST_DENY_PERMIT_STR,
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Source wildcard in HHHH.HHHH.HHHH format",
     "Destination any",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_QOS_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_QOS_FILTER_FMT_802_3)   ||
         (packet_format == NSM_QOS_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_QOS_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach mac acl from class map\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_unset(nm->acl, argv[0], type,
                                       argv[2], argv[3], "0.0.0", "FFFF.FFFF.FFFF",
                                       acl_type, packet_format);

  return lib_vty_return (cli, ret);
}

CLI (no_mac_access_list_address,
     no_mac_access_list_address_cmd,
     "no mac-access-list <2000-2699> (source|destination) MAC priority <0-7>",
     CLI_NO_STR,
     MAC_ACCESS_LIST_STR,
     "Specify packets with source MAC address",
     "Specify packets with destination MAC address",
     "Source host's MAC address in HHHH.HHHH.HHHH format",
     "Priority Class",
     "Priority Value")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  int type = QOS_FILTER_SOURCE;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format = NSM_QOS_FILTER_FMT_ETH_II;
  int priority;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE1 (type, argv[1]);

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("Priority Value", priority, argv[3], 0, 7);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, 
                                          nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach mac acl from class map\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_hw_unset (nm->acl, argv[0], type,
                                       argv[2], "FFFF.FFFF.FFFF", 
                                       "0.0.0", "FFFF.FFFF.FFFF",
                                       acl_type, packet_format, priority);

  return lib_vty_return (cli, ret);
}

CLI (no_mac_access_list_any_host,
     no_mac_access_list_any_host_cmd,
     "no mac-access-list <2000-2699> (deny|permit) any MAC MASK <1-8>",
     CLI_NO_STR,
     MAC_ACCESS_LIST_STR,
     MAC_ACCESS_LIST_DENY_PERMIT_STR,
     "Source any",
     "Destination host's MAC address in HHHH.HHHH.HHHH format",
     "Destination wildcard in HHHH.HHHH.HHHH format",
     "Specify packet format, (1:Ethernet II/ 2:802.3/ 4:SNMP/ 8:LLC)")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_MAC;
  int packet_format;
  int ret;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("ether type", packet_format, argv[4], 1, 8);

  if (! ((packet_format == NSM_QOS_FILTER_FMT_ETH_II)  ||
         (packet_format == NSM_QOS_FILTER_FMT_802_3)   ||
         (packet_format == NSM_QOS_FILTER_FMT_SNAP)    ||
         (packet_format == NSM_QOS_FILTER_FMT_LLC)))
    {
      cli_out (cli, "%% The packet format is wrong\n");
      return CLI_ERROR;
    }

  if (nm->cmap == NULL || argv[0] == NULL || nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach mac acl from class map\n");
      return CLI_ERROR;
    }

  ret = mac_access_list_extended_unset(nm->acl, argv[0], type,
                                       "0.0.0", "FFFF.FFFF.FFFF", argv[2], argv[3],
                                       acl_type, packet_format);
  return lib_vty_return (cli, ret);
}


#define CLI_IP_ACCESS_STR       "QoS's IP access list"

#define IP_STANDARD_ACCESS_LIST_HELP_STR        \
  "IP standard access list",                    \
    "IP standard access list (expanded range)", \
    "Specify packets to reject",                \
    "Specify pactets to permit"

#define IP_EXTENDED_ACCESS_LIST_HELP_STR        \
  "IP extended access list",                    \
    "IP extended access list (expanded range)", \
    "Specify packets to reject",                \
    "Specify pactets to permit"

/* IP QoS standard access-list */
CLI (ip_access_list_standard,
     ip_access_list_standard_cmd,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match",
     "Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }
  ret = ip_access_list_standard_set (nm->acl, argv[0], type,
                                     argv[2], argv[3],
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_standard_nomask,
     ip_access_list_standard_nomask_cmd,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }
  ret = ip_access_list_standard_set (nm->acl, argv[0], type,
                                     argv[2], "0.0.0.0",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

ALI (ip_access_list_standard_nomask,
     ip_access_list_standard_host_cmd,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) host A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "A single host address",
     "Address to match");

CLI (ip_access_list_standard_any,
     ip_access_list_standard_any_cmd,
     "ip-access-list (<1-99>|<1300-1999>) (deny|permit) any",
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "Any source host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }
  ret = ip_access_list_standard_set (nm->acl, argv[0], type,
                                     "0.0.0.0", "255.255.255.255",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_standard,
     no_ip_access_list_standard_cmd,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match",
     "Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_standard_unset (nm->acl, argv[0], type,
                                       argv[2], argv[3],
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_standard_nomask,
     no_ip_access_list_standard_nomask_cmd,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_standard_unset (nm->acl, argv[0], type,
                                       argv[2], "0.0.0.0",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

ALI (no_ip_access_list_standard_nomask,
     no_ip_access_list_standard_host_cmd,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) host A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "A single host address",
     "Address to match");

CLI (no_ip_access_list_standard_any,
     no_ip_access_list_standard_any_cmd,
     "no ip-access-list (<1-99>|<1300-1999>) (deny|permit) any",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_STANDARD_ACCESS_LIST_HELP_STR,
     "Any source host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_standard_unset (nm->acl, argv[0], type,
                                       "0.0.0.0", "255.255.255.255",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

/* Extended ip-access-list */
CLI (ip_access_list_extended,
     ip_access_list_extended_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Destination address",
     "Destination Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     argv[2], argv[3], argv[4], argv[5],
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_mask_any,
     ip_access_list_extended_mask_any_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D any",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Any destination host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  
  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     argv[2], argv[3], "0.0.0.0", "255.255.255.255",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_any_mask,
     ip_access_list_extended_any_mask_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any A.B.C.D A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Destination address",
     "Destination Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     "0.0.0.0", "255.255.255.255", argv[2], argv[3],
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_any_any,
     ip_access_list_extended_any_any_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any any",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Any destination host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     "0.0.0.0", "255.255.255.255", "0.0.0.0", "255.255.255.255",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_mask_host,
     ip_access_list_extended_mask_host_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D host A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "A single destination host",
     "Destination address")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     argv[2], argv[3], argv[4], "0.0.0.0",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_host_mask,
     ip_access_list_extended_host_mask_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D A.B.C.D A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Destination address",
     "Destination Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     argv[2], "0.0.0.0", argv[3], argv[4],
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_host_host,
     ip_access_list_extended_host_host_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D host A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "A single destination host",
     "Destination address")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     argv[2], "0.0.0.0", argv[3], "0.0.0.0",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_any_host,
     ip_access_list_extended_any_host_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any host A.B.C.D",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "A single destination host",
     "Destination address")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  
  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     "0.0.0.0", "255.255.255.255", argv[2], "0.0.0.0",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (ip_access_list_extended_host_any,
     ip_access_list_extended_host_any_cmd,
     "ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D any",
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Any destination host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_TRUE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach policy-map from interface before creating/modifying IP acl\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_set (nm->acl, argv[0], type,
                                     argv[2], "0.0.0.0", "0.0.0.0", "255.255.255.255",
                                     acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended,
     no_ip_access_list_extended_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Destination address",
     "Destination Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       argv[2], argv[3], argv[4], argv[5],
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended_mask_any,
     no_ip_access_list_extended_mask_any_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D any",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Any destination host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       argv[2], argv[3], "0.0.0.0", "255.255.255.255",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}


CLI (no_ip_access_list_extended_any_mask,
     no_ip_access_list_extended_any_mask_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Destination address",
     "Destination Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  
  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       "0.0.0.0", "255.255.255.255", argv[2], argv[3],
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended_any_any,
     no_ip_access_list_extended_any_any_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any any",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Any destination host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       "0.0.0.0", "255.255.255.255",
                                       "0.0.0.0", "255.255.255.255",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended_mask_host,
     no_ip_access_list_extended_mask_host_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D host A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "A single destination host",
     "Destination address")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       argv[2], argv[3], argv[4], "0.0.0.0",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended_host_mask,
     no_ip_access_list_extended_host_mask_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Destination address",
     "Destination Wildcard bits")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       argv[2], "0.0.0.0", argv[3], argv[4],
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended_host_host,
     no_ip_access_list_extended_host_host_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D host A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "A single destination host",
     "Destination address")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       argv[2], "0.0.0.0", argv[3], "0.0.0.0",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended_any_host,
     no_ip_access_list_extended_any_host_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip any host A.B.C.D",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "A single destination host",
     "Destination address")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       "0.0.0.0", "255.255.255.255", argv[2], "0.0.0.0",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}

CLI (no_ip_access_list_extended_host_any,
     no_ip_access_list_extended_host_any_cmd,
     "no ip-access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D any",
     CLI_NO_STR,
     CLI_IP_ACCESS_STR,
     IP_EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Any destination host")
{
  struct nsm_master *nm = cli->vr->proto;
  int attached;
  enum filter_type type;
  int acl_type = NSM_QOS_ACL_TYPE_IP;
  int ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_QOS_FILTER_TYPE (type, argv[1]);

  /* Check if policy-map is attached */
  attached = nsm_qos_check_macl_from_cmap_and_pmap (nm->cmap, nm->pmap, argv[0], NSM_FALSE);
  if (attached)
    {
      cli_out (cli, "%% Need to detach IP acl from class map\n");
      return CLI_ERROR;
    }

  ret = ip_access_list_extended_unset (nm->acl, argv[0], type,
                                       argv[2], "0.0.0.0", "0.0.0.0", "255.255.255.255",
                                       acl_type, 0);

  return lib_vty_return (cli, ret);
}


CLI (qos_class_map,
     qos_class_map_cmd,
     "class-map NAME",
     "Class map command",
     "Specify a class-map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl = NULL;
  struct nsm_cmap_list *cmapl = NULL;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (nm->cmap == NULL || argv[0] == NULL)
    {
      cli_out (cli, "%% No class-map master address %d or MAC acl name %s\n", nm->cmap, argv[0]);
      return CLI_ERROR;
    }

  if (nm->pmap == NULL)
    {
      cli_out (cli, "%% No policy-map master address %d\n", nm->pmap);
      return CLI_ERROR;
    }

  /* Check -
   * if the policy-map, which contains this class-map, exist
   * and also it is attached to an interface
   */
  pmapl = nsm_qos_check_class_from_pmap (nm->pmap, argv[0], NSM_TRUE);
  if (pmapl)
    {
      cli_out (cli, "%% Policy-map should be detached from the interface before changing class-map\n");
      return CLI_ERROR;
    }

  /* Check the class-map list */
  cmapl = nsm_cmap_list_get (nm->cmap, argv[0]);
  if (! cmapl)
    {
      cli_out (cli, "%% Fail to get class-map list \n");
      return CLI_ERROR;
    }

  cli->index = cmapl;
  cli->mode = CMAP_MODE;

  return CLI_SUCCESS;
}


CLI (no_qos_class_map,
     no_qos_class_map_cmd,
     "no class-map NAME",
     CLI_NO_STR,
     "Class map command",
     "Specify a class-map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl;
  struct nsm_cmap_list *cmapl;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check -
   * if the policy-map, which contains this class-map, exist
   * and also it is attached to an interface
   */
  cmapl = nsm_cmap_list_lookup (nm->cmap, argv[0]);
  if (! cmapl)
    {
      cli_out (cli, "%% Can't find class-map (%s)\n", argv[0]);
      return CLI_ERROR;
    }

  pmapl = nsm_qos_check_class_from_pmap (nm->pmap, argv[0], NSM_FALSE);
  if (pmapl)
    {
      cli_out (cli, "%% Class-map %s attached to policy map %s\n", argv[0], pmapl->name);
      return CLI_ERROR;
    }

  nsm_cmap_list_delete (cmapl);

  return CLI_SUCCESS;
}


CLI (qos_policy_map,
     qos_policy_map_cmd,
     "policy-map NAME",
     "Policy map command",
     "Specify a policy-map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl = NULL;

  if (! nm)
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (nm->pmap == NULL)
    {
      cli_out (cli, "%% address error\n");
      return CLI_ERROR;
    }

  pmapl = nsm_pmap_list_get (nm->pmap, argv[0]);
  if ( !pmapl )
    {
      cli_out (cli, "%% Fail to get policy-map list (%s)\n", argv[0]);
      return CLI_ERROR;
    }
  
  if (pmapl->attached > 0)
    {
      cli_out (cli, "%% Can't update policy-map(%s) because it is attached to some interfaces\n", argv[0]);
      return CLI_ERROR;
    }

  cli->index = pmapl;
  cli->mode = PMAP_MODE;

  return CLI_SUCCESS;
}


CLI (no_qos_policy_map,
     no_qos_policy_map_cmd,
     "no policy-map NAME",
     CLI_NO_STR,
     "Policy map command",
     "Specify a policy-map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  pmapl = nsm_pmap_list_lookup (nm->pmap, argv[0]);
  if (! pmapl)
    {
      cli_out (cli, "%% Can't find policy-map list (%s)\n", argv[0]);
      return CLI_ERROR;
    }

  if (pmapl->attached > 0)
    {
      cli_out (cli, "%% Can't delete policy-map(%s) because it is attached to some interfaces\n", argv[0]);
      return CLI_ERROR;
    }
  nsm_pmap_list_delete (pmapl);

  return CLI_SUCCESS;
}


CLI (mls_qos_map_cos_dscp,
     mls_qos_map_cos_dscp_cmd,
     "mls qos map cos-dscp <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the CoS-to-DSCP map",
     "DSCP value, mapping to CoS 0",
     "DSCP value, mapping to CoS 1",
     "DSCP value, mapping to CoS 2",
     "DSCP value, mapping to CoS 3",
     "DSCP value, mapping to CoS 4",
     "DSCP value, mapping to CoS 5",
     "DSCP value, mapping to CoS 6",
     "DSCP value, mapping to CoS 7")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t dscp_val[COS_TBL_SIZE];
  int i;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* PORT_DSCP_PRI table will be used */
  /* Check the number of argc */
  if (argc != COS_TBL_SIZE)
    {
      cli_out (cli, "%% The number of CoS not enough\n");
      return CLI_ERROR;
    }

  /* Check the range of DSCP vlaue */
  for ( i=0 ; i < COS_TBL_SIZE ; i++ )
    CLI_GET_UINT32_RANGE ("dscp vlaue", dscp_val[i], argv[i], 0, 63);

  /* Set new cos-dscp tble */
  for ( i=0 ; i < COS_TBL_SIZE ; i++ )
    work_map_tbls.cos_dscp_map_tbl[i] = (u_int8_t)dscp_val[i];

  return CLI_SUCCESS;
}


CLI (no_mls_qos_map_cos_dscp,
     no_mls_qos_map_cos_dscp_cmd,
     "no mls qos map cos-dscp",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the CoS-to_DSCP map")
{
  struct nsm_master *nm = cli->vr->proto;
  int i;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Set default cos-dscp value to working cos-dscp table */
  for ( i=0 ; i < COS_TBL_SIZE ; i++ )
    work_map_tbls.cos_dscp_map_tbl[i] = default_map_tbls.cos_dscp_map_tbl[i];

  return CLI_SUCCESS;
}


CLI (mls_qos_map_ip_prec_dscp,
     mls_qos_map_ip_prec_dscp_cmd,
     "mls qos map ip-prec-dscp <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the IP-PREC-to-DSCP map",
     "DSCP value, mapping to IP Precedence 0",
     "DSCP value, mapping to IP Precedence 1",
     "DSCP value, mapping to IP Precedence 2",
     "DSCP value, mapping to IP Precedence 3",
     "DSCP value, mapping to IP Precedence 4",
     "DSCP value, mapping to IP Precedence 5",
     "DSCP value, mapping to IP Precedence 6",
     "DSCP value, mapping to IP Precedence 7")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t dscp_val[8];
  int i;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* PORT_DSCP_PRI table will be used */
  /* Check the number of argc */
  if (argc != COS_TBL_SIZE)
    {
      cli_out (cli, "%% The number of CoS not enough\n");
      return CLI_ERROR;
    }

  /* Check the range of DSCP vlaue */
  for ( i=0 ; i < COS_TBL_SIZE ; i++ )
    CLI_GET_UINT32_RANGE ("dscp vlaue", dscp_val[i], argv[i], 0, 63);

  /* Set new cos-dscp tble */
  for ( i=0 ; i < COS_TBL_SIZE ; i++ )
    work_map_tbls.ip_prec_dscp_map_tbl[i] = (u_int8_t)dscp_val[i];

  return CLI_SUCCESS;
}


CLI (no_mls_qos_map_ip_prec_dscp,
     no_mls_qos_map_ip_prec_dscp_cmd,
     "no mls qos map ip-prec-dscp",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the IP-precedence-to-DSCP map")
{
  struct nsm_master *nm = cli->vr->proto;
  int i;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Set default ip_prec-dscp value to working ip_prec-dscp table */
  for ( i=0 ; i < COS_TBL_SIZE ; i++ )
    work_map_tbls.ip_prec_dscp_map_tbl[i] = default_map_tbls.ip_prec_dscp_map_tbl[i];

  return CLI_SUCCESS;
}


CLI (mls_qos_map_policed_dscp,
     mls_qos_map_policed_dscp_cmd,
     "mls qos map policed-dscp <0-63> to <0-63>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the policed-DSCP map",
     "Ingress DSCP value",
     "To",
     "Egress DSCP vlaue (marked down DSCP")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t in_dscp, out_dscp;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_UINT32_RANGE ("ingress dscp", in_dscp, argv[0], 0, 63);
  CLI_GET_UINT32_RANGE ("egress dscp", out_dscp, argv[1], 0, 63);

  work_map_tbls.policed_dscp_map_tbl[(u_int8_t)in_dscp] = (u_int8_t)out_dscp;

  return CLI_SUCCESS;
}


CLI (no_mls_qos_map_policed_dscp,
     no_mls_qos_map_policed_dscp_cmd,
     "no mls qos map policed-dscp",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the policed-DSCP map")
{
  struct nsm_master *nm = cli->vr->proto;
  int i;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Set default policed-dscp tble */
  for ( i=0 ; i < DSCP_TBL_SIZE ; i++ )
    work_map_tbls.policed_dscp_map_tbl[i] = i;

  return CLI_SUCCESS;
}


#if 0
CLI (mls_qos_map_dscp_cos,
     mls_qos_map_dscp_cos_cmd,
     "mls qos map dscp-cos <0-63> to <0-7>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the DSCP-to-CoS map",
     "DSCP table number <0-63>",
     "To",
     "CoS vlaue <0-7>")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t dscp_val, cos_val;
  int i, ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("dscp vlaue", dscp_val, argv[0], 0, 63);
  CLI_GET_INTEGER_RANGE ("cos vlaue", cos_val, argv[1], NSM_QOS_COS_MIN, NSM_QOS_COS_MAX);

  /* Set new dscp-cos tble */
  work_map_tbls.dscp_cos_map_tbl[(u_int8_t)dscp_val] = (u_int8_t)cos_val;

  return CLI_SUCCESS;
}


CLI (no_mls_qos_map_dscp_cos,
     no_mls_qos_map_dscp_cos_cmd,
     "no mls qos map dscp-cos",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the DSCP-to-CoS map")
{
  struct nsm_master *nm = cli->vr->proto;
  int i, ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Set default dscp-cos table */
  for (i=0 ; i < DSCP_TBL_SIZE ; i++ )
    work_map_tbls.dscp_cos_map_tbl[i] = default_map_tbls.dscp_cos_map_tbl[i];

  return CLI_SUCCESS;
}
#endif /* 0 */


CLI (mls_qos_map_dscp_cos,
     mls_qos_map_dscp_cos_cmd,
     "mls qos map dscp-cos NAME (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>) to <0-7>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify DSCP-to-COS map",
     "Specify map name",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "To",
     "Outgoing COS value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_dscp_cos_list *dcosl;
  int in_dscp, out_cos;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! argv[0])
    {
      cli_out (cli, "%% Need DSCP-to-COS map name\n");
      return CLI_ERROR;
    }

  dcosl = nsm_dscp_cos_list_get (qosg.dcosl, argv[0]);
  if (! dcosl)
    {
      cli_out (cli, "%% Failed to find or create a DSCP-to-COS map\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("Outgoing cos", out_cos, argv[argc - 1], 0, 7);
  
  switch (argc)
    {
    case 10:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[8], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
    case 9:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[7], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
    case 8:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[6], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
    case 7:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[5], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
    case 6:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[4], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
    case 5:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[3], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
    case 4:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[2], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
    case 3:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[1], 0, 63);
      dcosl->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_cos;
      break;
    default:
      cli_out (cli, "%% Invalid input \n");
      return CLI_ERROR;
    }

#ifdef HAVE_HA
   nsm_dscp_cos_data_cal_modify (dcosl);
#endif

  return CLI_SUCCESS;
}


CLI (no_mls_qos_map_dscp_cos,
     no_mls_qos_map_dscp_cos_cmd,
     "no mls qos map dscp-cos NAME",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify DSCP-to-COS map",
     "Specify DSCP-to-COS map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_dscp_cos_list *dcosl;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! argv[0])
    {
      cli_out (cli, "%% Need DSCP-to-COS map name\n");
      return CLI_ERROR;
    }

  dcosl = nsm_dscp_cos_list_lookup (qosg.dcosl, argv[0]);
  if (! dcosl)
    {
      cli_out (cli, "%% Can't find DSCP-to-COS map\n");
      return CLI_ERROR;
    }
  nsm_dscp_cos_list_delete (dcosl);

  return CLI_SUCCESS;
}


CLI (mls_qos_map_dscp_mutation,
     mls_qos_map_dscp_mutation_cmd,
     "mls qos map dscp-mutation NAME (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>) to <0-63>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the DSCP mutation map",
     "Specify DSCP mutation name or ID",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "Incoming DSCP value",
     "To",
     "Outgoing DSCP value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_dscp_mut_list *dscpml;
  int in_dscp, out_dscp;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (nsm_qos_if_dscp_mutation_map_attached (nm, argv[0]) == NSM_TRUE)
    {
      cli_out (cli, "%% Dscp-mutation map attached to interface\n");
      return CLI_ERROR;
    }

  dscpml = nsm_dscp_mut_list_get (qosg.dscpml, argv[0]);
  if (! dscpml)
    {
      cli_out (cli, "%% Failed to find or create a dscp mutation map\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("Outgoing dscp", out_dscp, argv[argc - 1], 0, 63);
  
  switch (argc)
    {
    case 10:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[8], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
    case 9:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[7], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
    case 8:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[6], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
    case 7:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[5], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
    case 6:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[4], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
    case 5:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[3], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
    case 4:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[2], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
    case 3:
      CLI_GET_INTEGER_RANGE ("Incoming dscp", in_dscp, argv[1], 0, 63);
      dscpml->d.dscp[(u_int8_t)in_dscp] = (u_int8_t) out_dscp;
      break;
    default:
      cli_out (cli, "%% Invalid input \n");
      return CLI_ERROR;
    }
#ifdef HAVE_HA
  nsm_dscp_mut_data_cal_modify(dscpml);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}


CLI (no_mls_qos_map_dscp_mutation,
     no_mls_qos_map_dscp_mutation_cmd,
     "no mls qos map dscp-mutation NAME",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify maps",
     "Modify the DSCP mutation map",
     "Specify DSCP mutation name or ID")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_dscp_mut_list *dscpml;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! argv[0])
    {
      cli_out (cli, "%% Need a DSCP mutation map's name\n");
      return CLI_ERROR;
    }

  if (nsm_qos_if_dscp_mutation_map_attached (nm, argv[0]) == NSM_TRUE)
    {
      cli_out (cli, "%% Dscp-mutation map attached to interface\n");
      return CLI_ERROR;
    }

  dscpml = nsm_dscp_mut_list_lookup (qosg.dscpml, argv[0]);
  if (! dscpml)
    {
      cli_out (cli, "%% Can't find dscp mutation map \n");
      return CLI_ERROR;
    }

  nsm_dscp_mut_list_delete (dscpml);

  return CLI_SUCCESS;
}


CLI (mls_qos_min_reserve,
     mls_qos_min_reserve_cmd,
     "mls qos min-reserve <1-8> <10-170>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Configure the buffer size of min-reserve level",
     "Specify level ID in Fast Ethernet",
     "Specify holding packets in Fast Ethernet")
{
  struct nsm_master *nm = cli->vr->proto;
  int lid, mr;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("level-id", lid, argv[0], 1, 8);
  CLI_GET_INTEGER_RANGE ("min-reserve", mr, argv[1], 10, 170);

  qosg.level[lid] = mr;

#ifdef HAVE_HAL
  /* Set min-reserve level */
  ret = hal_qos_set_min_reserve_level(lid, mr);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure min-reserve level \n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (&qosg);
#endif /*HAVE_HA*/
  return CLI_SUCCESS;
}


CLI (no_mls_qos_min_reserve,
     no_mls_qos_min_reserve_cmd,
     "no mls qos min-reserve <0-7>",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Minimum reservation",
     "Specify level ID")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t lid;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_UINT32_RANGE ("level-id", lid, argv[0], 0, 7);

  qosg.level[lid] = 0;
#ifdef HAVE_HAL
  /* Unset min-reserve level */
  ret = hal_qos_set_min_reserve_level(lid, 0);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to unconfigure min-reserve level \n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (&qosg);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}


CLI (mls_qos_aggregate_police,
     mls_qos_aggregate_police_cmd,
     "mls qos aggregate-police NAME <1-1000000> <1-20000> exceed-action drop",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify a policer for the classified traffic",
     "Specify aggregate-policer name",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes)",
     "Specify the action if exceed-action",
     "Drop the packet")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_agp_list *agpl;
  u_int32_t rate_bps, burst_byte, exd_act;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (nsm_pmap_class_agg_policer_exists (nm, argv[0], NSM_TRUE) == NSM_TRUE)
    {
      cli_out (cli, "%% Aggregate policer attached to atleast one policy map applied to one or more interfaces\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("average rate", rate_bps, argv[1], 1, 1000000);
  CLI_GET_INTEGER_RANGE ("burst buffer size", burst_byte, argv[2], 1, 20000);

  exd_act = NSM_QOS_EXD_ACT_DROP;

  agpl = nsm_agp_list_get (qosg.agp, argv[0]);
  if (! agpl)
    {
      cli_out (cli, "%% Failed to get aggregate-police\n");
      return CLI_ERROR;
    }

  agpl->p.avg = rate_bps;
  agpl->p.burst = burst_byte;
  agpl->p.exd_act = exd_act;

#ifdef HAVE_HA
  nsm_agp_data_cal_modify(agpl);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}


CLI (no_mls_qos_aggregate_police,
     no_mls_qos_aggregate_police_cmd,
     "no mls qos aggregate-police NAME",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Aggregate police",
     "Specify aggregate policer name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_agp_list *agpl;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  agpl = nsm_agp_list_lookup (qosg.agp, argv[0]);
  if (! agpl)
    {
      cli_out (cli, "%% Can't find aggregate-police (%s)\n", argv[0]);
      return CLI_ERROR;
    }

  if (nsm_pmap_class_agg_policer_exists (nm, argv[0], NSM_FALSE) == NSM_TRUE)
    {
      cli_out (cli, "%% Aggregate policer attached to atleast one policy map\n");
      return CLI_ERROR;
    }

  nsm_agp_list_delete (agpl);

  return CLI_SUCCESS;
}


/* Interface mode commands */
CLI (service_policy_input,
     service_policy_input_cmd,
     "service-policy input NAME",
     "Service policy",
     "Ingress service policy",
     "Specify policy input name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_pmap_list *pmapl;
  struct nsm_qif_list *qifl;
  int action, dir;

#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (ifp->hw_type == IF_TYPE_VLAN)
    {
      cli_out (cli, "%% Service policy cannot be applied to vlan interfaces\n");
      return CLI_ERROR;
    }

#ifdef HAVE_LACPD
  {
    struct nsm_if *zif;
    zif = ifp->info;
    if (zif->agg.type == NSM_IF_AGGREGATOR)
      {
        cli_out (cli, "%% Service policy cannot be applied to aggregator interfaces\n");
        return CLI_ERROR;
      }
  }
#endif /* HAVE_LACPD */

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, &ifp->name[0]);

  if (qifl == NULL)
    {
      qifl = nsm_qif_list_get (qosg.qif, &ifp->name[0]);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error\n");
                return CLI_ERROR;
        }
    }

  if (qifl->input_pmap_name[0] != 0x00)
    {
      cli_out (cli, "%% Fail because policy-map(%s) is already attached\n", qifl->input_pmap_name);
      return CLI_ERROR;
    }

  if (qifl->dscp_mut_name[0] != '\0')
    {
      cli_out (cli, "%% Policy-map cannot be applied as dscp-mutation is already applied to this interface, \n");
      return CLI_ERROR;
    }

  /* Get the policy-map information from linked list */
  pmapl = nsm_pmap_list_lookup (nm->pmap, argv[0]);
  if (! pmapl)
    {
      cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  /* Attach policy-map to the interface */
  action = NSM_QOS_ACTION_ADD;
  dir = NSM_QOS_DIRECTION_INGRESS;

#ifdef HAVE_HAL
  ret = hal_qos_set_policy_map (ifp->ifindex, action, dir);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to attach policy-map(%s) to the interface\n", argv[0]);
      return CLI_ERROR;
    }

  ret = nsm_qos_hal_set_all_class_map (nm, pmapl, ifp->ifindex, action, dir);

  if (ret < 0)
    {
      if (ret == HAL_ERR_QOS_INVALID_RATE_STEP)
        cli_out (cli, "%% Fail to attach class-map due to invalid CIR\n");
      else if (ret == HAL_ERR_QOS_INVALID_EBS)
        cli_out (cli, "%% Fail to attach class-map due to invalid EBS\n");
      else if (ret == HAL_ERR_QOS_INVALID_CBS)
        cli_out (cli, "%% Fail to attach class-map due to invalid CBS\n");
      else
        cli_out (cli, "%% Fail to attach class-map to the interface\n");
      hal_qos_send_policy_map_detach(ifp->ifindex, action, dir);
      return CLI_ERROR;
    }
  
  /* Install the policy-map into hardware */
  ret = hal_qos_send_policy_map_end(ifp->ifindex, action, dir);
  if (ret < 0)
    {
      /* Clear policy-map because of HW installation is failed */
      hal_qos_send_policy_map_detach(ifp->ifindex, action, dir);
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pmapl->attached ++;

  pal_strcpy ( &qifl->if_name[0], &ifp->name[0]);
  pal_strcpy ( &qifl->input_pmap_name[0], argv[0]);
#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/
  return CLI_SUCCESS;
}


CLI (no_service_policy_input,
     no_service_policy_input_cmd,
     "no service-policy input NAME",
     CLI_NO_STR,
     "Service policy",
     "Ingress service policy",
     "Specify policy input name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_pmap_list *pmapl;
  struct nsm_qif_list *qifl;
  int action, dir;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, &ifp->name[0]);
  if (qifl == NULL)
    {
      cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
      return CLI_ERROR;
    }
  else
    {
      if (pal_strcmp (qifl->input_pmap_name, argv[0]) != 0)
        {
          cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
          return CLI_ERROR;
        }
    }

  /* Get the policy-map information from linked list */
  pmapl = nsm_pmap_list_lookup (nm->pmap, argv[0]);
  if (! pmapl)
    {
      cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  /* Detach policy-map from the interface */
  action = NSM_QOS_ACTION_DELETE;
  dir = NSM_QOS_DIRECTION_INGRESS;

#ifdef HAVE_HAL
  ret = hal_qos_set_policy_map (ifp->ifindex, action, dir);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to detach policy-map(%s) from the interface\n", argv[0]);
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pmapl->attached --;
  pal_mem_set ( &qifl->input_pmap_name[0], 0, (INTERFACE_NAMSIZ + 1));
#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}


CLI (service_policy_output,
     service_policy_output_cmd,
     "service-policy output NAME",
     "Service policy",
     "Egress service policy",
     "Specify policy output name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_pmap_list *pmapl;
  struct nsm_qif_list *qifl;
  int action, dir;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, &ifp->name[0]);
  if (qifl != NULL)
    {
      if (qifl->output_pmap_name[0] != 0x00)
        {
          cli_out (cli, "%% Fail because policy-map(%s) is already attached\n", qifl->output_pmap_name);
          return CLI_ERROR;
        }
    }
  /* Get the policy-map information from linked list */
  pmapl = nsm_pmap_list_lookup (nm->pmap, argv[0]);
  if (! pmapl)
    {
      cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  /* Attach policy-map to the interface */
  action = NSM_QOS_ACTION_ADD;
  dir = NSM_QOS_DIRECTION_EGRESS;

#ifdef HAVE_HAL
  ret = hal_qos_set_policy_map (ifp->ifindex, action, dir);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to attach policy-map(%s) to the interface\n", argv[0]);
      return CLI_ERROR;
    }

  ret = nsm_qos_hal_set_all_class_map (nm, pmapl, ifp->ifindex, action, dir);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to attach class-map to the interface\n");
      hal_qos_send_policy_map_detach(ifp->ifindex, action, dir);
      return CLI_ERROR;
    }
  
  /* Install the policy-map into hardware */
  ret = hal_qos_send_policy_map_end(ifp->ifindex, action, dir);
  if (ret < 0)
    {
      /* Clear policy-map because of HW installation is failed */
      hal_qos_send_policy_map_detach(ifp->ifindex, action, dir);
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pmapl->attached ++;

  qifl = nsm_qif_list_get (qosg.qif, &ifp->name[0]);

  if (! qifl)
    {
      cli_out (cli, "%% Internal error\n");
      return CLI_ERROR;
    }

  pal_strcpy ( &qifl->if_name[0], &ifp->name[0]);
  pal_strcpy ( &qifl->output_pmap_name[0], argv[0]);
#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/
  return CLI_SUCCESS;
}


CLI (no_service_policy_output,
     no_service_policy_output_cmd,
     "no service-policy output NAME",
     CLI_NO_STR,
     "Service policy",
     "Egress service policy",
     "Specify policy output name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_pmap_list *pmapl;
  struct nsm_qif_list *qifl;
  int action, dir;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, &ifp->name[0]);
  if (qifl == NULL)
    {
      cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
      return CLI_ERROR;
    }
  else
    {
      if (qifl->output_pmap_name[0] == 0x00)
        {
          cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
          return CLI_ERROR;
        }
    }

  /* Get the policy-map information from linked list */
  pmapl = nsm_pmap_list_lookup (nm->pmap, argv[0]);
  if (! pmapl)
    {
      cli_out (cli, "%% Can't find policy-map(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  /* Detach policy-map from the interface */
  action = NSM_QOS_ACTION_DELETE;
  dir = NSM_QOS_DIRECTION_EGRESS;

#ifdef HAVE_HAL
  ret = hal_qos_set_policy_map (ifp->ifindex, action, dir);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to attach policy-map(%s) to the interface\n", argv[0]);
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pmapl->attached --;
  pal_mem_set ( &qifl->output_pmap_name[0], 0, (INTERFACE_NAMSIZ + 1));
#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}
#if 0
CLI (wrr_queue_weight_config,
     wrr_queue_weight_config_cmd,
     "wrr-queue weight <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016>",
     "WDRR queue",
     "Configure egress queue WDRR weights, weight 0 in particular means strict priority secheuling, weight 1-1016 in unit of 2 kilo bytes per second",
     "Weight value of Queue 0 ",
     "Weight value of Queue 1 ",
     "Weight value of Queue 2 ",
     "Weight value of Queue 3 ",
     "Weight value of Queue 4 ",
     "Weight value of Queue 5 ",
     "Weight value of Queue 6 ",
     "Weight value of Queue 7 ")
{
  struct nsm_master *nm = cli->vr->proto;
  //int sum = 10000;
  int ratio[MAX_NUM_OF_QUEUE];
  int i;
  struct interface *ifp = cli->index;

 // struct vport_port *portp = cli->index;

  struct nsm_qif_list *qifl;
  
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
/*
  sum = 0;
  for ( i=0 ; i < MAX_NUM_OF_QUEUE ; i++ )
    {
      CLI_GET_INTEGER_RANGE ("weight", ratio[i], argv[i], 0, 127);

      /* Should not be exceed 100 */
 /*     sum += ratio[i];
    }
   
*/
   /*wangjian@150114, in unit of 2kbps, */
   
   CLI_GET_INTEGER_RANGE ("weight", ratio[0], argv[0], 0, 1016);
   CLI_GET_INTEGER_RANGE ("weight", ratio[1], argv[1], 0, 1016);
   CLI_GET_INTEGER_RANGE ("weight", ratio[2], argv[2], 0, 1016);
   CLI_GET_INTEGER_RANGE ("weight", ratio[3], argv[3], 0, 1016);
   CLI_GET_INTEGER_RANGE ("weight", ratio[4], argv[4], 0, 1016);
   CLI_GET_INTEGER_RANGE ("weight", ratio[5], argv[5], 0, 1016);
   CLI_GET_INTEGER_RANGE ("weight", ratio[6], argv[6], 0, 1016);
   CLI_GET_INTEGER_RANGE ("weight", ratio[7], argv[7], 0, 1016);

 /* if (sum > 1016)   //127*8
    {
      cli_out (cli, "%% Total ratios exceed 1016\n");
	  
	  cli_out (cli, "%% test ratio_0 = %d  ratio_1 = %d  ratio_2 = %d  ratio_3 = %d  ratio_4 = %d  ratio_5 = %d  ratio_6 = %d  ratio_7 = %d\n",
					 ratio[0],ratio[1],ratio[2],ratio[3],ratio[4],ratio[5],ratio[6],ratio[7]);
      return CLI_ERROR;
    }
*/
  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (!qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }
 // cli_out (cli, " test qifl->name =	%s,   test portp->name = %s\n",qifl->name,portp->name);
 // return CLI_ERROR;

  pal_mem_cpy (qifl->queue_limit, ratio, MAX_NUM_OF_QUEUE * sizeof (int));
  SET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_QUEUE_LIMIT);
  qifl->weight[0] = ratio[0];
  qifl->weight[1] = ratio[1];
  qifl->weight[2] = ratio[2];
  qifl->weight[3] = ratio[3];
  qifl->weight[4] = ratio[4];
  qifl->weight[5] = ratio[5];
  qifl->weight[6] = ratio[6];
  qifl->weight[7] = ratio[7];
	 
  
#ifdef HAVE_HAL
  ret = hal_qos_wrr_queue_limit(ifp->ifindex, &ratio[0]);
  if (ret < 0)
    cli_out (cli, "%% Fail to configure egress queue size ratios\n");
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}
CLI (no_wrr_queue_queue_limit,
     no_wrr_queue_weight_config_cmd,
     "no wrr-queue weight",
     CLI_NO_STR,
     "WRR queue",
     "Restore weight default configuration")
     
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
   //struct vport_port *portp = cli->index;
  int i;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */
  struct nsm_qif_list *qifl;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }

  for (i = 0; i < 7; i++)
    {
      qifl->weight[i] = 1;
    }
   qifl->weight[7] = 0;
  
  UNSET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_QUEUE_LIMIT);

#ifdef HAVE_HAL
  ret = hal_qos_no_wrr_queue_limit(ifp->ifindex, &qifl->weight[0]);
  if (ret < 0)
    cli_out (cli, "%% Fail to configure egress queue size ratios\n");
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}
#endif
#if 0
CLI (wrr_queue_queue_limit,
     wrr_queue_queue_limit_cmd,
     "wrr-queue queue-limit <1-100> <1-100> <1-100> <1-100> <1-100> <1-100> <1-100> <1-100>",
     "WRR queue",
     "Configure egress queue size ratios (Should be 100 totally)",
     "Weight value Queue 0 ",
     "Weight value Queue 1 ",
     "Weight value Queue 2 ",
     "Weight value Queue 3 ",
     "Weight value Queue 4 ",
     "Weight value Queue 5 ",
     "Weight value Queue 6 ",
     "Weight value Queue 7 ")
{
  struct nsm_master *nm = cli->vr->proto;
  int sum, ratio[MAX_NUM_OF_QUEUE];
  int i;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  sum = 0;
  for ( i=0 ; i < MAX_NUM_OF_QUEUE ; i++ )
    {
      CLI_GET_INTEGER_RANGE ("ratio", ratio[i], argv[i], 1, 100);

      /* Should not be exceed 100 */
      sum += ratio[i];
    }

  if (sum > 100)
    {
      cli_out (cli, "%% Total ratios exceed 100\n");
      return CLI_ERROR;
    }

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }

  pal_mem_cpy (qifl->queue_limit, ratio, MAX_NUM_OF_QUEUE * sizeof (int));
  SET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_QUEUE_LIMIT);

#ifdef HAVE_HAL
  ret = hal_qos_wrr_queue_limit(ifp->ifindex, &ratio[0]);
  if (ret < 0)
    cli_out (cli, "%% Fail to configure egress queue size ratios\n");
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}
#endif
#if 0
CLI (no_wrr_queue_queue_limit,
     no_wrr_queue_queue_limit_cmd,
     "no wrr-queue queue-limit",
     CLI_NO_STR,
     "WRR queue",
     "Queue limit")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  int i;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */
  struct nsm_qif_list *qifl;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }

  for (i = 0; i < MAX_NUM_OF_QUEUE; i++)
    {
      qifl->queue_limit[i] = 1;
    }
  UNSET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_QUEUE_LIMIT);

#ifdef HAVE_HAL
  ret = hal_qos_no_wrr_queue_limit(ifp->ifindex, &qifl->queue_limit[0]);
  if (ret < 0)
    cli_out (cli, "%% Fail to configure egress queue size ratios\n");
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}
#endif
#if 0
CLI (wrr_queue_queueid_threshold,
     wrr_queue_queueid_threshold_cmd,
     "wrr-queue threshold <0-7> <1-100> <1-100>",
     "WRR queue",
     "Configure tail-drop threshold percentages",
     "Specify queue ID",
     "Specify the tail-drop threshold percentages 1",
     "Specify the tail-drop threshold percentages 2")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, thres1, thres2;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);
  CLI_GET_INTEGER_RANGE ("tail drop threshold1", thres1, argv[1], 1, 100);
  CLI_GET_INTEGER_RANGE ("tail drop threshold2", thres2, argv[2], 1, 100);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }

  SET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_TD_THRESHOLD);
  qifl->queue[qid].td_threshold1 = thres1;
  qifl->queue[qid].td_threshold2 = thres2;

#ifdef HAVE_HAL
  ret = hal_qos_wrr_tail_drop_threshold(ifp->ifindex, qid, thres1, thres2);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure tail-drop threshold\n");
      return CLI_ERROR;
    }
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}
#endif
#if 0
CLI (wrr_queue_egress_car,
     wrr_queue_egress_car_cmd,
     "wrr-queue car <0-7> <0-1000000> <0-1000000>",
     "WRR queue car",
     "Configure minimum and maximum bandwidth for the queue specified",
     "Specify queue ID",
     "Specify the cir in term of kbps",
     "Specify the pir in term of kbps")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, cir, pir ;

  //struct interface *ifp = cli->index;
  struct vport_port *portp = cli->index;
  
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  

  /* Check QoS state */
  
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }
       
  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);
  CLI_GET_INTEGER_RANGE ("car threshold1", cir , argv[1], 0, 1000000);
  CLI_GET_INTEGER_RANGE ("car threshold2", pir , argv[2], 0, 1000000);

  qifl = nsm_qif_list_lookup (qosg.qif, portp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, portp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error: cann't find port \n");
                return CLI_ERROR;
        }
    }

  /* record the car info into qos interface globals */
  SET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_TD_THRESHOLD);
  qifl->queue[qid].td_threshold1 = cir;
  qifl->queue[qid].td_threshold2 = pir;

#ifdef HAVE_HAL
  ret = hal_qos_wrr_tail_drop_threshold(portp->index, qid, cir , pir);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure egress car: index %d qid %d cir %d pir\n", portp->index,qid, cir, pir);
      return CLI_ERROR;
    }
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}


CLI (no_wrr_queue_queueid_threshold,
     no_wrr_queue_egress_car_cmd,
     "no wrr-queue car <0-7>",
     CLI_NO_STR,
     "WRR queue",
     "Restore egress car default configuration",
     "Specify queue ID")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid;
//  struct interface *ifp = cli->index;
  struct vport_port *portp = cli->index;

  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);

  qifl = nsm_qif_list_lookup (qosg.qif, portp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, portp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }

  /* Set default threshold */
  qifl->queue[qid].td_threshold1 = 0;
  qifl->queue[qid].td_threshold2 = 0;
  UNSET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_TD_THRESHOLD);

#ifdef HAVE_HAL
  ret = hal_qos_wrr_tail_drop_threshold( portp->index, qid, 0, 0);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure default tail-drop threshold\n");
      return CLI_ERROR;
    }
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}

#endif
CLI (wrr_queue_threshold_dscp_map,
     wrr_queue_threshold_dscp_map_cmd,
     "wrr-queue dscp-map <1-2> (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>)",
     "WRR queue",
     "Specify map DSCP values to the wred-drop thresholds of egress queue",
     "Specify the threshold ID",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value",
     "DSCP value")
{
  struct nsm_master *nm = cli->vr->proto;
  int thid;
  u_char dscp[8];
  int tmp_dscp;
  int i;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("threshold id", thid, argv[0], 1, 2);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  i = 0;
  switch (argc)
    {
    case 9:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[8], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
    case 8:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[7], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
    case 7:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[6], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
    case 6:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[5], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
    case 5:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[4], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
    case 4:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[3], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
    case 3:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[2], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
    case 2:
      CLI_GET_INTEGER_RANGE ("dscp value", tmp_dscp, argv[1], 0, 63);
      dscp[i] = (u_char )tmp_dscp;
      qifl->wred_dscp_threshold[dscp[i++]] = thid;
      break;
    default:
      cli_out (cli, "Invalid input \n");
      return CLI_ERROR;
    }


#ifdef HAVE_HAL
  /* Include threshold ID and DSCP values */
  ret = hal_qos_wrr_threshold_dscp_map (ifp->ifindex, thid, dscp, i);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure DSCP values to tail-drop thresholds ID\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}

CLI (no_wrr_queue_threshold_dscp_map,
     no_wrr_queue_threshold_dscp_map_cmd,
     "no wrr-queue dscp-map <1-2>",
     CLI_NO_STR,
     "WRR queue",
     "DSCP map",
     "Threshold ID of the queue, range is 1 to 2")
{
  struct nsm_master *nm = cli->vr->proto;
  int thid;
  struct interface *ifp = cli->index;
  u_char dscp[64];
  struct nsm_qif_list *qifl;
  int i, j;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("threshold id", thid, argv[0], 1, 2);


  /* Check the range */
  CLI_GET_INTEGER_RANGE ("threshold id", thid, argv[0], 1, 2);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      cli_out (cli, "%% Internal error \n");
      return CLI_ERROR;
    }

  j = 0;
  for (i = 0; i <= NSM_QOS_DSCP_MAX; i++)
    {
      if (qifl->wred_dscp_threshold[i] == thid)
        {
          qifl->wred_dscp_threshold[i] = 0;
          dscp[j++] = i;
        }
    }

#ifdef HAVE_HAL
  /* Include threshold ID and DSCP values */
  ret = hal_qos_wrr_threshold_dscp_map (ifp->ifindex, 0, dscp, j);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure default DSCP values to tail-drop thresholds ID\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}
#if 0
CLI (wrr_queue_random_detect_max_threshold,
     wrr_queue_random_detect_max_threshold_cmd,
     "wrr-queue random-detect max-threshold <0-7> <1-100> <1-100>",
     "WRR queue",
     "Random detect",
     "Configure WRED drop threshold percentages on egress queue",
     "Specify queue ID",
     "Specify the threshold percentages-1 values",
     "Specify the threshold percentages-2 values")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, threshold1, threshold2;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);
  CLI_GET_INTEGER_RANGE ("wred threshold1", threshold1, argv[1], 1, 100);
  CLI_GET_INTEGER_RANGE ("wred threshold2", threshold2, argv[2], 1, 100);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }

  SET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_WRED_THRESHOLD);
  qifl->queue[qid].wred_threshold1 = threshold1;
  qifl->queue[qid].wred_threshold2 = threshold2;
  
#ifdef HAVE_HAL
  ret = hal_qos_wrr_wred_drop_threshold(ifp->ifindex,
                                        qid,
                                        threshold1,
                                        threshold2);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure WRED drop threshold\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}
#endif
#if 0
CLI (wrr_queue_wred,
     wrr_queue_wred_cmd,
     "wrr-queue wred <0-7> <0-7> <0-100> <0-90> <0-255>",
     "WRR queue",
     "Configure WRED drop threshold and weight on egress queue",
     "Specify queue ID",
     "Specify the bit map of color combination,bit 0 = 1->red; bit 1 = 1->yellow; bit 2 = 1->green. if set 0,restore default setting",
     "Specify the percentage of average queue size to start dropping packets",
     "Specify the the probality slope of droping packets",
     "Specify the average time in us, average_time/4 = 2**WRED_weight")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, startpoint,slope,color,time;
  //struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
   struct vport_port *portp = cli->index;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);
  CLI_GET_INTEGER_RANGE ("wred color", color, argv[1], 0, 7);
  CLI_GET_INTEGER_RANGE ("wred startpoint",startpoint , argv[2], 0,100 );
  CLI_GET_INTEGER_RANGE ("wred slope", slope, argv[3], 0, 90);
  CLI_GET_INTEGER_RANGE ("wred average time", time, argv[4], 0, 255);

  qifl = nsm_qif_list_lookup (qosg.qif, portp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, portp->name);
      if (! qifl)
        {
                cli_out (cli, "%% Internal error \n");
                return CLI_ERROR;
        }
    }

  SET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_WRED_THRESHOLD);
  qifl->queue[qid].startpoint = startpoint;
  qifl->queue[qid].slope = slope;
  qifl->queue[qid].color = color;
  qifl->queue[qid].time= time;

 
  
#ifdef HAVE_HAL
  ret = hal_qos_wrr_wred_drop_threshold(portp->index,qid,color,startpoint,slope,time);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure WRED drop threshold\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}

CLI (no_wrr_queue_random_detect_max_threshold,
     no_wrr_queue_wred_cmd,
     "no wrr-queue wred <0-7>",
     CLI_NO_STR,
     "WRR queue",
     "Restore wred default configuration",
     "Specify queue Id")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid,startpoint,slope,color,time;
  struct vport_port *portp = cli->index;
 // struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);

  qifl = nsm_qif_list_lookup (qosg.qif, portp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, portp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  UNSET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_WRED_THRESHOLD);
    qifl->queue[qid].startpoint = 1;
	qifl->queue[qid].slope = 1;
	qifl->queue[qid].color = 0;
	qifl->queue[qid].time= 1;
  

#ifdef HAVE_HAL
  ret = hal_qos_wrr_wred_drop_threshold(portp->index,qid,0,1,1,1);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure default WRED drop threshold\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}

#endif
#if 0
CLI (no_wrr_queue_random_detect_max_threshold,
     no_wrr_queue_random_detect_max_threshold_cmd,
     "no wrr-queue random-detect max-thresold <0-7>",
     CLI_NO_STR,
     "WRR queue",
     "Random detect",
     "Max. threshold",
     "Queue Id")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  UNSET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_WRED_THRESHOLD);
  qifl->queue[qid].wred_threshold1 = 100;
  qifl->queue[qid].wred_threshold2 = 100;


#ifdef HAVE_HAL
  ret = hal_qos_wrr_wred_drop_threshold(ifp->ifindex,
                                        qid,
                                        100, 100);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure default WRED drop threshold\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}

#endif
CLI (priority_queue_out,
     priority_queue_out_cmd,
     "priority-queue out",
     "Enable the egress expedite queue",
     "Enable the egress expedite queue")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }
  
  qifl->weight[NSM_QOS_DEFAULT_EXPEDITE_QUEUE] = 0; 
  SET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_PRIORITY_QUEUE);

#ifdef HAVE_HAL
  /* Set the bandwidth of queue on the interace */
  ret = hal_qos_wrr_set_expedite_queue(ifp->ifindex, NSM_QOS_DEFAULT_EXPEDITE_QUEUE, 0);
                                       
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure priqueue on the interface\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}


CLI (no_priority_queue_out,
     no_priority_queue_out_cmd,
     "no priority-queue out",
     CLI_NO_STR,
     "Enable the egress expedite queue",
     "Enable the egress expedite queue")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      cli_out (cli, "%% Internal error \n");
      return CLI_ERROR;
    }
  
  qifl->weight[NSM_QOS_DEFAULT_EXPEDITE_QUEUE] = NSM_QOS_IF_QUEUE_WEIGHT_DEFAULT; 
  UNSET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_PRIORITY_QUEUE);

#ifdef HAVE_HAL
  ret = hal_qos_wrr_unset_expedite_queue(ifp->ifindex, NSM_QOS_DEFAULT_EXPEDITE_QUEUE, NSM_QOS_IF_QUEUE_WEIGHT_DEFAULT);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure priqueue on the interface\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}


CLI (wrr_queue_bandwidth,
     wrr_queue_bandwidth_cmd,
     "wrr-queue bandwidth <1-65535> <1-65535> <1-65535> <1-65535> <1-65535> <1-65535> <1-65535> <1-65535>",
     "WRR queue",
     "Specify bandwidth ratios",
     "Specify the weight of queue 0",
     "Specify the weight of queue 1",
     "Specify the weight of queue 2",
     "Specify the weight of queue 3",
     "Specify the weight of queue 4",
     "Specify the weight of queue 5",
     "Specify the weight of queue 6",
     "Specify the weight of queue 7")
{
  struct nsm_master *nm = cli->vr->proto;
  int weight[8];
  int i;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  for ( i=0 ; i < MAX_NUM_OF_QUEUE ; i++ )
    {
      CLI_GET_INTEGER_RANGE ("weight of queue", weight[i], argv[i], 1, 65535);
    }

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  pal_mem_cpy (qifl->weight, weight, 8 * sizeof (int));
  SET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_WEIGHT);

#ifdef HAVE_HAL
  /* Set the bandwidth of queue on the interace */
  /* Call bcmx_cosq_port_bandwidth_set() in HSL */
  ret = hal_qos_wrr_set_bandwidth_of_queue(ifp->ifindex, &weight[0]);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure bandwidth of queue on the interface\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}

CLI (no_wrr_queue_bandwidth,
     no_wrr_queue_bandwidth_cmd,
     "no wrr-queue bandwidth <0-7>",
     CLI_NO_STR,
     "WRR queue",
     "Specify bandwidth of QoS queue",
     "Specify Qos queue ID")
{
  struct nsm_master *nm = cli->vr->proto;
  int weight[8];
  struct interface *ifp = cli->index;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */
  struct nsm_qif_list *qifl;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  weight[0] = 1;
  weight[1] = 1;
  weight[2] = 1;
  weight[3] = 1;
  weight[4] = 1;
  weight[5] = 1;
  weight[6] = 1;
  weight[7] = 1;

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      cli_out (cli, "%% Internal error \n");
      return CLI_ERROR;
    }

  pal_mem_cpy (qifl->weight, weight, 8 * sizeof (int));
  UNSET_FLAG (qifl->config, NSM_QOS_IF_CONFIG_WEIGHT);

#ifdef HAVE_HAL
  ret = hal_qos_wrr_set_bandwidth_of_queue(ifp->ifindex, &weight[0]);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure default bandwidth of queue on the interface\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;

}

CLI (wrr_queue_cos_map,
     wrr_queue_cos_map_cmd,
     "wrr-queue cos-map <0-7> (<0-7>|<0-7> <0-7>|<0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     "WRR queue",
     "CoS map",
     "Specify queue ID",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value",
     "Specify CoS value")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, i;
  int cos[8];

  struct nsm_qif_list *qifl;
  struct interface *ifp = cli->index;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue ID", qid, argv[0], 0, 7);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  i = 0;
  switch (argc)
    {
    case 9:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[8], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
    case 8:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[7], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
    case 7:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[6], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
    case 6:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[5], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
    case 5:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[4], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
    case 4:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[3], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
    case 3:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[2], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
    case 2:
      CLI_GET_INTEGER_RANGE ("cos value", cos[i], argv[1], 0, 7);
      qifl->cosq_map[cos[i++]] = qid;
      break;
    default:
      cli_out (cli, "%% Invalid input \n");
      return CLI_ERROR;
    }

#ifdef HAVE_HAL
  ret = hal_qos_wrr_queue_cos_map_set (ifp->ifindex, qid, (u_char *)cos, i);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to set new CoS value to indicated egress queue\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}

CLI (no_wrr_queue_cos_map,
     no_wrr_queue_cos_map_cmd,
     "no wrr-queue cos-map <0-7>",
     CLI_NO_STR,
     "WRR queue",
     "CoS map",
     "Queue ID")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, cos[8];
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
  int i, j;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue_id", qid, argv[0], 0, 7);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      cli_out (cli, "%% Internal error \n");
      return CLI_ERROR;
    }

  /* Get default cos vlaues for egress queue */
  /* Queue id maps to the COS's eight values directly. */
  j = 0;
  for (i = 0; i < 8; i++)
    {
      if (qifl->cosq_map[i] == qid)
        {
          qifl->cosq_map[i] = i;
          cos[j++] = i;
        }
    }

#ifdef HAVE_HAL
  /* Send default cos-map for egress queue */
  ret = hal_qos_wrr_queue_cos_map_unset (ifp->ifindex, (u_char *)cos, j);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to set default CoS to indicated egress queue\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}


CLI (wrr_queue_min_reserve,
     wrr_queue_min_reserve_cmd,
     "wrr-queue min-reserve <0-7> <1-8>",
     "Wrr queue",
     "Configure the buffer size of the minimum-reserve level",
     "Specify a queue ID",
     "Specify a min-reserve level ID")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, lid;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);
  CLI_GET_INTEGER_RANGE ("min-reserve level id", lid, argv[1], 1, 8);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  qifl->queue[qid].min_reserve_level = lid;
  SET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_MIN_RESERVE); 

#ifdef HAVE_HAL
  /* Set min-reserve level to queue of a specific interface */
  ret = hal_qos_wrr_queue_min_reserve(ifp->ifindex, qid, lid);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure min-reserve level for a queue\n");
      return CLI_ERROR;
    }
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE__HA */
#endif /* HAVE_HAL*/
  return CLI_SUCCESS;
}


CLI (no_wrr_queue_min_reserve,
     no_wrr_queue_min_reserve_cmd,
     "no wrr-queue min-reserve <0-7>",
     CLI_NO_STR,
     "Wrr queue",
     "Configure the buffer size of the minimum-reserve level",
     "Specify a queue ID")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
    }

  qifl->queue[qid].min_reserve_level = 0;
  UNSET_FLAG (qifl->queue[qid].config, NSM_QOS_IFQ_CONFIG_MIN_RESERVE); 

#ifdef HAVE_HAL
  /* Set default min-reserve level to queue of a specific interface */
  ret = hal_qos_wrr_queue_min_reserve(ifp->ifindex, qid, 0);
  if (ret < 0)
    {
      cli_out (cli, "%% Fail to configure min-reserve level for a queue\n");
      return CLI_ERROR;
    }
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */
#endif /* HAVE_HAL */
  return CLI_SUCCESS;
}


CLI (mls_qos_monitor_dscp,
     mls_qos_monitor_dscp_cmd,
     "mls qos monitor dscp",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Monitor",
     "DSCP")
{
  struct nsm_master *nm = cli->vr->proto;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (mls_qos_trust,
     mls_qos_trust_cmd,
     "mls qos trust dscp)",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
  int trust_state;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Configure port trust state and then send message to HSL/BCM */
  /* Configure port trust state
   * Setting TRUST_COS/ TRUST_IP_PREC(TOS)/ TRUST_DSCP
   */
#if 0
  if (pal_strncmp (argv[0], "c", 1) == 0)
    trust_state = NSM_QOS_TRUST_COS;
  else if (pal_strncmp (argv[0], "d", 1) == 0)
    trust_state = NSM_QOS_TRUST_DSCP;
  else if (pal_strncmp (argv[0], "i", 1) == 0)
    trust_state = NSM_QOS_TRUST_IP_PREC;
  else
    {
      cli_out (cli, "%% Error in trust state\n");
      return CLI_ERROR;
    }
#endif /* 0 */

  trust_state = NSM_QOS_TRUST_DSCP;

#ifdef HAVE_HAL
  ret = hal_qos_set_trust_state_for_port(ifp->ifindex, trust_state);
  if (ret < 0)
    {
      cli_out (cli, "%% CoS not detached\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (qifl)
    {
      pal_strcpy ( &qifl->trust_state[0], argv[0]);
    }
  else
    {
      qifl = nsm_qif_list_get (qosg.qif, &ifp->name[0]);
      if (qifl)
        pal_strcpy ( &qifl->trust_state[0], argv[0]);
    }

  ifp->trust_state = trust_state;
  return CLI_SUCCESS;
}


CLI (no_mls_qos_trust,
     no_mls_qos_trust_cmd,
     "no mls qos trust dscp)",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_qif_list *qifl;
  int trust_state;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  trust_state = NSM_QOS_TRUST_NONE;
#ifdef HAVE_HAL
  ret = hal_qos_set_trust_state_for_port(ifp->ifindex, trust_state);
  if (ret < 0)
    {
      cli_out (cli, "%% CoS not detached\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (qifl)
    {
      pal_mem_set (&qifl->trust_state[0], 0, (INTERFACE_NAMSIZ + 1));
    }
  else
    {
      qifl = nsm_qif_list_get (qosg.qif, &ifp->name[0]);
      if (qifl)
        pal_mem_set (&qifl->trust_state[0], 0, (INTERFACE_NAMSIZ + 1));
    }

  ifp->trust_state = trust_state;
  return CLI_SUCCESS;
}


CLI (mls_qos_trust_cos_path_through_dscp,
     mls_qos_trust_cos_path_through_dscp_cmd,
     "mls cos trust cos path-through dscp",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Enable pass-through mode, not modify DSCP value",
     "CoS",
     "Path-through DSCP",
     "DSCP")
{
  struct nsm_master *nm = cli->vr->proto;
  int trust_state;
#ifdef HAVE_HAL
  struct interface *ifp = cli->index;
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Configure port trust state and then send message to HSL/BCM */
  /* Configure port trust state
   * Setting TRUST_COS/ TRUST_DSCP/ TRUST_IP_PREC(TOS)
   */
  trust_state = NSM_QOS_TRUST_COS_PT_DSCP;
#ifdef HAVE_HAL
  ret = hal_qos_set_trust_state_for_port(ifp->ifindex, trust_state);
  if (ret < 0)
    {
      cli_out (cli, "%% CoS not detached\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}


CLI (no_mls_qos_trust_cos_path_through_dscp,
     no_mls_qos_trust_cos_path_through_dscp_cmd,
     "no mls cos trust cos path-through dscp",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Enable pass-through mode, not modify DSCP value",
     "CoS",
     "Path-through DSCP",
     "DSCP")
{
  struct nsm_master *nm = cli->vr->proto;
  int trust_state;
#ifdef HAVE_HAL
  struct interface *ifp = cli->index;
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  trust_state = NSM_QOS_TRUST_NONE;
#ifdef HAVE_HAL
  ret = hal_qos_set_trust_state_for_port(ifp->ifindex, trust_state);
  if (ret < 0)
    {
      cli_out (cli, "%% CoS not detached\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}


CLI (mls_qos_trust_dscp_path_through_cos,
     mls_qos_trust_dscp_path_through_cos_cmd,
     "mls cos trust dscp path-through cos",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Enable pass-through mode, not modify CoS value",
     "DSCP",
     "Path-through CoS",
     "CoS")
{
  struct nsm_master *nm = cli->vr->proto;
  int trust_state;
#ifdef HAVE_HAL
  struct interface *ifp = cli->index;
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Configure port trust state and then send message to HSL/BCM */
  /* Configure port trust state
   * Setting TRUST_COS/ TRUST_DSCP/ TRUST_IP_PREC(TOS)
   */
  trust_state = NSM_QOS_TRUST_DSCP_PT_COS;
#ifdef HAVE_HAL
  ret = hal_qos_set_trust_state_for_port(ifp->ifindex, trust_state);
  if (ret < 0)
    {
      cli_out (cli, "%% CoS not detached\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}


CLI (no_mls_qos_trust_dscp_path_through_cos,
     no_mls_qos_trust_dscp_path_through_cos_cmd,
     "no mls cos trust dscp path-through cos",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Enable pass-through mode, not modify CoS value",
     "DSCP",
     "Path-through CoS",
     "CoS")
{
  struct nsm_master *nm = cli->vr->proto;
  int trust_state;
#ifdef HAVE_HAL
  struct interface *ifp = cli->index;
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  trust_state = NSM_QOS_TRUST_NONE;
#ifdef HAVE_HAL
  ret = hal_qos_set_trust_state_for_port(ifp->ifindex, trust_state);
  if (ret < 0)
    {
      cli_out (cli, "%% CoS not detached\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  return CLI_SUCCESS;
}


CLI (mls_qos_default_cos,
     mls_qos_default_cos_cmd,
     "mls qos cos <0-7>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Configure default CoS values",
     "CoS value")
{
  struct nsm_master *nm = cli->vr->proto;
  int cos_value;
  struct interface *ifp = cli->index;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("default cos", cos_value, argv[0], 0, 7);

  ret = nsm_qos_set_def_cos_for_port (&qosg, ifp, cos_value);

  if (ret < 0)
    {
      cli_out (cli, "%% CoS setting is failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (no_mls_qos_default_cos,
     no_mls_qos_default_cos_cmd,
     "no mls qos cos",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Configure the default CoS values ")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  int ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  ret = nsm_qos_set_def_cos_for_port (&qosg, ifp, NSM_QOS_DEFAULT_COS);

  if (ret < 0)
    {
      cli_out (cli, "%% CoS setting is failed\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}



CLI (mls_qos_dscp_cos,
     mls_qos_dscp_cos_cmd,
     "mls qos dscp-cos NAME",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify DSCP-to-COS",
     "Specify DSCP-to-COS map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_dscp_cos_list *dcosl;
  struct nsm_dscp_mut_list *dscpml = NULL;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  struct hal_dscp_map_table map_table[DSCP_TBL_SIZE];
  int ret;
  int i;
#endif /* HAVE_HAL */
  

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Lookup dscp-cos table */
  dcosl = nsm_dscp_cos_list_lookup (qosg.dcosl, argv[0]);
  if (! dcosl)
    {
      cli_out (cli, "%% Can't find a dscp-cos map (name %s)\n", argv[0]);
      return CLI_ERROR;
    }

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  
  /* Lookup dscp mutation table */
  if (qifl->dscp_mut_name[0] != '\0')
    {
      dscpml = nsm_dscp_mut_list_lookup (qosg.dscpml, qifl->dscp_mut_name);
      if (! dscpml)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

#ifdef HAVE_HAL
  for (i = 0; i < DSCP_TBL_SIZE; i++)
    {
            map_table[i].in_dscp = i;
            map_table[i].out_dscp = dscpml ? dscpml->d.dscp[i] : i;
            map_table[i].out_pri = dcosl->d.dscp[i];
    }

  /* Apply the map to the specified ingress port */
  ret = hal_qos_set_dscp_mapping_for_port(ifp->ifindex, 1, map_table, 
                                          DSCP_TBL_SIZE);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to set new DSCP-to-COS map to the interface\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pal_strcpy ( &qifl->dscp_cos_name[0], argv[0]);
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */

  return CLI_SUCCESS;
}


CLI (no_mls_qos_dscp_cos,
     no_mls_qos_dscp_cos_cmd,
     "no mls qos dscp-cos NAME",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify DSCP-to-COS",
     "Specify DSCP-to-DSCP map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_dscp_cos_list *dcosl;
  struct nsm_dscp_mut_list *dscpml = NULL;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  struct hal_dscp_map_table map_table[DSCP_TBL_SIZE];
  int ret;
  int i;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Lookup dscp-cos table */
  dcosl = nsm_dscp_cos_list_lookup (qosg.dcosl, argv[0]);
  if (! dcosl)
    {
      cli_out (cli, "%% Can't find a dscp-cos map (name %s)\n", argv[0]);
      return CLI_ERROR;
    }


  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  /* Lookup dscp mutation table */
  if (qifl->dscp_mut_name[0] != '\0')
    {
      dscpml = nsm_dscp_mut_list_lookup (qosg.dscpml, qifl->dscp_mut_name);
      if (! dscpml)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

#ifdef HAVE_HAL
  for (i = 0; i < DSCP_TBL_SIZE; i++)
    {
      map_table[i].in_dscp = i;
      map_table[i].out_dscp = dscpml ? dscpml->d.dscp[i] : i;
      map_table[i].out_pri = i/8;
    }

  /* Reset DSCP-to-COS map */
  ret = hal_qos_set_dscp_mapping_for_port(ifp->ifindex, 0, map_table, 
                                          DSCP_TBL_SIZE);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to clear DSCP-to-COS map\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pal_mem_set (&qifl->dscp_cos_name[0], 0, (INTERFACE_NAMSIZ + 1));

#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */


  return CLI_SUCCESS;
}



CLI (mls_qos_dscp_mutation,
     mls_qos_dscp_mutation_cmd,
     "mls qos dscp-mutation NAME",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify DSCP-to-DSCP mutation",
     "Specify the name of DSCP-to-DSCP mutaion map")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_dscp_mut_list *dscpml;
  struct nsm_dscp_cos_list *dcosl = NULL;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  struct hal_dscp_map_table map_table[DSCP_TBL_SIZE];
  int ret;
  int i, j;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (ifp->hw_type == IF_TYPE_VLAN)
    {
      cli_out (cli, "%% Dscp mutation  map cannot be applied to vlan interfaces\n");
      return CLI_ERROR;
    }

#ifdef HAVE_LACPD
  {
    struct nsm_if *zif;
    zif = ifp->info;
    if (zif->agg.type == NSM_IF_AGGREGATOR)
      {
        cli_out (cli, "%% Dscp mutation map cannot be applied to aggregator interfaces\n");
        return CLI_ERROR;
      }
  }
#endif /* HAVE_LACPD */

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error, failed to apply dscp-mutation to interface %s \n", ifp->name);
          return CLI_ERROR;
        }
    }

  if (qifl->input_pmap_name[0] != '\0')
    {
      cli_out (cli, "%% Dscp-mutation map cannot be applied as a service policy is already applied to this interface, \n");
      return CLI_ERROR;
    }

  /* Lookup dscp mutation table */
  dscpml = nsm_dscp_mut_list_lookup (qosg.dscpml, argv[0]);
  if (! dscpml)
    {
      cli_out (cli, "%% Can't find a dscp-mutation map (name %s)\n", argv[0]);
      return CLI_ERROR;
    }
  

  /* Lookup dscp-cos table */
  if (qifl->dscp_cos_name[0] != '\0')
    {
      dcosl = nsm_dscp_cos_list_lookup (qosg.dcosl, argv[0]);
      if (! dcosl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

#ifdef HAVE_HAL
  for (i = 0, j= 0; i < DSCP_TBL_SIZE; i++)
    {
      if (dscpml->d.dscp[i] != i)
        {
          map_table[j].in_dscp = i;
          map_table[j].out_dscp = dscpml->d.dscp[i];
          map_table[j].out_pri = dcosl ? dcosl->d.dscp[i] : i/8;
          j++;
        }
    }

  /* Apply the map to the specified ingress port */
  ret = hal_qos_set_dscp_mapping_for_port(ifp->ifindex, 0, map_table, j);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to set new DSCP mutation map to the interface\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pal_strcpy ( &qifl->dscp_mut_name[0], argv[0]);
#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */

  return CLI_SUCCESS;
}


CLI (no_mls_qos_dscp_mutation,
     no_mls_qos_dscp_mutation_cmd,
     "no mls qos dscp-mutation NAME",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify DSCP-to-DSCP mutation",
     "Specify the name of DSCP-to-DSCP mutaion map")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  struct nsm_dscp_mut_list *dscpml;
  struct nsm_dscp_cos_list *dcosl = NULL;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  struct hal_dscp_map_table map_table[DSCP_TBL_SIZE];
  int i, j;
  int ret;
#endif /* HAVE_HAL */


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Lookup dscp mutation table */
  dscpml = nsm_dscp_mut_list_lookup (qosg.dscpml, argv[0]);
  if (! dscpml)
    {
      cli_out (cli, "%% Can't find a dscp-mutation map (name %s)\n", argv[0]);
      return CLI_ERROR;
    }

  qifl = nsm_qif_list_lookup (qosg.qif, ifp->name);
  if (! qifl)
    {
      qifl = nsm_qif_list_get (qosg.qif, ifp->name);
      if (! qifl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

  /* Lookup dscp-cos table */
  if (qifl->dscp_cos_name[0] != '\0')
    {
      dcosl = nsm_dscp_cos_list_lookup (qosg.dcosl, argv[0]);
      if (! dcosl)
        {
          cli_out (cli, "%% Internal error \n");
          return CLI_ERROR;
        }
    }

#ifdef HAVE_HAL
  for (i = 0, j = 0; i < DSCP_TBL_SIZE; i++)
    {
      if (dscpml->d.dscp[i] != i)
      {
        map_table[j].in_dscp = i;
        map_table[j].out_dscp = i;
        map_table[j].out_pri = dcosl ? dcosl->d.dscp[i] : i/8;
        j++;
      }
    }

  /* Reset DSCP-to-DSCP mutation map */
  ret = hal_qos_set_dscp_mapping_for_port(ifp->ifindex, 0, map_table, j);
  if (ret < 0)
    {
      cli_out (cli, "%% Failed to clear DSCP mutation map\n");
      return CLI_ERROR;
    }
#endif /* HAVE_HAL */

  pal_mem_set (&qifl->dscp_mut_name[0], 0, (INTERFACE_NAMSIZ + 1));

#ifdef HAVE_HA
  nsm_qif_dscp_q_data_cal_modify(qifl);
#endif /* HAVE_HA */

  return CLI_SUCCESS;
}

/* CMAP_MODE (Class map) mode commands */
CLI (qos_cmap_match_access_group,
     qos_cmap_match_access_group_cmd,
     "match access-group NAME",
     "Define the match creteria",
     "List IP or MAC access contol lists",
     "Specify ACL list name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  struct qos_access_list *access;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (cmapl->match_type && cmapl->match_type != NSM_QOS_MATCH_TYPE_ACL)
    {
      cli_out (cli, "%% Only one match command is supported per class map \n");
      return CLI_ERROR;
    }

  access = qos_access_list_lookup (nm->acl, argv[0]);
  if (! access)
    {
      cli_out (cli, "%% No access-list: %s\n", argv[0]);
      return CLI_ERROR;
    }

  cmapl = nsm_qos_insert_acl_name_into_cmap (cmapl, argv[0]);
  if (! cmapl)
    {
      cli_out (cli, "%% Failed to attach ACL(%s) into class-map\n", argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (no_qos_cmap_match_access_group,
     no_qos_cmap_match_access_group_cmd,
     "no match access-group NAME",
     CLI_NO_STR,
     "Define the match creteria",
     "List IP or MAC access contol lists",
     "Specify ACL list name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  cmapl = nsm_qos_delete_acl_name_from_cmap (cmapl, argv[0]);
  if (! cmapl)
    {
      cli_out (cli, "%% Failed to detach ACL(%s) from class-map\n", argv[0]);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (qos_cmap_match_ip_dscp,
     qos_cmap_match_ip_dscp_cmd,
     "match ip-dscp (<0-63>|<0-63> <0-63>|<0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>|<0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63> <0-63>)",
     "Define the match creteria",
     "List IP DSCP values",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value",
     "Specify DSCP value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  u_int8_t d[8];
  int dscp;
  int i, j;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }
  
  if (cmapl->match_type && cmapl->match_type != NSM_QOS_MATCH_TYPE_DSCP)
    {
      cli_out (cli, "%% Only one match command is supported per class map \n");
      return CLI_ERROR;
    }

  switch (argc)
    {
    case 8:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[7], 0, 63);
      d[7] = (u_int8_t) dscp;
    case 7:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[6], 0, 63);
      d[6] = (u_int8_t) dscp;
    case 6:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[5], 0, 63);
      d[5] = (u_int8_t) dscp;
    case 5:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[4], 0, 63);
      d[4] = (u_int8_t) dscp;
    case 4:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[3], 0, 63);
      d[3] = (u_int8_t) dscp;
    case 3:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[2], 0, 63);
      d[2] = (u_int8_t) dscp;
    case 2:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[1], 0, 63);
      d[1] = (u_int8_t) dscp;
    case 1:
      CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[0], 0, 63);
      d[0] = (u_int8_t) dscp;
      break;
    default:
      cli_out (cli, "%% Invalid input \n");
      return CLI_ERROR;
    }

  for (i = 0; i < argc; i++)
    for (j = 0; j < argc; j++)
      {
        if (i != j && d[i] == d[j])
          {
            cli_out (cli, "%% Can't match two same dscp value \n");
            return CLI_ERROR;       
          }
      }

  nsm_qos_set_dscp_into_cmap (cmapl, argc, &d[0]);

  return CLI_SUCCESS;
}


CLI (no_qos_cmap_match_ip_dscp,
     no_qos_cmap_match_ip_dscp_cmd,
     "no match ip-dscp",
     CLI_NO_STR,
     "Define the match creteria",
     "IP DSCP")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (cmapl->match_type == NSM_QOS_MATCH_TYPE_DSCP)
    nsm_qos_delete_dscp_from_cmap (cmapl);
  else
    {
      cli_out (cli, "%% No ip-dscp match found for class-map %s\n",
               cmapl->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (qos_cmap_match_ip_precedence,
     qos_cmap_match_ip_precedence_cmd,
     "match ip-precedence (<0-7>|<0-7> <0-7>|<0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     "Define the match creteria",
     "List IP precedence vlaues",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value",
     "Specify IP precedence value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  u_int8_t p[8];
  int precedence;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (cmapl->match_type && cmapl->match_type != NSM_QOS_MATCH_TYPE_IP_PREC)
    {
      cli_out (cli, "%% Only one match command is supported per class map \n");
      return CLI_ERROR;
    }

  
  /* Check the range */
  switch (argc)
    {
    case 8:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[7], 0, 7);
      p[7] = (u_int8_t) precedence;
    case 7:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[6], 0, 7);
      p[6] = (u_int8_t) precedence;
    case 6:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[5], 0, 7);
      p[5] = (u_int8_t) precedence;
    case 5:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[4], 0, 7);
      p[4] = (u_int8_t) precedence;
    case 4:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[3], 0, 7);
      p[3] = (u_int8_t) precedence;
    case 3:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[2], 0, 7);
      p[2] = (u_int8_t) precedence;
    case 2:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[1], 0, 7);
      p[1] = (u_int8_t) precedence;
    case 1:
      CLI_GET_INTEGER_RANGE ("IP precedence value", precedence, argv[0], 0, 7);
      p[0] = (u_int8_t) precedence;
      break;
    default:
      cli_out (cli, "%% Invalid input\n");
      return CLI_ERROR;
    }

  nsm_qos_set_prec_into_cmap (cmapl, argc, &p[0]);

  return CLI_SUCCESS;
}


CLI (no_qos_cmap_match_ip_precedence,
     no_qos_cmap_match_ip_precedence_cmd,
     "no match ip-precedence <0-7>",
     CLI_NO_STR,
     "Define the match creteria",
     "List IP-Precedence list",
     "Specify IP precedence value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
     
  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }
  
  if (cmapl->match_type == NSM_QOS_MATCH_TYPE_IP_PREC)
    nsm_qos_delete_prec_from_cmap (cmapl);
  else
    {
      cli_out (cli, "%% No ip-precedence match found for class-map %s\n",
               cmapl->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (qos_cmap_match_exp,
     qos_cmap_match_exp_cmd,
     "match mpls exp-bit topmost (<0-7>|<0-7> <0-7>|<0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>|<0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>)",
     "Define the match criteria",
     "Specify Multi Protocol Label Switch specific values",
     "Specify MPLS experimental bits",
     "Match MPLS experimental value on topmost label",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value",
     "Specify experimental value")
{ 
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  u_int8_t e[8];
  int exp, i, j;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (cmapl->match_type && cmapl->match_type != NSM_QOS_MATCH_TYPE_EXP)
    {
      cli_out (cli, "%% Only one match command is supported per class map \n");
      return CLI_ERROR;
    }

  switch (argc)
    {
    case 8:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[7], 0, 7);
      e[7] = (u_int8_t) exp;
    case 7:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[6], 0, 7);
      e[6] = (u_int8_t) exp;
    case 6:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[5], 0, 7);
      e[5] = (u_int8_t) exp;
    case 5:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[4], 0, 7);
      e[4] = (u_int8_t) exp;
    case 4:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[3], 0, 7);
      e[3] = (u_int8_t) exp;
    case 3:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[2], 0, 7);
      e[2] = (u_int8_t) exp;
    case 2:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[1], 0, 7);
      e[1] = (u_int8_t) exp;
    case 1:
      CLI_GET_INTEGER_RANGE ("experimental value", exp, argv[0], 0, 7);
      e[0] = (u_int8_t) exp;
      break;
    default:
      cli_out (cli, "%% Invalid input \n");
      return CLI_ERROR;
    }

  for (i = 0; i < argc; i++)
    for (j = 0; j < argc; j++)
      {
        if (i != j && e[i] == e[j])
          {
            cli_out (cli, "%% Can't match two same EXP value \n");
            return CLI_ERROR;       
          }
      }

  nsm_qos_set_exp_into_cmap (cmapl, argc, &e[0]);
  /* Check the range */

  return CLI_SUCCESS;
}

CLI (no_qos_cmap_match_exp,
     no_qos_cmap_match_exp_cmd,
     "no match mpls exp-bit topmost",
     CLI_NO_STR,
     "Define the match criteria",
     "Specify Multi Protocol Label Switch specific values",
     "Specify MPLS experimental bits",
     "Match MPLS experimental value on topmost label")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (cmapl->match_type == NSM_QOS_MATCH_TYPE_EXP)
    nsm_qos_delete_exp_from_cmap (cmapl);
  else
    {
      cli_out (cli, "%% No mpls exp-bit match found for class-map %s\n",
               cmapl->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}
      

CLI (qos_cmap_match_l4_port,
     qos_cmap_match_l4_port_cmd,
     "match layer4 (source-port|destination-port) <1-65535>",
     "Define the match criteria",
     "Specify TCP/UDP port",
     "Specify source TCP/UDP port",
     "Specify destination TCP/UDP port",
     "TCP/UDP port value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  u_int32_t l4_port_id;
  u_char port_type = NSM_QOS_LAYER4_PORT_NONE;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (cmapl->match_type && cmapl->match_type != NSM_QOS_MATCH_TYPE_L4_PORT)
    {
      cli_out (cli, "%% Only one match command is supported per class map \n");
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("TCP/UDP port value", l4_port_id, argv[1], 0, 65535);

  if (argv[0][0] == 's')
    port_type = NSM_QOS_LAYER4_PORT_SRC;
  else
    port_type = NSM_QOS_LAYER4_PORT_DST;
  
  nsm_qos_set_layer4_port_into_cmap (cmapl, (u_int16_t)l4_port_id, port_type);

  return CLI_SUCCESS;
}


CLI (no_qos_cmap_match_l4_port,
     no_qos_cmap_match_l4_port_cmd,
     "no match layer4 (source-port|destination-port) <1-65535>",
     CLI_NO_STR,
     "Define the match criteria",
     "Specify TCP/UDP port",
     "Specify source TCP/UDP port",
     "Specify destination TCP/UDP port",
     "TCP/UDP port value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  u_int32_t l4_port_id;
  u_char port_type = NSM_QOS_LAYER4_PORT_NONE;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("TCP/UDP port value", l4_port_id, argv[1], 0, 65535);

  if (argv[0][0] == 's')
    port_type = NSM_QOS_LAYER4_PORT_SRC;
  else
    port_type = NSM_QOS_LAYER4_PORT_DST;

  if (cmapl->l4_port.port_type != port_type)
    {
      cli_out (cli, "%% Port type %d not configured!\n", port_type);
      return CLI_ERROR;
    }

  if (cmapl->l4_port.port_id != l4_port_id)
    {
      cli_out (cli, "%%  Port Id %d not configured!\n", l4_port_id);
      return CLI_ERROR;
    }
    
  nsm_qos_delete_layer4_port_from_cmap (cmapl, (u_int16_t)l4_port_id,
                                        port_type);

  return CLI_SUCCESS;
}

CLI (qos_cmap_match_vlan,
     qos_cmap_match_vlan_cmd,
     "match vlan <1-4094>",
     "Define the match creteria",
     "List VLAN ID",
     "Specify VLAN ID")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  u_int16_t v;
  int vid;
  int ret;
  int i;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if ((cmapl->v.num) >= MAX_NUM_OF_VLAN_FILTER)
    {
      cli_out (cli, "%% Class map cannot have more than %d vlan based filters\n", 
               MAX_NUM_OF_VLAN_FILTER);
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("vlan id", vid, argv[0], 1, 4094);
  v = (u_int16_t) vid;

  for (i = 0; i < cmapl->v.num; i++)
    {
      if (cmapl->v.vlan[i] == v)
        {
          cli_out (cli, "%% Vlan %d already configured for class-map %s \n", 
                   v, cmapl->name);
          return CLI_ERROR;
        }
    }

  ret =  nsm_qos_set_vid_into_cmap (cmapl, 1, &v);
  if (ret)
    {
      cli_out (cli, "%% Failed to set vid(%d) to class-map\n", v);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (qos_cmap_match_vlan_range,
     qos_cmap_match_vlan_range_cmd,
     "match vlan-range <1-4094> to <1-4094>",
     "Define the match creteria",
     "List VLAN ID's range",
     "Specify starting vlan ID",
     "Specify range",
     "Specify ending VLAN ID")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;
  u_int16_t vid[MAX_NUM_OF_VLAN_FILTER];
  int sub, start_vid, end_vid;
  int i, ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("Starting vlan id", start_vid, argv[0], 1, 4094);
  CLI_GET_INTEGER_RANGE ("Ending vlan id", end_vid, argv[1], 1, 4094);

  if (start_vid >= end_vid)
    {
      cli_out (cli, "%% Starting vid(%d) or ending vid(%d) is wrong\n", start_vid, end_vid);
      return CLI_ERROR;
    }

  sub = end_vid - start_vid;
  if ((cmapl->v.num + sub) >= MAX_NUM_OF_VLAN_FILTER)
    {
      cli_out (cli, "%% Class map cannot have more than %d vlan based filters\n", 
               MAX_NUM_OF_VLAN_FILTER);
      return CLI_ERROR;
    }

  for (i=0 ; i < (sub + 1) ; i++)
    vid[i] = (u_int16_t) (start_vid + i);

  ret =  nsm_qos_set_vid_into_cmap (cmapl, sub + 1, &vid[0]);
  if (ret)
    {
      cli_out (cli, "%% Failed to set start vid(%d) and end vid(%d)\n", start_vid, end_vid);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (no_qos_cmap_match_vlan,
     no_qos_cmap_match_vlan_cmd,
     "no match vlan",
     CLI_NO_STR,
     "Define the match creteria",
     "List VLAN ID")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  nsm_qos_delete_vid_from_cmap (cmapl);

  return CLI_SUCCESS;
}

/* PMAP_MODE (Policy Map) mode commands */
CLI (qos_pmap_class,
     qos_pmap_class_cmd,
     "class NAME",
     "Specify class-map",
     "Specify class map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl = cli->index;
  struct nsm_cmap_list *cmapl;

  if (! nm || (argv[0] == NULL))
    {
      cli_out (cli, "%% No nsm master address\n");
      return CLI_ERROR;
    }

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! pmapl)
    {
      cli_out (cli, "%% No policy-map list \n");
      return CLI_ERROR;
    }

  if (pmapl->attached > 0)
    {
      cli_out (cli, "%% Can't add or update class %s as policy-map is attached to some interfaces\n", argv[0]);
      return CLI_ERROR;
    }

  /* Find the class-map in all cmap lists */
  cmapl = nsm_qos_check_cmap_in_all_cmapls (nm->cmap, argv[0]);
  if (! cmapl)
    {
      cli_out (cli, "%% Can't get class-map(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  pmapl = nsm_pmap_list_lookup (nm->pmap, pmapl->name);

  if (! pmapl)
    {
      cli_out (cli, "%% No policy-map list\n");
      return CLI_ERROR;
    }

  /* Check class-map name from policy-map list */
  pmapl = nsm_qos_check_cl_name_in_pmapl (pmapl, argv[0]);
  if (! pmapl)
    {
      cli_out (cli, "%% Can't find class-map(%s) in this policy-map\n", argv[0]);
      return CLI_ERROR;
    }

  cli->index_sub = cmapl;
  cli->mode = PMAPC_MODE;

  return CLI_SUCCESS;
}


CLI (no_qos_pmap_class,
     no_qos_pmap_class_cmd,
     "no class NAME",
     CLI_NO_STR,
     "Specify class map",
     "Specify class map name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_pmap_list *pmapl = cli->index;
  struct nsm_cmap_list *cmapl;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check class-map name from policy-map list */
  cmapl = nsm_cmap_list_lookup (nm->cmap, argv[0]);
  if (! cmapl)
    {
      cli_out (cli, "%% Can't find class-map (%s)\n", argv[0]);
      return CLI_ERROR;
    }

  if (! pmapl)
    {
      cli_out (cli, "%% No policy-map list \n");
      return CLI_ERROR;
    }

  if (pmapl->attached > 0)
    {
      cli_out (cli, "%% Can't delete class %s from  policy-map because it is attached to some interfaces\n", argv[0]);
      return CLI_ERROR;
    }

  nsm_qos_delete_cmap_from_pmapl (pmapl, argv[0]);

  return CLI_SUCCESS;
}



/* PMAPC_MODE (Policy Map Class) mode commands */
CLI (qos_pmapc_trust,
     qos_pmapc_trust_cmd,
     "trust dscp",
     "Specify trust state for policy-map",
     "Trust DSCP")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  int ind, trust;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

#if 0
  if (pal_strcmp ("cos", argv[0]) == 0)
    ind = trust = NSM_QOS_TRUST_COS;
  else if (pal_strcmp ("dscp", argv[0]) == 0)
    ind = trust = NSM_QOS_TRUST_DSCP;
  else if (pal_strcmp ("ip-prec", argv[0]) == 0)
    ind = trust = NSM_QOS_TRUST_IP_PREC;
  else
    {
      cli_out (cli, "%% Fail to trust %s\n", argv[0]);
      return CLI_ERROR;
    }
#endif /* 0 */
  ind = trust = NSM_QOS_TRUST_DSCP;

  /* Check class-map name from policy-map list */
  nsm_qos_set_trust_into_cmap (cmapl, ind, trust);

  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_trust,
     no_qos_pmapc_trust_cmd,
     "no trust",
     CLI_NO_STR,
     "Specify trust mode for policy-map")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  /* Turn to the default port trust state */
  nsm_qos_set_trust_into_cmap (cmapl, 0, 0);

  return CLI_SUCCESS;
}

CLI (qos_pmapc_set_vlanpriority,
     qos_pmapc_set_vlanpriority_cmd,
     "set vlan-priority <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7> <0-7>",
     "Set Qos Parameters",
     "Set the priority for the queues",
     "Specify priority for queue 0",
     "Specify priority for queue 1",
     "Specify priority for queue 2",
     "Specify priority for queue 3",
     "Specify priority for queue 4",
     "Specify priority for queue 5",
     "Specify priority for queue 6",
     "Specify priority for queue 7")

{
  int iq0, iq1, iq2, iq3, iq4, iq5, iq6, iq7;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
 
  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Priority Value", iq0, argv[0], 0, 7);
  CLI_GET_INTEGER_RANGE ("Priority Value", iq1, argv[1], 0, 7);
  CLI_GET_INTEGER_RANGE ("Priority Value", iq2, argv[2], 0, 7);
  CLI_GET_INTEGER_RANGE ("Priority Value", iq3, argv[3], 0, 7);
  CLI_GET_INTEGER_RANGE ("Priority Value", iq4, argv[4], 0, 7);
  CLI_GET_INTEGER_RANGE ("Priority Value", iq5, argv[5], 0, 7);
  CLI_GET_INTEGER_RANGE ("Priority Value", iq6, argv[6], 0, 7);
  CLI_GET_INTEGER_RANGE ("Priority Value", iq7, argv[7], 0, 7);

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  cmapl->queue_priority.q0 = (char) iq0;
  cmapl->queue_priority.q1 = (char) iq1;
  cmapl->queue_priority.q2 = (char) iq2;
  cmapl->queue_priority.q3 = (char) iq3;
  cmapl->queue_priority.q4 = (char) iq4;
  cmapl->queue_priority.q5 = (char) iq5;
  cmapl->queue_priority.q6 = (char) iq6;
  cmapl->queue_priority.q7 = (char) iq7;    
 
  cmapl->vlan_priority_set = 1; 
  
  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_set_vlanpriority,
     no_qos_pmapc_set_vlanpriority_cmd,
     "no set vlan-priority",
     CLI_NO_STR,
     "Set Qos Parameters",
     "Set the priority for the queues")

{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  
  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  cmapl->queue_priority.q0 = (char) 0;
  cmapl->queue_priority.q1 = (char) 0;
  cmapl->queue_priority.q2 = (char) 0;
  cmapl->queue_priority.q3 = (char) 0;
  cmapl->queue_priority.q4 = (char) 0;
  cmapl->queue_priority.q5 = (char) 0;
  cmapl->queue_priority.q6 = (char) 0;
  cmapl->queue_priority.q7 = (char) 0;

  cmapl->vlan_priority_set = 0;

  return CLI_SUCCESS;
}

CLI (qos_pmapc_set_algorithm,
     qos_pmapc_set_algorithm_cmd,
     "set algorithm (strict|drr|drr-strict)",
     "Set Qos Parameters",
     "Set the algorithm for egress scheduling",
     "Strict algorithm",
     "drr algorithm",
     "drr-strict algorithm")

{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  if (pal_strncmp (argv[0], "s", 1) == 0)
    cmapl->scheduling_algo = NSM_CMAP_QOS_ALGORITHM_STRICT;
  else if(pal_strncmp (argv[0], "drr-", 4) == 0)
    cmapl->scheduling_algo = NSM_CMAP_QOS_ALGORITHM_DRR;
  else if(pal_strncmp (argv[0], "drr", 3) == 0)
    cmapl->scheduling_algo = NSM_CMAP_QOS_ALGORITHM_DRR_STRICT;
  else
   {
     cmapl->scheduling_algo_set = 0;
     cli_out (cli, "%% Error in setting the algotithm \n");
     return CLI_ERROR;
   }

  cmapl->scheduling_algo_set = 1;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif
  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_set_algorithm,
     no_qos_pmapc_set_algorithm_cmd,
     "no set algorithm",
     CLI_NO_STR,
     "Set Qos Parameters",
     "Set the algorithm for egress scheduling")

{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  cmapl->scheduling_algo = 0;
  cmapl->scheduling_algo_set = 0;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif
  return CLI_SUCCESS;
}

CLI (qos_pmapc_set_quantumpriority,
     qos_pmapc_set_quantumpriority_cmd,
     "set drr-priority <0-7> quantum <1-255>",
     "Set Qos Parameters",
     "DRR Priority",
     "DRR Priority Value",
     "DRR Quantum",
     "DRR Quantum Value")
{
  int priority;
  int quantum;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("Priority Value", priority, argv[0], 0, 7);

  CLI_GET_INTEGER_RANGE ("DRR Quantum Value", quantum, argv[1], 1, 255);

  cmapl->drr_priority = priority;
  cmapl->drr_quantum = quantum;
  cmapl->drr_set = 1;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif
  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_set_quantumpriority,
     no_qos_pmapc_set_quantumpriority_cmd,
     "no set drr-priority",
     CLI_NO_STR,
     "set Qos parameters",     
     "Setting the Priority and Quantum for DRR")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  cmapl->drr_priority = 0;
  cmapl->drr_quantum = 0;
  cmapl->drr_set = 0;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif
  return CLI_SUCCESS;

}

CLI (qos_pmapc_policing_meter,
     qos_pmapc_policing_meter_cmd,
     "policing meter <1-255>",
     "Specify a policer meter for the classified traffic",
     "Value is average traffic rate by burst rate",
     "Policing ratio")
{
  s_int32_t data_ratio;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("average rate between 1 to 255", 
                          data_ratio, argv[0], 1, 255);

  cmapl->policing_data_ratio = data_ratio;
  cmapl->policing_set = 1;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif
  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_policing_meter,
     no_qos_pmapc_policing_meter_cmd,
     "no policing meter",
     CLI_NO_STR,
     "Specify a policer for the classified traffic",
     "Value is average traffic by burst size")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }


  cmapl->policing_data_ratio = 0;
  cmapl->policing_set = 0;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif
  return CLI_SUCCESS;
}

CLI (qos_pmapc_set_cos,
     qos_pmapc_set_cos_cmd,
     "set cos (<0-7>|cos-inner)",
     "Setting a new value in the packet",
     "CoS",
     "Specify a new CoS value",
     "Specify CoS value to be copied from inner tag")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  int cos;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  if (cmapl->s.type != 0 &&
      cmapl->s.type != NSM_QOS_SET_COS)
    {
      cli_out (cli, "%% Only one set command allowed in a class which is a part of policy map \n");
      return CLI_ERROR;
    } 

  if (pal_strncmp (argv[0], "c", 1) == 0)
    cos = NSM_QOS_COS_INNER;
  else
    {
      /* Check the range */
      CLI_GET_INTEGER_RANGE ("CoS value", cos, argv[0], 0, 7);
    }

  /* Check class-map name from policy-map list */
  nsm_qos_set_val_into_cmap (cmapl, NSM_QOS_SET_COS, cos);

  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_set_cos,
     no_qos_pmapc_set_cos_cmd,
     "no set cos",
     CLI_NO_STR,
     "Setting a new value in the packet",
     "CoS")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  /* Check class-map name from policy-map list */
  if (cmapl->s.type == NSM_QOS_SET_COS)
    nsm_qos_set_val_into_cmap (cmapl, 0, 0);
  else
    {
      cli_out (cli, "%% cos not set for class-map %s\n", cmapl->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (qos_pmapc_set_ip_dscp,
     qos_pmapc_set_ip_dscp_cmd,
     "set ip-dscp <0-63>",
     "Setting a new value in the packet",
     "IP-DSCP",
     "Specify a new IP-DSCP value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  int dscp;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  if (cmapl->s.type != 0 && 
      cmapl->s.type !=  NSM_QOS_SET_DSCP)
    {
      cli_out (cli, "%% Only one set command allowed in a class which is a part of policy map \n");
      return CLI_ERROR;
    } 

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("DSCP value", dscp, argv[0], 0, 63);

  /* Check class-map name from policy-map list */
  nsm_qos_set_val_into_cmap (cmapl, NSM_QOS_SET_DSCP, dscp);

  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_set_ip_dscp,
     no_qos_pmapc_set_ip_dscp_cmd,
     "no set ip-dscp",
     CLI_NO_STR,
     "Setting a new value in the packet",
     "IP-DSCP")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  /* Check class-map name from policy-map list */
  if (cmapl->s.type == NSM_QOS_SET_DSCP)
    nsm_qos_set_val_into_cmap (cmapl, 0, 0);
  else
    {
      cli_out (cli, "%% ip-dscp not set for class-map %s\n", cmapl->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


CLI (qos_pmapc_set_ip_precedence,
     qos_pmapc_set_ip_precedence_cmd,
     "set ip-precedence <0-7>",
     "Setting a new value in the packet",
     "IP-Precedence",
     "Specify a new IP-Precedence value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  int prec;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  if (cmapl->s.type != 0 &&
      cmapl->s.type != NSM_QOS_SET_IP_PREC)
    {
      cli_out (cli, "%% Only one set command allowed in a class which is a part of policy map \n");
      return CLI_ERROR;
    } 

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("IP prec value", prec, argv[0], 0, 7);

  /* Check class-map name from policy-map list */
  nsm_qos_set_val_into_cmap (cmapl, NSM_QOS_SET_IP_PREC, prec);

  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_set_ip_precedence,
     no_qos_pmapc_set_ip_precedence_cmd,
     "no set ip-precedence",
     CLI_NO_STR,
     "Setting a new value in the packet",
     "IP-Precedence")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  /* Check class-map name from policy-map list */
  if (cmapl->s.type == NSM_QOS_SET_IP_PREC)
    nsm_qos_set_val_into_cmap (cmapl, 0, 0);
  else
    {
      cli_out (cli, "%% ip-precedence not set for class-map %s\n", cmapl->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (qos_pmapc_set_exp,
     qos_pmapc_set_exp_cmd,
     "set mpls exp-bit topmost <0-7>",
     "Setting a new value in the packet",
     "Multi Protocol Label Switch",
     "MPLS experimental bit",
     "Set MPLS experimental value on topmost label",
     "Specify experimental value")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  int exp;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }
 
  if (cmapl->s.type != 0 && 
      cmapl->s.type !=  NSM_QOS_SET_EXP)
    {
      cli_out (cli, "%% Only one set command allowed in a class which is a part of policy map \n");
      return CLI_ERROR;
    } 

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("EXP value", exp, argv[0], 0, 7);

  /* Check class-map name from policy-map list */
  nsm_qos_set_val_into_cmap (cmapl, NSM_QOS_SET_EXP, exp);

  return CLI_SUCCESS;
}

CLI (no_qos_pmapc_set_exp,
     no_qos_pmapc_set_exp_cmd,
     "no set mpls exp-bit topmost",
     CLI_NO_STR,
     "Setting a new value in the packet",
     "Multi Protocol Label Switch",
     "MPLS experimental bit",
     "Set MPLS experimental value on topmost label")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }
 
  /* Check class-map name from policy-map list */
  if (cmapl->s.type == NSM_QOS_SET_EXP)
    nsm_qos_set_val_into_cmap (cmapl, 0, 0);
  else
    {
      cli_out (cli, "%% mpls exp-bit topmost not set for class-map %s\n",
               cmapl->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (qos_pmapc_police,
     qos_pmapc_police_cmd,
     "police <1-1000000> <0-20000000> exceed-action drop",
     "Specify a policer for the classified traffic",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes)",
     "Specify the action if exceed-action",
     "Drop the packet")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  u_int32_t rate_bps, burst_byte, exd_act;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("average rate", rate_bps, argv[0], 1, 1000000);
  CLI_GET_UINT32_RANGE ("burst buffer size", burst_byte, argv[1], 0, 20000000);

  exd_act = NSM_QOS_EXD_ACT_DROP;

  nsm_qos_set_exd_act_into_cmap (cmapl, rate_bps, burst_byte,
                                 burst_byte, exd_act,
                                 NSM_QOS_FLOW_CONTROL_MODE_NONE);

  return CLI_SUCCESS;
}


CLI (no_qos_pmapc_police,
     no_qos_pmapc_police_cmd,
     "no police <1-1000000> <0-20000000> exceed-action drop",
     CLI_NO_STR,
     "Specify a policer for the classified traffic",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes)",
     "Specify the action if exceed-action",
     "Drop the packet")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  u_int32_t rate_bps, burst_byte, exd_act;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("average rate", rate_bps, argv[0], 1, 1000000);
  CLI_GET_UINT32_RANGE ("burst buffer size", burst_byte, argv[1], 0, 20000000);

  exd_act = NSM_QOS_EXD_ACT_DROP;
  
  nsm_qos_clear_exd_act_into_cmap (cmapl);

  return CLI_SUCCESS;
}


CLI (qos_pmapc_police_aggregate,
     qos_pmapc_police_aggregate_cmd,
     "police-aggregate NAME",
     "Specify a aggregate policer to multiple classes",
     "Specify a aggregate policer name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  struct nsm_agp_list *agpl;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  agpl = nsm_agp_list_lookup (qosg.agp, argv[0]);
  if (! agpl)
    {
      cli_out (cli, "%% Can't find aggregate-policer(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  /* Apply for policer into class-map */
  pal_strncpy(cmapl->agg_policer_name,argv[0],INTERFACE_NAMSIZ);
  cmapl->agg_policer_name_set = 1;
  cmapl->p.avg = agpl->p.avg;
  cmapl->p.burst = agpl->p.burst;
  cmapl->p.exd_act = agpl->p.exd_act;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}

CLI (no_qos_pmapc_police_aggregate,
     no_qos_pmapc_police_aggregate_cmd,
     "no police-aggregate NAME",
     CLI_NO_STR,
     "Specify a aggregate policer to multiple classes",
     "Specify a aggregate policer name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;
  struct nsm_agp_list *agpl;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl->agg_policer_name_set || 
      pal_strncmp (argv[0], cmapl->agg_policer_name,INTERFACE_NAMSIZ) != 0)
    {
      cli_out (cli, "%% Can't find aggregate-policer(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  agpl = nsm_agp_list_lookup (qosg.agp, argv[0]);
  if (! agpl)
    {
      cli_out (cli, "%% Can't find aggregate-policer(%s)\n", argv[0]);
      return CLI_ERROR;
    }

  /* Apply for policer into class-map */
  pal_mem_set (cmapl->agg_policer_name,'\0',INTERFACE_NAMSIZ); 
  cmapl->agg_policer_name_set = 0;
  cmapl->p.avg = 0;
  cmapl->p.burst = 0;
  cmapl->p.exd_act = 0;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif /*HAVE_HA*/

  return CLI_SUCCESS;
}

CLI (mls_qos_wrr_weight_queue,
     mls_qos_wrr_weight_queue_cmd,
     "mls qos wrr-weight <0-3> <1-32>",
      NSM_MLS_STR,
      NSM_QOS_STR,
     "Wrr weight",
     "Configure the queue id",
     "Specify a weight")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid, weight;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 7);
  CLI_GET_INTEGER_RANGE ("weight", weight, argv[1], 1, 32);


  nsm_mls_qos_set_queue_weight (&qosg, qid, weight);

  return CLI_SUCCESS;
}

CLI (no_mls_qos_wrr_weight_queue,
     no_mls_qos_wrr_weight_queue_cmd,
     "no mls qos wrr-weight <0-3>",
      NSM_MLS_STR,
      NSM_QOS_STR,
     "Wrr weight",
     "Configure the queue id",
     "Specify a weight")
{
  struct nsm_master *nm = cli->vr->proto;
  int qid;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range */
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[0], 0, 3);

  nsm_mls_qos_unset_queue_weight (&qosg, qid);

  return CLI_SUCCESS;
}

CLI (mls_qos_map_dscp_queue,
     mls_qos_map_dscp_queue_cmd,
     "mls qos dscp-queue <0-64> <0-3>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "DSCP-to-QUEUE",
     "Select dscp_value <0-63>",
     "Select queue id")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t qid;
  u_int32_t dscp;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("dscp value", dscp, argv[0], 0, 63);
  CLI_GET_UINT32_RANGE ("queue id", qid, argv[1], 0, 3);

  nsm_mls_qos_set_dscp_to_queue (&qosg, dscp, qid);
        
  return CLI_SUCCESS;
}


CLI (no_mls_qos_map_dscp_queue,
     no_mls_qos_map_dscp_queue_cmd,
     "no mls qos dscp-queue <0-63> <0-3>",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "DSCP-to-QUEUE",
     "Select dscp_value <0-64>",
     "Select queue id")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t qid;
  s_int32_t dscp;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("dscp value", dscp, argv[0], 0, 63);
  CLI_GET_UINT32_RANGE ("queue id", qid, argv[1], 0, 3);

  nsm_mls_qos_unset_dscp_to_queue (&qosg, dscp);        
  
  return CLI_SUCCESS;
}

CLI (mls_qos_map_cos_queue,
     mls_qos_map_cos_queue_cmd,
     "mls qos cos-queue <0-7> <0-3>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify cos-queue",
     "Specify Cos value with priority <0-7>",
     "Specify queue id <0-3>")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t cos_val;
  u_int32_t qid;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range of COS vlaue */
   CLI_GET_UINT32_RANGE ("cos value", cos_val, argv[0], 0, 7);
   CLI_GET_UINT32_RANGE ("queue id", qid, argv[1], 0, 7);

   nsm_mls_qos_set_cos_to_queue (&qosg, cos_val, qid);

   return CLI_SUCCESS;
}

CLI (no_mls_qos_map_cos_queue,
     no_mls_qos_map_cos_queue_cmd,
     "no mls qos cos-queue <0-7> <0-3>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify cos-queue",
     "Specify Cos value with priority <0-7>",
     "Specify queue id")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t cos_val;
  u_int32_t qid;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  /* Check the range of COS vlaue */
  CLI_GET_UINT32_RANGE ("cos value", cos_val, argv[0], 0, 7);
  CLI_GET_UINT32_RANGE ("queue id", qid, argv[1], 0, 3);

  nsm_mls_qos_unset_cos_to_queue (&qosg, cos_val);

  return CLI_SUCCESS;
}

CLI (mls_qos_vlan_priority,
     mls_qos_vlan_priority_cmd,
     "mls qos vlan-priority-override (bridge <1-32> |) VLANID <0-7>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Vlan Priority Override",
     "Bridge group commands",
     "Bridge group name",
     "Select vlan id <1-4094>",
     "Select priority <0-7>")
{
  struct nsm_master *nm = cli->vr->proto;
  char *bridge_name;
  int vid, priority;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (argc == 4)
    {
      bridge_name = argv [1];
      CLI_GET_INTEGER_RANGE ("vlan id", vid, argv[2], 1, 4094);
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[3], 0,7);
    }
  else
    {
      bridge_name = NULL;
      CLI_GET_INTEGER_RANGE ("vlan id", vid, argv[0], 1, 4094);
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[1], 0,7);
    }
  
  nsm_mls_qos_vlan_priority_set (nm, bridge_name, vid, priority);
  
  return CLI_SUCCESS;
}

CLI (no_mls_qos_vlan_priority,
     no_mls_qos_vlan_priority_cmd,
     "no mls qos vlan-priority-override (bridge <1-32> |) VLANID <0-7>",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Vlan Priority Override",
     "Bridge group commands",
     "Bridge group name",
     "Select vlan id <1-4094>",
     "Select priority <0-7>")
{

  struct nsm_master *nm = cli->vr->proto;
  char *bridge_name;
  int vid, priority;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (argc == 4)
    {
      bridge_name = argv [1];
      CLI_GET_INTEGER_RANGE ("vlan id", vid, argv[2], 1, 4094);
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[3], 0,7);
    }
  else
    {
      bridge_name = NULL;
      CLI_GET_INTEGER_RANGE ("vlan id", vid, argv[0], 1, 4094);
      CLI_GET_INTEGER_RANGE ("priority", priority, argv[1], 0,7);
    }

  nsm_mls_qos_vlan_priority_unset (nm, bridge_name, vid);

  return CLI_SUCCESS;

}

CLI (mls_qos_vlan_priority_cos_queue,
     mls_qos_vlan_priority_cos_queue_cmd,
     "mls qos vlan-priority-override (cos | queue | both)",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Vlan Priority Override",
     "Select COS",
     "Select QUEUE",
     "Select both")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  enum nsm_qos_vlan_pri_override port_mode;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }
 
  if (! pal_strcmp("cos",argv[0]))
     port_mode = NSM_QOS_VLAN_PRI_OVERRIDE_COS; 
  else if (! pal_strcmp("queue",argv[0]))
     port_mode = NSM_QOS_VLAN_PRI_OVERRIDE_QUEUE; 
  else if (! pal_strcmp("both",argv[0]))
     port_mode = NSM_QOS_VLAN_PRI_OVERRIDE_BOTH; 
  else   
     port_mode = NSM_QOS_VLAN_PRI_OVERRIDE_NONE; 

  nsm_qos_set_port_vlan_priority_override (&qosg, ifp, port_mode);
  
  return CLI_SUCCESS;
}

CLI (no_mls_qos_vlan_priority_cos_queue,
     no_mls_qos_vlan_priority_cos_queue_cmd,
     "no mls qos vlan-priority-override",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Vlan Priority Override")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }
 
  nsm_qos_set_port_vlan_priority_override (&qosg, ifp,
                                           NSM_QOS_VLAN_PRI_OVERRIDE_NONE);
  
  return CLI_SUCCESS;

}

CLI (mls_qos_da_priority_cos_queue,
     mls_qos_da_priority_cos_queue_cmd,
     "mls qos da-priority-override (cos | queue | both)",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Destination Address Priority Override",
     "Select COS",
     "Select QUEUE",
     "Select both")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  enum nsm_qos_da_pri_override port_mode;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }
 
  if (! pal_strcmp("cos",argv[0]))
     port_mode = NSM_QOS_DA_PRI_OVERRIDE_COS; 
  else if (! pal_strcmp("queue",argv[0]))
     port_mode = NSM_QOS_DA_PRI_OVERRIDE_QUEUE; 
  else if (! pal_strcmp("both",argv[0]))
     port_mode = NSM_QOS_DA_PRI_OVERRIDE_BOTH; 
  else   
     port_mode = NSM_QOS_DA_PRI_OVERRIDE_NONE; 

  nsm_qos_set_port_da_priority_override (&qosg, ifp, port_mode);
  
  return CLI_SUCCESS;
}

CLI (no_mls_qos_da_priority_cos_queue,
     no_mls_qos_da_priority_cos_queue_cmd,
     "no mls qos da-priority-override",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Destination Address Priority Override")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }
 
  nsm_qos_set_port_da_priority_override (&qosg, ifp,
                                         NSM_QOS_DA_PRI_OVERRIDE_NONE);
  
  return CLI_SUCCESS;

}


CLI (mls_qos_frame_type_priority,
     mls_qos_frame_type_priority_cmd,
     "mls qos frame-type-priority-override "
     "(bpdu-to-cpu | ucast-mgmt-to-cpu |  from-cpu | port-etype-match) <0-3>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify frame type priority",
     "Specify bpdu-to-cpu",
     "Specify ucast-mgmt-to-cpu",
     "Specify from-cpu",
     "Specify port-etype-match",
     "Specify QUEUE_ID")
{
  struct nsm_master *nm = cli->vr->proto;
  enum nsm_qos_frame_type ftype;
  int qid;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }
  
  if(! pal_strcmp("bpdu-to-cpu",argv[0]))
     ftype = NSM_QOS_FTYPE_DSA_TO_CPU_BPDU;
   else if(! pal_strcmp("ucast-mgmt-to-cpu",argv[0]))
     ftype = NSM_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT;
   else if(! pal_strcmp("from-cpu",argv[0]))
     ftype = NSM_QOS_FTYPE_DSA_FROM_CPU;
   else if(! pal_strcmp("port-etype-match",argv[0]))
     ftype = NSM_QOS_FTYPE_PORT_ETYPE_MATCH;
   else
     {
       cli_out (cli, "%% unknown frame type!\n");
       return CLI_ERROR;
     }
  
  CLI_GET_INTEGER_RANGE ("queue id", qid, argv[1], 0, 3);
  
  nsm_qos_set_frame_type_priority_override (&qosg, ftype, qid);

  return CLI_SUCCESS;
}

CLI (no_mls_qos_frame_type_priority,
     no_mls_qos_frame_type_priority_cmd,
     "no mls qos frame-type-priority-override (bpdu-to-cpu |"
     "ucast-mgmt-to-cpu | from-cpu | port-etype-match)",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Specify frame type priority",
     "Specify bpdu-to-cpu",
     "Specify ucast-mgmt-to-cpu",
     "Specify from-cpu",
     "Specify port-etype-match",
     "Specify QUEUE_ID")
{
  struct nsm_master *nm = cli->vr->proto;
  enum nsm_qos_frame_type ftype;

  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if(! pal_strcmp("bpdu-to-cpu",argv[0]))
     ftype = NSM_QOS_FTYPE_DSA_TO_CPU_BPDU;
   else if(! pal_strcmp("ucast-mgmt-to-cpu",argv[0]))
     ftype = NSM_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT;
   else if(! pal_strcmp("from-cpu",argv[0]))
     ftype = NSM_QOS_FTYPE_DSA_FROM_CPU;
   else if(! pal_strcmp("port-etype-match",argv[0]))
     ftype = NSM_QOS_FTYPE_PORT_ETYPE_MATCH;
   else
     {
       cli_out (cli, "%% unknown frame type!\n");
       return CLI_ERROR;
     }

  nsm_qos_unset_frame_type_priority_override (&qosg, ftype);

  return CLI_SUCCESS;
}

CLI (mls_qos_trust_dscp_cos,
     mls_qos_trust_dscp_cos_cmd,
     "mls qos trust (dscp | cos | both)",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values",
     "Classifies ingress packets with the packet COS values",
     "Classifies ingress packets with the packet BOTH values")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  u_int8_t trust_state; 

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! pal_strcmp("cos",argv[0]))
    trust_state = NSM_QOS_TRUST_COS;
  else if (! pal_strcmp("dscp",argv[0]))
    trust_state = NSM_QOS_TRUST_DSCP;
  else if (! pal_strcmp("both",argv[0]))
    trust_state = NSM_QOS_TRUST_DSCP_COS;
  else
    trust_state = NSM_QOS_TRUST_NONE;

  nsm_qos_set_trust_state (&qosg, ifp, trust_state);

  return CLI_SUCCESS;
}

CLI (no_mls_qos_trust_dscp_cos,
     no_mls_qos_trust_dscp_cos_cmd,
     "no mls qos trust (dscp | cos | both)",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Confiure port trust state",
     "Classifies ingress packets with the packet DSCP values",
     "Classifies ingress packets with the packet COS values",
     "Classifies ingress packets with the packet BOTH values")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }
  
  nsm_qos_set_trust_state (&qosg, ifp, NSM_QOS_TRUST_NONE);

  return CLI_SUCCESS;
}

CLI (mls_qos_force_trust_cos,
     mls_qos_force_trust_cos_cmd,
     "mls qos force-trust cos", 
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Confiure port trust state", 
     "Classifies ingress packets with the packet cos values")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  nsm_qos_set_force_trust_cos (&qosg, ifp, PAL_TRUE);

  return CLI_SUCCESS;
}

CLI (no_mls_qos_force_trust_cos,
     no_mls_qos_force_trust_cos_cmd,
     "no mls qos force-trust cos",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Confiure force trust state",
     "Classifies ingress packets with the packet cos values")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  nsm_qos_set_force_trust_cos (&qosg, ifp, PAL_FALSE);
  
  return CLI_SUCCESS;
}

u_int32_t
nsm_qos_str_to_traffic_type (char *traffic_type_str)
{
  u_int32_t traffic_type;

  if (! pal_strcmp("unknown-unicast", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_UNKNOWN_UNICAST;
  else if (! pal_strcmp("unknown-multicast", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_UNKNOWN_MULTICAST;
  else if(! pal_strcmp("broadcast", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_BROADCAST;
  else if(! pal_strcmp("multicast", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_MULTICAST;
  else if(! pal_strcmp("unicast", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_UNICAST;
  else if(! pal_strcmp("management", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_MGMT;
  else if(! pal_strcmp("arp", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_ARP;
  else if(! pal_strcmp("tcp-data", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_TCP_DATA;
  else if(! pal_strcmp("tcp-control", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_TCP_CTRL;
  else if(! pal_strcmp("udp", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_UDP;
  else if(! pal_strcmp("non-tcp-udp", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_NON_TCP_UDP;
  else if(! pal_strcmp("queue0", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE0;
  else if(! pal_strcmp("queue1", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE1;
  else if(! pal_strcmp("queue2", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE2;
  else if(! pal_strcmp("queue3", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE3;
  else if(! pal_strcmp("all", traffic_type_str))
    traffic_type = NSM_QOS_TRAFFIC_ALL;
  else
    traffic_type = NSM_QOS_TRAFFIC_NONE;

  return traffic_type;
  
}

CLI (qos_cmap_match_traffic_type,
     qos_cmap_match_traffic_type_cmd,
     "match traffic-type (unknown-unicast | "
     "unknown-multicast | broadcast | multicast "
     "| unicast | management | arp | tcp-data | "
     "tcp-control | udp | non-tcp-udp | queue0 | "
     "queue1 | queue2 | queue3 | all |) "
     "(traffic-type-and-queue | traffic-type-or-queue|)",
     "Define the match criteria",
     "Specify Traffic Type",
     "Specify unknown unicast frames",
     "Specify unknown multicast frames",
     "Specify broadcast frames",
     "Specify multicast frames",
     "Specify unicast frames",
     "Specify management frames",
     "Specify ARP frames",
     "Specify TCP data frames",
     "Specify TCP control frames",
     "Specify UDP frames",
     "Specify non TCP/UDP frames",
     "Specify queue0",
     "Specify queue1",
     "Specify queue2",
     "Specify queue3",
     "Specify all",
     "Specify traffic-type-and-queue",
     "Specify traffic-type-or-queue (default)")
{
  struct nsm_cmap_list *cmapl = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  enum nsm_qos_traffic_match criteria;
  u_int32_t traffic_type;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (argc == 0)
    {
      cli_out (cli, "%% Invalid Input: Need to specify atleast traffic type or"
                    " match criteria\n");
      return CLI_ERROR;
    }

  if (argc == 2)
    {

      if (pal_strcmp("all", argv [0]) == 0)
        {
          cli_out (cli, "%% Cannot specify match criteria with "
                        "match traffic all\n\n");
          return CLI_ERROR;
        }

      traffic_type = nsm_qos_str_to_traffic_type (argv [0]);

      if (traffic_type == NSM_QOS_TRAFFIC_NONE)
        {
          cli_out (cli, "%% No traffic type \n");
          return CLI_ERROR;
        }

      if (! pal_strcmp("traffic-type-and-queue",argv[1]))
        criteria = NSM_QOS_CRITERIA_TTYPE_AND_PRI;
      else if(! pal_strcmp("traffic-type-or-queue",argv[1]))
        criteria = NSM_QOS_CRITERIA_TTYPE_OR_PRI;
      else
        {
          cli_out (cli, "%% No traffic match \n");
          return CLI_ERROR;
        }
    }
  else
    {
      if (! pal_strcmp("traffic-type-and-queue",argv[0]))
        {
          if (cmapl->traffic_type.custom_traffic_type == NSM_QOS_TRAFFIC_ALL)
            {
              cli_out (cli, "%% Cannot specify match criteria with "
                            "match traffic all\n\n");
              return CLI_ERROR;
            }

          traffic_type = NSM_QOS_TRAFFIC_NONE;
          criteria = NSM_QOS_CRITERIA_TTYPE_AND_PRI;
        }
      else if(! pal_strcmp("traffic-type-or-queue",argv[0]))
        {
          if (cmapl->traffic_type.custom_traffic_type == NSM_QOS_TRAFFIC_ALL)
            {
              cli_out (cli, "%% Cannot specify match criteria with "
                            "match traffic all\n\n");
              return CLI_ERROR;
            }

          traffic_type = NSM_QOS_TRAFFIC_NONE;
          criteria = NSM_QOS_CRITERIA_TTYPE_OR_PRI;
        }
      else
        {
          traffic_type = nsm_qos_str_to_traffic_type (argv [0]);

          if (traffic_type == NSM_QOS_TRAFFIC_NONE)
            {
              cli_out (cli, "%% No traffic type \n");
              return CLI_ERROR;
            }

          criteria = NSM_QOS_CRITERIA_TTYPE_OR_PRI;
        }
    }
 
  nsm_qos_cmap_match_traffic_set (nm, cmapl->name, traffic_type, criteria);

  return CLI_SUCCESS;
        
}

CLI (no_qos_cmap_match_traffic_type,
     no_qos_cmap_match_traffic_type_cmd,
     "no match traffic-type",
     CLI_NO_STR,
     "Define the match criteria",
     "Specify Traffic Type")
{
  struct nsm_cmap_list *cmapl = cli->index;
  struct nsm_master *nm = cli->vr->proto;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (cmapl == NULL)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }
 
  nsm_qos_cmap_match_unset_traffic (nm, cmapl->name); 
  
  return CLI_SUCCESS;
        
}

CLI (no_qos_cmap_match_traffic_type1,
     no_qos_cmap_match_traffic_type_cmd1,
     "no match traffic-type (unknown-unicast | "
     "unknown-multicast | broadcast | multicast "
     "| unicast | management | arp | tcp-data | "
     "tcp-control | udp | non-tcp-udp | queue0 | "
     "queue1 | queue2 | queue3 | all) ",
     CLI_NO_STR,
     "Define the match criteria",
     "Specify Traffic Type",
     "Specify unknown unicast frames",
     "Specify unknown multicast frames",
     "Specify broadcast frames",
     "Specify multicast frames",
     "Specify unicast frames",
     "Specify management frames",
     "Specify ARP frames",
     "Specify TCP data frames",
     "Specify TCP control frames",
     "Specify UDP frames",
     "Specify non TCP/UDP frames",
     "Specify queue0",
     "Specify queue1",
     "Specify queue2",
     "Specify queue3",
     "Specify all")
{
  struct nsm_cmap_list *cmapl = cli->index;
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t traffic_type;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (!cmapl)
    {
      cli_out (cli, "%% No class-map list address\n");
      return CLI_ERROR;
    }

  if (! pal_strcmp("unknown-unicast",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_UNKNOWN_UNICAST;
  else if (! pal_strcmp("unknown-multicast",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_UNKNOWN_MULTICAST;
  else if(! pal_strcmp("broadcast",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_BROADCAST;
  else if(! pal_strcmp("multicast",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_MULTICAST;
  else if(! pal_strcmp("unicast",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_UNICAST;
  else if(! pal_strcmp("management",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_MGMT;
  else if(! pal_strcmp("arp",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_ARP;
  else if(! pal_strcmp("tcp-data",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_TCP_DATA;
  else if(! pal_strcmp("tcp-control",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_TCP_CTRL;
  else if(! pal_strcmp("udp",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_UDP;
  else if(! pal_strcmp("non-tcp-udp",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_NON_TCP_UDP;
  else if(! pal_strcmp("queue0",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE0;
  else if(! pal_strcmp("queue1",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE1;
  else if(! pal_strcmp("queue2",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE2;
  else if(! pal_strcmp("queue3",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_QUEUE3;
  else if(! pal_strcmp("all",argv[0]))
    traffic_type = NSM_QOS_TRAFFIC_ALL;
  else
    {
      cli_out (cli, "%% No traffic type \n");
      return CLI_ERROR;
    }
        
  nsm_qos_cmap_match_traffic_unset (nm, cmapl->name, traffic_type);

  return CLI_SUCCESS;
        
}

CLI (qos_pmapc_police_rate,
     qos_pmapc_police_rate_cmd,
     "police <1-1000000> <0-20000000> <1-20000000> exceed-action "
     "(drop | flow-control) reset-flow-control-mode available-bucket-room "
     "(full | cbs)",
     "Specify a policer for the classified rate",
     "Specify an average traffic rate (kbps)",
     "Specify a burst size in (bytes) EBS <0-20000000> ", 
     "Specify an exceed burst size in (bytes) EBS <1-20000000> ", 
     "Specify action if exceed-action",
     "Drop the frame",
     "Send a pause frame and pass packet",
     "Specify to generate flow control",
     "Specify when to deassert flow control",
     "De-assert flow when bucket room is full",
     "De-assert flow when bucket has enough room")
{
  struct nsm_cmap_list *cmapl = cli->index_sub;
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t rate_bps, burst, exceed_burst;
  enum nsm_qos_flow_control_mode fc_mode;
  enum nsm_exceed_action exc_act;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("average rate", rate_bps, argv[0], 1, 1000000);
  CLI_GET_UINT32_RANGE ("exceed burst size", burst,
                         argv[1], 0, 20000000);
  CLI_GET_INTEGER_RANGE ("exceed burst size", exceed_burst,
                         argv[2], 1, 20000000);

  if (! pal_strcmp("drop",argv[3]))
     exc_act = NSM_QOS_EXD_ACT_DROP;
  else if (! pal_strcmp("flow-control",argv[3])) 
     exc_act = NSM_QOS_EXD_ACT_POLICE_FLOW_CONTROL;
  else
    {
      cli_out (cli, "%% No exceed ation \n");
      return CLI_ERROR;
    } 

  if (! pal_strcmp("full",argv[4]))
    fc_mode = NSM_RESUME_ON_FULL_BUCKET_SIZE;
  else if (! pal_strcmp("cbs",argv[4]))
    fc_mode = NSM_RESUME_ON_CBS_BUCKET_SIZE;
  else
    { 
      cli_out (cli, "%% No flow control \n");
      return CLI_ERROR;
    }

  nsm_qos_set_exd_act_into_cmap (cmapl, rate_bps, burst,
                                 exceed_burst, exc_act, fc_mode);

  return CLI_SUCCESS;
}

CLI (no_qos_pmapc_police_rate,
     no_qos_pmapc_police_rate_cmd,
     "no police <1-1000000> <0-20000000> <1-20000000> exceed-action "
     "(drop | flow-control) reset-flow-control-mode available-bucket-room "
     "(full | cbs)",
     CLI_NO_STR,
     "Specify a policer for the classified rate",
     "Specify an average traffic rate (kbps)",
     "Specify a normal burst size (bytes) CBS <0-20000000>",
     "Specify an exceed burst size in (bytes) EBS <1-20000000> ", 
     "Specify action if exceed-action",
     "Drop the frame",
     "Send a pause frame and pass packet",
     "Specify to generate flow control",
     "De-assert flow when bucket room is full",
     "De-assert flow when bucket has enough room")
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_cmap_list *cmapl = cli->index_sub;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (! cmapl)
    {
      cli_out (cli, "%% No class-map list \n");
      return CLI_ERROR;
    }

  nsm_qos_clear_exd_act_into_cmap (cmapl);

  return CLI_SUCCESS;
}

CLI (mls_qos_egress_rate_shape,
     mls_qos_egress_rate_shape_cmd,
     "traffic-shape rate RATE (kbps|fps)",
     "Configure Traffic Shaping",
     "Configure the Traffic rate shape",
     "Traffic rate shape",
     "Kilo Bits Per Second",
     "Frames Per Second")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  enum nsm_qos_rate_unit rate_unit;
  u_int32_t rate;
  s_int32_t ret;


  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_INTEGER_RANGE ("average rate", rate, argv[0], 1, 1000000);

  rate_unit = NSM_QOS_RATE_KBPS;

  if (! pal_strcmp("kbps",argv[1]))
    rate_unit = NSM_QOS_RATE_KBPS;
  else if (! pal_strcmp("fps",argv[1]))
    rate_unit = NSM_QOS_RATE_FPS;

  ret = nsm_qos_set_egress_rate_shaping (&qosg, ifp, rate, rate_unit);

  if (ret != NSM_QOS_SUCCESS)
    {
      cli_out (cli, "%% Failed to set traffic shaping \n");
      return CLI_ERROR;
    }


  return CLI_SUCCESS;
}

CLI (no_mls_qos_egress_rate_shape,
     no_mls_qos_egress_rate_shape_cmd,
     "no traffic-shape",
     CLI_NO_STR,
     "Configure Traffic Shaping")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  s_int32_t ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  ret = nsm_qos_set_egress_rate_shaping (&qosg, ifp, NSM_QOS_NO_TRAFFIC_SHAPE,
                                         NSM_QOS_RATE_KBPS);

  if (ret != NSM_QOS_SUCCESS)
    {
      cli_out (cli, "%% Failed to set traffic shaping \n");
      return CLI_ERROR;
    }


  return CLI_SUCCESS;
}

CLI (mls_qos_strict_queue,
     mls_qos_strict_queue_cmd,
     "mls qos strict queue <0-3>",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Configure the Queue as Strict Queue",
     "The Queue which is configured as Strict Queue",
     "The Queue ID of Strict Queue")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  u_int32_t qid;
  s_int32_t ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("queue id", qid, argv[0], 0, 3);

  if (qid == 0)
    qid = NSM_QOS_STRICT_QUEUE0;

  ret = nsm_qos_set_strict_queue (&qosg, ifp, qid);

  if (ret != NSM_QOS_SUCCESS)
    {
      cli_out (cli, "%% Failed to set Strict Queue\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (no_mls_qos_strict_queue,
     no_mls_qos_strict_queue_cmd,
     "no mls qos strict queue <0-3>",
     CLI_NO_STR,
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Configure the Queue as WRR",
     "The Queue which is configured as WRR Queue",
     "The Queue ID of WRR Queue")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  u_int32_t qid;
  s_int32_t ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  CLI_GET_UINT32_RANGE ("queue id", qid, argv[0], 0, 3);

  ret = nsm_qos_unset_strict_queue (&qosg, ifp, qid);

  if (ret != NSM_QOS_SUCCESS)
    {
      cli_out (cli, "%% Failed to set Strict Queue\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

CLI (mls_qos_strict_queue_all,
     mls_qos_strict_queue_all_cmd,
     "mls qos strict queue (all|none)",
     NSM_MLS_STR,
     NSM_QOS_STR,
     "Configure the Queue as Strict Queue",
     "The Queue which is configured as Strict Queue",
     "Configure All Queues to be Strict",
     "Configure All Queues to be WRR")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp = cli->index;
  u_int32_t qid;
  s_int32_t ret;

  /* Check QoS state */
  if (nsm_qos_chk_if_qos_enabled (nm->pmap))
    {
      cli_out (cli, "%% QoS is not enabled globally!\n");
      return CLI_ERROR;
    }

  if (argv [0][0] == 'a')
    qid = NSM_QOS_STRICT_QUEUE_ALL;
  else
    qid = NSM_QOS_STRICT_QUEUE_NONE;

  ret = nsm_qos_set_strict_queue (&qosg, ifp, qid);

  if (ret != NSM_QOS_SUCCESS)
    {
      cli_out (cli, "%% Failed to set Strict Queue\n");
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* CLI Initialization. */
void
nsm_qos_cli_init (struct cli_tree *ctree)
{
  cli_install_config (ctree, QOS_MODE, nsm_qos_config_write);
  cli_install_default (ctree, QOS_MODE);
  cli_install_config (ctree, CMAP_MODE, nsm_qos_cmap_config_write);
  cli_install_default (ctree, CMAP_MODE);
  cli_install_config (ctree, PMAP_MODE, nsm_qos_pmap_config_write);
  cli_install_default (ctree, PMAP_MODE);
  cli_install_config (ctree, PMAPC_MODE, nsm_qos_pmapc_config_write);
  cli_install_default (ctree, PMAPC_MODE);


  /* EXEC mode commands. */
#if 0 

  cli_install (ctree, EXEC_MODE, &show_mls_qos_cmd);
  cli_install (ctree, EXEC_MODE, &show_mls_qos_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_mls_qos_aggregator_policer_cmd);
 /* Not supported in BCM */
  cli_install (ctree, EXEC_MODE, &show_mls_qos_maps_cos_dscp_cmd);
  cli_install (ctree, EXEC_MODE, &show_mls_qos_maps_ip_prec_dscp_cmd);
  cli_install (ctree, EXEC_MODE, &show_mls_qos_maps_policed_dscp_cmd);

  cli_install (ctree, EXEC_MODE, &show_mls_qos_maps_dscp_cos_cmd);
  cli_install (ctree, EXEC_MODE, &show_mls_qos_maps_dscp_mutation_cmd);
#endif

  cli_install (ctree, EXEC_MODE, &show_qos_access_list_name_cmd);
  cli_install (ctree, EXEC_MODE, &show_qos_access_list_cmd);
  cli_install (ctree, EXEC_MODE, &show_class_map_name_cmd);
  cli_install (ctree, EXEC_MODE, &show_class_map_cmd);
  cli_install (ctree, EXEC_MODE, &show_policy_map_name_cmd);
  cli_install (ctree, EXEC_MODE, &show_policy_map_cmd);

  /* CONFIG mode commands. */
  /* Enable / Disable the QoS */
#if 0
  cli_install (ctree, CONFIG_MODE, &mls_qos_enable_global_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_enable_global_new_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_enable_global_cmd);
#endif
 /* Configuring traffic by using ACLs */ 
  cli_install (ctree, CONFIG_MODE, &mac_access_list_address_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_access_list_address_cmd);
  cli_install (ctree, CONFIG_MODE, &mac_access_list_host_host_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_access_list_host_host_cmd);
  cli_install (ctree, CONFIG_MODE, &mac_access_list_host_any_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_access_list_host_any_cmd);
  cli_install (ctree, CONFIG_MODE, &mac_access_list_any_host_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mac_access_list_any_host_cmd);

  cli_install (ctree, CONFIG_MODE, &ip_access_list_standard_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_standard_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_standard_nomask_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_standard_nomask_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_standard_any_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_standard_any_cmd);

  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_mask_any_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_mask_any_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_any_mask_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_any_mask_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_any_any_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_any_any_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_mask_host_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_mask_host_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_host_mask_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_host_mask_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_host_host_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_host_host_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_any_host_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_any_host_cmd);
  cli_install (ctree, CONFIG_MODE, &ip_access_list_extended_host_any_cmd);
  cli_install (ctree, CONFIG_MODE, &no_ip_access_list_extended_host_any_cmd);

  /* Classfying traffic by using class maps */
  cli_install (ctree, CONFIG_MODE, &qos_class_map_cmd);
  cli_install (ctree, CONFIG_MODE, &no_qos_class_map_cmd);

  /* Classfying traffic by using class maps */
  cli_install (ctree, CONFIG_MODE, &qos_policy_map_cmd);
  cli_install (ctree, CONFIG_MODE, &no_qos_policy_map_cmd);

  /* Configuring DSCP maps */
#if 0 /* Not supported in BCM  and AXEL*/
  cli_install (ctree, CONFIG_MODE, &mls_qos_map_cos_dscp_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_map_cos_dscp_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_map_ip_prec_dscp_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_map_ip_prec_dscp_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_map_policed_dscp_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_map_policed_dscp_cmd);


  cli_install (ctree, CONFIG_MODE, &mls_qos_map_dscp_mutation_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_map_dscp_mutation_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_map_dscp_cos_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_map_dscp_cos_cmd);

  /* Configuring the aggregate-police command */
  cli_install (ctree, CONFIG_MODE, &mls_qos_aggregate_police_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_aggregate_police_cmd);


  cli_install (ctree, CONFIG_MODE, &mls_qos_min_reserve_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_min_reserve_cmd);
#endif

  /* Interface mode commands. */
  cli_install (ctree, INTERFACE_MODE, &service_policy_input_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_service_policy_input_cmd);
  cli_install (ctree, INTERFACE_MODE, &priority_queue_out_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_priority_queue_out_cmd);
  //cli_install (ctree, INTERFACE_MODE, &wrr_queue_queue_limit_cmd);
  /*wangjian@150116, change the queue limit cmd to RPORT_MODE */
  //cli_install (ctree, RPORT_MODE, &wrr_queue_weight_config_cmd);
  //cli_install (ctree, INTERFACE_MODE, &no_wrr_queue_queue_limit_cmd);
 // cli_install (ctree, RPORT_MODE, &no_wrr_queue_weight_config_cmd);
  //cli_install (ctree,  RPORT_MODE, &wrr_queue_egress_car_cmd);
  //cli_install (ctree, INTERFACE_MODE, &no_wrr_queue_queueid_threshold_cmd);
 // cli_install (ctree, RPORT_MODE, &no_wrr_queue_egress_car_cmd);

#if 0 /* Not supported in BCM and AXEL*/
  cli_install (ctree, INTERFACE_MODE, &service_policy_output_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_service_policy_output_cmd);

  cli_install (ctree, INTERFACE_MODE, &mls_qos_monitor_dscp_cmd);

  /* Configuring the buffer size of the minimum-reserve level */
#endif

  /* Allocating bandwidth among Egress queues */
  cli_install (ctree, INTERFACE_MODE, &wrr_queue_bandwidth_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_wrr_queue_bandwidth_cmd);
  cli_install (ctree, INTERFACE_MODE, &wrr_queue_cos_map_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_wrr_queue_cos_map_cmd);
 // cli_install (ctree,  RPORT_MODE, &wrr_queue_wred_cmd);
 // cli_install (ctree, INTERFACE_MODE, &no_wrr_queue_random_detect_max_threshold_cmd);
  //cli_install (ctree, RPORT_MODE, &no_wrr_queue_wred_cmd);
  cli_install (ctree, INTERFACE_MODE, &wrr_queue_threshold_dscp_map_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_wrr_queue_threshold_dscp_map_cmd);
  cli_install (ctree, INTERFACE_MODE, &wrr_queue_min_reserve_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_wrr_queue_min_reserve_cmd);

#if 0 /* Not supported in BCM and AXEL */
  /* Configuring classification by using port trust state */
  cli_install (ctree, INTERFACE_MODE, &mls_qos_trust_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_trust_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_trust_cos_path_through_dscp_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_trust_cos_path_through_dscp_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_trust_dscp_path_through_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_trust_dscp_path_through_cos_cmd);
#endif

  /* Configuring the DSCP trust state on an another QoS domain */
  cli_install (ctree, INTERFACE_MODE, &mls_qos_dscp_mutation_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_dscp_mutation_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_dscp_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_dscp_cos_cmd);
  
  /*
   * You can not configure both port-based classification and
   * VLAN-based classification at the same time
   * The port-based classifications are ACL, DSCP, IP-PREC.
   * The VLAN-based classification is VLAN
   */
  /* CMAP_MODE (Class Map) mode commands */
  cli_install (ctree, CMAP_MODE, &qos_cmap_match_access_group_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_access_group_cmd);
  
  cli_install (ctree, CMAP_MODE, &qos_cmap_match_ip_dscp_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_ip_dscp_cmd);

  cli_install (ctree, CMAP_MODE, &qos_cmap_match_ip_precedence_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_ip_precedence_cmd);

  cli_install (ctree, CMAP_MODE, &qos_cmap_match_exp_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_exp_cmd);

  cli_install (ctree, CMAP_MODE, &qos_cmap_match_l4_port_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_l4_port_cmd);

  cli_install (ctree, CMAP_MODE, &qos_cmap_match_vlan_cmd);
  cli_install (ctree, CMAP_MODE, &qos_cmap_match_vlan_range_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_vlan_cmd);

  cli_install (ctree, CMAP_MODE, &qos_cmap_match_traffic_type_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_traffic_type_cmd);
  cli_install (ctree, CMAP_MODE, &no_qos_cmap_match_traffic_type_cmd1);

  /* PMAP_MODE (Policy Map) mode commands */
  cli_install (ctree, PMAP_MODE, &qos_pmap_class_cmd);
  cli_install (ctree, PMAP_MODE, &no_qos_pmap_class_cmd);


#ifndef HAVE_BROADCOM
  cli_install (ctree, PMAPC_MODE, &qos_pmapc_trust_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_trust_cmd);
#endif /* HAVE_BROADCOM */

  cli_install (ctree, PMAPC_MODE, &qos_pmapc_set_cos_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_set_cos_cmd);

  cli_install (ctree, PMAPC_MODE, &qos_pmapc_set_ip_dscp_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_set_ip_dscp_cmd);

  cli_install (ctree, PMAPC_MODE, &qos_pmapc_set_ip_precedence_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_set_ip_precedence_cmd);

  cli_install (ctree, PMAPC_MODE, &qos_pmapc_set_exp_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_set_exp_cmd);

  cli_install (ctree, PMAPC_MODE, &qos_pmapc_police_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_police_cmd);

  cli_install (ctree, PMAPC_MODE, &qos_pmapc_police_aggregate_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_police_aggregate_cmd);

  cli_install (ctree, PMAPC_MODE, &qos_pmapc_set_vlanpriority_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_set_vlanpriority_cmd);
  cli_install (ctree, PMAPC_MODE, &qos_pmapc_set_algorithm_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_set_algorithm_cmd);
  cli_install (ctree, PMAPC_MODE, &qos_pmapc_set_quantumpriority_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_set_quantumpriority_cmd);
  cli_install (ctree, PMAPC_MODE, &qos_pmapc_policing_meter_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_policing_meter_cmd);
  cli_install (ctree, PMAPC_MODE, &qos_pmapc_police_rate_cmd);
  cli_install (ctree, PMAPC_MODE, &no_qos_pmapc_police_rate_cmd);

  /* Install in the PMAPC_MODE mode so that the startup configuration
   * restores the configuration OK.
   */
  cli_install (ctree, PMAPC_MODE, &qos_pmap_class_cmd);

  /* PMAPC_MODE (Policy Map Class) mode commands */

  /*wangjian@150109, disable the  old qos command */
  #if 0
  cli_install (ctree, CONFIG_MODE, &mls_qos_wrr_weight_queue_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_wrr_weight_queue_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_map_dscp_queue_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_map_dscp_queue_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_map_cos_queue_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_map_cos_queue_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_vlan_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_vlan_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &mls_qos_frame_type_priority_cmd);
  cli_install (ctree, CONFIG_MODE, &no_mls_qos_frame_type_priority_cmd);
  
  cli_install (ctree, INTERFACE_MODE, &mls_qos_vlan_priority_cos_queue_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_vlan_priority_cos_queue_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_da_priority_cos_queue_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_da_priority_cos_queue_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_trust_dscp_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_trust_dscp_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_force_trust_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_force_trust_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_default_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_default_cos_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_egress_rate_shape_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_egress_rate_shape_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_strict_queue_cmd);
  cli_install (ctree, INTERFACE_MODE, &mls_qos_strict_queue_all_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_mls_qos_strict_queue_cmd);
#endif 
  return;
}

#endif /* HAVE_QOS */
#endif /* HAVE_CUSTOM1 */
