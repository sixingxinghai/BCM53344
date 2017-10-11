/* Copyright (C) 2006-2007  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "snprintf.h"
#include "vty.h"

#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

#include "nsmd.h"
#include "nsm_interface.h"

#ifdef HAVE_QOS
#include "nsmd.h"
#include "nsm_qos.h"
#include "nsm_qos_list.h"
#include "nsm_qos_filter.h"
#include "nsm_qos_api.h"

#ifdef HAVE_HA
#include "nsm_flowctrl.h"
#include "nsm_qos_cal_api.h"
#endif /*HAVE_HA*/

/* Enable QoS */
int
nsm_qos_global_enable(struct nsm_master *nm)
{
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

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

  if (ret < 0)
    {
      nsm_qos_deinit( nm );
      return -1;
    }
#endif /* HAVE_HAL */

  /* Set enabled-state */
  qosg.state = NSM_QOS_ENABLED_STATE;
  qosg.config_new = 1;

#ifdef HAVE_HA
  nsm_qosg_data_cal_create (&qosg);
#endif /*HAVE_HA*/

  return 0;
}

/* Disable QoS */
int
nsm_qos_global_disable(struct nsm_master *nm)
{
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  CHECK_GLOBAL_QOS_ENABLED;

#ifdef HAVE_HAL
  /* Detach CoS queue from switch */
  ret = hal_qos_disable ();
  if (ret < 0)
    return -1;
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

  return 0;
}

#ifdef HAVE_SMI
/* Get QoS status */
int
nsm_qos_get_global_status(u_int8_t *status)
{
  *status = qosg.state;
  return SMI_SUCEESS;
}

int
nsm_qos_set_pmap_name(struct nsm_master *nm, char *pmap)
{
  struct nsm_pmap_list *pmapl = NULL;

  CHECK_GLOBAL_QOS_ENABLED;

  if (!nm || nm->pmap == NULL)
    return SMI_ERROR;

  pmapl = nsm_pmap_list_get (nm->pmap, pmap);
  if (!pmapl )
    return SMI_ERROR;
  
  if (pmapl->attached > 0)
    return SMI_ERROR;

  return SMI_SUCEESS;
}

#ifdef HAVE_SMI
int
nsm_qos_get_policy_map_info (struct nsm_master *nm, char *pmap_name, 
                             struct smi_qos_pmap *qos_map)
{
  struct nsm_pmap_list *pmapl;
  struct nsm_qos_pmap_master *master = nm->pmap;
  int i = 0;

  CHECK_GLOBAL_QOS_ENABLED;

  if (master == NULL || ! pmap_name)
    return SMI_ERROR;

  pmapl = nsm_pmap_list_lookup(master, pmap_name);
  if(pmapl == NULL)
    return SMI_ERROR;

  pal_mem_set(qos_map, 0, sizeof (struct smi_qos_pmap));

  snprintf(qos_map->name, SMI_QOS_POLICY_LEN, pmapl->name);
  snprintf(qos_map->acl_name, SMI_QOS_POLICY_LEN, pmapl->acl_name);
  qos_map->attached = pmapl->attached;
  for (i = 0; i < SMI_MAX_NUM_OF_CLASS_IN_PMAPL; i++)
    pal_snprintf(qos_map->cl_name[i], SMI_QOS_POLICY_LEN, pmapl->cl_name[i]);

  return SMI_SUCEESS;
}

int
nsm_qos_get_policy_map_names (struct nsm_master *nm, 
                              char pmap_names[][SMI_QOS_POLICY_LEN],
                              int first_call, char *pmap_name)
{
  struct nsm_pmap_list *pmapl;
  struct nsm_qos_pmap_master *master = nm->pmap;
  int pmap_found = 0;
  int i = 0;

  CHECK_GLOBAL_QOS_ENABLED;

  if (master == NULL || ! pmap_name)
    return SMI_ERROR;

  pal_mem_set(pmap_names, '\0', 
              SMI_NUM_REC * SMI_QOS_POLICY_LEN * sizeof(char));

  if(first_call)   /* First call */
  {
    for (i = 0, pmapl = master->head; pmapl && i <= SMI_NUM_REC; 
         pmapl = pmapl->next)
    {
      pal_mem_cpy(&pmap_names[i], pmapl->name, SMI_QOS_POLICY_LEN);
      i++;
    }
  }
  else
  {
    for (i = 0, pmapl = master->head; pmapl && i <= SMI_NUM_REC; 
         pmapl = pmapl->next)
    {
      if (pmapl->name && !(pal_strcmp (pmapl->name, pmap_name)))
      {
        pmap_found = 1;
        continue;
      }
      if(pmap_found)
      {
        pal_mem_cpy(&pmap_names[i], pmapl->name, SMI_QOS_POLICY_LEN);
        i++;
      }
    }
  }
  return SMI_SUCEESS;
}
#endif /* HAVE_SMI */

int
nsm_qos_delete_pmap (struct nsm_master *nm, char *pmap_name)
{
  struct nsm_qos_pmap_master *master = nm->pmap;
  struct nsm_pmap_list *pmapl, *pmapl_next;

  CHECK_GLOBAL_QOS_ENABLED;

  for (pmapl = master->head; pmapl; pmapl = pmapl_next)
  {
    pmapl_next = pmapl->next;
    if(!strcasecmp(pmapl->name, pmap_name))
    {
      if (pmapl->attached > 0)
       {
         zlog_err (NSM_ZG,
                   "Cann't delete Policy-Map %s as attached to Interfaces \n",
                   pmap_name);
         return SMI_ERROR;
       }
 
      nsm_pmap_list_delete (pmapl);
      break;
    }
  }
  return SMI_SUCEESS;
}

int
nsm_qos_set_cmap_name(struct nsm_master *nm, char *cmap_name)
{
  struct nsm_pmap_list *pmapl = NULL;
  struct nsm_cmap_list *cmapl = NULL;

  CHECK_GLOBAL_QOS_ENABLED;

  if (!nm)
    return SMI_ERROR;

  if (nm->pmap == NULL)
    return SMI_ERROR;

  /* Check -
   * if the policy-map, which contains this class-map, exist
   * and also it is attached to an interface
   */
  pmapl = nsm_qos_check_class_from_pmap (nm->pmap, cmap_name, NSM_TRUE);
  if (pmapl)
  {
    zlog_err (NSM_ZG, 
             "Policy-map should be detached from the interface before changing class-map\n");
    return SMI_ERROR;
  }

  /* Check the class-map list */
  cmapl = nsm_cmap_list_get (nm->cmap, cmap_name);
  if (!cmapl)
  {
    zlog_err (NSM_ZG, "Fail to get/add class-map list \n");
    return SMI_ERROR;
  }

  return SMI_SUCEESS;
}

#ifdef HAVE_SMI
int
nsm_qos_get_cmap_info (struct nsm_master *nm, char *cmap_name,
                       struct smi_qos_cmap *cmap_info)
{
  struct nsm_qos_cmap_master *master = nm->cmap;
  struct nsm_cmap_list *cmapl;

  CHECK_GLOBAL_QOS_ENABLED;

  cmapl = nsm_cmap_list_lookup (master, cmap_name);

  if (cmapl == NULL)
    return SMI_ERROR;

  snprintf (cmap_info->name, INTERFACE_NAMSIZ, cmapl->name);
  snprintf (cmap_info->acl_name, INTERFACE_NAMSIZ, cmapl->acl_name);
  snprintf (cmap_info->agg_policer_name, INTERFACE_NAMSIZ, 
            cmapl->agg_policer_name);
  pal_mem_cpy (&(cmap_info->v), &(cmapl->v), 
               sizeof(struct smi_vlan_filter));
  pal_mem_cpy (&(cmap_info->p), &(cmapl->p), 
               sizeof(struct smi_qos_police));
  pal_mem_cpy (&(cmap_info->s), &(cmapl->s), 
               sizeof(struct smi_set));
  pal_mem_cpy (&(cmap_info->t), &(cmapl->t), 
               sizeof(struct smi_trust));
  pal_mem_cpy (&(cmap_info->d), &(cmapl->d), 
               sizeof(struct smi_cmap_dscp));
  pal_mem_cpy (&(cmap_info->i), &(cmapl->i), 
               sizeof(struct smi_cmap_ip_prec));
  pal_mem_cpy (&(cmap_info->e), &(cmapl->e), 
               sizeof(struct smi_cmap_exp));
  cmap_info->match_type = cmapl->match_type;

  return SMI_SUCEESS;
}
#endif /* HAVE_SMI */

int
nsm_qos_delete_cmap (struct nsm_master *nm, char *cmap_name)
{
  struct nsm_qos_cmap_master *master = nm->cmap;
  struct nsm_cmap_list *cmapl, *cmapl_next;
  struct nsm_pmap_list *pmapl = NULL;

  CHECK_GLOBAL_QOS_ENABLED;

  pmapl = nsm_qos_check_class_from_pmap (nm->pmap, cmap_name, NSM_FALSE);
  if (pmapl) 
  {
    zlog_err (NSM_ZG,
              "Cann't delete Class-Map %s as attached to Policy-Map %s\n",
               cmap_name, pmapl->name);
    return SMI_ERROR;
  }  

  for (cmapl = master->head; cmapl; cmapl = cmapl_next)
  {
    cmapl_next = cmapl->next;
    if(!strcasecmp(cmapl->name, cmap_name))
    {
      nsm_cmap_list_delete (cmapl);
      break;
    }
  }
  return SMI_SUCEESS;
}

#ifdef HAVE_SMI
int
nsm_qos_pmapc_set_police (struct nsm_master *nm, 
                          char *pmap_name, 
                          char *cmap_name,
                          int rate_value, 
                          int commit_burst_size, 
                          int excess_brust_size, 
                          enum smi_exceed_action exceed_action,
                          enum smi_qos_flow_control_mode fc_mode)
{
  struct nsm_pmap_list *pmapl, *pmapl1;
  struct nsm_cmap_list *cmapl;
  int i = 0;

  CHECK_GLOBAL_QOS_ENABLED;

  pmapl = nsm_pmap_list_lookup (nm->pmap, pmap_name);
  if (!pmapl)
    return SMI_ERROR; 
  
  if (pmapl->attached > 0)
   {
     zlog_err (NSM_ZG,
               "Can't add or update class %s as policy-map attach to intf\n",
               cmap_name);
      return SMI_ERROR;
   }

  pmapl1 = nsm_qos_check_cl_name_in_pmapl (pmapl, cmap_name);
  if (! pmapl1)
    return SMI_ERROR;

  for (pmapl = nm->pmap->head; pmapl; pmapl = pmapl->next)
  {
    if(!strcasecmp(pmap_name, pmapl->name))
      for (i = 0; i < MAX_NUM_OF_CLASS_IN_PMAPL; i++)
      {
        if (pmapl->cl_name[i] == NULL)
          continue;
  
        if(!strcasecmp(cmap_name, pmapl->cl_name[i]))
        {
          cmapl = nsm_cmap_list_lookup (nm->cmap, cmap_name);
          if (!cmapl)
            return SMI_ERROR;

          nsm_qos_set_exd_act_into_cmap (cmapl, rate_value, commit_burst_size,
                                         excess_brust_size, exceed_action, 
                                         fc_mode);
        }
      }
  }
  return SMI_SUCEESS;
}

int
nsm_qos_pmapc_get_police (struct nsm_master *nm, 
                          char *pmap_name, 
                          char *cmap_name,
                          int *rate_value, 
                          int *commit_burst_size, 
                          int *excess_brust_size, 
                          enum smi_exceed_action *exceed_action,
                          enum smi_qos_flow_control_mode *fc_mode)
{
  struct nsm_pmap_list *pmapl;
  struct nsm_cmap_list *cmapl;
  int i = 0;

  CHECK_GLOBAL_QOS_ENABLED;

  for (pmapl = nm->pmap->head; pmapl; pmapl = pmapl->next)
  {
    if(!strcasecmp(pmap_name, pmapl->name))
      for (i = 0; i < MAX_NUM_OF_CLASS_IN_PMAPL; i++)
      {
        if (pmapl->cl_name[i] == NULL)
          continue;
  
        if(!strcasecmp(cmap_name, pmapl->cl_name[i]))
        {
          cmapl = nsm_cmap_list_lookup (nm->cmap, cmap_name);
          if (!cmapl)
            return SMI_ERROR;

          *rate_value = cmapl->p.avg;
          *commit_burst_size = cmapl->p.burst;
          *excess_brust_size = cmapl->p.excess_burst;
          *exceed_action = cmapl->p.exd_act;
          *fc_mode = cmapl->p.fc_mode;
          break;
        }
      }
  }
  return SMI_SUCEESS;
}
#endif /* HAVE_SMI */


int
nsm_qos_pmapc_delete_police (struct nsm_master *nm, 
                             char *pmap_name, char *cmap_name)
{
  struct nsm_pmap_list *pmapl = NULL;
  struct nsm_cmap_list *cmapl = NULL;
  int i = 0;

  CHECK_GLOBAL_QOS_ENABLED;

  pmapl = nsm_pmap_list_lookup (nm->pmap, pmap_name);
  if (pmapl->attached > 0)
   {
     zlog_err (NSM_ZG,
               "Cann't Delete the Policy Map %s as interface attached \n",
               pmap_name);
     return SMI_ERROR;
   }

  for (pmapl = nm->pmap->head; pmapl; pmapl = pmapl->next)
  {
    if(!strcasecmp(pmap_name, pmapl->name))
      for (i = 0; i < MAX_NUM_OF_CLASS_IN_PMAPL; i++)
      {
        if (pmapl->cl_name[i] == NULL)
          continue;
  
        if(!strcasecmp(cmap_name, pmapl->cl_name[i]))
        {
          cmapl = nsm_cmap_list_lookup (nm->cmap, cmap_name);
          if (!cmapl)
            return SMI_ERROR;

          nsm_qos_clear_exd_act_into_cmap (cmapl);
        }
      }
  }
  return SMI_SUCEESS;
}

int
nsm_qos_pmapc_delete_cmap (struct nsm_master *nm,
                           char *pmap_name, char *cmap_name)
{
  struct nsm_pmap_list *pmapl = NULL;
  struct nsm_cmap_list *cmapl = NULL;
  int i = 0;
  
  CHECK_GLOBAL_QOS_ENABLED;
   
  pmapl = nsm_pmap_list_lookup (nm->pmap, pmap_name);
     
  if (!pmapl)
    return SMI_ERROR;
       
  for (i = 0; i < MAX_NUM_OF_CLASS_IN_PMAPL; i++)
     {
       if (!strcasecmp(cmap_name, pmapl->cl_name[i]))
         continue; 
       else
         {
           cmapl = nsm_cmap_list_lookup (nm->cmap, cmap_name);
           if (!cmapl)
             return SMI_ERROR;             
           else
             {
               if (pmapl->attached > 0)
                 {
                   zlog_err (NSM_ZG,
                             "Can't delete cmap %s as Pmap attached %s\n",
                             cmap_name, pmap_name);
                   return SMI_ERROR;             
                 }
               nsm_qos_delete_cmap_from_pmapl (pmapl, cmap_name);
               return SMI_SUCEESS;
             }
         }
      }
  return SMI_ERROR;
}


int
nsm_qos_get_strict_queue (struct nsm_qos_global *qosg,
                          struct interface *ifp, u_int8_t *qid)
{
  struct nsm_qif_list *qifl;

  qifl = nsm_qif_list_lookup (qosg->qif, ifp->name);

  if (qifl == NULL)
    return SMI_ERROR;

  *qid = qifl->strict_queue;

  return SMI_SUCEESS;
}

int 
nsm_qos_set_port_service_policy(struct nsm_master *nm, 
                                struct interface *ifp, char *pmap_name)
{
  struct nsm_pmap_list *pmapl;
  struct nsm_qif_list *qifl;
  int action;
  u_int8_t dir = NSM_QOS_DIRECTION_INGRESS;

#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */

  CHECK_GLOBAL_QOS_ENABLED;

  if (ifp->hw_type == IF_TYPE_VLAN)
  {
    zlog_err (NSM_ZG, 
             "Service policy cannot be applied to vlan interfaces\n");
    return SMI_ERROR;
  }

#ifdef HAVE_LACPD
  {
    struct nsm_if *zif;
    zif = ifp->info;
    if (zif->agg.type == NSM_IF_AGGREGATOR)
      {
        zlog_err (NSM_ZG,
                "Service policy cannot be applied to aggregator interfaces\n");
        return SMI_ERROR;
      }
  }
#endif /* HAVE_LACPD */

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, &ifp->name[0]);

  if (qifl == NULL)
  {
    qifl = nsm_qif_list_get (qosg.qif, &ifp->name[0]);
    if (! qifl)
      return SMI_ERROR;
  }

  if (qifl->input_pmap_name[0] != 0x00)
  {
    zlog_err (NSM_ZG, 
              "Fail because policy-map(%s) is already attached\n", 
              qifl->input_pmap_name);
    return SMI_ERROR;
  }

  if (qifl->dscp_mut_name[0] != '\0')
  {
    zlog_err (NSM_ZG,
              "Policy-map cannot be applied as dscp-mutation is already applied to this interface, \n");
    return SMI_ERROR;
  }

  /* Get the policy-map information from linked list */
  pmapl = nsm_pmap_list_lookup (nm->pmap, pmap_name);
  if (! pmapl)
  {
    zlog_err (NSM_ZG, "Can't find policy-map(%s)\n", pmap_name);
      return SMI_ERROR;
  }

  /* Attach policy-map to the interface */
  action = NSM_QOS_ACTION_ADD;

#ifdef HAVE_HAL
  ret = hal_qos_set_policy_map (ifp->ifindex, action, dir);
  if (ret < 0)
  {
    zlog_err(NSM_ZG,
             "Fail to attach policy-map(%s) to the interface\n", pmap_name);
    return SMI_ERROR;
  }

  ret = nsm_qos_hal_set_all_class_map (nm, pmapl, ifp->ifindex, action, dir);
  if (ret < 0)
  {
    zlog_err (NSM_ZG, "Fail to attach class-map to the interface\n");
    hal_qos_send_policy_map_detach(ifp->ifindex, action, dir);
    return SMI_ERROR;
  }
  
  /* Install the policy-map into hardware */
  ret = hal_qos_send_policy_map_end(ifp->ifindex, action, dir);
  if (ret < 0)
  {
    /* Clear policy-map because of HW installation is failed */
    hal_qos_send_policy_map_detach(ifp->ifindex, action, dir);
    return SMI_ERROR;
  }
#endif /* HAVE_HAL */

  pmapl->attached ++;

  pal_strcpy ( &qifl->if_name[0], &ifp->name[0]);
  pal_strcpy ( &qifl->input_pmap_name[0], pmap_name);
  return SMI_SUCEESS;
}

int 
nsm_qos_unset_port_service_policy(struct nsm_master *nm, 
                                  struct interface *ifp, char *pmap_name)
{
  struct nsm_pmap_list *pmapl;
  struct nsm_qif_list *qifl;
#ifdef HAVE_HAL
  int ret;
#endif /* HAVE_HAL */
  u_int8_t dir = NSM_QOS_DIRECTION_INGRESS;

  CHECK_GLOBAL_QOS_ENABLED;

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, &ifp->name[0]);
  if (qifl == NULL)
  {
    zlog_err (NSM_ZG, "Can't find policy-map(%s)\n", pmap_name);
    return SMI_ERROR;
  }
  else
  {
    if (qifl->input_pmap_name == NULL ||
        pal_strcmp (qifl->input_pmap_name, pmap_name) != 0)
    {
      zlog_err (NSM_ZG, "Can't find policy-map(%s)\n", pmap_name);
        return SMI_ERROR;
    }
  }

  /* Get the policy-map information from linked list */
  pmapl = nsm_pmap_list_lookup (nm->pmap, pmap_name);
  if (! pmapl)
  {
    zlog_err (NSM_ZG, "Can't find policy-map(%s)\n", pmap_name);
    return SMI_ERROR;
  }

#ifdef HAVE_HAL
  ret = hal_qos_set_policy_map (ifp->ifindex, NSM_QOS_ACTION_DELETE, dir);
  if (ret < 0)
  {
    zlog_err (NSM_ZG,
              "Fail to detach policy-map(%s) from the interface\n", pmap_name);
      return SMI_ERROR;
  }
#endif /* HAVE_HAL */

  pmapl->attached --;
  pal_mem_set ( &qifl->input_pmap_name[0], 0, (INTERFACE_NAMSIZ + 1));

  return SMI_SUCEESS;
}

int 
nsm_qos_get_port_service_policy(struct nsm_master *nm, 
                                struct interface *ifp, char *pmap_name)
{
  struct nsm_qif_list *qifl;

  CHECK_GLOBAL_QOS_ENABLED;

  if (ifp->hw_type == IF_TYPE_VLAN)
    return SMI_ERROR;

#ifdef HAVE_LACPD
  {
    struct nsm_if *zif;
    zif = ifp->info;
    if (zif->agg.type == NSM_IF_AGGREGATOR)
        return SMI_ERROR;
  }
#endif /* HAVE_LACPD */

  /* Check policy name in the interface */
  qifl = nsm_qif_list_lookup (qosg.qif, &ifp->name[0]);

  if (qifl == NULL)
  {
    qifl = nsm_qif_list_get (qosg.qif, &ifp->name[0]);
    if (! qifl)
      return SMI_ERROR;
  }

  pal_sprintf(pmap_name, qifl->input_pmap_name);

  return SMI_SUCEESS;
}

#endif /* HAVE_QOS */
#endif /* HAVE_SMI */
