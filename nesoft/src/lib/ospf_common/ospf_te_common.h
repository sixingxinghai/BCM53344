/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_TE_COMMON_H
#define _PACOS_OSPF_TE_COMMON_H

#if defined (HAVE_OSPF_TE) || defined (HAVE_OSPF6_TE)

struct te_tlv_header
{
  u_int16_t tlv_type;
  u_int16_t tlv_len;
};

struct te_tlv
{
  struct te_tlv_header header;
  char data[1];
};

#define TE_TLV_ALIGNTO  4
#define TE_TLV_ALIGN(len) (((len)+TE_TLV_ALIGNTO-1) & ~(TE_TLV_ALIGNTO-1))
#define TE_TLV_LENGTH(tlvh) TE_TLV_ALIGN (sizeof (struct te_tlv_header)\
                                          + pal_ntoh16 ((tlvh)->tlv_len))
#define TE_TLV_DATA(tlvh) ((struct te_tlv*)(tlvh))->data

#define TE_TLV_NEXT(tlvh) ((struct te_tlv_header*)(((char *)(tlvh)) +\
                                                   TE_TLV_LENGTH (tlvh)))
#define TE_TLV_OK(tlvh, len) ((len) > 0 && TE_TLV_LENGTH(tlvh) <= (len))

#define TE_TLV_ROUTER_TYPE              1
#define TE_TLV_LINK_TYPE                2
#define TE_TLV_ROUTER_IPV6_ADDR_TYPE    3  /* For OSPFV6 */

#define TE_LINK_SUBTLV_LINK_TYPE                1
#define TE_LINK_SUBTLV_LINK_ID                  2
#define TE_LINK_SUBTLV_LOCAL_ADDRESS            3
#define TE_LINK_SUBTLV_REMOTE_ADDRESS           4
#define TE_LINK_SUBTLV_TE_METRIC                5
#define TE_LINK_SUBTLV_MAX_BANDWIDTH            6
#define TE_LINK_SUBTLV_MAX_RES_BANDWIDTH        7
#define TE_LINK_SUBTLV_UNRES_BANDWIDTH          8
#define TE_LINK_SUBTLV_RESOURCE_CLASS           9
#define TE_LINK_SUBTLV_BC                       10
#define TE_LINK_SUBTLV_LOCAL_REMOTE_ID          11
#define TE_LINK_SUBTLV_PROTECTION_TYPE          14
#define TE_LINK_SUBTLV_IF_SW_CAPABILITY         15
#define TE_LINK_SUBTLV_SHARED_RISK_GROUP        16
#define TE_LINK_SUBTLV_NEIGHBOR_ID              17 /*17, 18, 19 are for OSPFV6*/
#define TE_LINK_SUBTLV_LOCAL_IF_IPV6_ADDRESS    18
#define TE_LINK_SUBTLV_REMOTE_IF_IPV6_ADDRESS   19

/* TE TLV length. */
#define OSPF_TE_TLV_HEADER_FIX_LEN            4

#define OSPF_TE_TLV_RTADDR_FIX_LEN            4
#define OSPF_TE_TLV_RTADDR_IPV6_FIX_LEN       16 

#define OSPF_TE_TLV_LINK_TYPE_FIX_LEN         1
#define OSPF_TE_TLV_LINK_ID_FIX_LEN           4
#define OSPF_TE_TLV_IP_ADDR_MIN_LEN           4
#define OSPF_TE_TLV_IPV6_ADDR_MIN_LEN         16 
#define OSPF_TE_TLV_NEIGHBOR_ID_FIX_LEN       8  
#define OSPF_TE_TLV_METRIC_FIX_LEN            4
#define OSPF_TE_TLV_BW_FIX_LEN                4
#define OSPF_TE_TLV_UNRSV_BW_FIX_LEN          32
#define OSPF_TE_TLV_RSRC_COLOR_FIX_LEN        4
#define OSPF_TE_TLV_LOCAL_REMOTE_ID_FIX_LEN   8
#define OSPF_TE_TLV_PROTECTION_TYPE_FIX_LEN   4
#define OSPF_TE_TLV_IF_SW_CAPABILITY_MIN_LEN  36 
#define OSPF_TE_TLV_IF_SW_CAPABILITY_PSC_LEN  44
#define OSPF_TE_TLV_IF_SW_CAPABILITY_TDM_LEN  44
#define OSPF_TE_TLV_BC_MIN_LEN                5
#define OSPF_TE_TLV_BC_MAX_LEN                33
#define OSPF_TE_TLV_LINK_LOCAL_IDNTIFIER      32

#define TE_LINK_TYPE_PTP           1
#define TE_LINK_TYPE_MULTIACCESS   2

#endif /* HAVE_OSPF_TE, HAVE_OSPF6_TE */

#endif /* _PACOS_OSPF_TE_COMMON_H */
