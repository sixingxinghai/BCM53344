/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_ETHER_H_
#define _HSL_ETHER_H_

/* 
   VLAN header. 
*/
struct hsl_vlan_header
{
  u_int16_t tag_type;      /* 0x8100 */
  u_int16_t pri_cif_vid;   /* 3 bits priority
                                   1 bit canonical format indicator
                                   12 bits VLAN identifier. */
  u_int16_t type;          /* Ethernet frame type. */
} __attribute__((__packed__));

/*
  Ethernet header.
*/
struct hsl_eth_header
{
  u_char dmac[6];
  u_char smac[6];
  union
  {
    u_int16_t type;
    struct hsl_vlan_header vlan;
  } d;
} __attribute__((__packed__));

#define HSL_ETH_VLAN_GET_VID(pri_cif_vid)        (ntohs((pri_cif_vid)) & 0xfff)
#define HSL_ETH_VLAN_SET_VID(pri_cif_vid, vid)   ((pri_cif_vid) = (((pri_cif_vid) & htons(0x0fff)) | htons(vid)))

#define HSL_ETHER_ADDRLEN                  6
#define HSL_ETHER_TYPE_CFM                 0x8902
#define HSL_ETHER_TYPE_IP                  0x0800
#define HSL_ETHER_TYPE_IPV6                0x86DD
#define HSL_ETHER_TYPE_ARP                 0x0806
#define HSL_ETHER_TYPE_RARP                0x8035
#define HSL_ENET_8021Q_VLAN                0x8100
#define HSL_ETHER_TYPE_LLDP                0x8100
#define HSL_ETHER_TYPE_EFM                 0x8100
#define HSL_ETHER_LACP_SUBTYPE             0x01
#define HSL_ETHER_EFM_SUBTYPE              0x03

#define HSL_ETH_GET_SLOW_PRO_TYPE(P,TYPE)      \
 do {                                          \
   struct hsl_eth_header *eth;                 \
   eth = (struct hsl_eth_header *) (P);                                  \
   if (eth->d.type == HSL_L2_ETH_P_8021Q)      \
     (TYPE) = *((P) + ENET_TAGGED_HDR_LEN);    \
   else                                        \
     (TYPE) = *((P) + ENET_UNTAGGED_HDR_LEN);  \
    } while (0);

#define HSL_ETHER_MIN_DATA                 46 /* minimum packet user data length*/
#define HSL_ETHER_MAX_LEN                  1518 /* maximum packet length */
#define HSL_ETHER_MAX_DATA                 1500 /* maximum packet user data length */
#define HSL_8021Q_MAX_LEN                  1522 /* maximum tagged frame length */
#define HSL_ETH_PKT_SIZE                   1520
#define HSL_ETHER_CRC_LEN                  4

/* Function prototypes. */
char *hsl_etherAddrToStr(char *p, char *str);
char *hsl_etherStrToAddr (char *str, char *p);


#endif /* _HSL_ETHER_H_ */
