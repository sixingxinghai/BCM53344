/******************************************************************************
 *
 *  pac/service/lsp_grp_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2013 Deng Jingen <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  lsp(标签交换路径) protect group CLI部分，创建于2013.3.5
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "service.h"
#include "hsl/hal_vpls.h"
#include "hsl/hal_car.h"

#include "hsl/hal_netlink.h"//struct hal_nlmsghdr定义在这里

/*
 * 新增lsp保护组
 */
CLI(lsp_grp_id,
    lsp_grp_id_cmd,
    "lsp-protect-group <0-63>",
    CLI_LSP_GRP_MODE_STR,
    "lsp protect group id, range 0-63")
{
    struct service_lsp_group *srv;
    int index = atoi(argv[0]);

    if (!IS_LSP_PROTGRP_INDEX_VALID(index))
    {
        cli_out (cli, "%% the lsp protect group id is not validate.\n");
        return CLI_ERROR;
    }

    srv = service_find_lspgrp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp protect group %s does not exist, create it now.\n", argv[0]);
		srv = service_new_lsp_group(index);
        if (NULL == srv)
		{
            cli_out(cli, "%% create lsp protect group fail.\n");
            return CLI_ERROR;
        }
 	}

     cli->index = srv;
     cli->mode = LSP_GRP_MODE;

     return CLI_SUCCESS;
}

/*
 * 配置隧道保护组参数
 */
CLI(lsp_group_cfg,
    lsp_group_cfg_cmd,
    "config protect-type (1plus1|1plus1snc|1by1|1by1snc) work-lsp-id <1-2000> backup-lsp-id <1-2000>",
    "lsp protect group config command"
    "protect type",
    "1PLUS1 1 + 1",
    "1PLUS1 SNC",
    "1BY1 1:1",
    "1BY1SNC",
    "work lsp id",
    "work lsp id value",
    "backup lsp id",
    "backup lsp id value")
{
    struct service_lsp_group *srv;
    struct service_lsp *srv_work,*srv_back;
	struct service_lsp *old_srv_work;
	ial_protect_type type;
	int work_id = atoi(argv[1]);
	int back_id = atoi(argv[2]);
	ial_mpls_protect_group_cfg msgdata;

    if (!IS_SERVICE_INDEX_VALID(work_id))
    {
        cli_out (cli, "%% work lsp id is not validate.\n");
        return CLI_ERROR;
    }
	if (!IS_SERVICE_INDEX_VALID(back_id))
    {
        cli_out (cli, "%% back lsp id is not validate.\n");
        return CLI_ERROR;
    }

    if (0 == pal_strcmp("1plus1", argv[0]))
    {
        type = IAL_PROTECT_TYPE_1PLUS1;
    }
	else if (0 == pal_strcmp("1plus1snc", argv[0]))
    {
        type = IAL_PROTECT_TYPE_1PLUS1_SNC;
    }
    else if (0 == pal_strcmp("1by1", argv[0]))
    {
        type = IAL_PROTECT_TYPE_1BY1;
    }
	else if (0 == pal_strcmp("1by1snc", argv[0]))
    {
        type = IAL_PROTECT_TYPE_1BY1_SNC;
    } 
	
    srv_work = service_find_lsp_by_index(work_id);
	if (NULL == srv_work)
 	{
        cli_out (cli, "%% work lsp %d does not exist.\n", work_id);
		return CLI_ERROR;
	}
	if (!IS_LSP_VALID(srv_work))
    {
    	cli_out(cli, "%% target work-lsp does not valid.\n");
        return CLI_ERROR;
    }
	srv_back = service_find_lsp_by_index(back_id);
	if (NULL == srv_back)
 	{
        cli_out (cli, "%% backup lsp %d does not exist.\n", back_id);
		return CLI_ERROR;
	}
	if (!IS_LSP_VALID(srv_back))
    {
    	cli_out(cli, "%% target back-lsp does not valid.\n");
        return CLI_ERROR;
    }
	
	srv = service_get_current_lsp_grp(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp protect group does not exist.\n");
        return CLI_ERROR;
    }

	
    old_srv_work = service_find_lsp_by_index(srv->work_lsp_id);
    if ((NULL != old_srv_work) && (old_srv_work->grp_num > 0))
    {
        old_srv_work->grp_num--;
    }
	srv_work->grp_num++;
    
	srv->protect_type = IAL_PROTECT_TYPE_1BY1;//现在只支持IAL_PROTECT_TYPE_1BY1
	srv->work_lsp_id = work_id;
	srv->backup_lsp_id = back_id;

	msgdata.grp_id = srv->grp_id;
	msgdata.protect_type = srv->protect_type;
	msgdata.work_lsp_id = srv->work_lsp_id;
	msgdata.backup_lsp_id = srv->backup_lsp_id;
	comm_send_hal_msg(HAL_MSG_LSP_PROTECT_GRP_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

	srv->config_flag = 1;

     return CLI_SUCCESS;
}

/*
 * 删除隧道保护组
 */
CLI (no_lsp_group,
     no_lsp_group_cmd,
     "no lsp-protect-group <0-63>",
     CLI_NO_STR,
     "delete a lsp group's configuration",
     "lsp group id, range 0-63")
{
    int index = atoi(argv[0]);
    struct service_lsp_group *srv;
	struct service_lsp *lsp;

    struct
	{
	    unsigned int id;
	}msgdata;
    //struct hal_car_info_s car_msgdata = {0};

    if (!IS_LSP_PROTGRP_INDEX_VALID(index))
    {
        cli_out(cli, "%% ingress lsp group id error.\n");
        return CLI_ERROR;
    }
    srv = service_find_lspgrp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp group %s does not exist.\n", argv[0]);
        return CLI_ERROR;
 	}

	lsp = service_find_lsp_by_index(srv->work_lsp_id);
    if ((NULL != lsp) && (lsp->grp_num > 0))
    {
        lsp->grp_num--;
    }

    msgdata.id = index;

    comm_send_hal_msg(HAL_MSG_LSP_PROTECT_GRP_DEL, &msgdata, sizeof(msgdata), NULL, NULL);

    service_delete_lspgrp(srv);

    return CLI_SUCCESS;
}

/*
 * 修改隧道保护组参数
 */
CLI(lsp_group_modify,
    lsp_group_modify_cmd,
    "config  sw-back-flag (enable|disable) back-time <1-1000>",
    "lsp protect group config command",
    "switch back flag",
    "enable switch",
    "disable switch",
    "back time",
    "back time value")
{
    struct service_lsp_group *srv;
	ial_protect_switch_back back_flag = (0 == pal_strcmp("disable", argv[0])) ? IAL_PROTECT_SWITCH_BACK_DIS : IAL_PROTECT_SWITCH_BACK_EN;
	int back_time = atoi(argv[1]);
	ial_mpls_protect_group_cfg msgdata;

    srv = service_get_current_lsp_grp(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp protect group does not exist.\n");
        return CLI_ERROR;
    }
	if (!IS_LSP_PROTGRP_BACKTIME_VALID(back_time))
    {
        cli_out (cli, "%% lsp protect group back time is not validate.\n");
        return CLI_ERROR;
    }
	
	srv->sw_back_flag = back_flag;
	srv->back_time = back_time;

	msgdata.grp_id = srv->grp_id;
	msgdata.sw_back_flag = srv->sw_back_flag;
	msgdata.back_time = srv->back_time;
	comm_send_hal_msg(HAL_MSG_LSP_PROTECT_GRP_MOD_PARAM, &msgdata, sizeof(msgdata), NULL, NULL);

     return CLI_SUCCESS;
}

/*
 * 隧道保护组切换
 */
CLI(lsp_group_sw,
    lsp_group_sw_cmd,
    "config current-work-lsp <1-2000> sw-status (clear|lock|force) lock-lsp <1-2000>",
    "lsp protect group config command",
    "switch lsp",
    "switch lsp id",
    "switch status",
    "protect switch clear",
    "protect switch lock",
    "protect switch force",
    "lock lsp",
    "lock lsp id")
{
    struct service_lsp_group *srv;
	struct service_lsp *srv_sw,*srv_lock;
	int sw_id = atoi(argv[0]);
	ial_protect_switch_status sw_status;
	int lock_id = atoi(argv[2]);
	ial_mpls_protect_switch_cfg msgdata;

    srv = service_get_current_lsp_grp(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current lsp protect group does not exist.\n");
        return CLI_ERROR;
    }	

	if (!IS_SERVICE_INDEX_VALID(sw_id))
    {
        cli_out (cli, "%% current work lsp id is not validate.\n");
        return CLI_ERROR;
    }
	if (sw_id != srv->work_lsp_id && sw_id != srv->backup_lsp_id)
    {
        cli_out (cli, "%% current work lsp id must be work lsp id or backup lsp id.\n");
        return CLI_ERROR;
    }
	
	if (!IS_SERVICE_INDEX_VALID(lock_id))
    {
        cli_out (cli, "%% lock lsp id is not validate.\n");
        return CLI_ERROR;
    }
	if (lock_id != srv->work_lsp_id && lock_id != srv->backup_lsp_id)
    {
        cli_out (cli, "%% lock lsp id must be work lsp id or backup lsp id.\n");
        return CLI_ERROR;
    }

    if (0 == pal_strcmp("clear", argv[1]))
    {
        sw_status = IAL_PROTECT_SWITCH_CLEAR;
    }
	else if (0 == pal_strcmp("lock", argv[1]))
    {
        sw_status = IAL_PROTECT_SWITCH_LOCK;
    }
    else if (0 == pal_strcmp("force", argv[1]))
    {
        sw_status = IAL_PROTECT_SWITCH_FORCE;
    }
	
    srv_sw = service_find_lsp_by_index(sw_id);
	if (NULL == srv_sw)
 	{
        cli_out (cli, "%% switch lsp %d does not exist.\n", sw_id);
		return CLI_ERROR;
	}
	if (!IS_LSP_VALID(srv_sw))
    {
    	cli_out(cli, "%% target sw-lsp does not valid.\n");
        return CLI_ERROR;
    }
	srv_lock = service_find_lsp_by_index(lock_id);
	if (NULL == srv_lock)
 	{
        cli_out (cli, "%% lock lsp %d does not exist.\n", lock_id);
		return CLI_ERROR;
	}
	if (!IS_LSP_VALID(srv_lock))
    {
    	cli_out(cli, "%% target lock-lsp does not valid.\n");
        return CLI_ERROR;
    }
	
	msgdata.grp_id = srv->grp_id;
	msgdata.switch_lsp_id = sw_id;
	msgdata.switch_status = sw_status;
	msgdata.lock_lsp_id = lock_id;
	comm_send_hal_msg(HAL_MSG_LSP_PROTECT_GRP_SWITCH, &msgdata, sizeof(msgdata), NULL, NULL);

     return CLI_SUCCESS;
}

/* 
   Callback function for response of lsp group status get. 
*/
static int
_hal_lsp_group_status_get (struct hal_nlmsghdr *h, void *data)
{
  ial_mpls_protect_group_info *tmp, *resp;
  u_char *pnt;
  u_int32_t size;
  int ret;

  resp =  (ial_mpls_protect_group_info  *)data;
  pnt = HAL_NLMSG_DATA (h);
  size = h->nlmsg_len - sizeof (struct hal_nlmsghdr);
  tmp = (ial_mpls_protect_group_info  *)pnt;

  /* Decode message. */
  
  if (size < sizeof(ial_mpls_protect_group_info))
    return -1;
  resp->switch_status = tmp->switch_status;
  resp->back_time = tmp->back_time;
  resp->lock_lsp_id = tmp->lock_lsp_id;
  resp->current_lsp_id = tmp->current_lsp_id;

  return 0;
}

/*
 * 获取lsp隧道隧道保护组信息
 */
CLI (show_lspgrp_status,
     show_lspgrp_status_cmd,
     "show lsp-grp-status <1-2000>",
     "show command",
     "lsp protect group status",
     "lsp protect group id")
{
    int index = atoi(argv[0]);
    struct service_lsp_group *srv;
    ial_mpls_protect_group_info msgdata, resp;
	char protect_type[30], switch_status[30], sw_back_flag[30];
	char *tmp;

    if (!IS_LSP_PROTGRP_INDEX_VALID(index))
    {
        cli_out(cli, "%% ingress lsp protect group id error.\n");
        return CLI_ERROR;
    }
    srv = service_find_lspgrp_by_index(index);
    if (NULL == srv)
 	{
        cli_out (cli, "%% lsp protect group %s does not exist.\n", argv[0]);
        return CLI_ERROR;
 	}

    msgdata.grp_id = index;
	msgdata.backup_lsp_id = 0;
	msgdata.current_lsp_id = 0;
	msgdata.lock_lsp_id = 0;
	msgdata.work_lsp_id = 0;
	msgdata.back_time = 0;

    comm_send_hal_msg(HAL_MSG_LSP_GET_PROTECT_GRP_INFO, &msgdata, sizeof(msgdata), _hal_lsp_group_status_get, &resp);

    zlog_info(NSM_ZG, "lsp protect group %d ",msgdata.grp_id);
    if(resp.protect_type == IAL_PROTECT_TYPE_1PLUS1)
    {
        zlog_info(NSM_ZG, "protect type 1PLUS1");
		tmp = "protect-type 1PLUS1";
		pal_mem_cpy(protect_type, tmp, 30);
	}
	else if(resp.protect_type == IAL_PROTECT_TYPE_1PLUS1_SNC)
	{
        zlog_info(NSM_ZG, "protect type 1PLUS1_SNC");
		tmp = "protect-type 1PLUS1_SNC";
		pal_mem_cpy(protect_type, tmp, 30);
	}
	else if(resp.protect_type == IAL_PROTECT_TYPE_1BY1)
	{
        zlog_info(NSM_ZG, "protect type 1BY1");
		tmp = "protect-type 1BY1";
		pal_mem_cpy(protect_type, tmp, 30);
	}
	else if(resp.protect_type == IAL_PROTECT_TYPE_1BY1_SNC)
	{
        zlog_info(NSM_ZG, "protect type 1BY1_SNC");
		tmp = "protect-type 1BY1_SNC";
		pal_mem_cpy(protect_type, tmp, 30);
	}
	else 
	{
		zlog_info(NSM_ZG, "protect type invalid");
		tmp = "protect-type invalid";
		pal_mem_cpy(protect_type, tmp, 30);
	}
	

	if(resp.switch_status == IAL_PROTECT_SWITCH_CLEAR)
	{
        zlog_info(NSM_ZG, " switch status clear");
		tmp = "switch status clear";
		pal_mem_cpy(switch_status, tmp, 30);
	}
	else if(resp.switch_status == IAL_PROTECT_SWITCH_FORCE)
	{
        zlog_info(NSM_ZG, " switch status force");
		tmp = "switch status force";
		pal_mem_cpy(switch_status, tmp, 30);
	}
	else if(resp.switch_status == IAL_PROTECT_SWITCH_LOCK)
	{
        zlog_info(NSM_ZG, " switch status lock");
		tmp = "switch status lock";
		pal_mem_cpy(switch_status, tmp, 30);
	}
	else 
	{
		zlog_info(NSM_ZG, " switch status invalid");
		tmp = "switch status invalid";
		pal_mem_cpy(switch_status, tmp, 30);
	}

	if(resp.sw_back_flag == IAL_PROTECT_SWITCH_BACK_EN)
	{
        zlog_info(NSM_ZG, " switch back enable");
		tmp = " switch back enable";
		pal_mem_cpy(sw_back_flag, tmp, 30);
	}
	else if(resp.sw_back_flag == IAL_PROTECT_SWITCH_BACK_DIS)
	{
        zlog_info(NSM_ZG, " switch back disable");
		tmp = " switch back disable";
		pal_mem_cpy(sw_back_flag, tmp, 30);
	}
	else 
	{
		zlog_info(NSM_ZG, " switch back invalid");
		tmp = " switch back invalid";
		pal_mem_cpy(sw_back_flag, tmp, 30);
	}

	zlog_info(NSM_ZG, " switch back time %d",resp.back_time);
	zlog_info(NSM_ZG, " lock lsp %d",resp.lock_lsp_id);
	zlog_info(NSM_ZG, " current lsp %d",resp.current_lsp_id);
	zlog_info(NSM_ZG, " work lsp %d",resp.work_lsp_id);
	zlog_info(NSM_ZG, " backup lsp %d.\n",resp.backup_lsp_id);

    if (srv->config_flag == 1)
	    cli_out (cli, "current lsp %d\nlock lsp %d\n%s\nswitch back time %d\n", resp.current_lsp_id, resp.lock_lsp_id, switch_status, resp.back_time);
	else
		cli_out (cli, "lsp protect group %d\n", msgdata.grp_id);

    return CLI_ERROR;
    //return CLI_SUCCESS;
}

void lsp_grp_write_single_config(struct service_lsp_group *srv, void *para)
{
    struct cli *cli = (struct cli *)para;   
	char prot_type[10],flag[10];
	char *tmp;

    pal_assert(srv);
    pal_assert(cli);

	if(srv->protect_type == IAL_PROTECT_TYPE_1PLUS1)
    {
        tmp = "1plus1";
		pal_mem_cpy(prot_type, tmp, 10);
    }
	else if(srv->protect_type == IAL_PROTECT_TYPE_1PLUS1_SNC)
    {
        tmp = "1plus1snc";
		pal_mem_cpy(prot_type, tmp, 10);
    }
	else if(srv->protect_type == IAL_PROTECT_TYPE_1BY1)
    {
        tmp = "1by1";
		pal_mem_cpy(prot_type, tmp, 10);
    }
	else
    {
        tmp = "1by1snc";
		pal_mem_cpy(prot_type, tmp, 10);
    }
	
	if( srv->sw_back_flag == IAL_PROTECT_SWITCH_BACK_EN)
    {
        tmp = "enable";
		pal_mem_cpy(flag, tmp, 10);
    }
	else
    {
        tmp = "disable";
		pal_mem_cpy(flag, tmp, 10);
    }

	cli_out (cli, "lsp-grp %d\n",srv->grp_id);
	cli_out (cli, " config protect-type %s sw-back-flag %s back-time %d work-lsp-id %d backup-lsp-id %d\n",
				 prot_type, flag, srv->back_time, srv->work_lsp_id, srv->backup_lsp_id);
    cli_out (cli, "!\n");
	
    return;
}

/*
 * 打印lsp配置信息
 */
int lsp_grp_write_config(struct cli *cli)
{
    service_foreach_lsp_grp(lsp_grp_write_single_config, cli);
    return CLI_SUCCESS;
}

void lsp_grp_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, LSP_GRP_MODE);
    cli_install_config(ctree,LSP_GRP_MODE, lsp_grp_write_config);
	cli_install(ctree, CONFIG_MODE, &lsp_grp_id_cmd);
	cli_install(ctree, LSP_GRP_MODE, &lsp_group_cfg_cmd);
	cli_install(ctree, CONFIG_MODE, &no_lsp_group_cmd);
	cli_install(ctree, LSP_GRP_MODE, &lsp_group_modify_cmd);
	cli_install(ctree, LSP_GRP_MODE, &lsp_group_sw_cmd);
	cli_install(ctree, CONFIG_MODE, &show_lspgrp_status_cmd);
}
