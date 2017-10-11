/******************************************************************************
 *
 *  pac/ptp/ptp_cli.h
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Deng Jingen <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  ptp(IEEE1588精确时间协议全局模式)CLI头文件，创建于2012.7.5
 *
 */

#ifndef _PAC_PTP_CLI_H_
#define _PAC_PTP_CLI_H_

#include "comm.h"

/******************************************************************************
 *
 *  枚举/宏定义
 *
 *****************************************************************************/
#define PTP_MAX_DOMAIN_NUMBER 256
#define PTP_MAX_PORT_NUMBER 20
#define PTP_MAX_MPORT_NUMBER 2
#define PTP_PHYPORT_INVALID_FLAG -1
#define PTP_INIT_FLAG 1
#define PTP_NO_INIT_FLAG 0
#define PTP_BITMAP_SPORT_OFFSET 4
#define P2P_INIT_MEAN_DELAY  620





enum em_tx_frame_type
{
    PTP_L2,
	PTP_L3
};

enum em_clock_type
{
    PTP_TC,
	PTP_OC,
	PTP_BC
};

enum em_delay_mech
{
    PTP_E2E = 1,
	PTP_P2P
};

enum em_port_status
{
    PTP_MASTER,
	PTP_SLAVE,
	PTP_PASSIVE
};

enum em_port_state_mech
{
    PTP_BCM,
	PTP_MANNUL
};

struct ptp_port_config
{
    //UINT8  phy_card;//只有一个CPU
	int  phyport;//0-23, -1为没有绑定
	BOOL   enable;//enable virtual port, FALSE = 0, TURE != 0

	//enum  em_tx_frame_type  frame_type;//只支持l2帧
	//enum  em_clock_type clock_type;//暂定不考虑混合型时钟

	/*used for general clock,including transparent clock*/
	//UINT8  domain_number;//0-255， 在全局变量设置
	BOOL domain_set;
	int  asymmetry_delay;//<-500000000 - 500000000>
	enum em_delay_mech  delay_mech;//<1-2>  1 = E2E; 2 = P2P
	CHAR  p2p_req_interval;//log2 of interval. between -128 and 127
	UINT32  p2p_mean_delay;//ns of path delay. <0-1000000000>
	CHAR  delay_req_interval;//log2 of interval. between -128 and 127
		 
	/*used for oc or bc*/
    enum em_port_status    port_status;//master、slave、passive
	CHAR  sync_interval;//log2 of interval. between -128 and 127
	UINT8  time_out;//announce receipt <time_out>, log2 of interval. <0-255>
	CHAR  announce_interval;//log2 of interval. between -128 and 127
	UINT8  clock_class;//<0-255>
	enum em_port_state_mech	  port_state_mech;//bcm、manual
	UINT8	 priority1;//0-255
	UINT8	 priority2;//0-255
	int 	scale_log_var;//<-2147483648 - 2147483647>	   
	enum em_delay_mech slaveonly_delay_mech;//e2e  time delay mech End-to-end; p2p  time delay mech Peer-to-peer		   
};

#define IS_PTP_DOMAIN_VALID(id) ((0 <= id) && (id < PTP_MAX_DOMAIN_NUMBER))
#define IS_PTP_BITMAP_VALID(id) ptp_bitmap_valid(id)

	
extern struct ptp_port_config port_config_data[PTP_MAX_PORT_NUMBER];//时钟端口最多为20个，在所有的28个以太网中选择


/******************************************************************************
 *
 *  函数接口
 *
 *****************************************************************************/
BOOL ptp_bitmap_valid(UINT32);


#endif
