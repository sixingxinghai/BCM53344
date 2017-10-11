/* Copyright (C) 2002  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_TE 
#include "lib.h"
#include "mpls.h"

#include "prefix.h"
#include "table.h"
#include "log.h"
#include "qos_common.h"
#include "if.h"
#include "linklist.h"
#include "stream.h"
#include "label_pool.h"
#include "network.h"
#include "mpls_common.h"
#include "mpls_client.h"

#include "nsmd.h"
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"

#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#include "nsm_mpls.h"

#ifdef HAVE_MPLS_FRR
#include "nsm_mpls_rib.h"
#endif /* HAVE_MPLS_FRR */

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */

#ifdef HAVE_GMPLS
#include "nsm/gmpls/nsm_gmpls_if.h"
#endif /* HAVE_GMPLS */
#include "nsm_qos_serv.h"
#include "nsm_interface.h"
#include "nsm_mpls_qos.h"
#include "nsm_mpls_rib.h"
#include "nsm_debug.h"
#include "nsm_server.h"
#include "nsm_router.h"

/* Initialize QOS module */
void
nsm_qos_serv_init (struct nsm_master *nm)
{
  /* Set resource counter to one */
  nm->resource_counter = 1;
}

/* Initialize qos interface */
struct qos_interface *
nsm_qos_serv_if_init (struct interface *ifp)
{
  struct qos_interface *qos_if;
  int i;
  
  if (! ifp)
    return NULL;

  /* Create object */
  qos_if = XCALLOC (MTYPE_NSM_QOS_IF, sizeof (struct qos_interface));
  if (! qos_if)
    return NULL;

  qos_if->ifp = ifp;

  /* Initialize all eight route tables */
  for (i = 0; i < MAX_PRIORITIES; i++)
    qos_if->resource_array[i] = route_table_init ();

  /* Enable this interface */
  qos_if->status = QOS_INTERFACE_ENABLED;

  return qos_if;
}

/* GET qos interface */
struct qos_interface *
nsm_qos_serv_if_get (struct nsm_master *nm, u_int32_t ifindex)
{
  struct interface *ifp;
  struct nsm_if *nif;

  ifp = if_lookup_by_index (&nm->vr->ifm, ifindex);
  if (ifp
      && ((nif = ifp->info))
      && nif->qos_if)
    return nif->qos_if;

  return NULL;
}

/* Send preemption message for the resources in the list passed in. */
void
nsm_qos_serv_send_preempt (struct nsm_master *nm,
                           struct list **list, bool_t send_to_protocol,
                           bool_t remove_mpls_rib)
{
  struct nsm_msg_qos_preempt preempt;
  struct qos_resource *resource;
  struct route_node *rn;
  struct listnode *ln, *ln_next;
  u_int32_t id_array[MAX_RESOURCES_PER_MSG];
  u_char *pnt;

  if (! list || ! *list)
    return;

  /* If any resources were preempted, send up messages in batches. */
  do {
    pal_mem_set (&preempt, 0, sizeof (struct nsm_msg_qos_preempt));
    pnt = (u_char *)id_array;
    for (ln = LISTHEAD (*list); ln; ln = ln_next)
      {
        ln_next = ln->next;
        rn = ln->data;
        resource = rn->info;
        
        if (preempt.protocol_id == 0)
          preempt.protocol_id = resource->protocol_id;
        else if (resource->protocol_id != preempt.protocol_id)
          continue;

#ifdef HAVE_MPLS_FRR
        if (remove_mpls_rib)
          nsm_mpls_rib_qos_rsrc_del_process (nm, resource->resource_id);
#endif /* HAVE_MPLS_FRR */
        
        if (send_to_protocol == NSM_TRUE)
          {
            ++preempt.count;
            pal_mem_cpy (pnt, &resource->resource_id, sizeof (u_int32_t));
            pnt += sizeof (u_int32_t);
          }
        
        qos_common_resource_delete (resource);
        rn->info = NULL;
        route_unlock_node (rn);
        list_delete_node (*list, ln);

        if (preempt.count >= MAX_RESOURCES_PER_MSG)
          break;
      }
    
    /* Send message to protocol. */
    if (send_to_protocol == NSM_TRUE)
      {
        NSM_ASSERT (preempt.count > 0);
        preempt.preempted_ids = id_array;
        nsm_qos_preempt_resource (&preempt);
      }
  } while (LISTHEAD (*list) != NULL);
  
  /* Delete list. */
  list_delete (*list);
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
void
nsm_qos_serv_te_class_avail_bw_update (struct qos_interface *qos_if,
                                       u_char tec_ix)
{
  struct nsm_master *nm;
  float32_t bw1, bw2;
  struct interface *ifp;
  u_char priority;
  int ct_num;

  if (! qos_if || ! qos_if->ifp)
    return;

  ifp = qos_if->ifp;
  nm = ifp->vr->proto;

  priority = NSM_MPLS->te_class[tec_ix].priority;
  ct_num = NSM_MPLS->te_class[tec_ix].ct_num;
  if (priority == TE_PRIORITY_INVALID ||
      ct_num == CT_NUM_INVALID)
    {
      ifp->tecl_priority_bw[tec_ix] = 0;
      return;
    }

  bw1 = ifp->bw_constraint[ct_num];
  if (bw1 > 0)
    bw1 -= qos_if->ct_aggr_rsvd_bw[tec_ix];
  
  bw2 = ifp->max_resv_bw;
  if (bw2 > 0)
    bw2 -= qos_if->aggr_rsvd_bw[priority];
  
  ifp->tecl_priority_bw[tec_ix] = bw1 < bw2 ? bw1 : bw2;
}
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */


void
nsm_qos_serv_bw_release_process (struct qos_interface *qos_if, 
                                 float32_t bw, u_char h_priority, 
                                 int ct_num, struct qos_resource *resource)
{
  struct nsm_master *nm;
  struct interface *ifp;
#if ((defined (HAVE_PACKET) && defined (HAVE_DSTE)) || \
     (defined (HAVE_GMPLS) && !defined (HAVE_PACKET))) 
  int i;
#endif 

  int ret;

  if (! qos_if || ! qos_if->ifp || bw == 0)
    return;

  ifp = qos_if->ifp;
  nm = ifp->vr->proto;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  if (! IS_VALID_CT_NUM (ct_num))  
    return;
  
  for (i = h_priority; i < MAX_PRIORITIES; i++)
    qos_if->aggr_rsvd_bw[i] -= bw;
  
  for (i = 0; i < MAX_TE_CLASSES; i++)
    {
      if (NSM_MPLS->te_class[i].ct_num == ct_num &&
          NSM_MPLS->te_class[i].priority >= h_priority)
        qos_if->ct_aggr_rsvd_bw[i] -= bw;
      
      nsm_qos_serv_te_class_avail_bw_update (qos_if, i);
    }
#endif /* HAVE_DSTE */
#else
  for (i = h_priority; i < MAX_PRIORITIES; i++)
    {
      qos_if->aggr_rsvd_bw[i] -= bw;
      ifp->tecl_priority_bw[i] += bw;
    }
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#ifdef HAVE_GMPLS
  nsm_data_link_property_copy (ifp, PAL_TRUE);
#endif /* HAVE_GMPLS */

  ret = apn_mpls_qos_release (resource);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Can't release QoS resource-id %d from forwarder\n", 
                 resource->resource_id);
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Successfully release QoS resource-id %d from"
                        "forwarder\n",
               resource->resource_id);
}


void
nsm_qos_serv_bw_reserve_process (struct qos_interface *qos_if, 
                                 float32_t bw, u_char h_priority, 
                                 int ct_num, struct qos_resource *resource)
{
  struct nsm_master *nm;
  struct interface *ifp;
#if ((defined (HAVE_PACKET) && defined (HAVE_DSTE)) || \
     (defined (HAVE_GMPLS) && !defined (HAVE_PACKET))) 
  int i;
#endif 
  int ret;
  
  if (! qos_if || ! qos_if->ifp || bw == 0)
    return;
  
  ifp = qos_if->ifp;
  nm = ifp->vr->proto;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  if (! IS_VALID_CT_NUM (ct_num))
    return;
  
  for (i = h_priority; i < MAX_PRIORITIES; i++)
    qos_if->aggr_rsvd_bw[i] += bw;
  
  for (i = 0; i < MAX_TE_CLASSES; i++)
    {
      if (NSM_MPLS->te_class[i].ct_num == ct_num &&
          NSM_MPLS->te_class[i].priority >= h_priority)
        qos_if->ct_aggr_rsvd_bw[i] += bw;

      nsm_qos_serv_te_class_avail_bw_update (qos_if, i);
    }
#endif /* HAVE_DSTE */
#else
  for (i = h_priority; i < MAX_PRIORITIES; i++)
    {
      qos_if->aggr_rsvd_bw[i] += bw;
      ifp->tecl_priority_bw[i] -= bw;
    }
#endif /* !HAVE_GMPLS || HAVE_PACKET */

#ifdef HAVE_GMPLS
  nsm_data_link_property_copy (ifp, PAL_TRUE);
#endif /* HAVE_GMPLS */

  ret = apn_mpls_qos_reserve (resource);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Can't reserve QoS resource-id %d from forwarder\n", 
                 resource->resource_id);
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Successfully reserved QoS resource-id %d from"
                        "forwarder\n",
               resource->resource_id);
}




/* Preempt all LSPs on the specified interface with bandwidth higher
   than specified. */
void
nsm_qos_serv_preempt_by_bandwidth (struct list *preempt_list,
                                   struct qos_interface *qos_if,
                                   float32_t bandwidth)
{
  struct qos_resource *resource;
  struct route_table *table;
  struct route_node *rn;
  int i;
  float32_t tmp_bw = 0;

  /* Go through the tables and preempt offenders. */
  for (i = 0; i < MAX_PRIORITIES; i++)
    {
      /* Get table */
      table = qos_if->resource_array[i];
      NSM_ASSERT (table != NULL);
      if (! table)
        continue;

      for (rn = route_top (table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          /* Get resource */
          resource = rn->info;
          
          tmp_bw = resource->t_spec.committed_bucket.rate;
          
          /* If rate is greater than bw specified, preempt it. */
          if (tmp_bw > bandwidth)
            {
              /* Add to preempt list. */
              listnode_add (preempt_list, rn);
              nsm_qos_serv_bw_release_process (qos_if, 
                                               tmp_bw, i, resource->ct_num,
                                               resource);
            }
        }
    }
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
/* Preempt all LSPs on the specified interface belonging to the
   the specified class type 
*/ 
void
nsm_qos_serv_preempt_by_class (struct list *preempt_list,
                               struct qos_interface *qos_if,
                               int ct_num)
{
  struct qos_resource *resource;
  struct route_table *table;
  struct route_node *rn;
  int i;
  float32_t tmp_bw;

  /* Go through the tables and preempt resources for the specified class */
  for (i = 0, tmp_bw = 0; i < MAX_PRIORITIES; i++)
    {
      /* Get table */
      table = qos_if->resource_array[i];
      NSM_ASSERT (table != NULL);
      if (! table)
        continue;

      for (rn = route_top (table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          /* Get resource */
          resource = rn->info;

          /* Add to preempt list. */
          listnode_add (preempt_list, rn);

          if (resource->ct_num == ct_num)
            {
              tmp_bw = resource->t_spec.committed_bucket.rate;
              nsm_qos_serv_bw_release_process (qos_if, 
                                               tmp_bw, i, 
                                               ct_num, resource);
            }
        }
    }
}


/*
  Preempt all LSPs on the specified interface 
  belonging to the specified te class 
*/ 
void
nsm_qos_serv_preempt_by_te_class (struct list *preempt_list,
                                  struct qos_interface *qos_if,
                                  u_char tecl_ix)
{
  struct nsm_master *nm = qos_if->ifp->vr->proto;
  struct qos_resource *resource;
  struct route_table *table;
  struct route_node *rn;
  int i;
  float32_t tmp_bw;
  u_char priority;
  int ct_num;

  priority = NSM_MPLS->te_class[tecl_ix].priority;
  ct_num = NSM_MPLS->te_class[tecl_ix].ct_num;

  /* Traverse the tables and preempt resources for the specified te class */
  for (i = 0, tmp_bw = 0; i < MAX_PRIORITIES; i++)
    {
      /* Get table */
      table = qos_if->resource_array[i];
      NSM_ASSERT (table != NULL);
      if (! table)
        continue;

      for (rn = route_top (table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          /* Get resource */
          resource = rn->info;

          if (resource->ct_num == ct_num && 
              (resource->setup_priority == priority ||
               resource->hold_priority == priority))
            {
              /* Add to preempt list. */
              listnode_add (preempt_list, rn);

              tmp_bw = resource->t_spec.committed_bucket.rate;
              nsm_qos_serv_bw_release_process (qos_if, 
                                               tmp_bw, i, 
                                               ct_num, resource);
            }
        }
    }
}
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

/* Pre-empt offending flows */
qos_status
nsm_qos_serv_preempt_offenders (struct list *preempt_list, 
                                struct qos_interface *qos_if,
                                float32_t bw, int priority)
{
  struct qos_resource *resource;
  struct route_table *table;
  struct route_node *rn;
  float32_t tmp_bw, released_bw;
  int i;

  /* Go through the tables and preempt offenders. */
  for (i = (MAX_PRIORITIES - 1), tmp_bw = 0, released_bw = 0; 
       i > priority; i--)
    {
      /*
       * Gather enough bw to satisfy the new resource. If not
       * available, go the next priority level.
       */
      
      /* Get table */
      table = qos_if->resource_array[i];
      NSM_ASSERT (table != NULL);
      if (! table)
        continue;
      
      for (rn = route_top (table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          /* Get resource */
          resource = rn->info;

          /* Add resource to list. */
          listnode_add (preempt_list, rn);

          /* Update the temp bw count */
          tmp_bw = resource->t_spec.committed_bucket.rate;
          released_bw += tmp_bw;

          nsm_qos_serv_bw_release_process (qos_if, 
                                           tmp_bw, i, resource->ct_num,
                                           resource);
            
          /* If enough bandwidth has been made available ... */
          if (bw <= released_bw)
            return QOS_OK;
        }
    }


#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  return QOS_OK;
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  
  return QOS_ERR;
}


void
nsm_qos_serv_max_bw_preemption (struct list *preempt_list,
                                struct qos_interface *qos_if,
                                float32_t bandwidth,
                                float32_t bw_diff)
{
  struct qos_resource *resource;
  struct route_table *table;
  struct route_node *rn;
  int i;
  float32_t tmp_bw = 0;
  float32_t total_bw = 0;

  /* Go through the tables and preempt offenders. */
  for (i = MAX_PRIORITIES - 1; i >= 0; i--)
    {
      /* Get table */
      table = qos_if->resource_array[i];
      NSM_ASSERT (table != NULL);
      if (! table)
        continue;

      for (rn = route_top (table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          /* Get resource */
          resource = rn->info;
          
          tmp_bw = resource->t_spec.committed_bucket.rate;
          
          /* If rate is greater than bw specified, preempt it. */
          if (tmp_bw > bandwidth || total_bw < bw_diff)
            {
              total_bw += tmp_bw;

              /* Add to preempt list. */
              listnode_add (preempt_list, rn);
              nsm_qos_serv_bw_release_process (qos_if, 
                                               tmp_bw, i, resource->ct_num,
                                               resource);
            }
        }
    }
}


void
nsm_qos_serv_avail_bw_update (struct qos_interface *qos_if)
{
  int i;
  struct interface *ifp;
  
  if (! qos_if || ! qos_if->ifp)
    return;
  
  ifp = qos_if->ifp;
  
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  for (i = 0; i < MAX_TE_CLASS; i++)
    nsm_qos_serv_te_class_avail_bw_update (qos_if, i);
#else
  for (i = 0; i < MAX_PRIORITIES; i++)
    ifp->tecl_priority_bw[i] = ifp->max_resv_bw - qos_if->aggr_rsvd_bw[i];
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
}


/* Update max bw attribute */
int
nsm_qos_serv_update_max_bw (struct interface *ifp, float32_t bandwidth)
{
  struct nsm_master *nm = NULL;
  struct qos_interface *qos_if;
  struct nsm_if *nif;
  float32_t old_bw;
  
  if (! ifp)
    return -1;

  nm = ifp->vr->proto;  
  if (bandwidth > 0)
    SET_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);
  else
    UNSET_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED);
  
  if (ifp->bandwidth == bandwidth)
    return 0;
  
  nif = ifp->info;
  if (nif->qos_if == NULL)
    {
      /* Create one */
      nif->qos_if = nsm_qos_serv_if_init (ifp);
      if (! nif->qos_if)
        return -1;
    }
  
  qos_if = nif->qos_if;
  old_bw = ifp->bandwidth;
  if (CHECK_FLAG (ifp->conf_flags, NSM_BANDWIDTH_CONFIGURED))
    ifp->bandwidth = bandwidth;
  else pal_kernel_if_get_bw (ifp);
  
#ifdef HAVE_DSTE
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  if (bandwidth < old_bw && ifp->max_resv_bw > 0)
    {
      struct list *preempt_list = list_new ();
      
      nsm_qos_serv_preempt_by_bandwidth (preempt_list, qos_if, bandwidth);
      
      if (LISTCOUNT (preempt_list))
        nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
      else
        list_delete (preempt_list);
    }
#endif /*if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))*/
#else
  if (! CHECK_FLAG (ifp->conf_flags, NSM_MAX_RESV_BW_CONFIGURED) &&
      ifp->max_resv_bw != bandwidth)
    {
      float32_t old_max_resv_bw;
      float32_t total_rsvd_bw;

      old_max_resv_bw = ifp->max_resv_bw;
      ifp->max_resv_bw = bandwidth;
      total_rsvd_bw = qos_if->aggr_rsvd_bw[MAX_PRIORITIES - 1];
     
      if (bandwidth < old_bw || 
          (bandwidth < old_max_resv_bw &&
           bandwidth < total_rsvd_bw))
        {
          struct list *preempt_list = list_new ();
          
          /* preempt lsps */
          nsm_qos_serv_max_bw_preemption (preempt_list,
                                          qos_if,
                                          bandwidth,
                                          (total_rsvd_bw - bandwidth));
          if (LISTCOUNT (preempt_list))
            nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
          else
            list_delete (preempt_list);
        }
      
      /* Send max resv bandwidth update to clients */
      nsm_qos_send_max_resv_bw_update (ifp);
      
  /* Update available bandwidth information */
        nsm_qos_serv_avail_bw_update (qos_if);
    }
  else if (bandwidth < old_bw)
    {
      struct list *preempt_list = list_new ();
      
      nsm_qos_serv_preempt_by_bandwidth (preempt_list, qos_if, bandwidth);
      
      if (LISTCOUNT (preempt_list))
        nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
      else
        list_delete (preempt_list);
    }
#endif /* HAVE_DSTE */

#ifdef HAVE_GMPLS
  nsm_data_link_property_copy (ifp, PAL_TRUE);
#endif /* HAVE_GMPLS */

  /* Send available bandwidth update to clients */
  if (if_is_up (ifp))
    nsm_server_if_priority_bw_update (ifp);
  
  /* Send max bandwidth update to clients */
  nsm_qos_send_max_bw_update (ifp);

  return 0;
}

/* Update max resv bw attribute */
int
nsm_qos_serv_update_max_resv_bw (struct interface *ifp, float32_t bandwidth,
                                 bool_t is_explicit)
     
{
  struct nsm_master *nm = NULL;
  struct nsm_if *nif;
  struct qos_interface *qos_if;
  float32_t total_rsvd_bw, old_bw, effective_bw = 0;

  if (! ifp)
    return -1;

  nm = ifp->vr->proto;
  if (ifp->max_resv_bw == bandwidth)
    goto set_qos_flag;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  /* validation */
  {
    int i;

    for (i = 0; i < MAX_BW_CONST; i++)
      if (bandwidth < ifp->bw_constraint[i])
        return NSM_ERR_INVALID_BW_CONSTRAINT;
  }
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  /* get qos interface */
  nif = ifp->info;
  if (nif->qos_if == NULL)
    {
      /* Create one */
      nif->qos_if = nsm_qos_serv_if_init (ifp);
      if (! nif->qos_if)
        return 0;
    }

  qos_if = nif->qos_if;
  
  total_rsvd_bw = qos_if->aggr_rsvd_bw[MAX_PRIORITIES - 1];
  old_bw = ifp->max_resv_bw;
  ifp->max_resv_bw = bandwidth;

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  effective_bw = bandwidth;
#endif /* HAVE_DSTE */
#else
  effective_bw = bandwidth > 0 ? bandwidth : ifp->bandwidth;
#endif /* !HAVE_GMPLS || HAVE_PACKET */

  if (effective_bw < old_bw &&
      effective_bw < total_rsvd_bw)
    {
      struct list *preempt_list = list_new ();
      
      /* preempt lsps */
      nsm_qos_serv_preempt_offenders (preempt_list,
                                      qos_if,
                                      (total_rsvd_bw - effective_bw), 
                                      -1);
      if (LISTCOUNT (preempt_list))
        nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
      else
        list_delete (preempt_list);
    }
  
  /* Send max resv bandwidth update to clients */
  nsm_qos_send_max_resv_bw_update (ifp);
  
  /* Update available bandwidth information */
  nsm_qos_serv_avail_bw_update (qos_if);
  
#ifdef HAVE_GMPLS
  nsm_data_link_property_copy (ifp, PAL_TRUE);
#endif /* HAVE_GMPLS */

  /* Send available bandwidth update to protocols */
  if (if_is_up (ifp))
    nsm_server_if_priority_bw_update (ifp);

 set_qos_flag:
  if (is_explicit == NSM_TRUE)
    {
      if (bandwidth > 0)
        SET_FLAG (ifp->conf_flags, NSM_MAX_RESV_BW_CONFIGURED);
      else
        UNSET_FLAG (ifp->conf_flags, NSM_MAX_RESV_BW_CONFIGURED);
    }

  return 0;
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
/* Update bw constraint attribute */
int
nsm_qos_serv_update_bw_constraint (struct interface *ifp, float32_t bandwidth,
                                   int ct_num)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct qos_interface *qos_if;
  struct nsm_if *nsm_if;
  int i;
  float32_t total_rsvd_bw, old_bw;
  int pri;

  if (bandwidth == ifp->bw_constraint[ct_num])
    return 0;

  if (bandwidth > ifp->max_resv_bw)
    return NSM_ERR_INVALID_BW_CONSTRAINT;

  nsm_if = ifp->info;
  if (! nsm_if)
    return NSM_ERR_INTERNAL;

  if (nsm_if->qos_if == NULL)
    {
      /* Create one */
      nsm_if->qos_if = nsm_qos_serv_if_init (ifp);
      if (! nsm_if->qos_if)
        return NSM_ERR_INTERNAL;
    }

  qos_if = nsm_if->qos_if;

  /* compute aggregate bw reserved for this class type */
  for (i = 0, pri = 0, total_rsvd_bw = 0; i < MAX_TE_CLASS; i++)
    {
      if (NSM_MPLS->te_class[i].ct_num == ct_num &&
          NSM_MPLS->te_class[i].priority > pri)
        {
          pri = NSM_MPLS->te_class[i].priority;
          total_rsvd_bw = qos_if->ct_aggr_rsvd_bw[i];
        }
      
      if (pri == MAX_PRIORITIES - 1)
        break;
    }
  

  old_bw = ifp->bw_constraint[ct_num];
  ifp->bw_constraint[ct_num] = bandwidth;

  if (bandwidth < old_bw && bandwidth < total_rsvd_bw)
    {
      struct list *preempt_list = list_new ();

      nsm_qos_serv_preempt_offenders (preempt_list,
                                      qos_if,
                                      (total_rsvd_bw - bandwidth),
                                      -1);
      if (LISTCOUNT (preempt_list))
        nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
      else
        list_delete (preempt_list);
    }

  /* Send bandwidth constraint update to clients */
  nsm_qos_send_bw_constraint_update (ifp);
  
  /* Update available bandwidth information */
  nsm_qos_serv_avail_bw_update (qos_if);

#ifdef HAVE_GMPLS
  nsm_data_link_property_copy (ifp, PAL_TRUE);
#endif /* HAVE_GMPLS */

  /* Send available bandwidth update to protocols */
  if (if_is_up (ifp))
    nsm_server_if_priority_bw_update (ifp);

  return 0;
}
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

/* De-initialize an interface */
void
nsm_qos_serv_if_deinit (struct interface *ifp, bool_t _delete,
                        bool_t remove_mpls_rib,
                        bool_t send_update)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_if *nif;
  struct qos_interface *qos_if;
  struct route_table *table;
  struct route_node *rn;
  struct list *preempt_list;
  int i;

  nif = ifp->info;
  qos_if = nif->qos_if;
  if (! qos_if)
    return;

  /* Initialize preempt_list. */
  preempt_list = list_new ();

  /* Deinitialize all eight route tables */
  for (i = (MAX_PRIORITIES - 1); i >= 0; i--)
    {
      /* Get table */
      table = qos_if->resource_array [i];
      
      for (rn = route_top (table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;
         
          if (preempt_list) 
            /* Add node to preempt list. */
            listnode_add (preempt_list, rn);
        }

      qos_if->aggr_rsvd_bw[i] = 0;

#ifndef HAVE_DSTE
      qos_if->ifp->tecl_priority_bw[i] = ifp->max_resv_bw;
#endif /* HAVE_DSTE */
    }

  /* Reset dste bandwidth info */
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  for (i = 0; i < MAX_TE_CLASS; i++)
    {
      qos_if->ct_aggr_rsvd_bw[i] = 0;
      nsm_qos_serv_te_class_avail_bw_update (qos_if, i);
    }
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */


  /* Set status to disabled. */
  qos_if->status = QOS_INTERFACE_DISABLED;
  
#ifdef HAVE_GMPLS
  nsm_data_link_property_copy (ifp, send_update);
#endif /* HAVE_GMPLS */

  /* Send available bandwidth update to clients */
  if (if_is_up (ifp))
    nsm_server_if_priority_bw_update (ifp);
  
  /* Preempt all resources collected above. */
  if (LISTCOUNT (preempt_list))
    nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_FALSE, remove_mpls_rib);
  else if (preempt_list)
    list_delete (preempt_list);
  
  /* If delete required, delete interface. */
  if (_delete == NSM_TRUE)
    {
      for (i = 0; i < MAX_PRIORITIES; i++)
        {
          route_table_finish (qos_if->resource_array[i]);
          qos_if->resource_array[i] = NULL;
        }
                            
      nif->qos_if = NULL;
      XFREE (MTYPE_NSM_QOS_IF, qos_if);
    }
}


int
qos_resource_add (struct route_table *table, 
                  struct qos_resource *resource)
{
  struct prefix p;
  struct route_node *rn;

  if (! table || ! resource)
    return QOS_ERR;

  /* Create resource key */
  QOS_RESOURCE_KEY (&p, resource->resource_id);
  
  /* Add resource to table */
  rn = route_node_get (table, &p);
  NSM_ASSERT (rn != NULL && rn->info == NULL);
  if (! rn)
    return QOS_ERR;
  
  if (rn->info != NULL)
    {
      zlog_err (NSM_ZG, "Internal Error: Resource id %d is already in use",
                resource->resource_id);
      route_unlock_node (rn);
      return QOS_ERR;
    }
  
  rn->info = resource;
  
  return QOS_OK;
}
 
 
static struct route_node *
qos_resource_node_lookup (struct route_table *table, struct prefix *p)
{
  struct route_node *rn;
  
  rn = route_node_lookup (table, p);
  if (rn)
    {
      route_unlock_node (rn);
      return rn;
    }

  return NULL;
}


struct qos_resource *
qos_resource_lookup (struct route_table *table, struct prefix *p)
{ 
  struct route_node *rn;
  
  rn = qos_resource_node_lookup (table, p);
  if (rn)
    return rn->info;

  return NULL;
}


struct qos_resource *
qos_resource_remove (struct route_table *table, u_int32_t resource_id)
{
  struct route_node *rn;
  struct prefix p;
  struct qos_resource *resource;

  QOS_RESOURCE_KEY (&p, resource_id);

  rn = qos_resource_node_lookup (table, &p);
  if (rn)
    {
      resource = rn->info;
      rn->info = NULL;
      route_unlock_node (rn);
      
      return resource;
    }

  return NULL;
}


/* Lookup for resource in all tables on the specified interface. */
static struct route_node *
qos_resource_node_lookup_if (u_int32_t id, struct qos_interface *qos_if)
{
  int i;
  struct prefix p;
  struct route_node *rn;
  struct route_table *table;

  /* Create resource key */
  QOS_RESOURCE_KEY (&p, id);
  
  for (i = 0; i < MAX_PRIORITIES; i++)
    {
      table = qos_if->resource_array [i];
      rn = qos_resource_node_lookup (table, &p);
      if (rn)
        return rn;
    }

  return NULL;
}



/* Lookup for resource in all tables on the specified interface. */
static struct qos_resource *
qos_resource_lookup_if (u_int32_t id, struct qos_interface *qos_if)
{
  int i;
  struct prefix p;
  struct route_table *table;
  struct qos_resource *resource;

  /* Create resource key */
  QOS_RESOURCE_KEY (&p, id);
  
  for (i = 0; i < MAX_PRIORITIES; i++)
    {
      table = qos_if->resource_array [i];
      resource = qos_resource_lookup (table, &p);
      if (resource)
        return resource;
    }

  return NULL;
}

int
nsm_qos_serv_release_slow_match (struct nsm_master *nm,
                                 struct qos_resource *resrc_data,
                                 u_int32_t ifindex)
{
  struct qos_interface *qos_if;
  struct route_table *table;
  struct qos_resource *resource;
  struct route_node *rn;

  qos_if = nsm_qos_serv_if_get (nm, ifindex);
  if (! qos_if)
    return 0;

  /* Get correct table. */
  table = qos_if->resource_array[(int)resrc_data->hold_priority];
  if (! table)
    return 0;

  rn = route_top (table);

  if (rn)
    {
      if ((resource = rn->info) != NULL
          && resource->protocol_id == resrc_data->protocol_id
          && mpls_owner_same (&resource->owner, &resrc_data->owner)
          && resource->flags == resrc_data->flags
          && pal_mem_cmp (&resource->t_spec, &resrc_data->t_spec,
                          sizeof (struct qos_traffic_spec)) == 0
          && pal_mem_cmp (&resource->if_spec, &resrc_data->if_spec,
                          sizeof (struct qos_if_spec)) == 0
          && pal_mem_cmp (&resource->ad_spec, &resrc_data->ad_spec,
                          sizeof (struct qos_ad_spec)) == 0
          && resource->setup_priority == resrc_data->setup_priority
          && resource->ct_num == resrc_data->ct_num)
        {
          nsm_qos_serv_bw_release_process (qos_if, 
                                           resource->t_spec.
                                           committed_bucket.rate,
                                           resource->hold_priority,
                                           resource->ct_num, resource);

          /* Send available bandwidth update to clients */
          if (if_is_up (qos_if->ifp))
            nsm_server_if_priority_bw_update (qos_if->ifp);
                  
          /* Remove route node. */
          rn->info = NULL;
          route_unlock_node (rn);
          route_unlock_node (rn);

          /* Free resource. */
          qos_common_resource_delete (resource);

        }
      else
        return 0;
    }
  return 1;
}

/* Slow release for a resource. */
int
nsm_qos_serv_release_slow (struct nsm_master *nm,
                           struct qos_resource *resrc_data)
{
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif = NULL;
  struct datalink *dl = NULL;
  struct telink *tl = NULL;
  struct avl_node *ln;
  u_int32_t ret = 0;
#endif /*HAVE_GMPLS*/
  u_int32_t ifindex = 0;

#ifdef HAVE_GMPLS
  /* Get interface. */
  gmif = gmpls_if_get (nzg, nm->vr->id);
          
  if (!gmif)
    return 0;
    
  if (CHECK_FLAG (resrc_data->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))
    {
      if (CHECK_FLAG (resrc_data->gif_spec.flags, 
                      GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX))
        {
          dl = data_link_lookup_by_index (gmif, 
                                          resrc_data->gif_spec.dl_gifindex);
          if (dl)  
            {
              if (dl->ifp)
                {
                  ifindex = dl->ifp->ifindex;
                  nsm_qos_serv_release_slow_match (nm, resrc_data, ifindex);
                }
              return 0;
            }
        }
      else if (CHECK_FLAG (resrc_data->gif_spec.flags, 
                           GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX))
        {
          tl = te_link_lookup_by_index (gmif, 
                                        resrc_data->gif_spec.tel_gifindex);
          if (tl)
            {
              AVL_TREE_LOOP (&tl->dltree, dl, ln)
                {
                  if (dl->ifp == NULL)
                    continue;

                  ifindex = dl->ifp->ifindex;
                  ret = nsm_qos_serv_release_slow_match (nm, resrc_data, 
                                                         ifindex);
                  if (ret == 1)
                    break;
                }
            }
        }
    }
  else
#endif /*HAVE_GMPLS*/
    if (CHECK_FLAG (resrc_data->flags, QOS_RESOURCE_FLAG_IF_SPEC))
      {
        ifindex = resrc_data->if_spec.out_ifindex;
        nsm_qos_serv_release_slow_match (nm, resrc_data, ifindex);
        return 0;
      }

  return 0;
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
int
nsm_qos_serv_te_class_lookup (struct nsm_master *nm,
                              int priority, int ct_num)
{
  int i;

  for (i = 0; i < MAX_TE_CLASS; i++)
    if (NSM_MPLS->te_class[i].priority == priority &&
        NSM_MPLS->te_class[i].ct_num == ct_num)
      return i;
  
  return TE_CLASS_INVALID;
}
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

/*
 * Will return QOS_ERR or QOS_OK, depending on whether the bandwidth
 * is acceptable or not.
 */
qos_status
nsm_qos_serv_adm_control (struct qos_interface *qos_if,
                          struct qos_resource *resource,
                          struct qos_resource *old_resource,
                          float32_t *weight, u_int32_t dl_gifindex)
{
  struct nsm_master *nm;
  struct interface *ifp;
  float32_t bw;
  int s_priority, h_priority;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct datalink *dl = NULL;
#endif /*HAVE_GMPLS*/

  if (weight)
    *weight = NEGATIVE_INFINITY;

  if (! qos_if || ! qos_if->ifp)
    return QOS_ERR;

  ifp = qos_if->ifp;
  nm = ifp->vr->proto;

  bw = resource->t_spec.committed_bucket.rate;
  if (bw > ifp->bandwidth)
    return QOS_ERR;

#ifdef HAVE_GMPLS
#ifdef HAVE_OLD_CODE
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_ATTR)) 
    {
      if (!CHECK_FLAG (ifp->phy_prop.switching_cap, resource->gmpls_attr.sw_cap))
        return QOS_ERR;
      if (resource->gmpls_attr.encoding != ifp->phy_prop.encoding_type)
        return QOS_ERR;
      if (resource->gmpls_attr.protection > ifp->phy_prop.protection)
        return QOS_ERR;
    }
#endif /* HAVE_OLD_CODE */
#ifdef HAVE_PACKET
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC)) 
    {
      if (bw < ifp->phy_prop.min_lsp_bw)
        return QOS_ERR;
    }
#endif /*HAVE_PACKET*/
#endif /*HAVE_GMPLS*/

  s_priority = resource->setup_priority;
  h_priority = resource->hold_priority;

#ifdef HAVE_GMPLS
  if (dl_gifindex)
    {
      gmif = gmpls_if_get (nzg, nm->vr->id);
      if (!gmif)
        return QOS_ERR;

      dl = data_link_lookup_by_index (gmif, dl_gifindex);
      if (!dl)
        return QOS_ERR;
    }
#endif /*HAVE_GMPLS*/

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DSTE
  {
    int ct_num, tecl_ix;

    ct_num = resource->ct_num;
    if (! IS_VALID_CT_NUM (ct_num))
      return QOS_ERR;
    
    tecl_ix = nsm_qos_serv_te_class_lookup (nm, h_priority, 
                                            ct_num);
    if (tecl_ix == TE_CLASS_INVALID)
      return QOS_ERR;
    
    if (h_priority != s_priority)
      {
        tecl_ix = nsm_qos_serv_te_class_lookup (nm, s_priority, ct_num);
        if (tecl_ix == TE_CLASS_INVALID)
          return QOS_ERR;
      }
    
    if (! old_resource)
      {
#ifdef HAVE_GMPLS
        if (dl)
          {
            if (bw <= dl->prop->max_lsp_bw[tecl_ix])
              {
                if (weight)
                  *weight = bw/ifp->bw_constraint[ct_num];
                return QOS_OK;
              }
          }
        else 
#endif /*HAVE_GMPLS*/
          if (bw <= ifp->tecl_priority_bw[tecl_ix])
            {
              if (weight)
                *weight = bw/ifp->bw_constraint[ct_num];
              return QOS_OK;
            }
      }
    else if (ct_num == old_resource->ct_num)
      {
#ifdef HAVE_GMPLS
        if (dl)
          {
            if (bw <= old_resource->t_spec.committed_bucket.rate ||
                bw - old_resource->t_spec.committed_bucket.rate  
                <= dl->prop->max_lsp_bw[tecl_ix])
              {
                if (weight)
                  *weight = bw/ifp->bw_constraint[ct_num];
                return QOS_OK;
              }
          } 
        else
#endif /*HAVE_GMPLS*/
          if (bw <= old_resource->t_spec.committed_bucket.rate ||
              bw - old_resource->t_spec.committed_bucket.rate <=
              ifp->tecl_priority_bw[tecl_ix])
            {
              if (weight)
                *weight = bw/ifp->bw_constraint[ct_num];
              return QOS_OK;
            }
      }
    else if (s_priority >= old_resource->hold_priority)
      {
        float32_t avail_bw;
        
        avail_bw = ifp->max_resv_bw - 
          qos_if->aggr_rsvd_bw[old_resource->hold_priority] -
          old_resource->t_spec.committed_bucket.rate;
        
#ifdef HAVE_GMPLS
        if (dl)
          {
            if (bw <= (avail_bw < dl->prop->max_lsp_bw[tecl_ix] ?
                       avail_bw : dl->prop->max_lsp_bw[tecl_ix])) 
              { 
                if (weight)
                  *weight = bw/ifp->bw_constraint[ct_num];
                return QOS_OK;
              }
          }
        else   
#endif /*HAVE_GMPLS*/
          if (bw <= (avail_bw < ifp->tecl_priority_bw[tecl_ix] ?
                     avail_bw : ifp->tecl_priority_bw[tecl_ix]))
            { 
              if (weight)
                *weight = bw/ifp->bw_constraint[ct_num];
              return QOS_OK;
            }
      }
    else
      { 
#ifdef HAVE_GMPLS
        if (dl)
          {
            if (bw <= dl->prop->max_lsp_bw[tecl_ix])
              {
                if (weight)
                  *weight = bw/ifp->bw_constraint[ct_num];
                return QOS_OK;
              }
          }
        else
#endif /*HAVE_GMPLS*/
          if (bw <= ifp->tecl_priority_bw[tecl_ix])
            {
              if (weight)
                *weight = bw/ifp->bw_constraint[ct_num];
              return QOS_OK;
            }
      }
  }
#else
  if (! old_resource)
    {
#ifdef HAVE_GMPLS
      if (dl)
        {
          if (bw <= dl->prop->max_lsp_bw [s_priority])
            {
              if (weight)
                *weight = bw/ifp->max_resv_bw;
              return QOS_OK;
            }
        }
      else 
#endif /*HAVE_GMPLS*/
        if (bw <= ifp->tecl_priority_bw [s_priority])
          {
            if (weight)
              *weight = bw/ifp->max_resv_bw;
            return QOS_OK;
          }
    }
  else 
    {
#ifdef HAVE_GMPLS
      if (dl)
        {
          if (bw <= old_resource->t_spec.committed_bucket.rate ||
              bw - old_resource->t_spec.committed_bucket.rate
              <= dl->prop->max_lsp_bw[s_priority])
            {
              if (weight)
                *weight = bw/ifp->max_resv_bw;
               return QOS_OK;
            }
        }
      else
#endif /*HAVE_GMPLS*/
        if (bw <= old_resource->t_spec.committed_bucket.rate ||
            bw - old_resource->t_spec.committed_bucket.rate <=
            ifp->tecl_priority_bw[s_priority])
              {
                if (weight)
                  *weight = bw/ifp->max_resv_bw;
                return QOS_OK;
              }
    }
#endif /* HAVE_DSTE */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
  
  return QOS_ERR;
}


/* Dummy routine to fill resource with data */
qos_status
nsm_qos_serv_fill_resource (struct nsm_master *nm,
                            struct qos_resource *resource, bool_t is_probe,
                            float32_t weight)
{
  struct qos_traffic_spec *tspec;
  struct qos_ad_spec *adspec;

  if (! is_probe)
    resource->resource_id = ++nm->resource_counter;
  
  /* Protocol should already be set, and should not be undefined */
  NSM_ASSERT (resource->protocol_id != APN_PROTO_UNSPEC);
  if (resource->protocol_id == APN_PROTO_UNSPEC)
    {
      zlog_warn (NSM_ZG, "Internal Error: Protocol id not set in resource");
      return QOS_ERR;
    }

  /* Traffic spec ... */
  if (qos_resource_check_traffic_spec (resource))
    {
      /* Get pointer to tspec */
      tspec = &resource->t_spec;

      /* Service type should already be there */
      NSM_ASSERT (tspec->service_type != QOS_TRAFFIC_TYPE_UNDEFINED);
      if (tspec->service_type == QOS_TRAFFIC_TYPE_UNDEFINED)
        {
          zlog_warn (NSM_ZG, "Internal Error: Service type for QOS resource "
                     "is undefined");
          return QOS_ERR;
        }

      /* Exclusive flag ignored */

      /*
       * Depending on flag, put data. Data MUST be in the
       * order specified in qos_common.h.
       */

      /* Peak bucket ... */
      if ((qos_tspec_check_peak_rate (tspec)) &&
          (tspec->peak_rate == NEGATIVE_INFINITY))
        tspec->peak_rate = POSITIVE_INFINITY;

      /* Committed bucket ... */
      if (qos_tspec_check_committed_bucket (tspec))
        {
          if ((! qos_bucket_check_rate (&tspec->committed_bucket))
              || (tspec->committed_bucket.rate == NEGATIVE_INFINITY))
            return QOS_ERR;

          if ((qos_bucket_check_size (&tspec->committed_bucket))
              && (tspec->committed_bucket.size == NEGATIVE_INFINITY))
            tspec->committed_bucket.size = tspec->committed_bucket.rate;
        }

      /* Excess burst ... */
      if ((qos_tspec_check_excess_burst (tspec)) &&
          (tspec->excess_burst == NEGATIVE_INFINITY))
        tspec->excess_burst = 1000;

      /* Weight of flow ... */
      if ((! is_probe) && (qos_tspec_check_weight (tspec)))
        tspec->weight = weight;

      /* Min policed unit ... */
      if ((qos_tspec_check_min_policed_unit (tspec)) &&
          (tspec->weight == NEGATIVE_INFINITY))
        tspec->mpu = 20;

      /* Max packet size ... */
      if ((qos_tspec_check_max_packet_size (tspec)) &&
          (resource->if_spec.out_ifindex != 0) &&
          (is_probe == NSM_TRUE) &&
          (tspec->max_packet_size == NEGATIVE_INFINITY))
        {
          struct qos_interface *qos_if;

          /* Get qos_if from outgoing interface and set mtu. */
          qos_if = nsm_qos_serv_if_get (nm, resource->if_spec.out_ifindex);
          NSM_ASSERT (qos_if != NULL);
          if (! qos_if)
            {
              zlog_warn (NSM_ZG, "Internal Error: Invalid interface index %d"
                         "specified in QOS resource",
                         resource->if_spec.out_ifindex);
              return QOS_ERR;
            }
          tspec->max_packet_size = qos_if->ifp->mtu;
        }
    }

  /* Dont need to fill Interface spec */
  
  /* Advertisement spec ... */
  if (qos_resource_check_ad_spec (resource))
    {
      /* Get adspec pointer */
      adspec = &resource->ad_spec;

      /* Slack term */
      if (qos_adspec_check_slack (adspec))
        adspec->slack = 0;

      /* Ctot */
      if (qos_adspec_check_ctot (adspec))
        adspec->Ctot = 96000;

      /* Dtot */
      if (qos_adspec_check_dtot (adspec))
        adspec->Dtot = 1;

      /* Csum */
      if (qos_adspec_check_csum (adspec))
        adspec->Csum = 96000;
      
      /* Dsum */
      if (qos_adspec_check_dsum (adspec))
        adspec->Dsum = 1;

      /* Minimum path latency */
      if (qos_adspec_check_mpl (adspec))
        adspec->mpl = 0;

      /* Path bandwidth estimate */
      if (qos_adspec_check_pbe (adspec))
        adspec->pbe = 0;
    }

  return QOS_OK;
}

/* Reserve bandwidth. */
qos_status
nsm_qos_serv_reserve_on_interface (struct nsm_master *nm,                                              
                                   struct qos_resource *resource, 
                                   struct list *preempt_list,
                                   u_int32_t ifindex,        
                                   u_int32_t dl_gifindex)        
{
  struct qos_interface *qos_if;
  struct route_table *table;
  float32_t bw, avail_bw = 0;
  struct interface *ifp;
  u_int32_t ret;
  float32_t weight;
#ifdef HAVE_GMPLS
  struct datalink *dl = NULL;
#endif /*HAVE_GMPLS*/  

  qos_if = nsm_qos_serv_if_get (nm, ifindex);
  if (! qos_if || ! qos_if->ifp)
    return QOS_ERR;
  
  bw = resource->t_spec.committed_bucket.rate;

  ifp = qos_if->ifp;

  /* admission control */
  ret = nsm_qos_serv_adm_control (qos_if, resource,
                                  NULL, &weight, dl_gifindex);
  if (ret == QOS_ERR)
    return ret;

  /* Fill resource with data. */
  ret = nsm_qos_serv_fill_resource (nm, resource, PAL_FALSE, weight);
  if (ret == QOS_ERR)
    return ret;

#ifdef HAVE_GMPLS
  if (dl_gifindex)
    {
      if (ifp->num_dl >1)  
        return QOS_ERR;
      dl = ifp->dlink.datalink;
      if (!dl)
        return QOS_ERR;
    }
#endif /*HAVE_GMPLS*/  

#ifdef HAVE_DSTE 
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
  avail_bw = ifp->max_resv_bw - qos_if->aggr_rsvd_bw[MAX_PRIORITIES - 1];
#endif /* !HAVE_GMPLS || HAVE_PACKET */
#else
#ifdef HAVE_GMPLS
  if (dl)
    avail_bw = dl->prop->max_lsp_bw[MAX_PRIORITIES - 1];
  else
#endif /* HAVE_GMPLS*/
    avail_bw = ifp->tecl_priority_bw[MAX_PRIORITIES - 1];
#endif /* HAVE_DSTE */

  
  /* Check if we need to pre-empt anyone */
  if (resource->setup_priority < MAX_PRIORITIES - 1 &&
      avail_bw < bw)
    {
      ret = nsm_qos_serv_preempt_offenders (preempt_list,
                                            qos_if,
                                            bw - avail_bw,
                                            resource->hold_priority);
      if (ret == QOS_ERR)
        return ret;
    }

  /*
   * Save this resource.
   */

  /* Find right table */
  /* The new resource will be stored in the table identified by */
  /* it's hold priority, and not by it's setup priority */
  table = qos_if->resource_array [resource->hold_priority];
  NSM_ASSERT (table != NULL);
  if (! table)
    {
      zlog_err (NSM_ZG, "Internal Error: Resource table not found");
      return QOS_ERR;
    }

  ret = qos_resource_add (table, resource);
  if (ret != QOS_OK)
    return ret;

  /* Update interface bandwidth data */
  nsm_qos_serv_bw_reserve_process (qos_if, bw, resource->hold_priority,
                                   resource->ct_num, resource);

  /* Send available bandwidth update to clients */
  if (if_is_up (ifp))
    nsm_server_if_priority_bw_update (ifp);

  return QOS_OK;
}

/* Reserve bandwidth. */
qos_status
nsm_qos_serv_reserve (struct nsm_master *nm,
                      struct qos_resource *resource, struct list *preempt_list)
{
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct datalink *dl;
  struct telink *tl;
  struct avl_node *ln, *ln1;
  u_int32_t dl_gifindex = 0;
  u_int32_t tl_gifindex = 0;
  struct pal_in4_addr rtr_id;
#endif /*HAVE_GMPLS*/
  u_int32_t ifindex = 0;
  u_int32_t ret = QOS_ERR;

#ifdef HAVE_GMPLS
  pal_mem_set (&rtr_id, 0, sizeof (struct pal_in4_addr));
#endif /*HAVE_GMPLS*/

  /* There MUST be an outgoing interface and committed rate defined. */
  if (! resource 
      || ! qos_bucket_check_rate (&resource->t_spec.committed_bucket)
      || resource->t_spec.committed_bucket.rate == NEGATIVE_INFINITY)
    return QOS_ERR;

  /* Get the interface */
#ifdef HAVE_GMPLS
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))
    {
      if (CHECK_FLAG (resource->gif_spec.flags, 
                      GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX))
        dl_gifindex = resource->gif_spec.dl_gifindex;
      else if (CHECK_FLAG (resource->gif_spec.flags, 
                           GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX))
        tl_gifindex = resource->gif_spec.tel_gifindex;
      else if (CHECK_FLAG (resource->gif_spec.flags, 
                           GMPLS_QOS_IFSPEC_FLAG_CA_RTR_ID))
        rtr_id = resource->gif_spec.rtr_id;
      if ((tl_gifindex == 0) && (dl_gifindex == 0) && (rtr_id.s_addr == 0))
        return QOS_ERR;
    } 
  else
#endif /*HAVE_GMPLS*/
    if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_IF_SPEC))
      {
        ifindex = resource->if_spec.out_ifindex;
        if (ifindex == 0)
          return QOS_ERR;
        return nsm_qos_serv_reserve_on_interface (nm, resource, preempt_list, ifindex, 0);
      }

#ifdef HAVE_GMPLS
  gmif = gmpls_if_get (nzg, nm->vr->id);
  if (!gmif)
    return QOS_ERR;

  if (dl_gifindex)
    {
      dl = data_link_lookup_by_index (gmif, dl_gifindex);
      if (dl)
        {
          if (dl->ifp == NULL)
            return QOS_ERR;

          ifindex = dl->ifp->ifindex;
          if (ifindex == 0)
            return QOS_ERR;
          return nsm_qos_serv_reserve_on_interface (nm, resource, preempt_list,
                                                    ifindex, dl_gifindex);
        }
    }

  else if (tl_gifindex)
    {
      tl = te_link_lookup_by_index (gmif, tl_gifindex);
      if (tl)
        {
          AVL_TREE_LOOP (&tl->dltree, dl, ln)
            {
              if (dl->ifp == NULL)
                continue;

              ifindex = dl->ifp->ifindex;
              if (ifindex == 0)
                continue;

              ret = nsm_qos_serv_reserve_on_interface (nm, resource, 
                                                       preempt_list, ifindex, 
                                                       dl->gifindex);
              if (ret == QOS_OK)
                {
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX);
                  resource->gif_spec.dl_gifindex = dl->gifindex;
                  return ret;
                }
            }
        }
    }
  else if (rtr_id.s_addr)
    {
      /* If Neither TE Link gifindex or Data Link gifindex is sent
      Loop Through TE Links and for each TE Link loop through 
      Data Links and check resources for each Data Link*/ 
      AVL_TREE_LOOP (&gmif->teltree, tl, ln)
        {
          if (!IPV4_ADDR_SAME (&tl->prop.linkid, &rtr_id))
            continue;  
          AVL_TREE_LOOP (&tl->dltree, dl, ln1)
            {
              if (dl->ifp == NULL)
                continue;

              ifindex = dl->ifp->ifindex;
              if (ifindex == 0)
                continue;
              ret = nsm_qos_serv_reserve_on_interface (nm, resource, 
                                                       preempt_list, ifindex, 
                                                       dl->gifindex);
              if (ret == QOS_OK)
                {
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX);
                  resource->gif_spec.dl_gifindex = dl->gifindex;  
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX);
                  resource->gif_spec.tel_gifindex = tl->gifindex;  
                  return ret;
                } 
            }
        }
    }
#endif /*HAVE_GMPLS*/
  return ret;
}

qos_status
nsm_qos_serv_modify_on_interface (struct nsm_master *nm,
                                  struct qos_resource *resource, 
                                  struct list *preempt_list,
                                  u_int32_t ifindex, 
                                  u_int32_t dl_gifindex) 
{
  float32_t weight, needed_bw;
  struct interface *ifp;
  struct qos_resource *old_resource;
  struct qos_interface *qos_if;
  struct route_table *table;
  u_int32_t ret;
  bool_t prio_change = NSM_FALSE;
  float32_t bw, old_bw;
#ifdef HAVE_GMPLS
  struct datalink *dl = NULL;
#endif /*HAVE_GMPLS*/

  /* Get QoS interface. */
  qos_if = nsm_qos_serv_if_get (nm, ifindex);
  if (! qos_if)
    return QOS_ERR;

  ifp = qos_if->ifp;
  
  /* Find old resource. */
  old_resource = qos_resource_lookup_if (resource->resource_id, qos_if);
  if (! old_resource)
    return QOS_ERR;

  /* admission control */
  ret = nsm_qos_serv_adm_control (qos_if, resource,
                                  old_resource, &weight,
                                  dl_gifindex);
  if (ret != QOS_OK)
    return ret;

  bw = resource->t_spec.committed_bucket.rate;
  old_bw = old_resource->t_spec.committed_bucket.rate;

  /* Handle change in hold priority. */
  if (old_resource->hold_priority != resource->hold_priority)
    {
      table = qos_if->resource_array [old_resource->hold_priority];

      /* Remove resource from table */
      qos_resource_remove (table, old_resource->resource_id);

      /* Update interface bandwidth information */
      nsm_qos_serv_bw_release_process (qos_if, old_bw,
                                       old_resource->hold_priority, 
                                       old_resource->ct_num, old_resource);
      
      /* Set needed bandwidth to the new bandwidth. */
      needed_bw = bw;
      
      /* Set prio change flag. */
      prio_change = NSM_TRUE;
    }
  else
    /* If needed_bw is negative, we are releasing part of the original bw.
       This change will automatically handled below when we decrement the
       original committed bucket rate by the needed_bw. */
    needed_bw = bw - old_bw;
  
#ifdef HAVE_GMPLS
  if (dl_gifindex)
    {
      if (ifp->num_dl >1)  
        return QOS_ERR;
      dl = ifp->dlink.datalink;
      if (!dl)
        return QOS_ERR;
    } 
#endif /*HAVE_GMPLS*/  
    
  if (needed_bw > 0)
    {
      float32_t avail_bw = 0;
      
#ifdef HAVE_DSTE
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
      avail_bw = ifp->max_resv_bw - qos_if->aggr_rsvd_bw[MAX_PRIORITIES - 1];
#endif /* !HAVE_GMPLS || HAVE_PACKET */
#else
#ifdef HAVE_GMPLS
     if (dl)
       avail_bw = dl->prop->max_lsp_bw[MAX_PRIORITIES - 1];
     else                                          
#endif /* HAVE_GMPLS */             
       avail_bw = ifp->tecl_priority_bw[MAX_PRIORITIES - 1];
#endif /* HAVE_DSTE */

      /* Check for preemption */
      if (resource->setup_priority < MAX_PRIORITIES - 1 &&
          avail_bw < needed_bw)
        {
          ret = nsm_qos_serv_preempt_offenders (preempt_list,
                                                qos_if,
                                                needed_bw - avail_bw,
                                                resource->setup_priority);
          if (ret == QOS_ERR)
            return ret;
        }
    }

  /* Set new resource committed bandwidth. */
  old_resource->t_spec.committed_bucket.rate = bw;

  /* Update priorities. */
  old_resource->hold_priority = resource->hold_priority;
  old_resource->setup_priority = resource->setup_priority;
  
  /* Update interface bandwidth data */
  if (needed_bw < 0)
    nsm_qos_serv_bw_release_process (qos_if, 0 - needed_bw, 
                                     resource->hold_priority,
                                     resource->ct_num, resource);
  else
    {
      nsm_qos_serv_bw_reserve_process (qos_if, needed_bw, 
                                       resource->hold_priority,
                                       resource->ct_num, resource);
    }
  
  /* Move old_resource to new table. */
  if (prio_change == NSM_TRUE)
    {
      /* Get new table and add to it. */
      table = qos_if->resource_array[old_resource->hold_priority];
      
      ret = qos_resource_add (table, old_resource);
      if (ret != QOS_OK)
        return ret;
    }
  
  /* Send available bandwidth update to clients */
  if (if_is_up (ifp))
    nsm_server_if_priority_bw_update (ifp);

  return QOS_OK;
}

/* Modify already reserved bandwidth. */
qos_status
nsm_qos_serv_modify (struct nsm_master *nm,
                     struct qos_resource *resource, struct list *preempt_list)
{
  u_int32_t ret;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct datalink *dl;
  struct telink *tl;
  struct avl_node *ln, *ln1;
  u_int32_t dl_gifindex = 0;
  u_int32_t tl_gifindex = 0;
  struct pal_in4_addr rtr_id;
#endif /*HAVE_GMPLS*/
  u_int32_t ifindex = 0;

  ret = QOS_ERR;
#ifdef HAVE_GMPLS
  pal_mem_set (&rtr_id, 0, sizeof (struct pal_in4_addr));
#endif /*HAVE_GMPLS*/

  /* There MUST be an outgoing interface and committed rate defined. */
  if ((! qos_bucket_check_rate (&resource->t_spec.committed_bucket))
      || (resource->t_spec.committed_bucket.rate == NEGATIVE_INFINITY))
    return QOS_ERR;

  /* Get the interface */
#ifdef HAVE_GMPLS
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))
    {
      if (CHECK_FLAG (resource->gif_spec.flags, GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX))
        dl_gifindex = resource->gif_spec.dl_gifindex;
      else if (CHECK_FLAG (resource->gif_spec.flags, GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX))
        tl_gifindex = resource->gif_spec.tel_gifindex;
      else if (CHECK_FLAG (resource->gif_spec.flags, GMPLS_QOS_IFSPEC_FLAG_CA_RTR_ID))
        rtr_id = resource->gif_spec.rtr_id;
      if ((tl_gifindex == 0) && (dl_gifindex == 0) && (rtr_id.s_addr == 0))
        return QOS_ERR;
    }
  else
#endif /*HAVE_GMPLS*/
    if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_IF_SPEC))
      {
        ifindex = resource->if_spec.out_ifindex;
        if (ifindex == 0)
          return QOS_ERR;
        return nsm_qos_serv_modify_on_interface (nm, resource, preempt_list, ifindex, 0);
      }

#ifdef HAVE_GMPLS
  gmif = gmpls_if_get (nzg, nm->vr->id);
  if (!gmif)
    return QOS_ERR;

  if (dl_gifindex)
    {
      dl = data_link_lookup_by_index (gmif, dl_gifindex);
      if (dl)
        {
          if (dl->ifp == NULL)
            return QOS_ERR;

          ifindex = dl->ifp->ifindex;
          if (ifindex == 0)
            return QOS_ERR;
          return nsm_qos_serv_modify_on_interface (nm, resource, preempt_list, 
                                                   ifindex, dl_gifindex);
        }
    }

  else if (tl_gifindex)
    {
      tl = te_link_lookup_by_index (gmif, tl_gifindex);
      if (tl)
        {
          AVL_TREE_LOOP (&tl->dltree, dl, ln)
            {
              if (dl->ifp == NULL)
                continue;

              ifindex = dl->ifp->ifindex;
              if (ifindex == 0)
                continue;
              ret = nsm_qos_serv_modify_on_interface (nm, resource, 
                                                      preempt_list, ifindex, 
                                                      dl->gifindex);
              if (ret == QOS_OK)
                {
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX);
                  resource->gif_spec.dl_gifindex = dl->gifindex;
                  return ret;
                }
            }
        }
    }
  else if (rtr_id.s_addr)
    {
      /* If Neither TE Link gifindex or Data Link gifindex is sent
      Loop Through TE Links and for each TE Link loop through 
      Data Links and check resources for each Data Link*/ 
      AVL_TREE_LOOP (&gmif->teltree, tl, ln)
        {
          if (!IPV4_ADDR_SAME (&tl->prop.linkid, &rtr_id))
            continue;  
          AVL_TREE_LOOP (&tl->dltree, dl, ln1)
            {
              if (dl->ifp == NULL)
                continue;

              ifindex = dl->ifp->ifindex;
              if (ifindex == 0)
                continue;

              ret = nsm_qos_serv_modify_on_interface (nm, resource, 
                                                      preempt_list, ifindex, 
                                                      dl->gifindex);
              if (ret == QOS_OK)
                {
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX);
                  resource->gif_spec.dl_gifindex = dl->gifindex;  
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX);
                  resource->gif_spec.tel_gifindex = tl->gifindex;  
                  return ret;
                } 
            }
        }
    }
#endif /*HAVE_GMPLS*/
  return ret;
}
  
qos_status
nsm_qos_serv_probe_on_interface (struct nsm_master *nm, 
                                 struct qos_resource *resource,
                                 u_int32_t ifindex, u_int32_t dl_gifindex)
{
  float32_t weight;
  struct qos_resource *old_resource = NULL;
  struct qos_interface *qos_if;
  u_int32_t ret;

  /* Get outgoing interface. */
  qos_if = nsm_qos_serv_if_get (nm, ifindex);
  NSM_ASSERT (qos_if != NULL);
  if (! qos_if)
    {
      zlog_err (NSM_ZG, "Internal Error: No QOS interface found "
                "for index %d",
                resource->if_spec.out_ifindex);
      return QOS_ERR;
    }
  
  if (resource->resource_id > 0)
    {
      /* Get stored LSP resource object and compare attributes. */
      old_resource = qos_resource_lookup_if (resource->resource_id, 
                                             qos_if);
      if (! old_resource)
        {
          zlog_err (NSM_ZG, "Internal Error: No previous record of "
                    "resource (with id %d) found",
                    resource->resource_id);
          return QOS_ERR;
        }
    }
  
  ret = nsm_qos_serv_adm_control (qos_if,
                                  resource,
                                  old_resource,
                                  &weight, dl_gifindex);
  if (ret == QOS_ERR)
    return ret;
     
  /* Fill resource with data. */
  ret = nsm_qos_serv_fill_resource (nm, resource, NSM_TRUE, weight);
  if (ret == QOS_ERR)
    return ret;
  
  return QOS_OK;
}

/* Probe for bw */
qos_status
nsm_qos_serv_probe (struct nsm_master *nm, struct qos_resource **resrc)
{
  struct qos_resource *resource;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif; 
  struct telink *tl;
  struct avl_node *ln, *ln1;
  u_int32_t dl_gifindex = 0;
  u_int32_t tl_gifindex = 0;
  struct pal_in4_addr rtr_id;
  struct datalink *dl = NULL;
  struct datalink *selected_dl = NULL;
  struct telink *selected_tl = NULL;
  struct gmpls_gen_label out_label;
  struct addr_in nh_addr;
  int i = 0;
  int start_range;
  int end_range;
#endif /*HAVE_GMPLS*/
  u_int32_t ifindex = 0;
  u_int32_t ret = QOS_ERR;

  /* Set resource pointer */
  resource = *resrc;
#ifdef HAVE_GMPLS  
  pal_mem_set (&rtr_id, 0, sizeof (struct pal_in4_addr));
#endif /*HAVE_GMPLS*/

  /* There MUST be an outgoing interface and committed rate defined */
  if ((! qos_bucket_check_rate (&resource->t_spec.committed_bucket))
      || (resource->t_spec.committed_bucket.rate == NEGATIVE_INFINITY))
    return QOS_ERR;

#ifdef HAVE_GMPLS  
  if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC))
    {
      if (CHECK_FLAG (resource->gif_spec.flags, GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX))  
        dl_gifindex = resource->gif_spec.dl_gifindex;
      else if (CHECK_FLAG (resource->gif_spec.flags, GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX))  
        tl_gifindex = resource->gif_spec.tel_gifindex;
      else if (CHECK_FLAG (resource->gif_spec.flags, GMPLS_QOS_IFSPEC_FLAG_CA_RTR_ID))  
        rtr_id = resource->gif_spec.rtr_id;
      if ((tl_gifindex == 0) && (dl_gifindex == 0) && (rtr_id.s_addr == 0)) 
        return QOS_ERR;
     
    }  
  else
#endif /*HAVE_GMPLS*/
    if (CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_IF_SPEC))
      {
        ifindex = resource->if_spec.out_ifindex;
        if (ifindex == 0)
          return QOS_ERR;
        return nsm_qos_serv_probe_on_interface (nm, resource, ifindex, 0);
      }
      
#ifdef HAVE_GMPLS  
  gmif = gmpls_if_get (nzg, nm->vr->id);
  if (!gmif)
    return QOS_ERR;

  if (dl_gifindex)
    {
      dl = data_link_lookup_by_index (gmif, dl_gifindex);
      if (dl)
        {
          if (dl->ifp == NULL)
            return QOS_ERR;

          ifindex = dl->ifp->ifindex;
          if (ifindex == 0)
            return QOS_ERR;
          ret = nsm_qos_serv_probe_on_interface (nm, resource, ifindex, 
                                                 dl_gifindex);
          selected_dl = dl;
        }
    }

  else if (tl_gifindex)
    {
      tl = te_link_lookup_by_index (gmif, tl_gifindex);
      if (tl)
        {
          AVL_TREE_LOOP (&tl->dltree, dl, ln)
            {
              if (dl->ifp == NULL)
                continue;
             
              ifindex = dl->ifp->ifindex;
              if (ifindex == 0)
                continue;
             
              ret = QOS_OK;

              /* Not doing a Qos probe in case of PBB_TE data link interface */ 
              if (!INTF_IS_PBB_TE_CAPABLE(dl->ifp))
                ret = nsm_qos_serv_probe_on_interface (nm, resource, ifindex, 
                                                       dl->gifindex);

              if (ret == QOS_OK)
                {
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX);
                  resource->gif_spec.dl_gifindex = dl->gifindex;  
                  selected_dl = dl;
                  break;
                } 
            }
        } 
    }
  else if (rtr_id.s_addr)
    {
      AVL_TREE_LOOP (&gmif->teltree, tl, ln)
        {
          if (!IPV4_ADDR_SAME (&tl->prop.linkid, &rtr_id))
            continue;  
          AVL_TREE_LOOP (&tl->dltree, dl, ln1)
            {
              if (dl->ifp == NULL)
                continue;

              ifindex = dl->ifp->ifindex;
              if (ifindex == 0)
                continue;
              ret = nsm_qos_serv_probe_on_interface (nm, resource, ifindex, 
                                                     dl->gifindex);
              if (ret == QOS_OK)
                {
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX);
                  resource->gif_spec.dl_gifindex = dl->gifindex;  
                  selected_dl = dl;
                  SET_FLAG (resource->gif_spec.flags, 
                            GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX);
                  resource->gif_spec.tel_gifindex = tl->gifindex;  
                  break;  
                } 
            }
        }
    }
  if ((CHECK_FLAG (resource->flags, QOS_RESOURCE_FLAG_GMPLS_IF_SPEC)) &&
      (selected_dl))
    {
      selected_tl = selected_dl->telink;
      if (selected_tl != NULL)
        {
          if (CHECK_FLAG (selected_tl->flags, GMPLS_TE_LINK_NUMBERED))
            {
              nh_addr.afi = AFI_IP;
              IPV4_ADDR_COPY (&nh_addr.u.ipv4, 
                              &selected_tl->remote_addr.u.prefix4);
            }
          else
            {
              nh_addr.afi = AFI_UNNUMBERED;
              pal_mem_cpy (&nh_addr.u.ipv4, &selected_tl->r_linkid.linkid.lid,
                           sizeof (struct pal_in4_addr));
            }
        }
#ifdef HAVE_PACKET
      if (resource->gmpls_attr.encoding == GMPLS_ENCODING_TYPE_PACKET)
        {
          start_range = resource->label_spec.sl_lbl_set_from; 
          end_range = resource->label_spec.sl_lbl_set_to; 
          out_label.type = gmpls_entry_type_ip;
          for (i = start_range; i < end_range; i++)
            {
              out_label.u.pkt = i;
              if (nsm_gmpls_nhlfe_lookup (nm, &nh_addr, dl->gifindex, 
                  &out_label, PAL_FALSE) == NULL)             
                {
                  resource->label_spec.sugg_label = out_label;
                  SET_FLAG (resource->label_spec.flags, GMPLS_QOS_LABEL_SPEC_SUGG_LABEL);
                  break;
                }
            }
        }
#endif
    }
#endif /*HAVE_GMPLS*/
  
  return ret;
}

/* 
   Cleanup for a protocol. If ifindex is zero, cleanup all. 
   Otherwise clean the indicated interface. 
*/
void
nsm_qos_serv_clean_for (struct nsm_master *nm,
                        u_char protocol, u_int32_t ifindex)
{
  struct route_table *table;
  struct route_node *r1, *r2;
  struct qos_resource *resource;
  struct interface *ifp;
  struct nsm_if *nif;
  int i;
  float32_t bw;

  /* Go through all interfaces */
  for (r1 = route_top (nm->vr->ifm.if_table); r1; r1 = route_next (r1))
    {
      if ((ifp = r1->info) != NULL && (nif = ifp->info) != NULL &&
          nif->qos_if != NULL && (ifindex == 0 || ifindex == ifp->ifindex))
        {
          /* Go through all tables */
          for (i = 0; i < MAX_PRIORITIES; i++)
            {
              table = nif->qos_if->resource_array[i];
              
              /* Go through table */
              for (r2 = route_top (table); r2; r2 = route_next (r2))
                {
                  if ((resource = r2->info) != NULL && 
                      resource->protocol_id == protocol)
                    {
                      bw = resource->t_spec.committed_bucket.rate;
                      
                      nsm_qos_serv_bw_release_process (nif->qos_if, bw, i,
                                                       resource->ct_num, 
                                                       resource);
                      r2->info = NULL;
                      route_unlock_node (r2);
                      
                      /* Delete resource. */
                      qos_common_resource_delete (resource);
                    }
                }
            }
          
          /* Send available bandwidth update to clients */
          if (if_is_up (ifp))
            nsm_server_if_priority_bw_update (ifp);
        }
    }
}

#ifdef HAVE_GMPLS
/* 
   Cleanup for a protocol. If ifindex is zero, cleanup all. 
   Otherwise clean the indicated interface. 
*/
void
nsm_gmpls_qos_serv_clean_for (struct nsm_master *nm,
                              u_char protocol, u_int32_t tl_gifindex,
                              u_int32_t dl_gifindex)        
{
  struct gmpls_if *gmif = NULL;
  struct datalink *dl = NULL;
  struct telink *tl = NULL;
  struct avl_node *ln;
  u_int32_t ifindex;

  gmif = gmpls_if_get (nzg, nm->vr->id);
  if (!gmif)
    return;

  if (dl_gifindex) 
    {
      dl = data_link_lookup_by_index (gmif, dl_gifindex);
      if (dl)
        {
          if (dl->ifp)
            {
              ifindex = dl->ifp->ifindex;
              nsm_qos_serv_clean_for (nm, protocol, ifindex);
            }
        }
    }
  else if (tl_gifindex)
    {
      tl = te_link_lookup_by_index (gmif, tl_gifindex);
      if (tl)
        {
          AVL_TREE_LOOP (&tl->dltree, dl, ln)
            { 
              if (dl->ifp)
                {
                  ifindex = dl->ifp->ifindex;
                  nsm_qos_serv_clean_for (nm, protocol, ifindex);
                }
            }
        } 
    }
  else
    nsm_qos_serv_clean_for (nm, protocol, 0);
  return;
}
#endif /*HAVE_GMPLS*/

int
nsm_read_qos_client_init (struct nsm_msg_header *header,
                          void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  u_int32_t *protocol_id;
  struct nsm_server_client *nsc;
  struct nsm_master *nm;

  nse = arg;
  ns = nse->ns;
  protocol_id = message;
  nsc = NULL;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "QOS Module: Initializing for protocol: %s",
               modname_strl (*protocol_id));

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Clean system. */
  nsm_qos_serv_clean_for (nm, *protocol_id, 0);

  return 0;
}


int
nsm_read_qos_client_clean (struct nsm_msg_header *header,
                           void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_qos_clean *clean;
  struct nsm_server_client *nsc;
  u_int32_t ifindex;
  struct nsm_master *nm;

  nse = arg;
  ns = nse->ns;
  clean = message;
  nsc = NULL;

  /* Dump QOS release message. */
  if (IS_NSM_DEBUG_RECV)
    nsm_qos_clean_dump (ns->zg, clean);

  zlog_info (NSM_ZG, "QOS Module: Clean for protocol %s",
             modname_strl (clean->protocol_id));
  
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Get interface */
  if (NSM_CHECK_CTYPE (clean->cindex, NSM_QOS_CLEAN_CTYPE_IFINDEX))
    ifindex = clean->ifindex;
  else
    ifindex = 0;

  /* Cleanup. */
  nsm_qos_serv_clean_for (nm, clean->protocol_id, ifindex);

  return 0;
}

int
nsm_read_qos_client_reserve (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_qos *qos;
  struct nsm_msg_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;
  struct list *preempt_list;
  qos_status status;
  int nbytes;

  /* Preset status to failure */
  preempt_list = NULL;
  status = QOS_OK;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = nse->nsc;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map (qos, resource);
 
  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: RESERVE for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS release message. */
      nsm_qos_dump (ns->zg, qos, header->type);
    }

  /* Reserve and send result */
  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos));

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (status == QOS_OK)
    {
      preempt_list = list_new ();
      status = nsm_qos_serv_reserve (nm, resource, preempt_list); 
    }

  if (status == QOS_OK)
    {
      qos_prot_nsm_message_map (resource, &msg);
    }
  else
    {
      msg.resource_id = resource->resource_id;
      msg.protocol_id = resource->protocol_id;
      pal_mem_cpy (&msg.owner, &resource->owner, sizeof (struct mpls_owner));
      qos_common_resource_delete (resource);
    }

  /* Status */
  NSM_SET_CTYPE (msg.cindex, NSM_QOS_CTYPE_STATUS);
  msg.status = status;

  nbytes = nsm_encode_qos (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    {
      if (preempt_list)
        list_delete (preempt_list);
      return nbytes;
    }
  
  nsm_server_send_message (nse, header->vr_id, header->vrf_id,
                           header->type, header->message_id, nbytes);

  if (LISTCOUNT (preempt_list))
    nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
  else if (preempt_list)
    list_delete (preempt_list);

  return 0;
}

int
nsm_read_qos_client_probe (struct nsm_msg_header *header,
                           void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_qos *qos;
  struct nsm_msg_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;
  struct list *preempt_list;
  qos_status status;
  int nbytes;

  /* Preset status to failure */
  preempt_list = NULL;
  status = QOS_OK;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = nse->nsc;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map (qos, resource);

  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: PROBE for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS probe message. */
      nsm_qos_dump (ns->zg, qos, header->type);
    }

  /* Probe and send result */
  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos));

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (status == QOS_OK)
    status = nsm_qos_serv_probe (nm, &resource); 
  if (status == QOS_OK)
    {
      qos_prot_nsm_message_map (resource, &msg);
    }
  else
    {
      msg.resource_id = resource->resource_id;
      msg.protocol_id = resource->protocol_id;
      msg.id = resource->id;
      pal_mem_cpy (&msg.owner, &resource->owner, sizeof (struct mpls_owner));
    }

  /* Status */
  NSM_SET_CTYPE (msg.cindex, NSM_QOS_CTYPE_STATUS);
  msg.status = status;

  nbytes = nsm_encode_qos (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    {
      qos_common_resource_delete (resource);
      return nbytes;
    }
  
  nsm_server_send_message (nse, header->vr_id, header->vrf_id,
                           header->type, header->message_id, nbytes);

  /* Delete the dummy resource object */
  qos_common_resource_delete (resource);

  return 0;
}

int
nsm_read_qos_client_modify (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_qos *qos;
  struct nsm_msg_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;
  struct list *preempt_list;
  qos_status status;
  int nbytes;

  /* Preset status to failure */
  preempt_list = NULL;
  status = QOS_ERR;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = NULL;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map (qos, resource);

  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: MODIFY for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS modify message. */
      nsm_qos_dump (ns->zg, qos, header->type);
    }

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos));

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  preempt_list = list_new ();
  status = nsm_qos_serv_modify (nm, resource, preempt_list);
  if (status == QOS_OK)
    {
      qos_prot_nsm_message_map (resource, &msg);
    }
  else
    {
      msg.resource_id = resource->resource_id;
      msg.protocol_id = resource->protocol_id;
      msg.id = resource->id;
      pal_mem_cpy (&msg.owner, &resource->owner, sizeof (struct mpls_owner));
    }

  /* Status */
  NSM_SET_CTYPE (msg.cindex, NSM_QOS_CTYPE_STATUS);
  msg.status = status;
  
  nbytes = nsm_encode_qos (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    {
      list_delete (preempt_list);
      qos_common_resource_delete (resource);
      return nbytes;
    }
  
  nsm_server_send_message (nse, header->vr_id, header->vrf_id,
                           header->type, header->message_id, nbytes);

    /* Preempt all resources in this list */
  if (LISTCOUNT (preempt_list))
    nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
  else
    list_delete (preempt_list);

  /* Delete the dummy resource object. */
  qos_common_resource_delete (resource);

  return 0;
}

int
nsm_read_qos_client_release (struct nsm_msg_header *header,
                             void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_qos_release *release;
  struct nsm_server_client *nsc;
  struct qos_interface *qos_if;
  struct qos_resource *resource;
  struct route_node *rn;

  nse = arg;
  ns = nse->ns;
  release = message;
  nsc = NULL;

  /* Dump QOS release message. */
  if (IS_NSM_DEBUG_RECV)
    nsm_qos_release_dump (ns->zg, release);

  zlog_info (NSM_ZG, "QOS Module: Release for resource with id %d for "
             "protocol %s",
             release->resource_id, modname_strl (release->protocol_id));

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /*
   * Release resource.
   */

  /* Get interface */
  if (NSM_CHECK_CTYPE (release->cindex, NSM_QOS_RELEASE_CTYPE_IFINDEX))
    {
      qos_if = nsm_qos_serv_if_get (nm, release->ifindex);
      if (! qos_if)
        return 0;
    }
  else
    return 0;

  /* Brute force search for resource based on resource id. */
  rn = qos_resource_node_lookup_if (release->resource_id, qos_if);
  if (rn)
    {
      resource = rn->info;
      if (resource->protocol_id == release->protocol_id)
        {
          nsm_qos_serv_bw_release_process (qos_if, 
                                           resource->t_spec.
                                           committed_bucket.rate,
                                           resource->hold_priority,
                                           resource->ct_num, resource);

          /* Send available bandwidth update to clients */
          if (if_is_up (qos_if->ifp))
            nsm_server_if_priority_bw_update (qos_if->ifp);
          
          /* Remove route node. */
          rn->info = NULL;
          route_unlock_node (rn);
          qos_common_resource_delete (resource);
        }
    }

  return 0;
}


int
nsm_read_qos_client_release_slow (struct nsm_msg_header *header,
                                  void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_qos *qos;
  struct nsm_msg_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = NULL;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map (qos, resource);

  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: SLOW RELEASE for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS modify message. */
      nsm_qos_dump (ns->zg, qos, NSM_MSG_QOS_CLIENT_RELEASE);
    }

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos));

  /* Free resource. */
  nsm_qos_serv_release_slow (nm, resource);

  /* Free up temp resource. */
  qos_common_resource_delete (resource);

  return 0;
}

#ifdef HAVE_GMPLS
int
nsm_read_gmpls_qos_client_init (struct nsm_msg_header *header,
                                void *arg, void *message)
{
  return nsm_read_qos_client_init (header, arg, message);
}

int
nsm_read_gmpls_qos_client_clean (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_gmpls_qos_clean *clean;
  struct nsm_server_client *nsc;
  struct nsm_master *nm;
  u_int32_t dl_gifindex = 0;
  u_int32_t tl_gifindex = 0;

  nse = arg;
  ns = nse->ns;
  clean = message;
  nsc = NULL;

  /* Dump QOS release message. */
  if (IS_NSM_DEBUG_RECV)
    nsm_gmpls_qos_clean_dump (ns->zg, clean);

  zlog_info (NSM_ZG, "QOS Module: Clean for protocol %s",
             modname_strl (clean->protocol_id));
  
  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Get interface */
  /* If Data Link GifIndex is given, clean resources for interface for this data Link */
  if (NSM_CHECK_CTYPE (clean->cindex, NSM_GMPLS_QOS_CLEAN_CTYPE_DL_GIFINDEX))
    dl_gifindex = clean->dl_gifindex;
  /* Else If TE Link GifIndex is given, clean resources for all interfaces for this TE Link*/
  else if (NSM_CHECK_CTYPE (clean->cindex, NSM_GMPLS_QOS_CLEAN_CTYPE_TL_GIFINDEX))
    tl_gifindex = clean->tel_gifindex;
  /* Else Clean resources for all interfaces*/

  /* Cleanup. */
  nsm_gmpls_qos_serv_clean_for (nm, clean->protocol_id, tl_gifindex, dl_gifindex);

  return 0;
}

int
nsm_read_gmpls_qos_client_reserve (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_gmpls_qos *qos;
  struct nsm_msg_gmpls_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;
  struct list *preempt_list;
  qos_status status;
  int nbytes;

  /* Preset status to failure */
  preempt_list = NULL;
  status = QOS_OK;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = nse->nsc;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map_gmpls (qos, resource);
 
  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: RESERVE for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS release message. */
      nsm_gmpls_qos_dump (ns->zg, qos, header->type);
    }

  /* Reserve and send result */
  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_gmpls_qos));

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (status == QOS_OK)
    {
      preempt_list = list_new ();
      status = nsm_qos_serv_reserve (nm, resource, preempt_list); 
    }

  if (status == QOS_OK)
    {
      qos_prot_nsm_message_map_gmpls (resource, &msg);
    }
  else
    {
      msg.resource_id = resource->resource_id;
      msg.protocol_id = resource->protocol_id;
      pal_mem_cpy (&msg.owner, &resource->owner, sizeof (struct mpls_owner));
      qos_common_resource_delete (resource);
    }

  /* Status */
  NSM_SET_CTYPE (msg.cindex, NSM_QOS_CTYPE_STATUS);
  msg.status = status;

  nbytes = nsm_encode_gmpls_qos (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    {
      if (preempt_list)
        list_delete (preempt_list);
      return nbytes;
    }
  
  nsm_server_send_message (nse, header->vr_id, header->vrf_id,
                           header->type, header->message_id, nbytes);

  if (LISTCOUNT (preempt_list))
    nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
  else if (preempt_list)
    list_delete (preempt_list);

  return 0;
}

int
nsm_read_gmpls_qos_client_probe (struct nsm_msg_header *header,
                                void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_gmpls_qos *qos;
  struct nsm_msg_gmpls_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;
  struct list *preempt_list;
  qos_status status;
  int nbytes;

  /* Preset status to failure */
  preempt_list = NULL;
  status = QOS_OK;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = nse->nsc;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map_gmpls (qos, resource);

  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: PROBE for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS probe message. */
      nsm_gmpls_qos_dump (ns->zg, qos, header->type);
    }

  /* Probe and send result */
  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos));

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  if (status == QOS_OK)
    status = nsm_qos_serv_probe (nm, &resource); 
  if (status == QOS_OK)
    {
      qos_prot_nsm_message_map_gmpls (resource, &msg);
    }
  else
    {
      msg.resource_id = resource->resource_id;
      msg.protocol_id = resource->protocol_id;
      msg.id = resource->id;
      pal_mem_cpy (&msg.owner, &resource->owner, sizeof (struct mpls_owner));
    }

  /* Status */
  NSM_SET_CTYPE (msg.cindex, NSM_QOS_CTYPE_STATUS);
  msg.status = status;

  nbytes = nsm_encode_gmpls_qos (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    {
      qos_common_resource_delete (resource);
      return nbytes;
    }
  
  nsm_server_send_message (nse, header->vr_id, header->vrf_id,
                           header->type, header->message_id, nbytes);

  /* Delete the dummy resource object */
  qos_common_resource_delete (resource);

  return 0;
}

int
nsm_read_gmpls_qos_client_modify (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_gmpls_qos *qos;
  struct nsm_msg_gmpls_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;
  struct list *preempt_list;
  qos_status status;
  int nbytes;

  /* Preset status to failure */
  preempt_list = NULL;
  status = QOS_ERR;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = NULL;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map_gmpls (qos, resource);

  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: MODIFY for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS modify message. */
      nsm_gmpls_qos_dump (ns->zg, qos, header->type);
    }

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos));

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  preempt_list = list_new ();
  status = nsm_qos_serv_modify (nm, resource, preempt_list);
  if (status == QOS_OK)
    {
      qos_prot_nsm_message_map_gmpls (resource, &msg);
    }
  else
    {
      msg.resource_id = resource->resource_id;
      msg.protocol_id = resource->protocol_id;
      msg.id = resource->id;
      pal_mem_cpy (&msg.owner, &resource->owner, sizeof (struct mpls_owner));
    }

  /* Status */
  NSM_SET_CTYPE (msg.cindex, NSM_QOS_CTYPE_STATUS);
  msg.status = status;
  
  nbytes = nsm_encode_gmpls_qos (&nse->send.pnt, &nse->send.size, &msg);
  if (nbytes < 0)
    {
      list_delete (preempt_list);
      qos_common_resource_delete (resource);
      return nbytes;
    }
  
  nsm_server_send_message (nse, header->vr_id, header->vrf_id,
                           header->type, header->message_id, nbytes);

    /* Preempt all resources in this list */
  if (LISTCOUNT (preempt_list))
    nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
  else
    list_delete (preempt_list);

  /* Delete the dummy resource object. */
  qos_common_resource_delete (resource);

  return 0;
}

int
nsm_read_gmpls_qos_client_release (struct nsm_msg_header *header,
                                   void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_gmpls_qos_release *release;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;
  struct route_node *rn;
  struct qos_interface *qos_if = NULL;
  struct gmpls_if *gmif = NULL;
  struct datalink *dl = NULL;
  struct telink *tl = NULL;
  struct avl_node *ln;
  u_int32_t ifindex = 0;

  nse = arg;
  ns = nse->ns;
  release = message;
  nsc = NULL;

  /* Dump QOS release message. */
  if (IS_NSM_DEBUG_RECV)
    nsm_gmpls_qos_release_dump (ns->zg, release);

  zlog_info (NSM_ZG, "QOS Module: Release for resource with id %d for "
             "protocol %s",
             release->resource_id, modname_strl (release->protocol_id));

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /*
   * Release resource.
   */

  /* Get interface */
  gmif = gmpls_if_get (nzg, header->vr_id);

  if (!gmif)
    return 0;

  if (NSM_CHECK_CTYPE (release->cindex, NSM_GMPLS_QOS_RELEASE_CTYPE_DL_GIFINDEX))
    {
      dl = data_link_lookup_by_index (gmif, release->dl_gifindex);
      if (dl)
        {
          if (dl->ifp == NULL)
            return 0;

          ifindex = dl->ifp->ifindex;
          qos_if = nsm_qos_serv_if_get (nm, ifindex);
          if (! qos_if)
            return 0;
        } 
    }
  else if (NSM_CHECK_CTYPE (release->cindex, 
                            NSM_GMPLS_QOS_RELEASE_CTYPE_TL_GIFINDEX))
    {
      tl = te_link_lookup_by_index (gmif, release->tel_gifindex);
      if (tl)
        {
          AVL_TREE_LOOP (&tl->dltree, dl, ln)
            {
              ifindex = dl->ifp->ifindex;
              qos_if = nsm_qos_serv_if_get (nm, ifindex);
              if (! qos_if)
                continue;
              rn = qos_resource_node_lookup_if (release->resource_id, qos_if);
              if (rn)
                break;
            }
        }
    }   
  else
    return 0;

  if (! qos_if)
    return 0;

  /* Brute force search for resource based on resource id. */
  rn = qos_resource_node_lookup_if (release->resource_id, qos_if);
  if (rn)
    {
      resource = rn->info;
      if (resource->protocol_id == release->protocol_id)
        {
          nsm_qos_serv_bw_release_process (qos_if, 
                                           resource->t_spec.
                                           committed_bucket.rate,
                                           resource->hold_priority,
                                           resource->ct_num, resource);

          /* Send available bandwidth update to clients */
          if (if_is_up (qos_if->ifp))
            nsm_server_if_priority_bw_update (qos_if->ifp);
          
          /* Remove route node. */
          rn->info = NULL;
          route_unlock_node (rn);
          qos_common_resource_delete (resource);
        }
    }

  return 0;
}


int
nsm_read_gmpls_qos_client_release_slow (struct nsm_msg_header *header,
                                        void *arg, void *message)
{
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_server *ns;
  struct nsm_msg_gmpls_qos *qos;
  struct nsm_msg_gmpls_qos msg;
  struct nsm_server_client *nsc;
  struct qos_resource *resource;

  nse = arg;
  ns = nse->ns;
  qos = message;
  nsc = NULL;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  if (nm == NULL)
    return 0;

  /* Create and initialize resource */
  resource = qos_common_resource_new ();
  qos_common_resource_init (resource);

  qos_nsm_prot_message_map_gmpls (qos, resource);

  if (IS_NSM_DEBUG_RECV)
    {
      zlog_info (NSM_ZG, "QOS Module: SLOW RELEASE for protocol %s",
                 modname_strl (resource->protocol_id));
      /* Dump QOS modify message. */
      nsm_gmpls_qos_dump (ns->zg, qos, NSM_MSG_QOS_CLIENT_RELEASE);
    }

  pal_mem_set (&msg, 0, sizeof(struct nsm_msg_qos));

  /* Free resource. */
  nsm_qos_serv_release_slow (nm, resource);

  /* Free up temp resource. */
  qos_common_resource_delete (resource);

  return 0;
}
#endif /*HAVE_GMPLS */
#endif /* HAVE_TE */

