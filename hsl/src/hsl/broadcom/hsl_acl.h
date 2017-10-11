/*************************************************************************
文件名   :
描述        : 
版本        : 1.0
日期        : 2012
作者        : 
修改历史    :
**************************************************************************/
#ifndef __HSL_ACL_H__
#define __HSL_ACL_H__


#define ACL_ENTRY_VFP_MAX_NUM  1024
#define ACL_ENTRY_IFP_MAX_NUM  2048
#define ACL_ENTRY_EFP_MAX_NUM  512

#define  ACL_TYPE_VCAP	1
#define  ACL_TYPE_ICAP	2
#define  ACL_TYPE_ECAP	3


//group define
#define ACL_MAX_GROUP_VFP			10
#define ACL_VFP_GROUP_DEFAULT                                                                       1  /* normal mode */
#define ACL_VFP_GROUP_IP_STANDARD                                                                   2 
#define ACL_VFP_GROUP_IP_EXTENDED                                                                   3  
#define ACL_VFP_GROUP_LAYER_TWO                                                                     4  /* link mode */
#define ACL_VFP_GROUP_HYBRID                                                                        5  /* hybrid mode */
#define ACL_VFP_GROUP_IPV6_STANDARD                                                                 6  /* ipv6 mode */
#define ACL_VFP_GROUP_IPV6_EXTENDED                                                                 7 
#define ACL_VFP_GROUP_COUNT                                                                         8   /* all last,not usable,recorder the mod count */

//ifp
#define ACL_MAX_GROUP_IFP			40
/* ===========begining of  ACL mode defination ============================= */
#define ACL_IFP_STANDARD                 														 11 
/*******************************************************************************                     
*  STANDARD  Mode  Select(single-wide):                                                              
*  F1=Input Port Bitmap                                                                              
*  F2=Source IPv4 Address,Destination IPv4 Address,IP Protocol Field,TCP/UDP Source Port,            
*  TCP/UDP Destination Port,Diffserv Code Point,IPv4 Flags,TCP Control Flags,IPv4 Time To Live       
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format                               
*******************************************************************************/                     
#define ACL_IFP_EXTEND          																 12 
/*******************************************************************************                     
*  EXTEND Mode  Select(single-wide):                                                                 
*  F1=Input Port Bitmap                                                                              
*  F2=Source IPv4 Address,Destination IPv4 Address,IP Protocol Field,TCP/UDP Source Port Range       
*  Check Results,TCP/UDP Destination Port,Diffserv Code Point,IPv4 Flags,TCP Control Flags,IPv4      
*  Time To Live                                                                                      
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format                               
*******************************************************************************/                     
#define ACL_IFP_LINK             																13
/*******************************************************************************                     
*  LINK  Mode  Select(single-wide):                                                                  
*  F1=Input Port Bitmap                                                                              
*  F2=Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN                      
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format                               
*******************************************************************************/                     
#define ACL_IFP_MPLS_NOT_TERMINATED          												    14
/*******************************************************************************                     
*  LINK  Mode  Select(single-wide):                                                                  
*  F1=Input Port Bitmap,IP_TYPE                                                                      
*  F2=Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN                      
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format                               
*******************************************************************************/
#define ACL_IFP_HYBRID          														 		 15
/********************************************************************************
*  HYBRID  Mode 1 Select(double-wide):
*  F1=Input Port Bitmap ,IP_TYPE,Outer VLAN , Inner VLAN
*  F2=Source IPv4 Address,Destination IPv4 Address,IP Protocol Field,TCP/UDP Source Port Range 
*  Check Results,TCP/UDP Destination Port,Diffserv Code Point,IPv4 Flags,TCP Control Flags,IPv4 
*  Time To Live,Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format
*******************************************************************************/
#define ACL_IFP_IPv6              														         16
/*********************this mode for 69 only*************************************
*  IPv6 Mode 1 Select(double-wide):
*  F1=Input Port Bitmap
*  F2=IPv6 Source IP,IPv6 Destination IP
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format
*******************************************************************************/
#define ACL_IFP_PROTOCOL_PROTECT                										           17
/********************************************************************************
*  HYBRID  Mode 1 Select(double-wide):
*  F1=Input Port Bitmap ,IP_TYPE,Outer VLAN , Inner VLAN
*  F2=Source IPv4 Address,Destination IPv4 Address,IP Protocol Field,TCP/UDP Source Port Range 
*  Check Results,TCP/UDP Destination Port,Diffserv Code Point,IPv4 Flags,TCP Control Flags,IPv4 
*  Time To Live,Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format
*******************************************************************************/
#define ACL_IFP_INGRESS_RATE_LIMIT              												   18
/*******************************************************************************
*  LINK  Mode  Select(single-wide):
*  F1=Input Port Bitmap,IP_TYPE
*  F2=Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format
*******************************************************************************/
#define ACL_IFP_EAPS              																	 19
/*******************************************************************************
*  EAPS  Mode  Select(single-wide):
*  F1=Input Port Bitmap
*  F2=Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN
*  F3=IP Information,Pkt resolution,Module Header Opcode,Packet Format
*******************************************************************************/
#define ACL_IFP_UDF                 		                                                         20
#define ACL_IFP_MPLS_TERMINATED_LEFT_L2                                                              21
#define ACL_IFP_ETHERNET_OAM           	                                                             22
#define ACL_IFP_CFM                                                                                   23   /* for ethernet CFM */
#define ACL_IFP_1588                                                                                 24  /* for 1588 support */
#define ACL_IFP_TMPLS_OAM                                                                            25
#define ACL_IFP_BFD_IP_UDF1                                                                          26
#define ACL_IFP_BFD_IP6_UDF2                                                                         27
#define ACL_IFP_VPN_RATE_LIMIT                                                                        28
#define ACL_IFP_VPN_MUTTICASAT                                                                       29 
#define ACL_IFP_SVP_CLASS_ID_STORM_CONTROLT                                                           30  

 
//ecap
#define ACL_MAX_GROUP_EFP			50
#define ACL_EFP_LINK                															      41
/*******************************************************************************
*  EFP LINK Mode 1 Select(single-wide):
*  output Port Bitmap,Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN
*  inner VLAN
*******************************************************************************/
#define ACL_EFP_IPV4                													             42
/*******************************************************************************
*  EFP IPV4 Mode 1 Select(single-wide):
*  output Port Bitmap,Source IPv4 Address,Destination IPv4 Address,TCP/UDP Source Port
*  TCP/UDP Destination Port,IP Protocol Field,TOS,TTL,TCP Control Flags
*******************************************************************************/
#define ACL_EFP_IPV6                 																	43
/*******************************************************************************
*  EFP LINK Mode 1 Select(double-wide):
*  output Port Bitmap,IPv6 Source IP,IPv6 Destination IP,TC 
*******************************************************************************/
#define ACL_EFP_HYBRID                 																  	44
/*******************************************************************************
*  EFP LINK Mode 1 Select(double-wide):
*   output Port Bitmap,Destination L2 MAC Address,Source L2 MAC Address,Ethernet Type,Outer VLAN
Source IPv4 Address,Destination IPv4 Address,TCP/UDP Source Port
*  TCP/UDP Destination Port,IP Protocol Field,TOS,TTL,TCP Control Flags
*******************************************************************************/

/* DEFINED FOR EXTERNAL ACL */  
#define ACL_MAX_GROUP_EFP			60
#define ESM_ACL_STANDARD                                                                           51                                     
#define ESM_ACL_EXTEND                                                                             52                                  
#define ESM_ACL_LINK                                                                               53                               
#define ESM_ACL_HYBRID                                                                             54                                 
#define ESM_ACL_IPv6                                                                               55                              




#define GROUP_MAX_NUM  60


extern void hsl_acl_init(void);


#endif

