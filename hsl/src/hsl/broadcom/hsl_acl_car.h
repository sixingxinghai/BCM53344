/*************************************************************************
文件名   :
描述        : 
版本        : 1.0
日期        : 2012
作者        : 
修改历史    :
**************************************************************************/
#ifndef __HSL_ACL_CAR_H__
#define __HSL_ACL_CAR_H__

/* zlh@150123 */
typedef struct hsl_ac_infor_s
{
	uint32 index;//acid  / host port / pwid  / tunnel id 
	
	int enable;
	
	bcm_field_entry_t entry_id;	
	
}hsl_ac_info_t;

typedef struct hsl_car_infor_s
{
	uint32 index;//acid  / host port / pwid  / tunnel id 
	
	/* zlh@150123,save car info */
	int enable;
	uint32 port;
	uint32 cir;
	uint32 pir;
	uint32 cbs;
	uint32 pbs;
	
	uint32 o_tag_label; 

	bcm_field_entry_t entry_id;	
}hsl_car_info_t;

/* zlh@150116, ac priority info struct */

typedef struct hsl_priority_infor_s
{
	uint32 index; //acid  / host port / pwid  / tunnel id 
	
	/* zlh@150123,save priority info */
	int enable; 	//endable /disable
	uint32 port;
	unsigned char  priority;
	
	uint32 ivid;
	uint32 ovid;
	
	bcm_field_entry_t entry_id;	
}hsl_priority_info_t;



#define BCM_E_CHECK(op, rv)     \
    do {               \
        if (BCM_FAILURE(rv)) { \
	        printk(	"BCM: " #op " failed, %s; file:%s;   line:%d;\n", bcm_errmsg(rv), __FILE__, __LINE__); \
        } \
    } while(0)


void hsl_car_init(void);
int hsl_port_egr_shapping_set(hal_car_info_t car_info);
int hsl_port_ing_rate_limite_set(hal_car_info_t car_info);
//int hsl_ac_ing_car_set(hal_car_info_t car_info);
int hsl_ac_ing_car_set(hal_car_info_t car_info,hal_priority_info_t priority_info);

int hsl_mpls_ing_car_set(hal_car_info_t car_info);

int hsl_ac_ing_priority_set(hal_priority_info_t priority_info);//zlh@150116, set ac ing priority


//int ial_car_operate(unsigned long functionNo,void *arg1,void *arg2,void *cookie);


#endif

