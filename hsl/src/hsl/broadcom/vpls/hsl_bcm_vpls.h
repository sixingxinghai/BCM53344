
#ifndef _HSL_VPLS_H_
#define _HSL_VPLS_H_

#include "hsl_rsrc.h"
#include "hsl_linklist.h"
/* add from hsl-0213  @zlh0905 */
#include "unistd.h"
#include "hal_vpls.h"


#define BCM_UNIT   (0)
//#define BCM_MAX_PORT_NUM 40//zlh@141213
#define BCM_MAX_PORT_NUM 29//zlh@141224 max port 29


#define HSL_VPN_SVP_CLASS_ID   (100)

/* add from hsl-0213  @zlh0905*/
#define   HSL_OAM_MAX     (32)
#define HSL_BCM_VPLS_MAX   (100)
#define HSL_BCM_MAX_PORT  (29)
/* add from hsl-0213  @zlh0905*/
#define HSL_VLAN_INVALID   (4096)

#define HSL_L2VPN_NUM                           (2048)
#define HSL_L2VPN_MAX                               (2048 + HSL_OAM_MAX)
#define HSL_MPLS_PROTECT_GROUP_MAX   (64)
#define  HSL_MPLS_LSP_MAX     (2 * 1024)


#define HSL_VPLS_DEBUG_TYPE_SV_PORT     (0)
#define HSL_VPLS_DEBUG_TYPE_MPLS            (1)
#define HSL_VPLS_DEBUG_TYPE_OAM             (2)
#define HSL_VPLS_DEBUG_TYPE_PROTECT     (3)
#define HSL_VPLS_DEBUG_TYPE_QOS            (4)


#define HSL_CHECK_L2VPN_ID(rc, id)  \
{   \
      if(id >= HSL_L2VPN_MAX)   \
     {                                       \
           printk("l2 vpn id check: id = %d, max id = %d\n\r", id, HSL_L2VPN_MAX - 1);    \
            return DRV_E_L2VPN_ID_INVALID; \
      }                                      \
}

/* add from hsl-0213 @zlh0923 */
/*核对指针是否为空*/
#define HSL_CHECK_PTR(rc, ptr)   \
{   \
      if(ptr == NULL)   \
      {                         \
             printk("PTR CHECK NULL: %s, %d", __FILE__, __LINE__);     \
             return  0x6503; \
      }                        \
}     


#define HSL_CHECK_LSP_PROTECT_GRP_ID(rc, id)  \
{   \
      if(id >= HSL_MPLS_PROTECT_GROUP_MAX)   \
     {                                       \
           printk("lsp protect grp id check: id = %d, max id = %d\n\r", id, HSL_MPLS_PROTECT_GROUP_MAX - 1);    \
            return DRV_E_LSP_PROTECT_GRP_ID_INVALID; \
      }                                      \
}    

#define HSL_CHECK_LSP_ID(rc, id)  \
{   \
      if(id >= HSL_MPLS_LSP_MAX)   \
     {                                       \
           printk("lsp id check: id = %d, max id = %d\n\r", id, HSL_MPLS_LSP_MAX - 1);    \
            return DRV_E_LSP_ID_INVALID; \
      }                                      \
} 

#define HSL_VPLS_DEBUG_ENTRY_ON(cnt, id)  \
{   \
    cnt++;  \
    \
};


#define HSL_VPLS_DEBUG_ENTRY_OFF(cnt, id)  \
{   \
    cnt--; \
      \
};


typedef enum
{
      HSL_CFG_NO,
      HSL_CFG_YES,
}hsl_cfg;
/* add from hsl-0213 @zlh0923 */

typedef struct _bcm_tr_ing_nh_info_s {
    bcm_port_t      port;
    int      module;
    bcm_trunk_t      trunk;
    uint32   mtu;
} _bcm_tr_ing_nh_info_t;

typedef struct
{
    uint8 mac_addr[6];
}hsl_mac_addr;


struct hal_msg_vpls_vpn_s
{
     signed int vpnIndex;
     signed int vpnValue;
};

struct hsl_bcm_vpls_port_list
{
int index;
int type;
bcm_port_t			port;
bcm_vlan_t 			vlan;

bcm_l3_intf_t 		l3_if_infor;
bcm_if_t 		    obj_id;

bcm_gport_t mpls_port_id;
bcm_mpls_port_t   mpls_port;

struct hsl_bcm_vpls_port_list *next;
};


struct hsl_vpls_vpn_info_s
{
	int32 vpnIndex;
	bcm_vpn_t vpnId;

	bcm_multicast_t bcast_group;

	struct hsl_bcm_vpls_port_list *vpls_port_list[2];
};

typedef enum {
	HSL_BCM_VPN_VPWS,
	HSL_BCM_VPN_VPLS
}hsl_bcm_vpn_type;

typedef struct
{
    int hsl_vpn_id;
    int hsl_mc_group_id;
    bcm_mac_t  mc_mac;
    bcm_multicast_t bcm_mcast_id;      
}hsl_bcm_vpn_mcast_grp_cfg;


typedef struct 
{
    int hsl_vpn_id;
    int hsl_mc_group_id;

    unsigned int svp_type; /*端口类型 AC：0； pw：1*/

    int ac_port;
    int ac_vlan;

    int pw_in_port;
    int match_pw_label;
}hsl_bcm_vpn_mcast_port_cfg;


typedef struct hsl_bcm_vpn_info_tag
{
       //uint32 ing_pri_phb_id;// @zlh0908
	   //uint32 egr_pri_phb_id;// @zlh0908
       unsigned int             hsl_vpn_id;               /*平台下发的vpn id */
       hsl_cfg                      hsl_vpn_cfg;///* add from hsl-0213 @zlh0923 */
       bcm_vpn_t               bcm_vpn_id;             /*bcm 创建vpn返回的id*/
       hsl_bcm_vpn_type   hsl_vpn_type;                  /*vpls or vpws*/
       bcm_multicast_t        bcast_group_id;
              /*未知单播，组播，广播在转发域广播使能只有在vpls业务有效*/
       unsigned int             un_pkt_flg;           
       unsigned int             mac_learn_flg ;      /*mac 地址学习使能,只有在vpls业务有效*/   

       unsigned int            mpls_port_cnt;
       hsl_bcm_vpn_mcast_grp_cfg stVpnMcastGrp[8];

       T_LINK_CONTAINER  *hsl_ac_port_list;
       T_LINK_CONTAINER  *hsl_pw_port_list;
}hsl_bcm_vpn_info;


typedef enum {
	HSL_VPN_PORT_ATTR_HUB,
        HSL_VPN_PORT_ATTR_SPOKE
}hsl_vpn_port_attr_e;


typedef struct 
{
    bcm_vpn_t  hsl_vpn_id;
    bcm_port_t ac_port;
    int ac_vlan;
    hsl_vpn_port_attr_e   vpn_port_attr;
    bcm_gport_t  mpls_port_id;
    int out_disable;
    int in_disable;
	int ing_pri_phb_id;//zlh@141224
	int egr_pri_phb_id;//zlh@141224
	
}hsl_bcm_ac_port_info;

/*  add from hsl-0213 use pw_port_add */
typedef struct 
{
    unsigned int hsl_vpn_id;
    bcm_mpls_label_t match_pw_label;
  
    bcm_port_t in_port;
    bcm_port_t out_port;
    
    bcm_mpls_label_t out_pw_label;
    int8 out_pw_exp;
    hsl_vpn_port_attr_e   vpn_port_attr;
    int out_disable;
    int in_disable;
    bcm_gport_t  mpls_port_id;
    int egr_l3_next_hop_index;
    int  lsp_id;
}hsl_bcm_pw_port_info;

typedef struct 
{
    unsigned int hsl_vpn_id;
    
	//mpls port
    bcm_mpls_label_t match_pw_label;
    bcm_mpls_label_t match_lsp_label;
	
    bcm_port_t in_port;//@zlh1011
    bcm_port_t out_port;
    bcm_mpls_label_t out_lsp_label;
    int8 out_lsp_exp;
    bcm_mpls_label_t out_pw_label;
    int8 out_pw_exp;
    bcm_mac_t	 des_mac;
    int mpls_out_vlan;
    int mpls_in_vlan;
    hsl_vpn_port_attr_e   vpn_port_attr;
    int out_disable;
    int in_disable;
    bcm_gport_t  mpls_port_id;
    int egr_l3_next_hop_index;//zlh@1011
	int  lsp_id;//zlh@0923
	int ing_pri_phb_id;//zlh@141213
	int egr_pri_phb_id;//zlh@141213
	int pw_qos_mode;//zlh@141213
}hsl_bcm_pw_port_info_1307;




/******mpls tunnel *****/
typedef enum
{    
    IAL_LSP_STATUS_INVALID = 0,
    IAL_LSP_STATUS_DOWN,
    IAL_LSP_STATUS_UP,
}ial_lsp_link_status;

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
    IAL_PROTECT_STATUS_WORK = 0,           /*保护组在工作状态*/
    IAL_PROTECT_STATUS_BACKUP,              /*保护组在保护状态*/
}ial_protect_status;

typedef enum
{
    IAL_PROTECT_SWITCH_NONE = 0,
    IAL_PROTECT_SWITCH_CLEAR,
    IAL_PROTECT_SWITCH_LOCK,
    IAL_PROTECT_SWITCH_FORCE,
}ial_protect_switch_status;


typedef enum
{
    IAL_PROTECT_SWITCH_BACK_NONE = 0,
    IAL_PROTECT_SWITCH_BACK_DIS,
    IAL_PROTECT_SWITCH_BACK_EN,
}ial_protect_switch_back;


typedef struct
{
    uint32 lsp_id;
    ial_lsp_link_status lsp_ingress_status;
    ial_lsp_link_status lsp_egress_status;
}ial_mpls_lsp_status;


typedef struct
{
    uint32 grp_id;
    ial_protect_type  protect_type;
    ial_protect_switch_back          sw_back_flag;
    uint32    back_time;
    uint32    work_lsp_id;
    uint32    backup_lsp_id;
}ial_mpls_protect_group_cfg;

typedef struct
{
    uint32 grp_id;
    ial_protect_type  protect_type;
    ial_protect_switch_status  switch_status;;
    ial_protect_switch_back          sw_back_flag;
    uint32    back_time;
    uint32    lock_lsp_id;
    uint32    current_lsp_id;
    uint32    work_lsp_id;
    uint32    backup_lsp_id;
}ial_mpls_protect_group_info;


typedef struct
{
    uint32 grp_id;
    uint32 switch_lsp_id;
    ial_protect_switch_status  switch_status;
    uint32    lock_lsp_id;
}ial_mpls_protect_switch_cfg;


typedef struct 
{
    bcm_mac_t     des_mac;
    bcm_vlan_t     l3_vid;
    bcm_port_t    out_port;
    bcm_mpls_label_t out_lsp_label; 
    uint8                     out_lsp_exp; 
    bcm_if_t        l3_if;
    bcm_if_t        egr_if;
    int                  egr_mac_da_index;/* add from hsl-0213 @zlh0924 */
    int                 cnt;
	//ial_lsp_type  lsp_type;//add @zlh1015 传递lsp_type
}hsl_bcm_mpls_tunnel_egress_cfg;

typedef struct 
{
	bcm_mac_t   	  mac;	    //l3接口smac
	//bcm_mac_t   	  mac[6];	    //l3接口smac 	
	unsigned long    vlanid;	    //l3接口vid
        bcm_mpls_label_t out_lsp_label; 
        uint8                     out_lsp_exp;
	bcm_if_t             l3_if;
	int                      cnt;          /*记录次数*/
	//ial_lsp_type  lsp_type;//add @zlh1015 传递lsp_type
	
}hsl_bcm_mpls_tunnel_l3if_cfg;
/*
typedef enum 
{
    HSL_MPLS_LABEL_SWITCH,
	HSL_MPLS_LABEL_POP,
	HSL_MPLS_LABEL_PHP,
	HSL_MPLS_LABEL_INVALID
}hsl_mpls_label_action;
*/

//@zlh0923 match lsp_type
/*
typedef enum 
{
	HSL_MPLS_LABEL_INVALID = 0,
	HSL_MPLS_LABEL_PHP,
	HSL_MPLS_LABEL_POP,
    HSL_MPLS_LABEL_SWITCH
}hsl_mpls_label_action;
*/
#define HSL_MPLS_LABEL_SPACE_GLOBAL      0   /*基于设备分配标签*/
#define HSL_MPLS_LABEL_SPACE_PORT        1   /*基于端口分配标签*/

/* add from hsl-0213 @zlh0923 */
typedef struct
{
     unsigned int lsp_id;
     ial_lsp_type  lsp_type;

     /* ingress */
     unsigned int in_port;
     unsigned int in_label;
     unsigned short in_vlan;

    /* egress */
     unsigned int out_port;
     unsigned int out_label;
     unsigned char   out_label_exp;
     unsigned short out_vlan;  
     //unsigned char   des_mac[6];
     uint8 des_mac[6];//@zlh0923
}hsl_bcm_mpls_tunnel_switch_cfg;


typedef struct 
{
    unsigned long in_label;        /*入标签*/
    unsigned long out_label;       /*mpls 出标签*/
    //unsigned long label_action;  /*pop, swtich*/
    ial_lsp_type  lsp_type;//@zlh0924
    //unsigned char des_mac[6];    /*吓一跳mac地址*/
    uint8 des_mac[6];//@zlh0923
    unsigned long in_port;
    unsigned long out_port;
    unsigned char out_lsp_exp;
    int   mpls_in_vlan;
    int   mpls_out_vlan;
    int   cnt;
	int	  lsp_id;//@zlh0922
}hsl_bcm_mpls_tunnel_switch_cfg_1307;



/* add from hsl-0213 @zlh0923 */
typedef enum
{
     HSL_LSP_NODE_TYPE_PE,  /*pe节点*/
     HSL_LSP_NODE_TYPE_P     /*p 节点*/
}hsl_lsp_node_type;


/*单向隧道信息*/
typedef struct
{
      hsl_cfg           hsl_cfg;
      ial_lsp_type   lsp_type;
      uint32 in_port;
      uint32 in_label;
      bcm_vlan_t in_vlan;
      bcm_mac_t  src_mac;
   //   ial_qos_pipe_mode  qos_mode;

      uint32   mpls_entry_index;
      uint32   egr_l3_next_hop;
      
      uint8          ing_phb_id;
      ial_lsp_link_status          lsp_status;
}hsl_lsp_ingress_info;

typedef struct
{
      hsl_cfg           hsl_cfg;
      ial_lsp_type   lsp_type;
      uint32 out_port;
      uint32 out_label;
      uint8   out_label_exp;
      uint16 out_vlan; 
      
      bcm_mac_t   des_mac;
      bcm_if_t  egr_intf;
      bcm_if_t  l3_intf;
      
     // ial_qos_pipe_mode  qos_mode;
  
      uint8          egr_phb_id;
      uint16       pw_cnt;

      uint16      egr_mac_da_index;
      uint16      egr_l3_intf_index;
      T_LINK_CONTAINER       *bind_pw_list;      
      ial_lsp_link_status          lsp_status;
}hsl_lsp_engress_info;

typedef struct
{
      uint32 tunnel_id;
      hsl_cfg  lsp_cfg;
      
      hsl_lsp_node_type       lsp_node_type;  /*节点类型，p or pe */
      hsl_lsp_ingress_info     lsp_ingress_info;  /*对于p节点，只保留在ingress lsp info*/
      hsl_lsp_engress_info    lsp_egress_info;

      uint32   protect_grp_id;
      hsl_cfg  protect_status;
}hsl_lsp_info;


typedef struct
{
    bcm_mac_t  src_mac;
    bcm_vlan_t vlan_id;
    int  cnt;
}hsl_bcm_mpls_tcam_info;


typedef struct
{
    unsigned int hsl_vpn_id;
    unsigned int  port;
    unsigned char mac_addr[6];
    unsigned char src_drop;
    unsigned char dst_drop;
	
    unsigned int svp_type; /*端口类型 AC：0； pw：1*/
    unsigned int ac_vlan;
    unsigned int pw_label;
}hsl_bcm_static_mac_cfg;

/* add from hsl-0213 @zlh0923 */
typedef struct
{
    hsl_cfg    grp_cfg;
    ial_protect_type    protect_type;
    uint32  work_lsp_id;
    uint32  backup_lsp_id;
    uint32   current_lsp_id;    /*当前保护组 工作隧道id */
    uint32    lock_lsp_id; 
    ial_protect_switch_status  switch_status;
    ial_protect_switch_back          sw_back_flag;
    uint32    back_time_cnt;
    uint32    back_time;
}hsl_mpls_protect_group_info;




/****oam **********************************/
/* Constants for CCM periods defined by 802.1ag */
#define IAL_OAM_CCM_PERIOD_DISABLED 0          
#define IAL_OAM_CCM_PERIOD_3MS     3          
#define IAL_OAM_CCM_PERIOD_10MS    10         
#define IAL_OAM_CCM_PERIOD_100MS   100        
#define IAL_OAM_CCM_PERIOD_1S      1000       
#define IAL_OAM_CCM_PERIOD_10S     10000      
#define IAL_OAM_CCM_PERIOD_1M      60000      
#define IAL_OAM_CCM_PERIOD_10M     600000     


typedef struct
{
    unsigned int   oam_id;    
    unsigned int   lsp_id;    
    unsigned int   ccm_period;   
    unsigned int   in_port;   
    unsigned int   out_port;    
    char     ma_name[48];   
    char     mep_name;
}ial_oam_cfg;

typedef struct
{
     uint32 oam_id;
     uint32 lsp_id;
     
    uint32           rdi_error;
    uint32           xcon_error;
    uint32           ccm_timeout;
    uint32           unexpected_mep;     
}ial_oam_status;

#define HSL_CHECK_OAM_ID(rc, id)  \
{   \
      if(id >= HSL_OAM_MAX)   \
     {                                       \
           printk(0,0, "oam id check: id = %d, max id = %d\n\r", id, HSL_OAM_MAX - 1);    \
            return DRV_E_OAM_ID_INVALID; \
      }                                      \
}    

typedef struct
{
    uint32   oam_id;

    hsl_cfg  hsl_cfg;

    bcm_vlan_t vlan_id;
    uint32        in_port;
    uint32        out_port;
    uint32         out_pw_label;
    uint32         in_pw_label;
    uint32         lsp_id;

    uint32        ma_id;
    uint32        lmep_id;
    uint32        rmep_id;
    
    uint32         ccm_period;
    uint8           ma_name[50];
    uint8           mep_name;

    uint32           rdi_error;
    uint32           xcon_error;
    uint32           ccm_timeout;
    uint32           unexpected_mep;
	
}hsl_oam_grp_info;




typedef struct
{
    int mpls_port_id1;
    int mpls_port_id2;
    int work_patch_status;
}hsl_bcm_prot_info;

uint32 hsl_oam_init(void);
int hsl_oam_enevs_irq(int hsl_unit,  uint32 flags, 
    bcm_oam_event_type_t event_type, 
    bcm_oam_group_t group, 
    bcm_oam_endpoint_t endpoint, 
     void *user_data);

uint32 hsl_oam_add(ial_oam_cfg *oam_cfg);
uint32 hsl_oam_del(ial_oam_cfg *oam_cfg);
void hsl_oam_evens_thread(void);
uint32 hsl_oam_get_status(ial_oam_status *oam_status);
void hsl_oam_info_display(uint32 oam_id);

/////////////////////////////////////////////////////
hsl_bcm_vpn_info  * hsl_get_l2vpn_info(uint32 vpn_id);/* add from hsl-0213 @zlh0923 */
int hsl_bcm_vpls_vpn_add ( struct hal_msg_vpls_vpn_s *vpls );
int hsl_bcm_vpls_vpn_del ( struct hal_msg_vpls_vpn_s *vpls );
int hsl_bcm_vpls_port_add (struct hal_msg_vpls_port_s *vplsPort );
int hsl_bcm_vpls_port_del (struct hal_msg_vpls_port_s *vplsPort );


unsigned int hsl_bcm_vpn_cmp(void *pdata1,   void *pdata2);
int hsl_bcm_vpn_create ( hsl_bcm_msg_vpn_cfg *vpn_cfg );
int hsl_bcm_vpn_del (hsl_bcm_msg_vpn_cfg *vpn_cfg );

unsigned int hsl_bcm_vpn_ac_port_cmd(void *pdata1,   void *pdata2);
int hsl_bcm_vpn_ac_port_add(hsl_bcm_ac_port_info *ptHslVpnAcCfg);
int hsl_bcm_vpn_ac_port_del(hsl_bcm_ac_port_info *ptHslVpnAcCfg);

unsigned int hsl_bcm_vpn_pw_port_cmd(void *pdata1,   void *pdata2);
//int hsl_bcm_vpn_pw_port_add(hsl_bcm_pw_port_info *ptHslVpnPwCfg);
int hsl_bcm_vpn_pw_port_add(hsl_bcm_pw_port_info_1307 *ptHslVpnPwCfg);//@zlh0919
//int hsl_bcm_vpn_pw_port_del(hsl_bcm_pw_port_info *ptHslVpnPwCfg);
int hsl_bcm_vpn_pw_port_del(hsl_bcm_pw_port_info_1307 *ptHslVpnPwCfg);//@zlh0919

int hsl_bcm_mulitcast_group_create(hsl_bcm_vpn_mcast_grp_cfg *pMcastGrpCfg);
int hsl_bcm_mulitcast_group_del(hsl_bcm_vpn_mcast_grp_cfg *pMcastGrpCfg);
int hsl_bcm_mulitcast_group_port_set(hsl_bcm_vpn_mcast_port_cfg *ptVpnMcastPortCfg, int Optian);//no function define in hsl_bcm_vpls.c @zlh0910

int hsl_bcm_vpn_static_macaddr_set(hsl_bcm_static_mac_cfg *ptBcmMacCfg, int Optian);

hsl_lsp_info* hsl_mpls_get_lsp_info(uint32 lsp_id);/* add from hsl-0213 @zlh0923 */
unsigned int hsl_bcm_mpls_tunnel_l3if_cmp(void *pdata1, void *pdata2);
//int hsl_bcm_mpls_tunnel_l3if_add(hsl_bcm_mpls_tunnel_l3if_cfg *l3inf_cfg);
int hsl_bcm_mpls_tunnel_l3if_add(hsl_bcm_mpls_tunnel_l3if_cfg *l3intf_cfg, ial_lsp_type lsp_l3_type );//@zlh1017

int hsl_bcm_mpls_tunnel_l3if_del(hsl_bcm_mpls_tunnel_l3if_cfg *l3intf_cfg);

unsigned int hsl_bcm_mpls_tunnel_l3egr_cmp(void  *pdata1, void *pdata2);
//int hsl_mpls_tunnel_egress_add(hsl_bcm_mpls_tunnel_egress_cfg *tn_egr_cfg);
int hsl_mpls_tunnel_egress_add(hsl_bcm_mpls_tunnel_egress_cfg *tn_egr_cfg, ial_lsp_type  lsp_egr_type);//@zlh1017

int hsl_mpls_tunnel_egress_del(hsl_bcm_mpls_tunnel_egress_cfg *tn_egr_cfg);

unsigned int hsl_bcm_mpls_tunnel_cmp(void *pdata1,	void *pdata2);
//int hsl_bcm_mpls_tunnel_add(hsl_bcm_mpls_tunnel_switch_cfg *label_switch_info);
int hsl_bcm_mpls_tunnel_add(hsl_bcm_mpls_tunnel_switch_cfg_1307 *label_switch_info);
//int hsl_bcm_mpls_tunnel_del(hsl_bcm_mpls_tunnel_switch_cfg *label_switch_info);
int hsl_bcm_mpls_tunnel_del(hsl_bcm_mpls_tunnel_switch_cfg_1307 *label_switch_info);


uint32 hsl_mpls_get_lsp_status(ial_mpls_lsp_status *lsp_status);/* add from hsl-0213 @zlh0923 */
/* add from hsl-0213 @zlh0923 */
int hsl_mpls_protect_group_add(ial_mpls_protect_group_cfg *pro_grp_cfg);
int hsl_mpls_protect_group_del(ial_mpls_protect_group_cfg *pro_grp_cfg);
uint32 hsl_mpls_protect_group_mod_param(ial_mpls_protect_group_cfg *pro_grp_cfg);
uint32 hsl_mpls_protect_group_switch(ial_mpls_protect_switch_cfg *grp_sw_cfg);
uint32 hsl_mpls_get_protect_grp_info(ial_mpls_protect_group_info *pro_grp_cfg);

unsigned int hsl_bcm_mpls_station_tcam_cmp(void *pdata1,   void *pdata2);
unsigned int hsl_bcm_mpls_station_tcam_add(bcm_mac_t  src_mac,  bcm_vlan_t  vlan_id);
unsigned int hsl_bcm_mpls_station_tcam_del(bcm_mac_t  src_mac,  bcm_vlan_t  vlan_id);
int hsl_bcm_pvid_action_set(int port, int op, int pvid);
//int hsl_bcm_pvid_action_new_set(int port, int op, int pvid);
int hsl_bcm_vlan_new_create(int vid);
int hsl_bcm_vlan_new_delete(int vid);
int hsl_bcm_vlan_new_add_vid_to_port(int port, int vid, int untag_flag);
int hsl_bcm_vlan_new_rmv_vid_from_port(int port, int vid);
int hsl_bcm_vlan_new_add_mcast_to_port(int port, int vid, char *mac);
int hsl_bcm_vlan_new_rmv_mcast_to_port(int port, int vid, char *mac);



int    hsl_bcm_l2_switch_disable(bcm_vlan_t l2_switch_vlan, int enable);
int hsl_svp_in_disable(int if_class_id);

int hsl_bcm_vpn_init(void);

void show_lsp_info( uint32 lsp_id);


/***qos****/


/*************qos error code *************************************/
#define DRV_E_QOS_FUN_ID_INVALID                        0x6500
#define DRV_E_QOS_PHB_ID_NO_EXIST                     0x6501
#define DRV_E_QOS_PHB_ID_INVALID                        0x6502
#define DRV_E_QOS_COLOR_INVALID                         0x6503


typedef enum
{
      IAL_QOS_COLOR_GREEN = 0,
      IAL_QOS_COLOR_YELLOW,
      IAL_QOS_COLOR_RED,
      IAL_QOS_COLOR_MAX
}ial_qos_color;

typedef enum
{
    IAL_QOS_EXP_0 = 0,
    IAL_QOS_EXP_1,
    IAL_QOS_EXP_2,
    IAL_QOS_EXP_3,
    IAL_QOS_EXP_4,
    IAL_QOS_EXP_5,
    IAL_QOS_EXP_6,
    IAL_QOS_EXP_7,
    IAL_QOS_EXP_MAX,
}hsl_qos_exp;

typedef enum
{
    IAL_QOS_MOD_INVALID=0,
    IAL_QOS_MOD_UNIFORM,
    IAL_QOS_MOD_PIPE,
    IAL_QOS_MOD_SHORT_PIPE
}ial_qos_pipe_mode;


#define IAL_QOS_PHB_ID_MIN   (0)

#define IAL_QOS_PHB_ID_INVALID   (0xff)
#define IAL_QOS_PHB_ID_DEFAULT  (0)


typedef struct
{
    uint8     phb_id;    /*vlan pri: 1~64,  exp: 1~16*/
    uint8     color[IAL_QOS_EXP_MAX];
    uint8     inter_pri[IAL_QOS_EXP_MAX];
}ial_qos_in_phb_cfg;

typedef struct
{
     uint8   phb_id;  /*1~64*/
     uint8   user_pri[IAL_QOS_COLOR_MAX][IAL_QOS_EXP_MAX];  /*  报文优先级字段 */
}ial_qos_out_phb_cfg;



typedef struct
{
    uint32 vpn_id;
    ial_qos_pipe_mode  pw_qos_mode;
    uint8  ing_pri_phb_id;
    uint8  egr_pri_phb_id;
    uint8 ing_exp_phb_id;
    uint8 egr_exp_phb_id;
}ial_qos_vpn_cfg;


typedef struct
{
    uint32 logic_port;
    uint32 cos_num;
    uint32 red_limit;
    uint32 yellow_limit;
    uint32 all_limit;
}ial_qos_tail_drop_cfg;

typedef struct
{
    uint32 logic_port;
    uint32 cos_num;
    
    uint32 en_flg;

    uint32 green_min;
    uint32 green_max;
    uint32 green_drop_prob;

    uint32 yellow_min;
    uint32 yellow_max;
    uint32 yellow_drop_prob;

    uint32 red_min;
    uint32 red_max;
    uint32 red_drop_prob;
}ial_qos_wred_drop_cfg;

typedef enum
{
    IAL_QOS_PORT_SCHED_MOD_SP = 0,
	IAL_QOS_PORT_SCHED_MOD_RR,//zlh@141229
    IAL_QOS_PORT_SCHED_MOD_WRR,
    IAL_QOS_PORT_SCHED_MOD_DWRR,
}ial_qos_port_sched_mod;

typedef struct
{
    uint32 logic_port;
    ial_qos_port_sched_mod   sched_mode;
    uint32 wrr_weight[8];
}ial_qos_port_sched_cfg;


#define HSL_ING_PRI_CNG_MAP_NUM    64
#define HSL_ING_EXP_CNG_MAP_NUM   16

#define HSL_EGR_MPLS_PRI_MAP_NUM  16  /*inter pri to exp */
#define HSL_EGR_MPLS_EXP_MAP_NUM  8
#define HSL_EGR_MPLS_EXP_PRI_MAP_NUM  8


#define HSL_QOS_DEFAULT_PHB_ID   (0)
#define HSL_QOS_PORT_PKT_DEFAULT_LIMIT (256)

#define HSL_PACKET_FIELD_MAX     0x3fff


typedef enum
{
    IAL_QOS_TYPE_PRI_TO_INTER = 0,
    IAL_QOS_TYPE_EXP_TO_INTER,
    IAL_QOS_TYPE_INTER_TO_PRI,
    IAL_QOS_TYPE_INTER_TO_EXP,
    IAL_QOS_TYPE_EXP_TO_PRI,
}hsl_qos_type;

typedef struct 
{
    hsl_cfg  hsl_cfg;
    uint32   bcm_map_id;
    uint8     color[IAL_QOS_EXP_MAX];
    uint8     inter_pri[IAL_QOS_EXP_MAX];
}hsl_qos_inphb_info;

typedef struct 
{
    hsl_cfg  hsl_cfg;
    uint32   bcm_map_id; 
    uint8     pkt_pri[IAL_QOS_COLOR_MAX][16];
}hsl_qos_outphb_info;

typedef struct
{
    hsl_cfg  hsl_cfg;
    uint32   bcm_map_id; 
    uint8     pkt_pri[IAL_QOS_EXP_MAX];
}hsl_qos_exp_pri_info;

typedef struct 
{
    hsl_qos_inphb_info        hsl_qos_in_pri_tab[HSL_ING_PRI_CNG_MAP_NUM];
    hsl_qos_inphb_info        hsl_qos_in_exp_tab[HSL_ING_EXP_CNG_MAP_NUM];
    hsl_qos_outphb_info      hsl_qos_out_exp_tab[HSL_EGR_MPLS_EXP_MAP_NUM];
}hsl_qos_phb;


uint32 hsl_qos_interpri_to_cosq(uint32 port, uint32 pri, uint32 cos_q);
uint32 hsl_qos_pri_to_inter_tab_add(ial_qos_in_phb_cfg *phb_cfg);
uint32 hsl_qos_pri_to_inter_tab_del(ial_qos_in_phb_cfg *phb_cfg);
uint32 hsl_qos_exp_to_inter_tab_add(ial_qos_in_phb_cfg *phb_cfg);
uint32 hsl_qos_exp_to_inter_tab_del(ial_qos_in_phb_cfg *phb_cfg);
uint32 hsl_qos_inter_to_exp_tab_add(ial_qos_out_phb_cfg *out_phb_cfg);
uint32 hsl_qos_inter_to_exp_tab_del(ial_qos_out_phb_cfg *out_phb_cfg);
uint32 hsl_qos_to_bcm_color(uint32 color, bcm_color_t *bcm_color);
uint32 hsl_qos_get_bcm_map_id(uint8 phb_id, hsl_qos_type qos_type,  uint32 *bcm_map_id);
uint32 hsl_qos_set_tail_drop(ial_qos_tail_drop_cfg *qos_tail_drop);

int hsl_eport_get_mac_addr_by_bcmport(uint8 bcm_port, bcm_mac_t *mac_addr);

uint32 hsl_mpls_protect_group_switch_1by1(uint32 grp_id, uint32 sw_lsp_id);/* add from hsl-0213 @zlh0923 */
uint32 hsl_mpls_protect_group_switch_1by1snc(uint32 grp_id, uint32 sw_lsp_id);/* add from hsl-0213 @zlh0923 */
/* add from hsl-0213 @zlh0923 */
uint32 hsl_bcm_port__init(void);/* add from hsl-0213 @zlh0923 */

uint32 hsl_qos_init(void);/* add from hsl-0213 @zlh0923 */
uint32 hsl_qos_interpri_to_cosq(uint32 port, uint32 pri, uint32 cos_q);/* add from hsl-0213 @zlh0923 */
uint32 hsl_qos_port_sched_set(ial_qos_port_sched_cfg  *sched_cfg);/* add from hsl-0213 @zlh0923 */
void hsl_fp_port_untagged_priority_set(int unit, bcm_port_t port, int priority);/* add from hsl-0213 @zlh0923 */

/* add poll timer test handler */
static void polling_handler(unsigned long data);


#endif /* _HSL_VPWS_H_ */
