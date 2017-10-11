/******************************************************************************
 *
 *  pac/vport/tun_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  tun(tunnel)CLI部分，创建于2012.2.10
 *
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "hsl/hal_car.h"

/*
 * 新增tunnel
 */
CLI(tun_id,
    tun_id_cmd,
    "tun <1-2000>",
    CLI_TUN_MODE_STR,
    "tunnel id, range 1-2000")
{
    struct vport_tun *tun;
    int index = atoi(argv[0]);

    if (!IS_VPORT_INDEX_VALID(index))
    {
         cli_out(cli, "%% the tunnel id is not validate.\n");
         return CLI_ERROR;
    }

    tun = vport_find_tun_by_index(index);
    if ( NULL == tun )
    {
        cli_out (cli, "%% tunnel %s does not exist, create it now.\n", argv[0]);
        tun = vport_new_tun(index);
        if (NULL == tun)
        {
            cli_out(cli, "%% create tunnel fail.\n");
            return CLI_ERROR;
        }
    }

    cli->index = tun;
    cli->mode = TUN_MODE;

    return CLI_SUCCESS;
}

/*
 * 删除tunnel
 */
CLI(no_tun_id,
    no_tun_id_cmd,
    "no tun <1-2000>",
    CLI_NO_STR,
    "delete a tunnel's configuration",
    "tunnel id, range 1-2000")
{
    struct vport_tun *tun;
    int index = atoi(argv[0]);

    tun = vport_find_tun_by_index(index);
    if (NULL == tun)
    {
        cli_out (cli, "%% tunnel %s does not exist\n", argv[0]);
    	return CLI_ERROR;
    }

    if (IS_TUN_BIND_PW(tun))
    {
    	cli_out (cli, "%% the pseudo wire in this tunnel should be deleted first.\n");
        return CLI_ERROR;
    }

	if (IS_VPORT_USED(tun, INGRESS) || IS_VPORT_USED(tun, EGRESS))
    {
    	cli_out (cli, "%% the service instance binded with tunnel should be unbinded first.\n");
        return CLI_ERROR;
    }

    if ((tun->car.is_enable) && IS_VPORT_VALID(tun, INGRESS))
    {
        struct hal_car_info_s msgdata = {0};

        msgdata.sev_index=tun->index;
		msgdata.enable = FALSE;
        msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
        msgdata.i_tag_label = 0;
        msgdata.o_tag_label = tun->data[INGRESS].lable;
        comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
    }

    vport_delete_tun(tun);
    return CLI_SUCCESS;
}

/*
 * 配置tun模式
 */
CLI(tun_config,
    tun_config_cmd,
    "config (ingress (fe <1-24>|ge <1-4>) lable <0-1048575> vlan <0-4095>|egress (fe <1-24>|ge <1-4>) lable <0-1048575> vlan <0-4095> exp <0-7> peer-mac MAC)",
    "config tunnel paras: direction, interface type, interface id, lable, vlan, exp, peer-mac",
    "tun direction",
    "ingress-tun interface type",
    "fe interface id",
    "ingress-tun interface type",
    "ge interface id",
    "ingress-tun lable",
    "lable value",
    "ingress-tun vlan",
    "vlan id, 0 means vlan diable",
    "tun direction",
    "egress-tun interface type",
    "fe interface id",
    "egress-tun interface type",
    "ge interface id",
    "egress-tun lable",
    "lable value",
    "egress-tun vlan",
    "vlan id, 0 means vlan diable",
    "egress-tun exp",
    "exp value",
    "egress-tun peer mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format")
{
    struct vport_tun *tun;

    enum em_direction dir = (0 == pal_strcmp("ingress", argv[0])) ? INGRESS : EGRESS;
    enum em_intf_type intf_type = INTF_NONE;
    UINT16 intf_id = 0;
    UINT32 lable = 0;
    UINT16 vlan = 0;
    UINT8 exp = 0xFF;
    CHAR mac_addr [ETHER_ADDR_LEN] = {0};
    UINT8 i = 0;

    for (i = 1; i < argc - 1; i += 2)
    {
        if (0 == pal_strcmp("fe", argv[i]))
        {
            intf_type = INTF_FE;
            intf_id = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("ge", argv[i]))
        {
            intf_type = INTF_GE;
            intf_id = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("lable", argv[i]))
        {
            lable = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("vlan", argv[i]))
        {
            vlan = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("exp", argv[i]))
        {
            exp = atoi(argv[i + 1]);
        }
        else if (0 == pal_strcmp("peer-mac", argv[i]))
        {
            if (pal_sscanf (argv[10], "%04hx.%04hx.%04hx",
                (unsigned short *)&mac_addr[0],
                (unsigned short *)&mac_addr[2],
                (unsigned short *)&mac_addr[4]) != 3)
            {
                cli_out (cli, "%% unable to translate mac address %s\n", argv[7]);
                return CLI_ERROR;
            }
        }
    }

    if (!IS_DIR_VALID(dir))
    {
        cli_out(cli, "%% input direction error.\n");
        return CLI_ERROR;
    }

    if (!IS_INTF_VALID(intf_type, intf_id))
    {
        cli_out(cli, "%% input inerface info error.\n");
        return CLI_ERROR;
    }

    if (!IS_LABLE_VALID(lable))
    {
        cli_out(cli, "%% input lable error.\n");
        return CLI_ERROR;
    }

    if (!IS_VLAN_VALID(vlan))
    {
        cli_out(cli, "%% input vlan info error.\n");
        return CLI_ERROR;
    }

    if (EGRESS == dir)
    {
        if (!IS_EXP_VALID(exp))
        {
            cli_out(cli, "%% input exp value error.\n");
            return CLI_ERROR;
        }

        // mac address在输入时已经检测
    }

    tun = vport_get_current_tun(cli);
    if (NULL == tun)
    {
        cli_out(cli, "%% current tunnel does not exist.\n");
        return CLI_ERROR;
    }

    if (IS_TUN_BIND_PW(tun))
    {
    	cli_out (cli, "%% the pseudo wire in this tunnel should be deleted first.\n");
        return CLI_ERROR;
    }

    if (IS_VPORT_USED(tun, dir))
    {
    	cli_out(cli, "%% the service instance binded with tunnel should be unbinded first.\n");
        return CLI_ERROR;
    }

    if (tun->car.is_enable)
    {
        struct hal_car_info_s msgdata = {0};

        if (IS_VPORT_VALID(tun, INGRESS))
        {
            msgdata.sev_index=tun->index;
			msgdata.enable = FALSE;
            msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
            msgdata.i_tag_label = 0;
            msgdata.o_tag_label = tun->data[INGRESS].lable;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
        }

        msgdata.sev_index=tun->index;
        msgdata.enable = tun->car.is_enable;
        msgdata.portId = vport_clac_if_index(intf_type, intf_id);
        msgdata.i_tag_label = 0;
        msgdata.o_tag_label = lable;
        msgdata.cir = tun->car.cir;
        msgdata.cbs = tun->car.cbs;
        msgdata.pir = tun->car.pir;
        msgdata.pbs = tun->car.pbs;
        msgdata.fwdRed = tun->car.fwd_red;
        msgdata.drpYellow = tun->car.drp_yellow;
        comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
    }

    SET_TUN_DATA(tun, dir, intf_type, intf_id, lable, vlan, exp, mac_addr);

    zlog_info(NSM_ZG, "set tun_%d[%d] data: intf(type:%d, id:%d) lable: %d vlan %d exp %d mac_addr %04hx.%04hx.%04hx.",
        tun->index, dir, intf_type, intf_id, lable, vlan, exp, *(unsigned short *)&mac_addr[0], *(unsigned short *)&mac_addr[2], *(unsigned short *)&mac_addr[4]);

    return CLI_SUCCESS;
}

/*
 * 配置tun car参数
 */
CLI(tun_car,
    tun_car_cmd,
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
    struct vport_tun *tun;

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

    tun = vport_get_current_tun(cli);
    if (NULL == tun)
    {
        cli_out(cli, "%% current tunnel does not exist.\n");
        return CLI_ERROR;
    }

    if (IS_VPORT_VALID(tun, INGRESS))
    {
        struct hal_car_info_s msgdata = {0};
        if (tun->car.is_enable)
        {
            msgdata.sev_index=tun->index;
			msgdata.enable = FALSE;
            msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
            msgdata.i_tag_label = 0;
            msgdata.o_tag_label = tun->data[INGRESS].lable;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
        }

        if (is_enable)
        {
            msgdata.sev_index=tun->index;
            msgdata.enable = is_enable;
            msgdata.portId = vport_clac_if_index(tun->data[INGRESS].intf_type, tun->data[INGRESS].intf_id);
            msgdata.i_tag_label = 0;
            msgdata.o_tag_label = tun->data[INGRESS].lable;
            msgdata.cir = cir;
            msgdata.cbs = cbs;
            msgdata.pir = pir;
            msgdata.pbs = pbs;
            msgdata.fwdRed = fwd_red;
            msgdata.drpYellow = drp_yellow;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &msgdata, sizeof(msgdata), NULL, NULL);
        }

    }

    SET_CAR_DATA(tun->car, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);

    zlog_info(NSM_ZG, "set tun_%d car: is_enable:%d, cir:%d, cbs:%d, pir:%d, pbs:%d, fwd_red:%d, drp_yellow:%d.",
        tun->index, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);

    return CLI_SUCCESS;
}

void tun_write_single_config(struct vport_tun *tun, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(tun);
    pal_assert(cli);

    cli_out(cli, "tun %d\n", tun->index);

    if (IS_VPORT_VALID(tun, INGRESS))
    {
        cli_out(cli, " config ingress %s %d lable %d vlan %d \n",
            (tun->data[INGRESS].intf_type == INTF_FE) ? "fe" : "ge",
            tun->data[INGRESS].intf_id,
            tun->data[INGRESS].lable,
            tun->data[INGRESS].vlan);
    }

    if (IS_VPORT_VALID(tun, EGRESS))
    {
        cli_out(cli, " config egress %s %d lable %d vlan %d exp %d peer-mac %04hx.%04hx.%04hx\n",
            (tun->data[EGRESS].intf_type == INTF_FE) ? "fe" : "ge",
            tun->data[EGRESS].intf_id,
            tun->data[EGRESS].lable,
            tun->data[EGRESS].vlan,
            tun->data[EGRESS].exp,
            *(unsigned short *)&tun->data[EGRESS].peer_mac[0],
            *(unsigned short *)&tun->data[EGRESS].peer_mac[2],
            *(unsigned short *)&tun->data[EGRESS].peer_mac[4]);
    }

    if (tun->car.is_enable)
    {
        cli_out(cli, " car enable cir %d cbs %d pir %d pbs %d\n",
            tun->car.cir, tun->car.cbs, tun->car.pir, tun->car.pbs);
    }

    cli_out(cli, "!\n");

    return;
}

/*
 * 打印tunnel配置信息
 */
int tun_write_config (struct cli *cli)
{
    vport_foreach_tun(tun_write_single_config, cli);
    return CLI_SUCCESS;
}

void tun_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, TUN_MODE);
    cli_install_config(ctree, TUN_MODE, tun_write_config);
    cli_install(ctree, CONFIG_MODE, &tun_id_cmd);
    cli_install(ctree, CONFIG_MODE, &no_tun_id_cmd);
    cli_install(ctree, TUN_MODE, &tun_config_cmd);
    cli_install(ctree, TUN_MODE, &tun_car_cmd);
    return;
}
