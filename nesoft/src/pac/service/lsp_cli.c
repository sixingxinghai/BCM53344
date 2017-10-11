/******************************************************************************
 *
 *  pac/service/lsp_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2013 Deng jingen <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  lsp(标签交换路径， lable switch path)CLI部分，创建于2013.3.4
 *
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "service.h"
#include "hsl/hal_vpls.h"
#include "hsl/hal_car.h"

#include "hsl/hal_netlink.h"

#include "l2_timer.h"


/*
 * 新增lsp
 */
CLI(lsp_id,
    lsp_id_cmd,
    "lsp <1-2000>",
    CLI_LSP_MODE_STR,
    "lsp id, range 1-2000")
{
    struct service_lsp *srv;
    int index = atoi(argv[0]);

    if (!IS_SERVICE_INDEX_VALID(index))
    {
        cli_out (cli, "%% the lsp id is not validate.\n");
        return CLI_ERROR;
    }

    srv = service_find_lsp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp %s does not exist, create it now.\n", argv[0]);
		srv = service_new_lsp(index);
        if (NULL == srv)
		{
            cli_out(cli, "%% create lsp fail.\n");
            return CLI_ERROR;
        }
 	}

     cli->index = srv;
     cli->mode = LSP_MODE;

     return CLI_SUCCESS;
}

/*
 * 删除lsp
 */
CLI (no_lsp_id,
     no_lsp_id_cmd,
     "no lsp <1-2000>",
     CLI_NO_STR,
     "delete a lsp's configuration",
     "lsp id, range 1-2000")
{
    int index = atoi(argv[0]);
    struct service_lsp *srv;

    struct
	{
	    unsigned int id;
	}msgdata;
    //struct hal_car_info_s car_msgdata = {0};

    if (!IS_VPORT_INDEX_VALID(index))
    {
        cli_out(cli, "%% ingress lsp id error.\n");
        return CLI_ERROR;
    }
    srv = service_find_lsp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp %s does not exist.\n", argv[0]);
        return CLI_ERROR;
 	}

    if (srv->pw_num !=0)
	{
        cli_out (cli, "%% pw in lsp %s should be unbinded first.\n", argv[0]);
        return CLI_ERROR;
 	}
	if (srv->grp_num !=0)
	{
        cli_out (cli, "%% protect group in lsp %s should be unbinded first.\n", argv[0]);
        return CLI_ERROR;
 	}
    if (srv->oam_num !=0)
	{
        cli_out (cli, "%% oam in lsp %s should be unbinded first.\n", argv[0]);
        return CLI_ERROR;
 	}

			
    msgdata.id = index;

    comm_send_hal_msg(HAL_MSG_LSP_DEL, &msgdata, sizeof(msgdata), NULL, NULL);

    service_delete_lsp(srv);

    return CLI_SUCCESS;
}


/*
 * 配置lsp参数
 * p-switch，过环
 */
CLI(lsp_cfg_switch,
    lsp_cfg_switch_cmd,
    "config ingress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|) egress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|) exp <0-8> peer-mac MAC",
    "lsp config",
    "ingress tunnel",
    "ingress-tun interface type",
    "fe interface id",
    "ingress-tun interface type",
    "ge interface id",
    "ingress-tun lable",
    "lable value",
    "ingress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress tunnel",
    "egress-tun interface type",
    "fe interface id",
    "egress-tun interface type",
    "ge interface id",
    "egress-tun lable",
    "lable value",
    "egress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress-tun exp",
    "exp value,<0-7> outgoing lsp label exp uses the number specified; <8> outgoing lsp label exp uses internal priority mapping",
    "egress-tun peer mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format")
{
    struct service_lsp *srv;
	hsl_bcm_msg_mpls_tunnel_cfg msgdata;
    //int index = atoi(argv[0]);
    enum em_intf_type in_intf_type = INTF_NONE;
    UINT16 in_intf_id = 0;
    UINT32 in_label = 0;
    UINT16 in_vlan = 0;
	enum em_intf_type out_intf_type = INTF_NONE;
	UINT16 out_intf_id = 0;
    UINT32 out_label = 0;
    UINT16 out_vlan = 0;
    UINT8 out_exp = 0xFF;
    CHAR out_mac_addr [ETHER_ADDR_LEN] = {0};
	int k = 0,j = 0;

    /*if (!IS_SERVICE_INDEX_VALID(index))
    {
        cli_out (cli, "%% the lsp id is not validate.\n");
        return CLI_ERROR;
    }*/

    if (0 == pal_strcmp("fe", argv[0]))
    {
        in_intf_type = INTF_FE;
        in_intf_id = atoi(argv[1]);
    }
    else if (0 == pal_strcmp("ge", argv[0]))
    {
        in_intf_type = INTF_GE;
        in_intf_id = atoi(argv[1]);
    }
    in_label = atoi(argv[2]);

    if (0 == pal_strcmp("vlan", argv[3]))
    {
        in_vlan = atoi(argv[4]);
		k = 2;
		if (!IS_VLAN_VALID(in_vlan))
        {
            cli_out(cli, "%% input in vlan info error.\n");
            return CLI_ERROR;
        }
    }
    else 
		in_vlan = 4096;//标识vlan无效
	
    if (0 == pal_strcmp("fe", argv[3 + k]))
    {
        out_intf_type = INTF_FE;
        out_intf_id = atoi(argv[4 + k]);
    }
    else if (0 == pal_strcmp("ge", argv[3 + k]))
    {
        out_intf_type = INTF_GE;
        out_intf_id = atoi(argv[4 + k]);
    }
    out_label = atoi(argv[5 + k]);

	if (0 == pal_strcmp("vlan", argv[6 + k]))
    {
        out_vlan = atoi(argv[7 + k]);
		j = 2;
		if (!IS_VLAN_VALID(out_vlan))
        {
            cli_out(cli, "%% input out vlan info error.\n");
            return CLI_ERROR;
        }
    }
    else 
		out_vlan = 4096;//标识vlan无效
    
    out_exp = atoi(argv[6 + k + j]);
    if (pal_sscanf (argv[7 + k + j], "%4hx.%4hx.%4hx",
            (unsigned short *)&out_mac_addr[0],
            (unsigned short *)&out_mac_addr[2],
            (unsigned short *)&out_mac_addr[4]) != 3)
    {
        cli_out (cli, "%% unable to translate mac address %s\n", argv[7 + k + j]);
        return CLI_ERROR;
    }

    if (!IS_INTF_VALID(in_intf_type, in_intf_id))
    {
        cli_out(cli, "%% input in inerface info error.\n");
        return CLI_ERROR;
    }

    if (!IS_LABLE_VALID(in_label))
    {
        cli_out(cli, "%% input in lable error.\n");
        return CLI_ERROR;
    }
	
	if (!IS_INTF_VALID(out_intf_type, out_intf_id))
    {
        cli_out(cli, "%% input out inerface info error.\n");
        return CLI_ERROR;
    }

    if (!IS_LABLE_VALID(out_label))
    {
        cli_out(cli, "%% input out lable error.\n");
        return CLI_ERROR;
    }
	/*WMX@150110*/
	/*
	if (!IS_EXP_VALID(out_exp))
    {
        cli_out(cli, "%% input out exp value error.\n");
        return CLI_ERROR;
    }
    */
    // mac address在输入时已经检测

    srv = service_get_current_lsp(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp does not exist.\n");
        return CLI_ERROR;
    }

	if (srv->config_flag == 1)
	{
        cli_out(cli, "%% current lsp has been configed, it should be deleted first and then recreated.\n");
        return CLI_ERROR;
    }

    srv->lsp_type = IAL_LSP_P_SWITCH;
    
    srv->in_port_type = in_intf_type;
	srv->in_port_id = in_intf_id;
	srv->in_label = in_label;
	srv->tunnel_in_vlan = in_vlan;
	srv->out_port_type = out_intf_type;
	srv->out_port_id = out_intf_id;
	srv->out_label = out_label;
	srv->tunnel_out_vlan = out_vlan;
	srv->out_tunnel_exp = out_exp;
	pal_mem_cpy(srv->tunnel_out_mac, out_mac_addr, ETHER_ADDR_LEN);
		// config sdk
	msgdata.lsp_id = srv->id;
	msgdata.lsp_type = srv->lsp_type;
    msgdata.in_port = vport_clac_if_index(srv->in_port_type, srv->in_port_id);
	msgdata.in_label = srv->in_label;
    msgdata.tunnel_in_vlan = srv->tunnel_in_vlan;
	msgdata.out_port = vport_clac_if_index(srv->out_port_type, srv->out_port_id);
    msgdata.out_label = srv->out_label;
	msgdata.tunnel_out_vlan = srv->tunnel_out_vlan;
	msgdata.out_tunnel_exp = srv->out_tunnel_exp;
	pal_mem_cpy(msgdata.tunnel_out_mac, srv->tunnel_out_mac, ETHER_ADDR_LEN);
		
	comm_send_hal_msg(HAL_MSG_LSP_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

	srv->config_flag = 1;
	 
	zlog_info(NSM_ZG, "set lsp%d data: type %d in-port-type %d in-por-id %d in-label %d in-vlan %d out-port-type %d out-por-id %d out-label %d out-vlan %d out-exp %d out-mac: %x.%x.%x.",
			 srv->id, srv->lsp_type, srv->in_port_type, srv->in_port_id, srv->in_label, srv->tunnel_in_vlan, srv->out_port_type, srv->out_port_id, srv->out_label, srv->tunnel_out_vlan, 
			 srv->out_tunnel_exp,srv->tunnel_out_mac[0],srv->tunnel_out_mac[2],srv->tunnel_out_mac[4]);

     return CLI_SUCCESS;
}

/*
 * 配置lsp参数
 * pe-up,上环
 */
CLI(lsp_cfg_up,
    lsp_cfg_up_cmd,
    "config egress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|) exp <0-8> peer-mac MAC",
    "lsp config",
    "egress tunnel",
    "egress-tun interface type",
    "fe interface id",
    "egress-tun interface type",
    "ge interface id",
    "egress-tun lable",
    "lable value",
    "egress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress-tun exp",
    "exp value,<0-7> outgoing lsp label exp uses the number specified; <8> outgoing lsp label exp uses internal priority mapping",
    "egress-tun peer mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format")
{
    struct service_lsp *srv;
	hsl_bcm_msg_mpls_tunnel_cfg msgdata;
    //int index = atoi(argv[0]);
	enum em_intf_type out_intf_type = INTF_NONE;
	UINT16 out_intf_id = 0;
    UINT32 out_label = 0;
    UINT16 out_vlan = 0;
    UINT8 out_exp = 0xFF;
    CHAR out_mac_addr [ETHER_ADDR_LEN] = {0};
	int j = 0;

    /*if (!IS_SERVICE_INDEX_VALID(index))
    {
        cli_out (cli, "%% the lsp id is not validate.\n");
        return CLI_ERROR;
    }*/
	
    if (0 == pal_strcmp("fe", argv[0]))
    {
        out_intf_type = INTF_FE;
        out_intf_id = atoi(argv[1]);
    }
    else if (0 == pal_strcmp("ge", argv[0]))
    {
        out_intf_type = INTF_GE;
        out_intf_id = atoi(argv[1]);
    }
    out_label = atoi(argv[2]);

	if (0 == pal_strcmp("vlan", argv[3]))
    {
        out_vlan = atoi(argv[4]);
		j = 2;
	  if (!IS_VLAN_VALID(out_vlan))
        {
            cli_out(cli, "%% input out vlan info error.\n");
            return CLI_ERROR;
        }
    }
    else 
		out_vlan = 4096;//标识vlan无效
    
    out_exp = atoi(argv[3 + j]);
    if (pal_sscanf (argv[4 + j], "%4hx.%4hx.%4hx",
            (unsigned short *)&out_mac_addr[0],
            (unsigned short *)&out_mac_addr[2],
            (unsigned short *)&out_mac_addr[4]) != 3)
    {
        cli_out (cli, "%% unable to translate mac address %s\n", argv[4 + j]);
        return CLI_ERROR;
    }

	if (!IS_INTF_VALID(out_intf_type, out_intf_id))
    {
        cli_out(cli, "%% input out inerface info error.\n");
        return CLI_ERROR;
    }

    if (!IS_LABLE_VALID(out_label))
    {
        cli_out(cli, "%% input out lable error.\n");
        return CLI_ERROR;
    }
	/* wmx@150110 */
  /*
	if (!IS_EXP_VALID(out_exp))
    {
        cli_out(cli, "%% input out exp value error.\n");
        return CLI_ERROR;
    }
   */
    // mac address在输入时已经检测

    srv = service_get_current_lsp(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp does not exist.\n");
        return CLI_ERROR;
    }

	if (srv->config_flag == 1 && (srv->lsp_type == IAL_LSP_PE_UP || srv->lsp_type == IAL_LSP_P_SWITCH))
	{
		cli_out(cli, "%% current lsp has been configed, it should be deleted first and then recreated.\n");
		return CLI_ERROR;
	}

    srv->lsp_type = IAL_LSP_PE_UP;
 
	srv->out_port_type = out_intf_type;
	srv->out_port_id = out_intf_id;
	srv->out_label = out_label;
	srv->tunnel_out_vlan = out_vlan;
	srv->out_tunnel_exp = out_exp;
	pal_mem_cpy(srv->tunnel_out_mac, out_mac_addr, ETHER_ADDR_LEN);
		// config sdk
	msgdata.lsp_id = srv->id;
	msgdata.lsp_type = srv->lsp_type;
	msgdata.out_port = vport_clac_if_index(srv->out_port_type, srv->out_port_id);
    msgdata.out_label = srv->out_label;
	msgdata.tunnel_out_vlan = srv->tunnel_out_vlan;
	msgdata.out_tunnel_exp = srv->out_tunnel_exp;
	pal_mem_cpy(msgdata.tunnel_out_mac, srv->tunnel_out_mac, ETHER_ADDR_LEN);
		
	comm_send_hal_msg(HAL_MSG_LSP_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

	srv->config_flag = 1;
	 
	zlog_info(NSM_ZG, "set lsp%d data: type %d in-port-type %d in-por-id %d in-label %d in-vlan %d out-port-type %d out-por-id %d out-label %d out-vlan %d out-exp %d out-mac: %x.%x.%x.",
			 srv->id, srv->lsp_type, srv->in_port_type, srv->in_port_id, srv->in_label, srv->tunnel_in_vlan, srv->out_port_type, srv->out_port_id, srv->out_label, srv->tunnel_out_vlan, 
			 srv->out_tunnel_exp,srv->tunnel_out_mac[0],srv->tunnel_out_mac[2],srv->tunnel_out_mac[4]);

     return CLI_SUCCESS;
}

/*
 * 配置lsp参数
 * pe-down,下环
 */
CLI(lsp_cfg_down,
    lsp_cfg_down_cmd,
    "config ingress (fe <1-24>|ge <1-4>) label <0-1048575> (vlan <0-4095>|)",
    "lsp config",
    "ingress tunnel",
    "ingress-tun interface type",
    "fe interface id",
    "ingress-tun interface type",
    "ge interface id",
    "ingress-tun lable",
    "lable value",
    "ingress-tun vlan",
    "vlan id, 0 means vlan diable")
{
    struct service_lsp *srv;
	hsl_bcm_msg_mpls_tunnel_cfg msgdata;
    //int index = atoi(argv[0]);
    enum em_intf_type in_intf_type = INTF_NONE;
    UINT16 in_intf_id = 0;
    UINT32 in_label = 0;
    UINT16 in_vlan = 0;

    /*if (!IS_SERVICE_INDEX_VALID(index))
    {
        cli_out (cli, "%% the lsp id is not validate.\n");
        return CLI_ERROR;
    }*/

    if (0 == pal_strcmp("fe", argv[0]))
    {
        in_intf_type = INTF_FE;
        in_intf_id = atoi(argv[1]);
    }
    else if (0 == pal_strcmp("ge", argv[0]))
    {
        in_intf_type = INTF_GE;
        in_intf_id = atoi(argv[1]);
    }
    in_label = atoi(argv[2]);

    if ( argc > 3 )
    {
        if (0 == pal_strcmp("vlan", argv[3]))
        {
            in_vlan = atoi(argv[4]);
		    if (!IS_VLAN_VALID(in_vlan))
            {
                cli_out(cli, "%% input in vlan info error.\n");
                return CLI_ERROR;
            }
        }
    }
    else 
		in_vlan = 4096;//标识vlan无效
	

    if (!IS_INTF_VALID(in_intf_type, in_intf_id))
    {
        cli_out(cli, "%% input in inerface info error.\n");
        return CLI_ERROR;
    }

    if (!IS_LABLE_VALID(in_label))
    {
        cli_out(cli, "%% input in lable error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_lsp(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp does not exist.\n");
        return CLI_ERROR;
    }

    if (srv->lsp_type == IAL_LSP_P_SWITCH)
    {
        cli_out(cli, "%% current lsp has been configed to p switch type.\n");
        return CLI_ERROR;
    }

	if (srv->config_flag == 1 && (srv->lsp_type_down == IAL_LSP_PE_DOWN || srv->lsp_type == IAL_LSP_P_SWITCH))
	{
		cli_out(cli, "%% current lsp has been configed, it should be deleted first and then recreated.\n");
		return CLI_ERROR;
	}
	
    srv->lsp_type_down = IAL_LSP_PE_DOWN;
		
    srv->in_port_type = in_intf_type;
	srv->in_port_id = in_intf_id;
	srv->in_label = in_label;
	srv->tunnel_in_vlan = in_vlan;
	// config sdk
	msgdata.lsp_id = srv->id;
	msgdata.lsp_type = srv->lsp_type_down;
    msgdata.in_port = vport_clac_if_index(srv->in_port_type, srv->in_port_id);
	msgdata.in_label = srv->in_label;
    msgdata.tunnel_in_vlan = srv->tunnel_in_vlan;
		
	comm_send_hal_msg(HAL_MSG_LSP_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

	srv->config_flag = 1;
	 
	zlog_info(NSM_ZG, "set lsp%d data: type %d in-port-type %d in-port -id %d in-label %d in-vlan %d out-port-type %d out-por-id %d out-label %d out-vlan %d out-exp %d out-mac: %x.%x.%x.",
			 srv->id, srv->lsp_type, srv->in_port_type, srv->in_port_id, srv->in_label, srv->tunnel_in_vlan, srv->out_port_type, srv->out_port_id, srv->out_label, srv->tunnel_out_vlan, 
			 srv->out_tunnel_exp,srv->tunnel_out_mac[0],srv->tunnel_out_mac[2],srv->tunnel_out_mac[4]);

     return CLI_SUCCESS;
}


/* 
   Callback function for response of lsp status get. 
*/
static int
_hal_lsp_status_get (struct hal_nlmsghdr *h, void *data)
{
  ial_mpls_lsp_status *tmp, *resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (ial_mpls_lsp_status *)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);
  tmp = (ial_mpls_lsp_status *)pnt;

  /* Decode message. */
  
  if (size < sizeof(ial_mpls_lsp_status))
    return -1;
  resp->lsp_id = tmp->lsp_id;
  resp->lsp_egress_status = tmp->lsp_egress_status;
  resp->lsp_ingress_status = tmp->lsp_ingress_status;

  return 0;
}


/*
 * 获取lsp隧道状态
 */
CLI (show_lsp_status,
     show_lsp_status_cmd,
     "show lsp-status <1-2000>",
     "show command",
     "lsp egress tunnel status and ingress tunnel status",
     "lsp id")
{
    int index = atoi(argv[0]);
    struct service_lsp *srv;
    ial_mpls_lsp_status msgdata, resp;
	char in_lsp_status[30], out_lsp_status[30];
	char *tmp;
	int ret, len;

    if (!IS_VPORT_INDEX_VALID(index))
    {
        cli_out(cli, "%% ingress lsp id error.\n");
        return CLI_ERROR;
    }
    srv = service_find_lsp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp %s does not exist.\n", argv[0]);
        return CLI_ERROR;
 	}

    msgdata.lsp_id = index;
	msgdata.lsp_egress_status = IAL_LSP_STATUS_INVALID;
	msgdata.lsp_ingress_status = IAL_LSP_STATUS_INVALID;

    comm_send_hal_msg(HAL_MSG_LSP_GET_LINK_STATUS, &msgdata, sizeof(msgdata), _hal_lsp_status_get, &resp);

    zlog_info(NSM_ZG, "lsp %d ",msgdata.lsp_id);
    if(resp.lsp_egress_status == IAL_LSP_STATUS_UP)
    {   
        zlog_info(NSM_ZG, "egress up");
		tmp = "egress up";
		pal_mem_cpy(out_lsp_status, tmp, 30);
	}
        
	else if(resp.lsp_egress_status == IAL_LSP_STATUS_DOWN)
	{
		zlog_info(NSM_ZG, "egress down");
		tmp = "egress down";
		pal_mem_cpy(out_lsp_status, tmp, 30);
	}
	else
	{
		zlog_info(NSM_ZG, "egress invalid");
		tmp = "egress invalid";
		pal_mem_cpy(out_lsp_status, tmp, 30);
	}
	if(resp.lsp_ingress_status == IAL_LSP_STATUS_UP)
	{
        zlog_info(NSM_ZG, " ingress up.\n");
		tmp = "ingress up";
		pal_mem_cpy(in_lsp_status, tmp, 30);
	}
	else if(resp.lsp_ingress_status == IAL_LSP_STATUS_DOWN)
	{
		zlog_info(NSM_ZG, " ingress down.\n");
		tmp = "ingress down";
		pal_mem_cpy(in_lsp_status, tmp, 30);
	}
	else
	{
		zlog_info(NSM_ZG, " ingress invalid.\n");
		tmp = "ingress invalid";
		pal_mem_cpy(in_lsp_status, tmp, 30);   
	}

    if (srv->config_flag == 1)
    {
        if (srv->lsp_type == IAL_LSP_P_SWITCH)
        {
            cli_out (cli, "%s %s\n", in_lsp_status, out_lsp_status);
        }
	    else if (srv->lsp_type == IAL_LSP_PE_UP && srv->lsp_type_down != IAL_LSP_PE_DOWN)
	    {
            cli_out (cli, "%s\n", out_lsp_status);
        }
	    else if (srv->lsp_type != IAL_LSP_PE_UP && srv->lsp_type_down== IAL_LSP_PE_DOWN)
	    {
            cli_out (cli, "%s\n", in_lsp_status);
        }
	    else if (srv->lsp_type == IAL_LSP_PE_UP && srv->lsp_type_down== IAL_LSP_PE_DOWN)
	    {
            cli_out (cli, "%s\n%s\n", in_lsp_status, out_lsp_status);
        }
    }
	else
	{
	    cli_out (cli, "lsp %d\n", msgdata.lsp_id);
	}
		
	return CLI_ERROR;
    //return CLI_SUCCESS;
}


/*wmx@141231 增加car命令*/
CLI(lsp_car,
	lsp_car_cmd,
    "car (disable|enable cir <0-1000000> cbs <0-4095> pir <0-1000000> pbs <0-4095>)",
    "config car paras",
    "disable car",
    "enable car",
	"committed information rate",
    "rate value, kbps",
    "committed brust size",
	"brust size value, bytes",
	"peak information rate",
	"rate value, kbps",
	"peak brust size",
	"brust size value, bytes")
   {
	   //struct vport_pw *pw;
	   //struct vport_tun *tun;
	   struct service_lsp *srv;
	
   
	   BOOL is_enable = (0 == pal_strcmp("disable", argv[0])) ? FALSE : TRUE;
	   UINT32 cir = 0;
	   UINT32 cbs = 0;
	   UINT32 pir = 0;
	   UINT32 pbs = 0;
	   //u8 priority = 0;
	   //u8 weight = 0;
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
		   else if (0 == pal_strcmp("pir", argv[i]))
		   {
			   pir = atoi(argv[i + 1]);
		   }
		   else if (0 == pal_strcmp("pbs", argv[i]))
		   {
			   pbs = atoi(argv[i + 1]);
		   }
		   
		 /*  else if (0 == pal_strcmp("priority", argv[i]))
		   { 
				priority = atoi(argv[i + 1]);
		   }
		   */
		   
	   }
   
	   if ((!IS_CAR_XIR_VALID(cir)) || (! IS_CAR_XIR_VALID(pir)))
	   {
		   cli_out(cli, "%% input information rate error.\n");
		   return CLI_ERROR;
	   }
   
	   if ((!IS_CAR_XBS_VALID(cbs)) || (! IS_CAR_XBS_VALID(pbs)))
	   {
		   cli_out(cli, "%% input brust size error.\n");
		   return CLI_ERROR;
	   }
   
	    srv = service_get_current_lsp(cli);
       if (NULL == srv)
       {
          cli_out(cli, "%% current lsp does not exist.\n");
          return CLI_ERROR;
       }
		     struct hal_car_info_s msgdata = {0};
		     if (srv->car.is_enable)
		     {
			   msgdata.sev_index= srv->id;
			   msgdata.enable = FALSE;
			   //msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
			   msgdata.portId = vport_clac_if_index(srv->in_port_type, srv->in_port_id);
			   msgdata.i_tag_label = 0;
			   msgdata.o_tag_label = srv->in_label;
			   comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
		   }
   
		   if (is_enable)
		   {
			   msgdata.sev_index = srv->id;
			   msgdata.enable = is_enable;
			   //msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
			   msgdata.portId = vport_clac_if_index(srv->in_port_type, srv->in_port_id);
			   msgdata.i_tag_label = 0;
			   msgdata.o_tag_label = srv->in_label;
			   msgdata.cir = cir;
			   msgdata.cbs = cbs;
			   msgdata.pir = pir;
			   msgdata.pbs = pbs;
			  // msgdata.priority = priority;
			   //msgdata.weight = weight;
			   msgdata.fwdRed = fwd_red;
			   msgdata.drpYellow = drp_yellow;
			   comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
		   }
   
	   
   
	   SET_CAR_DATA(srv->car, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);
	   //srv->priority = priority;
	   //srv->weight = weight;
	   
   
	   zlog_info(NSM_ZG, "set lsp_%d car: is_enable:%d, cir:%d, cbs:%d, pir:%d, pbs:%d,fwd_red:%d, drp_yellow:%d.",
		   srv->id, is_enable, cir, cbs, pir, pbs,fwd_red, drp_yellow);
   
	   return CLI_SUCCESS;
   }


void hal_show_one_lsp_status(struct service_lsp *srv, void *para)
{
    ial_mpls_lsp_status msgdata, resp;
	UINT32 lsp_egress_status, lsp_ingress_status;

	lsp_egress_status = srv->lsp_egress_status;
	lsp_ingress_status = srv->lsp_ingress_status;
	
    msgdata.lsp_id = srv->id;
	msgdata.lsp_egress_status = IAL_LSP_STATUS_INVALID;
	msgdata.lsp_ingress_status = IAL_LSP_STATUS_INVALID;

    comm_send_hal_msg(HAL_MSG_LSP_GET_LINK_STATUS, &msgdata, sizeof(msgdata), _hal_lsp_status_get, &resp);

	if(resp.lsp_egress_status != lsp_egress_status)
	{
	    if(resp.lsp_egress_status == IAL_LSP_STATUS_UP)
        {   
            zlog_warn(NSM_ZG, "lsp %d egress up", srv->id);
	    }
	    else if(resp.lsp_egress_status == IAL_LSP_STATUS_DOWN)
	    {
		    zlog_warn(NSM_ZG, "lsp %d egress down", srv->id);
	    }
	    else
	    {
		    zlog_warn(NSM_ZG, "lsp %d egress invalid", srv->id);
	    }

		srv->lsp_egress_status = resp.lsp_egress_status;//保存更新后的端口状态
	}

	if(resp.lsp_ingress_status != lsp_ingress_status)
	{
	    if(resp.lsp_ingress_status == IAL_LSP_STATUS_UP)
        {   
            zlog_warn(NSM_ZG, "lsp %d ingress up", srv->id);
	    }
	    else if(resp.lsp_ingress_status == IAL_LSP_STATUS_DOWN)
	    {
		    zlog_warn(NSM_ZG, "lsp %d ingress down", srv->id);
	    }
	    else
	    {
		    zlog_warn(NSM_ZG, "lsp %d ingress invalid", srv->id);
	    }

		srv->lsp_ingress_status = resp.lsp_ingress_status;
	}
	zlog_info(NSM_ZG, "lsp %d test2", srv->id);
}

int hal_show_lsp_status(struct thread * t)
{
    service_foreach_lsp(hal_show_one_lsp_status, NULL);
	zlog_info(NSM_ZG, "show lsp test1");
	l2_start_timer (hal_show_lsp_status,
                        NULL, SECS_TO_TICS(1), NSM_ZG);
}

void  hal_lsp_log_init()
{
    /*pthread_t *thread;
	if((thread = (pthread_t *)malloc(sizeof(pthread_t))) == NULL)
	{
		zlog_info(NSM_ZG, "%s,%s,%s Out Of Memory!.\n", __FILE__, __func__, __LINE__);
	}
	if(pthread_create(thread, NULL, hal_show_lsp_status, NULL) != 0)
	{
		zlog_info(NSM_ZG, "%s,%s,%s create thread error!.\n", __FILE__, __func__, __LINE__);
	}*/	
	l2_start_timer (hal_show_lsp_status,
                        NULL, SECS_TO_TICS(1), NSM_ZG);
}


/*
 * 新增lsp
 * 旧命令
 */
/*CLI(lsp_id,
    lsp_id_cmd,
    "lsp <1-2000>",
    CLI_LSP_MODE_STR,
    "lsp id, range 1-2000")
{
    struct service_lsp *srv;
    int index = atoi(argv[0]);

    if (!IS_SERVICE_INDEX_VALID(index))
    {
        cli_out (cli, "%% the lsp id is not validate.\n");
        return CLI_ERROR;
    }

    srv = service_find_lsp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp %s does not exist, create it now.\n", argv[0]);
		srv = service_new_lsp(index);
        if (NULL == srv)
		{
            cli_out(cli, "%% create lsp fail.\n");
            return CLI_ERROR;
        }
 	}

     cli->index = srv;
     cli->mode = LSP_MODE;

     return CLI_SUCCESS;
}*/

/*
 * 删除lsp
 * 旧命令
 */
/*CLI (no_lsp_id,
     no_lsp_id_cmd,
     "no lsp <1-2000>",
     CLI_NO_STR,
     "delete a lsp's configuration",
     "lsp id, range 1-2000")
{
    int index = atoi(argv[0]);
    struct service_lsp *srv;
    struct vport_tun *ingress_tun, *egress_tun;

    hsl_bcm_msg_mpls_tunnel_cfg msgdata;
    //struct hal_car_info_s car_msgdata = {0};

    srv = service_find_lsp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp %s does not exist.\n", argv[0]);
        return CLI_ERROR;
 	}

    if ((!IS_VPORT_INDEX_VALID(srv->tun[INGRESS].id)) || (!IS_VPORT_INDEX_VALID(srv->tun[EGRESS].id)))
    {
        //cli_out(cli, "%% ingress tunnel or egress tunnel id errort.\n");
        service_delete_lsp(srv);
        return CLI_SUCCESS;
    }

    ingress_tun = vport_find_tun_by_index(srv->tun[INGRESS].id);
    egress_tun = vport_find_tun_by_index(srv->tun[EGRESS].id);
    if ((NULL == ingress_tun) || (NULL == egress_tun))
    {
        cli_out(cli, "%% ingress tunnel or egress tunnel not exist.\n");
        return CLI_ERROR;
    }

    // config sdk
    if (ingress_tun->car.is_enable)
    {
        car_msgdata.sev_index=ingress_tun->index;
        car_msgdata.enable = FALSE;
        car_msgdata.portId = vport_clac_if_index(ingress_tun->data[INGRESS].intf_type, ingress_tun->data[INGRESS].intf_id);
        car_msgdata.i_tag_label = 0;
        car_msgdata.o_tag_label = ingress_tun->data[INGRESS].lable;
        comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &car_msgdata, sizeof(car_msgdata));
    }

    msgdata.in_port = vport_clac_if_index(ingress_tun->data[INGRESS].intf_type, ingress_tun->data[INGRESS].intf_id);
    msgdata.in_label = ingress_tun->data[INGRESS].lable;
    msgdata.tunnel_in_vlan = ingress_tun->data[INGRESS].vlan;


    msgdata.out_port = vport_clac_if_index(egress_tun->data[EGRESS].intf_type, egress_tun->data[EGRESS].intf_id);
    msgdata.out_lable = egress_tun->data[EGRESS].lable;
    msgdata.tunnel_out_vlan = egress_tun->data[EGRESS].vlan;
    msgdata.out_tunnel_exp = egress_tun->data[EGRESS].exp;
    pal_mem_cpy(msgdata.tunnel_out_mac, egress_tun->data[EGRESS].peer_mac, ETHER_ADDR_LEN);

    comm_send_hal_msg(HAL_MSG_LSP_DEL, &msgdata, sizeof(msgdata));

    UNBIND_VPORT_FROM_SERVICE(ingress_tun, INGRESS, SERVICE_LSP, srv->index);
    UNBIND_VPORT_FROM_SERVICE(egress_tun, EGRESS, SERVICE_LSP, srv->index);
    service_delete_lsp(srv);

    return CLI_SUCCESS;
}*/

/*
 * 配置pw
 */
/*CLI(lsp_config,
    lsp_config_cmd,
    "config ingress-tun <1-2000> egress-tun <1-2000>",
    "config LSP paras: ingress tun and egress tun",
    "ingress tunnel",
    "ingress tunnel id, range 1-2000",
    "egress tunnel",
    "egress tunnel id, range 1-2000")
{
    struct service_lsp *srv;
    struct vport_tun *ingress_tun, *egress_tun;
    UINT32 ingress_id = atoi(argv[0]);
    UINT32 egress_id = atoi(argv[1]);

    hsl_bcm_msg_mpls_tunnel_cfg msgdata;
    //struct hal_car_info_s car_msgdata = {0};

    if ((!IS_VPORT_INDEX_VALID(ingress_id)) || (!IS_VPORT_INDEX_VALID(egress_id)))
    {
        cli_out(cli, "%% input id error.\n");
        return CLI_ERROR;
    }

    ingress_tun = vport_find_tun_by_index(ingress_id);
    egress_tun = vport_find_tun_by_index(egress_id);
    if ((NULL == ingress_tun) || (NULL == egress_tun))
    {
        cli_out(cli, "%% ingress tunnel or egress tunnel not exist.\n");
        return CLI_ERROR;
    }

    if ((!IS_VPORT_VALID(ingress_tun, INGRESS)) || (!IS_VPORT_VALID(egress_tun, EGRESS)))
    {
        cli_out(cli, "%% ingress tunnel or egress tunnel is not configed.\n");
        return CLI_ERROR;
    }

    if ((IS_VPORT_USED(ingress_tun, INGRESS)) || (IS_VPORT_USED(egress_tun, EGRESS)))
    {
        cli_out(cli, "%% ingress tunnel or egress tunnel is already used.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_lsp(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp does not exist.\n");
        return CLI_ERROR;
    }

    if ((IS_VPORT_INDEX_VALID(srv->tun[INGRESS].id)) || (IS_VPORT_INDEX_VALID(srv->tun[EGRESS].id)))
    {
        cli_out(cli, "%% current lsp is alread configed.\n");
        return CLI_ERROR;
    }

    BIND_VPORT_TO_SERVICE(ingress_tun, INGRESS, SERVICE_LSP, srv->index);
    BIND_VPORT_TO_SERVICE(egress_tun, EGRESS, SERVICE_LSP, srv->index);
    SET_LSP_DATA(srv, ingress_id, egress_id);

    // config sdk
    msgdata.in_port = vport_clac_if_index(ingress_tun->data[INGRESS].intf_type, ingress_tun->data[INGRESS].intf_id);
    msgdata.in_label = ingress_tun->data[INGRESS].lable;
    msgdata.tunnel_in_vlan = ingress_tun->data[INGRESS].vlan;


    msgdata.out_port = vport_clac_if_index(egress_tun->data[EGRESS].intf_type, egress_tun->data[EGRESS].intf_id);
    msgdata.out_lable = egress_tun->data[EGRESS].lable;
    msgdata.tunnel_out_vlan = egress_tun->data[EGRESS].vlan;
    msgdata.out_tunnel_exp = egress_tun->data[EGRESS].exp;
    pal_mem_cpy(msgdata.tunnel_out_mac, egress_tun->data[EGRESS].peer_mac, ETHER_ADDR_LEN);

    comm_send_hal_msg(HAL_MSG_LSP_ADD, &msgdata, sizeof(msgdata));

    zlog_info(NSM_ZG, "set lsp%d[%d] data: in-id %d out-type: %d out-id: %d.",
        srv->index, ingress_id, egress_id);

    return CLI_SUCCESS;
}*/

void lsp_write_single_config(struct service_lsp *srv, void *para)
{
    struct cli *cli = (struct cli *)para;
	char in_port_type[5],out_port_type[5];
	char *tmp;

    pal_assert(srv);
    pal_assert(cli);

	if( srv->in_port_type == INTF_FE)
    {
        tmp = "fe";
		pal_mem_cpy(in_port_type, tmp, 5);
    }
	else
    {
        tmp = "ge";
		pal_mem_cpy(in_port_type, tmp, 5);
    }
	
	if( srv->out_port_type == INTF_FE)
    {
        tmp = "fe";
		pal_mem_cpy(out_port_type, tmp, 5);
    }
	else
    {
        tmp = "ge";
		pal_mem_cpy(out_port_type, tmp, 5);
    }

	cli_out (cli, "lsp %d\n",srv->id);
	//cli_out (cli, " log file /test.log\n");
	if( srv->lsp_type == IAL_LSP_PE_UP)
    {
		cli_out (cli, " config egress %s %d label %d",
				 out_port_type, srv->out_port_id, srv->out_label);
		if (IS_VLAN_VALID(srv->tunnel_out_vlan))
		cli_out (cli, " vlan %d",srv->tunnel_out_vlan);
        cli_out (cli, " exp %d peer-mac %04hx.%04hx.%04hx\n",
				 srv->out_tunnel_exp,*(unsigned short *)&srv->tunnel_out_mac[0],*(unsigned short *)&srv->tunnel_out_mac[2],*(unsigned short *)&srv->tunnel_out_mac[4]);
		cli_out (cli, "!\n");
    }
	else if (srv->lsp_type == IAL_LSP_P_SWITCH)
    {
        cli_out (cli, " config ingress %s %d label %d",
				 in_port_type, srv->in_port_id, srv->in_label, srv->tunnel_in_vlan);
		if (IS_VLAN_VALID(srv->tunnel_in_vlan))
			cli_out (cli, " vlan %d",srv->tunnel_in_vlan);
		cli_out (cli, " egress %s %d label %d",
				 out_port_type, srv->out_port_id, srv->out_label);
		if (IS_VLAN_VALID(srv->tunnel_out_vlan))
		cli_out (cli, " vlan %d",srv->tunnel_out_vlan);
        cli_out (cli, " exp %d peer-mac %04hx.%04hx.%04hx\n",
				 srv->out_tunnel_exp, *(unsigned short *)&srv->tunnel_out_mac[0],*(unsigned short *)&srv->tunnel_out_mac[2],*(unsigned short *)&srv->tunnel_out_mac[4]);
		cli_out (cli, "!\n");
		return;//lsp type为p switch则必不是pe down
    }
	if( srv->lsp_type_down == IAL_LSP_PE_DOWN)
    {
        cli_out (cli, " config ingress %s %d label %d",
				 in_port_type, srv->in_port_id, srv->in_label, srv->tunnel_in_vlan);
		if (IS_VLAN_VALID(srv->tunnel_in_vlan))
			cli_out (cli, " vlan %d\n",srv->tunnel_in_vlan);
		else
			cli_out (cli, "\n");
		cli_out (cli, "!\n");
    }
	/*wmx@150110 增lsp car的回显打印*/
	if (srv->car.is_enable)
	   {
		   cli_out(cli, " car enable cir %d cbs %d pir %d pbs %d\n",
				   srv->car.cir, srv->car.cbs, srv->car.pir, srv->car.pbs);
	   }
	
	   cli_out(cli, "!\n");

   /* char prot_type[10],flag[10];
	if( srv->lsp_type == IAL_LSP_PE_UP)
    {
        tmp = "peup";
		pal_mem_cpy(lsp_type_tmp, tmp, 10);
    }
	else if( srv->lsp_type == IAL_LSP_PE_DOWN)
    {
        tmp = "pedown";
		pal_mem_cpy(lsp_type_tmp, tmp, 10);
    }
	else
    {
        tmp = "pswitch";
		pal_mem_cpy(lsp_type_tmp, tmp, 10);
    }
	
	if( srv->in_port_type == INTF_FE)
    {
        tmp = "fe";
		pal_mem_cpy(in_port_type, tmp, 5);
    }
	else
    {
        tmp = "ge";
		pal_mem_cpy(in_port_type, tmp, 5);
    }
    lsp-group <0-63> protect-type (1plus1|1plus1snc|1by1|1by1snc) sw-back-flag (enable|disable) back-time <1-1000> work-lsp-id <1-2000> backup-lsp-id <1-2000>*/
	
    return;
}

/*
 * 打印lsp配置信息
 */
int lsp_write_config(struct cli *cli)
{
    service_foreach_lsp(lsp_write_single_config, cli);
    return CLI_SUCCESS;
}

void lsp_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, LSP_MODE);
    cli_install_config(ctree,LSP_MODE, lsp_write_config);
	cli_install(ctree, CONFIG_MODE, &lsp_id_cmd);
	cli_install(ctree, LSP_MODE, &lsp_cfg_switch_cmd);
	cli_install(ctree, LSP_MODE, &lsp_cfg_up_cmd);
	cli_install(ctree, LSP_MODE, &lsp_cfg_down_cmd);
    cli_install(ctree, CONFIG_MODE, &no_lsp_id_cmd);
    cli_install(ctree, CONFIG_MODE, &show_lsp_status_cmd);
    cli_install(ctree, LSP_MODE, &lsp_car_cmd);          //wmx@141231
    //cli_install(ctree, CONFIG_MODE, &lsp_id_cmd);
    //cli_install(ctree, CONFIG_MODE, &no_lsp_id_cmd);
    //cli_install(ctree, LSP_MODE, &lsp_config_cmd);
    //hal_lsp_log_init();
}
