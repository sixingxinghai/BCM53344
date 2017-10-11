/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _NSM_VLAN_CLI_H_
#define _NSM_VLAN_CLI_H

#define NSM_VLAN_ERR_BASE                       -100
#define NSM_VLAN_ERR_BRIDGE_NOT_FOUND           (NSM_VLAN_ERR_BASE + 1)
#define NSM_VLAN_ERR_NOMEM                      (NSM_VLAN_ERR_BASE + 2)
#define NSM_VLAN_ERR_INVALID_MODE               (NSM_VLAN_ERR_BASE + 3)
#define NSM_VLAN_ERR_VLAN_NOT_FOUND             (NSM_VLAN_ERR_BASE + 4)
#define NSM_VLAN_ERR_MODE_CLEAR                 (NSM_VLAN_ERR_BASE + 5)
#define NSM_VLAN_ERR_MODE_SET                   (NSM_VLAN_ERR_BASE + 6)
#define NSM_VLAN_ERR_GENERAL                    (NSM_VLAN_ERR_BASE + 7)
#define NSM_VLAN_ERR_IFP_NOT_BOUND              (NSM_VLAN_ERR_BASE + 8)
#define NSM_VLAN_ERR_BRIDGE_NOT_VLAN_AWARE      (NSM_VLAN_ERR_BASE + 9)
#define NSM_VLAN_ERR_VLAN_NOT_IN_PORT           (NSM_VLAN_ERR_BASE + 10)
#define NSM_VLAN_ERR_CONFIG_PVID_TAG            (NSM_VLAN_ERR_BASE + 11)
#define NSM_VLAN_ERR_SVLAN_NOT_FOUND            (NSM_VLAN_ERR_BASE + 12)
#define NSM_VLAN_ERR_CVLAN_REG_EXIST            (NSM_VLAN_ERR_BASE + 13)
#define NSM_VLAN_ERR_CVLAN_PORT_REG_EXIST       (NSM_VLAN_ERR_BASE + 14)
#define NSM_VLAN_ERR_NOT_EDGE_BRIDGE            (NSM_VLAN_ERR_BASE + 15)
#define NSM_VLAN_ERR_VC_BOUND                   (NSM_VLAN_ERR_BASE + 16)
#define NSM_VLAN_ERR_INVALID_FRAME_TYPE         (NSM_VLAN_ERR_BASE + 17)
#define NSM_VLAN_ERR_NOT_PROVIDER_BRIDGE        (NSM_VLAN_ERR_BASE + 18)
#define NSM_VLAN_ERR_HAL_ERROR                  (NSM_VLAN_ERR_BASE + 19)
#define NSM_VLAN_ERR_VRRP_BIND_EXISTS           (NSM_VLAN_ERR_BASE + 20)
#define NSM_VLAN_ERR_NOT_BACKBONE_BRIDGE        (NSM_VLAN_ERR_BASE + 21)
#define NSM_VLAN_ERR_PBB_CONFIG_EXIST           (NSM_VLAN_ERR_BASE + 22)
#define NSM_PBB_VLAN_ERR_PORT_ADDR_NOT_SET      (NSM_VLAN_ERR_BASE + 23)
#define NSM_VLAN_ERR_G8031_CONFIG_EXIST         (NSM_VLAN_ERR_BASE + 24)
#define NSM_RING_PRIMARY_VID_ALREADY_EXISTS     (NSM_VLAN_ERR_BASE + 25)
#define NSM_VLAN_ERR_G8032_CONFIG_EXIST         (NSM_VLAN_ERR_BASE + 26)
#define NSM_VLAN_ERR_IFP_NOT_DELETED            (NSM_VLAN_ERR_BASE + 27)
#define NSM_VLAN_ERR_MAX_UNI_PT_TO_PT           (NSM_VLAN_ERR_BASE + 28)
#define NSM_VLAN_ERR_MAX_UNI_M2M                (NSM_VLAN_ERR_BASE + 30)
#define NSM_VLAN_ERR_EVC_ID_SET                 (NSM_VLAN_ERR_BASE + 31)
#define NSM_PRO_VLAN_ERR_SVLAN_SERVICE_ATTR     (NSM_VLAN_ERR_BASE + 32)
#define NSM_PRO_VLAN_ERR_DEFAULT_EVC_NOT_SET    (NSM_VLAN_ERR_BASE + 33)
#define NSM_VLAN_ERR_RESERVED_IN_HW             (NSM_VLAN_ERR_BASE + 34)
#define NSM_VLAN_ERR_IFP_INVALID                (NSM_VLAN_ERR_BASE + 35)

#define NSM_VLAN_STR   "VLAN"

#define NSM_VLAN_CVLAN_STR   "Customer VLAN"

#define NSM_VLAN_SVLAN_STR   "Service VLAN"

#define NSM_VLAN_P2P_STR     "Point to Point Service VLAN"

#define NSM_VLAN_M2M_STR     "Multi Point to Multi Point Service VLAN"

#define NSM_VLAN_SW_STR      "Set the switching characteristics" \
                             "of the Layer2 interface"

#define NSM_VLAN_PRO_STR     "Set the switching characteristics" \
                             "of provider Interface"

#define NSM_VLAN_MODE_STR    "Set the mode of the Layer2 interface"

#define NSM_VLAN_CE_STR      "Set the Layer2 interface as Customer Edge"

#define NSM_VLAN_CN_STR      "Set the Layer2 interface as Customer Network"

#define NSM_VLAN_CNP_STR      "Set the Layer2 interface as Customer Network Port"

#define NSM_VLAN_PIP_STR      "Set the Layer2 interface as Provider Instance Port"

#define NSM_VLAN_CBP_STR      "Set the Layer2 interface as Customer Backbone Port"

#define NSM_VLAN_PNP_STR      "Set the Layer2 interface as Provider Network Port"                             

#define NSM_VLAN_ACCESS_STR  "Set the Layer2 interface as Access"

#define NSM_VLAN_HYBRID_STR  "Set the Layer2 interface as hybrid"

#define NSM_VLAN_TRUNK_STR   "Set the Layer2 interface as trunk"

#define NSM_VLAN_PN_STR      "Set the Layer2 interface as provider network"

/* Function prototypes. */
int nsm_vlan_if_config_write (struct cli *cli, struct interface *ifp);

int
nsm_g8031_vlan_config_write (struct cli *cli);


#endif /* _NSM_VLAN_CLI_H */
