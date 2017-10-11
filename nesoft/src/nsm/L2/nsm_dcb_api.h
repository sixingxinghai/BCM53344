/* Copyright (C) 2009  All Rights Reserved */

/**@file nsm_dcb_api.h
   @brief This nsm_dcb_api.h file declares the NSM APIs related to DCB
          functionality.
*/

#ifndef __NSM_DCB_API_H__
#define __NSM_DCB_API_H__

#ifdef HAVE_DCB

/* Error Codes */
#define NSM_DCB_API_SET_SUCCESS 		0
#define NSM_DCB_API_SET_ERR_NO_NM 		-1
#define NSM_DCB_API_SET_ERR_NO_DCBG 		-2
#define NSM_DCB_API_SET_ERR_INTERFACE 		-3
#define NSM_DCB_API_SET_ERR_L2_INTERFACE 	-4
#define NSM_DCB_API_SET_ERR_NO_DCB 		-5
#define NSM_DCB_API_SET_ERR_NO_ETS 		-6
#define NSM_DCB_API_SET_ERR_DCB_IF_INIT 	-7
#define NSM_DCB_API_SET_ERR_ETS_IF_INIT 	-8
#define NSM_DCB_API_SET_ERR_INTERFACE_NO_DCB 	-9
#define NSM_DCB_API_SET_ERR_INTERFACE_NO_ETS 	-10
#define NSM_DCB_API_SET_ERR_DCB_INTERFACE 	-11
#define NSM_DCB_API_SET_ERR_ETS_INTERFACE 	-12
#define NSM_DCB_API_SET_ERR_WRONG_TCG 		-13
#define NSM_DCB_API_SET_ERR_DIFF_PFC 		-14
#define NSM_DCB_API_SET_ERR_WRONG_BW 		-15	
#define NSM_DCB_API_SET_ERR_APP_PRI_NOT_FOUND 	-16
#define NSM_DCB_API_SET_ERR_NO_SERV 		-17
#define NSM_DCB_API_SET_ERR_NO_SUPPORT 		-18
#define NSM_DCB_API_SET_ERR_NO_MEM 		-19
#define NSM_DCB_API_SET_ERR_DCB_EXISTS 		-20
#define NSM_DCB_API_SET_ERR_ETS_EXISTS 		-21
#define NSM_DCB_API_SET_ERR_INVALID_VALUE 	-22
#define NSM_DCB_API_SET_WARN_RECONFIG 		-23
#define NSM_DCB_API_SET_ERR_HW_NO_SUPPORT 	-24
#define NSM_DCB_API_SET_ERR_ETS_NO_TCGS         -25
#define NSM_DCB_API_SET_ERR_TCG_PRI_EXISTS      -26
#define NSM_DCB_API_SET_ERR_DUPLICATE_ARGS      -27
#define NSM_DCB_API_SET_ERR_DISPLAY_NO_TCG      -28
#define NSM_DCB_API_SET_ERR_DISPLAY_NO_APPL_PRI -29
#define NSM_DCB_API_SET_ERR_PFC_EXISTS          -30
#define NSM_DCB_API_SET_ERR_PFC_IF_INIT         -31
#define NSM_DCB_API_SET_ERR_PFC_INTERFACE       -32
#define NSM_DCB_API_SET_ERR_NO_PFC              -33
#define NSM_DCB_API_SET_ERR_INTERFACE_NO_PFC    -34
#define NSM_DCB_API_SET_ERR_EXCEED_PFC_CAP      -35
#define NSM_DCB_API_FAIL_TO_GET_DUPLEX          -36
#define NSM_DCB_API_SET_ERR_INTF_DUPLEX          -37
#define NSM_DCB_API_SET_ERR_LINK_FLOW_CNTL      -38
#define NSM_DCB_API_UNSET_ERR_LINK_FLOW_CNTL    -39
#define NSM_DCB_API_SET_ERR_PFC_CAP             -40
#define NSM_DCB_API_SET_ERR_RESV_VALUE          -41
#define NSM_DCB_API_SET_WARN_DIFF_TCG_PFC	-42

/* Functions to initialize DCB */
s_int32_t 
nsm_dcb_init (struct nsm_bridge *bridge);

s_int32_t 
nsm_dcb_deinit (struct nsm_bridge_master *master);

struct nsm_dcb_if* 
nsm_dcb_init_interface (u_int32_t vr_id, char *ifname);

s_int32_t
nsm_dcb_deinit_interface (u_int32_t vr_id, char *ifname);

struct nsm_dcb_if* 
nsm_dcb_ets_init_interface (u_int32_t vr_id, char *ifname);

/* Functions to enable/disable DCB at switch level */
s_int32_t 
nsm_dcb_bridge_enable (u_int32_t vr_id, char* bridge_name);

s_int32_t 
nsm_dcb_bridge_disable (u_int32_t vr_id, char* bridge_name);

s_int32_t 
nsm_dcb_ets_bridge_enable (u_int32_t vr_id, char *bridge_name);

s_int32_t 
nsm_dcb_ets_bridge_disable (u_int32_t vr_id, char *bridge_name);

/* Functions to enable/disable DCB at interface level */
s_int32_t 
nsm_dcb_enable_interface (u_int32_t vr_id, char *ifname);

s_int32_t 
nsm_dcb_disable_interface (u_int32_t vr_id, char *ifname);

s_int32_t 
nsm_dcb_ets_enable_interface (u_int32_t vr_id, char *ifname);

s_int32_t 
nsm_dcb_ets_disable_interface (u_int32_t vr_id, char *ifname);

/* Functions related to ETS feature of DCB at interface level */
s_int32_t 
nsm_dcb_set_ets_mode (u_int32_t vr_id, char *ifname, 
                      enum nsm_dcb_mode mode);

s_int32_t 
nsm_dcb_add_pri_to_tcg (u_int32_t vr_id, char *ifname, 
                        u_int8_t tcgid, u_int8_t pri);

s_int32_t 
nsm_dcb_remove_pri_from_tcg (u_int32_t vr_id, char *ifname, 
                             u_int8_t tcgid, u_int8_t pri);

s_int32_t 
nsm_dcb_assign_bw_percentage_to_tcg (u_int32_t vr_id, char *ifname, 
                                     u_int16_t *bw);

s_int32_t 
nsm_dcb_delete_tcgs (u_int32_t vr_id, char *ifname);

bool_t 
nsm_dcb_same_pfc_state (struct nsm_dcb_if *dcb_if, u_int8_t tcgid, 
                        u_int8_t pri);

/* Functions related to Application Priority */
s_int32_t 
nsm_dcb_application_priority_set (u_int32_t vr_id, char *ifname, 
                                  u_int8_t sel, u_int16_t proto_id, 
                                  u_int8_t pri,
                                  enum nsm_dcb_appl_pri_mapping_type type);

s_int32_t 
nsm_dcb_application_priority_unset (u_int32_t vr_id, char *ifname, 
                                    u_int8_t sel, u_int16_t proto_id,
                                    u_int8_t pri);

/* Functions related to PFC */
struct nsm_dcb_if*
nsm_dcb_pfc_init_interface (u_int32_t vr_id, char *ifname);

s_int32_t
nsm_dcb_avl_traverse_enable_pfc (void *avl_node_info, void *arg1);

s_int32_t
nsm_dcb_pfc_bridge_enable (u_int32_t vr_id, char *bridge_name);

s_int32_t
nsm_dcb_avl_traverse_disable_pfc (void *node_info, void *arg1);

s_int32_t
nsm_dcb_pfc_bridge_disable (u_int32_t vr_id, char *bridge_name);

s_int32_t
nsm_dcb_add_pfc_priority (u_int32_t vr_id, char *ifname, s_int8_t priority,
                          bool_t state);

s_int32_t
nsm_dcb_set_pfc_mode (u_int32_t id, char *ifname, enum nsm_dcb_pfc_mode mode);

s_int32_t
nsm_dcb_set_pfc_cap (u_int32_t vr_id, char *ifname, u_int8_t cap);

s_int32_t
nsm_dcb_set_pfc_lda (u_int32_t vr_id, char *ifname, u_int32_t lda);

s_int32_t
nsm_find_duplicate_args (u_int8_t count, u_int8_t value);

s_int32_t
nsm_dcb_pfc_disable_interface (u_int32_t vr_id, char *ifname);

s_int32_t
nsm_dcb_pfc_enable_interface (u_int32_t vr_id, char *ifname);

#endif /* HAVE_DCB*/
#endif /* __NSM_DCB_API_H__ */

