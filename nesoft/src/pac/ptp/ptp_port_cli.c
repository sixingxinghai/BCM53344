/******************************************************************************
 *
 *  pac/ptp/ptp_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Deng Jingen <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  ptp_port(IEEE1588精确时间协议端口模式， 目前只实现透传时钟)CLI部分，创建于2012.7.5
 *
 */

#include "pal.h"
#include "lib.h"

#include "avl_tree.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"

#include "ptp_port_cli.h"
#include "ptp_cli.h"

#include "hsl/hal_ptp.h"

UINT32 ptp_vport;

void hal_ptp_set_phyport (UINT32 vport, UINT32 phyport)
{
    struct hal_msg_ptp_phyport phyport_msgdata = {0};
	phyport_msgdata.vport = vport;
	phyport_msgdata.phyport = phyport;
	comm_send_hal_msg(HAL_MSG_PTP_PHYPORT_SET,&phyport_msgdata,sizeof(phyport_msgdata), NULL, NULL);
}

void hal_ptp_clear_phyport (UINT32 vport)
{
    comm_send_hal_msg(HAL_MSG_PTP_PHYPORT_CLEAR,&vport,sizeof(vport), NULL, NULL);
}

void hal_ptp_port_enable (UINT32 vport)
{
	comm_send_hal_msg(HAL_MSG_PTP_PORT_ENABLE,&vport,sizeof(vport), NULL, NULL);
}

void hal_ptp_port_disable (UINT32 vport)
{
	comm_send_hal_msg(HAL_MSG_PTP_PORT_DISABLE,&vport,sizeof(vport), NULL, NULL);
}

void hal_ptp_set_port_delay_mech(UINT32 vport, enum em_delay_mech mech)
{
    struct hal_msg_ptp_port_delay_mech  mech_msgdata ={0};
	mech_msgdata.vport = vport;
	mech_msgdata.delay_mech = mech;
    comm_send_hal_msg(HAL_MSG_PTP_PORT_DELAY_MECH,&mech_msgdata,sizeof(mech_msgdata), NULL, NULL);
}

void hal_ptp_set_req_interval (UINT32 vport, int num)
{
	struct hal_msg_ptp_req_interval  interval_msgdata ={0};
	interval_msgdata.vport = vport;
	interval_msgdata.interval = num;
    comm_send_hal_msg(HAL_MSG_PTP_REQ_INTERVAL, &interval_msgdata,sizeof(interval_msgdata), NULL, NULL);
}

void hal_ptp_set_p2p_interval (UINT32 vport, int num)
{
	struct hal_msg_ptp_req_interval  interval_msgdata ={0};
	interval_msgdata.vport = vport;
	interval_msgdata.interval = num;
    comm_send_hal_msg(HAL_MSG_PTP_P2P_INTERVAL, &interval_msgdata,sizeof(interval_msgdata), NULL, NULL);
}    

void hal_ptp_set_p2p_mean_delay (vport, num)
{
    struct hal_msg_p2p_delay  delay_msgdata ={0};
	delay_msgdata.vport = vport;
	delay_msgdata.delay = num;
    comm_send_hal_msg(HAL_MSG_PTP_P2P_MEANDELAY, &delay_msgdata,sizeof(delay_msgdata), NULL, NULL);
}    

void hal_ptp_set_asym_delay (vport, num)
{
    struct hal_msg_p2p_delay  delay_msgdata ={0};
	delay_msgdata.vport = vport;
	delay_msgdata.delay = num;
    comm_send_hal_msg(HAL_MSG_PTP_P2P_ASYMDELAY, &delay_msgdata,sizeof(delay_msgdata), NULL, NULL);
}    

/*
 * 创建ptp_port模式，(config-ptp-port)
 */
CLI(ptp_port_id,
    ptp_port_id_cmd,
    "port <0-19>",
    CLI_PTP_PORT_MODE_STR,
    "ptp port index")
{
	UINT32 index = atoi(argv[0]);
	
	if (!IS_PTP_PORT_INDEX_VALID(index))
		{
			 cli_out(cli, "%% the port id is not validate.\n");
			 return CLI_ERROR;
		}
	
		/*pw = vport_find_pw_by_index(index);
		if (NULL == pw)
		{
			cli_out(cli, "%% pseudo wire %s does not exist, create it now.\n", argv[0]);
	
			pw = vport_new_pw(index);
			if (NULL == pw)
			{
				cli_out(cli, "%% create pseudo wire fail.\n");
				return CLI_ERROR;
			}
		}*/
	 ptp_vport = index;
     cli->index = &ptp_vport;
     cli->mode = PTP_PORT_MODE;

     return CLI_SUCCESS;
}

/*
 * 把某个portindex和物理端口进行绑定
 * 在所有的28个以太网中选择，bit0-27
 */
CLI(ptp_port_bind_config,
    ptp_port_bind_config_cmd,
    "bind (fe <1-24>|ge <1-4>)",
    "bind ethernet interface",
    "interface type",
    "fe interface id",
    "interface type",
    "ge interface id")
{
    UINT32 vport,phyport;
	int i,j;
	
	if (0 == pal_strcmp("fe", argv[0]))
        phyport = atoi(argv[1]) - 1;
	else
		phyport = atoi(argv[1]) + 23;

	if ( !IS_PTP_PHYPORT_VALID(phyport))
	{
        cli_out(cli, "the bitmap should be between 0-27.\n");
        return CLI_ERROR;
    }

    vport = *(UINT32 *)(cli->index);

	for (i=0; i < PTP_MAX_PORT_NUMBER; i++)
	{
	    if (port_config_data[i].phyport == phyport)
		{
			cli_out(cli, "the phyport has been bound to vport %d already.\n",i);
            return CLI_ERROR;
	    }
	}
	if (port_config_data[vport].enable == 1)
	{
		cli_out(cli, "the vport %d must be set to no-enable before to rebind phyport %d .\n",vport,phyport);
        return CLI_ERROR;
	}
	
    hal_ptp_set_phyport (vport, phyport);
	port_config_data[vport].phyport = phyport;

    zlog_info(NSM_ZG, "set vport %d phyport %d.", vport, port_config_data[vport].phyport);

    return CLI_SUCCESS;
}

/*
 * 把某个portindex和物理端口进行解绑定
 */
CLI(ptp_port_unbind_config,
    ptp_port_unbind_config_cmd,
    "unbind",
    "unbind ethernet interface")
{
    UINT32 vport;
	vport = *(UINT32 *)(cli->index);
    hal_ptp_clear_phyport (vport);
	
	port_config_data[vport].phyport = PTP_PHYPORT_INVALID_FLAG;

    zlog_info(NSM_ZG, "unbind vport %d's phyport.", vport);

    return CLI_SUCCESS;
}

/*
 * 端口使能
 */
CLI(ptp_port_enable_config,
    ptp_port_enable_config_cmd,
    "enable-ptp",
    "enable port's ptp function")
{
	UINT32 vport;
	vport = *(UINT32 *)(cli->index);
    if( port_config_data[vport].domain_set == 0)
    {
    	cli_out(cli, "The port hasn't set domain.\n");
        return CLI_ERROR;
    }
	if( port_config_data[vport].phyport == PTP_PHYPORT_INVALID_FLAG)
    {
    	cli_out(cli, "The port hasn't set phyport.\n");
        return CLI_ERROR;
    }
	hal_ptp_port_enable (vport);
	port_config_data[vport].enable = 1;
	
	zlog_info(NSM_ZG, "set vport %d enable.", vport);
    return CLI_SUCCESS;
}

/*
 * 取消端口使能
 */
CLI(ptp_port_disable_config,
    ptp_port_disable_config_cmd,
    "no-enable",
    "disable port")
{
    UINT32 vport;
	vport = *(UINT32 *)(cli->index);
	hal_ptp_port_disable (vport);
		
	port_config_data[vport].enable = 0;
	
	zlog_info(NSM_ZG, "set vport %d disable.", vport);

    return CLI_SUCCESS;
}

/*
 * 设置传输时延测量机制
 * 1 = E2E; 2 = P2P
 */
CLI(delaymech_config,
    delaymech_config_cmd,
    "set-delay-mechanism (e2e|p2p)",
    "set delay mechanism",
    "end to end",
    "peer to peer")
{
    UINT32 vport;
    enum em_delay_mech type = (0 == pal_strcmp("p2p",argv[0])) ? PTP_P2P : PTP_E2E;
	vport = *(UINT32 *)(cli->index);
	hal_ptp_set_port_delay_mech (vport,type);
    port_config_data[vport].delay_mech = type;
	zlog_info(NSM_ZG, "set vport %d delay mech %d.", vport, port_config_data[0].delay_mech);

    return CLI_SUCCESS;
}

/*
 * 设置用于测量传输时延的请求响应报文的发送间隔
 * 用于E2E透传时钟
 */
CLI(delay_request_interval_config,
    delay_request_interval_config_cmd,
    "set-delay-req-interval INTERVAL",
    "set delay request interval",
    "log interval value, range -128 - 127")
{
    UINT32 vport;
    int num = atoi (argv[0]);

	if ( !IS_PTP_PKT_INTERVAL_VALID(num))
	{
        cli_out(cli, "the delay request interval should between 1 and 256.\n");
        return CLI_ERROR;
    }
	vport = *(UINT32 *)(cli->index);
    hal_ptp_set_req_interval (vport, num);
	port_config_data[vport].delay_req_interval = num;

    zlog_info(NSM_ZG, "set vport %d delay request interval %d.", vport, port_config_data[vport].delay_req_interval);

    return CLI_SUCCESS;
}

/*
 * 设置p2p延时测量方式的平均延迟时间
 */
CLI(p2p_meanpath_delay_config,
    p2p_meanpath_delay_config_cmd,
    "set-p2p-mean-delay  <0-1000000000>",
    "set p2p meanpath delay",
    "p2p meanpath delay value, range 0 - 1000000000")
{
    UINT32 vport;
    UINT32 num = atoi (argv[0]);

	if ( !IS_P2P_MEAN_DELAY_VALID(num))
	{
        cli_out(cli, "the p2p mean delay should between 0 and 1000000000 ns.\n");
        return CLI_ERROR;
    }
	vport = *(UINT32 *)(cli->index);
    hal_ptp_set_p2p_mean_delay (vport, num);
	port_config_data[vport].p2p_mean_delay = num;

    zlog_info(NSM_ZG, "set vport %d p2p mean delay %d.", vport, port_config_data[vport].p2p_mean_delay);

    return CLI_SUCCESS;
}

/*
 * 设置p2p方式的请求报文发送间隔
 * INTERVAL为发送间隔以2为底的对数值
 */
CLI(p2p_request_interval_config,
    p2p_request_interval_config_cmd,
    "set-p2p-req-interval INTERVAL",
    "set p2p request interval",
    "log p2p request interval, range -4 - 4")
{
    UINT32 vport;
    int num = atoi (argv[0]);

	if ( !IS_PTP_PKT_INTERVAL_VALID(num))
	{
        cli_out(cli, "the p2p request interval should between -4 and 4.\n");
        return CLI_ERROR;
    }
	vport = *(UINT32 *)(cli->index);
    hal_ptp_set_p2p_interval (vport, num);
	port_config_data[vport].p2p_req_interval = num;

    zlog_info(NSM_ZG, "set vport %d p2p request interval %d.", vport, port_config_data[vport].p2p_req_interval);

    return CLI_SUCCESS;
}

/*
 * 配置链路往返延时非对称性
 */
CLI(asymmetrydelay_config,
    asymmetrydelay_config_cmd,
    "set-asymdelay INTERVAL",
    "set asymmetry delay",
    "asymmetry delay value, range -500000000 - 500000000")
{
    UINT32 vport;
	int num = atoi (argv[0]);

	if ( !IS_PTP_ASYM_DELAY_VALID(num))
	{
        cli_out(cli, "the asymmetry delay should between -500000000 and 500000000 ns.\n");
        return CLI_ERROR;
    }
	vport = *(UINT32 *)(cli->index);
    hal_ptp_set_asym_delay (vport, num);
	port_config_data[vport].asymmetry_delay = num;

    zlog_info(NSM_ZG, "set vport %d asymmetry delay %d.", vport, port_config_data[vport].asymmetry_delay);

    return CLI_SUCCESS;
}

/*
 *设置sync报文发送间隔
 */
CLI(sync_interval_config,
    sync_interval_config_cmd,
    "set-sync-interval INTERVAL",
    "set sync interval",
    "sync interval value, range -128 - 127")
{


    return CLI_SUCCESS;
}


/*
 * 设置announce报文的间隔时间
 */
CLI(announce_interval_config,
    announce_interval_config_cmd,
    "set-announce-interval INTERVAL",
    "set announce interval",
    "announce interval, range <-128 - 127>")
{


    return CLI_SUCCESS;
}

/*
 * 设置announce报文的接收超时时间
 */
CLI(announce_receipt_config,
    announce_receipt_config_cmd,
    "set-announce-receipt <0 - 255>",
    "set announce receipt timeout",
    "announce receipt timeout, range <0 - 255>")
{


    return CLI_SUCCESS;
}

/*
 * 设置端口状态
 */
CLI(port_status_config,
    port_status_config_cmd,
    "set-port-status STATUS",
    "set port status",
    "port status, range <master slave passive>")
{


    return CLI_SUCCESS;
}

/*void lsp_write_single_config(struct service_lsp *srv, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(srv);
    pal_assert(cli);

    cli_out (cli, "lsp %d\n", srv->index);
    cli_out (cli, " config ingress-tun %d egress-tun %d\n",
        srv->tun[INGRESS].id,
        srv->tun[EGRESS].id);

    cli_out (cli, "!\n");

    return;
}*/

/*
 * 打印lsp配置信息
 */
/*int lsp_write_config(struct cli *cli)
{
    service_foreach_lsp(lsp_write_single_config, cli);
    return CLI_SUCCESS;
}*/

void ptp_port_cli_init(struct cli_tree *ctree)
{
    
	cli_install_default (ctree,PTP_PORT_MODE);
	cli_install (ctree, PTP_MODE,&ptp_port_id_cmd);
	cli_install (ctree, PTP_PORT_MODE,&ptp_port_id_cmd);
    cli_install(ctree, PTP_PORT_MODE, &ptp_port_bind_config_cmd);
    cli_install(ctree, PTP_PORT_MODE, &ptp_port_unbind_config_cmd);
    cli_install(ctree, PTP_PORT_MODE, &ptp_port_enable_config_cmd);
    cli_install(ctree, PTP_PORT_MODE, &ptp_port_disable_config_cmd);
    //cli_install(ctree, PTP_PORT_MODE, &asymmetrydelay_config_cmd);
	//cli_install(ctree, PTP_PORT_MODE, &announce_interval_config_cmd);
	//cli_install(ctree, PTP_PORT_MODE, &announce_receipt_config_cmd);
	cli_install(ctree, PTP_PORT_MODE, &delaymech_config_cmd);
	//cli_install(ctree, PTP_PORT_MODE, &delay_request_interval_config_cmd);
	cli_install(ctree, PTP_PORT_MODE, &p2p_meanpath_delay_config_cmd);
	cli_install(ctree, PTP_PORT_MODE, &p2p_request_interval_config_cmd);
	//cli_install(ctree, PTP_PORT_MODE, &port_status_config_cmd);
	//cli_install(ctree, PTP_PORT_MODE, &sync_interval_config_cmd);	
}
