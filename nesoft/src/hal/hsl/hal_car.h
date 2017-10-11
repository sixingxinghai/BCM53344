/* Copyright (C) 2004  All Rights Reserved */
#ifndef __HAL_CAR_H__
#define __HAL_CAR_H__

#define HAL_MSG_CAR_BASE                           (100+1700)
#define HAL_MSG_CAR_MSG(n)                         (HAL_MSG_CAR_BASE  + (n))
#define HAL_MSG_CAR_PORT_RATE_LIMIT                HAL_MSG_CAR_MSG(1)
#define HAL_MSG_CAR_PORT_SHAPPING                  HAL_MSG_CAR_MSG(2)
#define HAL_MSG_CAR_VPN_AC	      		           HAL_MSG_CAR_MSG(3)
#define HAL_MSG_CAR_VPN_MPLS      		           HAL_MSG_CAR_MSG(4)
#define HAL_MSG_PRIORITY_VPN_AC                    HAL_MSG_CAR_MSG(5)   //wmx@150116
#define HAL_MSG_WRR_CAR_VPN_PORT                     HAL_MSG_CAR_MSG(6) //wmx@150120
#define HAL_MSG_WRR_WRED_VPN_PORT                    HAL_MSG_CAR_MSG(7) //wmx@150120
#define HAL_MSG_WRR_WEIGHT_VPN_PORT                  HAL_MSG_CAR_MSG(8) //wmx@150120



typedef struct hal_car_info_s
{
	int  	sev_index;  //acid / pwid/ tunId
	int		enable;     //endable /disable

	unsigned long int  portId;

	unsigned long int  i_tag_label;	 //pw lable
	unsigned long int  o_tag_label;	//tunnel lable or vlanId

	unsigned long int  cir;				/*承诺速率 Kbps*/    
	unsigned long int  pir;				/*峰值速率 Kbps*/
	unsigned long int  cbs;				/*承诺速率 Kbps*/
	unsigned long int  pbs;				/*峰值速率 Kbps*/
	
	
	unsigned char drpYellow; //不用
	unsigned char fwdRed;   //不用
}hal_car_info_t;

/*wmx@150120 add struct for cmd of priotity/egress-car/wred/weight*/
typedef struct hal_priority_info_s
{
	int  	sev_index;  //acid / pwid/ tunId
	int		enable;     //endable /disable

	unsigned long int  portId;
    unsigned char priority;       //wmx@141230 增加优先级
    
    /* zlh@ 0122, add vid */
	unsigned long int ivid;
	unsigned long int ovid;
	
}hal_priority_info_t;

typedef struct hal_wrr_car_info_s
{
	
    int  	sev_index;  //acid / pwid/ tunId
    BOOL 	is_enable;	   //endable /disable
	unsigned long int  portId;
	int qid;
	int cir;
	int pir;
} hal_wrr_car_info_t;

typedef struct  hal_wrr_wred_info_s
{
	
    int  	sev_index;  //acid / pwid/ tunId
    BOOL 	is_enable;	   //endable /disable

	unsigned long int  portId;
	int qid;
	int startpoint; //0-100
	int slope;  //0-90
	int color; //yellow red green
	int time; //(0-255)
}hal_wrr_wred_info_t;

typedef struct  hal_wrr_weight_info_s
{
	
    int  	sev_index;//acid / pwid/ tunId
    BOOL 	is_enable;	   //endable /disable

	unsigned long int  portId;
	int weight[8];     //8 == MAX_NUM_OF_QUEUE
}hal_wrr_weight_info_t;


typedef struct hal_wrr_queue_info_s
{
	struct hal_wrr_car_info_s car[8];    //8 == MAX_NUM_OF_QUEUE
	struct hal_wrr_wred_info_s wred[8];
	struct hal_wrr_weight_info_s weight;
} hal_wrr_queue_info_t;









#endif /* __HAL_CAR_H__ */
