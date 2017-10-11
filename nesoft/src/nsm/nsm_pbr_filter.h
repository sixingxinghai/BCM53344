/* Copyright (C) 2008  All Rights Reserved. */

#ifndef _PACOS_NSM_PBR_FILTER_H
#define _PACOS_NSM_PBR_FILTER_H

#include "pal.h"
#include "routemap.h"
#include "pbr_filter.h"

#define PBR_SUCCESS                0
#define PBR_RMAP_NOT_FOUND         -1

/* Functions to initialize route map interface structure */
void nsm_pbr_route_map_if_init (struct nsm_master *);
void nsm_pbr_route_map_if_finish (struct nsm_master *);

/* Function to initialize event for PBR */
void  nsm_pbr_event_init (struct nsm_master *);

s_int32_t nsm_pbr_ip_policy_apply (u_int32_t , char *,
                                   char *, s_int32_t );

s_int32_t nsm_pbr_ip_policy_delete (u_int32_t , char *,
                                    char *, s_int32_t );

void nsm_pbr_ip_nh_validate_and_send (int , struct apn_vr *,
                                  struct route_map_index *,
                                  struct pbr_nexthop *,
                                  s_int32_t );

u_int32_t nsm_pbr_ip_access_list_traverse (struct apn_vr *, int , 
                                       char *, s_int32_t ,
                                       enum route_map_type, 
                                       char *,
                                       struct pbr_nexthop *,
                                       s_int32_t);

u_int32_t nsm_pbr_ip_filter_translate_rule (int , char *,
                                        s_int32_t ,
                                        struct filter_pacos_ext *,
                                        enum filter_type, char *, s_int32_t ,
                                        char *, s_int32_t);

struct route_map_if* nsm_pbr_route_map_interface_check (struct nsm_master *, char *);
struct route_map_if * nsm_pbr_route_map_name_lookup (struct nsm_master *, char *);
struct route_map_if * nsm_pbr_route_map_name_add (struct nsm_master *, char *);
struct route_map_if * nsm_pbr_route_map_name_get (struct nsm_master *, char *);
void nsm_pbr_route_map_interface_delete (struct route_map_if *, char *);
void nsm_pbr_route_map_name_delete (struct nsm_master *, struct route_map_if *);
char * nsm_pbr_route_map_interface_lookup (struct route_map_if *, char *);
void nsm_pbr_route_map_interface_add (struct route_map_if *, char *);
void nsm_pbr_route_map_nexthops_get (struct apn_vr *, struct route_map_rule_list,
                                struct pbr_nexthop **, struct pbr_nexthop **);
void nsm_pbr_rule_add_delete_by_ip_address (int , struct apn_vr *, 
                                            struct prefix *);
void nsm_pbr_rule_add_delete_by_match (struct apn_vr *, char *, 
                                   struct route_map_index *, 
                                   char *, enum pbr_cmd);
void nsm_pbr_rmap_nh_match_and_send (int , struct apn_vr *,
                                struct route_map_index *,
                                struct prefix *,
                                struct route_map_if *,
                                struct pbr_nexthop *);
void nsm_pbr_rule_send (int , struct apn_vr *, struct route_map_index *,
                    struct pbr_nexthop *, s_int32_t);
struct interface * nsm_pbr_nh_validate (struct apn_vr *, struct pbr_nexthop *);
void nsm_pbr_nh_validate_and_send (int , struct apn_vr *, struct route_map_index *,
                             struct pbr_nexthop *, s_int32_t);
void nsm_pbr_rule_add_delete_by_set (int , struct apn_vr *,
                                struct route_map_index *,
                                struct pbr_nexthop *);
void nsm_pbr_if_up (struct apn_vr *vr, char *);
u_int32_t nsm_pbr_route_map_check_and_send (struct apn_vr *, int,
                                        char *, s_int32_t);
void nsm_pbr_rules_delete_by_acl (struct apn_vr *, char *);
void nsm_pbr_rule_add_delete_by_acl_filter (int , struct apn_vr *, char *,
                                        struct filter_list *);
void
nsm_pbr_access_list_add (struct apn_vr *vr, struct access_list *access,
                         struct filter_list *filter);
void
nsm_pbr_access_list_delete (struct apn_vr *vr, struct access_list *access,
                         struct filter_list *filter);
void
nsm_pbr_rules_delete_by_index (struct apn_vr *, struct route_map_index *);

void nsm_pbr_route_map_rule_send_for_deletion (struct apn_vr *,
                                      struct route_map_if *);
#endif /* _PACOS_NSM_PBR_FILTER_H */

