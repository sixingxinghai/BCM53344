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

#define HAL_MSG_VPLS_BASE                           (100+1600)
#define HAL_MSG_VPLS_MSG(n)                         (HAL_MSG_VPLS_BASE  + (n))
#define HAL_MSG_VPLS_VPN_ADD	                    HAL_MSG_VPLS_MSG(1)
#define HAL_MSG_VPLS_VPN_DEL	                    HAL_MSG_VPLS_MSG(2)
#define HAL_MSG_VPLS_PORT_ADD                    	HAL_MSG_VPLS_MSG(3)
#define HAL_MSG_VPLS_PORT_DEL                       HAL_MSG_VPLS_MSG(4)

#define HSL_VPN_PORT_AC 0
#define HSL_VPN_PORT_PW 1

#define HSL_VPORT_DIR_INGRESS 0
#define HSL_VPORT_DIR_EGRESS 1


enum hal_vport_role{
	HUB,
	SPOKE
};


struct hal_msg_vpls_port_s
{
	signed int vpnIndex;

	signed int vportId;
	signed int portType;
	enum hal_vport_role portRole;

	struct vport_infor
	{
		signed int port;

		//ac
		signed int tag;

		//pw
		unsigned int tunnel;
		unsigned int pw;
	}vport[2];

};



/////for l2elin temp///////////////////////////////////////////////////////

#define HAL_MSG_L2ELINE_BASE                           (100+1650)
#define HAL_MSG_L2ELINE_MSG(n)                         (HAL_MSG_L2ELINE_BASE  + (n))
#define HAL_MSG_L2ELINE_ADD	                    HAL_MSG_L2ELINE_MSG(1)
#define HAL_MSG_L2ELINE_DEL	                    HAL_MSG_L2ELINE_MSG(2)

struct hal_msg_l2eline_s
{
	signed int Index;

	signed int inPort;
	signed int inTag;

	signed int outPort;
	signed int outTag;
};

/*2.vpls业务的创建和删除。*/
/*HAL_MSG_VPN_ADD/HAL_MSG_VPN_DEL */                 
typedef struct {       
     unsigned int             hsl_vpn_id;          /*平台下发的vpn id */       
     unsigned int             hsl_vpn_type;        /*vpls:1; vpws:0;  */         
     /*未知单播，组播，广播在转发域广播使能只有在vpls业务有效, 只对vpls业务有效*/       
    unsigned int             un_pkt_flg;                  
    unsigned int             mac_learn_flg ;      /*mac 地址学习使能,只有在vpls业务有效*/   
}hsl_bcm_msg_vpn_cfg;
/*注：删除时只需要配置hsl_vpn_id。*/

/*3.从vpn添加或删除端口*/
/*HAL_MSG_VPN_PORT_ADD/HAL_MSG_VPN_PORT_DEL*/

/* add from 0213 use tunnel add */
typedef struct {

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
    unsigned int  out_pw_label;
    unsigned char  out_pw_exp;
	unsigned int lsp_id;
}hsl_bcm_msg_vpn_port_cfg;

typedef struct {

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
	
    unsigned int in_tunnel_label;
    int   tunnel_in_vlan;     /*=0, 表示untag报文*/

    unsigned int  out_pw_label;
    unsigned char  out_pw_exp;
    unsigned int out_tunnel_label;
    unsigned char out_tunnel_exp;
    unsigned char   tunnel_out_mac[6];
    int   tunnel_out_vlan;    /*=0， 表示untag报文*/

}hsl_bcm_msg_vpn_port_cfg_1307;

/*注：删除端口时，ac端口需要配置pannel_port + ac_vlan; pw端口只需要配置pannel_port + in_pw_label;*/


/*4.增加、删除过环隧道*/
/*HAL_MSG_LSP_ADD\HAL_MSG_LSP_DEL*/
typedef enum
{
      IAL_LSP_NONE = 0,
      IAL_LSP_PE_UP,
      IAL_LSP_PE_DOWN,
      IAL_LSP_P_SWITCH
}ial_lsp_type;



typedef struct
{
    unsigned int tunnel_id;//add from hsl-0213
    ial_lsp_type  lsp_type;//add from hsl-0213
    unsigned int in_port;
    unsigned int in_label;        /*入标签*/
    unsigned int tunnel_in_vlan;     /*=0, 表示untag报文*/

    unsigned int out_port;
    unsigned int out_lable;       /*mpls 出标签*/
    unsigned char out_tunnel_exp;
    unsigned char tunnel_out_mac[6];
    unsigned int tunnel_out_vlan;    /*=0， 表示untag报文*/
}hsl_bcm_msg_mpls_tunnel_cfg;
/*注：删除隧道时，只需要配置in_label + in_port*/

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


#endif

