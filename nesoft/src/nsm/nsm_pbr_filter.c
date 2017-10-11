/* Copyright (C) 2009-2010  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "cli.h"

#include "snprintf.h"
#include "vty.h"
#include "ls_prefix.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "nsmd.h"
#include "pbr_filter.h"
#include "nsm_pbr_filter.h"
#include "nsm_api.h"

#ifdef HAVE_PBR

/* Initializing the route map interface list */
void
nsm_pbr_route_map_if_init (struct nsm_master *nm)
{
   nm->route_map_if = list_new ();
}

/* Check route map interface list for the given route map name to find out
   whether it is configued on interface or not */
struct route_map_if *
nsm_pbr_route_map_name_lookup (struct nsm_master *nm, char *rmap_name)
{
  struct listnode *rn = NULL;
  struct route_map_if *rmap_if;

  if (!rmap_name)
    return NULL;

  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    {
      if (!pal_strcmp (rmap_if->route_map_name, rmap_name))
        return rmap_if;
    }
  return NULL;
}

/* Add the given route map name to the route map interface list */
struct route_map_if *
nsm_pbr_route_map_name_add (struct nsm_master *nm, char *name)
{
  struct route_map_if *new = NULL;

  new = XMALLOC (MTYPE_ROUTE_MAP_PBR_IF, sizeof (struct route_map_if));
  if (new == NULL)
    return NULL;
 
  pal_mem_set (new, 0, sizeof (struct route_map_if));
  new->route_map_name = XSTRDUP (MTYPE_ROUTE_MAP_PBR_IF, name);
  new->ifnames = list_new ();
  listnode_add (nm->route_map_if, new);
  return new;
}

/* Checks the given route map is already added to the route map interface list.
   If not then adds the given route map name to route map interface list */
struct route_map_if *
nsm_pbr_route_map_name_get (struct nsm_master *nm, char *name)
{
  struct route_map_if *rmap_if = NULL;

  rmap_if = nsm_pbr_route_map_name_lookup (nm, name);
  if (rmap_if)
    return rmap_if;

  rmap_if = nsm_pbr_route_map_name_add (nm, name);
  return rmap_if;
}

/* Deletes the interface name matching the given interface from route map
   interfaces list */
void 
nsm_pbr_route_map_interface_delete (struct route_map_if *rmap_if,
                                    char *ifname)
{
  if (ifname)
    { 
      listnode_delete (rmap_if->ifnames, ifname);
      XFREE (MTYPE_ROUTE_MAP_PBR_IF, ifname);
    }
}

/* Delete all the members of the route_map_if structure for a given rmap_if */
void 
nsm_pbr_route_map_name_delete (struct nsm_master *nm, 
                               struct route_map_if *rmap_if)
{
  struct listnode *rn = NULL;
  char *ifname = NULL;

  if (rmap_if->route_map_name)
    XFREE (MTYPE_ROUTE_MAP_PBR_IF, rmap_if->route_map_name);
  
  /* Delete interface list associated with route map */
  LIST_LOOP (rmap_if->ifnames, ifname, rn)
    nsm_pbr_route_map_interface_delete (rmap_if, ifname);

  listnode_delete (nm->route_map_if, rmap_if);
  XFREE (MTYPE_ROUTE_MAP_PBR_IF, rmap_if);
}

/* Lookup interface from the list of interfaces of a particular route map */
char *
nsm_pbr_route_map_interface_lookup (struct route_map_if *rmap_if, char *ifname)
{
  struct listnode *rn;
  char *name;

  LIST_LOOP (rmap_if->ifnames, name, rn)
    {
      if (pal_strcmp (name, ifname) == 0)
        return name;
    }
  return NULL;
}

/* Checks if any route map is configured on the given interface */
struct route_map_if *
nsm_pbr_route_map_interface_check (struct nsm_master *nm, char *ifname)
{
  struct listnode *rn = NULL;
  struct route_map_if *rmap_if = NULL;
  
  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    { 
      if (nsm_pbr_route_map_interface_lookup (rmap_if, ifname))
        return rmap_if;
    }
  return NULL;
}

/* Adds given interface name to the list of interfaces of a route map */
void
nsm_pbr_route_map_interface_add (struct route_map_if *rmap_if, char *ifname)
{
  char * name = NULL;
  name = XSTRDUP (MTYPE_ROUTE_MAP_PBR_IF, ifname);
  listnode_add (rmap_if->ifnames, name);
  return ;
}

/* Checks whether the given nexthop is valid or not */
struct interface *
nsm_pbr_nh_validate (struct apn_vr *vr, struct pbr_nexthop *pbr_nexthop)
{
  struct interface *ifp = NULL;
  struct pal_in4_addr nexthop;
  struct connected *ifc = NULL;
  struct prefix pnh;
  s_int32_t ret;

  /* Rule for secondary nexthop */
  if (pbr_nexthop)
    {
      pal_mem_set (&nexthop, 0, sizeof (struct pal_in4_addr));
      ret = pal_inet_pton (AF_INET, (pbr_nexthop->nexthop_str),
                            &nexthop);
      if (ret <= 0)
        return NULL;

      /* interface configured for nexthop */
      if (pbr_nexthop->ifname)
        {
          ifp = if_lookup_by_name (&vr->ifm, pbr_nexthop->ifname);

          /* if the given interface name is valid */
          if (ifp != NULL)
            {
              pnh.family = AF_INET;
              pnh.prefixlen = IPV4_MAX_PREFIXLEN;
              IPV4_ADDR_COPY (&pnh.u.prefix4, &nexthop);
              if (if_match_ifc_ipv4_direct (ifp, &pnh) == NULL)
                return NULL;
            } 
        }
      else
        ifp = if_match_all_by_ipv4_address (&vr->ifm, &nexthop, &ifc);
    }
  return ifp;
}

/* For valid nexthop, send the rule after searching the match list */
void 
nsm_pbr_nh_validate_and_send (int op, struct apn_vr *vr, 
                             struct route_map_index *index, 
                             struct pbr_nexthop *pbr_nexthop, s_int32_t ifindex) 
{
  struct interface *ifp;
  int alloc_flag = 0;

  /* Nexthop validity check */
  if ((ifp = nsm_pbr_nh_validate (vr, pbr_nexthop)))
    {
      if (pbr_nexthop->ifname == NULL)
        {
          pbr_nexthop->ifname = XSTRDUP (MTYPE_PBR_STRING, ifp->name);
          alloc_flag = 1;
        }

      /* Valid nexthop then serch match list */
      nsm_pbr_rule_send (op, vr, index, pbr_nexthop, ifindex);

      if (alloc_flag == 1)
        {
          XFREE (MTYPE_PBR_STRING, pbr_nexthop->ifname);
          pbr_nexthop->ifname = NULL;
        }
    }
}
                                                
/* Traverse through all the filters of access list and send it */
u_int32_t 
nsm_pbr_ip_access_list_traverse (struct apn_vr *vr, int rmap_cmd, 
                                 char *rmap_name, s_int32_t seq_num, 
                                 enum route_map_type rmap_type, char *acl_name, 
                                 struct pbr_nexthop *pbr_nexthop, 
                                 s_int32_t ifindex) 
{
  struct access_list *acl = NULL;
  struct filter_list *filter = NULL;
  struct filter_pacos_ext acl_filter; 
  struct filter_common *cfilter = NULL;
  struct filter_pacos *zfilter = NULL;
  enum filter_type filter_type;
  result_t res = 0;

  /* Lookup ACL. If it does not exist we will configure the rule */
  acl = access_list_lookup (vr, AFI_IP, acl_name);
  if (!acl)
    return -1;

  /* Traverse all the filter list */
  for (filter = acl->head; filter; filter = filter->next)
    {
      /* initial value for filter */  
      filter_type = FILTER_DENY;
      pal_mem_set (&acl_filter, 0, sizeof (struct filter_pacos_ext));
      switch (filter->common)
        {
          case FILTER_COMMON:
            cfilter = &filter->u.cfilter;
            pbr_filter_copy_common (&acl_filter, cfilter);
            break;
          case FILTER_PACOS:
            zfilter = &filter->u.zfilter;
            prefix_copy (&acl_filter.sprefix, &zfilter->prefix); 
            break;
          case FILTER_PACOS_EXT:
            acl_filter = filter->u.zextfilter;
            break;
        }
       
      if ((rmap_type == RMAP_PERMIT && filter->type == FILTER_PERMIT) 
           ||
          (rmap_type == RMAP_DENY && filter->type == FILTER_DENY)) 
        filter_type = FILTER_PERMIT;
      res = nsm_pbr_ip_filter_translate_rule (rmap_cmd, rmap_name, seq_num, 
                                          &acl_filter, filter_type,
                                          pbr_nexthop->nexthop_str, 
                                          pbr_nexthop->nh_type,
                                          pbr_nexthop->ifname, 
                                          ifindex);
    }
  return res;
}

/* Send the policy parameters to the HAL */
u_int32_t 
nsm_pbr_ip_filter_translate_rule (int pbr_cmd, char *rmap_name, 
                              s_int32_t seq_num,
                              struct filter_pacos_ext *acl_filter,
                              enum filter_type type,
                              char *nexthop_str, s_int32_t nh_type,
                              char *ifname, 
                              s_int32_t ifindex)
{
  result_t res = 0;
  s_int32_t ret;
  
  struct pal_in4_addr nexthop;
  
  pal_mem_set (&nexthop, 0, sizeof (struct pal_in4_addr));  
  ret = pal_inet_pton (AF_INET, (nexthop_str), &(nexthop));
  if (ret <= 0)
    return -1;
#ifdef HAVE_HAL
  res = hal_pbr_ipv4_uc_route (pbr_cmd, rmap_name, seq_num,
                              (struct hal_prefix *) &acl_filter->sprefix,
                              (struct hal_prefix *) &acl_filter->dprefix,
                               acl_filter->protocol,
                               acl_filter->sport_op, acl_filter->sport_lo,
                               acl_filter->sport, acl_filter->dport_op,
                               acl_filter->dport_lo, acl_filter->dport,
                               acl_filter->tos_op, acl_filter->tos_lo,
                               acl_filter->tos,
                               type,
                               acl_filter->precedence,
                               (struct hal_in4_addr*) &nexthop, 
                               nh_type,
                               ifname, 
                               ifindex);
#endif /*HAVE_HAL */

  return res;
}
 
/* Get the primary and secondary nexthops configured in set clause */
void
nsm_pbr_route_map_nexthops_get (struct apn_vr *vr,
                           struct route_map_rule_list set_list,
                           struct pbr_nexthop **pbr_pri_nexthop,
                           struct pbr_nexthop **pbr_sec_nexthop)
{
  struct route_map_rule *rule = NULL;
  struct route_map_rule_cmd *cmd = NULL;
  char *command = PBR_ROUTE_MAP_SET_COMMAND;

  cmd = route_map_lookup_set (vr, command);

  /* IP address */
  for (rule = set_list.head; rule; rule = rule->next)
    {
      if (rule->cmd == cmd)
        {
          *pbr_pri_nexthop = rule->pbr_pri_nexthop;
          *pbr_sec_nexthop = rule->pbr_sec_nexthop;
          break;

        }

    }
}

/* Validate nexthop and send rules for match clause set and unset */
void
nsm_pbr_rule_add_delete_by_match (struct apn_vr *vr, char *rmap_name,
                     struct route_map_index *index, char *acl_name, 
                     enum pbr_cmd op)
{
  struct route_map *map = NULL;
  struct route_map_if *rmap_if = NULL;
  char *rmap_en_ifname = NULL;
  struct listnode *node = NULL;
  struct pbr_nexthop *pri_nexthop = NULL;
  struct pbr_nexthop *sec_nexthop = NULL;
  struct nsm_master *nm = NULL;
  s_int32_t ifindex;

  if (acl_name == NULL)
    return;

  nm = vr->proto;

  /* Check route-map configured on interface or not */
  rmap_if = nsm_pbr_route_map_name_lookup (nm, rmap_name);

  /* Given map not configured on any interface */
  if (rmap_if == NULL)
    return;

  map = route_map_lookup_by_name (vr, rmap_name);
  if (map == NULL)
    return ;

  /* Get nexthop associated with route map index */
  nsm_pbr_route_map_nexthops_get (vr, index->set_list, &pri_nexthop,
                             &sec_nexthop);
  if (pri_nexthop == NULL && sec_nexthop == NULL)
    return;
  
  LIST_LOOP (rmap_if->ifnames, rmap_en_ifname, node)
    {
      ifindex =  if_name2index (&vr->ifm, rmap_en_ifname);

      /* Rule for primary nexthop */
      if (pri_nexthop)
        nsm_pbr_nh_validate_and_send (op, vr, index, pri_nexthop, ifindex);

      /* Rule for secondary nexthop */
      if (sec_nexthop)
        nsm_pbr_nh_validate_and_send (op, vr, index, sec_nexthop, ifindex);
    }
}

/* Validate nexthop and send rules for set clause set and unset */
void
nsm_pbr_rule_add_delete_by_set (int op, struct apn_vr *vr,
                            struct route_map_index *index, 
                            struct pbr_nexthop *pbr_nexthop)
{
  struct route_map_if *rmap_if = NULL;
  struct listnode *rn = NULL;
  char *ifname = NULL;
  struct nsm_master *nm = NULL;
  s_int32_t ifindex;
  
  nm = vr->proto;
  rmap_if = nsm_pbr_route_map_name_lookup (nm, index->map->name);

  /* give route map not configured on any interface */
  if (rmap_if == NULL)
    return;
  
  LIST_LOOP (rmap_if->ifnames, ifname, rn)
    {
      ifindex = if_name2index (&vr->ifm, ifname);
      nsm_pbr_nh_validate_and_send (op, vr, index, pbr_nexthop, ifindex);
    }
}

/* Validate nexthop and send rules for ip address change */
void
nsm_pbr_rule_add_delete_by_ip_address (int op, struct apn_vr *vr, 
                                   struct prefix *ifc_prefix)
{
  struct route_map_if *rmap_if = NULL;
  struct listnode *rn = NULL;
  struct route_map *map = NULL;
  struct route_map_index *index = NULL;
  struct pbr_nexthop *pbr_pri_nexthop = NULL;
  struct pbr_nexthop *pbr_sec_nexthop = NULL; 
  struct nsm_master *nm = NULL; 
 
  nm = vr->proto;

  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    {
      /* Check whether the given route map is configured into the system or not */
      map = route_map_lookup_by_name (vr, rmap_if->route_map_name);
      if (map == NULL)
        continue;
      
      /* Get interface index */
            
      /* Apply route map indexes */
      for (index = map->head; index; index = index->next)
        {

          /* Get nexthop and interface */
          nsm_pbr_route_map_nexthops_get (vr, index->set_list, &pbr_pri_nexthop,
                                         &pbr_sec_nexthop);

          if (pbr_pri_nexthop)
            {
              nsm_pbr_rmap_nh_match_and_send (op, vr, index, ifc_prefix, 
                                          rmap_if, pbr_pri_nexthop);
            }
          if (pbr_sec_nexthop)
            {
              nsm_pbr_rmap_nh_match_and_send (op, vr, index, ifc_prefix, 
                                          rmap_if, pbr_sec_nexthop);
            } 
        }
    }
} 

/* Send rules if nexthop address matches with the connected interface address */
void 
nsm_pbr_rmap_nh_match_and_send (int op, struct apn_vr *vr, 
                            struct route_map_index *index, 
                            struct prefix *ifc_prefix,
                                struct route_map_if *rmap_if, 
                                struct pbr_nexthop *pbr_nexthop)
{
  struct pal_in4_addr nexthop;
  struct listnode *rn = NULL;
  char *ifname = NULL;
  struct ls_prefix p;
  s_int32_t ifindex; 
  s_int32_t ret;
  if (pbr_nexthop)
    {
      ret = pal_inet_pton (AF_INET, (pbr_nexthop->nexthop_str),
                            &nexthop);
      if (ret <= 0)
        return;
      ls_prefix_ipv4_set (&p, IPV4_MAX_BITLEN, nexthop);
     
      /* nexthop address is match with connected interface address */   
      if (prefix_match (ifc_prefix, (struct prefix *)&p))
        {
          /* For all the route map configured interfaces */
          LIST_LOOP (rmap_if->ifnames, ifname, rn)
            {
              ifindex = if_name2index (&vr->ifm, ifname);
              nsm_pbr_rule_send (op, vr, index, pbr_nexthop, ifindex);
            }
        }
    }
}

/* Search for the match list with the given command and traverse it */
void 
nsm_pbr_rule_send (int op, struct apn_vr *vr, struct route_map_index *index,
               struct pbr_nexthop *pbr_nexthop, s_int32_t ifindex)
{
  struct route_map_rule *rule = NULL;
  struct route_map_rule_cmd *cmd = NULL; 
  char *command = PBR_ROUTE_MAP_MATCH_COMMAND;

  cmd = route_map_lookup_match (vr, command);

  /* Valid nexthop or interface present then serch match list */
  for (rule = index->match_list.head; rule; rule = rule->next)
    {
      if (rule->cmd == cmd && rule->rule_str)
        nsm_pbr_ip_access_list_traverse (vr, op, index->map->name,
                                         index->pref, index->type,
                                         rule->rule_str, pbr_nexthop,
                                         ifindex);
    }
}

/* Send a rule when an interface comes up */
void nsm_pbr_if_up (struct apn_vr *vr, char *ifname)
{
  struct route_map_if *rmap_if = NULL;
  struct listnode *rn = NULL;
  struct nsm_master *nm = NULL;
  s_int32_t ifindex;
 
  nm = vr->proto;
  ifindex = if_name2index (&vr->ifm, ifname);
 
  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    {
      if (nsm_pbr_route_map_interface_lookup (rmap_if, ifname))
        {
          /* interface associated with single route map */
          nsm_pbr_route_map_check_and_send (vr, RMAP_POLICY_ADD, 
                                        rmap_if->route_map_name, 
                                        ifindex);
          break;
        }
    }    
}
s_int32_t
nsm_pbr_ip_policy_apply (u_int32_t vr_id, char *rmap_name, char *ifname,
                     s_int32_t ifindex)
{
  struct apn_vr *vr = NULL;
  struct route_map_if *rmap_if = NULL;
  struct nsm_master *nm = NULL;

  vr = apn_vr_get_by_id (nzg, vr_id);
  nm = vr->proto;

  /* Check route-map is already applied on given interface */
  if (nsm_pbr_route_map_interface_check (nm, ifname))
    return NSM_API_SET_ERR_IF_MAP_ENABLED;

  /* Check route map interface list for the given route map name*/
  rmap_if = nsm_pbr_route_map_name_lookup (nm, rmap_name);
  if (rmap_if == NULL)
    {
      rmap_if = nsm_pbr_route_map_name_add (nm, rmap_name);
    }

  /* Add interface name to the route map interface list */
  nsm_pbr_route_map_interface_add (rmap_if, ifname);

  /* Check whether the given route map is configured into the system or not */
  nsm_pbr_route_map_check_and_send (vr, RMAP_POLICY_ADD, rmap_name, ifindex);
  return 0;
}

s_int32_t
nsm_pbr_ip_policy_delete (u_int32_t vr_id, char *rmap_name, char *ifname,
                      s_int32_t ifindex)
{
  struct apn_vr *vr = NULL;
  struct route_map_index *index = NULL;
  struct route_map *map = NULL;
  struct route_map_if *rmap_if = NULL;
  struct pbr_nexthop *pbr_pri_nexthop = NULL;
  struct pbr_nexthop *pbr_sec_nexthop = NULL;
  char *pbr_ifname = NULL;
  struct nsm_master *nm = NULL;

  vr = apn_vr_get_by_id (nzg, vr_id);
  nm = vr->proto;

  /* Check route map interface list for the given route map name*/
  rmap_if = nsm_pbr_route_map_name_lookup (nm, rmap_name);
  if (rmap_if == NULL)
    return NSM_API_SET_ERR_MAP_NOT_CONFIGURED;

  /* Check whether the given route map is configured into the system or not */
  map = route_map_lookup_by_name (vr, rmap_name);
  if (map )
    {
      /* Apply route map indexes */
      for (index = map->head; index; index = index->next)
        {
          pbr_pri_nexthop  = NULL;
          pbr_sec_nexthop = NULL;

          /* Get nexthop and interface associated with route map index */
          nsm_pbr_route_map_nexthops_get (vr, index->set_list, &pbr_pri_nexthop,
                                     &pbr_sec_nexthop);

          /* Check nexthop or interface are configured for route map */
          /* If no nexthop or interface then no need to remove PBR rule */
          if (pbr_pri_nexthop == NULL && pbr_sec_nexthop == NULL)
            continue;

          /* Rule for primary nexthop deletion*/
          if (pbr_pri_nexthop)
            nsm_pbr_nh_validate_and_send (RMAP_POLICY_DELETE, vr, index, 
                             pbr_pri_nexthop, ifindex);
          /* Rule for secondary nexthop deletion*/
          if (pbr_sec_nexthop)
            nsm_pbr_nh_validate_and_send (RMAP_POLICY_DELETE, vr, index, 
                            pbr_sec_nexthop, ifindex);
        }
    }

  /* Remove interface from route map interface list */
  if ((pbr_ifname = nsm_pbr_route_map_interface_lookup (rmap_if, ifname)))
    nsm_pbr_route_map_interface_delete (rmap_if, pbr_ifname);
  else
    return NSM_API_SET_ERR_MAP_NOT_CONFIGURED_ON_IF;

  /* Route map policy is not existed in any of the interface then
     remove policy also from route map interface list */
  if (rmap_if->ifnames->count == 0)
    nsm_pbr_route_map_name_delete (nm, rmap_if);

  return 0;
}

/* Send rules for primary and secondary nexthop if they are valid */
u_int32_t
nsm_pbr_route_map_check_and_send (struct apn_vr *vr, int op, 
                              char *rmap_name, s_int32_t ifindex)
{
  struct route_map *map = NULL;
  struct route_map_index *index = NULL;
  struct pbr_nexthop *pbr_pri_nexthop = NULL;
  struct pbr_nexthop *pbr_sec_nexthop = NULL;

  /* Check whether the given route map is configured into the system or not */
  map = route_map_lookup_by_name (vr, rmap_name);
  if (map == NULL)
    return PBR_RMAP_NOT_FOUND; 

   /*Get nexthop and interface associated with route map index */
  /* Apply route map indexes */
  for (index = map->head; index; index = index->next)
    {
      pbr_pri_nexthop  = NULL;
      pbr_sec_nexthop = NULL;

      /* Get nexthop and interface associated with route map index */
      nsm_pbr_route_map_nexthops_get (vr, index->set_list, &pbr_pri_nexthop,
                                 &pbr_sec_nexthop);

      /* There are no set rules configured for the route-map*/
      if (pbr_pri_nexthop == NULL && pbr_sec_nexthop == NULL)
        continue;

      /* Rule for primary nexthop */
      if (pbr_pri_nexthop)
       nsm_pbr_nh_validate_and_send (op, vr, index, pbr_pri_nexthop,
                                 ifindex);
      /* Rule for secondary nexthop */
      if (pbr_sec_nexthop)
        nsm_pbr_nh_validate_and_send (op, vr, index, pbr_sec_nexthop,
                                  ifindex);
    } 
  return PBR_SUCCESS;
}


/* Add and delete rules by acl filter */
void
nsm_pbr_rule_add_delete_by_acl_filter (int op, struct apn_vr *vr, 
                                   char *acl_name,
                                   struct filter_list *filter)
{
  struct route_map_if *rmap_if = NULL;
  struct listnode *rn = NULL, *rn1 = NULL;
  struct route_map *map = NULL;
  struct route_map_index *index = NULL;
  struct route_map_rule *rule = NULL;
  struct pbr_nexthop *pbr_pri_nexthop = NULL;
  struct pbr_nexthop *pbr_sec_nexthop = NULL;
  char *ifname = NULL;
  struct route_map_rule_cmd *cmd = NULL;
  char *command = PBR_ROUTE_MAP_MATCH_COMMAND;
  struct filter_pacos_ext *acl_filter = NULL;
  s_int32_t ifindex;
  enum filter_type filter_type;
  struct nsm_master *nm = vr->proto;
  struct interface *ifp = NULL;
  int alloc_flag = 0;

  
  if (filter == NULL)
    return; 

  acl_filter = &filter->u.zextfilter;  
    
  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    {
      /* Check whether the given route map is configured into the system
         or not */
      map = route_map_lookup_by_name (vr, rmap_if->route_map_name);
      if (map == NULL)
        continue;


      /* Check if map configured on any interface */
      LIST_LOOP (rmap_if->ifnames, ifname, rn1)
        {
          ifindex = if_name2index (&vr->ifm, ifname);
           for (index = map->head; index; index = index->next)
            {

              /* initial value for filter */
              filter_type = FILTER_DENY;

              if ((index->type == RMAP_PERMIT && filter->type == FILTER_PERMIT)
                 ||
                (index->type == RMAP_DENY && filter->type == FILTER_DENY))
                filter_type = FILTER_PERMIT;

              nsm_pbr_route_map_nexthops_get (vr, index->set_list,
                                         &pbr_pri_nexthop, &pbr_sec_nexthop);
              if (pbr_pri_nexthop != NULL || pbr_sec_nexthop != NULL)
                {
                  cmd = route_map_lookup_match (vr, command); 
                  for (rule = index->match_list.head; rule; rule = rule->next)
                    {
                      if ( rule->cmd == cmd && rule->rule_str &&
                          pal_strcmp (acl_name, rule->rule_str) == 0)
                        {
                          if ((ifp = nsm_pbr_nh_validate (vr, pbr_pri_nexthop)))
                            {
                              if (pbr_pri_nexthop->ifname == NULL)
                                {
                                  pbr_pri_nexthop->ifname = 
                                     XSTRDUP (MTYPE_PBR_STRING, ifp->name);
                                  alloc_flag = 1;
                                }
                              nsm_pbr_ip_filter_translate_rule (op, index->map->name,
                                          index->pref,
                                          acl_filter, filter_type,
                                          pbr_pri_nexthop->nexthop_str,
                                          pbr_pri_nexthop->nh_type,
                                          pbr_pri_nexthop->ifname, ifindex);

                              /* If nsm allocates for interface then free here */
                              if (alloc_flag == 1)
                                {
                                  XFREE (MTYPE_PBR_STRING, pbr_pri_nexthop->ifname);
                                  pbr_pri_nexthop->ifname = NULL;
                                  alloc_flag = 0; 
                                }
                            }

                          if ((ifp = nsm_pbr_nh_validate (vr, pbr_sec_nexthop)))
                            {    
                              if (pbr_sec_nexthop->ifname == NULL)
                                {
                                  pbr_sec_nexthop->ifname = 
                                     XSTRDUP (MTYPE_PBR_STRING, ifp->name);
                                  alloc_flag = 1;
                                }
                              nsm_pbr_ip_filter_translate_rule (op, index->map->name,
                                               index->pref,
                                               acl_filter, filter_type,
                                               pbr_sec_nexthop->nexthop_str, 
                                               pbr_sec_nexthop->nh_type,
                                               pbr_sec_nexthop->ifname, ifindex);

                               /* If nsm allocates for interface then free here */
                              if (alloc_flag == 1)
                                {
                                  XFREE (MTYPE_PBR_STRING, pbr_sec_nexthop->ifname);
                                  pbr_sec_nexthop->ifname = NULL;
                                  alloc_flag = 0;
                                } 
                            }  /* End of nexthop validity */
                        }   
                    }
                } /* End of nexthop availability check */
            }   /* End of for loop */
        } /* End of second LIST_LOOP */
    } /* End of initial LIST_LOOP */
}

/* Delete rules by route map index */
void
nsm_pbr_rules_delete_by_index (struct apn_vr *vr, struct route_map_index *index)
{
  struct route_map_if *rmap_if = NULL;
  struct listnode *rn = NULL;
  struct listnode *rn1 = NULL;
  struct pbr_nexthop *pbr_pri_nexthop = NULL;
  struct pbr_nexthop *pbr_sec_nexthop = NULL;
  char *ifname = NULL;
  struct nsm_master *nm = vr->proto;

  s_int32_t ifindex;
  
  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    {
      if (pal_strcmp (rmap_if->route_map_name, index->map->name) == 0)
        {
          /* Get nexthop and interface associated with route map index */
          nsm_pbr_route_map_nexthops_get (vr, index->set_list, &pbr_pri_nexthop,
                                     &pbr_sec_nexthop);
          if (pbr_pri_nexthop == NULL && pbr_sec_nexthop == NULL)
            break;

          /* for all the interfaces configured with this route-map */
          LIST_LOOP (rmap_if->ifnames, ifname, rn1)
            {
              ifindex = if_name2index (&vr->ifm, ifname);
              /* Rule for primary nexthop deletion*/
              if (pbr_pri_nexthop)
                nsm_pbr_nh_validate_and_send (RMAP_POLICY_DELETE, vr, index, 
                                          pbr_pri_nexthop, ifindex);

              /* Rule for secondary nexthop deletion*/
              if (pbr_sec_nexthop)
                nsm_pbr_nh_validate_and_send (RMAP_POLICY_DELETE, vr, index, 
                                          pbr_sec_nexthop, ifindex); 
            }             
        }
    }
}


/* Send rules to delete all the route map information as well as rmap_if
   when vr is deleted */
void 
nsm_pbr_route_map_if_finish (struct nsm_master *nm)
{
  struct listnode *rn = NULL;
  struct route_map_if *rmap_if = NULL;
  
  LIST_LOOP (nm->route_map_if, rmap_if, rn)
    {
      nsm_pbr_route_map_rule_send_for_deletion (nm->vr, rmap_if);
      nsm_pbr_route_map_name_delete (nm, rmap_if); 
    }
}

/* Delete rules from hardware */
void
nsm_pbr_route_map_rule_send_for_deletion (struct apn_vr *vr, 
                                      struct route_map_if *rmap_if)
{
  struct listnode *rn = NULL; 
  char *ifname = NULL;
  s_int32_t ifindex; 
  s_int32_t ret = PBR_SUCCESS;
 
  /* Delete rules from hardware */
  LIST_LOOP (rmap_if->ifnames, ifname, rn)
    {
      ifindex = if_name2index (&vr->ifm, ifname);
      ret = nsm_pbr_route_map_check_and_send (vr, RMAP_POLICY_DELETE, 
                                          rmap_if->route_map_name, ifindex);
      if (ret != PBR_SUCCESS)
        break;
    }
}
void 
nsm_pbr_route_map_set_nexthop_add_delete (struct apn_vr *vr, void *data)
{
  struct pbr_rmap_set *nexthop_info = (struct pbr_rmap_set *)data;

  nsm_pbr_rule_add_delete_by_set (nexthop_info->cmd, vr, nexthop_info->index,
                              nexthop_info->pbr_nexthop); 
}

void
nsm_pbr_route_map_match_ip_addr_add_delete (struct apn_vr *vr, void *data)
{
  struct pbr_rmap_match *match_ip = (struct pbr_rmap_match *)data;
  
  nsm_pbr_rule_add_delete_by_match (vr, match_ip->map_name, match_ip->index,
                                    match_ip->rule_str, match_ip->cmd);
  
    
}
void
nsm_pbr_route_map_index_delete (struct apn_vr *vr, void *data)
{
  nsm_pbr_rules_delete_by_index (vr, (struct route_map_index *)data);  
} 

void 
nsm_pbr_process_event (pbr_rmap_event_t event, struct apn_vr *vr, void *data)
{
  switch (event)
    {
      case PBR_RMAP_EVENT_SET_IP_NEXTHOP_ADD_DEL:
        nsm_pbr_route_map_set_nexthop_add_delete (vr, data);
        break; 

      case PBR_RMAP_EVENT_MATCH_IP_ADDR_ADD_DEL:
        nsm_pbr_route_map_match_ip_addr_add_delete (vr, data);
        break; 

      case PBR_RMAP_EVENT_INDEX_DELETE:
        nsm_pbr_route_map_index_delete (vr, data); 
        break; 
    }         

}

void
nsm_pbr_event_hook (struct apn_vr *vr,
                    void (*func) (pbr_rmap_event_t, struct apn_vr *, void *))
{
  vr->pbr_event.event_hook = func;
}

void
nsm_pbr_event_init (struct nsm_master *nm)
{
  nsm_pbr_event_hook (nm->vr, nsm_pbr_process_event);
}

void 
nsm_pbr_access_list_add (struct apn_vr *vr, struct access_list *access,
                         struct filter_list *filter)
{
  if (access)
    nsm_pbr_rule_add_delete_by_acl_filter (RMAP_POLICY_ADD, vr, access->name, filter);
}

void 
nsm_pbr_access_list_delete (struct apn_vr *vr, struct access_list *access,
                         struct filter_list *filter)
{
  if (access)
    nsm_pbr_rule_add_delete_by_acl_filter (RMAP_POLICY_DELETE, vr, access->name, filter);
}

#endif /* HAVE_PBR */
