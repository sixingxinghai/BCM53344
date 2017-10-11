/* Copyright (C) 2009  All Rights Reserved */

/**@file nsm_dcb_show.h
   @brief This file defines DCB show related functions.
*/

#ifndef __NSM_DCB_SHOW_H__
#define __NSM_DCB_SHOW_H__

#ifdef HAVE_DCB

struct vrep *vrep;

void_t 
nsm_dcb_show_cli_init (struct cli_tree *ctree);

s_int32_t
nsm_dcb_show_tcg_by_id (struct cli* cli, u_int8_t tcgid, char *bridge_name);

s_int32_t
nsm_dcb_show_tcg_by_intf (struct cli *cli, char *ifname);

s_int32_t
nsm_dcb_show_tcg_by_bridge (struct cli *cli, char *bridge_name);

s_int32_t
nsm_dcb_show_appl_priority_table (struct cli *cli, char *ifname);

s_int32_t
nsm_dcb_show_pfc_request_stats_by_interface (struct cli* cli, char *ifname);

s_int32_t
nsm_dcb_pfc_avl_trav_req_show_bridge (void *avl_node_info, void *arg1);

s_int32_t
nsm_dcb_show_pfc_request_stats_by_bridge (struct cli *cli, char *bridge_name);

s_int32_t
nsm_dcb_show_pfc_indication_stats_by_interface (struct cli* cli, char *ifname);

s_int32_t
nsm_dcb_pfc_avl_trav_indi_show_bridge (void *avl_node_info, void *arg1);

s_int32_t
nsm_dcb_show_pfc_indication_stats_by_bridge (struct cli *cli, char *bridge);

s_int32_t
nsm_dcb_show_pfc_details_by_interface (struct cli* cli, struct vrep *vrep, 
                                        char *ifname);

s_int32_t
nsm_dcb_pfc_avl_trav_detail_show_bridge (void *avl_node_info, void *arg1, 
                                          void *arg2);

s_int32_t
nsm_dcb_show_pfc_details_by_bridge (struct cli *cli, struct vrep *vrep, 
                                     char *bridge_name);

#endif /* HAVE_DCB */
#endif /* __NSM_DCB_SHOW_H__ */
