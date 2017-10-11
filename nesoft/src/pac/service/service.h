/******************************************************************************
 *
 *  pac/service/service.h
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  service(业务类型)头文件，创建于2012.2.11
 *
 */

#ifndef _PAC_SERVICE_H_
#define _PAC_SERVICE_H_

#include "comm.h"
#include "linklist.h"
#include "memory.h"
#include "cli.h"
#include "vport.h"   //wmx@150105

/******************************************************************************
 *
 *  枚举/宏定义
 *
 *****************************************************************************/
#define MAX_SERVICE_INDEX        2000
#define MAX_VPWS_VPORT_NUM        2
#define MAX_VPLS_MUTIGRP_NUM        8
#define MAX_LSP_PROTGRP_NUM        63
#define MAX_LSP_PROTGRP_BACKTIME        1000
#define MAX_LSP_OAM_NUM        31
#define MAX_CCM_PERIOD        7
#define MAX_MA_NAME_NUM        47
#define MAX_MEP_NAME_NUM        1


enum em_vport_type
{
    VPORT_NONE,
    VPORT_AC,
    VPORT_PW,
    VPORT_TUN,
};

enum em_vpn_type
{
    VPN_VPWS,
    VPN_VPLS,
    VPN_BUT
};


enum em_vport_mode
{
    VPORT_HUB,
    VPORT_SPOKE
};

typedef enum
{
      IAL_LSP_NONE = 0,
      IAL_LSP_PE_UP,
      IAL_LSP_PE_DOWN,
      IAL_LSP_P_SWITCH
}ial_lsp_type;

typedef enum
{
    IAL_PROTECT_TYPE_NONE = 0,
    IAL_PROTECT_TYPE_1PLUS1,           /*1+1 路径保护*/
    IAL_PROTECT_TYPE_1PLUS1_SNC,  
    IAL_PROTECT_TYPE_1BY1,
    IAL_PROTECT_TYPE_1BY1_SNC
}ial_protect_type;

typedef enum
{
    IAL_PROTECT_SWITCH_BACK_NONE = 0,
    IAL_PROTECT_SWITCH_BACK_DIS,
    IAL_PROTECT_SWITCH_BACK_EN
}ial_protect_switch_back;

/*zlh@1014,redeclaration at hal_vpls.h*/
/*
typedef enum
{
    IAL_PROTECT_SWITCH_NONE = 0,
    IAL_PROTECT_SWITCH_CLEAR,
    IAL_PROTECT_SWITCH_LOCK,
    IAL_PROTECT_SWITCH_FORCE
}ial_protect_switch_status;
*/

/******************************************************************************
 *
 *  结构体定义
 *
 *****************************************************************************/
struct vport_vmac
{
    CHAR mac[ETHER_ADDR_LEN];
    UINT8 enable_src_drop;
    UINT8 enable_dst_drop;
};

struct vpn_vport
{
    enum em_vport_type type;
    UINT32 id;
    enum em_vport_mode mode;
    struct list *vmac_list;
};

struct mutigrp_vport
{
    enum em_vport_type type;
    UINT32 id;
};

struct muticast_grp
{
    UINT32 index;
    CHAR muticast_mac[ETHER_ADDR_LEN];
    struct list *vport_list;
};

struct service_vpn
{
    UINT32 index;
    CHAR name[MAX_NAME_LEN];
    CHAR desc[MAX_DESC_LEN];

    enum em_vpn_type type;
    UINT8 enable_muticast;
    UINT8 enable_mac_learn;

    struct list *vport_list;
    struct list *mutigrp_list; //多播组
};

struct service_lsp
{
    UINT32 id;
    CHAR name[MAX_NAME_LEN];
    CHAR desc[MAX_DESC_LEN];

    ial_lsp_type  lsp_type;//标识上环和过环是否配置
	ial_lsp_type  lsp_type_down;//下环和上环可以同时配置，此处只标识下环是否配置

	UINT32 in_port_type; 
    UINT32 in_port_id;
    UINT32 in_label;        /*入标签*/
    UINT32 tunnel_in_vlan;     /*=0, 表示untag报文*/
	UINT32 lsp_ingress_status;  /* 0:invalid, 1:down, 2:up.*/

    UINT32 out_port_type;
    UINT32 out_port_id;
    UINT32 out_label;       /*mpls 出标签*/
    UINT32 out_tunnel_exp;
	UINT32 tunnel_out_vlan;    /*=0， 表示untag报文*/
    CHAR tunnel_out_mac[6];
	UINT32 lsp_egress_status;

    UINT32 pw_num;
	UINT32 grp_num;//protect group number used this lsp
	UINT32 oam_num;//oam number used this lsp

	/*wmx@150105*/
	//unsigned char	priority;      //wmx@141230 增加优先级
	//unsigned char   weight;        //wmx@141230 增加权重

	struct qos_car car;            //wmx@141230 增加car
	

	CHAR    config_flag;//是否配置了参数,1为已配置,0为未配置
};

struct service_lsp_group
{
    UINT32 grp_id;   /*保护组id， 0~63*/
    ial_protect_type  protect_type;  /*保护类型，现在只支持IAL_PROTECT_TYPE_1BY1*/
    ial_protect_switch_back          sw_back_flag; /*配置IAL_PROTECT_SWITCH_BACK_DIS*/
    UINT32    back_time;        /*增加，删除保护组不用配置*/
    UINT32    work_lsp_id;        /*工作隧道id*/
    UINT32    backup_lsp_id;      /*保护隧道Id*/
	unsigned int switch_lsp_id;
    //ial_protect_switch_status  switch_status;//zlh@1014
    unsigned int    lock_lsp_id;
	CHAR    config_flag;//是否配置了参数,1为已配置,0为未配置
};

struct service_lsp_oam
{
    UINT32   oam_id;
    UINT32   lsp_id;
    UINT32   ccm_period;
	UINT32 in_port_type; 
    UINT32 in_port_id;
	UINT32 out_port_type;
    UINT32 out_port_id;
    CHAR     ma_name[48];
    CHAR     mep_name;
	CHAR    config_flag;//是否配置了参数,1为已配置,0为未配置
};


struct service_master
{
    struct list *vpn;
    struct list *lsp;
	struct list *lsp_group;
	struct list *lsp_oam;
    pal_time_t last_change_time;
};

typedef void (*vpn_foreach_func) (struct service_vpn *srv, void *para);
typedef void (*vpndata_foreach_func) (struct vpn_vport *data, void *para);
typedef void (*vpnmutigrp_foreach_func) (struct muticast_grp *data, void *para);
typedef void (*vpnmutigrpdata_foreach_func) (struct mutigrp_vport *data, void *para1, void *para2);
typedef void (*vpnvportdata_foreach_func) (struct vport_vmac *data, void *para1, void *para2);
typedef void (*lsp_foreach_func) (struct service_lsp *srv, void *para);
typedef void (*lsp_grp_foreach_func) (struct service_lsp_group *srv, void *para);
typedef void (*lsp_oam_foreach_func) (struct service_lsp_oam *srv, void *para);


#define IS_SERVICE_INDEX_VALID(id) ((0 < id) && (id <= MAX_SERVICE_INDEX))
#define IS_MUTIGRP_INDEX_VALID(id) ((0 < id) && (id <= MAX_VPLS_MUTIGRP_NUM))
#define TRANS_MUTIGRP_ID_TO_BCM(id) (id - 1) // 输入是1 - 8, bcm是 0 - 7
#define IS_LSP_PROTGRP_INDEX_VALID(id) ((0 <= id) && (id <= MAX_LSP_PROTGRP_NUM))
#define IS_LSP_PROTGRP_BACKTIME_VALID(id) ((0 < id) && (id <= MAX_LSP_PROTGRP_BACKTIME))
#define IS_LSP_OAM_INDEX_VALID(id) ((0 <= id) && (id <= MAX_LSP_OAM_NUM))
#define IS_CCM_PERIOD_VALID(id)    ((0 <= id) && (id <= MAX_CCM_PERIOD))



#define ADD_VPORT_TO_VPN(vpn, _type, _id, _mode) \
    do \
    { \
        struct vpn_vport *new_data; \
        new_data = XMALLOC(MTYPE_NSM_SI, sizeof(struct vpn_vport)); \
        new_data->type = _type;\
        new_data->id = _id;\
        new_data->mode = _mode;\
        new_data->vmac_list = list_new();\
        listnode_add(vpn->vport_list, new_data); \
    } while (0)

#define RMV_VPORT_FROM_VPN(vpn, _data) \
    do \
    { \
        struct vpn_vport *tmp_data; \
        struct listnode *node; \
        LIST_LOOP(vpn->vport_list, tmp_data, node) \
        { \
            if ((tmp_data->type == (_data)->type) && (tmp_data->id == (_data)->id)) \
            { \
                list_free(tmp_data->vmac_list); \
                listnode_delete(vpn->vport_list, tmp_data); \
                XFREE (MTYPE_NSM_SI, tmp_data); \
                break; \
            } \
        } \
    } while (0)

#define ADD_VMAC_TO_VPORT(vport, _mac, _src, _dst) \
    do \
    { \
        struct vport_vmac *new_data; \
        new_data = XMALLOC(MTYPE_NSM_SI, sizeof(struct vport_vmac)); \
        pal_mem_cpy(new_data->mac, _mac, ETHER_ADDR_LEN); \
        new_data->enable_src_drop = _src;\
        new_data->enable_dst_drop = _dst;\
        listnode_add(vport->vmac_list, new_data); \
    } while (0)

#define RMV_VMAC_FROM_VPORT(vport, _data) \
    do \
    { \
        struct vport_vmac *tmp_data; \
        struct listnode *node; \
        LIST_LOOP(vport->vmac_list, tmp_data, node) \
        { \
            if (0 == pal_mem_cmp(tmp_data->mac, (_data)->mac, ETHER_ADDR_LEN)) \
            { \
                listnode_delete(vport->vmac_list, tmp_data); \
                XFREE (MTYPE_NSM_SI, tmp_data); \
                break; \
            } \
        } \
    } while (0)

#define ADD_MUTIGRP_TO_VPN(vpn, id, mac_addr) \
    do \
    { \
        struct muticast_grp *new_data; \
        new_data = XMALLOC(MTYPE_NSM_SI, sizeof(struct muticast_grp)); \
        new_data->index = id;\
        pal_mem_cpy(new_data->muticast_mac, mac_addr, ETHER_ADDR_LEN); \
        new_data->vport_list = list_new();\
        listnode_add(vpn->mutigrp_list, new_data); \
    } while (0)

#define RMV_MUTIGRP_FROM_VPN(vpn, _data) \
    do \
    { \
        struct muticast_grp *tmp_data; \
        struct listnode *node; \
        LIST_LOOP(vpn->mutigrp_list, tmp_data, node) \
        { \
            if (tmp_data->index == (_data)->index) \
            { \
                list_free(tmp_data->vport_list); \
                listnode_delete(vpn->mutigrp_list, tmp_data); \
                XFREE (MTYPE_NSM_SI, tmp_data); \
                break; \
            } \
        } \
    } while (0)

#define ADD_VPORT_TO_MUTIGRP(grp, _type, _id) \
    do \
    { \
        struct mutigrp_vport *new_data; \
        new_data = XMALLOC(MTYPE_NSM_SI, sizeof(struct mutigrp_vport)); \
        new_data->type = _type;\
        new_data->id = _id;\
        listnode_add(grp->vport_list, new_data); \
    } while (0)

#define RMV_VPORT_FROM_MUTIGRP(grp, _data) \
    do \
    { \
        struct mutigrp_vport *tmp_data; \
        struct listnode *node; \
        LIST_LOOP(grp->vport_list, tmp_data, node) \
        { \
            if ((tmp_data->type == (_data)->type) && (tmp_data->id == (_data)->id)) \
            { \
                listnode_delete(grp->vport_list, tmp_data); \
                XFREE (MTYPE_NSM_SI, tmp_data); \
                break; \
            } \
        } \
    } while (0)

#define SET_LSP_DATA(lsp, in_tun_id, out_tun_id) \
    do \
    { \
        lsp->tun[INGRESS].id = in_tun_id; \
        lsp->tun[EGRESS].id = out_tun_id; \
    } while (0)

/******************************************************************************
 *
 *  函数接口
 *
 *****************************************************************************/
struct service_vpn *service_new_vpn(UINT32 index);
struct service_lsp *service_new_lsp(UINT32 id);
struct service_lsp_group *service_new_lsp_group(UINT32 id);
struct service_lsp_oam *service_new_lsp_oam(UINT32 id);


void service_delete_vpn(struct service_vpn *srv);
void service_delete_lsp(struct service_lsp *srv);
void service_delete_lspgrp(struct service_lsp_group *srv);
void service_delete_lspoam(struct service_lsp_oam *srv);


struct service_vpn *service_find_vpn_by_index(UINT32 index);
struct vpn_vport *vpn_find_vport_by_index(struct service_vpn *srv, enum em_vport_type type, UINT32 index);
struct muticast_grp *vpn_find_mutigrp_by_index(struct service_vpn *srv, UINT32 index);
struct mutigrp_vport *vpnmutigrp_find_vport_by_index(struct muticast_grp *grp, enum em_vport_type type, UINT32 index);
struct vport_vmac *vpnvport_find_vmac(struct vpn_vport *vport, CHAR mac[]);
BOOL is_mutigrpvport_in_vpn(struct service_vpn *srv, enum em_vport_type type, UINT32 index);
BOOL is_vportvmac_in_vpn(struct service_vpn *srv, CHAR mac[]);
struct service_lsp *service_find_lsp_by_index(UINT32 index);
struct service_lsp_group *service_find_lspgrp_by_index(UINT32 index);
struct service_lsp_oam *service_find_lspoam_by_index(UINT32 index);


void service_foreach_vpn(vpn_foreach_func func, void *para);
void vpn_foreach_vport(struct service_vpn *srv, vpndata_foreach_func func, void *para);
void vpn_foreach_mutigrp(struct service_vpn* srv, vpnmutigrp_foreach_func func, void *para);
void vpnmutigrp_foreach_vport(struct muticast_grp* grp, vpnmutigrpdata_foreach_func func, void *para1, void *para2);
void vpnvport_foreach_vmac(struct vpn_vport* vport, vpnvportdata_foreach_func func, void *para1, void *para2);
void service_foreach_lsp(lsp_foreach_func func, void *para);
void service_foreach_lsp_grp(lsp_grp_foreach_func func, void *para);
void service_foreach_lsp_oam(lsp_oam_foreach_func func, void *para);



struct service_vpn *service_get_current_vpn(struct cli *cli);
struct service_lsp *service_get_current_lsp(struct cli *cli);
struct service_lsp_group *service_get_current_lsp_grp(struct cli *cli);
struct service_lsp_oam *service_get_current_lsp_oam(struct cli *cli);



#endif
