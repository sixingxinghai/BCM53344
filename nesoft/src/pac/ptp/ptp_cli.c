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
 *  ptp(IEEE1588精确时间协议全局模式， 目前只实现透传时钟)CLI部分，创建于2012.7.5
 *
 */

#include "pal.h"
#include "lib.h"

#include "avl_tree.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"

#include "ptp_cli.h"
#include "ptp_port_cli.h"

#include "hsl/hal_ptp.h"


struct ptp_port_config port_config_data[PTP_MAX_PORT_NUMBER];//时钟端口最多为20个，在所有的28个以太网口中选择
static int portbitmap[PTP_MAX_DOMAIN_NUMBER];//配置时钟域(0-255)中的端口，一个时钟域中最多20个端口(主端口b0-1，从端口b6-23(代表2-19)，b31表示该域是否有效，1为有效)，一个端口可以同时处于两个时钟域
static enum em_clock_type clocktype;//暂定不考虑混合型时钟，每个设备只能为oc、bc、tc中的一种
static int ptp_init_flag = PTP_NO_INIT_FLAG;

void hal_ptp_port_init()
{
    int i;
	for(i=0; i<PTP_MAX_PORT_NUMBER; i++)
	{
		port_config_data[i].phyport = PTP_PHYPORT_INVALID_FLAG;
		port_config_data[i].delay_mech = PTP_P2P;
	} 
	for(i=0; i<PTP_MAX_MPORT_NUMBER; i++)
	{
	    //port_config_data[vport].p2p_req_interval = 0;
		port_config_data[i].p2p_mean_delay =  P2P_INIT_MEAN_DELAY;
	}
	
}

/*端口号0-19有效,不超过20个端口*/
BOOL ptp_bitmap_valid(UINT32 id)
{
	int i,j=0;
	for(i=0; i<24; i++)
	{
		if ((id & (0x01 << i)) != 0)
			j++;
	}
	if (j > PTP_MAX_PORT_NUMBER)
		return 0;
	for(i=24; i<31; i++)
	{
		if ((id & (0x01 << i)) != 0)
			return 0;
	}
	return 1;
}

void hal_ptp_set_type (enum em_clock_type type)
{
    struct hal_msg_ptp_clk_type type_msgdata ={0};
	type_msgdata.type = type;
    comm_send_hal_msg(HAL_MSG_PTP_CLK_TYPE_SET,&type_msgdata,sizeof(type_msgdata), NULL, NULL);
}

void hal_ptp_create_domain (UINT32 num)
{
    comm_send_hal_msg(HAL_MSG_PTP_DOMAIN_CREATE,&num,sizeof(num), NULL, NULL);
}

void hal_ptp_domain_add_mport (UINT32 num, UINT32 vport)
{
    struct hal_msg_ptp_domain_port port_msgdata= {0};
	port_msgdata.domain = num;
	port_msgdata.port = vport;
	comm_send_hal_msg(HAL_MSG_PTP_DOMAIN_MPORT_ADD,&port_msgdata,sizeof(port_msgdata), NULL, NULL);
}
    

void hal_ptp_domain_add_sport (UINT32 num, UINT32 vport)
{
    struct hal_msg_ptp_domain_port port_msgdata= {0};
	port_msgdata.domain = num;
	port_msgdata.port = vport;
	comm_send_hal_msg(HAL_MSG_PTP_DOMAIN_SPORT_ADD,&port_msgdata,sizeof(port_msgdata), NULL, NULL);
}




void hal_ptp_set_delay_mech(enum em_delay_mech mech)
{
    struct hal_msg_ptp_delay_mech mech_msgdata ={0};
	mech_msgdata.delay_mech = mech;
    comm_send_hal_msg(HAL_MSG_PTP_DELAY_MECH_SET,&mech_msgdata,sizeof(mech_msgdata), NULL, NULL);
}


/*
 * 创建ptp模式，(config-ptp)
 */
CLI(ptp_mode,
    ptp_mode_cmd,
    "ptp",
    CLI_PTP_MODE_STR)
{
     //cli->index = srv;
     cli->mode = PTP_MODE;
	 if( ptp_init_flag == PTP_NO_INIT_FLAG)
	 {
	     hal_ptp_port_init();
		 ptp_init_flag = PTP_INIT_FLAG;
	 }

     return CLI_SUCCESS;
}

/*
 * 配置时钟类型
 * 暂定不考虑混合型时钟，每个设备只能为oc、bc、tc中的一种
 */
CLI(clocktype_config,
    clocktype_config_cmd,
    "set-clock-type (tc|oc|bc)",
    "set clock type",
    "transparent clock",
    "ordinary clock",
    "boundary clock")
{
    enum em_clock_type type;

	if (0 == pal_strcmp("tc", argv[0]))
	{
	    type = PTP_TC;
	}
	else if (0 == pal_strcmp("oc", argv[0]))
    {
        cli_out(cli, "the clock-type oc and bc is not supported now.\n");
        return CLI_ERROR;
        type = PTP_OC;
    }
	else if (0 == pal_strcmp("bc", argv[0]))
    {
        cli_out(cli, "the clock-type oc and bc is not supported now.\n");
        return CLI_ERROR;
        type = PTP_BC;
    }
    else
    {
        cli_out(cli, "the clock-type should be tc or oc or bc.\n");
        return CLI_ERROR;
    }

	hal_ptp_set_type (type);

	clocktype = type;

	zlog_info(NSM_ZG, "set clock type %d.", clocktype);
    return CLI_SUCCESS;
}

/*
 * 时钟域创建
 */
CLI(domain_create,
    domain_create_cmd,
    "domain-create <0-255>",
    "create domain",
    "domain value, range 0 - 255")
{
    UINT32 num = atoi (argv[0]);

	if ( !IS_PTP_DOMAIN_VALID(num))
	{
        cli_out(cli, "the domain number should between 0-255.\n");
        return CLI_ERROR;
    }
	if ( (portbitmap[num] & (0x01<<31)) != 0 )
	{
		cli_out(cli, "the domain number is already created.\n");
        return CLI_ERROR;
    }

    hal_ptp_create_domain (num);
	
	portbitmap[num] = 0x01<<31;

    zlog_info(NSM_ZG, "create domain %d.", num);
    return CLI_SUCCESS;
}

/*
 * 配置时钟域中的端口
 * 一个时钟域中最多20个端口，一个端口可以同时处于两个时钟域
 * 主端口用于和主时钟相连
 */
CLI(domain_mport_add,
    domain_mport_add_cmd,
    "domain-add <0-255> mport <0-1>",
    "add vport to domain",
    "domain number, range 0 - 255",
    "add port to be connected with master clock", 
    "virtual port, range 0-1" )
{
    UINT32 num = atoi (argv[0]);
	UINT32 vport = atoi (argv[1]);
	
	if ( !IS_PTP_DOMAIN_VALID(num))
	{
        cli_out(cli, "the domain number should between 0-255.\n");
        return CLI_ERROR;
    }
	if ( !IS_PTP_MPORT_INDEX_VALID(vport))
    {
        cli_out(cli, "the virtual mport index should between 0-1.\n");
        return CLI_ERROR;
    }
	
	if( (portbitmap[num] & (0x01<<31)) == 0 )
	{
        cli_out(cli, "the domain %d has not been created.\n",num);
        return CLI_ERROR;
    }
	
	/*if( (portbitmap[num] & (0x01<<(4+vport))) != 0 )
	{
        cli_out(cli, "the virtual port %d has been set sport in domain %d.\n",vport,num);
        return CLI_ERROR;
       }*/

    hal_ptp_domain_add_mport (num, vport);
	
	portbitmap[num] |= (0x01<<vport);
	port_config_data[vport].domain_set = 1;

    zlog_info(NSM_ZG, "domain %d add mport %d.", num, vport);
    return CLI_SUCCESS;
}

/*
 * 配置时钟域中的端口
 * 一个时钟域中最多20个端口，一个端口可以同时处于两个时钟域
 * 从端口用于和从时钟相连
 */
	CLI(domain_sport_add,
		domain_sport_add_cmd,
		"domain-add <0-255> sport <2-19>",
		"add vport to domain",
		"domain number, range 0 - 255",
		"add port to be connected with slave clock", 
		"virtual port, range 2-19" )
	{
		UINT32 num = atoi (argv[0]);
		UINT32 vport = atoi (argv[1]);
		
		if ( !IS_PTP_DOMAIN_VALID(num))
		{
			cli_out(cli, "the domain number should between 0-255.\n");
			return CLI_ERROR;
		}
		
		if ( !IS_PTP_SPORT_INDEX_VALID(vport))
		{
			cli_out(cli, "the virtual sport index should between 2-19.\n");
			return CLI_ERROR;
		}
	
		if( (portbitmap[num] & (0x01<<31)) == 0 )
	    {
            cli_out(cli, "the domain %d has not been created.\n",num);
            return CLI_ERROR;
        }
		/*if( vport >= 0 && vport < 2 )
		{
		  if( (portbitmap[num] & (0x01<<(vport))) != 0 )
	      {
            cli_out(cli, "the virtual port %d has been set mport in domain %d.\n",vport,num);
            return CLI_ERROR;
          }
		}*/
	
		hal_ptp_domain_add_sport (num, vport);
		
		portbitmap[num] |= (0x01<<(vport+PTP_BITMAP_SPORT_OFFSET));
		port_config_data[vport].domain_set = 1;
	
		zlog_info(NSM_ZG, "domain %d add sport %d.", num, vport);
		return CLI_SUCCESS;
	}


/*
 * 设置传播延时的测量机制
 * 同时设定20个虚拟端口(和物理端口映射)的延时测量机制
 */
CLI(timedelmech_config,
	timedelmech_config_cmd,
	"set-delay-mech  (p2p|e2e)",
	"set time delay mech",
	"peer to peer",
	"end to end")
{
    int i;
    enum em_delay_mech type = (0 == pal_strcmp("p2p",argv[0])) ? PTP_P2P : PTP_E2E;

	hal_ptp_set_delay_mech (type);

	for (i = 0; i < PTP_MAX_PORT_NUMBER; i++)
	port_config_data[i].delay_mech = type;

	zlog_info(NSM_ZG, "set delay mech %d.", port_config_data[0].delay_mech);
	return CLI_SUCCESS;
}

/*
 * 设置发送帧的类型
 * 目前设备只支持l2帧
 */
/*CLI(txframetype_config,
	txframetype_config_cmd,
	"set txframetype  FLAG",
	"config txframetype",
	"tx frame type, range <l2 l3>")
{
	
	return CLI_SUCCESS;
}*/

/*
 * 配置时钟级别
 */
CLI(clockclass_config,
    clockclass_config_cmd,
    "set-clock-class <0 - 255>",
    "set clock class",
    "clock class, range <0 - 255>")
{


    return CLI_SUCCESS;
}


/*
 * 配置端口状态
 */
CLI(portstatus_config,
    portstatus_config_cmd,
    "set-port-status  NUMBER    STATUS",
    "set port status",
    "port number, range <1-8>",
    "status, range <master slave passive>")
{


    return CLI_SUCCESS;
}

/*
 * 配置端口状态机制
 * 暂定默认为手动方式
 */
CLI(portstatemech_config,
	portstatemech_config_cmd,
	"set-state-mech  STATEMECH",
	"config port state mechanism",
	"port state mechanism, range <bcm manual>")
{
	
	return CLI_SUCCESS;
}

/*
 * 配置优先级1
 */
CLI(priority_one_config,
	priority_one_config_cmd,
	"set-priority1  <0 - 255>",
	"set priority1",
	"priority1, range <0 - 255>")
{
	
	return CLI_SUCCESS;
}
/*
 * 配置优先级2
 */
CLI(priority_two_config,
	priority_two_config_cmd,
	"set-priority2  <0 - 255>",
	"set priority2",
	"priority2, range <0 - 255>")
{
	
	return CLI_SUCCESS;
}

/*
 * 配置PTP方差的采样周期
 */
CLI(scalelogvar_config,
	scalelogvar_config_cmd,
	"set-scale-log-var  <-2147483648 - 2147483647>",
	"set scalelogvar,used for variance algorithm",
	"scale log variance, range <-2147483648 - 2147483647>")
{
	
	return CLI_SUCCESS;
}

/*
 * 配置从时钟设备的发送帧类型
 *false:  tx frame type L3
 *true:   tx frame type L2
 */
/*CLI(slaveonly_config,
	slaveonly_config_cmd,
	"set-slaveonly  FLAG",
	"config slaveonly tx frame type",
	"slaveonly tx frame type, range <ture false>")
{
	
	return CLI_SUCCESS;
}*/



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
 * 打印ptp配置信息
 */
int ptp_write_config(struct cli *cli)
{
    int i,j;
    pal_assert(cli);

    cli_out (cli, "ptp\n");
	if( clocktype == PTP_TC )
	{
	    cli_out (cli, " set-clock-type tc\n");
	}
	else
	{
	    cli_out (cli, " other clock type\n");
	}
	for( i = 0; i <256; i ++ )
    {
        if( (portbitmap[i] & (0x01<<31)) != 0 )
        {
            cli_out (cli, " domain-create %d\n",i);
			for(j = 0; j < PTP_MAX_MPORT_NUMBER; j ++ )
            {
                if( (portbitmap[i] & ( 0x01 << j )) != 0 )
                {
                    cli_out (cli, " domain-add %d mport %d\n",i,j);
                }
			}
			for(j = PTP_MAX_MPORT_NUMBER; j < PTP_MAX_PORT_NUMBER; j ++ )
            {
                if( (portbitmap[i] & ( 0x01 << (PTP_BITMAP_SPORT_OFFSET+j))) != 0 )
                {
                    cli_out (cli, " domain-add %d sport %d\n",i,j);
                }
			}
            for(j = 0; j < PTP_MAX_MPORT_NUMBER; j ++ )
            {
                if( (portbitmap[i] & ( 0x01 << j )) != 0 )
                {
					cli_out (cli, " port %d\n",j);
					if ( port_config_data[j].phyport <= 23 )
					    cli_out (cli, "  bind fe %d\n",port_config_data[j].phyport+1);
					else
						cli_out (cli, "  bind ge %d\n",port_config_data[j].phyport-23);
					
					if (port_config_data[j].enable == 1)
						cli_out (cli, "  enable\n");
					else
						cli_out (cli, "  no-enable\n");
					
					if (port_config_data[j].delay_mech == PTP_P2P )
				    {
					    cli_out (cli, "  set-delay-mechanism p2p\n");
					    cli_out (cli, "  set-p2p-req-interval %d\n",port_config_data[j].p2p_req_interval);
						cli_out (cli, "  set-p2p-mean-delay %d\n",port_config_data[j].p2p_mean_delay);
					}
					else
						cli_out (cli, "  set-delay-mechanism e2e\n");
                }
            }
			for(j = PTP_MAX_MPORT_NUMBER; j < PTP_MAX_PORT_NUMBER; j ++ )
            {
                if( (portbitmap[i] & ( 0x01 << (PTP_BITMAP_SPORT_OFFSET+j))) != 0 )
                {
					cli_out (cli, " port %d\n",j);
					if ( port_config_data[j].phyport <= 23 )
					    cli_out (cli, "  bind fe %d\n",port_config_data[j].phyport+1);
					else
						cli_out (cli, "  bind ge %d\n",port_config_data[j].phyport-23);
					
					if (port_config_data[j].enable == 1)
						cli_out (cli, "  enable\n");
					else
						cli_out (cli, "  no-enable\n");
					
					if (port_config_data[j].delay_mech == PTP_P2P )
				    {
					    cli_out (cli, "  set-delay-mechanism p2p\n");
					    //cli_out (cli, "  p2p delay request interval %d\n",port_config_data[j].p2p_req_interval);
						//cli_out (cli, "  p2p mean path delay %d\n",port_config_data[j].p2p_mean_delay);
					}
					else
						cli_out (cli, "  set-delay-mechanism e2e\n");
                }
            }
        }
	}
    cli_out (cli, "!\n");
    return CLI_SUCCESS;
}

void ptp_cli_init(struct cli_tree *ctree)
{
    
	cli_install_default (ctree,PTP_MODE);
    cli_install_config(ctree,PTP_MODE, ptp_write_config);
	cli_install (ctree, CONFIG_MODE,&ptp_mode_cmd);	  
	cli_install(ctree, PTP_MODE, &clocktype_config_cmd);
	cli_install(ctree, PTP_MODE, &domain_create_cmd);
	cli_install(ctree, PTP_MODE, &domain_mport_add_cmd);
	cli_install(ctree, PTP_MODE, &domain_sport_add_cmd);
	//cli_install(ctree, PTP_MODE, &clockclass_config_cmd);
	//cli_install(ctree, PTP_MODE, &portstatus_config_cmd);
	//cli_install(ctree, PTP_MODE, &portstatemech_config_cmd);
	//cli_install(ctree, PTP_MODE, &priority_one_config_cmd);
	//cli_install(ctree, PTP_MODE, &priority_two_config_cmd);
	//cli_install(ctree, PTP_MODE, &scalelogvar_config_cmd);
	//cli_install(ctree, PTP_MODE, &slaveonly_config_cmd);
	cli_install(ctree, PTP_MODE, &timedelmech_config_cmd);
	//cli_install(ctree, PTP_MODE, &txframetype_config_cmd);
}
