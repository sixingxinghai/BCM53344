/******************************************************************************
 *
 *  pac/vport/pw_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  pw(Pseudo Wire)CLI部分，创建于2012.2.10
 *
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "hsl/hal_car.h"
#include "service.h"

/*
 * 新增pw
 */
CLI(pw_id,
    pw_id_cmd,
    "pw <1-2000>",
    CLI_PW_MODE_STR,
    "pseudo-wire id, range 1-2000")
{
    struct vport_pw *pw;
    int index = atoi(argv[0]);

    if (!IS_VPORT_INDEX_VALID(index))
    {
         cli_out(cli, "%% the pseudo wire id is not validate.\n");
         return CLI_ERROR;
    }

    pw = vport_find_pw_by_index(index);
    if (NULL == pw)
    {
        cli_out(cli, "%% pseudo wire %s does not exist, create it now.\n", argv[0]);

        pw = vport_new_pw(index);
        if (NULL == pw)
        {
            cli_out(cli, "%% create pseudo wire fail.\n");
            return CLI_ERROR;
        }
    }

    cli->index = pw;
    cli->mode = PW_MODE;

    return CLI_SUCCESS;
}

/*
 * 删除pw
 */
CLI(no_pw_id,
    no_pw_id_cmd,
    "no pw <1-2000>",
    CLI_NO_STR,
    "delete a pseudo-wire's configuration",
    "pseudo-wire id, range 1-2000")
{
    struct vport_pw *pw;
    //struct vport_tun *tun;
    struct service_lsp *lsp;
    int index = atoi(argv[0]);
    enum em_direction dir;

    pw = vport_find_pw_by_index(index);
    if (NULL == pw)
    {
        cli_out(cli, "%% pseudo wire %s does not exist.\n", argv[0]);
        return CLI_ERROR;
    }

	if (IS_PW_USED(pw))
    {
        cli_out(cli, "%% the service instance binded with pw should be unbinded first.\n");
        return CLI_ERROR;
    }


    //for (dir = INGRESS; dir < DIR_BUTT; dir++)
    //{
        //tun = vport_find_tun_by_index(pw->data[dir].tun_id);
        lsp = service_find_lsp_by_index(pw->lsp_id);
        if ((NULL != lsp) && (lsp->pw_num > 0))
        {
            lsp->pw_num--;
        }
   // }

    vport_delete_pw(pw);

    return CLI_SUCCESS;
}

/*
 * 配置pw
 */
/*CLI(pw_config,
    pw_config_cmd,
    "config (ingress lable <0-1048575> tun-id <1-2000>|egress lable <0-1048575> exp <0-7> tun-id <1-2000>)",
    "config pw paras: direction, lable, exp, tunnel",
    "pw direction",
    "ingress-pw lable",
    "lable value",
    "bind ingress-pw to tunnel",
    "tun id",
    "pw direction",
    "egress-pw lable",
    "lable value",
    "egress-pw exp",
    "exp value",
    "bind egress-pw to tunnel",
    "tun id")
{
    struct vport_pw *pw;
    struct vport_tun *old_tun, *tun;

    enum em_direction dir = (0 == pal_strcmp("ingress", argv[0])) ? INGRESS : EGRESS;
    enum em_direction opp_dir = (dir == INGRESS) ? EGRESS : INGRESS;
    UINT32 lable = 0;
    UINT8 exp = 0xFF;
    UINT16 tun_id = 0;
    UINT8 i = 0;

    for (i = 1; i < argc - 1; i += 2)
    {
        if (0 == pal_strcmp("lable", argv[i]))
        {
            lable = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("tun-id", argv[i]))
        {
            tun_id = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("exp", argv[i]))
        {
            exp = atoi(argv[i + 1]);
        }
    }

    if (!IS_DIR_VALID(dir))
    {
        cli_out(cli, "%% input pw direction error.\n");
        return CLI_ERROR;
    }

    if (!IS_LABLE_VALID(lable))
    {
        cli_out(cli, "%% input lable error.\n");
        return CLI_ERROR;
    }

    if (EGRESS == dir)
    {
        if (!IS_EXP_VALID(exp))
        {
            cli_out(cli, "%% input exp error.\n");
            return CLI_ERROR;
        }
    }

    pw = vport_get_current_pw(cli);
    if (NULL == pw)
    {
        cli_out(cli, "%% current pseudo wire does not exist.\n");
        return CLI_ERROR;
    }

    tun = vport_find_tun_by_index(tun_id);
    if (NULL == tun)
    {
        cli_out(cli, "%% target tunnel does not exist.\n");
        return CLI_ERROR;
    }

    if ((!IS_VPORT_VALID(tun, dir)) || IS_VPORT_VALID(tun, opp_dir))
    {
    	cli_out(cli, "%% target tunnel does not valid.\n");
        return CLI_ERROR;
    }

    if (IS_VPORT_USED(pw, dir))
    {
    	cli_out(cli, "%% the service instance binded with pseudo wire should be unbinded first.\n");
        return CLI_ERROR;
    }

    old_tun = vport_find_tun_by_index(pw->data[dir].tun_id);
    if ((NULL != old_tun) && (old_tun->data[dir].pw_num > 0))
    {
        old_tun->data[dir].pw_num--;
    }
    tun->data[dir].pw_num++;

    SET_PW_DATA(pw, dir, lable, exp, tun_id);

    zlog_info(NSM_ZG, "set pw_%d[%d] data: lable: %d exp %d tun: %d.", pw->index, dir, lable, exp, tun_id);

    return CLI_SUCCESS;
}*/

/*
 * 配置pw
 */
CLI(pw_config,
    pw_config_cmd,
    "config ingress lable <0-1048575> egress lable <0-1048575> exp <0-7> lsp-id <1-2000>",
    "config pw paras: direction, lable, exp, lsp",
    "pw direction",
    "ingress-pw lable",
    "lable value",
    "pw direction",
    "egress-pw lable",
    "lable value",
    "egress-pw exp",
    "exp value",
    "bind lsp to tunnel",
    "lsp id")
{
    struct vport_pw *pw;
    struct service_lsp *old_lsp, *lsp;

    UINT32 in_lable = 0;
	UINT32 out_lable = 0;
    UINT8 exp = 0xFF;
    UINT16 lsp_id = 0;
    UINT8 i = 0;

    in_lable = atoi(argv[0]);
    out_lable = atoi(argv[1]);
    exp = atoi(argv[2]);
    lsp_id = atoi(argv[3]);

    if (!IS_LABLE_VALID(in_lable))
    {
        cli_out(cli, "%% input ingress lable error.\n");
        return CLI_ERROR;
    }

    if (!IS_LABLE_VALID(out_lable))
    {
        cli_out(cli, "%% input egress lable error.\n");
        return CLI_ERROR;
    }

    if (!IS_EXP_VALID(exp))
    {
        cli_out(cli, "%% input exp error.\n");
        return CLI_ERROR;
    }

	if (!IS_VPORT_INDEX_VALID(lsp_id))
	{
        cli_out(cli, "%% lsp id error.\n");
		return CLI_ERROR;
	}

    pw = vport_get_current_pw(cli);
    if (NULL == pw)
    {
        cli_out(cli, "%% current pseudo wire does not exist.\n");
        return CLI_ERROR;
    }

    lsp = service_find_lsp_by_index(lsp_id);
    if (NULL == lsp)
    {
        cli_out(cli, "%% target lsp does not exist.\n");
        return CLI_ERROR;
    }

    if (!IS_LSP_VALID(lsp))
    {
    	cli_out(cli, "%% target lsp does not valid.\n");
        return CLI_ERROR;
    }

    if (IS_PW_USED(pw))
    {
    	cli_out(cli, "%% the service instance binded with pseudo wire should be unbinded first.\n");
        return CLI_ERROR;
    }

    /*old_tun = vport_find_tun_by_index(pw->data[dir].tun_id);
    if ((NULL != old_tun) && (old_tun->data[dir].pw_num > 0))
    {
        old_tun->data[dir].pw_num--;
    }
    tun->data[dir].pw_num++;*/
    old_lsp = service_find_lsp_by_index(pw->lsp_id);
    if ((NULL != old_lsp) && (old_lsp->pw_num > 0))
    {
        old_lsp->pw_num--;
    }
    lsp->pw_num++;

    //SET_PW_DATA(pw, dir, lable, exp, tun_id);
    SET_PW_DATA(pw, in_lable, out_lable, exp, lsp_id);

    zlog_info(NSM_ZG, "set pw_%d data: in_lable: %d out_lable: %d exp %d lsp_id: %d.", pw->index, in_lable, out_lable, exp, lsp_id);

    return CLI_SUCCESS;
}

/*
 * 配置pw car参数
 */
CLI(pw_car,
    pw_car_cmd,
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
    struct vport_pw *pw;
    //struct vport_tun *tun;
    struct service_lsp *lsp;

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
        else if (0 == pal_strcmp("pir", argv[i]))
        {
            pir = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("pbs", argv[i]))
        {
            pbs = atoi(argv[i + 1]);
        }
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

    pw = vport_get_current_pw(cli);
    if (NULL == pw)
    {
        cli_out(cli, "%% current pseudo wire does not exist.\n");
        return CLI_ERROR;
    }

    /*tun = vport_find_tun_by_index(pw->data[INGRESS].tun_id);
    if ((NULL == tun) || (!IS_VPORT_VALID(tun, INGRESS)))
    {
        cli_out(cli, "%% target tunnel does not exist.\n");
        return CLI_ERROR;
    }*/
    lsp = service_find_lsp_by_index(pw->lsp_id);
    if ((NULL == lsp) || (!IS_LSP_VALID(lsp)))
    {
        cli_out(cli, "%% target lsp does not exist.\n");
        return CLI_ERROR;
    }

    if (IS_PW_USED(pw))
    {
        struct hal_car_info_s msgdata = {0};
        if (pw->car.is_enable)
        {
            msgdata.sev_index=pw->index;
            msgdata.enable = FALSE;
            //msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
            msgdata.portId = vport_clac_if_index(lsp->in_port_type, lsp->in_port_id);
            msgdata.i_tag_label = pw->data[INGRESS].lable;
            msgdata.o_tag_label = 0;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
        }

        if (is_enable)
        {
            msgdata.sev_index=pw->index;
            msgdata.enable = is_enable;
            //msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
			msgdata.portId = vport_clac_if_index(lsp->in_port_type, lsp->in_port_id);
            msgdata.i_tag_label = pw->data[INGRESS].lable;
            msgdata.o_tag_label = 0;
            msgdata.cir = cir;
            msgdata.cbs = cbs;
            msgdata.pir = pir;
            msgdata.pbs = pbs;
            msgdata.fwdRed = fwd_red;
            msgdata.drpYellow = drp_yellow;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
        }

    }

    SET_CAR_DATA(pw->car, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);

    zlog_info(NSM_ZG, "set pw_%d car: is_enable:%d, cir:%d, cbs:%d, pir:%d, pbs:%d, fwd_red:%d, drp_yellow:%d.",
        pw->index, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);

    return CLI_SUCCESS;
}

void pw_write_single_config(struct vport_pw *pw, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(pw);
    pal_assert(cli);

    cli_out(cli, "pw %d\n", pw->index);

    /*if (IS_PW_BIND_TUN(pw, INGRESS))
    {
        cli_out(cli, " config ingress lable %d tun-id %d\n",
            pw->data[INGRESS].lable,
            pw->data[INGRESS].tun_id);
    }

    if (IS_PW_BIND_TUN(pw, EGRESS))
    {
        cli_out(cli, " config egress lable %d exp %d tun-id %d\n",
            pw->data[EGRESS].lable,
            pw->data[EGRESS].exp,
            pw->data[EGRESS].tun_id);
    }*/
    if (IS_PW_BIND_LSP(pw))
    {
        cli_out(cli, " config ingress lable %d egress lable %d exp %d lsp-id %d\n",
            pw->data[INGRESS].lable,
            pw->data[EGRESS].lable,
            pw->data[EGRESS].exp,
            pw->lsp_id);
    }

    if (pw->car.is_enable)
    {
        cli_out(cli, " car enable cir %d cbs %d pir %d pbs %d\n",
            pw->car.cir, pw->car.cbs, pw->car.pir, pw->car.pbs);
    }

    cli_out(cli, "!\n");

    return;
}

/*
 * 打印pw配置信息
 */
int pw_write_config (struct cli *cli)
{
    vport_foreach_pw(pw_write_single_config, cli);
    return CLI_SUCCESS;
}
void pw_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, PW_MODE);
    cli_install_config(ctree, PW_MODE, pw_write_config);
    cli_install(ctree, CONFIG_MODE, &pw_id_cmd);
    cli_install(ctree, CONFIG_MODE, &no_pw_id_cmd);
    cli_install(ctree, PW_MODE, &pw_config_cmd);
    cli_install(ctree, PW_MODE, &pw_car_cmd);
    return;
}
