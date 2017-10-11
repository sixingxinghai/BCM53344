/* Copyright (C) 2002-2005  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl_avl.h"
#include "hsl_error.h"
#include "hsl_table.h"
#include "hsl_ether.h"

#include "hsl_ifmgr.h"
#include "hsl_fib.h"
#include "hsl_fib_os.h"
#include "hsl_fib_hw.h"
#include "hal_types.h"
#include "hal_msg.h"
#include "hal_l3.h"

#ifdef HAVE_MPLS
#include "hsl_mpls.h"
#endif /* HAVE_MPLS */

/* Mac address table */
struct hsl_fib_table *p_hsl_fib_table = NULL;
static int hsl_fib_initialized = 0;

/* Forward declarations. */
int hsl_fib_hw_cb_register (void);
int hsl_fib_hw_cb_unregister (void);
int hsl_fib_os_cb_register (void);
int hsl_fibmgr_os_unregister (void);
/* 
   Comparision function for prefix 'pointer' tree.
*/
static int
_hsl_fib_prefix_ptr_cmp (void *param1, void *param2)
{
  unsigned int p1 = (unsigned int) param1;
  unsigned int p2 = (unsigned int) param2;

  /* Less than. */
  if (p1 < p2)
    return -1;

  /* Greater than. */
  if (p1 > p2)
    return 1;
  
  /* Equals to. */
  return 0;
}

/*
  Create NH entry. 
*/
static struct hsl_nh_entry *
_hsl_nh_entry_new (void)
{
  struct hsl_nh_entry *nh;
  int ret;

  nh = oss_malloc (sizeof (struct hsl_nh_entry), OSS_MEM_HEAP);
  if (! nh)
    return NULL;

  /* Create prefix 'pointer' tree. */
  ret = hsl_avl_create (&nh->prefix_tree, 0, _hsl_fib_prefix_ptr_cmp);
  if (ret < 0)
    {
      oss_free (nh, OSS_MEM_HEAP);
      return NULL;
    }

  return nh;
}

/* 
   Free NH entry.
*/
static void
_hsl_nh_entry_free (struct hsl_nh_entry *nh)
{
  if (nh)
    {
      /* Just delete the prefix 'pointer' tree. */
      if (nh->prefix_tree)
        hsl_avl_tree_free (&nh->prefix_tree, NULL); 

      oss_free (nh, OSS_MEM_HEAP);
    }
}

/*
  Free NH entry.
*/
static void
_hsl_nh_entry_free2 (void *data, void *data2)
{
  struct hsl_nh_entry *ent, *next;
  hsl_fib_id_t *p_fib_id;

  if (!data || !data2)
    return;

  p_fib_id = (hsl_fib_id_t *)data2;

  for (ent = (struct hsl_nh_entry *) data; ent != NULL; ent = next)
    {
      next = ent->next;

      if (CHECK_FLAG (ent->flags, HSL_NH_ENTRY_VALID))
        {
          /* Delete nh itself. */ 
           if (HSL_FIB_HWCB_CHECK(hw_nh_delete))
             HSL_FIB_HWCB_CALL(hw_nh_delete) (*p_fib_id, ent);

          /* Hardware ageing occured for this nexthop entry. Just delete
             ARP from the OS. */
          if (p_hsl_fib_table->os_cb && p_hsl_fib_table->os_cb->os_nh_delete)
            (*p_hsl_fib_table->os_cb->os_nh_delete) (*p_fib_id, ent);
        }

      _hsl_nh_entry_free (ent);
    }

}

/* 
   Create prefix entry.
*/
static struct hsl_prefix_entry *
_hsl_prefix_entry_new (void)
{
  return oss_malloc (sizeof (struct hsl_prefix_entry), OSS_MEM_HEAP);
}

/*
  Free prefix entry.
*/
static void
_hsl_prefix_entry_free (struct hsl_prefix_entry *p)
{
  HSL_FN_ENTER();

  if (p)
    oss_free (p, OSS_MEM_HEAP);

  HSL_FN_EXIT(STATUS_OK);
}

/* 
   Free prefix entry. 
*/
static void
_hsl_prefix_entry_free2 (void *data, void *data2)
{
  _hsl_prefix_entry_free ((struct hsl_prefix_entry *) data);
}

/*
  Allocate NH entry list node.
*/
static struct hsl_nh_entry_list_node *
_hsl_nh_entry_list_node_new (void)
{
  return oss_malloc (sizeof (struct hsl_nh_entry_list_node), OSS_MEM_HEAP);
}

/*
  Free NH entry list node.
*/
static void
_hsl_nh_entry_list_node_free (struct hsl_nh_entry_list_node *nh)
{
  if (nh)
    oss_free (nh, OSS_MEM_HEAP);
}

/*
  Allocate interface NH entry list node.
*/
static struct hsl_nh_if_list_node *
_hsl_nh_if_list_node_new (void)
{
  return oss_malloc (sizeof (struct hsl_nh_if_list_node), OSS_MEM_HEAP);
}

/*
  Free interface NH entry list node.
*/
static void
_hsl_nh_if_list_node_free (struct hsl_nh_if_list_node *nh)
{
  if (nh)
    oss_free (nh, OSS_MEM_HEAP);
}

/*
  Initialize NH table. 
*/
static struct hsl_route_table *
_hsl_nh_table_init ()
{
  struct hsl_route_table *t;

  t = hsl_route_table_init ();
  if (! t)
    return NULL;

  return t;
}

/*
  Common function to delete table(prefix/nh)
*/
static int
_hsl_table_deinit (struct hsl_route_node *top, void (*free_func) (void *data, void *data2), void *data2)
{
  struct hsl_route_node *tmp_node;
  struct hsl_route_node *node;
  void *data;

  if (! top)
    return 0;

  node = top;

  while (node)
    {
      if (node->l_left)
        {
          node = node->l_left;
          continue;
        }

      if (node->l_right)
        {
          node = node->l_right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          if (node->l_left == tmp_node)
            node->l_left = NULL;
          else
            node->l_right = NULL;

          data = (void *) tmp_node->info;
          if (free_func)
            (*free_func) (data, data2);

          hsl_route_node_free (tmp_node);
        }
      else
        {
          data = tmp_node->info;
          if (free_func)
            (*free_func) (data, data2);

          hsl_route_node_free (tmp_node);
          break;
        }
    }

  return 0;
}

/*
  Deinitialize NH table. 
*/
static int
_hsl_nh_table_deinit (struct hsl_route_table *nh, void *data2)
{
  if (nh == NULL)
    return 0;

  _hsl_table_deinit (nh->top, _hsl_nh_entry_free2, data2);

  oss_free (nh, OSS_MEM_HEAP);

  return 0;
}
  
/* 
   Initialize prefix table.
*/
static struct hsl_route_table *
_hsl_prefix_table_init ()
{
  struct hsl_route_table *t;

  t = hsl_route_table_init ();
  if (! t)
    return NULL;

  return t;  
}

/*
  Deinitialize Prefix table. 
*/
static int
_hsl_prefix_table_deinit (struct hsl_route_table *pt, void *data2)
{
  if (pt == NULL)
    return 0;

  _hsl_table_deinit (pt->top, _hsl_prefix_entry_free2, data2);

  oss_free (pt, OSS_MEM_HEAP);

  return 0;
}

/* Create a FIB table for a given id */
int
hsl_fib_create_tables (hsl_fib_id_t fib_id)
{
  int ret = HSL_FIB_ERR_NOMEM;

  struct hsl_route_table *nh = NULL, *prefix = NULL;
#ifdef HAVE_IPV6
  struct hsl_route_table *nh6 = NULL, *prefix6 = NULL;
#endif /* HAVE_IPV6 */

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  HSL_FIB_LOCK;

  /* Check if tables already exists */
  if (HSL_FIB_TBL_VALID (prefix, fib_id))
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT (STATUS_OK);
    }

  HSL_FIB_UNLOCK;

  /* Initialize NH table. */
  nh = _hsl_nh_table_init ();
  if (! nh)
    goto ERR;

  /* Initialize prefix table. */
  prefix = _hsl_prefix_table_init ();
  if (! prefix)
    goto ERR;


#ifdef HAVE_IPV6
  /* Initialize IPV6 NH table. */
  nh6 = _hsl_nh_table_init ();
  if (! nh6)
    goto ERR;

  /* Initialize IPV6 prefix table. */
  prefix6 = _hsl_prefix_table_init ();
  if (! prefix6)
    goto ERR;
#endif /* HAVE_IPV6 */

  /* Initialize fib in hw. */
  if (HSL_FIB_HWCB_CHECK(hw_fib_init))
    {
      ret = HSL_FIB_HWCB_CALL(hw_fib_init) (fib_id);
      if (ret < 0)
        goto ERR;
    }

  /* Add FIB to the IP stack. */
  if (HSL_FIB_OSCB_CHECK(os_fib_init))
    {
      ret = HSL_FIB_OSCB_CALL(os_fib_init) (fib_id);
      if (ret < 0)
        goto ERR;
    }

  HSL_FIB_LOCK;

  /* Initialize IPv4 nexthop table. */
  HSL_FIB_TBL (nh, fib_id) = nh;

  /* Initialize IPv4 prefix table. */
  HSL_FIB_TBL (prefix, fib_id) = prefix;

#ifdef HAVE_IPV6
  /* Initialize IPv6 nexthop table. */
  HSL_FIB_TBL (nh6, fib_id) = nh6;

  /* Initialize IPv6 prefix table. */
  HSL_FIB_TBL (prefix6, fib_id) = prefix6;
#endif /* HAVE_IPV6 */

  HSL_FIB_UNLOCK;

  ret = 0;

  HSL_FN_EXIT (ret);

ERR:
  if (nh)
    _hsl_nh_table_deinit (nh, (void *)&fib_id);

  if (prefix)
    _hsl_prefix_table_deinit (prefix, (void *)&fib_id);

#ifdef HAVE_IPV6
  if (nh6)
    _hsl_nh_table_deinit (nh6, (void *)&fib_id);

  if (prefix6)
    _hsl_prefix_table_deinit (prefix6, (void *)&fib_id);
#endif /* HAVE_IPV6 */

  HSL_FN_EXIT (ret);
}

/* Destroy FIB tables for a given id */
/* The assumption here is all the routes and nexthops will be 
 * destroyed by CP before this call is invoked
 */
int
hsl_fib_destroy_tables (hsl_fib_id_t fib_id)
{
  int ret = 0;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Deinitialize fib in hw. */
  if (HSL_FIB_HWCB_CHECK(hw_fib_deinit))
    {
      ret = HSL_FIB_HWCB_CALL(hw_fib_deinit) (fib_id);
      if (ret < 0)
        HSL_FN_EXIT (ret);
    }


  /* Delete FIB from the IP stack. */
  if (HSL_FIB_OSCB_CHECK(os_fib_deinit))
    {
      ret = HSL_FIB_OSCB_CALL(os_fib_deinit) (fib_id);
      if (ret < 0)
        HSL_FN_EXIT (ret);
    }

  HSL_FIB_LOCK;

  /* IPv4 nexthop table deinitialization. */
  if (HSL_FIB_TBL_VALID (nh, fib_id))
    _hsl_nh_table_deinit (HSL_FIB_TBL (nh, fib_id), (void *)&fib_id);

  /* IPv4 prefix table deinitialization. */
  if (HSL_FIB_TBL_VALID (prefix, fib_id))
    _hsl_prefix_table_deinit (HSL_FIB_TBL (prefix, fib_id), (void *)&fib_id);

#ifdef HAVE_IPV6
  /* IPv6 nexthop table deinitialization. */
  if (HSL_FIB_TBL_VALID (nh6, fib_id))
    _hsl_nh_table_deinit (HSL_FIB_TBL (nh6, fib_id), (void *)&fib_id);

  /* IPv6 prefix table deinitialization. */
  if (HSL_FIB_TBL_VALID (prefix6, fib_id))
    _hsl_prefix_table_deinit (HSL_FIB_TBL (prefix6, fib_id), (void *)&fib_id);
#endif /* HAVE_IPV6 */

  /* Initialize IPv4 nexthop table. */
  HSL_FIB_TBL (nh, fib_id) = NULL;

  /* Initialize IPv4 prefix table. */
  HSL_FIB_TBL (prefix, fib_id) = NULL;

#ifdef HAVE_IPV6
  /* Initialize IPv6 nexthop table. */
  HSL_FIB_TBL (nh6, fib_id) = NULL;

  /* Initialize IPv6 prefix table. */
  HSL_FIB_TBL (prefix6, fib_id) = NULL;
#endif /* HAVE_IPV6 */

  HSL_FIB_UNLOCK;

  HSL_FN_EXIT (ret);
}

/*
  Initialize FIB table.
*/
int
hsl_fib_init (void)
{
  int ret;

  HSL_FN_ENTER();

  /* If already initialized, just return. */
  if (hsl_fib_initialized)
    HSL_FN_EXIT (0);

  p_hsl_fib_table = oss_malloc (sizeof (struct hsl_fib_table), OSS_MEM_HEAP);
  if (! p_hsl_fib_table)
    goto ERR;

  /* Create protection mutex. */
  ret = oss_sem_new ("FIB Mutex", OSS_SEM_MUTEX, 0, NULL, &p_hsl_fib_table->mutex);
  if (ret < 0)
    goto ERR;

  /* Initialize FIB manager hardware callbacks. */
  hsl_fib_hw_cb_register ();

  /* Initialize FIb manager OS(TCP/IP) stack callbacks. */
  hsl_fib_os_cb_register ();

  /* FIB initialized. */
  hsl_fib_initialized = 1;

  HSL_FN_EXIT (0);

 ERR:
  if (p_hsl_fib_table->mutex)
    oss_sem_delete (OSS_SEM_MUTEX, p_hsl_fib_table->mutex);

  if (p_hsl_fib_table)
    oss_free (p_hsl_fib_table, OSS_MEM_HEAP);

  HSL_FN_EXIT (0);
}

/* 
   Deinitialize FIB table. 
*/
int
hsl_fib_deinit (void)
{
  int i;

  HSL_FN_ENTER();

  /* If FIB already initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (0);

  if (p_hsl_fib_table)
    {
      for (i = 0; i < HSL_MAX_FIB; i++)
        hsl_fib_destroy_tables (i);

      /* Unregister FIB manager hardware callbacks. */
      hsl_fibmgr_hw_cb_unregister ();
      
      /* Unregister FIB manager OS(TCP/IP) stack callbacks. */
      hsl_fibmgr_os_unregister ();    

      /* Delete FIB mutex. */
      if (p_hsl_fib_table->mutex)
        oss_sem_delete (OSS_SEM_MUTEX, p_hsl_fib_table->mutex);

      /* Free fib table. */
      oss_free (p_hsl_fib_table, OSS_MEM_HEAP);
    }

  /* Unset FIB initialization flag. */
  hsl_fib_initialized = 0;

  HSL_FN_EXIT (0);
}

/*
  Register HW callbacks with FIB manager.
*/
int
hsl_fibmgr_hw_cb_register (struct hsl_fib_hw_callbacks *cb)
{
  p_hsl_fib_table->hw_cb = cb;
  return 0;
}

/*
  Unregister HW callbacks for FIB manager.
*/
int
hsl_fibmgr_hw_cb_unregister (void)
{
  p_hsl_fib_table->hw_cb = NULL;
  return 0;
}

/* 
   Register OS(TCP/IP) callbacks with FIB manager.
*/
int
hsl_fibmgr_os_cb_register (struct hsl_fib_os_callbacks *cb)
{
  p_hsl_fib_table->os_cb = cb;
  return 0;
}

/*
  Unregister OS(TCP/IP) callbacks with FIB manager.
*/
int
hsl_fibmgr_os_unregister (void)
{
  p_hsl_fib_table->os_cb = NULL;
  return 0;
}


/*
   Get maximum number of multipaths.
*/
int
hsl_fib_get_max_num_multipath(u_int32_t *ecmp)
{
  HSL_FN_ENTER();
  if(!ecmp) 
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  if (HSL_FIB_HWCB_CHECK(hw_get_max_multipath))
    HSL_FIB_HWCB_CALL(hw_get_max_multipath) (ecmp);

  HSL_FN_EXIT (0);
}

/*
  Add prefix pointer to the NH.
*/
static int
_hsl_fib_add_prefix_to_nh (struct hsl_nh_entry *nh, struct hsl_route_node *rnp)
{
  HSL_FN_ENTER();
  
  if (! nh || ! rnp)
    return HSL_FIB_ERR_INVALID_PARAM;

  /* Add prefix to nexthop's AVL tree. */
  if (nh->prefix_tree)
    hsl_avl_insert (nh->prefix_tree, rnp);

  HSL_FN_EXIT(0);
}

/*
  Delete prefix pointer from the NH.
*/
static int
_hsl_fib_delete_prefix_from_nh (struct hsl_nh_entry *nh, struct hsl_route_node *rnp)
{
  HSL_FN_ENTER();
  
  if (! nh || ! rnp)
    return HSL_FIB_ERR_INVALID_PARAM;

  /* Delete prefix from nexthop's AVL tree. */
  if (nh->prefix_tree)
    hsl_avl_delete (nh->prefix_tree, rnp);

  HSL_FN_EXIT(0);
}

/*
  Add NH entry to prefix list.
*/
static int
_hsl_fib_add_nh_to_prefix (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct hsl_nh_entry_list_node *nhlist;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER();

  if (! rnp || ! rnp->info || ! nh)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  pe = rnp->info;

  /* First check if it exists. */
  for (nhlist = pe->nhlist; nhlist; nhlist = nhlist->next)
    {
      if (nhlist->entry == nh)
        {
          /* already in the list. */
          return HSL_FIB_ERR_NH_ALREADY_PRESENT_FOR_PREFIX;     
        }
    }

  /* Allocate new. */
  nhlist =_hsl_nh_entry_list_node_new ();
  if (! nhlist)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Memory allocation failure\n");
      return HSL_FIB_ERR_NOMEM;
    }

  nhlist->entry = nh;
  nhlist->next = pe->nhlist;
  pe->nhlist = nhlist;
  
  HSL_FN_EXIT(0);
}

/*
  Delete NH from prefix.
*/
static int
_hsl_fib_delete_nh_from_prefix (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct hsl_nh_entry_list_node *nhlist, *prev;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER();

  if (! rnp || ! rnp->info || ! nh)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  pe = rnp->info;

  /* First check if it exists. */
  prev = NULL;
  for (nhlist = pe->nhlist; nhlist; nhlist = nhlist->next)
    {
      if (nhlist->entry == nh)
        { 
          /* Unlink nh. */
          if (prev)
            prev->next = nhlist->next;
          else
            pe->nhlist = nhlist->next;

          /* Free nhlist node. */
          _hsl_nh_entry_list_node_free (nhlist);
          
          HSL_FN_EXIT(0);
        }       

      /* Previous. */
      prev = nhlist;
    }

  HSL_FN_EXIT(HSL_FIB_ERR_NH_NOT_PRESENT_FOR_PREFIX);
}


/*
  Add NH(ifp) entry to prefix list.
*/
static int
_hsl_fib_add_ifp_to_prefix (struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  struct hsl_nh_if_list_node *iflist;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER();

  if (! rnp || ! rnp->info)
    return HSL_FIB_ERR_INVALID_PARAM;

  pe = rnp->info;

  /* First check if it exists. */
  for (iflist = pe->iflist; iflist; iflist = iflist->next)
    {
      if (iflist->ifp == ifp)
        {
          /* already in the list. */
          return HSL_FIB_ERR_NH_ALREADY_PRESENT_FOR_PREFIX;     
        }
    }

  /* Allocate new. */
  iflist = _hsl_nh_if_list_node_new ();
  if (! iflist)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Memory allocation failure\n");
      return HSL_FIB_ERR_NOMEM;
    }

  iflist->ifp = ifp;
  iflist->next = pe->iflist;
  pe->iflist = iflist;
  
  /* Increment count. */
  pe->ifcount++;

  HSL_FN_EXIT(0);
}

/*
  Delete NH(ifp) from prefix.
*/
static int
_hsl_fib_delete_ifp_from_prefix (struct hsl_route_node *rnp, struct hsl_if *ifp)
{
  struct hsl_nh_if_list_node *iflist, *prev;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER();

  if (! rnp || ! rnp->info)
    return HSL_FIB_ERR_INVALID_PARAM;

  pe = rnp->info;

  /* First check if it exists. */
  prev = NULL;
  for (iflist = pe->iflist; iflist; iflist = iflist->next)
    {
      if (iflist->ifp == ifp)
        { 
          /* Unlink nh. */
          if (prev)
            prev->next = iflist->next;
          else
            pe->iflist = iflist->next;

          /* Free nhlist node. */
          _hsl_nh_if_list_node_free (iflist);
          /* decrement the if count */
          pe->ifcount--;
          return 0;
        }       

      /* Previous. */ 
      prev = iflist;
    }

  HSL_FN_EXIT(HSL_FIB_ERR_NH_NOT_PRESENT_FOR_PREFIX);
}

#define RNODE_LEFT(rnt)                                 \
  do {                                                  \
    while (rnt->l_left && (rnt->l_left->info == NULL))  \
      rnt = rnt->l_left;                                \
    if (rnt && rnt->l_left)                             \
      rnt = rnt->l_left;                                \
  } while (0)         

#define RNODE_RIGHT(rnt)                                        \
  do {                                                          \
    while (rnt->l_right && (rnt->l_right->info == NULL))        \
      rnt = rnt->l_right;                                       \
    if (rnt && rnt->l_right)                                    \
      rnt = rnt->l_right;                                       \
  } while (0)         

#define RNODE_PARENT(rnt)                               \
  do {                                                  \
    while (rnt->parent && (rnt->parent->info == NULL))  \
      rnt = rnt->parent;                                \
    if (rnt && rnt->parent)                             \
      rnt = rnt->parent;                                \
  } while (0)         
            
/*
  Link NH and prefix. 
*/
int
hsl_fib_link_nh_and_prefix (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  int ret;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER();

  if (! rnp || ! rnp->info || ! nh)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  pe = rnp->info;

  /* Add NH to prefix. */
  ret = _hsl_fib_add_nh_to_prefix (rnp, nh);
  if (! (ret == 0) || (ret == HSL_FIB_ERR_NH_ALREADY_PRESENT_FOR_PREFIX))
    goto ERR1;

  /* Add prefix to NH. */
  ret = _hsl_fib_add_prefix_to_nh (nh, rnp);
  if (ret < 0)
    goto ERR2;

  /* Increment refcnt respectively. */
  nh->refcnt++;
  pe->nhcount++;

  HSL_FN_EXIT(0);

 ERR2:
  /* Delete the NH from prefix. */
  _hsl_fib_delete_nh_from_prefix (rnp, nh);
 ERR1:
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error linking NH and prefix");

  if (ret == HSL_FIB_ERR_NH_ALREADY_PRESENT_FOR_PREFIX)
    HSL_FN_EXIT (HSL_FIB_ERR_NH_ALREADY_PRESENT_FOR_PREFIX);

  HSL_FN_EXIT(HSL_FIB_ERR_LINK_NH_PREFIX);
}

/*
  Unlink NH and prefix. 
*/
int
hsl_fib_unlink_nh_and_prefix (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  int ret;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER();

  if (! rnp || ! rnp->info || ! nh)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  pe = rnp->info;

  /* Delete prefix from NH. */
  _hsl_fib_delete_prefix_from_nh (nh, rnp);

  /* Delete NH from prefix. */
  ret = _hsl_fib_delete_nh_from_prefix (rnp, nh);
  if (ret < 0)
    HSL_FN_EXIT(ret);

  /* Decrement refcnt respectively. */
  nh->refcnt--;
  pe->nhcount--;

  HSL_FN_EXIT(0);
}

/*
  Add prefix to stack. 
*/
int
hsl_fib_add_to_stack (hsl_fib_id_t fib_id,
    struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  int ret = 0;

  HSL_FN_ENTER();

  /* Most stacks will not have a mechanism to 'update' the entries with new nexthops.
     The rnp's nh list will need to be accessed in full to populate this entry. */

  /* Send it to the IP stack. */
  if (p_hsl_fib_table->os_cb && p_hsl_fib_table->os_cb->os_prefix_add)
    {
      ret = (*p_hsl_fib_table->os_cb->os_prefix_add) (fib_id, rnp, nh);
      if (ret < 0)
       {

         HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to add %s prefix 0x%x/0x%x nexthop 0x%x entry to TCP/IP stack ret= %d\n",
                  rnp->p.family == AF_INET ? "IPv4" : "IPv6", rnp->p.u.prefix4, rnp->p.prefixlen, nh->rn->p.u.prefix4, ret);
         HSL_FN_EXIT(ret);
       }
    }

  HSL_FN_EXIT(0);
}

/*
  Delete prefix from stack.
*/
int
hsl_fib_delete_from_stack (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  HSL_FN_ENTER();

  /* Most stacks will not have a mechanism to 'delete' selective entries for a nexthop.
     The rnp's nh list will need to be accessed in full to update the 'new' entry. */

  /* Send delete to IP stack. */
  if (p_hsl_fib_table->os_cb && p_hsl_fib_table->os_cb->os_prefix_delete)
    (*p_hsl_fib_table->os_cb->os_prefix_delete) (fib_id, rnp, nh);
  
  HSL_FN_EXIT(0);
}

/*
  Internal function to prune children who are exceptions.
*/
static void
_hsl_fib_prune_exception_children (hsl_fib_id_t fib_id, struct hsl_route_node *rnp)
{
  struct hsl_route_node *rnt;

  HSL_FN_ENTER();

  if (! rnp)
    HSL_FN_EXIT(-1);

  /* 1. Is left child a exception. */
  rnt = rnp;
  if (rnp->l_left)
    {
      RNODE_LEFT(rnt);
      if ((rnt != rnp) && rnt && rnt->info)
        {
          if (HSL_PREFIX_ENTRY_IS_EXCEPTION (rnt))
            {
              /* Delete it from hardware. */
              if (HSL_FIB_HWCB_CHECK(hw_prefix_delete))
                HSL_FIB_HWCB_CALL(hw_prefix_delete) (fib_id, rnt, NULL);
            }
        }
    }
                      
  /* 2. Is right child a exception. */
  rnt = rnp;
  if (rnp->l_right)
    {
      RNODE_RIGHT(rnt);
      if ((rnt != rnp) && rnt && rnt->info)
        {
          if (HSL_PREFIX_ENTRY_IS_EXCEPTION (rnt))
            {
              /* Delete it from hardware. */
              if (HSL_FIB_HWCB_CHECK(hw_prefix_delete))
                HSL_FIB_HWCB_CALL(hw_prefix_delete) (fib_id, rnt, NULL);
            }
        }
    }

  HSL_FN_EXIT(STATUS_OK);
}

/*
  Internal function to handle failure for prefix addition to hardware.
*/
static void
_hsl_fib_handle_prefix_add_failure (hsl_fib_id_t fib_id, struct hsl_route_node *rnp)
{
  struct hsl_route_node *rnt, *rnm;

  HSL_FN_ENTER();

  if (! rnp || ! rnp->parent)
    HSL_FN_EXIT (-1);

  rnt = rnp;
  RNODE_PARENT(rnt);
  if (rnt == rnp)
    {
      /* No other parent found. Can happen for a top node.
         No need to take care of any exceptions as this will be
         trapped to the CPU. */
      HSL_FN_EXIT (-1); 
    }

  if (rnt && rnt->info)
    {
      if (HSL_PREFIX_ENTRY_IS_EXCEPTION (rnt))
        HSL_FN_EXIT(-1);

      /* Check if there is another parent who is a exception. */
      rnm = rnt;
      RNODE_PARENT (rnm);
      if (rnt == rnm)
        {
          if (HSL_FIB_HWCB_CHECK(hw_prefix_add_exception))
            HSL_FIB_HWCB_CALL(hw_prefix_add_exception) (fib_id, rnt);

        }
      else if (rnm  
               && rnm->info
               && HSL_PREFIX_ENTRY_IS_EXCEPTION (rnm) 
               && HSL_PREFIX_ENTRY_POPULATED_IN_HW(rnt))
        {
          if (HSL_FIB_HWCB_CHECK(hw_prefix_delete))
            HSL_FIB_HWCB_CALL(hw_prefix_delete) (fib_id, rnt, NULL);
        }

      /* Prune children of node who are exceptions. */
      _hsl_fib_prune_exception_children (fib_id, rnt);
    }
      
  HSL_FN_EXIT(STATUS_OK);
}








/*
  ADD prefix to hardware.
*/
int
hsl_fib_add_to_hw (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  int ret;
  struct hsl_route_node *rnt;
  int lstat, rstat;
  struct hsl_prefix_entry *pe;

  HSL_FN_ENTER();

#ifdef HAVE_MPLS
  if (nh->nh_type == HSL_NH_TYPE_MPLS)
    HSL_FN_EXIT (hsl_mpls_ftn_add_to_hw (rnp, nh));
#endif /* HAVE_MPLS */

  pe = rnp->info;

  ret = 0; 
  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID))
    {     
      /* If entry not in hardware, populate based on the children. */
      if (! HSL_PREFIX_ENTRY_POPULATED_IN_HW (rnp))
        {
          /* Go down children and check if they are real entries. */

          lstat = rstat = 1;
          
          /* 1. Left child. */
          rnt = rnp;
          if (rnp->l_left)
            { 
              RNODE_LEFT(rnt);
              if ((rnt != rnp) && rnt && rnt->info)
                lstat = HSL_PREFIX_ENTRY_POPULATED_IN_HW (rnt) ? 1 : 0;
              else 
                lstat = 1; 
            }
          else
            lstat = 1;

          /* 2. Right child. */
          rnt = rnp;
          if (rnp->l_right)
            {
              RNODE_RIGHT(rnt);
              if ((rnt != rnp) && rnt && rnt->info)
                rstat = HSL_PREFIX_ENTRY_POPULATED_IN_HW (rnt) ? 1 : 0;
              else
                rstat = 1;
            }
          else
            rstat = 1;

          if (lstat && rstat)
            {
              /* All children are in hardware, add this as a real entry. */
              
              /* Send it to the hardware. */
              if (HSL_FIB_HWCB_CHECK(hw_prefix_add))
                {
                  ret = HSL_FIB_HWCB_CALL(hw_prefix_add) (fib_id, rnp, nh);
                  if (ret < 0)
                    {
                      /* Failed to add prefix. */     
                      _hsl_fib_handle_prefix_add_failure (fib_id, rnp);
                    }
                  else
                    {
                      /* Validate the parent for exception. If exception reprocess to see if 
                         it can be populated. */
                      rnt = rnp;
                      if (rnp->parent)
                        {
                          RNODE_PARENT(rnt);
                          if ((rnt != rnp) 
                              && rnt 
                              && rnt->info 
                              && HSL_PREFIX_ENTRY_IS_EXCEPTION (rnt))
                            {
                              hsl_fib_reprocess_prefix (fib_id, rnt);
                            }
                        }
                    }
                } 
            }
          else
            {
              /* Some of the children are not in hardware, just add this new one as a dummy entry. */
              if (HSL_FIB_HWCB_CHECK(hw_prefix_add_exception))
                {
                  ret = HSL_FIB_HWCB_CALL(hw_prefix_add_exception) (fib_id, rnp);
                  if (ret < 0)
                    {
                      /* Failed to add exception. */
                      _hsl_fib_handle_prefix_add_failure (fib_id, rnp);
                    }
                  else
                    {
                      /* Go down and prune child exception nodes. */
                      _hsl_fib_prune_exception_children (fib_id, rnp);
                    }
                }
            }
        }
      else
        {
          if (HSL_FIB_HWCB_CHECK(hw_prefix_add))
            {
              ret = HSL_FIB_HWCB_CALL(hw_prefix_add) (fib_id, rnp, nh);
              if (ret < 0)
                {
                  /* We already had a route in the hardware. Only the 'new' nexthop addition failed.
                     This is very unlikely to fail. */
                }
            }
        }
    }
  else
    {
      /* If there is already, a route installed in hw, ignore this path. 
         This is the case where 
         * One of the nexthops is not active but others are 
         OR
         * There is a dummy entry. */
      if (HSL_PREFIX_ENTRY_IS_EXCEPTION (rnp) || HSL_PREFIX_ENTRY_IS_REAL (rnp))
        HSL_FN_EXIT(0);

      /* Go up parent. */
      rnt = rnp;

      RNODE_PARENT(rnt);
      if ((rnt != rnp) && rnt && rnt->info)
        if (HSL_PREFIX_ENTRY_IS_EXCEPTION (rnt))
          HSL_FN_EXIT(0);

      /* Add this prefix as a exception. */
      if (HSL_FIB_HWCB_CHECK(hw_prefix_add_exception))
        {
          ret = HSL_FIB_HWCB_CALL(hw_prefix_add_exception) (fib_id, rnp);
          if (ret < 0)
            {
              /* Failed to add exception. */
              _hsl_fib_handle_prefix_add_failure (fib_id, rnp);
            }
        }
    }
  
  HSL_FN_EXIT(0);
}

/*
  Delete prefix from hardware.
*/
int
hsl_fib_delete_from_hw (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct hsl_route_node *rnt, *rnl, *rnr;
  struct hsl_prefix_entry *pe;
  int lstat, rstat;
  int ret;

  HSL_FN_ENTER();

#ifdef HAVE_MPLS
  if (nh->nh_type == HSL_NH_TYPE_MPLS)
    HSL_FN_EXIT (hsl_mpls_ftn_del_from_hw (rnp, nh));
#endif /* HAVE_MPLS */

  pe = rnp->info;
  rstat = lstat = 0;

  /* Its a real entry. Delete from hardware, only if the NH is valid. */
  if ((CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID)) || 
      (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_DEL_IN_PROGRESS)))

    {     
      /* Delete it from hardware. */
      if (HSL_FIB_HWCB_CHECK(hw_prefix_delete))
        HSL_FIB_HWCB_CALL(hw_prefix_delete) (fib_id, rnp, nh);
    }

  if (HSL_PREFIX_DELETE_ELIGIBLE (pe))
    {
      if (HSL_PREFIX_ENTRY_IS_EXCEPTION (rnp))
        {
          /* Delete from hardware. */
          if (HSL_FIB_HWCB_CHECK(hw_prefix_delete))
            HSL_FIB_HWCB_CALL(hw_prefix_delete) (fib_id, rnp, NULL);
        }
      else
        {
          rnt = rnp;
          if (! rnp->parent)
            HSL_FN_EXIT(0);

          RNODE_PARENT(rnt);
          if ((rnt != rnp) && rnt && rnt->info)
            {
              if (HSL_PREFIX_ENTRY_IS_EXCEPTION (rnt))
                {
                  /* If parent is exception, then prune the children who are exceptions. */
                  _hsl_fib_prune_exception_children (fib_id, rnp);
                }
              else if (HSL_PREFIX_ENTRY_IS_REAL (rnt))
                {
                  /* If parent is valid, then check the children if they are not in hw. */
              
                  /* Left child. */
                  rnl = rnp;
                  if (rnp->l_left)
                    {
                      RNODE_LEFT(rnl);
                      if ((rnl != rnp) && rnl && rnl->info)
                        { 
                          if (! HSL_PREFIX_ENTRY_POPULATED_IN_HW (rnl))
                            lstat = 1;
                        }
                      else
                        lstat = 1;
                    }
                  /* Right child. */
                  rnr = rnp;
                  if (rnp->l_right)
                    {
                      RNODE_RIGHT(rnr);
                      if ((rnr != rnp) && rnr && rnr->info)
                        {
                          if (! HSL_PREFIX_ENTRY_POPULATED_IN_HW (rnr))
                            rstat = 1;
                        }
                      else
                        rstat = 1;
                    }

                  if (lstat || rstat)
                    {
                      if (rnt->parent && rnt->parent->info && HSL_PREFIX_ENTRY_IS_EXCEPTION (rnt->parent))
                        {
                          /* Delete from hardware. */
                          if (HSL_FIB_HWCB_CHECK(hw_prefix_delete))
                            HSL_FIB_HWCB_CALL(hw_prefix_delete) (fib_id, rnt, NULL);
                        }                   
                      else 
                        {
                          /* Set parent as a exception. */
                          if (HSL_FIB_HWCB_CHECK(hw_prefix_add_exception))
                            {
                              ret = HSL_FIB_HWCB_CALL(hw_prefix_add_exception) (fib_id, rnp);
                              if (ret < 0)
                                {
                                  /* Failed to add exception. */
                                  _hsl_fib_handle_prefix_add_failure (fib_id, rnp);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
  
  HSL_FN_EXIT(0);
}

/*
  Reprocess a particular prefix.
*/
int
hsl_fib_reprocess_prefix (hsl_fib_id_t fib_id, struct hsl_route_node *rnp)
{
  struct hsl_nh_entry_list_node *nhlist;
  struct hsl_prefix_entry *pe;
  struct hsl_nh_entry *nh;

  HSL_FN_ENTER ();

  pe = rnp->info;
  for (nhlist = pe->nhlist; nhlist; nhlist = nhlist->next)
    {
      nh = (struct hsl_nh_entry *) nhlist->entry;
     
      if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID))
        {     
          /* Send it to the hardware. */
          if (HSL_FIB_HWCB_CHECK(hw_prefix_add))
            HSL_FIB_HWCB_CALL(hw_prefix_add) (fib_id, rnp, nh);
        } 
    }

  HSL_FN_EXIT (0);
}

/*
  Activate all prefixes dependant on a NH.
*/
static int
_hsl_fib_activate_prefixes (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  struct hsl_avl_node *node;
  struct hsl_route_node *rnp;

  HSL_FN_ENTER();

  for (node = hsl_avl_top (nh->prefix_tree); node; node = hsl_avl_next (node))
    {
      rnp = node->info;
      if (! rnp)
        continue;
      
      /* Add to hardware. */
      hsl_fib_add_to_hw (fib_id, rnp, nh);
    }
  
  HSL_FN_EXIT(0);
}


/*
  De-activate all prefixes dependant on a NH.
*/
int
hsl_fib_deactivate_prefixes (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  struct hsl_avl_node *node;
  struct hsl_route_node *rnp;
  struct hsl_prefix_entry *pe;
  struct hsl_nh_entry_list_node *nhlist;
  struct hsl_nh_entry *nh2;
  int count; 

  HSL_FN_ENTER();

  if (! nh)
    HSL_FN_EXIT(-1);


  for (node = hsl_avl_top (nh->prefix_tree); node; node = hsl_avl_next (node))
    {
      rnp = node->info;
      if (! rnp)
        continue;

      /* Ignore exceptions. */
      if(HSL_PREFIX_ENTRY_POPULATED_AS_EXCEPTION(rnp))
        continue; 

      /* Delete prefix->nh pair from hardware. */
      hsl_fib_delete_from_hw (fib_id, rnp, nh);

      pe = rnp->info;
      /* Count number of valid next hops left for prefix.*/ 
      count = 0;
      for (nhlist = pe->nhlist; nhlist; nhlist = nhlist->next)
      {
         nh2 = (struct hsl_nh_entry *) nhlist->entry;
         if(CHECK_FLAG(nh2->flags, HSL_NH_ENTRY_VALID))
           count++;
      }
      /* Re add prefix as an exception. */  
      if(!count)
      { 
        if (HSL_FIB_HWCB_CHECK(hw_prefix_add_exception))
          HSL_FIB_HWCB_CALL(hw_prefix_add_exception) (fib_id, rnp);
      }
    }
  
  HSL_FN_EXIT(0);
}



#ifdef HAVE_MPLS
int
hsl_fib_activate_ilm_entries (struct hsl_nh_entry *nh)
{
  struct hsl_mpls_ilm_entry *ilm;

  for (ilm = nh->ilm_list; ilm; ilm = ilm->nh_ilm_next)
    {
      hsl_mpls_ilm_add_to_hw (ilm, nh);
    }

  return 0;
}

#ifdef HAVE_MPLS_VC
int
hsl_fib_activate_vc_ftn_entries (struct hsl_nh_entry *nh)
{
  struct hsl_mpls_vpn_vc *vc;

  for (vc = nh->vpn_vc_list; vc; vc = vc->nh_vc_next)
    {
      hsl_mpls_vpn_vc_ftn_add_to_hw (vc, nh);
    }

  return 0;
}
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_MPLS */


int
hsl_fib_activate_dependents (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh)
{
  _hsl_fib_activate_prefixes (fib_id, nh);

#ifdef HAVE_MPLS
  hsl_fib_activate_ilm_entries (nh);

#ifdef HAVE_MPLS_VC
  hsl_fib_activate_vc_ftn_entries (nh);
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_MPLS */
  return 0;
}

static void
_hsl_fib_activate_related_nexthops (hsl_fib_id_t fib_id, 
                                    struct hsl_route_node *rnh,
                                    struct hsl_nh_entry *nh)
{
  struct hsl_nh_entry *nh_entry;

  for (nh_entry = rnh->info; nh_entry; nh_entry = nh_entry->next)
    {
      if (nh_entry == nh)
        continue;

     if (nh->mac)
       memcpy (nh_entry->mac, nh->mac, HSL_ETHER_ALEN);

     nh_entry->flags |= (HSL_NH_ENTRY_VALID|HSL_NH_ENTRY_DEPENDENT); 
      
      hsl_fib_activate_dependents (fib_id, nh_entry);
    }
}


static int
_hsl_fib_nh_table_add (struct hsl_route_node *rnh, 
                       struct hsl_nh_entry *nh)
{
  struct hsl_nh_entry *nh_head;

  if ((nh_head = rnh->info) == NULL)
    {
      rnh->info = nh;
      nh->next = NULL;
      nh->rn = rnh;
      return 0;
    }

#ifdef HAVE_MPLS
  if (nh->nh_type == HSL_NH_TYPE_MPLS &&
      nh_head->nh_type != HSL_NH_TYPE_MPLS)
    {
      nh->next = nh_head->next;
      nh_head->next = nh;
    }
  else
#endif /* HAVE_MPLS */ 
    {
      nh->next = (struct hsl_nh_entry *)rnh->info;
      rnh->info = nh;
    }

  nh->rn = rnh;

  return 0;
}


static int
_hsl_fib_nh_table_remove (struct hsl_route_node *rnh,
                          struct hsl_nh_entry *nh)
{
  struct hsl_nh_entry *nh_tmp;

  if (rnh->info == nh)
    {
      rnh->info = nh->next;
      nh->next = NULL;
      return 0;
    }
  
  for (nh_tmp = rnh->info; nh_tmp; nh_tmp = nh_tmp->next)
    {
      if (nh == nh_tmp->next)
        {
          nh_tmp->next = nh->next;
          nh->next = NULL;

          break;
        }
    }

  return 0;
}

/* Internal function for nh add. Should be called with HSL_FIB_LOCK. */
static int
_hsl_fib_nh_add (hsl_fib_id_t fib_id, struct hsl_route_node *rnh, struct hsl_if *ifp, char *mac, 
                 u_char flags, u_char nh_type)
{
  struct hsl_nh_entry *nh = NULL;
  struct hsl_nh_entry *nh_tmp = NULL;

  HSL_FN_ENTER();

  if (rnh->info)
    {
      for (nh = (struct hsl_nh_entry *) rnh->info; nh; nh = nh->next)
        {
          /* check if it is the same. */
          if ((nh->nh_type == nh_type)
              && (nh->ifp == ifp) 
              && (! memcmp (nh->mac, mac, HSL_ETHER_ALEN)) 
              && (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID) == CHECK_FLAG (flags, HSL_NH_ENTRY_VALID)))
            {
              /* Update flags anyway. */
              nh->flags = flags;
              /* Just fine. */
              HSL_FIB_UNLOCK;
              HSL_FN_EXIT(0);
            }

#ifdef HAVE_MPLS
          if (nh_tmp == NULL && nh_type != HSL_NH_TYPE_MPLS &&
              nh->nh_type == nh_type)
#endif /* HAVE_MPLS */
            {
              nh_tmp = nh;
            }
        }

      nh = nh_tmp;

      /* Old NH was valid, whereas new one is invalid, treat it as a NH delete. */
      if (nh && CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID) && ! CHECK_FLAG (flags, HSL_NH_ENTRY_VALID))
        {
          if (HSL_FIB_HWCB_CHECK(hw_nh_delete))
            HSL_FIB_HWCB_CALL(hw_nh_delete) (fib_id, nh);  
        }
    }

  if (nh == NULL)
    {
      /* This is a new NH. */
      nh = _hsl_nh_entry_new ();
      if (! nh)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to allocate a nexthop entry.\n");
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
        }
  
      /* Set info. */
      _hsl_fib_nh_table_add (rnh, nh);
    }

  /* Just populate the entries. */
  nh->ifp = ifp;
  if (mac)
    memcpy (nh->mac, mac, HSL_ETHER_ALEN);
  nh->flags = flags;
  nh->nh_type = nh_type;

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID))
    {
      /* something changed for the NH. Update the hardware */
      if (HSL_FIB_HWCB_CHECK(hw_nh_add))
          HSL_FIB_HWCB_CALL(hw_nh_add) (fib_id, nh);
      
      /* If static, add it to OS. */
      if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_STATIC))
        if (p_hsl_fib_table->os_cb && p_hsl_fib_table->os_cb->os_nh_add)
          (*p_hsl_fib_table->os_cb->os_nh_add) (fib_id, nh);
      
      /* Update the liveliness counter for the NH. */
      nh->aliveCounter = 0;
      
      /* Activate all prefixes dependent on this NH. */
      hsl_fib_activate_dependents (fib_id, nh);
      _hsl_fib_activate_related_nexthops (fib_id, rnh, nh);
    } 
  HSL_FN_EXIT(0);
}




/*
  Nexthop add. 
*/
int
hsl_fib_nh_add (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, struct hsl_if *ifp, char *mac, u_char flags,
                u_char nh_type)
{
  struct hsl_route_node *rnh = NULL;
  int ret;

  HSL_FN_ENTER();
  HSL_FIB_LOCK;

  if (prefix->family == AF_INET)
    {
      rnh = hsl_route_node_get (HSL_FIB_TBL (nh, fib_id), prefix);
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      rnh = hsl_route_node_get (HSL_FIB_TBL (nh6, fib_id), prefix);
    }
#endif /* HAVE_IPV6 */

  if (! rnh)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to allocate/find a nexthop entry.\n");
      HSL_FIB_UNLOCK;
      return HSL_FIB_ERR_NOMEM;
    }

  ret = _hsl_fib_nh_add (fib_id, rnh, ifp, mac, flags, nh_type);

  HSL_FIB_UNLOCK;
  HSL_FN_EXIT(ret);
}


static int 
_hsl_fib_nh_delete2 (hsl_fib_id_t fib_id, struct hsl_nh_entry *nh, 
                     int delete_static, HSL_BOOL os_cb)
{
  HSL_FN_ENTER();

  if(!nh)   
     HSL_FN_EXIT(0);

  if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID))
    {

      SET_FLAG(nh->flags, HSL_NH_ENTRY_DEL_IN_PROGRESS);
      UNSET_FLAG(nh->flags, HSL_NH_ENTRY_VALID);

      /* Delete all prefixes pointing to this NH from hardware. */
       hsl_fib_deactivate_prefixes (fib_id, nh);

      /* Delete nh itself. */ 
       if (HSL_FIB_HWCB_CHECK(hw_nh_delete))
         HSL_FIB_HWCB_CALL(hw_nh_delete) (fib_id, nh);
      /* Hardware ageing occured for this nexthop entry. Just delete
         ARP from the OS. */
      if (os_cb && p_hsl_fib_table->os_cb
                && p_hsl_fib_table->os_cb->os_nh_delete)
        (*p_hsl_fib_table->os_cb->os_nh_delete) (fib_id, nh);

      UNSET_FLAG(nh->flags, HSL_NH_ENTRY_DEL_IN_PROGRESS);
    }

  /* If NH is ! static and is ! valid and refcnt is 0, it needs to be deleted. */
  if (HSL_NH_DELETE_ELIGIBLE (nh, delete_static))
    {
      _hsl_fib_nh_table_remove (nh->rn, nh);

      if (nh->rn->info == NULL)
        {
          hsl_route_unlock_node (nh->rn);
        }
          
      /* Free nh. */
      _hsl_nh_entry_free (nh);
    }
   else if (delete_static && FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC))
     UNSET_FLAG (nh->flags, HSL_NH_ENTRY_STATIC);

  return 0;
}



/*
  Internal function for NH delete. 
*/
static int
_hsl_fib_nh_delete (hsl_fib_id_t fib_id, struct hsl_route_node *rnh, u_int32_t ifindex, 
                    int delete_static,
                    int nh_type, HSL_BOOL os_cb)
{
  struct hsl_nh_entry *nh = NULL;
  int ret;

  HSL_FN_ENTER();

#ifdef HAVE_MPLS
  if (nh_type == HSL_NH_TYPE_MPLS && ifindex == 0)
    return -1;
#endif /* HAVE_MPLS */

  for (nh = rnh->info; nh; nh = nh->next)
    {
      if (nh->nh_type == nh_type &&
          (ifindex == 0 || (nh->ifp && ifindex == nh->ifp->ifindex))) 
        {
          break;
        }
    }

  if (nh == NULL)
    return -1;

  ret = _hsl_fib_nh_delete2 (fib_id, nh, delete_static, os_cb);

  HSL_FN_EXIT(ret);
}

/*
  Nexthop delete. 
*/
int
hsl_fib_nh_delete (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, u_int32_t ifindex, 
                   int delete_static, u_char nh_type)
{
  struct hsl_route_node *rnh = NULL;
  char buf[25];

  HSL_FN_ENTER();
  HSL_FIB_LOCK;

  if (prefix->family == AF_INET)
    {
      rnh = hsl_route_node_lookup (HSL_FIB_TBL (nh, fib_id), prefix);
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      rnh = hsl_route_node_lookup (HSL_FIB_TBL (nh6, fib_id), prefix);
    }
#endif /* HAVE_IPV6 */
  
  if (rnh)
    {
      hsl_route_unlock_node (rnh);
      if (rnh->info)
        _hsl_fib_nh_delete (fib_id, rnh, ifindex, delete_static, nh_type
                            , HSL_TRUE);
    }
  else
    {
      hsl_prefix2str (prefix, buf, sizeof(buf));     
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Cant find nexthop entry for a prefix %s\n", buf);
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT(HSL_FIB_ERR_NH_NOT_FOUND);
    }

  HSL_FIB_UNLOCK;
  HSL_FN_EXIT(0);
}

/*
  Process NHs for ageing.
*/
void
hsl_fib_process_nh_ageing (void)
{
  struct hsl_nh_entry *nh, *nh_next;
  struct hsl_route_node *rnh;
  struct hsl_route_table *tbl;
  hsl_fib_id_t i;
  int valid = 1;

  HSL_FN_ENTER();

  if (! hsl_fib_initialized)
    HSL_FN_EXIT(-1);

  HSL_FIB_LOCK;

  HSL_FIB_TBL_TRAVERSE_START (nh, tbl, i)
    {

      for (rnh = hsl_route_top (tbl); rnh; rnh = hsl_route_next (rnh))
        {
          for (nh = rnh->info; nh; nh = nh_next)
            {
              nh_next = nh->next;
               
#ifdef HAVE_MPLS
              /* Skip MPLS NH */
              if (nh->nh_type == HSL_NH_TYPE_MPLS)
                continue;
#endif /* HAVE_MPLS */

              /* Skip static and invalid. */
              if (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_STATIC)
                  || ! CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID) 
                  || FLAG_ISSET (nh->flags, HSL_NH_ENTRY_DEPENDENT)
                  || CHECK_FLAG (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
                continue;

              /* Check if the NH is valid in hardware. */
              if (HSL_FIB_HWCB_CHECK(hw_nh_hit))
                valid = HSL_FIB_HWCB_CALL(hw_nh_hit) (i, nh);

              if (valid)
                {
                  /* Valid. */

                  /* Just reset the aliveCounter. */
                  if (nh->ifp)
                    nh->aliveCounter = 0;
                }
              else
                {
                  /* Invalid. */

                  nh->aliveCounter++;

                  /* If alive counter is 0, the NH has become invalid. */
                  if (nh->aliveCounter 
                      >= HSL_ARP_ALIVE_COUNTER(nh->ifp->u.ip.arpTimeout))
                    _hsl_fib_nh_delete2 (i, nh, 0, HSL_TRUE);
                }   
            }
        }
    }
  HSL_FIB_TBL_TRAVERSE_END (nh, tbl, i)
  
  HSL_FIB_UNLOCK;
  HSL_FN_EXIT(STATUS_OK);

}



struct hsl_nh_entry *
_hsl_fib_nh_get (hsl_ifIndex_t ifindex, hsl_prefix_t *p,
                struct hsl_route_table *nh_table, u_char nh_type,
                enum hsl_ipuc_nexthop_type type)
{
  struct hsl_nh_entry *nh = NULL;
  struct hsl_nh_entry *nh_valid = NULL;
  struct hsl_if *ifp = NULL;
  struct hsl_route_node *rnh;

#ifdef HAVE_MPLS
  if (nh_type == HSL_NH_TYPE_MPLS && ifindex == 0)
    return NULL;
#endif /* HAVE_MPLS */

  rnh = hsl_route_node_get (nh_table, p);
  if (! rnh)
    return NULL;

  for (nh = rnh->info; nh; nh = nh->next)
    {
      if (! nh_valid && FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID))
        nh_valid = nh;
        
      if (nh_type != nh->nh_type)
        continue;

      if ((ifindex == 0) || (!(FLAG_ISSET (nh->flags, HSL_NH_ENTRY_BLACKHOLE)) && (nh->ifp->ifindex == ifindex)))
        {
          break;
        }
    }
  
  if (rnh->info)
    hsl_route_unlock_node (rnh);

  if (nh == NULL)
    {
      /* Allocate new 'invalid' NH. */
      nh = _hsl_nh_entry_new ();
      if (! nh)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error allocating memory for NH\n");
          return NULL;
        }
      if (type != HSL_IPUC_BLACKHOLE)
        {
          ifp = hsl_ifmgr_lookup_by_index (ifindex);
          if (! ifp)
            {
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Outgoing L3 interface(%d) for NH not found\n", ifindex);
              return NULL;
            }

          HSL_IFMGR_IF_REF_DEC (ifp);
        }
      
      /* Set NH values. */
      nh->ifp = ifp;
      nh->refcnt = 0;
      nh->flags = 0;
      nh->nh_type = nh_type;

      if (nh_valid)
        {
          memcpy (nh->mac, nh_valid->mac, HSL_ETHER_ALEN);
          if (type == HSL_IPUC_BLACKHOLE)
            nh->flags |= (HSL_NH_ENTRY_VALID|HSL_NH_ENTRY_BLACKHOLE);
          else
            nh->flags |= (HSL_NH_ENTRY_VALID|HSL_NH_ENTRY_DEPENDENT);
        }
      else if (type == HSL_IPUC_BLACKHOLE)
        {
          nh->flags |= (HSL_NH_ENTRY_VALID|HSL_NH_ENTRY_BLACKHOLE);
          memset (nh->mac, 0, HSL_ETHER_ALEN);
        }
      
      /* Set NH. */
      _hsl_fib_nh_table_add (rnh, nh);
    }

  return nh;
}


struct hsl_nh_entry *
hsl_fib_nh_get (hsl_fib_id_t fib_id, hsl_ifIndex_t ifindex, hsl_prefix_t *p, u_char nh_type)
{
  struct hsl_route_table *nh_table = NULL;
  struct hsl_nh_entry *nh = NULL;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    return NULL;

  if (! HSL_FIB_ID_VALID (fib_id))
    return NULL;

  if (nh_type == HSL_NH_TYPE_IP)
    nh_table = HSL_FIB_TBL (nh, fib_id);
#ifdef HAVE_IPV6
  else if (nh_type == HSL_NH_TYPE_IPV6)
    nh_table = HSL_FIB_TBL (nh6, fib_id);
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS
  else if (nh_type == HSL_NH_TYPE_MPLS)
    nh_table = HSL_FIB_TBL (nh, fib_id);
#endif /* HAVE_MPLS */

  if (nh_table == NULL)
    return NULL;

  HSL_FIB_LOCK;

  nh = _hsl_fib_nh_get (ifindex, p, nh_table, nh_type, HSL_IPUC_UNSPEC);

  HSL_FIB_UNLOCK;

  return nh;
}


struct hsl_nh_entry *
_hsl_fib_nh_lookup (hsl_ifIndex_t ifindex, hsl_prefix_t *p,
                    struct hsl_route_table *nh_table, u_char nh_type)
{
  struct hsl_nh_entry *nh = NULL;
  struct hsl_route_node *rnh;

#ifdef HAVE_MPLS
  if (nh_type == HSL_NH_TYPE_MPLS && ifindex == 0)
    return NULL;
#endif /* HAVE_MPLS */

  rnh = hsl_route_node_lookup (nh_table, p);
  if (! rnh)
    return NULL;

  for (nh = rnh->info; nh; nh = nh->next)
    {
      if (nh_type != nh->nh_type)
        continue;

      if (ifindex == 0 || (nh->ifp && nh->ifp->ifindex == ifindex))
        {
          hsl_route_unlock_node (rnh);
          return nh;
        }
    }
  
  hsl_route_unlock_node (rnh);
  return NULL;
}




struct hsl_nh_entry *
hsl_fib_nh_lookup (hsl_fib_id_t fib_id, hsl_ifIndex_t ifindex, hsl_prefix_t *p, u_char nh_type)
{
  struct hsl_route_table *nh_table = NULL;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    return NULL;

  if (! HSL_FIB_ID_VALID (fib_id))
    return NULL;

  if (nh_type == HSL_NH_TYPE_IP)
    nh_table = HSL_FIB_TBL (nh, fib_id);
#ifdef HAVE_IPV6
  else if (nh_type == HSL_NH_TYPE_IPV6)
    nh_table = HSL_FIB_TBL (nh6, fib_id);
#endif /* HAVE_IPV6 */
#ifdef HAVE_MPLS
  else if (nh_type == HSL_NH_TYPE_MPLS)
    nh_table = HSL_FIB_TBL (nh, fib_id);
#endif /* HAVE_MPLS */

  if (nh_table == NULL)
    return NULL;

  return _hsl_fib_nh_lookup (ifindex, p, nh_table, nh_type);
}



/*
  Add prefix with NH.
  To be called with FIB locked.
*/
static int
_hsl_fib_add_ipuc_prefix_nh (hsl_fib_id_t fib_id,
                             struct hsl_route_node *rnp,
                             enum hsl_ipuc_nexthop_type type,
                             hsl_ifIndex_t ifindex, hsl_prefix_t *p,
                             struct hsl_route_table *nh_table,
                             u_char nh_type)
{
  struct hsl_nh_entry *nh = NULL;
  int ret = 0;

  HSL_FN_ENTER();

  nh = _hsl_fib_nh_get (ifindex, p, nh_table, nh_type, type);
  if (! nh)
    HSL_FN_EXIT (-1);
  
  /* Link NH to prefix table. */
  ret = hsl_fib_link_nh_and_prefix (rnp, nh); 

#ifdef HAVE_MPLS  
  if (nh_type != HSL_NH_TYPE_MPLS)
#endif /* HAVE_MPLS */
    {
      /* Add it to CP stack(slow-path). */
      ret = hsl_fib_add_to_stack (fib_id, rnp, nh);
      if (ret < 0)
        {
          /* If this fails, there is a serious issue. Report error to caller. */
          
          /* Unlink prefix and nh. */
          hsl_fib_unlink_nh_and_prefix (rnp, nh);
          
          HSL_FN_EXIT(-1);
        }
    }
  
  /* Add it to hardware. */
  hsl_fib_add_to_hw(fib_id, rnp, nh);

  HSL_FN_EXIT(0);
}

/*
  Add prefix with NH as a interface.
*/
static int
_hsl_fib_add_ipuc_prefix_if (hsl_fib_id_t fib_id, struct hsl_route_node *rnp,
                             enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER();

  /* NH with interface only. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (type != HSL_IPUC_BLACKHOLE)
    {
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Outgoing L3 interface(%d) "
                   "for NH not found\n", ifindex);
          HSL_FN_EXIT(HSL_FIB_ERR_NH_NOT_FOUND);
        }

      /* Verify that fib id of interface matches. */
      if (ifp->fib_id != fib_id)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "FIB Id(%d) of outgoing L3 "
                   "interface(%d) for NH does not match fib id(%d)\n", ifindex,
                   ifp->fib_id, fib_id);
          HSL_FN_EXIT(HSL_FIB_ERR_MISMATCH_IF_FIB_ID);
        }
    }

  /* Add it to stack. */
  if (p_hsl_fib_table->os_cb && p_hsl_fib_table->os_cb->os_prefix_add_if)
    {
      ret = (*p_hsl_fib_table->os_cb->os_prefix_add_if) (fib_id, rnp, ifp);
      if (ret < 0)
        {
          if (ifp) 
            HSL_IFMGR_IF_REF_DEC (ifp);

          HSL_FN_EXIT(ret);
        }
    }

  if (ifp)
    {
      /* Link them. */
      _hsl_fib_add_ifp_to_prefix (rnp, ifp);

      HSL_IFMGR_IF_REF_DEC (ifp);
    }

  HSL_FN_EXIT(0);
}

/*
  Add connected route(for interface addresses) 
*/ 
int
hsl_fib_add_connected (hsl_prefix_t *prefix, struct hsl_if *ifp)
{
  struct hsl_route_node *rnp;
  struct hsl_route_table *table = NULL;
  hsl_prefix_t tmp_p;

  /* Add this as a exception to the prefix table in hardware. */

  HSL_FN_ENTER();

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  if (prefix->family == AF_INET)
    {
      /* Check if table exists */
      if (!HSL_FIB_TBL_VALID(prefix, ifp->fib_id))
        HSL_FN_EXIT(HSL_FIB_ERR_TBL_NOT_FOUND);

      table = HSL_FIB_TBL(prefix, ifp->fib_id);
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      /* Check if table exists */
      if (!HSL_FIB_TBL_VALID(prefix6, ifp->fib_id))
        HSL_FN_EXIT(HSL_FIB_ERR_TBL_NOT_FOUND);

      table = HSL_FIB_TBL(prefix6, ifp->fib_id);
    }
#endif /* HAVE_IPV6 */

  tmp_p = *prefix;

  hsl_apply_mask (&tmp_p);

  /* Find prefix entry. */ 
  rnp = hsl_fib_prefix_node_get (table, &tmp_p);
  if (! rnp)
    HSL_FN_EXIT (-1);

  /* TODO check for limits in prefix table. */
  if (HSL_FIB_HWCB_CHECK(hw_add_connected_route))
    HSL_FIB_HWCB_CALL(hw_add_connected_route) (ifp->fib_id, rnp, ifp); 

  HSL_FN_EXIT(0);
}

/* Delete all NHs(ARP) learned on given subnet (interface) */
int
hsl_fib_delete_connected_nh(hsl_prefix_t *prefix, struct hsl_if *ifp, 
                             HSL_BOOL os_cb)
{
  struct hsl_nh_entry *nh;
  struct hsl_route_node *rnh, *rnh_start;
  struct hsl_route_table *table=NULL;

  HSL_FN_ENTER();

  /* If global fib not initialized, just return. */
  if (!hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (!HSL_FIB_ID_VALID (ifp->fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  if (prefix->family == AF_INET)
    {
      /* Check if table exists */
      if (!HSL_FIB_TBL_VALID(nh, ifp->fib_id))
        HSL_FN_EXIT(HSL_FIB_ERR_TBL_NOT_FOUND);
      table = HSL_FIB_TBL(nh, ifp->fib_id);
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      /* Check if table exists */
      if (!HSL_FIB_TBL_VALID(nh6, ifp->fib_id))
        HSL_FN_EXIT(HSL_FIB_ERR_TBL_NOT_FOUND);
      table = HSL_FIB_TBL(nh6, ifp->fib_id);
    }
#endif /* HAVE_IPV6 */

  HSL_FIB_LOCK;
 
  rnh_start = hsl_route_node_get(table, prefix);
  if(rnh_start == NULL)
    {
      HSL_FIB_UNLOCK;
      return HSL_FIB_ERR_NOMEM;
    }

  for (rnh = hsl_route_lock_node(rnh_start); rnh != NULL; 
       rnh = hsl_route_next_until(rnh, rnh_start))
    {
      for (nh = rnh->info; nh; nh = nh->next)
        {
          if (nh->ifp != ifp)
            continue;

	  if ((prefix->family == AF_INET 
               && nh->nh_type == HSL_NH_TYPE_IP)
#ifdef HAVE_IPV6
            || (prefix->family == AF_INET6 
                && nh->nh_type == HSL_NH_TYPE_IPV6)
#endif /* HAVE_IPV6 */
             )
            break;
        }
          
      if ((nh == NULL) || 
          !FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID) ||
          FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC))
        continue;
     
      _hsl_fib_nh_delete (ifp->fib_id, rnh, ifp->ifindex, 0,
                           nh->nh_type, os_cb);       
    }
 
  hsl_route_unlock_node(rnh_start); 
  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(0);
}

/* 
   Delete connected route(for interface addresses)
*/
int
hsl_fib_delete_connected (hsl_prefix_t *prefix, struct hsl_if *ifp, 
                          HSL_BOOL os_cb)
{
  struct hsl_route_node *rnp;
  struct hsl_route_table *table = NULL;
  hsl_prefix_t tmp_p;

  /* Delete this as a exceptoin from the prefix table in hardware. */
  HSL_FN_ENTER();

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  if (prefix->family == AF_INET)
    {
      /* Check if table exists */
      if (!HSL_FIB_TBL_VALID(prefix, ifp->fib_id))
        HSL_FN_EXIT(HSL_FIB_ERR_TBL_NOT_FOUND);

      table = HSL_FIB_TBL(prefix, ifp->fib_id);
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      /* Check if table exists */
      if (!HSL_FIB_TBL_VALID(prefix6, ifp->fib_id))
        HSL_FN_EXIT(HSL_FIB_ERR_TBL_NOT_FOUND);

      table = HSL_FIB_TBL(prefix6, ifp->fib_id);
    }
#endif /* HAVE_IPV6 */

  /* Delete all arp/nh learned on this connected subnet */
  hsl_fib_delete_connected_nh(prefix, ifp, os_cb);

  tmp_p = *prefix;

  hsl_apply_mask (&tmp_p);

  /* Find prefix entry. */
  rnp = hsl_fib_prefix_node_lookup (table, &tmp_p);
  if (! rnp)
    HSL_FN_EXIT (-1);

  /* TODO check for limits in prefix table. */
  if (HSL_FIB_HWCB_CHECK(hw_delete_connected_route))
    HSL_FIB_HWCB_CALL(hw_delete_connected_route) (ifp->fib_id, rnp, ifp); 

  HSL_FN_EXIT(0);
}


struct hsl_route_node *
hsl_fib_prefix_node_get (struct hsl_route_table *prefix_table, hsl_prefix_t *prefix)
{
  struct hsl_route_node *rnp;
  struct hsl_prefix_entry *pe;

  HSL_FIB_LOCK;

  rnp = hsl_route_node_get (prefix_table, prefix);
  if (! rnp)
    {
      HSL_FIB_UNLOCK;
      return NULL;
    }

  if (rnp->info)
    {
      /* Prefix already exists. Add the new nexthop. */
      pe = rnp->info;
      hsl_route_unlock_node (rnp);
    }
  else
    {
      /* dummy node or new node. */
      pe = _hsl_prefix_entry_new ();
      if (! pe)
        {
          HSL_FIB_UNLOCK;
          return NULL;
        }
      
      /* Set rn info. */
      rnp->info = pe;
    }

  HSL_FIB_UNLOCK;

  return rnp;
}

struct hsl_route_node *
hsl_fib_prefix_node_lookup (struct hsl_route_table *prefix_table, hsl_prefix_t *prefix)
{
  struct hsl_route_node *rnp;

  HSL_FIB_LOCK;

  rnp = hsl_route_node_lookup (prefix_table, prefix);
  if (! rnp)
    {
      HSL_FIB_UNLOCK;
      return NULL;
    }

  hsl_route_unlock_node (rnp);

  HSL_FIB_UNLOCK;
  return rnp;
}

/*
  Add prefix.
*/
int
_hsl_fib_add_ipuc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex,
                          hsl_prefix_t *addr, 
                          struct hsl_route_table *prefix_table, 
                          struct hsl_route_table *nh_table, 
                          u_char nh_type)
{
  struct hsl_route_node *rnp;
  int ret;

  HSL_FN_ENTER();

  /* Find prefix entry. */
  rnp = hsl_fib_prefix_node_get (prefix_table, prefix);
  if (! rnp)
    HSL_FN_EXIT (-1);

  HSL_FIB_LOCK;

  if (addr || (type == HSL_IPUC_BLACKHOLE))
    {
      ret = _hsl_fib_add_ipuc_prefix_nh (fib_id, rnp, type, ifindex, addr, nh_table, nh_type);
      if (ret < 0)
        {
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT(ret);
        }
    }
  else
    {
      ret = _hsl_fib_add_ipuc_prefix_if (fib_id, rnp, type, ifindex);
      if (ret < 0)
        {
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT(ret);
        }
    }
  
  HSL_FIB_UNLOCK;
  HSL_FN_EXIT (0);
}


/* 
   Add prefix. 
*/
int
hsl_fib_add_ipv4uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex,
                           hsl_ipv4Address_t *addr)
{
  hsl_prefix_t p;
  int ret;

  HSL_FN_ENTER();

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (prefix, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  if (addr)
    {
      p.family = AF_INET;
      p.prefixlen = 32;
      p.u.prefix4 = *addr;
    }
  else if (type == HSL_IPUC_BLACKHOLE)
    {
       p.family = AF_INET;
       p.prefixlen = 0;
       memset (&p.u.prefix4, 0, sizeof (hsl_ipv4Address_t));
    }

  ret = _hsl_fib_add_ipuc_prefix (fib_id, prefix, type, ifindex, (addr ||
                                  (type == HSL_IPUC_BLACKHOLE)) ? &p : NULL,
                                  HSL_FIB_TBL (prefix, fib_id), HSL_FIB_TBL (nh, fib_id), HSL_NH_TYPE_IP);

  HSL_FN_EXIT(ret);
}

#ifdef HAVE_IPV6
/*
  Add IPV6 prefix.
*/
int
hsl_fib_add_ipv6uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex,
                           hsl_ipv6Address_t *addr)
{
  hsl_prefix_t p;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (prefix6, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  if (addr)
    {
      p.family = AF_INET6;
      p.prefixlen = 128;
      memcpy (&p.u.prefix6, addr, sizeof (hsl_ipv6Address_t));
    }
  else if (type == HSL_IPUC_BLACKHOLE)
    {
       p.family = AF_INET6;
       p.prefixlen = 0;
       memset (&p.u.prefix6, 0, sizeof (hsl_ipv6Address_t));
    }
                 
    return _hsl_fib_add_ipuc_prefix (fib_id, prefix, type, ifindex, (addr ||
                                   (type == HSL_IPUC_BLACKHOLE))? &p : NULL,
                                   HSL_FIB_TBL (prefix6, fib_id), HSL_FIB_TBL (nh6, fib_id), HSL_NH_TYPE_IPV6);
}
#endif /* HAVE_IPV6 */

#ifdef HAVE_MPLS
/* 
   Add ftn entry 
*/
int
hsl_fib_add_mpls_ftn (hsl_prefix_t *prefix, 
                      struct hsl_nh_entry *nh)
{
  struct hsl_route_node *rnp;
  hsl_fib_id_t fib_id = HSL_DEFAULT_FIB;

  HSL_FN_ENTER();

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (prefix, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  /* Find prefix entry. */
  rnp = hsl_fib_prefix_node_get (HSL_FIB_TBL (prefix, fib_id), prefix);
  if (! rnp)
    HSL_FN_EXIT (-1);

  HSL_FIB_LOCK;

  /* Link NH to prefix table. */
  hsl_fib_link_nh_and_prefix (rnp, nh);

  /* Add it to hardware. */
  hsl_fib_add_to_hw(fib_id, rnp, nh);

  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(0);
}

/* 
   Delete ftn entry 
*/
int
hsl_fib_delete_mpls_ftn (hsl_prefix_t *prefix, 
                         struct hsl_nh_entry *nh)
{
  struct hsl_route_node *rnp;
  struct hsl_prefix_entry *pe;
  hsl_fib_id_t fib_id = HSL_DEFAULT_FIB;

  HSL_FN_ENTER();

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (prefix, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  HSL_FIB_LOCK;

  /* Find prefix entry. */
  rnp = hsl_route_node_lookup (HSL_FIB_TBL (prefix, fib_id), prefix);
  if (! rnp)
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT (-1);
    }

  hsl_route_unlock_node (rnp);
  hsl_fib_unlink_nh_and_prefix (rnp, nh);

  hsl_fib_delete_from_hw(fib_id, rnp, nh);

  pe = rnp->info;

  /* Delete prefix if prefix is eligible for deletion. */
  if (HSL_PREFIX_DELETE_ELIGIBLE (pe))
    {
      rnp->info = NULL;
      hsl_route_unlock_node (rnp);
      
      
      /* Free prefix. */
      _hsl_prefix_entry_free (pe);
    }

  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(0);
}

void
hsl_fib_nh_release (struct hsl_nh_entry *nh)
{
  HSL_FIB_LOCK;
  _hsl_fib_nh_table_remove (nh->rn, nh);
  
  if (nh->rn->info == NULL)
    {
      hsl_route_unlock_node (nh->rn);
    }
  
  /* Free nh. */
  _hsl_nh_entry_free (nh);

  HSL_FIB_UNLOCK;
}
#endif /* HAVE_MPLS */



/*
  Delete with NH.
  To be called with FIB locked.
*/
static int
_hsl_fib_delete_ipuc_prefix_nh (hsl_fib_id_t fib_id, struct hsl_route_node *rnp,
                                enum hsl_ipuc_nexthop_type type, hsl_ifIndex_t ifindex, 
                                hsl_prefix_t *p,
                                struct hsl_route_table *nh_table, u_char nh_type)
{
  struct hsl_nh_entry *nh = NULL;
  struct hsl_route_node *rnh;
  int ret;

  HSL_FN_ENTER();

#ifdef HAVE_MPLS
  if (nh_type == HSL_NH_TYPE_MPLS && ifindex == 0)
    return -1;
#endif /* HAVE_MPLS */

  rnh = hsl_route_node_lookup (nh_table, p);
  if (rnh)
    {
      /* Unlock node. */
      hsl_route_unlock_node (rnh);
      
      if (rnh->info)
        {
          for (nh = rnh->info; nh; nh = nh->next)
            {
              if (nh_type != nh->nh_type)
                continue;

              if (ifindex == 0 ||
                  ((! FLAG_ISSET (nh->flags, HSL_NH_ENTRY_BLACKHOLE))&&
                  (nh->ifp->ifindex == ifindex)))
                {
                  break;
                }
            }
        }

      if (nh)
        {
          /* Unlink NH to prefix table. */
          hsl_fib_unlink_nh_and_prefix (rnp, nh);  

          /* Delete it from hardware. */
          hsl_fib_delete_from_hw (fib_id, rnp, nh);
 

#ifdef HAVE_MPLS
          if (nh_type != HSL_NH_TYPE_MPLS)
#endif /* HAVE_MPLS */
            {
              /* Delete it from stack. */
              ret = hsl_fib_delete_from_stack (fib_id, rnp, nh);
              if (ret < 0)
                {
                  /* Try again. */
                  hsl_fib_delete_from_stack (fib_id, rnp, nh);
                }
            }

          /* If NH is ! static and is ! valid and refcnt is 0, it needs to be deleted. */
          if (HSL_NH_DELETE_ELIGIBLE (nh, 0))
            {
              _hsl_fib_nh_table_remove (rnh, nh);

              if (rnh->info == NULL)
                {
                  hsl_route_unlock_node (rnh);
                }

              /* Free nh. */
              _hsl_nh_entry_free (nh);

              HSL_FN_EXIT (0);
            }

          HSL_FN_EXIT (0);
        }
      else
        {
#if 0
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Nexthop %x for prefix not found\n", *addr);
#endif
          HSL_FN_EXIT (HSL_FIB_ERR_NH_NOT_FOUND);
        }
    }
    /* In nsm_rib_process_update() all nh FIB flags are set and therefor NSM sends nh to be deleted when HSL does not have the nh associated
     * with the prefix. Returning error in this case results in other nexthops (for ECMP routes) not being deleted as the loop breaks.
     * Fix: Ignore not-found nexthop error.
     */
#if 0
  else
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Nexthop 0x%04x for prefix 0x%04x/0x%x not found\n", p->u.prefix4, rnp->p.u.prefix4, rnp->p.prefixlen);
      HSL_FN_EXIT (HSL_FIB_ERR_NH_NOT_FOUND);
    }
#endif

  HSL_FN_EXIT (0);
}

/*
  Delete prefix with NH as a interface.
*/
static int
_hsl_fib_delete_ipuc_prefix_if (hsl_fib_id_t fib_id,
                                struct hsl_route_node *rnp,
                                enum hsl_ipuc_nexthop_type type,
                                hsl_ifIndex_t ifindex, u_char nh_type)
{
  struct hsl_if *ifp;
  struct hsl_route_node *rnh;
  int delete_static = 0;
  struct hsl_prefix_entry *pe;
  int ret = -1;

  HSL_FN_ENTER();


  /* NH with interface only. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  /* ifp Null Check ignored to delete Blackhole routes */  

  /* Verify that fib id of interface matches. */
  if (ifp && ifp->fib_id != fib_id)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "FIB Id(%d) of outgoing L3 " 
               "interface(%d) for NH does not match fib id(%d)\n", ifindex,
               ifp->fib_id, fib_id);
      HSL_FN_EXIT(HSL_FIB_ERR_MISMATCH_IF_FIB_ID);
    }

  pe = rnp->info;
  if (! pe)
    {
      if (ifp)
        HSL_IFMGR_IF_REF_DEC (ifp);

      HSL_FN_EXIT (HSL_FIB_ERR_PREFIX_NOT_FOUND);
    }

  /* Add it to stack. */
  if (p_hsl_fib_table->os_cb && p_hsl_fib_table->os_cb->os_prefix_delete_if)
    {
      ret = (*p_hsl_fib_table->os_cb->os_prefix_delete_if) (fib_id, rnp, ifp);
      if (ret < 0)
        {
          if (ifp)
            HSL_IFMGR_IF_REF_DEC (ifp);

          HSL_FN_EXIT (ret);
        } 
    }

  if (! ifp)
    HSL_FN_EXIT (ret); 

  /* Delete the ifp from the pe iflist. */
  _hsl_fib_delete_ifp_from_prefix (rnp, ifp);


  if (rnp->p.family == AF_INET)
    rnh = hsl_route_top (HSL_FIB_TBL (nh, fib_id));
#ifdef HAVE_IPV6
  else if (rnp->p.family == AF_INET6)
    rnh = hsl_route_top (HSL_FIB_TBL (nh6, fib_id));
#endif /* HAVE_IPV6 */
  else
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (-1);
    }

  /* Delete all NHs who match the prefix. */
  for (; rnh; rnh = hsl_route_next (rnh))
    {
      if (hsl_prefix_match (&rnp->p, &rnh->p))
        {
          /* Process delete for this NH. */
          hsl_fib_nh_delete (fib_id, &rnh->p, ifindex, delete_static, nh_type);   
        }      
    }

  HSL_IFMGR_IF_REF_DEC (ifp);
  
  HSL_FN_EXIT(0); 
}


/*
  Delete prefix.
*/
int
_hsl_fib_delete_ipuc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, 
                             hsl_ifIndex_t ifindex, hsl_prefix_t *addr, 
                             struct hsl_route_table *prefix_table,
                             struct hsl_route_table *nh_table, u_char nh_type)
{
  struct hsl_route_node *rn;
  struct hsl_prefix_entry *pe;
  char buf[256];
  int ret;

  HSL_FN_ENTER();

  HSL_FIB_LOCK;

  /* Find prefix entry. */
  rn = hsl_route_node_lookup (prefix_table, prefix);
  if (rn)
    {
      /* Unlock node. */
      hsl_route_unlock_node (rn);

      if (rn->info)
        {
          /* Prefix already exists. Delete the new nexthop. */
          pe = rn->info;
        }
      else
        {
          hsl_prefix2str (prefix, buf, sizeof(buf));     
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Prefix %s not found in FIB table\n", buf);
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT (HSL_FIB_ERR_PREFIX_NOT_FOUND); 
        }

      if (addr || (type == HSL_IPUC_BLACKHOLE)) 
        {
          ret = _hsl_fib_delete_ipuc_prefix_nh (fib_id, rn, type, ifindex, addr, nh_table, nh_type);
          if (ret < 0)
            {
              HSL_FIB_UNLOCK;
              HSL_FN_EXIT (ret);
            }
        }
      else
        {
          ret = _hsl_fib_delete_ipuc_prefix_if (fib_id, rn, type, ifindex, nh_type);
          if (ret < 0)
            {
              HSL_FIB_UNLOCK;
              HSL_FN_EXIT (ret);
            }
        }
      
      /* Delete prefix if prefix is eligible for deletion. */
      if (HSL_PREFIX_DELETE_ELIGIBLE (pe))
        {
          rn->info = NULL;
          hsl_route_unlock_node (rn);
          
          
          /* Free prefix. */
          _hsl_prefix_entry_free (pe);    
        }         
    }
  
  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(0); 
}




/*
  Delete prefix.
*/
int
hsl_fib_delete_ipv4uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type, 
                              hsl_ifIndex_t ifindex, hsl_ipv4Address_t *addr)
{
  hsl_prefix_t p;
  int ret;

  HSL_FN_ENTER();                                                                                                                           
  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (prefix, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  if (addr)
    {
      p.family = AF_INET;
      p.prefixlen = 32;
      p.u.prefix4 = *addr;
    }
  else if (type == HSL_IPUC_BLACKHOLE)
    {
      p.family = AF_INET;
      p.prefixlen = 0;
      memset (&p.u.prefix4, 0, sizeof (hsl_ipv4Address_t));
    }

  ret = _hsl_fib_delete_ipuc_prefix (fib_id, prefix, type, ifindex, (addr ||
                                     (type == HSL_IPUC_BLACKHOLE))? &p : NULL,
                                     HSL_FIB_TBL (prefix, fib_id),
                                     HSL_FIB_TBL (nh, fib_id),
                                     HSL_NH_TYPE_IP);

  HSL_FN_EXIT(ret);
}

#ifdef HAVE_IPV6
/*
  Delete IPV6 prefix.
*/
int
hsl_fib_delete_ipv6uc_prefix (hsl_fib_id_t fib_id, hsl_prefix_t *prefix, enum hsl_ipuc_nexthop_type type,
                              hsl_ifIndex_t ifindex, hsl_ipv6Address_t *addr)
{
  hsl_prefix_t p;

  HSL_FN_ENTER();                                                                                                                              
  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (prefix6, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  if (addr)
    {
      p.family = AF_INET6;
      p.prefixlen = 128;
      memcpy (&p.u.prefix6, addr, sizeof (hsl_ipv6Address_t));
    }
  else if (type == HSL_IPUC_BLACKHOLE)
    {
      p.family = AF_INET6;
      p.prefixlen = 0;
      memset (&p.u.prefix6, 0, sizeof (hsl_ipv6Address_t));
    }

  return _hsl_fib_delete_ipuc_prefix (fib_id, prefix, type, ifindex,
                       (addr || type == HSL_IPUC_BLACKHOLE) ? &p : NULL,
                                      HSL_FIB_TBL (prefix6, fib_id),
                                      HSL_FIB_TBL (nh6, fib_id), HSL_NH_TYPE_IPV6);
}
#endif /* HAVE_IPV6 */

/*
  Handle the ARP frame. Basically add the nexthop to the hardware if required.
*/
int
hsl_fib_handle_arp (struct hsl_if *ifp, struct hsl_eth_header *eth, struct hsl_arp *arp)
{
  u_char flags = 0;
  hsl_prefix_t p;
  u_int32_t srcip;
  struct hsl_route_node *rnh;
  struct hsl_nh_entry *nh;
  int ret;
  int match_found;

  HSL_FN_ENTER();

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh, ifp->fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  if (arp->pro != HSL_ETHER_TYPE_IP)
          HSL_FN_EXIT(0);

  if (HSL_MAC_IS_INVALID (arp->eth_src))
          HSL_FN_EXIT(0);

  if ((arp->op == HSL_ARP_OP_REQUEST && 
       ! HSL_MAC_IS_ALL_ONES (eth->dmac) &&
       memcmp (eth->dmac, ifp->u.l2_ethernet.mac, HSL_ETHER_ALEN) != 0) ||
      (arp->op == HSL_ARP_OP_REPLY &&
       memcmp (eth->dmac, ifp->u.l2_ethernet.mac, HSL_ETHER_ALEN) != 0)) 
          HSL_FN_EXIT(0);

  srcip = GET_32ON16(arp->ip_src);

  /* Set as a valid NH. */
  flags |= HSL_NH_ENTRY_VALID;

  /* Set NH address. */
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_BITLEN;
  p.u.prefix4 = srcip;

  match_found = 0;

  /* validate src ip */
  if (ifp->type == HSL_IF_TYPE_IP)
    {
      hsl_prefix_list_t *plist;
      hsl_prefix_t pfx;
      hsl_ipv4Address_t mask;
  
      for (plist = ifp->u.ip.ipv4.ucAddr; plist; plist = plist->next)
        {
          pfx = plist->prefix;
          if (hsl_prefix_match (&pfx, &p))
            {
              hsl_masklen2ip (pfx.prefixlen, &mask);
              if (((p.u.prefix4 & mask) != srcip) &&
                  ((p.u.prefix4 | (~mask)) != srcip))
                {
                  match_found = 1;
                  break;
                }
            }
        }
    }

  if (match_found == 0)
          HSL_FN_EXIT(0);

  /* Check if a static entry for this ARP exists. If yes, ignore
     this ARP. */
  HSL_FIB_LOCK;
  rnh = hsl_route_node_get (HSL_FIB_TBL (nh, ifp->fib_id), &p);
  if (! rnh)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to allocate/find a nexthop entry.\n");
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
    }

  if (rnh->info)
    {
      /* This is a updation. */
      hsl_route_unlock_node (rnh);

      for (nh = rnh->info; nh; nh = nh->next)
        {
          if (nh->nh_type == HSL_NH_TYPE_IP &&
              FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC))
            {
              HSL_FIB_UNLOCK;
              HSL_FN_EXIT(0);
            }
        }
    }


  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "Adding nexthop %x (%02x%02x.%02x%02x.%02x%02x) on interface %s\n",
           srcip, arp->eth_src[0], arp->eth_src[1], arp->eth_src[2], arp->eth_src[3], arp->eth_src[4], arp->eth_src[5],
           ifp->name);

  ret = _hsl_fib_nh_add (ifp->fib_id, rnh, ifp, (char*)arp->eth_src, flags, HSL_NH_TYPE_IP);
  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(ret);
}

/*
  Handle exception packet.
*/
int
hsl_fib_handle_exception (hsl_fib_id_t fib_id, unsigned char *hdr, int ether_type)
{
  struct hsl_nh_entry_list_node *nhlist;
  struct hsl_ip      *l3_hdr_ver4;
#ifdef HAVE_IPV6
  struct hsl_ip6_hdr *l3_hdr_ver6;
#endif /* HAVE_IPV6 */  
  struct hsl_route_node   *rnp;
  struct hsl_prefix_entry *pe;
  hsl_prefix_t p;

  HSL_FN_ENTER();
  HSL_FIB_LOCK;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);
    }

  if (! HSL_FIB_ID_VALID (fib_id))
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);
    }

  /* Set the prefix. */
  switch(ether_type)
    {
    case HSL_ETHER_TYPE_IP:
      l3_hdr_ver4 = (struct hsl_ip *)hdr;  
      /* XXX Temp until mcast fib implemented. */
      if(IPV4_ADDR_MULTICAST(l3_hdr_ver4->ip_dst))
        { 
          HSL_FIB_UNLOCK; 
          HSL_FN_EXIT(0);
        }
      p.family    = AF_INET;
      p.prefixlen = IPV4_MAX_BITLEN;
      p.u.prefix4 = l3_hdr_ver4->ip_dst;

      /* Check if table exists */
      if (! HSL_FIB_TBL_VALID (prefix, fib_id))
        {
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);
        }

      rnp = hsl_route_node_match (HSL_FIB_TBL (prefix, fib_id), &p);
      break;
#ifdef HAVE_IPV6
    case HSL_ETHER_TYPE_IPV6:
      l3_hdr_ver6 = (struct hsl_ip6_hdr *)hdr;  
      /* XXX Temp until mcast fib implemented. */
      if(IPV6_IS_ADDR_MULTICAST(&l3_hdr_ver6->ip_dst))
        { 
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT(0);
        }
      p.family    = AF_INET6;
      p.prefixlen = IPV6_MAX_BITLEN;
      p.u.prefix6 = l3_hdr_ver6->ip_dst;

      /* Check if table exists */
      if (! HSL_FIB_TBL_VALID (prefix6, fib_id))
        {
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);
        }

      rnp = hsl_route_node_match (HSL_FIB_TBL (prefix6, fib_id), &p);
      break;
#endif /* HAVE_IPV6 */
    default:
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT(HSL_FIB_ERR_PREFIX_NOT_FOUND);
    }

  if (! rnp || ! rnp->info)
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT(HSL_FIB_ERR_PREFIX_NOT_FOUND);
    }

  hsl_route_unlock_node (rnp);

  /* For ECMP routes, calling hsl_fib_add_to_hw()
   * leads to duplicate ECMP route entries in BCM
   * "defip" table
   * Fix: Ignore ECMP routes
   */
  if (rnp->is_ecmp)
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT(0);
    }
 
  /* Try adding all the nexthops for this prefix. */
  pe = rnp->info;
  nhlist = pe->nhlist;
  while (nhlist)
    { 
      hsl_fib_add_to_hw (fib_id, rnp, nhlist->entry);
      nhlist = nhlist->next;
    }

  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(0);
}

/* Handle L2 station movement */
/* This should be called with FIB locked */
static int
_hsl_fib_nh_handle_l2_movement (struct hsl_route_table *nh_table, 
    hsl_mac_address_t src_mac, struct hsl_if *ifp)
{
  struct hsl_route_node *rnh;
  struct hsl_nh_entry *nh;

  HSL_FN_ENTER ();

  for (rnh = hsl_route_top (nh_table); rnh; rnh = hsl_route_next (rnh))
    {
      if (! rnh->info)
        continue;

      nh = rnh->info;

      if (! (CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID) &&
            CHECK_FLAG (nh->ext_flags, HSL_NH_ENTRY_EFLAG_IN_HW)))
        continue;

      if (nh->ifp != ifp)
        continue;

      if (! HSL_MAC_IS_SAME (src_mac, nh->mac))
        continue;

      /*  Update the hardware */
      if (HSL_FIB_HWCB_CHECK(hw_nh_add))
          HSL_FIB_HWCB_CALL(hw_nh_add) (ifp->fib_id, nh);
    }
  HSL_FN_EXIT (0);
}

/* Handle L2 station movement */
int
hsl_fib_handle_l2_movement (hsl_mac_address_t src_mac, struct hsl_if *ifp)
{
  int ret = 0;

  HSL_FN_ENTER();

  /* We need to do this only for VLAN interface */
  if (memcmp (ifp->name, "vlan", 4))
    HSL_FN_EXIT (0);

  HSL_FIB_LOCK;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);
    }

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    {
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);
    }

  if (HSL_FIB_TBL_VALID (nh, ifp->fib_id))
    ret = _hsl_fib_nh_handle_l2_movement (HSL_FIB_TBL (nh, ifp->fib_id), 
        src_mac, ifp);

#ifdef HAVE_IPV6
  if (HSL_FIB_TBL_VALID (nh6, ifp->fib_id))
    ret = _hsl_fib_nh_handle_l2_movement (HSL_FIB_TBL (nh6, ifp->fib_id), 
        src_mac, ifp);
#endif /* HAVE_IPV6 */

  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(ret);
}

static void
_hsl_fib_tbl_l2_port_state_change (hsl_fib_id_t fib_id,
                                   struct hsl_route_table *tbl,
                                   struct hsl_if *l2_ifp)
{
  struct hsl_route_node *rnh;
  struct hsl_nh_entry *nh, *nh_next;
  char buf[256];

  for (rnh = hsl_route_top (tbl); rnh; rnh = hsl_route_next (rnh))
    {
      for (nh = rnh->info; nh; nh = nh_next)
        {
          nh_next = nh->next;

          if (nh->ifp == NULL)
            continue;

          if (memcmp (nh->ifp->name, "vlan", 4))
            continue;

          if (nh->l2_ifp == NULL || nh->l2_ifp != l2_ifp)
            continue;

#ifdef HAVE_MPLS
          /* Skip MPLS NH */
          if (nh->nh_type == HSL_NH_TYPE_MPLS)
            continue;
#endif /* HAVE_MPLS */

          /* Skip static and invalid. */
          if (! CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID)
              || FLAG_ISSET (nh->flags, HSL_NH_ENTRY_DEPENDENT)
              || FLAG_ISSET (nh->flags, HSL_NH_ENTRY_BLACKHOLE)
              || ! CHECK_FLAG (nh->ext_flags, HSL_NH_ENTRY_EFLAG_IN_HW))
            continue;

          hsl_prefix2str (&nh->rn->p, buf, sizeof(buf));

          _hsl_fib_nh_delete2 (fib_id, nh, 0, HSL_TRUE);
        }
    }
}

void
hsl_fib_handle_l2_port_state_change (struct hsl_if *l2_ifp)
{
  struct hsl_route_table *tbl;
  hsl_fib_id_t i;

  if (! hsl_fib_initialized)
    HSL_FN_EXIT(-1);

  HSL_FIB_LOCK;

  HSL_FIB_TBL_TRAVERSE_START (nh, tbl, i)
    _hsl_fib_tbl_l2_port_state_change (i, tbl, l2_ifp);

#ifdef HAVE_IPV6
  HSL_FIB_TBL_TRAVERSE_START (nh6, tbl, i)
    _hsl_fib_tbl_l2_port_state_change (i, tbl, l2_ifp);
#endif /* HAVE_IPV6 */

  HSL_FIB_UNLOCK;
}

int
hsl_fib_arp_entry_add (hsl_ipv4Address_t *ip_addr,
                       unsigned int ifindex, 
                       unsigned char *mac_addr,
                       unsigned char flags)
{
  struct hsl_if *ifp;
  int ret = 0;
  hsl_prefix_t pfx;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  pfx.family = AF_INET;
  pfx.prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&pfx.u.prefix4, ip_addr);

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return HSL_IFMGR_ERR_IF_NOT_FOUND;

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);
    }

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh, ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);
    }

  ret = hsl_fib_nh_add (ifp->fib_id, &pfx, ifp, (char*)mac_addr,
                        flags, HSL_NH_TYPE_IP);
  HSL_IFMGR_IF_REF_DEC (ifp);

  return ret;
}


int
hsl_fib_arp_entry_del (hsl_ipv4Address_t *ip_addr, unsigned int ifindex)
{
  int ret = 0;
  struct hsl_nh_entry *nh;
  struct hsl_route_node *rn;
  struct hsl_if *ifp;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return HSL_IFMGR_ERR_IF_NOT_FOUND;

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);
    }

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh, ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);
    }

  HSL_FIB_LOCK;

  rn = hsl_route_node_lookup_ipv4 (HSL_FIB_TBL (nh, ifp->fib_id), ip_addr);
  if (rn)
    {
      hsl_route_unlock_node (rn);
      nh = rn->info;
      ret = _hsl_fib_nh_delete (ifp->fib_id, rn, 0,
                                FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC), 
                                HSL_NH_TYPE_IP, HSL_TRUE);
    }
  else
    ret = HSL_FIB_ERR_NH_NOT_FOUND;

  HSL_FIB_UNLOCK;

  HSL_IFMGR_IF_REF_DEC (ifp);
  return ret;
}



int
hsl_fib_arp_del_all (hsl_fib_id_t fib_id, unsigned char clr_flag)
{
  struct hsl_route_node *rnh;
  struct hsl_nh_entry *nh;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  HSL_FIB_LOCK;

  for (rnh = hsl_route_top (HSL_FIB_TBL (nh, fib_id)); rnh; 
       rnh = hsl_route_next (rnh))
    {
      if (! rnh->info)
        continue;

      nh = rnh->info;

      if ((! FLAG_ISSET (clr_flag,  HSL_ARP_FLAG_STATIC) && 
          FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC)) ||
          (! FLAG_ISSET (clr_flag, HSL_ARP_FLAG_DYNAMIC) &&
           ! FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC)) ||
           FLAG_ISSET (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
        continue;

      _hsl_fib_nh_delete (fib_id, rnh, 0,
                          FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC),
                          HSL_NH_TYPE_IP, HSL_TRUE);        
    }
  
  HSL_FIB_UNLOCK;
  return 0;
}

/* Get bundle of NHs(ARP). */
int
hsl_fib_nh_get_bundle (hsl_fib_id_t fib_id, hsl_ipv4Address_t *ip_addr, int count,
                       struct hal_arp_cache_entry *cache)
{
  struct hsl_nh_entry *nh;
  struct hsl_route_node *rnh, *rn_head;
  int i = 0;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  if (count <= 0)
    return 0;

  HSL_FIB_LOCK;
  
  if (ip_addr)
    {
      rn_head = hsl_route_node_get_ipv4 (HSL_FIB_TBL (nh, fib_id), ip_addr);
      if (rn_head)
        rn_head = hsl_route_next (rn_head);
    }
  else
    rn_head = hsl_route_top (HSL_FIB_TBL (nh, fib_id));
  
  if (! rn_head)
    {
      HSL_FIB_UNLOCK;
      return 0;
    }

  for (rnh = hsl_route_lock_node (rn_head); rnh; rnh = hsl_route_next (rnh))
    {
      for (nh = rnh->info; nh; nh = nh->next)
        {
          if (nh->nh_type == HSL_NH_TYPE_IP)
            break;
        }
          
      if (! nh || ! FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID))
        continue;

      if (nh->ifp == NULL)
        continue;
      
      IPV4_ADDR_COPY (&cache[i].ip_addr, &rnh->p.u.prefix4); 
      memcpy (&cache[i].mac_addr, nh->mac, HSL_ETHER_ALEN);
      cache[i].ifindex = nh->ifp->ifindex;
      if (FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC))
        cache[i].static_flag = 1;
      else
        cache[i].static_flag = 0;
      
      if (++i == count)
        {
          hsl_route_unlock_node (rnh);
          break;
        }
    }
  
  hsl_route_unlock_node (rn_head);

  HSL_FIB_UNLOCK;

  return i;
}

#ifdef HAVE_IPV6
/*
  Handle neighbor advertisement. Basically add the nexthop to the hardware if required.
*/
int
hsl_fib_handle_neigbor_discovery(struct hsl_if *ifp, char *l3_hdr_ptr, int *pkt_type)
{
  u_char flags = 0;
  hsl_prefix_t p;
  struct hsl_ip6_hdr   *l3_hdr_ver6;
  struct hsl_icmp6_hdr *icmp6_hdr; 
  struct hsl_nd_opt_hdr *nd_opt;
  hsl_ipv6Address_t *srcip;
  hsl_ipv6Address_t zero_addr;
  unsigned  char eth_src[HSL_ETHER_ALEN];
  struct hsl_route_node *rnh;
  struct hsl_nh_entry *nh;
#if 0
  hsl_prefix_list_t *lst;
#endif
  int ret;

  HSL_FN_ENTER();

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh6, ifp->fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  l3_hdr_ver6 = (struct hsl_ip6_hdr *)l3_hdr_ptr;

  /* Make sure packet type is ICMPv6. */
  if(HSL_PROTO_ICMPV6 != l3_hdr_ver6->nxt_hdr)
    HSL_FN_EXIT(0); 

  /* Learning only from advertisments and solicitations. */
  icmp6_hdr = (struct hsl_icmp6_hdr *)(l3_hdr_ptr + sizeof(struct hsl_ip6_hdr));
  if (IPV6_ND_NEIGHBOR_ADVERT != icmp6_hdr->icmp6_type
      && IPV6_ND_NEIGHBOR_SOLICIT != icmp6_hdr->icmp6_type
      && IPV6_ND_ROUTER_SOLICIT != icmp6_hdr->icmp6_type
     )
    HSL_FN_EXIT(0); 
     
  if(pkt_type)
    *pkt_type = IPV6_ND_NEIGHBOR_ADVERT;

  srcip = (hsl_ipv6Address_t *)(icmp6_hdr + 1);
  
  /* Get Neighbor discovery options */
  if (IPV6_ND_ROUTER_SOLICIT == icmp6_hdr->icmp6_type)
    nd_opt = (struct hsl_nd_opt_hdr *)(icmp6_hdr + 1);
  else
    nd_opt = (struct hsl_nd_opt_hdr *)(srcip + 1);

  /* If this is a neighbor solicitation message, use
   * the source IP address in the IPv6 header as the neighbor's
   * address.
   */
  if (IPV6_ND_NEIGHBOR_SOLICIT == icmp6_hdr->icmp6_type
      || IPV6_ND_ROUTER_SOLICIT == icmp6_hdr->icmp6_type)
        srcip = (hsl_ipv6Address_t *)&l3_hdr_ver6->ip_src;

  /* Verify that the src ip address is not unspecified address
   * (which could be case for some solicitations)
   */
  if (memcmp (srcip, &zero_addr, sizeof (zero_addr)) == 0)
    HSL_FN_EXIT(0);

  /* Check options type. */
  if (IPV6_ND_OPT_TARGET_LINKADDR != nd_opt->nd_opt_type
      && IPV6_ND_OPT_SRC_LINKADDR != nd_opt->nd_opt_type)
    HSL_FN_EXIT(HSL_FIB_ERR_WRONG_ND_OPT_TYPE);
 
  /* Get source link layer address. */   
  memcpy(eth_src, nd_opt->nd_opt_data,HSL_ETHER_ALEN);

  /* Set as a valid NH. */
  flags |= HSL_NH_ENTRY_VALID;

  /* Set NH address. */
  p.family    = AF_INET6;
  p.prefixlen = IPV6_MAX_BITLEN;
  p.u.prefix6 = *srcip;

  /* Check if a static entry for this ARP exists. If yes, ignore
     this ARP. */
  HSL_FIB_LOCK;
  rnh = hsl_route_node_get (HSL_FIB_TBL (nh6, ifp->fib_id), &p);
  if (! rnh)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to allocate/find a nexthop entry.\n");
      HSL_FIB_UNLOCK;
      HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
    }

  if (rnh->info)
    {
      /* This is a update. */
      hsl_route_unlock_node (rnh);

      nh = (struct hsl_nh_entry *) rnh->info;
      if (FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC))
        {
          HSL_FIB_UNLOCK;
          HSL_FN_EXIT(0);
        }
    }

  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "Adding nexthop %x:%x:%x:%x (%02x%02x.%02x%02x.%02x%02x) on interface %s\n",
           srcip->word[0],srcip->word[1],srcip->word[2],srcip->word[3], 
           eth_src[0], eth_src[1], eth_src[2], eth_src[3], 
           eth_src[4], eth_src[5], ifp->name);

  ret = _hsl_fib_nh_add (ifp->fib_id, rnh, ifp, (char*)eth_src, flags, HSL_NH_TYPE_IPV6);
  if(ret < 0) 
    { 
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Error (%d)adding nexthop %x:%x:%x:%x on interface %s\n",
               ret,srcip->word[0],srcip->word[1],srcip->word[2],
               srcip->word[3], ifp->name);
    }
  HSL_FIB_UNLOCK;

  HSL_FN_EXIT(ret);
}


int
hsl_fib_ipv6_nbr_add (hsl_ipv6Address_t *addr,
                      unsigned int ifindex,
                      unsigned char *mac_addr,
                      unsigned char flags)
{
  struct hsl_if *ifp;
  int ret = 0;
  hsl_prefix_t pfx;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  pfx.family = AF_INET6;
  pfx.prefixlen = IPV6_MAX_PREFIXLEN;
  IPV6_ADDR_COPY (&pfx.u.prefix6, addr);

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return HSL_IFMGR_ERR_IF_NOT_FOUND;

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);
    }

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh6, ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);
    }

  ret = hsl_fib_nh_add (ifp->fib_id, &pfx, ifp, (char*)mac_addr,
                        flags, HSL_NH_TYPE_IPV6);
  HSL_IFMGR_IF_REF_DEC (ifp);

  return ret;
}


int
hsl_fib_ipv6_nbr_del (hsl_ipv6Address_t *addr, int delete_static,
    unsigned int ifindex)
{
  int ret = 0;
  hsl_prefix_t pfx;
  struct hsl_if *ifp;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return HSL_IFMGR_ERR_IF_NOT_FOUND;

  if (! HSL_FIB_ID_VALID (ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);
    }

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh6, ifp->fib_id))
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);
    }

  pfx.family = AF_INET6;
  pfx.prefixlen = IPV6_MAX_PREFIXLEN;
  IPV6_ADDR_COPY (&pfx.u.prefix6, addr);

  ret = hsl_fib_nh_delete (ifp->fib_id, &pfx, 0, 
                           delete_static, HSL_NH_TYPE_IPV6); 

  HSL_IFMGR_IF_REF_DEC (ifp);

  return ret;
}


int
hsl_fib_ipv6_nbr_del_all (hsl_fib_id_t fib_id, unsigned char clr_flag)
{
  struct hsl_route_node *rnh;
  struct hsl_nh_entry *nh;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh6, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  HSL_FIB_LOCK;

  for (rnh = hsl_route_top (HSL_FIB_TBL (nh6, fib_id)); rnh; 
       rnh = hsl_route_next (rnh))
    {
      if (! rnh->info)
        continue;

      nh = rnh->info;

      if ((! FLAG_ISSET (clr_flag,  HSL_ARP_FLAG_STATIC) && 
          FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC)) ||
          (! FLAG_ISSET (clr_flag, HSL_ARP_FLAG_DYNAMIC) &&
           ! FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC)) ||
           FLAG_ISSET (nh->flags, HSL_NH_ENTRY_BLACKHOLE))
        continue;

      _hsl_fib_nh_delete (fib_id, rnh, 0, 
                          FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC),
                          HSL_NH_TYPE_IPV6, HSL_TRUE);      
    }
  
  HSL_FIB_UNLOCK;
  return 0;
}


int
hsl_fib_nh6_get_bundle (hsl_fib_id_t fib_id, hsl_ipv6Address_t *addr, int count,
                        struct hal_ipv6_nbr_cache_entry *cache)
{
  struct hsl_nh_entry *nh;
  struct hsl_route_node *rnh, *rn_head;
  int i = 0;

  /* If global fib not initialized, just return. */
  if (! hsl_fib_initialized)
    HSL_FN_EXIT (HSL_FIB_ERR_NOT_INITIALIZED);

  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_INVALID_ID);

  /* Check if table exists */
  if (! HSL_FIB_TBL_VALID (nh6, fib_id))
    HSL_FN_EXIT (HSL_FIB_ERR_TBL_NOT_FOUND);

  if (count <= 0)
    return 0;

  HSL_FIB_LOCK;
  
  if (addr)
    {
      rn_head = hsl_route_node_get_ipv6 (HSL_FIB_TBL (nh6, fib_id), addr);
      if (rn_head)
        rn_head = hsl_route_next (rn_head);
    }
  else
    rn_head = hsl_route_top (HSL_FIB_TBL (nh6, fib_id));
  
  if (! rn_head)
    {
      HSL_FIB_UNLOCK;
      return 0;
    }
  for (rnh = hsl_route_lock_node (rn_head); rnh; rnh = hsl_route_next (rnh))
    {
      for (nh = rnh->info; nh; nh = nh->next)
        {
          if (nh->nh_type == HSL_NH_TYPE_IPV6)
            break;
        }

      if (! nh || ! FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID))
        continue;
      
      if (nh->ifp == NULL)
        continue;

      IPV6_ADDR_COPY (&cache[i].addr, &rnh->p.u.prefix6); 
      memcpy (&cache[i].mac_addr, nh->mac, HSL_ETHER_ALEN);
      cache[i].ifindex = nh->ifp->ifindex;
      if (FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC))
        cache[i].static_flag = 1;
      else
        cache[i].static_flag = 0;
      
      if (++i == count)
        {
          hsl_route_unlock_node (rnh);
          break;
        }
    }
  
  hsl_route_unlock_node (rn_head);

  HSL_FIB_UNLOCK;

  return i;
}
#endif /* HAVE_IPV6 */

/* 
   Common function to show nh entry. 
*/
static int
_hsl_fib_show_nh(struct hsl_nh_entry *nh)
{
  char buf[256]; 
  char mac[20];

  HSL_FN_ENTER();

  if (! nh)
    HSL_FN_EXIT (-1);

  hsl_prefix2str (&nh->rn->p, buf, nh->rn->p.prefixlen);

  /* Skip invalid entries. */
  if(!CHECK_FLAG (nh->flags, HSL_NH_ENTRY_VALID))
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"Unresolved next hop: %s\n",buf);
      HSL_FN_EXIT(0);
    }

  hsl_mac_2_str(nh->mac,mac,20);
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"prefix: %s  mac: %s if: %s\n",buf, mac, nh->ifp ? nh->ifp->name : "Null");
  
  HSL_FN_EXIT(0);
}

/* 
   Common function to show prefix entry. 
*/
static int
_hsl_fib_show_prefix(struct hsl_route_node *rnp)
{
  struct hsl_prefix_entry *pe;
  char buf[256]; 
  char flags_buf[100];

  HSL_FN_ENTER();

  pe = rnp->info;
  if (! pe)
    return -1;

  hsl_prefix2str (&rnp->p, buf, rnp->p.prefixlen);
  sprintf(flags_buf,"%s %s", 
          (HSL_PREFIX_ENTRY_POPULATED_IN_HW(rnp))?"Installed":"Not Installed",
          (HSL_PREFIX_ENTRY_POPULATED_AS_EXCEPTION(rnp))?"TRAP_TO_CPU":"FORWARD");
 
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"prefix: %s  flags: %s\n",buf,flags_buf);
  HSL_FN_EXIT(0);
}

/*
  Common function to show prefix table(prefix/nh)
*/
int
hsl_fib_prefix_show(struct hsl_route_node *top)
{
  struct hsl_nh_entry *nh;
  struct hsl_route_node *rnp;
  struct hsl_prefix_entry *pe;
  struct hsl_nh_entry_list_node *nhlist;
  char mac[20];

  HSL_FN_ENTER();

  if ((!hsl_fib_initialized) || (!top))
    HSL_FN_EXIT(-1);

  HSL_FIB_LOCK;

  for (rnp = top; rnp; rnp = hsl_route_next (rnp))
    {
      pe = rnp->info;
      if (! pe)
        continue;

      _hsl_fib_show_prefix(rnp);

      if(HSL_PREFIX_ENTRY_POPULATED_AS_EXCEPTION(rnp))
        continue; 

      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"Next hops\n");
      for (nhlist = pe->nhlist; nhlist; nhlist = nhlist->next)
        {
          nh = nhlist->entry; 
          if (nh)
            {
              hsl_mac_2_str(nh->mac,mac,20);
              HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"mac: %s if: %s\n",mac, nh->ifp ? nh->ifp->name : "Null");
            }
        }       
    }
     
  HSL_FIB_UNLOCK;
  HSL_FN_EXIT(0);
}

/*
  Common function to show nh table(prefix/nh)
*/
int
hsl_fib_nh_show(struct hsl_route_node *top)
{
  struct hsl_nh_entry *nh;
  struct hsl_route_node *rnh;
  struct hsl_route_node *rnp;
  struct hsl_avl_node *node;

  HSL_FN_ENTER();

  if ((!hsl_fib_initialized) || (!top))
    HSL_FN_EXIT(-1);

  HSL_FIB_LOCK;

  for (rnh = top; rnh; rnh = hsl_route_next (rnh))
    {
      nh = rnh->info;
      if (! nh)
        continue;


      _hsl_fib_show_nh(nh);
      /* Show all routes depend on this nh. */ 
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"Dependant prefixes list\n");
      for (node = hsl_avl_top (nh->prefix_tree); node; node = hsl_avl_next (node))
        {
          rnp = node->info;
          if (! rnp)
            continue;
          _hsl_fib_show_prefix(rnp);
        }
    }
  
  HSL_FIB_UNLOCK;
  HSL_FN_EXIT(0);
}

int hsl_fib_dump(HSL_BOOL by_prefix)
{
  struct hsl_route_table *tbl;
  hsl_fib_id_t i;

  HSL_FN_ENTER();
  if(by_prefix)
    {
      HSL_FIB_TBL_TRAVERSE_START (prefix, tbl, i)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"IPv4 FIB %d\n", i);
          hsl_fib_prefix_show(hsl_route_top (tbl));
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"\n");
        }
      HSL_FIB_TBL_TRAVERSE_END (prefix, tbl, i)
#ifdef HAVE_IPV6
      HSL_FIB_TBL_TRAVERSE_START (prefix6, tbl, i)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"IPv6 FIB %d\n", i);
          hsl_fib_prefix_show(hsl_route_top (tbl));
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"\n");
        }
      HSL_FIB_TBL_TRAVERSE_END (prefix6, tbl, i)
#endif /* HAVE_IPV6 */
    }
  else 
    {
      HSL_FIB_TBL_TRAVERSE_START (nh, tbl, i)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"IPv4 FIB %d\n", i);
          hsl_fib_nh_show(hsl_route_top (tbl));
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"\n");
        }
      HSL_FIB_TBL_TRAVERSE_END (nh, tbl, i)
#ifdef HAVE_IPV6
      HSL_FIB_TBL_TRAVERSE_START (nh6, tbl, i)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"IPv6 FIB %d\n", i);
          hsl_fib_nh_show(hsl_route_top (tbl));
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO,"\n");
        }
      HSL_FIB_TBL_TRAVERSE_END (nh6, tbl, i)
#endif /* HAVE_IPV6 */
    }
  HSL_FN_EXIT(0);
}
