#ifndef _NSM_L2_OAM_CLI_H_
#define _NSM_L2_OAM_CLI_H_

#define NSM_L2_OAM_ETHER_STR                 "Ethernet Interface Configuration"
#define NSM_L2_OAM_ETHER_OAM_STR             "Ethernet OAM Configuration"
#define NSM_L2_OAM_LINK_FAULT_STR            "Link Fault Event"
#define NSM_L2_OAM_CRITICAL_LINK_STR         "Critical Link Event"
#define NSM_L2_OAM_DYING_GASP_STR            "Dying Gasp Event"
#define NSM_L2_OAM_LOCAL_EVENT_STR           "Send a Local Event to the OAM" \
                                             " Prorocol"
#define NSM_L2_OAM_LINK_MONITOR_STR          "Link Monitor related commands"
#define NSM_L2_OAM_SYMBOL_PERIOD_ERR_STR     "Symbol Period Errors"
#define NSM_L2_OAM_FRAME_PERIOD_ERR_STR      "Frame Period Errors"
#define NSM_L2_OAM_FRAME_ERR_STR             "Frame Event Errors"
#define NSM_L2_OAM_FRAME_SECONDS_ERR_STR     "Frame Seconds Errors"
#define NSM_L2_OAM_ERR_VAL_STR               "Error Value <0-65535>"
#define NSM_L2_OAM_FRAME_SECS_ERR_VAL_STR    "Error Value <1-900>"
#define NSM_L2_OAM_ERROR_VAL_MIN             0
#define NSM_L2_OAM_ERROR_VAL_MAX             65535
#define NSM_L2_OAM_FRAME_SECS_ERR_VAL_MIN    1
#define NSM_L2_OAM_FRAME_SECS_ERR_VAL_MAX    900

void nsm_efm_oam_cli_init (struct lib_globals *zg);

#endif /* _NSM_L2_OAM_CLI_H_ */
