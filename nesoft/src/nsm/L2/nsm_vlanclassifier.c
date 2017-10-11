/* Copyright (C) 2003  All Rights Reserved. 
   
VLAN Classification.

*/

#include "pal.h"
#include "lib.h"

#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "avl_tree.h"

#ifdef HAVE_VLAN_CLASS
#include "hal_incl.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#include "nsm_interface.h"
#include "nsm_vlanclassifier.h"

/* 
   Static functions declaration. 
*/
static int _nsm_vlan_classifier_cmp_int(int first,int second);
struct hal_vlan_classifier_rule *
nsm_get_classifier_rule_by_id(struct nsm_bridge_master *master, 
                              u_int32_t rule_id);
struct hal_vlan_classifier_rule *
nsm_add_new_classifier_rule(struct nsm_bridge_master *master,
                            struct hal_vlan_classifier_rule *rule);
static int
_show_nsm_vlan_classifier_rule(struct cli *cli,struct hal_vlan_classifier_rule *rule_ptr);
static int
_show_nsm_vlan_classifier_group(struct cli *cli,struct nsm_vlan_classifier_group *group_ptr);
int
_nsm_vlan_classifier_walk_group_rules(struct cli *cli,struct nsm_vlan_classifier_group *group_ptr,struct avl_node *top_node);
int
_nsm_vlan_classifier_walk_rules(struct cli *cli,struct avl_node *top_node);



/* Compare two integers. */
static int
_nsm_vlan_classifier_cmp_int(int first,int second)
{
  if(first > second)   
    return 1;

  if (second > first) 
    return -1;

  return 0;
}

/* Free vlan classifier group. */
static void
_nsm_vlan_classifier_free_group(void *group_ptr)
{
  XFREE (MTYPE_NSM_VLAN_CLASSIFIER_GROUP, group_ptr);
}

/* Free vlan classifier rule. */
static void
_nsm_vlan_classifier_free_rule(void *rule_ptr)
{
   XFREE (MTYPE_NSM_VLAN_CLASSIFIER_RULE,rule_ptr);
}

/* Free vlan classifier if group. */
void
nsm_vlan_classifier_free_if_group(void *group_ptr)
{
   XFREE (MTYPE_NSM_VLAN_CLASS_IF_GROUP, group_ptr);
}


/* Finish classifier group tree */
void
nsm_vlan_classifier_del_group_tree(struct avl_tree **pp_group_tree)
{
  avl_tree_free (pp_group_tree,_nsm_vlan_classifier_free_group);
}

/* Finish classifier rules tree */
void
nsm_vlan_classifier_del_rules_tree(struct avl_tree **pp_rules_tree)
{
  avl_tree_free (pp_rules_tree,_nsm_vlan_classifier_free_rule);
}

/* Compare rules based on rule id. */
int
nsm_vlan_classifier_rule_id_cmp(void *data1,void *data2)
{
  struct hal_vlan_classifier_rule *rule1 =(struct hal_vlan_classifier_rule *)data1; 
  struct hal_vlan_classifier_rule *rule2 =(struct hal_vlan_classifier_rule *)data2; 
   
  if((!data1) || (!data2))  
    return -1;

  return _nsm_vlan_classifier_cmp_int(rule1->rule_id,rule2->rule_id);
}

/* Compare groups based on group id. */
int
nsm_vlan_classifier_group_id_cmp(void *data1,void *data2)
{
  struct nsm_vlan_classifier_group *group1 =(struct nsm_vlan_classifier_group *)data1; 
  struct nsm_vlan_classifier_group *group2 =(struct nsm_vlan_classifier_group *)data2; 
   
  if((!data1) || (!data2))  
    return -1;

  return _nsm_vlan_classifier_cmp_int(group1->group_id,group2->group_id);
}

/* Compare interfaces based on ifindex. */
int
nsm_vlan_classifier_ifp_cmp(void *data1,void *data2)
{
  struct interface *ifp1=(struct interface *)data1; 
  struct interface *ifp2=(struct interface *)data2; 
   
  if((!data1) || (!data2))  
    return -1;

  return _nsm_vlan_classifier_cmp_int(ifp1->ifindex,ifp2->ifindex);
}

/* Compare groups based on group id. */
int
nsm_vlan_class_if_group_id_cmp(void *data1, void *data2)
{
  struct nsm_vlan_class_if_group *group1 =
                                        (struct nsm_vlan_class_if_group *)data1; 
  struct nsm_vlan_class_if_group *group2 =
                                        (struct nsm_vlan_class_if_group *)data2; 
   
  if ((!data1) || (!data2))  
    return RESULT_ERROR;

  return _nsm_vlan_classifier_cmp_int(group1->group_id, group2->group_id);
}
/* 
   Function to compare vlan classification rules.  
   Returns 0 in case rules are identical.  
   Return a value different than 0 in case rules are different.  
*/
int
nsm_vlan_classifier_rule_cmp(struct hal_vlan_classifier_rule *first,struct hal_vlan_classifier_rule *second)
{
  struct prefix subnet1,subnet2;

  if(first->type != second->type) 
    return 1;

  switch(first->type)
  {
  case HAL_VLAN_CLASSIFIER_MAC:
     return pal_mem_cmp (first->u.mac, second->u.mac, 6);

  case HAL_VLAN_CLASSIFIER_IPV4:
     subnet1.family    = AF_INET;
     subnet1.prefixlen = first->u.ipv4.masklen;
     subnet1.u.prefix4.s_addr = first->u.ipv4.addr;

     subnet2.family    = AF_INET;
     subnet2.prefixlen = second->u.ipv4.masklen;
     subnet2.u.prefix4.s_addr = second->u.ipv4.addr;
     return prefix_cmp (&subnet1,&subnet2);

  case HAL_VLAN_CLASSIFIER_PROTOCOL:
    if((first->u.protocol.ether_type == second->u.protocol.ether_type) && 
       (first->u.protocol.encaps     == second->u.protocol.encaps))
      return 0;
  }
  return 1;
}

/* 
   Get rule info by rule Id. 
*/ 
struct hal_vlan_classifier_rule *
nsm_get_classifier_rule_by_id (struct nsm_bridge_master *master,
                               u_int32_t rule_id)
{
  struct avl_node *rule_node = NULL;
  struct hal_vlan_classifier_rule rule;

  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->rule_tree))
    return NULL;

  pal_mem_set(&rule, 0, sizeof(rule));

  /* Find the rule. */
  rule.rule_id = rule_id; 
  rule_node = avl_search (master->rule_tree, &rule);
  if(!rule_node) /* No such rule. */
    return NULL;

  return (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(rule_node);  
}

/* 
   Get group info by group Id. 
*/ 

struct nsm_vlan_classifier_group *
nsm_get_classifier_group_by_id(struct nsm_bridge_master *master,
                               u_int32_t group_id)
{
  struct avl_node *group_node = NULL;
  struct nsm_vlan_classifier_group group;

  pal_mem_set(&group, 0, sizeof(struct nsm_vlan_classifier_group));

  /* Make sure bridge is initialized. */ 
  if ((!master) || (!master->group_tree))
    return NULL;

  /* Find the group. */
  group.group_id = group_id; 
  group_node = avl_search (master->group_tree, &group);

  if (!group_node) /* No such group. */
    return NULL;

  return (struct nsm_vlan_classifier_group *)AVL_NODE_INFO(group_node);  
}

struct nsm_vlan_class_if_group *
nsm_get_classifier_group_by_id_from_if (struct interface *ifp,
                                        u_int32_t group_id)
{
  struct avl_node *group_node = NULL;
  struct nsm_vlan_class_if_group group;
  struct nsm_if *zif = NULL;

  if (!ifp)
    return NULL;   

  zif = (struct nsm_if *)ifp->info;

  /* Make sure bridge is initialized. */
  if ((!zif) || (!zif->group_tree))
    return NULL;

  pal_mem_set(&group, 0, sizeof(struct nsm_vlan_class_if_group));

  /* Find the group. */
  group.group_id = group_id;

  group_node = avl_search (zif->group_tree, &group);

  if (!group_node) /* No such group. */
    return NULL;

  return (struct nsm_vlan_class_if_group *)AVL_NODE_INFO(group_node);
}

/* 
   nsm_add_new_classifier_rule  
*/ 

struct hal_vlan_classifier_rule *
nsm_add_new_classifier_rule (struct nsm_bridge_master *master,
                             struct hal_vlan_classifier_rule *rule)
{
   
  struct hal_vlan_classifier_rule *new_rule;
  int ret;

  /* Create new rule. */
  new_rule = XCALLOC (MTYPE_NSM_VLAN_CLASSIFIER_RULE, 
                      sizeof (struct hal_vlan_classifier_rule));
  if (!new_rule)
    return NULL;
     
  /* Preserve rule criteria. */
  *new_rule = *rule;

#ifdef HAVE_SNMP
  /* set the row_status */
  new_rule->row_status = NSM_VLAN_ROW_STATUS_ACTIVE;
#endif /* HAVE_SNMP */      

  /* Add rule to bridge rules tree */
  ret = avl_insert (master->rule_tree, (void *)new_rule);
  if(ret < 0)
  {
    _nsm_vlan_classifier_free_rule(new_rule);
    return NULL;
  }

  /* Create a tree of groups rule might be attached to. */
  ret = avl_create (&new_rule->group_tree, 0, nsm_vlan_classifier_group_id_cmp);
  if(ret < 0)
  { 
    avl_remove (master->rule_tree, new_rule);
    _nsm_vlan_classifier_free_rule(new_rule);
    return NULL;
  }
  return new_rule;
}

/* 
   nsm_del_classifier_rule - function removes single classifier rule 
*/ 

int
nsm_del_classifier_rule(struct nsm_bridge_master *master,
                        struct hal_vlan_classifier_rule *rule_ptr)
{
  struct avl_node *group_node,*group_node_next;
  struct avl_node *if_node;
  struct nsm_vlan_classifier_group *group_ptr;
  struct interface *ifp;
  int ret;


  if((!master) || (!rule_ptr))
    return RESULT_ERROR; 

  /* Go through all groups the rule participates 
     and remove rule from every group. */
  for (group_node = avl_top(rule_ptr->group_tree);group_node;group_node = group_node_next)
  {
    group_node_next = avl_next(group_node);

    group_ptr = (struct nsm_vlan_classifier_group *)AVL_NODE_INFO(group_node);
    if(!group_ptr)
      continue;

    /* Remove group from rule's groups tree. */
    avl_remove (rule_ptr->group_tree, group_ptr);

    /* 
       Remove the rule from all interfaces the group enabled. 
    */
    for (if_node = avl_top(group_ptr->if_tree);if_node;if_node = avl_next(if_node))
    {
      ifp = (struct interface *)AVL_NODE_INFO(if_node);
      if(!ifp)
        continue;
      ret = nsm_del_classifier_rule_from_if(ifp, rule_ptr);
    }

    /* Remove rule from group rules tree. */
    avl_remove (group_ptr->rule_tree, rule_ptr);

    /* If last rule was removed -> delete the group. */
    if(0 >= avl_get_tree_size(group_ptr->rule_tree))
       nsm_del_classifier_group_by_id(master,group_ptr->group_id);
  }

  /* Delete groups tree.  */
  nsm_vlan_classifier_del_group_tree(&rule_ptr->group_tree);

  /* Remove rule from master bridge . */
  avl_remove (master->rule_tree, rule_ptr);

  /* Free rule pointer. */
  _nsm_vlan_classifier_free_rule(rule_ptr);
  return RESULT_OK;
}

/* 
   nsm_del_classifier_rule_by_id  
   Routines deletes classifier rule by rule_id; 
*/ 

int
nsm_del_classifier_rule_by_id(struct nsm_bridge_master *master, 
                              u_int32_t rule_id)
{
  struct hal_vlan_classifier_rule *rule_ptr;
  int ret;
 
  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->rule_tree)) 
    return RESULT_ERROR;
   
  /* Get rule info. */
  rule_ptr = nsm_get_classifier_rule_by_id(master, rule_id);

  if(!rule_ptr)
    return RESULT_ERROR;
  
  ret = nsm_del_classifier_rule(master,rule_ptr);

  return ret;
}


/* 
   Function deletes all classifier rules
*/
int
nsm_del_all_classifier_rules(struct nsm_bridge_master *master)
{
  struct avl_node *node,*node_next;
  struct hal_vlan_classifier_rule *rule_ptr;

  if(!master)
    return RESULT_ERROR;

  /* Go through all the rules and delete them */
  for (node = avl_top(master->rule_tree);node;node = node_next)
  {
    node_next = avl_next(node); 

    rule_ptr = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
    if(!rule_ptr)
      continue;

    nsm_del_classifier_rule(master,rule_ptr);
  }
  return RESULT_OK;
}

/* 
   Function deletes all classifier groups 
*/
int
nsm_del_all_classifier_groups(struct nsm_bridge_master *master)
{
  struct avl_node *node,*node_next;
  struct nsm_vlan_classifier_group *group_ptr;

  if(!master)
    return RESULT_ERROR;

  /* Go through all the groups and delete them */
  for (node = avl_top(master->group_tree);node;node = node_next)
  {
    node_next = avl_next(node);  

    group_ptr = (struct nsm_vlan_classifier_group *)AVL_NODE_INFO(node);
    if(!group_ptr)
      continue;

    nsm_del_classifier_group(master,group_ptr);
  }
  return RESULT_OK;
}

/* 
   nsm_get_rule_reference_count  
   Count number of interfaces rule was installed on. 
*/ 

int
nsm_get_rule_reference_count(struct hal_vlan_classifier_rule *rule)
{
  struct avl_node *node;
  struct nsm_vlan_classifier_group *group_ptr;
  int count = 0;

  /* Go through all groups the rule participates 
     and sum up interfaces. */
  for (node = avl_top(rule->group_tree);node;node = avl_next(node))
  {
    group_ptr = (struct nsm_vlan_classifier_group *)AVL_NODE_INFO(node);
    if(!group_ptr)
      continue;

    /* Add all interfaces count. */
    count += avl_get_tree_size(group_ptr->if_tree);
  }
  return count;
}


/* 
   _nsm_add_new_classifier_group
   Auxiliary routine which allocates a new group and adds it to a proper trees. 
*/ 

struct nsm_vlan_classifier_group *
nsm_add_new_classifier_group(struct nsm_bridge_master *master,
                             u_int32_t group_id)
{
  struct nsm_vlan_classifier_group *new_group;
  int ret;

  /* Create new group. */
  new_group = XCALLOC (MTYPE_NSM_VLAN_CLASSIFIER_GROUP, 
                       sizeof (struct nsm_vlan_classifier_group));
  if (!new_group)
    return NULL;
     
  /* Preserve rule criteria. */
  new_group->group_id = group_id; 
  
  /* Add rule to bridge rules tree */
  ret = avl_insert (master->group_tree, (void *)new_group);
  if(ret < 0)
  {
    _nsm_vlan_classifier_free_group(new_group);
    return NULL;
  }

  /* Create a tree for member rules . */
  ret = avl_create (&new_group->rule_tree, 0,nsm_vlan_classifier_rule_id_cmp);
  if(ret < 0)
  { 
    avl_remove (master->group_tree, new_group);
    _nsm_vlan_classifier_free_group(new_group);
    return NULL;
  }

  /* Create a tree of interfaces where group is installed. */
  ret = avl_create (&new_group->if_tree, 0, nsm_vlan_classifier_ifp_cmp);
  if(ret < 0)
  { 
    nsm_vlan_classifier_del_rules_tree(&new_group->rule_tree);
    avl_remove (master->group_tree, new_group);
    _nsm_vlan_classifier_free_group(new_group);
    return NULL;
  }
  return new_group;
}

/* 
   Function deletes single classifier group
*/
int
nsm_del_classifier_group(struct nsm_bridge_master *master,
                         struct nsm_vlan_classifier_group *group_ptr)
{
  struct avl_node *node,*node_next;
  struct hal_vlan_classifier_rule *rule_ptr;
  struct interface *ifp = NULL;

  /* Go through all the interface & delete group's rules 
     and remove group from every interface. */
  for (node = avl_top(group_ptr->if_tree);node;node = node_next)
  {
    node_next = avl_next(node);  

    ifp = (struct interface *)AVL_NODE_INFO(node);
    if(!ifp)
      continue;

    nsm_del_classifier_group_from_if(ifp, group_ptr->group_id);
  }

  /* Remove group - rule bindings   */
  for (node = avl_top(group_ptr->rule_tree);node;node = node_next)
  {
    node_next = avl_next(node);

    rule_ptr =  (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
    if(!rule_ptr)
      continue;

    avl_remove (rule_ptr->group_tree, group_ptr);
  }

  /* Delete rules tree.  */
  avl_tree_free (&group_ptr->rule_tree, NULL);

  /* Delete interfaces tree. */
  avl_tree_free (&group_ptr->if_tree, NULL);

  /* Remove group from master bridge . */
  avl_remove (master->group_tree, group_ptr);

  /* Free group pointer. */
  _nsm_vlan_classifier_free_group(group_ptr);

  return RESULT_OK;
}

/* 
   nsm_del_classifier_group_by_id  
   Routines deletes classifier group by group_id; 
*/ 

int
nsm_del_classifier_group_by_id(struct nsm_bridge_master *master, 
                               u_int32_t group_id)
{
  struct nsm_vlan_classifier_group *group_ptr = NULL;
  int ret;
 
  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->group_tree)) 
    return RESULT_ERROR;
   
  /* Get group info. */
  group_ptr  = nsm_get_classifier_group_by_id(master, group_id);

  if (!group_ptr)
    return RESULT_ERROR;
  
  ret = nsm_del_classifier_group(master,group_ptr);
  return ret; 
}

/* NSM Server send vlan Classifier message. */
int
nsm_vlan_send_port_class_msg (struct nsm_msg_vlan_port_classifier *msg,
                              int msg_id)
{
  int nbytes;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE(nse->service.bits, NSM_SERVICE_VLAN))
      {
        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

        /* Encode NSM Vlan Classifier message. */
        nbytes = nsm_encode_vlan_port_class_msg (&nse->send.pnt,
                                                 &nse->send.size, msg);

        if (nbytes < 0)
          return nbytes;

        /* Send message. */
        nsm_server_send_message (nse, 0, 0, msg_id, 0, nbytes);
      }

  return 0;
}

int
nsm_add_classifier_rule_to_if(struct interface *ifp, 
                              struct hal_vlan_classifier_rule *rule_ptr)
{
   int ret = 0;
   int refcount = 0;
   struct nsm_msg_vlan_port_classifier msg;

   if((!ifp) || (!rule_ptr))
      return RESULT_ERROR;  

  refcount = nsm_get_rule_reference_count (rule_ptr);

  ret = hal_vlan_classifier_add (rule_ptr, ifp->ifindex, refcount);

  if (ret < 0)
    return ret;

  /* Send Message to Protocols */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port_classifier));

  msg.ifindex = ifp->ifindex;
  msg.vid_info = rule_ptr->vlan_id;
  msg.cnum = 0;
  msg.class_info = NULL;

  ret = nsm_vlan_send_port_class_msg (&msg, NSM_MSG_VLAN_PORT_CLASS_ADD);

  return ret;
}

int 
nsm_del_classifier_rule_from_if(struct interface *ifp, 
                                struct hal_vlan_classifier_rule *rule_ptr)
{
   int ret;
   int refcount;
   struct nsm_msg_vlan_port_classifier msg;

   if((!ifp) || (!rule_ptr))
      return RESULT_ERROR;  

  refcount = nsm_get_rule_reference_count (rule_ptr);

  ret = hal_vlan_classifier_del (rule_ptr, ifp->ifindex, refcount);
  if (ret < 0)
    return ret;

  /* Send Message to Protocols */
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port_classifier));

  msg.ifindex = ifp->ifindex;
  msg.vid_info = rule_ptr->vlan_id;
  msg.cnum = 0;
  msg.class_info = NULL;

  ret = nsm_vlan_send_port_class_msg (&msg, NSM_MSG_VLAN_PORT_CLASS_DEL);

  return ret;
}

int 
nsm_add_classifier_group_to_if(struct interface *ifp, 
                               int group_id,
                               int vlan_id)
{
  struct avl_node *node;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_class_if_group *if_new_group = NULL;
  struct nsm_vlan_classifier_group *group = NULL;
  struct nsm_bridge_master *master = NULL;
  int ret;

  if (!ifp || !ifp->vr || !ifp->vr->proto)
    return RESULT_ERROR;

  /* Get bridge master. */
  master  = nsm_bridge_get_master (ifp->vr->proto);
  if (!master)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    return RESULT_ERROR;

  group = nsm_get_classifier_group_by_id(master, group_id);
  if (!group)
    return APN_ERR_NO_GROUP_EXIST;
  
  if (vlan_id != NSM_VLAN_INVALID_VLAN_ID)
  {
    ret = nsm_vlan_apply_vlan_on_rule(group, vlan_id);
    if (ret < 0)
    {
      return APN_ERR_VLAN_ON_RULES_FAIL;
    }

  }
  else
  {
    node = avl_top(group->rule_tree);
    if (node)
      rule_ptr =  (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
    if (rule_ptr && rule_ptr->vlan_id != NSM_VLAN_INVALID_VLAN_ID)
      vlan_id = rule_ptr->vlan_id;
    else
      return APN_ERR_NO_VLAN_ON_GROUP;
  }

  /* Go through all the rules bound to the group
     and add rule to interface. */
  for (node = avl_top(group->rule_tree); node; node = avl_next(node))
  {
    rule_ptr =  (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
    if (!rule_ptr)
      continue;

    /* Check if a valid vlan is configured to a rule */
    if (rule_ptr->vlan_id == NSM_VLAN_INVALID_VLAN_ID)
      return APN_ERR_NO_VLAN_ON_RULES;
    
    ret = nsm_add_classifier_rule_to_if(ifp, rule_ptr);
  }

  /* Create a new group for interface. */
  if_new_group = XCALLOC (MTYPE_NSM_VLAN_CLASS_IF_GROUP,
                          sizeof (struct nsm_vlan_class_if_group));
  if (!if_new_group)
    return RESULT_ERROR;

  if_new_group->group_id = group_id;
  if_new_group->group_vid = vlan_id;

  /* Add interface to group member list */   
  avl_insert(group->if_tree, ifp);

#ifdef HAVE_SNMP
  if_new_group->row_status = NSM_VLAN_ROW_STATUS_ACTIVE;
#endif /* HAVE_SNMP */

  ret = avl_insert(zif->group_tree, (void *)if_new_group);
  if (ret < 0)
   {
     nsm_vlan_classifier_free_if_group(if_new_group);
     return RESULT_ERROR;
   }

  return RESULT_OK;
}

int 
nsm_del_classifier_group_from_if(struct interface *ifp,  
                                 int group_id)
{
  struct avl_node *node = NULL;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;
  struct nsm_vlan_classifier_group *group_ptr = NULL; 
  struct nsm_bridge_master *master = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_vlan_class_if_group *if_group_ptr = NULL; 
  int ret;

  if (!ifp)
    return RESULT_ERROR;

  if (!ifp->vr)
    return RESULT_ERROR;
 
  if (!ifp->vr->proto)
    return RESULT_ERROR;
  
  zif = (struct nsm_if *)(ifp->info);
  if (!zif)
    return RESULT_ERROR;

  master = nsm_bridge_get_master (ifp->vr->proto);
  if (!master)
    return RESULT_ERROR;

  /* Get group details. */
  group_ptr = nsm_get_classifier_group_by_id(master, group_id);
  if (group_ptr)
  { 
    /* Delete interface from group member list */   
    avl_remove (group_ptr->if_tree, ifp);

    /* Go through all the rules bound to the group
       and remove rule from interface. */
    for (node = avl_top(group_ptr->rule_tree); node; node = avl_next(node))
    {
      rule_ptr =  (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
      if (!rule_ptr)
	continue;

      ret = nsm_del_classifier_rule_from_if(ifp, rule_ptr);
    }
  }

  if_group_ptr = nsm_get_classifier_group_by_id_from_if(ifp, group_id);

  if (!group_ptr)
    return RESULT_ERROR;

  avl_remove (zif->group_tree, (void *)if_group_ptr);
  nsm_vlan_classifier_free_if_group(if_group_ptr);

  return RESULT_OK;
}


/*  Add a classifier to the protocol rules database. */
int
nsm_add_classifier_rule(struct nsm_bridge_master *master,
                        struct hal_vlan_classifier_rule *rule)
{
  struct avl_node *node;
  struct hal_vlan_classifier_rule *search_rule;

  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->rule_tree)) 
    return RESULT_ERROR;

  /* Go through the rules to see if the rule already exists */
  for (node = avl_top(master->rule_tree);node;node = avl_next(node))
  {
    search_rule=  (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
    if(!search_rule)
      continue;

    /* Check if rule id is already used. */
    if (search_rule->rule_id == rule->rule_id)
      return APN_ERR_RULE_ID_EXIST; 
    /* Check if exactly the same rule already exists. */
    if(0 == nsm_vlan_classifier_rule_cmp(search_rule,rule))
      return APN_ERR_SAME_RULE;
  }

  if(NULL == nsm_add_new_classifier_rule(master,rule)) 
    return RESULT_ERROR; 
   
  return  RESULT_OK;
}

/* Add a rule to classifier group database.  */
int
nsm_bind_rule_to_group(struct nsm_bridge_master *master,
                       u_int32_t group_id, u_int32_t rule_id)
{
  struct avl_node *node;
  struct hal_vlan_classifier_rule *rule_ptr;
  struct nsm_vlan_classifier_group *group_ptr;
  struct interface *ifp;
  int ret;

  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->rule_tree) || (!master->group_tree)) 
    return RESULT_ERROR;

  /* Find the rule first. */
  rule_ptr = nsm_get_classifier_rule_by_id(master, rule_id);
  if(!rule_ptr)
    return RESULT_ERROR;

  /* Check if group already exists or is it a new group. */ 
  group_ptr =  nsm_get_classifier_group_by_id(master, group_id);
  if(!group_ptr)
  {
    group_ptr = nsm_add_new_classifier_group(master, group_id);
    if(!group_ptr)  
      return RESULT_ERROR;
  }

  /* Insert group to rule's groups tree. */  
  ret = avl_insert (rule_ptr->group_tree, (void *)group_ptr);
  if(ret < 0)
  {
    if(0 >= avl_get_tree_size(group_ptr->rule_tree))
      nsm_del_classifier_group_by_id(master,group_id);
    return RESULT_ERROR;
  }

  /* Insert rule to group rules tree. */  
  ret = avl_insert (group_ptr->rule_tree, (void *)rule_ptr);
  if(ret < 0)
  {
    avl_remove(rule_ptr->group_tree, (void *)group_ptr);
    if(0 >= avl_get_tree_size(group_ptr->rule_tree))
      nsm_del_classifier_group_by_id(master,group_id);
    return RESULT_ERROR;
  } 
  /* 
     Activate the rule on all interfaces the group enabled. 
  */
  for (node = avl_top(group_ptr->if_tree);node;node = avl_next(node))
  {
    ifp = (struct interface *)AVL_NODE_INFO(node);
    if(!ifp)
      continue;
    ret = nsm_add_classifier_rule_to_if(ifp, rule_ptr);
  }
  return RESULT_OK;
}

/* Del a rule from classifier group database.  */
int
nsm_unbind_rule_from_group(struct nsm_bridge_master *master,
                           u_int32_t group_id, u_int32_t rule_id)
{
  struct avl_node *node;
  struct hal_vlan_classifier_rule *rule_ptr;
  struct nsm_vlan_classifier_group *group_ptr;
  struct interface *ifp;
  int ret;

  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->rule_tree) || (!master->group_tree)) 
    return RESULT_ERROR;

  /* Find the rule first. */
  rule_ptr = nsm_get_classifier_rule_by_id(master, rule_id);
  if(!rule_ptr)
    return RESULT_ERROR;

  /* Check if group exists. */ 
  group_ptr = nsm_get_classifier_group_by_id(master, group_id);
  if(!group_ptr)
    return RESULT_ERROR;

  /* Remove group from rule's groups tree. */  
  avl_remove(rule_ptr->group_tree, (void *)group_ptr);

  /* Remove rule from group's rules tree. */  
  avl_remove(group_ptr->rule_tree, (void *)rule_ptr);


  /* Remove the rule on all interfaces the group enabled.  */
  for (node = avl_top(group_ptr->if_tree);node;node = avl_next(node))
  {
    ifp = (struct interface *)AVL_NODE_INFO(node);
    if(!ifp)
      continue;
    ret = nsm_del_classifier_rule_from_if(ifp, rule_ptr);
  }

  /* If last rule was removed -> delete the group. */
  if(0 >= avl_get_tree_size(group_ptr->rule_tree))
     nsm_del_classifier_group_by_id(master,group_id);

  return RESULT_OK;
}

int
nsm_del_all_classifier_groups_from_if (struct interface *ifp)
{
  struct avl_node *node = NULL;
  struct avl_node *node_next = NULL;
  struct nsm_vlan_class_if_group *group_ptr = NULL;
  struct nsm_if *zif = NULL;

  zif = (struct nsm_if *)(ifp->info);
  if (!zif)
    return RESULT_ERROR;

  /* Go through all the groups and delete them */
  for (node = avl_top(zif->group_tree); node; node = node_next)
    {
      node_next = avl_next(node);

      group_ptr = (struct nsm_vlan_class_if_group *)AVL_NODE_INFO(node);
      if (!group_ptr)
        continue;

      nsm_del_classifier_group_from_if(ifp, group_ptr->group_id);
    }

  return RESULT_OK;

}

/* To set vlan for rules bind to a group */
int
nsm_vlan_apply_vlan_on_rule (struct nsm_vlan_classifier_group *group_ptr,
                             int vlan_id)
{

  struct avl_node *node = NULL;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;

  if (!group_ptr)
    return RESULT_ERROR;

  /* Go through all the rules bound to the group
     and apply vlan */
  for (node = avl_top(group_ptr->rule_tree);node;node = avl_next(node))
    {
      rule_ptr = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(node);
      if(!rule_ptr)
        continue;

      rule_ptr->vlan_id = vlan_id;
    }

  return RESULT_OK;
}


/* Classification protocol string. */
char *
nsm_classifier_proto_str (u_int16_t proto)
{
  switch (proto)
  {
  case APN_CLASS_P_PUP:
    return APN_CLASS_NAME_PUP;
  case APN_CLASS_P_PUPAT:
    return APN_CLASS_NAME_PUPAT;
  case APN_CLASS_P_IP:
    return APN_CLASS_NAME_IP;
  case APN_CLASS_P_X25:
    return APN_CLASS_NAME_X25;
  case APN_CLASS_P_ARP:
    return APN_CLASS_NAME_ARP;
  case APN_CLASS_P_BPQ:
    return APN_CLASS_NAME_BPQ;
  case APN_CLASS_P_IEEEPUP:
    return APN_CLASS_NAME_IEEEPUP;
  case APN_CLASS_P_IEEEPUPAT:
    return APN_CLASS_NAME_IEEEPUPAT;
  case APN_CLASS_P_DEC:
    return APN_CLASS_NAME_DEC;
  case APN_CLASS_P_DNA_DL:
    return APN_CLASS_NAME_DNA_DL;
  case APN_CLASS_P_DNA_RC:
    return APN_CLASS_NAME_DNA_RC;
  case APN_CLASS_P_DNA_RT:
    return APN_CLASS_NAME_DNA_RT;
  case APN_CLASS_P_LAT:
    return APN_CLASS_NAME_LAT;
  case APN_CLASS_P_DIAG:
    return APN_CLASS_NAME_DIAG;
  case APN_CLASS_P_CUST:
    return APN_CLASS_NAME_CUST;
  case APN_CLASS_P_SCA:
    return APN_CLASS_NAME_SCA;
  case APN_CLASS_P_RARP:
    return APN_CLASS_NAME_RARP;
  case APN_CLASS_P_ATALK:
    return APN_CLASS_NAME_ATALK;
  case APN_CLASS_P_AARP:
    return APN_CLASS_NAME_AARP;
  case APN_CLASS_P_IPX: 
    return APN_CLASS_NAME_IPX;   
  case APN_CLASS_P_IPV6:
    return APN_CLASS_NAME_IPV6;
  case APN_CLASS_P_PPP_DISC:
    return APN_CLASS_NAME_PPP_DISC;
  case APN_CLASS_P_PPP_SES:
    return APN_CLASS_NAME_PPP_SES;
  case APN_CLASS_P_ATMMPOA:
    return APN_CLASS_NAME_ATMMPOA;
  case APN_CLASS_P_ATMFATE:
    return APN_CLASS_NAME_ATMFATE;
  default:
    return NULL;
  }
}

/* Classification protocol encapsulation. */
static char *
nsm_classifier_encap_str (u_int32_t encaps)
{
  switch (encaps)
  {
  case HAL_VLAN_CLASSIFIER_ETH:
    return "ethv2";
  case HAL_VLAN_CLASSIFIER_SNAP_LLC:
    return "snapllc";
  case HAL_VLAN_CLASSIFIER_NOSNAP_LLC:
    return "nosnapllc";
  default:
    return NULL;
  }
}
/* 
  Recursive call to show all vlan classifier rules.
  Since number of rules limited to 256 tree rank should not exceed 8.
 */

int
_nsm_vlan_classifier_walk_rules(struct cli *cli,struct avl_node *top_node)
{
  struct hal_vlan_classifier_rule *rule_ptr;

  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int write = 0;

  /* Make sure bridge is initialized. */ 
  if((!top_node) || (!master) || (!master->rule_tree) || (!master->group_tree))
    return 0;

  if(AVL_NODE_LEFT(top_node)) 
    write += _nsm_vlan_classifier_walk_rules(cli,AVL_NODE_LEFT(top_node));

  rule_ptr = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(top_node);
  _show_nsm_vlan_classifier_rule(cli,rule_ptr);
  write++;
 
  if(AVL_NODE_RIGHT(top_node))
    write += _nsm_vlan_classifier_walk_rules(cli,AVL_NODE_RIGHT(top_node));

  return write;
}

int
nsm_vlan_classifier_write (struct cli *cli)
{
  struct avl_node *node; 
  struct nsm_vlan_classifier_group *group_ptr;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int write = 0;

  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->rule_tree) || (!master->group_tree))
    return 0;

  _nsm_vlan_classifier_walk_rules(cli,avl_top(master->rule_tree));

  for (node = avl_top(master->group_tree); node; node = avl_next(node))
  {
    group_ptr = (struct nsm_vlan_classifier_group *)AVL_NODE_INFO(node);
    write += _show_nsm_vlan_classifier_group(cli,group_ptr);
  }
  return write;
}

/* Remove a classifier rule*/
CLI (no_nsm_vlan_classifier_rule,
     no_nsm_vlan_classifier_rule_cmd,
     "no vlan classifier rule <1-256>",
     CLI_NO_STR,
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification rules commands",
     "Vlan classifier rule id")
{ 
  u_int32_t rule_id;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  
  CLI_GET_INTEGER_RANGE ("classifier group", rule_id, argv[0], 
                         NSM_VLAN_RULE_ID_MIN, NSM_VLAN_RULE_ID_MAX);

  /* Delete classifier group. */
  nsm_del_classifier_rule_by_id(master, rule_id);

  return CLI_SUCCESS;
}

/* Remove a classifier group */
CLI (no_nsm_vlan_classifier_group,
     no_nsm_vlan_classifier_group_cmd,
     "no vlan classifier group <1-16>",
     CLI_NO_STR,
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification groups commands",
     "Vlan classifier group id")
{ 
  u_int32_t group_id;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;
  
  CLI_GET_INTEGER_RANGE ("classifier group", group_id, argv[0], 
                         NSM_VLAN_GROUP_ID_MIN, NSM_VLAN_GROUP_ID_MAX);

  /* Delete classifier group. */
  ret = nsm_del_classifier_group_by_id(master, group_id);
  if (ret < 0)
    {
      cli_out (cli, "%% Unable to delete group %d\n", group_id);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* CLI Commands  */
/* Add a classifier to a group */
CLI (nsm_vlan_classifier_mac,
     nsm_vlan_classifier_mac_cmd,
     "vlan classifier rule <1-256> mac WORD vlan "NSM_VLAN_CLI_RNG,
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier rules commands",
     "Vlan classifier rule id",
     "Mac address classification",
     "MAC - mac address in HHHH.HHHH.HHHH format",
     "Vlan",
     "Vlan Identifier")
{ /* 802.1v: Section 8.6 */
  u_int32_t rule_id;
  u_int16_t mac[3];
  u_int16_t vlan_id;
  struct hal_vlan_classifier_rule rule;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;

  pal_mem_set (&rule, 0, sizeof(struct hal_vlan_classifier_rule));

  if (pal_sscanf (argv[1], "%4hx.%4hx.%4hx", &mac[0],  &mac[1],  &mac[2]) != 3)
  {
    cli_out (cli, "%% Unable to parse mac address %s\n", argv[1]);
    return CLI_ERROR;
  }
  
  /* Convert to network order */
  *(unsigned short *)&mac[0] = pal_hton16(*(unsigned short *)&mac[0]);
  *(unsigned short *)&mac[1] = pal_hton16(*(unsigned short *)&mac[1]);
  *(unsigned short *)&mac[2] = pal_hton16(*(unsigned short *)&mac[2]);

  CLI_GET_INTEGER_RANGE ("classifier rule", rule_id, argv[0], 
                         NSM_VLAN_RULE_ID_MIN, NSM_VLAN_RULE_ID_MAX);

  /* Get the vid and check if it is within range (2-4096). */
  CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[2], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  rule.type = HAL_VLAN_CLASSIFIER_MAC;
  pal_mem_cpy (rule.u.mac, mac, 6);
  rule.rule_id = rule_id; 
  rule.vlan_id = vlan_id; 

  ret = nsm_add_classifier_rule(master, &rule);
  if(ret != RESULT_OK)
  {
    switch(ret)
    {
       case APN_ERR_RULE_ID_EXIST:
          cli_out (cli, "%% rule %d is used for another classifier\n", rule_id);
          break;
       case APN_ERR_SAME_RULE: 
          cli_out (cli, "%% identical rule exists\n", argv[1],rule_id);
          break;
       default: 
          cli_out (cli, "%% Unable to add mac (%s) vlan classifier rule %d\n", argv[1],rule_id);
    }
    return CLI_ERROR;
  }
  return CLI_SUCCESS;
}


CLI (nsm_vlan_classifier_ipv4,
     nsm_vlan_classifier_ipv4_cmd,
     "vlan classifier rule <1-256> ipv4 A.B.C.D/M vlan "NSM_VLAN_CLI_RNG,
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification rules commands",
     "Vlan classifier rule id",
     "ipv4 address classification",
     "ipv4 address in A.B.C.D/M format",
     "Vlan",
     "Vlan Identifier")
{ 
  struct prefix_ipv4 addr;
  struct hal_vlan_classifier_rule rule;
  u_int32_t rule_id;
  u_int16_t vlan_id;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  int ret;

  pal_mem_set (&rule, 0, sizeof(struct hal_vlan_classifier_rule));

  CLI_GET_INTEGER_RANGE ("classifier group", rule_id, argv[0], 
                         NSM_VLAN_RULE_ID_MIN, NSM_VLAN_RULE_ID_MAX);

  CLI_GET_IPV4_PREFIX ( "ipv4 address classifier", addr, argv[1]);

  /* Get the vid and check if it is within range (2-4096).  */
  CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[2], NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);

  rule.type                 = HAL_VLAN_CLASSIFIER_IPV4;
  rule.u.ipv4.masklen       = addr.prefixlen;
  rule.u.ipv4.addr          = addr.prefix.s_addr; 
  rule.rule_id              = rule_id; 
  rule.vlan_id              = vlan_id; 

  /* Add classifier. */
  ret = nsm_add_classifier_rule(master, &rule);
  if(ret != RESULT_OK)
  {
    switch(ret)
    {
       case APN_ERR_RULE_ID_EXIST:
          cli_out (cli, "%% rule %d is used for another classifier\n", rule_id);
          break;
       case APN_ERR_SAME_RULE: 
          cli_out (cli, "%% identical rule exists\n", argv[1],rule_id);
          break;
       default: 
          cli_out (cli, "%% Unable to add subnet %s vlan classifier rule %d\n",argv[1], rule_id);
    }
    return CLI_ERROR;
  }
  return CLI_SUCCESS;
}

CLI (nsm_vlan_classifier_proto,
     nsm_vlan_classifier_proto_cmd,
     "vlan classifier rule <1-256> proto (ip|ipv6|ipx|x25|arp|rarp|atalkddp|atalkaarp|atmmulti|atmtransport|pppdiscovery|pppsession|xeroxpup|xeroxaddrtrans|g8bpqx25|ieeepup|ieeeaddrtrans|dec|decdnadumpload|decdnaremoteconsole|decdnarouting|declat|decdiagnostics|deccustom|decsyscomm|<0-65535>) encap (ethv2|snapllc|nosnapllc|) (vlan "NSM_VLAN_CLI_RNG"|)",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classification rules commands",
     "Vlan classifier rule id",
     "proto - specify an ethernet protocol classification",
     "protocol - IP",
     "protocol - IPv6",
     "protocol - IPX",
     "protocol - CCITT X.25",
     "protocol - Address Resolution",
     "protocol - Reverse Address Resolution",
     "protocol - Appletalk DDP",
     "protocol - Appletalk AARP",
     "protocol - MultiProtocol Over ATM",
     "protocol - Frame-based ATM Transport",
     "protocol - PPPoE discovery",
     "protocol - PPPoE session",
     "protocol - Xerox PUP",
     "protocol - Xerox PUP Address Translation",
     "protocol - G8BPQ AX.25",
     "protocol - Xerox IEEE802.3 PUP",
     "protocol - Xerox IEEE802.3 PUP Address Translation",
     "protocol - DEC Assigned",
     "protocol - DEC DNA Dump/Load",
     "protocol - DEC DNA Remote Console",
     "protocol - DEC DNA Routing",
     "protocol - DEC LAT",
     "protocol - DEC Diagnostics",
     "protocol - DEC Customer use",
     "protocol - DEC Systems Comms Arch",
     "ethernet decimal",
     "encap - specifify packet encapsulation",
     "ethv2 - ethernet v2",
     "llc_snap - llc snap encapsulation",
     "llc_nosnap - llc without snap encapsulation",
     "Vlan",
     "Vlan Identifier")
{
  int ret;
  u_int32_t rule_id;
  u_int16_t vlan_id;
  u_int32_t ether_type;
  struct hal_vlan_classifier_rule rule;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  u_int32_t encaps;

  pal_mem_set (&rule, 0, sizeof(struct hal_vlan_classifier_rule));

  CLI_GET_INTEGER_RANGE ("classifier group", rule_id, argv[0], 
                         NSM_VLAN_RULE_ID_MIN, NSM_VLAN_RULE_ID_MAX);

  rule.type = HAL_VLAN_CLASSIFIER_PROTOCOL;
  encaps = HAL_VLAN_CLASSIFIER_ETH;
  ether_type = APN_CLASS_P_IP;
  
  if (pal_char_isdigit (argv[1][0]))
  {
    u_int32_t value;

    CLI_GET_UINT32_RANGE ("Protocol Value", value, argv[1], 0, 65535);
    ether_type = value;
  }
  else
  {
    /* Look thru the predefined strings */
    switch (argv[1][0])
        {
        case 'a':  /* arp | appletalk_ddp | appletalk_aarp | atm_multiprotocol | atm_transport */
        case 'A':
      if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_ARP))
        ether_type = APN_CLASS_P_ARP;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATALK))
        ether_type = APN_CLASS_P_ATALK;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_AARP))
        ether_type = APN_CLASS_P_AARP;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATMMPOA))
        ether_type = APN_CLASS_P_ATMMPOA;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_ATMFATE))
        ether_type = APN_CLASS_P_ATMFATE;

          break;

      /* dec | dec_dna_dump_load | dec_dna_remote_console | dec_dna_routing | dec_lat | dec_diagnostics | dec_custom | dec_sys_comm */
        case 'd':  
        case 'D':
      if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_DEC)) 
        ether_type = APN_CLASS_P_DEC;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_DL)) 
        ether_type = APN_CLASS_P_DNA_DL;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_RC)) 
        ether_type = APN_CLASS_P_DNA_RC;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_DNA_RT)) 
        ether_type = APN_CLASS_P_DNA_RT;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_LAT)) 
        ether_type = APN_CLASS_P_LAT;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_DIAG)) 
        ether_type = APN_CLASS_P_DIAG;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_CUST)) 
        ether_type = APN_CLASS_P_CUST;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_SCA)) 
        ether_type = APN_CLASS_P_SCA;

      break;

        case 'g':  /* g8bpq_x25 */
        case 'G':

      if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_BPQ)) 
        ether_type = APN_CLASS_P_BPQ;

      break;

        case 'i':  /* ip, ipv6, ipx,*/
        case 'I':

      if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_IP)) 
        ether_type = APN_CLASS_P_IP;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_IPV6)) 
        ether_type = APN_CLASS_P_IPV6;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_IPX)) 
        ether_type = APN_CLASS_P_IPX;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_IEEEPUP)) 
        ether_type = APN_CLASS_P_IEEEPUP;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_IEEEPUPAT)) 
        ether_type = APN_CLASS_P_IEEEPUPAT;

          break;

        case 'p':  /* ppp_discovery | ppp_session */
        case 'P':

      if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_PPP_DISC)) 
        ether_type = APN_CLASS_P_PPP_DISC;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_PPP_SES)) 
        ether_type = APN_CLASS_P_PPP_SES;

          break;

        case 'r':  /* rarp */
        case 'R':
      if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_RARP)) 
        ether_type = APN_CLASS_P_RARP;

          break;

        case 'x':  /* x25 | xerox_pup | xerox_pup_addr_trans */
        case 'X':
      if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_X25)) 
        ether_type = APN_CLASS_P_X25;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_PUPAT)) 
        ether_type = APN_CLASS_P_PUPAT;

      else if(0 == pal_strcmp(argv[1],APN_CLASS_NAME_PUP)) 
        ether_type = APN_CLASS_P_PUP;

          break;

        default:
          break;
        }
  }
  rule.u.protocol.ether_type = ether_type;

  /* Packet encapsulation. */
  /* Look thru the predefined strings */
  switch (argv[2][0])
  {
  case 'E':  /* llc */
  case 'e':
    encaps =  HAL_VLAN_CLASSIFIER_ETH;
    break;
  case 'S':  /* snap-llc */
  case 's':
    encaps =  HAL_VLAN_CLASSIFIER_SNAP_LLC;
    break;
  case 'n':  /* nosnap-llc */
  case 'N':
    encaps =  HAL_VLAN_CLASSIFIER_NOSNAP_LLC;
    break;
  default:
    break;
  }

  if (argc > 3)
    {
      /* Get the vid and check if it is within range (2-4096).  */
      CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[4], NSM_VLAN_CLI_MIN, 
                             NSM_VLAN_CLI_MAX);

      rule.vlan_id = vlan_id; 
    } 
  else
     rule.vlan_id = NSM_VLAN_INVALID_VLAN_ID;

  rule.u.protocol.encaps = encaps;
  rule.rule_id           = rule_id; 

  ret = nsm_add_classifier_rule(master, &rule);
  if(ret != RESULT_OK)
  {
    switch(ret)
    {
       case APN_ERR_RULE_ID_EXIST:
          cli_out (cli, "%% rule %d is used for another classifier\n", rule_id);
          break;
       case APN_ERR_SAME_RULE: 
          cli_out (cli, "%% identical rule exists\n", argv[1],rule_id);
          break;
       default: 
          cli_out (cli, "%% Unable to add protocol classifier %s rule %d\n", argv[1], rule_id);
    }
    return CLI_ERROR;
  }

  return CLI_SUCCESS;
}

CLI (nsm_vlan_classifier_group_add_del_rule,
     nsm_vlan_classifier_group_add_del_rule_cmd,
     "vlan classifier group <1-16> (add | delete) rule <1-256>",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier groups commands",
     "Vlan classifier group id",
     "Command add rule to a group",
     "Command delete rule from a group",
     "Vlan classifier rule",
     "Vlan classifier rule id")
{ 
  u_int32_t group_id;
  u_int32_t rule_id;
  s_int8_t ret = -1; 
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  CLI_GET_INTEGER_RANGE ("classifier group", group_id, argv[0], 
                         NSM_VLAN_GROUP_ID_MIN, NSM_VLAN_GROUP_ID_MAX);

  CLI_GET_INTEGER_RANGE ("classifier rule", rule_id, argv[2], 
                         NSM_VLAN_RULE_ID_MIN, NSM_VLAN_RULE_ID_MAX);

  /* Look thru the predefined strings */
  switch (argv[1][0])
  {
  case 'a':  /* Add */
  case 'A':
    ret =  nsm_bind_rule_to_group(master, group_id, rule_id);
    break;
  case 'd':  /* Delete */                  
  case 'D':
    ret = nsm_unbind_rule_from_group(master, group_id, rule_id);
    break;
  default: 
    cli_out (cli, "%% Wrong command\n");
  }

  if(ret < 0)
    return CLI_ERROR;
  
  return CLI_SUCCESS;
}


CLI (nsm_vlan_classifier_activate,
     nsm_vlan_classifier_activate_cmd,
     "vlan classifier activate <1-16> (vlan NSM_VLAN_CLI_RNG |)",
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier activation commands",
     "Vlan classifier group id",
     "Vlan",
     "Vlan Identifier")
{ 
  u_int32_t group_id;
  int ret;
  u_int32_t vlan_id = 0;
  struct nsm_if *zif = NULL;

  struct interface *ifp = cli->index;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    return CLI_ERROR;

  CLI_GET_INTEGER_RANGE ("classifier group", group_id, argv[0], 
                         NSM_VLAN_GROUP_ID_MIN, NSM_VLAN_GROUP_ID_MAX);

  if (argc > 1)
    {
      CLI_GET_INTEGER_RANGE ("VLAN id", vlan_id, argv[2],
          NSM_VLAN_CLI_MIN, NSM_VLAN_CLI_MAX);
    }
  else
    vlan_id = NSM_VLAN_INVALID_VLAN_ID;

  ret  = nsm_add_classifier_group_to_if(ifp, group_id, vlan_id);
  if (ret < 0)
    {
      switch(ret)
        {
        case APN_ERR_VLAN_ON_RULES_FAIL:
          cli_out(cli,"%% Error in vlan configuration on Rules\n");
          return CLI_ERROR;
        case APN_ERR_NO_VLAN_ON_RULES:
          cli_out(cli,"%% No Vlan configured on rules\n");
          return CLI_ERROR;
        case APN_ERR_NO_GROUP_EXIST:
          cli_out(cli,"%% Group %d is not configured\n", group_id);
          return CLI_ERROR;
        default:
          cli_out(cli,"%% Unable to add classifier group %d to interface %s\n",
              group_id, ifp->name);
          return CLI_ERROR;
        }
    }

  return CLI_SUCCESS;
}

CLI (no_nsm_vlan_classifier_activate,
     no_nsm_vlan_classifier_activate_cmd,
     "no vlan classifier activate <1-16>",
     CLI_NO_STR,
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier activation commands",
     "Vlan classification groups commands",
     "Vlan classifier group id")
{ 
  s_int32_t ret; 

  struct interface *ifp = cli->index;
  struct nsm_vlan_class_if_group *if_group_ptr = NULL;
  u_int32_t group_id = 0;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = NULL;

  if (!nm)
    return CLI_ERROR;

  master = nsm_bridge_get_master (nm);
  if (!master)
    return CLI_ERROR;

  NSM_BRIDGE_CHECK_L2_PROPERTY(cli, ifp);
  NSM_BRIDGE_CHECK_BRIDGE_PROPERTY(cli, ifp);

  if (argc > 0)
    CLI_GET_INTEGER_RANGE ("classifier group", group_id, argv[0],
        NSM_VLAN_GROUP_ID_MIN, NSM_VLAN_GROUP_ID_MAX);

  if_group_ptr = nsm_get_classifier_group_by_id_from_if(ifp, group_id);
  if (!if_group_ptr)
    {
      cli_out (cli, "%% Unable to find classifier group %d on interface %s\n",
          group_id, ifp->name);
      return CLI_ERROR;
    }

  ret = nsm_del_classifier_group_from_if(ifp, group_id);
  if (ret < 0)
    {
      cli_out (cli, "%% Unable to deactivate classifier group %d on interface "
          "%s\n", group_id, ifp->name);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}


int
nsm_vlan_class_write(struct cli *cli, struct interface *ifp)
{

  struct nsm_if *zif = NULL;
  struct avl_node *node = NULL;
  struct nsm_vlan_class_if_group *group = NULL;
  int write = 0;

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    return CLI_ERROR;

   for (node = avl_top (zif->group_tree);
        node; node = avl_next (node))
    {
      group = (struct nsm_vlan_class_if_group *)AVL_NODE_INFO(node);
      if (!group)
        continue;

      if (group->group_vid > 0)
        cli_out (cli, " vlan classifier activate %d vlan %d\n",
                 group->group_id, group->group_vid);
      else
        cli_out (cli, " vlan classifier activate %d\n",
                 group->group_id);
      write++;
    }

  return write; 

}

static int
_show_nsm_vlan_classifier_rule(struct cli *cli,struct hal_vlan_classifier_rule *rule_ptr)
{
  char *str, *encap_str;

  if(!rule_ptr)
    return 0;

  switch (rule_ptr->type)
     {
        case HAL_VLAN_CLASSIFIER_MAC:
         cli_out (cli, "vlan classifier rule %d mac %x%x.%x%x.%x%x vlan %d\n",
                     rule_ptr->rule_id, rule_ptr->u.mac[0],
                     rule_ptr->u.mac[1], rule_ptr->u.mac[2],
                     rule_ptr->u.mac[3], rule_ptr->u.mac[4],
                     rule_ptr->u.mac[5], rule_ptr->vlan_id);
            break;
          case HAL_VLAN_CLASSIFIER_IPV4:
            {
              cli_out (cli, "vlan classifier rule %d ipv4 %r/%d vlan %d\n", 
                       rule_ptr->rule_id, &rule_ptr->u.ipv4.addr, 
                       rule_ptr->u.ipv4.masklen, rule_ptr->vlan_id);
            }
            break;
          case HAL_VLAN_CLASSIFIER_PROTOCOL:
            encap_str = nsm_classifier_encap_str(rule_ptr->u.protocol.encaps);
            if ((str = nsm_classifier_proto_str(rule_ptr->u.protocol.ether_type)))
              {
                if (rule_ptr->vlan_id != NSM_VLAN_INVALID_VLAN_ID)
                  {
                    cli_out (cli, "vlan classifier rule %d proto %s encap %s" 
                             " vlan %d\n", rule_ptr->rule_id, str, encap_str, 
                             rule_ptr->vlan_id);
                  }
                else
                  {
                    cli_out (cli, "vlan classifier rule %d proto %s encap %s\n",
                             rule_ptr->rule_id, str, encap_str);
                  }
              }
            else
              {
                if (rule_ptr->vlan_id != NSM_VLAN_INVALID_VLAN_ID)
                  {
                    cli_out (cli, "vlan classifier rule %d proto %d encap %s" 
                             " vlan %d\n", rule_ptr->rule_id, 
                             rule_ptr->u.protocol.ether_type, 
                             encap_str, rule_ptr->vlan_id);
                  }
                else
                  {
                    cli_out (cli, "vlan classifier rule %d proto %d encap %s\n",
                             rule_ptr->rule_id, rule_ptr->u.protocol.ether_type,
                             encap_str);
                  }
              }
      
            break;
    } 
    return 0;
}

/* Show the vlan classifier rules on the screen. Only show
   specified rule if rule argument is non-zero. */
CLI (show_nsm_vlan_classifier_rule,
     show_nsm_vlan_classifier_rule_cmd,
     "show vlan classifier rule(<1-256>|)",
     CLI_SHOW_STR,
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier rule id",
     "Rule Id")
{
  u_int32_t rule_id = 0;
  struct avl_node *rule_node;
  struct hal_vlan_classifier_rule *rule_ptr = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);

  if (argc > 0)
    {
      CLI_GET_INTEGER_RANGE ("classifier rule id", rule_id, argv[0], 
                             NSM_VLAN_RULE_ID_MIN, NSM_VLAN_RULE_ID_MAX);
    }

  /* Make sure bridge is initialized. */ 
  if((!master) || (!master->rule_tree))
    return CLI_ERROR;

  if(rule_id != 0)
  {
    rule_ptr = nsm_get_classifier_rule_by_id(master,rule_id);
    _show_nsm_vlan_classifier_rule(cli,rule_ptr);
  } 
  else
  {
    for (rule_node = avl_top(master->rule_tree); rule_node; rule_node = avl_next(rule_node))
    {
      rule_ptr = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(rule_node);
      _show_nsm_vlan_classifier_rule(cli,rule_ptr);
    }
  }
  return CLI_SUCCESS;
}

/* 
  Recursive call to show all vlan classifier group rules.
  Since number of rules limited to 16 tree rank should not exceed 4.
 */

int
_nsm_vlan_classifier_walk_group_rules(struct cli *cli,struct nsm_vlan_classifier_group *group_ptr,struct avl_node *top_node)
{
  struct hal_vlan_classifier_rule *rule_ptr;
  int write = 0;

  /* Make sure bridge is initialized. */ 
  if(!top_node)
    return 0;

  if(AVL_NODE_LEFT(top_node)) 
    write += _nsm_vlan_classifier_walk_group_rules(cli,group_ptr,AVL_NODE_LEFT(top_node));

  rule_ptr = (struct hal_vlan_classifier_rule *)AVL_NODE_INFO(top_node);
  cli_out (cli, "vlan classifier group %d add rule %d\n", group_ptr->group_id,rule_ptr->rule_id);
  write++;
 
  if(AVL_NODE_RIGHT(top_node))
    write += _nsm_vlan_classifier_walk_group_rules(cli,group_ptr,AVL_NODE_RIGHT(top_node));

  return write;
}


static int
_show_nsm_vlan_classifier_group(struct cli *cli,struct nsm_vlan_classifier_group *group_ptr)
{

  int write = 0;

  if(!group_ptr)
    return 0;

  write = _nsm_vlan_classifier_walk_group_rules(cli,group_ptr,avl_top(group_ptr->rule_tree));
  return write;
}

static int
_show_nsm_vlan_classifier_group_interface
                          (struct cli *cli,
                           struct interface *ifp)
{
  struct avl_node *if_group_node = NULL;
  struct nsm_vlan_class_if_group *if_group_ptr = NULL;
  struct nsm_if *zif = NULL;
  int write = 0;

  if (!ifp)
    return 0;

  zif = (struct nsm_if *)ifp->info;
  if (!zif)
    return 0;

  for (if_group_node = avl_top(zif->group_tree);
      if_group_node; if_group_node = avl_next (if_group_node))
  {
    if_group_ptr = (struct nsm_vlan_class_if_group *)
                   (AVL_NODE_INFO(if_group_node));
    if (!if_group_ptr)
      continue;

    if (if_group_ptr->group_vid != NSM_VLAN_INVALID_VLAN_ID)
      cli_out (cli, "vlan classifier activate %d vlan %d\n",
	       if_group_ptr->group_id, if_group_ptr->group_vid);
    else
      cli_out (cli, "vlan classifier activate %d\n",
	       if_group_ptr->group_id);
    write++;
  }

  return write;
}

static int
_show_nsm_vlan_classifier_interface_group
                          (struct cli *cli,
                           struct nsm_vlan_classifier_group *group_ptr)
{
  struct interface *ifp;
  struct avl_node *if_node;
  int write = 0;

  if (!group_ptr)
    return 0;

  for (if_node = avl_top (group_ptr->if_tree);
       if_node; if_node = avl_next (if_node))
    {
      ifp = (struct interface *)AVL_NODE_INFO(if_node);
      if (!ifp)
        continue;
      cli_out (cli, "vlan classifier group %d interface %s\n",
                               group_ptr->group_id, ifp->name);
      write++;
    }

  return write;
}


/* Show the vlan classifier groups on the screen. Only show
   specified group if group argument is non-zero. */
CLI (show_nsm_vlan_classifier_group,
     show_nsm_vlan_classifier_group_cmd,
     "show vlan classifier group (<1-16>|)",
     CLI_SHOW_STR,
     "Vlan commands",
     "Vlan classification commands",
     "Vlan classifier group id",
     "Group Id")
{
  struct avl_node *group_node;
  u_int32_t group_id = NSM_VLAN_INVALID_GROUP_ID;
  struct nsm_vlan_classifier_group *group_ptr;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
  if (argc > 0)
  {
    CLI_GET_INTEGER_RANGE ("classifier group id", group_id, argv[0], 
                           NSM_VLAN_GROUP_ID_MIN, NSM_VLAN_GROUP_ID_MAX);
  }

  if(group_id != NSM_VLAN_INVALID_GROUP_ID)
  {
    group_ptr = nsm_get_classifier_group_by_id(master,group_id);
    _show_nsm_vlan_classifier_group(cli,group_ptr);
  }
  else
  {
    for (group_node = avl_top(master->group_tree); group_node; group_node = avl_next(group_node))
    {
      group_ptr = (struct nsm_vlan_classifier_group *)AVL_NODE_INFO(group_node);
      _show_nsm_vlan_classifier_group(cli,group_ptr);
    }
  }
  return CLI_SUCCESS;
}

 /* Show the vlan classifier interface group on the screen. Only show
    specified group if group argument is non-zero. */
 CLI (show_nsm_vlan_classifier_interface_group,
      show_nsm_vlan_classifier_interface_group_cmd,
      "show vlan classifier interface group (<1-16>|)",
      CLI_SHOW_STR,
      "Vlan commands",
      "Vlan classification commands",
      "Interface group activated on",
      "Vlan classifier group id",
      "Group Id")
 {
   struct avl_node *group_node;
   u_int32_t group_id = NSM_VLAN_INVALID_GROUP_ID;
   struct nsm_vlan_classifier_group *group_ptr = NULL;
   struct nsm_master *nm = cli->vr->proto;
   struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
   if (argc > 0)
     {
       CLI_GET_INTEGER_RANGE ("classifier group id", group_id, argv[0],
                              NSM_VLAN_GROUP_ID_MIN, NSM_VLAN_GROUP_ID_MAX);
     }

   if (group_id != NSM_VLAN_INVALID_GROUP_ID)
     {
       group_ptr = nsm_get_classifier_group_by_id (master,group_id);
       _show_nsm_vlan_classifier_interface_group (cli, group_ptr);
     }
   else
     {
       for (group_node = avl_top (master->group_tree); 
            group_node; group_node = avl_next (group_node))
         {
           group_ptr = (struct nsm_vlan_classifier_group *)AVL_NODE_INFO (group_node);
           _show_nsm_vlan_classifier_interface_group (cli,group_ptr);
         }
     }

   return CLI_SUCCESS;
}

/* Show the vlan classifier interface group on the screen. Only show
    specified group if group argument is non-zero. */
CLI (show_nsm_vlan_classifier_group_interface,
     show_nsm_vlan_classifier_group_interface_cmd,
     "show vlan classifier group interface IFNAME",
     CLI_SHOW_STR,
     "Vlan commands",
     "Vlan classification commands",
     "group activated",
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  struct interface *ifp = NULL;

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if (!ifp || !ifp->info)
  {
    cli_out (cli, "%% Can't find interface %s\n", argv[0]);
    return CLI_ERROR;
  }

  _show_nsm_vlan_classifier_group_interface (cli, ifp);

  return CLI_SUCCESS;
}


void
nsm_vlan_classifier_cli_init (struct cli_tree *ctree)
{
  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_classifier_rule_cmd);
  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_classifier_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_classifier_interface_group_cmd);
  cli_install (ctree, EXEC_MODE, &show_nsm_vlan_classifier_group_interface_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_vlan_classifier_mac_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_vlan_classifier_ipv4_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_vlan_classifier_proto_cmd);
  cli_install (ctree, CONFIG_MODE, &no_nsm_vlan_classifier_group_cmd);
  cli_install (ctree, CONFIG_MODE, &no_nsm_vlan_classifier_rule_cmd);
  cli_install (ctree, CONFIG_MODE, &nsm_vlan_classifier_group_add_del_rule_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_vlan_classifier_activate_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_vlan_classifier_activate_cmd);
}

#endif /* HAVE_VLAN_CLASS */
