/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_VRRP_API_H
#define _PACOS_VRRP_API_H

#include "vrrp.h"

/*
** VRRP API Error returns.
** Starts at -3 because:
**   VRRP_OK            = 0
**   VRRP_FAILURE       = -1
**   VRRP_BAD_PACKET    = -2
*/
#define         VRRP_API_SET_ERR_EXIST                  -3
#define         VRRP_API_SET_ERR_NO_EXIST               -4
#define         VRRP_API_SET_ERR_ENABLED                -5
#define         VRRP_API_SET_ERR_DISABLED               -6
#define         VRRP_API_SET_ERR_BAD_IP_ADDR            -7
#define         VRRP_API_SET_ERR_BAD_INTF               -8
#define         VRRP_API_SET_ERR_VIP_SET                -9
#define         VRRP_API_SET_ERR_VIP_UNSET              -10
#define         VRRP_API_SET_ERR_PRIO_CANT_255          -11
#define         VRRP_API_SET_ERR_PRIO_MUST_255          -12
#define         VRRP_API_SET_ERR_CONFIG_UNSET           -13
#define         VRRP_API_SET_ERR_ENABLE                 -14
#define         VRRP_API_SET_ERR_DISABLE                -15
#define         VRRP_API_SET_ERR_PRIO_MISMATCH          -16
#define         VRRP_API_SET_ERR_NO_INTF_IP             -17
#define         VRRP_API_SET_WARN_DUPLICATE_IF          -18
#define         VRRP_API_SET_ERR_CANT_BE_OWNER          -19
#define         VRRP_API_SET_ERR_MUST_BE_OWNER          -20
#define         VRRP_API_SET_ERR_INTERFACE_CONFLICT_VIP_OWNER -21
#define         VRRP_API_SET_ERR_NO_SUCH_SESSION        -22
#define         VRRP_API_SET_ERR_DUPLICATE_SESSION        -23
#define         VRRP_API_SET_ERR_CANNOT_ADD_VRRP_INSTANCE -24
#define         VRRP_API_SET_ERR_CANNOT_ADD_TO_INTERFACE  -25
#define         VRRP_API_SET_ERR_NO_SUCH_INTERFACE       -26
#define         VRRP_API_SET_ERR_INVALID_LINKLOCAL_ADDRESS -27
#define         VRRP_API_SET_ERR_SEESION_GET_OR_CRE       -28
#define         VRRP_API_MASTER_FOUND                     -29
#define         VRRP_API_SET_ERR_L2_INTERFACE             -30


/* VRRP snmp api return value.  */
#define         VRRP_API_GET_SUCCESS            0
#define         VRRP_API_GET_ERROR             -1
#define         VRRP_API_SET_SUCCESS            0
#define         VRRP_API_SET_ERROR             -1

/*VRRP Snmp Row Status Values */
#define VRRP_SNMP_ROW_STATUS_MIN            0
#define VRRP_SNMP_ROW_STATUS_ACTIVE         1
#define VRRP_SNMP_ROW_STATUS_NOTINSERVICE   2
#define VRRP_SNMP_ROW_STATUS_NOTREADY       3
#define VRRP_SNMP_ROW_STATUS_CREATEANDGO    4
#define VRRP_SNMP_ROW_STATUS_CREATEANDWAIT  5
#define VRRP_SNMP_ROW_STATUS_DESTROY        6
#define VRRP_SNMP_ROW_STATUS_MAX            7


/* Values compatible with the vrrpNewMasterReason  */
#define VRRP_NEW_MASTER_REASON_NOT_MASTER         0
#define VRRP_NEW_MASTER_REASON_PRIORITY           1
#define VRRP_NEW_MASTER_REASON_PREEMPTED          2
#define VRRP_NEW_MASTER_REASON_MASTER_NO_RESPONSE 3

/* Values compatible with vrrpTrapProtoErrReason */
#define VRRP_PROTO_ERR_REASON_HOP_LIMIT_ERROR 0
#define VRRP_PROTO_ERR_REASON_VERSION_ERROR   1
#define VRRP_PROTO_ERR_REASON_CHECKSUM_ERROR  2
#define VRRP_PROTO_ERR_REASON_VRID_ERROR      3

#define         EMPTY_STRING                    ""


/* VRRP_IF master
   This structure is included into struct nsm_if.
   Now we allow multiple VRRP sessions per interface.
*/
typedef struct vrrp_if
{
    struct list      *lh_sess_ipv4;         /* List of VRRP sessions for IPv4*/
#ifdef HAVE_IPV6
    struct list      *lh_sess_ipv6;         /* List of VRRP sessions for IPv6*/
#endif
    u_int32_t         vrrp_flags;          /* */
#define NSM_VRRP_IF_SET_VIP             (1 << 0)
#define NSM_VRRP_IPV4_MCAST_JOINED      (1 << 1)

#ifdef HAVE_IPV6
#define NSM_VRRP_IF_SET_VIP6            (1 << 2)
#define NSM_VRRP_IPV6_MCAST_JOINED      (1 << 3)
#endif

    u_int8_t          vif_old_mac[6];

} VRRP_IF;

typedef struct vrrp_session VRRP_SESSION;

VRRP_SESSION *vrrp_api_lookup_session (int apn_vrid, u_int8_t af_type,
                                       int vrid, u_int32_t ifindex);
int vrrp_api_get_session_by_ifname (int apn_vrid, u_int8_t af_type, int vrid,
                                    char *ifname, VRRP_SESSION **pp_sess);
VRRP_SESSION *vrrp_api_get_session (int apn_vrid, u_int8_t af_type, int vrid,
                                    u_int32_t ifindex);
int vrrp_api_del_session_by_ifname (int apn_vrid, u_int8_t af_type, int vrid,
                                    char *ifname);
int vrrp_api_delete_session (int apn_vrid, u_int8_t af_type, int vrid,
                             u_int32_t ifindex);
int vrrp_api_virtual_ip (int apn_vrid, u_int8_t af_type, int vrid,
                         u_int32_t ifindex, u_int8_t *vip_addr, bool_t is_owner);
int vrrp_api_assign_interface (int apn_vrid, int vrid, u_int32_t ifindex);
int vrrp_api_monitored_circuit (int apn_vrid, u_int8_t af_type,
                                int vrid, u_int32_t ifindex, char *if_str,
                                int priority_delta);
int vrrp_api_priority (int apn_vrid, u_int8_t af_type, int vrid,
                       u_int32_t ifindex, int prio);
int vrrp_api_unset_priority (int apn_vrid, u_int8_t af_type, int vrid,
                             u_int32_t ifindex);
int vrrp_api_advt_interval (int apn_vrid, u_int8_t af_type, int vrid,
                            u_int32_t ifindex, int interval);
int vrrp_api_unset_advt_interval (int apn_vrid, u_int8_t af_type, int vrid,
                                  u_int32_t ifindex);
int vrrp_api_preempt_mode (int apn_vrid, u_int8_t af_type, int vrid,
                           u_int32_t ifindex, int mode);
int vrrp_api_enable_session (int apn_vrid, u_int8_t af_type, int vrid,
                             u_int32_t ifindex);
int vrrp_api_disable_session (int apn_vrid, u_int8_t af_type, int vrid,
                              u_int32_t ifindex);

typedef int (* vrrp_api_sess_walk_fun_t) (VRRP_SESSION *sess, void *ref);
int vrrp_api_walk_sessions (int apn_vrid, void *ref, vrrp_api_sess_walk_fun_t efun, int *num_sess);
int vrrp_api_check_any_sess_master (int apn_vrid);

int vrrp_api_set_vmac_status(int new_vmac_statsu);

/* Check VRRP address. */
bool_t vrrp_check_address (struct interface *, struct pal_in4_addr *);
bool_t vrrp_if_match_vip4_if (VRRP_SESSION *, struct pal_in4_addr *, struct interface *);

/* Interface event APIs. */

int vrrp_if_init (struct interface *ifp);
int vrrp_if_new (struct interface *);
void vrrp_if_delete (struct interface *);
int vrrp_if_up (struct interface *);

void vrrp_if_down (struct interface *);
int vrrp_if_add_ipv4_addr (struct interface *, struct connected *);
int vrrp_if_del_ipv4_addr (struct interface *, struct connected *);
bool_t vrrp_if_is_vipv4_addr (struct interface *ifp, struct pal_in4_addr *addr);
#ifdef HAVE_IPV6
int vrrp_if_add_ipv6_addr (struct interface *, struct connected *);
int vrrp_if_del_ipv6_addr (struct interface *, struct connected *);
bool_t vrrp_if_is_vipv6_addr (struct interface *ifp, struct pal_in6_addr *addr);
bool_t vrrp_if_match_vip6_if (VRRP_SESSION *, struct pal_in6_addr *, struct interface *);
#endif /* HAVE_IPV6 */
int  vrrp_if_add_sess(VRRP_SESSION *sess);
void vrrp_if_del_sess(VRRP_SESSION *sess);
bool_t vrrp_if_can_leave_mcast_grp (VRRP_SESSION *sess);
bool_t vrrp_if_must_join_mcast_grp (VRRP_SESSION *sess);

#ifdef HAVE_SNMP
/*Vrrp SNMP Trap Callback Prototype*/
int vrrp_trap_callback_set (int trapid, SNMP_TRAP_CALLBACK func);

int vrrp_api_enable_asso_entry(int apn_vrid, u_int8_t af_type, u_int8_t vrid, u_int32_t ifindex, u_int8_t *ipaddr,
                               int row_status);
int vrrp_get_new_master_reason (int apn_vrid, int *val);
int vrrp_get_notify (int apn_vrid, int *val);

int vrrp_get_oper_virtual_macaddr (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                  struct vmac_addr *vmac);
int vrrp_get_next_oper_virtual_macaddr (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex, int len,
                                       struct vmac_addr *vmac);
int vrrp_get_oper_state (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                         int *val);
int vrrp_get_next_oper_state (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                              int len, int *val);
int vrrp_get_oper_priority (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                            int *val);
int vrrp_get_next_oper_priority (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                 int len, int *val);
int vrrp_get_oper_addr_count (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                             int *val);
int vrrp_get_next_oper_addr_count (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                  int len, int *val);
int vrrp_get_oper_master_ipaddr (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                u_int8_t *ipaddr);
int vrrp_get_next_oper_master_ipaddr (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                     int len, u_int8_t *ipaddr);
int vrrp_get_oper_primary_ipaddr (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                 u_int8_t *ipaddr);
int vrrp_get_next_oper_primary_ipaddr (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                      int len, u_int8_t *ipaddr);
int vrrp_get_oper_adv_interval (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                               int *val);
int vrrp_get_next_oper_adv_interval (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                    int len, int *val);
int vrrp_get_oper_preempt_mode (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                               int *val);
int vrrp_get_next_oper_preempt_mode (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                    int len, int *val);
int vrrp_get_oper_accept_mode (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                              u_int32_t *val);
int vrrp_get_next_oper_accept_mode (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                   int len, u_int32_t *val);
int vrrp_get_oper_uptime (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                          u_int32_t *val);
int vrrp_get_next_oper_uptime (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                               int len, u_int32_t *val);
int vrrp_get_oper_storage_type (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                               u_int32_t *val);
int vrrp_get_next_oper_storage_type (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                    int len, u_int32_t *val);
int vrrp_get_oper_rowstatus (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                             int *val);
int vrrp_get_next_oper_rowstatus (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                  int len, int *val);
int vrrp_get_asso_storage_type (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex, u_int8_t *ipaddr,
                               u_int32_t *val);
int vrrp_get_next_asso_storage_type (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex, u_int8_t *ipaddr,
                                    int len, u_int32_t *val);
int vrrp_get_asso_ipaddr_rowstatus (int apn_vrid,  u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex, u_int8_t *ipaddr,
                                    int   *val);
int vrrp_get_next_asso_ipaddr_rowstatus (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex, u_int8_t *ipaddr,
                                         int len, int *val);
int vrrp_get_checksum_errors (int apn_vrid, int *val);
int vrrp_get_version_errors (int apn_vrid, int *val);
int vrrp_get_vrid_errors (int apn_vrid, int *val);

int vrrp_get_stats_master_transitions (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                  int *val);
int vrrp_get_next_stats_master_transitions (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                            int len, int *val);
int vrrp_get_stats_rcvd_advertisements (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                        int *val);
int vrrp_get_next_stats_rcvd_advertisements (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                             int len, int *val);
int vrrp_get_stats_adv_interval_errors (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                        int *val);
int vrrp_get_next_stats_adv_interval_errors (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                             int len, int *val);
int vrrp_get_stats_ip_ttl_errors (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                  int *val);
int vrrp_get_next_stats_ip_ttl_errors (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                       int len, int *val);
int vrrp_get_stats_rcvd_pri_zero_packets (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex, int *val);
int vrrp_get_next_stats_rcvd_pri_zero_packets (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                                 int len, int *val);
int vrrp_get_stats_sent_pri_zero_packets (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                            int *val);
int vrrp_get_next_stats_sent_pri_zero_packets (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                               int len, int *val);
int vrrp_get_stats_rcvd_invalid_type_pkts (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                           int *val);
int vrrp_get_next_stats_rcvd_invalid_type_pkts (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                          int len, int *val);
int vrrp_get_stats_address_list_errors (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex, int *val);
int vrrp_get_next_stats_address_list_errors (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                             int len, int *val);
int vrrp_get_stats_packet_length_errors (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                         int *val);
int vrrp_get_next_stats_packet_length_errors (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                              int len, int *val);
int vrrp_get_stats_rcvd_invalid_authentications (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex, int *val);
int vrrp_get_next_stats_rcvd_invalid_authentications (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                           int len, int *val);
int vrrp_get_stats_discontinuity_time (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                       int *val);
int vrrp_get_next_stats_discontinuity_time (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                            int len, int *val);
int vrrp_get_stats_refresh_rate (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                 int *val);
int vrrp_get_next_stats_refresh_rate (int apn_vrid, u_int8_t *aftype, u_int8_t *vrid, u_int32_t *ifindex,
                                      int len, int *val);
int vrrp_set_notify(int apn_vrid, int val);
int vrrp_set_oper_priority (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                            int val);
int vrrp_set_oper_primary_ipaddr (int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex, u_int8_t *ipaddr);
int vrrp_set_oper_adv_interval(int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                int val);
int vrrp_set_oper_preempt_mode(int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                int val);
int vrrp_set_oper_accept_mode(int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                               int val);
int vrrp_set_oper_storage_type(int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                                int val);
int vrrp_set_asso_storage_type(int apn_vrid, u_int8_t aftype, u_int8_t vrid,  u_int32_t ifindex, u_int8_t *ipaddr,
                               int val);
int vrrp_set_oper_rowstatus(int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex,
                            int val);

int vrrp_set_asso_ipaddr_rowstatus(int apn_vrid, u_int8_t aftype, u_int8_t vrid, u_int32_t ifindex, u_int8_t *ipaddr,
                                   int val);
void vrrp_trap_new_master (VRRP_SESSION *sess, int reason);
void vrrp_trap_proto_error (VRRP_SESSION *sess, int error);

#endif /* HAVE_SNMP */
#endif /* _PACOS_VRRP_API_H */


