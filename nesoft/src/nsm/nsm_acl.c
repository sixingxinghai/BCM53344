#include "pal.h"
#ifdef HAVE_L2LERN
#include "lib.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */



#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_acl.h"
#include "nsm_mac_acl_cli.h"

int
mac_acl_list_master_init (struct nsm_master *nm)
{
  struct nsm_mac_acl_master *master;

  master = XCALLOC (MTYPE_TMP, sizeof (struct nsm_mac_acl_master));

  if (! master)
    return -1;

  nm->mac_acl = master;

  return 0;
}

struct mac_acl *
mac_acl_new ()
{
  struct mac_acl *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct mac_acl));
  return new;
}

/* Free allocated access_list. */
void
mac_acl_free (struct mac_acl *access)
{
  if (access->name)
    XFREE (MTYPE_TMP, access->name);

  XFREE (MTYPE_TMP, access);
}

struct mac_acl *
mac_acl_lock (struct mac_acl *access)
{
  access->ref_cnt++;
  return access;
}

void
mac_acl_unlock (struct mac_acl *access)
{
  access->ref_cnt--;
}


struct mac_acl *
mac_acl_insert (struct nsm_mac_acl_master *master, const char *name)
{
  //int i;
  //long number;
  struct mac_acl *access;
  struct mac_acl *point;
  struct mac_acl_list *alist;

  if (master == NULL)
    return NULL;

  /* Allocate new access_list and copy given name. */
  access = mac_acl_new ();
  access->name = XSTRDUP (MTYPE_TMP, name);
  access->ref_cnt = 0;
  access->master = master;

      /* Set access_list to string list. */
      alist = &master->num;

      /* Set point to insertion point. */
      for (point = alist->head; point; point = point->next)
        if (pal_strcmp (point->name, name) >= 0)
          break;

  /* In case of this is the first element of master. */
  if (alist->head == NULL)
    {
      alist->head = alist->tail = access;
      return access;
    }

  /* In case of insertion is made at the tail of access_list. */
  if (point == NULL)
    {
      access->prev = alist->tail;
      alist->tail->next = access;
      alist->tail = access;
      return access;
    }

  /* In case of insertion is made at the head of access_list. */
  if (point == alist->head)
    {
      access->next = alist->head;
      alist->head->prev = access;
      alist->head = access;
      return access;
    }

  /* Insertion is made at middle of the access_list. */
  access->next = point;
  access->prev = point->prev;

  if (point->prev)
    point->prev->next = access;
  point->prev = access;

  return access;
}

/* Delete access_list from access_master and free it. */
void
mac_acl_delete (struct mac_acl *access)
{
  struct mac_acl_list *dlist;
  struct nsm_mac_acl_master *master;

  if ( access->ref_cnt != 0)
    return;

  master = access->master;

  dlist = &master->num;

  if (access->next)
    access->next->prev = access->prev;
  else
    dlist->tail = access->prev;

  if (access->prev)
    access->prev->next = access->next;
  else
    dlist->head = access->next;

}



/* Lookup access_list from list of access_list by name. */
struct mac_acl *
mac_acl_lookup (struct nsm_mac_acl_master *master, const char *name)
{
  struct mac_acl *access;

  if (name == NULL)
    return NULL;

  if (master == NULL)
    return NULL;

  for (access = master->num.head; access; access = access->next)
    if (pal_strcmp (access->name, name) == 0)
      return access;

  /*for (access = master->str.head; access; access = access->next)
    if (pal_strcmp (access->name, name) == 0)
      return access;*/

  return NULL;
}


struct mac_acl *
mac_acl_get (struct nsm_mac_acl_master *master, const char *name)
{
  struct mac_acl *access;

  access = mac_acl_lookup (master, name);
  if (access == NULL)
    access = mac_acl_insert (master, name);
  return access;
}
#endif /* HAVE_L2LERN */
