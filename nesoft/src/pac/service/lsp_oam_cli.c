/******************************************************************************
 *
 *  pac/service/lsp_oam_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2013 Deng Jingen <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  lsp(标签交换路径) oam CLI部分，创建于2013.3.5
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "service.h"
#include "hsl/hal_vpls.h"
#include "hsl/hal_car.h"

#include "hsl/hal_netlink.h"//struct hal_nlmsghdr定义在这里

/*
 * 新增lsp oam
 */
CLI(lsp_oam_id,
    lsp_oam_id_cmd,
    "lsp-oam <0-63>",
    CLI_LSP_GRP_MODE_STR,
    "lsp oam id, range 0-63")
{
    struct service_lsp_oam *srv;
    int index = atoi(argv[0]);

    if (!IS_LSP_OAM_INDEX_VALID(index))
    {
        cli_out (cli, "%% the lsp oam id is not validate.\n");
        return CLI_ERROR;
    }

    srv = service_find_lspoam_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp oam %s does not exist, create it now.\n", argv[0]);
		srv = service_new_lsp_oam(index);
        if (NULL == srv)
		{
            cli_out(cli, "%% create lsp oam fail.\n");
            return CLI_ERROR;
        }
 	}

     cli->index = srv;
     cli->mode = LSP_OAM_MODE;

     return CLI_SUCCESS;
}

/*
 * 配置隧道oam参数
 */
CLI(lsp_oam_cfg,
    lsp_oam_cfg_cmd,
    "config lsp-id <1-2000> ccm-period <1-10000> ma-name MANAME mep-name MAPNAME ingress (fe <1-24>|ge <1-4>) egress (fe <1-24>|ge <1-4>)",
    "config lsp oam command",
    "lsp id",
    "lsp id value",
    "ccm period",
    "ccm period value",
    "ma name",
    "ma name character",
    "mep name",
    "mep name character",
    "loopback inport",
    "loopback interface type",
    "fe interface id",
    "loopback interface type",
    "ge interface id",
    "loopback outport",
    "loopback interface type",
    "fe interface id",
    "loopback interface type",
    "ge interface id")
{
    struct service_lsp_oam *srv;
	struct service_lsp *srv_lsp;
	struct service_lsp *old_srv_lsp;
    int lsp_id = atoi(argv[0]);
	int ccm_period = atoi(argv[1]);
	//char ma_name[48];
	char mep_name[2];
	enum em_intf_type in_intf_type = INTF_NONE;
    UINT16 in_intf_id = 0;
	enum em_intf_type out_intf_type = INTF_NONE;
	UINT16 out_intf_id = 0;
	ial_oam_cfg msgdata;

	if (0 == pal_strcmp("fe", argv[4]))
    {
        in_intf_type = INTF_FE;
        in_intf_id = atoi(argv[5]);
    }
    else if (0 == pal_strcmp("ge", argv[4]))
    {
        in_intf_type = INTF_GE;
        in_intf_id = atoi(argv[5]);
    }
	if (0 == pal_strcmp("fe", argv[6]))
    {
        out_intf_type = INTF_FE;
        out_intf_id = atoi(argv[7]);
    }
    else if (0 == pal_strcmp("ge", argv[6]))
    {
        out_intf_type = INTF_GE;
        out_intf_id = atoi(argv[7]);
    }
	
    if (!IS_SERVICE_INDEX_VALID(lsp_id))
    {
        cli_out (cli, "%% lsp id is not validate.\n");
        return CLI_ERROR;
    }
	if (!IS_CCM_PERIOD_VALID(ccm_period))
    {
        cli_out (cli, "%% ccm period id is not validate.\n");
        return CLI_ERROR;
    }

    if ( pal_strlen(argv[2]) > MAX_MA_NAME_NUM)
    {
        cli_out (cli, "%% ma name is longer than 47 characters.\n");
		return CLI_ERROR;
    }
	if ( pal_strlen(argv[3]) > MAX_MEP_NAME_NUM)
    {
        cli_out (cli, "%% mep name is longer than 1 characters.\n");
		return CLI_ERROR;
    }
	if (!IS_INTF_VALID(in_intf_type, in_intf_id))
    {
        cli_out(cli, "%% input in inerface info error.\n");
        return CLI_ERROR;
    }
	if (!IS_INTF_VALID(out_intf_type, out_intf_id))
    {
        cli_out(cli, "%% input out inerface info error.\n");
        return CLI_ERROR;
    }
	
    srv_lsp = service_find_lsp_by_index(lsp_id);
	if (NULL == srv_lsp)
 	{
        cli_out (cli, "%% lsp %d does not exist.\n", lsp_id);
		return CLI_ERROR;
	}
	if (!IS_LSP_VALID(srv_lsp))
    {
    	cli_out(cli, "%% target lsp does not valid.\n");
        return CLI_ERROR;
    }
	
	
	srv = service_get_current_lsp_oam(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp oam does not exist.\n");
        return CLI_ERROR;
    }

	old_srv_lsp = service_find_lsp_by_index(srv->lsp_id);
    if ((NULL != old_srv_lsp) && (old_srv_lsp->oam_num > 0))
    {
        old_srv_lsp->oam_num--;
    }
	srv_lsp->oam_num++;
    
	srv->lsp_id = lsp_id;
	srv->ccm_period = ccm_period;
	pal_strcpy(srv->ma_name,argv[2]);
	pal_strcpy(mep_name,argv[3]);
	srv->mep_name = mep_name[0];
	srv->in_port_type = in_intf_type;
	srv->in_port_id = in_intf_id;
	srv->out_port_type = out_intf_type;
	srv->out_port_id = out_intf_id;
	//zlog_info (NSM_ZG, "config service_lsp_oam mep name %s %c %dsuccessfully.\n", argv[3],srv->mep_name,srv->mep_name);

    msgdata.oam_id = srv->oam_id;
	msgdata.lsp_id = srv->lsp_id;
	msgdata.ccm_period = srv->ccm_period;
	pal_strcpy(msgdata.ma_name,srv->ma_name);
	msgdata.mep_name = srv->mep_name;
	msgdata.in_port = vport_clac_if_index(srv->in_port_type, srv->in_port_id);
	msgdata.out_port = vport_clac_if_index(srv->out_port_type, srv->out_port_id);
	comm_send_hal_msg(HAL_MSG_OAM_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

	srv->config_flag = 1;

     return CLI_SUCCESS;
}

/*
 * 删除隧道oam
 */
CLI (no_lsp_oam,
     no_lsp_oam_cmd,
     "no lsp-oam <0-63>",
     CLI_NO_STR,
     "delete a lsp oam's configuration",
     "lsp oam id, range 0-63")
{
    int index = atoi(argv[0]);
    struct service_lsp_oam *srv;
	struct service_lsp *lsp;

    struct
	{
	    unsigned int id;
	}msgdata;
    //struct hal_car_info_s car_msgdata = {0};

    if (!IS_LSP_OAM_INDEX_VALID(index))
    {
        cli_out(cli, "%% ingress lsp oam id error.\n");
        return CLI_ERROR;
    }
    srv = service_find_lspoam_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp oam %s does not exist.\n", argv[0]);
        return CLI_ERROR;
 	}

    lsp = service_find_lsp_by_index(srv->lsp_id);
    if ((NULL != lsp) && (lsp->oam_num > 0))
    {
        lsp->oam_num--;
    }
	
    msgdata.id = index;

    comm_send_hal_msg( HAL_MSG_OAM_DEL, &msgdata, sizeof(msgdata), NULL, NULL);

    service_delete_lspoam(srv);

    return CLI_SUCCESS;
}

/* 
   Callback function for response of lsp oam status get. 
*/
static int
_hal_lsp_oam_status_get (struct hal_nlmsghdr *h, void *data)
{
  ial_oam_status *tmp, *resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (ial_oam_status  *)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);
  tmp = (ial_oam_status  *)pnt;

  /* Decode message. */
  
  if (size < sizeof(ial_oam_status))
    return -1;
  resp->rdi_error = tmp->rdi_error;
  resp->xcon_error = tmp->xcon_error;
  resp->ccm_timeout = tmp->ccm_timeout;
  resp->unexpected_mep = tmp->unexpected_mep;

  return 0;
}

/*
 * 获取lsp oam信息
 */
CLI (show_lspoam_status,
     show_lspoam_status_cmd,
     "show lsp-oam-status <0-63>",
     "show command",
     "lsp oam status",
     "lsp oam id")
{
    int index = atoi(argv[0]);
    struct service_lsp_oam *srv;
    ial_oam_status msgdata, resp;

    if (!IS_LSP_OAM_INDEX_VALID(index))
    {
        cli_out(cli, "%% ingress lsp oam id error.\n");
        return CLI_ERROR;
    }
    srv = service_find_lspoam_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp oam %s does not exist.\n", argv[0]);
        return CLI_ERROR;
 	}

    msgdata.oam_id = index;
	msgdata.ccm_timeout = 0;
	msgdata.lsp_id = 0;
	msgdata.rdi_error = 0;
	msgdata.xcon_error = 0;
	msgdata.unexpected_mep = 0;

    comm_send_hal_msg(HAL_MSG_OAM_GET_STATUS, &msgdata, sizeof(msgdata), _hal_lsp_oam_status_get, &resp);

    zlog_info(NSM_ZG, "lsp oam %d ",msgdata.oam_id);
	zlog_info(NSM_ZG, " lsp id %d",msgdata.lsp_id);
	zlog_info(NSM_ZG, " rdi_error value %d",msgdata.rdi_error);
	zlog_info(NSM_ZG, " xcon_error value %d",msgdata.xcon_error);
	zlog_info(NSM_ZG, " ccm timeout value %d",msgdata.ccm_timeout);
	zlog_info(NSM_ZG, " unexpected mep %d.\n",msgdata.unexpected_mep);

    if (srv->config_flag == 1)
	cli_out (cli, "rdi-error value %d\nxcon-error value %d\nccm-timeout value %d\nunexpected-mep %d\n",
		resp.rdi_error, resp.xcon_error, resp.ccm_timeout, resp.unexpected_mep);
	else 
		cli_out (cli, "lsp oam %d\n", msgdata.oam_id);

    return CLI_ERROR;
    //return CLI_SUCCESS;
}

void lsp_oam_write_single_config(struct service_lsp_oam *srv, void *para)
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
	
	cli_out (cli, "lsp-oam %d\n",srv->oam_id);
	cli_out (cli, " config lsp-id %d ccm-period %d ma-name %s mep-name %c ingress %s %d egress %s %d\n",
				  srv->lsp_id, srv->ccm_period, srv->ma_name,srv->mep_name,in_port_type, srv->in_port_id,out_port_type, srv->out_port_id);
    cli_out (cli, "!\n");
	
    return;
}

/*
 * 打印lsp配置信息
 */
int lsp_oam_write_config(struct cli *cli)
{
    service_foreach_lsp_oam(lsp_oam_write_single_config, cli);
    return CLI_SUCCESS;
}

void lsp_oam_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, LSP_OAM_MODE);
    cli_install_config(ctree,LSP_OAM_MODE, lsp_oam_write_config);
	cli_install(ctree, CONFIG_MODE, &lsp_oam_id_cmd);
	cli_install(ctree, LSP_OAM_MODE, &lsp_oam_cfg_cmd);
	cli_install(ctree, CONFIG_MODE, &no_lsp_oam_cmd);
	cli_install(ctree, CONFIG_MODE, &show_lspoam_status_cmd);
}
