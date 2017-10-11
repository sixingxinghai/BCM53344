#include "config.h"

#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_logs.h"

#include "bcm_incl.h"

#include <soc/debug.h>
#include <bcm/error.h>
#include <bcm/debug.h>

#include <bcm_int/common/multicast.h>
#include "hsl_rsrc.h"
#include "hsl_linklist.h"
#include "hal_vpls.h"
#include "hsl_bcm_vpls.h"
#include "hsl_bcm_vpws.h"
#include "hsl_error.h"
#include "hsl_oss.h" //@zlh1024
#include <bcm/mpls.h> //@zlh1030
#include <linux/timer.h>//zlh@150316
/////////////////////////////////////////////////////////////
#define MPLS_DEF_EXP 0
#define MPLS_DEF_TTL 127
#define DEF_MTU 1800
#define VLAN_NUM 3499//@zlh0913 vlan start
#define PW_NUM   0xA0000//@zlh0916 pw lable -lsp label
#define VPN_NUM  1530//@zlh0916
//#define OAM_ENDPOINT_PORT_1 24
//#define OAM_ENDPOINT_PORT_2 25

/*zlh@2015.11.30,use to old PTN,use port fe16 fe17 as oam endpoint port  */
#define OAM_ENDPOINT_PORT_1 18
#define OAM_ENDPOINT_PORT_2 19


#define HSL_VPLS_ROE(op, arg)     \
    do {               \
        int __rv__;    \
        __rv__ = op arg;    \
        if (BCM_FAILURE(__rv__)) { \
	        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, \
			"HSL test VPLS: Error: " #op " failed, %s\n", \
                           bcm_errmsg(__rv__)); \
            return HSL_FAIL; \
        } \
    } while(HSL_OK)

///define/////////////////////////////////////
#define HSL_FAIL -1
#define HSL_OK 0

int
port_to_gport( 
int unit,
bcm_port_t port )
{
	bcm_gport_t gport;
	HSL_VPLS_ROE(bcm_port_gport_get,( unit, port, &gport));
	return gport;
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

///parameters/////////////////////////////////////////

	  
//bcm_mac_t  lerdmac = "00:4e:a5:F0:10:01";
//bcm_mac_t  ipsmac  = "00:0a:0b:0c:0d:0e";
extern bcm_mac_t  smac_address_0;
extern bcm_mac_t  dmac_address_1;

hsl_mac_addr g_PortSrcMac[HSL_BCM_MAX_PORT + 1];

T_LINK_CONTAINER *g_hsl_l3_intf_list = NULL;
T_LINK_CONTAINER *g_hsl_l3_egr_list = NULL;
T_LINK_CONTAINER *g_hsl_mpls_station_tcam_list = NULL;
hsl_bcm_vpn_info      g_hsl_vpn_list[HSL_L2VPN_MAX];
T_LINK_CONTAINER *g_hsl_lsp_list = NULL;

hsl_lsp_info g_hsl_lsp_info[HSL_MPLS_LSP_MAX];
uint32 g_vpls_debug_on = 0;
hsl_oam_grp_info g_hsl_oam_group_info[HSL_OAM_MAX];


typedef void *oam_sem_t;//@zlh1022
oam_sem_t hsl_oam_evens_sem = NULL;

static volatile sal_thread_t hsl_oam_thread_id = SAL_THREAD_ERROR;
hsl_mpls_protect_group_info g_hsl_lsp_protect_grp_info[HSL_MPLS_PROTECT_GROUP_MAX+1];
char *hsl_base;

static struct timer_list polling_timer;//zlh@150309,define timer

int hsl_bcm_vpn_init()
{
    int rc = 0;
    int i;
    bcm_pbmp_t pbm;
    
    bcm_switch_control_set(BCM_UNIT, bcmSwitchL3EgressMode,1);
    
    rc = bcm_mpls_init(BCM_UNIT);
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_init: mpls init  err = %d \n", rc);
  	
    bcm_ipmc_init(BCM_UNIT);
    bcm_ipmc_enable(BCM_UNIT, 0);//zlh@12.08 set v4ipmc_enable=0,使能v4ipmc_enable
    
    memset(g_PortSrcMac, 0, sizeof(hsl_mac_addr) * (HSL_BCM_MAX_PORT + 1));
    memset(g_hsl_lsp_info, 0, sizeof(hsl_lsp_info) * HSL_MPLS_LSP_MAX);

    //memset(g_vlan_pro_info, 0, sizeof(hsl_bcm_prot_info) * 5000);

    for(i = 2; i <= HSL_BCM_MAX_PORT; i++)
    {
    	
        g_PortSrcMac[i].mac_addr[0] = 0x00;
        g_PortSrcMac[i].mac_addr[1] = 0x00;
        g_PortSrcMac[i].mac_addr[2] = 0x00;
        g_PortSrcMac[i].mac_addr[3] = 0x00;
        g_PortSrcMac[i].mac_addr[4] = 0x00;
        g_PortSrcMac[i].mac_addr[5] = i - 1; 
		
        bcm_port_ifilter_set(BCM_UNIT, i, 1);   /*默认开启vlan过滤功能。*/
        
        bcm_port_frame_max_set(BCM_UNIT, i, 1600);

        HSL_CHECK_RESULT(rc,bcm_port_untagged_priority_set(0, i, 0));

        memset(&pbm, 0, sizeof(bcm_pbmp_t));
        BCM_PBMP_PORT_SET(pbm, i);
        HSL_CHECK_RESULT(rc, bcm_cosq_port_sched_set(0,  pbm,  BCM_COSQ_STRICT,  NULL,  0));
    }

    bcm_switch_control_port_set(BCM_UNIT,REG_PORT_ANY, bcmSwitchMplsPortIndependentLowerRange1,0);
    bcm_switch_control_port_set(BCM_UNIT,REG_PORT_ANY, bcmSwitchMplsPortIndependentLowerRange2,0);
    bcm_switch_control_port_set(BCM_UNIT,REG_PORT_ANY, bcmSwitchMplsPortIndependentUpperRange1,10240);
    bcm_switch_control_port_set(BCM_UNIT,REG_PORT_ANY, bcmSwitchMplsPortIndependentUpperRange2,10240);

    /*配置全局标签空间*/  
    HSL_CHECK_RESULT(rc,hsl_bcm_l2_switch_disable(1, 1));
    HSL_CHECK_RESULT(rc, hsl_svp_in_disable(HSL_VPN_SVP_CLASS_ID));

    bcm_vlan_port_remove(BCM_UNIT, 1, PBMP_ALL(0));

    /*创建L3 interface 链表*/
    g_hsl_l3_intf_list = linkContainerNew(LINKLIST_NEED_SEM);
    if(g_hsl_l3_intf_list == NULL)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_init: create l3 intf list err \n");
    }
    g_hsl_l3_intf_list->pHead = NULL;
    g_hsl_l3_intf_list->pTail = NULL;
    g_hsl_l3_intf_list->count = 0;
    g_hsl_l3_intf_list->cmp   = hsl_bcm_mpls_tunnel_l3if_cmp;
    g_hsl_l3_intf_list->del   =  hsl_list_node_free;

    /*创建L3 egress interface 链表*/
    g_hsl_l3_egr_list = linkContainerNew(LINKLIST_NEED_SEM);
    if(g_hsl_l3_egr_list == NULL)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_init: create l3 egr list err \n");
    }
    g_hsl_l3_egr_list->pHead = NULL;
    g_hsl_l3_egr_list->pTail = NULL;
    g_hsl_l3_egr_list->count = 0;
    g_hsl_l3_egr_list->cmp   = hsl_bcm_mpls_tunnel_l3egr_cmp;
    g_hsl_l3_egr_list->del   =  hsl_list_node_free;


    /*创建 mpls station tcam 链表*/
    g_hsl_mpls_station_tcam_list = linkContainerNew(LINKLIST_NEED_SEM);
    if(g_hsl_mpls_station_tcam_list == NULL)
    {
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_init: create mpls station tcam list err \n");
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_init: Z L H test successful......!! \n");//test @zlh
    }
    g_hsl_mpls_station_tcam_list->pHead = NULL;
    g_hsl_mpls_station_tcam_list->pTail = NULL;
    g_hsl_mpls_station_tcam_list->count = 0;
    g_hsl_mpls_station_tcam_list->cmp   = hsl_bcm_mpls_station_tcam_cmp;
    g_hsl_mpls_station_tcam_list->del   =  hsl_list_node_free;

    memset(g_hsl_vpn_list, 0, sizeof(hsl_bcm_vpn_info) * HSL_L2VPN_MAX);
       
    //HSL_CHECK_RESULT(rc, hsl_qos_init());   /* qos init*/
    hsl_qos_init();
    hsl_oam_init();

   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl bcm vpn init .......success!!\n");

   return rc;

}  


/***************************************************************
* Function: hsl_get_vpn_info
* Description: get vpn info by vpn id
* Return:
*     DRV_OK:            
*    DRV_E_VPN_NO_EXIST
*   
*    
* Others:
***************************************************************/
hsl_bcm_vpn_info  * hsl_get_l2vpn_info(uint32 vpn_id)
{
    hsl_bcm_vpn_info *pt_vpn_info = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_get_l2_vpn_info: Z L H test successful......!! \n");//test @zlh
 
    if(vpn_id >= HSL_L2VPN_MAX)   
    {                                       
        return NULL; 
    }                                      
    
    pt_vpn_info = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[vpn_id]);
 
    return pt_vpn_info;
}

unsigned int hsl_bcm_vpn_cmp(void *pdata1,   void *pdata2)
{
     hsl_bcm_vpn_info *ptVpnInfo1 = NULL;
     hsl_bcm_vpn_info *ptVpnInfo2 = NULL;

	 
	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_cmp: Z L H test successful......!! \n");//test @zlh
     
    if((pdata1 ==NULL) || (pdata2 == NULL))
    {
             return Link_List_Compare_Mismatch;
    }

    ptVpnInfo1 = (hsl_bcm_vpn_info *)pdata1;
    ptVpnInfo2 = (hsl_bcm_vpn_info *)pdata2;

    if(ptVpnInfo1->hsl_vpn_id == ptVpnInfo2->hsl_vpn_id)
    {
          return Link_List_Compare_Match;
    }

    return Link_List_Compare_Mismatch;

}


int hsl_bcm_vpn_create (hsl_bcm_msg_vpn_cfg *vpn_cfg )
{
    unsigned int rc = 0;
    bcm_mpls_vpn_config_t bcm_vpn_cfg = {0};
    bcm_multicast_t bcast_group = BCM_VLAN_INVALID;
    hsl_bcm_vpn_info  *ptVpnInfo = NULL;
   // bcm_vlan_control_vlan_t   vlan_control;
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create: Z L H test successful......!! \n");//test @zlh
    HSL_CHECK_L2VPN_ID(rc, vpn_cfg->hsl_vpn_id);
	
    ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[vpn_cfg->hsl_vpn_id]);
    
    if(ptVpnInfo->hsl_vpn_cfg == HSL_CFG_YES)
    {
         return rc;
    }
    
    bcm_mpls_vpn_config_t_init(&bcm_vpn_cfg);

    if(vpn_cfg ->hsl_vpn_type == HSL_BCM_VPN_VPWS)
    {
        bcm_vpn_cfg.flags = BCM_MPLS_VPN_VPWS;
    }
    else
    {
        HSL_CHECK_RESULT(rc, bcm_multicast_create(BCM_UNIT, BCM_MULTICAST_TYPE_VPLS, &bcast_group));

        bcm_vpn_cfg.broadcast_group = bcast_group;
        bcm_vpn_cfg.unknown_unicast_group = bcast_group;
        bcm_vpn_cfg.unknown_multicast_group = bcast_group;

        bcm_vpn_cfg.flags = BCM_MPLS_VPN_VPLS;
    }

    HSL_CHECK_RESULT(rc, bcm_mpls_vpn_id_create(BCM_UNIT, &bcm_vpn_cfg));

    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
            "hsl_bcm_vpn_create: vpn id: 0x%x ; group_id: 0x%x; mac ler:0x%x; mcast:0x%x; vpn type: %d  %d--vpls,%d--vpws\n\r",
                     bcm_vpn_cfg.vpn, bcast_group, vpn_cfg->mac_learn_flg, vpn_cfg->un_pkt_flg, 
                                     vpn_cfg->hsl_vpn_type, HSL_BCM_VPN_VPLS, HSL_BCM_VPN_VPWS);

    sal_memset(ptVpnInfo, 0, sizeof(hsl_bcm_vpn_info));
    ptVpnInfo->hsl_vpn_id = vpn_cfg->hsl_vpn_id;
	
    ptVpnInfo->hsl_vpn_cfg = HSL_CFG_YES;
    ptVpnInfo->bcm_vpn_id = bcm_vpn_cfg.vpn;
    ptVpnInfo->bcast_group_id = bcast_group;
    ptVpnInfo->hsl_vpn_type = vpn_cfg->hsl_vpn_type;
    ptVpnInfo->mac_learn_flg = vpn_cfg->mac_learn_flg;
    ptVpnInfo->un_pkt_flg = vpn_cfg->un_pkt_flg;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create: hsl_vpn_type             = %d \n",ptVpnInfo->hsl_vpn_type);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create: bcm_vpn_id               = %d \n",ptVpnInfo->bcm_vpn_id);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create: mac_learn_flg            = %d \n",ptVpnInfo->mac_learn_flg);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create: un_pkt_flg               = %d \n",ptVpnInfo->un_pkt_flg);//test @zlh

    ptVpnInfo->hsl_ac_port_list = linkContainerNew(LINKLIST_NEED_SEM);
    if(ptVpnInfo->hsl_ac_port_list == NULL)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create: create ac port  list err \n");
    }
    ptVpnInfo->hsl_ac_port_list->pHead = NULL;
    ptVpnInfo->hsl_ac_port_list->pTail = NULL;
    ptVpnInfo->hsl_ac_port_list->count = 0;
    ptVpnInfo->hsl_ac_port_list->del   =  hsl_list_node_free;
    ptVpnInfo->hsl_ac_port_list->cmp   = hsl_bcm_vpn_ac_port_cmd;

    ptVpnInfo->hsl_pw_port_list = linkContainerNew(LINKLIST_NEED_SEM);
    if(ptVpnInfo->hsl_pw_port_list == NULL)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create: create pw port  list err \n");
    }
    ptVpnInfo->hsl_pw_port_list->pHead = NULL;
    ptVpnInfo->hsl_pw_port_list->pTail = NULL;
    ptVpnInfo->hsl_pw_port_list->count = 0;
    ptVpnInfo->hsl_pw_port_list->del    =  hsl_list_node_free;
    ptVpnInfo->hsl_pw_port_list->cmp  = hsl_bcm_vpn_pw_port_cmd;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_create_1 \n");//test @zlh

    return rc;
}

int hsl_bcm_vpn_del (hsl_bcm_msg_vpn_cfg *vpn_cfg)
{
    int rc = 0;
    hsl_bcm_vpn_info  *ptVpnInfo = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_del: Z L H test successful......!! \n");//test @zlh

    HSL_CHECK_L2VPN_ID(rc, vpn_cfg->hsl_vpn_id);
    ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[vpn_cfg->hsl_vpn_id]);

    if(ptVpnInfo->hsl_vpn_cfg == HSL_CFG_NO)
    {
        return rc;
    }

    if(ptVpnInfo->mpls_port_cnt >0)
    {
        bcm_mpls_port_delete_all(BCM_UNIT, ptVpnInfo->bcm_vpn_id);
    }

    if(vpn_cfg ->hsl_vpn_type == HSL_BCM_VPN_VPLS)
    {
        bcm_multicast_egress_delete_all(BCM_UNIT,  ptVpnInfo->bcast_group_id);
        HSL_CHECK_RESULT(rc, bcm_multicast_destroy(BCM_UNIT, ptVpnInfo->bcast_group_id));
    }

    HSL_CHECK_RESULT(rc, bcm_mpls_vpn_id_destroy(BCM_UNIT, ptVpnInfo->bcm_vpn_id));

    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
            "hsl_bcm_vpn_del: vpn id: 0x%x ;  vpn  type: %d  %d--vpls, %d--vpws\n\r",
                    ptVpnInfo->bcm_vpn_id, ptVpnInfo->hsl_vpn_type, HSL_BCM_VPN_VPLS, HSL_BCM_VPN_VPWS);

    linkListContainerFree(ptVpnInfo->hsl_ac_port_list);
    linkListContainerFree(ptVpnInfo->hsl_pw_port_list);

    sal_memset(ptVpnInfo, 0, sizeof(hsl_bcm_vpn_info));  

    return rc;
}


unsigned int hsl_bcm_vpn_ac_port_cmd(void *pdata1,   void *pdata2)
{
    hsl_bcm_ac_port_info *ptAcPortInfo1=NULL;
    hsl_bcm_ac_port_info *ptAcPortInfo2=NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_cmd: Z L H test successful......!! \n");//test @zlh

    if((pdata1 ==NULL) || (pdata2 == NULL))
    {
        return Link_List_Compare_Mismatch;
    }

    ptAcPortInfo1 = (hsl_bcm_ac_port_info *)pdata1;
    ptAcPortInfo2 = (hsl_bcm_ac_port_info *)pdata2;

    if(ptAcPortInfo1->hsl_vpn_id != ptAcPortInfo1->hsl_vpn_id)
    {
        return Link_List_Compare_Mismatch;
    }


    if((ptAcPortInfo1->ac_port == ptAcPortInfo2->ac_port)
        && (ptAcPortInfo1->ac_vlan == ptAcPortInfo2->ac_vlan))
    {
        return Link_List_Compare_Match;
    }

        return Link_List_Compare_Mismatch;
}


int hsl_bcm_vpn_ac_port_add(hsl_bcm_ac_port_info *ptHslVpnAcCfg)
{
    unsigned int rc = 0;
    bcm_gport_t bcm_gport;
    bcm_mpls_port_t bcm_ac_port;
    hsl_bcm_vpn_info  *ptVpnInfo = NULL;
    hsl_bcm_ac_port_info *ptHslAcPortInfo = NULL;
    bcm_pbmp_t  port_bmap_tag;
    uint32 egr_qos_map_id = 0;//zlh@141213
    uint32 ing_qos_map_id = 0;//zlh@141213

    HSL_CHECK_L2VPN_ID(rc, ptHslVpnAcCfg->hsl_vpn_id);
    ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[ptHslVpnAcCfg->hsl_vpn_id]);

    if(ptVpnInfo->hsl_vpn_cfg != HSL_CFG_YES)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                    "hsl_bcm_vpn_ac_port_add: vpn id = %d  \n",ptHslVpnAcCfg->hsl_vpn_id);
        return -1;
    }

    /*增加ac端口到vpn中*/
    HSL_CHECK_RESULT(rc, bcm_port_gport_get (BCM_UNIT, ptHslVpnAcCfg->ac_port, &bcm_gport));

    bcm_mpls_port_t_init(&bcm_ac_port);

    bcm_ac_port.port = bcm_gport;

    if(ptHslVpnAcCfg->ac_vlan != HSL_VLAN_INVALID)  /*端口+ vlan为ac, 使能vlan翻译功能*/
    {
          bcm_port_ifilter_set(BCM_UNIT, ptHslVpnAcCfg->ac_port, 0);  //关闭vlan过滤
        /*使能vlan 翻译功能 */
        HSL_CHECK_RESULT(rc, bcm_vlan_control_port_set(BCM_UNIT, ptHslVpnAcCfg->ac_port, bcmVlanTranslateIngressEnable, 1)); 
		
   
        bcm_ac_port.criteria = BCM_MPLS_PORT_MATCH_PORT_VLAN;
        bcm_ac_port.match_vlan = ptHslVpnAcCfg->ac_vlan;
        bcm_ac_port.flags |= BCM_MPLS_PORT_SERVICE_VLAN_ADD;
        bcm_ac_port.flags |= BCM_MPLS_PORT_SERVICE_TAGGED;
        bcm_ac_port.flags |= BCM_MPLS_PORT_SERVICE_VLAN_REPLACE;
		bcm_ac_port.service_tpid = 0x8100;
        bcm_ac_port.egress_service_vlan =  ptHslVpnAcCfg->ac_vlan; 
		
    }
    else   /*对于端口作为ac，关闭端口pvid功能*/
    {
		bcm_ac_port.criteria = BCM_MPLS_PORT_MATCH_PORT;
		
		HSL_CHECK_RESULT(rc, hsl_bcm_pvid_action_set(ptHslVpnAcCfg->ac_port, 0, 1));//zlh@150129

        bcm_ac_port.int_pri = 0;
    }

    if(ptHslVpnAcCfg->out_disable == 1)
    {
        bcm_ac_port.flags  |= BCM_MPLS_PORT_DROP;
    }

    if(ptHslVpnAcCfg->in_disable == 1)
    {
        bcm_ac_port.if_class = HSL_VPN_SVP_CLASS_ID;
    }
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add:	vpn_port_attr        = %x \n",ptHslVpnAcCfg->vpn_port_attr);//zlh@1501015 

    if(ptHslVpnAcCfg->vpn_port_attr == HSL_VPN_PORT_ATTR_SPOKE)
    {
        bcm_ac_port.flags |= BCM_MPLS_PORT_NETWORK;
    }
    else
    {
        bcm_ac_port.flags &=  (~(BCM_MPLS_PORT_NETWORK));
    }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add_1:	bcm_pw_port.flags        = %x \n",(bcm_ac_port.flags >> 5) & 0x0000000f);//zlh@1501015 

    bcm_ac_port.mtu = 0x3fff;
    bcm_ac_port.egress_tunnel_if = 0;

	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add_1: ptVpnInfo->bcm_vpn_id     = %d\n",ptVpnInfo->bcm_vpn_id);//test @zlh
	
    HSL_CHECK_RESULT(rc, bcm_mpls_port_add (BCM_UNIT, ptVpnInfo->bcm_vpn_id, &bcm_ac_port));

	/* zlh@150128,delete default vlan 1 */
    memset(&port_bmap_tag, 0, sizeof(bcm_pbmp_t));
    BCM_PBMP_PORT_SET(port_bmap_tag, ptHslVpnAcCfg->ac_port);
    bcm_vlan_port_remove(BCM_UNIT, 1, port_bmap_tag);
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add:	bcm_vlan_port_remove  successfully !!!!!!!\n");//zlh@150129
	
	#if 1
	/* add qos  */
	ptHslVpnAcCfg->ing_pri_phb_id = 0;//zlh@141213
	ptHslVpnAcCfg->egr_pri_phb_id = 0;//zlh@141213
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add_2: ing_pri_phb_id_2          = %d\n",ptHslVpnAcCfg->ing_pri_phb_id);//test @zlh
	HSL_CHECK_RESULT(rc,hsl_qos_get_bcm_map_id(ptHslVpnAcCfg->ing_pri_phb_id, IAL_QOS_TYPE_PRI_TO_INTER, &ing_qos_map_id));//zlh@141213 pri to inter priority
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add_3: egr_pri_phb_id_3          = %d\n",ptHslVpnAcCfg->egr_pri_phb_id);//test @zlh
	HSL_CHECK_RESULT(rc,hsl_qos_get_bcm_map_id(ptHslVpnAcCfg->egr_pri_phb_id, IAL_QOS_TYPE_INTER_TO_PRI, &egr_qos_map_id));//zlh@141213 inter to pri
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add_4: ing_qos_map_id_4          = %d\n",ing_qos_map_id);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add_5: egr_qos_map_id_5          = %d\n",egr_qos_map_id);//test @zlh
	
	
	bcm_qos_port_map_set(BCM_UNIT, bcm_ac_port.mpls_port_id , 0, 0);//zlh@150114 rc error don't return;
	
	#endif	
	/* zlh@141227 */
	if(ptVpnInfo->hsl_vpn_type == HSL_BCM_VPN_VPLS)
	{
		if(ptVpnInfo->mac_learn_flg == 0)
		{
			bcm_port_learn_set(BCM_UNIT, bcm_ac_port.mpls_port_id, BCM_PORT_LEARN_FWD);/*去使能mac地址学习*/
			HSL_CHECK_RESULT(rc,bcm_l2_addr_delete_by_port(BCM_UNIT, 0, bcm_ac_port.mpls_port_id, 0));
		}
		
		if(ptVpnInfo->un_pkt_flg == 1)
	     {
	     	bcm_if_t  bcm_encap_id;
			
		 	HSL_CHECK_RESULT(rc, bcm_multicast_vpls_encap_get(BCM_UNIT,  
		 	ptVpnInfo->bcast_group_id, bcm_gport, bcm_ac_port.mpls_port_id, &bcm_encap_id));
			
			HSL_CHECK_RESULT(rc,bcm_multicast_egress_add(BCM_UNIT, ptVpnInfo->bcast_group_id, bcm_gport, bcm_encap_id));
	     }

	}
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
           "hsl_bcm_vpn_ac_port_add: vpn id= 0x%x, vpn type = %d,  port = %d, vlan = %d;  mpls port id 0x%x\n", 
                 ptVpnInfo->bcm_vpn_id,  ptVpnInfo->hsl_vpn_type , ptHslVpnAcCfg->ac_port,  ptHslVpnAcCfg->ac_vlan, bcm_ac_port.mpls_port_id);//test@zlh
	
    ptHslAcPortInfo = sal_alloc(sizeof(hsl_bcm_ac_port_info), "ac_port_list");
    if(ptHslAcPortInfo == NULL)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_ac_port_add: malloc err\n");
        return -1;
    }

    sal_memset(ptHslAcPortInfo, 0, sizeof(hsl_bcm_ac_port_info));
    sal_memcpy(ptHslAcPortInfo, ptHslVpnAcCfg, sizeof(hsl_bcm_ac_port_info));
    ptHslAcPortInfo->mpls_port_id = bcm_ac_port.mpls_port_id;

    ptVpnInfo->mpls_port_cnt++;

    linkListAddAtTail(ptVpnInfo->hsl_ac_port_list, (void *)ptHslAcPortInfo);


    return rc;
}

int hsl_bcm_vpn_ac_port_del(hsl_bcm_ac_port_info *ptHslVpnAcCfg)
{
    int rc = 0;
    hsl_bcm_vpn_info  *ptVpnInfo = NULL;
    hsl_bcm_ac_port_info *ptHslAcPortInfo = NULL;
    T_LIST_NODE *ptListNode = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_ac_port_del: Z L H test successful......!! \n");//test @zlh
    HSL_CHECK_L2VPN_ID(rc, ptHslVpnAcCfg->hsl_vpn_id);
    ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[ptHslVpnAcCfg->hsl_vpn_id]);
    if(ptVpnInfo->hsl_vpn_cfg != HSL_CFG_YES)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                    "hsl_bcm_vpn_ac_port_add: vpn id = %d is not create \n",ptHslVpnAcCfg->hsl_vpn_id);
        return -1;
    }

    ptListNode = linkListLookup(ptVpnInfo->hsl_ac_port_list, (void *)ptHslVpnAcCfg);
    if(ptListNode == NULL)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
            "hsl_bcm_vpn_ac_port_del:  port = %d  vlan =%d  is not create \n",ptHslVpnAcCfg->ac_port, ptHslVpnAcCfg->ac_vlan);
        return -1;
    }

    ptHslAcPortInfo = (hsl_bcm_ac_port_info *)ptListNode->data;

    if(ptHslAcPortInfo->ac_vlan == HSL_VLAN_INVALID)
    {
        HSL_CHECK_RESULT(rc, hsl_bcm_pvid_action_set(ptHslVpnAcCfg->ac_port, 1, 1));
    }

	if((ptVpnInfo->un_pkt_flg == 1) && 
	 (ptVpnInfo->hsl_vpn_type == HSL_BCM_VPN_VPLS))
     {
          bcm_if_t  bcm_encap_id;
          bcm_gport_t bcm_gport;
		  
         HSL_CHECK_RESULT(rc, bcm_port_gport_get (BCM_UNIT, ptHslVpnAcCfg->ac_port, &bcm_gport));
	
	 	 HSL_CHECK_RESULT(rc, bcm_multicast_vpls_encap_get(BCM_UNIT,  
	 	 ptVpnInfo->bcast_group_id, bcm_gport, ptHslAcPortInfo->mpls_port_id, &bcm_encap_id));
		 
	 	 bcm_multicast_egress_delete(BCM_UNIT, ptVpnInfo->bcast_group_id, bcm_gport, bcm_encap_id);
     }

      HSL_CHECK_RESULT(rc, bcm_mpls_port_delete(BCM_UNIT, ptVpnInfo->bcm_vpn_id, ptHslAcPortInfo->mpls_port_id));

      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
   	                "hsl_bcm_vpn_ac_port_del: vpn id= 0x%x, ac mpls port id 0x%x\n", ptVpnInfo->bcm_vpn_id,ptHslAcPortInfo->mpls_port_id);

      linkListRemove(ptVpnInfo->hsl_ac_port_list, (void *)ptHslAcPortInfo);

    ptVpnInfo->mpls_port_cnt--;
    
    return rc;
}

unsigned int hsl_bcm_vpn_pw_port_cmd(void *pdata1,   void *pdata2)
{
    hsl_bcm_pw_port_info *ptPwPortInfo1=NULL;
    hsl_bcm_pw_port_info *ptPwPortInfo2=NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_port_cmd: Z L H test successful......!! \n");//test @zlh

    if((pdata1 ==NULL) || (pdata2 == NULL))
    {
        return Link_List_Compare_Mismatch;
    }

    ptPwPortInfo1 = (hsl_bcm_pw_port_info *)pdata1;
    ptPwPortInfo2 = (hsl_bcm_pw_port_info *)pdata2;

    if(ptPwPortInfo1->match_pw_label == ptPwPortInfo2->match_pw_label)
    {
        return Link_List_Compare_Match;

    }

    return Link_List_Compare_Mismatch;
}


/* add from hsl-svn @zlh0918 */
int hsl_bcm_vpn_pw_port_add(hsl_bcm_pw_port_info_1307 *ptHslVpnPwCfg)
{
	uint32 ing_phb_id; // zlh@0611 chg
	uint32 egr_phb_id;	
	bcm_port_t   in_port;
    bcm_port_t   out_port;
	unsigned int rc = 0;
    hsl_bcm_mpls_tunnel_egress_cfg bcm_mpls_egr_info;
    bcm_gport_t bcm_gport;
    bcm_mpls_port_t bcm_pw_port;
    hsl_bcm_vpn_info  *ptVpnInfo = NULL;
    hsl_bcm_pw_port_info_1307 *ptHslPwPortInfo = NULL;
    //hsl_bcm_vpn_info  stVpnInfo={0};
    hsl_lsp_info *lsp_info;
    hsl_bcm_mpls_tunnel_switch_cfg_1307  stTunnelCfg = {0};
    //T_LIST_NODE *ptListNode = NULL;
    int            vp;
	
	
    HSL_CHECK_L2VPN_ID(rc, ptHslVpnPwCfg->hsl_vpn_id);//@zlh0922
    
    ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[ptHslVpnPwCfg->hsl_vpn_id]);//@zlh0922
     
     if(ptVpnInfo->hsl_vpn_cfg != HSL_CFG_YES)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                    "hsl_bcm_vpn_pw_port_add: vpn id = %d  \n",ptHslVpnPwCfg->hsl_vpn_id);
        return -1;
    }

	 //ptVpnInfo = (hsl_bcm_vpn_info *)ptListNode->data;
	lsp_info = hsl_mpls_get_lsp_info(ptHslVpnPwCfg->lsp_id);
		 
    if(lsp_info == NULL)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    if(lsp_info->lsp_ingress_info.hsl_cfg == HSL_CFG_YES)//下环or过环
    {
        in_port = lsp_info->lsp_ingress_info.in_port;
    }
    else
    {
         in_port = lsp_info->lsp_egress_info.out_port;
    }

    if(lsp_info->lsp_egress_info.hsl_cfg == HSL_CFG_YES)//上环or过环
    {
        out_port = lsp_info->lsp_egress_info.out_port;
    }
     
    HSL_CHECK_RESULT(rc, bcm_port_gport_get(BCM_UNIT, in_port, &bcm_gport));//通过lsp的in_port获得bcm_gport在这个过程lsp与pw建立联系	
 
    bcm_mpls_port_t_init(&bcm_pw_port);	

   if(ptHslVpnPwCfg->match_pw_label <= 10240)
   {
        bcm_pw_port.criteria = BCM_MPLS_PORT_MATCH_LABEL;//MPLS label
        bcm_pw_port.port = bcm_gport;
   }
   else
   {
         bcm_pw_port.criteria = BCM_MPLS_PORT_MATCH_LABEL_PORT;//MPLS label + Mod/port/trunk
         bcm_pw_port.port = bcm_gport;
   }   	

   bcm_pw_port.flags |= BCM_MPLS_PORT_EGRESS_TUNNEL;//specified egress tunnul interface is valid
   
   bcm_pw_port.match_label = ptHslVpnPwCfg->match_pw_label;
   bcm_pw_port.egress_label.label = ptHslVpnPwCfg->out_pw_label;
   bcm_pw_port.egress_label.qos_map_id = 0;
   bcm_pw_port.egress_label.exp = ptHslVpnPwCfg->out_pw_exp;
   bcm_pw_port.egress_label.ttl = 255;
   bcm_pw_port.egress_label.pkt_pri = 0;
   bcm_pw_port.egress_label.pkt_cfi = 0;
   bcm_pw_port.mtu = 0xfff;
   bcm_pw_port.exp_map = 0;//zlh@141215
   bcm_pw_port.flags |= BCM_MPLS_PORT_INNER_VLAN_PRESERVE;//preserve the inner or customer VLAN tag
   		
   /*nni 端口使能mpls look */
    HSL_CHECK_RESULT(rc, bcm_port_control_set(BCM_UNIT, lsp_info->lsp_egress_info.out_port, bcmPortControlMpls, 1));
 
   bcm_pw_port.egress_tunnel_if = lsp_info->lsp_egress_info.egr_intf;//@zlh0928
    

   if(ptHslVpnPwCfg->vpn_port_attr == HSL_VPN_PORT_ATTR_SPOKE)
   {
         bcm_pw_port.flags |=	BCM_MPLS_PORT_NETWORK;// network-facing interface
   }
    else
    {
        bcm_pw_port.flags &=  (~(BCM_MPLS_PORT_NETWORK));//zlh@150115 , hub-member mode
    }
   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add_1:  bcm_pw_port.flags  = %x \n",(bcm_pw_port.flags >> 5) & 0x0000000f);//test @zlh

    if(ptHslVpnPwCfg->in_disable == 1)
   {
          bcm_pw_port.if_class = HSL_VPN_SVP_CLASS_ID;
   }
	
    if(ptHslVpnPwCfg->out_disable == 1)
    {

           bcm_pw_port.flags  |= BCM_MPLS_PORT_DROP;//Drop matching packets
    }


	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
	   "hsl_bcm_vpn_pw_port_add: vpn id= 0x%x, port = %d,  mpls port id 0x%x\n", 
	   ptVpnInfo->bcm_vpn_id,	lsp_info->lsp_egress_info.out_port, bcm_pw_port.mpls_port_id);

	#if 1
 	/* zlh@141213 add qos*/
	bcm_pw_port.flags |= BCM_MPLS_PORT_INT_PRI_MAP; 

	ptHslVpnPwCfg->ing_pri_phb_id = 0;//zlh@141213
	ptHslVpnPwCfg->egr_pri_phb_id = 0;//zlh@141213
	ptHslVpnPwCfg->pw_qos_mode = IAL_QOS_MOD_UNIFORM;//zlh@141226
                      
	HSL_CHECK_RESULT(rc,hsl_qos_get_bcm_map_id(ptHslVpnPwCfg->ing_pri_phb_id, IAL_QOS_TYPE_EXP_TO_INTER, &(bcm_pw_port.exp_map)));//zlh@141213
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: exp_map                 = %d\n",bcm_pw_port.exp_map);//test @zlh

        /*nni 端口使能mpls look */
     HSL_CHECK_RESULT(rc, bcm_port_control_set(BCM_UNIT, bcm_gport, bcmPortControlMpls, 1));

        /*统一模式，使用内部优先级到exp的映射*/
      if(ptHslVpnPwCfg->pw_qos_mode == IAL_QOS_MOD_UNIFORM) 
      {
      		bcm_pw_port.egress_label.flags |= BCM_MPLS_EGRESS_LABEL_EXP_REMARK;//use mapping
      		
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: pw_qos_mode				= %d\n",ptHslVpnPwCfg->pw_qos_mode);//test @zlh
			
      		/*pw inter pri to exp map */ 
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: qos_map_id				= %d\n",bcm_pw_port.egress_label.qos_map_id);//test @zlh
            HSL_CHECK_RESULT(rc,hsl_qos_get_bcm_map_id(ptHslVpnPwCfg->egr_pri_phb_id, IAL_QOS_TYPE_INTER_TO_EXP, &(bcm_pw_port.egress_label.qos_map_id)));                                  
        }
	  
	 HSL_CHECK_RESULT(rc, bcm_mpls_port_add(BCM_UNIT, ptVpnInfo->bcm_vpn_id, &bcm_pw_port));
	 
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: mpls_port_id              = %d\n",bcm_pw_port.mpls_port_id);//test
	
	bcm_qos_port_map_set(0, bcm_pw_port.mpls_port_id, 0, 0);//zlh@150114 rc error don't return;

	#endif
   
     if(ptVpnInfo->hsl_vpn_type == HSL_BCM_VPN_VPLS)
     {
	    if(ptVpnInfo->mac_learn_flg == 0)
	    {
	           bcm_port_learn_set(BCM_UNIT, bcm_pw_port.mpls_port_id, BCM_PORT_LEARN_FWD);  /*去使能mac地址学习*/
			   

	           HSL_CHECK_RESULT(rc,bcm_l2_addr_delete_by_port(BCM_UNIT, 0, bcm_pw_port.mpls_port_id, 0));
	    }
		
		
	     if(ptVpnInfo->un_pkt_flg == 1)
	     {
	           bcm_if_t  bcm_encap_id;
		   HSL_CHECK_RESULT(rc, bcm_multicast_vpls_encap_get(BCM_UNIT,  
		 	ptVpnInfo->bcast_group_id, bcm_gport, bcm_pw_port.mpls_port_id, &bcm_encap_id));
		   
	
		   HSL_CHECK_RESULT(rc,bcm_multicast_egress_add(BCM_UNIT, ptVpnInfo->bcast_group_id, bcm_gport, bcm_encap_id));
	     }
	    
     	}

	 ptHslPwPortInfo = sal_alloc(sizeof(hsl_bcm_pw_port_info_1307), "pw_port_list");//@zlh1008

	 
     if(ptHslPwPortInfo == NULL)
     {
                   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: malloc err\n");
		   return -1;
      }
	
    sal_memset(ptHslPwPortInfo, 0, sizeof(hsl_bcm_pw_port_info_1307));//@zlh1108
	sal_memcpy(ptHslPwPortInfo, ptHslVpnPwCfg, sizeof(hsl_bcm_pw_port_info_1307));//@zlh1008
	ptHslPwPortInfo->mpls_port_id = bcm_pw_port.mpls_port_id;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: mpls_port_id =          %d\n",bcm_pw_port.mpls_port_id);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: in_port =               %d\n",in_port);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: out_port =              %d\n",out_port);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: match_pw_label =        %d\n",ptHslPwPortInfo->match_pw_label);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: out_pw_label =          %d\n",ptHslPwPortInfo->out_pw_label);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: out_pw_exp =            %d\n",ptHslPwPortInfo->out_pw_exp);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_add: hsl_vpn_id =            %d\n",ptHslPwPortInfo->hsl_vpn_id);//test @zlh
	
	 linkListAddAtTail(ptVpnInfo->hsl_pw_port_list, (void *)ptHslPwPortInfo);     

     ptVpnInfo->mpls_port_cnt++;
	 
	 if(lsp_info->lsp_egress_info.hsl_cfg != HSL_CFG_YES)
    {
        return DRV_E_LSP_NO_EXIST;
    }
    
    if((ptHslPwPortInfo != NULL) && (ptHslPwPortInfo->match_pw_label < 0xf0000))
    {
        linkListAddAtTail(lsp_info->lsp_egress_info.bind_pw_list, ptHslPwPortInfo);
        lsp_info->lsp_egress_info.pw_cnt++;
    }

    return rc;
}

/* add from hsl-svn @zlh0918 */
int hsl_bcm_vpn_pw_port_del(hsl_bcm_pw_port_info_1307 *ptHslVpnPwCfg)
{
     int rc = 0;
     hsl_bcm_vpn_info  *ptVpnInfo = NULL;
     hsl_bcm_pw_port_info_1307 *ptHslPwPortInfo = NULL;
     hsl_bcm_mpls_tunnel_egress_cfg bcm_mpls_egr_info;
     hsl_bcm_mpls_tunnel_switch_cfg_1307  stTunnelCfg = {0};
     T_LIST_NODE *ptListNode = NULL;
	 hsl_lsp_info		 *lsp_info = NULL;
		
     
	 HSL_CHECK_L2VPN_ID(rc, ptHslVpnPwCfg->hsl_vpn_id);//@zlh0922
	 ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[ptHslVpnPwCfg->hsl_vpn_id]);//@zlh0922
	
	if(ptVpnInfo->hsl_vpn_cfg != HSL_CFG_YES)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_bcm_vpn_ac_port_add: vpn id = %d is not create \n",ptHslVpnPwCfg->hsl_vpn_id);
        return -1;
    }

	ptListNode = linkListLookup(ptVpnInfo->hsl_pw_port_list, (void *)ptHslVpnPwCfg);
    if(ptListNode == NULL)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                    "hsl_bcm_vpn_pw_port_del:  port = %d  pw_label =%d  is not create \n",
                                       ptHslVpnPwCfg->out_port, ptHslVpnPwCfg->match_pw_label);
        return -1;
    }
    ptHslPwPortInfo = (hsl_bcm_pw_port_info_1307*)ptListNode->data;
	
	lsp_info = hsl_mpls_get_lsp_info(ptHslPwPortInfo->lsp_id);//获取隧道信息和egr intf  @zlh0922	 

    if(lsp_info == NULL)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    if(lsp_info->lsp_egress_info.hsl_cfg != HSL_CFG_YES)
    {
        return DRV_E_LSP_NO_EXIST;
    }
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_del_2: ptHslPwPortInfo->out_port\n",ptHslPwPortInfo->out_port);//test @zlh
	
    if(ptHslPwPortInfo != NULL)
    {
        linkListRemove(lsp_info->lsp_egress_info.bind_pw_list, ptHslPwPortInfo);
           lsp_info->lsp_egress_info.pw_cnt--;
    }

     if((ptVpnInfo->un_pkt_flg == 1) && 
	 (ptVpnInfo->hsl_vpn_type == HSL_BCM_VPN_VPLS))
     {
          bcm_if_t  bcm_encap_id;
          bcm_gport_t bcm_gport;
		 
     HSL_CHECK_RESULT(rc, bcm_port_gport_get (BCM_UNIT, ptHslPwPortInfo->out_port, &bcm_gport));
         
	
	 HSL_CHECK_RESULT(rc, bcm_multicast_vpls_encap_get(BCM_UNIT,  ptVpnInfo->bcast_group_id, bcm_gport, ptHslPwPortInfo->mpls_port_id, &bcm_encap_id));
	 
	 bcm_multicast_egress_delete(BCM_UNIT, ptVpnInfo->bcast_group_id, bcm_gport, bcm_encap_id);
	
     }

    HSL_CHECK_RESULT(rc, bcm_mpls_port_delete(BCM_UNIT, ptVpnInfo->bcm_vpn_id, ptHslPwPortInfo->mpls_port_id));

   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_vpn_pw_port_del_3: ptHslPwPortInfo->out_port\n",ptHslPwPortInfo->out_port);//test @zlh
    if(ptHslPwPortInfo != NULL)
    {
        linkListRemove(lsp_info->lsp_egress_info.bind_pw_list, (void *)ptHslPwPortInfo);
    }
   linkListRemove(ptVpnInfo->hsl_pw_port_list, (void *)ptHslPwPortInfo);	

    ptVpnInfo->mpls_port_cnt--;

    return rc;
}

int hsl_bcm_mulitcast_group_create(hsl_bcm_vpn_mcast_grp_cfg *pMcastGrpCfg)
{
       int rc =0;
      bcm_multicast_t bcast_group = BCM_VLAN_INVALID;
      bcm_l2_addr_t   l2_addr;
      hsl_bcm_vpn_info  *ptVpnInfo = NULL;
      hsl_bcm_vpn_info   stVpnInfo = {0};
      T_LIST_NODE *ptListNode = NULL;	 
      hsl_bcm_vpn_mcast_grp_cfg *pVpnMcastInfo = NULL;

	  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mulitcast_group_create: Z L H test successful......!! \n");//test @zlh
	  
      stVpnInfo.hsl_vpn_id = pMcastGrpCfg->hsl_vpn_id;
      stVpnInfo.hsl_vpn_type = HSL_BCM_VPN_VPLS;
	  
	  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mulitcast_group_create: hsl_vpn_id = %d\n", stVpnInfo.hsl_vpn_id);//test @zlh
	  
	  HSL_CHECK_L2VPN_ID(rc, pMcastGrpCfg->hsl_vpn_id);
	  
	  ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[stVpnInfo.hsl_vpn_id]);

	  if(ptVpnInfo->hsl_vpn_cfg != HSL_CFG_YES)
     {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                         "hsl_bcm_mulitcast_group_del: vpn id = %d is not create \n", pMcastGrpCfg->hsl_vpn_id);
        return -1;
     }

      //ptListNode = linkListLookup(g_hsl_vpn_list, (void *)&stVpnInfo);
      //if(ptListNode == NULL)
      //{
           //return rc;  /*不存在，直接返回成功*/
      //}

     //ptVpnInfo = (hsl_bcm_vpn_info *)ptListNode->data;	
     HSL_CHECK_RESULT(rc, bcm_multicast_create(BCM_UNIT, BCM_MULTICAST_TYPE_VPLS, &bcast_group));
	  
	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mulitcast_group_create: bcast_group = %d\n",bcast_group);//test @zlh

     bcm_l2_addr_t_init(&l2_addr, pMcastGrpCfg->mc_mac, ptVpnInfo->bcm_vpn_id);

     l2_addr.flags = BCM_L2_STATIC | BCM_L2_MCAST;
     l2_addr.l2mc_index = bcast_group;

    HSL_CHECK_RESULT(rc, bcm_l2_addr_add(BCM_UNIT, &l2_addr));
	
    pVpnMcastInfo = &ptVpnInfo->stVpnMcastGrp[pMcastGrpCfg->hsl_mc_group_id];

    memcpy(pVpnMcastInfo, pMcastGrpCfg, sizeof(hsl_bcm_vpn_mcast_grp_cfg));

    pVpnMcastInfo->bcm_mcast_id = bcast_group;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mulitcast_group_create: bcm_mcast_id = %d\n",pVpnMcastInfo->bcm_mcast_id);//test @zlh
	
    return rc;
}

int hsl_bcm_mulitcast_group_del(hsl_bcm_vpn_mcast_grp_cfg *pMcastGrpCfg)
{
    int rc = 0;
    hsl_bcm_vpn_info  *ptVpnInfo = NULL;
    hsl_bcm_vpn_mcast_grp_cfg *pVpnMcastInfo = NULL;
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mulitcast_group_del: Z L H test successful......!! \n");//test @zlh

    HSL_CHECK_L2VPN_ID(rc, pMcastGrpCfg->hsl_vpn_id);
	
    ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[pMcastGrpCfg->hsl_vpn_id]);
	
    if(ptVpnInfo->hsl_vpn_cfg != HSL_CFG_YES)
    {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                         "hsl_bcm_mulitcast_group_del: vpn id = %d is not create \n", pMcastGrpCfg->hsl_vpn_id);
        return -1;
    }

    pVpnMcastInfo = &ptVpnInfo->stVpnMcastGrp[pMcastGrpCfg->hsl_mc_group_id];
    bcm_l2_addr_delete(BCM_UNIT, pVpnMcastInfo->mc_mac,  ptVpnInfo->bcm_vpn_id);

    if(pVpnMcastInfo->bcm_mcast_id > 0)
    {
        HSL_CHECK_RESULT(rc, bcm_multicast_destroy(BCM_UNIT, pVpnMcastInfo->bcm_mcast_id));
    }

    return rc;
}

/*zlh@12.06,copy form hsl-1307*/
int hsl_bcm_mulitcast_group_port_set(hsl_bcm_vpn_mcast_port_cfg *ptVpnMcastPortCfg, int Optian)
{
      int rc = 0;
      hsl_bcm_vpn_info  *ptVpnInfo = NULL;
      hsl_bcm_vpn_info   stVpnInfo = {0};
      T_LIST_NODE *ptListNode = NULL;	 
      hsl_bcm_vpn_mcast_grp_cfg *pVpnMcastInfo = NULL;
      bcm_gport_t  mpls_port_id;
      bcm_if_t  bcm_encap_id;
      bcm_gport_t bcm_gport;
	 
      stVpnInfo.hsl_vpn_id= ptVpnMcastPortCfg->hsl_vpn_id;
	  
	  ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[stVpnInfo.hsl_vpn_id]);

	  /*
      ptListNode = linkListLookup(g_hsl_vpn_list, (void *)&stVpnInfo);
      if(ptListNode == NULL)
      {
            return rc;  
      }

      ptVpnInfo = (hsl_bcm_vpn_info *)ptListNode->data;	
	   */

      pVpnMcastInfo = &ptVpnInfo->stVpnMcastGrp[ptVpnMcastPortCfg->hsl_mc_group_id];

	  if(ptVpnInfo->hsl_vpn_cfg != HSL_CFG_YES)
      {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                         "hsl_bcm_mulitcast_group_port_set: vpn id = %d is not create \n", ptVpnMcastPortCfg->hsl_vpn_id);
        return -1;
      }
	
	 if(ptVpnMcastPortCfg->svp_type == 0)
     {
     	hsl_bcm_ac_port_info stHslVpnAcCfg;
	 	hsl_bcm_ac_port_info *ptHslAvPortInfo = NULL;
	  
	  	memset(&stHslVpnAcCfg, 0, sizeof(hsl_bcm_ac_port_info));
	  	stHslVpnAcCfg.ac_port = ptVpnMcastPortCfg->ac_port;
	  	stHslVpnAcCfg.ac_vlan = ptVpnMcastPortCfg->ac_vlan;
		
        ptListNode = linkListLookup(ptVpnInfo->hsl_ac_port_list, (void *)&stHslVpnAcCfg);
		
        if(ptListNode == NULL)
        {
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
		  	"hsl_bcm_mulitcast_group_port_set:  port = %d  vlan =%d  is not create \n",
		  												stHslVpnAcCfg.ac_port, stHslVpnAcCfg.ac_vlan);
               return -1;
         }

         ptHslAvPortInfo = (hsl_bcm_ac_port_info *)ptListNode->data;
         HSL_CHECK_RESULT(rc, bcm_port_gport_get (BCM_UNIT, ptVpnMcastPortCfg->ac_port, &bcm_gport));
	  	 mpls_port_id = ptHslAvPortInfo->mpls_port_id;

     }
     else
     {
         hsl_bcm_pw_port_info *ptHslPwPortInfo = NULL;
         hsl_bcm_pw_port_info   stHslPwPortInfo;

	  	 memset(&stHslPwPortInfo, 0, sizeof(hsl_bcm_ac_port_info));
	 	 stHslPwPortInfo.out_port = ptVpnMcastPortCfg->pw_in_port;
	  	 stHslPwPortInfo.match_pw_label = ptVpnMcastPortCfg->match_pw_label;
		 
         ptListNode = linkListLookup(ptVpnInfo->hsl_pw_port_list, (void *)&stHslPwPortInfo);
         if(ptListNode == NULL)
         {
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
		  	"hsl_bcm_mulitcast_group_port_set:  port = %d  pw_label =%d  is not create \n",
		  									stHslPwPortInfo.out_port, stHslPwPortInfo.match_pw_label);
                return -1;
          }

       	 ptHslPwPortInfo = (hsl_bcm_pw_port_info *)ptListNode->data;
		 
	     HSL_CHECK_RESULT(rc, bcm_port_gport_get (BCM_UNIT, ptVpnMcastPortCfg->pw_in_port, &bcm_gport));
		 
	     mpls_port_id = ptHslPwPortInfo->mpls_port_id;
     }

     HSL_CHECK_RESULT(rc, bcm_multicast_vpls_encap_get(BCM_UNIT,
	 						ptVpnInfo->bcast_group_id, bcm_gport, mpls_port_id, &bcm_encap_id));

	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
		  	"hsl_bcm_mulitcast_group_port_set:  pVpnMcastInfo->bcm_mcast_id = %d  bcm_gport =%d  bcm_encap_id =%d\n",
		  																pVpnMcastInfo->bcm_mcast_id, bcm_gport,bcm_encap_id);
	 
    if(Optian == 1)
    {	
         HSL_CHECK_RESULT(rc,bcm_multicast_egress_add(BCM_UNIT, 
		  					pVpnMcastInfo->bcm_mcast_id, bcm_gport, bcm_encap_id));
    }
    else
    {
         
	 	HSL_CHECK_RESULT(rc,bcm_multicast_egress_delete(BCM_UNIT, 
	 						pVpnMcastInfo->bcm_mcast_id, bcm_gport, bcm_encap_id));

     }

    return rc;
}

int hsl_bcm_vpn_static_macaddr_set(hsl_bcm_static_mac_cfg *ptBcmMacCfg, int Optian)
{
     int rc = 0;
     hsl_bcm_vpn_info  *ptVpnInfo = NULL;
     hsl_bcm_vpn_info   stVpnInfo = {0};
     T_LIST_NODE *ptListNode = NULL;	 
     hsl_bcm_vpn_mcast_grp_cfg *pVpnMcastInfo = NULL;
     bcm_gport_t  mpls_port_id;
     bcm_l2_addr_t   l2_addr;

     stVpnInfo.hsl_vpn_id = ptBcmMacCfg->hsl_vpn_id;

	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
           	 	"hsl_bcm_vpn_static_macaddr_set: hsl_vpn_id = %d\n",stVpnInfo.hsl_vpn_id);
	 HSL_CHECK_L2VPN_ID(rc, ptBcmMacCfg->hsl_vpn_id);
	 ptVpnInfo = (hsl_bcm_vpn_info *)(&g_hsl_vpn_list[stVpnInfo.hsl_vpn_id]);
	 
	 /*
	 ptListNode = linkListLookup(g_hsl_vpn_list, (void *)&stVpnInfo);
      if(ptListNode == NULL)
      {
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                "hsl_bcm_vpn_static_macaddr_set: vpn = %d not create \n", ptBcmMacCfg->hsl_vpn_id);
             
              return -1;
      }

      ptVpnInfo = (hsl_bcm_vpn_info *)ptListNode->data;	
	*/
	
     if(Optian == 1)
     {
        if(ptBcmMacCfg->svp_type == 0)
        {
            hsl_bcm_ac_port_info stHslVpnAcCfg;
            hsl_bcm_ac_port_info *ptHslAvPortInfo = NULL;
            memset(&stHslVpnAcCfg, 0, sizeof(hsl_bcm_ac_port_info));
            stHslVpnAcCfg.ac_port = ptBcmMacCfg->port;
            stHslVpnAcCfg.ac_vlan = ptBcmMacCfg->ac_vlan;
			
            ptListNode = linkListLookup(ptVpnInfo->hsl_ac_port_list, (void *)&stHslVpnAcCfg);
			
            if(ptListNode == NULL)
            {
            	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
           	 	"hsl_bcm_vpn_static_macaddr_set:  port = %d  vlan =%d  is not create \n",stHslVpnAcCfg.ac_port, stHslVpnAcCfg.ac_vlan);
            	return -1;
            }

            ptHslAvPortInfo = (hsl_bcm_ac_port_info *)ptListNode->data;
            mpls_port_id = ptHslAvPortInfo->mpls_port_id;
        }
        else
        {
            hsl_bcm_pw_port_info *ptHslPwPortInfo = NULL;
            hsl_bcm_pw_port_info   stHslPwPortInfo;

            memset(&stHslPwPortInfo, 0, sizeof(hsl_bcm_ac_port_info));
            stHslPwPortInfo.out_port = ptBcmMacCfg->port;
            stHslPwPortInfo.match_pw_label = ptBcmMacCfg->pw_label;	  
            ptListNode = linkListLookup(ptVpnInfo->hsl_pw_port_list, (void *)&stHslPwPortInfo);
            if(ptListNode == NULL)
            {
                HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
                "hsl_bcm_vpn_static_macaddr_set:  port = %d  pw_label =%d  is not create \n",stHslPwPortInfo.out_port, stHslPwPortInfo.match_pw_label);
                return -1;
            }

            ptHslPwPortInfo = (hsl_bcm_pw_port_info *)ptListNode->data;
            mpls_port_id = ptHslPwPortInfo->mpls_port_id;
        }

        bcm_l2_addr_t_init(&l2_addr, ptBcmMacCfg->mac_addr, ptVpnInfo->bcm_vpn_id);

        l2_addr.port = mpls_port_id;
        l2_addr.flags = BCM_L2_STATIC;

        if(ptBcmMacCfg->src_drop == 1)
        {
             l2_addr.flags |= BCM_L2_DISCARD_SRC;
        }

        if(ptBcmMacCfg->dst_drop == 1)
        {
             l2_addr.flags |= BCM_L2_DISCARD_DST;
        }

        HSL_CHECK_RESULT(rc, bcm_l2_addr_add(BCM_UNIT, &l2_addr));  
     }
     else
     {
             HSL_CHECK_RESULT(rc,bcm_l2_addr_delete(BCM_UNIT,ptBcmMacCfg->mac_addr,  ptVpnInfo->bcm_vpn_id));
     }

      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,
		  	"hsl_bcm_vpn_static_macaddr_set: vpn id = 0x%X,  mpls port id=0x%x, Op=%x, macaddr=%2x:%2x:%2x:%2x:%2x:%2x\n",
	                           ptVpnInfo->bcm_vpn_id,
	                           mpls_port_id,
	                           Optian,
	                           ptBcmMacCfg->mac_addr[0],ptBcmMacCfg->mac_addr[1],ptBcmMacCfg->mac_addr[2],
	                           ptBcmMacCfg->mac_addr[3],ptBcmMacCfg->mac_addr[4],ptBcmMacCfg->mac_addr[5]
	                           );

      return rc;
}


hsl_lsp_info* hsl_mpls_get_lsp_info(uint32 lsp_id)
{
    hsl_lsp_info *lsp_info = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_get_lsp_info: lsp_id = %d \n",lsp_id);//test @zlh
       
    if(lsp_id >= HSL_MPLS_LSP_MAX) 
    {
        return NULL;
    }
    
    lsp_info = &g_hsl_lsp_info[lsp_id];
    if(lsp_info->lsp_cfg == HSL_CFG_YES)
    {
        return lsp_info;
    }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_get_lsp_info_1: Z L H test successful......!! \n");//test @zlh

    return NULL;
}

unsigned int hsl_bcm_mpls_tunnel_cmp(void *pdata1,	void *pdata2)
{
    hsl_bcm_mpls_tunnel_switch_cfg *ptMplsTunnelInfo1 = NULL;
    hsl_bcm_mpls_tunnel_switch_cfg *ptMplsTunnelInfo2 = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_cmp: Z L H test successful......!! \n");//test @zlh
    if((pdata1 ==NULL) || (pdata2 == NULL))
    {
        return Link_List_Compare_Mismatch;
    }

    ptMplsTunnelInfo1 = (hsl_bcm_mpls_tunnel_switch_cfg *)pdata1;
    ptMplsTunnelInfo2 = (hsl_bcm_mpls_tunnel_switch_cfg *)pdata2;

    if(ptMplsTunnelInfo1->in_label == ptMplsTunnelInfo2->in_label)
    {
        return Link_List_Compare_Match;
    }

    return Link_List_Compare_Mismatch; 
}

/* add from hsl-svn @zlh0918 */
/*增加mpls tunnel， 两种，pop or switch , php暂时不支持*/
int hsl_bcm_mpls_tunnel_add(hsl_bcm_mpls_tunnel_switch_cfg_1307 *label_switch_info)
{
    bcm_mpls_tunnel_switch_t bcm_label_info = {0};
    hsl_bcm_mpls_tunnel_egress_cfg bcm_mpls_egr_info;
    hsl_bcm_mpls_tunnel_switch_cfg_1307 *ptMplsTunnelInfo = NULL;
    T_LIST_NODE *ptListNode = NULL;
	hsl_lsp_info   *lsp_info = NULL;
	bcm_port_t    bcm_out_port;
    bcm_port_t    bcm_in_port;
    bcm_gport_t  bcm_gport;
    bcm_pbmp_t  port_bmap_untag;
    bcm_pbmp_t  port_bmap_tag;	
	uint32 mpls_entry_index;
	mpls_entry_entry_t ment;
	uint32 hn_index;
    int rc = 0;
	ial_lsp_type  lsp_egr_type;//@zlh1017
	int egr_index;
	
  	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add: Z L H test successful......!! \n");//test @zlh
  	
  	HSL_CHECK_PTR(rc, label_switch_info);
    HSL_CHECK_LSP_ID(rc, label_switch_info->lsp_id);
	
  	lsp_info = &g_hsl_lsp_info[label_switch_info->lsp_id];
	
	bcm_out_port = label_switch_info->out_port;
	bcm_in_port = label_switch_info->in_port;
   
	if(((label_switch_info->lsp_type != IAL_LSP_PE_DOWN) && (lsp_info->lsp_egress_info.hsl_cfg == HSL_CFG_YES))
					|| ((label_switch_info->lsp_type != IAL_LSP_PE_UP) && (lsp_info->lsp_ingress_info.hsl_cfg == HSL_CFG_YES)))
	{
		return DRV_E_LSP_EXIST;
	}
				
	bcm_label_info.inner_label = 0;
	bcm_label_info.exp_map = 0;
	bcm_label_info.int_pri = 0;
	bcm_label_info.vpn = 0;
		 
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_1: Z L H test successful......!! \n");//test @zlh
			/*设备默认使用lsp exp 映射到内部优先级*/
	bcm_label_info.flags |= BCM_MPLS_SWITCH_INT_PRI_MAP; 
	bcm_label_info.egress_label.flags = BCM_MPLS_EGRESS_LABEL_TTL_DECREMENT 
													| BCM_MPLS_EGRESS_LABEL_EXP_REMARK 
													| BCM_MPLS_EGRESS_LABEL_PRI_REMARK;


	
  	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add: label_switch_info->lsp_type = %d \n",label_switch_info->lsp_type);//test @zlh
  	
	if(label_switch_info->lsp_type == IAL_LSP_P_SWITCH)
	{
		lsp_info->lsp_node_type = HSL_LSP_NODE_TYPE_P;
		
	}
	else
	{
		lsp_info->lsp_node_type = HSL_LSP_NODE_TYPE_PE;
		
	}
			 
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_2: lsp_node_type = %d\n",lsp_info->lsp_node_type);//test @zlh
	   
  	
    bcm_mpls_tunnel_switch_t_init(&bcm_label_info);
	
  	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_3: Z L H test successful......!! \n");//test @zlh

	HSL_CHECK_RESULT(rc, bcm_port_gport_get (0, label_switch_info->in_port, &bcm_gport));
   	
  	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_4: in_port                          = %d\n",label_switch_info->in_port);//test @zlh
  	
  	
  	if(label_switch_info->in_label <= 10240)
		{
			bcm_label_info.port = BCM_GPORT_INVALID;
		}
		else
		{
			bcm_label_info.port = bcm_gport;
		}
	 
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_5: bcm_label_info.port              = %d\n",bcm_label_info.port);//test @zlh

	/*隧道标签报文统计*/
    bcm_label_info.flags = BCM_MPLS_SWITCH_COUNTED;

    bcm_label_info.label = label_switch_info->in_label;
	
    bcm_label_info.egress_label.label = label_switch_info->out_label;
    bcm_label_info.egress_label.exp = label_switch_info->out_lsp_exp;
    bcm_label_info.egress_label.ttl = 0xff;
    bcm_label_info.mtu = 0x3fff;
	
    bcm_label_info.egress_if = 0;

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_6: label_switch_info->lsp_type       = %d\n",label_switch_info->lsp_type);//test @zlh
   switch(label_switch_info->lsp_type)
	{
		case IAL_LSP_PE_UP:
        {
	     	bcm_label_info.action = BCM_MPLS_SWITCH_ACTION_POP;//pe node is pop
	     	break;
		 }
		case IAL_LSP_PE_DOWN:
		{
			 bcm_label_info.action = BCM_MPLS_SWITCH_ACTION_POP;
			 break;
		}
		
        case IAL_LSP_P_SWITCH:
        {
	     	bcm_label_info.action = BCM_MPLS_SWITCH_ACTION_SWAP;
	     	break;
        }
		
	 default:

	HSL_LOG (HSL_LOG_GENERAL, HSL_LEVEL_INFO, 
				   "\n\rErr: lsp_type %d", label_switch_info->lsp_type);
            return -1;
    }
	

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	 	"hsl_bcm_mpls_station_tcam_add_7 : mpls_in_vlan=%d, mac=%x:%x:%x:%x:%x:%x\n\r",
	 	 label_switch_info->mpls_in_vlan, 
	 	 g_PortSrcMac[label_switch_info->in_port].mac_addr[0],
	 	 g_PortSrcMac[label_switch_info->in_port].mac_addr[1],
	 	 g_PortSrcMac[label_switch_info->in_port].mac_addr[2],
	 	 g_PortSrcMac[label_switch_info->in_port].mac_addr[3],
	 	 g_PortSrcMac[label_switch_info->in_port].mac_addr[4],
	 	 g_PortSrcMac[label_switch_info->in_port].mac_addr[5]
	 	 );//test @zlh
	 	 
	if(label_switch_info->lsp_type != IAL_LSP_PE_UP)
	{
		HSL_CHECK_RESULT(rc, hsl_bcm_mpls_station_tcam_add(g_PortSrcMac[label_switch_info->in_port].mac_addr, label_switch_info->mpls_in_vlan));
	}
	
    memset(&port_bmap_untag, 0, sizeof(bcm_pbmp_t));
    memset(&port_bmap_tag, 0, sizeof(bcm_pbmp_t));

    BCM_PBMP_PORT_SET(port_bmap_tag, label_switch_info->in_port);
    if(label_switch_info->mpls_in_vlan == HSL_VLAN_INVALID)
    {
        BCM_PBMP_PORT_SET(port_bmap_untag, label_switch_info->in_port);
      //  HSL_CHECK_RESULT(rc, hsl_bcm_l2_switch_disable(1, 1));
        HSL_CHECK_RESULT(rc, bcm_vlan_port_add(BCM_UNIT, 1, port_bmap_tag, port_bmap_untag));
	
    }
    else
    {
          bcm_vlan_create(BCM_UNIT, label_switch_info->mpls_in_vlan);
          HSL_CHECK_RESULT(rc, hsl_bcm_l2_switch_disable(label_switch_info->mpls_in_vlan, 1));
          HSL_CHECK_RESULT(rc, bcm_vlan_port_add(BCM_UNIT, label_switch_info->mpls_in_vlan, port_bmap_tag, port_bmap_untag));
    }
   
   
    bcm_label_info.inner_label = 0;
    bcm_label_info.exp_map = 0;
    bcm_label_info.int_pri = 0;
    bcm_label_info.vpn = 0;

   /*过环、上环隧道需要配置下一跳*/
   //if(label_switch_info->label_action != HSL_MPLS_LABEL_POP)
   if(label_switch_info->lsp_type != IAL_LSP_PE_DOWN)
   {
       sal_memset(&bcm_mpls_egr_info, 0, sizeof(hsl_bcm_mpls_tunnel_egress_cfg));
       sal_memcpy(bcm_mpls_egr_info.des_mac, label_switch_info->des_mac, sizeof(bcm_mac_t));
       bcm_mpls_egr_info.l3_vid = label_switch_info->mpls_out_vlan;
       bcm_mpls_egr_info.out_port = label_switch_info->out_port;
	   bcm_mpls_egr_info.out_lsp_label = label_switch_info->out_label;// add @zlh0925
	   bcm_mpls_egr_info.out_lsp_exp = label_switch_info->out_lsp_exp;// add @zlh0925
	   //bcm_mpls_egr_info.lsp_type = label_switch_info->lsp_type;//add @zlh 传递lsp_type
	   lsp_egr_type = label_switch_info->lsp_type;//@zlh1017
	   
	   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_8:mpls_out_vlan        = %d \n",bcm_mpls_egr_info.l3_vid );//test @zlh
	   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_9:out_port             = %d \n",label_switch_info->out_port);//test @zlh
	   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_10:out_lsp_label       = %d \n",label_switch_info->out_label);//test @zlh
	   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_11:out_lsp_exp         = %d \n",label_switch_info->out_lsp_exp);//test @zlh
	   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_12:lsp_egr_type        = %d \n",lsp_egr_type);//test @zlh

	   HSL_CHECK_RESULT(rc,  hsl_mpls_tunnel_egress_add(&bcm_mpls_egr_info,lsp_egr_type));//@zlh1017 add lsp_egr_type
	   
       bcm_label_info.egress_if = bcm_mpls_egr_info.egr_if;
	   
	/* 通过egress_tunnel_if 产生 lsp的next hop index */
	HSL_CHECK_RESULT(rc,bcm_xgs3_get_nh_from_egress_object(BCM_UNIT, bcm_mpls_egr_info.egr_if, 0, 0, &egr_index));//zlh@150114
	 	
   }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_13: egress_if              = %d /n",bcm_label_info.egress_if);//test @zlh

	if(label_switch_info->out_lsp_exp == 8) 
	{
		bcm_label_info.egress_label.exp = 0;
	}
	
  /*设备默认使用lsp exp 映射到内部优先级*/
    bcm_label_info.flags |= BCM_MPLS_SWITCH_INT_PRI_MAP; 
    bcm_label_info.egress_label.flags = BCM_MPLS_EGRESS_LABEL_TTL_DECREMENT 
                                                        | BCM_MPLS_EGRESS_LABEL_EXP_REMARK 
                                                        | BCM_MPLS_EGRESS_LABEL_PRI_REMARK;
    

   HSL_CHECK_RESULT(rc, bcm_mpls_tunnel_switch_add(BCM_UNIT,  &bcm_label_info));//通过bcm_mpls_tunnel_switch_add 来建立mpls_entry
  
   if(lsp_info->lsp_cfg == HSL_CFG_NO)
	{
		memset(lsp_info, 0, sizeof(hsl_lsp_info));
	}
		  
	lsp_info->tunnel_id = label_switch_info->lsp_id;
	
	if(label_switch_info->lsp_type != IAL_LSP_PE_UP)
	{
		/* 实现将转换号的信息放入g_hsl_lsp_info*/
		
		lsp_info->lsp_ingress_info.hsl_cfg   = HSL_CFG_YES;
		lsp_info->lsp_ingress_info.in_port   = bcm_in_port;
		lsp_info->lsp_ingress_info.in_label  = label_switch_info->in_label;
		lsp_info->lsp_ingress_info.in_vlan   = label_switch_info->mpls_in_vlan;
		//lsp_info->lsp_ingress_info.mpls_entry_index = mpls_entry_index;//zlh@150114
		lsp_info->lsp_ingress_info.egr_l3_next_hop = egr_index;//zlh@150114

		HSL_CHECK_RESULT(rc, hsl_eport_get_mac_addr_by_bcmport(lsp_info->lsp_ingress_info.in_port, lsp_info->lsp_ingress_info.src_mac));

		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_14: lsp_ingress_info.hsl_cfg       = % d\n",lsp_info->lsp_ingress_info.hsl_cfg );//test @zlh
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_15: lsp_egress_info.in_port        = % d\n",bcm_in_port );//test @zlh
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_16: lsp_ingress_info.in_vlan       = % d\n",label_switch_info->mpls_in_vlan);//test @zlh		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_17: egr_l3_next_hop                = % d\n",egr_index);//test @zlh		
		
	}
		  
	if(label_switch_info->lsp_type != IAL_LSP_PE_DOWN)
	{
		/* 实现将转换号的信息放入g_hsl_lsp_info*/
		lsp_info->lsp_egress_info.hsl_cfg  = HSL_CFG_YES;
		lsp_info->lsp_egress_info.out_port = bcm_out_port;
		lsp_info->lsp_egress_info.out_label	= label_switch_info->out_label;
		lsp_info->lsp_egress_info.out_label_exp = label_switch_info->out_lsp_exp;
		lsp_info->lsp_egress_info.out_vlan = label_switch_info->mpls_out_vlan;  
		lsp_info->lsp_egress_info.egr_intf = bcm_mpls_egr_info.egr_if;
		lsp_info->lsp_egress_info.egr_mac_da_index = bcm_mpls_egr_info.l3_if;
		lsp_info->lsp_egress_info.l3_intf = bcm_mpls_egr_info.l3_if;
		lsp_info->lsp_egress_info.egr_l3_intf_index = bcm_mpls_egr_info.l3_if;
		memcpy(lsp_info->lsp_egress_info.des_mac, label_switch_info->des_mac, 6);
		
		}
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_17: lsp_egress_info.hsl_cfg            = % d\n",lsp_info->lsp_egress_info.hsl_cfg );//test @zlh		
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_18: lsp_egress_info.out_port           = % d\n",bcm_out_port);//test @zlh		
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_19: lsp_egress_info.out_label          = % d\n",label_switch_info->out_label);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_20: lsp_egress_info.out_label_exp      = % d\n",label_switch_info->out_lsp_exp);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_21: lsp_egress_info.out_vlan           = % d\n",label_switch_info->mpls_out_vlan);//test @zlh
	
	lsp_info->lsp_ingress_info.ing_phb_id = IAL_QOS_PHB_ID_DEFAULT;
	lsp_info->lsp_egress_info.egr_phb_id = IAL_QOS_PHB_ID_DEFAULT;
	
	lsp_info->lsp_egress_info.bind_pw_list = linkContainerNew(LINKLIST_NEED_SEM);   

	if(lsp_info->lsp_egress_info.bind_pw_list == NULL)
	{
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_lsp_add: create bind pw list err \n");
	}
		  
	lsp_info->lsp_cfg = HSL_CFG_YES;
	lsp_info->lsp_egress_info.bind_pw_list ->cmp = hsl_bcm_vpn_pw_port_cmd;
	lsp_info->lsp_egress_info.bind_pw_list ->del	  = NULL; //pw 信息，只在vpn 端口删除的时候释放
	 
	show_lsp_info(label_switch_info->lsp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_add_23: lsp_id                             = %d\n",label_switch_info->lsp_id);//test @
   return rc;
}

int hsl_bcm_mpls_tunnel_del(hsl_bcm_mpls_tunnel_switch_cfg_1307 *label_switch_info)
{
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del: Z L H test successful......!! \n");//test @zlh

    int rc = 0;
    bcm_mpls_tunnel_switch_t bcm_label_info = {0};
    hsl_bcm_mpls_tunnel_egress_cfg bcm_mpls_egr_info;
    hsl_bcm_mpls_tunnel_switch_cfg_1307 *ptMplsTunnelInfo = NULL;
    T_LIST_NODE *ptListNode = NULL;
    bcm_gport_t  bcm_gport;
	hsl_lsp_info   *hsl_lsp_info = NULL;
	
	HSL_CHECK_PTR(rc, label_switch_info);
    HSL_CHECK_LSP_ID(rc, label_switch_info->lsp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del:  label_switch_info->lsp_id = %d, \n", label_switch_info->lsp_id);//test @zlh
	
    hsl_lsp_info = hsl_mpls_get_lsp_info(label_switch_info->lsp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_1:  label_switch_info->lsp_id = %d, \n", label_switch_info->lsp_id);//test @zlh
    if(hsl_lsp_info == NULL)
    {
        return DRV_E_LSP_NO_EXIST;
    }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_2:  label_switch_info->lsp_id = %d, \n", label_switch_info->lsp_id);//test @zlh

    if(((hsl_lsp_info->lsp_ingress_info.hsl_cfg == HSL_CFG_NO) && (label_switch_info->lsp_type == IAL_LSP_PE_DOWN))
        || ((hsl_lsp_info->lsp_egress_info.hsl_cfg == HSL_CFG_NO)&&(label_switch_info->lsp_type == IAL_LSP_PE_UP)))
    {
        return DRV_E_LSP_NO_EXIST;
    }
 	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_3:  hsl_lsp_info->lsp_ingress_info.in_vlan \n", hsl_lsp_info->lsp_ingress_info.in_vlan);//test @zlh
 	
	if(hsl_lsp_info->lsp_ingress_info.hsl_cfg == HSL_CFG_YES)
    {
          HSL_CHECK_RESULT(rc, hsl_bcm_mpls_station_tcam_del(g_PortSrcMac[hsl_lsp_info->lsp_ingress_info.in_port].mac_addr, hsl_lsp_info->lsp_ingress_info.in_vlan));
		  
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_4: hsl_lsp_info->lsp_ingress_info.in_vlan = %d\n", hsl_lsp_info->lsp_ingress_info.in_vlan);//test @zlh

         bcm_mpls_tunnel_switch_t_init(&bcm_label_info);

        HSL_CHECK_RESULT(rc, bcm_port_gport_get (0, hsl_lsp_info->lsp_ingress_info.in_port, &bcm_gport));
		
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_5: hsl_lsp_info->lsp_ingress_info.in_port = %d\n",hsl_lsp_info->lsp_ingress_info.in_port);//test @zlh
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_5: bcm_gport = %d\n",bcm_gport);//test @zlh
	
         
        if(hsl_lsp_info->lsp_ingress_info.in_label<= 10240)
        {
            bcm_label_info.port = BCM_GPORT_INVALID;
        }
        else
        {
             bcm_label_info.port = bcm_gport;
        }
        
        
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_6:  bcm_label_info.port = %d\n", bcm_label_info.port);//test @zlh
         bcm_label_info.label = hsl_lsp_info->lsp_ingress_info.in_label;
         
         HSL_CHECK_RESULT(rc, bcm_mpls_tunnel_switch_delete(BCM_UNIT,  &bcm_label_info));
		 
		 memset(&hsl_lsp_info->lsp_ingress_info, 0, sizeof(hsl_lsp_ingress_info));//@zlh1017
		 
		}
		 
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_7: Z L H test successful......!! \n");//test @zlh
		 

      
       if(hsl_lsp_info->lsp_egress_info.hsl_cfg == HSL_CFG_YES)
       {
			sal_memset(&bcm_mpls_egr_info, 0, sizeof(hsl_bcm_mpls_tunnel_egress_cfg));
	        sal_memcpy(bcm_mpls_egr_info.des_mac, hsl_lsp_info->lsp_egress_info.des_mac, sizeof(bcm_mac_t));
	        bcm_mpls_egr_info.l3_vid = hsl_lsp_info->lsp_egress_info.out_vlan;
	        bcm_mpls_egr_info.out_port = hsl_lsp_info->lsp_egress_info.out_port;
	        bcm_mpls_egr_info.out_lsp_label = hsl_lsp_info->lsp_egress_info.out_label;
			
			HSL_CHECK_RESULT(rc,  hsl_mpls_tunnel_egress_del(&bcm_mpls_egr_info));
			
        	memset(&hsl_lsp_info->lsp_egress_info, 0, sizeof(hsl_lsp_engress_info));

		}
	   
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_8:bcm_mpls_egr_info.l3_vid = %d !! \n",bcm_mpls_egr_info.l3_vid);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_9:bcm_mpls_egr_info.out_port = %d !! \n",bcm_mpls_egr_info.out_port);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_10:lsp_ingress_info.hsl_cfg= %d !! \n",hsl_lsp_info->lsp_ingress_info.hsl_cfg);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_11:lsp_egress_info.hsl_cfg = %d !! \n",hsl_lsp_info->lsp_egress_info.hsl_cfg);//test @zlh

	 if((hsl_lsp_info->lsp_ingress_info.hsl_cfg == HSL_CFG_NO) 
        && (hsl_lsp_info->lsp_egress_info.hsl_cfg == HSL_CFG_NO))
    {
    	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_12:bcm_mpls_egr_info.hsl_lsp_info->lsp_cfg = %d !! \n",hsl_lsp_info->lsp_cfg);//test @zlh

		hsl_lsp_info->lsp_cfg = HSL_CFG_NO;
        memset(hsl_lsp_info, 0, sizeof(hsl_lsp_info));
    }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_del_13:bcm_mpls_egr_info.hsl_lsp_info->lsp_cfg = %d !! \n",hsl_lsp_info->lsp_cfg);//test @zlh

    return rc;
   }

//int hsl_mpls_tunnel_egress_add(hsl_bcm_mpls_tunnel_egress_cfg *tn_egr_cfg)
int hsl_mpls_tunnel_egress_add(hsl_bcm_mpls_tunnel_egress_cfg *tn_egr_cfg, ial_lsp_type  lsp_egr_type)//@zlh1017

{
    hsl_bcm_mpls_tunnel_l3if_cfg l3if_cfg;
    hsl_bcm_mpls_tunnel_egress_cfg *ptl3greinfo = NULL;
    T_LIST_NODE *pL3EgrNode = NULL;
    bcm_pbmp_t  port_bmap_untag ;
    bcm_pbmp_t  port_bmap_tag;
    bcm_l3_egress_t l3_egress;
    bcm_if_t egress_tunnel_if = -1;
    int rc = 0;
	ial_lsp_type  lsp_l3_type;//@zlh1017
	lsp_l3_type = lsp_egr_type;//@zlh1017
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_1: lsp_l3_type             = %d\n",lsp_l3_type);//test @zlh

	
      memset(&l3if_cfg, 0, sizeof(hsl_bcm_mpls_tunnel_l3if_cfg));
      sal_memcpy(l3if_cfg.mac, g_PortSrcMac[tn_egr_cfg->out_port].mac_addr, sizeof(bcm_mac_t));
      l3if_cfg.vlanid = tn_egr_cfg->l3_vid;
      l3if_cfg.out_lsp_label = tn_egr_cfg->out_lsp_label;
      l3if_cfg.out_lsp_exp = tn_egr_cfg->out_lsp_exp;
	  
	  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_3: l3if_cfg.vlanid           = %d \n",l3if_cfg.vlanid);//test @zlh
	  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_4: l3if_cfg.out_lsp_exp      = %d \n",l3if_cfg.out_lsp_exp);//test @zlh
	  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_5: l3if_cfg.out_lsp_label    = %d \n",l3if_cfg.out_lsp_label);//test @zlh
	  
	
      HSL_CHECK_RESULT(rc,hsl_bcm_mpls_tunnel_l3if_add(&l3if_cfg, lsp_l3_type));
      tn_egr_cfg->l3_if = l3if_cfg.l3_if;

      pL3EgrNode = linkListLookup(g_hsl_l3_egr_list, (void *)tn_egr_cfg);
     if(pL3EgrNode == NULL)
     {
         bcm_l3_egress_t_init(&l3_egress);//初始化
         l3_egress.flags = 0;
         l3_egress.module = 0;
         l3_egress.port = tn_egr_cfg->out_port;
         l3_egress.trunk = 0;
         l3_egress.intf = l3if_cfg.l3_if;
         l3_egress.vlan = 0;
	
		 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_6: Z L H test successful......!! \n");//test @zlh
		 
         sal_memcpy(l3_egress.mac_addr,  tn_egr_cfg->des_mac, sizeof(bcm_mac_t));
         HSL_CHECK_RESULT(rc, bcm_l3_egress_create(BCM_UNIT,0, &l3_egress, &egress_tunnel_if));//通过调用bcm_l3_egress_create 来产生egress_tunnel_if;

        memset(&port_bmap_untag, 0, sizeof(bcm_pbmp_t));
        memset(&port_bmap_tag, 0, sizeof(bcm_pbmp_t));
		
		BCM_PBMP_PORT_SET(port_bmap_tag, tn_egr_cfg->out_port);
		if(tn_egr_cfg->l3_vid == HSL_VLAN_INVALID)
		{
            BCM_PBMP_PORT_SET(port_bmap_untag, tn_egr_cfg->out_port);
	    tn_egr_cfg->l3_vid = 1;
		}
	
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_7: Z L H test successful......!! \n");//test @zlh
	
		HSL_CHECK_RESULT(rc, bcm_vlan_port_add(BCM_UNIT, tn_egr_cfg->l3_vid, port_bmap_tag, port_bmap_untag));

		 ptl3greinfo = sal_alloc(sizeof(hsl_bcm_mpls_tunnel_l3if_cfg), "l3_egr_list");
		 if(ptl3greinfo == NULL)
	 	{
	       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add: malloc err\n");
	       return -1;
	 	}

	 	sal_memset(ptl3greinfo, 0, sizeof(hsl_bcm_mpls_tunnel_l3if_cfg));
	 
		 ptl3greinfo->cnt = 1;
     	sal_memcpy(ptl3greinfo->des_mac, tn_egr_cfg->des_mac, sizeof(bcm_mac_t));
     	ptl3greinfo->egr_if = egress_tunnel_if;
     	ptl3greinfo->l3_if = l3if_cfg.l3_if;
	 	ptl3greinfo->l3_vid = tn_egr_cfg->l3_vid;
	 	ptl3greinfo->out_port = tn_egr_cfg->out_port;
		
	 	linkListAddAtTail(g_hsl_l3_egr_list, (void *)ptl3greinfo); 
	 
		 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_7:cnt = %d \n",ptl3greinfo->cnt);//test @zlh
    }
    else
    {
         ptl3greinfo = (hsl_bcm_mpls_tunnel_egress_cfg *)pL3EgrNode->data;
         ptl3greinfo->cnt++;
	
    }
	
    tn_egr_cfg->egr_if = ptl3greinfo->egr_if;//获得 egr_if

    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	 "hsl_mpls_tunnel_egress_add: l3_egr_intf=%d, cnt=%d\n",tn_egr_cfg->egr_if,   ptl3greinfo->cnt);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_add_8: Z L H test successful......!! \n");//test @zlh

  return rc;
}

int hsl_mpls_tunnel_egress_del(hsl_bcm_mpls_tunnel_egress_cfg *tn_egr_cfg)
{
      hsl_bcm_mpls_tunnel_l3if_cfg l3if_cfg;
      hsl_bcm_mpls_tunnel_egress_cfg *ptl3greinfo = NULL;
      T_LIST_NODE *pL3EgrNode = NULL;
	  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del: Z L H test successful......!! \n");//test @zlh
		  
      int rc = 0;

      memset(&l3if_cfg, 0, sizeof(hsl_bcm_mpls_tunnel_l3if_cfg));
      sal_memcpy(l3if_cfg.mac, g_PortSrcMac[tn_egr_cfg->out_port].mac_addr, sizeof(bcm_mac_t));
      l3if_cfg.vlanid = tn_egr_cfg->l3_vid;
       l3if_cfg.out_lsp_label = tn_egr_cfg->out_lsp_label;
      l3if_cfg.out_lsp_exp = tn_egr_cfg->out_lsp_exp;
      HSL_CHECK_RESULT(rc,hsl_bcm_mpls_tunnel_l3if_del(&l3if_cfg)); 
	  
     tn_egr_cfg->l3_if = l3if_cfg.l3_if;
     pL3EgrNode = linkListLookup(g_hsl_l3_egr_list, (void *)tn_egr_cfg);
	 
	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del_1: Z L H test successful......!! \n");//test @zlh
	 
     if(pL3EgrNode != NULL)
     {
          ptl3greinfo = (hsl_bcm_mpls_tunnel_egress_cfg *)pL3EgrNode->data;
		  
		  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del_2:cnt = %d  \n",ptl3greinfo->cnt);//test @zlh
	 
          if(ptl3greinfo->cnt > 1)
          {
                 ptl3greinfo->cnt--;
	         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
			 	"hsl_mpls_tunnel_egress_del: l3_intf= %d, cnt = %d\n", ptl3greinfo->egr_if, ptl3greinfo->cnt);
          }
		  
		 else if(ptl3greinfo->cnt == 1)
		  {

			   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del_3:egr_if = %d	\n",ptl3greinfo->egr_if);//test @zlh
		  		
               HSL_CHECK_RESULT(rc, bcm_l3_egress_destroy(BCM_UNIT,ptl3greinfo->egr_if));
			   
			   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del_4:cnt = %d  \n",ptl3greinfo->cnt);//test @zlh

	        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
			 	"hsl_mpls_tunnel_egress_del: l3_intf= %d, cnt = %d\n", ptl3greinfo->egr_if,ptl3greinfo->cnt);
			
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del_5:cnt = %d  \n",ptl3greinfo->cnt);//test @zlh
			
               linkListRemove(g_hsl_l3_egr_list, (void *)ptl3greinfo); 
               
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del_6:cnt = %d  \n",ptl3greinfo->cnt);//test @zlh
		
	       //ptl3greinfo   = NULL;????
		   
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del_7:cnt = %d  \n",ptl3greinfo->cnt);//test @zlh
	  }
     }
     else
     {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_tunnel_egress_del: ptl3greinfo==NULL, \n\r");
           return 1;

     }

     return rc;

}

/***************************************************************
* Function: hsl_mpls_get_lsp_status
* Description: get lsp link status
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_mpls_get_lsp_status(ial_mpls_lsp_status *lsp_status)
{
    uint32 rc = DRV_SUCCESS;
    hsl_lsp_info   *hsl_lsp_info = NULL;

    HSL_CHECK_PTR(rc, lsp_status);
    HSL_CHECK_LSP_ID(rc, lsp_status->lsp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_get_lsp_status: Z L H test successful......!! \n");//test @zlh
    hsl_lsp_info = hsl_mpls_get_lsp_info(lsp_status->lsp_id);
    if(hsl_lsp_info == NULL)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    if(hsl_lsp_info->lsp_ingress_info.hsl_cfg == HSL_CFG_YES)
    {
        lsp_status->lsp_ingress_status = hsl_lsp_info->lsp_ingress_info.lsp_status;
    }
    else
    {
        lsp_status->lsp_ingress_status = IAL_LSP_STATUS_INVALID;
    }

    
    if(hsl_lsp_info->lsp_egress_info.hsl_cfg == HSL_CFG_YES)
    {
       lsp_status->lsp_egress_status = hsl_lsp_info->lsp_egress_info.lsp_status;
    }
    else
    {
       lsp_status->lsp_egress_status = IAL_LSP_STATUS_INVALID;
    }

    show_lsp_info(lsp_status->lsp_id);   

    return rc;
}


unsigned int hsl_bcm_mpls_station_tcam_cmp(void *pdata1,   void *pdata2)
{
    hsl_bcm_mpls_tcam_info *ptMplsTcamInfo1 = NULL;
    hsl_bcm_mpls_tcam_info *ptMplsTcamInfo2 = NULL;

    if((pdata1 ==NULL) || (pdata2 == NULL))
    {
        return Link_List_Compare_Mismatch;
    }

    ptMplsTcamInfo1 = (hsl_bcm_mpls_tcam_info *)pdata1;
    ptMplsTcamInfo2 = (hsl_bcm_mpls_tcam_info *)pdata2;

    if((ptMplsTcamInfo1->vlan_id == ptMplsTcamInfo2->vlan_id)
        && (sal_memcmp(ptMplsTcamInfo1->src_mac, ptMplsTcamInfo2->src_mac, 6) == 0))
    {
        return Link_List_Compare_Match;

    }

    return Link_List_Compare_Mismatch;

}


unsigned int hsl_bcm_mpls_station_tcam_add(bcm_mac_t  src_mac,  bcm_vlan_t  vlan_id)
{
      T_LIST_NODE *ptMplsTcamNode = NULL;
      hsl_bcm_mpls_tcam_info  stMplsTcanInfo ;
      hsl_bcm_mpls_tcam_info *ptMplsTcamInfo = NULL;
      int rc = 0;
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add: Z L H test successful......!! \n");//test @zlh
	
   if(vlan_id == HSL_VLAN_INVALID)//zlh@150129,just vlan invalid default vlan 1;
   {
        vlan_id = 1;
   }
   
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_1: Z L H test successful......!! \n");//test @zlh
     memset(&stMplsTcanInfo, 0, sizeof(hsl_bcm_mpls_tcam_info));
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_2: Z L H test successful......!! \n");//test @zlh
     stMplsTcanInfo.vlan_id = vlan_id;
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_3: Z L H test successful......!! \n");//test @zlh
     memcpy(stMplsTcanInfo.src_mac, src_mac, 6);
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_4: Z L H test successful......!! \n");//test @zlh
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	 	"hsl_bcm_mpls_station_tcam_add_5: vlan=%d, mac=%x:%x:%x:%x:%x:%x, cnt=%d\n\r",
	 	 stMplsTcanInfo.vlan_id, 
	 	 stMplsTcanInfo.src_mac[0],
	 	 stMplsTcanInfo.src_mac[1],
	 	 stMplsTcanInfo.src_mac[2],
	 	 stMplsTcanInfo.src_mac[3],
	 	 stMplsTcanInfo.src_mac[4],
	 	 stMplsTcanInfo.src_mac[5],
	 	 stMplsTcanInfo.cnt);////test @zlh
	
     ptMplsTcamNode = linkListLookup(g_hsl_mpls_station_tcam_list, (void *)&stMplsTcanInfo);
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_6: Z L H test successful......!! \n");//test @zlh
     if(ptMplsTcamNode == NULL)
     {

    HSL_CHECK_RESULT(rc, bcm_l2_tunnel_add(BCM_UNIT, src_mac, vlan_id));// error:invalid parameter @zlh 0922
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_7: Z L H test successful......!! \n");//test @zlh
		   
           ptMplsTcamInfo = sal_alloc(sizeof(hsl_bcm_mpls_tcam_info), "mpls_tcam_list");
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_8: Z L H test successful......!! \n");//test @zlh
	   if(ptMplsTcamInfo == NULL)
	   {
                   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add: malloc err\n");
		   return -1;
	   }

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_add_9: Z L H test successful......!! \n");//test @zlh
	   sal_memset(ptMplsTcamInfo, 0, sizeof(hsl_bcm_mpls_tcam_info));
	   ptMplsTcamInfo->cnt = 1;
	   ptMplsTcamInfo->vlan_id = vlan_id;
	   sal_memcpy(ptMplsTcamInfo->src_mac, src_mac, 6);
	   linkListAddAtTail(g_hsl_mpls_station_tcam_list, (void *)ptMplsTcamInfo);
     }
     else
     {
          ptMplsTcamInfo = (hsl_bcm_mpls_tcam_info *)ptMplsTcamNode->data;
	  ptMplsTcamInfo->cnt++;
     }

	 
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	 	"hsl_bcm_mpls_station_tcam_add: vlan=%d, mac=%x:%x:%x:%x:%x:%x, cnt=%d\n\r",
	 	 ptMplsTcamInfo->vlan_id, 
	 	 ptMplsTcamInfo->src_mac[0],
	 	 ptMplsTcamInfo->src_mac[1],
	 	 ptMplsTcamInfo->src_mac[2],
	 	 ptMplsTcamInfo->src_mac[3],
	 	 ptMplsTcamInfo->src_mac[4],
	 	 ptMplsTcamInfo->src_mac[5],
	 	 ptMplsTcamInfo->cnt);
	 
     return rc;
}

unsigned int hsl_bcm_mpls_station_tcam_del(bcm_mac_t  src_mac,  bcm_vlan_t  vlan_id)
{
     T_LIST_NODE *ptMplsTcamNode = NULL;
      hsl_bcm_mpls_tcam_info  stMplsTcanInfo;
      hsl_bcm_mpls_tcam_info *ptMplsTcamInfo = NULL;
      int rc = 0;
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_del: Z L H test successful......!! \n");//test @zlh
	
      if(vlan_id == HSL_VLAN_INVALID)//zlh@150129 
     {
        vlan_id = 1;
     }
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_del_1: Z L H test successful......!! \n");//test @zlh
     memset(&stMplsTcanInfo, 0, sizeof(hsl_bcm_mpls_tcam_info));
     stMplsTcanInfo.vlan_id = vlan_id;
     memcpy(stMplsTcanInfo.src_mac, src_mac, 6);
     ptMplsTcamNode = linkListLookup(g_hsl_mpls_station_tcam_list, (void *)&stMplsTcanInfo);
	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_del_2: Z L H test successful......!! \n");//test @zlh
     if(ptMplsTcamNode != NULL)
     {
            ptMplsTcamInfo = (hsl_bcm_mpls_tcam_info *)ptMplsTcamNode->data;
	    if(ptMplsTcamInfo->cnt > 1)
	    {
                  ptMplsTcamInfo->cnt--;
		  	 
                  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	 	             "hsl_bcm_mpls_station_tcam_del: vlan=%d, mac=%x:%x:%x:%x:%x:%x, cnt=%d\n\r",
	 	                        ptMplsTcamInfo->vlan_id, 
	 	                        ptMplsTcamInfo->src_mac[0],
	 	                        ptMplsTcamInfo->src_mac[1],
	 	                        ptMplsTcamInfo->src_mac[2],
	 	                        ptMplsTcamInfo->src_mac[3],
	 	                        ptMplsTcamInfo->src_mac[4],
	 	                        ptMplsTcamInfo->src_mac[5],
	 	                        ptMplsTcamInfo->cnt);
	    }
	    else if(ptMplsTcamInfo->cnt == 1)
	    {
                   
                HSL_CHECK_RESULT(rc, bcm_l2_tunnel_delete(BCM_UNIT,src_mac, vlan_id));

		ptMplsTcamInfo->cnt--;	
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	 	                      "hsl_bcm_mpls_station_tcam_del: vlan=%d, mac=%x:%x:%x:%x:%x:%x, cnt=%d\n\r",
	 	                        ptMplsTcamInfo->vlan_id, 
	 	                        ptMplsTcamInfo->src_mac[0],
	 	                        ptMplsTcamInfo->src_mac[1],
	 	                        ptMplsTcamInfo->src_mac[2],
	 	                        ptMplsTcamInfo->src_mac[3],
	 	                        ptMplsTcamInfo->src_mac[4],
	 	                        ptMplsTcamInfo->src_mac[5],
	 	                        ptMplsTcamInfo->cnt);

		linkListRemove(g_hsl_mpls_station_tcam_list, (void *)ptMplsTcamInfo);   
	    }
     }
     else
     {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_station_tcam_del: ptMplsTcamNde ==NULL\n\r");
          return 1;
     }

    return rc;
}


unsigned int hsl_bcm_mpls_tunnel_l3egr_cmp(void  *pdata1, void *pdata2)//no execution
{
    hsl_bcm_mpls_tunnel_egress_cfg *l3_egr_info1 = NULL;
    hsl_bcm_mpls_tunnel_egress_cfg *l3_egr_info2 = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3egr_cmp: Z L H test successful......!! \n");//test @zlh

    if((pdata1 ==NULL) || (pdata2 == NULL))
    {
        return Link_List_Compare_Mismatch;
    }

    l3_egr_info1 = (hsl_bcm_mpls_tunnel_egress_cfg *) pdata1;
    l3_egr_info2 = (hsl_bcm_mpls_tunnel_egress_cfg *) pdata2;

    if((l3_egr_info1->out_port == l3_egr_info2->out_port)
        && (sal_memcmp(l3_egr_info1->des_mac, l3_egr_info2->des_mac, 6) == 0)
        && (l3_egr_info1->l3_if == l3_egr_info2->l3_if))
    {
        return Link_List_Compare_Match;
    }

    return Link_List_Compare_Mismatch;
}

unsigned int hsl_bcm_mpls_tunnel_l3if_cmp(void *pdata1, void *pdata2)//no execution
{

    hsl_bcm_mpls_tunnel_l3if_cfg *l3_intf_info1 = NULL;
    hsl_bcm_mpls_tunnel_l3if_cfg *l3_intf_info2 = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_cmp_1: Z L H test successful......!! \n");//test @zlh
  
    if((pdata1 ==NULL) || (pdata2 == NULL))
    {
             return Link_List_Compare_Mismatch;
    }

    l3_intf_info1 = (hsl_bcm_mpls_tunnel_l3if_cfg *) pdata1;
    l3_intf_info2 = (hsl_bcm_mpls_tunnel_l3if_cfg *) pdata2;

    if((l3_intf_info1->vlanid == l3_intf_info2->vlanid)
        && (l3_intf_info1->out_lsp_label == l3_intf_info2->out_lsp_label)
	&& (sal_memcmp(l3_intf_info1->mac, l3_intf_info2->mac, 6) == 0))
    {
         return Link_List_Compare_Match;
    }

    return Link_List_Compare_Mismatch;
}

/*创建三层接口*/
//int hsl_bcm_mpls_tunnel_l3if_add(hsl_bcm_mpls_tunnel_l3if_cfg *l3intf_cfg)
int hsl_bcm_mpls_tunnel_l3if_add(hsl_bcm_mpls_tunnel_l3if_cfg *l3intf_cfg, ial_lsp_type lsp_l3_type )

{
        int rv = 0;
	bcm_l3_intf_t l3_info;
	hsl_bcm_mpls_tunnel_l3if_cfg *ptl3intf_info;
	bcm_mpls_egress_label_t bcm_mpls_egr_label = {0};
	T_LIST_NODE *pL3IntfNode = NULL;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_add_1: Z L H test successful......!! \n");//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_add_2: vlanid = %d\n",l3intf_cfg->vlanid);//test @zlh
	
        if(l3intf_cfg->vlanid == HSL_VLAN_INVALID)
        {
                l3intf_cfg->vlanid  = 1;
        }
		
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_add_2: vlanid = %d\n",l3intf_cfg->vlanid);//test @zlh
        pL3IntfNode = linkListLookup(g_hsl_l3_intf_list, (void *)l3intf_cfg);
        if(pL3IntfNode == NULL)
        {
	     //创建一个用于mpls隧道的l3接口 
	     bcm_l3_intf_t_init(&l3_info);
	     l3_info.l3a_vid = l3intf_cfg->vlanid;
	     l3_info.l3a_mtu = 0x3fff;
	     memcpy(l3_info.l3a_mac_addr,l3intf_cfg->mac, sizeof(bcm_mac_t));
		 
		 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
	 	"hsl_bcm_mpls_tunnel_l3if_add_3: l3a_vid=%d, mac=%x:%x:%x:%x:%x:%x\n\r",
	 	 l3_info.l3a_vid, 
	 	 l3_info.l3a_mac_addr[0],
	 	 l3_info.l3a_mac_addr[1],
	 	 l3_info.l3a_mac_addr[2],
	 	 l3_info.l3a_mac_addr[3],
	 	 l3_info.l3a_mac_addr[4],
	 	 l3_info.l3a_mac_addr[5]
	 	 );//test @zlh
		
		 
             //创建l3接口前,接口vlan必须已创建
             bcm_vlan_create(BCM_UNIT,l3intf_cfg->vlanid);
			 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_add_3:l3intf_cfg->vlanid = %d \n",l3intf_cfg->vlanid);//test @zlh
             hsl_bcm_l2_switch_disable(l3intf_cfg->vlanid, 1);
			 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_add_4: l3intf_cfg->vlanid = %d \n",l3intf_cfg->vlanid);//test @zlh
			 
	     HSL_CHECK_RESULT(rv,bcm_l3_intf_create(BCM_UNIT,&l3_info));//调用bcm_l3_intf_create获取l3_if
	     
		 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_add_3: Z L H test successful......!! \n");//test @zlh
      
	     l3intf_cfg->l3_if = l3_info.l3a_intf_id;

	     //if(l3intf_cfg->lsp_type == IAL_LSP_PE_UP )//@zlh1015 判断action
	     //if(l3intf_cfg->out_lsp_label > 0)
	     if(lsp_l3_type == IAL_LSP_PE_UP)
	     {   
		    /*隧道绑定到L3 infc*/
		    memset(&bcm_mpls_egr_label, 0, sizeof(bcm_mpls_egress_label_t));    
		    bcm_mpls_egr_label.label = l3intf_cfg->out_lsp_label;
			
			if(l3intf_cfg->out_lsp_exp == 8)
			{
		    	bcm_mpls_egr_label.flags = BCM_MPLS_EGRESS_LABEL_TTL_COPY
														  | BCM_MPLS_EGRESS_LABEL_EXP_COPY;//specified ingress packets exp
				bcm_mpls_egr_label.exp = 0;
             }
			else
			{
				bcm_mpls_egr_label.flags = BCM_MPLS_EGRESS_LABEL_TTL_COPY
														   | BCM_MPLS_EGRESS_LABEL_EXP_SET;//zlh@150110 use specified exp

				bcm_mpls_egr_label.exp = l3intf_cfg->out_lsp_exp;
			}
			
		     bcm_mpls_egr_label.qos_map_id = 0;
		     bcm_mpls_egr_label.ttl = 255;
		     bcm_mpls_egr_label.pkt_cfi = 0;
		     bcm_mpls_egr_label.pkt_pri = 0;

		  /* 如果是3标签,对P设备动作为PHP;对PE设备不写驱动 */
	      HSL_CHECK_RESULT(rv,bcm_mpls_tunnel_initiator_set(BCM_UNIT,l3_info.l3a_intf_id,1,&bcm_mpls_egr_label));
	     }

	     ptl3intf_info = sal_alloc(sizeof(hsl_bcm_mpls_tunnel_l3if_cfg), "l3_intf_list");
	     if(ptl3intf_info == NULL)
	     {
                   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_add: malloc err\n");
		   return -1;
	     }

	     sal_memset(ptl3intf_info, 0, sizeof(hsl_bcm_mpls_tunnel_l3if_cfg));
	     ptl3intf_info->cnt = 1;
	     sal_memcpy(ptl3intf_info->mac,  l3intf_cfg->mac, 6);
	     ptl3intf_info->l3_if = l3_info.l3a_intf_id;
         ptl3intf_info->vlanid = l3intf_cfg->vlanid;
         ptl3intf_info->out_lsp_label = l3intf_cfg->out_lsp_label;
		 ptl3intf_info->out_lsp_exp = l3intf_cfg->out_lsp_exp;
	     linkListAddAtTail(g_hsl_l3_intf_list, (void *)ptl3intf_info);
        }
	else
	{
	    ptl3intf_info = (hsl_bcm_mpls_tunnel_l3if_cfg *)pL3IntfNode->data;
            ptl3intf_info->cnt ++;
	    l3intf_cfg->l3_if = ptl3intf_info->l3_if;
	}
	
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
			"hsl_bcm_mpls_tunnel_l3if_add: l3_if=0x%x cnt=%d\n", ptl3intf_info->l3_if, ptl3intf_info->cnt);
	return rv;
}



int hsl_bcm_mpls_tunnel_l3if_del(hsl_bcm_mpls_tunnel_l3if_cfg *l3intf_cfg)
{

    int rv = 0;
    bcm_l3_intf_t l3_info;
    hsl_bcm_mpls_tunnel_l3if_cfg *ptl3intf_info;
    T_LIST_NODE *pL3IntfNode = NULL;
    
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_del: Z L H test successful......!! \n");//test @zlh
    if(l3intf_cfg->vlanid == HSL_VLAN_INVALID)
    {
           l3intf_cfg->vlanid  = 1;
    }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_del_1: l3intf_cfg->vlanid = %d \n",l3intf_cfg->vlanid);//test @zlh
    pL3IntfNode = linkListLookup(g_hsl_l3_intf_list, (void *)l3intf_cfg);
    if(pL3IntfNode != NULL)
   {
        ptl3intf_info = (hsl_bcm_mpls_tunnel_l3if_cfg *)pL3IntfNode->data;

	l3intf_cfg->l3_if = ptl3intf_info->l3_if;
	   
        if(ptl3intf_info->cnt > 1)
	{
	      ptl3intf_info->cnt--;
	      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
			 "hsl_bcm_mpls_tunnel_l3if_del: l3_intf= %d, cnt = %d\n", ptl3intf_info->l3_if, ptl3intf_info->cnt);
	}
	else if(ptl3intf_info->cnt == 1)
	{
		
               bcm_mpls_tunnel_initiator_clear(BCM_UNIT, ptl3intf_info->l3_if);
				
	
	    	   bcm_l3_intf_t_init(&l3_info);
		l3_info.l3a_vid = l3intf_cfg->vlanid;
		l3_info.l3a_mtu = 0x3fff;
		memcpy(l3_info.l3a_mac_addr,l3intf_cfg->mac, sizeof(bcm_mac_t));
	        l3_info.l3a_intf_id = ptl3intf_info->l3_if;

		HSL_CHECK_RESULT(rv,bcm_l3_intf_delete(BCM_UNIT,&l3_info));

		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
			 	"hsl_bcm_mpls_tunnel_l3if_del: l3_intf= %d, cnt = 0\n", ptl3intf_info->l3_if);
			 
                linkListRemove(g_hsl_l3_intf_list, (void *)ptl3intf_info);
		ptl3intf_info = NULL;			
			
	}
    }
    else
    {
           HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_mpls_tunnel_l3if_del: ptl3intf_info==NULL\n\r");
            return 1;
    }
	
    return rv;
}



/***************************************************************
* Function: hsl_mpls_protect_group_add
* Description: 创建保护组
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
int hsl_mpls_protect_group_add(ial_mpls_protect_group_cfg *pro_grp_cfg)
{
    int rc = DRV_SUCCESS;
    hsl_lsp_info   *p_work_lsp_info = NULL;
    hsl_lsp_info   *p_backup_lsp_info = NULL;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;
         
    HSL_CHECK_PTR(rc, pro_grp_cfg);

    HSL_CHECK_LSP_PROTECT_GRP_ID(rc, pro_grp_cfg->grp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_1: Z L H test successful......!! \n");//test @zlh
    hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[pro_grp_cfg->grp_id]);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_2: grp_id                              = %d\n",pro_grp_cfg->grp_id);//test @zlh
    if(hsl_pro_grp_info->grp_cfg == HSL_CFG_YES)
    {
        return DRV_E_LSP_PROTECT_GRP_EXIST;
    }
        
    HSL_CHECK_LSP_ID(rc, pro_grp_cfg->work_lsp_id);
    HSL_CHECK_LSP_ID(rc, pro_grp_cfg->backup_lsp_id);
    
    p_work_lsp_info =  hsl_mpls_get_lsp_info(pro_grp_cfg->work_lsp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_3: work_lsp_id                         = %d\n",pro_grp_cfg->work_lsp_id);//test @zlh
	
    if(NULL == p_work_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    p_backup_lsp_info =   hsl_mpls_get_lsp_info(pro_grp_cfg->backup_lsp_id);
    if(NULL == p_backup_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    if(p_work_lsp_info->lsp_node_type != p_backup_lsp_info->lsp_node_type)
    {
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
            "hsl_mpls_protect_group_add: work type = %d, backup type = %d\n\r", 
                              p_work_lsp_info->lsp_node_type, p_backup_lsp_info->lsp_node_type);
         return DRV_E_LSP_PROTECT_GRP_PARAM;
    }

   /*pe 节点只支持路径保护*/
    if((p_work_lsp_info->lsp_node_type == HSL_LSP_NODE_TYPE_PE) 
        && ((pro_grp_cfg->protect_type == IAL_PROTECT_TYPE_1PLUS1_SNC) 
                || (pro_grp_cfg->protect_type == IAL_PROTECT_TYPE_1BY1_SNC)))
    {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
            "hsl_mpls_protect_group_add: protect type  = %d,  lsp  type = %d\n\r", 
                           pro_grp_cfg->protect_type, p_work_lsp_info->lsp_node_type);
        return DRV_E_LSP_PROTECT_GRP_PARAM;
    }

    /*p 节点只支持子网保护*/
    if((p_work_lsp_info->lsp_node_type == HSL_LSP_NODE_TYPE_P)
            && ((pro_grp_cfg->protect_type == IAL_PROTECT_TYPE_1PLUS1) 
            || (pro_grp_cfg->protect_type == IAL_PROTECT_TYPE_1BY1)))
    {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
               "hsl_mpls_protect_group_add: protect type  = %d,  lsp  type = %d\n\r", 
                             pro_grp_cfg->protect_type, p_work_lsp_info->lsp_node_type);
         
        return DRV_E_LSP_PROTECT_GRP_PARAM;
    }

    p_work_lsp_info->protect_grp_id = pro_grp_cfg->grp_id;
    p_work_lsp_info->protect_status = HSL_CFG_YES;
    
    p_backup_lsp_info->protect_grp_id = pro_grp_cfg->grp_id;
    p_backup_lsp_info->protect_status = HSL_CFG_YES;

    //hsl_pro_grp_info->switch_status = IAL_PROTECT_SWITCH_FORCE;
    hsl_pro_grp_info->switch_status = IAL_PROTECT_SWITCH_LOCK;//zlh@141212 status is lock
    hsl_pro_grp_info->lock_lsp_id = pro_grp_cfg->work_lsp_id;
    hsl_pro_grp_info->work_lsp_id = pro_grp_cfg->work_lsp_id;
    hsl_pro_grp_info->backup_lsp_id = pro_grp_cfg->backup_lsp_id;
    hsl_pro_grp_info->protect_type = pro_grp_cfg->protect_type;
    hsl_pro_grp_info->current_lsp_id = pro_grp_cfg->work_lsp_id;
    hsl_pro_grp_info->back_time  = pro_grp_cfg->back_time * 10;    /*1s-->100ms*/
    //hsl_pro_grp_info->sw_back_flag = IAL_PROTECT_SWITCH_BACK_DIS;
    hsl_pro_grp_info->sw_back_flag = IAL_PROTECT_SWITCH_BACK_EN;//zlh@141212 switch enable
    hsl_pro_grp_info->grp_cfg = HSL_CFG_YES;

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_4: p_work_lsp_info->protect_grp_id     = %d \n",pro_grp_cfg->grp_id            );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_5: p_work_lsp_info->protect_status     = %d\n", p_work_lsp_info->protect_status);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_6: p_backup_lsp_info->protect_grp_id   = %d\n",p_backup_lsp_info->protect_grp_id);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_7: p_backup_lsp_info->protect_status   = %d\n",p_backup_lsp_info->protect_status);//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_8: hsl_pro_grp_info->switch_status     = %d\n",hsl_pro_grp_info->switch_status  );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_7: hsl_pro_grp_info->lock_lsp_id       = %d\n",hsl_pro_grp_info->lock_lsp_id    );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_9: hsl_pro_grp_info->work_lsp_id       = %d\n",hsl_pro_grp_info->work_lsp_id    );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_10: hsl_pro_grp_info->backup_lsp_id    = %d\n",hsl_pro_grp_info->backup_lsp_id  );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_11: protect_typ                        = %d\n",hsl_pro_grp_info->protect_type   );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_12: hsl_pro_grp_info->current_lsp_id   = %d\n",hsl_pro_grp_info->current_lsp_id );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_13: hsl_pro_grp_info->back_time        = %d\n",hsl_pro_grp_info->back_time      );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_14: sw_back_flag                       = %d\n",hsl_pro_grp_info->sw_back_flag   );//test @zlh
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_add_15: grp_cfg                            = %d\n",hsl_pro_grp_info->grp_cfg        );//test @zlh
	
    return rc;
	
}


/***************************************************************
* Function: hsl_mpls_protect_group_del
* Description: 删除保护组
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
int hsl_mpls_protect_group_del(ial_mpls_protect_group_cfg *pro_grp_cfg)//no execution
{
    int rc = DRV_SUCCESS;
    hsl_lsp_info   *p_work_lsp_info = NULL;
    hsl_lsp_info   *p_backup_lsp_info = NULL;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;
         
    HSL_CHECK_PTR(rc, pro_grp_cfg);

    HSL_CHECK_LSP_PROTECT_GRP_ID(rc, pro_grp_cfg->grp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_del: Z L H test successful......!! \n");//test @zlh
    hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[pro_grp_cfg->grp_id]);
    if(hsl_pro_grp_info->grp_cfg == HSL_CFG_NO)
    {
        return DRV_E_LSP_PROTECT_GRP_NO_EXIST;
    }
    
    p_work_lsp_info =   hsl_mpls_get_lsp_info(hsl_pro_grp_info->work_lsp_id);
    if(NULL == p_work_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    p_backup_lsp_info =  hsl_mpls_get_lsp_info(hsl_pro_grp_info->backup_lsp_id);
    if(NULL == p_backup_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }

   hsl_pro_grp_info->switch_status =  IAL_PROTECT_SWITCH_FORCE;

   HSL_CHECK_RESULT(rc,hsl_mpls_protect_group_switch_1by1(pro_grp_cfg->grp_id, hsl_pro_grp_info->work_lsp_id));

    p_work_lsp_info->protect_grp_id = 0;
    p_work_lsp_info->protect_status = HSL_CFG_NO;
    p_backup_lsp_info->protect_grp_id = 0;
    p_backup_lsp_info->protect_status = HSL_CFG_NO;
    
    memset(hsl_pro_grp_info, 0, sizeof(hsl_mpls_protect_group_info));
    return rc;
}

/***************************************************************
* Function: hsl_mpls_protect_group_mod_param
* Description: 修改保护组参数
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_mpls_protect_group_mod_param(ial_mpls_protect_group_cfg *pro_grp_cfg)
{
    uint32 rc = DRV_SUCCESS;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;

    HSL_CHECK_PTR(rc, pro_grp_cfg);

    HSL_CHECK_LSP_PROTECT_GRP_ID(rc, pro_grp_cfg->grp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_mod_param_1: Z L H test successful......!! \n");//test @zlh
    hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[pro_grp_cfg->grp_id]);
    if(hsl_pro_grp_info->grp_cfg == HSL_CFG_NO)
    {
        return DRV_E_LSP_PROTECT_GRP_NO_EXIST;
    }

    hsl_pro_grp_info->back_time  = pro_grp_cfg->back_time * 10; /*1s  -> 100ms*/

    if(hsl_pro_grp_info->sw_back_flag != pro_grp_cfg->sw_back_flag)
    {
        hsl_pro_grp_info->back_time_cnt = hsl_pro_grp_info->back_time;
    }

    hsl_pro_grp_info->sw_back_flag = pro_grp_cfg->sw_back_flag;
    
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_mod_param_1: sw_back_flag = %d\n",pro_grp_cfg->sw_back_flag);//zlh@141212 test

}

/***************************************************************
* Function: hsl_mpls_protect_group_switch
* Description: 保护组状态切换
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_mpls_protect_group_switch(ial_mpls_protect_switch_cfg *grp_sw_cfg)
{
    int rc = DRV_SUCCESS;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;
    hsl_lsp_info  *lsp_info = NULL;
    uint32 sw_lsp_id = 0;
    uint32 sw_lsp_flg = 0;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_portect_group_switch_1: Z L H test successful......!! \n");//test @zlh

    HSL_CHECK_PTR(rc, grp_sw_cfg);

    HSL_CHECK_LSP_PROTECT_GRP_ID(rc, grp_sw_cfg->grp_id);
	
	
    hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[grp_sw_cfg->grp_id]);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_portect_group_switch_2: grp_id                 = %d \n",grp_sw_cfg->grp_id);//test @zlh
	
    if(hsl_pro_grp_info->grp_cfg == HSL_CFG_NO)
    {
        return DRV_E_LSP_PROTECT_GRP_NO_EXIST;
    }

    if(grp_sw_cfg->switch_status == IAL_PROTECT_SWITCH_FORCE)
    {
        if((grp_sw_cfg->switch_lsp_id != hsl_pro_grp_info->work_lsp_id) 
            && (grp_sw_cfg->switch_lsp_id != hsl_pro_grp_info->backup_lsp_id))
        {
            return DRV_E_LSP_ID_INVALID;
        }
        
        sw_lsp_flg = 1;
        sw_lsp_id  = grp_sw_cfg->switch_lsp_id;
    }
    
    if(IAL_PROTECT_SWITCH_LOCK == grp_sw_cfg->switch_status)
    {
        if((grp_sw_cfg->lock_lsp_id != hsl_pro_grp_info->work_lsp_id) 
            && (grp_sw_cfg->lock_lsp_id != hsl_pro_grp_info->backup_lsp_id))
        {
            return DRV_E_LSP_ID_INVALID;
        }
        
        lsp_info =  hsl_mpls_get_lsp_info(grp_sw_cfg->switch_lsp_id);//zlh@141212 switch_lsp_id是否为current_lsp_id??
        if(NULL == lsp_info)
        {
            return DRV_E_LSP_NO_EXIST;
        }

        if((lsp_info->lsp_egress_info.lsp_status == IAL_LSP_STATUS_UP) 
            && (lsp_info->lsp_ingress_info.lsp_status == IAL_LSP_STATUS_UP))
        {
            sw_lsp_flg = 1;
            sw_lsp_id  = grp_sw_cfg->lock_lsp_id;
        }
    }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_portect_group_switch_3: sw_lsp_flg             = %d\n",sw_lsp_flg);//zlh@141212 test

    if(sw_lsp_flg == 1)
    {
      
    }
    hsl_pro_grp_info->switch_status = grp_sw_cfg->switch_status;
    hsl_pro_grp_info->lock_lsp_id = grp_sw_cfg->lock_lsp_id;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_portect_group_switch_4: switch_status           = %d\n",grp_sw_cfg->switch_status);//zlh@141212 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_portect_group_switch_5: lock_lsp_id             = %d\n",grp_sw_cfg->lock_lsp_id  );//zlh@141212 test

	return rc;
}

/***************************************************************
* Function: hsl_mpls_protect_group_switch_1by1
* Description: 保护组状态切换1:1 路径保护切换
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/

uint32 hsl_mpls_protect_group_switch_1by1(uint32 grp_id, uint32 sw_lsp_id)
{
    int rc = DRV_SUCCESS;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;
    hsl_lsp_info   *p_work_lsp_info = NULL;
    hsl_lsp_info   *p_backup_lsp_info = NULL;
    hsl_lsp_info   *p_current_lsp_info = NULL;//zlh@1209
    uint32 egr_mac_da_index = 0;
    uint32 egr_l3_intf_index = 0;
	int in_port;//zlh@1211
	bcm_mpls_port_t bcm_pw_port;//zlh@1211
    bcm_port_t bcm_port;
    bcm_port_t bcm_port_b;
    bcm_gport_t bcm_gport;
    bcm_gport_t bcm_gport_b;
    
    uint32 egr_l3_next_hop;
    egr_l3_next_hop_entry_t egr_nh;
    ing_l3_next_hop_entry_t ing_nh;
    l2mc_entry_t    l2mc_entry;
    T_LIST_NODE *pListNode = NULL;
    hsl_bcm_pw_port_info_1307 *p_hsl_pw_info = NULL;
    hsl_bcm_vpn_info           *vpn_info = NULL;
    ial_protect_status pro_status;
	
    HSL_CHECK_LSP_PROTECT_GRP_ID(rc, grp_id);
    hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[grp_id]);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_switch_1by1_1   grp_id = %d\n",grp_id);//zlh@141216
	/* 获得当前lsp info */
	p_current_lsp_info = hsl_mpls_get_lsp_info(hsl_pro_grp_info->current_lsp_id);//zlh@141212
	
    if(hsl_pro_grp_info->current_lsp_id == sw_lsp_id)
    {
        return rc;
    }

    p_work_lsp_info =  hsl_mpls_get_lsp_info(hsl_pro_grp_info->work_lsp_id);
    if(NULL == p_work_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    p_backup_lsp_info =  hsl_mpls_get_lsp_info(hsl_pro_grp_info->backup_lsp_id);
    if(NULL == p_backup_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }
    

    if(sw_lsp_id == hsl_pro_grp_info->work_lsp_id)
    {
        pro_status = IAL_PROTECT_STATUS_WORK;
    }
    else if(sw_lsp_id == hsl_pro_grp_info->backup_lsp_id)
    {
        pro_status = IAL_PROTECT_STATUS_BACKUP;
    }
    else
    {
        return DRV_E_LSP_ID_INVALID;
    }
	
   pListNode = p_current_lsp_info->lsp_egress_info.bind_pw_list->pHead;//zlh@141212
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_switch_1by1_2: pro_status = %d\n",pro_status);//test zlh@1211
    while(pListNode != NULL)
    {
        p_hsl_pw_info = ( hsl_bcm_pw_port_info_1307* )pListNode->data;
      
        vpn_info = hsl_get_l2vpn_info(p_hsl_pw_info->hsl_vpn_id);
		
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_switch_1by1_3: hsl_vpn_id = %d\n",p_hsl_pw_info->hsl_vpn_id);//test zlh@1211
		
        if(vpn_info == NULL)
        {
            return DRV_E_L2VPN_NO_EXIST;
        }

		
		/* zlh@141209 */
		if(vpn_info->hsl_vpn_id >= 1970)
		{
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"vpn_info->hsl_vpn_id = %d\n",vpn_info->hsl_vpn_id);//zlh@1209
			 
		}
		else
		{
		
		/* zlh@141212 将vpn进行切换 */
		
			hsl_bcm_pw_port_info_1307 switch_pw_port_info = *p_hsl_pw_info;
		
			if(pro_status == IAL_PROTECT_STATUS_BACKUP)
			{
		
				switch_pw_port_info.in_port = p_backup_lsp_info->lsp_ingress_info.in_port;
				switch_pw_port_info.out_port = p_backup_lsp_info->lsp_egress_info.out_port;
				switch_pw_port_info.out_lsp_label = p_backup_lsp_info->lsp_egress_info.out_label;
				switch_pw_port_info.match_lsp_label = p_backup_lsp_info->lsp_ingress_info.in_label;
				sal_memcpy(switch_pw_port_info.des_mac, p_backup_lsp_info->lsp_egress_info.des_mac, sizeof(bcm_mac_t));//zlh@141212
				switch_pw_port_info.out_lsp_exp = p_backup_lsp_info->lsp_egress_info.out_label_exp;
				switch_pw_port_info.lsp_id = p_backup_lsp_info->tunnel_id;
			}
			else
			{
				switch_pw_port_info.in_port = p_work_lsp_info->lsp_ingress_info.in_port;
				switch_pw_port_info.out_port = p_work_lsp_info->lsp_egress_info.out_port;
				switch_pw_port_info.out_lsp_label = p_work_lsp_info->lsp_egress_info.out_label;
				switch_pw_port_info.match_lsp_label = p_work_lsp_info->lsp_ingress_info.in_label;
				sal_memcpy(switch_pw_port_info.des_mac, p_work_lsp_info->lsp_egress_info.des_mac, sizeof(bcm_mac_t));//zlh@141212
				switch_pw_port_info.out_lsp_exp = p_work_lsp_info->lsp_egress_info.out_label_exp;
				switch_pw_port_info.lsp_id = p_work_lsp_info->tunnel_id;
			}
			
		 	HSL_CHECK_RESULT(rc,hsl_bcm_vpn_pw_port_del( p_hsl_pw_info));
			//hsl_bcm_vpn_pw_port_del( p_hsl_pw_info);
		 	HSL_CHECK_RESULT(rc,hsl_bcm_vpn_pw_port_add( &switch_pw_port_info ));
		 	//hsl_bcm_vpn_pw_port_add( &switch_pw_port_info );
		}
			

        pListNode = pListNode->next;

		
    }

    hsl_pro_grp_info->current_lsp_id =  sw_lsp_id;
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_mpls_protect_group_switch_1by1_6: hsl_pro_grp_info->current_lsp_id = %d\n",sw_lsp_id);//zlh@141212
    return rc;
}

/***************************************************************
* Function: hsl_mpls_protect_group_switch_1by1snc
* Description: 保护组状态切换1:1 路径保护切换
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_mpls_protect_group_switch_1by1snc(uint32 grp_id, uint32 sw_lsp_id)//no execution
{
    int rc = DRV_SUCCESS;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;
    hsl_lsp_info   *p_work_lsp_info = NULL;
    hsl_lsp_info   *p_backup_lsp_info = NULL;
    uint32 mpls_entry_index;
    uint32 egr_l3_next_hop;
    mpls_entry_entry_t ment;
    ial_protect_status  pro_status;

    HSL_CHECK_LSP_PROTECT_GRP_ID(rc, grp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_protect_group_switch_1by1snc: Z L H test successful......!! \n");//test @zlh
	
    hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[grp_id]);
   
    if(hsl_pro_grp_info->current_lsp_id == sw_lsp_id)
    {
        return rc;
    }

    p_work_lsp_info =  hsl_mpls_get_lsp_info(hsl_pro_grp_info->work_lsp_id);
    if(NULL == p_work_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    p_backup_lsp_info =  hsl_mpls_get_lsp_info(hsl_pro_grp_info->backup_lsp_id);
    if(NULL == p_backup_lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    if(sw_lsp_id == hsl_pro_grp_info->work_lsp_id)
    {
        pro_status = IAL_PROTECT_STATUS_WORK;
    }
    else
    {
        pro_status = IAL_PROTECT_STATUS_BACKUP;
    }

    mpls_entry_index = p_work_lsp_info->lsp_ingress_info.mpls_entry_index;
    
    if(pro_status == IAL_PROTECT_STATUS_WORK)
    {
        egr_l3_next_hop = p_work_lsp_info->lsp_ingress_info.egr_l3_next_hop;
    }
    else
    {
         egr_l3_next_hop = p_backup_lsp_info->lsp_ingress_info.egr_l3_next_hop;
    }

  
    hsl_pro_grp_info->current_lsp_id =  sw_lsp_id;
    return rc;
}


/***************************************************************
* Function: hsl_mpls_get_protect_grp_info
* Description: get protect grp info
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_mpls_get_protect_grp_info(ial_mpls_protect_group_info *pro_grp_cfg)//no execution
{
    uint32 rc = DRV_SUCCESS;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;

    HSL_CHECK_PTR(rc, pro_grp_cfg);

    HSL_CHECK_LSP_PROTECT_GRP_ID(rc, pro_grp_cfg->grp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_get_protect_grp_info: Z L H test successful......!! \n");//test @zlh
    hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[pro_grp_cfg->grp_id]);
    if(hsl_pro_grp_info->grp_cfg == HSL_CFG_NO)
    {
        return DRV_E_LSP_PROTECT_GRP_NO_EXIST;
    }

    pro_grp_cfg->protect_type    =   hsl_pro_grp_info->protect_type;
    pro_grp_cfg->work_lsp_id     =   hsl_pro_grp_info->work_lsp_id;
    pro_grp_cfg->backup_lsp_id  =   hsl_pro_grp_info->backup_lsp_id;
    pro_grp_cfg->current_lsp_id  =   hsl_pro_grp_info->current_lsp_id;    /*当前保护组 工作隧道id */
    pro_grp_cfg->lock_lsp_id       =   hsl_pro_grp_info->lock_lsp_id; 
    pro_grp_cfg->switch_status  =   hsl_pro_grp_info->switch_status;
    pro_grp_cfg->sw_back_flag  =   hsl_pro_grp_info->sw_back_flag;
    pro_grp_cfg->back_time        =   hsl_pro_grp_info->back_time / 10;

    return rc;
}

uint32 hsl_oam_init(void)
{
	int rv = 0;
    uint32 rc = 0;
	int ret;
    bcm_oam_event_types_t event_types;

    HSL_CHECK_RESULT(rc,bcm_oam_init(0));

    memset(g_hsl_oam_group_info, 0, sizeof(hsl_oam_grp_info) * HSL_OAM_MAX);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_init: Z L H test successful......!! \n");//test @zlh

	/*zlh@1017 create sem */
    ret = oss_sem_new ("oam sem", OSS_SEM_BINARY, 0, NULL, &hsl_oam_evens_sem);
	if(ret < 0)
		{
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
                  "hsl_oam_init: create oam evens sem err  file:%s, line:%d\n", __FILE__, __LINE__);
        return 1;
    }
	 
     hsl_oam_thread_id = sal_thread_create("oam evens", SAL_THREAD_STKSZ,60, hsl_oam_evens_thread, 0);
     if (hsl_oam_thread_id == SAL_THREAD_ERROR)
     {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
                        "hsl_oam_init: create evens thread err  file:%s, line:%d\n", __FILE__, __LINE__);
         return 1;
     }

    BCM_OAM_EVENT_TYPE_SET_ALL(event_types);
    HSL_CHECK_RESULT(rc, bcm_oam_event_register(0, event_types,  hsl_oam_enevs_irq, NULL));
	/*  zlh@1211 */
	if (BCM_FAILURE(rv))
	{
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"oam_event_register failed: %s.\n ",bcm_errmsg(rv));
	return rv;
	}
	else
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"CCM event monitoring on\n");

    return rc;
}

/*******************************************************************
*Function:hsl_oam_add
*Description:添加 oam 
*othre: zlh@140912
********************************************************************/

uint32 hsl_oam_add(ial_oam_cfg *oam_cfg)
{
	int rv = 0;
	bcm_oam_group_info_t  group_info;
	bcm_oam_endpoint_info_t endpoint_info;
	char *maid = oam_cfg->ma_name;
	bcm_pbmp_t pbmp, upbmp;
	bcm_mac_t  oam_src = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x17 };
	//add Dst_mac
	bcm_mac_t  oam_dst = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x32 };
	hsl_oam_grp_info  *p_oam_info = NULL;
    bcm_port_t bcm_port;
	uint32 rc = 0;
    hsl_bcm_ac_port_info  stHslVpnAcCfg;
    hsl_bcm_pw_port_info_1307 stHslVpnPwCfg;
    hsl_bcm_msg_vpn_cfg vpn_cfg;
    hsl_lsp_info *lsp_info = NULL;

	
    HSL_CHECK_PTR(rc, oam_cfg);
    HSL_CHECK_OAM_ID(rc, oam_cfg->oam_id);
	p_oam_info = &g_hsl_oam_group_info[oam_cfg->oam_id];
	
	if(p_oam_info->hsl_cfg == HSL_CFG_YES)
    {
      return DRV_E_OAM_EXIST;
    }
	
	HSL_CHECK_LSP_ID(rc, oam_cfg->lsp_id);
	lsp_info=hsl_mpls_get_lsp_info(oam_cfg->lsp_id);
	if(NULL == lsp_info)
    {
        return DRV_E_LSP_NO_EXIST;
    }
	
	p_oam_info->vlan_id = oam_cfg->oam_id + VLAN_NUM;//3500 to 3800
	p_oam_info->in_pw_label = lsp_info->lsp_ingress_info.in_label+PW_NUM;
	p_oam_info->out_pw_label = lsp_info->lsp_egress_info.out_label+PW_NUM;

   
	bcm_oam_group_info_t_init (&group_info);
   	group_info.id = oam_cfg->oam_id;// group_info.id with oam_id 
   	group_info.flags |= BCM_OAM_GROUP_WITH_ID;
	
   /* 
    * To comply with IEEE 802.1ag section 21.6.5.1 Maintenance 
    * Domain Name Format, the first octet should be 4 to indicate the
    * MAID is composed of string. The second octet should be 10 to 
	* indicate the MAID length is 10 bytes 
	*/
   	group_info.name[0] = 4;
   	group_info.name[1] = sal_strlen(maid);
   	sal_strncpy((char *)group_info.name + 2, maid,BCM_OAM_GROUP_NAME_LENGTH  - 1);
		
   	rv =  bcm_oam_group_create (BCM_UNIT, &group_info);
   	if (BCM_FAILURE(rv)) 
		{
      		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"oam_group_create failed: %s.\n", bcm_errmsg(rv));
      	return rv;
		} else
      		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"MA group %d created.\n", group_info.id);
	

	/* Create CCM RX endpoint */
   	/* 
	* The equivalent CLI command is "oam endpoint add 
	* group=oam_cfg->oam_id ID=oam_cfg->oam_id CCM level=2 vlan=p_oam_info->vlan_id port=fe22” 
	*/
    bcm_oam_endpoint_info_t_init(&endpoint_info);
  	/* MA group ID, 1 in this example */
   	endpoint_info.group = oam_cfg->oam_id;
   	/* A resource limited ID # */
   	endpoint_info.id = oam_cfg->oam_id*2;
   	endpoint_info.flags |= BCM_OAM_ENDPOINT_WITH_ID;
   	/* Specify the endpoint is for CCM RX  */
   	endpoint_info.flags |= BCM_OAM_ENDPOINT_CCM_RX;
	/* Select 16 bits MEPID, resource limit is 512 entries */
   	endpoint_info.name = (uint16) 101;
   	/* MD level: 0 - 7 which needs to match CCM TX MDL */
   	endpoint_info.level = 2;
	/* Expected CCM packet frequency ranged from 3.3 to 180000 */
   	endpoint_info.ccm_period  = oam_cfg->ccm_period; /* milliseconds based */
	
   	BCM_IF_ERROR_RETURN(bcm_vlan_create(BCM_UNIT, p_oam_info->vlan_id));
   	BCM_PBMP_CLEAR(pbmp);
   	BCM_PBMP_CLEAR(upbmp);
   	BCM_PBMP_PORT_ADD(pbmp, OAM_ENDPOINT_PORT_1); /* Add GE2 (3) in VLAN 2 */
   	//BCM_PBMP_PORT_ADD(pbmp, 0); /* Add CPU port in VLAN 2 for debugging only */
                                  
   	BCM_IF_ERROR_RETURN(bcm_vlan_port_add(BCM_UNIT, p_oam_info->vlan_id, pbmp, upbmp));
	
   	/* Specify CCM VLAN  */
   	endpoint_info.vlan = p_oam_info->vlan_id;
   	/* Specify CCM RX port which is fe22 */
    BCM_GPORT_LOCAL_SET(endpoint_info.gport , OAM_ENDPOINT_PORT_1); // fe22 is 24 taking BCM56134_B0 

	/* Specify CCM packet SA */
	   sal_memcpy(endpoint_info.src_mac_address , oam_src, sizeof(bcm_mac_t));
	 sal_memcpy(endpoint_info.dst_mac_address , oam_dst, sizeof(bcm_mac_t));
	/* Specify CCM packet priority = 0 */
	   endpoint_info.pkt_pri = 0;
	   /* Specify CCM packet internal priority = 0	*/
	   endpoint_info.int_pri = 0;

   	/* Call API to install CCM RX end point  */
  	 rv =  bcm_oam_endpoint_create(BCM_UNIT, &endpoint_info);
  	 if (BCM_FAILURE(rv)) 
	 	{
      	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"oam_endpoint_create failed: %s.\n", bcm_errmsg(rv));
      	return rv;
   	 	} 
	 else
      	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"CCM RX endpoint %d created.\n", endpoint_info.id);
	
   

   	/* Create CCM RMEP information */
   	/* 
   	 * The equivalent CLI command is "oam endpoint add 
	* group=oam_cfg->oam_id ID=oam_cfg->oam_id Remote port=fe22 vlan=p_oam_info->vlan_id level=5 name=111
	* period=10000 
	*/
    bcm_oam_endpoint_info_t_init(&endpoint_info);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"bcm_oam_endpoint_info_t_init test successfull**********!!!\n");
	/* MA group ID, 1 in this example */
   	endpoint_info.group  = oam_cfg->oam_id;
   	/* A resource limited ID # */
   	endpoint_info.id = oam_cfg->oam_id*2+1;//
   	endpoint_info.flags  |= BCM_OAM_ENDPOINT_WITH_ID;
   	/* Indicate this endpoint is for RMEP information */
   	endpoint_info.flags  |= BCM_OAM_ENDPOINT_REMOTE;
   	/* Select 16 bits MEPID, resource limit is 512 entries */
   	endpoint_info.name = (uint16) 101;
   	/* MD level: 0 - 7 */
   	endpoint_info.level  = 2;
   	/* Expected CCM packet frequency ranged from 3.3 to 180000 */
   	endpoint_info.ccm_period  = oam_cfg->ccm_period; /* milliseconds based */
   	/* Specify CCM VLAN  */
   	endpoint_info.vlan = p_oam_info->vlan_id;
   	/* Specify CCM RX port which is fe22 */
   	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"bcm_oam_endpoint_info_t_init test successfull**********!!!\n");
    BCM_GPORT_LOCAL_SET(endpoint_info.gport , OAM_ENDPOINT_PORT_1);
   	/* Call API to install CCM RMEP endpoint */
   	rv =  bcm_oam_endpoint_create(BCM_UNIT, &endpoint_info);
   	if (BCM_FAILURE(rv)) 
		{
      	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"oam_endpoint_create failed: %s.\n", bcm_errmsg(rv));
      	return rv;
   		} 
	else
      	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"CCM remote endpoint %d created.\n", endpoint_info.id);

		/* 将p_oam_info信息存在本地 */

		p_oam_info->oam_id = oam_cfg->oam_id;
		p_oam_info->hsl_cfg = HSL_CFG_YES;
		p_oam_info->in_port= OAM_ENDPOINT_PORT_1;
		p_oam_info->out_port= OAM_ENDPOINT_PORT_2;
		p_oam_info->lsp_id = oam_cfg->lsp_id;
		p_oam_info->ma_id = oam_cfg->oam_id;
		p_oam_info->lmep_id = oam_cfg->oam_id*2;
		p_oam_info->rmep_id = oam_cfg->oam_id*2+1;
		p_oam_info->ccm_period = oam_cfg->ccm_period;
		p_oam_info->mep_name = oam_cfg->mep_name;
		p_oam_info->in_port = lsp_info->lsp_ingress_info.in_port;
		p_oam_info->out_port= lsp_info->lsp_egress_info.out_port;
		
		/*
		lsp_info->lsp_ingress_info.lsp_status = IAL_LSP_STATUS_UP;//@zlh1024
		lsp_info->lsp_egress_info.lsp_status = IAL_LSP_STATUS_UP;//@zlh1024
		*/
      	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_oam_add:lsp_info->lsp_ingress_info.lsp_status = %d\n",lsp_info->lsp_ingress_info.lsp_status);

		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_oam_add:lsp_info->lsp_egress_info.lsp_status = %d\n", lsp_info->lsp_egress_info.lsp_status);
		
	/* create oam_vpn_create*/
		vpn_cfg.hsl_vpn_id = p_oam_info->vlan_id-VPN_NUM;//1970-2000 共计 31个
		vpn_cfg.hsl_vpn_type = 0;
		vpn_cfg.un_pkt_flg = 0;
		vpn_cfg.mac_learn_flg = 0;
	
		HSL_CHECK_RESULT(rc,hsl_bcm_vpn_create ( &vpn_cfg ));
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"oam_vpn_create successfully\n");
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_vpn_id = %d\n",vpn_cfg.hsl_vpn_id);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_vpn_type =% d\n",vpn_cfg.hsl_vpn_type);

		/* create oam_ac_port_add */
		stHslVpnAcCfg.hsl_vpn_id = p_oam_info->vlan_id-VPN_NUM;
		stHslVpnAcCfg.ac_port = OAM_ENDPOINT_PORT_2;//fe23 is 24 taking BCM56134_B0 
		stHslVpnAcCfg.ac_vlan = p_oam_info->vlan_id;
		stHslVpnAcCfg.out_disable = 0;
		stHslVpnAcCfg.in_disable = 0;
	
	 	HSL_CHECK_RESULT(rc,hsl_bcm_vpn_ac_port_add(&stHslVpnAcCfg));
		//hsl_bcm_vpn_ac_port_add(&stHslVpnAcCfg);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"oam_vpn_ac_port_add successfully\n");
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_vpn_id = %d\n",stHslVpnAcCfg.hsl_vpn_id);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"ac_port = %d\n",stHslVpnAcCfg.ac_port);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"ac_vlan = %d\n",stHslVpnAcCfg.ac_vlan);		

		/* create oam_pw_port_add */

		stHslVpnPwCfg.hsl_vpn_id = p_oam_info->vlan_id-VPN_NUM;
		stHslVpnPwCfg.match_pw_label = p_oam_info->in_pw_label;
		stHslVpnPwCfg.in_disable = 0;
	    stHslVpnPwCfg.out_port = lsp_info->lsp_egress_info.out_port;
	    stHslVpnPwCfg.out_pw_exp = lsp_info->lsp_egress_info.out_label_exp;
	    stHslVpnPwCfg.out_pw_label = p_oam_info->out_pw_label;
	    stHslVpnPwCfg.vpn_port_attr = 0;
		stHslVpnPwCfg.out_disable = 0;
		stHslVpnPwCfg.lsp_id = oam_cfg->lsp_id;
				
		HSL_CHECK_RESULT(rc,hsl_bcm_vpn_pw_port_add(&stHslVpnPwCfg));
		//hsl_bcm_vpn_pw_port_add(&stHslVpnPwCfg);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"oam_vpn_pw_port_add successfully\n");
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_vpn_id      = %d\n",stHslVpnPwCfg.hsl_vpn_id);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"match_pw_label  = %d\n",stHslVpnPwCfg.match_pw_label);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"mpls_out_vlan   = %d\n",stHslVpnPwCfg.mpls_out_vlan);
				
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"out_port        = %d\n",stHslVpnPwCfg.out_port);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"out_pw_exp      = %d\n",stHslVpnPwCfg.out_pw_exp);
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"out_pw_label    = %d\n",stHslVpnPwCfg.out_pw_label);
				
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"lsp_id          = %d\n",stHslVpnPwCfg.lsp_id);

		return rc;
		
}


uint32 hsl_oam_del(ial_oam_cfg *oam_cfg)
{
    uint32 rc = 0;
    hsl_oam_grp_info  *p_oam_info=NULL;
    bcm_port_t bcm_port;
    hsl_bcm_ac_port_info  stHslVpnAcCfg;
    hsl_bcm_pw_port_info stHslVpnPwCfg;
    hsl_bcm_msg_vpn_cfg vpn_cfg;

    hsl_lsp_info *lsp_info = NULL;
    
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_del: Z L H test successful......!! \n");//test @zlh
    HSL_CHECK_PTR(rc, oam_cfg);
    HSL_CHECK_OAM_ID(rc, oam_cfg->oam_id);

    p_oam_info = &g_hsl_oam_group_info[oam_cfg->oam_id];

    if(p_oam_info->hsl_cfg == HSL_CFG_NO)
    {
      return DRV_E_OAM_NO_EXIST;
    }

    HSL_CHECK_RESULT(rc, bcm_oam_endpoint_destroy(0, p_oam_info->lmep_id));
    HSL_CHECK_RESULT(rc,bcm_oam_group_destroy(0, p_oam_info->ma_id));

    lsp_info = hsl_mpls_get_lsp_info(p_oam_info->lsp_id);
    if(lsp_info == NULL)
    {
        return DRV_E_LSP_NO_EXIST;
    }

    
    return rc;
}

uint32 hsl_oam_get_status(ial_oam_status *oam_status)
{
    uint32 rc =DRV_SUCCESS;
    hsl_oam_grp_info  *p_oam_info=NULL;
    
    HSL_CHECK_PTR(rc, oam_status);
    HSL_CHECK_OAM_ID(rc, oam_status->oam_id);

    hsl_oam_info_display(oam_status->oam_id);

	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_get_status: Z L H test successful......!! \n");//test @zlh
    p_oam_info = &g_hsl_oam_group_info[oam_status->oam_id];

    if(p_oam_info->hsl_cfg == HSL_CFG_NO)
    {
        return DRV_E_OAM_EXIST;
    }
    
    oam_status->lsp_id = p_oam_info->lsp_id;
    oam_status->ccm_timeout = p_oam_info->ccm_timeout;
    oam_status->rdi_error = p_oam_info->rdi_error;
    oam_status->unexpected_mep = p_oam_info->unexpected_mep;
    oam_status->xcon_error = p_oam_info->xcon_error;

//    hsl_oam_info_display(oam_status->oam_id);

   return rc;
}


void hsl_oam_evens_thread(void)
{
	int i = 0;
    uint32 rc = 0;
    uint32 oam_id;
    uint32 grp_id;
    hsl_oam_grp_info  *p_oam_info=NULL;
    ma_state_entry_t ma_state_entry;
    uint32   sw_lsp_flag = 0;
    uint32   sw_lsp_id;
    hsl_lsp_info *lsp_info = NULL;
    hsl_lsp_info *lsp_info2 = NULL;
    hsl_mpls_protect_group_info *hsl_pro_grp_info = NULL;
    ial_lsp_link_status lsp_ingress_status;
    ial_lsp_link_status lsp_egress_status;
    int link_status;
    uint32 sw_cnt;
    uint16 data;
	
    
    while(1)
    {
       	oss_sem_lock (OSS_SEM_BINARY, hsl_oam_evens_sem, OSS_WAIT_FOREVER);/* @zlh1024, OSS_WAIT_FOREVER参数表明lock一直在等待信号 */
        	
		//sal_sem_take(hsl_oam_evens_sem, 1000 * 100);		
        for(oam_id = 0; oam_id < HSL_OAM_MAX; oam_id++)
        {   
            p_oam_info = &g_hsl_oam_group_info[oam_id];
			
            if(p_oam_info->hsl_cfg == HSL_CFG_NO)
            {
                continue;
            }
			
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_1: oam_id = %d\n",p_oam_info->oam_id);//test @zlh
			/* 获取lsp信息 */
			lsp_info =  hsl_mpls_get_lsp_info(p_oam_info->lsp_id);//zlh@141212
            lsp_ingress_status = lsp_info->lsp_ingress_info.lsp_status;
            lsp_egress_status = lsp_info->lsp_egress_info.lsp_status;
			
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_2:  lsp_ingress_status   :%d\n",lsp_info->lsp_ingress_info.lsp_status);//test @zlh
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_3:  lsp_egress_status    :%d\n",lsp_info->lsp_egress_info.lsp_status );//test @zlh
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_4:  rdi_error            :%d\n",p_oam_info->rdi_error);//test @zlh
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_5:  ccm_timeout          :%d\n",p_oam_info->ccm_timeout);//test @zlh
			
			/* 在oam断的时候rdi_error和ccm_time应该都为1,lsp_status is down 
			   在oam恢复正常后lsp_status is up;
			*/
            if(p_oam_info->rdi_error != 0)
            {
                lsp_info->lsp_egress_info.lsp_status = IAL_LSP_STATUS_DOWN;
            }
            else
            {
                lsp_info->lsp_egress_info.lsp_status = IAL_LSP_STATUS_UP;
            }

            if((p_oam_info->ccm_timeout |p_oam_info->xcon_error | p_oam_info->unexpected_mep) != 0)
            {
                lsp_info->lsp_ingress_info.lsp_status = IAL_LSP_STATUS_DOWN;
            }
            else
            {
                lsp_info->lsp_ingress_info.lsp_status = IAL_LSP_STATUS_UP;
            }
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_6: lsp_info->lsp_ingress_info.lsp_status = %d\n",
																								lsp_info->lsp_ingress_info.lsp_status );//test @zlh
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_7: lsp_info->lsp_egress_info.lsp_status =  %d\n",
																								lsp_info->lsp_egress_info.lsp_status );//test @zlh
			
			/* 仅当lsp_ingress_status和lsp_egress_status都发生变化时继续执行*/
            if((lsp_ingress_status == lsp_info->lsp_ingress_info.lsp_status) 
                && (lsp_egress_status  == lsp_info->lsp_egress_info.lsp_status))
            {
                continue;
            }

            HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
                     "\n\rhsl_oam_evens_thread:oam_id = %d,lsp_id = %d, ingerss = %d, egress = %d\n ", 
                           oam_id, p_oam_info->lsp_id, lsp_info->lsp_ingress_info.lsp_status, lsp_info->lsp_egress_info.lsp_status);
			
			
			/* 查看oam_id绑定的lsp是否建立了protect group,并且获取protect grp info信息*/
            if(lsp_info->protect_status == HSL_CFG_YES)
            {
                hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[lsp_info->protect_grp_id]);
				
				HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_8: protect_grp_id   :%d\n",lsp_info->protect_grp_id);//test @zlh
				HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_9: grp_cfg          :%d\n",hsl_pro_grp_info->grp_cfg);//test @zlh
				HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_10: switch_status    :%d\n",hsl_pro_grp_info->switch_status);//test @zlh

                if((hsl_pro_grp_info->grp_cfg == HSL_CFG_NO) 
                    || (hsl_pro_grp_info->switch_status == IAL_PROTECT_SWITCH_FORCE))
                {
                    continue;
                }
				/* 判断当前oam_id对应的lsp_id是否为work_lsp_id */
				if(hsl_pro_grp_info->work_lsp_id == lsp_info->tunnel_id)
                {
                
					HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_11: tunnel_id = %d is oam_id current work_lsp_id \n",lsp_info->tunnel_id);//test @zlh
                    lsp_info2 = hsl_mpls_get_lsp_info(hsl_pro_grp_info->backup_lsp_id);
                    if(lsp_info2 == NULL)
                    {
                        continue;
                    } 
					/* 先|| 然后再& 再& 对应work lsp 如果之前status为down现在status都为up 则back_time_cn*/
                    if(((lsp_egress_status == IAL_LSP_STATUS_DOWN) || (lsp_ingress_status == IAL_LSP_STATUS_DOWN))
                        && (lsp_info->lsp_egress_info.lsp_status == IAL_LSP_STATUS_UP) 
                        && (lsp_info->lsp_ingress_info.lsp_status == IAL_LSP_STATUS_UP))
                    {
                        hsl_pro_grp_info->back_time_cnt = hsl_pro_grp_info->back_time;
						
						HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_12: Z L H back_time_cnt = %d\n",hsl_pro_grp_info->back_time);//test @zlh
                    }
                }
                else
                {/* 当前oam_id 对应lsp_id 不是work_lsp_id ,则将work_lsp_id 存在lsp_info2当中 */
                    lsp_info2 = hsl_mpls_get_lsp_info(hsl_pro_grp_info->work_lsp_id);
					HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_13: tunnel_id = %d not oam_id current work_lsp_id \n",lsp_info->tunnel_id);//test @zlh
                    if(lsp_info2 == NULL)
                    {
                        continue;
                    } 
                }

                sw_lsp_flag = 0;
                sw_lsp_id = 0;
    			/* 
    			zlh@141212 对于oam id 对应的lsp为work lsp，如果work_lsp is up backup_lsp is down 并且sw_lsp_id = work_lsp，则sw_lsp_flag = 1 触发倒换从backup侧向work侧倒换 
						   对于oam id 对应的lsp为back lsp，如果work_lsp is down backup_lsp is up 并且sw_lsp_id = work_lsp，则sw_lsp_flag = 1 触发倒换从work侧向backup侧倒换
				*/
                if(((lsp_info->lsp_ingress_info.lsp_status == IAL_LSP_STATUS_UP)
                    && (lsp_info->lsp_egress_info.lsp_status == IAL_LSP_STATUS_UP))//lsp_info 先&&
                    && ((lsp_info2->lsp_egress_info.lsp_status == IAL_LSP_STATUS_DOWN)//lsp_info2 先||
                    || (lsp_info2->lsp_ingress_info.lsp_status == IAL_LSP_STATUS_DOWN)))
				
                {
                    sw_lsp_id = lsp_info->tunnel_id;
					
						 sw_lsp_flag = 1;
					
				HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_14: sw_lsp_flag      :%d\n",sw_lsp_flag);//test @zlh
				HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_15: sw_lsp_id        :%d\n",sw_lsp_id);//test @zlh
                }
				/*  
				zlh@141212 对于oam id 对应的lsp为work lsp，如果work_lsp is down backup_lsp is up 则sw_lsp_flag = 1 触发倒换从work侧向backup侧倒换
						   对于oam id 对应的lsp为back lsp，如果work_lsp is up backup_lsp is down 则sw_lsp_flag = 1 触发倒换从back侧向work侧倒换
				*/
                if(((lsp_info->lsp_ingress_info.lsp_status == IAL_LSP_STATUS_DOWN)
                    ||  (lsp_info->lsp_egress_info.lsp_status == IAL_LSP_STATUS_DOWN))//先|| 然后再& 再&
                    && (lsp_info2->lsp_egress_info.lsp_status == IAL_LSP_STATUS_UP)
                    && (lsp_info2->lsp_ingress_info.lsp_status == IAL_LSP_STATUS_UP))
                {
                    sw_lsp_flag = 1;
                    sw_lsp_id = lsp_info2->tunnel_id;
				HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_16: sw_lsp_flag =   %d\n",sw_lsp_flag);//test @zlh
				HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_17: sw_lsp_id   =   %d\n",sw_lsp_id);//test @zlh
                }

                if(sw_lsp_flag == 1)
                {
                    rc = hsl_mpls_protect_group_switch_1by1(lsp_info->protect_grp_id, sw_lsp_id);
                    if(rc == DRV_SUCCESS)
                    {
                        hsl_pro_grp_info->current_lsp_id = sw_lsp_id;
                    }
                   
                      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
                                   "\n\rhsl_oam_evens_thread: switch  grp_id = %d, lsp_id = %d, err=%d\n", 
                                                                                                lsp_info->protect_grp_id,sw_lsp_id, rc);
                }
            }
        }

	   /* 当work lsp 和 backup lsp 都正常的情况下 切换到work lsp工作*/
       for(grp_id = 0; grp_id < HSL_MPLS_PROTECT_GROUP_MAX; grp_id++)
       {
            hsl_pro_grp_info= &(g_hsl_lsp_protect_grp_info[grp_id]);
			
            if(hsl_pro_grp_info->grp_cfg == HSL_CFG_NO)
            {
                continue;
            }	
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_evens_thread_18: lock_lsp_id =    %d\n",hsl_pro_grp_info->lock_lsp_id);//zlh@141212

			/* config swtich status 为 lock方式且current lsp不为lock lsp */
            if((hsl_pro_grp_info->switch_status != IAL_PROTECT_SWITCH_LOCK)
                || (hsl_pro_grp_info->sw_back_flag == IAL_PROTECT_SWITCH_BACK_DIS)
                ||(hsl_pro_grp_info->current_lsp_id == hsl_pro_grp_info->lock_lsp_id))
            {
                continue;
            }
            
			/* 获得lock lsp 的信息,lock lsp id = work lsp id */
            lsp_info = hsl_mpls_get_lsp_info(hsl_pro_grp_info->lock_lsp_id);
            if(lsp_info == NULL)
            {
                    continue;
            }
            
            if((lsp_info->lsp_egress_info.lsp_status == IAL_LSP_STATUS_DOWN)
                ||  (lsp_info->lsp_ingress_info.lsp_status == IAL_LSP_STATUS_DOWN))
            {
                    continue;
            }
			/* 当work lsp status 为up时*/
	
            if(hsl_pro_grp_info->back_time_cnt > 0)
            {
          
				sal_usleep(10000*100);//delay 1s
				
               	hsl_pro_grp_info->back_time_cnt -= hsl_pro_grp_info->back_time_cnt;
				
                  /* 触发切回到work lsp侧 */
                if(hsl_pro_grp_info->back_time_cnt == 0)
                {       
                    rc = hsl_mpls_protect_group_switch_1by1(grp_id, hsl_pro_grp_info->lock_lsp_id);
                    if(rc == DRV_SUCCESS)
                    {
                        hsl_pro_grp_info->current_lsp_id = hsl_pro_grp_info->lock_lsp_id;
                    }
          
                   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
                                   "\n\rhsl_oam_evens_thread: switch baeck grp_id = %d, lsp_id = %d, err=%d", 
                                                                                                grp_id, hsl_pro_grp_info->lock_lsp_id, rc);
                }
            }
       }
    }

}

/* zlh@140905 add events handler */
int hsl_oam_enevs_irq(int hsl_unit, 
                                                uint32 flags, 
                                                bcm_oam_event_type_t event_type, 
                                                bcm_oam_group_t group, 
                                                bcm_oam_endpoint_t endpoint, 

void *user_data
                                              )
{
	int group_valid = 0;
	int endpoint_valid = 0;
	const char *oam_event_string;
    hsl_oam_grp_info  *p_oam_info=NULL;
	int rc;
	uint32 oam_id;//zlh@141219
	hsl_lsp_info *lsp_info = NULL;//zlh@141219
	hsl_lsp_info *lsp_info_group = NULL;//zlh@141219
	bcm_oam_group_info_t group_info;//zlh@141222
   	bcm_oam_group_t group_index;//zlh@141222
	p_oam_info = &g_hsl_oam_group_info[group];
	lsp_info_group = hsl_mpls_get_lsp_info(p_oam_info->lsp_id);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_enevs_irq_1: group           = %d\n", group                  );//zlh@141212 test
	
	switch (event_type) {
	case  bcmOAMEventEndpointPortDown :
			oam_event_string = "Port down";
			endpoint_valid = 1;
			break;
	case  bcmOAMEventEndpointPortUp :
			oam_event_string = "Port up";
			endpoint_valid = 1;
			break;
	case  bcmOAMEventEndpointInterfaceDown :
			oam_event_string = "Interface down";
			endpoint_valid = 1;
			break;
	case bcmOAMEventEndpointInterfaceUp :
			oam_event_string = "Interface up";
			endpoint_valid = 1;
			break;
	case  bcmOAMEventGroupCCMxcon:
			oam_event_string = "CCM xcon";
			group_valid = 1;
			break;
	case  bcmOAMEventGroupCCMError:
			oam_event_string = "CCM error";
			group_valid = 1;
			break;
	case  bcmOAMEventGroupRemote:
			oam_event_string = "Some remote defect";
			group_valid = 1;
			endpoint_valid = 1;
			break;
	case  bcmOAMEventGroupCCMTimeout:
			oam_event_string = "Some CCM timeout";
			group_valid = 1;
			endpoint_valid = 1;
			break;
			
			default:
				oam_event_string = "Unknown event";
		}
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_enevs_irq: oam_event_string		= %s\n", oam_event_string);
		
		/* zlh@141222 get ma_state register status */
		for(oam_id = 0; oam_id < HSL_OAM_MAX; oam_id++)
		 {	 
			p_oam_info = &g_hsl_oam_group_info[oam_id];
			
			if(p_oam_info->hsl_cfg == HSL_CFG_NO)
			{
				 continue;
			}
			
			group_index = oam_id;
			
				
			lsp_info = hsl_mpls_get_lsp_info(p_oam_info->lsp_id);
			
			/* clear the sticky bit after reading, bit0=rdi bit1=timeout bit3=ccm error bit4=xcon */
			group_info.faults = 0;
			group_info.persistent_faults = 0;
		    group_info.clear_persistent_faults  = 0xf ; 
				
			HSL_CHECK_RESULT(rc,bcm_oam_group_get(BCM_UNIT, group_index,&group_info));
			
			/*  give all comnection  path send massge */
			if( event_type == 11 )//bcmOAMEventGroupRemote并且为连接状态
			{
				if(lsp_info->lsp_ingress_info.lsp_status  == IAL_LSP_STATUS_DOWN 
					&& lsp_info->lsp_egress_info.lsp_status == IAL_LSP_STATUS_DOWN 
					&& (group_info.faults & 0xf) != 2 )

				{
					//zlh@141217 通路连接sw_rdi置1,sleep 10ms后归0;
					HSL_CHECK_RESULT(rc,bcm_tr2x_oam_set_group_sw_rdi(BCM_UNIT, group_index, 1));
					sal_usleep (10000);
					HSL_CHECK_RESULT(rc,bcm_tr2x_oam_set_group_sw_rdi(BCM_UNIT, group_index, 0));
				}			
			}
			
			if((group_info.faults & 0xf) != 2)//断开后为group_info.faults & 0xf 等于2，此时置 cc_timeout、rdi_error为1
			{
				p_oam_info->ccm_timeout = 0;
				p_oam_info->rdi_error = 0;
			}
			else
			{
				p_oam_info->ccm_timeout = 1;
				p_oam_info->rdi_error = 1;
			}
			
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_enevs_irq_2:  group_info.id 	   			  = %d\n", group_info.id);
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_enevs_irq_3:  current_faults			      = %d\n", group_info.faults & 0xf);	
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_enevs_irq_4:  sticky_faults				 	  = %d\n", group_info.persistent_faults & 0xf);
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_enevs_irq_5: ccm_timeout 	                  = %d\n", p_oam_info->ccm_timeout);	
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_enevs_irq_6: rdi_error		                  = %d\n", p_oam_info->rdi_error);
		
			oss_sem_unlock (OSS_SEM_BINARY, hsl_oam_evens_sem);//@zlh1024 进行加1操作由0变为1，释放信号量，切换到线程thread
			
		}	
		
    return 0;
}

void hsl_oam_info_display(uint32 oam_id)
{
    uint32 oam_start;
    uint32 oam_end;
    hsl_oam_grp_info  *p_oam_info=NULL;
    uint32 i, j;

    oam_start = 0;
    oam_end = HSL_OAM_MAX -1;

   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_oam_info_display: Z L H test successful......!! \n");//test @zlh
   for(i = oam_start; i <= oam_end; i++)
   {
        p_oam_info = &g_hsl_oam_group_info[i];
        if(p_oam_info->hsl_cfg == HSL_CFG_NO)
        {
            continue;
        }
        
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r********oam info*****************\n\r");
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "oam id                              :%d\n\r", p_oam_info->oam_id);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "lsp id                              :%d\n\r",p_oam_info->lsp_id);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "ccm period                          :%d\n\r", p_oam_info->ccm_period);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "mep name                            :%d\n\r", p_oam_info->mep_name);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "ma name                             :%s\n\r", p_oam_info->ma_name);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "vlan id                             :%d\n\r", p_oam_info->vlan_id);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "in pw label                         :%x\n\r", p_oam_info->in_pw_label);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "out pw label                        :%x\n\r", p_oam_info->out_pw_label);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "in port                             :%d\n\r", p_oam_info->in_port);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "out port                            :%d\n\r", p_oam_info->out_port);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "ma id -- %d; lmep id -- %d; rmep id --%d\n\r", 
             p_oam_info->ma_id, p_oam_info->lmep_id, p_oam_info->rmep_id);

         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "rdi_error                           :%d\n\r", p_oam_info->rdi_error);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "xcon_error                          :%d\n\r", p_oam_info->xcon_error);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "ccm_timeout                         :%d\n\r", p_oam_info->ccm_timeout);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "unexpected_mep                      :%d\n\r", p_oam_info->unexpected_mep);


         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r");
   }

}




int hsl_bcm_l2_switch_disable(bcm_vlan_t l2_switch_vlan, int enable)
{
    bcm_vlan_control_vlan_t   vlan_control;
    int rc = 0;
    memset(&vlan_control, 0, sizeof(bcm_vlan_control_vlan_t));
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcml2_switch_disable_1: Z L H test successful......!! \n");//test @zlh
    
    HSL_CHECK_RESULT(rc, bcm_vlan_control_vlan_get(BCM_UNIT, l2_switch_vlan, &vlan_control));

    vlan_control.flags |= BCM_VLAN_UNKNOWN_UCAST_DROP 
		                      | BCM_VLAN_NON_UCAST_DROP
		                      | BCM_VLAN_LEARN_DISABLE
                                      | BCM_VLAN_L2_LOOKUP_DISABLE;

    HSL_CHECK_RESULT(rc,  bcm_vlan_control_vlan_set(BCM_UNIT, l2_switch_vlan, vlan_control));

   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcml2_switch_disable_2: Z L H test successful......!! \n");//test @zlh
    

}

int hsl_bcm_pvid_action_set(int port, int op,int pvid)
{
	int rv;
        bcm_vlan_action_set_t   action;            

	bcm_vlan_action_set_t_init(&action);
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_pvid_action_set: Z L H test successfully......!! \n");//test @zlh
    
	//action.new_outer_vlan = 1;
	
	if( op == 0 )//不添加PVID到报文中,for tmpls
	{
		action.ut_outer = bcmVlanActionNone;
		
	}
	else  //添加PVID到报文中,删除恢复 
    {
		action.ut_outer = bcmVlanActionAdd;
	}   

	//Set the port's default vlan tag actions
     rv = bcm_vlan_port_default_action_set(BCM_UNIT, port, &action);


    int flags,learn;
    learn = op;
    flags = BCM_PORT_LEARN_FWD;
    flags |= (learn ? BCM_PORT_LEARN_ARL : 0);
    rv = bcm_port_learn_set( BCM_UNIT, port, flags );   
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_bcm_pvid_action_set_2: action is successfully....!! \n");//test @zlh
    return rv;
	
}

int hsl_svp_in_disable(int if_class_id)
{
    int rc = 0;
    bcm_field_qset_t qset;
    bcm_field_group_t group;
    bcm_field_entry_t entry;

    BCM_FIELD_QSET_INIT (qset);
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_svp_in_disable: Z L H test successful......!! \n");//test @zlh

    HSL_CHECK_RESULT(rc, bcmx_field_group_create(qset, BCM_FIELD_GROUP_PRIO_ANY,&group));

    BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyStageIngress);
    BCM_FIELD_QSET_ADD (qset, bcmFieldQualifyInterfaceClassL3);

    HSL_CHECK_RESULT(rc,bcmx_field_group_set (group, qset));
    HSL_CHECK_RESULT(rc,bcmx_field_entry_create (group, &entry));

    HSL_CHECK_RESULT(rc,bcm_field_qualify_InterfaceClassL3(BCM_UNIT,    entry, if_class_id, 0xfff));

    HSL_CHECK_RESULT(rc,bcm_field_action_add(BCM_UNIT, entry, bcmFieldActionDrop,  0, 0));

    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_svp_in_disable: class id = %d; entry =%d;\n",if_class_id, entry);	

     return rc;
}
int hsl_eport_get_mac_addr_by_bcmport(uint8 bcm_port, bcm_mac_t *mac_addr)
{
    int bcm_error = 0;
	
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_eport_get_mac_addr_by_bcmport: Z L H test successful......!! \n");//test @zlh
    
    if(bcm_port <=1 || bcm_port > HSL_BCM_MAX_PORT)
    {
        return -1;
    }

    HSL_CHECK_PTR(bcm_error, mac_addr);

    memcpy(mac_addr, g_PortSrcMac[bcm_port].mac_addr, sizeof(bcm_mac_t));
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_eport_get_mac_addr_by_bcmport: port_id = %d\n",bcm_port);//test @zlh0925
	HSL_LOG (HSL_LOG_MSG, HSL_LEVEL_INFO,"hsl_eport_get_mac_addr_by_bcmport: mac_addr = %x.%x.%x.%x.%x.%x\n",  
                g_PortSrcMac[bcm_port].mac_addr[0],
               	g_PortSrcMac[bcm_port].mac_addr[1],
                g_PortSrcMac[bcm_port].mac_addr[2],
                g_PortSrcMac[bcm_port].mac_addr[3],
                g_PortSrcMac[bcm_port].mac_addr[4],
                g_PortSrcMac[bcm_port].mac_addr[5]);

    return bcm_error;
}

void show_lsp_info( uint32 lsp_id)
{
    
    hsl_lsp_info   *hsl_lsp_info = NULL;
    uint32 lsp_start, lsp_end;
    T_LIST_NODE *pListNode = NULL;
     hsl_bcm_pw_port_info *p_hsl_l2vpn_port_info = NULL;
	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "show_lsp_info: Z L H test successful......!! \n");//test @zlh
       
    int i,j;
    
    if(lsp_id >= HSL_MPLS_LSP_MAX)
    {
         lsp_start = 0;
         lsp_end = HSL_MPLS_LSP_MAX -1;
    }
    else
    {
          lsp_start = lsp_id;
           lsp_end = lsp_id;
    }
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "show_lsp_info_1: lsp_id = %d \n",lsp_id);//test @zlh

    for(i = lsp_start; i <= lsp_end; i++)
    {
         hsl_lsp_info = hsl_mpls_get_lsp_info(i);
        
        if(hsl_lsp_info == NULL)
        {
            continue;
        }
        
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
            "\n\r********mpls lsp info**********************\n\r");

        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
                 "tunnel id                    :%d\n\r", hsl_lsp_info->tunnel_id);
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
                 "lsp  cfg                       :%d\n\r", hsl_lsp_info->lsp_cfg);
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "node type                    :%d (pe ---%d; p---%d)\n\r",
             hsl_lsp_info->lsp_node_type, HSL_LSP_NODE_TYPE_PE, HSL_LSP_NODE_TYPE_P);
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, 
            "protect cfg                      :%d\n\r", hsl_lsp_info->protect_status);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "protect grp id                   :%d\n\r", hsl_lsp_info->protect_grp_id);
         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r");

        if(hsl_lsp_info->lsp_ingress_info.hsl_cfg == HSL_CFG_YES)
        {
           HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "***lsp ingress info ****\n\r");
           
           HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    ingress cfg                   :%d \n\r", 
                                                                              hsl_lsp_info->lsp_ingress_info.hsl_cfg);
           
           HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    ingress status                :%d INVALID--%d, down--%d, up--%d\n\r", 
                                                           hsl_lsp_info->lsp_ingress_info.lsp_status,
                                                           IAL_LSP_STATUS_INVALID,
                                                           IAL_LSP_STATUS_DOWN,
                                                           IAL_LSP_STATUS_UP);
            
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    in port                     :%d\n\r", hsl_lsp_info->lsp_ingress_info.in_port);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    in label                    :%d\n\r", hsl_lsp_info->lsp_ingress_info.in_label);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    in vlan                     :%d\n\r", hsl_lsp_info->lsp_ingress_info.in_vlan);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    mpls entry                  :%d\n\r", hsl_lsp_info->lsp_ingress_info.mpls_entry_index);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    egr l3 next hop             :%d\n\r", hsl_lsp_info->lsp_ingress_info.egr_l3_next_hop);
        //    printk("    qos mode                     :%d invalid-%d, uniform-%d, pipe-%d, spipe-%d\n\r", 
        //                                                                    hsl_lsp_info->lsp_ingress_info.qos_mode, 
        //                                                                    IAL_QOS_MOD_INVALID, IAL_QOS_MOD_UNIFORM, 
        //                                                                    IAL_QOS_MOD_PIPE, IAL_QOS_MOD_SHORT_PIPE);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    ing exp map id              :%d\n\r", hsl_lsp_info->lsp_ingress_info.ing_phb_id);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    in mac                      :%x:%x:%x:%x:%x:%x\n\r", 
            hsl_lsp_info->lsp_ingress_info.src_mac[0], hsl_lsp_info->lsp_ingress_info.src_mac[1],
            hsl_lsp_info->lsp_ingress_info.src_mac[2], hsl_lsp_info->lsp_ingress_info.src_mac[3],
            hsl_lsp_info->lsp_ingress_info.src_mac[4], hsl_lsp_info->lsp_ingress_info.src_mac[5]);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r");
        }

        if(hsl_lsp_info->lsp_egress_info.hsl_cfg == HSL_CFG_YES)
        {
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "***lsp egress info ****\n\r");
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    egress cfg                  :%d \n\r", 
                                                                              hsl_lsp_info->lsp_egress_info.hsl_cfg);
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    egress status               :%d INVALID--%d, down--%d, up--%d\n\r", 
                                                           hsl_lsp_info->lsp_egress_info.lsp_status,
                                                           IAL_LSP_STATUS_INVALID,
                                                           IAL_LSP_STATUS_DOWN,
                                                           IAL_LSP_STATUS_UP);
            
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    out port                     :%d\n\r", hsl_lsp_info->lsp_egress_info.out_port);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    out label                    :%d\n\r", hsl_lsp_info->lsp_egress_info.out_label);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    out label exp                :%d\n\r", hsl_lsp_info->lsp_egress_info.out_label_exp);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    out vlan                     :%d\n\r", hsl_lsp_info->lsp_egress_info.out_vlan);
         //   printk("    qos mode                   :%d invalid-%d, uniform-%d, pipe-%d, spipe-%d\n\r", 
        //                                                                    hsl_lsp_info->lsp_egress_info.qos_mode, 
       //                                                                     IAL_QOS_MOD_INVALID, IAL_QOS_MOD_UNIFORM, 
       //                                                                     IAL_QOS_MOD_PIPE, IAL_QOS_MOD_SHORT_PIPE);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    egr exp map id               :%d\n\r", hsl_lsp_info->lsp_egress_info.egr_phb_id);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    egr intf                     :%d\n\r", hsl_lsp_info->lsp_egress_info.egr_intf);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    l3 intf                      :%d\n\r", hsl_lsp_info->lsp_egress_info.l3_intf);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    egr mac da index             :%d\n\r", hsl_lsp_info->lsp_egress_info.egr_mac_da_index);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    egr l3 intf index            :%d\n\r", hsl_lsp_info->lsp_egress_info.egr_l3_intf_index);
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    des mac                      :%x:%x:%x:%x:%x:%x\n\r", 
                    hsl_lsp_info->lsp_egress_info.des_mac[0], hsl_lsp_info->lsp_egress_info.des_mac[1],
                    hsl_lsp_info->lsp_egress_info.des_mac[2],hsl_lsp_info->lsp_egress_info.des_mac[3],
                    hsl_lsp_info->lsp_egress_info.des_mac[4],hsl_lsp_info->lsp_egress_info.des_mac[5]);
            
             HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    bind pw info: cnt = %d\n\r", hsl_lsp_info->lsp_egress_info.pw_cnt);
          
            pListNode = hsl_lsp_info->lsp_egress_info.bind_pw_list->pHead;
            j = 0;
            while(pListNode != NULL)
            {
                p_hsl_l2vpn_port_info = (hsl_bcm_pw_port_info *)pListNode->data;

                 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "    0x%x", p_hsl_l2vpn_port_info->mpls_port_id);
                j++;
                if((j % 16) == 0)
                {
                       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r");
                

                pListNode = pListNode->next;
            }
            
        }

         HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "\n\r\n\r");
    }

   }
}

hsl_qos_phb   g_hsl_qos_phb_tab;
/***************************************************************
* Function: hsl_qos_init
* Description: qos module init
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_qos_init(void)
{
    uint32 bcm_port;
    int inter_pri;
    uint32 tab_index;
   // uint32 rc = DRV_SUCCESS;
	uint32 rc = 0;
   int flags;
    memset(&g_hsl_qos_phb_tab, 0, sizeof(hsl_qos_phb));

   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_init_1:zlh test function  successful!!!\n");//zlh@141214 test
    
   HSL_CHECK_RESULT(rc,  bcm_cosq_config_set(BCM_UNIT, 8));
   
  /*zlh@150108 enable discard */
   flags = BCM_COSQ_DISCARD_ENABLE;
   HSL_CHECK_RESULT(rc,bcm_cosq_discard_set(BCM_UNIT, flags));
   
   /* 在每一个port上，设置inter_pri与cos_q为一一对应关系 zlh@141224 */
    for(inter_pri = 7;  inter_pri >= 0; inter_pri--)
    {
        for(bcm_port = 1;  bcm_port <= BCM_MAX_PORT_NUM; bcm_port++)
        
        HSL_CHECK_RESULT(rc, hsl_qos_interpri_to_cosq(bcm_port, inter_pri, inter_pri));
    }

    /* untag pri */
    for(bcm_port = 1; bcm_port <= BCM_MAX_PORT_NUM; bcm_port++)
    {
        HSL_CHECK_RESULT(rc,bcm_port_untagged_priority_set(BCM_UNIT, bcm_port, 0));
    }
	/* 初始化设置phb_id为0的table,并且存在本地的array当中，array序号与inter_pri一一对应 */
   ial_qos_in_phb_cfg pri_cfg;
   for(tab_index = 0;  tab_index < 1; tab_index++)
   {
        pri_cfg.phb_id = tab_index;
        for(inter_pri = 0; inter_pri <= 7; inter_pri++)
        {
            pri_cfg.inter_pri[inter_pri] = inter_pri;
            pri_cfg.color[inter_pri] = 0;			
        }
        
        HSL_CHECK_RESULT(rc,hsl_qos_pri_to_inter_tab_add(&pri_cfg));
   }

    ial_qos_in_phb_cfg  exp_cfg;
    for(tab_index = 0;  tab_index < 1; tab_index++)
    {
         exp_cfg.phb_id = tab_index;
         for(inter_pri = 0; inter_pri <= 7; inter_pri++)
         {
             exp_cfg.inter_pri[inter_pri] = inter_pri;	
             exp_cfg.color[inter_pri] = (inter_pri % 3);
         }
         
         HSL_CHECK_RESULT(rc,hsl_qos_exp_to_inter_tab_add(&exp_cfg));
    }

     ial_qos_out_phb_cfg exp_out_phb_cfg;
    for(tab_index = 0; tab_index < 1; tab_index++)
    {
        for(inter_pri =0; inter_pri < IAL_QOS_EXP_MAX; inter_pri++)
        {
            exp_out_phb_cfg.user_pri[IAL_QOS_COLOR_GREEN][inter_pri] = inter_pri;
            exp_out_phb_cfg.user_pri[IAL_QOS_COLOR_YELLOW][inter_pri] =  inter_pri;
            exp_out_phb_cfg.user_pri[IAL_QOS_COLOR_RED][inter_pri] =  inter_pri;
        }

        exp_out_phb_cfg.phb_id = tab_index;
        HSL_CHECK_RESULT(rc, hsl_qos_inter_to_exp_tab_add(&exp_out_phb_cfg));
    }
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_init_2:tab_index = %d \n",tab_index);//zlh@141214 test
    ial_qos_wred_drop_cfg    qos_wred_drop_cfg;
    ial_qos_tail_drop_cfg    qos_tail_drop;
    ial_qos_port_sched_cfg   sched_cfg;
 
    //for(bcm_port = 2; bcm_port <= 29; bcm_port++)	
    for(bcm_port = 2; bcm_port <= 30; bcm_port++)//zlh@141229
    {
        sched_cfg.logic_port = bcm_port - 1;
        //sched_cfg.sched_mode = IAL_QOS_PORT_SCHED_MOD_SP;
        sched_cfg.sched_mode = IAL_QOS_PORT_SCHED_MOD_WRR;
		
		//zlh@141229 init wrr_weight
		sched_cfg.wrr_weight[0] = 1;
		sched_cfg.wrr_weight[1] = 1;
		sched_cfg.wrr_weight[2] = 1;
		sched_cfg.wrr_weight[3] = 1;
		sched_cfg.wrr_weight[4] = 1;
		sched_cfg.wrr_weight[5] = 1;
		sched_cfg.wrr_weight[6] = 1;
		sched_cfg.wrr_weight[7] = 0;
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_init_3:logic_port = %d \n",sched_cfg.logic_port);//zlh@141214 test
        HSL_CHECK_RESULT(rc,  hsl_qos_port_sched_set(&sched_cfg));
	
        qos_wred_drop_cfg.logic_port = bcm_port - 1;
        qos_tail_drop.logic_port     = bcm_port - 1;
		
        
        for(inter_pri = 0; inter_pri < 8; inter_pri ++)
        {
            qos_tail_drop.all_limit = 0;
            qos_tail_drop.cos_num = inter_pri;
            qos_tail_drop.yellow_limit = 0;
            qos_tail_drop.red_limit      = 0;
            //HSL_CHECK_RESULT(rc, hsl_qos_set_tail_drop(&qos_tail_drop));//zlh@141213,init不做处理
        }
        
    }
   
   HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_init_3:zlh test function  successful!!!\n");//zlh@141214 test
    return rc;
}


/***************************************************************
* Function: hsl_qos_interpri_to_cosq
* Description: 内部优先级到出端口队列映射
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_qos_interpri_to_cosq(uint32 port, uint32 pri, uint32 cos_q)
{
    uint32 rc = 0;

	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_interpri_to_cosq_1: port    =%d\n",port);//zlh@141214 test
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_interpri_to_cosq_2: pri     =%d\n",pri );//zlh@141214 test

	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_interpri_to_cosq_3: cos_q   =%d\n",cos_q);//zlh@141214 test

    if((port > BCM_MAX_PORT_NUM) || (pri >= 8) || (cos_q >= 8))
    {
        return BCM_E_PARAM;  
    }

     HSL_CHECK_RESULT(rc, bcm_cosq_port_mapping_set(BCM_UNIT, port, pri, cos_q));
	 
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_interpri_to_cosq_4: cos_q	 =%d\n",cos_q);//zlh@141214 test
     return rc;
}

/* add 140827@zlh */
/***************************************************************
* Function: hsl_qos_port_sched_set
* Description: 配置端口调度模式，sp，wrr，dwrr
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/

uint32 hsl_qos_port_sched_set(ial_qos_port_sched_cfg  *sched_cfg)
{

    bcm_pbmp_t  port_bmap;
    uint32 rc = 0;
    bcm_port_t bcm_port;
    int bcm_sched_mod;
	uint32 wrr_weight[8];

    HSL_CHECK_PTR(rc,sched_cfg);

	bcm_sched_mod = BCM_COSQ_DEFICIT_ROUND_ROBIN;//zlh@150109
	bcm_port = sched_cfg->logic_port;//zlh@141229 
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_port_sched_set_1: bcm_port          = %d\n",bcm_port);//zlh@1214 test

	/* cosq 7 is sp priority the rest is wrr with equal weight  */	
		wrr_weight[0] = sched_cfg->wrr_weight[0];
		wrr_weight[1] = sched_cfg->wrr_weight[1];
		wrr_weight[2] = sched_cfg->wrr_weight[2];
		wrr_weight[3] = sched_cfg->wrr_weight[3];
		wrr_weight[4] = sched_cfg->wrr_weight[4];
		wrr_weight[5] = sched_cfg->wrr_weight[5];
		wrr_weight[6] = sched_cfg->wrr_weight[6];
		wrr_weight[7] = sched_cfg->wrr_weight[7];

    memset(&port_bmap, 0, sizeof(bcm_pbmp_t));
	
    BCM_PBMP_PORT_SET(port_bmap, bcm_port);//zl@141229

	HSL_CHECK_RESULT(rc, bcm_cosq_port_sched_set(0, port_bmap,  bcm_sched_mod, wrr_weight,  0));
	 
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_port_sched_set_2: bcm_sched_mod	 = %d\n",bcm_sched_mod);//zlh@1214 test
	 
    return rc;
}

/***************************************************************
* Function: hsl_qos_pri_to_inter_tabl_add
* Description: 创建vlan pri 到internal pri 映射
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_qos_pri_to_inter_tab_add(ial_qos_in_phb_cfg *phb_cfg)
{
    uint32 rc = 0;
    hsl_qos_inphb_info *pri_inphb_info = NULL;
    bcm_qos_map_t     bcm_map;
    int32 bcm_map_id;
    uint32 flag = 0;
    uint32 use_pri = 0;
    uint32 cfi;
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_pri_to_inter_tab_add_1: z l h test hsl_qos_pri_to_inter_tab_add function successfully!!!\n");//zlh@141214 test
	
    HSL_CHECK_PTR(rc,phb_cfg);

    if(phb_cfg->phb_id > HSL_ING_PRI_CNG_MAP_NUM)
    {
        return DRV_E_QOS_PHB_ID_INVALID;
    }

    pri_inphb_info = &(g_hsl_qos_phb_tab.hsl_qos_in_pri_tab[phb_cfg->phb_id]);

    flag =  BCM_QOS_MAP_L2 | BCM_QOS_MAP_INGRESS;

    if(pri_inphb_info->bcm_map_id == 0)
    {
        HSL_CHECK_RESULT(rc, bcm_qos_map_create(BCM_UNIT,  flag, &bcm_map_id));
        pri_inphb_info->bcm_map_id = bcm_map_id;
    }
    else
    {
        flag |=  BCM_QOS_MAP_REPLACE;
        bcm_map_id = pri_inphb_info->bcm_map_id;
    }

    pri_inphb_info->hsl_cfg = HSL_CFG_YES;
	
     
    for(cfi = 0; cfi <= 1; cfi++)
    {
        for(use_pri = 0; use_pri <= 7; use_pri++)
        {
            bcm_qos_map_t_init(&bcm_map);

            bcm_map.pkt_pri = use_pri;
            bcm_map.pkt_cfi   = cfi;
            bcm_map.int_pri = phb_cfg->inter_pri[use_pri];

            HSL_CHECK_RESULT(rc, hsl_qos_to_bcm_color(phb_cfg->color[use_pri],  &(bcm_map.color)));
			
            HSL_CHECK_RESULT(rc, bcm_qos_map_add(BCM_UNIT, flag, &bcm_map, bcm_map_id));

            pri_inphb_info->color[use_pri] = phb_cfg->color[use_pri];
            pri_inphb_info->inter_pri[use_pri] = phb_cfg->inter_pri[use_pri];
        }
    }
     
	 HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_qos_pri_to_inter_tab_add_2: bcm map id = 0x%x, index = %d\n", bcm_map_id, phb_cfg->phb_id); 
	 
     return rc;
}


/***************************************************************
* Function: hsl_qos_pri_to_inter_tabl_del
* Description: 删除vlan pri 到internal pri 映射
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_qos_pri_to_inter_tab_del(ial_qos_in_phb_cfg *phb_cfg)
{
     uint32 rc = 0;
     hsl_qos_inphb_info *pri_inphb_info = NULL;
     
     HSL_CHECK_PTR(rc,phb_cfg);

    if(phb_cfg->phb_id > HSL_ING_PRI_CNG_MAP_NUM)
    {
          return DRV_E_QOS_PHB_ID_INVALID;
    }
    
    pri_inphb_info = &(g_hsl_qos_phb_tab.hsl_qos_in_pri_tab[phb_cfg->phb_id]);
    
    pri_inphb_info->hsl_cfg = HSL_CFG_NO;
     
     return rc;
}

/***************************************************************
* Function: hsl_qos_exp_to_inter_tabl_add
* Description: 创建mpls exp 到internal pri 映射
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_qos_exp_to_inter_tab_add(ial_qos_in_phb_cfg *phb_cfg)
{
     uint32 rc = 0;
     hsl_qos_inphb_info *pri_inphb_info = NULL;
     bcm_mpls_exp_map_t     bcm_exp_map;
     int32 bcm_map_id;
     uint32 flag = 0;
     uint32 exp_v = 0;
	 
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_exp_to_inter_tab_add_1:z l h test hsl_qos_exp_to_inter_tab_add function successfully!!!\n");//zlh@1214 test
    HSL_CHECK_PTR(rc,phb_cfg);

    if(phb_cfg->phb_id > HSL_ING_EXP_CNG_MAP_NUM)
    {
        return DRV_E_QOS_PHB_ID_INVALID;
    }

    pri_inphb_info = &(g_hsl_qos_phb_tab.hsl_qos_in_exp_tab[phb_cfg->phb_id]);

    flag =  BCM_MPLS_EXP_MAP_INGRESS;

    if(pri_inphb_info->bcm_map_id == 0)
    {
        HSL_CHECK_RESULT(rc, bcm_mpls_exp_map_create(BCM_UNIT,  flag, &bcm_map_id));
        pri_inphb_info->bcm_map_id = bcm_map_id;
    }
    else
    {
        bcm_map_id = pri_inphb_info->bcm_map_id;
    }

    pri_inphb_info->hsl_cfg = HSL_CFG_YES;

    for(exp_v = IAL_QOS_EXP_0; exp_v < IAL_QOS_EXP_MAX; exp_v++)
    {
        bcm_mpls_exp_map_t_init(&bcm_exp_map);
        bcm_exp_map.exp = exp_v;
        bcm_exp_map.priority = phb_cfg->inter_pri[exp_v];
        HSL_CHECK_RESULT(rc, hsl_qos_to_bcm_color(phb_cfg->color[exp_v],  &(bcm_exp_map.color)));
        
        HSL_CHECK_RESULT(rc, bcm_mpls_exp_map_set(BCM_UNIT, bcm_map_id, &bcm_exp_map));
        
        pri_inphb_info->color[exp_v] = phb_cfg->color[exp_v];
        pri_inphb_info->inter_pri[exp_v] = phb_cfg->inter_pri[exp_v];
    }
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_qos_exp_to_inter_tab_add_2: bcm map id = 0x%x, index = %d\n", bcm_map_id, phb_cfg->phb_id); 
     return rc;
}


/***************************************************************
* Function: hsl_qos_exp_to_inter_tab_del
* Description: 删除mpls exp 到internal pri 映射
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_qos_exp_to_inter_tab_del(ial_qos_in_phb_cfg *phb_cfg)
{
    uint32 rc = 0;
    hsl_qos_inphb_info *pri_inphb_info = NULL;

    HSL_CHECK_PTR(rc,phb_cfg);
    
    if(phb_cfg->phb_id > HSL_ING_EXP_CNG_MAP_NUM)
    {
        return DRV_E_QOS_PHB_ID_INVALID;
    }

    pri_inphb_info = &(g_hsl_qos_phb_tab.hsl_qos_in_exp_tab[phb_cfg->phb_id]);
    pri_inphb_info->hsl_cfg = HSL_CFG_NO;

    return rc;
}

/***************************************************************
* Function: hsl_qos_inter_to_exp_tab_add
* Description: 创建internal pri 到mpls exp 的映射
* Return:
*     DRV_OK:            
*    
* Others:
***************************************************************/
uint32 hsl_qos_inter_to_exp_tab_add(ial_qos_out_phb_cfg *out_phb_cfg)
{
     uint32 rc = 0;
     hsl_qos_outphb_info *pri_outphb_info = NULL;
     bcm_mpls_exp_map_t     bcm_exp_map;
     int32 bcm_map_id;
     uint32 flag = 0;
     uint32 color = 0;
     uint32 inter_pri = 0;
    
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_inter_to_exp_tab_add_1: z l h test hsl_qos_inter_to_exp_tab_add  function succssfully!!!\n");//zlh@1214 test
    HSL_CHECK_PTR(rc,out_phb_cfg);

    if(out_phb_cfg->phb_id > HSL_EGR_MPLS_EXP_MAP_NUM)
    {
        return DRV_E_QOS_PHB_ID_INVALID;
    }

    pri_outphb_info = &(g_hsl_qos_phb_tab.hsl_qos_out_exp_tab[out_phb_cfg->phb_id]);

    flag =  BCM_MPLS_EXP_MAP_EGRESS;

    if(pri_outphb_info->bcm_map_id == 0)
    {
        HSL_CHECK_RESULT(rc, bcm_mpls_exp_map_create(BCM_UNIT,  flag, &bcm_map_id));
    }
    else
    {
        bcm_map_id = pri_outphb_info->bcm_map_id;
    }
   
    pri_outphb_info->hsl_cfg = HSL_CFG_YES;

     for(inter_pri = 0;  inter_pri < IAL_QOS_EXP_MAX; inter_pri++)
    {
         for(color = 0; color < IAL_QOS_COLOR_MAX; color++)
        {
            bcm_mpls_exp_map_t_init(&bcm_exp_map);

            bcm_exp_map.priority = inter_pri;
            HSL_CHECK_RESULT(rc, hsl_qos_to_bcm_color(color,  &(bcm_exp_map.color)));
            bcm_exp_map.exp = out_phb_cfg->user_pri[color][inter_pri];
            
            HSL_CHECK_RESULT(rc, bcm_mpls_exp_map_set(BCM_UNIT, bcm_map_id, &bcm_exp_map));//设置in_pri到exp优先级的映射关系
            pri_outphb_info->pkt_pri[color][inter_pri]= out_phb_cfg->user_pri[color][inter_pri];
        }
    }
	 
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO,"hsl_qos_inter_to_exp_tab_add_2: bcm map id = 0x%x, index = %d", bcm_map_id, out_phb_cfg->phb_id); 

	return rc;
}

/***************************************************************
* Function: hsl_qos_inter_to_exp_tab_del
* Description: 删除internal pri 到mpls exp 的映射
* Return:
*     DRV_OK:            
*    
* Others:
***************************************************************/
uint32 hsl_qos_inter_to_exp_tab_del(ial_qos_out_phb_cfg *out_phb_cfg)
{
    uint32 rc = 0;
    hsl_qos_outphb_info *exp_outphb_info = NULL;

    HSL_CHECK_PTR(rc,out_phb_cfg);

    if(out_phb_cfg->phb_id > HSL_EGR_MPLS_EXP_MAP_NUM)
    {
        return DRV_E_QOS_PHB_ID_INVALID;
    }

    exp_outphb_info = &(g_hsl_qos_phb_tab.hsl_qos_out_exp_tab[out_phb_cfg->phb_id]);
    exp_outphb_info->hsl_cfg = HSL_CFG_NO;

    return rc;
}
/***************************************************************
* Function: hsl_qos_to_bcm_color
* Description: 转换为bcm color
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
uint32 hsl_qos_to_bcm_color(uint32 color, bcm_color_t *bcm_color)
{
    uint32 rc =0;
    
    HSL_CHECK_PTR(rc,bcm_color);
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_to_bcm_color_1:bcm_color = %d\n",color);//zlh@1214 test

    switch(color)
    {
        case IAL_QOS_COLOR_GREEN:
        {
            *bcm_color = bcmColorGreen;
            break;
        }            
        case IAL_QOS_COLOR_YELLOW:
        {
            *bcm_color = bcmColorYellow;
            break;
        }
        case IAL_QOS_COLOR_RED:
        {
            *bcm_color = bcmColorRed;
            break;
        }

        default:
        {
            return DRV_E_QOS_COLOR_INVALID;
        }
    }
    return rc;
}


/***************************************************************
* Function: hsl_qos_get_bcm_map_id
* Description: 根据phb_id 获取bcm map id
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
	uint32 hsl_qos_get_bcm_map_id(uint8 phb_id, hsl_qos_type qos_type,	uint32 *bcm_map_id)
	{
		uint32 rc = 0;
		hsl_qos_inphb_info		  *inphb_info = NULL;
		hsl_qos_outphb_info 	 *outphb_info = NULL; 
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_get_bcm_map_id_1:qos_type = %d\n",qos_type);//zlh@1214 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_qos_get_bcm_map_id_2:*bcm_map_id = %d\n",*bcm_map_id);//zlh@1214 test
			
		HSL_CHECK_PTR(rc,bcm_map_id);	
		
		switch(qos_type)			
		{
			case IAL_QOS_TYPE_PRI_TO_INTER: 			
			{  
				if(phb_id > HSL_ING_PRI_CNG_MAP_NUM)
				{
					return DRV_E_QOS_PHB_ID_INVALID;
				}
	
				inphb_info = &(g_hsl_qos_phb_tab.hsl_qos_in_pri_tab[phb_id]);	
				
				break;
			}
				
			
			case IAL_QOS_TYPE_EXP_TO_INTER: 			
			{
				if(phb_id > HSL_ING_EXP_CNG_MAP_NUM)
				{
					return DRV_E_QOS_PHB_ID_INVALID;
				}
	
				inphb_info = &(g_hsl_qos_phb_tab.hsl_qos_in_exp_tab[phb_id]);
	
				break;
			}
			
			case IAL_QOS_TYPE_INTER_TO_EXP: 
			case IAL_QOS_TYPE_INTER_TO_PRI:
			{
				if(phb_id > HSL_EGR_MPLS_EXP_MAP_NUM)
				{
					return DRV_E_QOS_PHB_ID_INVALID;
				}
	
				
				outphb_info = &(g_hsl_qos_phb_tab.hsl_qos_out_exp_tab[phb_id]);
				
				
				break;
			}
	
			default:
			{
				return DRV_E_QOS_PHB_ID_INVALID;
			}
	
		
		}
	
		if(inphb_info != NULL)
		{
			if(inphb_info->hsl_cfg == HSL_CFG_YES)
			{
			   *bcm_map_id = inphb_info->bcm_map_id;  
			}
			else 
			{
				rc = DRV_E_QOS_PHB_ID_NO_EXIST;
			}
		}
		else if(outphb_info != NULL) 
		{
			if(outphb_info->hsl_cfg == HSL_CFG_YES)
			{
				if(qos_type == IAL_QOS_TYPE_INTER_TO_PRI)
				{
				   *bcm_map_id = (outphb_info->bcm_map_id & 0xff)  | (2 << 10);//zlh@141224
				}
				else
				{
					*bcm_map_id = outphb_info->bcm_map_id;
				}
				
			}
			else 
			{
				rc = DRV_E_QOS_PHB_ID_NO_EXIST;
			}
		}
		else
		{
			rc = DRV_E_QOS_PHB_ID_INVALID;
		}
	
		return rc;
		
	}


