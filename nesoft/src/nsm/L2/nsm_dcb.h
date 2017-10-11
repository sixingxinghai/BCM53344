/* Copyright (C) 2009  All Rights Reserved */

/**@file nsm_dcb.h
   @brief This file defines stuctures and macros related to DCB.
*/

#ifndef __NSM_DCB_H__
#define __NSM_DCB_H__

#ifdef HAVE_DCB

/* Function Enter and Exit Macros for debugging */
#ifdef DEBUG
#ifdef HAVE_ISO_MACRO_VARARGS
#define NSM_DCB_FN_ENTER(...)                                         \
    do {                                                              \
         zlog_info (NZG, "Entering %s\n", __FUNCTION__);              \
    } while (0)
#define NSM_DCB_FN_EXIT(...)                                          \
    do {                                                              \
         zlog_info (NZG, "Return %s, %d\n", __FUNCTION__, __LINE__);  \
         return __VA_ARGS__;                                          \
    } while (0)
#else
#define NSM_DCB_FN_ENTER(ARGS...)                                     \
    do {                                                              \
         zlog_info (NZG, "Entering %s\n", __FUNCTION__);              \
    } while (0)
#define NSM_DCB_FN_EXIT(ARGS...)                                      \
    do {                                                              \
         zlog_info (NZG, "Return %s, %d\n", __FUNCTION__, __LINE__);  \
         return ARGS;                                                 \
    } while (0)
#endif /* HAVE_ISO_MACRO_VARARGS */
#else
#ifdef HAVE_ISO_MACRO_VARARGS
#define NSM_DCB_FN_ENTER(...)
#else
#define NSM_DCB_FN_ENTER(ARGS...)
#endif /* HAVE_ISO_MACRO_VARARGS */
#ifdef HAVE_ISO_MACRO_VARARGS
#define NSM_DCB_FN_EXIT(...)                                         \
    return __VA_ARGS__                      
#else
#define NSM_DCB_FN_EXIT(ARGS...)                                     \
    return ARGS
#endif /* HAVE_ISO_MACRO_VARARGS */
#endif /* DEBUG */

/* Default values */
#define NSM_DCB_MAX_TCG_DEFAULT 		8
#define NSM_DCB_NUM_USER_PRI 			8
#define NSM_DCB_MIN_USER_PRI 			0
#define NSM_DCB_MAX_USER_PRI 			7
#define NSM_DCB_ETS_MIN_TCGID 			0
#define NSM_DCB_ETS_MAX_TCGID 			7
#define NSM_DCB_ETS_DEFAULT_TCGID 		15
#define DCB_PFC_ENABLE                  	1
#define DCB_PFC_DISABLE                 	0
#define NSM_DCB_PFC_DEFAULT_PRIORITY    	0
#define NSM_DCB_PFC_CAP_DEFAULT         	8
#define NSM_DCB_PFC_LINK_DELAY_ALLOW_DEFAULT 	0
#define NSM_DCB_PFC_REQUEST_SENT        	0
#define NSM_DCB_PFC_INDICATIONS_RCVD    	0
#define NSM_DCB_PFC_MIN_PRI             	0	
#define NSM_DCB_PFC_MAX_PRI             	7
#define NSM_DCB_PFC_INVALID_PRI         	8

/* Bridge Structure for DCB */
struct nsm_dcb_bridge 
{
  /* Pointer to nsm bridge */ 
  struct nsm_bridge *bridge;

  /* This indicates the global configuration flags */
  u_int8_t dcb_global_flags;
#define NSM_DCB_ENABLE (1 << 0)
#define NSM_DCB_ETS_ENABLE (1 << 1)        		
#define NSM_DCB_PFC_ENABLE (1 << 2)

  /* This indicates the list of dcb interfaces */
  struct avl_tree *dcb_if_list; 	
};
 
enum nsm_dcb_mode
{
  NSM_DCB_MODE_ON = 0,
  NSM_DCB_MODE_AUTO = 1
};

enum nsm_dcb_pfc_mode
{
      NSM_DCB_PFC_MODE_ON = 0,
      NSM_DCB_PFC_MODE_AUTO = 1
};

/* DCB interface Structure */
struct nsm_dcb_if
{
  /* Pointer to dcb bridge */
  struct nsm_dcb_bridge *dcbg;

  /* Interface pointer. */
  struct interface *ifp;

  /* This indicates priority and TCG mapping 
   * If priority 1 belongs to TCG 2 then tcg_priority_table[1] = 2 
   */
  u_int8_t tcg_priority_table[NSM_DCB_NUM_USER_PRI];  

  /* This indicates TCG and BW mapping. Total should be 100 
   * If 50% bandwidth is assigned to TCG 1 and remaining to TCG 4 then 
   * tcg_bandwidth_table [1] = 50 and tcg_bandwidth_table [4] = 50.
   */
  u_int16_t tcg_bandwidth_table[NSM_DCB_MAX_TCG_DEFAULT];  

  /* This indicates priority and PFC enable/disable state mapping 
   * If PFC is enabled on priorities 2 and 4 pfc_enable_vector[2] = 1
   * and pfc_enable_vector[4] = 1 
   */
  u_int8_t pfc_enable_vector[NSM_DCB_NUM_USER_PRI]; 

  /* This indicates number of priorities in the TCG */
  u_int8_t tcg_priority_count[NSM_DCB_MAX_TCG_DEFAULT]; 

  /* This indicates the DCB flags on interface */
  u_int8_t dcb_if_flags;
#define NSM_DCB_IF_ENABLE (1 << 0)
#define NSM_DCB_IF_ETS_ENABLE (1 << 1)
#define NSM_DCB_IF_PFC_ENABLE (1 << 2)

  /* This indicates the ETS mode, on/auto */
  enum nsm_dcb_mode dcb_ets_mode;

  /* This indicates the PFC mode, on/auto */
  enum nsm_dcb_pfc_mode pfc_mode;

  /* This indicates max number of priority that can support PFC*/
  u_int8_t pfc_cap;

  /* This indicates PFC link delay allowance */
  u_int32_t pfc_link_delay_allowance;

  /* This is to maintain number of PFC requests sent*/
   u_int32_t pfc_requests_sent;

  /* This is to maintain number of PFC indications received*/
   u_int32_t pfc_indications_rcvd;

  /*This is to maintain priorities count on which PFC is enable*/
   u_int8_t pfc_en_pri_count;

  /* This will be list of all application priorities set on the interface */
  struct avl_tree *dcb_appl_pri;	
};

enum nsm_dcb_appl_pri_mapping_type
{
  NSM_DCB_MAP_SERV_NAME = 0,
  NSM_DCB_MAP_PORT_NO = 1,
  NSM_DCB_MAP_ETHERTYPE_VALUE = 2,
  NSM_DCB_MAP_ETHERTYPE_STR = 3
};

/* Protocol Selector values */
#define NSM_DCB_PROTO_ETHERTYPE     	 0
#define NSM_DCB_PROTO_TCP            	 1
#define NSM_DCB_PROTO_UDP            	 2
#define NSM_DCB_PROTO_BOTH_TCP_UDP   	 3
#define NSM_DCB_PROTO_NO_TCP_UDP	 4

/* DCB Application Priority Structure */
struct nsm_dcb_application_priority
{
  /* DCB Interface Pointer */
  struct nsm_dcb_if *dcbif;

  /*Protocol Selector*/
  u_int8_t sel;  

  /* Protocol Id or well known port numbers */
  u_int16_t proto_id; 

  u_int8_t priority; 

  enum nsm_dcb_appl_pri_mapping_type mapping_type;
};

#endif /* HAVE_DCB */
#endif /* __NSM_DCB_H__ */
