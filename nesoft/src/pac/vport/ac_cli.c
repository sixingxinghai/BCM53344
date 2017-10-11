/******************************************************************************
 *
 *  pac/vport/ac.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  ac(Attachment Circuit)，创建于2012.2.9
 *
 */
 

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "hsl/hal_car.h"




/*
 * 新增ac
 */
CLI(ac_id,
    ac_id_cmd,
    "ac <1-2000>",
    CLI_AC_MODE_STR,
    "attachment-circuit id, range 1-2000")
{
	struct vport_ac *ac;
	int index = atoi(argv[0]);

    if (!IS_VPORT_INDEX_VALID(index))
    {
        cli_out(cli, "%% the attachment circuit id is not validate.\n");
        return CLI_ERROR;
    }

    ac = vport_find_ac_by_index(index);
    if (NULL == ac)
    {
        cli_out(cli, "%% attachment circuit %s does not exist, create it now.\n", argv[0]);
        ac = vport_new_ac(index);
        if (NULL == ac)
        {
            cli_out(cli, "%% create attachment circuit fail.\n");
            return CLI_ERROR;
        }
    }

    cli->index = ac;
    cli->mode = AC_MODE;

    return CLI_SUCCESS;
}

/*
 * 删除ac
 */
CLI (no_ac_id,
    no_ac_id_cmd,
    "no ac <1-2000>",
    CLI_NO_STR,
    "delete a attachment-circuit's configuration",
    "attachment-circuit id, range 1-2000")
{
    struct vport_ac *ac;
    int index = atoi(argv[0]);

    ac = vport_find_ac_by_index(index);
    if (NULL == ac)
    {
        cli_out (cli, "%% attachment circuit %s does not exist\n", argv[0]);
        return CLI_ERROR;
    }

    if (IS_VPORT_USED(ac, INGRESS) || IS_VPORT_USED(ac, EGRESS))
    {
    	cli_out (cli, "%% the service instance binded with attachment circuit should be unbinded first.\n");
        return CLI_ERROR;
    }
	
	/* wmx@150116,删除car和priority field processor */
	struct hal_car_info_s msgdata_car = {0};
	struct hal_priority_info_s msgdata_priority = {0};
	if (ac->car.is_enable)
	{
		msgdata_car.sev_index=ac->index;
		msgdata_car.enable = FALSE;
		msgdata_car.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
		msgdata_car.i_tag_label = 0;
		msgdata_car.o_tag_label = ac->data[INGRESS].vlan_id;
		comm_send_hal_msg(HAL_MSG_CAR_VPN_AC, &msgdata_car, sizeof(msgdata_car), NULL, NULL);
	}
			
    if (ac->priority_enable)
	{
		msgdata_priority.sev_index=ac->index;
	    msgdata_priority.enable = FALSE;
		msgdata_priority.ivid = 0;    //zlh@0122
		msgdata_priority.ovid = ac->data[INGRESS].vlan_id;  //zlh@0122, add ovid
		msgdata_priority.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
		comm_send_hal_msg(HAL_MSG_PRIORITY_VPN_AC, &msgdata_priority, sizeof(msgdata_priority), NULL, NULL);
	}

    vport_delete_ac(ac);
    return CLI_SUCCESS;
}

/*
 * 配置ac模式
 */
CLI(ac_config,
    ac_config_cmd,
    "config (fe <1-24>|ge <1-4>) (vlan <1-4095>|)",//sdk不支持vlan 0
    "config ac paras: interface type, interface id, vlan",
    "interface type",
    "fe interface id",
    "interface type",
    "ge interface id",
    "config vlan info",
    "vlan id, 0-4095")
{
    struct vport_ac *ac;

    enum em_intf_type intf_type = (0 == pal_strcmp("fe", argv[0])) ? INTF_FE : INTF_GE;
    UINT16 intf_id = atoi(argv[1]);
	UINT16 vlan_id;

	if ( argc > 2 )
    {
        if (0 == pal_strcmp("vlan", argv[2]))
        {
            vlan_id = atoi(argv[3]);
		    if (!IS_VLAN_VALID(vlan_id))
            {
                cli_out(cli, "%% input in vlan info error.\n");
                return CLI_ERROR;
            }
        }
    }
    else 
		vlan_id = 4096;//标识vlan无效

    if (!IS_INTF_VALID(intf_type, intf_id))
    {
        cli_out(cli, "%% input inerface info error.\n");
        return CLI_ERROR;
    }

    ac = vport_get_current_ac(cli);
    if (NULL == ac)
    {
        cli_out(cli, "%% current attachment circuit does not exist.\n");
        return CLI_ERROR;
    }

    if (IS_VPORT_USED(ac, INGRESS) || IS_VPORT_USED(ac, EGRESS))
    {
    	cli_out(cli, "%% the service instance binded with attachment circuit should be unbinded first.\n");
        return CLI_ERROR;
    }

    SET_AC_DATA(ac, INGRESS, intf_type, intf_id, vlan_id);
    SET_AC_DATA(ac, EGRESS, intf_type, intf_id, vlan_id);

    zlog_info(NSM_ZG, "set ac_%d data: intf(type:%d, id:%d) vlan: %d.", ac->index, intf_type, intf_id, vlan_id);

    return CLI_SUCCESS;
}

/*
 * 配置ac car参数
 */


CLI(ac_car,
    ac_car_cmd,
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
    struct vport_ac *ac;

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

    ac = vport_get_current_ac(cli);
    if (NULL == ac)
    {
        cli_out(cli, "%% current attachment circuit does not exist.\n");
        return CLI_ERROR;
    }

    if (IS_VPORT_USED(ac, INGRESS))
    {
        struct hal_car_info_s msgdata = {0};
        if (ac->car.is_enable)
        {
            msgdata.sev_index=ac->index;
			msgdata.enable = FALSE;
            msgdata.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            msgdata.i_tag_label = 0;
            msgdata.o_tag_label = ac->data[INGRESS].vlan_id;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_AC, &msgdata, sizeof(msgdata), NULL, NULL);
        }

        if (is_enable)
        {
			msgdata.sev_index=ac->index;
			msgdata.enable = is_enable;
            msgdata.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            msgdata.i_tag_label = 0;
            msgdata.o_tag_label = ac->data[INGRESS].vlan_id;
            msgdata.cir = cir;
            msgdata.cbs = cbs;
            msgdata.pir = pir;
            msgdata.pbs = pbs;
            msgdata.fwdRed = fwd_red;
            msgdata.drpYellow = drp_yellow;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_AC, &msgdata, sizeof(msgdata), NULL, NULL);
        }

    }

    SET_CAR_DATA(ac->car, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);

    zlog_info(NSM_ZG, "set ac_%d car: is_enable:%d, cir:%d, cbs:%d, pir:%d, pbs:%d, fwd_red:%d, drp_yellow:%d.",
        ac->index, is_enable, cir, cbs, pir, pbs, fwd_red, drp_yellow);

    return CLI_SUCCESS;
}

	
	/* wmx@2015.01.16 增加priority命令 */
CLI(ac_priority,
	ac_priority_cmd,
	"priority (disable|enable <0-7>)",
	"config priority paras",
	"disable car",
	"enable car",
	"priority paras")
		
	{
		struct vport_ac *ac;
		
		BOOL   is_enable = (0 == pal_strcmp("disable", argv[0])) ? FALSE : TRUE;
		u8	   priority  = 0;  
		UINT8 i = 0;
		
		if(0 == pal_strcmp("enable", argv[0]))
	    {
			priority = atoi(argv[1]);
		}
		ac = vport_get_current_ac(cli);
		if (NULL == ac)
		{
			cli_out(cli, "%% current attachment circuit does not exist.\n");
			return CLI_ERROR;
		}
		
		//struct hal_priority_info_s msgdata = {0};	
		if (IS_VPORT_USED(ac, INGRESS))
		{
			struct hal_priority_info_s msgdata = {0};

			/* zlh@0122, last status is enable first must disable,current will valid */
			//if (ac->priority_enable  && (0 == pal_strcmp("disable", argv[0]) ) )
			if (ac->priority_enable )
			{
				msgdata.sev_index=ac->index;
				msgdata.enable = 0;
				msgdata.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
				//msgdata.priority = 0;
				
				msgdata.ivid = 0;//zlh@0122
				msgdata.ovid = ac->data[INGRESS].vlan_id;//zlh@0122, add ovid
				
				comm_send_hal_msg(HAL_MSG_PRIORITY_VPN_AC, &msgdata, sizeof(msgdata), NULL, NULL);
			}
	
			if (0 == pal_strcmp("enable", argv[0]) )
			{
				msgdata.sev_index=ac->index;
				msgdata.enable = 1;
				msgdata.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
				msgdata.priority = priority;
				
				msgdata.ivid = 0;//zlh@0122
				msgdata.ovid = ac->data[INGRESS].vlan_id;//zlh@0122, add ovid
				
			    comm_send_hal_msg(HAL_MSG_PRIORITY_VPN_AC, &msgdata, sizeof(msgdata), NULL, NULL);
				zlog_info(NSM_ZG, "set ac %d priority:  is_enable:%d, priority:%d, msgdata.ovid: %d, ac->data[INGRESS].vlan_id : %d",ac->index, is_enable,msgdata.priority,msgdata.ovid,ac->data[INGRESS].vlan_id);
			}
			zlog_info(NSM_ZG, "set ac_test_%d priority:  is_enable:%d, priority:%d, msgdata.ovid: %d, ac->data[INGRESS].vlan_id : %d",ac->index, is_enable,msgdata.priority,msgdata.ovid,ac->data[INGRESS].vlan_id);
	
		  }
	
		  ac->priority_enable = is_enable;
		  ac->priority = priority;   //wmx@150110
		  
		  zlog_info(NSM_ZG, "set ac_%d priority: is_enable:%d,priority:%d,ac->data[INGRESS].vlan_id : %d",ac->index, is_enable,priority,ac->data[INGRESS].vlan_id);
		  return CLI_SUCCESS;
	}


void ac_write_single_config(struct vport_ac *ac, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(ac);
    pal_assert(cli);

    cli_out(cli, "ac %d\n", ac->index);

    if (IS_VPORT_VALID(ac, INGRESS))
    {
        cli_out(cli, " config %s %d",
            (ac->data[INGRESS].intf_type == INTF_FE) ? "fe" : "ge",
            ac->data[INGRESS].intf_id);
		if (IS_VLAN_VALID(ac->data[INGRESS].vlan_id))
			cli_out (cli, " vlan %d",ac->data[INGRESS].vlan_id);
			
		cli_out (cli, "\n");

		if (ac->priority_enable)
        {
            cli_out(cli, " priority enable %d\n",ac->priority); 
        }

        if (ac->car.is_enable)
        {
            cli_out(cli, " car enable cir %d cbs %d pir %d pbs %d\n",
                ac->car.cir, ac->car.cbs, ac->car.pir, ac->car.pbs);
        }
		
		
    }
	

    cli_out(cli, "!\n");

    return;
}



/*
 * 打印ac配置信息
 */
int ac_write_config(struct cli *cli)
{
    vport_foreach_ac(ac_write_single_config, cli);
    return CLI_SUCCESS;
}

// debug// fpga write/read
#define FPGA_SIZE 4*1024

int drv_fpga_write(unsigned short address, int len, unsigned short *data)
{
	int ret;
	int fd;


	if(len < 1 || len > FPGA_SIZE || data == NULL)
		return -1;

    fd = open("/dev/fpga.0", O_RDWR);
    if(fd < 0)
    {
        zlog_info(NSM_ZG, "can't open device :spidev28672.0.");
        return -1;
	}

    address *= 2;

    if(address + len > FPGA_SIZE)
        len = FPGA_SIZE - address;

    lseek(fd, address, SEEK_SET);
    ret = write(fd, data, sizeof(unsigned short) * len);
	close(fd);

    if (sizeof(unsigned short) * len != ret)
    {
        zlog_info(NSM_ZG, "write fpga fail.");
        return -1;
    }

    return 0;
}

int drv_fpga_read(unsigned short address, int len, unsigned short* value)
{
	int ret;
	int fd;

	if(len < 1 || len > FPGA_SIZE || value == NULL)
		return -1;

	fd = open("/dev/fpga.0", O_RDWR);
    if(fd < 0)
    {
		zlog_info(NSM_ZG, "can't open device :spidev28672.0");
        return -1;
	}

    address *= 2;

    if(address + len > FPGA_SIZE)
        len = FPGA_SIZE - address;

    lseek(fd, address, SEEK_SET);
    ret = read(fd, value, sizeof(unsigned short) * len);
	close(fd);

    if (sizeof(unsigned short) * len != ret)
    {
        zlog_info(NSM_ZG, "read fpga fail.");
        return -1;
    }

    return 0;
}

/*
 * fpga写
 */
CLI(fpga_write,
    fpga_write_cmd,
    "debug fpga-write ADDR VALUE",
    CLI_DEBUG_STR,
    "write value to fpag",
    "fpga address, in HHHH format",
    "fpga value, in HHHH format")
{
	unsigned short addr, value;

    if (pal_sscanf (argv[0], "%4hx", &addr) != 1)
    {
        cli_out (cli, "%% unable to translate ADDR %s\n", argv[0]);
        return CLI_ERROR;
    }

    if (pal_sscanf (argv[1], "%4hx", &value) != 1)
    {
        cli_out (cli, "%% unable to translate VALUE %s\n", argv[1]);
        return CLI_ERROR;
    }

    if (0 != drv_fpga_write(addr, 1, &value))
    {
        cli_out (cli, "%% write fail\n");
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

/*
 * fpga读
 */
CLI (fpga_read,
    fpga_read_cmd,
    "debug fpga-read ADDR",
    CLI_DEBUG_STR,
    "read fpag value",
    "fpga address, in HHHH format")
{
    #define READ_LENGTH 20
	unsigned short addr;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
    unsigned short value[READ_LENGTH];

    if (pal_sscanf (argv[0], "%4hx", &addr) != 1)
    {
        cli_out (cli, "%% unable to translate ADDR %s\n", argv[0]);
        return CLI_ERROR;
    }

    if (0 != drv_fpga_read(addr, READ_LENGTH, value))
    {
        cli_out (cli, "%% read fail\n");
        return CLI_ERROR;
    }


    cli_out (cli, "%% %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
        value[0], value[1], value[2], value[3], value[4],
        value[5], value[6], value[7], value[8], value[9],
        value[10], value[11], value[12], value[13], value[14],
        value[15], value[16], value[17], value[18], value[19]);

    return CLI_ERROR;
}

void ac_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, AC_MODE);
    cli_install_config(ctree, AC_MODE, ac_write_config);
    cli_install(ctree, CONFIG_MODE, &ac_id_cmd);
    cli_install(ctree, CONFIG_MODE, &no_ac_id_cmd);

    cli_install(ctree, AC_MODE, &ac_config_cmd);
    cli_install(ctree, AC_MODE, &ac_car_cmd);
    cli_install(ctree, AC_MODE, &ac_priority_cmd);

    cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &fpga_write_cmd);
    cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0, &fpga_read_cmd);
    cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0, &fpga_write_cmd);
    cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0, &fpga_read_cmd);
    return;
}
