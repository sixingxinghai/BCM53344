/* Copyright (C) 2004  All Rights Reserved */

#ifndef __NSM_BRIDGE_CLI_H__
#define __NSM_BRIDGE_CLI_H__

#define NSM_BRIDGE_STR        "Bridge group commands."
#define NSM_BRIDGE_NAME_STR   "Bridge group for bridging."
#define NSM_SPANNING_STR      "Spanning Tree group commands."

/* Error codes. */
#define NSM_BRIDGE_ERR_BASE              -100
#define NSM_BRIDGE_ERR_EXISTS            (NSM_BRIDGE_ERR_BASE + 1)
#define NSM_BRIDGE_ERR_NOTFOUND          (NSM_BRIDGE_ERR_BASE + 2)
#define NSM_BRIDGE_ERR_MEM               (NSM_BRIDGE_ERR_BASE + 3)
#define NSM_BRIDGE_ERR_NOT_BOUND         (NSM_BRIDGE_ERR_BASE + 4)
#define NSM_BRIDGE_ERR_ALREADY_BOUND     (NSM_BRIDGE_ERR_BASE + 5)
#define NSM_BRIDGE_ERR_MISMATCH          (NSM_BRIDGE_ERR_BASE + 6)
#define NSM_HAL_FDB_ERR                  (NSM_BRIDGE_ERR_BASE + 7)
#define NSM_BRIDGE_ERR_GENERAL           (NSM_BRIDGE_ERR_BASE + 8)
#define NSM_BRIDGE_ERR_SET_AGE_TIME      (NSM_BRIDGE_ERR_BASE + 9)
#define NSM_BRIDGE_NOT_CFG               (NSM_BRIDGE_ERR_BASE + 10)
#define NSM_BRIDGE_NO_PORT_CFG           (NSM_BRIDGE_ERR_BASE + 11)
#define NSM_BRIDGE_ERR_INVALID_PROTO     (NSM_BRIDGE_ERR_BASE + 12)
#define NSM_BRIDGE_ERR_INVALID_ARG       (NSM_BRIDGE_ERR_BASE + 13)
#define NSM_BRIDGE_ERR_HAL               (NSM_BRIDGE_ERR_BASE + 13)
#define NSM_BRIDGE_ERR_INVALID_MODE      (NSM_BRIDGE_ERR_BASE + 14)
#define NSM_BRIDGE_ERR_BACKBONE_EXISTS   (NSM_BRIDGE_ERR_BASE + 15)

#define NSM_BRIDGE_ERR_PG_NOTFOUND       (NSM_BRIDGE_ERR_BASE + 16)
#define NSM_BRIDGE_ERR_PG_FOUND          (NSM_BRIDGE_ERR_BASE + 17)
#define NSM_BRIDGE_ERR_TESID_NOT_EXISTS  (NSM_BRIDGE_ERR_BASE + 18)
#define NSM_BRIDGE_ERR_PG_VLAN_EXISTS    (NSM_BRIDGE_ERR_BASE + 19)
#define NSM_BRIDGE_PG_ALREADY_EXISTS     (NSM_BRIDGE_ERR_BASE + 20)
#define NSM_PG_VLAN_OPERATION_NOT_ALLOWED (NSM_BRIDGE_ERR_BASE + 21)
#define NSM_G8031_LACP_IF_CONFIG_ERR     (NSM_BRIDGE_ERR_BASE + 22) 

#define NSM_BRIDGE_ERR_RING_NOTFOUND     (NSM_BRIDGE_ERR_BASE + 23)
#define NSM_BRIDGE_ERR_RING_VLAN_EXISTS  (NSM_BRIDGE_ERR_BASE + 24)
#define NSM_G8032_LACP_IF_CONFIG_ERR     (NSM_BRIDGE_ERR_BASE + 25)
#define NSM_G8032_INTERFACE_NAME_SAME_ERROR (NSM_BRIDGE_ERR_BASE + 26)
#define NSM_G8032_RING_NODE_NOT_CREATED   (NSM_BRIDGE_ERR_BASE + 27)
#define NSM_G8031_INSTANCE_IN_USE        (NSM_BRIDGE_ERR_BASE + 28)

#define NSM_G8032_MIN_RING_VAL  1
#define NSM_G8032_MAX_RING_VAL  65535

#define NSM_INTERFACE_L2_MODE            -200
#define NSM_INTERFACE_NOT_FOUND          -199
#define NSM_INTERFACE_IN_USE             -198
#define NSM_INTERFACE_NOT_UP             -197 

#define NSM_ERR_DUPLICATE_MAC            -300

#define NSM_ERR_BRIDGE_LIMIT             -24
#define NSM_BRIDGE_RING_ALREADY_EXISTS   -23
#define NSM_READ_MAC_ADDRESS(A,M)                                        \
        if (pal_sscanf ((A), "%4hx.%4hx.%4hx",                           \
                       (unsigned short *)&(M)[0],                        \
                       (unsigned short *)&(M)[2],                        \
                       (unsigned short *)&(M)[4]) != 3)                  \
         {                                                               \
           cli_out (cli, "%% Invalid MAC address format %s\n", (A));     \
           return CLI_ERROR;                                             \
         }

#define NSM_RESERVED_ADDRESS_CHECK(M)                                                       \
        do {                                                                                \
            if ((pal_strncmp (M,"0180.c200.00",12) == 0) &&                                 \
               ((pal_strncmp((M+12),"2",1) == 0) || (pal_strncmp ((M+12),"0",1) == 0)))     \
             {                                                                              \
              cli_out(cli, "%% Reserved address is not allowed %s\n",M);                    \
              return CLI_ERROR;                                                             \
             }                                                                              \
           }while (0)  
        


/* Function prototypes. */
int nsm_bridge_if_config_write (struct cli *cli, struct interface *ifp);
int nsm_bridge_cli_init (struct lib_globals *zg);

#endif /* __NSM_BRIDGE_CLI_H__ */
