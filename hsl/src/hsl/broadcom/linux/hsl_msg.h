/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_MSG_H_
#define _HSL_MSG_H_

#define HSL_MSG_PROCESS_RETURN(SOCK,HDR,RET)                                                       \
       do {                                                                                        \
            if ((HDR)->nlmsg_flags & HAL_NLM_F_ACK)                                                \
              {                                                                                    \
                if ((RET) < 0)                                                                     \
                 hsl_sock_post_ack ((SOCK), (HDR), 0, -1);                                         \
                else                                                                               \
                 hsl_sock_post_ack ((sock), (HDR), 0, 0);                                          \
              }                                                                                    \
       } while (0)

  
#define HSL_MSG_PROCESS_RETURN_RET(SOCK,HDR,RET)                                                   \
       do {                                                                                        \
            if ((HDR)->nlmsg_flags & HAL_NLM_F_ACK)                                                \
              {                                                                                    \
                hsl_sock_post_ack ((SOCK), (HDR), 0, RET);                                         \
              }                                                                                    \
       } while (0)
 

/* 
   Function prototypes.
*/
int hsl_msg_recv_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_getlink (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_get_metric (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_get_mtu (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_set_mtu (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_get_hwaddr (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_set_hwaddr (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_flags_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_flags_set (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_flags_unset (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_get_duplex (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_set_duplex (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_set_autonego (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_get_bw (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_set_bw (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_get_counters (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_delete_done (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_clear_counters (struct socket *sock, 
                                     struct hal_nlmsghdr *hdr, char *msgbuf);

#ifdef HAVE_L3
int hsl_msg_recv_if_get_arp_ageing_timeout (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_set_arp_ageing_timeout (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_set_port_type (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_create_svi (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_delete_svi (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_ipv4_newaddr (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_ipv4_deladdr (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_fib_create (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_fib_delete (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_uc_add (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_uc_delete (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_uc_update (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_ifnewaddr(struct socket *sock, void *param1, void *param2);
int hsl_msg_ifdeladdr(struct socket *sock, void *param1, void *param2);
int hsl_msg_recv_if_set_sec_hwaddrs(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_add_sec_hwaddrs(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_delete_sec_hwaddrs(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_get_max_multipath(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

#ifdef HAVE_MCAST_IPV4
int hsl_msg_recv_ipv4_mc_init(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_deinit(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_pim_init(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_pim_deinit(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_vif_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_vif_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_route_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_route_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv4_mc_stat_get(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
int hsl_msg_recv_ipv6_mc_init(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_deinit(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_pim_init(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_pim_deinit(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_vif_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_vif_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_route_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_route_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_mc_stat_get(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_IPV6
int hsl_msg_recv_if_ipv6_newaddr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_ipv6_deladdr (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_uc_init (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_uc_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_uc_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_uc_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_uc_update (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_IPV6 */
int hsl_msg_recv_if_bind_fib (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_if_unbind_fib (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_L3 */

#ifdef HAVE_L2
int hsl_msg_recv_if_init_l2 (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_add (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_delete (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_change_vlan_type (struct socket *,
                                          struct hal_nlmsghdr *hdr,
                                          char *msgbuf);
int hsl_msg_recv_set_ageing_time (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_set_learning (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_add_port (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_delete_port (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_add_instance (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_delete_instance (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_add_vlan_to_instance (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bridge_delete_vlan_from_instance (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#if defined (HAVE_STPD) || defined (HAVE_RSTPD) || defined (HAVE_MSTPD)
int hsl_msg_recv_set_port_state(struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif  /* defined (HAVE_STPD) || defined (HAVE_RSTPD) || defined (HAVE_MSTPD) */
int hsl_msg_recv_vlan_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_add (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_delete (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_set_port_type (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_set_default_pvid (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_add_vid_to_port (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_delete_vid_from_port (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_classifier_add (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_classifier_delete (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#ifdef HAVE_VLAN_STACK
int hsl_msg_recv_vlan_stacking_enable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_stacking_disable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_VLAN_STACK */

#ifdef HAVE_PROVIDER_BRIDGE
int hsl_msg_recv_cvlan_reg_tab_add (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_cvlan_reg_tab_del (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_protocol_process (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_PROVIDER_BRIDGE */

#ifdef HAVE_DCB
s_int32_t
hsl_msg_recv_dcb_init (struct socket *, struct hal_nlmsghdr *hdr, 
                       char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_deinit (struct socket *, struct hal_nlmsghdr *hdr,
                         char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_enable (struct socket *, struct hal_nlmsghdr *hdr,
                         char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_disable (struct socket *, struct hal_nlmsghdr *hdr,
                          char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_enable (struct socket *, struct hal_nlmsghdr *hdr,
                             char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_disable (struct socket *, struct hal_nlmsghdr *hdr,
                               char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_if_enable (struct socket *, struct hal_nlmsghdr *hdr,
                            char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_if_disable (struct socket *, struct hal_nlmsghdr *hdr,
                            char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_if_enable (struct socket *, struct hal_nlmsghdr *hdr,
                                char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_if_disable (struct socket *, struct hal_nlmsghdr *hdr,
                                 char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_select_mode (struct socket *, struct hal_nlmsghdr *hdr,
                                  char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_add_pri_to_tcg (struct socket *, struct hal_nlmsghdr *hdr,
                                     char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_remove_pri_from_tcg (struct socket *, 
                                          struct hal_nlmsghdr *hdr,
                                          char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_tcg_bw_set (struct socket *, struct hal_nlmsghdr *hdr,
                                 char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_app_pri_set (struct socket *, struct hal_nlmsghdr *hdr,
                                  char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_ets_app_pri_unset (struct socket *, struct hal_nlmsghdr *hdr,
                                    char *msgbuf);

s_int32_t
hsl_msg_recv_dcb_pfc_enable (struct socket *sock, struct hal_nlmsghdr *hdr,
                                    char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_pfc_disable (struct socket *sock, struct hal_nlmsghdr *hdr,
                                    char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_pfc_if_enable (struct socket *sock, struct hal_nlmsghdr *hdr,
                                    char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_pfc_select_mode (struct socket *sock, struct hal_nlmsghdr *hdr,
                                    char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_pfc_set_cap (struct socket *sock, struct hal_nlmsghdr *hdr,
                                    char *msgbuf);
s_int32_t
hsl_msg_recv_dcb_pfc_set_lda (struct socket *sock, struct hal_nlmsghdr *hdr,
                                    char *msgbuf);

#endif /* HAVE_DCB */

int hsl_msg_recv_flow_control_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_flow_control_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_flow_control_set (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_flow_control_statistics (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_qos_init (struct socket *, struct hal_nlmsghdr * hdr, char *msgbuf);
int hsl_msg_recv_l2_qos_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#ifdef HAVE_QOS
int hsl_msg_recv_l2_qos_default_user_priority_set (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_QOS */
int hsl_msg_recv_l2_qos_default_user_priority_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_qos_regen_user_priority_set (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_qos_regen_user_priority_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_qos_traffic_class_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int hsl_msg_recv_ratelimit_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ratelimit_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ratelimit_bcast (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_bcast_discards_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ratelimit_mcast (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_mcast_discards_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_recv_msg_igmp_snooping_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int
hsl_recv_msg_igmp_snooping_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_igmp_snooping_enable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_igmp_snooping_disable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_igmp_snooping_add_entry(struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_igmp_snooping_del_entry(struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_recv_msg_mld_snooping_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int
hsl_recv_msg_mld_snooping_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_mld_snooping_enable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_mld_snooping_disable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_mld_snooping_add_entry(struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_mld_snooping_del_entry(struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
int
hsl_msg_l2_unknown_mcast_discard_mode (struct socket *,struct hal_nlmsghdr * ,
                                       char *);
int
hsl_msg_l2_unknown_mcast_flood_mode (struct socket * ,struct hal_nlmsghdr * ,
                                     char *);
#endif

int hsl_msg_recv_l2_fdb_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_fdb_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_fdb_add (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_fdb_delete (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_fdb_unicast_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_fdb_multicast_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_fdb_flush (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_fdb_flush_by_mac (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_pmirror_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_pmirror_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_pmirror_set (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_pmirror_unset (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#ifdef HAVE_LACPD
int hsl_msg_recv_lacp_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_add_aggregator (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_delete_aggregator (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_attach_mux_to_aggregator (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_detach_mux_from_aggregator (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_psc_set (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_collecting (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_distributing (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_lacp_collecting_distributing (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_LACPD */
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD

int hsl_msg_recv_set_frame_period_window(struct socket *sock,
                                         struct hal_nlmsghdr *hdr,
                                         char *msgbuf);

int hsl_msg_recv_set_symbol_period_window(struct socket *sock,
                                          struct hal_nlmsghdr *hdr,
                                          char *msgbuf);

int hsl_msg_recv_get_frame_error_count(struct socket * sock,
                                       struct hal_nlmsghdr *hdr,
                                       char *msgbuf);

int hsl_msg_recv_get_frame_error_seconds_count(struct socket *sock,
                                               struct hal_nlmsghdr *hdr,
                                               char *msgbuf);

int hsl_msg_recv_efm_set_port_state(struct socket *sock,
                                    struct hal_nlmsghdr *hdr,
                                    char *msgbuf);

int hsl_msg_if_fpwindow_expiry(struct socket *sock, void *param1, void *param2);

#endif /*HAVE_ONMD*/

#ifdef HAVE_AUTHD
int hsl_msg_recv_auth_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_auth_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_auth_set_port_state (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#ifdef HAVE_MAC_AUTH
int hsl_msg_recv_auth_mac_set_port_state(struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif
#endif /* HAVE_AUTHD */

#ifdef HAVE_L3
int hsl_msg_recv_arp_add (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_arp_del (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_arp_cache_get (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_arp_del_all(struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);

#ifdef HAVE_IPV6
int hsl_msg_recv_ipv6_nbr_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_nbr_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_nbr_del_all (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ipv6_nbr_cache_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_IPV6 */
#endif /* HAVE_L3 */

#ifdef HAVE_L2LERN
int hsl_msg_recv_mac_access_grp_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_vlan_access_map_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif

#ifdef HAVE_QOS
int hsl_msg_recv_qos_init (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_deinit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_enable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_disable (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_queue_limit (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_tail_drop_threshold (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_threshold_dscp_map (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_wred_drop_threshold (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_set_bandwidth (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_queue_cos_map_set (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_queue_cos_map_unset (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_wrr_queue_min_reserve (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_set_trust_state (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_set_default_cos (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_set_dscp_mapping_tbl (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_set_class_map (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_qos_set_cmap_cos_inner (struct socket *, struct hal_nlmsghdr *hdr, char
*msgbuf);
int hsl_msg_recv_qos_set_policy_map (struct socket *, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_qos_num_cosq_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_l2_qos_num_cosq_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_QOS */

int hsl_msg_recv_if_init_l3(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_ratelimit_dlf_bcast (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_dlf_bcast_discards_get (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_ifnew (struct socket *sock, void *param1, void *unused);
int hsl_msg_ifdelete (struct socket *sock, void *param1, void *param2);
int hsl_msg_ifflags (struct socket *sock, void *param1, void *param2);

int hsl_msg_ifautonego(struct socket *sock, void *param1, void *unused);
int hsl_msg_ifhwaddr(struct socket *sock, void *param1, void *unused);
int hsl_msg_ifmtu(struct socket *sock, void *param1, void *unused);
int hsl_msg_ifduplex(struct socket *sock, void *param1, void *unused);
//int hsl_msg_ifdelete (struct socket *sock, void *param1, void *param2);
#ifdef HAVE_MPLS
int hsl_msg_recv_mpls_init (struct socket *sock, struct hal_nlmsghdr *hdr,
                            char *msgbuf);
int hsl_msg_recv_mpls_if_init (struct socket *sock, struct hal_nlmsghdr *hdr,
                               char *msgbuf);
int hsl_msg_recv_mpls_if_deinit (struct socket *sock, struct hal_nlmsghdr *hdr,
                                 char *msgbuf);
int hsl_msg_recv_mpls_vrf_init (struct socket *sock, struct hal_nlmsghdr *hdr,\
                                char *msgbuf);
int
hsl_msg_recv_mpls_vrf_deinit (struct socket *sock, struct hal_nlmsghdr *hdr, 
                              char *msgbuf);

int hsl_msg_recv_mpls_ilm_add (struct socket *sock, struct hal_nlmsghdr *hdr,
                               char *msgbuf);
int hsl_msg_recv_mpls_ilm_del (struct socket *sock, struct hal_nlmsghdr *hdr, 
                               char *msgbuf);
int hsl_msg_recv_mpls_ftn_add (struct socket *sock, struct hal_nlmsghdr *hdr,
                               char *msgbuf);
int hsl_msg_recv_mpls_ftn_del (struct socket *sock, struct hal_nlmsghdr *hdr,
                               char *msgbuf);
int hsl_msg_recv_mpls_vc_init (struct socket *sock, struct hal_nlmsghdr *hdr, 
                               char *msgbuf);
int hsl_msg_recv_mpls_vc_deinit (struct socket *sock, struct hal_nlmsghdr *hdr,                                  char *msgbuf);
int hsl_msg_recv_mpls_vc_fib_add (struct socket *sock, struct hal_nlmsghdr *hdr,
            char *msgbuf);
int hsl_msg_recv_mpls_vc_fib_del (struct socket *sock, struct hal_nlmsghdr *hdr,                                  char *msgbuf);

#ifdef HAVE_VPLS
int hsl_msg_recv_mpls_vpls_add (struct socket *sock, struct hal_nlmsghdr *hdr,
                                char *msgbuf);
int hsl_msg_recv_mpls_vpls_del (struct socket *sock, struct hal_nlmsghdr *hdr,
                                char *msgbuf);
int hsl_msg_recv_mpls_vpls_if_bind (struct socket *sock, 
                                    struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_mpls_vpls_if_unbind (struct socket *sock, 
                                    struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_mpls_vpls_fib_add (struct socket *sock, 
                                    struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_recv_mpls_vpls_fib_del (struct socket *sock, 
                                    struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS */

#ifdef HAVE_PVLAN
int
hsl_msg_recv_pvlan_set_vlan_type (struct socket *sock,
                                  struct hal_nlmsghdr *hdr,
                                  char *msgbuf);

int
hsl_msg_recv_pvlan_vlan_associate (struct socket *sock,
                                   struct hal_nlmsghdr *hdr,
                                   char *msgbuf);

int
hsl_msg_recv_pvlan_vlan_dissociate (struct socket *sock,
                                    struct hal_nlmsghdr *hdr,
                                    char *msgbuf);

int
hsl_msg_recv_pvlan_port_add (struct socket *sock,
                             struct hal_nlmsghdr *hdr,
                             char *msgbuf);

int
hsl_msg_recv_pvlan_port_delete (struct socket *sock,
                                struct hal_nlmsghdr *hdr,
                                char *msgbuf);

int
hsl_msg_recv_pvlan_set_port_mode (struct socket *sock,
                                  struct hal_nlmsghdr *hdr,
                                  char *msgbuf);

#endif /* HAVE_PVLAN */


int
hsl_msg_recv_ip_set_acl_filter(struct socket *sock,
                               struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ip_unset_acl_filter(struct socket *sock,
                                 struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ip_set_acl_filter_interface(struct socket *sock,
                                         struct hal_nlmsghdr *hdr,
                                         char *msgbuf);

int
hsl_msg_recv_ip_unset_acl_filter_interface(struct socket *sock,
                                           struct hal_nlmsghdr *hdr,
                                           char *msgbuf);

/* CPU and system topology related information */
int
hsl_msg_recv_cpu_get_num (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_cpu_getdb_info (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_cpu_get_master (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_cpu_get_local (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_cpu_set_master (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_cpu_get_info_index (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_cpu_get_dump_index (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
/* djg */
int
hsl_msg_recv_ptp_set_clock_type (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int 
hsl_msg_recv_ptp_create_domain(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_domain_add_mport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_domain_add_sport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_set_delay_mech (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_set_phyport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_clear_phyport (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_enable_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_disable_port (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_port_delay_mech (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_set_req_interval (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_set_p2p_interval (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int		  
hsl_msg_recv_ptp_set_p2p_meandelay (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int
hsl_msg_recv_ptp_set_asym_delay (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

#ifdef HAVE_TUNNEL
int
hsl_msg_recv_tunnel_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int
hsl_msg_recv_tunnel_delete (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int
hsl_msg_recv_tunnel_initiator_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int
hsl_msg_recv_tunnel_initiator_clear (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int
hsl_msg_recv_tunnel_terminator_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int
hsl_msg_recv_tunnel_terminator_clear (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
#endif /* HAVE_TUNNEL */

//#ifdef HAVE_VPWS   /*cai 2011-10 vpws module*/
int hsl_msg_vpws_pe_create  (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpws_p_create  (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpws_delete  (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
//#endif /* HAVE_VPWS*/


int hsl_msg_vpls_vpn_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpls_vpn_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpls_port_add (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpls_port_del (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);



int hsl_msg_vpn_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpn_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpn_port_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpn_port_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_lsp_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_lsp_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_get_lsp_status(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_lsp_protect_group_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_lsp_protect_group_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_lsp_protect_group_mod_param(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_lsp_protect_group_switch(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_lsp_get_protect_grp_info(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);


int hsl_msg_vpn_multicast_group_create(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpn_multicast_group_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vpn_multicast_group_port_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int  hsl_msg_vpn_multicast_group_port_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int  hsl_msg_vpn_static_mac_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int  hsl_msg_vpn_static_mac_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int hsl_msg_oam_add(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_oam_del(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_oam_get_status(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int hsl_msg_port_rate_limit(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_port_shapping(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_ac_car_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf); 
int hsl_msg_mpls_car_set (struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);

int hsl_msg_port_pvid_set(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vlan_new_create(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vlan_new_delete(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vlan_new_rmv_vid_from_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vlan_new_add_vid_to_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vlan_new_add_mcast_to_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);
int hsl_msg_vlan_new_rmv_mcast_to_port(struct socket *sock, struct hal_nlmsghdr *hdr, char *msgbuf);




#endif /* _HSL_MSG_H_ */
