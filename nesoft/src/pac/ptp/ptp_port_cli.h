/******************************************************************************
 *
 *  pac/ptp/ptp_port_cli.h
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Deng Jingen <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  ptp_port(IEEE1588精确时间协议端口配置)CLI头文件，创建于2012.7.5
 *
 */

#ifndef _PAC_PTP_PORT_CLI_H_
#define _PAC_PTP_PORT_CLI_H_

/******************************************************************************
 *
 *  枚举/宏定义
 *
 *****************************************************************************/
#define MAX_PHYPORT_NUMBER 28
#define MAX_PKT_INTERVAL 4
#define MIN_PKT_INTERVAL -4
#define MAX_MEAN_DELAY 1000000000
#define MAX_ASYM_DELAY  500000000
#define MIN_ASYM_DELAY  -500000000

#define IS_PTP_PORT_INDEX_VALID(id) ((0 <= id) && (id < PTP_MAX_PORT_NUMBER))
#define IS_PTP_MPORT_INDEX_VALID(id) ((0 <= id) && (id < PTP_MAX_MPORT_NUMBER))
#define IS_PTP_SPORT_INDEX_VALID(id) ((PTP_MAX_MPORT_NUMBER <= id) && (id < PTP_MAX_PORT_NUMBER))
#define IS_PTP_PHYPORT_VALID(id)  ((0 <= id) && (id < MAX_PHYPORT_NUMBER)) 
#define IS_PTP_PKT_INTERVAL_VALID(id)  ((MIN_PKT_INTERVAL <= id) && (id <= MAX_PKT_INTERVAL))
#define IS_P2P_MEAN_DELAY_VALID(id) ((0 < id) && (id <= MAX_MEAN_DELAY))
#define IS_PTP_ASYM_DELAY_VALID(id)  ((MIN_ASYM_DELAY <= id) && (id <= MAX_ASYM_DELAY))


/******************************************************************************
 *
 *  函数接口
 *
 *****************************************************************************/


#endif
