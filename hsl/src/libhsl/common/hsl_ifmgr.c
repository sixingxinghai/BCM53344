/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include "hsl_types.h"
#include "hsl_avl.h"
#include "hsl_oss.h"
#include "hsl_error.h"

#include "hsl_logger.h"
#include "hsl_logs.h"
#include "hsl_ifmgr.h"
#include "hsl_if_os.h"
#include "hsl_if_hw.h"
#include "hsl_if_cust.h"
#include "hsl_ether.h"
#ifdef HAVE_L3
#include "hsl_table.h"
#include "hsl_fib.h"
#endif /* HAVE_L3 */
#include "hal_acl.h"
#include "hal_msg.h"
#ifdef HAVE_VLAN
#include "hal_types.h"
#include "hal_l2.h"
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "hsl_mac_tbl.h"
#endif /* HAVE_VLAN */

/* 
   Interface manager database. 
*/
struct hsl_if_db *p_hsl_if_db = NULL;
static HSL_BOOL hsl_ifmgr_initialized = HSL_FALSE; 

/* 
   Forward declaration. 
*/
#ifdef HAVE_L3 
static int _hsl_ifmgr_ip_address_delete_process (struct hsl_if *ifp, hsl_prefix_t *prefix, HSL_BOOL os_cb);
#endif /* HAVE_L3 */
/*
  Interface type strings.
*/
static char *
_hsl_ifmgr_iftype_str (hsl_ifType_t type)
{
  switch (type)
    {
    case HSL_IF_TYPE_UNK:
      return "Unknown";
    case HSL_IF_TYPE_LOOPBACK:
      return "Loopback";
    case HSL_IF_TYPE_L2_ETHERNET:
      return "L2 Ethernet";
    case HSL_IF_TYPE_IP:
      return "IP interface";
    default:
      return "";
    }
}

/* 
   Find interface by index. 
*/
struct hsl_if *
hsl_ifmgr_lookup_by_index (hsl_ifIndex_t ifindex)
{
  struct hsl_if tifp, *ifp;
  struct hsl_avl_node *node;

  HSL_FN_ENTER ();

  if(!hsl_ifmgr_initialized) 
    return (NULL);

  tifp.ifindex = ifindex;

  HSL_IFMGR_LOCK;

  node = hsl_avl_lookup (p_hsl_if_db->if_tree, (void *)&tifp);
  if (! node)
    {
      HSL_IFMGR_UNLOCK;
      return  (NULL);
    }

  ifp = (struct hsl_if *) HSL_AVL_NODE_INFO (node);
  HSL_IFMGR_IF_REF_INC (ifp);
  HSL_IFMGR_UNLOCK;

  return ifp;
}

/*
  Find interface by name 
*/
struct hsl_if *
hsl_ifmgr_lookup_by_name (char *name)
{
  struct hsl_if *ifp;
  struct hsl_avl_node *node;
  char buf[HSL_IFNAM_SIZE + 1];
  int name_len;

  HSL_FN_ENTER (); 

  if(!hsl_ifmgr_initialized) 
    HSL_FN_EXIT(NULL);

  memset(buf,0,HSL_IFNAM_SIZE + 1);
  name_len = hsl_strlen(name);
  /* Name can be HSL_IFNAM_SIZE long. So peform strictly greater than check */
  if(name_len > HSL_IFNAM_SIZE)
    {
      HSL_FN_EXIT(NULL);
    }
  memcpy (buf, name, name_len);

  HSL_IFMGR_LOCK;

  for (node = hsl_avl_top (p_hsl_if_db->if_tree); node; node = hsl_avl_next (node))
    {
      if (((ifp = node->info) != NULL) && 
          (memcmp (ifp->name, buf, HSL_IFNAM_SIZE) == 0))
        {
          HSL_IFMGR_IF_REF_INC (ifp);
          HSL_IFMGR_UNLOCK;
          HSL_FN_EXIT (ifp);
        }
    }
  
  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (NULL);
}

/*
  Find interface by name and type
*/
struct hsl_if *
hsl_ifmgr_lookup_by_name_type (char *name, hsl_ifType_t type)
{
  struct hsl_if *ifp;
  struct hsl_avl_node *node;
  char buf[HSL_IFNAM_SIZE + 1];
  int name_len;

  HSL_FN_ENTER (); 

  memset(buf,0,HSL_IFNAM_SIZE + 1);
  name_len = hsl_strlen(name);
  /* Name can be HSL_IFNAM_SIZE long. So peform strictly greater than check */
  if(name_len > HSL_IFNAM_SIZE)
    {
      HSL_FN_EXIT(NULL);
    }
  memcpy (buf, name, name_len);

  HSL_IFMGR_LOCK;

  for (node = hsl_avl_top (p_hsl_if_db->if_tree); node; node = hsl_avl_next (node))
    {
      if (((ifp = node->info) != NULL) && 
          (memcmp (ifp->name, buf, HSL_IFNAM_SIZE) == 0) &&
          (ifp->type == type))
        {
          HSL_IFMGR_IF_REF_INC (ifp);
          HSL_IFMGR_UNLOCK;
          HSL_FN_EXIT (ifp);
        }
    }
  
  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (NULL);
}

/*
  Find interface by prefix
*/
struct hsl_if *
hsl_ifmgr_lookup_by_prefix (hsl_prefix_t *prefix)
{
  struct hsl_if *ifp;
  struct hsl_avl_node *node;
  hsl_prefix_list_t *ucaddr;

  HSL_FN_ENTER ();

  if(!hsl_ifmgr_initialized)
    HSL_FN_EXIT(NULL);

   HSL_IFMGR_LOCK;

  for (node = hsl_avl_top (p_hsl_if_db->if_tree); node; node = hsl_avl_next (node))
    {
      if ((ifp = node->info) != NULL && (ifp->type == HSL_IF_TYPE_IP))
        {
         if (prefix->family == AF_INET)
           {
             if (ifp->u.ip.ipv4.ucAddr)
               {  
                  ucaddr = ifp->u.ip.ipv4.ucAddr;
                  while (ucaddr)
                       {
                         if (IPV4_ADDR_SAME (&ucaddr->prefix.u.prefix4, 
                                             &prefix->u.prefix4)) 
                           {  
                              HSL_IFMGR_IF_REF_INC (ifp);
                              HSL_IFMGR_UNLOCK;
                              HSL_FN_EXIT (ifp);
                           }
                   /* Goto next address. */
                   ucaddr = ucaddr->next;
                  }           
               }
           }
        }
    }

  HSL_IFMGR_UNLOCK;

  HSL_FN_EXIT (NULL);
}

/*
  Get a new free ifindex for L2 ports. 
*/
static hsl_ifIndex_t
_hsl_ifmgr_get_L2_ifindex (void)
{
  int i;
  int idx;
  static int start_index = HSL_L2_IFINDEX_START;
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  for (i = 1; i < HSL_L2_IFINDEX_MAX - HSL_L2_IFINDEX_START; i++)
    {
      idx = (start_index - HSL_L2_IFINDEX_START + i)
        % (HSL_L2_IFINDEX_MAX - HSL_L2_IFINDEX_START) + HSL_L2_IFINDEX_START;

      ifp = hsl_ifmgr_lookup_by_index (idx);
      if (ifp == NULL)
        {
          start_index = idx;
          HSL_FN_EXIT (idx);
        }
      HSL_IFMGR_IF_REF_DEC (ifp);
    }

  HSL_FN_EXIT (0);
}

#ifdef HAVE_MPLS
/*
  Get a new free ifindex for MPLS ports. 
*/
static hsl_ifIndex_t
_hsl_ifmgr_get_mpls_ifindex (void)
{
  int i;
  int idx;
  static int start_index = HSL_MPLS_IFINDEX_START;
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  for (i = 1; i < HSL_MPLS_IFINDEX_MAX - HSL_MPLS_IFINDEX_START; i++)
    {
      idx = (start_index - HSL_MPLS_IFINDEX_START + i)
        % (HSL_MPLS_IFINDEX_MAX - HSL_MPLS_IFINDEX_START) + HSL_MPLS_IFINDEX_START;

      ifp = hsl_ifmgr_lookup_by_index (idx);
      if (ifp == NULL)
        {
          start_index = idx;
          HSL_FN_EXIT (idx);
        }
      HSL_IFMGR_IF_REF_DEC (ifp);
    }

  HSL_FN_EXIT (0);
}
#endif /* HAVE_MPLS */

/* 
   HSL interface structure allocation. 
*/
static struct hsl_if *
hsl_ifmgr_if_new ()
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  ifp = oss_malloc (sizeof (struct hsl_if), OSS_MEM_HEAP);

  HSL_FN_EXIT (ifp);
}

/* HSL interface structure free. */
static void
hsl_ifmgr_if_free (struct hsl_if *ifp)
{
  HSL_FN_ENTER ();

  if (ifp)
    oss_free (ifp, OSS_MEM_HEAP);

  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Internal routine to register ifmgr notifier chain. 
*/
static int
_hsl_ifmgr_notify_chain_register (struct hsl_if_notifier_chain **plist,
                                  struct hsl_if_notifier_chain *new)
{
  HSL_FN_ENTER ();

  while (*plist)
    {
      if (new->priority > (*plist)->priority)
        break;
      plist= &((*plist)->next);
    }
  new->next = *plist;
  *plist = new;

  HSL_FN_EXIT (0);
}


/* 
   BCM ifmgr notifier chain registration. 
*/
int
hsl_ifmgr_notify_chain_register (struct hsl_if_notifier_chain *new)
{
  HSL_FN_ENTER ();

  _hsl_ifmgr_notify_chain_register (&p_hsl_if_db->chain, new);

  HSL_FN_EXIT (0);
}

/* 
   Internal routine to unregister ifmgr notifier chain. 
*/
static int
_hsl_ifmgr_notify_chain_unregister (struct hsl_if_notifier_chain **plist,
                                    struct hsl_if_notifier_chain *old)
{
  HSL_FN_ENTER ();

  while ((*plist) != NULL)
    {
      if ((*plist) == old)
        {
          *plist = old->next;
          HSL_FN_EXIT (0);
        }
      plist = &((*plist)->next);
    }

  HSL_FN_EXIT (-1);
}

/* 
   ifmgr notifier chain unregistration. 
*/
int
hsl_ifmgr_notify_chain_unregister (struct hsl_if_notifier_chain *old)
{
  int ret;

  HSL_FN_ENTER ();

  ret = _hsl_ifmgr_notify_chain_unregister (&p_hsl_if_db->chain, old);

  HSL_FN_EXIT (ret);
}

/* 
   ifmgr notifier chain function. 
*/
static int
_hsl_ifmgr_notify_chain (struct hsl_if_notifier_chain **plist, int event, void *param1, void *param2)
{
  struct hsl_if_notifier_chain *list = *plist;

  HSL_FN_ENTER ();

  while (list)
    {
      /* Call notifier. */
      list->notifier (event, param1, param2);

      list = list->next;
    }

  HSL_FN_EXIT (0);
}

int
hsl_ifmgr_send_notification (int event, void *param1, void *param2)
{
  return _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, event, param1, param2);
}

/*
  Add ifp to a list. 
*/
static int
_hsl_ifmgr_add_to_list (struct hsl_if_list **list, struct hsl_if *ifp)
{
  struct hsl_if_list *node;

  HSL_FN_ENTER ();

  node = oss_malloc (sizeof (struct hsl_if_list), OSS_MEM_HEAP);
  if (! node)
    return (HSL_IFMGR_ERR_MEMORY);

  /* Set ifp. */
  node->ifp = ifp;
  
  if (*list)
    node->next = *list;
  *list = node;
  
  HSL_FN_EXIT (0);
}

static int
_hsl_ifmgr_add_to_list_tail (struct hsl_if_list **list, struct hsl_if *ifp)
{
  struct hsl_if_list *node;
  struct hsl_if_list *node_tail;

  HSL_FN_ENTER ();

  node = *list;
  while (node && node->next)
    node = node->next;

  node_tail = oss_malloc (sizeof (struct hsl_if_list), OSS_MEM_HEAP);
  if (! node_tail)
    return (HSL_IFMGR_ERR_MEMORY);

  /* Set ifp. */
  node_tail->ifp = ifp;

  if (node)
    node->next = node_tail;
  else
    *list = node_tail;

  HSL_FN_EXIT (0);
}

/*
  Check if ifp exists in a list.
*/
static HSL_BOOL
_hsl_ifmgr_list_ifp_is_exist (struct hsl_if_list **list, struct hsl_if *ifp)
{
  struct hsl_if_list *node;

  for (node = *list; node; node = node->next)
    {
      if (node->ifp == ifp)
        {
          return HSL_TRUE;
        }
    }
  return HSL_FALSE;
}

/*
  Delete ifp from a list.
*/
static int
_hsl_ifmgr_delete_from_list (struct hsl_if_list **list, struct hsl_if *ifp)
{
  struct hsl_if_list *node;
  struct hsl_if_list *prev = NULL;

  HSL_FN_ENTER ();

  node = *list;
  while (node)
    {
      if (node->ifp == ifp)
        {
          if (prev == NULL)
            *list = node->next;
          else
            prev->next = node->next;

          /* Free node. */
          oss_free (node, OSS_MEM_HEAP);

          break;
        }

      prev = node;
      node = node->next;
    }
                                  
  HSL_FN_EXIT (0);
}

/* 
   Comparision function.
*/
static int
_hsl_ifmgr_if_cmp (void *param1, void *param2)
{
  struct hsl_if *ifp1 = (struct hsl_if *) param1;
  struct hsl_if *ifp2 = (struct hsl_if *) param2;

  /* Less than. */
  if (ifp1->ifindex < ifp2->ifindex)
    return (-1);

  /* Greater than. */
  if (ifp1->ifindex > ifp2->ifindex)
    return (1);

  /* Equals to. */
  return (0);
}

static void
_hsl_ifmgr_if_data_free(void *ptr)
{
  struct hsl_if *ifp;

  if(NULL != ptr)
    {
      ifp = (struct hsl_if *)ptr;   
      hsl_ifmgr_if_free (ifp);
    }
  return;
}

/*
  Interface tree free function.
*/
static int
_hsl_ifmgr_if_tree_free (struct hsl_avl_tree **if_tree)
{
  HSL_FN_ENTER();
  hsl_avl_tree_free (if_tree,_hsl_ifmgr_if_data_free);
  HSL_FN_EXIT (0);
}

/*
  Interface manager database deinitialization.
*/
static int
_hsl_ifmgr_db_deinit (struct hsl_if_db **db)
{
  HSL_FN_ENTER ();

  if (! *db)
    HSL_FN_EXIT (0);

  /* Free tree. */
  _hsl_ifmgr_if_tree_free (&(*db)->if_tree);

  /* Delete semaphore. */
  oss_sem_delete (OSS_SEM_MUTEX, (*db)->ifmutex);

  /* Free database. */
  oss_free (*db, OSS_MEM_HEAP);
  *db = NULL;

  HSL_FN_EXIT (0);
}

/*
  Create interface manager database.
*/
static struct hsl_if_db *
_hsl_ifmgr_db_init (void)
{
  int ret;
  struct hsl_if_db *db;

  HSL_FN_ENTER ();

  /* Create database. */
  db = oss_malloc (sizeof (struct hsl_if_db), OSS_MEM_HEAP);
  if (! db)
    HSL_FN_EXIT (NULL);

  /* Create tree. */
  ret =  hsl_avl_create (&db->if_tree, 0, _hsl_ifmgr_if_cmp);
  if (ret < 0)
    {
      if (db)
        oss_free (db, OSS_MEM_HEAP);

      HSL_FN_EXIT (NULL);
    }

  db->chain = NULL;
  db->os_cb = NULL;
  db->hw_cb = NULL;
  db->proc_if_create_cb = NULL;
  db->proc_if_remove_cb = NULL;

  /* Create semaphore. */
  ret = oss_sem_new ("IFMGR_MUTEX", OSS_SEM_MUTEX, 0, NULL, &db->ifmutex);
  if (ret < 0)
    {
      _hsl_ifmgr_db_deinit (&db);
      HSL_FN_EXIT (NULL);
    }
    
  HSL_FN_EXIT (db);
}
/*
   Process updated interfaces list. 
 */
static int
_hsl_ifmgr_process_if_update_list (int oper, HSL_BOOL send_notification, struct hsl_if_list *p_ifp_update_list)
{
  struct hsl_if_list *p_node;
  struct hsl_if *ifp; 
#ifdef HAVE_L3
  hsl_prefix_list_t *ifaddr, *nifaddr;
#endif /*HAVE_L3*/

  HSL_FN_ENTER(); 

  p_node = p_ifp_update_list;

  while (p_node)
    {
       ifp = p_node->ifp;
       p_node = p_node->next;

       if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
       {
           /* Set flags in OS. */
           switch (oper)
           { 
              case HSL_IF_DEC_OP_COUNT:
                 if (p_hsl_if_db->os_cb && p_hsl_if_db->os_cb->os_l3_if_flags_set)
                    HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_unset) (ifp, IFF_RUNNING);
                 if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_flags_unset))
                    HSL_IFMGR_HWCB_CALL(hw_l3_if_flags_unset) (ifp, IFF_RUNNING);

#ifdef HAVE_L3
                 ifaddr = IFP_IP(ifp).ipv4.ucAddr;

                 if (IFP_IP(ifp).ipv4.nucAddr == 0)
                   break;

                 while (ifaddr)
                 {
                   /* Set next. */
                   nifaddr = ifaddr->next;
                   hsl_fib_delete_connected(&ifaddr->prefix, ifp, HSL_TRUE);
                   ifaddr = nifaddr;
                 }
#endif /*HAVE_L3*/
                 break;  
              case HSL_IF_INC_OP_COUNT:
                if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_set))
                    HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_set) (ifp, IFF_RUNNING);
                if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_flags_set))
                   HSL_IFMGR_HWCB_CALL(hw_l3_if_flags_set) (ifp, IFF_RUNNING);
                break;
            }
        }

        if (send_notification && (!CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED)))
            _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFFLAGS, ifp, NULL);
         
       _hsl_ifmgr_delete_from_list (&p_ifp_update_list, ifp);
       HSL_IFMGR_IF_REF_DEC(ifp);
     }

   HSL_FN_EXIT(STATUS_OK);  
}
/*
  Interface Op count update function 
*/
static int
_hsl_ifmgr_calc_op_count (struct hsl_if *ifp,int oper, int step, 
                          struct hsl_if_list **pp_ifp_update_list)
{
  int ret;
  HSL_FN_ENTER(); 

  if((!ifp) || (!pp_ifp_update_list))
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);

  /* Update operational count. */
  switch (oper)
    { 
    case HSL_IF_DEC_OP_COUNT:
      if ((ifp->operCnt - step) > 0)
        ifp->operCnt -= step;
      else 
        ifp->operCnt = 0;
 
      if (ifp->operCnt == 0)
        {
          /* Interface doesn't have any other active L2 ports. */  
          ifp->flags &= ~IFF_RUNNING;           
              
          /* Call notifier for interface link down. */
          if (!CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
            {
               /* Add interface to an updated interfaces list . */
               ret = _hsl_ifmgr_add_to_list (pp_ifp_update_list, ifp);
               if(ret < 0)
                  HSL_FN_EXIT(STATUS_ERROR);
               HSL_IFMGR_IF_REF_INC (ifp);
            }
        }
      break;
    case HSL_IF_INC_OP_COUNT:
      ifp->operCnt += step;
                   
      if ((ifp->operCnt == step) && (step > 0))
        {
          /* At least one L2 port is up. */
         if (ifp->flags & IFF_UP)
          ifp->flags |= IFF_RUNNING;

          /* Call notifier for interface link up. */
          if (!CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
            {
               /* Add interface to an updated interfaces list . */
               ret = _hsl_ifmgr_add_to_list (pp_ifp_update_list, ifp);
               if(ret < 0)
                  HSL_FN_EXIT(STATUS_ERROR);
               HSL_IFMGR_IF_REF_INC (ifp);
            }
        }
      break;
    }
  HSL_FN_EXIT(STATUS_OK);
}

/*
  Interface Op count update function. 
*/
static int
_hsl_ifmgr_update_op_count2 (struct hsl_if *ifp,int oper, int step, 
                            HSL_BOOL recursive_call, 
                            struct hsl_if_list **pp_ifp_update_list)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node;

  HSL_FN_ENTER();

  if((!ifp) || (!pp_ifp_update_list))
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);

  if(!step)
    HSL_FN_EXIT(STATUS_OK);

  /* Update op counter for interface */  
  if(HSL_TRUE != recursive_call)
    _hsl_ifmgr_calc_op_count (ifp,oper,step, pp_ifp_update_list);

  /* Update op counter for interface */  
  if (!ifp->parent_list)
    HSL_FN_EXIT(STATUS_OK);
    
  node = ifp->parent_list;

  while (node)
    {
      ifp2 = node->ifp;

      if(!ifp2)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Empty node in parent list.\n");
          continue;
        }

      /* Calculate interface operational count. */  
      _hsl_ifmgr_calc_op_count (ifp2, oper, step, pp_ifp_update_list);

      /* Update next level as well. */
      if(ifp2->parent_list) 
        _hsl_ifmgr_update_op_count2 (ifp2, oper, step, HSL_TRUE, pp_ifp_update_list);

      node = node->next;
    }

  HSL_FN_EXIT(STATUS_OK);
}

/*
  Interface Op count update function. 
*/
static int
_hsl_ifmgr_update_op_count (struct hsl_if *ifp,int oper, int step, HSL_BOOL send_notify)
{
  int ret;
  struct hsl_if_list *p_ifp_update_list = NULL;

  HSL_FN_ENTER();  

  HSL_IFMGR_LOCK;

  /* Update op count for all parent interfaces. */
  ret = _hsl_ifmgr_update_op_count2 (ifp, oper, step, HSL_FALSE, &p_ifp_update_list);

  HSL_IFMGR_UNLOCK;

  if(ret < 0)
   {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Error (%d) Fill interface op count update list.\n");
      HSL_FN_EXIT(STATUS_ERROR);
   }

   /* Process updates list. */
   _hsl_ifmgr_process_if_update_list (oper, send_notify, p_ifp_update_list);

   HSL_FN_EXIT(STATUS_OK);
}
/* 
   Initialization.
*/
int
hsl_ifmgr_init (void)
{
  struct hsl_if_db *db;

  HSL_FN_ENTER ();

  if (hsl_ifmgr_initialized)
    HSL_FN_EXIT (0);

  /* Initialize database. */
  db = _hsl_ifmgr_db_init ();
  if (! db)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INIT);

  /* Set database. */
  p_hsl_if_db = db;

#ifdef HAVE_L3
  /* Init OS interface callbacks. */
  hsl_if_os_cb_register ();
#endif /* HAVE_L3 */

  /* Init HW interface callbacks. */
  hsl_if_hw_cb_register ();

  /* Set policy for interface creation. */
  p_hsl_if_db->policy = HSL_IFMGR_IF_INIT_POLICY_NONE;

  /* Interface manager initialized. */
  hsl_ifmgr_initialized = HSL_TRUE;

  /* Initialize OS ifmgr data. */
  if (HSL_IFMGR_STACKCB_CHECK(os_if_init))
    HSL_IFMGR_STACKCB_CALL(os_if_init) ();

  /* Initialize Hw ifmgr data. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_init))
    HSL_IFMGR_HWCB_CALL(hw_if_init) ();

  HSL_FN_EXIT (0);
}

/* 
   Deinitialization.
*/
int
hsl_ifmgr_deinit (void)
{
  int ret;

  HSL_FN_ENTER ();

  if (! hsl_ifmgr_initialized)
    HSL_FN_EXIT (-1);

  /* Deinitlize HW ifmgr data. */
  if (p_hsl_if_db->os_cb && p_hsl_if_db->hw_cb->hw_if_deinit)
    HSL_IFMGR_HWCB_CALL(hw_if_deinit) ();

  /* Deinitialize OS ifmgr data. */
  if (HSL_IFMGR_STACKCB_CHECK(os_if_deinit))
    HSL_IFMGR_STACKCB_CALL(os_if_deinit) ();

  /* Unregister HW callbacks. */
  hsl_if_hw_cb_unregister ();

#ifdef HAVE_L3
  /* Unregister OS callbacks. */
  hsl_if_os_cb_unregister ();
#endif /* HAVE_L3 */

  /* Deinitialize database. */
  ret = _hsl_ifmgr_db_deinit (&p_hsl_if_db);
  if(ret < 0)
    HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Ifmgr db deinit failed.\n");
   
  hsl_ifmgr_initialized = HSL_FALSE; 

  HSL_FN_EXIT (0);
}

/*
  Get current interface creation policy.
*/
u_int8_t
hsl_ifmgr_get_policy (void)
{
  u_int8_t policy;

  if (! hsl_ifmgr_initialized)
    HSL_FN_EXIT (-1);

  HSL_IFMGR_LOCK;

  policy = p_hsl_if_db->policy;

  HSL_IFMGR_UNLOCK;
  
  return policy;
}

/* 
   Set policy for interface creation.
*/
void
hsl_ifmgr_set_policy (u_int8_t policy)
{
  HSL_IFMGR_LOCK;

  p_hsl_if_db->policy = policy;

  HSL_IFMGR_UNLOCK;
}

/*
  Copy properties from source interface to destination interface.
*/
static int
_hsl_ifmgr_copy_properties (struct hsl_if *dstif, struct hsl_if *srcif)
{
  int vlan_admin_flag_unset = 0;

  HSL_FN_ENTER ();

  /* saving the vlan admin state flag as the configuration on vlan interface
   * +      can be done independently of attached phy interface */
   if (!memcmp (dstif->name, "vlan", 4) && !HSL_IFP_ADMIN_UP (dstif))
     vlan_admin_flag_unset = 1;

  if (! dstif || ! srcif)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_PARAM);

  /* Flags. */
  if (HSL_IFP_ADMIN_UP (srcif))
    {
      dstif->flags = srcif->flags;
      dstif->flags |= IFF_UP;
    }
  else
    {
      dstif->flags = srcif->flags;
    }

  /* set ethernet port properties */
  if (dstif->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      /* Speed. */
      dstif->u.l2_ethernet.speed = srcif->u.l2_ethernet.speed;
      
      /* MTU. */
      dstif->u.l2_ethernet.mtu = srcif->u.l2_ethernet.mtu;

      /* Duplex. */
      dstif->u.l2_ethernet.duplex = srcif->u.l2_ethernet.duplex;

      /* Autonego. */
      dstif->u.l2_ethernet.autonego = srcif->u.l2_ethernet.autonego;
    }
  else if (dstif->type == HSL_IF_TYPE_IP) /* set L3 interface properties */
    {
      if (srcif->type == HSL_IF_TYPE_IP)
        {
          /* ipv4 properties */
          dstif->u.ip.ipv4.mtu = srcif->u.ip.ipv4.mtu;
          dstif->u.ip.ipv4.mode = srcif->u.ip.ipv4.mode;
          
#ifdef HAVE_IPV6
          /* ipv6 properties */
          dstif->u.ip.ipv6.mtu = srcif->u.ip.ipv6.mtu;
          dstif->u.ip.ipv6.mode = srcif->u.ip.ipv6.mode;
#endif /* HAVE_IPV6 */
        }
      else
        {
          dstif->flags |= srcif->flags;
          /* case of vlan interface*/
          if (vlan_admin_flag_unset)
            dstif->flags &= ~(IFF_UP|IFF_RUNNING);


          /* ipv4 properties */
          dstif->u.ip.ipv4.mtu = srcif->u.l2_ethernet.mtu;
#ifdef HAVE_IPV6
          /* ipv6 properties */
          dstif->u.ip.ipv6.mtu = srcif->u.l2_ethernet.mtu;
#endif /* HAVE_IPV6 */
        }
    }
#ifdef HAVE_MPLS 
  else if (dstif->type == HSL_IF_TYPE_MPLS)
    {
      if (srcif->type == HSL_IF_TYPE_L2_ETHERNET)
        {
          dstif->flags |= srcif->flags;
          dstif->u.mpls.mtu = srcif->u.l2_ethernet.mtu;
        }
    }
#endif /* HAVE_MPLS */

  HSL_FN_EXIT (0);
}

/*
  Reset properties of the interface.
*/
static int
_hsl_ifmgr_reset_properties (struct hsl_if *ifp)
{
  HSL_FN_ENTER ();

  /* reset ethernet port properties */
  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      ifp->u.l2_ethernet.speed = 0;
      ifp->flags = 0;
    }
  else
    {
      /* dont reset flags for vlan interface as there is no way we can restore
           the vlan's administrative flag without reconfiguration*/
       if (memcmp (ifp->name,"vlan",4))
      ifp->flags = 0;
    }
  
  HSL_FN_EXIT (0);
}

/*
  Find first L2 port.
*/
struct hsl_if *
hsl_ifmgr_get_first_L2_port (struct hsl_if *ifp)
{
  struct hsl_if *ifp2 = NULL;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();
  HSL_IFMGR_LOCK;

  node = ifp->children_list;

  while (node)
    {
      ifp2 = node->ifp;
      
      if (ifp2 == NULL)
        break;
      
      if (ifp2->type != HSL_IF_TYPE_L2_ETHERNET)
        {
          node = ifp2->children_list;
          continue;
        }
      else
        break;
    }

  HSL_IFMGR_UNLOCK;

  if (ifp2)
  { 
    HSL_IFMGR_IF_REF_INC (ifp2);
  }

  HSL_FN_EXIT (ifp2);
}

/*
  Find aggregated L2 port from member L2 port.
*/
struct hsl_if *
hsl_ifmgr_get_L2_parent (struct hsl_if *ifp)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node = NULL;

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    return NULL;

  node = ifp->parent_list;
  if (! node)
    return ifp;
  
  ifp2 = node->ifp;
  if (ifp2->type != HSL_IF_TYPE_L2_ETHERNET)
    return ifp;

  return ifp2;
}

/*
  Find top L3 port given a L2 port and NOT matching VLAN.
  Function checks if L2 port participate in any other SVI
  Returns -1 on error, 0 if no other SVI found, otherwise      
  returns positive value 
*/
int 
hsl_ifmgr_get_additional_L3_port (struct hsl_if *ifp, hsl_vid_t vid)
{
  struct hsl_if *ifp2 = NULL;
  struct hsl_if_list *node = NULL;

  HSL_FN_ENTER ();

  HSL_IFMGR_LOCK;

  node = ifp->parent_list;

  while (node)
    {
      ifp2 = node->ifp;

      if (ifp2->parent_list)
        {
          node = ifp2->parent_list;
          continue;
        }
      else
        break;
    }

  HSL_IFMGR_UNLOCK;
   
  /*  In case no parents found -> there is no additionanl SVIs */
  if(!ifp2)
    {
      HSL_FN_EXIT(0);
    }

  /* If top most interface is not L3 return an error. */
  if (ifp2->type != HSL_IF_TYPE_IP)
    {
      HSL_FN_EXIT(-1);
    }

  if (memcmp (ifp2->name, "vlan", 4))
    {
      /*
       * Found pure L3 interface. 
       * Should not happen for SVI check.       
       */   
      HSL_FN_EXIT(-1);
    }

  HSL_IFMGR_LOCK;

  while (node)
    {
      /* New vid found. */
      if (ifp2->u.ip.vid != vid)
        {
          HSL_IFMGR_UNLOCK;
          HSL_FN_EXIT(1);
        }
      node = node->next;
      ifp2 = node->ifp;
    }

  HSL_IFMGR_UNLOCK;

  /* No other SVI found. */
  HSL_FN_EXIT (0);
}


/*
  Find top L3 port given a L2 port and matching VLAN.
  For router ports, the VLAN is ignored. For SVIs the 
  matching interface based on VLAN is returned. This 
  function will increment reference count. The caller
  has to decrement the reference count after using the 
  ifp returned.
*/
struct hsl_if *
hsl_ifmgr_get_matching_L3_port (struct hsl_if *ifp, hsl_vid_t vid)
{
  struct hsl_if *ifp2 = NULL;
  struct hsl_if_list *node = NULL;

  HSL_FN_ENTER ();
  HSL_IFMGR_LOCK;
  node = ifp->parent_list;

  while (node && node->ifp->parent_list)
    node = node->ifp->parent_list;

  while (node && node->ifp)
    {
      ifp2 = node->ifp;
      if (ifp2->type == HSL_IF_TYPE_IP)
        break;
      node = node->next;
    }

  if (node && node->ifp)
    {
      ifp2 = node->ifp;

      if (ifp2->type != HSL_IF_TYPE_IP) 
        {
          HSL_IFMGR_UNLOCK;
          HSL_FN_EXIT (NULL);
        }

      if (memcmp (ifp2->name, "vlan", 4))
        {
          HSL_IFMGR_IF_REF_INC (ifp2);
          HSL_IFMGR_UNLOCK;
          HSL_FN_EXIT (ifp2);
        }
      else
        {
          while (node)
            {
              ifp2 = node->ifp;

              if (ifp2->u.ip.vid == vid)
                {
                  HSL_IFMGR_IF_REF_INC (ifp2);
                  HSL_IFMGR_UNLOCK;
                  HSL_FN_EXIT (ifp2);
                  break;
                }

              node = node->next;
            }
        }
    }

  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (NULL);
}

/*
  Lock all children.
*/
void
hsl_ifmgr_lock_children (struct hsl_if *ifp)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();
  HSL_IFMGR_LOCK;

  node = ifp->children_list;
  while (node)
    {
      ifp2 = node->ifp;
      HSL_IFMGR_IF_REF_INC (ifp2);
      node = node->next;
    }

  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Unlock all children.
*/
void
hsl_ifmgr_unlock_children (struct hsl_if *ifp)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();
  HSL_IFMGR_LOCK;

  node = ifp->children_list;
  while (node)
    {
      ifp2 = node->ifp;
      HSL_IFMGR_IF_REF_INC (ifp2);
      node = node->next;
    }

  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (STATUS_OK);
}

/*
  Lock all parents.
*/
void
hsl_ifmgr_lock_parents (struct hsl_if *ifp)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();
  HSL_IFMGR_LOCK;

  node = ifp->parent_list;
  while (node)
    {
      ifp2 = node->ifp;
      HSL_IFMGR_IF_REF_INC (ifp2);
      node = node->next;
    }

  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Unlock all parents.
*/
void
hsl_ifmgr_unlock_parents (struct hsl_if *ifp)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();
  HSL_IFMGR_LOCK;

  node = ifp->parent_list;
  while (node)
    {
      ifp2 = node->ifp;

      HSL_IFMGR_IF_REF_INC (ifp2);
      node = node->next;
    }

  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Set packet types accepted for interface.
*/
void
hsl_ifmgr_set_acceptable_packet_types (struct hsl_if *ifp, u_int32_t pkt_flags)
{
  HSL_FN_ENTER ();

  ifp->pkt_flags |= pkt_flags;

  /* Set new pkt types. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_packet_types_set))
    HSL_IFMGR_HWCB_CALL(hw_if_packet_types_set) (ifp, pkt_flags);

  HSL_FN_EXIT (STATUS_OK);
}

/*
  Unsett packet types acceptetd for this interface.
*/
void
hsl_ifmgr_unset_acceptable_packet_types (struct hsl_if *ifp, u_int32_t pkt_flags)
{
  HSL_FN_ENTER ();

  ifp->pkt_flags &= ~pkt_flags;

  /* Unset pkt types. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_packet_types_unset))
    HSL_IFMGR_HWCB_CALL(hw_if_packet_types_unset) (ifp, pkt_flags);
  
  HSL_FN_EXIT (STATUS_OK);
}

/*
  Is interface bound to each other?
*/
HSL_BOOL
hsl_ifmgr_isbound (struct hsl_if *ifpp, struct hsl_if *ifpc)
{
  HSL_BOOL ret = HSL_FALSE;
  
  HSL_FN_ENTER ();
  
  /* Sanity check. */
  if ((ifpc->type == HSL_IF_TYPE_IP) 
      && (ifpp->type == HSL_IF_TYPE_L2_ETHERNET))
    HSL_FN_EXIT (ret);
  
  if ((ifpp->children_list == NULL) || (ifpc->parent_list == NULL))
    HSL_FN_EXIT (ret);
 
  ret = ((_hsl_ifmgr_list_ifp_is_exist (&ifpp->children_list, ifpc) &&
          _hsl_ifmgr_list_ifp_is_exist (&ifpc->parent_list, ifpp)) ? HSL_TRUE : 
         HSL_FALSE);
  
  return ret;
}

/* 
   Bind interface with ifp.
*/
int
hsl_ifmgr_bind2 (struct hsl_if *ifpp, struct hsl_if *ifpc)
{
  int ret = 0;
  HSL_BOOL set_flags = HSL_FALSE;
  HSL_BOOL send_op_notify = HSL_FALSE;

  HSL_FN_ENTER ();

  /* Sanity check. */
  if ((ifpc->type == HSL_IF_TYPE_IP) 
      && (ifpp->type == HSL_IF_TYPE_L2_ETHERNET))
    HSL_FN_EXIT (HSL_IFMGR_ERR_BIND);

  /* Get properties of the first child port */
  if (ifpp->children_list == NULL)
    {
      _hsl_ifmgr_copy_properties (ifpp, ifpc);
      set_flags = HSL_TRUE;
    }

  HSL_IFMGR_LOCK;
 
  /* Add child to parent's child list. */
  ret = _hsl_ifmgr_add_to_list (&ifpp->children_list, ifpc);
  if (ret < 0)
    {
      HSL_IFMGR_UNLOCK;
      HSL_FN_EXIT (ret);
    }

  /* Add parent to child's parent list. */
  if (ifpp->type == HSL_IF_TYPE_MPLS)
    ret = _hsl_ifmgr_add_to_list_tail (&ifpc->parent_list, ifpp);
  else
    ret = _hsl_ifmgr_add_to_list (&ifpc->parent_list, ifpp);
  if (ret < 0)
    {
      _hsl_ifmgr_delete_from_list (&ifpp->children_list, ifpc);
      HSL_IFMGR_UNLOCK;
      HSL_FN_EXIT (ret);
    }

  HSL_IFMGR_UNLOCK;

  if (set_flags == HSL_TRUE)
    {
      /* Adjust the properties have been copied. Actually set the flags now. */
      hsl_ifmgr_set_flags2 (ifpp, ifpp->flags);
    }
  
  /* Update new parents  operational count. If pure L3 interface don't
     send notification, else send notification */
  if (!CHECK_FLAG (ifpc->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
    send_op_notify = HSL_TRUE;

  _hsl_ifmgr_update_op_count (ifpp, HSL_IF_INC_OP_COUNT, ifpc->operCnt,
                              send_op_notify);

  /* Perform any post configuration, if required. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_post_configure))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_post_configure) (ifpp, ifpc);
 
  HSL_FN_EXIT (ret);
}
  
/* 
   Bind interfaces with ifindexes.
*/
int
hsl_ifmgr_bind (hsl_ifIndex_t parentIfindex, hsl_ifIndex_t childIfindex)
{
  struct hsl_if *ifpp, *ifpc;
  int ret;

  HSL_FN_ENTER ();

  /* Sanity check. */
  if (parentIfindex == childIfindex)
    HSL_FN_EXIT (HSL_IFMGR_ERR_BINDING_SAME);

  /* Find parent in tree. */
  ifpp = hsl_ifmgr_lookup_by_index (parentIfindex);
  if (! ifpp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  /* Find child in tree. */
  ifpc = hsl_ifmgr_lookup_by_index (childIfindex);
  if (! ifpc)
    {
      HSL_IFMGR_IF_REF_DEC (ifpp);
      HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  ret = hsl_ifmgr_bind2 (ifpp, ifpc);

  HSL_IFMGR_IF_REF_DEC (ifpp);
  HSL_IFMGR_IF_REF_DEC (ifpc);

  HSL_FN_EXIT (ret);
}

/*
  Unbind interfaces using ifp.
*/
int
hsl_ifmgr_unbind2 (struct hsl_if *ifpp, struct hsl_if *ifpc)
{
  int ret;
  HSL_BOOL send_op_notify = HSL_FALSE;
  
  HSL_FN_ENTER ();

  /* Sanity check. */
  if ((ifpc->type == HSL_IF_TYPE_IP) 
      && (ifpp->type == HSL_IF_TYPE_L2_ETHERNET))
    HSL_FN_EXIT (HSL_IFMGR_ERR_BIND);

  /* Perform any pre unconfiguration, if required. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_pre_unconfigure))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_if_pre_unconfigure) (ifpp, ifpc);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Failure performing pre unconfiguration before unbinding\n");
          HSL_FN_EXIT (ret);
        }
    }
  
  /* Update old parents  operational count. If pure L3 interface don't send 
     notification, else send notification */
  if (!CHECK_FLAG (ifpc->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
    send_op_notify = HSL_TRUE;

  
  _hsl_ifmgr_update_op_count (ifpp, HSL_IF_DEC_OP_COUNT, ifpc->operCnt,
                              send_op_notify);

  HSL_IFMGR_LOCK;

  /* Delete child from parent's child list. */
  _hsl_ifmgr_delete_from_list (&ifpp->children_list, ifpc);

  /* Delete parent from child's parent list. */
  _hsl_ifmgr_delete_from_list (&ifpc->parent_list, ifpp);

  HSL_IFMGR_UNLOCK;

  /* Reset parent's port properties on deletion of last child */
  if (ifpp->children_list == NULL)
    {
      /* Reset properties. */
      _hsl_ifmgr_reset_properties (ifpp);

      /* reset ipv4(6) mode if parent and child ports are L3 */
      if (ifpp->type == HSL_IF_TYPE_IP &&
          ifpc->type == HSL_IF_TYPE_IP)
        {
          ifpp->u.ip.ipv4.mode = 0;
          ifpp->u.ip.ipv6.mode = 0;
        }
    }

  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Unbind interfaces using ifindexes.
*/
int
hsl_ifmgr_unbind (hsl_ifIndex_t parentIfindex, hsl_ifIndex_t childIfindex)
{
  struct hsl_if *ifpp, *ifpc;
  int ret = HSL_IFMGR_ERR_IF_NOT_FOUND;

  HSL_FN_ENTER ();

  /* Find parent in tree. */
  ifpp = hsl_ifmgr_lookup_by_index (parentIfindex);
  if (! ifpp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  /* Find child in tree. */
  ifpc = hsl_ifmgr_lookup_by_index (childIfindex);
  if (! ifpc)
    {
      HSL_IFMGR_IF_REF_DEC (ifpp);
      HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  ret = hsl_ifmgr_unbind2 (ifpp, ifpc);

  HSL_IFMGR_IF_REF_DEC (ifpp);
  HSL_IFMGR_IF_REF_DEC (ifpc);

  HSL_FN_EXIT (ret);
}

/* 
   Create Layer 2 ethernet interface.
*/
int
hsl_ifmgr_L2_ethernet_create (char *name, hsl_mac_address_t mac, 
                              u_int16_t mtu, u_int32_t speed, 
                              u_int32_t duplex, u_int32_t flags,
                              void *sys_info,
                              int send_notification,
                              u_int8_t if_flags,
                              struct hsl_if **ppifp)
{
  struct hsl_if ifp;
  hsl_ifIndex_t idx;
  int ret;
  int namelen;

  HSL_FN_ENTER ();
  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Registering port %s\n", name);

  
  /* Reset interface structure. */
  memset (&ifp,0,sizeof(struct hsl_if));

  /* Check name length. */
  if ((namelen = hsl_strlen (name)) > HSL_IFNAM_SIZE)
    HSL_FN_EXIT(HSL_IFMGR_ERR_NAME);

  /* Set name. */
  memcpy (ifp.name, name, namelen);

  /* Check if interface already exists. */
  *ppifp = hsl_ifmgr_lookup_by_name (ifp.name);
  if (*ppifp)
    {
      HSL_IFMGR_IF_REF_DEC (*ppifp);
      HSL_FN_EXIT(HSL_IFMGR_ERR_DUPLICATE);
    }

  /* Set type as L2. */
  ifp.type = HSL_IF_TYPE_L2_ETHERNET;

  /* Get new index for this port. */
  if (p_hsl_if_db->cm_cb && p_hsl_if_db->cm_cb->cust_if_alloc_ifindex)
    {
      ret = (*p_hsl_if_db->cm_cb->cust_if_alloc_ifindex) (&ifp,&idx);
      if(ret < 0) 
        HSL_FN_EXIT (HSL_IFMGR_ERR_INDEX);
    }
  else
    idx = _hsl_ifmgr_get_L2_ifindex ();

  if (idx == 0)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INDEX);

  /* Set index. */
  ifp.ifindex = idx;

  /* Speed. */
  ifp.u.l2_ethernet.speed = speed;

  /* MTU. */
  ifp.u.l2_ethernet.mtu = mtu;

  /* Duplex. */
  ifp.u.l2_ethernet.duplex = duplex;

  /* Flags. */
  ifp.flags = flags;

  /* System info. */
  ifp.system_info = sys_info;

  /* Internal Flags */
  ifp.if_flags = if_flags;

  /* Copy hardware address. */
  memcpy (ifp.u.l2_ethernet.mac, mac, HSL_ETHER_ALEN);

  /* Create interface in ifmgr. */
  ret = hsl_ifmgr_create_interface(&ifp, ppifp, send_notification, HSL_FALSE);
  if(ret != 0)
    {
      /* Free ifndex for this port. */
      if (p_hsl_if_db->cm_cb && p_hsl_if_db->cm_cb->cust_if_free_ifindex)
        (*p_hsl_if_db->cm_cb->cust_if_free_ifindex) (idx);
      HSL_FN_EXIT (ret);
    }
  /* Set acceptable packet types for pure L2 ports. */
  if(sys_info)
    {
      hsl_ifmgr_unset_acceptable_packet_types (*ppifp, HSL_IF_PKT_ALL);
      /* If port is up increment op count. */ 
      if(ifp.flags & IFF_RUNNING) 
        {
          _hsl_ifmgr_update_op_count(*ppifp, HSL_IF_INC_OP_COUNT, 1, HSL_TRUE);
        }
    }

  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Registered L2 port %s successfully.\n", name);
  HSL_FN_EXIT (ret);
}
     
/* 
   L2 port registration function. 
*/
int
hsl_ifmgr_L2_ethernet_register (char *name, hsl_mac_address_t mac, 
                                u_int16_t mtu, u_int32_t speed, 
                                u_int32_t duplex, u_int32_t flags,
                                void *sys_info,
                                u_int8_t if_flags,
                                struct hsl_if **ppifp)
{
  return hsl_ifmgr_L2_ethernet_create (name, mac, mtu, speed, duplex, flags,
                                       sys_info, 1, if_flags,
                                       ppifp);
}

/*
  Delete L2 port.
*/
void 
hsl_ifmgr_L2_ethernet_delete2 (struct hsl_if *ifp,
                               int send_notification)
{
  struct hsl_if_list *node;
  struct hsl_if *ifp2 = NULL;
  struct hsl_if *ifp1 = NULL;

  HSL_IFMGR_LOCK;
  HSL_FN_ENTER ();

  if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
  {
      node = ifp->parent_list;
      if(node)
        ifp1 = node->ifp;
  }

  if (send_notification)
  {
      if(ifp1)
        ifp = ifp1;
      /* Clean up will be done by nsm, just send notification & 
         wait for cleanup done. */
      _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFDELETE, ifp, NULL);
      HSL_IFMGR_UNLOCK;
      HSL_FN_EXIT(STATUS_OK);
  }

  /* Aggregated L2 port handling. */
  while (NULL != (node = ifp->children_list))
  {
     ifp2 = node->ifp;
     hsl_ifmgr_unbind2 (ifp, ifp2);
  }

#ifdef HAVE_VLAN
  /* Flush all vlans. */
  hsl_port_flush_vlans(ifp->ifindex);
#endif /* HAVE_L2 */

  if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
     hsl_ifmgr_delete_interface(ifp1, HSL_TRUE);

  /* Do any platform specific cleanup. */
  if (p_hsl_if_db->hw_cb && p_hsl_if_db->hw_cb->hw_l2_unregister)
    (*p_hsl_if_db->hw_cb->hw_l2_unregister) (ifp);

  /* Free ifndex for this port. */
  if (p_hsl_if_db->cm_cb && p_hsl_if_db->cm_cb->cust_if_free_ifindex)
    (*p_hsl_if_db->cm_cb->cust_if_free_ifindex) (ifp->ifindex);

  /* Delete the interface and notify PMs */
  hsl_ifmgr_delete_interface(ifp, HSL_TRUE);

  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (STATUS_OK);
}

/*
  Remove interface from interface manager. 
  IN ifindex -  Interface index. 
*/
int
hsl_ifmgr_clean_up_complete(hsl_ifIndex_t  ifindex)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER(); 

  ifp =  hsl_ifmgr_lookup_by_index (ifindex);
  if(!ifp)
  {
    HSL_FN_EXIT(0);
  }
  /* Do any platform specific cleanup. */
  if (HSL_IFMGR_HWCB_CHECK(hw_l2_unregister))
    HSL_IFMGR_HWCB_CALL(hw_l2_unregister) (ifp);

  /* Free ifndex for this port. */
  if (p_hsl_if_db->cm_cb && p_hsl_if_db->cm_cb->cust_if_free_ifindex)
    (*p_hsl_if_db->cm_cb->cust_if_free_ifindex) (ifp->ifindex);

  /* Delete the interface and notify PMs */
  hsl_ifmgr_delete_interface(ifp, HSL_FALSE);

  HSL_FN_EXIT (0);
}
/*
  Delete L2 port with option of calling notifiers.
*/
void
hsl_ifmgr_L2_ethernet_delete (struct hsl_if *ifp,
                              int send_notification)
{
  int ret;

  HSL_FN_ENTER ();

  ret = HSL_IFMGR_IF_REF_DEC_AND_TEST (ifp);
  if (ret)
    hsl_ifmgr_L2_ethernet_delete2 (ifp, send_notification);

  HSL_FN_EXIT (STATUS_OK);
}

/*
  L2 port unregistration function by interface.
*/
int
hsl_ifmgr_L2_ethernet_unregister (struct hsl_if *ifp)
{
  HSL_FN_ENTER ();

  hsl_ifmgr_L2_ethernet_delete (ifp, 1);

  HSL_FN_EXIT (0);
}  


/* 
   L2 port unregistration function by ifindex.
*/
int
hsl_ifmgr_L2_ethernet_unregister2 (hsl_ifIndex_t ifindex)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  /* Check for routed ports. If the interface type is not 'vlan' then the
     L3 interface has to be deleted and the notifiers called for the L3
     interface.
     For pure L2 ports, the notifiers should be called for them directly. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return HSL_IFMGR_ERR_IF_NOT_FOUND;
  HSL_IFMGR_IF_REF_DEC (ifp);

  ret = hsl_ifmgr_L2_ethernet_unregister (ifp);

  HSL_FN_EXIT (ret);
}

/*
  Bind an interface to set of child ports 
*/
int
hsl_ifmgr_bindings_add (struct hsl_if *ifpp, int num,
                        hsl_ifIndex_t *ifindexes)
{
  int ret, i, j;
  struct hsl_if *ifpc;

  HSL_FN_ENTER ();

  for (i = 0; i < num; i++)
    {  
      ifpc = hsl_ifmgr_lookup_by_index (ifindexes[i]); 
      if (! ifpc)
        goto UNBIND;
      HSL_IFMGR_IF_REF_DEC (ifpc);

      /* Bind. */
      ret = hsl_ifmgr_bind2 (ifpp, ifpc); 
      if (ret < 0)
        goto UNBIND;
    }

  HSL_FN_EXIT (0);

 UNBIND:
  /* Unbind all current bindings */
  for (j = 0; j < i; j++)
    {  
      ifpc = hsl_ifmgr_lookup_by_index (ifindexes[i]);
      if (! ifpc)
        continue;
      HSL_IFMGR_IF_REF_DEC (ifpc);
      
      /* Unbind. */
      hsl_ifmgr_unbind2 (ifpp, ifpc);
    }
  
  HSL_FN_EXIT (HSL_IFMGR_ERR_BIND);
}

/*
  Remove all bindings. 
*/
int
hsl_ifmgr_bindings_remove_all (struct hsl_if *ifpp)
{
  struct hsl_if *ifp;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();

  /* Remove children bindings. */
  node = ifpp->children_list;
  while (node)
    {
      ifp = node->ifp;
      
      /* Unbind. */
      hsl_ifmgr_unbind2 (ifpp, ifp);

      node = node->next;
    }

  /* Remove parent bindings. */
  node = ifpp->parent_list;
  while (node)
    {
      ifp = node->ifp;

      /* Unbind. */
      hsl_ifmgr_unbind2 (ifp, ifpp);

      node = node->next;
    }

  HSL_FN_EXIT (0);
}
#ifdef HAVE_L3
/* 
   Create L3 interface.
*/
int
hsl_ifmgr_L3_create (char *ifname, u_char *hwaddr, int hwaddrlen,
                     int send_notification, void *data, struct hsl_if **ppifp)
{
  struct hsl_if *ifp;
  hsl_ifIndex_t idx;
  int ret;
  void *sysifp = NULL;
  void *osifp = NULL;
  int br_id, vid;
  int namelen;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_if_new ();
  if (!ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_MEMORY);

  memset (ifp, 0, sizeof(struct hsl_if));
  /* TODO check for existing interface. */

  /* Check name length. */
  if ((namelen = hsl_strlen (ifname)) > HSL_IFNAM_SIZE)
    return HSL_IFMGR_ERR_NAME;
  
  /* Set name. */
  memcpy (ifp->name, ifname, namelen);

  /* set type */
  ifp->type = HSL_IF_TYPE_IP;

  /* Set vid. */
  if (!memcmp (ifname, "vlan", 4))
    {
      sscanf (ifname, "vlan%d.%d", &br_id, &vid);
      ifp->u.ip.vid = vid;
    }
  else
    ifp->u.ip.vid = 1;

  /* Create interface in OS and attach TCP/IP stack to it. */
  if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_configure))
    osifp = HSL_IFMGR_STACKCB_CALL(os_l3_if_configure) (ifp, ifname, hwaddr, hwaddrlen, 
                                                       &idx);
  if (! osifp)
    {
      ret = HSL_IFMGR_ERR_OS_L3;
      goto CLEANUP;
    }

  /* Set OS info pointer. */
  ifp->os_info = osifp;

  /* Set ifindex. */
  ifp->ifindex = idx;

  /* Set acceptable packet types for this interface. */
  ifp->pkt_flags = 0;

  /* set mac address */
  if (hwaddr)
    {
      memcpy (ifp->u.ip.mac, hwaddr, hwaddrlen);
    }

  /* Create the system interface structure. */
  if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_configure))
    sysifp = HSL_IFMGR_HWCB_CALL(hw_l3_if_configure) (ifp, data);
  if (! sysifp)
    {
      ret = HSL_IFMGR_ERR_SYSTEM_L3;
      goto CLEANUP;
    }
      
  /* Set system info pointer. */
  ifp->system_info = sysifp;

  /* Set the default ARP ageing time. */
  ifp->u.ip.arpTimeout = HSL_DEFAULT_ARP_TIMEOUT;

  /* Create interface in ifmgr. */
  ret = hsl_ifmgr_create_interface(ifp, ppifp, send_notification, HSL_TRUE);
  if(ret != 0) 
    goto CLEANUP; 

  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Registered L3 intferce (%s) successfully!\n", ifname);
  HSL_FN_EXIT (0);
 
 CLEANUP:
  if (ifp->system_info)
    {
      if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_unconfigure))
        HSL_IFMGR_HWCB_CALL(hw_l3_if_unconfigure) (ifp);
    }

  if (osifp)
    {
      if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_unconfigure))
        HSL_IFMGR_STACKCB_CALL(os_l3_if_unconfigure) (ifp);
    }
  hsl_ifmgr_if_free (ifp);

  HSL_FN_EXIT (ret);
}

/* 
   Register L3 interface. 
*/
int
hsl_ifmgr_L3_register (char *ifname, u_char *hwaddr, int hwaddrlen,
                       void *data, struct hsl_if **ppifp)
{
  return hsl_ifmgr_L3_create (ifname, hwaddr, hwaddrlen, 1, data, ppifp);
}

/*
  Delete all addresses. 
*/
static int
_hsl_ifmgr_L3_delete_addresses (struct hsl_if *ifp, hsl_prefix_list_t **list)
{
  hsl_prefix_list_t *ucaddr, *nucaddr;

  HSL_FN_ENTER ();

  ucaddr = *list;
  while (ucaddr)
    {
      nucaddr = ucaddr->next;

      /* Remove from list so that the callback for IPv6 link-local address 
       * delete from the TCP/IP stack will not delete ucaddr from underneath
       */
      *list = nucaddr;
      
      /* Delete address from OS/HW. */
      _hsl_ifmgr_ip_address_delete_process (ifp, &ucaddr->prefix, HSL_TRUE);
      
      /* Free address. */
      oss_free (ucaddr, OSS_MEM_HEAP);
      
      ucaddr = nucaddr;
    }
  *list = NULL;

  HSL_FN_EXIT (0);
}

/*
  Delete L3 interface.
*/
void
hsl_ifmgr_L3_delete2 (struct hsl_if *ifp,
                      int send_notification)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node;
  struct hsl_if_list *node_next;

  HSL_FN_ENTER ();

  if (CHECK_FLAG (ifp->if_flags, HSL_IFMGR_IF_L3_DELETE))
    HSL_FN_EXIT (STATUS_OK);

  SET_FLAG (ifp->if_flags, HSL_IFMGR_IF_L3_DELETE);

  /* Delete and free addresses. */
  _hsl_ifmgr_L3_delete_addresses (ifp, &ifp->u.ip.ipv4.ucAddr);
#ifdef HAVE_IPV6
  _hsl_ifmgr_L3_delete_addresses (ifp, &ifp->u.ip.ipv6.ucAddr);
#endif /* HAVE_IPV6 */

  /* Unconfigure L3 interface from OS. */
  if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_unconfigure))
    HSL_IFMGR_STACKCB_CALL(os_l3_if_unconfigure) (ifp);

  /* Unconfigure L3 interface from system. */
  if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_unconfigure))
    HSL_IFMGR_HWCB_CALL(hw_l3_if_unconfigure) (ifp);

  /* Call notifiers to delete interface. */
  if (send_notification)
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFDELETE, ifp, NULL);

  HSL_IFMGR_LOCK;

  /* Delete this interface from childrens parent list. */
  node = ifp->children_list;
  while (node)
    {
      node_next = node->next;
      ifp2 = node->ifp;
      if (ifp2)
        _hsl_ifmgr_delete_from_list (&ifp2->parent_list, ifp);
      oss_free (node, OSS_MEM_HEAP);
      node = node_next;
    }
 
    ifp->children_list = NULL;
  
  /* Delete this interface from parent's children list. */
  node = ifp->parent_list;
  while (node)
    {
      node_next = node->next;
      ifp2 = node->ifp;


      _hsl_ifmgr_delete_from_list (&ifp2->children_list, ifp);

      /* If the last port for the aggregator interface is deleted, 
         the aggregator is inoperable. Send a flags change. */
      if (! ifp2->children_list)
        {
          /* Unset running flag. */
          ifp2->flags &= ~IFF_RUNNING;
          
          /* Call notifier for flags change. */
          _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFFLAGS, ifp2, 
                                   NULL);
        }

      oss_free (node, OSS_MEM_HEAP);
      node = node_next;
    }
  
  ifp->parent_list = NULL;
  HSL_IFMGR_UNLOCK;

  hsl_ifmgr_delete_interface (ifp,HSL_FALSE);

  HSL_FN_EXIT (STATUS_OK);
}

/*
  Delete L3 interface with notification option.
*/
void
hsl_ifmgr_L3_delete (struct hsl_if *ifp,
                     int send_notification)
{
  int ret;

  HSL_FN_ENTER ();

  if (! ifp)
    HSL_FN_EXIT (-1);

  ret = HSL_IFMGR_IF_REF_DEC_AND_TEST (ifp);
  if (ret)
    hsl_ifmgr_L3_delete2 (ifp, send_notification);

  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Unregister L3 interface. 
*/
int
hsl_ifmgr_L3_unregister (char *name, hsl_ifIndex_t ifindex)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return HSL_IFMGR_ERR_IF_NOT_FOUND;
  HSL_IFMGR_IF_REF_DEC (ifp);

  hsl_ifmgr_L3_delete (ifp, 1);

  HSL_FN_EXIT (0);
}

static int 
_hsl_ifmgr_create_dummy_l2_if(struct hsl_if *ifp, int speed, int duplex)
{
  struct hsl_if *dummy_ifp;

  HSL_FN_ENTER(); 

  /* Input parameters check. */ 
  if(!ifp)
    {
      HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_PARAM);
    }

  /* Create ifp. */
  dummy_ifp = hsl_ifmgr_if_new ();
  if (!dummy_ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_MEMORY);

  /* Copy interface configuration. */
  *dummy_ifp = *ifp;
  dummy_ifp->type = HSL_IF_TYPE_L2_ETHERNET;
  
  /* FIX ME Need to add os cb to read duplex/bw....*/

  dummy_ifp->u.l2_ethernet.mtu = ifp->u.ip.ipv4.mtu;
  if (speed)
    dummy_ifp->u.l2_ethernet.speed = speed;
  else
  dummy_ifp->u.l2_ethernet.speed = HSL_IF_BW_UNIT_100M;

  if (duplex)
    dummy_ifp->u.l2_ethernet.duplex = duplex;
  else
  dummy_ifp->u.l2_ethernet.duplex = HSL_IF_DUPLEX_FULL;
  dummy_ifp->u.l2_ethernet.autonego = HSL_IF_AUTONEGO_ENABLE;

  /* Set reference count. */
  HSL_IFMGR_IF_REF_SET (dummy_ifp, 1);

  hsl_ifmgr_bind2 (ifp, dummy_ifp);
  HSL_FN_EXIT(0);
}

/*
  Register L3 loopback interface.
*/
int
hsl_ifmgr_L3_loopback_register (char *name, hsl_ifIndex_t ifindex, int mtu, 
                                u_int32_t flags, void *osifp)
{
  struct hsl_if ifp;
  struct hsl_if *p_ifp;
  hsl_prefix_t pfx;
  int ret;
  int namelen;

  HSL_FN_ENTER ();

  /* Reset interface structure. */
  memset (&ifp,0,sizeof(struct hsl_if));
  memset (&pfx,0,sizeof(hsl_prefix_t));

  /* Check size of name */
  if ((namelen = hsl_strlen(name)) > HSL_IFNAM_SIZE)
    HSL_FN_EXIT(HSL_IFMGR_ERR_NAME);

  /* Set name. */
  memcpy (ifp.name, name, namelen);

  /* Set OS info pointer. */
  ifp.os_info = osifp;

  /* Set ifindex. */
  ifp.ifindex = ifindex;

  /* Set if property (CPU interface) */
  ifp.if_property = HSL_IF_CPU_ONLY_INTERFACE;

  /* Flags. */
  ifp.flags = flags;

  /* Set type as IP. */
  ifp.type = HSL_IF_TYPE_LOOPBACK;

  /* Create an interface in ifmgr */
  ret = hsl_ifmgr_create_interface(&ifp, &p_ifp, HSL_TRUE, HSL_FALSE);
  if (ret < 0)
    return ret;

  _hsl_ifmgr_create_dummy_l2_if(p_ifp, 0, 0);

  /* Add the default loopback addresses */
  pfx.family = AF_INET;
  pfx.prefixlen = HSL_LOOPBACK_PREFIXLEN;   
  pfx.u.prefix4= htonl (HSL_INADDR_LOOPBACK);   

  (void)hsl_ifmgr_os_ip_address_add (ifp.name, ifp.ifindex, &pfx, 0);

#ifdef HAVE_IPV6
  memset (&pfx,0,sizeof(hsl_prefix_t));
  pfx.family = AF_INET6;
  pfx.prefixlen = HSL_IPV6_LOOPBACK_PREFIXLEN;   
  ret = hsl_inet_pton (AF_INET6, HSL_IPV6_LOOPBACK_ADDR, (void *)&pfx.u.prefix6);
  if (ret < 0)
    return ret;

  (void)hsl_ifmgr_os_ip_address_add (ifp.name, ifp.ifindex, &pfx, 0);
#endif /* HAVE_IPV6 */

  HSL_FN_EXIT (ret);
}


/*
  Register L3 cpu interface.
*/
int
hsl_ifmgr_L3_cpu_if_register (char *name, hsl_ifIndex_t ifindex, int mtu, 
                             int speed,  u_int32_t duplex, u_int32_t flags,
                             char *hw_addr,void *osifp)
{
  struct hsl_if ifp;
  struct hsl_if *p_ifp;
  hsl_prefix_t pfx;
  int ret;
  int namelen;

  HSL_FN_ENTER ();

  /* Reset interface structure. */
  memset (&ifp,0,sizeof(struct hsl_if));

  /* Check size of name */
  if ((namelen = hsl_strlen(name)) > HSL_IFNAM_SIZE)
    HSL_FN_EXIT(HSL_IFMGR_ERR_NAME);

  /* Set name. */
  memcpy (ifp.name, name, namelen);
  memcpy (ifp.mapped_name, name, namelen);

  /* Set OS info pointer. */
  ifp.os_info = osifp;

  /* Set if property (CPU interface) */
  ifp.if_property = HSL_IF_CPU_ONLY_INTERFACE;

  /* Set ifindex. */
  ifp.ifindex = ifindex;

  /* Flags. */
  ifp.flags = flags;

  /* Mtu */
  ifp.u.ip.ipv4.mtu = mtu;

  /* Set type as IP. */
  if(flags & IFF_LOOPBACK) 
    ifp.type = HSL_IF_TYPE_LOOPBACK;
  else
    {
      if (strncmp (name, "Tunnel", 6) == 0)
        ifp.type = HSL_IF_TYPE_TUNNEL;
      else
      ifp.type = HSL_IF_TYPE_IP;
      memcpy(ifp.u.ip.mac,hw_addr,HSL_ETHER_ALEN);
    }

  /* Create an interface in ifmgr */
  ret = hsl_ifmgr_create_interface(&ifp, &p_ifp, HSL_TRUE, HSL_FALSE);
  if(ret < 0)
    HSL_FN_EXIT (ret);

  /* Create a dummy l2 interface attached to cpu one. */
  _hsl_ifmgr_create_dummy_l2_if(p_ifp, speed, duplex);

  /* Set loopback addresses. */
  if(flags & IFF_LOOPBACK) 
  {
     /* Add the default loopback addresses */
     pfx.family = AF_INET;
     pfx.prefixlen = HSL_LOOPBACK_PREFIXLEN;   
     pfx.u.prefix4= htonl (HSL_INADDR_LOOPBACK);   

     (void)hsl_ifmgr_os_ip_address_add (ifp.name, ifp.ifindex, &pfx, 0);

#ifdef HAVE_IPV6
     memset (&pfx,0,sizeof(hsl_prefix_t));
     pfx.family = AF_INET6;
     pfx.prefixlen = HSL_IPV6_LOOPBACK_PREFIXLEN;   
     ret = hsl_inet_pton (AF_INET6, HSL_IPV6_LOOPBACK_ADDR, (void *)&pfx.u.prefix6);
     if (ret < 0)
       return ret;

    (void)hsl_ifmgr_os_ip_address_add (ifp.name, ifp.ifindex, &pfx, 0);
#ifdef PNE_VERSION_2_2
     memset (&pfx,0,sizeof(hsl_prefix_t));
     pfx.family = AF_INET6;
     pfx.prefixlen = HSL_IPV6_LINKLOCAL_LOOPBACK_PREFIXLEN;
     ret = hsl_inet_pton (AF_INET6, HSL_IPV6_LINKLOCAL_LOOPBACK_ADDR, (void *)&pfx.u.prefix6);
     if (ret < 0)
       return ret;

    (void)hsl_ifmgr_os_ip_address_add (ifp.name, ifp.ifindex, &pfx, 0);
#endif /* PNE_VERSION_2_2 */
#endif /* HAVE_IPV6 */
  }
  HSL_FN_EXIT (ret);
}
#endif /* HAVE_L3 */
/* 
   Link down for a L2 interface. To be called from the link scan only.
*/
int
hsl_ifmgr_L2_link_down (struct hsl_if *ifp, u_int32_t speed, u_int32_t duplex)
{
  HSL_FN_ENTER ();
  HSL_IFMGR_IF_REF_INC (ifp);

  /* Speed. */
  ifp->u.l2_ethernet.speed = speed;

  /* Duplex. */
  ifp->u.l2_ethernet.duplex = 0;
  
  if(!((ifp->flags & IFF_RUNNING) == IFF_RUNNING))
    {
      /* Interface was already down. -> no action needed. */ 
      goto DONE;
    }

  /* Unset running flag. */
  ifp->flags &= ~IFF_RUNNING;

  /* Link went down. Update parent interfaces. */
  _hsl_ifmgr_update_op_count (ifp, HSL_IF_DEC_OP_COUNT, 1, HSL_TRUE);

 DONE:
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Link up for a L2 interface. To be called from the link scan only.
*/
int
hsl_ifmgr_L2_link_up (struct hsl_if *ifp, u_int32_t speed, u_int32_t duplex)
{
  HSL_FN_ENTER ();
  HSL_IFMGR_IF_REF_INC (ifp);

  /* Speed. */
  ifp->u.l2_ethernet.speed = speed;

  /* Duplex. */
  ifp->u.l2_ethernet.duplex= duplex;

  if(((ifp->flags & IFF_RUNNING) == IFF_RUNNING))
    {
      /* Interface was already up. -> no action needed. */ 
      goto DONE;
    }

  /* Set  running flag. */
  ifp->flags |= IFF_RUNNING;

  /* Link went up. Update parent interfaces. */
  _hsl_ifmgr_update_op_count (ifp, HSL_IF_INC_OP_COUNT, 1, HSL_TRUE);

 DONE:
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (STATUS_OK);
}

/* 
   Set flags for a interface given a ifp.
*/
int
hsl_ifmgr_set_flags2 (struct hsl_if *ifp, u_int32_t flags)
{
  struct hsl_if *ifpc;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();

  if ( (node = ifp->children_list) )
    {
      ifpc = node->ifp;
      if ( (ifpc) && (ifpc->type == ifp->type) )
        {
          /* ifp is an aggregator apply flags to all its components */
#if 0     /* Temp don't update flags. */ 
          while (ifpc)
            {
              hsl_ifmgr_set_flags(ifpc->name, ifpc->ifindex, flags);
              node = node->next;
              if (!node)
                ifpc = NULL;
              else
                ifpc = node->ifp;
            }
#endif /* 0 */
          /* Change the aggregator flags in OS */
          if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
            {
              /* Set flags in OS. */
              if (HSL_IFMGR_STACKCB_CHECK(os_l2_if_flags_set))
                HSL_IFMGR_STACKCB_CALL(os_l2_if_flags_set) (ifp, flags);
            }
          else  
            {
              /* Set flags in OS. */
              if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_set))
                HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_set) (ifp, flags);
            }
          /* Set new flags in interface. */
          ifp->flags |= flags;
            
          return 0;
        }
    }

  if (ifp->type == HSL_IF_TYPE_LOOPBACK)
      flags |= IFF_RUNNING;

  /* Set new flags in interface. */
  ifp->flags |= flags;

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      /* Set flags in OS. */
      if (HSL_IFMGR_STACKCB_CHECK(os_l2_if_flags_set))
        HSL_IFMGR_STACKCB_CALL(os_l2_if_flags_set) (ifp, flags);

      /* Set flags in HW. */
      if (HSL_IFMGR_HWCB_CHECK(hw_l2_if_flags_set))
        HSL_IFMGR_HWCB_CALL(hw_l2_if_flags_set) (ifp, flags);
    }
  else if (ifp->type == HSL_IF_TYPE_IP) 
    {
      /* Set flags in OS. */
      if ((!strncmp(ifp->name,"vlan",4)) && ifp->operCnt)
        {
          if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_set))
             HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_set) (ifp, flags|IFF_RUNNING);

          ifp->flags |= IFF_RUNNING;
        }
      else if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_set))
        HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_set) (ifp, flags);

      /* Set flags in HW. */
      if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_flags_set))
        (*(p_hsl_if_db->hw_cb->hw_l3_if_flags_set)) (ifp, flags);

      /* In case of directly mapped L2 interfaces, set flags for them too. */
      ifpc = hsl_ifmgr_get_first_L2_port (ifp);
      if (ifpc)
        {
          if (CHECK_FLAG (ifpc->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
            {
              /* Set new flags in interface. */
              ifpc->flags |= flags;
              
              /* Set flags in OS. */
              if (HSL_IFMGR_STACKCB_CHECK(os_l2_if_flags_set))
                HSL_IFMGR_STACKCB_CALL(os_l2_if_flags_set) (ifpc, flags);
              
              /* Set flags in HW. */
              if (HSL_IFMGR_HWCB_CHECK(hw_l2_if_flags_set))
                HSL_IFMGR_HWCB_CALL(hw_l2_if_flags_set) (ifpc, flags);

            }
          HSL_IFMGR_IF_REF_DEC (ifpc);
        }
    }
    else if(ifp->type == HSL_IF_TYPE_LOOPBACK)
      {
        /* Set flags in OS. */
        if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_set))
          HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_set) (ifp, flags);
      }
    

    if (!strncmp(ifp->name,"vlan",4))
      hsl_ifmgr_send_notification (HSL_IF_EVENT_IFFLAGS, ifp, NULL);

  HSL_FN_EXIT (0);
}
 
/*
  Set flags for a interface. 
*/
int 
hsl_ifmgr_set_flags (char *name, hsl_ifIndex_t ifindex, u_int32_t flags)
{
  struct hsl_if *ifp;
  int ret;
  
  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
  

  ret = hsl_ifmgr_set_flags2 (ifp, flags);
  
  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);
}

/*
  Unset flags for a interface given a ifp.
*/
int
hsl_ifmgr_unset_flags2 (struct hsl_if *ifp, u_int32_t flags)
{
  struct hsl_if *ifpc;
  struct hsl_if_list *node;
  int lock;

  lock = 1;

  HSL_IFMGR_LOCK;

  if ( (node = ifp->children_list) )
    {
      ifpc = node->ifp;

      lock = 0;

      HSL_IFMGR_UNLOCK;
        
      if ( (ifpc) && (ifpc->type == ifp->type) )
        {
          /* ifp is an aggregator apply flags to all its components */
#if 0     /* Temp don't update flags. */ 
          while (ifpc)
            {
              hsl_ifmgr_unset_flags(ifpc->name, ifpc->ifindex, flags);
              node = node->next;
              if (!node)
                ifpc = NULL;
              else
                ifpc = node->ifp;
            }
#endif
          /* Change the aggregator flags in OS */
          if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
            {
              /* Set flags in OS. */
              if (HSL_IFMGR_STACKCB_CHECK(os_l2_if_flags_unset))
                HSL_IFMGR_STACKCB_CALL(os_l2_if_flags_unset) (ifp, flags);
            }
          else  
            {
              /* Set flags in OS. */
              if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_unset))
                HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_unset) (ifp, flags);
            }
          /* Set new flags in interface. */
          ifp->flags &= ~flags;
          return 0;
        }
    }

  if (lock)
    {
      HSL_IFMGR_UNLOCK;
    }
  
  if (ifp->type == HSL_IF_TYPE_LOOPBACK)
    flags |= IFF_RUNNING;

  /* Unset flags in interface. */
  ifp->flags &= ~flags;

  if (ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      /* Set flags in OS. */
      if (HSL_IFMGR_STACKCB_CHECK(os_l2_if_flags_unset))
        HSL_IFMGR_STACKCB_CALL(os_l2_if_flags_unset) (ifp, flags);

      /* Set flags in HW. */
      if (HSL_IFMGR_HWCB_CHECK(hw_l2_if_flags_unset))
        (*(p_hsl_if_db->hw_cb->hw_l2_if_flags_unset)) (ifp, flags);
    }
  else
    {
      if ((!strncmp(ifp->name,"vlan",4)))
        {
          if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_unset))
            HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_unset) (ifp, flags|IFF_RUNNING);

          ifp->flags &= ~IFF_RUNNING;
        }
      /* Set flags in OS. */
      else if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_flags_unset))
        HSL_IFMGR_STACKCB_CALL(os_l3_if_flags_unset) (ifp, flags);

      /* Set flags in HW. */
      if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_flags_unset))
        (*(p_hsl_if_db->hw_cb->hw_l3_if_flags_unset)) (ifp, flags);

      /* In case of directly mapped L2 interfaces, unset flags for them too. */
      ifpc = hsl_ifmgr_get_first_L2_port (ifp);
      if (ifpc)
        {
          if (CHECK_FLAG (ifpc->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED))
            {
              /* Set flags in OS. */
              if (HSL_IFMGR_STACKCB_CHECK(os_l2_if_flags_unset))
                HSL_IFMGR_STACKCB_CALL(os_l2_if_flags_unset) (ifpc, flags);
              
              /* Set flags in HW. */
              if (HSL_IFMGR_HWCB_CHECK(hw_l2_if_flags_unset))
                HSL_IFMGR_HWCB_CALL(hw_l2_if_flags_unset) (ifpc, flags);

              /* Unset flags in interface. */
              ifpc->flags &= ~flags;

            }
          HSL_IFMGR_IF_REF_DEC (ifpc);
        }
        /* Since vlan has no mechanism to notify the IFF_RUNNING to NSM do it manually */
        if (!strncmp(ifp->name,"vlan",4))
        {
          ifp->flags &= ~IFF_RUNNING;
          hsl_ifmgr_send_notification (HSL_IF_EVENT_IFFLAGS, ifp, NULL);
        }
    }

  _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFFLAGS, ifp, NULL);
  HSL_FN_EXIT (0);
}

/*
  Unset flags for a interface.
*/
int 
hsl_ifmgr_unset_flags (char *name, hsl_ifIndex_t ifindex, u_int32_t flags)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  ret = hsl_ifmgr_unset_flags2 (ifp, flags);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);
}

#ifdef HAVE_L3
/* 
   Add a interface address. 
*/
static int
_hsl_ifmgr_ip_address_add (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix,
                           u_char flags, HSL_BOOL send_notification, 
                           HSL_BOOL os_cb)
{
  struct hsl_if *ifp;
  hsl_prefix_list_t *ucaddrnew = NULL, *ucaddr;
  HSL_BOOL os_delete = HSL_FALSE;
  int ret;

  ret = 0;

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  /* Check duplicates. */
  if (prefix->family == AF_INET)
    {
      if (ifp->u.ip.ipv4.ucAddr)
        {
          ucaddr = ifp->u.ip.ipv4.ucAddr;
          while (ucaddr)
            {
              if (hsl_prefix_same (&ucaddr->prefix, prefix))
                return 0;

              /* Goto next address. */
              ucaddr = ucaddr->next;
            }
        }
    }
#ifdef HAVE_IPV6
  else 
    {
      if (ifp->u.ip.ipv6.ucAddr)
        {
          ucaddr = ifp->u.ip.ipv6.ucAddr;
          while (ucaddr)
            {
              if (hsl_prefix_same (&ucaddr->prefix, prefix))
                return 0;

              /* Goto next address. */
              ucaddr = ucaddr->next;
            }
        }
    }
#endif /* HAVE_IPV6 */
  
  /* Allocate new address. */
  ucaddrnew = oss_malloc (sizeof (hsl_prefix_list_t), OSS_MEM_HEAP);
  if (! ucaddrnew)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_IFMGR_ERR_MEMORY);
    }

  /* Copy prefix. */
  memcpy (&ucaddrnew->prefix, prefix, sizeof (hsl_prefix_t));

  /* Set flags. */
  ucaddrnew->flags = flags;

  /* XXX: Important!!!
   * Add the address to the list before OS call so that IPv6 link-local
   * address add callback finds the address in the list and does not
   * try to add it the hardware twice.
   * DO NOT CHANGE THIS ORDER.
   */
  /* Add it to the list of addresses. */
  if (prefix->family == AF_INET)
    {
      if (ifp->u.ip.ipv4.ucAddr)
        {
          /* Add it to head. */
          ucaddrnew->next = ifp->u.ip.ipv4.ucAddr;
        }
      /* Point head to new. */
      ifp->u.ip.ipv4.ucAddr = ucaddrnew;
 
      /* Increment count. */
      ifp->u.ip.ipv4.nucAddr++;
    }
#ifdef HAVE_IPV6
  else 
    {
      if (ifp->u.ip.ipv6.ucAddr)
        {
          /* Add it to head. */
          ucaddrnew->next = ifp->u.ip.ipv6.ucAddr;
        }
      /* Point head to new. */
      ifp->u.ip.ipv6.ucAddr = ucaddrnew;
                                                                                                                             
      /* Increment count. */
      ifp->u.ip.ipv6.nucAddr++;
    }
#endif /* HAVE_IPV6 */

  /* Set the interface address in the OS. */
  if ((os_cb == HSL_TRUE) && 
      HSL_IFMGR_STACKCB_CHECK(os_l3_if_address_add))
    {
      ret = HSL_IFMGR_STACKCB_CALL(os_l3_if_address_add) (ifp, prefix, flags);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Couldn't add address to OS %d\n", ret);
          ret = HSL_IFMGR_ERR_IP_ADDRESS;
          goto ERR;
        }
    }

  /* Set the interface address on the hardware. */
  if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_address_add))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_l3_if_address_add) (ifp, prefix, flags);
      if (ret < 0)
        {
          os_delete = HSL_TRUE;
          ret = HSL_IFMGR_ERR_IP_ADDRESS;
          goto ERR;
        }
    }

  /* Add in the new connected address in the FIB first. */
  hsl_fib_add_connected (prefix, ifp);


  /* Call notifiers. */
  if(HSL_TRUE == send_notification)
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFNEWADDR, ifp, prefix);

  HSL_IFMGR_IF_REF_DEC (ifp);

  return 0;

 ERR:
  if (ucaddrnew)
    {
      /* Remove it from the list of addresses. */
      if (prefix->family == AF_INET)
        {
          /* Point head to next. */
          ifp->u.ip.ipv4.ucAddr = ucaddrnew->next;

          /* Decrement count. */
          ifp->u.ip.ipv4.nucAddr--;
        }
#ifdef HAVE_IPV6
      else 
        {
          /* Point head to next. */
          ifp->u.ip.ipv6.ucAddr = ucaddrnew->next;

          /* Increment count. */
          ifp->u.ip.ipv6.nucAddr--;
        }
#endif /* HAVE_IPV6 */

      oss_free (ucaddrnew, OSS_MEM_HEAP);
    }

  /* XXX: Important! Make the OS delete call after removing the new
   * entry from the list above otherwise the OS callback for IPv6 linklocal
   * address will remove the entry in another function and we will end up
   * freeing the same memory twice in above oss_free().
   */
  /* Rolling back the interface address in OS. */
  if ((os_delete == HSL_TRUE) && (os_cb == HSL_TRUE) && 
      HSL_IFMGR_STACKCB_CHECK(os_l3_if_address_delete))
    HSL_IFMGR_STACKCB_CALL(os_l3_if_address_delete) (ifp, prefix);

  HSL_IFMGR_IF_REF_DEC (ifp);

  return ret;
}


/* 
   Add an IPV4 interface address. 
*/
int
hsl_ifmgr_ipv4_address_add (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix,
                            u_char flags)
{
  int ret;

  HSL_FN_ENTER ();

  /* Just process IPv4 addresses. */
  if (prefix->family != AF_INET)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_ADDRESS);

  ret = _hsl_ifmgr_ip_address_add (name, ifindex, prefix, flags, HSL_FALSE,
                                   HSL_TRUE);
  HSL_FN_EXIT (ret);
}

#ifdef HAVE_IPV6
/*
  Add an IPV6 interface address.
*/
int
hsl_ifmgr_ipv6_address_add (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix,
                            u_char flags)
{
  int ret;

  HSL_FN_ENTER ();
                                                                                                                             
  /* Just process IPv6 addresses. */
  if (prefix->family != AF_INET6)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_ADDRESS);
                                                                                                                             
  ret = _hsl_ifmgr_ip_address_add (name, ifindex, prefix, flags, HSL_FALSE,
                                   HSL_TRUE);
  HSL_FN_EXIT (ret);
}
#endif /* HAVE_IPV6 */

/*
  Address deletion process.
*/
static int
_hsl_ifmgr_ip_address_delete_process (struct hsl_if *ifp, hsl_prefix_t *prefix, HSL_BOOL os_cb)
{
  HSL_FN_ENTER ();

  /* Delete the connected address in the FIB first. */
#ifdef HAVE_IPNET
  hsl_fib_delete_connected (prefix, ifp, os_cb);
#else /* HAVE_IPNET */
  hsl_fib_delete_connected (prefix, ifp, HSL_TRUE);
#endif /* HAVE_IPNET */


  /* Unset the interface address in the OS. */
  if ((os_cb == HSL_TRUE) && 
      HSL_IFMGR_STACKCB_CHECK(os_l3_if_address_delete))
    HSL_IFMGR_STACKCB_CALL(os_l3_if_address_delete) (ifp, prefix);

  /* Unset the interface address on the hardware. */
  if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_address_delete))
    HSL_IFMGR_HWCB_CALL(hw_l3_if_address_delete) (ifp, prefix);

  HSL_FN_EXIT (0);
}

/* 
   Delete a interface address given a ifp. 
*/
static int
_hsl_ifmgr_ip_address_delete2 (struct hsl_if *ifp, hsl_prefix_t *prefix, HSL_BOOL send_notification,
                               HSL_BOOL os_cb)
{
  hsl_prefix_list_t *ucaddr = NULL, *prev;
  struct _hsl_ip_if *ipif;
  int found = 0;

  HSL_FN_ENTER ();

  if (prefix->family == AF_INET)
    {
      ipif = &ifp->u.ip.ipv4;
      ucaddr = ipif->ucAddr;
    }
#ifdef HAVE_IPV6
  else if (prefix->family == AF_INET6)
    {
      ipif = &ifp->u.ip.ipv6;
      ucaddr = ipif->ucAddr;
    }
#endif /* HAVE_IPV6 */

  if (ucaddr)
    {
      prev = NULL;
      while (ucaddr)
        {
          if (hsl_prefix_same (&ucaddr->prefix, prefix))
            {
              if (prev)
                {
                  prev->next = ucaddr->next;
                  oss_free (ucaddr, OSS_MEM_HEAP);
                  found = 1;
                  break;
                }
              else
                {
                  ipif->ucAddr = ucaddr->next;
                  oss_free (ucaddr, OSS_MEM_HEAP);
                  found = 1;
                  break;
                }
            }

          prev = ucaddr;
          ucaddr = ucaddr->next;
        }

      if (! found)
        {
          HSL_FN_EXIT (HSL_IFMGR_ERR_IP_ADDRESS);
        }
    }
  else
    HSL_FN_EXIT (HSL_IFMGR_ERR_IP_ADDRESS);

  /* Decrement count. */
  ipif->nucAddr--;

  /* Process the address delete in OS/HW. */
  _hsl_ifmgr_ip_address_delete_process (ifp, prefix, os_cb);

  /* Call notifiers. */
  if(HSL_TRUE == send_notification)
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFDELADDR, ifp, prefix);

  HSL_FN_EXIT (0);
}

/* 
   Delete a interface address. 
*/
static int
_hsl_ifmgr_ip_address_delete (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix, HSL_BOOL send_notification, HSL_BOOL os_cb)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  /* Delete address. */
  ret = _hsl_ifmgr_ip_address_delete2 (ifp, prefix, send_notification, os_cb);
   
  /* Decrement refcnt. */
  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);
}


/* 
   Delete a interface address. 
*/
int
hsl_ifmgr_ipv4_address_delete (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix)
{
  int ret;

  HSL_FN_ENTER ();

  /* Just process IPv4 address. */
  if (prefix->family != AF_INET)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_ADDRESS);

  ret = _hsl_ifmgr_ip_address_delete (name, ifindex, prefix, HSL_FALSE, HSL_TRUE);
  HSL_FN_EXIT (ret);
}


#ifdef HAVE_IPV6
/*
  Delete an interface IPV6 address.
*/
int
hsl_ifmgr_ipv6_address_delete (char *name, hsl_ifIndex_t ifindex, hsl_prefix_t *prefix)
{
  int ret;
           
  HSL_FN_ENTER ();
                                                                                                                             
  /* Just process IPv6 address. */
  if (prefix->family != AF_INET6)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_ADDRESS);
           
  ret = _hsl_ifmgr_ip_address_delete (name, ifindex, prefix, HSL_FALSE, HSL_TRUE);
  HSL_FN_EXIT (ret);
}
#endif /* HAVE_IPV6 */

/*
  Bind an interface to a FIB.
*/
int
hsl_ifmgr_if_bind_fib (hsl_ifIndex_t ifindex, hsl_fib_id_t fib_id)
{
  int ret = 0;
  struct hsl_if *ifp;
           
  HSL_FN_ENTER ();
                                                                                                                             
  /* Validate FIB */
  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_FIB_ID);

  if (! HSL_FIB_VALID (fib_id))
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_FIB);
           
  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  /* Only L3 interfaces can be bound */
  if (ifp->type != HSL_IF_TYPE_IP)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_MODE);
    }

  /* If the interface is already bound to the same FIB, ignore */
  if (ifp->fib_id == fib_id)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (STATUS_OK);
    }

  /* Bind the interface in the OS. */
  if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_bind_fib))
    {
      ret = HSL_IFMGR_STACKCB_CALL(os_l3_if_bind_fib) (ifp, fib_id);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Interface %s bind to FIB %d falied(%d) in OS\n", ifp->name, fib_id, ret);

          HSL_IFMGR_IF_REF_DEC (ifp);

          HSL_FN_EXIT (ret);
        }
    }

  /* Bind the interface in the HW. */
  if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_bind_fib))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_l3_if_bind_fib) (ifp, fib_id);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Interface %s bind to FIB %d falied(%d) in HW\n", ifp->name, fib_id, ret);
          goto ERR;
        }
    }

  ifp->fib_id = fib_id;

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);
ERR:
  /* Unbind the interface in the OS. */
  if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_unbind_fib))
      HSL_IFMGR_STACKCB_CALL(os_l3_if_unbind_fib) (ifp, fib_id);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);
}

/*
  Unnind an interface from a FIB.
*/
int
hsl_ifmgr_if_unbind_fib (hsl_ifIndex_t ifindex, hsl_fib_id_t fib_id)
{
  int ret = 0;
  struct hsl_if *ifp;
           
  HSL_FN_ENTER ();
                                                                                                                             
  /* Validate FIB */
  if (! HSL_FIB_ID_VALID (fib_id))
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_FIB_ID);

  if (! HSL_FIB_VALID (fib_id))
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_FIB);
           
  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  /* Only L3 interfaces can be bound */
  if (ifp->type != HSL_IF_TYPE_IP)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_MODE);
    }

  /* Check if the interface is not bound to the same FIB */
  if (ifp->fib_id != fib_id)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT (HSL_IFMGR_ERR_IF_FIB_MISMATCH);
    }

  /* Bind the interface in the OS. */
  if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_unbind_fib))
    {
      ret = HSL_IFMGR_STACKCB_CALL(os_l3_if_unbind_fib) (ifp, fib_id);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Interface %s unbind from FIB %d falied(%d) in OS\n", ifp->name, fib_id, ret);

          HSL_IFMGR_IF_REF_DEC (ifp);

          HSL_FN_EXIT (ret);
        }
    }

  /* Bind the interface in the HW. */
  if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_unbind_fib))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_l3_if_unbind_fib) (ifp, fib_id);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Interface %s unbind from FIB %d falied(%d) in HW\n", ifp->name, fib_id, ret);
          goto ERR;
        }
    }

  ifp->fib_id = HSL_INVALID_FIB_ID;

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);

ERR:
  /* Rebind the interface in the OS. */
  if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_bind_fib))
      HSL_IFMGR_STACKCB_CALL(os_l3_if_bind_fib) (ifp, fib_id);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);
}
#endif /* HAVE_L3 */

/* 
   Register OS specific callbacks. 
*/
int
hsl_ifmgr_set_os_callbacks (struct hsl_ifmgr_os_callbacks *cb)
{
  HSL_FN_ENTER ();

  if (p_hsl_if_db->os_cb)
    return HSL_IFMGR_ERR_OS_CB_ALREADY_REGISTERED;

  p_hsl_if_db->os_cb = cb;

  HSL_FN_EXIT (0);
}

/* 
   Unregister OS specific callbacks. 
*/
int
hsl_ifmgr_unset_os_callbacks (void)
{
  HSL_FN_ENTER ();

  if (p_hsl_if_db->os_cb)
    p_hsl_if_db->os_cb = NULL;

  HSL_FN_EXIT (0);
}

/* 
   Register HW specific callbacks. 
*/
int
hsl_ifmgr_set_hw_callbacks (struct hsl_ifmgr_hw_callbacks *cb)
{
  HSL_FN_ENTER ();

  if (p_hsl_if_db->hw_cb)
    return HSL_IFMGR_ERR_HW_CB_ALREADY_REGISTERED;

  p_hsl_if_db->hw_cb = cb;

  HSL_FN_EXIT (0);
}

/*
  Unregister HW specific callbacks. 
*/
int
hsl_ifmgr_unset_hw_callbacks (void)
{
  HSL_FN_ENTER ();

  if (p_hsl_if_db->hw_cb)
    p_hsl_if_db->hw_cb = NULL;

  HSL_FN_EXIT (0);
}
#ifdef HAVE_L3
/*
  Set ARP AGEING TIMEOUT for a interface.
*/
int
hsl_ifmgr_set_arp_ageing_timeout (hsl_ifIndex_t ifindex, int arp_ageing_timeout)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    return HSL_IFMGR_ERR_IF_NOT_FOUND;

  /* Set ARP AGEING TIMEOUT in interface manager. */
  if (ifp->type == HSL_IF_TYPE_IP)
    {
      /* Set ARP AGEING TIMER in interface manager */
      ifp->u.ip.arpTimeout = arp_ageing_timeout;
    }

  /* Call notifiers. */
  _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFARPAGEINGTIMEOUT, ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}
#endif /* HAVE_L3 */
#ifdef HAVE_L3
/*
  Set port to a ROUTER port. The input parameters has to be the L2 port. 
*/
int
hsl_ifmgr_set_router_port (struct hsl_if *ifp, void *data, struct hsl_if **ppifp, HSL_BOOL send_notification)
{
  struct hsl_if *ifp2;
  int ret;

  HSL_FN_ENTER ();

  if (ifp->type != HSL_IF_TYPE_L2_ETHERNET)
    {
      HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_MODE);
    }


  ret = hsl_ifmgr_L3_create (ifp->name, ifp->u.l2_ethernet.mac, 6, 0, data, &ifp2);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Couldn't set port for routing\n");
      HSL_FN_EXIT (ret);
    }

  if (ppifp)
    *ppifp = ifp2;

  SET_FLAG (ifp->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED);

  /* Bind the L2 and L3 interface. */
  hsl_ifmgr_bind2 (ifp2, ifp);

  /* Set acceptable pkt types as none. */
  hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_ARP | HSL_IF_PKT_RARP | HSL_IF_PKT_BCAST| HSL_IF_PKT_MCAST  | HSL_IF_PKT_IP | HSL_IF_PKT_LACP | HSL_IF_PKT_EAPOL | HSL_IF_PKT_LLDP | HSL_IF_PKT_EFM);

  /* Set switching type for this port. */
  if (HSL_IFMGR_HWCB_CHECK(hw_set_switching_type))
    HSL_IFMGR_HWCB_CALL(hw_set_switching_type) (ifp, HSL_IF_SWITCH_L3);

  /* Send notification for L3 port addition. */
  if(send_notification)
    hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEW, ifp2, NULL);

  HSL_IFMGR_UNLOCK;

  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Set interface %s(%d) to router port successfully!\n", ifp->name, ifp->ifindex);
  HSL_FN_EXIT (0);
}
#endif  /* HAVE_L3 */

#ifdef HAVE_L2
/* 
   L2 port initialization routine.
*/
static int 
hsl_ifmgr_init_l2_port(struct hsl_if *ifp)
{

  HSL_FN_ENTER();

  if(!ifp)
    HSL_FN_EXIT(STATUS_ERROR);     

  /* Set acceptable pkt types as none. */
  hsl_ifmgr_unset_acceptable_packet_types (ifp, HSL_IF_PKT_ALL);

  /* Set acceptable pkt type to L2 */
  hsl_ifmgr_set_acceptable_packet_types (ifp, HSL_IF_PKT_L2);

  /* Set switching type for this port. */
  if (HSL_IFMGR_HWCB_CHECK(hw_set_switching_type))
    HSL_IFMGR_HWCB_CALL(hw_set_switching_type) (ifp, HSL_IF_SWITCH_L2_L3);

  HSL_FN_EXIT(STATUS_OK);     
}

/*
  Init all ports as L2.
*/
int
hsl_ifmgr_init_policy_l2 (void)
{
  struct hsl_if       **ifp_arr;                  /* Array of all L2 interfaces. */
  struct hsl_if       *ifp;                       /* Inteface information.       */
  int ret;                                        /* General operation status.   */
  u_int16_t index;                                /* Index for iteration.        */
  u_int16_t count = 0;                            /* Interface size.             */
  u_int8_t policy;

  HSL_FN_ENTER();

  /* Get current policy. */
  policy = hsl_ifmgr_get_policy ();
  if (policy == HSL_IFMGR_IF_INIT_POLICY_L2)
    HSL_FN_EXIT (0);
 
  /* Lock interface manager. */
  HSL_IFMGR_LOCK;
 
  /* Create a snapshot of all L2 interfaces (ports). */
  ret = hsl_ifmgr_get_L2_array(&ifp_arr, &count);
  if (ret < 0 )
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Failed to read l2 interface array.\n");
      HSL_IFMGR_UNLOCK;
      HSL_FN_EXIT(STATUS_ERROR);
    }

  /* Enable L2 switching on every L2 interface saved in the snapshot. */
  for (index = 0 ;index < count; index++)
    {
      ifp  = ifp_arr[index];
      hsl_ifmgr_init_l2_port(ifp);
    }

  oss_free(ifp_arr,OSS_MEM_HEAP);

  HSL_IFMGR_UNLOCK;

  /* Set policy. */
  hsl_ifmgr_set_policy (HSL_IFMGR_IF_INIT_POLICY_L2);

  HSL_FN_EXIT(STATUS_OK);
}


/*
  Set port as a SWITCH port. The input port has to be a IP interface.
*/
int
hsl_ifmgr_set_switch_port (struct hsl_if *ifp, struct hsl_if **ppifp, HSL_BOOL send_notification)
{
  struct hsl_if *ifpc;
  struct hsl_if_list *node;

  HSL_FN_ENTER ();

  if (ifp->type != HSL_IF_TYPE_IP)
    {
      HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_MODE);
    }

  HSL_IFMGR_LOCK;

  /* Check of multiple children exist. Sanity check. */
  node = ifp->children_list;
  if (node)
    {
      if (node->next)
        {
          HSL_IFMGR_UNLOCK;
          HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_MODE);
        }

      ifpc = node->ifp;

      /* Unbind the interfaces. */
      hsl_ifmgr_unbind2 (ifp, ifpc);
      
      UNSET_FLAG (ifpc->if_flags, HSL_IFMGR_IF_DIRECTLY_MAPPED);
  
      if (ppifp)
        *ppifp = ifpc;

      /* Prep l2 port for switching. */
      hsl_ifmgr_init_l2_port(ifpc);

      /* Send notification for L2 port addition. */
      if(send_notification) 
        hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEW, ifpc, NULL);
    }

  HSL_IFMGR_UNLOCK;

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}
#endif /* HAVE_L2 */

/* Get interface MAC counters. */ 
int 
hsl_ifmgr_get_if_counters(hsl_ifIndex_t ifindex, struct hal_if_counters *cntrs)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: + %s \n", __FUNCTION__);


  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK(hw_if_get_counters))
     ret = HSL_IFMGR_HWCB_CALL(hw_if_get_counters) (ifp);

  memcpy(cntrs,&ifp->mac_cntrs,sizeof(struct hal_if_counters));

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: - %s \n", __FUNCTION__);

  HSL_FN_EXIT (0);
}

/* clear interface interface counters. */ 
int 
hsl_ifmgr_if_clear_counters(hsl_ifIndex_t ifindex)
{
  struct hsl_if *ifp;
  int ret;
 
  HSL_FN_ENTER ();
 
  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
 
  if (HSL_IFMGR_HWCB_CHECK(hw_if_clear_counters))
     ret = HSL_IFMGR_HWCB_CALL(hw_if_clear_counters) (ifp);
 
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (0);
}
 
int 
hsl_ifmgr_stats_flush(hsl_ifIndex_t  ifindex)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER();

  ifp =  hsl_ifmgr_lookup_by_index (ifindex);
  if(!ifp)
  {
    HSL_FN_EXIT(STATUS_OK);
  }
  /* Do any platform specific cleanup. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_clear_counters))
    HSL_IFMGR_HWCB_CALL(hw_if_clear_counters) (ifp);

  /* Clear the interface Mac Counters */
  memset(&ifp->mac_cntrs,0,sizeof(struct hal_if_counters));


  HSL_FN_EXIT (STATUS_OK);
}

/*
  Dump HSL interface manager.
*/
void
hsl_ifmgr_dump (void)
{
  struct hsl_avl_node *node;
  struct hsl_if *ifp;

  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Count %d\n", HSL_IFMGR_TREE->count);
  for (node = hsl_avl_top (HSL_IFMGR_TREE); node; node = hsl_avl_next(node))
    {
      ifp = HSL_AVL_NODE_INFO (node);
      if (! ifp)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Invalid node in interface tree.");
          continue;
        }

#ifdef HAVE_L3
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Interface: %s Ifindex: %d Type: %s Oper Count %d FIB: %d\n", ifp->name, ifp->ifindex, _hsl_ifmgr_iftype_str (ifp->type),ifp->operCnt, ifp->fib_id);
#else
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Interface: %s Ifindex: %d Type: %s Oper Count %d \n", ifp->name, ifp->ifindex, _hsl_ifmgr_iftype_str (ifp->type),ifp->operCnt);
#endif /* HAVE_L3 */

      if (HSL_IFMGR_STACKCB_CHECK(os_if_dump))
        HSL_IFMGR_STACKCB_CALL(os_if_dump)(ifp);

      if (ifp->if_property != HSL_IF_CPU_ONLY_INTERFACE)
        if (HSL_IFMGR_HWCB_CHECK(hw_if_dump))
          HSL_IFMGR_HWCB_CALL(hw_if_dump) (ifp);
    }
}

/*
  Dump HSL interface manager details.
*/
void
hsl_ifmgr_dump_detail (void)
{

}


/* 
   hsl_ifmgr_add_counters Add interface mac counters.   
   res = a + b;
*/
void
hsl_ifmgr_add_mac_counters(struct hal_if_counters *a, 
                           struct hal_if_counters *b,
                           struct hal_if_counters *res)
{
#if 0
  HSL_FN_ENTER ();
#endif
  hsl_add_uint64(&a->out_errors,&b->out_errors,&res->out_errors);
  hsl_add_uint64(&a->out_discards,&b->out_discards,&res->out_discards);
  hsl_add_uint64(&a->out_mc_pkts,&b->out_mc_pkts,&res->out_mc_pkts);
  hsl_add_uint64(&a->out_uc_pkts,&b->out_uc_pkts,&res->out_uc_pkts);
  hsl_add_uint64(&a->in_discards,&b->in_discards,&res->in_discards);
  hsl_add_uint64(&a->good_octets_rcv,&b->good_octets_rcv,&res->good_octets_rcv);
  hsl_add_uint64(&a->bad_octets_rcv,&b->bad_octets_rcv, &res->bad_octets_rcv);
  hsl_add_uint64(&a->mac_transmit_err,&b->mac_transmit_err,&res->mac_transmit_err);
  hsl_add_uint64(&a->good_pkts_rcv,&b->good_pkts_rcv,&res->good_pkts_rcv);
  hsl_add_uint64(&a->bad_pkts_rcv,&b->bad_pkts_rcv,&res->bad_pkts_rcv);
  hsl_add_uint64(&a->brdc_pkts_rcv,&b->brdc_pkts_rcv,&res->brdc_pkts_rcv);
  hsl_add_uint64(&a->mc_pkts_rcv,&b->mc_pkts_rcv,&res->mc_pkts_rcv);
  hsl_add_uint64(&a->pkts_64_octets,&b->pkts_64_octets,&res->pkts_64_octets);
  hsl_add_uint64(&a->pkts_65_127_octets,&b->pkts_65_127_octets,&res->pkts_65_127_octets);
  hsl_add_uint64(&a->pkts_128_255_octets,&b->pkts_128_255_octets,&res->pkts_128_255_octets);
  hsl_add_uint64(&a->pkts_256_511_octets,&b->pkts_256_511_octets,&res->pkts_256_511_octets);
  hsl_add_uint64(&a->pkts_512_1023_octets,&b->pkts_512_1023_octets,&res->pkts_512_1023_octets);
  hsl_add_uint64(&a->pkts_1024_max_octets,&b->pkts_1024_max_octets,&res->pkts_1024_max_octets);
  hsl_add_uint64(&a->good_octets_sent,&b->good_octets_sent,&res->good_octets_sent);
  hsl_add_uint64(&a->good_pkts_sent,&b->good_pkts_sent,&res->good_pkts_sent);
  hsl_add_uint64(&a->excessive_collisions,&b->excessive_collisions,&res->excessive_collisions);
  hsl_add_uint64(&a->mc_pkts_sent,&b->mc_pkts_sent,&res->mc_pkts_sent);
  hsl_add_uint64(&a->brdc_pkts_sent,&b->brdc_pkts_sent,&res->brdc_pkts_sent);
  hsl_add_uint64(&a->unrecog_mac_cntr_rcv,&b->unrecog_mac_cntr_rcv,&res->unrecog_mac_cntr_rcv);
  hsl_add_uint64(&a->fc_sent,&b->fc_sent,&res->fc_sent);
  hsl_add_uint64(&a->good_fc_rcv,&b->good_fc_rcv,&res->good_fc_rcv);
  hsl_add_uint64(&a->drop_events,&b->drop_events,&res->drop_events);
  hsl_add_uint64(&a->undersize_pkts,&b->undersize_pkts,&res->undersize_pkts);
  hsl_add_uint64(&a->fragments_pkts,&b->fragments_pkts,&res->fragments_pkts);
  hsl_add_uint64(&a->oversize_pkts,&b->oversize_pkts,&res->oversize_pkts);
  hsl_add_uint64(&a->jabber_pkts,&b->jabber_pkts,&res->jabber_pkts);
  hsl_add_uint64(&a->mac_rcv_error,&b->mac_rcv_error,&res->mac_rcv_error);
  hsl_add_uint64(&a->bad_crc,&b->bad_crc,&res->bad_crc);
  hsl_add_uint64(&a->collisions,&b->collisions,&res->collisions);
  hsl_add_uint64(&a->late_collisions,&b->late_collisions,&res->late_collisions);
  hsl_add_uint64(&a->bad_fc_rcv,&b->bad_fc_rcv,&res->bad_fc_rcv);
#if 0
  HSL_FN_EXIT ();
#endif
  return;
};

/* 
   hsl_ifmgr_sub_counters substruct  interface mac counters.   
   res = a - b;
*/
void
hsl_ifmgr_sub_mac_counters(struct hal_if_counters *a, 
                           struct hal_if_counters *b,
                           struct hal_if_counters *res)
{
  /*   HSL_FN_ENTER (); */
  hsl_sub_uint64(&a->out_errors,&b->out_errors,&res->out_errors);
  hsl_sub_uint64(&a->out_discards,&b->out_discards,&res->out_discards);
  hsl_sub_uint64(&a->out_mc_pkts,&b->out_mc_pkts,&res->out_mc_pkts);
  hsl_sub_uint64(&a->out_uc_pkts,&b->out_uc_pkts,&res->out_uc_pkts);
  hsl_sub_uint64(&a->in_discards,&b->in_discards,&res->in_discards);
  hsl_sub_uint64(&a->good_octets_rcv,&b->good_octets_rcv,&res->good_octets_rcv);
  hsl_sub_uint64(&a->bad_octets_rcv,&b->bad_octets_rcv, &res->bad_octets_rcv);
  hsl_sub_uint64(&a->mac_transmit_err,&b->mac_transmit_err,&res->mac_transmit_err);
  hsl_sub_uint64(&a->good_pkts_rcv,&b->good_pkts_rcv,&res->good_pkts_rcv);
  hsl_sub_uint64(&a->bad_pkts_rcv,&b->bad_pkts_rcv,&res->bad_pkts_rcv);
  hsl_sub_uint64(&a->brdc_pkts_rcv,&b->brdc_pkts_rcv,&res->brdc_pkts_rcv);
  hsl_sub_uint64(&a->mc_pkts_rcv,&b->mc_pkts_rcv,&res->mc_pkts_rcv);
  hsl_sub_uint64(&a->pkts_64_octets,&b->pkts_64_octets,&res->pkts_64_octets);
  hsl_sub_uint64(&a->pkts_65_127_octets,&b->pkts_65_127_octets,&res->pkts_65_127_octets);
  hsl_sub_uint64(&a->pkts_128_255_octets,&b->pkts_128_255_octets,&res->pkts_128_255_octets);
  hsl_sub_uint64(&a->pkts_256_511_octets,&b->pkts_256_511_octets,&res->pkts_256_511_octets);
  hsl_sub_uint64(&a->pkts_512_1023_octets,&b->pkts_512_1023_octets,&res->pkts_512_1023_octets);
  hsl_sub_uint64(&a->pkts_1024_max_octets,&b->pkts_1024_max_octets,&res->pkts_1024_max_octets);
  hsl_sub_uint64(&a->good_octets_sent,&b->good_octets_sent,&res->good_octets_sent);
  hsl_sub_uint64(&a->good_pkts_sent,&b->good_pkts_sent,&res->good_pkts_sent);
  hsl_sub_uint64(&a->excessive_collisions,&b->excessive_collisions,&res->excessive_collisions);
  hsl_sub_uint64(&a->mc_pkts_sent,&b->mc_pkts_sent,&res->mc_pkts_sent);
  hsl_sub_uint64(&a->brdc_pkts_sent,&b->brdc_pkts_sent,&res->brdc_pkts_sent);
  hsl_sub_uint64(&a->unrecog_mac_cntr_rcv,&b->unrecog_mac_cntr_rcv,&res->unrecog_mac_cntr_rcv);
  hsl_sub_uint64(&a->fc_sent,&b->fc_sent,&res->fc_sent);
  hsl_sub_uint64(&a->good_fc_rcv,&b->good_fc_rcv,&res->good_fc_rcv);
  hsl_sub_uint64(&a->drop_events,&b->drop_events,&res->drop_events);
  hsl_sub_uint64(&a->undersize_pkts,&b->undersize_pkts,&res->undersize_pkts);
  hsl_sub_uint64(&a->fragments_pkts,&b->fragments_pkts,&res->fragments_pkts);
  hsl_sub_uint64(&a->oversize_pkts,&b->oversize_pkts,&res->oversize_pkts);
  hsl_sub_uint64(&a->jabber_pkts,&b->jabber_pkts,&res->jabber_pkts);
  hsl_sub_uint64(&a->mac_rcv_error,&b->mac_rcv_error,&res->mac_rcv_error);
  hsl_sub_uint64(&a->bad_crc,&b->bad_crc,&res->bad_crc);
  hsl_sub_uint64(&a->collisions,&b->collisions,&res->collisions);
  hsl_sub_uint64(&a->late_collisions,&b->late_collisions,&res->late_collisions);
  hsl_sub_uint64(&a->bad_fc_rcv,&b->bad_fc_rcv,&res->bad_fc_rcv);
  /*  HSL_FN_EXIT (); */
  return;
};
/* 
   _hsl_ifmgr_ifname_cmp - Interface name comparison function.
*/
int
_hsl_ifmgr_ifname_cmp (void *data1, void *data2)
{
  struct hsl_if *ifp1,*ifp2;
  int ifp1_index,ifp2_index;
  int res;

  HSL_FN_ENTER();

  ifp1 = *(struct hsl_if **)data1;
  ifp2 = *(struct hsl_if **)data2;

  /* Compare fe/ge/xe first */
  res = memcmp(ifp1->name,ifp2->name,2);
  if(res != 0)
    HSL_FN_EXIT(res);

  /* For same type of interface compare index. */
  sscanf (ifp1->name + 2, "%d", &ifp1_index);
  sscanf (ifp2->name + 2, "%d", &ifp2_index);

  if(ifp1_index > ifp2_index)
    {
      HSL_FN_EXIT(1);
    }
  else if(ifp1_index < ifp2_index)
    {
      HSL_FN_EXIT(-1);
    }

  HSL_FN_EXIT(0);
}

#define _HSL_SET_L2_ARR(N, IFP, IFP_ARR, ARR_SIZE)                      \
  do {                                                                  \
    /* Get Interface info. */                                           \
    (IFP) = HSL_AVL_NODE_INFO (N);                                      \
    if ((IFP) &&                                                        \
        ((IFP)->type == HSL_IF_TYPE_L2_ETHERNET || (IFP)->if_property != HSL_IF_CPU_ONLY_INTERFACE)) \
      {                                                                 \
        /* Add interface to snapshot. */                                \
        (*(IFP_ARR))[*(ARR_SIZE)] = (IFP);                              \
        (*(ARR_SIZE))++;                                                \
      }                                                                 \
  } while (0)
       
/* 
   hsl_ifmgr_get_L2_array - Service routing to extract l2 interfaces sorted 
   array. It is caller responsibility to the memory allocated for array.
*/

int 
hsl_ifmgr_get_L2_array(struct hsl_if ***ifp_arr, u_int16_t *arr_size)
{
  struct hsl_avl_node **nlist;
  struct hsl_avl_node *node;
  struct hsl_if *ifp;
  u_int16_t count;
  u_int16_t top;
  int ret;

  HSL_FN_ENTER();

  if (! ifp_arr || ! arr_size)
    HSL_FN_EXIT (-1);

  nlist = NULL;
  *ifp_arr = NULL;
  top = 0;
  ret = 0;
  *arr_size =  0;

  if( !ifp_arr || ! arr_size )
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Wrong parameters\n");
      HSL_FN_EXIT(-1);
    } 

  *arr_size = 0;

  /* Lock interface manager. */
  HSL_IFMGR_LOCK;

  /* Get interface tree node count. */
  count = hsl_avl_get_tree_size(HSL_IFMGR_TREE);
  
  /* Allocate memory for storing nodes to be revisited. */
  nlist = oss_malloc ((count * sizeof (struct hsl_avl_node *)), OSS_MEM_HEAP);
  if (NULL == nlist)
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Memory allocation error\n");
      HSL_IFMGR_UNLOCK;
      ret = HSL_IFMGR_ERR_MEMORY; 
      goto ERR;
    }

  /* Allocate memory for interfaces array. */
  *ifp_arr = oss_malloc((count * sizeof(struct hsl_if *)),OSS_MEM_HEAP);
  if(NULL == *ifp_arr)
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Memory allocation error\n");
      HSL_IFMGR_UNLOCK;
      ret = HSL_IFMGR_ERR_MEMORY;
      goto ERR;
    }

  /* Walk Interface tree and save L2_ETHERNET if pointers. */
  node = hsl_avl_top (HSL_IFMGR_TREE);
  while (node || top)
    {
      while (node != NULL)
        {
          nlist[top++] = node;
          node = node->left; 
        }
 
      node = nlist[--top];
      _HSL_SET_L2_ARR(node, ifp, ifp_arr, arr_size);

      node = node->right;
    }

  HSL_IFMGR_UNLOCK;

  /* Free revisit node list. */
  oss_free (nlist, OSS_MEM_HEAP);

  HSL_FN_EXIT(ret);

 ERR:
  /* Free revisit node list. */
  if (nlist)
    oss_free (nlist, OSS_MEM_HEAP);
  HSL_FN_EXIT (ret);  
}

/* 
   Add port mac counters to parent interfaces. 
*/

void
_hsl_ifmgr_update_parents_mac_cntrs(struct hsl_if *ifp, struct hal_if_counters *mac_cntrs)
{
  struct hsl_if *tmpif; 
  struct hsl_if_list *node = NULL;

  /* Loop over all parents and add statistics. */
  for (node = ifp->parent_list; node; node = node->next)
    {
      tmpif = node->ifp;

      /* Update L2 interface counters. */
      hsl_ifmgr_add_mac_counters(mac_cntrs,&tmpif->mac_cntrs, &tmpif->mac_cntrs);

      /* Update parents of parents. */
      if(tmpif->parent_list)
        {
          _hsl_ifmgr_update_parents_mac_cntrs(tmpif,mac_cntrs);
        } 
    }
}

/* 
   Process Interface statistics collection.      
*/
void
hsl_ifmgr_collect_if_stat(void)
{
  struct hsl_avl_node *node;
  struct hsl_if *ifp;
  static struct hsl_if tifp;
  struct hal_if_counters old_cntrs;
  struct hal_if_counters diff_cntrs;
  int ret = 0;

  /*HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                  "Message: +hsl_ifmgr_collect_if_stat \n");*/

  /* Make sure system is ready */
  if(HSL_FALSE == hsl_ifmgr_initialized)
    {
      return;
    }
  /* Initialization. */
  memset(&diff_cntrs,0,sizeof(struct hal_if_counters)); 
  
  /* Lock interface manager. */
  HSL_IFMGR_LOCK;
 
  /*  Find last processed node. */
  node = hsl_avl_lookup (HSL_IFMGR_TREE, (void *)&tifp);
  if (! node)
    {
      /* If node not found start from the top of the tree. */
      node = hsl_avl_top (HSL_IFMGR_TREE);
    }
  else
    {
      /* Get next node to process. */
      node = hsl_avl_next(node);
    }

  /* Find first real interface (node without children) */
  for (; node; node = hsl_avl_next(node))
    {
      /* Get Interface info. */
      ifp = HSL_AVL_NODE_INFO (node);
      if (! ifp)
        {
#if 0
          HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Interface not found in database\n");
#endif
          continue;
        }
      /* Check statistics only on physical interfaces */
      if(ifp->children_list)
        continue;

      /* Preserve interface node info. */
      tifp = *ifp;
      break;  
    }
  if(!node)
    {
      /* Start from the begining */
      memset(&tifp,0,sizeof(struct hsl_if));
      HSL_IFMGR_UNLOCK;
      return;
    }

  /* Preserve the original counters. */
  old_cntrs = ifp->mac_cntrs; 

  if(ifp->if_property != HSL_IF_CPU_ONLY_INTERFACE)
    {
      /* Read counters from HW. */
      if (HSL_IFMGR_HWCB_CHECK(hw_if_get_counters))
        ret = HSL_IFMGR_HWCB_CALL(hw_if_get_counters) (ifp);
    } 
  else
    {
      /* Read counters from OS. */
      if (HSL_IFMGR_STACKCB_CHECK(os_if_get_counters))
        ret = HSL_IFMGR_STACKCB_CALL(os_if_get_counters) (ifp);
    }

  /* Bail out of read call failed. */ 
  if(ret < 0)
    {
      HSL_IFMGR_UNLOCK;
      return;
    }

  /* Calculate the change in counters. */
  hsl_ifmgr_sub_mac_counters(&ifp->mac_cntrs, &old_cntrs, &diff_cntrs);

  /* Update parent interfaces. */
  _hsl_ifmgr_update_parents_mac_cntrs(ifp, &diff_cntrs);

#ifdef HAVE_ONMD
  /* Calculate EFM frame errors*/
  hsl_efm_frame_errors (ifp, &diff_cntrs);
#endif /*HAVE_ONMD*/

  HSL_IFMGR_UNLOCK;
  return;
}

#ifdef HAVE_ONMD

/*
 * Calculate frame errors for EFM
 */
int
hsl_efm_frame_errors (struct hsl_if *ifp, struct hal_if_counters *diff_cntrs)
{
  u_int32_t total_errors = 0;
  int ret = 0;
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: + %s \n", __FUNCTION__);
  
  if (!ifp)
    {
      ret = HSL_IFMGR_ERR_IF_NOT_FOUND;
      HSL_FN_EXIT(ret);
    }

  /*Check if EFM counters has exceeded the window*/
  hsl_ifmgr_compute_efm_counters (ifp);

  /* Get EFM frame error count*/
  hsl_ifmgr_efm_get_frame_errors (ifp, diff_cntrs, &total_errors);

  /* update EFM error count - EFS*/
  if (total_errors)
    ifp->u.l2_ethernet.efm_err_frame.efm_efs_count++;
  
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: - %s \n", __FUNCTION__);
  HSL_FN_EXIT(ret);
}

/*
  Check if EFM window has expired.  
*/
int 
hsl_ifmgr_compute_efm_counters (struct hsl_if *ifp)
{
  ut_int64_t pkts_rcvd_curr;
  ut_int64_t diff_rcvd;
  int window_expired = 0;
  u_int32_t total_errors = 0;
  hsl_efm_err_frame *efm_err_frame = NULL;
  int ret = 0;

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: + %s \n", __FUNCTION__);
  
  if (!ifp)
    {
      ret = HSL_IFMGR_ERR_IF_NOT_FOUND;
      HSL_FN_EXIT(ret);
    }
 
  efm_err_frame = &(ifp->u.l2_ethernet.efm_err_frame);

  /*check if frame period monitor is enabled*/
  if (1 == efm_err_frame->frame_period_monitor)
    {
      /*check if frame period window has expired*/

      hsl_add_uint64(&ifp->mac_cntrs.good_pkts_rcv, 
                     &ifp->mac_cntrs.good_pkts_sent,
                     &pkts_rcvd_curr);

      hsl_sub_uint64(&pkts_rcvd_curr, 
                     (ut_int64_t*)&(efm_err_frame->efm_frame_count_initial),
                     &diff_rcvd);

      /*check if diff_rcvd > frame period window */
      if (diff_rcvd.l[0] >= efm_err_frame->frame_period_window)
        {
          window_expired = 1;  
        }
      
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
               "Message: diff_rcvd.l[1] = %u, diff_rcvd.l[0] = %u \n", 
               diff_rcvd.l[1], diff_rcvd.l[0]);

      if(window_expired)
        {

          /*compute frame errors occurred in the expired window*/
          compute_frame_errors(ifp, &total_errors);
          
          HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
                  "Message: Sending FPWindow Expiry to HAL. Total Error = %u,\
                   ifindex = % u\n Will prepare for next FP window \n",
                   total_errors, (u_int32_t)ifp->ifindex);

          /*format HAL msg and send it*/
          hsl_sock_if_event(HSL_IF_EVENT_FPWINDOW_EXPIRY, ifp, &total_errors);

          /*new window has started. Initialise the packet counters*/
          hsl_add_uint64(&ifp->mac_cntrs.good_pkts_rcv,
              &ifp->mac_cntrs.good_pkts_sent,
              (ut_int64_t*)&(efm_err_frame->efm_frame_count_initial));

          /*store the counter values when Frame Period Window begins*/
          memcpy(&efm_err_frame->efm_cntrs, &ifp->mac_cntrs,
              sizeof(struct hal_if_counters));
                              
        }
    }
 
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: - %s \n", __FUNCTION__); 
  HSL_FN_EXIT(ret);

       
}

/*
  Compute frame errors for the given interface
*/
int
compute_frame_errors (struct hsl_if *ifp, u_int32_t *total_errors)
{
  struct hal_if_counters diff_cntrs;
  int ret = 0;
  hsl_efm_err_frame *efm_err_frame = NULL;
 
  HSL_FN_ENTER ();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: + %s \n", __FUNCTION__);
  
  if (!ifp)
  {
    ret = HSL_IFMGR_ERR_IF_NOT_FOUND;
    HSL_FN_EXIT(ret);
  }
 
  efm_err_frame = &(ifp->u.l2_ethernet.efm_err_frame);
 
  /* 
   * find the difference between current counters and 
   * counters stored at the beginning of window
   */
  hsl_ifmgr_sub_mac_counters(&ifp->mac_cntrs, 
                             &efm_err_frame->efm_cntrs, 
                             &diff_cntrs);

  /*add different kinds of frame errors*/
  *total_errors = diff_cntrs.undersize_pkts.l[0] + 
                  diff_cntrs.fragments_pkts.l[0] +
                  diff_cntrs.oversize_pkts.l[0] +
                  diff_cntrs.jabber_pkts.l[0] +
                  diff_cntrs.bad_crc.l[0];    

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: - %s, total errors = %u\n",
                                         __FUNCTION__, *total_errors);
  HSL_FN_EXIT(ret);
}

/*
  Get frame errors for the given interface , if diff_cntrs == NULL.
  Otherwise calculate the frame errors for the given diff counter.  
*/
int
hsl_ifmgr_efm_get_frame_errors (struct hsl_if *ifp, 
                               struct hal_if_counters *diff_cntrs, 
                               u_int32_t *total_errors)
{
  int ret = 0;
  struct hal_if_counters *temp_cntrs = NULL;

  HSL_FN_ENTER ();
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: + %s \n", __FUNCTION__);
  
  if (!ifp)
  {
    ret = HSL_IFMGR_ERR_IF_NOT_FOUND;
    HSL_FN_EXIT(ret);
  }
  
  if (!diff_cntrs)
    temp_cntrs = &(ifp->mac_cntrs);
  else
    temp_cntrs = diff_cntrs;

  /*add all kinds of frame errors*/ 
  *total_errors = temp_cntrs->undersize_pkts.l[0] +
                  temp_cntrs->fragments_pkts.l[0] +
                  temp_cntrs->oversize_pkts.l[0] +
                  temp_cntrs->jabber_pkts.l[0] +
                  temp_cntrs->bad_crc.l[0];

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: - %s, total errors = %u \n", 
                                         __FUNCTION__, *total_errors);
  HSL_FN_EXIT(ret);
}


/* set frame period window */
int
hsl_ifmgr_efm_frame_period_window_set (hsl_ifIndex_t ifindex, u_int32_t window)
{
  struct hsl_if *ifp = NULL;
  int ret=0;
  hsl_efm_err_frame *efm_err_frame = NULL; 

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, 
           "Message: hsl_ifmgr_efm_frame_period_window_set \n");
  HSL_FN_ENTER ();

  /*Get the hsl interface pointer*/
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  
  /*Get the efm frame counter from hsl if struct*/
  efm_err_frame = &(ifp->u.l2_ethernet.efm_err_frame);
  if (!efm_err_frame)
  {
    ret = -1;
    HSL_FN_EXIT (ret);
  }
  
  /*store the window size*/
  efm_err_frame->frame_period_window = window;
  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,
           "Message: FP Window set to %u \n", window);
  
  if (window)
    {
      /* enable frame period monitoring */
      efm_err_frame->frame_period_monitor = 1;

      /* get the current counter and store it */
      hsl_ifmgr_get_if_counters (ifindex, &efm_err_frame->efm_cntrs);

      /* calculate the current frames tx'd + rx'd. store this */
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, 
               "Message: calculating init_count \n");
      hsl_add_uint64 (&efm_err_frame->efm_cntrs.good_pkts_rcv, 
                      &efm_err_frame->efm_cntrs.good_pkts_sent,
                      (ut_int64_t*)&(efm_err_frame->efm_frame_count_initial));

    } 
  else
    {
      /* disable frame period monitoring */
      efm_err_frame->frame_period_monitor = 0;
    }

  HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO, "Message: - %s \n", __FUNCTION__);

  HSL_FN_EXIT(ret);

}
#endif /*HAVE_ONMD*/


/*
  Init port mirroring.
*/
int
hsl_ifmgr_init_portmirror (void)
{
  int ret;

  ret = -1;

  HSL_FN_ENTER();

  if (HSL_IFMGR_HWCB_CHECK(hw_if_init_portmirror))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_init_portmirror) ();

  HSL_FN_EXIT(ret);
}

/*
  Denit port mirroring.
*/
int
hsl_ifmgr_deinit_portmirror (void)
{
  int ret;

  ret = -1;

  HSL_FN_ENTER();

  if (HSL_IFMGR_HWCB_CHECK(hw_if_deinit_portmirror))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_deinit_portmirror) ();

  HSL_FN_EXIT(ret);
}
/*
  Set port mirroring.
*/
int
hsl_ifmgr_set_portmirror (hsl_ifIndex_t to_ifindex, hsl_ifIndex_t from_ifindex,
                          enum hal_port_mirror_direction direction)
{
  struct hsl_if *to_ifp = NULL;
  struct hsl_if *from_ifp = NULL;
  struct hsl_if *tmp_to_ifp = NULL;
  struct hsl_if *tmp_from_ifp = NULL;
  int ret = 0;

  HSL_FN_ENTER ();

  do
    {
      to_ifp = hsl_ifmgr_lookup_by_index (to_ifindex);
      if (! to_ifp)
        {
          ret = HSL_ERR_BRIDGE_INVALID_PARAM;
          break; 
        }

      from_ifp = hsl_ifmgr_lookup_by_index (from_ifindex);
      if (! from_ifp)
        {
          ret = HSL_ERR_BRIDGE_INVALID_PARAM;
          break; 
        }

      if(to_ifp->type != from_ifp->type) 
        {
          ret = HSL_ERR_BRIDGE_INVALID_PARAM;
          break; 
        } 


      if(HSL_IF_TYPE_IP == to_ifp->type)
        {
          tmp_to_ifp   = hsl_ifmgr_get_first_L2_port (to_ifp);
          tmp_from_ifp = hsl_ifmgr_get_first_L2_port (from_ifp);
          if((!tmp_to_ifp) || (!tmp_from_ifp))
            {
              ret = HSL_ERR_BRIDGE_INVALID_PARAM;
              break; 
            }
        }

      if (HSL_IFMGR_HWCB_CHECK(hw_if_set_portmirror))
        ret = HSL_IFMGR_HWCB_CALL(hw_if_set_portmirror) ((tmp_to_ifp)?tmp_to_ifp:to_ifp, 
                                                           (tmp_from_ifp)?tmp_from_ifp:from_ifp, direction);
  
    }while (0);

  if(to_ifp)       HSL_IFMGR_IF_REF_DEC (to_ifp);
  if(from_ifp)     HSL_IFMGR_IF_REF_DEC (from_ifp);
  if(tmp_to_ifp)   HSL_IFMGR_IF_REF_DEC (tmp_to_ifp);
  if(tmp_from_ifp) HSL_IFMGR_IF_REF_DEC (tmp_from_ifp);

  HSL_FN_EXIT (ret);
}

/*
  Unset port mirroring.
*/
int
hsl_ifmgr_unset_portmirror (hsl_ifIndex_t to_ifindex, hsl_ifIndex_t from_ifindex,
                            enum hal_port_mirror_direction direction)
{
  struct hsl_if *to_ifp = NULL;
  struct hsl_if *from_ifp = NULL;
  struct hsl_if *tmp_to_ifp = NULL;
  struct hsl_if *tmp_from_ifp = NULL;
  int ret = 0;

  HSL_FN_ENTER ();

  do
    {
      to_ifp = hsl_ifmgr_lookup_by_index (to_ifindex);
      if (! to_ifp)
        {
          ret = HSL_ERR_BRIDGE_INVALID_PARAM;
          break; 
        }

      from_ifp = hsl_ifmgr_lookup_by_index (from_ifindex);
      if (! from_ifp)
        {
          ret = HSL_ERR_BRIDGE_INVALID_PARAM;
          break; 
        }

      if(to_ifp->type != from_ifp->type) 
        {
          ret = HSL_ERR_BRIDGE_INVALID_PARAM;
          break; 
        } 


      if(HSL_IF_TYPE_IP == to_ifp->type)
        {
          tmp_to_ifp   = hsl_ifmgr_get_first_L2_port (to_ifp);
          tmp_from_ifp = hsl_ifmgr_get_first_L2_port (from_ifp);
          if((!tmp_to_ifp) || (!tmp_from_ifp))
            {
              ret = HSL_ERR_BRIDGE_INVALID_PARAM;
              break; 
            }
        }

      if (HSL_IFMGR_HWCB_CHECK(hw_if_unset_portmirror))
        ret = HSL_IFMGR_HWCB_CALL(hw_if_unset_portmirror) ((tmp_to_ifp)?tmp_to_ifp:to_ifp, 
                                                             (tmp_from_ifp)?tmp_from_ifp:from_ifp, direction);
  
    }while (0);

  if(to_ifp)       HSL_IFMGR_IF_REF_DEC (to_ifp);
  if(from_ifp)     HSL_IFMGR_IF_REF_DEC (from_ifp);
  if(tmp_to_ifp)   HSL_IFMGR_IF_REF_DEC (tmp_to_ifp);
  if(tmp_from_ifp) HSL_IFMGR_IF_REF_DEC (tmp_from_ifp);

  HSL_FN_EXIT (ret);
}

/*
  Update specific interface L2 properties
*/
static int
_hsl_ifmgr_update_if_l2_properties(struct hsl_if *ifp, struct hsl_if *ifp_data, u_int32_t cindex)
{
  int ret = 0;

  /* Interface duplex. */ 
  if (CHECK_CINDEX (cindex, HSL_IF_CINDEX_DUPLEX))
    {
      if (ifp->if_property == HSL_IF_CPU_ONLY_INTERFACE) 
        {
          /* Set DUPLEX in OS. */
          if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_duplex_set))
            ret = HSL_IFMGR_STACKCB_CALL(os_l3_if_duplex_set) (ifp, ifp_data->u.l2_ethernet.duplex);
        }
      else if(ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        {
          /* Set DUPLEX in HW. */
          if (HSL_IFMGR_HWCB_CHECK(hw_if_duplex_set))
            ret = HSL_IFMGR_HWCB_CALL(hw_if_duplex_set) (ifp, ifp_data->u.l2_ethernet.duplex);
        }
      if(ret >= 0)
        ifp->u.l2_ethernet.duplex = ifp_data->u.l2_ethernet.duplex;
    }

  /* Interface mtu. */ 
  if (CHECK_CINDEX (cindex, HSL_IF_CINDEX_MTU))
    {
      /* Set MTU in HW. */
      if (ifp->if_property == HSL_IF_CPU_ONLY_INTERFACE) 
        {
          /* Set MTU in OS. */
          if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_mtu_set))
            ret = HSL_IFMGR_STACKCB_CALL(os_l3_if_mtu_set) (ifp, ifp_data->u.l2_ethernet.mtu);
        }
      else if(ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        {
          /* Set MTU in HW. */
          if(HSL_IFMGR_HWCB_CHECK(hw_if_mtu_set))
            ret = HSL_IFMGR_HWCB_CALL(hw_if_mtu_set) (ifp, ifp_data->u.l2_ethernet.mtu);
        }
      if(ret >= 0)
        ifp->u.l2_ethernet.mtu = ifp_data->u.l2_ethernet.mtu;
    }

  /* Autoneg on interface on/off  */
  if (CHECK_CINDEX (cindex, HSL_IF_CINDEX_AUTONEGO))
    { 
      if (ifp->if_property == HSL_IF_CPU_ONLY_INTERFACE) 
        {   
          /* Set autonego in OS. */
          if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_autonego_set))
            ret = HSL_IFMGR_STACKCB_CALL(os_l3_if_autonego_set) (ifp, ifp_data->u.l2_ethernet.autonego);
        }
      else if(ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        {
          /* Set autonego in HW. */
          if (HSL_IFMGR_HWCB_CHECK(hw_if_autonego_set))
            ret = HSL_IFMGR_HWCB_CALL(hw_if_autonego_set) (ifp, ifp_data->u.l2_ethernet.autonego);
        }
      /* Set autonego in interface manager. */
      if (ret >= 0)
        ifp->u.l2_ethernet.autonego = ifp_data->u.l2_ethernet.autonego;
    }
  /* Interface bandwidth. */
  if (CHECK_CINDEX (cindex, HSL_IF_CINDEX_BANDWIDTH))
    {
      if (ifp->if_property == HSL_IF_CPU_ONLY_INTERFACE) 
        {
          /* Set bandwidth in OS. */
          if (HSL_IFMGR_STACKCB_CHECK(os_if_bandwidth_set))
            ret = HSL_IFMGR_STACKCB_CALL(os_if_bandwidth_set) (ifp, ifp_data->u.l2_ethernet.speed);
        }
      else if(ifp->type == HSL_IF_TYPE_L2_ETHERNET)
        {
          /* Set bandwidth in HW. */
          if (HSL_IFMGR_HWCB_CHECK(hw_if_bandwidth_set))
            ret = HSL_IFMGR_HWCB_CALL(hw_if_bandwidth_set) (ifp, ifp_data->u.l2_ethernet.speed);
        }
      /* Bandwidth shouldn't be stored in ifp. Speed is being stored in ifp
       * in kb/s in the call to hw_if_bandwidth_set. Comment the below code
       */
      /*
      if(ret >= 0)
        ifp->u.l2_ethernet.speed = ifp_data->u.l2_ethernet.speed;
      */
    }

  HSL_FN_EXIT(ret);
}

/*
  Update all children L2 properties
*/
static int
_hsl_ifmgr_set_if_l2_properties(struct hsl_if *ifp_parent, struct hsl_if *ifp_data, u_int32_t cindex)
{
  struct hsl_if *ifp2;
  struct hsl_if_list *node;
  int ret;

  HSL_FN_ENTER ();

  if((!ifp_parent) || (!ifp_data))
    HSL_FN_EXIT(-1);
  
  HSL_IFMGR_LOCK;

  if(!ifp_parent->children_list)
    {
      HSL_IFMGR_IF_REF_INC (ifp_parent);

      ret = _hsl_ifmgr_update_if_l2_properties(ifp_parent, ifp_data, cindex);

      HSL_IFMGR_IF_REF_DEC (ifp_parent);
      HSL_IFMGR_UNLOCK;
      HSL_FN_EXIT(ret);
    }

  node = ifp_parent->children_list;
  while (node)
    {
      ifp2 = node->ifp;
      HSL_IFMGR_IF_REF_INC (ifp2);
      /* If child has grand children update them as well. */ 
      if(ifp2->children_list)
        ret = _hsl_ifmgr_set_if_l2_properties(ifp2,ifp_data, cindex);
      else
        ret = _hsl_ifmgr_update_if_l2_properties(ifp2, ifp_data, cindex);

      HSL_IFMGR_IF_REF_DEC(ifp2);
      node = node->next;
    }

  HSL_IFMGR_UNLOCK;
  HSL_FN_EXIT (ret);
}


int
_hsl_ifmgr_create_interface(struct hsl_if *ifp_params, 
                           struct hsl_if **new_ifp,
                           HSL_BOOL send_notification, 
                           HSL_BOOL allocated_params,
                           HSL_BOOL create_proc_entry)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER(); 

  /* Input parameters check. */ 
  if((!new_ifp) || (!ifp_params))
    {
      HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_PARAM);
    }

  /* Create ifp. */
  if(HSL_FALSE== allocated_params)
    {
      ifp = hsl_ifmgr_if_new ();
      if (!ifp)
        HSL_FN_EXIT (HSL_IFMGR_ERR_MEMORY);
      /* Preserve interface configuration. */
      *ifp  = *ifp_params;
    } 
  else
    {
      ifp = ifp_params;
    }

  /* Set reference count. */
  HSL_IFMGR_IF_REF_SET (ifp, 1);

  HSL_IFMGR_LOCK;
  /* Add this interface to the database. */
  ret = hsl_avl_insert (p_hsl_if_db->if_tree, ifp);
  HSL_IFMGR_UNLOCK;

  if (create_proc_entry && p_hsl_if_db->proc_if_create_cb) 
    {
      if(HSL_TRUE == allocated_params)
        {
          (*p_hsl_if_db->proc_if_create_cb) (ifp);
        }
    }
  
  /* Check for errors. */ 
  if(ret != 0)
    {
      if(HSL_FALSE == allocated_params)
        hsl_ifmgr_if_free (ifp);
      HSL_FN_EXIT(HSL_IFMGR_ERR_INDEX); 
    }

  /* Call notifiers for L2 port addition. */
  if (send_notification)
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFNEW, ifp, NULL);

  /* Return new interface pointer. */ 
  *new_ifp = ifp; 
  HSL_FN_EXIT (0);
}

/*
  Register interface with interface manager. 
  IN ifp_params        -  Interface parameters. 
  IN send_notification -  Send notification to protocol modules or not. 
  OUT new_ifp          -  Newly created interface. 
*/
int
hsl_ifmgr_create_interface(struct hsl_if *ifp_params, 
                           struct hsl_if **new_ifp,
                           HSL_BOOL send_notification, 
                           HSL_BOOL allocated_params)
{
  int ret;

  HSL_FN_ENTER(); 

  ret = _hsl_ifmgr_create_interface (ifp_params, new_ifp, send_notification, 
                                    allocated_params, HSL_TRUE);
  HSL_FN_EXIT(ret);
}


static int
_hsl_ifmgr_delete_interface (struct hsl_if *ifp, 
                             HSL_BOOL send_notification,
                             HSL_BOOL delete_proc_entry)
{
  HSL_FN_ENTER ();

  if(! ifp) 
    {
      HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_PARAM);
    }

  /* Call notifier for the port. */
  if(HSL_TRUE == send_notification) 
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFDELETE, ifp, NULL);

  if (delete_proc_entry && p_hsl_if_db->proc_if_remove_cb)
    {
      (*p_hsl_if_db->proc_if_remove_cb) (ifp);
    }
              
  /* Delete node from AVL tree. */
  HSL_IFMGR_LOCK;
  hsl_avl_delete (p_hsl_if_db->if_tree, ifp);
  HSL_IFMGR_UNLOCK;

  /* Free ifp. */
  hsl_ifmgr_if_free (ifp);

  HSL_FN_EXIT (0);
}

/*
  Remove interface from interface manager. 
  IN ifp               -  Interface pointer. 
  IN send_notification -  Send notification to protocol modules or not. 
*/
int
hsl_ifmgr_delete_interface(struct hsl_if *ifp, 
                           HSL_BOOL send_notification)
{
  HSL_FN_ENTER(); 

  _hsl_ifmgr_delete_interface (ifp, send_notification, HSL_TRUE);

  HSL_FN_EXIT(0);
}

/*
  Remove interface from interface manager. 
  IN ifindex -  Interface index. 
*/
int
hsl_ifmgr_delete_interface_api(hsl_ifIndex_t  ifindex)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER(); 

  ifp =  hsl_ifmgr_lookup_by_index (ifindex);
  if(!ifp)
    HSL_FN_EXIT(0);

  /* We don't need to keep this interface. */ 
  HSL_IFMGR_IF_REF_DEC (ifp);

  /* if property (CPU interface)  */
  if(ifp->if_property == HSL_IF_CPU_ONLY_INTERFACE)
    {
      hsl_ifmgr_delete_interface(ifp,HSL_TRUE); 
      HSL_FN_EXIT(0);
    }

  /* Delete hw interfaces.  */
  switch(ifp->type)
    {
    case HSL_IF_TYPE_L2_ETHERNET: 
      hsl_ifmgr_L2_ethernet_delete (ifp, HSL_TRUE);
      break;

#ifdef HAVE_L3
    case HSL_IF_TYPE_IP:
      hsl_ifmgr_L3_delete2 (ifp, HSL_TRUE);
      break;
#endif
#ifdef HAVE_MPLS
    case HSL_IF_TYPE_MPLS:
      hsl_ifmgr_mpls_delete (ifp);
      break;
#endif /* HAVE_MPLS */
 
    default:
      /* Strange we shouldn't get here. */
      hsl_ifmgr_delete_interface(ifp, HSL_TRUE); 
    }
  HSL_FN_EXIT(0);
}

/* 
   Set MTU for a interface. 
   IN ifindex -  Interface index. 
   IN mtu     -  Max transmit unit. 
*/
int
hsl_ifmgr_set_mtu (hsl_ifIndex_t ifindex, int mtu, HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  struct hsl_if ifp_data;
  u_int32_t cindex = 0;
  int ret = 0;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  /* HW L3 interface */
  if(ifp->type == HSL_IF_TYPE_IP)
    {
      /* Set MTU in OS.only  */
      if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_mtu_set))
        ret = HSL_IFMGR_STACKCB_CALL(os_l3_if_mtu_set) (ifp, mtu);

      if (ret >= 0)
        {
          /* Set MTU for L3 Interface */
          if (ifp->if_property != HSL_IF_CPU_ONLY_INTERFACE)
            {
              if (HSL_IFMGR_HWCB_CHECK (hw_if_l3_mtu_set))
                 ret = HSL_IFMGR_HWCB_CALL (hw_if_l3_mtu_set) (ifp, mtu);

              if (ret < 0)
                HSL_FN_EXIT (ret);
           }
        }
    }

  if(ret >= 0)
    {
      ifp_data.u.l2_ethernet.mtu = mtu;   
      SET_CINDEX (cindex, HSL_IF_CINDEX_MTU);
      ret = _hsl_ifmgr_set_if_l2_properties(ifp, &ifp_data,cindex);
    }

  if(ret >= 0)
    {
      if (ifp->type == HSL_IF_TYPE_IP)
        {
          /* Set MTU in interface manager. */
          ifp->u.ip.ipv4.mtu = mtu;
#ifdef HAVE_IPV6
          ifp->u.ip.ipv6.mtu = mtu;
#endif /* HAVE_IPV6 */
        }

      if(HSL_TRUE == send_notification)
        _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFMTU, ifp, NULL);
    }
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}


/*
  Set DUPLEX for a interface.
*/
int
hsl_ifmgr_set_duplex (hsl_ifIndex_t ifindex, int duplex, HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  struct hsl_if ifp_data;
  u_int32_t cindex = 0;
  int ret;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  ifp_data.u.l2_ethernet.duplex = duplex;   
  SET_CINDEX (cindex, HSL_IF_CINDEX_DUPLEX);
  ret = _hsl_ifmgr_set_if_l2_properties(ifp, &ifp_data,cindex);

  if((ret >= 0) &&(HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFDUPLEX, ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

/*
  Set AUTO-NEGOTIATE for a interface.
*/
int
hsl_ifmgr_set_autonego (hsl_ifIndex_t ifindex, int autonego, HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  struct hsl_if ifp_data;
  u_int32_t cindex = 0;
  int ret;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  ifp_data.u.l2_ethernet.autonego = autonego;   
  SET_CINDEX (cindex, HSL_IF_CINDEX_AUTONEGO);
  ret = _hsl_ifmgr_set_if_l2_properties(ifp, &ifp_data,cindex);

  if((ret >= 0) &&(HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFAUTONEGO, ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

/*
  Set MDIX crossover for a interface.
*/
int
hsl_ifmgr_set_mdix (hsl_ifIndex_t ifindex, int mdix, HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK(hw_if_mdix_set))
     ret = HSL_IFMGR_HWCB_CALL(hw_if_mdix_set) (ifp, mdix);

  if((ret >= 0) &&(HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_MDIX, ifp, NULL); 

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (0);
}

/* Add/Remove PortBased Vlan member ports  */
int
hsl_ifmgr_set_portbased_vlan (hsl_ifIndex_t ifindex, 
                              struct hal_port_map pbitmap,
                              HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();
  
  ret = STATUS_OK;  

  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  
  if (! ifp) 
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);  
  
  if (HSL_IFMGR_HWCB_CHECK(hw_if_portbased_vlan))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_portbased_vlan) (ifp, pbitmap); 
  
  if ((ret >= 0) && (HSL_TRUE == send_notification)) 
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_PORTBASED_VLAN,
                             ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);

  HSL_FN_EXIT (ret);
}

/* Set cpu port default vlan id */
int
hsl_ifmgr_cpu_default_vlan (hsl_ifIndex_t ifindex, int vid,
                              HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);  

  if (HSL_IFMGR_HWCB_CHECK(hw_if_cpu_default_vlan))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_cpu_default_vlan) (vid);

  if ((ret >= 0) && (HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, 
                         HSL_IF_EVENT_CPU_DEFAULT_VLAN, ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

/* Set Wayside port default vlan id */
int
hsl_ifmgr_wayside_default_vlan (hsl_ifIndex_t ifindex, int vid,
                              HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK(hw_if_wayside_default_vlan))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_wayside_default_vlan) (vid);

  if ((ret >= 0) && (HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain,
                         HSL_IF_EVENT_WAYSIDE_DEFAULT_VLAN, ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

int
hsl_ifmgr_set_preserve_ce_cos (hsl_ifIndex_t ifindex, 
                               HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
  
   if (HSL_IFMGR_HWCB_CHECK(hw_if_preserve_ce_cos))
    ret = HSL_IFMGR_HWCB_CALL (hw_if_preserve_ce_cos) (ifp);

  if ((ret >= 0) && (HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_PRESERVE_CE_COS,
                                                                 ifp, NULL);
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);

}

/* Set Port egress mode  */
int
hsl_ifmgr_set_port_egress (hsl_ifIndex_t ifindex, int egress,
                                                  HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();
  
  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK(hw_if_port_egress))
    ret = HSL_IFMGR_HWCB_CALL (hw_if_port_egress) (ifp, egress);

  if ((ret >= 0) && (HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_PORT_EGRESS,
                                                                 ifp, NULL);
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret); 
}

/* Set Force Vlan id */
int
hsl_ifmgr_set_force_vlan (hsl_ifIndex_t ifindex, int vid, 
                                                   HSL_BOOL send_notification )
{
  struct hsl_if *ifp;
  int ret;
  
  HSL_FN_ENTER ();
 
  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK (hw_if_set_force_vlan))  
    ret = HSL_IFMGR_HWCB_CALL (hw_if_set_force_vlan) (ifp, vid);
  
  if ((ret >= 0) && (HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_FORCE_VLAN,
                                                                ifp, NULL); 
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

int
hsl_ifmgr_set_sw_reset ()
{
  int ret; 
  
  HSL_FN_ENTER ();
  
  ret = STATUS_OK;

  if (HSL_IFMGR_HWCB_CHECK (hw_if_set_sw_reset))
     ret = HSL_IFMGR_HWCB_CALL (hw_if_set_sw_reset)();  

  HSL_FN_EXIT (ret);
}


/* Set Ether type */
int
hsl_ifmgr_set_ether_type (hsl_ifIndex_t ifindex, u_int16_t etype,
                          HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK (hw_if_set_ether_type))
    ret = HSL_IFMGR_HWCB_CALL (hw_if_set_ether_type) (ifp, etype);

  if ((ret >= 0) && (HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_ETHERTYPE,
                                                                ifp, NULL);
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

/* Set Learn Disable  */
int
hsl_ifmgr_set_learn_disable (hsl_ifIndex_t ifindex, int enable,
                             HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int flag = enable;
  int ret;

  HSL_FN_ENTER ();

  ret = STATUS_OK;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK (hw_if_learn_disable))
    ret = HSL_IFMGR_HWCB_CALL (hw_if_learn_disable) (ifp, &flag, HSL_TRUE);

  if ((ret >= 0) && (HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HAL_MSG_IF_SET_LEARN_DISABLE,
                             ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}


/* Get Learn Disable  */
int
hsl_ifmgr_get_learn_disable (hsl_ifIndex_t ifindex, int* enable)
{
  struct hsl_if *ifp;
  int ret, flag;
  
  HSL_FN_ENTER ();

  ret = 0;

  ifp = hsl_ifmgr_lookup_by_index (ifindex);

  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (HSL_IFMGR_HWCB_CHECK(hw_if_learn_disable))
     ret = HSL_IFMGR_HWCB_CALL(hw_if_learn_disable) (ifp, &flag, HSL_FALSE);

  memcpy(enable, &flag, sizeof(int));

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}


/*
  Set BANDWIDTH for a interface.
*/
int
hsl_ifmgr_set_bandwidth (hsl_ifIndex_t ifindex, u_int32_t bandwidth, HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  struct hsl_if ifp_data;
  u_int32_t cindex = 0;
  int ret;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  ifp_data.u.l2_ethernet.speed = bandwidth;   
  SET_CINDEX (cindex, HSL_IF_CINDEX_BANDWIDTH);
  ret = _hsl_ifmgr_set_if_l2_properties(ifp, &ifp_data,cindex);

  if((ret >= 0) &&(HSL_TRUE == send_notification))
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFBANDWIDTH, ifp, NULL);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}


/*
  Set Hardware address for a interface.
*/
int
hsl_ifmgr_set_hwaddr (hsl_ifIndex_t ifindex, int hwaddrlen, u_char *hwaddr, HSL_BOOL send_notification)
{
  struct hsl_if *ifp;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if(ifp->if_property != HSL_IF_CPU_ONLY_INTERFACE)
    {
      /* Set address in HW. */
      if (HSL_IFMGR_HWCB_CHECK(hw_if_hwaddr_set))
        HSL_IFMGR_HWCB_CALL(hw_if_hwaddr_set) (ifp, hwaddrlen, hwaddr);
    }

  if (ifp->type == HSL_IF_TYPE_IP)
    {
      /* Set address in OS. */
      if (HSL_IFMGR_STACKCB_CHECK(os_l3_if_hwaddr_set))
        HSL_IFMGR_STACKCB_CALL(os_l3_if_hwaddr_set) (ifp, hwaddrlen, hwaddr);

      /* Set address in interface manager. */
      memcpy (ifp->u.ip.mac, hwaddr, hwaddrlen);
    }
  else
    {
      /* Set address in interface manager. */
      memcpy (ifp->u.l2_ethernet.mac, hwaddr, hwaddrlen);
    }

  /* Call notifiers. */
  if(HSL_TRUE == send_notification)
    _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFHWADDR, ifp, hwaddr);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (0);
}

#ifdef HAVE_L3
/*
  Set secondary MAC addresses for a interface
*/
int
hsl_ifmgr_set_secondary_hwaddrs (hsl_ifIndex_t ifindex, 
                                 int hwaddrlen, 
                                 int num, u_char **hwaddrs, 
                                 HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;
  int hw;

  hw = 0;
  ret = 0;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (ifp->type != HSL_IF_TYPE_IP)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_PARAM);

  /* Set secondary addresses in hardware. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_secondary_hwaddrs_set))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_if_secondary_hwaddrs_set) (ifp, hwaddrlen, num, hwaddrs);
      if (ret < 0)
        {
          ret = HSL_IFMGR_ERR_HW_SEC_HWADDR;
          goto ERR;
        }
    }

  /* Populated in hardware. For rollback check if it fails in stack. */
  hw = 1;

  /* Set secondary addresses in stack. */
  if (HSL_IFMGR_STACKCB_CHECK(os_if_secondary_hwaddrs_set))
    {
      ret = HSL_IFMGR_STACKCB_CALL(os_if_secondary_hwaddrs_set) (ifp, hwaddrlen, num, hwaddrs);
      if (ret < 0)
        {
          ret = HSL_IFMGR_ERR_OS_SEC_HWADDR;
          goto ERR;
        }
    }

  HSL_FN_EXIT (ret);

 ERR:
  
  /* Error, rollback from hardware. */
  if (hw)
    {
      if (HSL_IFMGR_HWCB_CHECK(hw_if_secondary_hwaddrs_delete))
        HSL_IFMGR_HWCB_CALL(hw_if_secondary_hwaddrs_delete) (ifp, hwaddrlen, num, hwaddrs);
    }

  HSL_FN_EXIT (ret);
}

/*
  Add secondary MAC addresses for a interface.
*/
int
hsl_ifmgr_add_secondary_hwaddrs (hsl_ifIndex_t ifindex,  
                                 int hwaddrlen, 
                                 int num, u_char **hwaddrs, 
                                 HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;
  int hw;

  hw = 0;
  ret = 0;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if (ifp->type != HSL_IF_TYPE_IP)
  {
    HSL_IFMGR_IF_REF_DEC (ifp);
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_PARAM);
  }

  /* Set secondary addresses in hardware. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_secondary_hwaddrs_add))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_if_secondary_hwaddrs_add) (ifp, hwaddrlen, num, hwaddrs);
      if (ret < 0)
        {
          ret = HSL_IFMGR_ERR_HW_SEC_HWADDR;
          goto ERR;
        }
    }

  /* Populated in hardware. For rollback check if it fails in stack. */
  hw = 1;

  /* Set secondary addresses in stack. */
  if (HSL_IFMGR_STACKCB_CHECK(os_if_secondary_hwaddrs_add))
    {
      ret = HSL_IFMGR_STACKCB_CALL(os_if_secondary_hwaddrs_add) (ifp, hwaddrlen, num, hwaddrs);
      if (ret < 0)
        {
          ret = HSL_IFMGR_ERR_OS_SEC_HWADDR;
          goto ERR;
        }
    }
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);

 ERR:
  
  /* Error, rollback from hardware. */
  if (hw)
    {
      if (HSL_IFMGR_HWCB_CHECK(hw_if_secondary_hwaddrs_delete))
        HSL_IFMGR_HWCB_CALL(hw_if_secondary_hwaddrs_delete) (ifp, hwaddrlen, num, hwaddrs);
    }
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

/* 
   Delete secondary MAC address for a interface.
*/
int
hsl_ifmgr_delete_secondary_hwaddrs (hsl_ifIndex_t ifindex,
                                    int hwaddrlen, 
                                    int num, u_char **hwaddrs, 
                                    HSL_BOOL send_notification)
{
  struct hsl_if *ifp;
  int ret;
  int hw;

  hw = 0;
  ret = 0;

  HSL_FN_ENTER ();

  /* Find interface. */
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
  if (ifp->type != HSL_IF_TYPE_IP)
  {
    HSL_IFMGR_IF_REF_DEC (ifp);
    HSL_FN_EXIT (HSL_IFMGR_ERR_INVALID_PARAM);
  }

  /* Set secondary addresses in hardware. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_secondary_hwaddrs_delete))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_if_secondary_hwaddrs_delete) (ifp, hwaddrlen, num, hwaddrs);
      if (ret < 0)
        {
          ret = HSL_IFMGR_ERR_HW_SEC_HWADDR;
          goto ERR;
        }
    }

  /* Populated in hardware. For rollback check if it fails in stack. */
  hw = 1;

  /* Set secondary addresses in stack. */
  if (HSL_IFMGR_STACKCB_CHECK(os_if_secondary_hwaddrs_delete))
    {
      ret = HSL_IFMGR_STACKCB_CALL(os_if_secondary_hwaddrs_delete) (ifp, hwaddrlen, num, hwaddrs);
      if (ret < 0)
        {
          ret = HSL_IFMGR_ERR_OS_SEC_HWADDR;
          goto ERR;
        }
    }
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);

 ERR:
  
  /* Error, rollback from hardware. */
  if (hw)
    {
      if (HSL_IFMGR_HWCB_CHECK(hw_if_secondary_hwaddrs_add))
        HSL_IFMGR_HWCB_CALL(hw_if_secondary_hwaddrs_add) (ifp, hwaddrlen, num, hwaddrs);
    }
  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (ret);
}

#endif /* HAVE_L3 */

#ifdef HAVE_L3
/*
  Add ip address to interface. 
*/
int
hsl_ifmgr_os_ip_address_add (char *name, hsl_ifIndex_t ifindex,
                             hsl_prefix_t *prefix, u_char flags)
{
  _hsl_ifmgr_ip_address_add (name, ifindex, prefix, flags, HSL_TRUE, HSL_FALSE);
  return 0;
}

/*
  Remove address from interface. 
*/
int
hsl_ifmgr_os_ip_address_delete (char *name, hsl_ifIndex_t ifindex,
                                hsl_prefix_t *prefix)
{
  _hsl_ifmgr_ip_address_delete(name, ifindex, prefix, HSL_TRUE, HSL_FALSE);
  return 0;
}

#endif  /* HAVE_L3 */

#ifdef  HAVE_LACPD
/* 
   Add aggregator. 
*/
int
hsl_ifmgr_aggregator_add (char *agg_name, u_char agg_mac[], int agg_type)
{
  struct hsl_if *ifp;
  int ret = 0; 

  HSL_FN_ENTER(); 
  if ((!agg_name) ||
      ((agg_type != HSL_IF_TYPE_IP) && (agg_type != HSL_IF_TYPE_L2_ETHERNET)))
    HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_PARAM);
  
  /* lookup aggregator */
  ifp = hsl_ifmgr_lookup_by_name (agg_name);
  if (ifp)
    {
      HSL_IFMGR_IF_REF_DEC (ifp);
      HSL_FN_EXIT(HSL_IFMGR_ERR_AGG_EXISTS);
    }

#ifdef HAVE_L3
  if (agg_type == HSL_IF_TYPE_IP)
    {
      ret = hsl_ifmgr_L3_create (agg_name, agg_mac, HSL_ETHER_ALEN,
                                 0, NULL, &ifp);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                   "Failed to add aggregator %s, error %d\n", agg_name, ret); 
          HSL_FN_EXIT(ret);
        }
    }
  else
    {
#endif  /* HAVE_L3 */
      ret = hsl_ifmgr_L2_ethernet_create (agg_name, agg_mac, 0, 0, 0,
                                          0, NULL, 0, 0, &ifp);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                   "Failed to add aggregator %s, error %d\n", agg_name, ret); 
          HSL_FN_EXIT(ret);
        }
#ifdef HAVE_L3
    }
#endif /* HAVE_L3 */

  /* create trunk in hw */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_lacp_agg_add))
    {
      ret = HSL_IFMGR_HWCB_CALL(hw_if_lacp_agg_add) (ifp, agg_type);
      if(ret < 0)   
        goto ERR;
    }

  /* Set port selection criteria to a default one HAL_LACP_PSC_DST_MAC. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_lacp_psc_set))
    HSL_IFMGR_HWCB_CALL(hw_if_lacp_psc_set) (ifp, HAL_LACP_PSC_DST_MAC);

  /* Call interface addition notifier */
  hsl_ifmgr_send_notification (HSL_IF_EVENT_IFNEW, ifp, NULL);

  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, "Add aggregator %s successfully.\n", agg_name); 
  HSL_FN_EXIT(STATUS_OK);

 ERR:
#ifdef HAVE_L3
  if (agg_type == HSL_IF_TYPE_IP)
    hsl_ifmgr_L3_delete (ifp, 0);
  else
#endif
    hsl_ifmgr_L2_ethernet_delete (ifp, 0);
  HSL_FN_EXIT(ret);
}

/* 
   Delete aggregator. 
*/
int
hsl_ifmgr_aggregator_del (char *agg_name, u_int32_t agg_ifindex)
{
  struct hsl_if *ifp = NULL;
  int ret;

  HSL_FN_ENTER(); 
  /* get interface instance */
  if (agg_ifindex > 0)
    ifp = hsl_ifmgr_lookup_by_index (agg_ifindex);
  else
    ifp = hsl_ifmgr_lookup_by_name (agg_name);

  if (! ifp)
    HSL_FN_EXIT(STATUS_ERROR);

  /* delete trunk in hw */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_lacp_agg_del))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_lacp_agg_del) (ifp);

  HSL_IFMGR_IF_REF_DEC (ifp);
#ifdef HAVE_L3
  if (ifp->type == HSL_IF_TYPE_IP)
    hsl_ifmgr_L3_delete (ifp, 1);
  else
#endif /* HAVE_L3 */
    hsl_ifmgr_L2_ethernet_delete (ifp, 1);

  HSL_FN_EXIT(STATUS_OK);
}

/* 
   Set aggregator port selection criteria. 
*/
int
hsl_ifmgr_lacp_psc_set (u_int32_t agg_ifindex ,int psc)
{
  struct hsl_if *ifp;
  int ret = 0;

  HSL_FN_ENTER();

  /* Lookup interface. */
  ifp = hsl_ifmgr_lookup_by_index (agg_ifindex);
  if (!ifp)
    {
      HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_ERROR, "Interface (%d) not found in database\n",  agg_ifindex);
      HSL_FN_EXIT(-1);
    }

  if (HSL_IFMGR_HWCB_CHECK(hw_if_lacp_psc_set))
    ret = HSL_IFMGR_HWCB_CALL(hw_if_lacp_psc_set) (ifp, psc);

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT(ret);
}
/* 
   Add port to aggregator. 
*/
int
hsl_ifmgr_aggregator_port_attach (char *agg_name, u_int32_t agg_ifindex,
                                  u_int32_t port_ifindex)
{
  struct hsl_if *agg_ifp = NULL;
  struct hsl_if *port_ifp = NULL;
  int ret;
  int create_if = 0;

  HSL_FN_ENTER();
  /* Get aggregator port data */
  if (agg_ifindex > 0)
    agg_ifp = hsl_ifmgr_lookup_by_index (agg_ifindex);
  else
    agg_ifp = hsl_ifmgr_lookup_by_name (agg_name);

  /*
    If no aggregator found return an error. 
  */ 
  if (!agg_ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to bind port %d to trunk %s(%d), "
               "trunk doesn't exist \n", 
               port_ifindex, agg_name,agg_ifindex);
      HSL_FN_EXIT(HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* Get constituent port data */
  port_ifp = hsl_ifmgr_lookup_by_index (port_ifindex);
  if (!port_ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to bind port %d to trunk %s, "
               "port doesn't exist \n", port_ifindex, agg_name);
      HSL_IFMGR_IF_REF_DEC (agg_ifp);
      HSL_FN_EXIT(HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* 
     Make sure port & aggregator have the same interface type. 
  */
  if (agg_ifp->type != port_ifp->type)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to bind port %s to trunk %s, "
               "interface type mismatch \n",
               port_ifp->name, agg_ifp->name);
      HSL_IFMGR_IF_REF_DEC (agg_ifp);
      HSL_IFMGR_IF_REF_DEC (port_ifp);
      HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_MODE);
    }

  /* 
     If first member port was added bring the aggregator up. 
  */ 
  if ((agg_ifp->type == HSL_IF_TYPE_IP) &&
      (agg_ifp->children_list == NULL))
    {
      create_if = 1;
    }

#ifdef HAVE_VLAN
  if (agg_ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      /* Update port agg vlan hierarchy. */
      hsl_vlan_agg_port_bind_update(port_ifp, agg_ifp, HSL_TRUE);
    }
#endif /* HAVE_VLAN */
  /* Attach the individual port to aggregator */
  ret = hsl_ifmgr_bind2 (agg_ifp, port_ifp);
  if (ret < 0)
    {
      HSL_IFMGR_IF_REF_DEC (agg_ifp);
      HSL_IFMGR_IF_REF_DEC (port_ifp);
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to bind port %s to trunk %s, error = %d\n",
               port_ifp->name, agg_ifp->name, ret);
      HSL_FN_EXIT(ret);
    }
#ifdef HAVE_L3
  if ((create_if == 1) && (agg_ifp->flags & IFF_UP))
    {
      if (HSL_IFMGR_HWCB_CHECK(hw_l3_if_flags_set))
        HSL_IFMGR_HWCB_CALL(hw_l3_if_flags_set) (agg_ifp,IFF_UP);
    }
#endif /* HAVE_L3 */

  /* Adjust the properties have been copied. Actually set the flags now. */
  hsl_ifmgr_set_flags2 (agg_ifp, agg_ifp->flags);

  /* Attach port in hw. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_lacp_agg_port_attach))
    {
      ret =  HSL_IFMGR_HWCB_CALL(hw_if_lacp_agg_port_attach) (agg_ifp,port_ifp);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                   "Failed to attach port %d to trunk %s, "
                   "error %d\n", port_ifindex, agg_name, ret);
          hsl_ifmgr_aggregator_port_detach (agg_name, agg_ifindex, port_ifindex);
        }
    }
  HSL_IFMGR_IF_REF_DEC (agg_ifp);
  HSL_IFMGR_IF_REF_DEC (port_ifp);
  HSL_FN_EXIT(ret);
}

/* 
   Remove port from aggregator. 
*/
int
hsl_ifmgr_aggregator_port_detach (char *agg_name, u_int32_t agg_ifindex,
                                  u_int32_t port_ifindex)
{
  struct hsl_if *agg_ifp; 
  struct hsl_if *port_ifp;
  int ret;

  HSL_FN_ENTER();
  /* Get aggregator port data */
  if (agg_ifindex > 0)
    agg_ifp = hsl_ifmgr_lookup_by_index (agg_ifindex);
  else
    agg_ifp = hsl_ifmgr_lookup_by_name (agg_name);

  /*
    If no aggregator found return an error. 
  */ 
  if (! agg_ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to bind port %d to trunk %s, "
               "trunk doesn't exist \n", port_ifindex, agg_name);
      HSL_FN_EXIT(HSL_IFMGR_ERR_IF_NOT_FOUND);
    }


  /* Get constituent port data */
  port_ifp = hsl_ifmgr_lookup_by_index (port_ifindex);
  if (!port_ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
               "Failed to bind port %d to trunk %s, "
               "port doesn't exist \n", port_ifindex, agg_name);
      HSL_IFMGR_IF_REF_DEC (agg_ifp);
      HSL_FN_EXIT(HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* 
     Make sure port & aggregator have the same interface type. 
  */
  if (agg_ifp->type != port_ifp->type)
    {
      HSL_IFMGR_IF_REF_DEC (port_ifp);
      HSL_IFMGR_IF_REF_DEC (agg_ifp);
      HSL_FN_EXIT(HSL_IFMGR_ERR_INVALID_MODE);
    }

#ifdef HAVE_VLAN
  if (agg_ifp->type == HSL_IF_TYPE_L2_ETHERNET)
    {
      /* Update port agg vlan hierarchy. */
      hsl_vlan_agg_port_bind_update(port_ifp, agg_ifp, HSL_FALSE);
    }
#endif /* HAVE_VLAN */
  /* detach the individual port from aggregator */
  ret = hsl_ifmgr_unbind2 (agg_ifp, port_ifp);
  if (ret < 0)
    {
      HSL_IFMGR_IF_REF_DEC (port_ifp);
      HSL_IFMGR_IF_REF_DEC (agg_ifp);
      return ret;
    }

#ifdef HAVE_VLAN
  /* Now unbind the aggregator from vlan interface if the last child 
     has been removed */
  if ((agg_ifp->children_list == NULL) &&
      (agg_ifp->type == HSL_IF_TYPE_L2_ETHERNET))
    {
      /* Remove agg from vlan childrens list. */
      hsl_vlan_agg_unbind(agg_ifp);
    }

#endif /* HAVE_VLAN */

  /* Detach port in hw. */
  if (HSL_IFMGR_HWCB_CHECK(hw_if_lacp_agg_port_detach))
    {
      ret =  HSL_IFMGR_HWCB_CALL(hw_if_lacp_agg_port_detach) (agg_ifp,port_ifp);
      if (ret < 0)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR,
                   "Failed to detach port %d from trunk %s, "
                   "error %d\n", port_ifindex, agg_name, ret);
        }
    }
  HSL_IFMGR_IF_REF_DEC (port_ifp);
  HSL_IFMGR_IF_REF_DEC (agg_ifp);
  HSL_FN_EXIT(ret);
}
#endif /* HAVE_LACPD */

#ifdef HAVE_MPLS
/* 
   Create L3 interface.
*/
int
hsl_ifmgr_mpls_create (u_char *hwaddr, int hwaddrlen,
                       void *data, struct hsl_if **ppifp)
{
  struct hsl_if *ifp;
  hsl_ifIndex_t idx;
  int ret;
  void *sysifp = NULL;

  HSL_FN_ENTER ();

  ifp = hsl_ifmgr_if_new ();
  if (! ifp)
    HSL_FN_EXIT (HSL_IFMGR_ERR_MEMORY);

  /* set type */
  ifp->type = HSL_IF_TYPE_MPLS;

  /* Get new index for this port. */
  if (p_hsl_if_db->cm_cb && p_hsl_if_db->cm_cb->cust_if_alloc_ifindex)
    {
      ret = (*p_hsl_if_db->cm_cb->cust_if_alloc_ifindex) (ifp, &idx);
      if(ret < 0) 
        HSL_FN_EXIT (HSL_IFMGR_ERR_INDEX);
    }
  else
    idx = _hsl_ifmgr_get_mpls_ifindex ();

  if (idx == 0)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INDEX);

  /* Set index. */
  ifp->ifindex = idx;

  /* Set name. */
  sprintf (ifp->name, "mpls%d", idx);

  /* Set acceptable packet types for this interface. */
  ifp->pkt_flags = 0;

  /* set vlan id */
  ifp->u.mpls.vid = 1;

  /* set mac address */
  if (hwaddr)
    memcpy (ifp->u.mpls.mac, hwaddr, hwaddrlen);

  if (data)
    {
      ifp->u.mpls.label_info = oss_malloc (sizeof (struct hal_msg_mpls_ftn_add),
                                           OSS_MEM_HEAP);
      if (! ifp->u.mpls.label_info)
        HSL_FN_EXIT (-1);
      
      memcpy (ifp->u.mpls.label_info, data, sizeof (struct hal_msg_mpls_ftn_add));
    }

  /* Create the system interface structure. */
  if (p_hsl_if_db->hw_cb && p_hsl_if_db->hw_cb->hw_mpls_if_configure)
    sysifp = (*p_hsl_if_db->hw_cb->hw_mpls_if_configure) (ifp, NULL);
  if (! sysifp)
    {
      ret = HSL_IFMGR_ERR_SYSTEM_L3;
      goto CLEANUP;
    }
      
  /* Set system info pointer. */
  ifp->system_info = sysifp;

  /* Create interface in ifmgr. */
  ret = _hsl_ifmgr_create_interface (ifp, ppifp, 0, HSL_TRUE, HSL_FALSE);
  if(ret != 0) 
    goto CLEANUP; 

#ifdef HAVE_VRRP
  /* Global List Pointers */
  ifp->vrrp_vip_tree = NULL;
#ifdef HAVE_IPV6
  ifp->vrrp_vip6_tree = NULL;
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */

  HSL_FN_EXIT (0);
 
 CLEANUP:
  hsl_ifmgr_if_free (ifp);
  HSL_FN_EXIT (ret);
}


/*
  Delete mpls interface
*/
void
hsl_ifmgr_mpls_delete (struct hsl_if *ifp)
{
  int ret;
  struct hsl_if *ifp2;
  struct hsl_if_list *node;
  struct hsl_if_list *node_next;

  HSL_FN_ENTER ();

  if (! ifp)
    HSL_FN_EXIT ();

  ret = HSL_IFMGR_IF_REF_DEC_AND_TEST (ifp);
  if (ret)
    {
      /* Unconfigure mpls interface from system. */
      if (p_hsl_if_db->hw_cb && p_hsl_if_db->hw_cb->hw_mpls_if_unconfigure)
        (*p_hsl_if_db->hw_cb->hw_mpls_if_unconfigure) (ifp);
      
      HSL_IFMGR_LOCK;
      
      /* Delete this interface from childrens parent list. */
      node = ifp->children_list;
      while (node)
        {
          node_next = node->next;
          ifp2 = node->ifp;
          _hsl_ifmgr_delete_from_list (&ifp2->parent_list, ifp);
          oss_free (node, OSS_MEM_HEAP); 
          node = node_next;
        }
      ifp->children_list = NULL;
 
      /* Delete this interface from parent's children list. */
      node = ifp->parent_list;
      while (node)
        {
          node_next = node->next;
          ifp2 = node->ifp;
          
          _hsl_ifmgr_delete_from_list (&ifp2->children_list, ifp);
          
          /* If the last port for the aggregator interface is deleted, 
             the aggregator is inoperable. Send a flags change. */
          if (! ifp2->children_list)
            {
              /* Unset running flag. */
              ifp2->flags &= ~IFF_RUNNING;
              
              /* Call notifier for flags change. */
              _hsl_ifmgr_notify_chain (&p_hsl_if_db->chain, HSL_IF_EVENT_IFFLAGS, ifp2, 
                                       NULL);
            }
          
          oss_free (node, OSS_MEM_HEAP); 
          node = node_next;
        }
      
      ifp->parent_list = NULL;

      HSL_IFMGR_UNLOCK;
      
      /* Free ifndex for this port. */
      if (p_hsl_if_db->cm_cb && p_hsl_if_db->cm_cb->cust_if_free_ifindex)
        (*p_hsl_if_db->cm_cb->cust_if_free_ifindex) (ifp->ifindex);
      
      if (ifp->u.mpls.label_info)
        {
          oss_free (ifp->u.mpls.label_info, OSS_MEM_HEAP);
          ifp->u.mpls.label_info = NULL;
        }

      _hsl_ifmgr_delete_interface (ifp, HSL_FALSE, HSL_FALSE);
    }

  HSL_FN_EXIT ();
}
#endif /* HAVE_MPLS */
