/* Copyright (C) 2003. All Rights Reserved. */
#ifndef _NSM_HW_REG_API_H
#define _NSM_HW_REG_API_H

void nsm_hw_reg_init (struct cli_tree *ctree);
#ifdef HAVE_HAL
int nsm_hw_reg_get (u_int32_t addr, struct hal_reg_addr *reg);
int nsm_hw_flow_ctrl_set (struct hal_flow_control_wm *msg);
#endif /* HAVE_HAL */
int nsm_hw_reg_set (u_int32_t addr, u_int32_t value);
int nsm_hw_reg_cli_return (struct cli *cli, int ret);
#endif /* _NSM_HW_REG_API_H */

