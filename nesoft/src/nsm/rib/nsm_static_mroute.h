/* Copyright (C) 2005  All Rights Reserved. */
#ifndef _NSM_STATIC_MROUTE_H
#define _NSM_STATIC_MROUTE_H

/* Static multicast RIB */
/* For a given prefix the static mroutes will be linked via the
 * next and prev pointers in the rib member.
 */
struct nsm_mrib
{
  struct nsm_ptree_node *rn;

  /* The basic NSM rib structure */
  struct rib rib;

  /* Dependent unicast protocol type */
  u_char uc_rtype;
};

#define RIB2MRIB(r) (struct nsm_mrib *)((u_char *)(r) - apn_offsetof (struct nsm_mrib, rib))

/* Static multicast route configuration. */
struct nsm_mroute_config
{
  /* Linked list. */
  struct nsm_mroute_config *next;

  /* Address family. */
  afi_t afi;

  /* Dependent unicast protocol type */
  u_char uc_rtype;

  /* Administrative distance. */
  u_char distance;

  /* Flags for this static route's type -- same as nexthop type. */
  u_char type;

  /* Nexthop information. */
  union nsm_nexthop gate;

  /* Nexthop interface name. */
  char *ifname;
};

/* Multicast nexthop registration client */
struct nsm_mnh_reg_client
{
  struct nsm_mnh_reg_client *next;

  struct nsm_server_client *nsc;
};

/* Multicast nexthop registration structure */
struct nsm_mnh_reg
{
  struct nsm_mnh_reg *next;

  /* Lookup type */
  u_char lookup_type;

  /* RIB tracking */
  struct rib *rib;

  /* List of clients */
  struct nsm_mnh_reg_client *clients;
};

void nsm_cli_init_static_mroute (struct cli_tree *ctree);
void nsm_ip_mroute_table_clean_all (struct ptree *);
int nsm_config_write_ip_mroute (struct cli *cli, struct nsm_vrf *nv, afi_t afi);
int nsm_mrib_table_free (struct nsm_ptree_table *rt);

/* CLI APIs */
int nsm_ip_mroute_set (struct nsm_master *nm, afi_t afi, vrf_id_t vrf_id, 
    struct prefix *p, char *gate_str, u_char ucast_rtype, u_char distance);
int nsm_ip_mroute_ifname_set (struct nsm_master *nm, afi_t afi, 
    vrf_id_t vrf_id, struct prefix *p, struct prefix *gw, 
    char *ifname, u_char ucast_rtype, u_char distance);
int nsm_ip_mroute_unset (struct nsm_master *nm, afi_t afi, 
    vrf_id_t vrf_id, struct prefix *p, u_char ucast_rtype);

/* Route lookup APIs */
struct rib *nsm_mrib_lookup (struct nsm_vrf *vrf, afi_t afi, struct prefix *p,
                struct nsm_ptree_node **rn_ret, u_int32_t exclude_proto);
struct rib *nsm_mrib_match (struct nsm_vrf *vrf, afi_t afi, struct prefix *p,
                struct nsm_ptree_node **rn_ret, u_int32_t exclude_proto);

/* Multicast nexthop registration functions */
void nsm_multicast_nh_reg_table_clean_all (struct nsm_ptree_table *table);
void nsm_multicast_nh_reg (struct nsm_vrf *nv, afi_t afi,
    struct nsm_server_client *nsc, struct prefix *p, u_char lookup_type);
void nsm_multicast_nh_dereg (struct nsm_vrf *nv, afi_t afi,
    struct nsm_server_client *nsc, struct prefix *p, u_char lookup_type);
void nsm_multicast_nh_reg_client_disconnect (struct nsm_master *nm, 
    struct nsm_server_client *nsc);
void nsm_mnh_reg_reprocess (struct nsm_vrf *nv, afi_t afi, struct prefix *p,
    struct prefix *exact_lookup_p);

void nsm_mrib_handle_unicast_rib_change (struct nsm_vrf *nv, 
    struct prefix *unicast_p);
void nsm_mrib_handle_if_state_change (struct nsm_vrf *nv, afi_t afi,
    struct interface *ifp);
void nsm_static_mroute_delete_by_if (struct nsm_vrf *vrf, afi_t afi,
    struct interface *ifp);
#endif /* _NSM_STATIC_MROUTE_H */
