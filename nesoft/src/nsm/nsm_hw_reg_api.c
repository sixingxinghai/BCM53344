/* Copyright (C) 2003, Inc. All Rights Reserved. */
#include "pal.h"
#include "lib.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#include "hal/hsl/hal_msg.h"
#include "hal/hsl/hal_netlink.h"
#include "hal_hw_reg.h"
#endif /* HAVE_HAL */
#include "nsm/nsm_hw_reg_api.h"

#ifdef HAVE_HAL
int
nsm_hw_reg_get (u_int32_t reg_addr, struct hal_reg_addr *reg)
{
  int ret;

  if ((ret = hal_hw_reg_get (reg_addr, reg)) != HAL_SUCCESS)
    return ret;

  return 0;
}

int
nsm_hw_reg_set (u_int32_t reg_addr, u_int32_t value)
{
  int ret;

  if ((ret = hal_hw_reg_set (reg_addr, value)) != HAL_SUCCESS)
    return ret;

  return 0;
}

#endif /* HAVE_HAL */
