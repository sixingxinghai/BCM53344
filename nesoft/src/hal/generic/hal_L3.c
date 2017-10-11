/* Copyright (C) 2012  All Rights Reserved.

L3 HAL
  like stub for pc compile

*/

#include "pal.h"
#include "lib.h"

#include "hal_types.h"
#include "hal_error.h"

int
hal_if_bind_fib (u_int32_t ifindex, u_int32_t fib)
{
 return HAL_SUCCESS;
}

int
hal_if_unbind_fib (u_int32_t ifindex, u_int32_t fib)
{
 return HAL_SUCCESS;
}

hal_fib_create (unsigned int fib)
{
 return HAL_SUCCESS;
}
int
hal_fib_delete (unsigned int fib)
{
 return HAL_SUCCESS;
}
