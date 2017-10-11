/******************************************************************************
 *
 *  pac/service/vpn_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  vpn(虚拟交换业务， vsi virtual switch instance)CLI部分，创建于2012.2.14
 *
 */

#include "pal.h"
#include "nsmd.h"
#include "vport.h"
#include "service.h"
#include "hsl/hal_vpls.h"
#include "hsl/hal_car.h"

BOOL vpn_is_vport_ready(struct vpn_vport *data)
{
    pal_assert(data);

    if (data->type == VPORT_AC)
    {
        struct vport_ac* ac = vport_find_ac_by_index(data->id);
        if (NULL == ac)
        {
            return FALSE;
        }

        if ((!IS_VPORT_VALID(ac, INGRESS)) || (!IS_VPORT_VALID(ac, EGRESS)))
        {
            return FALSE;
        }

        if (IS_VPORT_USED(ac, INGRESS) || IS_VPORT_USED(ac, EGRESS))
        {
            return FALSE;
        }

        return TRUE;
    }

    if (data->type == VPORT_PW)
    {
        struct vport_pw* pw = vport_find_pw_by_index(data->id);
        if (NULL == pw)
        {
            return FALSE;
        }

        if ((!IS_PW_BIND_LSP(pw)) || (!IS_PW_BIND_LSP(pw)))
        {
            return FALSE;
        }

        if (IS_PW_USED(pw) || IS_PW_USED(pw))
        {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

void vpnvport_add_vmac(struct vport_vmac *data, void* para1, void* para2)
{
    struct service_vpn *srv = (struct service_vpn *)para1;
    struct vpn_vport *vport = (struct vpn_vport *)para2;

    hsl_bcm_msg_static_mac_cfg msgdata = {0};

    // config sdk
    msgdata.hsl_vpn_id = srv->index;
    pal_mem_cpy(msgdata.mac_addr, data->mac, ETHER_ADDR_LEN);
    msgdata.src_drop = data->enable_src_drop ? 1 : 0;
    msgdata.dst_drop = data->enable_dst_drop ? 1 : 0;

    msgdata.svp_type = (vport->type == VPORT_AC) ? 0 : 1;

    if (vport->type == VPORT_AC)
    {
        struct vport_ac* ac = vport_find_ac_by_index(vport->id);
        if (NULL != ac)
        {
            msgdata.port = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            msgdata.ac_vlan = ac->data[INGRESS].vlan_id;

            comm_send_hal_msg(HAL_MSG_VPN_STATIC_MAC_ADD, &msgdata, sizeof(msgdata), NULL, NULL);
        }
        return;
    }

    if (vport->type == VPORT_PW)
    {
        struct vport_pw* pw = vport_find_pw_by_index(vport->id);
        //struct vport_tun *out_tun;
        struct service_lsp *lsp;
        if (NULL != pw)
        {
            //out_tun = vport_find_tun_by_index(pw->data[EGRESS].tun_id);
            lsp = service_find_lsp_by_index(pw->lsp_id);
            if (NULL != lsp)
            {
                //msgdata.port = vport_clac_if_index(out_tun->data[EGRESS].intf_type, out_tun->data[EGRESS].intf_id);
                msgdata.port = vport_clac_if_index(lsp->out_port_type, lsp->out_port_id);
                msgdata.pw_label = pw->data[INGRESS].lable;

                comm_send_hal_msg(HAL_MSG_VPN_STATIC_MAC_ADD, &msgdata, sizeof(msgdata), NULL, NULL);
            }
        }
        return;
    }

    return;
}

void vpnvport_rmv_vmac(struct vport_vmac *data, void* para1, void* para2)
{
    struct service_vpn *srv = (struct service_vpn *)para1;

    hsl_bcm_msg_static_mac_cfg msgdata = {0};

    // config sdk
    msgdata.hsl_vpn_id = srv->index;
    pal_mem_cpy(msgdata.mac_addr, data->mac, ETHER_ADDR_LEN);
    comm_send_hal_msg(HAL_MSG_VPN_STATIC_MAC_DEL, &msgdata, sizeof(msgdata), NULL, NULL);

    return;
}

void vpnvport_delete_single_vmac_node(struct vport_vmac *data, void* para1, void* para2)
{
    struct vpn_vport *vport = (struct vpn_vport *)para1;
    RMV_VMAC_FROM_VPORT(vport, data);
}

void vpnvport_rmv_all_vmacs(struct vpn_vport *vport, void *para)
{
    struct service_vpn *srv = (struct service_vpn *)para;
    vpnvport_foreach_vmac(vport, vpnvport_rmv_vmac, srv, NULL);
    vpnvport_foreach_vmac(vport, vpnvport_delete_single_vmac_node, vport, NULL);

    return;
}

void vpn_bind_single_vport(struct vpn_vport *data, void* para)
{
    struct service_vpn *srv = (struct service_vpn *)para;
    hsl_bcm_msg_vpn_port_cfg msgdata = {0};

    msgdata.hsl_vpn_id = srv->index;
    msgdata.svp_port_type = (data->type == VPORT_AC) ? 0 : 1;
    msgdata.vpn_port_attr = (data->mode == VPORT_HUB) ? 0 : 1;

    msgdata.out_disable = 0;
    msgdata.in_disable = 0;

    if (data->type == VPORT_AC)
    {
        struct vport_ac* ac = vport_find_ac_by_index(data->id);
        if (NULL != ac)
        {

            BIND_VPORT_TO_SERVICE(ac, INGRESS, SERVICE_VPN, srv->index);
            BIND_VPORT_TO_SERVICE(ac, EGRESS, SERVICE_VPN, srv->index);

            msgdata.pannel_port = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            msgdata.ac_vlan = ac->data[INGRESS].vlan_id;

            comm_send_hal_msg(HAL_MSG_VPN_PORT_ADD, &msgdata, sizeof(msgdata), NULL, NULL);
        }

        if (ac->car.is_enable)
        {
            struct hal_car_info_s car_msgdata = {0};
			car_msgdata.sev_index= ac->index;
            car_msgdata.enable = ac->car.is_enable;
            car_msgdata.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            car_msgdata.i_tag_label= 0;
            car_msgdata.o_tag_label= ac->data[INGRESS].vlan_id;
            car_msgdata.cir = ac->car.cir;
            car_msgdata.cbs = ac->car.cbs;
            car_msgdata.pir = ac->car.pir;
            car_msgdata.pbs = ac->car.pbs;
			//car_msgdata.priority = ac->priority;   //wmx@150106
			//car_msgdata.weight = ac->car.weight;       //wmx@150106
            car_msgdata.fwdRed = ac->car.fwd_red;
            car_msgdata.drpYellow = ac->car.drp_yellow;
            comm_send_hal_msg(HAL_MSG_CAR_VPN_AC, &car_msgdata, sizeof(car_msgdata), NULL, NULL);
        }
		if (ac->priority_enable)
		{
			struct hal_priority_info_s msgdata = {0};
		    msgdata.sev_index=ac->index;
			msgdata.enable = ac->priority_enable;
			msgdata.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
			msgdata.priority = ac->priority;
			msgdata.ivid = 0;//zlh@0122
			msgdata.ovid = ac->data[INGRESS].vlan_id;//zlh@0122, add ovid
			comm_send_hal_msg(HAL_MSG_PRIORITY_VPN_AC, &msgdata, sizeof(msgdata), NULL, NULL);
		}
        return;
    }

    if (data->type == VPORT_PW)
    {
        struct vport_pw* pw = vport_find_pw_by_index(data->id);
        //struct vport_tun *in_tun, *out_tun;
        struct service_lsp *lsp;
        if (NULL != pw)
        {
            //BIND_VPORT_TO_SERVICE(pw, INGRESS, SERVICE_VPN, srv->index);
            //BIND_VPORT_TO_SERVICE(pw, EGRESS, SERVICE_VPN, srv->index);
            BIND_PW_TO_SERVICE(pw, SERVICE_VPN, srv->index);

            //in_tun = vport_find_tun_by_index(pw->data[INGRESS].tun_id);
            //out_tun = vport_find_tun_by_index(pw->data[EGRESS].tun_id);
            lsp = service_find_lsp_by_index(pw->lsp_id);
            //if ((NULL != in_tun) && (NULL != out_tun))
            if (NULL != lsp) 
            {
                //msgdata.pannel_port = vport_clac_if_index(out_tun->data[EGRESS].intf_type, out_tun->data[EGRESS].intf_id);
                
				msgdata.pannel_port = vport_clac_if_index(lsp->out_port_type, lsp->out_port_type);
                msgdata.in_pw_label = pw->data[INGRESS].lable;
                //msgdata.in_tunnel_label = in_tun->data[INGRESS].lable;
                //msgdata.tunnel_in_vlan = in_tun->data[INGRESS].vlan;


                msgdata.out_pw_label = pw->data[EGRESS].lable;
                msgdata.out_pw_exp = pw->data[EGRESS].exp;
                //msgdata.out_tunnel_label = out_tun->data[EGRESS].lable;
                //msgdata.out_tunnel_exp = out_tun->data[EGRESS].exp;
                //pal_mem_cpy(msgdata.tunnel_out_mac, out_tun->data[EGRESS].peer_mac, ETHER_ADDR_LEN);
                //msgdata.tunnel_out_vlan = out_tun->data[EGRESS].vlan;
                msgdata.lsp_id = lsp->id;

                comm_send_hal_msg(HAL_MSG_VPN_PORT_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

                if (pw->car.is_enable)
                {
                    struct hal_car_info_s car_msgdata = {0};
					car_msgdata.sev_index= pw->index;
                    car_msgdata.enable = pw->car.is_enable;
                    //car_msgdata.portId = vport_clac_if_index(in_tun->data[INGRESS].intf_type, in_tun->data[INGRESS].intf_id);
                    car_msgdata.portId = vport_clac_if_index(lsp->in_port_type, lsp->in_port_id);
                    car_msgdata.i_tag_label= pw->data[INGRESS].lable;
                    car_msgdata.o_tag_label= 0;
                    car_msgdata.cir = pw->car.cir;
                    car_msgdata.cbs = pw->car.cbs;
                    car_msgdata.pir = pw->car.pir;
                    car_msgdata.pbs = pw->car.pbs;
					//car_msgdata.priority = pw->priority;   //wmx@150106
			        //car_msgdata.weight = pw->car.weight;       //wmx@150106
                    car_msgdata.fwdRed = pw->car.fwd_red;
                    car_msgdata.drpYellow = pw->car.drp_yellow;
                    comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &car_msgdata, sizeof(car_msgdata), NULL, NULL);
                }
            }
        }

        return;
    }

    return;
}

void vpn_unbind_single_vport(struct vpn_vport *data, void* para)
{
    struct service_vpn *srv = (struct service_vpn *)para;
    hsl_bcm_msg_vpn_port_cfg msgdata = {0};

    msgdata.hsl_vpn_id = srv->index;
    msgdata.svp_port_type = (data->type == VPORT_AC) ? 0 : 1;

    vpnvport_rmv_all_vmacs(data, srv);

    if (data->type == VPORT_AC)
    {
        struct vport_ac* ac = vport_find_ac_by_index(data->id);

        if (NULL != ac)
        {
            UNBIND_VPORT_FROM_SERVICE(ac, INGRESS, SERVICE_VPN, srv->index);
            UNBIND_VPORT_FROM_SERVICE(ac, EGRESS, SERVICE_VPN, srv->index);

            if (ac->car.is_enable)
            {
                struct hal_car_info_s car_msgdata = {0};

				car_msgdata.sev_index= ac->index;
				car_msgdata.enable = FALSE;
                car_msgdata.portId = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
                car_msgdata.i_tag_label= 0;
                car_msgdata.o_tag_label= ac->data[INGRESS].vlan_id;
                comm_send_hal_msg(HAL_MSG_CAR_VPN_AC, &car_msgdata, sizeof(car_msgdata), NULL, NULL);
            }

            msgdata.pannel_port = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            msgdata.ac_vlan = ac->data[INGRESS].vlan_id;

            comm_send_hal_msg(HAL_MSG_VPN_PORT_DEL, &msgdata, sizeof(msgdata), NULL, NULL);
        }

        return;
    }

    if (data->type == VPORT_PW)
    {
        struct vport_pw* pw = vport_find_pw_by_index(data->id);
        //struct vport_tun *in_tun, *out_tun;
        struct service_lsp *lsp;
        if (NULL != pw)
        {
            //UNBIND_VPORT_FROM_SERVICE(pw, INGRESS, SERVICE_VPN, srv->index);
            //UNBIND_VPORT_FROM_SERVICE(pw, EGRESS, SERVICE_VPN, srv->index);
            UNBIND_PW_FROM_SERVICE(pw, SERVICE_VPN, srv->index);

            //in_tun = vport_find_tun_by_index(pw->data[INGRESS].tun_id);
            //out_tun = vport_find_tun_by_index(pw->data[EGRESS].tun_id);
            lsp = service_find_lsp_by_index(pw->lsp_id);
            //if ((NULL != in_tun) && (NULL != out_tun))
            if (NULL != lsp)
            {
                if (pw->car.is_enable)
                {
                    struct hal_car_info_s car_msgdata = {0};
					car_msgdata.sev_index= pw->index;
                    car_msgdata.enable = FALSE;
                    //car_msgdata.portId = vport_clac_if_index(in_tun->data[INGRESS].intf_type, in_tun->data[INGRESS].intf_id);
                    car_msgdata.portId = vport_clac_if_index(lsp->in_port_type, lsp->in_port_id);
                    car_msgdata.i_tag_label= pw->data[INGRESS].lable;
                    car_msgdata.o_tag_label= 0;
                    comm_send_hal_msg(HAL_MSG_CAR_VPN_MPLS, &car_msgdata, sizeof(car_msgdata), NULL, NULL);
                }

                //msgdata.pannel_port = vport_clac_if_index(out_tun->data[EGRESS].intf_type, out_tun->data[EGRESS].intf_id);
				msgdata.pannel_port = vport_clac_if_index(lsp->out_port_type, lsp->out_port_id);
                msgdata.in_pw_label = pw->data[INGRESS].lable;

                comm_send_hal_msg(HAL_MSG_VPN_PORT_DEL, &msgdata, sizeof(msgdata), NULL, NULL);
            }
        }
        return;
    }

    return;
}

void vpn_delete_vport_node(struct vpn_vport *data, void* para)
{
    struct service_vpn *srv = (struct service_vpn *)para;
    RMV_VPORT_FROM_VPN(srv, data);

    return;
}

void vpn_clean_all_vports(struct service_vpn *srv)
{
    vpn_foreach_vport(srv, vpn_unbind_single_vport, srv);
    vpn_foreach_vport(srv, vpn_delete_vport_node, srv);
    return;
}

void mutigrp_bind_single_vport(struct mutigrp_vport *data, void* para1, void* para2)
{
    struct service_vpn *srv = (struct service_vpn *)para1;
    struct muticast_grp *grp = (struct muticast_grp *)para2;

    hsl_bcm_msg_mcast_grp_port msgdata = {0};

     // config sdk
    msgdata.hsl_vpn_id = srv->index;
    msgdata.hsl_mc_group_id = TRANS_MUTIGRP_ID_TO_BCM(grp->index);

    msgdata.svp_port_type = (data->type == VPORT_AC) ? 0 : 1;

    if (data->type == VPORT_AC)
    {
        struct vport_ac* ac = vport_find_ac_by_index(data->id);
        if (NULL != ac)
        {
            msgdata.pannel_port = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            msgdata.ac_vlan = ac->data[INGRESS].vlan_id;

            comm_send_hal_msg(HAL_MSG_MULTICAST_GRP_PORT_ADD, &msgdata, sizeof(msgdata), NULL, NULL);
        }
        return;
    }

    if (data->type == VPORT_PW)
    {
        struct vport_pw* pw = vport_find_pw_by_index(data->id);
        //struct vport_tun *out_tun;
        struct service_lsp *lsp;
        if (NULL != pw)
        {
            //out_tun = vport_find_tun_by_index(pw->data[EGRESS].tun_id);
            lsp = service_find_lsp_by_index(pw->lsp_id);
            if (NULL != lsp)
            {
                //msgdata.pannel_port = vport_clac_if_index(out_tun->data[EGRESS].intf_type, out_tun->data[EGRESS].intf_id);
                msgdata.pannel_port = vport_clac_if_index(lsp->out_port_type, lsp->out_port_id);
                msgdata.pw_label = pw->data[INGRESS].lable;

                comm_send_hal_msg(HAL_MSG_MULTICAST_GRP_PORT_ADD, &msgdata, sizeof(msgdata), NULL, NULL);
            }
        }
        return;
    }

    return;
}


void mutigrp_unbind_single_vport(struct mutigrp_vport *data, void* para1, void* para2)
{
    struct service_vpn *srv = (struct service_vpn *)para1;
    struct muticast_grp *grp = (struct muticast_grp *)para2;

    hsl_bcm_msg_mcast_grp_port msgdata = {0};

    // config sdk
    msgdata.hsl_vpn_id = srv->index;
    msgdata.hsl_mc_group_id = TRANS_MUTIGRP_ID_TO_BCM(grp->index);

    msgdata.svp_port_type = (data->type == VPORT_AC) ? 0 : 1;

    if (data->type  == VPORT_AC)
    {
        struct vport_ac* ac = vport_find_ac_by_index(data->id);
        if (NULL != ac)
        {
            msgdata.pannel_port = vport_clac_if_index(ac->data[INGRESS].intf_type, ac->data[INGRESS].intf_id);
            msgdata.ac_vlan = ac->data[INGRESS].vlan_id;

            comm_send_hal_msg(HAL_MSG_MULTICAST_GRP_PORT_DEL, &msgdata, sizeof(msgdata), NULL, NULL);
        }
        return;
    }

    if (data->type == VPORT_PW)
    {
        struct vport_pw* pw = vport_find_pw_by_index(data->id);
		struct service_lsp *lsp;
        //struct vport_tun *out_tun;
        
        if (NULL != pw)
        {
            //out_tun = vport_find_tun_by_index(pw->data[EGRESS].tun_id);
            lsp = service_find_lsp_by_index(pw->lsp_id);
            if (NULL != lsp)
            {
                //msgdata.pannel_port = vport_clac_if_index(out_tun->data[EGRESS].intf_type, out_tun->data[EGRESS].intf_id);
                msgdata.pannel_port = vport_clac_if_index(lsp->out_port_type, lsp->out_port_id);
                msgdata.pw_label = pw->data[INGRESS].lable;

                comm_send_hal_msg(HAL_MSG_MULTICAST_GRP_PORT_DEL, &msgdata, sizeof(msgdata), NULL, NULL);
            }
        }
        return;
    }
    return;
}

void mutigrp_delete_single_vport_node(struct mutigrp_vport *data, void* para1, void* para2)
{
    struct muticast_grp *grp = (struct muticast_grp *)para1;
    RMV_VPORT_FROM_MUTIGRP(grp, data);
}

void mutigrp_unbind_all_vports(struct muticast_grp *grp, void *para)
{
    struct service_vpn *srv = (struct service_vpn *)para;
    vpnmutigrp_foreach_vport(grp, mutigrp_unbind_single_vport, srv, grp);
    vpnmutigrp_foreach_vport(grp, mutigrp_delete_single_vport_node, grp, NULL);

    return;
}

void vpn_delete_mutigrp_node(struct muticast_grp *grp, void *para)
{
    struct service_vpn *srv = (struct service_vpn *)para;
    RMV_MUTIGRP_FROM_VPN(srv, grp);

    return;
}

void vpn_clean_all_mutigrp(struct service_vpn *srv)
{
    vpn_foreach_mutigrp(srv, mutigrp_unbind_all_vports, srv);
    vpn_foreach_mutigrp(srv, vpn_delete_mutigrp_node, srv);

    return;
}

/*
 * 新增vpn
 */
CLI(vpn_id,
    vpn_id_cmd,
    "vpn <1-2000>",
    CLI_VPN_MODE_STR,
    "vpn id, range 1-2000")
{
    struct service_vpn *srv;
    int index = atoi(argv[0]);

    if (!IS_SERVICE_INDEX_VALID(index))
    {
         cli_out(cli, "%% the vpn id is not validate.\n");
        return CLI_ERROR;
    }

    srv = service_find_vpn_by_index(index);
    if (NULL == srv)
    {
        cli_out (cli, "%% vpn %s does not exist, create it now.\n", argv[0]);

        srv = service_new_vpn(index);
        if (NULL == srv)
        {
            cli_out(cli, "%% create vpn fail.\n");
            return CLI_ERROR;
        }
    }

     cli->index = srv;
     cli->mode = VPN_MODE;

     return CLI_SUCCESS;
}

/*
 * 删除vpn
 */
CLI (no_vpn_id,
     no_vpn_id_cmd,
     "no vpn <1-2000>",
     CLI_NO_STR,
    "delete a vpn's configuration",
    "vpn id, range 1-2000")
{
    struct service_vpn *srv;
    hsl_bcm_msg_vpn_cfg msgdata;

    int index = atoi(argv[0]);

    srv = service_find_vpn_by_index(index);
    if (NULL == srv)
    {
        cli_out (cli, "%% vpn %s does not exist.\n", argv[0]);
        return CLI_ERROR;
    }

    vpn_clean_all_mutigrp(srv);
    vpn_clean_all_vports(srv);

    service_delete_vpn(srv);

    // config sdk
    msgdata.hsl_vpn_id = index;
    comm_send_hal_msg(HAL_MSG_VPN_DEL, &msgdata, sizeof(msgdata), NULL, NULL);

    return CLI_SUCCESS;
}

/*
 * 配置vpn
 */
CLI(vpn_config,
    vpn_config_cmd,
    "config (vpws|vpls (enable-muticast|disable-muticast) (enable-mac-learn|disable-mac-learn)) ",
    "config vpn paras: type, muticast flag, mac learn flag",
    "vpn type",
    "vpn type",
    "muticast flag",
    "muticast flag",
    "mac learn flag",
    "mac learn flag")
{
    enum em_vpn_type type = (0 == pal_strcmp("vpws", argv[0])) ? VPN_VPWS: VPN_VPLS;
    UINT8 muti_flag = ((VPN_VPLS == type) && (0 == pal_strcmp("enable-muticast", argv[1]))) ? TRUE: FALSE;
    UINT8 mac_flag = ((VPN_VPLS == type) && (0 == pal_strcmp("enable-mac-learn", argv[2]))) ? TRUE: FALSE;
    struct service_vpn *srv;

    hsl_bcm_msg_vpn_cfg msgdata = {0};

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    if (srv->vport_list->count > 0)
    {
        cli_out (cli, "%% the vport in this vpn should be deleted first.\n");
        return CLI_ERROR;
    }

    // config sdk
    msgdata.hsl_vpn_id = srv->index;
    msgdata.hsl_vpn_type = srv->type = type;
    msgdata.un_pkt_flg = srv->enable_muticast = muti_flag;
    msgdata.mac_learn_flg = srv->enable_mac_learn= mac_flag;
    comm_send_hal_msg(HAL_MSG_VPN_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

    zlog_info(NSM_ZG, "set vpn %d data: type %s enable_muticast: %d enable_mac_learn: %d.",
        srv->index, argv[0], muti_flag, mac_flag);

    return CLI_SUCCESS;
}

CLI (vpn_add_vport,
    vpn_add_vport_cmd,
    "add-vport (ac|pw) (<1-2000> | <1-2000> (hub-member|spoke-member))",
    "add an attachment circuit or pseudo wire to vpn",
    "vport type, attachment circuit",
    "vport type, pseudo wire",
    "vport id value",
    "vport id value",
    "hub, not valid in vpws",
    "spoke, not valid in vpws")
{
    struct service_vpn *srv;
    struct vpn_vport data;
    enum em_vport_type type = (0 == pal_strcmp("ac", argv[0])) ? VPORT_AC : VPORT_PW;
    UINT32 id = atoi(argv[1]);
    enum em_vport_mode mode = ((3 <= argc) && (0 == pal_strcmp("spoke-member", argv[2]))) ? VPORT_SPOKE : VPORT_HUB;

    if (!IS_VPORT_INDEX_VALID(id))
    {
        cli_out(cli, "%% input id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    if (VPN_BUT == srv->type)
    {
        cli_out(cli, "%% current vpn does not configed.\n");
        return CLI_ERROR;
    }

    data.type = type;
    data.id = id;
    data.mode = (VPN_VPWS == srv->type) ? VPORT_HUB : mode;

    if (!vpn_is_vport_ready(&data))
    {
        cli_out(cli, "%% target vport is not configed or already used.\n");
        return CLI_ERROR;
    }

    if ((VPN_VPWS == srv->type) && (MAX_VPWS_VPORT_NUM <= srv->vport_list->count))
    {
        cli_out(cli, "%% current vpn is full.\n");
        return CLI_ERROR;
    }

    vpn_bind_single_vport(&data, srv);
    ADD_VPORT_TO_VPN(srv, type, id, mode);

    zlog_info(NSM_ZG, "add vport(type: %d, id %d, mode: %d) to vpn_%d.", type, id, mode, srv->index);

    return CLI_SUCCESS;
}

CLI (vpn_rmv_vport,
    vpn_rmv_vport_cmd,
    "rmv-vport (ac|pw) <1-2000>",
    "remove an attachment circuit or pseudo wire from vpn",
    "attachment circuit",
    "pseudo wire",
    "vport id value, range 1-2000")
{
    struct service_vpn *srv;
    struct vpn_vport* data;
    enum em_vport_type type = (0 == pal_strcmp("ac", argv[0])) ? VPORT_AC : VPORT_PW;
    UINT32 id = atoi(argv[1]);

    if (!IS_VPORT_INDEX_VALID(id))
    {
        cli_out(cli, "%% input id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    data = vpn_find_vport_by_index(srv, type, id);
    if (NULL == data)
    {
        cli_out(cli, "%% the vport is not on current vpn.\n");
        return CLI_ERROR;
    }

    if (is_mutigrpvport_in_vpn(srv, type, id))
    {
        cli_out(cli, "%% the vport is in muticast group.\n");
        return CLI_ERROR;
    }

    vpn_unbind_single_vport(data, srv);
    RMV_VPORT_FROM_VPN(srv, data);

    zlog_info(NSM_ZG, "rmv vport(type: %d, id %d, mode: %d) from vpn_%d.", data->type, data->id, data->mode, srv->index);

    return CLI_SUCCESS;
}
CLI (vpn_add_vport_mac,
    vpn_add_vport_mac_cmd,
    "add-vport-mac (ac|pw) <1-2000> mac MAC (enable-src-drop|disable-src-drop) (enable-dst-drop|disable-dst-drop)",
    "add an mac to vport",
    "vport type, attachment circuit",
    "vport type, pseudo wire",
    "vport id value",
    "virtual mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format",
    "drop when source mac address matched",
    "retain when source mac address matched",
    "drop when destination mac address matched",
    "retain when destination mac address matched")
{
    struct service_vpn *srv;
    struct vpn_vport* vport_data;

    enum em_vport_type vport_type = (0 == pal_strcmp("ac", argv[0])) ? VPORT_AC : VPORT_PW;
    UINT32 vport_id = atoi(argv[1]);
    struct vport_vmac vmac;

    if (pal_sscanf (argv[2], "%04hx.%04hx.%04hx",
        (unsigned short *)&vmac.mac[0],
        (unsigned short *)&vmac.mac[2],
        (unsigned short *)&vmac.mac[4]) != 3)
    {
        cli_out (cli, "%% unable to translate mac address %s\n", argv[2]);
        return CLI_ERROR;
    }

    vmac.enable_src_drop = (0 == pal_strcmp("enable-src-drop", argv[3])) ? TRUE : FALSE;
    vmac.enable_dst_drop = (0 == pal_strcmp("enable-dst-drop", argv[4])) ? TRUE : FALSE;

    if (!IS_VPORT_INDEX_VALID(vport_id))
    {
        cli_out(cli, "%% input vport-id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    if (VPN_VPLS != srv->type)
    {
        cli_out(cli, "%% only vpls support vport mac.\n");
        return CLI_ERROR;
    }

    vport_data = vpn_find_vport_by_index(srv, vport_type, vport_id);
    if (NULL == vport_data)
    {
        cli_out(cli, "%% the vport is not on current vpn.\n");
        return CLI_ERROR;
    }

    if (is_vportvmac_in_vpn(srv, vmac.mac))
    {
        vpnvport_rmv_vmac(&vmac, srv, NULL);
        RMV_VMAC_FROM_VPORT(vport_data, &vmac);
    }

    vpnvport_add_vmac(&vmac, srv, vport_data);
    ADD_VMAC_TO_VPORT(vport_data, &vmac.mac, vmac.enable_src_drop, vmac.enable_dst_drop);

    zlog_info(NSM_ZG, "add vmac(mac: %04hx.%04hx.%04hx, src %d, dst %d) to vpn_%d vport(type: %d, id :%d) ",
        *(unsigned short *)&vmac.mac[0], *(unsigned short *)&vmac.mac[2], *(unsigned short *)&vmac.mac[4],
        vmac.enable_src_drop, vmac.enable_dst_drop,
        srv->index, vport_data->type, vport_data->id);

    return CLI_SUCCESS;
}

CLI (vpn_rmv_vport_mac,
    vpn_rmv_vport_mac_cmd,
    "rmv-vport-mac (ac|pw) <1-2000> mac MAC",
    "rmv an mac from vport",
    "vport type, attachment circuit",
    "vport type, pseudo wire",
    "vport id value",
    "virtual mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format")
{
    struct service_vpn *srv;
    struct vpn_vport* vport_data;
    struct vport_vmac* vmac;

    enum em_vport_type vport_type = (0 == pal_strcmp("ac", argv[0])) ? VPORT_AC : VPORT_PW;
    UINT32 vport_id = atoi(argv[1]);
    CHAR mac_addr [ETHER_ADDR_LEN] = {0};

    if (pal_sscanf (argv[2], "%04hx.%04hx.%04hx",
        (unsigned short *)&mac_addr[0],
        (unsigned short *)&mac_addr[2],
        (unsigned short *)&mac_addr[4]) != 3)
    {
        cli_out (cli, "%% unable to translate mac address %s\n", argv[7]);
        return CLI_ERROR;
    }


    if (!IS_VPORT_INDEX_VALID(vport_id))
    {
        cli_out(cli, "%% input vport-id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    vport_data = vpn_find_vport_by_index(srv, vport_type, vport_id);
    if (NULL == vport_data)
    {
        cli_out(cli, "%% the vport is not on current vpn.\n");
        return CLI_ERROR;
    }

    vmac = vpnvport_find_vmac(vport_data, mac_addr);
    if (NULL == vmac)
    {
        cli_out(cli, "%% the vmac is not on the vport.\n");
        return CLI_ERROR;
    }

    vpnvport_rmv_vmac(vmac, srv, NULL);

    RMV_VMAC_FROM_VPORT(vport_data, vmac);

    zlog_info(NSM_ZG, "rmv vmac(mac: %04hx.%04hx.%04hx) from vpn_%d vport(type: %d, id :%d) ",
        *(unsigned short *)&mac_addr[0], *(unsigned short *)&mac_addr[2], *(unsigned short *)&mac_addr[4],
        srv->index, vport_data->type, vport_data->id);

    return CLI_SUCCESS;
}

CLI (vpn_add_mutigrp,
    vpn_add_mutigrp_cmd,
    "add-mutigrp id <1-8> muti-mac MAC",
    "add an muticast group in vpn",
    "muticast group index",
    "id value",
    "muticast mac address",
    "mac (hardware) address, in HHHH.HHHH.HHHH format")
{
    struct service_vpn *srv;
    hsl_bcm_msg_mcast_grp msgdata = {0};

    UINT32 id = atoi(argv[0]);
    CHAR mac_addr [ETHER_ADDR_LEN] = {0};

    if (pal_sscanf (argv[1], "%04hx.%04hx.%04hx",
        (unsigned short *)&mac_addr[0],
        (unsigned short *)&mac_addr[2],
        (unsigned short *)&mac_addr[4]) != 3)
    {
        cli_out (cli, "%% unable to translate mac address %s\n", argv[7]);
        return CLI_ERROR;
    }

    if (!IS_MUTIGRP_INDEX_VALID(id))
    {
        cli_out(cli, "%% input id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    if (VPN_VPLS != srv->type)
    {
        cli_out(cli, "%% only vpls support muticast.\n");
        return CLI_ERROR;
    }

    if (MAX_VPLS_MUTIGRP_NUM <= srv->mutigrp_list->count)
    {
        cli_out(cli, "%% current muticast group is full.\n");
        return CLI_ERROR;
    }

     ADD_MUTIGRP_TO_VPN(srv, id, mac_addr);

     // config sdk
    msgdata.hsl_vpn_id = srv->index;
    msgdata.hsl_mc_group_id = TRANS_MUTIGRP_ID_TO_BCM(id);
    pal_mem_cpy(msgdata.mc_mac, mac_addr, ETHER_ADDR_LEN);

    comm_send_hal_msg(HAL_MSG_MULTICAST_GROUP_ADD, &msgdata, sizeof(msgdata), NULL, NULL);

    zlog_info(NSM_ZG, "add mutigrp(id: %d, mac %04hx.%04hx.%04hx.) to vpn_%d.",
        id, *(unsigned short *)&mac_addr[0], *(unsigned short *)&mac_addr[2], *(unsigned short *)&mac_addr[4], srv->index);

    return CLI_SUCCESS;
}

CLI (vpn_rmv_mutigrp,
    vpn_rmv_mutigrp_cmd,
    "rmv-mutigrp id <1-8>",
    "rmv an muticast group in vpn",
    "muticast group index",
    "id value")
{
    struct service_vpn *srv;
    struct muticast_grp *grp_data;
    hsl_bcm_msg_mcast_grp msgdata = {0};

    UINT32 id = atoi(argv[0]);

    if (!IS_MUTIGRP_INDEX_VALID(id))
    {
        cli_out(cli, "%% input id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    if (VPN_VPLS != srv->type)
    {
        cli_out(cli, "%% only vpls support muticast.\n");
        return CLI_ERROR;
    }

    grp_data = vpn_find_mutigrp_by_index(srv, id);
    if (NULL == grp_data)
    {
        cli_out(cli, "%% the muti-grp is not configed.\n");
        return CLI_ERROR;
    }

    mutigrp_unbind_all_vports(grp_data, srv);

    // config sdk
    msgdata.hsl_vpn_id = srv->index;
    msgdata.hsl_mc_group_id = TRANS_MUTIGRP_ID_TO_BCM(id);
    pal_mem_cpy(msgdata.mc_mac, grp_data->muticast_mac, ETHER_ADDR_LEN);
    comm_send_hal_msg(HAL_MSG_MULTICAST_GROUP_DEL, &msgdata, sizeof(msgdata), NULL, NULL);

    RMV_MUTIGRP_FROM_VPN(srv, grp_data);

    zlog_info(NSM_ZG, "rmv mutigrp(id: %d) from vpn_%d.", id, srv->index);

    return CLI_SUCCESS;
}


CLI (vpn_add_mutigrp_vport,
    vpn_add_mutigrp_vport_cmd,
    "add-muti-vport grp-id <1-8> (ac|pw) <1-2000>",
    "add an muticast port to muticast group in vpn",
    "muticast group index",
    "id value",
    "attachment circuit",
    "pseudo wire",
    "vport id value")
{
    struct service_vpn *srv;
    struct muticast_grp *grp_data;
    struct vpn_vport* vport_data;
    struct mutigrp_vport* vport_idx;
    struct mutigrp_vport data = {0};

    UINT32 grp_id = atoi(argv[0]);
    enum em_vport_type vport_type = (0 == pal_strcmp("ac", argv[1])) ? VPORT_AC : VPORT_PW;
    UINT32 vport_id = atoi(argv[2]);

    if (!IS_MUTIGRP_INDEX_VALID(grp_id))
    {
        cli_out(cli, "%% input grp-id  error.\n");
        return CLI_ERROR;
    }

    if (!IS_VPORT_INDEX_VALID(vport_id))
    {
        cli_out(cli, "%% input vport-id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    grp_data = vpn_find_mutigrp_by_index(srv, grp_id);
    if (NULL == grp_data)
    {
        cli_out(cli, "%% the muti-grp is not configed.\n");
        return CLI_ERROR;
    }

    vport_data = vpn_find_vport_by_index(srv, vport_type, vport_id);
    if (NULL == vport_data)
    {
        cli_out(cli, "%% the vport is not on current vpn.\n");
        return CLI_ERROR;
    }

    vport_idx = vpnmutigrp_find_vport_by_index(grp_data, vport_type, vport_id);
    if (NULL != vport_idx)
    {
     cli_out(cli, "%% the vport is already on current mutigrp.\n");
     return CLI_ERROR;
    }

    data.type = vport_type;
    data.id = vport_id;

    mutigrp_bind_single_vport(&data, srv, grp_data);

    ADD_VPORT_TO_MUTIGRP(grp_data, vport_type, vport_id);

    zlog_info(NSM_ZG, "add vport(type: %d, id %d) to vpn_%d mutigrp_%d.",
        vport_type, vport_id, srv->index, grp_id);

    return CLI_SUCCESS;
}

CLI (vpn_rmv_mutigrp_vport,
    vpn_rmv_mutigrp_vport_cmd,
    "rmv-muti-vport grp-id <1-8> (ac|pw) <1-2000>",
    "rmv an muticast port from muticast group in vpn",
    "muticast group index",
    "id value",
    "attachment circuit",
    "pseudo wire",
    "vport id value")
{
    struct service_vpn *srv;
    struct muticast_grp *grp_data;
    struct vpn_vport* vport_data;
    struct mutigrp_vport* vport_idx;

    UINT32 grp_id = atoi(argv[0]);
    enum em_vport_type vport_type = (0 == pal_strcmp("ac", argv[1])) ? VPORT_AC : VPORT_PW;
    UINT32 vport_id = atoi(argv[2]);

    if (!IS_MUTIGRP_INDEX_VALID(grp_id))
    {
        cli_out(cli, "%% input grp-id  error.\n");
        return CLI_ERROR;
    }

    if (!IS_VPORT_INDEX_VALID(vport_id))
    {
        cli_out(cli, "%% input vport-id error.\n");
        return CLI_ERROR;
    }

    srv = service_get_current_vpn(cli);
    if (NULL == srv)
    {
        cli_out(cli, "%% current vpn does not exist.\n");
        return CLI_ERROR;
    }

    grp_data = vpn_find_mutigrp_by_index(srv, grp_id);
    if (NULL == grp_data)
    {
        cli_out(cli, "%% the muti-grp is not configed.\n");
        return CLI_ERROR;
    }

    vport_data = vpn_find_vport_by_index(srv, vport_type, vport_id);
    if (NULL == vport_data)
    {
        cli_out(cli, "%% the vport is not on current vpn.\n");
        return CLI_ERROR;
    }

    vport_idx = vpnmutigrp_find_vport_by_index(grp_data, vport_type, vport_id);
    if (NULL == vport_idx)
    {
        cli_out(cli, "%% the vport is already on current mutigrp.\n");
        return CLI_ERROR;
    }

    mutigrp_unbind_single_vport(vport_idx, srv, grp_data);
    RMV_VPORT_FROM_MUTIGRP(grp_data, vport_idx);
    vport_idx = NULL;

    zlog_info(NSM_ZG, "add vport(type: %d, id %d) to vpn_%d mutigrp_%d.",
        vport_type, vport_id, srv->index, grp_id);

    return CLI_SUCCESS;
}

void vpn_write_single_vpws_vport_config(struct vpn_vport *data, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(data);
    pal_assert(cli);

    cli_out(cli, " add-vport %s  %d\n",
        (data->type == VPORT_AC) ? "ac" : "pw", data->id);
    return;
}

void vpnvport_write_single_vmac_config(struct vport_vmac *data, void *para1, void* para2)
{
    struct cli *cli = (struct cli *)para1;
    struct vpn_vport *vport = (struct vpn_vport *)para2;

    pal_assert(data);
    pal_assert(cli);
    pal_assert(vport);

    cli_out(cli, "  add-vport-mac %s  %d mac  %04hx.%04hx.%04hx %s %s\n",
        (vport->type == VPORT_AC) ? "ac" : "pw",
        vport->id,
        *(unsigned short *)&data->mac[0],
        *(unsigned short *)&data->mac[2],
        *(unsigned short *)&data->mac[4],
        (data->enable_src_drop) ? "enable-src-drop" : "disable-src-drop",
        (data->enable_dst_drop) ? "enable-dst-drop" : "disable-dst-drop)");

    return;
}

void vpn_write_single_vpls_vport_config(struct vpn_vport *data, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(data);
    pal_assert(cli);

    cli_out(cli, " add-vport %s  %d %s\n",
        (data->type == VPORT_AC) ? "ac" : "pw",
        data->id,
        (data->mode == VPORT_HUB) ? "hub-member" : "spoke-member");

    vpnvport_foreach_vmac(data, vpnvport_write_single_vmac_config, cli, data);

    return;
}

void mutigrp_write_single_vport_config(struct mutigrp_vport *data, void* para1, void* para2)
{
    struct muticast_grp *grp = (struct muticast_grp *)para1;
    struct cli *cli = (struct cli *)para2;

    pal_assert(data);
    pal_assert(cli);

    cli_out(cli, "  add-muti-vport grp-id %d vport-type %s vport-id %d\n",
        grp->index,
        (data->type == VPORT_AC) ? "ac" : "pw",
        data->id);
    return;
}


void vpn_write_single_mutigrp_config(struct muticast_grp *data, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(data);
    pal_assert(cli);

    cli_out(cli, " add-mutigrp id %d muti-mac %04hx.%04hx.%04hx\n",
        data->index,
        *(unsigned short *)&data->muticast_mac[0],
        *(unsigned short *)&data->muticast_mac[2],
        *(unsigned short *)&data->muticast_mac[4]);

    vpnmutigrp_foreach_vport(data, mutigrp_write_single_vport_config, data, cli);

    return;
}


void vpn_write_single_config(struct service_vpn *srv, void *para)
{
    struct cli *cli = (struct cli *)para;

    pal_assert(srv);
    pal_assert(cli);

    cli_out(cli, "vpn %d\n", srv->index);

    if (VPN_VPWS == srv->type)
    {
        cli_out(cli, " config vpws\n");
        vpn_foreach_vport(srv, vpn_write_single_vpws_vport_config, cli);
    }
    else
    {
        cli_out(cli, " config vpls %s %s\n",
            srv->enable_muticast ? "enable-muticast" : "disable-muticast",
            srv->enable_mac_learn ? "enable-mac-learn" : "disable-mac-learn");

        vpn_foreach_vport(srv, vpn_write_single_vpls_vport_config, cli);
        vpn_foreach_mutigrp(srv, vpn_write_single_mutigrp_config, cli);
    }

    cli_out (cli, "!\n");

    return;
}

/*
 * 打印elan配置信息
 */
int vpn_write_config(struct cli *cli)
{
    service_foreach_vpn(vpn_write_single_config, cli);
    return CLI_SUCCESS;
}
void vpn_cli_init(struct cli_tree *ctree)
{
    cli_install_default(ctree, VPN_MODE);
    cli_install_config(ctree, VPN_MODE, vpn_write_config);
    cli_install(ctree, CONFIG_MODE, &vpn_id_cmd);
    cli_install(ctree, CONFIG_MODE, &no_vpn_id_cmd);
    cli_install(ctree, VPN_MODE, &vpn_config_cmd);
    cli_install(ctree, VPN_MODE, &vpn_add_vport_cmd);
    cli_install(ctree, VPN_MODE, &vpn_rmv_vport_cmd);
    cli_install(ctree, VPN_MODE, &vpn_add_vport_mac_cmd);
    cli_install(ctree, VPN_MODE, &vpn_rmv_vport_mac_cmd);
    cli_install(ctree, VPN_MODE, &vpn_add_mutigrp_cmd);
    cli_install(ctree, VPN_MODE, &vpn_rmv_mutigrp_cmd);
    cli_install(ctree, VPN_MODE, &vpn_add_mutigrp_vport_cmd);
    cli_install(ctree, VPN_MODE, &vpn_rmv_mutigrp_vport_cmd);
}
