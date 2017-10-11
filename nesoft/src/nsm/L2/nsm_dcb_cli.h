/* Copyright (C) 2009  All Rights Reserved */

/**@file nsm_dcb_cli.h
   @brief This file defines cli functions and strings.
*/

#ifndef __NSM_DCB_CLI_H__
#define __NSM_DCB_CLI_H__

#ifdef HAVE_DCB

#define NSM_DCB_STR "Data-Center-Briding"
#define NSM_ETS_STR "Enhanced-Transmission-Selection"
#define NSM_PFC_STR "Priority-Flow-Control"
#define NSM_TCG_STR "Traffic-Class-Group"
#define NSM_TCGID_STR "Specify Traffic-Class-Group Id"
#define NSM_BW_STR "Specify Bandwidth Percentage"
#define BRIDGE_STR "Bridge Group commands"
#define BRIDGE_NAME_STR "Bridge Group name for bridging"

s_int32_t
nsm_dcb_cli_return (struct cli *cli, s_int32_t ret);

s_int32_t 
nsm_dcb_config_write (struct cli *cli);

s_int32_t 
nsm_dcb_if_config_write (struct cli *cli, char *ifname);

void_t
nsm_dcb_cli_init (struct cli_tree *ctree);

#endif /* HAVE_DCB */
#endif /* __NSM_DCB_CLI_H__*/
