/******************************************************************************
 *
 *  pac/vport/vport.h
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  2012.2.11 创建
 *  2012.4.27 整合ac pw tun为vport
 *
 */

#ifndef _PAC_VPORT_H_
#define _PAC_VPORT_H_

#include "pac/comm.h"
#include "linklist.h"
#include "memory.h"
#include "cli.h"
#include "hal/hsl/hal_car.h"

/******************************************************************************
 *
 *  枚举/宏定义
 *
 *****************************************************************************/
#define MAX_VPORT_INDEX        2000
#define MAX_PORT_INDEX       29

#define MAX_LABLE        0xFFFFF
#define MAX_EXP        7

#define MAX_FE_INTF_INDEX        24
#define MAX_GE_INTF_INDEX        4

#define MAX_VLAN_NUM        4095
#define INVALID_VLAN        4096

#define MAX_CAR_XIR_VALUE        1000000
#define MAX_CAR_XBS_VALUE        4095

enum em_intf_type
{
    INTF_NONE,
    INTF_FE,
    INTF_GE,
};

enum em_service_type
{
    SERVICE_NONE,
    SERVICE_VPN,    // elan is a spescial vpn
    SERVICE_LSP,
};

/******************************************************************************
 *
 *  结构体定义
 *
 *****************************************************************************/
struct related_service
{
    enum em_service_type type;
    UINT32 index;
};

struct qos_car
{
    BOOL is_enable;
	UINT32 cir;   /*允诺速率*/
	UINT32 cbs ;  /*允诺令牌桶大小*/
	UINT32 pir ;   /*峰值速率*/
	UINT32 pbs ;  /*峰值令牌桶大小*/
	UINT8 fwd_red; /*forward red*/
	UINT8 drp_yellow; /*drop yellow*/
	BOOL weight_is_enable;
};

struct vport_ac
{
    UINT32 index;
    CHAR name[MAX_NAME_LEN];
    CHAR desc[MAX_DESC_LEN];

    struct ac_data
    {
        enum em_intf_type intf_type;
        UINT16 intf_id;
        UINT16 vlan_id;
        struct related_service service;
    }data[DIR_BUTT];

	unsigned char	priority;	   //wmx@141230 增加优先级
	//unsigned char	weight; 	   //wmx@141230 增加权重功能

	UINT16 lsp_id;
	BOOL priority_enable;

    struct qos_car car;
};

struct vport_pw
{
    UINT32 index;
    CHAR name[MAX_NAME_LEN];
    CHAR desc[MAX_DESC_LEN];

    struct pw_data
    {
        UINT32 lable;
        UINT16 exp;
        //UINT16 tun_id;
    }data[DIR_BUTT];

	//unsigned char	priority;      //wmx@141230 增加优先级
	//unsigned char   weight;        //wmx@141230 增加权重功能
	
    UINT16 lsp_id;
	
	struct related_service service;
 
	struct qos_car car;
   
};

struct vport_tun
{
    UINT32 index;
    CHAR name[MAX_NAME_LEN];
    CHAR desc[MAX_DESC_LEN];

    struct tun_data
    {
        enum em_intf_type intf_type;
        UINT16 intf_id;
        UINT32 lable;
        UINT16 vlan;
        UINT8 exp;
        CHAR peer_mac[ETHER_ADDR_LEN];

        UINT16 pw_num;
        struct related_service service;
    }data[DIR_BUTT];

    struct qos_car car;
};

struct vport_port
{
    UINT32 index;
    CHAR name[MAX_NAME_LEN];
    CHAR desc[MAX_DESC_LEN];

    struct port_data
    {
        enum em_intf_type intf_type;
        UINT16 intf_id;
//        struct related_service service;
    }data[DIR_BUTT];

    struct qos_car rate_limit;
    struct qos_car shapping;
	struct qos_car weight;

	/*wmx@150120 add struct for wred/egress-car/weight*/
	struct hal_wrr_queue_info_s queue;
	
};

struct vport_master
{
    struct list *ac;
    struct list *pw;
    struct list *tun;
	struct list *port;
    pal_time_t last_change_time;
};


/*wangjian@150119, move the struct in vport */


struct vport_wrr_car_info_s
{
	int  	sev_index;  //acid / pwid/ tunId
	int		enable;     //endable /disable

	unsigned long int  portId;
	int qid;
	int cir;
	int cbs;

    
};

struct vport_wrr_wred_info_s
{
	int  	sev_index;  //acid / pwid/ tunId
	int		enable;     //endable /disable

	unsigned long int  portId;
	int qid;
	int startpoint; //0-100
	int slope;  //0-90
	int color; //yellow red green
	int time; //(0-255)
};

struct vport_wrr_weight_info_s
{
	int  	sev_index;  //acid / pwid/ tunId
	int		enable;     //endable /disable

	unsigned long int  portId;
	int weight[8];
};

typedef void (*ac_foreach_func)(struct vport_ac *ac, void *para);
typedef void (*pw_foreach_func)(struct vport_pw *pw, void *para);
typedef void (*tun_foreach_func)(struct vport_tun *tun, void *para);
typedef void (*port_foreach_func)(struct vport_port *port, void *para);

#define IS_INTF_VALID(type, id) ((0 < id) && (((INTF_FE == type) && (id <= MAX_FE_INTF_INDEX)) || ((INTF_GE == type) && (id <= MAX_GE_INTF_INDEX))))
#define IS_VLAN_VALID(id) (MAX_VLAN_NUM >= id)
#define IS_LABLE_VALID(lable) (MAX_LABLE >= lable)
#define IS_EXP_VALID(exp) (MAX_EXP >= exp)

#define IS_VPORT_INDEX_VALID(id) ((0 < id) && (id <= MAX_VPORT_INDEX))
#define IS_PORT_INDEX_VALID(id) ((0 < id) && (id <= MAX_PORT_INDEX))
#define IS_VPORT_VALID(vport, dir) (vport->data[dir].intf_type != INTF_NONE) // 只适用于ac tun
#define IS_VPORT_USED(vport, dir) (vport->data[dir].service.type != SERVICE_NONE)
#define IS_PW_USED(vport) (vport->service.type != SERVICE_NONE)

#define IS_LSP_VALID(lsp) ((lsp->out_port_type != INTF_NONE) || (lsp->in_port_type != INTF_NONE))
#define IS_PW_BIND_LSP(pw) (0 != pw->lsp_id)

#define IS_PW_BIND_TUN(pw, dir) (0 != pw->data[dir].tun_id)
#define IS_TUN_BIND_PW(tun) ((0 < tun->data[INGRESS].pw_num) || (0 < tun->data[EGRESS].pw_num))

#define IS_CAR_XIR_VALID(rate) (MAX_CAR_XIR_VALUE >= rate)
#define IS_CAR_XBS_VALID(size) (MAX_CAR_XBS_VALUE >= size)


#define SET_AC_DATA(ac, dir, intf_type, intf_id, vlan_id) \
    do \
    { \
        ac->data[dir].intf_type = intf_type; \
        ac->data[dir].intf_id = intf_id; \
        ac->data[dir].vlan_id = vlan_id; \
    } while (0)

/*#define SET_PW_DATA(pw, dir, lable, exp, tun_id) \
    do \
    { \
        pw->data[dir].lable = lable; \
        pw->data[dir].exp = exp; \
        pw->data[dir].tun_id = tun_id; \
    } while (0)*/

	
#define SET_PW_DATA(pw, in_lable, out_lable, exp, lsp_id) \
		do \
		{ \
		    pw->data[INGRESS].lable = in_lable; \
		    pw->data[EGRESS].lable = out_lable; \
            pw->data[EGRESS].exp = exp; \
			pw->lsp_id = lsp_id; \
		} while (0)

#define SET_TUN_DATA(tun, dir, intf_type, intf_id, lable, vlan, exp, mac_addr) \
    do \
    { \
        tun->data[dir].intf_type = intf_type; \
        tun->data[dir].intf_id = intf_id; \
        tun->data[dir].lable = lable; \
        tun->data[dir].vlan = vlan; \
        tun->data[dir].exp = exp; \
        pal_mem_cpy(tun->data[dir].peer_mac, mac_addr, ETHER_ADDR_LEN); \
    } while (0)

#define SET_PORT_DATA(port, dir, _intf_type, _intf_id) \
    do \
    { \
        port->data[dir].intf_type = _intf_type; \
        port->data[dir].intf_id = _intf_id;  \
     } while (0)

#define BIND_VPORT_TO_SERVICE(vport, dir, service_type, service_id) \
    do \
    { \
        vport->data[dir].service.type = service_type; \
        vport->data[dir].service.index = service_id; \
    } while (0)
    
#define BIND_PW_TO_SERVICE(vport, service_type, service_id) \
    do \
    { \
        vport->service.type = service_type; \
        vport->service.index = service_id; \
    } while (0)

#define UNBIND_VPORT_FROM_SERVICE(vport, dir, service_type, service_id) \
    if ((vport->data[dir].service.type == service_type) && (vport->data[dir].service.index == service_id)) \
        {vport->data[dir].service.type = SERVICE_NONE; vport->data[dir].service.index = 0; }

#define UNBIND_PW_FROM_SERVICE(vport, service_type, service_id) \
    if ((vport->service.type == service_type) && (vport->service.index == service_id)) \
        {vport->service.type = SERVICE_NONE; vport->service.index = 0; }

#define SET_CAR_DATA(car, _enable, _cir, _cbs, _pir, _pbs, _fwd_red, _drp_yellow) \
    do \
    { \
        car.is_enable = _enable; \
        car.cir = _cir; \
        car.cbs = _cbs; \
        car.pir = _pir; \
        car.pbs = _pbs; \
        car.fwd_red = _fwd_red; \
        car.drp_yellow = _drp_yellow; \
     } while (0)

/******************************************************************************
 *
 *  函数接口
 *
 *****************************************************************************/
UINT32 vport_clac_if_index(enum em_intf_type type, UINT16 id);

struct vport_ac *vport_new_ac(UINT32 index);
struct vport_pw *vport_new_pw(UINT32 index);
struct vport_tun *vport_new_tun(UINT32 index);
struct vport_port*vport_new_port(UINT32 index);

void vport_delete_ac(struct vport_ac *ac);
void vport_delete_pw(struct vport_pw *pw);
void vport_delete_tun(struct vport_tun *tun);
void vport_delete_port(struct vport_port*port);

struct vport_ac *vport_find_ac_by_index(int index);
struct vport_pw *vport_find_pw_by_index(int index);
struct vport_tun *vport_find_tun_by_index(int index);
struct vport_port *vport_find_port_by_index(int index);

void vport_foreach_ac(ac_foreach_func func, void *para);
void vport_foreach_pw(pw_foreach_func func, void *para);
void vport_foreach_tun(tun_foreach_func func, void *para);
void vport_foreach_port(port_foreach_func func, void *para);

struct vport_ac *vport_get_current_ac(struct cli *cli);
struct vport_pw *vport_get_current_pw(struct cli *cli);
struct vport_tun *vport_get_current_tun(struct cli *cli);
struct vport_port*vport_get_current_port(struct cli *cli);


#endif
