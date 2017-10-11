/* Copyright (C) 2004  All Rights Reserved. */
#ifndef _HAL_HW_REG_H_
#define _HAL_HW_REG_H_

int hal_hw_reg_get (u_int32_t reg_addr, struct hal_reg_addr *reg);
int hal_hw_reg_set (u_int32_t reg_addr, u_int32_t value);
int hal_hw_reg_parse (struct hal_nlmsghdr *h, void *data);

#endif /*_HAL_HW_REG_H_*/
