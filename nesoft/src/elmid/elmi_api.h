/**@file elmi_api.h
 * @brief  This file contains the prototypes for elmi_api.c file.
 */
/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_API_H
#define _PACOS_ELMI_API_H

#include "nsm_client.h"
#include "elmi_bridge.h"
#include "elmi_types.h"

#define ELMI_MAX_EXPSIZE         7      /* Valid exponent range is 0 to 6 */
#define ELMI_UINT16_MAXFRACT     65535  /* Maximum fractional value. */
#define ELMI_UINT8_MAXFRACT      255    /* Maximum fractional value. */
#define ELMI_BIT_BASE            10     /* Maximum fractional value. */


s_int32_t
elmi_cli_return (struct cli *cli, s_int32_t ret);

s_int32_t
elmi_api_set_port_polling_time (u_int8_t *ifName,
                                u_int8_t polling_time);
s_int32_t
elmi_api_set_port_polling_verification_time (u_int8_t *ifName,
                                             u_int8_t polling_ver_time);

s_int32_t
elmi_api_set_port_polling_counter (u_int8_t *ifName,
                                   u_int16_t polling_counter);
s_int32_t
elmi_api_set_port_status_counter (u_int8_t *ifName,
                                  s_int8_t status_counter);

s_int32_t
elmi_api_set_port_async_min_time (u_int8_t *ifName,
                                  u_int8_t async_time_interval);
s_int32_t
elmi_api_get_port_status_counter (u_int8_t *ifName,
                                  s_int8_t *status_counter);

s_int32_t
elmi_api_enable_bridge_global (u_int8_t *br_name);

s_int32_t
elmi_api_disable_bridge_global (u_int8_t *br_name);

s_int32_t
elmi_api_enable_port (u_int32_t ifindex);

s_int32_t
elmi_api_disable_port (u_int32_t ifindex);


u_int8_t
elmi_get_local_evc_status (u_int8_t evc_status_type);

struct elmi_cvlan_evc_map *
elmi_lookup_cevlan_evc_map (struct elmi_ifp *elmi_if, u_int16_t evc_ref_id);

struct elmi_evc_status *
elmi_evc_look_up (struct elmi_ifp *elmi_if, u_int16_t evc_ref_id);

struct elmi_evc_status *
elmi_lookup_evc_by_name (struct elmi_ifp *elmi_if, u_int8_t *evc_name);

void
elmi_get_matissa_exponent_16bit (u_int32_t num, u_int16_t *multiplier,
                                 u_int8_t *magnitude);

void
elmi_get_matissa_exponent_8bit (u_int32_t num, u_int8_t *multiplier,
                                u_int8_t *magnitude);

enum cvlan_evc_map_type
elmi_get_map_type (enum elmi_nsm_uni_service_attr map_service_attr);
#endif /* _PACOS_ELMI_API_H */
