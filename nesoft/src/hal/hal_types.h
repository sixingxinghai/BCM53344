/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_TYPES_H_
#define _HAL_TYPES_H_

/* Interface name length. */
#define HAL_IFNAME_LEN                                      20

/* Bridge name length. */
#define HAL_BRIDGE_NAME_LEN                                 16

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN                         6
#endif /* ETHER_ADDR_LEN */


/* MAC address length. */
#define HAL_HW_LENGTH                                       ETHER_ADDR_LEN

/* Default and Max values */
#define HAL_VLAN_NAME_LEN                                   32 
#define HAL_DEFAULT_VLAN_ID                                 1
#define HAL_MAX_VLAN_ID                                     4094
#define HAL_BRIDGE_MAX_TRAFFIC_CLASS                        8 
#define HAL_BRIDGE_MAX_USER_PRIO                            7
#define HAL_BRIDGE_MIN_TRAFFIC_CLASS                        1
#define HAL_BRIDGE_MIN_USER_PRIO                            0
#ifdef HAVE_PBR
#define HAL_RMAP_NAME_LEN                       255
#endif /* HAVE_PBR */


typedef enum
{ 
  HAL_IF_TYPE_UNK      = 0,      /* Unknown.  */ 
  HAL_IF_TYPE_LOOPBACK = 1,      /* Loopback. */ 
  HAL_IF_TYPE_IP       = 2,      /* IP.       */ 
  HAL_IF_TYPE_ETHERNET = 3,      /* Ethernet. */ 
  HAL_IF_TYPE_VLAN     = 4,      /* SVI.      */ 
  HAL_IF_TYPE_PPP      = 5,      /* PPP.      */  
  HAL_IF_TYPE_TUNNEL   = 6       /* Tunnel.   */
} hal_ifType_t;

/* 802.1x port state. */
enum hal_auth_port_state
  {
    HAL_AUTH_PORT_STATE_BLOCK_INOUT,
    HAL_AUTH_PORT_STATE_BLOCK_IN,
    HAL_AUTH_PORT_STATE_UNBLOCK,
    HAL_AUTH_PORT_STATE_UNCONTROLLED
  };

/*MAC-based authentication Enhancement*/
enum hal_auth_mac_port_state
  {
    HAL_MACAUTH_PORT_STATE_ENABLED,
    HAL_MACAUTH_PORT_STATE_DISABLED,
  };

/* IPv4 address. */
struct hal_in4_addr
{
  u_int32_t s_addr;
};

#ifdef HAVE_IPV6

struct hal_in6_addr
{
  union
  {
    u_int8_t  u6_addr8[16];
    u_int16_t u6_addr16[8];
    u_int32_t u6_addr32[4];
  } in6_u;
};

struct hal_in6_header
{
  union
  {
    struct hal_ip6_hdrctl
    {
      uint32_t ip6_un1_flow;   /* 4 bits version, 8 bits TC,
                               20 bits flow-ID */
      uint16_t ip6_un1_plen;   /* payload length */
      uint8_t  ip6_un1_nxt;    /* next header */
      uint8_t  ip6_un1_hlim;   /* hop limit */
    } hal_ip6_un1;
        uint8_t ip6_un2_vfc;       /* 4 bits version, top 4 bits tclass */
  } hal_ip6_ctlun;

  struct hal_in6_addr ip6_src;      /* source address */
  struct hal_in6_addr ip6_dst;      /* destination address */
};

struct hal_in6_pktinfo 
{
  struct hal_in6_addr ipi6_addr;
  int    ifindex;
};


#define IPV6_ADDR_ZERO(addr) (((addr).in6_u.u6_addr32[0] == 0) && ((addr).in6_u.u6_addr32[1] == 0) && \
                              ((addr).in6_u.u6_addr32[2] == 0) && ((addr).in6_u.u6_addr32[3] == 0))
#endif /* HAVE_IPV6 */

#ifdef HAVE_PBR
struct hal_prefix
{
  u_int8_t family;
  u_int8_t prefixlen;
  u_int8_t pad1;
  u_int8_t pad2;
  union
    {
      u_int8_t prefix;
      struct hal_in4_addr prefix4;
#ifdef HAVE_IPV6
      struct hal_in6_addr prefix6;
#endif /* HAVE_IPV6 */
      struct
       {
         struct hal_in4_addr id;
         struct hal_in4_addr adv_router;
       } lp;
      u_int8_t val[9];
    } u;
};
#endif /* HAVE_PBR */

/* Some simple macros for Ethernet addresses. */
#define HAL_IS_ETH_BROADCAST(addr)                                                  \
    (((unsigned char *) (addr))[0] == ((unsigned char) 0xff))

#define HAL_IS_ETH_MULTICAST(addr)                                                  \
    (((unsigned char *) (addr))[0] & ((unsigned char) 0x1))

#define HAL_IS_ETH_ADDRESS_EQUAL(A,B)                                               \
    ((((unsigned char *) (A))[0] == ((unsigned char *) (B))[0]) &&                  \
    (((unsigned char *) (A))[1] == ((unsigned char *) (B))[1]) &&                   \
    (((unsigned char *) (A))[2] == ((unsigned char *) (B))[2]) &&                   \
    (((unsigned char *) (A))[3] == ((unsigned char *) (B))[3]) &&                   \
    (((unsigned char *) (A))[4] == ((unsigned char *) (B))[4]) &&                   \
    (((unsigned char *) (A))[5] == ((unsigned char *) (B))[5]))

#define HAL_COPY_ETH_ADDRESS(DST,SRC)                                               \
  do {                                                                              \
    ((unsigned char *) (DST))[0] = ((unsigned char *) (SRC))[0];                    \
    ((unsigned char *) (DST))[1] = ((unsigned char *) (SRC))[1];                    \
    ((unsigned char *) (DST))[2] = ((unsigned char *) (SRC))[2];                    \
    ((unsigned char *) (DST))[3] = ((unsigned char *) (SRC))[3];                    \
    ((unsigned char *) (DST))[4] = ((unsigned char *) (SRC))[4];                    \
    ((unsigned char *) (DST))[5] = ((unsigned char *) (SRC))[5];                    \
  } while (0)

#ifdef HAVE_PVLAN
enum hal_pvlan_type
  {
    HAL_PVLAN_NONE,
    HAL_PVLAN_COMMUNITY,
    HAL_PVLAN_ISOLATED,
    HAL_PVLAN_PRIMARY
  };
                                                                                
enum hal_pvlan_port_mode
  {
    HAL_PVLAN_PORT_MODE_INVALID,
    HAL_PVLAN_PORT_MODE_HOST,
    HAL_PVLAN_PORT_MODE_PROMISCUOUS
  };
#endif
struct hal_nsm_callbacks
{
  void (*nsm_connected_delete_ipv4_cb) (struct interface *, int, struct pal_in4_addr *,
                                                  int, struct pal_in4_addr *);
  void (*nsm_connected_delete_ipv6_cb) (struct interface *, struct pal_in6_addr *,
                                        s_int32_t , struct pal_in6_addr *);
  struct connected * (*nsm_connected_add_ipv4_cb) (struct interface *ifp, u_char flags,
                                               struct pal_in4_addr *addr, u_char prefixlen,
                                                        struct pal_in4_addr *broad);
  struct connected * (*nsm_connected_add_ipv6_cb) (struct interface *ifp, struct pal_in6_addr *addr,
                                                s_int32_t prefixlen, struct pal_in6_addr *broad);

  void (*nsm_if_delete_update_cb) (struct interface *);
  void (*nsm_if_ifindex_update_cb) (struct interface *, int);
  void (*nsm_if_add_update_cb) (struct interface *, u_int32_t);
  void (*nsm_if_up_cb) (struct interface *);
  void (*nsm_if_down_cb) (struct interface *);
  void (*nsm_server_if_up_update_cb) (struct interface *, u_int32_t);
  void (*nsm_server_if_down_update_cb) (struct interface *, u_int32_t);
#ifdef HAVE_EFM
  int (*nsm_efm_oam_process_local_event_cb) (struct interface *ifp,
                                  u_int16_t event, u_int8_t enable);
  int (*nsm_efm_oam_process_period_error_event_cb) (struct interface *ifp,
                                 enum nsm_efm_opcode opcode, u_int16_t no_of_errors);
#endif /* HAVE_EFM */
  void (*nsm_bridge_if_send_state_sync_req_wrap_cb) ( struct interface *ifp);
  int (*nsm_set_ecmp_cb) (int ecmp);
};

#endif /* _HAL_TYPES_H_ */
