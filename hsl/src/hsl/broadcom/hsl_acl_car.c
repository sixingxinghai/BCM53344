/*************************************************************************
文件名   :
描述        : 
版本        : 1.0
日期        : 2012
作者        : 
修改历史    :
**************************************************************************/
	
#include "config.h"
#include "hsl_os.h"
		
#include "hsl_types.h"
																						
#include "bcm_incl.h"
																						
		/* HAL includes. */
#include "hal_netlink.h"
#include "hal_msg.h"
		
#include "hsl_avl.h"
#include "hsl.h"
#include "hsl_oss.h"
#include "hsl_comm.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_msg.h"
#include "hsl_tlv.h"
#include "hsl_ether.h"
#include "hsl_error.h"
		
#ifdef HAVE_L2
#include "hsl_vlan.h"
#include "hsl_bridge.h"
#include "bcm_incl.h"
#include "hsl_bcm_vlanclassifier.h"
#include "hsl_bcm_l2.h"
#include "hsl_bcm_lacp.h"
#endif /* HAVE_L2 */
		
		//#ifdef HAVE_VPWS	 /*cai 2011-10 vpws module*/
#include "hsl_bcm_vpws.h"
		//#endif /* HAVE_VPWS*/
		
#include "hal_vpls.h"
#include "hsl_bcm_vpls.h"  /*cyh add */
#include "hal_car.h"
#include "hsl_acl_car.h"
#include "hsl_acl.h"
	
#include "bcm_int/esw/field.h"

////////////////////////////////////////////////////////
hsl_ac_info_t g_ac_info[2048] ={{0}};
hsl_car_info_t g_ac_car_info[2048]={{0}};
hsl_car_info_t g_pw_car_info[2048]={{0}};
hsl_car_info_t g_tun_car_info[2048]={{0}};
hsl_priority_info_t g_ac_priority_info[2048]={{0}};//zlh@150116 save priority info


void hsl_car_init(void)
{
	int i;
	int cosq;
	int port;
	int sev;
	
	for(i=0;i<1024;i++)
	{
		g_ac_car_info[i].entry_id = 0;//zlh@150116
		g_ac_priority_info[i].entry_id = 0;//zlh@150116
		g_ac_info[i].entry_id = -1;//zlh@150123
		g_pw_car_info[i].entry_id = -1;
		g_tun_car_info[i].entry_id = -1;
		
		
	}

}
/***************************************************************
* Function: hsl_port_egr_shapping_set
* Description: set egress shapping,egress rate
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
int
hsl_port_egr_shapping_set(hal_car_info_t car_info)
{	
    int bcm_error = BCM_E_NONE;

	bcm_port_t port;
    uint32 kbits_size = 0;                         /*Limits in kilobits per second*/
    uint32 kbits_burst = 0;                            /*Max burst size in kilobits*/

	port = car_info.portId;
	kbits_size= car_info.cir;									/*Limits in kilobits per second*/		 
	kbits_burst= car_info.cbs; 							  /*transmit byte to bit*/	   
	
	if (car_info.enable)	/*set traffic shape*/
	{
		bcm_error= bcm_port_rate_egress_set(0, (int)port,kbits_size, kbits_burst);/*set egress max bandwidth */
		BCM_E_CHECK(bcm_port_rate_egress_set,bcm_error);
	}
	else 
	{
		bcm_error= bcm_port_rate_egress_set(0, (int)port,0,4096);		   /*kbits_size sets 0 disable rate limit*,kbits_size set the min value*/
		BCM_E_CHECK(bcm_port_rate_egress_set,bcm_error);
	}

	return bcm_error;
}

/***************************************************************
* Function: hsl_port_ing_rate_limite_set
* Description: set egress rate limite 
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
int hsl_port_ing_rate_limite_set(hal_car_info_t car_info)
{
    int bcm_error = BCM_E_NONE;
	bcm_port_t port;   

	port = car_info.portId;


	if(car_info.enable)
	{
		if((car_info.cbs) < 4)
		{
			car_info.cbs = 4;
		}
		printk("%d %d %d %d \n",car_info.cir,car_info.cbs,car_info.portId, port);
		
		bcm_error= bcm_port_rate_ingress_set(0, port,  car_info.cir,car_info.cbs);/*acrroding argin set ingress max rate limit*/
		BCM_E_CHECK(bcm_port_rate_ingress_set,bcm_error);
	}
	else
	{			 
		bcm_error= bcm_port_rate_ingress_set(0, port, 0 ,car_info.cbs);/*kbits_size sets 0 disable rate limit*/
		BCM_E_CHECK(bcm_port_rate_ingress_set,bcm_error);
	}

	return bcm_error;

}

/***************************************************************
* Function: hsl_ac_ing_car_set
* Description: set ac ingress car and priority
* Return:
*     DRV_OK:            
*     
* Others:对与同一条件qualify只能创建一个域，对一种功能有效，因此
car和priority的action必须在同 一个quality下创建。
***************************************************************/
int hsl_ac_ing_car_set(hal_car_info_t car_info,hal_priority_info_t priority_info)
{
    int bcm_error = BCM_E_NONE;
    bcm_field_entry_t eid;
	bcm_field_entry_t eid_car;//zlh@150125
	bcm_field_entry_t eid_pri;//zlhA@150125
    bcm_port_t port;;
    uint32 pir;
    uint32 cir;
    uint32 pbs;
    uint32 cbs;
	unsigned char drop_yellow = car_info.drop_yellow; 
	unsigned char fwd_red = car_info.fwd_red;

	unsigned char priority;//zlh@150108 set internal priority	
	bcm_vlan_t ivid = 0;//zlh@150122
    bcm_vlan_t ovid;//zlh@150122
	int enable_car;
	int enable_pri;
	int index;
	
    bcm_pbmp_t pbmp, pbmp_mask;
    memset(&pbmp_mask,0xff,sizeof(bcm_pbmp_t));
    memset(&pbmp,0x0,sizeof(bcm_pbmp_t));

	if(car_info.sev_index != 0)
	{
	   index = car_info.sev_index;
	}
	if(priority_info.sev_index != 0)
	{
		index = priority_info.sev_index;
	}
	
	g_ac_info[index].index = index;
	g_ac_car_info[index].index = index;
	g_ac_priority_info[index].index =index;
	
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_1:  index                    = %d \n", index);//zlh@150125 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_2:  index_car                = %d \n", car_info.sev_index);//zlh@150125 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_3:  index_pri                = %d \n", priority_info.sev_index);//zlh@150125 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_4:  port_car                 = %d \n", car_info.portId);//zlh@150125 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_5:  port_pri				    = %d \n", priority_info.portId);//zlh@150125 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_6:  cir 					    = %d \n", car_info.cir);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_7:  pir 					    = %d \n", car_info.pir);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_8:  cbs 					    = %d \n", car_info.cbs);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_9:  pbs 					    = %d \n", car_info.pbs);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_10: car_ovid                 = %d \n", car_info.o_tag_label);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_11: car_info.enable          = %d \n", car_info.enable);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_12: priority                 = %d \n", priority_info.priority);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_13: priority_info.enable     = %d \n", priority_info.enable);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_14: pri_ovid                 = %d \n", priority_info.ovid);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_15: eid                      = %d \n", g_ac_info[index].entry_id);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_16: car_entry_id             = %d \n", g_ac_car_info[index].entry_id);//zlh@150122 test
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_17: priority_entry_id        = %d \n", g_ac_priority_info[index].entry_id);//zlh@150122 test
	
	/* zlh@150125,First config car but priority not config*/
	if(car_info.enable && (!(g_ac_priority_info[index].entry_id)) && (!(priority_info.enable)))
	{
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_18:  *******Just add car******* \n");//zlh@150122 test
		
		g_ac_car_info[index].entry_id = 1;

		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_19:  g_ac_car_info[index].entry_id = %d g_ac_priority_info[index].entry_id = %d\n",
																					g_ac_car_info[index].entry_id,g_ac_priority_info[index].entry_id);//zlh@150122 test
		enable_car = car_info.enable;
		port = car_info.portId;
		cir = car_info.cir;
		pir = car_info.pir;
		cbs = car_info.cbs;
		pbs = car_info.pbs;
		ovid = car_info.o_tag_label;
		
		priority = 0;
		enable_pri = 0;
	}
	
	/* zlh@150125,First config priority but car not config */
	if(priority_info.enable && (!(g_ac_car_info[index].entry_id)) &&  (!(car_info.enable)))
	{
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_20:  *****Just add priority******* \n");//zlh@150122 test
		
		g_ac_priority_info[index].entry_id = 1;
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_21:  g_ac_car_info[index].entry_id = %d g_ac_priority_info[index].entry_id = %d\n",
																					g_ac_car_info[index].entry_id,g_ac_priority_info[index].entry_id);//zlh@150122 test
		enable_car = 0;
		cir = 0;
		pir = 100000;
		cbs = 0;//zlh@150123 set 0 is max;
		pbs = 0;//zlh@150123 set 0 is max;
		
		port = priority_info.portId;
		enable_pri =  priority_info.enable;
		priority = priority_info.priority;
		ovid = priority_info.ovid;
		

	}
	
	/* zlh@150125,config priority also car already config or config car also priority already config */
	if((car_info.enable && g_ac_priority_info[index].entry_id && priority_info.enable) || (priority_info.enable && g_ac_car_info[index].entry_id && car_info.enable))
	{
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_22:  *****add car and priority***** \n");//zlh@150122 test

		
		if(-1 != g_ac_info[index].entry_id);
		{

			bcm_error = bcm_field_entry_destroy(0, g_ac_info[index].entry_id);
			BCM_E_CHECK(bcm_field_entry_destroy,bcm_error);	
			
			g_ac_info[index].entry_id = -1;		
		}	
		
		g_ac_priority_info[index].entry_id = 1;
		g_ac_car_info[index].entry_id = 1;

		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_23:g_ac_car_info[index].entry_id = %d g_ac_priority_info[index].entry_id = %d\n",
																						g_ac_car_info[index].entry_id,g_ac_priority_info[index].entry_id);//zlh@150122 test
				
		enable_car = car_info.enable;
		cir = car_info.cir;
		pir = car_info.pir;
		cbs = car_info.cbs;
		pbs = car_info.pbs;
		port = car_info.portId;
		
		enable_pri = priority_info.enable;
		priority = priority_info.priority;
		
		ovid = car_info.o_tag_label;		
	}

	/* zlh@150123,disable process */

	/* zlh@150125, just disable car*/
	if(g_ac_priority_info[index].entry_id && (!car_info.enable) && g_ac_car_info[index].entry_id)
	{
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_24: *******Just disable car******\n");//zlh@150122 test
		
		if(-1 != g_ac_info[index].entry_id);
		{

			bcm_error = bcm_field_entry_destroy(0, g_ac_info[index].entry_id);
			BCM_E_CHECK(bcm_field_entry_destroy,bcm_error);	
			
			g_ac_info[index].entry_id = -1;		
		}
		
		g_ac_car_info[index].entry_id = 0;
		g_ac_priority_info[index].entry_id = 1;

		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_25:  g_ac_car_info[index].entry_id = %d g_ac_priority_info[index].entry_id = %d\n",
																					g_ac_car_info[index].entry_id,g_ac_priority_info[index].entry_id);//zlh@150122 test
		enable_car = 0;
		cir = 0;
		pir = 100000;
		cbs = 0;
		pbs = 0;
		
		
		priority = priority_info.priority;
		enable_pri = priority_info.enable;
		ovid = priority_info.ovid;
		port = priority_info.portId;
	
	}
	
	/* zlh@150125, just disable priority */
	if(g_ac_car_info[index].entry_id&& (!priority_info.enable) && g_ac_priority_info[index].entry_id)
	{
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_26:	****Just disable priority****\n");//zlh@150122 test

		if(-1 != g_ac_info[index].entry_id);
		{

			bcm_error = bcm_field_entry_destroy(0, g_ac_info[index].entry_id);
			BCM_E_CHECK(bcm_field_entry_destroy,bcm_error);	
			
			g_ac_info[index].entry_id = -1;		
		}
		
		g_ac_car_info[index].entry_id = 1;
		g_ac_priority_info[index].entry_id = 0;

		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_27:  g_ac_car_info[index].entry_id = %d g_ac_priority_info[index].entry_id = %d\n",
																							g_ac_car_info[index].entry_id,g_ac_priority_info[index].entry_id);//zlh@150122 test
		
		enable_car = car_info.enable;
		ovid = car_info.o_tag_label;
		cir = car_info.cir;
		pir = car_info.pir;
		cbs = car_info.cbs;
		pbs = car_info.pbs;
		ovid = car_info.o_tag_label;
		port = car_info.portId;
		
		priority = 0;
		enable_pri = 0;
	
	}
	
	if(car_info.enable || priority_info.enable)
	{
		g_ac_info[index].enable = 1;
	}
	else
	{
		g_ac_info[index].enable = 0;
	}
	
	if(g_ac_info[index].enable)
	{
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_29  port					   = %d \n", port );//zlh@141229 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_30: cir                     = %d \n", cir);//zlh@150122 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_31: pir                     = %d \n", pir);//zlh@150122 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_32: cbs                     = %d \n", cbs);//zlh@150122 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_33: pbs                     = %d \n", pbs);//zlh@150122 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_34: ovid                    = %d \n", ovid);//zlh@150122 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_35: enable                  = %d \n", g_ac_info[index].enable);//zlh@150122 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_36: priority                = %d \n", priority);//zlh@150122 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_37: eid                     = %d \n", g_ac_info[index].entry_id);//zlh@150122 test
	
		eid = g_ac_info[index].entry_id;
	
		if(eid == -1)
		{
			bcm_error = bcm_field_entry_create(0, ACL_IFP_VPN_RATE_LIMIT, &eid);
			BCM_E_CHECK(bcm_field_entry_create,bcm_error);
		
		}
		
		BCM_PBMP_PORT_ADD(pbmp, port);
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_38: eid                     = %d \n",eid);//zlh@150122 test
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_39: priority                = %d \n",priority);//zlh@150122 test
		
		bcm_error = bcm_field_qualify_InPorts(0,  eid,	pbmp, pbmp_mask);
		BCM_E_CHECK(bcm_field_qualify_InPorts,bcm_error);
		

		if(ivid !=0)
		{
			bcm_error = bcm_field_qualify_InVlan(0,  eid,  ivid, 0xfff);
			BCM_E_CHECK(bcm_field_qualify_InVlan,bcm_error);
		}
		
		if(ovid !=0)
		{
			bcm_error = bcm_field_qualify_OuterVlan(0,	eid,  ovid, 0xfff);
			BCM_E_CHECK(bcm_field_qualify_OuterVlan,bcm_error);
		}
			
		/* zlh@150123 , action add new internal priority */
		bcm_error = bcm_field_action_add(0, eid, bcmFieldActionPrioIntNew, priority, 0);
		BCM_E_CHECK(bcm_field_action_add,bcm_error);
			
		/*action*/
		if((cir==0)&&(pir==0))
		{
			bcm_error = bcm_field_action_add(0, eid, bcmFieldActionDrop, 0, 0);
			BCM_E_CHECK(bcm_field_action_add,bcm_error);
					}
		else
		{			
			bcm_error = bcm_field_meter_create(0, eid); 		  /*create a meter*/
			BCM_E_CHECK(bcm_field_meter_create,bcm_error);			
			bcm_error = bcm_field_action_add(0, eid, bcmFieldActionMeterConfig, BCM_FIELD_METER_MODE_trTCM_COLOR_BLIND, 0);
			BCM_E_CHECK(bcm_field_action_add,bcm_error);
			
		
            /* wangjian@141227, drop red only   */
			
			/*  if(drop_yellow)
			{
				bcm_error = bcm_field_action_add(0, eid, bcmFieldActionYpDrop, 0, 0);
				BCM_E_CHECK(bcm_field_action_add,bcm_error);
			}             */
			   
	
			if(!fwd_red)
			{
				bcm_error = bcm_field_action_add(0, eid, bcmFieldActionRpDrop, 0, 0);
				BCM_E_CHECK(bcm_field_action_add,bcm_error);
			}
			
			/*set the odd meter*/
			if(cbs ==0)
			{
				bcm_error= bcm_field_meter_set(0, eid, BCM_FIELD_METER_COMMITTED, cir, 4096);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
			else
			{
				bcm_error = bcm_field_meter_set(0, eid, BCM_FIELD_METER_COMMITTED, cir, cbs);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
				
			/*set the even meter*/
			if(pbs ==0)
			{
				bcm_error = bcm_field_meter_set(0, eid, BCM_FIELD_METER_PEAK, pir, 4096);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
			else
			{
				bcm_error = bcm_field_meter_set(0, eid, BCM_FIELD_METER_PEAK, pir, pbs);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
			
		}
	
		bcm_error = bcm_field_entry_install(0,	eid);
		BCM_E_CHECK(bcm_field_entry_install,bcm_error);
		
		if(bcm_error)
		{
			bcm_error = bcm_field_entry_destroy(0, eid);
			BCM_E_CHECK(bcm_field_entry_destroy,bcm_error);
		}

		g_ac_info[index].entry_id = eid;
		
		g_ac_car_info[index].enable = enable_car;
		g_ac_car_info[index].port = port;
		g_ac_car_info[index].cir = cir;
		g_ac_car_info[index].pir = pir;
		g_ac_car_info[index].cbs = cbs ;
		g_ac_car_info[index].pbs = pbs;
		g_ac_car_info[index].o_tag_label= ovid;
		
		g_ac_priority_info[index].enable =enable_pri;
		g_ac_priority_info[index].port = port;;
		g_ac_priority_info[index].priority = priority;
		g_ac_priority_info[index].ovid  =ovid;
	
	}
	else
	{
		/* zlh@150123,when both car and priority are disable,destroy eid */
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_40:	**********all disable**********\n");//zlh@150123 test

		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_41: g_ac_info[index].entry_id         = %d \n",g_ac_info[index].entry_id);//zlh@150122 test
		if(-1 != g_ac_info[index].entry_id);
		{
			bcm_error = bcm_field_entry_destroy(0, g_ac_info[index].entry_id);
			BCM_E_CHECK(bcm_field_entry_destroy,bcm_error);	
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_42: bcm_error                        = %d \n",bcm_error);//zlh@150122 test
			
			g_ac_info[index].entry_id = -1;
			g_ac_info[index].index = 0;

		
			g_ac_car_info[index].index = 0;
			g_ac_car_info[index].port = 0;
			g_ac_car_info[index].enable = 0;
			g_ac_car_info[index].cir = 0;
			g_ac_car_info[index].pir = 0;
			g_ac_car_info[index].cbs = 0 ;
			g_ac_car_info[index].pbs = 0;
			g_ac_car_info[index].o_tag_label= 0;
			g_ac_car_info[index].entry_id = 0;

			g_ac_priority_info[index].index= 0;
			g_ac_priority_info[index].port = 0;
			g_ac_priority_info[index].enable = 0;
			g_ac_priority_info[index].priority = 0;
			g_ac_priority_info[index].ovid  = 0;
			g_ac_priority_info[index].entry_id = 0;

		}	
	}
	
	HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_ac_ing_car_set_43:  eid                                 = %d \n", g_ac_info[index].entry_id);//zlh@150123 test

    return bcm_error;
}

/***************************************************************
* Function: hsl_mpls_ing_car_set
* Description: set pw/ls ingress car,ingress rate.
* Return:
*     DRV_OK:            
*     
* Others:
***************************************************************/
int hsl_mpls_ing_car_set(hal_car_info_t car_info)
{
    int bcm_error = BCM_E_NONE;
    bcm_field_entry_t  eid = 0;

    bcm_port_t port = car_info.portId;
    bcm_mpls_label_t i_label = car_info.i_tag_label;
    bcm_mpls_label_t o_label = car_info.o_tag_label;
    uint32 pir = car_info.pir;
    uint32 cir = car_info.cir;
    uint32 pbs = car_info.pbs;
    uint32 cbs = car_info.cbs;
	
	unsigned char drop_yellow = car_info.drop_yellow; 
	unsigned char fwd_red = car_info.fwd_red;
    bcm_ip6_t  data_dip128 = {0};
    bcm_ip6_t  mask_dip128 ={0};
    bcm_pbmp_t pbmp, pbmp_mask;
	
	memset(&pbmp_mask,0xff,sizeof(bcm_pbmp_t));
	memset(&pbmp,0x0,sizeof(bcm_pbmp_t));

	if(car_info.enable)
	{
	
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_ing_car_set:o_label = %x	 \n",o_label);//zlh@141229 test
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_ing_car_set:i_label = %x	 \n",i_label);//zlh@141229 test
		
		if(o_label)
		{
			data_dip128[0]= (o_label>>12)&0xff;
			data_dip128[1]= (o_label>>4)&0xff;
			data_dip128[2]= (o_label&0xf)<<4;
			
			mask_dip128[0] = 0xff;
			mask_dip128[1] = 0xff;
			mask_dip128[2] = 0xf0;
		}
	
		if(i_label)
		{
			data_dip128[4]= (i_label>>12)&0xff;
			data_dip128[5]= (i_label>>4)&0xff;
			data_dip128[6]= (i_label&0xf)<<4;
			
			mask_dip128[4] = 0xff;
			mask_dip128[5] = 0xff;
			mask_dip128[6] = 0xf0;
		}

		if((o_label)&&(!i_label))
		{
			eid = g_tun_car_info[car_info.sev_index].entry_id;

			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_ing_car_set: *************************This is lsp car***************************	 \n");//zlh@141229 test
		
		}
		else if((!o_label)&&(i_label))
		{
			eid = g_pw_car_info[car_info.sev_index].entry_id;
			
			HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_ing_car_set: *************************This is pw car***************************	 \n");//zlh@141229 test
			
		}	
		
		HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "hsl_mpls_ing_car_set: eid              = %d	 \n",eid);//zlh@150116 teset
		
		if(eid == -1)
		{
			bcm_error = bcm_field_entry_create(0, ACL_IFP_VPN_RATE_LIMIT, &eid);
			BCM_E_CHECK(bcm_field_entry_create,bcm_error);
		}
		
		BCM_PBMP_PORT_ADD(pbmp, port);
		
		bcm_error = bcm_field_qualify_InPorts(0,  eid,	pbmp, pbmp_mask);
		BCM_E_CHECK(bcm_field_qualify_InPorts,bcm_error);
	
		bcm_error = bcm_field_qualify_DstIp6(0,  eid,  data_dip128, mask_dip128);
		BCM_E_CHECK(bcm_field_qualify_DstIp6,bcm_error);
	
	/*action*/
		if((cir==0)&&(pir==0))
		{
			bcm_error = bcm_field_action_add(0, eid, bcmFieldActionDrop, 0, 0);
			BCM_E_CHECK(bcm_field_action_add,bcm_error);
		}
	
		else
		{			
			bcm_error = bcm_field_meter_create(0, eid); 		  /*create a meter*/
			BCM_E_CHECK(bcm_field_meter_create,bcm_error);
			bcm_error = bcm_field_action_add(0, eid, bcmFieldActionMeterConfig, BCM_FIELD_METER_MODE_trTCM_COLOR_BLIND, 0);
			BCM_E_CHECK(bcm_field_action_add,bcm_error);

			if(drop_yellow)
			{
				bcm_error = bcm_field_action_add(0, eid, bcmFieldActionYpDrop, 0, 0);
				BCM_E_CHECK(bcm_field_action_add,bcm_error);
			}

	
			if(!fwd_red)
			{
				bcm_error = bcm_field_action_add(0, eid, bcmFieldActionRpDrop, 0, 0);
				BCM_E_CHECK(bcm_field_action_add,bcm_error);
			}

			/*set the odd meter*/
			if(cbs ==0)
			{
				bcm_error= bcm_field_meter_set(0, eid, BCM_FIELD_METER_COMMITTED, cir, 4096);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
			else
			{
				bcm_error = bcm_field_meter_set(0, eid, BCM_FIELD_METER_COMMITTED, cir, cbs);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
				
			/*set the even meter*/
			if(pbs ==0)
			{
				bcm_error = bcm_field_meter_set(0, eid, BCM_FIELD_METER_PEAK, pir, 4096);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
			else
			{
				bcm_error = bcm_field_meter_set(0, eid, BCM_FIELD_METER_PEAK, pir, pbs);
				BCM_E_CHECK(bcm_field_meter_set,bcm_error);
			}
			
		}
	
		bcm_error = bcm_field_entry_install(0,	eid);
		BCM_E_CHECK(bcm_field_entry_install,bcm_error);
		
		if(bcm_error)
		{
			bcm_error = bcm_field_entry_destroy(0, eid);
			BCM_E_CHECK(bcm_field_entry_destroy,bcm_error);
		}

		if((o_label)&&(!i_label))
		{
			g_tun_car_info[car_info.sev_index].entry_id = eid;
		}
		else if((!o_label)&&(i_label))
		{
			g_pw_car_info[car_info.sev_index].entry_id = eid;
		}
		

	}
	else
	{
		if((o_label)&&(!i_label))
		{
			eid = g_tun_car_info[car_info.sev_index].entry_id;
		}
		else if((!o_label)&&(i_label))
		{
			eid = g_pw_car_info[car_info.sev_index].entry_id;
		}

		if(-1 != eid)
		{
			bcm_error = bcm_field_entry_destroy(0, eid);
			BCM_E_CHECK(bcm_field_entry_destroy,bcm_error);	

			if((o_label)&&(!i_label))
			{
				g_tun_car_info[car_info.sev_index].entry_id = -1;
			}
			else if((!o_label)&&(i_label))
			{
				g_pw_car_info[car_info.sev_index].entry_id = -1;
			}
		}
	}

    return bcm_error;
}

#if 0
int ial_car_operate(
unsigned long functionNo,  /*  indicating which funcition shall be done for the interface. */
void *arg1,                    /* in: All the needed information for setting the port property,
                                                type casting is needed for getting the valid values. */
void *arg2,                 /*out: The output value structure poiter,
                                                type casting is needed for assigning output values */
void *cookie          )         /*  not used. */
{
 	int bcm_error = DRV_SUCCESS;
	(void *)arg2; 
	(void *)cookie; 

	switch (functionNo)
	{
		case IAL_FUNC_CAR_PORT_ING:
			bcm_error = hsl_port_ing_rate_limite_set(*(hsl_car_info_t *)arg1);
			break;
		case IAL_FUNC_CAR_PORT_EGR:
			bcm_error = hsl_port_egr_shapping_set(*(hsl_car_info_t*)arg1);
			break;
		case IAL_FUNC_CAR_AC_ING:
			bcm_error = hsl_ac_ing_car_set(*(hsl_car_info_t*)arg1);
			break;
		case IAL_FUNC_CAR_PW_ING:
			bcm_error = hsl_mpls_ing_car_set(*(hsl_car_info_t*)arg1);
			break;
		case IAL_FUNC_CAR_TUNNEL_ING:
			bcm_error = hsl_mpls_ing_car_set(*(hsl_car_info_t*)arg1);
			break;

		default:
			return -1;
		}

    return bcm_error;
}
#endif
