/*************************************************************************
文件名   :
描述        : 
版本        : 1.0
日期        : 2012
作者        : 
修改历史    :
**************************************************************************/


#define BCM_FIELD_SUPPORT 1

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "hal_types.h"

#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl_error.h"
#include "bcm_incl.h"

#include "hsl_acl.h"
#include "hal_car.h"
#include "hsl_logger.h"

#include "hsl_acl_car.h"
#include "bcm_int/esw/field.h"

/**************************************************************************
                           全局变量定义和模块初始化
**************************************************************************/
	
//t GROUP_ENTRY_COUNT[GROUP_MAX_NUM ];//};

void hsl_acl_init(void)
{
	//acl_add_ifp_group(ACL_IFP_INGRESS_RATE_LIMIT);	
	acl_add_ifp_group(ACL_IFP_VPN_RATE_LIMIT);	
	//acl_add_ifp_group(ACL_IFP_MPLS_NOT_TERMINATED);	
	//acl_add_ifp_group(ACL_IFP_MPLS_TERMINATED_LEFT_L2);	
}

int acl_add_vfp_group( bcm_field_group_t acl_group_id)
{
    bcm_field_qset_t  qset; 
    int rv = BCM_E_NONE;

    /* input param check */
    if((acl_group_id <0)||(acl_group_id >=GROUP_MAX_NUM ))
    {
        return  BCM_E_PARAM;
    }

    /* clear qset */
    BCM_FIELD_QSET_INIT(qset);

    bcm_field_group_mode_t group_mode = bcmFieldGroupModeDefault;

    switch(acl_group_id)
    {
        case ACL_VFP_GROUP_DEFAULT:
        case ACL_VFP_GROUP_IP_EXTENDED:
        case ACL_VFP_GROUP_IP_STANDARD:  /* single slice */
            /* FVF1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);       /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);       /*lint !e506 !e774*/
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyPortClass);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyVlanFormat);   /*lint !e506 !e774*/
            /* FVF2: 0x0 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);        /*lint !e506 !e774*/       
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);        /*lint !e506 !e774*/      
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);   /*lint !e506 !e774*/      
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);    /*lint !e506 !e774*/     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);    /*lint !e506 !e774*/     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDSCP);  /* BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTos) */  /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);   /*lint !e506 !e774*/    
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTtl);          /*lint !e506 !e774*/
            /* FVF3: 0x3 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);    /*lint !e506 !e774*/
            break;

        case ACL_VFP_GROUP_LAYER_TWO:  /* single slice */
            /* FVF1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);       /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);       /*lint !e506 !e774*/
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyPortClass);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyVlanFormat);   /*lint !e506 !e774*/
            /* FVF2: 0x3 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);       /*lint !e506 !e774*/      
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);       /*lint !e506 !e774*/      
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);    /*lint !e506 !e774*/     
            /* FVF3: 0x3 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);    /*lint !e506 !e774*/
            break;

        case ACL_VFP_GROUP_HYBRID:  /* double-wide slice */
            /* left FVF1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);       /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);       /*lint !e506 !e774*/
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyPortClass);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyVlanFormat);   /*lint !e506 !e774*/
            /* left FVF2: 0x3 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);       /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);       /*lint !e506 !e774*/        
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);    /*lint !e506 !e774*/  
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);        /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);        /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDSCP); /* BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTos) */ /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);   /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);   /*lint !e506 !e774*/
            /* left FVF3: 0x3 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);    /*lint !e506 !e774*/
            group_mode = bcmFieldGroupModeDouble;
            break;
            
        case ACL_VFP_GROUP_IPV6_EXTENDED: /* double-wide slice */
             /* FVF1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);      /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);      /*lint !e506 !e774*/
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyPortClass);   /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);   /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyVlanFormat);  /*lint !e506 !e774*/
            /* FVF2: 0x2 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);      /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);      /*lint !e506 !e774*/       
            /* left FVF3: 0x3 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);   /*lint !e506 !e774*/
            group_mode = bcmFieldGroupModeDouble; 
            break;
            
        case ACL_VFP_GROUP_IPV6_STANDARD:
            /* FVF1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPort);       /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);       /*lint !e506 !e774*/
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyPortClass);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);    /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyVlanFormat);   /*lint !e506 !e774*/
            /* FVF2: 0x1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);       /*lint !e506 !e774*/         
            /* FVF3 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);    /*lint !e506 !e774*/
            break;

       default:
            return BCM_E_PARAM;
    }

    /* VFP stage */
    BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageLookup);         /*lint !e506 !e774*/

    /*slice pri based 0 */	
    rv = bcm_field_group_create_mode_id(0, qset, acl_group_id, group_mode, acl_group_id);
    if(rv != 0)
    {
         return  BCM_E_FAIL;
    }
    
    return  BCM_E_NONE;
}


int acl_add_ifp_group( bcm_field_group_t acl_group_id )   
{
    bcm_field_qset_t qset;                                              /*creat a empty qset*/
    int bcm_error = BCM_E_NONE;                                                   /*return code*/
    _field_group_t *fg;
    int group_pri_offset = 0;                                                   /*group priority offset*/
    
    BCM_FIELD_QSET_INIT(qset);                                          /*initilize the qset*/
    
    switch (acl_group_id)  /*add selectors to qset according to the acl mode*/
    {                                                
        case ACL_IFP_STANDARD:                                        /*standard mode matches*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);  /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);           /*lint !e506 !e774*/ 
            /* F1-5 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);      /*lint !e506 !e774*/
            /* F2-1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDSCP);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpFlags);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTtl);               /*lint !e506 !e774*/ 
            /* fix */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);            /*lint !e506 !e774*/      
            break;
          
        case ACL_IFP_EXTEND:                                         /*extend mode matches*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);  /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);           /*lint !e506 !e774*/ 
            /* F2-1 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDSCP);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpFlags);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTtl);               /*lint !e506 !e774*/ 
            /* F3-7 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyRangeCheck);        /*lint !e506 !e774*/ 
            /* fix */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);            /*lint !e506 !e774*/ 
            break;
          
         case ACL_IFP_LINK:                                           /*link mode matches*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);  /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);           /*lint !e506 !e774*/ 
            /* F1-5 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);      /*lint !e506 !e774*/  
            /* F2-5 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);            /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);            /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);         /*lint !e506 !e774*/ 
            break;
            
        case ACL_IFP_VPN_RATE_LIMIT:       
        case ACL_IFP_MPLS_NOT_TERMINATED:                                           /*link mode matches*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);  /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);           /*lint !e506 !e774*/ 
            
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);      /*lint !e506 !e774*/     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);      /*lint !e506 !e774*/     
            /* F2-2 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);      /*lint !e506 !e774*/     
            /* F3-6 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyMplsTerminated);      /*lint !e506 !e774*/ 
            #if 1
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);      /*lint !e506 !e774*/  
            #endif
            break;

        case ACL_IFP_MPLS_TERMINATED_LEFT_L2:                                           /*link mode matches*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);  /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);           /*lint !e506 !e774*/ 
            /* F1-0*/
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL3IntfGroup);      /*lint !e506 !e774*/     
            /* F2-2 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);      /*lint !e506 !e774*/     
            /* F3-6 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);      /*lint !e506 !e774*/     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyMplsTerminated);      /*lint !e506 !e774*/      
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);      /*lint !e506 !e774*/  
             /* fix */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);            /*lint !e506 !e774*/ 
            break;
          
        case ACL_IFP_INGRESS_RATE_LIMIT:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);  /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);            /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);      /*lint !e506 !e774*/           
            break;
          
        case ACL_IFP_HYBRID:                                        /*hybrid mode matches*/
        case ACL_IFP_ETHERNET_OAM:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);    /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);               /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);               /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDSCP);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);        /*lint !e506 !e774*/ 
            break;      
          
        case ACL_IFP_IPv6:                                           /*IPv6 mode matches*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);    /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);        /*lint !e506 !e774*/ 
            break;
          
        case ACL_IFP_EAPS:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);  /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);           /*lint !e506 !e774*/ 
	      BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcTrunk);           /*lint !e506 !e774*/ 	
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);            /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);            /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);      /*lint !e506 !e774*/ 
            break;

         case ACL_IFP_PROTOCOL_PROTECT:                                           /*IP security-access mode matches*/
	     BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);   /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);               /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);               /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTtl);               /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);          /*lint !e506 !e774*/ 
            //BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcPortTgid);/*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);              /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);        /*lint !e506 !e774*/ 
            break; 

        case ACL_IFP_CFM:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);           /*lint !e506 !e774*/ 
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK1);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK2);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK3);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK4);
            break;
            
        case ACL_IFP_TMPLS_OAM:    
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);           /*lint !e506 !e774*/ 
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK1);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK2);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK3);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK4);
            break;

        case ACL_IFP_1588:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);         /*lint !e506 !e774*/ 
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK1);
            break;

        case ACL_IFP_BFD_IP6_UDF2:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);    
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyMplsTerminated);      /*lint !e506 !e774*/      
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK5);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK6);
			//bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK7);
            break;
            
        case ACL_IFP_BFD_IP_UDF1:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);    
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);                         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);         /*lint !e506 !e774*/ 
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK1);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK2);
            break;
            
        case ACL_IFP_VPN_MUTTICASAT:  
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInPorts);    
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);         
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInterfaceClassL3);  
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK1);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK3);
            //bcm_field_qset_add_udf(unit, &qset, ACL_UDF_ID_TRUNK4);
            break;
            
        case ACL_IFP_SVP_CLASS_ID_STORM_CONTROLT:  /* dingsl_090505 */  
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageIngress);     /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInterfaceClassL3); /*lint !e506 !e774*/ 

            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyLookupStatus);     /*lint !e506 !e774*/ 
            
            /* F2-5 */
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);            /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);            /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);         /*lint !e506 !e774*/ 
            break;
            
        default:                                                      /*no mode matches*/
                return BCM_E_NOT_FOUND;
    }


    switch(acl_group_id)
    {
        case ACL_IFP_1588:
        case ACL_IFP_BFD_IP_UDF1:
        case ACL_IFP_BFD_IP6_UDF2:
        case ACL_IFP_PROTOCOL_PROTECT:     /* double wide single slice */
            bcm_error = bcm_field_group_create_mode_id(0, qset,acl_group_id-ACL_MAX_GROUP_VFP, bcmFieldGroupModeDouble, acl_group_id);
			BCM_E_CHECK(bcm_field_group_create_mode_id,bcm_error);

            break;
            
        case ACL_IFP_HYBRID:/* double wide double slice */
        case ACL_IFP_IPv6:
        case ACL_IFP_CFM:
        case ACL_IFP_ETHERNET_OAM:
          //rtn_code = bcm_field_group_create_mode_id(unit, qset,acl_group_no-group_pri_offset, bcmFieldGroupModeDoubleLink, acl_group_no);
            break;
            
        default:        /* single wide */
            bcm_error = bcm_field_group_create_mode_id(0, qset,acl_group_id-group_pri_offset, bcmFieldGroupModeSingle, acl_group_id);
			BCM_E_CHECK(bcm_field_group_create_mode_id,bcm_error);
            break;
    }
    
    if( 0 != bcm_error)
    {
        return BCM_E_FAIL;                                                
    }
    
    if (BCM_E_NONE !=bcm_field_group_get(0, acl_group_id,&fg))
    {
        return BCM_E_FAIL;                                                
    }
                          
    return bcm_error;                                                 
}



int acl_add_efp_group(bcm_field_group_t acl_group_id)   
{
    bcm_field_qset_t qset;                                             
    int rtn_code = 0;                                                  
    _field_group_t *fg;
    int group_pri_offset = 0;                                                  
    
    BCM_FIELD_QSET_INIT(qset);                                          
    
    switch (acl_group_id)  /*add selectors to qset according to the acl mode*/
    {                                                
        case ACL_EFP_LINK:                                        
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageEgress);     /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOutPort);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);       /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);       /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);       /*lint !e506 !e774*/ 
            break;
          
        case ACL_EFP_IPV4:                                        
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageEgress);                       /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOutPort);                            /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);                            /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);                             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);                         /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);                 /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);                /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);              /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTos);                   /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTtl);                   /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);      /*lint !e506 !e774*/ 
            break;
            
        case ACL_EFP_IPV6:     /*  关注EFP_SLICE_CONTROL设置*/                                     
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageEgress);     /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOutPort);           /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIp6);               /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp6);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp6);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);         /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);       /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl); /*lint !e506 !e774*/ 
            break;
            
        case ACL_EFP_HYBRID:
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyStageEgress);     /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOutPort);        /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpType);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcIp);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstIp);           /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyIpProtocol);      /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4SrcPort);        /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyL4DstPort);         /*lint !e506 !e774*/
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTos);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTtl);             /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyTcpControl);      /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyDstMac);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifySrcMac);          /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyInnerVlan);       /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyOuterVlan);       /*lint !e506 !e774*/ 
            BCM_FIELD_QSET_ADD(qset, bcmFieldQualifyEtherType);       /*lint !e506 !e774*/ 
            break;

        default:                                                      /*no mode matches*/
                return BCM_E_NOT_FOUND;
    }

    if((ACL_EFP_IPV6==acl_group_id)||(ACL_EFP_HYBRID==acl_group_id))
    {
		//caiguoqing  rtn_code = bcm_field_group_create_mode_id(unit, qset,acl_group_no-group_pri_offset, bcmFieldGroupModeDoubleLink, acl_group_no);
    }
	else
	{
		rtn_code = bcm_field_group_create_mode_id(0, qset,acl_group_id-ACL_MAX_GROUP_IFP, bcmFieldGroupModeSingle, acl_group_id);
	}
    
    if( 0 != rtn_code)
    {
        return BCM_E_FAIL;             
    }
    
    if (BCM_E_NONE !=bcm_field_group_get(0, acl_group_id,&fg))
    {
        return BCM_E_FAIL;                                             
    }
                          
    return BCM_E_NONE;                                               
}


