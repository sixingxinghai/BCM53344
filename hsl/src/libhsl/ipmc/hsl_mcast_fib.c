/* Copyright (C) 2002-2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 || defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ether.h"
#include "hsl_avl.h"
#include "hsl_error.h"
#include "hsl_table.h"
#include "hsl_ether.h"

#include "hsl_ifmgr.h"
#ifdef HAVE_L2
#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#endif /* HAVE_L2 */ 
#include "hsl_mcast_fib.h"
#include "hsl_fib_mc_os.h"
#include "hsl_fib_mc_hw.h"


/* 
   Mc interface manager database. 
*/
static struct hsl_ipv4_mc_db *p_hsl_ipv4_mc_db = NULL;
static int hsl_ipv4_mc_db_initialized = HSL_FALSE;
#ifdef HAVE_IPV6
static struct hsl_ipv6_mc_db *p_hsl_ipv6_mc_db = NULL;
static int hsl_ipv6_mc_db_initialized = HSL_FALSE;;
#endif /* HAVE_IPV6 */

#ifdef HAVE_IGMP_SNOOP
static u_char hsl_igmp_multicast_mac_addr[6] = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x00};
#endif /* HAVE_IGMP_SNOOP */

/* Forward declarations. */
int hsl_ipv4_mc_hw_cb_register (void);

#ifdef HAVE_MCAST_IPV4
int hsl_ipv4_mc_os_cb_register (void);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_IPV6 

int hsl_ipv6_mc_hw_cb_register (void);

#ifdef HAVE_MCAST_IPV6
int hsl_ipv6_mc_os_cb_register (void);
#endif /* HAVE_MCAST_IPV6 */

#endif /* HAVE_IPV6 */
/* 
   Static functions declarations. 
*/
static int
hsl_mc_fib_key_init(void *p_group, void *p_src, u_char family, struct hsl_mc_fib_entry *p_grp_src_entry);
static int
_hsl_mc_fib_vlan_cmp(void *param1, void *param2);
static int
_hsl_mc_fib_if_cmp(void *param1, void *param2);
static int
_hsl_mc_fib_src_grp_cmp(void *param1, void *param2);
static int
_hsl_mc_igs_update_hw(struct hsl_avl_node *node, struct hsl_mc_fib_entry *p_key_grp_src_entry);
static int
_hsl_mc_fib_vlan_entry_alloc(struct hsl_mc_vlan_entry **pp_vlan_entry);
static void
_hsl_mc_fib_vlan_entry_free(void *ptr);
static int
_hsl_mc_vlan_add_l2_if (struct hsl_mc_vlan_entry  *p_vlan_entry,
                        hsl_ifIndex_t ifindex,
                        struct hsl_mc_eggress_if_entry **pp_if_entry,
                        char is_exclude);
static struct hsl_mc_fib_entry *
_hsl_mc_fib_entry_add(struct hsl_mc_fib_entry *p_new_grp_src_entry);
static int
_hsl_mc_fib_entry_del(struct hsl_mc_fib_entry *p_grp_src_entry);
static struct hsl_mc_vlan_entry*
_hsl_mc_fib_vlan_entry_add(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid);
static int
_hsl_mc_fib_vlan_entry_del(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid);
static void
_hsl_mc_fib_update_flags (struct hsl_mc_fib_entry *p_grp_src_entry,
                            struct hsl_mc_eggress_if_entry *p_egress_if,
                            u_int16_t flags, HSL_BOOL oper);
static int
_hsl_mc_fib_add_l2_if(struct hsl_mc_fib_entry *p_grp_src_entry,
                         hsl_ifIndex_t ifindex, hsl_vid_t vid,
                         u_int16_t flags, u_char is_exclude);
static int
_hsl_mc_fib_del_l2_if(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_ifIndex_t ifindex,
                      hsl_vid_t vid, u_int16_t flags);

static int
_hsl_mc_igs_add_del_if(struct hsl_avl_node *node,struct hsl_mc_fib_entry *p_key_grp_src_entry,
                      u_int16_t vid, u_int32_t ifindex, u_int16_t flags, HSL_BOOL oper, u_char is_exclude);
static void
_hsl_mc_fib_show (struct hsl_mc_fib_entry *p_grp_src_entry);

static HSL_BOOL
_hsl_mc_fib_check_if_node_is_empty(struct hsl_mc_fib_entry *p_grp_src_entry);

#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
static int
_hsl_mc_fib_add_l3_if(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_ifIndex_t ifindex, u_char ttl);
static int
_hsl_mc_fib_del_l3_if(struct hsl_mc_fib_entry *p_grp_src_entry,
                         hsl_ifIndex_t ifindex);
static void
_hsl_mc_v4_fib_dump(struct hsl_avl_node *node);
#ifdef HAVE_MCAST_IPV6
static void
_hsl_mc_v6_fib_dump(struct hsl_avl_node *node);

#endif /* HAVE_MCAST_IPV6 */

static HSL_BOOL
hsl_mc_vif_exists (u_char family, struct hsl_if *ifp);

#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

/* 
   Init v4 database. 
*/
int 
hsl_ipv4_mc_db_init()
{
  int ret;
#ifdef HAVE_MCAST_IPV4
  int index; 
#endif /* HAVE_MCAST_IPV4 */ 

  HSL_FN_ENTER(); 

  /* If IPv4 MC db initialized, return. */
  if (hsl_ipv4_mc_db_initialized)
    HSL_FN_EXIT (0);

  /* Allocate database. */
  p_hsl_ipv4_mc_db = oss_malloc(sizeof(struct hsl_ipv4_mc_db), OSS_MEM_HEAP); 
  if(!p_hsl_ipv4_mc_db)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INIT);

  /* Create semaphore. */
  ret = oss_sem_new ("V4 MCAST MUTEX", OSS_SEM_MUTEX, 0, NULL, &p_hsl_ipv4_mc_db->mutex);
  if (ret < 0)
    {
      hsl_ipv4_mc_db_deinit();
      HSL_FN_EXIT (-1);
    }

  /* Initialize IPv4 (s,g) tree. */
  ret = hsl_avl_create (&HSL_MCAST_FIB_V4_GS_TREE, 0, _hsl_mc_fib_src_grp_cmp);
  if (ret < 0)
    {
      hsl_ipv4_mc_db_deinit();
      HSL_FN_EXIT (-1);
    }
#ifdef HAVE_MCAST_IPV4
  /* Reset vif table. */
  for (index = 0; index < HSL_MAX_VIFS; index ++)
    p_hsl_ipv4_mc_db->vif_entry[index].valid = HSL_FALSE;

  /* Set registered vif to illegal one. */
  p_hsl_ipv4_mc_db->registered_vif = -1;

  /* Set pim running to false. */
  p_hsl_ipv4_mc_db->pim_running = HSL_FALSE;
   
  /*  Register os callbacks. */
  hsl_ipv4_mc_os_cb_register ();
#endif /* HAVE_MCAST_IPV4 */

  /*  Register hw callbacks. */
  hsl_ipv4_mc_hw_cb_register ();

  /* IPv4 MC db initialized. */
  hsl_ipv4_mc_db_initialized = HSL_TRUE;
 
  HSL_FN_EXIT(0);
}
/* 
   Deinit v4 database. 
*/
int 
hsl_ipv4_mc_db_deinit()
{
  HSL_FN_ENTER();

  if (! hsl_ipv4_mc_db_initialized)
    HSL_FN_EXIT (0);

  if(p_hsl_ipv4_mc_db)
    {
      /* Flush group source tree. */  
      if(HSL_MCAST_FIB_V4_GS_TREE)
        hsl_avl_tree_free(&HSL_MCAST_FIB_V4_GS_TREE,hsl_mc_fib_entry_free);

      /* Delete semaphore. */
      oss_sem_delete (OSS_SEM_MUTEX, p_hsl_ipv4_mc_db->mutex);

      /*  Unregister hw callbacks. */
      hsl_ipv4_mc_hw_cb_unreg();
#ifdef  HAVE_MCAST_IPV4
      /*  Unregister os callbacks. */
      hsl_ipv4_mc_os_cb_unreg();
#endif /* HAVE_MCAST_IPV4 */
      /* Free database. */

      oss_free(p_hsl_ipv4_mc_db,OSS_MEM_HEAP);
      p_hsl_ipv4_mc_db = NULL;
    }
  HSL_FN_EXIT (0);
}

#ifdef HAVE_IPV6
/* 
   Init v6 database. 
*/
int 
hsl_ipv6_mc_db_init()
{
  int ret;
#ifdef HAVE_MCAST_IPV6
  int index; 
#endif /* HAVE_MCAST_IPV6 */ 

  HSL_FN_ENTER(); 

  /* IPv6 MC DB initialized? */
  if (hsl_ipv6_mc_db_initialized)
    HSL_FN_EXIT (0);

  /* Allocate database. */
  p_hsl_ipv6_mc_db = oss_malloc(sizeof(struct hsl_ipv6_mc_db), OSS_MEM_HEAP); 
  if(!p_hsl_ipv6_mc_db)
    HSL_FN_EXIT (HSL_IFMGR_ERR_INIT);

  /* Create semaphore. */
  ret = oss_sem_new ("V6 MCAST MUTEX", OSS_SEM_MUTEX, 0, NULL, 
                     &p_hsl_ipv6_mc_db->mutex);
  if (ret < 0)
    {
      hsl_ipv6_mc_db_deinit();
      HSL_FN_EXIT (-1);
    }
    
  /* Initialize IPv4 (s,g) tree. */
  ret = hsl_avl_create (&HSL_MCAST_FIB_V6_GS_TREE, 0, _hsl_mc_fib_src_grp_cmp);
  if (ret < 0)
    {
      hsl_ipv6_mc_db_deinit();
      HSL_FN_EXIT (-1);
    }

#ifdef HAVE_MCAST_IPV6
  /* Reset vif database. */
  for (index = 0; index < HSL_IPV6_MAX_VIFS; index ++)
    p_hsl_ipv6_mc_db->vif_entry[index].valid = HSL_FALSE;


  /* Set registered vif to illegal one. */
  p_hsl_ipv6_mc_db->registered_vif = -1;

  /* Set pim running to false. */
  p_hsl_ipv6_mc_db->pim_running = HSL_FALSE;
   
  /*  Register os callbacks. */
  hsl_ipv6_mc_os_cb_register ();
#endif /* HAVE_MCAST_IPV6 */

  /*  Register hw callbacks. */
  hsl_ipv6_mc_hw_cb_register ();

  hsl_ipv6_mc_db_initialized = HSL_TRUE;

  HSL_FN_EXIT(0);
}

/* 
   Deinit database. 
*/
int 
hsl_ipv6_mc_db_deinit()
{
  HSL_FN_ENTER();

  if (! hsl_ipv4_mc_db_initialized)
    HSL_FN_EXIT (0);

  if(!p_hsl_ipv6_mc_db)
    {
      /* Flush group source tree. */  
      if(HSL_MCAST_FIB_V6_GS_TREE)
        hsl_avl_tree_free(&HSL_MCAST_FIB_V6_GS_TREE,hsl_mc_fib_entry_free);

      /* Delete semaphore. */
      oss_sem_delete (OSS_SEM_MUTEX, p_hsl_ipv6_mc_db->mutex);

      /*  Unregister hw callbacks. */
      hsl_ipv6_mc_hw_cb_unreg();
#ifdef HAVE_MCAST_IPV6
      /*  Unregister os callbacks. */
      hsl_ipv6_mc_os_cb_unreg();
#endif /* HAVE_MCAST_IPV6 */

      /* Free database. */
      oss_free(p_hsl_ipv6_mc_db, OSS_MEM_HEAP);

      hsl_ipv6_mc_db_initialized = HSL_FALSE;
    }

  HSL_FN_EXIT (0);
}

#endif /* HAVE_IPV6 */

/* Get access to mcast G,S tree.
  Name: hsl_mc_fib_tree_request
  Parameters:
    IN -> family      - Address family.
    IN -> pp_avl_tree - G,S tree pointer.

  Returns:
    void
*/
void
hsl_mc_fib_tree_request(int family, struct hsl_avl_tree **pp_avl_tree)
{
  HSL_FN_ENTER();

  if(AF_INET == family)
  {
     if(pp_avl_tree)
        *pp_avl_tree = HSL_MCAST_FIB_V4_GS_TREE;

     oss_sem_lock(OSS_SEM_MUTEX, p_hsl_ipv4_mc_db->mutex, OSS_WAIT_FOREVER);
  }
#ifdef HAVE_IPV6
  else if (AF_INET6 == family)
  {
     if(pp_avl_tree)
       *pp_avl_tree = HSL_MCAST_FIB_V6_GS_TREE; 
     oss_sem_lock(OSS_SEM_MUTEX, p_hsl_ipv6_mc_db->mutex, OSS_WAIT_FOREVER);
  }
#endif /* HAVE_IPV6 */
  else 
     *pp_avl_tree = NULL; 
  HSL_FN_EXIT(STATUS_OK);
}

/* Return access to mcast G,S tree.
  Name: hsl_mc_fib_tree_release
  Parameters:
    IN -> family      - Address family.
    IN -> pp_avl_tree - G,S tree pointer.

  Returns:
    void
*/
void
hsl_mc_fib_tree_release(family)
{

  HSL_FN_ENTER();

  if(AF_INET == family)
  {
     oss_sem_unlock(OSS_SEM_MUTEX, p_hsl_ipv4_mc_db->mutex);
  }
#ifdef HAVE_IPV6
  else if (AF_INET6 == family)
  {
     oss_sem_unlock(OSS_SEM_MUTEX, p_hsl_ipv6_mc_db->mutex);
  }
#endif /* HAVE_IPV6 */
  HSL_FN_EXIT(STATUS_OK);
}

/* Init Mcast Fib lookup Key.
  Name: hsl_mc_fib_key_init
  Parameters:
    IN -> p_group - Group address.
    IN -> p_src   - Source address.
    IN -> family  - Address family.
    IN -> p_grp_src_entry - Key entry to be initialized.

  Returns:
    Error - if initialization failed.
    0 otherwise.
*/
static int
hsl_mc_fib_key_init(void *p_group, void *p_src, u_char family, struct hsl_mc_fib_entry *p_grp_src_entry)
{
  HSL_FN_ENTER();

  /* Check input parameters & make sure table was initialized */
  if((!p_grp_src_entry) || (!p_group) || (!p_src))
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Init lookup key. */
  if(AF_INET ==family)
  {
     IPV4_ADDR_COPY(&p_grp_src_entry->group.u.prefix,p_group); 
     IPV4_ADDR_COPY(&p_grp_src_entry->src.u.prefix,p_src); 
  }
#ifdef HAVE_IPV6
  else if(AF_INET6 == family)
  {
     IPV6_ADDR_COPY(&p_grp_src_entry->group.u.prefix,p_group); 
     IPV6_ADDR_COPY(&p_grp_src_entry->src.u.prefix,p_src); 
  }
#endif /* HAVE_IPV6 */
  else
  {
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
  }
  HSL_MCAST_FIB_GROUP_FAMILY = HSL_MCAST_FIB_SRC_FAMILY = family; 
  HSL_FN_EXIT(STATUS_OK);
}

/*Comparision function for vlan tree.
  Name: _hsl_mc_fib_vlan_cmp 
  Parameters:
   IN -> param1 - compared data. 
   IN -> param2 - compared data. 

 Returns:
   HSL_COMP_LESS_THAN/HSL_COMP_GREATER_THAN/HSL_COMP_EQUAL 
*/
static int
_hsl_mc_fib_vlan_cmp(void *param1, void *param2)
{
  struct hsl_mc_vlan_entry *first;   /* First vlan structure.  */
  struct hsl_mc_vlan_entry *second;  /* Second vlan structure. */

  HSL_FN_ENTER();

  /* Input parameters check. */
  if((!param1) || (!param2))
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);


  /* Map compared data to real data type. */
  first  = (struct hsl_mc_vlan_entry *) param1;
  second = (struct hsl_mc_vlan_entry *) param2;

  /* Less than.   */    
  if(first->vid < second->vid)
    HSL_FN_EXIT(HSL_COMP_LESS_THAN);

  /* Greater than. */    
  if (first->vid > second->vid)
    HSL_FN_EXIT(HSL_COMP_GREATER_THAN);

  /* Equal. */
  HSL_FN_EXIT(HSL_COMP_EQUAL);
}

/* Comparision function for interfaces tree.
  Name: _hsl_mc_fib_if_cmp 
  Parameters:
    IN -> param1 - compared data. 
    IN -> param2 - compared data. 

Returns:
   HSL_COMP_LESS_THAN/HSL_COMP_GREATER_THAN/HSL_COMP_EQUAL 
*/
static int
_hsl_mc_fib_if_cmp(void *param1, void *param2)
{
  struct hsl_mc_eggress_if_entry *first;   /* First outgoint interface structure.  */
  struct hsl_mc_eggress_if_entry *second;  /* Second outgoint interface structure. */

  HSL_FN_ENTER();

  /* Input parameters check. */
  if((!param1) || (!param2))
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);


  /* Map compared data to real data type. */
  first  = (struct hsl_mc_eggress_if_entry *) param1;
  second = (struct hsl_mc_eggress_if_entry *) param2;

  /* Less than.   */    
  if(first->ifindex < second->ifindex)
    HSL_FN_EXIT(HSL_COMP_LESS_THAN);

  /* Greater than. */    
  if (first->ifindex > second->ifindex)
    HSL_FN_EXIT(HSL_COMP_GREATER_THAN);

  /* Equal. */
  HSL_FN_EXIT(HSL_COMP_EQUAL);
}

/* Comparision function for group/source tree.
  Name: _hsl_mc_fib_src_grp_cmp
  Parameters:
    IN -> param1 - compared data. 
    IN -> param2 - compared data. 

Returns:
   HSL_COMP_LESS_THAN/HSL_COMP_GREATER_THAN/HSL_COMP_EQUAL 
*/

static int
_hsl_mc_fib_src_grp_cmp(void *param1, void *param2)
{
  struct hsl_mc_fib_entry *p_grp_src_first;    /* First comared node.  */ 
  struct hsl_mc_fib_entry *p_grp_src_second;   /* Second comapred node.*/
  int result;                                  /* Comparision result.  */

  HSL_FN_ENTER();

  if((!param1) || (!param2))
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Map data to real data types. */  
  p_grp_src_first = (struct hsl_mc_fib_entry *) param1;
  p_grp_src_second = (struct hsl_mc_fib_entry *) param2;

  /* Assumption: same family type of addresseses are compared. */

  /* Compare group_id . */
  result = HSL_PREFIX_CMP(&p_grp_src_first->group,&p_grp_src_second->group);

  /* Group less than.   */    
  if(result < 0)
    HSL_FN_EXIT(HSL_COMP_LESS_THAN);

  /* Group greater than. */    
  if (result > 0)
    HSL_FN_EXIT(HSL_COMP_GREATER_THAN);

  /* Compare source */  
  result = HSL_PREFIX_CMP(&p_grp_src_first->src,&p_grp_src_second->src);

  /* Source less than.  */    
  if (result < 0)
    HSL_FN_EXIT(HSL_COMP_LESS_THAN);

  /* Source Greater than. */
  if (result > 0)
    HSL_FN_EXIT(HSL_COMP_GREATER_THAN);

  /* Equal. */
  HSL_FN_EXIT(HSL_COMP_EQUAL);
}

#ifdef HAVE_IGMP_SNOOP
/* Convert group address to a mac  
  Name: hsl_convert_ip_to_hw
  Parameters:
    IN -> addr     - Group address pointer. 
    IN -> mac_addr - Byte array to fill mac address. 
  Returns:
    void. 
*/
void
hsl_convert_ip_to_hw (hsl_ipv4Address_t *addr, u_char *mac_addr )
{
  hsl_ipv4Address_t *temp_ip_address;   /* Temp ip address.           */
  u_char group_ip[IPV4_MAX_BYTELEN];    /* Group ip addres byte array.*/   

  HSL_FN_ENTER();
  /*  Convert multicast IP address to a multicast mac address */
  temp_ip_address = ( u_int32_t *) addr;
  /* Save ip address in byte array. */
  memcpy (&group_ip, temp_ip_address, IPV4_MAX_BYTELEN);
  /* We need to copy 23 bits 
     First Copy 2 least significant bytes 16bits. 
  */
  memcpy (&mac_addr[4], &group_ip[2], 2);
  /* Get remaining 7 bits. */
  //mac_addr[3] = ( ( hsl_igmp_multicast_mac_addr[3] & 0x80 ) | ( group_ip[1] & 0x7f ) );
  mac_addr[3] = ( ( hsl_igmp_multicast_mac_addr[3] & 0x00 ) | ( group_ip[1] & 0x7f ) );
  /* Set 3 most significant bytes. */
  memcpy (mac_addr, hsl_igmp_multicast_mac_addr, 3);
  HSL_FN_EXIT(STATUS_OK);
}

#endif /* HAVE_IGMP_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP

/*  Update multicast entry in hw & os.
    In case source is not specified, function walks over 
    all the entires sharing the same group id and updates 
    entry information in HW.   
    
    Name: hsl_mc_igs_update_hw
    Parameters:
      IN -> node    - G,S tree node to if.
      IN -> p_key_grp_src_entry - Group source node information.    

    Returns:
      0 on success
      < 0 on error
*/
static int
_hsl_mc_igs_update_hw(struct hsl_avl_node *node, struct hsl_mc_fib_entry *p_key_grp_src_entry)
{
  struct hsl_mc_fib_entry *p_grp_src_entry;   /* G,S entry.              */ 
  int ret = 0;                                /* Operation status flag.  */

  HSL_FN_ENTER();
  
  if(!node)
    HSL_FN_EXIT(STATUS_OK);

  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER);

  /* 
     RECURSIVE IN SEQUENCE AVL TREE WALK. 
     DO NOT go left beyound your own group.  
     DO NOT go right beong your own group. 
  */ 

  /* Make sure we update entries for the same group. */
  ret = HSL_PREFIX_CMP(&HSL_MCAST_FIB_GROUP,&p_key_grp_src_entry->group);
  if(ret) /* Group doesn't match. */
    HSL_FN_EXIT(STATUS_OK);
  
  /* Check if left node exist & go to greater node. */
  if((HSL_AVL_NODE_LEFT(node)) && (HSL_PREFIX_IS_ZERO(&p_key_grp_src_entry->src)))
    _hsl_mc_igs_update_hw( HSL_AVL_NODE_LEFT(node), p_key_grp_src_entry);

  
  /* Check if right node exist & go to lower node. */
  if((HSL_AVL_NODE_RIGHT(node)) && (HSL_PREFIX_IS_ZERO(&p_key_grp_src_entry->src)))
    _hsl_mc_igs_update_hw( HSL_AVL_NODE_RIGHT(node), p_key_grp_src_entry);


  /* DO HW CALL */
  if(_hsl_mc_fib_check_if_node_is_empty(p_grp_src_entry))
  {
#ifdef HAVE_L3
     /* Node is empty delete route.*/
     if(AF_INET == HSL_MCAST_FIB_GROUP_FAMILY) 
        ret = hsl_ipv4_mc_route_del(p_grp_src_entry);
#ifdef HAVE_IPV6
     else if(AF_INET6 == HSL_MCAST_FIB_GROUP_FAMILY) 
        ret = hsl_ipv6_mc_route_del(p_grp_src_entry);
#endif /* HAVE_IPV6 */
#endif  /* HAVE_L3 */
     _hsl_mc_fib_entry_del(p_grp_src_entry);
  }
  else
  {
#ifdef HAVE_L3
    /* Node is not empty re-add it. */
     if(AF_INET == HSL_MCAST_FIB_GROUP_FAMILY) 
        ret = hsl_ipv4_mc_route_add(p_grp_src_entry, 0,0,NULL);
#ifdef HAVE_IPV6
     else if(AF_INET6 == HSL_MCAST_FIB_GROUP_FAMILY) 
        ret = hsl_ipv6_mc_route_add(p_grp_src_entry, 0,0,NULL);
#endif /* HAVE_IPV6 */
#endif  /* HAVE_L3 */
     /* Hw update complete. */ 
     p_grp_src_entry->do_hw_update = HSL_FALSE; 
  }

  HSL_FN_EXIT(ret);
}


/* Allocate & init vlan entry 
  Name: hsl_mc_fib_vlan_entry_alloc 
  Parameters:
    IN -> entry - Allocated data pointer.

  Returns:
    0 on success
    < 0 on error
*/

static int 
_hsl_mc_fib_vlan_entry_alloc(struct hsl_mc_vlan_entry **pp_vlan_entry)
{
  struct hsl_mc_vlan_entry *p_vlan_entry;    /* Temporary allocated entry.*/
  int ret;                                   /* Operation status.         */

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!pp_vlan_entry) 
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 
  
  /* Preset result to NULL. */
  *pp_vlan_entry = NULL;

  /* Allocate an entry. */ 
  p_vlan_entry = (struct hsl_mc_vlan_entry*)oss_malloc(sizeof(struct hsl_mc_vlan_entry),OSS_MEM_HEAP); 
  if(!p_vlan_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
 
  /* Create layer 2 if tree. */
  ret = hsl_avl_create(&HSL_MCAST_FIB_L2_IF_TREE,0,_hsl_mc_fib_if_cmp);
  if(ret < 0)
  {
    oss_free(p_vlan_entry, OSS_MEM_HEAP);     
    HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
  }

  /* Pass result to calling function. */
  *pp_vlan_entry = p_vlan_entry;
  HSL_FN_EXIT(STATUS_OK);
}

/*  Deinit & free vlan entry 

Name: _hsl_mc_vlan_entry_free
Parameters:
IN -> ptr -  multicast fib vlan entry.

Returns:
void
*/
static void 
_hsl_mc_fib_vlan_entry_free(void *ptr)
{
  struct hsl_mc_vlan_entry *p_vlan_entry;  /* Freed entry. */

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!ptr) 
    HSL_FN_EXIT(-1); 

  /* Map ptr to a real data type. */
  p_vlan_entry = (struct hsl_mc_vlan_entry *)ptr; 

  /* Flush layer 2 if tree. */  
  hsl_avl_tree_free(&HSL_MCAST_FIB_L2_IF_TREE, hsl_free);

  /* Free the entry. */ 
  oss_free(ptr,OSS_MEM_HEAP);

  HSL_FN_EXIT(STATUS_OK);
}


/*  Add l2 port/Trunk interface to a vlan node.
    Name: _hsl_mc_vlan_add_l2_if
    Parameters:
      IN -> p_vlan_entry - Vlan entry; 
      IN -> ifindex - l2 interface index for port/trunk.
      IN -> vid - vlan id 

    Returns:
      0 on success
      < 0 on error
*/

static int
_hsl_mc_vlan_add_l2_if (struct hsl_mc_vlan_entry  *p_vlan_entry,
                        hsl_ifIndex_t ifindex, 
                        struct hsl_mc_eggress_if_entry **pp_if_entry,
                        char is_exclude)
{
  struct hsl_mc_eggress_if_entry egress_if;     /* Lookup if entry.           */
  struct hsl_avl_node *node;                    /* Avl node with data.        */
  struct hsl_mc_eggress_if_entry *p_egress_if;  /* New egress interface entry.*/
  int ret;                                      /* Operation status flag.     */

  HSL_FN_ENTER();

  /* Check the input. */
  if((!p_vlan_entry) || (!pp_if_entry))
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
 
  /* Init lookup key. */
  egress_if.ifindex = ifindex; 

  /* Search if ifindex is already part of vlan tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_L2_IF_TREE, (void *)&egress_if);

  /* If node already exits just return egress if pointer to caller. */
  if(node)
   {
      *pp_if_entry = (struct hsl_mc_eggress_if_entry *) HSL_AVL_NODE_INFO (node); 
      HSL_FN_EXIT(STATUS_OK);
   }

  /* Allocate an entry. */ 
  p_egress_if = (struct hsl_mc_eggress_if_entry *)
                 oss_malloc(sizeof(struct hsl_mc_eggress_if_entry),OSS_MEM_HEAP); 

  if(!p_egress_if)
    HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);

  /* Set the entry data. */
  p_egress_if->ifindex = ifindex;

  /* Set the filter Mode */
  p_egress_if->is_exclude = is_exclude;

  /* Add interface to G,S interface  tree. */ 
  ret = hsl_avl_insert (HSL_MCAST_FIB_L2_IF_TREE,(void *)p_egress_if);
  if(ret < 0)
  {
    oss_free (p_egress_if, OSS_MEM_HEAP);
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }
  *pp_if_entry = p_egress_if;
  HSL_FN_EXIT(ret); 
}

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

/* Allocate & init G,S mcast entry 
   Name: hsl_mc_fib_entry_alloc 
   Parameters:
    IN -> pp_grp_src_entry - Allocated data pointer.

   Returns:
     0 on success
     < 0 on error
*/
int 
hsl_mc_fib_entry_alloc(struct hsl_mc_fib_entry **pp_grp_src_entry)
{
  struct hsl_mc_fib_entry *p_grp_src_entry;   /* Temporary allocated entry.*/
  int ret;                                    /* Operation status.         */

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!pp_grp_src_entry) 
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 
  
  /* Preset result to NULL. */
  *pp_grp_src_entry = NULL;

  /* Allocate an entry. */ 
  p_grp_src_entry = (struct hsl_mc_fib_entry *)oss_malloc(sizeof(struct hsl_mc_fib_entry),OSS_MEM_HEAP); 
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
 
  /* Create layer 2 if tree. */
  ret = hsl_avl_create(&HSL_MCAST_FIB_VLAN_TREE,0,_hsl_mc_fib_vlan_cmp);
  if(ret < 0)
  {
    oss_free(p_grp_src_entry,OSS_MEM_HEAP);     
    HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
  }

  /* Create layer 3 if tree. */
  ret = hsl_avl_create(&HSL_MCAST_FIB_L3_IF_TREE,0,_hsl_mc_fib_if_cmp);
  if(ret < 0)
  {
    hsl_avl_tree_free(&HSL_MCAST_FIB_VLAN_TREE, hsl_free);
    oss_free(p_grp_src_entry, OSS_MEM_HEAP);
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER);
  }

  /* Pass result to calling function. */
  *pp_grp_src_entry = p_grp_src_entry;
  HSL_FN_EXIT(STATUS_OK);
}

/* Deinit & free G,S mcast entry 
   Name: hsl_mc_fib_entry_free
   Parameters:
     IN -> ptr -  multicast fib entry.

   Returns:
     void
*/
void 
hsl_mc_fib_entry_free(void *ptr)
{
  struct hsl_mc_fib_entry *p_grp_src_entry;  /* Freed entry. */

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!ptr)
    HSL_FN_EXIT(-1); 

  /* Map ptr to a real data type. */
  p_grp_src_entry = (struct hsl_mc_fib_entry *)ptr; 

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  /* Flush layer 2 vlan tree. */  
  hsl_avl_tree_free(&HSL_MCAST_FIB_VLAN_TREE,_hsl_mc_fib_vlan_entry_free);
#endif /* HAVE_IGMP_SNOOP  || HAVE_MLD_SNOOP */

  /* Flush layer 3 ifindexes tree. */ 
  hsl_avl_tree_free(&HSL_MCAST_FIB_L3_IF_TREE, hsl_free);
 
  /* Free the entry. */ 
  oss_free(ptr,OSS_MEM_HEAP);

  HSL_FN_EXIT(STATUS_OK);
}

/*  Add multicast entry.
  Name: hsl_mc_fib_entry_add
  Parameters:
    IN -> p_new_grp_src_entry - group/src for added address.

  Returns:
    NULL - if addition failed;
    entry -if entry was successfully added.
*/

static struct hsl_mc_fib_entry *
_hsl_mc_fib_entry_add(struct hsl_mc_fib_entry *p_new_grp_src_entry)
{
  struct hsl_avl_node *node;                  /* Avl node with data.          */
  struct hsl_avl_tree *p_avl_tree;            /* Avl tree entry to be updated.*/
  struct hsl_mc_fib_entry *p_grp_src_entry;/* New entry pointer.           */
  int ret;                                    /* Operation status flag.       */

  HSL_FN_ENTER();

  /* Get G,S tree pointer & lock proper database. */
  hsl_mc_fib_tree_request(p_new_grp_src_entry->group.family, &p_avl_tree);
  if(!p_avl_tree)
    HSL_FN_EXIT(NULL);

  /* Search avl tree. */ 
  node = hsl_avl_lookup (p_avl_tree, (void *)p_new_grp_src_entry);

  /* Check if entry already exists. */ 
  if(node)
  {
    /* Unlock mc fib database. */ 
    hsl_mc_fib_tree_release(p_new_grp_src_entry->group.family);
    HSL_FN_EXIT((struct hsl_mc_fib_entry *)HSL_AVL_NODE_INFO (node)); 
  }

  /* Allocate a new node. */
  ret = hsl_mc_fib_entry_alloc(&p_grp_src_entry);
  if(ret < 0)
    {
      /* Failed allocate the entry. */ 
      hsl_mc_fib_tree_release(p_new_grp_src_entry->group.family);
      HSL_FN_EXIT(NULL);
    }

  /* Set entry key. */
  p_grp_src_entry->group = p_new_grp_src_entry->group; 
  p_grp_src_entry->src = p_new_grp_src_entry->src; 

  /* Insert the entry to mc fib database. */ 
  ret = hsl_avl_insert (p_avl_tree,(void *)p_grp_src_entry);
  if(ret < 0)
  {
    /* Failed to insert the entry. */ 
    hsl_mc_fib_entry_free(p_grp_src_entry);
    p_grp_src_entry = NULL;
  }

  hsl_mc_fib_tree_release(p_new_grp_src_entry->group.family);
  /* Return new entry.*/
  HSL_FN_EXIT(p_grp_src_entry); 
}


/*  Delete  multicast entry.
  Name: hsl_mc_fib_entry_del
  Parameters:
    IN -> p_grp_src_entry - group/src for added address.
  Returns:
    0 on success
    < 0 on error
*/
static int
_hsl_mc_fib_entry_del(struct hsl_mc_fib_entry *p_grp_src_entry)
{
  int ret;                                    /* Operation status flag.       */
  struct hsl_avl_tree *p_avl_tree;            /* Avl tree entry to be updated.*/
  struct hsl_avl_node *node;                  /* Avl node with data.          */
  struct hsl_mc_fib_entry *p_data_ptr;        /* Allocated node data.         */

  HSL_FN_ENTER();

  /* Get G,S tree pointer & lock proper database. */
  hsl_mc_fib_tree_request(HSL_MCAST_FIB_GROUP_FAMILY, &p_avl_tree);
  if(!p_avl_tree)
    HSL_FN_EXIT(STATUS_ERROR);

  /* Search avl tree. */
  node = hsl_avl_lookup (p_avl_tree, (void *)p_grp_src_entry);

  /* Check if entry exists. */
  if(!node)
  {
    /* Unlock mc fib database. */
    hsl_mc_fib_tree_release(HSL_MCAST_FIB_GROUP_FAMILY);
    /* Entry not there -> nothing to delete.*/
    HSL_FN_EXIT(STATUS_OK);
  }

  p_data_ptr = (struct hsl_mc_fib_entry *)HSL_AVL_NODE_INFO (node);
 
  ret = hsl_avl_delete (p_avl_tree, (void *)p_grp_src_entry);

  /* Unlock the database asap. */ 
  hsl_mc_fib_tree_release(HSL_MCAST_FIB_GROUP_FAMILY);

  /* Free entry data. */  
  hsl_mc_fib_entry_free(p_data_ptr);
 
  HSL_FN_EXIT(ret); 
}

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP

/* Add a vlan to  multicast entry.
   Name: _hsl_mc_fib_vlan_entry_add
   Parameters:
     IN -> p_grp_src_entry - G,S entry vlan being added to.
     IN -> vid - vlan id.

   Returns:
     NULL - if addition failed;
     entry -if entry was successfully added.
*/

static struct hsl_mc_vlan_entry*
_hsl_mc_fib_vlan_entry_add(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid)
{
  struct hsl_avl_node *node;             /* Avl node with data.   */
  struct hsl_mc_vlan_entry vlan_entry;   /* Lookup entry data.    */ 
  struct hsl_mc_vlan_entry *p_vlan_entry;/* New entry pointer.    */ 
  int ret;                               /* Operation status flag.*/

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!p_grp_src_entry)
    HSL_FN_EXIT(NULL);

  /* Init lookup key. */
  vlan_entry.vid = vid;

  /* Search avl tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_VLAN_TREE, (void *)&vlan_entry);

  /* Check if entry already exists. */ 
  if(node)
  {
    /* Unlock mc fib database. */ 
    HSL_FN_EXIT((struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (node)); 
  }

  /* Allocate a new node. */
  ret = _hsl_mc_fib_vlan_entry_alloc(&p_vlan_entry);
  if(ret < 0)
  {
    /* Failed allocate the entry. */ 
     HSL_FN_EXIT(NULL);
  }

  /* Init entry vlan id */
  p_vlan_entry->vid = vid;

  /* Insert the entry to group/source entry l2 interfaces vlan database. */ 
  ret = hsl_avl_insert (HSL_MCAST_FIB_VLAN_TREE,(void *)p_vlan_entry);
  if(ret < 0)
  {
    /* Failed to insert the entry. */ 
    _hsl_mc_fib_vlan_entry_free(p_vlan_entry);
    HSL_FN_EXIT(NULL); 
  }
  /* Return new entry.*/
  HSL_FN_EXIT(p_vlan_entry); 
}


/*  Delete  vlan multicast entry.
  Name: _hsl_mc_fib_vlan_entry_del
  Parameters:
    IN -> p_grp_src_entry - G,S entry vlan being added to.
    IN -> vid - vlan id.

  Returns:
     0 on success
     < 0 on error
*/
static int
_hsl_mc_fib_vlan_entry_del(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid)
{
  struct hsl_avl_node *node;                  /* Avl node with data.   */
  struct hsl_mc_vlan_entry vlan_entry;        /* Lookup entry data.    */ 
  struct hsl_mc_vlan_entry *p_vlan_entry;     /* New entry pointer.    */ 
  int ret;                                    /* Operation status flag.*/

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Init lookup key. */
  vlan_entry.vid = vid;

  /* Search avl tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_VLAN_TREE, (void *)&vlan_entry);

  /* Check if entry exists. */ 
  if(!node)
  {
    /* Entry not there -> nothing to delete.*/
    HSL_FN_EXIT(STATUS_OK); 
  }

  p_vlan_entry = (struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (node);

  ret = hsl_avl_delete (HSL_MCAST_FIB_VLAN_TREE, (void *)p_vlan_entry);

  /* Free entry data. */  
  _hsl_mc_fib_vlan_entry_free(p_vlan_entry);
 
  HSL_FN_EXIT(ret); 
}

/* Get vlan multicast entry.
   Name: hsl_mc_fib_vlan_entry_get
   Parameters:
     IN -> p_grp_src_entry - G,S entry vlan being added to.
     IN -> vid - vlan id.
   Returns:
     NULL if vlan entry was not found.
     vlan_entry if found.
*/
struct hsl_mc_vlan_entry *
hsl_mc_fib_vlan_entry_get(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid)
{
  struct hsl_avl_node *node;                  /* Avl node with data.   */
  struct hsl_mc_vlan_entry vlan_entry;        /* Lookup entry data.    */ 

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!p_grp_src_entry)
    HSL_FN_EXIT(NULL);

  /* Init lookup key. */
  vlan_entry.vid = vid;

  /* Search avl tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_VLAN_TREE, (void *)&vlan_entry);

  /* Check if entry exists. */ 
  if(!node)
    HSL_FN_EXIT(NULL); 

  HSL_FN_EXIT((struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (node));
}

#endif /* HAVE_IGMP_SNOOP  || HAVE_MLD_SNOOP */

/* Update l2 port/Trunk interface igmp version flags & counter.
   Name: _hsl_mc_fib_update_flags
   Parameters:
     IN -> p_grp_src_entry - fib entry; 
     IN -> p_egress_if - l2 interface data for port/trunk.
     IN -> flags - Entry flags. 
     IN -> oper - Operation Add/Delete.
   Returns:
     void.
*/

static void
_hsl_mc_fib_update_flags(struct hsl_mc_fib_entry *p_grp_src_entry, 
                         struct hsl_mc_eggress_if_entry *p_egress_if,
                         u_int16_t flags, HSL_BOOL oper)
{
  HSL_FN_ENTER(); 

  /* Prarmeters check.*/
  if((!p_grp_src_entry) || (!p_egress_if)) 
    HSL_FN_EXIT(-1);        

   
  if(HSL_MCAST_FIB_ADD == oper)
  {
    if(flags == HSL_MCAST_FIB_ACTUAL)
    {
      /* Check if entry was learned as V3 */
      if(!(p_egress_if->u.l2.flags & HSL_MCAST_FIB_ACTUAL))
        p_grp_src_entry->actual_joins_cntr++;
    }

    /* Update entry igmp version. */
    p_egress_if->u.l2.flags |= flags;
  }
  else if (HSL_MCAST_FIB_DEL == oper)
  {
    if(flags == HSL_MCAST_FIB_ACTUAL)
    {
      /* Check if entry was learned as V3 */
      if((p_egress_if->u.l2.flags & HSL_MCAST_FIB_ACTUAL) &&
         (p_grp_src_entry->actual_joins_cntr))
        p_grp_src_entry->actual_joins_cntr--;
    }

    /* Update entry learn method. */
    p_egress_if->u.l2.flags &= ~(flags);
  }
  HSL_FN_EXIT(STATUS_OK);
}

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP

/* Add l2 port/Trunk interface to multicast entry.
   Name: hsl_mc_fib_add_l2_if
   Parameters:
     IN -> entry - fib entry; 
     IN -> ifindex - l2 interface index for port/trunk.
     IN -> flags

   Returns:
     0 on success
     < 0 on error
*/

static int
_hsl_mc_fib_add_l2_if(struct hsl_mc_fib_entry *p_grp_src_entry, 
                      hsl_ifIndex_t ifindex, hsl_vid_t vid,
                      u_int16_t flags, u_char is_exclude)
{
  struct hsl_mc_eggress_if_entry *p_egress_if; /* New egress interface pointer.*/
  struct hsl_mc_vlan_entry *p_vlan_entry;      /* Vlan tree entry.             */
  int ret;                                     /* Operation status flag.       */

  HSL_FN_ENTER();

  /* Check the input. */
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  p_vlan_entry= _hsl_mc_fib_vlan_entry_add(p_grp_src_entry,vid);
  if(!p_vlan_entry) 
   {
      hsl_mc_fib_log_msg(HSL_LEVEL_ERROR, p_grp_src_entry,"Failed to add vlan (%d)",vid);
      HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER);
   }
 
  /* Add an entry for vlan,interface */ 
  ret = _hsl_mc_vlan_add_l2_if(p_vlan_entry, ifindex, &p_egress_if, is_exclude);
  if(ret < 0)
  {
    hsl_mc_fib_log_msg(HSL_LEVEL_ERROR, p_grp_src_entry,"Failed to add interface %d,vlan (%d)",ifindex,vid);
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER);
  }

  /* Update igmp version. */
  _hsl_mc_fib_update_flags(p_grp_src_entry, p_egress_if, flags, HSL_MCAST_FIB_ADD);  

  HSL_FN_EXIT(ret); 
}

/* Delete l2 port/Trunk interface from multicast entry.
   Name: _hsl_mc_fib_del_l2_if
   Parameters:
     IN -> p_grp_src_entry - fib entry; 
     IN -> ifindex - l2 interface index for port/trunk.
     IN -> vid - Vlan id.
     IN -> flags - Flags for entry removed. 

   Returns:
     0 on success
     < 0 on error
*/

static int
_hsl_mc_fib_del_l2_if(struct hsl_mc_fib_entry *p_grp_src_entry,
                      hsl_ifIndex_t ifindex, hsl_vid_t vid,
                      u_int16_t flags)
{
  struct hsl_avl_node *node;                  /* Avl node with data.     */
  struct hsl_mc_eggress_if_entry *p_egress_if;/* Egress interface pointer.  */
  struct hsl_mc_vlan_entry *p_vlan_entry;     /* Vlan node pointer.         */
  struct hsl_mc_eggress_if_entry egress_if;   /* Lookup egress interface.*/

  HSL_FN_ENTER();

  /* Check input params. */
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Search for vid in vlan tree. */ 
  p_vlan_entry = hsl_mc_fib_vlan_entry_get(p_grp_src_entry, vid);

  if(!p_vlan_entry)
  {
    /* Nothing to delete.*/
    HSL_FN_EXIT(STATUS_OK);
  }

  /* Delete the entry itself. */
  egress_if.ifindex = ifindex;

  /* Search if interface is already part of (G,S) interface tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_L2_IF_TREE, (void *)&egress_if);

  /* Check if entry exists. */ 
  if(!node)
  {
    /* Nothing to delete.*/
    HSL_FN_EXIT(STATUS_OK);
  }

  /* Get entry data pointer.*/
  p_egress_if =  (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (node); 

  /* Update counters if necessary */ 
  _hsl_mc_fib_update_flags(p_grp_src_entry, p_egress_if, flags, HSL_MCAST_FIB_DEL);  

  /* Delete  interface from vlan interfaces tree. */ 
  if(p_egress_if->u.l2.flags) 
  {
    /* There are still joins with different igmp version. */ 
    HSL_FN_EXIT(STATUS_OK);
  }

  /* No more joins delete the entry. */
  hsl_avl_delete(HSL_MCAST_FIB_L2_IF_TREE,(void *)p_egress_if);

  /* Free entry data.*/
  oss_free (p_egress_if, OSS_MEM_HEAP);

  /* Update vlan,* entry counter & delete it if necessary */ 
  if(!hsl_avl_get_tree_size(HSL_MCAST_FIB_L2_IF_TREE))
  {
     /* No more interfaces for this vlan -> delete it. */
     _hsl_mc_fib_vlan_entry_del(p_grp_src_entry, vid);
     /* If route is layer 2 delete it from hw. */ 
     if(! hsl_avl_get_tree_size(HSL_MCAST_FIB_L3_IF_TREE) &&
        ! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY))
       hsl_l2_mc_route_delete(p_grp_src_entry, vid);  
  }
  HSL_FN_EXIT(STATUS_OK);
}

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

/*  Check Group,Source node validity after interface deletion. 
  Name: _hsl_mc_fib_check_if_node_is_empty
  Parameters:
    IN -> p_grp_src_entry - fib entry; 
  Returns:
    HSL_TRUE - node is empty 
    HSL_FALSE - otherwise
*/

static HSL_BOOL
_hsl_mc_fib_check_if_node_is_empty(struct hsl_mc_fib_entry *p_grp_src_entry)
{
  HSL_FN_ENTER();
  if(!p_grp_src_entry)  
    HSL_FN_EXIT(HSL_FALSE); 

  /* If last interface was deleted delete G,S node */
  if(! hsl_avl_get_tree_size(HSL_MCAST_FIB_L3_IF_TREE) && 
      ! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY))
  {
    /* L3 tree empty */ 

    /* If vlan tree is empty -> delete the node. */  
    if(!hsl_avl_get_tree_size(HSL_MCAST_FIB_VLAN_TREE))
      HSL_FN_EXIT(HSL_TRUE);  

    /* If L2 tree is not empty, but has only inherited nodes -> delete the node. */   
    if((!p_grp_src_entry->actual_joins_cntr) && 
       (!HSL_PREFIX_IS_ZERO(&HSL_MCAST_FIB_SRC)))
      HSL_FN_EXIT(HSL_TRUE); 
  }
  HSL_FN_EXIT(HSL_FALSE); 
}


#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6

/* Add l3 interface to multicast entry.
   Name: hsl_mc_fib_add_l3_if
   Parameters:
     IN -> p_grp_src_entry - fib entry; 
     IN -> ifindex - Interface index.
     IN -> ttl     - Minimum TTL required in packet in order to be forwarded.
   Returns:
     0 on success
     < 0 on error
*/
static int
_hsl_mc_fib_add_l3_if(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_ifIndex_t ifindex, u_char ttl)
{
  struct hsl_mc_eggress_if_entry egress_if;   /* Lookup egress interface.*/
  struct hsl_avl_node *node;                  /* Avl node with data.     */
  struct hsl_mc_eggress_if_entry *p_egress_if;/* New egress interface.  */
  int ret;                                    /* Operation status flag.  */

  HSL_FN_ENTER();

  /* Check the parameters. */
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
 
  /* Init lookup key. */
  egress_if.ifindex = ifindex;

  /* Search if interface is already part of (G,S) interface tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_L3_IF_TREE, (void *)&egress_if);

  /* Check if entry exists. */ 
  if(node)
  {
    /* Update entry data & quit */
    p_egress_if =  (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (node); 
    p_egress_if->u.l3.ttl = ttl;
    HSL_FN_EXIT(STATUS_OK);
  }

  /* Allocate an entry. */
  p_egress_if = (struct hsl_mc_eggress_if_entry *)
    oss_malloc(sizeof(struct hsl_mc_eggress_if_entry),OSS_MEM_HEAP); 

  if(!p_egress_if)
  {
    HSL_FN_EXIT(HSL_FIB_ERR_NOMEM);
  }
  /* Set the entry data. */
  p_egress_if->ifindex = ifindex;
  p_egress_if->u.l3.ttl = ttl;

  /* Add interface to G,S interface  tree. */ 
  ret = hsl_avl_insert (HSL_MCAST_FIB_L3_IF_TREE,(void *)p_egress_if);
  if(ret < 0)
  {
    oss_free (p_egress_if, OSS_MEM_HEAP);
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }
  HSL_FN_EXIT(ret); 
}

/*  Delete l3 interface from multicast entry.
   Name: hsl_mc_fib_del_l3_if
   Parameters:
     IN -> entry   - fib entry; 
     IN -> ifindex - Interface index.

   Returns:
     0 on success
     < 0 on error
*/

static int
_hsl_mc_fib_del_l3_if(struct hsl_mc_fib_entry *p_grp_src_entry, 
                         hsl_ifIndex_t ifindex)
{
  struct hsl_avl_node *node;                  /* Avl node with data.     */
  struct hsl_mc_eggress_if_entry *p_egress_if;/* New egress interface.  */
  struct hsl_mc_eggress_if_entry egress_if;   /* Lookup egress interface.*/
  int ret;                                    /* Operation status flag.  */

  HSL_FN_ENTER();

  /* Check input params. */
  if(!p_grp_src_entry)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Init lookup key. */
  egress_if.ifindex = ifindex;

  /* Search if interface is already part of (G,S) interface tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_L3_IF_TREE, (void *)&egress_if);

  /* Check if entry exists. */ 
  if(!node)
  {
    /* Nothing to delete.*/
    HSL_FN_EXIT(STATUS_OK);
  }

  /* Get entry data pointer.*/
  p_egress_if =  (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (node); 

  /* Delete  interface from G,S interface tree. */ 
  ret = hsl_avl_delete(HSL_MCAST_FIB_L3_IF_TREE,(void *)p_egress_if);

  /* Free entry data.*/
  oss_free (p_egress_if, OSS_MEM_HEAP);

  if(ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER);

  HSL_FN_EXIT(STATUS_OK);
}

/*  Flush L3 interfaces from G,S entry. 
    Name: hsl_mc_flush_l3_interfaces
    Parameters:
      IN -> p_grp_src_entry - Multicast v4 fib entry.   

    Returns:
      0 on success
      < 0 on error
*/
int 
hsl_mc_flush_l3_interfaces(struct hsl_mc_fib_entry *p_grp_src_entry)
{
  struct hsl_avl_node *node;                        /* G,* AVL node.            */
  struct hsl_mc_eggress_if_entry *p_if_entry;       /* Egress interface ponter. */
  int ret = 0;                                      /* Operation status.        */                    

  HSL_FN_ENTER();

  if(!p_grp_src_entry)
    HSL_FN_ENTER(HSL_FIB_ERR_INVALID_PARAM);
  
  for (node = hsl_avl_top (HSL_MCAST_FIB_L3_IF_TREE); node; node = hsl_avl_next (node))
  {
    p_if_entry = (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (node);
    if(!p_if_entry)
      continue;

    ret = _hsl_mc_fib_del_l3_if(p_grp_src_entry,p_if_entry->ifindex);
    if (ret < 0)
       hsl_mc_fib_log_msg(HSL_LEVEL_ERROR, p_grp_src_entry,"Failed to delete interface (%d)",p_if_entry->ifindex); 
  }
  HSL_FN_EXIT(ret);  
}

#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
/*  Add/Delete l2 interface to/from multicast entry.
    In case source is not specified, function walks over 
    all the entires sharing the same group id.  
    
    Name: hsl_mc_igs_add_l2_if
    Parameters:
    IN -> node    - G,S tree node to if.
    IN -> p_key_grp_src_entry  - Original G,S interface was added to.    
    IN -> vid     - Vlan id.
    IN -> ifindex - Ifindex to add.

    Returns:
    0 on success
    < 0 on error
*/
static int
_hsl_mc_igs_add_del_if(struct hsl_avl_node *node,struct hsl_mc_fib_entry *p_key_grp_src_entry, 
                      u_int16_t vid, u_int32_t ifindex, u_int16_t flags, HSL_BOOL oper, u_char is_exclude)
{
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S entry. */ 
  int ret;                                 /* Operation status flag.  */

  HSL_FN_ENTER();

  if(!node)
    HSL_FN_EXIT(STATUS_OK);

  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 

  /* 
     RECURSIVE IN SEQUENCE AVL TREE WALK. 
     DO NOT go left beyound your own group.  
     DO NOT go right beong your own group. 
  */ 

  /* Make sure we update entries for the same group. */

  /* Compare group_id. */
  ret = HSL_PREFIX_CMP(&HSL_MCAST_FIB_GROUP,&p_key_grp_src_entry->group);

  if(ret) /* Group doesn't match. */
    HSL_FN_EXIT(STATUS_OK);

  /* Check if left node exist & go to greater node. */
  if((HSL_AVL_NODE_LEFT(node)) && (HSL_PREFIX_IS_ZERO(&p_key_grp_src_entry->src)))
    _hsl_mc_igs_add_del_if( HSL_AVL_NODE_LEFT(node),p_key_grp_src_entry,
                            vid, ifindex, flags, oper, is_exclude);

  /* Add interface to a node. */
  if(HSL_MCAST_FIB_ADD == oper)
     ret = _hsl_mc_fib_add_l2_if(p_grp_src_entry, ifindex, vid, flags, 
                                 is_exclude);
  else if(HSL_MCAST_FIB_DEL == oper)
     ret = _hsl_mc_fib_del_l2_if(p_grp_src_entry, ifindex, vid, flags);

  if (ret < 0)
    hsl_mc_fib_log_msg(HSL_LEVEL_ERROR, p_grp_src_entry,"Failed to add/delete(%d) interface (%d)",oper,ifindex); 

  /* Check if right node exist & go to lower node. */
  if((HSL_AVL_NODE_RIGHT(node)) && (HSL_PREFIX_IS_ZERO(&p_key_grp_src_entry->src)))
    _hsl_mc_igs_add_del_if(HSL_AVL_NODE_RIGHT(node),p_key_grp_src_entry,
                              vid, ifindex, flags, oper, is_exclude);

  /* Mark for hw update. */ 
  p_grp_src_entry->do_hw_update = HSL_TRUE;

  HSL_FN_EXIT(ret);
}

/* Inherit G,* L2 interfaces to G,S entry. 
   Name: hsl_mc_inherit_l2_interfaces 
   Parameters:
     IN -> p_grp_src_entry - Multicast fib entry.   
   Returns:
     0 on success
     < 0 on error
*/
int 
hsl_mc_inherit_l2_interfaces(struct hsl_mc_fib_entry *p_grp_src_entry,
                             HSL_BOOL oper)
{
  struct hsl_avl_node *node;                    /* G,* AVL node.                   */
  struct hsl_avl_tree *p_avl_tree;              /* AVL tree to operate.            */ 
  struct hsl_avl_node *if_node;                 /* G,* AVL layer 2 interface node. */
  struct hsl_avl_node *if_node_next;            /* G,* AVL layer 2 interface node. */
  struct hsl_mc_fib_entry *p_grp_star_entry;    /* G,* node info                   */ 
  struct hsl_mc_fib_entry grp_star_entry;       /* Lookup entry.                   */ 
  struct hsl_mc_vlan_entry *p_vlan_entry;       /* Vlan information for G,* node.  */ 
  struct hsl_mc_eggress_if_entry *p_if_entry;   /* Layer 2 interface info G,* node.*/ 
  int ret = 0;                                  /* Operation status.               */                    

  HSL_FN_ENTER();

  if(!p_grp_src_entry)
    HSL_FN_ENTER(HSL_FIB_ERR_INVALID_PARAM);

  /* Init lookup key. */
  grp_star_entry.group = HSL_MCAST_FIB_GROUP;
  memset (&grp_star_entry.src,0,sizeof(hsl_prefix_t));  

  if(AF_INET == HSL_MCAST_FIB_GROUP_FAMILY)
  { 
     grp_star_entry.src.family = grp_star_entry.group.family = AF_INET;
     p_avl_tree = HSL_MCAST_FIB_V4_GS_TREE;
  }
#ifdef HAVE_IPV6
  else if (AF_INET6 == p_grp_src_entry->group.family)
  {
     grp_star_entry.src.family = grp_star_entry.group.family = AF_INET6;
     p_avl_tree = HSL_MCAST_FIB_V6_GS_TREE;
  }
#endif /* HAVE_IPV6 */
  else
     HSL_FN_EXIT(STATUS_ERROR);

  /* Search avl tree. */ 
  node = hsl_avl_lookup (p_avl_tree, (void *)&grp_star_entry);

  if(!node)
  {
    /* We are done - no heritage */
    HSL_FN_EXIT(STATUS_OK);
  }
  p_grp_star_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  for (node = hsl_avl_top (p_grp_star_entry->l2_vlan_tree); node; node = hsl_avl_next (node))
  {
    /* Get vlan node info. */
    p_vlan_entry = (struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (node);
    if(!p_vlan_entry)
      continue;

    for (if_node = hsl_avl_top (HSL_MCAST_FIB_L2_IF_TREE); if_node; if_node = if_node_next)
    {
      if_node_next = hsl_avl_next (if_node);
      /* Get layer 2 interface info. */
      p_if_entry = (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (if_node);
      if(!p_if_entry)
        continue;

        /* Add/ Delete interface to G,S entry. */
      if (HSL_MCAST_FIB_ADD == oper)
        ret = _hsl_mc_fib_add_l2_if(p_grp_src_entry, p_if_entry->ifindex,
                                    p_vlan_entry->vid, HSL_MCAST_FIB_INHERITED,
                                    HSL_FALSE);
      else
        ret = _hsl_mc_fib_del_l2_if(p_grp_src_entry, p_if_entry->ifindex,
                                    p_vlan_entry->vid, HSL_MCAST_FIB_INHERITED);
    }
  }
  HSL_FN_EXIT(ret);  
}

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_IGMP_SNOOP

/* Add mac address to ipv4 multicast entry.
  Name: hsl_mc_v4_igs_add_entry 
  Parameters:
    IN -> bridge_name - Bridge name.     
    IN -> src   - Mcast Source address.     
    IN -> group - Mcast Group address.
    IN -> vid   - Vlan id.
    IN -> count - Number of ifindexes. 
    IN -> ifindexes - Array of ifindexes.
  Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v4_igs_add_entry(char *bridge_name, hsl_ipv4Address_t src, 
                        hsl_ipv4Address_t group, u_int16_t vid,
                        u_int16_t count, u_int32_t *ifindexes, char is_exclude)
{
  int index;                                     /* Loop generic index.                 */
  int ret = 0;                                   /* Operation status.                   */
#ifdef HAVE_MCAST_IPV4
  struct hsl_if *vlan_ifp;                       /* VLAN Interface                      */
  char ifname [HSL_IFNAM_SIZE + 1];              /* VLAN Interface name                 */
#endif
  struct hsl_avl_node *node;                     /* Avl node with data.                 */
  struct hsl_avl_node *top_node;                 /* The AVL node of root of the G tree  */
  struct hsl_avl_node *tmp_node;                 /* Tmp Avl node .                      */
  char mac_addr[HSL_ETHER_ALEN];               /* Mac address corresponds to a group. */
  struct hsl_mc_fib_entry grp_src_entry;         /* G,S Lookup  entry.                  */
  struct hsl_mc_fib_entry *p_grp_src_entry;      /* G,S new/existing entry.             */
  struct hsl_mc_fib_entry *top_p_grp_src_entry;  /* G,S of root of G tree.              */
  struct hsl_mc_fib_entry *tmp_p_grp_src_entry;  /* G,S tmp entry.                      */

  HSL_FN_ENTER();
 
  /* Parameters check. */
  if(!ifindexes) 
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 

  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(&group, &src, AF_INET, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Lock mc fib database. */
  HSL_MCAST_V4_DB_LOCK;

  /* Search avl tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_V4_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist create a new one, else use node info. */
  if(!node)
  {
    p_grp_src_entry = _hsl_mc_fib_entry_add(&grp_src_entry);
    node = hsl_avl_lookup (HSL_MCAST_FIB_V4_GS_TREE, (void *)&grp_src_entry);
  }
  else 
  {
    p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 
  }

  if (src)
    hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_ADD);

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    hsl_avl_delete (HSL_MCAST_FIB_V4_GS_TREE,(void *)&grp_src_entry);
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }

  top_node = tmp_node = NULL;
  tmp_p_grp_src_entry = top_p_grp_src_entry = NULL;

  for (tmp_node = HSL_AVL_NODE_PARENT (node); tmp_node;
       tmp_node = HSL_AVL_NODE_PARENT (tmp_node))
    {
      tmp_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (tmp_node);

      ret = HSL_PREFIX_CMP (&HSL_MCAST_FIB_GROUP, &tmp_p_grp_src_entry->group);
      if (ret)
        break;

      top_node =  tmp_node;
      top_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (top_node);
    }

  if (CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY))
    UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY);

  SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L2_ENTRY);

  /* Translate group to mac address. */
  hsl_convert_ip_to_hw (&HSL_MCAST_FIB_GROUP4, (u_char*)mac_addr);

  /* Add joined interface to all G,S pairs */ 
  for(index = 0; index < count; index++) 
  {
    if(!ifindexes[index])
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Invalid interface index passed\n");
      continue;  
    }

    ret = _hsl_mc_igs_add_del_if( (!src && top_node) ? top_node: node,
                                  &grp_src_entry, vid, ifindexes[index],
                                  (src)? HSL_MCAST_FIB_ACTUAL:
                                         HSL_MCAST_FIB_INHERITED,
                                  HSL_MCAST_FIB_ADD, is_exclude);
 
    /* Add layer 2 mac address. */
    ret =  hsl_bridge_add_fdb(bridge_name, ifindexes[index], mac_addr, 
                              HSL_ETHER_ALEN, vid, 0 ,HSL_TRUE);
  }

#ifdef HAVE_MCAST_IPV4
  /* If L3 vif exists for VLAN interface and NO L3 entry, mirror traffic to
   * CPU for generating NOCACHE
   */

  snprintf (ifname, HSL_IFNAM_SIZE, "vlan%s.%d", bridge_name, vid);

  vlan_ifp = hsl_ifmgr_lookup_by_name_type (ifname, HSL_IF_TYPE_IP);

  if (! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY) &&
      (vlan_ifp != NULL) && hsl_mc_vif_exists (AF_INET, vlan_ifp))
    p_grp_src_entry->mirror_to_cpu = HSL_TRUE;

  if (vlan_ifp != NULL)
    HSL_IFMGR_IF_REF_DEC (vlan_ifp);

#endif /* HAVE_MCAST_IPV4 */

  /* Exec hw /os call. */
  _hsl_mc_igs_update_hw ( (!src && top_node) ? top_node: node, p_grp_src_entry);

  /* Release G,S tree. */ 
  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(ret); 
}

/*  Delete mac address from ipv4 multicast entry.
   Name: hsl_mc_v4_igs_del_entry 
   Parameters:
     IN -> bridge_name - Bridge name.     
     IN -> src   - Mcast Source address.     
     IN -> group - Mcast Group address.
     IN -> vid   - Vlan id.
     IN -> count - Number of ifindexes. 
     IN -> ifindexes - Array of ifindexes.  
   Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v4_igs_del_entry(char *bridge_name, hsl_ipv4Address_t src, 
                        hsl_ipv4Address_t group, u_int16_t vid,
                        u_int16_t count, u_int32_t *ifindexes,
                        char is_exclude)
{
  int ret;                                       /* Operation status flag.              */ 
  int index;                                     /* Loop generic index.                 */
  struct hsl_avl_node *node;                     /* Avl node with data.          */
  struct hsl_avl_node *top_node;                 /* The AVL node of root of the G tree  */
  struct hsl_avl_node *tmp_node;                 /* Tmp Avl node .                      */
  char mac_addr[HSL_ETHER_ALEN];               /* Mac address corresponds to a group. */
  struct hsl_mc_fib_entry grp_src_entry;         /* G,S Lookup  entry.           */
  struct hsl_mc_fib_entry *p_grp_src_entry;      /* G,S new/existing entry.      */
  struct hsl_mc_fib_entry *top_p_grp_src_entry;  /* G,S of root of G tree.              */
  struct hsl_mc_fib_entry *tmp_p_grp_src_entry;  /* G,S tmp entry.                      */

  HSL_FN_ENTER();
 
  /* Parameters check. */
  if(!ifindexes)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 

  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(&group, &src, AF_INET, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Lock mc fib database. */
  HSL_MCAST_V4_DB_LOCK;
   
  /* Search avl tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_V4_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist return */
  if(!node)
  {
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }
  /* Get node info. */
  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    hsl_avl_delete (HSL_MCAST_FIB_V4_GS_TREE,(void *)&grp_src_entry);
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }

  top_node = tmp_node = NULL;
  tmp_p_grp_src_entry = top_p_grp_src_entry = NULL;

  for (tmp_node = HSL_AVL_NODE_PARENT (node); tmp_node;
       tmp_node = HSL_AVL_NODE_PARENT (tmp_node))
    {
      tmp_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (tmp_node);

      ret = HSL_PREFIX_CMP (&HSL_MCAST_FIB_GROUP, &tmp_p_grp_src_entry->group);
      if (ret)
        break;

      top_node =  tmp_node;
      top_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (top_node);
    }

  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L2_ENTRY);

  if (src)
    hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_DEL);

  /* Translate group to mac address. */
  hsl_convert_ip_to_hw (&HSL_MCAST_FIB_GROUP4, (u_char*)mac_addr);

  /* Add joined interface to all G,S pairs */ 
  for(index = 0; index < count; index++) 
  {
    if(!ifindexes[index])
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Invalid interface index passed\n");
      continue;  
    }
    /* Delete layer 2 interface from G,S entry. */
    _hsl_mc_igs_add_del_if( (!src && top_node) ? top_node: node,
                            &grp_src_entry, vid, ifindexes[index],
                            (src) ? HSL_MCAST_FIB_ACTUAL:
                            HSL_MCAST_FIB_INHERITED,
                            HSL_MCAST_FIB_DEL, is_exclude);

    /* Delete layer 2 mac address. */
    hsl_bridge_delete_fdb(bridge_name, ifindexes[index], mac_addr, HSL_ETHER_ALEN, vid, 0);
  }

  /* XXX: Nothing to be done to unset mirror to CPU for NOCACHE since 
   * the update of L3 entry (if existing) will take care of setting CPU
   * mirroring appropriately. If L3 entry does not exist, the entry will
   * be removed from hardware anyway.
   */

  /* Exec hw /os call. */
  _hsl_mc_igs_update_hw((!src && top_node) ? top_node: node, &grp_src_entry);

  /* Unlock G,S tree. */
  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(STATUS_OK); 
}

#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP

/* Add mac address to ipv4 multicast entry.
  Name: hsl_mc_v6_mlds_add_entry 
  Parameters:
    IN -> bridge_name - Bridge name.     
    IN -> src   - Mcast Source address.     
    IN -> group - Mcast Group address.
    IN -> vid   - Vlan id.
    IN -> count - Number of ifindexes. 
    IN -> ifindexes - Array of ifindexes.
  Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v6_mlds_add_entry(char *bridge_name, hsl_ipv6Address_t *src, 
                         hsl_ipv6Address_t *group, u_int16_t vid,
                         u_int16_t count, u_int32_t *ifindexes, char is_exclude)
{
  int index;                                     /* Loop generic index.                 */
  int ret = 0;                                   /* Operation status.                   */
#ifdef HAVE_MCAST_IPV6
  struct hsl_if *vlan_ifp;                       /* VLAN Interface                      */
  char ifname [HSL_IFNAM_SIZE + 1];              /* VLAN Interface name                 */
#endif
  struct hsl_avl_node *node;                     /* Avl node with data.                 */
  struct hsl_avl_node *top_node;                 /* The AVL node of root of the G tree  */
  struct hsl_avl_node *tmp_node;                 /* Tmp Avl node .                      */
  struct hsl_mc_fib_entry grp_src_entry;         /* G,S Lookup  entry.                  */
  struct hsl_mc_fib_entry *p_grp_src_entry;      /* G,S new/existing entry.             */
  struct hsl_mc_fib_entry *top_p_grp_src_entry;  /* G,S of root of G tree.              */
  struct hsl_mc_fib_entry *tmp_p_grp_src_entry;  /* G,S tmp entry.                      */

  HSL_FN_ENTER();

  /* Parameters check. */
  if(!ifindexes) 
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 

  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(group, src, AF_INET6, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Lock mc fib database. */
  HSL_MCAST_V6_DB_LOCK;

  /* Search avl tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_V6_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist create a new one, else use node info. */
  if(!node)
  {
    p_grp_src_entry = _hsl_mc_fib_entry_add(&grp_src_entry);
    node = hsl_avl_lookup (HSL_MCAST_FIB_V6_GS_TREE, (void *)&grp_src_entry);
  }
  else 
  {
      p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 
  }

  if((!HSL_PREFIX_IS_ZERO(&HSL_MCAST_FIB_SRC)))
    hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_ADD);

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    hsl_avl_delete (HSL_MCAST_FIB_V6_GS_TREE,(void *)&grp_src_entry);
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }

  top_node = tmp_node = NULL;
  tmp_p_grp_src_entry = top_p_grp_src_entry = NULL;

  for (tmp_node = HSL_AVL_NODE_PARENT (node); tmp_node;
       tmp_node = HSL_AVL_NODE_PARENT (tmp_node))
    {
      tmp_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (tmp_node);

      ret = HSL_PREFIX_CMP (&HSL_MCAST_FIB_GROUP, &tmp_p_grp_src_entry->group);
      if (ret)
        break;

      top_node =  tmp_node;
      top_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (top_node);
    }

  if (CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY))
    UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY);

  SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L2_ENTRY);

  /* Add joined interface to all G,S pairs */ 
  for(index = 0; index < count; index++) 
  {
    if(!ifindexes[index])
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Invalid interface index passed\n");
      continue;  
    }
    ret = _hsl_mc_igs_add_del_if( (! HSL_IPV6_ZERO_ADDR (*src) && top_node) ?
                                                              top_node: node,
                                 &grp_src_entry, vid, ifindexes[index],
                                 !HSL_IPV6_ZERO_ADDR (*src) ? HSL_MCAST_FIB_ACTUAL:
                                 HSL_MCAST_FIB_INHERITED,
                                 HSL_MCAST_FIB_ADD, is_exclude);
  }

#ifdef HAVE_MCAST_IPV6
  /* If L3 vif exists for VLAN interface and NO L3 entry, mirror traffic to
   * CPU for generating NOCACHE
   */

  snprintf (ifname, HSL_IFNAM_SIZE, "vlan%s.%d", bridge_name, vid);

  vlan_ifp = hsl_ifmgr_lookup_by_name_type (ifname, HSL_IF_TYPE_IP);

  if (! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY) &&
      (vlan_ifp != NULL) && hsl_mc_vif_exists (AF_INET6, vlan_ifp))
    p_grp_src_entry->mirror_to_cpu = HSL_TRUE;

  if (vlan_ifp != NULL)
    HSL_IFMGR_IF_REF_DEC (vlan_ifp);

#endif /* HAVE_MCAST_IPV6 */

  /* Exec hw /os call. */
  _hsl_mc_igs_update_hw ( (! HSL_IPV6_ZERO_ADDR (*src) && top_node) ?
                                               top_node: node,
                         p_grp_src_entry);

  /* Release G,S tree. */ 
  HSL_MCAST_V6_DB_UNLOCK;

  HSL_FN_EXIT(ret); 
}

/*  Delete mac address from ipv6 multicast entry.
   Name: hsl_mc_v6_mlds_del_entry 
   Parameters:
     IN -> bridge_name - Bridge name.     
     IN -> src   - Mcast Source address.     
     IN -> group - Mcast Group address.
     IN -> vid   - Vlan id.
     IN -> count - Number of ifindexes. 
     IN -> ifindexes - Array of ifindexes.  
   Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v6_mlds_del_entry(char *bridge_name, hsl_ipv6Address_t *src,
                         hsl_ipv6Address_t *group, u_int16_t vid,
                         u_int16_t count, u_int32_t *ifindexes, char is_exclude)
{
  int ret;                                       /* Operation status flag.              */
  int index;                                     /* Loop generic index.                 */
  struct hsl_avl_node *node;                     /* Avl node with data.                 */
  struct hsl_avl_node *top_node;                 /* The AVL node of root of the G tree  */
  struct hsl_avl_node *tmp_node;                 /* Tmp Avl node .                      */
  struct hsl_mc_fib_entry grp_src_entry;         /* G,S Lookup  entry.                  */
  struct hsl_mc_fib_entry *p_grp_src_entry;      /* G,S new/existing entry.             */
  struct hsl_mc_fib_entry *top_p_grp_src_entry;  /* G,S of root of G tree.              */
  struct hsl_mc_fib_entry *tmp_p_grp_src_entry;  /* G,S tmp entry.                      */

  HSL_FN_ENTER();

  /* Parameters check. */
  if(!ifindexes)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 

  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(group, src, AF_INET6, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Lock mc fib database. */
  HSL_MCAST_V6_DB_LOCK;

  /* Search avl tree. */ 
  node = hsl_avl_lookup (HSL_MCAST_FIB_V6_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist return */
  if(!node)
  {
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }
  /* Get node info. */
  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    hsl_avl_delete (HSL_MCAST_FIB_V6_GS_TREE,(void *)&grp_src_entry);
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }

  top_node = tmp_node = NULL;
  tmp_p_grp_src_entry = top_p_grp_src_entry = NULL;

  for (tmp_node = HSL_AVL_NODE_PARENT (node); tmp_node;
       tmp_node = HSL_AVL_NODE_PARENT (tmp_node))
    {
      tmp_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (tmp_node);

      ret = HSL_PREFIX_CMP (&HSL_MCAST_FIB_GROUP, &tmp_p_grp_src_entry->group);
      if (ret)
        break;

      top_node =  tmp_node;
      top_p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (top_node);
    }

  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L2_ENTRY);

 if((!HSL_PREFIX_IS_ZERO(&HSL_MCAST_FIB_SRC))) 
   hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_DEL);

  /* Add joined interface to all G,S pairs */ 
  for(index = 0; index < count; index++) 
  {
    if(!ifindexes[index])
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Invalid interface index passed\n");
      continue;  
    }
    /* Delete layer 2 interface from G,S entry. */
    _hsl_mc_igs_add_del_if( (! HSL_IPV6_ZERO_ADDR (*src) && top_node) ?
                                                   top_node: node,
                            &grp_src_entry, vid, ifindexes[index],
                            !HSL_IPV6_ZERO_ADDR (*src) ? HSL_MCAST_FIB_ACTUAL:
                            HSL_MCAST_FIB_INHERITED, HSL_MCAST_FIB_DEL, is_exclude);

  }

  /* XXX: Nothing to be done to unset mirror to CPU for NOCACHE since 
   * the update of L3 entry (if existing) will take care of setting CPU
   * mirroring appropriately. If L3 entry does not exist, the entry will
   * be removed from hardware anyway.
   */

  /* Exec hw /os call. */
  _hsl_mc_igs_update_hw( (! HSL_IPV6_ZERO_ADDR (*src) && top_node) ?
                            top_node: node,
                          &grp_src_entry);

  /* Unlock G,S tree. */
  HSL_MCAST_V6_DB_UNLOCK;

  HSL_FN_EXIT(STATUS_OK); 
}

#endif /* HAVE_MLD_SNOOP */

#ifdef  HAVE_MCAST_IPV4
/* Add multicast ipv4 route 
  Name: hsl_mc_v4_route_add 
  Parameters:
    IN -> src        - Mcast Source address.     
    IN -> group      - Mcast Group address.
    IN -> iif_vif    - Incoming interface.
    IN -> num_olist  - Number of ifindexes. 
    IN -> olist_ttls - Array of TTLs.

  Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v4_route_add(hsl_ipv4Address_t src,hsl_ipv4Address_t group,
                    u_int32_t iif_vif,u_int32_t num_olist,u_char *olist_ttls)
{
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.         */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.    */ 
  struct hsl_avl_node *node;         /* Avl node with data.                 */
  int ret;                           /* Operation status.                   */
  int index;                         /* Loop generic index.                 */
  hsl_ifIndex_t ifindex = 0;         /* Interface index.                    */
  HSL_BOOL pim_running;              /* pim running flag.                   */ 
  HSL_BOOL inherit_l2_if = HSL_FALSE;/* Flag to inherit l2 interfaces.      */  
  s_int32_t registered_vif = -1;      /* Registered VIF.                     */   
  u_int8_t  os_only = 0;

  HSL_FN_ENTER();
 
  /* Parameters check. */
  if(!olist_ttls)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 

  /* Check if rpf failure should be trapped to cpu.*/
  pim_running = hsl_mc_is_pim_running(AF_INET);

  if(pim_running)
  {
    /* Get registered VIF. */
    registered_vif = hsl_ipv4_get_registered_vif ();
  
    /* If iif is register vif, just make entry 
     * in the OS stack only,  
     */
    if (iif_vif == registered_vif)
      os_only = 1;
  }

  /* Get incoming interface & iif_vid */ 
  if (! os_only)
    {
      ret = hsl_mc_ipv4_get_vif_ifindex(iif_vif,&ifindex);
      if(ret < 0)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to get interface index for iif (%d)\n", iif_vif);
          HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER);
        }
    }
  
  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(&group, &src, AF_INET, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
  
  /* Lock mc G,S database. */
  HSL_MCAST_V4_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V4_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist create a new one, else use node info. */
  if(!node)
  {
    p_grp_src_entry = _hsl_mc_fib_entry_add(&grp_src_entry);
    inherit_l2_if = HSL_TRUE;
  }
  else 
  {
      p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

      /* Treate entries created by pkt handling as "new" entries */
      if (CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY))
        {
          inherit_l2_if = HSL_TRUE;
          UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY);
        }
      else
        p_grp_src_entry->do_hw_update = HSL_TRUE;
  }

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    /* Unlock G,S tree. */
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }

  /* Check for OS only entry */
  if (os_only)
    SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY);
  else
    UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY);

  SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY);

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  /* Copy (G,*) l2 interfaces to an entry. */
  if((inherit_l2_if) && (src)) 
  {
    hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_ADD);
  }
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

  /* Update entry incoming interface */
  p_grp_src_entry->prev_iif = p_grp_src_entry->iif;
  p_grp_src_entry->iif = ifindex;

  /* Check if mirroring to cpu is required.*/ 
  /* XXX: If L2 entry exists and the mirror to CPU was set, it must be unset.
   * This is already taken care below.
   */
  if((registered_vif >= 0) && (registered_vif < HSL_MAX_VIFS))
    {  
      if(HSL_MC_TTL_VALID(olist_ttls[registered_vif]))
        p_grp_src_entry->mirror_to_cpu = HSL_TRUE;
      else
        p_grp_src_entry->mirror_to_cpu = HSL_FALSE;
    }
  else
    p_grp_src_entry->mirror_to_cpu = HSL_FALSE;

  /* Add ds interfaces to G,S */ 
  for(index = 0; index < num_olist; index++) 
  {
    ret = hsl_mc_ipv4_get_vif_ifindex(index,&ifindex);
    if(ret < 0)
      continue;

    if(HSL_MC_TTL_VALID(olist_ttls[index]))
      ret = _hsl_mc_fib_add_l3_if(p_grp_src_entry, ifindex, olist_ttls[index]);
    else
      ret = _hsl_mc_fib_del_l3_if(p_grp_src_entry, ifindex);

    if(ret < 0)
      hsl_mc_fib_log_msg(HSL_LEVEL_ERROR, p_grp_src_entry,"Failed to add interface (%d)",ifindex); 
  }

  /* DO HW CALL */
  ret = hsl_ipv4_mc_route_add(p_grp_src_entry, iif_vif, num_olist, olist_ttls);

  /* Hw update complete. */
  p_grp_src_entry->do_hw_update = HSL_FALSE; 

  /* Unlock G,S tree. */
  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(ret); 
}

/* Delete multicast ipv4 route 
   Name: hsl_mc_v4_route_del
   Parameters:
     IN -> src        - Mcast Source address.     
     IN -> group      - Mcast Group address.
   Returns:
     0 on success
     < 0 on error
*/
int
hsl_mc_v4_route_del(hsl_ipv4Address_t src, hsl_ipv4Address_t group)
{
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.         */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.    */ 
  struct hsl_avl_node *node;               /* Avl node with data.        */
  int ret;                                 /* Operation status.          */   

  HSL_FN_ENTER();
 
  /* Get (source/group) information. */
  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(&group, &src, AF_INET, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
  
  /* Lock mc fib database. */
  HSL_MCAST_V4_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V4_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist, strange but there is nothing to delete. */
  if(!node)
  {
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  } 
  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  }

  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY);

  /* Do hw/os call. */ 
  ret = hsl_ipv4_mc_route_del(p_grp_src_entry);

  /* Flush l3 interfaces from S,G */ 
  hsl_mc_flush_l3_interfaces(p_grp_src_entry);

  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY);

  p_grp_src_entry->prev_iif = 0;
  p_grp_src_entry->iif = 0;

  /* XXX: If L2 entry exists, set mirror to CPU for NOCACHE */
  if (CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L2_ENTRY))
    {
      p_grp_src_entry->mirror_to_cpu = HSL_TRUE;
    }

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  /* Delete Inherited (G,*) l2 interfaces from the entry entry. */
  if (src) 
   hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_DEL);
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

  /* Make sure node is still needed. */  
  if(_hsl_mc_fib_check_if_node_is_empty(p_grp_src_entry))
    _hsl_mc_fib_entry_del(p_grp_src_entry);
  else /* Re add layer 2 entry. */
    {
       hsl_ipv4_mc_route_add(p_grp_src_entry, 0,0,NULL);

       /* Hw update complete. */
       p_grp_src_entry->do_hw_update = HSL_FALSE; 
    }

      
  /* Unlock G,S tree. */
  HSL_MCAST_V4_DB_UNLOCK;
  HSL_FN_EXIT(ret); 
}

/*  Get multicast ipv4 route usage statistics 
  Name: hsl_mc_v4_sg_stat
  Parameters:
    IN -> src        - Mcast Source address.     
    IN -> group      - Mcast Group address.

  Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v4_sg_stat (hsl_ipv4Address_t src,hsl_ipv4Address_t group, u_int32_t iif_vif,
                   u_int32_t *pktcnt,u_int32_t *bytecnt, u_int32_t *wrong_if)
{
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.        */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.   */ 
  struct hsl_avl_node *node;               /* Avl node with data.       */
  int ret;                                 /* Operation status.         */
  hsl_ifIndex_t ifindex;                   /* Interface index.          */

  HSL_FN_ENTER();
 
  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(&group, &src, AF_INET, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
  
  /* Lock mc fib database. */
  HSL_MCAST_V4_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V4_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist, strange but there is no statistics. */
  if(!node)
  {
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  } 

  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry ||
      CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY)) 
  {
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  }

  /* Verify incoming interface. */
  ret = hsl_mc_ipv4_get_vif_ifindex(iif_vif, &ifindex);
  if(ret < 0)
  {
     HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to get interface index for vif (%d)\n", iif_vif);
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 
  }

  /* If incoming interface doesn't match ignore statistics request. */
  if(ifindex != p_grp_src_entry->iif) 
    HSL_FN_EXIT(STATUS_OK);
     

  /* Do hw call. */ 
  ret = hsl_ipv4_mc_sg_stat (p_grp_src_entry, pktcnt, bytecnt, wrong_if);

  /* Unlock G,S tree. */
  HSL_MCAST_V4_DB_UNLOCK;
  HSL_FN_EXIT(ret); 
}

/* Register incoming L2 interface for SVI iif 
  Name: hsl_mc_v4_fib_register_iif_l2_port
  Parameters:
    IN -> l3_hdr_ptr - IPv4 header pointer.     
    IN -> ifp        - L3 incoming interface.
    IN -> l2_ifp     - L2 incoming port.

  Returns:
    None.
*/
void
hsl_mc_v4_fib_register_iif_l2_port (void *l3_hdr_ptr, struct hsl_if *ifp,
    struct hsl_if *l2_ifp)
{
  struct hsl_ip  *ip_hdr;
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.         */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.    */ 
  struct hsl_avl_node *node;         /* Avl node with data.                 */
  int ret;

  HSL_FN_ENTER();
  
  /* Input check */
  if (l3_hdr_ptr == NULL || ifp == NULL || l2_ifp == NULL)
    HSL_FN_EXIT ();

  /* IP destination check */
  ip_hdr = (struct hsl_ip *) l3_hdr_ptr;
  if(! IPV4_ADDR_MULTICAST(ip_hdr->ip_dst) ||
      (HSL_GET_NTOHL (&ip_hdr->ip_dst) <= INADDR_MAX_LOCAL_GROUP) ||
     ip_hdr->ip_p == HSL_PROTO_IGMP)
    HSL_FN_EXIT ();
 
  /* If IPv4 MC db is not initialized, return. */
  if (! hsl_ipv4_mc_db_initialized)
    HSL_FN_EXIT ();

  /* Only register iif L2 port for SVIs */
  if (memcmp (ifp->name, "vlan", 4)
      || ! hsl_mc_vif_exists (AF_INET, ifp))
    HSL_FN_EXIT ();

  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init (&ip_hdr->ip_dst, &ip_hdr->ip_src, 
      AF_INET, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(ret);
  
  /* Lock mc G,S database. */
  HSL_MCAST_V4_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V4_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist create a new one, else use node info. */
  if(!node)
  {
    p_grp_src_entry = _hsl_mc_fib_entry_add (&grp_src_entry);
    SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY);
  }
  else 
  {
    p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 
    /* XXX: TODO Handle the update case */
  }

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    /* Unlock G,S tree. */
    HSL_MCAST_V4_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  }

  /* Update the L2 iif */
  p_grp_src_entry->l2_iif = l2_ifp->ifindex;

  /* Unlock G,S tree. */
  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(STATUS_OK); 
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef  HAVE_MCAST_IPV6
/* Add multicast ipv6 route 
  Name: hsl_mc_v6_route_add 
  Parameters:
    IN -> src        - Mcast Source address.     
    IN -> group      - Mcast Group address.
    IN -> iif_vif    - Incoming interface.
    IN -> num_olist  - Number of ifindexes. 
    IN -> olist      - Array of added interfaces.

  Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v6_route_add(hsl_ipv6Address_t *src,hsl_ipv6Address_t *group,
                    u_int32_t iif_vif,u_int32_t num_olist,u_int16_t *olist)
{
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.         */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.    */ 
  struct hsl_avl_node *node;         /* Avl node with data.                 */
  int ret;                           /* Operation status.                   */
  int index;                         /* Loop generic index.                 */
  hsl_ifIndex_t ifindex = 0;             /* Interface index.                    */
  HSL_BOOL pim_running;              /* pim running flag.                   */ 
  HSL_BOOL reg_vif_olist = HSL_FALSE; /* reg vif in olsit.                 */ 
  s_int32_t registered_vif = -1;      /* Registered VIF.                     */   
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  HSL_BOOL inherit_l2_if = HSL_FALSE;/* Flag to inherit l2 interfaces.      */  
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
  u_int8_t os_only = 0;

  HSL_FN_ENTER();
 
  /* Parameters check. */
  if(!olist)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 

  /* Check if rpf failure should be trapped to cpu.*/
  pim_running = hsl_mc_is_pim_running(AF_INET6);

  if(pim_running)
  {
    /* Get registered VIF. */
    registered_vif = hsl_ipv6_get_registered_vif ();
  
    /* If iif is register vif, just make entry 
     * in the OS stack only,  
     */
    if (iif_vif == registered_vif)
      os_only = 1;
  }

  /* Get incoming interface & iif_vid */ 
  if (! os_only)
    {
      ret = hsl_mc_ipv6_get_vif_ifindex(iif_vif,&ifindex);
      if(ret < 0)
        {
          HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to get interface index for iif (%d)\n", iif_vif);
          HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER);
        }
    }
  
  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(group, src, AF_INET6, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
  
  /* Lock mc fib database. */
  HSL_MCAST_V6_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V6_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist create a new one, else use node info. */
  if(!node)
  {
    p_grp_src_entry = _hsl_mc_fib_entry_add(&grp_src_entry);
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
    inherit_l2_if = HSL_TRUE;
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
  }
  else 
  {
      p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

      /* Treate entries created by pkt handling as "new" entries */
      if (CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY))
        {
#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
          inherit_l2_if = HSL_TRUE;
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
          UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY);
        }
      else
        p_grp_src_entry->do_hw_update = HSL_TRUE;
  }
  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(HSL_FIB_ERR_DB_OPER); 
  }

  /* Check for OS only entry */
  if (os_only)
    SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY);
  else
    UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY);

  SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY);

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  /* Copy (G,*) l2 interfaces to an entry. */
  if((inherit_l2_if) && (!HSL_PREFIX_IS_ZERO(&HSL_MCAST_FIB_SRC))) 
  { 
    hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_ADD);
  }
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP*/

  /* CHECK MLD SNOOPING */
  /* Update entry incoming interface */
  p_grp_src_entry->prev_iif = p_grp_src_entry->iif;
  p_grp_src_entry->iif = ifindex;

  /* V6 passes only new valid interfaces -> flush old ones. */  
  hsl_mc_flush_l3_interfaces(p_grp_src_entry);
  
  for(index = 0; index < num_olist; index++) 
  {
    if (registered_vif != -1 && olist[index] == registered_vif )
    {
      p_grp_src_entry->mirror_to_cpu = HSL_TRUE;
      reg_vif_olist = HSL_TRUE;
      continue;
    }

    ret = hsl_mc_ipv6_get_vif_ifindex(olist[index],&ifindex);
    if(ret < 0)
    {
      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to get interface index for vif (%d)\n", index);
      continue;
    }

    ret = _hsl_mc_fib_add_l3_if(p_grp_src_entry, ifindex, 1);
    if(ret < 0)
      hsl_mc_fib_log_msg(HSL_LEVEL_ERROR, p_grp_src_entry,"Failed to add interface (%d)",ifindex); 
  }

  /* XXX: If L2 entry exists and the mirror to CPU was set, it must be unset.
   * This is taken care below when L3 entry does not have reg vif in olist.
   */
  if (reg_vif_olist == HSL_FALSE)
    p_grp_src_entry->mirror_to_cpu = HSL_FALSE;

  /* DO HW CALL */
  ret = hsl_ipv6_mc_route_add(p_grp_src_entry, iif_vif, num_olist, olist);

  /* Hw update complete. */
  p_grp_src_entry->do_hw_update = HSL_FALSE; 

  /* Unlock G,S tree. */
  HSL_MCAST_V6_DB_UNLOCK;
  HSL_FN_EXIT(ret); 
}

/* Delete multicast ipv6 route 
   Name: hsl_mc_v6_route_del
   Parameters:
     IN -> src        - Mcast Source address.     
     IN -> group      - Mcast Group address.
   Returns:
     0 on success
     < 0 on error
*/
int
hsl_mc_v6_route_del(hsl_ipv6Address_t *src, hsl_ipv6Address_t *group)
{
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.         */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.    */ 
  struct hsl_avl_node *node;               /* Avl node with data.        */
  int ret;                                 /* Operation status.          */   

  HSL_FN_ENTER();
 
  /* Get (source/group) information. */
  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(group, src, AF_INET6, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
  
  /* Lock mc fib database. */
  HSL_MCAST_V6_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V6_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist, strange but there is nothing to delete. */
  if(!node)
  {
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  } 
  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  }

  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY);

  /* Do hw/os call. */ 
  ret = hsl_ipv6_mc_route_del(p_grp_src_entry);

  /* Flush l3 interfaces from S,G */ 
  hsl_mc_flush_l3_interfaces(p_grp_src_entry);

  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY);

  p_grp_src_entry->prev_iif = 0;
  p_grp_src_entry->iif = 0;

  /* XXX: If L2 entry exists, set mirror to CPU for NOCACHE */
  if (CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L2_ENTRY))
    p_grp_src_entry->mirror_to_cpu = HSL_TRUE;

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  /* Delete Inherited (G,*) l2 interfaces from the entry entry. */
  if((!HSL_PREFIX_IS_ZERO(&HSL_MCAST_FIB_SRC))) 
  {
    hsl_mc_inherit_l2_interfaces(p_grp_src_entry, HSL_MCAST_FIB_DEL);
  }
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP*/

  /* Make sure node is still needed. */  
  if(_hsl_mc_fib_check_if_node_is_empty(p_grp_src_entry))
    _hsl_mc_fib_entry_del(p_grp_src_entry);
#ifdef HAVE_MLD_SNOOP
  else /* Re add layer 2 entry. */
    {
       hsl_ipv6_mc_route_add(p_grp_src_entry, 0,0,NULL);

       /* Hw update complete. */
       p_grp_src_entry->do_hw_update = HSL_FALSE; 
    }
#endif /* HAVE_MLD_SNOOP */      

  /* Release G,S tree. */
  HSL_MCAST_V6_DB_UNLOCK;
  HSL_FN_EXIT(ret); 
}

/*  Get multicast ipv6 route usage statistics 
  Name: hsl_mc_v6_sg_stat
  Parameters:
    IN -> src        - Mcast Source address.     
    IN -> group      - Mcast Group address.

  Returns:
    0 on success
    < 0 on error
*/
int
hsl_mc_v6_sg_stat (hsl_ipv6Address_t *src,hsl_ipv6Address_t *group, u_int32_t iif_vif,
                   u_int32_t *pktcnt,u_int32_t *bytecnt, u_int32_t *wrong_if)
{
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.       */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.  */ 
  struct hsl_avl_node *node;               /* Avl node with data.      */
  int ret;                                 /* Operation status.        */
  hsl_ifIndex_t ifindex;                   /* Interface index.         */

  HSL_FN_ENTER();
 
  /* Init lookup key. */
  ret = hsl_mc_fib_key_init(group, src, AF_INET6, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);
  
  /* Lock mc fib database. */
  HSL_MCAST_V6_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V6_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist, strange but there is no statistics. */
  if(!node)
  {
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  } 

  p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry || 
      CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY)) 
  {
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  }

  /* Verify incoming interface. */
  ret = hsl_mc_ipv6_get_vif_ifindex(iif_vif, &ifindex);
  if(ret < 0)
  {
     HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_ERROR, "Failed to get interface index for vif (%d)\n", iif_vif);
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM); 
  }

  /* If incoming interface doesn't match ignore statistics request. */
  if(ifindex != p_grp_src_entry->iif) 
    HSL_FN_EXIT(STATUS_OK);
     

  /* Do hw call. */ 
  ret = hsl_ipv6_mc_sg_stat (p_grp_src_entry, pktcnt, bytecnt, wrong_if);

  /* Release G,S tree. */
  HSL_MCAST_V6_DB_UNLOCK;
  HSL_FN_EXIT(ret); 
}

/* Register incoming L2 interface for SVI iif 
  Name: hsl_mc_v6_fib_register_iif_l2_port
  Parameters:
    IN -> l3_hdr_ptr - IPv6 header pointer.     
    IN -> ifp        - L3 incoming interface.
    IN -> l2_ifp     - L2 incoming port.

  Returns:
    None.
*/
void
hsl_mc_v6_fib_register_iif_l2_port (void *l3_hdr_ptr, struct hsl_if *ifp,
    struct hsl_if *l2_ifp)
{
  struct hsl_ip6_hdr  *ip6_hdr;
  struct hsl_mc_fib_entry grp_src_entry;   /* G,S Lookup  entry.         */ 
  struct hsl_mc_fib_entry *p_grp_src_entry;/* G,S new/existing entry.    */ 
  struct hsl_avl_node *node;         /* Avl node with data.                 */
  int ret;

  HSL_FN_ENTER();
  
  /* Input check */
  if (l3_hdr_ptr == NULL || ifp == NULL || l2_ifp == NULL)
    HSL_FN_EXIT ();

  /* IP destination check */
  ip6_hdr = (struct hsl_ip6_hdr *) l3_hdr_ptr;
  if(! IPV6_IS_ADDR_MULTICAST (&ip6_hdr->ip_dst) || 
      IPV6_IS_ADDR_MC_LINKLOCAL (&ip6_hdr->ip_dst))
    HSL_FN_EXIT ();
 
  /* If IPv4 MC db is not initialized, return. */
  if (! hsl_ipv6_mc_db_initialized)
    HSL_FN_EXIT ();

  /* Only register iif L2 port for SVIs */
  if (memcmp (ifp->name, "vlan", 4))
    HSL_FN_EXIT ();

  /* Get (source/group) information. */

  /* Init lookup key. */
  ret = hsl_mc_fib_key_init (&ip6_hdr->ip_dst, &ip6_hdr->ip_src, 
      AF_INET6, &grp_src_entry);
  if (ret < 0)
    HSL_FN_EXIT(ret);
  
  /* Lock mc G,S database. */
  HSL_MCAST_V6_DB_LOCK;
   
  /* Search avl tree. */
  node = hsl_avl_lookup (HSL_MCAST_FIB_V6_GS_TREE, (void *)&grp_src_entry);

  /* If node doesn't exist create a new one, else use node info. */
  if(!node)
  {
    p_grp_src_entry = _hsl_mc_fib_entry_add (&grp_src_entry);
    SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY);
  }
  else 
  {
    p_grp_src_entry = (struct hsl_mc_fib_entry *) HSL_AVL_NODE_INFO (node); 
    /* XXX: TODO Handle the update case */
  }

  /* If there is no G,S node at this point bail out. */ 
  if(!p_grp_src_entry) 
  {
    /* Unlock G,S tree. */
    HSL_MCAST_V6_DB_UNLOCK;
    HSL_FN_EXIT(STATUS_OK); 
  }

  /* Update the L2 iif */
  p_grp_src_entry->l2_iif = l2_ifp->ifindex;

  /* Unlock G,S tree. */
  HSL_MCAST_V6_DB_UNLOCK;

  HSL_FN_EXIT(STATUS_OK); 
}
#endif /* HAVE_MCAST_IPV6 */

/* Show multicast entry. 
   Name: 
   Parameters:
     IN -> p_grp_src_entry - G,S tree node info.
   Returns:
     void
*/
static void
_hsl_mc_fib_show (struct hsl_mc_fib_entry *p_grp_src_entry)
{
  struct hsl_avl_node *node;                  /* Vlan tree node.              */
  struct hsl_avl_node *if_node;               /* Layer 2 interface tree node. */
  struct hsl_mc_eggress_if_entry *p_if_entry; /* Layer 2 interface node info. */
  struct hsl_mc_vlan_entry *p_vlan_entry;     /* Vlan node info.              */
  char src[100];                               /* Source address string.       */ 
  char group[100];                             /* Group Address string.        */
  
  HSL_FN_ENTER();

  if(!p_grp_src_entry)
    HSL_FN_EXIT(-1);
  
 
  if(AF_INET == HSL_MCAST_FIB_GROUP_FAMILY)
   {
      hsl_inet_ntop (HSL_MCAST_FIB_GROUP_FAMILY, &HSL_MCAST_FIB_GROUP4, group, sizeof(group));
      hsl_inet_ntop (HSL_MCAST_FIB_SRC_FAMILY, &HSL_MCAST_FIB_SRC4, src, sizeof(src));
   }
#ifdef HAVE_IPV6   
  else if(AF_INET6 == HSL_MCAST_FIB_GROUP_FAMILY) 
   {
      hsl_inet_ntop (HSL_MCAST_FIB_GROUP_FAMILY, &HSL_MCAST_FIB_GROUP6, group, sizeof(group));
      hsl_inet_ntop (HSL_MCAST_FIB_SRC_FAMILY, &HSL_MCAST_FIB_SRC6, src, sizeof(src));
   }
#endif /* HAVE_IPV6 */

  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "Group (%s) Source (%s)\n", group,src);
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "%s \n", p_grp_src_entry->do_hw_update ? "update":"new");
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "%s %s %s Entry\n", 
      CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L2_ENTRY) ? "L2" : "",
      CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_L3_ENTRY) ? "L3" : "",
      CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IIF_L2_ENTRY) ? "Pkt" : "");
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "%s\n", 
      p_grp_src_entry->mirror_to_cpu ? "Mirror To CPU" : ""); 
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "L3 IIf = %d, L2 IIF = %d \n", 
      p_grp_src_entry->iif, p_grp_src_entry->l2_iif); 
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "  L3 outgoing interfaces:\n");
 
  for (node = hsl_avl_top (HSL_MCAST_FIB_L3_IF_TREE); node; node = hsl_avl_next (node))
  {
    p_if_entry = (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (node);
    if(!p_if_entry) 
      continue;

    HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "     Ifindex (%d) ttl (%d) is-exlude (%d)\n",
             p_if_entry->ifindex, p_if_entry->u.l3.ttl, p_if_entry->is_exclude);
  }

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
  HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "  L2 outgoing interfaces:\n");
  for (node = hsl_avl_top (HSL_MCAST_FIB_VLAN_TREE); node; node = hsl_avl_next (node))
  {
    /* Get vlan node info. */
    p_vlan_entry = (struct hsl_mc_vlan_entry *)HSL_AVL_NODE_INFO (node);
    if(!p_vlan_entry)
      continue;

    for (if_node = hsl_avl_top (HSL_MCAST_FIB_L2_IF_TREE); if_node; if_node = hsl_avl_next (if_node))
    {
      /* Get layer 2 interface info. */
      p_if_entry = (struct hsl_mc_eggress_if_entry *)HSL_AVL_NODE_INFO (if_node);
      if(!p_if_entry)
        continue;

      HSL_LOG (HSL_LOG_FIB, HSL_LEVEL_INFO, "  Ifindex (%d) Vid(%d) flags (%x) is-exclude (%d)\n",
               p_if_entry->ifindex, p_vlan_entry->vid, p_if_entry->u.l2.flags, p_if_entry->is_exclude);

    }
  }
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

  HSL_FN_EXIT(STATUS_OK);
}  

/* Dump all v4 multicast tree. 
   Name: _hsl_mc_v4_fib_dump
   Parameters:
      IN -> node    - G,S tree node to if.

   Returns:
      void
*/
static void
_hsl_mc_v4_fib_dump(struct hsl_avl_node *node)
{
  HSL_FN_ENTER();

  if(!node)
    HSL_FN_EXIT(-1);

  /* Check if left node exist & go to greater node. */
  if(HSL_AVL_NODE_LEFT(node))
    _hsl_mc_v4_fib_dump(HSL_AVL_NODE_LEFT(node));

  /* Show node info. */
  _hsl_mc_fib_show((struct hsl_mc_fib_entry *)HSL_AVL_NODE_INFO (node));
  
  /* Check if right node exist & go to lower node. */
  if(HSL_AVL_NODE_RIGHT(node))
    _hsl_mc_v4_fib_dump(HSL_AVL_NODE_RIGHT(node));

  HSL_FN_EXIT(STATUS_OK);
}   

#ifdef HAVE_IPV6
  
/* Dump all v6 multicast tree. 
   Name: _hsl_mc_v6_fib_dump
   Parameters:
      IN -> node    - G,S tree node to if.

   Returns:
      void
*/
static void
_hsl_mc_v6_fib_dump(struct hsl_avl_node *node)
{
  HSL_FN_ENTER();

  if(!node)
    HSL_FN_EXIT(-1);

  /* Check if left node exist & go to greater node. */
  if(HSL_AVL_NODE_LEFT(node))
    _hsl_mc_v6_fib_dump(HSL_AVL_NODE_LEFT(node));

  /* Show interface info. */
  _hsl_mc_fib_show((struct hsl_mc_fib_entry *)HSL_AVL_NODE_INFO (node));
  
  /* Check if right node exist & go to lower node. */
  if(HSL_AVL_NODE_RIGHT(node))
    _hsl_mc_v6_fib_dump(HSL_AVL_NODE_RIGHT(node));

  HSL_FN_EXIT(STATUS_OK);
}   
#endif /* HAVE_IPV6 */

void
hsl_mc_fib_dump()
{
  HSL_FN_ENTER();
  _hsl_mc_v4_fib_dump(hsl_avl_top(HSL_MCAST_FIB_V4_GS_TREE));
#ifdef HAVE_IPV6
  _hsl_mc_v6_fib_dump(hsl_avl_top(HSL_MCAST_FIB_V6_GS_TREE));
#endif /* HAVE_IPV6 */
  HSL_FN_EXIT(STATUS_OK);
}
  
  
/* Log hsl_mcast fib message
  Name: _hsl_mc_fib_log_msg
  Parameters:
    IN ->level - Log level;  
    IN -> p_grp_src_entry - grp source information.
    IN -> message. 
  Returns:
    void
*/
void
hsl_mc_fib_log_msg(u_int16_t level, struct hsl_mc_fib_entry *p_grp_src_entry, const char* format, ...)
{
  char src[50];                             /* Source address string.       */
  char group[50];                           /* Group address string.        */
  char buf[OSS_LOG_BUFFER_SIZE];            /* Buffer to parse format.      */
  int length;                               /* Message length.              */   
  va_list args;                             /* Message args.                */  
  
  HSL_FN_ENTER();

  if(!p_grp_src_entry)
    HSL_FN_ENTER(HSL_FIB_ERR_INVALID_PARAM);

  length = vsprintf(buf, format, args);
  if( length  > OSS_LOG_BUFFER_SIZE )
    {
      /* We better reboot at this point, 
       * because memory overwrite has happened 
       */
    }
  va_end(args);

  if(AF_INET == HSL_MCAST_FIB_GROUP_FAMILY)
   {
      hsl_inet_ntop (HSL_MCAST_FIB_GROUP_FAMILY, &HSL_MCAST_FIB_GROUP4, group, sizeof(group));
      hsl_inet_ntop (HSL_MCAST_FIB_SRC_FAMILY, &HSL_MCAST_FIB_SRC4, src, sizeof(src));
   }
#ifdef HAVE_IPV6   
  else if(AF_INET6 == HSL_MCAST_FIB_GROUP_FAMILY) 
   {
      hsl_inet_ntop (HSL_MCAST_FIB_GROUP_FAMILY, &HSL_MCAST_FIB_GROUP6, group, sizeof(group));
      hsl_inet_ntop (HSL_MCAST_FIB_SRC_FAMILY, &HSL_MCAST_FIB_SRC6, src, sizeof(src));
   }
#endif /* HAVE_IPV6 */
  
  /* Log the message */ 
  HSL_LOG (HSL_LOG_FIB, level, "Group (%s) Source (%s): %s\n", group, src, buf);

}


HSL_BOOL
hsl_mc_is_pim_running(int family)
{
  HSL_FN_ENTER(); 

#ifdef HAVE_MCAST_IPV4
  if(AF_INET == family)
     HSL_FN_EXIT(p_hsl_ipv4_mc_db->pim_running);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  if(AF_INET6 == family)
     HSL_FN_EXIT(p_hsl_ipv6_mc_db->pim_running);
#endif /* HAVE_MCAST_IPV4 */
  return HSL_FALSE;
}

#ifdef HAVE_L2
/*
  Delete multicast l2 route
*/
int
hsl_l2_mc_route_delete(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Input parameters check. */
  if(!p_grp_src_entry) 
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

  /* Delete it from the hw. */
  if(AF_INET == HSL_MCAST_FIB_GROUP_FAMILY)
  {
    if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_l2_mc_route_del)
       ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_l2_mc_route_del) (p_grp_src_entry,vid);
  }
#ifdef HAVE_IPV6 
  else if(AF_INET6 == HSL_MCAST_FIB_GROUP_FAMILY)
  {
    if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_l2_mc_route_del)
       ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_l2_mc_route_del) (p_grp_src_entry,vid);
  }
#endif /* HAVE_IPV6 */
  HSL_FN_EXIT(ret);
}
#endif /* HAVE_L2 */

/*
  Add multicast ipv4 route
*/
int
hsl_ipv4_mc_route_add(struct hsl_mc_fib_entry *p_grp_src_entry,
                      u_int32_t iif_vif,u_int32_t num_olist,u_char *olist_ttls)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Input parameters check. */
  if(!p_grp_src_entry) 
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

#ifdef  HAVE_MCAST_IPV4 
  /* Add it to the os. */
  if (olist_ttls && p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_route_add)
    {
      ret = (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_route_add) (HSL_MCAST_FIB_SRC4,HSL_MCAST_FIB_GROUP4,
                                                              iif_vif, num_olist,olist_ttls);
      if (ret < 0)
        HSL_FN_EXIT(ret);

      SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS);
    }
#endif /* HAVE_MCAST_IPV4  */

  /* Add it to the hw. */
  if (! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY))
    {
      if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_route_add)
        {
          ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_route_add) (p_grp_src_entry);
          if (ret < 0)
            {
#ifdef  HAVE_MCAST_IPV4 
              if (olist_ttls && p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_route_del)        
                {
                  (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_route_del) (HSL_MCAST_FIB_SRC4,HSL_MCAST_FIB_GROUP4);

                  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS);
                }
#endif /* HAVE_MCAST_IPV4  */
              HSL_FN_EXIT(ret);
            }
        }
    }
  HSL_FN_EXIT(0);
}

/*
  Delete multicast ipv4 route
*/
int
hsl_ipv4_mc_route_del(struct hsl_mc_fib_entry *p_grp_src_entry)
{
  int ret = 0;

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!p_grp_src_entry) 
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

#ifdef  HAVE_MCAST_IPV4 
  /* Delete route from the os if l3 interfaces present . */
   if(CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS) && 
     (p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_route_del))
    {
      ret = (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_route_del) (HSL_MCAST_FIB_SRC4,HSL_MCAST_FIB_GROUP4);

      UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS);
    }
#endif /* HAVE_MCAST_IPV4 */

  if (! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY))
    {
      /* Delete route from the hw. */
      if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_route_del)
        {
          ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_route_del) (p_grp_src_entry);
        }
    }
  HSL_FN_EXIT(ret);
}

#ifdef HAVE_MCAST_IPV4
/*
  Init ipv4 multicast. 
*/
int
hsl_ipv4_mc_init(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init hw. */
  if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_init)
    {
      ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_init)();
    }
  HSL_FN_EXIT(ret);
   
}

/*
  Deinit ipv4 multicast. 
*/
int
hsl_ipv4_mc_deinit(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init hw. */
  if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_deinit)
    {
      ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_deinit)();
    }
  /* Flush vif database. */
  hsl_ipv4_flush_vif_db();

  HSL_FN_EXIT(ret);
}

/* 
   Flush vif database.    
*/
int 
hsl_ipv4_flush_vif_db()
{
  int index;
  HSL_FN_ENTER(); 

  HSL_MCAST_V4_DB_LOCK;
  for (index = 0; index < HSL_MAX_VIFS; index ++)
    {
      if(HSL_TRUE == p_hsl_ipv4_mc_db->vif_entry[index].valid)
        {
          hsl_ipv4_mc_vif_del(index);
        }
    }
  HSL_MCAST_V4_DB_UNLOCK;
  HSL_FN_EXIT(0);
}
/*
  Init ipv4 pim multicast. 
*/
int
hsl_ipv4_mc_pim_init(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init os. */
  if (p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_pim_init)
    {
      ret = (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_pim_init)();
    }

  HSL_MCAST_V4_DB_LOCK;

  /* Set pim running to true */ 
  p_hsl_ipv4_mc_db->pim_running = HSL_TRUE;

  HSL_MCAST_V4_DB_UNLOCK;
  
  HSL_FN_EXIT(ret);
   
}

/*
  Deinit ipv4 pim multicast. 
*/
int
hsl_ipv4_mc_pim_deinit(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init hw. */
  if (p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_pim_deinit)
    {
      ret = (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_pim_deinit)();
    }

  HSL_MCAST_V4_DB_LOCK;

  /* Set pim running to false */ 
  p_hsl_ipv4_mc_db->pim_running = HSL_FALSE;

  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(ret);
}


/* 
   Get registered vif.
*/

int hsl_ipv4_get_registered_vif()
{
  HSL_FN_ENTER(); 
  HSL_FN_EXIT(p_hsl_ipv4_mc_db->registered_vif); 
}
/*
  Get multicast ipv4 route usage statistics 
*/
int
hsl_ipv4_mc_sg_stat (struct hsl_mc_fib_entry *p_grp_src_entry,
                     u_int32_t *pktcnt,u_int32_t *bytecnt, u_int32_t *wrong_if)
{
  int ret = 0;
  HSL_FN_ENTER();
  /* Delete route from the hw. */
  if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_sg_stat)
    {
      ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_sg_stat) (p_grp_src_entry,
                                                            pktcnt, bytecnt,wrong_if);
    }
  HSL_FN_EXIT(ret); 
}

int 
_hsl_mc_ipv4_set_vif_ifindex(u_int32_t vif_index,hsl_ifIndex_t ifindex,u_int32_t flags)
{
  HSL_FN_ENTER();
  /* Sanity check */
  if((vif_index > HSL_MAX_VIFS) || (vif_index < 0) || (!p_hsl_ipv4_mc_db)) 
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Illegal parameters vif %d mc_db %p\n",
               vif_index, p_hsl_ipv4_mc_db);
      HSL_FN_EXIT(-1); 
    }

  HSL_MCAST_V4_DB_LOCK;
  p_hsl_ipv4_mc_db->vif_entry[vif_index].ifindex = ifindex;
  p_hsl_ipv4_mc_db->vif_entry[vif_index].flags = flags;
  p_hsl_ipv4_mc_db->vif_entry[vif_index].valid = HSL_TRUE;
  /* Check if registered_vif was added */
  if(flags & HSL_VIFF_REGISTER) 
    p_hsl_ipv4_mc_db->registered_vif = vif_index;

  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(0);
}
int 
_hsl_mc_ipv4_unset_vif_ifindex(u_int32_t vif_index)
{
  HSL_FN_ENTER();
  /* Sanity check */
  if((vif_index > HSL_MAX_VIFS) || (vif_index < 0) || (!p_hsl_ipv4_mc_db)) 
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Illegal parameters\n");
      HSL_FN_EXIT(-1); 
    }

  HSL_MCAST_V4_DB_LOCK;

  /* Set entry invalid */
  p_hsl_ipv4_mc_db->vif_entry[vif_index].valid = HSL_FALSE;

  /* Check if registered_vif was removed*/
  if(vif_index == p_hsl_ipv4_mc_db->registered_vif)
    p_hsl_ipv4_mc_db->registered_vif = -1;

  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(0);
}


int 
hsl_mc_ipv4_get_vif_ifindex(u_int32_t vif_index,hsl_ifIndex_t *ifindex)
{
  int ret = -1;

  HSL_FN_ENTER();
  /* Sanity check */
  if((vif_index > HSL_MAX_VIFS) || (vif_index < 0) || (!ifindex) || (!p_hsl_ipv4_mc_db))
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Illegal parametersi vif %d mc_db %p\n", vif_index, p_hsl_ipv4_mc_db);
      HSL_FN_EXIT(-1); 
    }

  HSL_MCAST_V4_DB_LOCK;

  if((HSL_TRUE == p_hsl_ipv4_mc_db->vif_entry[vif_index].valid) &&
     !(p_hsl_ipv4_mc_db->vif_entry[vif_index].flags & HSL_VIFF_REGISTER))
    { 
      *ifindex = p_hsl_ipv4_mc_db->vif_entry[vif_index].ifindex;  
      ret = 0;
    }
  HSL_MCAST_V4_DB_UNLOCK;

  HSL_FN_EXIT(ret);
}

/*
  Add multicast interface.
*/
int 
hsl_ipv4_mc_vif_add(u_int32_t vif_index,
                    hsl_ifIndex_t ifindex,
                    hsl_ipv4Address_t loc_addr,
                    hsl_ipv4Address_t rmt_addr, 
                    u_int16_t flags)
{
 
  struct hsl_if *ifp;                  /* Interface pointer. */
  int ret = 0;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, 
           "Interface(%d) for vif (%d) \n", 
           ifindex, vif_index);
  
  /* Add mc interface to os */
  if (p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_vif_add)
    {
      ret = (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_vif_add) (vif_index,loc_addr, rmt_addr,flags);
      if(ret < 0)
        HSL_FN_EXIT(ret);
    }

  /* Map vif to real ifindex . */
  ret = _hsl_mc_ipv4_set_vif_ifindex(vif_index,ifindex,flags);
  if(ret < 0)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if(flags & HSL_VIFF_REGISTER) 
    HSL_FN_EXIT (0);
 
  /* Get interface data structure. */ 
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Couldn't get interface(%d) for vif (%d) \n", 
               ifindex, vif_index);
      HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* Create interface in hw */
  if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_vif_add)
    {
      ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_vif_add) (ifp,flags);
      if(ret < 0)
        {
          /* Delete interface from os. */
          if (p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_vif_del)
            (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_vif_del) (vif_index);
        }
    }

  HSL_IFMGR_IF_REF_DEC (ifp); 
  HSL_FN_EXIT (ret);
}

/*
  Delete multicast interface.
*/
int 
hsl_ipv4_mc_vif_del(u_int32_t vif_index)
{
  struct hsl_if *ifp;                  /* Interface pointer. */
  hsl_ifIndex_t ifindex = 0;           /* Interface index.   */
  int registered_vif;
  int ret; 

  HSL_FN_ENTER();

  registered_vif =  hsl_ipv4_get_registered_vif();
  
  /* Get ifindex for vif_index if not Register vif */
  if(vif_index != registered_vif)
    {
      ret = hsl_mc_ipv4_get_vif_ifindex(vif_index,&ifindex);
      if(ret < 0) 
        HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* Unset vif ifndex  */
  _hsl_mc_ipv4_unset_vif_ifindex(vif_index);

  /* Delete interface from os. */ 
  if (p_hsl_ipv4_mc_db->os_cb && p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_vif_del)
    {
      ret = (*p_hsl_ipv4_mc_db->os_cb->os_ipv4_mc_vif_del) (vif_index);
    }

  if(vif_index == registered_vif)
    HSL_FN_EXIT (0);
    
  /* Get interface data structure. */ 
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Couldn't get interface for vif (%d) \n", vif_index);
      HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* Delete interface from hw. */ 
  if (p_hsl_ipv4_mc_db->hw_cb && p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_vif_del)
    { 
      ret = (*p_hsl_ipv4_mc_db->hw_cb->hw_ipv4_mc_vif_del) (ifp);
    }

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (0);
}
/* 
   Register OS(TCP/IP) callbacks for ipv4 mc.
*/
int
hsl_ipv4_mc_os_cb_reg(struct hsl_fib_mc_os_callbacks *cb)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv4_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv4_mc_db->os_cb = cb;
  HSL_FN_EXIT(0);
}

/*
  Unregister OS(TCP/IP) callbacks with FIB manager.
*/
int
hsl_ipv4_mc_os_cb_unreg(void)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv4_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv4_mc_db->os_cb = NULL;
  HSL_FN_EXIT(0);
}
#endif /* HAVE_MCAST_IPV4 */ 

/*
  Register HW callbacks for ipv4 mc.
*/
int
hsl_ipv4_mc_hw_cb_reg(struct hsl_fib_mc_hw_callbacks *cb)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv4_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv4_mc_db->hw_cb = cb;
  HSL_FN_EXIT(0);
}

/*
  Unregister HW callbacks for ipv4 mc.
*/
int
hsl_ipv4_mc_hw_cb_unreg(void)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv4_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv4_mc_db->hw_cb = NULL;
  HSL_FN_EXIT(0);
}

#ifdef HAVE_IPV6
/*
  Add multicast ipv6 route
*/
int
hsl_ipv6_mc_route_add(struct hsl_mc_fib_entry *p_grp_src_entry,
                      u_int32_t iif_vif, u_int32_t num_olist, 
                      u_int16_t *olist)
{
  int ret;

  HSL_FN_ENTER(); 

  /* Input parameters check. */
  if(!p_grp_src_entry) 
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

#ifdef  HAVE_MCAST_IPV6  
  /* Add it to the os. */
  if (olist && p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_route_add)
    {
      ret = (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_route_add) (&HSL_MCAST_FIB_SRC6,&HSL_MCAST_FIB_GROUP6, 
                                                              iif_vif, num_olist, olist);
      if (ret < 0)
        HSL_FN_EXIT(ret);

      SET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS);
    }
#endif /* HAVE_MCAST_IPV6 */ 

  /* Add it to the hw. */
  if (! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY))
    {
      if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_route_add)
        {
          ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_route_add) (p_grp_src_entry);
          if (ret < 0)
            {
#ifdef  HAVE_MCAST_IPV6  
              if (olist && p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_route_del)        
                {
                  (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_route_del) (&HSL_MCAST_FIB_SRC6,&HSL_MCAST_FIB_GROUP6);

                  UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS);
                }
#endif /* HAVE_MCAST_IPV6 */ 
              HSL_FN_EXIT(ret);
            } 
        }
    }
  HSL_FN_EXIT(0);
}

/*
  Delete multicast ipv6 route
*/
int
hsl_ipv6_mc_route_del(struct hsl_mc_fib_entry *p_grp_src_entry)
{
  int ret = 0;

  HSL_FN_ENTER();

  /* Input parameters check. */
  if(!p_grp_src_entry) 
     HSL_FN_EXIT(HSL_FIB_ERR_INVALID_PARAM);

#ifdef  HAVE_MCAST_IPV6  
  /* Delete route from the os. */
   if(CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS) && 
      (p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_route_del))
    { 
      ret = (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_route_del) (&HSL_MCAST_FIB_SRC6,&HSL_MCAST_FIB_GROUP6);

      UNSET_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_IN_OS);
    }
#endif /* HAVE_MCAST_IPV6 */ 

  /* Delete route from the hw. */
  if (! CHECK_FLAG (p_grp_src_entry->flags, HSL_MC_FIB_OS_ONLY_ENTRY))
    {
      if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_route_del)
        {
          ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_route_del) (p_grp_src_entry);
        }
    }
  HSL_FN_EXIT(ret);
}

#ifdef HAVE_MCAST_IPV6
/*
  Init ipv6 multicast. 
*/
int
hsl_ipv6_mc_init(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init hw. */
  if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_init)
    {
      ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_init)();
    }
  HSL_FN_EXIT(ret);
   
}

/*
  Deinit ipv6 multicast. 
*/
int
hsl_ipv6_mc_deinit(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init hw. */
  if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_deinit)
    {
      ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_deinit)();
    }
  /* Flush vif database. */
  hsl_ipv6_flush_vif_db();

  HSL_FN_EXIT(ret);
}

/* 
   Flush vif database.    
*/
int 
hsl_ipv6_flush_vif_db()
{
  int index;
  HSL_FN_ENTER(); 

  HSL_MCAST_V6_DB_LOCK;
  for (index = 0; index < HSL_IPV6_MAX_VIFS; index ++)
    {
      if(HSL_TRUE == p_hsl_ipv6_mc_db->vif_entry[index].valid)
        {
          hsl_ipv6_mc_vif_del(index);
        }
    }
  HSL_MCAST_V6_DB_UNLOCK;
  HSL_FN_EXIT(0);
}
/*
  Init ipv6 pim multicast. 
*/
int
hsl_ipv6_mc_pim_init(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init os. */
  if (p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_pim_init)
    {
      ret = (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_pim_init)();
    }

  HSL_MCAST_V6_DB_LOCK;

  /* Set pim running to true */ 
  p_hsl_ipv6_mc_db->pim_running = HSL_TRUE;

  HSL_MCAST_V6_DB_UNLOCK;
  
  HSL_FN_EXIT(ret);
   
}

/*
  Deinit ipv6 pim multicast. 
*/
int
hsl_ipv6_mc_pim_deinit(void)
{
  int ret = 0;

  HSL_FN_ENTER(); 

  /* Init hw. */
  if (p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_pim_deinit)
    {
      ret = (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_pim_deinit)();
    }

  HSL_MCAST_V6_DB_LOCK;

  /* Set pim running to false */ 
  p_hsl_ipv6_mc_db->pim_running = HSL_FALSE;

  HSL_MCAST_V6_DB_UNLOCK;

  HSL_FN_EXIT(ret);
}


/* 
   Get registered vif.
*/

int hsl_ipv6_get_registered_vif()
{
  HSL_FN_ENTER(); 
  HSL_FN_EXIT(p_hsl_ipv6_mc_db->registered_vif); 
}
/*
  Get multicast ipv6 route usage statistics 
*/
int
hsl_ipv6_mc_sg_stat (struct hsl_mc_fib_entry *p_grp_src_entry,
                      u_int32_t *pktcnt, u_int32_t *bytecnt, u_int32_t *wrong_if)
{
  int ret = 0;
  HSL_FN_ENTER();
  /* Delete route from the hw. */
  if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_sg_stat)
    {
      ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_sg_stat) (p_grp_src_entry, 
                                                            pktcnt, bytecnt, wrong_if);
    }
  HSL_FN_EXIT(ret); 
}

int 
_hsl_mc_ipv6_set_vif_ifindex (u_int32_t vif_index, hsl_ifIndex_t ifindex,
                              u_int32_t flags)
{
  HSL_FN_ENTER();
  /* Sanity check */
  if((vif_index > HSL_IPV6_MAX_VIFS) || (vif_index < 0) || (!p_hsl_ipv6_mc_db)) 
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Illegal parameters ipv6 vif %d mc_db %p\n",
               vif_index, p_hsl_ipv6_mc_db);
      HSL_FN_EXIT(-1); 
    }

  HSL_MCAST_V6_DB_LOCK;
  p_hsl_ipv6_mc_db->vif_entry[vif_index].ifindex = ifindex;
  p_hsl_ipv6_mc_db->vif_entry[vif_index].flags = flags;
  p_hsl_ipv6_mc_db->vif_entry[vif_index].valid = HSL_TRUE;
  /* Check if registered_vif was added */
  if(flags & HSL_IPV6_VIFF_REGISTER) 
    p_hsl_ipv6_mc_db->registered_vif = vif_index;

  HSL_MCAST_V6_DB_UNLOCK;

  HSL_FN_EXIT(0);
}
int 
_hsl_mc_ipv6_unset_vif_ifindex (u_int32_t vif_index)
{
  HSL_FN_ENTER();
  /* Sanity check */
  if((vif_index > HSL_IPV6_MAX_VIFS) || (vif_index < 0) || (!p_hsl_ipv6_mc_db)) 
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Illegal parameters\n");
      HSL_FN_EXIT(-1); 
    }

  HSL_MCAST_V6_DB_LOCK;

  /* Set entry invalid */
  p_hsl_ipv6_mc_db->vif_entry[vif_index].valid = HSL_FALSE;

  /* Check if registered_vif was removed*/
  if(vif_index == p_hsl_ipv6_mc_db->registered_vif)
    p_hsl_ipv6_mc_db->registered_vif = -1;

  HSL_MCAST_V6_DB_UNLOCK;

  HSL_FN_EXIT(0);
}


int 
hsl_mc_ipv6_get_vif_ifindex(u_int32_t vif_index, hsl_ifIndex_t *ifindex)
{
  int ret = -1;

  HSL_FN_ENTER();
  /* Sanity check */
  if((vif_index > HSL_IPV6_MAX_VIFS) || (vif_index < 0) || (!ifindex) || (!p_hsl_ipv6_mc_db))
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Illegal parametersi ipv6 vif %d mc_db %p\n", vif_index, 
               p_hsl_ipv6_mc_db);
      HSL_FN_EXIT(-1); 
    }

  HSL_MCAST_V6_DB_LOCK;

  if((HSL_TRUE == p_hsl_ipv6_mc_db->vif_entry[vif_index].valid) &&
     !(p_hsl_ipv6_mc_db->vif_entry[vif_index].flags & HSL_IPV6_VIFF_REGISTER))
    { 
      *ifindex = p_hsl_ipv6_mc_db->vif_entry[vif_index].ifindex;  
      ret = 0;
    }
  HSL_MCAST_V6_DB_UNLOCK;

  HSL_FN_EXIT(ret);
}

/*
  Add multicast interface.
*/
int 
hsl_ipv6_mc_vif_add (u_int32_t vif_index,
                     hsl_ifIndex_t ifindex,
                     u_int16_t flags)
{
 
  struct hsl_if *ifp;                  /* Interface pointer. */
  int ret = 0;

  HSL_FN_ENTER();

  HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_INFO, 
           "Interface(%d) for IPv6 vif (%d) \n", 
           ifindex, vif_index);
  
  /* Add mc interface to os */
  if (p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_vif_add)
    {
      ret = (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_vif_add) (vif_index, ifindex, flags);
      if(ret < 0)
        HSL_FN_EXIT(ret);
    }

  /* Map vif to real ifindex . */
  ret = _hsl_mc_ipv6_set_vif_ifindex (vif_index, ifindex, flags);
  if(ret < 0)
    HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);

  if(flags & HSL_IPV6_VIFF_REGISTER) 
    HSL_FN_EXIT (0);
 
  /* Get interface data structure. */ 
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, 
               "Couldn't get interface(%d) for IPv6 vif (%d) \n", 
               ifindex, vif_index);
      HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* Create interface in hw */
  if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_vif_add)
    {
      ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_vif_add) (ifp, flags);
      if(ret < 0)
        {
          /* Delete interface from os. */
          if (p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_vif_del)
            (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_vif_del) (vif_index);
        }
    }

  HSL_IFMGR_IF_REF_DEC (ifp); 
  HSL_FN_EXIT (ret);
}

/*
  Delete multicast interface.
*/
int 
hsl_ipv6_mc_vif_del (u_int32_t vif_index)
{
  struct hsl_if *ifp;                  /* Interface pointer. */
  hsl_ifIndex_t ifindex = 0;           /* Interface index.   */
  int registered_vif;
  int ret; 

  HSL_FN_ENTER();

  registered_vif =  hsl_ipv6_get_registered_vif();
  
  /* Get ifindex for vif_index for non-register VIFs */
  if(vif_index != registered_vif)
    {
      ret = hsl_mc_ipv6_get_vif_ifindex (vif_index,&ifindex);
      if(ret < 0) 
        HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* Unset vif ifndex  */
  _hsl_mc_ipv6_unset_vif_ifindex (vif_index);

  /* Delete interface from os. */ 
  if (p_hsl_ipv6_mc_db->os_cb && p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_vif_del)
    {
      ret = (*p_hsl_ipv6_mc_db->os_cb->os_ipv6_mc_vif_del) (vif_index);
    }

  if(vif_index == registered_vif)
    HSL_FN_EXIT (0);
    
  /* Get interface data structure. */ 
  ifp = hsl_ifmgr_lookup_by_index (ifindex);
  if (! ifp)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "Couldn't get interface for IPv6 vif (%d) \n", vif_index);
      HSL_FN_EXIT (HSL_IFMGR_ERR_IF_NOT_FOUND);
    }

  /* Delete interface from hw. */ 
  if (p_hsl_ipv6_mc_db->hw_cb && p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_vif_del)
    { 
      ret = (*p_hsl_ipv6_mc_db->hw_cb->hw_ipv6_mc_vif_del) (ifp);
    }

  HSL_IFMGR_IF_REF_DEC (ifp);
  HSL_FN_EXIT (0);
}

/* 
   Register OS(TCP/IP) callbacks for ipv6 mc.
*/
int
hsl_ipv6_mc_os_cb_reg (struct hsl_fib_ipv6_mc_os_callbacks *cb)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv6_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv6_mc_db->os_cb = cb;
  HSL_FN_EXIT(0);
}

/*
  Unregister OS(TCP/IP) callbacks with FIB manager.
*/
int
hsl_ipv6_mc_os_cb_unreg(void)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv6_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv6_mc_db->os_cb = NULL;
  HSL_FN_EXIT(0);
}
#endif /* HAVE_MCAST_IPV6 */

/*
  Register HW callbacks for ipv6 mc.
*/
int
hsl_ipv6_mc_hw_cb_reg (struct hsl_fib_ipv6_mc_hw_callbacks *cb)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv6_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv6_mc_db->hw_cb = cb;
  HSL_FN_EXIT(0);
}

/*
  Unregister HW callbacks for ipv6 mc.
*/
int
hsl_ipv6_mc_hw_cb_unreg (void)
{
  HSL_FN_ENTER();
  if(!p_hsl_ipv6_mc_db)
    HSL_FN_EXIT(-1);
  p_hsl_ipv6_mc_db->hw_cb = NULL;
  HSL_FN_EXIT(0);
}
#endif /* HAVE_IPV6 */

#if defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6)
static HSL_BOOL
hsl_mc_vif_exists (u_char family, struct hsl_if *ifp)
{
  int index;

  HSL_FN_ENTER();

  if (AF_INET == family)
    {
      HSL_MCAST_V4_DB_LOCK;
      for (index = 0; index < HSL_MAX_VIFS; index ++)
        {
          if (HSL_FALSE == p_hsl_ipv4_mc_db->vif_entry[index].valid)
            continue;

          if (p_hsl_ipv4_mc_db->vif_entry[index].ifindex == ifp->ifindex)
            {
              HSL_MCAST_V4_DB_UNLOCK;
              HSL_FN_EXIT (HSL_TRUE);
            }
        }
      HSL_MCAST_V4_DB_UNLOCK;
    }
#ifdef HAVE_MCAST_IPV6
  else if (AF_INET6 == family)
    {
      HSL_MCAST_V6_DB_LOCK;
      for (index = 0; index < HSL_IPV6_MAX_VIFS; index ++)
        {
          if (HSL_FALSE == p_hsl_ipv6_mc_db->vif_entry[index].valid)
            continue;

          if (p_hsl_ipv6_mc_db->vif_entry[index].ifindex == ifp->ifindex)
            {
              HSL_MCAST_V6_DB_UNLOCK;
              HSL_FN_EXIT (HSL_TRUE);
            }
        }
      HSL_MCAST_V6_DB_UNLOCK;
    }
#endif /* HAVE_MCAST_IPV6 */

  HSL_FN_EXIT (HSL_FALSE);
}
#endif /* defined (HAVE_MCAST_IPV4) || defined (HAVE_MCAST_IPV6) */

#endif /* HAVE_MCAST_IPV4 || HAVE_MCAST_IPV6 || HAVE_IGMP_SNOOP */
