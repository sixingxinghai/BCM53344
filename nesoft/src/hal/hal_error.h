/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_ERROR_H_
#define _HAL_ERROR_H_

#define HAL_SUCCESS                                 0

#define HAL_ERR_BASE                                -200
#define HAL_ERR_PMIRROR_SET                         (HAL_ERR_BASE + 1)
#define HAL_ERR_PMIRROR_UNSET                       (HAL_ERR_BASE + 2)
#define HAL_ERR_FLOW_CONTROL_SET                    (HAL_ERR_BASE + 3)
#define HAL_ERR_FLOW_CONTROL_UNSET                  (HAL_ERR_BASE + 4)
#define HAL_ERR_L2_BCAST_STORM_SUPPRESS             (HAL_ERR_BASE + 5)

#define HAL_ERR_NOT_SUPPORTED                       (HAL_ERR_BASE + 6)
#define HAL_ERR_INVALID_ARG                         (HAL_ERR_BASE + 7)

#define HAL_ERR_QOS_BASE                            -300
#define HAL_ERR_QOS_INVALID_RATE_STEP              (HAL_ERR_QOS_BASE + 1) 
#define HAL_ERR_QOS_INVALID_EBS                    (HAL_ERR_QOS_BASE + 2) 
#define HAL_ERR_QOS_INVALID_CBS                    (HAL_ERR_QOS_BASE + 3) 
#define HAL_ERR_BRIDGE_BASE                         -400
#define HAL_ERR_BRIDGE_VLAN_RESERVED_IN_HW         (HAL_ERR_BRIDGE_BASE - 9)

#endif /* _HAL_ERROR_H_ */
