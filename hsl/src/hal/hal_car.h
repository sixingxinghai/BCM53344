/* Copyright (C) 2004  All Rights Reserved */
#ifndef __HAL_CAR_H__
#define __HAL_CAR_H__

#define HAL_MSG_CAR_BASE                           (100+1700)
#define HAL_MSG_CAR_MSG(n)                         (HAL_MSG_CAR_BASE  + (n))
#define HAL_MSG_CAR_PORT_RATE_LIMIT                HAL_MSG_CAR_MSG(1)
#define HAL_MSG_CAR_PORT_SHAPPING                  HAL_MSG_CAR_MSG(2)
#define HAL_MSG_CAR_VPN_AC	      		           HAL_MSG_CAR_MSG(3)
#define HAL_MSG_CAR_VPN_MPLS      		           HAL_MSG_CAR_MSG(4)
#define HAL_MSG_PRIORITY_VPN_AC      		       HAL_MSG_CAR_MSG(5) //zlh@150116 ,Add priority msg
/* zlh@150119 ,add weight wred egress-car msg*/
#define HAL_MSG_WRR_CAR_VPN_AC                     HAL_MSG_CAR_MSG(6)
#define HAL_MSG_WRR_WRED_VPN_AC                    HAL_MSG_CAR_MSG(7)
#define HAL_MSG_WRR_WEIGHT_VPN_AC                  HAL_MSG_CAR_MSG(8) 



#define uint32 unsigned int


typedef struct hal_car_info_s
{
	uint32 	sev_index;  //acid / pwid/ tunId
	int		enable;     //endable /disable

	uint32 portId;

	uint32 i_tag_label;	 //pw lable
	uint32 o_tag_label;	//tunnel lable or vlanId

	uint32 cir;				/*承诺速率 Kbps*/    
	uint32 pir;				/*峰值速率 Kbps*/
	uint32 cbs;				/*承诺速率 Kbps*/
	uint32 pbs;				/*峰值速率 Kbps*/
	
	unsigned char drop_yellow; //不用
	unsigned char fwd_red;   //不用
	
	
}hal_car_info_t;

/* zlh@150116, add priority info type*/
typedef struct hal_priority_info_s
{
	uint32 	sev_index;  //acid / pwid/ tunId
	int		enable;     //endable /disable
	
	uint32 portId;
	
	unsigned char  priority;
	
	/* zlh@150122 */
	uint32 ivid;
	uint32 ovid;

	
	
}hal_priority_info_t;




#endif /* __HAL_CAR_H__ */
