/* Copyright (C) 2007  All Rights Reserved. */

#include "pal.h"

#include "prefix.h"
#include "log.h"
#include "table.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */

#include "nsmd.h"
#include "nsm_mpls.h"
#include "nsm_mpls_dep.h"

struct confirm_list *
mpls_confirm_list_new (u_char node_type, struct prefix *p)
{
  struct confirm_list *c_list;

  c_list = XCALLOC (MTYPE_MPLS_CONFIRM_LIST, sizeof (struct confirm_list));
 
  if (c_list)
    {
      c_list->type = node_type; 
      if (p)
        prefix_copy (&c_list->p, p);
    }

  return c_list;
}

void
mpls_confirm_list_free (struct confirm_list *c_list)
{
  if (c_list)
    XFREE (MTYPE_MPLS_CONFIRM_LIST, c_list);
  
  return;
}

int
mpls_confirm_list_add (struct confirm_list *c_list, 
		       struct lsp_dep_confirm *confirm)
{
  NSM_ASSERT (c_list != NULL && confirm != NULL);
  if (! c_list || ! confirm)
    return NSM_ERR_INVALID_ARGS;
  
  if (c_list->head)
    c_list->head->prev = confirm;

  confirm->next = c_list->head;
  confirm->prev = NULL;
  c_list->head = confirm;

  if (! c_list->tail)
    c_list->tail = confirm;

  c_list->count++;

  return NSM_SUCCESS;
}

int
mpls_confirm_list_remove (struct confirm_list *c_list,
			  struct lsp_dep_confirm *confirm)
{
  NSM_ASSERT (c_list != NULL && confirm != NULL);
  if (! c_list || ! confirm)
    return NSM_ERR_INVALID_ARGS;
  
  if (confirm->next)
    confirm->next->prev = confirm->prev;
  if (confirm->prev)
    confirm->prev->next = confirm->next;

  if (c_list->head == confirm)
    c_list->head = confirm->next;

  if (c_list->tail == confirm)
    c_list->tail = confirm->prev;

  confirm->next = confirm->prev = NULL;

  c_list->count--;

  return NSM_SUCCESS;
}

struct lsp_dep_confirm *
mpls_confirm_list_lookup (struct confirm_list *c_list,
			  void *data, u_char dep_type, 
			  bool_t rem_flag)
{
  struct lsp_dep_confirm *confirm;

  NSM_ASSERT (c_list != NULL && data != NULL);
  if (! c_list || ! data)
    return NULL;

  for (confirm = c_list->head; confirm; confirm = confirm->next)
    {
      if (confirm->type == dep_type && confirm->data == data)
        {
	  if (rem_flag)
	    mpls_confirm_list_remove (c_list, confirm);
	  
	  return confirm;
        }
    }

  return NULL;
}

int
mpls_confirm_list_add_list (struct confirm_list *cl_dst, 
			    struct confirm_list *cl_src)
{
  NSM_ASSERT (cl_src != NULL && cl_dst != NULL);
  if (! cl_src || ! cl_dst)
    return NSM_ERR_INVALID_ARGS;

  if (cl_src->head)
    cl_src->head->prev = NULL;

  if (cl_src->tail)
    cl_src->tail->next = cl_dst->head;
  
  if (cl_dst->head)
    cl_dst->head->prev = cl_src->tail;
  
  cl_dst->head = cl_src->head;

  if (! cl_dst->tail)
    cl_dst->tail = cl_src->head;

  cl_dst->count += cl_src->count;

  return NSM_SUCCESS;
}

/* Move confirm to another table */
int
mpls_confirm_entry_move (struct lsp_dep_confirm *confirm, 
                         struct confirm_list *c_list, 
                         struct route_table *table,
                         int type, struct prefix *fec_prefix)
{
  struct route_node *rn_new;
  struct confirm_list *c_list_new = NULL;
  struct prefix *temp_prefix = NULL;  

  if (! table || ! confirm || ! c_list)
    return -1;
 
  if (fec_prefix == NULL)
    temp_prefix = &c_list->p;
  else
    temp_prefix = fec_prefix;

  rn_new = route_node_get (table, temp_prefix);

  if (! rn_new)
    return -1;

  /* Remove from source c_list */
  mpls_confirm_list_remove (c_list, confirm);

  if (rn_new->info)
    {
      c_list_new = rn_new->info;
      route_unlock_node (rn_new);
    }
  else
    {
      c_list_new = mpls_confirm_list_new (type, temp_prefix);
      if (! c_list_new)
        return -1;

      rn_new->info = c_list_new;
    }

  mpls_confirm_list_add (c_list_new, confirm);

  return 0;
}
