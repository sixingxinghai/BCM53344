/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _NSM_NEXTHOP_H
#define _NSM_NEXTHOP_H

struct nexthop_lookup_reg
{
  struct prefix p;
  u_char lookup_type;
  struct list client_list; /* List of nse(client). */
};

int nsm_nexthop_process_rib_add (struct nsm_ptree_node *, struct rib *);
int nsm_nexthop_process_rib_delete (struct nsm_ptree_node *, struct rib *);
int nsm_nexthop_process_route_node (struct nsm_ptree_node *);
struct nsm_ptree_node *nsm_nexthop_process_route_node_add (struct nsm_ptree_node *, struct nsm_ptree_table *);
struct nsm_ptree_node *nsm_nexthop_process_route_node_delete (struct nsm_ptree_node *, struct rib *rib, struct nsm_ptree_table *);
void nsm_nexthop_rib_clean_all (struct nsm_ptree_table *);
void nsm_nexthop_nh_table_clean_all (struct nsm_ptree_table *);
void nsm_nexthop_reg_clean (struct nsm_master *, struct nsm_server_client *);

#endif /* _NSM_NEXTHOP_H */
