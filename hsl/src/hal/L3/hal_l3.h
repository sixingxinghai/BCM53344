/* Copyright (C) 2004  ll Rights Reserved. */

#ifndef _HAL_L3_
#define _HAL_L3_

/* VIF type. */
#define HAL_IPV4_VIF_TUNNEL                        (1 << 0)
#define HAL_IPV4_VIF_REGISTER                      (1 << 1)

/* The type of multicast signal messages from the forwarder. */
#define HAL_IPV4_MESSAGE_NOCACHE                    1
#define HAL_IPV4_MESSAGE_WRONGVIF                   2
#define HAL_IPV4_MESSAGE_WHOLEPKT                   3

/* MFC flags. */
#define HAL_IPV4_MRT_MFC_FLAGS_DISABLE_WRONGVIF     (1 << 0)
#define HAL_IPV4_MRT_MFC_RP                         (1 << 1)

#ifdef HAVE_IPV6
/* VIF type. */
#define HAL_IPV6_VIF_TUNNEL                        (1 << 0)
#define HAL_IPV6_VIF_REGISTER                      (1 << 1)

/* The type of multicast signal messages from the forwarder. */
#define HAL_IPV6_MESSAGE_NOCACHE                    1
#define HAL_IPV6_MESSAGE_WRONGVIF                   2
#define HAL_IPV6_MESSAGE_WHOLEPKT                   3

/* MFC flags. */
#define HAL_IPV6_MRT_MFC_FLAGS_DISABLE_WRONGVIF     (1 << 0)
#define HAL_IPV6_MRT_MFC_RP                         (1 << 1)

#endif /* HAVE_IPV6 */

/* Nexthop type. */
enum hal_ipuc_nexthop_type
  {
    HAL_IPUC_UNSPEC,         /* Placeholder for 'unspecified' */
    HAL_IPUC_LOCAL,          /* Nexthop is directly attached. */
    HAL_IPUC_REMOTE,         /* Nexthop is remote. */
    HAL_IPUC_SEND_TO_CP,     /* Send to Control plane. */
    HAL_IPUC_BLACKHOLE,      /* Drop. */
    HAL_IPUC_PROHIBIT        /* Administratively probihited. */
  };

/* Nexthop structure. */
struct hal_ipv4uc_nexthop
{
  unsigned int id;
  enum hal_ipuc_nexthop_type type;
  unsigned int egressIfindex;
  char *egressIfname;
  struct hal_in4_addr nexthopIP;
};

#ifdef HAVE_IPV6
/* Nexthop structure. */
struct hal_ipv6uc_nexthop
{
  unsigned int id;
  enum hal_ipuc_nexthop_type type;
  unsigned int egressIfindex;
  struct hal_in6_addr nexthopIP;
};

#define HAL_IPV6_NBR_CACHE_GET_COUNT 50

struct hal_ipv6_nbr_cache_entry
{
  struct hal_in6_addr addr;
  unsigned char mac_addr[ETHER_ADDR_LEN];
  unsigned int  ifindex;
  int static_flag;
};
#endif /* HAVE_IPV6 */

#define HAL_ARP_CACHE_GET_COUNT 50

struct hal_arp_cache_entry
{
  struct hal_in4_addr ip_addr;
  unsigned char mac_addr[ETHER_ADDR_LEN];
  unsigned int  ifindex;
  int static_flag;
};

#ifdef HAVE_VRRP
enum vrrp_hal_msg_code
  {
    HSL_VRRP_VIP_ADD = 1,
    HSL_VRRP_VIP_DEL,
  };

struct hal_msg_vrrp_msg
  {
    /* Msg type. */
    unsigned char msg_type;

    /* VrId. */
    int vrid;

    /* Address family */
    u_int8_t af_type;

    /* Virtual IP address. */
    struct hal_in4_addr vip_v4;

#ifdef HAVE_IPV6
   /* Virtual IPv6 address. */
   struct hal_in6_addr vip_v6;
#endif /*HAVE_IPV6 */

   /* Interface mapping. */
   s_int32_t ifindex;
};
#endif /* HAVE_VRRP */

#endif /* _HAL_L3_ */
