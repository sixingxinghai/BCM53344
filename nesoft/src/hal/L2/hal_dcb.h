/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_DCB_H_
#define _HAL_DCB_H_

#ifdef HAVE_DCB

#define HAL_DCB_NUM_DEFAULT_TCGS   8
#define HAL_DCB_NUM_USER_PRIORITY    8

enum hal_dcb_mode
{
  HAL_DCB_MODE_ON = 0,
  HAL_DCB_MODE_AUTO = 1
};

s_int32_t
hal_dcb_init (char *bridge_name);

s_int32_t 
hal_dcb_deinit (char *bridge_name);

s_int32_t 
hal_dcb_bridge_enable (char *bridge_name);

s_int32_t 
hal_dcb_bridge_disable (char *bridge_name);

s_int32_t
hal_dcb_ets_bridge_enable (char *brigde_name);

s_int32_t
hal_dcb_ets_bridge_disable (char *bridge_name);

s_int32_t
hal_dcb_interface_enable (char *bridge_name, s_int32_t ifindex);

s_int32_t
hal_dcb_interface_disable (char *bridge_name, s_int32_t ifindex);

s_int32_t
hal_dcb_ets_interface_enable (char *bridge_name, s_int32_t ifindex);

s_int32_t
hal_dcb_ets_interface_disable (char *bridge_name, s_int32_t ifindex);

s_int32_t
hal_dcb_select_ets_mode (char *bridge_name, s_int32_t ifindex, 
                         enum hal_dcb_mode mode);

s_int32_t
hal_dcb_ets_add_pri_to_tcg (char *bridge_name, s_int32_t ifindex, 
                            u_int8_t tcgid, u_int8_t pri);

s_int32_t
hal_dcb_ets_remove_pri_from_tcg (char *bridge_name, s_int32_t ifindex, 
                                 u_int8_t tcgid, u_int8_t pri);

s_int32_t 
hal_dcb_ets_assign_bw_to_tcgs (char *bridge_name, s_int32_t ifindex, 
                               u_int16_t *bw);

s_int32_t
hal_dcb_ets_set_application_priority (char *bridge_name, s_int32_t ifindex, 
                                      u_int8_t sel, u_int16_t proto_id, 
                                      u_int8_t pri);

s_int32_t
hal_dcb_ets_unset_application_priority (char *bridge_name, s_int32_t ifindex, 
                                        u_int8_t sel, u_int16_t proto_id, 
                                        u_int8_t pri);


/* DCB-PFC related routines */
s_int32_t
hal_dcb_pfc_bridge_enable (char *bridge_name);

s_int32_t
hal_dcb_pfc_bridge_disable (char *bridge_name);

s_int32_t
hal_dcb_enable_pfc_priority (char *bridge, s_int32_t ifindex, s_int8_t pri);

s_int32_t
hal_dcb_set_pfc_cap (char *bridge_name, s_int32_t ifindex, u_int8_t cap);

s_int32_t
hal_dcb_set_pfc_lda (char *bridge_name, s_int32_t ifindex, u_int32_t lda);

s_int32_t
hal_dcb_select_pfc_mode (char *bridge_name, s_int32_t ifindex,
                         enum hal_dcb_mode mode);

s_int32_t
hal_dcb_pfc_interface_enable (char *bridge_name, s_int32_t ifindex);

#endif /* HAVE_DCB */
#endif /* __HAL_DCB_H__ */
