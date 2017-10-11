/* Copyright (C) 2004  All Rights Reserved. */
#ifndef _HSL_BCM_ACL_H_
#define _HSL_BCM_ACL_H_


#define ACL_UNUSED_OBJ  0
#define ACL_USED_OBJ    1


#define ACL_VPN_MUTTICASAT_MODE              24 /* tspan_090408 */
#define ACL_SVP_CLASS_ID_STORM_CONTROLT_MODE  25  /* dingsl_090505 */


#define ACL_INGRESS_RATE_LIMIT_MODE              12



#define ACL_VPN_RATE_LIMIT_MODE              23
#define ACL_MPLS_NOT_TERMINATED             4
  /*******************************************************************************
  *  LINK  Mode  Select(single-wide):
  *  F1=Input Port Bitmap,IP_TYPE
  *  F2=Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN
  *  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format
  *******************************************************************************/


 /* ====================begining of   group number and entry number  defination ==================*/
#define ACL_GROUP_MAX_VFP             4            /* number of max vfp group in a pp*/
#define ACL_GROUP_MAX_IFP             16           /* number of max ifp group in a pp*/
#define ACL_GROUP_MAX_EFP             4            /* number of max not efp group in a pp*/
 

/////////////////////////////////////////////////////////
#define ACL_VPN_RATE_LIMIT_MODE     23
#define ACL_GROUP_TOTAL_NUM			24
#define GROUP_FOR_VPN_RATE_LIMIT     6    


#define  ACL_ENTRY_VFP_MAX_NUM          (512*4)
#define  ACL_ENTRY_IFP_MAX_NUM          (512*16 - 256)
#define  ACL_ENTRY_EFP_MAX_NUM          (256*4)
#define  ACL_ENTRY_VFP_IFP_MAX_NUM      (ACL_ENTRY_VFP_MAX_NUM + ACL_ENTRY_IFP_MAX_NUM)
#define ACL_ENTRY_INTERNAL_TOTAL_NUM    (ACL_ENTRY_VFP_MAX_NUM+ACL_ENTRY_IFP_MAX_NUM+ACL_ENTRY_EFP_MAX_NUM)
#define ACL_ENTRY_EXTERNAL_TOTAL_NUM    0
#define ACL_ENTRY_TOTAL_NUM                      (ACL_ENTRY_INTERNAL_TOTAL_NUM+ACL_ENTRY_EXTERNAL_TOTAL_NUM)


#define ACL_NUMBER_FOR_VPN_RATE_LIMIT           15013
#define ACL_NUMBER_FOR_MCC_RATE_LIMIT           15014


typedef struct vpn_rate_limit_info_s{
uint32      port;
uint32      inner_vlan;
uint32      outer_vlan;
uint32      inner_label;       /*当匹配tunnel时，该变量为pw标签，否则=0*/
uint32      outer_label;         /*当不匹配隧道标签时，该变量为pw标签，否则为隧道标签*/
uint32      cir;                     /*承诺速率 Kbps*/
uint32      pir;                   /*峰值速率 Kbps*/
uint32      cbs;                 /*承诺速率 Kbps*/
uint32      pbs;                /*峰值速率 Kbps*/
}vpn_rate_limit_info_t;

typedef struct acl_vpn_rate_limit_s{
    uint32                        used;
    vpn_rate_limit_info_t   info;
    int                             eid;
}acl_vpn_rate_limit_t;

#define debug_acl_enable 1

extern int  ACL_GROUP_MODE_TABLE[ACL_GROUP_TOTAL_NUM];
extern int  ACL_RULE_EID_TABLE[ACL_ENTRY_TOTAL_NUM];

///////////////////////////////////////////////////////////////////////////



#endif
