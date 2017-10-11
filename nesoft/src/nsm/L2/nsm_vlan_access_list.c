/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#ifdef HAVE_VLAN
#ifdef HAVE_L2LERN
#include "lib.h"
#include "log.h"
#include "cli.h"
#include "show.h"
#include "filter.h"

#include "nsmd.h"

#include "nsm_vlan_access_list.h"
#include "nsm_qos_filter.h"

//extern struct nsm_qos_global   qosg;

/*----------------------*/
/* CMAP list            */
/*----------------------*/
/* Allocate new cmap-list structure. */
struct nsm_vlan_access_list *
nsm_vmap_list_new ()
{
  struct nsm_vlan_access_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct nsm_vlan_access_list));
  return new;
}

void
nsm_vmap_list_free (struct nsm_vlan_access_list *vmapl)
{
  if (vmapl->acl_name)
    XFREE (MTYPE_TMP, vmapl->acl_name);
  vmapl->acl_name = NULL;

  XFREE (MTYPE_TMP, vmapl);
}

int
nsm_vmap_list_master_init (struct nsm_master *nm)
{
  struct nsm_vlan_access_master *master;


  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_vlan_access_master));
  if (! master)
    return -1;

  nm->vmap = master;
  master->head = master->tail = NULL;
  return 0;
}

/* Lookup cmap-list from list of cmap-list by name. */
struct nsm_vlan_access_list *
nsm_vmap_list_lookup (struct nsm_vlan_access_master *master,
                      const char *vmap_name)
{
  struct nsm_vlan_access_list *vmapl;

  if (master == NULL || ! vmap_name)
    return NULL;

  for (vmapl = master->head; vmapl; vmapl = vmapl->next)
    if (vmapl->name && pal_strcmp (vmapl->name, vmap_name) == 0)
      return vmapl;
  return NULL;
}

/* Insert new cmap-list to list of advert-list. */
struct nsm_vlan_access_list *
nsm_vmap_list_insert (struct nsm_vlan_access_master *master,
                      const char *vmap_name,
                      const char *acl_name)
{
  struct nsm_vlan_access_list *vmapl;

  if (master == NULL || ! vmap_name)
    return NULL;

  /* Allocate new cmap-list and copy given name. */
  vmapl = nsm_vmap_list_new ();
  vmapl->master = master;

  if (master->tail)
    {
      master->tail->next = vmapl;
      vmapl->prev = master->tail;
    }
  else
    master->head = vmapl;

  master->tail = vmapl;
  vmapl->next = NULL;

  /* update cmap attributes */
  vmapl->name = XSTRDUP (MTYPE_TMP, vmap_name);

  if (acl_name)
    vmapl->acl_name = XSTRDUP (MTYPE_TMP, acl_name);
  else
    vmapl->acl_name = NULL;

  return vmapl;
}

/* Delete cmap-list from cmap_master and free it. */
void
nsm_vmap_list_delete (struct nsm_vlan_access_list *vmapl)
{
  struct nsm_vlan_access_master *master;

  master = vmapl->master;

  if (vmapl->next)
    vmapl->next->prev = vmapl->prev;
  else
    master->tail = vmapl->prev;

  if (vmapl->prev)
    vmapl->prev->next = vmapl->next;
  else
    master->head = vmapl->next;

  nsm_vmap_list_free (vmapl);
}


/* Get cmap-list from list of cmap-list.
   If there isn't matched cmap-list create new one and return it. */
struct nsm_vlan_access_list *
nsm_vmap_list_get (struct nsm_vlan_access_master *master,
                   const char *vmap_name)
{
  struct nsm_vlan_access_list *vmapl;

  vmapl = nsm_vmap_list_lookup (master, vmap_name);
  if (vmapl == NULL)
    vmapl = nsm_vmap_list_insert (master, vmap_name, NULL);
  return vmapl;
}

/* Insert acl name into class-map list */
int
nsm_insert_mac_acl_name_into_vmap (struct nsm_vlan_access_list *vmapl,
                                   const char *acl_name)
{
  if (vmapl->acl_name && pal_strcmp (vmapl->acl_name, acl_name) == 0)
    return 0;
  else if (! vmapl->acl_name)
    {
      vmapl->acl_name = XSTRDUP (MTYPE_TMP, acl_name);
      return 0;
    }
  else
    return -1;
}


/* Delete acl name from class-map list */
int
nsm_delete_mac_acl_name_from_vmap (struct nsm_vlan_access_list *vmapl,
                                   const char *acl_name)
{
  if (vmapl->acl_name && pal_strcmp (vmapl->acl_name, acl_name) == 0)
    {
      XFREE (MTYPE_TMP, vmapl->acl_name);
      vmapl->acl_name = NULL;
      return 0;
    }
  else
    return -1;
}
#endif /* HAVE_L2LERN */
#endif /* HAVE_VLAN */
