/* Copyright (C) 2012  All Rights Reserved.

L3 HAL
  like stub for pc compile

*/

#include "pal.h"
#include "lib.h"

#include "hal_types.h"
#include "hal_error.h"

int
hal_igmp_snooping_delete_entry (char *bridge_name,
                                struct hal_in4_addr *src,
                                struct hal_in4_addr *group,
                                char is_exclude,
                                int vid,
                                int svid,
                                int count,
                                u_int32_t *ifindexes)
{
 return HAL_SUCCESS;
}

int
hal_igmp_snooping_add_entry (char *bridge_name,
                             struct hal_in4_addr *src,
                             struct hal_in4_addr *group,
                             char is_exclude,
                             int vid,
                             int svid,
                             int count,
                             u_int32_t *ifindexes)
{
 return HAL_SUCCESS;
}

int
hal_igmp_snooping_if_enable (char *name, unsigned int ifindex)
{
 return HAL_SUCCESS;
}

int
hal_igmp_snooping_disable (char *bridge_name)
{
 return HAL_SUCCESS;
}

int
hal_igmp_snooping_enable (char *bridge_name)
{
 return HAL_SUCCESS;
}

