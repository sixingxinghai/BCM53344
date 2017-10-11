/**@file elmi_cli.h
 * * @brief  This elm_cli.h file contains the Prototypes for ELMI User
 *          interface functionality.
 * **/
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_CLI_H_
#define _PACOS_ELMI_CLI_H_

#include "elmi_types.h"
#define BRIDGE_STR        "Bridge group commands"
#define BRIDGE_NAME_STR   "Bridge Group name for bridging"

void
elmi_cli_init (struct lib_globals * ezg);
void
elmi_cli_show_init (struct lib_globals * ezg);

int
elmi_display_lmi_parameters_info_if (char *ifName, struct cli *cli);

int
elmi_display_evc_detail_info (struct cli *cli, struct elmi_ifp *elmi_if,
                              struct elmi_evc_status *evc_info);
int
elmi_display_evc_info (struct cli *cli, struct elmi_ifp *elmi_if);

int
elmi_display_interface_evc_info (struct cli *cli, struct elmi_ifp *elmi_if);

int
elmi_display_interface_evc_details (struct cli *cli, struct elmi_ifp *elmi_if);

int
elmi_display_if_evc_detail_by_vid (struct cli *cli,
                                    struct elmi_ifp *elmi_if,
                                    u_int16_t evc_ref_id);
int
elmi_display_if_evc_detail_by_name (struct cli *cli,
                                     struct elmi_ifp *elmi_if,
                                     u_char *evc_id);

int
elmi_display_bridge_evc_info (struct cli *cli, u_int8_t *br_name);

int
elmi_display_br_evc_detail_by_name (struct cli *cli, u_int8_t *br_name,
      u_char *evc_id);

int
elmi_display_br_evc_detail_by_vid (struct cli *cli, u_int8_t *br_name,
      u_int16_t evc_ref_id);

int
elmi_display_lmi_evc_map_info (struct cli *cli, struct elmi_ifp *elmi_if);
int
elmi_display_uni_info (struct cli *cli, u_int8_t *if_name);

int
elmi_display_statistics_if (struct cli *cli, struct elmi_ifp *elmi_if);

int
elmi_display_statistics_bridge (struct cli *cli, u_int8_t *bridge_name);

int
elmi_clear_statistics_interface (struct cli *cli, struct elmi_ifp *elmi_if);

int
elmi_clear_statistics_bridge (struct cli *cli, u_int8_t *bridge_name);



#endif /* _PACOS_ELMI_CLI_H_ */
