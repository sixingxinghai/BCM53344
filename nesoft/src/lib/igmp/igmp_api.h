/* Copyright (C) 2001-2005  All Rights Reserved. */

#ifndef _PACOS_IGMP_API_H
#define _PACOS_IGMP_API_H

/*
 * IGMP Global CONFIG CLI APIs
 */

s_int32_t
igmp_limit_set (struct igmp_instance *igi,
                u_int32_t limit,
                u_int8_t *except_alist);
s_int32_t
igmp_limit_unset (struct igmp_instance *igi);
s_int32_t
igmp_snooping_set (struct igmp_instance *igi);
s_int32_t
igmp_snooping_unset (struct igmp_instance *igi);
s_int32_t
igmp_ssm_map_enable_set (struct igmp_instance *igi);
s_int32_t
igmp_ssm_map_enable_unset (struct igmp_instance *igi);
s_int32_t
igmp_ssm_map_static_set (struct igmp_instance *igi,
                         u_int8_t *alist,
                         u_int8_t *msrc_arg);
s_int32_t
igmp_ssm_map_static_unset (struct igmp_instance *igi,
                           u_int8_t *alist,
                           u_int8_t *msrc_arg);

/*
 * IGMP IF CONFIG CLI APIs
 */

s_int32_t
igmp_if_set (struct igmp_instance *igi,
             struct interface *ifp);
s_int32_t
igmp_if_unset (struct igmp_instance *igi,
               struct interface *ifp);
s_int32_t
igmp_offlink_if_set (struct igmp_instance *igi,
             struct interface *ifp);
s_int32_t
igmp_offlink_if_unset (struct igmp_instance *igi,
             struct interface *ifp);
s_int32_t
igmp_if_access_list_set (struct igmp_instance *igi,
                         struct interface *ifp,
                         u_int8_t *alist);
s_int32_t
igmp_if_access_list_unset (struct igmp_instance *igi,
                           struct interface *ifp);
s_int32_t
igmp_if_immediate_leave_set (struct igmp_instance *igi,
                             struct interface *ifp,
                             u_int8_t *alist);
s_int32_t
igmp_if_immediate_leave_unset (struct igmp_instance *igi,
                               struct interface *ifp);
s_int32_t
igmp_if_lmqc_set (struct igmp_instance *igi,
                  struct interface *ifp,
                  u_int32_t lmqc);
s_int32_t
igmp_if_lmqc_unset (struct igmp_instance *igi,
                    struct interface *ifp);
s_int32_t
igmp_if_lmqi_set (struct igmp_instance *igi,
                  struct interface *ifp,
                  u_int32_t lmqi);
s_int32_t
igmp_if_lmqi_unset (struct igmp_instance *igi,
                    struct interface *ifp);
s_int32_t
igmp_if_limit_set (struct igmp_instance *igi,
                   struct interface *ifp,
                   u_int32_t limit,
                   u_int8_t *except_alist);
s_int32_t
igmp_if_limit_unset (struct igmp_instance *igi,
                     struct interface *ifp);
s_int32_t
igmp_if_mroute_pxy_set (struct igmp_instance *igi,
                        struct interface *ifp,
                        u_int8_t *mrtr_pxy_ifname);
s_int32_t
igmp_if_mroute_pxy_unset (struct igmp_instance *igi,
                          struct interface *ifp);
s_int32_t
igmp_if_pxy_service_set (struct igmp_instance *igi,
                         struct interface *ifp);
s_int32_t
igmp_if_pxy_service_unset (struct igmp_instance *igi,
                           struct interface *ifp);
s_int32_t
igmp_if_querier_timeout_set (struct igmp_instance *igi,
                             struct interface *ifp,
                             u_int16_t other_querier_interval);
s_int32_t
igmp_if_querier_timeout_unset (struct igmp_instance *igi,
                               struct interface *ifp);
s_int32_t
igmp_if_query_interval_set (struct igmp_instance *igi,
                            struct interface *ifp,
                            u_int32_t query_interval);
s_int32_t
igmp_if_query_interval_unset (struct igmp_instance *igi,
                              struct interface *ifp);
s_int32_t
igmp_if_startup_query_interval_set (struct igmp_instance *igi,
                                    struct interface *ifp,
                                    u_int32_t query_interval);
s_int32_t
igmp_if_startup_query_interval_unset (struct igmp_instance *igi,
                                      struct interface *ifp);

s_int32_t
igmp_if_startup_query_count_set (struct igmp_instance *igi,
                                 struct interface *ifp,
                                 u_int32_t query_count);
s_int32_t
igmp_if_startup_query_count_unset (struct igmp_instance *igi,
                                   struct interface *ifp);
s_int32_t
igmp_if_query_response_interval_set (struct igmp_instance *igi,
                                     struct interface *ifp,
                                     u_int32_t response_interval);
s_int32_t
igmp_if_query_response_interval_unset (struct igmp_instance *igi,
                                       struct interface *ifp);
s_int32_t
igmp_if_robustness_var_set (struct igmp_instance *igi,
                            struct interface *ifp,
                            u_int32_t robustness_var);
s_int32_t
igmp_if_robustness_var_unset (struct igmp_instance *igi,
                              struct interface *ifp);
s_int32_t
igmp_if_ra_set (struct igmp_instance *igi,
                struct interface *ifp);
s_int32_t
igmp_if_ra_unset (struct igmp_instance *igi,
                  struct interface *ifp);
s_int32_t
igmp_if_snooping_set (struct igmp_instance *igi,
                      struct interface *ifp);
s_int32_t
igmp_if_snooping_unset (struct igmp_instance *igi,
                        struct interface *ifp);
s_int32_t
igmp_if_snoop_fast_leave_set (struct igmp_instance *igi,
                              struct interface *ifp);
s_int32_t
igmp_if_snoop_fast_leave_unset (struct igmp_instance *igi,
                                struct interface *ifp);
s_int32_t
igmp_if_snoop_mrouter_if_set (struct igmp_instance *igi,
                              struct interface *ifp,
                              u_int8_t *mrtr_ifname);
s_int32_t
igmp_if_snoop_mrouter_if_unset (struct igmp_instance *igi,
                                struct interface *ifp,
                                u_int8_t *mrtr_ifname);
s_int32_t
igmp_if_snoop_querier_set (struct igmp_instance *igi,
                           struct interface *ifp);
s_int32_t
igmp_if_snoop_querier_unset (struct igmp_instance *igi,
                             struct interface *ifp);
s_int32_t
igmp_if_snoop_report_suppress_set (struct igmp_instance *igi,
                                   struct interface *ifp);
s_int32_t
igmp_if_snoop_report_suppress_unset (struct igmp_instance *igi,
                                     struct interface *ifp);
s_int32_t
igmp_if_version_set (struct igmp_instance *igi,
                     struct interface *ifp,
                     u_int16_t version);
s_int32_t
igmp_if_version_unset (struct igmp_instance *igi,
                       struct interface *ifp);
s_int32_t
igmp_if_static_group_source_set (struct igmp_instance *igi,
                                 struct interface *ifp,
                                 struct pal_in4_addr *pgrp,
                                 struct pal_in4_addr *psrc,
                                 u_int8_t *ifname,
                                 bool_t is_ssm_mapped);
s_int32_t
igmp_if_static_group_source_unset (struct igmp_instance *igi,
                                   struct interface *ifp,
                                   struct pal_in4_addr *pgrp,
                                   struct pal_in4_addr *psrc,
                                   u_int8_t *ifname,
                                   bool_t is_ssm_mapped);
void
igmp_static_group_source_flag_unset (struct igmp_group_rec *igr);

s_int32_t
igmp_if_join_group_set (struct igmp_instance *igi,
                        struct interface *ifp,
                        struct pal_in4_addr *pgrp);

s_int32_t
igmp_if_join_group_unset (struct igmp_instance *igi,
                          struct interface *ifp,
                          struct pal_in4_addr *pgrp);

#ifdef HAVE_SNMP

/*
 * IGMP SNMP APIs
 */

s_int32_t
igmp_if_status_set (struct igmp_instance *igi,
                    struct interface *ifp,
                    u_int32_t statusval);
s_int32_t
igmp_if_querier_get (struct igmp_instance *,
                     struct interface **,
                     u_int8_t **,
                     pal_size_t *);
s_int32_t
igmp_if_querier_get_next (struct igmp_instance *,
                          struct interface **,
                          u_int8_t **,
                          pal_size_t *);
s_int32_t
igmp_if_query_interval_get (struct igmp_instance *,
                            struct interface **,
                            u_int8_t **,
                            pal_size_t *);
s_int32_t
igmp_if_query_interval_get_next (struct igmp_instance *,
                                 struct interface **,
                                 u_int8_t **,
                                 pal_size_t *);
s_int32_t
igmp_if_status_get (struct igmp_instance *,
                    struct interface **,
                    u_int8_t **,
                    pal_size_t *);
s_int32_t
igmp_if_status_get_next (struct igmp_instance *,
                         struct interface **,
                         u_int8_t **,
                         pal_size_t *);
s_int32_t
igmp_if_version_get (struct igmp_instance *,
                     struct interface **,
                     u_int8_t **,
                     pal_size_t *);
s_int32_t
igmp_if_version_get_next (struct igmp_instance *,
                          struct interface **,
                          u_int8_t **,
                          pal_size_t *);
s_int32_t
igmp_if_query_response_interval_get (struct igmp_instance *,
                                     struct interface **,
                                     u_int8_t **,
                                     pal_size_t *);
s_int32_t
igmp_if_query_response_interval_get_next (struct igmp_instance *,
                                          struct interface **,
                                          u_int8_t **,
                                          pal_size_t *);
s_int32_t
igmp_if_querier_uptime_get (struct igmp_instance *,
                            struct interface **,
                            u_int8_t **,
                            pal_size_t *);
s_int32_t
igmp_if_querier_uptime_get_next (struct igmp_instance *,
                                 struct interface **,
                                 u_int8_t **,
                                 pal_size_t *);
s_int32_t
igmp_if_querier_expiry_time_get (struct igmp_instance *,
                                 struct interface **,
                                 u_int8_t **,
                                 pal_size_t *);
s_int32_t
igmp_if_querier_expiry_time_get_next (struct igmp_instance *,
                                      struct interface **,
                                      u_int8_t **,
                                      pal_size_t *);
s_int32_t
igmp_if_wrong_version_queries_get (struct igmp_instance *,
                                   struct interface **,
                                   u_int8_t **,
                                   pal_size_t *);
s_int32_t
igmp_if_wrong_version_queries_get_next (struct igmp_instance *,
                                        struct interface **,
                                        u_int8_t **,
                                        pal_size_t *);
s_int32_t
igmp_if_joins_get (struct igmp_instance *,
                   struct interface **,
                   u_int8_t **,
                   pal_size_t *);
s_int32_t
igmp_if_joins_get_next (struct igmp_instance *,
                        struct interface **,
                        u_int8_t **,
                        pal_size_t *);
s_int32_t
igmp_if_mroute_pxy_get (struct igmp_instance *,
                        struct interface **,
                        u_int8_t **,
                        pal_size_t *);
s_int32_t
igmp_if_mroute_pxy_get_next (struct igmp_instance *,
                             struct interface **,
                             u_int8_t **,
                             pal_size_t *);
s_int32_t
igmp_if_groups_get (struct igmp_instance *,
                    struct interface **,
                    u_int8_t **,
                    pal_size_t *);
s_int32_t
igmp_if_groups_get_next (struct igmp_instance *,
                         struct interface **,
                         u_int8_t **,
                         pal_size_t *);
s_int32_t
igmp_if_robustness_var_get (struct igmp_instance *,
                            struct interface **,
                            u_int8_t **,
                            pal_size_t *);
s_int32_t
igmp_if_robustness_var_get_next (struct igmp_instance *,
                                 struct interface **,
                                 u_int8_t **,
                                 pal_size_t *);
s_int32_t
igmp_if_lmqi_get (struct igmp_instance *,
                  struct interface **,
                  u_int8_t **,
                  pal_size_t *);
s_int32_t
igmp_if_lmqi_get_next (struct igmp_instance *,
                       struct interface **,
                       u_int8_t **,
                       pal_size_t *);
s_int32_t
igmp_if_lmqc_get (struct igmp_instance *,
                  struct interface **,
                  u_int8_t **,
                  pal_size_t *);
s_int32_t
igmp_if_lmqc_get_next (struct igmp_instance *,
                       struct interface **,
                       u_int8_t **,
                       pal_size_t *);
s_int32_t
igmp_if_sqc_get (struct igmp_instance *,
                 struct interface **,
                 u_int8_t **,
                 pal_size_t *);
s_int32_t
igmp_if_sqc_get_next (struct igmp_instance *,
                      struct interface **,
                      u_int8_t **,
                      pal_size_t *);
s_int32_t
igmp_if_sqi_get (struct igmp_instance *,
                 struct interface **,
                 u_int8_t **,
                 pal_size_t *);
s_int32_t
igmp_if_sqi_get_next (struct igmp_instance *,
                      struct interface **,
                      u_int8_t **,
                      pal_size_t *);
s_int32_t
igmp_if_cache_last_reporter_get (struct igmp_instance *,
                                 struct interface **,
                                 struct igmp_snmp_rtr_cache_index *,
                                 u_int8_t **,
                                 pal_size_t *);
s_int32_t
igmp_if_cache_last_reporter_get_next (struct igmp_instance *,
                                      struct interface **,
                                      struct igmp_snmp_rtr_cache_index *,
                                      u_int8_t **,
                                      pal_size_t *);
s_int32_t
igmp_if_cache_uptime_get (struct igmp_instance *,
                          struct interface **,
                          struct igmp_snmp_rtr_cache_index *,
                          u_int8_t **,
                          pal_size_t *);
s_int32_t
igmp_if_cache_uptime_get_next (struct igmp_instance *,
                               struct interface **,
                               struct igmp_snmp_rtr_cache_index *,
                               u_int8_t **,
                               pal_size_t *);

s_int32_t
igmp_if_cache_expiry_time_get (struct igmp_instance *,
                               struct interface **,
                               struct igmp_snmp_rtr_cache_index *,
                               u_int8_t **,
                               pal_size_t *);
s_int32_t
igmp_if_cache_expiry_time_get_next (struct igmp_instance *,
                                    struct interface **,
                                    struct igmp_snmp_rtr_cache_index *,
                                    u_int8_t **,
                                    pal_size_t *);
s_int32_t
igmp_if_cache_exclmode_exp_timer_get (struct igmp_instance *,
                                      struct interface **,
                                      struct igmp_snmp_rtr_cache_index *,
                                      u_int8_t **,
                                      pal_size_t *);
s_int32_t
igmp_if_cache_exclmode_exp_timer_get_next (struct igmp_instance *,
                                           struct interface **,
                                           struct igmp_snmp_rtr_cache_index *,
                                           u_int8_t **,
                                           pal_size_t *);
s_int32_t
igmp_if_cache_ver1_host_timer_get (struct igmp_instance *,
                                   struct interface **,
                                   struct igmp_snmp_rtr_cache_index *,
                                   u_int8_t **,
                                   pal_size_t *);
s_int32_t
igmp_if_cache_ver1_host_timer_get_next (struct igmp_instance *,
                                        struct interface **,
                                        struct igmp_snmp_rtr_cache_index *,
                                        u_int8_t **,
                                        pal_size_t *);
s_int32_t
igmp_if_cache_ver2_host_timer_get (struct igmp_instance *,
                                   struct interface **,
                                   struct igmp_snmp_rtr_cache_index *,
                                   u_int8_t **,
                                   pal_size_t *);
s_int32_t
igmp_if_cache_ver2_host_timer_get_next (struct igmp_instance *,
                                        struct interface **,
                                        struct igmp_snmp_rtr_cache_index *,
                                        u_int8_t **,
                                        pal_size_t *);
s_int32_t
igmp_if_cache_src_filter_mode_get (struct igmp_instance *,
                                   struct interface **,
                                   struct igmp_snmp_rtr_cache_index *,
                                   u_int8_t **,
                                   pal_size_t *);
s_int32_t
igmp_if_cache_src_filter_mode_get_next (struct igmp_instance *,
                                        struct interface **,
                                        struct igmp_snmp_rtr_cache_index *,
                                        u_int8_t **,
                                        pal_size_t *);
s_int32_t
igmp_if_inv_cache_address_get (struct igmp_instance *,
                               struct interface **,
                               struct igmp_snmp_inv_rtr_index *index,
                               u_int8_t **,
                               pal_size_t *);

s_int32_t
igmp_if_inv_cache_address_get_next (struct igmp_instance *,
                                    struct interface **,
                                    struct igmp_snmp_inv_rtr_index *index,
                                    u_int8_t **,
                                    pal_size_t *);
s_int32_t
igmp_if_srclist_host_address_get (struct igmp_instance *,
                                  struct interface **,
                                  struct pal_in4_addr *,
                                  struct pal_in4_addr *,
                                  struct  igmp_snmp_rtr_src_list_index *,
                                  u_int8_t **,
                                  pal_size_t *);
s_int32_t
igmp_if_srclist_host_address_get_next (struct igmp_instance *,
                                       struct interface **,
                                       struct pal_in4_addr *,
                                       struct pal_in4_addr *,
                                       struct  igmp_snmp_rtr_src_list_index *,
                                       u_int8_t **,
                                       pal_size_t *);

s_int32_t
igmp_if_srclist_expiry_time_get (struct igmp_instance *,
                                 struct interface **,
                                 struct pal_in4_addr *,
                                 struct pal_in4_addr *,
                                 struct igmp_snmp_rtr_src_list_index *,
                                 u_int8_t **,
                                 pal_size_t *);
s_int32_t
igmp_if_srclist_expiry_time_get_next (struct igmp_instance *,
                                      struct interface **,
                                      struct pal_in4_addr *,
                                      struct pal_in4_addr *,
                                      struct igmp_snmp_rtr_src_list_index *,
                                      u_int8_t **,
                                      pal_size_t *);
#endif /* HAVE_SNMP */

/*
 * IGMP CLEAR CLI API
 */

s_int32_t
igmp_clear (struct igmp_instance *igi,
            struct interface *ifp,
            struct pal_in4_addr *pgrp,
            struct pal_in4_addr *psrc);

/*
 * IGMP Utility API
 */

u_int8_t *
igmp_strerror (s_int32_t iret);

#endif /* _PACOS_IGMP_API_H */
