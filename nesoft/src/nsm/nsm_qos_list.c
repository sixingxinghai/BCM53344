/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "log.h"
#include "cli.h"
#include "show.h"
#include "filter.h"
#include "avl_tree.h"

#include "nsmd.h"

#ifdef HAVE_QOS
#ifdef HAVE_VLAN
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#endif /* HAVE_VLAN */
#include "nsm_qos.h"
#include "nsm_qos_filter.h"
#include "nsm_qos_list.h"
#ifdef HAVE_HA
#include "nsm_bridge_cal.h"
#include "nsm_flowctrl.h"
#include "nsm_qos_cal_api.h"
#endif /*HAVE_HA*/

extern struct nsm_qos_global   qosg;

/*----------------------*/
/* ACL list             */
/*----------------------*/
int
nsm_acl_list_master_init (struct nsm_master *nm)
{
  struct nsm_qos_acl_master *master;

  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_acl_master));

  if (! master)
    return -1;

  nm->acl = master;

  return 0;
}

void
nsm_acl_list_master_deinit (struct nsm_master *nm)
{
  if (nm->acl)
    {
      nsm_acl_list_clean (nm->acl);
      XFREE (MTYPE_TMP, nm->acl);
      nm->acl = NULL;
    }
}

/*----------------------*/
/* CMAP list            */
/*----------------------*/
/* Allocate new cmap-list structure. */
struct nsm_cmap_list *
nsm_cmap_list_new ()
{
  struct nsm_cmap_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct nsm_cmap_list));
  return new;
}

/* Free allocated cmap-list. */
void
nsm_cmap_list_free (struct nsm_cmap_list *cmapl)
{
  if (cmapl->acl_name_set)
    pal_mem_set (cmapl->acl_name,'\0',INTERFACE_NAMSIZ);

  cmapl->acl_name_set = 0;

#ifdef HAVE_HA
  nsm_cmap_data_cal_delete(cmapl);
#endif /*HAVE_HA*/

  XFREE (MTYPE_TMP, cmapl);
}

/* Delete cmap-list from cmap_master and free it. */
void
nsm_cmap_list_delete (struct nsm_cmap_list *cmapl)
{
  struct nsm_qos_cmap_master *master;

  master = cmapl->master;

  if (cmapl->next)
    cmapl->next->prev = cmapl->prev;
  else
    master->tail = cmapl->prev;

  if (cmapl->prev)
    cmapl->prev->next = cmapl->next;
  else
    master->head = cmapl->next;

  nsm_cmap_list_free (cmapl);
}

/* Clean cmap-list. */
void
nsm_cmap_list_clean (struct nsm_qos_cmap_master *master)
{
  struct nsm_cmap_list *cmapl, *cmapl_next;

  for (cmapl = master->head; cmapl; cmapl = cmapl_next)
    {
      cmapl_next = cmapl->next;
      nsm_cmap_list_delete (cmapl);
    }

  master->head = master->tail = NULL;
}

/* Insert new cmap-list to list of advert-list. */
struct nsm_cmap_list *
nsm_cmap_list_insert (struct nsm_qos_cmap_master *master,
                      const char *cmap_name,
                      const char *acl_name)
{
  struct nsm_cmap_list *cmapl;

  if (master == NULL || ! cmap_name)
    return NULL;

  /* Allocate new cmap-list and copy given name. */
  cmapl = nsm_cmap_list_new ();
  cmapl->master = master;

  if (master->tail)
    {
      master->tail->next = cmapl;
      cmapl->prev = master->tail;
    }
  else
    master->head = cmapl;

  master->tail = cmapl;
  cmapl->next = NULL;

  /* update cmap attributes */
  cmapl->name = XSTRDUP (MTYPE_TMP, cmap_name);

  if (acl_name)
  {
    pal_strncpy(cmapl->acl_name,acl_name,INTERFACE_NAMSIZ);
    cmapl->acl_name_set = 1;
  }
  else
    cmapl->acl_name_set = 0;

  return cmapl;
}

/* Lookup cmap-list from list of cmap-list by name. */
struct nsm_cmap_list *
nsm_cmap_list_lookup (struct nsm_qos_cmap_master *master,
                      const char *cmap_name)
{
  struct nsm_cmap_list *cmapl;

  if (master == NULL || ! cmap_name)
    return NULL;

  for (cmapl = master->head; cmapl; cmapl = cmapl->next)
    if (cmapl->name && pal_strcmp (cmapl->name, cmap_name) == 0)
      return cmapl;
  return NULL;
}


/* Lookup cmap-list from list of cmap-list by acl-name. */
struct nsm_cmap_list *
nsm_cmap_list_lookup_by_acl (struct nsm_qos_cmap_master *master,
                             const char *acl_name)
{
  struct nsm_cmap_list *cmapl;

  if (master == NULL || ! acl_name)
    return NULL;

  for (cmapl = master->head; cmapl; cmapl = cmapl->next)
    if (pal_strcmp (cmapl->acl_name, acl_name) == 0)
      return cmapl;
  return NULL;
}


/* Get cmap-list from list of cmap-list.
   If there isn't matched cmap-list create new one and return it. */
struct nsm_cmap_list *
nsm_cmap_list_get (struct nsm_qos_cmap_master *master,
                   const char *cmap_name)
{
  struct nsm_cmap_list *cmapl;

  cmapl = nsm_cmap_list_lookup (master, cmap_name);
  if (cmapl == NULL)
    cmapl = nsm_cmap_list_insert (master, cmap_name, NULL);
  else
    return cmapl;
#ifdef HAVE_HA
  nsm_cmap_data_cal_create(cmapl);
#endif /*HAVE_HA*/
  return cmapl;
}


int
nsm_cmap_list_master_init (struct nsm_master *nm)
{
  struct nsm_qos_cmap_master *master;


  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_cmap_master));
  if (! master)
    return -1;

  nm->cmap = master;
  master->head = master->tail = NULL;
  return 0;
}


void
nsm_cmap_list_master_deinit (struct nsm_master *nm)
{
  if (nm->cmap)
    {
      nsm_cmap_list_clean (nm->cmap);
      XFREE (MTYPE_TMP, nm->cmap);
      nm->cmap = NULL;
    }
}


/*----------------------*/
/* PMAP list            */
/*----------------------*/
/* Allocate new pmap-list structure. */
struct nsm_pmap_list *
nsm_pmap_list_new ()
{
  struct nsm_pmap_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct nsm_pmap_list));
  return new;
}


/* Free allocated pmap-list. */
void
nsm_pmap_list_free (struct nsm_pmap_list *pmapl)
{
  int i;

  if (pmapl->acl_name)
    XFREE (MTYPE_TMP, pmapl->acl_name);
  pmapl->acl_name = NULL;

  for (i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++)
    if (pmapl->cl_name[i][0] != 0x00)
      pal_mem_set(pmapl->cl_name[i],0x00,INTERFACE_NAMSIZ);

#ifdef HAVE_HA
  nsm_pmap_data_cal_delete (pmapl);
#endif /*HAVE_HA*/
}


/* Delete pmap-list from pmap_master and free it. */
void
nsm_pmap_list_delete (struct nsm_pmap_list *pmapl)
{
  struct nsm_qos_pmap_master *master;

  master = pmapl->master;

  if (pmapl->next)
    pmapl->next->prev = pmapl->prev;
  else
    master->tail = pmapl->prev;

  if (pmapl->prev)
    pmapl->prev->next = pmapl->next;
  else
    master->head = pmapl->next;

  nsm_pmap_list_free (pmapl);
}


/* Clean pmap-list. */
void
nsm_pmap_list_clean (struct nsm_qos_pmap_master *master)
{
  struct nsm_pmap_list *pmapl, *pmapl_next;

  for (pmapl = master->head; pmapl; pmapl = pmapl_next)
    {
      pmapl_next = pmapl->next;
      nsm_pmap_list_delete (pmapl);
    }

  master->head = master->tail = NULL;
}


/* Insert new pmap-list to list of advert-list. */
struct nsm_pmap_list *
nsm_pmap_list_insert (struct nsm_qos_pmap_master *master,
                      const char *pmap_name,
                      const char *acl_name)
{
  struct nsm_pmap_list *pmapl;

  if (master == NULL || ! pmap_name)
    return NULL;

  /* Allocate new pmap-list and copy given name. */
  pmapl = nsm_pmap_list_new ();
  pmapl->master = master;

  if (master->tail)
    {
      master->tail->next = pmapl;
      pmapl->prev = master->tail;
    }
  else
    master->head = pmapl;

  master->tail = pmapl;
  pmapl->next = NULL;

  /* update pmap attributes */
  pmapl->name = XSTRDUP (MTYPE_TMP, pmap_name);

  if (acl_name)
    pmapl->acl_name = XSTRDUP (MTYPE_TMP, acl_name);
  else
    pmapl->acl_name = NULL;

  return pmapl;
}


/* Lookup pmap-list from list of pmap-list by name. */
struct nsm_pmap_list *
nsm_pmap_list_lookup (struct nsm_qos_pmap_master *master,
                      const char *pmap_name)
{
  struct nsm_pmap_list *pmapl;

  if (master == NULL || ! pmap_name)
    return NULL;

  for (pmapl = master->head; pmapl; pmapl = pmapl->next)
    if (pmapl->name && pal_strcmp (pmapl->name, pmap_name) == 0)
      return pmapl;
  return NULL;
}


/* Lookup pmap-list from list of pmap-list by acl-name. */
struct nsm_pmap_list *
nsm_pmap_list_lookup_by_acl (struct nsm_qos_pmap_master *master,
                             const char *acl_name)
{
  struct nsm_pmap_list *pmapl;

  if (master == NULL || ! acl_name)
    return NULL;

  for (pmapl = master->head; pmapl; pmapl = pmapl->next)
    if (pmapl->acl_name && pal_strcmp (pmapl->acl_name, acl_name) == 0)
      return pmapl;
  return NULL;
}


/* Get pmap-list from list of pmap-list.
   If there isn't matched pmap-list create new one and return it. */
struct nsm_pmap_list *
nsm_pmap_list_get (struct nsm_qos_pmap_master *master,
                   const char *pmap_name)
{
  struct nsm_pmap_list *pmapl;

  pmapl = nsm_pmap_list_lookup (master, pmap_name);
  if (pmapl == NULL)
    pmapl = nsm_pmap_list_insert (master, pmap_name, NULL);
  else
    return pmapl;
#ifdef HAVE_HA
  nsm_pmap_data_cal_create (pmapl);
#endif /*HAVE_HA*/
  return pmapl;
}


int
nsm_pmap_list_master_init (struct nsm_master *nm)
{
  struct nsm_qos_pmap_master *master;

  /* Allocate policy map master. */
  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_pmap_master));
  if (! master)
    return -1;

  nm->pmap = master;
  master->head = master->tail = NULL;

  return 0;
}

int
nsm_pmap_list_master_deinit (struct nsm_master *nm)
{
  if (nm->pmap)
    {
      nsm_pmap_list_clean (nm->pmap);
      XFREE (MTYPE_TMP, nm->pmap);
      nm->pmap = NULL;
    }

  return 0;
}


/*------------------------*/
/* DSCP mutation map list */
/*------------------------*/
/* Allocate new dscpml-list structure. */
struct nsm_dscp_mut_list *
nsm_dscp_mut_list_new ()
{
  struct nsm_dscp_mut_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct nsm_dscp_mut_list));
  return new;
}


/* Free allocated dscpml-list. */
void
nsm_dscp_mut_list_free (struct nsm_dscp_mut_list *dscpml)
{
  if (dscpml->acl_name)
    XFREE (MTYPE_TMP, dscpml->acl_name);
  dscpml->acl_name = NULL;
#ifdef HAVE_HA
  nsm_dscp_mut_data_cal_delete(dscpml);
#endif /*HAVE_HA*/

  XFREE (MTYPE_TMP, dscpml);
}


/* Delete dscpml-list from dscpml_master and free it. */
void
nsm_dscp_mut_list_delete (struct nsm_dscp_mut_list *dscpml)
{
  struct nsm_qos_dscpml_master *master;

  master = dscpml->master;

  if (dscpml->next)
    dscpml->next->prev = dscpml->prev;
  else
    master->tail = dscpml->prev;

  if (dscpml->prev)
    dscpml->prev->next = dscpml->next;
  else
    master->head = dscpml->next;

  nsm_dscp_mut_list_free (dscpml);
}


/* Clean dscpml-list. */
void
nsm_dscp_mut_list_clean (struct nsm_qos_dscpml_master *master)
{
  struct nsm_dscp_mut_list *dscpml, *dscpml_next;

  for (dscpml = master->head; dscpml; dscpml = dscpml_next)
    {
      dscpml_next = dscpml->next;
      nsm_dscp_mut_list_delete (dscpml);
    }

  master->head = master->tail = NULL;
}


/* Insert new dscpml-list to list of advert-list. */
struct nsm_dscp_mut_list *
nsm_dscp_mut_list_insert (struct nsm_qos_dscpml_master *master,
                          const char *dscpml_name,
                          const char *acl_name)
{
  struct nsm_dscp_mut_list *dscpml;
  u_char i;

  if (master == NULL || ! dscpml_name)
    return NULL;

  /* Allocate new dscpml-list and copy given name. */
  dscpml = nsm_dscp_mut_list_new ();
  dscpml->master = master;

  if (master->tail)
    {
      master->tail->next = dscpml;
      dscpml->prev = master->tail;
    }
  else
    master->head = dscpml;

  master->tail = dscpml;
  dscpml->next = NULL;

  /* update dscpml attributes */
  dscpml->name = XSTRDUP (MTYPE_TMP, dscpml_name);

  if (acl_name)
    dscpml->acl_name = XSTRDUP (MTYPE_TMP, acl_name);
  else
    dscpml->acl_name = NULL;
  
  for (i = 0; i < DSCP_TBL_SIZE; i++)
    dscpml->d.dscp[i] = i;
  
  return dscpml;
}


/* Lookup dscpml-list from list of dscpml-list by name. */
struct nsm_dscp_mut_list *
nsm_dscp_mut_list_lookup (struct nsm_qos_dscpml_master *master,
                          const char *dscpml_name)
{
  struct nsm_dscp_mut_list *dscpml;

  if (master == NULL || ! dscpml_name)
    return NULL;

  for (dscpml = master->head; dscpml; dscpml = dscpml->next)
    if (dscpml->name && pal_strcmp (dscpml->name, dscpml_name) == 0)
      return dscpml;
  return NULL;
}


/* Lookup dscpml-list from list of dscpml-list by acl-name. */
struct nsm_dscp_mut_list *
nsm_dscp_mut_list_lookup_by_acl (struct nsm_qos_dscpml_master *master,
                                 const char *acl_name)
{
  struct nsm_dscp_mut_list *dscpml;

  if (master == NULL || ! acl_name)
    return NULL;

  for (dscpml = master->head; dscpml; dscpml = dscpml->next)
    if (dscpml->acl_name && pal_strcmp (dscpml->acl_name, acl_name) == 0)
      return dscpml;
  return NULL;
}


/* Get dscpml-list from list of dscpml-list.
   If there isn't matched dscpml-list create new one and return it. */
struct nsm_dscp_mut_list *
nsm_dscp_mut_list_get (struct nsm_qos_dscpml_master *master,
                       const char *dscpml_name)
{
  struct nsm_dscp_mut_list *dscpml;
  int i;

  dscpml = nsm_dscp_mut_list_lookup (master, dscpml_name);
  if (dscpml == NULL)
  {
    dscpml = nsm_dscp_mut_list_insert (master, dscpml_name, NULL);
    /* In new dscp-mutation list case, just initialize it */
    /* From 0 to 63 */
    for (i=0 ; i < DSCP_TBL_SIZE ; i++)
      dscpml->d.dscp[i] = i;
  }
  else
    return dscpml;
#ifdef HAVE_HA
  nsm_dscp_mut_data_cal_create(dscpml);
#endif /*HAVE_HA*/
  return dscpml;
}

static void
nsm_mls_qos_copy_weights (struct nsm_qos_global *qg,
                          int weights [])
{
   if (qg->q0[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [0] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [0] = qg->q0[NSM_QOS_QUEUE_WEIGHT_INDEX];

   if (qg->q1[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [1] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [1] = qg->q1[NSM_QOS_QUEUE_WEIGHT_INDEX];

   if (qg->q2[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [2] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [2] = qg->q2[NSM_QOS_QUEUE_WEIGHT_INDEX];

   if (qg->q3[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [3] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [3] = qg->q3[NSM_QOS_QUEUE_WEIGHT_INDEX];

   if (qg->q4[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [4] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [4] = qg->q4[NSM_QOS_QUEUE_WEIGHT_INDEX];

   if (qg->q5[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [5] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [5] = qg->q5[NSM_QOS_QUEUE_WEIGHT_INDEX];

   if (qg->q6[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [6] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [6] = qg->q6[NSM_QOS_QUEUE_WEIGHT_INDEX];

   if (qg->q7[NSM_QOS_QUEUE_WEIGHT_INDEX] == NSM_QOS_QUEUE_WEIGHT_INVALID)
     weights [7] = HAL_QOS_QUEUE_WEIGHT_INVLAID;
   else
     weights [7] = qg->q7[NSM_QOS_QUEUE_WEIGHT_INDEX];

   return;
}

void
nsm_qos_queue_weight_init (struct nsm_qos_global *qg)
{
  int weights [HAL_QOS_MAX_QUEUE_SIZE];

  qg->q0[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;
  qg->q1[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;
  qg->q2[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;
  qg->q3[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;
  qg->q4[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;
  qg->q5[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;
  qg->q6[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;
  qg->q7[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

  nsm_mls_qos_copy_weights (qg, weights);


#ifdef HAVE_HAL
  hal_mls_qos_set_queue_weight (weights);
#endif /* HAVE_HAL */

  return;
}

int
nsm_mls_qos_set_queue_weight (struct nsm_qos_global *qg,
                              u_int8_t queue_id,
                              u_int8_t weight)
{
   int weights [HAL_QOS_MAX_QUEUE_SIZE];
   s_int32_t ret;

   if (queue_id == NSM_QOS_QUEUE0)
    qg->q0[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   if (queue_id == NSM_QOS_QUEUE1)
    qg->q1[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   if (queue_id == NSM_QOS_QUEUE2)
    qg->q2[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   if (queue_id == NSM_QOS_QUEUE3)
    qg->q3[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   if (queue_id == NSM_QOS_QUEUE4)
    qg->q4[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   if (queue_id == NSM_QOS_QUEUE5)
    qg->q5[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   if (queue_id == NSM_QOS_QUEUE6)
    qg->q6[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   if (queue_id == NSM_QOS_QUEUE7)
    qg->q7[NSM_QOS_QUEUE_WEIGHT_INDEX] = weight;

   nsm_mls_qos_copy_weights (qg, weights);

#ifdef HAVE_HAL
   ret = hal_mls_qos_set_queue_weight (weights);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_get_queue_weight (struct nsm_qos_global *qg,
                              int *weights)
{
   nsm_mls_qos_copy_weights (qg, weights);

   return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_unset_queue_weight (struct nsm_qos_global *qg,
                                u_int8_t queue_id)
{
   int weights [HAL_QOS_MAX_QUEUE_SIZE];
   s_int32_t ret;

   if (queue_id == NSM_QOS_QUEUE0)
    qg->q0[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   if (queue_id == NSM_QOS_QUEUE1)
    qg->q1[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   if (queue_id == NSM_QOS_QUEUE2)
    qg->q2[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   if (queue_id == NSM_QOS_QUEUE3)
    qg->q3[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   if (queue_id == NSM_QOS_QUEUE4)
    qg->q4[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   if (queue_id == NSM_QOS_QUEUE5)
    qg->q5[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   if (queue_id == NSM_QOS_QUEUE6)
    qg->q6[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   if (queue_id == NSM_QOS_QUEUE7)
    qg->q7[NSM_QOS_QUEUE_WEIGHT_INDEX] = NSM_QOS_QUEUE_WEIGHT_INVALID;

   nsm_mls_qos_copy_weights (qg, weights);

#ifdef HAVE_HAL
   ret = hal_mls_qos_set_queue_weight (weights);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}

void
nsm_qos_dscp_to_queue_init (struct nsm_qos_global *qg)
{
  int i;

  for (i = 0; i < DSCP_TBL_SIZE; i++)
    qg->dscp_queue_map_tbl [i] = 0;

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

  return;
}


int
nsm_mls_qos_set_dscp_to_queue (struct nsm_qos_global *qg,
                               u_int8_t dscp,
                               u_int8_t queue_id)
{
   s_int32_t ret;

   qg->dscp_queue_map_tbl [dscp] = queue_id;

#ifdef HAVE_HAL
   ret = hal_qos_dscp_to_queue (dscp, queue_id);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

   SET_FLAG (qg->dscp_queue_map_tbl [dscp], NSM_QOS_DSCP_QUEUE_CONF);

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_unset_dscp_to_queue (struct nsm_qos_global *qg,
                                 u_int8_t dscp)
{
   s_int32_t ret;

   UNSET_FLAG (qg->dscp_queue_map_tbl [dscp], NSM_QOS_DSCP_QUEUE_CONF);

   qg->dscp_queue_map_tbl [dscp] = NSM_QOS_QUEUE0;

#ifdef HAVE_HAL
   ret = hal_qos_dscp_to_queue (dscp, NSM_QOS_QUEUE0);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */


#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}

void
nsm_qos_cos_to_queue_init (struct nsm_qos_global *qg)
{
  int i;

  for (i = 0; i < COS_TBL_SIZE; i++)
    qg->cos_queue_map_tbl [i] = 0;


#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

  return;
}


int
nsm_mls_qos_set_cos_to_queue (struct nsm_qos_global *qg,
                              u_int8_t cos,
                              u_int8_t queue_id)
{
   s_int32_t ret;

   qg->cos_queue_map_tbl [cos] = queue_id;

#ifdef HAVE_HAL
   ret = hal_qos_cos_to_queue (cos, queue_id);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE)HAL */

   SET_FLAG (qg->cos_queue_map_tbl [cos], NSM_QOS_COS_QUEUE_CONF);

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_unset_cos_to_queue (struct nsm_qos_global *qg,
                                u_int8_t cos)
{
   s_int32_t ret;

   UNSET_FLAG (qg->cos_queue_map_tbl [cos], NSM_QOS_COS_QUEUE_CONF);

   qg->cos_queue_map_tbl [cos] = NSM_QOS_QUEUE0;

#ifdef HAVE_HAL
   ret = hal_qos_cos_to_queue (cos, NSM_QOS_QUEUE0);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE)HAL */

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}


char *
nsm_qos_ttype_cri_to_str (enum nsm_qos_traffic_match ttype_criteria)
{

  switch (ttype_criteria)
    {
      case NSM_QOS_CRITERIA_TTYPE_AND_PRI:
        return "traffic-type-and-queue";
        break;
      case NSM_QOS_CRITERIA_TTYPE_OR_PRI:
        return "traffic-type-or-queue";
        break;
      default:
        break;
    }

  return "Invalid";

}

char *
nsm_qos_ftype_to_str (enum nsm_qos_frame_type ftype)
{
  switch (ftype)
    {
      case NSM_QOS_FTYPE_DSA_TO_CPU_BPDU:
        return "bpdu-to-cpu";
        break;
      case NSM_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT:
        return "ucast-mgmt-to-cpu";
        break;
      case NSM_QOS_FTYPE_DSA_FROM_CPU:
        return "from-cpu";
        break;
      case NSM_QOS_FTYPE_PORT_ETYPE_MATCH:
        return "port-etype-match";
        break;
      default:
        break;
    }

  return "Invalid";
}

void
nsm_qos_set_frame_type_priority_override_init (struct nsm_qos_global *qg)
{
  int i;

  for (i = 0; i < NSM_QOS_FTYPE_MAX; i++)
    qg->ftype_pri_tbl [i] = 0;

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

  return;
}


int
nsm_qos_set_frame_type_priority_override (struct nsm_qos_global *qg,
                                          enum nsm_qos_frame_type ftype,
                                          u_int8_t queue_id)
{
   s_int32_t ret;

#ifdef HAVE_HAL
   enum hal_qos_frame_type hal_ftype;
#endif /* HAVE_HAL */
  
   qg->ftype_pri_tbl [ftype] = queue_id;

#ifdef HAVE_HAL
   switch (ftype)
    {
      case NSM_QOS_FTYPE_DSA_TO_CPU_BPDU:
        hal_ftype = HAL_QOS_FTYPE_DSA_TO_CPU_BPDU;
        break;
      case NSM_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT:
        hal_ftype = HAL_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT;
        break;
      case NSM_QOS_FTYPE_DSA_FROM_CPU:
        hal_ftype = HAL_QOS_FTYPE_DSA_FROM_CPU;
        break;
      case NSM_QOS_FTYPE_PORT_ETYPE_MATCH:
        hal_ftype = HAL_QOS_FTYPE_PORT_ETYPE_MATCH;
        break;
      default:
        hal_ftype = HAL_QOS_FTYPE_MAX;
        break;
    }


   ret = hal_qos_set_frame_type_priority_override (hal_ftype, queue_id);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

   SET_FLAG (qg->ftype_pri_tbl [ftype], NSM_QOS_FTYPE_PRI_OVERRIDE_CONF);

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}

int
nsm_qos_unset_frame_type_priority_override (struct nsm_qos_global *qg,
                                            enum nsm_qos_frame_type ftype)
{
   s_int32_t ret;
#ifdef HAVE_HAL
   enum hal_qos_frame_type hal_ftype;
#endif /* HAVE_HAL */

   UNSET_FLAG (qg->ftype_pri_tbl [ftype], NSM_QOS_FTYPE_PRI_OVERRIDE_CONF);

   qg->ftype_pri_tbl [ftype] = 0;

#ifdef HAVE_HAL
   switch (ftype)
    {
      case NSM_QOS_FTYPE_DSA_TO_CPU_BPDU:
        hal_ftype = HAL_QOS_FTYPE_DSA_TO_CPU_BPDU;
        break;
      case NSM_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT:
        hal_ftype = HAL_QOS_FTYPE_DSA_TO_CPU_UCAST_MGMT;
        break;
      case NSM_QOS_FTYPE_DSA_FROM_CPU:
        hal_ftype = HAL_QOS_FTYPE_DSA_FROM_CPU;
        break;
      case NSM_QOS_FTYPE_PORT_ETYPE_MATCH:
        hal_ftype = HAL_QOS_FTYPE_PORT_ETYPE_MATCH;
        break;
      default:
        hal_ftype = HAL_QOS_FTYPE_MAX;
        break;
    }

   ret = hal_qos_unset_frame_type_priority_override (hal_ftype);

   if (ret != HAL_SUCCESS)
     return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_qosg_data_cal_modify (qg);
#endif /*HAVE_HA*/

   return NSM_QOS_SUCCESS;
}

#ifdef HAVE_VLAN

/* Compare function to insert nsm_qos_vlan_pri in AVL tree */

int
nsm_mls_qos_vlan_pri_cmp (void *data1, void *data2)
{
  struct nsm_qos_vlan_pri *pri1= (struct nsm_qos_vlan_pri *)data1;
  struct nsm_qos_vlan_pri *pri2= (struct nsm_qos_vlan_pri *)data2;

  if (data1 == NULL
      || data2 == NULL)
    return -1;

  if (pri1->brno > pri2->brno)
    return 1;

  if (pri2->brno > pri1->brno)
    return -1;

  if (pri1->vid > pri2->vid)
    return 1;

  if (pri2->vid > pri1->vid)
    return -1;

  return 0;
}

void
nsm_mld_qos_vlan_pri_config_add (char *bridge_name,
                                 u_int16_t vid,
                                 u_int8_t priority)
{
  s_int32_t ret;
  u_int32_t brno;
  struct avl_node *node;
  struct nsm_qos_vlan_pri key;
  struct nsm_qos_vlan_pri *pri;

  pal_mem_set (&key, 0, sizeof (key));

  pal_sscanf (bridge_name, "%u", &brno);

  key.brno = brno;
  key.vid = vid;

  node = avl_search (qosg.vlan_pri_tree, (void *) &key);

  if (node != NULL)
    {
      pri = (struct nsm_qos_vlan_pri *) node->info;

      if (pri == NULL)
        return;

      pri->prio = priority;
    }
  else
    {
      pri = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_vlan_pri));

      if (pri == NULL)
        return;

      pri->brno = brno;

      pri->vid = vid;

      pri->prio = priority;

      ret = avl_insert (qosg.vlan_pri_tree, pri);

      if (ret < 0)
        {
          XFREE (MTYPE_TMP, pri);
          return;
        }
    }

  return;
}

int
nsm_mls_qos_vlan_pri_free (struct nsm_qos_vlan_pri *pri)
{
  XFREE (MTYPE_TMP, pri);
  return 0;
}

struct nsm_qos_vlan_pri *
nsm_mld_qos_vlan_pri_config_search (char *bridge_name,
                                    u_int16_t vid)
{
  u_int32_t brno;
  struct avl_node *node;
  struct nsm_qos_vlan_pri key;

  pal_mem_set (&key, 0, sizeof (key));

  pal_sscanf (bridge_name, "%u", &brno);

  key.brno = brno;
  key.vid = vid;

  node = avl_search (qosg.vlan_pri_tree, (void *) &key);

  return (node == NULL ? NULL: (node->info));

}

void
nsm_mls_qos_vlan_priority_config_restore (struct nsm_master *nm,
                                          char *bridge_name,
                                          u_int16_t vid)
{
   struct nsm_qos_vlan_pri *pri;

   pri = nsm_mld_qos_vlan_pri_config_search (bridge_name, vid);

   if (pri == NULL)
     return;

  nsm_mls_qos_vlan_priority_set (nm, bridge_name, vid, pri->prio);

  avl_remove (qosg.vlan_pri_tree, (void *) pri);

  XFREE (MTYPE_TMP, pri);

  return;
}

void
nsm_mls_qos_vlan_pri_config_deinit ()
{
  avl_tree_free (&qosg.vlan_pri_tree, (void_t (*) (void_t *)) 
                                      nsm_mls_qos_vlan_pri_free);
}

int
nsm_mls_qos_vlan_priority_set (struct nsm_master *nm,
                               char *bridge_name,
                               u_int16_t vid,
                               u_int8_t priority)
{
  s_int32_t ret;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);


  if (bridge_name == NULL)
    bridge = nsm_get_default_bridge (master);
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (bridge == NULL)
    return NSM_QOS_ERR_BRIDGE_NOT_FOUND;

  vlan = nsm_vlan_find (bridge, vid);

  if (vlan == NULL)
    {
      nsm_mld_qos_vlan_pri_config_add (bridge->name, vid,
                                      (priority & NSM_VLAN_PRIORITY_MASK));
      return NSM_QOS_ERR_VLAN_NOT_FOUND;
    }


  vlan->priority = (priority & NSM_VLAN_PRIORITY_MASK);

#ifdef HAVE_HAL
  ret = hal_qos_set_vlan_priority (vid, priority);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#endif /* HAVE)HAL */

  SET_FLAG (vlan->priority, NSM_VLAN_PRIORITY_CONF);

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_vlan(vlan);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_vlan_priority_get (struct nsm_master *nm,
                               char *bridge_name,
                               u_int16_t vid,
                               u_int8_t *priority)
{
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  *priority = NSM_VLAN_PRIORITY_NONE;

  if (bridge_name == NULL)
    bridge = nsm_get_default_bridge (master);
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (bridge == NULL)
    return NSM_QOS_ERR_BRIDGE_NOT_FOUND;

  vlan = nsm_vlan_find (bridge, vid);

  if (vlan == NULL)
    return NSM_QOS_ERR_VLAN_NOT_FOUND;

  if (CHECK_FLAG (vlan->priority, NSM_VLAN_PRIORITY_CONF))
    *priority = vlan->priority & NSM_VLAN_PRIORITY_MASK;

  return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_vlan_priority_unset (struct nsm_master *nm,
                                 char *bridge_name,
                                 u_int16_t vid)
{
  s_int32_t ret;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);


  if (bridge_name == NULL)
    bridge = nsm_get_default_bridge (master);
  else
    bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (bridge == NULL)
    return NSM_QOS_ERR_BRIDGE_NOT_FOUND;

  vlan = nsm_vlan_find (bridge, vid);

  if (vlan == NULL)
    return NSM_QOS_ERR_VLAN_NOT_FOUND;

  UNSET_FLAG (vlan->priority, NSM_VLAN_PRIORITY_CONF);

  vlan->priority = NSM_VLAN_PRIORITY_NONE;

#ifdef HAVE_HAL
  ret = hal_qos_unset_vlan_priority (vid);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_bridge_cal_modify_nsm_vlan(vlan);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;
}

int
nsm_qos_set_port_vlan_priority_override (struct nsm_qos_global *qosg,
                                       struct interface *ifp,
                                       enum nsm_qos_vlan_pri_override port_mode)
{
#ifdef HAVE_HAL
  enum hal_qos_vlan_pri_override hal_port_mode;
#endif /* HAVE_HAL */
  struct nsm_qif_list *qifl;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

  qifl->port_mode = port_mode;

#ifdef HAVE_HAL
  hal_port_mode = HAL_QOS_VLAN_PRI_OVERRIDE_NONE;

  switch (port_mode)
    {
      case NSM_QOS_VLAN_PRI_OVERRIDE_NONE:
        hal_port_mode = HAL_QOS_VLAN_PRI_OVERRIDE_NONE;
        break;
      case NSM_QOS_VLAN_PRI_OVERRIDE_COS:
        hal_port_mode = HAL_QOS_VLAN_PRI_OVERRIDE_COS;
        break;
      case NSM_QOS_VLAN_PRI_OVERRIDE_QUEUE:
        hal_port_mode = HAL_QOS_VLAN_PRI_OVERRIDE_QUEUE;
        break;
      case NSM_QOS_VLAN_PRI_OVERRIDE_BOTH:
        hal_port_mode = HAL_QOS_VLAN_PRI_OVERRIDE_BOTH;
        break;
      default:
        break;
    }

  ret = hal_qos_set_port_vlan_priority_override (ifp->ifindex, 
                                                 hal_port_mode);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/
#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/
#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/
#endif /* HAVE_HAL */

  return NSM_QOS_SUCCESS;
}

#endif /* HAVE_VLAN */

int
nsm_qos_set_port_da_priority_override (struct nsm_qos_global *qosg,
                                       struct interface *ifp,
                                       enum nsm_qos_da_pri_override port_mode)
{
#ifdef HAVE_HAL
  enum hal_qos_da_pri_override hal_port_mode;
#endif /* HAVE_HAL */
  struct nsm_qif_list *qifl;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

  qifl->da_port_mode = port_mode;

#ifdef HAVE_HAL
  hal_port_mode = HAL_QOS_DA_PRI_OVERRIDE_NONE;

  switch (port_mode)
    {
      case NSM_QOS_DA_PRI_OVERRIDE_NONE:
        hal_port_mode = HAL_QOS_DA_PRI_OVERRIDE_NONE;
        break;
      case NSM_QOS_DA_PRI_OVERRIDE_COS:
        hal_port_mode = HAL_QOS_DA_PRI_OVERRIDE_COS;
        break;
      case NSM_QOS_DA_PRI_OVERRIDE_QUEUE:
        hal_port_mode = HAL_QOS_DA_PRI_OVERRIDE_QUEUE;
        break;
      case NSM_QOS_DA_PRI_OVERRIDE_BOTH:
        hal_port_mode = HAL_QOS_DA_PRI_OVERRIDE_BOTH;
        break;
      default:
        break;
    }

  ret = hal_qos_set_port_da_priority_override (ifp->ifindex, 
                                               hal_port_mode);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;

#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/
#endif /* HAVE_HAL */

  return NSM_QOS_SUCCESS;
}
int
nsm_qos_set_trust_state (struct nsm_qos_global *qosg,
                         struct interface *ifp,
                         u_int32_t trust_state)
{
#ifdef HAVE_HAL
  enum hal_qos_trust_state hal_trust_state;
#endif /* HAVE_HAL */
  struct nsm_qif_list *qifl;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

  qifl->trust_type = trust_state;

#ifdef HAVE_HAL
  hal_trust_state = HAL_QOS_TRUST_NONE;

  switch (trust_state)
    {
      case NSM_QOS_TRUST_NONE:
        hal_trust_state = HAL_QOS_TRUST_NONE;
        break;
      case NSM_QOS_TRUST_COS:
      case NSM_QOS_TRUST_COS_PT_DSCP:
        hal_trust_state = HAL_QOS_TRUST_8021P;
        break;
      case NSM_QOS_TRUST_DSCP:
      case NSM_QOS_TRUST_DSCP_PT_COS:
        hal_trust_state = HAL_QOS_TRUST_DSCP;
        break;
      case NSM_QOS_TRUST_DSCP_COS:
        hal_trust_state = HAL_QOS_TRUST_BOTH;
        break;
      default:
        break;
    }

  ret = hal_qos_set_trust_state (ifp->ifindex, hal_trust_state);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#ifdef HAVE_HA
  nsm_qif_data_cal_modify (qifl);
#endif /*HAVE_HA*/
#endif /* HAVE_HAL */

  return NSM_QOS_SUCCESS;
}

int
nsm_qos_set_force_trust_cos (struct nsm_qos_global *qosg,
                             struct interface *ifp,
                             u_int8_t force_trust_cos)
{
  struct nsm_qif_list *qifl;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

  qifl->force_trust_cos = force_trust_cos;

#ifdef HAVE_HAL
  ret = hal_qos_set_force_trust_cos (ifp->ifindex, force_trust_cos);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_qif_data_cal_modify (qifl);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;
}

int
nsm_qos_set_def_cos_for_port (struct nsm_qos_global *qosg,
                              struct interface *ifp,
                              u_int8_t cos)
{
  struct nsm_qif_list *qifl;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

  qifl->def_cos = cos;
  
#ifdef HAVE_HAL
  ret = hal_qos_set_default_cos_for_port (ifp->ifindex, cos);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

#ifdef HAVE_HA
  nsm_qif_data_cal_modify (qifl);
#endif /*HAVE_HA*/


  return NSM_QOS_SUCCESS;
}

int
nsm_qos_set_strict_queue (struct nsm_qos_global *qosg,
                          struct interface *ifp,
                          u_int8_t qid)
{
  struct nsm_qif_list *qifl;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

#ifdef HAVE_HAL
  if (qid == NSM_QOS_STRICT_QUEUE_ALL
      || qid == NSM_QOS_STRICT_QUEUE_NONE
      || qid == NSM_QOS_STRICT_QUEUE0)
    ret = hal_qos_set_strict_queue (ifp->ifindex, qid);
  else
    ret = hal_qos_set_strict_queue (ifp->ifindex,
                                    qifl->strict_queue | (1 << qid));

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

  if (qid == NSM_QOS_STRICT_QUEUE_ALL
      || qid == NSM_QOS_STRICT_QUEUE_NONE
      || qid == NSM_QOS_STRICT_QUEUE0)
    qifl->strict_queue = qid;
  else
    SET_FLAG (qifl->strict_queue, (1 << qid));

#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;
}

int
nsm_qos_unset_strict_queue (struct nsm_qos_global *qosg,
                            struct interface *ifp,
                            u_int8_t qid)
{
  struct nsm_qif_list *qifl;
  u_int8_t strict_queue;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

  strict_queue = qifl->strict_queue;

  UNSET_FLAG (strict_queue, (1 << qid));

#ifdef HAVE_HAL
  ret = hal_qos_set_strict_queue (ifp->ifindex, strict_queue);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

  UNSET_FLAG (qifl->strict_queue, (1 << qid));

#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;

}

int
nsm_qos_set_egress_rate_shaping (struct nsm_qos_global *qosg,
                                 struct interface *ifp,
                                 u_int32_t rate,
                                 enum nsm_qos_rate_unit rate_unit)
{
#ifdef HAVE_HAL
  enum hal_qos_rate_unit hal_rate_unit;
#endif /* HAVE_HAL */
  struct nsm_qif_list *qifl;
  s_int32_t ret;

  qifl = nsm_qif_list_get (qosg->qif, ifp->name);

  if (qifl == NULL)
    return NSM_QOS_ERR_MEM;

#ifdef HAVE_HAL
  hal_rate_unit = HAL_QOS_RATE_KBPS;

  switch (rate_unit)
    {
      case NSM_QOS_RATE_KBPS:
        hal_rate_unit = HAL_QOS_RATE_KBPS;
        break;
      case NSM_QOS_RATE_FPS:
        hal_rate_unit = HAL_QOS_RATE_FPS;
        break;
      default:
        break;
    }

  if (rate == NSM_QOS_NO_TRAFFIC_SHAPE)
    ret = hal_qos_set_egress_rate_shape (ifp->ifindex, HAL_QOS_NO_TRAFFIC_SHAPE,
                                         hal_rate_unit);
  else
    ret = hal_qos_set_egress_rate_shape (ifp->ifindex, rate, hal_rate_unit);

  if (ret != HAL_SUCCESS)
    return NSM_QOS_ERR_HAL;
#endif /* HAVE_HAL */

  qifl->egress_rate_unit = rate_unit;
  qifl->egress_shape_rate = rate;

#ifdef HAVE_HA
  nsm_qif_data_cal_modify(qifl);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;
}

int
nsm_dscp_mut_list_master_init (struct nsm_qos_global *qg)
{
  struct nsm_qos_dscpml_master *master;

  /* Allocate DSCP mutation list master. */
  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_dscpml_master));
  if (! master)
    return  -1;

  qg->dscpml = master;
  master->head = master->tail = NULL;

  return 0;
}

int
nsm_dscp_mut_list_master_deinit (struct nsm_qos_global *qg)
{
  if (qg->dscpml)
    {
      nsm_dscp_mut_list_clean (qg->dscpml);
      XFREE (MTYPE_TMP, qg->dscpml);
      qg->dscpml = NULL;
    }

  return 0;
}


int
nsm_qos_if_dscp_mutation_map_attached (struct nsm_master *nm, char *dscp_mut_map_name)
{
  struct nsm_qif_list *qifl;
  
  for (qifl = qosg.qif->head; qifl; qifl = qifl->next)
    {
      if ((pal_strlen(qifl->dscp_mut_name) > 0) && 
          pal_strcmp (qifl->dscp_mut_name, dscp_mut_map_name) == 0)
        return NSM_TRUE;
    }

  return NSM_FALSE;
}


/*------------------------*/
/* DSCP-to-COS map list   */
/*------------------------*/
/* Allocate new dcosl-list structure. */
struct nsm_dscp_cos_list *
nsm_dscp_cos_list_new ()
{
  struct nsm_dscp_cos_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct nsm_dscp_cos_list));
  return new;
}


/* Free allocated dcosl-list. */
void
nsm_dscp_cos_list_free (struct nsm_dscp_cos_list *dcosl)
{
  if (dcosl->acl_name)
    XFREE (MTYPE_TMP, dcosl->acl_name);
  dcosl->acl_name = NULL;

#ifdef HAVE_HA
  nsm_dscp_cos_data_cal_delete (dcosl);
#endif /*HAVE_HA*/

  XFREE (MTYPE_TMP, dcosl);
}


/* Delete dcosl-list from dcosl_master and free it. */
void
nsm_dscp_cos_list_delete (struct nsm_dscp_cos_list *dcosl)
{
  struct nsm_qos_dcosl_master *master;

  master = dcosl->master;

  if (dcosl->next)
    dcosl->next->prev = dcosl->prev;
  else
    master->tail = dcosl->prev;

  if (dcosl->prev)
    dcosl->prev->next = dcosl->next;
  else
    master->head = dcosl->next;

  nsm_dscp_cos_list_free (dcosl);
}


/* Clean dcosl-list. */
void
nsm_dscp_cos_list_clean (struct nsm_qos_dcosl_master *master)
{
  struct nsm_dscp_cos_list *dcosl, *dcosl_next;

  for (dcosl = master->head; dcosl; dcosl = dcosl_next)
    {
      dcosl_next = dcosl->next;
      nsm_dscp_cos_list_delete (dcosl);
    }

  master->head = master->tail = NULL;
}


/* Insert new dcosl-list to list of advert-list. */
struct nsm_dscp_cos_list *
nsm_dscp_cos_list_insert (struct nsm_qos_dcosl_master *master,
                          const char *dcosl_name,
                          const char *acl_name)
{
  struct nsm_dscp_cos_list *dcosl;

  if (master == NULL || ! dcosl_name)
    return NULL;

  /* Allocate new dcosl-list and copy given name. */
  dcosl = nsm_dscp_cos_list_new ();
  dcosl->master = master;

  if (master->tail)
    {
      master->tail->next = dcosl;
      dcosl->prev = master->tail;
    }
  else
    master->head = dcosl;

  master->tail = dcosl;
  dcosl->next = NULL;

  /* update dcosl attributes */
  dcosl->name = XSTRDUP (MTYPE_TMP, dcosl_name);

  if (acl_name)
    dcosl->acl_name = XSTRDUP (MTYPE_TMP, acl_name);
  else
    dcosl->acl_name = NULL;

  return dcosl;
}


/* Lookup dcosl-list from list of dcosl-list by name. */
struct nsm_dscp_cos_list *
nsm_dscp_cos_list_lookup (struct nsm_qos_dcosl_master *master,
                          const char *dcosl_name)
{
  struct nsm_dscp_cos_list *dcosl;

  if (master == NULL || ! dcosl_name)
    return NULL;

  for (dcosl = master->head; dcosl; dcosl = dcosl->next)
    if (dcosl->name && pal_strcmp (dcosl->name, dcosl_name) == 0)
      return dcosl;
  return NULL;
}


/* Lookup dcosl-list from list of dcosl-list by acl-name. */
struct nsm_dscp_cos_list *
nsm_dscp_cos_list_lookup_by_acl (struct nsm_qos_dcosl_master *master,
                                 const char *acl_name)
{
  struct nsm_dscp_cos_list *dcosl;

  if (master == NULL || ! acl_name)
    return NULL;

  for (dcosl = master->head; dcosl; dcosl = dcosl->next)
    if (dcosl->acl_name && pal_strcmp (dcosl->acl_name, acl_name) == 0)
      return dcosl;
  return NULL;
}


/* Get dcosl-list from list of dcosl-list.
   If there isn't matched dcosl-list create new one and return it. */
struct nsm_dscp_cos_list *
nsm_dscp_cos_list_get (struct nsm_qos_dcosl_master *master,
                       const char *dcosl_name)
{
  struct nsm_dscp_cos_list *dcosl;
  int i;

  dcosl = nsm_dscp_cos_list_lookup (master, dcosl_name);
  if (dcosl == NULL)
    {
      dcosl = nsm_dscp_cos_list_insert (master, dcosl_name, NULL);
      /* In new dscp-to-cos list case, just initialize it */
      /* From 0 to 7 */
      for (i=0 ; i < DSCP_TBL_SIZE ; i++)
        dcosl->d.dscp[i] = i/8;
    }
  else 
      return dcosl;

#ifdef HAVE_HA
  nsm_dscp_cos_data_cal_create (dcosl);
#endif /*HAVE_HA*/

  return dcosl;
}

int
nsm_dscp_cos_list_master_init (struct nsm_qos_global *qg)
{
  struct nsm_qos_dcosl_master *master;

  /* Allocate DSCP -> COS master. */
  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_dcosl_master));
  if (! master)
    return -1;

  qg->dcosl = master;
  master->head = master->tail = NULL;

  return 0;
}

int
nsm_dscp_cos_list_master_deinit (struct nsm_qos_global *qg)
{
  if (qg->dcosl)
    {
      nsm_dscp_cos_list_clean (qg->dcosl);
      XFREE (MTYPE_TMP, qg->dcosl);
      qg->dcosl = NULL;
    }

  return 0;
}


/*------------------------*/
/* Aggregate-policer list */
/*------------------------*/
/* Allocate new agpl-list structure. */
struct nsm_agp_list *
nsm_agp_list_new ()
{
  struct nsm_agp_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct nsm_agp_list));
  return new;
}


/* Free allocated agpl-list. */
void
nsm_agp_list_free (struct nsm_agp_list *agpl)
{
  if (agpl->acl_name)
    XFREE (MTYPE_TMP, agpl->acl_name);
  agpl->acl_name = NULL;
#ifdef HAVE_HA
  nsm_agp_data_cal_delete(agpl);
#endif

  XFREE (MTYPE_TMP, agpl);
}


/* Delete agpl-list from agpl_master and free it. */
void
nsm_agp_list_delete (struct nsm_agp_list *agpl)
{
  struct nsm_qos_agp_master *master;

  master = agpl->master;

  if (agpl->next)
    agpl->next->prev = agpl->prev;
  else
    master->tail = agpl->prev;

  if (agpl->prev)
    agpl->prev->next = agpl->next;
  else
    master->head = agpl->next;

  nsm_agp_list_free (agpl);
}


/* Clean agpl-list. */
void
nsm_agp_list_clean (struct nsm_qos_agp_master *master)
{
  struct nsm_agp_list *agpl, *agpl_next;

  for (agpl = master->head; agpl; agpl = agpl_next)
    {
      agpl_next = agpl->next;
      nsm_agp_list_delete (agpl);
    }

  master->head = master->tail = NULL;
}


/* Insert new agpl-list to list of advert-list. */
struct nsm_agp_list *
nsm_agp_list_insert (struct nsm_qos_agp_master *master,
                     const char *agpl_name,
                     const char *acl_name)
{
  struct nsm_agp_list *agpl;

  if (master == NULL || ! agpl_name)
    return NULL;

  /* Allocate new agpl-list and copy given name. */
  agpl = nsm_agp_list_new ();
  agpl->master = master;

  if (master->tail)
    {
      master->tail->next = agpl;
      agpl->prev = master->tail;
    }
  else
    master->head = agpl;

  master->tail = agpl;
  agpl->next = NULL;

  /* update agpl attributes */
  agpl->name = XSTRDUP (MTYPE_TMP, agpl_name);

  if (acl_name)
    agpl->acl_name = XSTRDUP (MTYPE_TMP, acl_name);
  else
    agpl->acl_name = NULL;

  return agpl;
}


/* Lookup agpl-list from list of agpl-list by name. */
struct nsm_agp_list *
nsm_agp_list_lookup (struct nsm_qos_agp_master *master,
                     const char *agpl_name)
{
  struct nsm_agp_list *agpl;

  if (master == NULL || ! agpl_name)
    return NULL;

  for (agpl = master->head; agpl; agpl = agpl->next)
    if (agpl->name && pal_strcmp (agpl->name, agpl_name) == 0)
      return agpl;
  return NULL;
}


/* Lookup agpl-list from list of agpl-list by acl-name. */
struct nsm_agp_list *
nsm_agp_list_lookup_by_acl (struct nsm_qos_agp_master *master,
                            const char *acl_name)
{
  struct nsm_agp_list *agpl;

  if (master == NULL || ! acl_name)
    return NULL;

  for (agpl = master->head; agpl; agpl = agpl->next)
    if (agpl->acl_name && pal_strcmp (agpl->acl_name, acl_name) == 0)
      return agpl;
  return NULL;
}


/*------------------------*/
/* QoS interface list     */
/*------------------------*/
/* Allocate new qif-list structure. */
struct nsm_qif_list *
nsm_qif_list_new ()
{
  struct nsm_qif_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct nsm_qif_list));
  return new;
}


/* Free allocated qifl-list. */
void
nsm_qif_list_free (struct nsm_qif_list *qifl)
{
  if (qifl->acl_name)
    XFREE (MTYPE_TMP, qifl->acl_name);
  qifl->acl_name = NULL;
#ifdef HAVE_HA
  nsm_qif_data_cal_delete(qifl);
#endif

  XFREE (MTYPE_TMP, qifl);
}


/* Delete qifl-list from qifl_master and free it. */
void
nsm_qif_list_delete (struct nsm_qif_list *qifl)
{
  struct nsm_qos_if_master *master;

  master = qifl->master;

  if (qifl->next)
    qifl->next->prev = qifl->prev;
  else
    master->tail = qifl->prev;

  if (qifl->prev)
    qifl->prev->next = qifl->next;
  else
    master->head = qifl->next;

  nsm_qif_list_free (qifl);
}


/* Clean qifl-list. */
void
nsm_qif_list_clean (struct nsm_qos_if_master *master)
{
  struct nsm_qif_list *qifl, *qifl_next;

  for (qifl = master->head; qifl; qifl = qifl_next)
    {
      qifl_next = qifl->next;
      nsm_qif_list_delete (qifl);
    }

  master->head = master->tail = NULL;
}

/* Insert new qifl-list to list of advert-list. */
struct nsm_qif_list *
nsm_qif_list_insert (struct nsm_qos_if_master *master,
                     const char *qifl_name,
                     const char *acl_name)
{
  struct nsm_qif_list *qifl;

  if (master == NULL || ! qifl_name)
    return NULL;

  /* Allocate new qifl-list and copy given name. */
  qifl = nsm_qif_list_new ();
  qifl->master = master;

  if (master->tail)
    {
      master->tail->next = qifl;
      qifl->prev = master->tail;
    }
  else
    master->head = qifl;

  master->tail = qifl;
  qifl->next = NULL;

  /* update qifl attributes */
  qifl->name =  XSTRDUP (MTYPE_TMP, qifl_name);

  if (acl_name)
    qifl->acl_name = XSTRDUP (MTYPE_TMP, acl_name);
  else
    qifl->acl_name = NULL;

  return qifl;
}


/* Lookup qifl-list from list of qifl-list by name. */
struct nsm_qif_list *
nsm_qif_list_lookup (struct nsm_qos_if_master *master,
                     const char *qifl_name)
{
  struct nsm_qif_list *qifl;

  if (master == NULL || ! qifl_name)
    return NULL;

  for (qifl = master->head; qifl; qifl = qifl->next)
    if (qifl->name && pal_strcmp (qifl->name, qifl_name) == 0)
      return qifl;
  return NULL;
}

/* Lookup qifl-list from list of qifl-list by acl-name. */
struct nsm_qif_list *
nsm_qif_list_lookup_by_acl (struct nsm_qos_if_master *master,
                            const char *acl_name)
{
  struct nsm_qif_list *qifl;

  if (master == NULL || ! acl_name)
    return NULL;

  for (qifl = master->head; qifl; qifl = qifl->next)
    if (qifl->acl_name && pal_strcmp (qifl->acl_name, acl_name) == 0)
      return qifl;
  return NULL;
}


/* Get qifl-list from list of qifl-list.
   If there isn't matched qifl-list create new one and return it. */
struct nsm_qif_list *
nsm_qif_list_get (struct nsm_qos_if_master *master,
                  char *qifl_name)
{
  struct nsm_qif_list *qifl;
  int i;

  qifl = nsm_qif_list_lookup (master, qifl_name);
  if (qifl == NULL)
    qifl = nsm_qif_list_insert (master, qifl_name, NULL);
  else
    return qifl;

  if (qifl)
    {
      for (i = 0; i < 8; i++)
        {
          qifl->cosq_map[i] = i;
        }

      for (i = 0; i <= NSM_QOS_DSCP_MAX; i++)
        {
          qifl->wred_dscp_threshold[i] = 0;
        }
    }
#ifdef HAVE_HA
  nsm_qif_data_cal_create(qifl);
#endif 
  return qifl;
}

int
nsm_qif_list_master_init (struct nsm_qos_global *qg)
{
  struct nsm_qos_if_master *master;


  /* Allocate QoS interface master. */
  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_if_master));
  if (! master)
    return -1;

  qg->qif = master;
  master->head = master->tail = NULL;

  return 0;
}

int
nsm_qif_list_master_deinit (struct nsm_qos_global *qg)
{
  if (qg->qif)
    {
      nsm_qif_list_clean (qg->qif);
      XFREE (MTYPE_TMP, qg->qif);
      qg->qif = NULL;
    }

  return 0;
}

/* Get agpl-list from list of agpl-list.
   If there isn't matched agpl-list create new one and return it. */
struct nsm_agp_list *
nsm_agp_list_get (struct nsm_qos_agp_master *master,
                  const char *agpl_name)
{
  struct nsm_agp_list *agpl;

  agpl = nsm_agp_list_lookup (master, agpl_name);
  if (agpl == NULL)
    agpl = nsm_agp_list_insert (master, agpl_name, NULL);
  else
    return agpl;
#ifdef HAVE_HA
  nsm_agp_data_cal_create(agpl);
#endif
  return agpl;
}

int
nsm_agp_list_master_init (struct nsm_qos_global *qg)
{
  struct nsm_qos_agp_master *master;

  /* Allocate agp list master. */
  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_qos_agp_master));
  if (! master)
    return -1;

  qg->agp = master;
  master->head = master->tail = NULL;

  return 0;
}

int
nsm_agp_list_master_deinit (struct nsm_qos_global *qg)
{
  if (qg->agp)
    {
      nsm_agp_list_clean (qg->agp);
      XFREE (MTYPE_TMP, qg->agp);
      qg->agp = NULL;
    }

  return 0;
}


/* Lookup MAC acl from all cmap-lists and pmap-lists */
int
nsm_qos_check_macl_from_cmap_and_pmap (struct nsm_qos_cmap_master *cmaster,
                                       struct nsm_qos_pmap_master *pmaster,
                                       const char *acl_name, int check_attached)
{
  struct nsm_cmap_list *cmapl;
  struct nsm_pmap_list *pmapl;
  int i;

  for (cmapl = cmaster->head; cmapl; cmapl = cmapl->next)
    {
      if (cmapl->acl_name_set && pal_strcmp (cmapl->acl_name, acl_name) == 0)
        {
          if (! check_attached)
            return NSM_TRUE;

          for (pmapl = pmaster->head; pmapl; pmapl = pmapl->next)
            {
              for ( i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++ )
                {
                  if ((pmapl->cl_name[i][0] != 0x00)
                      && pal_strcmp (pmapl->cl_name[i], cmapl->name) == 0)
                    {
                      /* Check pmap is already attached to an interface */
                      if (pmapl->attached > 0)
                        {
                          return NSM_TRUE;
                        }
                    }
                }
            }
        }
    }
  return NSM_FALSE;
}


int
nsm_pmap_class_agg_policer_exists (struct nsm_master *nm,
                                   char *name, int check_attached)
{
  struct nsm_cmap_list *cmapl;
  struct nsm_pmap_list *pmapl;
  int i;
  
  for (pmapl = nm->pmap->head; pmapl; pmapl = pmapl->next)
    {
      for ( i=0; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++)
        {
          if ((pmapl->cl_name[i][0] == 0x00))
            continue;
          
          cmapl = nsm_cmap_list_lookup (nm->cmap, pmapl->cl_name[i]);
          if (! cmapl)
            continue;

          if (cmapl->agg_policer_name_set &&
              pal_strcmp (cmapl->agg_policer_name, name) == 0 &&
              (! check_attached || pmapl->attached > 0))
            return NSM_TRUE;
        }
    }
  
  return NSM_FALSE;
}


/* Lookup class-name from all pmap-lists */
struct nsm_pmap_list *
nsm_qos_check_class_from_pmap (struct nsm_qos_pmap_master *master,
                               const char *cl_name, int check_attached)
{
  struct nsm_pmap_list *pmapl;
  int i;

  if (master == NULL || ! cl_name)
    return NULL;

  for (pmapl = master->head; pmapl; pmapl = pmapl->next)
    {
      for ( i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++ )
        {
          if ((pmapl->cl_name[i][0] == 0x00))
            continue;
          if (pal_strcmp (pmapl->cl_name[i], cl_name) == 0)
            {
              if (! check_attached || pmapl->attached > 0)
                return pmapl;
            }
        }
    }
  return NULL;
}


/* Insert acl name into class-map list */
struct nsm_cmap_list *
nsm_qos_insert_acl_name_into_cmap (struct nsm_cmap_list *cmapl,
                                   const char *acl_name)
{
  if (cmapl->acl_name_set && pal_strcmp (cmapl->acl_name, acl_name) == 0)
    return cmapl;
  
  if (! cmapl->acl_name_set)
    {
      pal_strncpy (cmapl->acl_name, acl_name,INTERFACE_NAMSIZ);
      cmapl->match_type = NSM_QOS_MATCH_TYPE_ACL;
      cmapl->acl_name_set = 1; 
      return cmapl;
    }
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif
  return NULL;
}


/* Delete acl name from class-map list */
struct nsm_cmap_list *
nsm_qos_delete_acl_name_from_cmap (struct nsm_cmap_list *cmapl,
                                   const char *acl_name)
{
  if (cmapl->acl_name_set && pal_strcmp (cmapl->acl_name, acl_name) == 0)
    {
      pal_mem_set (cmapl->acl_name,'\0',INTERFACE_NAMSIZ);
      cmapl->acl_name_set = 0;
      cmapl->match_type = NSM_QOS_MATCH_TYPE_NONE;
      return cmapl;
    }

#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif
  return NULL;
}


/* Set DSCP into class-map list */
void
nsm_qos_set_dscp_into_cmap (struct nsm_cmap_list *cmapl,
                            int num,
                            u_int8_t *dscp)
{
  cmapl->d.num = num;
  pal_mem_cpy (&cmapl->d.dscp[0], dscp, num);
  cmapl->match_type = NSM_QOS_MATCH_TYPE_DSCP;

#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}


/* Delete DSCP from class-map list */
void
nsm_qos_delete_dscp_from_cmap (struct nsm_cmap_list *cmapl)
{
  cmapl->d.num = 0;
  pal_mem_set (&cmapl->d.dscp[0], 0, MAX_NUM_OF_DSCP_IN_CMAP);
  cmapl->match_type = NSM_QOS_MATCH_TYPE_NONE;

#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}


/* Set prec into class-map list */
void
nsm_qos_set_prec_into_cmap (struct nsm_cmap_list *cmapl,
                            int num,
                            u_int8_t *dscp)
{
  cmapl->i.num = num;
  pal_mem_cpy (&cmapl->i.prec[0], dscp, num);
  cmapl->match_type = NSM_QOS_MATCH_TYPE_IP_PREC;
#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}


/* Delete prec from class-map list */
void
nsm_qos_delete_prec_from_cmap (struct nsm_cmap_list *cmapl)
{
  cmapl->i.num = 0;
  pal_mem_set (&cmapl->i.prec[0], 0, MAX_NUM_OF_IP_PREC_IN_CMAP);
  cmapl->match_type = NSM_QOS_MATCH_TYPE_NONE;
#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}

/* Set EXP into class-map list */
void
nsm_qos_set_exp_into_cmap (struct nsm_cmap_list *cmapl, int num,
                           u_int8_t *exp)
{
  cmapl->e.num = num;
  pal_mem_cpy (&cmapl->e.exp[0], exp, num);
  cmapl->match_type = NSM_QOS_MATCH_TYPE_EXP;
#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}

/* Delete EXP from class-map list */
void
nsm_qos_delete_exp_from_cmap (struct nsm_cmap_list *cmapl)
{
  cmapl->e.num = 0;
  pal_mem_set (&cmapl->e.exp[0], 0, MAX_NUM_OF_EXP_IN_CMAP);
  cmapl->match_type = NSM_QOS_MATCH_TYPE_NONE;
#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}

void
nsm_qos_set_layer4_port_into_cmap (struct nsm_cmap_list *cmapl,
                                   u_int16_t l4_port_id, u_char port_type)
{
  cmapl->l4_port.port_type = port_type;
  cmapl->l4_port.port_id = l4_port_id;
  cmapl->match_type = NSM_QOS_MATCH_TYPE_L4_PORT;
#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}


void
nsm_qos_delete_layer4_port_from_cmap (struct nsm_cmap_list *cmapl,
                                      u_int16_t l4_port_id, u_char port_type)
{
  cmapl->l4_port.port_type = NSM_QOS_LAYER4_PORT_NONE;
  cmapl->l4_port.port_id = 0;
  cmapl->match_type = NSM_QOS_MATCH_TYPE_NONE;
#ifdef HAVE_HA
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}



/* Set vid into class-map list */
int
nsm_qos_set_vid_into_cmap (struct nsm_cmap_list *cmapl,
                           int num,
                           u_int16_t *vid)
{
  int sum;

  sum = cmapl->v.num + num;
  if (sum > MAX_NUM_OF_VLAN_FILTER)
    return -1;

  /* FFS */
  pal_mem_cpy (&cmapl->v.vlan[cmapl->v.num], vid, num*2);
  cmapl->v.num += num;

#ifdef HAVE_HA
  nsm_cmap_vlan_q_data_cal_modify (cmapl);
#endif /*HAVE_HA*/
  return 0;
}


/* Delete vid from class-map list */
void
nsm_qos_delete_vid_from_cmap (struct nsm_cmap_list *cmapl)
{
  cmapl->v.num = 0;
  pal_mem_set (&cmapl->v.vlan[0], 0, MAX_NUM_OF_VLAN_FILTER*2);
#ifdef HAVE_HA
  nsm_cmap_vlan_q_data_cal_modify(cmapl);
#endif
}


/* Lookup class-map name in pmap-list */
struct nsm_pmap_list *
nsm_qos_check_cl_name_in_pmapl (struct nsm_pmap_list *pmapl,
                                const char *cl_name)
{
  int i;


  for ( i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++ )
    {
      if ((pmapl->cl_name[i][0] != 0x00) 
          && (pal_strcmp (pmapl->cl_name[i], cl_name) == 0))
        return pmapl;
    }
  for ( i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++ )
    {
      if ((pmapl->cl_name[i][0] == 0x00)) 
        {
          pal_strncpy(pmapl->cl_name[i], cl_name,INTERFACE_NAMSIZ);
          #ifdef HAVE_HA
            nsm_pmap_data_cal_modify (pmapl);
          #endif /*HAVE_HA*/
          return pmapl;
        }
    }
  return NULL;
}


/* Delete cmap from pmap-list */
void
nsm_qos_delete_cmap_from_pmapl (struct nsm_pmap_list *pmapl,
                                const char *cl_name)
{
  int i;

  for ( i=0 ; i < MAX_NUM_OF_CLASS_IN_PMAPL ; i++ )
    {
      if ((pmapl->cl_name[i][0] != 0x00) 
          && (pal_strcmp (pmapl->cl_name[i], cl_name) == 0))
        {
          pal_mem_set(pmapl->cl_name[i],0x00,INTERFACE_NAMSIZ);
          #ifdef HAVE_HA
            nsm_pmap_data_cal_modify (pmapl);
          #endif /*HAVE_HA*/
          break;
        }
    }
}

/* Lookup class-map in all cmap lists */
struct nsm_cmap_list *
nsm_qos_check_cmap_in_all_cmapls (struct nsm_qos_cmap_master *master,
                                  const char *cl_name)
{
  struct nsm_cmap_list *cmapl;

  for (cmapl = master->head; cmapl; cmapl = cmapl->next)
    {
      if (cmapl->name && pal_strcmp (cmapl->name, cl_name) == 0)
        return cmapl;
    }
  return NULL;
}


/* Set trust state to a specific class-map */
void
nsm_qos_set_trust_into_cmap (struct nsm_cmap_list *cmapl,
                             int ind,
                             int trust)
{
  cmapl->t.cos_dscp_prec_ind = (u_int8_t)ind;
  cmapl->t.val = (u_int8_t)trust;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}

/* Set cos, ip-dscp, or ip-precedence vlaue to the class-map */
void
nsm_qos_set_val_into_cmap (struct nsm_cmap_list *cmapl,
                           int type,
                           int val)
{
  cmapl->s.type = (u_int8_t)type;
  cmapl->s.val = (u_int8_t)val;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}


/* Set rate, buffer size, and exceed-action to the class-map */
void
nsm_qos_set_exd_act_into_cmap (struct nsm_cmap_list *cmapl,
                               u_int32_t rate,
                               u_int32_t burst,
                               u_int32_t excess_burst,
                               enum nsm_exceed_action exd,
                               enum nsm_qos_flow_control_mode fc_mode)
{
  cmapl->p.avg = rate;
  cmapl->p.burst = burst;
  cmapl->p.exd_act = exd;
  cmapl->p.excess_burst = excess_burst;
  cmapl->p.fc_mode = fc_mode;

#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}


/* Clear rate, buffer size, and exceed-action to the class-map */
void
nsm_qos_clear_exd_act_into_cmap (struct nsm_cmap_list *cmapl)
{
  cmapl->p.avg = 0;
  cmapl->p.burst = 0;
  cmapl->p.excess_burst = 0;
  cmapl->p.fc_mode = NSM_QOS_FLOW_CONTROL_MODE_NONE;
  cmapl->p.exd_act = NSM_QOS_EXD_ACT_NONE;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif /*HAVE_HA*/
}

s_int32_t
nsm_qos_cmap_match_traffic_set (struct nsm_master *nm,
                                char *cname,
                                u_int32_t traffic_type,
                                enum nsm_qos_traffic_match criteria)
{
  struct nsm_cmap_list *cmapl = NULL;
  struct nsm_pmap_list *pmapl = NULL;

  cmapl = nsm_cmap_list_lookup (nm->cmap, cname);

  if (cmapl == NULL)
    return NSM_QOS_ERR_CMAP_NOT_FOUND;

  pmapl = nsm_qos_check_class_from_pmap (nm->pmap, cname, NSM_TRUE);
  if (pmapl)
   {
     zlog_err (NSM_ZG,
               "Traffic Type Cann't be set as class-map %s attached to Policy\n",
                cname);
     return NSM_QOS_ERR_ALREADY_BOUND;
   }
  cmapl->match_type = NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC;

  cmapl->traffic_type.criteria = criteria;

  if (traffic_type == NSM_QOS_TRAFFIC_ALL)
     cmapl->traffic_type.custom_traffic_type = traffic_type;
  else if (traffic_type != NSM_QOS_TRAFFIC_NONE)
     SET_FLAG (cmapl->traffic_type.custom_traffic_type, traffic_type);

#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
  nsm_cmap_dscp_ip_data_cal_modify(cmapl);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;
}

#ifdef HAVE_SMI

int
nsm_qos_get_egress_rate_shaping (struct nsm_qos_global *qosg,
                                 struct interface *ifp,
                                 u_int32_t *rate,
                                 enum smi_qos_rate_unit *rate_unit)
{
  struct nsm_qif_list *qifl;

  qifl = nsm_qif_list_lookup (qosg->qif, ifp->name);

  if (qifl == NULL)
    {
      *rate = 0;
      *rate_unit = NSM_QOS_RATE_KBPS;
      return 0;
    }

  *rate_unit = qifl->egress_rate_unit;
  *rate = qifl->egress_shape_rate;

  return NSM_QOS_SUCCESS;
}

s_int32_t
nsm_qos_cmap_match_traffic_get (struct nsm_master *nm,
                                char *cname,
                                enum smi_qos_traffic_match_mode *criteria,
                                u_int32_t *traffic_type)
{
  struct nsm_cmap_list *cmapl;

  cmapl = nsm_cmap_list_lookup (nm->cmap, cname);
  if (cmapl == NULL)
    return NSM_QOS_ERR_CMAP_NOT_FOUND;

  if (cmapl->match_type != NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC)
    return NSM_QOS_ERR_CMAP_NOT_FOUND;

  if (*criteria == SMI_QOS_CRITERIA_UNKNOWN)
   {
    *criteria = cmapl->traffic_type.criteria;
   }
  else
   {
     if (*criteria != cmapl->traffic_type.criteria)
       return NSM_QOS_ERR_CMAP_NOT_FOUND;
   }
  
  if (cmapl->traffic_type.custom_traffic_type == NSM_QOS_TRAFFIC_ALL)
   *traffic_type = SMI_QOS_TRAFFIC_ALL;
  else
   *traffic_type = cmapl->traffic_type.custom_traffic_type;

  return NSM_QOS_SUCCESS;
}
#endif /* HAVE_SMI */

s_int32_t
nsm_qos_cmap_match_traffic_unset (struct nsm_master *nm,
                                  char *cname,
                                  u_int32_t traffic_type)
{
  struct nsm_cmap_list *cmapl;
  struct nsm_pmap_list *pmapl = NULL;

  cmapl = nsm_cmap_list_lookup (nm->cmap, cname);

  if (cmapl == NULL)
    return NSM_QOS_ERR_CMAP_NOT_FOUND;

  pmapl = nsm_qos_check_class_from_pmap (nm->pmap, cname, NSM_TRUE);
  if (pmapl)
   {
     zlog_err (NSM_ZG,
               "Traffic Type Cann't be set as class-map %s attached to Policy\n",
                cname);
     return NSM_QOS_ERR_ALREADY_BOUND;
   }

  if (cmapl->match_type != NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC)
    return NSM_QOS_ERR_CMAP_MATCH_TYPE;

  UNSET_FLAG (cmapl->traffic_type.custom_traffic_type, traffic_type);

#ifdef HAVE_HA
  nsm_cmap_data_cal_modify(cmapl);
#endif /*HAVE_HA*/

  return NSM_QOS_SUCCESS;
}

s_int32_t
nsm_qos_cmap_match_unset_traffic (struct nsm_master *nm,
                                  char *cname)
{
  struct nsm_cmap_list *cmapl;

  cmapl = nsm_cmap_list_lookup (nm->cmap, cname);

  if (cmapl == NULL)
    return NSM_QOS_ERR_CMAP_NOT_FOUND;

  if (cmapl->match_type != NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC)
    return NSM_QOS_ERR_CMAP_MATCH_TYPE;

  cmapl->traffic_type.criteria = NSM_QOS_CRITERIA_NONE;
  cmapl->match_type = NSM_QOS_MATCH_TYPE_CUSTOM_TRAFFIC;
  cmapl->traffic_type.custom_traffic_type = NSM_QOS_TRAFFIC_NONE;
#ifdef HAVE_HA
  nsm_cmap_data_cal_modify (cmapl);
  nsm_cmap_dscp_ip_data_cal_modify (cmapl);
#endif

  return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_get_cos_to_queue (struct nsm_qos_global *qg,
                              u_int8_t *cos_to_queue)
{
  pal_mem_cpy(cos_to_queue, qg->cos_queue_map_tbl, COS_TBL_SIZE);

  return NSM_QOS_SUCCESS;
}

int
nsm_mls_qos_get_dscp_to_queue (struct nsm_qos_global *qg,
                              u_int8_t *dscp_to_queue)
{
  pal_mem_cpy(dscp_to_queue, qg->dscp_queue_map_tbl, DSCP_TBL_SIZE);

  return NSM_QOS_SUCCESS;
}

#ifdef HAVE_SMI
int
nsm_qos_get_trust_state (struct nsm_qos_global *qosg,
                         struct interface *ifp,
                         enum smi_qos_trust_state *trust_state)
{
  struct nsm_qif_list *qifl;

  *trust_state = SMI_QOS_TRUST_NONE;

  qifl = nsm_qif_list_lookup (qosg->qif, ifp->name);

  if (qifl)
    {
      switch (qifl->trust_type)
        {
          case NSM_QOS_TRUST_NONE:
           *trust_state = SMI_QOS_TRUST_NONE;
          break;
          case NSM_QOS_TRUST_COS:
          case NSM_QOS_TRUST_COS_PT_DSCP:
            *trust_state = SMI_QOS_TRUST_8021P;
          break;
          case NSM_QOS_TRUST_DSCP:
          case NSM_QOS_TRUST_DSCP_PT_COS:
            *trust_state = SMI_QOS_TRUST_DSCP;
          break;
          case NSM_QOS_TRUST_DSCP_COS:
            *trust_state = SMI_QOS_TRUST_BOTH;
          break;
          default:
           break;
        }
    }
  return NSM_QOS_SUCCESS;
}

int
nsm_qos_get_port_vlan_priority_override (struct nsm_qos_global *qosg,
                                         struct interface *ifp,
                                      enum smi_qos_vlan_pri_override *port_mode)
{
  struct nsm_qif_list *qifl;

  *port_mode = SMI_QOS_VLAN_PRI_OVERRIDE_NONE;

  qifl = nsm_qif_list_lookup (qosg->qif, ifp->name);

  if (qifl)
    {
      switch (qifl->port_mode)
        {
          case NSM_QOS_VLAN_PRI_OVERRIDE_NONE:
            *port_mode = SMI_QOS_VLAN_PRI_OVERRIDE_NONE;
          break;
          case NSM_QOS_VLAN_PRI_OVERRIDE_COS:
            *port_mode = SMI_QOS_VLAN_PRI_OVERRIDE_COS;
          break;
          case NSM_QOS_VLAN_PRI_OVERRIDE_QUEUE:
            *port_mode = SMI_QOS_VLAN_PRI_OVERRIDE_QUEUE;
          break;
          case NSM_QOS_VLAN_PRI_OVERRIDE_BOTH:
            *port_mode = SMI_QOS_VLAN_PRI_OVERRIDE_BOTH;
          break;
          default:
            break;
        }
    }
  return NSM_QOS_SUCCESS;
}

int
nsm_qos_get_port_da_priority_override (struct nsm_qos_global *qosg,
                                       struct interface *ifp,
                                       enum smi_qos_da_pri_override *port_mode)
{
  struct nsm_qif_list *qifl;

  *port_mode = SMI_QOS_DA_PRI_OVERRIDE_NONE;

  qifl = nsm_qif_list_lookup (qosg->qif, ifp->name);

  if (qifl)
    {
      switch (qifl->da_port_mode)
        {
          case NSM_QOS_DA_PRI_OVERRIDE_NONE:
            *port_mode = SMI_QOS_DA_PRI_OVERRIDE_NONE;
          break;
          case NSM_QOS_DA_PRI_OVERRIDE_COS:
            *port_mode = SMI_QOS_DA_PRI_OVERRIDE_COS;
          break;
          case NSM_QOS_DA_PRI_OVERRIDE_QUEUE:
            *port_mode = SMI_QOS_DA_PRI_OVERRIDE_QUEUE;
          break;
          case NSM_QOS_DA_PRI_OVERRIDE_BOTH:
            *port_mode = SMI_QOS_DA_PRI_OVERRIDE_BOTH;
          break;
          default:
           break;
        }
    }

  return NSM_QOS_SUCCESS;
}

#endif /* HAVE_SMI */

int
nsm_qos_get_force_trust_cos (struct nsm_qos_global *qosg,
                             struct interface *ifp,
                             u_int8_t *force_trust_cos)
{
  struct nsm_qif_list *qifl;

  *force_trust_cos = PAL_FALSE;

  qifl = nsm_qif_list_lookup (qosg->qif, ifp->name);

  if (qifl)
    *force_trust_cos = qifl->force_trust_cos;

  return NSM_QOS_SUCCESS;
}

#endif /* HAVE_QOS */
