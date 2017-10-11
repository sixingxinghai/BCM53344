/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_API_H
#define _PACOS_OSPF_API_H

/* OSPF API General error code. */
#define OSPF_API_GET_SUCCESS                                             0
#define OSPF_API_GET_ERROR                                              -1
#define OSPF_API_SET_SUCCESS                                             0
#define OSPF_API_SET_ERROR                                              -1

/* OSPF SNMP-API Error code. */
#define OSPF_API_SET_ERR_WRONG_VALUE                                    -2
#define OSPF_API_SET_ERR_INCONSISTENT_VALUE                             -3

/* OSPF CLI-API. */
#define OSPF_API_SET_ERR_INVALID_VALUE                                  -11
#define OSPF_API_SET_ERR_INVALID_NETWORK                                -12
#define OSPF_API_SET_ERR_INVALID_HELPER_POLICY                          -13
#define OSPF_API_SET_ERR_INVALID_REASON                                 -14
#define OSPF_API_SET_ERR_INVALID_FILTER_TYPE                            -15
#define OSPF_API_SET_ERR_NETWORK_TYPE_INVALID                           -16
#define OSPF_API_SET_ERR_ABR_TYPE_INVALID                               -17
#define OSPF_API_SET_ERR_TIMER_VALUE_INVALID                            -18
#define OSPF_API_SET_ERR_COST_INVALID                                   -19
#define OSPF_API_SET_ERR_AUTH_TYPE_INVALID                              -20
#define OSPF_API_SET_ERR_METRIC_INVALID                                 -21
#define OSPF_API_SET_ERR_METRIC_TYPE_INVALID                            -22
#define OSPF_API_SET_ERR_DISTANCE_INVALID                               -62
#define OSPF_API_SET_ERR_EXTERNAL_LSDB_LIMIT_INVALID                    -23
#define OSPF_API_SET_ERR_OVERFLOW_INTERVAL_INVALID                      -24
#define OSPF_API_SET_ERR_REDISTRIBUTE_PROTO_INVALID                     -25
#define OSPF_API_SET_ERR_DEFAULT_ORIGIN_INVALID                         -26
#define OSPF_API_SET_ERR_REFERENCE_BANDWIDTH_INVALID                    -27
#define OSPF_API_SET_ERR_RESTART_METHOD_INVALID                         -28
#define OSPF_API_SET_ERR_GRACE_PERIOD_INVALID                           -29
#define OSPF_API_SET_ERR_DEPRECATED_COMMAND                             -30

#ifdef HAVE_OSPF_MULTI_AREA
#define OSPF_API_SET_ERR_MULTI_AREA_LINK_CANT_GET                       -31
#define OSPF_API_SET_ERR_MULTI_AREA_ADJ_NOT_SET                         -32
#endif /* HAVE_OSPF_MULTI_AREA */

#define OSPF_API_SET_ERR_EXT_MULTI_INST_NOT_ENABLED                     -34

#define OSPF_API_SET_ERR_PASSIVE_IF_ENTRY_NOT_EXIST                     -35
#define OSPF_API_SET_ERR_PASSIVE_IF_ENTRY_EXIST                         -36
#define OSPF_API_SET_ERR_NO_PASSIVE_IF_ENTRY_EXIST                      -37

#define OSPF_API_SET_ERR_INVALID_DOMAIN_ID                              -41
#define OSPF_API_SET_ERR_DUPLICATE_DOMAIN_ID                            -42
#define OSPF_API_SET_ERR_SEC_DOMAIN_ID                                  -43
#define OSPF_API_SET_ERROR_REMOVE_SEC                                   -44
#define OSPF_API_SET_ERR_NON_ZERO_DOMAIN_ID                             -45
#define OSPF_API_SET_ERR_INVALID_HEX_VALUE                              -46

#define OSPF_API_SET_ERR_VR_NOT_EXIST                                   -51
#define OSPF_API_SET_ERR_VRF_NOT_EXIST                                  -52
#define OSPF_API_SET_ERR_VRF_ALREADY_BOUND                              -53
#define OSPF_API_SET_ERR_NETWORK_EXIST                                  -54
#define OSPF_API_SET_ERR_NETWORK_NOT_EXIST                              -55
#define OSPF_API_SET_ERR_VLINK_NOT_EXIST                                -56
#define OSPF_API_SET_ERR_VLINK_CANT_GET                                 -57
#define OSPF_API_SET_ERR_SUMMARY_ADDRESS_EXIST                          -58
#define OSPF_API_SET_ERR_SUMMARY_ADDRESS_NOT_EXIST                      -59
#define OSPF_API_SET_ERR_DISTANCE_NOT_EXIST                             -60
#define OSPF_API_SET_ERR_HOST_ENTRY_EXIST                               -61
#define OSPF_API_SET_ERR_HOST_ENTRY_NOT_EXIST                           -62
#define OSPF_API_SET_ERR_REDISTRIBUTE_NOT_SET                           -63
#define OSPF_API_SET_ERR_MD5_KEY_EXIST                                  -64
#define OSPF_API_SET_ERR_MD5_KEY_NOT_EXIST                              -65
#define OSPF_API_SET_ERR_READONLY                                       -66
#define OSPF_API_SET_ERR_INVALID_IPV4_ADDRESS                           -67
#define OSPF_API_SET_ERR_NETWORK_OWNED_BY_ANOTHER_AREA                  -68
#define OSPF_API_SET_ERR_NETWORK_WITH_ANOTHER_INST_ID_EXIST             -69
#define OSPF_API_SET_ERR_NETWORK_NOT_ACTIVE                             -70

#define OSPF_API_SET_ERR_PROCESS_ID_INVALID                             -101
#define OSPF_API_SET_ERR_PROCESS_NOT_EXIST                              -102
#define OSPF_API_SET_ERR_SELF_REDIST                                    -103
#define OSPF_API_SET_ERR_SAME_ROUTER_ID_EXIST                           -104

#define OSPF_API_SET_ERR_IF_NOT_EXIST                                   -201
#define OSPF_API_SET_ERR_IF_PARAM_NOT_CONFIGURED                        -202
#define OSPF_API_SET_ERR_IF_COST_INVALID                                -203
#define OSPF_API_SET_ERR_IF_TRANSMIT_DELAY_INVALID                      -204
#define OSPF_API_SET_ERR_IF_RETRANSMIT_INTERVAL_INVALID                 -205
#define OSPF_API_SET_ERR_IF_HELLO_INTERVAL_INVALID                      -206
#define OSPF_API_SET_ERR_IF_DEAD_INTERVAL_INVALID                       -207
#define OSPF_API_SET_ERR_IF_RESYNC_TIMEOUT_INVALID                      -208
#define OSPF_API_SET_ERR_IF_INSTANCE_ID_INVALID                         -209
#define OSPF_API_SET_ERR_IF_INST_ID_CANT_SET                            -210
#define OSPF_API_SET_ERR_IF_INST_ID_NOT_MATCH                           -211

#define OSPF_API_SET_ERR_AREA_NOT_EXIST                                 -301
#define OSPF_API_SET_ERR_AREA_NOT_DEFAULT                               -302
#define OSPF_API_SET_ERR_AREA_NOT_NSSA                                  -303
#define OSPF_API_SET_ERR_AREA_IS_BACKBONE                               -304
#define OSPF_API_SET_ERR_AREA_IS_DEFAULT                                -305
#define OSPF_API_SET_ERR_AREA_IS_STUB                                   -306
#define OSPF_API_SET_ERR_AREA_IS_NSSA                                   -307
#define OSPF_API_SET_ERR_AREA_ID_FORMAT_INVALID                         -308
#define OSPF_API_SET_ERR_AREA_ID_NOT_MATCH                              -309
#define OSPF_API_SET_ERR_AREA_TYPE_INVALID                              -310
#define OSPF_API_SET_ERR_AREA_RANGE_NOT_EXIST                           -311
#define OSPF_API_SET_ERR_AREA_HAS_VLINK                                 -312
#define OSPF_API_SET_ERR_AREA_LIMIT                                     -313

#define OSPF_API_SET_ERR_NBR_CONFIG_INVALID                             -401
#define OSPF_API_SET_ERR_NBR_P2MP_CONFIG_REQUIRED                       -402
#define OSPF_API_SET_ERR_NBR_P2MP_CONFIG_INVALID                        -403
#define OSPF_API_SET_ERR_NBR_NBMA_CONFIG_INVALID                        -404
#define OSPF_API_SET_ERR_NBR_STATIC_EXIST                               -405
#define OSPF_API_SET_ERR_NBR_STATIC_NOT_EXIST                           -406
#define OSPF_API_WARN_INVALID_NETWORK                                   -407

#define OSPF_API_SET_ERR_TELINK_FLOOD_SCOPE_ENABLED                     -501
#define OSPF_API_SET_ERR_TELINK_FLOOD_SCOPE_NOT_ENABLED                 -502
#define OSPF_API_SET_ERR_TELINK_METRIC_EXIST                            -503
#define OSPF_API_SET_ERR_TELINK_METRIC_NOT_EXIST                        -504
#define OSPF_API_SET_ERR_TELINK_LOCAL_LSA_ENABLED                       -505
#define OSPF_API_SET_ERR_TELINK_LOCAL_LSA_NOT_ENABLED                   -506

#define OSPF_API_SET_ERR_RMAP_NOT_EXIST                                 -801
#define OSPF_API_SET_ERR_RMAP_INDEX_NOT_EXIST                           -802
#define OSPF_API_SET_ERR_RMAP_COMPILE_ERROR                             -803
#define OSPF_API_SET_ERR_RMAP_RULE_MISSING                              -804

#define OSPF_API_SET_ERR_CSPF_INSTANCE_EXIST                            -901
#define OSPF_API_SET_ERR_CSPF_INSTANCE_NOT_EXIST                        -902
#define OSPF_API_SET_ERR_CSPF_CANT_START                                -903
#define OSPF_API_SET_ERR_MALLOC_FAIL_FOR_ROUTERID                       -904
#define OSPF_API_SET_ERR_IP_ADDR_IN_USE                                 -905
#define OSPF_API_SET_ERR_EMPTY_NEVER_RTR_ID                             -906
#define OSPF_API_SET_ERR_NEVER_RTR_ID_NOT_EXIST                         -907    

/* Admin Stat. */
#define OSPF_API_STATUS_ENABLED  1
#define OSPF_API_STATUS_DISABLED 2

/* OSPF table insert/delete Error code. */
#define OSPF_API_ENTRY_INSERT_ERROR    0
#define OSPF_API_ENTRY_INSERT_SUCCESS  1
#define OSPF_API_ENTRY_DELETE_ERROR    0
#define OSPF_API_ENTRY_DELETE_SUCCESS  1

/* SYNTAX TruthValue from SNMPv2-TC. */
#define OSPF_API_TRUE   1
#define OSPF_API_FALSE  2

/* OSPF Interface Type. */
#define OSPF_API_IFTYPE_NONE              0
#define OSPF_API_IFTYPE_BROADCAST         1
#define OSPF_API_IFTYPE_NBMA              2
#define OSPF_API_IFTYPE_POINTOPOINT       3
#define OSPF_API_IFTYPE_POINTOMULTIPOINT  5

/* OSPF Interface State Machine State. */
#define OSPF_API_IFSM_STATE_DOWN                    1
#define OSPF_API_IFSM_STATE_LOOPBACK                2
#define OSPF_API_IFSM_STATE_WAITING                 3
#define OSPF_API_IFSM_STATE_POINTTOPOINT            4
#define OSPF_API_IFSM_STATE_DESIGNATEDROUTER        5
#define OSPF_API_IFSM_STATE_BACKUPDESIGNATEDROUTER  6
#define OSPF_API_IFSM_STATE_OTHERDESIGNATEDROUTER   7

/* OSPF NBMA Neighbor Permanence. */
#define OSPF_API_NBMA_NBR_DYNAMIC    1
#define OSPF_API_NBMA_NBR_PERMANENT  2

/* OSPF Area Aggregate Type. */
#define OSPF_API_AREA_AGGREGATE_TYPE_SUMMARY 3
#define OSPF_API_AREA_AGGREGATE_TYPE_NSSA    7

/* OSPF Multicast Forwarding. */
#define OSPF_API_MULTICASTFORWARD_BLOCKED    1
#define OSPF_API_MULTICASTFORWARD_MULTICAST  2
#define OSPF_API_MULTICASTFORWARD_UNICAST    3

#define OSPF_API_advertiseMatching       1
#define OSPF_API_doNotAdvertiseMatching  2

/* Definition of Ranges. */
/* Metric. */
#define OSPF_API_Metric_MIN                        0x0000
#define OSPF_API_Metric_MAX                        0xFFFF
/* BigMetric. */
#define OSPF_API_BigMetric_MIN                   0x000000
#define OSPF_API_BigMetric_MAX                   0xFFFFFF
/* PositiveInteger. */
#define OSPF_API_PositiveInteger_MIN           0x00000000
#define OSPF_API_PositiveInteger_MAX           0x7FFFFFFF
/* HelloRange. */
#define OSPF_API_HelloRange_MIN                    0x0001
#define OSPF_API_HelloRange_MAX                    0xFFFF
/* UpToMaxAge. */
#define OSPF_API_UpToMaxAge_MIN                         0
#define OSPF_API_UpToMaxAge_MAX                      3600
/* DesignatedRouterPriority. */
#define OSPF_API_DesignatedRouterPriority_MIN        0x00
#define OSPF_API_DesignatedRouterPriority_MAX        0xFF

/* Process ID. */
#define OSPF_API_PROCESS_ID_MIN  OSPF_PROCESS_ID_MIN
#define OSPF_API_PROCESS_ID_MAX  OSPF_PROCESS_ID_MAX

/* Interface instance-id */
#define OSPF_API_INSTANCE_ID_MIN                        0
#define OSPF_API_INSTANCE_ID_MAX                      255

#define OSPF_API_AuthType_MIN                           0
#define OSPF_API_AuthType_none                          0
#define OSPF_API_AuthType_simplePassword                1
#define OSPF_API_AuthType_md5                           2
#define OSPF_API_AuthType_MAX                           2

#define OSPF_API_CHECK_RANGE(V, C)                                            \
    ((V) >= OSPF_API_ ## C ## _MIN && (V) <= OSPF_API_ ## C ## _MAX)

#define OSPF_API_CHECK_SELF_REDIST(V, C)                                      \
    ((V) == (C) ? 1 : 0)                     

/* OSPF Neighbor Static configuration option.  */
#define OSPF_API_NBR_CONFIG_COST                (1 << 0) 
#define OSPF_API_NBR_CONFIG_POLL_INTERVAL       (1 << 1)
#define OSPF_API_NBR_CONFIG_PRIORITY            (1 << 2)

/* Row Status. */
#define ROW_STATUS_MIN                  1
#define ROW_STATUS_ACTIVE               1
#define ROW_STATUS_NOTINSERVICE         2
#define ROW_STATUS_NOTREADY             3
#define ROW_STATUS_CREATEANDGO          4
#define ROW_STATUS_CREATEANDWAIT        5
#define ROW_STATUS_DESTROY              6
#define ROW_STATUS_MAX                  7

/* Table depth. */
#define OSPF_AREA_TABLE_DEPTH                  4
#define OSPF_STUB_AREA_TABLE_DEPTH             5
#define OSPF_LSDB_TABLE_DEPTH                 13
#define OSPF_AREA_RANGE_TABLE_DEPTH            8
#define OSPF_HOST_TABLE_DEPTH                  5
#define OSPF_IF_TABLE_DEPTH                    8
#define OSPF_IF_METRIC_TABLE_DEPTH             9
#define OSPF_VIRT_IF_TABLE_DEPTH               8
#define OSPF_NBR_TABLE_DEPTH                   8
#define OSPF_VIRT_NBR_TABLE_DEPTH              8
#define OSPF_EXT_LSDB_TABLE_DEPTH              9
#define OSPF_AREA_AGGREGATE_TABLE_DEPTH       13
#define OSPF_LSDB_UPPER_TABLE_DEPTH            5
#define OSPF_LSDB_LOWER_TABLE_DEPTH            8
#define OSPF_AREA_AGGREGATE_UPPER_TABLE_DEPTH  5
#define OSPF_AREA_AGGREGATE_LOWER_TABLE_DEPTH  8

#define OSPF_GENERAL_GROUP                1
#define OSPF_AREA_TABLE                   2
#define OSPF_STUB_AREA_TABLE              3
#define OSPF_LSDB_TABLE                   4
#define OSPF_AREA_RANGE_TABLE             5
#define OSPF_HOST_TABLE                   6
#define OSPF_IF_TABLE                     7
#define OSPF_IF_METRIC_TABLE              8
#define OSPF_VIRT_IF_TABLE                9
#define OSPF_NBR_TABLE                   10
#define OSPF_VIRT_NBR_TABLE              11
#define OSPF_EXT_LSDB_TABLE              12
#define OSPF_ROUTE_GROUP                 13
#define OSPF_AREA_AGGREGATE_TABLE        14

#define OSPF_NBR_STATIC_TABLE            16
#define OSPF_LSDB_UPPER_TABLE            17
#define OSPF_LSDB_LOWER_TABLE            18
#define OSPF_AREA_AGGREGATE_LOWER_TABLE  19

#define OSPF_API_INDEX_VARS_MAX           8
struct ls_table_index
{
  unsigned int len;
  unsigned int octets;
  char vars[OSPF_API_INDEX_VARS_MAX];
};

/* extern variable. */
extern struct ls_table_index ospf_api_table_def[];

/* OSPF CLI-API funcition prototypes.  */
int ospf_process_set (u_int32_t, int);
int ospf_process_set_vrf (u_int32_t, int, char *);
int ospf_process_unset (u_int32_t, int);
int ospf_process_unset_graceful (u_int32_t, int);
int ospf_network_set (u_int32_t, int, struct pal_in4_addr, u_char,
                      struct pal_in4_addr, s_int16_t);
int ospf_network_unset (u_int32_t, int, struct pal_in4_addr, u_char,
                        struct pal_in4_addr, s_int16_t);
int ospf_network_format_set (u_int32_t, int, struct pal_in4_addr, u_char,
                             struct pal_in4_addr, u_char);
int ospf_network_wildmask_set (u_int32_t, int, struct pal_in4_addr, u_char,
                               struct pal_in4_addr, u_char);
#ifdef HAVE_VRF_OSPF
int ospf_domain_id_set (u_int32_t, int, char *, 
                        u_int8_t *, bool_t);
int ospf_domain_id_unset (u_int32_t, int, char *,
                          u_int8_t *, bool_t);
#endif /* HAVE_VRF_OSPF */
int ospf_router_id_set (u_int32_t, int, struct pal_in4_addr);
int ospf_router_id_unset (u_int32_t, int);
int ospf_passive_interface_set (u_int32_t, int, char *);
int ospf_passive_interface_set_by_addr (u_int32_t, int, char *,
                                        struct pal_in4_addr);
int ospf_passive_interface_default_set (u_int32_t, int);
int ospf_passive_interface_unset (u_int32_t, int, char *);
int ospf_passive_interface_unset_by_addr (u_int32_t, int, char *,
                                          struct pal_in4_addr);
int ospf_passive_interface_default_unset (u_int32_t, int);
int ospf_host_entry_set (u_int32_t, int, struct pal_in4_addr,
                         struct pal_in4_addr);
int ospf_host_entry_unset (u_int32_t, int, struct pal_in4_addr,
                           struct pal_in4_addr);
int ospf_host_entry_cost_set (u_int32_t, int, struct pal_in4_addr,
                              struct pal_in4_addr, int);
int ospf_host_entry_cost_unset (u_int32_t, int, struct pal_in4_addr,
                                struct pal_in4_addr);
int ospf_host_entry_format_set (u_int32_t, int, struct pal_in4_addr,
                                struct pal_in4_addr, u_char);
int ospf_abr_type_set (u_int32_t, int, u_char);
int ospf_abr_type_unset (u_int32_t, int);
int ospf_compatible_rfc1583_set (u_int32_t, int);
int ospf_compatible_rfc1583_unset (u_int32_t, int);

int ospf_timers_spf_set (u_int32_t, int, u_int32_t, u_int32_t);
int ospf_timers_spf_unset (u_int32_t, int);
int ospf_timers_refresh_set (u_int32_t, int, int);
int ospf_timers_refresh_unset (u_int32_t, int);

int ospf_auto_cost_reference_bandwidth_set (u_int32_t, int, int);
int ospf_auto_cost_reference_bandwidth_unset (u_int32_t, int);
int ospf_max_concurrent_dd_set (u_int32_t, int, u_int16_t);
int ospf_max_concurrent_dd_unset (u_int32_t, int);
int ospf_max_unuse_packet_set (u_int32_t, int, u_int32_t);
int ospf_max_unuse_packet_unset (u_int32_t, int);
int ospf_max_unuse_lsa_set (u_int32_t, int, u_int32_t);
int ospf_max_unuse_lsa_unset (u_int32_t, int);
#ifdef HAVE_OSPF_DB_OVERFLOW
int ospf_overflow_database_external_limit_set (u_int32_t, int, u_int32_t);
int ospf_overflow_database_external_limit_unset (u_int32_t, int);
int ospf_overflow_database_external_interval_set (u_int32_t, int, int);
int ospf_overflow_database_external_interval_unset (u_int32_t, int);
#endif /* HAVE_OSPF_DB_OVERFLOW */

#ifdef HAVE_OSPF_MULTI_INST
s_int8_t ospf_enable_ext_multi_inst (u_int32_t);
s_int8_t ospf_disable_ext_multi_inst (u_int32_t);
#endif /* HAVE_OSPF_MULTI_INST */

int ospf_if_network_set (u_int32_t, char *, int);
int ospf_if_network_unset (u_int32_t, char *);
int ospf_if_network_p2mp_nbma_set (u_int32_t, char *);
int ospf_if_authentication_type_set (u_int32_t, char *, u_char);
int ospf_if_authentication_type_set_by_addr (u_int32_t, char *,
                                             struct pal_in4_addr, u_char);
int ospf_if_authentication_type_unset (u_int32_t, char *);
int ospf_if_authentication_type_unset_by_addr (u_int32_t, char *,
                                               struct pal_in4_addr);
int ospf_if_priority_set (u_int32_t, char *, u_char);
int ospf_if_priority_set_by_addr (u_int32_t, char *, struct pal_in4_addr,
                                  u_char);
int ospf_if_priority_unset (u_int32_t, char *);
int ospf_if_priority_unset_by_addr (u_int32_t, char *, struct pal_in4_addr);
int ospf_if_mtu_set (u_int32_t, char *, u_int16_t);
int ospf_if_mtu_unset (u_int32_t, char *);
int ospf_if_mtu_ignore_set (u_int32_t, char *);
int ospf_if_mtu_ignore_set_by_addr (u_int32_t, char *, struct pal_in4_addr);
int ospf_if_mtu_ignore_unset (u_int32_t, char *);
int ospf_if_mtu_ignore_unset_by_addr (u_int32_t, char *, struct pal_in4_addr);
int ospf_if_cost_set (u_int32_t, char *, u_int32_t);
int ospf_if_cost_set_by_addr (u_int32_t, char *, struct pal_in4_addr,
                              u_int32_t);
int ospf_if_cost_unset (u_int32_t, char *);
int ospf_if_cost_unset_by_addr (u_int32_t, char *, struct pal_in4_addr);
int ospf_if_transmit_delay_set (u_int32_t, char *, u_int32_t);
int ospf_if_transmit_delay_set_by_addr (u_int32_t, char *, struct pal_in4_addr,
                                        u_int32_t);
int ospf_if_transmit_delay_unset (u_int32_t, char *);
int ospf_if_transmit_delay_unset_by_addr (u_int32_t, char *,
                                          struct pal_in4_addr);
int ospf_if_retransmit_interval_set (u_int32_t, char *, u_int32_t);
int ospf_if_retransmit_interval_set_by_addr (u_int32_t, char *,
                                             struct pal_in4_addr, u_int32_t);
int ospf_if_retransmit_interval_unset (u_int32_t, char *);
int ospf_if_retransmit_interval_unset_by_addr (u_int32_t, char *,
                                               struct pal_in4_addr);
int ospf_if_hello_interval_set (u_int32_t, char *, u_int32_t);
int ospf_if_hello_interval_set_by_addr (u_int32_t, char *, struct pal_in4_addr,
                                        u_int32_t);
int ospf_if_hello_interval_unset (u_int32_t, char *);
int ospf_if_hello_interval_unset_by_addr (u_int32_t, char *,
                                          struct pal_in4_addr);
int ospf_if_dead_interval_set (u_int32_t, char *, u_int32_t);
int ospf_if_dead_interval_set_by_addr (u_int32_t, char *, struct pal_in4_addr,
                                       u_int32_t);
int ospf_if_dead_interval_unset (u_int32_t, char *);
int ospf_if_dead_interval_unset_by_addr (u_int32_t, char *,
                                         struct pal_in4_addr);
int ospf_if_authentication_key_set (u_int32_t, char *, char *);
int ospf_if_authentication_key_set_by_addr (u_int32_t, char *,
                                            struct pal_in4_addr, char *);
int ospf_if_authentication_key_unset (u_int32_t, char *);
int ospf_if_authentication_key_unset_by_addr (u_int32_t, char *,
                                              struct pal_in4_addr);
int ospf_if_message_digest_key_set (u_int32_t, char *, u_char, char *);
int ospf_if_message_digest_key_set_by_addr (u_int32_t, char *,
                                            struct pal_in4_addr,
                                            u_char, char *);
int ospf_if_message_digest_key_unset (u_int32_t, char *, u_char);
int ospf_if_message_digest_key_unset_by_addr (u_int32_t, char *,
                                              struct pal_in4_addr, u_char);
#ifdef HAVE_OSPF_TE
int ospf_if_te_metric_set (u_int32_t, char *, u_int32_t);
int ospf_if_te_metric_unset (u_int32_t, char *);
#endif /* HAVE_OSPF_TE */

int ospf_if_database_filter_set (u_int32_t, char *);
int ospf_if_database_filter_set_by_addr (u_int32_t, char *,
                                         struct pal_in4_addr);
int ospf_if_database_filter_unset (u_int32_t, char *);
int ospf_if_database_filter_unset_by_addr (u_int32_t, char *,
                                           struct pal_in4_addr);
int ospf_if_disable_all_set (u_int32_t, char *);
int ospf_if_disable_all_unset (u_int32_t, char *);

int ospf_if_resync_timeout_set (u_int32_t, char *, u_int32_t);
int ospf_if_resync_timeout_set_by_addr (u_int32_t, char *, struct pal_in4_addr,
                                        u_int32_t);
int ospf_if_resync_timeout_unset (u_int32_t, char *);
int ospf_if_resync_timeout_unset_by_addr (u_int32_t, char *,
                                          struct pal_in4_addr);

int ospf_vlink_set (u_int32_t, int, struct pal_in4_addr, struct pal_in4_addr);
int ospf_vlink_unset (u_int32_t, int, struct pal_in4_addr,
                      struct pal_in4_addr);
int ospf_vlink_format_set (u_int32_t, int, struct pal_in4_addr,
                           struct pal_in4_addr, u_char);
int ospf_vlink_dead_interval_set (u_int32_t, int, struct pal_in4_addr,
                                  struct pal_in4_addr, int);
int ospf_vlink_hello_interval_set (u_int32_t, int, struct pal_in4_addr,
                                   struct pal_in4_addr, int);
int ospf_vlink_retransmit_interval_set (u_int32_t, int, struct pal_in4_addr,
                                        struct pal_in4_addr, int);
int ospf_vlink_transmit_delay_set (u_int32_t, int, struct pal_in4_addr,
                                   struct pal_in4_addr, int);
int ospf_vlink_dead_interval_unset (u_int32_t, int, struct pal_in4_addr,
                                    struct pal_in4_addr);
int ospf_vlink_hello_interval_unset (u_int32_t, int, struct pal_in4_addr,
                                     struct pal_in4_addr);
int ospf_vlink_retransmit_interval_unset (u_int32_t, int, struct pal_in4_addr,
                                          struct pal_in4_addr);
int ospf_vlink_transmit_delay_unset (u_int32_t, int, struct pal_in4_addr,
                                     struct pal_in4_addr);
int ospf_vlink_authentication_type_set (u_int32_t, int, struct pal_in4_addr,
                                        struct pal_in4_addr, int);
int ospf_vlink_authentication_type_unset (u_int32_t, int, struct pal_in4_addr,
                                          struct pal_in4_addr);
int ospf_vlink_authentication_key_set (u_int32_t, int, struct pal_in4_addr,
                                       struct pal_in4_addr, char *);
int ospf_vlink_authentication_key_unset (u_int32_t, int, struct pal_in4_addr,
                                         struct pal_in4_addr);
int ospf_vlink_message_digest_key_set (u_int32_t, int, struct pal_in4_addr,
                                       struct pal_in4_addr, u_char,
                                       char *);
int ospf_vlink_message_digest_key_unset (u_int32_t, int, struct pal_in4_addr,
                                         struct pal_in4_addr, u_char);

int ospf_summary_address_set (u_int32_t, int, struct pal_in4_addr, u_char);
int ospf_summary_address_unset (u_int32_t, int, struct pal_in4_addr, u_char);
int ospf_summary_address_tag_set (u_int32_t, int, struct pal_in4_addr, u_char,
                                  u_int32_t);
int ospf_summary_address_tag_unset (u_int32_t, int, struct pal_in4_addr,
                                    u_char);
int ospf_summary_address_not_advertise_set (u_int32_t, int,
                                            struct pal_in4_addr, u_char);
int ospf_summary_address_not_advertise_unset (u_int32_t, int,
                                              struct pal_in4_addr, u_char);

int ospf_nbr_static_config_check (u_int32_t, int, struct pal_in4_addr, int);
int ospf_nbr_static_set (u_int32_t, int, struct pal_in4_addr);
int ospf_nbr_static_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_nbr_static_priority_set (u_int32_t, int, struct pal_in4_addr, u_char);
int ospf_nbr_static_priority_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_nbr_static_poll_interval_set (u_int32_t, int, struct pal_in4_addr,
                                       int);
int ospf_nbr_static_poll_interval_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_nbr_static_cost_set (u_int32_t, int, struct pal_in4_addr, u_int16_t);
int ospf_nbr_static_cost_unset (u_int32_t, int, struct pal_in4_addr);

int ospf_area_format_set (u_int32_t, int, struct pal_in4_addr, u_char);
int ospf_area_auth_type_set (u_int32_t, int, struct pal_in4_addr, u_char);
int ospf_area_auth_type_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_area_stub_set (u_int32_t, int, struct pal_in4_addr);
int ospf_area_stub_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_area_no_summary_set (u_int32_t, int, struct pal_in4_addr);
int ospf_area_no_summary_unset (u_int32_t, int, struct pal_in4_addr);

int ospf_area_default_cost_set (u_int32_t, int, struct pal_in4_addr,
                                u_int32_t);
int ospf_area_default_cost_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_area_range_set (u_int32_t, int, struct pal_in4_addr,
                         struct pal_in4_addr, u_char);
int ospf_area_range_unset (u_int32_t, int, struct pal_in4_addr,
                           struct pal_in4_addr, u_char);
int ospf_area_range_not_advertise_set (u_int32_t, int, struct pal_in4_addr,
                                       struct pal_in4_addr, u_char);
int ospf_area_range_not_advertise_unset (u_int32_t, int, struct pal_in4_addr,
                                         struct pal_in4_addr, u_char);
int ospf_area_range_substitute_set (u_int32_t, int, struct pal_in4_addr,
                                    struct pal_in4_addr, u_char,
                                    struct pal_in4_addr, u_char);
int ospf_area_range_substitute_unset (u_int32_t, int, struct pal_in4_addr,
                                      struct pal_in4_addr, u_char);
int ospf_area_filter_list_prefix_set (u_int32_t, int, struct pal_in4_addr, int,
                                      char *);
int ospf_area_filter_list_prefix_unset (u_int32_t, int, struct pal_in4_addr,
                                        int);
int ospf_area_filter_list_access_set (u_int32_t, int, struct pal_in4_addr, int,
                                      char *);
int ospf_area_filter_list_access_unset (u_int32_t, int, struct pal_in4_addr,
                                        int);
int ospf_area_export_list_set (u_int32_t, int, struct pal_in4_addr, char *);
int ospf_area_export_list_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_area_import_list_set (u_int32_t, int, struct pal_in4_addr, char *);
int ospf_area_import_list_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_area_shortcut_set (u_int32_t, int, struct pal_in4_addr, u_char);
int ospf_area_shortcut_unset (u_int32_t, int, struct pal_in4_addr);
#ifdef HAVE_OSPF_MULTI_AREA
s_int32_t ospf_multi_area_adjacency_set (u_int32_t , int , struct pal_in4_addr,
                u_char *, struct pal_in4_addr, int );
s_int32_t ospf_multi_area_adjacency_unset (u_int32_t, int, struct pal_in4_addr,
                                    u_char *, struct pal_in4_addr);
#endif /* HAVE_OSPF_MULTI_AREA */
#ifdef HAVE_NSSA
int ospf_area_nssa_set (u_int32_t, int, struct pal_in4_addr);
int ospf_area_nssa_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_area_nssa_translator_role_set (u_int32_t, int, struct pal_in4_addr,
                                        u_char);
int ospf_area_nssa_translator_role_unset (u_int32_t, int, struct pal_in4_addr);
int ospf_area_nssa_no_redistribution_set (u_int32_t, int, struct pal_in4_addr);
int ospf_area_nssa_no_redistribution_unset (u_int32_t, int,
                                            struct pal_in4_addr);
int ospf_area_nssa_default_originate_set (u_int32_t, int, struct pal_in4_addr);
int ospf_area_nssa_default_originate_metric_set (u_int32_t, int,
                                                 struct pal_in4_addr, int);
int ospf_area_nssa_default_originate_metric_type_set (u_int32_t, int,
                                                      struct pal_in4_addr,
                                                      int);
int ospf_area_nssa_default_originate_unset (u_int32_t, int,
                                            struct pal_in4_addr);
#endif /* HAVE_NSSA */

int ospf_opaque_link_lsa_set (u_int32_t, int, struct pal_in4_addr, u_char,
                              u_int32_t, u_char *, u_int32_t);
int ospf_opaque_area_lsa_set (u_int32_t, int, struct pal_in4_addr, u_char,
                              u_int32_t, u_char *, u_int32_t);
int ospf_opaque_as_lsa_set (u_int32_t, int, u_char, u_int32_t, u_char *,
                            u_int32_t);
#define ospf_opaque_lsa_capable_set(A,B)  ospf_capability_opaque_lsa_set(A,B)
#define ospf_opaque_lsa_capable_unset(A,B)ospf_capability_opaque_lsa_unset(A,B)
#define ospf_cspf_set(A,B)              ospf_capability_cspf_set(A,B)
#define ospf_cspf_unset(A,B)            ospf_capability_cspf_unset(A,B)

int ospf_capability_opaque_lsa_set (u_int32_t, int);
int ospf_capability_opaque_lsa_unset (u_int32_t, int);
int ospf_capability_traffic_engineering_set (u_int32_t, int);
int ospf_capability_traffic_engineering_unset (u_int32_t, int);
int ospf_te_link_flood_scope_set (u_int32_t, char *, int,
                                  struct pal_in4_addr, int);
int ospf_te_link_flood_scope_unset (u_int32_t, char *, int,
                                  struct pal_in4_addr);
int ospf_telink_te_metric_set (u_int32_t, char *, u_int32_t);
int ospf_telink_te_metric_unset (u_int32_t, char *);
int ospf_opaque_te_link_local_lsa_enable (u_int32_t, char *);
int ospf_opaque_te_link_local_lsa_disable (u_int32_t, char *);
int ospf_capability_cspf_set (u_int32_t, int);
int ospf_capability_cspf_unset (u_int32_t, int);
int ospf_cspf_better_protection_type (u_int32_t, int, bool_t);
int ospf_enable_db_summary_opt (u_int32_t, int);
int ospf_disable_db_summary_opt (u_int32_t, int);

#define ospf_redistribute_unset(A,B,C)          ospf_redist_proto_unset(A,B,C)
#define ospf_redistribute_default_unset(A,B)    ospf_redist_default_unset(A,B)
int ospf_redistribute_set (u_int32_t, int, int, int, int, int);
int ospf_redistribute_default_set (u_int32_t, int, int, int, int);
int ospf_redist_default_set (u_int32_t, int, int);
int ospf_redist_default_unset (u_int32_t, int);
int ospf_redist_proto_set (u_int32_t, int, int, int);
int ospf_redist_proto_unset (u_int32_t, int, int, int);
int ospf_redist_metric_set (u_int32_t, int, int, int, int);
int ospf_redist_metric_unset (u_int32_t, int, int, int);
int ospf_redist_metric_type_set (u_int32_t, int, int, int, int);
int ospf_redist_metric_type_unset (u_int32_t, int, int, int);
int ospf_redist_tag_set (u_int32_t, int, int, u_int32_t, int);
int ospf_redist_tag_unset (u_int32_t, int, int, int);

int ospf_distribute_list_in_set (u_int32_t, int, char *);
int ospf_distribute_list_in_unset (u_int32_t, int, char *);
int ospf_distribute_list_out_set (u_int32_t, int, int, int, char *);
int ospf_distribute_list_out_unset (u_int32_t, int, int, int, char *);
int ospf_routemap_set (u_int32_t, int, int, char *, int);
int ospf_routemap_unset (u_int32_t, int, int, int);
int ospf_routemap_default_set (u_int32_t, int, char *);
int ospf_routemap_default_unset (u_int32_t, int);
int ospf_default_metric_set (u_int32_t, int, int);
int ospf_default_metric_unset (u_int32_t, int);
int ospf_distance_all_set (u_int32_t, int, int);
int ospf_distance_all_unset (u_int32_t, int);
int ospf_distance_intra_area_set (u_int32_t, int, int);
int ospf_distance_intra_area_unset (u_int32_t, int);
int ospf_distance_inter_area_set (u_int32_t, int, int);
int ospf_distance_inter_area_unset (u_int32_t, int);
int ospf_distance_external_set (u_int32_t, int, int);
int ospf_distance_external_unset (u_int32_t, int);
#ifdef HAVE_RESTART
int ospf_graceful_restart_set (struct ospf_master *, int, int);
int ospf_graceful_restart_unset (struct ospf_master *);
int ospf_restart_helper_policy_set (u_int32_t, int);
int ospf_restart_helper_policy_unset (u_int32_t);
int ospf_restart_helper_grace_period_set (u_int32_t, int);
int ospf_restart_helper_grace_period_unset (u_int32_t);
int ospf_capability_restart_set (u_int32_t, int, int);
int ospf_capability_restart_unset (u_int32_t, int);
int ospf_restart_graceful (struct lib_globals *, u_int16_t , int);
int ospf_restart_helper_never_router_set (u_int32_t, struct pal_in4_addr);
int ospf_restart_helper_never_router_unset (u_int32_t, struct pal_in4_addr);
int ospf_restart_helper_never_router_unset_all (u_int32_t);
#endif /* HAVE_RESTART */

int ospf_max_area_limit_set (struct ospf *, u_int32_t);
int ospf_max_area_limit_unset (struct ospf *);


/* OSPF SNMP-API function prototypes.  */
int ospf_get_router_id (int, struct pal_in4_addr *, u_int32_t);
int ospf_get_admin_stat (int, int *, u_int32_t);
int ospf_get_version_number (int, int *, u_int32_t);
int ospf_get_area_bdr_rtr_status (int, int *, u_int32_t);
int ospf_get_asbdr_rtr_status (int, int *, u_int32_t);
int ospf_get_extern_lsa_count (int, int *, u_int32_t);
int ospf_get_extern_lsa_cksum_sum (int, int *, u_int32_t);
int ospf_get_tos_support (int, int *, u_int32_t);
int ospf_get_originate_new_lsas (int, int *, u_int32_t);
int ospf_get_rx_new_lsas (int, int *, u_int32_t);
int ospf_get_ext_lsdb_limit (int, int *, u_int32_t);
int ospf_get_multicast_extensions (int, int *, u_int32_t);
int ospf_get_exit_overflow_interval (int, int *, u_int32_t);
int ospf_get_demand_extensions (int, int *, u_int32_t);
int ospf_set_router_id (int, struct pal_in4_addr, u_int32_t);
int ospf_set_admin_stat (int, int, u_int32_t);
int ospf_set_asbdr_rtr_status (int, int, u_int32_t);
int ospf_set_tos_support (int, int, u_int32_t);
int ospf_set_ext_lsdb_limit (int, int, u_int32_t);
int ospf_set_multicast_extensions (int, int, u_int32_t);
int ospf_set_exit_overflow_interval (int, int, u_int32_t);
int ospf_set_demand_extensions (int, int, u_int32_t);
int ospf_set_lsdb_limit (struct ospf *, u_int32_t, int, int);

int ospf_get_area_id (int, struct pal_in4_addr, struct pal_in4_addr *, 
                      u_int32_t);
int ospf_get_auth_type (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_import_as_extern (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_spf_runs (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_area_bdr_rtr_count (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_asbdr_rtr_count (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_area_lsa_count (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_area_lsa_cksum_sum (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_area_summary (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_area_status (int, struct pal_in4_addr, int *, u_int32_t);
int ospf_get_next_area_id (int, struct pal_in4_addr *, int,
                           struct pal_in4_addr *, u_int32_t);
int ospf_get_next_auth_type (int, struct pal_in4_addr *, int, int *, u_int32_t);
int ospf_get_next_import_as_extern (int, struct pal_in4_addr *, int, int *, 
                                    u_int32_t);
int ospf_get_next_spf_runs (int, struct pal_in4_addr *, int, int *, u_int32_t);
int ospf_get_next_area_bdr_rtr_count (int, struct pal_in4_addr *, int, int *, 
                                      u_int32_t);
int ospf_get_next_asbdr_rtr_count (int, struct pal_in4_addr *, int, int *, 
                                   u_int32_t);
int ospf_get_next_area_lsa_count (int, struct pal_in4_addr *, int, int *, 
                                  u_int32_t);
int ospf_get_next_area_lsa_cksum_sum (int, struct pal_in4_addr *, int, int *, 
                                      u_int32_t);
int ospf_get_next_area_summary (int, struct pal_in4_addr *, int, int *, 
                                u_int32_t);
int ospf_get_next_area_status (int, struct pal_in4_addr *, int, int *, 
                               u_int32_t);
int ospf_set_auth_type (int, struct pal_in4_addr, int, u_int32_t);
int ospf_set_import_as_extern (int, struct pal_in4_addr, int, u_int32_t);
int ospf_set_area_summary (int, struct pal_in4_addr, int, u_int32_t);
int ospf_set_area_status (int, struct pal_in4_addr, int, u_int32_t);

int ospf_get_stub_area_id (int, struct pal_in4_addr, int,
                           struct pal_in4_addr *, u_int32_t);
int ospf_get_stub_tos (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_stub_metric (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_stub_status (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_stub_metric_type (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_next_stub_area_id (int, struct pal_in4_addr *, int *, int,
                                struct pal_in4_addr *, u_int32_t);
int ospf_get_next_stub_tos (int, struct pal_in4_addr *, int *, int, int *, 
                            u_int32_t);
int ospf_get_next_stub_metric (int, struct pal_in4_addr *, int *, int, int *, 
                               u_int32_t);
int ospf_get_next_stub_status (int, struct pal_in4_addr *, int *, int, int *, 
                               u_int32_t);
int ospf_get_next_stub_metric_type (int, struct pal_in4_addr *, int *, int,
                                    int *, u_int32_t);
int ospf_set_stub_metric (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_stub_status (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_stub_metric_type (int, struct pal_in4_addr, int, int, u_int32_t);

int ospf_get_lsdb_area_id (int, struct pal_in4_addr, int, struct pal_in4_addr,
                           struct pal_in4_addr, struct pal_in4_addr *, 
                           u_int32_t);
int ospf_get_lsdb_type (int, struct pal_in4_addr, int, struct pal_in4_addr,
                        struct pal_in4_addr, int *, u_int32_t);
int ospf_get_lsdb_lsid (int, struct pal_in4_addr, int, struct pal_in4_addr,
                        struct pal_in4_addr, struct pal_in4_addr *, u_int32_t);
int ospf_get_lsdb_router_id (int, struct pal_in4_addr, int,
                             struct pal_in4_addr, struct pal_in4_addr,
                             struct pal_in4_addr *, u_int32_t);
int ospf_get_lsdb_sequence (int, struct pal_in4_addr, int, struct pal_in4_addr,
                            struct pal_in4_addr, int *, u_int32_t);
int ospf_get_lsdb_age (int, struct pal_in4_addr, int, struct pal_in4_addr,
                       struct pal_in4_addr, int *, u_int32_t);
int ospf_get_lsdb_checksum (int, struct pal_in4_addr, int, struct pal_in4_addr,
                            struct pal_in4_addr, int *, u_int32_t);
int ospf_get_lsdb_advertisement (int, struct pal_in4_addr, int,
                                 struct pal_in4_addr, struct pal_in4_addr,
                                 u_char **, size_t *, u_int32_t);
int ospf_get_next_lsdb_area_id (int, struct pal_in4_addr *, int *,
                                struct pal_in4_addr *, struct pal_in4_addr *,
                                int, struct pal_in4_addr *, u_int32_t);
int ospf_get_next_lsdb_type (int, struct pal_in4_addr *, int *,
                             struct pal_in4_addr *, struct pal_in4_addr *, int,
                             int *, u_int32_t);
int ospf_get_next_lsdb_lsid (int, struct pal_in4_addr *, int *,
                             struct pal_in4_addr *, struct pal_in4_addr *, int,
                             struct pal_in4_addr *, u_int32_t);
int ospf_get_next_lsdb_router_id (int, struct pal_in4_addr *, int *,
                                  struct pal_in4_addr *, struct pal_in4_addr *,
                                  int, struct pal_in4_addr *, u_int32_t);
int ospf_get_next_lsdb_sequence (int, struct pal_in4_addr *, int *,
                                 struct pal_in4_addr *, struct pal_in4_addr *,
                                 int, int *, u_int32_t);
int ospf_get_next_lsdb_age (int, struct pal_in4_addr *, int *,
                            struct pal_in4_addr *, struct pal_in4_addr *, int,
                            int *, u_int32_t);
int ospf_get_next_lsdb_checksum (int, struct pal_in4_addr *, int *,
                                 struct pal_in4_addr *, struct pal_in4_addr *,
                                 int, int *, u_int32_t);
int ospf_get_next_lsdb_advertisement (int, struct pal_in4_addr *, int *,
                                      struct pal_in4_addr *,
                                      struct pal_in4_addr *, int, u_char **,
                                      size_t *, u_int32_t);
int ospf_get_area_range_area_id (int, struct pal_in4_addr, struct pal_in4_addr,
                                 struct pal_in4_addr *, u_int32_t);
int ospf_get_area_range_net (int, struct pal_in4_addr, struct pal_in4_addr,
                             struct pal_in4_addr *, u_int32_t);
int ospf_get_area_range_mask (int, struct pal_in4_addr, struct pal_in4_addr,
                              struct pal_in4_addr *, u_int32_t);
int ospf_get_area_range_status (int, struct pal_in4_addr, struct pal_in4_addr,
                                int *, u_int32_t);
int ospf_get_area_range_effect (int, struct pal_in4_addr, struct pal_in4_addr,
                                int *, u_int32_t);
int ospf_get_next_area_range_area_id (int, struct pal_in4_addr *,
                                      struct pal_in4_addr *, int,
                                      struct pal_in4_addr *, u_int32_t);
int ospf_get_next_area_range_net (int, struct pal_in4_addr *,
                                  struct pal_in4_addr *,
                                  int, struct pal_in4_addr *, u_int32_t);
int ospf_get_next_area_range_mask (int, struct pal_in4_addr *,
                                   struct pal_in4_addr *, int,
                                   struct pal_in4_addr *, u_int32_t);
int ospf_get_next_area_range_status (int, struct pal_in4_addr *,
                                     struct pal_in4_addr *, int, int *, 
                                     u_int32_t);
int ospf_get_next_area_range_effect (int, struct pal_in4_addr *,
                                     struct pal_in4_addr *, int, int *, 
                                     u_int32_t);
int ospf_set_area_range_mask (int, struct pal_in4_addr, struct pal_in4_addr,
                              struct pal_in4_addr, u_int32_t);
int ospf_set_area_range_status (int, struct pal_in4_addr, struct pal_in4_addr,
                                int, u_int32_t);
int ospf_set_area_range_effect (int, struct pal_in4_addr, struct pal_in4_addr,
                                int, u_int32_t);

int ospf_get_host_ip_address (int, struct pal_in4_addr, int,
                              struct pal_in4_addr *, u_int32_t);
int ospf_get_host_tos (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_host_metric (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_host_status (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_host_area_id (int, struct pal_in4_addr, int,
                           struct pal_in4_addr *, u_int32_t);
int ospf_get_next_host_ip_address (int, struct pal_in4_addr *, int *, int,
                                   struct pal_in4_addr *, u_int32_t);
int ospf_get_next_host_tos (int, struct pal_in4_addr *, int *, int, int *, 
                            u_int32_t);
int ospf_get_next_host_metric (int, struct pal_in4_addr *, int *, int, int *, 
                               u_int32_t);
int ospf_get_next_host_status (int, struct pal_in4_addr *, int *, int, int *, 
                               u_int32_t);
int ospf_get_next_host_area_id (int, struct pal_in4_addr *, int *, int,
                                struct pal_in4_addr *, u_int32_t);
int ospf_set_host_metric (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_host_status (int, struct pal_in4_addr, int, int, u_int32_t);

int ospf_get_if_ip_address (int, struct pal_in4_addr, int,
                            struct pal_in4_addr *, u_int32_t);
int ospf_get_address_less_if (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_area_id (int, struct pal_in4_addr, int, struct pal_in4_addr *, 
                         u_int32_t);
int ospf_get_if_type (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_admin_stat (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_rtr_priority (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_transit_delay (int, struct pal_in4_addr, int, int *, 
                               u_int32_t);
int ospf_get_if_retrans_interval (int, struct pal_in4_addr, int, int *, 
                                  u_int32_t);
int ospf_get_if_hello_interval (int, struct pal_in4_addr, int, int *, 
                                u_int32_t);
int ospf_get_if_rtr_dead_interval (int, struct pal_in4_addr, int, int *, 
                                   u_int32_t);
int ospf_get_if_poll_interval (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_state (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_designated_router (int, struct pal_in4_addr, int,
                                   struct pal_in4_addr *, u_int32_t);
int ospf_get_if_backup_designated_router (int, struct pal_in4_addr, int,
                                          struct pal_in4_addr *, u_int32_t);
int ospf_get_if_events (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_auth_key (int, struct pal_in4_addr, int, char **, u_int32_t);
int ospf_get_if_status (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_multicast_forwarding (int, struct pal_in4_addr, int, int *,
                                      u_int32_t);
int ospf_get_if_demand (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_if_auth_type (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_next_if_ip_address (int, struct pal_in4_addr *, int *, int,
                                 struct pal_in4_addr *, u_int32_t);
int ospf_get_next_address_less_if (int, struct pal_in4_addr *, int *, int,
                                   int *, u_int32_t);
int ospf_get_next_if_area_id (int, struct pal_in4_addr *, int *, int,
                              struct pal_in4_addr *, u_int32_t);
int ospf_get_next_if_type (int, struct pal_in4_addr *, int *, int, int *, 
                           u_int32_t);
int ospf_get_next_if_admin_stat (int, struct pal_in4_addr *, int *, int,
                                 int *, u_int32_t);
int ospf_get_next_if_rtr_priority (int, struct pal_in4_addr *, int *, int,
                                   int *, u_int32_t);
int ospf_get_next_if_transit_delay (int, struct pal_in4_addr *, int *, int,
                                    int *, u_int32_t);
int ospf_get_next_if_retrans_interval (int, struct pal_in4_addr *, int *, int,
                                       int *, u_int32_t);
int ospf_get_next_if_hello_interval (int, struct pal_in4_addr *, int *, int,
                                     int *, u_int32_t);
int ospf_get_next_if_rtr_dead_interval (int, struct pal_in4_addr *, int *, int,
                                        int *, u_int32_t);
int ospf_get_next_if_poll_interval (int, struct pal_in4_addr *, int *, int,
                                    int *, u_int32_t);
int ospf_get_next_if_state (int, struct pal_in4_addr *, int *, int,
                                    int *, u_int32_t);
int ospf_get_next_if_designated_router (int, struct pal_in4_addr *, int *, int,
                                        struct pal_in4_addr *, u_int32_t);
int ospf_get_next_if_backup_designated_router (int, struct pal_in4_addr *,
                                               int *, int,
                                               struct pal_in4_addr *,
                                               u_int32_t);
int ospf_get_next_if_events (int, struct pal_in4_addr *, int *, int, int *, 
                             u_int32_t);
int ospf_get_next_if_auth_key (int, struct pal_in4_addr *, int *, int,
                               char **, u_int32_t);
int ospf_get_next_if_status (int, struct pal_in4_addr *, int *, int, int *,
                             u_int32_t);
int ospf_get_next_if_multicast_forwarding (int, struct pal_in4_addr *, int *,
                                           int, int *, u_int32_t);
int ospf_get_next_if_demand (int, struct pal_in4_addr *, int *, int, int *,
                             u_int32_t);
int ospf_get_next_if_auth_type (int, struct pal_in4_addr *, int *, int, int *,
                                u_int32_t);
int ospf_set_if_area_id (int, struct pal_in4_addr, int, struct pal_in4_addr,
                         u_int32_t);
int ospf_set_if_type (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_admin_stat (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_rtr_priority (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_transit_delay (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_retrans_interval (int, struct pal_in4_addr, int, int, 
                                  u_int32_t);
int ospf_set_if_hello_interval (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_rtr_dead_interval (int, struct pal_in4_addr, int, int, 
                                   u_int32_t);
int ospf_set_if_poll_interval (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_auth_key (int, struct pal_in4_addr, int, int, char *, 
                          u_int32_t);
int ospf_set_if_status (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_multicast_forwarding (int, struct pal_in4_addr, int, int,
                                      u_int32_t);
int ospf_set_if_demand (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_if_auth_type (int, struct pal_in4_addr, int, int, u_int32_t);

int ospf_get_if_metric_ip_address (int, struct pal_in4_addr, int, int,
                                   struct pal_in4_addr *, u_int32_t);
int ospf_get_if_metric_address_less_if (int, struct pal_in4_addr, int, int,
                                        int *, u_int32_t);
int ospf_get_if_metric_tos (int, struct pal_in4_addr, int, int, int *,
                            u_int32_t);
int ospf_get_if_metric_value (int, struct pal_in4_addr, int, int, int *,
                              u_int32_t);
int ospf_get_if_metric_status (int, struct pal_in4_addr, int, int, int *,
                               u_int32_t);
int ospf_get_next_if_metric_ip_address (int, struct pal_in4_addr *, int *,
                                        int *, int, struct pal_in4_addr *, 
                                        u_int32_t);
int ospf_get_next_if_metric_address_less_if (int, struct pal_in4_addr *, int *,
                                             int *, int, int *, u_int32_t);
int ospf_get_next_if_metric_tos (int, struct pal_in4_addr *, int *, int *, int,
                                 int *, u_int32_t);
int ospf_get_next_if_metric_value (int, struct pal_in4_addr *, int *, int *,
                                   int, int *, u_int32_t);
int ospf_get_next_if_metric_status (int, struct pal_in4_addr *, int *, int *,
                                    int, int *, u_int32_t);
int ospf_set_if_metric_value (int, struct pal_in4_addr, int, int, int, 
                              u_int32_t);
int ospf_set_if_metric_status (int, struct pal_in4_addr, int, int, int, 
                               u_int32_t);

int ospf_get_virt_if_area_id (int, struct pal_in4_addr, struct pal_in4_addr,
                              struct pal_in4_addr *, u_int32_t);
int ospf_get_virt_if_neighbor (int, struct pal_in4_addr, struct pal_in4_addr,
                               struct pal_in4_addr *, u_int32_t);
int ospf_get_virt_if_transit_delay (int, struct pal_in4_addr,
                                    struct pal_in4_addr, int *, 
                                    u_int32_t);
int ospf_get_virt_if_retrans_interval (int, struct pal_in4_addr,
                                       struct pal_in4_addr, int *, 
                                       u_int32_t);
int ospf_get_virt_if_hello_interval (int, struct pal_in4_addr,
                                     struct pal_in4_addr, int *, 
                                     u_int32_t);
int ospf_get_virt_if_rtr_dead_interval (int, struct pal_in4_addr,
                                        struct pal_in4_addr, int *, 
                                        u_int32_t);
int ospf_get_virt_if_state (int, struct pal_in4_addr, struct pal_in4_addr,
                            int *, u_int32_t);
int ospf_get_virt_if_events (int, struct pal_in4_addr, struct pal_in4_addr,
                             int *, u_int32_t);
int ospf_get_virt_if_auth_type (int, struct pal_in4_addr, struct pal_in4_addr,
                                int *, u_int32_t);
int ospf_get_virt_if_auth_key (int, struct pal_in4_addr, struct pal_in4_addr,
                               char **, u_int32_t);
int ospf_get_virt_if_status (int, struct pal_in4_addr, struct pal_in4_addr,
                             int *, u_int32_t);
int ospf_get_next_virt_if_area_id (int, struct pal_in4_addr *,
                                   struct pal_in4_addr *, int,
                                   struct pal_in4_addr *, u_int32_t);
int ospf_get_next_virt_if_neighbor (int, struct pal_in4_addr *,
                                    struct pal_in4_addr *, int,
                                    struct pal_in4_addr *, u_int32_t);
int ospf_get_next_virt_if_transit_delay (int, struct pal_in4_addr *,
                                         struct pal_in4_addr *, int, int *,
                                         u_int32_t);
int ospf_get_next_virt_if_retrans_interval (int, struct pal_in4_addr *,
                                            struct pal_in4_addr *, int, int *,
                                            u_int32_t);
int ospf_get_next_virt_if_hello_interval (int, struct pal_in4_addr *,
                                          struct pal_in4_addr *, int, int *,
                                          u_int32_t);
int ospf_get_next_virt_if_rtr_dead_interval (int, struct pal_in4_addr *,
                                             struct pal_in4_addr *, int,
                                             int *, u_int32_t);
int ospf_get_next_virt_if_state (int, struct pal_in4_addr *,
                                 struct pal_in4_addr *, int, int *,
                                 u_int32_t);
int ospf_get_next_virt_if_events (int, struct pal_in4_addr *,
                                  struct pal_in4_addr *, int, int *,
                                  u_int32_t);
int ospf_get_next_virt_if_auth_type (int, struct pal_in4_addr *,
                                     struct pal_in4_addr *, int, int *,
                                     u_int32_t);
int ospf_get_next_virt_if_auth_key (int, struct pal_in4_addr *,
                                    struct pal_in4_addr *, int, char **,
                                    u_int32_t);
int ospf_get_next_virt_if_status (int, struct pal_in4_addr *,
                                  struct pal_in4_addr *, int, int *,
                                  u_int32_t);
int ospf_set_virt_if_transit_delay (int, struct pal_in4_addr,
                                    struct pal_in4_addr, int,
                                    u_int32_t);
int ospf_set_virt_if_retrans_interval (int, struct pal_in4_addr,
                                       struct pal_in4_addr, int,
                                       u_int32_t);
int ospf_set_virt_if_hello_interval (int, struct pal_in4_addr,
                                     struct pal_in4_addr, int, 
                                     u_int32_t);
int ospf_set_virt_if_rtr_dead_interval (int, struct pal_in4_addr,
                                        struct pal_in4_addr, int, 
                                        u_int32_t);
int ospf_set_virt_if_auth_key (int, struct pal_in4_addr, struct pal_in4_addr,
                               int, char *, u_int32_t);
int ospf_set_virt_if_status (int, struct pal_in4_addr, struct pal_in4_addr,
                             int, u_int32_t);
int ospf_set_virt_if_auth_type (int, struct pal_in4_addr, struct pal_in4_addr,
                                int, u_int32_t);

int ospf_get_nbr_ip_addr (int, struct pal_in4_addr, int,
                          struct pal_in4_addr *, u_int32_t);
int ospf_get_nbr_address_less_index (int, struct pal_in4_addr, int, int *, 
                                     u_int32_t);
int ospf_get_nbr_rtr_id (int, struct pal_in4_addr, int, struct pal_in4_addr *,
                         u_int32_t);
int ospf_get_nbr_options (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_nbr_priority (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_nbr_state (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_nbr_events (int, struct pal_in4_addr, int, int *, u_int32_t);
int ospf_get_nbr_ls_retrans_qlen (int, struct pal_in4_addr, int, int *, 
                                  u_int32_t);
int ospf_get_nbma_nbr_status (int, struct pal_in4_addr, int, int *, 
                              u_int32_t);
int ospf_get_nbma_nbr_permanence (int, struct pal_in4_addr, int, int *, 
                                  u_int32_t);
int ospf_get_nbr_hello_suppressed (int, struct pal_in4_addr, int, int *,
                                   u_int32_t);
int ospf_get_next_nbr_ip_addr (int, struct pal_in4_addr *, int *, int,
                               struct pal_in4_addr *, u_int32_t);
int ospf_get_next_nbr_address_less_index (int, struct pal_in4_addr *, int *,
                                          int, int *, u_int32_t);
int ospf_get_next_nbr_rtr_id (int, struct pal_in4_addr *, int *, int,
                              struct pal_in4_addr *, u_int32_t);
int ospf_get_next_nbr_options (int, struct pal_in4_addr *, int *, int, int *, 
                               u_int32_t);
int ospf_get_next_nbr_priority (int, struct pal_in4_addr *, int *, int, int *, 
                                u_int32_t);
int ospf_get_next_nbr_state (int, struct pal_in4_addr *, int *, int, int *, 
                             u_int32_t);
int ospf_get_next_nbr_events (int, struct pal_in4_addr *, int *, int, int *, 
                              u_int32_t);
int ospf_get_next_nbr_ls_retrans_qlen (int, struct pal_in4_addr *, int *, int,
                                       int *, u_int32_t);
int ospf_get_next_nbma_nbr_status (int, struct pal_in4_addr *, int *, int,
                                   int *, u_int32_t);
int ospf_get_next_nbma_nbr_permanence (int, struct pal_in4_addr *, int *, int,
                                       int *, u_int32_t);
int ospf_get_next_nbr_hello_suppressed (int, struct pal_in4_addr *, int *, int,
                                        int *, u_int32_t);
int ospf_set_nbr_priority (int, struct pal_in4_addr, int, int, u_int32_t);
int ospf_set_nbma_nbr_status (int, struct pal_in4_addr, int, int, u_int32_t);

int ospf_get_virt_nbr_area (int, struct pal_in4_addr, struct pal_in4_addr,
                            struct pal_in4_addr *, u_int32_t);
int ospf_get_virt_nbr_rtr_id (int, struct pal_in4_addr, struct pal_in4_addr,
                              struct pal_in4_addr *, u_int32_t);
int ospf_get_virt_nbr_ip_addr (int, struct pal_in4_addr, struct pal_in4_addr,
                               struct pal_in4_addr *, u_int32_t);
int ospf_get_virt_nbr_options (int, struct pal_in4_addr, struct pal_in4_addr,
                               int *, u_int32_t);
int ospf_get_virt_nbr_state (int, struct pal_in4_addr, struct pal_in4_addr,
                             int *, u_int32_t);
int ospf_get_virt_nbr_events (int, struct pal_in4_addr, struct pal_in4_addr,
                              int *, u_int32_t);
int ospf_get_virt_nbr_ls_retrans_qlen (int, struct pal_in4_addr,
                                       struct pal_in4_addr, int *,
                                       u_int32_t);
int ospf_get_virt_nbr_hello_suppressed (int, struct pal_in4_addr,
                                        struct pal_in4_addr, int *, u_int32_t);
int ospf_get_next_virt_nbr_area (int, struct pal_in4_addr *,
                                 struct pal_in4_addr *, int,
                                 struct pal_in4_addr *, u_int32_t);
int ospf_get_next_virt_nbr_rtr_id (int, struct pal_in4_addr *,
                                   struct pal_in4_addr *, int,
                                   struct pal_in4_addr *, u_int32_t);
int ospf_get_next_virt_nbr_ip_addr (int, struct pal_in4_addr *,
                                    struct pal_in4_addr *, int,
                                    struct pal_in4_addr *, u_int32_t);
int ospf_get_next_virt_nbr_options (int, struct pal_in4_addr *,
                                    struct pal_in4_addr *, int, int *,
                                    u_int32_t);
int ospf_get_next_virt_nbr_state (int, struct pal_in4_addr *,
                                  struct pal_in4_addr *, int, int *, 
                                  u_int32_t);
int ospf_get_next_virt_nbr_events (int, struct pal_in4_addr *,
                                   struct pal_in4_addr *, int, int *, 
                                   u_int32_t);
int ospf_get_next_virt_nbr_ls_retrans_qlen (int, struct pal_in4_addr *,
                                            struct pal_in4_addr *, int, int *,
                                            u_int32_t);
int ospf_get_next_virt_nbr_hello_suppressed (int, struct pal_in4_addr *,
                                             struct pal_in4_addr *, int,
                                             int *, u_int32_t);

int ospf_get_ext_lsdb_type (int, int, struct pal_in4_addr, struct pal_in4_addr,
                            int *, u_int32_t);
int ospf_get_ext_lsdb_lsid (int, int, struct pal_in4_addr, struct pal_in4_addr,
                            struct pal_in4_addr *, u_int32_t);
int ospf_get_ext_lsdb_router_id (int, int, struct pal_in4_addr,
                                 struct pal_in4_addr, struct pal_in4_addr *, 
                                 u_int32_t);
int ospf_get_ext_lsdb_sequence (int, int, struct pal_in4_addr,
                                struct pal_in4_addr, int *, u_int32_t);
int ospf_get_ext_lsdb_age (int, int, struct pal_in4_addr, struct pal_in4_addr,
                           int *, u_int32_t);
int ospf_get_ext_lsdb_checksum (int, int, struct pal_in4_addr,
                                struct pal_in4_addr, int *, u_int32_t);
int ospf_get_ext_lsdb_advertisement (int, int, struct pal_in4_addr,
                                     struct pal_in4_addr, u_char **,
                                     size_t *, u_int32_t);
int ospf_get_next_ext_lsdb_type (int, int *, struct pal_in4_addr *,
                                 struct pal_in4_addr *, int, int *,
                                 u_int32_t);
int ospf_get_next_ext_lsdb_lsid (int, int *, struct pal_in4_addr *,
                                 struct pal_in4_addr *, int,
                                 struct pal_in4_addr *, u_int32_t);
int ospf_get_next_ext_lsdb_router_id (int, int *, struct pal_in4_addr *,
                                      struct pal_in4_addr *, int,
                                      struct pal_in4_addr *, u_int32_t);
int ospf_get_next_ext_lsdb_sequence (int, int *, struct pal_in4_addr *,
                                     struct pal_in4_addr *, int, int *, 
                                     u_int32_t);
int ospf_get_next_ext_lsdb_age (int, int *, struct pal_in4_addr *,
                                struct pal_in4_addr *, int, int *, 
                                u_int32_t);
int ospf_get_next_ext_lsdb_checksum (int, int *, struct pal_in4_addr *,
                                     struct pal_in4_addr *, int, int *,
                                     u_int32_t);
int ospf_get_next_ext_lsdb_advertisement (int, int *, struct pal_in4_addr *,
                                          struct pal_in4_addr *, int,
                                          u_char **, size_t *, u_int32_t);

int ospf_get_area_aggregate_area_id (int, struct pal_in4_addr, int,
                                     struct pal_in4_addr, struct pal_in4_addr,
                                     struct pal_in4_addr *, u_int32_t);
int ospf_get_area_aggregate_lsdb_type (int, struct pal_in4_addr, int,
                                       struct pal_in4_addr,
                                       struct pal_in4_addr, int *, 
                                       u_int32_t);
int ospf_get_area_aggregate_net (int, struct pal_in4_addr, int,
                                 struct pal_in4_addr, struct pal_in4_addr,
                                 struct pal_in4_addr *, u_int32_t);
int ospf_get_area_aggregate_mask (int, struct pal_in4_addr, int,
                                  struct pal_in4_addr, struct pal_in4_addr,
                                  struct pal_in4_addr *, u_int32_t);
int ospf_get_area_aggregate_status (int, struct pal_in4_addr, int,
                                    struct pal_in4_addr, struct pal_in4_addr,
                                    int *, u_int32_t);
int ospf_get_area_aggregate_effect (int, struct pal_in4_addr, int,
                                    struct pal_in4_addr, struct pal_in4_addr,
                                    int *, u_int32_t);
int ospf_get_next_area_aggregate_area_id (int, struct pal_in4_addr *, int *,
                                          struct pal_in4_addr *,
                                          struct pal_in4_addr *, int,
                                          struct pal_in4_addr *,
                                          u_int32_t);
int ospf_get_next_area_aggregate_lsdb_type (int, struct pal_in4_addr *, int *,
                                            struct pal_in4_addr *,
                                            struct pal_in4_addr *, int, int *, 
                                            u_int32_t);
int ospf_get_next_area_aggregate_net (int, struct pal_in4_addr *, int *,
                                      struct pal_in4_addr *,
                                      struct pal_in4_addr *, int,
                                      struct pal_in4_addr *,
                                      u_int32_t);
int ospf_get_next_area_aggregate_mask (int, struct pal_in4_addr *, int *,
                                       struct pal_in4_addr *,
                                       struct pal_in4_addr *, int,
                                       struct pal_in4_addr *, 
                                       u_int32_t);
int ospf_get_next_area_aggregate_status (int, struct pal_in4_addr *, int *,
                                         struct pal_in4_addr *,
                                         struct pal_in4_addr *, int, int *,
                                         u_int32_t);
int ospf_get_next_area_aggregate_effect (int, struct pal_in4_addr *, int *,
                                         struct pal_in4_addr *,
                                         struct pal_in4_addr *, int, int *, 
                                         u_int32_t);

int ospf_set_area_aggregate_status (int, struct pal_in4_addr, int,
                                    struct pal_in4_addr, struct pal_in4_addr,
                                    int, u_int32_t);
int ospf_set_area_aggregate_effect (int, struct pal_in4_addr, int,
                                    struct pal_in4_addr, struct pal_in4_addr,
                                    int, u_int32_t);

#ifdef HAVE_SNMP
/* Trap callback register function for ospfTrapPrefix. */
int ospf_trap_callback_set (int, SNMP_TRAP_CALLBACK);
int ospf_trap_callback_unset (int, SNMP_TRAP_CALLBACK);
#endif /* HAVE_SNMP */

#endif /* _PACOS_OSPF_API_H */
