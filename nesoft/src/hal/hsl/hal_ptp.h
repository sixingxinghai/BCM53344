/******************************************************************************
  文 件 名    : ptp.h
  版 本 号    : 初稿
  作    者   	:deng
  生成日期 : 2012年7月
  功能描述 : ptp 模块内部实现的头文件
  函数列表 :
  修改历史 :
  ******************************************************************************/
#ifndef _HAL_PTP_H_
#define _HAL_PTP_H_

#ifndef HAL_MSG_BASE_MSG
#define HAL_MSG_BASE_MSG(n)                         (100 + (n))
#endif


#define HAL_MSG_PTP_BASE                           HAL_MSG_BASE_MSG(1750)
#define HAL_MSG_PTP_MSG(n)                        (HAL_MSG_PTP_BASE  + (n))
#define HAL_MSG_PTP_CLK_TYPE_SET	                           HAL_MSG_PTP_MSG(1)
#define HAL_MSG_PTP_DELAY_MECH_SET	                           HAL_MSG_PTP_MSG(3)
#define HAL_MSG_PTP_PHYPORT_SET	                           HAL_MSG_PTP_MSG(4)
#define HAL_MSG_PTP_PHYPORT_CLEAR                          HAL_MSG_PTP_MSG(5)
#define HAL_MSG_PTP_PORT_ENABLE                          HAL_MSG_PTP_MSG(6)
#define HAL_MSG_PTP_PORT_DISABLE                          HAL_MSG_PTP_MSG(7)
#define HAL_MSG_PTP_PORT_DELAY_MECH                          HAL_MSG_PTP_MSG(8)
#define HAL_MSG_PTP_REQ_INTERVAL                          HAL_MSG_PTP_MSG(9)
#define HAL_MSG_PTP_P2P_INTERVAL                          HAL_MSG_PTP_MSG(10)
#define HAL_MSG_PTP_P2P_MEANDELAY                          HAL_MSG_PTP_MSG(11)
#define HAL_MSG_PTP_P2P_ASYMDELAY                          HAL_MSG_PTP_MSG(12)
#define HAL_MSG_PTP_DOMAIN_CREATE	                           HAL_MSG_PTP_MSG(13)
#define HAL_MSG_PTP_DOMAIN_MPORT_ADD	                           HAL_MSG_PTP_MSG(14)
#define HAL_MSG_PTP_DOMAIN_SPORT_ADD                           HAL_MSG_PTP_MSG(15)


struct hal_msg_ptp_clk_type
{
   unsigned int type;
};

struct hal_msg_ptp_domain_port
{
    unsigned int domain;
	unsigned int port;
};

struct hal_msg_ptp_delay_mech
{
    unsigned int delay_mech;
};

struct hal_msg_ptp_phyport
{
    unsigned int vport;
	unsigned int phyport;
};

struct hal_msg_ptp_port_delay_mech
{
    unsigned int vport;
    unsigned int delay_mech;
};

struct hal_msg_ptp_req_interval
{
    unsigned int vport;
    int interval;
};

struct hal_msg_p2p_delay
{
    unsigned int vport;
    int delay;
};


#endif
