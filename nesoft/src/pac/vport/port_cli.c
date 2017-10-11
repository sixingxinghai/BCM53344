/******************************************************************************
 *
 *  pac/vport/port.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Zhangke <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  port，创建于2012.7.24
 *
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "hsl/hal_car.h"

#include "pal.h"
#include "lib.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */



#include "avl_tree.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"


#include "nsm_qos.h"
#include "nsm_qos_cli.h"
#include "nsm_qos_list.h"
#include "nsm_qos_filter.h"

#ifdef HAVE_VLAN
#include "nsm_bridge.h"
#include "nsm_vlan.h"
#endif /* HAVE_VLAN */
#ifdef HAVE_HA
#include "nsm_flowctrl.h"
#include "nsm_qos_cal_api.h"
#endif /*HAVE_HA*/




/*
 * 新增 port mode under configuration mode 
 * wangjian@150116:
 * and move the qos wrr-queue to this port mode from interface mode 
 *
 */

CLI(port_fe,
    port_fe_cmd,
    "fe <1-24>",
    CLI_RPORT_MODE_STR,
    "fe interface id")
{
	struct vport_port *port;

	UINT16 intf_id = atoi(argv[0]);
	UINT16 index = 0;
	if (!IS_INTF_VALID(INTF_FE, intf_id))
	{
		cli_out(cli, "%% input inerface info error.\n");
		return CLI_ERROR;
	}
	
	
	index = vport_clac_if_index(INTF_FE, intf_id);
	port = vport_find_port_by_index(index);
	
   
	if (NULL == port)
    {
        cli_out(cli, "%% port %s does not exist, create it now.\n", argv[0]);
        port = vport_new_port(index);
        if (NULL == port)
        {
            cli_out(cli, "%% create port fail.\n");
            return CLI_ERROR;
        }
    }

	SET_PORT_DATA(port, INGRESS, INTF_FE, intf_id);
	SET_PORT_DATA(port, EGRESS, INTF_FE, intf_id);

	zlog_info(NSM_ZG, "data: intf(type:%d, id:%d) .",  INTF_FE, intf_id);

	cli->index = port;
	cli->mode = RPORT_MODE;
	return CLI_SUCCESS;
}


/*wangjian@150116, change port cmd format from ge/fe to port ge/fe */

CLI(port_ge,
    port_ge_cmd,
    "ge <1-4>",
    CLI_RPORT_MODE_STR,
    "ge interface id")
{
	struct vport_port *port;

	UINT16 intf_id = atoi(argv[0]);
	UINT16 index = 0;
	if (!IS_INTF_VALID(INTF_GE, intf_id))
	{
		cli_out(cli, "%% input inerface info error.\n");
		return CLI_ERROR;
	}
	index = vport_clac_if_index(INTF_GE, intf_id);
	port = vport_find_port_by_index( index);
    if (NULL == port)
    {
        cli_out(cli, "%% port %s does not exist, create it now.\n", argv[0]);
        port = vport_new_port(index);
        if (NULL == port)
        {
            cli_out(cli, "%% create port fail.\n");
            return CLI_ERROR;
        }
    }

	SET_PORT_DATA(port, INGRESS, INTF_GE, intf_id);
	SET_PORT_DATA(port, EGRESS, INTF_GE, intf_id);

	zlog_info(NSM_ZG, "data: intf(type:%d, id:%d) .",  INTF_GE, intf_id);

	cli->index = port;
	cli->mode = RPORT_MODE;
	return CLI_SUCCESS;
}



/*
 * 删除port
 */
/*CLI (no_port_idt,
    no_port_idt_cmd,
    "no (fe <1-24>|ge <1-4>)",
    CLI_NO_STR,
    "delete a port's configuration",
    "fe interface id",
    "delete a port's configuration",
    "ge interface id")
{
	struct vport_port *port;
	enum em_intf_type intf_type = (0 == pal_strcmp("fe", argv[0])) ? INTF_FE : INTF_GE;
	UINT16 intf_id = atoi(argv[1]);
	UINT16 index = vport_clac_if_index(intf_type, intf_id);

    port= vport_find_port_by_index(index);
    if (NULL == port)
    {
        cli_out (cli, "%% port %s does not exist\n", argv[0]);
        return CLI_ERROR;
    }

    vport_delete_port(port);
    return CLI_SUCCESS;
}*/

/*
 * 配置car port rate limit 参数
 */
CLI(port_rate_limit,
    port_rate_limit_cmd,
    "rate-limit (disable|enable cir <0-1000000> cbs <0-4095> )",
    "config port rate limit paras",
    "disable rate limit",
    "enable rate limit",
    "committed information rate",
    "rate value, kbps",
    "committed brust size",
    "brust size value, bytes")
{
    struct vport_port *port;
    BOOL is_enable = (0 == pal_strcmp("disable", argv[0])) ? FALSE : TRUE;
	UINT32 cir = 0;
	UINT32 cbs = 0;
	UINT32 pir = 0;
	UINT32 pbs = 0;
	UINT8 fwd_red = 0;
	UINT8 drp_yellow = 0;
    UINT8 i = 0;
	
    for (i = 1; i < argc - 1; i += 2)
    {

		if (0 == pal_strcmp("cir", argv[i]))
        {
            cir = atoi(argv[i + 1]);
        }

        else if (0 == pal_strcmp("cbs", argv[i]))
        {
            cbs = atoi(argv[i + 1]);
        }

    }

    if (!IS_CAR_XIR_VALID(cir))
    {
        cli_out(cli, "%% input information rate error.\n");
        return CLI_ERROR;
    }
	
	if (!IS_CAR_XBS_VALID(cbs)) 
	{
		cli_out(cli, "%% input brust size error.\n");
		return CLI_ERROR;
	}


	port= vport_get_current_port(cli);
    if (NULL == port)
    {
        cli_out(cli, "%% current port does not exist.\n");
        return CLI_ERROR;
    }

    {
        struct hal_car_info_s msgdata = {0};
        if (port->rate_limit.is_enable)
        {
			msgdata.enable = FALSE;
            msgdata.portId = port->index;
            comm_send_hal_msg(HAL_MSG_CAR_PORT_RATE_LIMIT, &msgdata, sizeof(msgdata), NULL, NULL);
        }

        if (is_enable)
        {
			msgdata.enable = is_enable;
            msgdata.portId = port->index;
            msgdata.cir = cir;
            msgdata.cbs= cbs;
            comm_send_hal_msg(HAL_MSG_CAR_PORT_RATE_LIMIT, &msgdata, sizeof(msgdata), NULL, NULL);
        }
    }

	SET_CAR_DATA(port->rate_limit, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);

    zlog_info(NSM_ZG, "set car: is_enable:%d,  cir:%d,  cbs:%d.",
      is_enable, cir,  cbs);

    return CLI_SUCCESS;
}



/*
 * 配置car port shapping参数
 */
CLI(port_shapping,
	port_shapping_cmd,
	"shapping (disable|enable cir <0-1000000> cbs <0-4095>)",
    "config shapping paras",
    "disable shapping",
    "enable shapping",
    "committed information rate",
    "rate value, kbps",
    "committed brust size",
    "brust size value, bytes")
{
	struct vport_port *port;
	BOOL is_enable = (0 == pal_strcmp("disable", argv[0])) ? FALSE : TRUE;
	UINT32 cir = 0;
	UINT32 cbs = 0;
	UINT32 pir = 0;
	UINT32 pbs = 0;
	UINT8 fwd_red = 0;
	UINT8 drp_yellow = 0;
	UINT8 i = 0;

	for (i = 1; i < argc - 1; i += 2)
	{

		if (0 == pal_strcmp("cir", argv[i]))
		{
			cir = atoi(argv[i + 1]);
		}

		else if (0 == pal_strcmp("cbs", argv[i]))
		{
			cbs= atoi(argv[i + 1]);
		}
	}

	if (!IS_CAR_XIR_VALID(cir)) 
	{
		cli_out(cli, "%% input information rate error.\n");
		return CLI_ERROR;
	}
	
	if (!IS_CAR_XBS_VALID(cbs)) 
	{
			cli_out(cli, "%% input brust size error.\n");
			return CLI_ERROR;
	}

	port= vport_get_current_port(cli);
	if (NULL == port)
	{
		cli_out(cli, "%% current port does not exist.\n");
		return CLI_ERROR;
	}

	{
	    struct hal_car_info_s msgdata = {0};
	    if (port->shapping.is_enable)
	    {
			msgdata.enable = FALSE;
	        msgdata.portId = port->index;
			//msgdata.qid = qid;
	        comm_send_hal_msg(HAL_MSG_CAR_PORT_SHAPPING, &msgdata, sizeof(msgdata), NULL, NULL);
	    }

	    if (is_enable)
	    {
			msgdata.enable = is_enable;
	        msgdata.portId = port->index;
	        msgdata.cir = cir;
	        msgdata.cbs= cbs;
	        comm_send_hal_msg(HAL_MSG_CAR_PORT_SHAPPING, &msgdata, sizeof(msgdata), NULL, NULL);
	    }

	}

	SET_CAR_DATA(port->shapping, is_enable, cir, cbs, pir, pbs, fwd_red,drp_yellow);
    //port->sp_is_enable = is_enable;
	
	zlog_info(NSM_ZG, "set car: is_enable:%d,  cir:%d,  cbs:%d.",
	   is_enable, cir,  cbs);

	return CLI_SUCCESS;
}


/*wmx150120 add cmd of eress-car/wred/weight*/
CLI (wrr_queue_egress_car,
     wrr_queue_egress_car_cmd,
     "egress-car (disable qid <0-7>|enable qid <0-7> cir <0-1000000> pir <0-1000000>)",
     "WRR queue car",
     "disable to restore default egress car configuration",
     "Specify queue ID",
     "Queue ID 0-7",
     "enable egress car ",
     "Specify queue ID",
     "Queue ID 0-7",
     "Set cir paras",
     "Specify the cir in term of kbps",
     "Set pir paras",
     "Specify the pir in term of kbps")
{   
  
	 struct vport_port *port;
	 BOOL is_enable = (0 == pal_strcmp("disable", argv[0])) ? FALSE : TRUE;
	 int cir = 0;
	 int pir = 0;
	 int qid = 0;
	 UINT8 i = 0;

	 /* check argument  */
	 for (i = 1; i < argc - 1; i += 2)
	 {
	 	if (0 == pal_strcmp("qid", argv[i]))
		{
			qid = atoi(argv[i + 1]);
		}
	 	else if (0 == pal_strcmp("cir", argv[i]))
		{
			cir= atoi(argv[i + 1]);
		}
		else if (0 == pal_strcmp("pir", argv[i]))
		{
			pir = atoi(argv[i + 1]);
		}
	 }
	 
     /* get port struct from cli and send msg */
	 port= vport_get_current_port(cli);

	 if (NULL == port)
	 {
		cli_out(cli, "%% current port does not exist.\n");
		return CLI_ERROR;
	 }
	 
	 struct hal_wrr_car_info_s msgdata = {0};
	 
	 if (port->queue.car[qid].is_enable)
	 {
		msgdata.is_enable = FALSE;
	    msgdata.portId = port->index;
		msgdata.qid = qid;
	    comm_send_hal_msg(HAL_MSG_WRR_CAR_VPN_PORT, &msgdata, sizeof(msgdata), NULL, NULL);
	 }


	 if (is_enable)
	 {
		msgdata.is_enable = is_enable;
	    msgdata.portId = port->index;
		msgdata.qid = qid;
	    msgdata.cir = cir;
	    msgdata.pir= pir;
	    comm_send_hal_msg(HAL_MSG_WRR_CAR_VPN_PORT, &msgdata, sizeof(msgdata), NULL, NULL);
	 }

	  /* record car info in port struct */
     port->queue.car[qid].cir = cir;
	 port->queue.car[qid].pir = pir;
	 port->queue.car[qid].is_enable = is_enable;
	 
	 

	 zlog_info(NSM_ZG, "egerss-car set: qid:%d, is_enable:%d, cir:%d,  pir:%d.",
			 qid, is_enable, cir,  pir);

     return CLI_SUCCESS;
     
}


CLI (wrr_queue_wred,
     wrr_queue_wred_cmd,
     "wred (disable qid <0-7>|enable qid <0-7> color <0-7> dropstart <0-100> slope <0-90> time <0-255>)",
     "Configure WRED drop threshold and weight on egress queue.",
     "disable to restore the queue to default wred configuration. ",
     "set queue ID.",
     "ID number 0-7.",
     "enable wred configuration.",
     "set queue ID",
     "ID number 0-7",
     "Specify the bit map of color combination",
     "Set color combinations, bit0=1->red; bit1=1->yellow; bit2=1->green. if set 0,restore default setting",
     "Specify the percentage of average queue size to start dropping packets",
     "Set start dropping packets paras",
     "Specify the the probality slope of droping packets",
     "Set slope paras",
     "Specify the average time in us, average_time/4 = 2**WRED_weight",
     "Set average_time paras")
{
	struct vport_port *port;
	BOOL is_enable = (0 == pal_strcmp("disable", argv[0])) ? FALSE : TRUE;
	int color = 0;
	int startpoint = 0;
	int slope = 0;
	int time = 0;
	int qid = 0;
	UINT8 i = 0;
		
	for (i = 1; i < argc - 1; i += 2)
	{
		
	    if (0 == pal_strcmp("qid", argv[i]))
		{
		   qid = atoi(argv[i + 1]);
		}
		
		else if (0 == pal_strcmp("color", argv[i]))
		{
			color = atoi(argv[i + 1]);
		}
		else if (0 == pal_strcmp("dropstart", argv[i]))
		{
			startpoint = atoi(argv[i + 1]);
		}
		else if (0 == pal_strcmp("slope", argv[i]))
		{
			slope = atoi(argv[i + 1]);
		}
		
		else if (0 == pal_strcmp("time", argv[i]))
		{
			time = atoi(argv[i + 1]);
		}
				
	}
		
	port = vport_get_current_port(cli);
	if (NULL == port)
	{
		cli_out(cli, "%% current port does not exist.\n");
		 return CLI_ERROR;
	}
		 
	struct hal_wrr_wred_info_s msgdata = {0};
	if (port->queue.wred[qid].is_enable)
	{
		msgdata.is_enable = FALSE;
		msgdata.portId = port->index;
		msgdata.qid = qid;
		comm_send_hal_msg(HAL_MSG_WRR_WRED_VPN_PORT, &msgdata, sizeof(msgdata), NULL, NULL);
	}
	
	if (is_enable)
	{
		msgdata.is_enable = is_enable;
		msgdata.portId = port->index;
	    msgdata.qid = qid;
		msgdata.color = color;
		msgdata.startpoint = startpoint;
		msgdata.slope = slope;
		msgdata.time = time;
			   
	    comm_send_hal_msg(HAL_MSG_WRR_WRED_VPN_PORT, &msgdata, sizeof(msgdata), NULL, NULL);
	}
	
	port->queue.wred[qid].color = color;
	port->queue.wred[qid].startpoint = startpoint;
	port->queue.wred[qid].slope = slope;
	port->queue.wred[qid].time = time;
	port->queue.wred[qid].is_enable = is_enable;
	
	
	zlog_info(NSM_ZG, "set wred: qid:%d, is_enable:%d, color%d, startpoint:%d, slope:%d, time:%d",
	   qid,is_enable,color,startpoint,slope,time);
	
	return CLI_SUCCESS;


}

CLI (wrr_queue_weight_config,
     wrr_queue_weight_config_cmd,
     "weight (disable|enable <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016> <0-1016>)",
     "WDRR queue weight configuration",
     "disable to restore the default wrr configuration",
     "eenable wrr configuration",
     "Weight value of Queue 0",
     "Weight value of Queue 1",
     "Weight value of Queue 2",
     "Weight value of Queue 3",
     "Weight value of Queue 4",
     "Weight value of Queue 5",
     "Weight value of Queue 6",
     "Weight value of Queue 7")
{
     
	    struct vport_port *port;
		BOOL is_enable = (0 == pal_strcmp("disable", argv[0])) ? FALSE : TRUE;
		UINT8 i = 0;
		int weight[8];
		
	    if(argc>1)
        { 
           for(i = 0;i<8;i++)
           weight[i] = atoi(argv[i + 1]);
        }
		
		port= vport_get_current_port(cli);
		if (NULL == port)
		{
			cli_out(cli, "%% current port does not exist.\n");
			return CLI_ERROR;
		}
	
		struct hal_wrr_weight_info_s  msgdata = {0};
			
		if (port->queue.weight.is_enable)
		{
		    msgdata.is_enable = FALSE;
			msgdata.portId = port->index;
			comm_send_hal_msg(HAL_MSG_WRR_WEIGHT_VPN_PORT, &msgdata, sizeof(msgdata), NULL, NULL);
		}
	
		if (is_enable)
		{
			msgdata.is_enable = is_enable;
			msgdata.portId = port->index;
			
			for(i = 0;i<8; i++)
		    {
			    msgdata.weight[i] = weight[i];
		    }
			
			comm_send_hal_msg(HAL_MSG_WRR_WEIGHT_VPN_PORT, &msgdata, sizeof(msgdata), NULL, NULL);
		}

		/*record wrr weight in vport struct  */
	    for(i = 0;i<8; i++)
	    {
		    port->queue.weight.weight[i] = weight[i];
	    }
	    port->queue.weight.is_enable = is_enable;
	    zlog_info(NSM_ZG, "set weight: is_enable:%d, weight_0:%d, weight_1:%d,weight_2:%d,weight_3:%d,weight_4:%d,weight_5:%d,weight_6:%d,weight_7:%d,",
	    is_enable,weight[0],weight[1],weight[2],weight[3],weight[4],weight[5],weight[6],weight[7] );
	    return CLI_SUCCESS;
	
}

	
void port_write_single_config(struct vport_port *port,void *para)
{
    struct cli *cli = (struct cli *)para;
	
	int i;
	int car = 0;
	int wred = 0;
	int weight = 0;
	int count = 0;
	

    pal_assert(port);
    pal_assert(cli);

	/*wmx@150120 判断egress-car/wred/weight的is_enable是否全为0，全为0表示没有设置egress-car/wred/weight*/
	for (i = 0;i<8;i++)
	{
			car += port->queue.car[i].is_enable;
			wred += port->queue.wred[i].is_enable;
	}

	count = car +wred+ port->queue.weight.is_enable;
		
	if ((!port->rate_limit.is_enable)&&(!port->shapping.is_enable)&&(!count))
	{
		return CLI_SUCCESS;
	}
  
    cli_out(cli, "%s %d\n",
        (port->data[INGRESS].intf_type == INTF_FE) ? "fe" : "ge",
         port->data[INGRESS].intf_id);

	if (port->rate_limit.is_enable)
    {
      cli_out(cli, " rate-limit enable cir %d cbs %d\n",
		port->rate_limit.cir, port->rate_limit.cbs);
    }

	if (port->shapping.is_enable)
	{
	  cli_out(cli, " shapping enable cir %d cbs %d \n",
			port->shapping.cir, port->shapping.cbs);
	}

	/*wmx@150120*/
	for (i = 0; i < MAX_NUM_OF_QUEUE; i++)
	{
	  if (port->queue.car[i].is_enable)
	  
	    cli_out(cli, " egress-car enable qid %d cir %d pir %d\n",
		                 i,port->queue.car[i].cir,port->queue.car[i].pir);
	}
	
	for (i = 0; i < MAX_NUM_OF_QUEUE; i++)
	{
	    if (port->queue.wred[i].is_enable)
	   
	       cli_out(cli, " wred enable qid %d color %d dropstart %d slope %d time %d\n",
		   i,port->queue.wred[i].color, port->queue.wred[i].startpoint, port->queue.wred[i].slope, port->queue.wred[i].time);
	   
	}

	if (port->queue.weight.is_enable)
		
		cli_out(cli, " weight enable %d %d %d %d %d %d %d %d\n",
					 port->queue.weight.weight[0], 
					 port->queue.weight.weight[1], 
					 port->queue.weight.weight[2], 
					 port->queue.weight.weight[3], 
					 port->queue.weight.weight[4],  
					 port->queue.weight.weight[5], 
					 port->queue.weight.weight[6], 
					 port->queue.weight.weight[7]);
	
	
    cli_out(cli, "!\n");

    return;
}


/*
 * 打印ac配置信息
 */
int port_write_config(struct cli *cli)
{
    vport_foreach_port(port_write_single_config, cli);
    return CLI_SUCCESS;
}

void port_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, RPORT_MODE);
    cli_install_config(ctree, RPORT_MODE, port_write_config);
    cli_install(ctree, CONFIG_MODE, &port_fe_cmd);
    cli_install(ctree, CONFIG_MODE, &port_ge_cmd); /* combine fe and ge into one cmd */
    // cli_install(ctree, CONFIG_MODE, &no_port_idt_cmd);
    cli_install(ctree, RPORT_MODE, &port_rate_limit_cmd);
	cli_install(ctree, RPORT_MODE, &port_shapping_cmd);
	
	cli_install(ctree, RPORT_MODE, &wrr_queue_egress_car_cmd);    //wmx@150120
	cli_install(ctree, RPORT_MODE, &wrr_queue_wred_cmd);          //wmx@150120
	cli_install(ctree, RPORT_MODE, &wrr_queue_weight_config_cmd);  //wmx@150120

	return;
}
