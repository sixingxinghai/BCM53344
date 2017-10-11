/* Copyright (C) 2001-2003  All Rights Reserved. 


        This module contains the global daemon configuration data.
*/

#ifndef _PACOS_MSTPD_H
#define _PACOS_MSTPD_H

/* Included files */
#include "lib.h"

/* MSTP port number. */
#define MSTP_PORT_DEFAULT               525

/* Extern variables. */
extern struct lib_globals *mstpm;

/* extern procedures */
extern int mstp_nsm_send_port_state (u_int32_t ifindex, int port_state);

extern int mstp_nsm_send_ageing_time (const char *const br_name,
                                      s_int32_t ageing_time);

extern const char bridge_grp_add[];

extern const char pro_bridge_grp_add[];

extern const char rpvst_bridge_grp_addr[];

extern const char pro_backbone_bridge_oui[];

#define MSTP_IS_CUSTOMER_BPDU(ADDR)                                \
                      (pal_mem_cmp (ADDR, bridge_grp_add,          \
                                    ETHER_ADDR_LEN) == 0)

#define MSTP_IS_RPVST_PLUS_BPDU(ADDR)                              \
                      (pal_mem_cmp (ADDR, rpvst_bridge_grp_addr,   \
                                    ETHER_ADDR_LEN) == 0)


#define MSTP_IS_PROVIDER_BPDU(ADDR)                                \
                      (pal_mem_cmp (ADDR, pro_bridge_grp_add,      \
                                    ETHER_ADDR_LEN) == 0)

#define MSTP_IS_PBB_BPDU(ADDR)                                       \
                      (pal_mem_cmp (ADDR, pro_backbone_bridge_oui,   \
                                    3) == 0)

#endif /* _PACOS_MSTPD_H */
