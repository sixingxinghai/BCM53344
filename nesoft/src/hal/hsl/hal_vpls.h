/******************************************************************************
  文 件 名    : vpls.h
  版 本 号    : 初稿
  作    者   	:cai
  生成日期 : 2012年2月
  功能描述 : vpls 模块内部实现的头文件
  函数列表 :
  修改历史 :
  ******************************************************************************/
#ifndef _HAL_VPLS_H_
#define _HAL_VPLS_H_

#ifndef HAL_MSG_BASE_MSG
#define HAL_MSG_BASE_MSG(n)                         (100 + (n))
#endif

/*djg 
 *HAL_MSG_BASE_MSG(1750) is used by ptp in file hal_ptp.h.so it can not be used here.
 */
#define HAL_MSG_VPN_BASE                           HAL_MSG_BASE_MSG(1500)
#define HAL_MSG_VPN_MSG(n)                        (HAL_MSG_VPN_BASE  + (n))
#define HAL_MSG_VPN_ADD	                           HAL_MSG_VPN_MSG(1)
#define HAL_MSG_VPN_DEL	                           HAL_MSG_VPN_MSG(2)
#define HAL_MSG_VPN_PORT_ADD                       HAL_MSG_VPN_MSG(3)
#define HAL_MSG_VPN_PORT_DEL                       HAL_MSG_VPN_MSG(4)
#define HAL_MSG_VPN_STATIC_MAC_ADD                 HAL_MSG_VPN_MSG(5)
#define HAL_MSG_VPN_STATIC_MAC_DEL                 HAL_MSG_VPN_MSG(6)


/*add cyh 2012 06 21*/
#define HAL_MSG_LSP_BASE                            HAL_MSG_BASE_MSG(1550)
#define HAL_MSG_LSP_MSG(n)                         (HAL_MSG_LSP_BASE  + (n))
#define HAL_MSG_LSP_ADD                             HAL_MSG_LSP_MSG(1)
#define HAL_MSG_LSP_DEL                             HAL_MSG_LSP_MSG(2)
#define HAL_MSG_LSP_GET_LINK_STATUS                 HAL_MSG_LSP_MSG(3)
#define HAL_MSG_LSP_PROTECT_GRP_ADD                 HAL_MSG_LSP_MSG(4)
#define HAL_MSG_LSP_PROTECT_GRP_DEL                 HAL_MSG_LSP_MSG(5)
#define HAL_MSG_LSP_PROTECT_GRP_MOD_PARAM           HAL_MSG_LSP_MSG(6)
#define HAL_MSG_LSP_PROTECT_GRP_SWITCH             HAL_MSG_LSP_MSG(7)
#define HAL_MSG_LSP_GET_PROTECT_GRP_INFO           HAL_MSG_LSP_MSG(8)


/*add cyh 2012 06 21*/
#define HAL_MSG_MULTICAST_BASE                      HAL_MSG_BASE_MSG(1600)
#define HAL_MSG_MULTICAST_MSG(n)                    (HAL_MSG_MULTICAST_BASE  + (n))
#define HAL_MSG_MULTICAST_GROUP_ADD                 HAL_MSG_MULTICAST_MSG(1)
#define HAL_MSG_MULTICAST_GROUP_DEL                 HAL_MSG_MULTICAST_MSG(2)
#define HAL_MSG_MULTICAST_GRP_PORT_ADD                 HAL_MSG_MULTICAST_MSG(3)
#define HAL_MSG_MULTICAST_GRP_PORT_DEL                 HAL_MSG_MULTICAST_MSG(4)

/*add cyh 2013 02 21 */
#define HAL_MSG_OAM_BASE                                     HAL_MSG_BASE_MSG(1650)
#define HAL_MSG_OAM_MSG(n)                                 (HAL_MSG_OAM_BASE  + (n))
#define HAL_MSG_OAM_ADD                                      HAL_MSG_OAM_MSG(1)
#define HAL_MSG_OAM_DEL                                       HAL_MSG_OAM_MSG(2)
#define HAL_MSG_OAM_GET_STATUS                        HAL_MSG_OAM_MSG(3)

//#define HAL_MSG_DEV_SRC_MAC_BASE                  HAL_MSG_BASE_MSG(1650)
//#define HAL_MSG_DEV_SRC_MAC_MSG(n)              (HAL_MSG_DEV_SRC_MAC_BASE  + (n))
//#define HAL_MSG_SET_DEV_SRC_MAC                   HAL_MSG_DEV_SRC_MAC_MSG(1)


/*2.vpls业务的创建和删除。*/
/*HAL_MSG_VPN_ADD/HAL_MSG_VPN_DEL */
typedef struct
{
       unsigned int             hsl_vpn_id;          /*平台下发的vpn id */
       unsigned int             hsl_vpn_type;        /*vpls:1; vpws:0;  */
       /*未知单播，组播，广播在转发域广播使能只有在vpls业务有效, 只对vpls业务有效*/
       unsigned int             un_pkt_flg;
       unsigned int             mac_learn_flg ;      /*mac 地址学习使能,只有在vpls业务有效*/
}hsl_bcm_msg_vpn_cfg;
/*注：删除时只需要配置hsl_vpn_id。*/



/*3.从vpn添加或删除端口*/
/*HAL_MSG_VPN_PORT_ADD/HAL_MSG_VPN_PORT_DEL*/
typedef struct
{
    unsigned int   hsl_vpn_id;
    unsigned int svp_port_type; /*端口类型 AC：0； pw：1*/

    unsigned int pannel_port;    /*面板端口号， ac为出入端口， pw为tunnel出端口*/

    unsigned int   vpn_port_attr;   /*hub spoke 属性配置，hub：0； spoke：1*/
    int out_disable;      /*=1关闭出端口*/
    int in_disable;       /*=1关闭端口入*/

    /*ac端口vlan*/
    int ac_vlan;  //int16

    //pw port
    unsigned int  in_pw_label;
    /*unsigned int in_tunnel_label;
    int   tunnel_in_vlan;*/     /*=4096, 表示untag报文*/

    unsigned int  out_pw_label;
    unsigned char  out_pw_exp;
    /*unsigned int out_tunnel_label;
    unsigned char out_tunnel_exp;
    unsigned char   tunnel_out_mac[6];
    int   tunnel_out_vlan;*/    /*=4096， 表示untag报文*/
    unsigned int lsp_id; /*绑定隧道id*/

}hsl_bcm_msg_vpn_port_cfg;
/*注：删除端口时，ac端口需要配置pannel_port + ac_vlan; pw端口只需要配置pannel_port + in_pw_label;*/


/*4.增加、删除隧道*/
/*lsp_type: 0上环，1下环，2过环*/
/*HAL_MSG_LSP_ADD\HAL_MSG_LSP_DEL*/
typedef struct
{
    unsigned int lsp_id;
    unsigned int  lsp_type;
	 
    unsigned int in_port;
    unsigned int in_label;        /*入标签*/
    unsigned short tunnel_in_vlan;     /*=0, 表示untag报文*/

    unsigned int out_port;
    unsigned int out_label;       /*mpls 出标签*/
    unsigned char out_tunnel_exp;
	unsigned short tunnel_out_vlan;    /*=0， 表示untag报文*/
    unsigned char tunnel_out_mac[6];
}hsl_bcm_msg_mpls_tunnel_cfg;
/*注：删除隧道时，只需要下发lsp_id*/
/*注：以前版本，删除隧道时，需要配置in_label + in_port*/

/*5.配置设备mac地址， 端口地址为设备mac+port*/
/*HAL_MSG_SET_DEV_SRC_MAC*/
typedef struct
{
    unsigned char   dev_src_mac[6];
}hsl_bcm_msg_dev_mac;


/*6.组播组添加、删除（暂定每个vpn最多能添加8个组播组)*/
/*HAL_MSG_MULTICAST_GROUP_ADD/HAL_MSG_MULTICAST_GROUP_DEL*/
typedef struct
{
    unsigned int hsl_vpn_id;
    unsigned int hsl_mc_group_id;  /*从0~7编号*/
    unsigned char  mc_mac[6];
}hsl_bcm_msg_mcast_grp;
/*注：删除隧道时，只需要配置hsl_vpn_id + hsl_mc_group_id*/


/*7.添加删除组播端口*/
/*HAL_MSG_MULTICAST_GRP_PORT_ADD\HAL_MSG_MULTICAST_GRP_PORT_DEL*/
typedef struct
{
    unsigned int hsl_vpn_id;
    unsigned int hsl_mc_group_id;

    unsigned int svp_port_type; /*端口类型 AC：0； pw：1*/
    unsigned int pannel_port;    /*面板端口号， ac为出入端口， pw为tunnel出端口*/

    unsigned int ac_vlan;
    unsigned int pw_label;
}hsl_bcm_msg_mcast_grp_port;

/*8.vpls 静态mac地址增加，删除*/
typedef struct
{
    unsigned int hsl_vpn_id;      /*vpn id*/
    unsigned int  port;           /*端口号*/
    unsigned char mac_addr[6];
    unsigned char src_drop;       /*原mac地址为该mac丢弃*/
    unsigned char dst_drop;       /*目的mac地址为该mac丢弃*/
	
    unsigned int svp_type;         /*端口类型 AC：0； pw：1*/
    unsigned int ac_vlan;
    unsigned int pw_label;
}hsl_bcm_msg_static_mac_cfg;
/*删除mac地址， 只需要下发vpn id和mac地址*/

/*9.获取隧道状态接口*/
typedef enum
{    
    IAL_LSP_STATUS_INVALID = 0,
    IAL_LSP_STATUS_DOWN,
    IAL_LSP_STATUS_UP
}ial_lsp_link_status;

typedef struct
{
    unsigned int lsp_id;
    ial_lsp_link_status lsp_ingress_status;  
    ial_lsp_link_status lsp_egress_status;
}ial_mpls_lsp_status;

/*10.增加、删除隧道保护组*/

typedef struct
{
    unsigned int grp_id;   /*保护组id， 0~63*/
    unsigned int  protect_type;  /*保护类型，现在只支持IAL_PROTECT_TYPE_1BY1*/
    unsigned int    sw_back_flag; /*配置IAL_PROTECT_SWITCH_BACK_DIS*/
    unsigned int    back_time;        /*增加，删除保护组不用配置*/
    unsigned int    work_lsp_id;        /*工作隧道id*/
    unsigned int    backup_lsp_id;      /*保护隧道Id*/
}ial_mpls_protect_group_cfg;
//注意：删除只需要下发grp_id.

/*11.保护组切换 */

typedef enum
{
    IAL_PROTECT_SWITCH_CLEAR=0,
    IAL_PROTECT_SWITCH_LOCK,
    IAL_PROTECT_SWITCH_FORCE
}ial_protect_switch_status;

typedef struct
{
    unsigned int grp_id;
    unsigned int switch_lsp_id;
    ial_protect_switch_status  switch_status;
    unsigned int    lock_lsp_id;
}ial_mpls_protect_switch_cfg;

/*12.获取保护组信息 */
typedef struct
{
    unsigned int grp_id;
    ial_protect_type  protect_type;
    ial_protect_switch_status  switch_status;;
    ial_protect_switch_back          sw_back_flag;
    unsigned int    back_time;
    unsigned int    lock_lsp_id;
    unsigned int    current_lsp_id;
    unsigned int    work_lsp_id;
    unsigned int    backup_lsp_id;
}ial_mpls_protect_group_info;
//注：grp_id为下发参数，其他为输出参数

/*13.oam增加，删除*/
typedef struct
{
    unsigned int   oam_id;
    unsigned int   lsp_id;
    unsigned int   ccm_period;
	unsigned int in_port;
	unsigned int out_port;
    char     ma_name[48];
    char     mep_name;
}ial_oam_cfg;

/*14.获取oam状态信息 */
typedef struct
{
    unsigned int oam_id;
    unsigned int  lsp_id;
     
    unsigned int          rdi_error;
    unsigned int          xcon_error;
    unsigned int          ccm_timeout;
    unsigned int         unexpected_mep; 
    //unsigned int    in_port;
    //unsigned int    out_port;
}ial_oam_status;


#endif
