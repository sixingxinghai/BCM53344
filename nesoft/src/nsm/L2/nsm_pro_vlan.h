/*   Copyright (C) 2003  All Rights Reserved.  
  
   This module declares the interface to the Layer 2 Provider Vlan
  
*/


#ifndef __L2_PRO_VLAN_H_
#define __L2_PRO_VLAN_H_

#define NSM_PEP_SVID_MSG_NUM                         2
struct nsm_cvlan_reg_key
{
  u_int16_t cvid;
  u_int16_t svid;
};

struct nsm_vlan_trans_key
{
  u_int16_t vid;
  u_int16_t trans_vid;
};

struct nsm_cvlan_trans_key
{
  u_int16_t svid;
  u_int16_t cvid;
  u_int16_t trans_cvid;
};

struct nsm_cvlan_reg_tab
{
#define NSM_CVLAN_REG_TAB_NAME_MAX 16
  struct list  *port_list;
  struct avl_tree *reg_tab;
  struct nsm_bridge *bridge;
  struct avl_tree *svlan_tree;
  char name [NSM_CVLAN_REG_TAB_NAME_MAX + 1];
};

struct nsm_pro_edge_sw_ctx
{
  u_int16_t svid;
  u_int16_t cvid;
  u_int16_t refcnt;
  struct interface *ifp;
  struct list *port_list;
  struct ptree *static_fdb_list;
};

struct nsm_svlan_switch_ctx
{
  /* Type of switching context */
  u_int8_t type;
#define NSM_SVLAN_PORT_SW_CTX      (1 << 0)
#define NSM_SVLAN_LCL_SW_CTX       (1 << 1)

  /* Switching context identifier */
  u_int32_t scidx;

  /* Port from which the scidx is derived */
  struct interface *ifp;

  /* List of ports in the switching context */
  struct list *port_list;

};

struct nsm_pro_edge_info
{
  u_int16_t untagged_vid;
  struct interface *ifp;
  u_int16_t pvid;
};

struct nsm_svlan_reg_info
{
  struct nsm_vlan_bmp cvlanMemberBmp;
  u_int8_t preserve_ce_cos;
  u_int16_t num_of_cvlans;
  struct list *port_list;
  u_int16_t svid;
};

void
nsm_pro_edge_add_port_to_swctx (struct nsm_bridge *bridge,
                                struct interface *ifp,
                                struct nsm_vlan *svlan);

void
nsm_pro_edge_del_port_from_swctx (struct nsm_bridge *bridge,
                                  struct interface *ifp,
                                  struct nsm_vlan *svlan);

struct nsm_pro_edge_sw_ctx *
nsm_pro_edge_swctx_lookup (struct nsm_bridge *bridge, u_int16_t cvid,
                           u_int16_t svid);

s_int32_t
nsm_svlan_create_sw_ctx (struct nsm_bridge *bridge,
                         struct nsm_vlan *svlan, u_int8_t type,
                         struct interface *ifp);

void
nsm_svlan_delete_port_sw_ctx (u_int16_t svid, struct interface *ifp);

s_int32_t
nsm_svlan_add_port_to_lcl_ctx (struct nsm_vlan *svlan, struct interface *ifp);

s_int32_t
nsm_svlan_port_enable_local_switching (u_int16_t svid, struct interface *ifp);

s_int32_t
nsm_svlan_port_disable_local_switching (u_int16_t svid, struct interface *ifp);

s_int32_t
nsm_svlan_add_port_to_all_sw_ctx (struct nsm_vlan *svlan, struct interface *ifp);

s_int32_t
nsm_svlan_delete_port_from_all_sw_ctx (struct nsm_vlan *svlan,
                                       struct interface *ifp);

s_int32_t
nsm_cvlan_reg_tab_if_apply (struct interface *ifp, char *cvlan_reg_tab_name);

s_int32_t
nsm_cvlan_reg_tab_if_delete (struct interface *ifp);

struct nsm_cvlan_reg_tab *
nsm_cvlan_reg_tab_get (struct nsm_bridge *bridge, char *cvlan_reg_tab_name);

void
nsm_cvlan_reg_tab_delete (struct nsm_bridge *bridge, char *cvlan_reg_tab_name);

s_int32_t
nsm_cvlan_reg_tab_check_membership (struct nsm_cvlan_reg_tab *regtab,
                                    u_int16_t cvid);

s_int32_t
nsm_cvlan_reg_tab_entry_add (struct nsm_cvlan_reg_tab *regtab, u_int16_t cvid,
                             u_int16_t svid);

s_int32_t
nsm_cvlan_reg_tab_entry_delete (struct nsm_cvlan_reg_tab *regtab,
                                u_int16_t cvid);

s_int32_t
nsm_cvlan_reg_tab_entry_delete_by_svid (struct nsm_cvlan_reg_tab *regtab,
                                        u_int16_t svid);

int
nsm_vlan_trans_tab_ent_cmp (void *data1, void *data2);

int
nsm_vlan_rev_trans_tab_ent_cmp (void *data1, void *data2);

int
nsm_cvlan_trans_tab_ent_cmp (void *data1, void *data2);

int
nsm_pro_edge_sw_ctx_ent_cmp (void *data1, void *data2);


s_int32_t
nsm_vlan_trans_tab_entry_add (struct interface *ifp, u_int16_t local_svid,
                              u_int16_t relay_svid);

s_int32_t
nsm_cvlan_trans_tab_entry_add (struct interface *ifp, u_int16_t local_svid,
                               u_int16_t local_cvid, u_int16_t relay_svid);

s_int32_t
nsm_vlan_trans_tab_entry_delete (struct interface *ifp, u_int16_t vid);

s_int32_t
nsm_cvlan_trans_tab_entry_delete (struct interface *ifp, u_int16_t local_svid,
                                  u_int16_t local_cvid, u_int16_t trans_cvid);

void
nsm_vlan_trans_tab_delete (struct interface *ifp);

void
nsm_cvlan_trans_tab_delete (struct interface *ifp);

s_int32_t
nsm_vlan_cvlan_regis_exist (struct nsm_bridge *bridge,
                            struct nsm_vlan *vlan);

struct nsm_cvlan_reg_key *
nsm_vlan_port_cvlan_regis_exist (struct interface *ifp,
                                 u_int16_t cvid);

struct nsm_cvlan_reg_tab *
nsm_cvlan_reg_tab_lookup (struct nsm_bridge *bridge, char *cvlan_reg_tab_name);

enum nsm_uni_service_attr
nsm_map_str_to_service_attr (u_int8_t *proto_str);

s_int32_t
nsm_svlan_set_cos_preservation (struct nsm_bridge_master *master,
                                char *bridge_name, u_int16_t vid,
                                u_int8_t preserve_cos);

#ifdef HAVE_PROVIDER_BRIDGE
s_int32_t
nsm_bridge_uni_set_service_attr (struct interface *ifp,
                                 enum nsm_uni_service_attr service_attr);

s_int32_t
nsm_bridge_uni_unset_service_attr (struct interface *ifp,
                                   enum nsm_uni_service_attr service_attr);
#endif/* HAVE_PROVIDER_BRIDGE */
s_int32_t
nsm_bridge_uni_set_name (struct interface *ifp,
                         char *uni_name);

s_int32_t
nsm_bridge_uni_unset_name (struct interface *ifp);

s_int32_t 
nsm_uni_set_max_evc (struct interface *ifp, u_int16_t max_evc);

s_int32_t 
nsm_uni_unset_max_evc (struct interface *ifp);

s_int32_t
nsm_svlan_set_max_uni (struct nsm_bridge_master *master,
                    u_int8_t *bridge_name, u_int16_t svid, u_int8_t *evc_id,
                    u_int16_t max_uni);
s_int32_t
nsm_svlan_unset_max_uni (struct nsm_bridge_master *master,
                    u_int8_t *bridge_name, u_int16_t svid, u_int8_t *evc_id);


struct nsm_vlan*
nsm_find_svlan_by_evc_id (struct nsm_bridge *bridge, u_int8_t *evc_id);


s_int32_t
nsm_svlan_set_evc_id (struct nsm_bridge_master *master,
                          u_int8_t *bridge_name, u_int16_t svid, 
                          u_int8_t *evc_id);
s_int32_t 
nsm_svlan_unset_evc_id (struct nsm_bridge_master *master, 
                      u_int8_t *bridge_name, u_int16_t svid, u_int8_t *evc_id);


s_int32_t
nsm_bridge_uni_type_detect (u_int8_t *br_name, u_int8_t uni_type_mode,
                              struct nsm_bridge_master *master);


s_int32_t
nsm_uni_type_detect (struct interface *ifp, u_int8_t uni_type_mode);

s_int32_t
nsm_uni_bw_profiling (struct interface *ifp, u_int32_t cir, u_int32_t cbs, 
    u_int32_t eir, u_int32_t ebs, u_int8_t coupling_flag, u_int8_t color_mode,
    u_int8_t bw_profile_type, u_int8_t bw_profile_parameter);


s_int32_t
nsm_delete_uni_bw_profiling (struct interface *ifp, u_int8_t bw_profile_type);

s_int32_t
nsm_set_uni_bw_profiling (struct interface *ifp, u_int8_t bw_profile_type,
    u_int8_t bw_profile_status);

#ifdef HAVE_PROVIDER_BRIDGE
s_int32_t
nsm_uni_evc_set_service_instance (struct interface *ifp, u_int16_t instance_id,
        u_int8_t *evc_id, struct  nsm_uni_evc_bw_profile 
        **ret_nsm_uni_evc_bw_profile);

s_int32_t
nsm_unset_uni_evc_set_service_instance (struct interface *ifp,
    u_int16_t instance_id, u_int8_t *evc_id);

s_int32_t 
nsm_delete_evc_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *evc_bw_profile,  u_int8_t bw_profile_type);

s_int32_t
nsm_set_evc_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int8_t bw_profile_type,
    u_int8_t bw_profile_status);

s_int32_t
nsm_evc_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int32_t cir, u_int32_t cbs,
    u_int32_t eir, u_int32_t ebs, u_int8_t coupling_flag, u_int8_t color_mode,
    u_int8_t bw_profile_type, u_int8_t bw_profile_parameter);

s_int32_t 
nsm_delete_evc_cos_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile,  u_int8_t *cos,
    u_int8_t bw_profile_type);

s_int32_t
nsm_set_evc_cos_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int8_t *cos, 
    u_int8_t bw_profile_type, u_int8_t bw_profile_status);

s_int32_t
nsm_evc_cos_bw_profiling (struct interface *ifp, 
    struct nsm_uni_evc_bw_profile *bw_profile, u_int8_t *cos, u_int32_t cir,
    u_int32_t cbs, u_int32_t eir, u_int32_t ebs, u_int8_t coupling_flag,
    u_int8_t color_mode, u_int8_t bw_profile_type,
    u_int8_t bw_profile_parameter);
#endif /* HAVE_PROVIDER_BRIDGE */
struct nsm_band_width_profile *
nsm_bridge_if_bw_profile_init (struct nsm_if *zif);


struct interface *
nsm_if_lookup_by_uni_id (struct if_vr_master *ifm, char *uni_name);


s_int32_t
nsm_pro_edge_port_set_pvid (struct interface *ifp,
                            u_int16_t svid,
                            u_int16_t pvid);

s_int32_t
nsm_pro_edge_port_set_untagged_vid (struct interface *ifp,
                                    u_int16_t svid,
                                    u_int16_t untagged_vid);

struct nsm_svlan_reg_info *
nsm_reg_tab_svlan_info_lookup (struct nsm_cvlan_reg_tab *reg_tab,
                               u_int16_t svid);

int
nsm_pro_vlan_ce_port_sync (struct nsm_master *nm,
                           struct interface *ifp);

void
nsm_pro_vlan_send_proto_process (u_int8_t *bridge_name,
                                 struct interface *ifp,
                                 u_int8_t bpdu_process);

s_int32_t
nsm_customer_edge_port_set_def_svid (struct interface *ifp,
                                     u_int16_t svid, 
                                     enum nsm_vlan_port_mode mode,
                                     enum nsm_vlan_port_mode sub_mode);

s_int32_t
nsm_customer_edge_port_unset_def_svid (struct interface *ifp, 
                                       enum nsm_vlan_port_mode mode,
                                       enum nsm_vlan_port_mode sub_mode);
#ifdef HAVE_SMI

int
nsm_pro_vlan_set_dtag_mode (struct interface *ifp,
                            enum smi_dtag_mode dtag_mode);

int
nsm_pro_vlan_get_dtag_mode (struct interface *ifp,
                            enum smi_dtag_mode *dtag_mode);

#endif /* HAVE_SMI */

#define NSM_L2_ERR_NONE                         0

#define NSM_DEFAULT_VAL                         0

#define NSM_L2_ERR_BASE                         -200
#define NSM_L2_ERR_INVALID_ARG                  (NSM_L2_ERR_BASE + 1)
#define NSM_L2_ERR_MEM                          (NSM_L2_ERR_BASE + 2)


#define NSM_PRO_VLAN_ERR_BASE                   -900
#define NSM_PRO_VLAN_ERR_SCIDX_ALLOC_FAIL       (NSM_PRO_VLAN_ERR_BASE + 1)
#define NSM_PRO_VLAN_ERR_SWCTX_HAL_ERR          (NSM_PRO_VLAN_ERR_BASE + 2)
#define NSM_PRO_VLAN_ERR_INVALID_MODE           (NSM_PRO_VLAN_ERR_BASE + 3)
#define NSM_PRO_VLAN_ERR_CVLAN_REGIS_EXISTS     (NSM_PRO_VLAN_ERR_BASE + 4)
#define NSM_PRO_VLAN_ERR_CVLAN_REGIS_NOT_FOUND  (NSM_PRO_VLAN_ERR_BASE + 5)
#define NSM_PRO_VLAN_ERR_CVLAN_REGIS_HAL_ERR    (NSM_PRO_VLAN_ERR_BASE + 6)
#define NSM_PRO_VLAN_ERR_CVLAN_PORT_NOT_FOUND   (NSM_PRO_VLAN_ERR_BASE + 7)
#define NSM_PRO_VLAN_ERR_CVLAN_MAP_EXISTS       (NSM_PRO_VLAN_ERR_BASE + 8)
#define NSM_PRO_VLAN_ERR_VLAN_TRANS_HAL_ERR     (NSM_PRO_VLAN_ERR_BASE + 9)
#define NSM_PRO_VLAN_ERR_CVLAN_MAP_ERR          (NSM_PRO_VLAN_ERR_BASE + 10)
#define NSM_PRO_VLAN_ERR_INVALID_SERVICE_ATTR   (NSM_PRO_VLAN_ERR_BASE + 11)
#define NSM_PRO_VLAN_ERR_PRO_EDGE_NOT_FOUND     (NSM_PRO_VLAN_ERR_BASE + 12)
#define NSM_PRO_VLAN_ERR_HAL_ERR                (NSM_PRO_VLAN_ERR_BASE + 13)
#define NSM_PRO_VLAN_ERR_VLAN_TRANS_EXISTS      (NSM_PRO_VLAN_ERR_BASE + 14)
#define NSM_ERR_BW_PROFILE_PER_UNI_CONFIGURED   (NSM_PRO_VLAN_ERR_BASE + 15)
#define NSM_ERR_BW_PROFILE_PER_COS_CONFIGURED   (NSM_PRO_VLAN_ERR_BASE + 16)
#define NSM_ERR_BW_PROFILE_PER_EVC_CONFIGURED   (NSM_PRO_VLAN_ERR_BASE + 17)
#define NSM_ERR_COS_ALREADY_CONFIGURED          (NSM_PRO_VLAN_ERR_BASE + 18)
#define NSM_ERR_BW_PROFILE_NOT_CONFIGURED       (NSM_PRO_VLAN_ERR_BASE + 19)
#define NSM_ERR_BW_PROFILE_CBS_NOT_CONFIGURED   (NSM_PRO_VLAN_ERR_BASE + 20)
#define NSM_ERR_BW_PROFILE_EBS_NOT_CONFIGURED   (NSM_PRO_VLAN_ERR_BASE + 21)
#define NSM_ERR_BW_PROFILE_EIR_NOT_CONFIGURED   (NSM_PRO_VLAN_ERR_BASE + 22)
#define NSM_ERR_BW_PROFILE_ACTIVE               (NSM_PRO_VLAN_ERR_BASE + 23)
#define NSM_ERR_BW_PROFILE_NOT_ACTIVE           (NSM_PRO_VLAN_ERR_BASE + 24)
#define NSM_ERR_CROSSED_MAX_EVC                 (NSM_PRO_VLAN_ERR_BASE + 25)
#define NSM_ERR_MAX_EVC_LESS                    (NSM_PRO_VLAN_ERR_BASE + 26)
#define NSM_ERR_MAX_EVC_CONFIGURED              (NSM_PRO_VLAN_ERR_BASE + 27)
#define NSM_ERR_INSTANCE_ID_DO_NOT_MATCH        (NSM_PRO_VLAN_ERR_BASE + 28)
#define NSM_ERR_UNI_MODE_CONFIGURED             (NSM_PRO_VLAN_ERR_BASE + 29)

#endif /* __L2_PRO_VLAN_H_ */
