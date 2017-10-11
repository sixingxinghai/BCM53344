/**@file elmid.h
 * @brief  This file contains constant definitions and macros for ELMI base 
 * module.
 */
/* Copyright (C) 2010  All Rights Reserved.  */

#ifndef __ELMI_H__
#define __ELMI_H__

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */
#include "L2/l2lib.h"

extern struct lib_globals *elmi_zg;
#define ZG      (elmi_zg)

extern const char elmi_dest_addr[];

void 
elmi_nsm_init (struct lib_globals * zg);

s_int32_t 
elmi_master_init (struct apn_vr *);

s_int32_t 
elmi_master_finish (struct apn_vr *);

void 
elmi_bridge_terminate (struct lib_globals *);

void 
elmi_terminate (struct lib_globals *);

void 
elmi_cli_init (struct lib_globals *);

struct elmi_master *
elmi_master_get (void);

void 
elmi_init (struct lib_globals *);

void 
elmi_onm_init (struct lib_globals *);

void elmi_nsm_debug_set (struct elmi_master *);
void elmi_nsm_debug_unset (struct elmi_master *);

void
elmi_debug_init (struct elmi_master *);

#define ELMI_DBG_FN_DESC_MAX                    (32)

#define ELMI_FN_DESC(FN_DESC)                                        \
      (# FN_DESC == "" ? __FUNCTION__ : # FN_DESC)

#define ELMI_DBG_FN_DESC                dbg_fn_desc
#ifdef HAVE_ISO_MACRO_VARARGS
#define ELMI_FN_ENTER(...)                                           \
      u_int8_t ELMI_DBG_FN_DESC [ELMI_DBG_FN_DESC_MAX];              \
    pal_strncpy (ELMI_DBG_FN_DESC, ELMI_FN_DESC (__VA_ARGS__),       \
                         ELMI_DBG_FN_DESC_MAX - 1);                  \
    ELMI_DBG_FN_DESC [ELMI_DBG_FN_DESC_MAX - 1] = '\0';
#define ELMI_FN_EXIT(...)                                            \
      ELMI_DBG_FN_DESC [0] = '\0';                                   \
    return __VA_ARGS__;
#else
#define ELMI_FN_ENTER(ARGS...)                                       \
      u_int8_t ELMI_DBG_FN_DESC [ELMI_DBG_FN_DESC_MAX];              \
    pal_strncpy (ELMI_DBG_FN_DESC, ELMI_FN_DESC (ARGS),              \
                         ELMI_DBG_FN_DESC_MAX - 1);                  \
    ELMI_DBG_FN_DESC [ELMI_DBG_FN_DESC_MAX - 1] = '\0';
#define ELMI_FN_EXIT(ARGS...)                                        \
      ELMI_DBG_FN_DESC [0] = '\0';                                   \
    return ARGS;
#endif

#define ELMI_PREFIX_VLAN_SET(P,I)                                      \
     do {                                                              \
               pal_mem_set ((P), 0, sizeof (struct elmi_prefix_vlan)); \
               (P)->family = AF_INET;                                  \
               (P)->prefixlen = 32;                                    \
               (P)->vid = pal_hton32 (I);                              \
            } while (0)

#define ELMI_VLAN_INFO_SET(R,V)                                        \
  do {                                                                 \
      (R)->info = (V);                                                 \
      route_lock_node (R);                                             \
  } while (0)

#define ELMI_VLAN_INFO_UNSET(R)                                        \
  do {                                                                 \
      (R)->info = NULL;                                                \
      route_unlock_node(R);                                            \
  } while (0)

#define ELMI_IS_VALID_DEST_MAC(ADDR)                                   \
  (pal_mem_cmp (ADDR, elmi_dest_addr,                                  \
                ETHER_ADDR_LEN) == 0)
#endif /* __ELMI_H__ */
