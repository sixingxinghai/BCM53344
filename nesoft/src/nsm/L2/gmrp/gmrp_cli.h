/*   Copyright (C) 2003  All Rights Reserved.  */

#ifndef __PACOS_GMRP_CLI_H__
#define __PACOS_GMRP_CLI_H__

/*
   802.1D GMRP 
   This module declares the interface to GMRP 
  
 */

#include "vty.h"

#define NSM_BRIDGE_ERR_GMRP_BASE       -25
#define NSM_BRIDGE_ERR_GMRP_NOCONFIG            (NSM_BRIDGE_ERR_GMRP_BASE + 1) 
#define NSM_ERR_GMRP_NOCONFIG_ONPORT            (NSM_BRIDGE_ERR_GMRP_BASE + 2)
#define NSM_GMRP_ERR_VLAN_NOT_FOUND             (NSM_BRIDGE_ERR_GMRP_BASE + 3)
#define NSM_GMRP_ERR_GMRP_GLOBAL_CFG_BR         (NSM_BRIDGE_ERR_GMRP_BASE + 4)
#define NSM_GMRP_ERR_GMRP_NOT_CFG_ON_VLAN       (NSM_BRIDGE_ERR_GMRP_BASE + 5)
#define NSM_GMRP_ERR_VLAN_NOT_CFG_ON_PORT       (NSM_BRIDGE_ERR_GMRP_BASE + 6)
#define NSM_GMRP_ERR_GMRP_GLOBAL_CFG_PORT       (NSM_BRIDGE_ERR_GMRP_BASE + 7)
#define NSM_GMRP_ERR_GMRP_NOT_CFG_ON_PORT_VLAN  (NSM_BRIDGE_ERR_GMRP_BASE + 8)

#define GMRP_ALLOC_PORT_CONFIG(Z,I,V,A)                                       \
    do {                                                                      \
      if ((Z) == NULL)                                                        \
        {                                                                     \
          (Z) = XCALLOC (MTYPE_GMRP_PORT_CONFIG,                              \
                sizeof (struct gmrp_port_config));                            \
          if (Z)                                                              \
            {                                                                 \
              (I) = (V);                                                      \
              cli_out (cli, "%%Interface %s not bound to bridge\n", (A));     \
              return CLI_ERROR;                                               \
            }                                                                 \
          else                                                                \
            {                                                                 \
              return CLI_ERROR;                                               \
            }                                                                 \
        }                                                                     \
      else                                                                    \
        {                                                                     \
           (I) = (V);                                                         \
           cli_out (cli, "%%Interface %s not bound to bridge\n", (A));        \
           return CLI_ERROR;                                                  \
        }                                                                     \
    } while (0)

#define MMRP_POINT_TO_POINT_ENABLE 1
#define MMRP_POINT_TO_POINT_DISABLE 0

#define CHECK_XMRP_IS_RUNNING(X,R)                                            \
   do {                                                                       \
        if ((X->gmrp_bridge->reg_type == REG_TYPE_GMRP) &&                    \
            (!pal_strncmp(R, "mmrp", 4)))                                     \
          {                                                                   \
            cli_out(cli, "can't run mmrp command on gmrp enabled "            \
                    "bridge %s\n", X->name);                                  \
            return CLI_ERROR;                                                 \
          }                                                                   \
        else if ((X->gmrp_bridge->reg_type == REG_TYPE_MMRP) &&               \
                 (!pal_strncmp(R, "gmrp", 4)))                                \
          {                                                                   \
            cli_out(cli, "can't run gmrp command on mmrp enabled "            \
                    "bridge %s\n", X->name);                                  \
            return CLI_ERROR;                                                 \
          }                                                                   \
   } while (0)

#define GMRP_REG_TYPE_TO_STR(REG_TYPE) ((REG_TYPE) == REG_TYPE_GMRP) ?        \
                                       "gmrp" : "mmrp"                        \

/* Install 802.1D GMRP related CLI.  */
void
gmrp_cli_init (struct lib_globals *gmrpm);

#endif
