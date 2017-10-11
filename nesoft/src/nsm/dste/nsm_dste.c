/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_DSTE

#include "ptree.h"
#include "table.h"
#include "nsmd.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "mpls.h"
#include "mpls_common.h"
#include "dste.h"
#include "nsm_message.h"
#include "log.h"
#include "hash.h"
#include "buffer.h"
#include "if.h"

#include "nsm_dste.h"
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#include "nsm_server.h"
#include "nsm_debug.h"
#include "nsm_qos_serv.h"
#include "nsm_mpls_qos.h"
#include "nsm_interface.h"
#ifdef HAVE_GMPLS
#include "nsm_gmpls_if.h"
#endif /*HAVE_GMPLS*/

/* Get the Class Type number from Class Type name. */
int
nsm_dste_get_class_type_num (struct nsm_master *nm,
                             char *name)
{
  u_char i;

  for (i = 0; i < MAX_CLASS_TYPE; i++)
    if (pal_strcmp(NSM_MPLS->ct_name[i], name) == 0)
      return i;

  return CT_NUM_INVALID;
}

int
nsm_dste_class_type_add (struct nsm_master *nm, int ct_num, char *name)
{
  int i;
 
  /* If there is no change, return. */
  if (pal_strcmp(NSM_MPLS->ct_name[ct_num], name) == 0)
    return NSM_SUCCESS;

  pal_mem_cpy(NSM_MPLS->ct_name[ct_num], name, pal_strlen(name)+1);
  
  for (i = 0; i < MAX_TE_CLASS; i++)
    if (NSM_MPLS->te_class[i].ct_num == ct_num)
      nsm_dste_te_class_update (nm, i);
  
  return NSM_SUCCESS;
}

int
nsm_dste_class_type_delete (struct nsm_master *nm, int ct_num)
{
  struct interface *ifp;
  struct route_node *rn;
  int i;
  int ret = NSM_SUCCESS;

  /* Remove Bandwidth Constraint associated with it. */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      {
       ret = nsm_qos_serv_update_bw_constraint (ifp, 0, ct_num);
       if (ret < 0 && IS_NSM_DEBUG_EVENT)
         zlog_warn (nzg, "DSTE class type delete %d: "
                   "failed for interface %s:", ct_num, ifp->name); 
      }

  /* Remove the Class Type. */
  NSM_MPLS->ct_name[ct_num][0] = '\0';

  for (i = 0; i < MAX_TE_CLASS; i++)
    if (NSM_MPLS->te_class[i].ct_num == ct_num)
      nsm_dste_te_class_delete (nm, i);

  return NSM_SUCCESS;
}


int
nsm_dste_te_class_add (struct nsm_master *nm,
                       u_char tecl_index, int ct_num, u_char priority)
{
  struct qos_interface *qos_if;
  struct nsm_if *nsm_if;
  struct interface *ifp;
  struct route_node *rn;

  /* If there is no change, return success. */
  if (NSM_MPLS->te_class[tecl_index].ct_num == ct_num
      && NSM_MPLS->te_class[tecl_index].priority == priority)
    return NSM_SUCCESS;
 
  NSM_MPLS->te_class[tecl_index].ct_num = ct_num;
  NSM_MPLS->te_class[tecl_index].priority = priority;
  
  /* Update the TE Class. */
  nsm_dste_te_class_update (nm, tecl_index);
  
  /* Update the available bandwidth of this TE Class on each interface. */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if ((nsm_if = ifp->info))
        if ((qos_if = nsm_if->qos_if))
          {  
            /* recompute available bandwidth */
            nsm_qos_serv_te_class_avail_bw_update (qos_if, tecl_index);

#ifdef HAVE_GMPLS
            nsm_data_link_property_copy (ifp, PAL_TRUE);
#endif /* HAVE_GMPLS */

            /* send available bandwidth update to clients */
            if (if_is_up (ifp))
              nsm_server_if_priority_bw_update (ifp);
          }

  return NSM_SUCCESS;
}

int
nsm_dste_te_class_delete (struct nsm_master *nm, u_char tecl_index)
{
  struct interface *ifp;
  struct qos_interface *qos_if;
  struct nsm_if *nsm_if;
  struct route_node *rn;
  struct list *preempt_list = list_new ();

  /* Release the bandwidth constraint associated with this TE Class. */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      if ((nsm_if = ifp->info))
        if ((qos_if = nsm_if->qos_if))
          nsm_qos_serv_preempt_by_te_class (preempt_list, qos_if, tecl_index);

  if (LISTCOUNT (preempt_list))
    nsm_qos_serv_send_preempt (nm, &preempt_list, NSM_TRUE, NSM_TRUE);
  else
    list_free (preempt_list);
    
  /* Delete the TE Class. */
  NSM_MPLS->te_class[tecl_index].ct_num = CT_NUM_INVALID;
  NSM_MPLS->te_class[tecl_index].priority = TE_PRIORITY_INVALID;
  
  nsm_dste_te_class_update (nm, tecl_index);
  
  return NSM_SUCCESS;

}

int
nsm_dste_bc_mode_update (char *name, struct interface *ifp)
{
  bc_mode_t bc_mode;

  bc_mode = pal_strcmp (name, "MAM") == 0 ? BC_MODE_MAM : 
    (pal_strcmp (name, "RSDL") == 0 ? BC_MODE_RSDL : BC_MODE_MAR);

  /* Send update to protocols. */
  if (bc_mode != ifp->bc_mode)
    {
      ifp->bc_mode = bc_mode;
      nsm_dste_send_bc_mode_update (ifp);
    }

  return NSM_SUCCESS;
}

#endif /* HAVE_DSTE */
