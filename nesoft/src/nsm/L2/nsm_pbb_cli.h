/* Copyright (C) 2004  All Rights Reserved */
#ifndef __NSM_PBB_CLI_H__
#define __NSM_PBB_CLI_H__

/* Error codes yet to be defined */

#define NSM_PBB_VLAN_ERR_PORT_MODE_INVALID   -11  /*switch port mode not set properly*/
#define NSM_PBB_VLAN_ERR_PORT_CNP            -12  /*port is already configured as cnp*/
#define NSM_PBB_VIP_PIP_MAP_EXHAUSTED        -13  /*.no more vips can be added to this pip*/
#define NSM_PBB_VLAN_ERR_CNP_PORT_BASED      -14  /*.cnp configured to port based cant edit this to svlan based.*/
#define NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED  -15  /*.cannot edit a dispatched service.*/
#define NSM_PBB_BACKBONE_BRIDGE_UNCONFIGURED -16  /*.backbone bridge not configured.*/
#define NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST -17  /*. no corresponding entry been added.*/
#define NSM_PBB_VLAN_ERR_PORT_ENTRY_EXISTS   -18  /*.Entry already exists.*/
#define NSM_PBB_SERVICE_DISPATCH_ERROR       -19  /*.service cannot be dispatched*/
#define NSM_PBB_SERVICE_REMOVE_ERROR         -20  /*.service cannot be removed */
#define NSM_PBB_LOGICAL_INTERFACE_ERROR      -21  /*.logical interface addition error */
#define NSM_PBB_ISID_EXISTS                  -22  /*.ISID is not added yet */
#define NSM_PBB_ISID_NOT_CREATED             -23  /*ISID not created yet */
#define NSM_PBB_SERVICE_MAPPED_TO_DIFF_PIP   -24  /* service mapped to another PIP */

/*Error codes definition ends here */

typedef u_int32_t pbb_isid_t;
#define NSM_PBB_ISID_MIN 1 
#define NSM_PBB_ISID_MAX 16777214
#define NSM_PBB_ISID_NONE 0
/* Function prototypes */
void nsm_pbb_cli_init (struct lib_globals *zg);
int nsm_pbb_if_config_write (struct cli *cli, struct interface *ifp);

#endif /* __NSM_PBB_CLI_H__ */


