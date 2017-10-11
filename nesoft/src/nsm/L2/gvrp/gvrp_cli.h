/*   Copyright (C) 2003  All Rights Reserved.  */

#ifndef __PACOS_GVRP_CLI_H__
#define __PACOS_GVRP_CLI_H__

/* 802.1D GVRP
   This module declares the interface to GVRP.  */
#define NSM_BRIDGE_ERR_GVRP_BASE     -50
#define NSM_BRIDGE_ERR_GVRP_NOCONFIG (NSM_BRIDGE_ERR_GVRP_BASE + 1)
#define NSM_ERR_GVRP_NOCONFIG_ONPORT (NSM_BRIDGE_ERR_GVRP_BASE + 2)
#ifdef HAVE_PVLAN
#define NSM_PVLAN_ERR_CONFIGURED     (NSM_BRIDGE_ERR_GVRP_BASE + 3)
#endif /* HAVE_PVLAN */

#define GVRP_ALLOC_PORT_CONFIG(Z,I,V,A)                                       \
    do {                                                                      \
      if ((Z) == NULL)                                                        \
        {                                                                     \
          (Z) = XCALLOC (MTYPE_GVRP_PORT_CONFIG,                              \
                sizeof (struct gvrp_port_config));                            \
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

#define MVRP_POINT_TO_POINT_ENABLE  1
#define MVRP_POINT_TO_POINT_DISABLE 0

#define CHECK_XVRP_IS_RUNNING(X,R)                                            \
   do {                                                                       \
        if ((X->gvrp->reg_type == REG_TYPE_GVRP) &&                           \
            (!pal_strncmp (R, "mvrp", 4)))                                    \
          {                                                                   \
            cli_out (cli,"can't run mvrp command on gvrp "                    \
                     "enabled bridge %s\n", X->name);                         \
            return CLI_ERROR;                                                 \
          }                                                                   \
        else if ((X->gvrp->reg_type == REG_TYPE_MVRP) &&                      \
                 (!pal_strncmp (R, "gvrp", 4)))                               \
          {                                                                   \
            cli_out (cli, "can't run gvrp command on mvrp "                   \
                     "enabled bridge %s\n", X->name);                         \
            return CLI_ERROR;                                                 \
          }                                                                   \
      } while (0)

#define GVRP_REG_TYPE_TO_STR(REG_TYPE) ((REG_TYPE) == REG_TYPE_GVRP) ?        \
                                       "gvrp" : "mvrp"                        \

/* Install 802.1D GVRP related CLI.  */
void
gvrp_cli_init (struct lib_globals *gvrpm);
extern void
gvrp_port_activate (struct nsm_bridge_master *master, struct interface *ifp);

#endif
