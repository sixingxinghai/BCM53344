/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _NSM_L2_MCAST_H
#define _NSM_L2_MCAST_H

#ifdef HAVE_IGMP_SNOOP
#include "igmp.h"
#endif /* HAVE_IGMP_SNOOP */
#ifdef HAVE_MLD_SNOOP
#include "mld.h"
#endif /* HAVE_MLD_SNOOP */

/* The Layer 2 Buffer size is equal to the MTU*/

#define NSM_L2_MCAST_BUFFER_SIZE            2048

#define L2MZG(m)   (m)->master->nm->zg

#define NSM_L2_MCAST_VLAN_SHIFT (12)
#define NSM_L2_MCAST_BRIDGE_SHIFT (24)
#define NSM_L2_MCAST_VLAN_MASK  (0xFFF)
#define NSM_L2_MCAST_BRIDGE_MASK  (0xFF)

#define NSM_L2_MCAST_MAKE_KEY(BRID, VID1, VID2)     \
        ((BRID << NSM_L2_MCAST_BRIDGE_SHIFT ) | \
         (VID1 << NSM_L2_MCAST_VLAN_SHIFT) | VID2)

#define NSM_L2_MCAST_GET_BRID(SUID)          \
                 ((SUID >> NSM_L2_MCAST_BRIDGE_SHIFT) &  \
                   NSM_L2_MCAST_BRIDGE_MASK)

#define NSM_L2_MCAST_GET_CVID(SUID)          \
                 ((SUID >> NSM_L2_MCAST_VLAN_SHIFT) & NSM_L2_MCAST_VLAN_MASK)

#define NSM_L2_MCAST_GET_SVID(SUID)          \
                 ((SUID & NSM_L2_MCAST_VLAN_MASK))

#define NSM_L2_MCAST_GET_VID(SUID)           \
                 ((SUID & NSM_L2_MCAST_VLAN_MASK))

struct nsm_l2_mcast
{
  /* NSM Bridge Master */
  struct nsm_bridge_master *master;

  /* Packet Input/Output buffer */
  struct stream *iobuf;

  /* Packet Output buffer */
  struct stream *obuf;

  /* IGMP Instance */
  struct igmp_instance *igmp_inst;

  /* IGMP L2 Service Registration ID */
  u_int32_t igmp_svc_reg_id;

  /* MLD Instance */
  struct mld_instance *mld_inst;

  /* MLD L2 Service Registration ID */
  u_int32_t mld_svc_reg_id;

  /* Bridge Listener of the default bridge */
  struct nsm_bridge_listener *nsm_l2mcast_br_appln;

  /* Vlan Listener of the default bridge */
  struct nsm_vlan_listener *nsm_l2_mcast_vlan_appln;

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP
enum 
  {
    NSM_L2_UNKNOWN_MCAST_FLOOD = 0,
    NSM_L2_UNKNOWN_MCAST_DISCARD = 1,
  }nsm_l2_unknown_mcast;
#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */
};

enum nsm_l2mcast_filter_mode_state
{
  NSM_L2_MCAST_FMS_INVALID=0,
  NSM_L2_MCAST_FMS_INCLUDE,
  NSM_L2_MCAST_FMS_EXCLUDE,
  NSM_L2_MCAST_FMS_MAX
};

struct nsm_l2_mcast_grec
{
  struct nsm_if *owning_if;

  enum nsm_l2mcast_filter_mode_state filt_mode;

  struct ptree *nsm_l2_mcast_src_tib;

  struct ptree_node *owning_pn;

  u_int32_t num_srcs;

  u_int8_t family;
};

struct nsm_l2_mcast_source_rec
{
  struct nsm_l2_mcast_grec *owning_ngr;

  struct ptree_node *owning_pn;

  u_int16_t flags;
#define NSM_ISR_SFLAG_VALID            (1 << 0)

};

struct nsm_l2_mcast_grec_key
{
  u_int32_t su_id;
  union
  {
    struct pal_in4_addr grp_addr;
#ifdef HAVE_MLD_SNOOP
    struct pal_in6_addr grp6_addr;
#endif /* HAVE_MLD_SNOOP */
  } addr;
};

#ifdef HAVE_IGMP_SNOOP
#define NSM_L2_MCAST_IPV4_PREFIXLEN (IPV4_MAX_PREFIXLEN +        \
                                     (sizeof  (u_int32_t) * 8))
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
#define NSM_L2_MCAST_IPV6_PREFIXLEN (IPV6_MAX_PREFIXLEN +        \
                                     (sizeof  (u_int32_t) * 8))
#endif /* HAVE_MLD_SNOOP */


typedef s_int32_t (* nsm_l2_mcast_action_func_t)
                          (char *bridgename,
                          u_int8_t family,
                          void_t *pgrp,
                          u_int32_t su_id,
                          struct interface *ifp,
                          u_int16_t num_srcs,
                          void_t *src_addr_list,
                          enum nsm_l2mcast_filter_mode_state filt_mode,
                          struct nsm_l2_mcast_grec *igr);


void 
nsm_l2_mcast_if_add(struct nsm_if **zif,struct interface *ifp);

void
nsm_l2_mcast_if_delete(struct nsm_if **zif);

s_int32_t
nsm_l2_mcast_action_include (char *bridgename,
                             u_int8_t family,
                             void_t *pgrp,
                             u_int32_t su_id,
                             struct interface *ifp,
                             u_int16_t num_srcs,
                             void_t *src_addr_list,
                             enum nsm_l2mcast_filter_mode_state filt_mode,
                             struct nsm_l2_mcast_grec *igr);


s_int32_t
nsm_l2_mcast_action_exclude (char *bridgename,
                             u_int8_t family,
                             void_t *pgrp,
                             u_int32_t su_id,
                             struct interface *ifp,
                             u_int16_t num_srcs,
                             void_t *src_addr_list,
                             enum nsm_l2mcast_filter_mode_state filt_mode,
                             struct nsm_l2_mcast_grec *igr);

s_int32_t
nsm_l2_mcast_action_invalid (char *bridgename,
                             u_int8_t family,
                             void_t *pgrp,
                             u_int32_t su_id,
                             struct interface *ifp,
                             u_int16_t num_srcs,
                             void_t *src_addr_list,
                             enum nsm_l2mcast_filter_mode_state filt_mode,
                             struct nsm_l2_mcast_grec *igr);


s_int32_t
nsm_l2_mcast_delete_port (struct nsm_bridge_master *master,
                          char *bridge_name,
                          struct interface *ifp);
s_int32_t
nsm_l2_mcast_enable_port (struct nsm_bridge_master *master,
                          struct interface *ifp);
s_int32_t
nsm_l2_mcast_disable_port (struct nsm_bridge_master *master,
                           struct interface *ifp);

s_int32_t
nsm_l2_mcast_add_port_cb (struct nsm_bridge_master *master,
                          char *bridge_name,
                          struct interface *ifp);
s_int32_t
nsm_l2_mcast_delete_port_cb (struct nsm_bridge_master *master,
                             char *bridge_name,
                             struct interface *ifp);
s_int32_t
nsm_l2_mcast_enable_port_cb (struct nsm_bridge_master *master,
                             struct interface *ifp);
s_int32_t
nsm_l2_mcast_disable_port_cb (struct nsm_bridge_master *master,
                              struct interface *ifp);

s_int32_t
nsm_l2_mcast_send_query_for_bridge (struct nsm_bridge_master *master,
                                    char *bridge_name);
s_int32_t
nsm_l2_mcast_add_vlan (struct nsm_bridge_master *master,
                       char *bridgename,
                       u_int16_t vlan_id,
                       u_int16_t svlan_id);
s_int32_t
nsm_l2_mcast_del_vlan (struct nsm_bridge_master *master,
                       char *bridgename,
                       u_int16_t vid,
                       u_int16_t svid);
s_int32_t
nsm_l2_mcast_add_vlan_port (struct nsm_bridge_master *master,
                            char *bridgename,
                            struct interface *ifp,
                            u_int16_t vid,
                            u_int16_t svid);
s_int32_t
nsm_l2_mcast_del_vlan_port (struct nsm_bridge_master *master,
                            char *bridgename,
                            struct interface *ifp,
                            u_int16_t vid,
                            u_int16_t svid);

#ifdef HAVE_PROVIDER_BRIDGE

s_int32_t
nsm_l2_mcast_add_swctx_cb (struct nsm_bridge_master *master,
                           char *bridgename,
                           u_int16_t cvlan_id,
                           u_int16_t svlan_id);
s_int32_t
nsm_l2_mcast_del_swctx_cb (struct nsm_bridge_master *master,
                           char *bridgename,
                           u_int16_t cvid,
                           u_int16_t svid);
s_int32_t
nsm_l2_mcast_add_swctx_port_cb (struct nsm_bridge_master *master,
                                char *bridgename,
                                struct interface *ifp,
                                u_int16_t cvid,
                                u_int16_t svid);
s_int32_t
nsm_l2_mcast_del_swctx_port_cb (struct nsm_bridge_master *master,
                                char *bridgename,
                                struct interface *ifp,
                                u_int16_t cvid,
                                u_int16_t svid);

s_int32_t
nsm_l2_mcast_delete_pro_edge_port (struct nsm_bridge_master *master,
                                   char *bridge_name,
                                   struct interface *ifp);

s_int32_t
nsm_l2_mcast_enable_pro_edge_port (struct nsm_bridge_master *master,
                                   struct interface *ifp);

s_int32_t
nsm_l2_mcast_disable_pro_edge_port (struct nsm_bridge_master *master,
                                    struct interface *ifp);

#endif /* HAVE_PROVIDER_BRIDGE */

s_int32_t
nsm_l2_mcast_add_vlan_cb (struct nsm_bridge_master *master,
                          char *bridgename,
                          u_int16_t vlan_id);

s_int32_t
nsm_l2_mcast_del_vlan_cb (struct nsm_bridge_master *master,
                          char *bridgename,
                          u_int16_t vid);
s_int32_t
nsm_l2_mcast_add_vlan_port_cb (struct nsm_bridge_master *master,
                               char *bridgename,
                               struct interface *ifp,
                               u_int16_t vid);
s_int32_t
nsm_l2_mcast_del_vlan_port_cb (struct nsm_bridge_master *master,
                               char *bridgename,
                               struct interface *ifp,
                               u_int16_t vid);

s_int32_t
nsm_l2_mcast_delete_bridge (struct nsm_bridge_master *master,
                            char *bridge_name);

s_int32_t
nsm_l2_mcast_init (struct nsm_master *nm);

void
nsm_l2_mcast_if_add (struct nsm_if **zif, struct interface *ifp);

void
nsm_l2_mcast_if_delete (struct nsm_if **zif);

s_int32_t
nsm_l2_mcast_add_bridge (struct nsm_bridge *bridge,
                         struct nsm_bridge_master *master);
#ifdef HAVE_VLAN
s_int32_t
nsm_l2_mcast_vlan_init (struct nsm_bridge *bridge,
                        struct nsm_bridge_master *master);
s_int32_t
nsm_l2_mcast_vlan_deinit (struct nsm_bridge_master *master,
                          char *name);
#endif /* HAVE_VLAN */

void_t
nsm_l2_mcast_deinit (struct nsm_master *nm);

s_int32_t
nsm_l2_mcast_if_su_id_get (u_int32_t su_id,
                           struct interface *ifp,
                           u_int16_t sec_key,
                           u_int32_t *if_su_id);

#ifdef HAVE_IGMP_SNOOP
s_int32_t
nsm_l2_mcast_igmp_lmem (u_int32_t su_id,
                        struct interface *ifp,
                        u_int32_t vid,
                        enum igmp_filter_mode_state filt_mode,
                        struct pal_in4_addr *grp_addr,
                        u_int16_t num_srcs,
                        struct pal_in4_addr src_addr_list []);
s_int32_t
nsm_l2_mcast_igmp_mrt_if_update (u_int32_t su_id,
                                 struct interface *ifp,
                                 u_int32_t key,
                                 bool_t is_exclude);
void_t *
nsm_l2_mcast_igmp_get_instance (struct apn_vrf *ivrf);
s_int32_t
nsm_l2_mcast_igmp_init (struct nsm_l2_mcast *l2mcast);
s_int32_t
nsm_l2_mcast_igmp_uninit (struct nsm_l2_mcast *l2mcast);
s_int32_t
nsm_l2_mcast_igmp_svc_reg (struct nsm_l2_mcast *l2mcast);
s_int32_t
nsm_l2_mcast_igmp_svc_dereg (struct nsm_l2_mcast *l2mcast);
s_int32_t

nsm_l2_mcast_igmp_if_create (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             u_int32_t su_id);
s_int32_t
nsm_l2_mcast_igmp_if_delete (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             u_int32_t su_id);
s_int32_t
nsm_l2_mcast_igmp_if_enable (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             struct interface *p_ifp,
                             u_int32_t su_id);
s_int32_t
nsm_l2_mcast_igmp_if_disable (struct nsm_l2_mcast *l2mcast,
                              struct interface *ifp,
                              struct interface *p_ifp,
                              u_int32_t su_id);
s_int32_t
nsm_l2_mcast_igmp_if_update (struct nsm_l2_mcast *mcast,
                             struct interface *ifp,
                             u_int32_t su_id);

#endif /* HAVE_IGMP_SNOOP */

#if defined HAVE_IGMP_SNOOP || defined HAVE_MLD_SNOOP

s_int32_t
nsm_l2_mcast_unknown_mcast_mode (int);

#endif /* HAVE_IGMP_SNOOP || HAVE_MLD_SNOOP */

#ifdef HAVE_MLD_SNOOP
s_int32_t
nsm_l2_mcast_mld_lmem (u_int32_t su_id,
                       struct interface *ifp,
                       u_int32_t vid,
                       enum mld_filter_mode_state filt_mode,
                       struct pal_in6_addr *grp_addr,
                       u_int16_t num_srcs,
                       struct pal_in6_addr src_addr_list []);
void_t *
nsm_l2_mcast_mld_get_instance (struct apn_vrf *ivrf);
s_int32_t
nsm_l2_mcast_mld_init (struct nsm_l2_mcast *l2mcast);
s_int32_t
nsm_l2_mcast_mld_uninit (struct nsm_l2_mcast *l2mcast);
s_int32_t
nsm_l2_mcast_mld_svc_reg (struct nsm_l2_mcast *l2mcast);
s_int32_t
nsm_l2_mcast_mld_svc_dereg (struct nsm_l2_mcast *l2mcast);

s_int32_t
nsm_l2_mcast_mld_if_create (struct nsm_l2_mcast *l2mcast,
                            struct interface *ifp,
                            u_int32_t su_id);
s_int32_t
nsm_l2_mcast_mld_if_delete (struct nsm_l2_mcast *l2mcast,
                            struct interface *ifp,
                            u_int32_t su_id);
s_int32_t
nsm_l2_mcast_mld_if_enable (struct nsm_l2_mcast *l2mcast,
                            struct interface *ifp,
                            struct interface *p_ifp,
                            u_int32_t su_id);
s_int32_t
nsm_l2_mcast_mld_if_disable (struct nsm_l2_mcast *l2mcast,
                             struct interface *ifp,
                             struct interface *p_ifp,
                             u_int32_t su_id);
s_int32_t
nsm_l2_mcast_mld_if_update (struct nsm_l2_mcast *mcast,
                            struct interface *ifp,
                            u_int32_t su_id);
#endif /* HAVE_MLD_SNOOP */

#endif /* _NSM_L2_MCAST_H */
